libdxfrw ![Build status](https://api.travis-ci.org/LibreCAD/libdxfrw.svg?branch=master)
==========

libdxfrw 2.x is a C++17 library that reads and writes DXF files in ASCII and
binary form and reads and writes the pinned LibreCAD DWG version routes. Format
rows remain explicitly supported, experimental, or unsupported according to
the evidence-led support matrix; recognition alone is not a support claim.
It is licensed under the terms of the GNU General Public License version 2 (or at you option
any later version).


libdxfrw was created by [LibreCAD](https://github.com/LibreCAD/LibreCAD) contributors in the process of making LibreCAD.
As the original code at [SourceForge](https://sourceforge.net/projects/libdxfrw) was no longer supported by the orignal authors, this repo has become its successor.

If you are looking for historical information about the project, it's still there:
http://sourceforge.net/projects/libdxfrw


Please note:
----------
When you clone or download this project to build [LibreCAD_3](https://github.com/LibreCAD/LibreCAD_3) use the branch **LibreCAD_3**. The master or other branches may have incompatible interface definitions which are not yet implemented in LibreCAD_3!

Building and installing the library
==========

CMake is the supported 2.x build. It requires CMake 3.10 or newer and C++17:

```
cmake -S . -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DLIBDXFRW_BUILD_TESTS=ON -DLIBDXFRW_BUILD_DOC=OFF
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix /tmp/libdxfrw-install
```

The historical Autotools, MinGW, and Conan recipes are retained for reference
but are deprecated for the 2.x convergence until they consume the canonical
source manifest and have a maintained C++17/CI lane. See
`docs/UPGRADE_SUPPORT.md` for the support and release policy.

Debug version
----------

```
mkdir build
cd build
cmake ..
make 
sudo make install
```

Non-debug version
----------

```
mkdir release
cd release
cmake -DCMAKE_BUILD_TYPE=Release ..
make 
sudo make install
```

Ubuntu/Mint Folks
----------

```
mkdir release
cd release
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr .. && make all
make 
sudo make install
```


Example usage of the library
==========

See how we use it in LibreCAD V3 : https://github.com/LibreCAD/LibreCAD_3/tree/master/persistence/libdxfrw
