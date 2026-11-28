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
import re
import shutil
import struct
import subprocess
import tempfile

from generate_ac1012_dictionary_witness import Bits, crc16

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


def source_style_angles(dwgread, source):
    # LibreDWG JSON rounds doubles; recover the original two BD values from
    # the CRC-checked R13/R14/R2000 frame using ODA v5.4.1 sections 20.1/20.4.73.
    result = run([str(dwgread), '-v4', str(source)])
    trace = (result.stdout + result.stderr).decode(errors='replace')
    matches = re.findall(r'Size: (\d+) \[MS\], Type: 73 \[BS\], Address: (\d+)', trace)
    require(len(matches) == 1, 'expected one peer-identified MLINESTYLE frame')
    size, address = map(int, matches[0])
    data = source.read_bytes()
    require(size < 0x8000 and address >= 2 and address + size + 2 <= len(data)
            and data[address-2:address] == size.to_bytes(2, 'little')
            and int.from_bytes(data[address+size:address+size+2], 'little')
                == crc16(data[address-2:address+size]), 'invalid MLINESTYLE frame or CRC')
    bits = Bits(data[address:address+size])
    require(bits.bs() == 73, 'wrong style type')
    boundary = bits.little(4) if data.startswith(b'AC1015') else None
    require(bits.handle(0, own=True)['value'] == 24 and bits.bs() == 0, 'style header mismatch')
    if boundary is None: boundary = bits.little(4)
    require(bits.bl() == 1, 'style reactor count mismatch')
    def text():
        return bytes(bits.take(8) for _ in range(bits.bs()))
    require(text() in (b'Standard', b'Standard\0') and text() == b''
            and bits.bs() == 0 and bits.bs() == 256, 'style fields mismatch')
    def double():
        tag = bits.take(2)
        if tag == 0: return struct.unpack('<d', bytes(bits.take(8) for _ in range(8)))[0]
        require(tag in (1, 2), 'invalid bit-double')
        return 1.0 if tag == 1 else 0.0
    values = [double(), double()]
    require(bits.position <= boundary and values == [1.5707963267948966] * 2,
            'unexpected original style angles')
    return values


def fix_generated_graph(doc, angles):
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
    styles = [o for o in doc['OBJECTS'] if o.get('object') == 'MLINESTYLE']
    require(len(styles) == 1, 'expected one generated MLINESTYLE')
    styles[0]['start_angle'], styles[0]['end_angle'] = angles


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


def check_output(ezdxf, output, label):
    drawing = ezdxf.readfile(output)
    entities = list(drawing.modelspace())
    require([e.dxftype() for e in entities] == ['INSERT', 'INSERT', 'INSERT', 'LINE'],
            f'{label}: independent modelspace readback mismatch')
    require([tuple(e.dxf.insert) for e in entities[:3]]
            == [(2.0, 0.0, 0.0), (4.0, 0.0, 0.0), (6.0, 0.0, 0.0)]
            and all(e.dxf.name == 'TEST_BLOCK' for e in entities[:3])
            and tuple(entities[3].dxf.start) == (8.0, 0.0, 0.0)
            and tuple(entities[3].dxf.end) == (9.0, 1.0, 0.0)
            and len(drawing.blocks['TEST_BLOCK']) == 1,
            f'{label}: independent geometry or block readback mismatch')
    block_line = list(drawing.blocks['TEST_BLOCK'])[0]
    require(block_line.dxftype() == 'LINE'
            and tuple(block_line.dxf.start) == (0.0, 0.0, 0.0)
            and tuple(block_line.dxf.end) == (1.0, 1.0, 0.0),
            f'{label}: block geometry mismatch')


def check_r2000_plotstyle(ezdxf, output, label):
    drawing = ezdxf.readfile(output)
    root = drawing.rootdict
    dictionary = root['ACAD_PLOTSTYLENAME']
    placeholder = dictionary['Normal']
    # Also bind the original physical handles: ezdxf can otherwise replace
    # an unrecognized alias with a new default graph while opening the file.
    require(root.dxf.handle == 'C' and dictionary.dxftype() == 'ACDBDICTIONARYWDFLT'
            and ('ACDBDICTIONARYWDFLT', 'AcDbDictionaryWithDefault') in drawing.classes.classes
            and not any(key[0] == 'DICTIONARYWDFLT' for key in drawing.classes.classes)
            and dictionary.dxf.handle == 'E' and dictionary.dxf.owner == 'C'
            and dictionary.dxf.default == 'F' and dictionary.dxf.cloning == 1
            and dictionary.dxf.hard_owned == 0 and dictionary.get_reactors() == ['C']
            and placeholder.dxftype() == 'ACDBPLACEHOLDER'
            and placeholder.dxf.handle == 'F' and placeholder.dxf.owner == 'E'
            and placeholder.get_reactors() == ['E'], f'{label}: plot-style graph changed')


def check_r2000_layout_and_style(ezdxf, output, label):
    drawing = ezdxf.readfile(output)
    root = drawing.rootdict
    layouts = root['ACAD_LAYOUT']
    require(layouts.dxf.handle == '1A' and layouts.dxf.owner == 'C'
            and set(layouts.keys()) == {'Model', 'Layout1'}, f'{label}: layout dictionary changed')
    for name, handle, block in [('Model', '27', '1F'), ('Layout1', '29', '1E')]:
        layout = layouts[name]
        record = drawing.entitydb[block]
        require(layout.dxftype() == 'LAYOUT' and layout.dxf.handle == handle
                and layout.dxf.owner == '1A' and layout.get_reactors() == ['1A']
                and layout.dxf.block_record_handle == block
                and record.dxftype() == 'BLOCK_RECORD' and record.dxf.layout == handle,
                f'{label}: {name} layout or block backlink changed')
    styles = root['ACAD_MLINESTYLE']
    style = styles['Standard']
    require(styles.dxf.handle == '17' and styles.dxf.owner == 'C'
            and style.dxftype() == 'MLINESTYLE' and style.dxf.handle == '18'
            and style.dxf.owner == '17' and style.get_reactors() == ['17']
            and style.dxf.start_angle == 90 and style.dxf.end_angle == 90
            and [(e.offset, e.color, e.linetype) for e in style.elements]
                == [(0.5, 256, 'BYLAYER'), (-0.5, 256, 'BYLAYER')],
            f'{label}: MLINESTYLE graph or fields changed')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--candidate', required=True)
    parser.add_argument('--adapter-test', help='optional generated INSERT same-adapter retry test executable')
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
        initial_style = next(o for o in doc['OBJECTS'] if o.get('object') == 'MLINESTYLE')
        rounded_angles = [initial_style['start_angle'], initial_style['end_angle']]
        exact_angles = source_style_angles(tools['dwgread'], source)
        fix_generated_graph(doc, exact_angles)
        corrected_json, corrected_dwg = root / (version + '-corrected.json'), root / (version + '-corrected.dwg')
        corrected_json.write_text(json.dumps(doc))
        run([str(tools['dwgwrite']), '-I', 'JSON', '--as', version, '-y', '-o', str(corrected_dwg), str(corrected_json)])
        require(corrected_dwg.read_bytes()[:6] == magic.encode(), 'wrong version')
        reopened = root / (version + '-reopened.json')
        run([str(tools['dwgread']), '-O', 'JSON', '-o', str(reopened), str(corrected_dwg)])
        check_graph(json.loads(reopened.read_text()))
        observations = []
        if version == 'r14':
            # Verify the source-wire gate distinguishes the rounded JSON
            # representation, then retry the original exact source below.
            require(rounded_angles != exact_angles, 'peer JSON no longer demonstrates rounding')
            altered = json.loads(json.dumps(doc))
            altered_style = next(o for o in altered['OBJECTS'] if o.get('object') == 'MLINESTYLE')
            altered_style['start_angle'], altered_style['end_angle'] = rounded_angles
            altered_json, altered_dwg = root / 'r14-rounded.json', root / 'r14-rounded.dwg'
            altered_json.write_text(json.dumps(altered))
            run([str(tools['dwgwrite']), '-I', 'JSON', '--as', version, '-y', '-o', str(altered_dwg), str(altered_json)])
            for binary in (False, True):
                label = 'rounded-binary' if binary else 'rounded-ascii'
                output = root / (label + '.dxf')
                output.write_bytes(SENTINEL)
                cmd = [str(tools['candidate']), str(altered_dwg), '-y', '-d', '-o', str(output)]
                if binary: cmd.append('-b')
                candidate = run(cmd, check=False)
                trace = (candidate.stdout + candidate.stderr).decode(errors='replace')
                require(candidate.returncode != 0 and output.read_bytes() == SENTINEL
                        and 'MLINESTYLE DWG source/output snapshot' in trace,
                        f'{label}: changed physical source must be refused')
                observations.append({'mode': label, 'exit': candidate.returncode,
                                     'destinationPreserved': True,
                                     'sourceSha256': digest(altered_dwg)})
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
                accepted = (version == 'r13' and partial) or version in ('r14', 'r2000')
                require((candidate.returncode == 0) == accepted,
                        f'{version} {label}: unexpected admission result')
                observation = {'mode': label, 'exit': candidate.returncode,
                               'traceSha256': digest(root / (version + '-' + label + '.log'))}
                if accepted:
                    check_output(ezdxf, output, f'{version} {label}')
                    if version == 'r2000':
                        check_r2000_plotstyle(ezdxf, output, f'{version} {label}')
                        check_r2000_layout_and_style(ezdxf, output, f'{version} {label}')
                    observation['outputSha256'] = digest(output)
                    observation['independentReadback'] = 'three INSERTs and following LINE, positions and target block'
                else:
                    require(output.read_bytes() == SENTINEL,
                            f'{version} {label} did not preserve destination on refusal')
                    observation['destinationPreserved'] = True
                observations.append(observation)
        if version == 'r2000':
            for partial in (False, True):
                for binary in (False, True):
                    label = ('partial' if partial else 'strict') + ('-r2010-binary' if binary else '-r2010-ascii')
                    output = root / (version + '-' + label + '.dxf')
                    cmd = [str(tools['candidate']), str(corrected_dwg), '-y', '-v2010', '-o', str(output)]
                    if partial: cmd.append('--allow-partial-dwg-read')
                    if binary: cmd.append('-b')
                    run(cmd)
                    check_output(ezdxf, output, label)
                    check_r2000_plotstyle(ezdxf, output, label)
                    check_r2000_layout_and_style(ezdxf, output, label)
                    observations.append({'mode': label, 'exit': 0, 'outputSha256': digest(output),
                                         'independentReadback': 'canonical plot-style graph and geometry'})
        result['samples'][version] = {'version': magic, 'dwgSha256': digest(corrected_dwg),
                                      'peerReopenSha256': digest(reopened), 'checks': observations}
        if version == 'r14' and args.adapter_test:
            adapter_test = Path(args.adapter_test).resolve()
            require(adapter_test.is_file(), 'missing adapter retry test')
            output = root / 'r14-adapter-retry.dxf'
            run([str(adapter_test), '--generated-insert-retry', str(corrected_dwg), str(output)])
            check_output(ezdxf, output, 'R14 same-adapter retry')
            result['samples'][version]['adapterRetry'] = {
                'testSha256': digest(adapter_test), 'outputSha256': digest(output),
                'result': 'mutation refused and destination preserved; same-adapter ASCII and Binary retry passed'}
        if version == 'r2000' and args.adapter_test:
            adapter_test = Path(args.adapter_test).resolve()
            output = root / 'r2000-adapter-retry.dxf'
            run([str(adapter_test), '--generated-r2000-default-retry', str(corrected_dwg), str(output)])
            check_output(ezdxf, output, 'R2000 same-adapter retry')
            check_r2000_plotstyle(ezdxf, output, 'R2000 same-adapter retry')
            check_r2000_layout_and_style(ezdxf, output, 'R2000 same-adapter retry')
            result['samples'][version]['adapterRetry'] = {
                'testSha256': digest(adapter_test), 'outputSha256': digest(output),
                'result': 'wrong reactor, linetype, or layout owner refused for both targets; changed source style bytes or angle refused for R2000; destination preserved and same-adapter strict ASCII/Binary retry passed'}
    receipt = root / 'receipt.json'
    receipt.write_text(json.dumps(result, indent=2) + '\n')
    print(root)
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
