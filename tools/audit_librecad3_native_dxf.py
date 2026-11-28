#!/usr/bin/env python3
"""Author or independently inspect the scratch LibreCAD_3 planar DXF probe."""

import argparse
from pathlib import Path

import ezdxf


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("author", "check"))
    parser.add_argument("path", type=Path)
    args = parser.parse_args()
    if ezdxf.__version__ != "1.4.5":
        raise ValueError("this evidence profile requires ezdxf 1.4.5")
    if args.action == "author":
        if args.path.exists():
            raise ValueError("refusing to replace an existing source")
        doc = ezdxf.new("R2000")
        model = doc.modelspace()
        model.add_line((1, 2, 0), (4, 5, 0))
        model.add_circle((7, 8, 0), 2.5)
        doc.saveas(args.path)
        return
    doc = ezdxf.readfile(args.path)
    if doc.dxfversion != "AC1015" or doc.audit().errors:
        raise ValueError("saved file has wrong revision or audit errors")
    entities = list(doc.modelspace())
    if sorted(entity.dxftype() for entity in entities) != ["CIRCLE", "LINE"]:
        raise ValueError("unexpected model-space entity inventory")
    line = next(entity for entity in entities if entity.dxftype() == "LINE")
    circle = next(entity for entity in entities if entity.dxftype() == "CIRCLE")
    if (tuple(line.dxf.start) != (1, 2, 0) or tuple(line.dxf.end) != (4, 5, 0)
            or tuple(circle.dxf.center) != (7, 8, 0) or circle.dxf.radius != 2.5):
        raise ValueError("saved geometry differs from the authored source")
    print("AC1015 planar LINE/CIRCLE independent audit: PASS")


if __name__ == "__main__":
    main()
