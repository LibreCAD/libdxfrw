#!/usr/bin/env python3
"""Create ODA R13/R14 XRECORD length controls and inspect their wire bytes.

Requires ezdxf, LibreDWG dwgread, ODA File Converter and the optional
libdxfrw_dwg_ac1014_output_external_tests executable. All samples and mutated
negative controls remain in a unique scratch directory.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

from generate_ac1012_dictionary_witness import Bits, crc16, executable_identity
from verify_oda_legacy_layer_graph import class_frame, traced_frame


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(*args):
    result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=False)
    if result.returncode:
        raise RuntimeError(f'{args[0]} exited {result.returncode}: {result.stderr[-1200:]!r}')
    return result


def inspect_xrecord(dwgread, source, model, expected):
    own = model['handle'][-1]
    frame, offset, following = traced_frame(dwgread, source, own, model['type'])
    bits = Bits(frame)
    if bits.bs() != model['type'] or bits.handle(0, own=True)['value'] != own or bits.bs() != 0:
        raise ValueError('XRECORD type, handle or EED mismatch')
    body_end = bits.little(4)
    reactors, size = bits.bl(), bits.bl()
    data_start = bits.position
    if bits.little(2) != 1:
        raise ValueError('missing string group')
    length_bit = bits.position
    length = bits.little(2)
    page = bits.take(8)
    text = bytes(bits.take(8) for _ in range(length))
    if length != expected or page != 45 or text != b'A' * expected \
            or bits.little(2) != 90 or bits.little(4) != expected \
            or bits.position != body_end or bits.position - data_start != size * 8:
        raise ValueError('XRECORD string count, bytes, marker or boundary mismatch')
    owner = bits.handle(4, own)
    reactor_handles = [bits.handle(4, own) for _ in range(reactors)]
    extension = bits.handle(3, own)
    if owner['value'] != model['ownerhandle'][-1] or extension['value'] != model.get('xdicobjhandle', [0])[-1] \
            or [h['value'] for h in reactor_handles] != [h[-1] for h in model.get('reactors', [])] \
            or len(frame) * 8 - bits.position >= 8:
        raise ValueError('XRECORD handle tail mismatch')
    return {'handle': own, 'type': model['type'], 'offset': offset, 'size': len(frame),
            'frameSha256': hashlib.sha256(frame).hexdigest(), 'crcValid': True,
            'bodyEndBit': body_end, 'dataStartBit': data_start, 'lengthBit': length_bit,
            'length': length, 'lengthBytes': length.to_bytes(2, 'little').hex(),
            'codepage': page, 'followingGroup90': expected, 'nextFrame': following,
            'owner': owner, 'reactors': reactor_handles, 'extension': extension}


def make_bad_length(source, destination, proof):
    data = bytearray(source.read_bytes())
    for bit in range(16):
        absolute = proof['offset'] * 8 + proof['lengthBit'] + bit
        data[absolute // 8] |= 0x80 >> (absolute % 8)
    start, size = proof['offset'], proof['size']
    data[start+size:start+size+2] = crc16(data[start-2:start+size]).to_bytes(2, 'little')
    destination.write_bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--oda', required=True)
    parser.add_argument('--dwgread', required=True)
    parser.add_argument('--candidate-test', required=True)
    parser.add_argument('--output-dir', type=Path, required=True)
    args = parser.parse_args()
    import ezdxf
    args.output_dir.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix='oda-legacy-xrecord-length-', dir=args.output_dir))
    incoming = root / 'input'; incoming.mkdir()
    document = ezdxf.new('R2000')
    document.modelspace().add_line((1, 2, 3), (4, 6, 9))
    dictionary = document.rootdict.add_new_dict('LEN_PROBE')
    lengths = (1, 254, 255, 256, 300)
    for length in lengths:
        record = dictionary.add_xrecord('LEN_' + str(length))
        record.extend([(1, 'A' * length), (90, length)])
    seed = incoming / 'seed.dxf'; document.saveas(seed)
    report = {'oda': executable_identity(args.oda), 'dwgread': executable_identity(args.dwgread),
              'candidateTest': executable_identity(args.candidate_test), 'ezdxfVersion': ezdxf.__version__,
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
        model = json.loads(peer.read_text()); objects = model['OBJECTS']
        parents = [x for x in objects if x.get('object') == 'DICTIONARY'
                   and set(x.get('items', {})) == {'LEN_' + str(n) for n in lengths}]
        if len(parents) != 1:
            raise ValueError('length-control dictionary missing or ambiguous')
        proofs = {}
        class_proof = None
        for length in lengths:
            handle = parents[0]['items']['LEN_' + str(length)][-1]
            rows = [x for x in objects if x.get('object') == 'XRECORD' and x.get('handle', [None])[-1] == handle]
            if len(rows) != 1 or rows[0]['ownerhandle'][-1] != parents[0]['handle'][-1]:
                raise ValueError('XRECORD source identity or owner mismatch')
            if class_proof is None:
                class_proof = class_frame(source, model['CLASSES'], rows[0]['type'])
                if class_proof['selected']['dxf'] != 'XRECORD' or class_proof['selected']['cpp'] != 'AcDbXrecord':
                    raise ValueError('XRECORD source class mismatch')
            elif class_proof['selected']['number'] != rows[0]['type']:
                raise ValueError('different XRECORD source class')
            proofs[str(length)] = inspect_xrecord(args.dwgread, source, rows[0], length)
        bad = root / (version + '-bad-length.dwg')
        make_bad_length(source, bad, proofs['300'])
        run(args.candidate_test, '--legacy-xrecord-lengths', str(source), str(bad))
        run(args.oda, str(source_dir), str(reopened), 'ACAD2013', 'DXF', '0', '1', '*.dwg')
        output = reopened / 'seed.dxf'
        if not output.is_file() or (reopened / 'seed.dxf.err').exists():
            raise ValueError('ODA reopen failed')
        readback = ezdxf.readfile(output).rootdict.get('LEN_PROBE')
        for length in lengths:
            values = [(item.code, item.value) for item in readback.get('LEN_' + str(length)).tags]
            if values != [(1, 'A' * length), (90, length)]:
                raise ValueError('ODA XRECORD string or following marker changed')
        report['versions'][version] = {'sourceSha256': sha(source), 'badLengthSha256': sha(bad),
            'odaReadbackSha256': sha(output), 'classFrame': class_proof, 'frames': proofs,
            'candidateMalformedRefusedAndSameAdapterRetryPassed': True,
            'odaReadbackMatched': True}
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
