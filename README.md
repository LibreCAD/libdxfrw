# libdxfrw-web

This is a webassembly version of libdxfrw which can be used in browser and Node.js environments. It supports the following features.

- Read/write DXF files (in both formats, ascii and binary form).
- Read DWG files from AutoCAD R14 to AutoCAD 2020.
- Convert DWG file to DXF file.

You can play with it through this [live demo](https://mlight-lee.github.io/libdxfrw/).

## Build WebAssembly

Download and install emscripten according to [this doc](https://emscripten.org/docs/getting_started/downloads.html).

### Use auotmake

```
autoconf
automake --add-missing --force-missing
mkdir build
cd build
emconfigure ../configure
cd src
emmake make
emcc -O2 -lembind *.o intern/*.o -o libdxfrw.js -s ALLOW_MEMORY_GROWTH=1 --emit-tsd libdxfrw.d.ts
```

If you want to debug WebAssembly in Chrome DevTools. Please compile your application with DWARF debug information included. Run the latest Emscripten compiler and pass it the -gsource-map flag. For example:

```
emcc -gsource-map -lembind *.o intern/*.o -o libdxfrw.js -s ALLOW_MEMORY_GROWTH=1 --emit-tsd libdxfrw.d.ts
```

### Use CMake

```
mkdir build
cd build
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
emmake make
emcc -O2 -lembind CMakeFiles/dxfrw.dir/src/*.o CMakeFiles/dxfrw.dir/src/intern/*.o -o libdxfrw.js -s ALLOW_MEMORY_GROWTH=1 --emit-tsd libdxfrw.d.ts
```

## Usage

Please refer to example code in [index.html](./dist/index.html). You can use the following command to install it as one of your package dependencies. 

```
npm install @mlightcad/libdxfrw-web
```

## How to Debug?

- [emscripten Debugging](https://emscripten.org/docs/porting/Debugging.html)
- [Debugging WebAssembly with modern tools](https://developer.chrome.com/blog/wasm-debugging-2020/)
