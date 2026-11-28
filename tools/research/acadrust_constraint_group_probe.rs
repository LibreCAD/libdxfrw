use acadrust::{DwgReader, objects::{ObjectType, AssociativeData}};
fn main() {
    let path = std::env::args().nth(1).expect("input DWG");
    let mut reader = DwgReader::from_file(&path).expect("open");
    match reader.read_with_stats() {
        Ok(outcome) => {
            println!("STATS={:#?}", outcome.stats);
            let doc = outcome.document;
            let mut count = 0;
            for (handle, object) in doc.objects.iter() {
                if let ObjectType::Associative(assoc) = object {
                    if let AssociativeData::ConstraintGroup(group) = &assoc.data {
                        count += 1;
                        println!("GROUP handle={:?} data={:#?}", handle, group);
                    }
                }
            }
            println!("GROUP_COUNT={}", count);
        }
        Err(e) => { eprintln!("READ_ERROR={e:?}"); std::process::exit(1); }
    }
}
