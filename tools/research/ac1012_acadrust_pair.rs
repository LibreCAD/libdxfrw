// Scratch-only AC1012 producer witness. Build against the pinned acadrust
// source through generate_ac1012_acadrust_pair.py; no sample is committed.
use acadrust::{CadDocument, DwgReader, DwgWriter};
use acadrust::entities::{EntityType, Line};
use acadrust::objects::{MLineStyle, ObjectType};
use acadrust::types::DxfVersion;
use std::f64::consts::PI;
use std::path::PathBuf;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let output = PathBuf::from(std::env::var_os("AC1012_WITNESS_OUTPUT")
        .ok_or("AC1012_WITNESS_OUTPUT is required")?);
    for (side, first, unique, second) in [
        ("destination", 15., "DEST_ONLY", 22.),
        ("source", 37., "SOURCE_ONLY", 51.),
    ] {
        let mut doc = CadDocument::with_version(DxfVersion::AC1012);
        doc.add_entity(EntityType::Line(Line::from_coords(1., 2., 0., 4., 5., 0.)))?;
        let owner = doc.ensure_subdictionary("ACAD_MLINESTYLE");
        for (name, degrees) in [("SAME_KEY", first), (unique, second)] {
            let handle = doc.allocate_handle();
            let mut style = MLineStyle::new(name);
            style.handle = handle;
            style.owner = owner;
            style.start_angle = degrees * PI / 180.;
            style.add_element_at_offset(0.5);
            style.add_element_at_offset(-0.5);
            doc.objects.insert(handle, ObjectType::MLineStyle(style));
            match doc.objects.get_mut(&owner) {
                Some(ObjectType::Dictionary(dictionary)) => { dictionary.insert(name, handle); }
                _ => return Err("style dictionary missing".into()),
            }
        }
        let original = output.join(format!("{side}.dwg"));
        DwgWriter::write_to_file(&original, &doc)?;
        let reopened = DwgReader::from_file(&original)?.read()?;
        DwgWriter::write_to_file(output.join(format!("{side}-resaved.dwg")), &reopened)?;
        println!("created {side} original and resaved witnesses");
    }
    Ok(())
}
