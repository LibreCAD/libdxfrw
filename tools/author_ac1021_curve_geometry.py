#!/usr/bin/env python3
"""Create scratch DXFs for the AC1021 curve and owned-POLYLINE matrix."""
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
    modes = ('line', 'spline', 'hatch', 'polyline2d', 'polyline3d', 'mixed')
    paths = [args.output_dir / (mode + '.dxf') for mode in modes]
    if any(p.exists() for p in paths):
        raise SystemExit('refusing to overwrite source drawings')
    for mode, path in zip(modes, paths):
        doc = ezdxf.new('R2007'); model = doc.modelspace()
        if mode in ('line', 'mixed'):
            model.add_line((1, 2, 3), (4, 5, 6))
        if mode in ('spline', 'mixed'):
            model.add_open_spline([(0, 0, 0), (2, 4, 0), (5, 0, 0)],
                                  degree=2, knots=[0, 0, 0, 1, 1, 1])
        if mode in ('hatch', 'mixed'):
            hatch = model.add_hatch()
            hatch.paths.add_polyline_path([(0, 0), (5, 0), (5, 3), (0, 3)],
                                          is_closed=True, flags=2)
        if mode in ('polyline2d', 'mixed'):
            model.add_polyline2d([(0, 0), (5, 0), (5, 3)])
        if mode == 'polyline3d':
            model.add_polyline3d([(0, 0, 0), (5, 0, 1), (5, 3, 2)])
        doc.saveas(path)


if __name__ == '__main__':
    main()
