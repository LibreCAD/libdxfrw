// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1024_MLINESTYLE_WIRE_H
#define DX_AC1024_MLINESTYLE_WIRE_H

#include "dx_layerstates_dwg_wire.h"
#include <cstdint>
#include <cstring>

// Exact AC1024 C -> 17 -> 18 source-wire observation, based on the two
// byte-identical physical witnesses. It does not establish source identity,
// NOD membership, output closure, or general MLINESTYLE support.
namespace dx_ac1024_mlinestyle_wire {
inline bool observeRoot(const DRW_UnsupportedObject& raw,
                        const DRW_Dictionary& source) noexcept {
    using dx_layerstates_dwg_wire::LinkKind;
    if (source.handle != 0x17u || source.parentHandle != 0xCu
        || source.flags != 0u || !source.name.empty() || source.cloning != 1
        || source.hardOwner != 0 || source.reactorCount() != 1
        || source.extensionDictionaryFlag() != 1u
        || source.reactorHandles.size() != 1u || source.reactorHandles.front() != 0xCu
        || source.m_entries.size() != 1u || source.m_entries.front().m_name != "Standard"
        || source.m_entries.front().m_handle != 0x18u
        || source.m_entries.front().m_dxfHandleCode
            != DRW_Dictionary::Entry::DxfHandleCode::Unspecified
        || !source.extData.empty()
        || raw.m_objectSize != 34u || raw.m_rawBytes.size() != 34u
        || raw.m_bodyBitSize != 50u || (raw.m_rawBytes.back() & 3u) != 3u)
        return false;
    return dx_layerstates_dwg_wire::observeDictionary(raw, source,
        [](std::uint32_t, const std::string&) noexcept { return false; },
        [](LinkKind kind, std::size_t index, std::uint8_t role,
           std::uint8_t length) noexcept {
            return index == 0u && length == 1u
                && ((kind == LinkKind::Owner && role == 12u)
                    || (kind == LinkKind::Reactor && role == 4u)
                    || (kind == LinkKind::Item && role == 2u));
        });
}

inline bool observeStyle(const DRW_UnsupportedObject& raw,
                         const DRW_MLineStyle& source) noexcept {
    using namespace dx_raster_wire;
    if (raw.m_version != DRW::AC1024 || raw.m_objectType != 73
        || raw.m_handle != 0x18u || raw.m_isEntity || raw.m_isCustomClass
        || raw.m_hasClassDefinition || !raw.m_recordName.empty()
        || !raw.m_className.empty() || !raw.m_classAppName.empty()
        || raw.m_objectSize != 83u || raw.m_rawBytes.size() != 83u
        || raw.m_bodyBitSize != 26u || raw.m_hasDataStorage
        || !raw.m_commonHandleDataValidated
        || !drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence)
        || raw.m_parentHandle != 0x17u || raw.m_numReactors != 1
        || raw.m_reactorHandles.size() != 1u || raw.m_reactorHandles.front() != 0x17u
        || raw.m_xDictFlag != 1u || raw.m_xDictHandle != 0u
        || source.tType != DRW::MLINESTYLE || source.handle != 0x18u
        || source.parentHandle != 0x17u
        || source.name != "STANDARD" || !source.description.empty() || source.flags != 0
        || source.reactorCount() != 1 || source.extensionDictionaryFlag() != 1u
        || source.reactorHandles.size() != 1u || source.reactorHandles.front() != 0x17u
        || source.xDictHandle != 0u || source.hasDataStorageBinaryData()
        || !source.appData.empty() || !source.extData.empty()
        || !source.pendingAppIdResolutions.empty() || !source.pendingLayerRefResolutions.empty()
        || source.fillColor != 256 || source.fillColor24 != -1
        || source.fillColorMethod != 192u || source.fillColorFlags != 0u
        || source.fillColorDwgIndex != 0u || !source.fillColorName.empty()
        || !source.fillColorBookName.empty() || source.startAngle != 90.0
        || source.endAngle != 90.0 || source.elements.size() != 2u)
        return false;
    for (std::size_t i = 0; i < source.elements.size(); ++i) {
        const auto& element = source.elements[i];
        if (element.offset != (i == 0u ? 0.5 : -0.5) || element.color != 256
            || element.color24 != -1 || element.colorMethod != 192u
            || element.colorFlags != 0u || element.colorDwgIndex != 0u
            || !element.colorName.empty() || !element.colorBookName.empty()
            || !element.linetype.empty() || element.linetypeIndex != 32767
            || element.linetypeHandle != 0u) return false;
    }
    detail::Bits body{raw.m_rawBytes.data(), 0u, 481u};
    if (body.take(2u) != 0u || body.take(8u) != 73u) return false;
    const auto self = detail::handle(body, 0u, 0u, true);
    if (!body.good || self.role != 0u || self.length != 1u || self.value != 0x18u
        || body.shortValue() != 0u || body.longValue() != 1u
        || body.take(1u) != 1u || body.shortValue() != 0u) return false;
    const auto color = [&body]() noexcept {
        return body.shortValue() == 0u && body.longValue() == 0xc0000000u
            && body.take(8u) == 0u && body.good;
    };
    const auto fullDouble = [&body](std::uint64_t bits) noexcept {
        return body.take(2u) == 0u && body.little(8u) == bits && body.good;
    };
    if (!color() || !fullDouble(0x3ff921fb54442d18ULL)
        || !fullDouble(0x3ff921fb54442d18ULL) || body.take(8u) != 2u
        || !fullDouble(0x3fe0000000000000ULL) || !color()
        || body.shortValue() != 32767u
        || !fullDouble(0xbfe0000000000000ULL) || !color()
        || body.shortValue() != 32767u || !body.good || body.pos != body.end)
        return false;
    detail::Bits strings{raw.m_rawBytes.data(), 481u, 621u};
    if (strings.shortValue() != 8u) return false;
    for (const char c : {'S','T','A','N','D','A','R','D'})
        if (strings.little(2u) != static_cast<unsigned char>(c)) return false;
    if (strings.shortValue() != 0u || !strings.good || strings.pos != strings.end)
        return false;
    detail::Bits footer{raw.m_rawBytes.data(), 621u, 638u};
    if (footer.little(2u) != 140u || footer.take(1u) != 1u || !footer.good)
        return false;
    detail::Bits links{raw.m_rawBytes.data(), 638u, 664u};
    const auto owner = detail::handle(links, 4u, 0x18u);
    const auto reactor = detail::handle(links, 4u, 0x18u);
    return links.good && owner.role == 8u && owner.length == 0u && owner.value == 0x17u
        && reactor.role == 4u && reactor.length == 1u && reactor.value == 0x17u
        && links.take(2u) == 3u && links.good && links.pos == links.end;
}
} // namespace dx_ac1024_mlinestyle_wire
#endif
