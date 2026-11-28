"""Check one source-bound, meter-unit MINSERT profile inside FreeCADCmd.

Set FREECAD_MINSERT_DXF to a selected DXF and FREECAD_MINSERT_SHA256 to its
expected SHA-256. This checks only six line endpoints in two importer modes; it
is not a whole-file, GUI, producer, or DWG-semantics qualification. The caller
must check the candidate executable/module identities and require an `ok` JSON
result, rather than relying on FreeCADCmd's process exit status.
"""
import hashlib
import json
import os
from pathlib import Path
import platform
import traceback

import FreeCAD as App
import Import

EXPECTED = sorted(((-10000, 30000, z), (-10000, 30000, z + 2000))
                  for z in (20000, 25000, 30000)) + sorted(
                     ((-3000, 30000, z), (-3000, 30000, z + 2000))
                     for z in (20000, 25000, 30000))


def main():
    report = {"ok": False, "results": [], "scope": "selected headless array geometry only"}
    try:
        path = Path(os.environ["FREECAD_MINSERT_DXF"]).resolve(strict=True)
        expected_hash = os.environ["FREECAD_MINSERT_SHA256"]
        actual_hash = hashlib.sha256(path.read_bytes()).hexdigest()
        if actual_hash != expected_hash:
            raise ValueError("DXF input SHA-256 does not match the requested source")
        if App.GuiUp:
            raise ValueError("This probe requires headless FreeCADCmd")
        report.update({"input": str(path), "inputSha256": actual_hash,
                       "importModule": getattr(Import, "__file__", None),
                       "freecadVersion": list(App.Version()), "platform": platform.platform(),
                       "machine": platform.machine()})
        for mode in (2, 3):
            document = App.newDocument("MInsertProbe")
            try:
                key = "User parameter:BaseApp/Preferences/Mod/Draft/MInsertProbe"
                options = App.ParamGet(key)
                options.SetInt("DxfImportMode", mode)
                options.SetBool("dxfUseDraftVisGroups", False)
                options.SetBool("dxftext", False)
                stats = Import.readDXF(str(path), document.Name, True, key)
                document.recompute()
                links = [obj for obj in document.Objects if obj.Name.startswith("Link_B")]
                edges = [edge for obj in links for edge in obj.Shape.Edges]
                actual = sorted(tuple(sorted((round(v.Point.x, 6), round(v.Point.y, 6),
                                              round(v.Point.z, 6)) for v in edge.Vertexes))
                                for edge in edges)
                report["results"].append({"mode": mode, "ok": actual == EXPECTED,
                                          "edges": len(edges), "endpoints": actual,
                                          "unsupported": {key: len(value) for key, value in
                                                          stats["unsupportedFeatures"].items()}})
            finally:
                App.closeDocument(document.Name)
        report["ok"] = len(report["results"]) == 2 and all(x["ok"] for x in report["results"])
    except Exception:
        report["error"] = traceback.format_exc()
    print("FREECAD_MINSERT_HANDOFF=" + json.dumps(report, sort_keys=True), flush=True)


if __name__ in ("__main__", Path(__file__).stem):
    main()
