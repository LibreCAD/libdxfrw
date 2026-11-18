#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Verify and record one installed libdxfrw package after prefix relocation."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
from tempfile import TemporaryDirectory


CMAKE = r'''cmake_minimum_required(VERSION 3.28)
project(installed_package_probe LANGUAGES CXX)
find_package(libdxfrw ${REQUESTED_VERSION} CONFIG REQUIRED
             NO_DEFAULT_PATH PATHS "${PROBE_PREFIX}")
if(NOT TARGET libdxfrw::libdxfrw)
    message(FATAL_ERROR "installed package did not define libdxfrw::libdxfrw")
endif()
get_target_property(_type libdxfrw::libdxfrw TYPE)
if(EXPECT_KIND STREQUAL "static" AND NOT _type STREQUAL "STATIC_LIBRARY")
    message(FATAL_ERROR "expected static package, imported target type is ${_type}")
endif()
if(EXPECT_KIND STREQUAL "shared" AND NOT _type STREQUAL "SHARED_LIBRARY")
    message(FATAL_ERROR "expected shared package, imported target type is ${_type}")
endif()
file(REAL_PATH "${PROBE_PREFIX}" _prefix)
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/selected-target.txt"
     CONTENT "$<TARGET_FILE:libdxfrw::libdxfrw>\n")
add_executable(installed_package_probe probe.cpp)
target_compile_features(installed_package_probe PRIVATE cxx_std_17)
target_compile_definitions(installed_package_probe PRIVATE
                           PROBE_SHARED=$<STREQUAL:${EXPECT_KIND},shared>)
target_link_libraries(installed_package_probe PRIVATE
                      libdxfrw::libdxfrw ${CMAKE_DL_LIBS})
'''

CPP = r'''#if defined(__linux__) && !defined(_GNU_SOURCE)
#  define _GNU_SOURCE
#endif
#include <libdxfrw.h>
#include <filesystem>
#include <iostream>
#include <string>
#if PROBE_SHARED
#  if defined(_WIN32)
#    include <windows.h>
#  elif defined(__APPLE__)
#    include <mach-o/dyld.h>
#  else
#    include <link.h>
#  endif
#endif

#if PROBE_SHARED && !defined(_WIN32) && !defined(__APPLE__)
static int locate(dl_phdr_info* info, size_t, void* result) {
    const std::string path = info->dlpi_name ? info->dlpi_name : "";
    if (path.find("libdxfrw.so.3") != std::string::npos) {
        *static_cast<std::string*>(result) = path;
        return 1;
    }
    return 0;
}
#endif

int main() {
    dxfRW codec("");
    if (codec.getError() != DRW::BAD_NONE) return 1;
#if PROBE_SHARED
    std::string path;
#  if defined(_WIN32)
    HMODULE module = GetModuleHandleA("dxfrw-3.dll");
    char name[32768] = {};
    const DWORD size = module ? GetModuleFileNameA(module, name, sizeof(name)) : 0;
    if (size == 0 || size >= sizeof(name)) return 2;
    path.assign(name, size);
#  elif defined(__APPLE__)
    for (uint32_t i = 0; i < _dyld_image_count(); ++i) {
        const char* name = _dyld_get_image_name(i);
        if (name && std::string(name).find("libdxfrw.3.") != std::string::npos)
            path = name;
    }
#  else
    dl_iterate_phdr(locate, &path);
#  endif
    if (path.empty()) return 3;
    std::cout << "LOADED=" << std::filesystem::canonical(path).string() << '\n';
#endif
}
'''


def run(command: list[str], *, env: dict[str, str] | None = None,
        expect_success: bool = True) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(command, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, env=env)
    if (result.returncode == 0) != expect_success:
        raise RuntimeError("command result mismatch: %r\n%s" %
                           (command, result.stdout))
    return result


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def inventory(prefix: Path) -> dict[str, dict[str, object]]:
    result: dict[str, dict[str, object]] = {}
    for path in sorted(prefix.rglob("*")):
        if path.is_symlink():
            result[path.relative_to(prefix).as_posix()] = {
                "kind": "symlink",
                "target": os.readlink(path),
                "sha256": sha256(path.resolve(strict=True)),
            }
        elif path.is_file():
            result[path.relative_to(prefix).as_posix()] = {
                "kind": "file",
                "sizeBytes": path.stat().st_size,
                "sha256": sha256(path),
            }
    return result


def manifest_paths(manifest: Path, prefix: Path) -> set[str]:
    paths: set[str] = set()
    for entry in manifest.read_text(encoding="utf-8").splitlines():
        if not entry:
            continue
        path = Path(entry)
        if not path.is_absolute():
            raise RuntimeError("install manifest contains a nonabsolute path: " + entry)
        try:
            relative = path.relative_to(prefix)
        except ValueError as error:
            raise RuntimeError("install manifest path escapes the prefix: " + entry) from error
        paths.add(relative.as_posix())
    return paths


def assert_manifest_matches(prefix: Path, manifest: Path) -> dict[str, dict[str, object]]:
    files = inventory(prefix)
    installed = manifest_paths(manifest, prefix)
    actual = set(files)
    if installed != actual:
        missing = sorted(installed - actual)
        unlisted = sorted(actual - installed)
        raise RuntimeError("install manifest mismatch; missing=%r unlisted=%r" %
                           (missing, unlisted))
    return files


def configure_consumer(prefix: Path, kind: str, source: Path, build: Path,
                       *, version: str) -> subprocess.CompletedProcess[str]:
    common = ["cmake", "-S", str(source), "-DPROBE_PREFIX=" + str(prefix),
              "-DEXPECT_KIND=" + kind, "-DREQUESTED_VERSION=" + version,
              "-DCMAKE_BUILD_TYPE=Release"]
    return run(common + ["-B", str(build)], expect_success=(version != "2.0"))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--kind", choices=("shared", "static"), required=True)
    parser.add_argument("--prefix", type=Path, required=True)
    parser.add_argument("--install-manifest", type=Path, required=True)
    parser.add_argument("--cli", type=Path,
                        help="installed dwg2dxf path, absolute or relative to the prefix")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    prefix = args.prefix.resolve(strict=True)
    manifest = args.install_manifest.resolve(strict=True)
    original_files = assert_manifest_matches(prefix, manifest)
    if not original_files:
        raise RuntimeError("installed package is empty")
    cli_relative: Path | None = None
    if args.cli is not None:
        if args.kind != "shared":
            raise RuntimeError("the installed CLI check requires a shared package")
        requested_cli = args.cli if args.cli.is_absolute() else prefix / args.cli
        cli_path = requested_cli.resolve(strict=True)
        if not cli_path.is_file():
            raise RuntimeError("installed CLI is not a file: %s" % cli_path)
        try:
            cli_relative = cli_path.relative_to(prefix)
        except ValueError as error:
            raise RuntimeError("installed CLI escapes the package prefix: %s" %
                               cli_path) from error
        if cli_relative.as_posix() not in original_files:
            raise RuntimeError("installed CLI is absent from the install manifest: %s" %
                               cli_relative.as_posix())

    with TemporaryDirectory(prefix="libdxfrw relocated package ") as directory:
        root = Path(directory)
        relocated = root / "relocated prefix"
        shutil.copytree(prefix, relocated, symlinks=True)
        relocated_files = inventory(relocated)
        if original_files != relocated_files:
            raise RuntimeError("relocated package inventory differs from staged package")

        source = root / "consumer source"
        source.mkdir()
        (source / "CMakeLists.txt").write_text(CMAKE, encoding="utf-8")
        (source / "probe.cpp").write_text(CPP, encoding="utf-8")
        rejected = configure_consumer(relocated, args.kind, source,
                                      root / "reject-version-2", version="2.0")
        if "3.0.0" not in rejected.stdout or "compatible" not in rejected.stdout.lower():
            raise RuntimeError("ABI-2 rejection did not identify the incompatible 3.0.0 package")
        build = root / "consumer build"
        configure_consumer(relocated, args.kind, source, build, version="3.0")
        run(["cmake", "--build", str(build), "--config", "Release", "--parallel", "2"])
        target_file = (build / "selected-target.txt").read_text(encoding="utf-8").strip()
        relocated_root = relocated.resolve(strict=True)
        selected = Path(target_file).resolve(strict=True)
        try:
            selected_relative = selected.relative_to(relocated_root)
        except ValueError as error:
            raise RuntimeError(
                "consumer target resolves outside relocated prefix: %s (prefix %s)" %
                (selected, relocated_root)
            ) from error

        executable_name = ("installed_package_probe.exe" if os.name == "nt"
                           else "installed_package_probe")
        candidates = [build / executable_name,
                      build / "Release" / executable_name]
        binaries = [path for path in candidates if path.is_file()]
        if len(binaries) != 1:
            raise RuntimeError("expected exactly one package consumer executable")
        env = os.environ.copy()
        loaded_path: Path | None = None
        if args.kind == "shared":
            variable = "PATH" if os.name == "nt" else (
                "DYLD_LIBRARY_PATH" if sys.platform == "darwin" else "LD_LIBRARY_PATH")
            env[variable] = str(selected.parent) + os.pathsep + env.get(variable, "")
            observed = run([str(binaries[0])], env=env).stdout.strip()
            if not observed.startswith("LOADED="):
                raise RuntimeError("shared consumer did not report its loaded runtime: " + observed)
            loaded_path = Path(observed[7:]).resolve(strict=True)
            if loaded_path != selected:
                raise RuntimeError("consumer loaded %s instead of relocated %s" %
                                   (loaded_path, selected))
        else:
            run([str(binaries[0])], env=env)

        cli_receipt = None
        if cli_relative is not None:
            relocated_cli = (relocated / cli_relative).resolve(strict=True)
            cli_env = os.environ.copy()
            # Exercise the installed RPATH / app-local DLL layout rather than
            # allowing a developer loader override to rescue a broken package.
            for variable in tuple(cli_env):
                if variable.startswith("DYLD_"):
                    cli_env.pop(variable, None)
            for variable in ("LD_LIBRARY_PATH", "LD_PRELOAD", "LD_DEBUG",
                             "LD_AUDIT", "LD_ORIGIN_PATH",
                             "LD_TRACE_LOADED_OBJECTS"):
                cli_env.pop(variable, None)
            cli_run = run([str(relocated_cli)], env=cli_env,
                          expect_success=False)
            if cli_run.returncode != 1 or "Usage:" not in cli_run.stdout:
                raise RuntimeError(
                    "relocated dwg2dxf did not reach its expected usage path "
                    "(exit %d): %s" % (cli_run.returncode, cli_run.stdout)
                )
            cli_receipt = {
                "path": cli_relative.as_posix(),
                "sha256": sha256(relocated_cli),
                "relocatedExecution": True,
                "usageOutputObserved": True,
                "expectedUsageExitCode": 1,
                "actualExitCode": cli_run.returncode,
                "knownLoaderOverrideVariablesCleared": True,
                "runtimeIdentity": (
                    "The relocated shared consumer above reports the exact "
                    "loaded dxfrw module path and hash."
                ),
            }

        receipt = {
            "schema": "libdxfrw-installed-package-receipt-v1",
            "kind": args.kind,
            "platform": sys.platform,
            "abiVersion2Rejected": True,
            "abiVersion3Accepted": True,
            "installManifestSha256": sha256(manifest),
            "installedFiles": original_files,
            "relocatedFilesMatch": True,
            "selectedTarget": {
                "path": selected_relative.as_posix(),
                "sha256": sha256(selected),
            },
            "loadedRuntime": ({
                "path": selected_relative.as_posix(),
                "sha256": sha256(loaded_path),
            } if loaded_path else None),
            "installedCli": cli_receipt,
            "limitations": [
                "Package selection, install inventory, relocation, a minimal consumer, and the CLI usage path only.",
                "Does not establish complete C++ ABI compatibility or drawing semantics.",
            ],
        }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"kind": args.kind, "platform": sys.platform,
                      "installedFiles": len(original_files),
                      "selectedTarget": receipt["selectedTarget"],
                      "loadedRuntime": receipt["loadedRuntime"],
                      "installedCli": receipt["installedCli"]}, sort_keys=True))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError) as exc:
        print("installed package probe: FAIL:", exc, file=sys.stderr)
        raise SystemExit(1)
