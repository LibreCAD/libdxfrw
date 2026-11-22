#!/usr/bin/env python3
"""Generate and check scratch-only legacy INSERT chains using LibreDWG 0.14.

The dwgadd block command misassigns its model/block graph. Repair that *generated*
JSON graph explicitly before serialization, then reopen it with the peer and
check candidate diagnostics. This is not native producer qualification.
"""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

RECIPE = '''block "TEST_BLOCK"
line (0 0 0) (1 1 0)
endblk
insert (2 0 0) "TEST_BLOCK" 1 1 1 0
insert (4 0 0) "TEST_BLOCK" 1 1 1 0
insert (6 0 0) "TEST_BLOCK" 1 1 1 0
line (8 0 0) (9 1 0)
dimstyle "TEST_DIMSTYLE"
'''
SENTINEL = b'legacy insert destination must survive refusal\n'


def require(condition, message):
    if not condition:
        raise ValueError(message)


def run(args, check=True):
    result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            timeout=60, check=False)
    if check and result.returncode:
        raise RuntimeError(f'{args[0]} failed: {result.stderr.decode(errors="replace")[-1500:]}')
    return result


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def graph(doc):
    objects = doc['OBJECTS']
    by_handle = {o['handle'][-1]: o for o in objects if 'handle' in o}
    require(len(by_handle) == sum('handle' in o for o in objects), 'duplicate source handles')
    def named(kind, name):
        matches = [o for o in objects if o.get('object') == kind and o.get('name') == name]
        require(len(matches) == 1, f'missing or duplicate {kind} {name}')
        return matches[0]
    def entity(kind, handle):
        obj = by_handle.get(handle, {})
        require(obj.get('entity') == kind, f'missing {kind} {handle:x}')
        return obj
    model = named('BLOCK_HEADER', '*MODEL_SPACE')
    block = named('BLOCK_HEADER', 'TEST_BLOCK')
    dimstyle = named('DIMSTYLE', 'Standard')
    block_begins = [o for o in objects if o.get('entity') == 'BLOCK']
    begins = {o.get('name'): o['handle'][-1] for o in block_begins}
    require(len(begins) == len(block_begins), 'duplicate block delimiters')
    require(model['handle'][-1] == 31 and block['handle'][-1] == 44
            and dimstyle['handle'][-1] == 52
            and begins.get('*MODEL_SPACE') == 36 and begins.get('TEST_BLOCK') == 45,
            'unexpected generated object identities')
    for kind, handle in [('ENDBLK', 37), ('ENDBLK', 47), ('LINE', 46),
                         ('INSERT', 48), ('INSERT', 49), ('INSERT', 50), ('LINE', 51)]:
        entity(kind, handle)
    for handle, x in [(48, 2), (49, 4), (50, 6)]:
        obj = by_handle[handle]
        require(obj['has_attribs'] == 0 and obj['ins_pt'] == [x, 0.0, 0.0]
                and obj['block_header'][-1] == 44, 'unexpected INSERT data')
    return by_handle


def fix_generated_graph(doc):
    objects = graph(doc)
    def ref(role, value):
        return [role, 1, value, value]
    for key, value in {'block_entity': 36, 'endblk_entity': 37,
                       'first_entity': 48, 'last_entity': 51}.items():
        objects[31][key] = ref(3 if key in ('block_entity', 'endblk_entity') else 4, value)
    for key, value in {'block_entity': 45, 'endblk_entity': 47,
                       'first_entity': 46, 'last_entity': 46}.items():
        objects[44][key] = ref(3 if key in ('block_entity', 'endblk_entity') else 4, value)
    objects[46]['next_entity'] = [4, 0, 0, 0]
    for handle in (45, 46, 47):
        objects[handle]['entmode'] = 0
        objects[handle]['ownerhandle'] = ref(4, 44)
    for handle in (48, 49, 50):
        objects[handle]['nolinks'] = 1
        objects[handle].pop('prev_entity', None)
        objects[handle].pop('next_entity', None)
    doc['HEADER']['DIMSTYLE'] = ref(5, 52)


def check_graph(doc):
    objects = graph(doc)
    for handle, expected in [(31, (36, 37, 48, 51)), (44, (45, 47, 46, 46))]:
        obj = objects[handle]
        actual = tuple(obj[k][-1] for k in ('block_entity', 'endblk_entity', 'first_entity', 'last_entity'))
        require(actual == expected, 'reopened block graph mismatch')
    for handle in (45, 46, 47):
        require(objects[handle]['entmode'] == 0 and objects[handle]['ownerhandle'][-1] == 44,
                'reopened block owner mismatch')
    for handle in (48, 49, 50):
        require(objects[handle]['nolinks'] == 1, 'INSERT must use implicit consecutive links')
    require(doc['HEADER']['DIMSTYLE'][-1] == 52, 'missing header DIMSTYLE')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--candidate', required=True)
    args = parser.parse_args()
    import ezdxf  # Independent DXF readback; install separately when needed.
    tools = {name: Path(shutil.which(name) or '').resolve() for name in ('dwgadd', 'dwgread', 'dwgwrite')}
    tools['candidate'] = Path(args.candidate).resolve()
    for name, path in tools.items():
        require(path.is_file(), f'missing executable {name}')
        if name != 'candidate':
            require(b'0.14' in run([str(path), '--version']).stdout, f'{name} must be LibreDWG 0.14')
    args.output_dir.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix='legacy-insert-chain-', dir=args.output_dir))
    recipe = root / 'source.dwgadd'
    recipe.write_text(RECIPE)
    result = {'schema': 1, 'scope': 'generated source chain; not native interoperability',
              'ezdxfVersion': ezdxf.__version__,
              'tools': {name: {'path': str(path), 'sha256': digest(path)} for name, path in tools.items()},
              'samples': {}}
    for version, magic in (('r13', 'AC1012'), ('r14', 'AC1014'), ('r2000', 'AC1015')):
        source, decoded = root / (version + '-initial.dwg'), root / (version + '-initial.json')
        run([str(tools['dwgadd']), '--as', version, '-o', str(source), str(recipe)])
        run([str(tools['dwgread']), '-O', 'JSON', '-o', str(decoded), str(source)])
        doc = json.loads(decoded.read_text())
        fix_generated_graph(doc)
        corrected_json, corrected_dwg = root / (version + '-corrected.json'), root / (version + '-corrected.dwg')
        corrected_json.write_text(json.dumps(doc))
        run([str(tools['dwgwrite']), '-I', 'JSON', '--as', version, '-y', '-o', str(corrected_dwg), str(corrected_json)])
        require(corrected_dwg.read_bytes()[:6] == magic.encode(), 'wrong version')
        reopened = root / (version + '-reopened.json')
        run([str(tools['dwgread']), '-O', 'JSON', '-o', str(reopened), str(corrected_dwg)])
        check_graph(json.loads(reopened.read_text()))
        observations = []
        for partial in (False, True):
            for binary in (False, True):
                label = ('partial' if partial else 'strict') + ('-binary' if binary else '-ascii')
                output = root / (version + '-' + label + '.dxf')
                output.write_bytes(SENTINEL)
                cmd = [str(tools['candidate']), str(corrected_dwg), '-y', '-d', '-o', str(output)]
                if partial: cmd.append('--allow-partial-dwg-read')
                if binary: cmd.append('-b')
                candidate = run(cmd, check=False)
                trace = (candidate.stdout + candidate.stderr).decode(errors='replace')
                (root / (version + '-' + label + '.log')).write_text(trace)
                require('operationSucceeded=true' in trace and 'entityParseFailures=0 objectParseFailures=0' in trace,
                        f'{version} {label} did not read cleanly')
                insert_sections = trace.split('parsing insert/minsert')
                require(len(insert_sections) == 4, f'{version} {label}: unexpected INSERT parse count')
                for section in insert_sections[:-1]:
                    marker = 'haveNextLinks (0 yes, 1 prev next): '
                    require(marker in section and section.rsplit(marker, 1)[1].startswith('1'),
                            f'{version} {label}: INSERT did not use implicit links')
                for x in (2, 4, 6):
                    require(trace.count(f'insertion point: x: {x}.000000, y: 0.000000, z: 0.000000') == 1,
                            f'{version} {label} INSERT {x} not visited once')
                require('0x33 [bidi-debug pre-graphFlag' in trace, f'{version} {label} following LINE missing')
                accepted = version == 'r13' and partial
                require((candidate.returncode == 0) == accepted,
                        f'{version} {label}: unexpected admission result')
                observation = {'mode': label, 'exit': candidate.returncode,
                               'traceSha256': digest(root / (version + '-' + label + '.log'))}
                if accepted:
                    drawing = ezdxf.readfile(output)
                    entities = list(drawing.modelspace())
                    require([e.dxftype() for e in entities] == ['INSERT', 'INSERT', 'INSERT', 'LINE'],
                            f'{version} {label}: independent modelspace readback mismatch')
                    require([tuple(e.dxf.insert) for e in entities[:3]]
                            == [(2.0, 0.0, 0.0), (4.0, 0.0, 0.0), (6.0, 0.0, 0.0)]
                            and all(e.dxf.name == 'TEST_BLOCK' for e in entities[:3])
                            and tuple(entities[3].dxf.start) == (8.0, 0.0, 0.0)
                            and tuple(entities[3].dxf.end) == (9.0, 1.0, 0.0)
                            and len(drawing.blocks['TEST_BLOCK']) == 1,
                            f'{version} {label}: independent geometry or block readback mismatch')
                    block_line = list(drawing.blocks['TEST_BLOCK'])[0]
                    require(block_line.dxftype() == 'LINE'
                            and tuple(block_line.dxf.start) == (0.0, 0.0, 0.0)
                            and tuple(block_line.dxf.end) == (1.0, 1.0, 0.0),
                            f'{version} {label}: block geometry mismatch')
                    observation['outputSha256'] = digest(output)
                    observation['independentReadback'] = 'three INSERTs and following LINE, positions and target block'
                else:
                    require(output.read_bytes() == SENTINEL,
                            f'{version} {label} did not preserve destination on refusal')
                    observation['destinationPreserved'] = True
                observations.append(observation)
        result['samples'][version] = {'version': magic, 'dwgSha256': digest(corrected_dwg),
                                      'peerReopenSha256': digest(reopened), 'checks': observations}
    receipt = root / 'receipt.json'
    receipt.write_text(json.dumps(result, indent=2) + '\n')
    print(root)
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
