#!/usr/bin/env python3
"""Scratch LWPOLYLINE missing-Y and explicit-zero controls for ezdxf."""
import sys
import ezdxf
from ezdxf.lldxf.const import DXFStructureError
from ezdxf.lldxf.tagwriter import BinaryTagWriter, TagWriter


def author(path, binary, missing):
    with open(path, "wb" if binary else "w") as stream:
        if binary:
            stream.write(b"AutoCAD Binary DXF\r\n\x1a\x00")
            writer = BinaryTagWriter(stream, dxfversion="AC1027")
        else:
            writer = TagWriter(stream, dxfversion="AC1027")
        for code, value in ((0, "SECTION"), (2, "HEADER"), (9, "$ACADVER"), (1, "AC1027"),
                            (0, "ENDSEC"), (0, "SECTION"), (2, "ENTITIES"),
                            (0, "LWPOLYLINE"), (100, "AcDbEntity"), (8, "0"),
                            (100, "AcDbPolyline"), (90, 3)):
            writer.write_tag2(code, value)
        for index in range(3):
            writer.write_tag2(10, float(index + 1))
            if not missing or index != 1:
                writer.write_tag2(20, 0.)
        writer.write_tag2(0, "ENDSEC");writer.write_tag2(0, "EOF")


def verify(path, missing):
    try:
        drawing = ezdxf.readfile(path)
    except DXFStructureError as error:
        if missing and "Missing required y coordinate" in str(error):
            print("ezdxf rejects missing Y PASS")
            return
        raise
    if missing:
        raise AssertionError("ezdxf accepted the missing-Y source")
    entities = list(drawing.modelspace())
    assert len(entities) == 1 and [point[1] for point in entities[0].get_points()] == [0., 0., 0.]
    print("ezdxf explicit zero Y PASS")


def main():
    if len(sys.argv) != 5 or sys.argv[1] not in ("author", "verify") or sys.argv[3] not in ("asc", "bin") or sys.argv[4] not in ("missing", "zero"):
        raise SystemExit("usage: audit_dxf_lwpolyline_missing_y.py author|verify <scratch.dxf> asc|bin missing|zero")
    if ezdxf.__version__ != "1.4.5":
        raise SystemExit("requires ezdxf 1.4.5")
    if sys.argv[1] == "author":
        author(sys.argv[2], sys.argv[3] == "bin", sys.argv[4] == "missing")
    else:
        verify(sys.argv[2], sys.argv[4] == "missing")


if __name__ == "__main__":
    main()
