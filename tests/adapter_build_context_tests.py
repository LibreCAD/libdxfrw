#!/usr/bin/env python3
"""Configure tiny scratch projects to protect the adapter sharing boundary.

No drawings, library build, network access, or support qualification is involved.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess
import tempfile


PROJECT = r'''
cmake_minimum_required(VERSION 3.28)
project(AdapterContext LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(LIBDXFRW_DEDUP_TEST_ADAPTER "" ON)
add_library(dxfrw INTERFACE)
set(LIBDXFRW_WARNING_FLAGS "")
include("${ADAPTER_MODULE}")
foreach(_target IN ITEMS adapter block dimstyle numeric)
    add_executable(${_target} main.cpp ${_libdxfrw_test_adapter_source})
    target_compile_features(${_target} PRIVATE cxx_std_17)
    target_compile_options(${_target} PRIVATE ${LIBDXFRW_WARNING_FLAGS})
    target_include_directories(${_target} PRIVATE
        ${CMAKE_SOURCE_DIR}/dwg2dxf ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(${_target} PRIVATE dxfrw)
endforeach()
if(DRIFT STREQUAL "definition")
    target_compile_definitions(numeric PRIVATE DIFFERENT_CONTEXT=1)
elseif(DRIFT STREQUAL "include")
    target_include_directories(numeric PRIVATE ${CMAKE_SOURCE_DIR}/different)
elseif(DRIFT STREQUAL "option")
    target_compile_options(numeric PRIVATE "$<$<CONFIG:Debug>:different-option>")
elseif(DRIFT STREQUAL "dependency")
    add_library(different INTERFACE)
    target_compile_definitions(different INTERFACE DIFFERENT_CONTEXT=1)
    target_link_libraries(numeric PRIVATE different)
elseif(DRIFT STREQUAL "runtime")
    set_property(TARGET numeric PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded)
elseif(DRIFT STREQUAL "ipo")
    set_property(TARGET numeric PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endif()
libdxfrw_check_test_adapter_context(adapter block dimstyle numeric)
file(WRITE "${CMAKE_BINARY_DIR}/shared.txt" "${_libdxfrw_test_adapter_shared}")
'''


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cmake", required=True)
    parser.add_argument("--module", type=Path, required=True)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--generator", required=True)
    parser.add_argument("--generator-platform", default="")
    parser.add_argument("--generator-toolset", default="")
    parser.add_argument("--toolchain", default="")
    args = parser.parse_args()
    common = [args.cmake, "-G", args.generator,
              f"-DCMAKE_CXX_COMPILER:FILEPATH={args.compiler}",
              f"-DADAPTER_MODULE:FILEPATH={args.module.resolve()}"]
    if args.generator_platform:
        common += ["-A", args.generator_platform]
    if args.generator_toolset:
        common += ["-T", args.generator_toolset]
    if args.toolchain:
        common += [f"-DCMAKE_TOOLCHAIN_FILE:FILEPATH={args.toolchain}"]
    cases = [("shared", [], True),
             ("exports-off", ["-DCMAKE_ENABLE_EXPORTS=OFF"], True),
             ("disabled", ["-DLIBDXFRW_DEDUP_TEST_ADAPTER=OFF"], False),
             ("pic", ["-DCMAKE_POSITION_INDEPENDENT_CODE=ON"], False),
             ("exports", ["-DCMAKE_ENABLE_EXPORTS=ON"], False)]
    cases += [(kind, [f"-DDRIFT={kind}"], None)
              for kind in ("definition", "include", "option", "dependency", "runtime", "ipo")]
    with tempfile.TemporaryDirectory(prefix="dxfrw-adapter-context-") as scratch:
        source = Path(scratch) / "source"
        (source / "dwg2dxf").mkdir(parents=True)
        (source / "CMakeLists.txt").write_text(PROJECT, encoding="utf-8")
        (source / "main.cpp").write_text("int main() { return 0; }\n", encoding="utf-8")
        (source / "dwg2dxf/dx_iface.cpp").write_text("int adapter() { return 0; }\n", encoding="utf-8")
        # Reuse compiler detection, not stale case options or object outputs.
        # These are configure/graph controls; real normal/ASan builds stay in
        # separate build trees and are exercised by the owning C++ tests.
        build = Path(scratch) / "build"
        for name, options, shared in cases:
            reset = ["-DLIBDXFRW_DEDUP_TEST_ADAPTER=ON", "-DDRIFT=",
                     "-UCMAKE_POSITION_INDEPENDENT_CODE", "-UCMAKE_ENABLE_EXPORTS"]
            result = subprocess.run(common + ["-S", str(source), "-B", str(build)] + reset + options,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    text=True, timeout=45, check=False)
            if shared is None:
                assert result.returncode != 0 and "differs from the shared adapter" in " ".join(result.stdout.split()), result.stdout
            else:
                assert result.returncode == 0, result.stdout
                assert (build / "shared.txt").read_text() == ("TRUE" if shared else "FALSE")
                commands = build / "compile_commands.json"
                if commands.exists():
                    rows = json.loads(commands.read_text())
                    count = sum(Path(row["file"]).name == "dx_iface.cpp" for row in rows)
                    # Multi-config generators record one command per config.
                    configs = sum(Path(row["file"]).name == "main.cpp" for row in rows) // 4
                    assert count == configs * (1 if shared else 4), (name, count, configs)
            print(f"PASS {name}")


if __name__ == "__main__":
    main()
