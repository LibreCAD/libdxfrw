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
from audit_sunstudy_dxf_graph import author as author_sunstudy, inspect as inspect_sunstudy


SOURCE = (b"0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n"
          b"0\nSECTION\n2\nENTITIES\n0\nLINE\n5\nA0\n8\n0\n"
          b"10\n1\n20\n2\n30\n3\n11\n5\n21\n8\n31\n13\n"
          b"0\nENDSEC\n0\nEOF\n")
BINARY_PREFIX = b"AutoCAD Binary DXF\r\n\x1a\x00"
SENTINEL = b"CLI overwrite-authority sentinel\n"


def check_sunstudy_observation_cli(converter, root):
    """Locally authored graph: explicit observation, strict output, atomic refusal."""
    source = root / "sun-source.dxf"
    author_sunstudy(source)
    source_bytes = source.read_bytes()
    destination = root / "sun-output.dxf"
    flag = "--observe-whole-dxf-source"

    def convert(args, should_succeed):
        result = run_bounded([str(converter), *map(str, args)], 8, 131072, cwd=root)
        if (result.returncode == 0) != should_succeed:
            raise AssertionError(f"SUNSTUDY CLI exit={result.returncode}: "
                                 f"{(result.stdout + result.stderr)[:2048]!r}")

    # No observation and an unknown similarly spelled switch must not grant it.
    for options in ([], [flag + "-unexpected"]):
        destination.write_bytes(SENTINEL)
        convert([source, "-o", destination, "-y", *options], False)
        if destination.read_bytes() != SENTINEL:
            raise AssertionError("unqualified SUNSTUDY output replaced destination")

    # The eligible source works in each encoding. The adapter selector checks
    # exact graph readback in both; here inspect ASCII and Binary identity bytes.
    for binary in (False, True):
        options = ["-b"] if binary else []
        convert([source, "-o", destination, "-y", flag, *options], True)
        if destination.read_bytes().startswith(BINARY_PREFIX) != binary:
            raise AssertionError("incorrect SUNSTUDY DXF encoding")
        if binary:
            payload = destination.read_bytes()
            if any(value not in payload for value in
                   (b"SUNSTUDY\x00", b"MY_SUNSTUDIES\x00", b"Morning\x00")):
                raise AssertionError("Binary SUNSTUDY output lost selected identities")
        else:
            inspect_sunstudy(destination)
        second = root / "sun-second-pass.dxf"
        convert([destination, "-o", second, "-y", flag], True)
        inspect_sunstudy(second)
        third = root / "sun-third-pass.dxf"
        convert([second, "-o", third, "-y", flag, *options], True)
        if third.read_bytes().startswith(BINARY_PREFIX) != binary:
            raise AssertionError("third-pass SUNSTUDY encoding mismatch")
        if not binary:
            inspect_sunstudy(third)
        if not binary:
            repeated_source = destination.read_bytes()
            edge = b"  3\nACAD_GROUP\n350\nD\n"
            group = b"  5\nD\n330\nC\n100\nAcDbDictionary\n281\n    1\n"
            if repeated_source.count(edge) != 1 or repeated_source.count(group) != 1:
                raise AssertionError("expected exact generated GROUP source profile")
            mutations = (
                (edge, b"  3\nACAD_GROUP\n360\nD\n"),
                (edge, b"  3\nACAD_GROUP\n350\nF0\n"),
                (group, group.replace(b"330\nC\n", b"330\n0\n")),
                (group, group + b"300\nunmodeled\n"),
                (group, group + b"  3\nEXTRA\n350\nF0\n"),
            )
            for index, (old, new) in enumerate(mutations):
                bad = root / f"sun-repeat-bad-{index}.dxf"
                bad.write_bytes(repeated_source.replace(old, new, 1))
                second.write_bytes(SENTINEL)
                convert([bad, "-o", second, "-y", flag], False)
                if second.read_bytes() != SENTINEL:
                    raise AssertionError("invalid repeated GROUP source replaced destination")
                convert([destination, "-o", second, "-y", flag], True)
                inspect_sunstudy(second)

    # Unknown CLASS fields and wrong target revision still fail closed even
    # with observation, preserving the existing output; corrected retry works.
    altered = root / "sun-altered.dxf"
    marker = b" 91\n1\n280\n0\n"
    if source_bytes.count(marker) != 1:
        raise AssertionError("expected unique SUNSTUDY CLASS marker")
    altered.write_bytes(source_bytes.replace(marker, b" 91\n1\n300\nunknown\n280\n0\n"))
    altered_dictionary = root / "sun-altered-dictionary.dxf"
    dictionary_marker = b"100\nAcDbDictionary\n280\n0\n"
    if source_bytes.count(dictionary_marker) != 2:
        raise AssertionError("expected two canonical dictionaries")
    altered_dictionary.write_bytes(source_bytes.replace(
        dictionary_marker, b"100\nAcDbDictionary\n300\nunknown\n280\n0\n", 1))
    for bad_source, extra in ((altered, []), (altered_dictionary, []),
                              (source, ["-v2010"])):
        destination.write_bytes(SENTINEL)
        convert([bad_source, "-o", destination, "-y", flag, *extra], False)
        if destination.read_bytes() != SENTINEL:
            raise AssertionError("strict SUNSTUDY refusal replaced destination")
        convert([source, "-o", destination, "-y", flag], True)
        inspect_sunstudy(destination)

    # Traditional syntax and batch propagation must also opt in explicitly.
    traditional = root / "sun-traditional.dxf"
    convert([source, "-y", "-v2013", flag, traditional], True)
    inspect_sunstudy(traditional)
    batch = root / "sun-batch"
    batch.mkdir()
    listing = root / "sun-list.txt"
    listing.write_text(str(source) + "\n", encoding="utf-8")
    (batch / source.name).write_bytes(SENTINEL)
    convert([listing, "-B", "-y", "-v2013", batch], False)
    if (batch / source.name).read_bytes() != SENTINEL:
        raise AssertionError("batch without observation replaced destination")
    convert([listing, "-B", "-y", "-v2013", flag, batch], True)
    inspect_sunstudy(batch / source.name)
    # Other witnessed source revisions are separate same-revision profiles.
    for revision in ("AC1024", "AC1032"):
        versioned_source = root / f"sun-{revision}.dxf"
        author_sunstudy(versioned_source, revision)
        for binary in (False, True):
            versioned_output = root / f"sun-{revision}-output.dxf"
            convert([versioned_source, "-o", versioned_output, "-y", flag,
                     *(["-b"] if binary else [])], True)
            if not output_has_revision(versioned_output.read_bytes(), revision.encode(), binary):
                raise AssertionError("wrong SUNSTUDY output revision or encoding")
            versioned_readback = root / f"sun-{revision}-readback.dxf"
            convert([versioned_output, "-o", versioned_readback, "-y", flag], True)
            inspect_sunstudy(versioned_readback)
            versioned_output.write_bytes(SENTINEL)
            convert([versioned_source, "-o", versioned_output, "-y", flag, "-v2013"], False)
            if versioned_output.read_bytes() != SENTINEL:
                raise AssertionError("cross-revision SUNSTUDY output replaced destination")
            convert([versioned_source, "-o", versioned_output, "-y", flag], True)
            inspect_sunstudy(versioned_output)
    if source.read_bytes() != source_bytes:
        raise AssertionError("source was modified")
    return 42
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


def output_has_revision(payload, revision, binary):
    if payload.startswith(BINARY_PREFIX) != binary:
        return False
    if binary:
        # R12 Binary DXF stores group codes 0..254 in one byte. Later DXF
        # revisions store group codes as little-endian 16-bit values.
        code_width = 1 if revision == b"AC1009" else 2
        code_9 = b"\x09" if code_width == 1 else b"\x09\x00"
        code_1 = b"\x01" if code_width == 1 else b"\x01\x00"
        field = code_9 + b"$ACADVER\x00" + code_1 + revision + b"\x00"
        return payload.count(field) == 1
    lines = [line.strip() for line in payload.splitlines()]
    return sum(
        lines[index:index + 4] == [b"9", b"$ACADVER", b"1", revision]
        for index in range(max(0, len(lines) - 3))) == 1


def check_default_output_versions(converter, root, invoke):
    """Pin FreeCAD -o default-version selection and output encoding."""
    profiles = (
        (b"AC1009", b"AC1009"),
        (b"AC1012", b"AC1014"),
        (b"AC1014", b"AC1014"),
        (b"AC1015", b"AC1015"),
        (b"AC1018", b"AC1018"),
        (b"AC1021", b"AC1021"),
        (b"AC1024", b"AC1024"),
        (b"AC1027", b"AC1027"),
        (b"AC1032", b"AC1032"),
    )
    checked = 0
    for source_revision, output_revision in profiles:
        source = root / f"default-version-source-{source_revision.decode()}.dxf"
        original = SOURCE.replace(b"AC1021", source_revision, 1)
        source.write_bytes(original)
        for binary in (False, True):
            output = root / (
                f"default-version-output-{source_revision.decode()}-"
                f"{'binary' if binary else 'ascii'}.dxf")
            arguments = [source, "-o", output]
            inspect_projection = source_revision == b"AC1012" and not binary
            if inspect_projection:
                arguments.append("-d")
            if binary:
                arguments.append("-b")
            result = invoke(arguments)
            if result.returncode != 0 or source.read_bytes() != original:
                raise AssertionError(
                    f"FreeCAD default output version failed for "
                    f"{source_revision!r}/{binary}: {result!r}")
            payload = output.read_bytes()
            if not output_has_revision(payload, output_revision, binary):
                raise AssertionError(
                    f"FreeCAD default output for {source_revision!r}/{binary} "
                    f"did not encode exactly {output_revision!r}")
            if inspect_projection:
                required_projection = (
                    b"CLI conversion projection: schema=1 sourceFormat=DXF "
                    b"sourceVersion=AC1012 targetVersion=AC1014 "
                    b"targetSelection=source-default outputEncoding=ASCII "
                    b"readCompleteness=not-applicable stage=before-output-preflight")
                if required_projection not in result.stderr:
                    raise AssertionError(
                        "AC1012 DXF default target projection is incomplete: "
                        f"{result.stderr[-2048:]!r}")
            checked += 1
    unsupported_source = root / "unsupported default source.dxf"
    unsupported_bytes = SOURCE.replace(b"AC1021", b"AC1006", 1)
    unsupported_source.write_bytes(unsupported_bytes)
    unsupported_output = root / "unsupported default result.dxf"
    result = invoke([unsupported_source, "-o", unsupported_output, "-d"])
    unsupported_projection = (
        b"CLI conversion projection: schema=1 sourceFormat=DXF "
        b"sourceVersion=AC1006 targetVersion=unknown "
        b"targetSelection=source-default-unavailable outputEncoding=ASCII "
        b"readCompleteness=not-applicable stage=before-output-preflight")
    if (result.returncode != 1 or unsupported_source.read_bytes() != unsupported_bytes
            or unsupported_output.exists()
            or b"no supported default DXF version" not in result.stdout
            or unsupported_projection not in result.stderr):
        raise AssertionError(
            "unsupported implicit target did not report its preflight projection "
            f"and refuse publication: {result!r}")
    checked += 1
    return checked


def check_default_dwg_output_versions(converter, root, writer,
                                      ac1021_source):
    """Pin DWG-derived FreeCAD -o defaults for the generated source profiles."""
    sources = [(b"AC1021", ac1021_source)]
    for revision in (b"AC1015", b"AC1018", b"AC1024", b"AC1027", b"AC1032"):
        source = root / f"default-version-source-{revision.decode()}.dwg"
        generated = run_bounded(
            [str(writer), "--generate-cli-batch-source", str(source),
             revision.decode()], 10, 131072, cwd=root)
        if generated.returncode != 0 or not source.is_file():
            raise AssertionError(
                f"could not author {revision.decode()} default-version source: "
                f"{generated!r}")
        sources.append((revision, source))

    checked = 0
    for revision, source in sources:
        original = source.read_bytes()
        if original[:6] != revision:
            raise AssertionError(f"generated DWG source has wrong revision: {source}")
        if revision == b"AC1018":
            for binary in (False, True):
                output = root / f"ac1018-explicit-ac1024-{'binary' if binary else 'ascii'}.dxf"
                output.write_bytes(SENTINEL)
                args = [str(converter), str(source), "-o", str(output), "-v2010", "-y", "-d"]
                if binary:
                    args.append("-b")
                result = run_bounded(args, 10, 2 * 1024 * 1024, cwd=root)
                payload = output.read_bytes()
                projection = (b"CLI conversion projection: schema=1 sourceFormat=DWG "
                    b"sourceVersion=AC1018 targetVersion=AC1024 targetSelection=explicit outputEncoding="
                    + (b"Binary" if binary else b"ASCII")
                    + b" readCompleteness=strict stage=before-output-preflight")
                if result.returncode != 0 or source.read_bytes() != original or not output_has_revision(payload, b"AC1024", binary) or projection not in result.stderr:
                    raise AssertionError(f"AC1018 explicit AC1024 export failed: {result!r}")
                readback = root / f"ac1018-explicit-readback-{'binary' if binary else 'ascii'}.dxf"
                result = run_bounded([str(converter), str(output), "-o", str(readback)], 10, 2 * 1024 * 1024, cwd=root)
                if result.returncode != 0 or not readback.is_file() or not output_has_revision(readback.read_bytes(), b"AC1024", False):
                    raise AssertionError(f"AC1018 explicit AC1024 readback failed: {result!r}")
                checked += 1

        for binary in (False, True):
            output = root / (
                f"default-version-dwg-output-{revision.decode()}-"
                f"{'binary' if binary else 'ascii'}.dxf")
            arguments = [str(converter), str(source), "-o", str(output)]
            # AC1027 import currently has an explicit strict-output guard
            # until source owner/member roles and entity references are fully
            # qualified. Keep this generated profile as an atomic refusal
            # control; do not make the CLI contract imply AC1027 support.
            ac1027_strict_refusal = revision == b"AC1027"
            inspect_projection = (revision == b"AC1018"
                                  or (revision == b"AC1021" and not binary)
                                  or ac1027_strict_refusal)
            if inspect_projection:
                arguments.append("-d")
            if binary:
                arguments.append("-b")
            result = run_bounded(arguments, 10, 2 * 1024 * 1024, cwd=root)
            payload = output.read_bytes() if output.is_file() else b""
            if ac1027_strict_refusal:
                required_projection = (
                    b"CLI conversion projection: schema=1 sourceFormat=DWG "
                    b"sourceVersion=AC1027 targetVersion=AC1027 "
                    b"targetSelection=source-default outputEncoding="
                    + (b"Binary" if binary else b"ASCII")
                    + b" readCompleteness=strict stage=before-output-preflight")
                if (result.returncode != 1 or source.read_bytes() != original
                        or output.exists()
                        or b"AC1027 DWG source references are not fully qualified"
                        not in result.stderr
                        or required_projection not in result.stderr):
                    raise AssertionError(
                        "AC1027 default export did not preserve the source and "
                        "atomically refuse its unqualified reference graph: "
                        f"{result!r}")
                checked += 1
                continue
            if (result.returncode != 0 or source.read_bytes() != original
                    or not output_has_revision(payload, revision, binary)):
                raise AssertionError(
                    f"DWG-derived default target failed for "
                    f"{revision.decode()}/{binary}: {result!r}")
            if inspect_projection:
                required_projection = (
                    b"CLI conversion projection: schema=1 sourceFormat=DWG "
                    b"sourceVersion=" + revision + b" targetVersion=" + revision
                    + b" targetSelection=source-default outputEncoding="
                    + (b"Binary" if binary else b"ASCII")
                    + b" readCompleteness=strict stage=before-output-preflight")
                if required_projection not in result.stderr:
                    raise AssertionError(
                        "successful default DWG conversion omitted its target "
                        f"projection: {result.stderr[-2048:]!r}")
            checked += 1

            if binary:
                readback = root / f"default-version-dwg-readback-{revision.decode()}.dxf"
                readback_result = run_bounded(
                    [str(converter), str(output), "-o", str(readback)],
                    10, 2 * 1024 * 1024, cwd=root)
                readback_payload = readback.read_bytes() if readback.is_file() else b""
                if (readback_result.returncode != 0
                        or not output_has_revision(readback_payload, revision, False)
                        or output.read_bytes() != payload):
                    raise AssertionError(
                        f"Binary DWG-derived default output did not read back "
                        f"as {revision.decode()}: {readback_result!r}")
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

    aliases = [("same path", source)]
    for label, create_alias in (("symlink", getattr(os, "symlink", None)),
                                ("hardlink", getattr(os, "link", None))):
        if create_alias is None:
            continue
        alias = root / ("source " + label + ".dxf")
        try:
            create_alias(source, alias)
        except (AttributeError, NotImplementedError, OSError):
            # Some Windows configurations do not grant unprivileged symlink
            # creation; count only aliases the filesystem actually created.
            continue
        aliases.append((label, alias))

    for alias_label, alias_path in aliases:
        routes = (
            ("positional", [source, "-y", "-v2010", alias_path]),
            ("FreeCAD -o", [source, "-o", alias_path, "-v2010", "-y"]),
            ("positional without overwrite", [source, "-v2010", alias_path]),
        )
        for route, arguments in routes:
            source.write_bytes(SOURCE)
            if alias_path != source and alias_path.read_bytes() != SOURCE:
                raise AssertionError(f"{alias_label}: alias did not resolve to source bytes")
            result = invoke(arguments)
            if (result.returncode != 1 or b"output aliases input" not in result.stdout
                    or b"Success:" in result.stdout or b"overwrite Y/N" in result.stdout
                    or source.read_bytes() != SOURCE
                    or alias_path.read_bytes() != SOURCE):
                raise AssertionError(
                    f"{alias_label} on {route}: input/output alias was not refused "
                    f"unchanged: {result!r}")
            if alias_path != source and not os.path.samefile(source, alias_path):
                raise AssertionError(f"{alias_label}: alias identity changed during refusal")
            checked += 1

    malformed_celtscale = root / "wrong type CELTSCALE.dxf"
    malformed_celtscale_bytes = SOURCE.replace(
        b"9\n$ACADVER\n1\nAC1021\n",
        b"9\n$ACADVER\n1\nAC1021\n9\n$CELTSCALE\n70\n7\n", 1)
    malformed_celtscale.write_bytes(malformed_celtscale_bytes)
    for binary in (False, True):
        protected_output = root / ("wrong type CELTSCALE "
                                   + ("binary" if binary else "ascii")
                                   + " output.dxf")
        protected_output.write_bytes(SENTINEL)
        arguments = [malformed_celtscale, "-o", protected_output,
                     "-v2010", "-y"]
        if binary:
            arguments.append("-b")
        result = invoke(arguments)
        if (result.returncode != 1
                or protected_output.read_bytes() != SENTINEL
                or malformed_celtscale.read_bytes() != malformed_celtscale_bytes):
            raise AssertionError(
                "typed output silently replaced malformed CELTSCALE or changed "
                f"the protected destination: {result!r}")
        checked += 1

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

    # Legacy output remains interactive when overwrite authority is absent.
    # run_bounded supplies EOF on stdin, so this verifies the prompt/cancel path
    # without a tty or an unbounded test wait.
    destination.write_bytes(SENTINEL)
    result = invoke([source, "-v2007", destination])
    if (result.returncode != 1 or b"overwrite Y/N" not in result.stdout
            or b"Cancelled." not in result.stdout or b"Success:" in result.stdout
            or destination.read_bytes() != SENTINEL or source.read_bytes() != SOURCE):
        raise AssertionError(
            f"legacy overwrite prompt did not cancel safely at stdin EOF: {result!r}")
    checked += 1

    # FreeCAD's -o form never prompts. Missing -y refuses an existing file;
    # -y/-Y authorizes replacement for both ASCII and Binary output.
    destination.write_bytes(SENTINEL)
    result = invoke([source, "-o", destination])
    if (result.returncode != 1 or b"refusing to overwrite" not in result.stdout
            or b"overwrite Y/N" in result.stdout or destination.read_bytes() != SENTINEL
            or source.read_bytes() != SOURCE):
        raise AssertionError(f"FreeCAD -o overwrote without authority: {result!r}")
    checked += 1

    for token in ("-y", "-Y"):
        for binary in (False, True):
            destination.write_bytes(SENTINEL)
            arguments = [source, "-o", destination, token]
            if binary:
                arguments.append("-b")
            result = invoke(arguments)
            payload = destination.read_bytes()
            if (result.returncode != 0 or not output_has_revision(
                    payload, b"AC1021", binary)
                    or source.read_bytes() != SOURCE):
                raise AssertionError(
                    f"FreeCAD {token} overwrite failed for binary={binary}: {result!r}")
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
    checked += check_default_output_versions(converter, root, invoke)
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
    mixed = (str(source) + "\n" + str(missing) + "\n").encode("utf-8")
    output, result = invoke(mixed, "mixed missing last", 1)
    if (set(output.iterdir()) != {output / output_name(source)}
            or b"Success:" not in result.stdout or b"Failed:" not in result.stdout):
        raise AssertionError(
            "success-first mixed batch did not publish the valid item and report "
            "the later failure")
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
    checked += check_default_dwg_output_versions(
        converter, root, writer, dwg_source)

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


def check_partial_dwg_read_cli(converter, root, writer):
    """Exercise strict refusal and the explicit partial CLI route locally."""
    if writer is None:
        raise AssertionError("partial-read CLI control requires the local DWG writer")
    checked = 0

    def has_exact_partial_projection(result):
        projection = next((line for line in result.stderr.splitlines()
                           if line.startswith(
                               b"CLI source disposition projection:")), b"")
        fields = {field.split(b"=", 1)[0]: field.split(b"=", 1)[1]
                  for field in projection.split() if b"=" in field}
        expected = {
            b"sourceDwgEntityParseFailures": b"1",
            b"sourceDwgObjectParseFailures": b"0",
            b"sourceDwgFrameCoverage": b"partial",
            b"sourceDwgFramesFailed": b"1",
            b"sourceDwgFramesUnresolved": b"0",
            b"sourceDwgFramesUnrecognized": b"0",
        }
        if any(fields.get(key) != value for key, value in expected.items()):
            return False
        try:
            source_frames = int(fields[b"sourceDwgSourceFrames"])
            published = int(fields[b"sourceDwgFramesPublished"])
            disposition_total = sum(int(fields[key]) for key in (
                b"sourceDwgFramesPending", b"sourceDwgFramesDeferred",
                b"sourceDwgFramesStaged", b"sourceDwgFramesPublished",
                b"sourceDwgFramesQuarantined", b"sourceDwgFramesFailed",
                b"sourceDwgFramesUnresolved"))
        except (KeyError, ValueError):
            return False
        return source_frames > 1 and published == source_frames - 1 \
            and disposition_total == source_frames

    for revision, version in ((b"AC1015", "AC1015"),
                              (b"AC1018", "AC1018"),
                              (b"AC1021", "AC1021"),
                              (b"AC1024", "AC1024"),
                              (b"AC1027", "AC1027"),
                              (b"AC1032", "AC1032")):
        source = root / f"generated partial-read source {version}.dwg"
        generated = run_bounded(
            [str(writer), "--generate-cli-partial-read-source", str(source), version],
            10, 131072, cwd=root)
        if generated.returncode != 0 or not source.is_file():
            raise AssertionError(
                f"could not author {version} partial-read DWG locally: {generated!r}")
        original = source.read_bytes()
        if original[:6] != revision:
            raise AssertionError(
                f"partial-read DWG generator emitted the wrong version for {version}")

        direct_read = run_bounded(
            [str(writer), "--count-dwg-lines", str(source)],
            10, 131072, cwd=root)
        direct_fields = {
            field.split(b"=", 1)[0]: field.split(b"=", 1)[1]
            for field in direct_read.stdout.split() if b"=" in field
        }
        try:
            direct_source_frames = int(direct_fields[b"sourceFrames"])
            direct_published_frames = int(direct_fields[b"publishedFrames"])
            direct_failed_frames = int(direct_fields[b"failedFrames"])
        except (KeyError, ValueError):
            direct_source_frames = direct_published_frames = direct_failed_frames = -1
        if (direct_read.returncode != 0
                or b"readSucceeded=true" not in direct_read.stdout
                or b"lineCount=1" not in direct_read.stdout
                or b"entityParseFailures=1" not in direct_read.stdout
                or b"objectParseFailures=0" not in direct_read.stdout
                or direct_failed_frames != 1
                or direct_published_frames + direct_failed_frames
                    != direct_source_frames):
            raise AssertionError(
                f"{version} entity parse failure stopped the following LINE or "
                f"escaped terminal frame accounting: {direct_read!r}")
        checked += 1

        strict_output = root / f"partial-read strict result {version}.dxf"
        strict_output.write_bytes(SENTINEL)
        strict = run_bounded(
            [str(converter), str(source), "-o", str(strict_output), "-y", "-d"],
            10, 2 * 1024 * 1024, cwd=root)
        strict_required = (
            b"DWG read observations: schema=1 operationSucceeded=true sourceVersion="
            + revision,
            b"entityParseFailures=1",
            b"failed=1",
            b"sourceDwgEntityParseFailures=1",
            b"sourceDwgObjectParseFailures=0",
            b"sourceDwgFrameCoverage=partial",
            b"sourceDwgFramesFailed=1",
            b"sourceDwgFramesUnresolved=0",
            (b"AC1027 DWG source references are not fully qualified"
             if revision == b"AC1027"
             else b"strict-complete conversion requires successful, complete DWG read disposition"),
        )
        if (strict.returncode != 1 or strict_output.read_bytes() != SENTINEL
                or source.read_bytes() != original
                or any(item not in strict.stderr for item in strict_required)
                or b"WARNING: explicit partial DWG read mode" in strict.stderr):
            raise AssertionError(
                f"strict {version} partial-read refusal was not diagnosed atomically: "
                f"{strict!r}")
        checked += 1

        if revision == b"AC1027":
            # The explicit partial-read opt-in does not waive AC1027 source
            # reference qualification. Keep that distinct guard fail-closed
            # for both encodings and verify no destination is published.
            partial_output = root / f"partial-read admitted result {version}.dxf"
            partial_output.write_bytes(SENTINEL)
            partial = run_bounded(
                [str(converter), str(source), "-o", str(partial_output), "-y",
                 "--allow-partial-dwg-read", "-d"],
                10, 2 * 1024 * 1024, cwd=root)
            if (partial.returncode != 1 or partial_output.read_bytes() != SENTINEL
                    or source.read_bytes() != original
                    or b"AC1027 DWG source references are not fully qualified"
                    not in partial.stderr):
                raise AssertionError(
                    "AC1027 partial-read opt-in bypassed the unqualified-source "
                    f"guard or changed protected files: {partial!r}")
            checked += 1

            binary_output = root / f"partial-read binary result {version}.dxf"
            binary_output.write_bytes(SENTINEL)
            binary_partial = run_bounded(
                [str(converter), str(source), "-o", str(binary_output), "-b", "-y",
                 "--allow-partial-dwg-read", "-d"],
                10, 2 * 1024 * 1024, cwd=root)
            if (binary_partial.returncode != 1
                    or binary_output.read_bytes() != SENTINEL
                    or source.read_bytes() != original
                    or b"AC1027 DWG source references are not fully qualified"
                    not in binary_partial.stderr):
                raise AssertionError(
                    "AC1027 Binary partial-read opt-in bypassed the unqualified-source "
                    f"guard or changed protected files: {binary_partial!r}")
            checked += 1
            continue

        # The generated AC1015/AC1018 source carries VPORT state that cannot
        # be represented by those legacy DXF revisions. Preserve that state by
        # selecting the first admitting R2007 target instead of relying on a
        # same-revision default that correctly refuses the downgrade.
        partial_target_revision = (b"AC1021"
                                   if revision in (b"AC1015", b"AC1018")
                                   else revision)
        partial_target_option = (
            ("-v2007",)
            if partial_target_revision == b"AC1021" and revision < b"AC1021"
            else ())
        partial_output = root / f"partial-read admitted result {version}.dxf"
        partial = run_bounded(
            [str(converter), str(source), "-o", str(partial_output),
             *partial_target_option, "--allow-partial-dwg-read", "-d"],
            10, 2 * 1024 * 1024, cwd=root)
        partial_required = (
            b"WARNING: explicit partial DWG read mode",
            b"Owner/reference checks remain strict.",
            b"entityParseFailures=1",
            b"failed=1",
        )
        payload = partial_output.read_bytes() if partial_output.is_file() else b""
        line_count = sum(1 for line in payload.splitlines() if line.strip() == b"LINE")
        if (partial.returncode != 0 or b"Success:" not in partial.stdout
                or any(item not in partial.stderr for item in partial_required)
                or not has_exact_partial_projection(partial)
                or not output_has_revision(payload, partial_target_revision, False)
                or line_count != 1 or source.read_bytes() != original):
            raise AssertionError(
                f"explicit {version} partial-read route did not warn and publish only "
                f"retained data: {partial!r}")
        checked += 1

        reopened = run_bounded(
            [str(writer), "--count-dxf-lines", str(partial_output)],
            10, 131072, cwd=root)
        if (reopened.returncode != 0 or b"lineCount=1" not in reopened.stdout
                or b"vportCount=1" not in reopened.stdout
                or partial_output.read_bytes() != payload
                or source.read_bytes() != original):
            raise AssertionError(
                f"{version} partial output did not reopen with the retained LINE only: "
                f"{reopened!r}")
        checked += 1

        # The opt-in partial path must apply the same disposition policy when
        # writing Binary DXF. Reopen through the CLI as ASCII so that this
        # check observes the retained entity instead of just the binary header.
        binary_output = root / f"partial-read binary result {version}.dxf"
        binary_output.write_bytes(SENTINEL)
        binary_strict = run_bounded(
            [str(converter), str(source), "-o", str(binary_output),
             "-b", "-y", "-d"], 10, 2 * 1024 * 1024, cwd=root)
        if (binary_strict.returncode != 1
                or binary_output.read_bytes() != SENTINEL
                or source.read_bytes() != original
                or any(item not in binary_strict.stderr for item in strict_required)):
            raise AssertionError(
                f"strict {version} Binary partial-read refusal was not atomic: "
                f"{binary_strict!r}")
        checked += 1

        binary_partial = run_bounded(
            [str(converter), str(source), "-o", str(binary_output), "-b", "-y",
             *partial_target_option, "--allow-partial-dwg-read", "-d"],
            10, 2 * 1024 * 1024, cwd=root)
        binary_payload = binary_output.read_bytes()
        if (binary_partial.returncode != 0
                or any(item not in binary_partial.stderr for item in partial_required)
                or not has_exact_partial_projection(binary_partial)
                or not output_has_revision(binary_payload, partial_target_revision, True)
                or source.read_bytes() != original):
            raise AssertionError(
                f"explicit {version} Binary partial-read output failed: {binary_partial!r}")
        checked += 1

        binary_reopened = run_bounded(
            [str(writer), "--count-dxf-lines", str(binary_output)],
            10, 2 * 1024 * 1024, cwd=root)
        if (binary_reopened.returncode != 0
                or b"lineCount=1" not in binary_reopened.stdout
                or b"vportCount=1" not in binary_reopened.stdout
                or binary_output.read_bytes() != binary_payload
                or source.read_bytes() != original):
            raise AssertionError(
                f"{version} Binary partial output did not reopen with the retained "
                f"LINE only: {binary_reopened!r}")
        checked += 1

    # A size-delimited OBJECTS frame may be valid while its common object
    # prologue is truncated. The reader must continue through the independent
    # object map and publish later records, but both strict and explicit-
    # partial export stay closed because the integrity ledger contains an
    # error for that failed frame.
    malformed_object_profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        # AC1027 has an independent source-reference closure issue that blocks
        # CLI output, so qualify only the reader's continuation and accounting.
        "AC1027": None,
        "AC1032": "2018",
    }
    for revision, target_year in malformed_object_profiles.items():
        source = root / f"generated malformed OBJECTS source {revision}.dwg"
        generator = (
            "--generate-cli-malformed-object-prologue-source"
            if revision in ("AC1015", "AC1018") else
            "--generate-cli-malformed-object-source")
        generated = run_bounded(
            [str(writer), generator, str(source), revision],
            10, 131072, cwd=root)
        if generated.returncode != 0 or not source.is_file():
            raise AssertionError(
                f"could not author malformed {revision} OBJECTS source: {generated!r}")
        original = source.read_bytes()
        if original[:6] != revision.encode("ascii"):
            raise AssertionError(
                f"malformed OBJECTS generator emitted another revision for {revision}")

        counted = run_bounded(
            [str(writer), "--count-dwg-lines", str(source)],
            10, 131072, cwd=root)
        counted_fields = {
            field.split(b"=", 1)[0]: field.split(b"=", 1)[1]
            for field in counted.stdout.split() if b"=" in field
        }
        try:
            source_frames = int(counted_fields[b"sourceFrames"])
            published_frames = int(counted_fields[b"publishedFrames"])
            failed_frames = int(counted_fields[b"failedFrames"])
        except (KeyError, ValueError):
            source_frames = published_frames = failed_frames = -1
        if (counted.returncode != 0 or b"readSucceeded=true" not in counted.stdout
                or b"lineCount=1" not in counted.stdout
                or b"objectParseFailures=1" not in counted.stdout
                or failed_frames != 1 or source_frames <= 1
                or published_frames + failed_frames != source_frames):
            raise AssertionError(
                f"{revision} malformed object stopped later OBJECTS records: {counted!r}")
        checked += 1

        if target_year is None:
            # AC1027 is useful for reader recovery, but its independent
            # source-reference closure gate masks this CLI disposition. Verify
            # that neither strict nor explicit-partial mode bypasses it.
            output = root / f"malformed OBJECTS body {revision} source gate.dxf"
            output.write_bytes(SENTINEL)
            for partial_mode in (False, True):
                arguments = [str(converter), str(source), "-o", str(output),
                             "-v2013", "-y", "-d"]
                if partial_mode:
                    arguments.append("--allow-partial-dwg-read")
                result = run_bounded(arguments, 10, 2 * 1024 * 1024, cwd=root)
                if (result.returncode != 1 or output.read_bytes() != SENTINEL
                        or source.read_bytes() != original
                        or b"AC1027 DWG source references are not fully qualified"
                           not in result.stderr):
                    raise AssertionError(
                        f"{revision} {('partial' if partial_mode else 'strict')} "
                        f"output bypassed its source-reference gate: {result!r}")
                if partial_mode and (
                        b"WARNING: explicit partial DWG read mode" not in result.stderr
                        or b"sourceDwgObjectParseFailures=1" not in result.stderr):
                    raise AssertionError(
                        f"{revision} partial source-gate refusal omitted its "
                        f"failure disposition: {result!r}")
                checked += 1
            continue

        for binary in (False, True):
            encoding = "Binary" if binary else "ASCII"
            for partial_mode in (False, True):
                mode = "partial" if partial_mode else "strict"
                output = root / (
                    f"malformed OBJECTS {mode} {revision} {encoding}.dxf")
                output.write_bytes(SENTINEL)
                arguments = [str(converter), str(source), "-o", str(output),
                             f"-v{target_year}", "-y", "-d"]
                if binary:
                    arguments.append("-b")
                if partial_mode:
                    arguments.append("--allow-partial-dwg-read")
                result = run_bounded(arguments, 10, 2 * 1024 * 1024, cwd=root)
                projection = next((line for line in result.stderr.splitlines()
                                   if line.startswith(
                                       b"CLI source disposition projection:")), b"")
                projected_fields = {
                    field.split(b"=", 1)[0]: field.split(b"=", 1)[1]
                    for field in projection.split() if b"=" in field
                }
                try:
                    projected_source_frames = int(
                        projected_fields[b"sourceDwgSourceFrames"])
                    projected_published_frames = int(
                        projected_fields[b"sourceDwgFramesPublished"])
                except (KeyError, ValueError):
                    projected_source_frames = projected_published_frames = -1
                required_projection = (
                    b"sourceDwgObjectParseFailures=1",
                    b"sourceDwgEntityParseFailures=0",
                    b"sourceDwgFrameCoverage=partial",
                    b"sourceDwgFramesFailed=1",
                    b"sourceDwgIntegrityErrorsAmongRetained=1",
                )
                expected_gate = (
                    b"partial DWG output requires a successful read and finalized, bounded frame dispositions"
                    if partial_mode else
                    b"strict-complete conversion requires successful, complete DWG read disposition")
                if (result.returncode != 1 or output.read_bytes() != SENTINEL
                        or source.read_bytes() != original
                        or b"operationSucceeded=true" not in result.stderr
                        or any(field not in projection for field in required_projection)
                        or projected_source_frames <= 1
                        or projected_published_frames + 1 != projected_source_frames
                        or expected_gate not in result.stderr):
                    raise AssertionError(
                        f"{revision} malformed OBJECTS {mode} {encoding} did not "
                        f"fail closed atomically: {result!r}")
                if partial_mode and (
                        b"WARNING: explicit partial DWG read mode" not in result.stderr
                        or b"is not a completeness/support claim" not in result.stderr):
                    raise AssertionError(
                        f"{revision} malformed OBJECTS partial mode lacked its "
                        f"incomplete-output warning: {result!r}")
                checked += 1

    # Keep the common object prologue intact and corrupt a supported
    # MLINESTYLE body. This distinguishes body-parser recovery from the
    # truncated-prologue control above: the OBJECTS frame must be isolated,
    # later frames retained, and strict export must stay closed. Explicit
    # partial mode may publish the retained graph when no retained integrity
    # error makes omission unsafe.
    malformed_body_profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        # The independent source-reference closure gate prevents output
        # qualification for AC1027; retain this profile as reader-only.
        "AC1027": None,
        "AC1032": "2018",
    }
    for revision, target_year in malformed_body_profiles.items():
        source = root / f"generated malformed OBJECTS body source {revision}.dwg"
        generated = run_bounded(
            [str(writer), "--generate-cli-malformed-object-body-source",
             str(source), revision], 10, 131072, cwd=root)
        if generated.returncode != 0 or not source.is_file():
            raise AssertionError(
                f"could not author malformed {revision} OBJECTS body source: "
                f"{generated!r}")
        original = source.read_bytes()
        if original[:6] != revision.encode("ascii"):
            raise AssertionError(
                f"malformed OBJECTS body generator emitted another revision for {revision}")

        counted = run_bounded(
            [str(writer), "--count-dwg-lines", str(source)],
            10, 131072, cwd=root)
        counted_fields = {
            field.split(b"=", 1)[0]: field.split(b"=", 1)[1]
            for field in counted.stdout.split() if b"=" in field
        }
        try:
            source_frames = int(counted_fields[b"sourceFrames"])
            published_frames = int(counted_fields[b"publishedFrames"])
            failed_frames = int(counted_fields[b"failedFrames"])
        except (KeyError, ValueError):
            source_frames = published_frames = failed_frames = -1
        if (counted.returncode != 0 or b"readSucceeded=true" not in counted.stdout
                or b"lineCount=1" not in counted.stdout
                or b"objectParseFailures=1" not in counted.stdout
                or b"entityParseFailures=0" not in counted.stdout
                or failed_frames != 1 or source_frames <= 1
                or published_frames + failed_frames != source_frames):
            raise AssertionError(
                f"{revision} malformed object body stopped later OBJECTS records: "
                f"{counted!r}")
        checked += 1

        if target_year is None:
            continue

        for binary in (False, True):
            encoding = "Binary" if binary else "ASCII"
            output = root / f"malformed OBJECTS body {revision} {encoding}.dxf"
            output.write_bytes(SENTINEL)
            for partial_mode in (False, True):
                mode = "partial" if partial_mode else "strict"
                arguments = [str(converter), str(source), "-o", str(output),
                             f"-v{target_year}", "-y", "-d"]
                if binary:
                    arguments.append("-b")
                if partial_mode:
                    arguments.append("--allow-partial-dwg-read")
                result = run_bounded(arguments, 10, 2 * 1024 * 1024, cwd=root)
                projection = next((line for line in result.stderr.splitlines()
                                   if line.startswith(
                                       b"CLI source disposition projection:")), b"")
                required_projection = (
                    b"sourceDwgObjectParseFailures=1",
                    b"sourceDwgEntityParseFailures=0",
                    b"sourceDwgFrameCoverage=partial",
                    b"sourceDwgFramesFailed=1",
                    # The failed MLINESTYLE is represented by its terminal frame
                    # disposition; its malformed body does not add a retained
                    # object-level integrity row.
                    b"sourceDwgIntegrityErrorsAmongRetained=0",
                )
                expected_gate = (
                    b"strict-complete conversion requires successful, complete DWG read disposition")
                if (result.returncode != (0 if partial_mode else 1)
                        or source.read_bytes() != original
                        or b"operationSucceeded=true" not in result.stderr
                        or any(field not in projection for field in required_projection)):
                    raise AssertionError(
                        f"{revision} malformed OBJECTS body {mode} output had the wrong "
                        f"disposition: {result!r}")
                if partial_mode:
                    reopened = run_bounded(
                        [str(writer), "--count-dxf-lines", str(output)],
                        10, 131072, cwd=root)
                    if (not output.is_file() or output.read_bytes() == SENTINEL
                            or reopened.returncode != 0
                            or b"lineCount=1" not in reopened.stdout
                            or b"vportCount=1" not in reopened.stdout):
                        raise AssertionError(
                            f"{revision} malformed OBJECTS body partial output did not "
                            f"reopen with retained LINE: {reopened!r}")
                elif (output.read_bytes() != SENTINEL or expected_gate not in result.stderr):
                    raise AssertionError(
                        f"{revision} malformed OBJECTS body strict output did not "
                        f"refuse atomically: {result!r}")
                if partial_mode and (
                        b"WARNING: explicit partial DWG read mode" not in result.stderr
                        or b"is not a completeness/support claim" not in result.stderr):
                    raise AssertionError(
                        f"{revision} malformed OBJECTS body partial mode lacked its "
                        f"incomplete-output warning: {result!r}")
                checked += 1

    # Exercise a second typed OBJECTS body parser. DBCOLOR has a compact CMC
    # payload and no semantic edge in this generated profile, so partial
    # output can prove recovery and readback independently of MLINESTYLE.
    malformed_dbcolor_profiles = {
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    for revision, target_year in malformed_dbcolor_profiles.items():
        source = root / f"generated malformed DBCOLOR body source {revision}.dwg"
        generated = run_bounded(
            [str(writer), "--generate-cli-malformed-dbcolor-body-source",
             str(source), revision], 10, 131072, cwd=root)
        if generated.returncode != 0 or not source.is_file():
            raise AssertionError(
                f"could not author malformed {revision} DBCOLOR source: {generated!r}")
        original = source.read_bytes()
        counted = run_bounded(
            [str(writer), "--count-dwg-lines", str(source)],
            10, 131072, cwd=root)
        fields = {
            field.split(b"=", 1)[0]: field.split(b"=", 1)[1]
            for field in counted.stdout.split() if b"=" in field
        }
        try:
            source_frames = int(fields[b"sourceFrames"])
            published_frames = int(fields[b"publishedFrames"])
            failed_frames = int(fields[b"failedFrames"])
        except (KeyError, ValueError):
            source_frames = published_frames = failed_frames = -1
        if (counted.returncode != 0 or b"readSucceeded=true" not in counted.stdout
                or b"lineCount=1" not in counted.stdout
                or b"objectParseFailures=1" not in counted.stdout
                or b"entityParseFailures=0" not in counted.stdout
                or failed_frames != 1 or source_frames <= 1
                or published_frames + 1 != source_frames):
            raise AssertionError(
                f"{revision} malformed DBCOLOR stopped later object frames: "
                f"{counted!r}")
        checked += 1

        if target_year is None:
            output = root / f"malformed DBCOLOR body {revision} source gate.dxf"
            output.write_bytes(SENTINEL)
            for partial_mode in (False, True):
                arguments = [str(converter), str(source), "-o", str(output),
                             "-v2013", "-y", "-d"]
                if partial_mode:
                    arguments.append("--allow-partial-dwg-read")
                result = run_bounded(arguments, 10, 2 * 1024 * 1024, cwd=root)
                if (result.returncode != 1 or output.read_bytes() != SENTINEL
                        or source.read_bytes() != original
                        or b"AC1027 DWG source references are not fully qualified"
                           not in result.stderr):
                    raise AssertionError(
                        f"{revision} DBCOLOR body output bypassed source gate: "
                        f"{result!r}")
                checked += 1
            continue

        for binary in (False, True):
            encoding = "Binary" if binary else "ASCII"
            output = root / f"malformed DBCOLOR body {revision} {encoding}.dxf"
            output.write_bytes(SENTINEL)
            for partial_mode in (False, True):
                mode = "partial" if partial_mode else "strict"
                arguments = [str(converter), str(source), "-o", str(output),
                             f"-v{target_year}", "-y", "-d"]
                if binary:
                    arguments.append("-b")
                if partial_mode:
                    arguments.append("--allow-partial-dwg-read")
                result = run_bounded(arguments, 10, 2 * 1024 * 1024, cwd=root)
                projection = next((line for line in result.stderr.splitlines()
                                   if line.startswith(
                                       b"CLI source disposition projection:")), b"")
                required = (
                    b"sourceDwgObjectParseFailures=1",
                    b"sourceDwgEntityParseFailures=0",
                    b"sourceDwgFrameCoverage=partial",
                    b"sourceDwgFramesFailed=1",
                    b"sourceDwgIntegrityErrorsAmongRetained=0",
                )
                if (result.returncode != (0 if partial_mode else 1)
                        or source.read_bytes() != original
                        or any(field not in projection for field in required)):
                    raise AssertionError(
                        f"{revision} DBCOLOR body {mode} had the wrong disposition: "
                        f"{result!r}")
                if partial_mode:
                    reopened = run_bounded(
                        [str(writer), "--count-dxf-lines", str(output)],
                        10, 131072, cwd=root)
                    if (output.read_bytes() == SENTINEL
                            or reopened.returncode != 0
                            or b"lineCount=1" not in reopened.stdout
                            or b"vportCount=1" not in reopened.stdout):
                        raise AssertionError(
                            f"{revision} DBCOLOR partial output did not reopen: "
                            f"{reopened!r}")
                    if (b"WARNING: explicit partial DWG read mode" not in result.stderr
                            or b"is not a completeness/support claim" not in result.stderr):
                        raise AssertionError(
                            f"{revision} DBCOLOR partial output lacked warning: "
                            f"{result!r}")
                else:
                    gate = (b"strict-complete conversion requires successful, complete DWG read disposition")
                    if output.read_bytes() != SENTINEL or gate not in result.stderr:
                        raise AssertionError(
                            f"{revision} DBCOLOR strict output did not refuse atomically: "
                            f"{result!r}")
                checked += 1
    malformed_xrecord_profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    for revision, target_year in malformed_xrecord_profiles.items():
        source = root / f"generated malformed XRECORD body source {revision}.dwg"
        generated = run_bounded(
            [str(writer), "--generate-cli-malformed-xrecord-body-source",
             str(source), revision], 10, 131072, cwd=root)
        if generated.returncode != 0 or not source.is_file():
            raise AssertionError(
                f"could not author malformed {revision} XRECORD source: {generated!r}")
        original = source.read_bytes()
        counted = run_bounded(
            [str(writer), "--count-dwg-lines", str(source)],
            10, 131072, cwd=root)
        fields = {
            field.split(b"=", 1)[0]: field.split(b"=", 1)[1]
            for field in counted.stdout.split() if b"=" in field
        }
        try:
            source_frames = int(fields[b"sourceFrames"])
            published_frames = int(fields[b"publishedFrames"])
            failed_frames = int(fields[b"failedFrames"])
        except (KeyError, ValueError):
            source_frames = published_frames = failed_frames = -1
        if (counted.returncode != 0 or b"readSucceeded=true" not in counted.stdout
                or b"lineCount=1" not in counted.stdout
                or b"objectParseFailures=1" not in counted.stdout
                or b"entityParseFailures=0" not in counted.stdout
                or failed_frames != 1 or source_frames <= 1
                or published_frames + 1 != source_frames):
            raise AssertionError(
                f"{revision} malformed XRECORD stopped later object frames: "
                f"{counted!r}")
        checked += 1

        if target_year is None:
            output = root / f"malformed XRECORD body {revision} source gate.dxf"
            output.write_bytes(SENTINEL)
            for partial_mode in (False, True):
                arguments = [str(converter), str(source), "-o", str(output),
                             "-v2013", "-y", "-d"]
                if partial_mode:
                    arguments.append("--allow-partial-dwg-read")
                result = run_bounded(arguments, 10, 2 * 1024 * 1024, cwd=root)
                if (result.returncode != 1 or output.read_bytes() != SENTINEL
                        or source.read_bytes() != original
                        or b"AC1027 DWG source references are not fully qualified"
                           not in result.stderr):
                    raise AssertionError(
                        f"{revision} XRECORD body output bypassed source gate: "
                        f"{result!r}")
                checked += 1
            continue

        for binary in (False, True):
            encoding = "Binary" if binary else "ASCII"
            output = root / f"malformed XRECORD body {revision} {encoding}.dxf"
            output.write_bytes(SENTINEL)
            for partial_mode in (False, True):
                mode = "partial" if partial_mode else "strict"
                arguments = [str(converter), str(source), "-o", str(output),
                             f"-v{target_year}", "-y", "-d"]
                if binary:
                    arguments.append("-b")
                if partial_mode:
                    arguments.append("--allow-partial-dwg-read")
                result = run_bounded(arguments, 10, 2 * 1024 * 1024, cwd=root)
                projection = next((line for line in result.stderr.splitlines()
                                   if line.startswith(
                                       b"CLI source disposition projection:")), b"")
                required = (
                    b"sourceDwgObjectParseFailures=1",
                    b"sourceDwgEntityParseFailures=0",
                    b"sourceDwgFrameCoverage=partial",
                    b"sourceDwgFramesFailed=1",
                    b"sourceDwgIntegrityErrorsAmongRetained=0",
                )
                if (result.returncode != (0 if partial_mode else 1)
                        or source.read_bytes() != original
                        or any(field not in projection for field in required)):
                    raise AssertionError(
                        f"{revision} XRECORD body {mode} had the wrong disposition: "
                        f"{result!r}")
                if partial_mode:
                    reopened = run_bounded(
                        [str(writer), "--count-dxf-lines", str(output)],
                        10, 131072, cwd=root)
                    if (output.read_bytes() == SENTINEL
                            or reopened.returncode != 0
                            or b"lineCount=1" not in reopened.stdout
                            or b"vportCount=1" not in reopened.stdout
                            or b"WARNING: explicit partial DWG read mode"
                               not in result.stderr
                            or b"is not a completeness/support claim"
                               not in result.stderr):
                        raise AssertionError(
                            f"{revision} XRECORD partial output did not reopen cleanly: "
                            f"{reopened!r}")
                else:
                    gate = (b"strict-complete conversion requires successful, complete DWG read disposition")
                    if output.read_bytes() != SENTINEL or gate not in result.stderr:
                        raise AssertionError(
                            f"{revision} XRECORD strict output did not refuse atomically: "
                            f"{result!r}")
                checked += 1
    malformed_dictionaryvar_profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    for revision, target_year in malformed_dictionaryvar_profiles.items():
        source = root / f"generated malformed DICTIONARYVAR body source {revision}.dwg"
        generated = run_bounded(
            [str(writer), "--generate-cli-malformed-dictionaryvar-body-source",
             str(source), revision], 10, 131072, cwd=root)
        if generated.returncode != 0 or not source.is_file():
            raise AssertionError(
                f"could not author malformed {revision} DICTIONARYVAR source: "
                f"{generated!r}")
        original = source.read_bytes()
        counted = run_bounded(
            [str(writer), "--count-dwg-lines", str(source)],
            10, 131072, cwd=root)
        fields = {
            field.split(b"=", 1)[0]: field.split(b"=", 1)[1]
            for field in counted.stdout.split() if b"=" in field
        }
        try:
            source_frames = int(fields[b"sourceFrames"])
            published_frames = int(fields[b"publishedFrames"])
            failed_frames = int(fields[b"failedFrames"])
        except (KeyError, ValueError):
            source_frames = published_frames = failed_frames = -1
        if (counted.returncode != 0 or b"readSucceeded=true" not in counted.stdout
                or b"lineCount=1" not in counted.stdout
                or b"objectParseFailures=1" not in counted.stdout
                or b"entityParseFailures=0" not in counted.stdout
                or failed_frames != 1 or source_frames <= 1
                or published_frames + 1 != source_frames):
            raise AssertionError(
                f"{revision} malformed DICTIONARYVAR stopped later object frames: "
                f"{counted!r}")
        checked += 1

        if target_year is None:
            output = root / f"malformed DICTIONARYVAR body {revision} source gate.dxf"
            output.write_bytes(SENTINEL)
            for partial_mode in (False, True):
                arguments = [str(converter), str(source), "-o", str(output),
                             "-v2013", "-y", "-d"]
                if partial_mode:
                    arguments.append("--allow-partial-dwg-read")
                result = run_bounded(arguments, 10, 2 * 1024 * 1024, cwd=root)
                if (result.returncode != 1 or output.read_bytes() != SENTINEL
                        or source.read_bytes() != original
                        or b"AC1027 DWG source references are not fully qualified"
                           not in result.stderr):
                    raise AssertionError(
                        f"{revision} DICTIONARYVAR body output bypassed source gate: "
                        f"{result!r}")
                checked += 1
            continue

        for binary in (False, True):
            encoding = "Binary" if binary else "ASCII"
            output = root / f"malformed DICTIONARYVAR body {revision} {encoding}.dxf"
            output.write_bytes(SENTINEL)
            for partial_mode in (False, True):
                mode = "partial" if partial_mode else "strict"
                arguments = [str(converter), str(source), "-o", str(output),
                             f"-v{target_year}", "-y", "-d"]
                if binary:
                    arguments.append("-b")
                if partial_mode:
                    arguments.append("--allow-partial-dwg-read")
                result = run_bounded(arguments, 10, 2 * 1024 * 1024, cwd=root)
                projection = next((line for line in result.stderr.splitlines()
                                   if line.startswith(
                                       b"CLI source disposition projection:")), b"")
                required = (
                    b"sourceDwgObjectParseFailures=1",
                    b"sourceDwgEntityParseFailures=0",
                    b"sourceDwgFrameCoverage=partial",
                    b"sourceDwgFramesFailed=1",
                    b"sourceDwgIntegrityErrorsAmongRetained=0",
                )
                if (result.returncode != (0 if partial_mode else 1)
                        or source.read_bytes() != original
                        or any(field not in projection for field in required)):
                    raise AssertionError(
                        f"{revision} DICTIONARYVAR body {mode} had the wrong "
                        f"disposition: {result!r}")
                if partial_mode:
                    reopened = run_bounded(
                        [str(writer), "--count-dxf-lines", str(output)],
                        10, 131072, cwd=root)
                    if (output.read_bytes() == SENTINEL
                            or reopened.returncode != 0
                            or b"lineCount=1" not in reopened.stdout
                            or b"vportCount=1" not in reopened.stdout
                            or b"WARNING: explicit partial DWG read mode"
                               not in result.stderr
                            or b"is not a completeness/support claim"
                               not in result.stderr):
                        raise AssertionError(
                            f"{revision} DICTIONARYVAR partial output did not reopen: "
                            f"{reopened!r}")
                else:
                    gate = (b"strict-complete conversion requires successful, complete DWG read disposition")
                    if output.read_bytes() != SENTINEL or gate not in result.stderr:
                        raise AssertionError(
                            f"{revision} DICTIONARYVAR strict output did not refuse "
                            f"atomically: {result!r}")
                checked += 1
    return checked


def check_malformed_object_body_family(
        converter, root, writer, object_name, generator_option, profiles):
    """Exercise one bounded typed OBJECTS-body parser family."""
    if writer is None:
        raise AssertionError(
            f"{object_name} recovery control requires the local DWG writer")
    checked = 0
    output_revisions = {
        "2007": b"AC1021",
        "2010": b"AC1024",
        "2013": b"AC1027",
        "2018": b"AC1032",
    }
    for revision, target_year in profiles.items():
        source = root / (
            f"generated malformed {object_name} body source {revision}.dwg")
        generated = run_bounded(
            [str(writer), generator_option, str(source), revision],
            10, 131072, cwd=root)
        if generated.returncode != 0 or not source.is_file():
            raise AssertionError(
                f"could not author malformed {revision} {object_name} source: "
                f"{generated!r}")
        original = source.read_bytes()
        if original[:6] != revision.encode("ascii"):
            raise AssertionError(
                f"{object_name} generator emitted the wrong revision for {revision}")

        counted = run_bounded(
            [str(writer), "--count-dwg-lines", str(source)],
            10, 131072, cwd=root)
        fields = {
            field.split(b"=", 1)[0]: field.split(b"=", 1)[1]
            for field in counted.stdout.split() if b"=" in field
        }
        try:
            source_frames = int(fields[b"sourceFrames"])
            published_frames = int(fields[b"publishedFrames"])
            failed_frames = int(fields[b"failedFrames"])
        except (KeyError, ValueError):
            source_frames = published_frames = failed_frames = -1
        if (counted.returncode != 0
                or b"readSucceeded=true" not in counted.stdout
                or b"lineCount=1" not in counted.stdout
                or b"objectParseFailures=1" not in counted.stdout
                or b"entityParseFailures=0" not in counted.stdout
                or failed_frames != 1 or source_frames <= 1
                or published_frames + 1 != source_frames):
            raise AssertionError(
                f"{revision} malformed {object_name} stopped later OBJECTS frames: "
                f"{counted!r}")
        checked += 1

        if target_year is None:
            output = root / (
                f"malformed {object_name} body {revision} source gate.dxf")
            output.write_bytes(SENTINEL)
            for partial_mode in (False, True):
                arguments = [str(converter), str(source), "-o", str(output),
                             "-v2013", "-y", "-d"]
                if partial_mode:
                    arguments.append("--allow-partial-dwg-read")
                result = run_bounded(
                    arguments, 10, 2 * 1024 * 1024, cwd=root)
                if (result.returncode != 1
                        or output.read_bytes() != SENTINEL
                        or source.read_bytes() != original
                        or b"AC1027 DWG source references are not fully qualified"
                           not in result.stderr):
                    raise AssertionError(
                        f"{revision} {object_name} output bypassed source gate: "
                        f"{result!r}")
                checked += 1
            continue

        for binary in (False, True):
            encoding = "Binary" if binary else "ASCII"
            output = root / (
                f"malformed {object_name} body {revision} {encoding}.dxf")
            for partial_mode in (False, True):
                output.write_bytes(SENTINEL)
                mode = "partial" if partial_mode else "strict"
                arguments = [str(converter), str(source), "-o", str(output),
                             f"-v{target_year}", "-y", "-d"]
                if binary:
                    arguments.append("-b")
                if partial_mode:
                    arguments.append("--allow-partial-dwg-read")
                result = run_bounded(
                    arguments, 10, 2 * 1024 * 1024, cwd=root)
                projection = next((line for line in result.stderr.splitlines()
                                   if line.startswith(
                                       b"CLI source disposition projection:")), b"")
                required = (
                    b"sourceDwgObjectParseFailures=1",
                    b"sourceDwgEntityParseFailures=0",
                    b"sourceDwgFrameCoverage=partial",
                    b"sourceDwgFramesFailed=1",
                    b"sourceDwgIntegrityErrorsAmongRetained=0",
                )
                if (result.returncode != (0 if partial_mode else 1)
                        or source.read_bytes() != original
                        or any(field not in projection for field in required)):
                    raise AssertionError(
                        f"{revision} malformed {object_name} {mode} disposition "
                        f"was incorrect: {result!r}")
                if partial_mode:
                    reopened = run_bounded(
                        [str(writer), "--count-dxf-lines", str(output)],
                        10, 131072, cwd=root)
                    if (output.read_bytes() == SENTINEL
                            or not output_has_revision(
                                output.read_bytes(),
                                output_revisions[target_year], binary)
                            or reopened.returncode != 0
                            or b"lineCount=1" not in reopened.stdout
                            or b"vportCount=1" not in reopened.stdout
                            or b"WARNING: explicit partial DWG read mode"
                               not in result.stderr
                            or b"is not a completeness/support claim"
                               not in result.stderr):
                        raise AssertionError(
                            f"{revision} {object_name} partial output failed readback: "
                            f"{reopened!r}")
                elif (output.read_bytes() != SENTINEL
                      or b"strict-complete conversion requires successful, "
                         b"complete DWG read disposition" not in result.stderr):
                    raise AssertionError(
                        f"{revision} {object_name} strict output did not refuse "
                        f"atomically: {result!r}")
                checked += 1
    return checked


def check_malformed_vba_project_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "VBA_PROJECT",
        "--generate-cli-malformed-vbaproject-body-source", profiles)


def check_malformed_blockrepresentation_body(converter, root, writer):
    profiles = {"AC1015": "2007", "AC1018": "2007", "AC1021": "2007"}
    return check_malformed_object_body_family(
        converter, root, writer, "BLOCKREPRESENTATION_DATA",
        "--generate-cli-malformed-blockrepresentation-body-source", profiles)


def check_malformed_group_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "GROUP",
        "--generate-cli-malformed-group-body-source", profiles)


def check_malformed_dictionary_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "DICTIONARY",
        "--generate-cli-malformed-dictionary-body-source", profiles)


def check_malformed_vxcontrol_body(converter, root, writer):
    profiles = {
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "VX_CONTROL",
        "--generate-cli-malformed-vxcontrol-body-source", profiles)


def check_malformed_vxtablerecord_body(converter, root, writer):
    profiles = {
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "VX_TABLE_RECORD",
        "--generate-cli-malformed-vxtablerecord-body-source", profiles)


def check_malformed_tvdeviceproperties_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "TV_DEVICE_PROPERTIES",
        "--generate-cli-malformed-tvdeviceproperties-body-source", profiles)


def check_malformed_pointpath_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "POINTPATH",
        "--generate-cli-malformed-pointpath-body-source", profiles)


def check_malformed_partialviewingindex_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "PARTIAL_VIEWING_INDEX",
        "--generate-cli-malformed-partialviewingindex-body-source", profiles)


def check_malformed_curvepath_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "CURVEPATH",
        "--generate-cli-malformed-curvepath-body-source", profiles)


def check_malformed_plotsettings_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "PLOTSETTINGS",
        "--generate-cli-malformed-plotsettings-body-source", profiles)


def check_malformed_underlaydefinition_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "UNDERLAYDEFINITION",
        "--generate-cli-malformed-underlaydefinition-body-source", profiles)


def check_malformed_material_body(converter, root, writer):
    profiles = {"AC1015": "2007", "AC1018": "2007"}
    return check_malformed_object_body_family(
        converter, root, writer, "MATERIAL",
        "--generate-cli-malformed-material-body-source", profiles)


def check_malformed_visualstyle_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "VISUALSTYLE",
        "--generate-cli-malformed-visualstyle-body-source", profiles)


def check_malformed_dependency_root_object_body(
        converter, root, writer, object_name, generator_option, root_key,
        profiles):
    if writer is None:
        raise AssertionError(
            f"{object_name} recovery control requires the local DWG writer")
    checked = 0
    for revision, target_year in profiles.items():
        source = root / (
            f"generated malformed {object_name} body source {revision}.dwg")
        generated = run_bounded(
            [str(writer), generator_option, str(source), revision],
            10, 131072, cwd=root)
        if generated.returncode != 0 or not source.is_file():
            raise AssertionError(
                f"could not author malformed {revision} {object_name} source: "
                f"{generated!r}")
        original = source.read_bytes()
        counted = run_bounded(
            [str(writer), "--count-dwg-lines", str(source)],
            10, 131072, cwd=root)
        fields = {
            field.split(b"=", 1)[0]: field.split(b"=", 1)[1]
            for field in counted.stdout.split() if b"=" in field
        }
        try:
            source_frames = int(fields[b"sourceFrames"])
            published_frames = int(fields[b"publishedFrames"])
            failed_frames = int(fields[b"failedFrames"])
        except (KeyError, ValueError):
            source_frames = published_frames = failed_frames = -1
        if (counted.returncode != 0
                or b"readSucceeded=true" not in counted.stdout
                or b"lineCount=1" not in counted.stdout
                or b"objectParseFailures=1" not in counted.stdout
                or b"entityParseFailures=0" not in counted.stdout
                or failed_frames != 1 or source_frames <= 1
                or published_frames + 1 != source_frames):
            raise AssertionError(
                f"{revision} malformed {object_name} stopped later OBJECTS "
                f"frames: {counted!r}")
        checked += 1

        target = "2013" if target_year is None else target_year
        for partial_mode in (False, True):
            output = root / (
                f"malformed {object_name} body {revision} "
                f"{'partial' if partial_mode else 'strict'}.dxf")
            output.write_bytes(SENTINEL)
            arguments = [str(converter), str(source), "-o", str(output),
                         f"-v{target}", "-y", "-d"]
            if partial_mode:
                arguments.append("--allow-partial-dwg-read")
            result = run_bounded(
                arguments, 10, 2 * 1024 * 1024, cwd=root)
            if target_year is None:
                refusal = b"AC1027 DWG source references are not fully qualified"
            elif partial_mode:
                refusal = (
                    b"NOD dictionary entry target is missing or ambiguous: key="
                    + root_key.encode("ascii"))
            else:
                refusal = (b"strict-complete conversion requires successful, "
                           b"complete DWG read disposition")
            projection = next((line for line in result.stderr.splitlines()
                               if line.startswith(
                                   b"CLI source disposition projection:")), b"")
            required = (
                b"sourceDwgObjectParseFailures=1",
                b"sourceDwgFrameCoverage=partial",
                b"sourceDwgFramesFailed=1",
            )
            if (result.returncode != 1 or output.read_bytes() != SENTINEL
                    or source.read_bytes() != original
                    or refusal not in result.stderr
                    or any(field not in projection for field in required)):
                raise AssertionError(
                    f"{revision} malformed {object_name} "
                    f"{'partial' if partial_mode else 'strict'} "
                    f"conversion did not refuse atomically at its integrity gate: "
                    f"{result!r}")
            checked += 1
    return checked


def check_malformed_wipeoutvariables_body(converter, root, writer):
    profiles = {"AC1015": "2007", "AC1018": "2007", "AC1021": "2007"}
    return check_malformed_dependency_root_object_body(
        converter, root, writer, "WIPEOUTVARIABLES",
        "--generate-cli-malformed-wipeoutvariables-body-source",
        "ACAD_WIPEOUT_VARS", profiles)


def check_malformed_rastervariables_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_dependency_root_object_body(
        converter, root, writer, "RASTERVARIABLES",
        "--generate-cli-malformed-rastervariables-body-source",
        "ACAD_IMAGE_VARS", profiles)


def check_malformed_background_solid_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "SOLIDBACKGROUND",
        "--generate-cli-malformed-background-solid-body-source", profiles)


def check_malformed_background_groundplane_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "GROUNDPLANEBACKGROUND",
        "--generate-cli-malformed-background-groundplane-body-source",
        profiles)


def check_malformed_background_gradient_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "GRADIENTBACKGROUND",
        "--generate-cli-malformed-background-gradient-body-source", profiles)


def check_malformed_background_image_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        # No one-bit mutation within the bounded Image body fails its parser.
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "IMAGEBACKGROUND",
        "--generate-cli-malformed-background-image-body-source", profiles)


def check_malformed_background_ibl_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        # No one-bit mutation within the bounded IBL body fails its parser.
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "IBLBACKGROUND",
        "--generate-cli-malformed-background-ibl-body-source", profiles)


def check_malformed_background_skylight_handle(converter, root, writer):
    profiles = {
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "SKYLIGHTBACKGROUND",
        "--generate-cli-malformed-background-skylight-handle-source", profiles)


def check_malformed_motionpath_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "MOTIONPATH",
        "--generate-cli-malformed-motionpath-body-source", profiles)


def check_malformed_sunstudy_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "SUNSTUDY",
        "--generate-cli-malformed-sunstudy-body-source", profiles)


def check_malformed_sun_body(converter, root, writer):
    profiles = {
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "SUN",
        "--generate-cli-malformed-sun-body-source", profiles)


def check_malformed_navisworks_modeldef_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "NAVISWORKSMODELDEF",
        "--generate-cli-malformed-navisworks-modeldef-body-source", profiles)


def check_malformed_section_manager_body(converter, root, writer):
    profiles = {
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "SECTION_MANAGER",
        "--generate-cli-malformed-section-manager-body-source", profiles)


def check_malformed_section_settings_body(converter, root, writer):
    profiles = {
        "AC1021": "2007",
        "AC1027": None,
    }
    return check_malformed_object_body_family(
        converter, root, writer, "SECTION_SETTINGS",
        "--generate-cli-malformed-section-settings-body-source", profiles)


def check_malformed_pointcloud_colormap_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "POINTCLOUDCOLORMAP",
        "--generate-cli-malformed-pointcloud-colormap-body-source", profiles)


def check_malformed_pointcloud_definition_bodies(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    definition = check_malformed_object_body_family(
        converter, root, writer, "POINTCLOUDDEFINITION",
        "--generate-cli-malformed-pointcloud-definition-body-source", profiles)
    definition_ex = check_malformed_object_body_family(
        converter, root, writer, "POINTCLOUDDEFINITIONEX",
        "--generate-cli-malformed-pointcloud-definition-ex-body-source",
        profiles)
    return definition + definition_ex


def check_malformed_pointcloud_reactor_bodies(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
    }
    reactor = check_malformed_object_body_family(
        converter, root, writer, "POINTCLOUDDEFREACTOR",
        "--generate-cli-malformed-pointcloud-reactor-body-source", profiles)
    reactor_ex = check_malformed_object_body_family(
        converter, root, writer, "POINTCLOUDDEFREACTOREX",
        "--generate-cli-malformed-pointcloud-reactor-ex-body-source", profiles)
    return reactor + reactor_ex


def check_malformed_mleaderstyle_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "MLEADERSTYLE",
        "--generate-cli-malformed-mleaderstyle-body-source", profiles)


def check_malformed_spatial_filter_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "SPATIAL_FILTER",
        "--generate-cli-malformed-spatial-filter-body-source", profiles)


def check_malformed_idbuffer_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "IDBUFFER",
        "--generate-cli-malformed-idbuffer-body-source", profiles)


def check_malformed_layer_index_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "LAYER_INDEX",
        "--generate-cli-malformed-layer-index-body-source", profiles)


def check_malformed_spatial_index_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "SPATIAL_INDEX",
        "--generate-cli-malformed-spatial-index-body-source", profiles)


def check_malformed_lightlist_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "LIGHTLIST",
        "--generate-cli-malformed-lightlist-body-source", profiles)


def check_malformed_geodata_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
    }
    qualified = check_malformed_object_body_family(
        converter, root, writer, "GEODATA",
        "--generate-cli-malformed-geodata-body-source", profiles)
    source = root / "generated malformed GEODATA body source AC1018.dwg"
    generated = run_bounded(
        [str(writer), "--generate-cli-malformed-geodata-body-source",
         str(source), "AC1018"], 10, 131072, cwd=root)
    if generated.returncode != 0 or not source.is_file():
        raise AssertionError(
            f"could not author malformed AC1018 GEODATA source: {generated!r}")
    original = source.read_bytes()
    counted = run_bounded(
        [str(writer), "--count-dwg-lines", str(source)],
        10, 131072, cwd=root)
    if (counted.returncode != 0
            or b"readSucceeded=true" not in counted.stdout
            or b"lineCount=1" not in counted.stdout
            or b"objectParseFailures=1" not in counted.stdout
            or b"entityParseFailures=0" not in counted.stdout
            or b"failedFrames=1" not in counted.stdout):
        raise AssertionError(
            f"AC1018 malformed GEODATA did not continue with one failed frame: "
            f"{counted!r}")
    checked = 2
    for binary in (False, True):
        for partial_mode in (False, True):
            output = root / (
                f"malformed GEODATA AC1018 {'Binary' if binary else 'ASCII'} "
                f"{'partial' if partial_mode else 'strict'}.dxf")
            output.write_bytes(SENTINEL)
            arguments = [str(converter), str(source), "-o", str(output),
                         "-v2007", "-y", "-d"]
            if binary:
                arguments.append("-b")
            if partial_mode:
                arguments.append("--allow-partial-dwg-read")
            result = run_bounded(
                arguments, 10, 2 * 1024 * 1024, cwd=root)
            expected_diagnostic = (
                b"DXF export preflight failed: GEODATA source CLASS, owner, "
                b"host-block, or body profile" if partial_mode else
                b"strict-complete conversion requires successful, complete "
                b"DWG read disposition")
            if (result.returncode != 1 or output.read_bytes() != SENTINEL
                    or source.read_bytes() != original
                    or expected_diagnostic not in result.stderr):
                raise AssertionError(
                    f"AC1018 malformed GEODATA {'partial' if partial_mode else 'strict'} "
                    f"{'Binary' if binary else 'ASCII'} refusal was not atomic: "
                    f"{result!r}")
            checked += 1
    return qualified + checked


def check_malformed_tablestyle_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "TABLESTYLE",
        "--generate-cli-malformed-tablestyle-body-source", profiles)


def check_malformed_field_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "FIELD",
        "--generate-cli-malformed-field-body-source", profiles)


def check_malformed_fieldlist_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "FIELDLIST",
        "--generate-cli-malformed-fieldlist-body-source", profiles)


def check_malformed_evaluationgraph_body(converter, root, writer):
    profiles = {
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "ACAD_EVALUATION_GRAPH",
        "--generate-cli-malformed-evaluationgraph-body-source", profiles)


def check_malformed_dimassoc_body(converter, root, writer):
    profiles = {
        "AC1021": "2007",
        # The generated AC1024 bounded body has no one-bit parser-failing mutation.
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "DIMASSOC",
        "--generate-cli-malformed-dimassoc-body-source", profiles)


def check_malformed_layout_body(converter, root, writer):
    profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    return check_malformed_object_body_family(
        converter, root, writer, "LAYOUT",
        "--generate-cli-malformed-layout-body-source", profiles)


def check_malformed_rendersettings_bodies(converter, root, writer):
    all_profiles = {
        "AC1015": "2007",
        "AC1018": "2007",
        "AC1021": "2007",
        "AC1024": "2010",
        "AC1027": None,
        "AC1032": "2018",
    }
    variants = {
        "Settings": ("--generate-cli-malformed-rendersettings-body-source",
                     all_profiles),
        "Global": ("--generate-cli-malformed-rendersettings-global-body-source",
                   all_profiles),
        "Environment": (
            "--generate-cli-malformed-rendersettings-environment-body-source",
            {"AC1015": "2007", "AC1018": "2007", "AC1021": "2007"}),
        "Entry": ("--generate-cli-malformed-rendersettings-entry-body-source",
                  all_profiles),
        "RapidRT": ("--generate-cli-malformed-rendersettings-rapidrt-body-source",
                    all_profiles),
        "MentalRay": (
            "--generate-cli-malformed-rendersettings-mentalray-body-source",
            all_profiles),
    }
    return {
        name: check_malformed_object_body_family(
            converter, root, writer, f"RENDERSETTINGS_{name.upper()}",
            generator_option, profiles)
        for name, (generator_option, profiles) in variants.items()
    }


def check_skipped_record_projection(converter, root, writer):
    """Check generated skipped custom-class disposition across writer revisions."""
    if writer is None:
        raise AssertionError("skip projection control requires the local DWG writer")
    checked = 0
    for version in ("AC1015", "AC1018", "AC1021",
                    "AC1024", "AC1027", "AC1032"):
        version_bytes = version.encode("ascii")
        strict_guard = (b"AC1027 DWG source references are not fully qualified"
                        if version == "AC1027" else
                        b"strict-complete conversion requires successful, complete DWG read disposition")
        profiles = (
            ("--generate-cli-skip-source", "single", 1, False),
            ("--generate-cli-skip-source-multiple", "multiple", 3, False),
            ("--generate-cli-skip-source-multi-class", "multi-class", 3, True),
        )
        for option, label, instances, distinct_classes in profiles:
            source = root / f"generated {label} skipped-record source {version}.dwg"
            generated = run_bounded(
                [str(writer), option, str(source), version], 10, 131072, cwd=root)
            if generated.returncode != 0 or not source.is_file():
                raise AssertionError(
                    f"could not author {version} {label} skipped-record DWG: {generated!r}")
            original = source.read_bytes()
            if original[:6] != version_bytes:
                raise AssertionError(f"{version} skip-source generator emitted another version")
            output = root / f"{label} skipped-record destination {version}.dxf"
            output.write_bytes(SENTINEL)
            result = run_bounded(
                [str(converter), str(source), "-o", str(output), "-v2013", "-y", "-d"],
                10, 2 * 1024 * 1024, cwd=root)
            projection = next((line for line in result.stderr.splitlines()
                               if line.startswith(b"CLI source disposition projection:")), b"")
            count = str(instances).encode("ascii")
            expected = (
                b"sourceDwgSkippedCustomClassInstances=" + count,
                b"sourceDwgSkippedUnsupportedObjectInstances=" + count,
            )
            if (result.returncode != 1 or output.read_bytes() != SENTINEL
                    or source.read_bytes() != original
                    or any(item not in projection for item in expected)
                    or strict_guard not in result.stderr):
                raise AssertionError(
                    f"{version} {label} skipped records were not projected and "
                    f"refused atomically: {result!r}")
            if distinct_classes and (
                    b"skippedCustomClasses: sourceNames=3 observedInstances=3"
                    not in result.stderr
                    or b"skippedUnsupportedObjects: sourceNames=3 observedInstances=3"
                    not in result.stderr):
                raise AssertionError(
                    f"{version} distinct skipped classes were not reported: {result!r}")
            checked += 1

    # A source TABLECONTENT can have a valid common object shell while its
    # body remains raw-only. Strict conversion must refuse it; explicit
    # partial mode may omit it only with a clear warning and exact frame
    # disposition. Generate locally for every admitted source revision whose
    # output preflight permits this isolated raw-only control; never check
    # these sources in as fixtures.
    raw_only_projection = (
        b"sourceDwgSkippedCustomClassInstances=2",
        b"sourceDwgSkippedUnsupportedObjectInstances=2",
        b"sourceDwgEntityParseFailures=0",
        b"sourceDwgObjectParseFailures=0",
        b"sourceDwgFrameCoverage=complete",
        b"sourceDwgFramesFailed=0",
        b"sourceDwgFramesUnresolved=0",
        b"sourceDwgFramesUnrecognized=0",
    )
    raw_only_versions = {
        # AC1015/AC1018 use the AC1021 target because generated VPORT state
        # is not representable by their source DXF revisions.
        "AC1015": (b"AC1015", b"AC1021", "2007", b"29"),
        "AC1018": (b"AC1018", b"AC1021", "2007", b"28"),
        "AC1021": (b"AC1021", b"AC1021", "2007", b"28"),
        "AC1024": (b"AC1024", b"AC1024", "2010", b"28"),
        "AC1032": (b"AC1032", b"AC1032", "2018", b"28"),
    }
    import re
    for source_version, (revision, target_revision, target_year, frame_count) in raw_only_versions.items():
        raw_source = root / f"generated raw-only TABLECONTENT {source_version}.dwg"
        generated = run_bounded(
            [str(writer), "--generate-cli-raw-only-source", str(raw_source),
             source_version], 10, 131072, cwd=root)
        if generated.returncode != 0 or not raw_source.is_file():
            raise AssertionError(
                f"could not author {source_version} raw-only TABLECONTENT source: "
                f"{generated!r}")
        raw_source_bytes = raw_source.read_bytes()
        if raw_source_bytes[:6] != revision:
            raise AssertionError(
                f"raw-only TABLECONTENT generator emitted the wrong revision "
                f"for {source_version}")

        for binary in (False, True):
            encoding = "Binary" if binary else "ASCII"
            strict_output = root / (
                f"raw-only TABLECONTENT strict {source_version} {encoding}.dxf")
            strict_output.write_bytes(SENTINEL)
            strict_args = [str(converter), str(raw_source), "-o", str(strict_output),
                           f"-v{target_year}", "-y", "-d"]
            if binary:
                strict_args.append("-b")
            strict = run_bounded(strict_args, 10, 2 * 1024 * 1024, cwd=root)
            projection = next((line for line in strict.stderr.splitlines()
                               if line.startswith(
                                   b"CLI source disposition projection:")), b"")
            source_frames = re.search(rb"sourceDwgSourceFrames=(\d+)", projection)
            published_frames = re.search(rb"sourceDwgFramesPublished=(\d+)",
                                          projection)
            if (strict.returncode != 1 or strict_output.read_bytes() != SENTINEL
                    or raw_source.read_bytes() != raw_source_bytes
                    or any(field not in projection for field in raw_only_projection)
                    or not source_frames or not published_frames
                    or source_frames.group(1) != published_frames.group(1)
                    or source_frames.group(1) != frame_count
                    or b"strict-complete conversion requires successful, complete DWG read disposition"
                       not in strict.stderr):
                raise AssertionError(
                    f"strict {source_version} {encoding} output did not refuse "
                    f"raw-only TABLECONTENT atomically: {strict!r}")
            checked += 1

            partial_output = root / (
                f"raw-only TABLECONTENT partial {source_version} {encoding}.dxf")
            partial_args = [str(converter), str(raw_source), "-o",
                            str(partial_output), f"-v{target_year}", "-y", "-d",
                            "--allow-partial-dwg-read"]
            if binary:
                partial_args.append("-b")
            partial = run_bounded(partial_args, 10, 2 * 1024 * 1024, cwd=root)
            payload = partial_output.read_bytes() if partial_output.is_file() else b""
            projection = next((line for line in partial.stderr.splitlines()
                               if line.startswith(
                                   b"CLI source disposition projection:")), b"")
            source_frames = re.search(rb"sourceDwgSourceFrames=(\d+)", projection)
            published_frames = re.search(rb"sourceDwgFramesPublished=(\d+)",
                                          projection)
            if (partial.returncode != 0 or b"Success:" not in partial.stdout
                    or b"WARNING: explicit partial DWG read mode" not in partial.stderr
                    or b"is not a completeness/support claim" not in partial.stderr
                    or any(field not in projection for field in raw_only_projection)
                    or not source_frames or not published_frames
                    or source_frames.group(1) != published_frames.group(1)
                    or source_frames.group(1) != frame_count
                    or not output_has_revision(payload, target_revision, binary)
                    or b"TABLECONTENT" in payload
                    or raw_source.read_bytes() != raw_source_bytes):
                raise AssertionError(
                    f"explicit {source_version} {encoding} partial mode did not "
                    f"diagnose and omit raw-only TABLECONTENT: {partial!r}")
            checked += 1

            parsed = run_bounded(
                [str(writer), "--count-dxf-lines", str(partial_output)],
                10, 131072, cwd=root)
            if (parsed.returncode != 0 or b"lineCount=1" not in parsed.stdout
                    or b"vportCount=1" not in parsed.stdout
                    or partial_output.read_bytes() != payload
                    or raw_source.read_bytes() != raw_source_bytes):
                raise AssertionError(
                    f"raw-only {source_version} {encoding} partial output failed "
                    f"direct DXF readback: {parsed!r}")
            checked += 1
    return checked


def check_historical_ac1021_role_refusal(converter, root):
    """Keep the known AC1021 ENC role defects visible and fail closed."""
    source = (Path(__file__).resolve().parents[1] / "tests" / "fixtures"
              / "dwg" / "ordinary_enc_AC1021.dwg")
    output = root / "historical AC1021 strict destination.dxf"
    output.write_bytes(SENTINEL)
    original = source.read_bytes()
    result = run_bounded(
        [str(converter), str(source), "-o", str(output), "-v2010", "-y", "-d"],
        10, 2 * 1024 * 1024, cwd=root)
    role_rows = [line for line in result.stderr.splitlines()
                 if b"kind=42" in line]
    if (result.returncode != 1 or output.read_bytes() != SENTINEL
            or source.read_bytes() != original
            or b"sourceDwgFrameCoverage=complete" not in result.stderr
            or b"sourceDwgSourceFrames=26" not in result.stderr
            or b"sourceDwgEntityParseFailures=0" not in result.stderr
            or b"sourceDwgIntegrityRetainedRows=16" not in result.stderr
            or b"sourceDwgIntegrityErrorsAmongRetained=7" not in result.stderr
            or b"sourceDwgIntegrityWarningsAmongRetained=9" not in result.stderr
            or b"sourceDwgIntegrityExtractionOmitted=9" not in result.stderr
            or b"sourceDwgIntegrityUpstreamDroppedLowerBound=0" not in result.stderr
            or b"sourceDwgIntegrityAggregateSaturated=false" not in result.stderr
            or len(role_rows) != 4
            or any(b"logicalHandle=23" not in line
                   or b"expected=3 hasObserved=1 observed=4" not in line
                   for line in role_rows)
            or b"kind=39 offsetSpace=0" not in result.stderr
            or b"logicalHandle=18 hasExpected=1 expected=5 hasObserved=1 observed=4"
               not in result.stderr
            or b"strict-complete conversion requires successful, complete DWG read disposition"
            not in result.stderr):
        raise AssertionError(
            "historical AC1021 role defects were not retained while strict output "
            f"remained atomic: {result!r}")
    return 1


def check_source_disposition_projection(converter, root, writer):
    """Expose repair, skip, decoder-issue and required-version decisions."""
    checked = 0

    malformed_header = root / "header repair required.dxf"
    malformed_output = root / "header repair destination.dxf"
    malformed_bytes = SOURCE.replace(
        b"0\nENDSEC\n0\nSECTION\n2\nENTITIES",
        b"9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n0\nSECTION\n2\nENTITIES", 1)
    malformed_header.write_bytes(malformed_bytes)
    malformed_output.write_bytes(SENTINEL)
    malformed = run_bounded(
        [str(converter), str(malformed_header), "-o", str(malformed_output),
         "-y", "-d"], 10, 131072, cwd=root)
    if (malformed.returncode != 1 or malformed_output.read_bytes() != SENTINEL
            or malformed_header.read_bytes() != malformed_bytes
            or b"sourceRepairAction=none" not in malformed.stderr
            or b"sourceHeaderStructure=repair-required" not in malformed.stderr
            or b"sourceDwgEntityParseFailures=not-applicable" not in malformed.stderr
            or b"sourceDwgFrameCoverage=not-applicable" not in malformed.stderr
            or b"sourceDwgFramesFailed=not-applicable" not in malformed.stderr):
        raise AssertionError(
            f"malformed HEADER was not projected as repair-required: {malformed!r}")
    checked += 1

    valid_ltscale = root / "valid LTSCALE header.dxf"
    valid_ltscale_output = root / "valid LTSCALE header output.dxf"
    valid_ltscale_bytes = SOURCE.replace(
        b"0\nENDSEC\n0\nSECTION\n2\nENTITIES",
        b"9\n$LTSCALE\n40\n2.0\n0\nENDSEC\n0\nSECTION\n2\nENTITIES", 1)
    valid_ltscale.write_bytes(valid_ltscale_bytes)
    admitted_ltscale = run_bounded(
        [str(converter), str(valid_ltscale), "-o", str(valid_ltscale_output),
         "-y", "-d"], 10, 131072, cwd=root)
    valid_ltscale_output_bytes = (valid_ltscale_output.read_bytes()
                                  if valid_ltscale_output.is_file() else b"")
    if (admitted_ltscale.returncode != 0
            or valid_ltscale.read_bytes() != valid_ltscale_bytes
            or b"$LTSCALE" not in valid_ltscale_output_bytes
            or b"sourceHeaderStructure=ready" not in admitted_ltscale.stderr
            or b"sourceHeaderInitialPreflight=admitted" not in admitted_ltscale.stderr):
        raise AssertionError(
            f"valid LTSCALE HEADER was not admitted by preflight: {admitted_ltscale!r}")
    checked += 1

    wrong_type_ltscale = root / "wrong type LTSCALE header.dxf"
    wrong_type_ltscale_output = root / "wrong type LTSCALE destination.dxf"
    wrong_type_ltscale_bytes = SOURCE.replace(
        b"0\nENDSEC\n0\nSECTION\n2\nENTITIES",
        b"9\n$LTSCALE\n70\n2\n0\nENDSEC\n0\nSECTION\n2\nENTITIES", 1)
    wrong_type_ltscale.write_bytes(wrong_type_ltscale_bytes)
    wrong_type_ltscale_output.write_bytes(SENTINEL)
    refused_ltscale = run_bounded(
        [str(converter), str(wrong_type_ltscale), "-o",
         str(wrong_type_ltscale_output), "-y", "-d"], 10, 131072, cwd=root)
    if (refused_ltscale.returncode != 1
            or wrong_type_ltscale.read_bytes() != wrong_type_ltscale_bytes
            or wrong_type_ltscale_output.read_bytes() != SENTINEL
            or b"sourceHeaderStructure=ready" not in refused_ltscale.stderr
            or b"sourceHeaderInitialPreflight=refused" not in refused_ltscale.stderr):
        raise AssertionError(
            "wrong-type LTSCALE was not distinguished from HEADER structure "
            f"while preserving source/destination: {refused_ltscale!r}")
    checked += 1

    checked += check_historical_ac1021_role_refusal(converter, root)

    repeated_group_header = root / "repeated HEADER group code.dxf"
    repeated_group_output = root / "repeated HEADER group destination.dxf"
    repeated_group_bytes = SOURCE.replace(
        b"0\nENDSEC\n0\nSECTION\n2\nENTITIES",
        b"9\n$INSBASE\n10\n0\n20\n0\n30\n0\n10\n1\n"
        b"0\nENDSEC\n0\nSECTION\n2\nENTITIES", 1)
    repeated_group_header.write_bytes(repeated_group_bytes)
    repeated_group_output.write_bytes(SENTINEL)
    repeated_group = run_bounded(
        [str(converter), str(repeated_group_header), "-o",
         str(repeated_group_output), "-y", "-d"], 10, 131072, cwd=root)
    if (repeated_group.returncode != 1
            or repeated_group_output.read_bytes() != SENTINEL
            or repeated_group_header.read_bytes() != repeated_group_bytes
            or b"sourceRepairAction=none" not in repeated_group.stderr
            or b"sourceHeaderStructure=repair-required" not in repeated_group.stderr):
        raise AssertionError(
            "repeated group code inside one HEADER variable was not refused "
            f"atomically: {repeated_group!r}")
    checked += 1

    additional_header_profiles = {
        "duplicate LTSCALE": (
            b"9\n$LTSCALE\n40\n1.0\n9\n$LTSCALE\n40\n2.0\n"),
        "repeated EXTMIN component": (
            b"9\n$EXTMIN\n10\n0.0\n10\n1.0\n20\n0.0\n30\n0.0\n"),
    }
    for index, (description, fields) in enumerate(additional_header_profiles.items()):
        header = root / f"{description} header structure.dxf"
        output = root / f"{description} header destination.dxf"
        header_bytes = SOURCE.replace(
            b"0\nENDSEC\n0\nSECTION\n2\nENTITIES",
            fields + b"0\nENDSEC\n0\nSECTION\n2\nENTITIES", 1)
        header.write_bytes(header_bytes)
        output.write_bytes(SENTINEL)
        result = run_bounded(
            [str(converter), str(header), "-o", str(output), "-y", "-d"],
            10, 131072, cwd=root)
        if (result.returncode != 1 or output.read_bytes() != SENTINEL
                or header.read_bytes() != header_bytes
                or b"sourceRepairAction=none" not in result.stderr
                or b"sourceHeaderStructure=repair-required" not in result.stderr):
            raise AssertionError(
                f"{description} HEADER shape was not reported/refused atomically: "
                f"{result!r}")
        checked += 1

    required_versions = root / "required versions mask.dxf"
    required_versions_output = root / "required versions destination.dxf"
    required_versions_bytes = SOURCE.replace(
        b"0\nENDSEC\n0\nSECTION\n2\nENTITIES",
        b"9\n$REQUIREDVERSIONS\n160\n1\n0\nENDSEC\n0\nSECTION\n2\nENTITIES", 1)
    required_versions.write_bytes(required_versions_bytes)
    required_versions_output.write_bytes(SENTINEL)
    masked = run_bounded(
        [str(converter), str(required_versions), "-o", str(required_versions_output),
         "-v2010", "-y", "-d"], 10, 131072, cwd=root)
    if (masked.returncode != 1 or required_versions_output.read_bytes() != SENTINEL
            or required_versions.read_bytes() != required_versions_bytes
            or b"sourceRequiredVersionsStatus=present" not in masked.stderr
            or b"sourceRequiredVersionsMask=0x0000000000000001" not in masked.stderr
            or b"sourceRequiredVersionsOutput=refused" not in masked.stderr):
        raise AssertionError(
            f"required-version feature mask was not projected atomically: {masked!r}")
    checked += 1

    admitted_output = root / "required versions admitted AC1032.dxf"
    admitted = run_bounded(
        [str(converter), str(required_versions), "-o", str(admitted_output),
         "-v2018", "-y", "-d"], 10, 131072, cwd=root)

    def has_required_versions_mask(payload):
        lines = [line.strip() for line in payload.splitlines()]
        return sum(
            lines[index:index + 4]
            == [b"9", b"$REQUIREDVERSIONS", b"160", b"1"]
            for index in range(max(0, len(lines) - 3))) == 1

    def has_header_boolean(payload, name, expected):
        lines = [line.strip() for line in payload.splitlines()]
        marker = [b"9", name.encode("ascii"), b"290", str(expected).encode("ascii")]
        return sum(lines[index:index + 4] == marker
                   for index in range(max(0, len(lines) - 3))) == 1

    admitted_bytes = (admitted_output.read_bytes()
                      if admitted_output.is_file() else b"")
    if (admitted.returncode != 0
            or required_versions.read_bytes() != required_versions_bytes
            or b"sourceRequiredVersionsStatus=present" not in admitted.stderr
            or b"sourceRequiredVersionsMask=0x0000000000000001" not in admitted.stderr
            or b"sourceRequiredVersionsOutput=admitted" not in admitted.stderr
            or not output_has_revision(admitted_bytes, b"AC1032", False)
            or not has_required_versions_mask(admitted_bytes)
            or not has_header_boolean(admitted_bytes, "$PSTYLEMODE", 1)
            or not has_header_boolean(admitted_bytes, "$EXTNAMES", 1)
            or not has_header_boolean(admitted_bytes, "$OLESTARTUP", 0)):
        raise AssertionError(
            f"supported required-version mask did not survive AC1032 output: {admitted!r}")

    admitted_readback = root / "required versions admitted readback.dxf"
    reread = run_bounded(
        [str(converter), str(admitted_output), "-o", str(admitted_readback),
         "-v2018", "-y", "-d"], 10, 131072, cwd=root)
    reread_bytes = (admitted_readback.read_bytes()
                    if admitted_readback.is_file() else b"")
    if (reread.returncode != 0
            or b"sourceRequiredVersionsStatus=present" not in reread.stderr
            or b"sourceRequiredVersionsMask=0x0000000000000001" not in reread.stderr
            or b"sourceRequiredVersionsOutput=admitted" not in reread.stderr
            or not has_required_versions_mask(reread_bytes)
            or admitted_output.read_bytes() != admitted_bytes):
        raise AssertionError(
            f"supported required-version mask failed AC1032 readback: {reread!r}")
    checked += 2

    malformed_required_version_fields = {
        "duplicate variable": (
            b"9\n$REQUIREDVERSIONS\n160\n1\n"
            b"9\n$REQUIREDVERSIONS\n160\n2\n"),
        "repeated group 160": (
            b"9\n$REQUIREDVERSIONS\n160\n1\n160\n2\n"),
        "wrong group code": b"9\n$REQUIREDVERSIONS\n70\n1\n",
    }
    for index, (description, fields) in enumerate(
            malformed_required_version_fields.items()):
        malformed_source = root / f"malformed required versions {index}.dxf"
        malformed_output = root / f"malformed required versions destination {index}.dxf"
        malformed_bytes = SOURCE.replace(
            b"0\nENDSEC\n0\nSECTION\n2\nENTITIES",
            fields + b"0\nENDSEC\n0\nSECTION\n2\nENTITIES", 1)
        malformed_source.write_bytes(malformed_bytes)
        malformed_output.write_bytes(SENTINEL)
        malformed_mask = run_bounded(
            [str(converter), str(malformed_source), "-o", str(malformed_output),
             "-v2018", "-y", "-d"], 10, 131072, cwd=root)
        if (malformed_mask.returncode != 1
                or malformed_output.read_bytes() != SENTINEL
                or malformed_source.read_bytes() != malformed_bytes
                or b"sourceRequiredVersionsStatus=malformed" not in malformed_mask.stderr
                or b"sourceRequiredVersionsMask=unknown" not in malformed_mask.stderr
                or b"sourceRequiredVersionsOutput=refused" not in malformed_mask.stderr):
            raise AssertionError(
                f"{description} REQUIREDVERSIONS was not diagnosed and refused "
                f"atomically: {malformed_mask!r}")
        checked += 1

    issue_pairs = [
        (0, b"SECTION"), (2, b"HEADER"), (9, b"$ACADVER"), (1, b"AC1021"),
        (9, b"$DWGCODEPAGE"), (3, b"ANSI_1252"), (0, b"ENDSEC"),
        (0, b"SECTION"), (2, b"ENTITIES"), (0, b"TEXT"), (5, b"A0"),
        (8, b"0"), (10, 1.0), (20, 2.0), (40, 1.0), (1, b"\xc0\xaf"),
        (0, b"ENDSEC")]
    issue_source = root / "decoder issue mask.dxf"
    issue_output = root / "decoder issue destination.dxf"
    issue_bytes = encode_authored_pairs(issue_pairs, False)
    issue_source.write_bytes(issue_bytes)
    issue_output.write_bytes(SENTINEL)
    issue = run_bounded(
        [str(converter), str(issue_source), "-o", str(issue_output), "-y", "-d",
         "--dxf-codepage-policy=exact-named"], 10, 131072, cwd=root)
    projection = next((line for line in issue.stderr.splitlines()
                       if line.startswith(b"CLI source disposition projection:")), b"")
    issue_mask = next((field.split(b"=", 1)[1] for field in projection.split()
                       if field.startswith(b"sourceDxfTextIssueMask=")), b"0x00000000")
    if (issue.returncode != 1 or issue_output.read_bytes() != SENTINEL
            or issue_source.read_bytes() != issue_bytes
            or b"inputCodePagePolicy=exact-named" not in projection
            or issue_mask != b"0x0000000000000820"):
        raise AssertionError(
            f"DXF invalid UTF-8 issue mask/policy was not projected exactly: {issue!r}")
    checked += 1

    escape_pairs = [
        (0, b"SECTION"), (2, b"HEADER"), (9, b"$ACADVER"), (1, b"AC1021"),
        (9, b"$DWGCODEPAGE"), (3, b"ANSI_1252"), (0, b"ENDSEC"),
        (0, b"SECTION"), (2, b"ENTITIES"), (0, b"TEXT"), (5, b"A0"),
        (8, b"0"), (10, 1.0), (20, 2.0), (40, 1.0), (1, b"\\U+xyz0"),
        (0, b"ENDSEC")]
    escape_source = root / "decoder malformed escape.dxf"
    escape_output = root / "decoder malformed escape destination.dxf"
    escape_bytes = encode_authored_pairs(escape_pairs, False)
    escape_source.write_bytes(escape_bytes)
    escape_output.write_bytes(SENTINEL)
    escaped = run_bounded(
        [str(converter), str(escape_source), "-o", str(escape_output), "-y", "-d",
         "--dxf-codepage-policy=exact-named"], 10, 131072, cwd=root)
    projection = next((line for line in escaped.stderr.splitlines()
                       if line.startswith(b"CLI source disposition projection:")), b"")
    escape_mask = next((field.split(b"=", 1)[1] for field in projection.split()
                        if field.startswith(b"sourceDxfTextIssueMask=")), b"")
    if (escaped.returncode != 1 or escape_output.read_bytes() != SENTINEL
            or escape_source.read_bytes() != escape_bytes
            or b"inputCodePagePolicy=exact-named" not in projection
            or escape_mask != b"0x0000000000000600"):
        raise AssertionError(
            f"DXF malformed-escape issue mask was not projected exactly: {escaped!r}")
    checked += 1

    undefined_pairs = [
        (0, b"SECTION"), (2, b"HEADER"), (9, b"$ACADVER"), (1, b"AC1015"),
        (9, b"$DWGCODEPAGE"), (3, b"ANSI_1252"), (0, b"ENDSEC"),
        (0, b"SECTION"), (2, b"ENTITIES"), (0, b"TEXT"), (5, b"A0"),
        (8, b"0"), (10, 1.0), (20, 2.0), (40, 1.0), (1, b"L\x81R"),
        (0, b"ENDSEC")]
    undefined_source = root / "decoder undefined legacy byte.dxf"
    undefined_output = root / "decoder undefined legacy destination.dxf"
    undefined_bytes = encode_authored_pairs(undefined_pairs, False)
    undefined_source.write_bytes(undefined_bytes)
    undefined_output.write_bytes(SENTINEL)
    undefined = run_bounded(
        [str(converter), str(undefined_source), "-o", str(undefined_output),
         "-y", "-d", "--dxf-codepage-policy=exact-named"],
        10, 131072, cwd=root)
    projection = next((line for line in undefined.stderr.splitlines()
                       if line.startswith(b"CLI source disposition projection:")), b"")
    undefined_mask = next((field.split(b"=", 1)[1] for field in projection.split()
                           if field.startswith(b"sourceDxfTextIssueMask=")), b"")
    if (undefined.returncode != 1 or undefined_output.read_bytes() != SENTINEL
            or undefined_source.read_bytes() != undefined_bytes
            or b"inputCodePagePolicy=exact-named" not in projection
            or undefined_mask != b"0x0000000000000140"):
        raise AssertionError(
            "DXF undefined legacy-byte issue mask was not projected exactly: "
            f"{undefined!r}")
    checked += 1

    exact_issue_cases = [
        ("unrepresented MIF", b"AC1015", b"ANSI_1252", b"\\M+48861",
         "--dxf-codepage-policy=exact-named", b"exact-named",
         b"0x0000000000000400"),
        ("invalid scalar CIF", b"AC1015", b"ANSI_1252", b"\\U+D800",
         "--dxf-codepage-policy=exact-named", b"exact-named",
         b"0x0000000000000c00"),
        ("unknown codepage with exact mapping", b"AC1015", b"unknown-local", b"\xe9",
         "--dxf-codepage-policy=exact-named", b"exact-named",
         b"0x0000000000000084"),
        ("unspecified codepage", b"AC1015", None, b"\xe9",
         "--dxf-codepage-policy=exact-named", b"exact-named",
         b"0x0000000000000001"),
        ("legacy UTF-16 profile", b"AC1015", b"UTF16", b"AB",
         "--dxf-codepage-policy=exact-named", b"exact-named",
         b"0x0000000000005100"),
        ("legacy UTF-8 profile", b"AC1015", b"UTF8", b"\xc3\xa9",
         "--dxf-codepage-policy=exact-named", b"exact-named",
         b"0x0000000000004000"),
        ("compatibility codepage alias", b"AC1015", b"ISO8859-4", b"L\xa1R",
         "--dxf-codepage-policy=compatible", b"compatible",
         b"0x0000000000000142"),
        ("unspecified source revision", None, b"ANSI_1252", b"L\xe9R",
         "--dxf-codepage-policy=exact-named", b"exact-named",
         b"0x0000000000002000"),
        ("unknown codepage with compatible mapping", b"AC1015", b"unknown-local", b"\xe9",
         "--dxf-codepage-policy=compatible", b"compatible",
         b"0x0000000000000008"),
        ("malformed legacy UTF-16", b"AC1015", b"UTF16", b"\x00\xd8",
         "--dxf-codepage-policy=exact-named", b"exact-named",
         b"0x0000000000005180"),
        ("reapplied UTF-8 codepage", b"AC1015", b"UTF8", b"\xc3\xa9",
         "--dxf-codepage-policy=compatible", b"compatible",
         b"0x0000000000000010"),
    ]
    for index, (description, revision, page, value, policy_option,
                expected_policy, expected_mask) in enumerate(exact_issue_cases):
        pairs = [(0, b"SECTION"), (2, b"HEADER")]
        if revision is not None:
            pairs.extend([(9, b"$ACADVER"), (1, revision)])
        if page is not None:
            pairs.extend([(9, b"$DWGCODEPAGE"), (3, page)])
        pairs.extend([
            (0, b"ENDSEC"), (0, b"SECTION"), (2, b"ENTITIES"),
            (0, b"TEXT"), (5, b"A0"), (8, b"0"), (10, 1.0),
            (20, 2.0), (40, 1.0), (1, value), (0, b"ENDSEC")])
        source = root / f"decoder exact issue {index}.dxf"
        output = root / f"decoder exact issue destination {index}.dxf"
        original = encode_authored_pairs(pairs, False)
        source.write_bytes(original)
        output.write_bytes(SENTINEL)
        result = run_bounded(
            [str(converter), str(source), "-o", str(output), "-y", "-d",
             policy_option], 10, 131072, cwd=root)
        projection = next((line for line in result.stderr.splitlines()
                           if line.startswith(
                               b"CLI source disposition projection:")), b"")
        issue_mask = next((field.split(b"=", 1)[1] for field in projection.split()
                           if field.startswith(b"sourceDxfTextIssueMask=")), b"")
        if (result.returncode != 1 or output.read_bytes() != SENTINEL
                or source.read_bytes() != original
                or b"inputCodePagePolicy=" + expected_policy not in projection
                or issue_mask != expected_mask):
            raise AssertionError(
                f"{description} DXF issue mask was not projected exactly/atomically: "
                f"{result!r}")
    checked += len(exact_issue_cases)

    checked += check_skipped_record_projection(converter, root, writer)
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
    parser.add_argument("--partial-read-only", action="store_true",
                        help="run only the strict/explicit-partial DWG CLI controls")
    parser.add_argument("--section-settings-only", action="store_true",
                        help="run only the generated SECTION_SETTINGS recovery controls")
    parser.add_argument("--motionpath-recovery-only", action="store_true",
                        help="run only generated MOTIONPATH recovery controls")
    parser.add_argument("--navisworks-modeldef-recovery-only", action="store_true",
                        help="run only generated NAVISWORKSMODELDEF recovery controls")
    parser.add_argument("--section-manager-recovery-only", action="store_true",
                        help="run only generated SECTION_MANAGER recovery controls")
    parser.add_argument("--wipeoutvariables-recovery-only", action="store_true",
                        help="run only generated WIPEOUTVARIABLES recovery controls")
    parser.add_argument("--rastervariables-recovery-only", action="store_true",
                        help="run only generated RASTERVARIABLES recovery controls")
    parser.add_argument("--pointcloud-colormap-only", action="store_true",
                        help="run only generated POINTCLOUDCOLORMAP recovery controls")
    parser.add_argument("--pointcloud-definition-only", action="store_true",
                        help="run only generated POINTCLOUDDEFINITION recovery controls")
    parser.add_argument("--pointcloud-reactor-only", action="store_true",
                        help="run only generated POINTCLOUDDEFREACTOR recovery controls")
    parser.add_argument("--mleaderstyle-only", action="store_true",
                        help="run only generated MLEADERSTYLE recovery controls")
    parser.add_argument("--spatial-filter-only", action="store_true",
                        help="run only generated SPATIAL_FILTER recovery controls")
    parser.add_argument("--idbuffer-only", action="store_true",
                        help="run only generated IDBUFFER recovery controls")
    parser.add_argument("--layer-index-only", action="store_true",
                        help="run only generated LAYER_INDEX recovery controls")
    parser.add_argument("--spatial-index-only", action="store_true",
                        help="run only generated SPATIAL_INDEX recovery controls")
    parser.add_argument("--lightlist-only", action="store_true",
                        help="run only generated LIGHTLIST recovery controls")
    parser.add_argument("--geodata-only", action="store_true",
                        help="run only generated GEODATA recovery controls")
    parser.add_argument("--tablestyle-only", action="store_true",
                        help="run only generated TABLESTYLE recovery controls")
    parser.add_argument("--field-recovery-only", action="store_true",
                        help="run only generated FIELD recovery controls")
    parser.add_argument("--fieldlist-recovery-only", action="store_true",
                        help="run only generated FIELDLIST recovery controls")
    parser.add_argument("--evaluationgraph-recovery-only", action="store_true",
                        help="run only generated evaluation-graph recovery controls")
    parser.add_argument("--dimassoc-recovery-only", action="store_true",
                        help="run only generated DIMASSOC recovery controls")
    parser.add_argument("--layout-recovery-only", action="store_true",
                        help="run only generated LAYOUT recovery controls")
    parser.add_argument("--source-disposition-only", action="store_true",
                        help="run only CLI repair/skip/mask preflight projections")
    parser.add_argument("--historical-ac1021-only", action="store_true",
                        help="run only the tracked historical AC1021 role/refusal check")
    parser.add_argument("--skipped-record-only", action="store_true",
                        help="run only generated skipped-record disposition controls")
    parser.add_argument("--sunstudy-observation-only", action="store_true",
                        help="run only explicit DXF source observation CLI controls")
    parser.add_argument("--sunstudy-recovery-only", action="store_true",
                        help="run only generated legacy SUNSTUDY recovery controls")
    parser.add_argument("--sun-only", action="store_true",
                        help="run only generated SUN recovery controls")
    args = parser.parse_args()
    converter = args.converter.resolve(strict=True)
    # Child processes run in a temporary directory, so resolve a caller's
    # relative writer path before changing their working directory.
    if args.writer is not None:
        args.writer = args.writer.resolve(strict=True)
    with tempfile.TemporaryDirectory(prefix="libdxfrw-cli-contract-") as directory:
        root = Path(directory)
        if args.historical_ac1021_only:
            checked = check_historical_ac1021_role_refusal(converter, root)
            print(f"CLI historical AC1021 roles: PASS ({checked} strict-refusal control)")
            return
        if args.sunstudy_observation_only:
            checked = check_sunstudy_observation_cli(converter, root)
            print(f"SUNSTUDY observation CLI: PASS ({checked} route/refusal controls)")
            return
        if args.sunstudy_recovery_only:
            checked = check_malformed_sunstudy_body(
                converter, root, args.writer)
            print(f"CLI SUNSTUDY recovery: PASS ({checked} controls)")
            return
        if args.sun_only:
            checked = check_malformed_sun_body(converter, root, args.writer)
            print(f"CLI SUN recovery: PASS ({checked} controls)")
            return
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
        if args.skipped_record_only:
            checked = check_skipped_record_projection(converter, root, args.writer)
            print(f"CLI skipped-record disposition: PASS ({checked} version/profile controls)")
            return
        if args.section_settings_only:
            checked = check_malformed_section_settings_body(
                converter, root, args.writer)
            print(f"CLI SECTION_SETTINGS recovery: PASS ({checked} controls)")
            return
        if args.motionpath_recovery_only:
            checked = check_malformed_motionpath_body(
                converter, root, args.writer)
            print(f"CLI MOTIONPATH recovery: PASS ({checked} controls)")
            return
        if args.navisworks_modeldef_recovery_only:
            checked = check_malformed_navisworks_modeldef_body(
                converter, root, args.writer)
            print(f"CLI NAVISWORKSMODELDEF recovery: PASS ({checked} controls)")
            return
        if args.section_manager_recovery_only:
            checked = check_malformed_section_manager_body(
                converter, root, args.writer)
            print(f"CLI SECTION_MANAGER recovery: PASS ({checked} controls)")
            return
        if args.wipeoutvariables_recovery_only:
            checked = check_malformed_wipeoutvariables_body(
                converter, root, args.writer)
            print(f"CLI WIPEOUTVARIABLES recovery: PASS ({checked} controls)")
            return
        if args.rastervariables_recovery_only:
            checked = check_malformed_rastervariables_body(
                converter, root, args.writer)
            print(f"CLI RASTERVARIABLES recovery: PASS ({checked} controls)")
            return
        if args.pointcloud_colormap_only:
            checked = check_malformed_pointcloud_colormap_body(
                converter, root, args.writer)
            print(f"CLI POINTCLOUDCOLORMAP recovery: PASS ({checked} controls)")
            return
        if args.pointcloud_definition_only:
            checked = check_malformed_pointcloud_definition_bodies(
                converter, root, args.writer)
            print(f"CLI POINTCLOUDDEFINITION recovery: PASS ({checked} controls)")
            return
        if args.pointcloud_reactor_only:
            checked = check_malformed_pointcloud_reactor_bodies(
                converter, root, args.writer)
            print(f"CLI POINTCLOUDDEFREACTOR recovery: PASS ({checked} controls)")
            return
        if args.mleaderstyle_only:
            checked = check_malformed_mleaderstyle_body(
                converter, root, args.writer)
            print(f"CLI MLEADERSTYLE recovery: PASS ({checked} controls)")
            return
        if args.spatial_filter_only:
            checked = check_malformed_spatial_filter_body(
                converter, root, args.writer)
            print(f"CLI SPATIAL_FILTER recovery: PASS ({checked} controls)")
            return
        if args.idbuffer_only:
            checked = check_malformed_idbuffer_body(
                converter, root, args.writer)
            print(f"CLI IDBUFFER recovery: PASS ({checked} controls)")
            return
        if args.layer_index_only:
            checked = check_malformed_layer_index_body(
                converter, root, args.writer)
            print(f"CLI LAYER_INDEX recovery: PASS ({checked} controls)")
            return
        if args.spatial_index_only:
            checked = check_malformed_spatial_index_body(
                converter, root, args.writer)
            print(f"CLI SPATIAL_INDEX recovery: PASS ({checked} controls)")
            return
        if args.lightlist_only:
            checked = check_malformed_lightlist_body(
                converter, root, args.writer)
            print(f"CLI LIGHTLIST recovery: PASS ({checked} controls)")
            return
        if args.geodata_only:
            checked = check_malformed_geodata_body(
                converter, root, args.writer)
            print(f"CLI GEODATA recovery: PASS ({checked} controls)")
            return
        if args.tablestyle_only:
            checked = check_malformed_tablestyle_body(
                converter, root, args.writer)
            print(f"CLI TABLESTYLE recovery: PASS ({checked} controls)")
            return
        if args.field_recovery_only:
            checked = check_malformed_field_body(
                converter, root, args.writer)
            print(f"CLI FIELD recovery: PASS ({checked} controls)")
            return
        if args.fieldlist_recovery_only:
            checked = check_malformed_fieldlist_body(
                converter, root, args.writer)
            print(f"CLI FIELDLIST recovery: PASS ({checked} controls)")
            return
        if args.evaluationgraph_recovery_only:
            checked = check_malformed_evaluationgraph_body(
                converter, root, args.writer)
            print(f"CLI evaluation-graph recovery: PASS ({checked} controls)")
            return
        if args.dimassoc_recovery_only:
            checked = check_malformed_dimassoc_body(
                converter, root, args.writer)
            print(f"CLI DIMASSOC recovery: PASS ({checked} controls)")
            return
        if args.layout_recovery_only:
            checked = check_malformed_layout_body(
                converter, root, args.writer)
            print(f"CLI LAYOUT recovery: PASS ({checked} controls)")
            return
        if args.partial_read_only:
            checked = check_partial_dwg_read_cli(converter, root, args.writer)
            vba_checked = check_malformed_vba_project_body(
                converter, root, args.writer)
            block_representation_checked = check_malformed_blockrepresentation_body(
                converter, root, args.writer)
            group_checked = check_malformed_group_body(
                converter, root, args.writer)
            dictionary_checked = check_malformed_dictionary_body(
                converter, root, args.writer)
            vxcontrol_checked = check_malformed_vxcontrol_body(
                converter, root, args.writer)
            vxtablerecord_checked = check_malformed_vxtablerecord_body(
                converter, root, args.writer)
            tvdeviceproperties_checked = check_malformed_tvdeviceproperties_body(
                converter, root, args.writer)
            pointpath_checked = check_malformed_pointpath_body(
                converter, root, args.writer)
            partialviewingindex_checked = check_malformed_partialviewingindex_body(
                converter, root, args.writer)
            curvepath_checked = check_malformed_curvepath_body(
                converter, root, args.writer)
            plotsettings_checked = check_malformed_plotsettings_body(
                converter, root, args.writer)
            underlaydefinition_checked = check_malformed_underlaydefinition_body(
                converter, root, args.writer)
            material_checked = check_malformed_material_body(
                converter, root, args.writer)
            visualstyle_checked = check_malformed_visualstyle_body(
                converter, root, args.writer)
            rastervariables_checked = check_malformed_rastervariables_body(
                converter, root, args.writer)
            wipeoutvariables_checked = check_malformed_wipeoutvariables_body(
                converter, root, args.writer)
            background_checked = check_malformed_background_solid_body(
                converter, root, args.writer)
            background_groundplane_checked = (
                check_malformed_background_groundplane_body(
                    converter, root, args.writer))
            background_gradient_checked = (
                check_malformed_background_gradient_body(
                    converter, root, args.writer))
            background_image_checked = check_malformed_background_image_body(
                converter, root, args.writer)
            background_ibl_checked = check_malformed_background_ibl_body(
                converter, root, args.writer)
            background_skylight_checked = (
                check_malformed_background_skylight_handle(
                    converter, root, args.writer))
            motionpath_checked = check_malformed_motionpath_body(
                converter, root, args.writer)
            sunstudy_checked = check_malformed_sunstudy_body(
                converter, root, args.writer)
            navisworks_modeldef_checked = (
                check_malformed_navisworks_modeldef_body(
                    converter, root, args.writer))
            section_manager_checked = check_malformed_section_manager_body(
                converter, root, args.writer)
            section_settings_checked = check_malformed_section_settings_body(
                converter, root, args.writer)
            render_settings_checked = check_malformed_rendersettings_bodies(
                converter, root, args.writer)
            render_settings_count = sum(render_settings_checked.values())
            print(f"CLI partial DWG read: PASS ({checked} strict/partial/readback "
                  f"controls, {vba_checked} VBA_PROJECT, "
                  f"{block_representation_checked} BLOCKREPRESENTATION_DATA, "
                  f"{group_checked} GROUP, {dictionary_checked} DICTIONARY, "
                  f"{vxcontrol_checked} VX_CONTROL, "
                  f"{vxtablerecord_checked} VX_TABLE_RECORD, "
                  f"{tvdeviceproperties_checked} TV_DEVICE_PROPERTIES, "
                  f"{pointpath_checked} POINTPATH, "
                  f"{partialviewingindex_checked} PARTIAL_VIEWING_INDEX, "
                  f"{curvepath_checked} CURVEPATH, "
                  f"{plotsettings_checked} PLOTSETTINGS, "
                  f"{underlaydefinition_checked} UNDERLAYDEFINITION, "
                  f"{material_checked} MATERIAL, "
                  f"{visualstyle_checked} VISUALSTYLE, "
                  f"{rastervariables_checked} RASTERVARIABLES, "
                  f"{wipeoutvariables_checked} WIPEOUTVARIABLES, "
                  f"{background_checked} SOLIDBACKGROUND, "
                  f"{background_groundplane_checked} GROUNDPLANEBACKGROUND, "
                  f"{background_gradient_checked} GRADIENTBACKGROUND, "
                  f"{background_image_checked} IMAGEBACKGROUND, "
                  f"{background_ibl_checked} IBLBACKGROUND, "
                  f"{background_skylight_checked} SKYLIGHTBACKGROUND, "
                  f"{motionpath_checked} MOTIONPATH, "
                  f"{sunstudy_checked} SUNSTUDY, "
                  f"{navisworks_modeldef_checked} NAVISWORKSMODELDEF, "
                  f"{section_manager_checked} SECTION_MANAGER, "
                  f"{section_settings_checked} SECTION_SETTINGS, and "
                  f"{render_settings_count} RENDERSETTINGS controls across "
                  f"Settings ({render_settings_checked['Settings']}), "
                  f"Global ({render_settings_checked['Global']}), "
                  f"Environment ({render_settings_checked['Environment']}), "
                  f"Entry ({render_settings_checked['Entry']}), "
                  f"RapidRT ({render_settings_checked['RapidRT']}), and "
                  f"MentalRay ({render_settings_checked['MentalRay']}) subtypes)")
            return
        if args.source_disposition_only:
            checked = check_source_disposition_projection(converter, root, args.writer)
            print(f"CLI source disposition projection: PASS ({checked} repair/skip/mask controls)")
            return
        if args.fast_only:
            check_sunstudy_observation_cli(converter, root)
            checked = check_options(converter, root)
            batch_checked = check_batch(converter, root, args.writer)
            observation_checked = check_read_observations(converter, root)
            partial_read_checked = check_partial_dwg_read_cli(
                converter, root, args.writer)
            disposition_checked = check_source_disposition_projection(
                converter, root, args.writer)
            print(f"CLI fast contract: PASS ({checked} option controls, "
                  f"{batch_checked} batch controls, {observation_checked} "
                  f"read-observation controls, {partial_read_checked} "
                  f"partial-read controls, {disposition_checked} "
                  "source-disposition controls)")
            return
        check_sunstudy_observation_cli(converter, root)
        checked = check_options(converter, root)
        batch_checked = check_batch(converter, root, args.writer)
        observation_checked = check_read_observations(converter, root)
        partial_read_checked = check_partial_dwg_read_cli(
            converter, root, args.writer)
        disposition_checked = check_source_disposition_projection(
            converter, root, args.writer)
        source_checked = check_source_text_admission(converter, root)
        target = check_target_text_transport(converter, root)
        embedded_mtext = check_embedded_mtext_downlevel(converter, root)
        proxy_graphics = check_proxy_graphics_loss(converter, root)
        pre_r13_markers = check_pre_r13_marker_failure(converter, root)
        policy = check_exact_input_policy(converter, root)
    print(f"CLI contract: PASS ({checked} option/filename and "
          f"{batch_checked} batch controls, {observation_checked} read-observation controls; "
          f"{partial_read_checked} partial-read controls, "
          f"{disposition_checked} source-disposition controls; "
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
