#!/usr/bin/env python3
"""CLI strict-refusal contract for an incomplete WIPEOUTVARIABLES source graph."""

import argparse
from pathlib import Path
import tempfile
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from bounded_process import run_bounded


SENTINEL = b"pre-existing CLI destination\n"


def run(converter, generator):
    with tempfile.TemporaryDirectory(prefix="libdxfrw-wipeout-cli-") as temp:
        root = Path(temp)
        source = root / "wipeoutvariables-ac1027.dwg"
        generated = run_bounded(
            [str(generator), "--create-source", str(source)], 10, 131072,
            cwd=root)
        if generated.returncode != 0 or source.read_bytes()[:6] != b"AC1027":
            raise AssertionError("from-scratch AC1027 DWG generation failed: "
                                 f"{(generated.stdout + generated.stderr)[:2048]!r}")

        destination = root / "destination.dxf"
        for options in ([], ["--allow-partial-dwg-read"]):
            destination.write_bytes(SENTINEL)
            result = run_bounded(
                [str(converter), str(source), "-o", str(destination), "-y",
                 "-d", *options], 12, 131072, cwd=root)
            if result.returncode == 0:
                raise AssertionError("incomplete source graph unexpectedly converted")
            if destination.read_bytes() != SENTINEL:
                raise AssertionError("strict CLI refusal replaced the destination")
            output = result.stdout + result.stderr
            if (b"Conversion failed" not in output
                    or b"source references are not fully qualified" not in output):
                raise AssertionError("missing strict source-reference diagnostic: "
                                     f"{output[:2048]!r}")

        print("WIPEOUTVARIABLES CLI strict refusal: incomplete AC1027 source "
              "stays atomic; partial-read option does not bypass reference gate")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--converter", required=True, type=Path)
    parser.add_argument("--generator", required=True, type=Path)
    arguments = parser.parse_args()
    run(arguments.converter, arguments.generator)


if __name__ == "__main__":
    main()
