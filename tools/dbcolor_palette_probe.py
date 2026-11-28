#!/usr/bin/env python3
"""Prepare and compare locally authored DBCOLOR RGB/fallback probes.

Preparation and the optional ezdxf palette comparison need ezdxf; conversion
must be performed separately. No saved producer output is a test fixture.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re
import tempfile

from dbcolor_fallback_differential import digest, fields, make_base, read_pairs, record_bounds

RGBS = (0x000000, 0xffffff, 0xff0000, 0xffff00, 0x00ff00, 0x00ffff,
        0x0000ff, 0xff00ff, 0x333333, 0x505050, 0x696969, 0x808080,
        0x828282, 0xbebebe, 0xc0c0c0, 0xcfd0cf, 0xcccccc, 0xf0f0f0,
        0x123456, 0x64320a, 0xabcdef, 0xa0a0a0, 0x7f007f, 0x808000,
        0x000080, 0x404040, 0x7f7f7f, 0x818181, 0x7f0000, 0xa50000,
        0xff7f7f, 0x557799, 0x112233, 0x102030, 0x999999)


def nearest(rgb: int, palette: list[int]) -> list[int]:
    if len(palette) < 256:
        raise ValueError('palette has fewer than 256 entries')
    def distance(color: int) -> int:
        return sum((((rgb >> s) & 255) - ((color >> s) & 255)) ** 2
                   for s in (0, 8, 16))
    best = min(distance(c) for c in palette[1:256])
    return [i for i in range(1, 256) if distance(palette[i]) == best]


def palette_from_libredwg(path: Path) -> list[int]:
    source = path.read_text(encoding='utf-8')
    region = source.split('rgb_palette[256] = {', 1)[1].split('};', 1)[0]
    tuples = re.findall(r'\{\s*0x([0-9a-fA-F]+)\s*,\s*0x([0-9a-fA-F]+)\s*,\s*0x([0-9a-fA-F]+)\s*\}', region)
    palette = [(int(r, 16) << 16) | (int(g, 16) << 8) | int(b, 16)
               for r, g, b in tuples]
    if len(palette) != 256:
        raise ValueError('expected exactly 256 LibreDWG palette entries')
    return palette


def prepare(directory: Path) -> None:
    import ezdxf
    directory.mkdir(parents=True, exist_ok=False)
    with tempfile.TemporaryDirectory(prefix='dbcolor-palette-scaffold-') as tmp:
        scaffold = Path(tmp) / 'scaffold.dxf'
        document = ezdxf.new('R2007')
        document.header['$HANDSEED'] = '400'
        document.saveas(scaffold)
        base = make_base(read_pairs(scaffold))
    start, end = record_bounds(base, 'DBCOLOR')
    for rgb in RGBS:
        for named in (False, True):
            record = []
            for code, value in base[start:end]:
                if code == '1001':  # Exclude XDATA from this palette-only probe.
                    break
                if code == '420':
                    value = str(rgb)
                if code == '430' and not named:
                    continue
                record.append((code, value))
            pairs = base[:start] + record + base[end:]
            name = f'{rgb:06x}_{"named" if named else "unnamed"}.dxf'
            (directory / name).write_text(''.join(f'{c}\n{v}\n' for c, v in pairs),
                                          encoding='utf-8')
    print(json.dumps({'ezdxfVersion': ezdxf.__version__, 'inputs': 2 * len(RGBS)}))


def compare(inputs: Path, outputs: Path, libre_source: Path) -> None:
    import ezdxf
    from ezdxf.colors import DXF_DEFAULT_COLORS
    libre_palette = palette_from_libredwg(libre_source)
    results = []
    for rgb in sorted(RGBS):
        for named in (False, True):
            name = f'{rgb:06x}_{"named" if named else "unnamed"}.dxf'
            source, output = inputs / name, outputs / name
            if not source.is_file() or not output.is_file() or output.with_suffix('.dxf.err').exists():
                raise ValueError(f'missing input/output or converter error: {name}')
            input_fields, output_fields = fields(read_pairs(source)), fields(read_pairs(output))
            if (input_fields['group62'] or input_fields['group420'] != [str(rgb)] or
                len(output_fields['group62']) != 1 or output_fields['group420'] != [str(rgb)]):
                raise ValueError(f'unexpected DBCOLOR input or output fields: {name}')
            results.append({'name': name, 'inputSha256': digest(source),
                            'outputSha256': digest(output),
                            'input420': rgb, 'input430': input_fields['group430'],
                            'output420': int(output_fields['group420'][0]),
                            'output430': output_fields['group430'],
                            'output62': int(output_fields['group62'][0]),
                            'ezdxfEuclideanNearest': nearest(rgb, DXF_DEFAULT_COLORS),
                            'libredwgEuclideanNearest': nearest(rgb, libre_palette)})
    print(json.dumps({'ezdxfVersion': ezdxf.__version__,
                      'libredwgDwgCSha256': digest(libre_source),
                      'results': results}, indent=2))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest='command', required=True)
    p = sub.add_parser('prepare'); p.add_argument('inputs', type=Path)
    p = sub.add_parser('compare'); p.add_argument('inputs', type=Path); p.add_argument('outputs', type=Path); p.add_argument('libredwg_dwg_c', type=Path)
    sub.add_parser('self-test')
    args = parser.parse_args()
    if args.command == 'prepare':
        prepare(args.inputs)
    elif args.command == 'compare':
        compare(args.inputs, args.outputs, args.libredwg_dwg_c)
    else:
        palette = [0] * 256
        palette[1], palette[2], palette[3] = 0x010101, 0x030303, 0x030303
        assert nearest(0x020202, palette) == [1, 2, 3]
        print('DBCOLOR palette comparator self-test: PASS')


if __name__ == '__main__':
    main()
