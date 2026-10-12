#!/usr/bin/env python3
"""Create a locally authored, known-coordinate ACIS-218 DXF control.

This is a parser/carrier control, not an AutoCAD-produced witness.  It uses
ezdxf's documented mesh-to-ACIS polyhedron writer and deliberately writes all
outputs into a new temporary directory outside the repository.  It does not
create a DWG or ACDSDATA section; use a separately authorized ODA conversion
for those checks.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import sys
import tempfile
from pathlib import Path


DIMENSIONS = (5.0, 7.0, 11.0)


def build_box_mesh(mesh_builder_type, center):
    """Build a closed outward-oriented six-quad box mesh."""
    expected_min = tuple(c - d / 2.0 for c, d in zip(center, DIMENSIONS))
    expected_max = tuple(c + d / 2.0 for c, d in zip(center, DIMENSIONS))
    xmin, ymin, zmin = expected_min
    xmax, ymax, zmax = expected_max
    mesh = mesh_builder_type()

    # Each loop is wound so its normal points out of the solid.
    faces = (
        ((xmin, ymin, zmin), (xmin, ymax, zmin), (xmax, ymax, zmin), (xmax, ymin, zmin)),
        ((xmin, ymin, zmax), (xmax, ymin, zmax), (xmax, ymax, zmax), (xmin, ymax, zmax)),
        ((xmin, ymin, zmin), (xmin, ymin, zmax), (xmin, ymax, zmax), (xmin, ymax, zmin)),
        ((xmax, ymin, zmin), (xmax, ymax, zmin), (xmax, ymax, zmax), (xmax, ymin, zmax)),
        ((xmin, ymin, zmin), (xmax, ymin, zmin), (xmax, ymin, zmax), (xmin, ymin, zmax)),
        ((xmin, ymax, zmin), (xmin, ymax, zmax), (xmax, ymax, zmax), (xmax, ymax, zmin)),
    )
    for face in faces:
        mesh.add_face(face)
    return mesh


def create_output_directory(requested: Path | None, repository_root: Path) -> Path:
    if requested is None:
        output_dir = Path(tempfile.mkdtemp(prefix="libdxfrw-acis218-box-"))
    else:
        output_dir = requested.expanduser().resolve()
        if output_dir == repository_root or repository_root in output_dir.parents:
            raise ValueError("refusing to write generated drawing files inside the repository")
        if output_dir.exists():
            raise ValueError(f"refusing to reuse existing output directory: {output_dir}")
        output_dir.mkdir(parents=True)

    if output_dir == repository_root or repository_root in output_dir.parents:
        # This directory was created by this invocation when requested is None.
        if requested is None:
            output_dir.rmdir()
        raise ValueError("refusing to write generated drawing files inside the repository")
    return output_dir


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output-dir",
        type=Path,
        help="new output directory outside the repository (default: unique system temp directory)",
    )
    parser.add_argument(
        "--center",
        nargs=3,
        type=float,
        default=(0.0, 0.0, 0.0),
        metavar=("X", "Y", "Z"),
        help=("box center in ACIS model coordinates (default: origin; nonzero values test "
              "embedded transforms; drawing-unit conversion is not qualified)"),
    )
    args = parser.parse_args()
    center = tuple(args.center)
    if not all(math.isfinite(value) for value in center):
        parser.error("center coordinates must be finite")
    expected_min = tuple(c - d / 2.0 for c, d in zip(center, DIMENSIONS))
    expected_max = tuple(c + d / 2.0 for c, d in zip(center, DIMENSIONS))
    expected_local_min = tuple(-d / 2.0 for d in DIMENSIONS)
    expected_local_max = tuple(d / 2.0 for d in DIMENSIONS)

    try:
        import ezdxf
        from ezdxf.acis import api as acis
        from ezdxf.render import MeshBuilder
    except ImportError as exc:
        print("This optional witness generator requires ezdxf with ACIS tools.", file=sys.stderr)
        print(
            "Install it in an isolated Python environment (for example: "
            'pip install "ezdxf>=1.4,<2").',
            file=sys.stderr,
        )
        print(f"Import failure: {exc}", file=sys.stderr)
        return 2

    repository_root = Path(__file__).resolve().parents[1]
    output_dir: Path | None = None
    created_files: list[Path] = []
    try:
        output_dir = create_output_directory(args.output_dir, repository_root)
        dxf_path = output_dir / "acis218_box_ac1027.dxf"
        sab_path = output_dir / "acis218_box.sab"
        manifest_path = output_dir / "acis218_box_ac1027.manifest.json"
        if dxf_path.exists() or sab_path.exists() or manifest_path.exists():
            raise ValueError("refusing to overwrite an existing witness output")
        created_files.extend((dxf_path, sab_path, manifest_path))

        drawing = ezdxf.new("R2013", setup=True)
        drawing.header["$INSUNITS"] = 4  # millimetres
        body = acis.body_from_mesh(build_box_mesh(MeshBuilder, center))
        solid = drawing.modelspace().add_3dsolid()
        acis.export_dxf(solid, [body])
        if not solid.has_binary_data:
            raise RuntimeError("ezdxf did not emit the expected binary SAB carrier")

        sab = bytes(solid.sab)
        if not sab.startswith(b"ACIS BinaryFile"):
            raise RuntimeError("generated modeler data lacks the SAB signature")
        sab_path.write_bytes(sab)
        drawing.saveas(dxf_path)

        # A same-library structural sanity check is intentionally not treated
        # as independent geometry evidence.
        readback = ezdxf.readfile(dxf_path)
        solids = list(readback.modelspace().query("3DSOLID"))
        if readback.dxfversion != "AC1027" or len(solids) != 1:
            raise RuntimeError("generated DXF failed its version/entity self-check")
        if not solids[0].has_binary_data or bytes(solids[0].sab) != sab:
            raise RuntimeError("generated SAB changed during DXF write/readback")

        manifest = {
            "evidence_scope": (
                "locally-authored ezdxf polyhedron control in ACIS model coordinates; "
                "not AutoCAD producer or drawing-unit evidence"
            ),
            "ezdxf_version": ezdxf.__version__,
            "dxf_version": readback.dxfversion,
            "entity": "one 3DSOLID",
            "sab_filename": sab_path.name,
            "shape": "closed axis-aligned rectangular polyhedron",
            "center_acis_model_coordinates": center,
            "dimensions_acis_model_units": DIMENSIONS,
            "expected_min_acis_model_coordinates": expected_min,
            "expected_max_acis_model_coordinates": expected_max,
            "expected_min_local": expected_local_min,
            "expected_max_local": expected_local_max,
            "expected_volume_acis_model_units3": math.prod(DIMENSIONS),
            "expected_surface_area_acis_model_units2": 2.0
            * (DIMENSIONS[0] * DIMENSIONS[1]
               + DIMENSIONS[0] * DIMENSIONS[2]
               + DIMENSIONS[1] * DIMENSIONS[2]),
            "dxf_insunits": "millimetres (header metadata; SAB conversion is unqualified)",
            "sab_bytes": len(sab),
            "sab_sha256": hashlib.sha256(sab).hexdigest(),
            "limitations": [
                "ezdxf does not provide an ACIS kernel",
                "this generator is limited to a flat-faced polyhedron",
                "the manifest geometry is derived from authored ACIS model-coordinate parameters",
                "the ACIS-to-drawing-unit scale is not established by $INSUNITS or this generator",
                "nonzero centers test the embedded ACIS transform, not AutoCAD placement semantics",
                "no DWG or ACDSDATA is generated",
                "same-library readback is only a structural self-check",
            ],
        }
        manifest_path.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    except Exception as exc:
        for path in created_files:
            path.unlink(missing_ok=True)
        if output_dir is not None and output_dir.exists():
            output_dir.rmdir()
        print(f"Failed to create the ACIS-218 witness: {exc}", file=sys.stderr)
        return 1

    print(f"DXF: {dxf_path}")
    print(f"SAB: {sab_path}")
    print(f"Manifest: {manifest_path}")
    print(f"SAB SHA-256: {manifest['sab_sha256']}")
    print(
        "Scope: local ACIS-coordinate polyhedron only; no AutoCAD, ACDSDATA, "
        "or drawing-unit qualification"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
