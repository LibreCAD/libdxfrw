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
emcc -O2 -lembind *.o intern/*.o -o libdxfrw.js -s ALLOW_MEMORY_GROWTH=1 -s EXPORT_ES6=1 -s MODULARIZE=1 -s EXPORT_NAME="createModule" --emit-tsd libdxfrw.d.ts
```

If you want to debug WebAssembly in Chrome DevTools. Please compile your application with DWARF debug information included. Run the latest Emscripten compiler and pass it the -g flag. For example:

```
emcc -g -lembind *.o intern/*.o -o libdxfrw.js -s ALLOW_MEMORY_GROWTH=1 -s EXPORT_ES6=1 -s MODULARIZE=1 -s EXPORT_NAME="createModule" --emit-tsd libdxfrw.d.ts
```

### Use CMake

```
mkdir build
cd build
emcmake cmake .. -DCMAKE_BUILD_TYPE=Release
emmake make
emcc -O2 -lembind CMakeFiles/dxfrw.dir/src/*.o CMakeFiles/dxfrw.dir/src/intern/*.o -o libdxfrw.js -s ALLOW_MEMORY_GROWTH=1 -s EXPORT_ES6=1 -s MODULARIZE=1 -s EXPORT_NAME="createModule" --emit-tsd libdxfrw.d.ts
```

## Usage

Please refer to example code in [index.html](./dist/index.html). You can use the following command to install it as one of your package dependencies. 

```
npm install @mlightcad/libdxfrw-web
```

### How to iterate data represented by std::vector?

Emscripten doesn't convert std::vector to JavaScript array. So std::vector are converted to one seperated class. For example, std::vector<double> are converted to the following class.

```TypeScript
export interface DRW_DoubleList extends ClassHandle {
  push_back(_0: number): void;
  resize(_0: number, _1: number): void;
  size(): number;
  get(_0: number): number | undefined;
  set(_0: number, _1: number): boolean;
}
```

So it means you can't use JavaScript iterator to iterate them. For example, you need to iterate vertexes of one polyline as follows.

```JavaScript
const vertexes = polyline.getVertexList();
for (let index = 0, size = vertexes.size(); index < size; ++index) {
  const vertex = vertexes.get(index);
  ......
}
```

### Readonly list

I can't find one good way to expose property defined by one std container with `std::shared_ptr`. So one new public method is added for expose data defined by those properties. Values returned by those methods is are the copy of property data instead of the referernce. So it means that data stored in the property will not change after you changed data returned by those methods. 

```JavaScript
class DRW_Hatch : public DRW_Point {
  ......
public
  // Newly added method to expose data defined by property 'looplist'
  std::vector<DRW_HatchLoop*> getLoopList() const {
    std::vector<DRW_HatchLoop*> loopList;
    int loopnum = looplist.size();
    for (int i = 0; i< loopnum; i++){
      DRW_HatchLoop* loop = looplist.at(i).get();
      loopList.push_back(loop);
    }
    return loopList;
  }
  std::vector<std::shared_ptr<DRW_HatchLoop>> looplist;  /*!< polyline list */
  ......
}
```

The following methods use this pattern.

```C++
DRW_LWPolyline::getVertexList
DRW_Polyline::getVertexList
DRW_Spline::getControlList
DRW_Spline::getFitList
DRW_HatchLoop::getLoopList
DRW_Hatch::getLoopList
```

## How to Debug?

- [emscripten Debugging](https://emscripten.org/docs/porting/Debugging.html)
- [Debugging WebAssembly with modern tools](https://developer.chrome.com/blog/wasm-debugging-2020/)
