#!/usr/bin/env python3
"""Probe hash-bound AC1024 LayerStates graph records; no output admission.

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
    profiles = {
        "0.1.D5": (
            "bitsize: 858", "data_size: 782/0x30e", "ownerhandle: (4.1.2) abs:2",
            'texts[0]: "ACAD_LAYERFILTERS"', 'texts[1]: "ACAD_LAYERSTATES"',
            'texts[2]: "ACLYDICTIONARY"', 'itemhandles[0]: (2.1.EE) abs:EE',
            'itemhandles[1]: (2.2.1B8) abs:1B8', 'itemhandles[2]: (2.1.ED) abs:ED'),
        "0.2.1B8": (
            "bitsize: 438", "data_size: 346/0x15a", "ownerhandle: (4.1.D5) abs:D5",
            "reactors[0]: (4.1.D5) abs:D5", 'texts[0]: "ACAD_VIEWS_RenderView"',
            'itemhandles[0]: (2.2.249) abs:249'),
        "0.1.EE": (
            "bitsize: 60", "has_strings: 0", "ownerhandle: (12.1.19) abs:D5",
            "reactors[0]: (4.1.D5) abs:D5", "numitems: 0"),
        "0.2.24A": (
            "bitsize: 462", "data_size: 378/0x17a", "EED[0] size: 0", "num_reactors: 0",
            "is_xdic_missing: 1", "numitems: 1", "cloning: 1", "is_hardowner: 0x1",
            "ownerhandle: (8.0.0) abs:249", 'texts[0]: "AcadLayerStateAnnoScale"',
            'itemhandles[0]: (2.2.24B) abs:24B'),
        "0.1.ED": (
            "bitsize: 174", "has_strings: 0", "ownerhandle: (12.1.18) abs:D5",
            "reactors[0]: (4.1.D5) abs:D5", "numitems: 0"),
    }
    for handle, tokens in profiles.items():
        position = text.find("handle: " + handle + " [H 5]")
        begin = text.rfind("< Next object:", 0, position)
        end = text.find("< Next object:", position)
        if position < 0 or begin < 0 or end < 0:
            raise SystemExit("independent dictionary object boundaries missing: " + handle)
        peer_object = text[begin:end]
        if "Decode object DICTIONARY" not in peer_object:
            raise SystemExit("independent object is not DICTIONARY: " + handle)
        for token in tokens:
            if token not in peer_object:
                raise SystemExit("independent dictionary evidence missing: " + handle + " " + token)
    scale_handle = text.find("handle: 0.2.127 [H 5]")
    scale_start = text.rfind("< Next object:", 0, scale_handle)
    scale_end = text.find("< Next object:", scale_handle)
    if scale_handle < 0 or scale_start < 0 or scale_end < 0:
        raise SystemExit("independent SCALE object boundaries are missing")
    scale_section = text[scale_start:scale_end]
    for token in ("Add object SCALE", "Type: 512", "bitsize: 129", "data_size: 58/0x3a",
                  "EED[0] size: 0", "num_reactors: 1", "is_xdic_missing: 1",
                  "ownerhandle: (8.0.0) abs:126", "reactors[0]: (4.2.126) abs:126",
                  'flag: 0', 'name: "1:1"', 'paper_units: 1', 'drawing_units: 1',
                  'is_unit_scale: 1'):
        if token not in scale_section:
            raise SystemExit("independent SCALE evidence missing: " + token)
    xr_handle = text.find("handle: 0.2.24B [H 5]")
    xr_start = text.rfind("< Next object:", 0, xr_handle)
    xr_end = text.find("< Next object:", xr_handle)
    if xr_handle < 0 or xr_start < 0 or xr_end < 0:
        raise SystemExit("independent nested XRECORD boundaries are missing")
    xr_section = text[xr_start:xr_end]
    for token in ("Decode object XRECORD", "bitsize: 148", "has_strings: 0",
                  "ownerhandle: (8.0.0) abs:24A", "reactors[0]: (4.2.24A) abs:24A",
                  "xdata_size: 10", "xdata[0] type: 330", "xdata[0]: 127", "cloning: 1"):
        if token not in xr_section:
            raise SystemExit("independent nested XRECORD evidence missing: " + token)
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
