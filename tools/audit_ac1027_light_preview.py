#!/usr/bin/env python3
"""Hash-bound selected LIGHT DXF preview and ODA return audit, not whole-scene qualification."""
import argparse
import hashlib
import json
from pathlib import Path


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    require(path.is_file() and path.stat().st_size < 64 * 1024 * 1024, 'missing or oversized artifact')
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    import ezdxf
    from ezdxf.lldxf.tagger import ascii_tags_loader, binary_tags_loader
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('manifest', type=Path)
    parser.add_argument('--sha256', required=True)
    args = parser.parse_args()
    require(ezdxf.__version__ == '1.4.5', 'unexpected ezdxf version')
    require(digest(args.manifest) == args.sha256, 'manifest digest mismatch')
    manifest = json.loads(args.manifest.read_text())
    require(set(manifest) == {'original', 'selected', 'ascii', 'binary',
                               'oda_ascii_dwg', 'oda_binary_dwg',
                               'oda_ascii_dxf', 'oda_binary_dxf'}, 'inventory mismatch')
    paths = {}
    for name, info in manifest.items():
        path = Path(info['path']).resolve(strict=True)
        require(digest(path) == info['sha256'], name + ' digest mismatch')
        paths[name] = path
    def record(path):
        raw = path.read_bytes()
        if raw.startswith(b'AutoCAD Binary DXF'):
            tags = list(binary_tags_loader(raw))
        else:
            with path.open() as stream:
                tags = list(ascii_tags_loader(stream))
        starts = [i for i, tag in enumerate(tags) if tag.code == 0 and tag.value == 'LIGHT']
        require(len(starts) == 1, 'expected one LIGHT')
        i = starts[0]
        j = next((j for j in range(i + 1, len(tags)) if tags[j].code == 0), len(tags))
        return tags[i:j]
    def values(tags, code):
        return [t.value for t in tags if t.code == code]
    def canon(v):
        return v if isinstance(v, bytes) else str(v).strip()
    original = record(paths['original'])
    selected = record(paths['selected'])
    require([(x.code, canon(x.value)) for x in original] ==
            [(x.code, canon(x.value)) for x in selected], 'selected record differs from source')
    for name in ('selected', 'ascii', 'binary', 'oda_ascii_dxf', 'oda_binary_dxf'):
        tags = record(paths[name])
        require([int(x) for x in values(tags, 160)] == [8], name + ': count')
        chunks = [v if isinstance(v, bytes) else bytes.fromhex(v) for v in values(tags, 310)]
        require(b''.join(chunks) == b'\x08\0\0\0\0\0\0\0', name + ': preview')
        require([int(x) for x in values(tags, 63)] == [148], name + ': ACI')
        require([int(x) for x in values(tags, 421)] == [1193046], name + ': RGB')
        doc = ezdxf.readfile(paths[name])
        lights = [e for e in doc.modelspace() if e.dxftype() == 'LIGHT']
        require(len(lights) == 1 and lights[0].dxf.name == 'LOCAL_LIGHT'
                and tuple(lights[0].dxf.location) == (1.0, 2.0, 3.0)
                and lights[0].dxf.intensity == 2.5, name + ': selected fields')
    for name, path in paths.items():
        require(digest(path) == manifest[name]['sha256'], name + ': changed while auditing')
    require(digest(args.manifest) == args.sha256, 'manifest changed while auditing')
    print('PASS: selected LIGHT record and ASCII/Binary preview with ODA returns')


if __name__ == '__main__':
    main()
