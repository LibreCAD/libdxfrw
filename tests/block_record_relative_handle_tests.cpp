// SPDX-License-Identifier: GPL-2.0-or-later
#include "drw_objects.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include <cstdint>
#include <iostream>
#include <vector>

class DwgBlockOwnershipTestAccess {
public:
    static bool parse(DRW_Block_Record& record, DRW::Version version, dwgBuffer& input) {
        return record.parseDwg(version, &input);
    }
    static std::uint32_t first(const DRW_Block_Record& record) { return record.firstEH; }
    static std::uint32_t last(const DRW_Block_Record& record) { return record.lastEH; }
};

namespace {
enum Field : unsigned { Control, Reactor, XDictionary, Xref, Block, First, Last, End, Insert, Layout };

// Independently authored inline object bodies and tails (ODA 20.1/20.4.52).
std::vector<std::uint8_t> frame(DRW::Version version, Field changed,
                                std::uint8_t code, std::uint8_t size,
                                std::uint64_t value) {
    dwgBufferW b;
    b.putObjType(version, 49u);
    std::uint64_t sizeOffset = 0u;
    if (version >= DRW::AC1015) { sizeOffset = b.bitCount(); b.putRawLong32(0u); }
    b.putFixedHandle(0u, 2u, 0x200u); b.putBitShort(0u);
    if (version < DRW::AC1015) { sizeOffset = b.bitCount(); b.putRawLong32(0u); }
    b.putBitLong(1); // one reactor
    if (version >= DRW::AC1018) b.putBit(0u); // extension dictionary present
    b.putVariableText(version, "BLOCK_ROLE_TEST");
    b.putBit(0u); b.putBitShort(0u); b.putBit(0u); // xref flags/index
    b.putBit(0u); b.putBit(0u); b.putBit(0u); b.putBit(0u);
    if (version >= DRW::AC1015) b.putBit(0u); // loaded
    if (version >= DRW::AC1018) b.putBitLong(0); // no owned-entity vector
    b.put3BitDouble({0., 0., 0.}); b.putVariableText(version, "");
    if (version >= DRW::AC1015) {
        b.putRawChar8(1u); b.putRawChar8(0u); // one INSERT reference
        b.putVariableText(version, ""); b.putBitLong(0); // description, preview
    }
    const auto bodyEnd = static_cast<std::uint32_t>(b.bitCount());
    const std::uint8_t encoded[] = {static_cast<std::uint8_t>(bodyEnd),
        static_cast<std::uint8_t>(bodyEnd >> 8u), static_cast<std::uint8_t>(bodyEnd >> 16u),
        static_cast<std::uint8_t>(bodyEnd >> 24u)};
    if (!b.patchRawBytesAtBit(sizeOffset, encoded, sizeof encoded)) return {};
    const auto put = [&](Field field, std::uint8_t defaultRole, std::uint8_t defaultSize,
                         std::uint64_t defaultValue) {
        b.putFixedHandle(field == changed ? code : defaultRole,
                         field == changed ? size : defaultSize,
                         field == changed ? value : defaultValue);
    };
    put(Control, 4u, 1u, 0x10u); put(Reactor, 4u, 1u, 0x11u);
    put(XDictionary, 3u, 0u, 0u); put(Xref, 5u, 0u, 0u);
    put(Block, 3u, 2u, 0x201u);
    if (version <= DRW::AC1015) {
        put(First, 4u, 2u, 0x210u); put(Last, 4u, 2u, 0x211u);
    }
    put(End, 3u, 2u, 0x202u);
    if (version >= DRW::AC1015) {
        put(Insert, 4u, 2u, 0x220u); put(Layout, 5u, 0u, 0u);
    }
    b.alignToByte();
    return b.data();
}
}

int main() {
    int failures = 0;
    const auto expect = [&failures](bool result, const char* description) {
        if (!result) { std::cerr << description << '\n'; ++failures; }
    };
    for (const auto version : {DRW::AC1012, DRW::AC1014, DRW::AC1015, DRW::AC1018}) {
        DRW_Block_Record record;
        const auto parse = [&](const std::vector<std::uint8_t>& raw) {
            dwgBuffer input(raw.data(), raw.size());
            return DwgBlockOwnershipTestAccess::parse(record, version, input) && input.isGood();
        };
        const auto good = frame(version, Control, 4u, 1u, 0x10u);
        expect(parse(good) && record.handle == 0x200u, "valid BLOCK_HEADER must parse");
        struct Relative { std::uint8_t code, size; std::uint64_t value; std::uint32_t result; };
        for (const Relative ref : {Relative{6u, 0u, 0u, 0x201u}, Relative{8u, 0u, 0u, 0x1ffu},
                                   Relative{10u, 1u, 0x23u, 0x223u}, Relative{12u, 1u, 0x23u, 0x1ddu}}) {
            expect(parse(frame(version, Control, ref.code, ref.size, ref.value))
                       && record.parentHandle == ref.result,
                   "relative BLOCK_CONTROL reference must remain supported");
            if (version <= DRW::AC1015) {
                expect(parse(frame(version, First, ref.code, ref.size, ref.value))
                           && DwgBlockOwnershipTestAccess::first(record) == ref.result,
                       "relative first entity must resolve against the record handle");
                expect(parse(frame(version, Last, ref.code, ref.size, ref.value))
                           && DwgBlockOwnershipTestAccess::last(record) == ref.result,
                       "relative last entity must resolve against the record handle");
            }
            if (version >= DRW::AC1015)
                expect(parse(frame(version, Insert, ref.code, ref.size, ref.value))
                           && record.insertHandles == std::vector<std::uint32_t>{ref.result},
                       "relative INSERT reference must resolve against the record handle");
        }
        for (const auto field : {Control, Reactor, XDictionary, Xref, Block, First, Last, End, Insert, Layout}) {
            if ((field == First || field == Last) && version >= DRW::AC1018) continue;
            if ((field == Insert || field == Layout) && version < DRW::AC1015) continue;
            const std::uint8_t role = field == XDictionary || field == Block || field == End ? 3u
                : field == Xref || field == Layout ? 5u : 4u;
            expect(!parse(frame(version, field, role, 5u, UINT64_C(0x100000010)))
                       && record.dwgHandleFieldSpans().empty(),
                   "wide BLOCK_HEADER handle must refuse without publishing spans");
            expect(parse(good), "corrected BLOCK_HEADER must parse after wide handle");
        }
        if (version <= DRW::AC1015) {
            expect(!parse(frame(version, First, 6u, 1u, 1u)), "relative +1 must have zero length");
            expect(parse(good), "corrected BLOCK_HEADER must parse after malformed relative handle");
            expect(!parse(frame(version, First, 8u, 1u, 1u)), "relative -1 must have zero length");
            expect(parse(good), "corrected BLOCK_HEADER must parse after malformed -1 handle");
            expect(!parse(frame(version, First, 12u, 2u, 0x201u)), "relative handle underflow must refuse");
            expect(parse(good), "corrected BLOCK_HEADER must parse after relative underflow");
            expect(!parse(frame(version, First, 10u, 5u, UINT64_C(0x100000000))),
                   "relative handle beyond the model range must refuse");
            expect(parse(good), "corrected BLOCK_HEADER must parse after relative overflow");
        }
    }
    return failures ? 1 : 0;
}
