#!/usr/bin/env python3
"""Bounded, fixture-free CLI authorization and process-contract regressions."""

import argparse
import os
from pathlib import Path
import struct
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


def encode_authored_pairs(pairs, binary):
    """Small from-scratch recipe for only the string/double tags used here."""
    result = bytearray(BINARY_PREFIX if binary else b"")
    for code, value in [*pairs, (0, b"EOF")]:
        if binary:
            result += struct.pack("<H", code)
            result += value + b"\0" if isinstance(value, bytes) else struct.pack("<d", value)
        else:
            result += str(code).encode() + b"\n"
            result += (value if isinstance(value, bytes) else repr(value).encode()) + b"\n"
    return bytes(result)


def check_source_text_admission(converter, root):
    """Actual CLI source refusal before publication; no peer/runtime dependency."""
    def header(version, page):
        pairs = [(0, b"SECTION"), (2, b"HEADER")]
        if version is not None:
            pairs += [(9, b"$ACADVER"), (1, version)]
        if page is not None:
            pairs += [(9, b"$DWGCODEPAGE"), (3, page)]
        return pairs + [(0, b"ENDSEC")]

    def text(payload, layer=b"0"):
        return [(0, b"SECTION"), (2, b"ENTITIES"), (0, b"TEXT"),
                (5, b"A0"), (8, layer), (10, 1.0), (20, 2.0), (40, 1.0),
                (1, payload), (0, b"LINE"), (5, b"B0"), (8, layer),
                (10, 34.0), (20, 35.0), (30, 36.0),
                (11, 37.0), (21, 38.0), (31, 39.0), (0, b"ENDSEC")]

    encode = encode_authored_pairs

    negatives = [
        ("alias deletion", b"AC1015", b"ISO8859-4", b"L\xa1R"),
        ("alias wrong scalar", b"AC1015", b"ISO8859-5", b"\xb0"),
        ("unavailable page", b"AC1015", b"CP866", b"\x80"),
        ("unknown page", b"AC1015", b"unknown-local", b"\xe9"),
        ("missing page", b"AC1015", None, b"\xe9"),
        ("undefined byte", b"AC1015", b"ANSI_1252", b"L\x81R"),
        ("truncated DBCS", b"AC1015", b"ANSI_932", b"L\x82"),
        ("overlong UTF8", b"AC1021", b"ANSI_1252", b"\xc0\xaf"),
        ("surrogate UTF8", b"AC1021", b"ANSI_1252", b"\xed\xa0\x80"),
        ("out of range UTF8", b"AC1021", b"ANSI_1252", b"\xf4\x90\x80\x80"),
        ("continuation UTF8", b"AC1021", b"ANSI_1252", b"\x80"),
        ("truncated UTF8", b"AC1021", b"ANSI_1252", b"\xe2\x82"),
        ("unrepresented MIF", b"AC1015", b"ANSI_1252", b"\\M+48861"),
        ("malformed CIF", b"AC1015", b"ANSI_1252", b"\\U+ZZZZ"),
        ("non scalar CIF", b"AC1021", b"ANSI_1252", b"\\U+D800"),
        ("legacy UTF8 profile", b"AC1015", b"UTF8", b"plain"),
        ("legacy UTF16 profile", b"AC1015", b"UTF16", b"AB"),
        ("missing revision", None, None, b"\xc3\xa9"),
        ("unknown revision", b"AC9999", None, b"\xc3\xa9"),
    ]
    rejected = retries = positives = 0
    clean = header(b"AC1021", b"ANSI_1252") + text(b"clean retry")
    for index in range(len(negatives) + 7):
        if index < len(negatives):
            label, version, page, payload = negatives[index]
            pairs = header(version, page) + text(payload)
        elif index == len(negatives):
            label = "later canonical metadata"
            pairs = header(b"AC1015", b"ISO8859-4") + text(b"\xa1") + header(None, b"ANSI_1252")
        elif index == len(negatives) + 1:
            label = "later revision cannot authorize earlier bytes"
            pairs = header(None, None) + text(b"\xc3\xa9") + header(b"AC1021", b"ANSI_1252")
        elif index == len(negatives) + 2:
            label = "valid scalar unsupported earlier UTF16 profile"
            pairs = header(b"AC1015", b"UTF16") + text(b"AB", b"00") + header(None, b"ANSI_1252")
        elif index == len(negatives) + 3:
            label = "full advisory list cannot erase loss"
            pairs = header(b"AC1015", b"ISO8859-4") * 20 + text(b"\xa1")
        elif index == len(negatives) + 4:
            label = "layer name source loss"
            pairs = header(b"AC1015", b"ANSI_1252") + text(b"plain", b"L\x81R")
        elif index == len(negatives) + 5:
            label = "deferred HEADER source loss"
            pairs = [(0, b"SECTION"), (2, b"HEADER"), (9, b"$CLAYER"), (8, b"L\x81R"),
                     (9, b"$ACADVER"), (1, b"AC1015"), (9, b"$DWGCODEPAGE"), (3, b"ANSI_1252"),
                     (0, b"ENDSEC")] + text(b"plain")
        else:
            label = "XDATA source invalidity"
            entities = text(b"plain")
            line_at = entities.index((0, b"LINE"))
            entities[line_at:line_at] = [(1001, b"AUTHORED_APP"), (1000, b"L\x80R")]
            pairs = header(b"AC1021", b"ANSI_1252") + entities
        for binary in (False, True):
            source = root / f"text-source-{index}-{binary}.dxf"
            bad = encode(pairs, binary)
            for freecad in (False, True):
                output = root / f"text-result-{index}-{binary}-{freecad}.dxf"
                args = ([source, "-o", output, "-v2007", "-y", "-b"] if freecad
                        else [source, "-v2007", "-y", output])
                for existing in (False, True):
                    source.write_bytes(bad)
                    if existing:
                        output.write_bytes(SENTINEL)
                    else:
                        output.unlink(missing_ok=True)
                    before = set(root.iterdir())
                    result = run_bounded([str(converter), *map(str, args)], 5, 65536, cwd=root)
                    expected = SENTINEL if existing else None
                    actual = output.read_bytes() if output.exists() else None
                    if (result.returncode != 1 or actual != expected or set(root.iterdir()) != before
                            or b"DXF source-text admission failed:" not in result.stderr
                            or b"Success:" in result.stdout or source.read_bytes() != bad):
                        raise AssertionError(f"{label}: source refusal published/modified or false success: {result!r}")
                    if len(result.stderr) > 1024 or any(b > 127 or b == 0 for b in result.stderr):
                        raise AssertionError(f"{label}: unbounded or unsafe source refusal report")
                    rejected += 1
                source.write_bytes(encode(clean, binary))
                result = run_bounded([str(converter), *map(str, args)], 5, 65536, cwd=root)
                if result.returncode != 0 or output.read_bytes() == SENTINEL:
                    raise AssertionError(f"{label}: corrected same-path retry failed: {result!r}")
                retries += 1

    positive_rows = [
        (b"AC1015", b"unknown-local", b"plain", b"plain"),
        (b"AC1015", b"unknown-local", b"\\U+0410", "А".encode()),
        (b"AC1015", b"ISO8859-4", b"plain", b"plain"),
        (b"AC1015", b"CP866", b"\\M+193FA", "日".encode()),
        (b"AC1015", b"ANSI_1252", b"L\xe9R", "LéR".encode()),
        (b"AC1015", None, b"plain", b"plain"),
        (b"AC1021", b"unknown-local", "é中".encode(), "é中".encode()),
        (b"AC1021", b"UTF16", "é中".encode(), "é中".encode()),
        (None, None, b"plain", b"plain"),
        (b"AC9999", None, b"\\U+0410", "А".encode()),
    ]
    for index, (version, page, payload, expected) in enumerate(positive_rows):
        for binary in (False, True):
            source = root / f"text-positive-{index}-{binary}.dxf"
            source.write_bytes(encode(header(version, page) + text(payload), binary))
            for freecad in (False, True):
                output = root / f"text-positive-result-{index}-{binary}-{freecad}.dxf"
                args = [source, "-o", output, "-v2007"] if freecad else [source, "-v2007", output]
                result = run_bounded([str(converter), *map(str, args)], 5, 65536, cwd=root)
                if result.returncode != 0 or expected not in output.read_bytes() or result.stderr:
                    raise AssertionError(f"source positive {index}: lost scalar/incorrect refusal: {result!r}")
                positives += 1

    bad_source, good_source = root / "text-batch-bad.dxf", root / "text-batch-good.dxf"
    bad_source.write_bytes(encode(header(b"AC1015", b"ISO8859-4") + text(b"\xa1"), False))
    good_source.write_bytes(encode(clean, False))
    listing, output_dir = root / "text-batch-list.txt", root / "text-batch-result"
    output_dir.mkdir()
    listing.write_text(str(bad_source) + "\n" + str(good_source) + "\n", encoding="utf-8")
    destination = output_dir / bad_source.name
    destination.write_bytes(SENTINEL)
    result = run_bounded([str(converter), str(listing), "-B", "-v2007", "-y", str(output_dir)],
                         5, 65536, cwd=root)
    if (result.returncode != 1 or destination.read_bytes() != SENTINEL
            or not (output_dir / good_source.name).is_file()
            or b"Failed:" not in result.stdout or b"Success:" not in result.stdout):
        raise AssertionError(f"source-refused batch must preserve destination and continue: {result!r}")
    return rejected, retries, positives, 1


def check_target_text_transport(converter, root):
    def drawing(kind, payload, binary, layer=b"0"):
        return encode_authored_pairs([
            (0, b"SECTION"), (2, b"HEADER"), (9, b"$ACADVER"), (1, b"AC1021"),
            (9, b"$DWGCODEPAGE"), (3, b"ANSI_1252"), (0, b"ENDSEC"),
            (0, b"SECTION"), (2, b"ENTITIES"), (0, kind), (5, b"A0"), (8, layer),
            (10, 1.0), (20, 2.0), (40, 1.0), (1, payload),
            (0, b"LINE"), (5, b"B0"), (8, b"0"), (10, 34.0), (20, 35.0),
            (30, 36.0), (11, 37.0), (21, 38.0), (31, 39.0), (0, b"ENDSEC")], binary)

    def invoke(source, output, version, freecad, binary=False):
        args = [source, "-o", output, version, "-y"] if freecad else [source, version, "-y", output]
        if binary:
            args.insert(-1 if not freecad else len(args), "-b")
        return run_bounded([str(converter), *map(str, args)], 5, 65536, cwd=root)

    refused = retries = modern = repeated = 0
    emoji = "😀".encode()
    for index, kind in enumerate((b"TEXT", b"MTEXT", b"TEXT")):
        for binary in (False, True):
            source = root / f"target-source-{index}-{binary}.dxf"
            original = drawing(kind, emoji if index != 2 else b"plain", binary,
                               layer=emoji if index == 2 else b"0")
            for freecad in (False, True):
                output = root / f"target-result-{index}-{binary}-{freecad}.dxf"
                source.write_bytes(original)
                for existing in (False, True):
                    if existing:
                        output.write_bytes(SENTINEL)
                    else:
                        output.unlink(missing_ok=True)
                    before = set(root.iterdir())
                    result = invoke(source, output, "-v2004", freecad, binary=freecad)
                    actual = output.read_bytes() if output.exists() else None
                    if (result.returncode != 1 or actual != (SENTINEL if existing else None)
                            or set(root.iterdir()) != before or b"dxf-text-target-loss" not in result.stderr
                            or b"Success:" in result.stdout or source.read_bytes() != original):
                        raise AssertionError(f"target scalar downgrade false success/publication: {result!r}")
                    refused += 1
                source.write_bytes(drawing(kind, "А".encode(), binary))
                result = invoke(source, output, "-v2004", freecad, binary=freecad)
                if result.returncode != 0 or b"\\U+0410" not in output.read_bytes():
                    raise AssertionError(f"BMP CIF target retry failed: {result!r}")
                retries += 1
                source.write_bytes(original)
                result = invoke(source, output, "-v2007", freecad, binary=freecad)
                if result.returncode != 0 or emoji not in output.read_bytes():
                    raise AssertionError(f"modern supplementary scalar was lost: {result!r}")
                modern += 1
    for kind in (b"TEXT", b"MTEXT"):
        for source_binary in (False, True):
            source = root / f"literal-source-{kind.decode()}-{source_binary}.dxf"
            # Genuine source CIF becomes a literal escape-looking semantic
            # backslash. Wire output must keep this distinction on both passes.
            source.write_bytes(drawing(kind, b"A" * 248 + b"\\U+005CU+0041suffix", source_binary))
            for version in ("-v2004", "-v2007"):
                for binary in (False, True):
                    first = root / f"literal-first-{repeated}.dxf"
                    second = root / f"literal-second-{repeated}.dxf"
                    for input_path, output in ((source, first), (first, second)):
                        result = invoke(input_path, output, version, True, binary)
                        if result.returncode != 0 or b"\\U+005CU+0041suffix" not in output.read_bytes():
                            raise AssertionError(f"literal semantic CIF lost on repeated conversion: {result!r}")
                    repeated += 1
    return refused, retries, modern, repeated


def check_exact_input_policy(converter, root):
    """Actual option, profile refusal and repeated-conversion handoff; no peer."""
    exact = "--dxf-codepage-policy=exact-named"
    compatible = "--dxf-codepage-policy=compatible"

    def header(version, page):
        return [(0, b"SECTION"), (2, b"HEADER"), (9, b"$ACADVER"), (1, version),
                (9, b"$DWGCODEPAGE"), (3, page), (0, b"ENDSEC")]

    def body(payload):
        return [(0, b"SECTION"), (2, b"ENTITIES"), (0, b"TEXT"), (5, b"A0"),
                (8, b"0"), (10, 1.0), (20, 2.0), (40, 1.0), (1, payload),
                (0, b"LINE"), (5, b"B0"), (8, b"0"), (10, 34.0), (20, 35.0),
                (30, 36.0), (11, 37.0), (21, 38.0), (31, 39.0), (0, b"ENDSEC")]

    def invoke(source, output, tokens, freecad=True, version="-v2007", binary=False):
        args = [source, "-o", output, version, "-y", *tokens] if freecad else [source, version, "-y", *tokens, output]
        if binary:
            args.insert(len(args) if freecad else -1, "-b")
        return run_bounded([str(converter), *map(str, args)], 5, 65536, cwd=root)

    def verify_ascii(path, payload, revision):
        data = path.read_bytes()
        if data.startswith(BINARY_PREFIX):
            raise AssertionError("profile projection expected actual ASCII output")
        lines = data.splitlines()
        if len(lines) % 2:
            raise AssertionError("profile ASCII output lost a physical code/value pair")
        pairs = [(int(lines[i].strip()), lines[i+1]) for i in range(0, len(lines), 2)]
        if not any(a == (9, b"$ACADVER") and b == (1, revision) for a, b in zip(pairs, pairs[1:])):
            raise AssertionError("profile actual output has the wrong source/target revision")
        entities = []
        for code, value in pairs:
            if code == 0:
                entities.append([value, []])
            elif entities:
                entities[-1][1].append((code, value))
        texts = [tags for name, tags in entities if name == b"TEXT"]
        geometry = [tags for name, tags in entities if name == b"LINE"]
        if len(texts) != 1 or [v for c, v in texts[0] if c == 1] != [payload] or len(geometry) != 1:
            raise AssertionError("profile output changed text bytes/type or entity coverage")
        coordinates = {c: float(v) for c, v in geometry[0] if c in (10, 20, 30, 11, 21, 31)}
        if coordinates != {10: 34.0, 20: 35.0, 30: 36.0, 11: 37.0, 21: 38.0, 31: 39.0}:
            raise AssertionError("profile output lost its distinct nonzero-Z LINE")

    option_rejections = 0
    destination = root / "policy bad option destination.dxf"
    missing_source = root / "missing policy input.dxf"
    bad_groups = [[token] for token in ("--dxf-codepage-policy", "--dxf-codepage-policy=",
                  "--dxf-codepage-policy=EXACT-NAMED", "--dxf-codepage-policy=unknown",
                  "--dxf-codepage-policy=exact-named-extra", "--dxf-codepage-policy=compatible-extra",
                  "--dxf-codepage-policy =exact-named")]
    bad_groups += [[a, b] for a in (exact, compatible) for b in (exact, compatible)]
    for tokens in bad_groups:
        for freecad in (False, True):
            destination.write_bytes(SENTINEL)
            before = set(root.iterdir())
            result = invoke(missing_source, destination, tokens, freecad)
            if (result.returncode != 1 or b"Bad options." not in result.stdout
                    or b"Error can't open" in result.stdout or destination.read_bytes() != SENTINEL
                    or set(root.iterdir()) != before):
                raise AssertionError(f"profile invalid option did not fail before I/O: {result!r}")
            option_rejections += 1

    routes = 0
    for source_binary in (False, True):
        source = root / f"policy canonical {source_binary}.dxf"
        original = encode_authored_pairs(header(b"AC1015", b"ANSI_1252") + body(b"L\xe9Z"), source_binary)
        source.write_bytes(original)
        for policy in (compatible, exact):
            for freecad in (False, True):
                for version, revision, value in (("-v2004", b"AC1018", b"L\xe9Z"), ("-v2007", b"AC1021", "LéZ".encode())):
                    for binary in (False, True):
                        first = root / f"policy first {routes}.dxf"
                        second = root / f"policy second {routes}.dxf"
                        result = invoke(source, first, [policy], freecad, version, binary)
                        if result.returncode != 0 or source.read_bytes() != original:
                            raise AssertionError(f"profile first conversion failed/changed source: {result!r}")
                        if binary:
                            if not first.read_bytes().startswith(BINARY_PREFIX):
                                raise AssertionError("profile first output ignored -b")
                        else:
                            verify_ascii(first, value, revision)
                        first_payload = first.read_bytes()
                        result = invoke(first, second, [], True, version)
                        if result.returncode != 0 or first.read_bytes() != first_payload:
                            raise AssertionError(f"default input second profile conversion failed: {result!r}")
                        verify_ascii(second, value, revision)
                        routes += 1

    # Modern metadata and represented ASCII text do not request an ISO byte
    # decoder. These remain distinct from the raw-ISO refusal cells below.
    independent = 0
    for modern in (False, True):
        for source_binary in (False, True):
            source = root / f"policy independent {modern}-{source_binary}.dxf"
            payload = "LĄZ".encode() if modern else b"L\\U+0104Z"
            original = encode_authored_pairs(header(b"AC1021" if modern else b"AC1015", b"ISO8859-4") + body(payload), source_binary)
            source.write_bytes(original)
            for freecad in (False, True):
                output = root / f"policy independent result {independent}.dxf"
                result = invoke(source, output, [exact], freecad)
                if result.returncode != 0 or source.read_bytes() != original:
                    raise AssertionError(f"decoder-independent/modern profile lost eligibility: {result!r}")
                verify_ascii(output, "LĄZ".encode(), b"AC1021")
                independent += 1

    for source_binary in (False, True):
        source = root / f"policy canonical {source_binary}.dxf"
        output = root / f"policy default source revision {source_binary}.dxf"
        before = source.read_bytes()
        result = run_bounded([str(converter), str(source), "-o", str(output), exact], 5, 65536, cwd=root)
        if result.returncode != 0 or source.read_bytes() != before:
            raise AssertionError(f"exact option changed FreeCAD source-revision default: {result!r}")
        verify_ascii(output, b"L\xe9Z", b"AC1015")
        independent += 1

    refusals = retries = 0
    for version, page, payload, diagnostic in (
            (b"AC1015", b"ISO8859-4", b"L\xa1Z", b"unsupported-source-profile issueMask=0"),
            (b"AC1015", b"EUC-JP", b"L\xa1Z", b"observed-source-issues"),
            (b"AC1021", b"ANSI_1252", b"\xc0\xaf", b"observed-source-issues")):
        for source_binary in (False, True):
            for freecad in (False, True):
                source = root / f"policy refused source {refusals}.dxf"
                output = root / f"policy refused output {refusals}.dxf"
                original = encode_authored_pairs(header(version, page) + body(payload), source_binary)
                source.write_bytes(original)
                for existing in (False, True):
                    if existing:
                        output.write_bytes(SENTINEL)
                    before = set(root.iterdir())
                    result = invoke(source, output, [exact], freecad)
                    if (result.returncode != 1 or diagnostic not in result.stderr
                            or (output.read_bytes() if output.exists() else None) != (SENTINEL if existing else None)
                            or set(root.iterdir()) != before or source.read_bytes() != original
                            or b"Success:" in result.stdout):
                        raise AssertionError(f"profile refusal failed policy/atomicity/source contract: {result!r}")
                    refusals += 1
                corrected = encode_authored_pairs(header(b"AC1015", b"ANSI_1252") + body(b"L\xe9Z"), source_binary)
                source.write_bytes(corrected)
                result = invoke(source, output, [exact], freecad)
                if result.returncode != 0 or source.read_bytes() != corrected:
                    raise AssertionError(f"profile corrected retry failed: {result!r}")
                verify_ascii(output, "LéZ".encode(), b"AC1021")
                retries += 1

    output_dir = root / "policy batch output"
    output_dir.mkdir()
    bad = root / "policy batch bad.dxf"
    good = root / "policy batch good.dxf"
    bad_bytes = encode_authored_pairs(header(b"AC1015", b"ISO8859-4") + body(b"\xa1"), False)
    good_bytes = encode_authored_pairs(header(b"AC1015", b"ANSI_1252") + body(b"L\xe9Z"), False)
    bad.write_bytes(bad_bytes); good.write_bytes(good_bytes)
    listing = root / "policy batch.txt"
    list_bytes = (str(bad) + "\n" + str(good) + "\n").encode()
    listing.write_bytes(list_bytes)
    (output_dir / bad.name).write_bytes(SENTINEL)
    result = run_bounded([str(converter), str(listing), "-B", "-v2007", "-y", exact, str(output_dir)], 5, 65536, cwd=root)
    if (result.returncode != 1 or b"Failed:" not in result.stdout or b"Success:" not in result.stdout
            or (output_dir / bad.name).read_bytes() != SENTINEL or bad.read_bytes() != bad_bytes
            or good.read_bytes() != good_bytes or listing.read_bytes() != list_bytes):
        raise AssertionError(f"profile batch did not preserve failure/continue independent item: {result!r}")
    verify_ascii(output_dir / good.name, "LéZ".encode(), b"AC1021")
    return option_rejections, routes, independent, refusals, retries, 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--converter", required=True, type=Path)
    parser.add_argument("--target-text-only", action="store_true",
                        help="run only the focused target transport controls while iterating")
    parser.add_argument("--exact-input-only", action="store_true",
                        help="run only explicit input policy/profile controls while iterating")
    args = parser.parse_args()
    converter = args.converter.resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix="libdxfrw-cli-contract-") as directory:
        root = Path(directory)
        if args.exact_input_only:
            policy = check_exact_input_policy(converter, root)
            print(f"CLI exact input: PASS ({policy[0]} option refusals/{policy[1]} two-pass routes/"
                  f"{policy[2]} independent positives/{policy[3]} atomic source refusals/"
                  f"{policy[4]} corrected retries/{policy[5]} mixed batch)")
            return
        if args.target_text_only:
            target = check_target_text_transport(converter, root)
            print(f"CLI target transport: PASS ({target[0]} refusals/{target[1]} retries/"
                  f"{target[2]} modern positives/{target[3]} two-pass literal routes)")
            return
        checked = check_options(converter, root)
        batch_checked = check_batch(converter, root)
        observation_checked = check_read_observations(converter, root)
        source_checked = check_source_text_admission(converter, root)
        target = check_target_text_transport(converter, root)
        policy = check_exact_input_policy(converter, root)
    print(f"CLI contract: PASS ({checked} option/filename and "
          f"{batch_checked} batch controls, {observation_checked} read-observation controls; "
          f"source text {source_checked[0]} refusals/{source_checked[1]} retries/"
          f"{source_checked[2]} positives/{source_checked[3]} mixed batch; target text "
          f"{target[0]} refusals/{target[1]} retries/{target[2]} modern/{target[3]} two-pass routes; "
          f"exact input {policy[0]} option refusals/{policy[1]} two-pass/{policy[2]} independent/"
          f"{policy[3]} atomic refusals/{policy[4]} retries/{policy[5]} mixed batch, bounded subprocesses)")


if __name__ == "__main__":
    main()
