#!/usr/bin/env python3
"""Replay the ordered FreeCAD importer candidate against its source archive.

This is a source-application check, not a build or interoperability claim. It
reads five files from a hash-bound FreeCAD source archive into a disposable
directory, applies the patches there and optionally compares the result with a
local candidate checkout. It never changes the archive or candidate checkout.
Update PATCHES in order whenever the downstream chain changes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tarfile
import tempfile

ARCHIVES = {
    "29bdfe30632e084c0b6d015a1258f0ff75b8ed84fca660cec4e74fd4e2604bb9":
        "d956ec561639cfa83f755504b1b3358ea799428e",
    "5ba6c1409d59405dfdb0ddac9922633171b6d9117e9776913b3ca0888b3870f4":
        "157bfcb7462a733b03a7d041aeaa79fee7aff7a3",
}
FILES = (
    "src/Mod/Import/App/dxf/ImpExpDxf.cpp",
    "src/Mod/Import/App/dxf/ImpExpDxf.h",
    "src/Mod/Import/App/dxf/dxf.cpp",
    "src/Mod/Import/App/dxf/dxf.h",
    "src/Mod/Import/importtests/TestImportDXF.py",
)
PATCHES = (
    "freecad-p8-default-solid-ocs-2026-11-15.patch",
    "freecad-p8-default-planar-3dface-2026-11-15.patch",
    "freecad-p8-default-classic-polyline-flags-2026-11-15.patch",
    "freecad-p8-default-bounded-polyface-2026-11-15.patch",
    "freecad-p8-default-open-polygon-mesh-2026-11-15.patch",
    "freecad-p8-default-closed-polygon-mesh-2026-11-15.patch",
    "freecad-p8-default-shared-mesh-layer-2026-11-16.patch",
    "freecad-p8-default-block-mesh-layer-refusal-2026-11-16.patch",
    "freecad-p8-default-mesh-parent-aci-color-2026-11-16.patch",
    "freecad-p8-default-mesh-color-consistency-2026-11-16.patch",
    "freecad-p8-default-polyface-uniform-face-aci-2026-11-16.patch",
    "freecad-p8-default-mixed-face-no-color-2026-11-16.patch",
    "freecad-p8-default-mixed-vertex-no-color-2026-11-16.patch",
    "freecad-p8-default-byblock-no-color-2026-11-16.patch",
    "freecad-p8-default-off-layer-color-2026-11-16.patch",
    "freecad-p8-off-layer-no-group-diagnostic-2026-11-16.patch",
    "freecad-p8-frozen-layer-initial-visibility-2026-11-16.patch",
    "freecad-p8-layer-color-range-guard-2026-11-16.patch",
    "freecad-p8-layer-color-syntax-guard-2026-11-16.patch",
    "freecad-p8-layer-color-sign-whitespace-2026-11-16.patch",
)


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            result.update(chunk)
    return result.hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-archive", required=True, type=Path)
    parser.add_argument("--candidate-root", type=Path)
    parser.add_argument("--patch-dir", type=Path,
                        default=Path(__file__).resolve().parents[2] / "docs" / "patches")
    args = parser.parse_args()
    archive_hash = digest(args.source_archive)
    source_commit = ARCHIVES.get(archive_hash)
    if source_commit is None:
        raise ValueError("FreeCAD source archive SHA-256 does not match a recorded baseline")
    patch_dir = args.patch_dir.resolve()
    patch_hashes = {}
    with tempfile.TemporaryDirectory(prefix="freecad-patch-replay-") as temp:
        root = Path(temp)
        with tarfile.open(args.source_archive, "r:gz") as archive:
            for relative in FILES:
                member = archive.getmember("FreeCAD-" + source_commit + "/" + relative)
                if not member.isfile() or member.size > 20 * 1024 * 1024:
                    raise ValueError("unexpected archive entry: " + relative)
                source = archive.extractfile(member)
                if source is None:
                    raise ValueError("unreadable archive entry: " + relative)
                target = root / relative
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(source.read())
        for name in PATCHES:
            patch = patch_dir / name
            patch_hashes[name] = digest(patch)
            # The upstream header contains CRLF bytes, while this patch chain
            # was produced with input normalization. Do not depend on the
            # caller's global core.autocrlf setting.
            proc = subprocess.run(["git", "-c", "core.autocrlf=input", "apply", "--unidiff-zero", str(patch)], cwd=root,
                                  capture_output=True, text=True, check=False)
            if proc.returncode:
                raise RuntimeError("patch failed: " + name + "\n" + proc.stderr.strip())
        actual_files = set()
        for item in root.rglob("*"):
            if item.is_symlink():
                raise ValueError("patch created an unexpected symlink: " + str(item.relative_to(root)))
            if item.is_file():
                actual_files.add(item.relative_to(root).as_posix())
            elif not item.is_dir():
                raise ValueError("patch created an unexpected entry: " + str(item.relative_to(root)))
        if actual_files != set(FILES):
            raise ValueError("patch replay changed the expected source-file inventory")
        output_hashes = {relative: digest(root / relative) for relative in FILES}
        if args.candidate_root:
            for relative, expected in output_hashes.items():
                if digest(args.candidate_root / relative) != expected:
                    raise ValueError("candidate file differs from replay: " + relative)
        print(json.dumps({"sourceCommit": source_commit, "archiveSha256": archive_hash,
                          "patchCount": len(PATCHES), "patchSha256": patch_hashes,
                          "outputSha256": output_hashes,
                          "candidateCompared": args.candidate_root is not None}, sort_keys=True))


if __name__ == "__main__":
    main()
