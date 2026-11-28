#!/usr/bin/env python3
"""Write hash-locked diagnostic source copies; never modifies the input tree."""
import argparse, hashlib
from pathlib import Path

EXPECTED = {
 'src/intern/dwgwriter15.cpp': '7fa54138180fdf4e3fb324054319e363477c406e8c8e82784460d3aed2a9f2d1',
 'src/intern/dwg_default_material.h': 'a09326dd4919da95fedca05dd71ca795f9151523f021c9dcd8e0e7b467e5d734',
 'src/drw_objects.cpp': '016679e106d9f5cc7619ad2c2ea421dd94148b8db4769f5515741223cd7cb5d3',
}

def replace_once(text, old, new):
    if text.count(old) != 1: raise ValueError('expected source fragment not unique')
    return text.replace(old, new)


def prepare(source, output):
    source=source.resolve(strict=True); output=output.resolve()
    if output == source or source in output.parents: raise ValueError('output must be outside source')
    if output.exists() and any(output.iterdir()): raise ValueError('output directory must be empty')
    content={}
    for name,expected in EXPECTED.items():
        data=(source/name).read_bytes()
        if hashlib.sha256(data).hexdigest()!=expected: raise ValueError('source hash mismatch: '+name)
        content[name]=data.decode('utf-8')
    p='src/intern/dwgwriter15.cpp';content[p]=replace_once(content[p],
       'if ((m_version != DRW::AC1021 && m_version != DRW::AC1024)\n        || m_dwgClassesFrozen',
       'if ((m_version != DRW::AC1021 && m_version != DRW::AC1024 && m_version != DRW::AC1027 && m_version != DRW::AC1032)\n        || m_dwgClassesFrozen')
    p='src/intern/dwg_default_material.h';content[p]=replace_once(content[p],
       'if ((version != DRW::AC1021 && version != DRW::AC1024)\n        || name',
       'if ((version != DRW::AC1021 && version != DRW::AC1024 && version != DRW::AC1027 && version != DRW::AC1032)\n        || name')
    p='src/drw_objects.cpp';content[p]=replace_once(content[p],
       '(version == DRW::AC1021 || version == DRW::AC1024)\n                && typedNullReferences',
       '(version == DRW::AC1021 || version == DRW::AC1024 || version == DRW::AC1027 || version == DRW::AC1032)\n                && typedNullReferences')
    content[p]=replace_once(content[p],
       '    if (version >= DRW::AC1027)\n        hb->putHandle(makeRefW(unknownHandle.ref, DRW::DwgHardPointer));',
       '    if (version >= DRW::AC1027) {\n        auto value = makeRefW(unknownHandle.ref, DRW::DwgHardPointer);\n        if (unknownHandle.ref == 0) value.code = DRW::DwgHardPointer;\n        hb->putHandle(value);\n    }')
    for name,text in content.items():
        path=output/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_text(text,encoding='utf-8')
    return len(content)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('source',type=Path);p.add_argument('output',type=Path);a=p.parse_args()
    print('prepared',prepare(a.source,a.output),'diagnostic source copies')
if __name__=='__main__': main()
