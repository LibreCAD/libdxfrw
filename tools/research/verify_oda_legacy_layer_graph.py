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


def traced_frame(dwgread, source, own, expected_type):
    trace = run(dwgread, '-v4', str(source))
    trace_text = (trace.stdout + trace.stderr).decode(errors='replace')
    blocks = re.split(r'(?=Object number:)', trace_text)
    candidates = [block for block in blocks if re.search(r'\bhandle: 0\.\d+\.' + format(own, 'X') + r'\s', block)]
    if len(candidates) != 1:
        raise ValueError('ambiguous object trace')
    location = re.search(r'Size: (\d+) \[MS\], Type: (\d+) \[BS\], Address: (\d+)', candidates[0])
    if not location:
        raise ValueError('object frame location missing')
    size, object_type, offset = map(int, location.groups())
    data = source.read_bytes()
    if object_type != expected_type or offset < 2 or offset + size + 2 > len(data) \
            or data[offset-2:offset] != size.to_bytes(2, 'little'):
        raise ValueError('object envelope or type invalid')
    if int.from_bytes(data[offset+size:offset+size+2], 'little') != crc16(data[offset-2:offset+size]):
        raise ValueError('object CRC mismatch')
    locations = sorted((int(addr), int(length), int(kind)) for length, kind, addr in
        re.findall(r'Size: (\d+) \[MS\], Type: (\d+) \[BS\], Address: (\d+)', trace_text))
    following = next((entry for entry in locations if entry[0] > offset), None)
    if following is None or following[0] - 2 != offset + size + 2:
        raise ValueError('following physical frame is not adjacent')
    next_offset, next_size, next_type = following
    if next_offset + next_size + 2 > len(data) or data[next_offset-2:next_offset] != next_size.to_bytes(2, 'little') \
            or int.from_bytes(data[next_offset+next_size:next_offset+next_size+2], 'little') != crc16(data[next_offset-2:next_offset+next_size]):
        raise ValueError('following physical frame envelope or CRC mismatch')
    return data[offset:offset+size], offset, {'offset': next_offset, 'size': next_size, 'type': next_type, 'crcValid': True}


def dictionary_frame(dwgread, source, version, model, appid, layer):
    own = model['handle'][-1]
    frame, offset, following = traced_frame(dwgread, source, own, 42)
    size = len(frame)
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
            'crcValid': True, 'nextFrame': following, 'r14Byte': r14_byte, 'owner': owner, 'reactors': reactor_handles,
            'extension': xdict, 'eed': chunks, 'entryCount': count}


def class_frame(source, classes, selected_type):
    """Walk the complete R13/R14 class section and bind one class record."""
    data = source.read_bytes()
    if len(data) < 25:
        raise ValueError('truncated file header')
    count = int.from_bytes(data[21:25], 'little')
    if count > 64 or 25 + count * 9 > len(data):
        raise ValueError('invalid section locator count')
    locators = []
    for index in range(count):
        at = 25 + index * 9
        if data[at] == 1:
            locators.append((int.from_bytes(data[at+1:at+5], 'little'),
                             int.from_bytes(data[at+5:at+9], 'little')))
    if len(locators) != 1:
        raise ValueError('class section locator missing or ambiguous')
    start, size = locators[0]
    if size < 38 or start + size > len(data):
        raise ValueError('class section outside file')
    section = data[start:start+size]
    if section[:16] != bytes.fromhex('8da1c4b8c4a9f8c5c0dcf45fe7cfb68a') \
            or section[-16:] != bytes.fromhex('725e3b473b56073a3f230ba018304975'):
        raise ValueError('class section sentinel mismatch')
    count_bytes = int.from_bytes(section[16:20], 'little')
    if count_bytes != size - 38 or int.from_bytes(section[-18:-16], 'little') != crc16(section[16:-18]):
        raise ValueError('class section size or CRC mismatch')
    if len(classes) > 4096:
        raise ValueError('too many peer class records')
    bits = Bits(section[20:-18]); selected = []; seen = set()
    def text_value():
        n = bits.bs()
        if n > 4096:
            raise ValueError('class string too long')
        value_start = bits.position
        return bytes(bits.take(8) for _ in range(n)).rstrip(b'\0').decode('ascii'), value_start
    for peer in classes:
        begin = bits.position
        number, flags = bits.bs(), bits.bs()
        app, _ = text_value()
        cpp, _ = text_value()
        dxf, dxf_start = text_value()
        zombie, item_class = bits.take(1), bits.bs()
        if number in seen or (number, flags, app, cpp, dxf, zombie, item_class) != (
                peer['number'], peer['proxyflag'], peer['appname'], peer['cppname'],
                peer['dxfname'], peer['is_zombie'], peer['item_class_id']):
            raise ValueError('class physical and peer records differ')
        seen.add(number)
        if number == selected_type:
            selected.append({'number': number, 'flags': flags, 'app': app, 'cpp': cpp,
                             'dxf': dxf, 'zombie': zombie, 'itemClassId': item_class,
                             'physicalStartBit': (start + 20) * 8 + begin,
                             'physicalEndBit': (start + 20) * 8 + bits.position,
                             'physicalDxfValueStartBit': (start + 20) * 8 + dxf_start})
    if len(selected) != 1 or len(section[20:-18]) * 8 - bits.position >= 8:
        raise ValueError('class record missing or trailing class data')
    return {'sectionOffset': start, 'sectionSize': size,
            'sectionSha256': hashlib.sha256(section).hexdigest(), 'crcValid': True,
            'classCount': len(classes), 'selected': selected[0]}


def class_mutation_control(source, root, classes, class_proof):
    data = bytearray(source.read_bytes())
    bit = class_proof['selected']['physicalDxfValueStartBit'] + 7
    data[bit // 8] ^= 0x80 >> (bit % 8)
    start, size = class_proof['sectionOffset'], class_proof['sectionSize']
    data[start+size-18:start+size-16] = crc16(data[start+16:start+size-18]).to_bytes(2, 'little')
    patched = root / ('mutated-class-' + source.parent.name + '.dwg')
    patched.write_bytes(data)
    try:
        class_frame(patched, classes, class_proof['selected']['number'])
    except (ValueError, IndexError):
        pass
    else:
        raise ValueError('mutated class name was accepted')
    retry = class_frame(source, classes, class_proof['selected']['number'])
    if retry['sectionSha256'] != class_proof['sectionSha256']:
        raise ValueError('unchanged class-section retry mismatch')
    return {'changedDxfNameRejectedWithRepairedCrc': True, 'unchangedRetryPassed': True}


def xrecord_frame(dwgread, source, model, expected_values):
    own = model['handle'][-1]
    object_type = model['type']
    frame, offset, following = traced_frame(dwgread, source, own, object_type)
    bits = Bits(frame)
    if bits.bs() != object_type or bits.handle(0, own=True)['value'] != own or bits.bs() != 0:
        raise ValueError('XRECORD type, identity or EED mismatch')
    body_end_field = bits.position
    body_end = bits.little(4)
    reactors, size = bits.bl(), bits.bl()
    start = bits.position
    payload = bytes(bits.take(8) for _ in range(size))
    if bits.position != body_end or reactors != len(model.get('reactors', [])):
        raise ValueError('XRECORD body or data boundary mismatch')
    index, decoded = 0, []
    while index < len(payload):
        if index + 5 > len(payload):
            raise ValueError('truncated XRECORD string')
        code = int.from_bytes(payload[index:index+2], 'little')
        length = payload[index+2]
        codepage = int.from_bytes(payload[index+3:index+5], 'big')
        index += 5
        if index + length > len(payload) or codepage != 45:
            raise ValueError('XRECORD string length or codepage mismatch')
        value = payload[index:index+length].decode('ascii')
        decoded.append([code, value]); index += length
    if decoded != expected_values:
        raise ValueError('XRECORD physical values mismatch')
    owner_bit = bits.position
    owner = bits.handle(4, own)
    reactor_handles = [bits.handle(4, own) for _ in range(reactors)]
    xdict = bits.handle(3, own)
    if owner['value'] != model['ownerhandle'][-1] or xdict['value'] != 0 \
            or [item['value'] for item in reactor_handles] != [item[-1] for item in model['reactors']] \
            or len(frame) * 8 - bits.position >= 8:
        raise ValueError('XRECORD handle tail or trailing boundary mismatch')
    return {'handle': own, 'type': object_type, 'frameSha256': hashlib.sha256(frame).hexdigest(),
            'frameOffset': offset, 'frameSize': len(frame), 'crcValid': True, 'nextFrame': following,
            'bodyEndBit': body_end, 'bodyEndFieldBit': body_end_field, 'dataStartBit': start,
            'ownerBit': owner_bit, 'dataBytes': size,
            'dataHex': payload.hex(), 'values': decoded, 'owner': owner,
            'reactors': reactor_handles, 'extension': xdict}


def mutation_control(dwgread, source, root, model, values, proof):
    """Repair the CRC after each source-bit mutation; require a failed proof."""
    failures = {}
    for name, bit in (('value', proof['dataStartBit'] + 47),
                      ('ownerRole', proof['ownerBit']),
                      ('bodyBoundary', proof['bodyEndFieldBit'])):
        data = bytearray(source.read_bytes())
        absolute = proof['frameOffset'] * 8 + bit
        if absolute // 8 >= len(data):
            raise ValueError('mutation bit outside file')
        data[absolute // 8] ^= 0x80 >> (absolute % 8)
        offset, size = proof['frameOffset'], proof['frameSize']
        data[offset+size:offset+size+2] = crc16(data[offset-2:offset+size]).to_bytes(2, 'little')
        patched = root / f"mutation-{proof['handle']}-{name}.dwg"
        patched.write_bytes(data)
        try:
            xrecord_frame(dwgread, patched, model, values)
        except (RuntimeError, ValueError, IndexError):
            failures[name] = True
        else:
            raise ValueError(f'XRECORD {name} mutation was accepted')
    retry = xrecord_frame(dwgread, source, model, values)
    if retry['frameSha256'] != proof['frameSha256']:
        raise ValueError('unchanged XRECORD retry mismatch')
    return {'rejected': failures, 'unchangedRetryPassed': True}


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
        peer_model = json.loads(peer.read_text())
        objects = peer_model['OBJECTS']
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
        class_proof = None
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
            class_rows = [row for row in peer_model['CLASSES'] if row.get('number') == child['type']]
            if len(class_rows) != 1 or class_rows[0].get('dxfname') != 'XRECORD' \
                    or class_rows[0].get('cppname') != 'AcDbXrecord' \
                    or class_rows[0].get('appname') != 'ObjectDBX Classes':
                raise ValueError('XRECORD class-table peer identity mismatch')
            if class_proof is None:
                class_proof = class_frame(source, peer_model['CLASSES'], child['type'])
            elif class_proof['selected']['number'] != child['type']:
                raise ValueError('extension XRECORD class mismatch')
            child_frame = xrecord_frame(args.dwgread, source, child, expected_values)
            controls = mutation_control(args.dwgread, source, root, child, expected_values, child_frame)
            proof.update({'extensionFrame': extension_frame, 'extensionEntry': entry[-1],
                          'extensionChildType': 'XRECORD', 'extensionChildXdata': expected_values,
                          'extensionChildClass': class_rows[0], 'extensionChildFrame': child_frame,
                          'extensionChildMutationControls': controls})
            proofs[kind] = proof
        class_controls = class_mutation_control(source, root, peer_model['CLASSES'], class_proof)
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
            'proofs': proofs, 'classFrame': class_proof, 'classMutationControls': class_controls,
            'odaReadbackSha256': sha(dxf), 'odaReadbackLayer': 'PROBE_LAYER',
            'candidate': candidate_results}
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
