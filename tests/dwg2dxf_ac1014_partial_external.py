#!/usr/bin/env python3
"""Exercise explicit partial conversion with a temporary R14 fault control.

The input is an external, hash-pinned LibreDWG drawing; it is never copied into
the repository. A single coordinate is changed to NaN and the object CRC is
recomputed in a temporary directory. No producer-authored failure is claimed.
"""
import hashlib
import struct
import subprocess
import sys
import tempfile
from pathlib import Path


SOURCE_SHA256 = "e296ca50f237abb9eec1a8bdab77aa16fd59518e246df812aa4dd4f859bd12d3"
FAULT_SHA256 = "1d1d378f2ac1cd130c536d43df7acb9ee537ba465fb3fd6f8386143e5555f5b2"
SENTINEL = b"protected AC1014 partial destination\n"


def require(value, message):
    if not value:
        raise AssertionError(message)


def crc16(payload):
    crc = 0xC0C1
    for byte in payload:
        crc ^= byte
        for _ in range(8):
            crc = (crc >> 1) ^ (0xA001 if crc & 1 else 0)
    return crc


def sha(data):
    return hashlib.sha256(data).hexdigest()


def source_control(payload):
    require(sha(payload) == SOURCE_SHA256 and payload[:6] == b"AC1014", "source hash/version")
    data = bytearray(payload)
    require(data[1600:1602] == bytes([49, 0])
            and int.from_bytes(data[1651:1653], "little") == crc16(data[1600:1651]),
            "source LINE frame/CRC")
    coordinate = struct.pack("<d", -37.73771261670087)
    old = "".join(f"{byte:08b}" for byte in coordinate)
    base = 1602 * 8 + 141
    observed = "".join(str((data[(base + i) // 8] >> (7 - (base + i) % 8)) & 1)
                       for i in range(64))
    require(observed == old, "source LINE coordinate span")
    replacement = "".join(f"{byte:08b}" for byte in struct.pack("<Q", 0x7FF8000000000000))
    for i, bit in enumerate(replacement):
        position = base + i
        mask = 0x80 >> (position % 8)
        data[position // 8] = (data[position // 8] | mask if bit == "1"
                               else data[position // 8] & ~mask)
    data[1651:1653] = crc16(data[1600:1651]).to_bytes(2, "little")
    require(sha(data) == FAULT_SHA256, "controlled failure hash")
    return data


def invoke(args):
    return subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          timeout=30, check=False)


def main():
    if len(sys.argv) != 4:
        raise SystemExit("usage: script <dwg2dxf> <profile-test> <external-r14.dwg>")
    converter, profile_test, source = sys.argv[1:]
    original = Path(source).read_bytes()
    with tempfile.TemporaryDirectory(prefix="libdxfrw-ac1014-partial-") as directory:
        root = Path(directory)
        damaged = root / "controlled-failure.dwg"
        damaged.write_bytes(source_control(original))
        bad_crc = root / "invalid-frame-crc.dwg"
        crc_bytes = bytearray(damaged.read_bytes())
        crc_bytes[1610] ^= 1
        bad_crc.write_bytes(crc_bytes)
        expected = (b"sourceDwgEntityParseFailures=1", b"sourceDwgObjectParseFailures=0",
                    b"sourceDwgSourceFrames=51", b"sourceDwgFramesPublished=50",
                    b"sourceDwgFramesFailed=1", b"sourceDwgFramesUnresolved=0")
        for binary in (False, True):
            output = root / ("binary.dxf" if binary else "ascii.dxf")
            output.write_bytes(SENTINEL)
            command = [converter, str(damaged), "-o", str(output), "-y", "-d"]
            if binary:
                command.append("-b")
            strict = invoke(command)
            require(strict.returncode == 1 and output.read_bytes() == SENTINEL
                    and all(item in strict.stderr for item in expected),
                    "strict failure or destination preservation")
            partial = invoke(command + ["--allow-partial-dwg-read"])
            require(partial.returncode == 0 and output.read_bytes() != SENTINEL
                    and all(item in partial.stderr for item in expected)
                    and b"explicit partial DWG read mode" in partial.stderr,
                    "explicit partial conversion")
            output.write_bytes(SENTINEL)
            invalid_command = [*command]
            invalid_command[1] = str(bad_crc)
            invalid = invoke(invalid_command + ["--allow-partial-dwg-read"])
            require(invalid.returncode == 1 and output.read_bytes() == SENTINEL
                    and b"operationSucceeded=false" in invalid.stderr,
                    "invalid source frame CRC refusal")
            partial = invoke(command + ["--allow-partial-dwg-read"])
            require(partial.returncode == 0 and output.read_bytes() != SENTINEL,
                    "corrected source retry after CRC refusal")
            fresh = invoke([profile_test, "--check-partial", str(output)])
            require(fresh.returncode == 0, "fresh CLI DXF readback")
        adapter = invoke([profile_test, "--partial", str(damaged), str(root / "adapter.dxf")])
        require(adapter.returncode == 0, "same-adapter strict/partial/refusal/retry controls")
    require(Path(source).read_bytes() == original, "source was changed")
    print("AC1014 controlled partial-read CLI and same-adapter tests: PASS")


if __name__ == "__main__":
    main()
