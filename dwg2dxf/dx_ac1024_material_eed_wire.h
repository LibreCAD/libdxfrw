// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1024_MATERIAL_EED_WIRE_H
#define DX_AC1024_MATERIAL_EED_WIRE_H

// Independent, bounded observer for the seven EED-bearing MATERIAL frames in
// the pinned AC1024 aerial corpus. This checks only EED field spans and their
// agreement with the typed variants. Source identity, APPID provenance,
// MATERIAL body semantics, graph closure, and output remain separate gates.
#include "dx_raster_dwg_wire.h"
#include "drw_objects.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace dx_ac1024_material_eed_wire {

struct Profile {
    std::uint32_t handle;
    std::uint32_t eedBegin;
    std::uint32_t eedEnd;
    // Signed values for DXF codes 1070, 1070, 1070, 1071, 1070. The 1000
    // string between them is exactly empty in this source profile.
    std::array<std::int32_t, 5> values;
};

constexpr std::array<Profile, 7> profiles{{
    {0x98u, 26u, 214u, {{0, 3, 0, 0, 0}}},
    {0x2C2u, 34u, 222u, {{-1, 3, 1, 72, 0}}},
    {0x2CCu, 34u, 222u, {{-1, 3, 1, 72, 0}}},
    {0x2D6u, 34u, 222u, {{-1, 3, 1, 72, 0}}},
    {0x2E0u, 34u, 222u, {{-1, 3, 1, 8, 0}}},
    {0x2EAu, 34u, 222u, {{-1, 3, 1, 0, 0}}},
    {0x32Eu, 34u, 222u, {{-1, 3, 1, 0, 0}}}
}};

inline const Profile* profileFor(std::uint32_t handle) noexcept {
    for (const auto& profile : profiles)
        if (profile.handle == handle)
            return &profile;
    return nullptr;
}

inline std::size_t otherProfileIndex(std::uint32_t handle) noexcept {
    for (std::size_t index = 1u; index < profiles.size(); ++index)
        if (profiles[index].handle == handle)
            return index - 1u;
    return profiles.size();
}

template <class AppIdSource>
bool observeProfile(const DRW_UnsupportedObject& raw,
                    const DRW_Material& material,
                    const Profile& profile,
                    AppIdSource appIdSource) noexcept {
    using namespace dx_raster_wire;
    using namespace dx_raster_wire::detail;
    static_assert(noexcept(appIdSource(std::uint32_t{},
                                       std::declval<std::string_view>())),
                  "APPID source proof must not throw");
    if (raw.m_version != DRW::AC1024 || raw.m_objectType != 505
        || raw.m_handle != profile.handle || raw.m_isEntity
        || !raw.m_isCustomClass || !raw.m_hasClassDefinition
        || raw.m_recordName != "MATERIAL"
        || raw.m_className != "AcDbMaterial"
        || material.tType != DRW::MATERIAL || material.handle != profile.handle
        || raw.m_rawBytes.empty() || raw.m_rawBytes.size() > 4096u
        || raw.m_objectSize != raw.m_rawBytes.size()
        || raw.m_bodyBitSize == 0u
        || raw.m_bodyBitSize >= raw.m_rawBytes.size() * 8u
        || material.extData.size() != 7u
        || !material.pendingAppIdResolutions.empty())
        return false;

    const auto total = static_cast<std::uint32_t>(raw.m_rawBytes.size() * 8u);
    Bits body{raw.m_rawBytes.data(), 0u, total - raw.m_bodyBitSize};
    const auto typeTag = body.take(2u);
    const auto type = static_cast<std::uint32_t>(
        typeTag == 0u ? body.take(8u)
        : typeTag == 1u ? body.take(8u) + 0x1f0u
                        : body.little(2u));
    const auto self = handle(body, 0u, 0u, true);
    if (!body.good || type != 505u || self.value != material.handle
        || body.pos != profile.eedBegin)
        return false;

    constexpr std::array<std::uint32_t, 6> itemCodes{{
        1070u, 1070u, 1070u, 1000u, 1071u, 1070u
    }};
    const auto visit = [&](const EedToken& token) noexcept {
        if (token.index >= material.extData.size())
            return false;
        const DRW_Variant* value = material.extData[token.index];
        if (value == nullptr || value->code() != static_cast<int>(token.code))
            return false;
        if (token.index == 0u) {
            return token.code == 1001u && token.role == 5u
                && token.value == 0x12u && value->stringEquals("ACAD")
                && appIdSource(0x12u, "ACAD");
        }
        const std::size_t item = token.index - 1u;
        if (item >= itemCodes.size() || token.code != itemCodes[item])
            return false;
        if (token.code == 1000u)
            return token.value == 0u && value->stringEquals("");
        const std::size_t valueIndex = item < 3u ? item : item - 1u;
        const std::int32_t expected = profile.values[valueIndex];
        const std::uint32_t wireValue = token.code == 1070u
            ? static_cast<std::uint16_t>(expected)
            : static_cast<std::uint32_t>(expected);
        return token.value == wireValue
            && value->type() == DRW_Variant::INTEGER
            && value->i_val() == expected;
    };
    Proof proof;
    if (eed(body, 2010u, proof, visit) != Status::Proven
        || proof.eedChunks != 1u || proof.eedItems != 6u
        || proof.eedVariants != material.extData.size()
        || !proof.at(Field::Eed).present
        || proof.at(Field::Eed).begin != profile.eedBegin
        || proof.at(Field::Eed).end != profile.eedEnd)
        return false;
    return body.good && body.pos == profile.eedEnd;
}

template <class AppIdSource>
bool observeGlobal(const DRW_UnsupportedObject& raw,
                   const DRW_Material& material,
                   AppIdSource appIdSource) noexcept {
    return raw.m_handle == profiles.front().handle
        && observeProfile(raw, material, profiles.front(), appIdSource);
}

template <class AppIdSource>
bool observeOther(const DRW_UnsupportedObject& raw,
                  const DRW_Material& material,
                  AppIdSource appIdSource) noexcept {
    const Profile* profile = profileFor(raw.m_handle);
    return profile != nullptr && profile->handle != profiles.front().handle
        && observeProfile(raw, material, *profile, appIdSource);
}

} // namespace dx_ac1024_material_eed_wire

#endif
