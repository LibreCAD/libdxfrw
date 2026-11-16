#!/usr/bin/env python3
"""Run a fixture-free, headless FreeCAD/Draft mesh-layer integration probe.

Execute as a script with the tested FreeCADCmd. FreeCAD, Import, and Part must
come from that build. If Draft or its Python dependencies are not installed
with that build, set FREECAD_DRAFT_MODULE_DIR, FREECAD_3RDPARTY_DIR and
FREECAD_PYSIDE_DIR to matching local directories. Draft_rc must be importable.
Use a disposable FreeCAD user/system configuration for the command. The script
creates and deletes its DXF controls in a temporary directory; it does not
establish GUI rendering, platform coverage, or broad DWG/DXF support.
"""
from __future__ import annotations

import ctypes
import json
import os
from pathlib import Path
import sys
import tempfile
import uuid

for name in ("FREECAD_PYSIDE_DIR", "FREECAD_3RDPARTY_DIR", "FREECAD_DRAFT_MODULE_DIR"):
    if os.environ.get(name):
        sys.path.insert(0, os.environ[name])

# Headless FreeCAD builds may not install the legacy PySide compatibility alias.
try:
    import PySide  # noqa: F401
except ImportError:
    import PySide6
    from PySide6 import QtCore, QtGui, QtWidgets
    sys.modules["PySide"] = PySide6
    sys.modules["PySide.QtCore"] = QtCore
    sys.modules["PySide.QtGui"] = QtGui
    sys.modules["PySide.QtWidgets"] = QtWidgets

import Draft
import FreeCAD as App
import Import


def drawing(case: str, units: int = 4) -> str:
    polygon = case in ("polygon", "nested_polygon")
    nested = case in ("nested", "nested_polygon")
    tags = [(0, "SECTION"), (2, "HEADER"), (9, "$ACADVER"), (1, "AC1021"),
            (9, "$INSUNITS"), (70, units), (0, "ENDSEC"),
            (0, "SECTION"), (2, "TABLES"), (0, "TABLE"), (2, "LAYER"), (70, 3)]
    for layer in ("0", "MESH", "OTHER"):
        tags.extend([(0, "LAYER"), (2, layer), (70, 0), (62, 1), (6, "CONTINUOUS")])
    tags.extend([(0, "ENDTAB"), (0, "ENDSEC"), (0, "SECTION"),
                 (2, "BLOCKS" if nested else "ENTITIES")])
    if nested:
        tags.extend([(0, "BLOCK"), (8, "0"), (2, "INNER"), (70, 0),
                     (10, 0), (20, 0), (30, 0)])
    tags.extend([(0, "POLYLINE"), (5, "A"), (8, "MESH"), (66, 1),
                 (70, 16 if polygon else 64)])
    coords = ((0, 0), (0, 3), (4, 0), (4, 3)) if polygon else ((0, 0), (4, 0), (0, 3))
    if polygon:
        tags.extend([(71, 2), (72, 2)])
    for x, y in coords:
        tags.extend([(0, "VERTEX"), (8, "MESH"), (10, x), (20, y), (30, 0),
                     (70, 64 if polygon else 192)])
    if not polygon:
        tags.extend([(0, "VERTEX"), (8, "OTHER" if case == "mixed" else "MESH"),
                     (10, 0), (20, 0), (30, 0), (70, 128), (71, 1), (72, 2), (73, 3)])
    tags.extend([(0, "SEQEND"), (8, "MESH")])
    if nested:
        tags.extend([(0, "ENDBLK"), (8, "0"), (0, "ENDSEC"),
                     (0, "SECTION"), (2, "ENTITIES"), (0, "INSERT"),
                     (8, "0"), (2, "INNER"), (10, 1), (20, 2), (30, 0)])
    tags.extend([(0, "LINE"), (8, "0"), (10, 10), (20, 0), (30, 0),
                 (11, 12), (21, 0), (31, 0), (0, "ENDSEC"), (0, "EOF")])
    return "".join(f"{code}\n{value}\n" for code, value in tags)


def main() -> None:
    if not getattr(Draft, "__file__", None):
        raise RuntimeError("A real Draft module is required")
    qt_frameworks = {}
    if sys.platform == "darwin":
        dyld = ctypes.CDLL(None)
        dyld._dyld_image_count.restype = ctypes.c_uint32
        dyld._dyld_get_image_name.argtypes = [ctypes.c_uint32]
        dyld._dyld_get_image_name.restype = ctypes.c_char_p
        for index in range(dyld._dyld_image_count()):
            image = dyld._dyld_get_image_name(index).decode()
            for component in image.split("/"):
                if component.startswith("Qt") and component.endswith(".framework"):
                    qt_frameworks.setdefault(component, []).append(image)
        duplicates = {name: paths for name, paths in qt_frameworks.items() if len(paths) != 1}
        if "QtCore.framework" not in qt_frameworks or duplicates:
            raise RuntimeError(f"Missing QtCore or duplicate Qt frameworks: {qt_frameworks}")
    parent = App.ParamGet("User parameter:BaseApp/Preferences/Mod/Import")
    option_name = "CodexHeadlessMeshLayerProbe_" + uuid.uuid4().hex
    options = parent.GetGroup(option_name)
    options.SetInt("DxfImportMode", 2)
    options.SetBool("dxfUseDraftVisGroups", True)
    option_source = "User parameter:BaseApp/Preferences/Mod/Import/" + option_name
    results = []
    try:
        with tempfile.TemporaryDirectory(prefix="freecad-real-draft-layer-") as scratch:
            for case in ("polyface", "polygon", "mixed", "nested"):
                path = Path(scratch) / (case + ".dxf")
                path.write_text(drawing(case), encoding="ascii")
                doc = App.newDocument("RealDraftMeshProbe")
                try:
                    stats = Import.readDXF(str(path), doc.Name, True, option_source)
                    doc.recompute()
                    groups = {obj.Label: [member.Name for member in obj.Group]
                              for obj in doc.Objects
                              if hasattr(obj, "Group")
                              and getattr(getattr(obj, "Proxy", None), "Type", None) == "Layer"}
                    shapes = [obj for obj in doc.Objects
                              if hasattr(obj, "Shape") and not obj.Shape.isNull()]
                    lines = [obj for obj in shapes if obj.Name.startswith("Shape")
                             and len(obj.Shape.Edges) == 1]
                    assert len(lines) == 1 and lines[0].Name in groups["0"]
                    if case in ("polyface", "polygon"):
                        name = "Polyface" if case == "polyface" else "PolygonMesh"
                        meshes = [obj for obj in shapes if obj.Name.startswith(name)]
                        assert len(meshes) == 1 and meshes[0].Name in groups["MESH"]
                        assert meshes[0].OriginalLayer == "MESH"
                        assert len(meshes[0].Shape.Faces) == 1
                        assert abs(meshes[0].Shape.Area - (6 if case == "polyface" else 12)) < 1e-9
                        assert not stats["unsupportedFeatures"]
                    else:
                        reason = ("POLYFACE nondefault layer in block" if case == "nested"
                                  else "POLYFACE unsupported or invalid profile")
                        assert reason in stats["unsupportedFeatures"]
                        assert not groups["MESH"]
                    results.append({"case": case, "groups": groups,
                                    "unsupported": stats["unsupportedFeatures"]})
                finally:
                    App.closeDocument(doc.Name)
    finally:
        parent.RemGroup(option_name)
    print("FREECAD_REAL_DRAFT_MESH_LAYER=" + json.dumps({
        "draftModule": Draft.__file__, "importModule": Import.__file__,
        "qtFrameworkImages": qt_frameworks, "results": results
    }, sort_keys=True, default=str))


if __name__ in ("__main__", Path(__file__).stem):
    main()
