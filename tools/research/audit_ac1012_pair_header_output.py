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
from generate_ac1012_dictionary_witness import require, run, sha


SKIP = set("""VX_TABLE_RECORD HANDSEED CLAYER TEXTSTYLE CELTYPE DIMSTYLE
CMLSTYLE DIMTXSTY UCSNAME PUCSNAME CECOLOR MENU TDUCREATE TDUUPDATE TDINDWG
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
    require(set(omitted) == OMITTED and len(matched) == 141, "unexpected HEADER output profile")
    return {"matchedFields": len(matched), "matchedNames": matched, "omittedNames": sorted(omitted),
            "selectedAliases": ALIASES}


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
            peer = json.loads(peer_file.read_text())["HEADER"]
            source = audit_prefix(data, peer)
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
                require(compare(source["fields"], drawing.header) == checked, "corrected DXF retry changed")
                auditor = drawing.audit()
                require(not auditor.errors and not auditor.fixes, "independent DXF audit failed")
                checked.update({"sha256": sha(output.read_bytes()), "strictSentinelPreserved": True,
                                "changedDimtihRejectedAndRetryPassed": True,
                                "auditErrors": 0, "auditFixes": 0})
                results[f"{side}-{'binary' if binary else 'ascii'}"] = checked
    print(json.dumps({"candidateSha256": sha(args.dwg2dxf.read_bytes()), "outputs": results}, indent=2))


if __name__ == "__main__":
    main()
