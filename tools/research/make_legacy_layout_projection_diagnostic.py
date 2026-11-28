#!/usr/bin/env python3
"""Create an explicitly unsafe scratch adapter for legacy projection research.

The generated file bypasses the legacy target refusal and three empty-family
CLASS guards only for source-bound zero-count evidence. It must never be used
for releases or support claims. The input source is not modified.
"""
import argparse
import hashlib
import json
from pathlib import Path


def digest(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--sha256', required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--inject-table-proof-failure', action='store_true',
                        help='add an environment-controlled staged TABLES mismatch')
    args = parser.parse_args()
    source = args.source.resolve(strict=True)
    output = args.output.resolve()
    repo = Path(__file__).resolve().parents[2]
    if output == repo or repo in output.parents:
        raise ValueError('diagnostic output must be outside the repository')
    if not output.parent.is_dir() or any(
            path.suffix.lower() in ('.h', '.hpp', '.hxx')
            for path in output.parent.iterdir()):
        raise ValueError('use an existing isolated output directory without headers')
    data = source.read_bytes()
    if digest(data) != args.sha256:
        raise ValueError('adapter source hash mismatch')
    text = data.decode('utf-8')
    old = ('    if (legacyForward)\n'
           '        return reject("R13/R14 DWG LAYOUT forward references need a qualified target projection");')
    new = ('    // UNSAFE SCRATCH DIAGNOSTIC: allow target projection study only.\n'
           '    (void)legacyForward;')
    if text.count(old) != 1:
        raise ValueError('legacy projection guard changed or is ambiguous')
    text = text.replace(old, new)
    ranges = [('prepareDwgGeoDataOutput', 'prepareDwgRasterVariablesOutput'),
              ('prepareDwgRasterVariablesOutput', 'prepareDwgWipeoutVariablesOutput'),
              ('prepareDwgWipeoutVariablesOutput', 'fileExport')]
    anchor = '    if (sourceClass->m_state != DRW_DwgClassCoverageState::Published\n'
    inject = ('    // UNSAFE SCRATCH DIAGNOSTIC: selected empty legacy class only.\n'
              '    std::size_t diagnosticCount = 0u;\n'
              '    if (typedCount == 0u && sourceFrameCount == 0u\n'
              '        && cData->legacyClassFrameCount(sourceClass->m_classNumber, diagnosticCount)\n'
              '        && diagnosticCount == 0u) return true;\n')
    for name, following in ranges:
        begin = text.index('bool dx_iface::' + name + '(')
        end = text.index('bool dx_iface::' + following + '(', begin)
        section = text[begin:end]
        if section.count(anchor) != 1:
            raise ValueError(name + ' CLASS guard changed or is ambiguous')
        text = text[:begin] + section.replace(anchor, inject + anchor) + text[end:]
    if args.inject_table_proof_failure:
        anchor = '        return dxfW->inspectDxfLayoutBlockRecords(view, links)\n'
        if text.count(anchor) != 1 or text.count('#include <iostream>\n') != 1:
            raise ValueError('legacy staged TABLES proof changed or is ambiguous')
        text = text.replace('#include <iostream>\n', '#include <iostream>\n#include <cstdlib>\n', 1)
        text = text.replace(anchor,
            '        // UNSAFE SCRATCH DIAGNOSTIC: force a staged proof mismatch.\n'
            '        if (std::getenv("LIBDXFRW_DIAGNOSTIC_BAD_LAYOUT") && !links.empty())\n'
            '            ++links.front().layout;\n' + anchor, 1)
    result = text.encode('utf-8')
    with output.open('xb') as stream:
        stream.write(result)
    print(json.dumps({'sourceSha256': digest(data), 'diagnosticSha256': digest(result),
                      'scope': 'UNSAFE scratch-only guard-bypass diagnostic',
                      'supportClaimEffect': 'none'}, sort_keys=True))


if __name__ == '__main__':
    main()
