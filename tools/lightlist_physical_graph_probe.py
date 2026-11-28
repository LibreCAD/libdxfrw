#!/usr/bin/env python3
"""Create the locally authored LIGHTLIST graph used for a scratch probe.

The generated DXF is not a fixture and must be placed outside the repository.
"""
import argparse
import hashlib
import json
from pathlib import Path

import ezdxf

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("output", type=Path)
args = parser.parse_args()
if ezdxf.__version__ != "1.4.5":
    parser.error("ezdxf 1.4.5 is required")
if args.output.exists():
    parser.error("output must not already exist")
args.output.parent.mkdir(parents=True, exist_ok=True)
document = ezdxf.new("R2013")
light = document.modelspace().new_entity(
    "LIGHT", dxfattribs={"name": "LOCAL_LIGHT", "type": 2,
                          "location": (1, 2, 3), "intensity": 2.5, "status": 1})
dictionary = document.rootdict.add_new_dict("ACAD_LIGHT")
document.header["$HANDSEED"] = "F100"
document.saveas(args.output)
lines = args.output.read_text().splitlines()
tags = [(int(lines[i].strip()), lines[i + 1]) for i in range(0, len(lines), 2)]
section = ""
for i, (code, value) in enumerate(tags):
    if code == 0 and value == "SECTION":
        section = tags[i + 1][1]
    if section != "OBJECTS" or code != 0 or value != "DICTIONARY":
        continue
    end = i + 1
    while end < len(tags) and tags[end][0] != 0:
        end += 1
    if (5, dictionary.dxf.handle) in tags[i:end]:
        tags[end:end] = [(3, "LIGHTLIST"), (350, "F000")]
        break
else:
    parser.error("authored dictionary was not found")
section = ""
for i, (code, value) in enumerate(tags):
    if code == 0 and value == "SECTION":
        section = tags[i + 1][1]
    if section == "OBJECTS" and code == 0 and value == "ENDSEC":
        tags[i:i] = [(0, "LIGHTLIST"), (5, "F000"), (102, "{ACAD_REACTORS"),
                     (330, dictionary.dxf.handle), (102, "}"),
                     (330, dictionary.dxf.handle), (100, "AcDbLightList"),
                     (90, "1"), (90, "1"), (5, light.dxf.handle), (1, "LOCAL_LIGHT")]
        break
else:
    parser.error("OBJECTS end was not found")
args.output.write_text("".join(f"{code:3d}\n{value}\n" for code, value in tags))
print(json.dumps({"sha256": hashlib.sha256(args.output.read_bytes()).hexdigest(),
                  "root": document.rootdict.dxf.handle,
                  "dictionary": dictionary.dxf.handle,
                  "light": light.dxf.handle, "list": "F000"}))
