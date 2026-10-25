"""Optional style/map/symbol source observations, never support eligibility.

Corroborates handle/local-ID scalar spans and named-reference positions.
For STYLE/LTYPE also checks original row flags, per-element text/shape codes and
STYLE handles against ezdxf's retained pattern tags and observed target kinds.
It does not certify decoded names, full rich/symbol schema, font/glyph/rendering
or rotation semantics, common/transitive closure, output, native layouts or
modeler semantics. Original source tags, never re-exported defaults, own spans.
"""

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile


PATTERN = re.compile(r"^(map|style|STYLE|LTYPE)-reference handle=([0-9a-f]+) kind=(\d+) index=(\d+) code=(\d+) value=([0-9a-f]+)$", re.M)
ROWS = re.compile(r"^symbol-row handle=([0-9a-f]+) family=(STYLE|LTYPE) flags=(\d+)$", re.M)
SEGMENTS = re.compile(r"^symbol-segment handle=([0-9a-f]+) index=(\d+) flags=(\d+) shape=(\d+) style=([0-9a-f]+)$", re.M)
CODES = {0: 5, 1: 330, 2: 330, 3: 360, 4: 340, 5: 340,
         6: 90, 7: 1005, 8: 1001, 9: 1003, 11: 7, 12: 340, 13: 340}


def is_handle(code):
    return code == 5 or 320 <= code <= 369 or 390 <= code <= 399 or code in (480, 481, 1005)


def compare(tags, events, table_style=False):
    if not events or len(tags) > 65536 or len(events) > len(tags):
        raise ValueError("empty/oversized map projection")
    previous = -1
    handles, definitions, names = set(), set(), set()
    for kind, index, code, value in events:
        if not 0 <= value <= 0xFFFFFFFF or index <= previous or not 0 <= index < len(tags):
            raise ValueError("duplicate/reordered/out-of-range map event")
        previous = index
        if tags[index][0] != code or (kind != 10 and CODES.get(kind) != code):
            raise ValueError("event code/role differs")
        scalar = tags[index][1]
        if kind == 6:
            if type(scalar) is not int or scalar != value or index == 0 or tags[index - 1] != (1, "CELLSTYLE_BEGIN"):
                raise ValueError("local definition differs")
            definitions.add(index)
        elif kind in (8, 9, 11):
            if type(scalar) is not str or value != 0:
                raise ValueError("named namespace event differs")
            names.add(index)
        else:
            if not is_handle(code) or type(scalar) is not str or int(scalar, 16) != value:
                raise ValueError("handle span differs")
            handles.add(index)
    if handles != {i for i, (code, _) in enumerate(tags) if is_handle(code)}:
        raise ValueError("omitted/invented map handle")
    expected, expected_names = set(), set()
    depth = 0
    for i, (code, scalar) in enumerate(tags):
        if code == 102:
            if type(scalar) is not str:
                raise ValueError("unexpanded application group")
            if scalar.startswith("{"):
                depth += 1
            elif scalar == "}" and depth:
                depth -= 1
            else:
                raise ValueError("bad application framing")
        elif not depth:
            if not table_style and (code, scalar) == (1, "CELLSTYLE_BEGIN"):
                expected.add(i + 1)
            if code in (1001, 1003) or (table_style and code == 7):
                expected_names.add(i)
    if depth:
        raise ValueError("unclosed application group")
    if definitions != expected:
        raise ValueError("omitted/invented local definition")
    if names != expected_names:
        raise ValueError("omitted/invented named namespace position")
    return {"handleSlots": len(handles), "localDefinitions": len(definitions), "referenceEvents": len(events)}


def flatten(xtags):
    tags = []
    for subclass in xtags.subclasses:
        for tag in subclass:
            if tag.code == 102 and type(tag.value) is int:
                if not 0 <= tag.value < len(xtags.appdata):
                    raise ValueError("bad independent appData placeholder")
                tags.extend((t.code, t.value) for t in xtags.appdata[tag.value])
            elif hasattr(tag, "dxftags"):
                tags.extend((t.code, t.value) for t in tag.dxftags())
            else:
                tags.append((tag.code, tag.value))
    for block in xtags.xdata:
        for tag in block:
            if hasattr(tag, "dxftags"):
                tags.extend((t.code, t.value) for t in tag.dxftags())
            else:
                tags.append((tag.code, tag.value))
    return tags


def source_records(records):
    """Keep physical source bodies in their actual TABLES/OBJECTS scope."""
    found = {}
    section = table = None
    for record in records:
        if not record:
            continue
        family = record[0][1]
        if family == "SECTION":
            section = next((v for c, v in record[1:] if c == 2), None)
        elif family == "ENDSEC":
            section = table = None
        elif family == "TABLE" and section == "TABLES":
            table = next((v for c, v in record[1:] if c == 2), None)
        elif family == "ENDTAB":
            table = None
        elif (family in ("STYLE", "LTYPE") and section == "TABLES" and table == family
              or family in ("TABLESTYLE", "CELLSTYLEMAP") and section == "OBJECTS"):
            if len(record) > 65537:
                raise ValueError("oversized original resource body")
            handle = next((v for c, v in record[1:] if c == 5), "0").upper()
            if not 0 <= int(handle, 16) <= 0xFFFFFFFF:
                raise ValueError("oversized original resource handle")
            if handle == "0":
                continue  # legacy source transport cannot qualify a modern row
            key = (family, handle)
            if key in found or len(found) >= 10000:
                raise ValueError("duplicate/oversized original resource inventory")
            found[key] = record[1:]
    return found


def original_resources(source):
    from ezdxf.lldxf.tagger import ascii_tags_loader, binary_tags_loader
    from ezdxf.lldxf.tags import group_tags
    from ezdxf.lldxf.types import cast_tag_value
    with Path(source).open("rb") as stream:
        binary = stream.read(22) == b"AutoCAD Binary DXF\r\n\x1a\x00"
    if binary:
        tags = binary_tags_loader(Path(source).read_bytes(), errors="strict")
        return source_records(([(t.code, t.value) for t in row] for row in group_tags(tags)))
    # This observer's symbol profile is modern UTF-8 DXF only. Scalar casting
    # keeps physical coordinate/group positions; tag_compiler would fold them.
    with Path(source).open(encoding="utf-8", errors="strict") as stream:
        tags = ascii_tags_loader(stream)
        return source_records(([(t.code, cast_tag_value(t.code, t.value)) for t in row]
                               for row in group_tags(tags)))


def pattern_elements(tags):
    result = []
    fields = None
    for code, value in tags:
        if code == 49:
            fields = {}
            result.append(fields)
        elif fields is not None and code in (74, 75, 340):
            if code in fields:
                raise ValueError("duplicate original segment identity field")
            fields[code] = value
    return [(i, row.get(74, 0), row.get(75, 0), int(row.get(340, "0"), 16))
            for i, row in enumerate(result)]


def compare_symbol(tags, events, family, projected_flags, projected_segments):
    result = compare(tags, events)
    application = None
    body = xdata = False
    dash = -1
    expected_roles = {}
    # Exclude common application and trailing XDATA from dash grouping.
    payload = []
    for index, (code, value) in enumerate(tags):
        if code == 102:
            if value == "}" and application is not None:
                application = None
            elif isinstance(value, str) and value.startswith("{") and application is None:
                application = value
            else:
                raise ValueError("ambiguous original symbol application framing")
            continue
        if application is None and code == 100:
            body = True
        if application is None and code == 1001:
            xdata = True
        if application is None and body and not xdata:
            payload.append((code, value))
            if code == 49:
                dash += 1
        if not is_handle(code):
            continue
        kind = 10
        if application == "{ACAD_REACTORS" and code == 330:
            kind = 2
        elif application == "{ACAD_XDICTIONARY" and code == 360:
            kind = 3
        elif application is None:
            if code == 5 and not body:
                kind = 0
            elif code == 330 and not body:
                kind = 1
            elif code == 1005 and xdata:
                kind = 7
            elif family == "LTYPE" and code == 340 and dash >= 0 and not xdata:
                kind = ("dash", dash)
        expected_roles[index] = kind
    segments = pattern_elements(payload) if family == "LTYPE" else []
    flags = [v for c, v in payload if c == 70]
    if len(flags) != 1 or flags[0] != projected_flags:
        raise ValueError("original symbol row flags differ")
    if family == "LTYPE" and [v for c, v in payload if c == 73] != [len(segments)]:
        raise ValueError("original declared element count differs")
    if segments != projected_segments:
        raise ValueError("original complex element identities differ")
    for index, kind in expected_roles.items():
        if isinstance(kind, tuple):
            flag = segments[kind[1]][1] & 6
            expected_roles[index] = {2: 12, 4: 13}.get(flag, 10)
    if {i: k for k, i, c, v in events if is_handle(c)} != expected_roles:
        raise ValueError("original symbol handle roles differ")
    return {**result, "flags": projected_flags, "elements": len(segments),
            "textElements": sum((s[1] & 6) == 2 for s in segments),
            "shapeElements": sum((s[1] & 6) == 4 for s in segments)}


def observe(source, projection):
    import ezdxf
    if ezdxf.__version__ != "1.4.4":
        raise ValueError("observer requires ezdxf 1.4.4")
    if Path(source).stat().st_size > 64 * 1024 * 1024 or Path(projection).stat().st_size > 16 * 1024 * 1024:
        raise ValueError("oversized advisory input")
    document = ezdxf.readfile(source)
    if document.dxfversion not in ("AC1021", "AC1024", "AC1027", "AC1032"):
        raise ValueError("observer requires a modern declared source revision")
    original = original_resources(source)
    text = Path(projection).read_text()
    events = {}
    for prefix, handle, kind, index, code, value in PATTERN.findall(text):
        family = {"style": "TABLESTYLE", "map": "CELLSTYLEMAP"}.get(prefix, prefix)
        events.setdefault((family, handle.upper()), []).append((int(kind), int(index), int(code), int(value, 16)))
    if not events or len(events) > 1000:
        raise ValueError("missing/oversized map census")
    results = []
    rows = {}
    for handle, family, flags in ROWS.findall(text):
        key = (family, handle.upper())
        if key in rows:
            raise ValueError("duplicate symbol row projection")
        rows[key] = int(flags)
    segments = {}
    for handle, index, flags, shape, style in SEGMENTS.findall(text):
        segments.setdefault(handle.upper(), []).append((int(index), int(flags), int(shape), int(style, 16)))
    for (family, handle), values in events.items():
        obj = document.entitydb[handle]
        if obj.dxftype() != family:
            raise ValueError("wrong source resource family")
        tags = original[(family, handle)]
        if family in ("STYLE", "LTYPE"):
            row_flags = rows.pop((family, handle))
            parts = segments.pop(handle, [])
            details = compare_symbol(tags, values, family, row_flags, parts)
            if obj.dxf.flags != row_flags:
                raise ValueError("independent typed symbol flags differ")
            if family == "LTYPE":
                typed = pattern_elements([(t.code, t.value) for t in obj.pattern_tags.tags])
                if typed != parts:
                    raise ValueError("independent retained pattern identities differ")
                for _, flags, _, target in parts:
                    kind = flags & 6
                    if kind in (2, 4):
                        target_key = ("STYLE", format(target, "X"))
                        target_obj = document.entitydb.get(target_key[1])
                        if (target == 0 or target_key not in events or target_obj is None
                                or target_obj.dxftype() != "STYLE" or bool(target_obj.dxf.flags & 1) != (kind == 4)):
                            raise ValueError("independent complex STYLE target kind differs")
        else:
            details = compare(tags, values, family == "TABLESTYLE")
        results.append({"family": family, "handle": handle, **details})
    if rows or segments:
        raise ValueError("unmatched symbol row/element projection")
    return {"schemaVersion": 3, "status": "SCOPED_RESOURCE_REFERENCE_SPANS_MATCH", "qualificationEligible": False,
            "ezdxfVersion": ezdxf.__version__, "resources": results,
            "sourceSha256": hashlib.sha256(Path(source).read_bytes()).hexdigest(),
            "projectionSha256": hashlib.sha256(Path(projection).read_bytes()).hexdigest(),
            "limits": __doc__.split("\n\n", 1)[1].strip()}


def generated_check(executable):
    results = []
    with tempfile.TemporaryDirectory(prefix="libdxfrw-symbol-observer-") as directory:
        root = Path(directory)
        for version in ("AC1021", "AC1024", "AC1027", "AC1032"):
            for encoding in ("ascii", "binary"):
                source = root / (version + "-" + encoding + ".dxf")
                projection = source.with_suffix(".log")
                subprocess.run([executable, "--generate-symbol-source", version, encoding, str(source)], check=True)
                observed = subprocess.run([executable, "--inspect", str(source)], check=True, capture_output=True, text=True)
                projection.write_text(observed.stdout, encoding="utf-8")
                result = observe(source, projection)
                symbols = [r for r in result["resources"] if r["family"] in ("STYLE", "LTYPE")]
                linetypes = [r for r in symbols if r["family"] == "LTYPE"]
                if (len(symbols) != 3 or len(linetypes) != 1 or linetypes[0]["elements"] != 2
                        or linetypes[0]["textElements"] != 1 or linetypes[0]["shapeElements"] != 1):
                    raise ValueError("independent generated symbol inventory differs")
                results.append({"version": version, "encoding": encoding, **result})
    return {"schemaVersion": 3, "status": "GENERATED_SYMBOL_OBSERVATIONS_MATCH", "qualificationEligible": False, "routes": results}


def self_test():
    tags = [(5, "C120"), (102, "{ACAD_REACTORS"), (330, "C110"), (102, "}"),
            (330, "C110"), (340, "C101"), (1, "CELLSTYLE_BEGIN"), (90, 101),
            (1001, "APP"), (1003, "LAYER")]
    valid = [(0, 0, 5, 0xC120), (2, 2, 330, 0xC110), (1, 4, 330, 0xC110),
             (4, 5, 340, 0xC101), (6, 7, 90, 101), (8, 8, 1001, 0), (9, 9, 1003, 0)]
    assert compare(tags, valid) == {"handleSlots": 4, "localDefinitions": 1, "referenceEvents": 7}
    bad = [[], valid[:-1], valid + [valid[-1]], valid[::-1],
           [*valid[:4], (6, 7, 90, 102), *valid[5:]], [*valid[:4], (6, 6, 90, 101), *valid[5:]],
           [valid[0], *valid[2:]], [(0, 0, 5, 0x10000C120), *valid[1:]],
           [(10, 0, 90, 0xC120), *valid[1:]]]
    for fault in bad:
        try:
            compare(tags, fault)
        except ValueError:
            continue
        raise AssertionError("corrupt map projection passed")
    from types import SimpleNamespace
    tag = lambda c, v: SimpleNamespace(code=c, value=v)
    xtags = SimpleNamespace(subclasses=[[tag(5, "A"), tag(102, 0)]],
                            appdata=[[tag(102, "{APP"), tag(330, "B"), tag(102, "}")]], xdata=[])
    assert flatten(xtags) == [(5, "A"), (102, "{APP"), (330, "B"), (102, "}")]
    style = [(5, "A"), (330, "B"), (7, "FONT")]
    style_events = [(0, 0, 5, 10), (1, 1, 330, 11), (11, 2, 7, 0)]
    assert compare(style, style_events, True) == {"handleSlots": 2, "localDefinitions": 0, "referenceEvents": 3}
    try:
        compare(style, style_events[:-1], True)
    except ValueError:
        pass
    else:
        raise AssertionError("missing TABLESTYLE name slot passed")
    ltype = [(5, "A"), (330, "B"), (100, "AcDbSymbolTableRecord"), (100, "AcDbLinetypeTableRecord"),
             (70, 0), (73, 2), (49, 1.0), (74, 2), (75, 0), (340, "C"), (50, 0.0), (9, "T"),
             (49, -0.5), (74, 4), (75, 7), (340, "D"), (50, 0.0)]
    ltype_events = [(0, 0, 5, 10), (1, 1, 330, 11), (12, 9, 340, 12), (13, 15, 340, 13)]
    parts = [(0, 2, 0, 12), (1, 4, 7, 13)]
    assert compare_symbol(ltype, ltype_events, "LTYPE", 0, parts)["shapeElements"] == 1
    textstyle = [(5, "C"), (330, "B"), (100, "AcDbSymbolTableRecord"), (100, "AcDbTextStyleTableRecord"), (70, 1)]
    assert compare_symbol(textstyle, [(0, 0, 5, 12), (1, 1, 330, 11)], "STYLE", 1, [])["flags"] == 1
    corruptions = [
        (ltype, [*ltype_events[:2], (13, 9, 340, 12), ltype_events[3]], 0, parts),
        (ltype, ltype_events, 1, parts),
        (ltype, ltype_events, 0, [(0, 2, 0, 12)]),
        (ltype, ltype_events, 0, [parts[0], (1, 4, 8, 13)]),
        (ltype, ltype_events, 0, [parts[0], (1, 4, 7, 14)]),
        ([*ltype[:5], (73, 3), *ltype[6:]], ltype_events, 0, parts),
        ([*ltype[:5], (70, 0), *ltype[6:]], ltype_events, 0, parts)
    ]
    for tags, events, flags, projected in corruptions:
        try:
            compare_symbol(tags, events, "LTYPE", flags, projected)
        except ValueError:
            continue
        raise AssertionError("corrupt symbol identity observation passed")
    framed = [[(0, "SECTION"), (2, "TABLES")], [(0, "TABLE"), (2, "STYLE")],
              [(0, "STYLE"), *textstyle], [(0, "ENDTAB")], [(0, "ENDSEC")]]
    assert source_records(iter(framed)) == {("STYLE", "C"): textstyle}
    try:
        source_records(iter([*framed[:3], framed[2]]))
    except ValueError:
        pass
    else:
        raise AssertionError("duplicate original resource accepted")
    print("resource reference observer: four positives, 18 corruptions, appData expansion and original scopes; no eligibility")


if __name__ == "__main__":
    if sys.flags.optimize:
        raise ValueError("optimized Python cannot run evidence checks")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--source")
    parser.add_argument("--projection")
    parser.add_argument("--generated-executable")
    args = parser.parse_args()
    if args.self_test:
        self_test()
    elif args.generated_executable:
        print(json.dumps(generated_check(str(Path(args.generated_executable).resolve())), indent=2, allow_nan=False))
    elif args.source and args.projection:
        print(json.dumps(observe(args.source, args.projection), indent=2, allow_nan=False))
    else:
        parser.error("use --self-test or --source DXF --projection CXX_LOG")
