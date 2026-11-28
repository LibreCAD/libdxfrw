"""FreeCAD console probe for the recorded source-derived torus parameters.

Run inside FreeCAD's Python console, for example by piping an exec(open(...))
expression to freecadcmd -c. This constructs a new torus; it does not import
the source SAT or qualify that carrier's topology.
"""
import json
import math

import FreeCAD as App
import Part

center = [1383.6151200595348, 418.50000000000074, 1158.755189492682]
axis = [-0.5014237824134797, -1.6431677767887314e-16, -0.8652018206350235]
major, minor = 19.0, 2.7899834596164137
shape = Part.makeTorus(major, minor, App.Vector(*center), App.Vector(*axis))


def box_values(box):
    return [box.XMin, box.YMin, box.ZMin, box.XMax, box.YMax, box.ZMax]


def snapshot():
    return {
        "default": box_values(shape.BoundBox),
        "optimal_no_mesh": box_values(shape.optimalBoundingBox(False, False)),
        "optimal_mesh": box_values(shape.optimalBoundingBox(True, False)),
    }


extent = [major * math.sqrt(max(0.0, 1.0 - n * n)) + minor for n in axis]
analytic = [center[i] - extent[i] for i in range(3)] + [
    center[i] + extent[i] for i in range(3)
]
result = {
    "analytic": analytic,
    "before": snapshot(),
    "valid": shape.isValid(),
    "area": shape.Area,
    "volume": shape.Volume,
    "topology": [len(shape.Faces), len(shape.Edges), len(shape.Vertexes)],
}
for deflection in (0.5, 0.05, 0.005):
    shape.tessellate(deflection)
    result[str(deflection)] = snapshot()
print("P7_TORUS_BOUNDS_JSON=" + json.dumps(result, sort_keys=True))
