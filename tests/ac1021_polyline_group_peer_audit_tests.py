#!/usr/bin/env python3
"""Fixture-free positive and negative controls for the independent peer audit."""
import copy
import importlib.util
from pathlib import Path

spec = importlib.util.spec_from_file_location("group_peer", Path(__file__).resolve().parents[1] / "tools/audit_ac1021_polyline_group_peer.py")
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)
profile = audit.PROFILES["generated2d"]

def tags(entries):
    return "".join(f"{c}\n{v}\n" for c, v in entries)

sample = tags([
    (0, "POLYLINE"), (5, "451"), (70, 0),
    (0, "VERTEX"), (5, "501"), (102, "{ACAD_REACTORS"), (330, "900"), (102, "}"), (330, "451"), (10, 1), (20, 2), (30, 0),
    (0, "VERTEX"), (5, "502"), (330, "451"), (10, 3), (20, 4), (30, 0),
    (0, "GROUP"), (5, "900"), (102, "{ACAD_REACTORS"), (330, "D"), (102, "}"), (330, "D"), (340, "501"),
    (0, "EOF"),
])
peer = {"OBJECTS": [
    {"object": "GROUP", "handle": [0, 2, 0x900], "groups": [[5, 2, 0x501, 0x501]], "ownerhandle": [4, 1, 13, 13], "reactors": [[4, 1, 13, 13]], "is_xdic_missing": 1},
    {"entity": "POLYLINE_2D", "handle": [0, 2, 0x451], "vertex": [[3, 2, 0x501, 0x501], [3, 2, 0x502, 0x502]], "seqend": [3, 2, 0x503, 0x503]},
    {"entity": "VERTEX_2D", "handle": [0, 2, 0x501], "point": [1, 2, 0], "ownerhandle": [4, 2, 0x451, 0x451], "reactors": [[4, 2, 0x900, 0x900]]},
    {"entity": "VERTEX_2D", "handle": [0, 2, 0x502], "point": [3, 4, 0], "ownerhandle": [4, 2, 0x451, 0x451]},
    {"entity": "SEQEND", "handle": [0, 2, 0x503], "ownerhandle": [4, 2, 0x451, 0x451]},
    {"object": "BLOCK_HEADER", "name": "*Model_Space", "entities": [[3, 2, 0x451, 0x451]]},
]}
audit.validate_dxf(sample, profile)
audit.validate_peer(peer, profile)
for mutated in (sample.replace("340\n501\n", "340\n502\n"),
                sample.replace("330\n900\n", "330\n901\n", 1),
                sample.replace("330\n451\n", "330\n452\n", 1),
                sample.replace("20\n4\n", "20\n8\n", 1)):
    try:
        audit.validate_dxf(mutated, profile)
    except ValueError:
        pass
    else:
        raise SystemExit("DXF corruption passed")
for index, field, value in [(0, "groups", [[5, 2, 0x502, 0x502]]),
                            (1, "vertex", [[5, 2, 0x501, 0x501], [3, 2, 0x502, 0x502]]),
                            (2, "reactors", []),
                            (5, "entities", [[3, 2, 0x451, 0x451], [3, 2, 0x501, 0x501]])]:
    changed = copy.deepcopy(peer)
    changed["OBJECTS"][index][field] = value
    try:
        audit.validate_peer(changed, profile)
    except ValueError:
        pass
    else:
        raise SystemExit("peer corruption passed")
collision = copy.deepcopy(peer)
collision["OBJECTS"].append({"object": "XRECORD", "handle": [0, 2, 0x501]})
try:
    audit.validate_peer(collision, profile)
except ValueError:
    pass
else:
    raise SystemExit("global identity collision passed")
print("AC1021 GROUP peer audit positive and negative controls PASS")
