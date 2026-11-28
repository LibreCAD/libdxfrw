#!/usr/bin/env python3
"""Compare the pinned forum DWG's peer inventory with its ODA DXF output.

This is source-specific transport evidence; a null counted DWG handle is not
proved to be a deleted entry by this comparison.
"""
import hashlib
import json
from pathlib import Path
import sys

EXPECTED = (
    "685bf22ed533121238c62c07357bf03fa6ddb40983cff7967c481fbb244b6b4f",
    "1ad1aa1677c56ff001ef676be3c810a7cd2c5b18c94e5be2fa1f426f62d14b87",
    "1b0010ad90eb12b6a27fcc81da0361e78a8c131448d131501a5110d04a07ec96",
)


def main():
    if len(sys.argv) != 4:
        raise ValueError("expected source DWG, LibreDWG JSON, ODA ASCII DXF")
    files = [Path(path).read_bytes() for path in sys.argv[1:]]
    if tuple(hashlib.sha256(data).hexdigest() for data in files) != EXPECTED:
        raise ValueError("input hashes do not match the selected evidence")
    objects = json.loads(files[1])["OBJECTS"]
    controls = [obj for obj in objects if obj.get("object") == "LTYPE_CONTROL"]
    if len(controls) != 1:
        raise ValueError("expected one LTYPE_CONTROL")
    control = controls[0]
    if (control["handle"][-1] != 5 or control["entries"] != [[2, 1, 22, 22], [2, 0, 0, 0]]
            or control["byblock"][-1] != 20 or control["bylayer"][-1] != 21):
        raise ValueError("unexpected DWG control profile")
    source_rows = sorted((obj["handle"][-1], obj["name"], obj["ownerhandle"][-1])
                         for obj in objects if obj.get("object") == "LTYPE")
    expected_rows = [(20, "ByBlock", 5), (21, "ByLayer", 5), (22, "Continuous", 5)]
    if source_rows != expected_rows:
        raise ValueError("unexpected DWG LTYPE records")

    lines = files[2].decode("latin-1").splitlines()
    if len(lines) % 2:
        raise ValueError("invalid DXF pair count")
    pairs = [(lines[i].strip(), lines[i + 1].strip()) for i in range(0, len(lines), 2)]
    if not pairs or pairs[-1] != ("0", "EOF"):
        raise ValueError("DXF EOF missing")
    starts = [i for i in range(len(pairs) - 1)
              if pairs[i] == ("0", "TABLE") and pairs[i + 1] == ("2", "LTYPE")]
    if len(starts) != 1:
        raise ValueError("expected one LTYPE table")
    index = starts[0] + 2
    records = [[]]
    while index < len(pairs) and pairs[index] != ("0", "ENDTAB"):
        code, value = pairs[index]
        if code == "0":
            if value != "LTYPE":
                raise ValueError("unexpected LTYPE table record")
            records.append([])
        else:
            records[-1].append((code, value))
        index += 1
    if index == len(pairs):
        raise ValueError("LTYPE table has no ENDTAB")

    def one(record, code):
        values = [value for key, value in record if key == code]
        if len(values) != 1:
            raise ValueError("missing or repeated DXF group " + code)
        return values[0]

    header = records[0]
    if one(header, "5") != "5" or one(header, "70") != "1":
        raise ValueError("unexpected DXF LTYPE table header")
    target_rows = sorted((int(one(row, "5"), 16), one(row, "2"),
                          int(one(row, "330"), 16)) for row in records[1:])
    if target_rows != expected_rows:
        raise ValueError("DXF LTYPE identities or owners differ")
    print(json.dumps({"source_counted_entries": control["entries"],
                      "dxf_table_group_70": 1, "source_and_dxf_rows": target_rows},
                     sort_keys=True))


if __name__ == "__main__":
    try:
        main()
    except (KeyError, ValueError, OSError, UnicodeError) as exc:
        print("comparison refused: " + str(exc), file=sys.stderr)
        sys.exit(1)
