#!/usr/bin/env python3
"""Author/inspect a scratch SUNSTUDY DXF graph; never writes repository fixtures."""
import argparse
import re
from pathlib import Path


def author(path, version="AC1027"):
    if path.exists():
        raise ValueError("refusing to replace an existing source")
    tags = [
        (0, "SECTION"), (2, "HEADER"), (9, "$ACADVER"), (1, version), (0, "ENDSEC"),
        (0, "SECTION"), (2, "CLASSES"), (0, "CLASS"), (1, "SUNSTUDY"),
        (2, "AcDbSunStudy"), (3, "SCENEOE"), (90, 1153), (91, 1), (280, 0), (281, 0),
        (0, "ENDSEC"), (0, "SECTION"), (2, "TABLES"), (0, "ENDSEC"),
        (0, "SECTION"), (2, "BLOCKS"), (0, "ENDSEC"),
        (0, "SECTION"), (2, "ENTITIES"), (0, "LINE"), (5, "90"), (8, "0"),
        (10, 1), (20, 2), (30, 3), (11, 4), (21, 5), (31, 6), (0, "ENDSEC"),
        (0, "SECTION"), (2, "OBJECTS"), (0, "DICTIONARY"), (5, "C"), (330, 0),
        (100, "AcDbDictionary"), (280, 0), (281, 1), (3, "MY_SUNSTUDIES"), (350, "E0"),
        (0, "DICTIONARY"), (5, "E0"), (330, "C"), (100, "AcDbDictionary"),
        (280, 0), (281, 1), (3, "Morning"), (350, "F0"),
        (0, "SUNSTUDY"), (5, "F0"), (102, "{ACAD_REACTORS"), (330, "E0"),
        (102, "}"), (330, "E0"), (100, "AcDbSunStudy"), (90, 1),
        (1, "Morning"), (2, "Example"), (70, 1), (291, 1), (91, 1),
        (90, 2460000), (90, 3600), (292, 0), (73, 2), (290, 1), (290, 0),
        (74, 0), (75, 1), (76, 1), (77, 1), (40, 0.25), (293, 1), (294, 0),
        (0, "ENDSEC"), (0, "EOF"),
    ]
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("".join(f"{code:3d}\n{value}\n" for code, value in tags), encoding="ascii")
    print("authored", path)


def author_oda(path, version="AC1027"):
    import ezdxf
    from ezdxf.entities import DXFClass
    if ezdxf.__version__ != "1.4.5":
        raise ValueError("ezdxf 1.4.5 required")
    if path.exists():
        raise ValueError("refusing to replace an existing source")
    path.parent.mkdir(parents=True, exist_ok=True)
    document = ezdxf.new({"AC1024": "R2010", "AC1027": "R2013", "AC1032": "R2018"}[version])
    document.modelspace().add_line((1, 2, 3), (4, 5, 6))
    owner = document.rootdict.add_new_dict("MY_SUNSTUDIES")
    placeholder = document.objects.new_entity("ACDBPLACEHOLDER", dxfattribs={"owner": owner.dxf.handle})
    owner.add("Morning", placeholder)
    source_class = DXFClass.new(doc=document)
    source_class.update_dxf_attribs({"name": "SUNSTUDY", "cpp_class_name": "AcDbSunStudy",
        "app_name": "SCENEOE", "flags": 1153, "was_a_proxy": 0, "is_an_entity": 0,
        "instance_count": 1})
    document.classes.register(source_class)
    document.saveas(path)
    text = path.read_text(encoding="utf-8")
    pattern = (r"\nACDBPLACEHOLDER\n  5\n" + re.escape(placeholder.dxf.handle)
               + r"\n330\n" + re.escape(owner.dxf.handle) + r"(?=\n  0\n)")
    content = [(0, "SUNSTUDY"), (5, placeholder.dxf.handle), (102, "{ACAD_REACTORS"),
               (330, owner.dxf.handle), (102, "}"), (330, owner.dxf.handle),
               (100, "AcDbSunStudy"), (90, 1), (1, "Morning"), (2, "Example"),
               (70, 1), (291, 1), (91, 1), (90, 2460000), (90, 3600), (292, 0),
               (73, 2), (290, 1), (290, 0), (74, 0), (75, 1), (76, 1),
               (77, 1), (40, 0.25), (293, 1), (294, 0)]
    replacement = "\nSUNSTUDY\n" + "".join(f"{code:3d}\n{value}\n" for code, value in content[1:]).rstrip("\n")
    result, count = re.subn(pattern, lambda _: replacement, text)
    if count != 1:
        raise ValueError("source placeholder not uniquely identified")
    path.write_text(result, encoding="utf-8")
    print("authored", path)


def records(path):
    raw = path.read_bytes()
    if raw.startswith(b"AutoCAD Binary DXF\r\n\x1a\x00"):
        import ezdxf
        from ezdxf.lldxf.tagger import binary_tags_loader
        if ezdxf.__version__ != "1.4.5":
            raise ValueError("binary inspection requires ezdxf 1.4.5")
        pairs = ((tag.code, str(tag.value).strip()) for tag in binary_tags_loader(raw))
    else:
        lines = raw.decode("utf-8").splitlines()
        if len(lines) % 2:
            raise ValueError("unpaired DXF group")
        pairs = ((int(lines[i].strip()), lines[i + 1].strip()) for i in range(0, len(lines), 2))
    current = None
    for code, value in pairs:
        if code == 0:
            if current is not None:
                yield current
            current = [(code, value)]
        elif current is not None:
            current.append((code, value))
    if current is not None:
        yield current


def inspect(path):
    objects = list(records(path))
    studies = [r for r in objects if r[0] == (0, "SUNSTUDY")]
    if len(studies) != 1:
        raise ValueError("expected one SUNSTUDY record")
    study = studies[0]
    def values(record, code):
        return [value for key, value in record if key == code]
    identity = values(study, 5)
    owner_values = values(study, 330)
    if len(identity) != 1 or len(owner_values) != 2 or owner_values[0] != owner_values[1]:
        raise ValueError("SUNSTUDY identity, owner/reactor mismatch")
    owner = owner_values[1]
    dictionaries = [r for r in objects if r[0] == (0, "DICTIONARY")]
    def entries(record):
        return [(record[i][1], record[i + 1][1]) for i in range(len(record) - 1)
                if record[i][0] == 3 and record[i + 1][0] in (350, 360)]
    owners = [r for r in dictionaries if values(r, 5) == [owner]
              and ("Morning", identity[0]) in entries(r)]
    roots = [r for r in dictionaries if ("MY_SUNSTUDIES", owner) in entries(r)]
    if len(owners) != 1 or len(roots) != 1 or values(owners[0], 330) != values(roots[0], 5):
        raise ValueError("SUNSTUDY dictionary graph is not reciprocal")
    expected = {1: ["Morning"], 2: ["Example"], 70: ["1"], 91: ["1"],
                90: ["1", "2460000", "3600"], 73: ["2"], 290: ["1", "0"],
                74: ["0"], 75: ["1"], 76: ["1"], 77: ["1"], 293: ["1"], 294: ["0"]}
    if any(values(study, code) != expected_values for code, expected_values in expected.items()):
        raise ValueError("SUNSTUDY field mismatch")
    if len(values(study, 40)) != 1 or float(values(study, 40)[0]) != .25:
        raise ValueError("SUNSTUDY spacing mismatch")
    print("SUNSTUDY source fields and dictionary/reactor edges PASS", path)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("author", "author-oda", "inspect"))
    parser.add_argument("path", type=Path)
    parser.add_argument("--version", choices=("AC1024", "AC1027", "AC1032"), default="AC1027",
                        help="revision for author or author-oda")
    args = parser.parse_args()
    if args.action == "author":
        author(args.path, args.version)
    elif args.action == "author-oda":
        author_oda(args.path, args.version)
    else:
        inspect(args.path)


if __name__ == "__main__":
    main()
