#!/usr/bin/env python3
"""Hash-pin and run the opt-in AC1021 inline-SAB TORUS source probe."""

import argparse
import hashlib
from pathlib import Path
import subprocess
import sys


EXPECTED_SHA256 = "0e3eccc70ea6b82120c77449bb1d1d27ee280a1ed1c0f2e88e31b1db503ca66f"


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dwg", type=Path, help="pinned LibreDWG AC1021 source")
    parser.add_argument("output_directory", type=Path)
    parser.add_argument(
        "--probe", type=Path,
        help="probe executable (default: repository build/libdxfrw_dwg_local_roundtrip)",
    )
    args = parser.parse_args()

    source = args.dwg.resolve()
    if not source.is_file():
        parser.error(f"DWG input does not exist: {source}")
    actual = sha256_file(source)
    if actual != EXPECTED_SHA256:
        print(
            f"refusing unpinned source: SHA256={actual}; "
            f"expected {EXPECTED_SHA256}",
            file=sys.stderr,
        )
        return 2

    repository = Path(__file__).resolve().parents[2]
    probe = (args.probe or repository / "build/libdxfrw_dwg_local_roundtrip").resolve()
    if not probe.is_file():
        print(f"probe executable does not exist: {probe}", file=sys.stderr)
        return 2
    args.output_directory.mkdir(parents=True, exist_ok=True)
    print(f"source SHA256 verified: {actual}")
    return subprocess.run(
        [
            str(probe),
            "--check-acsh-torus-source-projection",
            str(source),
            str(args.output_directory.resolve()),
        ],
        check=False,
    ).returncode


if __name__ == "__main__":
    raise SystemExit(main())
