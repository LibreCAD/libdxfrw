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
import tempfile
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
    "libdxfrw_public_header_hygiene",
    "libdxfrw_bounded_process",
    "libdxfrw_independent_dxf_selftest",
    "libdxfrw_debug_state",
    "dwg2dxf_cli_contract",
    "libdxfrw_dxf_fixtures",
    "libdxfrw_dwg_local_roundtrip",
    "libdxfrw_3d_consumer_probe",
    "dwg2dxf_version_policy",
    "dwg2dxf_image_path_cli",
    "dwg2dxf_header_maintenance_ascii_cli",
    "dwg2dxf_header_maintenance_binary_cli",
    "libdxfrw_numeric_locale",
    "libdxfrw_numeric_process_locale",
    "dwg2dxf_numeric_locale_ascii_cli",
    "dwg2dxf_numeric_locale_binary_cli",
    "dwg2dxf_3dline_adapter_cli",
    "dwg2dxf_3dline_binary_adapter_cli",
    "dwg2dxf_dynamic_block_fail_closed",
    "libdxfrw_dimstyle_optional_handles",
    "dwg2dxf_dimstyle_optional_handles_ascii_cli",
    "dwg2dxf_dimstyle_optional_handles_binary_cli",
    "libdxfrw_block_owners",
    "libdxfrw_extension_graph_limits",
    "dwg2dxf_block_owner_rejection_cli",
    "dwg2dxf_block_owners_ascii_cli",
    "dwg2dxf_block_owners_binary_cli",
    "dwg2dxf_hatch_tangents_ascii_cli",
    "dwg2dxf_hatch_tangents_binary_cli",
    "dwg2dxf_mpolygon_header_ascii_cli",
    "dwg2dxf_mpolygon_header_binary_cli",
    "dwg2dxf_mpolygon_metadata_ascii_cli",
    "dwg2dxf_mpolygon_metadata_binary_cli",
    "dwg2dxf_dimension_frames_ascii_cli",
    "dwg2dxf_dimension_frames_binary_cli",
    "dwg2dxf_arc_dimension_flags_ascii_cli",
    "dwg2dxf_arc_dimension_flags_binary_cli",
    "dwg2dxf_leader_advisory_count_ascii_cli",
    "dwg2dxf_leader_advisory_count_binary_cli",
    "dwg2dxf_mtext_orientation_ascii_cli",
    "dwg2dxf_mtext_orientation_binary_cli",
    "dwg2dxf_r12_ellipse_carriers_ascii_cli",
    "dwg2dxf_r12_ellipse_carriers_binary_cli",
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
                or not isinstance(command[0], str) or not command[0]
                or any(not isinstance(arg, str) for arg in command)):
            raise ValueError(f"required fast CTest {name!r} has no built command")
    # CTest uses extended POSIX regex, not Python's non-capturing groups.
    return "^(" + "|".join(re.escape(name) for name in CTEST_REGRESSIONS) + ")$"


def require_ctest_passes(junit_path: Path) -> None:
    # Reuse the exact-set/duplicate/status validation without producing or
    # weakening native qualification receipts. This lane is regression-only.
    from run_native_qualification import QualificationError, parse_junit
    try:
        _, summary = parse_junit(junit_path, list(CTEST_REGRESSIONS))
    except QualificationError as exc:
        raise ValueError(str(exc)) from exc
    if summary["passed"] != len(CTEST_REGRESSIONS):
        raise ValueError("required fast CTests did not all execute and pass: "
                         + json.dumps(summary, sort_keys=True))


def run_ctest_regressions(build_dir: Path, configuration: str | None) -> None:
    command = ["ctest", "--test-dir", str(build_dir)]
    if configuration:
        command.extend(["-C", configuration])
    inventory = json.loads(subprocess.check_output(
        [*command, "-N", "--show-only=json-v1"], text=True))
    selector = ctest_selector(inventory)
    with tempfile.TemporaryDirectory(prefix="libdxfrw fast ctest ") as directory:
        junit_path = Path(directory) / "ctest.xml"
        subprocess.run([*command, "-R", selector, "--no-tests=error",
                        "--timeout", "60", "--output-on-failure",
                        "--output-junit", str(junit_path)], check=True)
        require_ctest_passes(junit_path)


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
    # The default self-test needs no independent-reader package. Optional
    # runtime/family qualification and arbitrary user samples stay unselected.
    for excluded in (
        "libdxfrw_independent_dxf_runtime", "libdxfrw_independent_dxf_core",
        "libdxfrw_independent_dxf_spline-unit", "libdxfrw_independent_dxf_mtext-height",
        "libdxfrw_independent_dxf_header90", "dwg_read_user_sample",
        "libdxfrw_target_differential_advisory",
    ):
        assert re.fullmatch(selector, excluded) is None
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
    # CMake preserves empty option values, e.g. an unset compiler target or
    # sysroot in the header harness. Only argv[0] must be nonempty.
    optional_empty = {"tests": [dict(row) for row in inventory["tests"]]}
    optional_empty["tests"][0]["command"] = ["generated-test", "--sysroot", ""]
    assert ctest_selector(optional_empty) == selector
    for name in CTEST_REGRESSIONS:
        missing = {"tests": [row for row in inventory["tests"] if row["name"] != name]}
        try:
            ctest_selector(missing)
        except ValueError:
            pass
        else:
            raise AssertionError(f"missing required fast CTest {name!r} accepted")
    for command in (None, [], [""], ["", "--sentinel"], ["ctest", None]):
        unbuilt = {"tests": [dict(row) for row in inventory["tests"]]}
        unbuilt["tests"][0]["command"] = command
        try:
            ctest_selector(unbuilt)
        except ValueError:
            pass
        else:
            raise AssertionError("required fast CTest with invalid built command accepted")
    with tempfile.TemporaryDirectory(prefix="libdxfrw fast results ") as directory:
        junit_path = Path(directory) / "ctest.xml"
        passed = [f'<testcase name="{name}" status="run"/>'
                  for name in CTEST_REGRESSIONS]
        def write_results(cases: list[str]) -> None:
            junit_path.write_text("<testsuite>" + "".join(cases) + "</testsuite>",
                                  encoding="utf-8")
        write_results(passed)
        require_ctest_passes(junit_path)
        for invalid in (
            passed[:-1], [*passed, passed[0]],
            [*passed, '<testcase name="unselected" status="run"/>'],
            *[[f'<testcase name="{CTEST_REGRESSIONS[0]}" {status}>{body}</testcase>',
               *passed[1:]] for status, body in (
                   ('status="run"', '<skipped/>'),
                   ('status="run"', '<failure/>'),
                   ('status="run"', '<error/>'),
                   ('status="disabled"', ''),
                   ('status="notrun"', ''),
                   ('status="skipped"', ''),
                   ('', ''),
               )],
        ):
            write_results(invalid)
            try:
                require_ctest_passes(junit_path)
            except ValueError:
                pass
            else:
                raise AssertionError("incomplete/nonpassing required CTest result accepted")
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
