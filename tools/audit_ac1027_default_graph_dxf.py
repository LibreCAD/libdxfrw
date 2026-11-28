#!/usr/bin/env python3
"""Hash-bound raw-tag and ezdxf audit of a selected strict AC1027 LINE output.

Input lineage is an independently recorded join; no source or fixture is bundled.
"""
import argparse
import hashlib
from pathlib import Path
import ezdxf
from ezdxf.lldxf.tagger import ascii_tags_loader, binary_tags_loader


def require(value, message):
    if not value:
        raise ValueError(message)


def records(tags):
    current = []
    for tag in tags:
        if tag.code == 0:
            if current:
                yield current
            current = []
        current.append(tag)
    if current:
        yield current


def field(record, code):
    values = [str(t.value) for t in record if t.code == code]
    require(len(values) == 1, f"expected exactly one group {code}")
    return values[0]


def entry_map(record):
    result = {}
    pending = None
    for tag in record:
        if tag.code == 3:
            require(pending is None, "unpaired dictionary name")
            pending = str(tag.value)
        elif tag.code in (350, 360) and pending is not None:
            require(pending not in result, "invalid dictionary member")
            result[pending] = str(tag.value).upper()
            pending = None
    require(pending is None, "dangling dictionary name")
    return result


def audit(path, expected):
    blob = path.read_bytes()
    require(hashlib.sha256(blob).hexdigest() == expected, "DXF SHA-256 mismatch")
    is_binary = blob.startswith(b"AutoCAD Binary DXF\r\n\x1a\0")
    if is_binary:
        tags = list(binary_tags_loader(blob))
    else:
        with path.open(encoding="utf-8") as stream:
            tags = list(ascii_tags_loader(stream))
    rows = list(records(tags))
    selected = lambda name: [r for r in rows if r and r[0].code == 0 and r[0].value == name]
    require(not selected("DICTIONARYWDFLT"), "legacy dictionary marker in selected output")
    dic = selected("ACDBDICTIONARYWDFLT")
    place = selected("ACDBPLACEHOLDER")
    require(len(dic) == len(place) == 1, "expected one plotstyle dictionary and placeholder")
    dic, place = dic[0], place[0]
    dh, ph = field(dic, 5).upper(), field(place, 5).upper()
    require(field(dic, 340).upper() == ph and entry_map(dic) == {"Normal": ph}, "plotstyle default/member mismatch")
    require(field(place, 330).upper() == dh, "placeholder owner mismatch")
    classes = [r for r in selected("CLASS") if any(t.code == 1 and t.value == "ACDBDICTIONARYWDFLT" for t in r)]
    require(len(classes) == 1 and field(classes[0], 2) == "AcDbDictionaryWithDefault", "canonical CLASS missing")
    materials = selected("MATERIAL")
    require(len(materials) == 3, "expected three material records")
    material_by_handle = {field(r, 5).upper(): r for r in materials}
    require(len(material_by_handle) == 3, "duplicate material handle")
    roots = []
    mdict = None
    for row in selected("DICTIONARY"):
        entries = entry_map(row)
        if "ACAD_MATERIAL" in entries and "ACAD_PLOTSTYLENAME" in entries:
            roots.append((row, entries))
        if set(entries) == {"ByBlock", "ByLayer", "Global"}:
            require(mdict is None, "multiple material dictionaries")
            mdict = (row, entries)
    require(len(roots) == 1 and mdict is not None, "missing unique rooted dictionaries")
    root, root_entries = roots[0]
    require(all(not any(t.code == 360 for t in row) for row in (root, mdict[0], dic)),
            "selected dictionary member role is not group 350")
    root_h, material_h = field(root, 5).upper(), field(mdict[0], 5).upper()
    require(root_entries["ACAD_MATERIAL"] == material_h and root_entries["ACAD_PLOTSTYLENAME"] == dh, "NOD links differ")
    require(field(dic, 330).upper() == root_h and field(mdict[0], 330).upper() == root_h, "dictionary owners differ")
    for name, handle in mdict[1].items():
        require(handle in material_by_handle, f"missing {name} target")
        item = material_by_handle[handle]
        require(field(item, 1) == name, f"material name {name} differs")
        # First 330 may be an explicit persistent reactor; final one is owner.
        owners = [str(t.value).upper() for t in item if t.code == 330]
        require(owners and all(h == material_h for h in owners), f"material owner/reactor differs for {name}")
    layers = [r for r in selected("LAYER") if any(t.code == 2 and t.value == "0" for t in r)]
    require(len(layers) == 1 and field(layers[0], 347).upper() == mdict[1]["Global"]
            and field(layers[0], 390).upper() == ph, "raw LAYER 347/390 do not close")
    refs = [str(tags[i + 1].value).upper() for i, tag in enumerate(tags[:-1])
            if tag.code == 9 and tag.value == "$CMATERIAL" and tags[i + 1].code == 347]
    require(refs == [mdict[1]["ByLayer"]], "raw HEADER CMATERIAL does not close")
    lines = selected("LINE")
    require(len(lines) == 1, "LINE inventory differs")
    for code, expected_value in ((10, 1.), (20, 2.), (30, 3.), (11, 4.), (21, 5.), (31, 6.)):
        require(float(field(lines[0], code)) == expected_value, "LINE coordinates differ")
    doc = ezdxf.readfile(path)
    result = doc.audit()
    require(not result.errors and not result.fixes, "ezdxf audit reports errors or repairs")
    root = doc.rootdict
    plot_dict = root.get("ACAD_PLOTSTYLENAME")
    material_dict = root.get("ACAD_MATERIAL")
    require(plot_dict and material_dict and plot_dict.dxf.handle == dh
            and plot_dict.dxf.default == ph and plot_dict.get("Normal").dxf.handle == ph,
            "ezdxf plotstyle graph differs")
    require({key for key, _ in material_dict.items()} == {"ByBlock", "ByLayer", "Global"},
            "ezdxf material dictionary differs")
    require(doc.header.get("$CMATERIAL") == mdict[1]["ByLayer"], "ezdxf HEADER material differs")
    require(len(doc.modelspace()) == 1 and doc.modelspace()[0].dxftype() == "LINE", "ezdxf model inventory differs")
    print(f"{path}: {'binary' if is_binary else 'ASCII'} raw and ezdxf graph PASS")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dxf", required=True, type=Path)
    parser.add_argument("--sha256", required=True)
    args = parser.parse_args()
    audit(args.dxf, args.sha256.lower())


if __name__ == "__main__":
    main()
