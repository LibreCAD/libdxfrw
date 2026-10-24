#!/usr/bin/env python3
"""Optional, source-only isolation controls for the pinned independent oracle.

Only runtime-only oracle processes are launched: this wrapper never invokes a
converter or qualifies DWG/DXF support. All hostile host inputs are generated
in an automatically cleaned scratch directory, not installed on the host.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import shutil

# -I excludes the script directory. Load only the committed, trusted helper;
# neither the working directory nor PYTHONPATH is a helper import authority.
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from bounded_process import run_bounded


EXPECTED_OPTIONS = {
    "load_proxy_graphics": True,
    "store_proxy_graphics": True,
    "filter_invalid_xdata_group_codes": False,
    "write_fixed_meta_data_for_testing": True,
    "disable_c_ext": True,
    "use_c_ext": False,
}
TIMEOUT_SECONDS = 30
MAX_REPORT_BYTES = 1024 * 1024


def require(condition, message):
    if not condition:
        raise ValueError(message)


def snapshot(paths):
    return {str(path): (path.read_bytes(), path.stat().st_mtime_ns,
                        path.stat().st_mode) for path in paths}


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        require(key not in result, "duplicate runtime report key: " + key)
        result[key] = value
    return result


def invalid_constant(value):
    raise ValueError("nonfinite runtime report constant: " + value)


def read_report(path, scratch, *, controls_executed=False):
    with path.open("rb") as source:
        data = source.read(MAX_REPORT_BYTES + 1)
    require(len(data) <= MAX_REPORT_BYTES, "runtime report exceeds byte budget")
    report = json.loads(data.decode("utf-8"), object_pairs_hook=unique_object,
                        parse_constant=invalid_constant)
    require(type(report) is dict and type(report.get("schema")) is int
            and report["schema"] == 2, "runtime report must use schema 2")
    require(report.get("qualificationEligible") is False,
            "runtime controls must remain nonqualifying")
    require(report.get("oracleConfigurationUnchanged") is True
            and report.get("oracleContentBeforeAfterUnchanged") is True
            and report.get("runtimeControlsExecuted") is controls_executed,
            "runtime report did not bind completed controls/content stability")
    negative_controls = report.get("reportOnlyNegativeControls")
    if controls_executed:
        require(type(negative_controls) is list and len(negative_controls) == 13,
                "runtime report omitted a report-only negative control")
        require(all(type(item) is dict for item in negative_controls),
                "runtime negative controls must be JSON objects")
        expected_ids = [
            "prefix-only-parser-is-not-full-file-conformance",
            "unsupported-empty-primitives-are-not-zero-geometry",
            "active-xclip-is-not-applied-by-recursive-decomposition",
            "wrong-declared-property-count",
            "missing-final-property-companion",
            "mixed-soft-then-hard-edge-codes",
            "mixed-hard-then-soft-edge-codes",
            "duplicate-dictionary-key",
            "audit-prunes-dangling-dictionary-target",
            "acadrust-visualstyle-defaults-are-not-source-fidelity",
            "acadsharp-visualstyle-writer-family-omission",
            "acadsharp-material-writer-family-omission",
            "acadsharp-dimension-association-writer-family-omission",
        ]
        require([item.get("id") for item in negative_controls] == expected_ids,
                "runtime negative-control identities or order changed")
        require(all(item.get("claimEligible") is False
                    and item.get("qualificationEligible") is False
                    for item in negative_controls),
                "negative controls must never promote claims or qualification")
        prefix, projection, clipping = negative_controls[:3]
        dangling = negative_controls[8]
        required_checks = ["sentinel", "envelope", "trailer", "versioned-header-fields"]
        require(prefix.get("execution") == "not-run-source-review-only"
                and prefix.get("observedChecks") == ["minimum-prefix"]
                and prefix.get("requiredFullFileChecks") == required_checks
                and prefix.get("missingChecks") == required_checks
                and prefix.get("status") == "excluded-from-full-file-conformance",
                "prefix-only peer source review was misrepresented as an execution")
        primitives = projection.get("primitiveProjection")
        source_counts = projection.get("sourceEntityCounts")
        require(type(source_counts) is dict
                and set(source_counts) == {"LINE", "RAY", "XLINE", "3DSOLID"}
                and all(type(count) is int for count in source_counts.values()),
                "projection source counts are not strictly typed integers")
        def valid_primitive(kind, *, empty, vertex_count):
            observed = primitives.get(kind) if type(primitives) is dict else None
            return (type(observed) is dict and set(observed) == {"empty", "vertexCount"}
                    and type(observed["empty"]) is bool
                    and observed["empty"] is empty
                    and type(observed["vertexCount"]) is int
                    and observed["vertexCount"] == vertex_count)
        require(projection.get("execution") == "pinned-ezdxf-1.4.4-in-memory-generated-dxf"
                and projection.get("sourceVersion") == "AC1032"
                and source_counts == {"LINE": 1, "RAY": 1, "XLINE": 1, "3DSOLID": 1}
                and valid_primitive("LINE", empty=False, vertex_count=2)
                and all(valid_primitive(kind, empty=True, vertex_count=0)
                        for kind in ("RAY", "XLINE", "3DSOLID"))
                and type(projection.get("solidSABBytes")) is int
                and projection["solidSABBytes"] > 0
                and projection.get("status") == "empty-unsupported-projection-not-measured-as-zero",
                "unsupported empty projections were not distinguished from absent geometry")
        expected_segments = [
            [[2.0, 4.0, 10.0], [3.0, 4.0, 10.0]],
            [[12.0, 4.0, 10.0], [13.0, 4.0, 10.0]],
        ]
        endpoints = clipping.get("decomposedLineEndpoints")
        require(type(endpoints) is list and len(endpoints) == 2
                and all(type(segment) is list and len(segment) == 2
                        and all(type(point) is list and len(point) == 3
                                and all(type(coordinate) is float
                                        for coordinate in point)
                                for point in segment)
                        for segment in endpoints),
                "clipping projection endpoints have an unexpected JSON shape/type")
        require(clipping.get("execution") == "pinned-ezdxf-1.4.4-in-memory-generated-dxf"
                and clipping.get("sourceVersion") == "AC1027"
                and clipping.get("clippingPathPresent") is True
                and clipping.get("clippingEnabled") is True
                and type(clipping.get("expectedVisibleLines")) is int
                and clipping.get("expectedVisibleLines") == 1
                and type(clipping.get("decomposedLineCount")) is int
                and clipping.get("decomposedLineCount") == 2
                and type(clipping.get("outsideLinesRetained")) is int
                and clipping.get("outsideLinesRetained") == 1
                and endpoints == expected_segments
                and clipping.get("status") == "clipping-profile-mismatch",
                "active XCLIP negative did not reproduce its outside-line mismatch")
        style_controls = negative_controls[3:5]
        expected_style = [
            (3, 2, 1),  # declared, authored property count, group-176 companions
            (2, 2, 1),
        ]
        require(all(item.get("execution") == "pinned-ezdxf-1.4.4-in-memory-generated-dxf"
                    and type(item.get("declaredPropertyCount")) is int
                    and type(item.get("authoredPropertyCount")) is int
                    and type(item.get("group176CompanionCount")) is int
                    and item.get("opaqueTailPreserved") is True
                    and item.get("status") == "opaque-tail-preserved-without-grammar-validation"
                    for item in style_controls)
                and [(item["declaredPropertyCount"], item["authoredPropertyCount"],
                      item["group176CompanionCount"]) for item in style_controls]
                    == expected_style,
                "VISUALSTYLE count/companion negatives no longer preserve the observed raw tail")
        dictionary_controls = negative_controls[5:8]
        expected_dictionary = [
            (360, [[360, "C1"], [360, "C2"]]),
            (350, [[350, "C1"], [350, "C2"]]),
            (350, [[350, "C2"]]),
        ]
        require(all(item.get("execution") == "pinned-ezdxf-1.4.4-in-memory-generated-dxf"
                    and type(item.get("normalizedEdgeCode")) is int
                    and item.get("status") == "diagnostic-robustness-only-not-valid-format-policy"
                    for item in dictionary_controls)
                and [(item["normalizedEdgeCode"], item.get("emittedEdges"))
                     for item in dictionary_controls] == expected_dictionary,
                "DICTIONARY normalization/duplicate-key controls changed")
        require(dangling.get("execution") == "pinned-ezdxf-1.4.4-in-memory-generated-dxf"
                and dangling.get("entriesBeforeAudit") == ["MISSING"]
                and dangling.get("entriesAfterAudit") == []
                and type(dangling.get("auditErrorCount")) is int
                and dangling["auditErrorCount"] == 0
                and type(dangling.get("auditFixCount")) is int
                and dangling["auditFixCount"] > 0
                and dangling.get("status") == "repair-prunes-target-zero-errors-is-not-lossless-closure",
                "dangling DICTIONARY audit pruning was misrepresented")
        synthesized_defaults = negative_controls[9]
        require(synthesized_defaults.get("execution") == "not-run-source-review-only"
                and synthesized_defaults.get("modelBlob") == "ac987d3"
                and synthesized_defaults.get("writerRevision")
                    == "a1a216d642d09c7548cc26ac6d3db6f79bd5a667"
                and synthesized_defaults.get("observedLimitation")
                    == "model-synthesizes-missing-properties-and-normalizes-legacy-magnitudes"
                and synthesized_defaults.get("status")
                    == "defaulted-projection-not-source-fidelity",
                "ACadRust source-only defaulting limitation was misrepresented as execution")
        family_omissions = negative_controls[10:]
        expected_family_omissions = [
            ("acadsharp-visualstyle-writer-family-omission", "VISUALSTYLE"),
            ("acadsharp-material-writer-family-omission", "MATERIAL"),
            ("acadsharp-dimension-association-writer-family-omission",
             "DIMENSIONASSOCIATION"),
        ]
        require([(item.get("id"), item.get("family")) for item in family_omissions]
                    == expected_family_omissions
                and all(item.get("execution") == "not-run-source-review-only"
                        and item.get("peer") == "ACadSharp"
                        and item.get("sourceRevision")
                            == "9bd47f48766a371c441e8d1ab80733e21f62b711"
                        and item.get("reviewedMethod")
                            == "DxfObjectsSectionWriter.isObjectSupported"
                        and item.get("observedDisposition")
                            == "explicitly-skipped-as-unsupported"
                        and item.get("status")
                            == "family-omission-not-a-peer-writer-vote"
                        for item in family_omissions),
                "ACadSharp family-specific source omissions were misrepresented as execution")
    else:
        require(negative_controls == [],
                "runtime-only check unexpectedly claims negative controls executed")
    context = report["oracleContext"]
    require(context["version"] == "1.4.4", "oracle release is not pinned")
    configuration = context["configuration"]
    require(configuration["name"] == "generated-ezdxf-1.4.4-pure-python-v1",
            "unexpected oracle profile")
    options = configuration["effectiveOptions"]
    require(type(options) is dict and set(options) == set(EXPECTED_OPTIONS)
            and all(options[key] is value for key, value in EXPECTED_OPTIONS.items()),
            "controlled oracle options differ in type or value")
    full_options = configuration["fullOptions"]
    require(type(full_options) is dict, "full options are not an object")
    encoded = json.dumps(full_options, sort_keys=True, separators=(",", ":"),
                         allow_nan=False).encode("utf-8")
    require(configuration["fullOptionsSha256"] == hashlib.sha256(encoded).hexdigest(),
            "full options digest does not attest the reported options")
    require(full_options["core"]["test_files"] == "",
            "host TEST_FILES escaped configuration isolation")
    require(configuration["pythonIsolated"] is True,
            "runtime report was not produced by an isolated Python")
    require(type(configuration["pythonOptimize"]) is int
            and configuration["pythonOptimize"] == 0,
            "runtime controls ran with assertions disabled")
    bytecode = configuration["bytecode"]
    cache_prefix = Path(bytecode["prefix"])
    require(bytecode["cacheWrites"] is False
            and bytecode["cacheReads"] == "fresh private prefix only"
            and cache_prefix.is_absolute()
            and cache_prefix.name.startswith("libdxfrw-oracle-bytecode-")
            and not cache_prefix.is_relative_to(scratch),
            "runtime could use an existing source-tree bytecode cache")
    loaded = configuration["loadedConfigs"]
    require(type(loaded) is list and len(loaded) == 1,
            "oracle loaded more than its private profile")
    profile = Path(loaded[0]["path"])
    require(profile.is_absolute() and profile.name == "profile.ini"
            and profile.parent.name.startswith("libdxfrw-oracle-profile-")
            and not profile.is_relative_to(scratch),
            "configuration origin is not the runner's private profile")
    require(type(loaded[0]["size"]) is int and loaded[0]["size"] > 0
            and len(loaded[0]["sha256"]) == 64,
            "private configuration origin is not content-attested")
    accelerator = configuration["accelerator"]
    require(accelerator["inUse"] is False
            and accelerator["requested"] == "disabled-before-import"
            and accelerator["directNativeImports"] == "blocked for ezdxf.acc.*"
            and accelerator["vectorImplementation"] == "ezdxf.math._vector"
            and accelerator["tagImplementation"] == "ezdxf.lldxf.types",
            "oracle used a native or unexpected implementation path")
    package = Path(context["packageOrigin"])
    bootstrap = Path(accelerator["bootstrapModule"]["path"])
    require(package.is_absolute() and not package.is_relative_to(scratch)
            and bootstrap.is_relative_to(package) and bootstrap.suffix == ".py",
            "oracle package or accelerator bootstrap came from a shadow/native path")
    return configuration


def invoke(python, runner, arguments, *, cwd, env, isolated=True, optimized=False):
    command = [str(python)] + (["-I"] if isolated else [])
    if optimized:
        command.append("-O")
    command += ["-B", str(runner)] + arguments
    return run_bounded(command, TIMEOUT_SECONDS, output_limit=65536,
                       cwd=str(cwd), env=env)


def require_success(result, label):
    require(result.returncode == 0, "%s failed (%s): %s\n%s" % (
        label, result.returncode, result.stdout.decode("utf-8", "replace"),
        result.stderr.decode("utf-8", "replace")))


def run_controls(python, runner):
    original_environment = dict(os.environ)
    original_directory = Path.cwd()
    try:
        with tempfile.TemporaryDirectory(prefix="libdxfrw-runtime-host-") as directory:
            scratch = Path(directory).resolve()
            host = scratch / "host config"
            cwd = host / "cwd"
            xdg = host / "xdg" / "ezdxf"
            shadow = host / "shadow"
            for path in (cwd, xdg, shadow):
                path.mkdir(parents=True)
            (cwd / "ezdxf.ini").write_text("[malformed cwd\n", encoding="utf-8")
            (xdg / "ezdxf.ini").write_text("not an ini section\n", encoding="utf-8")
            marker = host / "SHADOW_IMPORTED"
            (shadow / "ezdxf.py").write_text(
                "from pathlib import Path\nPath(%r).write_text('unexpected import')\n"
                "raise RuntimeError('host shadow ezdxf imported')\n" % str(marker),
                encoding="utf-8")
            profiles = []
            for name, value in (("on", "true"), ("off", "false")):
                path = host / (name + ".ini")
                lines = ["[core]"] + [key + " = " + value
                    for key in EXPECTED_OPTIONS if key != "use_c_ext"]
                lines.append("TEST_FILES = host-ini-sentinel-" + name)
                path.write_text("\n".join(lines) + "\n", encoding="utf-8")
                profiles.append(path)
            host_paths = tuple(path for path in host.rglob("*") if path.is_file())
            host_before = snapshot(host_paths)
            host_tree_before = sorted(str(path.relative_to(host)) for path in host.rglob("*"))

            def host_environment(profile):
                environment = dict(original_environment)
                environment.update(EZDXF_CONFIG_FILE=str(profile),
                    XDG_CONFIG_HOME=str(xdg.parent), EZDXF_DISABLE_C_EXT="false",
                    EZDXF_TEST_FILES="host-env-test-files-sentinel",
                    TEST_FILES="host-generic-test-files-sentinel", PYTHONPATH=str(shadow))
                return environment

            environment = host_environment(profiles[0])
            self_report = scratch / "runtime-controls.json"
            result = invoke(python, runner, ["--runtime-self-test", "--output", str(self_report)],
                            cwd=cwd, env=environment)
            require_success(result, "actual runtime option/proxy controls")
            require(b"independent DXF runtime controls: PASS" in result.stdout,
                    "runtime self-test did not execute its controls")
            read_report(self_report, scratch, controls_executed=True)
            configurations = []
            for index, profile in enumerate(profiles):
                environment = host_environment(profile)
                environment_before = dict(environment)
                report = scratch / ("runtime-%d.json" % index)
                result = invoke(python, runner, ["--check-runtime", "--output", str(report)],
                                cwd=cwd, env=environment)
                require_success(result, "host profile " + profile.stem)
                configurations.append(read_report(report, scratch))
                require(environment == environment_before,
                        "bounded invocation changed its caller's environment mapping")
            left, right = configurations
            for key in ("effectiveOptions", "fullOptions", "fullOptionsSha256"):
                require(left[key] == right[key], "host on/off profiles changed " + key)
            require(left["loadedConfigs"][0]["sha256"] == right["loadedConfigs"][0]["sha256"],
                    "private profile contents changed between fresh invocations")
            rejected_report = scratch / "nonisolated.json"
            result = invoke(python, runner, ["--check-runtime", "--output", str(rejected_report)],
                            cwd=cwd, env=environment, isolated=False)
            require(result.returncode != 0
                    and b"invoke Python with -I" in result.stderr
                    and not rejected_report.exists(),
                    "non-isolated runtime check was not explicitly rejected before output")
            optimized_report = scratch / "optimized.json"
            for arguments in (["--check-runtime", "--output", str(optimized_report)],
                              ["--runtime-self-test"], ["--self-test"]):
                result = invoke(python, runner, arguments, cwd=cwd, env=environment,
                                optimized=True)
                require(result.returncode != 0
                        and b"without -O or -OO" in result.stderr
                        and not optimized_report.exists(),
                        "optimized Python silently skipped assertion-based controls")
            # A valid timestamp/size cache contains different executable code
            # from the intact helper source. Prove it is selected normally,
            # then prove our bootstrap ignores it without modifying any real
            # repository or installed cache.
            copied = scratch / "copied tools"
            copied.mkdir()
            for name in (runner.name, "bounded_process.py", "normalize_drawing_result.py"):
                shutil.copy2(runner.with_name(name), copied / name)
            helper = copied / "bounded_process.py"
            original_size = helper.stat().st_size
            bad = scratch / "cache-payload.py"
            payload = b"raise RuntimeError('stale-bytecode-sentinel')\n"
            require(len(payload) < original_size, "helper is too small for cache control")
            bad.write_bytes(payload + b" " * (original_size - len(payload)))
            os.utime(bad, ns=(helper.stat().st_atime_ns, helper.stat().st_mtime_ns))
            # Build with the selected oracle interpreter: its cache tag/magic
            # may differ from the normal test interpreter running this wrapper.
            built_cache = run_bounded([str(python), "-I", "-B", "-c",
                "import importlib.util,json,py_compile,sys; from pathlib import Path; "
                "p=Path(importlib.util.cache_from_source(sys.argv[2])); p.parent.mkdir(); "
                "py_compile.compile(sys.argv[1],cfile=str(p),dfile=sys.argv[2],doraise=True); "
                "print(json.dumps(str(p)))", str(bad), str(helper)],
                TIMEOUT_SECONDS, output_limit=65536, cwd=str(cwd), env=environment)
            require_success(built_cache, "matching-interpreter bytecode control")
            cache = Path(json.loads(built_cache.stdout.decode("utf-8")))
            require(cache.is_file() and cache.is_relative_to(copied),
                    "bytecode control escaped its generated helper directory")
            ordinary = run_bounded([str(python), "-I", "-B", "-c",
                "import sys; sys.path.insert(0, %r); import bounded_process" % str(copied)],
                TIMEOUT_SECONDS, output_limit=65536, cwd=str(cwd), env=environment)
            require(ordinary.returncode != 0
                    and b"stale-bytecode-sentinel" in ordinary.stderr,
                    "negative bytecode cache was not actually selected")
            cache_before = snapshot((helper, cache))
            cache_report = scratch / "private-cache.json"
            result = invoke(python, copied / runner.name,
                ["--check-runtime", "--output", str(cache_report)], cwd=cwd, env=environment)
            require_success(result, "private bytecode cache bootstrap")
            read_report(cache_report, scratch)
            require(snapshot((helper, cache)) == cache_before,
                    "oracle bootstrap changed the existing helper source/cache")
            require(snapshot(host_paths) == host_before and not marker.exists()
                    and sorted(str(path.relative_to(host)) for path in host.rglob("*"))
                    == host_tree_before, "runtime checks modified host files or imported the shadow")
    finally:
        require(dict(os.environ) == original_environment,
                "runtime wrapper modified the host environment")
        require(Path.cwd() == original_directory, "runtime wrapper modified the host cwd")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--oracle-python", required=True, type=Path)
    parser.add_argument("--runner", required=True, type=Path)
    args = parser.parse_args(argv)
    try:
        # Preserve a venv executable's symlink: resolving it can lose the venv.
        python = args.oracle_python.absolute()
        runner = args.runner.resolve(strict=True)
        require(python.is_file() and runner.is_file(), "oracle Python or runner is not a file")
        run_controls(python, runner)
    except (OSError, ValueError, KeyError, TypeError, subprocess.SubprocessError) as exc:
        print("independent DXF runtime isolation: FAIL: %s" % exc, file=sys.stderr)
        return 1
    print("independent DXF runtime isolation: PASS (host on/off, malformed configs, "
          "shadow import, native disable, non-isolated/optimized rejection, "
          "stale bytecode ignored; supplemental only)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
