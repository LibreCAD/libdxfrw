// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1014_DICTIONARYVAR_WIRE_H
#define DX_AC1014_DICTIONARYVAR_WIRE_H

#include "drw_objects.h"
#include "dx_raster_dwg_wire.h"
#include <cstdint>
#include <string_view>

// Exact R14 source frames observed in LibreDWG's test/test-data/r14/v.dwg.
// The caller must also bind each frame to one import, its typed model, the
// complete graph and staged output. ODA v5.4.1 sections 20.4.44 and 20.4.74.
namespace dx_ac1014_dictionaryvar_wire {
using dx_raster_wire::detail::Bits;
using dx_raster_wire::detail::handle;

inline bool token(Bits& bits, unsigned role, unsigned length,
                  std::uint32_t base, std::uint32_t value,
                  bool own = false) noexcept {
    const auto h = handle(bits, own ? 0u : (role == 8u ? 4u : role), base, own);
    return bits.good && h.role == role && h.length == length && h.value == value;
}

inline bool text(Bits& bits, std::string_view expected) noexcept {
    if (bits.shortValue() != expected.size()) return false;
    for (unsigned char c : expected)
        if (bits.take(8u) != c) return false;
    return bits.good;
}

inline bool metadata(const DRW_UnsupportedObject& raw, std::uint32_t id,
                     int type, std::size_t size) noexcept {
    return raw.m_version == DRW::AC1014 && raw.m_handle == id
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
    const auto id = nod ? 0xcu : (mlineOwner ? 0x1au : 0x22u);
    const auto size = nod ? 69u : (mlineOwner ? 28u : 27u);
    const auto bodyEnd = nod ? 486u : (mlineOwner ? 162u : 154u);
    const auto count = nod ? 3u : 1u;
    if (!metadata(raw, id, 42u, size) || raw.m_isCustomClass
        || raw.m_hasClassDefinition || model.tType != DRW::DICTIONARY
        || raw.m_parentHandle != (nod ? 0u : 0xcu)
        || raw.m_numReactors != (nod ? 0 : 1)
        || raw.m_reactorHandles != model.reactorHandles
        || model.handle != id || model.parentHandle != (nod ? 0u : 0xcu)
        || !model.name.empty() || model.flags != 0u || model.hardOwner != 0
        || model.m_entries.size() != count || !model.hasCompleteDwgEntries()
        || !model.hasDwgCommonLinkTail() || model.countCap
        || model.xDictHandle != 0u || model.hasDataStorageBinaryData()
        || !model.appData.empty() || !model.extData.empty()
        || !model.pendingAppIdResolutions.empty()
        || !model.pendingLayerRefResolutions.empty()
        || model.reactorCount() != (nod ? 0 : 1)
        || model.reactorHandles.size() != (nod ? 0u : 1u)
        || (!nod && model.reactorHandles.front() != 0xcu)) return false;
    constexpr std::string_view keys[]{"ACAD_GROUP", "ACAD_MLINESTYLE",
                                      "ACDBVARIABLEDICTIONARY"};
    constexpr std::uint32_t targets[]{0xdu, 0x1au, 0x22u};
    for (std::size_t i = 0; i < count; ++i) {
        const auto& entry = model.m_entries[i];
        if (entry.m_name != (nod ? keys[i] : (mlineOwner
                ? std::string_view("STANDARD") : std::string_view("DIMADEC")))
            || entry.m_handle != (nod ? targets[i] : (mlineOwner ? 0x1bu : 0x23u))
            || entry.m_dxfHandleCode
                != DRW_Dictionary::Entry::DxfHandleCode::Unspecified) return false;
    }
    Bits bits{raw.m_rawBytes.data(), 0u, static_cast<std::uint32_t>(size * 8u)};
    if (bits.shortValue() != 42u || !token(bits, 0u, 1u, 0u, id, true)
        || bits.shortValue() != 0u || bits.little(4u) != bodyEnd
        || bits.longValue() != (nod ? 0u : 1u) || bits.longValue() != count
        || bits.take(8u) != 0u) return false;
    for (std::size_t i = 0; i < count; ++i)
        if (!text(bits, nod ? keys[i] : (mlineOwner
                ? std::string_view("STANDARD") : std::string_view("DIMADEC")))) return false;
    if (bits.pos != bodyEnd
        || !token(bits, 4u, nod ? 0u : 1u, id, nod ? 0u : 0xcu)
        || (!nod && !token(bits, 4u, 1u, id, 0xcu))
        || !token(bits, 3u, 0u, id, 0u)) return false;
    for (std::size_t i = 0; i < count; ++i)
        if (!token(bits, 2u, 1u, id, nod ? targets[i] : (mlineOwner ? 0x1bu : 0x23u))) return false;
    return bits.take(nod ? 2u : 6u) == (nod ? 2u : (mlineOwner ? 0u : 6u))
        && bits.good && bits.pos == bits.end;
}

inline bool variable(const DRW_UnsupportedObject& raw,
                     const DRW_DictionaryVar& model) noexcept {
    if (!metadata(raw, 0x23u, 500u, 17u) || !raw.m_isCustomClass
        || !raw.m_hasClassDefinition || raw.m_recordName != "DICTIONARYVAR"
        || raw.m_className != "AcDbDictionaryVar"
        || raw.m_parentHandle != 0x22u || raw.m_numReactors != 1
        || raw.m_reactorHandles != model.reactorHandles
        || model.tType != DRW::DICTIONARYVAR || model.handle != 0x23u
        || model.parentHandle != 0x22u || !model.name.empty() || model.flags != 0u
        || model.m_schema != 0 || model.m_value != "0"
        || model.reactorCount() != 1 || model.reactorHandles.size() != 1u
        || model.reactorHandles.front() != 0x22u || model.xDictHandle != 0u
        || !model.hasDwgCommonLinkTail() || model.hasDataStorageBinaryData()
        || !model.appData.empty() || !model.extData.empty()
        || !model.pendingAppIdResolutions.empty()
        || !model.pendingLayerRefResolutions.empty()) return false;
    Bits bits{raw.m_rawBytes.data(), 0u, 136u};
    return bits.shortValue() == 500u && token(bits, 0u, 1u, 0u, 0x23u, true)
        && bits.shortValue() == 0u && bits.little(4u) == 104u
        && bits.longValue() == 1u && bits.take(8u) == 0u && text(bits, "0")
        && bits.pos == 104u && token(bits, 8u, 0u, 0x23u, 0x22u)
        && token(bits, 4u, 1u, 0x23u, 0x22u)
        && token(bits, 3u, 0u, 0x23u, 0u)
        && bits.good && bits.pos == bits.end;
}
} // namespace dx_ac1014_dictionaryvar_wire
#endif
