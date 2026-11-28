#!/usr/bin/env python3
"""Author scratch AC1021 POLYLINE paper-space and named-block witnesses."""
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
    modes = ('paper2d', 'paper3d', 'named2d', 'named3d')
    paths = [args.output_dir / (mode + '.dxf') for mode in modes]
    if any(path.exists() for path in paths):
        raise SystemExit('refusing to overwrite source drawings')
    for mode, path in zip(modes, paths):
        doc = ezdxf.new('R2007'); model = doc.modelspace()
        if mode.startswith('named'):
            target = doc.blocks.new('OWNER_TEST')
            model.add_blockref('OWNER_TEST', (10, 20, 0))
        else:
            target = doc.layout('Layout1')
            model.add_line((10, 20, 0), (11, 21, 0))
        target.add_line((-3, -1, 0), (-2, -1, 0))
        if mode.endswith('2d'):
            target.add_polyline2d([(0, 0), (5, 0), (5, 3)])
        else:
            target.add_polyline3d([(0, 0, 0), (5, 0, 1), (5, 3, 2)])
        target.add_line((7, 4, 0), (8, 4, 0))
        doc.saveas(path)


if __name__ == '__main__':
    main()
