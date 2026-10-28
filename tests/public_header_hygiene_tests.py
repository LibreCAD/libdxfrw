#!/usr/bin/env python3
"""Compiler-only public None-macro contract; no drawing/library runtime tests."""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import re
import sys
import tempfile
import time

TOOLS = Path(__file__).resolve().parents[1] / "tools"
sys.path.insert(0, str(TOOLS))
from bounded_process import run_bounded
from check_staged_package import PUBLIC_HEADERS

ENUMS = {
    "Admission": ("DRW_DxfIdentitySource::Admission", "Unobserved", "Accepted"),
    "DwgDataStorageWriterOperation": ("DwgDataStorageWriterOperation", "NoOperation", "WriteMLeaderStyle"),
    "OperationKind": ("DRW::OperationKind", "NoOperation", "Read"),
    "OperationPhase": ("DRW::OperationPhase", "NoPhase", "Argument"),
    "OperationCause": ("DRW::OperationCause", "NoCause", "InvalidArgument"),
    "DebugLevel": ("DRW::DebugLevel", "Disabled", "Debug"),
    "DRW_DwgFrameCoverageReason": ("DRW_DwgFrameCoverageReason", "NoReason", "ReceiptPublished"),
    "DRW_DwgClassCoverageReason": ("DRW_DwgClassCoverageReason", "NoReason", "Parse"),
    "Units": ("DRW_Header::Units", "Unitless", "Inch"),
    "DwgDataStorageWriterBinding": ("DwgDataStorageWriterBinding", "NoBinding", "MLeaderStyle"),
    "DwgIntegrityAddressSpace": ("DwgIntegrityAddressSpace", "NoAddressSpace", "PhysicalFile"),
}
INCLUDES = "".join('#include <%s>\n' % name for name in PUBLIC_HEADERS)
SAFE_ASSERTS = "".join(
    'static_assert(static_cast<int>(%s::%s) == 0);\n'
    'static_assert(static_cast<int>(%s::%s) == 1);\n'
    'static_assert(std::is_same<decltype(%s::%s), %s>::value);\n'
    % (scope, safe, scope, following, scope, safe, scope)
    for scope, safe, following in ENUMS.values())
SAFE_ASSERTS += "static_assert(DRW_DxfIdentitySource::NoIndex == (std::numeric_limits<std::size_t>::max)());\n"
LEGACY_ASSERTS = "".join(
    'static_assert(%s::%s == %s::None);\n' % (scope, safe, scope)
    for scope, safe, _ in ENUMS.values())

LEGACY_ASSERTS += "static_assert(DRW_DxfIdentitySource::NoIndex == DRW_DxfIdentitySource::None);\n"


def checked(command: list[str], root: Path, timeout: float = 15) -> None:
    result = run_bounded(command, timeout=timeout, output_limit=65536, cwd=root)
    if result.returncode:
        raise RuntimeError("compiler contract failed:\n" + result.stdout.decode("utf-8", "replace")
                           + result.stderr.decode("utf-8", "replace"))


def enum_tokens(text: str) -> dict[str, list[str]]:
    result = {}
    for name in ENUMS:
        matches = re.findall(r"\benum(?:\s+class)?\s+" + name
                             + r"\b[^;{}]*\{[^{}]*\}\s*;", text)
        if len(matches) != 1:
            raise RuntimeError("preprocessed enum missing/ambiguous: " + name)
        result[name] = re.findall(r"\w+|[^\w\s]", matches[0])
    return result


@dataclass(frozen=True)
class CmakeContext:
    cmake: str
    generator: str
    compiler: str
    compiler_id: str
    frontend_variant: str = ""
    platform: str = ""
    toolset: str = ""
    instance: str = ""
    toolchain: str = ""
    make_program: str = ""
    compiler_target: str = ""
    sysroot: str = ""
    system_name: str = ""
    system_version: str = ""
    system_processor: str = ""
    configuration: str = "Release"


def cmake_quote(value: str) -> str:
    """Quote paths/values as data, including spaces, semicolons and brackets."""
    equals = ""
    while "]" + equals + "]" in value:
        equals += "="
    return "[" + equals + "[" + value + "]" + equals + "]"


def configure_command(context: CmakeContext, root: Path) -> list[str]:
    command = [context.cmake, "-S", str(root), "-B", str(root / "build"),
               "-G", context.generator]
    for flag, value in (("-A", context.platform), ("-T", context.toolset)):
        if value:
            command += [flag, value]
    for key, value in (
            ("CMAKE_CXX_COMPILER", context.compiler),
            ("CMAKE_GENERATOR_INSTANCE", context.instance),
            ("CMAKE_TOOLCHAIN_FILE", context.toolchain),
            ("CMAKE_MAKE_PROGRAM", context.make_program),
            ("CMAKE_CXX_COMPILER_TARGET", context.compiler_target),
            ("CMAKE_SYSROOT", context.sysroot),
            ("CMAKE_SYSTEM_NAME", context.system_name),
            ("CMAKE_SYSTEM_VERSION", context.system_version),
            ("CMAKE_SYSTEM_PROCESSOR", context.system_processor),
            ("CMAKE_BUILD_TYPE", context.configuration)):
        if value:
            command.append("-D%s=%s" % (key, value))
    return command


def build_command(context: CmakeContext, root: Path) -> list[str]:
    return [context.cmake, "--build", str(root / "build"), "--config",
            context.configuration, "--target", "header_hygiene", "--parallel", "2"]


def consumer_project(context: CmakeContext, include_root: Path,
                     sources: list[Path], definitions: list[Path],
                     mixed: list[Path], x11_include: Path | None) -> str:
    # Use configured CMake targets for every MSVC-style compiler operation.
    # In particular, a Visual Studio generator supplies its selected VC/SDK
    # include/library paths even when CTest was not launched by vcvarsall.
    lines = ["cmake_minimum_required(VERSION 3.28)",
             "project(HeaderHygiene LANGUAGES CXX)",
             "set(CMAKE_CXX_STANDARD 17)", "set(CMAKE_CXX_STANDARD_REQUIRED ON)",
             "set(CMAKE_CXX_EXTENSIONS OFF)",
             "if(NOT CMAKE_CXX_COMPILER_ID STREQUAL " + cmake_quote(context.compiler_id) + ")",
             '  message(FATAL_ERROR "header consumer compiler ID changed")', "endif()",
             "if(NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "
             + cmake_quote(context.frontend_variant) + ")",
             '  message(FATAL_ERROR "header consumer compiler frontend changed")', "endif()",
             "file(REAL_PATH " + cmake_quote(context.compiler) + " expected_compiler)",
             'file(REAL_PATH "${CMAKE_CXX_COMPILER}" selected_compiler)',
             "if(WIN32)", '  string(TOLOWER "${expected_compiler}" expected_compiler)',
             '  string(TOLOWER "${selected_compiler}" selected_compiler)', "endif()",
             "if(NOT selected_compiler STREQUAL expected_compiler)",
             '  message(FATAL_ERROR "header consumer compiler binding changed")', "endif()",
             "function(header_options target)",
             "  target_compile_options(${target} PRIVATE /EHsc /W3 /WX /permissive- /bigobj /utf-8)",
             "  target_include_directories(${target} PRIVATE " + cmake_quote(str(include_root)) + ")"]
    if x11_include is not None:
        lines.append("  target_include_directories(${target} PRIVATE "
                     + cmake_quote(str(x11_include)) + ")")
    lines += ["endfunction()", "add_library(header_entries OBJECT"]
    lines += ["  " + cmake_quote(str(path)) for path in sources]
    lines += [")", "header_options(header_entries)"]
    for index, path in enumerate(definitions):
        target = "header_preprocess_%d" % index
        lines += ["add_library(%s OBJECT %s)" % (target, cmake_quote(str(path))),
                  "header_options(%s)" % target,
                  # /P deliberately produces only .i. OBJECT libraries have no
                  # archive/link step and are never consumed as object inputs.
                  "target_compile_options(%s PRIVATE /P %s)" % (
                      target, cmake_quote("/Fi" + str(path.with_suffix(".i"))))]
    lines += ["add_executable(header_mixed " + " ".join(cmake_quote(str(path)) for path in mixed) + ")",
              "header_options(header_mixed)",
              'file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/mixed-path-$<CONFIG>.txt"',
              '  CONTENT "$<TARGET_FILE:header_mixed>")',
              "add_custom_target(header_hygiene DEPENDS header_entries header_preprocess_0 header_preprocess_1 header_mixed)"]
    return "\n".join(lines) + "\n"


def construction_selftest() -> None:
    # These controls need neither an installed compiler nor a Windows host.
    root = Path("consumer directory")
    context = CmakeContext("cmake tool", "Visual Studio 17 2022", "compiler path/cl.exe", "MSVC",
                           "MSVC", "x64", "v143,host=x64", "VS instance", "toolchain file.cmake",
                           "MSBuild.exe", "", "SDK root", "Windows", "10.0", "AMD64", "Debug")
    command = configure_command(context, root)
    if command[:7] != ["cmake tool", "-S", str(root), "-B", str(root / "build"),
                       "-G", "Visual Studio 17 2022"]:
        raise RuntimeError("CMake generator command construction changed")
    required = ["-A", "x64", "-T", "v143,host=x64", "-DCMAKE_GENERATOR_INSTANCE=VS instance",
                "-DCMAKE_TOOLCHAIN_FILE=toolchain file.cmake", "-DCMAKE_CXX_COMPILER=compiler path/cl.exe",
                "-DCMAKE_MAKE_PROGRAM=MSBuild.exe", "-DCMAKE_SYSROOT=SDK root",
                "-DCMAKE_SYSTEM_NAME=Windows", "-DCMAKE_SYSTEM_VERSION=10.0",
                "-DCMAKE_SYSTEM_PROCESSOR=AMD64", "-DCMAKE_BUILD_TYPE=Debug"]
    if any(value not in command for value in required):
        raise RuntimeError("CMake consumer lost a configured toolchain control")
    minimal = configure_command(CmakeContext("cmake", "Ninja", "clang-cl", "Clang", "MSVC"), root)
    if "-A" in minimal or "-T" in minimal or any(arg.endswith("=") for arg in minimal):
        raise RuntimeError("empty CMake controls were passed as overrides")
    if build_command(context, root) != ["cmake tool", "--build", str(root / "build"),
                                       "--config", "Debug", "--target", "header_hygiene", "--parallel", "2"]:
        raise RuntimeError("CMake consumer build/configuration command changed")
    quoted = cmake_quote("a; b ]] c ]=] d")
    if quoted != "[==[a; b ]] c ]=] d]==]":
        raise RuntimeError("CMake value quoting changed")
    project = consumer_project(context, Path("headers"), [Path("entry.cpp")],
                               [Path("plain.cpp"), Path("macro.cpp")],
                               [Path("first.cpp"), Path("second.cpp")], None)
    if ("/EHsc /W3 /WX" not in project
            or len(re.findall(r"add_library\([^ ]+ OBJECT\b", project)) != 3
            or "/P [[/Fiplain.i]]" not in project or "add_executable(header_mixed" not in project
            or "execute_process" in project or "add_custom_command" in project):
        raise RuntimeError("MSVC consumer no longer uses strict configured CMake targets")


def check(include_root: Path, compiler: str, msvc: bool,
          x11_include: Path | None, target_flags: list[str],
          context: CmakeContext | None = None) -> None:
    with tempfile.TemporaryDirectory(prefix="libdxfrw-header-hygiene-") as directory:
        root = Path(directory)
        flags = ["-std=c++17", "-Wall", "-Wextra", "-pedantic", "-Werror",
                 "-I" + str(include_root)]
        syntax = ["-fsyntax-only"]
        flags += target_flags
        sources = []

        def source(name: str, body: str) -> Path:
            path = root / (name + ".cpp")
            path.write_text("#include <type_traits>\n" + body, encoding="utf-8")
            return path

        # Each header is a direct entry point; a prior umbrella include must
        # not hide an unguarded declaration or an unbalanced repeat guard.
        for index, header in enumerate(PUBLIC_HEADERS):
            repeated = '#include <%s>\n#include <%s>\n' % (header, header)
            restored = '#if !defined(None) || None != 731\n#error caller macro changed\n#endif\n'
            sources.append(source("first-%d" % index, "#define None 731\n" + repeated + restored))
            sources.append(source("last-%d" % index,
                                  repeated + "#define None 731\n" + repeated + restored))
        sources.append(source("undefined", INCLUDES + INCLUDES
                              + "#ifdef None\n#error undefined macro became defined\n#endif\n"
                              + SAFE_ASSERTS + LEGACY_ASSERTS))
        sources.append(source("function-like", "#define None(x) ((x) + 5)\n" + INCLUDES
                              + "#if None(7) != 12\n#error function macro changed\n#endif\n"
                              + SAFE_ASSERTS))
        sources.append(source("nested-stack", '#define None 11\n#pragma push_macro("None")\n'
                              + "#undef None\n#define None 731\n" + INCLUDES + SAFE_ASSERTS
                              + '#if None != 731\n#error active stack value changed\n#endif\n'
                              + '#pragma pop_macro("None")\n#if None != 11\n#error caller stack changed\n#endif\n'))
        # Match the repository's small -j2 build budget. Entry-point checks
        # remain separate TUs so a prior umbrella include cannot mask a guard.
        if not msvc:
            with ThreadPoolExecutor(max_workers=2) as workers:
                list(workers.map(lambda path: checked(
                    [compiler, *flags, *syntax, str(path)], root), sources))

        # Compare actual preprocessed declarations, not merely equal sizeof or
        # link success: those do not establish the ODR's token requirement.
        definition_paths = []
        for name, prefix in (("plain-enums", ""), ("macro-enums", "#define None 731\n")):
            path = source(name, prefix + INCLUDES)
            definition_paths.append(path)
            output = root / (name + ".i")
            if not msvc:
                checked([compiler, *flags, "-E", "-P", "-o", str(output), str(path)], root)

        first = source("mixed-first", "#define None 731\n" + INCLUDES + SAFE_ASSERTS
                       + "DRW::OperationCause header_cause() { return DRW::OperationCause::NoCause; }\n")
        second = source("mixed-second", INCLUDES + LEGACY_ASSERTS
                        + "DRW::OperationCause header_cause();\n"
                        + "int main() { return header_cause() == DRW::OperationCause::None ? 0 : 1; }\n")
        executable = root / "mixed"
        if not msvc:
            checked([compiler, *flags, str(first), str(second), "-o", str(executable)], root)

        if x11_include is not None:
            for name, body in (("x11-first", "#include <X11/X.h>\n" + INCLUDES),
                               ("x11-last", INCLUDES + "#include <X11/X.h>\n")):
                path = source(name, body + SAFE_ASSERTS
                              + "#if !defined(None) || None != 0L\n#error X11 macro changed\n#endif\n")
                sources.append(path)
                if not msvc:
                    checked([compiler, *flags, "-I" + str(x11_include), *syntax, str(path)], root)
        if msvc:
            if context is None:
                raise RuntimeError("MSVC-style consumer requires an explicit CMake context")
            (root / "CMakeLists.txt").write_text(
                consumer_project(context, include_root, sources, definition_paths,
                                 [first, second], x11_include), encoding="utf-8")
            checked(configure_command(context, root), root)
            checked(build_command(context, root), root, timeout=60)
            binding = root / "build" / ("mixed-path-" + context.configuration + ".txt")
            if binding.stat().st_size > 8192:
                raise RuntimeError("mixed translation-unit binding exceeds bounded input")
            executable = Path(binding.read_text(encoding="utf-8", errors="strict"))
        definitions = []
        for path in definition_paths:
            output = path.with_suffix(".i")
            if output.stat().st_size > 8 * 1024 * 1024:
                raise RuntimeError("preprocessed header contract exceeds bounded input")
            definitions.append(enum_tokens(output.read_text(encoding="utf-8", errors="strict")))
        if definitions[0] != definitions[1]:
            raise RuntimeError("public enum definitions depend on caller macro state")
        # Link only: the configured compiler may target another machine.
        if not executable.is_file():
            raise RuntimeError("mixed translation-unit link produced no executable")
        print("public header hygiene: PASS (%d entry points, %d identical enums, mixed-TU link%s)"
              % (len(PUBLIC_HEADERS), len(ENUMS), ", actual X11" if x11_include else "; actual X11 unavailable/not requested"))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--compiler")
    parser.add_argument("--compiler-id")
    parser.add_argument("--frontend-variant", default="")
    parser.add_argument("--compiler-target", default="")
    parser.add_argument("--sysroot", default="")
    parser.add_argument("--include-root", type=Path)
    parser.add_argument("--x11-include", type=Path)
    parser.add_argument("--cmake")
    parser.add_argument("--generator")
    parser.add_argument("--generator-platform", default="")
    parser.add_argument("--generator-toolset", default="")
    parser.add_argument("--generator-instance", default="")
    parser.add_argument("--toolchain", default="")
    parser.add_argument("--make-program", default="")
    parser.add_argument("--system-name", default="")
    parser.add_argument("--system-version", default="")
    parser.add_argument("--system-processor", default="")
    parser.add_argument("--configuration", default="")
    args = parser.parse_args()
    construction_selftest()
    if args.self_test:
        print("public header hygiene construction: PASS (compiler-free CMake/MSVC controls)")
        return 0
    if not args.compiler or not args.compiler_id or args.include_root is None:
        parser.error("--compiler, --compiler-id and --include-root are required")
    if args.compiler_id not in ("GNU", "Clang", "AppleClang", "MSVC"):
        parser.error("unsupported macro-stack compiler; no unqualified fallback")
    if args.x11_include and not (args.x11_include / "X11/X.h").is_file():
        parser.error("--x11-include does not contain X11/X.h")
    msvc = args.compiler_id == "MSVC" or args.frontend_variant == "MSVC"
    context = None
    if msvc:
        if not args.cmake or not args.generator:
            parser.error("MSVC-style consumer requires --cmake and configured --generator")
        context = CmakeContext(args.cmake, args.generator, args.compiler, args.compiler_id,
                               args.frontend_variant, args.generator_platform,
                               args.generator_toolset, args.generator_instance,
                               str(Path(args.toolchain).resolve()) if args.toolchain else "",
                               args.make_program, args.compiler_target, args.sysroot,
                               args.system_name, args.system_version, args.system_processor,
                               args.configuration or "Release")
    target_flags = []
    if args.compiler_target and not msvc:
        if args.compiler_id not in ("Clang", "AppleClang"):
            parser.error("explicit compiler-target control requires the Clang driver")
        target_flags.append("--target=" + args.compiler_target)
    if args.sysroot and not msvc:
        target_flags.append("--sysroot=" + args.sysroot)
    started = time.monotonic()
    check(args.include_root.resolve(), args.compiler, msvc, args.x11_include, target_flags, context)
    print("compiler-only duration: %.2fs; no drawing/runtime/native qualification" % (time.monotonic() - started))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
