#!/usr/bin/env python3
"""Compare a scratch callback probe with the independent pinned source walk.

The source digest is checked, but the candidate JSON itself is not a reader
capability; run the documented probe on the hash-matched input before comparing.
"""
import argparse
import hashlib
import json
from pathlib import Path

SOURCE = '1ba45d704ed7cc846ea4d6d08c7c78062f8090842ebc9e9a4cdefcb44ea6ab2a'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--candidate', type=Path, required=True)
    parser.add_argument('--reference', type=Path, required=True)
    args = parser.parse_args()
    if hashlib.sha256(args.source.read_bytes()).hexdigest() != SOURCE:
        raise ValueError('source digest mismatch')
    candidate = json.loads(args.candidate.read_text())
    reference = json.loads(args.reference.read_text())
    if (reference['sourceSha256'] != SOURCE
        or candidate['body'] != reference['dataBitEnd']
        or candidate['tail'] != reference['handleBitEnd']
        or candidate['parent'] != reference['handles'][0]['value']):
        raise ValueError('source, owner, or boundary mismatch')
    if candidate['text'] != reference['textFields']:
        raise ValueError('text span mismatch')
    keys = ('field', 'startBit', 'endBit', 'role', 'size', 'value')
    handles = [{key: row[key] for key in keys} for row in reference['handles'][1:]]
    if candidate['handles'] != handles:
        raise ValueError('handle span mismatch')
    print(f"PASS: {len(candidate['text'])} text and {len(handles)} handle spans")

if __name__ == '__main__':
    main()
