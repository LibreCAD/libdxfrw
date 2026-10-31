#ifndef LIBDXFRW_TEST_MATERIAL_DWG_TEST_HELPERS_H
#define LIBDXFRW_TEST_MATERIAL_DWG_TEST_HELPERS_H

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "drw_objects.h"
#include "intern/drw_textcodec.h"
#include "intern/dwgbufferw.h"

namespace material_dwg_test {

struct Frame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits{0u};
};

struct Options {
    // -1 emits the supported profile; 0..5 emits an unsupported procedural
    // source value in the corresponding fixed map slot.
    int proceduralMapIndex{-1};
    // A value below 16 stops the first map matrix early while retaining a
    // correctly bounded object frame. The incomplete body must remain opaque.
    unsigned diffuseTransformCount{16u};
    // false models a DWG common header with no extension-dictionary handle.
    bool hasExtensionDictionary{true};
};

class CodecProbe final : public DRW_Material {
public:
    using DRW_Material::parseDwg;
};

inline void appendBits(dwgBufferW& destination, const dwgBufferW& source) {
    for (std::uint64_t bit = 0u; bit < source.bitCount(); ++bit) {
        const auto value = static_cast<std::uint8_t>(
            (source.data()[static_cast<std::size_t>(bit / 8u)]
             >> (7u - static_cast<unsigned>(bit & 7u))) & 1u);
        destination.putBit(value);
    }
}

inline bool makeFrame(const Options& options, Frame& frame) {
    constexpr DRW::Version version = DRW::AC1024;
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    dwgBufferW body(&codec), strings(&codec), object(&codec), handles(&codec);

    strings.putVariableText(version, "ByBlock");
    strings.putVariableText(version, "generated AC1024 MATERIAL");

    const auto putColor = [&body](std::uint32_t value, double factor) {
        body.putRawChar8(1u); // explicit color value
        body.putBitDouble(factor);
        body.putBitLong(static_cast<std::int32_t>(value));
    };
    putColor(0x81234567u, 0.625);
    putColor(0x89ABCDEFu, 0.75);

    const auto putMap = [&body, &strings, &options](
            unsigned index, const char* name) {
        body.putBitDouble(0.125 + static_cast<double>(index) * 0.1);
        body.putRawChar8(static_cast<std::uint8_t>(1u + index % 4u));
        body.putRawChar8(static_cast<std::uint8_t>(1u + index % 3u));
        body.putRawChar8(static_cast<std::uint8_t>(index % 8u));
        for (unsigned component = 0u; component < 16u; ++component) {
            if (index == 0u && component >= options.diffuseTransformCount)
                return false;
            const double value = static_cast<double>(index * 32u + component)
                + 0.25;
            body.putBitDouble(value);
        }
        const std::uint8_t source = options.proceduralMapIndex
                == static_cast<int>(index)
            ? 2u : 1u;
        body.putRawChar8(source);
        if (source == 1u)
            strings.putVariableText(version, name);
        return body.isGood() && strings.isGood();
    };
    if (!putMap(0u, "diffuse-map.png"))
        goto finish_body;

    putColor(0xFEDCBA98u, 0.875);
    if (!putMap(1u, "specular-map.png"))
        goto finish_body;
    body.putBitDouble(0.6875); // specular gloss
    if (!putMap(2u, "reflection-map.png"))
        goto finish_body;
    body.putBitDouble(0.9375); // opacity percent
    if (!putMap(3u, "opacity-map.png")
        || !putMap(4u, "bump-map.png"))
        goto finish_body;
    body.putBitDouble(1.375); // refraction index
    if (!putMap(5u, "refraction-map.png"))
        goto finish_body;
    body.putBitDouble(0.1875); // translucence
    body.putBitDouble(0.0); // self illumination, outside selected support
    body.putBitDouble(0.3125); // reflectivity
    body.putBitLong(3); // illumination model
    body.putBitLong(5); // channel flags
    body.putBitLong(0); // nonprocedural material mode

finish_body:
    if (!body.isGood() || !strings.isGood())
        return false;
    body.alignToByte();
    strings.alignToByte();

    object.putObjType(version, DRW_Material::kDwgClassNum);
    dwgHandle own;
    own.ref = 0x97u;
    object.putHandle(own);
    object.putBitShort(0u); // no EED
    object.putBitLong(1); // one reactor
    object.putBit(options.hasExtensionDictionary ? 0u : 1u);
    appendBits(object, body);
    object.alignToByte();
    if (!strings.data().empty())
        object.putBytes(strings.data().data(), strings.size());
    for (unsigned bit = 0u; bit < 7u; ++bit)
        object.putBit(0u);
    const std::uint64_t stringBits = strings.size() * 8u + 7u;
    if (stringBits > std::numeric_limits<std::uint16_t>::max())
        return false;
    object.putRawShort16(static_cast<std::uint16_t>(stringBits));
    object.putBit(1u); // bounded AC1024 detached string stream
    object.alignToByte();

    const auto putHandle = [&handles](std::uint8_t code,
                                      std::uint32_t reference) {
        dwgHandle value;
        value.code = code;
        value.ref = reference;
        handles.putHandle(value);
    };
    putHandle(DRW::DwgSoftPointer, 0x72u); // owner dictionary
    putHandle(DRW::DwgSoftPointer, 0x72u); // selected reactor
    if (options.hasExtensionDictionary)
        putHandle(DRW::DwgHardOwnership, 0x110u); // extension dictionary
    handles.alignToByte();
    if (!object.isGood() || !handles.isGood()
        || handles.bitCount() > std::numeric_limits<std::uint32_t>::max())
        return false;
    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    object.putBytes(handles.data().data(), handles.size());
    if (!object.isGood())
        return false;
    frame.bytes = object.data();
    return true;
}

inline bool parse(const Frame& frame, CodecProbe& parsed) {
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);
    dwgBuffer input(const_cast<std::uint8_t*>(frame.bytes.data()),
                    frame.bytes.size(), &codec);
    return parsed.parseDwg(DRW::AC1024, &input, frame.handleBits)
        && input.isGood();
}

} // namespace material_dwg_test

#endif
