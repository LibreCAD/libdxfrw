#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Generate independent MATERIAL controls with ezdxf 1.4.5 and run body probes.

Run with an isolated Python containing ezdxf==1.4.5. The probe is the built
libdxfrw_dxf_fixture_tests executable. An optional converter also checks selected
metadata, material and geometry round trips. Native rendering and arbitrary
source ordering remain unqualified.
Optional SCALE checks use authored generic tags and independent projections;
ezdxf has no typed annotation-scale implementation for this profile.
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


def physical_scale_projection(ezdxf, path, binary, expected, source_recipe=False):
    """Inspect the actual file through low-level tags before document setup."""
    from ezdxf.filemanagement import dxf_file_info
    from ezdxf.lldxf.extendedtags import ExtendedTags
    from ezdxf.lldxf.tagger import ascii_tags_loader, binary_tags_loader, tag_compiler
    from ezdxf.lldxf.tags import group_tags
    from ezdxf.tools.text import caret_decode

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
    objects = [record for scope, record in records if scope == "OBJECTS"]
    dictionaries = [ExtendedTags(record) for record in objects
                    if record[0].value == "DICTIONARY"]

    def entries(xtags):
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

    roots = [d for d in dictionaries
             if int(d.subclasses[0].get_first_value(330, "0"), 16) == 0]
    assert len(roots) == 1, "physical root dictionary census"
    root = roots[0]
    root_handle = root.subclasses[0].get_first_value(5)
    dictionary_handle = entries(root)["ACAD_SCALELIST"]["handle"]
    selected = [d for d in dictionaries
                if d.subclasses[0].get_first_value(5) == dictionary_handle]
    assert len(selected) == 1, "physical SCALE dictionary target"
    dictionary = selected[0]
    assert dictionary.subclasses[0].get_first_value(330) == root_handle, "physical dictionary owner"
    targets = entries(dictionary)
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
    classes = [r for scope, r in records if scope == "CLASSES" and r[0].value == "CLASS"
               and r.get_first_value(1) == "SCALE"]
    assert len(classes) == 1 and classes[0].get_first_value(2) == "AcDbScale", "physical SCALE class"
    materials = [{"handle": r.get_first_value(5), "name": r.get_first_value(1)}
                 for r in objects if r[0].value == "MATERIAL"]
    if source_recipe:
        assert not materials, "selected source unexpectedly contains MATERIAL bodies"
    return {"fields": fields, "key": PEER_SCALE_KEY, "scaleHandle": scale_handle,
            "dictionaryHandle": dictionary_handle, "rootHandle": root_handle,
            "dictionaryPointerRole": targets[PEER_SCALE_KEY]["role"],
            "layerMaterialBindings": layers, "materialObjects": materials,
            "scaleCount": len(scales), "lineCount": len(lines),
            "beforeDocumentSetup": True, "beforeExplicitAuditOrRecovery": True}


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
        null_layer_bindings = []
        for layer in doc.layers:
            null_layer_bindings.append({"name": layer.dxf.name,
                                        "originalMaterialHandle": layer.dxf.get("material_handle"),
                                        "selectedMaterialHandle": "0"})
            layer.dxf.material_handle = "0"
        omitted_material_handles = [{"name": key, "handle": obj.dxf.handle}
                                    for key, obj in doc.materials]
        omitted_materials = [row["name"] for row in omitted_material_handles]
        for key in omitted_materials:
            doc.materials.delete(key)
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
            for target_format in ("asc", "bin"):
                for edit in (False, True):
                    target = output / (("edit-" if edit else "out-")
                                       + stem + "-" + target_format + ".dxf")
                    if edit:
                        command = [str(probe), "--scale-edit-probe", str(source),
                                   str(target), target_format]
                    else:
                        command = [str(converter), str(source), "-y", "-v" + version[1:]]
                        if target_format == "bin":
                            command.append("-b")
                        command.append(str(target))
                    row = {"version": doc.dxfversion, "page": page, "input": fmt,
                           "output": target_format, "sourceSha256": original["sha256"],
                           "path": target.name, "command": command, "matched": False}
                    try:
                        run = subprocess.run(command, capture_output=True, text=True, timeout=60)
                        row.update({"exitCode": run.returncode, "stdout": run.stdout,
                                    "stderr": run.stderr})
                        if run.returncode == 0:
                            row["sha256"] = digest(target)
                            row["observations"] = selected_scale_projection(
                                ezdxf, target, target_format == "bin",
                                PEER_SCALE_EDIT if edit else expected)
                            row["matched"] = original["matched"]
                    except subprocess.TimeoutExpired:
                        row.update({"exitCode": 124, "mismatch": "SCALE process timeout (60s)"})
                    except Exception as error:
                        row["mismatch"] = str(error)
                    (edits if edit else round_trips).append(row)
    return inputs, round_trips, edits


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
    args = parser.parse_args()
    if (args.scale_probe or args.scale_only) and not args.converter:
        parser.error("SCALE qualification requires --converter for unchanged actual CLI output")
    if args.scale_only and (args.edit_variables or args.legacy_variables):
        parser.error("--scale-only cannot be combined with variable qualification flags")
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
    for version in (() if args.scale_only else ("R2007", "R2010", "R2013", "R2018")):
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
        "qualificationScriptSha256": digest(Path(__file__)),
        "converterSha256": digest(converter) if converter else None,
        "nativeRenderingQualified": False, "fullObjectiveComplete": False,
    }
    (output / "qualification.json").write_text(json.dumps(report, indent=2) + "\n")
    if not args.scale_only:
        print("Independent MATERIAL selected body matches: %d/%d" % (passed, len(rows)))
    if converter and not args.scale_only:
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
    return 0 if passed == len(rows) and all(
        row["matched"] for row in round_trips + variable_edits + legacy_round_trips + legacy_edits
        + scale_inputs + scale_round_trips + scale_edits) else 1


if __name__ == "__main__":
    sys.exit(main())
