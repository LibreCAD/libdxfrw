// SPDX-License-Identifier: GPL-2.0-or-later
#include "drw_entities.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include <cstdint>
#include <iostream>
#include <limits>
#include <vector>

class LegacyTableProbe : public DRW_Table {
public:
    using DRW_Table::parseDwg;
};

namespace {
// A locally authored R13/R14 common entity and TABLE prefix. The tail is
// deliberately opaque: this control makes no claim about TABLE cell semantics.
std::vector<std::uint8_t> frame(DRW::Version version, std::uint8_t role = 5u,
                                std::uint8_t size = 2u,
                                std::uint64_t block = 0x5678u,
                                bool attributes = false, bool nonfinite = false) {
    dwgBufferW body, handles;
    body.putObjType(version, 523u);
    body.putFixedHandle(0u, 2u, 0x1234u);
    body.putBitShort(0u); // no EED
    body.putBit(0u); // no proxy graphics
    const auto sizeOffset = body.bitCount();
    body.putRawLong32(0u);
    body.put2Bits(2u); // model space
    body.putBitLong(0); body.putBit(1u); body.putBit(1u); // no reactors, BYLAYER, no links
    body.putBitShort(256u); body.putBitDouble(1.0); body.putBitShort(0u);
    body.put3BitDouble({10.0, 20.0, 30.0});
    body.putBitDouble(nonfinite ? std::numeric_limits<double>::quiet_NaN() : 2.0);
    body.putBitDouble(3.0); body.putBitDouble(4.0);
    body.putBitDouble(0.5); body.put3BitDouble({0.0, 0.0, 1.0});
    body.putBit(attributes ? 1u : 0u);
    body.putRawChar8(0xA5u); // unqualified cell-body sentinel
    const auto dataEnd = static_cast<std::uint32_t>(body.bitCount());
    const std::uint8_t dataEndBytes[] = {
        static_cast<std::uint8_t>(dataEnd), static_cast<std::uint8_t>(dataEnd >> 8u),
        static_cast<std::uint8_t>(dataEnd >> 16u), static_cast<std::uint8_t>(dataEnd >> 24u)};
    if (!body.patchRawBytesAtBit(sizeOffset, dataEndBytes, sizeof dataEndBytes)) return {};
    handles.putFixedHandle(3u, 0u, 0u); // extension dictionary
    handles.putFixedHandle(5u, 1u, 0x12u); // layer
    handles.putFixedHandle(role, size, block); // BLOCK_HEADER
    for (std::uint64_t bit = 0; bit < handles.bitCount(); ++bit)
        body.putBit((handles.data()[bit / 8u] >> (7u - bit % 8u)) & 1u);
    body.alignToByte();
    return body.data();
}
}

int main() {
    int failures = 0;
    const auto expect = [&failures](bool value, const char* message) {
        if (!value) { std::cerr << message << '\n'; ++failures; }
    };
    for (const auto version : {DRW::AC1012, DRW::AC1014}) {
        LegacyTableProbe table;
        const auto parse = [&](const std::vector<std::uint8_t>& bytes) {
            dwgBuffer input(bytes.data(), bytes.size());
            return table.parseDwg(version, &input, 0u) && input.isGood();
        };
        const auto good = frame(version);
        const auto matches = [&]() {
            return table.handle == 0x1234u && table.blockRecH.ref == 0x5678u
                && table.basePoint.x == 10.0 && table.basePoint.y == 20.0
                && table.basePoint.z == 30.0 && table.xscale == 2.0
                && table.yscale == 3.0 && table.zscale == 4.0
                && table.angle == 0.5 && table.extPoint.x == 0.0
                && table.extPoint.y == 0.0 && table.extPoint.z == 1.0
                && table.m_hasSemanticContent && !table.m_semanticContentComplete;
        };
        expect(!good.empty() && parse(good) && matches(),
               "legacy TABLE preserves the bounded insertion prefix and marks its tail incomplete");
        for (const auto role : {std::uint8_t{2}, std::uint8_t{3}, std::uint8_t{4},
                                std::uint8_t{6}, std::uint8_t{8}, std::uint8_t{10}, std::uint8_t{12}}) {
            expect(!parse(frame(version, role)) && !table.m_hasSemanticContent,
                   "wrong TABLE BLOCK_HEADER handle role must refuse and reset");
            expect(parse(good) && matches(), "TABLE retries after a wrong handle role");
        }
        expect(!parse(frame(version, 5u, 5u, UINT64_C(0x100005678)))
                   && !table.m_hasSemanticContent,
               "wide TABLE BLOCK_HEADER handle must refuse and reset");
        expect(parse(good) && matches(), "TABLE retries after a wide handle");
        expect(!parse(frame(version, 5u, 2u, 0x5678u, true))
                   && !table.m_hasSemanticContent,
               "unqualified legacy ATTRIB sequence must refuse and reset");
        expect(parse(good) && matches(), "TABLE retries after an ATTRIB sequence");
        expect(!parse(frame(version, 5u, 2u, 0x5678u, false, true))
                   && !table.m_hasSemanticContent,
               "non-finite scale must refuse and reset");
        expect(parse(good) && matches(), "TABLE retries after a non-finite scale");
        auto truncated = good;
        truncated.pop_back();
        expect(!parse(truncated) && !table.m_hasSemanticContent,
               "truncated BLOCK_HEADER handle must refuse and reset");
        expect(parse(good) && matches(), "TABLE retries after truncation");
    }
    return failures ? 1 : 0;
}
