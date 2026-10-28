#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Generate independent MATERIAL controls with ezdxf 1.4.5 and run body probes.

Run with an isolated Python containing ezdxf==1.4.5. The probe is the built
libdxfrw_dxf_fixture_tests executable. This verifies selected imported body
values, not whole-document export, native rendering, or arbitrary source order.
All drawings are created from scratch in the requested empty output directory.
"""
import argparse
import hashlib
import importlib.metadata
import inspect
import json
from pathlib import Path
import subprocess
import sys


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--probe", required=True, type=Path)
    args = parser.parse_args()
    import ezdxf
    from ezdxf.math import Matrix44

    if ezdxf.__version__ != "1.4.5":
        parser.error("this source profile requires pinned ezdxf 1.4.5")
    probe = args.probe.resolve(strict=True)
    output = args.output_dir.resolve()
    if output.exists() and any(output.iterdir()):
        parser.error("output directory must be empty")
    output.mkdir(parents=True, exist_ok=True)
    rows = []
    source = None
    for version in ("R2007", "R2010", "R2013", "R2018"):
        doc = ezdxf.new(version)
        material = doc.materials.new("PeerSurface")
        material.dxf.update({
            "description": "independent local recipe",
            "bump_map_projection_method": 2, "bump_map_tiling_method": 3,
            "normal_map_method": 7, "normal_map_strength": 0.5,
            "normal_map_file_name": "peer-normal.png", "normal_map_source": 0,
            "normal_map_projection_method": 4, "normal_map_tiling_method": 2,
            "normal_map_auto_transform_method": 4, "color_bleed_scale": 0.6,
            "indirect_dump_scale": 0.7, "reflectance_scale": 0.8,
            "transmittance_scale": 0.9, "two_sided_material": 1,
        })
        material.diffuse_mapper_matrix = Matrix44([100.0 + i for i in range(16)])
        material.bump_mapper_matrix = Matrix44([900.0 + i for i in range(16)])
        material.refraction_mapper_matrix = Matrix44([1100.0 + i for i in range(16)])
        material.normal_mapper_matrix = Matrix44([200.0 + i for i in range(16)])
        # This peer emits its specular color method at reused code73. Select1
        # to isolate the normal-header gap without an invalid code73=0. This
        # harness does not qualify specular-color or complete peer semantics.
        for _, existing in doc.materials:
            existing.dxf.specular_color_method = 1
        doc.modelspace().add_line((1, 2, 3), (4, 5, 6), dxfattribs={
            "material_handle": material.dxf.handle,
        })
        source = Path(inspect.getfile(type(material)))
        for fmt in ("asc", "bin"):
            path = output / ("peer-" + doc.dxfversion + "-" + fmt + ".dxf")
            doc.saveas(path, fmt=fmt)
            run = subprocess.run([str(probe), "--material-source-order-probe", str(path)],
                                 capture_output=True, text=True)
            rows.append({"version": doc.dxfversion, "format": fmt,
                         "path": path.name, "sha256": digest(path),
                         "exitCode": run.returncode, "stdout": run.stdout,
                         "stderr": run.stderr})
    passed = sum(row["exitCode"] == 0 for row in rows)
    report = {
        "schema": "libdxfrw.material-peer-source-order.v1",
        "scope": "Selected imported body values only; not full/native support",
        "python": sys.version, "producer": "ezdxf", "version": ezdxf.__version__,
        "packages": {name: importlib.metadata.version(name) for name in (
            "ezdxf", "numpy", "fonttools", "pyparsing", "typing_extensions")},
        "producerMaterialSourceSha256": digest(source),
        "probeSha256": digest(probe), "generated": len(rows), "passed": passed,
        "results": rows, "wholeDocumentExportQualified": False,
        "nativeRenderingQualified": False, "fullObjectiveComplete": False,
    }
    (output / "qualification.json").write_text(json.dumps(report, indent=2) + "\n")
    print("Independent MATERIAL selected body matches: %d/%d" % (passed, len(rows)))
    return 0 if passed == len(rows) else 1


if __name__ == "__main__":
    sys.exit(main())
