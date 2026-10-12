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

CMake is the supported 2.x build (CMake 3.28 or newer, C++17). The Conan 2
recipe consumes the same canonical source manifest and disables tests and
documentation for a small package build. Autotools and MinGW remain reference
builds.

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

For Conan 2, create the package directly from a checkout:

`conan create . --name libdxfrw --version 2.0.0 -o '&:shared=False'`

See `docs/UPGRADE_SUPPORT.md` for the support and release policy.

```sh
python3 tools/run_fast_focus.py --build-dir build
```

### Using `dwg2dxf` from FreeCAD

The installed `dwg2dxf` executable can serve as FreeCAD Draft's external
DWG-to-DXF converter for the `Draft.importDWG.open()` and
`Draft.importDWG.insert()` import workflows. This project does not provide
FreeCAD's paired `dxf2dwg` program, so it does not provide LibreDWG-based DWG
export.

The end-to-end profile qualified here is FreeCAD 1.1.3 (revision
`145529fe741292ff0b3977a01195bf0247425794`) on macOS 27 arm64 with the C++ DXF
importer and PATH discovery. Both `open()` and `insert()` were verified with an
installed converter and a generated AC1015 nonzero-Z LINE control in both
FreeCAD's Automatic (`DWGConversion=0`) and LibreDWG-only
(`DWGConversion=1`) modes. In the Automatic-mode control, the installed
`dwg2dxf` succeeded first and no ODA/QCAD fallback ran; fallbacks may still be
attempted after a failed conversion. This narrow result does not imply broad
entity support; native Linux and Windows FreeCAD profiles, including Windows
executable discovery, remain unqualified.

Add the installed bindir (usually `bin`) to `PATH` before launching FreeCAD, or
set FreeCAD's DWG converter path to the full installed executable path. Select
FreeCAD's LibreDWG-compatible converter option when you want to attribute an
import specifically to this executable; in automatic mode FreeCAD may try ODA
or QCAD after a failed conversion. If multiple programs named `dwg2dxf` are on
`PATH`, the first match is used.

FreeCAD shares its `TeighaFileConverter` preference between DWG import and
export. When that preference names `dwg2dxf`, FreeCAD derives the sibling
`dxf2dwg` path for export; if the sibling is absent, the current resolver does
not fall back to `PATH`. For import-only use that should preserve an existing
export setup, prefer PATH discovery and avoid setting the shared preference to
either LibreDWG executable. If you configure this `dwg2dxf` path directly,
route export through a separately available converter that actually supports
`dxf2dwg`. See [FreeCAD's converter resolver](https://github.com/FreeCAD/FreeCAD/blob/main/src/Mod/Draft/importDWG.py#L129-L165)
and [conversion/fallback logic](https://github.com/FreeCAD/FreeCAD/blob/main/src/Mod/Draft/importDWG.py#L244-L335).

FreeCAD invokes the converter as `dwg2dxf <input.dwg> -o <output.dxf>`; this
form is noninteractive, writes ASCII DXF by default, and retains a supported
source revision unless an explicit output version is supplied. The converter
produces DXF for FreeCAD's own importer: it does not extend that importer's
entity or 3D geometry support. For example, a DXF entity may be preserved
correctly by `dwg2dxf` while FreeCAD reports it as unsupported.

Installed consumers can use either `find_package(libdxfrw CONFIG REQUIRED)` and
`libdxfrw::libdxfrw`, or `pkg-config --cflags --libs libdxfrw`. The package is
relocatable and selects compatible package versions with `SameMajorVersion`.
Packagers that need module-mode discovery can install the same development
files and append the installed `${libdir}/cmake/libdxfrw` directory to
`CMAKE_MODULE_PATH`; `find_package(Libdxfrw MODULE REQUIRED)` then provides the
same `libdxfrw::libdxfrw` target.

## Support boundary

| Capability | Status | Qualification boundary |
| --- | --- | --- |
| ASCII/binary DXF baseline read and write | supported | Public API and fast regression lane |
| DXF raw sections, opaque objects, ACIS/SAB and proxy carriers | experimental | Local preservation vectors pass; independent semantic oracle still required |
| DWG read across the dispatched AC versions | experimental | Reader and section safety matrices pass; eligible semantic fixtures remain required |
| DWG writing and versioned feature families | experimental | Local six-version round-trips pass; self-read is not an independent oracle |
| Existing 2D consumer compatibility | checked narrowly | `lc3_compat_check` verifies the LibreCAD-style source callback surface, and one generated DXF vector guards legacy `ext=true` LWPOLYLINE behavior; not a binary-ABI or UI guarantee |
| 3D consumer typed-data access | experimental DXF subset; exact opaque SAB carriers in four AC1024 and one AC1027 DWG samples | AC1027 ASCII/binary callback probe covers listed DXF fields/families; local DWG samples establish only exact SAB byte extraction/association for 129 AC1024 inline solids and one AC1027 handle-linked solid. No general DWG modeler semantics, renderer, or geometry kernel is included |
| Derived rendering or CAD application policy | unsupported | Owned by the consumer, not this library |

The 3D consumer lane exposes typed XYZ, OCS/elevation, topology, placement, and
selected surface-reference data through the opt-in `ext=false` path, without
forcing the existing 2D projection policy into library callbacks or changing
the existing `ext=true` mode. This is experimental data delivery, not a
general 3D format-support, rendering, or editing claim. Existing 2D consumers
such as LibreCAD retain their source-compatible callback contract. See the
[3D consumer contract](docs/3D_CONSUMER_CONTRACT.md) and [3D format/version
status matrix](docs/3D_SUPPORT_STATUS.md) for the exact tested scope and
remaining evidence limits.

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
