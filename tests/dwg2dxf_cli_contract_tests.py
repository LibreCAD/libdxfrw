#!/usr/bin/env python3
"""Bounded, fixture-free CLI authorization and process-contract regressions."""

import argparse
from pathlib import Path
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from bounded_process import run_bounded


SOURCE = (b"0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
          b"0\nSECTION\n2\nENTITIES\n0\nLINE\n5\nA0\n8\n0\n"
          b"10\n1\n20\n2\n30\n3\n11\n5\n21\n8\n31\n13\n"
          b"0\nENDSEC\n0\nEOF\n")
BINARY_PREFIX = b"AutoCAD Binary DXF\r\n\x1a\x00"
SENTINEL = b"CLI overwrite-authority sentinel\n"


def check_options(converter, root):
    source = root / "source drawing.dxf"
    source.write_bytes(SOURCE)
    destination = root / "existing result.dxf"
    checked = 0

    def invoke(arguments):
        return run_bounded([str(converter), *map(str, arguments)], 5, 131072,
                           cwd=root)

    def reject(label, arguments, paths):
        nonlocal checked
        before = {path: path.read_bytes() if path.exists() else None for path in paths}
        result = invoke(arguments)
        if result.returncode != 1 or b"Bad options." not in result.stdout:
            raise AssertionError(f"{label}: expected checked option rejection, "
                                 f"got {result.returncode}: {result.stdout[:2048]!r} "
                                 f"{result.stderr[:2048]!r}")
        for path, payload in before.items():
            after = path.read_bytes() if path.exists() else None
            if after != payload:
                raise AssertionError(f"{label}: invalid options modified {path}")
        checked += 1

    # The invalid token itself must never confer overwrite authorization.
    for token in ("-yUnexpected", "-bad", "-Bextra", "-dextra", "-Dextra",
                  "-v2007suffix", "--", "", "-", "-unknown"):
        destination.write_bytes(SENTINEL)
        reject(f"legacy invalid {token!r}",
               [source, "-v2007", token, destination], [source, destination])
        reject(f"legacy invalid after -y {token!r}",
               [source, "-y", "-v2007", token, destination], [source, destination])
        reject(f"FreeCAD invalid {token!r}",
               [source, "-o", destination, "-y", token], [source, destination])

    reject("conflicting legacy versions", [source, "-v2000", "-v2007", destination],
           [source, destination])
    reject("duplicate legacy version", [source, "-v2007", "-v2007", destination],
           [source, destination])
    reject("conflicting FreeCAD versions", [source, "-o", destination, "-v2000", "-v2007"],
           [source, destination])
    reject("missing FreeCAD output", [source, "-o"], [source, destination])
    reject("empty positional output", [source, "-v2007", ""], [source, destination])
    reject("empty FreeCAD output", [source, "-o", ""], [source, destination])

    for token in ("-y", "-Y", "-d", "-D", "-b"):
        destination.write_bytes(SENTINEL)
        extra_overwrite = [] if token in ("-y", "-Y") else ["-y"]
        arguments = [source, "-v2007", *extra_overwrite, token, destination]
        result = invoke(arguments)
        if result.returncode != 0:
            raise AssertionError(f"documented legacy {token}: {result.stdout[:2048]!r} "
                                 f"{result.stderr[:2048]!r}")
        payload = destination.read_bytes()
        if token == "-b":
            if not payload.startswith(BINARY_PREFIX):
                raise AssertionError("exact -b did not produce binary DXF")
        elif b"AC1021" not in payload[:2048] or payload.startswith(BINARY_PREFIX):
            raise AssertionError(f"documented legacy {token}: wrong output revision/kind")
        checked += 1

    for token, version in (("-R12", "AC1009"), ("-v2000", "AC1015"),
                           ("-v2004", "AC1018"), ("-v2007", "AC1021"),
                           ("-v2010", "AC1024")):
        output = root / (version + " exact version.dxf")
        result = invoke([source, token, output])
        if result.returncode != 0 or version.encode() not in output.read_bytes()[:2048]:
            raise AssertionError(f"accepted version {token}: wrong output or failure")
        checked += 1

    # Leading dashes remain legitimate in unambiguous positional filenames.
    dash_source = root / "-source.dxf"
    dash_source.write_bytes(SOURCE)
    result = invoke([dash_source.name, "-v2007", "-result.dxf"])
    if result.returncode != 0 or not (root / "-result.dxf").is_file():
        raise AssertionError("unambiguous positional dash filename was rejected")
    checked += 1

    freecad = root / "four argument FreeCAD result.dxf"
    result = invoke([source, "-o", freecad])
    if result.returncode != 0 or b"AC1021" not in freecad.read_bytes()[:2048]:
        raise AssertionError("four-argument FreeCAD command changed")
    checked += 1
    if source.read_bytes() != SOURCE or dash_source.read_bytes() != SOURCE:
        raise AssertionError("CLI checks changed their locally generated input")
    return checked


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--converter", required=True, type=Path)
    args = parser.parse_args()
    converter = args.converter.resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix="libdxfrw-cli-contract-") as directory:
        checked = check_options(converter, Path(directory))
    print(f"CLI contract: PASS ({checked} bounded option/filename controls)")


if __name__ == "__main__":
    main()
