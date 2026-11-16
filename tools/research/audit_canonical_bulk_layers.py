#!/usr/bin/env python3
"""Audit locally generated canonical DWGs and their ODA-produced DXFs.

Run libdxfrw_dwg_default_material_optin_tests --keep, convert the six
canonical-bulk-{15,16,17}-{legacy,extended}.dwg files with ODA to ACAD2013
DXF, then invoke this tool with the input and output directories. Requires
LibreDWG's dwgread and an independently installed ezdxf.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess

import ezdxf


def require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def target(handle: object, role: int, message: str) -> int:
    require(isinstance(handle, list) and len(handle) >= 3 and handle[0] == role,
            message)
    return int(handle[-1])


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--dwg-dir", type=Path, required=True)
    parser.add_argument("--dxf-dir", type=Path, required=True)
    parser.add_argument("--dwgread", required=True)
    args = parser.parse_args()
    results = []
    for version, code in (("AC1021", "15"), ("AC1024", "16"),
                          ("AC1027", "17")):
        for mode in ("legacy", "extended"):
            stem = f"canonical-bulk-{code}-{mode}"
            dwg = args.dwg_dir / (stem + ".dwg")
            dxf = args.dxf_dir / (stem + ".dxf")
            require(dwg.is_file() and dxf.is_file(), f"missing {stem} drawing")
            require(not Path(str(dxf) + ".err").exists(), f"ODA error for {stem}")
            before = sha256(dwg)
            data = json.loads(subprocess.run(
                [args.dwgread, "-O", "minJSON", str(dwg)],
                stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True).stdout)
            objects = data["OBJECTS"]
            layers = [obj for obj in objects if obj.get("object") == "LAYER"]
            roots = [obj for obj in objects if obj.get("object") == "DICTIONARY"
                     and obj.get("handle", [None])[-1] == 12]
            plots = [obj for obj in objects if obj.get("object") == "DICTIONARYWDFLT"
                     and "Normal" in obj.get("items", {})]
            materials = [obj for obj in objects if obj.get("object") == "MATERIAL"
                         and obj.get("name") == "Global"]
            ltypes = [obj for obj in objects if obj.get("object") == "LTYPE"
                      and obj.get("name") == "CONTINUOUS"]
            require(len(roots) == len(plots) == len(materials) == len(ltypes) == 1,
                    f"DWG default graph count for {stem}")
            plot_dict = int(plots[0]["handle"][-1])
            plot_target = target(plots[0]["defaultid"], 5, stem)
            global_target = int(materials[0]["handle"][-1])
            ltype_target = int(ltypes[0]["handle"][-1])
            material_dict = target(roots[0]["items"]["ACAD_MATERIAL"], 2, stem)
            material_owners = [obj for obj in objects if obj.get("object") == "DICTIONARY"
                               and obj.get("handle", [None])[-1] == material_dict]
            require(len(material_owners) == 1
                    and target(material_owners[0]["ownerhandle"], 4, stem) == 12
                    and target(material_owners[0]["items"]["Global"], 2, stem) == global_target
                    and target(materials[0]["ownerhandle"], 4, stem) == material_dict,
                    f"DWG material graph for {stem}")
            require(target(roots[0]["items"]["ACAD_PLOTSTYLENAME"], 2, stem) == plot_dict
                    and target(plots[0]["ownerhandle"], 4, stem) == 12
                    and target(plots[0]["items"]["Normal"], 2, stem) == plot_target,
                    f"DWG plotstyle graph for {stem}")
            placeholders = [obj for obj in objects if obj.get("object") == "PLACEHOLDER"
                            and obj.get("handle", [None])[-1] == plot_target]
            require(len(placeholders) == 1
                    and target(placeholders[0]["ownerhandle"], 4, stem) == plot_dict,
                    f"DWG placeholder owner for {stem}")
            expected = {"0", "Defpoints", "CUSTOM-3D", "Q" * 31}
            expected.update(f"P6_LAYER_{i}" for i in range(130))
            if mode == "extended":
                expected.update({"R" * 255, "Named 3D.Layer"})
            require(len(layers) == len(expected)
                    and {obj["name"] for obj in layers} == expected,
                    f"DWG names differ for {stem}")
            doc = ezdxf.readfile(dxf)
            require(doc.header.get("$EXTNAMES") == (mode == "extended"),
                    f"EXTNAMES differs for {stem}")
            require(len(doc.layers) == len(expected)
                    and {layer.dxf.name for layer in doc.layers} == expected,
                    f"DXF names differ for {stem}")
            for obj in layers:
                require(obj.get("is_xdic_missing") == 1, f"xdict flag for {stem}")
                require(target(obj.get("xref"), 5, stem) == 0, f"xref for {stem}")
                require(target(obj.get("plotstyle"), 5, stem) == plot_target, f"plot for {stem}")
                require(target(obj.get("material"), 5, stem) == global_target, f"material for {stem}")
                require(target(obj.get("ltype"), 5, stem) == ltype_target, f"linetype for {stem}")
                if version == "AC1027":
                    require(target(obj.get("visualstyle"), 5, stem) == 0,
                            f"trailing null for {stem}")
                layer = doc.layers.get(obj["name"])
                require(layer.dxf.get("linetype") == "CONTINUOUS", f"DXF linetype for {stem}")
                material = doc.entitydb.get(layer.dxf.get("material_handle"))
                plot = doc.entitydb.get(layer.dxf.get("plotstyle_handle"))
                require(material is not None and material.dxftype() == "MATERIAL"
                        and material.dxf.get("name") == "Global", f"DXF material for {stem}")
                material_owner = doc.rootdict.get("ACAD_MATERIAL")
                require(material_owner is not None and material_owner.get("Global") == material
                        and material.dxf.get("owner") == material_owner.dxf.handle,
                        f"DXF material graph for {stem}")
                require(plot is not None and plot.dxftype() == "ACDBPLACEHOLDER",
                        f"DXF plotstyle for {stem}")
                owner = doc.entitydb.get(plot.dxf.get("owner"))
                require(owner is not None and owner.dxftype() == "ACDBDICTIONARYWDFLT"
                        and owner.dxf.get("default") == plot.dxf.handle
                        and owner.get("Normal") == plot
                        and doc.rootdict.get("ACAD_PLOTSTYLENAME") == owner,
                        f"DXF plotstyle graph for {stem}")
            lines = list(doc.modelspace().query("LINE"))
            require(len(lines) == 1 and tuple(lines[0].dxf.start) == (1.0, 2.0, 3.0)
                    and tuple(lines[0].dxf.end) == (4.0, 5.0, 6.0),
                    f"LINE geometry differs for {stem}")
            require(sha256(dwg) == before, f"source changed while auditing {stem}")
            results.append({"version": version, "mode": mode, "layers": len(layers),
                            "dwgSha256": before, "dxfSha256": sha256(dxf)})
    print(json.dumps({"scope": "generated canonical-layer profiles only",
                      "ezdxfVersion": ezdxf.__version__, "results": results}, indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
