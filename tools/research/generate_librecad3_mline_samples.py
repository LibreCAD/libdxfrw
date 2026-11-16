#!/usr/bin/env python3
"""Generate temporary DXF/DWG LINE + MLINE controls for LibreCAD_3.

Requires ezdxf and ODA File Converter. Output must be a new directory outside
this repository. The minimized DXF avoids unrelated unsupported OBJECTS while
retaining a valid STANDARD MLINESTYLE dictionary and original entity records.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
from pathlib import Path


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--odafc', type=Path, required=True)
    args = parser.parse_args()
    import ezdxf

    converter = args.odafc.resolve(strict=True)
    if not converter.is_file():
        parser.error('--odafc must name a converter executable')
    root = args.output.resolve()
    repo = Path(__file__).resolve().parents[2]
    if root == repo or repo in root.parents:
        parser.error('--output must be outside the repository')
    root.mkdir(parents=True, exist_ok=False)
    source_dir, dwg_dir = root / 'source', root / 'dwg'
    source_dir.mkdir()
    dwg_dir.mkdir()
    doc = ezdxf.new('R2010')
    model = doc.modelspace()
    model.add_line((0, 0), (5, 0))
    model.add_mline([(0, 2), (5, 2)], dxfattribs={'style_name': 'STANDARD'})
    source = source_dir / 'mline.dxf'
    doc.saveas(source)
    subprocess.run([str(converter), str(source_dir), str(dwg_dir),
                    'ACAD2010', 'DWG', '0', '1'], check=True)
    dwg = dwg_dir / 'mline.dwg'
    if dwg.read_bytes()[:6] != b'AC1024':
        raise ValueError('converter did not produce an AC1024 DWG')

    lines = source.read_text().splitlines()
    pairs = [(lines[i].strip(), lines[i + 1]) for i in range(0, len(lines), 2)]
    start = pairs.index(('2', 'ENTITIES')) + 1
    end = next(i for i in range(start, len(pairs)) if pairs[i] == ('0', 'ENDSEC'))
    records = []
    for code, value in pairs[start:end]:
        if code == '0':
            records.append([])
        if not records:
            raise ValueError('entity section missing a record boundary')
        records[-1].append((code, value))
    selected = {record[0][1]: record for record in records}
    if set(selected) != {'LINE', 'MLINE'} or len(records) != 2:
        raise ValueError('unexpected generated entity records')
    style = next(value for code, value in selected['MLINE'] if code == '340')
    header = [('0', 'SECTION'), ('2', 'HEADER'), ('9', '$ACADVER'), ('1', 'AC1015'),
              ('0', 'ENDSEC'), ('0', 'SECTION'), ('2', 'ENTITIES')]
    end_entities = [('0', 'ENDSEC')]
    # The source's STANDARD style uses these offsets, color and angles.
    objects = [('0', 'SECTION'), ('2', 'OBJECTS'),
               ('0', 'DICTIONARY'), ('5', 'F00'), ('330', '0'), ('100', 'AcDbDictionary'),
               ('281', '1'), ('3', 'ACAD_MLINESTYLE'), ('350', 'F01'),
               ('0', 'DICTIONARY'), ('5', 'F01'), ('330', 'F00'), ('100', 'AcDbDictionary'),
               ('281', '1'), ('3', 'STANDARD'), ('350', style),
               ('0', 'MLINESTYLE'), ('5', style), ('330', 'F01'), ('100', 'AcDbMlineStyle'),
               ('2', 'STANDARD'), ('70', '0'), ('3', ''), ('62', '256'), ('51', '90'),
               ('52', '90'), ('71', '2'), ('49', '0.5'), ('62', '256'), ('6', 'BYLAYER'),
               ('49', '-0.5'), ('62', '256'), ('6', 'BYLAYER'), ('0', 'ENDSEC')]
    def write(path: Path, groups: list[tuple[str, str]]) -> None:
        path.write_text(''.join(f'{code}\n{value}\n' for code, value in groups + [('0', 'EOF')]))
        audit = ezdxf.readfile(path).audit()
        if audit.has_errors or audit.has_fixes:
            raise ValueError(f'generated DXF did not pass ezdxf audit: {path}')
    dxf, line = root / 'mline.dxf', root / 'line.dxf'
    write(dxf, header + selected['LINE'] + selected['MLINE'] + end_entities + objects)
    write(line, header + selected['LINE'] + end_entities)
    paths = {'dxf': dxf, 'dwg': dwg, 'lineOnlyDxf': line, 'producerDxf': source}
    receipt = {'ezdxfVersion': ezdxf.__version__, 'converter': str(converter),
               'files': {key: {'path': str(path), 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
                         for key, path in paths.items()}}
    (root / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps(receipt, indent=2))
    print('Set LC3_MLINE_DXF, LC3_MLINE_DWG, and LC3_LINE_DXF to the generated paths.')


if __name__ == '__main__':
    main()
