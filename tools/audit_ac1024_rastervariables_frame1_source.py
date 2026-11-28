#!/usr/bin/env python3
"""Author the scratch witness or verify its exact AC1024 source and DXF pair."""
import hashlib
import subprocess
import sys
from pathlib import Path

EXPECTED = (
    "ebb8fd284c6180bf75d5d1383667ba0582beda9bf194ed95bb677213654cf5e7",
    "8ee228314bea28c6f850ca1f79e3cf201b3d0a672fbfa1749b6ea41b890e9ef2",
    "38ff22e2e90c80693ff04014012a63bf6247191904f12ea20a32693dcefa04be",
)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    if len(sys.argv) == 3 and sys.argv[1] == "author":
        import ezdxf
        if ezdxf.__version__ != "1.4.5":
            raise SystemExit("author requires ezdxf 1.4.5")
        doc = ezdxf.new("R2010")
        doc.set_raster_variables(frame=1, quality=1, units="m")
        doc.modelspace().add_line((1, 2, 0), (4, 5, 0))
        doc.saveas(sys.argv[2])
        return
    if len(sys.argv) != 6 or sys.argv[1] != "verify":
        raise SystemExit("usage: audit_ac1024_rastervariables_frame1_source.py author <scratch.dxf> | verify <input.dxf> <source.dwg> <oda-output.dxf> <compiled-probe>")
    paths = sys.argv[2:5]
    if tuple(map(digest, paths)) != EXPECTED:
        raise SystemExit("source or paired DXF SHA-256 does not match witness")
    import ezdxf
    for path in (paths[0], paths[2]):
        doc = ezdxf.readfile(path)
        record = doc.rootdict.get("ACAD_IMAGE_VARS")
        if (doc.dxfversion != "AC1024" or record is None
                or (record.dxf.class_version, record.dxf.frame,
                    record.dxf.quality, record.dxf.units) != (0, 1, 1, 3)):
            raise SystemExit("DXF version or raster variables differ")
    subprocess.run([sys.argv[5], paths[1]], check=True)
    if tuple(map(digest, paths)) != EXPECTED:
        raise SystemExit("witness changed during verification")
    print("hash-bound AC1024 producer input, wire and independent DXF fields PASS")


if __name__ == "__main__":
    main()
