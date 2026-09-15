#!/usr/bin/env python3
"""Compile installed libdxfrw headers and consumers without source-tree paths."""

from __future__ import annotations

import argparse
import os
import shlex
import shutil
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

PROFILE_DECLARATIONS = (
    "enum class DxfCompatibilityProfile",
    "void setDxfCompatibilityProfile(DxfCompatibilityProfile profile) noexcept;",
    "DxfCompatibilityProfile dxfCompatibilityProfile() const noexcept;",
)

PROFILE_DOCUMENTATION = (
    "Select the DXF classifier profile used by subsequent read, write, and",
    "StandaloneSafe is the default",
    "LibreCadMasterLegacy",
    "never selected implicitly from a version or file format",
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


def assert_reported_prefix(reported: str, prefix: Path) -> None:
    reported_path = Path(reported)
    if not reported_path.is_absolute() or reported_path.resolve() != prefix.resolve():
        raise RuntimeError(
            "pkg-config reported prefix does not identify staged root: %s"
            % reported)


def self_test_staged_flags() -> None:
    prefix = Path(tempfile.gettempdir()) / "libdxfrw-staged-prefix"
    assert_staged_flags(
        ["-I" + str(prefix / "include"), "-L" + str(prefix / "lib"),
         "-ldxfrw"], prefix)
    source_root = Path(__file__).resolve().parents[1]
    for stale_flags in (
        ["-I/usr/local/include", "-L/usr/local/lib"],
        ["-I" + str(source_root / "src"), "-L" + str(source_root / "build")],
    ):
        try:
            assert_staged_flags(stale_flags, prefix)
        except RuntimeError as error:
            offending = next(flag for flag in stale_flags if flag.startswith(("-I", "-L")))
            if offending not in str(error):
                raise RuntimeError(
                    "staged path diagnostic omitted offending flag: %s"
                    % offending) from error
            continue
        raise RuntimeError(
            "staged pkg-config path guard accepted non-staged paths: %s"
            % stale_flags)


def self_test_package_root_identity() -> None:
    with tempfile.TemporaryDirectory(prefix="libdxfrw-root-a-") as first_dir:
        with tempfile.TemporaryDirectory(prefix="libdxfrw-root-b-") as second_dir:
            first = Path(first_dir)
            second = Path(second_dir)
            assert_staged_flags(
                ["-I" + str(first / "include"), "-L" + str(first / "lib")],
                first)
            for mixed_flags in (
                ["-I" + str(second / "include"), "-L" + str(first / "lib")],
                ["-I" + str(first / "include"), "-L" + str(second / "lib")],
            ):
                try:
                    assert_staged_flags(mixed_flags, first)
                except RuntimeError as error:
                    offending = str(second)
                    if offending not in str(error):
                        raise RuntimeError(
                            "root diagnostic omitted offending path: %s"
                            % offending) from error
                    continue
                raise RuntimeError(
                    "pkg-config root guard accepted mixed roots: %s"
                    % mixed_flags)
            assert_reported_prefix(str(first), first)
            for stale_prefix in (str(second), "relative-prefix"):
                try:
                    assert_reported_prefix(stale_prefix, first)
                except RuntimeError as error:
                    if stale_prefix not in str(error):
                        raise RuntimeError(
                            "prefix diagnostic omitted offending value: %s"
                            % stale_prefix) from error
                    continue
                raise RuntimeError(
                    "pkg-config prefix guard accepted stale value: %s"
                    % stale_prefix)


def assert_profile_symbols(prefix: Path) -> None:
    library_candidates = sorted((prefix / "lib").glob("libdxfrw.a"))
    if not library_candidates:
        library_candidates = sorted((prefix / "lib").glob("libdxfrw.*"))
    if not library_candidates:
        raise RuntimeError("staged prefix has no libdxfrw library")
    nm = shutil.which("nm")
    cxxfilt = shutil.which("c++filt")
    if nm is None or cxxfilt is None:
        raise RuntimeError("nm and c++filt are required for symbol checks")
    symbols = subprocess.run(
        [nm, "-g", str(library_candidates[0])], check=True,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    demangled = subprocess.run(
        [cxxfilt], input=symbols.stdout, check=True,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True).stdout
    required = (
        "dxfRW::setDxfCompatibilityProfile(",
        "dxfRW::dxfCompatibilityProfile() const",
    )
    missing = [symbol for symbol in required if symbol not in demangled]
    if missing:
        raise RuntimeError(
            "staged libdxfrw is missing profile symbols: "
            + ", ".join(missing))


def assert_relocatable_cmake_export(prefix: Path) -> None:
    config_root = prefix / "lib" / "cmake" / "libdxfrw"
    export = config_root / "libdxfrwTargets.cmake"
    if not export.is_file():
        raise RuntimeError("CMake export is missing libdxfrwTargets.cmake")
    text = export.read_text(encoding="utf-8")
    if '"${_IMPORT_PREFIX}/include/libdxfrw"' not in text:
        raise RuntimeError("CMake export does not use a relocatable include root")
    source_root = str(Path(__file__).resolve().parents[1])
    target_exports = sorted(config_root.glob("libdxfrwTargets*.cmake"))
    if len(target_exports) < 2:
        raise RuntimeError("CMake export is missing configuration-specific targets")
    for target in target_exports:
        target_text = target.read_text(encoding="utf-8")
        for leaked_path in (source_root, str(prefix), "/usr/local"):
            if leaked_path in target_text:
                raise RuntimeError(
                    "CMake export contains a non-relocatable path: %s (%s)"
                    % (leaked_path, target.name))
        if target.name != export.name and "${_IMPORT_PREFIX}" not in target_text:
            raise RuntimeError(
                "configuration-specific CMake export does not use _IMPORT_PREFIX: %s"
                % target.name)
    for config in sorted(config_root.glob("libdxfrwConfig*.cmake")):
        config_text = config.read_text(encoding="utf-8")
        for leaked_path in (source_root, str(prefix), "/usr/local"):
            if leaked_path in config_text:
                raise RuntimeError(
                    "CMake package config contains a non-relocatable path: %s (%s)"
                    % (leaked_path, config.name))


def self_test_relocatable_cmake_export() -> None:
    with tempfile.TemporaryDirectory(prefix="libdxfrw-export-self-test-") as directory:
        prefix = Path(directory)
        config_root = prefix / "lib" / "cmake" / "libdxfrw"
        config_root.mkdir(parents=True)
        (config_root / "libdxfrwTargets.cmake").write_text(
            'INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include/libdxfrw"\n',
            encoding="utf-8")
        (config_root / "libdxfrwTargets-noconfig.cmake").write_text(
            'IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libdxfrw.a"\n',
            encoding="utf-8")
        (config_root / "libdxfrwConfig.cmake").write_text(
            "include(\"${CMAKE_CURRENT_LIST_DIR}/libdxfrwTargets.cmake\")\n",
            encoding="utf-8")
        (config_root / "libdxfrwConfigVersion.cmake").write_text(
            "set(PACKAGE_VERSION \"2.0.0\")\n", encoding="utf-8")
        assert_relocatable_cmake_export(prefix)
        (config_root / "libdxfrwTargets.cmake").unlink()
        try:
            assert_relocatable_cmake_export(prefix)
        except RuntimeError as error:
            if "missing libdxfrwTargets.cmake" not in str(error):
                raise RuntimeError(
                    "missing-target diagnostic is not stable: %s" % error) from error
        else:
            raise RuntimeError("CMake export self-test accepted a missing target")
        (config_root / "libdxfrwTargets.cmake").write_text(
            'INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include/libdxfrw"\n',
            encoding="utf-8")
        (config_root / "libdxfrwTargets-noconfig.cmake").unlink()
        try:
            assert_relocatable_cmake_export(prefix)
        except RuntimeError as error:
            if "missing configuration-specific targets" not in str(error):
                raise RuntimeError(
                    "missing-config-target diagnostic is not stable: %s" % error) from error
        else:
            raise RuntimeError("CMake export self-test accepted missing config targets")
        (config_root / "libdxfrwTargets-noconfig.cmake").write_text(
            'IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libdxfrw.a"\n',
            encoding="utf-8")
        (config_root / "libdxfrwTargets.cmake").write_text(
            'INTERFACE_INCLUDE_DIRECTORIES "/tmp/include/libdxfrw"\n',
            encoding="utf-8")
        try:
            assert_relocatable_cmake_export(prefix)
        except RuntimeError as error:
            if "does not use a relocatable include root" not in str(error):
                raise RuntimeError(
                    "include-root diagnostic is not stable: %s" % error) from error
        else:
            raise RuntimeError("CMake export self-test accepted an absolute include root")
        (config_root / "libdxfrwTargets.cmake").write_text(
            'INTERFACE_INCLUDE_DIRECTORIES "${_IMPORT_PREFIX}/include/libdxfrw"\n',
            encoding="utf-8")
        (config_root / "libdxfrwTargets-noconfig.cmake").write_text(
            'IMPORTED_LOCATION_NOCONFIG "/tmp/lib/libdxfrw.a"\n',
            encoding="utf-8")
        try:
            assert_relocatable_cmake_export(prefix)
        except RuntimeError as error:
            if "does not use _IMPORT_PREFIX" not in str(error):
                raise RuntimeError(
                    "config-path diagnostic is not stable: %s" % error) from error
        else:
            raise RuntimeError("CMake export self-test accepted an absolute library path")
        (config_root / "libdxfrwTargets-noconfig.cmake").write_text(
            'IMPORTED_LOCATION_NOCONFIG "${_IMPORT_PREFIX}/lib/libdxfrw.a"\n',
            encoding="utf-8")
        source_root = str(Path(__file__).resolve().parents[1])
        for stale_path in ("/usr/local", str(prefix), source_root):
            (config_root / "libdxfrwConfigVersion.cmake").write_text(
                "set(PACKAGE_VERSION \"%s\")\n" % stale_path,
                encoding="utf-8")
            try:
                assert_relocatable_cmake_export(prefix)
            except RuntimeError as error:
                if stale_path not in str(error):
                    raise RuntimeError(
                        "CMake diagnostic omitted offending path: %s"
                        % stale_path) from error
                continue
            raise RuntimeError(
                "CMake export self-test accepted stale path: %s" % stale_path)


def check_relocated_consumer(prefix: Path, cxx: str) -> None:
    """Build one minimal consumer after copying the install to a new root."""
    prefix = prefix.resolve()
    if not (prefix / "include" / "libdxfrw").is_dir():
        raise RuntimeError("relocation source is missing installed headers")
    with tempfile.TemporaryDirectory(prefix="libdxfrw-relocated-") as directory:
        root = Path(directory)
        relocated = root / "prefix"
        shutil.copytree(prefix, relocated)
        consumer = root / "consumer.cpp"
        consumer.write_text(
            "#include <libdxfrw.h>\n"
            "int main() {\n"
            "  dxfRW codec(\"\");\n"
            "  return codec.dxfCompatibilityProfile() ==\n"
            "      dxfRW::DxfCompatibilityProfile::StandaloneSafe ? 0 : 1;\n"
            "}\n",
            encoding="utf-8")
        cmake = root / "CMakeLists.txt"
        cmake.write_text(
            "cmake_minimum_required(VERSION 3.10)\n"
            "project(libdxfrw_relocated_consumer LANGUAGES CXX)\n"
            "set(CMAKE_CXX_STANDARD 17)\n"
            "find_package(libdxfrw CONFIG REQUIRED)\n"
            "add_executable(relocated_consumer consumer.cpp)\n"
            "target_link_libraries(relocated_consumer PRIVATE libdxfrw::libdxfrw)\n",
            encoding="utf-8")
        cmake_build = root / "cmake-build"
        run(["cmake", "-S", str(root), "-B", str(cmake_build),
             "-DCMAKE_PREFIX_PATH=" + str(relocated),
             "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"])
        run(["cmake", "--build", str(cmake_build), "-j2"])

        pkgconfig = os.environ.copy()
        pkgconfig["PKG_CONFIG_PATH"] = str(relocated / "lib" / "pkgconfig")
        reported_prefix = run(
            ["pkg-config", "--define-prefix", "--variable=prefix", "libdxfrw"],
            env=pkgconfig).stdout.strip()
        assert_reported_prefix(reported_prefix, relocated)
        flags = shlex.split(run(
            ["pkg-config", "--define-prefix", "--cflags", "--libs", "libdxfrw"],
            env=pkgconfig).stdout)
        assert_staged_flags(flags, relocated)
        run([cxx, "-std=c++17", "-Wall", "-Wextra", "-Werror",
             str(consumer), "-o", str(root / "pkgconfig-consumer")] + flags)


def check(prefix: Path, cxx: str) -> None:
    prefix = prefix.resolve()
    include_root = prefix / "include" / "libdxfrw"
    config_root = prefix / "lib" / "cmake" / "libdxfrw"
    if not include_root.is_dir() or not config_root.is_dir():
        raise RuntimeError("prefix is missing installed headers or CMake package")
    public_facade = include_root / "libdxfrw.h"
    public_text = public_facade.read_text(encoding="utf-8")
    missing = [declaration for declaration in PROFILE_DECLARATIONS
               if declaration not in public_text]
    if missing:
        raise RuntimeError(
            "installed libdxfrw.h is missing profile declarations: "
            + ", ".join(missing))
    missing_docs = [marker for marker in PROFILE_DOCUMENTATION
                    if marker not in public_text]
    if missing_docs:
        raise RuntimeError(
            "installed libdxfrw.h is missing profile documentation: "
            + ", ".join(missing_docs))
    assert_profile_symbols(prefix)
    assert_relocatable_cmake_export(prefix)

    with tempfile.TemporaryDirectory(prefix="libdxfrw-package-") as directory:
        root = Path(directory)
        for header in PUBLIC_HEADERS:
            source = root / (header.replace("/", "_") + ".cpp")
            source_text = "#include <libdxfrw/%s>\n" % header
            if header == "libdxfrw.h":
                source_text += (
                    "static_assert(static_cast<unsigned>(dxfRW::DxfCompatibilityProfile::StandaloneSafe) == 0);\n"
                    "static_assert(static_cast<unsigned>(dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy) == 1);\n"
                )
            source_text += "int main() { return 0; }\n"
            source.write_text(source_text, encoding="utf-8")
            run([cxx, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                 "-I", str(prefix / "include"), "-c", str(source),
                 "-o", str(source.with_suffix(".o"))])

        consumer = root / "consumer.cpp"
        consumer.write_text(
            "#include <libdxfrw.h>\n"
            "static_assert(static_cast<unsigned>(dxfRW::DxfCompatibilityProfile::StandaloneSafe) == 0);\n"
            "static_assert(static_cast<unsigned>(dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy) == 1);\n"
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
        reported_prefix = run(
            ["pkg-config", "--define-prefix", "--variable=prefix", "libdxfrw"],
            env=pkgconfig).stdout.strip()
        assert_reported_prefix(reported_prefix, prefix)
        flags = shlex.split(run(["pkg-config", "--define-prefix", "--cflags", "--libs",
                                 "libdxfrw"], env=pkgconfig).stdout)
        assert_staged_flags(flags, prefix)
        run([cxx, "-std=c++17", "-Wall", "-Wextra", "-Werror",
             str(consumer), "-o", str(root / "pkgconfig-consumer")] + flags)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prefix", type=Path, action="append",
                        help="staged install prefix (repeat for isolation checks)")
    parser.add_argument("--self-test", action="store_true",
                        help="exercise staged-path acceptance and rejection")
    parser.add_argument("--relocation-smoke", action="store_true",
                        help="copy one staged prefix and build relocated consumers")
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    args = parser.parse_args()
    if args.self_test:
        self_test_staged_flags()
        self_test_package_root_identity()
        self_test_relocatable_cmake_export()
        print("staged package checker self-test: PASS")
        if not args.prefix:
            return 0
    if not args.prefix:
        parser.error("--prefix is required unless --self-test is used alone")
    prefixes = [prefix.resolve() for prefix in args.prefix]
    if len(prefixes) != len(set(prefixes)):
        parser.error("--prefix values must identify distinct staged roots")
    try:
        for prefix in prefixes:
            check(prefix, args.cxx)
        if args.relocation_smoke:
            if len(prefixes) != 1:
                parser.error("--relocation-smoke requires exactly one --prefix")
            check_relocated_consumer(prefixes[0], args.cxx)
    except subprocess.CalledProcessError as error:
        if error.output:
            print(error.output, end="")
        raise
    print("staged package check: PASS (%d prefix%s)" %
          (len(prefixes), "es" if len(prefixes) != 1 else ""))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
