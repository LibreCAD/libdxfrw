#!/usr/bin/env python3
"""Run the focused FreeCAD GUI view-property probe with identity checks.

The smoke profile does not establish rendered-pixel behavior or broad format
support. Run with a usable GUI display or an explicitly selected Qt platform;
record the display backend when interpreting the result.
"""
import argparse
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import tempfile

import run_freecad_candidate_probe as common
from freecad_framebuffer_capture import NAMES as PIXEL_FILES

HERE = Path(__file__).resolve().parent
MACRO = HERE / "freecad_gui_mesh_probe.FCMacro"
MARKER = "FREECAD_GUI_MESH_PROBE="
CASES = ("polyface", "polygon", "mixed", "nested_nolayers", "layer_on_2",
         "layer_off_2", "layer_frozen_2", "color", "color_mixed", "color_mixed_nocolor")


def analyze_rgb_frames(width, height, frames):
    """Check mesh removal, independent line removal and restoration in RGB frames."""
    size = width * height
    if width < 16 or height < 16 or len(frames) != 4 or any(len(frame) != size * 3 for frame in frames):
        raise ValueError("invalid framebuffer dimensions or data")

    def changed(first, second):
        return [index // 3 for index in range(0, size * 3, 3)
                if max(abs(first[index + channel] - second[index + channel])
                       for channel in range(3)) >= 24]

    mesh = changed(frames[0], frames[1])
    line = changed(frames[1], frames[2])
    restored = changed(frames[0], frames[3])
    if len(mesh) < max(64, size // 1000) or len(mesh) > size * 0.7 or len(line) < 4:
        raise ValueError("mesh or independent line visibility did not change enough pixels")
    xs, ys = [p % width for p in mesh], [p // width for p in mesh]
    bounds = [min(xs), min(ys), max(xs), max(ys)]
    if bounds[2] - bounds[0] < 8 or bounds[3] - bounds[1] < 8:
        raise ValueError("mesh pixel difference is too small in either dimension")
    if len(restored) > max(16, len(mesh) // 100):
        raise ValueError("restored framebuffer differs from the original")
    return {"width": width, "height": height, "meshChangedPixels": len(mesh),
            "lineChangedPixels": len(line), "restorationChangedPixels": len(restored),
            "meshDifferenceBounds": bounds, "channelThreshold": 24}


def analyze_pixel_files(run_dir):
    from PIL import Image

    frames, hashes = [], {}
    for name in PIXEL_FILES:
        path = run_dir / name
        with Image.open(path) as image:
            if image.format != "PNG" or image.size != (320, 240):
                raise ValueError("unexpected framebuffer image format or dimensions")
            frames.append(image.convert("RGB").tobytes())
        hashes[name] = common.digest(path)
    return {**analyze_rgb_frames(320, 240, frames), "imageSha256": hashes}


def gui_diagnostic_failures(stdout, stderr):
    """Detect observed fatal GUI/context failures even when FreeCAD exits zero."""
    output = (stdout + "\n" + stderr).casefold()
    signatures = {
        "unhandled_gui_exception": "unhandled base::exception caught in guiapplication::notify",
        "access_violation": "the error message is: access violation",
        "unsupported_opengl": "freecad requires opengl 2.0 or above",
        "texture_allocation_failure": "qopenglwidget: failed to create wrapper texture",
        "opengl_context_failure": "error when setting up the gl context",
    }
    return {name: output.count(text) for name, text in signatures.items() if text in output}


def require_windows_opengl(expected_path, observed_path, build_root, platform_name):
    expected = expected_path.resolve(strict=True)
    if platform_name != "win32" or not expected.is_file() or not common.within(expected, build_root):
        raise RuntimeError("required OpenGL module is not a Windows build-local file")
    if not observed_path or Path(observed_path).resolve(strict=True) != expected:
        raise RuntimeError("FreeCAD did not load the required OpenGL module")
    return common.digest(expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-archive", required=True, type=Path)
    parser.add_argument("--expected-source-commit", required=True)
    parser.add_argument("--require-git-base", action="store_true")
    parser.add_argument("--source-root", required=True, type=Path)
    parser.add_argument("--build-root", required=True, type=Path)
    parser.add_argument("--freecad", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--pyside-dir", type=Path)
    parser.add_argument("--draft-resource-dir", type=Path,
                        help="explicit resource directory; defaults to the selected build's Mod/Draft")
    parser.add_argument("--qt-platform", help="optional Qt platform plugin, e.g. offscreen")
    parser.add_argument("--require-windows-opengl", type=Path,
                        help="require this build-local OpenGL module on Windows")
    parser.add_argument("--require-pixels", action="store_true",
                        help="also verify framebuffer visibility and restoration")
    parser.add_argument("--timeout", type=int, default=600)
    args = parser.parse_args()
    if not 1 <= args.timeout <= 3600:
        parser.error("--timeout must be between 1 and 3600 seconds")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    run_dir = Path(tempfile.mkdtemp(prefix="freecad-gui-probe-", dir=args.output_dir)).resolve()
    receipt = {"ok": False}
    try:
        source = args.source_root.resolve(strict=True)
        build = args.build_root.resolve(strict=True)
        command = args.freecad.resolve(strict=True)
        resource_root = (args.draft_resource_dir or (build / "Mod" / "Draft")).resolve(strict=True)
        if not command.is_file() or not common.within(command, build):
            raise ValueError("FreeCAD GUI executable must be inside the build root")
        cache = build / "CMakeCache.txt"
        prefix = "CMAKE_HOME_DIRECTORY:INTERNAL="
        homes = [line[len(prefix):] for line in cache.read_text(encoding="utf-8", errors="replace").splitlines()
                 if line.startswith(prefix)]
        if len(homes) != 1 or Path(homes[0]).resolve(strict=True) != source:
            raise ValueError("FreeCAD build was not configured from the selected candidate source")
        replay = subprocess.run([sys.executable, str(common.REPLAY), "--source-archive", str(args.source_archive),
                                 "--candidate-root", str(source)], capture_output=True, text=True,
                                timeout=args.timeout, check=False)
        if replay.returncode:
            raise RuntimeError("source replay failed: " + replay.stderr.strip())
        source_receipt = json.loads(replay.stdout)
        if not source_receipt.get("candidateCompared") or source_receipt.get("sourceCommit") != args.expected_source_commit:
            raise RuntimeError("source replay did not match the expected candidate commit")
        git_base_verified = False
        if args.require_git_base:
            head = subprocess.run(["git", "-C", str(source), "rev-parse", "HEAD"], capture_output=True,
                                  text=True, check=True).stdout.strip()
            changed = subprocess.run(["git", "-C", str(source), "diff", "--name-only", "HEAD",
                                      "--ignore-submodules=all"], capture_output=True, text=True, check=True).stdout.splitlines()
            if head != args.expected_source_commit or set(changed) != set(source_receipt["outputSha256"]):
                raise RuntimeError("source checkout does not have the expected base and patched-file set")
            git_base_verified = True
        env = os.environ.copy()
        env.update({"FREECAD_RESEARCH_TOOLS_DIR": str(HERE), "FREECAD_GUI_MESH_PROFILE": "smoke",
                    "FREECAD_3RDPARTY_DIR": str(source / "src" / "3rdParty"),
                    "FREECAD_DRAFT_RESOURCE_DIR": str(resource_root),
                    "FREECAD_DRAFT_MODULE_DIR": str(source / "src" / "Mod" / "Draft"),
                    "FREECAD_PART_MODULE_DIR": str(source / "src" / "Mod" / "Part")})
        if args.pyside_dir:
            env["FREECAD_PYSIDE_DIR"] = str(args.pyside_dir.resolve(strict=True))
        if args.qt_platform:
            env["QT_QPA_PLATFORM"] = args.qt_platform
        if args.require_pixels:
            env["FREECAD_GUI_CAPTURE_DIR"] = str(run_dir)
        try:
            proc = subprocess.run([str(command), "--safe-mode", "--user-cfg", str(run_dir / "user.cfg"),
                                   "--system-cfg", str(run_dir / "system.cfg"), str(MACRO)],
                                  cwd=run_dir, env=env, capture_output=True, text=True,
                                  errors="replace", timeout=args.timeout, check=False)
        except subprocess.TimeoutExpired as exc:
            (run_dir / "stdout.log").write_text(common.log_text(exc.stdout), encoding="utf-8")
            (run_dir / "stderr.log").write_text(common.log_text(exc.stderr), encoding="utf-8")
            raise RuntimeError("FreeCAD GUI timed out; partial output was preserved") from exc
        (run_dir / "stdout.log").write_text(proc.stdout, encoding="utf-8")
        (run_dir / "stderr.log").write_text(proc.stderr, encoding="utf-8")
        failures = gui_diagnostic_failures(proc.stdout, proc.stderr)
        if failures:
            raise RuntimeError("FreeCAD GUI emitted fatal diagnostics: " + json.dumps(failures, sort_keys=True))
        markers = [line.split(MARKER, 1)[1] for line in proc.stdout.splitlines() if MARKER in line]
        if proc.returncode or len(markers) != 1:
            raise RuntimeError("FreeCAD GUI failed or did not emit exactly one result")
        report = json.loads(markers[0])
        if not report.get("ok"):
            raise RuntimeError("GUI smoke probe failed: " + str(report.get("error", "unknown error")))
        if report.get("guiUp") != 1 or report.get("profile") != "smoke" or [x.get("case") for x in report.get("results", [])] != list(CASES):
            raise RuntimeError("FreeCAD GUI did not complete the exact smoke profile")
        importer = Path(report.get("importModule") or "").resolve(strict=True)
        draft = Path(report.get("draftModule") or "").resolve(strict=True)
        draft_resource = Path(report.get("draftResourceModule") or "").resolve(strict=True)
        executable = Path(report.get("executable") or "").resolve(strict=True)
        if executable != command or not importer.is_file() or not common.within(importer, build):
            raise RuntimeError("FreeCAD GUI executable or ImportGui module differs from the build")
        if draft != (source / "src" / "Mod" / "Draft" / "Draft.py").resolve(strict=True):
            raise RuntimeError("FreeCAD loaded Draft from a different source")
        if not draft_resource.is_file() or not common.within(draft_resource, resource_root):
            raise RuntimeError("FreeCAD loaded Draft resources outside the selected resource directory")
        if report.get("platform") != sys.platform or common.machine_name(report.get("machine", "")) != common.machine_name(platform.machine()):
            raise RuntimeError("FreeCAD GUI platform or architecture differs from the host")
        if not report.get("qtPlatform") or (args.qt_platform and report["qtPlatform"] != args.qt_platform):
            raise RuntimeError("FreeCAD GUI did not report the expected Qt platform")
        opengl_digest = None
        if args.require_windows_opengl:
            opengl_digest = require_windows_opengl(args.require_windows_opengl,
                                                   report.get("windowsOpenGLModule"), build, sys.platform)
        if args.require_pixels and report["results"][0].get("detail", {}).get("pixelFiles") != list(PIXEL_FILES):
            raise RuntimeError("GUI did not report the requested framebuffer sequence")
        pixels = analyze_pixel_files(run_dir) if args.require_pixels else None
        receipt = {"ok": True, "scope": ("focused GUI view-property and framebuffer checks" if pixels
                                         else "focused GUI view-property checks; no pixel-rendering claim"),
                   "sourceReplay": source_receipt, "gitBaseVerified": git_base_verified,
                   "cmakeSourceRoot": homes[0], "cmakeCacheSha256": common.digest(cache),
                   "freecadSha256": common.digest(command), "importGuiSha256": common.digest(importer),
                   "draftSha256": common.digest(draft), "draftResourceSha256": common.digest(draft_resource),
                   "draftResourceWithinBuild": common.within(draft_resource, build), "macroSha256": common.digest(MACRO),
                   "qtPlatformRequested": args.qt_platform, "requiredOpenGLSha256": opengl_digest,
                   "pixelEvidence": pixels, "report": report}
    except Exception as exc:
        receipt = {"ok": False, "error": str(exc)}
    (run_dir / "receipt.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"ok": receipt["ok"], "receipt": str(run_dir / "receipt.json"),
                      "error": receipt.get("error")}, sort_keys=True), flush=True)
    return 0 if receipt["ok"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
