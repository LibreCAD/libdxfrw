#!/usr/bin/env python3
"""Run the focused FreeCAD candidate probe with source and runtime checks.

The result is limited to the selected headless tests. It does not qualify GUI
rendering, independent producer semantics, or broad format support. The
optional Git check covers HEAD and tracked changed paths; it does not verify
untracked or ignored files, submodule contents, or the build provenance.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
PROBE = HERE / "freecad_candidate_headless_probe.py"
REPLAY = HERE / "replay_freecad_import_patches.py"
MARKER = "FREECAD_CANDIDATE_HEADLESS="


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            result.update(chunk)
    return result.hexdigest()


def within(path, root):
    try:
        path.relative_to(root)
        return True
    except ValueError:
        return False


def machine_name(value):
    value = value.lower()
    return {"amd64": "x86_64", "x64": "x86_64", "aarch64": "arm64"}.get(value, value)


def log_text(value):
    if isinstance(value, bytes):
        return value.decode("utf-8", errors="replace")
    return value or ""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-archive", required=True, type=Path)
    parser.add_argument("--expected-source-commit", required=True)
    parser.add_argument("--require-git-base", action="store_true",
                        help="require the source checkout at the expected base with only the patched files changed")
    parser.add_argument("--source-root", required=True, type=Path)
    parser.add_argument("--build-root", required=True, type=Path)
    parser.add_argument("--freecadcmd", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--timeout", type=int, default=300)
    args = parser.parse_args()
    if args.timeout < 1 or args.timeout > 3600:
        parser.error("--timeout must be between 1 and 3600 seconds")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    run_dir = Path(tempfile.mkdtemp(prefix="freecad-candidate-probe-", dir=args.output_dir))
    receipt = {"ok": False}
    try:
        source = args.source_root.resolve(strict=True)
        build = args.build_root.resolve(strict=True)
        command = args.freecadcmd.resolve(strict=True)
        if not command.is_file() or not within(command, build):
            raise ValueError("FreeCADCmd must be a file inside the selected build root")
        cache = build / "CMakeCache.txt"
        prefix = "CMAKE_HOME_DIRECTORY:INTERNAL="
        homes = [line[len(prefix):] for line in cache.read_text(encoding="utf-8", errors="replace").splitlines()
                 if line.startswith(prefix)]
        if len(homes) != 1 or Path(homes[0]).resolve(strict=True) != source:
            raise ValueError("FreeCAD build was not configured from the selected candidate source")
        replay = subprocess.run([sys.executable, str(REPLAY), "--source-archive", str(args.source_archive),
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
        spec = importlib.util.spec_from_file_location("freecad_candidate_probe_definition", PROBE)
        if spec is None or spec.loader is None:
            raise RuntimeError("cannot load the probe definition")
        definition = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(definition)
        env = os.environ.copy()
        env["FREECAD_CANDIDATE_SOURCE_ROOT"] = str(source)
        env["FREECAD_CANDIDATE_SOURCE_COMMIT"] = args.expected_source_commit
        try:
            proc = subprocess.run([str(command), "--user-cfg", str(run_dir / "user.cfg"),
                                   "--system-cfg", str(run_dir / "system.cfg"), str(PROBE)],
                                  cwd=run_dir, env=env, capture_output=True, text=True,
                                  errors="replace", timeout=args.timeout, check=False)
        except subprocess.TimeoutExpired as exc:
            (run_dir / "stdout.log").write_text(log_text(exc.stdout), encoding="utf-8")
            (run_dir / "stderr.log").write_text(log_text(exc.stderr), encoding="utf-8")
            raise RuntimeError("FreeCADCmd timed out; partial output was preserved") from exc
        (run_dir / "stdout.log").write_text(proc.stdout, encoding="utf-8")
        (run_dir / "stderr.log").write_text(proc.stderr, encoding="utf-8")
        markers = [line.split(MARKER, 1)[1] for line in proc.stdout.splitlines() if MARKER in line]
        if proc.returncode != 0 or len(markers) != 1:
            raise RuntimeError("FreeCADCmd failed or did not emit exactly one result")
        report = json.loads(markers[0])
        importer = Path(report.get("importModule") or "").resolve(strict=True)
        executable = Path(report.get("executable") or "").resolve(strict=True)
        expected_test = source / "src" / "Mod" / "Import" / "importtests" / "TestImportDXF.py"
        if not report.get("ok") or report.get("guiUp") != 0 or report.get("skipped") != 0 or report.get("failures") != 0 or report.get("errors") != 0:
            raise RuntimeError("focused FreeCAD tests failed or were skipped")
        expected_tests = definition.tests_for_commit(args.expected_source_commit)
        if report.get("tests") != list(expected_tests) or report.get("testsRun") != len(expected_tests) or report.get("testsExpected") != len(expected_tests):
            raise RuntimeError("FreeCAD did not execute the exact required test set")
        if Path(report.get("sourceRoot") or "").resolve() != source or Path(report.get("testModule") or "").resolve() != expected_test:
            raise RuntimeError("FreeCAD used a different candidate source or test module")
        if executable != command or not importer.is_file() or not within(importer, build):
            raise RuntimeError("FreeCAD used an executable or Import module outside the selected build")
        if report.get("platform") != sys.platform or machine_name(report.get("machine", "")) != machine_name(platform.machine()):
            raise RuntimeError("FreeCAD platform or architecture differs from the host")
        receipt = {"ok": True, "scope": "focused headless candidate checks only",
                   "sourceReplay": source_receipt, "gitBaseVerified": git_base_verified, "buildRoot": str(build),
                   "cmakeSourceRoot": homes[0], "cmakeCacheSha256": digest(cache),
                   "freecadcmd": str(command), "freecadcmdSha256": digest(command),
                   "importModule": str(importer), "importModuleSha256": digest(importer),
                   "probeSha256": digest(PROBE), "report": report}
    except Exception as exc:
        receipt = {"ok": False, "error": str(exc)}
    (run_dir / "receipt.json").write_text(json.dumps(receipt, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"ok": receipt["ok"], "receipt": str(run_dir / "receipt.json"),
                      "error": receipt.get("error")}, sort_keys=True), flush=True)
    return 0 if receipt["ok"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
