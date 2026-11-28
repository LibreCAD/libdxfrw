// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1024_AERIAL_MATERIAL_WIDE_WIRE_H
#define DX_AC1024_AERIAL_MATERIAL_WIDE_WIRE_H

// Exact local-corpus source fingerprints for the AC1024 DIFFUSE/BUMP
// XRECORDs. These profiles authorize only the two observed records and do
// not assign semantic meaning to Autodesk material values.
#include "drw_objects.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dx_ac1024_aerial_material_wide_wire {

struct Profile {
    std::uint32_t materialHandle;
    std::uint32_t extensionHandle;
    std::uint32_t recordHandle;
    const char* dictionaryKey;
    std::size_t frameBytes;
    std::uint64_t rawFnv64;
    std::uint64_t codeTypeFnv64;
};

inline constexpr std::array<Profile, 2> profiles{{
    {0x2EAu, 0x2EBu, 0x2EEu, "DIFFUSE", 2904u,
     0x56f72ea4c964a5bfull, 0xd92b916a1d51c6e5ull},
    {0x2EAu, 0x2EBu, 0x2F0u, "BUMP", 2924u,
     0x76cca7b44a4e1250ull, 0xd92b916a1d51c6e5ull}
}};

inline std::uint64_t fnv1a64(const std::uint8_t* bytes,
                             std::size_t size) noexcept {
    std::uint64_t hash = 14695981039346656037ull;
    for (std::size_t index = 0u; index < size; ++index) {
        hash ^= bytes[index];
        hash *= 1099511628211ull;
    }
    return hash;
}

inline bool observe(const DRW_UnsupportedObject& raw,
                    const DRW_XRecord& source,
                    const Profile& profile) noexcept {
    if (profile.materialHandle != 0x2EAu
        || profile.extensionHandle != 0x2EBu
        || (profile.recordHandle != 0x2EEu && profile.recordHandle != 0x2F0u)
        || source.tType != DRW::XRECORD
        || source.handle != profile.recordHandle
        || source.parentHandle != profile.extensionHandle
        || !source.name.empty() || source.flags != 0u
        || source.reactorHandles.size() != 1u
        || source.reactorHandles.front() != profile.extensionHandle
        || source.reactorCount() != 1
        || source.extensionDictionaryFlag() != 1u
        || source.xDictHandle != 0u || source.hasDataStorageBinaryData()
        || !source.hasDwgCommonLinkTail() || !source.appData.empty()
        || !source.extData.empty()
        || !source.pendingAppIdResolutions.empty()
        || !source.pendingLayerRefResolutions.empty()
        || source.m_cloning != 1 || source.m_rawDataValid
        || !source.m_rawData.empty() || source.hasDwgRawDataSourceRecord()
        || !source.m_handleValues.empty()
        || !source.hasExactDxfCoordinateTuples()
        || source.m_values.size() != 172u
        || source.m_dataEntries.size() != 172u
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
        || raw.m_bodyBitSize != 44u
        || fnv1a64(raw.m_rawBytes.data(), raw.m_rawBytes.size())
            != profile.rawFnv64)
        return false;

    constexpr std::array<int, 8> codes{{300, 301, 271, 62, 420, 291, 292, 469}};
    constexpr std::array<std::size_t, 8> counts{{84u, 5u, 13u, 2u,
                                                  2u, 2u, 11u, 53u}};
    const std::array<DRW_Variant::TYPE, 8> types{{
        DRW_Variant::STRING, DRW_Variant::STRING,
        DRW_Variant::INTEGER, DRW_Variant::INTEGER,
        DRW_Variant::INTEGER, DRW_Variant::INTEGER,
        DRW_Variant::INTEGER, DRW_Variant::DOUBLE
    }};
    std::array<std::size_t, codes.size()> observed{};
    std::uint64_t shapeHash = 14695981039346656037ull;
    for (std::size_t index = 0u; index < source.m_dataEntries.size(); ++index) {
        const auto& data = source.m_dataEntries[index];
        const auto& value = source.m_values[index];
        if (data.code() != value.code() || data.type() != value.type()
            || (data.code() == 469 && !std::isfinite(data.d_val())))
            return false;
        std::size_t codeIndex = 0u;
        while (codeIndex < codes.size() && codes[codeIndex] != data.code())
            ++codeIndex;
        if (codeIndex == codes.size() || data.type() != types[codeIndex])
            return false;
        ++observed[codeIndex];
        const std::uint32_t code = static_cast<std::uint32_t>(data.code());
        for (unsigned shift = 0u; shift < 32u; shift += 8u) {
            shapeHash ^= static_cast<std::uint8_t>(code >> shift);
            shapeHash *= 1099511628211ull;
        }
        shapeHash ^= static_cast<std::uint8_t>(data.type());
        shapeHash *= 1099511628211ull;
    }
    return observed == counts && shapeHash == profile.codeTypeFnv64;
}

} // namespace dx_ac1024_aerial_material_wide_wire

#endif
