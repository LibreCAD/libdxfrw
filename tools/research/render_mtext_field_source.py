#!/usr/bin/env python3
"""Render one cached MTEXT root from the pinned FIELD source for inspection.

Requires ezdxf, Pillow and CairoSVG in the selected Python environment.
Usage: python3 render_mtext_field_source.py SOURCE.dxf OUTPUT_DIR
The output is an SVG/PNG rendering of cached MTEXT, not field evaluation.
"""
import hashlib
import json
from pathlib import Path
import sys

import cairosvg
import ezdxf
from ezdxf.addons.drawing import Frontend, RenderContext, layout, svg

SOURCE_HASH = 'fb09a7af8882787553149a675c6d15b45bab8ff4a49f58200460832eab3de401'
TEXT_HASH = '0cdeb6e8385f4c102d02e17326b6b35f795913632b45803972ffe52be4c76ec2'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    if len(sys.argv) != 3:
        raise SystemExit(f'usage: {sys.argv[0]} SOURCE.dxf OUTPUT_DIR')
    source = Path(sys.argv[1])
    output = Path(sys.argv[2])
    if digest(source.read_bytes()) != SOURCE_HASH:
        raise ValueError('Source SHA-256 mismatch')
    doc = ezdxf.readfile(source)
    entity = doc.entitydb.get('9A')
    if entity is None or entity.dxftype() != 'MTEXT':
        raise ValueError('Expected root MTEXT 9A')
    if digest(entity.text.encode('utf-8')) != TEXT_HASH:
        raise ValueError('MTEXT cached text SHA-256 mismatch')
    backend = svg.SVGBackend()
    Frontend(RenderContext(doc), backend).draw_entities([entity])
    vector = backend.get_string(layout.Page(0, 0)).encode('utf-8')
    output.mkdir(parents=True, exist_ok=True)
    svg_file = output / 'field-root-9a.svg'
    png_file = output / 'field-root-9a.png'
    svg_file.write_bytes(vector)
    cairosvg.svg2png(bytestring=vector, write_to=str(png_file), output_width=1600)
    print(json.dumps({'source_sha256': SOURCE_HASH, 'root_mtext': '9A',
                      'cached_text_sha256': TEXT_HASH,
                      'ezdxf_version': ezdxf.__version__,
                      'cairosvg_version': cairosvg.__version__,
                      'svg_sha256': digest(vector),
                      'png_sha256': digest(png_file.read_bytes()),
                      'rendered_cached_text_only': True}, sort_keys=True))


if __name__ == '__main__':
    main()
