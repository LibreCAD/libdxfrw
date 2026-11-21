#!/usr/bin/env python3
"""Build scratch R13/R14 baseline and EED variants for local qualification work.

Requires LibreDWG 0.14's dwgadd, dwgread and dwgwrite. No drawings are
committed; the inputs originate in the tracked, locally authored recipe.
"""
import argparse
import hashlib
import json
import re
import subprocess
from pathlib import Path

from generate_ac1012_dictionary_witness import Bits, crc16


def run(*args):
    completed = subprocess.run(args, capture_output=True, text=True, check=False)
    if completed.returncode:
        raise RuntimeError(f"{args[0]} failed: {completed.stderr[-2000:]}")
    return completed


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def replace_eed_text(dwgread, source, destination, value, appid, group):
    """Patch the locally generated fixed-size EED string and repair its CRC."""
    raw = value.encode('utf-8')
    if len(raw) != 5:
        raise ValueError('control text must occupy exactly five UTF-8 bytes')
    trace = run(dwgread, '-v4', str(source))
    blocks = re.split(r'(?=Object number:)', trace.stdout + trace.stderr)
    matches = [b for b in blocks if re.search(r'\bhandle: 0\.\d+\.' + format(group, 'X') + r'\s', b)]
    if len(matches) != 1:
        raise RuntimeError('group dictionary trace is missing or ambiguous')
    location = re.search(r'Size: (\d+) \[MS\], Type: 42 \[BS\], Address: (\d+)', matches[0])
    if location is None:
        raise RuntimeError('group dictionary frame location missing')
    size, address = map(int, location.groups())
    data = bytearray(source.read_bytes())
    if address < 2 or address + size + 2 > len(data) or data[address-2:address] != size.to_bytes(2, 'little'):
        raise RuntimeError('unexpected source frame envelope')
    if int.from_bytes(data[address+size:address+size+2], 'little') != crc16(data[address-2:address+size]):
        raise RuntimeError('source frame CRC mismatch')
    bits = Bits(data[address:address+size])
    if (bits.bs() != 42 or bits.handle(0, own=True)['value'] != group or bits.bs() != 12
            or bits.handle(5)['value'] != appid or bits.take(8) != 0 or bits.take(8) != 5):
        raise RuntimeError('unexpected source EED framing')
    offset = bits.position
    if bytes(bits.take(8) for _ in range(7)) != b'\x00\x1eHELLO':
        raise RuntimeError('unexpected source codepage or text')
    for index, byte in enumerate(b'\x00\x2d' + raw):
        for bit in range(8):
            pos = address * 8 + offset + index * 8 + bit
            mask = 0x80 >> (pos % 8)
            data[pos // 8] = ((data[pos // 8] | mask) if byte & (0x80 >> bit)
                               else (data[pos // 8] & ~mask))
    data[address+size:address+size+2] = crc16(data[address-2:address+size]).to_bytes(2, 'little')
    destination.write_bytes(data)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--version', choices=('r13', 'r14'), default='r14')
    parser.add_argument('--alternate-app', action='store_true')
    parser.add_argument('--alternate-group', action='store_true')
    parser.add_argument('--nonempty-group', action='store_true')
    target_group = parser.add_mutually_exclusive_group()
    target_group.add_argument('--eed-on-nod', action='store_true')
    target_group.add_argument('--eed-on-mline', action='store_true')
    parser.add_argument('--dwgadd', default='dwgadd')
    parser.add_argument('--dwgread', default='dwgread')
    parser.add_argument('--dwgwrite', default='dwgwrite')
    args = parser.parse_args()
    for tool in (args.dwgadd, args.dwgread, args.dwgwrite):
        if '0.14' not in run(tool, '--version').stdout:
            raise RuntimeError(f'{tool} must be LibreDWG 0.14')
    root = args.output_dir.resolve()
    root.mkdir(parents=True, exist_ok=True)
    recipe = Path(__file__).resolve().parents[2] / 'tests/fixtures/dwg/ac1012_partial_three_line_control.dwgadd'
    base = root / 'dwgadd.dwg'
    source = root / 'dwgadd.json'
    if any((root / name).exists() for name in ('dwgadd.dwg', 'dwgadd.json', 'baseline.dwg', 'eed.dwg', 'unqualified.dwg', 'utf8.dwg', 'wide.dwg')):
        raise RuntimeError('output directory must not contain a previous control')
    effective_recipe = recipe
    if args.nonempty_group:
        effective_recipe = root / 'nonempty-group.dwgadd'
        effective_recipe.write_text(recipe.read_text() + '\ngroup "MYGROUP"\n')
    run(args.dwgadd, '--as', args.version, '-o', str(base), str(effective_recipe))
    run(args.dwgread, '-O', 'JSON', '-o', str(source), str(base))
    document = json.loads(source.read_text())
    styles = [o for o in document['OBJECTS'] if o.get('object') == 'MLINESTYLE']
    dictionaries = [o for o in document['OBJECTS'] if o.get('object') == 'DICTIONARY'
                    and o.get('handle', [0, 0, 0])[2] == 13]
    apps = [o for o in document['OBJECTS'] if o.get('object') == 'APPID'
            and o.get('handle', [0, 0, 0])[2] == 18 and o.get('name') == 'ACAD']
    if len(styles) != 1 or len(dictionaries) != 1 or len(apps) != 1:
        raise RuntimeError('unexpected dwgadd graph or APPID')
    appid = 18
    if args.alternate_app:
        controls = [o for o in document['OBJECTS'] if o.get('object') == 'APPID_CONTROL']
        used = {o.get('handle', [0, 0, 0])[2] for o in document['OBJECTS'] if 'handle' in o}
        if len(controls) != 1 or controls[0].get('entries') != [[2, 1, 18, 18]] or 51 in used:
            raise RuntimeError('unexpected APPID control or occupied alternate handle')
        appid = 51
        controls[0]['entries'] = [[2, 1, appid, appid]]
        apps[0]['handle'] = [0, 1, appid]
        apps[0]['ownerhandle'] = [4, 1, 9, 9]
        document['HEADER']['HANDSEED'] = [0, 1, 52, 52]
    group = 13
    if args.alternate_group:
        used = {o.get('handle', [0, 0, 0])[2] for o in document['OBJECTS'] if 'handle' in o}
        nod = [o for o in document['OBJECTS'] if o.get('object') == 'DICTIONARY'
               and o.get('handle', [0, 0, 0])[2] == 12]
        siblings = [o for o in document['OBJECTS'] if o.get('object') == 'DICTIONARY'
                    and o.get('handle', [0, 0, 0])[2] in (23, 25)]
        if 52 in used or 53 in used or len(nod) != 1 or len(siblings) != 2:
            raise RuntimeError('unexpected dictionary graph or occupied alternate handles')
        group, root_handle = 52, 53
        document['HEADER']['HANDSEED'] = [0, 1, 54, 54]
        document['HEADER']['DICTIONARY_ACAD_GROUP'] = [5, 1, group, group]
        document['HEADER']['DICTIONARY_NAMED_OBJECT'] = [3, 1, root_handle, root_handle]
        nod[0]['handle'] = [0, 1, root_handle]
        nod[0]['items']['ACAD_GROUP'] = [2, 1, group, group]
        dictionaries[0]['handle'] = [0, 1, group]
        for child in [dictionaries[0], *siblings]:
            child['ownerhandle'] = [4, 1, root_handle, root_handle]
            child['reactors'] = [[4, 1, root_handle, root_handle]]
    if args.nonempty_group:
        groups = [o for o in document['OBJECTS'] if o.get('object') == 'GROUP']
        lines = [o for o in document['OBJECTS'] if o.get('entity') == 'LINE']
        if len(groups) != 1 or len(lines) != 3 or groups[0].get('handle', [0, 0, 0])[2] != 49:
            raise RuntimeError('unexpected generated GROUP or LINE graph')
        # LibreDWG dwgadd leaves this scratch GROUP malformed. Construct the
        # bounded graph with the ODA v5.4.1 §20.4.72 hard-pointer member role.
        dictionaries[0]['numitems'] = 1
        dictionaries[0]['items'] = {'MYGROUP': [2, 1, 49, 49]}
        groups[0]['ownerhandle'] = [4, 1, group, group]
        groups[0]['reactors'] = [[4, 1, group, group]]
        groups[0]['unnamed'] = 0
        groups[0]['selectable'] = 1
        groups[0]['groups'] = [[5, 1, lines[0]['handle'][2], lines[0]['handle'][2]]]
    root_handle = 53 if args.alternate_group else 12
    roots = [o for o in document['OBJECTS'] if o.get('object') == 'DICTIONARY'
             and o.get('handle', [0, 0, 0])[2] == root_handle]
    if len(roots) != 1:
        raise RuntimeError('named-object dictionary not unique')
    mline_dicts = [o for o in document['OBJECTS'] if o.get('object') == 'DICTIONARY'
                   and o.get('handle', [0, 0, 0])[2] == 23]
    if len(mline_dicts) != 1:
        raise RuntimeError('MLINESTYLE owner dictionary not unique')
    target = roots[0] if args.eed_on_nod else mline_dicts[0] if args.eed_on_mline else dictionaries[0]
    target_handle = root_handle if args.eed_on_nod else 23 if args.eed_on_mline else group
    # dwgread JSON rounds the angle; restore the exact generated binary value
    # so the baseline remains within the existing MLINESTYLE wire profile.
    styles[0]['start_angle'] = styles[0]['end_angle'] = 1.5707963267948966
    results = {}
    for name in ('baseline', 'eed', 'int32', 'reals', 'unqualified'):
        if name == 'eed':
            target['eed'] = [
                {'size': 12, 'handle': [5, 1, appid], 'code': 0, 'value': 'HELLO'},
                {'code': 70, 'value': 42},
            ]
        if name == 'int32':
            target['eed'][0]['size'] = 27
            target['eed'].extend({'code': 71, 'value': value}
                                 for value in (123456, 0x80000000, 0x7fffffff))
        if name == 'reals':
            target['eed'] = [
                {'size': 39, 'handle': [5, 1, appid], 'code': 0, 'value': 'HELLO'},
                {'code': 70, 'value': 42}, {'code': 40, 'value': 1.25},
                {'code': 41, 'value': -0.5}, {'code': 42, 'value': 42.125},
            ]
        if name == 'unqualified':
            # A balanced 1002 group remains outside this bounded source proof.
            target['eed'] = [
                {'size': 16, 'handle': [5, 1, appid], 'code': 0, 'value': 'HELLO'},
                {'code': 70, 'value': 42}, {'code': 2, 'value': 0}, {'code': 2, 'value': 1},
            ]
        path = root / (name + '.json')
        drawing = root / (name + '.dwg')
        path.write_text(json.dumps(document))
        run(args.dwgwrite, '--as', args.version, '-I', 'JSON', '-o', str(drawing), str(path))
        check = root / (name + '-readback.json')
        run(args.dwgread, '-O', 'JSON', '-o', str(check), str(drawing))
        reread = json.loads(check.read_text())
        selected = [o for o in reread['OBJECTS'] if o.get('object') == 'DICTIONARY'
                    and o.get('handle', [0, 0, 0])[2] == group]
        roots = [o for o in reread['OBJECTS'] if o.get('object') == 'DICTIONARY'
                 and o.get('handle', [0, 0, 0])[2] == root_handle]
        if len(selected) != 1 or len(roots) != 1:
            raise RuntimeError(f'{name} independent dictionary readback mismatch')
        mline_roots = [o for o in reread['OBJECTS'] if o.get('object') == 'DICTIONARY'
                       and o.get('handle', [0, 0, 0])[2] == 23]
        if len(mline_roots) != 1 or roots[0].get('items', {}).get('ACAD_MLINESTYLE', [0])[-1] != 23 \
                or mline_roots[0].get('ownerhandle', [0])[-1] != root_handle:
            raise RuntimeError(f'{name} MLINESTYLE owner graph mismatch')
        peer_target = roots[0] if args.eed_on_nod else mline_roots[0] if args.eed_on_mline else selected[0]
        if name != 'baseline' and peer_target.get('eed') != target['eed']:
            raise RuntimeError(f'{name} independent EED readback mismatch')
        peer_apps = [o for o in reread['OBJECTS'] if o.get('object') == 'APPID'
                     and o.get('handle', [0, 0, 0])[2] == appid and o.get('name') == 'ACAD']
        peer_controls = [o for o in reread['OBJECTS'] if o.get('object') == 'APPID_CONTROL']
        if (len(roots) != 1 or roots[0].get('items', {}).get('ACAD_GROUP', [0])[-1] != group
                or selected[0].get('ownerhandle', [0])[-1] != root_handle
                or len(peer_apps) != 1 or len(peer_controls) != 1
                or [entry[-1] for entry in peer_controls[0].get('entries', [])] != [appid]):
            raise RuntimeError(f'{name} independent source graph mismatch')
        if args.nonempty_group:
            group_objects = [o for o in reread['OBJECTS'] if o.get('object') == 'GROUP'
                             and o.get('handle', [0, 0, 0])[2] == 49]
            if (selected[0].get('items', {}).get('MYGROUP', [0])[-1] != 49
                    or len(group_objects) != 1 or group_objects[0].get('ownerhandle', [0])[-1] != group
                    or group_objects[0].get('groups') != [[5, 1, 46, 46]]):
                raise RuntimeError(f'{name} peer group member graph mismatch')
        if name == 'baseline' and peer_target.get('eed'):
            raise RuntimeError('baseline unexpectedly contains EED')
        results[name] = {'path': str(drawing), 'sha256': digest(drawing)}
    if args.nonempty_group:
        broken = json.loads((root / 'eed.json').read_text())
        member = [o for o in broken['OBJECTS'] if o.get('object') == 'GROUP'
                  and o.get('handle', [0, 0, 0])[2] == 49]
        if len(member) != 1 or member[0].get('groups') != [[5, 1, 46, 46]]:
            raise RuntimeError('unexpected GROUP before wrong-role control')
        member[0]['groups'] = [[2, 1, 46, 46]]
        (root / 'bad-member-role.json').write_text(json.dumps(broken))
        drawing = root / 'bad-member-role.dwg'
        run(args.dwgwrite, '--as', args.version, '-I', 'JSON', '-o', str(drawing), str(root / 'bad-member-role.json'))
        check = root / 'bad-member-role-readback.json'
        run(args.dwgread, '-O', 'JSON', '-o', str(check), str(drawing))
        peer = json.loads(check.read_text())
        members = [o for o in peer['OBJECTS'] if o.get('object') == 'GROUP'
                   and o.get('handle', [0, 0, 0])[2] == 49]
        if len(members) != 1 or members[0].get('groups') != [[2, 1, 46, 46]]:
            raise RuntimeError('wrong-member-role peer readback mismatch')
        results['badMemberRole'] = {'path': str(drawing), 'sha256': digest(drawing)}
    if args.alternate_group:
        broken = json.loads((root / 'eed.json').read_text())
        nod = [o for o in broken['OBJECTS'] if o.get('object') == 'DICTIONARY'
               and o.get('handle', [0, 0, 0])[2] == 53]
        bad_key = 'ACAD_MLINESTYLE' if args.eed_on_mline else 'ACAD_GROUP'
        expected_target = 23 if args.eed_on_mline else group
        if len(nod) != 1 or nod[0].get('items', {}).get(bad_key, [0])[-1] != expected_target:
            raise RuntimeError('unexpected source graph before negative control')
        nod[0]['items'][bad_key] = [2, 1, 25, 25]
        (root / 'bad-graph.json').write_text(json.dumps(broken))
        drawing = root / 'bad-graph.dwg'
        run(args.dwgwrite, '--as', args.version, '-I', 'JSON', '-o', str(drawing), str(root / 'bad-graph.json'))
        check = root / 'bad-graph-readback.json'
        run(args.dwgread, '-O', 'JSON', '-o', str(check), str(drawing))
        peer = json.loads(check.read_text())
        roots = [o for o in peer['OBJECTS'] if o.get('object') == 'DICTIONARY'
                 and o.get('handle', [0, 0, 0])[2] == 53]
        if len(roots) != 1 or roots[0].get('items', {}).get(bad_key, [0])[-1] != 25:
            raise RuntimeError('bad graph peer readback mismatch')
        results['badGraph'] = {'path': str(drawing), 'sha256': digest(drawing)}
    for name, value in (('utf8', 'café'), ('wide', '漢é')):
        drawing = root / (name + '.dwg')
        replace_eed_text(args.dwgread, root / 'eed.dwg', drawing, value, appid, target_handle)
        results[name] = {'path': str(drawing), 'sha256': digest(drawing)}
    print(json.dumps({'recipeSha256': digest(recipe), 'controls': results,
                      'version': args.version, 'dictionaryHandle': group, 'appidHandle': appid,
                      'nonemptyGroup': args.nonempty_group, 'eedTargetHandle': target_handle,
                      'status': 'research control; no support claim'}, indent=2))


if __name__ == '__main__':
    main()
