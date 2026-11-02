#!/usr/bin/env python3
"""Bounded, fixture-free CLI authorization and process-contract regressions."""

import argparse
import os
from pathlib import Path
import struct
import sys
import tempfile
import time

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from bounded_process import run_bounded


SOURCE = (b"0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
          b"0\nSECTION\n2\nENTITIES\n0\nLINE\n5\nA0\n8\n0\n"
          b"10\n1\n20\n2\n30\n3\n11\n5\n21\n8\n31\n13\n"
          b"0\nENDSEC\n0\nEOF\n")
BINARY_PREFIX = b"AutoCAD Binary DXF\r\n\x1a\x00"
SENTINEL = b"CLI overwrite-authority sentinel\n"
PRE_R13_MARKERS = (
    (5, b"unsupported-r11-repeat"),
    (6, b"unsupported-r11-endrep"),
    (10, b"unsupported-r11-load"),
)


def make_pre_r13_marker_dwg(magic, marker_type):
    """Build a minimal AC1006/AC1009 drawing entirely from local bytes."""
    entity_start = 0x600
    header_vars = 158 if magic == b"AC1006" else 205
    header = bytearray(entity_start)
    header[:6] = magic
    struct.pack_into("<H", header, 0x11, header_vars)
    struct.pack_into("<I", header, 0x14, entity_start)
    marker = struct.pack("<BBHHH", marker_type, 0, 8, 0, 0)
    # This supported block/end-block pair is a following-record sentinel. If
    # the marker is silently skipped, the reader can continue to a callback.
    block = struct.pack("<BBHHHdd", 12, 0, 24, 0, 0, 4.0, 9.0)
    endblock = struct.pack("<BBHHH", 13, 0, 8, 0, 0)
    records = marker + block + endblock
    struct.pack_into("<I", header, 0x18, entity_start + len(records))
    return bytes(header) + records


def check_pre_r13_marker_failure(converter, root):
    checked = 0
    for magic in (b"AC1006", b"AC1009"):
        for marker_type, diagnostic in PRE_R13_MARKERS:
            label = f"{magic.decode()} marker {marker_type}"
            source = root / f"{label}.dwg"
            destination = root / f"{label}.dxf"
            source_bytes = make_pre_r13_marker_dwg(magic, marker_type)
            source.write_bytes(source_bytes)
            if magic == b"AC1009" and marker_type == 10:
                destination.write_bytes(SENTINEL)
            before = destination.read_bytes() if destination.exists() else None
            result = run_bounded(
                [str(converter), str(source), "-y", "-v2010", str(destination)],
                5, 131072, cwd=root)
            output = result.stdout + result.stderr
            after = destination.read_bytes() if destination.exists() else None
            if (result.returncode == 0 or diagnostic not in output
                    or before != after or source.read_bytes() != source_bytes):
                raise AssertionError(
                    f"{label}: incomplete read was published or not identified; "
                    f"exit={result.returncode}, diagnostic={diagnostic!r}, "
                    f"output={output[:2048]!r}, destination-preserved={before == after}")
            checked += 1
    return checked


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
                  "-v2007suffix", "-v20130", "-v2018suffix", "-V2018",
                  "--allow-partial-dwg-read-extra",
                  "--", "", "-", "-unknown"):
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

    # SOURCE intentionally has HEADER and ENTITIES but no TABLES section. A
    # successful consumer import must be exportable and remain importable after
    # the adapter synthesizes its canonical layer table. Author both files in
    # this temporary directory; this is a compatibility control, not a fixture.
    canonical = root / "no tables canonical.dxf"
    result = invoke([source, "-y", "-v2010", canonical])
    if result.returncode != 0:
        raise AssertionError("no-TABLES DXF import was not exportable: "
                             f"{result.stdout[:2048]!r} {result.stderr[:2048]!r}")
    if b"\nLINE\n" not in canonical.read_bytes():
        raise AssertionError("no-TABLES DXF export dropped its entity")
    canonical_readback = root / "no tables canonical readback.dxf"
    result = invoke([canonical, "-y", "-v2010", canonical_readback])
    if result.returncode != 0 or b"\nLINE\n" not in canonical_readback.read_bytes():
        raise AssertionError("canonical no-TABLES DXF could not be read and exported: "
                             f"{result.stdout[:2048]!r} {result.stderr[:2048]!r}")
    checked += 2

    for token, version in (("-R12", "AC1009"), ("-v2000", "AC1015"),
                           ("-v2004", "AC1018"), ("-v2007", "AC1021"),
                           ("-v2010", "AC1024"), ("-v2013", "AC1027"),
                           ("-v2018", "AC1032")):
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

    partial_positional = root / "partial option positional.dxf"
    result = invoke([source, "--allow-partial-dwg-read", "-v2007",
                     partial_positional])
    if result.returncode != 0 or b"AC1021" not in partial_positional.read_bytes()[:2048]:
        raise AssertionError("exact partial-read option was not accepted positionally")
    checked += 1
    partial_freecad = root / "partial option FreeCAD.dxf"
    result = invoke([source, "-o", partial_freecad, "--allow-partial-dwg-read"])
    if result.returncode != 0 or b"AC1021" not in partial_freecad.read_bytes()[:2048]:
        raise AssertionError("exact partial-read option changed the -o route")
    checked += 1
    protected = root / "partial option cannot overwrite.dxf"
    protected.write_bytes(SENTINEL)
    result = invoke([source, "-o", protected, "--allow-partial-dwg-read"])
    if result.returncode != 1 or protected.read_bytes() != SENTINEL:
        raise AssertionError("partial-read option unexpectedly authorized overwrite")
    checked += 1
    if source.read_bytes() != SOURCE or dash_source.read_bytes() != SOURCE:
        raise AssertionError("CLI checks changed their locally generated input")
    return checked


def check_modern_versions(converter, root):
    """Explicit targets share policy across positional, -o, and batch routes."""
    routes = batches = refused = retries = 0
    sources = []

    def invoke(arguments):
        return run_bounded([str(converter), *map(str, arguments)], 5, 131072, cwd=root)

    def verify(path, revision, binary):
        payload = path.read_bytes()
        if payload.startswith(BINARY_PREFIX) != binary:
            raise AssertionError("modern selector changed the requested DXF encoding")
        if binary:
            if b"\x09\x00$ACADVER\x00\x01\x00" + revision + b"\x00" not in payload[:2048]:
                raise AssertionError("modern binary header did not use the requested revision")
            return
        lines = payload.splitlines()
        if len(lines) % 2:
            raise AssertionError("modern ASCII output lost a physical group/value pair")
        pairs = [(int(lines[i].strip()), lines[i+1]) for i in range(0, len(lines), 2)]
        if sum(a == (9, b"$ACADVER") and b == (1, revision) for a, b in zip(pairs, pairs[1:])) != 1:
            raise AssertionError("modern ASCII header did not use the requested revision")
        records = []
        in_entities = False
        for index, (code, value) in enumerate(pairs):
            if (code, value) == (0, b"SECTION"):
                in_entities = pairs[index+1] == (2, b"ENTITIES")
            elif (code, value) == (0, b"ENDSEC"):
                in_entities = False
            elif in_entities:
                if code == 0:
                    records.append((value, []))
                elif records:
                    records[-1][1].append((code, value))
        if len(records) != 1 or records[0][0] != b"LINE":
            raise AssertionError("modern selector changed the entity set")
        coordinates = [(c, float(v)) for c, v in records[0][1] if c in (10, 20, 30, 11, 21, 31)]
        if dict(coordinates) != {10: 1., 20: 2., 30: 3., 11: 5., 21: 8., 31: 13.} or len(coordinates) != 6:
            raise AssertionError("modern target/readback changed the distinct nonzero-Z LINE")

    def accepted(arguments, output, revision, binary):
        result = invoke(arguments)
        if result.returncode != 0:
            raise AssertionError(f"modern target conversion failed: {result!r}")
        verify(output, revision, binary)

    for source_revision in (b"AC1021", b"AC1027", b"AC1032"):
        source = root / f"modern source {source_revision.decode()} é.dxf"
        original = SOURCE.replace(b"AC1021", source_revision, 1)
        source.write_bytes(original)
        sources.append((source, original))
        for token, revision in (("-v2013", b"AC1027"), ("-v2018", b"AC1032")):
            for freecad in (False, True):
                for binary in (False, True):
                    first = root / f"modern first {routes}.dxf"
                    second = root / f"modern second {routes}.dxf"
                    normalized = root / f"modern normalized {routes}.dxf"
                    arguments = ([source, "-o", first, token, "-y"] if freecad
                                 else [source, token, "-y", first])
                    if binary:
                        arguments.insert(len(arguments) if freecad else -1, "-b")
                    accepted(arguments, first, revision, binary)
                    before = first.read_bytes()
                    accepted([first, "-o", second, token, "-y", *([] if binary else ["-b"])],
                             second, revision, not binary)
                    # Actually decode whichever pass was binary, then inspect
                    # its complete ASCII geometry; do not infer it from magic.
                    accepted([first if binary else second, "-o", normalized, token],
                             normalized, revision, False)
                    if first.read_bytes() != before or source.read_bytes() != original:
                        raise AssertionError("modern conversion changed a source drawing")
                    routes += 1

    batch_list = root / "modern batch list.txt"
    listing = b"\r\n".join(str(source).encode() for source, _ in sources) + b"\r\n"
    batch_list.write_bytes(listing)
    for token, revision in (("-v2013", b"AC1027"), ("-v2018", b"AC1032")):
        for binary in (False, True):
            directory = root / f"modern batch {batches}"
            directory.mkdir()
            result = invoke([batch_list, "-B", token, "-y", *(["-b"] if binary else []), directory])
            if result.returncode != 0 or batch_list.read_bytes() != listing:
                raise AssertionError(f"modern batch target failed: {result!r}")
            for source, original in sources:
                output = directory / source.name
                verify(output, revision, binary)
                if binary:
                    normalized = root / f"modern batch readback {batches}-{source.stem}.dxf"
                    accepted([output, "-o", normalized, token], normalized, revision, False)
                if source.read_bytes() != original:
                    raise AssertionError("modern batch changed a source")
            batches += 1

    destination = root / "modern protected.dxf"
    missing_source = root / "missing modern option source.dxf"
    for versions in (("-v2013", "-v2013"), ("-v2018", "-v2018"),
                     ("-v2013", "-v2018"), ("-v2018", "-v2013")):
        for freecad in (False, True):
            destination.write_bytes(SENTINEL)
            arguments = ([missing_source, "-o", destination, "-y", *versions] if freecad
                         else [missing_source, "-y", *versions, destination])
            result = invoke(arguments)
            if (result.returncode != 1 or b"Bad options." not in result.stdout
                    or b"Error can't open" in result.stdout or destination.read_bytes() != SENTINEL):
                raise AssertionError("duplicate/conflicting modern versions reached I/O or changed output")
            refused += 1
    for token, revision in (("-v2013", b"AC1027"), ("-v2018", b"AC1032")):
        for binary in (False, True):
            destination.write_bytes(SENTINEL)
            arguments = [sources[0][0], "-o", destination, token, *(["-b"] if binary else [])]
            result = invoke(arguments)
            if result.returncode != 1 or destination.read_bytes() != SENTINEL:
                raise AssertionError("modern version/encoding granted overwrite authority")
            refused += 1
            accepted([*arguments, "-y"], destination, revision, binary)
            retries += 1
    return routes, batches, refused, retries


def check_batch(converter, root, writer):
    source_dir = root / "batch inputs"
    source_dir.mkdir()
    source = source_dir / " leading é trailing .dxf"
    source.write_bytes(SOURCE)

    def output_name(path):
        return Path(path).with_suffix(".dxf").name

    list_path = root / "batch list.txt"
    checked = 0

    def invoke(payload, label, expected, inputs=(source,), overwrite=True,
               existing=False):
        nonlocal checked
        output_dir = root / ("batch result " + label)
        output_dir.mkdir()
        if existing:
            (output_dir / output_name(source)).write_bytes(SENTINEL)
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
        if not (output / output_name(source)).is_file() or b"Success:" not in result.stdout:
            raise AssertionError(f"batch {label}: lost input whitespace/UTF-8 or outcome")

    missing = source_dir / "missing.dxf"
    mixed = (str(missing) + "\n" + str(source) + "\n").encode("utf-8")
    output, result = invoke(mixed, "mixed missing first", 1)
    if not (output / output_name(source)).is_file() or b"Failed:" not in result.stdout:
        raise AssertionError("mixed batch did not continue/report independent item")
    output, _ = invoke((str(missing) + "\n").encode(), "missing only", 1)
    if list(output.iterdir()):
        raise AssertionError("missing-only batch published output")
    output, result = invoke(str(source).encode() + b"\n", "existing no overwrite", 1,
                            overwrite=False, existing=True)
    if (output / output_name(source)).read_bytes() != SENTINEL or b"refusing to overwrite" not in result.stdout:
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
        if not (output / output_name(source)).is_file() or (output / output_name(fifo)).exists():
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

    # Existing output names that alias a source through filesystem identity
    # must be rejected even when their lexical paths are distinct.
    for alias_kind, create_alias in (("symlink", getattr(os, "symlink", None)),
                                     ("hardlink", getattr(os, "link", None))):
        if create_alias is None:
            continue
        source_alias = source_dir / ("batch " + alias_kind + " source.dwg")
        source_alias.write_bytes(SOURCE)
        alias_output = root / ("batch " + alias_kind + " output")
        alias_output.mkdir()
        alias_destination = alias_output / source_alias.with_suffix(".dxf").name
        try:
            create_alias(source_alias, alias_destination)
        except (AttributeError, NotImplementedError, OSError):
            # Some Windows configurations do not grant unprivileged symlink
            # creation; only count a control when the filesystem created it.
            continue
        list_path.write_text(str(source_alias) + "\n", encoding="utf-8")
        result = run_bounded([str(converter), str(list_path), "-B", "-v2007", "-y",
                              str(alias_output)], 5, 131072, cwd=root)
        if (result.returncode != 2 or not os.path.samefile(source_alias, alias_destination)
                or alias_destination.read_bytes() != SOURCE
                or source_alias.read_bytes() != SOURCE
                or list(alias_output.iterdir()) != [alias_destination]):
            raise AssertionError(f"batch {alias_kind} alias was not rejected unchanged: {result!r}")
        checked += 1

    # Future output names are admitted by native filesystem lookup in a
    # verified same-collation namespace, not by a portable ASCII/Unicode fold.
    # Use a valid locally generated DWG so this filename test does not depend
    # on unrelated dangling XDICTIONARY metadata in a tracked reader sample.
    if writer is None:
        raise AssertionError("batch DWG control requires the local DWG writer")
    dwg_source = root / "batch source.dwg"
    generated = run_bounded(
        [str(writer), "--generate-cli-batch-source", str(dwg_source)],
        10, 131072, cwd=root)
    if generated.returncode != 0 or not dwg_source.is_file():
        raise AssertionError(f"could not author CLI batch DWG control locally: {generated!r}")
    case_dwg_source = dwg_source

    def aliases_in_directory(directory, first_name, second_name):
        first = directory / first_name
        second = directory / second_name
        first.mkdir()
        aliases = second.exists() and os.path.samefile(first, second)
        first.rmdir()
        return aliases

    for label, first_stem, second_stem in (
            ("case collision", "CaseCollision", "casecollision"),
            ("Unicode normalization collision", "caf\u00e9", "cafe\u0301")):
        first_source_dir = root / ("batch " + label + " source A")
        second_source_dir = root / ("batch " + label + " source B")
        first_source_dir.mkdir()
        second_source_dir.mkdir()
        first_source = first_source_dir / (first_stem + ".dwg")
        second_source = second_source_dir / (second_stem + ".dxf")
        first_source.write_bytes(case_dwg_source.read_bytes())
        second_source.write_bytes(SOURCE)

        output = root / ("batch " + label + " output")
        output.mkdir()
        first_output_name = first_stem + ".dxf"
        second_output_name = second_stem + ".dxf"
        aliases = aliases_in_directory(output, first_output_name, second_output_name)
        list_path.write_text(str(first_source) + "\n" + str(second_source) + "\n",
                             encoding="utf-8")
        result = run_bounded([str(converter), str(list_path), "-B", "-v2007", "-y",
                              str(output)], 5, 131072, cwd=root)
        if aliases:
            if (result.returncode != 2 or b"colliding output names" not in result.stdout
                    or list(output.iterdir())):
                raise AssertionError(f"batch {label}: native collision was not rejected: {result!r}")
        elif (result.returncode != 0
              or not (output / first_output_name).is_file()
              or not (output / second_output_name).is_file()
              or set(output.iterdir()) != {output / first_output_name,
                                           output / second_output_name}
              or os.path.samefile(output / first_output_name, output / second_output_name)):
            raise AssertionError(f"batch {label}: native distinct names failed: {result!r}")
        checked += 1

    relative_output = root / "batch relative output"
    relative_output.mkdir()
    list_path.write_text(str(source) + "\n", encoding="utf-8")
    result = run_bounded([str(converter), str(list_path), "-B", "-v2007", "-y",
                          relative_output.name], 5, 131072, cwd=root)
    if (result.returncode != 0
            or not (relative_output / output_name(source)).is_file()
            or set(relative_output.iterdir()) != {relative_output / output_name(source)}):
        raise AssertionError(f"batch relative output directory failed: {result!r}")
    checked += 1

    # Reuse that generated source to verify suffix dispatch and readback.
    list_path.write_text(str(dwg_source) + "\n", encoding="utf-8")
    output = root / "batch DWG suffix"
    output.mkdir()
    result = run_bounded([str(converter), str(list_path), "-B", "-v2007", "-y",
                          str(output)], 5, 131072, cwd=root)
    dwg_output = output / dwg_source.with_suffix(".dxf").name
    dwg_payload = dwg_output.read_bytes() if dwg_output.is_file() else b""
    if result.returncode != 0 or not dwg_payload or dwg_payload.startswith(BINARY_PREFIX):
        raise AssertionError(f"batch DWG did not publish ASCII DXF with .dxf suffix: {result!r}")
    checked += 1
    readback = output / "batch source readback.dxf"
    result = run_bounded([str(converter), str(dwg_output), "-v2007", str(readback)],
                         5, 131072, cwd=root)
    if result.returncode != 0 or not readback.is_file():
        raise AssertionError(f"batch DWG output could not be reopened by suffix-dispatched CLI: {result!r}")
    checked += 1

    # Same-stem DWG/DXF inputs map to one output; reject in both orders before
    # converting either source or publishing any destination.
    colliding_dxf = source_dir / dwg_source.with_suffix(".dxf").name
    colliding_dxf.write_bytes(SOURCE)
    for label, paths in (("DWG first", (dwg_source, colliding_dxf)),
                         ("DXF first", (colliding_dxf, dwg_source))):
        output = root / ("batch collision " + label)
        output.mkdir()
        list_path.write_text("".join(str(path) + "\n" for path in paths), encoding="utf-8")
        result = run_bounded([str(converter), str(list_path), "-B", "-v2007", "-y",
                              str(output)], 5, 131072, cwd=root)
        if result.returncode != 2 or list(output.iterdir()):
            raise AssertionError(f"batch {label}: colliding stems were not rejected preflight")
        checked += 1
    return checked


def check_batch_name_limit(converter, root):
    output = root / "batch name-limit output"
    output.mkdir()
    listing = root / "batch name-limit list.txt"
    missing = root / "batch name-limit missing inputs"
    lines = [str(missing / f"missing-{index:05d}.dxf") for index in range(10000)]
    payload = ("\n".join(lines) + "\n").encode("utf-8")
    listing.write_bytes(payload)
    started = time.monotonic()
    result = run_bounded([str(converter), str(listing), "-B", "-v2007", "-y",
                          str(output)], 20, 4 * 1024 * 1024, cwd=root)
    elapsed = time.monotonic() - started
    if (result.returncode != 1 or b"Failed:" not in result.stdout
            or listing.read_bytes() != payload or list(output.iterdir())):
        raise AssertionError(f"10,000-name preflight control failed: {result!r}")
    return elapsed


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


def check_unsupported_dwg_codepage(converter, root):
    """Mutate only the envelope codepage of an admitted local sample."""
    fixture = (Path(__file__).resolve().parents[1] / "tests" / "fixtures" / "dwg"
               / "ordinary_enc_AC1021.dwg")
    payload = bytearray(fixture.read_bytes())
    if len(payload) < 21 or payload[:6] != b"AC1021":
        raise AssertionError("eligible AC1021 repository fixture is unavailable")
    original_id = struct.unpack_from("<H", payload, 19)[0]
    if original_id in (0, 0xFF, 999):
        raise AssertionError(f"unexpected repository fixture codepage id {original_id}")
    struct.pack_into("<H", payload, 19, 999)
    source = root / "unsupported-codepage.dwg"
    output = root / "unsupported-codepage.dxf"
    source.write_bytes(payload)
    result = run_bounded(
        [str(converter), str(source), "-y", "-v2010", str(output)],
        20, 2 * 1024 * 1024, cwd=root)
    report = result.stderr
    required = (b"DWG read observations: schema=1 operationSucceeded=true",
                b"sourceCodePage: fieldPresent=true id=999 codecRecognized=false",
                b"fallback=version-selected textMayBeMisdecoded=true")
    if (any(value not in report for value in required)
            or b"Error reading file" in result.stdout
            or source.read_bytes() != payload):
        raise AssertionError(
            "unsupported DWG codepage was not surfaced after a successful read: "
            f"exit={result.returncode}, stdout={result.stdout[:1024]!r}, "
            f"stderr={report[:2048]!r}")
    return 1


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


def check_embedded_mtext_downlevel(converter, root):
    """R2018 ATTRIB MTEXT must not silently downgrade through the CLI."""
    pairs = [
        (0, b"SECTION"), (2, b"HEADER"), (9, b"$ACADVER"), (1, b"AC1032"),
        (9, b"$DWGCODEPAGE"), (3, b"ANSI_1252"), (0, b"ENDSEC"),
        (0, b"SECTION"), (2, b"BLOCKS"),
        (0, b"BLOCK"), (5, b"3000"), (100, b"AcDbEntity"), (8, b"0"),
        (100, b"AcDbBlockBegin"), (2, b"MTEXT_BLOCK"), (70, b"0"),
        (10, 0.0), (20, 0.0), (30, 0.0), (3, b"MTEXT_BLOCK"),
        (0, b"ENDBLK"), (5, b"3001"), (100, b"AcDbEntity"), (8, b"0"),
        (100, b"AcDbBlockEnd"), (0, b"ENDSEC"),
        (0, b"SECTION"), (2, b"ENTITIES"),
        (0, b"INSERT"), (5, b"3002"), (100, b"AcDbEntity"), (8, b"0"),
        (100, b"AcDbBlockReference"), (66, b"1"), (2, b"MTEXT_BLOCK"),
        (10, 0.0), (20, 0.0), (30, 0.0), (41, 1.0), (42, 1.0), (43, 1.0),
    ]

    def source_pairs(embedded):
        result = list(pairs)
        result += [
            (0, b"ATTRIB"), (5, b"4000"), (100, b"AcDbEntity"), (8, b"0"),
            (100, b"AcDbText"), (10, 4.0), (20, 5.0), (30, 6.0), (40, 2.0),
            (1, b"fallback attribute"),
        ]
        if embedded:
            result += [
                (101, b"Embedded Object"), (10, 1.0), (20, 2.0), (30, 3.0),
                (40, 2.0), (41, 10.0), (46, 200.0), (71, b"1"), (72, b"1"),
                (1, b"multiline\\Pattribute"),
            ]
        result += [
            (100, b"AcDbAttribute"), (2, b"MTEXT_TAG"), (70, b"0"),
            (0, b"SEQEND"), (5, b"4001"), (100, b"AcDbEntity"), (8, b"0"),
            (0, b"LINE"), (5, b"5000"), (100, b"AcDbEntity"), (8, b"0"),
            (100, b"AcDbLine"), (10, 71.0), (20, 72.0), (30, 73.0),
            (11, 74.0), (21, 75.0), (31, 76.0), (0, b"ENDSEC"),
        ]
        return result

    source = root / "r2018 embedded mtext source.dxf"
    original = encode_authored_pairs(source_pairs(True), False)
    plain = encode_authored_pairs(source_pairs(False), False)
    source.write_bytes(original)
    checked = 0
    for binary in (False, True):
        for freecad in (False, True):
            output = root / f"downlevel embedded result {binary}-{freecad}.dxf"
            sentinel = SENTINEL if binary else b"pre-existing output sentinel\n"
            for existing in (False, True):
                output.write_bytes(sentinel) if existing else output.unlink(missing_ok=True)
                before = set(root.iterdir())
                args = ([source, "-o", output, "-v2007", "-y"] if freecad
                        else [source, "-v2007", "-y", output])
                if binary:
                    args.insert(len(args) if freecad else len(args) - 1, "-b")
                result = run_bounded([str(converter), *map(str, args)], 5, 65536,
                                     cwd=root)
                actual = output.read_bytes() if output.exists() else None
                if (result.returncode != 1 or b"Conversion failed" not in result.stdout
                        or b"Error reading file" in result.stdout
                        or b"multiline attribute (not in this DXF version)" not in result.stderr
                        or actual != (sentinel if existing else None)
                        or set(root.iterdir()) != before or source.read_bytes() != original):
                    raise AssertionError(
                        f"embedded MTEXT downlevel published through CLI: {result!r}")
                checked += 1

            # Same source route without the embedded object remains supported;
            # this distinguishes the targeted refusal from a broken ATTRIB path.
            source.write_bytes(plain)
            output.unlink(missing_ok=True)
            args = ([source, "-o", output, "-v2007", "-y"] if freecad
                    else [source, "-v2007", "-y", output])
            if binary:
                args.insert(len(args) if freecad else len(args) - 1, "-b")
            result = run_bounded([str(converter), *map(str, args)], 5, 65536,
                                 cwd=root)
            payload = output.read_bytes() if output.exists() else b""
            valid_encoding = payload.startswith(BINARY_PREFIX) if binary else (
                not payload.startswith(BINARY_PREFIX) and b"AC1021" in payload[:2048])
            if (result.returncode != 0 or b"Success:" not in result.stdout
                    or not valid_encoding or b"ATTRIB" not in payload
                    or source.read_bytes() != plain):
                raise AssertionError(f"plain ATTRIB CLI retry control failed: {result!r}")
            checked += 1
            source.write_bytes(original)
    return checked


def check_proxy_graphics_loss(converter, root):
    """Strict CLI export must not publish when a typed entity cache is lost."""
    proxy = (bytes(8) + struct.pack("<II", 64, 2)
             + struct.pack("<7d", 2.0, 3.0, 4.0, 5.0, 0.0, 0.0, 1.0))
    if len(proxy) != 72:
        raise AssertionError("source-only proxy circle recipe must be exactly 72 bytes")
    pairs = [
        (0, b"SECTION"), (2, b"HEADER"), (9, b"$ACADVER"), (1, b"AC1024"),
        (0, b"ENDSEC"), (0, b"SECTION"), (2, b"ENTITIES"),
        (0, b"LINE"), (5, b"A0"), (8, b"0"), (100, b"AcDbEntity"),
        (100, b"AcDbLine"), (10, 10.0), (20, 20.0), (30, 0.0),
        (11, 15.0), (21, 25.0), (31, 0.0), (92, 72), (310, proxy.hex().upper().encode()),
        (0, b"ENDSEC"),
    ]
    source = root / "proxy graphics source.dxf"
    original = encode_authored_pairs(pairs, False)
    source.write_bytes(original)

    checked = 0
    for freecad in (False, True):
        for binary in (False, True):
            for existing in (False, True):
                output = root / f"proxy output {freecad}-{binary}-{existing}.dxf"
                if existing:
                    output.write_bytes(SENTINEL)
                arguments = [str(converter), str(source)]
                if freecad:
                    arguments += ["-o", str(output)]
                    arguments += ["-v2010", "-y"]
                else:
                    arguments += ["-v2010", "-y"]
                if binary:
                    arguments.append("-b")
                if not freecad:
                    arguments.append(str(output))
                before = set(root.iterdir())
                result = run_bounded(arguments, 5, 65536, cwd=root)
                expected = SENTINEL if existing else None
                if (result.returncode != 1
                        or b"typed-proxy-graphics-not-preserved" not in result.stderr
                        or b"Success:" in result.stdout
                        or (output.read_bytes() if output.exists() else None) != expected
                        or set(root.iterdir()) != before
                        or source.read_bytes() != original):
                    raise AssertionError(
                        f"strict proxy loss was not reported/published atomically "
                        f"(FreeCAD={freecad}, binary={binary}, existing={existing}): "
                        f"{result!r}")
                checked += 1
    return checked


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
    parser.add_argument("--writer", type=Path,
                        help="local test executable that authors generated DWG controls")
    parser.add_argument("--fast-only", action="store_true",
                        help="run bounded option, batch, and read-observation smoke controls")
    parser.add_argument("--options-only", action="store_true",
                        help="run only fast option-token and overwrite-authority controls")
    parser.add_argument("--modern-versions-only", action="store_true",
                        help="run explicit R2013/R2018 CLI route/readback controls")
    parser.add_argument("--target-text-only", action="store_true",
                        help="run only the focused target transport controls while iterating")
    parser.add_argument("--exact-input-only", action="store_true",
                        help="run only explicit input policy/profile controls while iterating")
    parser.add_argument("--batch-only", action="store_true",
                        help="run only bounded batch conversion controls while iterating")
    parser.add_argument("--batch-name-limit-only", action="store_true",
                        help="run only the bounded 10,000-name preflight performance control")
    parser.add_argument("--embedded-mtext-only", action="store_true",
                        help="run only embedded-MTEXT downlevel CLI refusal controls")
    parser.add_argument("--proxy-graphics-only", action="store_true",
                        help="run only strict typed proxy-graphics loss CLI controls")
    parser.add_argument("--pre-r13-marker-only", action="store_true",
                        help="run only generated AC1006/AC1009 marker publication refusals")
    parser.add_argument("--unsupported-dwg-codepage-only", action="store_true",
                        help="run only the unsupported DWG envelope-codepage diagnostic control")
    args = parser.parse_args()
    converter = args.converter.resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix="libdxfrw-cli-contract-") as directory:
        root = Path(directory)
        if args.options_only:
            checked = check_options(converter, root)
            print(f"CLI option contract: PASS ({checked} token/filename/overwrite controls)")
            return
        if args.modern_versions_only:
            modern = check_modern_versions(converter, root)
            print(f"CLI modern versions: PASS ({modern[0]} two-pass routes/{modern[1]} batches/"
                  f"{modern[2]} atomic option/overwrite refusals/{modern[3]} corrected retries)")
            return
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
        if args.batch_only:
            batch_checked = check_batch(converter, root, args.writer)
            print(f"CLI batch: PASS ({batch_checked} bounded batch controls)")
            return
        if args.batch_name_limit_only:
            elapsed = check_batch_name_limit(converter, root)
            print(f"CLI batch name limit: PASS (10000 planned names; {elapsed:.3f}s)")
            return
        if args.embedded_mtext_only:
            checked = check_embedded_mtext_downlevel(converter, root)
            print(f"CLI embedded-MTEXT downlevel: PASS ({checked} refusal/retry controls)")
            return
        if args.proxy_graphics_only:
            checked = check_proxy_graphics_loss(converter, root)
            print(f"CLI proxy graphics loss: PASS ({checked} atomic refusal controls)")
            return
        if args.pre_r13_marker_only:
            checked = check_pre_r13_marker_failure(converter, root)
            print(f"CLI pre-R13 markers: PASS ({checked} atomic refusal controls)")
            return
        if args.unsupported_dwg_codepage_only:
            checked = check_unsupported_dwg_codepage(converter, root)
            print(f"CLI unsupported DWG codepage: PASS ({checked} diagnostic control)")
            return
        if args.fast_only:
            checked = check_options(converter, root)
            batch_checked = check_batch(converter, root, args.writer)
            observation_checked = check_read_observations(converter, root)
            print(f"CLI fast contract: PASS ({checked} option controls, "
                  f"{batch_checked} batch controls, {observation_checked} "
                  "read-observation controls)")
            return
        checked = check_options(converter, root)
        batch_checked = check_batch(converter, root, args.writer)
        observation_checked = check_read_observations(converter, root)
        source_checked = check_source_text_admission(converter, root)
        target = check_target_text_transport(converter, root)
        embedded_mtext = check_embedded_mtext_downlevel(converter, root)
        proxy_graphics = check_proxy_graphics_loss(converter, root)
        pre_r13_markers = check_pre_r13_marker_failure(converter, root)
        policy = check_exact_input_policy(converter, root)
    print(f"CLI contract: PASS ({checked} option/filename and "
          f"{batch_checked} batch controls, {observation_checked} read-observation controls; "
          f"source text {source_checked[0]} refusals/{source_checked[1]} retries/"
          f"{source_checked[2]} positives/{source_checked[3]} mixed batch; target text "
          f"{target[0]} refusals/{target[1]} retries/{target[2]} modern/{target[3]} two-pass routes; "
          f"embedded-MTEXT downlevel {embedded_mtext} refusal/retry controls; "
          f"proxy graphics {proxy_graphics} atomic refusals; "
          f"pre-R13 marker {pre_r13_markers} atomic refusals; "
          f"exact input {policy[0]} option refusals/{policy[1]} two-pass/{policy[2]} independent/"
          f"{policy[3]} atomic refusals/{policy[4]} retries/{policy[5]} mixed batch, bounded subprocesses)")


if __name__ == "__main__":
    main()
