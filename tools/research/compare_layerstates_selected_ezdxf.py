#!/usr/bin/env python3
"""Compare selected LayerStates DXFs with a LibreDWG JSON observation.

The drawing and generated files remain external to the repository. This is
independent transport readback, not native rendering or producer evidence.
"""

import argparse
import hashlib
import json
from pathlib import Path

import ezdxf


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("source_json", type=Path)
    parser.add_argument("ascii_dxf", type=Path)
    parser.add_argument("binary_dxf", type=Path)
    parser.add_argument("--material", action="store_true", help="also verify selected Global MATERIAL and FBXASSET")
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
    for path in (args.ascii_dxf, args.binary_dxf):
        document = ezdxf.readfile(path)
        root = document.entitydb["2"].get_extension_dict().dictionary
        original_root = objects[0xD5]
        assert root.dxf.owner == "2" and root.dxf.hard_owned == original_root["is_hardowner"]
        assert set(root.keys()) == set(original_root["items"])
        assert not list(root["ACAD_LAYERFILTERS"].keys())
        companion = root["ACLYDICTIONARY"]
        assert [(tag.code, tag.value) for tag in companion.get_xdata("ACAD")] == [
            (1000, objects[0xED]["eed"][0]["value"])
        ]
        states = root["ACAD_LAYERSTATES"]
        assert set(states.keys()) == {"ACAD_VIEWS_RenderView"}
        primary = states["ACAD_VIEWS_RenderView"]
        assert primary.dxftype() == "XRECORD" and primary.dxf.owner == states.dxf.handle
        expected = []
        for code, value in objects[0x249]["xdata"]:
            if code == 330:
                value = document.layers.get("0").dxf.handle
            elif code == 331:
                value = document.linetypes.get("Continuous").dxf.handle
            expected.append((code, value))
        assert [(tag.code, tag.value) for tag in primary.tags] == expected
        nested = primary.get_extension_dict().dictionary
        assert nested.dxf.owner == primary.dxf.handle
        assert set(nested.keys()) == {"AcadLayerStateAnnoScale"}
        record = nested["AcadLayerStateAnnoScale"]
        scale_list = document.rootdict["ACAD_SCALELIST"]
        assert len(list(scale_list.keys())) == 32
        source_scale = objects[0x127]
        scale = document.entitydb["A127"]
        assert scale.dxftype() == "SCALE" and scale.dxf.owner == scale_list.dxf.handle
        assert [(tag.code, tag.value) for tag in record.tags] == [(330, scale.dxf.handle)]
        fields = {tag.code: tag.value for group in scale.xtags.subclasses for tag in group}
        assert fields[300] == source_scale["name"]
        assert fields[140] == source_scale["paper_units"] and fields[141] == source_scale["drawing_units"]
        assert fields[290] == source_scale["is_unit_scale"]
        assert "ACAD" in document.appids
        if args.material:
            material_root = document.rootdict["ACAD_MATERIAL"]
            # ezdxf may synthesize missing default ByLayer/ByBlock materials
            # during load, so its in-memory dictionary is not a wire census.
            assert "Global" in material_root
            material = material_root["Global"]
            assert material.dxftype() == "MATERIAL" and material.dxf.owner == material_root.dxf.handle
            source_eed = objects[0x98]["eed"]
            expected_eed = [
                (1070, source_eed[0]["value"]), (1070, source_eed[1]["value"]),
                (1070, source_eed[2]["value"]), (1000, source_eed[3]["value"]),
                (1071, source_eed[4]["value"]), (1070, source_eed[5]["value"]),
            ]
            assert [(tag.code, tag.value) for tag in material.get_xdata("ACAD")] == expected_eed
            material_extension = material.get_extension_dict().dictionary
            assert material_extension.dxf.owner == material.dxf.handle
            assert set(material_extension.keys()) == {"FBXASSET"}
            fbx = material_extension["FBXASSET"]
            assert fbx.dxftype() == "XRECORD" and fbx.dxf.owner == material_extension.dxf.handle
            expected_fbx = [(code, bytes.fromhex(value) if code == 310 else value)
                            for code, value in objects[0x291]["xdata"]]
            assert [(tag.code, tag.value) for tag in fbx.tags] == expected_fbx
        result["outputs"].append({
            "path": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "root_entries": 3, "scales": 32, "signed_group_370": -3, "result": "pass",
            "material_eed_and_fbxasset": bool(args.material),
        })
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
