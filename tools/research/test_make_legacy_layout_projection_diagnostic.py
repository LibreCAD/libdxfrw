#!/usr/bin/env python3
"""Fast refusal controls for the scratch-only diagnostic generator."""
import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / 'tools/research/make_legacy_layout_projection_diagnostic.py'
SOURCE = ROOT / 'dwg2dxf/dx_iface.cpp'


def sha(data):
    return hashlib.sha256(data).hexdigest()


class DiagnosticControls(unittest.TestCase):
    def test_hash_anchor_destination_and_retry(self):
        original = SOURCE.read_bytes()
        with tempfile.TemporaryDirectory(prefix='legacy-projection-generator-') as directory:
            root = Path(directory)
            output = root / 'diagnostic.cpp'
            def run(source, digest, target, *options):
                return subprocess.run([sys.executable, '-B', str(TOOL), '--source', str(source),
                                       '--sha256', digest, '--output', str(target), *options],
                                      capture_output=True, text=True)
            self.assertNotEqual(run(SOURCE, '0' * 64, output).returncode, 0)
            self.assertFalse(output.exists())
            altered = root / 'altered.cpp'
            anchor = b'if (legacyForward)\n        return reject('
            self.assertEqual(original.count(anchor), 1)
            altered.write_bytes(original.replace(anchor, b'if (false)\n        return reject(', 1))
            self.assertNotEqual(run(altered, sha(altered.read_bytes()), output).returncode, 0)
            self.assertFalse(output.exists())
            self.assertNotEqual(run(SOURCE, sha(original), ROOT / 'diagnostic.cpp').returncode, 0)
            shadow = root / 'dx_iface.h'
            shadow.write_text('unexpected local header')
            self.assertNotEqual(run(SOURCE, sha(original), output).returncode, 0)
            self.assertFalse(output.exists())
            shadow.unlink()
            self.assertEqual(run(SOURCE, sha(original), output).returncode, 0)
            generated = output.read_bytes()
            self.assertEqual(generated.count(b'UNSAFE SCRATCH DIAGNOSTIC'), 4)
            self.assertNotEqual(run(SOURCE, sha(original), output).returncode, 0)
            self.assertEqual(output.read_bytes(), generated)
            second = root / 'retry.cpp'
            self.assertEqual(run(SOURCE, sha(original), second).returncode, 0)
            self.assertEqual(second.read_bytes(), generated)
            injected = root / 'fault.cpp'
            self.assertEqual(run(SOURCE, sha(original), injected,
                                 '--inject-table-proof-failure').returncode, 0)
            self.assertIn(b'LIBDXFRW_DIAGNOSTIC_BAD_LAYOUT', injected.read_bytes())
            self.assertEqual(injected.read_bytes().count(b'UNSAFE SCRATCH DIAGNOSTIC'), 5)
            self.assertEqual(SOURCE.read_bytes(), original)


if __name__ == '__main__':
    unittest.main()
