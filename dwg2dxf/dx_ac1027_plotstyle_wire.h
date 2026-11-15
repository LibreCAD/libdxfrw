// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1027_PLOTSTYLE_WIRE_H
#define DX_AC1027_PLOTSTYLE_WIRE_H

#include "dx_raster_dwg_wire.h"
#include "drw_objects.h"

// Shared decoder for the separately bounded AC1024 and AC1027 no-reactor
// Normal/PLACEHOLDER profiles. Frame CRC and class binding are checked by
// the reader and adapter, respectively.
namespace dx_plotstyle_wire_detail {
inline bool matches(const DRW_UnsupportedObject& raw, bool dictionary,
                    std::uint32_t owner, std::uint32_t placeholder,
                    DRW::Version version) noexcept {
    using namespace dx_raster_wire::detail;
    if (raw.m_version != version || raw.m_isEntity
        || raw.m_handle == 0u || owner == 0u || placeholder == 0u
        || raw.m_parentHandle != owner || raw.m_numReactors != 0
        || !raw.m_reactorHandles.empty() || raw.m_xDictFlag != 1u
        || raw.m_hasDataStorage || raw.m_rawBytes.empty()
        || raw.m_rawBytes.size() > 1024u || raw.m_bodyBitSize == 0u)
        return false;
    const auto total = static_cast<std::uint32_t>(raw.m_rawBytes.size() * 8u);
    if (raw.m_bodyBitSize >= total) return false;
    const std::uint32_t end = total - raw.m_bodyBitSize;
    Bits body{raw.m_rawBytes.data(), 0u, end};
    const auto tag = body.take(2u);
    const auto type = static_cast<std::uint32_t>(tag == 0u ? body.take(8u)
        : tag == 1u ? body.take(8u) + 0x1f0u : body.little(2u));
    if (!body.good || type != static_cast<std::uint32_t>(raw.m_objectType)
        || (dictionary ? type < 500u || !raw.m_isCustomClass
                       : type != 80u || raw.m_isCustomClass)) return false;
    const auto self = handle(body, 0u, 0u, true);
    if (!body.good || self.value != raw.m_handle || body.shortValue() != 0u
        || body.longValue() != 0u || body.take(1u) != 1u
        || (version > DRW::AC1024 && body.take(1u) != 0u)
        || !body.good) return false;
    if (dictionary) {
        if (body.longValue() != 1u || body.shortValue() != 1u
            || body.take(8u) != 0u || !body.good || end < 17u)
            return false;
        Bits footer{raw.m_rawBytes.data(), end - 17u, end};
        const auto size = static_cast<std::uint32_t>(footer.little(2u));
        if (footer.take(1u) != 1u || !footer.good
            || (size != 106u && size != 131u
                && !(version == DRW::AC1024 && size == 124u))
            || size > end - 17u || end - 17u - size != body.pos)
            return false;
        Bits text{raw.m_rawBytes.data(), body.pos, end - 17u};
        const auto length = text.shortValue();
        if (length != (size == 106u ? 6u : 7u)) return false;
        for (const char c : { 'N', 'o', 'r', 'm', 'a', 'l' })
            if (text.little(2u) != static_cast<unsigned char>(c)) return false;
        if (length == 7u && text.little(2u) != 0u) return false;
        const auto tail = text.end - text.pos;
        if ((size == 106u ? tail != 0u : size == 124u ? tail != 2u : tail != 9u)
            || text.take(tail) != 0u || !text.good) return false;
    } else {
        const auto remaining = body.end - body.pos;
        if ((remaining != 1u && remaining != 24u
                && !(version == DRW::AC1024 && remaining == 25u))
            || body.take(remaining) != 0u || !body.good) return false;
    }
    Bits handles{raw.m_rawBytes.data(), end, total};
    const auto ownerToken = handle(handles, 4u, raw.m_handle);
    if (!handles.good || ownerToken.role != 4u || ownerToken.value != owner)
        return false;
    if (dictionary) {
        const auto entry = handle(handles, 2u, raw.m_handle);
        const auto def = handle(handles, 5u, raw.m_handle);
        if (!handles.good || entry.role != 2u || entry.value != placeholder
            || def.role != 5u || def.value != placeholder) return false;
    }
    const auto tail = handles.end - handles.pos;
    return tail < 8u && handles.take(tail) == 0u && handles.good;
}
} // namespace dx_plotstyle_wire_detail

namespace dx_ac1027_plotstyle_wire {
inline bool matches(const DRW_UnsupportedObject& raw, bool dictionary,
                    std::uint32_t owner, std::uint32_t placeholder) noexcept {
    return dx_plotstyle_wire_detail::matches(
        raw, dictionary, owner, placeholder, DRW::AC1027);
}
} // namespace dx_ac1027_plotstyle_wire

namespace dx_ac1024_plotstyle_wire {
inline bool matches(const DRW_UnsupportedObject& raw, bool dictionary,
                    std::uint32_t owner, std::uint32_t placeholder) noexcept {
    return dx_plotstyle_wire_detail::matches(
        raw, dictionary, owner, placeholder, DRW::AC1024);
}
} // namespace dx_ac1024_plotstyle_wire
#endif
