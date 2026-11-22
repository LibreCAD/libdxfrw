// SPDX-License-Identifier: GPL-2.0-or-later
#include "drw_entities.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

class LegacyTableProbe : public DRW_Table {
public:
    using DRW_Table::encodeDwg;
    using DRW_Table::parseDwg;
};

namespace {
struct AttributeWire {
    std::uint8_t firstRole{4u}, lastRole{4u}, seqRole{3u};
    bool nullEndpoints{false}, nullSeqend{false}, nullFirstOnly{false};
    bool invalidUnitWidth{false}, excessiveOffset{false}, wideFirst{false};
};
// A locally authored R13/R14 common entity and TABLE prefix. The tail is
// deliberately opaque: this control makes no claim about TABLE cell semantics.
std::vector<std::uint8_t> frame(DRW::Version version, std::uint8_t role = 5u,
                                std::uint8_t size = 2u,
                                std::uint64_t block = 0x5678u,
                                bool attributes = false, bool nonfinite = false,
                                bool grid = false, std::uint8_t styleRole = 5u,
                                bool gridNonfinite = false, bool gridOversized = false,
                                bool gridEmpty = false, AttributeWire attrib = {},
                                std::uint32_t self = 0x1234u) {
    dwgBufferW body, handles;
    body.putObjType(version, 523u);
    body.putFixedHandle(0u, 2u, self);
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
    if (grid) {
        body.putBitShort(0x16u);
        body.put3BitDouble({1.0, 0.0, 0.0});
        body.putBitLong(gridOversized ? 1001 : gridEmpty ? 0 : 3); body.putBitLong(2);
        body.putBitDouble(gridNonfinite ? std::numeric_limits<double>::quiet_NaN() : 10.0);
        body.putBitDouble(20.0); body.putBitDouble(30.0);
        body.putBitDouble(4.0); body.putBitDouble(5.0);
    }
    body.putRawChar8(0xA5u); // unqualified cell-body sentinel
    const auto dataEnd = static_cast<std::uint32_t>(body.bitCount());
    const std::uint8_t dataEndBytes[] = {
        static_cast<std::uint8_t>(dataEnd), static_cast<std::uint8_t>(dataEnd >> 8u),
        static_cast<std::uint8_t>(dataEnd >> 16u), static_cast<std::uint8_t>(dataEnd >> 24u)};
    if (!body.patchRawBytesAtBit(sizeOffset, dataEndBytes, sizeof dataEndBytes)) return {};
    handles.putFixedHandle(3u, 0u, 0u); // extension dictionary
    handles.putFixedHandle(5u, 1u, 0x12u); // layer
    handles.putFixedHandle(role, size, block); // BLOCK_HEADER
    if (attributes) {
        const auto putEndpoint = [&handles, &attrib](std::uint8_t code, bool first) {
            const bool relativeUnit = code == 6u || code == 8u;
            const bool isNull = attrib.nullEndpoints || (attrib.nullFirstOnly && first);
            const bool badWidth = attrib.invalidUnitWidth && first && relativeUnit;
            const bool wide = attrib.wideFirst && first;
            const bool relativeOffset = code == 10u || code == 12u;
            const std::uint64_t value = wide ? UINT64_C(0x100002345)
                : badWidth ? 1u : isNull || relativeUnit ? 0u
                : relativeOffset ? (attrib.excessiveOffset ? 0xFFFFu : 0x30u) : 0x2345u;
            handles.putFixedHandle(code, wide ? 5u : badWidth ? 1u
                                   : isNull || relativeUnit ? 0u : 2u, value);
        };
        putEndpoint(attrib.firstRole, true);
        putEndpoint(attrib.lastRole, false);
        handles.putFixedHandle(attrib.seqRole, attrib.nullSeqend ? 0u : 2u,
                               attrib.nullSeqend ? 0u : 0x2346u);
    }
    if (grid) handles.putFixedHandle(styleRole, 1u, 0x87u); // TABLESTYLE
    for (std::uint64_t bit = 0; bit < handles.bitCount(); ++bit)
        body.putBit((handles.data()[bit / 8u] >> (7u - bit % 8u)) & 1u);
    body.alignToByte();
    return body.data();
}

// R2000 uses the same first/last attribute links, but a different common
// entity and scale encoding. An empty grid keeps this control focused on links.
std::vector<std::uint8_t> frame2000Attributes() {
    dwgBufferW body, handles;
    body.putObjType(DRW::AC1015, 523u);
    const auto sizeOffset = body.bitCount();
    body.putRawLong32(0u);
    body.putFixedHandle(0u, 2u, 0x1234u);
    body.putBitShort(0u); body.putBit(0u); // no EED or proxy
    body.put2Bits(2u); body.putBitLong(0); body.putBit(1u); // model, no reactors/links
    body.putBitShort(256u); body.putBitDouble(1.0);
    body.put2Bits(0u); body.put2Bits(0u); // by-layer linetype and plot style
    body.putBitShort(0u); body.putRawChar8(0u);
    body.put3BitDouble({10.0, 20.0, 30.0});
    body.put2Bits(3u); // unit scales
    body.putBitDouble(0.5); body.put3BitDouble({0.0, 0.0, 1.0});
    body.putBit(1u); // attributes
    body.putBitShort(0x16u); body.put3BitDouble({1.0, 0.0, 0.0});
    body.putBitLong(0); body.putBitLong(0); // empty grid
    for (int i = 0; i < 4; ++i) body.putBit(0u); // no table overrides
    const auto dataEnd = static_cast<std::uint32_t>(body.bitCount());
    const std::uint8_t dataEndBytes[] = {
        static_cast<std::uint8_t>(dataEnd), static_cast<std::uint8_t>(dataEnd >> 8u),
        static_cast<std::uint8_t>(dataEnd >> 16u), static_cast<std::uint8_t>(dataEnd >> 24u)};
    if (!body.patchRawBytesAtBit(sizeOffset, dataEndBytes, sizeof dataEndBytes)) return {};
    handles.putFixedHandle(3u, 0u, 0u); handles.putFixedHandle(5u, 1u, 0x12u);
    handles.putFixedHandle(5u, 2u, 0x5678u); // block
    handles.putFixedHandle(4u, 2u, 0x2345u); handles.putFixedHandle(4u, 2u, 0x2345u);
    handles.putFixedHandle(3u, 2u, 0x2346u); handles.putFixedHandle(5u, 1u, 0x87u);
    for (std::uint64_t bit = 0; bit < handles.bitCount(); ++bit)
        body.putBit((handles.data()[bit / 8u] >> (7u - bit % 8u)) & 1u);
    body.alignToByte();
    return body.data();
}
}

int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--source-frame") {
        std::ifstream input(argv[2], std::ios::binary);
        const std::vector<std::uint8_t> bytes{
            std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        if (!input.is_open() || bytes.empty()) return 2;
        LegacyTableProbe table;
        dwgBuffer buffer(bytes.data(), bytes.size());
        const bool ok = table.parseDwg(DRW::AC1014, &buffer, 0u) && buffer.isGood()
            && table.handle == 0x528u && table.blockRecH.ref == 0x570u
            && table.attribHandles.size() == 2u
            && table.attribHandles[0].ref == 0x705u
            && table.attribHandles[1].ref == 0x705u
            && table.seqendH.ref == 0x706u && table.m_tableStyleHandle == 0x87u
            && table.m_content.m_columns.size() == 3u
            && table.m_content.m_rows.size() == 7u && !table.m_semanticContentComplete;
        if (!ok) std::cerr << "source TABLE attribute prefix mismatch\n";
        return ok ? 0 : 1;
    }
    if (argc != 1) return 2;
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
                && table.m_hasSemanticContent && !table.m_semanticContentComplete
                && table.colcount == 1 && table.rowcount == 1;
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
        const auto attributeFrame = [version](AttributeWire wire = {}) {
            return frame(version, 5u, 2u, 0x5678u, true, false, true,
                         5u, false, false, false, wire);
        };
        const auto withAttributes = attributeFrame();
        expect(parse(withAttributes) && matches()
                   && table.attribHandles.size() == 2u
                   && table.attribHandles[0].ref == 0x2345u
                   && table.attribHandles[1].ref == 0x2345u
                   && table.seqendH.ref == 0x2346u
                   && table.m_tableStyleHandle == 0x87u,
               "legacy TABLE parses attribute endpoints and following style");
        for (const auto role : {std::uint8_t{2}, std::uint8_t{3}, std::uint8_t{5}}) {
            AttributeWire wire; wire.firstRole = role;
            expect(!parse(attributeFrame(wire))
                       && !table.m_hasSemanticContent,
                   "wrong first ATTRIB role must refuse and reset");
            expect(parse(withAttributes) && table.attribHandles.size() == 2u,
                   "TABLE retries after wrong ATTRIB role");
        }
        AttributeWire wrongSequence; wrongSequence.seqRole = 4u;
        expect(!parse(attributeFrame(wrongSequence))
                   && !table.m_hasSemanticContent,
               "wrong SEQEND role must refuse and reset");
        expect(parse(withAttributes) && table.seqendH.ref == 0x2346u,
               "TABLE retries after wrong SEQEND role");
        AttributeWire wrongLast; wrongLast.lastRole = 5u;
        expect(!parse(attributeFrame(wrongLast))
                   && !table.m_hasSemanticContent,
               "wrong last ATTRIB role must refuse and reset");
        expect(parse(withAttributes) && table.attribHandles.size() == 2u,
               "TABLE retries after wrong last ATTRIB role");
        AttributeWire relative; relative.firstRole = 6u; relative.lastRole = 8u;
        expect(parse(attributeFrame(relative))
                   && table.attribHandles.size() == 2u
                   && table.attribHandles[0].ref == 0x1235u
                   && table.attribHandles[1].ref == 0x1233u,
               "relative unit ATTRIB handles resolve against the TABLE");
        relative.invalidUnitWidth = true;
        expect(!parse(attributeFrame(relative)) && !table.m_hasSemanticContent,
               "relative unit ATTRIB handles with payload must refuse");
        expect(parse(withAttributes) && table.attribHandles.size() == 2u,
               "TABLE retries after a malformed relative handle");
        AttributeWire offsets; offsets.firstRole = 10u; offsets.lastRole = 12u;
        expect(parse(attributeFrame(offsets)) && table.attribHandles.size() == 2u
                   && table.attribHandles[0].ref == 0x1264u
                   && table.attribHandles[1].ref == 0x1204u,
               "relative offset ATTRIB handles resolve against the TABLE");
        offsets.firstRole = 12u; offsets.excessiveOffset = true;
        expect(!parse(attributeFrame(offsets)) && !table.m_hasSemanticContent,
               "an underflowing ATTRIB offset must refuse and reset");
        AttributeWire wide; wide.wideFirst = true;
        expect(!parse(attributeFrame(wide)) && !table.m_hasSemanticContent,
               "an overwide ATTRIB handle must refuse and reset");
        expect(parse(withAttributes) && table.attribHandles.size() == 2u,
               "TABLE retries after invalid offset and width");
        AttributeWire underflow; underflow.firstRole = 8u; underflow.lastRole = 8u;
        expect(!parse(frame(version, 5u, 2u, 0x5678u, true, false, true,
                            5u, false, false, false, underflow, 1u))
                   && !table.m_hasSemanticContent,
               "relative ATTRIB references must not resolve to the null handle");
        expect(parse(withAttributes) && table.attribHandles.size() == 2u,
               "TABLE retries after a null relative reference");
        AttributeWire nulls; nulls.nullEndpoints = true;
        expect(parse(attributeFrame(nulls))
                   && table.attribHandles.size() == 2u
                   && table.attribHandles[0].ref == DRW::NoHandle
                   && table.attribHandles[1].ref == DRW::NoHandle,
               "both legacy attribute endpoints may be typed nulls");
        AttributeWire oneNull; oneNull.nullFirstOnly = true;
        expect(!parse(attributeFrame(oneNull))
                   && !table.m_hasSemanticContent,
               "a single null ATTRIB endpoint must refuse and reset");
        AttributeWire nullSequence; nullSequence.nullSeqend = true;
        expect(!parse(attributeFrame(nullSequence))
                   && !table.m_hasSemanticContent,
               "missing SEQEND must refuse and reset");
        expect(parse(withAttributes) && table.seqendH.ref == 0x2346u,
               "TABLE retries after a missing SEQEND");
        auto truncatedAttributes = withAttributes;
        truncatedAttributes.resize(truncatedAttributes.size() - 4u);
        expect(!parse(truncatedAttributes) && !table.m_hasSemanticContent,
               "truncated ATTRIB/SEQEND tail must refuse and reset");
        expect(parse(withAttributes) && table.seqendH.ref == 0x2346u,
               "TABLE retries after a truncated ATTRIB/SEQEND tail");
        expect(!parse(frame(version, 5u, 2u, 0x5678u, false, true))
                   && !table.m_hasSemanticContent,
               "non-finite scale must refuse and reset");
        expect(parse(good) && matches(), "TABLE retries after a non-finite scale");
        auto truncated = good;
        truncated.pop_back();
        expect(!parse(truncated) && !table.m_hasSemanticContent,
               "truncated BLOCK_HEADER handle must refuse and reset");
        expect(parse(good) && matches(), "TABLE retries after truncation");
        const auto grid = frame(version, 5u, 2u, 0x5678u, false, false, true);
        const auto gridMatches = [&]() {
            return matches() && table.m_valueFlag == 0x16 && table.m_tableStyleHandle == 0x87u
                && table.m_content.m_tableStyleHandle == 0x87u
                && table.m_horizontalDirection.x == 1.0
                && table.m_horizontalDirection.y == 0.0
                && table.m_content.m_columns.size() == 3u
                && table.m_content.m_columns[0].m_width == 10.0
                && table.m_content.m_columns[1].m_width == 20.0
                && table.m_content.m_columns[2].m_width == 30.0
                && table.m_content.m_rows.size() == 2u
                && table.m_content.m_rows[0].m_height == 4.0
                && table.m_content.m_rows[1].m_height == 5.0
                && table.m_content.m_rows[0].m_cells.size() == 3u;
        };
        expect(parse(grid) && gridMatches(),
               "legacy TABLE publishes a bounded grid but keeps cells incomplete");
        dwgBufferW outputBody, outputHandles;
        outputBody.putRawChar8(0xA5u);
        outputHandles.putRawChar8(0x5Au);
        const auto bodyBefore = outputBody.data();
        const auto handlesBefore = outputHandles.data();
        expect(!table.encodeDwg(DRW::AC1018, &outputBody, 0u, nullptr, &outputHandles)
                   && outputBody.data() == bodyBefore && outputHandles.data() == handlesBefore,
               "incomplete legacy grid cannot be encoded as a complete TABLE");
        for (const auto& bad : {
                frame(version, 5u, 2u, 0x5678u, false, false, true, 4u),
                frame(version, 5u, 2u, 0x5678u, false, false, true, 5u, true),
                frame(version, 5u, 2u, 0x5678u, false, false, true, 5u, false, true),
                frame(version, 5u, 2u, 0x5678u, false, false, true, 5u, false, false, true)}) {
            expect(parse(bad) && matches() && table.colcount == 1 && table.rowcount == 1
                       && table.m_content.m_columns.empty() && table.m_content.m_rows.empty(),
                   "invalid optional grid retains only the valid insertion prefix");
            expect(parse(grid) && gridMatches(), "corrected grid succeeds after optional fallback");
        }
    }
    {
        const auto bytes = frame2000Attributes();
        LegacyTableProbe table;
        dwgBuffer input(bytes.data(), bytes.size());
        expect(!bytes.empty() && table.parseDwg(DRW::AC1015, &input, 0u)
                   && input.isGood() && table.attribHandles.size() == 2u
                   && table.attribHandles[0].ref == 0x2345u
                   && table.attribHandles[1].ref == 0x2345u
                   && table.seqendH.ref == 0x2346u
                   && table.m_tableStyleHandle == 0x87u,
               "R2000 TABLE uses legacy attribute endpoints before its style");
    }
    return failures ? 1 : 0;
}
