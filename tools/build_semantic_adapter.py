#!/usr/bin/env python3
"""Build the pinned target adapter or attest an existing standalone build.

The target build is deliberately independent of LibreCAD's top-level CMake
project.  It archives exactly the source paths and commit named by the lock,
checks the archive and manifest identities before extraction, and builds the
one shared adapter source against that extracted source closure.
"""

from __future__ import annotations

import argparse
import hashlib
import json
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
    build_tree: Path, cmake_program: str, configuration: str
) -> dict[str, object]:
    _ensure_codemodel(build_tree, configuration, cmake_program)
    cache = _parse_cmake_cache(build_tree)
    compiler_text = _cmake_generated_text(
        build_tree, "CMakeCXXCompiler.cmake", "C++ compiler identity"
    )
    system_text = _cmake_generated_text(
        build_tree, "CMakeSystem.cmake", "CMake target-system identity"
    )
    api_identity = _file_api_cmake_identity(build_tree)
    version_output = _run([cmake_program, "--version"]).splitlines()
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
    config_upper = configuration.upper()
    return {
        "cmakeVersion": cmake_version,
        **api_identity,
        "generatorPlatform": cache.get("CMAKE_GENERATOR_PLATFORM", ""),
        "generatorToolset": cache.get("CMAKE_GENERATOR_TOOLSET", ""),
        "generatorInstance": cache.get("CMAKE_GENERATOR_INSTANCE", ""),
        "configurationTypes": configuration_types,
        "compilerPath": compiler_path,
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
        "compilerLinkerPath": _cmake_set_value(
            compiler_text, "CMAKE_CXX_COMPILER_LINKER",
            _cmake_set_value(compiler_text, "CMAKE_LINKER"),
        ),
        "compilerLinkerId": _cmake_set_value(
            compiler_text, "CMAKE_CXX_COMPILER_LINKER_ID"
        ),
        "compilerLinkerVersion": _cmake_set_value(
            compiler_text, "CMAKE_CXX_COMPILER_LINKER_VERSION"
        ),
        "compilerLinkerFrontendVariant": _cmake_set_value(
            compiler_text, "CMAKE_CXX_COMPILER_LINKER_FRONTEND_VARIANT"
        ),
        "archiverPath": _cmake_set_value(compiler_text, "CMAKE_AR"),
        "ranlibPath": _cmake_set_value(compiler_text, "CMAKE_RANLIB"),
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
    }


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
    build_tree: Path, configuration: str, cmake_program: str
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
    _run([cmake_program, "-S", home, "-B", str(build_tree)])
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
) -> dict[str, object]:
    expected_library = library_path.resolve()
    _, targets = _ensure_codemodel(build_tree, configuration, cmake_program)
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
            return {
                "verifiedLinkCommandDigest": _sha256_file(link_files[0]),
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
) -> dict[str, object]:
    _, targets = _ensure_codemodel(build_tree, configuration, cmake_program)
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
    flag_values = [
        row.get("fragment")
        for row in fragments
        if isinstance(row, dict) and isinstance(row.get("fragment"), str)
    ]
    combined_flags = " ".join(flag_values)
    cmake_identity = _cmake_metadata(build_tree, cmake_program, configuration)
    msvc_style = (
        cmake_identity["compilerId"] == "MSVC"
        or cmake_identity["compilerFrontendVariant"] == "MSVC"
    )
    lower_flags = combined_flags.lower()
    if msvc_style:
        if "/wx" not in lower_flags or "/bigobj" not in lower_flags:
            raise BuildError("MSVC-style semantic adapter lacks /WX or /bigobj")
    elif "-Werror" not in combined_flags:
        raise BuildError("semantic adapter compile does not enforce warning errors")
    defines_value = group.get("defines")
    if not isinstance(defines_value, list):
        raise BuildError("semantic adapter compile definitions are absent")
    defines = {
        row.get("define")
        for row in defines_value
        if isinstance(row, dict) and isinstance(row.get("define"), str)
    }
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
    if not required_defines.issubset(defines):
        raise BuildError("semantic adapter compile definitions are stale")
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
        "compileContractEnforced": True,
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
    methods = re.findall(
        r"\bvirtual\b(?:(?![;{}]).)*?\b([A-Za-z_]\w*)\s*\(",
        body,
        flags=re.DOTALL,
    )
    methods = [name for name in methods if name != "DRW_Interface"]
    if not methods or len(methods) != len(set(methods)):
        raise BuildError(
            "DRW_Interface callback inventory is empty or contains overloads; "
            "update the inventory parser explicitly"
        )
    return sorted(methods)


def _adapter_override_methods(adapter_source: Path) -> list[str]:
    text = _require_file(adapter_source, "semantic adapter source").read_text(
        encoding="utf-8", errors="strict"
    )
    body = _class_body(text, "SemanticSink")
    if re.search(
        r"^\s*#\s*(?:if|ifdef|ifndef|elif|else|endif)\b",
        body,
        flags=re.MULTILINE,
    ):
        raise BuildError(
            "SemanticSink callback inventory contains conditional preprocessing"
        )
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


def _verify_interface_contract(interface_header: Path,
                               adapter_source: Path) -> dict[str, object]:
    interface_methods = _interface_virtual_methods(interface_header)
    adapter_methods = _adapter_override_methods(adapter_source)
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


def _split_command_fragment(fragment: str, label: str) -> list[str]:
    try:
        tokens = shlex.split(fragment, posix=os.name != "nt")
    except ValueError as exc:
        raise BuildError(f"cannot parse {label}: {exc}") from exc
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
) -> dict[str, object]:
    _, targets = _ensure_codemodel(build_tree, configuration, cmake_program)
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
    return {
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


PARENT_CMAKE_FORWARD_KEYS = (
    "CMAKE_CXX_COMPILER",
    "CMAKE_CXX_COMPILER_LAUNCHER",
    "CMAKE_CXX_LINKER_LAUNCHER",
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
) -> None:
    command = [
        cmake_program,
        "-S",
        str(wrapper),
        "-B",
        str(build_tree),
        "-DLIBDXFRW_SEMANTIC_TARGET_ROOT=" + str(target_root),
        "-DLIBDXFRW_SEMANTIC_ADAPTER_SOURCE=" + str(adapter_source),
        "-DLIBDXFRW_SEMANTIC_ADAPTER_COMMIT=" + commit,
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
            command.append(f"-D{key}={value}")
        if not bool(parent_settings["multiConfig"]):
            command.append("-DCMAKE_BUILD_TYPE=" + configuration)
    if config_digest is not None:
        command.append("-DLIBDXFRW_SEMANTIC_CONFIG_DIGEST=" + config_digest)
    _run(command)
    if parent_settings is None:
        # Discover whether the default generator is single- or multi-config.
        # A second configure pins the requested build type only for the former.
        cache = _parse_cmake_cache(build_tree)
        if not cache.get("CMAKE_CONFIGURATION_TYPES", ""):
            _run(command + ["-DCMAKE_BUILD_TYPE=" + configuration])
    _verify_requested_configuration(build_tree, configuration)


def _find_one(paths: list[Path], label: str) -> Path:
    matches = sorted({path.resolve() for path in paths if path.is_file()})
    if len(matches) != 1:
        raise BuildError(f"expected exactly one {label}; found {matches}")
    return matches[0]


def _target_artifacts(
    build_tree: Path, configuration: str, cmake_program: str
) -> tuple[Path, Path]:
    _, targets = _ensure_codemodel(build_tree, configuration, cmake_program)
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
    parent_settings = _parent_cmake_settings(
        getattr(args, "parent_cmake_build_dir", None), args.configuration
    )

    build_key = _canonical_digest(
        {
            "schema": 1,
            "targetArchiveDigest": lock_data["archiveDigest"],
            "adapterSourceDigest": adapter_source_digest,
            "wrapperDigest": wrapper_digest,
            "semanticManifestDigest": semantic_manifest_digest,
            "configuration": args.configuration,
            "parentCMake": parent_settings,
        }
    )
    build_tree = build_dir / "cmake" / build_key
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
    )
    cmake_data = _cmake_metadata(build_tree, args.cmake, args.configuration)
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
    _run(build_command)
    executable, library = _target_artifacts(
        build_tree, args.configuration, args.cmake
    )
    link_identity = _verify_link_closure(
        build_tree=build_tree,
        configuration=args.configuration,
        executable_target=TARGET_EXECUTABLE,
        library_target=TARGET_LIBRARY,
        library_path=library,
        cmake_program=args.cmake,
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
    )
    interface_identity = _verify_interface_contract(
        target_root / "src/drw_interface.h", adapter_source
    )
    build_parity_identity = _build_parity_identity(
        build_tree=build_tree,
        configuration=args.configuration,
        cmake_program=args.cmake,
        cmake_data=cmake_data,
        executable_target=TARGET_EXECUTABLE,
        library_target=TARGET_LIBRARY,
        library_path=library,
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
            "semanticManifestPath": _logical_input_path(semantic_manifest_path),
            "semanticManifestDigest": semantic_manifest_digest,
            "packageCommit": lock_data["commit"],
            "packageArchiveDigest": lock_data["archiveDigest"],
            "configFiles": config_files,
            **link_identity,
            **compile_identity,
            **interface_identity,
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
    cmake_data = _cmake_metadata(build_tree, args.cmake, args.configuration)
    _verify_requested_configuration(build_tree, args.configuration)
    config_file_records = sorted(
        [
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
    )
    interface_identity = _verify_interface_contract(
        cmake_home_path / "src/drw_interface.h", adapter_source
    )
    build_parity_identity = _build_parity_identity(
        build_tree=build_tree,
        configuration=args.configuration,
        cmake_program=args.cmake,
        cmake_data=cmake_data,
        executable_target=STANDALONE_EXECUTABLE,
        library_target="dxfrw",
        library_path=library,
    )
    build_parity_digest = _canonical_digest(build_parity_identity)
    config_identity = {
        "schema": 1,
        "side": "standalone",
        "configuration": args.configuration,
        "cmake": cmake_data,
        "inputs": {
            "adapterSourceDigest": _sha256_file(adapter_source),
            "semanticManifestPath": _logical_input_path(semantic_manifest),
            "semanticManifestDigest": semantic_manifest_digest,
            "packageCommit": commit,
            "packageArchiveDigest": None,
            "configFiles": config_file_records,
            **link_identity,
            **compile_identity,
            **interface_identity,
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
        "wrapperDigest": config_file_records[0]["digest"],
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
    _write_receipt(receipt_path, receipt)
    return receipt, receipt_path


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
    adapter.write_text(
        '#include "drw_interface.h"\n'
        '#include "minimal.h"\n'
        "#if !defined(LIBDXFRW_SEMANTIC_SIDE_TARGET) && "
        "!defined(LIBDXFRW_SEMANTIC_SIDE_STANDALONE)\n"
        '#error "semantic side definition is missing"\n'
        "#endif\n"
        "class SemanticSink final : public DRW_Interface {\n"
        "public:\n"
        "    void callback() override {}\n"
        "};\n"
        "int main() { return semantic_self_test_value() == 42 ? 0 : 1; }\n",
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
        wrapper_source=DEFAULT_WRAPPER,
        target_git_dir=git_dir,
        git=git_program,
        cmake=cmake_program,
        configuration="Release",
        parallel=2,
        receipt=None,
    )


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


def self_test(git_program: str, cmake_program: str) -> None:
    with tempfile.TemporaryDirectory(prefix="semantic-adapter-self-test-") as directory:
        root = Path(directory)
        git_dir, commit, _, adapter = _self_test_repository(root, git_program)
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
            if "conditional preprocessing" not in str(exc):
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
            "staticLibraryDigest",
            "binaryDigest",
            "linkedClosureDigest",
        ):
            _assert_equal(first["adapter"][key], second["adapter"][key], key)
        executable = first_path.parent / first["artifacts"]["executable"]["path"]
        _run([str(executable.resolve())])

        standalone_config, standalone_build, standalone_executable, standalone_library = (
            _self_test_standalone_build(
                root, git_dir.parent, adapter, cmake_program
            )
        )
        standalone_args = argparse.Namespace(
            adapter_source=adapter,
            semantic_manifest=semantic_manifest,
            executable=standalone_executable,
            static_library=standalone_library,
            cmake_build_dir=standalone_build,
            commit=commit,
            git=git_program,
            cmake=cmake_program,
            config_source=[standalone_config],
            configuration="Release",
            receipt=standalone_build / "standalone-receipt.json",
        )
        standalone, _ = receipt_standalone(standalone_args)
        _assert_equal(standalone["adapter"]["side"], "standalone", "standalone side")
        _assert_equal(standalone["targetLock"], None, "standalone target lock")
        _assert_equal(
            first["build"]["buildParityIdentity"],
            standalone["build"]["buildParityIdentity"],
            "target/standalone build parity identity",
        )
        _assert_equal(
            first["build"]["buildParityDigest"],
            standalone["build"]["buildParityDigest"],
            "target/standalone build parity digest",
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
    target.add_argument("--wrapper-source", type=Path, default=DEFAULT_WRAPPER)
    target.add_argument("--configuration", default="Release")
    target.add_argument("--parallel", type=int)
    target.add_argument("--receipt", type=Path)
    target.add_argument("--git", default="git", help=argparse.SUPPRESS)
    target.add_argument("--cmake", default="cmake", help=argparse.SUPPRESS)

    standalone = subparsers.add_parser(
        "receipt-standalone",
        help="write an identity receipt for an existing standalone build",
    )
    standalone.add_argument("--executable", type=Path, required=True)
    standalone.add_argument("--static-library", type=Path, required=True)
    standalone.add_argument("--cmake-build-dir", type=Path, required=True)
    standalone.add_argument("--adapter-source", type=Path, default=DEFAULT_ADAPTER)
    standalone.add_argument(
        "--semantic-manifest", type=Path, default=DEFAULT_SEMANTIC_MANIFEST
    )
    standalone.add_argument("--commit")
    standalone.add_argument("--config-source", type=Path, action="append")
    standalone.add_argument("--configuration", default="Release")
    standalone.add_argument("--receipt", type=Path)
    standalone.add_argument("--git", default="git", help=argparse.SUPPRESS)
    standalone.add_argument("--cmake", default="cmake", help=argparse.SUPPRESS)
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
        parser.error("a subcommand is required unless --self-test is used")
    except (BuildError, OSError, UnicodeError, ValueError, KeyError) as exc:
        print(f"semantic adapter build: FAIL: {exc}", file=sys.stderr)
        return 2
    return 2


if __name__ == "__main__":
    sys.exit(main())
