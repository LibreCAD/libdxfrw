// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1014_DICTIONARYVAR_WIRE_H
#define DX_AC1014_DICTIONARYVAR_WIRE_H

#include "dx_legacy_dwg_wire.h"

// Bounded R14 DICTIONARYVAR witness (ODA v5.4.1 section 20.4.74).
namespace dx_ac1014_dictionaryvar_wire {
using namespace dx_legacy_dwg_wire;

inline bool token(Bits& bits, unsigned role, unsigned length,
                  std::uint32_t base, std::uint32_t value,
                  bool own = false) noexcept {
    const auto h = handle(bits, own ? 0u : (role == 8u ? 4u : role), base, own);
    return bits.good && h.role == role && h.length == length && h.value == value;
}

inline bool variable(const DRW_UnsupportedObject& raw,
                     const DRW_DictionaryVar& model) noexcept {
    if (raw.m_version != DRW::AC1014 || !metadata(raw, 0x23u, 500u, 17u) || !raw.m_isCustomClass
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
