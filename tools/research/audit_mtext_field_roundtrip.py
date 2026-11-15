#!/usr/bin/env python3
"""Audit an isolated pinned FIELD record through libdxfrw and ezdxf.

Requires ezdxf and the libdxfrw_dxf_field_mtext_tests executable. No source
drawing or generated output is copied into the repository. This verifies one
record's text transport, not its surrounding graph or dynamic evaluation.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

import ezdxf
from ezdxf.tools.text import plain_mtext
from audit_mtext_field_source import audit, handle, parse


def digest(value):
    return hashlib.sha256(value.encode('utf-8')).hexdigest()


def main():
    if len(sys.argv) != 3:
        raise SystemExit('usage: audit_mtext_field_roundtrip.py source.dxf test-executable')
    source_path = Path(sys.argv[1])
    source = audit(source_path)
    raw = source_path.read_bytes()
    record = next(r for r in parse(raw) if r[0][1] == 'FIELD' and handle(r) == 'A9')
    frame = raw[record[0][2]:record[-1][3]]
    with tempfile.TemporaryDirectory(prefix='field-mtext-roundtrip-') as temporary:
        directory = Path(temporary)
        isolated = directory / 'isolated.dxf'
        isolated.write_bytes(b'0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1027\n'
                             b'0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n' + frame
                             + b'0\nENDSEC\n0\nEOF\n')
        outputs = [directory / 'ascii.dxf', directory / 'binary.dxf']
        subprocess.run([sys.argv[2], '--source-record', str(isolated),
                        *(str(p) for p in outputs)], check=True, timeout=60,
                       capture_output=True, text=True)
        results = []
        for output in outputs:
            fields = list(ezdxf.readfile(output).objects.query('FIELD'))
            if len(fields) != 1:
                raise ValueError('peer did not recover one FIELD')
            tags = list(fields[0].xtags.subclasses[-1])
            code_end = next(i for i, t in enumerate(tags) if t.code == 90)
            start = next(i for i, t in enumerate(tags) if t.code == 7)
            end = next(i for i in range(start, len(tags)) if tags[i].code == 304)
            code = ''.join(t.value for t in tags[:code_end] if t.code in (2, 3))
            value = ''.join(t.value for t in tags[start:end] if t.code in (1, 2))
            cache = ''.join(t.value for t in tags[end + 1:] if t.code in (9, 301))
            expected = source['field_details']['A9']
            if (digest(code) != expected['expression_sha256']
                    or digest(value) != expected['cached_sha256']
                    or digest(cache) != expected['cached_sha256']
                    or '1^2' not in plain_mtext(cache)):
                raise ValueError('peer FIELD text or formatted fraction differs')
            results.append({'encoding': output.stem, 'code_sha256': digest(code),
                            'value_sha256': digest(value), 'cache_sha256': digest(cache),
                            'field_code_continuations': sum(t.code == 3 for t in tags[:code_end]),
                            'cache_continuations': sum(t.code == 9 for t in tags[end + 1:])})
    print(json.dumps({'source_sha256': source['source_sha256'], 'field': 'A9',
                      'peer': 'ezdxf ' + ezdxf.__version__, 'outputs': results,
                      'limit': 'Isolated FIELD text only; graph and evaluation remain unqualified.'},
                     indent=2))


if __name__ == '__main__':
    main()
