# libdxfrw-web

This is a WebAssembly version of libdxfrw. It can read/write DXF files (in both formats, ascii and binary form) and read DWG files from AutoCAD R14 to AutoCAD 2020 in browser and Node.js environment.

## Build WebAssembly

```
autoconf
automake --add-missing --force-missing
mkdir build
cd build
emconfigure ../configure
cd src
emmake make
emcc -O2 -lembind *.o intern/*.o -o libdxfrw.js --emit-tsd libdxfrw.d.ts
```

If you want to debug WebAssembly in Chrome DevTools. Please compile your application with DWARF debug information included. Run the latest Emscripten compiler and pass it the -gsource-map flag. For example:

```
emcc -gsource-map -lembind *.o intern/*.o -o libdxfrw.js --emit-tsd libdxfrw.d.ts
```

## Usage

Please refer to example code in [index.html](./dist/index.html).

## How to Debug?

- [emscripten Debugging](https://emscripten.org/docs/porting/Debugging.html)
- [Debugging WebAssembly with modern tools](https://developer.chrome.com/blog/wasm-debugging-2020/)
