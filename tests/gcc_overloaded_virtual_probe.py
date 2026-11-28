#!/usr/bin/env python3
"""Check GCC's intentional partial-override policy and legacy source lookup."""
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
COMPILER = sys.argv[1] if len(sys.argv) == 2 else "g++"
if len(sys.argv) > 2:
    raise SystemExit("usage: gcc_overloaded_virtual_probe.py [g++]")

legacy = r'''
#include "drw_entities.h"
#include <cstdint>
enum LegacySize { LegacyZero = 0 };
struct LegacyCircle : DRW_Circle {
    bool zero(dwgBuffer* b) { return parseDwg(DRW::AC1024, b, 0); }
    bool wide(dwgBuffer* b) { return parseDwg(DRW::AC1024, b, 1L); }
    bool unsig(dwgBuffer* b) { return parseDwg(DRW::AC1024, b, std::uint32_t(1)); }
    bool named_enum(dwgBuffer* b) { return parseDwg(DRW::AC1024, b, LegacyZero); }
    bool string_buffer(dwgBuffer* b) { return DRW_Entity::parseDwg(DRW::AC1024, b, b, 0); }
    static auto member() { return &LegacyCircle::parseDwg; }
};
struct LegacyVertex : DRW_Vertex {
    bool four(dwgBuffer* b) { return parseDwg(DRW::AC1024, b, 0, 0); }
};
'''
partial = r'''
struct Base { virtual void f(); virtual void f(int); };
struct Partial : Base { void f(int) override; };
'''
complete = partial + r'''
struct Bad : Base { void f(double); };
'''
# A traditional NULL may itself cause GCC's independent -Wconversion-null
# diagnostic when converted to the historical integer parameter. Check source
# acceptance with overloaded-virtual errors enabled without changing the real
# production -Werror policy or suppressing that unrelated diagnostic.
legacy_null = r'''
#include "drw_entities.h"
#include <cstddef>
struct LegacyCircle : DRW_Circle {
    bool null_size(dwgBuffer* b) { return parseDwg(DRW::AC1024, b, NULL); }
};
'''
with tempfile.TemporaryDirectory(prefix="libdxfrw-gcc-warning-") as directory:
    base = Path(directory)
    def compile_text(name, content):
        source = base / (name + ".cpp")
        source.write_text(content)
        return subprocess.run([COMPILER, "-std=c++17", "-Wall", "-Wextra",
                               "-pedantic", "-Werror", "-Woverloaded-virtual=1",
                               "-I", str(ROOT / "src"), "-fsyntax-only", str(source)],
                              text=True, capture_output=True, check=False)
    for name, content in (("legacy", legacy), ("partial", partial)):
        result = compile_text(name, content)
        if result.returncode:
            raise SystemExit(name + " unexpectedly failed:\n" + result.stderr)
    null_source = base / "legacy_null.cpp"
    null_source.write_text(legacy_null)
    null_result = subprocess.run([COMPILER, "-std=c++17", "-Werror=overloaded-virtual",
                                  "-Woverloaded-virtual=1", "-I", str(ROOT / "src"),
                                  "-fsyntax-only", str(null_source)],
                                 text=True, capture_output=True, check=False)
    if null_result.returncode:
        raise SystemExit("legacy NULL source lookup unexpectedly failed:\n" + null_result.stderr)
    result = compile_text("complete", complete)
    if result.returncode == 0 or "overloaded-virtual" not in result.stderr:
        raise SystemExit("complete hiding must still fail with -Werror:\n" + result.stderr)
print("GCC overloaded-virtual level 1 and legacy lookup controls PASS")
