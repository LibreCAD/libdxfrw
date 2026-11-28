#!/usr/bin/env python3
"""Inspect the bounded AC1015 LINE/CIRCLE/ARC chain with GNU LibreDWG."""
import json
import subprocess
import sys
import tempfile
from pathlib import Path


def main():
    if len(sys.argv) not in (3, 4):
        raise SystemExit("usage: dwg_legacy_model_chain_libredwg.py <dwgread> <generated.dwg> [nonmonotonic|singleton|group]")
    if len(sys.argv) == 4 and sys.argv[3] not in ("nonmonotonic", "singleton", "group"):
        raise SystemExit("unknown chain order")
    with tempfile.TemporaryDirectory() as tmp:
        target = Path(tmp) / "out.json"
        subprocess.run([sys.argv[1], "-O", "JSON", "-o", str(target), sys.argv[2]], check=True)
        document = json.loads(target.read_text())
    objects = document["OBJECTS"]
    by_handle = {o["handle"][2]: o for o in objects if o.get("handle")}
    mode = sys.argv[3] if len(sys.argv) == 4 else "default"
    handles = ([0x451, 0x130, 0x999] if mode == "nonmonotonic" else
               [0x130] if mode == "singleton" else [0x130, 0x451, 0x999])
    names = ["LINE"] if mode == "singleton" else ["LINE", "CIRCLE", "ARC"]
    blocks = [o for o in objects if o.get("object") == "BLOCK_HEADER" and o.get("name") == "*Model_Space"]
    assert len(blocks) == 1
    assert blocks[0]["first_entity"][:3] == [4, 2, handles[0]]
    assert blocks[0]["last_entity"][:3] == [4, 2, handles[-1]]
    for i, handle in enumerate(handles):
        obj = by_handle[handle]
        assert obj["entity"] == names[i] and obj["entmode"] == 2 and obj["nolinks"] == 0
        prev = handles[i - 1] if i else 0
        nxt = handles[i + 1] if i + 1 < len(handles) else 0
        assert obj["prev_entity"][0] == 4 and obj["prev_entity"][3] == prev
        assert obj["next_entity"][0] == 4 and obj["next_entity"][3] == nxt
    assert by_handle[handles[0]]["start"] == [1.0, 2.0, 3.0]
    assert by_handle[handles[0]]["end"] == [4.0, 5.0, 6.0]
    if mode != "singleton":
        assert by_handle[handles[1]]["center"] == [7.0, 8.0, 0.0]
        assert by_handle[handles[1]]["radius"] == 2.5
        assert by_handle[handles[2]]["center"] == [10.0, 11.0, 0.0]
        assert by_handle[handles[2]]["radius"] == 4.0
    if mode == "group":
        groups = [o for o in objects if o.get("object") == "GROUP" and o.get("handle", [0, 0, 0])[2] == 0xB300]
        assert len(groups) == 1 and groups[0]["ownerhandle"][3] == 0xD
        assert len(groups[0]["groups"]) == 1 and groups[0]["groups"][0][0] == 5
        assert groups[0]["groups"][0][3] == 0x451
    print("LibreDWG AC1015 bounded model-space chain PASS")


if __name__ == "__main__":
    main()
