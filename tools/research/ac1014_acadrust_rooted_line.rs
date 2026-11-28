// SPDX-License-Identifier: GPL-2.0-or-later
// Scratch-only generated R14 LINE and rooted plot-style control. The producer
// reconstructs LAYOUT on reopen; this is not native AutoCAD evidence.
use acadrust::{CadDocument, DwgReader, DwgWriter};
use acadrust::entities::{EntityType, Line};
use acadrust::types::DxfVersion;
use std::path::PathBuf;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let output = PathBuf::from(std::env::var_os("AC1012_WITNESS_OUTPUT")
        .ok_or("AC1012_WITNESS_OUTPUT is required")?);
    let mut doc = CadDocument::with_version(DxfVersion::AC1014);
    doc.add_entity(EntityType::Line(Line::from_coords(1., 2., 0., 4., 5., 0.)))?;
    let plotstyle = doc.header.acad_plotstylename_dict_handle;
    let original = output.join("r14-line.dwg");
    DwgWriter::write_to_file(&original, &doc)?;
    let mut reopened = DwgReader::from_file(&original)?.read()?;
    reopened.named_objects_dictionary_mut().ok_or("missing root")?
        .insert("ACAD_PLOTSTYLENAME", plotstyle);
    DwgWriter::write_to_file(output.join("r14-line-rooted-resaved.dwg"), &reopened)?;
    Ok(())
}
