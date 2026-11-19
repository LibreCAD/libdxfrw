// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1024_AERIAL_FBXASSET_WIRE_H
#define DX_AC1024_AERIAL_FBXASSET_WIRE_H

// Bounded source observer for the six remaining AC1024 aerial MATERIAL
// FBXASSET XRECORDs. It binds same-import typed entries to their complete raw
// frames and graph roles. This local-corpus profile grants no output authority
// and establishes no FBX or texture semantics.
#include "dx_raster_dwg_wire.h"
#include "drw_objects.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace dx_ac1024_aerial_fbxasset_wire {

struct Profile {
    std::uint32_t materialHandle;
    std::uint32_t extensionHandle;
    std::uint32_t recordHandle;
    std::uint32_t frameBytes;
    std::uint32_t payloadBytes;
    std::size_t entryCount;
    std::size_t binaryBytes;
};

constexpr std::array<Profile, 6> profiles{{
    {0x2C2u, 0x2C3u, 0x2C5u, 37537u, 37520u, 292u, 36566u},
    {0x2CCu, 0x2CDu, 0x2CFu, 38092u, 38075u, 297u, 37106u},
    {0x2D6u, 0x2D7u, 0x2D9u, 40486u, 40469u, 315u, 39446u},
    {0x2E0u, 0x2E1u, 0x2E3u, 33200u, 33183u, 259u, 32328u},
    {0x2EAu, 0x2EBu, 0x2EDu, 53578u, 53561u, 416u, 52235u},
    {0x32Eu, 0x32Fu, 0x331u, 25556u, 25539u, 200u, 24861u}
}};

inline const Profile* profileForRecord(std::uint32_t handle) noexcept {
    for (const auto& profile : profiles)
        if (profile.recordHandle == handle)
            return &profile;
    return nullptr;
}

inline bool observeRecord(const DRW_UnsupportedObject& raw,
                          const DRW_XRecord& source,
                          const Profile& profile) noexcept {
    using namespace dx_raster_wire;
    using namespace dx_raster_wire::detail;
    if (profile.extensionHandle + 2u != profile.recordHandle
        || profile.materialHandle + 1u != profile.extensionHandle
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
        || source.m_values.size() != profile.entryCount
        || source.m_dataEntries.size() != profile.entryCount
        || raw.m_version != DRW::AC1024 || raw.m_objectType != 79u
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
        || raw.m_objectSize != profile.frameBytes
        || raw.m_rawBytes.size() != profile.frameBytes
        || raw.m_bodyBitSize != 44u)
        return false;

    const auto clean = [](const DRW_Variant& value) noexcept {
        return !value.dwgCmColorValue() && !value.isLayerRef()
            && !value.rawDwgLayerReference();
    };
    const auto totalBits = static_cast<std::uint32_t>(raw.m_rawBytes.size() * 8u);
    if (totalBits <= raw.m_bodyBitSize)
        return false;
    Bits body{raw.m_rawBytes.data(), 0u, totalBits - raw.m_bodyBitSize};
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
        || body.take(1u) != 1u
        || body.longValue() != profile.payloadBytes || body.pos != 81u)
        return false;

    Bits payload{raw.m_rawBytes.data(), body.pos,
                 body.pos + profile.payloadBytes * 8u};
    std::size_t binaryBytes = 0u;
    for (std::size_t index = 0u; index < profile.entryCount; ++index) {
        const auto& a = source.m_dataEntries[index];
        const auto& b = source.m_values[index];
        const int expectedCode = index == 0u ? 70
            : index == 1u ? 90 : index == 2u ? 71
            : index == 3u ? 1 : 310;
        if (a.code() != expectedCode || b.code() != expectedCode
            || !clean(a) || !clean(b)
            || payload.little(2u) != static_cast<unsigned>(expectedCode))
            return false;

        if (index < 3u) {
            if (a.type() != DRW_Variant::INTEGER
                || b.type() != DRW_Variant::INTEGER
                || a.i_val() != b.i_val())
                return false;
            const auto width = index == 1u ? 4u : 2u;
            if (payload.little(width)
                != static_cast<std::uint32_t>(a.i_val()))
                return false;
        } else if (index == 3u) {
            if (a.type() != DRW_Variant::STRING
                || b.type() != DRW_Variant::STRING
                || !a.content.s || !b.content.s
                || *a.content.s != *b.content.s
                || a.content.s->size() != 36u
                || payload.little(2u) != a.content.s->size())
                return false;
            for (unsigned char character : *a.content.s) {
                if (character > 127u
                    || payload.little(2u) != character)
                    return false;
            }
        } else {
            if (a.type() != DRW_Variant::BINARY
                || b.type() != DRW_Variant::BINARY
                || !a.binary() || !b.binary()
                || *a.binary() != *b.binary()
                || a.binary()->empty() || a.binary()->size() > 127u
                || payload.take(8u) != a.binary()->size())
                return false;
            binaryBytes += a.binary()->size();
            for (std::uint8_t byte : *a.binary())
                if (payload.take(8u) != byte)
                    return false;
        }
        if (!payload.good)
            return false;
    }
    if (!payload.good || payload.pos != payload.end
        || binaryBytes != profile.binaryBytes)
        return false;

    body.pos = payload.end;
    if (body.shortValue() != 1u || body.take(1u) != 0u
        || !body.good || body.pos != body.end)
        return false;
    Bits links{raw.m_rawBytes.data(), body.end, totalBits};
    const auto owner = handle(links, 4u, source.handle);
    const auto reactor = handle(links, 4u, source.handle);
    const bool valid = links.good && owner.role == 12u && owner.length == 1u
        && owner.value == profile.extensionHandle
        && reactor.role == 4u && reactor.length == 2u
        && reactor.value == profile.extensionHandle
        && links.take(4u) == 15u && links.good && links.pos == links.end;
    return valid;
}

} // namespace dx_ac1024_aerial_fbxasset_wire
#endif
