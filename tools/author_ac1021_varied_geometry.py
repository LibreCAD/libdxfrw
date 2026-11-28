#!/usr/bin/env python3
"""Author five scratch DXF controls for the AC1021 varied-entity matrix."""
import argparse
from pathlib import Path

import ezdxf


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output_dir', type=Path)
    args = parser.parse_args()
    if ezdxf.__version__ != '1.4.5':
        raise SystemExit('requires ezdxf 1.4.5')
    args.output_dir.mkdir(parents=True, exist_ok=True)
    modes = ('line', 'ellipse', 'lwpolyline', 'text', 'mixed')
    paths = [args.output_dir / (mode + '.dxf') for mode in modes]
    if any(path.exists() for path in paths):
        raise SystemExit('refusing to overwrite a source drawing')
    for mode, path in zip(modes, paths):
        doc = ezdxf.new('R2007')
        model = doc.modelspace()
        if mode in ('line', 'mixed'):
            model.add_line((1, 2, 3), (4, 5, 6))
        if mode in ('ellipse', 'mixed'):
            model.add_ellipse((2, 3, 4), (4, 0, 0), .5, .2, 2.4)
        if mode in ('lwpolyline', 'mixed'):
            model.add_lwpolyline([(0, 0), (5, 0), (5, 3)], format='xy',
                                 dxfattribs={'elevation': 2})
        if mode in ('text', 'mixed'):
            model.add_text('HELLO', dxfattribs={'insert': (3, 4, 0),
                                               'height': 2, 'rotation': 30})
        doc.saveas(path)


if __name__ == '__main__':
    main()
