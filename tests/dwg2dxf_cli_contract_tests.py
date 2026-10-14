#!/usr/bin/env python3
"""Bounded, fixture-free CLI authorization and process-contract regressions."""

import argparse
import os
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


def check_batch(converter, root):
    source_dir = root / "batch inputs"
    source_dir.mkdir()
    source = source_dir / " leading é trailing .dxf"
    source.write_bytes(SOURCE)
    list_path = root / "batch list.txt"
    checked = 0

    def invoke(payload, label, expected, inputs=(source,), overwrite=True,
               existing=False):
        nonlocal checked
        output_dir = root / ("batch result " + label)
        output_dir.mkdir()
        if existing:
            (output_dir / source.name).write_bytes(SENTINEL)
        list_path.write_bytes(payload)
        before = {path: path.read_bytes() for path in inputs}
        args = [str(converter), str(list_path), "-B", "-v2007"]
        if overwrite:
            args.append("-y")
        args.append(str(output_dir))
        result = run_bounded(args, 5, 131072, cwd=root)
        if result.returncode != expected:
            raise AssertionError(f"batch {label}: exit {result.returncode}, "
                                 f"expected {expected}: {result.stdout[:2048]!r} "
                                 f"{result.stderr[:2048]!r}")
        if list_path.read_bytes() != payload:
            raise AssertionError(f"batch {label}: changed list file")
        if any(path.read_bytes() != value for path, value in before.items()):
            raise AssertionError(f"batch {label}: changed source")
        checked += 1
        return output_dir, result

    for label, ending in (("LF", b"\n"), ("CRLF", b"\r\n"), ("no newline", b"")):
        output, result = invoke(str(source).encode("utf-8") + ending, label, 0)
        if not (output / source.name).is_file() or b"Success:" not in result.stdout:
            raise AssertionError(f"batch {label}: lost input whitespace/UTF-8 or outcome")

    missing = source_dir / "missing.dxf"
    mixed = (str(missing) + "\n" + str(source) + "\n").encode("utf-8")
    output, result = invoke(mixed, "mixed missing first", 1)
    if not (output / source.name).is_file() or b"Failed:" not in result.stdout:
        raise AssertionError("mixed batch did not continue/report independent item")
    output, _ = invoke((str(missing) + "\n").encode(), "missing only", 1)
    if list(output.iterdir()):
        raise AssertionError("missing-only batch published output")
    output, result = invoke(str(source).encode() + b"\n", "existing no overwrite", 1,
                            overwrite=False, existing=True)
    if (output / source.name).read_bytes() != SENTINEL or b"refusing to overwrite" not in result.stdout:
        raise AssertionError("batch prompted or modified an unauthorized destination")
    for label, payload in (("empty", b""), ("blank", b"\n\r\n"),
                           ("NUL", str(source).encode() + b"\x00\n"),
                           ("long line", b"x" * (128 * 1024 + 1) + b"\n"),
                           ("many items", (str(source).encode() + b"\n") * 10001)):
        output, _ = invoke(payload, label, 2)
        if list(output.iterdir()):
            raise AssertionError(f"batch {label}: published before list admission")

    output, _ = invoke(str(source).encode() + b"\n" + b"x" * (128 * 1024 + 1),
                       "valid prefix then oversized", 2)
    if list(output.iterdir()):
        raise AssertionError("oversized later line published an earlier item")

    for label, path, expected in (("missing list", root / "absent list", 2),
                                   ("directory list", source_dir, 2)):
        output = root / ("batch result " + label)
        output.mkdir()
        result = run_bounded([str(converter), str(path), "-B", "-v2007", "-y",
                              str(output)], 5, 131072, cwd=root)
        if result.returncode != expected or list(output.iterdir()):
            raise AssertionError(f"batch {label}: wrong I/O outcome")
        checked += 1

    if hasattr(os, "mkfifo"):
        fifo = source_dir / "pipe.dxf"
        os.mkfifo(fifo)
        output, _ = invoke((str(fifo) + "\n" + str(source) + "\n").encode(),
                           "FIFO input", 1)
        if not (output / source.name).is_file() or (output / fifo.name).exists():
            raise AssertionError("special input blocked/lost a later independent item")
        output = root / "batch result FIFO list"
        output.mkdir()
        result = run_bounded([str(converter), str(fifo), "-B", "-v2007", "-y",
                              str(output)], 5, 131072, cwd=root)
        if result.returncode != 2 or list(output.iterdir()):
            raise AssertionError("special list input was opened as a stream")
        checked += 1

    output = root / "missing batch version"
    output.mkdir()
    list_path.write_text(str(source) + "\n", encoding="utf-8")
    result = run_bounded([str(converter), str(list_path), "-B", "-y", str(output)],
                          5, 131072, cwd=root)
    if result.returncode != 1 or b"Bad options." not in result.stdout or list(output.iterdir()):
        raise AssertionError("batch did not retain explicit version requirement")
    checked += 1

    output = root / "batch source alias"
    output.mkdir()
    alias_source = output / "source.dxf"
    alias_source.write_bytes(SOURCE)
    list_path.write_text(str(alias_source) + "\n", encoding="utf-8")
    result = run_bounded([str(converter), str(list_path), "-B", "-v2007", "-y",
                          str(output)], 5, 131072, cwd=root)
    if result.returncode != 2 or alias_source.read_bytes() != SOURCE:
        raise AssertionError("batch -y authorized an input alias")
    checked += 1
    return checked


def check_read_observations(converter, root):
    checked = 0
    for label, payload, version in (("short modern header", b"AC1027\n", b"AC1027"),
                                    ("invalid magic", b"BAD000\n", b"unknown")):
        source = root / (label + ".dwg")
        output = root / (label + ".dxf")
        source.write_bytes(payload)
        output.write_bytes(SENTINEL)
        result = run_bounded([str(converter), str(source), "-o", str(output),
                              "-v2010", "-y"], 5, 65536, cwd=root)
        if result.returncode != 1 or source.read_bytes() != payload or output.read_bytes() != SENTINEL:
            raise AssertionError(f"{label}: failed DWG import changed Boolean/atomic output contract")
        required = (b"DWG read observations: schema=1 operationSucceeded=false",
                    b"sourceVersion=" + version, b"frameSourceDenominator=unknown",
                    b"uniqueSkippedRecordTotal=unknown", b"reportedUpstreamDroppedLowerBound=0")
        if any(value not in result.stderr for value in required):
            raise AssertionError(f"{label}: missing bounded read observations: {result.stderr!r}")
        if len(result.stderr) > 32768 or any(byte > 127 or byte == 0 for byte in result.stderr):
            raise AssertionError(f"{label}: observation report escaped its ASCII/byte contract")
        checked += 1
    return checked


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--converter", required=True, type=Path)
    args = parser.parse_args()
    converter = args.converter.resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix="libdxfrw-cli-contract-") as directory:
        root = Path(directory)
        checked = check_options(converter, root)
        batch_checked = check_batch(converter, root)
        observation_checked = check_read_observations(converter, root)
    print(f"CLI contract: PASS ({checked} option/filename and "
          f"{batch_checked} batch controls, {observation_checked} read-observation controls, bounded subprocesses)")


if __name__ == "__main__":
    main()
