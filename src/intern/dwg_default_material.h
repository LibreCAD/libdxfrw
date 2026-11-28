// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef LIBDXFRW_DWG_DEFAULT_MATERIAL_H
#define LIBDXFRW_DWG_DEFAULT_MATERIAL_H

#include "dwgbufferw.h"

// Private source-profile building block. This does not register a class,
// allocate a handle, write an object frame, or enable any writer behavior.
// Its caller must establish the complete material dictionary ownership graph.
namespace drwDwgDefaultMaterial {

enum class Kind { ByBlock, ByLayer, Global };

// Exact nonprocedural default profiles witnessed in ODA-produced AC1021,
// AC1024, and AC1027 drawings; no other version or material profile is
// admitted. AC1027's witnessed defaults use inherited diffuse color, a scene
// refraction map (and therefore no detached filename), and channel flags 127.
// On success the two destination buffers are replaced with the body and
// detached strings.
// The body deliberately excludes the common preamble and handle stream.
inline bool build(DRW::Version version, Kind kind,
                  dwgBufferW& bodyOut, dwgBufferW& stringsOut) {
    const char* name = nullptr;
    switch (kind) {
    case Kind::ByBlock: name = "ByBlock"; break;
    case Kind::ByLayer: name = "ByLayer"; break;
    case Kind::Global: name = "Global"; break;
    }
    if ((version != DRW::AC1021 && version != DRW::AC1024 && version != DRW::AC1027)
        || name == nullptr || &bodyOut == &stringsOut)
        return false;
    dwgBufferW body, strings;
    std::uint16_t length = 0;
    while (name[length] != '\0') ++length;
    // These three observed ASCII names have a TU length excluding any null.
    strings.putBitShort(length);
    for (std::uint16_t i = 0; i < length; ++i) {
        strings.putRawChar8(static_cast<std::uint8_t>(name[i]));
        strings.putRawChar8(0);
    }
    strings.putBitShort(0); // description

    body.putRawChar8(0); // ambient color: inherited
    body.putBitDouble(1.0);
    const bool r2013Profile = version == DRW::AC1027;
    body.putRawChar8(r2013Profile ? 0u : 1u); // diffuse color method
    body.putBitDouble(1.0);
    if (!r2013Profile)
        body.putBitLong(-1023410177); // explicit default white
    const auto putDefaultMap = [&](bool sceneSource = false) {
        body.putBitDouble(1.0);
        body.putRawChar8(1); // projection
        body.putRawChar8(1); // tiling
        body.putRawChar8(1); // autotransform
        for (unsigned i = 0; i < 16; ++i)
            body.putBitDouble(i % 5 == 0 ? 1.0 : 0.0);
        body.putRawChar8(sceneSource ? 0u : 1u); // scene/file source
        if (!sceneSource)
            strings.putBitShort(0); // empty file name
    };
    putDefaultMap(); // diffuse
    body.putRawChar8(0); // specular color: inherited
    body.putBitDouble(1.0);
    putDefaultMap(); // specular
    body.putBitDouble(0.5); // gloss
    putDefaultMap(); // reflection
    body.putBitDouble(1.0); // opacity
    putDefaultMap(); // opacity
    putDefaultMap(); // bump
    body.putBitDouble(1.0); // refraction index
    putDefaultMap(r2013Profile); // refraction
    body.putBitDouble(0.0); // translucence
    body.putBitDouble(0.0); // self illumination
    body.putBitDouble(0.0); // reflectivity
    body.putBitLong(0); // illumination model
    body.putBitLong(r2013Profile ? 127 : 63); // channel flags
    body.putBitLong(0); // nonprocedural mode
    if (!body.isGood() || !strings.isGood()) return false;
    bodyOut.swap(body);
    stringsOut.swap(strings);
    return true;
}
} // namespace drwDwgDefaultMaterial

#endif
