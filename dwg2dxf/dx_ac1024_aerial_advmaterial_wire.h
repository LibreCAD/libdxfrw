// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1024_AERIAL_ADVMATERIAL_WIRE_H
#define DX_AC1024_AERIAL_ADVMATERIAL_WIRE_H

// Source-only profile for the compact AC1024 ADVMATERIAL child of the first
// aerial MATERIAL. This proves exact source bytes and graph identity, not the
// meaning of Autodesk's extended 460-series application fields.
#include "dx_raster_dwg_wire.h"
#include "drw_objects.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace dx_ac1024_aerial_advmaterial_wire {

struct Profile {
    std::uint32_t materialHandle;
    std::uint32_t extensionHandle;
    std::uint32_t recordHandle;
};

constexpr Profile first{0x2C2u, 0x2C3u, 0x2C4u};
inline constexpr std::array<Profile, 6> profiles{{
    first,
    {0x2CCu, 0x2CDu, 0x2CEu},
    {0x2D6u, 0x2D7u, 0x2D8u},
    {0x2E0u, 0x2E1u, 0x2E2u},
    {0x2EAu, 0x2EBu, 0x2ECu},
    {0x32Eu, 0x32Fu, 0x330u}
}};

inline bool rawDouble(dx_raster_wire::detail::Bits& bits,
                      double expected) noexcept {
    const auto raw = bits.little(8u);
    double actual = 0.0;
    static_assert(sizeof(actual) == sizeof(raw), "DWG double must be 64-bit");
    std::memcpy(&actual, &raw, sizeof(actual));
    return bits.good && std::isfinite(actual)
        && std::memcmp(&actual, &expected, sizeof(actual)) == 0;
}

inline bool observe(const DRW_UnsupportedObject& raw,
                    const DRW_XRecord& source,
                    const Profile& profile = first) noexcept {
    using namespace dx_raster_wire;
    using namespace dx_raster_wire::detail;
    if (profile.materialHandle + 1u != profile.extensionHandle
        || profile.extensionHandle + 1u != profile.recordHandle
        || source.tType != DRW::XRECORD
        || source.handle != profile.recordHandle
        || source.parentHandle != profile.extensionHandle
        || !source.name.empty() || source.flags != 0u
        || source.reactorHandles.size() != 1u
        || source.reactorHandles.front() != profile.extensionHandle
        || source.reactorCount() != 1
        || source.extensionDictionaryFlag() != 1u
        || source.xDictHandle != 0u
        || source.hasDataStorageBinaryData() || !source.hasDwgCommonLinkTail()
        || !source.appData.empty() || !source.extData.empty()
        || !source.pendingAppIdResolutions.empty()
        || !source.pendingLayerRefResolutions.empty()
        || source.m_cloning != 1 || source.m_rawDataValid
        || !source.m_rawData.empty() || source.hasDwgRawDataSourceRecord()
        || !source.m_handleValues.empty()
        || !source.hasExactDxfCoordinateTuples()
        || source.m_values.size() != 10u
        || source.m_dataEntries.size() != 10u
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
        || raw.m_objectSize != 81u || raw.m_rawBytes.size() != 81u
        || raw.m_bodyBitSize != 36u)
        return false;

    const auto totalBits = static_cast<std::uint32_t>(raw.m_rawBytes.size() * 8u);
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
        || body.take(1u) != 1u || body.longValue() != 68u
        || body.pos != 57u)
        return false;

    Bits payload{raw.m_rawBytes.data(), body.pos, body.pos + 68u * 8u};
    const std::array<int, 10> codes{{460, 461, 462, 463, 290,
                                     270, 464, 293, 272, 273}};
    const std::array<double, 5> reals{{100.0, 100.0, 100.0, 100.0, 0.0}};
    const std::array<std::int32_t, 5> integers{{1, 1, 0, 3, 3}};
    std::size_t realIndex = 0u;
    std::size_t integerIndex = 0u;
    for (std::size_t index = 0u; index < codes.size(); ++index) {
        const auto& a = source.m_dataEntries[index];
        const auto& b = source.m_values[index];
        if (a.code() != codes[index] || b.code() != codes[index]
            || a.type() != b.type()
            || a.dwgCmColorValue() || b.dwgCmColorValue()
            || a.isLayerRef() || b.isLayerRef()
            || a.rawDwgLayerReference() || b.rawDwgLayerReference()
            || payload.little(2u) != static_cast<unsigned>(codes[index]))
            return false;
        if (codes[index] >= 460 && codes[index] <= 469) {
            const double aValue = a.d_val();
            const double bValue = b.d_val();
            if (a.type() != DRW_Variant::DOUBLE
                || std::memcmp(&aValue, &bValue, sizeof(double)) != 0
                || std::memcmp(&aValue, &reals[realIndex], sizeof(double)) != 0
                || !rawDouble(payload, reals[realIndex]))
                return false;
            ++realIndex;
        } else if (codes[index] == 290 || codes[index] == 293) {
            if (a.type() != DRW_Variant::INTEGER
                || a.i_val() != integers[integerIndex]
                || b.i_val() != integers[integerIndex]
                || payload.take(8u) != static_cast<unsigned>(integers[integerIndex]))
                return false;
            ++integerIndex;
        } else {
            if (a.type() != DRW_Variant::INTEGER
                || a.i_val() != integers[integerIndex]
                || b.i_val() != integers[integerIndex]
                || payload.little(2u)
                    != static_cast<std::uint16_t>(integers[integerIndex]))
                return false;
            ++integerIndex;
        }
        if (!payload.good)
            return false;
    }
    if (realIndex != reals.size() || integerIndex != integers.size()
        || !payload.good || payload.pos != payload.end)
        return false;

    body.pos = payload.end;
    if (body.shortValue() != 1u || body.take(1u) != 0u
        || !body.good || body.pos != body.end)
        return false;
    Bits links{raw.m_rawBytes.data(), body.end, totalBits};
    const auto owner = handle(links, 4u, source.handle);
    const auto reactor = handle(links, 4u, source.handle);
    return links.good && owner.role == 8u && owner.length == 0u
        && owner.value == profile.extensionHandle
        && reactor.role == 4u && reactor.length == 2u
        && reactor.value == profile.extensionHandle
        && links.take(4u) == 15u && links.good && links.pos == links.end;
}

} // namespace dx_ac1024_aerial_advmaterial_wire
#endif
