#!/usr/bin/env python3
"""Generate bounded R13/R14 SHAPE controls outside the repository.

The pinned ACadRust writer shares DWG-code lineage with ACadSharp. These
drawings are producer controls, not native AutoCAD or support evidence.
Run with an already populated offline Cargo cache. No DWG/DXF is committed.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import tempfile

REVISION = "534ff8abaf112d9c9ec3e08292080002b0e9de3a"
PROGRAM = r'''use acadrust::entities::{Line, Shape};
use acadrust::tables::TextStyle;
use acadrust::{CadDocument, DwgWriter, DxfVersion, EntityType, Vector3};

fn main() {
    let output = std::path::PathBuf::from(std::env::args_os().nth(1).unwrap());
    for (version, release) in [(DxfVersion::AC1012, "r13"), (DxfVersion::AC1014, "r14")] {
        for valid_style in [false, true] {
            let mut doc = CadDocument::with_version(version);
            let mut shape = if valid_style {
                let mut style = TextStyle::new("GEN_SHAPES");
                style.handle = doc.allocate_handle();
                style.is_shape_file = true;
                style.font_file = "ltypeshp.shx".into();
                let handle = style.handle;
                doc.text_styles.add(style).unwrap();
                let mut result = Shape::with_number(Vector3::new(1., 2., 3.), 132, 5.);
                result.style_handle = Some(handle);
                result.style_name = "GEN_SHAPES".into();
                result
            } else {
                Shape::with_name(Vector3::new(1., 2., 3.), "BOX", 5.)
            };
            shape.shape_name = "BOX".into();
            doc.add_entity(EntityType::Shape(shape)).unwrap();
            doc.add_entity(EntityType::Line(Line::from_coords(21., 34., 55., 89., 144., 233.))).unwrap();
            let kind = if valid_style { "bound" } else { "null" };
            DwgWriter::write_to_file(output.join(format!("{release}-{kind}.dwg")), &doc).unwrap();
        }
    }
}
'''


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--acadrust-root", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--cargo-home", type=Path, required=True)
    parser.add_argument("--cargo-target", type=Path, required=True)
    args = parser.parse_args()
    source = args.acadrust_root.resolve(strict=True)
    output = args.output_dir.resolve()
    repository = Path(__file__).resolve().parents[2]
    if output == repository or repository in output.parents:
        parser.error("output directory must be outside the libdxfrw repository")
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=source, text=True).strip()
    if revision != REVISION:
        parser.error(f"ACadRust revision must be {REVISION}, got {revision}")
    if subprocess.run(["git", "diff", "--quiet", "HEAD", "--", "Cargo.toml", "src"], cwd=source).returncode:
        parser.error("ACadRust source or manifest has tracked modifications")
    if subprocess.check_output(["git", "ls-files", "--others", "--exclude-standard", "--", "src"], cwd=source):
        parser.error("ACadRust source has untracked files")
    output.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="acadrust-legacy-shape-", dir=output))
    project = root / "project"
    (project / "src").mkdir(parents=True)
    (project / "Cargo.toml").write_text(
        '[package]\nname = "libdxfrw-shape-witness"\nversion = "0.1.0"\nedition = "2021"\n'
        '[dependencies]\nacadrust = { path = ' + json.dumps(str(source)) + ' }\n', encoding="utf-8")
    (project / "src/main.rs").write_text(PROGRAM, encoding="utf-8")
    env = os.environ.copy()
    env["CARGO_HOME"] = str(args.cargo_home.resolve())
    env["CARGO_TARGET_DIR"] = str(args.cargo_target.resolve())
    subprocess.run(["cargo", "run", "--offline", "--manifest-path", str(project / "Cargo.toml"), "--", str(root)],
                   check=True, env=env)
    files = {}
    for release, magic in (("r13", b"AC1012"), ("r14", b"AC1014")):
        for kind in ("null", "bound"):
            path = root / f"{release}-{kind}.dwg"
            if path.read_bytes()[:6] != magic:
                raise RuntimeError(f"wrong DWG version in {path}")
            files[path.name] = {"sha256": sha256(path), "bytes": path.stat().st_size}
    binary_name = "libdxfrw-shape-witness.exe" if os.name == "nt" else "libdxfrw-shape-witness"
    binary = args.cargo_target.resolve() / "debug" / binary_name
    receipt = {"schema": 1, "scope": "generated producer controls, no native semantic claim",
               "producer": "ACadRust 0.6.8", "producerRevision": revision,
               "producerLineage": "ACadSharp DWG codec", "files": files,
               "generatorSha256": sha256(Path(__file__)),
               "rustProgramSha256": sha256(project / "src/main.rs"),
               "cargoLockSha256": sha256(project / "Cargo.lock"),
               "binarySha256": sha256(binary)}
    (root / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(root)


if __name__ == "__main__":
    main()
