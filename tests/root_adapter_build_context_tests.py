#!/usr/bin/env python3
"""Protect the two audited root test adapter pools with scratch build graphs.

Tiny generated C++ controls, not drawing fixtures or platform qualification.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess
import tempfile


PROJECT = r'''
cmake_minimum_required(VERSION 3.28)
project(RootAdapterContext LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
option(LIBDXFRW_DEDUP_TEST_ADAPTER "" ON)
if(SIMULATE_MSVC)
    set(MSVC TRUE)
endif()
add_library(dxfrw INTERFACE)
include("${ADAPTER_MODULE}")
enable_testing()
foreach(_name IN ITEMS dwg_local_roundtrip dxf_fixture_tests dwg_fixture_tests
        symbol_output_tests dwg_reader_matrix_tests wave1_tests)
    set(_adapter "${_libdxfrw_root_test_adapter_source}")
    if(_name STREQUAL "wave1_tests")
        set(_adapter "${_libdxfrw_wave1_adapter_source}")
    elseif(_name MATCHES "^(symbol_output_tests|dwg_reader_matrix_tests)$")
        set(_adapter "${_libdxfrw_reader_test_adapter_source}")
    endif()
    add_executable(libdxfrw_${_name} tests/${_name}.cpp ${_adapter})
    target_compile_features(libdxfrw_${_name} PRIVATE cxx_std_17)
    target_include_directories(libdxfrw_${_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
    if(_name MATCHES "^(symbol_output_tests|dwg_reader_matrix_tests|wave1_tests)$")
        target_include_directories(libdxfrw_${_name} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/intern ${CMAKE_CURRENT_SOURCE_DIR}/dwg2dxf)
    else()
        target_include_directories(libdxfrw_${_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    endif()
    target_link_libraries(libdxfrw_${_name} PRIVATE dxfrw)
    add_test(NAME ${_name} COMMAND libdxfrw_${_name})
endforeach()
if(MSVC)
    target_compile_definitions(libdxfrw_wave1_tests PRIVATE LIBDXFRW_FORCE_TEST_SEAMS=1)
endif()
foreach(_source IN ITEMS dxf_fixture_tests dwg_fixture_tests dwg_reader_matrix_tests)
    set_property(SOURCE tests/${_source}.cpp APPEND PROPERTY
        COMPILE_DEFINITIONS FIXTURE_DRIVER_ONLY="fixture path with spaces")
endforeach()
if(DRIFT_POOL STREQUAL "root")
    set(_target libdxfrw_dxf_fixture_tests)
else()
    set(_target libdxfrw_dwg_reader_matrix_tests)
endif()
if(DRIFT STREQUAL "definition")
    target_compile_definitions(${_target} PRIVATE DIFFERENT_CONTEXT=1)
elseif(DRIFT STREQUAL "include")
    target_include_directories(${_target} PRIVATE ${CMAKE_SOURCE_DIR}/different)
elseif(DRIFT STREQUAL "option")
    target_compile_options(${_target} PRIVATE "$<$<CONFIG:Debug>:different-option>")
elseif(DRIFT STREQUAL "dependency")
    add_library(different INTERFACE)
    target_compile_definitions(different INTERFACE DIFFERENT_CONTEXT=1)
    target_link_libraries(${_target} PRIVATE different)
elseif(DRIFT STREQUAL "runtime")
    set_property(TARGET ${_target} PROPERTY MSVC_RUNTIME_LIBRARY MultiThreaded)
elseif(DRIFT STREQUAL "ipo")
    set_property(TARGET ${_target} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
elseif(DRIFT STREQUAL "warnings")
    set_property(TARGET ${_target} PROPERTY COMPILE_WARNING_AS_ERROR ON)
elseif(DRIFT STREQUAL "pic")
    set_property(TARGET ${_target} PROPERTY POSITION_INDEPENDENT_CODE ON)
elseif(DRIFT STREQUAL "directory")
    add_subdirectory(nested)
    libdxfrw_check_adapter_context(libdxfrw_${DRIFT_POOL}_test_adapter different_directory)
endif()
libdxfrw_check_root_test_adapter_context()
file(WRITE "${CMAKE_BINARY_DIR}/shared.txt" "${_libdxfrw_root_test_adapters_shared}")
'''

NAMES = ("dwg_local_roundtrip", "dxf_fixture_tests", "dwg_fixture_tests",
         "symbol_output_tests", "dwg_reader_matrix_tests", "wave1_tests")
DRIVER = '''
extern int adapter();
#ifdef REQUIRE_FIXTURE_DRIVER
#ifndef FIXTURE_DRIVER_ONLY
#error missing driver fixture definition
#endif
#endif
int main() {
#ifdef LIBDXFRW_FORCE_TEST_SEAMS
    return adapter() != 43;
#else
    return adapter() != 42;
#endif
}
'''
ADAPTER = '''
#ifdef FIXTURE_DRIVER_ONLY
#error driver-only fixture definition leaked into adapter
#endif
int adapter() {
#ifdef LIBDXFRW_FORCE_TEST_SEAMS
    return 43;
#else
    return 42;
#endif
}
'''


def checked(command: list[str]) -> str:
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, timeout=45, check=False)
    if result.returncode:
        raise RuntimeError(result.stdout)
    return result.stdout


def check_real_graph(build: Path) -> None:
    commands = build / "compile_commands.json"
    if not commands.exists():
        print("SKIP actual compile-command inventory (generator has no export)")
        return
    rows = json.loads(commands.read_text())
    for filename, macro in (("dxf_fixture_tests.cpp", "LIBDXFRW_DXF_FIXTURE_DIR"),
                            ("dwg_fixture_tests.cpp", "LIBDXFRW_DWG_FIXTURE_DIR"),
                            ("dwg_reader_matrix_tests.cpp", "LIBDXFRW_READER_FIXTURE_DIR")):
        drivers = [r for r in rows if Path(r["file"]).name == filename]
        assert drivers and all(macro in r["command"] for r in drivers), filename
    for row in rows:
        command = row["command"]
        if "libdxfrw_root_test_adapter.dir" in command or "libdxfrw_reader_test_adapter.dir" in command:
            assert "FIXTURE_DIR" not in command and "LIBDXFRW_FORCE_TEST_SEAMS" not in command, command
    print("PASS actual driver-only fixture definitions and pooled seam isolation")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("cmake", "ctest", "module", "compiler", "generator", "build-dir"):
        parser.add_argument("--" + name, required=True)
    for name in ("generator-platform", "generator-toolset", "generator-instance",
                 "toolchain", "make-program", "compiler-target", "sysroot",
                 "system-name", "system-version", "system-processor", "configuration"):
        parser.add_argument("--" + name, default="")
    args = parser.parse_args()
    common = [args.cmake, "-G", args.generator,
              f"-DCMAKE_CXX_COMPILER:FILEPATH={args.compiler}",
              f"-DADAPTER_MODULE:FILEPATH={Path(args.module).resolve()}"]
    for flag, value in (("-A", args.generator_platform), ("-T", args.generator_toolset)):
        if value:
            common += [flag, value]
    for key, value in (("CMAKE_GENERATOR_INSTANCE", args.generator_instance),
                       ("CMAKE_TOOLCHAIN_FILE", args.toolchain),
                       ("CMAKE_MAKE_PROGRAM", args.make_program),
                       ("CMAKE_CXX_COMPILER_TARGET", args.compiler_target),
                       ("CMAKE_SYSROOT", args.sysroot),
                       ("CMAKE_SYSTEM_NAME", args.system_name),
                       ("CMAKE_SYSTEM_VERSION", args.system_version),
                       ("CMAKE_SYSTEM_PROCESSOR", args.system_processor),
                       ("CMAKE_BUILD_TYPE", args.configuration)):
        if value:
            common += [f"-D{key}={value}"]
    configuration = ["--config", args.configuration] if args.configuration else []
    ctest_configuration = ["-C", args.configuration] if args.configuration else []
    cases = [("shared", [], True, False),
             ("exports-off", ["-DCMAKE_ENABLE_EXPORTS=OFF"], True, False),
             ("disabled", ["-DLIBDXFRW_DEDUP_TEST_ADAPTER=OFF"], False, False),
             ("pic", ["-DCMAKE_POSITION_INDEPENDENT_CODE=ON"], False, False),
             ("exports", ["-DCMAKE_ENABLE_EXPORTS=ON"], False, False),
             ("msvc-seam-routing", ["-DSIMULATE_MSVC=ON"], True, True)]
    with tempfile.TemporaryDirectory(prefix="dxfrw root adapter ") as scratch:
        source = Path(scratch) / "source"
        (source / "dwg2dxf").mkdir(parents=True)
        (source / "tests").mkdir()
        (source / "nested").mkdir()
        (source / "nested/CMakeLists.txt").write_text(
            "add_library(different_directory INTERFACE)\n", encoding="utf-8")
        (source / "CMakeLists.txt").write_text(PROJECT, encoding="utf-8")
        (source / "dwg2dxf/dx_iface.cpp").write_text(ADAPTER, encoding="utf-8")
        for name in NAMES:
            fixture_driver = name in ("dxf_fixture_tests", "dwg_fixture_tests", "dwg_reader_matrix_tests")
            prefix = "#define REQUIRE_FIXTURE_DRIVER\n" if fixture_driver else ""
            (source / f"tests/{name}.cpp").write_text(prefix + DRIVER, encoding="utf-8")
        build = Path(scratch) / "build"
        for name, options, shared, simulate_msvc in cases:
            reset = ["-DLIBDXFRW_DEDUP_TEST_ADAPTER=ON", "-DDRIFT=", "-DSIMULATE_MSVC=OFF",
                     "-UCMAKE_POSITION_INDEPENDENT_CODE", "-UCMAKE_ENABLE_EXPORTS"]
            checked(common + ["-S", str(source), "-B", str(build)] + reset + options)
            assert (build / "shared.txt").read_text() == ("TRUE" if shared else "FALSE")
            commands = build / "compile_commands.json"
            if commands.exists():
                rows = json.loads(commands.read_text())
                configs = sum(Path(r["file"]).name == "dwg_local_roundtrip.cpp" for r in rows)
                # A real MSVC toolchain also always keeps Wave1 separate.
                seam = simulate_msvc or any("LIBDXFRW_FORCE_TEST_SEAMS" in r["command"] for r in rows)
                count = sum(Path(r["file"]).name == "dx_iface.cpp" for r in rows)
                assert count == configs * ((3 if seam else 2) if shared else 6), (name, count, configs)
            # Compile and execute shared, opt-out, PIC/export and seam routes.
            # No real converter/library build is involved.
            checked([args.cmake, "--build", str(build), "--parallel", "2"] + configuration)
            checked([args.ctest, "--test-dir", str(build), "--output-on-failure"] + ctest_configuration)
            print(f"PASS {name}")
        for pool in ("root", "reader"):
            for drift in ("definition", "include", "option", "dependency", "runtime", "ipo", "warnings", "pic", "directory"):
                result = subprocess.run(common + ["-S", str(source), "-B", str(build),
                    "-DSIMULATE_MSVC=OFF", "-DLIBDXFRW_DEDUP_TEST_ADAPTER=ON",
                    "-UCMAKE_POSITION_INDEPENDENT_CODE", "-UCMAKE_ENABLE_EXPORTS",
                    f"-DDRIFT_POOL={pool}", f"-DDRIFT={drift}"],
                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=45, check=False)
                assert result.returncode != 0 and "differs from the shared adapter" in " ".join(result.stdout.split()), result.stdout
                print(f"PASS {pool} {drift} refusal")
    check_real_graph(Path(args.build_dir))


if __name__ == "__main__":
    main()
