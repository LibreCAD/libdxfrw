#!/usr/bin/env python3
"""Author a new local frame-one AC1024 WIPEOUTVARIABLES scratch DXF."""
import argparse
from pathlib import Path
import ezdxf

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('output', type=Path)
args = parser.parse_args()
if ezdxf.__version__ != '1.4.5':
    parser.error('ezdxf 1.4.5 required')
if args.output.exists():
    parser.error('output already exists')
args.output.parent.mkdir(parents=True, exist_ok=True)
doc = ezdxf.new('R2010')
doc.modelspace().add_wipeout([(0, 0), (4, 0), (4, 4), (0, 4)])
doc.rootdict['ACAD_WIPEOUT_VARS'].dxf.frame = 1
doc.saveas(args.output)
