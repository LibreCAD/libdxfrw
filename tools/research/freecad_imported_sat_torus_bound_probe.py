"""Measure the selected source SAT solid in FreeCAD's Python console.

Set P7_SAT_PATH and P7_INVENTORLOADER_PATH to the recovered SAT and the
pinned InventorLoader checkout.  The probe accepts only the recorded SAT and
Acis.py hashes, and fails if the expected single untransformed body is absent.
Run with FreeCADCmd -c by piping an exec(open(...).read()) expression.
"""
import hashlib
import json
import math
import os
import sys

sat_path = os.environ["P7_SAT_PATH"]
loader_path = os.environ["P7_INVENTORLOADER_PATH"]


def digest(path):
    with open(path, "rb") as stream:
        return hashlib.sha256(stream.read()).hexdigest()


sat_sha = digest(sat_path)
loader_sha = digest(os.path.join(loader_path, "Acis.py"))
if sat_sha != "4917d971df23651987e01f18cf8635d19dab7ec2c306c910734ba0093641da08":
    raise ValueError("SAT bytes do not match the selected source payload")
if loader_sha != "ec84f9528355f0df2e28bd94eecf7d9e4be89c5b9f7a77eef9b9cccbc21366bb":
    raise ValueError("InventorLoader Acis.py does not match the pinned importer")
for name, expected in (
    ("importerUtils.py", "ce95b3dd5cd1c5a69adf61a03549e196a88e14b254dc3f482d694e3e78404715"),
    ("importerConstants.py", "f7db288e42fdd1653caccbdc2b4f3e73c13d0dcf41b2a0856dc71f6b1d65807e"),
):
    if digest(os.path.join(loader_path, name)) != expected:
        raise ValueError("InventorLoader dependency does not match pinned importer: " + name)

sys.path.insert(0, loader_path)
import Acis  # noqa: E402
import FreeCAD as App  # noqa: E402
import Part  # noqa: E402
for name in ("Acis", "importerUtils", "importerConstants"):
    actual = os.path.realpath(sys.modules[name].__file__)
    expected = os.path.realpath(os.path.join(loader_path, name + ".py"))
    if actual != expected:
        raise ValueError("imported module from unexpected path: " + name)

with open(sat_path, "r", encoding="ascii") as stream:
    reader = Acis.AcisReader(stream)
    if not reader.readText():
        raise ValueError("SAT parser did not accept the selected payload")
Acis.init()
Acis.setReader(reader)
bodies = []
for record in reader.getRecords():
    if record.name in ("Begin-of-ACIS-History-Data", "End-of-ACIS-History-Section", "End-of-ACIS-data"):
        break
    entity = Acis.createEntity(record)
    if record.name == "body":
        bodies.append(entity)
if len(bodies) != 1 or bodies[0].getTransform() is not None:
    raise ValueError("expected exactly one untransformed imported SAT body")
faces = []
for lump in bodies[0].getLumps():
    for shell in lump.getShells():
        for face in shell.getFaces():
            built = face.build()
            if built is None:
                raise ValueError("SAT face builder returned no face")
            faces.append(built)
if len(faces) != 1:
    raise ValueError("expected exactly one source-derived face")
shell = Part.Shell(faces)
if not shell.isValid() or not shell.isClosed():
    raise ValueError("source-derived shell is not valid and closed")
shape = Part.makeSolid(shell)
if not shape.isValid():
    raise ValueError("source-derived solid is invalid")


def box_values(box):
    return [box.XMin, box.YMin, box.ZMin, box.XMax, box.YMax, box.ZMax]


def snapshot():
    return {
        "default": box_values(shape.BoundBox),
        "optimal_no_mesh": box_values(shape.optimalBoundingBox(False, False)),
        "optimal_mesh": box_values(shape.optimalBoundingBox(True, False)),
    }


center = [1383.6151200595348, 418.50000000000074, 1158.755189492682]
axis = [-0.5014237824134797, -1.6431677767887314e-16, -0.8652018206350235]
major, minor = 19.0, 2.7899834596164137
extents = [major * math.sqrt(max(0.0, 1.0 - value * value)) + minor for value in axis]
analytic = [center[i] - extents[i] for i in range(3)] + [center[i] + extents[i] for i in range(3)]
result = {
    "sat_sha256": sat_sha,
    "importer_sha256": loader_sha,
    "body_count": len(bodies),
    "shell_closed": shell.isClosed(),
    "valid": shape.isValid(),
    "topology": [len(shape.Faces), len(shape.Edges), len(shape.Vertexes)],
    "area": shape.Area,
    "volume": shape.Volume,
    "freecad_version": App.Version(),
    "python_version": sys.version,
    "module_paths": {name: getattr(sys.modules.get(name), "__file__", None)
                     for name in ("Acis", "importerUtils", "importerConstants", "Part", "olefile", "PySide")},
    "analytic": analytic,
    "before": snapshot(),
}
for deflection in (0.5, 0.05, 0.005):
    shape.tessellate(deflection)
    result[str(deflection)] = snapshot()
max_error = max(abs(a - b) for key in ("before", "0.5", "0.05", "0.005")
                for a, b in zip(result[key]["optimal_no_mesh"], analytic))
if max_error > 2e-7 or result["topology"] != [1, 2, 1]:
    raise ValueError("source-derived optimal bounds or topology differ from the selected profile")
result["max_optimal_no_mesh_analytic_error"] = max_error
print("P7_IMPORTED_SAT_BOUNDS_JSON=" + json.dumps(result, sort_keys=True))
