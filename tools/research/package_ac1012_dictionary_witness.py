#!/usr/bin/env python3
"""Package an existing generated ODA AC1012 pair for a native witness.

The source run is produced by generate_ac1012_dictionary_witness.py --oda.
This verifies the receipt, physical DXF fields and the AC1012 dictionary
frames before copying them to a new scratch directory. It does not perform
or claim any AutoCAD duplicate-merge observation.
"""
import argparse
import json
from pathlib import Path
import shutil
import tempfile

from audit_dxf_dictionary_cloning import audit_bytes
from generate_ac1012_dictionary_witness import (
    executable_identity, model_and_frame, physical_dxf_version, require, run,
    selected_dictionary, sha,
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run", type=Path, help="completed generator run directory")
    parser.add_argument("output", type=Path, help="new handoff directory")
    parser.add_argument("--dwgread", default="dwgread")
    args = parser.parse_args()
    require(not args.output.exists(), "output already exists")
    require(args.output.parent.is_dir(), "output parent does not exist")
    require(b"0.14" in run([args.dwgread, "--version"]).stdout, "dwgread 0.14 required")
    dwgread = executable_identity(args.dwgread)
    raw_receipt = (args.run / "receipt.json").read_bytes()
    receipt = json.loads(raw_receipt)
    oda = receipt["oda"]
    require(set(oda["samples"]) == {"0", "1"}, "expected both seed controls")
    records = {}
    with tempfile.TemporaryDirectory(prefix="ac1012-packet-", dir=args.output.parent) as tmp:
        packet = Path(tmp) / "packet"
        packet.mkdir()
        for flag in (0, 1):
            record = oda["samples"][str(flag)]
            files = [("source.dwg", args.run / "oda-r13" / f"seed-{flag}.dwg", record),
                     ("seed.dxf", args.run / "oda-input" / f"seed-{flag}.dxf", record["seed"]),
                     ("oda-r14.dxf", args.run / "oda-to-acad14" / f"seed-{flag}.dxf", record["ACAD14"]),
                     ("oda-r2000.dxf", args.run / "oda-to-acad2000" / f"seed-{flag}.dxf", record["ACAD2000"])]
            destination = packet / f"seed-{flag}"
            destination.mkdir()
            hashes = {}
            for name, source, expected in files:
                data = source.read_bytes()
                require(sha(data) == expected["sha256"], f"receipt hash mismatch: {source}")
                (destination / name).write_bytes(data)
                hashes[name] = sha(data)
            seed = selected_dictionary(audit_bytes((destination / "seed.dxf").read_bytes(), include_rows=True))
            require(seed["cloning"] == flag, "seed cloning field differs")
            for name, version, clone in (("oda-r14.dxf", "AC1014", None),
                                         ("oda-r2000.dxf", "AC1015", 1)):
                data = (destination / name).read_bytes()
                require(physical_dxf_version(data) == version, "peer DXF version differs")
                require(selected_dictionary(audit_bytes(data, include_rows=True))["cloning"] == clone,
                        "peer dictionary cloning field differs")
            # model_and_frame creates a JSON peer decode beside its input, so
            # perform that audit on a scratch copy rather than the handoff.
            audit = Path(tmp) / f"audit-{flag}.dwg"
            shutil.copyfile(destination / "source.dwg", audit)
            observed = model_and_frame(args.dwgread, audit, "r13")
            require(observed["wire"]["cloning"] is None, "unexpected AC1012 cloning field")
            require(observed["wire"]["frameSha256"] == record["wire"]["frameSha256"],
                    "dictionary frame differs from receipt")
            records[str(flag)] = {"files": hashes, "seedCloning": flag,
                                  "sourceHandle": observed["sourceHandle"],
                                  "nodHandle": observed["nodHandle"],
                                  "sourceFrameSha256": observed["wire"]["frameSha256"],
                                  "sourceCloningField": None,
                                  "odaR14CloningField": None,
                                  "odaR2000CloningField": 1}
        require(records["0"]["sourceFrameSha256"] == records["1"]["sourceFrameSha256"],
                "paired source frames differ")
        guide = Path(__file__).with_name("AC1012_DICTIONARY_NATIVE_WITNESS.md")
        shutil.copyfile(guide, packet / "NATIVE_WITNESS.md")
        (packet / "README.txt").write_text(
            "Generated AC1012 dictionary source controls; no native semantics observed.\n"
            "Verify file hashes against manifest.json before and after the witness.\n"
            "seed-0 and seed-1 are separate seed DXFs with explicit cloning values;\n"
            "their selected source.dwg dictionary frames are identical and have no\n"
            "cloning field. ODA reopens these files; AutoCAD acceptance is untested.\n"
            "oda-r14.dxf and oda-r2000.dxf are ODA outputs, not native outputs.\n"
            "Run the NATIVE_WITNESS.md procedure on copies in a fresh host session.\n"
            "The markdown guide contains repository-relative links; the complete\n"
            "original is available in the libdxfrw repository under tools/research.\n"
            "A normal open, native duplicate experiment, and independent readback\n"
            "remain required before changing any support claim.\n")
        (packet / "source-receipt.json").write_bytes(raw_receipt)
        manifest = {"schema": 1, "scope": "generated source and peer output; native semantics unobserved",
                    "sourceReceiptSha256": sha(raw_receipt),
                    "generatorSha256": receipt["generatorSha256"],
                    "auditDwgread": dwgread,
                    "oda": {"bundleVersion": oda["bundleVersion"], "executable": oda["executable"]},
                    "guideSha256": sha((packet / "NATIVE_WITNESS.md").read_bytes()),
                    "readmeSha256": sha((packet / "README.txt").read_bytes()),
                    "samples": records}
        (packet / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
        packet.rename(args.output)
    print(args.output)


if __name__ == "__main__":
    main()
