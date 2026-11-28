#!/usr/bin/env python3
"""Optional LibreDWG readback of locally generated opt-in plotstyle graphs.

Run libdxfrw_dwg_default_plotstyle_optin_tests --keep, then pass its OUTPUT
folder and the path to a dwgread binary. No drawings are stored in the repo.
"""
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import tempfile


def main(folder, dwgread):
    drawings = sorted(folder.glob("*.dwg"))
    if len(drawings) != 6:
        raise ValueError("expected six generated DWGs")
    with tempfile.TemporaryDirectory(prefix="libdxfrw-plotstyle-readback-") as tmp:
        for drawing in drawings:
            result = Path(tmp) / (drawing.stem + ".json")
            subprocess.run([str(dwgread), "-O", "JSON", "-o", str(result), str(drawing)],
                           check=True, capture_output=True)
            document = json.loads(result.read_text())
            objects = document["OBJECTS"]
            header = document["HEADER"]["DICTIONARY_PLOTSTYLENAME"]
            nod = next(o for o in objects if o.get("object") == "DICTIONARY"
                       and o.get("handle", [0])[-1] == 12)
            root = nod["items"]["ACAD_PLOTSTYLENAME"]
            dictionary_id = root[-1]
            dictionary = next(o for o in objects if o.get("object") == "DICTIONARYWDFLT"
                              and o.get("handle", [0])[-1] == dictionary_id)
            placeholder_id = dictionary["defaultid"][-1]
            placeholder = next(o for o in objects if o.get("object") == "PLACEHOLDER"
                               and o.get("handle", [0])[-1] == placeholder_id)
            layer = next(o for o in objects if o.get("object") == "LAYER" and o.get("name") == "0")
            klass = next(c for c in document["CLASSES"] if c.get("dxfname") == "ACDBDICTIONARYWDFLT")
            if not (header[0] == 5 and header[-1] == dictionary_id and root[0] == 2
                    and dictionary["ownerhandle"][0] == 4 and dictionary["ownerhandle"][-1] == 12
                    and dictionary["items"]["Normal"][0] == 2
                    and dictionary["items"]["Normal"][-1] == placeholder_id
                    and dictionary["defaultid"][0] == 5
                    and placeholder["ownerhandle"][0] == 4
                    and placeholder["ownerhandle"][-1] == dictionary_id
                    and layer["plotstyle"][0] == 5 and layer["plotstyle"][-1] == placeholder_id
                    and sum(o.get("entity") == "LINE" for o in objects) == 1):
                raise ValueError(f"incomplete graph in {drawing}")
            version = drawing.read_bytes()[:6].decode("ascii")
            if version != "AC1015" and klass["num_instances"] != 1:
                raise ValueError(f"incorrect class count in {drawing}")
            print(version, "complete bounded graph PASS", hashlib.sha256(drawing.read_bytes()).hexdigest())


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: dwg_default_plotstyle_libredwg.py OUTPUT_DIR DWGREAD")
    main(Path(sys.argv[1]), Path(sys.argv[2]))
