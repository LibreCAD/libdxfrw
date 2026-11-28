#!/usr/bin/env python3
"""Audit the hash-pinned ezdxf MTEXT/FIELDLIST DXF source, without copying it.

Usage: python3 tools/research/audit_mtext_field_source.py /path/to/source.dxf
The output records byte spans, hashes and graph edges; it does not evaluate a
field or claim that the source's originating CAD application is known.
"""
import hashlib
import json
from pathlib import Path
import sys

EXPECTED = 'fb09a7af8882787553149a675c6d15b45bab8ff4a49f58200460832eab3de401'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def parse(data):
    if data.startswith(b'AutoCAD Binary DXF'):
        raise ValueError('Expected the pinned ASCII DXF source')
    lines = data.splitlines(keepends=True)
    if len(lines) % 2:
        raise ValueError('Unpaired DXF line')
    tags = []
    offset = 0
    for i in range(0, len(lines), 2):
        code_line, value_line = lines[i:i + 2]
        code = int(code_line.strip())
        value = value_line.rstrip(b'\r\n').decode('utf-8')
        end = offset + len(code_line) + len(value_line)
        tags.append((code, value, offset, end))
        offset = end
    if offset != len(data) or not tags or tags[-1][:2] != (0, 'EOF'):
        raise ValueError('Incomplete source framing')
    records = []
    current = []
    for tag in tags:
        if tag[0] == 0:
            if current:
                records.append(current)
            current = [tag]
        elif current:
            current.append(tag)
    if current:
        records.append(current)
    return records


def values(record, code):
    return [value for c, value, _, _ in record if c == code]


def handle(record):
    matches = values(record, 5)
    if len(matches) != 1:
        raise ValueError('Missing or ambiguous record handle')
    return matches[0].upper()


def common_owner(record):
    depth = 0
    owners = []
    for code, value, _, _ in record[1:]:
        if code == 100:
            break
        if code == 102:
            if value.startswith('{'):
                depth += 1
            elif value == '}':
                depth -= 1
                if depth < 0:
                    raise ValueError('Unbalanced control group')
        elif code == 330 and depth == 0:
            owners.append(value.upper())
    if len(owners) != 1:
        raise ValueError('Missing or ambiguous common owner')
    return owners[0]


def xdictionary(record):
    inside = False
    found = []
    for code, value, _, _ in record:
        if code == 102 and value == '{ACAD_XDICTIONARY':
            if inside:
                raise ValueError('Nested extension dictionary')
            inside = True
        elif code == 102 and value == '}' and inside:
            inside = False
        elif inside and code == 360:
            found.append(value.upper())
    if inside or len(found) != 1:
        raise ValueError('Missing or ambiguous MTEXT extension dictionary')
    return found[0]


def dictionary_entries(record):
    if record[0][1] != 'DICTIONARY':
        raise ValueError('Expected DICTIONARY')
    pending = None
    entries = {}
    body = False
    for code, value, _, _ in record:
        if code == 100 and value == 'AcDbDictionary':
            body = True
        elif body and code == 3:
            if pending is not None:
                raise ValueError('Dictionary entry without target')
            pending = value
        elif body and code in (350, 360) and pending is not None:
            if pending in entries:
                raise ValueError('Duplicate dictionary key')
            entries[pending] = (code, value.upper())
            pending = None
    if pending is not None:
        raise ValueError('Unterminated dictionary entry')
    return entries


def field_children(record):
    in_body = False
    children = []
    for code, value, _, _ in record:
        if code == 100:
            in_body = value == 'AcDbField'
        elif in_body and code == 360:
            children.append(value.upper())
    return children


def field_details(record, data):
    in_body = False
    expression = []
    first_count = None
    spans = []
    references = []
    for code, value, start, end in record:
        if code == 100:
            in_body = value == 'AcDbField'
        elif in_body:
            spans.append({'code': code, 'offset': start, 'length': end - start,
                          'sha256': digest(data[start:end])})
            if 320 <= code <= 369 or code in (390, 1005):
                references.append([code, value.upper()])
            if code == 90 and first_count is None:
                first_count = int(value)
            elif first_count is None and code in (2, 3):
                expression.append(value)
    if first_count is None:
        raise ValueError('FIELD missing declared child count')
    text = ''.join(expression)
    cached = ''.join(values(record, 301))
    return {'declared_child_count': first_count,
            'expression_chars': len(text),
            'expression_sha256': digest(text.encode('utf-8')),
            'field_index_marker_count': text.count('\\_FldIdx'),
            'cached_chars': len(cached),
            'cached_sha256': digest(cached.encode('utf-8')),
            'body_references': references,
            'body_tag_spans': spans}


def fieldlist_members(record):
    in_idset = False
    members = []
    declared = []
    for code, value, _, _ in record:
        if code == 100:
            in_idset = value == 'AcDbIdSet'
        elif in_idset and code == 90:
            declared.append(int(value))
        elif in_idset and code == 330:
            members.append(value.upper())
    if len(declared) != 1 or declared[0] != len(members):
        raise ValueError('FIELDLIST membership count mismatch')
    if len(set(members)) != len(members):
        raise ValueError('Duplicate FIELDLIST member')
    return members


def frame(record, data):
    start, end = record[0][2], record[-1][3]
    return {'type': record[0][1], 'handle': handle(record),
            'offset': start, 'length': end - start,
            'sha256': digest(data[start:end])}


def audit(path):
    data = path.read_bytes()
    if digest(data) != EXPECTED:
        raise ValueError('Source SHA-256 does not match pinned example')
    records = parse(data)
    by_handle = {}
    for record in records:
        hs = values(record, 5)
        if len(hs) == 1:
            h = hs[0].upper()
            if h in by_handle:
                raise ValueError(f'Duplicate handle {h}')
            by_handle[h] = record
    nod = by_handle['C']
    nod_entry = dictionary_entries(nod)['ACAD_FIELDLIST']
    if nod_entry != (350, 'AB'):
        raise ValueError('Unexpected named-object FIELDLIST binding')
    fieldlist = by_handle['AB']
    if fieldlist[0][1] != 'FIELDLIST' or common_owner(fieldlist) != 'C':
        raise ValueError('Unexpected FIELDLIST owner')
    members = fieldlist_members(fieldlist)
    fields = {handle(record): record for record in records
              if record[0][1] == 'FIELD'}
    if set(members) != set(fields) or len(fields) != 16:
        raise ValueError('FIELDLIST does not enumerate all 16 fields exactly')
    roots = []
    used = set()
    for record in records:
        if record[0][1] != 'MTEXT':
            continue
        entity = handle(record)
        xdict_handle = xdictionary(record)
        xdict = by_handle[xdict_handle]
        if common_owner(xdict) != entity:
            raise ValueError('MTEXT extension-dictionary owner mismatch')
        xdict_entries = dictionary_entries(xdict)
        if set(xdict_entries) != {'ACAD_FIELD', 'AcDbContextDataManager'}:
            raise ValueError('Unexpected MTEXT extension-dictionary siblings')
        code, context_handle = xdict_entries['AcDbContextDataManager']
        context = by_handle[context_handle]
        if code != 360 or common_owner(context) != xdict_handle:
            raise ValueError('Unexpected annotation-context dictionary link')
        context_entries = dictionary_entries(context)
        if set(context_entries) != {'ACDB_ANNOTATIONSCALES'}:
            raise ValueError('Unexpected annotation-context dictionary members')
        code, scales_handle = context_entries['ACDB_ANNOTATIONSCALES']
        scales = by_handle[scales_handle]
        if code != 350 or common_owner(scales) != context_handle or dictionary_entries(scales):
            raise ValueError('Unexpected annotation-scale dictionary')
        code, field_dict_handle = xdict_entries['ACAD_FIELD']
        if code != 360:
            raise ValueError('Unexpected ACAD_FIELD dictionary role')
        field_dict = by_handle[field_dict_handle]
        if common_owner(field_dict) != xdict_handle:
            raise ValueError('FIELD dictionary owner mismatch')
        field_entries = dictionary_entries(field_dict)
        if set(field_entries) != {'TEXT'}:
            raise ValueError('Unexpected FIELD dictionary members')
        code, root_handle = field_entries['TEXT']
        if code != 360:
            raise ValueError('Unexpected TEXT field role')
        root = fields[root_handle]
        if common_owner(root) != field_dict_handle:
            raise ValueError('Root FIELD owner mismatch')
        children = field_children(root)
        if len(children) != 1:
            raise ValueError('Expected exactly one child FIELD')
        child = fields[children[0]]
        if common_owner(child) != root_handle or field_children(child):
            raise ValueError('Unexpected child FIELD owner or descendants')
        root_details = field_details(root, data)
        child_details = field_details(child, data)
        if root_details['declared_child_count'] != 1 or child_details['declared_child_count'] != 0:
            raise ValueError('FIELD child count mismatch')
        if root_details['body_references'] != [[360, children[0]]] or child_details['body_references']:
            raise ValueError('Unexpected FIELD body reference')
        if root_details['field_index_marker_count'] != 1:
            raise ValueError('Expected one indexed child expression')
        if values(root, 1)[0] != '_text' or values(child, 1)[:2] != ['AcVar', 'Filename']:
            raise ValueError('Unexpected field evaluator')
        if values(child, 2)[0] != '\\AcVar Filename \\f "%fn6"':
            raise ValueError('Unexpected filename FIELD expression')
        mtext = ''.join(values(record, 3) + values(record, 1))
        if 'mtext_complex_inline_codes.dxf' not in mtext:
            raise ValueError('Expected cached MTEXT filename absent')
        if digest(mtext.encode('utf-8')) != root_details['cached_sha256']:
            raise ValueError('MTEXT does not equal the root FIELD cache')
        if 'mtext_complex_inline_codes.dxf' not in ''.join(values(child, 301)):
            raise ValueError('Expected cached child FIELD filename absent')
        used.update((root_handle, children[0]))
        roots.append({'mtext': entity, 'extension_dictionary': xdict_handle,
                      'context_dictionary': context_handle, 'annotation_scales_dictionary': scales_handle,
                      'field_dictionary': field_dict_handle, 'root_field': root_handle,
                      'child_field': children[0], 'evaluator': 'AcVar',
                      'expression': values(child, 2)[0],
                      'root_cached_sha256': root_details['cached_sha256'],
                      'child_cached_sha256': child_details['cached_sha256']})
    if len(roots) != 8 or used != set(members):
        raise ValueError('Visible MTEXT roots do not cover FIELDLIST')
    relevant = {'C', 'AB'} | set(members)
    for root in roots:
        relevant.update(root[k] for k in ('mtext', 'extension_dictionary', 'context_dictionary',
                                          'annotation_scales_dictionary', 'field_dictionary'))
    return {'source_sha256': digest(data), 'source_bytes': len(data),
            'fieldlist': 'AB', 'fieldlist_owner': 'C', 'members': members,
            'roots': roots,
            'field_details': {h: field_details(fields[h], data) for h in members},
            'frames': [frame(by_handle[h], data) for h in sorted(relevant, key=lambda x: int(x, 16))],
            'limits': ['Source origin application is unverified.',
                       'Matching cached text does not demonstrate dynamic field evaluation or rendering.']}


if __name__ == '__main__':
    if len(sys.argv) != 2:
        raise SystemExit(f'usage: {sys.argv[0]} SOURCE.dxf')
    print(json.dumps(audit(Path(sys.argv[1])), indent=2, sort_keys=True))
