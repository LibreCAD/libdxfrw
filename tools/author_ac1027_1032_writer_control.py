#!/usr/bin/env python3
"""Create the scratch R2013 DXF used for modern writer comparison."""
from pathlib import Path
import sys
import ezdxf
if len(sys.argv) != 2 or ezdxf.__version__ != '1.4.5':
    raise SystemExit('usage: author_source.py NEW_DXF (ezdxf 1.4.5 required)')
p=Path(sys.argv[1])
if p.exists(): raise SystemExit('refusing overwrite')
d=ezdxf.new('R2013'); d.modelspace().add_line((1,2,3),(4,5,6)); d.saveas(p)
