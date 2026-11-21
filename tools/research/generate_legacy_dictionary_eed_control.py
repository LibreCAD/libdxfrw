#!/usr/bin/env python3
"""Build scratch R14 baseline and EED variants for local qualification work.

Requires LibreDWG 0.14's dwgadd, dwgread and dwgwrite. No drawings are
committed; the inputs originate in the tracked, locally authored recipe.
"""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path


def run(*args):
    completed = subprocess.run(args, capture_output=True, text=True, check=False)
    if completed.returncode:
        raise RuntimeError(f"{args[0]} failed: {completed.stderr[-2000:]}")
    return completed


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, required=True)
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
    if any((root / name).exists() for name in ('dwgadd.dwg', 'dwgadd.json', 'baseline.dwg', 'eed.dwg')):
        raise RuntimeError('output directory must not contain a previous control')
    run(args.dwgadd, '--as', 'r14', '-o', str(base), str(recipe))
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
    for name in ('baseline', 'eed'):
        if name == 'eed':
            dictionaries[0]['eed'] = [
                {'size': 12, 'handle': [5, 1, 18], 'code': 0, 'value': 'HELLO'},
                {'code': 70, 'value': 42},
            ]
        path = root / (name + '.json')
        drawing = root / (name + '.dwg')
        path.write_text(json.dumps(document))
        run(args.dwgwrite, '--as', 'r14', '-I', 'JSON', '-o', str(drawing), str(path))
        check = root / (name + '-readback.json')
        run(args.dwgread, '-O', 'JSON', '-o', str(check), str(drawing))
        reread = json.loads(check.read_text())
        selected = [o for o in reread['OBJECTS'] if o.get('object') == 'DICTIONARY'
                    and o.get('handle', [0, 0, 0])[2] == 13]
        if len(selected) != 1 or (name == 'eed' and selected[0].get('eed') != dictionaries[0]['eed']):
            raise RuntimeError(f'{name} independent JSON readback mismatch')
        if name == 'baseline' and selected[0].get('eed'):
            raise RuntimeError('baseline unexpectedly contains EED')
        results[name] = {'path': str(drawing), 'sha256': digest(drawing)}
    print(json.dumps({'recipeSha256': digest(recipe), 'controls': results,
                      'dictionaryHandle': 13, 'appidHandle': 18,
                      'status': 'research control; no support claim'}, indent=2))


if __name__ == '__main__':
    main()
