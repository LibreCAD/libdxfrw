#!/usr/bin/env python3
"""Author and verify a scratch sparse-width LWPOLYLINE with ezdxf 1.4.5."""
import sys

import ezdxf
from ezdxf.lldxf.tagwriter import BinaryTagWriter, TagWriter


def main():
    if ezdxf.__version__ != "1.4.5":
        raise SystemExit("requires ezdxf 1.4.5")
    if len(sys.argv) != 4 or sys.argv[1] not in ("author", "verify") or sys.argv[3] not in ("asc", "bin"):
        raise SystemExit("usage: audit_dxf_sparse_lwpolyline.py author|verify <scratch.dxf> asc|bin")
    expected = [(1., 2., 0., 0., 0.), (3., 4., .4, .8, -.5), (5., 6., 0., 0., .25)]
    if sys.argv[1] == "author":
        document = ezdxf.new("R2013")
        document.modelspace().add_lwpolyline(expected, format="xyseb")
        document.modelspace().add_line((11, 12, 13), (14, 15, 16))
        # Use ezdxf's entity serializer with a minimal container so unrelated
        # producer OBJECTS do not affect the libdxfrw callback probe.
        with open(sys.argv[2], "wb" if sys.argv[3] == "bin" else "w") as stream:
            if sys.argv[3] == "bin":
                stream.write(b"AutoCAD Binary DXF\r\n\x1a\x00")
                writer = BinaryTagWriter(stream, dxfversion="AC1027")
            else:
                writer = TagWriter(stream, dxfversion="AC1027")
            for code, value in ((0, "SECTION"), (2, "HEADER"), (9, "$ACADVER"), (1, "AC1027"),
                                (0, "ENDSEC"), (0, "SECTION"), (2, "ENTITIES")):
                writer.write_tag2(code, value)
            for entity in document.modelspace():
                entity.export_dxf(writer)
            writer.write_tag2(0, "ENDSEC");writer.write_tag2(0, "EOF")
    else:
        model = list(ezdxf.readfile(sys.argv[2]).modelspace())
        assert [entity.dxftype() for entity in model] == ["LWPOLYLINE", "LINE"]
        assert [tuple(point) for point in model[0].get_points("xyseb")] == expected
        assert tuple(model[1].dxf.start) == (11., 12., 13.)
        print("ezdxf sparse per-vertex source PASS")


if __name__ == "__main__":
    main()
