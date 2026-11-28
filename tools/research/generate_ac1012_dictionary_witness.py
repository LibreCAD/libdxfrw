#!/usr/bin/env python3
"""Create scratch R13/R14/R2000 dictionary boundary controls and audit them.

Requires GNU LibreDWG 0.14. Optional peer mode needs ezdxf; ODA mode additionally
needs an ODA File Converter that can access its desktop session. Outputs are deliberately
kept in a new unique directory; no fixture is copied into the repository.
This is producer/transport evidence, not native duplicate-merge semantics.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import plistlib
import shutil
import subprocess
import tempfile

from audit_dxf_dictionary_cloning import audit_bytes
from audit_ac1024_sortents_boundary import crc16
from legacy_dwg_dictionary_wire import Bits, VERSIONS, decode_dictionary_frame, locate_object_frame

SENTINEL = b"destination must survive failed conversion\n"


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def run(args: list[str], *, allow_failure: bool = False) -> subprocess.CompletedProcess:
    result = subprocess.run(args, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=60, check=False)
    if result.returncode and not allow_failure:
        raise RuntimeError(f"{args[0]} exited {result.returncode}: {result.stderr.decode(errors='replace')[-1000:]}")
    return result


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def executable_identity(name: str) -> dict:
    resolved = shutil.which(name)
    require(resolved is not None, f"executable not found: {name}")
    path = Path(resolved).resolve()
    return {"path": str(path), "sha256": sha(path.read_bytes())}


def observe_dictionary(frame: bytes, version: str, expected_self: int) -> dict:
    """Walk the observed empty DICTIONARY using ODA v5.4.1 sections 20.1/20.4.44."""
    row = decode_dictionary_frame(frame, version, expected_self, expected_entries={})
    return {"dataBits": row["dataEndBit"], "cloning": row["cloning"], "hardOwner": row["hardOwner"],
            "owner": row["owner"], "reactors": row["reactors"], "xdict": row["xdict"],
            "xdictBitOffset": row["xdictBitOffset"],
            "eed": [{"appid": e["appid"], "rawHex": e["hex"]} for e in row["eed"]],
            "paddingBits": row["paddingBits"], "frameSha256": row["frameSha256"], "frameHex": frame.hex()}


def model_and_frame(dwgread: str, path: Path, expected_version: str) -> dict:
    data = path.read_bytes()
    require(data.startswith(VERSIONS[expected_version]), "DWG version mismatch")
    decoded = path.with_suffix(".json")
    run([dwgread, "-O", "JSON", "-o", str(decoded), str(path)])
    objects = json.loads(decoded.read_text())["OBJECTS"]
    nods = [o for o in objects if o.get("object") == "DICTIONARY" and "TEST_DICT" in o.get("items", {})]
    require(len(nods) == 1, "TEST_DICT root membership is not unique")
    nod = nods[0]
    target = nod["items"]["TEST_DICT"][-1]
    rows = [o for o in objects if o.get("object") == "DICTIONARY" and o.get("handle", [0])[-1] == target]
    require(len(rows) == 1 and rows[0]["numitems"] == 0, "TEST_DICT is not an empty dictionary")
    require(rows[0]["ownerhandle"][-1] == nod["handle"][-1], "owner mismatch in peer model")
    trace = run([dwgread, "-v4", str(path)])
    text = (trace.stdout + trace.stderr).decode(errors="replace")
    address, frame = locate_object_frame(data, text, expected_version, target, 42)
    size = len(frame)
    proof = observe_dictionary(frame, expected_version, target)
    require(proof["owner"]["value"] == nod["handle"][-1], "wire owner mismatch")
    require(proof["xdict"]["value"] == 0, "unexpected extension dictionary")
    return {"path": str(path), "sha256": sha(data), "sourceHandle": target,
            "nodHandle": nod["handle"][-1], "frameOffset": address, "frameSize": size,
            "crcValid": True, "wire": proof}


def candidate_result(candidate: str, path: Path, output: Path, *, binary: bool = False) -> dict:
    output.write_bytes(SENTINEL)
    command = [candidate, str(path)]
    if binary:
        command.append("-b")
    result = run(command + ["-o", str(output), "-y", "-d"], allow_failure=True)
    log = result.stderr.decode(errors="replace")
    if result.returncode:
        require(output.read_bytes() == SENTINEL, "candidate failure replaced destination")
    lines = [line.strip() for line in log.splitlines()
             if "failed:" in line or "Unqualified opaque" in line]
    observed = {"exit": result.returncode, "destinationPreservedOnFailure": result.returncode != 0,
                "diagnostics": lines[-6:]}
    if not result.returncode:
        summary = audit_bytes(output.read_bytes(), include_rows=True)
        observed["outputSha256"] = summary["sha256"]
        observed["cloningFieldCounts"] = summary["cloning_field_counts"]
        observed["selectedDictionary"] = selected_dictionary(summary)
    return observed


def selected_dictionary(summary: dict) -> dict:
    roots = [row for row in summary["dictionary_rows"]
             if any(entry["name"] == "TEST_DICT" for entry in row["entries"])]
    require(len(roots) == 1, "TEST_DICT root membership is not unique")
    target = next(entry["handle"] for entry in roots[0]["entries"]
                  if entry["name"] == "TEST_DICT")
    children = [row for row in summary["dictionary_rows"] if row["handle"] == target]
    require(len(children) == 1, "TEST_DICT child dictionary missing")
    return children[0]


def peer_output_check(ezdxf, ascii_output: Path, binary_output: Path,
                      source_model: dict, version: str, flag: int) -> dict:
    """Independently read successful candidate output in both DXF encodings."""
    outputs = {}
    for encoding, output in (("ascii", ascii_output), ("binary", binary_output)):
        summary = audit_bytes(output.read_bytes(), include_rows=True)
        child = selected_dictionary(summary)
        expected_cloning = flag if version == "r2000" else None
        require(child["cloning"] == expected_cloning, "physical cloning field mismatch")
        document = ezdxf.readfile(output)
        dictionary = document.rootdict.get("TEST_DICT")
        require(dictionary is not None and dictionary.dxftype() == "DICTIONARY",
                "peer cannot resolve TEST_DICT")
        require(int(dictionary.dxf.handle, 16) == source_model["sourceHandle"]
                and int(dictionary.dxf.owner, 16) == source_model["nodHandle"]
                and not list(dictionary.keys()), "peer dictionary graph mismatch")
        require(dictionary.dxf.hasattr("cloning") == (expected_cloning is not None),
                "peer cloning presence mismatch")
        if expected_cloning is not None:
            require(dictionary.dxf.cloning == expected_cloning, "peer cloning value mismatch")
        lines = list(document.modelspace().query("LINE"))
        require(len(lines) == 1 and tuple(lines[0].dxf.start) == (1., 2., 3.)
                and tuple(lines[0].dxf.end) == (4., 6., 9.), "peer line geometry mismatch")
        outputs[encoding] = {"sha256": sha(output.read_bytes()),
                             "dictionaryHandle": dictionary.dxf.handle,
                             "ownerHandle": dictionary.dxf.owner,
                             "physicalCloning": child["cloning"],
                             "lineStart": list(lines[0].dxf.start),
                             "lineEnd": list(lines[0].dxf.end)}
    return outputs


def physical_dxf_version(data: bytes) -> str:
    lines = data.decode("utf-8", "surrogateescape").splitlines()
    require(len(lines) % 2 == 0, "invalid ASCII DXF framing")
    pairs = [(lines[i].strip(), lines[i + 1].strip()) for i in range(0, len(lines), 2)]
    headers = [i for i, pair in enumerate(pairs) if pair == ("9", "$ACADVER")]
    require(len(headers) == 1 and headers[0] + 1 < len(pairs)
            and pairs[headers[0] + 1][0] == "1", "DXF version header missing or ambiguous")
    return pairs[headers[0] + 1][1]


def self_test() -> None:
    # Locally generated empty-dictionary frame controls. The source decoder
    # is this script's independent bit walker, not the library under test.
    controls = [
        ("r13", "4a804be48000000406c123410c30", None),
        ("r14", "4a804be5000000040600c123410c30", None),
        ("r2000", "4a94800000004be406803048d0430c00", 0),
        ("r2000", "4a96800000004be40640403048d0430c00", 1),
    ]
    for version, encoded, cloning in controls:
        frame = bytes.fromhex(encoded)
        proof = observe_dictionary(frame, version, 0x2F)
        require(proof["cloning"] == cloning and proof["owner"]["value"] == 0xC,
                "self-test valid frame")
        wrong_role = bytearray(frame)
        role_bit = proof["xdictBitOffset"] + 3
        wrong_role[role_bit // 8] ^= 0x80 >> (role_bit % 8)
        invalid = [frame[:-1], bytes(wrong_role)]
        for bad in invalid:
            try:
                observe_dictionary(bad, version, 0x2F)
            except ValueError:
                continue
            raise AssertionError("self-test accepted truncated or wrong-role frame")
    try:
        observe_dictionary(bytes.fromhex(controls[0][1]), "r14", 0x2F)
    except ValueError:
        pass
    else:
        raise AssertionError("self-test accepted R13 frame under R14 layout")
    print("dictionary witness bit-walker self-test: PASS")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output-dir", type=Path, help="parent for a new unique output directory")
    parser.add_argument("--self-test", action="store_true", help="check version and malformed frame controls")
    parser.add_argument("--dwgadd", default="dwgadd")
    parser.add_argument("--dwgread", default="dwgread")
    parser.add_argument("--candidate", help="optional libdxfrw dwg2dxf executable")
    parser.add_argument("--ezdxf-peer", action="store_true",
                        help="independently read successful candidate ASCII and binary DXF outputs")
    parser.add_argument("--oda", help="optional ODAFileConverter executable; needs desktop access and ezdxf")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        if args.output_dir is None:
            return
    if args.output_dir is None:
        parser.error("--output-dir is required unless only --self-test is selected")
    if args.ezdxf_peer and not args.candidate:
        parser.error("--ezdxf-peer requires --candidate")
    if args.ezdxf_peer:
        import ezdxf
    require(b"0.14" in run([args.dwgadd, "--version"]).stdout, "dwgadd 0.14 required")
    require(b"0.14" in run([args.dwgread, "--version"]).stdout, "dwgread 0.14 required")
    args.output_dir.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="ac1012-dictionary-", dir=args.output_dir))
    results = {"schema": 1, "scope": "generated dictionary source fields; not native semantics",
               "generatorSha256": sha(Path(__file__).read_bytes()),
               "producer": "GNU LibreDWG dwgadd 0.14",
               "executables": {"dwgadd": executable_identity(args.dwgadd),
                               "dwgread": executable_identity(args.dwgread)}, "samples": {}}
    if args.candidate:
        results["executables"]["candidate"] = executable_identity(args.candidate)
    if args.ezdxf_peer:
        results["ezdxfPeerVersion"] = ezdxf.__version__
    for version in VERSIONS:
        for flag in (0, 1):
            name = f"{version}-cloning-{flag}"
            recipe = root / (name + ".dwgadd")
            recipe.write_text(f'dimstyle "OTHER"\nline (1 2 3) (4 6 9)\ndictionary "TEST_DICT" "unused" 0\ndictionary.numitems = 0\ndictionary.cloning = {flag}\n')
            path = root / (name + ".dwg")
            run([args.dwgadd, "--as", version, "-o", str(path), str(recipe)])
            item = model_and_frame(args.dwgread, path, version)
            require(item["wire"]["cloning"] == (flag if version == "r2000" else None), "cloning boundary mismatch")
            if args.candidate:
                output = root / (name + "-candidate.dxf")
                item["candidate"] = candidate_result(args.candidate, path, output)
                if args.ezdxf_peer:
                    binary = root / (name + "-candidate-binary.dxf")
                    item["candidateBinary"] = candidate_result(args.candidate, path, binary, binary=True)
                    require((item["candidate"]["exit"] == 0) == (item["candidateBinary"]["exit"] == 0),
                            "candidate ASCII/binary conversion status differs")
                    if item["candidate"]["exit"] == 0:
                        item["ezdxfPeer"] = peer_output_check(ezdxf, output, binary, item, version, flag)
            results["samples"][name] = item
    for version in ("r13", "r14", "r2000"):
        x = results["samples"][f"{version}-cloning-0"]["wire"]["frameSha256"]
        y = results["samples"][f"{version}-cloning-1"]["wire"]["frameSha256"]
        require((x != y) == (version == "r2000"), "unexpected version/flag frame comparison")
    if args.oda:
        import ezdxf  # Optional research dependency; do not install automatically.
        inputs, produced = root / "oda-input", root / "oda-r13"
        inputs.mkdir(); produced.mkdir()
        for flag in (0, 1):
            document = ezdxf.new("R2000")
            document.modelspace().add_line((1, 2, 3), (4, 6, 9))
            dictionary = document.rootdict.add_new_dict("TEST_DICT")
            dictionary.dxf.cloning = flag
            document.saveas(inputs / f"seed-{flag}.dxf")
        run([args.oda, str(inputs), str(produced), "ACAD13", "DWG", "0", "1", "*.dxf"])
        bundle_info = Path(args.oda).resolve().parent.parent / "Info.plist"
        bundle_version = None
        if bundle_info.is_file():
            with bundle_info.open("rb") as stream:
                bundle_version = plistlib.load(stream).get("CFBundleShortVersionString")
        oda = {"producer": "ODA File Converter", "executable": executable_identity(args.oda),
               "bundleVersion": bundle_version, "ezdxfVersion": ezdxf.__version__, "samples": {}}
        probe_input, probe_output = root / "oda-peer-input", root / "oda-peer-output"
        probe_input.mkdir(); probe_output.mkdir()
        for name in results["samples"]:
            shutil.copy2(root / (name + ".dwg"), probe_input / (name + ".dwg"))
        run([args.oda, str(probe_input), str(probe_output), "ACAD14", "DXF", "0", "1", "*.dwg"])
        oda["libredwgSampleScreen"] = {}
        for name in results["samples"]:
            output = probe_output / (name + ".dxf")
            error = probe_output / (name + ".dxf.err")
            require(output.is_file() != error.is_file(), "ODA sample screen has ambiguous output")
            oda["libredwgSampleScreen"][name] = {
                "accepted": output.is_file(),
                "error": error.read_text(errors="replace").strip() if error.is_file() else None,
                "outputSha256": sha(output.read_bytes()) if output.is_file() else None}
        for flag in (0, 1):
            path = produced / f"seed-{flag}.dwg"
            require(path.is_file(), "ODA did not produce the AC1012 file")
            item = model_and_frame(args.dwgread, path, "r13")
            seed = inputs / f"seed-{flag}.dxf"
            seed_audit = audit_bytes(seed.read_bytes(), include_rows=True)
            seed_child = selected_dictionary(seed_audit)
            require(seed_child["cloning"] == flag, "seed cloning value was not physically written")
            item["seed"] = {"path": str(seed), "sha256": seed_audit["sha256"],
                            "selectedDictionary": seed_child}
            if args.candidate:
                item["candidate"] = candidate_result(args.candidate, path, root / f"oda-{flag}-candidate.dxf")
            oda["samples"][str(flag)] = item
        require(oda["samples"]["0"]["wire"]["frameSha256"] == oda["samples"]["1"]["wire"]["frameSha256"], "ODA source dictionary frames differ")
        for target in ("ACAD14", "ACAD2000"):
            out = root / ("oda-to-" + target.lower()); out.mkdir()
            run([args.oda, str(produced), str(out), target, "DXF", "0", "1", "*.dwg"])
            for flag in (0, 1):
                path = out / f"seed-{flag}.dxf"
                require(path.is_file(), "ODA conversion output missing")
                dxf_bytes = path.read_bytes()
                physical_version = physical_dxf_version(dxf_bytes)
                require(physical_version == ("AC1014" if target == "ACAD14" else "AC1015"),
                        "ODA output DXF version mismatch")
                summary = audit_bytes(dxf_bytes, include_rows=True)
                child = selected_dictionary(summary)
                independently_read = ezdxf.readfile(path)
                read_dictionary = independently_read.rootdict.get("TEST_DICT")
                require(read_dictionary is not None, "ezdxf cannot find TEST_DICT")
                oda["samples"][str(flag)][target] = {"path": str(path), "sha256": summary["sha256"],
                    "physicalVersion": physical_version,
                    "cloningFieldCounts": summary["cloning_field_counts"], "selectedDictionary": child,
                    "ezdxfReadback": {"readVersion": independently_read.dxfversion,
                        "cloningAttributePresent": read_dictionary.dxf.hasattr("cloning"),
                        "cloningValueOrDefault": read_dictionary.dxf.cloning}}
        results["oda"] = oda
    (root / "receipt.json").write_text(json.dumps(results, indent=2) + "\n")
    print(root)
    print(json.dumps({name: {"sha256": item["sha256"], "frameSha256": item["wire"]["frameSha256"],
                            "cloning": item["wire"]["cloning"]}
                      for name, item in results["samples"].items()}, indent=2))


if __name__ == "__main__":
    main()
