#!/usr/bin/env python3
"""Check installed licenses/consumer interfaces and opt-in configure contracts."""

from __future__ import annotations

import argparse
import hashlib
import os
import shlex
import shutil
import subprocess
import sys
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

LICENSE_RELATIVE = Path("licenses") / "libdxfrw" / "COPYING"
LICENSE_SOURCE = Path(__file__).resolve().parents[1] / "COPYING"

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


def assert_installed_license(prefix: Path, datadir: str = "share") -> str:
    license_path = layout_root(prefix.resolve(), datadir) / LICENSE_RELATIVE
    if not license_path.is_file():
        raise RuntimeError("installed COPYING is missing: %s" % license_path)
    try:
        license_path.resolve().relative_to(prefix.resolve())
    except ValueError as error:
        raise RuntimeError("installed COPYING resolves outside staged prefix") from error
    expected = LICENSE_SOURCE.read_bytes()
    actual = license_path.read_bytes()
    expected_digest = hashlib.sha256(expected).hexdigest()
    if actual != expected or hashlib.sha256(actual).hexdigest() != expected_digest:
        raise RuntimeError("installed COPYING differs from source content: %s" % license_path)
    return expected_digest


def assert_install_manifest(manifest: Path, prefix: Path, datadir: str = "share",
                            *, recorded_prefix: Path | None = None) -> None:
    """Check actual installed files, rebasing the original manifest after a move."""
    prefix = prefix.resolve()
    recorded = (recorded_prefix or prefix).resolve()
    expected_license = layout_root(prefix, datadir) / LICENSE_RELATIVE
    paths = []
    for entry in manifest.read_text(encoding="utf-8").splitlines():
        path = Path(entry)
        if not path.is_absolute():
            raise RuntimeError("install manifest contains a nonabsolute path: %s" % entry)
        try:
            # Normalize parent aliases (e.g. macOS /var -> /private/var), but
            # keep a library symlink's own filename as a manifest identity.
            relative = (path.parent.resolve() / path.name).relative_to(recorded)
            installed = prefix / relative
            installed.resolve().relative_to(prefix)
        except ValueError as error:
            raise RuntimeError("install manifest path escapes staged prefix: %s" % entry) from error
        if not installed.is_file():
            raise RuntimeError("install manifest file is missing: %s" % installed)
        paths.append(installed)
    if expected_license not in paths:
        raise RuntimeError("install manifest omits installed COPYING")


def assert_runtime_only_package(prefix: Path, libdir: str, includedir: str) -> None:
    for path in (layout_root(prefix, includedir) / "libdxfrw",
                 layout_root(prefix, libdir) / "cmake" / "libdxfrw",
                 layout_root(prefix, libdir) / "pkgconfig" / "libdxfrw.pc"):
        if path.exists():
            raise RuntimeError("runtime-only install contains development files: %s" % path)


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


def copy_staged_install(prefix: Path, destination: Path) -> None:
    # Preserve versioned-library aliases and expose nonrelocatable absolute
    # links to the moved-prefix checks instead of hiding them as copied bytes.
    shutil.copytree(prefix, destination, symlinks=True)


def select_package_library(prefix: Path, libdir: str) -> tuple[Path, bool]:
    """Require one Unix library profile; versioned symlink aliases are one file."""
    library_root = layout_root(prefix.resolve(), libdir)
    archive = library_root / "libdxfrw.a"
    shared = sorted({path for pattern in ("libdxfrw.so", "libdxfrw.so.*",
                                          "libdxfrw.dylib", "libdxfrw.*.dylib")
                     for path in library_root.glob(pattern) if path.is_file()})
    if archive.is_file() and shared:
        raise RuntimeError("mixed static/shared prefix: use a fresh one-profile install")
    if archive.is_file():
        selected, is_static = archive, True
    elif shared:
        if len({path.resolve() for path in shared}) != 1:
            raise RuntimeError("multiple shared-library variants in staged prefix")
        selected, is_static = shared[0], False
    else:
        raise RuntimeError("staged prefix has no Unix libdxfrw archive/shared library")
    try:
        selected.resolve().relative_to(prefix.resolve())
    except ValueError as error:
        raise RuntimeError("selected library resolves outside staged prefix") from error
    return selected, is_static


def bind_archive_flags(flags: list[str], archive: Path) -> list[str]:
    """--static selects dependency closure, not the linker library variant."""
    if flags.count("-ldxfrw") != 1:
        raise RuntimeError("pkg-config must identify exactly one -ldxfrw library")
    return [str(archive) if flag == "-ldxfrw" else flag for flag in flags]


def pkg_config_flag_sets(environment: dict, options: list[str], *,
                         static: bool = False,
                         module: str = "libdxfrw") -> tuple[list[str], list[str]]:
    command = ["pkg-config", *options, *(["--static"] if static else [])]
    return tuple(shlex.split(run([*command, part, module], env=environment).stdout)
                 for part in ("--cflags", "--libs"))


def compile_pkg_config_consumer(cxx: str, consumer: Path, executable: Path,
                                cflags: list[str], libs: list[str]) -> None:
    # Exercise compilation separately: linker-only success must not hide lost
    # Cflags. No global -static is used; only libdxfrw's archive is selected.
    obj = executable.with_suffix(".o")
    run([cxx, "-std=c++17", "-Wall", "-Wextra", "-Werror", "-c",
         str(consumer), "-o", str(obj)] + cflags)
    run([cxx, str(obj), "-o", str(executable)] + libs)


def check_private_dependency_negative(prefix: Path, cxx: str, root: Path,
                                      libdir: str, includedir: str,
                                      library: Path, is_static: bool) -> None:
    """A scratch metadata control, never a mutation of the installed package."""
    directory = root / "private-dependency-negative"
    directory.mkdir()
    missing = "libdxfrw_missing_private_" + hashlib.sha256(
        str(root).encode("utf-8")).hexdigest()[:16]
    pc = directory / "libdxfrw-private-probe.pc"
    pc.write_text(
        "prefix=%s\nlibdir=${prefix}/%s\nincludedir=${prefix}/%s\n"
        "Name: libdxfrw-private-probe\nDescription: generated dependency negative\n"
        "Version: 1.0.0\nLibs: -L\"${libdir}\" -ldxfrw\n"
        "Libs.private: -l%s\n"
        "Cflags: -I\"${includedir}/libdxfrw\" -DLIBDXFRW_PRIVATE_PROBE=1\n"
        % (prefix.as_posix(), libdir, includedir, missing), encoding="utf-8")
    environment = pkg_config_environment(prefix, libdir)
    environment["PKG_CONFIG_PATH"] = str(directory)
    environment["PKG_CONFIG_LIBDIR"] = str(directory)
    consumer = directory / "consumer.cpp"
    consumer.write_text(
        "#include <libdxfrw.h>\n#ifndef LIBDXFRW_PRIVATE_PROBE\n"
        "#error missing pkg-config compile flags\n#endif\n"
        "int main() { dxfRW codec(\"\"); return codec.getError(); }\n",
        encoding="utf-8")
    for override in (False, True):
        options = ["--define-prefix"] if override else []
        # The fixture intentionally uses absolute paths outside pcfiledir. An
        # explicit override must not rewrite them to its scratch module root.
        options += ["--define-variable=prefix=" + prefix.as_posix()]
        _, ordinary = pkg_config_flag_sets(environment, options,
            module="libdxfrw-private-probe")
        cflags, libs = pkg_config_flag_sets(environment, options, static=True,
            module="libdxfrw-private-probe")
        if "-l" + missing in ordinary or "-l" + missing not in libs:
            raise RuntimeError("private dependency negative did not distinguish static closure")
        assert_staged_flags(cflags + libs, prefix)
        if is_static:
            libs = bind_archive_flags(libs, library)
        executable = directory / ("override" if override else "plain")
        try:
            compile_pkg_config_consumer(cxx, consumer, executable, cflags, libs)
        except subprocess.CalledProcessError as error:
            if missing not in (error.stdout or ""):
                raise RuntimeError("private dependency negative failed for another reason") from error
        else:
            raise RuntimeError("static route accepted a missing private library")
    print("pkg-config missing-private-library controls: PASS (plain/override)")


def check_package_requests(prefix: Path, cxx: str, root: Path,
                           libdir: str, includedir: str, library: Path,
                           version: str) -> None:
    """Configure request controls against this install; no library rebuild."""
    directory = root / "package-requests"
    directory.mkdir()
    (directory / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.28)\n"
        "project(package_requests LANGUAGES CXX)\n"
        "if(DISCOVERY STREQUAL MODULE)\n"
        " list(PREPEND CMAKE_MODULE_PATH \"${PACKAGE_CONFIG}\")\n"
        " find_package(Libdxfrw ${REQUESTED_VERSION} ${EXACT_ARGUMENT}\n"
        "   MODULE ${REQUIREMENT} ${COMPONENT_ARGUMENT})\n"
        " set(request_found ${Libdxfrw_FOUND})\n"
        "else()\n"
        " find_package(libdxfrw ${REQUESTED_VERSION} ${EXACT_ARGUMENT}\n"
        "   CONFIG ${REQUIREMENT} ${COMPONENT_ARGUMENT}\n"
        "   PATHS \"${PACKAGE_CONFIG}\" NO_DEFAULT_PATH)\n"
        " set(request_found ${libdxfrw_FOUND})\n"
        "endif()\n"
        "if(EXPECT_NOT_FOUND)\n"
        " if(request_found)\n"
        "  message(FATAL_ERROR \"unexpected accepted package request\")\n"
        " endif()\n"
        " message(STATUS \"request-not-found\")\n"
        " return()\n"
        "endif()\n"
        "if(NOT request_found)\n"
        " message(FATAL_ERROR \"valid package request rejected\")\n"
        "endif()\n"
        "if(DISCOVERY STREQUAL MODULE)\n"
        " if(NOT TARGET Libdxfrw::Libdxfrw)\n"
        "  message(FATAL_ERROR \"missing module target alias\")\n"
        " endif()\n"
        " file(GENERATE OUTPUT \"${CMAKE_BINARY_DIR}/alias.txt\" CONTENT\n"
        "   \"$<TARGET_FILE:Libdxfrw::Libdxfrw>\\n\")\n"
        "endif()\n"
        "file(GENERATE OUTPUT \"${CMAKE_BINARY_DIR}/binding.txt\" CONTENT\n"
        " \"$<TARGET_FILE:libdxfrw::libdxfrw>\\n"
        "$<TARGET_PROPERTY:libdxfrw::libdxfrw,INTERFACE_INCLUDE_DIRECTORIES>\\n\")\n",
        encoding="utf-8")
    major, minor, patch = (int(part) for part in version.split("."))
    cases = [
        ("unversioned", "", "", "", True),
        ("compatible", "%d.0" % major, "", "", True),
        ("exact", version, "EXACT", "", True),
        ("short-exact", "%d.%d" % (major, minor), "EXACT", "", False),
        ("newer-major", "%d.0" % (major + 1), "", "", False),
        ("newer-patch", "%d.%d.%d" % (major, minor, patch + 1), "", "", False),
        ("wrong-exact", "%d.%d.%d" % (major, minor, patch + 1), "EXACT", "", False),
        ("inclusive", version + "..." + version, "", "", True),
        ("exclusive-next-major", version + "...<%d.0" % (major + 1), "", "", True),
        ("inclusive-next-major", version + "...%d.0" % (major + 1), "", "", False),
        ("required-component", "", "", "COMPONENTS;nonexistent_libdxfrw_component", False),
        ("optional-component", "", "", "OPTIONAL_COMPONENTS;nonexistent_libdxfrw_component", True),
    ]
    if major > 0:
        cases += [("older-major", "%d.0" % (major - 1), "", "", False),
                  ("zero-version", "0.0", "", "", False)]
    for discovery in ("CONFIG", "MODULE"):
        build = directory / discovery.lower()
        for name, requested, exact, component, expected in cases:
            for requirement in (("REQUIRED",) if expected else ("REQUIRED", "QUIET")):
                command = ["cmake", "-S", str(directory), "-B", str(build),
                    "-DPACKAGE_CONFIG=" + str(layout_root(prefix, libdir) / "cmake/libdxfrw"),
                    "-DDISCOVERY=" + discovery, "-DREQUESTED_VERSION=" + requested,
                    "-DEXACT_ARGUMENT=" + exact, "-DCOMPONENT_ARGUMENT=" + component,
                    "-DREQUIREMENT=" + requirement,
                    "-DEXPECT_NOT_FOUND=" + ("OFF" if expected else "ON"),
                    "-DCMAKE_CXX_COMPILER=" + cxx,
                    "-DCMAKE_LIBRARY_PATH=" + str(layout_root(prefix, libdir)),
                    "-DCMAKE_INCLUDE_PATH=" + str(layout_root(prefix, includedir)),
                    "-DCMAKE_FIND_ROOT_PATH=" + str(prefix),
                    "-DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY",
                    "-DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY",
                    "-DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER"]
                try:
                    result = run(command)
                except subprocess.CalledProcessError as error:
                    if expected or requirement != "REQUIRED":
                        raise
                    token = "nonexistent_libdxfrw_component" if component else requested
                    if token not in error.stdout or "unexpected accepted" in error.stdout:
                        raise RuntimeError("package request failed for another reason: " + name) from error
                    continue
                if not expected:
                    if requirement == "REQUIRED" or "request-not-found" not in result.stdout:
                        raise RuntimeError("package accepted invalid request: " + name)
                    continue
                binding = (build / "binding.txt").read_text(encoding="utf-8").splitlines()
                if (len(binding) != 2 or Path(binding[0]).resolve() != library.resolve()
                    or binding[1] != str(layout_root(prefix, includedir) / "libdxfrw")):
                    raise RuntimeError("request control selected another package: " + name)
                if discovery == "MODULE" and Path((build / "alias.txt").read_text().strip()).resolve() != library.resolve():
                    raise RuntimeError("module alias selected another library")
        print("CMake %s package requests: PASS (%d controls, required/quiet negatives)"
              % (discovery, len(cases)))
    check_package_request_boundaries(prefix, cxx, directory, libdir, includedir, library)


def check_package_request_boundaries(prefix: Path, cxx: str, root: Path,
                                     libdir: str, includedir: str, library: Path) -> None:
    """Identity/cache/quiet controls; mutate only owned copies of one install."""
    source = root / "boundaries"
    source.mkdir()
    (source / "CMakeLists.txt").write_text('''cmake_minimum_required(VERSION 3.28)
project(package_boundaries LANGUAGES CXX)
set(libdxfrw_DIR "${HOST_DIR}" CACHE PATH "host sentinel" FORCE)
set(PACKAGE_FIND_VERSION "caller-version-sentinel")
set(PACKAGE_VERSION_COMPATIBLE "caller-compat-sentinel")
set(PACKAGE_VERSION_EXACT TRUE CACHE BOOL "caller exact sentinel" FORCE)
set(Libdxfrw_VERSION "caller cached version" CACHE STRING "caller version sentinel" FORCE)
set(libdxfrw_FIND_REQUIRED "caller-required-sentinel")
if(COLLISION)
 add_library(${COLLISION} UNKNOWN IMPORTED)
 set_target_properties(${COLLISION} PROPERTIES IMPORTED_LOCATION "${HOST_DIR}/wrong.a"
   INTERFACE_INCLUDE_DIRECTORIES "${HOST_DIR}/wrong-include")
endif()
if(DISABLE_THREADS)
 set(CMAKE_DISABLE_FIND_PACKAGE_Threads TRUE)
endif()
if(NEW_MANUAL_MAP)
 set(CMAKE_BUILD_TYPE Debug)
 if(NEW_MANUAL_MAP STREQUAL empty)
  set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG "")
 else()
  set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG UNAVAILABLE)
 endif()
endif()
if(DISCOVERY STREQUAL MODULE)
 list(PREPEND CMAKE_MODULE_PATH "${PACKAGE_CONFIG}")
 find_package(Libdxfrw ${REQUESTED_VERSION} ${EXACT_ARGUMENT} MODULE QUIET)
 set(request_found ${Libdxfrw_FOUND})
 if(REPEAT AND request_found)
  find_package(Libdxfrw MODULE REQUIRED OPTIONAL_COMPONENTS nonexistent_libdxfrw_component)
  find_package(Libdxfrw MODULE REQUIRED)
  if(NOT Libdxfrw_FOUND OR Libdxfrw_nonexistent_libdxfrw_component_FOUND)
   message(FATAL_ERROR "repeat component state leaked")
  endif()
 endif()
else()
 # CONFIG itself owns its DIR variable; MODULE must leave the host cache alone.
 set(libdxfrw_DIR "${PACKAGE_CONFIG}" CACHE PATH "host sentinel" FORCE)
 find_package(libdxfrw CONFIG QUIET PATHS "${PACKAGE_CONFIG}" NO_DEFAULT_PATH)
 set(request_found ${libdxfrw_FOUND})
 if(REPEAT AND request_found)
  find_package(libdxfrw CONFIG REQUIRED OPTIONAL_COMPONENTS nonexistent_libdxfrw_component)
  find_package(libdxfrw CONFIG REQUIRED)
  if(NOT libdxfrw_FOUND OR libdxfrw_nonexistent_libdxfrw_component_FOUND)
   message(FATAL_ERROR "repeat component state leaked")
  endif()
 endif()
endif()
if(DISCOVERY STREQUAL MODULE)
 get_property(dir_value CACHE libdxfrw_DIR PROPERTY VALUE)
 get_property(dir_type CACHE libdxfrw_DIR PROPERTY TYPE)
 get_property(dir_help CACHE libdxfrw_DIR PROPERTY HELPSTRING)
 if(NOT dir_value STREQUAL HOST_DIR OR NOT dir_type STREQUAL "PATH"
    OR NOT dir_help STREQUAL "host sentinel")
  message(FATAL_ERROR "module changed caller DIR cache")
 endif()
 if(NOT PACKAGE_FIND_VERSION STREQUAL "caller-version-sentinel"
    OR NOT PACKAGE_VERSION_COMPATIBLE STREQUAL "caller-compat-sentinel"
    OR NOT libdxfrw_FIND_REQUIRED STREQUAL "caller-required-sentinel")
  message(FATAL_ERROR "module leaked version/config variables")
 endif()
 get_property(cached_exact CACHE PACKAGE_VERSION_EXACT PROPERTY VALUE)
 get_property(cached_version CACHE Libdxfrw_VERSION PROPERTY VALUE)
 if(NOT cached_exact OR NOT cached_version STREQUAL "caller cached version")
  message(FATAL_ERROR "module changed caller version cache")
 endif()
 if(MISSING_METADATA AND NOT "${Libdxfrw_VERSION}" STREQUAL "")
  message(FATAL_ERROR "manual fallback advertised inherited cached version")
 endif()
endif()
if(EXPECT_FOUND)
 if(NOT request_found)
  message(FATAL_ERROR "valid boundary request rejected")
 endif()
else()
 if(request_found)
  message(FATAL_ERROR "unexpected accepted boundary request")
 endif()
endif()
message(STATUS "boundary-control-pass")
''', encoding="utf-8")
    host = root / "host sentinel"
    host.mkdir()
    (host / "libdxfrwConfig.cmake").write_text(
        'message(FATAL_ERROR "host package executed")\n', encoding="utf-8")
    manual = root / "manual prefix"
    copy_staged_install(prefix, manual)
    manual_config = layout_root(manual, libdir) / "cmake/libdxfrw"
    (manual_config / "libdxfrwConfigVersion.cmake").unlink()
    unsuitable = root / "unsuitable prefix"
    copy_staged_install(prefix, unsuitable)
    unsuitable_version = layout_root(unsuitable, libdir) / "cmake/libdxfrw/libdxfrwConfigVersion.cmake"
    unsuitable_version.write_text(unsuitable_version.read_text(encoding="utf-8")
                                  + '\nset(PACKAGE_VERSION_UNSUITABLE TRUE)\n', encoding="utf-8")
    contract = root / "version contract prefix"
    copy_staged_install(prefix, contract)
    contract_version = layout_root(contract, libdir) / "cmake/libdxfrw/libdxfrwConfigVersion.cmake"
    version = run(["pkg-config", "--modversion", "libdxfrw"],
                  env=pkg_config_environment(prefix, libdir)).stdout.strip()
    major, minor, patch = (int(value) for value in version.split("."))
    lower = version + ".0"
    upper = "%d.0.0.0" % (major + 1)
    complete = lower + "...<" + upper
    contract_fields = {
        "NAME": "libdxfrw", "VERSION": lower, "VERSION_COMPLETE": complete,
        "VERSION_MAJOR": str(major), "VERSION_MINOR": str(minor),
        "VERSION_PATCH": str(patch), "VERSION_TWEAK": "0", "VERSION_COUNT": "4",
        "VERSION_RANGE": complete, "VERSION_RANGE_MIN": "INCLUDE", "VERSION_RANGE_MAX": "EXCLUDE",
        "VERSION_MIN": lower, "VERSION_MIN_MAJOR": str(major), "VERSION_MIN_MINOR": str(minor),
        "VERSION_MIN_PATCH": str(patch), "VERSION_MIN_TWEAK": "0", "VERSION_MIN_COUNT": "4",
        "VERSION_MAX": upper, "VERSION_MAX_MAJOR": str(major + 1), "VERSION_MAX_MINOR": "0",
        "VERSION_MAX_PATCH": "0", "VERSION_MAX_TWEAK": "0", "VERSION_MAX_COUNT": "4",
    }
    contract_checks = "".join(
        'if(NOT "${PACKAGE_FIND_%s}" STREQUAL "%s")\n'
        ' message(FATAL_ERROR "version contract input %s differs")\nendif()\n' % (key, value, key)
        for key, value in contract_fields.items())
    contract_version.write_text(contract_checks + contract_version.read_text(encoding="utf-8"), encoding="utf-8")
    mixed_headers = host / "headers"
    mixed_headers.mkdir()
    shutil.copyfile(layout_root(prefix, includedir) / "libdxfrw/drw_base.h", mixed_headers / "drw_base.h")
    cases = [
        ("host-repeat", prefix, "MODULE", "", "", False, True, True),
        ("config-repeat", prefix, "CONFIG", "", "", False, True, True),
        ("canonical-module-collision", prefix, "MODULE", "", "libdxfrw::libdxfrw", False, False, False),
        ("alias-module-collision", prefix, "MODULE", "", "Libdxfrw::Libdxfrw", False, False, False),
        ("canonical-config-collision", prefix, "CONFIG", "", "libdxfrw::libdxfrw", False, False, False),
        ("alias-config-collision", prefix, "CONFIG", "", "Libdxfrw::Libdxfrw", False, False, False),
        ("missing-version-unversioned", manual, "MODULE", "", "", False, True, True),
        ("missing-version-request", manual, "MODULE", "2.0", "", False, False, False),
        ("missing-version-zero", manual, "MODULE", "0.0", "", False, False, False),
        ("manual-target-collision", manual, "MODULE", "", "libdxfrw::libdxfrw", False, False, False),
        ("unsuitable-unversioned", unsuitable, "MODULE", "", "", False, False, False),
        ("threads-quiet", prefix, "MODULE", "", "", True, False, False),
        ("version-contract", contract, "MODULE", complete, "", False, False, True),
        ("mixed-headers", prefix, "MODULE", "", "", False, False, False),
        ("stale-exact-cache", prefix, "MODULE", "%d.%d" % (major, minor), "", False, False, False),
        ("new-manual-empty-map", manual, "MODULE", "", "", False, True, True),
        ("new-manual-unavailable-map", manual, "MODULE", "", "", False, False, False),
    ]
    for name, selected, discovery, requested, collision, disabled, repeat, expected in cases:
        result = run(["cmake", "-S", str(source), "-B", str(source / name),
            "-DPACKAGE_CONFIG=" + str(layout_root(selected, libdir) / "cmake/libdxfrw"),
            "-DDISCOVERY=" + discovery, "-DREQUESTED_VERSION=" + requested,
            "-DEXACT_ARGUMENT=" + ("EXACT" if name == "stale-exact-cache" else ""),
            "-DMISSING_METADATA=" + ("ON" if selected == manual else "OFF"),
            "-DHOST_DIR=" + str(host), "-DCOLLISION=" + collision,
            "-DDISABLE_THREADS=" + ("ON" if disabled else "OFF"),
            "-DNEW_MANUAL_MAP=" + ("empty" if name == "new-manual-empty-map" else
                                   "unavailable" if name == "new-manual-unavailable-map" else ""),
            "-DREPEAT=" + ("ON" if repeat else "OFF"),
            "-DEXPECT_FOUND=" + ("ON" if expected else "OFF"),
            "-DLibdxfrw_INCLUDE_DIR=" + str(mixed_headers if name == "mixed-headers"
                                           else layout_root(selected, includedir) / "libdxfrw"),
            "-DLibdxfrw_LIBRARY=" + str(layout_root(selected, libdir) / library.name),
            "-DCMAKE_CXX_COMPILER=" + cxx])
        if "boundary-control-pass" not in result.stdout or "host package executed" in result.stdout:
            raise RuntimeError("package boundary control did not complete: " + name)
    print("CMake package boundaries: PASS (%d controls; cache/quiet/identity/repeat)" % len(cases))
    check_package_mapping_boundaries(prefix, cxx, root, libdir, includedir, library)


def check_package_mapping_boundaries(prefix: Path, cxx: str, root: Path,
                                     libdir: str, includedir: str, library: Path) -> None:
    """Retain OLD mappings, with consensus admission on capable runtimes."""
    source = root / "mapping-boundaries"
    source.mkdir()
    installed = prefix
    manual = source / "legacy manual prefix"
    copy_staged_install(prefix, manual)
    (layout_root(manual, libdir) / "cmake/libdxfrw/libdxfrwConfigVersion.cmake").unlink()
    prefix = manual
    library = layout_root(prefix, libdir) / library.name
    (source / "CMakeLists.txt").write_text('''cmake_minimum_required(VERSION 3.28)
project(package_mapping LANGUAGES CXX)
cmake_policy(SET CMP0111 OLD) # Observe missing locations without generating uses.
if(POLICY CMP0200)
 cmake_policy(SET CMP0200 OLD)
endif()
add_library(libdxfrw::libdxfrw UNKNOWN IMPORTED)
set(CMAKE_BUILD_TYPE Debug)
set_target_properties(libdxfrw::libdxfrw PROPERTIES
 INTERFACE_INCLUDE_DIRECTORIES "${Libdxfrw_INCLUDE_DIR}"
 IMPORTED_CONFIGURATIONS RELEASE
 IMPORTED_LOCATION "${SELECTED_LIBRARY}"
 IMPORTED_LOCATION_RELEASE "${SELECTED_LIBRARY}")
if(MAPPING_CASE STREQUAL unavailable)
 set_property(TARGET libdxfrw::libdxfrw PROPERTY MAP_IMPORTED_CONFIG_DEBUG UNAVAILABLE)
elseif(MAPPING_CASE STREQUAL default-first OR MAPPING_CASE STREQUAL release-first
       OR MAPPING_CASE STREQUAL middle-empty)
 set_property(TARGET libdxfrw::libdxfrw PROPERTY IMPORTED_LOCATION_RELEASE "${WRONG_LIBRARY}")
 if(MAPPING_CASE STREQUAL default-first)
  set(_mapping ";RELEASE")
 elseif(MAPPING_CASE STREQUAL release-first)
  set(_mapping "RELEASE;")
 else()
  set(_mapping "UNAVAILABLE;;RELEASE")
 endif()
 set_property(TARGET libdxfrw::libdxfrw PROPERTY MAP_IMPORTED_CONFIG_DEBUG "${_mapping}")
elseif(MAPPING_CASE STREQUAL lowercase)
 set_property(TARGET libdxfrw::libdxfrw PROPERTY MAP_IMPORTED_CONFIG_DEBUG release)
 set_property(TARGET libdxfrw::libdxfrw PROPERTY IMPORTED_LOCATION "${WRONG_LIBRARY}")
elseif(MAPPING_CASE STREQUAL empty OR MAPPING_CASE STREQUAL empty-no-default)
 set_property(TARGET libdxfrw::libdxfrw PROPERTY MAP_IMPORTED_CONFIG_DEBUG "")
 if(MAPPING_CASE STREQUAL empty-no-default)
  set_property(TARGET libdxfrw::libdxfrw PROPERTY IMPORTED_LOCATION)
 else()
  set_property(TARGET libdxfrw::libdxfrw PROPERTY IMPORTED_LOCATION_RELEASE "${WRONG_LIBRARY}")
 endif()
elseif(MAPPING_CASE STREQUAL unmapped-default)
 set_property(TARGET libdxfrw::libdxfrw PROPERTY IMPORTED_LOCATION_RELEASE "${WRONG_LIBRARY}")
elseif(MAPPING_CASE STREQUAL unmapped-exact OR MAPPING_CASE STREQUAL noconfig)
 if(MAPPING_CASE STREQUAL noconfig)
  set(CMAKE_BUILD_TYPE "")
  set(_config NOCONFIG)
 else()
  set(_config DEBUG)
 endif()
 set_property(TARGET libdxfrw::libdxfrw PROPERTY IMPORTED_LOCATION_${_config} "${SELECTED_LIBRARY}")
 set_property(TARGET libdxfrw::libdxfrw PROPERTY IMPORTED_LOCATION "${WRONG_LIBRARY}")
 set_property(TARGET libdxfrw::libdxfrw PROPERTY IMPORTED_LOCATION_RELEASE "${WRONG_LIBRARY}")
elseif(MAPPING_CASE STREQUAL noconfig-map)
 set(CMAKE_BUILD_TYPE "")
 set_property(TARGET libdxfrw::libdxfrw PROPERTY MAP_IMPORTED_CONFIG_NOCONFIG Release)
 set_property(TARGET libdxfrw::libdxfrw PROPERTY IMPORTED_LOCATION "${WRONG_LIBRARY}")
elseif(MAPPING_CASE STREQUAL empty-location)
 set_property(TARGET libdxfrw::libdxfrw PROPERTY MAP_IMPORTED_CONFIG_DEBUG "DEBUG;RELEASE")
 set_property(TARGET libdxfrw::libdxfrw PROPERTY IMPORTED_LOCATION_DEBUG "")
endif()
list(PREPEND CMAKE_MODULE_PATH "${PACKAGE_CONFIG}")
find_package(Libdxfrw MODULE QUIET)
_libdxfrw_target_artifact(libdxfrw::libdxfrw DEBUG OLD selected_old)
if(MAPPING_CASE STREQUAL noconfig OR MAPPING_CASE STREQUAL noconfig-map)
 _libdxfrw_target_artifact(libdxfrw::libdxfrw NOCONFIG OLD selected_old)
endif()
if(EXPECT_OLD AND NOT selected_old STREQUAL SELECTED_LIBRARY)
 message(FATAL_ERROR "OLD selector lost its accepted mapping: ${MAPPING_CASE}")
elseif(NOT EXPECT_OLD AND selected_old STREQUAL SELECTED_LIBRARY)
 message(FATAL_ERROR "OLD selector accepted an invalid mapping: ${MAPPING_CASE}")
endif()
if(POLICY CMP0200)
 set(expected ${EXPECT_CONSENSUS})
else()
 set(expected ${EXPECT_OLD})
endif()
if(NOT "${Libdxfrw_FOUND}" STREQUAL "${expected}")
 message(FATAL_ERROR "mapping request result differs: ${MAPPING_CASE}")
endif()
if(Libdxfrw_FOUND)
 file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/mapping-binding.txt"
  CONTENT "$<TARGET_FILE:libdxfrw::libdxfrw>\\n$<TARGET_FILE:Libdxfrw::Libdxfrw>\\n")
endif()
message(STATUS "mapping-control-pass")
''', encoding="utf-8")
    wrong = source / "wrong.a"
    wrong.write_bytes(b"identity sentinel, never linked\n")
    cases = (("unavailable", False, False), ("default-first", True, False),
             ("release-first", False, False), ("middle-empty", True, False),
             ("lowercase", True, True), ("empty", True, False),
             ("empty-no-default", False, False), ("unmapped-default", True, False),
             ("unmapped-exact", True, False), ("noconfig", True, False),
             ("noconfig-map", True, True), ("empty-location", False, False))
    build = source / "build"
    for name, old, consensus in cases:
        result = run(["cmake", "-S", str(source), "-B", str(build),
            "-DPACKAGE_CONFIG=" + str(Path(__file__).resolve().parents[1] / "cmake"),
            "-DSELECTED_LIBRARY=" + str(library.resolve()), "-DWRONG_LIBRARY=" + str(wrong.resolve()),
            "-DMAPPING_CASE=" + name, "-DEXPECT_OLD=" + ("TRUE" if old else "FALSE"),
            "-DEXPECT_CONSENSUS=" + ("TRUE" if consensus else "FALSE"),
            "-DLibdxfrw_LIBRARY=" + str(library),
            "-DLibdxfrw_INCLUDE_DIR=" + str(layout_root(prefix, includedir) / "libdxfrw"),
            "-DCMAKE_CXX_COMPILER=" + cxx])
        if "mapping-control-pass" not in result.stdout:
            raise RuntimeError("mapping control did not complete: " + name)
        if consensus:
            locations = (build / "mapping-binding.txt").read_text(encoding="utf-8").splitlines()
            if len(locations) != 2 or any(Path(value).resolve() != library.resolve() for value in locations):
                raise RuntimeError("mapping control selected another library: " + name)
    print("CMake legacy manual mappings: PASS (%d OLD controls; policy-consensus admission)" % len(cases))
    check_package_policy_boundaries(manual, installed, cxx, root, libdir, includedir, library)


def check_package_policy_boundaries(manual: Path, installed: Path, cxx: str, root: Path,
                                    libdir: str, includedir: str, library: Path) -> None:
    """Native creation-policy observations plus narrow artifact admission."""
    source = root / "policy-boundaries"
    source.mkdir()
    (source / "CMakeLists.txt").write_text('''cmake_minimum_required(VERSION 3.28)
project(package_policy LANGUAGES CXX)
cmake_policy(SET CMP0111 OLD) # Test-only missing-location observations.
set(CMAKE_BUILD_TYPE Debug)
set(_config DEBUG)
if(POLICY CMP0200)
 cmake_policy(SET CMP0200 ${CREATION})
endif()
if(EXPORTED)
 find_package(libdxfrw CONFIG REQUIRED PATHS "${PACKAGE_CONFIG}" NO_DEFAULT_PATH)
 set(_target libdxfrw::libdxfrw)
elseif(ALIAS_CONTROL)
 add_library(underlying ${KIND} IMPORTED)
 add_library(libdxfrw::libdxfrw ALIAS underlying)
 set(_target underlying)
else()
 add_library(libdxfrw::libdxfrw ${KIND} IMPORTED)
 set(_target libdxfrw::libdxfrw)
endif()
set_target_properties(${_target} PROPERTIES
 INTERFACE_INCLUDE_DIRECTORIES "${Libdxfrw_INCLUDE_DIR}"
 IMPORTED_LOCATION "${SELECTED_LIBRARY}")
if(NOT SCENARIO STREQUAL default-only AND NOT SCENARIO STREQUAL no-list-map)
 set_target_properties(${_target} PROPERTIES IMPORTED_CONFIGURATIONS RELEASE
  IMPORTED_LOCATION_RELEASE "${WRONG_LIBRARY}")
endif()
if(SCENARIO STREQUAL leading-empty)
 set_property(TARGET ${_target} PROPERTY MAP_IMPORTED_CONFIG_DEBUG ";RELEASE")
elseif(SCENARIO STREQUAL mapped-provided OR SCENARIO STREQUAL no-list-map)
 set_property(TARGET ${_target} PROPERTY MAP_IMPORTED_CONFIG_DEBUG release)
 set_property(TARGET ${_target} PROPERTY IMPORTED_LOCATION "${WRONG_LIBRARY}")
 set_property(TARGET ${_target} PROPERTY IMPORTED_LOCATION_RELEASE "${SELECTED_LIBRARY}")
elseif(SCENARIO STREQUAL unavailable)
 set_property(TARGET ${_target} PROPERTY MAP_IMPORTED_CONFIG_DEBUG UNAVAILABLE)
elseif(SCENARIO STREQUAL new-default-fallback OR SCENARIO STREQUAL empty-list)
 set_property(TARGET ${_target} PROPERTY MAP_IMPORTED_CONFIG_DEBUG RELEASE)
 set_property(TARGET ${_target} PROPERTY IMPORTED_LOCATION_RELEASE)
 if(SCENARIO STREQUAL empty-list)
  set_property(TARGET ${_target} PROPERTY IMPORTED_CONFIGURATIONS "")
 endif()
elseif(SCENARIO STREQUAL empty-location)
 set_property(TARGET ${_target} PROPERTY IMPORTED_CONFIGURATIONS "DEBUG;RELEASE")
 set_property(TARGET ${_target} PROPERTY MAP_IMPORTED_CONFIG_DEBUG "DEBUG;RELEASE")
 set_property(TARGET ${_target} PROPERTY IMPORTED_LOCATION_DEBUG "")
 set_property(TARGET ${_target} PROPERTY IMPORTED_LOCATION_RELEASE "${SELECTED_LIBRARY}")
elseif(SCENARIO STREQUAL noconfig)
 set(CMAKE_BUILD_TYPE "")
 set(_config NOCONFIG)
 set_property(TARGET ${_target} PROPERTY IMPORTED_LOCATION "${WRONG_LIBRARY}")
 set_property(TARGET ${_target} PROPERTY IMPORTED_LOCATION_NOCONFIG "${SELECTED_LIBRARY}")
elseif(SCENARIO STREQUAL unadvertised-map)
 set_property(TARGET ${_target} PROPERTY MAP_IMPORTED_CONFIG_DEBUG DEBUG)
 set_property(TARGET ${_target} PROPERTY IMPORTED_LOCATION_DEBUG "${SELECTED_LIBRARY}")
elseif(SCENARIO STREQUAL same-artifact)
 set_property(TARGET ${_target} PROPERTY IMPORTED_LOCATION_RELEASE "${SELECTED_LIBRARY}")
elseif(SCENARIO STREQUAL shared-importlib)
 set_property(TARGET ${_target} PROPERTY IMPORTED_CONFIGURATIONS "")
 set_property(TARGET ${_target} PROPERTY IMPORTED_LOCATION "${WRONG_LIBRARY}")
 set_property(TARGET ${_target} PROPERTY IMPORTED_IMPLIB "${SELECTED_LIBRARY}")
elseif(SCENARIO STREQUAL wrong-importlib OR SCENARIO STREQUAL ignored-importlib)
 set_property(TARGET ${_target} PROPERTY IMPORTED_CONFIGURATIONS "")
 set_property(TARGET ${_target} PROPERTY IMPORTED_IMPLIB "${WRONG_LIBRARY}")
elseif(SCENARIO STREQUAL dll-no-importlib OR SCENARIO STREQUAL empty-compat)
 set_property(TARGET ${_target} PROPERTY IMPORTED_CONFIGURATIONS "")
elseif(SCENARIO STREQUAL separators-list)
 set_property(TARGET ${_target} PROPERTY IMPORTED_CONFIGURATIONS ";;")
 set_property(TARGET ${_target} PROPERTY MAP_IMPORTED_CONFIG_DEBUG RELEASE)
 set_property(TARGET ${_target} PROPERTY IMPORTED_LOCATION_RELEASE "${SELECTED_LIBRARY}")
elseif(SCENARIO STREQUAL multi-good OR SCENARIO STREQUAL multi-bad)
 set_property(TARGET ${_target} PROPERTY IMPORTED_CONFIGURATIONS "DEBUG;RELEASE")
 set_property(TARGET ${_target} PROPERTY IMPORTED_LOCATION_DEBUG "${SELECTED_LIBRARY}")
 if(SCENARIO STREQUAL multi-good)
  set_property(TARGET ${_target} PROPERTY IMPORTED_LOCATION_RELEASE "${SELECTED_LIBRARY}")
 endif()
endif()
if(OLD_EMPTY_IMPLEMENTATION)
 set(CMAKE_VERSION 4.4.3) # Source-derived implementation branch, not that runtime.
endif()
if(POLICY CMP0200 AND CMAKE_VERSION VERSION_LESS 4.4.4)
 get_target_property(_advertised ${_target} IMPORTED_CONFIGURATIONS)
 if(_advertised STREQUAL "")
  set(EXPECT_NEW MISSING)
  set(EXPECT_CONSENSUS FALSE)
 endif()
endif()
if(POLICY CMP0200)
 if(CREATION STREQUAL OLD)
  cmake_policy(SET CMP0200 NEW)
 else()
  cmake_policy(SET CMP0200 OLD)
 endif()
endif()
if(WINDOWS_METADATA)
 set(WIN32 TRUE) # Typed artifact branch only; no native Windows qualification.
endif()
list(PREPEND CMAKE_MODULE_PATH "${MODULE_DIR}")
find_package(Libdxfrw MODULE QUIET)
foreach(policy OLD NEW)
 if(policy STREQUAL NEW AND NOT POLICY CMP0200)
  continue()
 endif()
 _libdxfrw_target_artifact(${_target} "${_config}" ${policy} artifact)
 set(expected "${EXPECT_${policy}}")
 if(expected STREQUAL A)
  set(expected "${SELECTED_LIBRARY}")
 elseif(expected STREQUAL B)
  set(expected "${WRONG_LIBRARY}")
 else()
  set(expected "")
 endif()
 if(NOT artifact STREQUAL expected)
  message(FATAL_ERROR "selector mismatch ${SCENARIO}/${policy}: ${artifact} != ${expected}")
 endif()
endforeach()
if(OLD_ONLY_CONTROL OR NOT POLICY CMP0200)
 set(expected_found ${EXPECT_OLD_FOUND})
else()
 set(expected_found ${EXPECT_CONSENSUS})
endif()
if(NOT "${Libdxfrw_FOUND}" STREQUAL "${expected_found}")
 message(FATAL_ERROR "admission mismatch ${SCENARIO}: ${Libdxfrw_FOUND}")
endif()
if(NATIVE_OBSERVATION OR (EXPORTED AND POLICY CMP0200 AND NOT EXPECT_NEW STREQUAL MISSING))
 get_target_property(native ${_target} LOCATION_${_config})
 if(POLICY CMP0200)
  set(expected "${EXPECT_${CREATION}}")
 else()
  set(expected "${EXPECT_OLD}")
 endif()
 if(expected STREQUAL A)
  set(expected "${SELECTED_LIBRARY}")
 elseif(expected STREQUAL B)
  set(expected "${WRONG_LIBRARY}")
 elseif(native MATCHES "-NOTFOUND$")
  set(expected "${native}")
 else()
  message(FATAL_ERROR "native missing-location observation differs")
 endif()
 if(NOT native STREQUAL expected)
  message(FATAL_ERROR "native creation policy differs ${SCENARIO}/${CREATION}")
 endif()
endif()
if(Libdxfrw_FOUND)
 find_package(Libdxfrw MODULE REQUIRED)
 if(NOT TARGET Libdxfrw::Libdxfrw)
  message(FATAL_ERROR "missing accepted alias")
 endif()
endif()
message(STATUS "policy-control-pass ${SCENARIO}/${CREATION}")
if(POLICY CMP0200)
 message(STATUS "policy-capable-runtime")
endif()
''', encoding="utf-8")
    wrong = source / "unrelated.a"
    wrong.write_bytes(b"identity sentinel, never linked\n")
    references = (("default-only", "A", "A"), ("default-vs-release", "A", "B"),
                  ("leading-empty", "A", "B"), ("mapped-provided", "A", "A"),
                  ("unavailable", "MISSING", "MISSING"),
                  ("new-default-fallback", "MISSING", "A"),
                  ("empty-location", "MISSING", "MISSING"),
                  ("noconfig", "A", "B"), ("empty-list", "MISSING", "A"))
    module = Path(__file__).resolve().parents[1] / "cmake"
    old_only = source / "old-only-module"
    old_only.mkdir()
    module_text = (module / "FindLibdxfrw.cmake").read_text(encoding="utf-8")
    capability_branch = "        if(POLICY CMP0200)\n"
    if module_text.count(capability_branch) != 1:
        raise RuntimeError("cannot isolate the no-CMP0200 capability branch")
    (old_only / "FindLibdxfrw.cmake").write_text(
        module_text.replace(capability_branch, "        if(FALSE)\n"), encoding="utf-8")
    cases = [(name, creation, old, new, "UNKNOWN", False, False, False, True)
             for name, old, new in references for creation in ("OLD", "NEW")]
    cases += [(name, creation, old, new, kind, alias, exported, windows, False)
              for name, old, new, kind, alias, exported, windows in (
                  ("unadvertised-map", "A", "MISSING", "UNKNOWN", False, False, False),
                  ("no-list-map", "A", "A", "UNKNOWN", False, False, False),
                  ("separators-list", "A", "MISSING", "UNKNOWN", False, False, False),
                  ("same-artifact", "A", "A", "UNKNOWN", True, False, False),
                  ("shared-importlib", "A", "A", "SHARED", False, False, True),
                  ("wrong-importlib", "B", "B", "SHARED", False, False, True),
                  ("ignored-importlib", "A", "A", "STATIC", False, False, True),
                  ("ignored-importlib", "A", "A", "UNKNOWN", True, False, True),
                  ("dll-no-importlib", "MISSING", "MISSING", "SHARED", False, False, True))
              for creation in ("OLD", "NEW")]
    cases += [("empty-compat", "OLD", "A", "MISSING", "UNKNOWN", False, False, False, False)]
    # Use the real current installed export, not a guessed policy upper bound.
    cases += [(name, "NEW", old, new, "STATIC", False, True, False, False)
              for name, old, new in (("same-artifact", "A", "A"),
                                     ("leading-empty", "A", "B"),
                                     ("unavailable", "MISSING", "MISSING"),
                                     ("new-default-fallback", "MISSING", "A"))]
    cases += [("oldonly-" + name, "OLD", old, new, "UNKNOWN", False, False, False, True)
              for name, old, new in (("leading-empty", "A", "B"),
                                     ("default-only", "A", "A"),
                                     ("unavailable", "MISSING", "MISSING"))]
    if shutil.which("ninja"):
        cases += [("multi-good", "NEW", "A", "A", "STATIC", False, False, False, False),
                  ("multi-bad", "NEW", "A", "A", "STATIC", False, False, False, False)]
    for name, creation, old, new, kind, alias, actual, windows, native in cases:
        selected = installed if actual else manual
        selected_library = layout_root(selected, libdir) / library.name
        multi = name.startswith("multi-")
        old_branch = name.startswith("oldonly-")
        scenario = name.removeprefix("oldonly-")
        extra = (["-G", "Ninja Multi-Config", "-DCMAKE_CONFIGURATION_TYPES=Debug;Release"]
                 if multi else [])
        # The Debug reference still agrees in multi-bad: only Release differs.
        # Admission must examine all enabled configurations, not just Debug.
        good = name != "multi-bad"
        result = run(["cmake", "-S", str(source), "-B", str(source / ("multi" if multi else "build")),
            "-DMODULE_DIR=" + str(old_only if old_branch else module),
            "-DPACKAGE_CONFIG=" + str(layout_root(selected, libdir) / "cmake/libdxfrw"),
            "-DLibdxfrw_INCLUDE_DIR=" + str(layout_root(selected, includedir) / "libdxfrw"),
            "-DLibdxfrw_LIBRARY=" + str(selected_library),
            "-DSELECTED_LIBRARY=" + str(selected_library.resolve()),
            "-DWRONG_LIBRARY=" + str(wrong.resolve()), "-DCREATION=" + creation,
            "-DSCENARIO=" + scenario, "-DKIND=" + kind,
            "-DOLD_ONLY_CONTROL=" + ("ON" if old_branch else "OFF"),
            "-DOLD_EMPTY_IMPLEMENTATION=" + ("ON" if name == "empty-compat" else "OFF"),
            "-DALIAS_CONTROL=" + ("ON" if alias else "OFF"),
            "-DEXPORTED=" + ("ON" if actual else "OFF"),
            "-DWINDOWS_METADATA=" + ("ON" if windows else "OFF"),
            "-DNATIVE_OBSERVATION=" + ("ON" if native else "OFF"),
            "-DEXPECT_OLD=" + old, "-DEXPECT_NEW=" + new,
            "-DEXPECT_OLD_FOUND=" + ("TRUE" if good and old == "A" else "FALSE"),
            "-DEXPECT_CONSENSUS=" + ("TRUE" if good and old == new == "A" else "FALSE"),
            "-DCMAKE_CXX_COMPILER=" + cxx] + extra)
        if "policy-control-pass " + scenario + "/" + creation not in result.stdout:
            raise RuntimeError("policy control did not complete: " + name)
    native_scope = ("18 opposite-caller native observations" if "policy-capable-runtime" in result.stdout
                    else "OLD-only runtime; NEW/opposite-caller unavailable")
    print("CMake policy/artifact controls: PASS (%d; %s; 4 forced capability/implementation-branch controls are not older-runtime runs; Windows metadata only)"
          % (len(cases), native_scope))


def check_consumer_routes(prefix: Path, cxx: str, root: Path,
                          consumer: Path, libdir: str, includedir: str) -> None:
    prefix = prefix.resolve()
    config_root = layout_root(prefix, libdir) / "cmake" / "libdxfrw"
    library, is_static = select_package_library(prefix, libdir)
    assert_profile_symbols(prefix, libdir, library=library)
    print("selected %s: %s SHA-256 %s" % (
        "static archive" if is_static else "shared library", library,
        hashlib.sha256(library.read_bytes()).hexdigest()))
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
        for static in (False, True):
            cflags, libs = pkg_config_flag_sets(pkgconfig, options, static=static)
            flags = cflags + libs
            assert_staged_flags(flags, prefix)
            for kind, expected in (("-I", layout_root(prefix, includedir) / "libdxfrw"),
                                   ("-L", layout_root(prefix, libdir))):
                actual = {Path(flag[2:]).resolve() for flag in flags if flag.startswith(kind)}
                if expected.resolve() not in actual:
                    raise RuntimeError("pkg-config omitted expected %s path: %s" % (kind, expected))
            if is_static and not static:
                # --libs alone intentionally omits private dependencies. Do
                # not require those incomplete flags to link a static install.
                continue
            if is_static:
                libs = bind_archive_flags(libs, library)
            route = ("override" if override else "plain") + ("-static" if static else "")
            executable = root / ("pkgconfig-" + route)
            compile_pkg_config_consumer(cxx, consumer, executable, cflags, libs)
            run_consumer(executable, prefix, libdir, direct_link=not is_static)
            print("pkg-config %s: PASS (%s)" % (route,
                "archive-bound" if is_static else "shared artifact; static metadata only" if static else "shared"))

    check_private_dependency_negative(prefix, cxx, root, libdir, includedir,
                                      library, is_static)

    cmake = root / "CMakeLists.txt"
    cmake.write_text(
        "cmake_minimum_required(VERSION 3.28)\n"
        "project(libdxfrw_staged_consumer LANGUAGES CXX)\n"
        # Both installed targets must propagate C++17, not rely on this probe.
        "set(CMAKE_CXX_STANDARD 14)\n"
        "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
        "if(DISCOVERY STREQUAL MODULE)\n"
        "  list(PREPEND CMAKE_MODULE_PATH \"${PACKAGE_CONFIG}\")\n"
        "  find_package(Libdxfrw ${REQUESTED_VERSION} MODULE REQUIRED)\n"
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
        if Path(binding[0]).resolve() != library.resolve():
            raise RuntimeError("CMake and pkg-config selected different library variants")
        assert_staged_flags(["-L" + str(Path(binding[0]).parent)]
                            + ["-I" + item for item in binding[1].split(";")], prefix)
        expected_include = layout_root(prefix, includedir) / "libdxfrw"
        if binding[1] != str(expected_include):
            raise RuntimeError("CMake include binding differs: %s" % binding[1])
        run(["cmake", "--build", str(build), "-j2"])
        run_consumer(build / "staged_consumer", prefix, libdir)

    check_package_requests(prefix, cxx, root, libdir, includedir, library, version)


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


def self_test_static_library_policy() -> None:
    # No compiler/pkg-config dependency: runtime-only/self-test stays usable
    # with a nonexistent CXX. Actual dependency negatives run in dev checks.
    with tempfile.TemporaryDirectory(prefix="libdxfrw-variant-self-test-") as directory:
        root = Path(directory)
        prefix = root / "prefix with spaces"
        library_root = prefix / "lib/vendor"
        library_root.mkdir(parents=True)
        archive = library_root / "libdxfrw.a"
        archive.write_bytes(b"archive selection fixture")
        selected, is_static = select_package_library(prefix, "lib/vendor")
        assert is_static and selected.resolve() == archive.resolve()
        flags = ["-L" + str(library_root), "-ldxfrw", "-pthread"]
        assert bind_archive_flags(flags, archive) == [flags[0], str(archive), "-pthread"]
        for invalid in ([], ["-lother"], ["-ldxfrw", "-ldxfrw"]):
            try:
                bind_archive_flags(invalid, archive)
            except RuntimeError:
                pass
            else:
                raise RuntimeError("archive binding accepted an absent/ambiguous library flag")
        shared = library_root / "libdxfrw.so.2"
        shared.write_bytes(b"shared selection fixture")
        try:
            select_package_library(prefix, "lib/vendor")
        except RuntimeError as error:
            if "mixed static/shared" not in str(error):
                raise
        else:
            raise RuntimeError("one-profile policy accepted a mixed archive/shared prefix")
        archive.unlink()
        selected, is_static = select_package_library(prefix, "lib/vendor")
        assert not is_static and selected.resolve() == shared.resolve()
        alias = library_root / "libdxfrw.so"
        try:
            alias.symlink_to(shared.name)
        except OSError:
            # File-selection logic itself remains testable on platforms where
            # unprivileged symlink creation is unavailable; no native claim.
            pass
        else:
            selected, is_static = select_package_library(prefix, "lib/vendor")
            assert not is_static and selected.resolve() == shared.resolve()
            moved = root / "moved shared prefix"
            copy_staged_install(prefix, moved)
            moved_alias = moved / "lib/vendor" / alias.name
            if not moved_alias.is_symlink():
                raise RuntimeError("relocation dereferenced a shared-library alias")
            selected, is_static = select_package_library(moved, "lib/vendor")
            assert not is_static and selected.resolve() == (
                moved / "lib/vendor" / shared.name).resolve()
            alias.unlink()
        second = library_root / "libdxfrw.so.3"
        second.write_bytes(b"different shared version")
        try:
            select_package_library(prefix, "lib/vendor")
        except RuntimeError as error:
            if "multiple shared-library variants" not in str(error):
                raise
        else:
            raise RuntimeError("library selection accepted distinct shared variants")


def self_test_installed_license() -> None:
    expected = LICENSE_SOURCE.read_bytes()
    with tempfile.TemporaryDirectory(prefix="libdxfrw-license-self-test-") as directory:
        root = Path(directory)
        prefix = root / "original prefix"
        license_path = prefix / "share" / LICENSE_RELATIVE
        license_path.parent.mkdir(parents=True)
        manifest = root / "install_manifest.txt"
        manifest.write_text(str(license_path) + "\n", encoding="utf-8")
        for bad_content in (None, expected[:-1], expected + b"\n",
                            expected.replace(b"\n", b"\r\n")):
            if bad_content is not None:
                license_path.write_bytes(bad_content)
            try:
                assert_installed_license(prefix)
            except RuntimeError:
                pass
            else:
                raise RuntimeError("license checker accepted missing/modified COPYING")
        license_path.write_bytes(expected)
        assert assert_installed_license(prefix) == hashlib.sha256(expected).hexdigest()
        assert_install_manifest(manifest, prefix)
        assert_runtime_only_package(prefix, "lib", "include")
        moved = root / "moved prefix"
        shutil.copytree(prefix, moved)
        assert_installed_license(moved)
        assert_install_manifest(manifest, moved, recorded_prefix=prefix)
        for invalid in ("", "relative/COPYING\n", str(root / "outside") + "\n"):
            manifest.write_text(invalid, encoding="utf-8")
            try:
                assert_install_manifest(manifest, prefix)
            except RuntimeError:
                pass
            else:
                raise RuntimeError("license checker accepted an invalid/missing manifest entry")
        manifest.write_text(str(license_path) + "\n" + str(prefix / "missing") + "\n",
                            encoding="utf-8")
        try:
            assert_install_manifest(manifest, prefix)
        except RuntimeError:
            pass
        else:
            raise RuntimeError("license checker accepted a missing manifest file")
        leaked = prefix / "include" / "libdxfrw"
        leaked.mkdir(parents=True)
        try:
            assert_runtime_only_package(prefix, "lib", "include")
        except RuntimeError:
            pass
        else:
            raise RuntimeError("runtime-only checker accepted installed development headers")


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


def self_test_thread_metadata(cxx: str) -> None:
    """Configure three forced metadata branches, never build the library."""
    if os.name != "posix":
        raise RuntimeError("thread metadata controls require a Unix compiler; no native Windows claim")
    source = Path(__file__).resolve().parents[1]
    cases = (
        ("flag", ["-DCMAKE_HAVE_LIBC_PTHREAD=FALSE",
                  "-DTHREADS_HAVE_PTHREAD_ARG=TRUE"], "-pthread", "-pthread"),
        ("libc-empty", ["-DCMAKE_HAVE_LIBC_PTHREAD=TRUE",
                        "-DTHREADS_HAVE_PTHREAD_ARG=FALSE"], None, None),
        ("library-only", ["-DCMAKE_HAVE_LIBC_PTHREAD=FALSE",
                          "-DTHREADS_HAVE_PTHREAD_ARG=FALSE",
                          "-DCMAKE_HAVE_PTHREADS_CREATE=FALSE",
                          "-DCMAKE_HAVE_PTHREAD_CREATE=TRUE"], None, "-lpthread"),
    )
    with tempfile.TemporaryDirectory(prefix="libdxfrw-thread-metadata-") as directory:
        for name, variables, compile_flag, link_flag in cases:
            build = Path(directory) / name
            try:
                run(["cmake", "-S", str(source), "-B", str(build),
                     "-DLIBDXFRW_BUILD_DOC=OFF", "-DLIBDXFRW_BUILD_DWG2DXF=OFF",
                     "-DLIBDXFRW_BUILD_TESTS=OFF", "-DCMAKE_CXX_COMPILER=" + cxx,
                     *variables])
                environment = pkg_config_environment(build, "lib")
                environment["PKG_CONFIG_PATH"] = str(build)
                environment["PKG_CONFIG_LIBDIR"] = str(build)
                for static in (False, True):
                    cflags, libs = pkg_config_flag_sets(environment, [], static=static)
                    actual_compile = {flag for flag in cflags if flag in ("-pthread", "-lpthread")}
                    actual_link = {flag for flag in libs if flag in ("-pthread", "-lpthread")}
                    expected_compile = {compile_flag} if compile_flag else set()
                    expected_link = {link_flag} if static and link_flag else set()
                    if actual_compile != expected_compile or actual_link != expected_link:
                        raise RuntimeError(
                            "thread metadata %s static=%s: cflags=%r libs=%r"
                            % (name, static, cflags, libs))
            except subprocess.CalledProcessError as error:
                if error.output:
                    print(error.output, end="")
                raise
            print("thread metadata %s: PASS (forced configure/query control)" % name)
    print("thread metadata self-test: PASS (no native platform qualification)")


BUILD_OPTION_CASES = (
    ("tests-without-library", False, False, True,
     "LIBDXFRW_BUILD_TESTS requires LIBDXFRW_BUILD_LIBRARY"),
    ("tests-with-library", True, False, True, None),
    ("all-disabled", False, False, False, None),
    ("cli-without-library", False, True, False,
     "LIBDXFRW_BUILD_DWG2DXF requires LIBDXFRW_BUILD_LIBRARY"),
)


def build_option_command(source: Path, build: Path, cxx: str, python: str,
                         library: bool, cli: bool, tests: bool) -> list[str]:
    def option(value: bool) -> str:
        return "ON" if value else "OFF"
    return ["cmake", "-S", str(source), "-B", str(build),
            "-DCMAKE_CXX_COMPILER=" + cxx,
            "-DLIBDXFRW_PYTHON_EXECUTABLE=" + python,
            "-DLIBDXFRW_BUILD_LIBRARY=" + option(library),
            "-DLIBDXFRW_BUILD_DWG2DXF=" + option(cli),
            "-DLIBDXFRW_BUILD_TESTS=" + option(tests),
            "-DLIBDXFRW_BUILD_DOC=OFF", "-DLIBDXFRW_BUILD_LONG_FUZZ=OFF",
            "-DLIBDXFRW_BUILD_QUALIFICATION=OFF",
            "-DLIBDXFRW_ENABLE_INDEPENDENT_DXF_ORACLE=OFF"]


def assert_build_option_outcome(status: int, output: str,
                                diagnostic: str | None) -> None:
    if diagnostic is None:
        if status != 0 or "Generating done" not in output:
            raise RuntimeError("supported build options did not configure/generate successfully")
    elif (status == 0 or diagnostic not in output
          or any(marker in output for marker in
                 ("Configuring done", "Generating done", "CMake Generate step failed"))):
        raise RuntimeError("build options did not fail early with: " + diagnostic)


def self_test_build_option_contract() -> None:
    """Test the configure-result contract without invoking any compiler/tool."""
    for _, library, cli, tests, diagnostic in BUILD_OPTION_CASES:
        command = build_option_command(Path("source with spaces"), Path("build with spaces"),
                                       "nonexistent cxx", "explicit python", library, cli, tests)
        if command[:5] != ["cmake", "-S", "source with spaces", "-B", "build with spaces"]:
            raise RuntimeError("build-option configure command split a path")
        for control in ("-DCMAKE_CXX_COMPILER=nonexistent cxx",
                        "-DLIBDXFRW_PYTHON_EXECUTABLE=explicit python",
                        "-DLIBDXFRW_BUILD_DOC=OFF", "-DLIBDXFRW_BUILD_LONG_FUZZ=OFF",
                        "-DLIBDXFRW_BUILD_QUALIFICATION=OFF",
                        "-DLIBDXFRW_ENABLE_INDEPENDENT_DXF_ORACLE=OFF"):
            if control not in command:
                raise RuntimeError("build-option configure command lost: " + control)
        assert_build_option_outcome(1 if diagnostic else 0,
                                    diagnostic or "Configuring done\nGenerating done", diagnostic)
    diagnostic = BUILD_OPTION_CASES[0][-1]
    for status, output, expected in (
            (0, diagnostic, diagnostic), (1, "unrelated compiler failure", diagnostic),
            (1, diagnostic + "\nConfiguring done", diagnostic),
            (1, diagnostic + "\nCMake Generate step failed", diagnostic),
            (1, "Generating done", None), (0, "configure incomplete", None)):
        try:
            assert_build_option_outcome(status, output, expected)
        except RuntimeError:
            continue
        raise RuntimeError("build-option checker accepted an incorrect configure outcome")


def self_test_build_options(cxx: str) -> None:
    """Exercise four real configure-only option profiles; never build targets."""
    # Keep ordinary --self-test compiler-free. Only this explicit opt-in runs
    # CMake/compiler discovery, using the invoking Python rather than PATH's
    # possibly older python3 when the tests-enabled positive is configured.
    from bounded_process import run_bounded
    source = Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory(prefix="libdxfrw-build-options-") as directory:
        for name, library, cli, tests, diagnostic in BUILD_OPTION_CASES:
            build = Path(directory) / name
            command = build_option_command(source, build, cxx, sys.executable, library, cli, tests)
            result = run_bounded(command, timeout=60, output_limit=131072)
            output = (result.stdout + result.stderr).decode("utf-8", "replace")
            try:
                assert_build_option_outcome(result.returncode, output, diagnostic)
                if diagnostic is not None and any(path.exists() for path in (
                        build / "CMakeFiles" / "TargetDirectories.txt",
                        build / "CTestTestfile.cmake")):
                    raise RuntimeError("rejected options created a target/test graph")
            except RuntimeError:
                print(output, end="")
                raise
            print("build options %s: PASS (configure-only control)" % name)
    print("build options self-test: PASS (4 profiles; no library/format build)")


def assert_profile_symbols(prefix: Path, libdir: str = "lib", *,
                           library: Path | None = None) -> None:
    selected, _ = select_package_library(prefix, libdir)
    if library is not None and library.resolve() != selected.resolve():
        raise RuntimeError("symbol inspection selected a different library variant")
    nm = shutil.which("nm")
    cxxfilt = shutil.which("c++filt")
    if nm is None or cxxfilt is None:
        raise RuntimeError("nm and c++filt are required for symbol checks")
    symbols = subprocess.run(
        [nm, "-g", str(selected)], check=True,
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
                             includedir: str = "include", datadir: str = "share",
                             *, runtime_only: bool = False,
                             manifest: Path | None = None) -> None:
    """Build one minimal consumer after copying the install to a new root."""
    prefix = prefix.resolve()
    if not runtime_only and not (layout_root(prefix, includedir) / "libdxfrw").is_dir():
        raise RuntimeError("relocation source is missing installed headers")
    with tempfile.TemporaryDirectory(prefix="libdxfrw-relocated-") as directory:
        root = Path(directory)
        relocated = root / "moved prefix with spaces"
        copy_staged_install(prefix, relocated)
        assert_installed_license(relocated, datadir)
        if manifest is not None:
            assert_install_manifest(manifest, relocated, datadir, recorded_prefix=prefix)
        if runtime_only:
            assert_runtime_only_package(relocated, libdir, includedir)
            return
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
          includedir: str = "include", datadir: str = "share",
          *, runtime_only: bool = False, manifest: Path | None = None) -> None:
    prefix = prefix.resolve()
    license_digest = assert_installed_license(prefix, datadir)
    if manifest is not None:
        assert_install_manifest(manifest, prefix, datadir)
    print("installed COPYING SHA-256: " + license_digest)
    if runtime_only:
        assert_runtime_only_package(prefix, libdir, includedir)
        return
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
    parser.add_argument("--thread-metadata-self-test", action="store_true",
                        help="opt-in configure/pkg-config checks for three forced thread branches")
    parser.add_argument("--build-options-self-test", action="store_true",
                        help="opt-in configure-only checks for four library/CLI/test profiles")
    parser.add_argument("--relocation-smoke", action="store_true",
                        help="copy one staged prefix and build relocated consumers")
    parser.add_argument("--cxx", default=os.environ.get("CXX", "c++"))
    parser.add_argument("--libdir", default="lib",
                        help="relative GNUInstallDirs library destination")
    parser.add_argument("--includedir", default="include",
                        help="relative GNUInstallDirs header destination")
    parser.add_argument("--datadir", default="share",
                        help="relative GNUInstallDirs data destination")
    parser.add_argument("--runtime-only", action="store_true",
                        help="check license/manifest and absent development files without compiling")
    parser.add_argument("--install-manifest", type=Path,
                        help="CMake install_manifest.txt for exactly one staged prefix")
    args = parser.parse_args()
    if args.thread_metadata_self_test:
        if args.runtime_only:
            parser.error("thread metadata controls cannot be combined with --runtime-only")
        self_test_thread_metadata(args.cxx)
    if args.build_options_self_test:
        if args.runtime_only:
            parser.error("build option controls cannot be combined with --runtime-only")
        self_test_build_options(args.cxx)
    if ((args.thread_metadata_self_test or args.build_options_self_test)
            and not args.prefix and not args.self_test):
        return 0
    if args.self_test:
        self_test_staged_flags()
        self_test_package_root_identity()
        self_test_layout_and_discovery()
        self_test_static_library_policy()
        self_test_relocatable_cmake_export()
        self_test_installed_license()
        self_test_fast_command_scope()
        self_test_build_option_contract()
        print("staged package checker self-test: PASS")
        if not args.prefix:
            return 0
    if not args.prefix:
        parser.error("--prefix is required unless --self-test is used alone")
    prefixes = [prefix.resolve() for prefix in args.prefix]
    if len(prefixes) != len(set(prefixes)):
        parser.error("--prefix values must identify distinct staged roots")
    if args.runtime_only and args.install_manifest is None:
        parser.error("--runtime-only requires --install-manifest")
    if args.install_manifest is not None and len(prefixes) != 1:
        parser.error("--install-manifest requires exactly one --prefix")
    try:
        for prefix in prefixes:
            check(prefix, args.cxx, args.libdir, args.includedir, args.datadir,
                  runtime_only=args.runtime_only, manifest=args.install_manifest)
        if args.relocation_smoke:
            if len(prefixes) != 1:
                parser.error("--relocation-smoke requires exactly one --prefix")
            check_relocated_consumer(prefixes[0], args.cxx,
                                     args.libdir, args.includedir, args.datadir,
                                     runtime_only=args.runtime_only,
                                     manifest=args.install_manifest)
    except subprocess.CalledProcessError as error:
        if error.output:
            print(error.output, end="")
        raise
    print("staged package check: PASS (%d prefix%s)" %
          (len(prefixes), "es" if len(prefixes) != 1 else ""))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
