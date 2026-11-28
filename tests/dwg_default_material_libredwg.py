#!/usr/bin/env python3
"""Independently inspect an opt-in graph test DWG with GNU LibreDWG 0.14."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path


def ref(value, code):
    assert isinstance(value, list) and len(value) >= 3 and value[0] == code, value
    return value[2]


def main():
    if len(sys.argv) not in (3, 4):
        raise SystemExit("usage: dwg_default_material_libredwg.py <dwgread> <generated.dwg> [ByLayer|Global]")
    current_name = sys.argv[3] if len(sys.argv) == 4 else "ByLayer"
    assert current_name in ("ByLayer", "Global")
    with tempfile.TemporaryDirectory() as tmp:
        target = Path(tmp) / "decoded.json"
        subprocess.run([sys.argv[1], "-O", "JSON", "-o", str(target), sys.argv[2]], check=True)
        document = json.loads(target.read_text())
    objects = document["OBJECTS"]
    by_handle = {item["handle"][2]: item for item in objects if "handle" in item}
    roots = [item for item in objects if item.get("object") == "DICTIONARY" and
             "ACAD_MATERIAL" in item.get("items", {})]
    assert len(roots) == 1
    nod = roots[0]
    dictionary_handle = ref(nod["items"]["ACAD_MATERIAL"], 2)
    dictionary = by_handle[dictionary_handle]
    assert ref(document["HEADER"]["DICTIONARY_MATERIAL"], 5) == dictionary_handle
    assert dictionary["object"] == "DICTIONARY"
    assert ref(dictionary["ownerhandle"], 4) == nod["handle"][2]
    entries = dictionary["items"]
    assert set(entries) == {"ByBlock", "ByLayer", "Global"}
    for name in ("ByBlock", "ByLayer", "Global"):
        obj = by_handle[ref(entries[name], 2)]
        assert obj["object"] == "MATERIAL" and obj["name"] == name
        assert ref(obj["ownerhandle"], 4) == dictionary_handle
        assert len(obj["reactors"]) == 1 and ref(obj["reactors"][0], 4) == dictionary_handle
        assert obj["diffuse_color.rgb"] == 0xc2ffffff and obj["channel_flags"] == 63
        assert obj["specular_gloss_factor"] == 0.5 and obj["mode"] == 0
        for field in ("diffusemap", "specularmap", "reflectionmap", "opacitymap", "bumpmap", "refractionmap"):
            assert obj[field + ".source"] == 1 and obj[field + ".filename"] == ""
            assert obj[field + ".transmatrix"] == [1.0 if i % 5 == 0 else 0.0 for i in range(16)]
    assert ref(document["HEADER"]["CMATERIAL"], 5) == ref(entries[current_name], 2)
    layers = [item for item in objects if item.get("object") == "LAYER" and item.get("name") == "0"]
    assert len(layers) == 1 and ref(layers[0]["material"], 5) == ref(entries["Global"], 2)
    classes = [item for item in document["CLASSES"] if item.get("dxfname") == "MATERIAL"]
    assert len(classes) == 1 and classes[0]["cppname"] == "AcDbMaterial"
    assert classes[0]["proxyflag"] == 0x481 and classes[0]["num_instances"] == 3
    print("LibreDWG default material graph PASS")


if __name__ == "__main__":
    main()
