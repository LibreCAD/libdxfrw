// SPDX-License-Identifier: GPL-2.0-or-later
#include "drw_objects.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "dx_legacy_layout_wire.h"
#include <cstdint>
#include <iostream>
#include <vector>

namespace {
class LayoutProbe : public DRW_Layout {
public:
    using DRW_Layout::parseDwg;
};

// Hand-authored R13/R14 LAYOUT body and inline handle tail, ODA 20.1/20.4.84.
std::vector<std::uint8_t> frame(DRW::Version version, unsigned field,
                                std::uint8_t role, std::uint8_t size,
                                std::uint64_t target) {
    dwgBufferW b;
    b.putObjType(version, 502u);
    b.putFixedHandle(0u, 1u, 0x2eu);
    b.putBitShort(0u); // no EED
    const auto sizeOffset = b.bitCount();
    b.putRawLong32(0u);
    b.putBitLong(1); // one reactor
    b.putVariableText(version, ""); b.putVariableText(version, "");
    b.putBitShort(0u);
    for (int i = 0; i < 6; ++i) b.putBitDouble(0.0);
    b.putVariableText(version, "");
    b.putBitDouble(0.0); b.putBitDouble(0.0);
    b.putBitShort(0u); b.putBitShort(0u); b.putBitShort(0u);
    for (int i = 0; i < 4; ++i) b.putBitDouble(0.0);
    b.putVariableText(version, "");
    b.putBitDouble(1.0); b.putBitDouble(1.0);
    b.putVariableText(version, ""); b.putBitShort(0u);
    b.putBitDouble(1.0); b.putBitDouble(0.0); b.putBitDouble(0.0);
    b.putVariableText(version, "Model"); b.putBitLong(0); b.putBitShort(0u);
    b.put3BitDouble({0., 0., 0.});
    for (int i = 0; i < 4; ++i) b.putRawDouble(0.0);
    b.put3BitDouble({0., 0., 0.}); b.put3BitDouble({1., 0., 0.});
    b.put3BitDouble({0., 1., 0.}); b.putBitDouble(0.0); b.putBitShort(0u);
    b.put3BitDouble({0., 0., 0.}); b.put3BitDouble({0., 0., 0.});
    const auto end = static_cast<std::uint32_t>(b.bitCount());
    const std::uint8_t bytes[] = {static_cast<std::uint8_t>(end),
        static_cast<std::uint8_t>(end >> 8u), static_cast<std::uint8_t>(end >> 16u),
        static_cast<std::uint8_t>(end >> 24u)};
    if (!b.patchRawBytesAtBit(sizeOffset, bytes, sizeof bytes)) return {};
    b.putFixedHandle(field == 4u ? role : 4u, field == 4u ? size : 1u,
                     field == 4u ? target : 0x23u); // owner
    b.putFixedHandle(field == 6u ? role : 4u, field == 6u ? size : 1u,
                     field == 6u ? target : 0x25u); // reactor
    b.putFixedHandle(field == 5u ? role : 3u, field == 5u ? size : 0u,
                     field == 5u ? target : 0u); // xdictionary
    const std::uint8_t roles[] = {4u, 4u, 5u, 5u};
    for (unsigned i = 0; i < 4u; ++i)
        b.putFixedHandle(i == field ? role : roles[i], i == field ? size : 1u,
                         i == field ? target : 0x40u + i);
    b.alignToByte();
    return b.data();
}
}

int main() {
    int failures = 0;
    const auto expect = [&failures](bool ok, const char* label) {
        if (!ok) { std::cerr << label << '\n'; ++failures; }
    };
    for (const auto version : {DRW::AC1012, DRW::AC1014}) {
        LayoutProbe object;
        const auto parse = [&](const std::vector<std::uint8_t>& raw) {
            dwgBuffer input(raw.data(), raw.size());
            return object.parseDwg(version, &input, 0u) && input.isGood();
        };
        const auto good = frame(version, 0u, 4u, 1u, 0x40u);
        expect(parse(good) && object.name == "Model"
                   && object.paperSpaceBlockRecordHandle.ref == 0x40u,
               "valid legacy LAYOUT must parse");
        DRW_UnsupportedObject raw;
        raw.m_version = version; raw.m_objectType = 82; raw.m_handle = object.handle;
        raw.m_objectSize = static_cast<std::uint32_t>(good.size()); raw.m_rawBytes = good;
        dx_legacy_layout_wire::Proof proof;
        expect(dx_legacy_layout_wire::layout(raw, object, &proof)
                   && proof.sourceClass == 502u && proof.bodyEnd != 0u,
               "independent LAYOUT observer must match the source frame");
        DRW_Layout changed = object;
        ++changed.paperSpaceBlockRecordHandle.ref;
        proof.sourceClass = 123u;
        expect(!dx_legacy_layout_wire::layout(raw, changed, &proof)
                   && proof.sourceClass == 123u,
               "independent observer must reject changed block target");
        changed = object; changed.paperSpaceBlockRecordHandle.rawCode = 5u;
        expect(!dx_legacy_layout_wire::layout(raw, changed),
               "independent observer must reject changed typed role metadata");
        changed = object; ++changed.paperSpaceBlockRecordHandle.ref64;
        expect(!dx_legacy_layout_wire::layout(raw, changed),
               "independent observer must reject changed full-width handle metadata");
        changed = object; changed.marginLeft = 2.0;
        expect(!dx_legacy_layout_wire::layout(raw, changed),
               "independent observer must reject changed body fields");
        changed = object; changed.marginLeft = -0.0;
        expect(!dx_legacy_layout_wire::layout(raw, changed),
               "independent observer must preserve signed bit-double zero");
        changed = object; changed.limMinX = -0.0;
        expect(!dx_legacy_layout_wire::layout(raw, changed),
               "independent observer must preserve signed raw-double zero");
        auto damaged = raw; damaged.m_rawBytes.front() ^= 0x80u;
        expect(!dx_legacy_layout_wire::layout(damaged, object)
                   && dx_legacy_layout_wire::layout(raw, object),
               "independent observer must reject a changed frame and allow retry");
        for (unsigned field = 0; field < 4u; ++field) {
            const std::uint8_t expected = field < 2u ? 4u : 5u;
            for (const std::uint8_t role : {std::uint8_t{0}, std::uint8_t{2},
                    std::uint8_t{3}, std::uint8_t{4}, std::uint8_t{5},
                    std::uint8_t{6}, std::uint8_t{8}, std::uint8_t{10}, std::uint8_t{12}}) {
                if (role == expected) continue;
                const auto badRole = frame(version, field, role, 1u, 0x40u + field);
                expect(!parse(badRole) && object.name.empty(), "wrong handle role must reset state");
                expect(parse(good), "corrected source must parse after wrong role");
            }
            const auto wide = frame(version, field, expected, 5u, UINT64_C(0x100000040) + field);
            expect(!parse(wide) && object.name.empty(), "wide handle must not truncate");
            expect(parse(good), "corrected source must parse after wide handle");
            const auto nullRef = frame(version, field, 0u, 0u, 0u);
            expect(parse(nullRef), "canonical null reference must parse");
            const auto typedNull = frame(version, field, expected, 0u, 0u);
            expect(parse(typedNull), "typed null reference must parse");
        }
        for (const auto field : {4u, 5u, 6u}) {
            const auto bad = frame(version, field, 5u, 1u, 0x23u);
            expect(!parse(bad) && object.name.empty(), "wrong common handle role must reset state");
            expect(parse(good), "corrected source must parse after common role failure");
            const auto wide = frame(version, field, field == 5u ? 3u : 4u,
                                    5u, UINT64_C(0x100000023));
            expect(!parse(wide) && object.name.empty(), "wide common handle must not truncate");
            expect(parse(good), "corrected source must parse after wide common handle");
        }
        const auto relativeOwner = frame(version, 4u, 6u, 0u, 0u);
        expect(parse(relativeOwner) && object.parentHandle == 0x2fu,
               "relative soft-pointer owner must remain supported");
        auto truncated = good; truncated.pop_back();
        expect(!parse(truncated) && object.name.empty(), "truncated handle must reset state");
        expect(parse(good), "corrected source must parse after truncation");
    }
    return failures ? 1 : 0;
}
