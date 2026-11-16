#!/usr/bin/env python3
"""Check that supported package/version surfaces agree."""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path


def checks(root: Path, expected: str) -> list[str]:
    patterns = {
        "CMakeLists.txt": (r"project\(DXFRW VERSION ([0-9]+\.[0-9]+\.[0-9]+)\)", 1),
        "src/drw_base.h": (r"#define DRW_VERSION \"([^\"]+)\"", 1),
        "configure.ac": (r"AC_INIT\(\[libdxfrw\], \[([^\]]+)\]", 1),
        "conanfile.py": (r"version\s*=\s*\"([^\"]+)\"", 1),
    }
    errors = []
    for relative, (pattern, group) in patterns.items():
        path = root / relative
        text = path.read_text(encoding="utf-8")
        match = re.search(pattern, text)
        if not match:
            errors.append(f"{relative}: version declaration not found")
        elif match.group(group) != expected:
            errors.append(f"{relative}: {match.group(group)} != {expected}")
    major = expected.split(".", 1)[0]
    configure = (root / "configure.ac").read_text(encoding="utf-8")
    abi = re.search(r"^LIBDXFRW_ABI_MAJOR=([0-9]+)$", configure, re.MULTILINE)
    if not abi or abi.group(1) != major:
        errors.append(f"configure.ac: ABI major must be {major}")
    automake = (root / "src/Makefile.am").read_text(encoding="utf-8")
    if not re.search(r"-version-number\s+\$\(LIBDXFRW_ABI_MAJOR\):0:0", automake):
        errors.append("src/Makefile.am: libtool version must follow ABI major")
    cmake = (root / "CMakeLists.txt").read_text(encoding="utf-8")
    if not re.search(r'SOVERSION\s+"\$\{PROJECT_VERSION_MAJOR\}"', cmake):
        errors.append("CMakeLists.txt: SOVERSION must follow project major")
    if not re.search(r'RUNTIME_OUTPUT_NAME\s+"dxfrw-\$\{PROJECT_VERSION_MAJOR\}"', cmake):
        errors.append("CMakeLists.txt: Windows runtime name must follow project major")
    if not re.search(r"COMPATIBILITY\s+SameMajorVersion", cmake):
        errors.append("CMakeLists.txt: package compatibility must require same major")
    return errors


def self_test() -> None:
    from tempfile import TemporaryDirectory
    with TemporaryDirectory() as directory:
        root = Path(directory)
        (root / "src").mkdir()
        (root / "CMakeLists.txt").write_text(
            'project(DXFRW VERSION 2.0.0)\nSOVERSION "${PROJECT_VERSION_MAJOR}"\n'
            'RUNTIME_OUTPUT_NAME "dxfrw-${PROJECT_VERSION_MAJOR}"\n'
            'COMPATIBILITY SameMajorVersion\n')
        (root / "src/drw_base.h").write_text('#define DRW_VERSION "2.0.0"\n')
        (root / "src/Makefile.am").write_text(
            "libdxfrw_la_LDFLAGS = -version-number $(LIBDXFRW_ABI_MAJOR):0:0\n")
        (root / "configure.ac").write_text(
            "AC_INIT([libdxfrw], [2.0.0], [x])\nLIBDXFRW_ABI_MAJOR=2\n")
        (root / "conanfile.py").write_text('version = "2.0.0"\n')
        assert checks(root, "2.0.0") == []
        assert checks(root, "1.0.0")
        (root / "configure.ac").write_text(
            "AC_INIT([libdxfrw], [2.0.0], [x])\nLIBDXFRW_ABI_MAJOR=1\n")
        assert any("ABI major" in error for error in checks(root, "2.0.0"))
        (root / "src/Makefile.am").write_text("libdxfrw_la_LDFLAGS = -version-number 2:0:0\n")
        assert any("libtool version" in error for error in checks(root, "2.0.0"))
    print("check_version_surfaces self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path("."))
    parser.add_argument("--expected", default="3.0.0")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    if args.self_test:
        self_test()
        return 0
    errors = checks(args.root, args.expected)
    if errors:
        for error in errors:
            print(f"ERROR: {error}", file=sys.stderr)
        return 1
    print(f"Version surfaces: PASS ({args.expected})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
