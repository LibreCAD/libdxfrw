#!/usr/bin/env python3
"""Recheck the pinned AC1018 RASTERVARIABLES witness and its strict-output refusal.

LibreDWG supplies an independent decoded trace; the callback/bit probe uses
the in-tree decompressor and frame reader. This does not qualify output.
"""
import hashlib
import re
import subprocess
import sys
import tempfile
from pathlib import Path

SOURCE_SHA256 = "60be1cbf4f15b895274738761994c0fe7581c115892387ec05ef2482752043ef"
LIBREDWG_SHA256 = "536eef295f1dfdc2e63b4f1cd2961905f98c55bbd1fe017ef3574c0f1345f1ff"


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def require(value, message):
    if not value:
        raise ValueError(message)


def main():
    require(len(sys.argv) == 5,
            "usage: audit_rastervariables_ac1018_source.py <witness.dwg> <compiled-probe> <dwgread> <dwg2dxf>")
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
        matches = [b for b in blocks if "Decode object RASTERVARIABLES\n" in b
                   and re.search(r"(?m)^handle: 0\.1\.2F \[H 5\]$", b)]
        require(len(matches) == 1, "source object missing or ambiguous")
        block = matches[0]
        for pattern in (
            r"Offset: -?\d+ \[MC\] @2753\n",
            r"Size: 17 \[MS\], Type: 512 \[BS\]",
            r"(?m)^bitsize: 103 \[RL\]",
            r"(?m)^num_reactors: 1 \[BL 0\]",
            r"(?m)^is_xdic_missing: 1 \[B 0\]",
            r"(?m)^ownerhandle: \(4\.1\.A\) abs:A \[H 330\] @12\.7$",
            r"(?m)^reactors\[0\]: \(4\.1\.A\) abs:A \[H\* 330\]",
            r"(?m)^class_version: 0 \[BL 90\] @10\.1$",
            r"(?m)^image_frame: 0 \[BS 70\] @10\.3$",
            r"(?m)^image_quality: 1 \[BS 71\] @11\.5$",
            r"(?m)^units: 3 \[BS 72\] @12\.7$",
            r"(?m)^ padding: 14/0 \(1 bits\)$",
            r"(?m)^ check_CRC 2753-2772 = 19: EADC == EADC$",
        ):
            require(re.search(pattern, block), "missing peer trace field: " + pattern)
        root = [b for b in blocks if "Decode object DICTIONARY\n" in b
                and re.search(r"(?m)^handle: 0\.1\.A \[H 5\]$", b)]
        require(len(root) == 1 and re.search(r'(?m)^\s*texts\[12\]: "ACAD_IMAGE_VARS" \[TV 3\]', root[0])
                and re.search(r"(?m)^itemhandles\[12\]: \(2\.1\.2F\) abs:2F \[H\* 0\]", root[0]),
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
                and b"DWG IMAGEDEF/reactor source capture or immutable payload correlation failed" in result.stdout,
                "expected HEADER/strict refusal was not observed")
    require(digest(source) == SOURCE_SHA256, "source changed during audit")
    print("AC1018 RASTERVARIABLES independent roles and strict refusal PASS")


if __name__ == "__main__":
    main()
