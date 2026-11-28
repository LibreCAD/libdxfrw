#!/usr/bin/env python3
"""Compare selected AC1012 layout graphs with supplied ODA DXF readback.

Generated JSON is a bounded peer observation, not native semantic qualification.
Input/output DWG and DXF files remain outside the repository.
"""
import argparse
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path

import ezdxf
from ezdxf.lldxf.tagger import ascii_tags_loader


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(condition, message):
    if not condition:
        raise ValueError(message)


def handle(value):
    require(isinstance(value, list) and len(value) >= 3, 'invalid LibreDWG handle')
    return value[-1]


def physical_block_links(path):
    # Read the physical ASCII tags before ezdxf's document loader can repair
    # associations. Restrict this check to the two selected space records.
    result = {}
    current = None

    def finish():
        if current is None:
            return
        names = [value for code, value in current if code == 2]
        if len(names) != 1 or names[0] not in ('*Model_Space', '*Paper_Space'):
            return
        handles = [value for code, value in current if code == 5]
        backlinks = [value for code, value in current if code == 340]
        require(len(handles) == 1 and len(backlinks) == 1
                and names[0] not in result, 'invalid physical BLOCK_RECORD links')
        result[names[0]] = (int(handles[0], 16), int(backlinks[0], 16))

    with path.open('rt', encoding='utf-8') as stream:
        for tag in ascii_tags_loader(stream):
            if tag.code == 0:
                finish()
                current = [] if tag.value == 'BLOCK_RECORD' else None
            elif current is not None:
                current.append((tag.code, tag.value))
    finish()
    require(set(result) == {'*Model_Space', '*Paper_Space'},
            'missing physical space BLOCK_RECORD')
    return result


def audit(dwg, dxf, dwgread, resaved):
    with tempfile.TemporaryDirectory(prefix='ac1012-layout-peer-') as temporary:
        raw = Path(temporary) / 'peer.json'
        result = subprocess.run([dwgread, '-O', 'JSON', '-o', str(raw), str(dwg)],
                                capture_output=True, text=True, check=False)
        require(result.returncode == 0 and raw.is_file(), 'LibreDWG decode failed')
        peer = json.loads(raw.read_text())
    objects = peer['OBJECTS']
    def obj(kind, ident):
        found = [row for row in objects if row.get('object') == kind
                 and handle(row.get('handle')) == ident]
        require(len(found) == 1, 'missing or ambiguous ' + kind)
        return found[0]
    roots = [row for row in objects if row.get('object') == 'DICTIONARY'
             and handle(row.get('ownerhandle')) == 0]
    require(len(roots) == 1, 'missing or ambiguous root dictionary')
    root = roots[0]
    layouts = [row for row in objects if row.get('object') == 'LAYOUT']
    require(len(layouts) == 2, 'expected two layout objects')
    by_name = {row['layout_name']: row for row in layouts}
    require(set(by_name) == {'Model', 'Layout1'}, 'unexpected layout names')
    owner = handle(by_name['Model']['ownerhandle'])
    require(owner == handle(by_name['Layout1']['ownerhandle']), 'layout owners differ')
    dictionary = obj('DICTIONARY', owner)
    require(handle(dictionary['ownerhandle']) == handle(root['handle']), 'layout dictionary owner mismatch')
    require({name: handle(value) for name, value in dictionary['items'].items()}
            == {name: handle(row['handle']) for name, row in by_name.items()},
            'layout dictionary membership mismatch')
    root_entry = root['items'].get('ACAD_LAYOUT')
    require((root_entry is not None) == resaved, 'unexpected source root membership')
    if root_entry is not None:
        require(handle(root_entry) == owner, 'source root target mismatch')
    source = {}
    for name, expected_block in [('Model', '*Model_Space'), ('Layout1', '*Paper_Space')]:
        row = by_name[name]
        block = obj('BLOCK_HEADER', handle(row['block_header']))
        require(block['name'] == expected_block, 'source block name mismatch')
        source[name] = {'layout': handle(row['handle']), 'block': handle(block['handle']),
                        'activeViewport': handle(row['active_viewport'])}
    wire_links = physical_block_links(dxf)
    document = ezdxf.readfile(dxf)
    report = document.audit()
    require(not report.errors and not report.fixes, 'DXF audit reported errors or fixes')
    require('ACAD_LAYOUT' in document.rootdict, 'DXF layout root missing')
    layout_dict = document.rootdict['ACAD_LAYOUT']
    observed = {}
    for layout in document.layouts:
        require(layout.name in source and layout.name not in observed, 'unexpected DXF layout')
        record = layout.dxf_layout
        block = layout.block_record
        require(record.dxf.owner == layout_dict.dxf.handle, 'DXF layout owner mismatch')
        require(layout_dict.get(layout.name).dxf.handle == record.dxf.handle,
                'DXF layout dictionary target mismatch')
        require(block.dxf.layout == record.dxf.handle
                and record.dxf.block_record_handle == block.dxf.handle,
                'DXF reciprocal links mismatch')
        require(wire_links[block.dxf.name]
                == (int(block.dxf.handle, 16), int(record.dxf.handle, 16)),
                'physical DXF block backlink mismatch')
        require(int(block.dxf.handle, 16) == source[layout.name]['block'], 'DXF block identity changed')
        observed[layout.name] = {'layout': int(record.dxf.handle, 16),
                                 'block': int(block.dxf.handle, 16),
                                 'activeViewport': int(record.dxf.get('viewport_handle', '0'), 16)}
    require(set(observed) == set(source), 'missing DXF layout')
    for name in source:
        require((observed[name]['layout'] == source[name]['layout']) == resaved,
                'unexpected layout identity retention')
    return {'dwgSha256': digest(dwg), 'dxfSha256': digest(dxf),
            'sourceRootContainsLayout': root_entry is not None,
            'sourceDictionary': owner, 'source': source,
            'dxfDictionary': int(layout_dict.dxf.handle, 16), 'dxf': observed,
            'auditErrors': 0, 'auditFixes': 0}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sample-dir', type=Path, required=True)
    parser.add_argument('--oda-original-dir', type=Path, required=True)
    parser.add_argument('--oda-resaved-dir', type=Path, required=True)
    parser.add_argument('--dwgread', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    output = args.output.resolve()
    require(not output.is_relative_to(root), 'output must be outside the repository')
    manifest = json.loads((args.sample_dir / 'manifest.json').read_text())
    require(manifest.get('schema') == 'libdxfrw.ac1012-acadrust-pair.v1'
            and manifest.get('ok') is True, 'invalid sample manifest')
    observations = {}
    for side in ('destination', 'source'):
        for resaved in (False, True):
            stem = side + ('-resaved' if resaved else '')
            dwg = args.sample_dir / (stem + '.dwg')
            expected = manifest['samples'][dwg.name]
            require(dwg.is_file() and dwg.stat().st_size == expected['bytes']
                    and digest(dwg) == expected['sha256'], 'DWG does not match manifest')
            dxf_dir = args.oda_resaved_dir if resaved else args.oda_original_dir
            observations[stem] = audit(dwg, dxf_dir / (stem + '.dxf'), args.dwgread, resaved)
    receipt = {'schema': 'libdxfrw.ac1012-layout-peer.v1', 'ezdxf': ezdxf.__version__,
               'manifestSha256': digest(args.sample_dir / 'manifest.json'),
               'observations': observations, 'supportClaimEffect': 'none',
               'limitations': ['ODA DXFs are supplied inputs; this audit does not independently attest their conversion provenance.',
                               'Peer agreement is not native AutoCAD open or cloning evidence.',
                               'DXF layout identities synthesized by ODA for unrooted source layouts do not validate source ownership.']}
    with output.open('x', encoding='utf-8') as target:
        json.dump(receipt, target, indent=2, sort_keys=True)
        target.write('\n')
    print('AC1012 layout peer audit: PASS (4 drawings; no support claim)')


if __name__ == '__main__':
    main()
