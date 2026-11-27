#!/usr/bin/env python3
"""Fast controls for the FreeCAD patch replay's source and line-ending guards."""
import contextlib
import hashlib
import importlib.util
import io
from pathlib import Path
import shutil
import sys
import tarfile
import tempfile
import unittest
from unittest import mock

path = Path(__file__).resolve().parents[1] / 'tools/research/replay_freecad_import_patches.py'
spec = importlib.util.spec_from_file_location('freecad_replay', path)
replay = importlib.util.module_from_spec(spec)
spec.loader.exec_module(replay)


@unittest.skipUnless(shutil.which('git'), 'git is required for patch replay')
class ReplayLineEndings(unittest.TestCase):
    def run_replay(self, data, preserve, expected_error=None):
        with tempfile.TemporaryDirectory(prefix='freecad-replay-test-') as temp:
            root = Path(temp)
            archive = root / 'source.tar.gz'
            commit = 'synthetic'
            relative = 'src/test.h'
            with tarfile.open(archive, 'w:gz') as stream:
                member = tarfile.TarInfo(f'FreeCAD-{commit}/{relative}')
                member.size = len(data)
                stream.addfile(member, io.BytesIO(data))
            digest = hashlib.sha256(archive.read_bytes()).hexdigest()
            patches = root / 'patches'
            patches.mkdir()
            (patches / 'change.patch').write_text(
                'diff --git a/src/test.h b/src/test.h\n'
                '--- a/src/test.h\n+++ b/src/test.h\n'
                '@@ -1,2 +1,2 @@\n-old\n+new\n keep\n')
            output = root / 'output'
            output.mkdir()
            args = ['replay', '--source-archive', str(archive), '--output-dir', str(output),
                    '--patch-dir', str(patches)]
            with mock.patch.object(replay, 'ARCHIVES', {digest: commit}), \
                 mock.patch.object(replay, 'FILES', (relative,)), \
                 mock.patch.object(replay, 'PATCHES', ('change.patch',)), \
                 mock.patch.object(replay, 'PRESERVE_NEWLINES', {commit} if preserve else set()), \
                 mock.patch.object(sys, 'argv', args), contextlib.redirect_stdout(io.StringIO()):
                if expected_error:
                    with self.assertRaisesRegex(ValueError, expected_error):
                        replay.main()
                    self.assertFalse(any(output.iterdir()))
                    return None
                replay.main()
            return (output / relative).read_bytes()

    def test_crlf_and_lf_sources_preserve_their_endings(self):
        self.assertEqual(self.run_replay(b'old\r\nkeep\r\n', True), b'new\r\nkeep\r\n')
        self.assertEqual(self.run_replay(b'old\nkeep\n', True), b'new\nkeep\n')

    def test_mixed_source_endings_refused_before_publication(self):
        self.run_replay(b'old\r\nkeep\n', True, 'mixed source line endings')

    def test_legacy_baseline_retains_previous_normalization(self):
        self.assertEqual(self.run_replay(b'old\r\nkeep\r\n', False), b'new\nkeep\n')


if __name__ == '__main__':
    unittest.main()
