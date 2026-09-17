# libdxfrw 2.0 support and release notes

This branch converges standalone libdxfrw with the pinned LibreCAD master
implementation. The 2.0 boundary requires C++17 and is an ABI-breaking release
even where source-level compatibility shims remain.

Support status is evidence-based, not dispatch-based. A reader/writer route may
be recognized by a version or class table while remaining experimental until a
fixture-policy-eligible regression, an authoritative specification vector, or
an independent oracle qualifies it. The current matrix contains 1,345 `dwgRW`
routes and 1,475 `dxfRW` routes, with zero unmapped target rows but zero
qualified or advertised rows; source recognition is therefore not a broad
format-support claim. The modern R2007+ TU reader accepts both
length-includes-terminator and length-excludes-terminator streams; its
independent-cursor probe preserves alignment for either form, while external
support promotion remains evidence-gated.

The support ledger distinguishes dispatch, decode, callback publication, read
support, write support, raw preservation, derived rendering, and validation.
Only rows marked `QUALIFIED_FORMAT_PARITY` may be advertised. `SOURCE_PARITY`,
`DISPATCH_PARITY`, and `EXPERIMENTAL` rows remain non-promoting; external and
independent-reader results stored as hashes and summaries are advisory until
the fixture and promotion gates are satisfied.

The repository does not commit downloaded DWG/DXF bytes. A fixture may be
committed only when it is an exact Git-tracked blob already present in LibreCAD
or libdxfrw at the lock revision, or when it is created locally from scratch
with a reproducible recipe and provenance. External corpora are advisory and
must remain outside Git. Every slice runs the admission and external-fixture
hooks before commit.

The standard qualification loop is:

```sh
cmake -S . -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DLIBDXFRW_BUILD_TESTS=ON -DLIBDXFRW_BUILD_DOC=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

The release gate additionally requires staged-header, CMake-package,
pkg-config, LibreCAD system-package, sanitizer, source-scope, sync, and
fixture-policy checks. A fresh pinned LibreCAD system-package build with
`BUILD_TESTS=ON` now builds `librecad_lib`,
`librecad_filter_compile_check`, and `libdxfrw_system_fast_tests`; the latter
passes 176 assertions in 12 cases. The standalone package handoff for
LibreCAD is recorded in `metadata/librecad-system-package-handoff.md`; the
downstream sibling CMake change and hosted native-platform run remain external
release follow-ups.

CMake is the supported 2.0.0 build and requires C++17. The historical
Autotools, MinGW, and Conan recipes do not consume the canonical imported
source manifest and are deprecated for the 2.x convergence until they gain a
maintained C++17/CI lane; no qmake recipe is shipped. `DRW_VERSION` remains the
legacy 0.6.3 DXF banner for source/byte compatibility, while the package and
ABI release version is 2.0.0.

The public `dxfRW::getLastDiagnostic()` and `dwgRW::getLastDiagnostic()`
snapshots retain the legacy `DRW::error` code while exposing the operation,
first failing phase/cause, stable code/message, optional offset/handle, and a
bounded list of secondary cleanup entries. Diagnostics reset at the start of
each read/write operation; the first failure wins, and support claims still
require independent evidence.

Known limitations remain visible in the plan: AC1032/R2018 remains a
pass-through reader, and broad feature recognition exceeds independently
qualified support.
