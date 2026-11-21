#!/usr/bin/env python3
"""Produce and inspect ODA R13/R14 dictionary layer/extension controls.

Requires ezdxf, LibreDWG dwgread, and ODA File Converter with desktop access.
All drawings and converter output remain in a unique scratch directory. This
establishes the selected producer profile, not AutoCAD-native semantics.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

from generate_ac1012_dictionary_witness import Bits, crc16, executable_identity


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(*args):
    result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    if result.returncode:
        raise RuntimeError(f'{args[0]} exited {result.returncode}: {result.stderr[-1200:]!r}')
    return result


def dictionary_frame(dwgread, source, version, model, appid, layer):
    own = model['handle'][-1]
    trace = run(dwgread, '-v4', str(source))
    blocks = re.split(r'(?=Object number:)', (trace.stdout + trace.stderr).decode(errors='replace'))
    candidates = [block for block in blocks if re.search(r'\bhandle: 0\.\d+\.' + format(own, 'X') + r'\s', block)]
    if len(candidates) != 1:
        raise ValueError('ambiguous dictionary trace')
    location = re.search(r'Size: (\d+) \[MS\], Type: 42 \[BS\], Address: (\d+)', candidates[0])
    if not location:
        raise ValueError('dictionary frame location missing')
    size, offset = map(int, location.groups())
    data = source.read_bytes()
    if offset < 2 or offset + size + 2 > len(data) or data[offset-2:offset] != size.to_bytes(2, 'little'):
        raise ValueError('dictionary envelope invalid')
    if int.from_bytes(data[offset+size:offset+size+2], 'little') != crc16(data[offset-2:offset+size]):
        raise ValueError('dictionary CRC mismatch')
    frame = data[offset:offset+size]
    bits = Bits(frame)
    if bits.bs() != 42 or bits.handle(0, own=True)['value'] != own:
        raise ValueError('dictionary source identity mismatch')
    chunks = []
    while True:
        length = bits.bs()
        if not length:
            break
        app = bits.handle(5)['value']
        payload = bytes(bits.take(8) for _ in range(length))
        chunks.append({'appid': app, 'hex': payload.hex()})
    matching = [bytes.fromhex(chunk['hex']) for chunk in chunks if chunk['appid'] == appid]
    if layer is not None and (len(matching) != 1
            or not matching[0].startswith(b'\x03' + layer.to_bytes(8, 'big'))):
        raise ValueError('dictionary layer EED source mismatch')
    body_end = bits.little(4)
    reactors, count = bits.bl(), bits.bl()
    if count != model['numitems'] or reactors > 1024:
        raise ValueError('dictionary counts mismatch')
    r14_byte = bits.take(8) if version == 'r14' else None
    names = []
    for _ in range(count):
        n = bits.bs()
        name = bytes(bits.take(8) for _ in range(n)).rstrip(b'\0').decode('ascii')
        names.append(name)
    if len(set(names)) != count or bits.position != body_end:
        raise ValueError('duplicate names or dictionary body/handle boundary mismatch')
    owner = bits.handle(4, own)['value']
    reactor_handles = [bits.handle(4, own)['value'] for _ in range(reactors)]
    xdict = bits.handle(3, own)
    entries = [bits.handle(2, own)['value'] for _ in names]
    if owner != model['ownerhandle'][-1] or xdict['value'] != model.get('xdicobjhandle', [0])[-1] \
            or dict(zip(names, entries)) != {k: v[-1] for k, v in model['items'].items()} \
            or len(frame) * 8 - bits.position >= 8:
        raise ValueError('dictionary source role or entry mismatch')
    return {'handle': own, 'frameSha256': hashlib.sha256(frame).hexdigest(),
            'frameOffset': offset, 'frameSize': size, 'bodyEndBit': body_end,
            'crcValid': True, 'r14Byte': r14_byte, 'owner': owner, 'reactors': reactor_handles,
            'extension': xdict, 'eed': chunks, 'entryCount': count}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--oda', required=True)
    parser.add_argument('--dwgread', default='dwgread')
    parser.add_argument('--candidate', required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    import ezdxf
    args.output_dir.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix='oda-legacy-layer-graph-', dir=args.output_dir))
    incoming = root / 'input'; incoming.mkdir()
    document = ezdxf.new('R2000')
    document.layers.new('PROBE_LAYER')
    document.modelspace().add_line((1, 2, 3), (4, 6, 9))
    document.rootdict.set_xdata('ACAD', [(1003, 'PROBE_LAYER')])
    document.rootdict.get('ACAD_MLINESTYLE').set_xdata('ACAD', [(1003, 'PROBE_LAYER')])
    seed = incoming / 'seed.dxf'; document.saveas(seed)
    report = {'oda': executable_identity(args.oda), 'dwgread': executable_identity(args.dwgread),
              'candidate': executable_identity(args.candidate), 'ezdxfVersion': ezdxf.__version__,
              'seedSha256': sha(seed), 'versions': {}}
    for version, target, magic in (('r13', 'ACAD13', b'AC1012'), ('r14', 'ACAD14', b'AC1014')):
        source_dir, reopened = root / version, root / (version + '-reopen')
        source_dir.mkdir(); reopened.mkdir()
        run(args.oda, str(incoming), str(source_dir), target, 'DWG', '0', '1', '*.dxf')
        source = source_dir / 'seed.dwg'
        if not source.is_file() or source.read_bytes()[:6] != magic:
            raise ValueError('ODA source missing or wrong version')
        peer = source_dir / 'peer.json'
        run(args.dwgread, '-O', 'JSON', '-o', str(peer), str(source))
        objects = json.loads(peer.read_text())['OBJECTS']
        by_handle = {}
        for item in objects:
            if item.get('handle'):
                handle = item['handle'][-1]
                if handle in by_handle:
                    raise ValueError('duplicate peer handle')
                by_handle[handle] = item
        layers = [x for x in objects if x.get('object') == 'LAYER' and x.get('name') == 'PROBE_LAYER']
        apps = [x for x in objects if x.get('object') == 'APPID' and x.get('name') == 'ACAD']
        roots = [x for x in objects if x.get('object') == 'DICTIONARY'
                 and x.get('ownerhandle', [None])[-1] == 0 and 'ACAD_MLINESTYLE' in x.get('items', {})]
        if len(layers) != 1 or len(apps) != 1 or len(roots) != 1:
            raise ValueError('source layer, APPID or NOD identity is ambiguous')
        layer, appid, nod = layers[0]['handle'][-1], apps[0]['handle'][-1], roots[0]
        mline = by_handle.get(nod['items']['ACAD_MLINESTYLE'][-1])
        if not mline or mline.get('object') != 'DICTIONARY' or mline['ownerhandle'][-1] != nod['handle'][-1]:
            raise ValueError('MLINESTYLE dictionary graph mismatch')
        proofs = {}
        for kind, item in (('nod', nod), ('mline', mline)):
            proof = dictionary_frame(args.dwgread, source, version, item, appid, layer)
            xdict = by_handle.get(proof['extension']['value'])
            if (not xdict or xdict.get('object') != 'DICTIONARY'
                    or xdict['ownerhandle'][-1] != proof['handle']
                    or xdict.get('numitems') != 1 or len(xdict.get('items', {})) != 1):
                raise ValueError('extension dictionary owner or membership mismatch')
            entry = xdict.get('items', {}).get('ACAD_XREC_ROUNDTRIP')
            child = by_handle.get(entry[-1]) if entry else None
            if not child or child.get('object') != 'XRECORD' or child['ownerhandle'][-1] != xdict['handle'][-1]:
                raise ValueError('extension XRECORD graph mismatch')
            extension_frame = dictionary_frame(args.dwgread, source, version, xdict, appid, None)
            expected_values = ([[102, 'EXTNAMES'], [1, 'ACDBVARIABLEDICTIONARY'],
                                [2, 'AcDbVariableDictionary']] if kind == 'nod' else
                               [[102, 'EXTNAMES'], [1, 'STANDARD'], [2, 'Standard']])
            if child.get('xdata') != expected_values or child.get('reactors') != [[4, 1, xdict['handle'][-1], xdict['handle'][-1]]]:
                raise ValueError('extension XRECORD peer content or reactor mismatch')
            proof.update({'extensionFrame': extension_frame, 'extensionEntry': entry[-1],
                          'extensionChildType': 'XRECORD', 'extensionChildXdata': expected_values})
            proofs[kind] = proof
        run(args.oda, str(source_dir), str(reopened), 'ACAD2013', 'DXF', '0', '1', '*.dwg')
        dxf = reopened / 'seed.dxf'
        if not dxf.is_file() or (reopened / 'seed.dxf.err').exists():
            raise ValueError('ODA peer reopen failed')
        doc = ezdxf.readfile(dxf)
        for dictionary in (doc.rootdict, doc.rootdict.get('ACAD_MLINESTYLE')):
            if [(t.code, t.value) for t in dictionary.get_xdata('ACAD')] != [(1003, 'PROBE_LAYER')]:
                raise ValueError('ODA peer lost the layer reference')
        candidate_results = {}
        for mode in ('strict', 'partial'):
            destination = root / f'{version}-{mode}-candidate.dxf'
            sentinel = b'destination preserved\n'; destination.write_bytes(sentinel)
            command = [args.candidate, str(source), '-o', str(destination), '-y', '-d',
                       '--dwg-eed-layer-byte-order=big']
            if mode == 'partial':
                command.append('--allow-partial-dwg-read')
            candidate = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
            if candidate.returncode == 0 or destination.read_bytes() != sentinel:
                raise ValueError('candidate unexpectedly published the unqualified whole-source output')
            errors = [line.strip() for line in candidate.stderr.decode(errors='replace').splitlines()
                      if 'failed:' in line or 'unqualified' in line.lower()]
            candidate_results[mode] = {'exit': candidate.returncode, 'destinationPreserved': True,
                                       'diagnostics': errors[-5:]}
        report['versions'][version] = {'dwgSha256': sha(source), 'layerHandle': layer, 'appidHandle': appid,
            'proofs': proofs, 'odaReadbackSha256': sha(dxf), 'odaReadbackLayer': 'PROBE_LAYER',
            'candidate': candidate_results}
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
