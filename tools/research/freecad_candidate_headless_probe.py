"""Run focused candidate-importer tests inside a FreeCADCmd process.

Set FREECAD_CANDIDATE_SOURCE_ROOT and FREECAD_CANDIDATE_SOURCE_COMMIT to the
patched FreeCAD source and its verified base, and use disposable configuration
files. The caller must verify the
source and executable identities and require exactly one JSON result with
``ok`` true; FreeCADCmd's process exit status alone is not a test result.
This probe does not establish GUI rendering or producer interoperability.
"""
import importlib.util
import json
import os
from pathlib import Path
import platform
import sys
import traceback
import unittest

TESTS = (
    "test_read_lines",
    "test_dimension_ocs_orientation_axes",
    "test_read_solid_ocs_vertices",
    "test_read_3dface_and_report_unsupported_profiles",
    "test_classic_polyline_header_flags_and_mesh_disposition",
    "test_bounded_polyface_geometry_and_rejections",
    "test_polyface_invalid_then_valid_same_file",
    "test_bounded_polygon_mesh_grid_and_rejections",
    "test_polygon_mesh_invalid_then_valid_same_file",
    "test_closed_polygon_mesh_seams_and_refusals",
    "test_closed_polygon_mesh_invalid_then_valid_same_file",
    "test_off_layer_without_grouping_reports_visibility_loss",
    "test_frozen_layer_without_grouping_remains_reported",
    "test_out_of_range_layer_colors_report_and_keep_off_state",
    "test_malformed_layer_colors_report_and_keep_off_state",
)

EXTRA_TESTS = {
    "477a7041803823a1fcae528133cb07ee5533b1b2": (
        "test_polyface_display_invalid_then_valid_same_file",
        "test_3dface_block_layer_loss_is_reported",
        "test_block_line_named_layer_loss_is_reported",
        "test_block_circle_named_layer_loss_is_reported",
        "test_nested_block_original_layer_metadata",
        "test_nested_insert_named_layer_loss_is_reported",
        "test_block_reference_cycles_and_retry",
        "test_block_annotation_depth_guard_and_retry",
    ),
}


def tests_for_commit(commit):
    return TESTS + EXTRA_TESTS.get(commit, ())


def main():
    report = {"ok": False}
    try:
        source = os.environ.get("FREECAD_CANDIDATE_SOURCE_ROOT")
        if not source:
            raise RuntimeError("FREECAD_CANDIDATE_SOURCE_ROOT is required")
        commit = os.environ.get("FREECAD_CANDIDATE_SOURCE_COMMIT")
        if not commit:
            raise RuntimeError("FREECAD_CANDIDATE_SOURCE_COMMIT is required")
        tests = tests_for_commit(commit)
        source_root = Path(source).resolve(strict=True)
        import_dir = source_root / "src" / "Mod" / "Import"
        test_path = import_dir / "importtests" / "TestImportDXF.py"
        sys.path.insert(0, str(import_dir))
        import FreeCAD as App
        import Import

        if App.GuiUp:
            raise RuntimeError("This probe requires a headless FreeCADCmd process")
        spec = importlib.util.spec_from_file_location("freecad_candidate_import_tests", test_path)
        if spec is None or spec.loader is None:
            raise RuntimeError("Cannot load the candidate FreeCAD DXF tests")
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        suite = unittest.TestSuite(module.TestImportDXF(name) for name in tests)
        result = unittest.TextTestRunner(verbosity=2).run(suite)
        report = {"ok": result.wasSuccessful() and result.testsRun == len(tests) and not result.skipped,
                  "sourceRoot": str(source_root), "testModule": str(test_path),
                  "importModule": getattr(Import, "__file__", None), "guiUp": App.GuiUp,
                  "executable": sys.executable, "freecadVersion": list(App.Version()),
                  "pythonVersion": sys.version, "platform": sys.platform, "machine": platform.machine(),
                  "testsRun": result.testsRun, "testsExpected": len(tests),
                  "failures": len(result.failures), "errors": len(result.errors),
                  "skipped": len(result.skipped), "tests": list(tests)}
    except Exception:
        report["error"] = traceback.format_exc()
    print("FREECAD_CANDIDATE_HEADLESS=" + json.dumps(report, sort_keys=True), flush=True)


if __name__ in ("__main__", Path(__file__).stem):
    main()
