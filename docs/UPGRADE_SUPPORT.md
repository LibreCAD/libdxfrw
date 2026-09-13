# libdxfrw 2.0 support and release notes

This branch converges standalone libdxfrw with the pinned LibreCAD master
implementation. The 2.0 boundary requires C++17 and is an ABI-breaking release
even where source-level compatibility shims remain.

Support status is evidence-based, not dispatch-based. A reader/writer route may
be recognized by a version or class table while remaining experimental until a
fixture-policy-eligible regression, an authoritative specification vector, or
an independent oracle qualifies it. The current writer matrix deliberately
promotes no feature row; the modern TU terminator question is explicitly
deferred pending an authoritative ODA/sample check.

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
fixture-policy checks. The standalone package handoff for LibreCAD is recorded
in `metadata/librecad-system-package-handoff.md`; until that sibling CMake
change lands, system-package mode remains an explicit external follow-up.

Known limitations remain visible in the plan: structured operation diagnostics
are not yet exposed, AC1032/R2018 remains a pass-through reader, and broad
feature recognition exceeds independently qualified support.
