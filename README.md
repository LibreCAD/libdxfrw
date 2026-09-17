libdxfrw [![Build status](https://github.com/LibreCAD/libdxfrw/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/LibreCAD/libdxfrw/actions/workflows/build.yml)
==========

libdxfrw 2.x is a C++17 library for reading and writing DXF files in ASCII and
binary form and for reading DWG files across the LibreCAD interoperability
routes. The merged [PR #93](https://github.com/LibreCAD/libdxfrw/pull/93)
adds the standalone DWG/DXF implementation aligned with the bundled code in
current LibreCAD master.

The DWG reader routes are AC1012, AC1014, AC1015, AC1018, AC1021, AC1024,
AC1027, and AC1032. The DWG writer routes are AC1015, AC1018, AC1021, AC1024,
AC1027, and AC1032. The DXF reader and writer handle ASCII and binary input and
output. Individual rows remain explicitly supported, experimental, or
unsupported according to the evidence-led support matrix; route recognition
alone is not a format-support claim.

libdxfrw is licensed under the terms of the GNU General Public License version 2
(or, at your option, any later version).


libdxfrw was created by [LibreCAD](https://github.com/LibreCAD/LibreCAD)
contributors in the process of making LibreCAD. As the original code at
[SourceForge](https://sourceforge.net/projects/libdxfrw) was no longer
maintained by its original authors, this repository has become its successor.

If you are looking for historical information about the project, it's still there:
http://sourceforge.net/projects/libdxfrw


Please note:
----------
When you clone or download this project to build
[LibreCAD_3](https://github.com/LibreCAD/LibreCAD_3), use the branch
**LibreCAD_3**. The `master` branch targets the current LibreCAD integration and
may expose interface definitions that LibreCAD_3 does not yet implement.

Building and installing the library
==========

CMake is the supported 2.x build. It requires CMake 3.10 or newer and C++17:
When `LIBDXFRW_BUILD_TESTS=ON`, the test and qualification checks additionally
require Python 3.10 or newer.

```
cmake -S . -B build \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_SHARED_LIBS=OFF \
  -DLIBDXFRW_BUILD_TESTS=ON \
  -DLIBDXFRW_BUILD_DWG2DXF=ON \
  -DLIBDXFRW_BUILD_DOC=OFF
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --install build --prefix /tmp/libdxfrw-install
```

The historical Autotools, MinGW, and Conan recipes are retained for reference
but are deprecated for the 2.x convergence until they consume the canonical
source manifest and have a maintained C++17/CI lane. See
`docs/UPGRADE_SUPPORT.md` for the support and release policy.

The optional `dwg2dxf` command-line converter is built by the configuration
above:

```
build/dwg2dxf/dwg2dxf input.dwg -y -v2010 output.dxf
```


Example usage of the library
==========

See how we use it in LibreCAD V3 : https://github.com/LibreCAD/LibreCAD_3/tree/master/persistence/libdxfrw
