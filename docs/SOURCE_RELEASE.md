# Building a source release

CMake and Autotools use the same library and converter source inventories.
The installed public header set includes its four required internal headers.
Both builds require C++17 and use ABI major 2. CMake also installs its exported
package; Autotools installs the headers and `libdxfrw.pc` for pkg-config.
The narrow GCC warning policy for the frozen entity parser matches CMake;
`-Werror` remains enabled throughout the build.

From a Git checkout, update the canonical `libdxfrw_sources.cmake` and
`dwg2dxf/source-files.cmake` when adding implementation files, then run:

```sh
python3 tools/generate_build_inventory.py --write
python3 tools/generate_build_inventory.py --check
./autogen.sh
mkdir build-autotools
cd build-autotools
../configure --prefix="$PWD/prefix"
make -j2
make install
COPYFILE_DISABLE=1 make dist
```

From an extracted release, run `./configure`, `make`, and `make install`;
Python, Git, Autoconf and Automake are not needed for that build. Use
`--disable-shared` for a static-only build or `--disable-static` for shared-only.
The configured install prefix and standard Autotools directory overrides apply.
Relocation of an Autotools install is not promised; the CMake package retains
its documented relocation behavior. Existing MinGW/Visual Studio reference
files do not define the current production source inventory.

The archive also provides the CMake build and test sources. Tests default OFF;
enable them with `-DLIBDXFRW_BUILD_TESTS=ON` and Python 3.10 or newer. Native CAD,
independent kernel and Git/pinned-checkout qualification still need their
documented environments; archive inclusion does not supply those prerequisites.

The committed release file list contains only tracked project files and the
explicitly managed inventory files. Untracked local drawings, editor state and
build directories are excluded. The active append-only parity execution plan
and historical research receipts are omitted to avoid an archive/hash cycle;
the two research reports used by CMake tests are included. No new drawing bytes
or derivatives are admitted by generating a release.

Maintainers can check every listed archive file against its source bytes:

```sh
python3 tools/generate_build_inventory.py --check --archive path/to/libdxfrw-2.0.0.tar.gz
```

GNU `make distcheck` additionally exercises a read-only-source VPATH build,
installation, uninstall, cleanup and regeneration. This is separate from native
Linux/Windows CAD consumer and DWG/DXF semantic support qualification.
