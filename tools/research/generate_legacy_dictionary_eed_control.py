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


def replace_eed_text(dwgread, source, destination, value):
    """Patch the locally generated fixed-size EED string and repair its CRC."""
    raw = value.encode('utf-8')
    if len(raw) != 5:
        raise ValueError('control text must occupy exactly five UTF-8 bytes')
    trace = run(dwgread, '-v4', str(source))
    blocks = re.split(r'(?=Object number:)', trace.stdout + trace.stderr)
    matches = [b for b in blocks if re.search(r'\bhandle: 0\.\d+\.D\s', b)]
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
    if (bits.bs() != 42 or bits.handle(0, own=True)['value'] != 13 or bits.bs() != 12
            or bits.handle(5)['value'] != 18 or bits.take(8) != 0 or bits.take(8) != 5):
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
    run(args.dwgadd, '--as', args.version, '-o', str(base), str(recipe))
    run(args.dwgread, '-O', 'JSON', '-o', str(source), str(base))
    document = json.loads(source.read_text())
    styles = [o for o in document['OBJECTS'] if o.get('object') == 'MLINESTYLE']
    dictionaries = [o for o in document['OBJECTS'] if o.get('object') == 'DICTIONARY'
                    and o.get('handle', [0, 0, 0])[2] == 13]
    apps = [o for o in document['OBJECTS'] if o.get('object') == 'APPID'
            and o.get('handle', [0, 0, 0])[2] == 18 and o.get('name') == 'ACAD']
    if len(styles) != 1 or len(dictionaries) != 1 or len(apps) != 1:
        raise RuntimeError('unexpected dwgadd graph or APPID')
    # dwgread JSON rounds the angle; restore the exact generated binary value
    # so the baseline remains within the existing MLINESTYLE wire profile.
    styles[0]['start_angle'] = styles[0]['end_angle'] = 1.5707963267948966
    results = {}
    for name in ('baseline', 'eed', 'unqualified'):
        if name == 'eed':
            dictionaries[0]['eed'] = [
                {'size': 12, 'handle': [5, 1, 18], 'code': 0, 'value': 'HELLO'},
                {'code': 70, 'value': 42},
            ]
        if name == 'unqualified':
            # A 1071 long is deliberately outside the bounded source proof.
            dictionaries[0]['eed'][0]['size'] = 17
            dictionaries[0]['eed'].append({'code': 71, 'value': 123456})
        path = root / (name + '.json')
        drawing = root / (name + '.dwg')
        path.write_text(json.dumps(document))
        run(args.dwgwrite, '--as', args.version, '-I', 'JSON', '-o', str(drawing), str(path))
        check = root / (name + '-readback.json')
        run(args.dwgread, '-O', 'JSON', '-o', str(check), str(drawing))
        reread = json.loads(check.read_text())
        selected = [o for o in reread['OBJECTS'] if o.get('object') == 'DICTIONARY'
                    and o.get('handle', [0, 0, 0])[2] == 13]
        if len(selected) != 1 or (name != 'baseline' and selected[0].get('eed') != dictionaries[0]['eed']):
            raise RuntimeError(f'{name} independent JSON readback mismatch')
        if name == 'baseline' and selected[0].get('eed'):
            raise RuntimeError('baseline unexpectedly contains EED')
        results[name] = {'path': str(drawing), 'sha256': digest(drawing)}
    for name, value in (('utf8', 'café'), ('wide', '漢é')):
        drawing = root / (name + '.dwg')
        replace_eed_text(args.dwgread, root / 'eed.dwg', drawing, value)
        results[name] = {'path': str(drawing), 'sha256': digest(drawing)}
    print(json.dumps({'recipeSha256': digest(recipe), 'controls': results,
                      'version': args.version, 'dictionaryHandle': 13, 'appidHandle': 18,
                      'status': 'research control; no support claim'}, indent=2))


if __name__ == '__main__':
    main()
