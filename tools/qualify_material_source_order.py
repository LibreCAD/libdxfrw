#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Generate independent MATERIAL controls with ezdxf 1.4.5 and run body probes.

Run with an isolated Python containing ezdxf==1.4.5. The probe is the built
libdxfrw_dxf_fixture_tests executable. An optional converter also checks selected
metadata, material and geometry round trips. Native rendering and arbitrary
source ordering remain unqualified.
Optional SCALE checks use authored generic tags and independent projections;
ezdxf has no typed annotation-scale implementation for this profile.
Optional MLINESTYLE checks use its typed producer/parser for descriptions and
unchanged field/element tuples, plus selected name/LTYPE text transcoding;
selected element ACI edits reuse the dictionary-authority source profiles.
Actual MLINE rendering remains unqualified.
All drawings are created from scratch in the requested empty output directory.
"""
import argparse
import hashlib
import importlib.metadata
import inspect
import json
from pathlib import Path
import subprocess
import sys


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


PEER_VARIABLE_EDIT = r"edited peer_Ω^A\U+1234" + chr(1)
PEER_SCALE_KEY = "A0"
PEER_SCALE_EDIT = {70: 9, 300: r"edited peer café^A\U+1234",
                   140: 2.5, 141: 7.5, 290: 0}
PEER_MLINE_STYLE_NAME = "PeerMline"
PEER_MLINE_STYLE_RENAMED_KEY = "RenamedPeer"
PEER_MLINE_STYLE_EDIT = r"edited peer café^A\U+1234"
PEER_MLINE_STYLE_SCALAR_EDIT = {"flags": 273, "fillColor": 6,
                                "startAngle": 22.5, "endAngle": 157.5,
                                "description": PEER_MLINE_STYLE_EDIT}
PEER_MLINE_STYLE_ELEMENT_ACI_EDIT = (0, 256)


def selected_physical_records(path, binary):
    """Load selected-source tags without constructing or repairing a document."""
    from ezdxf.filemanagement import dxf_file_info
    from ezdxf.lldxf.tagger import ascii_tags_loader, binary_tags_loader, tag_compiler
    from ezdxf.lldxf.tags import group_tags

    if binary:
        tags = list(tag_compiler(binary_tags_loader(path.read_bytes(), errors="strict")))
    else:
        info = dxf_file_info(path)
        with path.open(encoding=info.encoding, errors="strict") as stream:
            tags = list(tag_compiler(ascii_tags_loader(stream)))
    section, records = None, []
    for record in group_tags(tags):
        kind = record[0].value
        if kind == "SECTION":
            section = record.get_first_value(2)
        elif kind == "ENDSEC":
            section = None
        else:
            records.append((section, record))
    return records


def selected_dictionary_entries(xtags):
    result, pending = {}, None
    for code, value in xtags.get_subclass("AcDbDictionary"):
        if code == 3:
            assert pending is None, "physical dictionary missing pointer"
            pending = value
        elif code in (350, 360):
            assert pending is not None and pending not in result, "physical dictionary key"
            result[pending] = {"handle": value, "role": code}
            pending = None
    assert pending is None, "physical dictionary dangling key"
    return result


def selected_physical_controls(records, source_recipe=False):
    """The shared authored LINE/null-material controls for resource probes."""
    layers = []
    for scope, record in records:
        if scope != "TABLES" or record[0].value != "LAYER":
            continue
        handles = [t.value for t in record if t.code == 347]
        assert len(handles) <= 1, "physical LAYER material multiplicity"
        assert not handles or int(handles[0], 16) == 0, "physical LAYER dangling/non-null material"
        layers.append({"name": record.get_first_value(2),
                       "materialHandlePresent": bool(handles),
                       "materialHandle": handles[0] if handles else None,
                       "selectedEffectiveBinding": "null"})
    assert len(layers) == 2 and {row["name"].upper() for row in layers} == {"0", "DEFPOINTS"}, "physical LAYER census"
    lines = [r for scope, r in records if scope == "ENTITIES" and r[0].value == "LINE"]
    assert len(lines) == 1, "physical LINE census"
    assert tuple(lines[0].get_first_value(10)) == (1, 2, 3), "physical LINE start"
    assert tuple(lines[0].get_first_value(11)) == (4, 5, 6), "physical LINE end"
    materials = [{"handle": r.get_first_value(5), "name": r.get_first_value(1)}
                 for scope, r in records if scope == "OBJECTS" and r[0].value == "MATERIAL"]
    if source_recipe:
        assert not materials, "selected source unexpectedly contains MATERIAL bodies"
    return {"layerMaterialBindings": layers, "materialObjects": materials,
            "lineCount": len(lines), "beforeDocumentSetup": True,
            "beforeExplicitAuditOrRecovery": True}


def author_null_material_bindings(doc):
    """Author null bindings before omitting unused defaults; keep the lineage."""
    bindings = []
    for layer in doc.layers:
        bindings.append({"name": layer.dxf.name,
                         "originalMaterialHandle": layer.dxf.get("material_handle"),
                         "selectedMaterialHandle": "0"})
        layer.dxf.material_handle = "0"
    omitted = [{"name": key, "handle": obj.dxf.handle} for key, obj in doc.materials]
    for row in omitted:
        doc.materials.delete(row["name"])
    return bindings, omitted


def physical_scale_projection(ezdxf, path, binary, expected, source_recipe=False):
    """Inspect the actual file through low-level tags before document setup."""
    from ezdxf.lldxf.extendedtags import ExtendedTags
    from ezdxf.tools.text import caret_decode

    records = selected_physical_records(path, binary)
    controls = selected_physical_controls(records, source_recipe)
    objects = [record for scope, record in records if scope == "OBJECTS"]
    dictionaries = [ExtendedTags(record) for record in objects
                    if record[0].value == "DICTIONARY"]
    roots = [d for d in dictionaries
             if int(d.subclasses[0].get_first_value(330, "0"), 16) == 0]
    assert len(roots) == 1, "physical root dictionary census"
    root = roots[0]
    root_handle = root.subclasses[0].get_first_value(5)
    dictionary_handle = selected_dictionary_entries(root)["ACAD_SCALELIST"]["handle"]
    selected = [d for d in dictionaries
                if d.subclasses[0].get_first_value(5) == dictionary_handle]
    assert len(selected) == 1, "physical SCALE dictionary target"
    dictionary = selected[0]
    assert dictionary.subclasses[0].get_first_value(330) == root_handle, "physical dictionary owner"
    targets = selected_dictionary_entries(dictionary)
    assert list(targets) == [PEER_SCALE_KEY], "physical SCALE dictionary keys"
    scales = [ExtendedTags(record) for record in objects if record[0].value == "SCALE"]
    assert len(scales) == 1, "physical SCALE census"
    scale = scales[0]
    scale_handle = scale.subclasses[0].get_first_value(5)
    assert targets[PEER_SCALE_KEY]["handle"] == scale_handle, "physical SCALE key target"
    assert scale.subclasses[0].get_first_value(330) == dictionary_handle, "physical SCALE owner"
    fields = {}
    for code in (70, 300, 140, 141, 290):
        values = [tag.value for tag in scale.get_subclass("AcDbScale") if tag.code == code]
        assert len(values) == 1, "physical SCALE field count %d" % code
        fields[code] = values[0]
    assert isinstance(fields[70], int) and isinstance(fields[290], int), "physical SCALE integer types"
    assert isinstance(fields[140], float) and isinstance(fields[141], float), "physical SCALE real types"
    fields[300] = ezdxf.decode_dxf_unicode(fields[300])
    if not binary:
        fields[300] = caret_decode(fields[300])
    assert fields == expected, "physical SCALE fields: %r != %r" % (fields, expected)
    classes = [r for scope, r in records if scope == "CLASSES" and r[0].value == "CLASS"
               and r.get_first_value(1) == "SCALE"]
    assert len(classes) == 1 and classes[0].get_first_value(2) == "AcDbScale", "physical SCALE class"
    return {"fields": fields, "key": PEER_SCALE_KEY, "scaleHandle": scale_handle,
            "dictionaryHandle": dictionary_handle, "rootHandle": root_handle,
            "dictionaryPointerRole": targets[PEER_SCALE_KEY]["role"],
            "scaleCount": len(scales), **controls}


def selected_scale_projection(ezdxf, path, binary, expected, source_recipe=False):
    """Independent physical projection, then ordinary read with setup disclosed.

    ezdxf 1.4.5 retains SCALE as DXFTagStorage rather than a typed annotation
    object. These checks establish selected field values, not native semantics.
    """
    from ezdxf.tools.text import caret_decode

    physical = physical_scale_projection(ezdxf, path, binary, expected, source_recipe)
    doc = ezdxf.readfile(path)
    dictionary = doc.rootdict["ACAD_SCALELIST"]
    assert dictionary.dxftype() == "DICTIONARY", "scale dictionary type"
    assert dictionary.dxf.owner == doc.rootdict.dxf.handle, "scale dictionary owner"
    assert list(dictionary.keys()) == [PEER_SCALE_KEY], "scale dictionary keys"
    scales = [e for e in doc.objects if e.dxftype() == "SCALE"]
    assert len(scales) == 1, "original/converted SCALE census"
    scale = dictionary[PEER_SCALE_KEY]
    assert scale is scales[0], "scale dictionary target identity"
    assert scale.dxf.owner == dictionary.dxf.handle, "scale body owner"
    body = scale.xtags.get_subclass("AcDbScale")
    fields = {}
    for code in (70, 300, 140, 141, 290):
        values = [tag.value for tag in body if tag.code == code]
        assert len(values) == 1, "SCALE field presence/count %d" % code
        fields[code] = values[0]
    assert isinstance(fields[70], int) and isinstance(fields[290], int), "SCALE integer widths"
    assert isinstance(fields[140], float) and isinstance(fields[141], float), "SCALE real widths"
    assert isinstance(fields[300], str), "SCALE name type"
    # One CIF decode for either wire format, followed by one ASCII-only caret
    # decode. A literal backslash-U name must not be decoded a second time.
    fields[300] = ezdxf.decode_dxf_unicode(fields[300])
    if not binary:
        fields[300] = caret_decode(fields[300])
    assert fields == expected, "SCALE fields: %r != %r" % (fields, expected)
    assert fields[300] != PEER_SCALE_KEY, "dictionary key/name distinction"
    lines = list(doc.modelspace().query("LINE"))
    assert len(lines) == 1, "selected LINE census"
    assert tuple(lines[0].dxf.start) == (1, 2, 3), "selected LINE start"
    assert tuple(lines[0].dxf.end) == (4, 5, 6), "selected LINE end"
    assert any(c.dxf.name == "SCALE" for c in doc.classes), "SCALE CLASS entry"
    material_handles = {row["handle"] for row in physical["materialObjects"]}
    added_materials = [{"handle": obj.dxf.handle, "name": obj.dxf.name}
                       for obj in doc.objects if obj.dxftype() == "MATERIAL"
                       and obj.dxf.handle not in material_handles]
    return {"fields": fields, "key": PEER_SCALE_KEY, "scaleHandle": scale.dxf.handle,
            "scaleOwner": scale.dxf.owner, "dictionaryHandle": dictionary.dxf.handle,
            "dictionaryOwner": dictionary.dxf.owner, "rootHandle": doc.rootdict.dxf.handle,
            "line3d": True, "physicalBeforeDocumentSetup": physical,
            "ordinaryReadAfterDocumentSetup": True, "explicitAuditOrRecoveryRun": False,
            "documentSetupAddedMaterials": added_materials,
            "documentSetupAddedMaterialCount": len(added_materials),
            "representation": type(scale).__name__, "nativeSemanticsQualified": False}


def qualify_scales(ezdxf, probe, converter, output):
    """Author a small generic-tag matrix and check actual unchanged/edited output."""
    from ezdxf.entities import factory
    from ezdxf.lldxf.extendedtags import ExtendedTags

    inputs, round_trips, edits = [], [], []
    profiles = (("R2000", "cp1252", "Peer café scale"),
                ("R2000", "cp932", "Peer 表タ scale"),
                ("R2007", "utf-8", "Peer Ω scale"),
                ("R2018", "utf-8", "Peer café Ω scale"))
    for version, page, name in profiles:
        doc = ezdxf.new(version)
        if version == "R2000":
            doc.encoding = page
        # Author a primitive/SCALE profile with explicit null material bindings,
        # then omit the unused defaults. Deleting them first leaves modern
        # LAYER347 dangling; ordinary peer document setup can hide that defect
        # by creating new defaults. The physical pre-setup check forbids it.
        null_layer_bindings, omitted_material_handles = author_null_material_bindings(doc)
        omitted_materials = [row["name"] for row in omitted_material_handles]
        dictionary = doc.rootdict.get_required_dict("ACAD_SCALELIST")
        handle = doc.entitydb.next_handle()
        tags = ExtendedTags.from_text(
            "0\nSCALE\n5\n%s\n330\n%s\n100\nAcDbScale\n"
            "70\n0\n300\n%s\n140\n1.25\n141\n1.25\n290\n1\n"
            % (handle, dictionary.dxf.handle, name))
        scale = factory.load(tags, doc)
        factory.bind(scale, doc)
        doc.objects.add_object(scale)
        dictionary.add(PEER_SCALE_KEY, scale)
        doc.modelspace().add_line((1, 2, 3), (4, 5, 6))
        expected = {70: 0, 300: name, 140: 1.25, 141: 1.25, 290: 1}
        for fmt in ("asc", "bin"):
            stem = "scale-" + doc.dxfversion + "-" + page + "-" + fmt
            source = output / (stem + ".dxf")
            doc.saveas(source, fmt=fmt)
            original = {"version": doc.dxfversion, "page": page, "format": fmt,
                        "path": source.name, "sha256": digest(source), "matched": False,
                        "omittedUnusedDefaultMaterials": omitted_materials,
                        "omittedDefaultMaterialHandles": omitted_material_handles,
                        "selectedNullLayerBindings": null_layer_bindings,
                        "bindingIntent": "Authored SCALE/LINE profile with no material dependency; not the original default document"}
            try:
                original["observations"] = selected_scale_projection(
                    ezdxf, source, fmt == "bin", expected, source_recipe=True)
                original["matched"] = True
            except Exception as error:
                original["mismatch"] = str(error)
            inputs.append(original)
            unchanged, edited = selected_resource_outputs(
                ezdxf, probe, converter, source, version, page, fmt, original,
                "--scale-edit-probe", selected_scale_projection, expected, PEER_SCALE_EDIT)
            round_trips.extend(unchanged)
            edits.extend(edited)
    return inputs, round_trips, edits


def selected_resource_outputs(ezdxf, probe, converter, source, version, page, fmt,
                              original, selector, projection, expected, edited_expected,
                              modes=(False, True), edit_prefix="edit-", edit_arguments=()):
    """Reuse the same bounded CLI/edit invocation for the selected resources."""
    round_trips, edits = [], []
    for target_format in ("asc", "bin"):
        for edit in modes:
            target = source.parent / ((edit_prefix if edit else "out-")
                                      + source.stem + "-" + target_format + ".dxf")
            if edit:
                command = [str(probe), selector, str(source), str(target), target_format,
                           *edit_arguments]
            else:
                command = [str(converter), str(source), "-y", "-v" + version[1:]]
                if target_format == "bin":
                    command.append("-b")
                command.append(str(target))
            row = {"version": original["version"], "page": page, "input": fmt,
                   "output": target_format, "sourceSha256": original["sha256"],
                   "path": target.name, "command": command, "matched": False}
            try:
                run = subprocess.run(command, capture_output=True, text=True, timeout=60)
                row.update({"exitCode": run.returncode, "stdout": run.stdout,
                            "stderr": run.stderr})
                if run.returncode == 0:
                    row["sha256"] = digest(target)
                    row["observations"] = projection(
                        ezdxf, target, target_format == "bin", edited_expected if edit else expected)
                    row["matched"] = original["matched"]
            except subprocess.TimeoutExpired:
                row.update({"exitCode": 124, "mismatch": "Selected resource process timeout (60s)"})
            except Exception as error:
                row["mismatch"] = str(error)
            (edits if edit else round_trips).append(row)
    return round_trips, edits


def mline_style_values(style):
    return {"name": style.dxf.name, "flags": style.dxf.flags,
            "description": style.dxf.description, "fillColor": style.dxf.fill_color,
            "startAngle": style.dxf.start_angle, "endAngle": style.dxf.end_angle,
            "elements": [(e.offset, e.color, e.linetype) for e in style.elements]}


def selected_semantic_text(ezdxf, value, binary):
    """Decode each transport layer once, including names and reference names."""
    from ezdxf.tools.text import caret_decode

    value = ezdxf.decode_dxf_unicode(value)
    return value if binary else caret_decode(value)


def mline_consumer_values(entity):
    """Freeze authored or ordinarily loaded geometry without regeneration."""
    return {"styleName": entity.dxf.style_name, "scale": entity.dxf.scale_factor,
            "justification": entity.dxf.justification, "flags": entity.dxf.flags,
            "count": len(entity.vertices), "elementCount": entity.dxf.style_element_count,
            "start": tuple(entity.dxf.start_location), "extrusion": tuple(entity.dxf.extrusion),
            "layer": entity.dxf.layer,
            "vertices": [{"location": tuple(vertex.location),
                          "direction": tuple(vertex.line_direction), "miter": tuple(vertex.miter_direction),
                          "lineParameters": [tuple(values) for values in vertex.line_params],
                          "fillParameters": [tuple(values) for values in vertex.fill_params]}
                         for vertex in entity.vertices]}


def physical_mline_consumers(ezdxf, records, binary, style_handle, expected):
    """Bind original physical 340/owner roles and independently parse all tuples."""
    from ezdxf.lldxf.extendedtags import ExtendedTags

    missing = object()

    def one(record, code, default=missing):
        values = [tag.value for tag in record if tag.code == code]
        assert len(values) <= 1, "physical MLINE duplicate code %d" % code
        if not values:
            assert default is not missing, "physical MLINE missing code %d" % code
            return default
        return values[0]

    controls = [record for scope, record in records
                if scope == "TABLES" and record[0].value == "TABLE"
                and record.get_first_value(2, None) == "BLOCK_RECORD"]
    assert len(controls) == 1, "physical BLOCK_RECORD control census"
    control_handle = one(controls[0], 5)
    # This selected adapter route has nonnull 32-bit identities. UINT32_MAX is
    # included; it is not a universal format-level null or sentinel value.
    assert 0 < int(control_handle, 16) <= 0xFFFFFFFF, "physical BLOCK_RECORD control identity"
    assert 0 < int(style_handle, 16) <= 0xFFFFFFFF, "physical MLINESTYLE target identity"
    assert int(control_handle, 16) != int(style_handle, 16), "physical control/style identity collision"
    identities = {int(control_handle, 16), int(style_handle, 16)}
    block_records = {}
    for scope, record in records:
        if scope != "TABLES" or record[0].value != "BLOCK_RECORD":
            continue
        name, handle, owner = one(record, 2), one(record, 5), one(record, 330)
        identity = int(handle, 16)
        assert name not in block_records, "physical BLOCK_RECORD name collision"
        assert 0 < identity <= 0xFFFFFFFF and identity not in identities, "physical BLOCK_RECORD identity collision"
        assert int(owner, 16) == int(control_handle, 16), "physical BLOCK_RECORD owner/control association"
        block_records[name] = handle
        identities.add(identity)
    actual, active_block, block_frames = {}, None, {}
    for scope, record in records:
        kind = record[0].value
        if scope == "BLOCKS" and kind == "BLOCK":
            assert active_block is None, "physical nested BLOCK"
            active_block = one(record, 2)
            assert active_block in block_records and active_block not in block_frames, "physical BLOCK target census"
            assert int(one(record, 330), 16) == int(block_records[active_block], 16), "physical BLOCK owner"
            handle = one(record, 5)
            identity = int(handle, 16)
            assert 0 < identity <= 0xFFFFFFFF and identity not in identities, "physical BLOCK identity collision"
            identities.add(identity)
            block_frames[active_block] = {"beginHandle": handle, "endHandle": None,
                                          "beginOwner": one(record, 330), "endOwner": None}
        elif scope == "BLOCKS" and kind == "ENDBLK":
            assert active_block is not None, "physical ENDBLK without BLOCK"
            assert int(one(record, 330), 16) == int(block_records[active_block], 16), "physical ENDBLK owner"
            handle = one(record, 5)
            identity = int(handle, 16)
            assert 0 < identity <= 0xFFFFFFFF and identity not in identities, "physical ENDBLK identity collision"
            identities.add(identity)
            block_frames[active_block]["endHandle"] = handle
            block_frames[active_block]["endOwner"] = one(record, 330)
            active_block = None
        elif kind == "MLINE":
            block = "*Model_Space" if scope == "ENTITIES" else active_block
            role = scope + ":" + str(block)
            assert role in expected and role not in actual, "physical MLINE source scope/census"
            handle, owner = one(record, 5), one(record, 330)
            identity = int(handle, 16)
            assert 0 < identity <= 0xFFFFFFFF and identity not in identities, "physical MLINE identity collision"
            identities.add(identity)
            assert int(owner, 16) == int(block_records[block], 16), "physical MLINE BLOCK_RECORD owner"
            body = ExtendedTags(record).get_subclass("AcDbMline")
            assert sum(tag.code == 100 and tag.value == "AcDbMline" for tag in record) == 1, "physical MLINE body multiplicity"
            assert int(one(body, 340), 16) == int(style_handle, 16), "physical MLINE authoritative style target"
            first = next((i for i, tag in enumerate(body) if tag.code == 11), len(body))
            header = body[:first]
            assert all(tag.code in (100, 2, 340, 40, 70, 71, 72, 73, 10, 210) for tag in header), "physical MLINE header grammar"
            element_count = one(header, 73)
            fields = {"styleName": selected_semantic_text(ezdxf, one(header, 2), binary),
                      "scale": one(header, 40), "justification": one(header, 70),
                      "flags": one(header, 71), "count": one(header, 72),
                      "elementCount": element_count, "start": tuple(one(header, 10)),
                      "extrusion": tuple(one(header, 210, (0.0, 0.0, 1.0))),
                      "layer": selected_semantic_text(ezdxf, one(record, 8), binary), "vertices": []}
            cursor = first

            def take(code):
                nonlocal cursor
                assert cursor < len(body) and body[cursor].code == code, "physical MLINE tuple code %d" % code
                value = body[cursor].value
                cursor += 1
                return value

            while cursor < len(body):
                vertex = {"location": tuple(take(11)), "direction": tuple(take(12)),
                          "miter": tuple(take(13)), "lineParameters": [], "fillParameters": []}
                for _ in range(element_count):
                    line_count = take(74)
                    assert 0 <= line_count <= len(body) - cursor, "physical MLINE line-parameter count"
                    vertex["lineParameters"].append(tuple(take(41) for _ in range(line_count)))
                    fill_count = take(75)
                    assert 0 <= fill_count <= len(body) - cursor, "physical MLINE fill-parameter count"
                    vertex["fillParameters"].append(tuple(take(42) for _ in range(fill_count)))
                fields["vertices"].append(vertex)
            assert fields["count"] == len(fields["vertices"]), "physical MLINE vertex count"
            assert fields == expected[role], "physical MLINE original fields/geometry %s" % role
            actual[role] = {"fields": fields, "handle": handle, "owner": owner,
                            "styleHandle": one(body, 340), "blockRecord": block_records[block]}
    assert active_block is None and set(actual) == set(expected), "physical MLINE complete scope census"
    return {"consumers": actual, "blockRecords": block_records,
            "blockRecordControl": control_handle, "blockFrames": block_frames,
            "consumerCount": len(actual), "beforeDocumentSetup": True,
            "geometryRegenerated": False, "nativeMlineConsumerQualified": False}


def physical_mline_style_projection(ezdxf, path, binary, expected, source_recipe=False,
                                    expected_presence=None, require_linetypes=False,
                                    consumer_expected=None, consumer_style_key=None):
    """Check every selected style and its ordered element tuples before setup."""
    from ezdxf.lldxf.extendedtags import ExtendedTags
    records = selected_physical_records(path, binary)
    controls = selected_physical_controls(records, source_recipe)
    objects = [ExtendedTags(record) for scope, record in records if scope == "OBJECTS"]
    dictionaries = [r for r in objects if r.dxftype() == "DICTIONARY"]
    roots = [d for d in dictionaries
             if int(d.subclasses[0].get_first_value(330, "0"), 16) == 0]
    assert len(roots) == 1, "physical MLINESTYLE root census"
    root = roots[0]
    root_handle = root.subclasses[0].get_first_value(5)
    dictionary_handle = selected_dictionary_entries(root)["ACAD_MLINESTYLE"]["handle"]
    selected = [d for d in dictionaries
                if d.subclasses[0].get_first_value(5) == dictionary_handle]
    assert len(selected) == 1, "physical MLINESTYLE dictionary target"
    dictionary = selected[0]
    assert dictionary.subclasses[0].get_first_value(330) == root_handle, "physical MLINESTYLE dictionary owner"
    raw_targets = selected_dictionary_entries(dictionary)
    targets = {selected_semantic_text(ezdxf, key, binary): {**value, "wireKey": key}
               for key, value in raw_targets.items()}
    assert len(targets) == len(raw_targets), "physical MLINESTYLE decoded key collision"
    assert set(targets) == set(expected), "physical MLINESTYLE keys"
    linetypes, linetype_control = {}, None
    if require_linetypes:
        table_controls = [record for scope, record in records
                          if scope == "TABLES" and record[0].value == "TABLE"
                          and record.get_first_value(2, None) == "LTYPE"]
        assert len(table_controls) == 1, "physical LTYPE TABLE control census"
        control_handles = [tag.value for tag in table_controls[0] if tag.code == 5]
        assert len(control_handles) == 1, "physical LTYPE TABLE control handle count"
        control_id = int(control_handles[0], 16)
        assert 0 < control_id <= 0xFFFFFFFF, "physical LTYPE TABLE control handle"
        linetype_control = {"handle": control_handles[0], "recordCount": 0}
        identities = {control_id}
        for scope, record in records:
            if scope != "TABLES" or record[0].value != "LTYPE":
                continue
            handles = [tag.value for tag in record if tag.code == 5]
            owners = [tag.value for tag in record if tag.code == 330]
            assert len(handles) == 1, "physical LTYPE record handle count"
            assert len(owners) == 1, "physical LTYPE record owner count"
            identity = int(handles[0], 16)
            assert 0 < identity <= 0xFFFFFFFF and identity not in identities, "physical LTYPE duplicate/null identity"
            assert int(owners[0], 16) == control_id, "physical LTYPE owner/control association"
            identities.add(identity)
            wire_name = record.get_first_value(2)
            name = selected_semantic_text(ezdxf, wire_name, binary)
            assert name not in linetypes, "physical LTYPE decoded name collision"
            linetypes[name] = {"handle": handles[0], "owner": owners[0], "wireName": wire_name}
            linetype_control["recordCount"] += 1
    styles = [r for r in objects if r.dxftype() == "MLINESTYLE"]
    assert len(styles) == len(expected), "physical MLINESTYLE census"
    observations = {}
    for key, target in targets.items():
        candidates = [r for r in styles if r.subclasses[0].get_first_value(5) == target["handle"]]
        assert len(candidates) == 1, "physical MLINESTYLE target identity"
        style = candidates[0]
        assert style.subclasses[0].get_first_value(330) == dictionary_handle, "physical MLINESTYLE owner"
        body = style.get_subclass("AcDbMlineStyle")
        counts = [i for i, tag in enumerate(body) if tag.code == 71]
        assert len(counts) == 1, "physical MLINESTYLE element count presence"
        prefix, tail = body[:counts[0]], body[counts[0] + 1:]
        values, presence = {}, {}
        for name, code, default in (("name", 2, "Standard"), ("flags", 70, 0),
                                    ("description", 3, ""), ("fillColor", 62, 256),
                                    ("startAngle", 51, 90.0), ("endAngle", 52, 90.0)):
            filed = [t.value for t in prefix if t.code == code]
            assert len(filed) <= 1, "physical MLINESTYLE duplicate field %d" % code
            presence[code] = bool(filed)
            value = filed[0] if filed else default
            if name in ("name", "description"):
                value = selected_semantic_text(ezdxf, value, binary)
            values[name] = value
        if expected_presence and key in expected_presence:
            for code, present in expected_presence[key].items():
                assert presence[code] == present, "physical MLINESTYLE %s field %d presence" % (key, code)
        assert len(values["description"]) <= 255, "physical MLINESTYLE description characters"
        elements, current = [], None
        for code, value in tail:
            if code == 49:
                if current is not None:
                    elements.append((current[49], current.get(62, 0), current.get(6, "BYLAYER")))
                current = {49: value}
            elif code in (62, 6):
                assert current is not None and code not in current, "physical MLINESTYLE tuple field"
                current[code] = selected_semantic_text(ezdxf, value, binary) if code == 6 else value
            else:
                raise AssertionError("unexpected selected MLINESTYLE tuple code %d" % code)
        if current is not None:
            elements.append((current[49], current.get(62, 0), current.get(6, "BYLAYER")))
        assert len(elements) == body[counts[0]].value, "physical MLINESTYLE declared/actual elements"
        values["elements"] = elements
        assert values == expected[key], "physical MLINESTYLE %s: %r != %r" % (key, values, expected[key])
        references = []
        if require_linetypes:
            for _, _, name in elements:
                if name.upper() in ("BYLAYER", "BYBLOCK"):
                    references.append({"name": name, "disposition": "inherited"})
                else:
                    assert name in linetypes, "physical MLINESTYLE LTYPE target"
                    references.append({"name": name, "disposition": "defined",
                                       **linetypes[name]})
        observations[key] = {"fields": values, "handle": target["handle"],
                             "owner": dictionary_handle, "pointerRole": target["role"],
                             "wireDictionaryKey": target["wireKey"],
                             "wireBodyName": next((t.value for t in prefix if t.code == 2), "Standard"),
                             "wireElementLinetypes": [t.value for t in tail if t.code == 6],
                             "fieldPresence": presence, "elementCount": len(elements),
                             "linetypeReferences": references}
    consumers = None if consumer_expected is None else physical_mline_consumers(
        ezdxf, records, binary, observations[consumer_style_key]["handle"], consumer_expected)
    return {"styles": observations, "styleCount": len(styles), "rootHandle": root_handle,
            "dictionaryHandle": dictionary_handle, "linetypeDefinitions": linetypes,
            "linetypeControl": linetype_control, "linetypeTargetsChecked": require_linetypes,
            "mlineConsumers": consumers, **controls}


def selected_mline_style_projection(ezdxf, path, binary, expected, source_recipe=False,
                                   expected_presence=None, require_linetypes=False,
                                   consumer_expected=None, consumer_style_key=None):
    physical = physical_mline_style_projection(
        ezdxf, path, binary, expected, source_recipe, expected_presence, require_linetypes,
        consumer_expected, consumer_style_key)
    doc = ezdxf.readfile(path)
    dictionary = doc.rootdict["ACAD_MLINESTYLE"]
    assert dictionary.dxf.owner == doc.rootdict.dxf.handle, "MLINESTYLE dictionary owner after setup"
    named_styles = {selected_semantic_text(ezdxf, name, binary): style
                    for name, style in dictionary.items()}
    assert len(named_styles) == len(dictionary), "MLINESTYLE decoded key collision after setup"
    assert set(named_styles) == set(expected), "MLINESTYLE keys after setup"
    styles = [e for e in doc.objects if e.dxftype() == "MLINESTYLE"]
    assert len(styles) == len(expected), "MLINESTYLE census after setup"
    for name, wanted in expected.items():
        style = named_styles[name]
        assert style in styles and style.dxf.owner == dictionary.dxf.handle, "MLINESTYLE typed owner"
        values = mline_style_values(style)
        for key in ("name", "description"):
            values[key] = selected_semantic_text(ezdxf, values[key], binary)
        values["elements"] = [(offset, color, selected_semantic_text(ezdxf, linetype, binary))
                              for offset, color, linetype in values["elements"]]
        assert values == wanted, "MLINESTYLE typed fields %s" % name
    if require_linetypes:
        typed_linetypes = {selected_semantic_text(ezdxf, entry.dxf.name, binary): entry
                           for entry in doc.linetypes}
        assert len(typed_linetypes) == len(doc.linetypes), "typed LTYPE decoded name collision"
        for row in physical["styles"].values():
            for reference in row["linetypeReferences"]:
                if reference["disposition"] == "defined":
                    target = typed_linetypes[reference["name"]]
                    assert target.dxf.handle == reference["handle"], "typed LTYPE target identity"
                    assert target.dxf.owner == reference["owner"], "typed LTYPE target owner"
    if consumer_expected is not None:
        target = named_styles[consumer_style_key]
        typed_consumers = {}
        for layout in [doc.modelspace(), *(block for block in doc.blocks
                                          if block.name not in ("*Model_Space", "*Paper_Space"))]:
            for entity in layout.query("MLINE"):
                role = ("ENTITIES:*Model_Space" if layout is doc.modelspace()
                        else "BLOCKS:" + layout.name)
                assert role in consumer_expected and role not in typed_consumers, "typed MLINE scope/census"
                values = mline_consumer_values(entity)
                values["styleName"] = selected_semantic_text(ezdxf, values["styleName"], binary)
                values["layer"] = selected_semantic_text(ezdxf, values["layer"], binary)
                assert values == consumer_expected[role], "typed MLINE fields/geometry %s" % role
                row = physical["mlineConsumers"]["consumers"][role]
                assert entity.dxf.handle == row["handle"] and entity.dxf.owner == row["owner"], "typed MLINE self/owner"
                assert entity.style is target and entity.dxf.style_handle == row["styleHandle"], "typed MLINE 340 target identity"
                typed_consumers[role] = entity.dxf.handle
        assert set(typed_consumers) == set(consumer_expected), "typed MLINE complete scope census"
    original_materials = {row["handle"] for row in physical["materialObjects"]}
    added_materials = [{"handle": e.dxf.handle, "name": e.dxf.name}
                       for e in doc.objects if e.dxftype() == "MATERIAL"
                       and e.dxf.handle not in original_materials]
    return {"physicalBeforeDocumentSetup": physical, "ordinaryReadAfterDocumentSetup": True,
            "ordinaryReadEncoding": doc.encoding, "ordinaryReadVersion": doc.dxfversion,
            "explicitAuditOrRecoveryRun": False, "documentSetupAddedMaterials": added_materials,
            "documentSetupAddedMaterialCount": len(added_materials),
            "typedStylesMatched": len(styles), "nativeMlineConsumerQualified": False}


def author_mline_style_profile(ezdxf, version, page, description,
                               start_angle=37.5, end_angle=142.5,
                               style_name=PEER_MLINE_STYLE_NAME,
                               linetype_name="CONTINUOUS"):
    doc = ezdxf.new(version)
    if version == "R2000":
        doc.encoding = page
    null_bindings, omitted = author_null_material_bindings(doc)
    if linetype_name != "CONTINUOUS":
        doc.linetypes.new(linetype_name, dxfattribs={
            "description": "authored selected continuous reference", "pattern": [0.0]})
    style = doc.mline_styles.new(style_name)
    style.dxf.update({"description": description, "flags": 3, "fill_color": 4,
                      "start_angle": start_angle, "end_angle": end_angle})
    style.elements.append(1.25, 3, linetype_name)
    style.elements.append(-0.75, 5, "BYLAYER")
    doc.modelspace().add_line((1, 2, 3), (4, 5, 6))
    expected = {name: mline_style_values(value) for name, value in doc.mline_styles}
    return doc, style, expected, null_bindings, omitted


def qualify_mline_styles(ezdxf, probe, converter, output):
    inputs, round_trips, edits = [], [], []
    profiles = (("R2000", "cp1252", "Peer café mline"),
                ("R2000", "cp932", "Peer 表タ mline"),
                ("R2007", "utf-8", "Peer Ω mline"),
                ("R2018", "utf-8", "Peer café Ω mline"))
    for version, page, description in profiles:
        doc, _, expected, null_bindings, omitted = author_mline_style_profile(
            ezdxf, version, page, description)
        edited_expected = {name: dict(value) for name, value in expected.items()}
        edited_expected[PEER_MLINE_STYLE_NAME]["description"] = PEER_MLINE_STYLE_EDIT
        for fmt in ("asc", "bin"):
            source = output / ("mline-style-" + doc.dxfversion + "-" + page + "-" + fmt + ".dxf")
            doc.saveas(source, fmt=fmt)
            original = {"version": doc.dxfversion, "page": page, "format": fmt,
                        "path": source.name, "sha256": digest(source), "matched": False,
                        "selectedNullLayerBindings": null_bindings,
                        "omittedDefaultMaterialHandles": omitted,
                        "bindingIntent": "Typed MLINESTYLE definitions and LINE; no MLINE consumer or default MATERIAL qualification"}
            try:
                original["observations"] = selected_mline_style_projection(
                    ezdxf, source, fmt == "bin", expected, source_recipe=True)
                original["matched"] = True
            except Exception as error:
                original["mismatch"] = str(error)
            inputs.append(original)
            unchanged, edited = selected_resource_outputs(
                ezdxf, probe, converter, source, version, page, fmt, original,
                "--mline-style-edit-probe", selected_mline_style_projection, expected, edited_expected)
            round_trips.extend(unchanged)
            edits.extend(edited)
    return inputs, round_trips, edits


def qualify_mline_style_boundaries(ezdxf, probe, converter, output):
    """Four authored angle boundaries, separated from the original 8/16/16 lane."""
    from functools import partial

    class OmittedAnglesWriter:
        # This is a generation-time filter for this one authored style, not
        # normalization of a downloaded or previously qualified input file.
        def __init__(self, writer, omitted, counts):
            self.writer, self.omitted, self.counts = writer, omitted, counts

        def __getattr__(self, name):
            return getattr(self.writer, name)

        def write_tag(self, tag):
            if tag.code in self.omitted:
                self.counts[tag.code] += 1
            else:
                self.writer.write_tag(tag)

        def write_tag2(self, code, value):
            if code in self.omitted:
                self.counts[code] += 1
            else:
                self.writer.write_tag2(code, value)

    inputs, round_trips, edits, scalar_edits = [], [], [], []
    cases = (("both-missing", "R2000", "cp1252", "asc", (51, 52), 90.0, 90.0),
             ("start-missing", "R2007", "utf-8", "bin", (51,), 90.0, 142.5),
             ("end-missing", "R2018", "utf-8", "asc", (52,), 37.5, 90.0),
             ("explicit-zero", "R2000", "cp932", "bin", (), 0.0, 0.0))
    for case, version, page, fmt, missing, start, end in cases:
        description = "Boundary 表タ mline" if page == "cp932" else "Boundary café mline"
        doc, style, expected, null_bindings, omitted = author_mline_style_profile(
            ezdxf, version, page, description, start, end)
        exported_omissions = {code: 0 for code in missing}
        original_export = style.export_entity
        if missing:
            style.export_entity = lambda writer: original_export(
                OmittedAnglesWriter(writer, missing, exported_omissions))
        source = output / ("mline-style-boundary-" + case + "-" + doc.dxfversion
                           + "-" + page + "-" + fmt + ".dxf")
        doc.saveas(source, fmt=fmt)
        assert all(count == 1 for count in exported_omissions.values()), "authored angle omission counts"
        presence = {PEER_MLINE_STYLE_NAME: {code: code not in missing for code in (51, 52)}}
        projection = partial(selected_mline_style_projection, expected_presence=presence)
        original = {"case": case, "version": doc.dxfversion, "page": page, "format": fmt,
                    "path": source.name, "sha256": digest(source), "matched": False,
                    "selectedNullLayerBindings": null_bindings, "omittedDefaultMaterialHandles": omitted,
                    "authoredOmittedAngleCodes": list(missing), "exportedOmissionCounts": exported_omissions,
                    "recipe": "Typed authored style with scoped generation-time angle-tag omission; independent defaults are 90 degrees; explicit zero remains zero"}
        try:
            original["observations"] = projection(ezdxf, source, fmt == "bin", expected, source_recipe=True)
            original["matched"] = True
        except Exception as error:
            original["mismatch"] = str(error)
        inputs.append(original)
        description_expected = {name: dict(value) for name, value in expected.items()}
        description_expected[PEER_MLINE_STYLE_NAME]["description"] = PEER_MLINE_STYLE_EDIT
        unchanged, edited = selected_resource_outputs(
            ezdxf, probe, converter, source, version, page, fmt, original,
            "--mline-style-edit-probe", projection, expected, description_expected)
        for row in unchanged + edited:
            row["case"] = case
        round_trips.extend(unchanged)
        edits.extend(edited)
        scalar_expected = {name: dict(value) for name, value in expected.items()}
        scalar_expected[PEER_MLINE_STYLE_NAME].update(PEER_MLINE_STYLE_SCALAR_EDIT)
        scalar_presence = {PEER_MLINE_STYLE_NAME: {code: True for code in (3, 70, 62, 51, 52)}}
        _, scalar = selected_resource_outputs(
            ezdxf, probe, converter, source, version, page, fmt, original,
            "--mline-style-scalar-probe",
            partial(selected_mline_style_projection, expected_presence=scalar_presence),
            expected, scalar_expected, modes=(True,), edit_prefix="scalar-edit-")
        for row in scalar:
            row["case"] = case
        scalar_edits.extend(scalar)
    return inputs, round_trips, edits, scalar_edits


def qualify_mline_style_text_boundaries(ezdxf, probe, converter, output):
    """Two unchanged-name sources; format changes and one legacy page change.

    The producer accepts semantic caret names but does not itself caret-escape
    them. Author that transport layer while the three selected owning entities
    are being written, before an input file or its digest exists. A legacy
    unrepresentable Omega is independently CIF-encoded by ezdxf's dxfreplace.
    """
    from functools import partial

    class SelectedTextWriter:
        def __init__(self, writer, selected, binary, counts):
            self.writer, self.selected = writer, selected
            self.binary, self.counts = binary, counts

        def __getattr__(self, name):
            return getattr(self.writer, name)

        def write_tag(self, tag):
            self.write_tag2(tag.code, tag.value)

        def write_tag2(self, code, value):
            role = self.selected.get((code, value)) if isinstance(value, str) else None
            if role:
                self.counts[role] += 1
                if not self.binary:
                    value = value.replace("^", "^ ")
            self.writer.write_tag2(code, value)

    inputs, round_trips, transcoded = [], [], []
    cases = (("legacy-cif-caret", "R2000", "cp1252", "asc", "café_Ω"),
             ("modern-unicode-caret", "R2007", "utf-8", "bin", "表タ_Ω"))
    projection = partial(selected_mline_style_projection, require_linetypes=True)
    for case, version, page, fmt, glyphs in cases:
        style_name, linetype_name = "Peer_" + glyphs + "^A", "Line_" + glyphs + "^A"
        doc, style, expected, null_bindings, omitted = author_mline_style_profile(
            ezdxf, version, page, "Unchanged text boundary", style_name=style_name,
            linetype_name=linetype_name)
        exported = {role: 0 for role in ("bodyName2", "elementLinetype6", "dictionaryKey3", "tableName2")}
        selected = ((style, {(2, style_name): "bodyName2", (6, linetype_name): "elementLinetype6"}),
                    (doc.rootdict["ACAD_MLINESTYLE"], {(3, style_name): "dictionaryKey3"}),
                    (doc.linetypes.get(linetype_name), {(2, linetype_name): "tableName2"}))
        for entity, slots in selected:
            original_export = entity.export_entity
            entity.export_entity = lambda writer, export=original_export, fields=slots: export(
                SelectedTextWriter(writer, fields, fmt == "bin", exported))
        source = output / ("mline-style-text-" + case + "-" + doc.dxfversion + "-" + fmt + ".dxf")
        doc.saveas(source, fmt=fmt)
        assert all(count == 1 for count in exported.values()), "authored selected text role counts"
        original = {"case": case, "version": doc.dxfversion, "page": page, "format": fmt,
                    "path": source.name, "sha256": digest(source), "matched": False,
                    "selectedNullLayerBindings": null_bindings, "omittedDefaultMaterialHandles": omitted,
                    "semanticStyleName": style_name, "semanticLinetypeName": linetype_name,
                    "exportedTextRoleCounts": exported,
                    "recipe": "Typed authored style/dictionary/LTYPE; scoped generation-time ASCII caret encoding, legacy CIF by producer; no post-generation source changes or name edits"}
        try:
            original["observations"] = projection(ezdxf, source, fmt == "bin", expected, source_recipe=True)
            original["matched"] = True
        except Exception as error:
            original["mismatch"] = str(error)
        inputs.append(original)
        unchanged, _ = selected_resource_outputs(
            ezdxf, probe, converter, source, version, page, fmt, original,
            "--mline-style-text-probe", projection, expected, expected, modes=(False,))
        for row in unchanged:
            row["case"] = case
        round_trips.extend(unchanged)
        if version != "R2000":
            continue
        for target_format in ("asc", "bin"):
            target = output / ("page-1251-" + source.stem + "-" + target_format + ".dxf")
            command = [str(probe), "--mline-style-text-probe", str(source), str(target),
                       target_format, "ANSI_1251"]
            row = {"case": case, "version": doc.dxfversion, "sourcePage": page,
                   "targetPage": "cp1251", "input": fmt, "output": target_format,
                   "sourceSha256": original["sha256"], "path": target.name,
                   "command": command, "matched": False}
            try:
                run = subprocess.run(command, capture_output=True, text=True, timeout=60)
                row.update({"exitCode": run.returncode, "stdout": run.stdout, "stderr": run.stderr})
                if run.returncode == 0:
                    row["sha256"] = digest(target)
                    row["observations"] = projection(ezdxf, target, target_format == "bin", expected)
                    assert row["observations"]["ordinaryReadEncoding"] == "cp1251", "output HEADER page is not cp1251"
                    assert row["observations"]["ordinaryReadVersion"] == doc.dxfversion, "text transport changed version"
                    row["matched"] = original["matched"]
            except subprocess.TimeoutExpired:
                row.update({"exitCode": 124, "mismatch": "Selected text process timeout (60s)"})
            except Exception as error:
                row["mismatch"] = str(error)
            transcoded.append(row)
    return inputs, round_trips, transcoded


def qualify_mline_style_key_authority(ezdxf, probe, converter, output):
    """Reuse real MLINE sources for independent dictionary-key and ACI edits."""
    inputs, round_trips, edits, aci_edits = [], [], [], []
    for fmt in ("asc", "bin"):
        doc, style, _, null_bindings, omitted = author_mline_style_profile(
            ezdxf, "R2013", "utf-8", "Dictionary authority boundary")
        block = doc.blocks.new("PeerMlineBlock")
        model_line = doc.modelspace().add_mline(
            [(1, 2, 3), (5, 2, 3), (5, 2, 8)],
            dxfattribs={"style_name": PEER_MLINE_STYLE_NAME, "scale_factor": 1.25,
                        "justification": 1, "extrusion": (0, 1, 0)})
        block_line = block.add_mline(
            [(10, 20, 30), (10, 24, 30), (10, 24, 35)],
            dxfattribs={"style_name": PEER_MLINE_STYLE_NAME, "scale_factor": 2.5,
                        "justification": 2, "extrusion": (1, 0, 0)})
        # ezdxf's export_entity() unconditionally adds HAS_VERTICES. Author the
        # applicable flag now, before freezing expectations, rather than
        # comparing a pre-export flag=0 snapshot to its emitted flag=1 body.
        for entity in (model_line, block_line):
            entity.set_flag_state(entity.HAS_VERTICES, True)
        # Set these redundant fields only after the typed independent producer
        # has computed vertices, miters and line/fill parameters. Saving does
        # not regenerate MLINE geometry; no source file is modified afterward.
        style.dxf.name = "RedundantBody"
        model_line.dxf.style_name = "Model_café_Ω"
        block_line.dxf.style_name = "Block_表_Ω"
        expected = {name: mline_style_values(value) for name, value in doc.mline_styles}
        consumers = {"ENTITIES:*Model_Space": mline_consumer_values(model_line),
                     "BLOCKS:PeerMlineBlock": mline_consumer_values(block_line)}

        def projection(module, path, binary, wanted, source_recipe=False):
            keys = [key for key in wanted if key != "Standard"]
            assert len(keys) == 1, "selected authority key census"
            result = selected_mline_style_projection(
                module, path, binary, wanted, source_recipe=source_recipe,
                consumer_expected=consumers, consumer_style_key=keys[0])
            # The authored collection uses soft-owner pointers. Preserve this
            # selected role; this is not a restriction on every valid dictionary.
            assert all(row["pointerRole"] == 350
                       for row in result["physicalBeforeDocumentSetup"]["styles"].values()), "selected dictionary pointer role"
            return result

        source = output / ("mline-style-key-authority-AC1027-" + fmt + ".dxf")
        doc.saveas(source, fmt=fmt)
        original = {"version": doc.dxfversion, "page": "utf-8", "format": fmt,
                    "path": source.name, "sha256": digest(source), "matched": False,
                    "selectedNullLayerBindings": null_bindings, "omittedDefaultMaterialHandles": omitted,
                    "dictionaryKey": PEER_MLINE_STYLE_NAME, "redundantBodyName": style.dxf.name,
                    "redundantConsumerNames": [model_line.dxf.style_name, block_line.dxf.style_name],
                    "authoring": "Typed MLINE geometry and applicable HAS_VERTICES flag authored first in two WCS planes; distinct redundant group-2 values with nonASCII consumer names assigned before first save; no post-generation file mutation",
                    "expectedConsumers": consumers, "nativeMlineConsumerQualified": False}
        try:
            original["observations"] = projection(ezdxf, source, fmt == "bin", expected, source_recipe=True)
            original["matched"] = True
        except Exception as error:
            original["mismatch"] = str(error)
        inputs.append(original)
        renamed = {key: value for key, value in expected.items() if key != PEER_MLINE_STYLE_NAME}
        renamed[PEER_MLINE_STYLE_RENAMED_KEY] = expected[PEER_MLINE_STYLE_NAME]
        unchanged, edited = selected_resource_outputs(
            ezdxf, probe, converter, source, "R2013", "utf-8", fmt, original,
            "--mline-style-key-probe", projection, expected, renamed,
            edit_prefix="key-edit-", edit_arguments=(PEER_MLINE_STYLE_RENAMED_KEY,))
        for row in unchanged + edited:
            row["redundantNamesAndGeometryPreserved"] = row["matched"]
        round_trips.extend(unchanged)
        edits.extend(edited)
        aci_expected = {key: dict(value) for key, value in expected.items()}
        elements = expected[PEER_MLINE_STYLE_NAME]["elements"]
        assert len(elements) == len(PEER_MLINE_STYLE_ELEMENT_ACI_EDIT), "selected ACI element census"
        aci_expected[PEER_MLINE_STYLE_NAME]["elements"] = [
            (offset, color, linetype)
            for (offset, _, linetype), color in zip(elements, PEER_MLINE_STYLE_ELEMENT_ACI_EDIT)]
        _, edited_aci = selected_resource_outputs(
            ezdxf, probe, converter, source, "R2013", "utf-8", fmt, original,
            "--mline-style-aci-probe", projection, expected, aci_expected,
            modes=(True,), edit_prefix="element-aci-edit-")
        for row in edited_aci:
            row["dictionaryAuthoritySourceReused"] = True
            row["onlyElementAciChanged"] = row["matched"]
        aci_edits.extend(edited_aci)
    return inputs, round_trips, edits, aci_edits


def selected_round_trip(ezdxf, source, output, metadata_updates=None, target_binary=False, legacy=False):
    before, after = ezdxf.readfile(source), ezdxf.readfile(output)
    expected, actual = before.rootdict["EZDXF_META"], after.rootdict["EZDXF_META"]
    expected_values = {k: v.dxf.value for k, v in expected.items()}
    actual_values = {k: v.dxf.value for k, v in actual.items()}
    if metadata_updates:
        from ezdxf.tools.text import caret_decode
        assert set(metadata_updates) <= set(expected_values), "missing metadata key"
        expected_values.update(metadata_updates)
        # The regular peer loader intentionally leaves DXF escapes encoded.
        # Unicode escapes apply to both formats; caret escapes are ASCII-only.
        actual_values = {k: ezdxf.decode_dxf_unicode(v)
                         for k, v in actual_values.items()}
        if not target_binary:
            actual_values = {k: caret_decode(v) for k, v in actual_values.items()}
    assert expected_values == actual_values, "metadata values: %r != %r" % (
        expected_values, actual_values)
    assert actual.dxf.owner == after.rootdict.dxf.handle, "metadata dictionary owner"
    assert all(v.dxftype() == "DICTIONARYVAR" and v.dxf.owner == actual.dxf.handle
               for _, v in actual.items()), "variable owners"
    if not legacy:
        a, b = before.materials["PeerSurface"], after.materials["PeerSurface"]
        for field in ("normal_map_method", "normal_map_strength", "normal_map_file_name",
                      "color_bleed_scale", "indirect_dump_scale", "reflectance_scale",
                      "transmittance_scale", "two_sided_material"):
            assert a.dxf.get(field) == b.dxf.get(field), field
        for field in ("diffuse_mapper_matrix", "bump_mapper_matrix",
                      "refraction_mapper_matrix", "normal_mapper_matrix"):
            assert list(getattr(a, field)) == list(getattr(b, field)), field
    lines = list(after.modelspace().query("LINE"))
    assert len(lines) == 1 and tuple(lines[0].dxf.start) == (1, 2, 3), "3D start"
    assert tuple(lines[0].dxf.end) == (4, 5, 6), "3D end"
    if not legacy:
        assert lines[0].dxf.material_handle == b.dxf.handle, "material reference"
    return {"metadataEntries": len(actual), "matrices": 0 if legacy else 4,
            "line3d": True, "materialLinkChecked": not legacy,
            "line3dAndMaterial": not legacy}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--converter", type=Path,
                        help="also convert each source to ASCII/Binary and verify selected values")
    parser.add_argument("--edit-variables", action="store_true",
                        help="also edit peer metadata with the C++ adapter and independently decode it")
    parser.add_argument("--legacy-variables", action="store_true",
                        help="also qualify R2000/R2004 variable text in three legacy codepages")
    parser.add_argument("--scale-probe", action="store_true",
                        help="also check authored generic SCALE tags with independent selected projections")
    parser.add_argument("--scale-only", action="store_true",
                        help="run only SCALE projections, actual CLI round trips and adapter edits")
    parser.add_argument("--mline-style-probe", action="store_true",
                        help="also check typed MLINESTYLE definitions and selected description edits")
    parser.add_argument("--mline-style-only", action="store_true",
                        help="run only MLINESTYLE projections, actual CLI round trips and description edits")
    args = parser.parse_args()
    selected_only = args.scale_only or args.mline_style_only
    if (args.scale_probe or args.scale_only or args.mline_style_probe or args.mline_style_only) and not args.converter:
        parser.error("selected resource qualification requires --converter for unchanged actual CLI output")
    if selected_only and (args.edit_variables or args.legacy_variables):
        parser.error("resource-only qualification cannot be combined with variable qualification flags")
    if (args.scale_only and (args.mline_style_only or args.mline_style_probe)
            or args.mline_style_only and args.scale_probe):
        parser.error("resource-only flags cannot select another resource lane")
    import ezdxf
    from ezdxf.math import Matrix44

    if ezdxf.__version__ != "1.4.5":
        parser.error("this source profile requires pinned ezdxf 1.4.5")
    probe = args.probe.resolve(strict=True)
    converter = args.converter.resolve(strict=True) if args.converter else None
    output = args.output_dir.resolve()
    if output.exists() and any(output.iterdir()):
        parser.error("output directory must be empty")
    output.mkdir(parents=True, exist_ok=True)
    rows = []
    round_trips = []
    variable_edits = []
    legacy_inputs, legacy_round_trips, legacy_edits = [], [], []
    source = None
    for version in (() if selected_only else ("R2007", "R2010", "R2013", "R2018")):
        doc = ezdxf.new(version)
        material = doc.materials.new("PeerSurface")
        material.dxf.update({
            "description": "independent local recipe",
            "bump_map_projection_method": 2, "bump_map_tiling_method": 3,
            "normal_map_method": 7, "normal_map_strength": 0.5,
            "normal_map_file_name": "peer-normal.png", "normal_map_source": 0,
            "normal_map_projection_method": 4, "normal_map_tiling_method": 2,
            "normal_map_auto_transform_method": 4, "color_bleed_scale": 0.6,
            "indirect_dump_scale": 0.7, "reflectance_scale": 0.8,
            "transmittance_scale": 0.9, "two_sided_material": 1,
        })
        material.diffuse_mapper_matrix = Matrix44([100.0 + i for i in range(16)])
        material.bump_mapper_matrix = Matrix44([900.0 + i for i in range(16)])
        material.refraction_mapper_matrix = Matrix44([1100.0 + i for i in range(16)])
        material.normal_mapper_matrix = Matrix44([200.0 + i for i in range(16)])
        # This peer emits its specular color method at reused code73. Select1
        # to isolate the normal-header gap without an invalid code73=0. This
        # harness does not qualify specular-color or complete peer semantics.
        for _, existing in doc.materials:
            existing.dxf.specular_color_method = 1
        doc.modelspace().add_line((1, 2, 3), (4, 5, 6), dxfattribs={
            "material_handle": material.dxf.handle,
        })
        source = Path(inspect.getfile(type(material)))
        for fmt in ("asc", "bin"):
            path = output / ("peer-" + doc.dxfversion + "-" + fmt + ".dxf")
            doc.saveas(path, fmt=fmt)
            run = subprocess.run([str(probe), "--material-source-order-probe", str(path)],
                                 capture_output=True, text=True)
            rows.append({"version": doc.dxfversion, "format": fmt,
                         "path": path.name, "sha256": digest(path),
                         "exitCode": run.returncode, "stdout": run.stdout,
                         "stderr": run.stderr})
            if converter:
                for target_format in ("asc", "bin"):
                    target = output / ("out-" + doc.dxfversion + "-" + fmt + "-" + target_format + ".dxf")
                    command = [str(converter), str(path), "-y", "-v" + version[1:]]
                    if target_format == "bin":
                        command.append("-b")
                    command.append(str(target))
                    converted = subprocess.run(command, capture_output=True, text=True)
                    row = {"version": doc.dxfversion, "input": fmt, "output": target_format,
                           "exitCode": converted.returncode, "stdout": converted.stdout,
                           "stderr": converted.stderr, "matched": False}
                    if converted.returncode == 0:
                        row["sha256"] = digest(target)
                        try:
                            row["observations"] = selected_round_trip(ezdxf, path, target)
                            row["matched"] = True
                        except Exception as error:
                            row["mismatch"] = str(error)
                    round_trips.append(row)
            if args.edit_variables:
                for target_format in ("asc", "bin"):
                    target = output / ("edit-" + doc.dxfversion + "-" + fmt + "-" + target_format + ".dxf")
                    command = [str(probe), "--dictionaryvar-edit-probe", str(path), str(target), target_format]
                    edited = subprocess.run(command, capture_output=True, text=True)
                    row = {"version": doc.dxfversion, "input": fmt, "output": target_format,
                           "exitCode": edited.returncode, "stdout": edited.stdout,
                           "stderr": edited.stderr, "matched": False}
                    if edited.returncode == 0:
                        row["sha256"] = digest(target)
                        try:
                            row["observations"] = selected_round_trip(
                                ezdxf, path, target, {"CREATED_BY_EZDXF": PEER_VARIABLE_EDIT},
                                target_binary=target_format == "bin")
                            row["matched"] = True
                        except Exception as error:
                            row["mismatch"] = str(error)
                    variable_edits.append(row)
    if args.legacy_variables:
        for version in ("R2000", "R2004"):
            for page, text in (("cp1252", "café"), ("cp1251", "Привет"), ("cp932", "表タ")):
                doc = ezdxf.new(version)
                doc.encoding = page
                # A variable/primitive legacy profile. Keep the separately
                # retained default-producer MATERIAL failure unqualified.
                # These three unused defaults are omitted before generation.
                for name, _ in list(doc.materials):
                    doc.materials.delete(name)
                doc.ezdxf_metadata()["CREATED_BY_EZDXF"] = text
                doc.modelspace().add_line((1, 2, 3), (4, 5, 6))
                for fmt in ("asc", "bin"):
                    path = output / ("legacy-" + doc.dxfversion + "-" + page + "-" + fmt + ".dxf")
                    doc.saveas(path, fmt=fmt)
                    legacy_inputs.append({"version": doc.dxfversion, "page": page,
                                          "format": fmt, "sha256": digest(path)})
                    for target_format in ("asc", "bin"):
                        for edit in (False, True):
                            if not edit and not converter:
                                continue
                            target = output / (("legacy-edit-" if edit else "legacy-out-")
                                               + doc.dxfversion + "-" + page + "-" + fmt + "-" + target_format + ".dxf")
                            if edit:
                                command = [str(probe), "--dictionaryvar-edit-probe", str(path),
                                           str(target), target_format, text]
                            else:
                                command = [str(converter), str(path), "-y", "-v" + version[1:]]
                                if target_format == "bin":
                                    command.append("-b")
                                command.append(str(target))
                            converted = subprocess.run(command, capture_output=True, text=True)
                            row = {"version": doc.dxfversion, "page": page, "input": fmt,
                                   "output": target_format, "exitCode": converted.returncode,
                                   "stdout": converted.stdout, "stderr": converted.stderr, "matched": False}
                            if converted.returncode == 0:
                                row["sha256"] = digest(target)
                                try:
                                    row["observations"] = selected_round_trip(
                                        ezdxf, path, target,
                                        {"CREATED_BY_EZDXF": PEER_VARIABLE_EDIT} if edit else None,
                                        target_binary=target_format == "bin", legacy=True)
                                    row["matched"] = True
                                except Exception as error:
                                    row["mismatch"] = str(error)
                            (legacy_edits if edit else legacy_round_trips).append(row)
    scale_inputs, scale_round_trips, scale_edits = [], [], []
    if args.scale_probe or args.scale_only:
        scale_inputs, scale_round_trips, scale_edits = qualify_scales(
            ezdxf, probe, converter, output)
    mline_inputs, mline_round_trips, mline_edits = [], [], []
    mline_boundary_inputs, mline_boundary_round_trips, mline_boundary_edits, mline_scalar_edits = [], [], [], []
    mline_text_inputs, mline_text_round_trips, mline_text_transcodes = [], [], []
    mline_key_inputs, mline_key_round_trips, mline_key_edits = [], [], []
    mline_element_aci_edits = []
    if args.mline_style_probe or args.mline_style_only:
        mline_inputs, mline_round_trips, mline_edits = qualify_mline_styles(
            ezdxf, probe, converter, output)
        (mline_boundary_inputs, mline_boundary_round_trips,
         mline_boundary_edits, mline_scalar_edits) = qualify_mline_style_boundaries(
            ezdxf, probe, converter, output)
        mline_text_inputs, mline_text_round_trips, mline_text_transcodes = qualify_mline_style_text_boundaries(
            ezdxf, probe, converter, output)
        (mline_key_inputs, mline_key_round_trips, mline_key_edits,
         mline_element_aci_edits) = qualify_mline_style_key_authority(
            ezdxf, probe, converter, output)
    passed = sum(row["exitCode"] == 0 for row in rows)
    report = {
        "schema": "libdxfrw.material-peer-source-order.v1",
        "scope": "Selected imported/converted body, metadata and geometry values; not full/native support",
        "python": sys.version, "producer": "ezdxf", "version": ezdxf.__version__,
        "packages": {name: importlib.metadata.version(name) for name in (
            "ezdxf", "numpy", "fonttools", "pyparsing", "typing_extensions")},
        "producerMaterialSourceSha256": digest(source) if source else None,
        "probeSha256": digest(probe), "generated": len(rows), "passed": passed,
        "results": rows, "wholeDocumentExportQualified": False,
        "roundTrips": round_trips,
        "legacyInputs": legacy_inputs,
        "legacyRecipe": "Primitive LINE and metadata; three unused producer default materials omitted before generation",
        "legacyDefaultMaterialProfileQualified": False,
        "legacyRoundTrips": legacy_round_trips,
        "legacyRoundTripsMatched": sum(row["matched"] for row in legacy_round_trips),
        "legacyEdits": legacy_edits,
        "legacyEditsMatched": sum(row["matched"] for row in legacy_edits),
        "variableEdits": variable_edits,
        "variableEditsMatched": sum(row["matched"] for row in variable_edits),
        "variableEditNormalization": {"allFormats": ["ezdxf.decode_dxf_unicode"],
                                      "asciiOnly": ["ezdxf.tools.text.caret_decode"]},
        "roundTripsMatched": sum(row["matched"] for row in round_trips),
        "scaleInputs": scale_inputs,
        "scaleInputsMatched": sum(row["matched"] for row in scale_inputs),
        "scaleRoundTrips": scale_round_trips,
        "scaleRoundTripsMatched": sum(row["matched"] for row in scale_round_trips),
        "scaleEdits": scale_edits,
        "scaleEditsMatched": sum(row["matched"] for row in scale_edits),
        "scaleRecipe": {
            "source": "Authored generic SCALE tags; ezdxf 1.4.5 document/tag writer",
            "producerTypedScaleImplementation": False,
            "projection": "Physical low-level tags/compiled fields, class, owner/key, null LAYER347 and LINE before document setup; ordinary DXFTagStorage read reported separately",
            "materialBindingRecipe": "Set LAYER material handles to null before omitting three unused defaults; retain original handles and disclose setup-created MATERIAL objects",
            "normalization": {"allFormats": ["ezdxf.decode_dxf_unicode once"],
                              "asciiOnly": ["ezdxf.tools.text.caret_decode once"]},
            "scope": "Selected field/reference transport; annotation semantics/native acceptance unqualified",
            "defaultMaterialProfileQualified": False,
            "modules": {name: digest(Path(ezdxf.__file__).parent / name) for name in (
                "entities/dxfentity.py", "entities/factory.py", "sections/classes.py", "sections/objects.py",
                "lldxf/tagger.py", "lldxf/validator.py", "lldxf/tags.py", "lldxf/extendedtags.py")},
        },
        "mlineStyleInputs": mline_inputs,
        "mlineStyleInputsMatched": sum(row["matched"] for row in mline_inputs),
        "mlineStyleRoundTrips": mline_round_trips,
        "mlineStyleRoundTripsMatched": sum(row["matched"] for row in mline_round_trips),
        "mlineStyleEdits": mline_edits,
        "mlineStyleEditsMatched": sum(row["matched"] for row in mline_edits),
        "mlineStyleBoundaryInputs": mline_boundary_inputs,
        "mlineStyleBoundaryInputsMatched": sum(row["matched"] for row in mline_boundary_inputs),
        "mlineStyleBoundaryRoundTrips": mline_boundary_round_trips,
        "mlineStyleBoundaryRoundTripsMatched": sum(row["matched"] for row in mline_boundary_round_trips),
        "mlineStyleBoundaryEdits": mline_boundary_edits,
        "mlineStyleBoundaryEditsMatched": sum(row["matched"] for row in mline_boundary_edits),
        "mlineStyleScalarEdits": mline_scalar_edits,
        "mlineStyleScalarEditsMatched": sum(row["matched"] for row in mline_scalar_edits),
        "mlineStyleTextInputs": mline_text_inputs,
        "mlineStyleTextInputsMatched": sum(row["matched"] for row in mline_text_inputs),
        "mlineStyleTextRoundTrips": mline_text_round_trips,
        "mlineStyleTextRoundTripsMatched": sum(row["matched"] for row in mline_text_round_trips),
        "mlineStyleTextCodepageTranscodes": mline_text_transcodes,
        "mlineStyleTextCodepageTranscodesMatched": sum(row["matched"] for row in mline_text_transcodes),
        "mlineStyleKeyInputs": mline_key_inputs,
        "mlineStyleKeyInputsMatched": sum(row["matched"] for row in mline_key_inputs),
        "mlineStyleKeyRoundTrips": mline_key_round_trips,
        "mlineStyleKeyRoundTripsMatched": sum(row["matched"] for row in mline_key_round_trips),
        "mlineStyleKeyEdits": mline_key_edits,
        "mlineStyleKeyEditsMatched": sum(row["matched"] for row in mline_key_edits),
        "mlineStyleElementAciEdits": mline_element_aci_edits,
        "mlineStyleElementAciEditsMatched": sum(row["matched"] for row in mline_element_aci_edits),
        "mlineStyleRecipe": {
            "source": "Authored typed ezdxf 1.4.5 MLINESTYLE definitions, Standard and PeerMline, plus LINE",
            "producerTypedMlineStyleImplementation": True,
            "editedStyle": PEER_MLINE_STYLE_NAME, "editedDescription": PEER_MLINE_STYLE_EDIT,
            "descriptionMaxCharacters": 255, "angleWireUnits": "degrees",
            "angleDefaultDegrees": 90,
            "additionalBoundaryCases": ["both-missing", "start-missing", "end-missing", "explicit-zero"],
            "originalDescriptionMatrixCountsUnchanged": {"inputs": 8, "roundTrips": 16, "edits": 16},
            "boundaryScope": "Four separately reported authored sources; eight CLI, eight description and eight scalar outputs; expected physical presence/absence checked before setup and across output, independent typed defaults checked separately",
            "scalarProbe": {"selector": "--mline-style-scalar-probe", "edits": PEER_MLINE_STYLE_SCALAR_EDIT,
                            "knownFlagBits": [1, 2, 16, 32, 64, 256, 512, 1024], "knownFlagMask": 1907,
                            "fillColorScope": "Selected ACI 6 within 1..255; not broad CMC or extra producer-value qualification"},
            "textBoundaryScope": {
                "sources": 2, "unchangedCliOutputs": 4, "legacyCodepageOutputs": 2,
                "sourceVersionsFormats": ["AC1015/cp1252/ASCII", "AC1021/UTF-8/Binary"],
                "codepageProbe": "--mline-style-text-probe input output asc|bin ANSI_1251",
                "semanticValues": "Valid typed nonASCII style and LTYPE names containing literal caret-A; legacy Omega uses producer CIF; no source-version/name/reference edits",
                "authoring": "Scoped generation-time caret encoding on owning style2/6, dictionary3 and LTYPE2; original files are never changed after generation",
                "projection": "Independent pre-setup dictionary/body joins, unique nonnull LTYPE TABLE control/record identities, exact single owner-to-control joins, defined LTYPE target names, ordered tuples and LINE; typed ordinary read separately",
                "literalBackslashSymbolNamesQualified": False, "nativeSymbolCollationQualified": False,
            },
            "keyAuthorityScope": {
                "sources": 2, "unchangedCliOutputs": 4, "dictionaryKeyEditOutputs": 4,
                "sourceVersionsFormats": ["AC1027/UTF-8/ASCII", "AC1027/UTF-8/Binary"],
                "keyProbe": "--mline-style-key-probe input output asc|bin RenamedPeer",
                "authoring": "Typed valid modelspace and named-block MLINE geometry in distinct WCS planes, then distinct redundant body/entity names before first save; unchanged 340 points to dictionary-selected style",
                "projection": "Independent physical dictionary/body/340/self/BLOCK_RECORD control/owner/frame joins, every MLINE scalar/vector/line/fill tuple; typed ordinary read with authoritative handle identity separately; no geometry regeneration or explicit audit/recovery",
                "rename": "Dictionary key only; redundant group-2 values and geometry remain their original values",
                "nativeMlineRenderingQualified": False, "wholeDocumentClosureQualified": False,
            },
            "elementAciScope": {
                "reusedSourceLane": "mlineStyleKeyInputs", "reusedSources": 2,
                "additionalSourcesGenerated": 0, "additionalUnchangedCliRuns": 0,
                "sourceVersionsFormats": ["AC1027/UTF-8/ASCII", "AC1027/UTF-8/Binary"],
                "editOutputs": 4, "probe": "--mline-style-aci-probe input output asc|bin",
                "editedDictionaryKey": PEER_MLINE_STYLE_NAME,
                "editedElementColors": PEER_MLINE_STYLE_ELEMENT_ACI_EDIT,
                "projection": "Reuse frozen pre-setup physical and ordinary typed style/consumer projections; change only the two ordered element ACI values, preserving all other style fields, offsets, LTYPE names, dictionary/body/consumer names, owners, 340 targets and complete MLINE geometry",
                "noSourcePostMutationOrGeometryRegeneration": True,
                "nativeByBlockByLayerSemanticsQualified": False,
                "otherVersionOrCmcProfilesQualified": False,
            },
            "projection": "All selected style definitions, dictionary/owner/name joins, base fields and ordered 49/62/6 tuples before setup; typed ordinary read separately",
            "primaryReference": "https://help.autodesk.com/cloudhelp/2015/ENU/AutoCAD-DXF/files/GUID-3EC12E5B-F5F6-484D-880F-D69EBE186D79.htm",
            "currentPrimaryReference": "https://help.autodesk.com/cloudhelp/2025/ENU/AutoCAD-DXF/files/GUID-3EC12E5B-F5F6-484D-880F-D69EBE186D79.htm",
            "colorReference": "https://help.autodesk.com/cloudhelp/2023/ENU/AutoCAD-DXF/files/GUID-3610039E-27D1-4E23-B6D3-7E60B22BB5BD.htm",
            "aciReference": "https://help.autodesk.com/cloudhelp/2018/ENU/OARX-RefGuide/files/OREF-AcCmColor__setColorIndex_Adesk__UInt16.html",
            "materialBindingRecipe": "Shared authored null LAYER bindings before omitting unused defaults; setup-created MATERIALs disclosed",
            "normalization": {"allFormats": ["ezdxf.decode_dxf_unicode once"],
                              "asciiOnly": ["ezdxf.tools.text.caret_decode once"]},
            "nativeMlineConsumerQualified": False, "defaultMaterialProfileQualified": False,
            "modules": {name: digest(Path(ezdxf.__file__).parent / name) for name in (
                "entities/mline.py", "entities/objectcollection.py", "entities/dxfentity.py",
                "entities/factory.py", "sections/objects.py", "lldxf/tagger.py",
                "entities/dictionary.py", "entities/ltype.py", "entities/dxfns.py", "sections/table.py",
                "graphicsfactory.py", "layouts/blocklayout.py", "sections/blocks.py",
                "lldxf/encoding.py", "lldxf/tagwriter.py", "lldxf/validator.py", "lldxf/tags.py", "lldxf/extendedtags.py")},
        },
        "qualificationScriptSha256": digest(Path(__file__)),
        "converterSha256": digest(converter) if converter else None,
        "nativeRenderingQualified": False, "fullObjectiveComplete": False,
    }
    (output / "qualification.json").write_text(json.dumps(report, indent=2) + "\n")
    if not selected_only:
        print("Independent MATERIAL selected body matches: %d/%d" % (passed, len(rows)))
    if converter and not selected_only:
        print("Independent selected document round trips: %d/%d" % (
            report["roundTripsMatched"], len(round_trips)))
    if args.edit_variables:
        print("Independent edited metadata round trips: %d/%d" % (
            report["variableEditsMatched"], len(variable_edits)))
    if args.legacy_variables:
        print("Independent legacy round trips/edits: %d/%d, %d/%d" % (
            report["legacyRoundTripsMatched"], len(legacy_round_trips),
            report["legacyEditsMatched"], len(legacy_edits)))
    if args.scale_probe or args.scale_only:
        print("Independent SCALE source/unchanged/edit projections: %d/%d, %d/%d, %d/%d" % (
            report["scaleInputsMatched"], len(scale_inputs),
            report["scaleRoundTripsMatched"], len(scale_round_trips),
            report["scaleEditsMatched"], len(scale_edits)))
    if args.mline_style_probe or args.mline_style_only:
        print("Independent MLINESTYLE source/unchanged/edit projections: %d/%d, %d/%d, %d/%d" % (
            report["mlineStyleInputsMatched"], len(mline_inputs),
            report["mlineStyleRoundTripsMatched"], len(mline_round_trips),
            report["mlineStyleEditsMatched"], len(mline_edits)))
        print("Independent MLINESTYLE boundary source/CLI/description/scalar projections: %d/%d, %d/%d, %d/%d, %d/%d" % (
            report["mlineStyleBoundaryInputsMatched"], len(mline_boundary_inputs),
            report["mlineStyleBoundaryRoundTripsMatched"], len(mline_boundary_round_trips),
            report["mlineStyleBoundaryEditsMatched"], len(mline_boundary_edits),
            report["mlineStyleScalarEditsMatched"], len(mline_scalar_edits)))
        print("Independent MLINESTYLE text source/CLI/codepage projections: %d/%d, %d/%d, %d/%d" % (
            report["mlineStyleTextInputsMatched"], len(mline_text_inputs),
            report["mlineStyleTextRoundTripsMatched"], len(mline_text_round_trips),
            report["mlineStyleTextCodepageTranscodesMatched"], len(mline_text_transcodes)))
        print("Independent MLINESTYLE key source/CLI/rename projections: %d/%d, %d/%d, %d/%d" % (
            report["mlineStyleKeyInputsMatched"], len(mline_key_inputs),
            report["mlineStyleKeyRoundTripsMatched"], len(mline_key_round_trips),
            report["mlineStyleKeyEditsMatched"], len(mline_key_edits)))
        print("Independent MLINESTYLE element ACI edits on reused authority sources: %d/%d" % (
            report["mlineStyleElementAciEditsMatched"], len(mline_element_aci_edits)))
    return 0 if passed == len(rows) and all(
        row["matched"] for row in round_trips + variable_edits + legacy_round_trips + legacy_edits
        + scale_inputs + scale_round_trips + scale_edits
        + mline_inputs + mline_round_trips + mline_edits
        + mline_boundary_inputs + mline_boundary_round_trips + mline_boundary_edits
        + mline_scalar_edits + mline_text_inputs + mline_text_round_trips
        + mline_text_transcodes + mline_key_inputs + mline_key_round_trips
        + mline_key_edits + mline_element_aci_edits) else 1


if __name__ == "__main__":
    sys.exit(main())
