#!/usr/bin/env python3
"""Check the generated forward-only-to-R2000 graph using raw tags and ezdxf.

This runs the in-tree fixture test, retains its scratch output just long enough
for an independent peer check, then removes it. No external drawings are
created in or copied to the repository. This is not legacy DWG qualification.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

from audit_binary_dxf_layout_nod import (layout_report, normalize_handle,
                                         read_pairs, read_records)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def physical_records(path: Path, binary: bool):
    data = path.read_bytes()
    if binary:
        pairs = read_pairs(data)
    else:
        lines = data.decode('utf-8').splitlines()
        require(len(lines) % 2 == 0, 'unpaired ASCII DXF tags')
        pairs = tuple((int(lines[i].strip()), lines[i + 1].strip())
                      for i in range(0, len(lines), 2))
    return read_records(pairs)


def physical_layout_class(path: Path, binary: bool, instance_count=None) -> dict:
    rows = [record for record in physical_records(path, binary)
            if record.section == 'CLASSES' and record.type == 'CLASS'
            and any((code == 1 and str(value).upper() == 'LAYOUT')
                    or (code == 2 and str(value).upper() == 'ACDBLAYOUT')
                    for code, value in record.values)]
    require(len(rows) == 1, 'missing or ambiguous physical LAYOUT CLASS')
    expected = {1: 'LAYOUT', 2: 'AcDbLayout', 3: 'ObjectDBX Classes',
                90: '0', 280: '0', 281: '0'}
    for code, expected_value in expected.items():
        values = [str(value) for actual, value in rows[0].values if actual == code]
        require(values == [expected_value], 'incorrect physical LAYOUT CLASS field')
    counts = [int(value) for code, value in rows[0].values if code == 91]
    require(counts == ([] if instance_count is None else [instance_count]),
            'incorrect versioned LAYOUT CLASS instance count')
    return {'record': expected[1], 'class': expected[2], 'app': expected[3],
            'proxyFlag': 0, 'wasProxy': 0, 'entity': 0,
            'instanceCount': instance_count}


def physical_graph(path: Path, binary: bool, expected_blocks=None) -> dict:
    if expected_blocks is None:
        expected_blocks = {'Model': '*Model_Space', 'Sheet A': '*Paper_Space',
                           'Sheet B': '*Paper_Space0'}
    records = physical_records(path, binary)
    blocks = {}
    layouts = {}
    for record in records:
        if record.type == 'BLOCK_RECORD' and record.section == 'TABLES':
            names = [v for c, v in record.values if c == 2]
            handles = [v for c, v in record.values if c == 5]
            links = [normalize_handle(v) for c, v in record.values if c == 340]
            require(record.handle and len(handles) == 1 and len(names) == 1 and len(links) == 1
                    and links[0] and record.handle not in blocks,
                    'invalid physical BLOCK_RECORD identity or backlink')
            blocks[record.handle] = {'name': str(names[0]), 'layout': links[0]}
        elif record.type == 'LAYOUT' and record.section == 'OBJECTS':
            row = layout_report(record)
            handles = [v for c, v in record.values if c == 5]
            body_start = record.values.index((100, 'AcDbLayout')) + 1
            links = [normalize_handle(v) for c, v in record.values[body_start:]
                     if c == 330]
            require(row['handle'] and row['name'] and len(handles) == 1 and len(links) == 1
                    and links[0] and row['name'] not in layouts,
                    'invalid physical LAYOUT identity or block link')
            layouts[row['name']] = {'handle': row['handle'],
                                    'block': links[0]}
    require(set(layouts) == set(expected_blocks)
            and len(blocks) == len(expected_blocks), 'unexpected layout profile')
    for row in layouts.values():
        block = blocks.get(row['block'])
        require(block is not None and block['layout'] == row['handle'],
                'physical layout links do not agree')
        row['blockName'] = block['name']
    require(len({row['handle'] for row in layouts.values()}) == len(expected_blocks)
            and {name: row['blockName'] for name, row in layouts.items()} == expected_blocks,
            'physical layout handles or block roles are wrong')
    return layouts


def peer_graph(path: Path, ezdxf, version='AC1015') -> tuple[dict, int, int]:
    doc = ezdxf.readfile(path)
    require(doc.dxfversion == version, 'unexpected peer DXF version')
    dictionary = doc.rootdict['ACAD_LAYOUT']
    result = {}
    for name in dictionary.keys():
        layout = dictionary[name]
        require(layout.dxftype() == 'LAYOUT', 'peer dictionary target is not LAYOUT')
        block_handle = normalize_handle(layout.dxf.block_record_handle)
        block = doc.entitydb.get(block_handle)
        require(block is not None and block.dxftype() == 'BLOCK_RECORD',
                'peer layout block target missing')
        layout_handle = normalize_handle(layout.dxf.handle)
        require(normalize_handle(block.dxf.layout) == layout_handle,
                'peer block backlink differs')
        result[name] = {'handle': layout_handle, 'block': block_handle,
                        'blockName': block.dxf.name}
    audit = doc.audit()
    return result, len(audit.errors), len(audit.fixes)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture-test', type=Path, required=True,
                        help='built libdxfrw_dxf_fixture_tests executable')
    args = parser.parse_args()
    import ezdxf  # Separately installed independent reader.

    executable = args.fixture_test.resolve(strict=True)
    environment = os.environ.copy()
    environment['LIBDXFRW_KEEP_LAYOUT_GRAPH_CONTROL'] = '1'
    run = subprocess.run([str(executable), '--layout-graph-read-contract'],
                         env=environment, capture_output=True, text=True,
                         timeout=60, check=False)
    marker = 'saved layout graph controls: '
    rows = [line.split(marker, 1)[1] for line in run.stderr.splitlines()
            if marker in line]
    require(len(rows) == 1, 'fixture did not report exactly one scratch directory')
    # std::filesystem streams quote paths; parse the quoted path as JSON.
    directory = Path(json.loads(rows[0])).resolve(strict=True)
    require(directory.parent == Path(tempfile.gettempdir()).resolve()
            and directory.name.startswith('libdxfrw-layout-graph-'),
            'unexpected scratch directory; refusing cleanup')
    try:
        require(run.returncode == 0, 'fixture test failed: ' + run.stderr[-3000:])
        result = {'schema': 1,
                  'scope': 'generated R2000-R2013 DXF projection controls; no legacy DWG or native semantics qualification',
                  'fixtureTestSha256': digest(executable),
                  'ezdxfVersion': ezdxf.__version__, 'outputs': {}}
        for encoding in ('ascii', 'binary'):
            path = directory / ('layout-graph-export-AC1015-' + encoding + '.dxf')
            wire = physical_graph(path, encoding == 'binary')
            layout_class = physical_layout_class(path, encoding == 'binary')
            peer, errors, fixes = peer_graph(path, ezdxf)
            require(wire == peer, encoding + ': physical and peer graph differ')
            require(errors == 0 and fixes == 0, encoding + ': peer audit made fixes or reported errors')
            # A peer can repair relationships while loading. Check that the
            # original-wire observer rejects a broken backlink before relying
            # on the agreement above, and that the original remains readable.
            target = wire['Sheet A']['handle'].encode('ascii')
            prefix = b'\x54\x01' if encoding == 'binary' else b'340\n'
            terminator = b'\0' if encoding == 'binary' else b'\n'
            needle = prefix + target + terminator
            original = path.read_bytes()
            class_needle = (b'\x02\x00AcDbLayout\x00' if encoding == 'binary'
                            else b'2\nAcDbLayout\n')
            require(original.count(class_needle) == 1,
                    encoding + ': cannot uniquely locate the physical LAYOUT CLASS')
            bad_class = directory / ('bad-class-' + encoding + '.dxf')
            bad_class.write_bytes(original.replace(
                class_needle, class_needle.replace(b'AcDbLayout', b'AcDbLayouT'), 1))
            try:
                physical_layout_class(bad_class, encoding == 'binary')
            except ValueError:
                pass
            else:
                raise ValueError(encoding + ': damaged LAYOUT CLASS was accepted')
            require(physical_layout_class(path, encoding == 'binary') == layout_class,
                    encoding + ': original class failed after malformed control')
            require(original.count(needle) == 1,
                    encoding + ': cannot uniquely locate the selected backlink')
            damaged = directory / ('damaged-' + encoding + '.dxf')
            damaged.write_bytes(original.replace(needle, prefix + b'0' + terminator, 1))
            try:
                physical_graph(damaged, encoding == 'binary')
            except ValueError:
                pass
            else:
                raise ValueError(encoding + ': broken backlink was accepted')
            require(physical_graph(path, encoding == 'binary') == wire,
                    encoding + ': original failed after malformed control')
            result['outputs'][encoding] = {'sha256': digest(path),
                                           'layouts': wire,
                                           'layoutClass': layout_class,
                                           'brokenClassRefused': True,
                                           'auditErrors': errors,
                                           'auditFixes': fixes,
                                           'brokenBacklinkRefused': True,
                                           'originalRetryPassed': True}
        result['laterVersions'] = {}
        for version in ('AC1018', 'AC1021', 'AC1024', 'AC1027'):
            result['laterVersions'][version] = {}
            for encoding in ('ascii', 'binary'):
                path = directory / ('layout-graph-export-' + version + '-' + encoding + '.dxf')
                wire = physical_graph(path, encoding == 'binary')
                class_row = physical_layout_class(path, encoding == 'binary', 3)
                peer, errors, fixes = peer_graph(path, ezdxf, version)
                require(wire == peer and errors == 0 and fixes == 0,
                        version + ' ' + encoding + ': peer graph/audit differs')
                result['laterVersions'][version][encoding] = {
                    'sha256': digest(path), 'layoutClass': class_row,
                    'auditErrors': errors, 'auditFixes': fixes}
        print(json.dumps(result, indent=2, sort_keys=True))
    finally:
        shutil.rmtree(directory)


if __name__ == '__main__':
    main()
