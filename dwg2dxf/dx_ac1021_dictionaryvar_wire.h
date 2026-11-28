// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1021_DICTIONARYVAR_WIRE_H
#define DX_AC1021_DICTIONARYVAR_WIRE_H

#include "dx_layerstates_dwg_wire.h"
#include <cstdint>

// Source-only observer for the local AC1021 CTABLESTYLE variable profile.
// The caller must independently prove same-import class and graph identity.
// This observer does not authorize cross-version output or prove TABLESTYLE
// semantics. ODA DWG v5.4.1 sections 20.1 and 20.4.74.
namespace dx_ac1021_dictionaryvar_wire {
inline bool matchesOwner(const DRW_Dictionary& source) noexcept {
    return source.tType == DRW::DICTIONARY && source.handle == 0x5abu
        && source.parentHandle == 0xcu && source.name.empty() && source.flags == 0u
        && source.reactorCount() == 1 && source.extensionDictionaryFlag() == 1u
        && source.reactorHandles.size() == 1u && source.reactorHandles.front() == 0xcu
        && source.xDictHandle == 0u && !source.hasDataStorageBinaryData()
        && source.hasDwgCommonLinkTail() && source.hasCompleteDwgEntries()
        && source.appData.empty() && source.extData.empty() && !source.countCap
        && source.pendingAppIdResolutions.empty() && source.pendingLayerRefResolutions.empty()
        && source.cloning == 1 && source.hardOwner == 0 && source.m_entries.size() == 1u
        && source.m_entries.front().m_name == "CTABLESTYLE"
        && source.m_entries.front().m_handle == 0x5acu
        && source.m_entries.front().m_dxfHandleCode == DRW_Dictionary::Entry::DxfHandleCode::Unspecified;
}
inline bool matchesVariable(const DRW_DictionaryVar& source) noexcept {
    return source.tType == DRW::DICTIONARYVAR && source.handle == 0x5acu
        && source.parentHandle == 0x5abu && source.name.empty() && source.flags == 0u
        && source.m_schema == 0 && source.m_value == "STANDARD"
        && source.reactorCount() == 1 && source.extensionDictionaryFlag() == 1u
        && source.reactorHandles.size() == 1u && source.reactorHandles.front() == 0x5abu
        && source.xDictHandle == 0u && !source.hasDataStorageBinaryData()
        && source.hasDwgCommonLinkTail() && source.appData.empty() && source.extData.empty()
        && source.pendingAppIdResolutions.empty() && source.pendingLayerRefResolutions.empty();
}
inline bool observeOwner(const DRW_UnsupportedObject& raw,
                         const DRW_Dictionary& source) noexcept {
    using dx_layerstates_dwg_wire::LinkKind;
    if (!matchesOwner(source) || raw.m_objectSize != 46u
        || raw.m_rawBytes.size() != 46u || raw.m_bodyBitSize != 0u
        || (raw.m_rawBytes.back() & 3u) != 3u) return false;
    return dx_layerstates_dwg_wire::observeAc1021Dictionary(raw, source,
        [](std::uint32_t, const std::string&) noexcept { return false; },
        [](LinkKind kind, std::size_t index, std::uint8_t role,
           std::uint8_t length) noexcept {
            if (kind == LinkKind::Owner || kind == LinkKind::Reactor)
                return index == 0u && role == 4u && length == 1u;
            return kind == LinkKind::Item && index == 0u && role == 2u && length == 2u;
        });
}
inline bool observeVariable(const DRW_UnsupportedObject& raw,
                            const DRW_DictionaryVar& source) noexcept {
    using namespace dx_raster_wire;
    if (!matchesVariable(source) || raw.m_version != DRW::AC1021
        || raw.m_objectType != 504 || raw.m_handle != source.handle
        || raw.m_isEntity || !raw.m_isCustomClass || !raw.m_hasClassDefinition
        || raw.m_recordName != "DICTIONARYVAR" || raw.m_className != "AcDbDictionaryVar"
        || raw.m_parentHandle != 0x5abu || raw.m_numReactors != 1
        || raw.m_reactorHandles.size() != 1u || raw.m_reactorHandles.front() != 0x5abu
        || raw.m_xDictFlag != 1u || raw.m_xDictHandle != 0u || raw.m_hasDataStorage
        || !raw.m_commonHandleDataValidated
        || !drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence)
        || raw.m_objectSize != 36u || raw.m_rawBytes.size() != 36u
        || raw.m_bodyBitSize != 0u || (raw.m_rawBytes.back() & 63u) != 63u)
        return false;
    constexpr std::uint32_t total = 36u * 8u;
    detail::Bits body{raw.m_rawBytes.data(), 0u, total};
    if (body.shortValue() != 504u || body.little(4u) != 250u || !body.good)
        return false;
    constexpr std::uint32_t end = 250u;
    body.end = end;
    const auto self = detail::handle(body, 0u, 0u, true);
    if (!body.good || self.role != 0u || self.length != 2u
        || self.value != source.handle || body.shortValue() != 0u
        || body.longValue() != 1u || body.take(1u) != 1u
        || body.take(8u) != 0u || body.pos != 95u
        || body.shortValue() != 8u) return false;
    for (unsigned char c : source.m_value)
        if (c > 0x7fu || body.little(2u) != c) return false;
    if (!body.good || body.pos != 233u || body.little(2u) != 138u
        || body.take(1u) != 1u || !body.good || body.pos != end) return false;
    detail::Bits links{raw.m_rawBytes.data(), end, total};
    const auto owner = detail::handle(links, 4u, source.handle);
    const auto reactor = detail::handle(links, 4u, source.handle);
    return links.good && owner.role == 8u && owner.length == 0u
        && owner.value == 0x5abu && reactor.role == 4u && reactor.length == 2u
        && reactor.value == 0x5abu && links.take(6u) == 63u && links.good
        && links.pos == links.end;
}
} // namespace dx_ac1021_dictionaryvar_wire
#endif
