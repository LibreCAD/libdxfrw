#!/usr/bin/env python3
"""Create disposable ezdxf wide/narrow dictionary controls and optionally probe a candidate.

Requires ezdxf 1.4.5. The output directory must not exist or be in the checkout.
The probe is observational: a failure or successful parse is not a support claim.
"""
import argparse
import hashlib
import json
import subprocess
from pathlib import Path

import ezdxf


ROOT = Path(__file__).resolve().parents[2]


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--candidate', type=Path)
    args = parser.parse_args()
    output = args.output_dir.resolve()
    if output == ROOT or ROOT in output.parents:
        parser.error('output must be outside the repository')
    if ezdxf.__version__ != '1.4.5':
        parser.error('requires ezdxf 1.4.5')
    candidate = args.candidate.resolve() if args.candidate else None
    if candidate is not None and not candidate.is_file():
        parser.error('candidate must be an executable file')
    output.mkdir(parents=True, exist_ok=False)

    doc = ezdxf.new('R2013')
    dictionary = doc.rootdict.add_new_dict('CODEX_GRAPH', hard_owned=True)
    record = dictionary.add_xrecord('CODEX_RECORD')
    record.reset([(1, 'locally authored graph'), (340, dictionary.dxf.handle)])
    # Complete ezdxf's normal setup before assigning wide handles: otherwise
    # newly created APPID records confound the graph-specific input boundary.
    doc.saveas(output / 'warmup.dxf')
    rows = []
    for profile in ('narrow', 'wide'):
        if profile == 'wide':
            if not (doc.entitydb.reset_handle(dictionary, '100000010')
                    and doc.entitydb.reset_handle(record, '100000011')):
                raise RuntimeError('wide handle collision')
            record.dxf.owner = dictionary.dxf.handle
            record.reset([(1, 'locally authored graph'), (340, dictionary.dxf.handle)])
            doc.entitydb.handles.reset('100000012')
        for encoding in ('asc', 'bin'):
            source = output / (profile + '-' + encoding + '.dxf')
            doc.saveas(source, fmt=encoding)
            peer = ezdxf.readfile(source)
            peer_dictionary = peer.rootdict['CODEX_GRAPH']
            peer_record = peer_dictionary['CODEX_RECORD']
            audit = peer.audit()
            if audit.errors or audit.fixes:
                raise RuntimeError('peer audit reported errors or fixes')
            if (peer_record.dxf.owner != peer_dictionary.dxf.handle
                    or (340, peer_dictionary.dxf.handle)
                    not in [(tag.code, tag.value) for tag in peer_record.tags]):
                raise RuntimeError('peer graph changed on reopen')
            row = {'profile': profile, 'encoding': encoding,
                   'sourceSha256': sha256(source),
                   'dictionaryHandle': peer_dictionary.dxf.handle,
                   'dictionaryOwner': peer_dictionary.dxf.owner,
                   'recordHandle': peer_record.dxf.handle,
                   'recordOwner': peer_record.dxf.owner,
                   'reference340': peer_dictionary.dxf.handle,
                   'handseed': peer.header['$HANDSEED'],
                   'peerAuditErrors': 0, 'peerAuditFixes': 0}
            if candidate is not None:
                destination = output / ('candidate-' + profile + '-' + encoding + '.dxf')
                sentinel = b'WIDE-GRAPH-DESTINATION-SENTINEL\n'
                destination.write_bytes(sentinel)
                run = subprocess.run([str(candidate), str(source), '-d', '-y', '-v2013', str(destination)],
                                     stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                     timeout=120, check=False)
                log = output / ('candidate-' + profile + '-' + encoding + '.log')
                log.write_bytes(run.stdout)
                row['candidate'] = {'exitCode': run.returncode,
                                    'destinationPreserved': destination.read_bytes() == sentinel,
                                    'logSha256': sha256(log)}
            rows.append(row)
    result = {'ezdxfVersion': ezdxf.__version__, 'candidateSha256': sha256(candidate) if candidate else None,
              'controls': rows}
    (output / 'receipt.json').write_text(json.dumps(result, indent=2) + '\n')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
