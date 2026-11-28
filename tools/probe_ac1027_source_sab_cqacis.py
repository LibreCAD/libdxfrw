#!/usr/bin/env python3
"""Observe three hash-bound source SABs with optional cq-acis/OCCT.

The caller supplies scratch files extracted from the documented AC1027 source;
this tool does not establish the DWG/AcDs association or an independent oracle.
A reported shape may have passed through sewing and remains advisory.  Conversion
failures are reported per entity, not silently omitted. No fixture is bundled.
"""
from __future__ import annotations

import argparse
from collections import Counter
from hashlib import sha256
from importlib.metadata import version
import json
import math
from pathlib import Path
import sys

from verify_acis218_sab_cqacis import bounded_read

EXPECTED = {
    '176': '596d37aaf81ebe9662bd22ba199bce876403dec8da71dd3287b0255adb354782',
    '37d': 'fabe3e9804c44472907987ea96409b30b7db35575a34688430b0157d1b6c5566',
    '2e1': '5ca4756434d068b932ac6e0100e5ff07eae36c7eed29a43367eeff8113cca0ec',
}
MAX_BYTES = 65536


def read_payload(directory: Path, identity: str) -> bytes:
    path = directory / (identity + '.sab')
    raw = bounded_read(path, MAX_BYTES)
    if len(raw) > MAX_BYTES or sha256(raw).hexdigest() != EXPECTED[identity]:
        raise ValueError(f'{identity}: payload digest or size mismatch')
    return raw


def observe(identity: str, raw: bytes) -> dict:
    from cq_acis import parse_sab_model, to_cadquery
    model = parse_sab_model(raw, source_id=identity)
    entities = model.entities
    by_index = {e.raw.index if hasattr(e, 'raw') else e.index: e for e in entities}
    raw_records = []
    for e in entities:
        if type(e).__name__ != 'RawEntity':
            continue
        refs = [value.index for value in e.values
                if type(value).__name__ == 'EntityRef' and value.index >= 0]
        raw_records.append({'index': e.index, 'type': e.type_name,
                            'refs': [{'index': i, 'type': type(by_index.get(i)).__name__}
                                     for i in refs]})
    bodies = model.bodies()
    row = {'handle': identity.upper(), 'sha256': sha256(raw).hexdigest(),
           'body_count': len(bodies), 'body_transforms': [str(b.transform) for b in bodies],
           'units_mm_reported_by_parser': model.metadata.units_mm,
           'dialect': model.metadata.dialect,
           'entity_types': dict(sorted(Counter(type(e).__name__ for e in entities).items())),
           'diagnostics': [str(d) for d in model.diagnostics], 'raw_records': raw_records}
    try:
        result = to_cadquery(model)
        shapes = []
        for shape in result.vals():
            bounds = shape.BoundingBox()
            values = [bounds.xmin, bounds.ymin, bounds.zmin,
                      bounds.xmax, bounds.ymax, bounds.zmax,
                      shape.Volume(), shape.Area()]
            if not all(math.isfinite(value) for value in values):
                raise ValueError('non-finite shape measurement')
            shapes.append({'type': type(shape).__name__, 'valid': shape.isValid(),
                           'solids': len(shape.Solids()), 'shells': len(shape.Shells()),
                           'faces': len(shape.Faces()), 'edges': len(shape.Edges()),
                           'vertices': len(shape.Vertices()), 'bounds': values[:6],
                           'volume': values[6], 'area': values[7]})
        row['conversion'] = {'status': 'reported', 'shapes': shapes}
    except Exception as exc:
        row['conversion'] = {'status': 'failed', 'error_type': type(exc).__name__,
                             'error': str(exc)[:1000]}
    return row


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sab-dir', type=Path, required=True)
    args = parser.parse_args()
    if version('cq-acis') != '0.3.12':
        raise ValueError('expected pinned cq-acis 0.3.12')
    # Check every payload before invoking any external parser or converter.
    payloads = {identity: read_payload(args.sab_dir, identity) for identity in EXPECTED}
    report = {'schema': 'libdxfrw-ac1027-cqacis-advisory-v1',
              'qualification_eligible': False,
              'reader': {'cq_acis': version('cq-acis'), 'cadquery': version('cadquery'),
                         'cadquery_ocp': version('cadquery-ocp'), 'python': sys.version},
              'limit': 'OCCT-family converter, potentially sewn geometry; raw records and placement semantics are not independently resolved.',
              'entities': [observe(identity, raw) for identity, raw in payloads.items()]}
    print(json.dumps(report, indent=2, allow_nan=False))
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError, ImportError) as exc:
        print(f'ERROR: {exc}', file=sys.stderr)
        sys.exit(1)
