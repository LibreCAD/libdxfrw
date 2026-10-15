#!/usr/bin/env python3
"""Optional generated-control ODA qualification; requires existing ezdxf 1.4.4.

All drawings and reports are created in a new/empty output directory. This is
an interoperability experiment, separate from the dependency-free fast tests.
An ODA zero exit status alone does not count as retained modeler content.
"""

import argparse
import hashlib
import json
import platform
import plistlib
import subprocess
from datetime import datetime, timezone
from pathlib import Path


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command):
    try:
        result = subprocess.run(command, capture_output=True, text=True, timeout=60)
        return {"command": command, "status": result.returncode,
                "stdout": result.stdout, "stderr": result.stderr}
    except subprocess.TimeoutExpired:
        return {"command": command, "status": "timeout"}


def inspect(path):
    import ezdxf
    from ezdxf.acis import api
    from ezdxf.lldxf.tagger import ascii_tags_loader

    if not path.is_file():
        return {"exists": False}
    result = {"exists": True, "sha256": digest(path)}
    try:
        doc = ezdxf.readfile(path)
        result["entities"] = []
        for entity in doc.modelspace().query("3DSOLID REGION BODY"):
            row = {"type": entity.dxftype(), "sat_lines": len(entity.sat),
                   "sab_bytes": len(entity.sab)}
            payload = entity.sab if entity.has_binary_data else "\n".join(entity.sat).encode("ascii")
            row["carrier_sha256"] = hashlib.sha256(payload).hexdigest()
            try:
                bodies = api.load_dxf(entity)
                meshes = [mesh for body in bodies for mesh in api.mesh_from_body(body)]
                vertices = [tuple(v) for mesh in meshes for v in mesh.vertices]
                row["bodies"] = len(bodies)
                row["faces"] = sum(len(mesh.faces) for mesh in meshes)
                if vertices:
                    row["bounds"] = [[min(v[i] for v in vertices) for i in range(3)],
                                     [max(v[i] for v in vertices) for i in range(3)]]
            except Exception as error:
                row["acis_error"] = str(error)
            result["entities"].append(row)
        if not path.read_bytes().startswith(b"AutoCAD Binary DXF"):
            with path.open(encoding="cp1252") as stream:
                tags = list(ascii_tags_loader(stream))
            result["modeler_fields"] = []
            active = None
            for tag in tags:
                if tag.code == 0:
                    if active is not None:
                        result["modeler_fields"].append(active)
                    active = [] if tag.value in ("3DSOLID", "REGION", "BODY") else None
                if active is not None and tag.code in (0, 100, 70, 350, 290, 2):
                    active.append([tag.code, tag.value])
    except Exception as error:
        result["read_error"] = str(error)
    return result


def retained(observation, kind, expected):
    entities = observation.get("entities", [])
    return (len(entities) == 1 and entities[0].get("type") == kind
            and entities[0].get("bodies") == 1
            and entities[0].get("faces") == expected["faces"]
            and entities[0].get("bounds") == expected["bounds"])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dwg2dxf", type=Path, required=True)
    parser.add_argument("--oda", type=Path, required=True)
    parser.add_argument("--oda-version", help="exact installed version; macOS defaults to bundle metadata")
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--versions", type=int, nargs="+", choices=(2000, 2004, 2007, 2010, 2013),
                        default=(2000, 2004, 2007, 2010, 2013))
    args = parser.parse_args()
    try:
        import ezdxf
        from ezdxf.acis import api, entities, mesh as acis_mesh
        from ezdxf.lldxf.tagwriter import TagWriter
        from ezdxf.render import MeshBuilder
        from ezdxf.render.forms import cube
    except ImportError:
        parser.error("use an existing Python environment with ezdxf exactly 1.4.4")
    if ezdxf.__version__ != "1.4.4":
        parser.error("ezdxf exactly 1.4.4 is required")
    cli, oda, root = args.dwg2dxf.resolve(), args.oda.resolve(), args.output_dir.resolve()
    if not cli.is_file() or not oda.is_file():
        parser.error("both executable paths must already exist")
    oda_version = args.oda_version
    version_source = "argument"
    bundle_metadata = oda.parent.parent / "Info.plist"
    if oda_version is None and bundle_metadata.is_file():
        with bundle_metadata.open("rb") as stream:
            oda_version = plistlib.load(stream).get("CFBundleShortVersionString")
        version_source = str(bundle_metadata)
    if not oda_version:
        parser.error("supply the exact installed ODA version with --oda-version")
    if root.exists() and (not root.is_dir() or any(root.iterdir())):
        parser.error("output directory must be new or empty")
    root.mkdir(parents=True, exist_ok=True)
    report = {
        "scope": "generated modeler DXF syntax/retention; no native producer, general ACIS or unit qualification",
        "created_utc": datetime.now(timezone.utc).isoformat(),
        "platform": {"system": platform.system(), "release": platform.release(), "machine": platform.machine()},
        "generator_sha256": digest(Path(__file__)),
        "ezdxf_version": ezdxf.__version__,
        "ezdxf_sources": {str(Path(module.__file__).resolve()): digest(Path(module.__file__))
                           for module in (ezdxf, api, entities, acis_mesh)},
        "cli": {"path": str(cli), "sha256": digest(cli)},
        "oda": {"path": str(oda), "sha256": digest(oda),
                "version": oda_version, "version_source": version_source},
        "versions": [],
    }
    all_retained = True
    for year in args.versions:
        directory = root / str(year)
        inputs, dwg, output = directory / "inputs", directory / "oda-dwg", directory / "oda-dxf"
        for path in (inputs, dwg, output):
            path.mkdir(parents=True)
        version = {"year": year, "files": {}, "processes": []}
        for kind, factory in (("3DSOLID", "add_3dsolid"), ("REGION", "add_region"), ("BODY", "add_body")):
            mesh = cube()
            expected = {"faces": 6, "bounds": [[-0.5] * 3, [0.5] * 3]}
            if kind == "REGION":
                mesh = MeshBuilder()
                mesh.add_face([(0, 0, 0), (2, 0, 0), (2, 3, 0), (0, 3, 0)])
                expected = {"faces": 1, "bounds": [[0.0, 0.0, 0.0], [2.0, 3.0, 0.0]]}
            doc = ezdxf.new("R" + str(year))
            doc.units = 0
            entity = getattr(doc.modelspace(), factory)()
            api.export_dxf(entity, [api.body_from_mesh(mesh)])
            for binary in (False, True):
                seed = inputs / ("seed_" + kind + ("_binary" if binary else "_ascii") + ".dxf")
                doc.saveas(seed, fmt="bin" if binary else "asc")
                version["files"][seed.name] = {"role": "oracle-calibration", "kind": kind,
                                              "expected": expected, "input": inspect(seed)}
            # Isolate the codec/writer from unrelated extension-object graphs.
            # The full source above is retained separately as the ODA baseline.
            minimal = directory / ("minimal_" + kind + ".dxf")
            entity.dxf.owner = "0"
            with minimal.open("w", encoding="ascii") as stream:
                writer = TagWriter(stream, dxfversion=doc.dxfversion)
                for code, value in ((0, "SECTION"), (2, "HEADER"), (9, "$ACADVER"),
                                    (1, doc.dxfversion), (9, "$INSUNITS"), (70, 0),
                                    (0, "ENDSEC"), (0, "SECTION"), (2, "ENTITIES")):
                    writer.write_tag2(code, value)
                entity.export_dxf(writer)
                writer.write_tag2(0, "ENDSEC")
                if year >= 2013:
                    doc.acdsdata.export_dxf(writer)
                writer.write_tag2(0, "EOF")
            for binary in (False, True):
                target = inputs / ("libdxfrw_" + kind + ("_binary" if binary else "_ascii") + ".dxf")
                command = [str(cli), str(minimal), "-o", str(target)] + (["-b"] if binary else [])
                version["files"][target.name] = {
                    "role": "subject", "kind": kind, "expected": expected,
                    "source_sha256": digest(minimal), "conversion": run(command), "input": inspect(target),
                }
        for source, target, filetype, pattern in ((inputs, dwg, "DWG", "*.dxf"),
                                                 (dwg, output, "DXF", "*.dwg")):
            print("ODA", year, filetype, flush=True)
            version["processes"].append(run([str(oda), str(source), str(target),
                                             "ACAD" + str(year), filetype, "0", "1", pattern]))
        for name, row in version["files"].items():
            row["oda_dwg_exists"] = (dwg / (Path(name).stem + ".dwg")).is_file()
            row["oda_readback"] = inspect(output / name)
            row["retained"] = (all(process["status"] == 0 for process in version["processes"])
                               and row["oda_dwg_exists"]
                               and retained(row["oda_readback"], row["kind"], row["expected"]))
            if row["role"] == "subject":
                row["retained"] = row["retained"] and row["conversion"]["status"] == 0
                all_retained = all_retained and row["retained"]
            print(year, name, "retained=" + str(row["retained"]), flush=True)
        report["versions"].append(version)
        (root / "report.json").write_text(json.dumps(report, indent=2, allow_nan=False) + "\n")
    print("Report:", root / "report.json", flush=True)
    return 0 if all_retained else 1


if __name__ == "__main__":
    raise SystemExit(main())
