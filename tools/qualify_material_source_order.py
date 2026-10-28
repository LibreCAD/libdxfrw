#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-or-later
"""Generate independent MATERIAL controls with ezdxf 1.4.5 and run body probes.

Run with an isolated Python containing ezdxf==1.4.5. The probe is the built
libdxfrw_dxf_fixture_tests executable. An optional converter also checks selected
metadata, material and geometry round trips. Native rendering and arbitrary
source ordering remain unqualified.
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
    args = parser.parse_args()
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
    for version in ("R2007", "R2010", "R2013", "R2018"):
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
    passed = sum(row["exitCode"] == 0 for row in rows)
    report = {
        "schema": "libdxfrw.material-peer-source-order.v1",
        "scope": "Selected imported/converted body, metadata and geometry values; not full/native support",
        "python": sys.version, "producer": "ezdxf", "version": ezdxf.__version__,
        "packages": {name: importlib.metadata.version(name) for name in (
            "ezdxf", "numpy", "fonttools", "pyparsing", "typing_extensions")},
        "producerMaterialSourceSha256": digest(source),
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
        "nativeRenderingQualified": False, "fullObjectiveComplete": False,
    }
    (output / "qualification.json").write_text(json.dumps(report, indent=2) + "\n")
    print("Independent MATERIAL selected body matches: %d/%d" % (passed, len(rows)))
    if converter:
        print("Independent selected document round trips: %d/%d" % (
            report["roundTripsMatched"], len(round_trips)))
    if args.edit_variables:
        print("Independent edited metadata round trips: %d/%d" % (
            report["variableEditsMatched"], len(variable_edits)))
    if args.legacy_variables:
        print("Independent legacy round trips/edits: %d/%d, %d/%d" % (
            report["legacyRoundTripsMatched"], len(legacy_round_trips),
            report["legacyEditsMatched"], len(legacy_edits)))
    return 0 if passed == len(rows) and all(
        row["matched"] for row in round_trips + variable_edits + legacy_round_trips + legacy_edits) else 1


if __name__ == "__main__":
    sys.exit(main())
