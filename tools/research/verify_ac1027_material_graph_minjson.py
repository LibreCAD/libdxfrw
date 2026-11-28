#!/usr/bin/env python3
"""Check a selected AC1027 MATERIAL graph in LibreDWG minJSON output.

This is an independent-reader transport check for the exact default-material
profile. It is not native producer acceptance or a general MATERIAL schema.
"""

import argparse
import json
from pathlib import Path


EXPECTED_NAMES = ("ByBlock", "ByLayer", "Global")
IDENTITY = [
    1.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 1.0,
]


def require(condition, message):
    if not condition:
        raise ValueError(message)


def handle(value):
    require(isinstance(value, list) and len(value) >= 3,
            f"malformed handle tuple: {value!r}")
    return value[-1]


def verify(document):
    require(document.get("FILEHEADER", {}).get("version") == "AC1027",
            "input is not AC1027")
    classes = [c for c in document.get("CLASSES", [])
               if c.get("dxfname") == "MATERIAL"]
    require(len(classes) == 1, "expected one MATERIAL class definition")
    material_class = classes[0]
    require(material_class.get("cppname") == "AcDbMaterial"
            and material_class.get("appname") == "ObjectDBX Classes"
            and material_class.get("proxyflag") == 0x481
            and material_class.get("num_instances") == 3
            and material_class.get("is_zombie") == 0
            and material_class.get("item_class_id") == 0x1F3
            and material_class.get("dwg_version") == 23
            and material_class.get("maint_version") == 14,
            "MATERIAL CLASSES metadata differs from the qualified AC1027 profile")
    objects = document.get("OBJECTS")
    require(isinstance(objects, list), "missing OBJECTS array")

    dictionaries = [o for o in objects if o.get("object") == "DICTIONARY"]
    roots = [o for o in dictionaries
             if "ACAD_MATERIAL" in o.get("items", {})]
    require(len(roots) == 1, "expected one NOD ACAD_MATERIAL entry")
    root = roots[0]
    root_handle = handle(root["handle"])
    material_dictionary_handle = handle(root["items"]["ACAD_MATERIAL"])
    material_dictionaries = [
        o for o in dictionaries
        if handle(o.get("handle", [])) == material_dictionary_handle
    ]
    require(len(material_dictionaries) == 1,
            "ACAD_MATERIAL target dictionary is absent or duplicated")
    material_dictionary = material_dictionaries[0]
    require(handle(material_dictionary["ownerhandle"]) == root_handle,
            "ACAD_MATERIAL dictionary owner does not match NOD")
    entries = material_dictionary.get("items", {})
    require(set(entries) == set(EXPECTED_NAMES),
            "ACAD_MATERIAL dictionary keys do not match the exact profile")

    materials = [o for o in objects if o.get("object") == "MATERIAL"]
    require(len(materials) == len(EXPECTED_NAMES),
            "unexpected MATERIAL record count")
    by_name = {o.get("name"): o for o in materials}
    require(set(by_name) == set(EXPECTED_NAMES),
            "MATERIAL names do not match the dictionary profile")

    for name in EXPECTED_NAMES:
        material = by_name[name]
        material_handle = handle(material["handle"])
        require(handle(entries[name]) == material_handle,
                f"dictionary key {name} targets the wrong MATERIAL")
        require(handle(material["ownerhandle"]) == material_dictionary_handle,
                f"{name} has the wrong owner")
        reactors = material.get("reactors", [])
        require(len(reactors) == 1 and handle(reactors[0])
                == material_dictionary_handle,
                f"{name} has the wrong reactor list")
        require(material.get("_subclass") == "AcDbMaterial",
                f"{name} has the wrong subclass")
        require(material.get("description") == "",
                f"{name} has an unexpected description")
        for color in ("ambient_color", "diffuse_color", "specular_color"):
            require(material.get(f"{color}.flag") == 0
                    and material.get(f"{color}.factor") == 1.0,
                    f"{name} has unexpected {color} fields")
        for map_name in ("diffusemap", "specularmap", "reflectionmap",
                         "opacitymap", "bumpmap", "refractionmap"):
            prefix = f"{map_name}."
            require(material.get(prefix + "blendfactor") == 1.0
                    and material.get(prefix + "projection") == 1
                    and material.get(prefix + "tiling") == 1
                    and material.get(prefix + "autotransform") == 1
                    and material.get(prefix + "transmatrix") == IDENTITY,
                    f"{name} has unexpected {map_name} settings")
            expected_source = 0 if map_name == "refractionmap" else 1
            require(material.get(prefix + "source") == expected_source,
                    f"{name} has unexpected {map_name} source")
            if expected_source:
                require(material.get(prefix + "filename") == "",
                        f"{name} has unexpected {map_name} filename")
        require(material.get("specular_gloss_factor") == 0.5
                and material.get("opacity_percent") == 1.0
                and material.get("refraction_index") == 1.0
                and material.get("translucence") == 0.0
                and material.get("self_illumination") == 0.0
                and material.get("reflectivity") == 0.0
                and material.get("illumination_model") == 0
                and material.get("channel_flags") == 127
                and material.get("mode") == 0,
                f"{name} has unexpected scalar/channel fields")
    return {
        "version": "AC1027",
        "nodHandle": root_handle,
        "materialDictionaryHandle": material_dictionary_handle,
        "names": list(EXPECTED_NAMES),
        "handles": {name: handle(by_name[name]["handle"])
                    for name in EXPECTED_NAMES},
        "result": "PASS",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("minjson", type=Path)
    args = parser.parse_args()
    try:
        result = verify(json.loads(args.minjson.read_text(encoding="utf-8")))
    except (OSError, json.JSONDecodeError, KeyError, TypeError, ValueError) as exc:
        parser.error(str(exc))
    print(json.dumps(result, sort_keys=True))


if __name__ == "__main__":
    main()
