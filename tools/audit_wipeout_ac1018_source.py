#!/usr/bin/env python3
"""Recheck the pinned AC1018 WIPEOUT witness and its strict-output refusal.

LibreDWG supplies an independent decoded trace; the callback/bit probe uses
the in-tree decompressor and frame reader. This does not qualify output.
"""
import hashlib
import re
import subprocess
import sys
import tempfile
from pathlib import Path

SOURCE_SHA256 = "ec41e065f406cd19d133940892754d5c76e85c1c8f6bd3f283632a75fddebb54"
LIBREDWG_SHA256 = "536eef295f1dfdc2e63b4f1cd2961905f98c55bbd1fe017ef3574c0f1345f1ff"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(value, message):
    if not value:
        raise ValueError(message)


def main():
    require(len(sys.argv) == 5,
            "usage: audit_wipeout_ac1018_source.py <witness.dwg> <compiled-probe> <dwgread> <dwg2dxf>")
    source, probe, peer, converter = map(Path, sys.argv[1:])
    require(digest(source) == SOURCE_SHA256, "source SHA-256 mismatch")
    require(digest(peer.resolve()) == LIBREDWG_SHA256, "LibreDWG SHA-256 mismatch")
    subprocess.run([str(probe), str(source)], check=True)
    with tempfile.TemporaryDirectory() as temporary:
        temp = Path(temporary)
        peer_result = subprocess.run(
            [str(peer), "-v9", "-O", "JSON", "-o", str(temp / "peer.json"), str(source)],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
            encoding="utf-8", errors="replace", check=False)
        require(peer_result.returncode == 0, "LibreDWG read failed")
        trace = peer_result.stdout
        blocks = trace.split("< Next object:")
        matches = [b for b in blocks if "Decode object WIPEOUTVARIABLES\n" in b
                   and re.search(r"(?m)^handle: 0\.1\.30 \[H 5\]$", b)]
        require(len(matches) == 1, "source object missing or ambiguous")
        block = matches[0]
        for pattern in (
            r"Offset: -?\d+ \[MC\] @2737\n",
            r"Size: 15 \[MS\], Type: 513 \[BS\]",
            r"(?m)^bitsize: 81 \[RL\]",
            r"(?m)^num_reactors: 1 \[BL 0\]",
            r"(?m)^is_xdic_missing: 1 \[B 0\]",
            r"(?m)^ownerhandle: \(4\.1\.A\) abs:A \[H 330\] @10\.1$",
            r"(?m)^reactors\[0\]: \(4\.1\.A\) abs:A \[H\* 330\]",
            r"(?m)^display_frame: 0 \[BS 70\] @10\.1$",
            r"(?m)^ padding: 0/0 \(7 bits\)$",
            r"(?m)^ check_CRC 2737-2754 = 17: 2441 == 2441$",
        ):
            require(re.search(pattern, block), "missing peer trace field: " + pattern)
        root = [b for b in blocks if "Decode object DICTIONARY\n" in b
                and re.search(r"(?m)^handle: 0\.1\.A \[H 5\]$", b)]
        require(len(root) == 1 and re.search(r'(?m)^\s*texts\[12\]: "ACAD_WIPEOUT_VARS" \[TV 3\]', root[0])
                and re.search(r"(?m)^itemhandles\[12\]: \(2\.1\.30\) abs:30 \[H\* 0\]", root[0]),
                "peer root-key role mismatch")
        output = temp / "prior.dxf"
        sentinel = b"prior destination\n"
        output.write_bytes(sentinel)
        result = subprocess.run(
            [str(converter), str(source), "-o", str(output), "-y", "-d", "-v2004"],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, check=False)
        require(result.returncode != 0 and output.read_bytes() == sentinel,
                "strict conversion did not refuse atomically")
        require(b"unclaimed bytes=739" in result.stdout
                and b"strict-complete conversion requires successful, complete DWG read disposition" in result.stdout,
                "expected HEADER/strict refusal was not observed")
    require(digest(source) == SOURCE_SHA256, "source changed during audit")
    print("AC1018 WIPEOUTVARIABLES independent roles and strict refusal PASS")


if __name__ == "__main__":
    main()
