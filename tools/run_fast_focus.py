#!/usr/bin/env python3
"""Run the dependency-free libdxfrw focused executable set.

This is intentionally narrower than CTest: it exercises deterministic parser,
graph, writer, and diagnostic vectors without requiring external DWG/DXF
payloads.  Fixture-backed executables remain separate and are never silently
treated as part of this fast lane.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path


TARGETS = (
    "libdxfrw_wave1_tests",
    "libdxfrw_hardening_tests",
    "libdxfrw_graph_preservation_tests",
    "libdxfrw_writer_primitives_tests",
    "libdxfrw_dwg_object_vectors_tests",
    "libdxfrw_writer_version_matrix_tests",
    "libdxfrw_diagnostic_tests",
)

# Fixed, bounded PR integration controls. Fixture-backed tests deliberately
# remain outside the default dependency-free executable lane above.
CTEST_REGRESSIONS = (
    "libdxfrw_dxf_fixtures",
    "libdxfrw_dwg_local_roundtrip",
    "libdxfrw_3d_consumer_probe",
    "dwg2dxf_version_policy",
    "dwg2dxf_image_path_cli",
    "dwg2dxf_header_maintenance_ascii_cli",
    "dwg2dxf_header_maintenance_binary_cli",
    "dwg2dxf_3dline_adapter_cli",
    "dwg2dxf_3dline_binary_adapter_cli",
    "dwg2dxf_dynamic_block_fail_closed",
    "libdxfrw_dimstyle_optional_handles",
    "dwg2dxf_dimstyle_optional_handles_ascii_cli",
    "dwg2dxf_dimstyle_optional_handles_binary_cli",
    "libdxfrw_block_owners",
    "dwg2dxf_block_owner_rejection_cli",
    "dwg2dxf_block_owners_ascii_cli",
    "dwg2dxf_block_owners_binary_cli",
    "dwg2dxf_hatch_tangents_ascii_cli",
    "dwg2dxf_hatch_tangents_binary_cli",
    "dwg2dxf_freecad_cli_compat",
    "dwg2dxf_freecad_installed_cli_compat",
)


def ctest_selector(inventory: dict) -> str:
    """Fail closed if a required registration or its built command is absent."""
    rows = inventory.get("tests") if isinstance(inventory, dict) else None
    if not isinstance(rows, list):
        raise ValueError("CTest inventory has no test array")
    by_name = {}
    for row in rows:
        name = row.get("name") if isinstance(row, dict) else None
        if not isinstance(name, str) or not name or name in by_name:
            raise ValueError("CTest inventory has invalid or duplicate names")
        by_name[name] = row
    missing = sorted(set(CTEST_REGRESSIONS) - set(by_name))
    if missing:
        raise ValueError("required fast CTests missing: " + ", ".join(missing))
    for name in CTEST_REGRESSIONS:
        command = by_name[name].get("command")
        if (not isinstance(command, list) or not command
                or any(not isinstance(arg, str) or not arg for arg in command)):
            raise ValueError(f"required fast CTest {name!r} has no built command")
    # CTest uses extended POSIX regex, not Python's non-capturing groups.
    return "^(" + "|".join(re.escape(name) for name in CTEST_REGRESSIONS) + ")$"


def run_ctest_regressions(build_dir: Path, configuration: str | None) -> None:
    command = ["ctest", "--test-dir", str(build_dir)]
    if configuration:
        command.extend(["-C", configuration])
    inventory = json.loads(subprocess.check_output(
        [*command, "-N", "--show-only=json-v1"], text=True))
    selector = ctest_selector(inventory)
    subprocess.run([*command, "-R", selector, "--no-tests=error",
                    "--timeout", "60", "--output-on-failure"], check=True)


def executable(build_dir: Path, target: str, configuration: str | None) -> Path:
    names = (target, target + ".exe")
    roots = [build_dir]
    if configuration:
        roots.insert(0, build_dir / configuration)
    for root in roots:
        for name in names:
            candidate = root / name
            if candidate.is_file():
                return candidate
    raise FileNotFoundError(
        f"focused executable {target!r} is missing under {build_dir}"
    )


def run(build_dir: Path, configuration: str | None,
        ctest_regressions: bool = False) -> int:
    for target in TARGETS:
        path = executable(build_dir, target, configuration)
        print(f"[{target}] {path}", flush=True)
        subprocess.run([str(path)], check=True)
    if ctest_regressions:
        run_ctest_regressions(build_dir, configuration)
    suffix = f" + {len(CTEST_REGRESSIONS)} required CTests" if ctest_regressions else ""
    print(f"fast focus: PASS ({len(TARGETS)} executables{suffix})")
    return 0


def self_test() -> None:
    assert TARGETS[0] == "libdxfrw_wave1_tests"
    assert len(TARGETS) == len(set(TARGETS))
    assert CTEST_REGRESSIONS and len(CTEST_REGRESSIONS) == len(set(CTEST_REGRESSIONS))
    inventory = {"tests": [
        {"name": name, "command": ["generated-test", "--sentinel"]}
        for name in CTEST_REGRESSIONS
    ]}
    selector = ctest_selector(inventory)
    assert all(re.fullmatch(selector, name) for name in CTEST_REGRESSIONS)
    assert re.fullmatch(selector, CTEST_REGRESSIONS[0] + "_extra") is None
    for bad in (
        {}, {"tests": []}, {"tests": inventory["tests"][:-1]},
        {"tests": [*inventory["tests"], inventory["tests"][0]]},
        {"tests": [{"name": name} for name in CTEST_REGRESSIONS]},
        {"tests": [None]},
    ):
        try:
            ctest_selector(bad)
        except ValueError:
            pass
        else:
            raise AssertionError("invalid fast CTest inventory accepted")
    assert ctest_selector({"tests": [*inventory["tests"],
                                     {"name": "unselected-advisory"}]}) == selector
    print("run_fast_focus self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--configuration", default=None)
    parser.add_argument("--ctest-regressions", action="store_true",
                        help="also run the fixed PR integration CTests")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    if args.self_test:
        self_test()
        return 0
    if args.build_dir is None:
        parser.error("--build-dir is required unless --self-test is used")
    try:
        return run(args.build_dir.resolve(), args.configuration,
                   args.ctest_regressions)
    except (OSError, ValueError, subprocess.CalledProcessError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
