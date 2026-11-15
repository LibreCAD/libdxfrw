#!/usr/bin/env python3
"""Check the scratch AC1021 FIELD derivative with an independent dwgread.

The input is a locally patched ACadRust experiment, not an original producer
witness. Neither this tool nor its success authorizes FIELD output.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile

from audit_mtext_field_source import audit as audit_source

EXPECTED_DWG = '3bc06ca9f22d478bc9477e8d08cadf2f6e245afa46e614f7fb1190e2fdb5c4ac'


def sha(text):
    return hashlib.sha256(text.encode('utf-8')).hexdigest()


def inspect_graph(source, objects):
    indexed = {}
    for obj in objects:
        h = obj.get('handle')
        if not isinstance(h, list) or not h:
            continue
        number = h[-1]
        if number in indexed:
            raise ValueError('duplicate object handle')
        indexed[number] = obj

    def get(hex_handle, kind):
        obj = indexed[int(hex_handle, 16)]
        if obj.get('object', obj.get('entity')) != kind:
            raise ValueError('unexpected object type')
        return obj

    def target(value):
        if not isinstance(value, list) or len(value) < 3:
            raise ValueError('invalid handle reference')
        return value[-1]

    fieldlist = get(source['fieldlist'], 'FIELDLIST')
    nod = get(source['fieldlist_owner'], 'DICTIONARY')
    if target(fieldlist['ownerhandle']) != target(nod['handle']) \
            or target(nod['items']['ACAD_FIELDLIST']) != target(fieldlist['handle']):
        raise ValueError('FIELDLIST named-object link or owner changed')
    members = [target(h) for h in fieldlist['fields']]
    expected = {int(h, 16) for h in source['members']}
    fields = {h for h, obj in indexed.items() if obj.get('object') == 'FIELD'}
    if len(members) != len(expected) or set(members) != expected or fields != expected:
        raise ValueError('FIELDLIST membership differs from source')

    results = []
    for root in source['roots']:
        mtext = get(root['mtext'], 'MTEXT')
        xdict = get(root['extension_dictionary'], 'DICTIONARY')
        field_dict = get(root['field_dictionary'], 'DICTIONARY')
        field = get(root['root_field'], 'FIELD')
        child = get(root['child_field'], 'FIELD')
        h = lambda obj: target(obj['handle'])
        if (target(mtext['xdicobjhandle']) != h(xdict)
                or target(xdict['ownerhandle']) != h(mtext)
                or target(xdict['items']['ACAD_FIELD']) != h(field_dict)
                or target(field_dict['ownerhandle']) != h(xdict)
                or target(field_dict['items']['TEXT']) != h(field)
                or target(field['ownerhandle']) != h(field_dict)
                or target(child['ownerhandle']) != h(field)
                or [target(v) for v in field.get('childs', [])] != [h(child)]):
            raise ValueError('MTEXT/FIELD ownership graph differs from source')
        if (field['id'] != '_text' or child['id'] != 'AcVar'
                or child['code'] != root['expression']
                or sha(child['value_string']) != root['child_cached_sha256']
                or field['value_string'] != mtext['text']):
            raise ValueError('FIELD expression or cached text mismatch')
        results.append({'mtext': root['mtext'], 'root_field': root['root_field'],
                        'child_field': root['child_field'],
                        'derivative_cached_sha256': sha(field['value_string']),
                        'source_cached_sha256': root['root_cached_sha256'],
                        'source_cache_bytes_equal': sha(field['value_string'])
                        == root['root_cached_sha256']})
    if len(results) != 8:
        raise ValueError('unexpected MTEXT root count')
    return {'fieldlist_members': len(members), 'mtext_roots': results}


def main():
    if len(sys.argv) != 3:
        raise SystemExit('usage: audit_ac1021_field_graph.py source.dxf derivative.dwg')
    source = audit_source(Path(sys.argv[1]))
    dwg = Path(sys.argv[2])
    if hashlib.sha256(dwg.read_bytes()).hexdigest() != EXPECTED_DWG:
        raise ValueError('unexpected scratch DWG SHA-256')
    with tempfile.TemporaryDirectory(prefix='field-graph-') as tmp:
        output = Path(tmp) / 'decoded.json'
        result = subprocess.run(['dwgread', '-O', 'minJSON', '-o', str(output),
                                 str(dwg)], capture_output=True, text=True,
                                check=False, timeout=60)
        if result.returncode != 0:
            raise RuntimeError('dwgread failed: ' + result.stderr[-1000:])
        decoded = json.loads(output.read_text())
    result = inspect_graph(source, decoded['OBJECTS'])
    result.update({'dwg_sha256': EXPECTED_DWG,
                   'decoder': decoded.get('created_by'),
                   'limits': ['Scratch patched writer; original producer unverified.',
                              'No physical DWG field spans or evaluation proof.',
                              'Source/derivative root text differs at escape sequences.']})
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
