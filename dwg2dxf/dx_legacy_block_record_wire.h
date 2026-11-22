// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_LEGACY_BLOCK_RECORD_WIRE_H
#define DX_LEGACY_BLOCK_RECORD_WIRE_H

#include "dx_legacy_layout_wire.h"

// Read-only access to parser-owned linkage; this does not alter the public
// BLOCK_RECORD layout or allow an adapter to install source evidence.
class DRW_DwgBlockRecordWireAccess {
public:
    static std::uint32_t block(const DRW_Block_Record& value) noexcept { return value.block; }
    static std::uint32_t end(const DRW_Block_Record& value) noexcept { return value.endBlock; }
    static std::uint32_t first(const DRW_Block_Record& value) noexcept { return value.firstEH; }
    static std::uint32_t last(const DRW_Block_Record& value) noexcept { return value.lastEH; }
};

// Independent, allocation-free R13/R14 BLOCK_HEADER body/tail observer. The
// caller must bind the captured frame, block control, graph and output.
namespace dx_legacy_block_record_wire {
using dx_legacy_dwg_wire::Bits;
enum class Field : std::size_t { Control, Reactors, XDictionary, Xref, Block,
                                 First, Last, End, Count };
struct Proof {
    std::uint32_t bodyEnd = 0u;
    std::array<dx_raster_wire::Span, static_cast<std::size_t>(Field::Count)> handles{};
};

inline bool record(DRW::Version version, const std::vector<std::uint8_t>& bytes,
                   const DRW_Block_Record& value, Proof* proof = nullptr) noexcept {
    using namespace dx_legacy_dwg_wire;
    using dx_legacy_layout_wire::absolute;
    using dx_legacy_layout_wire::ascii;
    using dx_legacy_layout_wire::point;
    using dx_legacy_layout_wire::typedAbsolute;
    const auto size = bytes.size();
    constexpr int allowedFlags = 1 | 2 | 4 | 8 | 16 | 64;
    if (!supported(version) || size == 0u || size > UINT32_MAX / 8u
        || value.tType != DRW::BLOCK_RECORD || value.handle == 0u
        || value.handle == DRW::NoHandle || value.parentHandle == 0u
        || value.parentHandle == DRW::NoHandle || value.name.empty()
        || !ascii(value.name) || !ascii(value.xrefPath) || (value.flags & ~allowedFlags) != 0
        || value.xrefIndex < -1 || value.xrefIndex > 65534
        || value.reactorCount() < 0
        || value.reactorHandles.size() != static_cast<std::size_t>(value.reactorCount())
        || value.xrefUnloaded || value.insUnits != 0 || !value.canExplode
        || value.blockScaling != 0 || !value.description.empty() || !value.previewData.empty()
        || value.layoutHandle != 0u || value.insertCount != 0u || !value.insertHandles.empty()
        || value.xrefBlockHandle.ref != 0u
        || value.entityMapCount() != 0u || value.hasDataStorageBinaryData()
        || !value.appData.empty() || !value.extData.empty()
        || !value.pendingAppIdResolutions.empty() || !value.pendingLayerRefResolutions.empty())
        return false;
    Bits b{bytes.data(), 0u, static_cast<std::uint32_t>(size * 8u)};
    std::uint32_t bodyEnd = 0u;
    if (!frameStart(b, version, 49u, value.handle, value.reactorHandles.size(),
                    [](Bits& source) noexcept { return source.shortValue() == 0u && source.good; },
                    bodyEnd)
        || !text(b, value.name)
        || b.take(1u) != static_cast<unsigned>((value.flags & 64) != 0)
        || b.shortValue() != static_cast<std::uint32_t>(value.xrefIndex + 1)
        || b.take(1u) != static_cast<unsigned>((value.flags & 16) != 0)
        || b.take(1u) != static_cast<unsigned>((value.flags & 1) != 0)
        || b.take(1u) != static_cast<unsigned>((value.flags & 2) != 0)
        || b.take(1u) != static_cast<unsigned>((value.flags & 4) != 0)
        || b.take(1u) != static_cast<unsigned>((value.flags & 8) != 0)
        || !point(b, value.basePoint) || !text(b, value.xrefPath)
        || !b.good || b.pos != bodyEnd) return false;

    Proof staged; staged.bodyEnd = bodyEnd;
    std::size_t index = 0u;
    const auto mark = [&staged](Field field, std::uint32_t begin, std::uint32_t end) {
        staged.handles[static_cast<std::size_t>(field)] = {begin, end, true};
    };
    const auto observe = [&b, &mark, &index, &value, version](
            Field field, const char* name, std::uint8_t role,
            std::uint32_t expected, bool offset) {
        const auto begin = b.pos;
        Bits roleProbe = b;
        const auto rawCode = static_cast<std::uint8_t>(roleProbe.take(4u));
        if (!roleProbe.good || (offset ? !reference(b, role, value.handle, expected)
                                       : !absolute(b, role, expected))) return false;
        const auto& spans = value.dwgHandleFieldSpans();
        if (index >= spans.size()) return false;
        const auto& span = spans[index++];
        if (span.m_name != name || span.m_version != version || !span.m_parseComplete
            || span.m_startBit != begin || span.m_bitSize != b.pos - begin
            || span.m_rawHandleCode != rawCode) return false;
        mark(field, begin, b.pos);
        return true;
    };
    if (!observe(Field::Control, "blockControl", 4u, value.parentHandle, true)) return false;
    auto start = b.pos;
    for (const auto reactor : value.reactorHandles)
        if (!absolute(b, 4u, reactor)) return false;
    mark(Field::Reactors, start, b.pos);
    start = b.pos;
    if (!absolute(b, 3u, value.xDictHandle)) return false;
    mark(Field::XDictionary, start, b.pos);
    start = b.pos;
    if (!typedAbsolute(b, 5u, value.xrefBlockHandle)) return false;
    const auto& spans = value.dwgHandleFieldSpans();
    if (index >= spans.size() || spans[index].m_name != "xrefBlock"
        || spans[index].m_version != version || !spans[index].m_parseComplete
        || spans[index].m_startBit != start || spans[index].m_bitSize != b.pos - start
        || spans[index].m_rawHandleCode != value.xrefBlockHandle.rawCode) return false;
    ++index; mark(Field::Xref, start, b.pos);
    if (!observe(Field::Block, "block", 3u, DRW_DwgBlockRecordWireAccess::block(value), false)) return false;
    if ((value.flags & (4 | 8)) == 0) {
        if (!observe(Field::First, "firstEntity", 4u, DRW_DwgBlockRecordWireAccess::first(value), true)
            || !observe(Field::Last, "lastEntity", 4u, DRW_DwgBlockRecordWireAccess::last(value), true))
            return false;
    } else if (DRW_DwgBlockRecordWireAccess::first(value) != DRW::NoHandle
               || DRW_DwgBlockRecordWireAccess::last(value) != DRW::NoHandle) return false;
    if (!observe(Field::End, "endBlock", 3u, DRW_DwgBlockRecordWireAccess::end(value), false)
        || index != spans.size() || !padding(b)) return false;
    if (proof != nullptr) *proof = staged;
    return true;
}
} // namespace dx_legacy_block_record_wire
#endif
