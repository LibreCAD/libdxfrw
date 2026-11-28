"""Build a placed ACIS 700 SAT cube through InventorLoader and FreeCAD Part.

Run this script inside FreeCADCmd's Python environment with
P7_SAT_PATH and P7_INVENTORLOADER_PATH set. The single runtime patch adapts
InventorLoader's SAT-header reader to the standard ACIS @length string prefix;
the record tokenizer and entity builders remain the pinned upstream code.
"""
import hashlib
import json
import os
import sys

SAT_SHA256 = "119ed8a97ce3d4d72d8d197157244f4f310fb0fccde6c0dd780becba1ef08920"
ACIS_PY_SHA256 = "ec84f9528355f0df2e28bd94eecf7d9e4be89c5b9f7a77eef9b9cccbc21366bb"
IMPORTER_UTILS_SHA256 = "ce95b3dd5cd1c5a69adf61a03549e196a88e14b254dc3f482d694e3e78404715"
IMPORTER_CONSTANTS_SHA256 = "f7db288e42fdd1653caccbdc2b4f3e73c13d0dcf41b2a0856dc71f6b1d65807e"


def digest(path):
    with open(path, "rb") as stream:
        return hashlib.sha256(stream.read()).hexdigest()


sat_path = os.environ["P7_SAT_PATH"]
loader_path = os.environ["P7_INVENTORLOADER_PATH"]
if digest(sat_path) != SAT_SHA256:
    raise ValueError("SAT input hash does not match the recorded sample")
expected_modules = {
    "Acis.py": ACIS_PY_SHA256,
    "importerUtils.py": IMPORTER_UTILS_SHA256,
    "importerConstants.py": IMPORTER_CONSTANTS_SHA256,
}
for filename, expected in expected_modules.items():
    if digest(os.path.join(loader_path, filename)) != expected:
        raise ValueError("pinned InventorLoader file hash mismatch: " + filename)

sys.path.insert(0, loader_path)
import Acis
import FreeCAD as App
import Part
import importerConstants
import importerUtils

for module, filename in (
    (Acis, "Acis.py"),
    (importerUtils, "importerUtils.py"),
    (importerConstants, "importerConstants.py"),
):
    if os.path.realpath(module.__file__) != os.path.realpath(
        os.path.join(loader_path, filename)
    ):
        raise ValueError("unexpected InventorLoader module path: " + filename)

# InventorLoader's text-header helper accepts bare decimal string lengths,
# while standard ACIS 700 SAT uses @length. Retain the pinned parser and alter
# only how its existing header helper reads that prefix for this probe.
def read_text_header_string(self, data):
    start = 1 if data.startswith("@") else 0
    index = start
    while index < len(data) and data[index].isdigit():
        index += 1
    if index == start:
        raise ValueError("missing ACIS text string length")
    count = int(data[start:index])
    remaining = data[index + 1:]
    if len(remaining) < count:
        remaining += self._stream.readline()
    return remaining[:count], remaining[count + 1:]


Acis.AcisReader._getNextText = read_text_header_string

with open(sat_path, "r", encoding="ascii") as stream:
    reader = Acis.AcisReader(stream)
    if not reader.readText():
        raise ValueError("InventorLoader rejected the SAT stream")

Acis.init()
Acis.setReader(reader)
bodies = []
for record in reader.getRecords():
    if record.name in (
        "Begin-of-ACIS-History-Data",
        "End-of-ACIS-History-Section",
        "End-of-ACIS-data",
    ):
        break
    entity = Acis.createEntity(record)
    if record.name == "body":
        bodies.append(entity)
if len(bodies) != 1:
    raise ValueError("expected one body; got %d" % len(bodies))

faces = []
for lump in bodies[0].getLumps():
    for shell_record in lump.getShells():
        for face_record in shell_record.getFaces():
            face = face_record.build()
            if face is None:
                raise ValueError("SAT face builder returned no face")
            faces.append(face)
shell = Part.Shell(faces)
shape = Part.makeSolid(shell)
if not shell.isValid() or not shell.isClosed() or not shape.isValid():
    raise ValueError("native FreeCAD shell or solid is invalid")

# ACIS BODY topology is defined in body space; its TRANSFORM record positions
# the body in world space. The pinned InventorLoader exposes that reference
# separately from its face builders, so apply it before reporting world bounds.
body_transform = bodies[0].getTransform()
if body_transform is None:
    raise ValueError("SAT body has no transform record")
placement = body_transform.getPlacement()
placed_shape = shape.transformShape(placement.toMatrix(), True)
if placed_shape is not None:
    shape = placed_shape
if not shape.isValid():
    raise ValueError("native FreeCAD placed solid is invalid")

box = shape.optimalBoundingBox(False, False)
area = shape.Area
volume = shape.Volume
expected_area = 6.0 * (777.0 ** 2)
expected_volume = 777.0 ** 3
result = {
    "sat_sha256": SAT_SHA256,
    "inventorloader_acis_sha256": ACIS_PY_SHA256,
    "freecad_version": App.Version(),
    "body_count": len(bodies),
    "face_count": len(faces),
    "shell_closed": shell.isClosed(),
    "valid": shape.isValid(),
    "topology": [len(shape.Faces), len(shape.Edges), len(shape.Vertexes)],
    "bodyPlacement": str(placement),
    "bounds": [box.XMin, box.YMin, box.ZMin,
               box.XMax, box.YMax, box.ZMax],
    "area": area,
    "volume": volume,
    "relative_area_error": abs(area - expected_area) / expected_area,
    "relative_volume_error": abs(volume - expected_volume) / expected_volume,
    "headerParserAdjustment": "Runtime @length support in InventorLoader _getNextText only; pinned record tokenizer and entity builders unchanged.",
}
if result["topology"] != [6, 12, 8]:
    raise ValueError("unexpected cube topology: " + repr(result["topology"]))
if result["bounds"] != [0.0, 0.0, 0.0, 777.0, 777.0, 777.0]:
    raise ValueError("unexpected cube bounds: " + repr(result["bounds"]))
if max(result["relative_area_error"], result["relative_volume_error"]) > 1e-12:
    raise ValueError("native measurements differ from cube formulas")
print("P7_ACIS700_FREECAD_JSON=" + json.dumps(result, sort_keys=True))
