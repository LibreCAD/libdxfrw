// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1027_LAYER_WIRE_H
#define DX_AC1027_LAYER_WIRE_H
#include "dx_raster_dwg_wire.h"
#include "drw_objects.h"
// Exact CRC-validated AC1027 layer-0 source profile. The CRC and source-map
// publication are separate evidence supplied by the table-frame reader.
namespace dx_ac1027_layer_wire {
inline bool matches(const DRW_UnsupportedObject& raw, const DRW_Layer& typed,
                    std::uint32_t plot, std::uint32_t material) noexcept {
    using namespace dx_raster_wire::detail;
    if (raw.m_version != DRW::AC1027 || raw.m_objectType != 51 || raw.m_isEntity
        || raw.m_isCustomClass || raw.m_handle != typed.handle || raw.m_parentHandle != typed.parentHandle
        || raw.m_numReactors != 0 || !raw.m_reactorHandles.empty() || raw.m_xDictFlag != 1u
        || raw.m_hasDataStorage || raw.m_rawBytes.empty() || raw.m_rawBytes.size() > 1024u
        || raw.m_bodyBitSize == 0u || typed.name != "0" || !plot || !material
        || typed.unknownHandle.code != 5u || typed.unknownHandle.size != 0u
        || typed.unknownHandle.ref != 0u || typed.unknownHandle.ref64 != 0u)
        return false;
    const auto total = static_cast<std::uint32_t>(raw.m_rawBytes.size() * 8u);
    if (raw.m_bodyBitSize >= total) return false;
    const auto end = total - raw.m_bodyBitSize;
    Bits body{raw.m_rawBytes.data(), 0u, end};
    const auto tag = body.take(2u);
    const auto type = static_cast<std::uint32_t>(tag == 0u ? body.take(8u)
        : tag == 1u ? body.take(8u) + 0x1f0u : body.little(2u));
    const auto self = handle(body, 0u, 0u, true);
    if (!body.good || type != 51u || self.value != raw.m_handle
        || body.shortValue() != 0u || body.longValue() != 0u
        || body.take(1u) != 1u || body.take(1u) != 0u
        || body.shortValue() != 0u || body.shortValue() != 1008u
        || body.shortValue() != 0u || body.longValue() != 0xc3000007u
        || body.take(8u) != 0u || !body.good || end < 17u) return false;
    Bits footer{raw.m_rawBytes.data(), end - 17u, end};
    const auto size = static_cast<std::uint32_t>(footer.little(2u));
    if (footer.take(1u) != 1u || !footer.good || (size != 26u && size != 42u)
        || size > end - 17u || end - 17u - size != body.pos) return false;
    Bits text{raw.m_rawBytes.data(), body.pos, end - 17u};
    if (text.shortValue() != (size == 26u ? 1u : 2u) || text.little(2u) != '0'
        || (size == 42u && text.little(2u) != 0u)
        || !text.good || text.pos != text.end) return false;
    Bits handles{raw.m_rawBytes.data(), end, total};
    const auto owner = handle(handles, 4u, raw.m_handle);
    const auto xref = handle(handles, 5u, raw.m_handle);
    const auto plotToken = handle(handles, 5u, raw.m_handle);
    const auto matToken = handle(handles, 5u, raw.m_handle);
    const auto ltype = handle(handles, 5u, raw.m_handle);
    const auto unknown = handle(handles, 5u, raw.m_handle);
    if (!handles.good || owner.role != 4u || owner.value != raw.m_parentHandle
        || xref.role != 5u || xref.length != 0u || xref.value != 0u
        || plotToken.role != 5u || plotToken.value != plot
        || matToken.role != 5u || matToken.value != material
        || ltype.role != 5u || ltype.value != typed.lTypeH.ref
        || unknown.role != 5u || unknown.length != 0u || unknown.value != 0u)
        return false;
    const auto tail = handles.end - handles.pos;
    return tail < 8u && handles.take(tail) == 0u && handles.good;
}
} // namespace dx_ac1027_layer_wire
#endif
