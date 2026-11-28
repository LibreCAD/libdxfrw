// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1024_DICTIONARYVAR_WIRE_H
#define DX_AC1024_DICTIONARYVAR_WIRE_H

#include "dx_layerstates_dwg_wire.h"
#include <array>
#include <cstdint>
#include <string_view>

// Exact source-wire observations for the pinned aerial AC1024 0x66 graph.
// They do not prove same-import provenance, class identity, semantic targets,
// NOD membership, staged output, or other DICTIONARYVAR profiles.
namespace dx_ac1024_dictionaryvar_wire {
struct Entry {
    const char* name;
    std::uint32_t handle;
    const char* value;
    std::uint32_t bytes;
    std::uint8_t ownerRole, ownerLength, selfLength;
};
constexpr std::array<Entry, 7> entries{{
    {"CANNOSCALE", 0x162u, "1:1", 21u, 4u, 1u, 2u},
    {"CMLEADERSTYLE", 0x161u, "STANDARD", 31u, 4u, 1u, 2u},
    {"CTABLESTYLE", 0x89u, "STANDARD", 30u, 12u, 1u, 1u},
    {"DIMASSOC", 0x67u, "2", 15u, 8u, 0u, 1u},
    {"HIDETEXT", 0x6bu, "1", 16u, 12u, 1u, 1u},
    {"LAYEREVAL", 0x312u, "1", 17u, 4u, 1u, 2u},
    {"LAYERNOTIFY", 0x313u, "15", 19u, 4u, 1u, 2u}
}};
inline const Entry* entryFor(std::uint32_t handle) noexcept {
    for (const auto& entry : entries) if (entry.handle == handle) return &entry;
    return nullptr;
}
inline bool matchesProfile(const DRW_Dictionary& source) noexcept {
    if (source.tType != DRW::DICTIONARY || source.handle != 0x66u
        || source.parentHandle != 0xCu || !source.name.empty() || source.flags != 0u
        || source.reactorCount() != 1 || source.extensionDictionaryFlag() != 1u
        || source.reactorHandles.size() != 1u || source.reactorHandles.front() != 0xCu
        || source.xDictHandle != 0u || source.hasDataStorageBinaryData()
        || !source.appData.empty() || !source.extData.empty() || source.countCap
        || !source.hasDwgCommonLinkTail() || !source.hasCompleteDwgEntries()
        || !source.pendingAppIdResolutions.empty() || !source.pendingLayerRefResolutions.empty()
        || source.cloning != 1 || source.hardOwner != 0 || source.m_entries.size() != entries.size())
        return false;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& actual = source.m_entries[i];
        if (actual.m_name != entries[i].name || actual.m_handle != entries[i].handle
            || actual.m_dxfHandleCode != DRW_Dictionary::Entry::DxfHandleCode::Unspecified)
            return false;
    }
    return true;
}
inline bool matchesProfile(const DRW_DictionaryVar& source) noexcept {
    const Entry* entry = entryFor(source.handle);
    return entry && source.tType == DRW::DICTIONARYVAR && source.parentHandle == 0x66u
        && source.name.empty() && source.flags == 0u && source.m_schema == 0
        && source.m_value == entry->value && source.reactorCount() == 1
        && source.extensionDictionaryFlag() == 1u && source.reactorHandles.size() == 1u
        && source.reactorHandles.front() == 0x66u && source.xDictHandle == 0u
        && !source.hasDataStorageBinaryData() && source.hasDwgCommonLinkTail()
        && source.appData.empty() && source.extData.empty()
        && source.pendingAppIdResolutions.empty() && source.pendingLayerRefResolutions.empty();
}
inline bool observeRoot(const DRW_UnsupportedObject& raw,
                        const DRW_Dictionary& source) noexcept {
    using dx_layerstates_dwg_wire::LinkKind;
    if (!matchesProfile(source) || raw.m_objectSize != 182u
        || raw.m_rawBytes.size() != 182u || raw.m_bodyBitSize != 182u
        || (raw.m_rawBytes.back() & 0x3fu) != 0x3fu) return false;
    return dx_layerstates_dwg_wire::observeDictionary(raw, source,
        [](std::uint32_t, const std::string&) noexcept { return false; },
        [](LinkKind kind, std::size_t index, std::uint8_t role,
           std::uint8_t length) noexcept {
            if (kind == LinkKind::Owner || kind == LinkKind::Reactor)
                return index == 0u && role == 4u && length == 1u;
            if (kind != LinkKind::Item || index >= entries.size() || role != 2u)
                return false;
            return length == (entries[index].handle > 0xffu ? 2u : 1u);
        });
}
inline bool observeVariable(const DRW_UnsupportedObject& raw,
                            const DRW_DictionaryVar& source) noexcept {
    using namespace dx_raster_wire;
    const Entry* entry = entryFor(source.handle);
    if (!matchesProfile(source) || !entry || raw.m_version != DRW::AC1024
        || raw.m_objectType != 503 || raw.m_handle != source.handle
        || raw.m_isEntity || !raw.m_isCustomClass || !raw.m_hasClassDefinition
        || raw.m_recordName != "DICTIONARYVAR" || raw.m_className != "AcDbDictionaryVar"
        || raw.m_parentHandle != 0x66u || raw.m_numReactors != 1
        || raw.m_reactorHandles.size() != 1u || raw.m_reactorHandles.front() != 0x66u
        || raw.m_xDictFlag != 1u || raw.m_xDictHandle != 0u || raw.m_hasDataStorage
        || !raw.m_commonHandleDataValidated
        || !drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence)
        || raw.m_objectSize != entry->bytes || raw.m_rawBytes.size() != entry->bytes
        || raw.m_bodyBitSize != (source.handle == 0x67u ? 30u : 38u)
        || (raw.m_rawBytes.back() & 0x3fu) != 0x3fu) return false;
    const auto total = static_cast<std::uint32_t>(entry->bytes * 8u);
    const auto bodyEnd = total - raw.m_bodyBitSize;
    detail::Bits body{raw.m_rawBytes.data(), 0u, bodyEnd};
    if (body.take(2u) != 1u || body.take(8u) + 0x1f0u != 503u) return false;
    const auto self = detail::handle(body, 0u, 0u, true);
    if (!body.good || self.role != 0u || self.length != entry->selfLength
        || self.value != source.handle || body.shortValue() != 0u
        || body.longValue() != 1u || body.take(1u) != 1u || body.take(8u) != 0u)
        return false;
    const std::string_view text(entry->value);
    const auto textBits = static_cast<std::uint32_t>(10u + 16u * text.size());
    if (!body.good || bodyEnd < 17u || textBits > bodyEnd - 17u
        || body.pos != bodyEnd - 17u - textBits
        || body.shortValue() != text.size()) return false;
    for (unsigned char c : text) if (body.little(2u) != c) return false;
    if (!body.good || body.pos != bodyEnd - 17u || body.little(2u) != textBits
        || body.take(1u) != 1u || !body.good || body.pos != bodyEnd) return false;
    detail::Bits links{raw.m_rawBytes.data(), bodyEnd, total};
    const auto owner = detail::handle(links, 4u, source.handle);
    const auto reactor = detail::handle(links, 4u, source.handle);
    return links.good && owner.role == entry->ownerRole && owner.length == entry->ownerLength
        && owner.value == 0x66u && reactor.role == 4u && reactor.length == 1u
        && reactor.value == 0x66u && links.take(6u) == 63u && links.good
        && links.pos == links.end;
}
} // namespace dx_ac1024_dictionaryvar_wire
#endif
