#!/usr/bin/env python3
"""Probe one hash-bound AC1024 LayerStates companion; never enables output.

Requires the local AC1024 corpus file and a LibreDWG 0.14 -v4 trace. The
probe is local research evidence, not a portable qualification or a fixture.
"""
import argparse
import hashlib
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
SOURCE_SHA256 = "edf22e187337e37d2083b26d6d00ce30ed78d2fd31d6bf0e9869902325ad3ab1"
PEER_TRACE_SHA256 = "9bead9945a7e9b78c541e73d9ab4c0fe461c8e3ba4105e95bcd3103fe7f9603c"


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("peer_trace", type=Path)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build-gcc")
    parser.add_argument("--cxx", default="c++")
    args = parser.parse_args()
    source = args.source.resolve()
    trace = args.peer_trace.resolve()
    if sha256(source) != SOURCE_SHA256:
        raise SystemExit("source SHA-256 does not match this physical profile")
    if sha256(trace) != PEER_TRACE_SHA256:
        raise SystemExit("independent LibreDWG trace SHA-256 does not match")
    text = trace.read_text(errors="replace")
    start = text.find("handle: 0.1.ED [H 5]")
    if start < 0:
        raise SystemExit("independent trace does not contain companion 0xED")
    end = text.find("< Next object:", start)
    if end < 0:
        raise SystemExit("independent trace companion boundary is missing")
    section = text[start:end]
    for token in ('EED[0] size: 11 [BS]', 'EED[0] handle: 5.1.12',
                  'EED[0] code: 0 [RC], wstring: len=4 [RS] "<All"'):
        if token not in section:
            raise SystemExit("independent trace is missing expected EED: " + token)
    app_handle = text.find("handle: 0.1.12 [H 5]")
    app_start = text.rfind("< Next object:", 0, app_handle)
    app_end = text.find("< Next object:", app_handle)
    if app_handle < 0 or app_start < 0 or app_end < 0:
        raise SystemExit("independent APPID object boundaries are missing")
    app_section = text[app_start:app_end]
    if 'Add table record APPID' not in app_section or 'name: "ACAD" [TU 2]' not in app_section:
        raise SystemExit("independent trace does not bind APPID 0x12 to ACAD")
    build = args.build_dir.resolve()
    subprocess.run(["cmake", "--build", str(build), "--target",
                    "libdxfrw_test_adapter", "-j2"], cwd=ROOT, check=True)
    objects = list((build / "tests" / "CMakeFiles" /
                    "libdxfrw_test_adapter.dir").rglob("dx_iface.cpp.o"))
    if len(objects) != 1 or not (build / "libdxfrw.a").is_file():
        raise SystemExit("expected exactly one adapter object and libdxfrw.a")
    with tempfile.TemporaryDirectory(prefix="layerstates-eed-") as tmp:
        exe = Path(tmp) / "probe"
        subprocess.run([args.cxx, "-std=c++17", "-O0", "-Wall", "-Wextra", "-pedantic", "-Werror",
                        "-I", str(ROOT / "dwg2dxf"),
                        "-I", str(ROOT / "src"),
                        str(ROOT / "tools/research/probe_ac1024_layerstates_eed.cpp"),
                        str(objects[0]), str(build / "libdxfrw.a"), "-o", str(exe)],
                       cwd=ROOT, check=True)
        subprocess.run([str(exe), str(source)], cwd=ROOT, check=True)
    print("peer trace and source hashes, peer EED fields, and local controls: PASS")


if __name__ == "__main__":
    main()
