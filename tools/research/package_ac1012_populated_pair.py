#!/usr/bin/env python3
"""Package the locally generated, ODA-reopened AC1012 merge controls.

The package is scratch-only. Physical source checks do not establish AutoCAD
normal-open behavior or the dictionary's native duplicate-merge semantics.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import tempfile

from audit_ac1012_oda_merge_pair import PROFILES, audit
from generate_ac1012_dictionary_witness import executable_identity, require, run, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run", type=Path, help="root-case-aligned ODA scratch run")
    parser.add_argument("output", type=Path, help="new scratch handoff directory")
    parser.add_argument("--dwgread", default="dwgread")
    args = parser.parse_args()
    require(not os.path.lexists(args.output), "output already exists")
    require(args.output.parent.is_dir(), "output parent does not exist")
    dwgread = shutil.which(args.dwgread)
    require(dwgread is not None, "dwgread executable not found")
    require(b"0.14" in run([dwgread, "--version"]).stdout, "dwgread 0.14 required")
    root = Path(__file__).resolve().parents[2]
    receipt_path = root / "docs/research/p6-ac1012-oda-root-case-pair-2026-11-25.json"
    receipt_bytes = receipt_path.read_bytes()
    receipt = json.loads(receipt_bytes)
    guide = root / "tools/research/AC1012_DICTIONARY_NATIVE_WITNESS.md"
    with tempfile.TemporaryDirectory(prefix="ac1012-populated-packet-", dir=args.output.parent) as tmp:
        packet = Path(tmp) / "packet"
        packet.mkdir()
        records = {}
        for side in PROFILES:
            expected = receipt["samples"][side]
            files = ((f"{side}.dwg", args.run / expected["path"], expected["sha256"]),
                     (f"{side}-oda-reopen.dxf", args.run / "allcase-reopen" / f"{side}.dxf",
                      expected["reopenedDxfSha256"]))
            hashes = {}
            for name, source, digest in files:
                data = source.read_bytes()
                require(sha(data) == digest, f"receipt hash mismatch: {source}")
                (packet / name).write_bytes(data)
                hashes[name] = digest
            observed = audit(packet / f"{side}.dwg", side, dwgread, Path(tmp), False, True)
            require(observed["sha256"] == hashes[f"{side}.dwg"], "audited source changed")
            records[side] = {"files": hashes, "stylesDegrees": PROFILES[side],
                             "rootHandle": observed["rootHandle"],
                             "dictionaryHandle": observed["dictionaryHandle"],
                             "dictionaryFrameSha256": observed["frame"]["frameSha256"],
                             "rootFrameSha256": observed["rootFrame"]["frameSha256"]}
        shutil.copyfile(guide, packet / "NATIVE_WITNESS.md")
        (packet / "source-receipt.json").write_bytes(receipt_bytes)
        readme = ("Locally generated, ODA-reopened AC1012 controls; native behavior is unobserved.\n"
                  "Check manifest hashes before and after testing; work on copies only.\n"
                  "destination: SAME_KEY 15 degrees, DEST_ONLY 22 degrees.\n"
                  "source: SAME_KEY 37 degrees, SOURCE_ONLY 51 degrees.\n"
                  "The DXF files are ODA reopens, not AutoCAD output.\n"
                  "Screen normal-open and style validity before native merge testing.\n"
                  "See NATIVE_WITNESS.md; repository-relative links refer to libdxfrw.\n"
                  "The root empty-name/null entry and ACAD EED remain unqualified.\n")
        (packet / "README.txt").write_text(readme)
        manifest = {"schema": 1, "scope": "selected physical controls; native semantics unobserved",
                    "sourceReceiptSha256": sha(receipt_bytes),
                    "dwgread": executable_identity(dwgread),
                    "guideSha256": sha((packet / "NATIVE_WITNESS.md").read_bytes()),
                    "readmeSha256": sha(readme.encode()), "samples": records}
        (packet / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
        require(not os.path.lexists(args.output), "output already exists")
        packet.rename(args.output)
    print(args.output)


if __name__ == "__main__":
    main()
