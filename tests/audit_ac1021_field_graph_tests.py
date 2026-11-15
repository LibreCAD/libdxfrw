#!/usr/bin/env python3
"""Negative controls for the FIELD graph evidence checker."""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools/research'))
from audit_ac1021_field_graph import inspect_graph, sha  # noqa: E402


def graph():
    ref = lambda h: [4, 1, h, h]
    source = {'fieldlist': '2', 'fieldlist_owner': '1', 'members': [], 'roots': []}
    objects = [
        {'object': 'DICTIONARY', 'handle': ref(1),
         'items': {'ACAD_FIELDLIST': ref(2)}},
        {'object': 'FIELDLIST', 'handle': ref(2), 'ownerhandle': ref(1), 'fields': []},
    ]
    for n in range(8):
        m, x, d, p, c = (10 + n * 5 + i for i in range(5))
        source['members'].extend([f'{p:X}', f'{c:X}'])
        source['roots'].append({'mtext': f'{m:X}', 'extension_dictionary': f'{x:X}',
            'field_dictionary': f'{d:X}', 'root_field': f'{p:X}', 'child_field': f'{c:X}',
            'expression': 'expr', 'child_cached_sha256': sha('cached'),
            'root_cached_sha256': sha('visible')})
        objects[1]['fields'].extend([ref(p), ref(c)])
        objects.extend([
            {'entity': 'MTEXT', 'handle': ref(m), 'xdicobjhandle': ref(x), 'text': 'visible'},
            {'object': 'DICTIONARY', 'handle': ref(x), 'ownerhandle': ref(m),
             'items': {'ACAD_FIELD': ref(d)}},
            {'object': 'DICTIONARY', 'handle': ref(d), 'ownerhandle': ref(x),
             'items': {'TEXT': ref(p)}},
            {'object': 'FIELD', 'handle': ref(p), 'ownerhandle': ref(d),
             'id': '_text', 'value_string': 'visible', 'childs': [ref(c)]},
            {'object': 'FIELD', 'handle': ref(c), 'ownerhandle': ref(p),
             'id': 'AcVar', 'code': 'expr', 'value_string': 'cached'},
        ])
    return source, objects


class GraphAuditTest(unittest.TestCase):
    def test_closed_graph(self):
        source, objects = graph()
        result = inspect_graph(source, objects)
        self.assertEqual(result['fieldlist_members'], 16)
        self.assertTrue(all(x['source_cache_bytes_equal'] for x in result['mtext_roots']))

    def test_reject_duplicate_missing_or_wrong_member(self):
        source, objects = graph()
        for mode in ('duplicate', 'missing', 'wrong'):
            altered = copy.deepcopy(objects)
            members = altered[1]['fields']
            if mode == 'duplicate':
                members[-1] = members[0]
            elif mode == 'missing':
                members.pop()
            else:
                members[-1] = [4, 1, 999, 999]
            with self.subTest(mode=mode), self.assertRaises(ValueError):
                inspect_graph(source, altered)

    def test_reject_changed_owner_expression_or_cache(self):
        source, objects = graph()
        for mode in ('owner', 'expression', 'cache'):
            altered = copy.deepcopy(objects)
            if mode == 'owner':
                altered[6]['ownerhandle'] = [4, 1, 999, 999]
            elif mode == 'expression':
                altered[6]['code'] = 'different'
            else:
                altered[5]['value_string'] = 'different'
            with self.subTest(mode=mode), self.assertRaises(ValueError):
                inspect_graph(source, altered)


if __name__ == '__main__':
    unittest.main()
