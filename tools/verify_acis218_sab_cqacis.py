#!/usr/bin/env python3
"""Check a locally generated ACIS-218 SAB with cq-acis/CadQuery.

This is an optional external-reader check for the output of
create_acis218_mesh_witness.py. cq-acis supplies an independent Rust ACIS
parser; its CadQuery adapter builds shapes with OCCT, the same kernel family
used by FreeCAD. The check therefore provides an independent parser and
placement/geometry readback, not an independent-kernel result or AutoCAD,
DWG, ACDSDATA, or general ACIS qualification. It writes only to stdout.
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.metadata
import json
import math
import sys
from pathlib import Path
from typing import Iterable


class VerificationError(ValueError):
    """A witness failed the independent-reader contract."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise VerificationError(message)


def read_vec3(value: object, label: str) -> tuple[float, float, float]:
    require(isinstance(value, list) and len(value) == 3,
            f"manifest {label} must be a three-value array")
    result = tuple(float(component) for component in value)
    require(all(math.isfinite(component) for component in result),
            f"manifest {label} contains a non-finite value")
    return result  # type: ignore[return-value]


def close_values(actual: Iterable[float], expected: Iterable[float]) -> bool:
    return all(
        math.isclose(a, e, rel_tol=1e-8, abs_tol=1e-7)
        for a, e in zip(actual, expected)
    )


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sab", type=Path, help="SAB emitted by the local witness generator")
    parser.add_argument("manifest", type=Path, help="matching JSON manifest")
    args = parser.parse_args()

    try:
        manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
        require(isinstance(manifest, dict), "manifest root must be a JSON object")
        sab = args.sab.read_bytes()
        digest = hashlib.sha256(sab).hexdigest()
        require(sab.startswith(b"ACIS BinaryFile"), "input lacks the SAB signature")
        require(digest == manifest.get("sab_sha256"),
                "SAB SHA-256 does not match the manifest")
        require(len(sab) == int(manifest.get("sab_bytes", -1)),
                "SAB length does not match the manifest")

        import cq_acis

        model = cq_acis.parse_sab_model(sab, source_id=args.sab.name)
        header_units_mm = model.metadata.units_mm
        effective_units_mm = 1.0 if header_units_mm is None else float(header_units_mm)
        require(math.isfinite(effective_units_mm) and effective_units_mm > 0.0,
                f"invalid ACIS header millimetre scale: {header_units_mm!r}")
        shapes = cq_acis.convert_model(model)
        require(len(shapes) == 1, f"expected one reconstructed shape, got {len(shapes)}")
        shape = shapes[0]
        solids = shape.Solids()
        shells = shape.Shells()
        require(shape.ShapeType() == "Solid", f"expected Solid, got {shape.ShapeType()}")
        require(shape.isValid(), "CadQuery/OCCT reports an invalid shape")
        require(len(solids) == 1, f"expected one solid, got {len(solids)}")
        require(len(shells) == 1 and shells[0].Closed(),
                "expected one closed boundary shell")
        require(len(shape.Faces()) == 6, "expected six planar box faces")
        require(len(shape.Edges()) == 12, "expected twelve box edges")

        bounds = shape.BoundingBox()
        actual_min = (bounds.xmin, bounds.ymin, bounds.zmin)
        actual_max = (bounds.xmax, bounds.ymax, bounds.zmax)
        expected_min = read_vec3(
            manifest.get("expected_min_acis_model_coordinates"),
            "expected_min_acis_model_coordinates",
        )
        expected_max = read_vec3(
            manifest.get("expected_max_acis_model_coordinates"),
            "expected_max_acis_model_coordinates",
        )
        expected_min_scaled = tuple(value * effective_units_mm for value in expected_min)
        expected_max_scaled = tuple(value * effective_units_mm for value in expected_max)
        require(close_values(actual_min, expected_min_scaled),
                "ACIS model-coordinate minimum differs: "
                f"actual={actual_min}, expected={expected_min_scaled} "
                f"(header scale={effective_units_mm} mm/source-unit)")
        require(close_values(actual_max, expected_max_scaled),
                "ACIS model-coordinate maximum differs: "
                f"actual={actual_max}, expected={expected_max_scaled} "
                f"(header scale={effective_units_mm} mm/source-unit)")

        expected_volume = (float(manifest["expected_volume_acis_model_units3"])
                           * effective_units_mm**3)
        expected_area = (float(manifest["expected_surface_area_acis_model_units2"])
                         * effective_units_mm**2)
        actual_volume = float(shape.Volume())
        actual_area = float(shape.Area())
        require(math.isclose(actual_volume, expected_volume,
                             rel_tol=1e-8, abs_tol=1e-7),
                f"volume differs: actual={actual_volume}, expected={expected_volume}")
        require(math.isclose(actual_area, expected_area,
                             rel_tol=1e-8, abs_tol=1e-7),
                f"area differs: actual={actual_area}, expected={expected_area}")

        result = {
            "result": "PASS",
            "reader": "cq-acis Rust SAB parser + CadQuery/OCCT geometry adapter",
            "cq_acis_version": importlib.metadata.version("cq-acis"),
            "cadquery_version": importlib.metadata.version("cadquery"),
            "occt_binding_version": __import__("OCP").__version__,
            "sab_sha256": digest,
            "sab_bytes": len(sab),
            "shape_type": shape.ShapeType(),
            "valid": bool(shape.isValid()),
            "solid_count": len(solids),
            "closed_shell_count": sum(1 for shell in shells if shell.Closed()),
            "faces": len(shape.Faces()),
            "edges": len(shape.Edges()),
            "acis_header_units_mm": header_units_mm,
            "effective_acis_header_scale_mm_per_source_unit": effective_units_mm,
            "bounds_after_acis_header_scale_mm": actual_min + actual_max,
            "volume_after_acis_header_scale_mm3": actual_volume,
            "surface_area_after_acis_header_scale_mm2": actual_area,
            "parser_diagnostics": [
                {"code": item.code, "message": item.message}
                for item in model.diagnostics
            ],
            "qualification_limit": (
                "ezdxf-authored ACIS-218 polyhedron and embedded transform; reports "
                "cq-acis header-scale interpretation, not AutoCAD/drawing-unit truth; "
                "OCCT is also used by FreeCAD; no DWG, ACDSDATA, or family-wide claim"
            ),
        }
        print(json.dumps(result, sort_keys=True))
        return 0
    except (OSError, KeyError, TypeError, ValueError, ImportError) as exc:
        print(f"ACIS witness verification failed: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
