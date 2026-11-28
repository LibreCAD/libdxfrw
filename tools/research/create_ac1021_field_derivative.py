#!/usr/bin/env python3
"""Create a scratch AC1021 FIELD experiment from the pinned public DXF.

Requires a local acadrust 0.6.8 crate and its Cargo dependencies. The crate is
copied and patched only in a new scratch directory. This derivative is not a
producer-authored witness and must not be committed as a fixture.
"""
import hashlib
from pathlib import Path
import shutil
import subprocess
import sys

SOURCE_SHA = 'fb09a7af8882787553149a675c6d15b45bab8ff4a49f58200460832eab3de401'
FILES = {
    'src/io/dwg/dwg_stream_writers/object_writer/entities.rs': (
        '140a860335fbe628b7b7fa4d4afd07f778dda38f42c4c9ad999cfc533fd2aa89',
        '                4 => self.write_table_string_value(&v.text),',
        '                4 => self.writer.write_variable_text(&v.text),'),
    'src/io/dwg/dwg_stream_readers/object_reader/entities.rs': (
        '9d66b4b951e0f022cd375ecc938610b61dbee2e2eb642af63f8d368b98e17a8f',
        '            4 => v.text = read_string_cad_value(reader, version),',
        '            4 => v.text = reader.read_variable_text(),'),
}
RUST = r'''use acadrust::{DxfReader, DwgReader, DwgWriter};
use acadrust::objects::ObjectType;
use acadrust::types::DxfVersion;
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args: Vec<String> = std::env::args().collect();
    let mut doc = DxfReader::from_file(&args[1])?.read()?;
    doc.version = DxfVersion::AC1021;
    DwgWriter::write_to_file(&args[2], &doc)?;
    let out = DwgReader::from_file(&args[2])?.read()?;
    let count = out.objects.values().filter(|o| matches!(o, ObjectType::Field(_))).count();
    let lists: Vec<_> = out.objects.values().filter_map(|o| match o {
        ObjectType::FieldList(l) => Some(l.fields.len()), _ => None,
    }).collect();
    if count != 16 || lists != [16] { return Err("unexpected FIELD graph".into()); }
    println!("AC1021 self-readback: {count} FIELDs and {lists:?} FIELDLIST slots");
    Ok(())
}'''


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    if len(sys.argv) != 4:
        raise SystemExit('usage: create_ac1021_field_derivative.py source.dxf '
                         'acadrust-0.6.8-crate new-scratch-directory')
    source, crate, scratch = (Path(p).resolve() for p in sys.argv[1:])
    if digest(source) != SOURCE_SHA:
        raise ValueError('unrecognized source DXF')
    for name, (expected, _, _) in FILES.items():
        if digest(crate / name) != expected:
            raise ValueError('unrecognized ACadRust 0.6.8 source: ' + name)
    scratch.mkdir(parents=True, exist_ok=False)
    patched = scratch / 'acadrust'
    shutil.copytree(crate, patched)
    for name, (_, old, new) in FILES.items():
        path = patched / name
        text = path.read_text()
        if text.count(old) != 1:
            raise ValueError('ambiguous ACadRust patch: ' + name)
        path.write_text(text.replace(old, new))
    (scratch / 'src').mkdir()
    (scratch / 'src' / 'main.rs').write_text(RUST + '\n')
    (scratch / 'Cargo.toml').write_text(
        '[package]\nname="libdxfrw_field_tv_probe"\nversion="0.1.0"\n'
        'edition="2021"\n[dependencies]\nacadrust={path="acadrust"}\n')
    output = scratch / 'output.dwg'
    subprocess.run(['cargo', 'run', '--offline', '--manifest-path',
                    str(scratch / 'Cargo.toml'), '--', str(source), str(output)],
                   check=True)
    print('DWG SHA-256:', digest(output))


if __name__ == '__main__':
    main()
