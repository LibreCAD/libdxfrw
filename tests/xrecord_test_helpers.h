#ifndef LIBDXFRW_XRECORD_TEST_HELPERS_H
#define LIBDXFRW_XRECORD_TEST_HELPERS_H

#include <cstdint>
#include <string>
#include <vector>

#include "drw_objects.h"
#include "intern/drw_textcodec.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"

namespace xrecord_test {

class CodecProbe final : public DRW_XRecord {
public:
    using DRW_XRecord::encodeDwg;
    using DRW_XRecord::parseDwg;
};

inline std::vector<std::uint8_t> data(DRW::Version version,
                                     bool opaque = true) {
    dwgBufferW payload;
    payload.putRawShort16(1);
    payload.putRawShort16(4);
    if (version <= DRW::AC1018)
        payload.putRawChar8(0);
    for (const char value : std::string("seed")) {
        payload.putRawChar8(static_cast<std::uint8_t>(value));
        if (version > DRW::AC1018)
            payload.putRawChar8(0);
    }
    payload.putRawShort16(330);
    payload.putRawLong64(0x1234u);
    if (opaque) {
        // Fabricated unmodeled data tests bounded preservation only.
        payload.putRawShort16(500);
        payload.putRawLong32(0x87654321u);
    }
    return payload.data();
}

// ODA v5.4.1 §20.4.104: frame a from-scratch R2004/R2007 XRECORD
// with inline data, no EED/reactors/xdictionary, and a null string stream.
// The library's established legacy short-length encoding is held fixed here;
// this regression concerns raw replay provenance, not its independent layout
// qualification. No downloaded drawing is needed or committed.
inline std::vector<std::uint8_t> frame(DRW::Version version,
                                      bool opaque = true) {
    const auto payload = data(version, opaque);
    dwgBufferW body;
    body.putObjType(version, 79);
    const auto sizeOffset = body.bitCount();
    body.putRawLong32(0);
    dwgHandle own;
    own.ref = 0xA620u;
    body.putHandle(own);
    body.putBitShort(0);
    body.putBitLong(0);
    body.putBit(1);
    body.putBitLong(static_cast<std::int32_t>(payload.size()));
    body.putBytes(payload.data(), payload.size());
    body.putBitShort(0);
    if (version > DRW::AC1018)
        body.putBit(0);
    body.patchRawLong32AtBit(sizeOffset, body.bitCount());
    dwgHandle owner;
    owner.code = 4;
    owner.ref = 0xCu;
    body.putHandle(owner);
    body.alignToByte();
    return body.data();
}

} // namespace xrecord_test

#endif
