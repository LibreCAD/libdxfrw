#!/usr/bin/env python3
"""Author a scratch-only R2013 WIPEOUT source for direct-NOD graph analysis."""
import argparse
import hashlib
from pathlib import Path
import ezdxf

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("output", type=Path)
args = parser.parse_args()
if ezdxf.__version__ != "1.4.5":
    parser.error("ezdxf 1.4.5 is required")
if args.output.exists():
    parser.error("output must not already exist")
args.output.parent.mkdir(parents=True, exist_ok=True)
document = ezdxf.new("R2013")
document.modelspace().add_wipeout([(0, 0), (4, 0), (4, 4), (0, 4)])
document.saveas(args.output)
print("sha256=" + hashlib.sha256(args.output.read_bytes()).hexdigest())
