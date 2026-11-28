#!/usr/bin/env python3
"""Build pinned-target and standalone adapters or inspect a legacy build.

The target build is deliberately independent of LibreCAD's top-level CMake
project.  It archives exactly the source paths and commit named by the lock,
checks the archive and manifest identities before extraction, and builds the
one shared adapter source against that extracted source closure.  The
standalone qualification route snapshots its source closure and uses a fresh,
controlled CMake build tree; receipt-standalone remains a legacy inspection
path that cannot attest an earlier process environment.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import ntpath
import os
import re
import shlex
import shutil
import subprocess
import sys
import tarfile
import tempfile
from pathlib import Path, PurePosixPath


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_LOCK = REPOSITORY_ROOT / "metadata/libdxfrw-target-lock.json"
DEFAULT_MANIFEST = REPOSITORY_ROOT / "metadata/libdxfrw-target-source-manifest.txt"
DEFAULT_ADAPTER = REPOSITORY_ROOT / "tests/semantic_differential_adapter.cpp"
DEFAULT_WRAPPER = REPOSITORY_ROOT / "cmake/semantic_adapter_target"
DEFAULT_SEMANTIC_MANIFEST = (
    REPOSITORY_ROOT / "metadata/qualified-differential-v2.json"
)
TARGET_EXECUTABLE = "libdxfrw_semantic_adapter_target"
STANDALONE_EXECUTABLE = "libdxfrw_semantic_adapter_standalone"
INTERFACE_PROFILE_NAME = "common170-standalone176-callbacks-v2"
CAPABILITY_PROFILE_NAME = "semantic-adapter-capabilities-v1"
CAPABILITY_PROFILE_KEYS = {"schema", "name", "side", "capabilities"}
CAPABILITY_NAMES = (
    "dwgReadObservations",
    "modelerDwgSourceVersion",
    "modelerDwgAcisPayload",
    "modelerDxfPayloadChunks",
    "modelerPayloadStatusName",
    "payloadRangeBitOffset",
    "surfaceDxfPayloadChunks",
)
EXPECTED_CAPABILITIES = {
    "target": {name: False for name in CAPABILITY_NAMES},
    "standalone": {name: True for name in CAPABILITY_NAMES},
}
COMMON_INTERFACE_METHOD_COUNT = 170
COMMON_INTERFACE_METHOD_DIGEST = (
    "894c58db480ef3f5c7e25a9066ca9a178ecf5e41137a9e709a1144f340faba7a"
)
SUPPLEMENTAL_INTERFACE_METHODS = (
    "addBlockEnd",
    "addBlockRecord",
    "addDwgTemplateDescription",
    "addNamedObjectDictionary",
    "addProxySubentityMarker",
    "finalizeDxfWrite",
)
# Ownership-transfer callbacks are additive overloads whose base
# implementation forwards to the legacy const-reference callback. The
# semantic adapter intentionally overrides only the const-reference forms;
# keep the exact overload set source-checked so a new overload cannot vanish
# behind the name-only callback inventory.
KNOWN_INTERFACE_OVERLOADS = {
    "addDataStorage": frozenset({
        "const DRW_DataStorageSection&",
        "DRW_DataStorageSection&&",
    }),
    "addModelerGeometry": frozenset({
        "const DRW_ModelerGeometry&",
        "DRW_ModelerGeometry&&",
    }),
    "addUnsupportedObject": frozenset({
        "const DRW_UnsupportedObject&",
        "DRW_UnsupportedObject&&",
    }),
}
TARGET_LIBRARY = "libdxfrw_semantic_target"
ARCHIVE_PATHS = (
    "libraries/libdxfrw/src",
    "libraries/libdxfrw/libdxfrw_sources.cmake",
)
ARCHIVE_PREFIX = "libdxfrw/"
HEX40 = re.compile(r"[0-9a-f]{40}\Z")
HEX64 = re.compile(r"[0-9a-f]{64}\Z")
MAX_ARCHIVE_MEMBERS = 4096
MAX_ARCHIVE_BYTES = 512 * 1024 * 1024
MAX_COMPILER_RESOURCE_ENTRIES = 100_000
MAX_COMPILER_RESOURCE_BYTES = 2 * 1024 * 1024 * 1024
MAX_APPLE_SDK_METADATA_BYTES = 32 * 1024 * 1024
APPLE_SDK_METADATA_NAMES = ("SDKSettings.json", "SDKSettings.plist")
MAX_APPLE_LINK_MAP_BYTES = 64 * 1024 * 1024
MAX_APPLE_LINK_MAP_OBJECTS = 16_384
MAX_APPLE_LINK_INPUT_BYTES = 64 * 1024 * 1024
MAX_APPLE_LINK_TOTAL_INPUT_BYTES = 512 * 1024 * 1024
MAX_APPLE_RUNTIME_IMAGES = 4096
MAX_APPLE_RUNTIME_DEPTH = 32
MAX_APPLE_RUNTIME_EDGES = 100_000
MAX_APPLE_TBD_BYTES = 64 * 1024 * 1024
MAX_GCC_BUILTIN_SPECS_BYTES = 16 * 1024 * 1024
MAX_GCC_EXTERNAL_SPECS_BYTES = 16 * 1024 * 1024
GCC_SPECS_FILE_INCLUDE_RE = re.compile(
    rb"(?m)^[ \t\r\f\v]*%include(?:_noerr)?(?=[ \t\r\f\v]|$)"
)
GCC_SPECS_FUNCTION_INCLUDE_RE = re.compile(
    rb"(?<!%)%:[ \t\r\f\v]*include[ \t\r\f\v]*\("
)


class BuildError(RuntimeError):
    """A fail-closed build or identity check failed."""


def _sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _canonical_bytes(value: object) -> bytes:
    return (
        json.dumps(
            value,
            sort_keys=True,
            separators=(",", ":"),
            ensure_ascii=False,
            allow_nan=False,
        )
        + "\n"
    ).encode("utf-8")


def _canonical_digest(value: object) -> str:
    return _sha256_bytes(_canonical_bytes(value))


def _require_file(path: Path, label: str) -> Path:
    resolved = path.resolve()
    if not resolved.is_file():
        raise BuildError(f"{label} is not a regular file: {path}")
    return resolved


def _require_directory(path: Path, label: str) -> Path:
    resolved = path.resolve()
    if not resolved.is_dir():
        raise BuildError(f"{label} is not a directory: {path}")
    return resolved


def _run(
    command: list[str],
    *,
    cwd: Path | None = None,
    env: dict[str, str] | None = None,
) -> str:
    try:
        result = subprocess.run(
            command,
            cwd=cwd,
            env=env,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
        )
    except OSError as exc:
        raise BuildError(f"cannot run {command[0]}: {exc}") from exc
    if result.returncode:
        output = result.stdout[-12000:].strip()
        rendered = " ".join(command)
        raise BuildError(f"command failed ({result.returncode}): {rendered}\n{output}")
    return result.stdout


def _expected_capability_profile(side: str) -> dict[str, object]:
    if side not in EXPECTED_CAPABILITIES:
        raise BuildError("capability profile side is unsupported")
    return {
        "schema": 1,
        "name": CAPABILITY_PROFILE_NAME,
        "side": side,
        "capabilities": dict(EXPECTED_CAPABILITIES[side]),
    }


def _validate_capability_profile(
    value: object, side: str, label: str = "capability profile",
) -> dict[str, object]:
    if side not in EXPECTED_CAPABILITIES:
        raise BuildError(f"{label} side is unsupported")
    if type(value) is not dict or set(value) != CAPABILITY_PROFILE_KEYS:
        raise BuildError(f"{label} has an unsupported field set")
    profile = value
    if type(profile["schema"]) is not int or profile["schema"] != 1:
        raise BuildError(f"{label} schema is unsupported")
    if profile["name"] != CAPABILITY_PROFILE_NAME:
        raise BuildError(f"{label} name is unsupported")
    if profile["side"] != side:
        raise BuildError(f"{label} side does not match the build side")
    capabilities = profile["capabilities"]
    if (type(capabilities) is not dict
            or set(capabilities) != set(CAPABILITY_NAMES)):
        raise BuildError(f"{label} capability fields are incomplete or unknown")
    expected = EXPECTED_CAPABILITIES[side]
    for name in CAPABILITY_NAMES:
        capability = capabilities[name]
        if type(capability) is not bool or capability is not expected[name]:
            raise BuildError(f"{label} capability {name} is stale")
    return profile


def _probe_capability_profile(
    executable: Path, side: str, env: dict[str, str] | None,
) -> dict[str, object]:
    executable = _require_file(executable, f"{side} semantic adapter executable")
    try:
        result = subprocess.run(
            [str(executable), "--capability-profile"],
            env=env,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=10,
        )
    except (OSError, subprocess.TimeoutExpired) as exc:
        raise BuildError(
            f"cannot probe {side} semantic adapter capability profile: {exc}"
        ) from exc
    if result.returncode != 0:
        detail = result.stderr[:4096].decode("utf-8", "replace").strip()
        raise BuildError(
            f"{side} semantic adapter capability probe exited "
            f"{result.returncode}: {detail}"
        )
    if result.stderr:
        detail = result.stderr[:4096].decode("utf-8", "replace").strip()
        raise BuildError(
            f"{side} semantic adapter capability probe wrote to stderr: {detail}"
        )
    if len(result.stdout) > 16 * 1024:
        raise BuildError(
            f"{side} semantic adapter capability profile exceeds the output limit"
        )

    def reject_duplicates(pairs: list[tuple[str, object]]) -> dict[str, object]:
        parsed: dict[str, object] = {}
        for key, item in pairs:
            if key in parsed:
                raise BuildError(
                    f"duplicate JSON key {key!r} in {side} capability profile"
                )
            parsed[key] = item
        return parsed

    def reject_constant(token: str) -> object:
        raise BuildError(
            f"non-finite JSON token {token!r} in {side} capability profile"
        )

    try:
        value = json.loads(
            result.stdout.decode("utf-8"),
            object_pairs_hook=reject_duplicates,
            parse_constant=reject_constant,
        )
    except (UnicodeError, json.JSONDecodeError) as exc:
        preview = result.stdout[:4096].decode("utf-8", "replace")
        raise BuildError(
            f"invalid {side} semantic adapter capability profile: {exc}; "
            f"stdout={preview!r}"
        ) from exc
    return _validate_capability_profile(
        value, side, f"{side} semantic adapter capability profile"
    )


_DRIVER_ENV_DIGEST_KEYS = (
    "CC", "CXX", "CFLAGS", "CXXFLAGS", "CPPFLAGS", "LDFLAGS", "PATH",
    "CMAKE_GENERATOR", "CMAKE_GENERATOR_PLATFORM", "CMAKE_GENERATOR_TOOLSET",
    "CMAKE_TOOLCHAIN_FILE", "CMAKE_PROJECT_INCLUDE",
    "CMAKE_PROJECT_INCLUDE_BEFORE", "CMAKE_PROJECT_TOP_LEVEL_INCLUDES",
    "CMAKE_PREFIX_PATH", "GCC_EXEC_PREFIX", "COMPILER_PATH",
    "CPATH", "C_INCLUDE_PATH", "CPLUS_INCLUDE_PATH", "OBJC_INCLUDE_PATH",
    "LIBRARY_PATH", "INCLUDE", "LIB", "LIBPATH", "SDKROOT",
    "MACOSX_DEPLOYMENT_TARGET", "HOME", "USERPROFILE", "APPDATA",
    "LOCALAPPDATA",
)
_REJECTED_DRIVER_ENV_KEYS = (
    "CCC_OVERRIDE_OPTIONS", "CL", "_CL_", "GCC_SPECS",
    "CLANG_CONFIG_FILE_USER_DIR", "CLANG_CONFIG_FILE_SYSTEM_DIR",
    "CMAKE_CXX_COMPILER_ARG1",
    "CMAKE_CXX_COMPILER_LAUNCHER", "CMAKE_CXX_LINKER_LAUNCHER",
    "CMAKE_RULE_LAUNCH_COMPILE", "CMAKE_RULE_LAUNCH_LINK",
    # These variables alter compiler subprogram, header, or library lookup
    # without appearing in CMake's direct compile-option vector. Hashing their
    # strings is insufficient: the referenced directories/files are mutable
    # external inputs. MSVC's INCLUDE/LIB/LIBPATH are intentionally fail-closed
    # here until a toolchain-specific content-binding profile is qualified.
    "GCC_EXEC_PREFIX", "COMPILER_PATH", "LIBRARY_PATH",
    "CPATH", "C_INCLUDE_PATH", "CPLUS_INCLUDE_PATH",
    "OBJC_INCLUDE_PATH", "OBJCPLUS_INCLUDE_PATH",
    "INCLUDE", "LIB", "LIBPATH",
    # CMake path/config injection can select packages or execute caller-supplied
    # project/toolchain files outside the controlled wrapper's source snapshot.
    "CMAKE_PREFIX_PATH", "CMAKE_MODULE_PATH", "CMAKE_TOOLCHAIN_FILE",
    "CMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN",
    "CMAKE_PROJECT_INCLUDE", "CMAKE_PROJECT_INCLUDE_BEFORE",
    "CMAKE_PROJECT_TOP_LEVEL_INCLUDES",
    # Visual Studio developer environments select or expose toolset, SDK,
    # and target-architecture roots. The controlled builder has no verified
    # MSVC content-closure profile yet, so these channels cannot qualify.
    "VSINSTALLDIR", "VCINSTALLDIR", "VCTOOLSINSTALLDIR", "VCTOOLSVERSION",
    "WINDOWSSDKDIR", "WINDOWSSDKVERSION", "UNIVERSALCRTSDKDIR", "UCRTVERSION",
    "WINDOWSLIBPATH", "EXTENSIONSDKDIR", "DEVENVDIR", "VISUALSTUDIOVERSION",
)
_REJECTED_DRIVER_ENV_PREFIXES = ("VSCMD_",)
_WRAPPER_DRIVER_NAMES = {
    "ccache", "sccache", "distcc", "icecc", "clcache",
}


def _controlled_driver_environment(
    parent_settings: dict[str, object] | None,
    source_environment: dict[str, str] | None = None,
    *,
    cmake_program: str = "cmake",
) -> tuple[dict[str, str], dict[str, object]]:
    """Screen known ambient overrides and bind the selected compiler inputs.

    Values are only hashed into the returned identity; environment contents
    (which may include credentials or private paths) are never serialized.
    """
    source = dict(os.environ if source_environment is None else source_environment)
    by_upper = {key.upper(): value for key, value in source.items()}
    rejected = [
        key for key in _REJECTED_DRIVER_ENV_KEYS
        if by_upper.get(key, "").strip()
    ]
    rejected.extend(
        key for key, value in by_upper.items()
        if value.strip() and key.startswith(_REJECTED_DRIVER_ENV_PREFIXES)
    )
    if rejected:
        raise BuildError(
            "unsupported ambient compiler/build override(s): "
            + ", ".join(sorted(rejected))
        )
    child = {
        key: value for key, value in source.items()
        if key.upper() not in _REJECTED_DRIVER_ENV_KEYS
    }

    cache_values: dict[str, object] = {}
    if parent_settings is not None:
        values = parent_settings.get("cacheVariables")
        if not isinstance(values, dict):
            raise BuildError("parent CMake cache-variable identity is malformed")
        cache_values = values
        forbidden_cache = [
            name for name in (
                "CMAKE_CXX_COMPILER_ARG1",
                "CMAKE_CXX_COMPILER_LAUNCHER",
                "CMAKE_CXX_LINKER_LAUNCHER",
                "CMAKE_RULE_LAUNCH_COMPILE",
                "CMAKE_RULE_LAUNCH_LINK",
                "CMAKE_TOOLCHAIN_FILE",
                "CMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN",
            )
            if str(cache_values.get(name, "")).strip()
        ]
        if forbidden_cache:
            raise BuildError(
                "parent CMake cache uses unsupported compiler, launcher, or "
                "toolchain channel(s): " + ", ".join(forbidden_cache)
            )

    selected_hashes: dict[str, str | None] = {}
    for name in _DRIVER_ENV_DIGEST_KEYS:
        value = by_upper.get(name)
        selected_hashes[name] = (
            None if value is None
            else _sha256_bytes(value.encode("utf-8", "surrogatepass"))
        )

    compiler_value = cache_values.get("CMAKE_CXX_COMPILER")
    if not isinstance(compiler_value, str) or not compiler_value:
        compiler_value = by_upper.get("CXX", "")
    if compiler_value:
        tokens = _split_command_fragment(
            compiler_value, "selected C++ compiler", reject_ambiguous_quotes=True
        )
        if len(tokens) != 1:
            raise BuildError(
                "compiler command contains an unbound argument-1/wrapper channel"
            )
        compiler_candidate = tokens[0]
    else:
        compiler_candidate = next(
            (candidate for candidate in ("c++", "g++", "clang++", "cl")
             if shutil.which(candidate, path=by_upper.get("PATH"))),
            "",
        )
        if not compiler_candidate:
            raise BuildError("cannot resolve a C++ compiler for controlled build context")
    compiler_path = _resolve_driver_path(
        compiler_candidate, by_upper.get("PATH"), "selected C++ compiler driver"
    )
    compiler_file = _require_file(compiler_path, "selected C++ compiler driver")
    compiler_digest = _sha256_file(compiler_file)
    cmake_path = _resolve_driver_path(
        cmake_program, by_upper.get("PATH"), "selected CMake executable"
    )
    identity = {
        "schema": 1,
        "mode": "controlled-driver-v1",
        "environmentDigest": _canonical_digest(selected_hashes),
        "cmakePath": cmake_path.as_posix(),
        "cmakePathDigest": _sha256_bytes(
            cmake_path.as_posix().encode("utf-8", "surrogatepass")
        ),
        "cmakeBinaryDigest": _sha256_file(cmake_path),
        "compilerPath": compiler_path.as_posix(),
        "compilerPathDigest": _sha256_bytes(
            compiler_path.as_posix().encode("utf-8", "surrogatepass")
        ),
        "compilerBinaryDigest": compiler_digest,
    }
    identity["digest"] = _canonical_digest(identity)
    return child, identity


def _resolve_driver_path(
    command: str, path_environment: str | None, label: str
) -> Path:
    resolved = shutil.which(command, path=path_environment)
    # Preserve the exact invocation path (including symlink basename): driver
    # behavior can depend on argv[0].  Hashing and file checks follow the link.
    candidate = Path(resolved or command).absolute()
    if not candidate.is_file():
        raise BuildError(f"cannot resolve {label}")
    if candidate.name.casefold() in _WRAPPER_DRIVER_NAMES:
        raise BuildError(f"unsupported compiler wrapper selected as {label}")
    return candidate


def _tool_binary_identity(
    command: str,
    label: str,
    path_environment: str | None,
) -> dict[str, str]:
    if not command:
        return {"path": "", "digest": ""}
    path = _resolve_driver_path(command, path_environment, label)
    return {"path": path.as_posix(), "digest": _sha256_file(path)}


def _verify_driver_binary(driver_context: dict[str, object]) -> None:
    cmake_path = driver_context.get("cmakePath")
    cmake_expected = driver_context.get("cmakeBinaryDigest")
    if cmake_path is not None or cmake_expected is not None:
        if not isinstance(cmake_path, str) or not isinstance(cmake_expected, str):
            raise BuildError("controlled CMake identity is malformed")
        cmake_binary = _require_file(Path(cmake_path), "controlled CMake executable")
        if _sha256_file(cmake_binary) != cmake_expected:
            raise BuildError("selected CMake executable changed during the controlled build")
    path = driver_context.get("compilerPath")
    expected = driver_context.get("compilerBinaryDigest")
    if not isinstance(path, str) or not isinstance(expected, str):
        raise BuildError("controlled compiler identity is malformed")
    compiler = _require_file(Path(path), "controlled C++ compiler driver")
    if _sha256_file(compiler) != expected:
        raise BuildError("selected C++ compiler changed during the controlled build")


def _verify_tool_binary_identities(value: object) -> None:
    if not isinstance(value, dict):
        raise BuildError("configured build tool identity is malformed")
    for role, identity in value.items():
        if not isinstance(role, str) or not isinstance(identity, dict):
            raise BuildError("configured build tool identity is malformed")
        path = identity.get("path")
        expected = identity.get("digest")
        if not isinstance(path, str) or not isinstance(expected, str):
            raise BuildError("configured build tool identity is malformed")
        if bool(path) != bool(expected):
            raise BuildError(f"configured {role} path/digest presence differs")
        if not path:
            continue
        binary = _require_file(Path(path), f"configured {role} binary")
        if _sha256_file(binary) != expected:
            raise BuildError(f"configured {role} binary changed during the controlled build")


def _compiler_subprogram_identity(
    compiler_path: str,
    program: str,
    *,
    path_environment: str | None,
    env: dict[str, str] | None,
    driver_context: dict[str, object] | None,
) -> dict[str, str]:
    """Bind a GCC subprogram only when the driver reports an exact file path."""
    if driver_context is not None:
        _verify_driver_binary(driver_context)
    output = _run(
        [compiler_path, "-print-prog-name=" + program], env=env
    ).strip()
    if driver_context is not None:
        _verify_driver_binary(driver_context)
    if not output or len(output.splitlines()) != 1:
        return {"path": "", "digest": ""}
    candidate = Path(output)
    # A bare program name means the driver did not expose the resolved binary
    # path. Do not resolve it through our PATH, which could select a different
    # tool from the driver's private search directories.
    if not candidate.is_absolute() or not candidate.is_file():
        return {"path": "", "digest": ""}
    return _tool_binary_identity(
        candidate.as_posix(),
        "configured compiler subprogram " + program,
        path_environment,
    )


def _compiler_reported_file_identity(
    compiler_path: str,
    option: str,
    label: str,
    *,
    path_environment: str | None,
    env: dict[str, str] | None,
    driver_context: dict[str, object] | None,
) -> dict[str, str]:
    if driver_context is not None:
        _verify_driver_binary(driver_context)
    output = _run([compiler_path, option], env=env).strip()
    if driver_context is not None:
        _verify_driver_binary(driver_context)
    if not output or len(output.splitlines()) != 1:
        return {"path": "", "digest": ""}
    candidate = Path(output)
    if not candidate.is_absolute() or not candidate.is_file():
        return {"path": "", "digest": ""}
    return _tool_binary_identity(
        candidate.as_posix(), label, path_environment
    )


def _compiler_builtin_specs_profile(
    compiler_id: str,
    compiler_path: str,
    *,
    controlled_fresh_build: bool,
    env: dict[str, str] | None,
    driver_context: dict[str, object] | None,
    external_specs_identity: dict[str, str] | None = None,
    schema: int = 2,
) -> dict[str, object]:
    """Bind raw GCC specs and conservatively identify unresolved includes."""
    if schema not in (1, 2):
        raise BuildError("unsupported GCC built-in specs profile schema")
    profile: dict[str, object] = {
        "schema": schema,
        "status": "unavailable",
        "method": "",
        "compilerId": compiler_id,
        "byteCount": 0,
        "digest": "",
        "reason": "compiler-family-not-qualified",
    }
    if schema == 2:
        profile["includeClosure"] = {
            "schema": 1,
            "status": "unavailable",
            "builtinDirectiveCount": 0,
            "externalDirectiveCount": 0,
            "reason": "compiler-family-not-qualified",
        }

    def unavailable(reason: str) -> dict[str, object]:
        profile["reason"] = reason
        include_closure = profile.get("includeClosure")
        if isinstance(include_closure, dict):
            include_closure["reason"] = reason
        return profile

    if compiler_id != "GNU":
        return profile
    if not controlled_fresh_build:
        return unavailable("existing-build-not-scanned-before-compilation")
    if not compiler_path:
        return unavailable("compiler-path-unavailable")
    if driver_context is not None:
        _verify_driver_binary(driver_context)
    try:
        result = subprocess.run(
            [compiler_path, "-dumpspecs"],
            env=env,
            check=False,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            timeout=30,
        )
    except (OSError, subprocess.SubprocessError):
        if driver_context is not None:
            _verify_driver_binary(driver_context)
        return unavailable("gcc-built-in-specs-query-failed")
    if driver_context is not None:
        _verify_driver_binary(driver_context)
    output = result.stdout
    if result.returncode != 0 or result.stderr:
        return unavailable("gcc-built-in-specs-query-failed")
    if len(output) > MAX_GCC_BUILTIN_SPECS_BYTES:
        return unavailable("gcc-built-in-specs-size-limit")
    if not output:
        return unavailable("gcc-built-in-specs-empty")
    captured: dict[str, object] = {
        "schema": schema,
        "status": "captured",
        "method": "gcc-dumpspecs-stdout-v1",
        "compilerId": compiler_id,
        "byteCount": len(output),
        "digest": hashlib.sha256(output).hexdigest(),
        "reason": "",
    }
    if schema == 2:
        captured["includeClosure"] = _gcc_specs_include_observation(
            output, external_specs_identity
        )
    return captured


def _gcc_specs_include_directive_count(data: bytes) -> int:
    """Count known GCC specs include forms without attempting to resolve them.

    This deliberately over-detects directive-like text. It is only an
    admission guard: a positive count means the profile remains partial until
    a native resolver or actual-open observer binds the effective targets.
    """
    return (
        len(GCC_SPECS_FILE_INCLUDE_RE.findall(data))
        + len(GCC_SPECS_FUNCTION_INCLUDE_RE.findall(data))
    )


def _gcc_specs_include_observation(
    builtin_specs: bytes,
    external_specs_identity: dict[str, str] | None,
) -> dict[str, object]:
    observation: dict[str, object] = {
        "schema": 1,
        "status": "unavailable",
        "builtinDirectiveCount": _gcc_specs_include_directive_count(builtin_specs),
        "externalDirectiveCount": 0,
        "reason": "external-specs-file-identity-unavailable",
    }
    if not isinstance(external_specs_identity, dict):
        return observation
    path = external_specs_identity.get("path")
    expected_digest = external_specs_identity.get("digest")
    if not isinstance(path, str) or not isinstance(expected_digest, str):
        return observation
    if not path or not expected_digest:
        return observation
    try:
        external_specs = _require_file(Path(path), "configured GCC specs file")
        with external_specs.open("rb") as stream:
            content = stream.read(MAX_GCC_EXTERNAL_SPECS_BYTES + 1)
    except (BuildError, OSError):
        observation["reason"] = "external-specs-file-unreadable"
        return observation
    if len(content) > MAX_GCC_EXTERNAL_SPECS_BYTES:
        observation["reason"] = "external-specs-file-size-limit"
        return observation
    actual_digest = hashlib.sha256(content).hexdigest()
    if actual_digest != expected_digest:
        raise BuildError("configured GCC specs file changed during capture")
    external_count = _gcc_specs_include_directive_count(content)
    observation["externalDirectiveCount"] = external_count
    total_count = int(observation["builtinDirectiveCount"]) + external_count
    if total_count:
        observation["status"] = "partial"
        observation["reason"] = "spec-includes-not-resolved-or-content-bound"
    else:
        observation["status"] = "no-includes-detected"
        observation["reason"] = ""
    return observation


def _verify_compiler_builtin_specs_profile(
    cmake_data: dict[str, object],
    *,
    env: dict[str, str] | None,
    driver_context: dict[str, object] | None,
) -> None:
    expected = cmake_data.get("compilerBuiltinSpecs")
    compiler_id = cmake_data.get("compilerId")
    compiler_path = cmake_data.get("compilerPath")
    if (not isinstance(expected, dict) or not isinstance(compiler_id, str)
            or not isinstance(compiler_path, str)):
        raise BuildError("GCC built-in specs profile is malformed")
    if expected.get("status") != "captured":
        return
    current = _compiler_builtin_specs_profile(
        compiler_id, compiler_path, controlled_fresh_build=True,
        env=env, driver_context=driver_context,
        external_specs_identity=(
            cmake_data.get("toolBinaries", {}).get("compilerSpecs")
            if isinstance(cmake_data.get("toolBinaries"), dict)
            else None
        ),
        schema=(2 if expected.get("schema") == 2 else 1),
    )
    if current != expected:
        raise BuildError("GCC built-in specs changed during the controlled build")


def _read_json(path: Path, label: str) -> dict[str, object]:
    source = _require_file(path, label)

    def reject_duplicates(pairs: list[tuple[str, object]]) -> dict[str, object]:
        result: dict[str, object] = {}
        for key, value in pairs:
            if key in result:
                raise BuildError(f"duplicate JSON key {key!r} in {label}")
            result[key] = value
        return result

    def reject_constant(token: str) -> object:
        raise BuildError(f"non-finite JSON token {token!r} in {label}")

    try:
        value = json.loads(
            source.read_bytes().decode("utf-8"),
            object_pairs_hook=reject_duplicates,
            parse_constant=reject_constant,
        )
    except (UnicodeError, json.JSONDecodeError) as exc:
        raise BuildError(f"invalid {label}: {exc}") from exc
    if not isinstance(value, dict):
        raise BuildError(f"{label} root must be an object")
    return value


def _object(parent: dict[str, object], key: str) -> dict[str, object]:
    value = parent.get(key)
    if not isinstance(value, dict):
        raise BuildError(f"lock field {key!r} must be an object")
    return value


def _string(parent: dict[str, object], key: str) -> str:
    value = parent.get(key)
    if not isinstance(value, str) or not value:
        raise BuildError(f"lock field {key!r} must be a non-empty string")
    return value


def _integer(parent: dict[str, object], key: str) -> int:
    value = parent.get(key)
    if isinstance(value, bool) or not isinstance(value, int) or value < 0:
        raise BuildError(f"lock field {key!r} must be a non-negative integer")
    return value


def _validate_lock(lock: dict[str, object]) -> dict[str, object]:
    if lock.get("schema") != 1:
        raise BuildError("target lock schema must be 1")
    target = _object(lock, "libreCAD")
    archive = _object(lock, "archive")
    manifest = _object(lock, "manifest")
    commit = _string(target, "commit")
    snapshot_revision = _string(target, "snapshotRevision")
    archive_digest = _string(archive, "sha256")
    manifest_digest = _string(manifest, "sha256")
    if not HEX40.fullmatch(commit):
        raise BuildError("target commit must be a lowercase 40-hex object ID")
    if not HEX40.fullmatch(snapshot_revision):
        raise BuildError("snapshot revision must be a lowercase 40-hex object ID")
    if not HEX64.fullmatch(archive_digest):
        raise BuildError("archive SHA-256 must be lowercase 64-hex")
    if not HEX64.fullmatch(manifest_digest):
        raise BuildError("manifest SHA-256 must be lowercase 64-hex")
    if _string(target, "sourceRoot") != ARCHIVE_PATHS[0]:
        raise BuildError("target sourceRoot differs from the closed archive recipe")
    if _string(target, "sourceList") != ARCHIVE_PATHS[1]:
        raise BuildError("target sourceList differs from the closed archive recipe")
    if _string(archive, "format") != "tar":
        raise BuildError("target archive format must be tar")
    if _string(archive, "prefix") != ARCHIVE_PREFIX:
        raise BuildError("target archive prefix must be libdxfrw/")
    _integer(manifest, "entries")
    return {
        "commit": commit,
        "snapshotRevision": snapshot_revision,
        "archiveDigest": archive_digest,
        "manifestDigest": manifest_digest,
        "manifestEntries": _integer(manifest, "entries"),
    }


def _safe_manifest_name(name: str) -> bool:
    path = PurePosixPath(name)
    return (
        bool(name)
        and not name.startswith("/")
        and "\\" not in name
        and "\x00" not in name
        and ".." not in path.parts
        and "." not in path.parts
    )


def _read_manifest(path: Path, lock_data: dict[str, object]) -> dict[str, tuple[str, str]]:
    manifest_path = _require_file(path, "target source manifest")
    manifest_bytes = manifest_path.read_bytes()
    actual_digest = _sha256_bytes(manifest_bytes)
    if actual_digest != lock_data["manifestDigest"]:
        raise BuildError(
            "target source manifest SHA-256 differs from the target lock: "
            f"{actual_digest}"
        )
    entries: dict[str, tuple[str, str]] = {}
    allowed = {"manifest", "source", "header-listed", "header-unlisted"}
    for line_number, line in enumerate(
        manifest_bytes.decode("utf-8").splitlines(), 1
    ):
        if not line or line.startswith("#"):
            continue
        fields = line.split("|")
        if len(fields) != 4 or not all(fields):
            raise BuildError(f"malformed source manifest line {line_number}")
        name, mode, blob, classification = fields
        if not _safe_manifest_name(name):
            raise BuildError(f"unsafe source manifest path on line {line_number}")
        if name in entries:
            raise BuildError(f"duplicate source manifest path: {name}")
        if not re.fullmatch(r"100[0-7]{3}", mode):
            raise BuildError(f"invalid source manifest mode on line {line_number}")
        if not HEX40.fullmatch(blob):
            raise BuildError(f"invalid source manifest blob on line {line_number}")
        if classification not in allowed:
            raise BuildError(
                f"invalid source manifest classification on line {line_number}"
            )
        if name != ARCHIVE_PATHS[1] and not name.startswith(ARCHIVE_PATHS[0] + "/"):
            raise BuildError(f"source manifest path is outside the archive recipe: {name}")
        entries[name] = (mode, blob)
    if len(entries) != lock_data["manifestEntries"]:
        raise BuildError(
            f"source manifest has {len(entries)} entries; lock requires "
            f"{lock_data['manifestEntries']}"
        )
    return entries


def _git_blob_digest(data: bytes) -> str:
    header = f"blob {len(data)}\0".encode("ascii")
    return hashlib.sha1(header + data).hexdigest()  # noqa: S324 - Git SHA-1 identity


def _write_locked_archive(
    git_program: str,
    git_dir: Path,
    commit: str,
    destination: Path,
) -> str:
    git_directory = _require_directory(git_dir, "LibreCAD Git directory")
    resolved = _run(
        [
            git_program,
            "--git-dir=" + str(git_directory),
            "rev-parse",
            "--verify",
            commit + "^{commit}",
        ]
    ).strip()
    if resolved != commit:
        raise BuildError(f"target commit resolved to unexpected object: {resolved}")
    command = [
        git_program,
        "--git-dir=" + str(git_directory),
        "archive",
        "--format=tar",
        "--prefix=" + ARCHIVE_PREFIX,
        commit,
        *ARCHIVE_PATHS,
    ]
    destination.parent.mkdir(parents=True, exist_ok=True)
    try:
        with destination.open("wb") as output, tempfile.TemporaryFile() as error:
            process = subprocess.Popen(command, stdout=output, stderr=error)
            status = process.wait()
            if status:
                error.seek(0)
                details = error.read().decode("utf-8", "replace")[-12000:].strip()
                raise BuildError(f"git archive failed ({status}): {details}")
    except OSError as exc:
        raise BuildError(f"cannot create locked target archive: {exc}") from exc
    return _sha256_file(destination)


def _archive_member_path(name: str) -> PurePosixPath:
    if not name or name.startswith("/") or "\\" in name or "\x00" in name:
        raise BuildError(f"unsafe target archive path: {name!r}")
    path = PurePosixPath(name)
    if ".." in path.parts or "." in path.parts:
        raise BuildError(f"unsafe target archive path: {name!r}")
    archive_root = ARCHIVE_PREFIX.rstrip("/")
    if name.rstrip("/") != archive_root and not name.startswith(ARCHIVE_PREFIX):
        raise BuildError(f"target archive member is outside {ARCHIVE_PREFIX}: {name}")
    return path


def _extract_and_verify_archive(
    archive_path: Path,
    destination: Path,
    manifest: dict[str, tuple[str, str]],
) -> None:
    seen: set[str] = set()
    file_entries: dict[str, tuple[str, str]] = {}
    total_bytes = 0
    try:
        archive = tarfile.open(archive_path, mode="r:")
    except (OSError, tarfile.TarError) as exc:
        raise BuildError(f"cannot open target archive: {exc}") from exc
    with archive:
        members = archive.getmembers()
        if len(members) > MAX_ARCHIVE_MEMBERS:
            raise BuildError("target archive has too many members")
        for member in members:
            member_path = _archive_member_path(member.name)
            normalized = member_path.as_posix().rstrip("/")
            if normalized in seen:
                raise BuildError(f"duplicate target archive member: {normalized}")
            seen.add(normalized)
            if not (member.isdir() or member.isfile()):
                raise BuildError(
                    f"target archive contains a non-file/non-directory: {member.name}"
                )
            if member.isfile():
                if member.size < 0:
                    raise BuildError(f"negative target archive size: {member.name}")
                total_bytes += member.size
                if total_bytes > MAX_ARCHIVE_BYTES:
                    raise BuildError("target archive expands beyond the size limit")
                relative_name = normalized[len(ARCHIVE_PREFIX) :]
                expected = manifest.get(relative_name)
                if expected is None:
                    raise BuildError(
                        f"target archive file is absent from the manifest: {relative_name}"
                    )
                source = archive.extractfile(member)
                if source is None:
                    raise BuildError(f"cannot read target archive member: {member.name}")
                data = source.read()
                if len(data) != member.size:
                    raise BuildError(f"short target archive member: {member.name}")
                # git archive's portable tar mode adds group-write bits.  Git's
                # blob mode identity is only regular-vs-executable, so normalize
                # to the corresponding tree mode before comparing the manifest.
                mode = "100755" if member.mode & 0o111 else "100644"
                blob = _git_blob_digest(data)
                if (mode, blob) != expected:
                    raise BuildError(
                        f"target archive identity differs from manifest: {relative_name}"
                    )
                file_entries[relative_name] = (mode, blob)

                output = destination.joinpath(*member_path.parts)
                output.parent.mkdir(parents=True, exist_ok=True)
                if output.exists():
                    raise BuildError(f"target archive extraction collision: {member.name}")
                with output.open("xb") as stream:
                    stream.write(data)
                output.chmod(int(expected[0][-3:], 8))
            else:
                destination.joinpath(*member_path.parts).mkdir(
                    parents=True, exist_ok=True
                )
    if file_entries != manifest:
        missing = sorted(set(manifest) - set(file_entries))
        raise BuildError(f"target archive omits manifest files: {missing}")


def _verify_extracted_tree(
    source_cache: Path, manifest: dict[str, tuple[str, str]]
) -> None:
    prefix_root = source_cache / ARCHIVE_PREFIX.rstrip("/")
    if not prefix_root.is_dir() or prefix_root.is_symlink():
        raise BuildError("cached target source root is missing or unsafe")
    actual: dict[str, tuple[str, str]] = {}
    for path in sorted(prefix_root.rglob("*")):
        if path.is_symlink():
            raise BuildError(f"cached target source contains a symlink: {path}")
        if path.is_dir():
            continue
        if not path.is_file():
            raise BuildError(f"cached target source contains a special file: {path}")
        relative = path.relative_to(prefix_root).as_posix()
        if os.name == "nt":
            # Windows filesystems do not preserve Git's POSIX rwx bits.  The
            # archive member mode was already checked during extraction; for a
            # cached tree, retain that locked mode and revalidate content.
            expected = manifest.get(relative)
            mode = expected[0] if expected is not None else "100644"
        else:
            mode = "100755" if path.stat().st_mode & 0o111 else "100644"
        actual[relative] = (mode, _git_blob_digest(path.read_bytes()))
    if actual != manifest:
        missing = sorted(set(manifest) - set(actual))
        extra = sorted(set(actual) - set(manifest))
        changed = sorted(
            name
            for name in set(actual) & set(manifest)
            if actual[name] != manifest[name]
        )
        raise BuildError(
            "cached target source differs from manifest: "
            f"missing={missing} extra={extra} changed={changed}"
        )


def _materialize_target_source(
    *,
    build_dir: Path,
    git_program: str,
    git_dir: Path,
    lock_data: dict[str, object],
    manifest: dict[str, tuple[str, str]],
) -> Path:
    source_root = build_dir / "sources"
    source_root.mkdir(parents=True, exist_ok=True)
    digest = str(lock_data["archiveDigest"])
    cached = source_root / digest
    if cached.exists():
        if not cached.is_dir() or cached.is_symlink():
            raise BuildError(f"target source cache is not a safe directory: {cached}")
        _verify_extracted_tree(cached, manifest)
        return cached

    with tempfile.TemporaryDirectory(prefix=".archive-", dir=source_root) as temporary:
        temporary_path = Path(temporary)
        archive_path = temporary_path / "target.tar"
        actual_digest = _write_locked_archive(
            git_program, git_dir, str(lock_data["commit"]), archive_path
        )
        if actual_digest != digest:
            raise BuildError(
                "target archive SHA-256 differs from the target lock: "
                f"{actual_digest}"
            )
        staged = temporary_path / "extracted"
        staged.mkdir()
        _extract_and_verify_archive(archive_path, staged, manifest)
        _verify_extracted_tree(staged, manifest)
        try:
            staged.rename(cached)
        except FileExistsError:
            _verify_extracted_tree(cached, manifest)
    return cached


def _parse_cmake_cache(path: Path) -> dict[str, str]:
    cache_path = _require_file(path / "CMakeCache.txt", "CMake cache")
    values: dict[str, str] = {}
    for line in cache_path.read_text(encoding="utf-8", errors="replace").splitlines():
        if not line or line.startswith("#") or line.startswith("//") or "=" not in line:
            continue
        key_and_type, value = line.split("=", 1)
        key = key_and_type.split(":", 1)[0]
        values[key] = value
    return values


def _cmake_set_value(text: str, name: str, default: str = "") -> str:
    quoted = re.search(
        r"(?m)^set\(\s*" + re.escape(name)
        + r'\s+"((?:\\.|[^"\r\n])*)"\s*\)$',
        text,
    )
    if quoted:
        return quoted.group(1)
    unquoted = re.search(
        r"(?m)^set\(\s*" + re.escape(name) + r"(?:\s+([^\r\n)]*?))?\s*\)$",
        text,
    )
    if unquoted:
        return (unquoted.group(1) or "").strip()
    return default


def _cmake_generated_text(build_tree: Path, filename: str, label: str) -> str:
    candidates = sorted((build_tree / "CMakeFiles").glob("*/" + filename))
    if len(candidates) != 1:
        raise BuildError(
            f"expected exactly one configured {label}; found {candidates}"
        )
    return candidates[0].read_text(encoding="utf-8", errors="replace")


def _cmake_list(value: str) -> list[str]:
    return [item for item in value.split(";") if item]


def _cmake_bool(value: str, label: str) -> bool:
    normalized = value.upper()
    if normalized in {"1", "ON", "TRUE", "YES", "Y"}:
        return True
    if normalized in {"", "0", "OFF", "FALSE", "NO", "N", "IGNORE", "NOTFOUND"}:
        return False
    raise BuildError(f"configured {label} is not boolean: {value!r}")


def _file_api_cmake_identity(build_tree: Path) -> dict[str, object]:
    reply_root = build_tree / ".cmake/api/v1/reply"
    indexes = sorted(
        reply_root.glob("index-*.json"), key=lambda path: (path.stat().st_mtime, path.name)
    )
    for index_path in reversed(indexes):
        index = _read_json(index_path, "CMake file-api index")
        cmake = index.get("cmake")
        if not isinstance(cmake, dict):
            continue
        version = cmake.get("version")
        generator = cmake.get("generator")
        paths = cmake.get("paths")
        if (
            not isinstance(version, dict)
            or not isinstance(version.get("string"), str)
            or not version["string"]
            or not isinstance(generator, dict)
            or not isinstance(generator.get("name"), str)
            or not generator["name"]
            or not isinstance(generator.get("multiConfig"), bool)
            or not isinstance(paths, dict)
            or not isinstance(paths.get("cmake"), str)
            or not paths["cmake"]
        ):
            raise BuildError("CMake file-api configure identity is incomplete")
        return {
            "configuredCmakeVersion": version["string"],
            "cmakeExecutable": paths["cmake"],
            "generator": generator["name"],
            "multiConfig": generator["multiConfig"],
        }
    raise BuildError("CMake file-api configure identity is absent")


def _cmake_metadata(
    build_tree: Path,
    cmake_program: str,
    configuration: str,
    *,
    env: dict[str, str] | None = None,
    driver_context: dict[str, object] | None = None,
    controlled_fresh_build: bool = False,
) -> dict[str, object]:
    _ensure_codemodel(
        build_tree, configuration, cmake_program, env=env,
        driver_context=driver_context,
    )
    cache = _parse_cmake_cache(build_tree)
    unsupported_cache_channels = (
        "CMAKE_CXX_COMPILER_ARG1",
        "CMAKE_CXX_COMPILER_LAUNCHER",
        "CMAKE_CXX_LINKER_LAUNCHER",
        "CMAKE_RULE_LAUNCH_COMPILE",
        "CMAKE_RULE_LAUNCH_LINK",
    )
    active_channels = [
        key for key in unsupported_cache_channels
        if cache.get(key, "").strip()
    ]
    if active_channels:
        raise BuildError(
            "configured CMake cache contains unsupported compiler wrapper/"
            "argument channel(s): " + ", ".join(active_channels)
        )
    compiler_text = _cmake_generated_text(
        build_tree, "CMakeCXXCompiler.cmake", "C++ compiler identity"
    )
    system_text = _cmake_generated_text(
        build_tree, "CMakeSystem.cmake", "CMake target-system identity"
    )
    api_identity = _file_api_cmake_identity(build_tree)
    version_output = _run([cmake_program, "--version"], env=env).splitlines()
    if not version_output or not version_output[0].strip():
        raise BuildError("CMake version output is empty")
    cmake_version = version_output[0].strip()
    cache_build_type = cache.get("CMAKE_BUILD_TYPE", "")
    configuration_types = _cmake_list(cache.get("CMAKE_CONFIGURATION_TYPES", ""))
    if api_identity["generator"] != cache.get("CMAKE_GENERATOR", ""):
        raise BuildError("CMake cache/file-api generator identities differ")
    if bool(api_identity["multiConfig"]) != bool(configuration_types):
        raise BuildError("CMake multi-config identity is inconsistent")
    compiler_path = _cmake_set_value(compiler_text, "CMAKE_CXX_COMPILER")
    compiler_id = _cmake_set_value(compiler_text, "CMAKE_CXX_COMPILER_ID")
    compiler_version = _cmake_set_value(
        compiler_text, "CMAKE_CXX_COMPILER_VERSION"
    )
    if not compiler_path or not compiler_id or not compiler_version:
        raise BuildError("configured C++ compiler identity is incomplete")
    path_environment = env.get("PATH") if env is not None else None
    linker_path = _cmake_set_value(
        compiler_text,
        "CMAKE_CXX_COMPILER_LINKER",
        _cmake_set_value(compiler_text, "CMAKE_LINKER"),
    )
    tool_binaries = {
        "cmake": _tool_binary_identity(
            str(api_identity["cmakeExecutable"]),
            "configured CMake executable",
            path_environment,
        ),
        "buildProgram": _tool_binary_identity(
            cache.get("CMAKE_MAKE_PROGRAM", ""),
            "configured build program",
            path_environment,
        ),
        "compiler": _tool_binary_identity(
            compiler_path, "configured C++ compiler driver", path_environment
        ),
        "compilerLinker": _tool_binary_identity(
            linker_path, "configured compiler linker", path_environment
        ),
        "archiver": _tool_binary_identity(
            _cmake_set_value(compiler_text, "CMAKE_AR"),
            "configured static archiver",
            path_environment,
        ),
        "ranlib": _tool_binary_identity(
            _cmake_set_value(compiler_text, "CMAKE_RANLIB"),
            "configured archive indexer",
            path_environment,
        ),
    }
    for role in (
        "compilerFrontend", "compilerAssembler", "compilerCollect2",
        "compilerLtoWrapper", "compilerSpecs",
    ):
        tool_binaries[role] = {"path": "", "digest": ""}
    if compiler_id == "GNU":
        for role, program in (
            ("compilerFrontend", "cc1plus"),
            ("compilerAssembler", "as"),
            ("compilerCollect2", "collect2"),
        ):
            tool_binaries[role] = _compiler_subprogram_identity(
                compiler_path,
                program,
                path_environment=path_environment,
                env=env,
                driver_context=driver_context,
            )
        tool_binaries["compilerLtoWrapper"] = _compiler_subprogram_identity(
            compiler_path,
            "lto-wrapper",
            path_environment=path_environment,
            env=env,
            driver_context=driver_context,
        )
        tool_binaries["compilerSpecs"] = _compiler_reported_file_identity(
            compiler_path,
            "-print-file-name=specs",
            "configured GCC specs file",
            path_environment=path_environment,
            env=env,
            driver_context=driver_context,
        )
    if driver_context is not None and any(
        not tool_binaries[name]["digest"]
        for name in ("cmake", "buildProgram", "compiler")
    ):
        raise BuildError(
            "controlled CMake identity omits a required tool binary digest"
        )
    if driver_context is not None:
        cmake_path_digest = _sha256_bytes(
            tool_binaries["cmake"]["path"].encode("utf-8", "surrogatepass")
        )
        if cmake_path_digest != driver_context.get("cmakePathDigest"):
            raise BuildError(
                "configured CMake executable differs from the controlled driver"
            )
        if tool_binaries["cmake"]["digest"] != driver_context.get(
            "cmakeBinaryDigest"
        ):
            raise BuildError(
                "configured CMake binary differs from the controlled driver"
            )
    if driver_context is not None:
        configured_compiler = _resolve_driver_path(
            compiler_path, env.get("PATH") if env is not None else None,
            "configured C++ compiler driver",
        )
        configured_path_digest = _sha256_bytes(
            configured_compiler.as_posix().encode("utf-8", "surrogatepass")
        )
        if configured_path_digest != driver_context.get("compilerPathDigest"):
            raise BuildError(
                "configured C++ compiler differs from the controlled driver"
            )
        if _sha256_file(configured_compiler) != driver_context.get(
            "compilerBinaryDigest"
        ):
            raise BuildError(
                "configured C++ compiler binary differs from the controlled driver"
            )
    config_upper = configuration.upper()
    identity: dict[str, object] = {
        "cmakeVersion": cmake_version,
        **api_identity,
        "cmakeExecutable": tool_binaries["cmake"]["path"],
        "generatorPlatform": cache.get("CMAKE_GENERATOR_PLATFORM", ""),
        "generatorToolset": cache.get("CMAKE_GENERATOR_TOOLSET", ""),
        "generatorInstance": cache.get("CMAKE_GENERATOR_INSTANCE", ""),
        "configurationTypes": configuration_types,
        "compilerPath": tool_binaries["compiler"]["path"],
        "compilerId": compiler_id,
        "compilerVersion": compiler_version,
        "compilerFrontendVariant": _cmake_set_value(
            compiler_text, "CMAKE_CXX_COMPILER_FRONTEND_VARIANT"
        ),
        "compilerSimulateId": _cmake_set_value(
            compiler_text, "CMAKE_CXX_SIMULATE_ID"
        ),
        "compilerSimulateVersion": _cmake_set_value(
            compiler_text, "CMAKE_CXX_SIMULATE_VERSION"
        ),
        "compilerTarget": _cmake_set_value(
            compiler_text,
            "CMAKE_CXX_COMPILER_TARGET",
            cache.get("CMAKE_CXX_COMPILER_TARGET", ""),
        ),
        "compilerArchitectureId": _cmake_set_value(
            compiler_text, "CMAKE_CXX_COMPILER_ARCHITECTURE_ID"
        ),
        "compilerAbi": _cmake_set_value(compiler_text, "CMAKE_CXX_COMPILER_ABI"),
        "compilerByteOrder": _cmake_set_value(
            compiler_text, "CMAKE_CXX_BYTE_ORDER"
        ),
        "sizeofDataPointer": _cmake_set_value(
            compiler_text, "CMAKE_CXX_SIZEOF_DATA_PTR"
        ),
        "compilerAppleSysroot": _cmake_set_value(
            compiler_text, "CMAKE_CXX_COMPILER_APPLE_SYSROOT"
        ),
        "compilerLinkerPath": tool_binaries["compilerLinker"]["path"],
        "compilerLinkerId": _cmake_set_value(
            compiler_text, "CMAKE_CXX_COMPILER_LINKER_ID"
        ),
        "compilerLinkerVersion": _cmake_set_value(
            compiler_text, "CMAKE_CXX_COMPILER_LINKER_VERSION"
        ),
        "compilerLinkerFrontendVariant": _cmake_set_value(
            compiler_text, "CMAKE_CXX_COMPILER_LINKER_FRONTEND_VARIANT"
        ),
        "archiverPath": tool_binaries["archiver"]["path"],
        "ranlibPath": tool_binaries["ranlib"]["path"],
        "systemName": _cmake_set_value(system_text, "CMAKE_SYSTEM_NAME"),
        "systemVersion": _cmake_set_value(system_text, "CMAKE_SYSTEM_VERSION"),
        "systemProcessor": _cmake_set_value(system_text, "CMAKE_SYSTEM_PROCESSOR"),
        "crossCompiling": _cmake_bool(
            _cmake_set_value(system_text, "CMAKE_CROSSCOMPILING"),
            "CMAKE_CROSSCOMPILING",
        ),
        "cxxStandard": 17,
        "cxxExtensions": False,
        "cacheBuildType": cache_build_type,
        "cxxFlags": cache.get("CMAKE_CXX_FLAGS", ""),
        "configurationFlags": cache.get("CMAKE_CXX_FLAGS_" + config_upper, ""),
        "exeLinkerFlags": cache.get("CMAKE_EXE_LINKER_FLAGS", ""),
        "configurationExeLinkerFlags": cache.get(
            "CMAKE_EXE_LINKER_FLAGS_" + config_upper, ""
        ),
        "staticLinkerFlags": cache.get("CMAKE_STATIC_LINKER_FLAGS", ""),
        "configurationStaticLinkerFlags": cache.get(
            "CMAKE_STATIC_LINKER_FLAGS_" + config_upper, ""
        ),
        "toolchainFile": cache.get("CMAKE_TOOLCHAIN_FILE", ""),
        "sysroot": cache.get("CMAKE_SYSROOT", ""),
        "sysrootCompile": cache.get("CMAKE_SYSROOT_COMPILE", ""),
        "sysrootLink": cache.get("CMAKE_SYSROOT_LINK", ""),
        "compilerLauncher": cache.get("CMAKE_CXX_COMPILER_LAUNCHER", ""),
        "linkerLauncher": cache.get("CMAKE_CXX_LINKER_LAUNCHER", ""),
        "osxArchitectures": _cmake_list(cache.get("CMAKE_OSX_ARCHITECTURES", "")),
        "osxSysroot": cache.get("CMAKE_OSX_SYSROOT", ""),
        "osxDeploymentTarget": cache.get("CMAKE_OSX_DEPLOYMENT_TARGET", ""),
        "msvcRuntimeLibrary": cache.get("CMAKE_MSVC_RUNTIME_LIBRARY", ""),
        "windowsTargetPlatformVersion": cache.get(
            "CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION", ""
        ),
        "positionIndependentCode": cache.get(
            "CMAKE_POSITION_INDEPENDENT_CODE", ""
        ),
        "interproceduralOptimization": cache.get(
            "CMAKE_INTERPROCEDURAL_OPTIMIZATION", ""
        ),
        "configurationInterproceduralOptimization": cache.get(
            "CMAKE_INTERPROCEDURAL_OPTIMIZATION_" + config_upper, ""
        ),
        "toolBinaries": tool_binaries,
    }
    identity["compilerBuiltinSpecs"] = _compiler_builtin_specs_profile(
        compiler_id,
        compiler_path,
        controlled_fresh_build=controlled_fresh_build,
        env=env,
        driver_context=driver_context,
        external_specs_identity=tool_binaries.get("compilerSpecs"),
    )
    return identity


def _request_codemodel(build_tree: Path) -> None:
    query = build_tree / ".cmake/api/v1/query/codemodel-v2"
    query.parent.mkdir(parents=True, exist_ok=True)
    query.touch(exist_ok=True)


def _codemodel_configuration(
    build_tree: Path, configuration: str
) -> tuple[dict[str, object], dict[str, dict[str, object]]]:
    reply_root = build_tree / ".cmake/api/v1/reply"
    indexes = sorted(reply_root.glob("index-*.json"), key=lambda path: path.stat().st_mtime)
    for index_path in reversed(indexes):
        index = _read_json(index_path, "CMake file-api index")
        reply = index.get("reply")
        if not isinstance(reply, dict):
            continue
        reference = reply.get("codemodel-v2")
        if not isinstance(reference, dict) or not isinstance(
            reference.get("jsonFile"), str
        ):
            continue
        codemodel = _read_json(
            reply_root / reference["jsonFile"], "CMake codemodel"
        )
        configurations = codemodel.get("configurations")
        if not isinstance(configurations, list):
            raise BuildError("CMake codemodel configurations must be an array")
        exact_candidates = [
            item
            for item in configurations
            if isinstance(item, dict)
            and item.get("name") == configuration
        ]
        if len(exact_candidates) == 1:
            selected = exact_candidates[0]
        else:
            default_candidates = [
                item
                for item in configurations
                if isinstance(item, dict) and item.get("name") == ""
            ]
            if len(exact_candidates) > 1 or len(default_candidates) != 1:
                raise BuildError(
                    f"CMake codemodel has no unique {configuration!r} configuration"
                )
            selected = default_candidates[0]
        target_refs = selected.get("targets")
        if not isinstance(target_refs, list):
            raise BuildError("CMake codemodel target list is missing")
        targets: dict[str, dict[str, object]] = {}
        for reference_row in target_refs:
            if not isinstance(reference_row, dict):
                raise BuildError("CMake codemodel target reference is malformed")
            name = reference_row.get("name")
            json_file = reference_row.get("jsonFile")
            if not isinstance(name, str) or not isinstance(json_file, str):
                raise BuildError("CMake codemodel target reference is incomplete")
            if name in targets:
                raise BuildError(f"duplicate CMake codemodel target: {name}")
            targets[name] = _read_json(
                reply_root / json_file, f"CMake target {name}"
            )
        return selected, targets
    raise BuildError("CMake file-api codemodel reply is absent")


def _ensure_codemodel(
    build_tree: Path,
    configuration: str,
    cmake_program: str,
    *,
    env: dict[str, str] | None = None,
    driver_context: dict[str, object] | None = None,
) -> tuple[dict[str, object], dict[str, dict[str, object]]]:
    try:
        return _codemodel_configuration(build_tree, configuration)
    except BuildError as exc:
        if "absent" not in str(exc):
            raise
    cache = _parse_cmake_cache(build_tree)
    home = cache.get("CMAKE_HOME_DIRECTORY")
    if not home:
        raise BuildError("CMake cache omits CMAKE_HOME_DIRECTORY")
    _request_codemodel(build_tree)
    command = [cmake_program, "-S", home, "-B", str(build_tree)]
    if driver_context is not None:
        _verify_driver_binary(driver_context)
        compiler_path = driver_context.get("compilerPath")
        if not isinstance(compiler_path, str) or not compiler_path:
            raise BuildError("controlled compiler identity is malformed")
        command.append("-DCMAKE_CXX_COMPILER=" + compiler_path)
    _run(command, env=env)
    if driver_context is not None:
        _verify_driver_binary(driver_context)
    return _codemodel_configuration(build_tree, configuration)


def _verify_requested_configuration(build_tree: Path, configuration: str) -> None:
    cache = _parse_cmake_cache(build_tree)
    build_type = cache.get("CMAKE_BUILD_TYPE", "")
    configurations = [
        value
        for value in cache.get("CMAKE_CONFIGURATION_TYPES", "").split(";")
        if value
    ]
    if configurations:
        if build_type not in {"", configuration}:
            raise BuildError(
                f"requested configuration {configuration!r} differs from the "
                f"multi-config cache CMAKE_BUILD_TYPE {build_type!r}"
            )
        if configuration not in configurations:
            raise BuildError(
                f"requested configuration {configuration!r} is absent from the "
                f"multi-config generator choices {configurations}"
            )
    elif build_type != configuration:
        # An empty CMAKE_BUILD_TYPE is not Release (or any other requested
        # configuration): accepting it would let the receipt claim flags that
        # were never used to build the artifacts.
        raise BuildError(
            f"requested configuration {configuration!r} differs from "
            f"CMAKE_BUILD_TYPE {build_type!r}"
        )


def _target_artifact_paths(
    build_tree: Path, target: dict[str, object], label: str
) -> list[Path]:
    rows = target.get("artifacts")
    if not isinstance(rows, list) or not rows:
        raise BuildError(f"CMake target {label} declares no artifacts")
    paths: list[Path] = []
    for row in rows:
        if not isinstance(row, dict) or not isinstance(row.get("path"), str):
            raise BuildError(f"CMake target {label} has a malformed artifact")
        path = PurePosixPath(row["path"])
        if path.is_absolute() or ".." in path.parts:
            raise BuildError(f"CMake target {label} has an unsafe artifact path")
        paths.append(build_tree.joinpath(*path.parts).resolve())
    return paths


def _is_local_library_token(token: str, build_tree: Path) -> bool:
    value = token.strip("'\"")
    lower = value.lower()
    extensions = (".a", ".so", ".dylib", ".lib", ".dll")
    if not lower.endswith(extensions):
        return False
    if not ("/" in value or "\\" in value):
        # Bare .lib names on MSVC are normally system import libraries.  A
        # project-local library still resolves inside the build tree below.
        return (build_tree / value).exists()
    candidate = Path(value)
    if candidate.is_absolute():
        system_prefixes = (
            "/usr/lib/",
            "/lib/",
            "/System/Library/",
            "/Applications/Xcode.app/",
        )
        return not any(str(candidate).startswith(prefix) for prefix in system_prefixes)
    return True


def _verify_link_closure(
    *,
    build_tree: Path,
    configuration: str,
    executable_target: str,
    library_target: str,
    library_path: Path,
    cmake_program: str,
    env: dict[str, str] | None = None,
    driver_context: dict[str, object] | None = None,
) -> dict[str, object]:
    expected_library = library_path.resolve()
    _, targets = _ensure_codemodel(
        build_tree, configuration, cmake_program, env=env,
        driver_context=driver_context,
    )
    executable = targets.get(executable_target)
    library = targets.get(library_target)
    if executable is None or library is None:
        raise BuildError(
            "CMake codemodel omits the semantic adapter or its library target"
        )
    if executable.get("type") != "EXECUTABLE":
        raise BuildError("semantic adapter CMake target is not an executable")
    if library.get("type") != "STATIC_LIBRARY":
        raise BuildError("attested libdxfrw CMake target is not a static library")
    link_files = sorted(
        path
        for path in build_tree.rglob("link.txt")
        if path.parent.name == executable_target + ".dir"
    )
    if len(link_files) > 1:
        raise BuildError("multiple semantic adapter link.txt files are ambiguous")
    if link_files:
        link_text = link_files[0].read_text(encoding="utf-8", errors="replace")
        try:
            link_tokens = shlex.split(link_text, posix=os.name != "nt")
        except ValueError as exc:
            raise BuildError(f"cannot parse semantic adapter link command: {exc}") from exc
        library_mentions = 0
        for token in link_tokens:
            candidate = Path(token.strip("'\""))
            resolved = (
                candidate.resolve()
                if candidate.is_absolute()
                else (build_tree / candidate).resolve()
            )
            if resolved == expected_library:
                library_mentions += 1
            elif _is_local_library_token(token, build_tree):
                raise BuildError(
                    f"semantic adapter link has an unattested local library: {token}"
                )
        if library_mentions == 1:
            normalized_link_text = link_text
            for spelling in {
                str(build_tree.resolve()), build_tree.resolve().as_posix(),
            }:
                normalized_link_text = normalized_link_text.replace(
                    spelling, "<build>"
                )
            return {
                "verifiedLinkCommandDigest": _sha256_bytes(
                    normalized_link_text.encode("utf-8")
                ),
                "linkClosureEnforced": True,
                "linkVerificationMethod": "link.txt",
                "linkedLibraryTarget": library_target,
            }
        if library_mentions > 1:
            raise BuildError(
                "semantic adapter link command names the attested library more than once"
            )

    id_to_name: dict[str, str] = {}
    for name, target in targets.items():
        identifier = target.get("id")
        if isinstance(identifier, str):
            id_to_name[identifier] = name
    dependencies = executable.get("dependencies")
    if not isinstance(dependencies, list):
        raise BuildError("semantic adapter CMake target has no dependency metadata")
    dependency_names: list[str] = []
    for row in dependencies:
        if not isinstance(row, dict) or not isinstance(row.get("id"), str):
            raise BuildError("semantic adapter CMake dependency is malformed")
        name = id_to_name.get(row["id"])
        if name is None:
            raise BuildError("semantic adapter CMake dependency is dangling")
        dependency_names.append(name)
    if sorted(dependency_names) != [library_target]:
        raise BuildError(
            "semantic adapter has unexpected CMake target dependencies: "
            f"{sorted(dependency_names)}"
        )

    library_artifacts = _target_artifact_paths(build_tree, library, library_target)
    if library_artifacts != [expected_library]:
        raise BuildError(
            "semantic adapter static library differs from the CMake target artifact: "
            f"{library_artifacts}"
        )
    link = executable.get("link")
    if not isinstance(link, dict) or not isinstance(link.get("commandFragments"), list):
        raise BuildError("semantic adapter CMake target has no link command metadata")
    fragments: list[dict[str, str]] = []
    library_mentions = 0
    for row in link["commandFragments"]:
        if (
            not isinstance(row, dict)
            or not isinstance(row.get("fragment"), str)
            or not isinstance(row.get("role"), str)
        ):
            raise BuildError("semantic adapter link fragment is malformed")
        fragment = row["fragment"]
        role = row["role"]
        fragments.append({"role": role, "fragment": fragment})
        if role != "libraries":
            continue
        try:
            tokens = shlex.split(fragment, posix=os.name != "nt")
        except ValueError as exc:
            raise BuildError(f"cannot parse semantic adapter link fragment: {exc}") from exc
        for token in tokens:
            candidate = Path(token.strip("'\""))
            resolved = (
                candidate.resolve()
                if candidate.is_absolute()
                else (build_tree / candidate).resolve()
            )
            if resolved == expected_library:
                library_mentions += 1
            elif _is_local_library_token(token, build_tree):
                raise BuildError(
                    f"semantic adapter link has an unattested local library: {token}"
                )
    if library_mentions != 1:
        raise BuildError(
            "semantic adapter link metadata does not name the attested library once"
        )

    method = "cmake-file-api-v2"
    digest = _canonical_digest(
        {
            "schema": 1,
            "target": executable_target,
            "dependencies": sorted(dependency_names),
            "libraryArtifact": expected_library.name,
            "fragments": fragments,
        }
    )
    return {
        "verifiedLinkCommandDigest": digest,
        "linkClosureEnforced": True,
        "linkVerificationMethod": method,
        "linkedLibraryTarget": library_target,
    }


def _compile_include_key(path: str, *, msvc_style: bool) -> str:
    if not path or "\x00" in path:
        raise BuildError("semantic adapter has an invalid include path")
    if msvc_style:
        normalized = ntpath.normcase(ntpath.normpath(path))
        if not ntpath.isabs(normalized):
            raise BuildError("semantic adapter include path is not absolute")
        return normalized
    candidate = Path(path)
    if not candidate.is_absolute():
        raise BuildError("semantic adapter include path is not absolute")
    return os.path.normcase(str(candidate.resolve()))


def _validate_adapter_compile_options(
    flag_values: list[str],
    defines_value: list[object],
    *,
    side: str,
    required_defines: set[str],
    msvc_style: bool,
    compiler_id: str,
    allowed_include_paths: set[str],
    require_no_default_config: bool = False,
) -> set[str]:
    """Validate the closed, reviewed compile-option profile for the adapter.

    No downgrade is admitted even if a later flag might reverse it. Critical
    definitions belong only to the codemodel's defines list: a second -D/-U
    channel or opaque forwarding/response channel cannot attest that profile.
    Unrecognized options fail closed: a compiler flag is not presumed harmless
    merely because it does not visibly turn off -Werror.
    """
    if side not in {"target", "standalone"}:
        raise BuildError("semantic adapter compile side is invalid")
    if msvc_style:
        if compiler_id not in {"MSVC", "Clang"}:
            raise BuildError("semantic adapter compiler frontend is unsupported")
    elif compiler_id not in {"GNU", "Clang", "AppleClang"}:
        raise BuildError("semantic adapter compiler family is unsupported")
    known_critical = {
        "LIBDXFRW_SEMANTIC_SIDE_TARGET",
        "LIBDXFRW_SEMANTIC_SIDE_STANDALONE",
        "LIBDXFRW_SEMANTIC_ADAPTER_NAME",
        "LIBDXFRW_SEMANTIC_ADAPTER_PACKAGE",
        "LIBDXFRW_SEMANTIC_ADAPTER_COMMIT",
        "LIBDXFRW_SEMANTIC_ADAPTER_SOURCE_DIGEST",
        "LIBDXFRW_SEMANTIC_ADAPTER_CONFIG_DIGEST",
        "LIBDXFRW_SEMANTIC_ADAPTER_STATIC_LIBRARY_DIGEST",
        "LIBDXFRW_SEMANTIC_ADAPTER_BINARY_DIGEST",
        "LIBDXFRW_SEMANTIC_ADAPTER_LINKED_CLOSURE_DIGEST",
        "LIBDXFRW_SEMANTIC_3D_CONSUMER_PROBE",
    }
    defines: set[str] = set()
    critical: dict[str, str] = {}
    for row in defines_value:
        if not isinstance(row, dict) or not isinstance(row.get("define"), str):
            raise BuildError("semantic adapter compile definition is malformed")
        define = row["define"]
        key = define.partition("=")[0]
        if not re.fullmatch(r"[A-Za-z_]\w*", key) or "\x00" in define or "\n" in define:
            raise BuildError("semantic adapter compile definition is malformed")
        if key.startswith("LIBDXFRW_SEMANTIC_"):
            if key not in known_critical:
                raise BuildError("semantic adapter has an undeclared critical definition")
            if key in critical:
                raise BuildError("semantic adapter has duplicate critical definitions")
            critical[key] = define
        defines.add(define)
    expected_side = "LIBDXFRW_SEMANTIC_SIDE_" + side.upper()
    side_keys = {key for key in critical if key.startswith("LIBDXFRW_SEMANTIC_SIDE_")}
    if side_keys != {expected_side} or critical[expected_side] != expected_side + "=1":
        raise BuildError("semantic adapter compile side definition is not exclusive")
    probe = critical.get("LIBDXFRW_SEMANTIC_3D_CONSUMER_PROBE")
    if probe is not None and probe != "LIBDXFRW_SEMANTIC_3D_CONSUMER_PROBE=1":
        raise BuildError("semantic adapter probe definition has an unsupported value")
    if defines != required_defines or len(defines) != len(defines_value):
        raise BuildError("semantic adapter compile definitions differ from the reviewed profile")

    tokens: list[str] = []
    for index, fragment in enumerate(flag_values):
        tokens.extend(_split_command_fragment(
            fragment, f"adapter compile fragment {index}", reject_ambiguous_quotes=True,
        ))
    # Response expansion precedes ordinary option-operand parsing, including
    # a response token that appears where an include path would otherwise be.
    if any(token.startswith("@") for token in tokens):
        raise BuildError("semantic adapter has an unsupported response-file channel")
    # The adapter profile has no need to override the toolchain, language,
    # output, dependency, sysroot, or linker/assembler behavior. Only the
    # include roots declared by the controlled CMake target are accepted as
    # operand-taking direct options.
    include_options = {"-I", "/I"}

    def require_reviewed_include(path: str) -> None:
        if _compile_include_key(path, msvc_style=msvc_style) not in allowed_include_paths:
            raise BuildError("semantic adapter has an unreviewed include path")

    direct_options: list[str] = []
    index = 0
    while index < len(tokens):
        token = tokens[index]
        if (token.startswith(("@", "-Wp,", "-X", "-include", "--include", "-imacros", "--imacros",
                              "--config", "/FI", "-FI"))
                or token.lower().startswith(("/clang:", "-clang:"))
                or token.partition("=")[0] in {
                    "-Wp", "--for-linker", "--for-assembler",
                }
                or token.lower() in {"/link", "-link"}):
            raise BuildError("semantic adapter has an unsupported opaque compile-option channel")
        if token in include_options:
            if index + 1 >= len(tokens):
                raise BuildError("semantic adapter has an incomplete compile-option operand")
            operand = tokens[index + 1]
            if not operand or operand.startswith(("@", "-")) or "\x00" in operand:
                raise BuildError("semantic adapter has an unsupported include path operand")
            require_reviewed_include(operand)
            index += 2
            continue
        # A definition in the raw flag fragment is allowed only for the
        # standard CMake Release marker. Adapter-specific definitions must be
        # present exactly once in the codemodel's structured defines list.
        define_prefixes = ("-D", "/D")
        undefine_prefixes = ("-U", "/U")
        macro_argument: str | None = None
        macro_option = False
        for prefix in define_prefixes + undefine_prefixes:
            if token == prefix:
                macro_option = True
                if index + 1 >= len(tokens):
                    raise BuildError("semantic adapter has an incomplete definition option")
                macro_argument = tokens[index + 1]
                consumed = 2
                break
            if token.startswith(prefix):
                macro_option = True
                macro_argument = token[len(prefix):]
                consumed = 1
                break
        if macro_option:
            if token.startswith(undefine_prefixes):
                raise BuildError("semantic adapter has an unsupported macro undefinition")
            if macro_argument != "NDEBUG":
                raise BuildError("semantic adapter has an unsupported macro definition")
            index += consumed
            continue
        # Attached include paths are emitted by common CMake generators.
        if token.startswith("-I") and len(token) > 2:
            if token[2:].startswith("@") or "\x00" in token[2:]:
                raise BuildError("semantic adapter has an unsupported include path")
            require_reviewed_include(token[2:])
            index += 1
            continue
        if token.startswith("/I") and len(token) > 2:
            if token[2:].startswith("@") or "\x00" in token[2:]:
                raise BuildError("semantic adapter has an unsupported include path")
            require_reviewed_include(token[2:])
            index += 1
            continue
        direct_options.append(token)
        index += 1

    for token in direct_options:
        if (token.startswith("-Wno-error")
                or token in {"-w", "--no-warnings", "-Wno-everything"}
                or token in {"/WX-", "-WX-", "/w", "/W0", "-W0"}):
            raise BuildError("semantic adapter compile flags downgrade warning enforcement")
        if ((token.lower().startswith("/wx") and token != "/WX")
                or token.startswith("-WX")):
            raise BuildError("semantic adapter has an unsupported warning-error option")
        if token.startswith("-Werror") and token != "-Werror" and not token.startswith("-Werror="):
            raise BuildError("semantic adapter has an unsupported warning-error option")
    if msvc_style:
        if "/WX" not in direct_options or "/bigobj" not in direct_options:
            raise BuildError("MSVC-style semantic adapter lacks exact /WX or /bigobj")
        reviewed_options = {
            "/W3", "/WX", "/bigobj", "/EHsc", "/MD", "/MDd", "/MT", "/MTd",
            "/std:c++17", "/O1", "/O2", "/Od", "/Os", "/Ot",
            "/Ob0", "/Ob1", "/Ob2", "/Ob3", "/Zi", "/Z7",
            "/diagnostics:caret", "/diagnostics:classic",
        }
        if compiler_id == "Clang":
            reviewed_options.add("--no-default-config")
        if any(token not in reviewed_options for token in direct_options):
            raise BuildError("semantic adapter has an unreviewed MSVC-style compile option")
    elif "-Werror" not in direct_options:
        raise BuildError("semantic adapter compile lacks exact global -Werror")
    else:
        reviewed_options = {
            "-Wall", "-Wextra", "-pedantic", "-Werror", "-std=c++17",
            "--no-default-config", "-O0", "-O1", "-O2", "-O3", "-Os", "-Oz", "-Og",
            "-g", "-g0", "-g1", "-g2", "-g3", "-pthread",
            "-fPIC", "-fpic", "-fPIE", "-fpie",
        }
        if any(token not in reviewed_options
               and re.fullmatch(r"-Werror=[A-Za-z0-9_-]+", token) is None
               for token in direct_options):
            raise BuildError("semantic adapter has an unreviewed compile option")
    if require_no_default_config and "--no-default-config" not in direct_options:
        raise BuildError("Clang semantic adapter does not disable default config files")
    if (not msvc_style and compiler_id not in {"Clang", "AppleClang"}
            and "--no-default-config" in direct_options):
        raise BuildError("GNU semantic adapter has a Clang-only compile option")
    return defines


def _compile_contract_profile(
    *,
    compiler_id: str,
    driver_context: dict[str, object] | None,
    controlled_fresh_build: bool,
) -> dict[str, object]:
    controlled = driver_context is not None and controlled_fresh_build
    checks = {
        "directOptions": "pass",
        "driverIdentity": "pass" if driver_context is not None else "unavailable",
        "freshBuild": "pass" if controlled_fresh_build else "unavailable",
        "ambientOverrides": "pass" if controlled else "unavailable",
        "clangDefaultConfig": (
            "pass" if controlled else "unavailable"
        ) if compiler_id in {"Clang", "AppleClang"} else "not-applicable",
        "implicitConfiguration": "partial" if controlled else "unavailable",
        "effectiveOptionPolicy": "pass" if controlled else "pending",
    }
    profile: dict[str, object] = {
        "schema": 1,
        "name": "effective-driver-contract-v1",
        "mode": (
            "controlled-fresh-build-v1"
            if controlled else "existing-build-inspection-v1"
        ),
        "disposition": "advisory" if controlled else "unavailable",
        "checks": checks,
    }
    profile["digest"] = _canonical_digest(profile)
    return profile


def _verify_adapter_compile_contract(
    *,
    build_tree: Path,
    configuration: str,
    executable_target: str,
    adapter_source: Path,
    side: str,
    adapter_name: str,
    adapter_commit: str | None,
    config_digest: str | None,
    cmake_program: str,
    env: dict[str, str] | None = None,
    driver_context: dict[str, object] | None = None,
    controlled_fresh_build: bool = False,
) -> dict[str, object]:
    _, targets = _ensure_codemodel(
        build_tree, configuration, cmake_program, env=env,
        driver_context=driver_context,
    )
    target = targets.get(executable_target)
    if target is None or target.get("type") != "EXECUTABLE":
        raise BuildError("semantic adapter compile target is absent or not executable")
    sources = target.get("sources")
    groups = target.get("compileGroups")
    if not isinstance(sources, list) or not isinstance(groups, list):
        raise BuildError("semantic adapter compile metadata is incomplete")
    compiled_sources: list[Path] = []
    referenced_groups: set[int] = set()
    cache = _parse_cmake_cache(build_tree)
    source_root_text = cache.get("CMAKE_HOME_DIRECTORY", "")
    if not source_root_text:
        raise BuildError("CMake cache omits CMAKE_HOME_DIRECTORY")
    source_root = Path(source_root_text)
    for row in sources:
        if not isinstance(row, dict) or not isinstance(row.get("path"), str):
            raise BuildError("semantic adapter source metadata is malformed")
        group_index = row.get("compileGroupIndex")
        if isinstance(group_index, bool) or not isinstance(group_index, int):
            continue
        source_path = Path(row["path"])
        if not source_path.is_absolute():
            source_path = source_root / source_path
        compiled_sources.append(source_path.resolve())
        referenced_groups.add(group_index)
    if compiled_sources != [adapter_source.resolve()]:
        raise BuildError(
            "semantic adapter target was not compiled from the attested source"
        )
    if len(referenced_groups) != 1:
        raise BuildError("semantic adapter must use exactly one compile group")
    group_index = next(iter(referenced_groups))
    if group_index < 0 or group_index >= len(groups):
        raise BuildError("semantic adapter compile-group reference is invalid")
    group = groups[group_index]
    if not isinstance(group, dict) or group.get("language") != "CXX":
        raise BuildError("semantic adapter compile group is not C++")
    standard = group.get("languageStandard")
    if not isinstance(standard, dict) or standard.get("standard") != "17":
        raise BuildError("semantic adapter compile group is not C++17")
    fragments = group.get("compileCommandFragments")
    if not isinstance(fragments, list):
        raise BuildError("semantic adapter compile flags are absent")
    if any(not isinstance(row, dict) or not isinstance(row.get("fragment"), str)
           for row in fragments):
        raise BuildError("semantic adapter compile fragment is malformed")
    flag_values = [row["fragment"] for row in fragments]
    cmake_identity = _cmake_metadata(
        build_tree, cmake_program, configuration,
        env=env, driver_context=driver_context,
    )
    msvc_style = (
        cmake_identity["compilerId"] == "MSVC"
        or cmake_identity["compilerFrontendVariant"] == "MSVC"
    )
    if side == "target":
        target_root_text = cache.get("LIBDXFRW_SEMANTIC_TARGET_ROOT", "")
        if not target_root_text:
            raise BuildError("semantic adapter target cache omits its source root")
        include_root = Path(target_root_text).resolve()
    else:
        include_root = source_root.resolve()
    expected_include_paths = [
        (include_root / "src").resolve(),
        (include_root / "src/intern").resolve(),
    ]
    include_rows = group.get("includes")
    if not isinstance(include_rows, list):
        raise BuildError("semantic adapter compile include metadata is absent")
    actual_include_paths: list[Path] = []
    for row in include_rows:
        if (not isinstance(row, dict) or not isinstance(row.get("path"), str)
                or row.get("isSystem", False) is not False):
            raise BuildError("semantic adapter has an unreviewed include metadata row")
        include_path = Path(row["path"]).resolve()
        actual_include_paths.append(include_path)
    if actual_include_paths != expected_include_paths:
        raise BuildError("semantic adapter include roots differ from the reviewed profile")
    allowed_include_paths = {
        _compile_include_key(str(path), msvc_style=msvc_style)
        for path in expected_include_paths
    }
    defines_value = group.get("defines")
    if not isinstance(defines_value, list):
        raise BuildError("semantic adapter compile definitions are absent")
    side_macro = (
        "LIBDXFRW_SEMANTIC_SIDE_TARGET=1"
        if side == "target"
        else "LIBDXFRW_SEMANTIC_SIDE_STANDALONE=1"
    )
    required_defines = {
        side_macro,
        f'LIBDXFRW_SEMANTIC_ADAPTER_NAME="{adapter_name}"',
    }
    if adapter_commit is not None:
        required_defines.add(
            f'LIBDXFRW_SEMANTIC_ADAPTER_COMMIT="{adapter_commit}"'
        )
    if config_digest is not None:
        required_defines.add(
            f'LIBDXFRW_SEMANTIC_ADAPTER_CONFIG_DIGEST="{config_digest}"'
        )
    defines = _validate_adapter_compile_options(
        flag_values, defines_value, side=side,
        required_defines=required_defines, msvc_style=msvc_style,
        compiler_id=str(cmake_identity["compilerId"]),
        allowed_include_paths=allowed_include_paths,
        require_no_default_config=(
            driver_context is not None and controlled_fresh_build
            and cmake_identity["compilerId"] in {"Clang", "AppleClang"}
        ),
    )
    contract_profile = _compile_contract_profile(
        compiler_id=str(cmake_identity["compilerId"]),
        driver_context=driver_context,
        controlled_fresh_build=controlled_fresh_build,
    )
    identity = {
        "schema": 1,
        "sourceDigest": _sha256_file(adapter_source),
        "language": "CXX",
        "standard": "17",
        "flags": flag_values,
        "defines": sorted(defines),
    }
    return {
        "verifiedCompileCommandDigest": _canonical_digest(identity),
        # CMake metadata proves the declared direct options only.  It does
        # not bind ambient compiler edits, default configs, or launchers, so
        # new receipts must not call this an effective compiler contract.
        "compileContractMode": "direct-options-only",
        "compileOptionAdmissionEnforced": True,
        "compileContractEnforced": False,
        "compileContractProfile": contract_profile,
    }


def _without_cpp_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", " ", text, flags=re.DOTALL)
    return re.sub(r"//[^\r\n]*", " ", text)


def _class_body(text: str, class_name: str) -> str:
    cleaned = _without_cpp_comments(text)
    declaration = re.search(
        r"\bclass\s+" + re.escape(class_name) + r"\b[^;{]*\{", cleaned
    )
    if declaration is None:
        raise BuildError(f"cannot find class {class_name} for interface inventory")
    opening = declaration.end() - 1
    depth = 0
    for index in range(opening, len(cleaned)):
        if cleaned[index] == "{":
            depth += 1
        elif cleaned[index] == "}":
            depth -= 1
            if depth == 0:
                return cleaned[opening + 1:index]
    raise BuildError(f"class {class_name} has an unterminated body")


def _interface_virtual_methods(interface_header: Path) -> list[str]:
    body = _class_body(
        _require_file(interface_header, "DRW_Interface header").read_text(
            encoding="utf-8", errors="strict"
        ),
        "DRW_Interface",
    )
    declarations = re.findall(
        r"\bvirtual\b(?:(?![;{}]).)*?\b([A-Za-z_]\w*)\s*\(([^(){};]*)\)",
        body,
        flags=re.DOTALL,
    )
    methods: list[str] = []
    overloads: dict[str, set[str]] = {}
    for name, parameters in declarations:
        if name == "DRW_Interface":
            continue
        methods.append(name)
        normalized = re.sub(r"\s+", " ", parameters.strip())
        if normalized:
            # Parameter names do not affect the overload contract. The known
            # overloads each have one parameter; reject unfamiliar parameter
            # lists instead of attempting to guess nested/template syntax.
            if "," not in normalized:
                normalized = re.sub(
                    r"\s*(?P<ref>&&|&|\*+)\s*[A-Za-z_]\w*\s*$",
                    r"\g<ref>",
                    normalized,
                )
                normalized = re.sub(r"\s*(&&|&|\*+)\s*", r"\1", normalized)
        overloads.setdefault(name, set()).add(normalized)
    method_names = sorted(set(methods))
    repeated = {name for name in methods if methods.count(name) > 1}
    if not method_names:
        raise BuildError(
            "DRW_Interface callback inventory is empty"
        )
    unexpected_overloads = {
        name: signatures for name, signatures in overloads.items()
        if name in repeated
        and (
            KNOWN_INTERFACE_OVERLOADS.get(name) != frozenset(signatures)
            or methods.count(name) != len(KNOWN_INTERFACE_OVERLOADS[name])
        )
    }
    if unexpected_overloads:
        raise BuildError(
            "DRW_Interface callback inventory contains an unreviewed overload; "
            f"update KNOWN_INTERFACE_OVERLOADS explicitly ({unexpected_overloads})"
        )
    return method_names


def _active_interface_class_body(body: str, side: str | None) -> str:
    lines: list[str] = []
    frames: list[tuple[bool, bool, bool]] = []
    active = True
    for line in body.splitlines():
        directive = re.match(r"^\s*#\s*(\w+)(?:\s+(.*?))?\s*$", line)
        if directive is None:
            if active:
                lines.append(line)
            continue
        command, argument = directive.group(1), directive.group(2) or ""
        if command == "ifdef" and argument in {
            "LIBDXFRW_SEMANTIC_SIDE_TARGET",
            "LIBDXFRW_SEMANTIC_SIDE_STANDALONE",
        }:
            if side not in {"target", "standalone"}:
                raise BuildError(
                    "semantic adapter callback inventory needs a named side "
                    "to evaluate conditional supplements"
                )
            condition = argument == (
                "LIBDXFRW_SEMANTIC_SIDE_" + side.upper()
            )
            frames.append((active, condition, False))
            active = active and condition
        elif command == "else" and not argument:
            if not frames or frames[-1][2]:
                raise BuildError("semantic adapter callback conditional is malformed")
            parent_active, condition, _ = frames[-1]
            frames[-1] = (parent_active, condition, True)
            active = parent_active and not condition
        elif command == "endif" and not argument:
            if not frames:
                raise BuildError("semantic adapter callback conditional is unmatched")
            parent_active, _, _ = frames.pop()
            active = parent_active
        elif command in {"define", "undef"}:
            if active:
                lines.append(line)
        else:
            raise BuildError(
                "semantic adapter callback inventory contains an unsupported "
                "preprocessor directive"
            )
    if frames:
        raise BuildError("semantic adapter callback conditional is unterminated")
    return "\n".join(lines)


def _adapter_override_methods(
    adapter_source: Path, *, side: str | None = None
) -> list[str]:
    text = _require_file(adapter_source, "semantic adapter source").read_text(
        encoding="utf-8", errors="strict"
    )
    body = _class_body(text, "SemanticSink")
    body = _active_interface_class_body(body, side)
    explicit = re.findall(
        r"^\s*(?:[A-Za-z_:][A-Za-z0-9_:<>,*&\s]*)\s+"
        r"([A-Za-z_]\w*)\s*\([^;{}]*\)\s*(?:const\s*)?override\b",
        body,
        flags=re.MULTILINE | re.DOTALL,
    )
    explicit = [name for name in explicit if name != "method"]
    macro = re.findall(
        r"^\s*SEMANTIC_OPAQUE_(?:POINTER|VALUE)\(\s*([A-Za-z_]\w*)\s*,",
        body,
        flags=re.MULTILINE,
    )
    methods = explicit + macro
    if len(methods) != len(set(methods)):
        raise BuildError(
            "semantic adapter override inventory contains duplicates"
        )
    return sorted(methods)


def _verify_interface_contract(
    interface_header: Path,
    adapter_source: Path,
    *,
    side: str | None = None,
    profile: str | None = None,
) -> dict[str, object]:
    interface_methods = _interface_virtual_methods(interface_header)
    adapter_methods = _adapter_override_methods(adapter_source, side=side)
    if profile == INTERFACE_PROFILE_NAME:
        if side not in {"target", "standalone"}:
            raise BuildError("named interface profile requires a build side")
        extras = (
            list(SUPPLEMENTAL_INTERFACE_METHODS)
            if side == "standalone" else []
        )
        common_methods = [
            method for method in interface_methods if method not in extras
        ]
        expected_interface_count = (
            COMMON_INTERFACE_METHOD_COUNT + len(extras)
        )
        if len(interface_methods) != expected_interface_count:
            raise BuildError(
                f"{profile} {side} interface has {len(interface_methods)} "
                f"callbacks; expected {expected_interface_count}"
            )
        common_digest = _canonical_digest({
            "schema": 1,
            "methods": common_methods,
        })
        if (len(common_methods) != COMMON_INTERFACE_METHOD_COUNT
                or common_digest != COMMON_INTERFACE_METHOD_DIGEST):
            raise BuildError(
                f"{profile} {side} common callback inventory is not the "
                "reviewed common170 surface"
            )
        if adapter_methods != interface_methods:
            missing = sorted(set(interface_methods) - set(adapter_methods))
            extra = sorted(set(adapter_methods) - set(interface_methods))
            raise BuildError(
                f"{profile} {side} adapter does not cover its exact "
                "declared interface "
                f"(missing={missing}, extra={extra})"
            )
        full_digest = _canonical_digest({
            "schema": 1,
            "methods": interface_methods,
        })
        return {
            "verifiedInterfaceContractDigest": common_digest,
            "interfaceContractEnforced": True,
            "interfaceMethodCount": COMMON_INTERFACE_METHOD_COUNT,
            "interfaceProfile": {
                "schema": 1,
                "name": profile,
                "side": side,
                "commonMethodCount": COMMON_INTERFACE_METHOD_COUNT,
                "commonMethodDigest": common_digest,
                "interfaceMethodCount": len(interface_methods),
                "interfaceMethodDigest": full_digest,
                "defaultedInterfaceExtras": extras,
                "adapterSupplementalMethods": sorted(
                    set(adapter_methods) & set(extras)
                ),
                "unobservedDefaultedExtras": sorted(
                    set(extras) - set(adapter_methods)
                ),
            },
        }
    if profile not in {None, "self-test-equal-v1"}:
        raise BuildError(f"unsupported interface inventory profile: {profile}")
    if interface_methods != adapter_methods:
        missing = sorted(set(interface_methods) - set(adapter_methods))
        extra = sorted(set(adapter_methods) - set(interface_methods))
        raise BuildError(
            "semantic adapter does not cover the complete DRW_Interface callback "
            f"surface (missing={missing}, extra={extra})"
        )
    identity = {"schema": 1, "methods": interface_methods}
    return {
        "verifiedInterfaceContractDigest": _canonical_digest(identity),
        "interfaceContractEnforced": True,
        "interfaceMethodCount": len(interface_methods),
    }


def _self_test_named_interface_profile() -> None:
    current_header = REPOSITORY_ROOT / "src/drw_interface.h"
    current_methods = _interface_virtual_methods(current_header)
    common_methods = [
        method for method in current_methods
        if method not in SUPPLEMENTAL_INTERFACE_METHODS
    ]
    if _canonical_digest({"schema": 1, "methods": common_methods}) != (
        COMMON_INTERFACE_METHOD_DIGEST
    ):
        raise BuildError("self-test current interface no longer matches common170")

    standalone = _verify_interface_contract(
        current_header,
        DEFAULT_ADAPTER,
        side="standalone",
        profile=INTERFACE_PROFILE_NAME,
    )
    standalone_profile = standalone["interfaceProfile"]
    if not isinstance(standalone_profile, dict):
        raise BuildError("self-test named profile omitted its inventory")
    if (standalone_profile["interfaceMethodCount"] != 176
            or standalone_profile["defaultedInterfaceExtras"]
                != list(SUPPLEMENTAL_INTERFACE_METHODS)
            or standalone_profile["adapterSupplementalMethods"]
                != list(SUPPLEMENTAL_INTERFACE_METHODS)
            or standalone_profile["unobservedDefaultedExtras"] != []):
        raise BuildError("self-test did not report the captured callback bridge")

    with tempfile.TemporaryDirectory(prefix="semantic-interface-profile-test-") as tmp:
        root = Path(tmp)

        def write_header(path: Path, methods: list[str]) -> None:
            path.write_text(
                "class DRW_Interface {\npublic:\n"
                "    virtual ~DRW_Interface() = default;\n"
                + "".join(f"    virtual void {name}();\n" for name in methods)
                + "};\n",
                encoding="utf-8",
            )

        unreviewed_overload = root / "unreviewed-overload.h"
        unreviewed_overload.write_text(
            "class DRW_Interface {\npublic:\n"
            "    virtual void addLine();\n"
            "    virtual void addLine(int value);\n"
            "};\n",
            encoding="utf-8",
        )
        try:
            _interface_virtual_methods(unreviewed_overload)
        except BuildError as exc:
            if "unreviewed overload" not in str(exc):
                raise
        else:
            raise BuildError(
                "self-test accepted an unreviewed overloaded callback"
            )

        changed_known_overload = root / "changed-known-overload.h"
        changed_known_overload.write_text(
            "class DRW_Interface {\npublic:\n"
            "    virtual void addDataStorage(const DRW_DataStorageSection &data);\n"
            "    virtual void addDataStorage(DRW_DataStorageSection &&data);\n"
            "    virtual void addDataStorage(int data);\n"
            "};\n",
            encoding="utf-8",
        )
        try:
            _interface_virtual_methods(changed_known_overload)
        except BuildError as exc:
            if "unreviewed overload" not in str(exc):
                raise
        else:
            raise BuildError(
                "self-test accepted a changed ownership-transfer overload set"
            )

        duplicate_known_overload = root / "duplicate-known-overload.h"
        duplicate_known_overload.write_text(
            "class DRW_Interface {\npublic:\n"
            "    virtual void addDataStorage(const DRW_DataStorageSection &first);\n"
            "    virtual void addDataStorage(const DRW_DataStorageSection &second);\n"
            "    virtual void addDataStorage(DRW_DataStorageSection &&moved);\n"
            "};\n",
            encoding="utf-8",
        )
        try:
            _interface_virtual_methods(duplicate_known_overload)
        except BuildError as exc:
            if "unreviewed overload" not in str(exc):
                raise
        else:
            raise BuildError(
                "self-test accepted a duplicate overload declaration"
            )

        target_header = root / "target-interface.h"
        write_header(target_header, common_methods)
        target = _verify_interface_contract(
            target_header,
            DEFAULT_ADAPTER,
            side="target",
            profile=INTERFACE_PROFILE_NAME,
        )
        target_profile = target["interfaceProfile"]
        if not isinstance(target_profile, dict) or (
            target_profile["interfaceMethodCount"] != 170
            or target_profile["defaultedInterfaceExtras"] != []
            or target_profile["unobservedDefaultedExtras"] != []
        ):
            raise BuildError("self-test target profile inventory is stale")

        standalone_header = root / "standalone-interface.h"
        all_methods = sorted(common_methods + list(SUPPLEMENTAL_INTERFACE_METHODS))
        write_header(standalone_header, all_methods)

        missing_common = root / "missing-common.h"
        write_header(missing_common, common_methods[:-1])
        try:
            _verify_interface_contract(
                missing_common,
                DEFAULT_ADAPTER,
                side="target",
                profile=INTERFACE_PROFILE_NAME,
            )
        except BuildError as exc:
            if "expected 170" not in str(exc):
                raise
        else:
            raise BuildError("self-test accepted an incomplete common interface")

        unexpected = root / "unexpected-target.h"
        write_header(unexpected, common_methods + ["unreviewedCallback"])
        try:
            _verify_interface_contract(
                unexpected,
                DEFAULT_ADAPTER,
                side="target",
                profile=INTERFACE_PROFILE_NAME,
            )
        except BuildError as exc:
            if "expected 170" not in str(exc):
                raise
        else:
            raise BuildError("self-test accepted an unreviewed target callback")

        missing_supplement = root / "missing-supplement.h"
        write_header(
            missing_supplement,
            sorted(common_methods + list(SUPPLEMENTAL_INTERFACE_METHODS[:-1])),
        )
        try:
            _verify_interface_contract(
                missing_supplement,
                DEFAULT_ADAPTER,
                side="standalone",
                profile=INTERFACE_PROFILE_NAME,
            )
        except BuildError as exc:
            if "expected 176" not in str(exc):
                raise
        else:
            raise BuildError("self-test accepted a stale standalone callback set")


def _self_test_capability_profile() -> None:
    for side in ("target", "standalone"):
        expected = _expected_capability_profile(side)
        _assert_equal(
            _validate_capability_profile(expected, side),
            expected,
            side + " capability profile",
        )
        mutations: list[tuple[str, dict[str, object]]] = []
        changed = dict(expected)
        changed["schema"] = True
        mutations.append(("boolean schema", changed))
        changed = dict(expected)
        changed["name"] = "unreviewed-capability-profile"
        mutations.append(("profile name", changed))
        changed = dict(expected)
        changed["side"] = "standalone" if side == "target" else "target"
        mutations.append(("wrong side", changed))
        changed = dict(expected)
        changed.pop("side")
        mutations.append(("missing top-level field", changed))
        changed = dict(expected)
        changed["unknown"] = False
        mutations.append(("unknown top-level field", changed))
        changed = dict(expected)
        capabilities = dict(expected["capabilities"])
        capabilities[CAPABILITY_NAMES[0]] = not capabilities[CAPABILITY_NAMES[0]]
        changed["capabilities"] = capabilities
        mutations.append(("changed capability", changed))
        changed = dict(expected)
        capabilities = dict(expected["capabilities"])
        capabilities[CAPABILITY_NAMES[0]] = 1
        changed["capabilities"] = capabilities
        mutations.append(("non-boolean capability", changed))
        changed = dict(expected)
        capabilities = dict(expected["capabilities"])
        capabilities.pop(CAPABILITY_NAMES[0])
        changed["capabilities"] = capabilities
        mutations.append(("missing capability", changed))
        changed = dict(expected)
        capabilities = dict(expected["capabilities"])
        capabilities["unreviewedCapability"] = False
        changed["capabilities"] = capabilities
        mutations.append(("unknown capability", changed))
        for label, value in mutations:
            try:
                _validate_capability_profile(value, side, "self-test profile")
            except BuildError:
                pass
            else:
                raise BuildError(
                    f"self-test accepted invalid {side} capability profile: {label}"
                )


def _split_command_fragment(fragment: str, label: str, *,
                            reject_ambiguous_quotes: bool = False) -> list[str]:
    try:
        tokens = shlex.split(fragment, posix=os.name != "nt")
    except ValueError as exc:
        raise BuildError(f"cannot parse {label}: {exc}") from exc
    # shlex's non-POSIX mode is not a Windows compiler command-line parser:
    # joined quoted /D or /I operands containing spaces can be split apart.
    # Decline that ambiguity and escaped-quote forms for compile attestation,
    # before stripping quotes; a backslash-quote may be literal on Windows.
    # Existing identity/profile callers retain their historical tokenization.
    if (reject_ambiguous_quotes and os.name == "nt"
            and (re.search(r'\\+"', fragment)
                 or any(token.count('"') % 2 for token in tokens))):
        raise BuildError(f"cannot attest ambiguous Windows quoting in {label}")
    return [token.strip('"') if os.name == "nt" else token for token in tokens]


def _is_diagnostic_compile_flag(token: str) -> bool:
    if token in {"-pedantic", "-pedantic-errors"}:
        return True
    if token.startswith("-W") and not token.startswith(("-Wa,", "-Wl,", "-Wp,")):
        return True
    lower = token.lower()
    if lower in {"/w0", "/w1", "/w2", "/w3", "/w4", "/wall", "/wx", "/wx-"}:
        return True
    return lower.startswith(("/wd", "/we", "/wo", "/external:w"))


def _normalize_profile_text(
    value: str, *, build_tree: Path, package_root: Path
) -> str:
    result = value
    replacements = (
        (build_tree.resolve(), "<build>"),
        (package_root.resolve(), "<package>"),
    )
    for root, marker in replacements:
        for spelling in {str(root), root.as_posix()}:
            result = result.replace(spelling, marker)
    return result.replace("\\", "/")


def _target_source_root(build_tree: Path) -> Path:
    cache = _parse_cmake_cache(build_tree)
    value = cache.get("CMAKE_HOME_DIRECTORY", "")
    if not value:
        raise BuildError("CMake cache omits CMAKE_HOME_DIRECTORY")
    return Path(value).resolve()


def _compiled_source_paths(
    build_tree: Path, target: dict[str, object], label: str
) -> tuple[list[Path], set[int]]:
    sources = target.get("sources")
    if not isinstance(sources, list):
        raise BuildError(f"CMake target {label} has no source metadata")
    source_root = _target_source_root(build_tree)
    compiled: list[Path] = []
    groups: set[int] = set()
    for row in sources:
        if not isinstance(row, dict) or not isinstance(row.get("path"), str):
            raise BuildError(f"CMake target {label} has a malformed source")
        group_index = row.get("compileGroupIndex")
        if isinstance(group_index, bool) or not isinstance(group_index, int):
            continue
        path = Path(row["path"])
        if not path.is_absolute():
            path = source_root / path
        compiled.append(path.resolve())
        groups.add(group_index)
    if not compiled:
        raise BuildError(f"CMake target {label} has no compiled sources")
    return compiled, groups


def _parse_make_dependency_file(path: Path) -> list[str]:
    """Read the single dependency rule emitted by GCC/Clang -M.

    The controlled profile currently uses this only on POSIX hosts; on Windows
    the builder records dependency capture as unavailable rather than guessing
    at Makefile escaping for drive-letter/backslash paths.
    """
    try:
        text = path.read_text(encoding="utf-8")
    except (OSError, UnicodeError) as exc:
        raise BuildError(f"cannot read compiler dependency output: {exc}") from exc
    if not text or "\x00" in text:
        raise BuildError("compiler dependency output is empty or malformed")
    text = text.replace("\\\r\n", " ").replace("\\\n", " ")
    separator = re.search(r":\s", text)
    if separator is None:
        raise BuildError("compiler dependency output omits its rule separator")
    words: list[str] = []
    current: list[str] = []
    escaped = False
    for char in text[separator.end():]:
        if escaped:
            current.append(char)
            escaped = False
        elif char == "\\":
            escaped = True
        elif char.isspace():
            if current:
                words.append("".join(current))
                current = []
        else:
            current.append(char)
    if escaped:
        raise BuildError("compiler dependency output ends in an escape")
    if current:
        words.append("".join(current))
    if not words:
        raise BuildError("compiler dependency output contains no inputs")
    return words


def _dependency_scan_argv(
    command: list[str], *, source: Path, cwd: Path, dependency_file: Path,
    compiler_path: Path,
) -> list[str]:
    if not command:
        raise BuildError("compile database contains an empty compiler command")
    invoked = Path(command[0])
    if not invoked.is_absolute():
        raise BuildError("compile database compiler path is not absolute")
    if invoked.resolve() != compiler_path.resolve():
        raise BuildError("compile database compiler differs from configured compiler")

    retained = [command[0]]
    source_occurrences = 0
    index = 1
    while index < len(command):
        token = command[index]
        if token.startswith("@"):
            raise BuildError("opaque compiler response file prevents dependency capture")
        if token in {"-c", "-MD", "-MMD", "-MP"}:
            index += 1
            continue
        if token in {"-o", "-MF", "-MT", "-MQ"}:
            if index + 1 >= len(command):
                raise BuildError("compile database has an incomplete output option")
            index += 2
            continue
        if token.startswith(("-o", "-MF", "-MT", "-MQ")) and len(token) > 2:
            index += 1
            continue
        candidate = Path(token)
        if not candidate.is_absolute():
            candidate = cwd / candidate
        if candidate.resolve() == source.resolve():
            source_occurrences += 1
            index += 1
            continue
        retained.append(token)
        index += 1
    if source_occurrences != 1:
        raise BuildError("compile database source does not occur exactly once")
    retained.extend(["-M", "-MF", str(dependency_file), "-MT", "semantic-dependency-probe",
                     str(source)])
    return retained


def _compiler_dependency_profile(
    *,
    build_tree: Path,
    configuration: str,
    cmake_program: str,
    cmake_data: dict[str, object],
    executable_target: str,
    library_target: str,
    env: dict[str, str] | None,
    driver_context: dict[str, object] | None,
) -> dict[str, object]:
    """Hash external header inputs reached by the target's actual compile calls.

    This is deliberately a partial compiler-input profile: it binds file
    dependencies, but not compiler resource binaries, SDK metadata, or dynamic
    linker inputs. Those remain separate gates in the plan.
    """
    unavailable = {
        "schema": 1,
        "status": "unavailable",
        "method": "",
        "translationUnitCount": 0,
        "externalInputs": [],
        "externalInputsDigest": _canonical_digest([]),
        "reason": "",
    }
    if os.name == "nt":
        unavailable["reason"] = "windows-depfile-grammar-not-qualified"
        return unavailable
    if cmake_data.get("multiConfig"):
        unavailable["reason"] = "multi-config-compile-database-not-qualified"
        return unavailable
    compiler_id = cmake_data.get("compilerId")
    if compiler_id not in {"GNU", "Clang", "AppleClang"}:
        unavailable["reason"] = "compiler-family-not-qualified"
        return unavailable

    database = build_tree / "compile_commands.json"
    if not database.is_file():
        unavailable["reason"] = "compile-database-not-generated"
        return unavailable
    try:
        rows = json.loads(database.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise BuildError(f"cannot read CMake compile database: {exc}") from exc
    if not isinstance(rows, list):
        raise BuildError("CMake compile database is not an array")

    _, targets = _ensure_codemodel(
        build_tree, configuration, cmake_program, env=env,
        driver_context=driver_context,
    )
    executable = targets.get(executable_target)
    library = targets.get(library_target)
    if executable is None or library is None:
        raise BuildError("CMake codemodel omits a dependency-profile target")
    package_root = _package_root_from_library(build_tree, library, library_target)
    expected_sources: set[Path] = set()
    for target, label in ((executable, executable_target), (library, library_target)):
        sources, _ = _compiled_source_paths(build_tree, target, label)
        expected_sources.update(path.resolve() for path in sources)
    if not expected_sources:
        raise BuildError("dependency profile has no compilation units")

    commands: dict[Path, tuple[list[str], Path]] = {}
    for index, row in enumerate(rows):
        if not isinstance(row, dict):
            raise BuildError(f"compile database row {index} is malformed")
        directory = row.get("directory")
        filename = row.get("file")
        if not isinstance(directory, str) or not isinstance(filename, str):
            raise BuildError(f"compile database row {index} omits directory/file")
        cwd = Path(directory)
        if not cwd.is_absolute():
            raise BuildError(f"compile database row {index} has a relative directory")
        source = Path(filename)
        if not source.is_absolute():
            source = cwd / source
        source = source.resolve()
        if source not in expected_sources:
            continue
        arguments = row.get("arguments")
        command = row.get("command")
        if isinstance(arguments, list) and all(isinstance(item, str) for item in arguments):
            argv = list(arguments)
        elif isinstance(command, str):
            try:
                argv = shlex.split(command, posix=True)
            except ValueError as exc:
                raise BuildError(
                    f"cannot parse compile database command {index}: {exc}"
                ) from exc
        else:
            raise BuildError(f"compile database row {index} has no usable command")
        if source in commands:
            raise BuildError("compile database repeats a qualification translation unit")
        commands[source] = (argv, cwd.resolve())
    if set(commands) != expected_sources:
        missing = len(expected_sources - set(commands))
        raise BuildError(
            f"compile database covers {len(commands)} of {len(expected_sources)} "
            f"qualification translation units ({missing} missing)"
        )

    compiler_path_value = cmake_data.get("compilerPath")
    if not isinstance(compiler_path_value, str) or not compiler_path_value:
        raise BuildError("CMake metadata omits the dependency-scan compiler")
    compiler_path = Path(compiler_path_value)
    external: dict[str, dict[str, object]] = {}
    with tempfile.TemporaryDirectory(prefix="semantic-dependency-scan-") as temp:
        temp_root = Path(temp)
        for index, source in enumerate(sorted(expected_sources)):
            argv, cwd = commands[source]
            dependency_file = temp_root / f"{index}.d"
            scan = _dependency_scan_argv(
                argv, source=source, cwd=cwd, dependency_file=dependency_file,
                compiler_path=compiler_path,
            )
            if driver_context is not None:
                _verify_driver_binary(driver_context)
            _run(scan, cwd=cwd, env=env)
            for dependency in _parse_make_dependency_file(dependency_file):
                path = Path(dependency)
                if not path.is_absolute():
                    path = cwd / path
                path = path.resolve()
                if path == source:
                    continue
                try:
                    path.relative_to(package_root.resolve())
                    continue
                except ValueError:
                    pass
                try:
                    path.relative_to(build_tree.resolve())
                    continue
                except ValueError:
                    pass
                if not path.is_file():
                    raise BuildError(
                        "compiler dependency is not a regular file: "
                        + path.as_posix()
                    )
                external[path.as_posix()] = {
                    "path": path.as_posix(),
                    "size": path.stat().st_size,
                    "digest": _sha256_file(path),
                }
    files = [external[key] for key in sorted(external)]
    return {
        "schema": 1,
        "status": "captured",
        "method": "gnu-clang-m-dependency-scan-v1",
        "translationUnitCount": len(expected_sources),
        "externalInputs": files,
        "externalInputsDigest": _canonical_digest(files),
        "reason": "",
    }


def _directory_tree_identity(root: Path) -> dict[str, object]:
    """Return a bounded, deterministic content identity for a tool directory."""
    root = _require_directory(root, "compiler resource directory")
    entries: list[dict[str, object]] = [{
        "path": ".", "kind": "directory", "mode": root.stat().st_mode & 0o777,
    }]
    file_count = 0
    total_bytes = 0
    for current_text, directory_names, file_names in os.walk(
        root, topdown=True, followlinks=False
    ):
        current = Path(current_text)
        directory_names.sort()
        file_names.sort()
        descend: list[str] = []
        for name in directory_names:
            path = current / name
            relative = path.relative_to(root).as_posix()
            if path.is_symlink():
                entries.append({
                    "path": relative, "kind": "symlink",
                    "target": os.readlink(path),
                })
            else:
                entries.append({
                    "path": relative, "kind": "directory",
                    "mode": path.stat().st_mode & 0o777,
                })
                descend.append(name)
        directory_names[:] = descend
        for name in file_names:
            path = current / name
            relative = path.relative_to(root).as_posix()
            if path.is_symlink():
                entries.append({
                    "path": relative, "kind": "symlink",
                    "target": os.readlink(path),
                })
                continue
            before = path.stat()
            if not path.is_file():
                raise BuildError(
                    "compiler resource tree contains a non-file entry: "
                    + path.as_posix()
                )
            file_digest = _sha256_file(path)
            after = path.stat()
            if ((before.st_size, before.st_mtime_ns, before.st_ino)
                    != (after.st_size, after.st_mtime_ns, after.st_ino)):
                raise BuildError("compiler resource changed while being hashed")
            file_count += 1
            total_bytes += after.st_size
            entries.append({
                "path": relative, "kind": "file", "mode": after.st_mode & 0o777,
                "size": after.st_size, "digest": file_digest,
            })
            if (len(entries) > MAX_COMPILER_RESOURCE_ENTRIES
                    or total_bytes > MAX_COMPILER_RESOURCE_BYTES):
                raise BuildError("compiler resource tree exceeds the capture budget")
        if len(entries) > MAX_COMPILER_RESOURCE_ENTRIES:
            raise BuildError("compiler resource tree exceeds the capture budget")
    return {
        "entryCount": len(entries),
        "fileCount": file_count,
        "totalBytes": total_bytes,
        "treeDigest": _canonical_digest(entries),
    }


def _compiler_resource_profile(
    cmake_data: dict[str, object], *, env: dict[str, str] | None
) -> dict[str, object]:
    """Bind Clang's selected builtin resource directory when it can be read."""
    compiler_id = cmake_data.get("compilerId")
    unavailable = {
        "schema": 1,
        "status": "unavailable",
        "method": "",
        "compilerId": compiler_id if isinstance(compiler_id, str) else "",
        "path": "",
        "entryCount": 0,
        "fileCount": 0,
        "totalBytes": 0,
        "treeDigest": _canonical_digest([]),
        "reason": "compiler-family-not-qualified",
    }
    if os.name == "nt" or compiler_id not in {"Clang", "AppleClang"}:
        return unavailable
    compiler_path_value = cmake_data.get("compilerPath")
    if not isinstance(compiler_path_value, str) or not compiler_path_value:
        unavailable["reason"] = "compiler-path-unavailable"
        return unavailable
    query = [compiler_path_value, "--no-default-config", "-print-resource-dir"]
    try:
        output = _run(query, env=env).strip()
    except BuildError:
        unavailable["reason"] = "clang-resource-query-failed"
        return unavailable
    if not output or "\n" in output or "\r" in output:
        unavailable["reason"] = "clang-resource-query-malformed"
        return unavailable
    resource_path = Path(output)
    if not resource_path.is_absolute():
        unavailable["reason"] = "clang-resource-path-not-absolute"
        return unavailable
    try:
        resource_path = _require_directory(resource_path, "Clang resource directory")
        tree = _directory_tree_identity(resource_path)
    except BuildError as exc:
        unavailable["reason"] = (
            "clang-resource-capture-budget" if "capture budget" in str(exc)
            else "clang-resource-tree-unavailable"
        )
        return unavailable
    except OSError:
        unavailable["reason"] = "clang-resource-tree-unavailable"
        return unavailable
    return {
        "schema": 1,
        "status": "captured",
        "method": "clang-resource-tree-v1",
        "compilerId": compiler_id,
        "path": resource_path.as_posix(),
        **tree,
        "reason": "",
    }


def _verify_compiler_resource_profile(profile: dict[str, object]) -> None:
    if profile.get("status") != "captured":
        return
    path_value = profile.get("path")
    if not isinstance(path_value, str) or not Path(path_value).is_absolute():
        raise BuildError("Clang resource profile path is malformed")
    try:
        tree = _directory_tree_identity(Path(path_value))
    except OSError as exc:
        raise BuildError("Clang resource directory could not be rechecked") from exc
    for key in ("entryCount", "fileCount", "totalBytes", "treeDigest"):
        if tree[key] != profile.get(key):
            raise BuildError("Clang resource directory changed during build")


def _apple_sdk_root_identity(
    selected_paths: list[dict[str, str]],
) -> dict[str, object]:
    if not selected_paths:
        raise BuildError("Apple SDK root has no selected sysroot path")
    selected_path_values = sorted({row["path"] for row in selected_paths})
    if not selected_path_values:
        raise BuildError("Apple SDK root has no selected sysroot path")
    selected_path = Path(selected_path_values[0])
    if not selected_path.is_absolute():
        raise BuildError("Apple SDK selected sysroot is not absolute")
    resolved = selected_path.resolve(strict=True)
    if not resolved.is_dir():
        raise BuildError("Apple SDK selected sysroot is not a directory")
    metadata_files: list[dict[str, object]] = []
    total_bytes = 0
    for name in APPLE_SDK_METADATA_NAMES:
        metadata_path = resolved / name
        if not metadata_path.exists():
            if name == "SDKSettings.json":
                raise BuildError("Apple SDK JSON settings manifest is unavailable")
            continue
        if metadata_path.is_symlink() or not metadata_path.is_file():
            raise BuildError("Apple SDK settings manifest is not a regular file")
        before = metadata_path.stat()
        if before.st_size < 0 or before.st_size > MAX_APPLE_SDK_METADATA_BYTES:
            raise BuildError("Apple SDK settings manifest exceeds capture budget")
        total_bytes += before.st_size
        if total_bytes > MAX_APPLE_SDK_METADATA_BYTES:
            raise BuildError("Apple SDK metadata exceeds capture budget")
        digest = _sha256_file(metadata_path)
        after = metadata_path.stat()
        if ((before.st_size, before.st_mtime_ns, before.st_ino)
                != (after.st_size, after.st_mtime_ns, after.st_ino)):
            raise BuildError("Apple SDK settings changed while being hashed")
        metadata_files.append({
            "path": name,
            "size": after.st_size,
            "digest": digest,
        })
    if not metadata_files:
        raise BuildError("Apple SDK settings manifests are unavailable")
    return {
        "selectedPaths": [
            {"source": row["source"], "path": row["path"]}
            for row in sorted(
                selected_paths, key=lambda item: (item["source"], item["path"])
            )
        ],
        "resolvedPath": resolved.as_posix(),
        "metadataFiles": metadata_files,
        "metadataDigest": _canonical_digest(metadata_files),
    }


def _apple_sdk_profile(cmake_data: dict[str, object]) -> dict[str, object]:
    """Bind selected Apple SDK roots and their small settings manifests.

    Reached headers are captured independently by the compile dependency scan;
    this profile intentionally does not claim to hash the SDK or link runtime.
    """
    system_name = cmake_data.get("systemName")
    compiler_id = cmake_data.get("compilerId")
    profile: dict[str, object] = {
        "schema": 1,
        "status": "unavailable",
        "method": "",
        "systemName": system_name if isinstance(system_name, str) else "",
        "compilerId": compiler_id if isinstance(compiler_id, str) else "",
        "roots": [],
        "rootsDigest": _canonical_digest([]),
        "reason": "platform-not-qualified",
    }
    if (system_name != "Darwin"
            or compiler_id not in {"AppleClang", "Clang"}):
        return profile

    candidates = (
        ("CMAKE_SYSROOT", cmake_data.get("sysroot")),
        ("CMAKE_SYSROOT_COMPILE", cmake_data.get("sysrootCompile")),
        ("CMAKE_SYSROOT_LINK", cmake_data.get("sysrootLink")),
        ("CMAKE_OSX_SYSROOT", cmake_data.get("osxSysroot")),
        ("CMAKE_CXX_COMPILER_APPLE_SYSROOT",
         cmake_data.get("compilerAppleSysroot")),
    )
    selections: dict[str, list[dict[str, str]]] = {}
    for source, value in candidates:
        if not isinstance(value, str) or not value:
            continue
        path = Path(value)
        # CMAKE_OSX_SYSROOT may be an SDK selector such as "macosx". Its
        # concrete selected path is also reported by the compiler sysroot.
        if not path.is_absolute():
            continue
        try:
            resolved = path.resolve(strict=True)
        except OSError:
            profile["reason"] = "apple-sdk-root-unavailable"
            return profile
        selections.setdefault(resolved.as_posix(), []).append({
            "source": source,
            "path": path.as_posix(),
        })
    if not selections:
        profile["reason"] = "apple-sdk-path-unavailable"
        return profile

    roots: list[dict[str, object]] = []
    try:
        for resolved_path in sorted(selections):
            root = _apple_sdk_root_identity(selections[resolved_path])
            if root["resolvedPath"] != resolved_path:
                raise BuildError("Apple SDK root changed while being captured")
            roots.append(root)
    except (BuildError, OSError) as exc:
        reason = (
            "apple-sdk-metadata-budget"
            if "capture budget" in str(exc)
            else "apple-sdk-settings-unavailable"
        )
        profile["reason"] = reason
        return profile
    profile.update({
        "status": "captured",
        "method": "apple-sdk-settings-v1",
        "roots": roots,
        "rootsDigest": _canonical_digest(roots),
        "reason": "",
    })
    return profile


def _verify_apple_sdk_profile(profile: dict[str, object]) -> None:
    if profile.get("status") != "captured":
        return
    roots = profile.get("roots")
    if not isinstance(roots, list):
        raise BuildError("Apple SDK profile roots are malformed")
    refreshed: list[dict[str, object]] = []
    for row in roots:
        if not isinstance(row, dict):
            raise BuildError("Apple SDK profile root is malformed")
        selected_paths = row.get("selectedPaths")
        if not isinstance(selected_paths, list):
            raise BuildError("Apple SDK profile selected paths are malformed")
        try:
            current = _apple_sdk_root_identity(selected_paths)
        except (BuildError, OSError) as exc:
            raise BuildError("Apple SDK metadata could not be rechecked") from exc
        if current != row:
            raise BuildError("Apple SDK selection or settings changed during build")
        refreshed.append(current)
    if _canonical_digest(refreshed) != profile.get("rootsDigest"):
        raise BuildError("Apple SDK root digest is stale")


def _dyld_output_header(line: str) -> tuple[str, str] | None:
    value = line.strip()
    if value.startswith("-") and value.endswith(":"):
        return None
    match = re.fullmatch(r"(.+?) \[([^\]]+)\]:", value)
    if match:
        return match.group(1), match.group(2)
    match = re.fullmatch(r"(.+):", value)
    if match:
        return match.group(1), "unknown"
    return None


def _parse_dyld_uuid_output(output: str) -> dict[str, list[dict[str, str]]]:
    images: dict[str, list[dict[str, str]]] = {}
    current_path = ""
    current_arch = "unknown"
    in_uuid = False
    for line in output.splitlines():
        header = _dyld_output_header(line)
        if header is not None:
            current_path, current_arch = header
            in_uuid = False
            continue
        stripped = line.strip()
        if stripped == "-uuid:":
            in_uuid = True
            continue
        if stripped.startswith("-") and stripped.endswith(":"):
            in_uuid = False
            continue
        if in_uuid and re.fullmatch(r"[0-9A-Fa-f]{8}(?:-[0-9A-Fa-f]{4}){3}-[0-9A-Fa-f]{12}", stripped):
            images.setdefault(current_path, []).append({
                "architecture": current_arch,
                "uuid": stripped.upper(),
            })
            in_uuid = False
    for rows in images.values():
        rows.sort(key=lambda row: (row["architecture"], row["uuid"]))
    return images


def _parse_dyld_linked_output(
    output: str,
) -> dict[str, list[dict[str, str]]]:
    edges: dict[str, list[dict[str, str]]] = {}
    current_path = ""
    current_arch = "unknown"
    in_linked = False
    for line in output.splitlines():
        header = _dyld_output_header(line)
        if header is not None:
            current_path, current_arch = header
            in_linked = False
            continue
        stripped = line.strip()
        if stripped == "-linked_dylibs:":
            in_linked = True
            continue
        if stripped.startswith("-") and stripped.endswith(":"):
            in_linked = False
            continue
        if not in_linked or not stripped or stripped == "attributes       load path":
            continue
        columns = stripped.split(None, 1)
        if stripped.startswith(("/", "@")):
            attribute, target = "load", stripped
        elif len(columns) == 2 and columns[1].startswith(("/", "@")):
            attribute, target = columns
        else:
            continue
        if current_path and target:
            edges.setdefault(current_path, []).append({
                "source": current_path,
                "target": target,
                "architecture": current_arch,
                "attribute": attribute,
            })
    for rows in edges.values():
        rows.sort(key=lambda row: (
            row["target"], row["architecture"], row["attribute"]
        ))
    return edges


def _apple_sdk_stub_candidates(root: Path, install_name: str) -> list[Path]:
    if not install_name.startswith("/") or "\x00" in install_name:
        return []
    relative = PurePosixPath(install_name.lstrip("/"))
    if ".." in relative.parts:
        return []
    candidates: list[Path] = []
    exact = root.joinpath(*relative.parts)
    if relative.suffix == ".dylib":
        candidates.append(exact.with_suffix(".tbd"))
    candidates.append(Path(str(exact) + ".tbd"))
    framework_index = next(
        (index for index, part in enumerate(relative.parts)
         if part.endswith(".framework")),
        None,
    )
    if framework_index is not None:
        framework_parts = relative.parts[:framework_index + 1]
        framework = relative.parts[framework_index][:-len(".framework")]
        framework_root = root.joinpath(*framework_parts)
        candidates.extend((
            framework_root / (framework + ".tbd"),
            framework_root / "Versions" / "Current" / (framework + ".tbd"),
        ))
    return list(dict.fromkeys(candidates))


def _apple_sdk_stub_identities(
    install_name: str, sdk_profile: dict[str, object]
) -> list[dict[str, object]]:
    roots_value = sdk_profile.get("roots")
    if not isinstance(roots_value, list):
        return []
    stubs: list[dict[str, object]] = []
    for root_row in roots_value:
        if not isinstance(root_row, dict):
            continue
        root_value = root_row.get("resolvedPath")
        if not isinstance(root_value, str):
            continue
        root = Path(root_value).resolve()
        for candidate in _apple_sdk_stub_candidates(root, install_name):
            try:
                if candidate.is_symlink() or not candidate.is_file():
                    continue
                resolved = candidate.resolve(strict=True)
                resolved.relative_to(root)
                before = resolved.stat()
                if (before.st_size <= 0
                        or before.st_size > MAX_APPLE_TBD_BYTES):
                    continue
                text = resolved.read_text(encoding="utf-8", errors="replace")
                install_match = re.search(
                    r"(?m)^\s*install-name:\s*(?:'([^']+)'|\"([^\"]+)\"|([^\s#]+))",
                    text,
                )
                if install_match is None:
                    continue
                tbd_install_name = next(
                    value for value in install_match.groups() if value is not None
                )
                if tbd_install_name != install_name:
                    continue
                current_match = re.search(
                    r"(?m)^\s*current-version:\s*([^\s#]+)", text
                )
                compatibility_match = re.search(
                    r"(?m)^\s*compatibility-version:\s*([^\s#]+)", text
                )
                digest = _sha256_file(resolved)
                after = resolved.stat()
                if ((before.st_size, before.st_mtime_ns, before.st_ino)
                        != (after.st_size, after.st_mtime_ns, after.st_ino)):
                    raise BuildError("Apple SDK link stub changed while being hashed")
                stubs.append({
                    "sdkRoot": root.as_posix(),
                    "path": candidate.as_posix(),
                    "resolvedPath": resolved.as_posix(),
                    "size": after.st_size,
                    "digest": digest,
                    "installName": tbd_install_name,
                    "currentVersion": current_match.group(1) if current_match else "",
                    "compatibilityVersion": (
                        compatibility_match.group(1)
                        if compatibility_match else ""
                    ),
                })
                break
            except (OSError, ValueError):
                continue
    stubs.sort(key=lambda row: (str(row["sdkRoot"]), str(row["path"])))
    return stubs


def _take_dyld_image_batch(
    queue: list[tuple[str, int]],
    visited: set[str],
    *,
    batch_size: int = 64,
) -> tuple[int, list[str], list[tuple[str, int]]]:
    if not queue:
        raise BuildError("cannot take a batch from an empty dyld queue")
    if batch_size <= 0:
        raise BuildError("dyld image batch size must be positive")
    depth = min(item_depth for _, item_depth in queue)
    depth_paths = sorted({
        path for path, item_depth in queue
        if item_depth == depth and path not in visited
    })
    remaining = [
        (path, item_depth) for path, item_depth in queue if item_depth != depth
    ]
    batch = depth_paths[:batch_size]
    remaining.extend((path, depth) for path in depth_paths[batch_size:])
    return depth, batch, remaining


def _apple_runtime_profile(
    executable_path: Path,
    sdk_profile: dict[str, object],
    *,
    system_name: str,
    compiler_id: str,
    env: dict[str, str] | None,
) -> dict[str, object]:
    """Capture the Mach-O direct/transitive dyld image UUID graph.

    Runtime image UUIDs identify the concrete shared-cache images on this host.
    SDK .tbd files are matching candidates, not proof of the exact stub that
    the linker consumed, so the profile remains partial and non-promoting.
    """
    unavailable: dict[str, object] = {
        "schema": 1,
        "status": "unavailable",
        "method": "",
        "systemName": system_name,
        "compilerId": compiler_id,
        "executableDigest": "",
        "tool": {"path": "", "digest": ""},
        "nodes": [],
        "edges": [],
        "nodesDigest": _canonical_digest([]),
        "edgesDigest": _canonical_digest([]),
        "unresolvedImageCount": 0,
        "unmatchedSdkStubCount": 0,
        "maximumDepth": 0,
        "reason": "platform-not-qualified",
    }
    if system_name != "Darwin" or compiler_id not in {"AppleClang", "Clang"}:
        return unavailable
    if sdk_profile.get("status") != "captured":
        unavailable["reason"] = "apple-sdk-profile-unavailable"
        return unavailable
    tool_candidate = Path("/usr/bin/dyld_info")
    if not tool_candidate.is_file():
        unavailable["reason"] = "dyld-info-unavailable"
        return unavailable
    try:
        tool_path = _require_file(tool_candidate, "dyld_info")
        tool = {"path": tool_path.as_posix(), "digest": _sha256_file(tool_path)}
        executable = _require_file(executable_path, "semantic adapter executable")
        executable_digest = _sha256_file(executable)
        direct_output = _run(
            [tool["path"], "-linked_dylibs", executable.as_posix()], env=env
        )
    except BuildError:
        unavailable["reason"] = "macho-inspection-failed"
        return unavailable
    direct_by_source = _parse_dyld_linked_output(direct_output)
    root_edges = direct_by_source.get(executable.as_posix(), [])
    if not root_edges and "-linked_dylibs:" not in direct_output:
        unavailable["reason"] = "macho-load-command-malformed"
        return unavailable
    direct_install_names = {edge["target"] for edge in root_edges}

    edges: dict[tuple[str, str, str, str], dict[str, str]] = {}
    nodes: dict[str, dict[str, object]] = {}
    queue: list[tuple[str, int]] = []
    for edge in root_edges:
        row = dict(edge)
        row["source"] = "adapter-executable"
        edges[(row["source"], row["target"], row["architecture"],
               row["attribute"])] = row
        queue.append((row["target"], 1))
    visited: set[str] = set()
    maximum_depth = 0
    truncated = False
    while queue:
        depth, batch, queue = _take_dyld_image_batch(queue, visited)
        if not batch:
            continue
        maximum_depth = min(
            max(maximum_depth, depth), MAX_APPLE_RUNTIME_DEPTH
        )
        if depth > MAX_APPLE_RUNTIME_DEPTH or len(visited) + len(batch) > MAX_APPLE_RUNTIME_IMAGES:
            truncated = True
            break
        visited.update(batch)
        absolute = [path for path in batch if path.startswith("/")]
        nonabsolute = [path for path in batch if not path.startswith("/")]
        for path in nonabsolute:
            nodes[path] = {
                "installName": path,
                "directLinkInput": path in direct_install_names,
                "images": [],
                "sdkStubs": [],
            }
        if not absolute:
            continue
        try:
            uuid_output = _run(
                [tool["path"], "-uuid", *absolute], env=env
            )
            linked_output = _run(
                [tool["path"], "-linked_dylibs", *absolute], env=env
            )
        except BuildError:
            # Some third-party install names are not present in dyld's cache.
            # Retry individually so one such image does not erase valid peers.
            uuid_parts: list[str] = []
            linked_parts: list[str] = []
            failed: set[str] = set()
            for path in absolute:
                try:
                    uuid_parts.append(_run(
                        [tool["path"], "-uuid", path], env=env
                    ))
                    linked_parts.append(_run(
                        [tool["path"], "-linked_dylibs", path], env=env
                    ))
                except BuildError:
                    failed.add(path)
            uuid_output = "\n".join(uuid_parts)
            linked_output = "\n".join(linked_parts)
        else:
            failed = set()
        uuid_by_path = _parse_dyld_uuid_output(uuid_output)
        linked_by_source = _parse_dyld_linked_output(linked_output)
        for path in absolute:
            images = uuid_by_path.get(path, [])
            is_direct_link_input = path in direct_install_names
            stubs = (
                _apple_sdk_stub_identities(path, sdk_profile)
                if is_direct_link_input else []
            )
            nodes[path] = {
                "installName": path,
                "directLinkInput": is_direct_link_input,
                "images": images,
                "sdkStubs": stubs,
            }
            if path not in failed:
                for edge in linked_by_source.get(path, []):
                    edge_key = (
                        edge["source"], edge["target"], edge["architecture"],
                        edge["attribute"],
                    )
                    edges[edge_key] = edge
                    if len(edges) > MAX_APPLE_RUNTIME_EDGES:
                        truncated = True
                        break
                    if edge["target"] not in visited:
                        queue.append((edge["target"], depth + 1))
                if truncated:
                    break
    node_rows = [nodes[path] for path in sorted(nodes)]
    edge_rows = [edges[key] for key in sorted(edges)]
    unresolved_images = sum(
        1 for node in node_rows
        if not isinstance(node.get("images"), list) or not node["images"]
    )
    unmatched_stubs = sum(
        1 for node in node_rows
        if node.get("directLinkInput") is True
        and (not isinstance(node.get("sdkStubs"), list) or not node["sdkStubs"])
    )
    if _sha256_file(tool_path) != tool["digest"]:
        raise BuildError("dyld_info changed while capturing the runtime closure")
    reason = (
        "runtime-closure-limit" if truncated
        else "link-stub-selection-not-proven"
    )
    return {
        "schema": 1,
        "status": "partial",
        "method": "macho-dyld-uuid-closure-v1",
        "systemName": system_name,
        "compilerId": compiler_id,
        "executableDigest": executable_digest,
        "tool": tool,
        "nodes": node_rows,
        "edges": edge_rows,
        "nodesDigest": _canonical_digest(node_rows),
        "edgesDigest": _canonical_digest(edge_rows),
        "unresolvedImageCount": unresolved_images,
        "unmatchedSdkStubCount": unmatched_stubs,
        "maximumDepth": maximum_depth,
        "reason": reason,
    }


def _verify_apple_runtime_profile(
    profile: dict[str, object],
    executable_path: Path,
) -> None:
    if profile.get("status") == "unavailable":
        return
    if profile.get("status") != "partial":
        raise BuildError("Apple runtime profile has an unsupported status")
    executable = _require_file(executable_path, "semantic adapter executable")
    if _sha256_file(executable) != profile.get("executableDigest"):
        raise BuildError("semantic adapter executable changed after runtime inspection")
    tool = profile.get("tool")
    if not isinstance(tool, dict):
        raise BuildError("Apple runtime profile tool identity is malformed")
    tool_path = tool.get("path")
    tool_digest = tool.get("digest")
    if (not isinstance(tool_path, str) or not Path(tool_path).is_absolute()
            or not isinstance(tool_digest, str) or not HEX64.fullmatch(tool_digest)):
        raise BuildError("Apple runtime profile tool identity is malformed")
    if _sha256_file(Path(tool_path)) != tool_digest:
        raise BuildError("dyld_info changed during runtime inspection")
    nodes = profile.get("nodes")
    if not isinstance(nodes, list):
        raise BuildError("Apple runtime profile nodes are malformed")
    for node in nodes:
        if not isinstance(node, dict):
            raise BuildError("Apple runtime profile node is malformed")
        stubs = node.get("sdkStubs")
        if not isinstance(stubs, list):
            raise BuildError("Apple runtime SDK stubs are malformed")
        for stub in stubs:
            if not isinstance(stub, dict):
                raise BuildError("Apple runtime SDK stub is malformed")
            path_value = stub.get("resolvedPath")
            size = stub.get("size")
            digest = stub.get("digest")
            if (not isinstance(path_value, str)
                    or not Path(path_value).is_absolute()
                    or isinstance(size, bool) or not isinstance(size, int)
                    or not isinstance(digest, str) or not HEX64.fullmatch(digest)):
                raise BuildError("Apple runtime SDK stub identity is malformed")
            path = Path(path_value)
            if (not path.is_file() or path.stat().st_size != size
                    or _sha256_file(path) != digest):
                raise BuildError("Apple SDK link stub changed after runtime inspection")


APPLE_LINK_MAP_OBJECT_RE = re.compile(r"^\[\s*(\d+)\]\s+(.+?)\s*$")
APPLE_LINK_MAP_ARCHIVE_RE = re.compile(r"^(.+?\.a)\(([^()]+)\)$")


def _stable_file_identity(path: Path, *, maximum_bytes: int, label: str) -> dict[str, object]:
    resolved = _require_file(path, label)
    before = resolved.stat()
    if before.st_size < 0 or before.st_size > maximum_bytes:
        raise BuildError(f"{label} exceeds its capture budget")
    digest = _sha256_file(resolved)
    after = resolved.stat()
    if ((before.st_size, before.st_mtime_ns, before.st_ino)
            != (after.st_size, after.st_mtime_ns, after.st_ino)):
        raise BuildError(f"{label} changed while being hashed")
    return {"path": resolved, "size": after.st_size, "digest": digest}


def _parse_apple_link_map(
    data: bytes,
) -> tuple[str, str, list[tuple[int, str]]]:
    if len(data) > MAX_APPLE_LINK_MAP_BYTES:
        raise BuildError("Apple linker map exceeds its capture budget")
    try:
        lines = data.decode("utf-8", errors="strict").splitlines()
    except UnicodeDecodeError as exc:
        raise BuildError("Apple linker map is not valid UTF-8") from exc
    output_paths = [
        line.partition(":")[2].strip()
        for line in lines if line.startswith("# Path:")
    ]
    if (len(output_paths) != 1 or not output_paths[0]
            or "\x00" in output_paths[0]):
        raise BuildError("Apple linker map output-path header is malformed")
    architectures = [
        line.partition(":")[2].strip()
        for line in lines if line.startswith("# Arch:")
    ]
    if len(architectures) != 1 or not re.fullmatch(
            r"[A-Za-z0-9_+-]{1,32}", architectures[0]):
        raise BuildError("Apple linker map architecture header is malformed")
    starts = [index for index, line in enumerate(lines)
              if line == "# Object files:"]
    ends = [index for index, line in enumerate(lines) if line == "# Sections:"]
    if len(starts) != 1 or len(ends) != 1 or starts[0] >= ends[0]:
        raise BuildError("Apple linker map object-file section is malformed")
    rows: list[tuple[int, str]] = []
    for line in lines[starts[0] + 1:ends[0]]:
        if not line:
            continue
        match = APPLE_LINK_MAP_OBJECT_RE.fullmatch(line)
        if match is None:
            raise BuildError("Apple linker map contains an unknown object row")
        rows.append((int(match.group(1)), match.group(2)))
        if len(rows) > MAX_APPLE_LINK_MAP_OBJECTS:
            raise BuildError("Apple linker map object count exceeds its budget")
    indices = [index for index, _ in rows]
    if (not rows or indices != list(range(len(rows)))
            or len(set(indices)) != len(indices)):
        raise BuildError("Apple linker map object indexes are incomplete")
    return output_paths[0], architectures[0], rows


def _apple_link_inputs_profile(
    *,
    map_path: Path,
    build_tree: Path,
    executable_path: Path,
    library_path: Path,
    sdk_profile: dict[str, object],
    system_name: str,
    compiler_id: str,
) -> dict[str, object]:
    """Bind actual Apple ld map inputs separately from dyld runtime images.

    A captured profile classifies each map row as a build object, a member of
    the attested static archive, a linker-synthesized row, or an SDK file whose
    bytes were hashed. It does not claim closure over unreferenced SDK files,
    shared-cache runtime bytes, or other platforms.
    """
    empty: dict[str, object] = {
        "schema": 1,
        "status": "unavailable",
        "method": "",
        "systemName": system_name,
        "compilerId": compiler_id,
        "architecture": "",
        "executableDigest": "",
        "mapSize": 0,
        "mapDigest": "",
        "objectCount": 0,
        "synthesizedObjectCount": 0,
        "buildObjects": [],
        "buildObjectsDigest": _canonical_digest([]),
        "linkedArchive": {"path": "", "size": 0, "digest": ""},
        "archiveMembers": [],
        "archiveMembersDigest": _canonical_digest([]),
        "externalInputs": [],
        "externalInputsDigest": _canonical_digest([]),
        "unclassifiedCount": 0,
        "unclassifiedDigest": _canonical_digest([]),
        "reason": "platform-not-qualified",
    }
    if system_name != "Darwin" or compiler_id not in {"AppleClang", "Clang"}:
        return empty
    if sdk_profile.get("status") != "captured":
        empty["reason"] = "apple-sdk-profile-unavailable"
        return empty

    map_identity = _stable_file_identity(
        map_path, maximum_bytes=MAX_APPLE_LINK_MAP_BYTES,
        label="Apple linker map",
    )
    map_data = map_path.read_bytes()
    if (len(map_data) != map_identity["size"]
            or _sha256_bytes(map_data) != map_identity["digest"]):
        raise BuildError("Apple linker map changed while being parsed")
    map_output_path, architecture, rows = _parse_apple_link_map(map_data)
    build_root = build_tree.resolve(strict=True)
    executable = _require_file(executable_path, "semantic adapter executable")
    declared_output = Path(map_output_path)
    if not declared_output.is_absolute():
        declared_output = build_root / declared_output
    try:
        resolved_output = declared_output.resolve(strict=True)
    except OSError as exc:
        raise BuildError(
            "Apple linker map output path does not resolve to an executable"
        ) from exc
    if resolved_output != executable.resolve(strict=True):
        raise BuildError(
            "Apple linker map output path does not match the attested executable"
        )
    expected_archive = library_path.resolve(strict=True)
    sdk_roots_value = sdk_profile.get("roots")
    if not isinstance(sdk_roots_value, list):
        raise BuildError("Apple SDK profile roots are malformed")
    sdk_roots: list[Path] = []
    for root_value in sdk_roots_value:
        if not isinstance(root_value, dict) or not isinstance(
                root_value.get("resolvedPath"), str):
            raise BuildError("Apple SDK profile root is malformed")
        sdk_roots.append(Path(root_value["resolvedPath"]).resolve(strict=True))

    build_objects: dict[str, dict[str, object]] = {}
    archive_members: set[str] = set()
    seen_archive_members: set[tuple[Path, str]] = set()
    archive_rows: set[Path] = set()
    external_inputs: dict[tuple[str, str], dict[str, object]] = {}
    seen_build_objects: set[str] = set()
    unclassified: list[str] = []
    synthesized_count = 0
    total_input_bytes = 0
    for _, item in rows:
        if item == "linker synthesized":
            synthesized_count += 1
            continue
        archive_match = APPLE_LINK_MAP_ARCHIVE_RE.fullmatch(item)
        if archive_match is not None:
            archive_token, member = archive_match.groups()
            archive_candidate = Path(archive_token)
            if not archive_candidate.is_absolute():
                archive_candidate = build_root / archive_candidate
            try:
                archive_resolved = archive_candidate.resolve(strict=True)
            except OSError:
                unclassified.append(item)
                continue
            if archive_resolved != expected_archive or not member:
                unclassified.append(item)
                continue
            member_key = (archive_resolved, member)
            if member_key in seen_archive_members:
                raise BuildError(
                    "Apple linker map repeats a selected archive member"
                )
            seen_archive_members.add(member_key)
            archive_rows.add(archive_resolved)
            archive_members.add(member)
            continue

        candidate = Path(item)
        if not candidate.is_absolute():
            candidate = build_root / candidate
        try:
            resolved = candidate.resolve(strict=True)
        except OSError:
            unclassified.append(item)
            continue
        try:
            relative_build = resolved.relative_to(build_root)
        except ValueError:
            relative_build = None
        if relative_build is not None:
            relative_key = relative_build.as_posix()
            if relative_key in seen_build_objects:
                raise BuildError("Apple linker map repeats a selected build object")
            seen_build_objects.add(relative_key)
            identity = _stable_file_identity(
                resolved, maximum_bytes=MAX_APPLE_LINK_INPUT_BYTES,
                label="Apple linker build object",
            )
            total_input_bytes += int(identity["size"])
            build_objects[relative_key] = {
                "path": relative_key,
                "size": identity["size"],
                "digest": identity["digest"],
            }
            continue

        sdk_match: tuple[Path, Path] | None = None
        for sdk_root in sdk_roots:
            try:
                relative_sdk = resolved.relative_to(sdk_root)
            except ValueError:
                continue
            sdk_match = (sdk_root, relative_sdk)
            break
        if sdk_match is None:
            unclassified.append(item)
            continue
        sdk_root, relative_sdk = sdk_match
        identity = _stable_file_identity(
            resolved, maximum_bytes=MAX_APPLE_LINK_INPUT_BYTES,
            label="Apple SDK selected link input",
        )
        total_input_bytes += int(identity["size"])
        if total_input_bytes > MAX_APPLE_LINK_TOTAL_INPUT_BYTES:
            raise BuildError("Apple linker input bytes exceed their capture budget")
        row = {
            "sdkRoot": sdk_root.as_posix(),
            "path": relative_sdk.as_posix(),
            "size": identity["size"],
            "digest": identity["digest"],
        }
        external_key = (row["sdkRoot"], row["path"])
        if external_key in external_inputs:
            raise BuildError("Apple linker map repeats a selected SDK input")
        external_inputs[external_key] = row

    if archive_rows != {expected_archive} or not archive_members:
        raise BuildError(
            "Apple linker map does not identify members of the attested archive"
        )
    if not build_objects:
        raise BuildError("Apple linker map contains no bound build object")
    archive_identity = _stable_file_identity(
        expected_archive, maximum_bytes=MAX_APPLE_LINK_INPUT_BYTES * 8,
        label="Apple linked static archive",
    )
    if total_input_bytes > MAX_APPLE_LINK_TOTAL_INPUT_BYTES:
        raise BuildError("Apple linker input bytes exceed their capture budget")
    build_object_rows = [build_objects[key] for key in sorted(build_objects)]
    archive_member_rows = sorted(archive_members)
    external_rows = [external_inputs[key] for key in sorted(external_inputs)]
    unknown_rows = sorted(unclassified)
    status = "partial" if unknown_rows else "captured"
    reason = "unclassified-link-inputs" if unknown_rows else ""
    return {
        "schema": 1,
        "status": status,
        "method": "apple-ld-map-v1",
        "systemName": system_name,
        "compilerId": compiler_id,
        "architecture": architecture,
        "executableDigest": _sha256_file(executable),
        "mapSize": map_identity["size"],
        "mapDigest": map_identity["digest"],
        "objectCount": len(rows),
        "synthesizedObjectCount": synthesized_count,
        "buildObjects": build_object_rows,
        "buildObjectsDigest": _canonical_digest(build_object_rows),
        "linkedArchive": {
            "path": "<build>/" + expected_archive.relative_to(build_root).as_posix(),
            "size": archive_identity["size"],
            "digest": archive_identity["digest"],
        },
        "archiveMembers": archive_member_rows,
        "archiveMembersDigest": _canonical_digest(archive_member_rows),
        "externalInputs": external_rows,
        "externalInputsDigest": _canonical_digest(external_rows),
        "unclassifiedCount": len(unknown_rows),
        "unclassifiedDigest": _canonical_digest(unknown_rows),
        "reason": reason,
    }


def _verify_apple_link_inputs_profile(
    profile: dict[str, object],
    *,
    map_path: Path,
    build_tree: Path,
    executable_path: Path,
    library_path: Path,
    sdk_profile: dict[str, object],
    system_name: str,
    compiler_id: str,
) -> None:
    if profile.get("status") == "unavailable":
        return
    current = _apple_link_inputs_profile(
        map_path=map_path,
        build_tree=build_tree,
        executable_path=executable_path,
        library_path=library_path,
        sdk_profile=sdk_profile,
        system_name=system_name,
        compiler_id=compiler_id,
    )
    if current != profile:
        raise BuildError("Apple linker input identity changed after link inspection")


def _bind_apple_runtime_selected_stubs(
    runtime_profile: dict[str, object],
    link_inputs_profile: dict[str, object],
) -> dict[str, object]:
    """Bind runtime install-name stub candidates to exact linker-map inputs."""
    if runtime_profile.get("status") != "partial":
        return runtime_profile
    external_inputs = link_inputs_profile.get("externalInputs")
    if not isinstance(external_inputs, list):
        external_inputs = []
    selected_input_keys = {
        (
            row.get("sdkRoot"), row.get("path"), row.get("size"),
            row.get("digest"),
        )
        for row in external_inputs if isinstance(row, dict)
    }
    nodes = runtime_profile.get("nodes")
    if not isinstance(nodes, list):
        raise BuildError("Apple runtime profile nodes are malformed")
    direct_names: set[str] = set()
    selected_rows: list[dict[str, object]] = []
    selected_names: set[str] = set()
    for node in nodes:
        if not isinstance(node, dict):
            raise BuildError("Apple runtime profile node is malformed")
        if node.get("directLinkInput") is not True:
            continue
        install_name = node.get("installName")
        candidates = node.get("sdkStubs")
        if not isinstance(install_name, str) or not isinstance(candidates, list):
            raise BuildError("Apple runtime direct stub candidates are malformed")
        direct_names.add(install_name)
        for candidate in candidates:
            if not isinstance(candidate, dict):
                raise BuildError("Apple runtime SDK stub candidate is malformed")
            root_value = candidate.get("sdkRoot")
            resolved_value = candidate.get("resolvedPath")
            size = candidate.get("size")
            digest = candidate.get("digest")
            if (not isinstance(root_value, str)
                    or not isinstance(resolved_value, str)
                    or isinstance(size, bool) or not isinstance(size, int)
                    or not isinstance(digest, str)):
                raise BuildError("Apple runtime SDK stub candidate identity is malformed")
            try:
                relative_path = Path(resolved_value).relative_to(
                    Path(root_value)
                ).as_posix()
            except ValueError:
                raise BuildError(
                    "Apple runtime SDK stub resolves outside its SDK root"
                ) from None
            if (root_value, relative_path, size, digest) not in selected_input_keys:
                continue
            selected_rows.append({
                "installName": install_name,
                "sdkRoot": root_value,
                "path": relative_path,
                "size": size,
                "digest": digest,
            })
            selected_names.add(install_name)
    selected_rows.sort(key=lambda row: (
        str(row["installName"]), str(row["sdkRoot"]), str(row["path"])
    ))
    bound = dict(runtime_profile)
    bound.update({
        "schema": 2,
        "linkerSelectedStubs": selected_rows,
        "linkerSelectedStubsDigest": _canonical_digest(selected_rows),
        "unselectedDirectInstallNameCount": len(direct_names - selected_names),
    })
    if runtime_profile.get("reason") != "runtime-closure-limit":
        if (link_inputs_profile.get("status") == "captured"
                and selected_names == direct_names):
            bound["reason"] = "runtime-byte-identity-not-proven"
        else:
            bound["reason"] = "link-stub-selection-not-proven"
    return bound


def _verify_compiler_dependency_profile(
    profile: dict[str, object], *, build_tree: Path
) -> None:
    if profile.get("status") != "captured":
        return
    inputs = profile.get("externalInputs")
    if not isinstance(inputs, list):
        raise BuildError("compiler dependency profile is malformed")
    for row in inputs:
        if not isinstance(row, dict):
            raise BuildError("compiler dependency input is malformed")
        path_value = row.get("path")
        size = row.get("size")
        digest = row.get("digest")
        if (not isinstance(path_value, str) or not Path(path_value).is_absolute()
                or isinstance(size, bool) or not isinstance(size, int)
                or not isinstance(digest, str) or not HEX64.fullmatch(digest)):
            raise BuildError("compiler dependency input identity is malformed")
        path = Path(path_value)
        if not path.is_file() or path.stat().st_size != size:
            raise BuildError("external compiler dependency changed during build")
        if _sha256_file(path) != digest:
            raise BuildError("external compiler dependency changed during build")
    if _canonical_digest(inputs) != profile.get("externalInputsDigest"):
        raise BuildError("compiler dependency closure digest is stale")


def _package_root_from_library(
    build_tree: Path, target: dict[str, object], label: str
) -> Path:
    sources, _ = _compiled_source_paths(build_tree, target, label)
    roots: set[Path] = set()
    for source in sources:
        src_directory = next(
            (parent for parent in source.parents if parent.name == "src"), None
        )
        if src_directory is None:
            raise BuildError(
                f"CMake target {label} source is outside a package src tree: {source}"
            )
        roots.add(src_directory.parent.resolve())
    if len(roots) != 1:
        raise BuildError(f"CMake target {label} spans multiple package roots")
    return next(iter(roots))


def _normalize_include_path(
    value: str, *, build_tree: Path, package_root: Path
) -> str:
    path = Path(value)
    if not path.is_absolute():
        path = _target_source_root(build_tree) / path
    resolved = path.resolve()
    try:
        relative = resolved.relative_to(package_root.resolve())
    except ValueError:
        try:
            relative = resolved.relative_to(build_tree.resolve())
        except ValueError:
            return resolved.as_posix()
        return "<build>/" + relative.as_posix()
    return "<package>/" + relative.as_posix()


def _compile_profile(
    *,
    build_tree: Path,
    target: dict[str, object],
    label: str,
    package_root: Path,
    ignored_define_prefixes: tuple[str, ...] = (),
) -> dict[str, object]:
    _, referenced_groups = _compiled_source_paths(build_tree, target, label)
    groups = target.get("compileGroups")
    if not isinstance(groups, list) or len(referenced_groups) != 1:
        raise BuildError(f"CMake target {label} must use exactly one compile group")
    group_index = next(iter(referenced_groups))
    if group_index < 0 or group_index >= len(groups):
        raise BuildError(f"CMake target {label} has a dangling compile group")
    group = groups[group_index]
    if not isinstance(group, dict) or group.get("language") != "CXX":
        raise BuildError(f"CMake target {label} compile group is not C++")
    standard = group.get("languageStandard")
    if not isinstance(standard, dict) or not isinstance(standard.get("standard"), str):
        raise BuildError(f"CMake target {label} omits its C++ language standard")
    fragments = group.get("compileCommandFragments")
    if not isinstance(fragments, list):
        raise BuildError(f"CMake target {label} omits compile-command fragments")
    flags: list[str] = []
    for index, row in enumerate(fragments):
        if not isinstance(row, dict) or not isinstance(row.get("fragment"), str):
            raise BuildError(f"CMake target {label} has a malformed compile fragment")
        tokens = _split_command_fragment(
            row["fragment"], f"{label} compile fragment {index}"
        )
        flags.extend(
            _normalize_profile_text(
                token, build_tree=build_tree, package_root=package_root
            )
            for token in tokens
            if not _is_diagnostic_compile_flag(token)
        )

    defines_value = group.get("defines", [])
    if not isinstance(defines_value, list):
        raise BuildError(f"CMake target {label} has malformed compile definitions")
    defines: list[str] = []
    for row in defines_value:
        if not isinstance(row, dict) or not isinstance(row.get("define"), str):
            raise BuildError(f"CMake target {label} has a malformed definition")
        define = row["define"]
        if any(define.startswith(prefix) for prefix in ignored_define_prefixes):
            continue
        defines.append(
            _normalize_profile_text(
                define, build_tree=build_tree, package_root=package_root
            )
        )

    includes_value = group.get("includes", [])
    if not isinstance(includes_value, list):
        raise BuildError(f"CMake target {label} has malformed include metadata")
    includes: list[dict[str, object]] = []
    for row in includes_value:
        if not isinstance(row, dict) or not isinstance(row.get("path"), str):
            raise BuildError(f"CMake target {label} has a malformed include path")
        includes.append(
            {
                "path": _normalize_include_path(
                    row["path"], build_tree=build_tree, package_root=package_root
                ),
                "isSystem": bool(row.get("isSystem", False)),
            }
        )
    sysroot = group.get("sysroot")
    if sysroot is None:
        sysroot_path = ""
    elif isinstance(sysroot, dict) and isinstance(sysroot.get("path"), str):
        sysroot_path = _normalize_profile_text(
            sysroot["path"], build_tree=build_tree, package_root=package_root
        )
    else:
        raise BuildError(f"CMake target {label} has malformed compile sysroot metadata")
    return {
        "language": "CXX",
        "languageStandard": standard["standard"],
        "flags": flags,
        "defines": sorted(defines),
        "includes": includes,
        "sysroot": sysroot_path,
    }


def _token_names_library(token: str, library_path: Path) -> bool:
    value = token.strip('"\'')
    if Path(value).name.lower() == library_path.name.lower():
        return True
    stem = library_path.stem
    if stem.lower().startswith("lib"):
        stem = stem[3:]
    return value.lower() == "-l" + stem.lower()


def _link_profile(
    *,
    build_tree: Path,
    target: dict[str, object],
    label: str,
    package_root: Path,
    library_path: Path,
) -> dict[str, object]:
    link = target.get("link")
    if not isinstance(link, dict) or link.get("language") != "CXX":
        raise BuildError(f"CMake target {label} has no C++ link metadata")
    fragments_value = link.get("commandFragments")
    if not isinstance(fragments_value, list):
        raise BuildError(f"CMake target {label} has no link-command fragments")
    fragments: list[dict[str, object]] = []
    library_mentions = 0
    for index, row in enumerate(fragments_value):
        if (
            not isinstance(row, dict)
            or not isinstance(row.get("fragment"), str)
            or not isinstance(row.get("role"), str)
        ):
            raise BuildError(f"CMake target {label} has a malformed link fragment")
        tokens = _split_command_fragment(
            row["fragment"], f"{label} link fragment {index}"
        )
        normalized: list[str] = []
        for token in tokens:
            if row["role"] == "libraries" and _token_names_library(
                token, library_path
            ):
                library_mentions += 1
                continue
            normalized.append(
                _normalize_profile_text(
                    token, build_tree=build_tree, package_root=package_root
                )
            )
        if normalized:
            fragments.append({"role": row["role"], "tokens": normalized})
    if library_mentions != 1:
        raise BuildError(
            f"CMake target {label} parity link profile does not name its "
            "attested library once"
        )
    sysroot = link.get("sysroot")
    if sysroot is None:
        sysroot_path = ""
    elif isinstance(sysroot, dict) and isinstance(sysroot.get("path"), str):
        sysroot_path = _normalize_profile_text(
            sysroot["path"], build_tree=build_tree, package_root=package_root
        )
    else:
        raise BuildError(f"CMake target {label} has malformed link sysroot metadata")
    return {
        "language": "CXX",
        "fragments": fragments,
        "sysroot": sysroot_path,
    }


def _build_parity_identity(
    *,
    build_tree: Path,
    configuration: str,
    cmake_program: str,
    cmake_data: dict[str, object],
    executable_target: str,
    library_target: str,
    library_path: Path,
    env: dict[str, str] | None = None,
    driver_context: dict[str, object] | None = None,
    compiler_dependencies: dict[str, object] | None = None,
    compiler_resources: dict[str, object] | None = None,
    apple_sdk: dict[str, object] | None = None,
    apple_runtime: dict[str, object] | None = None,
    apple_link_inputs: dict[str, object] | None = None,
) -> dict[str, object]:
    _, targets = _ensure_codemodel(
        build_tree, configuration, cmake_program, env=env,
        driver_context=driver_context,
    )
    executable = targets.get(executable_target)
    library = targets.get(library_target)
    if executable is None or executable.get("type") != "EXECUTABLE":
        raise BuildError("semantic adapter parity executable metadata is absent")
    if library is None or library.get("type") != "STATIC_LIBRARY":
        raise BuildError("semantic adapter parity library metadata is absent")
    package_root = _package_root_from_library(build_tree, library, library_target)
    adapter_defines = (
        "LIBDXFRW_SEMANTIC_SIDE_",
        "LIBDXFRW_SEMANTIC_ADAPTER_NAME=",
        "LIBDXFRW_SEMANTIC_ADAPTER_COMMIT=",
        "LIBDXFRW_SEMANTIC_ADAPTER_CONFIG_DIGEST=",
    )
    identity = {
        "schema": 1,
        "configuration": configuration,
        "cmake": cmake_data,
        "adapterCompile": _compile_profile(
            build_tree=build_tree,
            target=executable,
            label=executable_target,
            package_root=package_root,
            ignored_define_prefixes=adapter_defines,
        ),
        "libraryCompile": _compile_profile(
            build_tree=build_tree,
            target=library,
            label=library_target,
            package_root=package_root,
        ),
        "adapterLink": _link_profile(
            build_tree=build_tree,
            target=executable,
            label=executable_target,
            package_root=package_root,
            library_path=library_path,
        ),
    }
    if compiler_dependencies is not None:
        identity["compilerDependencies"] = compiler_dependencies
    if compiler_resources is not None:
        identity["compilerResources"] = compiler_resources
    if apple_sdk is not None:
        identity["appleSdk"] = apple_sdk
    if apple_runtime is not None:
        identity["appleRuntime"] = apple_runtime
    if apple_link_inputs is not None:
        identity["appleLinkInputs"] = apple_link_inputs
    return identity


PARENT_CMAKE_FORWARD_KEYS = (
    "CMAKE_CXX_COMPILER",
    "CMAKE_CXX_COMPILER_ARG1",
    "CMAKE_CXX_COMPILER_LAUNCHER",
    "CMAKE_CXX_LINKER_LAUNCHER",
    "CMAKE_RULE_LAUNCH_COMPILE",
    "CMAKE_RULE_LAUNCH_LINK",
    "CMAKE_CXX_COMPILER_TARGET",
    "CMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN",
    "CMAKE_MAKE_PROGRAM",
    "CMAKE_TOOLCHAIN_FILE",
    "CMAKE_SYSROOT",
    "CMAKE_SYSROOT_COMPILE",
    "CMAKE_SYSROOT_LINK",
    "CMAKE_SYSTEM_NAME",
    "CMAKE_SYSTEM_VERSION",
    "CMAKE_SYSTEM_PROCESSOR",
    "CMAKE_OSX_ARCHITECTURES",
    "CMAKE_OSX_SYSROOT",
    "CMAKE_OSX_DEPLOYMENT_TARGET",
    "CMAKE_CXX_FLAGS",
    "CMAKE_EXE_LINKER_FLAGS",
    "CMAKE_STATIC_LINKER_FLAGS",
    "CMAKE_CXX_STANDARD_LIBRARIES",
    "CMAKE_POSITION_INDEPENDENT_CODE",
    "CMAKE_INTERPROCEDURAL_OPTIMIZATION",
    "CMAKE_MSVC_RUNTIME_LIBRARY",
    "CMAKE_GENERATOR_INSTANCE",
    "CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION",
)


def _parent_cmake_settings(
    build_tree: Path | None, configuration: str
) -> dict[str, object] | None:
    if build_tree is None:
        return None
    parent = _require_directory(build_tree, "parent CMake build")
    _verify_requested_configuration(parent, configuration)
    cache = _parse_cmake_cache(parent)
    generator = cache.get("CMAKE_GENERATOR", "")
    if not generator:
        raise BuildError("parent CMake cache omits CMAKE_GENERATOR")
    configuration_types = [
        value
        for value in cache.get("CMAKE_CONFIGURATION_TYPES", "").split(";")
        if value
    ]
    forwarded: dict[str, str] = {}
    for key in PARENT_CMAKE_FORWARD_KEYS:
        value = cache.get(key, "")
        if value:
            forwarded[key] = value
    configuration_upper = configuration.upper()
    for prefix in (
        "CMAKE_CXX_FLAGS_",
        "CMAKE_EXE_LINKER_FLAGS_",
        "CMAKE_STATIC_LINKER_FLAGS_",
        "CMAKE_INTERPROCEDURAL_OPTIMIZATION_",
    ):
        key = prefix + configuration_upper
        if cache.get(key, ""):
            forwarded[key] = cache[key]
    if configuration_types:
        forwarded["CMAKE_CONFIGURATION_TYPES"] = ";".join(configuration_types)
    return {
        "generator": generator,
        "generatorPlatform": cache.get("CMAKE_GENERATOR_PLATFORM", ""),
        "generatorToolset": cache.get("CMAKE_GENERATOR_TOOLSET", ""),
        "multiConfig": bool(configuration_types),
        "cacheVariables": forwarded,
    }


def _configure_target(
    *,
    cmake_program: str,
    wrapper: Path,
    build_tree: Path,
    target_root: Path,
    adapter_source: Path,
    commit: str,
    configuration: str,
    config_digest: str | None,
    parent_settings: dict[str, object] | None,
    env: dict[str, str],
    driver_context: dict[str, object],
) -> None:
    compiler_path = driver_context.get("compilerPath")
    if not isinstance(compiler_path, str) or not compiler_path:
        raise BuildError("controlled compiler identity is malformed")
    command = [
        cmake_program,
        "-S",
        str(wrapper),
        "-B",
        str(build_tree),
        "-DLIBDXFRW_SEMANTIC_TARGET_ROOT=" + str(target_root),
        "-DLIBDXFRW_SEMANTIC_ADAPTER_SOURCE=" + str(adapter_source),
        "-DLIBDXFRW_SEMANTIC_ADAPTER_COMMIT=" + commit,
        "-DCMAKE_CXX_COMPILER=" + compiler_path,
    ]
    if parent_settings is not None:
        command.extend(["-G", str(parent_settings["generator"])])
        platform = str(parent_settings["generatorPlatform"])
        toolset = str(parent_settings["generatorToolset"])
        if platform:
            command.extend(["-A", platform])
        if toolset:
            command.extend(["-T", toolset])
        cache_variables = parent_settings["cacheVariables"]
        if not isinstance(cache_variables, dict):
            raise BuildError("parent CMake cache-variable identity is malformed")
        for key, value in sorted(cache_variables.items()):
            if key == "CMAKE_CXX_COMPILER":
                continue
            command.append(f"-D{key}={value}")
        if not bool(parent_settings["multiConfig"]):
            command.append("-DCMAKE_BUILD_TYPE=" + configuration)
    if config_digest is not None:
        command.append("-DLIBDXFRW_SEMANTIC_CONFIG_DIGEST=" + config_digest)
    # Make/Ninja expose exact compiler calls through this machine-readable file.
    # Other generators remain explicitly unavailable in the dependency profile.
    command.append("-DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
    _verify_driver_binary(driver_context)
    _run(command, env=env)
    _verify_driver_binary(driver_context)
    if parent_settings is None:
        # Discover whether the default generator is single- or multi-config.
        # A second configure pins the requested build type only for the former.
        cache = _parse_cmake_cache(build_tree)
        if not cache.get("CMAKE_CONFIGURATION_TYPES", ""):
            _verify_driver_binary(driver_context)
            _run(command + ["-DCMAKE_BUILD_TYPE=" + configuration], env=env)
            _verify_driver_binary(driver_context)
    _verify_requested_configuration(build_tree, configuration)


def _find_one(paths: list[Path], label: str) -> Path:
    matches = sorted({path.resolve() for path in paths if path.is_file()})
    if len(matches) != 1:
        raise BuildError(f"expected exactly one {label}; found {matches}")
    return matches[0]


def _target_artifacts(
    build_tree: Path,
    configuration: str,
    cmake_program: str,
    *,
    env: dict[str, str] | None = None,
    driver_context: dict[str, object] | None = None,
) -> tuple[Path, Path]:
    _, targets = _ensure_codemodel(
        build_tree, configuration, cmake_program, env=env,
        driver_context=driver_context,
    )
    executable_target = targets.get(TARGET_EXECUTABLE)
    library_target = targets.get(TARGET_LIBRARY)
    if executable_target is None or executable_target.get("type") != "EXECUTABLE":
        raise BuildError("CMake codemodel omits the target adapter executable")
    if library_target is None or library_target.get("type") != "STATIC_LIBRARY":
        raise BuildError("CMake codemodel omits the target adapter static library")
    executable = _find_one(
        _target_artifact_paths(build_tree, executable_target, TARGET_EXECUTABLE),
        "semantic adapter executable",
    )
    library = _find_one(
        _target_artifact_paths(build_tree, library_target, TARGET_LIBRARY),
        "semantic target static library",
    )
    return executable, library


def _artifact_path(path: Path, base: Path) -> str:
    try:
        value = path.resolve().relative_to(base.resolve()).as_posix()
    except ValueError:
        raise BuildError(
            f"receipt artifacts must be contained by the receipt directory: {path}"
        ) from None
    if not value or value == ".":
        raise BuildError(f"receipt artifact has no relative filename: {path}")
    return value


def _logical_input_path(path: Path) -> str:
    try:
        return path.resolve().relative_to(REPOSITORY_ROOT).as_posix()
    except ValueError:
        # External inputs are content-addressed in the receipt.  Retain a useful
        # logical label without leaking a machine-specific absolute path.
        return path.name


def _snapshot_input(
    source: Path, destination_root: Path, filename: str, label: str
) -> tuple[Path, str]:
    source_path = _require_file(source, label)
    data = source_path.read_bytes()
    digest = _sha256_bytes(data)
    destination = destination_root / digest / filename
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists():
        if not destination.is_file() or _sha256_file(destination) != digest:
            raise BuildError(f"cached {label} snapshot differs from its digest")
    else:
        try:
            with destination.open("xb") as stream:
                stream.write(data)
        except FileExistsError:
            if not destination.is_file() or _sha256_file(destination) != digest:
                raise BuildError(f"raced {label} snapshot differs from its digest")
    return destination, digest


def _snapshot_adapter_support_inputs(
    paths: list[Path], staged_adapter_source: Path
) -> list[dict[str, str]]:
    """Place content-bound quoted-include inputs beside the adapter source."""
    rows: list[dict[str, str]] = []
    names = {staged_adapter_source.name}
    for path in paths:
        source = _require_file(path, "adapter support input")
        filename = source.name
        if (not filename or filename in names or filename in {".", ".."}
                or "/" in filename or "\\" in filename):
            raise BuildError("adapter support input has a duplicate or unsafe filename")
        names.add(filename)
        data = source.read_bytes()
        digest = _sha256_bytes(data)
        destination = staged_adapter_source.parent / filename
        if destination.is_symlink():
            raise BuildError("adapter support snapshot must not be a symlink")
        if destination.exists():
            if not destination.is_file() or _sha256_file(destination) != digest:
                raise BuildError("cached adapter support snapshot differs from its digest")
        else:
            try:
                with destination.open("xb") as stream:
                    stream.write(data)
            except FileExistsError:
                if not destination.is_file() or _sha256_file(destination) != digest:
                    raise BuildError("raced adapter support snapshot differs from its digest")
        rows.append({
            "path": _logical_input_path(source),
            "snapshotPath": filename,
            "digest": digest,
        })
    return rows


def _snapshot_build_artifact(
    source: Path, destination_root: Path, label: str, *, executable: bool
) -> Path:
    source_path = _require_file(source, label)
    snapshot, digest = _snapshot_input(
        source_path,
        destination_root / "artifacts",
        source_path.name,
        label,
    )
    if executable and os.name != "nt":
        snapshot.chmod(snapshot.stat().st_mode | 0o500)
    if _sha256_file(snapshot) != digest:
        raise BuildError(f"snapshotted {label} differs from its source digest")
    return snapshot


def _artifact(path: Path, role: str, receipt_parent: Path) -> dict[str, object]:
    source = _require_file(path, role)
    size = source.stat().st_size
    if size <= 0:
        raise BuildError(f"{role} is empty: {source}")
    return {
        "role": role,
        "path": _artifact_path(source, receipt_parent),
        "size": size,
        "digest": _sha256_file(source),
    }


def _linked_closure(artifacts: list[dict[str, object]]) -> dict[str, object]:
    rows: list[bytes] = []
    for artifact in artifacts:
        role = artifact.get("role")
        path = artifact.get("path")
        size = artifact.get("size")
        digest = artifact.get("digest")
        if (
            not isinstance(role, str)
            or not isinstance(path, str)
            or isinstance(size, bool)
            or not isinstance(size, int)
            or size <= 0
            or not isinstance(digest, str)
            or not HEX64.fullmatch(digest)
            or "\x00" in role + path
            or "\n" in role + path
        ):
            raise BuildError("invalid link-closure artifact")
        rows.append(f"{role}\0{path}\0{size}\0{digest}\n".encode("utf-8"))
    if not rows:
        raise BuildError("linked-library closure cannot be empty")
    digest = hashlib.sha256(b"".join(sorted(rows))).hexdigest()
    return {
        "algorithm": "sha256-nul-tuples-v1",
        "digest": digest,
        "artifacts": artifacts,
    }


def _write_receipt(path: Path, receipt: dict[str, object]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = (
        json.dumps(
            receipt,
            indent=2,
            sort_keys=True,
            ensure_ascii=False,
            allow_nan=False,
        )
        + "\n"
    )
    temporary_name: str | None = None
    try:
        with tempfile.NamedTemporaryFile(
            mode="w",
            encoding="utf-8",
            newline="\n",
            prefix="." + path.name + ".",
            suffix=".tmp",
            dir=path.parent,
            delete=False,
        ) as temporary:
            temporary_name = temporary.name
            temporary.write(payload)
            temporary.flush()
            os.fsync(temporary.fileno())
        os.replace(temporary_name, path)
    finally:
        if temporary_name is not None:
            try:
                Path(temporary_name).unlink()
            except FileNotFoundError:
                pass


def _receipt(
    *,
    name: str,
    side: str,
    package: str,
    commit: str,
    adapter_source: Path,
    config_digest: str,
    executable_path: Path,
    library_path: Path,
    receipt_path: Path,
    build_data: dict[str, object],
    target_lock: dict[str, object] | None,
) -> dict[str, object]:
    if side not in {"target", "standalone"}:
        raise BuildError(f"invalid adapter side: {side}")
    if not HEX40.fullmatch(commit):
        raise BuildError(f"{side} commit must be lowercase 40-hex")
    if not HEX64.fullmatch(config_digest):
        raise BuildError("adapter config digest must be lowercase 64-hex")
    parent = receipt_path.resolve().parent
    executable = _artifact(executable_path, "adapter-executable", parent)
    library = _artifact(library_path, "libdxfrw-static", parent)
    closure = _linked_closure([library])
    adapter = {
        "name": name,
        "side": side,
        "package": package,
        "commit": commit,
        "sourceDigest": _sha256_file(_require_file(adapter_source, "adapter source")),
        "configDigest": config_digest,
        "staticLibraryDigest": library["digest"],
        "binaryDigest": executable["digest"],
        "linkedClosureDigest": closure["digest"],
    }
    return {
        "schema": 1,
        "kind": "libdxfrw-semantic-adapter-build-receipt",
        "adapter": adapter,
        "targetLock": target_lock,
        "artifacts": {
            "executable": {
                key: executable[key] for key in ("path", "size", "digest")
            },
            "staticLibrary": {
                key: library[key] for key in ("path", "size", "digest")
            },
        },
        "linkClosure": closure,
        "build": build_data,
    }


def build_target(args: argparse.Namespace) -> tuple[dict[str, object], Path]:
    build_dir = args.build_dir.resolve()
    build_dir.mkdir(parents=True, exist_ok=True)
    lock_path = _require_file(args.lock, "target lock")
    manifest_path = _require_file(args.manifest, "target source manifest")
    semantic_manifest_path = _require_file(
        args.semantic_manifest, "semantic differential manifest"
    )
    adapter_path = _require_file(args.adapter_source, "adapter source")
    wrapper_path = _require_directory(args.wrapper_source, "target adapter wrapper")
    wrapper_file_path = _require_file(
        wrapper_path / "CMakeLists.txt", "target adapter wrapper"
    )
    inputs = build_dir / "inputs"
    lock_snapshot, lock_digest = _snapshot_input(
        lock_path, inputs / "lock", "libdxfrw-target-lock.json", "target lock"
    )
    manifest_snapshot, _ = _snapshot_input(
        manifest_path,
        inputs / "manifest",
        "libdxfrw-target-source-manifest.txt",
        "target source manifest",
    )
    semantic_manifest_snapshot, semantic_manifest_digest = _snapshot_input(
        semantic_manifest_path,
        inputs / "semantic-manifest",
        "qualified-differential-v2.json",
        "semantic differential manifest",
    )
    _read_json(semantic_manifest_snapshot, "semantic differential manifest")
    adapter_source, adapter_source_digest = _snapshot_input(
        adapter_path,
        inputs / "adapter",
        "semantic_differential_adapter.cpp",
        "adapter source",
    )
    adapter_support_inputs = _snapshot_adapter_support_inputs(
        list(getattr(args, "adapter_support_input", None) or []),
        adapter_source,
    )
    wrapper_file, wrapper_digest = _snapshot_input(
        wrapper_file_path,
        inputs / "wrapper",
        "CMakeLists.txt",
        "target adapter wrapper",
    )
    wrapper = wrapper_file.parent
    lock = _read_json(lock_snapshot, "target lock")
    lock_data = _validate_lock(lock)
    manifest = _read_manifest(manifest_snapshot, lock_data)
    source_cache = _materialize_target_source(
        build_dir=build_dir,
        git_program=args.git,
        git_dir=args.target_git_dir,
        lock_data=lock_data,
        manifest=manifest,
    )
    target_root = source_cache / "libdxfrw/libraries/libdxfrw"
    _require_directory(target_root / "src", "extracted target source root")
    _require_file(
        target_root / "libdxfrw_sources.cmake", "extracted target source list"
    )
    interface_identity = _verify_interface_contract(
        target_root / "src/drw_interface.h",
        adapter_source,
        side="target",
        profile=getattr(args, "interface_profile", INTERFACE_PROFILE_NAME),
    )
    parent_settings = _parent_cmake_settings(
        getattr(args, "parent_cmake_build_dir", None), args.configuration
    )
    driver_environment, driver_context = _controlled_driver_environment(
        parent_settings, cmake_program=args.cmake
    )

    build_key = _canonical_digest(
        {
            "schema": 1,
            "targetArchiveDigest": lock_data["archiveDigest"],
            "adapterSourceDigest": adapter_source_digest,
            "adapterSupportInputs": adapter_support_inputs,
            "wrapperDigest": wrapper_digest,
            "semanticManifestDigest": semantic_manifest_digest,
            "configuration": args.configuration,
            "parentCMake": parent_settings,
            "controlledDriverContext": driver_context,
        }
    )
    build_key_root = build_dir / "cmake" / build_key
    build_key_root.mkdir(parents=True, exist_ok=True)
    build_tree = Path(tempfile.mkdtemp(
        prefix="run-", dir=build_key_root
    )).resolve()
    _request_codemodel(build_tree)
    _configure_target(
        cmake_program=args.cmake,
        wrapper=wrapper,
        build_tree=build_tree,
        target_root=target_root,
        adapter_source=adapter_source,
        commit=str(lock_data["commit"]),
        configuration=args.configuration,
        config_digest=semantic_manifest_digest,
        parent_settings=parent_settings,
        env=driver_environment,
        driver_context=driver_context,
    )
    cmake_data = _cmake_metadata(
        build_tree, args.cmake, args.configuration,
        env=driver_environment, driver_context=driver_context,
        controlled_fresh_build=True,
    )
    compiler_dependencies = _compiler_dependency_profile(
        build_tree=build_tree,
        configuration=args.configuration,
        cmake_program=args.cmake,
        cmake_data=cmake_data,
        executable_target=TARGET_EXECUTABLE,
        library_target=TARGET_LIBRARY,
        env=driver_environment,
        driver_context=driver_context,
    )
    compiler_resources = _compiler_resource_profile(
        cmake_data, env=driver_environment
    )
    apple_sdk = _apple_sdk_profile(cmake_data)
    _verify_requested_configuration(build_tree, args.configuration)
    build_command = [
        args.cmake,
        "--build",
        str(build_tree),
        "--target",
        TARGET_EXECUTABLE,
        "--config",
        args.configuration,
    ]
    if args.parallel is not None:
        if args.parallel <= 0:
            raise BuildError("--parallel must be positive")
        build_command.extend(["--parallel", str(args.parallel)])
    _verify_driver_binary(driver_context)
    _verify_tool_binary_identities(cmake_data.get("toolBinaries"))
    _verify_compiler_builtin_specs_profile(
        cmake_data, env=driver_environment, driver_context=driver_context
    )
    _run(build_command, env=driver_environment)
    _verify_tool_binary_identities(cmake_data.get("toolBinaries"))
    _verify_driver_binary(driver_context)
    _verify_compiler_builtin_specs_profile(
        cmake_data, env=driver_environment, driver_context=driver_context
    )
    _verify_compiler_dependency_profile(
        compiler_dependencies, build_tree=build_tree
    )
    _verify_compiler_resource_profile(compiler_resources)
    _verify_apple_sdk_profile(apple_sdk)
    executable, library = _target_artifacts(
        build_tree, args.configuration, args.cmake, env=driver_environment,
        driver_context=driver_context,
    )
    capability_profile = _probe_capability_profile(
        executable, "target", driver_environment
    )
    apple_runtime = _apple_runtime_profile(
        executable, apple_sdk,
        system_name=str(cmake_data.get("systemName", "")),
        compiler_id=str(cmake_data.get("compilerId", "")),
        env=driver_environment,
    )
    _verify_apple_runtime_profile(apple_runtime, executable)
    apple_link_inputs = _apple_link_inputs_profile(
        map_path=build_tree / "semantic-adapter-link.map",
        build_tree=build_tree,
        executable_path=executable,
        library_path=library,
        sdk_profile=apple_sdk,
        system_name=str(cmake_data.get("systemName", "")),
        compiler_id=str(cmake_data.get("compilerId", "")),
    )
    _verify_apple_link_inputs_profile(
        apple_link_inputs,
        map_path=build_tree / "semantic-adapter-link.map",
        build_tree=build_tree,
        executable_path=executable,
        library_path=library,
        sdk_profile=apple_sdk,
        system_name=str(cmake_data.get("systemName", "")),
        compiler_id=str(cmake_data.get("compilerId", "")),
    )
    apple_runtime = _bind_apple_runtime_selected_stubs(
        apple_runtime, apple_link_inputs
    )
    _verify_apple_runtime_profile(apple_runtime, executable)
    link_identity = _verify_link_closure(
        build_tree=build_tree,
        configuration=args.configuration,
        executable_target=TARGET_EXECUTABLE,
        library_target=TARGET_LIBRARY,
        library_path=library,
        cmake_program=args.cmake,
        env=driver_environment,
        driver_context=driver_context,
    )
    compile_identity = _verify_adapter_compile_contract(
        build_tree=build_tree,
        configuration=args.configuration,
        executable_target=TARGET_EXECUTABLE,
        adapter_source=adapter_source,
        side="target",
        adapter_name=TARGET_EXECUTABLE,
        adapter_commit=str(lock_data["commit"]),
        config_digest=semantic_manifest_digest,
        cmake_program=args.cmake,
        env=driver_environment,
        driver_context=driver_context,
        controlled_fresh_build=True,
    )
    build_parity_identity = _build_parity_identity(
        build_tree=build_tree,
        configuration=args.configuration,
        cmake_program=args.cmake,
        cmake_data=cmake_data,
        executable_target=TARGET_EXECUTABLE,
        library_target=TARGET_LIBRARY,
        library_path=library,
        env=driver_environment,
        driver_context=driver_context,
        compiler_dependencies=compiler_dependencies,
        compiler_resources=compiler_resources,
        apple_sdk=apple_sdk,
        apple_runtime=apple_runtime,
        apple_link_inputs=apple_link_inputs,
    )
    build_parity_digest = _canonical_digest(build_parity_identity)

    config_files = [
        {
            "path": _logical_input_path(wrapper_file_path),
            "digest": wrapper_digest,
        }
    ]
    config_identity = {
        "schema": 1,
        "side": "target",
        "configuration": args.configuration,
        "cmake": cmake_data,
        "inputs": {
            "adapterSourceDigest": adapter_source_digest,
            "adapterSupportInputs": adapter_support_inputs,
            "semanticManifestPath": _logical_input_path(semantic_manifest_path),
            "semanticManifestDigest": semantic_manifest_digest,
            "packageCommit": lock_data["commit"],
            "packageArchiveDigest": lock_data["archiveDigest"],
            "configFiles": config_files,
            "driverContext": driver_context,
            **link_identity,
            **compile_identity,
            **interface_identity,
            "capabilityProfile": capability_profile,
        },
        "definitions": {
            "sideMacro": "LIBDXFRW_SEMANTIC_SIDE_TARGET=1",
            "adapterName": TARGET_EXECUTABLE,
            "adapterCommit": lock_data["commit"],
            "embeddedConfigDigest": semantic_manifest_digest,
        },
        "warningPolicy": {
            "library": "native-nonfatal",
            "adapter": "warnings-as-errors",
        },
        "targets": {
            "executable": TARGET_EXECUTABLE,
            "staticLibrary": TARGET_LIBRARY,
        },
    }
    build_config_digest = _canonical_digest(config_identity)

    receipt_path = (
        args.receipt.resolve()
        if args.receipt is not None
        else build_dir / "semantic-adapter-target-receipt.json"
    )
    lock_receipt = {
        "path": _logical_input_path(lock_path),
        "digest": lock_digest,
        "targetCommit": lock_data["commit"],
        "snapshotRevision": lock_data["snapshotRevision"],
        "archiveDigest": lock_data["archiveDigest"],
        "manifestDigest": lock_data["manifestDigest"],
        "manifestEntries": lock_data["manifestEntries"],
    }
    build_data = {
        "configuration": args.configuration,
        "cmakeVersion": cmake_data["cmakeVersion"],
        "generator": cmake_data["generator"],
        "compilerId": cmake_data["compilerId"],
        "compilerVersion": cmake_data["compilerVersion"],
        "wrapperDigest": wrapper_digest,
        "buildConfigDigest": build_config_digest,
        "configIdentity": config_identity,
        "buildParityIdentity": build_parity_identity,
        "buildParityDigest": build_parity_digest,
    }
    receipt = _receipt(
        name=TARGET_EXECUTABLE,
        side="target",
        package="LibreCAD-bundled-libdxfrw",
        commit=str(lock_data["commit"]),
        adapter_source=adapter_source,
        config_digest=semantic_manifest_digest,
        executable_path=executable,
        library_path=library,
        receipt_path=receipt_path,
        build_data=build_data,
        target_lock=lock_receipt,
    )
    _write_receipt(receipt_path, receipt)
    return receipt, receipt_path


def _git_head(git_program: str, work_tree: Path) -> str:
    value = _run([git_program, "-C", str(work_tree), "rev-parse", "HEAD"]).strip()
    if not HEX40.fullmatch(value):
        raise BuildError(f"standalone HEAD is not a lowercase 40-hex commit: {value}")
    return value


def _standalone_source_lists(source_list: Path) -> dict[str, list[str]]:
    """Read only the closed source-list grammar used by libdxfrw_sources.cmake.

    The standalone qualification wrapper consumes these names as data.  It
    never includes or executes a source-list file from the working tree.
    """
    text = _require_file(source_list, "standalone source list").read_text(
        encoding="utf-8", errors="strict"
    )
    lines = text.splitlines()
    expected = {
        "LIBDXFRW_SOURCES",
        "LIBDXFRW_PRIVATE_HEADERS",
        "LIBDXFRW_PUBLIC_HEADERS",
        "LIBDXFRW_ALL_FILES",
    }
    parsed: dict[str, list[str]] = {}
    index = 0
    while index < len(lines):
        line = lines[index].strip()
        if not line or line.startswith("#"):
            index += 1
            continue
        match = re.fullmatch(r"set\((LIBDXFRW_[A-Z_]+)", line)
        if match is None or match.group(1) not in expected:
            raise BuildError("standalone source list uses an unsupported CMake construct")
        name = match.group(1)
        if name in parsed:
            raise BuildError(f"standalone source list duplicates {name}")
        index += 1
        values: list[str] = []
        while index < len(lines) and lines[index].strip() != ")":
            item = lines[index].strip()
            index += 1
            if not item or item.startswith("#"):
                continue
            if name == "LIBDXFRW_ALL_FILES":
                if item not in {
                    "${LIBDXFRW_SOURCES}",
                    "${LIBDXFRW_PRIVATE_HEADERS}",
                    "${LIBDXFRW_PUBLIC_HEADERS}",
                }:
                    raise BuildError("standalone source list has an invalid all-files entry")
                values.append(item)
                continue
            value = re.fullmatch(
                r'"\$\{CMAKE_CURRENT_LIST_DIR\}/src/([^"\\]+)"', item
            )
            if value is None:
                raise BuildError(
                    f"standalone source list has an unsupported {name} entry"
                )
            relative = PurePosixPath(value.group(1))
            if (
                relative.is_absolute()
                or any(part in {"", ".", ".."} for part in relative.parts)
                or "\\" in value.group(1)
                or "\x00" in value.group(1)
            ):
                raise BuildError("standalone source list contains an unsafe source path")
            values.append(relative.as_posix())
        if index >= len(lines) or lines[index].strip() != ")":
            raise BuildError(f"standalone source list has an unterminated {name}")
        index += 1
        parsed[name] = values

    if not {"LIBDXFRW_SOURCES"}.issubset(parsed):
        raise BuildError("standalone source list omits LIBDXFRW_SOURCES")
    for name in ("LIBDXFRW_PRIVATE_HEADERS", "LIBDXFRW_PUBLIC_HEADERS"):
        parsed.setdefault(name, [])
    expected_all = [
        "${LIBDXFRW_SOURCES}",
        "${LIBDXFRW_PRIVATE_HEADERS}",
        "${LIBDXFRW_PUBLIC_HEADERS}",
    ]
    if "LIBDXFRW_ALL_FILES" in parsed and parsed["LIBDXFRW_ALL_FILES"] != expected_all:
        raise BuildError("standalone source list has a noncanonical all-files inventory")
    if not parsed["LIBDXFRW_SOURCES"]:
        raise BuildError("standalone source list contains no C++ sources")
    for name in ("LIBDXFRW_SOURCES", "LIBDXFRW_PRIVATE_HEADERS", "LIBDXFRW_PUBLIC_HEADERS"):
        values = parsed[name]
        if len(values) != len(set(values)):
            raise BuildError(f"standalone source list duplicates a path in {name}")
    return parsed


def _standalone_cmake_wrapper(source_lists: dict[str, list[str]]) -> str:
    def render(name: str) -> str:
        rows = source_lists[name]
        if not rows:
            return ""
        values = "\n".join(
            f'    "${{CMAKE_CURRENT_SOURCE_DIR}}/src/{value}"' for value in rows
        )
        return f"set(_{name.lower()}\n{values}\n)\n"

    return (
        "cmake_minimum_required(VERSION 3.28)\n"
        "project(libdxfrw_semantic_standalone LANGUAGES CXX)\n"
        "foreach(_name IN ITEMS CMAKE_CXX_COMPILER_ARG1 "
        "CMAKE_CXX_COMPILER_LAUNCHER CMAKE_CXX_LINKER_LAUNCHER "
        "CMAKE_RULE_LAUNCH_COMPILE CMAKE_RULE_LAUNCH_LINK)\n"
        "  if(DEFINED ${_name} AND NOT \"${${_name}}\" STREQUAL \"\")\n"
        "    message(FATAL_ERROR \"unsupported compiler/build channel: ${_name}\")\n"
        "  endif()\n"
        "endforeach()\n"
        "get_property(_global_compile_launcher GLOBAL PROPERTY RULE_LAUNCH_COMPILE)\n"
        "get_property(_global_link_launcher GLOBAL PROPERTY RULE_LAUNCH_LINK)\n"
        "if(NOT \"${_global_compile_launcher}\" STREQUAL \"\" OR "
        "NOT \"${_global_link_launcher}\" STREQUAL \"\")\n"
        "  message(FATAL_ERROR \"unsupported global rule launcher\")\n"
        "endif()\n"
        "set(CMAKE_CXX_STANDARD 17)\n"
        "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
        "set(CMAKE_CXX_EXTENSIONS OFF)\n"
        + render("LIBDXFRW_SOURCES")
        + render("LIBDXFRW_PRIVATE_HEADERS")
        + render("LIBDXFRW_PUBLIC_HEADERS")
        + "find_package(Threads REQUIRED)\n"
        "add_library(dxfrw STATIC ${_libdxfrw_sources} "
        "${_libdxfrw_private_headers} ${_libdxfrw_public_headers})\n"
        "target_compile_features(dxfrw PUBLIC cxx_std_17)\n"
        "target_include_directories(dxfrw PUBLIC \"${CMAKE_CURRENT_SOURCE_DIR}/src\" "
        "PRIVATE \"${CMAKE_CURRENT_SOURCE_DIR}/src/intern\")\n"
        "target_link_libraries(dxfrw PUBLIC Threads::Threads)\n"
        "if(MSVC)\n"
        "  target_compile_options(dxfrw PRIVATE /W3 /bigobj)\n"
        "  set(_adapter_warnings /W3 /WX /bigobj)\n"
        "else()\n"
        "  target_compile_options(dxfrw PRIVATE -Wall -Wextra -pedantic -Werror)\n"
        "  set(_adapter_warnings -Wall -Wextra -pedantic -Werror)\n"
        "  if(CMAKE_CXX_COMPILER_ID STREQUAL \"GNU\")\n"
        "    set_source_files_properties(\"${CMAKE_CURRENT_SOURCE_DIR}/src/drw_entities.cpp\" "
        "PROPERTIES COMPILE_OPTIONS -Wno-extra)\n"
        "  endif()\n"
        "endif()\n"
        "if(CMAKE_CXX_COMPILER_ID MATCHES \"Clang\")\n"
        "  target_compile_options(dxfrw PRIVATE --no-default-config)\n"
        "  list(APPEND _adapter_warnings --no-default-config)\n"
        "endif()\n"
        "add_executable(libdxfrw_semantic_adapter_standalone "
        "\"${CMAKE_CURRENT_SOURCE_DIR}/tests/semantic_differential_adapter.cpp\")\n"
        "target_compile_features(libdxfrw_semantic_adapter_standalone PRIVATE cxx_std_17)\n"
        "target_compile_options(libdxfrw_semantic_adapter_standalone PRIVATE ${_adapter_warnings})\n"
        "target_compile_definitions(libdxfrw_semantic_adapter_standalone PRIVATE "
        "LIBDXFRW_SEMANTIC_SIDE_STANDALONE=1 "
        "\"LIBDXFRW_SEMANTIC_ADAPTER_NAME=\\\"libdxfrw_semantic_adapter_standalone\\\"\")\n"
        "target_include_directories(libdxfrw_semantic_adapter_standalone PRIVATE "
        "\"${CMAKE_CURRENT_SOURCE_DIR}/src\" "
        "\"${CMAKE_CURRENT_SOURCE_DIR}/src/intern\")\n"
        "target_link_libraries(libdxfrw_semantic_adapter_standalone PRIVATE dxfrw)\n"
        "if(CMAKE_SYSTEM_NAME STREQUAL \"Darwin\" AND "
        "CMAKE_CXX_COMPILER_ID MATCHES \"Clang\")\n"
        "  target_link_options(libdxfrw_semantic_adapter_standalone PRIVATE "
        "\"-Wl,-map,${CMAKE_BINARY_DIR}/semantic-adapter-link.map\")\n"
        "endif()\n"
        "foreach(_target IN ITEMS dxfrw libdxfrw_semantic_adapter_standalone)\n"
        "  foreach(_property IN ITEMS CXX_COMPILER_LAUNCHER CXX_LINKER_LAUNCHER)\n"
        "    get_target_property(_value ${_target} ${_property})\n"
        "    if(_value AND NOT _value MATCHES \"-NOTFOUND$\")\n"
        "      message(FATAL_ERROR \"unsupported target launcher: ${_target}.${_property}\")\n"
        "    endif()\n"
        "  endforeach()\n"
        "endforeach()\n"
    )


def _snapshot_standalone_inputs(
    source_root: Path,
    source_list: Path,
    adapter_source: Path,
    semantic_manifest: Path,
    destination_root: Path,
    adapter_support_inputs: list[Path] | None = None,
) -> tuple[Path, dict[str, object]]:
    root = _require_directory(source_root, "standalone source root")
    lists = _standalone_source_lists(source_list)
    records: list[dict[str, str]] = []
    payloads: dict[str, bytes] = {}
    src_root = _require_directory(root / "src", "standalone src directory")
    for name in (
        "LIBDXFRW_SOURCES", "LIBDXFRW_PRIVATE_HEADERS",
        "LIBDXFRW_PUBLIC_HEADERS",
    ):
        for relative in lists[name]:
            listed = src_root.joinpath(*PurePosixPath(relative).parts)
            _require_file(listed, f"standalone {name} entry")
    for path in sorted(src_root.rglob("*"), key=lambda value: value.as_posix()):
        if path.is_symlink():
            raise BuildError("standalone source tree contains a symlink")
        if path.is_dir():
            continue
        if not path.is_file():
            raise BuildError("standalone source tree contains a non-regular file")
        relative = path.relative_to(root).as_posix()
        payload = path.read_bytes()
        payloads[relative] = payload
        records.append({"path": relative, "digest": _sha256_bytes(payload)})
    source_list_path = _require_file(source_list, "standalone source list")
    if source_list_path.resolve().parent != root:
        raise BuildError("standalone source list must be at the source root")
    if source_list_path.name != "libdxfrw_sources.cmake":
        raise BuildError("standalone source list must be libdxfrw_sources.cmake")
    source_list_payload = source_list_path.read_bytes()
    payloads[source_list_path.name] = source_list_payload
    records.append({"path": source_list_path.name,
                    "digest": _sha256_bytes(source_list_payload)})
    adapter_payload = _require_file(adapter_source, "adapter source").read_bytes()
    payloads["tests/semantic_differential_adapter.cpp"] = adapter_payload
    records.append({"path": "tests/semantic_differential_adapter.cpp",
                    "digest": _sha256_bytes(adapter_payload)})
    for support_path in adapter_support_inputs or []:
        support = _require_file(support_path, "adapter support input")
        relative = PurePosixPath("tests") / support.name
        relative_text = relative.as_posix()
        if relative_text in payloads:
            raise BuildError("adapter support input collides in standalone snapshot")
        payload = support.read_bytes()
        payloads[relative_text] = payload
        records.append({"path": relative_text, "digest": _sha256_bytes(payload)})
    manifest_payload = _require_file(
        semantic_manifest, "semantic differential manifest"
    ).read_bytes()
    payloads["metadata/qualified-differential-v2.json"] = manifest_payload
    records.append({"path": "metadata/qualified-differential-v2.json",
                    "digest": _sha256_bytes(manifest_payload)})
    wrapper_payload = _standalone_cmake_wrapper(lists).encode("utf-8")
    payloads["CMakeLists.txt"] = wrapper_payload
    records.append({"path": "CMakeLists.txt",
                    "digest": _sha256_bytes(wrapper_payload)})
    records.sort(key=lambda row: row["path"])
    input_digest = _canonical_digest({"schema": 1, "files": records})
    snapshot = destination_root / input_digest
    snapshot.mkdir(parents=True, exist_ok=True)
    for path in snapshot.rglob("*"):
        if path.is_symlink():
            raise BuildError("standalone source snapshot contains a symlink")
    for relative, payload in payloads.items():
        destination = snapshot.joinpath(*PurePosixPath(relative).parts)
        destination.parent.mkdir(parents=True, exist_ok=True)
        digest = _sha256_bytes(payload)
        if destination.exists():
            if not destination.is_file() or _sha256_file(destination) != digest:
                raise BuildError("cached standalone source snapshot differs from its digest")
        else:
            try:
                with destination.open("xb") as stream:
                    stream.write(payload)
            except FileExistsError:
                if not destination.is_file() or _sha256_file(destination) != digest:
                    raise BuildError("raced standalone source snapshot differs from its digest")
    actual_files = {
        path.relative_to(snapshot).as_posix()
        for path in snapshot.rglob("*")
        if path.is_file()
    }
    if actual_files != set(payloads):
        raise BuildError("standalone source snapshot contains unbound files")
    return snapshot, {
        "schema": 1,
        "digest": input_digest,
        "fileCount": len(records),
        "files": records,
    }


def receipt_standalone(args: argparse.Namespace) -> tuple[dict[str, object], Path]:
    adapter_source = _require_file(args.adapter_source, "adapter source")
    semantic_manifest = _require_file(
        args.semantic_manifest, "semantic differential manifest"
    )
    _read_json(semantic_manifest, "semantic differential manifest")
    semantic_manifest_digest = _sha256_file(semantic_manifest)
    executable = _require_file(args.executable, "standalone adapter executable")
    library = _require_file(args.static_library, "standalone static library")
    build_tree = _require_directory(args.cmake_build_dir, "standalone CMake build")
    commit = args.commit or _git_head(args.git, REPOSITORY_ROOT)
    if not HEX40.fullmatch(commit):
        raise BuildError("standalone commit must be lowercase 40-hex")
    config_sources = args.config_source or [
        REPOSITORY_ROOT / "CMakeLists.txt",
        REPOSITORY_ROOT / "libdxfrw_sources.cmake",
    ]
    config_files = [
        _require_file(path, "standalone config source") for path in config_sources
    ]
    cache = _parse_cmake_cache(build_tree)
    cmake_home = cache.get("CMAKE_HOME_DIRECTORY", "")
    if not cmake_home:
        raise BuildError("standalone CMake cache omits CMAKE_HOME_DIRECTORY")
    cmake_home_path = Path(cmake_home).resolve()
    resolved_config_files = {path.resolve() for path in config_files}
    if (cmake_home_path / "CMakeLists.txt").resolve() not in resolved_config_files:
        raise BuildError("standalone receipt omits its root CMakeLists.txt")
    for config_file in resolved_config_files:
        try:
            config_file.relative_to(cmake_home_path)
        except ValueError:
            raise BuildError(
                "standalone receipt config source is outside CMAKE_HOME_DIRECTORY"
            ) from None
    driver_context = getattr(args, "driver_context", None)
    driver_environment = getattr(args, "driver_environment", None)
    if driver_context is not None:
        _verify_driver_binary(driver_context)
    cmake_data = _cmake_metadata(
        build_tree, args.cmake, args.configuration,
        env=driver_environment, driver_context=driver_context,
    )
    _verify_requested_configuration(build_tree, args.configuration)
    config_file_records = sorted(
        getattr(args, "config_file_records", None)
        or [
            {
                "path": _logical_input_path(path),
                "digest": _sha256_file(path),
            }
            for path in config_files
        ],
        key=lambda row: (str(row["path"]), str(row["digest"])),
    )
    link_identity = _verify_link_closure(
        build_tree=build_tree,
        configuration=args.configuration,
        executable_target=STANDALONE_EXECUTABLE,
        library_target="dxfrw",
        library_path=library,
        cmake_program=args.cmake,
        env=driver_environment,
        driver_context=driver_context,
    )
    compile_identity = _verify_adapter_compile_contract(
        build_tree=build_tree,
        configuration=args.configuration,
        executable_target=STANDALONE_EXECUTABLE,
        adapter_source=adapter_source,
        side="standalone",
        adapter_name=STANDALONE_EXECUTABLE,
        adapter_commit=None,
        config_digest=None,
        cmake_program=args.cmake,
        env=driver_environment,
        driver_context=driver_context,
        controlled_fresh_build=getattr(args, "controlled_fresh_build", False),
    )
    interface_identity = _verify_interface_contract(
        cmake_home_path / "src/drw_interface.h",
        adapter_source,
        side="standalone",
        profile=getattr(args, "interface_profile", INTERFACE_PROFILE_NAME),
    )
    capability_profile = _probe_capability_profile(
        executable, "standalone", driver_environment
    )
    adapter_support_identity = getattr(args, "adapter_support_inputs", [])
    if not isinstance(adapter_support_identity, list):
        raise BuildError("standalone adapter support-input identity is malformed")
    for row in adapter_support_identity:
        if (not isinstance(row, dict)
                or set(row) != {"path", "snapshotPath", "digest"}
                or not isinstance(row.get("path"), str)
                or not isinstance(row.get("snapshotPath"), str)
                or not isinstance(row.get("digest"), str)
                or not HEX64.fullmatch(row["digest"])):
            raise BuildError("standalone adapter support-input identity is malformed")
        relative = PurePosixPath(row["snapshotPath"])
        if (relative.is_absolute()
                or any(part in {"", ".", ".."} for part in relative.parts)
                or relative.parts[:1] != ("tests",)):
            raise BuildError("standalone adapter support snapshot path is unsafe")
        support_path = cmake_home_path.joinpath(*relative.parts)
        if not support_path.is_file() or _sha256_file(support_path) != row["digest"]:
            raise BuildError("standalone adapter support snapshot changed")
    compiler_dependencies = getattr(args, "compiler_dependencies", None)
    if compiler_dependencies is None:
        compiler_dependencies = {
            "schema": 1,
            "status": "unavailable",
            "method": "",
            "translationUnitCount": 0,
            "externalInputs": [],
            "externalInputsDigest": _canonical_digest([]),
            "reason": "existing-build-not-scanned-before-compilation",
        }
    compiler_resources = getattr(args, "compiler_resources", None)
    apple_sdk = getattr(args, "apple_sdk", None)
    apple_runtime = getattr(args, "apple_runtime", None)
    apple_link_inputs = getattr(args, "apple_link_inputs", None)
    build_parity_identity = _build_parity_identity(
        build_tree=build_tree,
        configuration=args.configuration,
        cmake_program=args.cmake,
        cmake_data=cmake_data,
        executable_target=STANDALONE_EXECUTABLE,
        library_target="dxfrw",
        library_path=library,
        env=driver_environment,
        driver_context=driver_context,
        compiler_dependencies=compiler_dependencies,
        compiler_resources=compiler_resources,
        apple_sdk=apple_sdk,
        apple_runtime=apple_runtime,
        apple_link_inputs=apple_link_inputs,
    )
    build_parity_digest = _canonical_digest(build_parity_identity)
    config_identity = {
        "schema": 1,
        "side": "standalone",
        "configuration": args.configuration,
        "cmake": cmake_data,
        "inputs": {
            "adapterSourceDigest": _sha256_file(adapter_source),
            "adapterSupportInputs": adapter_support_identity,
            "semanticManifestPath": _logical_input_path(semantic_manifest),
            "semanticManifestDigest": semantic_manifest_digest,
            "packageCommit": commit,
            "packageArchiveDigest": None,
            "configFiles": config_file_records,
            **({"driverContext": driver_context}
               if driver_context is not None else {}),
            **link_identity,
            **compile_identity,
            **interface_identity,
            "capabilityProfile": capability_profile,
        },
        "definitions": {
            "sideMacro": "LIBDXFRW_SEMANTIC_SIDE_STANDALONE=1",
            "adapterName": STANDALONE_EXECUTABLE,
            "adapterCommit": commit,
            "embeddedConfigDigest": None,
        },
        "warningPolicy": {
            "library": "repository-maintainer-policy",
            "adapter": "warnings-as-errors",
        },
        "targets": {
            "executable": STANDALONE_EXECUTABLE,
            "staticLibrary": "dxfrw",
        },
    }
    build_config_digest = _canonical_digest(config_identity)
    receipt_path = (
        args.receipt.resolve()
        if args.receipt is not None
        else build_tree / "semantic-adapter-standalone-receipt.json"
    )
    executable_snapshot = _snapshot_build_artifact(
        executable, receipt_path.parent, "standalone adapter executable",
        executable=True,
    )
    if (isinstance(apple_runtime, dict)
            and _sha256_file(executable_snapshot)
                != apple_runtime.get("executableDigest")):
        raise BuildError("standalone artifact differs from its runtime profile")
    library_snapshot = _snapshot_build_artifact(
        library, receipt_path.parent, "standalone static library",
        executable=False,
    )
    build_data = {
        "configuration": args.configuration,
        "cmakeVersion": cmake_data["cmakeVersion"],
        "generator": cmake_data["generator"],
        "compilerId": cmake_data["compilerId"],
        "compilerVersion": cmake_data["compilerVersion"],
        "wrapperDigest": getattr(args, "wrapper_digest", None)
        or config_file_records[0]["digest"],
        "buildConfigDigest": build_config_digest,
        "configIdentity": config_identity,
        "buildParityIdentity": build_parity_identity,
        "buildParityDigest": build_parity_digest,
    }
    receipt = _receipt(
        name=STANDALONE_EXECUTABLE,
        side="standalone",
        package="standalone-libdxfrw",
        commit=commit,
        adapter_source=adapter_source,
        config_digest=semantic_manifest_digest,
        executable_path=executable_snapshot,
        library_path=library_snapshot,
        receipt_path=receipt_path,
        build_data=build_data,
        target_lock=None,
    )
    if driver_context is not None:
        _verify_driver_binary(driver_context)
    _write_receipt(receipt_path, receipt)
    return receipt, receipt_path


def build_standalone(args: argparse.Namespace) -> tuple[dict[str, object], Path]:
    build_root = args.build_dir.resolve()
    build_root.mkdir(parents=True, exist_ok=True)
    source_root = _require_directory(args.source_root, "standalone source root")
    source_list = _require_file(args.source_list, "standalone source list")
    semantic_manifest = _require_file(
        args.semantic_manifest, "semantic differential manifest"
    )
    _read_json(semantic_manifest, "semantic differential manifest")
    adapter_source = _require_file(args.adapter_source, "adapter source")
    parent_settings = _parent_cmake_settings(
        getattr(args, "parent_cmake_build_dir", None), args.configuration
    )
    driver_environment, driver_context = _controlled_driver_environment(
        parent_settings, cmake_program=args.cmake
    )
    cmake_invocation = shutil.which(
        args.cmake, path=driver_environment.get("PATH")
    )
    if not cmake_invocation:
        raise BuildError("cannot resolve selected CMake executable")
    # Preserve the invoked symlink path: CMake records this path in its File
    # API identity, and target/standalone parity compares that identity.
    cmake_path = Path(cmake_invocation).absolute()
    cmake_version_output = _run([str(cmake_path), "--version"], env=driver_environment)
    cmake_version = cmake_version_output.splitlines()[0].strip()
    if not cmake_version:
        raise BuildError("CMake version output is empty")

    input_root = build_root / "inputs" / "standalone"
    adapter_support_paths = list(
        getattr(args, "adapter_support_input", None) or []
    )
    snapshot, source_identity = _snapshot_standalone_inputs(
        source_root, source_list, adapter_source, semantic_manifest, input_root,
        adapter_support_paths,
    )
    adapter_support_identity = [
        {
            "path": _logical_input_path(path),
            "snapshotPath": (Path("tests") / path.name).as_posix(),
            "digest": _sha256_file(_require_file(path, "adapter support input")),
        }
        for path in adapter_support_paths
    ]
    wrapper = _require_file(snapshot / "CMakeLists.txt", "standalone build wrapper")
    wrapper_digest = _sha256_file(wrapper)
    cmake_digest = _sha256_file(cmake_path.resolve())
    build_key = _canonical_digest({
        "schema": 1,
        "route": "controlled-fresh-standalone-v1",
        "configuration": args.configuration,
        "sourceIdentityDigest": source_identity["digest"],
        "parentCMake": parent_settings,
        "driverContext": driver_context,
        "cmakePathDigest": _sha256_bytes(
            cmake_path.as_posix().encode("utf-8", "surrogatepass")
        ),
        "cmakeBinaryDigest": cmake_digest,
        "cmakeVersion": cmake_version,
    })
    build_parent = build_root / "cmake-standalone"
    build_parent.mkdir(parents=True, exist_ok=True)
    # Every invocation receives a new cache and object directory, even when
    # the content key matches a previous invocation.
    build_tree = Path(tempfile.mkdtemp(
        prefix=build_key[:16] + "-", dir=build_parent
    )).resolve()
    _request_codemodel(build_tree)
    configure = [
        str(cmake_path), "-S", str(snapshot), "-B", str(build_tree),
        "-DCMAKE_CXX_COMPILER=" + str(driver_context["compilerPath"]),
    ]
    if parent_settings is not None:
        configure.extend(["-G", str(parent_settings["generator"])])
        platform = str(parent_settings["generatorPlatform"])
        toolset = str(parent_settings["generatorToolset"])
        if platform:
            configure.extend(["-A", platform])
        if toolset:
            configure.extend(["-T", toolset])
        cache_variables = parent_settings["cacheVariables"]
        if not isinstance(cache_variables, dict):
            raise BuildError("parent CMake cache-variable identity is malformed")
        for key, value in sorted(cache_variables.items()):
            if key == "CMAKE_CXX_COMPILER":
                continue
            configure.append(f"-D{key}={value}")
        if not bool(parent_settings["multiConfig"]):
            configure.append("-DCMAKE_BUILD_TYPE=" + args.configuration)
    else:
        configure.append("-DCMAKE_BUILD_TYPE=" + args.configuration)
    configure.append("-DCMAKE_EXPORT_COMPILE_COMMANDS=ON")
    _verify_driver_binary(driver_context)
    _run(configure, env=driver_environment)
    _verify_driver_binary(driver_context)
    _verify_requested_configuration(build_tree, args.configuration)
    cmake_data = _cmake_metadata(
        build_tree, str(cmake_path), args.configuration,
        env=driver_environment, driver_context=driver_context,
        controlled_fresh_build=True,
    )
    compiler_dependencies = _compiler_dependency_profile(
        build_tree=build_tree,
        configuration=args.configuration,
        cmake_program=str(cmake_path),
        cmake_data=cmake_data,
        executable_target=STANDALONE_EXECUTABLE,
        library_target="dxfrw",
        env=driver_environment,
        driver_context=driver_context,
    )
    compiler_resources = _compiler_resource_profile(
        cmake_data, env=driver_environment
    )
    apple_sdk = _apple_sdk_profile(cmake_data)
    command = [
        str(cmake_path), "--build", str(build_tree),
        "--target", STANDALONE_EXECUTABLE,
        "--config", args.configuration,
    ]
    if args.parallel is not None:
        if args.parallel <= 0:
            raise BuildError("--parallel must be positive")
        command.extend(["--parallel", str(args.parallel)])
    _verify_driver_binary(driver_context)
    _verify_tool_binary_identities(cmake_data.get("toolBinaries"))
    _verify_compiler_builtin_specs_profile(
        cmake_data, env=driver_environment, driver_context=driver_context
    )
    _run(command, env=driver_environment)
    _verify_tool_binary_identities(cmake_data.get("toolBinaries"))
    _verify_driver_binary(driver_context)
    _verify_compiler_builtin_specs_profile(
        cmake_data, env=driver_environment, driver_context=driver_context
    )
    _verify_compiler_dependency_profile(
        compiler_dependencies, build_tree=build_tree
    )
    _verify_compiler_resource_profile(compiler_resources)
    _verify_apple_sdk_profile(apple_sdk)
    _, targets = _ensure_codemodel(
        build_tree, args.configuration, str(cmake_path),
        env=driver_environment, driver_context=driver_context,
    )
    executable_target = targets.get(STANDALONE_EXECUTABLE)
    library_target = targets.get("dxfrw")
    if executable_target is None or executable_target.get("type") != "EXECUTABLE":
        raise BuildError("standalone CMake codemodel omits the adapter executable")
    if library_target is None or library_target.get("type") != "STATIC_LIBRARY":
        raise BuildError("standalone CMake codemodel omits the static library")
    executable = _find_one(
        _target_artifact_paths(build_tree, executable_target, STANDALONE_EXECUTABLE),
        "standalone adapter executable",
    )
    library = _find_one(
        _target_artifact_paths(build_tree, library_target, "dxfrw"),
        "standalone static library",
    )
    apple_link_inputs = _apple_link_inputs_profile(
        map_path=build_tree / "semantic-adapter-link.map",
        build_tree=build_tree,
        executable_path=executable,
        library_path=library,
        sdk_profile=apple_sdk,
        system_name=str(cmake_data.get("systemName", "")),
        compiler_id=str(cmake_data.get("compilerId", "")),
    )
    _verify_apple_link_inputs_profile(
        apple_link_inputs,
        map_path=build_tree / "semantic-adapter-link.map",
        build_tree=build_tree,
        executable_path=executable,
        library_path=library,
        sdk_profile=apple_sdk,
        system_name=str(cmake_data.get("systemName", "")),
        compiler_id=str(cmake_data.get("compilerId", "")),
    )
    commit = args.commit or _git_head(args.git, source_root)
    if not HEX40.fullmatch(commit):
        raise BuildError("standalone commit must be lowercase 40-hex")
    source_list_digest = _sha256_file(snapshot / source_list.name)
    config_file_records = [
        {"path": "CMakeLists.txt",
         "digest": wrapper_digest},
        {"path": source_list.name, "digest": source_list_digest},
        {"path": "controlled-standalone-source-tree-v1",
         "digest": str(source_identity["digest"])},
        {"path": "controlled-standalone-build-key-v1", "digest": build_key},
    ]
    standalone_args = argparse.Namespace(
        adapter_source=snapshot / "tests/semantic_differential_adapter.cpp",
        semantic_manifest=snapshot / "metadata/qualified-differential-v2.json",
        executable=executable,
        static_library=library,
        cmake_build_dir=build_tree,
        commit=commit,
        git=args.git,
        cmake=str(cmake_path),
        config_source=[wrapper, snapshot / source_list.name],
        configuration=args.configuration,
        receipt=args.receipt,
        interface_profile=args.interface_profile,
        driver_context=driver_context,
        driver_environment=driver_environment,
        controlled_fresh_build=True,
        config_file_records=config_file_records,
        wrapper_digest=wrapper_digest,
        adapter_support_inputs=adapter_support_identity,
        compiler_dependencies=compiler_dependencies,
        compiler_resources=compiler_resources,
        apple_sdk=apple_sdk,
        apple_runtime=None,
        apple_link_inputs=apple_link_inputs,
    )
    standalone_args.apple_runtime = _apple_runtime_profile(
        executable, apple_sdk,
        system_name=str(cmake_data.get("systemName", "")),
        compiler_id=str(cmake_data.get("compilerId", "")),
        env=driver_environment,
    )
    standalone_args.apple_runtime = _bind_apple_runtime_selected_stubs(
        standalone_args.apple_runtime, apple_link_inputs
    )
    _verify_apple_runtime_profile(standalone_args.apple_runtime, executable)
    return receipt_standalone(standalone_args)


def _self_test_repository(root: Path, git_program: str) -> tuple[Path, str, Path, Path]:
    repository = root / "repository"
    source = repository / "libraries/libdxfrw/src"
    source.mkdir(parents=True)
    (source / "intern").mkdir()
    (source / "minimal.h").write_text(
        "#pragma once\nint semantic_self_test_value();\n", encoding="utf-8"
    )
    (source / "minimal.cpp").write_text(
        '#include "minimal.h"\nint semantic_self_test_value() { return 42; }\n',
        encoding="utf-8",
    )
    (source / "drw_interface.h").write_text(
        "#pragma once\n"
        "class DRW_Interface {\n"
        "public:\n"
        "    virtual ~DRW_Interface() = default;\n"
        "    virtual void callback() {}\n"
        "};\n",
        encoding="utf-8",
    )
    source_list = repository / "libraries/libdxfrw/libdxfrw_sources.cmake"
    source_list.write_text(
        "set(LIBDXFRW_SOURCES\n"
        '    "${CMAKE_CURRENT_LIST_DIR}/src/minimal.cpp"\n'
        ")\n",
        encoding="utf-8",
    )
    _run([git_program, "init", "--quiet", str(repository)])
    _run([git_program, "-C", str(repository), "config", "user.name", "Semantic Test"])
    _run(
        [
            git_program,
            "-C",
            str(repository),
            "config",
            "user.email",
            "semantic-test@example.invalid",
        ]
    )
    _run([git_program, "-C", str(repository), "add", "libraries"])
    environment = dict(os.environ)
    environment.update(
        {
            "GIT_AUTHOR_DATE": "2001-01-01T00:00:00Z",
            "GIT_COMMITTER_DATE": "2001-01-01T00:00:00Z",
        }
    )
    _run(
        [git_program, "-C", str(repository), "commit", "--quiet", "-m", "fixture"],
        env=environment,
    )
    commit = _git_head(git_program, repository)
    adapter = root / "adapter.cpp"
    adapter_support = repository / "adapter_support.h"
    adapter_support.write_text(
        "#pragma once\n#define ADAPTER_SUPPORT_VALUE 42\n",
        encoding="utf-8",
    )
    adapter.write_text(
        '#include "drw_interface.h"\n'
        '#include "minimal.h"\n'
        '#include "adapter_support.h"\n'
        "#include <cstring>\n"
        "#include <iostream>\n"
        "#if !defined(LIBDXFRW_SEMANTIC_SIDE_TARGET) && "
        "!defined(LIBDXFRW_SEMANTIC_SIDE_STANDALONE)\n"
        '#error "semantic side definition is missing"\n'
        "#endif\n"
        "class SemanticSink final : public DRW_Interface {\n"
        "public:\n"
        "    void callback() override {}\n"
        "};\n"
        "int main(int argc, char** argv) {\n"
        "    if (argc == 2 && std::strcmp(argv[1], "
        '"--capability-profile") == 0) {\n'
        "#ifdef LIBDXFRW_SEMANTIC_SIDE_TARGET\n"
        '        std::cout << R"json({"schema":1,"name":'
        '"semantic-adapter-capabilities-v1","side":"target",'
        '"capabilities":{"dwgReadObservations":false,'
        '"modelerDwgSourceVersion":false,"modelerDwgAcisPayload":false,'
        '"modelerDxfPayloadChunks":false,"modelerPayloadStatusName":false,'
        '"payloadRangeBitOffset":false,"surfaceDxfPayloadChunks":false}})json";\n'
        "#else\n"
        '        std::cout << R"json({"schema":1,"name":'
        '"semantic-adapter-capabilities-v1","side":"standalone",'
        '"capabilities":{"dwgReadObservations":true,'
        '"modelerDwgSourceVersion":true,"modelerDwgAcisPayload":true,'
        '"modelerDxfPayloadChunks":true,"modelerPayloadStatusName":true,'
        '"payloadRangeBitOffset":true,"surfaceDxfPayloadChunks":true}})json";\n'
        "#endif\n"
        "        return 0;\n"
        "    }\n"
        "    return semantic_self_test_value() == ADAPTER_SUPPORT_VALUE ? 0 : 1;\n"
        "}\n",
        encoding="utf-8",
    )
    return repository / ".git", commit, source_list, adapter


def _self_test_lock(
    root: Path,
    git_program: str,
    git_dir: Path,
    commit: str,
) -> tuple[Path, Path]:
    listing = _run(
        [
            git_program,
            "--git-dir=" + str(git_dir),
            "ls-tree",
            "-r",
            "--format=%(path)|%(objectmode)|%(objectname)",
            commit,
            "--",
            *ARCHIVE_PATHS,
        ]
    )
    rows = []
    for line in listing.splitlines():
        name, mode, blob = line.split("|", 2)
        classification = (
            "manifest"
            if name == ARCHIVE_PATHS[1]
            else "source"
            if name.endswith(".cpp")
            else "header-unlisted"
        )
        rows.append(f"{name}|{mode}|{blob}|{classification}")
    manifest = root / "manifest.txt"
    manifest.write_text(
        "# path|mode|blob|classification\n" + "\n".join(rows) + "\n",
        encoding="utf-8",
    )
    archive = root / "identity.tar"
    archive_digest = _write_locked_archive(git_program, git_dir, commit, archive)
    archive.unlink()
    lock = {
        "schema": 1,
        "lockedAt": "2001-01-01",
        "standalone": {"commit": commit},
        "libreCAD": {
            "commit": commit,
            "snapshotRevision": commit,
            "sourceRoot": ARCHIVE_PATHS[0],
            "sourceList": ARCHIVE_PATHS[1],
        },
        "archive": {
            "format": "tar",
            "prefix": ARCHIVE_PREFIX,
            "sha256": archive_digest,
        },
        "manifest": {
            "path": manifest.name,
            "sha256": _sha256_file(manifest),
            "entries": len(rows),
        },
    }
    lock_path = root / "lock.json"
    lock_path.write_text(json.dumps(lock, indent=2) + "\n", encoding="utf-8")
    return lock_path, manifest


def _target_test_args(
    root: Path,
    git_program: str,
    cmake_program: str,
    git_dir: Path,
    lock: Path,
    manifest: Path,
    semantic_manifest: Path,
    adapter: Path,
    name: str,
) -> argparse.Namespace:
    return argparse.Namespace(
        build_dir=root / name,
        lock=lock,
        manifest=manifest,
        semantic_manifest=semantic_manifest,
        adapter_source=adapter,
        adapter_support_input=[git_dir.parent / "adapter_support.h"],
        wrapper_source=DEFAULT_WRAPPER,
        interface_profile="self-test-equal-v1",
        target_git_dir=git_dir,
        git=git_program,
        cmake=cmake_program,
        configuration="Release",
        parallel=2,
        receipt=None,
    )


def _self_test_target_cmake_launcher_guards(
    root: Path,
    git_program: str,
    cmake_program: str,
    git_dir: Path,
    lock: Path,
    manifest: Path,
    semantic_manifest: Path,
    adapter: Path,
) -> None:
    original = _require_file(
        DEFAULT_WRAPPER / "CMakeLists.txt", "semantic target wrapper"
    ).read_text(encoding="utf-8")
    project_line = "project(libdxfrw_semantic_target_adapter LANGUAGES CXX)\n"
    source_include = 'include("${LIBDXFRW_SEMANTIC_SOURCE_LIST}")\n'
    target_guard = (
        "# CMAKE_* variables normally initialize these target properties, but a\n"
    )
    mutations = (
        (
            "normal-variable compiler launcher",
            project_line,
            "set(CMAKE_CXX_COMPILER_LAUNCHER ccache)\n",
            "unsupported compiler argument/launcher channel: "
            "CMAKE_CXX_COMPILER_LAUNCHER",
        ),
        (
            "global rule launcher property",
            source_include,
            "set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)\n",
            "unsupported global launcher property: RULE_LAUNCH_COMPILE",
        ),
        (
            "post-include compiler launcher variable",
            source_include,
            "set(CMAKE_CXX_COMPILER_LAUNCHER ccache)\n",
            "unsupported compiler argument/launcher channel: "
            "CMAKE_CXX_COMPILER_LAUNCHER",
        ),
        (
            "target compiler launcher property",
            target_guard,
            "set_property(TARGET libdxfrw_semantic_target PROPERTY "
            "CXX_COMPILER_LAUNCHER ccache)\n",
            "unsupported target launcher property:",
        ),
    )
    for index, (label, marker, addition, expected) in enumerate(mutations):
        if marker not in original:
            raise BuildError(f"self-test cannot locate CMake guard for {label}")
        wrapper = root / f"wrapper-launch-negative-{index}"
        wrapper.mkdir()
        source = wrapper / "CMakeLists.txt"
        source.write_text(
            original.replace(marker, marker + addition, 1), encoding="utf-8"
        )
        args = _target_test_args(
            root,
            git_program,
            cmake_program,
            git_dir,
            lock,
            manifest,
            semantic_manifest,
            adapter,
            f"launcher-negative-{index}",
        )
        args.wrapper_source = wrapper
        try:
            build_target(args)
        except BuildError as exc:
            if expected not in str(exc) or (
                label == "target compiler launcher property"
                and "CXX_COMPILER_LAUNCHER" not in str(exc)
            ):
                raise BuildError(
                    f"self-test {label} failed for an unexpected reason: {exc}"
                ) from exc
        else:
            raise BuildError(f"self-test accepted {label}")


def _assert_equal(left: object, right: object, label: str) -> None:
    if left != right:
        raise BuildError(f"self-test {label} differs: {left!r} != {right!r}")


def _self_test_standalone_build(
    root: Path,
    repository: Path,
    adapter: Path,
    cmake_program: str,
) -> tuple[Path, Path, Path, Path]:
    source = root / "standalone-source"
    source.mkdir()
    (source / "src").mkdir()
    shutil.copyfile(
        repository / "libraries/libdxfrw/src/drw_interface.h",
        source / "src/drw_interface.h",
    )
    cmake_file = source / "CMakeLists.txt"
    minimal_source = repository / "libraries/libdxfrw/src/minimal.cpp"
    minimal_include = repository / "libraries/libdxfrw/src"
    cmake_file.write_text(
        "cmake_minimum_required(VERSION 3.28)\n"
        "project(semantic_standalone_self_test LANGUAGES CXX)\n"
        "set(CMAKE_CXX_STANDARD 17)\n"
        "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
        "set(CMAKE_CXX_EXTENSIONS OFF)\n"
        "set(CMAKE_RUNTIME_OUTPUT_DIRECTORY \"${CMAKE_BINARY_DIR}/bin\")\n"
        "set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY \"${CMAKE_BINARY_DIR}/lib\")\n"
        "foreach(config DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)\n"
        "  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config} \"${CMAKE_BINARY_DIR}/bin\")\n"
        "  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config} \"${CMAKE_BINARY_DIR}/lib\")\n"
        "endforeach()\n"
        f'add_library(dxfrw STATIC "{minimal_source.as_posix()}")\n'
        f'target_include_directories(dxfrw PUBLIC "{minimal_include.as_posix()}" '
        f'PRIVATE "{(minimal_include / "intern").as_posix()}")\n'
        f'add_executable({STANDALONE_EXECUTABLE} "{adapter.as_posix()}")\n'
        f'target_include_directories({STANDALONE_EXECUTABLE} PRIVATE '
        f'"{minimal_include.as_posix()}" '
        f'"{(minimal_include / "intern").as_posix()}")\n'
        f"target_compile_definitions({STANDALONE_EXECUTABLE} PRIVATE "
        "LIBDXFRW_SEMANTIC_SIDE_STANDALONE=1 "
        f'LIBDXFRW_SEMANTIC_ADAPTER_NAME=\\"{STANDALONE_EXECUTABLE}\\")\n'
        "if(MSVC)\n"
        f"  target_compile_options({STANDALONE_EXECUTABLE} PRIVATE /WX /bigobj)\n"
        "else()\n"
        f"  target_compile_options({STANDALONE_EXECUTABLE} PRIVATE -Werror)\n"
        "endif()\n"
        f"target_link_libraries({STANDALONE_EXECUTABLE} PRIVATE dxfrw)\n",
        encoding="utf-8",
    )
    build_tree = root / "standalone-build"
    _request_codemodel(build_tree)
    _run(
        [
            cmake_program,
            "-S",
            str(source),
            "-B",
            str(build_tree),
            "-DCMAKE_BUILD_TYPE=Release",
        ]
    )
    _run(
        [
            cmake_program,
            "--build",
            str(build_tree),
            "--target",
            STANDALONE_EXECUTABLE,
            "--config",
            "Release",
            "--parallel",
            "2",
        ]
    )
    executable = _find_one(
        [
            build_tree / "bin" / STANDALONE_EXECUTABLE,
            build_tree / "bin" / (STANDALONE_EXECUTABLE + ".exe"),
        ],
        "standalone self-test executable",
    )
    library = _find_one(
        [build_tree / "lib/libdxfrw.a", build_tree / "lib/dxfrw.lib"],
        "standalone self-test static library",
    )
    return cmake_file, build_tree, executable, library


def _self_test_adapter_compile_options() -> None:
    name = 'LIBDXFRW_SEMANTIC_ADAPTER_NAME="probe"'
    target = "LIBDXFRW_SEMANTIC_SIDE_TARGET=1"
    standalone = "LIBDXFRW_SEMANTIC_SIDE_STANDALONE=1"
    base = [{"define": target}, {"define": name}]

    def validate(flags: list[str], rows: list[object] | None = None,
                 *, side: str = "target", msvc: bool = False,
                 compiler_id: str | None = None,
                 allowed_include_paths: list[str] | None = None,
                 require_no_default_config: bool = False) -> set[str]:
        expected_side = "LIBDXFRW_SEMANTIC_SIDE_" + side.upper() + "=1"
        include_keys = {
            _compile_include_key(path, msvc_style=msvc)
            for path in (allowed_include_paths or [])
        }
        return _validate_adapter_compile_options(
            flags, base if rows is None else rows, side=side,
            required_defines={expected_side, name}, msvc_style=msvc,
            compiler_id=(compiler_id or ("MSVC" if msvc else "GNU")),
            allowed_include_paths=include_keys,
            require_no_default_config=require_no_default_config,
        )

    valid = [
        ("GNU ordinary", ["-O3 -DNDEBUG -Wall -Wextra -Werror -std=c++17"], False, []),
        ("GNU ordered", ["-Werror -O0 -Werror=unused-variable"], False, []),
        ("ordinary warning not MSVC WX", ["-Werror -Wextra -pedantic"], False, []),
        ("quoted path", ['-I "/tmp/SDK with spaces" -Werror'], False,
         ["/tmp/SDK with spaces"]),
        ("attached include path", ["-I/tmp/SDK -Werror"], False, ["/tmp/SDK"]),
        ("cross-fragment quoted path", ["-I", '"/tmp/SDK with spaces"', "-Werror"],
         False, ["/tmp/SDK with spaces"]),
        ("MSVC ordinary", ["/W3 /WX /bigobj /EHsc /MD /O2 /std:c++17"], True, []),
        ("MSVC ordered", ["/bigobj /O2 /WX /diagnostics:caret /DNDEBUG"], True, []),
        ("MSVC quoted path", ['/I "C:\\SDK with spaces" /WX /bigobj'], True,
         [r"C:\SDK with spaces"]),
    ]
    for label, flags, msvc, include_paths in valid:
        if validate(flags, msvc=msvc, allowed_include_paths=include_paths) != {
            target, name
        }:
            raise BuildError(f"self-test changed valid compile identity: {label}")
    for msvc, flags in ((False, ["-Werror"]), (True, ["/WX /bigobj"])):
        if validate(flags, [{"define": standalone}, {"define": name}],
                    side="standalone", msvc=msvc) != {standalone, name}:
            raise BuildError("self-test changed valid standalone compile identity")
    if validate(["/W3 /WX /bigobj --no-default-config"], msvc=True,
                compiler_id="Clang", require_no_default_config=True) != {target, name}:
        raise BuildError("self-test changed the clang-cl default-config policy")
    if validate(["-Werror", "--no-default-config"], compiler_id="Clang",
                require_no_default_config=True) != {target, name}:
        raise BuildError("self-test changed the Clang default-config policy")
    try:
        validate(["-Werror"], compiler_id="Clang", require_no_default_config=True)
    except BuildError:
        pass
    else:
        raise BuildError("self-test accepted Clang default config files")

    bad_definitions = [
        ("both side macros", base + [{"define": standalone}]),
        ("opposite side zero", base + [{"define": "LIBDXFRW_SEMANTIC_SIDE_STANDALONE=0"}]),
        ("wrong side only", [{"define": standalone}, {"define": name}]),
        ("side zero", [{"define": "LIBDXFRW_SEMANTIC_SIDE_TARGET=0"}, {"define": name}]),
        ("side absent", [{"define": name}]),
        ("duplicate side", base + [{"define": target}]),
        ("conflicting side", base + [{"define": "LIBDXFRW_SEMANTIC_SIDE_TARGET=0"}]),
        ("duplicate name", base + [{"define": name}]),
        ("conflicting name", base + [{"define": 'LIBDXFRW_SEMANTIC_ADAPTER_NAME="other"'}]),
        ("unknown side", base + [{"define": "LIBDXFRW_SEMANTIC_SIDE_OTHER=1"}]),
        ("unknown profile", base + [{"define": "LIBDXFRW_SEMANTIC_PROFILE_OTHER=1"}]),
        ("unknown identity key", base + [{"define": "LIBDXFRW_SEMANTIC_ADAPTER_OTHER=1"}]),
        ("probe zero", base + [{"define": "LIBDXFRW_SEMANTIC_3D_CONSUMER_PROBE=0"}]),
        ("malformed row", base + [None]),
        ("malformed value", base + [{"define": 1}]),
        ("missing value", base + [{}]),
        ("whitespace critical key", base + [{"define": " LIBDXFRW_SEMANTIC_SIDE_STANDALONE=1"}]),
    ]
    for label, rows in bad_definitions:
        try:
            validate(["-Werror"], rows)
        except BuildError:
            pass
        else:
            raise BuildError(f"self-test accepted invalid compile definitions: {label}")

    bad_flags = [
        ("global absent", ["-Wall"], False),
        ("specific only", ["-Werror=unused-variable"], False),
        ("embedded only", ["-DNOTE=-Werror"], False),
        ("split embedded only", ["-D", '"-Werror"'], False),
        ("path operand only", ["-I", '"-Werror"'], False),
        ("global demotion", ["-Werror -Wno-error"], False),
        ("reordered global demotion", ["-Wno-error -Werror"], False),
        ("specific demotion", ["-Werror -Wno-error=unused-variable"], False),
        ("reordered specific demotion", ["-Wno-error=unused-variable -Werror"], False),
        ("global suppression", ["-Werror -w"], False),
        ("long global suppression", ["-Werror --no-warnings"], False),
        ("Clang global suppression", ["-Werror -Wno-everything"], False),
        ("unknown error switch", ["-Werror -Werror-other"], False),
        ("MSVC negative only", ["/WX- /bigobj"], True),
        ("MSVC demotion", ["/WX /WX- /bigobj"], True),
        ("MSVC reordered demotion", ["/WX- /WX /bigobj"], True),
        ("MSVC dash demotion", ["/WX -WX- /bigobj"], True),
        ("MSVC suppression", ["/WX /w /bigobj"], True),
        ("MSVC level zero", ["/WX /W0 /bigobj"], True),
        ("MSVC wrong case", ["/wx /bigobj"], True),
        ("MSVC unknown case with real flag", ["/WX /wx /bigobj"], True),
        ("MSVC bigobj wrong case", ["/WX /BIGOBJ"], True),
        ("MSVC undocumented plus", ["/WX+ /bigobj"], True),
        ("MSVC ABI override", ["/WX /bigobj /GR-"], True),
        ("MSVC target override", ["/WX /bigobj /arch:AVX2"], True),
        ("MSVC profile override", ["/WX /bigobj /std:c++latest"], True),
        ("linker specific setting", ["/WX /WX:4001 /bigobj"], True),
        ("MSVC arbitrary macro", ["/DNOTE=/WX /WX /bigobj"], True),
        ("MSVC linker boundary", ["/link /WX /bigobj"], True),
        ("embedded macro cannot attest enforcement", ['-DNOTE="-Werror" -Werror'], False),
        ("arbitrary macro option", ["-DNOTE=ok -Werror"], False),
        ("long macro option", ["--define-macro=NOTE=ok -Werror"], False),
        ("split macro option", ["-D", "NOTE=ok -Werror"], False),
        ("macro undefinition", ["-UNDEBUG -Werror"], False),
        ("unreviewed target option", ["-Werror -march=native"], False),
        ("ABI option", ["-Werror -fno-exceptions"], False),
        ("standard override", ["-Werror -std=c++20"], False),
        ("unreviewed warning option", ["-Werror -Wno-unused-parameter"], False),
        ("toolchain prefix option", ["-Werror -B/tmp/toolchain"], False),
        ("sysroot option", ["-Werror --sysroot=/tmp/sysroot"], False),
        ("linker operand", ["-Xlinker -Werror"], False),
        ("long linker operand", ["--for-linker -Werror"], False),
        ("joined long linker operand", ["--for-linker=-Werror"], False),
        ("assembler operand", ["-Xassembler -Werror"], False),
        ("long assembler operand", ["--for-assembler -Werror"], False),
        ("joined assembler without global", ["--for-assembler=-Werror"], False),
        ("joined assembler with global", ["-Werror --for-assembler=-Wno-error"], False),
        ("critical joined define", ["-DLIBDXFRW_SEMANTIC_SIDE_TARGET=0 -Werror"], False),
        ("critical split define", ["-D", "LIBDXFRW_SEMANTIC_SIDE_TARGET=1 -Werror"], False),
        ("critical joined undefine", ["-ULIBDXFRW_SEMANTIC_SIDE_TARGET -Werror"], False),
        ("critical split undefine", ["-U LIBDXFRW_SEMANTIC_ADAPTER_NAME -Werror"], False),
        ("critical long define", ["--define-macro=LIBDXFRW_SEMANTIC_SIDE_TARGET=1 -Werror"], False),
        ("critical long undefine", ["--undefine-macro LIBDXFRW_SEMANTIC_SIDE_TARGET -Werror"], False),
        ("critical MSVC define", ["/DLIBDXFRW_SEMANTIC_SIDE_TARGET=0 /WX /bigobj"], True),
        ("critical MSVC undefine", ["/U", "LIBDXFRW_SEMANTIC_ADAPTER_NAME /WX /bigobj"], True),
        ("response file", ["@options.rsp -Werror"], False),
        ("response in path operand", ["-I @options.rsp -Werror"], False),
        ("preprocessor forwarding", ["-Wp,-DLIBDXFRW_SEMANTIC_SIDE_TARGET=0 -Werror"], False),
        ("preprocessor operand", ["-Xpreprocessor -Werror"], False),
        ("compiler forwarding", ["-Xclang -Werror"], False),
        ("joined compiler forwarding", ["-Werror -Xclang=-Wno-error"], False),
        ("compiler assembler forwarding", ["-Xclangas -Werror"], False),
        ("architecture-scoped forwarding", ["-Xarch_x86_64 -Werror"], False),
        ("offload-target forwarding", ["-Xopenmp-target=x86_64 -Werror"], False),
        ("unknown forwarding channel", ["-Werror -Xunreviewed=options"], False),
        ("clang-cl late forwarding", ["/WX /bigobj /clang:-Wno-error"], True),
        ("clang-cl dash late forwarding", ["/WX /bigobj -clang:-Wno-error"], True),
        ("forced include", ["-include macros.h -Werror"], False),
        ("long forced include", ["--include=macros.h -Werror"], False),
        ("forced macros", ["-imacros macros.h -Werror"], False),
        ("MSVC forced include", ["/FImacros.h /WX /bigobj"], True),
        ("opaque configuration", ["--config=options.cfg -Werror"], False),
        ("incomplete define", ["-Werror -D"], False),
        ("incomplete path", ["-Werror -I"], False),
        ("unreviewed system include option", ["-Werror -isystem /tmp/SDK"], False),
        ("unreviewed include path", ["-Werror -I/tmp/unreviewed"], False),
    ]
    for label, flags, msvc in bad_flags:
        try:
            validate(flags, msvc=msvc)
        except BuildError:
            pass
        else:
            raise BuildError(f"self-test accepted invalid compile flags: {label}")
    try:
        validate(["-Werror"], side="other")
    except BuildError:
        pass
    else:
        raise BuildError("self-test accepted an invalid compile side")

    # Exercise the Windows splitter on every host, not just MSVC-looking
    # flags passed through the current host's POSIX grammar. No native build
    # or Windows execution is implied by these dependency-free controls.
    from unittest.mock import patch
    with patch.object(os, "name", "nt"):
        for flags in (["/WX /bigobj"],
                      ['"/WX" "/bigobj"'],
                      ['/DNDEBUG /WX /bigobj']):
            validate(flags, msvc=True)
        validate(['/I "C:\\SDK with spaces" /WX /bigobj'], msvc=True,
                 allowed_include_paths=[r"C:\SDK with spaces"])
        for flags in (['/DNOTE="/WX /bigobj" /WX'],
                      [r'/DNOTE="a\" /WX /bigobj b\"" /WX'],
                      ['/I"C:\\SDK /WX /bigobj" /WX'],
                      ['/I "C:\\SDK /WX /bigobj" /WX'],
                      ['/DNOTE="/WX" /bigobj'],
                      ['/I @options.rsp /WX /bigobj']):
            try:
                validate(flags, msvc=True)
            except BuildError:
                pass
            else:
                raise BuildError("self-test accepted a Windows operand as compile enforcement")


def _self_test_controlled_driver_environment() -> None:
    from copy import deepcopy

    tool = _tool_binary_identity(
        sys.executable, "self-test Python executable", os.environ.get("PATH")
    )
    if (tool["path"] != Path(sys.executable).absolute().as_posix()
            or tool["digest"] != _sha256_file(Path(sys.executable))):
        raise BuildError("self-test failed to bind a configured tool binary")

    source = {"PATH": os.environ.get("PATH", "")}
    source["PRIVATE_SENTINEL"] = "must-not-appear-in-receipt"
    child, first = _controlled_driver_environment(None, source)
    same_child, same = _controlled_driver_environment(None, source)
    _assert_equal(first, same, "controlled driver identity stability")
    _assert_equal(child, same_child, "controlled child environment stability")
    if "PRIVATE_SENTINEL" in json.dumps(first) or (
        "must-not-appear-in-receipt" in json.dumps(first)
    ):
        raise BuildError("self-test serialized an unrelated environment value")
    if (not first["compilerPathDigest"] or not first["compilerBinaryDigest"]
            or not first.get("cmakePathDigest")
            or not first.get("cmakeBinaryDigest")):
        raise BuildError("self-test omitted compiler/CMake path or binary identity")

    changed = dict(source)
    changed["CXXFLAGS"] = "-DSEMANTIC_CONTEXT_SENTINEL=1"
    _, changed_identity = _controlled_driver_environment(None, changed)
    if changed_identity["environmentDigest"] == first["environmentDigest"]:
        raise BuildError("self-test failed to bind a changed compiler environment")

    for key in _REJECTED_DRIVER_ENV_KEYS:
        rejected = dict(source)
        rejected[key] = "private-override-sentinel"
        try:
            _controlled_driver_environment(None, rejected)
        except BuildError as exc:
            if key not in str(exc) or "private-override-sentinel" in str(exc):
                raise BuildError(
                    "self-test leaked or misidentified a rejected environment value"
                ) from exc
        else:
            raise BuildError(f"self-test accepted ambient override {key}")

    for key in ("VSCMD_VER", "VSCMD_ARG_TGT_ARCH", "VSCMD_UNREVIEWED_SENTINEL"):
        rejected = dict(source)
        rejected[key] = "private-override-sentinel"
        try:
            _controlled_driver_environment(None, rejected)
        except BuildError as exc:
            if key not in str(exc) or "private-override-sentinel" in str(exc):
                raise BuildError(
                    "self-test leaked or misidentified a rejected Visual Studio environment"
                ) from exc
        else:
            raise BuildError(f"self-test accepted Visual Studio environment {key}")

    invalid_parent_settings = deepcopy({
        "cacheVariables": {
            "CMAKE_CXX_COMPILER": "c++",
            "CMAKE_CXX_COMPILER_LAUNCHER": "ccache",
        }
    })
    try:
        _controlled_driver_environment(invalid_parent_settings, source)
    except BuildError as exc:
        if "CMAKE_CXX_COMPILER_LAUNCHER" not in str(exc):
            raise
    else:
        raise BuildError("self-test accepted a parent CMake launcher")

    for forbidden in (
        "CMAKE_CXX_COMPILER_ARG1",
        "CMAKE_CXX_COMPILER_LAUNCHER",
        "CMAKE_CXX_LINKER_LAUNCHER",
        "CMAKE_RULE_LAUNCH_COMPILE",
        "CMAKE_RULE_LAUNCH_LINK",
        "CMAKE_TOOLCHAIN_FILE",
        "CMAKE_CXX_COMPILER_EXTERNAL_TOOLCHAIN",
    ):
        settings = {
            "cacheVariables": {
                "CMAKE_CXX_COMPILER": "c++",
                forbidden: "private-channel-sentinel",
            }
        }
        try:
            _controlled_driver_environment(settings, source)
        except BuildError as exc:
            if forbidden not in str(exc) or "private-channel-sentinel" in str(exc):
                raise BuildError(
                    "self-test leaked or misidentified a parent CMake channel"
                ) from exc
        else:
            raise BuildError(f"self-test accepted parent CMake channel {forbidden}")

    invalid_compiler = dict(source)
    invalid_compiler["CXX"] = "c++ --config=private.cfg"
    try:
        _controlled_driver_environment(None, invalid_compiler)
    except BuildError:
        pass
    else:
        raise BuildError("self-test accepted compiler argument-1 channel")

    with tempfile.TemporaryDirectory(prefix="semantic-driver-wrapper-test-") as tmp:
        wrapper_dir = Path(tmp)
        wrapper = wrapper_dir / "ccache"
        wrapper.write_text("not executed by this self-test\n", encoding="utf-8")
        wrapper.chmod(0o700)
        wrapper_environment = dict(source)
        wrapper_environment["PATH"] = str(wrapper_dir)
        wrapper_environment["CXX"] = "ccache"
        try:
            _controlled_driver_environment(None, wrapper_environment)
        except BuildError as exc:
            if "wrapper" not in str(exc):
                raise
        else:
            raise BuildError("self-test accepted a compiler wrapper as the driver")


def _self_test_compiler_subprogram_identity() -> None:
    from unittest.mock import patch

    with tempfile.TemporaryDirectory(prefix="semantic-compiler-subprogram-") as tmp:
        root = Path(tmp)
        compiler = root / "g++"
        frontend = root / "cc1plus"
        compiler.write_bytes(b"mock compiler driver\n")
        frontend.write_bytes(b"mock configured frontend\n")
        compiler.chmod(0o700)
        frontend.chmod(0o700)
        with patch(
            __name__ + "._run", return_value=frontend.as_posix()
        ):
            identity = _compiler_subprogram_identity(
                compiler.as_posix(),
                "cc1plus",
                path_environment=None,
                env=None,
                driver_context=None,
            )
        _assert_equal(
            identity,
            {"path": frontend.as_posix(), "digest": _sha256_file(frontend)},
            "resolved compiler subprogram identity",
        )
        _verify_tool_binary_identities({"compilerFrontend": identity})
        original = frontend.read_bytes()
        frontend.write_bytes(original + b"changed")
        try:
            _verify_tool_binary_identities({"compilerFrontend": identity})
        except BuildError as exc:
            if "changed during the controlled build" not in str(exc):
                raise
        else:
            raise BuildError("self-test accepted a changed compiler subprogram")
        finally:
            frontend.write_bytes(original)
        with patch(__name__ + "._run", return_value="cc1plus"):
            unresolved = _compiler_subprogram_identity(
                compiler.as_posix(),
                "cc1plus",
                path_environment=None,
                env=None,
                driver_context=None,
            )
        _assert_equal(
            unresolved, {"path": "", "digest": ""},
            "unresolved compiler subprogram remains unbound",
        )
        with patch(
            __name__ + "._run", return_value=frontend.as_posix()
        ):
            specs = _compiler_reported_file_identity(
                compiler.as_posix(),
                "-print-file-name=specs",
                "self-test GCC specs file",
                path_environment=None,
                env=None,
                driver_context=None,
            )
        _assert_equal(
            specs,
            {"path": frontend.as_posix(), "digest": _sha256_file(frontend)},
            "compiler-reported configuration file identity",
        )


def _self_test_compiler_builtin_specs_profile() -> None:
    from unittest.mock import patch

    raw_specs = b"*cpp:\n%{posix:-D_POSIX_SOURCE}\r\n\x00"
    with tempfile.TemporaryDirectory(prefix="gcc-spec-closure-self-test-") as temp:
        external_specs = Path(temp) / "specs"
        external_specs.write_bytes(b"*link:\n%{shared:%:include(shared.spec)}\n")
        external_specs_identity = {
            "path": external_specs.as_posix(),
            "digest": _sha256_file(external_specs),
        }
        completed = subprocess.CompletedProcess(
            args=["g++", "-dumpspecs"], returncode=0,
            stdout=raw_specs, stderr=b"",
        )
        with patch(__name__ + ".subprocess.run", return_value=completed):
            profile = _compiler_builtin_specs_profile(
                "GNU", "/toolchain/bin/g++", controlled_fresh_build=True,
                env={}, driver_context=None,
                external_specs_identity=external_specs_identity,
            )
        _assert_equal(
            profile,
            {
                "schema": 2,
                "status": "captured",
                "method": "gcc-dumpspecs-stdout-v1",
                "compilerId": "GNU",
                "byteCount": len(raw_specs),
                "digest": hashlib.sha256(raw_specs).hexdigest(),
                "reason": "",
                "includeClosure": {
                    "schema": 1,
                    "status": "partial",
                    "builtinDirectiveCount": 0,
                    "externalDirectiveCount": 1,
                    "reason": "spec-includes-not-resolved-or-content-bound",
                },
            },
            "GCC external specs include remains partial",
        )
        cmake_data = {
            "compilerId": "GNU",
            "compilerPath": "/toolchain/bin/g++",
            "toolBinaries": {"compilerSpecs": external_specs_identity},
            "compilerBuiltinSpecs": profile,
        }
        with patch(__name__ + ".subprocess.run", return_value=completed):
            _verify_compiler_builtin_specs_profile(
                cmake_data, env={}, driver_context=None
            )
        external_specs.write_bytes(b"*link:\n%{shared:-shared}\n")
        try:
            with patch(__name__ + ".subprocess.run", return_value=completed):
                _verify_compiler_builtin_specs_profile(
                    cmake_data, env={}, driver_context=None
                )
        except BuildError as exc:
            if "changed during capture" not in str(exc):
                raise
        else:
            raise BuildError("self-test accepted changed external GCC specs")

        include_cases = (
            b"%include <plain.spec>\n",
            b"%include_noerr <optional.spec>\n",
            b"*link:\n%:include(nested.spec)\n",
            b"*link:\n%{shared:%:include(shared.spec)}\n",
            b"*link:\n%{m32:%:include(multilib.spec)}\n",
        )
        for sample in include_cases:
            if _gcc_specs_include_directive_count(sample) != 1:
                raise BuildError(
                    "GCC specs include detector missed a supported directive form"
                )
        no_include_specs = Path(temp) / "plain-specs"
        no_include_specs.write_bytes(b"*link:\n%{shared:-shared}\n")
        no_include_identity = {
            "path": no_include_specs.as_posix(),
            "digest": _sha256_file(no_include_specs),
        }
        builtin_with_include = b"*cpp:\n%include <site.spec>\n"
        with patch(
            __name__ + ".subprocess.run",
            return_value=subprocess.CompletedProcess(
                args=["g++", "-dumpspecs"], returncode=0,
                stdout=builtin_with_include, stderr=b"",
            ),
        ):
            builtin_include_profile = _compiler_builtin_specs_profile(
                "GNU", "/toolchain/bin/g++", controlled_fresh_build=True,
                env={}, driver_context=None,
                external_specs_identity=no_include_identity,
            )
        builtin_closure = builtin_include_profile["includeClosure"]
        if (not isinstance(builtin_closure, dict)
                or builtin_closure["status"] != "partial"
                or builtin_closure["builtinDirectiveCount"] != 1
                or builtin_closure["externalDirectiveCount"] != 0):
            raise BuildError("GCC built-in specs include was not fail-closed")
        if _gcc_specs_include_directive_count(b"*link:\n%%:include(literal)\n"):
            raise BuildError("GCC specs include detector misread escaped percent")

        observation = _gcc_specs_include_observation(
            raw_specs, no_include_identity
        )
        _assert_equal(
            (observation["status"], observation["builtinDirectiveCount"],
             observation["externalDirectiveCount"], observation["reason"]),
            ("no-includes-detected", 0, 0, ""),
            "GCC specs sources with no detected includes",
        )

    with patch(__name__ + ".subprocess.run", return_value=completed):
        profile_without_external = _compiler_builtin_specs_profile(
            "GNU", "/toolchain/bin/g++", controlled_fresh_build=True,
            env={}, driver_context=None,
        )
    include_closure = profile_without_external["includeClosure"]
    if (not isinstance(include_closure, dict)
            or include_closure["status"] != "unavailable"
            or include_closure["reason"]
            != "external-specs-file-identity-unavailable"):
        raise BuildError("unbound GCC external specs path was treated as closed")

    completed = subprocess.CompletedProcess(
        args=["g++", "-dumpspecs"], returncode=0,
        stdout=raw_specs, stderr=b"",
    )
    with patch(__name__ + ".subprocess.run", return_value=completed):
        profile = _compiler_builtin_specs_profile(
            "GNU", "/toolchain/bin/g++", controlled_fresh_build=True,
            env={}, driver_context=None, schema=1,
        )
    _assert_equal(
        profile,
        {
            "schema": 1,
            "status": "captured",
            "method": "gcc-dumpspecs-stdout-v1",
            "compilerId": "GNU",
            "byteCount": len(raw_specs),
            "digest": hashlib.sha256(raw_specs).hexdigest(),
            "reason": "",
        },
        "raw GCC built-in specs identity",
    )
    cmake_data = {
        "compilerId": "GNU",
        "compilerPath": "/toolchain/bin/g++",
        "compilerBuiltinSpecs": profile,
    }
    with patch(__name__ + ".subprocess.run", return_value=completed):
        _verify_compiler_builtin_specs_profile(
            cmake_data, env={}, driver_context=None
        )
    changed = subprocess.CompletedProcess(
        args=["g++", "-dumpspecs"], returncode=0,
        stdout=raw_specs + b"changed", stderr=b"",
    )
    with patch(__name__ + ".subprocess.run", return_value=changed):
        try:
            _verify_compiler_builtin_specs_profile(
                cmake_data, env={}, driver_context=None
            )
        except BuildError as exc:
            if "changed during the controlled build" not in str(exc):
                raise
        else:
            raise BuildError("self-test accepted changed GCC built-in specs")

    with patch(
        __name__ + ".subprocess.run",
        return_value=subprocess.CompletedProcess(
            args=["g++", "-dumpspecs"], returncode=1,
            stdout=b"", stderr=b"compiler query failed",
        ),
    ):
        unavailable = _compiler_builtin_specs_profile(
            "GNU", "/toolchain/bin/g++", controlled_fresh_build=True,
            env={}, driver_context=None,
        )
    _assert_equal(
        (unavailable["status"], unavailable["reason"], unavailable["digest"]),
        ("unavailable", "gcc-built-in-specs-query-failed", ""),
        "failed GCC built-in specs query",
    )
    with patch(__name__ + ".subprocess.run") as run:
        not_gnu = _compiler_builtin_specs_profile(
            "AppleClang", "/toolchain/bin/clang++",
            controlled_fresh_build=True, env={}, driver_context=None,
        )
    run.assert_not_called()
    _assert_equal(
        (not_gnu["status"], not_gnu["reason"]),
        ("unavailable", "compiler-family-not-qualified"),
        "non-GNU built-in specs profile",
    )
    with patch(__name__ + ".subprocess.run") as run:
        posthoc = _compiler_builtin_specs_profile(
            "GNU", "/toolchain/bin/g++", controlled_fresh_build=False,
            env={}, driver_context=None,
        )
    run.assert_not_called()
    _assert_equal(
        (posthoc["status"], posthoc["reason"]),
        ("unavailable", "existing-build-not-scanned-before-compilation"),
        "existing-build specs are not retroactively attested",
    )
    with patch(__name__ + ".MAX_GCC_BUILTIN_SPECS_BYTES", len(raw_specs) - 1):
        with patch(__name__ + ".subprocess.run", return_value=completed):
            over_limit = _compiler_builtin_specs_profile(
                "GNU", "/toolchain/bin/g++", controlled_fresh_build=True,
                env={}, driver_context=None,
            )
    _assert_equal(
        (over_limit["status"], over_limit["reason"], over_limit["digest"]),
        ("unavailable", "gcc-built-in-specs-size-limit", ""),
        "oversized GCC built-in specs profile",
    )


def _self_test_compiler_dependency_capture(root: Path, cmake_program: str) -> None:
    resource_root = root / "compiler-resource-self-test"
    (resource_root / "include").mkdir(parents=True)
    resource_file = resource_root / "include/resource.h"
    resource_file.write_text("#define RESOURCE_VALUE 1\n", encoding="utf-8")
    resource_before = _directory_tree_identity(resource_root)
    resource_file.write_text("#define RESOURCE_VALUE 2\n", encoding="utf-8")
    resource_after = _directory_tree_identity(resource_root)
    if (resource_before["treeDigest"] == resource_after["treeDigest"]
            or resource_before["fileCount"] != 1
            or resource_before["entryCount"] != 3):
        raise BuildError("self-test compiler resource tree identity is not content-bound")

    package = root / "dependency-package"
    source_dir = package / "src"
    external_include = root / "external-headers"
    source_dir.mkdir(parents=True)
    external_include.mkdir()
    external_header = external_include / "captured_external.h"
    external_header.write_text(
        "#pragma once\n#define SEMANTIC_EXTERNAL_VALUE 17\n",
        encoding="utf-8",
    )
    (source_dir / "minimal.h").write_text(
        "#pragma once\n#include <string>\nstd::string semantic_value();\n",
        encoding="utf-8",
    )
    (source_dir / "minimal.cpp").write_text(
        '#include "minimal.h"\n'
        '#include "captured_external.h"\n'
        "std::string semantic_value() { return std::to_string(SEMANTIC_EXTERNAL_VALUE); }\n",
        encoding="utf-8",
    )
    (source_dir / "adapter.cpp").write_text(
        '#include "minimal.h"\n'
        '#include "captured_external.h"\n'
        "int main() { return semantic_value().empty() || SEMANTIC_EXTERNAL_VALUE != 17; }\n",
        encoding="utf-8",
    )
    cmake_source = root / "dependency-CMakeLists.txt"
    cmake_source.write_text(
        "cmake_minimum_required(VERSION 3.28)\n"
        "project(semantic_dependency_self_test LANGUAGES CXX)\n"
        "set(CMAKE_CXX_STANDARD 17)\n"
        "set(CMAKE_CXX_STANDARD_REQUIRED ON)\n"
        "set(CMAKE_CXX_EXTENSIONS OFF)\n"
        f'add_library(dxfrw STATIC "{(source_dir / "minimal.cpp").as_posix()}")\n'
        f'add_executable({STANDALONE_EXECUTABLE} "{(source_dir / "adapter.cpp").as_posix()}")\n'
        f'target_include_directories(dxfrw PRIVATE "{external_include.as_posix()}")\n'
        f'target_include_directories({STANDALONE_EXECUTABLE} PRIVATE "{external_include.as_posix()}")\n'
        f'target_link_libraries({STANDALONE_EXECUTABLE} PRIVATE dxfrw)\n',
        encoding="utf-8",
    )
    # CMake requires the source file to be named CMakeLists.txt in the source
    # directory; the temporary project is isolated under the self-test root.
    project_root = root / "dependency-project"
    project_root.mkdir()
    shutil.move(str(cmake_source), str(project_root / "CMakeLists.txt"))
    build_tree = root / "dependency-build"
    _request_codemodel(build_tree)
    _run([
        cmake_program, "-S", str(project_root), "-B", str(build_tree),
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    ])
    cmake_data = _cmake_metadata(
        build_tree, cmake_program, "Release"
    )
    profile = _compiler_dependency_profile(
        build_tree=build_tree,
        configuration="Release",
        cmake_program=cmake_program,
        cmake_data=cmake_data,
        executable_target=STANDALONE_EXECUTABLE,
        library_target="dxfrw",
        env=None,
        driver_context=None,
    )
    if profile["status"] == "captured":
        inputs = profile["externalInputs"]
        if not isinstance(inputs, list) or not any(
            isinstance(row, dict)
            and row.get("path") == external_header.resolve().as_posix()
            for row in inputs
        ):
            raise BuildError("self-test omitted a known external compiler header")
    _run([
        cmake_program, "--build", str(build_tree),
        "--target", STANDALONE_EXECUTABLE, "--config", "Release",
        "--parallel", "2",
    ])
    if profile["status"] == "captured":
        _verify_compiler_dependency_profile(profile, build_tree=build_tree)
        original = external_header.read_bytes()
        external_header.write_bytes(original + b"\n// changed after compile\n")
        try:
            _verify_compiler_dependency_profile(profile, build_tree=build_tree)
        except BuildError as exc:
            if "changed during build" not in str(exc):
                raise
        else:
            raise BuildError("self-test accepted a changed external header")
        finally:
            external_header.write_bytes(original)

    depfile = root / "make-dependencies.d"
    depfile.write_text(
        "probe.o: /usr/include/example.h path\\ with\\ spaces.h \\\n"
        " nested\\#name.h\n",
        encoding="utf-8",
    )
    _assert_equal(
        _parse_make_dependency_file(depfile),
        ["/usr/include/example.h", "path with spaces.h", "nested#name.h"],
        "compiler dependency Makefile escaping",
    )


def _self_test_apple_sdk_capture(root: Path) -> None:
    sdk_root = root / "synthetic-apple-sdk"
    sdk_root.mkdir()
    settings_json = sdk_root / "SDKSettings.json"
    settings_plist = sdk_root / "SDKSettings.plist"
    settings_json.write_text('{"Version":"test-1"}\n', encoding="utf-8")
    settings_plist.write_bytes(b"synthetic SDK settings\n")
    cmake_data: dict[str, object] = {
        "systemName": "Darwin",
        "compilerId": "AppleClang",
        "sysroot": "",
        "sysrootCompile": "",
        "sysrootLink": "",
        "osxSysroot": "macosx",
        "compilerAppleSysroot": sdk_root.as_posix(),
    }
    profile = _apple_sdk_profile(cmake_data)
    if profile.get("status") != "captured":
        raise BuildError("self-test failed to capture the synthetic Apple SDK")
    _verify_apple_sdk_profile(profile)
    original = settings_json.read_bytes()
    settings_json.write_bytes(original + b" ")
    try:
        _verify_apple_sdk_profile(profile)
    except BuildError as exc:
        if "changed during build" not in str(exc):
            raise
    else:
        raise BuildError("self-test accepted changed Apple SDK metadata")
    finally:
        settings_json.write_bytes(original)
    _verify_apple_sdk_profile(profile)
    unavailable = _apple_sdk_profile({
        "systemName": "Linux", "compilerId": "GNU",
    })
    if (unavailable.get("status") != "unavailable"
            or unavailable.get("reason") != "platform-not-qualified"):
        raise BuildError("self-test omitted explicit non-Apple SDK disposition")


def _self_test_apple_link_map(root: Path) -> None:
    build_tree = root / "apple-link-map"
    sdk_root = root / "apple-link-map-sdk"
    outside_root = root / "apple-link-map-unclassified"
    (build_tree / "CMakeFiles/adapter.dir").mkdir(parents=True)
    (build_tree / "lib").mkdir()
    (sdk_root / "usr/lib").mkdir(parents=True)
    outside_root.mkdir()
    executable = build_tree / "bin/adapter"
    executable.parent.mkdir()
    executable.write_bytes(b"synthetic executable\n")
    (build_tree / "bin/different-adapter").write_bytes(
        b"different synthetic executable\n"
    )
    adapter_object = build_tree / "CMakeFiles/adapter.dir/adapter.cpp.o"
    adapter_object.write_bytes(b"synthetic adapter object\n")
    library = build_tree / "lib/libdxfrw.a"
    library.write_bytes(b"synthetic static archive\n")
    sdk_stub = sdk_root / "usr/lib/libSystem.tbd"
    sdk_stub.write_text("--- !tapi-tbd\ninstall-name: /usr/lib/libSystem.B.dylib\n",
                        encoding="utf-8")
    other_stub = outside_root / "liboutside.tbd"
    other_stub.write_text("unclassified external linker input\n", encoding="utf-8")
    sdk_profile: dict[str, object] = {
        "schema": 1,
        "status": "captured",
        "method": "apple-sdk-settings-v1",
        "systemName": "Darwin",
        "compilerId": "AppleClang",
        "roots": [{"resolvedPath": sdk_root.resolve().as_posix()}],
        "rootsDigest": _canonical_digest([sdk_root.resolve().as_posix()]),
        "reason": "",
    }
    map_path = build_tree / "semantic-adapter-link.map"

    def write_map(
        *,
        unknown: bool = False,
        duplicate: str = "",
        output_path: str | None = None,
    ) -> None:
        rows = [
            "[  0] linker synthesized",
            "[  1] CMakeFiles/adapter.dir/adapter.cpp.o",
            "[  2] lib/libdxfrw.a(drw_entities.cpp.o)",
            f"[  3] {sdk_stub.resolve().as_posix()}",
        ]
        if duplicate == "build-object":
            rows.append("[  4] CMakeFiles/adapter.dir/adapter.cpp.o")
        elif duplicate == "archive-member":
            rows.append("[  4] lib/libdxfrw.a(drw_entities.cpp.o)")
        elif duplicate == "sdk-input":
            rows.append(f"[  4] {sdk_stub.resolve().as_posix()}")
        if unknown:
            rows.append(
                f"[{len(rows):3d}] {other_stub.resolve().as_posix()}"
            )
        map_path.write_text(
            "# Path: " + (output_path or "bin/adapter") + "\n"
            "# Arch: arm64\n# Object files:\n"
            + "\n".join(rows) + "\n# Sections:\n# synthetic\n",
            encoding="utf-8",
        )

    write_map()
    profile = _apple_link_inputs_profile(
        map_path=map_path,
        build_tree=build_tree,
        executable_path=executable,
        library_path=library,
        sdk_profile=sdk_profile,
        system_name="Darwin",
        compiler_id="AppleClang",
    )
    if (profile.get("status") != "captured"
            or profile.get("archiveMembers") != ["drw_entities.cpp.o"]
            or len(profile.get("externalInputs", [])) != 1):
        raise BuildError("self-test failed to classify synthetic Apple link inputs")
    stub_digest = _sha256_file(sdk_stub)
    stub_size = sdk_stub.stat().st_size
    runtime_node = {
        "installName": "/usr/lib/libSystem.B.dylib",
        "directLinkInput": True,
        "images": [{
            "architecture": "arm64",
            "uuid": "A21743EE-6E1C-3E74-97EA-7AF793DE4BDF",
        }],
        "sdkStubs": [{
            "sdkRoot": sdk_root.resolve().as_posix(),
            "path": sdk_stub.resolve().as_posix(),
            "resolvedPath": sdk_stub.resolve().as_posix(),
            "size": stub_size,
            "digest": stub_digest,
            "installName": "/usr/lib/libSystem.B.dylib",
            "currentVersion": "",
            "compatibilityVersion": "",
        }],
    }
    runtime_profile: dict[str, object] = {
        "schema": 1,
        "status": "partial",
        "method": "macho-dyld-uuid-closure-v1",
        "systemName": "Darwin",
        "compilerId": "AppleClang",
        "executableDigest": _sha256_file(executable),
        "tool": {"path": "/usr/bin/dyld_info", "digest": "0" * 64},
        "nodes": [runtime_node],
        "edges": [{
            "source": "adapter-executable",
            "target": "/usr/lib/libSystem.B.dylib",
            "architecture": "arm64",
            "attribute": "load",
        }],
        "nodesDigest": "",
        "edgesDigest": "",
        "unresolvedImageCount": 0,
        "unmatchedSdkStubCount": 0,
        "maximumDepth": 1,
        "reason": "link-stub-selection-not-proven",
    }
    selected_runtime = _bind_apple_runtime_selected_stubs(runtime_profile, profile)
    if (selected_runtime.get("schema") != 2
            or selected_runtime.get("linkerSelectedStubs") != [{
                "installName": "/usr/lib/libSystem.B.dylib",
                "sdkRoot": sdk_root.resolve().as_posix(),
                "path": "usr/lib/libSystem.tbd",
                "size": stub_size,
                "digest": stub_digest,
            }]
            or selected_runtime.get("unselectedDirectInstallNameCount") != 0
            or selected_runtime.get("reason") != "runtime-byte-identity-not-proven"):
        raise BuildError("self-test failed to bind selected SDK stub to runtime candidate")
    unmatched_runtime = {
        **runtime_profile,
        "nodes": [{**runtime_node, "sdkStubs": []}],
        "unmatchedSdkStubCount": 1,
    }
    unmatched = _bind_apple_runtime_selected_stubs(unmatched_runtime, profile)
    if (unmatched.get("linkerSelectedStubs") != []
            or unmatched.get("unselectedDirectInstallNameCount") != 1
            or unmatched.get("reason") != "link-stub-selection-not-proven"):
        raise BuildError("self-test failed to retain unmatched runtime stub disposition")
    _verify_apple_link_inputs_profile(
        profile,
        map_path=map_path,
        build_tree=build_tree,
        executable_path=executable,
        library_path=library,
        sdk_profile=sdk_profile,
        system_name="Darwin",
        compiler_id="AppleClang",
    )

    write_map(output_path="bin/different-adapter")
    try:
        _apple_link_inputs_profile(
            map_path=map_path,
            build_tree=build_tree,
            executable_path=executable,
            library_path=library,
            sdk_profile=sdk_profile,
            system_name="Darwin",
            compiler_id="AppleClang",
        )
    except BuildError:
        pass
    else:
        raise BuildError("self-test accepted a map for a different executable")

    for duplicate_kind in ("build-object", "archive-member", "sdk-input"):
        write_map(duplicate=duplicate_kind)
        try:
            _apple_link_inputs_profile(
                map_path=map_path,
                build_tree=build_tree,
                executable_path=executable,
                library_path=library,
                sdk_profile=sdk_profile,
                system_name="Darwin",
                compiler_id="AppleClang",
            )
        except BuildError:
            pass
        else:
            raise BuildError(
                f"self-test accepted duplicate {duplicate_kind} map input"
            )

    write_map(unknown=True)
    partial = _apple_link_inputs_profile(
        map_path=map_path,
        build_tree=build_tree,
        executable_path=executable,
        library_path=library,
        sdk_profile=sdk_profile,
        system_name="Darwin",
        compiler_id="AppleClang",
    )
    if (partial.get("status") != "partial"
            or partial.get("reason") != "unclassified-link-inputs"
            or partial.get("unclassifiedCount") != 1
            or str(other_stub.resolve()) in json.dumps(partial)):
        raise BuildError("self-test did not fail closed on an external link input")

    malformed = map_path.read_bytes().replace(b"[  2]", b"[  9]", 1)
    map_path.write_bytes(malformed)
    try:
        _parse_apple_link_map(malformed)
    except BuildError:
        pass
    else:
        raise BuildError("self-test accepted a non-contiguous Apple map index")

    unavailable = _apple_link_inputs_profile(
        map_path=map_path,
        build_tree=build_tree,
        executable_path=executable,
        library_path=library,
        sdk_profile=sdk_profile,
        system_name="Linux",
        compiler_id="GNU",
    )
    if (unavailable.get("status") != "unavailable"
            or unavailable.get("reason") != "platform-not-qualified"):
        raise BuildError("self-test omitted unavailable non-Apple link input status")


def _self_test_dyld_batch_retention() -> None:
    paths = [f"/synthetic/dylib-{index:03d}" for index in range(130)]
    visited = {paths[17]}
    expected = sorted(set(paths) - visited)
    queue = [(path, 2) for path in reversed(paths)] + [(paths[4], 2)]
    processed: list[str] = []
    while queue:
        depth, batch, queue = _take_dyld_image_batch(
            queue, visited, batch_size=64
        )
        if depth != 2:
            raise BuildError("self-test dyld batch changed the dependency depth")
        processed.extend(batch)
        visited.update(batch)
    if processed != expected:
        raise BuildError("self-test dyld batches dropped or duplicated image paths")


def self_test(git_program: str, cmake_program: str) -> None:
    _self_test_adapter_compile_options()
    _self_test_controlled_driver_environment()
    _self_test_compiler_subprogram_identity()
    _self_test_compiler_builtin_specs_profile()
    _self_test_dyld_batch_retention()
    # Exercise the actual source, not only the synthetic one-callback fixture.
    # Preserve the frozen common170 adapter surface.  Current standalone
    # interfaces have accumulated additive callbacks independently of the
    # locked target interface; keep their exact names visible rather than
    # silently widening the common inventory.
    real_methods = _adapter_override_methods(DEFAULT_ADAPTER, side="target")
    common_identity = {"schema": 1, "methods": real_methods}
    if len(real_methods) != 170 or _canonical_digest(common_identity) != (
        "894c58db480ef3f5c7e25a9066ca9a178ecf5e41137a9e709a1144f340faba7a"
    ):
        raise BuildError("real semantic adapter common170 inventory changed")
    _self_test_named_interface_profile()
    _self_test_capability_profile()

    with tempfile.TemporaryDirectory(prefix="semantic-adapter-self-test-") as directory:
        root = Path(directory)
        _self_test_apple_sdk_capture(root)
        _self_test_apple_link_map(root)
        _self_test_compiler_dependency_capture(root, cmake_program)
        git_dir, commit, source_list, adapter = _self_test_repository(root, git_program)
        _assert_equal(
            _standalone_source_lists(source_list)["LIBDXFRW_SOURCES"],
            ["minimal.cpp"],
            "closed standalone source-list parser",
        )
        unsafe_source_list = root / "unsafe-source-list.cmake"
        unsafe_source_list.write_text(
            source_list.read_text(encoding="utf-8")
            + "\nmessage(FATAL_ERROR \\\"must not execute\\\")\n",
            encoding="utf-8",
        )
        try:
            _standalone_source_lists(unsafe_source_list)
        except BuildError:
            pass
        else:
            raise BuildError("self-test accepted executable source-list CMake")
        lock, manifest = _self_test_lock(root, git_program, git_dir, commit)
        semantic_manifest = root / "semantic-manifest.json"
        semantic_manifest.write_text(
            '{"kind":"self-test-semantic-manifest","schema":2}\n',
            encoding="utf-8",
        )
        duplicate_json = root / "duplicate.json"
        duplicate_json.write_text('{"schema":1,"schema":2}\n', encoding="utf-8")
        try:
            _read_json(duplicate_json, "duplicate self-test JSON")
        except BuildError as exc:
            if "duplicate JSON key" not in str(exc):
                raise
        else:
            raise BuildError("self-test accepted a duplicate JSON key")
        nonfinite_json = root / "nonfinite.json"
        nonfinite_json.write_text('{"value":NaN}\n', encoding="utf-8")
        try:
            _read_json(nonfinite_json, "non-finite self-test JSON")
        except BuildError as exc:
            if "non-finite JSON token" not in str(exc):
                raise
        else:
            raise BuildError("self-test accepted a non-finite JSON token")

        blank_configuration = root / "blank-configuration"
        blank_configuration.mkdir()
        (blank_configuration / "CMakeCache.txt").write_text(
            "CMAKE_BUILD_TYPE:STRING=\n", encoding="utf-8"
        )
        try:
            _verify_requested_configuration(blank_configuration, "Release")
        except BuildError as exc:
            if "CMAKE_BUILD_TYPE ''" not in str(exc):
                raise
        else:
            raise BuildError(
                "self-test accepted an unconfigured single-config build as Release"
            )

        mismatched_configuration = root / "mismatched-configuration"
        mismatched_configuration.mkdir()
        (mismatched_configuration / "CMakeCache.txt").write_text(
            "CMAKE_BUILD_TYPE:STRING=Debug\n", encoding="utf-8"
        )
        try:
            _verify_requested_configuration(mismatched_configuration, "Release")
        except BuildError as exc:
            if "CMAKE_BUILD_TYPE 'Debug'" not in str(exc):
                raise
        else:
            raise BuildError(
                "self-test accepted a Debug single-config build as Release"
            )

        missing_override = root / "missing-override.cpp"
        missing_override.write_text(
            '#include "drw_interface.h"\n'
            "class SemanticSink final : public DRW_Interface {};\n"
            "int main() { return 0; }\n",
            encoding="utf-8",
        )
        try:
            _verify_interface_contract(
                git_dir.parent / "libraries/libdxfrw/src/drw_interface.h",
                missing_override,
            )
        except BuildError as exc:
            if "complete DRW_Interface callback surface" not in str(exc):
                raise
        else:
            raise BuildError(
                "self-test accepted an incomplete DRW_Interface callback inventory"
            )

        commented_override = root / "commented-override.cpp"
        commented_override.write_text(
            '#include "drw_interface.h"\n'
            "class SemanticSink final : public DRW_Interface {\n"
            "public:\n"
            "    /* void callback() override {} */\n"
            "};\n",
            encoding="utf-8",
        )
        try:
            _verify_interface_contract(
                git_dir.parent / "libraries/libdxfrw/src/drw_interface.h",
                commented_override,
            )
        except BuildError as exc:
            if "complete DRW_Interface callback surface" not in str(exc):
                raise
        else:
            raise BuildError(
                "self-test counted a block-commented callback override"
            )

        inactive_override = root / "inactive-override.cpp"
        inactive_override.write_text(
            '#include "drw_interface.h"\n'
            "class SemanticSink final : public DRW_Interface {\n"
            "public:\n"
            "#if 0\n"
            "    void callback() override {}\n"
            "#endif\n"
            "};\n",
            encoding="utf-8",
        )
        try:
            _verify_interface_contract(
                git_dir.parent / "libraries/libdxfrw/src/drw_interface.h",
                inactive_override,
            )
        except BuildError as exc:
            if "unsupported preprocessor directive" not in str(exc):
                raise
        else:
            raise BuildError(
                "self-test counted a conditionally inactive callback override"
            )

        traversal = root / "traversal.tar"
        with tarfile.open(traversal, mode="w") as archive:
            info = tarfile.TarInfo("../escape")
            info.size = 0
            archive.addfile(info)
        try:
            _extract_and_verify_archive(traversal, root / "unsafe", {})
        except BuildError:
            pass
        else:
            raise BuildError("self-test accepted an archive traversal")

        first, first_path = build_target(
            _target_test_args(
                root,
                git_program,
                cmake_program,
                git_dir,
                lock,
                manifest,
                semantic_manifest,
                adapter,
                "first",
            )
        )
        target_support = git_dir.parent / "adapter_support.h"
        expected_target_support = [{
            "path": _logical_input_path(target_support),
            "snapshotPath": target_support.name,
            "digest": _sha256_file(target_support),
        }]
        target_inputs = first["build"]["configIdentity"]["inputs"]
        _assert_equal(
            target_inputs.get("capabilityProfile"),
            _expected_capability_profile("target"),
            "target binary capability profile",
        )
        _assert_equal(
            target_inputs.get("adapterSupportInputs"),
            expected_target_support,
            "target adapter support-input identity",
        )
        second, _ = build_target(
            _target_test_args(
                root,
                git_program,
                cmake_program,
                git_dir,
                lock,
                manifest,
                semantic_manifest,
                adapter,
                "first",
            )
        )
        for key in (
            "sourceDigest",
            "configDigest",
        ):
            _assert_equal(first["adapter"][key], second["adapter"][key], key)
        # Fresh builds may contain toolchain/archive timestamp variation; the
        # deterministic comparison is the build contract, not binary bytes.
        _assert_equal(
            first["build"]["buildConfigDigest"],
            second["build"]["buildConfigDigest"],
            "fresh target build config identity",
        )
        first_parity = dict(first["build"]["buildParityIdentity"])
        second_parity = dict(second["build"]["buildParityIdentity"])
        first_link_inputs = first_parity.pop("appleLinkInputs", None)
        second_link_inputs = second_parity.pop("appleLinkInputs", None)
        _assert_equal(first_parity, second_parity,
                      "fresh target build parity core")
        if not isinstance(first_link_inputs, dict) or not isinstance(
                second_link_inputs, dict):
            raise BuildError("fresh target build omitted Apple link-map profile")
        stable_link_keys = (
            "status", "method", "systemName", "compilerId", "architecture",
            "objectCount", "synthesizedObjectCount", "archiveMembers",
            "externalInputs", "unclassifiedCount", "unclassifiedDigest",
            "reason",
        )
        for key in stable_link_keys:
            _assert_equal(
                first_link_inputs[key], second_link_inputs[key],
                "fresh target Apple link-map " + key,
            )
        first_driver_context = first["build"]["configIdentity"]["inputs"].get(
            "driverContext"
        )
        second_driver_context = second["build"]["configIdentity"]["inputs"].get(
            "driverContext"
        )
        if not isinstance(first_driver_context, dict):
            raise BuildError("target self-test omitted controlled driver context")
        _assert_equal(
            first_driver_context, second_driver_context,
            "controlled driver context",
        )
        _assert_equal(
            first["build"]["configIdentity"]["inputs"]["compileContractMode"],
            "direct-options-only",
            "target effective compile-contract holdback",
        )
        if "PRIVATE_SENTINEL" in json.dumps(first_driver_context):
            raise BuildError("target self-test receipt leaked ambient environment")
        executable = first_path.parent / first["artifacts"]["executable"]["path"]
        _run([str(executable.resolve())])

        standalone_support = root / "standalone-adapter-support.h"
        standalone_support.write_text(
            "// content-bound standalone adapter include input\n",
            encoding="utf-8",
        )
        standalone_args = argparse.Namespace(
            source_root=git_dir.parent / "libraries/libdxfrw",
            source_list=source_list,
            parent_cmake_build_dir=None,
            build_dir=root / "standalone-controlled-build",
            adapter_source=adapter,
            adapter_support_input=[standalone_support, target_support],
            semantic_manifest=semantic_manifest,
            commit=commit,
            git=git_program,
            cmake=cmake_program,
            configuration="Release",
            parallel=2,
            receipt=None,
            interface_profile="self-test-equal-v1",
        )
        standalone, standalone_receipt_path = build_standalone(standalone_args)
        # Exercise the actual standalone binary as well as the locked target.
        # The adapter self-test owns generated clean/partial/opaque read and
        # DWG readback sentinels; compiling the standalone side alone would
        # leave its reader path unexecuted and could hide a side-specific
        # capability or retention regression.
        standalone_executable = (
            standalone_receipt_path.parent
            / standalone["artifacts"]["executable"]["path"]
        )
        _run([str(standalone_executable.resolve())])
        _assert_equal(standalone["adapter"]["side"], "standalone", "standalone side")
        _assert_equal(standalone["targetLock"], None, "standalone target lock")
        standalone_inputs = standalone["build"]["configIdentity"]["inputs"]
        _assert_equal(
            standalone_inputs.get("capabilityProfile"),
            _expected_capability_profile("standalone"),
            "standalone binary capability profile",
        )
        standalone_driver_context = standalone_inputs.get("driverContext")
        if not isinstance(standalone_driver_context, dict):
            raise BuildError("controlled standalone self-test omitted driver context")
        _assert_equal(first_driver_context, standalone_driver_context,
                      "target/standalone controlled driver context")
        if not any(
            row.get("path") == "controlled-standalone-source-tree-v1"
            for row in standalone_inputs.get("configFiles", [])
            if isinstance(row, dict)
        ):
            raise BuildError("controlled standalone self-test omitted source snapshot identity")
        expected_support_identity = [
            {
                "path": standalone_support.name,
                "snapshotPath": "tests/" + standalone_support.name,
                "digest": _sha256_file(standalone_support),
            },
            {
                "path": _logical_input_path(target_support),
                "snapshotPath": "tests/" + target_support.name,
                "digest": _sha256_file(target_support),
            },
        ]
        _assert_equal(
            standalone_inputs.get("adapterSupportInputs"),
            expected_support_identity,
            "standalone adapter support-input identity",
        )
        standalone_cache = _parse_cmake_cache(standalone_receipt_path.parent)
        snapshot_home = Path(
            standalone_cache.get("CMAKE_HOME_DIRECTORY", "")
        ).resolve()
        if (snapshot_home.parent.name != "standalone"
                or not HEX64.fullmatch(snapshot_home.name)):
            raise BuildError(
                "controlled standalone self-test did not build its content snapshot"
            )
        repeat_args = argparse.Namespace(**vars(standalone_args))
        repeated, repeated_receipt_path = build_standalone(repeat_args)
        if repeated_receipt_path.parent == standalone_receipt_path.parent:
            raise BuildError("controlled standalone self-test reused a build cache")
        repeated_inputs = repeated["build"]["configIdentity"]["inputs"]
        _assert_equal(
            standalone_inputs["driverContext"], repeated_inputs["driverContext"],
            "fresh standalone compiler context",
        )
        _assert_equal(
            standalone_inputs["configFiles"], repeated_inputs["configFiles"],
            "fresh standalone source/build-key inputs",
        )
        _assert_equal(
            standalone_inputs["adapterSupportInputs"],
            repeated_inputs["adapterSupportInputs"],
            "fresh standalone adapter support inputs",
        )
        for receipt, label in ((first, "target"), (standalone, "standalone")):
            cmake_identity = receipt["build"]["buildParityIdentity"]["cmake"]
            builtin_specs = cmake_identity.get("compilerBuiltinSpecs")
            if not isinstance(builtin_specs, dict):
                raise BuildError(
                    f"self-test {label} omitted compiler built-in specs profile"
                )
            if (cmake_identity["compilerId"] != "GNU"
                    and (builtin_specs.get("status") != "unavailable"
                         or builtin_specs.get("reason")
                         != "compiler-family-not-qualified")):
                raise BuildError(
                    f"self-test {label} misreported the unsupported built-in specs route"
                )
            dependency_identity = receipt["build"]["buildParityIdentity"].get(
                "compilerDependencies"
            )
            resource_identity = receipt["build"]["buildParityIdentity"].get(
                "compilerResources"
            )
            apple_sdk_identity = receipt["build"]["buildParityIdentity"].get(
                "appleSdk"
            )
            apple_runtime_identity = receipt["build"]["buildParityIdentity"].get(
                "appleRuntime"
            )
            apple_link_inputs_identity = receipt["build"][
                "buildParityIdentity"
            ].get("appleLinkInputs")
            apple_link_inputs_identity = receipt["build"][
                "buildParityIdentity"
            ].get("appleLinkInputs")
            if not isinstance(dependency_identity, dict):
                raise BuildError(
                    f"self-test {label} omitted compiler dependency identity"
                )
            if not isinstance(resource_identity, dict):
                raise BuildError(
                    f"self-test {label} omitted compiler resource identity"
                )
            if not isinstance(apple_sdk_identity, dict):
                raise BuildError(f"self-test {label} omitted Apple SDK identity")
            if not isinstance(apple_runtime_identity, dict):
                raise BuildError(f"self-test {label} omitted Apple runtime identity")
            if not isinstance(apple_link_inputs_identity, dict):
                raise BuildError(
                    f"self-test {label} omitted Apple linker-input identity"
                )
            if not isinstance(apple_link_inputs_identity, dict):
                raise BuildError(
                    f"self-test {label} omitted Apple linker-input identity"
                )
            if (cmake_identity["compilerId"] in {"GNU", "Clang", "AppleClang"}
                    and not cmake_identity["multiConfig"]
                    and os.name != "nt"
                    and dependency_identity.get("status") != "captured"):
                raise BuildError(
                    f"self-test {label} failed to capture supported compiler dependencies"
                )
            if (cmake_identity["compilerId"] in {"Clang", "AppleClang"}
                    and os.name != "nt"
                    and resource_identity.get("status") != "captured"):
                raise BuildError(
                    f"self-test {label} failed to capture the Clang resource tree"
                )
            if (cmake_identity["systemName"] == "Darwin"
                    and cmake_identity["compilerId"] in {"Clang", "AppleClang"}
                    and apple_sdk_identity.get("status") != "captured"):
                raise BuildError(
                    f"self-test {label} failed to capture Apple SDK manifests"
                )
            if (cmake_identity["systemName"] == "Darwin"
                    and cmake_identity["compilerId"] in {"Clang", "AppleClang"}
                    and apple_runtime_identity.get("status") != "partial"):
                raise BuildError(
                    f"self-test {label} failed to capture partial dyld runtime evidence"
                )
            if (cmake_identity["systemName"] == "Darwin"
                    and cmake_identity["compilerId"] in {"Clang", "AppleClang"}
                    and apple_link_inputs_identity.get("status") != "captured"):
                raise BuildError(
                    f"self-test {label} failed to capture selected Apple link inputs"
                )
            if (cmake_identity["systemName"] == "Darwin"
                    and cmake_identity["compilerId"] in {"Clang", "AppleClang"}
                    and apple_link_inputs_identity.get("status") != "captured"):
                raise BuildError(
                    f"self-test {label} failed to capture selected Apple link inputs"
                )
            tool_binaries = cmake_identity.get("toolBinaries")
            if not isinstance(tool_binaries, dict):
                raise BuildError(f"self-test {label} omitted tool binary identity")
            for role in ("cmake", "buildProgram", "compiler"):
                identity = tool_binaries.get(role)
                if (not isinstance(identity, dict)
                        or not identity.get("path")
                        or not HEX64.fullmatch(str(identity.get("digest", "")))):
                    raise BuildError(
                        f"self-test {label} omitted required {role} binary digest"
                    )
            inputs = receipt["build"]["configIdentity"]["inputs"]
            _assert_equal(inputs["compileContractMode"], "direct-options-only",
                          label + " compile-contract disposition")
            _assert_equal(inputs["compileOptionAdmissionEnforced"], True,
                          label + " direct-option admission")
            _assert_equal(inputs["compileContractEnforced"], False,
                          label + " effective compile-contract status")
            profile = inputs.get("compileContractProfile")
            if (not isinstance(profile, dict)
                    or profile.get("name") != "effective-driver-contract-v1"
                    or profile.get("mode") != "controlled-fresh-build-v1"
                    or profile.get("disposition") != "advisory"
                    or not isinstance(profile.get("checks"), dict)
                    or profile["checks"].get("effectiveOptionPolicy") != "pass"
                    or profile["checks"].get("implicitConfiguration") != "partial"):
                raise BuildError(
                    f"self-test {label} did not report an advisory compile profile"
                )
        target_parity = dict(first["build"]["buildParityIdentity"])
        standalone_parity = dict(standalone["build"]["buildParityIdentity"])
        target_dependencies = target_parity.pop("compilerDependencies", None)
        standalone_dependencies = standalone_parity.pop(
            "compilerDependencies", None
        )
        target_link_inputs = target_parity.pop("appleLinkInputs", None)
        standalone_link_inputs = standalone_parity.pop("appleLinkInputs", None)
        for parity in (target_parity, standalone_parity):
            runtime_profile = parity.get("appleRuntime")
            if isinstance(runtime_profile, dict):
                parity["appleRuntime"] = {
                    key: value for key, value in runtime_profile.items()
                    if key != "executableDigest"
                }
        _assert_equal(
            target_parity, standalone_parity,
            "target/standalone build parity core",
        )
        if not isinstance(target_link_inputs, dict) or not isinstance(
                standalone_link_inputs, dict):
            raise BuildError("self-test omitted target/standalone Apple link evidence")
        for key in ("status", "method", "systemName", "compilerId"):
            _assert_equal(
                target_link_inputs[key], standalone_link_inputs[key],
                "target/standalone Apple linker-input " + key,
            )
        if target_link_inputs["status"] == "captured":
            target_selected = {
                (row["sdkRoot"], row["path"]): row
                for row in target_link_inputs["externalInputs"]
            }
            standalone_selected = {
                (row["sdkRoot"], row["path"]): row
                for row in standalone_link_inputs["externalInputs"]
            }
            for path_key in sorted(target_selected.keys() & standalone_selected.keys()):
                _assert_equal(
                    target_selected[path_key], standalone_selected[path_key],
                    "shared target/standalone selected Apple link input "
                    + path_key[1],
                )
        else:
            _assert_equal(
                target_link_inputs, standalone_link_inputs,
                "unavailable target/standalone Apple link-input profile",
            )
        if not isinstance(target_link_inputs, dict) or not isinstance(
                standalone_link_inputs, dict):
            raise BuildError("self-test omitted target/standalone Apple link evidence")
        for key in ("status", "method", "systemName", "compilerId"):
            _assert_equal(
                target_link_inputs[key], standalone_link_inputs[key],
                "target/standalone Apple linker-input " + key,
            )
        if target_link_inputs["status"] == "captured":
            target_selected = {
                (row["sdkRoot"], row["path"]): row
                for row in target_link_inputs["externalInputs"]
            }
            standalone_selected = {
                (row["sdkRoot"], row["path"]): row
                for row in standalone_link_inputs["externalInputs"]
            }
            for path_key in sorted(target_selected.keys() & standalone_selected.keys()):
                _assert_equal(
                    target_selected[path_key], standalone_selected[path_key],
                    "shared target/standalone selected Apple link input "
                    + path_key[1],
                )
        else:
            _assert_equal(
                target_link_inputs, standalone_link_inputs,
                "unavailable target/standalone Apple link-input profile",
            )
        if not isinstance(target_dependencies, dict) or not isinstance(
            standalone_dependencies, dict
        ):
            raise BuildError("self-test omitted target/standalone dependency evidence")
        _assert_equal(
            target_dependencies["status"], standalone_dependencies["status"],
            "target/standalone dependency capture status",
        )
        if target_dependencies["status"] == "captured":
            target_inputs = {
                row["path"]: row for row in target_dependencies["externalInputs"]
            }
            standalone_inputs = {
                row["path"]: row
                for row in standalone_dependencies["externalInputs"]
            }
            for path in sorted(target_inputs.keys() & standalone_inputs.keys()):
                _assert_equal(
                    target_inputs[path], standalone_inputs[path],
                    "shared target/standalone compiler dependency " + path,
                )
        else:
            _assert_equal(
                target_dependencies, standalone_dependencies,
                "unavailable target/standalone dependency reason",
            )
        _self_test_target_cmake_launcher_guards(
            root,
            git_program,
            cmake_program,
            git_dir,
            lock,
            manifest,
            semantic_manifest,
            adapter,
        )

        tampered = json.loads(lock.read_text(encoding="utf-8"))
        tampered["archive"]["sha256"] = "0" * 64
        tampered_lock = root / "tampered-lock.json"
        tampered_lock.write_text(
            json.dumps(tampered, indent=2) + "\n", encoding="utf-8"
        )
        try:
            build_target(
                _target_test_args(
                    root,
                    git_program,
                    cmake_program,
                    git_dir,
                    tampered_lock,
                    manifest,
                    semantic_manifest,
                    adapter,
                    "tampered",
                )
            )
        except BuildError as exc:
            if "archive SHA-256 differs" not in str(exc):
                raise
        else:
            raise BuildError("self-test accepted a tampered archive digest")


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--git", default="git", help=argparse.SUPPRESS)
    parser.add_argument("--cmake", default="cmake", help=argparse.SUPPRESS)
    subparsers = parser.add_subparsers(dest="command")

    target = subparsers.add_parser(
        "build-target", help="build the adapter from the exact locked target archive"
    )
    target.add_argument("--target-git-dir", type=Path, required=True)
    target.add_argument("--build-dir", type=Path, required=True)
    target.add_argument(
        "--parent-cmake-build-dir",
        type=Path,
        help="mirror the generator/toolchain and flags from this configured build",
    )
    target.add_argument("--lock", type=Path, default=DEFAULT_LOCK)
    target.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    target.add_argument(
        "--semantic-manifest", type=Path, default=DEFAULT_SEMANTIC_MANIFEST
    )
    target.add_argument("--adapter-source", type=Path, default=DEFAULT_ADAPTER)
    target.add_argument(
        "--adapter-support-input", type=Path, action="append", default=[]
    )
    target.add_argument("--wrapper-source", type=Path, default=DEFAULT_WRAPPER)
    target.add_argument(
        "--interface-profile", choices=(INTERFACE_PROFILE_NAME,),
        default=INTERFACE_PROFILE_NAME,
    )
    target.add_argument("--configuration", default="Release")
    target.add_argument("--parallel", type=int)
    target.add_argument("--receipt", type=Path)
    target.add_argument("--git", default="git", help=argparse.SUPPRESS)
    target.add_argument("--cmake", default="cmake", help=argparse.SUPPRESS)

    standalone = subparsers.add_parser(
        "receipt-standalone",
        help="write a direct-option receipt for an existing standalone build",
    )
    standalone.add_argument("--executable", type=Path, required=True)
    standalone.add_argument("--static-library", type=Path, required=True)
    standalone.add_argument("--cmake-build-dir", type=Path, required=True)
    standalone.add_argument("--adapter-source", type=Path, default=DEFAULT_ADAPTER)
    standalone.add_argument(
        "--interface-profile", choices=(INTERFACE_PROFILE_NAME,),
        default=INTERFACE_PROFILE_NAME,
    )
    standalone.add_argument(
        "--semantic-manifest", type=Path, default=DEFAULT_SEMANTIC_MANIFEST
    )
    standalone.add_argument("--commit")
    standalone.add_argument("--config-source", type=Path, action="append")
    standalone.add_argument("--configuration", default="Release")
    standalone.add_argument("--receipt", type=Path)
    standalone.add_argument("--git", default="git", help=argparse.SUPPRESS)
    standalone.add_argument("--cmake", default="cmake", help=argparse.SUPPRESS)

    controlled = subparsers.add_parser(
        "build-standalone",
        help="build and receipt a fresh standalone adapter under a controlled driver context",
    )
    controlled.add_argument("--source-root", type=Path, default=REPOSITORY_ROOT)
    controlled.add_argument(
        "--source-list", type=Path,
        default=REPOSITORY_ROOT / "libdxfrw_sources.cmake",
    )
    controlled.add_argument("--build-dir", type=Path, required=True)
    controlled.add_argument(
        "--parent-cmake-build-dir", type=Path,
        help="mirror generator, compiler, and toolchain settings from a configured build",
    )
    controlled.add_argument("--adapter-source", type=Path, default=DEFAULT_ADAPTER)
    controlled.add_argument(
        "--adapter-support-input", type=Path, action="append", default=[]
    )
    controlled.add_argument(
        "--interface-profile", choices=(INTERFACE_PROFILE_NAME,),
        default=INTERFACE_PROFILE_NAME,
    )
    controlled.add_argument(
        "--semantic-manifest", type=Path, default=DEFAULT_SEMANTIC_MANIFEST
    )
    controlled.add_argument("--commit")
    controlled.add_argument("--configuration", default="Release")
    controlled.add_argument("--parallel", type=int)
    controlled.add_argument("--receipt", type=Path)
    controlled.add_argument("--git", default="git", help=argparse.SUPPRESS)
    controlled.add_argument("--cmake", default="cmake", help=argparse.SUPPRESS)
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = _parser()
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            if args.command is not None:
                raise BuildError("--self-test does not accept a subcommand")
            self_test(args.git, args.cmake)
            print("build_semantic_adapter self-test: PASS")
            return 0
        if args.command == "build-target":
            _, path = build_target(args)
            print(f"semantic target adapter build: PASS ({path})")
            return 0
        if args.command == "receipt-standalone":
            _, path = receipt_standalone(args)
            print(f"semantic standalone adapter receipt: PASS ({path})")
            return 0
        if args.command == "build-standalone":
            _, path = build_standalone(args)
            print(f"controlled semantic standalone build: PASS ({path})")
            return 0
        parser.error("a subcommand is required unless --self-test is used")
    except (BuildError, OSError, UnicodeError, ValueError, KeyError) as exc:
        print(f"semantic adapter build: FAIL: {exc}", file=sys.stderr)
        return 2
    return 2


if __name__ == "__main__":
    sys.exit(main())
