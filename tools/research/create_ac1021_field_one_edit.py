#!/usr/bin/env python3
"""Create one scratch-only FIELD string edit for an ODA producer witness.

This changes one selected DXF group value and refuses to overwrite output. It
does not claim the edited field is a valid or evaluable native object.
"""
import argparse
import hashlib
import json
from pathlib import Path

import verify_ac1021_metric_field_spans as metric


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--source-sha256", required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--handle", type=lambda value: int(value, 16), required=True)
    parser.add_argument("--before", required=True)
    parser.add_argument("--after", required=True)
    args = parser.parse_args()
    before, after = args.before.encode("ascii"), args.after.encode("ascii")
    metric.span.require(before and len(before) == len(after) and before != after and
                        not any(value in after for value in (b"\0", b"\r", b"\n")),
                        "edit must be a different same-length ASCII string")
    source = metric.span.checked_bytes(args.source, args.source_sha256)
    selected = metric.selected_fields(metric.span.records_from_dxf(source))
    metric.span.require(args.handle in selected and selected[args.handle][2] == args.before,
                        "source FIELD or value differs")
    lines = source.splitlines(keepends=True)
    metric.span.require(len(lines) % 2 == 0, "unpaired DXF lines")
    kind, selected_record, child, changed = None, False, False, 0
    for index in range(0, len(lines), 2):
        code, value = int(lines[index].strip()), lines[index + 1].strip()
        if code == 0:
            kind, selected_record, child = value, False, False
        elif kind == b"FIELD" and code == 5:
            selected_record = int(value, 16) == args.handle
        elif selected_record and code == 6:
            child = value == b"ObjectPropertyName"
        elif selected_record and child and code == 1:
            metric.span.require(value == before and changed == 0, "ambiguous target value")
            ending = b"\r\n" if lines[index + 1].endswith(b"\r\n") else b"\n"
            lines[index + 1] = after + ending
            child, changed = False, changed + 1
    metric.span.require(changed == 1, "target edit missing")
    output = b"".join(lines)
    revised = metric.selected_fields(metric.span.records_from_dxf(output))
    metric.span.require(revised[args.handle][2] == args.after, "edited value did not round trip")
    with args.output.open("xb") as stream:
        stream.write(output)
    print(json.dumps({"sourceSha256": args.source_sha256.lower(),
                      "outputSha256": hashlib.sha256(output).hexdigest(),
                      "handle": f"{args.handle:X}", "before": args.before, "after": args.after}))


if __name__ == "__main__":
    main()
