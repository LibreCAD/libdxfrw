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


def assert_staged_flags(flags, prefix: Path) -> None:
    prefix = prefix.resolve()
    for flag in flags:
        if flag.startswith(("-I", "-L")) and len(flag) > 2:
            path = Path(flag[2:]).resolve()
            try:
                path.relative_to(prefix)
            except ValueError as error:
                raise RuntimeError(
                    "pkg-config resolved outside staged prefix: %s" % flag
                ) from error


def self_test_staged_flags() -> None:
    prefix = Path(tempfile.gettempdir()) / "libdxfrw-staged-prefix"
    assert_staged_flags(
        ["-I" + str(prefix / "include"), "-L" + str(prefix / "lib"),
         "-ldxfrw"], prefix)
    try:
        assert_staged_flags(["-I/usr/local/include", "-L/usr/local/lib"],
                            prefix)
    except RuntimeError:
        return
    raise RuntimeError("staged pkg-config path guard accepted system paths")


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
            "#include <libdxfrw.h>\n"
            "static bool configure_librecad_dxf_adapter(dxfRW& codec) {\n"
            "  codec.setDxfCompatibilityProfile(\n"
            "      dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy);\n"
            "  using Read = bool (dxfRW::*)(DRW_Interface*, bool);\n"
            "  using ReadAscii = bool (dxfRW::*)(DRW_Interface*, bool, std::string&);\n"
            "  using Write = bool (dxfRW::*)(DRW_Interface*, DRW::Version, bool);\n"
            "  const Read read = &dxfRW::read;\n"
            "  const ReadAscii readAscii = &dxfRW::readAscii;\n"
            "  const Write write = &dxfRW::write;\n"
            "  (void)read; (void)readAscii; (void)write;\n"
            "  return codec.dxfCompatibilityProfile() ==\n"
            "      dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy;\n"
            "}\n"
            "int main() {\n"
            "  dxfRW writer(\"\");\n"
            "  if (writer.dxfCompatibilityProfile() !=\n"
            "      dxfRW::DxfCompatibilityProfile::StandaloneSafe) return 1;\n"
            "  writer.setDxfCompatibilityProfile(\n"
            "      dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy);\n"
            "  if (writer.dxfCompatibilityProfile() !=\n"
            "      dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy) return 2;\n"
            "  writer.setDxfCompatibilityProfile(\n"
            "      dxfRW::DxfCompatibilityProfile::StandaloneSafe);\n"
            "  if (!configure_librecad_dxf_adapter(writer)) return 3;\n"
            "  return writer.getError();\n"
            "}\n",
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
        flags = shlex.split(run(["pkg-config", "--define-prefix", "--cflags", "--libs",
                                 "libdxfrw"], env=pkgconfig).stdout)
        assert_staged_flags(flags, prefix)
        run([cxx, "-std=c++17", "-Wall", "-Wextra", "-Werror",
             str(consumer), "-o", str(root / "pkgconfig-consumer")] + flags)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prefix", type=Path)
    parser.add_argument("--self-test", action="store_true",
                        help="exercise staged-path acceptance and rejection")
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    args = parser.parse_args()
    if args.self_test:
        self_test_staged_flags()
        print("staged package checker self-test: PASS")
        if args.prefix is None:
            return 0
    if args.prefix is None:
        parser.error("--prefix is required unless --self-test is used alone")
    try:
        check(args.prefix, args.cxx)
    except subprocess.CalledProcessError as error:
        if error.output:
            print(error.output, end="")
        raise
    print("staged package check: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
