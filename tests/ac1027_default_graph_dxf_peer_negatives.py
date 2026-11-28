#!/usr/bin/env python3
"""Fixture-free mutation checks for a hash-bound selected ASCII peer output."""
import argparse
import hashlib
from pathlib import Path
import subprocess
import sys
import tempfile


def require(value, message):
    if not value:
        raise ValueError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dxf", type=Path, required=True)
    parser.add_argument("--sha256", required=True)
    args = parser.parse_args()
    raw = args.dxf.read_bytes()
    require(hashlib.sha256(raw).hexdigest() == args.sha256.lower(), "input digest mismatch")
    auditor = Path(__file__).resolve().parents[1] / "tools/audit_ac1027_default_graph_dxf.py"
    cases = (
        (b"\n347\n35\n 70\n", b"\n347\n36\n 70\n", "LAYER material"),
        (b"\n390\n31\n  0\nENDTAB", b"\n390\n32\n  0\nENDTAB", "LAYER plotstyle"),
        (b"$CMATERIAL\n347\n34\n", b"$CMATERIAL\n347\n35\n", "HEADER material"),
        (b"\n  0\nACDBDICTIONARYWDFLT\n", b"\n  0\nDICTIONARYWDFLT\n", "OBJECT marker"),
        (b"\n  1\nACDBDICTIONARYWDFLT\n", b"\n  1\nDICTIONARYWDFLT\n", "CLASS marker"),
    )
    with tempfile.TemporaryDirectory(prefix="ac1027-graph-peer-") as tmp:
        path = Path(tmp) / "mutated.dxf"
        for before, after, label in cases:
            require(raw.count(before) == 1, f"ambiguous mutation: {label}")
            changed = raw.replace(before, after, 1)
            path.write_bytes(changed)
            digest = hashlib.sha256(changed).hexdigest()
            result = subprocess.run([sys.executable, "-O", str(auditor), "--dxf", str(path),
                                     "--sha256", digest], stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                    check=False)
            require(result.returncode != 0, f"auditor accepted {label}")
    print(f"{len(cases)} raw graph mutation controls PASS")


if __name__ == "__main__":
    main()
