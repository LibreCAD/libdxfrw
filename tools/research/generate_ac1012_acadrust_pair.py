#!/usr/bin/env python3
"""Generate scratch-only, pinned acadrust legacy drawing controls.

Requires an existing clean checkout at the pinned revision and Cargo. The tool
builds a disposable project, never changes the checkout, and records the actual
resolved Cargo.lock hash. Output must be an empty directory outside this repo.
The drawings do not establish AutoCAD acceptance or native clone semantics.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

PINNED = "d7c71f5032fc2d8844142ad7df7a6f0ee5ac535f"
FILES = ("destination.dwg", "source.dwg", "destination-resaved.dwg", "source-resaved.dwg")


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(args, **kwargs):
    return subprocess.run(args, check=True, text=True, capture_output=True, **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--profile", choices=("ac1012-pair", "ac1014-rooted-line"),
                        default="ac1012-pair")
    parser.add_argument("--acadrust-root", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--cargo-target-dir", type=Path)
    parser.add_argument("--cargo-lock", type=Path,
                        help="reuse an earlier generated Cargo.lock without updating dependencies")
    parser.add_argument("--offline", action="store_true")
    args = parser.parse_args()
    source = args.acadrust_root.resolve(strict=True)
    output = args.output_dir.resolve()
    repo = Path(__file__).resolve().parents[2]
    if output == repo or repo in output.parents:
        raise ValueError("sample output must be outside the libdxfrw repository")
    if run(["git", "-C", str(source), "rev-parse", "HEAD"]).stdout.strip() != PINNED:
        raise ValueError("acadrust checkout does not match the pinned commit")
    if run(["git", "-C", str(source), "status", "--porcelain", "--untracked-files=all"]).stdout:
        raise ValueError("acadrust checkout must be clean")
    output.mkdir(parents=True, exist_ok=True)
    if any(output.iterdir()):
        raise ValueError("output directory must be empty")
    r14 = args.profile == "ac1014-rooted-line"
    generator = Path(__file__).with_name(
        "ac1014_acadrust_rooted_line.rs" if r14 else "ac1012_acadrust_pair.rs")
    with tempfile.TemporaryDirectory(prefix="ac1012-acadrust-build-") as directory:
        project = Path(directory)
        (project / "src").mkdir()
        (project / "src" / "main.rs").write_bytes(generator.read_bytes())
        (project / "Cargo.toml").write_text(
            '[package]\nname = "ac1012-distinct-witness"\nversion = "0.1.0"\nedition = "2021"\n'
            '\n[dependencies]\nacadrust = { path = ' + json.dumps(str(source))
            + ', default-features = false }\n', encoding="utf-8")
        if args.cargo_lock:
            (project / "Cargo.lock").write_bytes(args.cargo_lock.resolve(strict=True).read_bytes())
        environment = os.environ.copy()
        environment["AC1012_WITNESS_OUTPUT"] = str(output)
        if args.cargo_target_dir:
            environment["CARGO_TARGET_DIR"] = str(args.cargo_target_dir.resolve())
        command = ["cargo", "run", "--manifest-path", str(project / "Cargo.toml")]
        if args.cargo_lock:
            command.append("--locked")
        if args.offline:
            command.append("--offline")
        result = run(command, env=environment, timeout=600)
        lock_hash = sha(project / "Cargo.lock")
        (output / "Cargo.lock").write_bytes((project / "Cargo.lock").read_bytes())
    samples = {}
    files = ("r14-line.dwg", "r14-line-rooted-resaved.dwg") if r14 else FILES
    magic = b"AC1014" if r14 else b"AC1012"
    for name in files:
        path = output / name
        if not path.is_file() or path.read_bytes()[:6] != magic:
            raise ValueError("writer did not produce the expected " + magic.decode() + " sample: " + name)
        samples[name] = {"sha256": sha(path), "bytes": path.stat().st_size}
    receipt = {"schema": ("libdxfrw.ac1014-acadrust-rooted-line.v1" if r14
                          else "libdxfrw.ac1012-acadrust-pair.v1"), "ok": True,
               "producerCommit": PINNED, "generatorSha256": sha(generator),
               "cargoLockSha256": lock_hash, "cargoVersion": run(["cargo", "--version"]).stdout.strip(),
               "rustVersion": run(["rustc", "--version"]).stdout.strip(), "samples": samples,
               "producerOutput": result.stdout.strip(), "supportClaimEffect": "none"}
    (output / "manifest.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"ok": True, "manifest": str(output / "manifest.json")}))


if __name__ == "__main__":
    main()
