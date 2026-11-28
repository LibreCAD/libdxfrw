#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Exercise the installed ABI-3 shared package from a fresh CMake consumer."""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
from tempfile import TemporaryDirectory

CMAKE = r'''cmake_minimum_required(VERSION 3.28)
project(shared_abi_consumer LANGUAGES CXX)
find_package(libdxfrw ${REQUESTED_VERSION} CONFIG REQUIRED NO_DEFAULT_PATH PATHS "${PROBE_PREFIX}")
add_executable(shared_abi_consumer probe.cpp)
target_compile_features(shared_abi_consumer PRIVATE cxx_std_17)
target_link_libraries(shared_abi_consumer PRIVATE libdxfrw::libdxfrw ${CMAKE_DL_LIBS})
'''
CPP = r'''#if defined(__linux__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif
#include <libdxfrw.h>
#include <filesystem>
#include <iostream>
#include <string>
#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#else
#include <link.h>
#endif
#if !defined(_WIN32) && !defined(__APPLE__)
static int locate(dl_phdr_info* info, size_t, void* result) {
    std::string path = info->dlpi_name ? info->dlpi_name : "";
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
    std::string path;
#if defined(_WIN32)
    HMODULE module = GetModuleHandleA("dxfrw-3.dll");
    char name[32768] = {};
    const DWORD size = module ? GetModuleFileNameA(module, name, sizeof(name)) : 0;
    if (size == 0 || size >= sizeof(name)) return 2;
    path.assign(name, size);
#elif defined(__APPLE__)
    for (uint32_t i = 0; i < _dyld_image_count(); ++i) {
        const char* name = _dyld_get_image_name(i);
        if (name && std::string(name).find("libdxfrw.3.") != std::string::npos)
            path = name;
    }
#else
    dl_iterate_phdr(locate, &path);
#endif
    if (path.empty()) return 3;
    std::cout << "LOADED=" << std::filesystem::canonical(path).string() << '\n';
}
'''


def run(command: list[str], *, env: dict[str, str] | None = None,
        expect_success: bool = True) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(command, text=True, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, env=env)
    if (result.returncode == 0) != expect_success:
        raise RuntimeError("command result mismatch: %s\n%s" % (command, result.stdout))
    return result


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prefix", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    prefix = args.prefix.resolve(strict=True)
    if os.name == "nt":
        expected = prefix / "bin" / "dxfrw-3.dll"
        old = prefix / "bin" / "dxfrw.dll"
    elif sys.platform == "darwin":
        expected = prefix / "lib" / "libdxfrw.3.dylib"
        old = prefix / "lib" / "libdxfrw.2.dylib"
    elif sys.platform.startswith("linux"):
        expected = prefix / "lib" / "libdxfrw.so.3"
        old = prefix / "lib" / "libdxfrw.so.2"
    else:
        raise RuntimeError("unsupported probe platform: " + sys.platform)
    expected = expected.resolve(strict=True)
    if old.exists():
        raise RuntimeError("fresh install unexpectedly contains old ABI runtime")
    with TemporaryDirectory(prefix="libdxfrw-shared-abi-") as directory:
        root = Path(directory)
        src = root / "src"
        src.mkdir()
        (src / "CMakeLists.txt").write_text(CMAKE, encoding="utf-8")
        (src / "probe.cpp").write_text(CPP, encoding="utf-8")
        common = ["cmake", "-S", str(src), "-DPROBE_PREFIX=" + str(prefix)]
        rejected = run(common + ["-B", str(root / "old"),
                                 "-DREQUESTED_VERSION=2.0"], expect_success=False)
        if "3.0.0" not in rejected.stdout or "compatible" not in rejected.stdout.lower():
            raise RuntimeError("version-2 rejection did not identify the incompatible 3.0.0 package")
        build = root / "current"
        run(common + ["-B", str(build), "-DREQUESTED_VERSION=3.0",
                      "-DCMAKE_BUILD_TYPE=Release"])
        run(["cmake", "--build", str(build), "--config", "Release", "--parallel", "2"])
        candidates = [build / ("shared_abi_consumer.exe" if os.name == "nt" else "shared_abi_consumer"),
                      build / "Release" / ("shared_abi_consumer.exe" if os.name == "nt" else "shared_abi_consumer")]
        binaries = [p for p in candidates if p.is_file()]
        if len(binaries) != 1:
            raise RuntimeError("expected one built consumer executable")
        env = os.environ.copy()
        variable = "PATH" if os.name == "nt" else "DYLD_LIBRARY_PATH" if sys.platform == "darwin" else "LD_LIBRARY_PATH"
        env[variable] = str(expected.parent) + os.pathsep + env.get(variable, "")
        observed = run([str(binaries[0])], env=env).stdout.strip()
        if not observed.startswith("LOADED=") or Path(observed[7:]).resolve(strict=True) != expected:
            raise RuntimeError("consumer loaded a different library: " + observed)
    receipt = {"scope": "native shared package boundary; not full ABI or consumer qualification",
               "platform": sys.platform, "version2Rejected": True,
               "version3Accepted": True, "oldRuntimeAbsent": True,
               "loadedLibrary": str(expected),
               "librarySha256": hashlib.sha256(expected.read_bytes()).hexdigest()}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(receipt, sort_keys=True))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError) as exc:
        print("shared ABI probe: FAIL:", exc, file=sys.stderr)
        raise SystemExit(1)
