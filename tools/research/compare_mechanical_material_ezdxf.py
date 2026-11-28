#!/usr/bin/env python3
"""Compare a LibreDWG JSON observation with ezdxf readback of two DXFs.

This is a bounded independent-reader check, not a producer or native CAD
qualification. The input drawings and decoded JSON stay outside the repository.
"""

import argparse
import hashlib
import json
import math
from pathlib import Path

import ezdxf


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source_json", type=Path)
    parser.add_argument("ascii_dxf", type=Path)
    parser.add_argument("binary_dxf", type=Path)
    args = parser.parse_args()
    with args.source_json.open(encoding="utf-8") as stream:
        source = json.load(stream)
    objects = {}
    for obj in source["OBJECTS"]:
        if not isinstance(obj.get("handle"), list):
            continue
        handle = obj["handle"][-1]
        if handle in objects:
            raise ValueError(f"duplicate source handle: {handle:x}")
        objects[handle] = obj
    result = {"reader": f"ezdxf {ezdxf.__version__}", "outputs": []}
    names = {"BUMPTILE", "DIFFUSETILE", "OPACITYTILE", "REFLECTIONTILE"}
    matrices = (
        ("diffuse_mapper_matrix", "diffusemap.transmatrix"),
        ("specular_mapper_matrix", "specularmap.transmatrix"),
        ("reflexion_mapper_matrix", "reflectionmap.transmatrix"),
        ("opacity_mapper_matrix", "opacitymap.transmatrix"),
        ("bump_mapper_matrix", "bumpmap.transmatrix"),
        ("refraction_mapper_matrix", "refractionmap.transmatrix"),
    )
    for path in (args.ascii_dxf, args.binary_dxf):
        document = ezdxf.readfile(path)
        materials = document.rootdict["ACAD_MATERIAL"]
        assert set(materials.keys()) == {"ByLayer", "ByBlock", "Global"}
        for name, handle in (("ByLayer", 0x96), ("ByBlock", 0x97), ("Global", 0x98)):
            material = materials[name]
            original = objects[handle]
            assert material.dxftype() == "MATERIAL" and material.dxf.owner == materials.dxf.handle
            for kind in ("diffuse", "specular", "opacity", "bump", "refraction"):
                assert getattr(material.dxf, kind + "_map_projection_method") == original[kind + "map.projection"]
            assert material.dxf.reflection_map_projection_method == original["reflectionmap.projection"]
            for attribute, key in matrices:
                actual = list(getattr(material, attribute))
                expected = original[key]
                assert len(actual) == len(expected) == 16
                assert all(math.isclose(a, b, rel_tol=0, abs_tol=1e-13) for a, b in zip(actual, expected))
        global_material = materials["Global"]
        source_eed = objects[0x98]["eed"]
        first = source_eed[0]["value"]
        signed_first = first - 65536 if first >= 32768 else first
        assert [(tag.code, tag.value) for tag in global_material.get_xdata("ACAD")] == [
            (1070, signed_first), (1070, source_eed[1]["value"]),
            (1070, source_eed[2]["value"]), (1000, source_eed[3]["value"]),
            (1071, source_eed[4]["value"]), (1070, source_eed[5]["value"]),
        ]
        extension = global_material.get_extension_dict()
        assert extension.dictionary.dxf.owner == global_material.dxf.handle
        assert set(extension.keys()) == names
        for name in names:
            record = extension[name]
            assert record.dxftype() == "XRECORD" and record.dxf.owner == extension.dictionary.dxf.handle
            assert [(tag.code, tag.value) for tag in record.tags] == [(270, 1), (271, 1)]
        assert "ACAD" in document.appids
        result["outputs"].append({
            "path": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "materials": 3, "matrices_checked": 18, "tile_records": 4,
            "signed_eed": signed_first, "result": "pass",
        })
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
