#!/usr/bin/env python3
"""Create two source-bound diagnostic writer variants in an empty scratch directory."""
import argparse
import hashlib
from pathlib import Path


def transform(path, expected, before, after, target):
    data=path.read_bytes()
    if hashlib.sha256(data).hexdigest()!=expected:
        raise ValueError('unexpected source SHA-256: '+str(path))
    source=data.decode('utf-8')
    if source.count(before)!=1:
        raise ValueError('expected one precise hunk: '+str(path))
    if target.exists():
        raise ValueError('refusing to replace '+str(target))
    target.write_text(source.replace(before,after))


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('repo',type=Path,help='clean ac762fce snapshot')
    p.add_argument('output',type=Path,help='empty scratch directory')
    a=p.parse_args();a.output.mkdir(parents=True,exist_ok=True)
    if any(a.output.iterdir()): raise ValueError('output directory must be empty')
    old15=('    if (!plannedChild && !recordBlockOwnedEntity(*ent, &checkpoint.blockOwnerHandle))\n'
           '        return fail();\n    checkpoint.entityHandle = ent->handle;\n'
           '    checkpoint.blockOwnedEntityRecorded = !plannedChild;')
    new15=('    const bool testFiltered = m_version == DRW::AC1018 && ent->space == DRW::ModelSpace\n'
           '        && ent->parentHandle != DRW::NoHandle\n'
           '        && (dynamic_cast<DRW_Vertex*>(ent) || dynamic_cast<DRW_SeqEnd*>(ent));\n'
           '    if (!plannedChild && !testFiltered && !recordBlockOwnedEntity(*ent, &checkpoint.blockOwnerHandle))\n'
           '        return fail();\n    checkpoint.entityHandle = ent->handle;\n'
           '    checkpoint.blockOwnedEntityRecorded = !plannedChild && !testFiltered;')
    old24=('    const bool polylineChild = m_version == DRW::AC1021 && ent != nullptr\n'
           '        && m_ac1021PolylineChildPermit == ent\n'
           '        && (dynamic_cast<DRW_Vertex*>(ent) != nullptr\n'
           '            || dynamic_cast<DRW_SeqEnd*>(ent) != nullptr);')
    new24=('    const bool polylineChild = ent != nullptr\n'
           '        && ((m_version == DRW::AC1021 && m_ac1021PolylineChildPermit == ent)\n'
           '            || (m_version == DRW::AC1024 && ent->space == DRW::ModelSpace\n'
           '                && ent->parentHandle != DRW::NoHandle))\n'
           '        && (dynamic_cast<DRW_Vertex*>(ent) != nullptr\n'
           '            || dynamic_cast<DRW_SeqEnd*>(ent) != nullptr);')
    transform(a.repo/'src/intern/dwgwriter15.cpp','25cf9d24b5caf879ca8baab6e3900e8292ca6237031a2c61549c0eb30a897d81',
              old15,new15,a.output/'dwgwriter15.cpp')
    transform(a.repo/'src/intern/dwgwriter24.cpp','636b77180783a9e2995cf9458853f78610201d178dd7d27435bf9b8696064f16',
              old24,new24,a.output/'dwgwriter24.cpp')


if __name__=='__main__':main()
