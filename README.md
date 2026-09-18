libdxfrw ![Build status](https://api.travis-ci.org/LibreCAD/libdxfrw.svg?branch=master)
==========

libdxfrw is LibreCAD's C++17 library for reading and writing ASCII and binary
DXF, with DWG reader/writer and raw-carrier preservation paths. The 2.0.0
release is an ABI boundary: it keeps the historical callback surface usable,
but consumers should rebuild against the installed package.

The project is GPL-2.0-or-later. It was created by [LibreCAD](https://github.com/LibreCAD/LibreCAD)
contributors; historical project information remains at
[SourceForge](https://sourceforge.net/projects/libdxfrw).

## Build and install

CMake is the supported 2.x build (CMake 3.28 or newer, C++17). The historical
Autotools, MinGW, and Conan recipes are retained for reference and are
deprecated until they consume the canonical source manifest and have a
maintained C++17/CI lane.

```sh
cmake -S . -B build
cmake --build build
cmake --install build --prefix "$PWD/stage"
```

The default build produces `dxfrw` and `dwg2dxf`. Use
`-DLIBDXFRW_BUILD_TESTS=ON` for the dependency-free regression targets and
`-DLIBDXFRW_BUILD_LONG_FUZZ=ON` for the opt-in, in-memory long fuzz lane. The
documented fast selector is:

`LIBDXFRW_INSTALL_DEV` (default `ON`) controls whether `cmake --install`
deposits the development files -- the headers, `libdxfrw.pc`, and the CMake
package -- alongside the library. Packagers want the default. An application
that vendors libdxfrw with `add_subdirectory` and only needs the library in its
own prefix can set it `OFF`, either on the command line or with
`set(LIBDXFRW_INSTALL_DEV OFF)` before `add_subdirectory`:

```
cmake -S . -B build -DLIBDXFRW_INSTALL_DEV=OFF
```

The historical Autotools, MinGW, and Conan recipes are retained for reference
but are deprecated for the 2.x convergence until they consume the canonical
source manifest and have a maintained C++17/CI lane. See
`docs/UPGRADE_SUPPORT.md` for the support and release policy.

```sh
python3 tools/run_fast_focus.py --build-dir build
```

Installed consumers can use either `find_package(libdxfrw CONFIG REQUIRED)` and
`libdxfrw::libdxfrw`, or `pkg-config --cflags --libs libdxfrw`. The package is
relocatable and selects compatible package versions with `SameMajorVersion`.

## Support boundary

| Capability | Status | Qualification boundary |
| --- | --- | --- |
| ASCII/binary DXF baseline read and write | supported | Public API and fast regression lane |
| DXF raw sections, opaque objects, ACIS/SAB and proxy carriers | experimental | Local preservation vectors pass; independent semantic oracle still required |
| DWG read across the dispatched AC versions | experimental | Reader and section safety matrices pass; eligible semantic fixtures remain required |
| DWG writing and versioned feature families | experimental | Local six-version round-trips pass; self-read is not an independent oracle |
| Derived rendering or CAD application policy | unsupported | Owned by the consumer, not this library |

The six external-only follow-ups J256, J260, J268, J284, J293, and J295 are
still `DEFERRED_EXTERNAL`. Their evidence is advisory and cannot promote any
support claim. External DWG/DXF payloads are not committed; tests that need
them use protected/local corpus paths, while committed malformed and fuzz
vectors are generated in memory or from scratch.

LibreCAD can compile its filter against this package with the system-package
mode described in [LIBRECAD_SYNC.md](LIBRECAD_SYNC.md). The filter must not
propagate the bundled `libraries/libdxfrw` include path when that mode is
enabled. The standalone-safe DXF classifier is the default; consumers that
need historical LibreCAD group-code behavior must opt in explicitly through
`dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy`.

## Development and provenance

The source import, adaptation allowlist, pinned LibreCAD revision, package
consumer checks, and fixture-admission policy are recorded in
[LIBRECAD_SYNC.md](LIBRECAD_SYNC.md) and `metadata/`. Run
`python3 tools/update_upgrade_plan.py --check` before committing a plan slice.
Every committed slice carries a `Plan-Slice` trailer and records its focused
evidence. See `tests/samples/README.md` for the policy governing local and
external sample files.

Example usage remains in the `dwg2dxf/` CLI and in LibreCAD's filter adapter.
