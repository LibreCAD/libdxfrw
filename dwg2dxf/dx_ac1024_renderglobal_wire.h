// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1024_RENDERGLOBAL_WIRE_H
#define DX_AC1024_RENDERGLOBAL_WIRE_H

#include "dx_layerstates_dwg_wire.h"
#include <cstdint>

// Exact source-wire observations for the pinned aerial AC1024 render-global
// graph. They do not prove import/class provenance, NOD membership, output
// order/closure, rendering semantics, or any other source profile.
namespace dx_ac1024_renderglobal_wire {
inline bool matchesProfile(const DRW_RenderSettings& source) noexcept {
    return source.tType == DRW::RENDERSETTINGS && source.m_kind == DRW_RenderSettings::Global
        && source.handle == 0x237u && source.parentHandle == 0xCu && source.name.empty()
        && source.flags == 0u && source.reactorCount() == 1
        && source.extensionDictionaryFlag() == 0u && source.xDictHandle == 0x251u
        && source.reactorHandles.size() == 1u && source.reactorHandles.front() == 0xCu
        && !source.hasDataStorageBinaryData() && source.appData.empty() && source.extData.empty()
        && source.pendingAppIdResolutions.empty() && source.pendingLayerRefResolutions.empty()
        && !source.hasDwgRawData() && source.dwgRawData().empty()
        && source.dwgRawDataBitSize() == 0u
        && source.dwgRawDataVersion() == DRW::UNKNOWNV
        && !source.hasDwgCommonLinkTail()
        && source.m_name.empty() && source.m_environmentImageFilename.empty()
        && source.m_description.empty() && source.m_displayIndex == 0
        && !source.m_fogEnabled && !source.m_fogBackgroundEnabled
        && !source.m_backfacesEnabled && !source.m_environmentImageEnabled
        && !source.m_hasPredefined && !source.m_hasPredefinedPresent
        && source.m_fogColorR == 0 && source.m_fogColorG == 0 && source.m_fogColorB == 0
        && source.m_fogDensityNear == 0.0 && source.m_fogDensityFar == 0.0
        && source.m_fogDistanceNear == 0.0 && source.m_fogDistanceFar == 0.0
        && source.m_classVersion == 2
        && source.m_procedure == 0 && source.m_destination == 0
        && source.m_longs.size() == 5u && source.m_longs[0] == 2
        && source.m_longs[1] == 0 && source.m_longs[2] == 0
        && source.m_longs[3] == 640 && source.m_longs[4] == 480
        && source.m_strings.size() == 1u && source.m_strings[0].empty()
        && source.m_bools.size() == 3u && !source.m_bools[0]
        && source.m_bools[1] && source.m_bools[2]
        && source.m_shorts.empty() && source.m_bytes.empty() && source.m_doubles.empty();
}
inline bool matchesProfile(const DRW_Dictionary& source) noexcept {
    return source.tType == DRW::DICTIONARY && source.handle == 0x251u
        && source.parentHandle == 0x237u && source.name.empty() && source.flags == 0u
        && source.reactorCount() == 0 && source.reactorHandles.empty()
        && source.extensionDictionaryFlag() == 1u && source.xDictHandle == 0u
        && !source.hasDataStorageBinaryData() && source.appData.empty() && source.extData.empty()
        && source.pendingAppIdResolutions.empty() && source.pendingLayerRefResolutions.empty()
        && !source.countCap && source.hasCompleteDwgEntries() && source.hasDwgCommonLinkTail()
        && source.cloning == 1 && source.hardOwner == 1 && source.m_entries.size() == 1u
        && source.m_entries.front().m_name == "ADSK_XREC_RENDERGLOBALINFO"
        && source.m_entries.front().m_handle == 0x252u
        && source.m_entries.front().m_dxfHandleCode == DRW_Dictionary::Entry::DxfHandleCode::Unspecified;
}
inline bool matchesProfile(const DRW_XRecord& source) noexcept {
    if (source.tType != DRW::XRECORD || source.handle != 0x252u
        || source.parentHandle != 0x251u || !source.name.empty() || source.flags != 0u
        || source.reactorCount() != 1 || source.reactorHandles.size() != 1u
        || source.reactorHandles.front() != 0x251u || source.extensionDictionaryFlag() != 1u
        || source.xDictHandle != 0u || source.hasDataStorageBinaryData()
        || !source.hasDwgCommonLinkTail() || !source.appData.empty() || !source.extData.empty()
        || !source.pendingAppIdResolutions.empty() || !source.pendingLayerRefResolutions.empty()
        || source.m_cloning != 1 || source.m_rawDataValid || !source.m_rawData.empty()
        || source.hasDwgRawDataSourceRecord() || !source.m_handleValues.empty()
        || source.m_values.size() != 1u || source.m_dataEntries.size() != 1u) return false;
    for (const DRW_Variant* value : {&source.m_values.front(), &source.m_dataEntries.front()})
        if (value->code() != 70 || value->type() != DRW_Variant::INTEGER || value->i_val() != 0)
            return false;
    return true;
}
inline bool observeGlobal(const DRW_UnsupportedObject& raw,
                          const DRW_RenderSettings& source) noexcept {
    using namespace dx_raster_wire;
    if (!matchesProfile(source) || raw.m_version != DRW::AC1024 || raw.m_objectType != 521
        || raw.m_handle != 0x237u || raw.m_isEntity || !raw.m_isCustomClass || !raw.m_hasClassDefinition
        || raw.m_recordName != "RENDERGLOBAL" || raw.m_className != "AcDbRenderGlobal"
        || raw.m_parentHandle != 0xCu || raw.m_numReactors != 1 || raw.m_reactorHandles.size() != 1u
        || raw.m_reactorHandles.front() != 0xCu || raw.m_xDictFlag != 0u || raw.m_xDictHandle != 0x251u
        || raw.m_hasDataStorage || !raw.m_commonHandleDataValidated
        || !drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence)
        || raw.m_objectSize != 26u || raw.m_rawBytes.size() != 26u || raw.m_bodyBitSize != 57u)
        return false;
    detail::Bits body{raw.m_rawBytes.data(), 0u, 132u};
    if (body.take(2u) != 1u || body.take(8u) + 0x1f0u != 521u) return false;
    const auto self = detail::handle(body, 0u, 0u, true);
    if (!body.good || self.role != 0u || self.length != 2u || self.value != 0x237u
        || body.shortValue() != 0u || body.longValue() != 1u || body.take(1u) != 0u
        || body.longValue() != 2u || body.longValue() != 0u || body.longValue() != 0u
        || body.take(1u) != 0u || body.longValue() != 640u || body.longValue() != 480u
        || body.take(1u) != 1u || body.take(1u) != 1u || !body.good || body.pos != body.end)
        return false;
    detail::Bits string{raw.m_rawBytes.data(), 132u, 151u};
    if (string.shortValue() != 0u || string.little(2u) != 2u || string.take(1u) != 1u
        || !string.good || string.pos != string.end) return false;
    detail::Bits links{raw.m_rawBytes.data(), 151u, 208u};
    const auto owner = detail::handle(links, 4u, source.handle);
    const auto reactor = detail::handle(links, 4u, source.handle);
    const auto extension = detail::handle(links, 3u, source.handle);
    return links.good && owner.role == 4u && owner.length == 1u && owner.value == 0xCu
        && reactor.role == 4u && reactor.length == 1u && reactor.value == 0xCu
        && extension.role == 3u && extension.length == 2u && extension.value == 0x251u
        && links.take(1u) == 1u && links.good && links.pos == links.end;
}
inline bool observeDictionary(const DRW_UnsupportedObject& raw,
                              const DRW_Dictionary& source) noexcept {
    using dx_layerstates_dwg_wire::LinkKind;
    if (!matchesProfile(source) || raw.m_objectSize != 69u || raw.m_rawBytes.size() != 69u
        || raw.m_bodyBitSize != 42u || (raw.m_rawBytes.back() & 3u) != 3u) return false;
    return dx_layerstates_dwg_wire::observeDictionary(raw, source,
        [](std::uint32_t, const std::string&) noexcept { return false; },
        [](LinkKind kind, std::size_t index, std::uint8_t role, std::uint8_t length) noexcept {
            return index == 0u && ((kind == LinkKind::Owner && role == 12u && length == 1u)
                || (kind == LinkKind::Item && role == 2u && length == 2u));
        });
}
inline bool observeXrecord(const DRW_UnsupportedObject& raw,
                           const DRW_XRecord& source) noexcept {
    using namespace dx_raster_wire;
    if (!matchesProfile(source) || raw.m_version != DRW::AC1024 || raw.m_objectType != 79
        || raw.m_handle != source.handle || raw.m_isEntity || raw.m_isCustomClass
        || raw.m_hasClassDefinition || !raw.m_recordName.empty() || !raw.m_className.empty()
        || !raw.m_classAppName.empty() || raw.m_parentHandle != 0x251u
        || raw.m_numReactors != 1 || raw.m_reactorHandles.size() != 1u
        || raw.m_reactorHandles.front() != 0x251u || raw.m_xDictFlag != 1u
        || raw.m_xDictHandle != 0u || raw.m_hasDataStorage || !raw.m_commonHandleDataValidated
        || !drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence)
        || raw.m_objectSize != 17u || raw.m_rawBytes.size() != 17u || raw.m_bodyBitSize != 36u)
        return false;
    detail::Bits body{raw.m_rawBytes.data(), 0u, 100u};
    if (body.take(2u) != 0u || body.take(8u) != 79u) return false;
    const auto self = detail::handle(body, 0u, 0u, true);
    if (!body.good || self.role != 0u || self.length != 2u || self.value != 0x252u
        || body.shortValue() != 0u || body.longValue() != 1u || body.take(1u) != 1u
        || body.longValue() != 4u || body.little(2u) != 70u || body.little(2u) != 0u
        || body.shortValue() != 1u || body.take(1u) != 0u || !body.good || body.pos != body.end)
        return false;
    detail::Bits links{raw.m_rawBytes.data(), 100u, 136u};
    const auto owner = detail::handle(links, 4u, source.handle);
    const auto reactor = detail::handle(links, 4u, source.handle);
    return links.good && owner.role == 8u && owner.length == 0u && owner.value == 0x251u
        && reactor.role == 4u && reactor.length == 2u && reactor.value == 0x251u
        && links.take(4u) == 15u && links.good && links.pos == links.end;
}
} // namespace dx_ac1024_renderglobal_wire
#endif
