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

FAST_FORBIDDEN_COMMANDS = frozenset({
    "ctest", "curl", "wget", "git", "pip", "pip3", "brew", "apt", "apt-get",
})
_FAST_COMMAND_GUARD = False

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
    if _FAST_COMMAND_GUARD and Path(command[0]).name in FAST_FORBIDDEN_COMMANDS:
        raise RuntimeError(
            "fast validation forbids external/full-suite command: %s"
            % command[0])
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


def layout_root(prefix: Path, destination: str) -> Path:
    relative = Path(destination)
    if relative.is_absolute() or ".." in relative.parts:
        raise RuntimeError("relocation check requires a relative in-prefix destination: %s"
                           % destination)
    return prefix / relative


def pkg_config_environment(prefix: Path, libdir: str) -> dict:
    environment = {key: value for key, value in os.environ.items()
                   if not key.startswith("PKG_CONFIG_")}
    directory = str(layout_root(prefix, libdir) / "pkgconfig")
    environment["PKG_CONFIG_PATH"] = directory
    environment["PKG_CONFIG_LIBDIR"] = directory
    return environment


def pkg_config_path_value(value: str) -> str:
    # pkgconf escapes spaces in --variable output; other implementations may
    # print the raw value. Resolve the path, without shell evaluation, first.
    if Path(value).exists():
        return value
    fragments = shlex.split(value)
    return fragments[0] if len(fragments) == 1 else value


def run_consumer(executable: Path, prefix: Path, libdir: str,
                 *, direct_link: bool = False) -> None:
    environment = os.environ.copy()
    for key in ("LD_LIBRARY_PATH", "DYLD_LIBRARY_PATH", "DYLD_FALLBACK_LIBRARY_PATH"):
        environment.pop(key, None)
    # Direct pkg-config link flags intentionally do not promise a runtime RPATH.
    # Restrict its loader override to this install; CONFIG consumers use their
    # imported-target build RPATH with no developer loader paths.
    if direct_link:
        environment["LD_LIBRARY_PATH"] = str(layout_root(prefix, libdir))
        environment["DYLD_LIBRARY_PATH"] = str(layout_root(prefix, libdir))
    run([str(executable)], env=environment)


def check_consumer_routes(prefix: Path, cxx: str, root: Path,
                          consumer: Path, libdir: str, includedir: str) -> None:
    prefix = prefix.resolve()
    config_root = layout_root(prefix, libdir) / "cmake" / "libdxfrw"
    pkgconfig = pkg_config_environment(prefix, libdir)
    version = run(["pkg-config", "--modversion", "libdxfrw"],
                  env=pkgconfig).stdout.strip()
    major, minor, patch = (int(part) for part in version.split("."))
    for override in (False, True):
        options = ["--define-prefix"] if override else []
        reported_prefix = run(
            ["pkg-config", *options, "--variable=prefix", "libdxfrw"],
            env=pkgconfig).stdout.strip()
        if not override:
            assert_reported_prefix(pkg_config_path_value(reported_prefix), prefix)
        elif Path(pkg_config_path_value(reported_prefix)).resolve() != prefix:
            # The optional override has a two-parent heuristic in some tools.
            # Flags must still bind correctly via independent pcfiledir paths.
            print("pkg-config --define-prefix guessed %s; checking independent flags"
                  % reported_prefix)
        reported_directory = run(
            ["pkg-config", *options, "--variable=pcfiledir", "libdxfrw"],
            env=pkgconfig).stdout.strip()
        if Path(pkg_config_path_value(reported_directory)).resolve() != (layout_root(prefix, libdir)
                                                   / "pkgconfig").resolve():
            raise RuntimeError("pkg-config loaded another package: %s" % reported_directory)
        flags = shlex.split(run(
            ["pkg-config", *options, "--cflags", "--libs", "libdxfrw"],
            env=pkgconfig).stdout)
        assert_staged_flags(flags, prefix)
        for kind, expected in (("-I", layout_root(prefix, includedir) / "libdxfrw"),
                               ("-L", layout_root(prefix, libdir))):
            actual = {Path(flag[2:]).resolve() for flag in flags if flag.startswith(kind)}
            if expected.resolve() not in actual:
                raise RuntimeError("pkg-config omitted expected %s path: %s" % (kind, expected))
        executable = root / ("pkgconfig-override" if override else "pkgconfig-plain")
        run([cxx, "-std=c++17", "-Wall", "-Wextra", "-Werror",
             str(consumer), "-o", str(executable)] + flags)
        run_consumer(executable, prefix, libdir, direct_link=True)

    cmake = root / "CMakeLists.txt"
    cmake.write_text(
        "cmake_minimum_required(VERSION 3.28)\n"
        "project(libdxfrw_staged_consumer LANGUAGES CXX)\n"
        # Both installed targets must propagate C++17, not rely on this probe.
        "set(CMAKE_CXX_STANDARD 14)\n"
        "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
        "if(DISCOVERY STREQUAL MODULE)\n"
        "  list(PREPEND CMAKE_MODULE_PATH \"${PACKAGE_CONFIG}\")\n"
        "  find_package(Libdxfrw MODULE REQUIRED)\n"
        "else()\n"
        "  find_package(libdxfrw ${REQUESTED_VERSION} CONFIG REQUIRED\n"
        "    PATHS \"${PACKAGE_CONFIG}\" NO_DEFAULT_PATH)\n"
        "endif()\n"
        "add_executable(staged_consumer consumer.cpp)\n"
        "target_link_libraries(staged_consumer PRIVATE libdxfrw::libdxfrw)\n"
        "file(GENERATE OUTPUT \"${CMAKE_BINARY_DIR}/binding.txt\" CONTENT\n"
        "  \"$<TARGET_FILE:libdxfrw::libdxfrw>\\n"
        "$<TARGET_PROPERTY:libdxfrw::libdxfrw,INTERFACE_INCLUDE_DIRECTORIES>\\n\")\n",
        encoding="utf-8")
    for discovery in ("CONFIG", "MODULE"):
        build = root / ("cmake-" + discovery.lower())
        configure = ["cmake", "-S", str(root), "-B", str(build),
                     "-DPACKAGE_CONFIG=" + str(config_root),
                     "-DDISCOVERY=" + discovery,
                     "-DREQUESTED_VERSION=%d.0" % major,
                     "-DCMAKE_PREFIX_PATH=" + str(prefix),
                     # Module discovery needs ordinary search hints for custom
                     # GNUInstallDirs layouts, not pre-filled found variables.
                     "-DCMAKE_LIBRARY_PATH=" + str(layout_root(prefix, libdir)),
                     "-DCMAKE_INCLUDE_PATH=" + str(layout_root(prefix, includedir)),
                     "-DCMAKE_FIND_ROOT_PATH=" + str(prefix),
                     "-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY",
                     "-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY",
                     "-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER",
                     "-DCMAKE_CXX_COMPILER=" + cxx]
        run(configure)
        binding = (build / "binding.txt").read_text(encoding="utf-8").splitlines()
        if len(binding) != 2:
            raise RuntimeError("CMake did not report its package binding")
        assert_staged_flags(["-L" + str(Path(binding[0]).parent)]
                            + ["-I" + item for item in binding[1].split(";")], prefix)
        expected_include = layout_root(prefix, includedir) / "libdxfrw"
        if binding[1] != str(expected_include):
            raise RuntimeError("CMake include binding differs: %s" % binding[1])
        run(["cmake", "--build", str(build), "-j2"])
        run_consumer(build / "staged_consumer", prefix, libdir)

    rejected_versions = ["%d.0" % (major + 1),
                         "%d.%d.%d" % (major, minor, patch + 1)]
    if major > 0:
        rejected_versions.append("%d.0" % (major - 1))
    for requested in rejected_versions:
        build = root / ("reject-version-" + requested)
        try:
            run(["cmake", "-S", str(root), "-B", str(build),
                 "-DPACKAGE_CONFIG=" + str(config_root), "-DDISCOVERY=CONFIG",
                 "-DREQUESTED_VERSION=" + requested, "-DCMAKE_CXX_COMPILER=" + cxx])
        except subprocess.CalledProcessError as error:
            if "version" not in error.stdout or requested not in error.stdout:
                raise RuntimeError("version rejection failed for another reason") from error
        else:
            raise RuntimeError("CMake accepted incompatible package version: %s" % requested)


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


def self_test_layout_and_discovery() -> None:
    with tempfile.TemporaryDirectory(prefix="libdxfrw-layout-self-test-") as directory:
        prefix = Path(directory) / "prefix with spaces"
        prefix.mkdir()
        for destination in ("lib", "lib64", "lib/vendor/arm64", "headers/public"):
            expected = prefix / destination
            if layout_root(prefix, destination) != expected:
                raise RuntimeError("relative package layout changed")
        for invalid in ("/usr/local/lib", "../lib", "lib/../../lib"):
            try:
                layout_root(prefix, invalid)
            except RuntimeError:
                pass
            else:
                raise RuntimeError("relocation checker accepted absolute/escaping layout")
        for spelling in (str(prefix), shlex.quote(str(prefix)),
                         str(prefix).replace(" ", "\\ ")):
            assert_reported_prefix(pkg_config_path_value(spelling), prefix)
        environment = pkg_config_environment(prefix, "lib/vendor/arm64")
        expected = str(prefix / "lib/vendor/arm64/pkgconfig")
        if environment["PKG_CONFIG_PATH"] != expected or environment["PKG_CONFIG_LIBDIR"] != expected:
            raise RuntimeError("pkg-config discovery is not isolated to the staged package")


def self_test_fast_command_scope() -> None:
    global _FAST_COMMAND_GUARD
    _FAST_COMMAND_GUARD = True
    try:
        for forbidden in sorted(FAST_FORBIDDEN_COMMANDS):
            try:
                run([forbidden, "--probe"])
            except RuntimeError as error:
                if forbidden not in str(error):
                    raise RuntimeError(
                        "fast command diagnostic omitted command: %s" % forbidden
                    ) from error
                continue
            raise RuntimeError(
                "fast command guard allowed forbidden command: %s" % forbidden)
    finally:
        _FAST_COMMAND_GUARD = False


def assert_profile_symbols(prefix: Path, libdir: str = "lib") -> None:
    library_root = layout_root(prefix, libdir)
    library_candidates = sorted(library_root.glob("libdxfrw.a"))
    if not library_candidates:
        library_candidates = sorted(library_root.glob("libdxfrw.*"))
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


def assert_relocatable_cmake_export(prefix: Path, libdir: str = "lib",
                                    includedir: str = "include") -> None:
    config_root = layout_root(prefix, libdir) / "cmake" / "libdxfrw"
    export = config_root / "libdxfrwTargets.cmake"
    if not export.is_file():
        raise RuntimeError("CMake export is missing libdxfrwTargets.cmake")
    text = export.read_text(encoding="utf-8")
    if '"${_IMPORT_PREFIX}/%s/libdxfrw"' % includedir not in text:
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


def check_relocated_consumer(prefix: Path, cxx: str, libdir: str = "lib",
                             includedir: str = "include") -> None:
    """Build one minimal consumer after copying the install to a new root."""
    prefix = prefix.resolve()
    if not (layout_root(prefix, includedir) / "libdxfrw").is_dir():
        raise RuntimeError("relocation source is missing installed headers")
    with tempfile.TemporaryDirectory(prefix="libdxfrw-relocated-") as directory:
        root = Path(directory)
        relocated = root / "moved prefix with spaces"
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
        assert_relocatable_cmake_export(relocated, libdir, includedir)
        check_consumer_routes(relocated, cxx, root, consumer, libdir, includedir)


def check(prefix: Path, cxx: str, libdir: str = "lib",
          includedir: str = "include") -> None:
    prefix = prefix.resolve()
    include_root = layout_root(prefix, includedir) / "libdxfrw"
    config_root = layout_root(prefix, libdir) / "cmake" / "libdxfrw"
    if not include_root.is_dir() or not config_root.is_dir():
        raise RuntimeError("prefix is missing installed headers or CMake package")
    hidden_allocator = include_root / "handle_allocator.h"
    if hidden_allocator.exists():
        raise RuntimeError(
            "implementation-only HandleAllocator header was installed: %s"
            % hidden_allocator)
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
    assert_profile_symbols(prefix, libdir)
    assert_relocatable_cmake_export(prefix, libdir, includedir)

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
            if header == "drw_base.h":
                source_text += (
                    "static_assert(static_cast<unsigned>(DwgDataStorageWriterOperation::None) == 0);\n"
                    "static_assert(static_cast<unsigned>(DwgDataStorageWriterOperation::WriteVxTableRecord) == 33);\n"
                )
            source_text += "int main() { return 0; }\n"
            source.write_text(source_text, encoding="utf-8")
            run([cxx, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                 "-I", str(layout_root(prefix, includedir)), "-c", str(source),
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
        check_consumer_routes(prefix, cxx, root, consumer, libdir, includedir)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prefix", type=Path, action="append",
                        help="staged install prefix (repeat for isolation checks)")
    parser.add_argument("--self-test", action="store_true",
                        help="exercise staged-path acceptance and rejection")
    parser.add_argument("--relocation-smoke", action="store_true",
                        help="copy one staged prefix and build relocated consumers")
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    parser.add_argument("--libdir", default="lib",
                        help="relative GNUInstallDirs library destination")
    parser.add_argument("--includedir", default="include",
                        help="relative GNUInstallDirs header destination")
    args = parser.parse_args()
    if args.self_test:
        self_test_staged_flags()
        self_test_package_root_identity()
        self_test_layout_and_discovery()
        self_test_relocatable_cmake_export()
        self_test_fast_command_scope()
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
            check(prefix, args.cxx, args.libdir, args.includedir)
        if args.relocation_smoke:
            if len(prefixes) != 1:
                parser.error("--relocation-smoke requires exactly one --prefix")
            check_relocated_consumer(prefixes[0], args.cxx,
                                     args.libdir, args.includedir)
    except subprocess.CalledProcessError as error:
        if error.output:
            print(error.output, end="")
        raise
    print("staged package check: PASS (%d prefix%s)" %
          (len(prefixes), "es" if len(prefixes) != 1 else ""))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
