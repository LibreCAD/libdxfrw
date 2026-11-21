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
            eed_root = root / "r14-eed"
            generated = invoke([sys.executable, str(generator), "--output-dir", str(eed_root),
                                "--dwgadd", dwgadd, "--dwgread", dwgread, "--dwgwrite", dwgwrite])
            require(generated.returncode == 0, "R14 EED scratch control generation")
            eed_input = eed_root / "eed.dwg"
            require(invoke([profile_test, "--r14-eed", str(eed_input),
                            str(eed_root / "adapter.dxf")]).returncode == 0,
                    "R14 EED staged output, mutation refusal and retry")
            for binary in (False, True):
                output = eed_root / ("cli-binary.dxf" if binary else "cli-ascii.dxf")
                command = [converter, str(eed_input), "-o", str(output), "-y"]
                if binary:
                    command.append("-b")
                require(invoke(command).returncode == 0
                        and invoke([profile_test, "--check-r14-generated", str(output)]).returncode == 0,
                        "R14 EED CLI conversion and fresh readback")
    print("AC1012 locally generated partial-read CLI and same-adapter tests: PASS")


if __name__ == "__main__":
    main()
