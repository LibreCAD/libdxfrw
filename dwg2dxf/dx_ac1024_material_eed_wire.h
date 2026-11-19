// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1024_MATERIAL_EED_WIRE_H
#define DX_AC1024_MATERIAL_EED_WIRE_H

// Independent, bounded observer for the AC1024 Global MATERIAL EED sequence
// in the pinned aerial corpus. This proves only the EED field spans and their
// agreement with the typed variants; source identity, APPID provenance,
// MATERIAL body semantics, graph closure, and output remain separate gates.
#include "dx_raster_dwg_wire.h"
#include "drw_objects.h"

#include <cstdint>
#include <string_view>
#include <utility>

namespace dx_ac1024_material_eed_wire {

template <class AppIdSource>
bool observeGlobal(const DRW_UnsupportedObject& raw,
                   const DRW_Material& material,
                   AppIdSource appIdSource) noexcept {
    using namespace dx_raster_wire;
    using namespace dx_raster_wire::detail;
    static_assert(noexcept(appIdSource(std::uint32_t{},
                                       std::declval<std::string_view>())),
                  "APPID source proof must not throw");
    if (raw.m_version != DRW::AC1024 || raw.m_objectType != 505
        || raw.m_handle != 0x98u || raw.m_isEntity
        || !raw.m_isCustomClass || !raw.m_hasClassDefinition
        || raw.m_recordName != "MATERIAL"
        || raw.m_className != "AcDbMaterial"
        || material.tType != DRW::MATERIAL || material.handle != 0x98u
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
        || body.pos != 26u)
        return false;

    const auto expected = [](std::size_t index, std::uint32_t& code,
                             std::int32_t& value) noexcept {
        switch (index) {
        case 1u: code = 1070u; value = 0; return true;
        case 2u: code = 1070u; value = 3; return true;
        case 3u: code = 1070u; value = 0; return true;
        case 4u: code = 1000u; value = 0; return true;
        case 5u: code = 1071u; value = 0; return true;
        case 6u: code = 1070u; value = 0; return true;
        default: return false;
        }
    };
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
        std::uint32_t expectedCode = 0u;
        std::int32_t expectedValue = 0;
        if (!expected(token.index, expectedCode, expectedValue)
            || token.code != expectedCode)
            return false;
        if (token.code == 1000u)
            return token.value == 0u && value->stringEquals("");
        return token.value == static_cast<std::uint32_t>(expectedValue)
            && value->type() == DRW_Variant::INTEGER
            && value->i_val() == expectedValue;
    };
    Proof proof;
    if (eed(body, 2010u, proof, visit) != Status::Proven
        || proof.eedChunks != 1u || proof.eedItems != 6u
        || proof.eedVariants != material.extData.size()
        || !proof.at(Field::Eed).present
        || proof.at(Field::Eed).begin != 26u
        || proof.at(Field::Eed).end != 214u)
        return false;
    return body.good && body.pos == 214u;
}

} // namespace dx_ac1024_material_eed_wire

#endif
