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
        result["outputs"].append({
            "path": str(path), "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
            "root_entries": 3, "scales": 32, "signed_group_370": -3, "result": "pass",
        })
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
