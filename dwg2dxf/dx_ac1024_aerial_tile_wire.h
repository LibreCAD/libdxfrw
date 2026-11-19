// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1024_AERIAL_TILE_WIRE_H
#define DX_AC1024_AERIAL_TILE_WIRE_H

// Bounded source observer for the 36 AC1024 TILE XRECORDs in the local
// aerial MATERIAL profile. This proves their field spans, typed values and
// witnessed handle roles only. It does not admit TILE output or establish
// texture semantics.
#include "dx_layerstates_dwg_wire.h"
#include "dx_raster_dwg_wire.h"
#include "drw_objects.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace dx_ac1024_aerial_tile_wire {

struct MaterialProfile {
    std::uint32_t materialHandle;
    std::uint32_t extensionHandle;
    std::array<std::uint32_t, 6> tileHandles;
};

constexpr std::array<MaterialProfile, 6> materials{{
    {0x2C2u, 0x2C3u, {{0x2C6u, 0x2C7u, 0x2C8u, 0x2C9u, 0x2CAu, 0x2CBu}}},
    {0x2CCu, 0x2CDu, {{0x2D0u, 0x2D1u, 0x2D2u, 0x2D3u, 0x2D4u, 0x2D5u}}},
    {0x2D6u, 0x2D7u, {{0x2DAu, 0x2DBu, 0x2DCu, 0x2DDu, 0x2DEu, 0x2DFu}}},
    {0x2E0u, 0x2E1u, {{0x2E4u, 0x2E5u, 0x2E6u, 0x2E7u, 0x2E8u, 0x2E9u}}},
    {0x2EAu, 0x2EBu, {{0x2EFu, 0x2F1u, 0x2F2u, 0x2F3u, 0x2F4u, 0x2F5u}}},
    {0x32Eu, 0x32Fu, {{0x332u, 0x333u, 0x334u, 0x335u, 0x336u, 0x337u}}}
}};

constexpr std::array<const char*, 6> keys{{
    "DIFFUSETILE", "BUMPTILE", "OPACITYTILE", "SPECULARTILE",
    "REFLECTIONTILE", "REFRACTIONTILE"
}};

struct TileProfile {
    std::size_t materialIndex;
    std::size_t keyIndex;
    std::uint32_t materialHandle;
    std::uint32_t extensionHandle;
    std::uint32_t recordHandle;
};

inline const MaterialProfile* materialForExtension(
        std::uint32_t extensionHandle) noexcept {
    for (const auto& profile : materials)
        if (profile.extensionHandle == extensionHandle)
            return &profile;
    return nullptr;
}

inline bool profileForRecord(std::uint32_t recordHandle,
                             TileProfile& result) noexcept {
    for (std::size_t materialIndex = 0u;
         materialIndex < materials.size(); ++materialIndex) {
        for (std::size_t keyIndex = 0u; keyIndex < keys.size(); ++keyIndex) {
            if (materials[materialIndex].tileHandles[keyIndex] == recordHandle) {
                result = {materialIndex, keyIndex,
                    materials[materialIndex].materialHandle,
                    materials[materialIndex].extensionHandle, recordHandle};
                return true;
            }
        }
    }
    return false;
}

inline bool observeRecord(const DRW_UnsupportedObject& raw,
                          const DRW_XRecord& source,
                          const TileProfile& profile) noexcept {
    using namespace dx_raster_wire;
    using namespace dx_raster_wire::detail;
    if (profile.materialIndex >= materials.size()
        || profile.keyIndex >= keys.size()
        || materials[profile.materialIndex].materialHandle
            != profile.materialHandle
        || materials[profile.materialIndex].extensionHandle
            != profile.extensionHandle
        || materials[profile.materialIndex].tileHandles[profile.keyIndex]
            != profile.recordHandle
        || source.tType != DRW::XRECORD
        || source.handle != profile.recordHandle
        || source.parentHandle != profile.extensionHandle
        || !source.name.empty() || source.flags != 0u
        || source.reactorHandles.size() != 1u
        || source.reactorHandles.front() != profile.extensionHandle
        || source.reactorCount() != 1
        || source.extensionDictionaryFlag() != 1u || source.xDictHandle != 0u
        || source.hasDataStorageBinaryData() || !source.hasDwgCommonLinkTail()
        || !source.appData.empty() || !source.extData.empty()
        || !source.pendingAppIdResolutions.empty()
        || !source.pendingLayerRefResolutions.empty()
        || source.m_cloning != 1 || source.m_rawDataValid
        || !source.m_rawData.empty() || source.hasDwgRawDataSourceRecord()
        || !source.m_handleValues.empty()
        || !source.hasExactDxfCoordinateTuples()
        || source.m_values.size() != 2u || source.m_dataEntries.size() != 2u
        || raw.m_version != DRW::AC1024 || raw.m_objectType != 79
        || raw.m_handle != profile.recordHandle || raw.m_isEntity
        || raw.m_isCustomClass || raw.m_hasClassDefinition
        || !raw.m_recordName.empty() || !raw.m_className.empty()
        || !raw.m_classAppName.empty()
        || raw.m_parentHandle != profile.extensionHandle
        || raw.m_numReactors != 1
        || raw.m_reactorHandles != source.reactorHandles
        || raw.m_xDictFlag != 1u || raw.m_xDictHandle != 0u
        || raw.m_hasDataStorage || !raw.m_commonHandleDataValidated
        || !drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence)
        || raw.m_objectSize != 22u || raw.m_rawBytes.size() != 22u
        || raw.m_bodyBitSize != 44u)
        return false;

    const auto cleanInteger = [](const DRW_Variant& value,
                                 std::uint32_t code) noexcept {
        return value.code() == static_cast<int>(code)
            && value.type() == DRW_Variant::INTEGER
            && value.i_val() == 1 && !value.dwgCmColorValue()
            && !value.isLayerRef() && !value.rawDwgLayerReference();
    };
    if (!cleanInteger(source.m_values[0], 270u)
        || !cleanInteger(source.m_values[1], 271u)
        || !cleanInteger(source.m_dataEntries[0], 270u)
        || !cleanInteger(source.m_dataEntries[1], 271u))
        return false;

    const auto total = static_cast<std::uint32_t>(raw.m_rawBytes.size() * 8u);
    if (total <= raw.m_bodyBitSize)
        return false;
    Bits body{raw.m_rawBytes.data(), 0u, total - raw.m_bodyBitSize};
    const auto typeTag = body.take(2u);
    const auto type = static_cast<std::uint32_t>(
        typeTag == 0u ? body.take(8u)
        : typeTag == 1u ? body.take(8u) + 0x1f0u
                        : body.little(2u));
    const auto self = handle(body, 0u, 0u, true);
    Proof eedProof;
    if (!body.good || type != 79u || self.value != profile.recordHandle
        || eed(body, 2010u, eedProof,
               [](const EedToken&) noexcept { return false; })
            != Status::Proven
        || eedProof.eedVariants != 0u || body.longValue() != 1u
        || body.take(1u) != 1u || body.longValue() != 8u
        || body.little(2u) != 270u || body.little(2u) != 1u
        || body.little(2u) != 271u || body.little(2u) != 1u
        || body.shortValue() != 1u || body.take(1u) != 0u
        || !body.good || body.pos != body.end)
        return false;

    Bits links{raw.m_rawBytes.data(), body.end, total};
    const auto owner = handle(links, 4u, profile.recordHandle);
    const auto reactor = handle(links, 4u, profile.recordHandle);
    const auto padding = links.end - links.pos;
    return links.good && owner.role == 12u && owner.length == 1u
        && owner.value == profile.extensionHandle
        && reactor.role == 4u && reactor.length == 2u
        && reactor.value == profile.extensionHandle
        && padding == 4u && links.take(padding) == 15u && links.good
        && links.pos == links.end;
}

inline bool observeExtensionDictionary(
        const DRW_UnsupportedObject& raw, const DRW_Dictionary& source,
        const MaterialProfile& profile) noexcept {
    const bool expanded = profile.extensionHandle == 0x2EBu;
    if (source.handle != profile.extensionHandle
        || source.parentHandle != profile.materialHandle
        || source.hardOwner != 1 || source.cloning != 1
        || source.m_entries.size() != (expanded ? 10u : 8u)
        || raw.m_objectSize != (expanded ? 254u : 224u)
        || raw.m_rawBytes.size() != raw.m_objectSize
        || raw.m_bodyBitSize != (expanded ? 248u : 204u)
        || (!expanded && (raw.m_rawBytes.back() & 0x0Fu) != 0x0Fu))
        return false;
    for (std::size_t keyIndex = 0u; keyIndex < keys.size(); ++keyIndex) {
        const auto named = std::count_if(source.m_entries.begin(),
            source.m_entries.end(), [keyIndex](const DRW_Dictionary::Entry& row) {
                return row.m_name == keys[keyIndex];
            });
        const auto targeted = std::count_if(source.m_entries.begin(),
            source.m_entries.end(), [&profile, keyIndex](
                    const DRW_Dictionary::Entry& row) {
                return row.m_handle == profile.tileHandles[keyIndex];
            });
        const auto found = std::find_if(source.m_entries.begin(),
            source.m_entries.end(), [keyIndex](const DRW_Dictionary::Entry& row) {
                return row.m_name == keys[keyIndex];
            });
        if (named != 1u || targeted != 1u
            || found == source.m_entries.end()
            || found->m_handle != profile.tileHandles[keyIndex])
            return false;
    }

    return dx_layerstates_dwg_wire::observeDictionary(raw, source,
        [](std::uint32_t, const std::string&) noexcept { return false; },
        [](dx_layerstates_dwg_wire::LinkKind kind, std::size_t index,
           std::uint8_t role, std::uint8_t length) noexcept {
            if (kind == dx_layerstates_dwg_wire::LinkKind::Owner)
                return index == 0u && role == 8u && length == 0u;
            if (kind == dx_layerstates_dwg_wire::LinkKind::Item)
                return role == 2u && length == 2u;
            return false;
        });
}

} // namespace dx_ac1024_aerial_tile_wire

#endif
