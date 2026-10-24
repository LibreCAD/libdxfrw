#!/usr/bin/env python3
"""Focused, advisory DXF differential controls; requires ezdxf exactly 1.4.4.

Generate drawings locally from scratch, run ``dwg2dxf input -o output`` and
compare ordinary-read semantic projections before a separate audit load. No
recovery retry or audit repair can turn a failed comparison into a pass. The
result is supplemental evidence, never a support qualification receipt.

Example: python -I -B tools/run_independent_dxf.py --converter build/dwg2dxf/dwg2dxf
         --family header90 --output build/header90-independent.json
"""

from __future__ import annotations

import argparse
import configparser
from contextlib import contextmanager
import hashlib
import importlib.abc
import importlib.metadata
import io
import json
import math
import os
from pathlib import Path
import subprocess
import struct
import sys
import tempfile

# -B prevents cache writes, not reads. Select an empty private cache prefix
# before importing repository helpers or ezdxf so stale installed bytecode
# cannot stand in for the source content attested below.
_ORACLE_CACHE_ROOT = tempfile.TemporaryDirectory(prefix="libdxfrw-oracle-bytecode-")
sys.pycache_prefix = _ORACLE_CACHE_ROOT.name
sys.dont_write_bytecode = True

# -I excludes the script directory too. Add only this trusted helper directory,
# never cwd/PYTHONPATH, before loading our hash-bound repository helpers.
sys.path.insert(0, str(Path(__file__).resolve().parent))
from bounded_process import OutputLimitExceeded, run_bounded
from normalize_drawing_result import normalize, typed_equal


PINNED_VERSION = "1.4.4"
PINNED_REVISION = "dd7f6cc34983fdbf0b2b910f2497e0720d06dd16"
FAMILIES = ("core", "spline-unit", "mtext-height", "header90")
BINARY_PREFIX = b"AutoCAD Binary DXF\r\n\x1a\x00"
MAX_DRAWING_BYTES = 2 * 1024 * 1024
MAX_DRAWING_TAGS = 100000
MAX_ATTESTED_FILE_BYTES = 512 * 1024 * 1024
ORACLE_PROFILE = {
    "load_proxy_graphics": True,
    "store_proxy_graphics": True,
    "filter_invalid_xdata_group_codes": False,
    "write_fixed_meta_data_for_testing": True,
    "disable_c_ext": True,
    "use_c_ext": False,
}
PROFILE_NAME = "generated-ezdxf-1.4.4-pure-python-v1"


class _DisableOracleNativeImports(importlib.abc.MetaPathFinder):
    def find_spec(self, fullname, path=None, target=None):
        # Pinned npshapes directly imports np_support even when disable_c_ext
        # is true. Its supported ImportError fallback keeps this profile
        # genuinely Python-only without modifying the installed package.
        if fullname.startswith("ezdxf.acc."):
            raise ModuleNotFoundError("native oracle modules disabled by " + PROFILE_NAME,
                                      name=fullname)
        return None


def require_profile(values, *, isolated, loaded_configs, expected_config):
    if not isolated:
        raise ValueError("oracle requires a fresh isolated interpreter; invoke Python with -I")
    if not typed_equal(values, ORACLE_PROFILE):
        raise ValueError("effective oracle options differ from " + PROFILE_NAME)
    if loaded_configs != [str(expected_config)]:
        raise ValueError("oracle loaded configuration outside the controlled profile")


def require_context_unchanged(before, after):
    if not typed_equal(before, after):
        raise ValueError("oracle content or configuration changed during comparison")


def missing_required_checks(observed, required):
    """Return required full-file checks absent from a parser's observed claims."""
    if (type(observed) is not list
            or any(type(value) is not str or not value for value in observed)
            or len(set(observed)) != len(observed)):
        raise ValueError("observed checks must be a list of unique nonempty strings")
    if (type(required) is not list or not required
            or any(type(value) is not str or not value for value in required)
            or len(set(required)) != len(required)):
        raise ValueError("required checks must be a nonempty list of unique strings")
    return [check for check in required if check not in observed]


def bytecode_profile():
    prefix = Path(_ORACLE_CACHE_ROOT.name).resolve()
    if (sys.pycache_prefix != _ORACLE_CACHE_ROOT.name or not sys.dont_write_bytecode
            or not prefix.is_dir() or any(prefix.rglob("*.pyc"))):
        raise ValueError("oracle bytecode cache policy changed")
    return {"prefix": str(prefix), "cacheReads": "fresh private prefix only",
            "cacheWrites": False}


def effective_profile(ezdxf, config_path):
    values = {key: getattr(ezdxf.options, key) for key in ORACLE_PROFILE}
    loaded = [str(Path(path).resolve()) for path in ezdxf.options.loaded_config_files]
    require_profile(values, isolated=bool(sys.flags.isolated),
                    loaded_configs=loaded, expected_config=config_path)
    from ezdxf import acc
    from ezdxf.math import Vec3
    from ezdxf.lldxf.types import DXFTag
    if (acc.USE_C_EXT or Vec3.__module__ != "ezdxf.math._vector"
            or DXFTag.__module__ != "ezdxf.lldxf.types"
            or any(name.startswith("ezdxf.acc.") for name in sys.modules)):
        raise ValueError("loaded oracle accelerator path differs from the pure-Python profile")
    # Public options.write includes all effective default/config sections,
    # not just the small policy allowlist checked above.
    stream = io.StringIO()
    ezdxf.options.write(stream)
    config = configparser.ConfigParser(interpolation=None)
    config.read_string(stream.getvalue())
    full_options = {section: dict(config.items(section)) for section in config.sections()}
    encoded = json.dumps(full_options, sort_keys=True, separators=(",", ":"), allow_nan=False)
    return {"name": PROFILE_NAME, "effectiveOptions": values,
            "fullOptions": full_options,
            "fullOptionsSha256": hashlib.sha256(encoded.encode("utf-8")).hexdigest(),
            "loadedConfigs": [{"path": str(config_path), **digest(config_path)}],
            "pythonIsolated": bool(sys.flags.isolated),
            "pythonOptimize": sys.flags.optimize,
            "bytecode": bytecode_profile(),
            "accelerator": {"requested": "disabled-before-import", "inUse": False,
                            "directNativeImports": "blocked for ezdxf.acc.*",
                            "vectorImplementation": Vec3.__module__,
                            "tagImplementation": DXFTag.__module__,
                            "bootstrapModule": {"path": str(Path(acc.__file__).resolve()),
                                                **digest(Path(acc.__file__))}}}


@contextmanager
def controlled_runtime():
    if sys.version_info < (3, 10):
        raise ValueError("oracle requires Python 3.10 or newer")
    if not sys.flags.isolated:
        raise ValueError("oracle requires a fresh isolated interpreter; invoke Python with -I")
    if sys.flags.optimize:
        raise ValueError("oracle controls require Python without -O or -OO")
    if any(name == "ezdxf" or name.startswith("ezdxf.") for name in sys.modules):
        raise ValueError("oracle was imported before controlled configuration bootstrap")
    original_directory = Path.cwd()
    original_environment = {key: value for key, value in os.environ.items()
                            if key.startswith("EZDXF_") or key == "XDG_CONFIG_HOME"}
    with tempfile.TemporaryDirectory(prefix="libdxfrw-oracle-profile-") as directory:
        root = Path(directory).resolve()
        config_path = root / "profile.ini"
        config = configparser.ConfigParser(interpolation=None)
        config["core"] = {key: str(value).lower() for key, value in ORACLE_PROFILE.items()
                          if key != "use_c_ext"}
        with config_path.open("w", encoding="utf-8") as output:
            config.write(output)
        import_guard = _DisableOracleNativeImports()
        try:
            for key in original_environment:
                os.environ.pop(key, None)
            os.environ.update(EZDXF_CONFIG_FILE=str(config_path),
                              EZDXF_DISABLE_C_EXT="true", XDG_CONFIG_HOME=str(root / "config"))
            os.chdir(root)
            sys.meta_path.insert(0, import_guard)
            import ezdxf
            require_version(ezdxf)
            before = effective_profile(ezdxf, config_path)
            yield ezdxf, config_path, before
            if not typed_equal(before, effective_profile(ezdxf, config_path)):
                raise ValueError("oracle configuration changed during comparison")
        finally:
            if import_guard in sys.meta_path:
                sys.meta_path.remove(import_guard)
            os.chdir(original_directory)
            for key in tuple(os.environ):
                if key.startswith("EZDXF_") or key == "XDG_CONFIG_HOME":
                    os.environ.pop(key, None)
            os.environ.update(original_environment)


def require_effective_profile(ezdxf):
    # The bootstrap path is runner-owned; reject mutations before each semantic
    # read/generation, not only at the final evidence boundary.
    path = Path(os.environ["EZDXF_CONFIG_FILE"]).resolve()
    values = {key: getattr(ezdxf.options, key) for key in ORACLE_PROFILE}
    require_profile(values, isolated=bool(sys.flags.isolated),
                    loaded_configs=[str(Path(p).resolve()) for p in ezdxf.options.loaded_config_files],
                    expected_config=path)
    from ezdxf import acc
    if acc.USE_C_EXT or any(name.startswith("ezdxf.acc.") for name in sys.modules):
        raise ValueError("oracle accelerator state changed before semantic processing")


def file_identity(stat):
    return (stat.st_dev, stat.st_ino, stat.st_size, stat.st_mtime_ns)


def digest(path):
    hashed, size = hashlib.sha256(), 0
    with path.open("rb") as source:
        before = file_identity(os.fstat(source.fileno()))
        if before[2] > MAX_ATTESTED_FILE_BYTES:
            raise ValueError("attested file exceeds content hashing budget: " + str(path))
        while chunk := source.read(65536):
            hashed.update(chunk)
            size += len(chunk)
            if size > before[2]:
                raise ValueError("file grew during content hashing: " + str(path))
        after = file_identity(os.fstat(source.fileno()))
    if before != after or after != file_identity(path.stat()) or size != after[2]:
        raise ValueError("file changed during content hashing: " + str(path))
    return {"sha256": hashed.hexdigest(), "size": size}


def byte_digest(data):
    return {"sha256": hashlib.sha256(data).hexdigest(), "size": len(data)}


def drawing_bytes(path, limit=MAX_DRAWING_BYTES):
    if type(limit) is not int or limit <= 0:
        raise ValueError("drawing byte limit must be a positive integer")
    with path.open("rb") as source:
        before = file_identity(os.fstat(source.fileno()))
        data = source.read(limit + 1)
        after = file_identity(os.fstat(source.fileno()))
    if len(data) > limit:
        raise ValueError("drawing exceeds advisory byte admission limit: " + str(path))
    if before != after or after != file_identity(path.stat()) or len(data) != after[2]:
        raise ValueError("drawing changed during snapshot admission: " + str(path))
    return data


def bounded_tags(tags, limit=MAX_DRAWING_TAGS):
    if type(limit) is not int or limit <= 0:
        raise ValueError("drawing tag limit must be a positive integer")
    for index, tag in enumerate(tags):
        if index >= limit:
            raise ValueError("drawing exceeds advisory tag admission limit")
        yield tag


def ordinary_tags(data):
    from ezdxf.lldxf.tagger import ascii_tags_loader, binary_tags_loader
    if data.startswith(BINARY_PREFIX):
        return bounded_tags(binary_tags_loader(data, errors="strict"))
    return bounded_tags(ascii_tags_loader(io.StringIO(data.decode("utf-8", "strict"))))


def ordinary_load(ezdxf, path, data):
    # Same non-recovery tag path as pinned readfile, but all projections and
    # fresh audit objects consume one bounded immutable byte snapshot.
    require_effective_profile(ezdxf)
    from ezdxf.document import Drawing
    doc = Drawing.load(ordinary_tags(data))
    doc.filename = str(path)
    return doc


def require_version(module):
    if module.__version__ != PINNED_VERSION:
        raise ValueError("requires ezdxf %s, found %s" % (
            PINNED_VERSION, module.__version__))


def oracle_context(ezdxf, config_path):
    distribution = importlib.metadata.distribution("ezdxf")
    package_root = Path(distribution.locate_file("ezdxf")).resolve()
    module_path = Path(ezdxf.__file__).resolve()
    if distribution.version != PINNED_VERSION or module_path.parent != package_root:
        raise ValueError("loaded ezdxf origin/version does not match the attested distribution")
    files = []
    for entry in distribution.files or ():
        name = str(entry).replace("\\", "/")
        if not name.startswith("ezdxf/") or "__pycache__" in name or name.endswith(".pyc"):
            continue
        path = Path(distribution.locate_file(entry))
        if path.is_file():
            files.append({"path": name, **digest(path)})
    if not files:
        raise ValueError("installed oracle package files are not attestable")
    files.sort(key=lambda item: item["path"])
    package_digest = hashlib.sha256(json.dumps(files, sort_keys=True,
        separators=(",", ":"), allow_nan=False).encode("utf-8")).hexdigest()
    dependencies = {}
    for name in ("numpy", "pyparsing", "typing_extensions", "fonttools"):
        try:
            dependencies[name] = importlib.metadata.version(name)
        except importlib.metadata.PackageNotFoundError:
            dependencies[name] = None
    return {"version": ezdxf.__version__, "referenceReleaseRevision": PINNED_REVISION,
            "packageOrigin": str(package_root), "moduleDigest": digest(module_path),
            "packageFileCount": len(files), "packageContentSha256": package_digest,
            "dependencies": dependencies, "dependencyAttribution": "distribution versions only; not loaded dependency content",
            "pythonVersion": sys.version,
            "pythonExecutable": str(Path(sys.executable).resolve()),
            "pythonDigest": digest(Path(sys.executable)),
            "generatorDigest": digest(Path(__file__)),
            "configuration": effective_profile(ezdxf, config_path),
            "helperDigests": {name: digest(Path(__file__).with_name(name)) for name in
                              ("bounded_process.py", "normalize_drawing_result.py")}}


def run_conversion(command, timeout, output_limit=65536):
    """Bound converter time and captured log bytes; no recovery/repair retry."""
    result = run_bounded(command, timeout, output_limit)
    return {"argv": command, "exitStatus": result.returncode,
            "stdout": result.stdout.decode("utf-8", "backslashreplace"),
            "stderr": result.stderr.decode("utf-8", "backslashreplace")}


def generate(ezdxf, family, path, binary):
    require_effective_profile(ezdxf)
    from ezdxf.lldxf.tagger import ascii_tags_loader, tag_compiler
    from ezdxf.lldxf.tags import DXFTag
    from ezdxf.lldxf.tagwriter import BinaryTagWriter, TagWriter

    doc = ezdxf.new("R2018")
    model = doc.modelspace()
    if family in ("core", "header90"):
        model.add_line((1.25, -2.5, 3.75), (-4.5, 5.25, -6.75))
    if family == "core":
        model.add_point((8.5, -9.25, 10.75))
        model.add_circle((11.5, 12.25, -13.75), 2.625)
        model.add_arc((-14.5, 15.25, 16.75), 3.125, 17.5, 231.25)
    elif family == "spline-unit":
        spline = model.add_spline(degree=2, dxfattribs={"flags": 4})
        spline.control_points = [(0, 0, 1), (2, 3, 4), (5, -1, 2)]
        spline.knots = [0, 0, 0, 1, 1, 1]
        # Empty weights intentionally mean unit weights, not malformed data.
    elif family == "mtext-height":
        model.add_mtext("independent-height-sentinel", dxfattribs={
            "insert": (11, 22, 33), "char_height": 2.5,
            "width": 18.75, "defined_height": 12.5,
            "attachment_point": 5,
        })
    elif family == "header90":
        doc.header["$ACADMAINTVER"] = 70001
    # ezdxf updates ACADMAINTVER during document export. Inject the literal
    # wide sentinel afterwards, before independent low-level tag serialization.
    stream = io.StringIO()
    doc.write(stream)
    tags = list(ascii_tags_loader(io.StringIO(stream.getvalue())))
    if family == "header90":
        for index, tag in enumerate(tags):
            if tag.code == 9 and tag.value == "$ACADMAINTVER":
                tags[index + 1] = DXFTag(90, 70001)
                break
        else:
            raise ValueError("independent generator omitted maintenance variable")
    if binary:
        with path.open("wb") as output:
            writer = BinaryTagWriter(output, dxfversion="AC1032")
            writer.write_signature()
            writer.write_tags(tag_compiler(iter(tags)))
    else:
        with path.open("w", encoding="utf-8", newline="") as output:
            TagWriter(output, dxfversion="AC1032").write_tags(tags)


def raw_observations(data, doc):
    binary = data.startswith(BINARY_PREFIX)
    tags = ordinary_tags(data)
    records, current = [], []
    headers, header_name, in_header = {}, None, False
    for tag in tags:
        if tag.code == 2 and tag.value == "HEADER":
            in_header = True
        elif tag.code == 0 and tag.value == "ENDSEC":
            in_header = False
        if in_header:
            if tag.code == 9:
                header_name = str(tag.value)
                headers.setdefault(header_name, [])
            elif header_name is not None and tag.code != 2:
                headers[header_name].append([tag.code, str(tag.value)])
        if tag.code == 0:
            if current:
                records.append(current)
            current = []
        current.append(tag)
    if current:
        records.append(current)
    handles, handle_names = {}, {}
    entities = []
    table_owners = []
    table_handle = None
    for record in records:
        kind = str(record[0].value)
        handle = next((str(t.value).upper() for t in record if t.code in (5, 105)), None)
        owner, depth = None, 0
        for tag in record[1:]:
            if tag.code == 100:
                break
            if tag.code == 102:
                if str(tag.value).startswith("{"):
                    depth += 1
                elif str(tag.value) == "}":
                    depth -= 1
            elif tag.code == 330 and depth == 0 and owner is None:
                owner = str(tag.value).upper()
        name = next((str(t.value) for t in record if t.code == 2), None)
        if handle and kind != "SECTION":
            handles[handle] = kind
            if name is not None:
                handle_names[handle] = name
        if kind == "TABLE":
            table_handle = handle
        elif kind == "ENDTAB":
            table_handle = None
        elif table_handle and owner != table_handle:
            table_owners.append({"type": kind, "name": name, "handle": handle,
                                 "owner": owner, "expectedTable": table_handle})
        if kind in ("LINE", "POINT", "CIRCLE", "ARC", "SPLINE", "MTEXT"):
            entities.append({"type": kind, "handle": handle, "owner": owner,
                             "tags": [[t.code, str(t.value)] for t in record[1:]]})
    created = [{"handle": handle, "type": entity.dxftype()}
               for handle, entity in doc.entitydb.items()
               if handle.upper() not in handles and entity.is_alive]
    owner_observations = [{"handle": item["handle"], "rawOwner": item["owner"],
                           "readerOwner": doc.entitydb[item["handle"]].dxf.get("owner")}
                          for item in entities if item["handle"] in doc.entitydb]
    observed_headers = ("$ACADVER", "$ACADMAINTVER", "$REQUIREDVERSIONS", "$HANDSEED", "$INSUNITS")
    return {"binary": binary,
            "header": {name: headers[name] for name in observed_headers if name in headers},
            "headerVariableCount": len(headers), "entities": entities,
            "rawHandleTypes": handles, "rawHandleNames": handle_names,
            "readerCreatedRecords": created,
            "readerOwnerObservations": owner_observations,
            "tableOwnerMismatches": table_owners}


def project(ezdxf, path, family, data):
    # Ordinary read only. Capture every compared value before audit is called.
    doc = ordinary_load(ezdxf, path, data)
    raw = raw_observations(data, doc)
    model = doc.modelspace()
    owner = model.block_record_handle.upper()
    identity = {owner: "layout:Model"}
    entities, relationships = [], []
    for index, entity in enumerate(model):
        kind = entity.dxftype()
        handle = entity.dxf.handle.upper()
        entity_owner = entity.dxf.owner.upper()
        identity[handle] = "entity:%d:%s" % (index, kind)
        fields = {"layer": entity.dxf.layer}
        if kind == "LINE":
            fields.update(start=list(entity.dxf.start), end=list(entity.dxf.end))
        elif kind == "POINT":
            fields["location"] = list(entity.dxf.location)
        elif kind in ("CIRCLE", "ARC"):
            fields.update(center=list(entity.dxf.center), radius=entity.dxf.radius,
                          extrusion=list(entity.dxf.extrusion))
            if kind == "ARC":
                fields.update(startAngleDegrees=entity.dxf.start_angle,
                              endAngleDegrees=entity.dxf.end_angle)
        elif kind == "SPLINE":
            controls = [[float(value) for value in point] for point in entity.control_points]
            fields.update(degree=entity.dxf.degree, flags=entity.dxf.flags,
                          knots=[float(value) for value in entity.knots], controlPoints=controls,
                          fitPoints=[[float(value) for value in point] for point in entity.fit_points],
                          effectiveWeights=[float(value) for value in entity.weights]
                              or [1.0] * len(controls))
        elif kind == "MTEXT":
            fields.update(text=entity.text, insert=list(entity.dxf.insert),
                          charHeight=entity.dxf.char_height, width=entity.dxf.width,
                          definedHeight=entity.dxf.defined_height,
                          attachmentPoint=entity.dxf.attachment_point)
        else:
            raise ValueError("unexpected sentinel entity: " + kind)
        entities.append({"type": kind, "handle": handle, "owner": entity_owner,
                         "fields": fields})
        relationships.append({"from": handle, "kind": "owner", "to": entity_owner})
    result = normalize({
        "format": "DXF", "version": doc.dxfversion,
        "toolVersion": "ezdxf " + ezdxf.__version__, "exitStatus": 0,
        "diagnostics": [], "entities": entities,
        "objects": [{"type": "BLOCK_RECORD", "handle": owner, "name": "*Model_Space"}],
        "relationships": relationships, "opaquePayloadHashes": [],
        "projectionScope": {"family": family, "identity": "ordered-modelspace-plus-owner-role",
                            "excluded": ["full-resource-graph", "paper-layouts", "rendering"],
                            "anglePolicy": "degree fields allow four binary64 ULPs for radians handoff"},
    })
    violations = []
    if raw["rawHandleTypes"].get(owner) != "BLOCK_RECORD":
        violations.append("modelspace owner is not a literal BLOCK_RECORD")
    if raw["rawHandleNames"].get(owner, "").upper() != "*MODEL_SPACE":
        violations.append("modelspace owner is not the literal reserved modelspace record")
    if any(entity["owner"] != owner for entity in raw["entities"]):
        violations.append("raw sentinel owner differs from modelspace identity")
    if family == "header90":
        result["header"] = {"$ACADMAINTVER": doc.header["$ACADMAINTVER"]}
        if raw["header"].get("$ACADMAINTVER") != [[90, "70001"]]:
            violations.append("literal HEADER group90/value70001 not retained")
    elif family == "mtext-height":
        height_tags = [value for entity in raw["entities"] for code, value in entity["tags"]
                       if entity["type"] == "MTEXT" and code == 46]
        if len(height_tags) != 1 or float(height_tags[0]) != 12.5:
            violations.append("literal MTEXT group46/height12.5 not retained")
    return result, identity, raw, violations


def mapped(projection, identity):
    value = json.loads(json.dumps(projection))
    for item in value["entities"]:
        item["handle"] = identity[item["handle"]]
        item["owner"] = identity[item["owner"]]
    for item in value["objects"]:
        item["handle"] = identity[item["handle"]]
    for edge in value["relationships"]:
        edge["from"], edge["to"] = identity[edge["from"]], identity[edge["to"]]
    return normalize(value)


def differences(left, right, path="projection"):
    if type(left) is not type(right):
        return [path]
    if type(left) is float and path.endswith("AngleDegrees"):
        # Only angular fields cross a degrees→radians→degrees API boundary.
        # Retain their literal values in the report; never apply this allowance
        # to coordinates, integer metadata, knots or unrelated floating fields.
        if abs(left - right) <= 4 * max(math.ulp(left), math.ulp(right)):
            return []
    if isinstance(left, dict):
        if left.keys() != right.keys():
            return [path + ".keys"]
        return [item for key in left for item in differences(left[key], right[key], path + "." + key)]
    if isinstance(left, list):
        if len(left) != len(right):
            return [path + ".length"]
        return [item for index, (a, b) in enumerate(zip(left, right))
                for item in differences(a, b, "%s[%d]" % (path, index))]
    return [] if typed_equal(left, right) else [path]


def audit(ezdxf, path, data):
    # Fresh load: audit may repair, but cannot modify the ordinary snapshot.
    result = ordinary_load(ezdxf, path, data).audit()
    return {name: [{"code": int(item.code), "message": item.message}
                   for item in getattr(result, name)] for name in ("fixes", "errors")}


def run_case(ezdxf, converter, directory, family, input_binary, output_binary, timeout):
    from ezdxf.lldxf.const import DXFError

    name = "%s-%s-to-%s" % (family, "binary" if input_binary else "ascii",
                              "binary" if output_binary else "ascii")
    source, destination, repeated = (directory / (name + suffix) for suffix in
                                    (" source.dxf", " output.dxf", " repeated.dxf"))
    record = {"case": name, "family": family, "qualificationEligible": False,
              "status": "failed", "ordinaryRead": {},
              "scope": "ordered modelspace sentinel fields and owner edges only"}
    stage = "generation"
    try:
        generate(ezdxf, family, source, input_binary)
        stage = "input-byte-admission"
        before_bytes = drawing_bytes(source)
        record["inputDigest"] = byte_digest(before_bytes)
        stage = "input-ordinary-read"
        before, before_map, before_raw, before_violations = project(ezdxf, source, family, before_bytes)
        record["ordinaryRead"]["input"] = "success"
        record.update(inputProjection=before, inputIdentityMapping=before_map,
                      inputRawObservations=before_raw, inputScopeViolations=before_violations)
        if before_violations:
            raise ValueError("generated control violates its declared scope")
        if before_raw["binary"] != input_binary:
            raise ValueError("input encoding differs from requested route")
        expected_count = 4 if family == "core" else 1
        if len(before["entities"]) != expected_count:
            raise ValueError("generator sentinel count differs from declared family")
        if family == "spline-unit":
            fields = before["entities"][0]["fields"]
            if (fields["degree"] != 2 or fields["flags"] != 4
                    or len(fields["knots"]) != 6 or len(fields["controlPoints"]) != 3
                    or any(code == 41 for entity in before_raw["entities"]
                           for code, _ in entity["tags"])):
                raise ValueError("generated implicit-unit spline degree/flags/counts are invalid")
        stage = "input-audit-eligibility"
        record["inputAudit"] = audit(ezdxf, source, before_bytes)
        if any(record["inputAudit"].values()):
            record["inputEligibility"] = "generator-or-oracle-ineligible"
            raise ValueError("source requires audit changes or reports errors")
        record["inputEligibility"] = "ordinary-read-and-clean-audit"
        command = [str(converter), str(source), "-o", str(destination)]
        if output_binary:
            command.append("-b")
        stage = "conversion"
        result = run_conversion(command, timeout)
        record["conversion"] = result
        if result["exitStatus"] != 0 or not destination.is_file():
            raise ValueError("converter failed or did not publish a drawing")
        stage = "output-byte-admission"
        after_bytes = drawing_bytes(destination)
        record["outputDigest"] = byte_digest(after_bytes)
        stage = "output-ordinary-read"
        after, after_map, after_raw, after_violations = project(ezdxf, destination, family, after_bytes)
        record["ordinaryRead"]["output"] = "success"
        record.update(outputProjection=after, outputIdentityMapping=after_map,
                      outputRawObservations=after_raw, outputScopeViolations=after_violations)
        if after_raw["binary"] != output_binary:
            raise ValueError("output encoding differs from requested route")
        stage = "comparison"
        record["mismatches"] = differences(mapped(before, before_map), mapped(after, after_map))
        record["status"] = "mismatch" if record["mismatches"] or after_violations else "scope-match"
        stage = "repeat-conversion"
        command = [str(converter), str(destination), "-o", str(repeated)]
        if output_binary:
            command.append("-b")
        repeat_result = run_conversion(command, timeout)
        record["repeatConversion"] = repeat_result
        if repeat_result["exitStatus"] != 0 or not repeated.is_file():
            raise ValueError("repeat converter failed or did not publish a drawing")
        stage = "repeated-byte-admission"
        final_bytes = drawing_bytes(repeated)
        record["repeatedDigest"] = byte_digest(final_bytes)
        stage = "third-ordinary-read"
        final, final_map, final_raw, final_violations = project(ezdxf, repeated, family, final_bytes)
        record["ordinaryRead"]["repeated"] = "success"
        record.update(repeatedProjection=final, repeatedIdentityMapping=final_map,
                      repeatedRawObservations=final_raw,
                      repeatedScopeViolations=final_violations)
        if final_raw["binary"] != output_binary:
            raise ValueError("repeated output encoding differs from requested route")
        record["repeatMismatches"] = differences(mapped(before, before_map), mapped(final, final_map))
        if record["repeatMismatches"] or final_violations:
            record["status"] = "mismatch"
        # Repairs/errors are recorded separately and never used to repair a comparison.
        stage = "separate-audit-load"
        record["outputAudit"] = audit(ezdxf, destination, after_bytes)
        record["repeatedAudit"] = audit(ezdxf, repeated, final_bytes)
        if (record["status"] == "scope-match"
                and (any(record["outputAudit"].values()) or any(record["repeatedAudit"].values()))):
            record["status"] = "scope-match-with-audit-findings"
    except (OSError, ValueError, KeyError, DXFError, subprocess.SubprocessError) as exc:
        if stage == "generation" or stage.startswith("input-"):
            record.setdefault("inputEligibility", "generator-or-oracle-ineligible")
        record["status"] = "failed"
        record["failureStage"] = stage
        record["failure"] = "%s: %s" % (type(exc).__name__, exc)
    return record


def self_test():
    from types import SimpleNamespace
    require_version(SimpleNamespace(__version__=PINNED_VERSION))
    try:
        require_version(SimpleNamespace(__version__="1.4.3"))
    except ValueError:
        pass
    else:
        raise AssertionError("unpinned version accepted")
    context = {"packageContentSha256": "A" * 64,
               "generatorDigest": {"sha256": "B" * 64, "size": 7},
               "configuration": {"load_proxy_graphics": True}}
    require_context_unchanged(context, json.loads(json.dumps(context)))
    for key in context:
        changed = json.loads(json.dumps(context))
        changed[key] = None
        try:
            require_context_unchanged(context, changed)
        except ValueError:
            pass
        else:
            raise AssertionError("changed oracle context admitted: " + key)
    required_full_file_checks = ["sentinel", "envelope", "trailer", "versioned-header-fields"]
    assert missing_required_checks(["minimum-prefix"], required_full_file_checks) == required_full_file_checks
    assert missing_required_checks([], required_full_file_checks) == required_full_file_checks
    assert missing_required_checks(required_full_file_checks, required_full_file_checks) == []
    for observed, required in ((["minimum-prefix", "minimum-prefix"], required_full_file_checks),
                               ([1], required_full_file_checks),
                               (["minimum-prefix"], "sentinel")):
        try:
            missing_required_checks(observed, required)
        except ValueError:
            pass
        else:
            raise AssertionError("malformed full-file check inventory admitted")
    require_profile(dict(ORACLE_PROFILE), isolated=True,
                    loaded_configs=["profile.ini"], expected_config="profile.ini")
    for key in ORACLE_PROFILE:
        changed = dict(ORACLE_PROFILE)
        changed[key] = not changed[key]
        try:
            require_profile(changed, isolated=True,
                            loaded_configs=["profile.ini"], expected_config="profile.ini")
        except ValueError:
            pass
        else:
            raise AssertionError("changed oracle policy admitted: " + key)
    for isolated, configs, values in (
            (False, ["profile.ini"], dict(ORACLE_PROFILE)),
            (True, ["ambient.ini", "profile.ini"], dict(ORACLE_PROFILE)),
            (True, ["profile.ini", "profile.ini"], dict(ORACLE_PROFILE)),
            (True, ["profile.ini"], dict(ORACLE_PROFILE, load_proxy_graphics=1))):
        try:
            require_profile(values, isolated=isolated,
                            loaded_configs=configs, expected_config="profile.ini")
        except ValueError:
            pass
        else:
            raise AssertionError("uncontrolled/ill-typed oracle configuration admitted")
    source = normalize({"format": "DXF", "version": "AC1032", "toolVersion": "test",
                        "exitStatus": 0, "diagnostics": [],
                        "entities": [{"handle": "A", "owner": "B", "type": "LINE", "fields": {"x": 1.25}}],
                        "objects": [{"handle": "B", "type": "BLOCK_RECORD"}],
                        "relationships": [{"from": "A", "kind": "owner", "to": "B"}],
                        "opaquePayloadHashes": []})
    target = json.loads(json.dumps(source).replace('"A"', '"C"').replace('"B"', '"D"'))
    a = mapped(source, {"A": "entity:0:LINE", "B": "layout:Model"})
    b = mapped(target, {"C": "entity:0:LINE", "D": "layout:Model"})
    assert not differences(a, b)
    b["entities"][0]["fields"]["x"] = 2.5
    assert differences(a, b) == ["projection.entities[0].fields.x"]
    b = mapped(target, {"C": "entity:0:LINE", "D": "layout:Paper"})
    assert differences(a, b), "owner identity must not be normalized away"
    assert differences([1, 2], [2, 1]), "ordered fields must remain ordered"
    assert differences(True, 1), "Boolean and integer must stay distinct"
    assert differences(2**60, 2**60 + 1), "large integer comparison must be exact"
    assert differences(1.0, math.nextafter(1.0, 2.0)), "native field changes must remain visible"
    assert not differences(231.25, math.nextafter(231.25, math.inf),
                           "projection.endAngleDegrees")
    assert differences(231.25, 231.2500001, "projection.endAngleDegrees")
    result = run_conversion([sys.executable, "-c", "print('bounded control')"], 5)
    assert result["exitStatus"] == 0 and result["stdout"] == "bounded control\n"
    try:
        run_conversion([sys.executable, "-c", "print('x' * 4096)"], 5, output_limit=128)
    except OutputLimitExceeded:
        pass
    else:
        raise AssertionError("unbounded converter logs were admitted")
    for timeout in (0, -1, float("nan"), float("inf"), 61):
        try:
            run_conversion([], timeout)
        except ValueError:
            pass
        else:
            raise AssertionError("invalid timeout admitted")
    with tempfile.TemporaryDirectory(prefix="libdxfrw-byte-admission-") as directory:
        path = Path(directory) / "control"
        for size in (127, 128, 129):
            path.write_bytes(b"x" * size)
            if size <= 128:
                assert drawing_bytes(path, 128) == b"x" * size
                assert digest(path) == byte_digest(b"x" * size)
            else:
                try:
                    drawing_bytes(path, 128)
                except ValueError:
                    pass
                else:
                    raise AssertionError("drawing byte admission exceeded its limit")
    assert list(bounded_tags(iter(range(3)), 3)) == [0, 1, 2]
    try:
        list(bounded_tags(iter(range(4)), 3))
    except ValueError:
        pass
    else:
        raise AssertionError("drawing tag admission exceeded its limit")
    print("independent DXF runner self-test: PASS (no external dependency)")


def runtime_self_test(ezdxf, config_path):
    from ezdxf.document import Drawing
    from ezdxf.lldxf.tagger import ascii_tags_loader
    from ezdxf.proxygraphic import ProxyGraphic
    before = effective_profile(ezdxf, config_path)
    try:
        importlib.import_module("ezdxf.acc.vector")
    except ModuleNotFoundError:
        pass
    else:
        raise AssertionError("direct native oracle import bypassed the profile")
    data = struct.pack("<2L", 72, 0) + struct.pack("<2L7d", 64, 2, 2, 3, 4, 5, 0, 0, 1)
    doc = ezdxf.new("R2013")
    doc.modelspace().add_mtext("option sentinel").proxy_graphic = data
    stream = io.StringIO()
    doc.write(stream)
    source = stream.getvalue().encode("utf-8")
    loaded = ordinary_load(ezdxf, Path("option-sentinel.dxf"), source)
    entity = next(iter(loaded.modelspace()))
    assert entity.proxy_graphic == data
    primitive, = ProxyGraphic(data, loaded).virtual_entities()
    assert primitive.dxftype() == "CIRCLE" and tuple(primitive.dxf.center) == (2, 3, 4)
    assert primitive.dxf.radius == 5
    for key, value in ORACLE_PROFILE.items():
        # Real pinned options, not a stand-in module. The use_c_ext mutation is
        # a state-guard control, not a native compiled-extension qualification.
        if key == "use_c_ext":
            ezdxf.options._use_c_ext = not value
        else:
            ezdxf.options.set("core", key, str(not value).lower())
        try:
            if key == "load_proxy_graphics":
                unguarded = Drawing.load(ascii_tags_loader(io.StringIO(source.decode("utf-8"))))
                assert next(iter(unguarded.modelspace())).proxy_graphic is None
            try:
                require_effective_profile(ezdxf)
            except ValueError:
                pass
            else:
                raise AssertionError("actual oracle option change admitted: " + key)
        finally:
            if key == "use_c_ext":
                ezdxf.options._use_c_ext = value
            else:
                ezdxf.options.set("core", key, str(value).lower())
    changes = differences(before, effective_profile(ezdxf, config_path))
    if changes:
        raise ValueError("runtime controls changed configuration: " + ", ".join(changes))
    controls = report_only_negative_controls(ezdxf)
    print("independent DXF runtime controls: PASS (proxy retention, six option mutations, "
          "%d report-only negatives; supplemental only)" % len(controls))
    return controls


def report_only_negative_controls(ezdxf):
    """Exercise known oracle gaps without turning them into support evidence."""
    from ezdxf.audit import Auditor
    from ezdxf.acis import api
    from ezdxf.disassemble import make_primitive, recursive_decompose
    from ezdxf.entities import Dictionary, VisualStyle
    from ezdxf.lldxf.tags import Tags
    from ezdxf.render import forms
    from ezdxf.lldxf.tagwriter import TagCollector
    from ezdxf.xclip import XClip

    required = ["sentinel", "envelope", "trailer", "versioned-header-fields"]
    prefix_missing = missing_required_checks(["minimum-prefix"], required)
    prefix = {
        "id": "prefix-only-parser-is-not-full-file-conformance",
        "execution": "not-run-source-review-only",
        "observedChecks": ["minimum-prefix"],
        "requiredFullFileChecks": required,
        "missingChecks": prefix_missing,
        "status": "excluded-from-full-file-conformance",
        "claimEligible": False,
        "qualificationEligible": False,
    }

    projection_doc = ezdxf.new("R2018")
    projection_msp = projection_doc.modelspace()
    projection_msp.add_line((0, 0, 0), (1, 0, 0))
    projection_msp.add_ray((0, 0, 0), (1, 0, 0))
    projection_msp.add_xline((0, 0, 0), (1, 0, 0))
    solid = projection_msp.add_3dsolid()
    api.export_dxf(solid, [api.body_from_mesh(forms.cube())])
    projection_stream = io.StringIO()
    projection_doc.write(projection_stream)
    projection_source = projection_stream.getvalue().encode("utf-8")
    projection_loaded = ordinary_load(ezdxf, Path("projection-negative.dxf"), projection_source)
    projected = {}
    for entity in projection_loaded.modelspace():
        primitive = make_primitive(entity)
        projected[entity.dxftype()] = {
            "empty": primitive.is_empty,
            "vertexCount": len(list(primitive.vertices())),
        }
    loaded_solid = next(entity for entity in projection_loaded.modelspace()
                        if entity.dxftype() == "3DSOLID")
    sab_bytes = len(loaded_solid.sab)
    if (projection_loaded.dxfversion != "AC1032" or sab_bytes <= 0
            or projected.get("LINE") != {"empty": False, "vertexCount": 2}
            or any(projected.get(kind) != {"empty": True, "vertexCount": 0}
                   for kind in ("RAY", "XLINE", "3DSOLID"))):
        raise ValueError("pinned oracle projection negative did not reproduce")
    projection = {
        "id": "unsupported-empty-primitives-are-not-zero-geometry",
        "execution": "pinned-ezdxf-1.4.4-in-memory-generated-dxf",
        "sourceVersion": projection_loaded.dxfversion,
        "sourceEntityCounts": {"LINE": 1, "RAY": 1, "XLINE": 1, "3DSOLID": 1},
        "primitiveProjection": projected,
        "solidSABBytes": sab_bytes,
        "status": "empty-unsupported-projection-not-measured-as-zero",
        "claimEligible": False,
        "qualificationEligible": False,
    }

    clip_doc = ezdxf.new("R2013")
    clip_msp = clip_doc.modelspace()
    clip_block = clip_doc.blocks.new("CLIPPED")
    clip_block.add_line((0, 0, 3), (1, 0, 3))
    clip_block.add_line((10, 0, 3), (11, 0, 3))
    insert = clip_msp.add_blockref("CLIPPED", (2, 4, 7))
    clip = XClip(insert)
    clip.set_block_clipping_path([(-1, -1), (2, -1), (2, 1), (-1, 1)])
    clip.enable_clipping()
    clip_stream = io.StringIO()
    clip_doc.write(clip_stream)
    clip_source = clip_stream.getvalue().encode("utf-8")
    clip_loaded = ordinary_load(ezdxf, Path("xclip-negative.dxf"), clip_source)
    loaded_insert = next(entity for entity in clip_loaded.modelspace()
                         if entity.dxftype() == "INSERT")
    loaded_clip = XClip(loaded_insert)
    decomposed = list(recursive_decompose([loaded_insert]))
    segments = sorted([
        [list(entity.dxf.start), list(entity.dxf.end)] for entity in decomposed
        if entity.dxftype() == "LINE"
    ])
    expected_segments = [
        [[2.0, 4.0, 10.0], [3.0, 4.0, 10.0]],
        [[12.0, 4.0, 10.0], [13.0, 4.0, 10.0]],
    ]
    if (clip_loaded.dxfversion != "AC1027" or not loaded_clip.has_clipping_path
            or not loaded_clip.is_clipping_enabled or segments != expected_segments):
        raise ValueError("pinned oracle XCLIP negative did not reproduce")
    clipping = {
        "id": "active-xclip-is-not-applied-by-recursive-decomposition",
        "execution": "pinned-ezdxf-1.4.4-in-memory-generated-dxf",
        "sourceVersion": clip_loaded.dxfversion,
        "clippingPathPresent": loaded_clip.has_clipping_path,
        "clippingEnabled": loaded_clip.is_clipping_enabled,
        "expectedVisibleLines": 1,
        "decomposedLineCount": len(segments),
        "outsideLinesRetained": 1,
        "decomposedLineEndpoints": segments,
        "status": "clipping-profile-mismatch",
        "claimEligible": False,
        "qualificationEligible": False,
    }

    def dxf_source(pairs):
        return "".join("%s\n%s\n" % pair for pair in pairs)

    def emitted_tags(entity):
        collector = TagCollector(dxfversion="AC1032")
        entity.export_dxf(collector)
        return [(tag.code, tag.value) for tag in collector.tags]

    style_prefix = [
        (0, "VISUALSTYLE"), (5, "A1"), (330, "B1"),
        (100, "AcDbVisualStyle"), (2, "LOCAL_STYLE"), (70, 8),
        (73, 6), (421, 16777215), (291, 0),
    ]
    style_tail = [
        (70, 2), (62, 7), (420, 16777215), (176, 2),
        (90, 123456), (176, 1),
    ]
    style_rows = []
    style_cases = (
        ("wrong-declared-property-count", [(70, 3)] + style_tail[1:], 3, 2, 1),
        ("missing-final-property-companion", style_tail[:-1], 2, 2, 1),
    )
    for control_id, tail, declared_count, authored_count, companion_count in style_cases:
        style = VisualStyle.from_text(dxf_source(style_prefix + tail))
        emitted = emitted_tags(style)
        emitted_tail = emitted[emitted.index((291, 0)) + 1:]
        if emitted_tail != tail:
            raise ValueError("pinned oracle VISUALSTYLE opaque-tail control changed")
        style_rows.append({
            "id": control_id,
            "execution": "pinned-ezdxf-1.4.4-in-memory-generated-dxf",
            "declaredPropertyCount": declared_count,
            "authoredPropertyCount": authored_count,
            "group176CompanionCount": companion_count,
            "opaqueTailPreserved": True,
            "status": "opaque-tail-preserved-without-grammar-validation",
            "claimEligible": False,
            "qualificationEligible": False,
        })

    dictionary_prefix = [
        (0, "DICTIONARY"), (5, "B1"), (330, "0"),
        (100, "AcDbDictionary"), (280, 0), (281, 1),
    ]
    dictionary_rows = []
    dictionary_cases = (
        ("mixed-soft-then-hard-edge-codes",
         [(3, "A"), (350, "C1"), (3, "B"), (360, "C2")], 360,
         [(360, "C1"), (360, "C2")]),
        ("mixed-hard-then-soft-edge-codes",
         [(3, "A"), (360, "C1"), (3, "B"), (350, "C2")], 350,
         [(350, "C1"), (350, "C2")]),
        ("duplicate-dictionary-key",
         [(3, "A"), (350, "C1"), (3, "A"), (350, "C2")], 350,
         [(350, "C2")]),
    )
    for control_id, entries, normalized_code, expected_edges in dictionary_cases:
        dictionary = Dictionary.from_text(dxf_source(dictionary_prefix + entries))
        emitted = emitted_tags(dictionary)
        edges = [(code, value) for code, value in emitted if code in (350, 360)]
        if edges != expected_edges:
            raise ValueError("pinned oracle DICTIONARY negative control changed")
        dictionary_rows.append({
            "id": control_id,
            "execution": "pinned-ezdxf-1.4.4-in-memory-generated-dxf",
            "normalizedEdgeCode": normalized_code,
            "emittedEdges": edges,
            "status": "diagnostic-robustness-only-not-valid-format-policy",
            "claimEligible": False,
            "qualificationEligible": False,
        })

    dangling_doc = ezdxf.new("R2013")
    dangling_dictionary = dangling_doc.rootdict.add_new_dict("LOCAL_DANGLING")
    dangling_dictionary.load_dict(
        Tags.from_text(dxf_source([(3, "MISSING"), (350, "FFFF")])))
    dangling_before = list(dangling_dictionary.keys())
    auditor = Auditor(dangling_doc)
    dangling_dictionary.audit(auditor)
    dangling_after = list(dangling_dictionary.keys())
    if (dangling_before != ["MISSING"] or dangling_after
            or auditor.errors or not auditor.fixes):
        raise ValueError("pinned oracle dangling-DICTIONARY audit control changed")
    dangling = {
        "id": "audit-prunes-dangling-dictionary-target",
        "execution": "pinned-ezdxf-1.4.4-in-memory-generated-dxf",
        "entriesBeforeAudit": dangling_before,
        "entriesAfterAudit": dangling_after,
        "auditErrorCount": len(auditor.errors),
        "auditFixCount": len(auditor.fixes),
        "status": "repair-prunes-target-zero-errors-is-not-lossless-closure",
        "claimEligible": False,
        "qualificationEligible": False,
    }
    synthesized_defaults = {
        "id": "acadrust-visualstyle-defaults-are-not-source-fidelity",
        "execution": "not-run-source-review-only",
        "modelBlob": "ac987d3",
        "writerRevision": "a1a216d642d09c7548cc26ac6d3db6f79bd5a667",
        "observedLimitation": "model-synthesizes-missing-properties-and-normalizes-legacy-magnitudes",
        "status": "defaulted-projection-not-source-fidelity",
        "claimEligible": False,
        "qualificationEligible": False,
    }
    family_omission = {
        "id": "acadsharp-visualstyle-writer-family-omission",
        "execution": "not-run-source-review-only",
        "peer": "ACadSharp",
        "sourceRevision": "9bd47f48766a371c441e8d1ab80733e21f62b711",
        "reviewedMethod": "DxfObjectsSectionWriter.isObjectSupported",
        "family": "VISUALSTYLE",
        "observedDisposition": "explicitly-skipped-as-unsupported",
        "status": "family-omission-not-a-peer-writer-vote",
        "claimEligible": False,
        "qualificationEligible": False,
    }
    additional_family_omissions = [
        {
            "id": "acadsharp-material-writer-family-omission",
            "execution": "not-run-source-review-only",
            "peer": "ACadSharp",
            "sourceRevision": "9bd47f48766a371c441e8d1ab80733e21f62b711",
            "reviewedMethod": "DxfObjectsSectionWriter.isObjectSupported",
            "family": "MATERIAL",
            "observedDisposition": "explicitly-skipped-as-unsupported",
            "status": "family-omission-not-a-peer-writer-vote",
            "claimEligible": False,
            "qualificationEligible": False,
        },
        {
            "id": "acadsharp-dimension-association-writer-family-omission",
            "execution": "not-run-source-review-only",
            "peer": "ACadSharp",
            "sourceRevision": "9bd47f48766a371c441e8d1ab80733e21f62b711",
            "reviewedMethod": "DxfObjectsSectionWriter.isObjectSupported",
            "family": "DIMENSIONASSOCIATION",
            "observedDisposition": "explicitly-skipped-as-unsupported",
            "status": "family-omission-not-a-peer-writer-vote",
            "claimEligible": False,
            "qualificationEligible": False,
        },
    ]
    return ([prefix, projection, clipping] + style_rows + dictionary_rows
            + [dangling, synthesized_defaults, family_omission]
            + additional_family_omissions)


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--self-test", action="store_true")
    mode.add_argument("--check-runtime", action="store_true")
    mode.add_argument("--runtime-self-test", action="store_true")
    parser.add_argument("--converter", type=Path)
    parser.add_argument("--family", choices=FAMILIES, action="append")
    parser.add_argument("--output", type=Path)
    parser.add_argument("--timeout", type=float, default=15.0)
    args = parser.parse_args(argv)
    if sys.flags.optimize:
        parser.error("oracle controls require Python without -O or -OO")
    if args.self_test:
        self_test()
        return 0
    if not args.converter and not (args.check_runtime or args.runtime_self_test):
        parser.error("--converter is required")
    if not math.isfinite(args.timeout) or not 0 < args.timeout <= 60:
        parser.error("timeout must be finite and in (0, 60]")
    if args.family and len(set(args.family)) != len(args.family):
        parser.error("duplicate family would duplicate the evidence denominator")
    try:
        converter = args.converter.resolve(strict=True) if args.converter else None
        output_path = args.output.resolve() if args.output else None
        runtime_only = args.check_runtime or args.runtime_self_test
        with controlled_runtime() as (ezdxf, config_path, _):
            context = oracle_context(ezdxf, config_path)
            report_only_controls = []
            if args.runtime_self_test:
                report_only_controls = runtime_self_test(ezdxf, config_path)
            if not runtime_only:
                converter_before = digest(converter)
                with tempfile.TemporaryDirectory(prefix="libdxfrw-independent-dxf-") as directory:
                    records = [run_case(ezdxf, converter, Path(directory), family, incoming, outgoing, args.timeout)
                               for family in (args.family or FAMILIES)
                               for incoming in (False, True) for outgoing in (False, True)]
            # Rebind the complete on-disk inventory, not just options. This
            # covers the package, interpreter and trusted helpers; dependency
            # distribution versions remain explicitly metadata-only.
            require_context_unchanged(context, oracle_context(ezdxf, config_path))
        if runtime_only:
            payload = {"schema": 2, "tool": "libdxfrw-independent-dxf-runtime",
                       "qualificationEligible": False, "oracleContext": context,
                       "oracleConfigurationUnchanged": True,
                       "oracleContentBeforeAfterUnchanged": True,
                       "runtimeControlsExecuted": bool(args.runtime_self_test),
                       "reportOnlyNegativeControls": report_only_controls}
            if output_path:
                output_path.write_text(json.dumps(payload, sort_keys=True, indent=2,
                                                  allow_nan=False) + "\n", encoding="utf-8")
            if args.check_runtime:
                print("independent DXF runtime: PASS (ezdxf %s, %s)" % (PINNED_VERSION, PROFILE_NAME))
            return 0
        payload = {"schema": 2, "tool": "libdxfrw-independent-dxf",
                   "oracleVersion": ezdxf.__version__, "pythonVersion": sys.version.split()[0],
                   "converterDigest": converter_before, "converterAfterDigest": digest(converter),
                   "qualificationEligible": False,
                   "oracleContext": context,
                   "oracleConfigurationUnchanged": True,
                   "oracleContentBeforeAfterUnchanged": True,
                   "oracleBootstrap": "fresh -I interpreter; private cwd/config; pure Python before import",
                   "nativeFieldComparison": "exact JSON type/value except four-ULP degree-angle handoff",
                   "logLimitBytesPerStream": 65536,
                   "drawingByteLimit": MAX_DRAWING_BYTES, "drawingTagLimit": MAX_DRAWING_TAGS,
                   "attestedFileByteLimit": MAX_ATTESTED_FILE_BYTES,
                   "ordinaryReader": "pinned Drawing.load on immutable bounded tag snapshots",
                   "oracleCpuIsolation": "not enabled; locally generated bounded sentinels only",
                   "recoveryAttempted": False, "records": records}
        payload["converterBeforeAfterUnchanged"] = converter_before == payload["converterAfterDigest"]
        encoded = json.dumps(payload, sort_keys=True, indent=2, allow_nan=False) + "\n"
        if output_path:
            output_path.write_text(encoded, encoding="utf-8")
            matches = sum(item["status"] == "scope-match" for item in records)
            print("independent DXF runner: %d/%d scope matches; supplemental only; %s" % (
                matches, len(records), output_path))
        else:
            print(encoded, end="")
        return 0 if (payload["converterBeforeAfterUnchanged"]
                     and all(item["status"] == "scope-match" for item in records)) else 1
    except (ImportError, OSError, ValueError, KeyError, configparser.Error) as exc:
        print("independent DXF runner: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
