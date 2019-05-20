libdxfrw ![Build status](https://api.travis-ci.org/LibreCAD/libdxfrw.svg?branch=master)
==========

libdxfrw is a free C++ library to read and write DXF files in both formats, ascii and binary form.
 It is licensed under the terms of the GNU General Public License version 2 (or at you option
any later version).


If you are looking for general information about the project, check our website:
http://sourceforge.net/projects/libdxfrw


WARNING: This project is a fork to add a CMakeLists.txt for some of our downstream project's in LibreCAD to make compiling and following this project easier.
==========

Building and installing the library
==========
```
mkdir build
cd build
cmake ..
make 
sudo make install
```

For non-debug version:
==========

```
mkdir release
cd release
cmake -DCMAKE_BUILD_TYPE=Release ..
make 
sudo make install
```

== UBUNTU/Mint Folks ==

```
mkdir release
cd release
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=/usr .. && make all
make 
sudo make install
```


== Example usage of the library ==

See how we use it in LibreCAD V3 : https://github.com/LibreCAD/LibreCAD_3/tree/master/persistence/libdxfrw
