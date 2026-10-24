#ifndef LIBDXFRW_APPID_TEST_HELPERS_H
#define LIBDXFRW_APPID_TEST_HELPERS_H

#include "drw_objects.h"
#include "intern/drw_textcodec.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"

namespace appid_test {

class Probe final : public DRW_AppId {
public:
    using DRW_AppId::parseDwg;
};

struct Frame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits{0u};
};

inline void appendBits(dwgBufferW& destination, const dwgBufferW& source) {
    for (std::uint64_t bit = 0u; bit < source.bitCount(); ++bit)
        destination.putBit(static_cast<std::uint8_t>(
            (source.data()[static_cast<std::size_t>(bit / 8u)] >> (7u - bit % 8u)) & 1u));
}

// Literal from-scratch APPID field/frame controls. ODA20.4.66 fixes this
// existing table type at67; this is not dynamic-class dispatch or a fixture.
inline Frame frame(DRW::Version version, const std::string& rawName,
                   std::uint32_t ownHandle = 0x123u,
                   const std::vector<std::uint8_t>& eed = {},
                   std::uint16_t objectType = 67u) {
    dwgBufferW body, strings, handles;
    body.putObjType(version, objectType);
    const auto sizeOffset = body.bitCount();
    if (version < DRW::AC1024) body.putRawLong32(0u);
    dwgHandle own; own.ref = ownHandle;
    body.putHandle(own);
    if (!eed.empty()) {
        body.putBitShort(static_cast<std::uint16_t>(eed.size()));
        dwgHandle app; app.code = 5u; app.ref = 0x456u;
        body.putHandle(app);
        body.putBytes(eed.data(), eed.size());
    }
    body.putBitShort(0u);
    body.putBitLong(0);
    if (version >= DRW::AC1018) body.putBit(1u);
    if (version > DRW::AC1024) body.putBit(0u);
    dwgBufferW& text = version > DRW::AC1018 ? strings : body;
    text.putBitShort(static_cast<std::uint16_t>(
        version > DRW::AC1018 ? rawName.size() / 2u : rawName.size()));
    text.putBytes(reinterpret_cast<const std::uint8_t*>(rawName.data()), rawName.size());
    if (version > DRW::AC1018) body.putBitShort(0u);
    else { body.putBit(0u); body.putBitShort(0u); body.putBit(0u); }
    body.putRawChar8(0u);
    dwgHandle owner; owner.code = 4u; owner.ref = 9u;
    handles.putHandle(owner);
    dwgHandle null;
    if (version < DRW::AC1018) handles.putHandle(null);
    handles.putHandle(null); // xref block
    if (version > DRW::AC1018) {
        strings.alignToByte();
        const auto padding = (7u - (body.bitCount() & 7u)) & 7u;
        appendBits(body, strings);
        for (std::uint64_t bit = 0u; bit < padding; ++bit) body.putBit(0u);
        body.putRawShort16(static_cast<std::uint16_t>(strings.bitCount() + padding));
        body.putBit(1u);
    }
    const auto handleStart = body.bitCount();
    if (version < DRW::AC1024)
        body.patchRawLong32AtBit(sizeOffset, static_cast<std::uint32_t>(handleStart));
    appendBits(body, handles);
    body.alignToByte();
    return {body.data(), static_cast<std::uint32_t>(body.bitCount() - handleStart)};
}

inline std::string unicodeBytes(const std::string& ascii) {
    std::string bytes;
    for (const auto value : ascii) { bytes.push_back(value); bytes.push_back('\0'); }
    return bytes;
}

inline bool parse(DRW::Version version, const std::string& rawName,
                  const char* page, DRW_AppId& result,
                  std::uint32_t ownHandle = 0x123u,
                  std::uint16_t objectType = 67u,
                  bool byteLength = false) {
    const auto encoded = frame(version, rawName, ownHandle, {}, objectType);
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    codec.setByteCodePage(page);
    if (version <= DRW::AC1018) codec.setCodePage(page, false);
    dwgBuffer body(encoded.bytes.data(), encoded.bytes.size(), &codec);
    body.setVariableTextByteLength(byteLength);
    Probe parsed;
    if (!parsed.parseDwg(version, &body, encoded.handleBits) || !body.isGood())
        return false;
    result = parsed;
    return true;
}

} // namespace appid_test
#endif
