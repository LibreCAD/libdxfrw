#!/usr/bin/env python3
"""Hash-bound, opt-in AC1027 AcDs/entity/ODA comparison; no fixtures bundled."""
import argparse
import hashlib
import subprocess
import sys
from pathlib import Path

SOURCE_SHA = 'ce80953e4881899cde8c42b97919feb2697259554ca54dc4005b19068a12c18f'
SECTION_SHA = '6447fe7ae583c89ef0ef4cd28446848968363b600fce7012df8f1f391d078ee8'
MAX_BYTES = 20 * 1024 * 1024
EXPECTED = {0x176: ('REGION', 1709, 0), 0x2E1: ('3DSOLID', 8798, 0x2E4),
            0x37D: ('REGION', 2196, 0)}
EXPECTED_HISTORY_FIELD = (3, 3, 2, 0x2E4, 446, 24)
EXPECTED_HISTORY_OBJECT = (0x2E1, 0x2E3, 'ACSH_HISTORY_CLASS', 1, 0, 0)
EXPECTED_SWEEP_OBJECT = (0x2E3, 0, 'ACSH_SWEEP_CLASS', 0, 1, 1)
EXPECTED_EVALUATION_GRAPH = (0x2E4, 1)
EXPECTED_EVALUATION_NODE = (32, 1, 0x2E2)


def require(ok, why):
    if not ok:
        raise ValueError(why)


def checked(path, digest):
    require(path.is_file() and path.stat().st_size <= MAX_BYTES, f'bad input: {path}')
    data = path.read_bytes()
    require(hashlib.sha256(data).hexdigest() == digest.lower(), f'hash mismatch: {path}')
    return data


class Bits:
    def __init__(self, data):
        self.bits = ''.join(f'{b:08b}' for b in data)
        self.pos = 0

    def take(self, n):
        require(0 <= n <= 32 and self.pos + n <= len(self.bits), 'truncated entity frame')
        value = int(self.bits[self.pos:self.pos+n], 2) if n else 0
        self.pos += n
        return value

    def octets(self, n):
        require(0 <= n <= 65536 and self.pos + n*8 <= len(self.bits), 'entity byte bound')
        return bytes(self.take(8) for _ in range(n))

    def short(self):
        tag = self.take(2)
        return (int.from_bytes(self.octets(2), 'little') if tag == 0 else
                self.take(8) if tag == 1 else 0 if tag == 2 else 256)

    def long(self):
        tag = self.take(2)
        return (int.from_bytes(self.octets(4), 'little') if tag == 0 else
                self.take(8) if tag == 1 else 0)

    def handle(self):
        code, count = self.take(4), self.take(4)
        require(count <= 4, 'handle width outside profile')
        return code, int.from_bytes(self.octets(count), 'big')


def entity_prefix(frame, identity, kind):
    b = Bits(frame)
    tag = b.take(2)
    dtype = b.take(8) if tag == 0 else 0x1F0 + b.take(8) if tag == 1 else int.from_bytes(b.octets(2), 'little')
    require(dtype == (38 if kind == '3DSOLID' else 37), 'source entity type mismatch')
    require(b.handle() == (0, identity), 'source entity identity mismatch')
    for _ in range(128):
        length = b.short()
        if length == 0:
            break
        b.handle()
        b.octets(length)
    else:
        raise ValueError('too many EED chunks')
    graph_at = b.pos
    require(b.take(1) == 0, 'proxy graphics outside this profile')
    mode_at = b.pos
    require(b.take(2) == 2, 'entity is not explicitly in model space')
    reactors = b.long()
    require(reactors <= 128 and b.take(1) == 1 and b.take(1) == 1,
            'unexpected reactor/extension/data-storage prefix')
    return graph_at, mode_at, reactors


def history_field(row):
    require(len(row) == 11, 'invalid history-field row')
    identity = int(row[1], 16)
    present, raw_code, code, size = map(int, row[2:6])
    reference = int(row[6], 16)
    start, bits, version, complete = map(int, row[7:11])
    require(present in (0, 1), 'invalid history-field presence flag')
    if not present:
        require((raw_code, code, size, reference, start, bits, complete)
                == (0, 0, 0, 0, 0, 0, 0),
                'absent history field carries stale metadata')
        return identity, None
    require(raw_code == code and size <= 8 and bits == 8 + 8 * size
            and complete == 1 and version != 0,
            'history H role/value span is malformed or truncated')
    return identity, (raw_code, code, size, reference, start, bits)


def history_graph_matches(acsh, graphs, nodes):
    return (acsh.get(0x2E4) == EXPECTED_HISTORY_OBJECT
            and acsh.get(0x2E2) == EXPECTED_SWEEP_OBJECT
            and graphs.get(0x2E3) == EXPECTED_EVALUATION_GRAPH
            and nodes.get((0x2E3, 0)) == EXPECTED_EVALUATION_NODE)


def dxf_records(data):
    lines = data.decode('utf-8', errors='strict').splitlines()
    require(len(lines) % 2 == 0, 'odd DXF group stream')
    section, record, rows = None, [], []
    for index in range(0, len(lines), 2):
        code, value = int(lines[index].strip()), lines[index+1].strip()
        if code == 0:
            if record:
                rows.append((section, record))
            if value == 'ENDSEC':
                section = None
            record = [(code, value)]
        elif code == 2 and record == [(0, 'SECTION')]:
            section = value
            record.append((code, value))
        else:
            record.append((code, value))
    return rows


def one(record, code):
    values = [v for c, v in record if c == code]
    require(len(values) == 1, f'expected one DXF group {code}')
    return values[0]


def dxf_projection(data):
    entities, acds, schema = {}, {}, None
    for section, r in dxf_records(data):
        kind = r[0][1]
        if section == 'ENTITIES' and kind in ('REGION', '3DSOLID', 'BODY'):
            identity = int(one(r, 5), 16)
            require(identity not in entities, 'duplicate modeler entity')
            depth = 0
            owners = []
            for c, v in r:
                if c == 102 and v.startswith('{'): depth += 1
                elif c == 102 and v == '}': depth -= 1
                elif c == 330 and depth == 0: owners.append(int(v, 16))
                require(depth >= 0, 'bad entity application-data braces')
            require(depth == 0 and owners == [0x1F]
                    and (100, 'AcDbModelerGeometry') in r, 'entity owner/class mismatch')
            hist = [int(v, 16) for c, v in r if c == 350]
            entities[identity] = (kind, hist[0] if hist else 0)
        elif (section == 'ACDSDATA' and kind == 'ACDSSCHEMA'
              and next((v for c, v in r if c == 90), None) == '1'):
            require(schema is None, 'duplicate AcDs schema 1')
            schema = (next((v for c, v in r if c == 1), None),
                      [v for c, v in r if c == 2][:2])
        elif section == 'ACDSDATA' and kind == 'ACDSRECORD':
            identity = int(one(r, 320), 16)
            require(identity not in acds, 'duplicate AcDs DXF handle')
            payload = b''.join(bytes.fromhex(v) for c, v in r if c == 310)
            require(int(one(r, 94)) == len(payload), 'AcDs DXF payload size mismatch')
            acds[identity] = (int(one(r, 90)), payload)
    require(schema == ('AcDb3DSolid_ASM_Data', ['AcDbDs::ID', 'ASM_Data']),
            'AcDs schema mismatch')
    return entities, acds


def compare(source, oda, probe):
    run = subprocess.run([str(probe), str(source)], capture_output=True, timeout=60, check=True)
    require(len(run.stdout) <= MAX_BYTES, 'probe output exceeds bound')
    section, schema, indexes, records, entities, model_owners = None, [], [], {}, {}, []
    history_fields = {}
    acsh_objects, evaluation_graphs, evaluation_nodes = {}, {}, {}
    for line in run.stdout.decode('ascii', errors='strict').splitlines():
        row = line.split()
        if not row: continue
        if row[0] == 'SECTION':
            require(section is None and len(row) == 3, 'duplicate/invalid source section')
            section = bytes.fromhex(row[2])
            require(len(section) == int(row[1]), 'source section size mismatch')
        elif row[0] == 'SCHEMA' and row[1] == '1': schema.append(row[4:])
        elif row[0] == 'INDEX': indexes.append(tuple(map(int, row[1:4])))
        elif row[0] == 'RECORD':
            require(len(row) == 9, 'invalid source record')
            identity = int(row[1], 16)
            require(identity not in records, 'duplicate source record')
            payload = bytes.fromhex(row[8])
            require(len(payload) == int(row[7]), 'source payload length mismatch')
            records[identity] = (int(row[2]), int(row[3]), int(row[4]),
                                 int(row[5]), int(row[6]), payload)
        elif row[0] == 'MODEL_BLOCK_RECORD': model_owners.append(int(row[1], 16))
        elif row[0] == 'HISTORY_FIELD':
            identity, field = history_field(row)
            require(identity not in history_fields, 'duplicate source history field')
            history_fields[identity] = field
        elif row[0] == 'ACSH':
            require(len(row) == 8, 'invalid AcSh object row')
            identity = int(row[1], 16)
            require(identity not in acsh_objects, 'duplicate AcSh object')
            acsh_objects[identity] = (
                int(row[2], 16), int(row[3], 16), row[4],
                int(row[5]), int(row[6]), int(row[7]))
        elif row[0] == 'EVALGRAPH':
            require(len(row) == 4, 'invalid evaluation-graph row')
            identity = int(row[1], 16)
            require(identity not in evaluation_graphs,
                    'duplicate evaluation graph')
            evaluation_graphs[identity] = (int(row[2], 16), int(row[3]))
        elif row[0] == 'EVALNODE':
            require(len(row) == 6, 'invalid evaluation-graph node row')
            graph = int(row[1], 16)
            index = int(row[2])
            key = (graph, index)
            require(key not in evaluation_nodes,
                    'duplicate evaluation-graph node')
            evaluation_nodes[key] = (
                int(row[3]), int(row[4]), int(row[5], 16))
        elif row[0] == 'ENTITY':
            require(len(row) == 13 and row[1] == 'MODEL', 'entity scope/profile mismatch')
            identity = int(row[3], 16)
            require(identity not in entities, 'duplicate source modeler')
            raw = bytes.fromhex(row[12])
            require(len(raw) == int(row[11]), 'source entity frame size mismatch')
            require(int(row[4], 16) == 0 and int(row[5], 16) == identity
                    and int(row[7], 16) == identity and row[8:11] == ['1', '2', '1'],
                    'entity storage association mismatch')
            entities[identity] = (row[2], int(row[6], 16), raw)
    require(section is not None and hashlib.sha256(section).hexdigest() == SECTION_SHA,
            'decoded AcDs section digest mismatch')
    require(schema == [['AcDbDs::ID:10', 'ASM_Data:15']], 'source schema mismatch')
    require(indexes == [(0,0,0),(1,9,0),(0,9,20),(1,9,40),(1,9,60)], 'data-index mismatch')
    require(model_owners == [0x1F] and set(entities) == set(EXPECTED),
            'model-space block record or entity census mismatch')
    require(set(history_fields) == set(EXPECTED), 'history-field census mismatch')
    require(history_graph_matches(acsh_objects, evaluation_graphs,
                                  evaluation_nodes),
            'History ID owner/dependency graph mismatch')
    dxf_entities, dxf_acds = dxf_projection(oda)
    require(set(dxf_entities) == set(EXPECTED) and set(records) == set(dxf_acds),
            'ODA source census mismatch')
    for identity, (record_schema, segment, local, offset, blob, payload) in records.items():
        require(blob == 0 and segment == 9 and (record_schema, segment, local) in indexes,
                'record/index/segment mismatch')
        require(offset <= len(section) and len(payload) <= len(section)-offset
                and section[offset:offset+len(payload)] == payload
                and dxf_acds[identity] == (record_schema, payload), 'exact payload mismatch')
    for identity, (kind, expected_size, history) in EXPECTED.items():
        actual_kind, actual_history, raw = entities[identity]
        require(actual_kind == kind and actual_history == history
                and dxf_entities[identity] == (kind, history), 'modeler type/history mismatch')
        payload = records[identity][5]
        require(records[identity][0] == 1 and len(payload) == expected_size
                and payload.startswith(b'ACIS BinaryFile'), 'SAB schema/prefix/length mismatch')
        graph, mode, reactors = entity_prefix(raw, identity, kind)
        if history:
            require(history_fields[identity] == EXPECTED_HISTORY_FIELD,
                    'source history handle code/value/span mismatch')
        else:
            require(history_fields[identity] is None,
                    'unexpected History ID on source profile')
        print(f'{identity:X} {kind} modelspace-mode-bit={mode} reactors={reactors} '
              f'segment={records[identity][1]} local={records[identity][2]} '
              f'payload-bytes={len(payload)} sha256={hashlib.sha256(payload).hexdigest()}')
    print('PASS: 3 model-space source entities bind to byte-identical ODA AcDs SAB records')


def main():
    if sys.argv[1:] == ['--self-test']:
        # An authored minimal entity prefix; no source drawing bytes are used.
        bits = ('00' + f'{37:08b}' + '00000010' + f'{0x176:016b}'
                + '10' + '0' + '10' + '10' + '1' + '1')
        bits += '0' * ((8 - len(bits) % 8) % 8)
        frame = bytes(int(bits[i:i+8], 2) for i in range(0, len(bits), 8))
        require(entity_prefix(frame, 0x176, 'REGION') == (36, 37, 0),
                'generated model-space prefix mismatch')
        for bit in (0, 37, 42):
            modified = bytearray(frame)
            modified[bit // 8] ^= 1 << (7 - bit % 8)
            try:
                entity_prefix(bytes(modified), 0x176, 'REGION')
            except ValueError:
                continue
            raise ValueError(f'mutated bit {bit} was accepted')
        try:
            entity_prefix(frame[:2], 0x176, 'REGION')
        except ValueError:
            pass
        else:
            raise ValueError('truncated entity prefix was accepted')
        good_history = ['HISTORY_FIELD', '2e1', '1', '3', '3', '2',
                        '2e4', '446', '24', '6', '1']
        _, good = history_field(good_history)
        require(good == EXPECTED_HISTORY_FIELD,
                'generated history H profile mismatch')
        for fault, label in ((['HISTORY_FIELD', '2e1', '1', '5', '5', '2',
                               '2e4', '446', '24', '6', '1'], 'role'),
                             (['HISTORY_FIELD', '2e1', '1', '3', '3', '2',
                               '2e4', '446', '23', '6', '1'], 'truncation')):
            try:
                _, parsed = history_field(fault)
                require(parsed == EXPECTED_HISTORY_FIELD,
                        f'mutated history {label} was accepted')
            except ValueError:
                pass
            _, retry = history_field(good_history)
            require(retry == EXPECTED_HISTORY_FIELD,
                    f'corrected history {label} retry failed')
        acsh = {0x2E4: EXPECTED_HISTORY_OBJECT,
                0x2E2: EXPECTED_SWEEP_OBJECT}
        graphs = {0x2E3: EXPECTED_EVALUATION_GRAPH}
        nodes = {(0x2E3, 0): EXPECTED_EVALUATION_NODE}
        require(history_graph_matches(acsh, graphs, nodes),
                'generated History ID graph did not close')
        bad_nodes = dict(nodes)
        bad_nodes[(0x2E3, 0)] = (32, 1, 0x2E4)
        require(not history_graph_matches(acsh, graphs, bad_nodes),
                'mutated evaluation-graph dependency was accepted')
        require(history_graph_matches(acsh, graphs, nodes),
                'corrected History ID graph retry failed')
        print('PASS: generated entity, history field and dependency graph controls')
        return 0
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--oda-dxf', type=Path, required=True)
    parser.add_argument('--oda-sha256', required=True)
    args = parser.parse_args()
    checked(args.source, SOURCE_SHA)
    oda = checked(args.oda_dxf, args.oda_sha256)
    compare(args.source, oda, args.probe)
    return 0


if __name__ == '__main__':
    try: sys.exit(main())
    except (OSError, ValueError, subprocess.SubprocessError) as exc:
        print(f'FAIL: {exc}', file=sys.stderr)
        sys.exit(1)
