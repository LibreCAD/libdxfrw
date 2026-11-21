#!/usr/bin/env python3
"""Generate an R13 drawing from scratch and test bounded partial recovery."""
import shutil
import struct
import sys
import tempfile
from pathlib import Path

from dwg2dxf_ac1014_partial_external import crc16, invoke, require, sha, SENTINEL

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools" / "research"))
from audit_dxf_dictionary_cloning import audit_bytes


def require_legacy_dictionary_presence(path):
    rows = audit_bytes(path.read_bytes(), include_rows=True)["dictionary_rows"]
    for handle in ("C", "D"):
        matches = [row for row in rows if row["handle"] == handle]
        require(len(matches) == 1 and matches[0]["cloning"] is None,
                "legacy root/group dictionaries must not invent group 281")


def make_fault(data):
    data = bytearray(data)
    require(data[:6] == b"AC1012" and data[2537:2539] == bytes([57, 0])
            and int.from_bytes(data[2596:2598], "little") == crc16(data[2537:2596]),
            "generated AC1012 LINE frame or CRC")
    base = 2539 * 8 + 77
    old = "".join(f"{b:08b}" for b in struct.pack("<d", 2.0))
    observed = "".join(str((data[(base + i) // 8] >> (7 - (base + i) % 8)) & 1)
                       for i in range(64))
    require(observed == old, "generated LINE coordinate span")
    new = "".join(f"{b:08b}" for b in struct.pack("<Q", 0x7FF8000000000000))
    for i, bit in enumerate(new):
        position = base + i
        mask = 0x80 >> (position % 8)
        data[position // 8] = (data[position // 8] | mask if bit == "1"
                               else data[position // 8] & ~mask)
    data[2596:2598] = crc16(data[2537:2596]).to_bytes(2, "little")
    return data


def main():
    if len(sys.argv) != 5:
        raise SystemExit("usage: script <dwgadd> <dwg2dxf> <profile-test> <recipe>")
    dwgadd, converter, profile_test, recipe = sys.argv[1:]
    version = invoke([dwgadd, "--version"])
    require(version.returncode == 0 and b"dwgadd 0.14" in version.stdout,
            "this source-profile control requires dwgadd 0.14")
    with tempfile.TemporaryDirectory(prefix="libdxfrw-ac1012-partial-") as directory:
        root = Path(directory)
        original = root / "generated.dwg"
        created = invoke([dwgadd, "--as", "r13", "-o", str(original), recipe])
        require(created.returncode == 0 and original.is_file(), "dwgadd generation")
        source_bytes = original.read_bytes()
        require(invoke([profile_test, "--r13-generated", str(original),
                        str(root / "r13-adapter.dxf")]).returncode == 0,
                "R13 strict refusal, destination preservation and partial readback")
        for binary in (False, True):
            output = root / ("r13-binary.dxf" if binary else "r13-ascii.dxf")
            command = [converter, str(original), "-o", str(output), "-y"]
            if binary:
                command.append("-b")
            output.write_bytes(SENTINEL)
            require(invoke(command).returncode == 1 and output.read_bytes() == SENTINEL,
                    "R13 strict output must remain refused")
            require(invoke(command + ["--allow-partial-dwg-read"]).returncode == 0
                    and invoke([profile_test, "--check-r14-generated", str(output)]).returncode == 0,
                    "R13 partial CLI conversion and fresh 3D coordinate readback")
            require_legacy_dictionary_presence(output)
        damaged = root / "controlled-failure.dwg"
        damaged.write_bytes(make_fault(source_bytes))
        print("source SHA-256:", sha(source_bytes), "fault SHA-256:", sha(damaged.read_bytes()))
        bad_crc = root / "invalid-frame-crc.dwg"
        crc_bytes = bytearray(damaged.read_bytes())
        crc_bytes[2547] ^= 1
        bad_crc.write_bytes(crc_bytes)
        expected = (b"sourceDwgEntityParseFailures=1", b"sourceDwgObjectParseFailures=0",
                    b"sourceDwgSourceFrames=33", b"sourceDwgFramesPublished=32",
                    b"sourceDwgFramesFailed=1", b"sourceDwgFramesUnresolved=0")
        for binary in (False, True):
            output = root / ("binary.dxf" if binary else "ascii.dxf")
            output.write_bytes(SENTINEL)
            command = [converter, str(damaged), "-o", str(output), "-y", "-d"]
            if binary:
                command.append("-b")
            strict = invoke(command)
            require(strict.returncode == 1 and output.read_bytes() == SENTINEL
                    and all(value in strict.stderr for value in expected),
                    "strict refusal and destination preservation")
            partial = invoke(command + ["--allow-partial-dwg-read"])
            require(partial.returncode == 0 and output.read_bytes() != SENTINEL
                    and all(value in partial.stderr for value in expected)
                    and b"explicit partial DWG read mode" in partial.stderr,
                    "partial conversion")
            output.write_bytes(SENTINEL)
            invalid_command = [*command]
            invalid_command[1] = str(bad_crc)
            invalid = invoke(invalid_command + ["--allow-partial-dwg-read"])
            require(invalid.returncode == 1 and output.read_bytes() == SENTINEL
                    and b"operationSucceeded=false" in invalid.stderr,
                    "invalid CRC refusal")
            partial = invoke(command + ["--allow-partial-dwg-read"])
            require(partial.returncode == 0 and output.read_bytes() != SENTINEL,
                    "corrected source retry")
            require(invoke([profile_test, "--check-r13-partial", str(output)]).returncode == 0,
                    "fresh output readback")
        adapter = invoke([profile_test, "--r13-partial", str(damaged), str(root / "adapter.dxf")])
        require(adapter.returncode == 0, "same-adapter failure and corrected retry")
        require(original.read_bytes() == source_bytes, "generated source was changed")
        r14 = root / "generated-r14.dwg"
        created = invoke([dwgadd, "--as", "r14", "-o", str(r14), recipe])
        require(created.returncode == 0 and r14.read_bytes()[:6] == b"AC1014",
                "R14 generation")
        r14_bytes = r14.read_bytes()
        require(invoke([profile_test, "--r14-generated", str(r14),
                        str(root / "r14-adapter.dxf")]).returncode == 0,
                "R14 graph, rollback, retry and readback")
        for binary in (False, True):
            output = root / ("r14-binary.dxf" if binary else "r14-ascii.dxf")
            command = [converter, str(r14), "-o", str(output), "-y"]
            if binary:
                command.append("-b")
            require(invoke(command).returncode == 0
                    and invoke([profile_test, "--check-r14-generated", str(output)]).returncode == 0,
                    "R14 CLI conversion and fresh 3D coordinate readback")
            require_legacy_dictionary_presence(output)
        require(r14.read_bytes() == r14_bytes, "generated R14 source was changed")
        dwgread, dwgwrite = shutil.which("dwgread"), shutil.which("dwgwrite")
        if dwgread and dwgwrite and b"0.14" in invoke([dwgread, "--version"]).stdout \
                and b"0.14" in invoke([dwgwrite, "--version"]).stdout:
            generator = Path(__file__).resolve().parents[1] / "tools/research/generate_legacy_dictionary_eed_control.py"
            for source_version in ("r13", "r14"):
                eed_root = root / (source_version + "-eed")
                generated = invoke([sys.executable, str(generator), "--version", source_version,
                                    "--output-dir", str(eed_root), "--dwgadd", dwgadd,
                                    "--dwgread", dwgread, "--dwgwrite", dwgwrite])
                require(generated.returncode == 0, source_version + " EED scratch control generation")
                eed_input = eed_root / "eed.dwg"
                require(invoke([profile_test, "--" + source_version + "-eed", str(eed_input),
                                str(eed_root / "adapter.dxf")]).returncode == 0,
                        source_version + " EED staged output, mutation refusal and retry")
                require(invoke([profile_test, "--" + source_version + "-eed-utf8",
                                str(eed_root / "utf8.dwg"), str(eed_root / "utf8-output.dxf")]).returncode == 0,
                        source_version + " UTF-8 EED staged output, mutation refusal and retry")
                require(invoke([profile_test, "--" + source_version + "-eed-wide",
                                str(eed_root / "wide.dwg"), str(eed_root / "utf8.dwg"),
                                str(eed_root / "wide-output.dxf")]).returncode == 0,
                        source_version + " unrepresentable EED refusal and corrected retry")
                for profile, name in (("little", "unqualified.dwg"),
                                      ("big", "big-layer.dwg"),
                                      ("missing", "big-missing.dwg"),
                                      ("wide", "big-wide.dwg"),
                                      ("zero", "big-zero.dwg")):
                    require(invoke([profile_test, "--layer-policy-" + profile,
                                    str(eed_root / name)]).returncode == 0,
                            source_version + " explicit layer byte-order policy " + profile)
                require(invoke([profile_test, "--layer-big-partial", str(eed_root / "big-layer.dwg"),
                                str(eed_root / "big-missing.dwg"), str(eed_root / "layer-output.dxf")]).returncode == 0,
                        source_version + " big-endian EED layer partial output, refusal and retry")
                require(invoke([profile_test, "--layer-big-partial", str(eed_root / "big-layer.dwg"),
                                str(eed_root / "big-zero.dwg"), str(eed_root / "layer-zero-output.dxf")]).returncode == 0,
                        source_version + " zero EED layer refusal and corrected retry")
                for binary in (False, True):
                    layer_output = eed_root / ("cli-layer-binary.dxf" if binary else "cli-layer-ascii.dxf")
                    sentinel = b"unresolved layer destination preserved\n"
                    command = [converter, str(eed_root / "big-layer.dwg"), "-o", str(layer_output), "-y",
                               "--allow-partial-dwg-read"] + (["-b"] if binary else [])
                    layer_output.write_bytes(sentinel)
                    require(invoke(command).returncode != 0 and layer_output.read_bytes() == sentinel,
                            source_version + " default byte order refuses unresolved source")
                    require(invoke(command + ["--dwg-eed-layer-byte-order=big"]).returncode == 0
                            and invoke([profile_test, "--check-layer-dxf", str(layer_output)]).returncode == 0,
                            source_version + " CLI explicit big-endian layer output")
                    layer_output.write_bytes(sentinel)
                    wrong = list(command)
                    wrong[1] = str(eed_root / "big-missing.dwg")
                    require(invoke(wrong + ["--dwg-eed-layer-byte-order=big"]).returncode != 0
                            and layer_output.read_bytes() == sentinel
                            and invoke(command + ["--dwg-eed-layer-byte-order=big"]).returncode == 0,
                            source_version + " CLI unresolved layer refusal and corrected retry")
                require(invoke([profile_test, "--legacy-little-layer-eed",
                                str(eed_root / "unqualified.dwg"),
                                str(eed_root / "unqualified-output.dxf")]).returncode == 0,
                        source_version + " little-endian layer EED source proof and output")
                alternate = root / (source_version + "-eed-alternate-app")
                require(invoke([sys.executable, str(generator), "--version", source_version,
                                "--alternate-app", "--output-dir", str(alternate), "--dwgadd", dwgadd,
                                "--dwgread", dwgread, "--dwgwrite", dwgwrite]).returncode == 0,
                        source_version + " alternate APPID control generation")
                for profile, file in (("eed", "eed.dwg"), ("eed-utf8", "utf8.dwg")):
                    require(invoke([profile_test, "--" + source_version + "-" + profile + "-alternate-app",
                                    str(alternate / file), str(alternate / "adapter.dxf")]).returncode == 0,
                            source_version + " alternate APPID " + profile + " proof and retry")
                require(invoke([profile_test, "--" + source_version + "-eed-wide",
                                str(alternate / "wide.dwg"), str(alternate / "utf8.dwg"),
                                str(alternate / "wide-output.dxf")]).returncode == 0,
                        source_version + " alternate APPID unrepresentable refusal")
                remapped = root / (source_version + "-eed-remapped")
                require(invoke([sys.executable, str(generator), "--version", source_version,
                                "--alternate-app", "--alternate-group", "--output-dir", str(remapped),
                                "--dwgadd", dwgadd, "--dwgread", dwgread, "--dwgwrite", dwgwrite]).returncode == 0,
                        source_version + " remapped dictionary graph control generation")
                for profile, file in (("eed", "eed.dwg"), ("eed-utf8", "utf8.dwg")):
                    require(invoke([profile_test, "--" + source_version + "-" + profile + "-remapped",
                                    str(remapped / file), str(remapped / "adapter.dxf")]).returncode == 0,
                            source_version + " remapped dictionary " + profile + " graph proof and retry")
                require(invoke([profile_test, "--" + source_version + "-eed-bad-graph",
                                str(remapped / "bad-graph.dwg"), str(remapped / "eed.dwg"),
                                str(remapped / "graph-output.dxf")]).returncode == 0,
                        source_version + " bad source graph refusal and corrected retry")
                require(invoke([profile_test, "--" + source_version + "-eed-wide-remapped",
                                str(remapped / "wide.dwg"), str(remapped / "utf8.dwg"),
                                str(remapped / "wide-output.dxf")]).returncode == 0,
                        source_version + " remapped dictionary unrepresentable refusal")
                for remap_group in (False, True):
                    group_root = root / (source_version + ("-nonempty-remapped" if remap_group else "-nonempty"))
                    group_command = [sys.executable, str(generator), "--version", source_version,
                                     "--nonempty-group", "--output-dir", str(group_root), "--dwgadd", dwgadd,
                                     "--dwgread", dwgread, "--dwgwrite", dwgwrite]
                    if remap_group:
                        group_command += ["--alternate-app", "--alternate-group"]
                    require(invoke(group_command).returncode == 0,
                            source_version + " nonempty group source generation")
                    suffix = "-nonempty-remapped" if remap_group else "-nonempty"
                    for profile, file in (("eed", "eed.dwg"), ("eed-utf8", "utf8.dwg")):
                        require(invoke([profile_test, "--" + source_version + "-" + profile + suffix,
                                        str(group_root / file), str(group_root / "adapter.dxf")]).returncode == 0,
                                source_version + " nonempty group " + profile + " graph proof and retry")
                    wide_profile = "--" + source_version + "-eed-wide" + ("-remapped" if remap_group else "")
                    require(invoke([profile_test, wide_profile, str(group_root / "wide.dwg"),
                                    str(group_root / "utf8.dwg"), str(group_root / "wide-output.dxf")]).returncode == 0,
                            source_version + " nonempty group unrepresentable EED refusal")
                    if remap_group:
                        require(invoke([profile_test, "--" + source_version + "-eed-dangling",
                                        str(group_root / "bad-handle.dwg"), str(group_root / "handle.dwg"),
                                        str(group_root / "dangling-output.dxf")]).returncode == 0,
                                source_version + " group dangling EED handle refusal and retry")
                        require(invoke([profile_test, "--" + source_version + "-group-handle",
                                        str(group_root / "handle.dwg"), str(group_root / "handle-output.dxf")]).returncode == 0,
                                source_version + " group EED handle, mutation and retry")
                        require(invoke([profile_test, "--" + source_version + "-group-points",
                                        str(group_root / "points.dwg"), str(group_root / "points-output.dxf")]).returncode == 0,
                                source_version + " group 3D EED points, mutation and retry")
                        require(invoke([profile_test, "--" + source_version + "-group-binary",
                                        str(group_root / "binary.dwg"), str(group_root / "binary-output.dxf")]).returncode == 0,
                                source_version + " group binary EED chunking, mutation and retry")
                        require(invoke([profile_test, "--" + source_version + "-group-groups",
                                        str(group_root / "groups.dwg"), str(group_root / "groups-output.dxf")]).returncode == 0,
                                source_version + " group nested 1002 groups, mutation and retry")
                        require(invoke([profile_test, "--" + source_version + "-group-reals",
                                        str(group_root / "reals.dwg"), str(group_root / "reals-output.dxf")]).returncode == 0,
                                source_version + " group 1040-1042 real values, mutation and retry")
                        require(invoke([profile_test, "--" + source_version + "-group-int32",
                                        str(group_root / "int32.dwg"), str(group_root / "int32-output.dxf")]).returncode == 0,
                                source_version + " group 1071 boundaries, mutation and retry")
                    if remap_group:
                        require(invoke([profile_test, "--" + source_version + "-eed-bad-graph",
                                        str(group_root / "bad-graph.dwg"), str(group_root / "eed.dwg"),
                                        str(group_root / "graph-output.dxf")]).returncode == 0,
                                source_version + " nonempty group bad source graph refusal")
                    require(invoke([profile_test, "--" + source_version + "-eed-bad-member",
                                    str(group_root / "bad-member-role.dwg"), str(group_root / "eed.dwg"),
                                    str(group_root / "role-output.dxf")]).returncode == 0,
                            source_version + " wrong member handle role refusal and corrected retry")
                for remap_nod in (False, True):
                    nod_root = root / (source_version + ("-nod-remapped" if remap_nod else "-nod"))
                    nod_command = [sys.executable, str(generator), "--version", source_version,
                                   "--eed-on-nod", "--output-dir", str(nod_root), "--dwgadd", dwgadd,
                                   "--dwgread", dwgread, "--dwgwrite", dwgwrite]
                    if remap_nod:
                        nod_command += ["--alternate-app", "--alternate-group", "--nonempty-group"]
                    require(invoke(nod_command).returncode == 0, source_version + " NOD EED source generation")
                    suffix = "-remapped" if remap_nod else ""
                    if not remap_nod:
                        require(invoke([profile_test, "--layer-big-nod", str(nod_root / "big-layer.dwg"),
                                        str(nod_root / "big-missing.dwg"), str(nod_root / "layer-output.dxf")]).returncode == 0,
                                source_version + " NOD layer source, refusal and retry")
                    if remap_nod:
                        require(invoke([profile_test, "--" + source_version + "-eed-dangling",
                                        str(nod_root / "bad-handle.dwg"), str(nod_root / "handle.dwg"),
                                        str(nod_root / "dangling-output.dxf")]).returncode == 0,
                                source_version + " nod dangling EED handle refusal and retry")
                        require(invoke([profile_test, "--" + source_version + "-nod-handle",
                                        str(nod_root / "handle.dwg"), str(nod_root / "handle-output.dxf")]).returncode == 0,
                                source_version + " nod EED handle, mutation and retry")
                        require(invoke([profile_test, "--" + source_version + "-nod-points",
                                        str(nod_root / "points.dwg"), str(nod_root / "points-output.dxf")]).returncode == 0,
                                source_version + " nod 3D EED points, mutation and retry")
                        require(invoke([profile_test, "--" + source_version + "-nod-binary",
                                        str(nod_root / "binary.dwg"), str(nod_root / "binary-output.dxf")]).returncode == 0,
                                source_version + " nod binary EED chunking, mutation and retry")
                        require(invoke([profile_test, "--" + source_version + "-nod-groups",
                                        str(nod_root / "groups.dwg"), str(nod_root / "groups-output.dxf")]).returncode == 0,
                                source_version + " nod nested 1002 groups, mutation and retry")
                        require(invoke([profile_test, "--" + source_version + "-nod-reals",
                                        str(nod_root / "reals.dwg"), str(nod_root / "reals-output.dxf")]).returncode == 0,
                                source_version + " nod 1040-1042 real values, mutation and retry")
                        require(invoke([profile_test, "--" + source_version + "-nod-int32",
                                        str(nod_root / "int32.dwg"), str(nod_root / "int32-output.dxf")]).returncode == 0,
                                source_version + " NOD 1071 boundaries, mutation and retry")
                    if remap_nod:
                        require(invoke([profile_test, "--" + source_version + "-nod-bad-graph",
                                        str(nod_root / "bad-graph.dwg"), str(nod_root / "eed.dwg"),
                                        str(nod_root / "bad-output.dxf")]).returncode == 0,
                                source_version + " NOD bad group graph refusal and retry")
                    for profile, file in (("nod-eed", "eed.dwg"), ("nod-utf8", "utf8.dwg")):
                        require(invoke([profile_test, "--" + source_version + "-" + profile + suffix,
                                        str(nod_root / file), str(nod_root / "adapter.dxf")]).returncode == 0,
                                source_version + " NOD " + profile + " graph proof and retry")
                    require(invoke([profile_test, "--" + source_version + "-nod-wide" + suffix,
                                    str(nod_root / "wide.dwg"), str(nod_root / "utf8.dwg"),
                                    str(nod_root / "wide-output.dxf")]).returncode == 0,
                            source_version + " NOD unrepresentable EED refusal")
                for remap_mline in (False, True):
                    mline_root = root / (source_version + ("-mline-remapped" if remap_mline else "-mline"))
                    mline_command = [sys.executable, str(generator), "--version", source_version,
                                     "--eed-on-mline", "--output-dir", str(mline_root), "--dwgadd", dwgadd,
                                     "--dwgread", dwgread, "--dwgwrite", dwgwrite]
                    if remap_mline:
                        mline_command += ["--alternate-app", "--alternate-group", "--nonempty-group"]
                    require(invoke(mline_command).returncode == 0,
                            source_version + " MLINESTYLE dictionary EED source generation")
                    suffix = "-remapped" if remap_mline else ""
                    if not remap_mline:
                        require(invoke([profile_test, "--layer-big-mline", str(mline_root / "big-layer.dwg"),
                                        str(mline_root / "big-missing.dwg"), str(mline_root / "layer-output.dxf")]).returncode == 0,
                                source_version + " MLINESTYLE owner layer source, refusal and retry")
                    if remap_mline:
                        require(invoke([profile_test, "--" + source_version + "-eed-dangling",
                                        str(mline_root / "bad-handle.dwg"), str(mline_root / "handle.dwg"),
                                        str(mline_root / "dangling-output.dxf")]).returncode == 0,
                                source_version + " mline dangling EED handle refusal and retry")
                        require(invoke([profile_test, "--" + source_version + "-mline-handle",
                                        str(mline_root / "handle.dwg"), str(mline_root / "handle-output.dxf")]).returncode == 0,
                                source_version + " mline EED handle, mutation and retry")
                        require(invoke([profile_test, "--" + source_version + "-mline-points",
                                        str(mline_root / "points.dwg"), str(mline_root / "points-output.dxf")]).returncode == 0,
                                source_version + " mline 3D EED points, mutation and retry")
                        require(invoke([profile_test, "--" + source_version + "-mline-binary",
                                        str(mline_root / "binary.dwg"), str(mline_root / "binary-output.dxf")]).returncode == 0,
                                source_version + " mline binary EED chunking, mutation and retry")
                        require(invoke([profile_test, "--" + source_version + "-mline-groups",
                                        str(mline_root / "groups.dwg"), str(mline_root / "groups-output.dxf")]).returncode == 0,
                                source_version + " mline nested 1002 groups, mutation and retry")
                        require(invoke([profile_test, "--" + source_version + "-mline-reals",
                                        str(mline_root / "reals.dwg"), str(mline_root / "reals-output.dxf")]).returncode == 0,
                                source_version + " mline 1040-1042 real values, mutation and retry")
                        require(invoke([profile_test, "--" + source_version + "-mline-int32",
                                        str(mline_root / "int32.dwg"), str(mline_root / "int32-output.dxf")]).returncode == 0,
                                source_version + " MLINESTYLE 1071 boundaries, mutation and retry")
                    if remap_mline:
                        require(invoke([profile_test, "--" + source_version + "-mline-bad-graph",
                                        str(mline_root / "bad-graph.dwg"), str(mline_root / "eed.dwg"),
                                        str(mline_root / "bad-output.dxf")]).returncode == 0,
                                source_version + " MLINESTYLE bad source graph refusal")
                    for profile, file in (("mline-eed", "eed.dwg"), ("mline-utf8", "utf8.dwg")):
                        require(invoke([profile_test, "--" + source_version + "-" + profile + suffix,
                                        str(mline_root / file), str(mline_root / "adapter.dxf")]).returncode == 0,
                                source_version + " MLINESTYLE " + profile + " output and retry")
                    require(invoke([profile_test, "--" + source_version + "-mline-wide",
                                    str(mline_root / "wide.dwg"), str(mline_root / "utf8.dwg"),
                                    str(mline_root / "wide-output.dxf")]).returncode == 0,
                            source_version + " MLINESTYLE unrepresentable EED refusal")
                for binary in (False, True):
                    output = eed_root / ("cli-binary.dxf" if binary else "cli-ascii.dxf")
                    command = [converter, str(eed_input), "-o", str(output), "-y"]
                    if binary:
                        command.append("-b")
                    if source_version == "r13":
                        command.append("--allow-partial-dwg-read")
                    require(invoke(command).returncode == 0
                            and invoke([profile_test, "--check-r14-generated", str(output)]).returncode == 0,
                            source_version + " EED CLI conversion and fresh readback")
    print("AC1012 locally generated partial-read CLI and same-adapter tests: PASS")


if __name__ == "__main__":
    main()
