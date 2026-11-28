#!/usr/bin/env python3
"""Generate ODA R13/R14 dictionary-ownership controls from scratch.

Requires ezdxf, LibreDWG dwgread and ODA File Converter. All drawings stay in
an output scratch directory. This is an ODA-family observation, not proof of
AutoCAD's cloning or duplicate-merge behavior.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

from audit_dxf_dictionary_cloning import audit_bytes
from generate_ac1012_dictionary_witness import executable_identity, physical_dxf_version
from verify_oda_legacy_layer_graph import dictionary_frame, traced_frame


def require(condition, message):
    if not condition:
        raise ValueError(message)


def run(*args, allow_failure=False):
    result = subprocess.run([str(arg) for arg in args], capture_output=True, timeout=90)
    if result.returncode and not allow_failure:
        raise RuntimeError(f'{args[0]} exited {result.returncode}: {result.stderr.decode(errors="replace")[-1000:]}')
    return result


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--oda', required=True)
    parser.add_argument('--dwgread', default='dwgread')
    parser.add_argument('--candidate', help='optional dwg2dxf candidate')
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    import ezdxf

    args.output_dir.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix='oda-dictionary-ownership-', dir=args.output_dir))
    seeds = root / 'seeds'; seeds.mkdir()
    expected_tags = [(1, 'from_scratch'), (90, 1234)]
    for hard in (0, 1):
        doc = ezdxf.new('R2000')
        doc.modelspace().add_line((1, 2, 3), (4, 6, 9))
        child = doc.rootdict.add_new_dict('TEST_DICT', hard_owned=bool(hard))
        record = child.add_xrecord('PAYLOAD')
        record.reset(expected_tags)
        require(record.dxf.owner == child.dxf.handle, 'seed XRECORD owner mismatch')
        doc.saveas(seeds / f'owner-{hard}.dxf')

    receipt = {'scope': 'two locally authored ODA-family dictionary ownership controls; no native semantics claim',
               'generatorSha256': sha(Path(__file__)),
               'oda': executable_identity(args.oda), 'dwgread': executable_identity(args.dwgread),
               'ezdxfVersion': ezdxf.__version__, 'versions': {}}
    if args.candidate:
        receipt['candidate'] = executable_identity(args.candidate)
    for version, target, magic in (('r13', 'ACAD13', b'AC1012'), ('r14', 'ACAD14', b'AC1014')):
        source_dir = root / version; source_dir.mkdir()
        output_dir = root / (version + '-reopen'); output_dir.mkdir()
        run(args.oda, seeds, source_dir, target, 'DWG', '0', '1', '*.dxf')
        run(args.oda, source_dir, output_dir, 'ACAD2000', 'DXF', '0', '1', '*.dwg')
        samples, frames = {}, []
        for hard in (0, 1):
            source = source_dir / f'owner-{hard}.dwg'
            output = output_dir / f'owner-{hard}.dxf'
            require(source.is_file() and source.read_bytes()[:6] == magic, 'source missing or wrong version')
            require(output.is_file() and physical_dxf_version(output.read_bytes()) == 'AC1015', 'reopen missing or wrong version')
            model_file = source_dir / f'owner-{hard}.json'
            run(args.dwgread, '-O', 'JSON', '-o', model_file, source)
            objects = json.loads(model_file.read_text())['OBJECTS']
            roots = [o for o in objects if o.get('object') == 'DICTIONARY' and 'TEST_DICT' in o.get('items', {})]
            require(len(roots) == 1, 'ambiguous TEST_DICT root')
            root_object = roots[0]
            by_handle = {o['handle'][-1]: o for o in objects if o.get('handle')}
            require(len(by_handle) == sum(bool(o.get('handle')) for o in objects), 'duplicate source handles')
            child = by_handle[root_object['items']['TEST_DICT'][-1]]
            require(child.get('object') == 'DICTIONARY' and child['ownerhandle'][-1] == root_object['handle'][-1]
                    and list(child['items']) == ['PAYLOAD'], 'source dictionary graph mismatch')
            record = by_handle[child['items']['PAYLOAD'][-1]]
            require(record.get('object') == 'XRECORD' and record['ownerhandle'][-1] == child['handle'][-1]
                    and record.get('xdata') == [list(tag) for tag in expected_tags], 'source XRECORD mismatch')
            proof = dictionary_frame(args.dwgread, source, version, child, None, None)
            frame, _, _ = traced_frame(args.dwgread, source, child['handle'][-1], 42)
            frames.append(frame)
            if version == 'r13':
                apps = [o for o in objects if o.get('object') == 'APPID' and o.get('name') == 'ACAD']
                require(len(apps) == 1, 'ACAD APPID is ambiguous')
                raw = b'\x00\x0d\x00-TREAT_AS_HARD\x46' + hard.to_bytes(2, 'little')
                require(proof['r14Byte'] is None and proof['eed'] == [{'appid': apps[0]['handle'][-1], 'hex': raw.hex()}],
                        'R13 EED ownership marker differs')
            else:
                require(proof['r14Byte'] == hard and not proof['eed'] and child.get('is_hardowner') == hard,
                        'R14 hard-ownership control differs')
            doc = ezdxf.readfile(output)
            output_child = doc.rootdict.get('TEST_DICT')
            require(output_child is not None and output_child.dxf.hard_owned == hard, 'reopened ownership mismatch')
            output_record = output_child.get('PAYLOAD')
            require(output_record is not None and output_record.dxf.owner == output_child.dxf.handle
                    and [(t.code, t.value) for t in output_record.tags] == expected_tags, 'reopened XRECORD mismatch')
            rows = audit_bytes(output.read_bytes(), include_rows=True)['dictionary_rows']
            matched = [r for r in rows if r['handle'] == output_child.dxf.handle.upper()]
            require(len(matched) == 1 and matched[0]['owner'] == doc.rootdict.dxf.handle.upper(), 'output owner mismatch')
            entries = matched[0]['entries']
            require(len(entries) == 1 and entries[0]['name'] == 'PAYLOAD'
                    and entries[0]['handle'] == output_record.dxf.handle.upper()
                    and entries[0]['code'] == 350 + 10 * hard, 'output dictionary link mismatch')
            item = {'seedSha256': sha(seeds / f'owner-{hard}.dxf'), 'sourceSha256': sha(source),
                    'sourceDictionary': proof, 'sourceXrecord': {'handle': record['handle'][-1],
                    'owner': record['ownerhandle'][-1], 'peerValues': record['xdata']},
                    'reopenSha256': sha(output), 'outputOwnerFlag': output_child.dxf.hard_owned,
                    'outputEntryCode': entries[0]['code']}
            if args.candidate:
                destination = root / f'{version}-{hard}-candidate.dxf'
                sentinel = b'preserve existing destination\n'; destination.write_bytes(sentinel)
                result = run(args.candidate, source, '-o', destination, '-y', allow_failure=True)
                item['candidate'] = {'exit': result.returncode,
                    'destinationPreservedOnFailure': result.returncode != 0 and destination.read_bytes() == sentinel,
                    'diagnostics': [x for x in result.stderr.decode(errors='replace').splitlines() if 'failed:' in x][-4:]}
                require(result.returncode == 0 or item['candidate']['destinationPreservedOnFailure'], 'candidate replaced destination on failure')
            samples[str(hard)] = item
        require(len(frames[0]) == len(frames[1]), 'frame length changed across paired controls')
        differences = [i for i, (a, b) in enumerate(zip(*frames)) if a != b]
        require(len(differences) == 1, 'paired source dictionary frames have unexpected differences')
        receipt['versions'][version] = {'samples': samples, 'differingFrameByteOffsets': differences}
    path = root / 'receipt.json'; path.write_text(json.dumps(receipt, indent=2) + '\n')
    print(path)


if __name__ == '__main__':
    main()
