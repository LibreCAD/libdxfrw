#!/usr/bin/env python3
"""Author a scratch image DXF or hash-bind the documented AC1024 frame probe."""
import hashlib
import subprocess
import sys
from pathlib import Path

EXPECTED = "fc1371b35a566d916b4fd143e66333c6695dbd341aa31b2025bc5dcd45fe30ce"


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    if len(sys.argv) == 3 and sys.argv[1] == "author":
        import ezdxf
        if ezdxf.__version__ != "1.4.5":
            raise SystemExit("author requires ezdxf 1.4.5")
        document = ezdxf.new("R2010")
        image = document.add_image_def("source.png", (64, 64))
        document.modelspace().add_image(image, insert=(1, 2), size_in_units=(3, 4))
        document.saveas(sys.argv[2])
    elif len(sys.argv) == 4 and sys.argv[1] == "verify":
        if sha256(sys.argv[2]) != EXPECTED:
            raise SystemExit("source SHA-256 does not match the documented witness")
        subprocess.run([sys.argv[3], sys.argv[2]], check=True)
        if sha256(sys.argv[2]) != EXPECTED:
            raise SystemExit("source SHA-256 changed during the frame probe")
    else:
        raise SystemExit("usage: audit_ac1024_rastervariables_source.py author <scratch.dxf> | verify <documented.dwg> <compiled-probe>")


if __name__ == "__main__":
    main()
