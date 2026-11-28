#!/usr/bin/env python3
"""Compare bounded FIELD spans before and after an ODA AC1021 resave.

The resave is derived from the original drawing, so it is not an independent
native semantic witness. This check does not authorize a parser change.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import subprocess

import verify_ac1021_metric_field_spans as metric


def frames(probe, source):
    result = subprocess.run([str(probe), str(source)], capture_output=True, timeout=60, check=True)
    metric.span.require(len(result.stdout) <= metric.span.MAX_BYTES, "oversized raw probe output")
    rows = {}
    for line in result.stdout.decode("ascii", errors="strict").splitlines():
        if not line.startswith("RAW "):
            continue
        parts = line.split()
        metric.span.require(len(parts) == 4, "invalid raw frame line")
        handle, length, data = int(parts[1], 16), int(parts[2]), bytes.fromhex(parts[3])
        metric.span.require(handle not in rows and length == len(data) <= 1024 * 1024,
                            "duplicate or invalid raw frame")
        rows[handle] = data
    metric.span.require(len(rows) == 162, "unexpected FIELD frame count")
    return rows


def reject(call, message):
    try:
        call()
    except ValueError:
        return
    raise ValueError(message)


def flip(data, bit):
    result = bytearray(data)
    result[bit // 8] ^= 1 << (7 - bit % 8)
    return bytes(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", required=True, type=Path)
    parser.add_argument("--resaved", required=True, type=Path)
    parser.add_argument("--resaved-sha256", required=True)
    parser.add_argument("--oda-dxf", required=True, type=Path)
    parser.add_argument("--oda-sha256", required=True)
    parser.add_argument("--acadsharp-log", required=True, type=Path)
    parser.add_argument("--acadsharp-sha256", required=True)
    parser.add_argument("--raw-probe", required=True, type=Path)
    parser.add_argument("--edit-handle", type=lambda value: int(value, 16))
    parser.add_argument("--edit-before")
    parser.add_argument("--edit-after")
    parser.add_argument("--baseline-dxf", type=Path)
    parser.add_argument("--baseline-sha256")
    parser.add_argument("--edited-dxf", type=Path)
    parser.add_argument("--edited-sha256")
    args = parser.parse_args()
    original = metric.span.checked_bytes(args.original, metric.SOURCE_SHA)
    resaved = metric.span.checked_bytes(args.resaved, args.resaved_sha256)
    metric.span.require(resaved[:6] == b"AC1021" and resaved != original, "invalid resaved source")
    oda = metric.span.checked_bytes(args.oda_dxf, args.oda_sha256)
    sharp_data = metric.span.checked_bytes(args.acadsharp_log, args.acadsharp_sha256)
    records = metric.span.records_from_dxf(oda)
    selected = metric.selected_fields(records)
    edit_args = (args.edit_handle, args.edit_before, args.edit_after, args.baseline_dxf,
                 args.baseline_sha256, args.edited_dxf, args.edited_sha256)
    metric.span.require(all(value is None for value in edit_args) or
                        all(value is not None for value in edit_args), "incomplete edited-source profile")
    edited_witness = None
    if args.edit_handle is not None:
        baseline = metric.span.checked_bytes(args.baseline_dxf, args.baseline_sha256)
        edited = metric.span.checked_bytes(args.edited_dxf, args.edited_sha256)
        old_lines, new_lines = baseline.splitlines(), edited.splitlines()
        metric.span.require(len(old_lines) == len(new_lines), "edited DXF line count changed")
        changes = [i for i, (old, new) in enumerate(zip(old_lines, new_lines)) if old != new]
        metric.span.require(len(changes) == 1 and changes[0] % 2 == 1 and
                            old_lines[changes[0] - 1].strip() == b"1" and
                            old_lines[changes[0]].strip().decode("ascii") == args.edit_before and
                            new_lines[changes[0]].strip().decode("ascii") == args.edit_after,
                            "edited DXF is not the selected one-value change")
        original_selected = metric.selected_fields(metric.span.records_from_dxf(baseline))
        edited_records = metric.span.records_from_dxf(edited)
        edited_selected = metric.selected_fields(edited_records)
        metric.span.require(args.edit_handle in selected and args.edit_handle in original_selected and
                            args.edit_handle in edited_selected, "edited FIELD is missing")
        allowed_codes = {0, 1, 2, 5, 6, 7, 90, 91, 92, 93, 94, 95, 96, 97, 98,
                         100, 140, 300, 301, 302, 304, 330, 331}
        metric.span.require(all(code in allowed_codes for code, _ in edited_records[args.edit_handle]),
                            "edited FIELD has an unqualified opaque group")
        for handle in selected:
            old, new = original_selected[handle], edited_selected[handle]
            expected = list(old)
            if handle == args.edit_handle:
                metric.span.require(old[2] == args.edit_before and new[2] == args.edit_after,
                                    "edited FIELD value differs")
                expected[2] = args.edit_after
            metric.span.require(tuple(expected) == new == selected[handle],
                                f"{handle:X}: edited or readback FIELD profile changed")
        edited_witness = {"handle": f"{args.edit_handle:X}", "before": args.edit_before,
                          "after": args.edit_after, "baselineDxfSha256": args.baseline_sha256.lower(),
                          "editedDxfSha256": args.edited_sha256.lower(),
                          "opaqueFieldGroupsAbsent": True}
    sharp = metric.sharp_values(sharp_data)
    sharp_edges = metric.span.acadsharp_fields(sharp_data)
    before, after = frames(args.raw_probe, args.original), frames(args.raw_probe, args.resaved)
    metric.span.require(set(before) == set(after), "FIELD handle set changed")
    changed = sum(before[handle] != after[handle] for handle in before)
    metric.span.require(changed == 162, "unexpectedly unchanged FIELD frames")
    counts = Counter()
    for handle, (owner, target, name, flags, evaluator) in selected.items():
        metric.span.require(handle in sharp and sharp[handle].get("owner") == owner and
                            sharp[handle].get("ObjectPropertyName") == name and
                            handle in sharp_edges and sharp_edges[handle].get("objects") == f"{target:X}",
                            f"{handle:X}: independent value mismatch")
        original_name = args.edit_before if handle == args.edit_handle else name
        old = metric.span.inspect_frame(before[handle], handle, owner, target, original_name, flags)
        new = metric.span.inspect_frame(after[handle], handle, owner, target, name, flags,
                                        owner_encoding="absolute")
        metric.span.require(old == new == (287, 309, 32, 567), "selected body span differs")
        bits = ''.join(f"{value:08b}" for value in after[handle])
        end = int.from_bytes(metric.span.bit_bytes(bits, 18, 4), "little")
        before_bits = ''.join(f"{value:08b}" for value in before[handle])
        before_end = int.from_bytes(metric.span.bit_bytes(before_bits, 18, 4), "little")
        reject(lambda: metric.span.inspect_frame(before[handle], handle, owner, target, original_name, flags,
                                                 owner_encoding="absolute"),
               "absolute role accepted in the relative profile")
        reject(lambda: metric.span.inspect_frame(flip(before[handle], before_end + 3), handle, owner,
                                                 target, original_name, flags), "wrong relative owner role accepted")
        reject(lambda: metric.span.inspect_frame(after[handle], handle, owner, target, name, flags),
               "relative role accepted in the absolute profile")
        reject(lambda: metric.span.inspect_frame(flip(after[handle], end + 3), handle, owner, target,
                                                 name, flags, owner_encoding="absolute"),
               "wrong absolute owner role accepted")
        reject(lambda: metric.span.inspect_frame(flip(after[handle], new[1]), handle, owner, target,
                                                 name, flags, owner_encoding="absolute"),
               "changed in-body payload accepted")
        metric.span.require(metric.span.inspect_frame(after[handle], handle, owner, target, name, flags,
                                                       owner_encoding="absolute") == new,
                            "clean retry changed")
        counts[evaluator] += 1
    before_remaining = metric.remaining_spans(records, selected, before, sharp)
    after_remaining = metric.remaining_spans(records, selected, after, sharp, owner_encoding="absolute")
    def summary(result):
        return [(row["evaluator"], row["rootFlags"], row["rootType"], row["stringsPerField"], row["count"])
                for row in result["profiles"]]
    metric.span.require(summary(before_remaining) == summary(after_remaining) and
                        before_remaining["valueSpans"] == after_remaining["valueSpans"],
                        "remaining candidate profiles differ")
    example = int(after_remaining["profiles"][0]["example"]["handle"], 16)
    example_bits = ''.join(f"{value:08b}" for value in after[example])
    example_end = int.from_bytes(metric.span.bit_bytes(example_bits, 18, 4), "little")
    damaged = dict(after)
    damaged[example] = flip(after[example], example_end + 3)
    reject(lambda: metric.remaining_spans(records, selected, damaged, sharp, owner_encoding="absolute"),
           "wrong remaining owner role accepted")
    metric.span.require(metric.remaining_spans(records, selected, after, sharp, owner_encoding="absolute") ==
                        after_remaining, "remaining profile retry changed")
    print(json.dumps({"originalSha256": metric.SOURCE_SHA, "resavedSha256": args.resaved_sha256.lower(),
                      "odaDxfSha256": args.oda_sha256.lower(), "acadsharpSha256": args.acadsharp_sha256.lower(),
                      "rawProbeSha256": hashlib.sha256(args.raw_probe.read_bytes()).hexdigest(),
                      "frames": len(before), "rewrittenFrames": changed, "selected": len(selected),
                      "evaluators": dict(sorted(counts.items())),
                      "originalRemaining": before_remaining, "resavedRemaining": after_remaining,
                      "editedWitness": edited_witness,
                      "wrongOwnerAndPayloadRejectedAndRetryPassed": True,
                      "scope": "resaved derivative and selected source spans; no native or support claim"},
                     indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
