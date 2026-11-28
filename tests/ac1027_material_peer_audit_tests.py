#!/usr/bin/env python3
"""Fixture-free controls for the independent AC1027 material audit."""
import copy
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from audit_ac1027_material_peer import NAMES, verify_pair


def sample(generated):
    root = 12 if generated else 10
    dictionary = 0xB203 if generated else 14
    children = [0xB204, 0xB205, 0xB206] if generated else [31, 32, 33]
    width = 2 if generated else 1
    def ref(code, value, size=width):
        return [code, size, value]
    objects = [
        {"object": "DICTIONARY", "handle": ref(0, root),
         "items": {"ACAD_MATERIAL": ref(2, dictionary)}},
        {"object": "DICTIONARY", "handle": ref(0, dictionary),
         "ownerhandle": ref(4, root),
         "items": {name: ref(2, handle) for name, handle in zip(NAMES, children)}},
    ]
    trace = {}
    for name, handle in zip(NAMES, children):
        boundary = (727 if name == "Global" else 743) + (8 if generated else 0)
        string_bits = 120 if name == "Global" else 136
        size = (98 if generated else 95) if name == "Global" else (100 if generated else 97)
        objects.append({"object": "MATERIAL", "name": name, "handle": ref(0, handle),
                        "type": 505 if generated else 503,
                        "ownerhandle": ref(4, dictionary), "reactors": [ref(4, dictionary)],
                        "size": size, "diffuse_color.rgb": 0xc2ffffff})
        trace[name] = (boundary, string_bits, 38 if generated else 30)
    objects.append({"object": "LAYER", "name": "0", "handle": ref(0, 100),
                    "material": ref(5, children[2])})
    return {"OBJECTS": objects, "FILEHEADER": {"version": "AC1027"},
            "CLASSES": [{"dxfname": "MATERIAL", "cppname": "AcDbMaterial",
                         "appname": "ObjectDBX Classes", "proxyflag": 0x481,
                         "num_instances": 3, "is_zombie": 0, "item_class_id": 499,
                         "number": 505 if generated else 503}],
            "HEADER": {"DICTIONARY_MATERIAL": ref(5, dictionary),
                       "CMATERIAL": ref(5, children[1])}}, trace


class MaterialAuditTests(unittest.TestCase):
    def test_positive_and_distinct_negative_controls(self):
        left, left_trace = sample(False)
        right, right_trace = sample(True)
        self.assertEqual(3, len(verify_pair(left, left_trace, right, right_trace)))
        for fault in ("boundary", "string", "common", "owner-role", "reactor-target",
                      "root-edge-role", "current-material", "frame-size", "semantic", "duplicate",
                      "class-count", "layer-material", "revision"):
            with self.subTest(fault=fault):
                mutated, trace = copy.deepcopy(right), copy.deepcopy(right_trace)
                material = mutated["OBJECTS"][2]
                if fault == "boundary": trace["ByBlock"] = (752, 136, 38)
                if fault == "string": trace["ByBlock"] = (751, 135, 38)
                if fault == "common": trace["ByBlock"] = (751, 136, 37)
                if fault == "owner-role": material["ownerhandle"][0] = 3
                if fault == "reactor-target": material["reactors"][0][2] += 1
                if fault == "root-edge-role": mutated["OBJECTS"][0]["items"]["ACAD_MATERIAL"][0] = 3
                if fault == "current-material": mutated["HEADER"]["CMATERIAL"][2] += 1
                if fault == "frame-size": material["size"] += 1
                if fault == "semantic": material["diffuse_color.rgb"] = 0
                if fault == "duplicate": mutated["OBJECTS"].append(copy.deepcopy(material))
                if fault == "class-count": mutated["CLASSES"][0]["num_instances"] = 2
                if fault == "layer-material": mutated["OBJECTS"][-1]["material"][2] += 1
                if fault == "revision": mutated["FILEHEADER"]["version"] = "AC1024"
                with self.assertRaises(ValueError):
                    verify_pair(left, left_trace, mutated, trace)


if __name__ == "__main__":
    unittest.main()
