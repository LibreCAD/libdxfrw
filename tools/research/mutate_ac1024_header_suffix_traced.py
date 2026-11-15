#!/usr/bin/env python3
"""Create a scratch-only HEADER suffix mutation using pinned LZ provenance.

The verifier dependency is a separately downloaded checkout of
realDragonium/dwg-verifier at 5a7dc5c6790cd8fece1494c710585019a253dbba.
This tool is an experiment, not a DWG repair or a support qualification.
"""
import argparse
import hashlib
import importlib.util
import inspect
from pathlib import Path
import struct
import sys

PINNED = {
    '__init__.py': 'e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855',
    'r2004.py': '3b0c692a41f0319687f221d6ed0c4bf35919641cf2de29c6e024d8f127ae24b1',
    'structural.py': '8fd01b73158cbabf8944ff3d903ac83af9a1215e2315ee6cbadd31b444b776b1',
    'report.py': '8b0ea26b0d64c73fdca20683aae192c731ed2448b0fe64e751bafba9c04d4a6d',
}
MAX_WINDOW = 1024 * 1024
MAX_SOURCE = 512 * 1024 * 1024


def sha(data):
    return hashlib.sha256(data).hexdigest()


def load_verifier(checkout):
    package = checkout / 'src' / 'dwgcheck'
    for name, expected in PINNED.items():
        if sha((package / name).read_bytes()) != expected:
            raise ValueError('verifier dependency does not match pinned source: ' + name)
    sys.path.insert(0, str(package.parent))
    from dwgcheck import r2004, structural
    if (Path(r2004.__file__).resolve() != (package / 'r2004.py').resolve()
            or Path(structural.__file__).resolve() != (package / 'structural.py').resolve()):
        raise ValueError('loaded verifier does not match the pinned checkout')
    return r2004, structural


def traced_decoder(r2004):
    code = inspect.getsource(r2004.decompress)
    replacements = (
        ('def decompress(', 'def traced('),
        ('    out = bytearray()\n', '    out = bytearray()\n    origins = []\n'),
        ('        out.extend(data[pos : pos + count])\n',
         '        out.extend(data[pos : pos + count])\n        origins.extend(range(pos, pos + count))\n'),
        ('            out.extend(chunk)\n',
         '            out.extend(chunk)\n            origins.extend(origins[start : start + len(chunk)])\n'),
        ('    return bytes(out)\n', '    return bytes(out), origins\n'),
    )
    for old, new in replacements:
        if code.count(old) != 1:
            raise ValueError('pinned decoder shape changed')
        code = code.replace(old, new)
    namespace = dict(r2004.__dict__)
    exec(code, namespace)  # Pinned source was verified before import.
    return namespace['traced']


def logical_header_size(data, layout, structural, r2004):
    page = next(p for p in layout.pages if p.number == layout.header.section_map_id)
    raw = structural._read_system_page(data, page.address, r2004.PAGE_TYPE_SECTION_MAP,
                                       'section map', 'T1.sectionmap')
    count = struct.unpack_from('<I', raw, 0)[0]
    pos, sizes = 0x14, []
    for _ in range(count):
        if pos + 0x60 > len(raw):
            raise ValueError('section map is truncated')
        size, pages = struct.unpack_from('<QI', raw, pos)
        name = raw[pos + 0x20:pos + 0x60].split(b'\0', 1)[0]
        pos += 0x60
        if pages > (len(raw) - pos) // 16:
            raise ValueError('section page list is truncated')
        pos += pages * 16
        if name == b'AcDb:Header':
            sizes.append(size)
    if pos != len(raw) or len(sizes) != 1:
        raise ValueError('HEADER section identity is ambiguous')
    return sizes[0]


def run(args):
    r2004, structural = load_verifier(args.verifier)
    probe_path = Path(__file__).resolve().parents[1] / 'mutate_r2004_header_suffix_probe.py'
    spec = importlib.util.spec_from_file_location('header_probe', probe_path)
    probe = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(probe)
    if args.source.stat().st_size > MAX_SOURCE or args.decoded_header.stat().st_size > MAX_WINDOW:
        raise ValueError('source or logical HEADER exceeds the experiment limit')
    drawing = bytearray(args.source.read_bytes())
    logical = args.decoded_header.read_bytes()
    if sha(drawing) != args.source_sha256.lower() or drawing[:6] != b'AC1024':
        raise ValueError('source hash or version does not match')
    result = structural.check_bytes(drawing)
    if result.findings or result.unsupported:
        raise ValueError('source failed the pinned independent container check')
    layout = structural.read_layout(drawing)
    sections = [s for s in layout.sections if s.name == 'AcDb:Header']
    if len(sections) != 1 or len(sections[0].pages) != 1 or sections[0].compressed != 2:
        raise ValueError('HEADER is not a single compressed page')
    section, page_ref = sections[0], sections[0].pages[0]
    if (page_ref.start_offset != 0 or section.max_page_size > MAX_WINDOW
            or logical_header_size(drawing, layout, structural, r2004) != len(logical)):
        raise ValueError('logical HEADER size or page window is invalid')
    pages = [p for p in layout.pages if p.number == page_ref.number]
    if len(pages) != 1:
        raise ValueError('HEADER physical page identity is ambiguous')
    page_at = pages[0].address
    payload_at, compressed = page_at + 32, page_ref.data_size
    if payload_at + compressed > len(drawing):
        raise ValueError('compressed page exceeds source')
    end = logical.find(probe.END)
    if (not logical.startswith(probe.BEGIN) or end < 22
            or logical.find(probe.END, end + 1) != -1):
        raise ValueError('HEADER sentinel mismatch')
    crc_pos = end - 2
    declared = struct.unpack_from('<I', logical, 16)[0]
    if not (20 + declared == crc_pos or
            (logical[20:24] == bytes(4) and 24 + declared == crc_pos)):
        raise ValueError('HEADER envelope size mismatch')
    if probe.crc16(logical[16:crc_pos]) != struct.unpack_from('<H', logical, crc_pos)[0]:
        raise ValueError('HEADER envelope CRC mismatch')
    suffix_at = end + len(probe.END)
    if suffix_at >= len(logical):
        raise ValueError('HEADER has no suffix')
    compressed_data = bytes(drawing[payload_at:payload_at + compressed])
    decoded, origins = traced_decoder(r2004)(compressed_data, section.max_page_size)
    if decoded[:len(logical)] != logical or len(origins) != len(decoded):
        raise ValueError('independent decompression does not match logical HEADER')
    roots = set(origins[suffix_at:len(logical)])
    if any(root in roots and not suffix_at <= index < len(logical)
           for index, root in enumerate(origins)):
        raise ValueError('a suffix literal also contributes outside the suffix')
    for root in roots:
        if not 0 <= root < compressed:
            raise ValueError('literal origin is out of bounds')
        at = payload_at + root
        drawing[at] = 0 if args.variant == 'zero' else drawing[at] ^ 0xA5
    mask = 0x4164536B ^ page_at
    header = bytearray().join(struct.pack('<I', struct.unpack_from('<I', drawing, page_at + i)[0] ^ mask)
                              for i in range(0, 32, 4))
    if struct.unpack_from('<I', header, 0)[0] != 0x4163043B:
        raise ValueError('unexpected page header')
    struct.pack_into('<I', header, 28, probe.checksum(drawing[payload_at:payload_at + compressed]))
    struct.pack_into('<I', header, 24, 0)
    struct.pack_into('<I', header, 24, probe.checksum(header, struct.unpack_from('<I', header, 28)[0]))
    for i in range(0, 32, 4):
        struct.pack_into('<I', drawing, page_at + i, struct.unpack_from('<I', header, i)[0] ^ mask)
    modified = r2004.decompress(drawing[payload_at:payload_at + compressed], section.max_page_size)
    expected = (bytes(len(logical) - suffix_at) if args.variant == 'zero'
                else bytes(x ^ 0xA5 for x in logical[suffix_at:]))
    if (modified[:suffix_at] != decoded[:suffix_at] or modified[suffix_at:len(logical)] != expected
            or modified[len(logical):] != decoded[len(logical):]):
        raise ValueError('mutation changed data outside the suffix')
    result = structural.check_bytes(drawing)
    if result.findings or result.unsupported:
        raise ValueError('mutated file failed the pinned container check')
    with args.output.open('xb') as output:
        output.write(drawing)
    print('suffix_bytes={} literal_roots={} sha256={}'.format(
        len(logical) - suffix_at, len(roots), sha(drawing)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('decoded_header', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--verifier', required=True, type=Path)
    parser.add_argument('--source-sha256', required=True)
    parser.add_argument('--variant', choices=('zero', 'xor'), required=True)
    args = parser.parse_args()
    try:
        run(args)
    except (OSError, ValueError, StopIteration, struct.error) as exc:
        parser.exit(1, 'refused: {}\n'.format(exc))


if __name__ == '__main__':
    main()
