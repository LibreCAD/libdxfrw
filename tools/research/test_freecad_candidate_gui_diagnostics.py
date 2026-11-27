#!/usr/bin/env python3
"""Focused checks for GUI process diagnostics that may accompany exit zero."""
import hashlib
from pathlib import Path
import tempfile
import unittest

from run_freecad_candidate_gui_probe import gui_diagnostic_failures, require_windows_opengl


class GuiDiagnosticTests(unittest.TestCase):
    def test_unhandled_exception_is_rejected_from_either_stream(self):
        message = ("Unhandled Base::Exception caught in GUIApplication::notify.\n"
                   "The error message is: Access violation\n")
        expected = {"unhandled_gui_exception": 1, "access_violation": 1}
        self.assertEqual(gui_diagnostic_failures(message, ""), expected)
        self.assertEqual(gui_diagnostic_failures("", message), expected)

    def test_unsupported_gl_and_context_failures_are_reported(self):
        message = ("FreeCAD requires OpenGL 2.0 or above.\n"
                   "QOpenGLWidget: Failed to create wrapper texture\n"
                   "Error when setting up the GL context.\n")
        self.assertEqual(gui_diagnostic_failures("", message),
                         {"unsupported_opengl": 1, "texture_allocation_failure": 1,
                          "opengl_context_failure": 1})

    def test_unrelated_warnings_do_not_fail(self):
        self.assertEqual(gui_diagnostic_failures(
            "GUI smoke completed", "Cannot find icon: optional.svg\nFirst reference to missing Layer '0'"), {})

    def test_opengl_identity_requires_the_build_local_loaded_module(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary).resolve()
            build = root / "build"
            build.mkdir()
            required = build / "opengl32.dll"
            required.write_bytes(b"local test library")
            other = root / "opengl32.dll"
            other.write_bytes(b"other library")
            digest = hashlib.sha256(required.read_bytes()).hexdigest()
            self.assertEqual(require_windows_opengl(required, str(required), build, "win32"), digest)
            with self.assertRaises(RuntimeError):
                require_windows_opengl(required, str(other), build, "win32")
            with self.assertRaises(RuntimeError):
                require_windows_opengl(other, str(other), build, "win32")
            with self.assertRaises(RuntimeError):
                require_windows_opengl(required, str(required), build, "linux")
            with self.assertRaises(RuntimeError):
                require_windows_opengl(required, None, build, "win32")


if __name__ == "__main__":
    unittest.main()
