#!/usr/bin/env python3
"""Compare bounded AC1012 HEADER source values with partial DXF outputs.

Requires ezdxf and an explicit dwg2dxf candidate. Generated drawings stay in
temporary storage. This checks the selected pair, not native interpretation.
"""
import argparse
import json
import math
from pathlib import Path
import tempfile

import ezdxf

from audit_ac1012_pair_header_prefix import audit_prefix
from audit_binary_dxf_layout_nod import SENTINEL as BINARY_SENTINEL, read_pairs
from generate_ac1012_dictionary_witness import require, run, sha


SKIP = set("""VX_TABLE_RECORD HANDSEED CLAYER TEXTSTYLE CELTYPE DIMSTYLE
CMLSTYLE DIMTXSTY UCSNAME PUCSNAME TDUCREATE TDUUPDATE TDINDWG
TDUSRTIMER""".split())
OMITTED = set("BLIPMODE ATTREQ ATTDIA DRAGMODE OSMODE COORDS".split())
ALIASES = {"DIMFIT": "DIMATFIT", "DIMUNIT": "DIMLUNIT"}
SENTINEL = b"preserve strict-output destination\n"


def comparable(value, other):
    if isinstance(value, list):
        try:
            return len(value) == len(other) and all(comparable(a, b) for a, b in zip(value, other))
        except TypeError:
            return False
    if isinstance(value, (int, float)) and isinstance(other, (int, float)):
        return math.isclose(value, other, rel_tol=1e-12, abs_tol=1e-12)
    return value == other


def compare(fields, header):
    matched = []
    omitted = []
    for name, field in fields.items():
        if (name in SKIP or name.startswith(("unit", "unknown_", "DICTIONARY_", "BLOCK_RECORD_", "LTYPE_"))
                or name.endswith(("CONTROL_OBJECT", "_T"))):
            continue
        key = "$" + ALIASES.get(name, name)
        if key not in header:
            omitted.append(name)
            continue
        require(comparable(field["value"], header[key]), f"HEADER mismatch: {name}")
        matched.append(name)
    require(set(omitted) == OMITTED and len(matched) == 143, "unexpected HEADER output profile")
    return {"matchedFields": len(matched), "matchedNames": matched, "omittedNames": sorted(omitted),
            "selectedAliases": ALIASES}


def check_references(fields, objects, header):
    checked = {}
    for name in ("CLAYER", "TEXTSTYLE", "CELTYPE", "DIMSTYLE", "CMLSTYLE", "DIMTXSTY"):
        target = fields[name]["value"][2]
        row = objects.get(target)
        require(row is not None and row.get("name") and header.get("$" + name) == row["name"],
                f"HEADER target name mismatch: {name}")
        checked[name] = {"sourceHandle": target, "outputName": row["name"]}
    for name in ("UCSNAME", "PUCSNAME"):
        require(fields[name]["value"][2] == 0 and header.get("$" + name) == "",
                f"non-null HEADER UCS target: {name}")
        checked[name] = {"sourceHandle": 0, "outputName": ""}
    return checked


def check_physical_times(data, fields):
    if data.startswith(BINARY_SENTINEL):
        tags = read_pairs(data)
    else:
        lines = data.decode("utf-8").splitlines()
        require(len(lines) % 2 == 0, "unpaired ASCII DXF tags")
        tags = [(int(lines[i].strip()), lines[i + 1].strip()) for i in range(0, len(lines), 2)]
    start = next((i for i in range(len(tags) - 1)
                  if tags[i] == (0, "SECTION") and tags[i + 1] == (2, "HEADER")), None)
    require(start is not None, "missing physical HEADER")
    expected = {"TDCREATE": "TDUCREATE", "TDUPDATE": "TDUUPDATE",
                "TDINDWG": "TDINDWG", "TDUSRTIMER": "TDUSRTIMER"}
    observed = {}
    for i in range(start + 2, len(tags) - 1):
        code, value = tags[i]
        if (code, value) == (0, "ENDSEC"):
            break
        if code != 9 or not value.startswith("$") or value[1:] not in expected:
            continue
        name = value[1:]
        require(name not in observed and tags[i + 1][0] == 40, f"invalid physical timestamp: {name}")
        day, millis = fields[expected[name]]["value"]
        actual = float(tags[i + 1][1])
        require(math.isclose(actual, day + millis / 86400000.0, rel_tol=0.0, abs_tol=1e-9),
                f"physical timestamp mismatch: {name}")
        observed[name] = actual
    require(set(observed) == set(expected), "missing physical timestamp")
    return observed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("packet", type=Path)
    parser.add_argument("--dwg2dxf", required=True, type=Path)
    parser.add_argument("--dwgread", default="dwgread")
    args = parser.parse_args()
    require(args.dwg2dxf.is_file(), "candidate executable missing")
    manifest = json.loads((args.packet / "manifest.json").read_text())
    require(manifest.get("profile") == "populated-links", "wrong packet profile")
    results = {}
    with tempfile.TemporaryDirectory(prefix="ac1012-header-output-") as temporary:
        root = Path(temporary)
        for side in ("destination", "source"):
            path = args.packet / f"{side}.dwg"
            data = path.read_bytes()
            require(sha(data) == manifest["samples"][side]["files"][path.name], "source hash mismatch")
            peer_file = root / f"{side}.json"
            run([args.dwgread, "-O", "minJSON", str(path), "-o", str(peer_file)])
            drawing_source = json.loads(peer_file.read_text())
            source = audit_prefix(data, drawing_source["HEADER"])
            objects = {row["handle"][-1]: row for row in drawing_source["OBJECTS"]}
            for binary in (False, True):
                output = root / f"{side}-{'binary' if binary else 'ascii'}.dxf"
                output.write_bytes(SENTINEL)
                command = [str(args.dwg2dxf), str(path), "-v2010", "-y"]
                if binary:
                    command.append("-b")
                strict = run(command + [str(output)], allow_failure=True)
                require(strict.returncode != 0 and b"duplicate-merge semantics lack native qualification"
                        in strict.stdout + strict.stderr and output.read_bytes() == SENTINEL,
                        "strict refusal did not preserve destination")
                run(command + ["--allow-partial-dwg-read", str(output)])
                drawing = ezdxf.readfile(output)
                checked = compare(source["fields"], drawing.header)
                checked["references"] = check_references(source["fields"], objects, drawing.header)
                checked["physicalTimes"] = check_physical_times(output.read_bytes(), source["fields"])
                altered_fields = dict(source["fields"])
                altered_create = dict(altered_fields["TDUCREATE"])
                day, millis = altered_create["value"]
                altered_create["value"] = [day + 1, millis]
                altered_fields["TDUCREATE"] = altered_create
                try:
                    check_physical_times(output.read_bytes(), altered_fields)
                except ValueError:
                    pass
                else:
                    raise ValueError("changed source creation time was accepted")
                require(check_physical_times(output.read_bytes(), source["fields"]) == checked["physicalTimes"],
                        "corrected physical timestamp retry changed")
                original = drawing.header["$DIMTIH"]
                drawing.header["$DIMTIH"] = 0 if original else 1
                try:
                    compare(source["fields"], drawing.header)
                except ValueError:
                    pass
                else:
                    raise ValueError("changed DXF DIMTIH was accepted")
                finally:
                    drawing.header["$DIMTIH"] = original
                retry = compare(source["fields"], drawing.header)
                require(all(checked[key] == value for key, value in retry.items()), "corrected DXF retry changed")
                auditor = drawing.audit()
                require(not auditor.errors and not auditor.fixes, "independent DXF audit failed")
                checked.update({"sha256": sha(output.read_bytes()), "strictSentinelPreserved": True,
                                "changedDimtihRejectedAndRetryPassed": True,
                                "changedSourceTimeRejectedAndRetryPassed": True,
                                "auditErrors": 0, "auditFixes": 0})
                results[f"{side}-{'binary' if binary else 'ascii'}"] = checked
    print(json.dumps({"candidateSha256": sha(args.dwg2dxf.read_bytes()), "outputs": results}, indent=2))


if __name__ == "__main__":
    main()
