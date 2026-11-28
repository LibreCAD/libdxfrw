#ifndef LIBDXFRW_LAYER_TEST_HELPERS_H
#define LIBDXFRW_LAYER_TEST_HELPERS_H

#include "appid_test_helpers.h"

namespace layer_test {
class Probe final : public DRW_Layer {
public:
    using DRW_Layer::parseDwg;
};

// Literal scratch LAYER frames (ODA20.4.54/2.11), not a writer roundtrip or
// downloaded fixture. Name and optional CMC-name fields have separate owners.
inline appid_test::Frame frame(DRW::Version version, const std::string& rawName,
        std::uint32_t ownHandle = 0x234u, std::uint16_t objectType = 51u,
        const std::string& rawColorName = {}) {
    dwgBufferW body, strings, handles;
    body.putObjType(version, objectType);
    const auto sizeOffset = body.bitCount();
    if (version < DRW::AC1024) body.putRawLong32(0u);
    dwgHandle own; own.ref = ownHandle; body.putHandle(own);
    body.putBitShort(0u); body.putBitLong(0);
    if (version >= DRW::AC1018) body.putBit(1u);
    if (version > DRW::AC1024) body.putBit(0u);
    dwgBufferW& text = version > DRW::AC1018 ? strings : body;
    const auto putText = [&](const std::string& bytes) {
        text.putBitShort(static_cast<std::uint16_t>(version > DRW::AC1018 ? bytes.size() / 2u : bytes.size()));
        text.putBytes(reinterpret_cast<const std::uint8_t*>(bytes.data()), bytes.size());
    };
    putText(rawName);
    if (version > DRW::AC1018) body.putBitShort(0u);
    else { body.putBit(0u); body.putBitShort(0u); body.putBit(0u); }
    body.putSBitShort(0x3F0); // plotting enabled, default lineweight
    if (version >= DRW::AC1018) {
        body.putBitShort(0u);
        body.putBitLong(static_cast<std::int32_t>(rawColorName.empty() ? 0xC3000007u : 0xC2112233u));
        body.putRawChar8(rawColorName.empty() ? 0u : 1u);
        if (!rawColorName.empty()) putText(rawColorName);
    } else body.putBitShort(7u);
    dwgHandle owner; owner.code = 4u; owner.ref = 2u; handles.putHandle(owner);
    dwgHandle null;
    if (version < DRW::AC1018) handles.putHandle(null);
    handles.putHandle(null); // xref
    handles.putHandle(null); // plot style
    if (version > DRW::AC1018) handles.putHandle(null); // material
    dwgHandle line; line.code = 5u; line.ref = 0x14u; handles.putHandle(line);
    if (version >= DRW::AC1027) handles.putHandle(null);
    if (version > DRW::AC1018) {
        strings.alignToByte();
        const auto padding = (7u - (body.bitCount() & 7u)) & 7u;
        appid_test::appendBits(body, strings);
        for (std::uint64_t bit = 0u; bit < padding; ++bit) body.putBit(0u);
        body.putRawShort16(static_cast<std::uint16_t>(strings.bitCount() + padding));
        body.putBit(1u);
    }
    const auto handleStart = body.bitCount();
    if (version < DRW::AC1024) body.patchRawLong32AtBit(sizeOffset, static_cast<std::uint32_t>(handleStart));
    appid_test::appendBits(body, handles);
    body.alignToByte();
    return {body.data(), static_cast<std::uint32_t>(body.bitCount() - handleStart)};
}

inline bool parse(DRW::Version version, const std::string& rawName,
        const char* page, DRW_Layer& result, std::uint32_t handle = 0x234u,
        std::uint16_t type = 51u, bool byteLength = false) {
    const auto encoded = frame(version, rawName, handle, type);
    DRW_TextCodec codec; codec.setVersion(version, false); codec.setByteCodePage(page);
    if (version <= DRW::AC1018) codec.setCodePage(page, false);
    dwgBuffer body(encoded.bytes.data(), encoded.bytes.size(), &codec);
    body.setVariableTextByteLength(byteLength);
    Probe parsed;
    if (!parsed.parseDwg(version, &body, encoded.handleBits) || !body.isGood()) return false;
    result = parsed;
    return true;
}
} // namespace layer_test
#endif
