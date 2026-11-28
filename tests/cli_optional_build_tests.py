#!/usr/bin/env python3
"""Check real CTest registration with and without the optional converter.

Configure only: no library build, external runtime, network or drawing writes.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile


def checked(command: list[str]) -> str:
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                            text=True, timeout=45, check=False)
    if result.returncode:
        raise RuntimeError(result.stdout)
    return result.stdout


def properties(test: dict) -> dict:
    return {p["name"]: p["value"] for p in test.get("properties", [])}


def values(value) -> set[str]:
    return set(value if isinstance(value, list) else value.split(";")) - {""}


def validate_dependencies(tests: dict[str, dict]) -> None:
    setups = set()
    for test in tests.values():
        setups |= values(properties(test).get("FIXTURES_SETUP", []))
    for name, test in tests.items():
        props = properties(test)
        missing = values(props.get("DEPENDS", [])) - tests.keys()
        if missing:
            raise RuntimeError(f"{name}: missing test dependencies: {sorted(missing)}")
        missing = values(props.get("FIXTURES_REQUIRED", [])) - setups
        if missing:
            raise RuntimeError(f"{name}: missing fixture setup: {sorted(missing)}")


def projected(test: dict) -> dict:
    # CTest backtrace indices change when optional registrations are omitted.
    return {key: test[key] for key in ("name", "command", "properties") if key in test}


def dependency_self_test() -> None:
    setup = {"name": "setup", "properties": [{"name": "FIXTURES_SETUP", "value": ["f"]}]}
    user = {"name": "user", "properties": [{"name": "FIXTURES_REQUIRED", "value": ["f"]},
                                              {"name": "DEPENDS", "value": ["setup"]}]}
    validate_dependencies({"setup": setup, "user": user})
    for test in (user, {"name": "orphan", "properties": [user["properties"][0]]}):
        try:
            validate_dependencies({test["name"]: test})
        except RuntimeError:
            pass
        else:
            raise RuntimeError("dependency checker accepted an orphan")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("cmake", "ctest", "source", "compiler", "generator"):
        parser.add_argument("--" + name, required=True)
    for name in ("generator-platform", "generator-toolset", "generator-instance",
                 "toolchain", "make-program", "compiler-target", "sysroot",
                 "system-name", "system-version", "system-processor", "configuration"):
        parser.add_argument("--" + name, default="")
    args = parser.parse_args()
    dependency_self_test()
    source = Path(args.source).resolve()
    controls = re.findall(r"libdxfrw_require_test_cli\((LIBDXFRW_ENABLE_[A-Z0-9_]+)\)",
                          (source / "tests/CMakeLists.txt").read_text())
    if not controls or len(set(controls)) != len(controls):
        raise RuntimeError("missing/duplicate explicit CLI control inventory")
    with tempfile.TemporaryDirectory(prefix="dxfrw-cli-optional-") as scratch:
        build = Path(scratch) / "build"
        command = [args.cmake, "-S", str(source), "-B", str(build), "-G", args.generator,
                   "-DLIBDXFRW_BUILD_TESTS=ON", "-DLIBDXFRW_BUILD_DOC=OFF"]
        for flag, value in (("-A", args.generator_platform), ("-T", args.generator_toolset)):
            if value:
                command += [flag, value]
        for key, value in (("CMAKE_CXX_COMPILER", args.compiler),
                           ("CMAKE_GENERATOR_INSTANCE", args.generator_instance),
                           ("CMAKE_TOOLCHAIN_FILE", args.toolchain),
                           ("CMAKE_MAKE_PROGRAM", args.make_program),
                           ("CMAKE_CXX_COMPILER_TARGET", args.compiler_target),
                           ("CMAKE_SYSROOT", args.sysroot),
                           ("CMAKE_SYSTEM_NAME", args.system_name),
                           ("CMAKE_SYSTEM_VERSION", args.system_version),
                           ("CMAKE_SYSTEM_PROCESSOR", args.system_processor),
                           ("CMAKE_BUILD_TYPE", args.configuration)):
            if value:
                command += [f"-D{key}={value}"]
        command += [f"-D{control}=OFF" for control in controls]
        show = [args.ctest, "--test-dir", str(build), "--show-only=json-v1"]
        if args.configuration:
            show += ["-C", args.configuration]
        for shared in ("OFF", "ON"):
            base = command + [f"-DBUILD_SHARED_LIBS={shared}"]
            checked(base + ["-DLIBDXFRW_BUILD_DWG2DXF=ON"])
            on = {t["name"]: t for t in json.loads(checked(show))["tests"]}
            cli_command = on["dwg2dxf_cli_contract"]["command"]
            converter = cli_command[cli_command.index("--converter") + 1]
            cli_tests = {name for name, test in on.items()
                         if any(converter in arg for arg in test.get("command", []))}
            if not cli_tests:
                raise RuntimeError("CLI-on inventory found no converter-dependent tests")
            checked(base + ["-DLIBDXFRW_BUILD_DWG2DXF=OFF"])
            off = {t["name"]: t for t in json.loads(checked(show))["tests"]}
            if off.keys() != on.keys() - cli_tests:
                raise RuntimeError(f"unexpected CLI-off delta: {off.keys() ^ (on.keys() - cli_tests)}")
            for name, test in off.items():
                if projected(test) != projected(on[name]):
                    raise RuntimeError(f"retained test command/properties changed: {name}")
            for required in ("dwg2dxf_version_policy", "libdxfrw_block_owners",
                             "libdxfrw_dimstyle_optional_handles", "libdxfrw_numeric_locale",
                             "dwg2dxf_selected_raster_reactor_metadata", "libdxfrw_3d_consumer_probe"):
                if required not in off:
                    raise RuntimeError(f"standalone consumer/adapter test lost: {required}")
            validate_dependencies(on)
            validate_dependencies(off)
            print(f"PASS shared={shared}: {len(on)} CLI-on, {len(off)} CLI-off, "
                  f"exactly {len(cli_tests)} CLI-dependent removals", flush=True)
        for control in controls:
            result = subprocess.run(command + ["-DLIBDXFRW_BUILD_DWG2DXF=OFF", f"-D{control}=ON"],
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    text=True, timeout=45, check=False)
            expected = f"{control} requires LIBDXFRW_BUILD_DWG2DXF=ON"
            if not result.returncode or expected not in " ".join(result.stdout.split()):
                raise RuntimeError(f"missing explicit-control error for {control}:\n{result.stdout}")
        print(f"PASS {len(controls)} explicit CLI-control refusals; dependency/orphan controls")


if __name__ == "__main__":
    main()
