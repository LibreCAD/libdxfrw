#!/usr/bin/env python3
"""Fast negative and framing controls for the source-bound FIELD audit."""
import importlib.util
from pathlib import Path
import tempfile
import unittest

path = Path(__file__).resolve().parents[1] / 'tools/research/audit_mtext_field_source.py'
spec = importlib.util.spec_from_file_location('field_audit', path)
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


def wire(member_count='2', members='330\nA9\n330\nCD\n'):
    return (f'0\nFIELDLIST\n5\nAB\n102\n{{ACAD_REACTORS\n330\nC\n'
            f'102\n}}\n330\nC\n100\nAcDbIdSet\n90\n{member_count}\n'
            f'{members}100\nAcDbFieldList\n0\nEOF\n').encode('utf-8')


class FieldAuditControls(unittest.TestCase):
    def test_membership_owner_and_frame_bytes(self):
        data = wire()
        record = audit.parse(data)[0]
        self.assertEqual(audit.common_owner(record), 'C')
        self.assertEqual(audit.fieldlist_members(record), ['A9', 'CD'])
        info = audit.frame(record, data)
        self.assertEqual(data[info['offset']:info['offset'] + info['length']],
                         data[:-len(b'0\nEOF\n')])
        self.assertEqual(info['sha256'], audit.digest(data[:info['length']]))

    def test_wrong_count_and_duplicate_members_fail(self):
        with self.assertRaises(ValueError):
            audit.fieldlist_members(audit.parse(wire(member_count='3'))[0])
        with self.assertRaises(ValueError):
            audit.fieldlist_members(audit.parse(wire(members='330\nA9\n330\nA9\n'))[0])

    def test_truncated_framing_and_binary_refused(self):
        with self.assertRaises(ValueError):
            audit.parse(wire()[:-3])
        with self.assertRaises(ValueError):
            audit.parse(b'AutoCAD Binary DXF\r\n\x1a\x00')

    def test_dictionary_roles_and_missing_target(self):
        data = (b'0\nDICTIONARY\n5\nAA\n330\nA1\n100\nAcDbDictionary\n'
                b'3\nTEXT\n360\nA9\n0\nEOF\n')
        record = audit.parse(data)[0]
        self.assertEqual(audit.dictionary_entries(record), {'TEXT': (360, 'A9')})
        with self.assertRaises(ValueError):
            audit.dictionary_entries(audit.parse(data.replace(b'360\nA9\n', b''))[0])

    def test_wrong_source_hash_refused(self):
        with tempfile.TemporaryDirectory() as temp:
            path = Path(temp) / 'scratch.dxf'
            path.write_bytes(wire())
            with self.assertRaises(ValueError):
                audit.audit(path)


if __name__ == '__main__':
    unittest.main()
