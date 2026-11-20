// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1024_MECHANICAL_TILE_WIRE_H
#define DX_AC1024_MECHANICAL_TILE_WIRE_H

// Source-only observations for the four TILE XRECORDs in the AC1024
// mechanical Global MATERIAL extension. These checks do not authorize output.
// ODA DWG v5.4.1 sections 20.4.44 and 20.4.104.
#include "dx_layerstates_dwg_wire.h"
#include "dx_raster_dwg_wire.h"
#include "drw_objects.h"

#include <array>
#include <cstdint>

namespace dx_ac1024_mechanical_tile_wire {

constexpr std::array<std::uint32_t, 4> handles{{0x112u, 0x111u, 0x113u, 0x114u}};
constexpr std::array<const char*, 4> names{{
    "BUMPTILE", "DIFFUSETILE", "OPACITYTILE", "REFLECTIONTILE"
}};

// The dictionary frame establishes membership, not the bodies or output
// semantics of the three referenced MATERIAL objects.
inline bool observeMaterialRoot(const DRW_UnsupportedObject& raw,
                                const DRW_Dictionary& source) noexcept {
    constexpr std::array<const char*, 3> rootNames{{"ByBlock", "ByLayer", "Global"}};
    constexpr std::array<std::uint32_t, 3> rootHandles{{0x97u, 0x96u, 0x98u}};
    if (source.handle != 0x72u || source.parentHandle != 0xcu
        || source.cloning != 1 || source.hardOwner != 0
        || source.reactorHandles.size() != 1u || source.reactorHandles[0] != 0xcu
        || source.m_entries.size() != rootNames.size() || !source.extData.empty()
        || raw.m_objectSize != 65u || raw.m_rawBytes.size() != 65u
        || raw.m_bodyBitSize != 86u) return false;
    for (std::size_t i = 0u; i < rootNames.size(); ++i)
        if (source.m_entries[i].m_name != rootNames[i]
            || source.m_entries[i].m_handle != rootHandles[i]) return false;
    return dx_layerstates_dwg_wire::observeDictionary(raw, source,
        [](std::uint32_t, const std::string&) noexcept { return false; },
        [](dx_layerstates_dwg_wire::LinkKind kind, std::size_t index,
           std::uint8_t role, std::uint8_t length) noexcept {
            if (kind == dx_layerstates_dwg_wire::LinkKind::Owner
                || kind == dx_layerstates_dwg_wire::LinkKind::Reactor)
                return index == 0u && role == 4u && length == 1u;
            return kind == dx_layerstates_dwg_wire::LinkKind::Item
                && index < 3u && role == 2u && length == 1u;
        });
}

inline bool observeExtension(const DRW_UnsupportedObject& raw,
                             const DRW_Dictionary& source) noexcept {
    if (source.handle != 0x110u || source.parentHandle != 0x98u
        || source.hardOwner != 1 || source.cloning != 1
        || source.m_entries.size() != names.size()
        || !source.reactorHandles.empty() || !source.extData.empty()
        || raw.m_objectSize != 118u || raw.m_rawBytes.size() != 118u
        || raw.m_bodyBitSize != 116u) return false;
    for (std::size_t i = 0; i < names.size(); ++i)
        if (source.m_entries[i].m_name != names[i]
            || source.m_entries[i].m_handle != handles[i]) return false;
    return dx_layerstates_dwg_wire::observeDictionary(raw, source,
        [](std::uint32_t, const std::string&) noexcept { return false; },
        [](dx_layerstates_dwg_wire::LinkKind kind, std::size_t index,
           std::uint8_t role, std::uint8_t length) noexcept {
            if (kind == dx_layerstates_dwg_wire::LinkKind::Owner)
                return index == 0u && role == 12u && length == 1u;
            return kind == dx_layerstates_dwg_wire::LinkKind::Item
                && index < handles.size() && role == 2u && length == 2u;
        });
}

inline bool observeRecord(const DRW_UnsupportedObject& raw,
                          const DRW_XRecord& source) noexcept {
    using namespace dx_raster_wire;
    using namespace dx_raster_wire::detail;
    const auto id = source.handle;
    if ((id != 0x111u && id != 0x112u && id != 0x113u && id != 0x114u)
        || source.tType != DRW::XRECORD || source.parentHandle != 0x110u
        || !source.name.empty() || source.flags != 0u
        || source.reactorHandles.size() != 1u
        || source.reactorHandles.front() != 0x110u
        || source.reactorCount() != 1 || source.extensionDictionaryFlag() != 1u
        || source.xDictHandle != 0u || source.hasDataStorageBinaryData()
        || !source.hasDwgCommonLinkTail() || !source.appData.empty()
        || !source.extData.empty() || !source.pendingAppIdResolutions.empty()
        || !source.pendingLayerRefResolutions.empty() || source.m_cloning != 1
        || source.m_rawDataValid || !source.m_rawData.empty()
        || source.hasDwgRawDataSourceRecord() || !source.m_handleValues.empty()
        || !source.hasExactDxfCoordinateTuples() || source.m_values.size() != 2u
        || source.m_dataEntries.size() != 2u
        || raw.m_version != DRW::AC1024 || raw.m_objectType != 79
        || raw.m_handle != id || raw.m_isEntity || raw.m_isCustomClass
        || raw.m_hasClassDefinition || !raw.m_recordName.empty()
        || !raw.m_className.empty() || !raw.m_classAppName.empty()
        || raw.m_parentHandle != 0x110u || raw.m_numReactors != 1
        || raw.m_reactorHandles != source.reactorHandles || raw.m_xDictFlag != 1u
        || raw.m_xDictHandle != 0u || raw.m_hasDataStorage
        || !raw.m_commonHandleDataValidated
        || !drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence)
        || raw.m_objectSize != (id == 0x111u ? 21u : 22u)
        || raw.m_rawBytes.size() != raw.m_objectSize
        || raw.m_bodyBitSize != (id == 0x111u ? 36u : 44u)) return false;
    const auto clean = [](const DRW_Variant& v, int code) noexcept {
        return v.code() == code && v.type() == DRW_Variant::INTEGER
            && v.i_val() == 1 && !v.dwgCmColorValue()
            && !v.isLayerRef() && !v.rawDwgLayerReference();
    };
    if (!clean(source.m_values[0], 270) || !clean(source.m_values[1], 271)
        || !clean(source.m_dataEntries[0], 270)
        || !clean(source.m_dataEntries[1], 271)) return false;
    const auto total = static_cast<std::uint32_t>(raw.m_rawBytes.size() * 8u);
    Bits body{raw.m_rawBytes.data(), 0u, total - raw.m_bodyBitSize};
    const auto tag = body.take(2u);
    const auto type = static_cast<std::uint32_t>(
        tag == 0u ? body.take(8u) : tag == 1u ? body.take(8u) + 0x1f0u
                                                    : body.little(2u));
    const auto self = handle(body, 0u, 0u, true);
    Proof eedProof;
    if (!body.good || type != 79u || self.value != id
        || eed(body, 2010u, eedProof, [](const EedToken&) noexcept { return false; })
            != Status::Proven || eedProof.eedVariants != 0u
        || body.longValue() != 1u || body.take(1u) != 1u
        || body.longValue() != 8u || body.little(2u) != 270u
        || body.little(2u) != 1u || body.little(2u) != 271u
        || body.little(2u) != 1u || body.shortValue() != 1u
        || body.take(1u) != 0u || !body.good || body.pos != body.end) return false;
    Bits links{raw.m_rawBytes.data(), body.end, total};
    const auto owner = handle(links, 4u, id);
    const auto reactor = handle(links, 4u, id);
    const auto padding = links.end - links.pos;
    return links.good && owner.value == 0x110u
        && owner.role == (id == 0x111u ? 8u : 12u)
        && owner.length == (id == 0x111u ? 0u : 1u)
        && reactor.role == 4u && reactor.length == 2u && reactor.value == 0x110u
        && padding == 4u && links.take(padding) == 15u && links.good
        && links.pos == links.end;
}

} // namespace dx_ac1024_mechanical_tile_wire
#endif
