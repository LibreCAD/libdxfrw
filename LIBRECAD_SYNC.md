# LibreCAD synchronization and installed-package integration

This file records the source boundary used by the 2.0.0 upgrade. The
standalone repository is the integration branch; the LibreCAD checkout is a
consumer and is not modified by the package check.

## Pinned revisions

| Input | Revision | Meaning |
| --- | --- | --- |
| LibreCAD target | `2043e5254fbf71729e97bd54e48a184153161ff2` | `origin/master` target recorded in `metadata/libdxfrw-target-lock.json` |
| Bundled libdxfrw snapshot | `89b762bef636c90eb370cb1af3cec80fe759cb32` | Snapshot used to derive the imported source archive |
| Standalone baseline | `e83609dd27473c38272b790a843e54eded036bbc` | Pre-upgrade `LibreCAD/libdxfrw` baseline |

The complete source lock, SHA-256 manifest, and adaptation hashes are checked
in under `metadata/`. Refreshing a pin is a new review boundary; it must not be
done implicitly while implementing a slice.

## Import and adaptation procedure

1. Verify the target and baseline revisions in `metadata/libdxfrw-target-lock.json`.
2. Import only the target-derived source manifest through
   `libdxfrw_sources.cmake`; do not copy sample payloads.
3. Apply only the paths and reasons listed in
   `metadata/adaptation-allowlist.json`.
4. Run `tools/check_import_scope.py`, the fixture-admission guard, the C++17
   `-Werror` build, and the fast focus selector.
5. Install to a fresh prefix and run both CMake and pkg-config consumer checks.
6. Compile the LibreCAD filter in system-package mode and verify that the
   command contains no `libraries/libdxfrw` include path.

## Integration evidence

- Standalone C++17/2.0.0 build, install, public-header smoke, and package
  relocation checks pass in fresh temporary prefixes.
- The pinned LibreCAD `rs_filterdxfrw.cpp` source overlay compiles against the
  staged package with `tools/check_librecad_system_package.py`; incomplete IDE
  include databases are repaired with source-directory includes, never with a
  bundled DXFRW include root.
- The seven-target fast lane, local DXF/DWG qualification vectors, six-version
  writer matrix, bounded hardening vectors, long fuzz lane, and focused
  ASan/UBSan run pass. Their reports deliberately keep broad support claims
  experimental where an independent semantic oracle is absent.
- The external 17-input/51-case corpus is advisory only. Its six named
  external-only follow-ups (J256, J260, J268, J284, J293, J295) remain
  `DEFERRED_EXTERNAL` and cannot satisfy a support claim.

## System-package LibreCAD mode

The downstream LibreCAD build should expose a system-libdxfrw option. When it
is enabled, it must skip creation and include propagation of the bundled
`libraries/libdxfrw` target, call `find_package(libdxfrw CONFIG REQUIRED)`, and
link the filter and parser checks to `libdxfrw::libdxfrw`. The standalone
checker is intentionally consumer-side: it validates the installed headers and
filter syntax without editing the dirty LibreCAD checkout.

## Fixture and licensing boundary

No external DWG/DXF payloads are part of this upgrade. Any protected corpus is
local/CI-only and is summarized by hashes and counts. Committed malformed,
fuzz, and round-trip vectors are generated in memory or from scratch and are
covered by `metadata/fixture-registry.json` plus
`tools/check_fixture_admission.py`. Imported source remains GPL-2.0-or-later;
the adaptation allowlist and existing `AUTHORS`/`NOTICE` provenance are kept
with the release.
