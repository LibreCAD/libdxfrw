// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_LEGACY_DWG_WIRE_H
#define DX_LEGACY_DWG_WIRE_H

#include "drw_objects.h"
#include "dx_raster_dwg_wire.h"
#include <algorithm>
#include <cstdint>
#include <string_view>

// Shared bounded R13/R14 object framing and dictionary observer. Callers
// must additionally bind import identity, the complete graph and output.
// ODA v5.4.1 sections 20.1 and 20.4.44; only R14 has the extra dictionary RC.
namespace dx_legacy_dwg_wire {
using dx_raster_wire::detail::Bits;
using dx_raster_wire::detail::handle;
inline bool supported(DRW::Version version) noexcept {
    return version == DRW::AC1012 || version == DRW::AC1014;
}

inline bool text(Bits& bits, std::string_view expected) noexcept {
    const auto length = bits.shortValue();
    if (length != expected.size() && length != expected.size() + 1u) return false;
    for (unsigned char c : expected)
        if (bits.take(8u) != c) return false;
    if (length != expected.size() && bits.take(8u) != 0u) return false;
    return bits.good;
}

// DWG object alignment bits have no specified value. Still require that the
// complete remaining suffix is just the final, bounded byte alignment.
inline bool padding(Bits& bits) noexcept {
    return bits.good && bits.pos <= bits.end && bits.end - bits.pos < 8u
        && bits.skip(bits.end - bits.pos);
}

inline bool reference(Bits& bits, unsigned role, std::uint32_t base,
                      std::uint32_t expected, bool own = false) noexcept {
    const auto h = handle(bits, role, base, own);
    return bits.good && h.value == expected && (expected != 0u || h.role == role);
}

inline bool metadata(const DRW_UnsupportedObject& raw, std::uint32_t id,
                     int type, std::size_t size) noexcept {
    return supported(raw.m_version) && raw.m_handle == id
        && raw.m_objectType == type && !raw.m_isEntity
        && raw.m_objectSize == size && raw.m_rawBytes.size() == size
        && raw.m_bodyBitSize == 0u && raw.m_commonHandleDataValidated
        && drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence)
        && !raw.m_hasDataStorage && raw.m_xDictHandle == 0u;
}

inline bool dictionary(const DRW_UnsupportedObject& raw,
                       const DRW_Dictionary& model, bool nod,
                       bool mlineOwner = false) noexcept {
    if (nod && mlineOwner) return false;
    const auto id = model.handle;
    const auto size = raw.m_rawBytes.size();
    const auto count = model.m_entries.size();
    if (!metadata(raw, id, 42u, size) || raw.m_isCustomClass
        || raw.m_hasClassDefinition || model.tType != DRW::DICTIONARY
        || id == 0u || size > UINT32_MAX / 8u || count > 64u
        || raw.m_parentHandle != model.parentHandle
        || raw.m_numReactors != model.reactorCount()
        || raw.m_reactorHandles != model.reactorHandles
        || !model.name.empty() || model.flags != 0u || model.hardOwner != 0
        || model.cloning != 0 || !model.hasCompleteDwgEntries()
        || !model.hasDwgCommonLinkTail() || model.countCap
        || model.xDictHandle != 0u || model.hasDataStorageBinaryData()
        || !model.appData.empty() || !model.extData.empty()
        || !model.pendingAppIdResolutions.empty()
        || !model.pendingLayerRefResolutions.empty()
        || model.reactorCount() < 0
        || model.reactorHandles.size() != static_cast<std::size_t>(model.reactorCount())
        || (nod && (model.parentHandle != 0u || !model.reactorHandles.empty()))) return false;
    for (const auto& entry : model.m_entries)
        if (entry.m_name.empty() || entry.m_handle == 0u
            || entry.m_dxfHandleCode != DRW_Dictionary::Entry::DxfHandleCode::Unspecified
            || !std::all_of(entry.m_name.begin(), entry.m_name.end(),
                            [](unsigned char c) { return c > 0u && c < 128u; })) return false;
    Bits bits{raw.m_rawBytes.data(), 0u, static_cast<std::uint32_t>(size * 8u)};
    if (bits.shortValue() != 42u || !reference(bits, 0u, 0u, id, true)
        || bits.shortValue() != 0u) return false;
    const auto bodyEnd = bits.little(4u);
    if (bits.longValue() != model.reactorHandles.size() || bits.longValue() != count
        || (raw.m_version == DRW::AC1014 && bits.take(8u) != 0u)) return false;
    for (const auto& entry : model.m_entries)
        if (!text(bits, entry.m_name)) return false;
    if (bits.pos != bodyEnd || !reference(bits, 4u, id, model.parentHandle)) return false;
    for (const auto reactor : model.reactorHandles)
        if (!reference(bits, 4u, id, reactor)) return false;
    if (!reference(bits, 3u, id, 0u)) return false;
    for (const auto& entry : model.m_entries)
        if (!reference(bits, 2u, id, entry.m_handle)) return false;
    return padding(bits);
}

} // namespace dx_legacy_dwg_wire
#endif
