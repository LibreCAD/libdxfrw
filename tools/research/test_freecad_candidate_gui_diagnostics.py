#!/usr/bin/env python3
"""Focused checks for GUI process diagnostics that may accompany exit zero."""
import hashlib
from pathlib import Path
import tempfile
import unittest

from freecad_framebuffer_capture import NAMES, capture
from run_freecad_candidate_gui_probe import (analyze_rgb_frames, gui_diagnostic_failures,
                                              require_windows_opengl)


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

    def test_pixel_guard_checks_mesh_line_and_restoration(self):
        width = height = 64
        blank = bytearray([255] * (width * height * 3))
        line = blank.copy()
        for x in range(35, 51):
            pos = (40 * width + x) * 3
            line[pos:pos + 3] = bytes((0, 0, 0))
        visible = line.copy()
        for y in range(4, 21):
            for x in range(4, 21):
                pos = (y * width + x) * 3
                visible[pos:pos + 3] = bytes((200, 20, 20))
        result = analyze_rgb_frames(width, height, [visible, line, blank, visible])
        self.assertEqual(result["meshChangedPixels"], 17 * 17)
        self.assertEqual(result["lineChangedPixels"], 16)
        self.assertEqual(result["restorationChangedPixels"], 0)
        for bad in ([blank, blank, blank, blank], [visible, line, line, visible],
                    [visible, line, blank, blank]):
            with self.assertRaises(ValueError):
                analyze_rgb_frames(width, height, bad)
        with self.assertRaises(ValueError):
            analyze_rgb_frames(width, height, [visible[:-1], line, blank, visible])

    def test_capture_restores_visibility_after_failure(self):
        class Item:
            Visibility = True

        class View:
            def viewTop(self): pass
            def fitAll(self): pass
            def saveImage(self, path, width, height, color):
                if Path(path).name == NAMES[1]:
                    raise RuntimeError("simulated capture failure")
                Path(path).write_bytes(b"scratch")

        mesh, line = Item(), Item()
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaises(RuntimeError):
                capture(View(), mesh, line, temporary, lambda: None)
            self.assertTrue(mesh.Visibility)
            self.assertTrue(line.Visibility)

    def test_capture_sequence_keeps_line_as_an_independent_control(self):
        class Item:
            Visibility = True

        mesh, line = Item(), Item()

        class View:
            states = []
            def viewTop(self): pass
            def fitAll(self): pass
            def saveImage(self, path, width, height, color):
                self.states.append((mesh.Visibility, line.Visibility, width, height, color))
                Path(path).write_bytes(b"scratch")

        with tempfile.TemporaryDirectory() as temporary:
            view = View()
            self.assertEqual(capture(view, mesh, line, temporary, lambda: None), list(NAMES))
            self.assertEqual(view.states, [(True, True, 320, 240, "White"),
                                           (False, True, 320, 240, "White"),
                                           (False, False, 320, 240, "White"),
                                           (True, True, 320, 240, "White")])
            with self.assertRaises(ValueError):
                capture(view, mesh, line, temporary, lambda: None)

    def test_capture_rejects_coupled_visibility(self):
        class Item:
            Visibility = True

        line = Item()

        class Mesh:
            _visible = True
            @property
            def Visibility(self): return self._visible
            @Visibility.setter
            def Visibility(self, value):
                self._visible = value
                if not value: line.Visibility = False

        class View:
            def viewTop(self): pass
            def fitAll(self): pass
            def saveImage(self, path, width, height, color): Path(path).write_bytes(b"scratch")

        mesh = Mesh()
        def propagate():
            if not mesh.Visibility: line.Visibility = False
        with tempfile.TemporaryDirectory() as temporary:
            with self.assertRaisesRegex(RuntimeError, "line=False"):
                capture(View(), mesh, line, temporary, propagate)
            self.assertTrue(mesh.Visibility)
            self.assertTrue(line.Visibility)


if __name__ == "__main__":
    unittest.main()
