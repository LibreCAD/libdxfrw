#!/usr/bin/env python3
"""Compile installed libdxfrw headers and consumers without source-tree paths."""

from __future__ import annotations

import argparse
import os
import shlex
import subprocess
import tempfile
from pathlib import Path


PUBLIC_HEADERS = (
    "drw_acis.h",
    "drw_base.h",
    "drw_classes.h",
    "drw_datastorage.h",
    "drw_entities.h",
    "drw_header.h",
    "drw_interface.h",
    "drw_objects.h",
    "libdwgr.h",
    "libdxfrw.h",
)


def run(command, *, cwd=None, env=None):
    return subprocess.run(command, cwd=cwd, env=env, check=True,
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          text=True)


def check(prefix: Path, cxx: str) -> None:
    prefix = prefix.resolve()
    include_root = prefix / "include" / "libdxfrw"
    config_root = prefix / "lib" / "cmake" / "libdxfrw"
    if not include_root.is_dir() or not config_root.is_dir():
        raise RuntimeError("prefix is missing installed headers or CMake package")

    with tempfile.TemporaryDirectory(prefix="libdxfrw-package-") as directory:
        root = Path(directory)
        for header in PUBLIC_HEADERS:
            source = root / (header.replace("/", "_") + ".cpp")
            source.write_text("#include <libdxfrw/%s>\nint main() { return 0; }\n"
                              % header, encoding="utf-8")
            run([cxx, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                 "-I", str(prefix / "include"), "-c", str(source),
                 "-o", str(source.with_suffix(".o"))])

        consumer = root / "consumer.cpp"
        consumer.write_text(
            "#include <libdxfrw/libdxfrw.h>\n"
            "int main() { dxfRW writer(\"\"); return writer.getError(); }\n",
            encoding="utf-8")
        cmake = root / "CMakeLists.txt"
        cmake.write_text(
            "cmake_minimum_required(VERSION 3.10)\n"
            "project(libdxfrw_staged_consumer LANGUAGES CXX)\n"
            "set(CMAKE_CXX_STANDARD 17)\n"
            "find_package(libdxfrw CONFIG REQUIRED)\n"
            "add_executable(staged_consumer consumer.cpp)\n"
            "target_link_libraries(staged_consumer PRIVATE libdxfrw::libdxfrw)\n",
            encoding="utf-8")
        cmake_build = root / "cmake-build"
        run(["cmake", "-S", str(root), "-B", str(cmake_build),
             "-DCMAKE_PREFIX_PATH=" + str(prefix),
             "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"])
        run(["cmake", "--build", str(cmake_build), "-j2"])

        pkgconfig = os.environ.copy()
        pkgconfig["PKG_CONFIG_PATH"] = str(prefix / "lib" / "pkgconfig")
        flags = shlex.split(run(["pkg-config", "--cflags", "--libs",
                                 "libdxfrw"], env=pkgconfig).stdout)
        run([cxx, "-std=c++17", "-Wall", "-Wextra", "-Werror",
             str(consumer), "-o", str(root / "pkgconfig-consumer")] + flags)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prefix", type=Path, required=True)
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    args = parser.parse_args()
    check(args.prefix, args.cxx)
    print("staged package check: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
