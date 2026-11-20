// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1024_BYBLOCK_MATERIAL_WIRE_H
#define DX_AC1024_BYBLOCK_MATERIAL_WIRE_H

#include "dx_layerstates_dwg_wire.h"
#include <cmath>
#include <cstring>

// Source-only aerial and mechanical AC1024 ByLayer/ByBlock MATERIAL frames.
// The dictionary and XRECORD observers below are aerial-only. This observer
// does not establish same-import/class provenance, unchanged source baselines,
// graph closure, staged output or native FBX semantics. In particular, the
// text and binary bytes below are compared to the supplied live model: a caller
// must bind both the raw frame and model to immutable original-import evidence.
// ODA DWG v5.4.1 §§2.12, 2.13, 20.1, 20.4.44 and 20.4.104.
namespace dx_ac1024_byblock_material_wire {
namespace detail {
using dx_raster_wire::detail::Bits;
using dx_raster_wire::detail::handle;
inline bool number(Bits& b, double expected) noexcept {
    const auto tag = b.take(2u);
    double value = 0.0;
    if (tag == 0u) {
        const auto bits = b.little(8u);
        static_assert(sizeof(bits) == sizeof(value), "64-bit double required");
        std::memcpy(&value, &bits, sizeof(value));
    } else if (tag == 1u) {
        value = 1.0;
    } else if (tag != 2u) {
        return false;
    }
    return b.good && std::memcmp(&value, &expected, sizeof(value)) == 0;
}
inline bool text(Bits& b, const std::string& value) noexcept {
    if (b.shortValue() != value.size()) {
        return false;
    }
    for (unsigned char c : value) {
        if (c > 127u || b.little(2u) != c) {
            return false;
        }
    }
    return b.good;
}
inline bool color(Bits& b, std::uint8_t method, double factor, std::uint32_t value, bool hasMethod,
                  bool hasFactor, bool hasValue) noexcept {
    return method == 0u && factor == 1.0 && value == 0u && hasMethod && hasFactor && !hasValue
           && b.take(8u) == 0u && number(b, 1.0);
}
inline bool mapper(Bits& b, Bits& strings, double blend, std::uint8_t source, const std::string& filename,
                   std::uint8_t projection, std::uint8_t tiling, std::uint8_t automatic,
                   const std::array<double, 16>& matrix, bool hasBlend, bool hasSource, bool hasFilename,
                   bool hasProjection, bool hasTiling, bool hasAutomatic, bool hasMatrix,
                   std::uint8_t expectedSource, std::uint8_t expectedProjection,
                   bool scaled = false) noexcept {
    if (blend != 1.0 || source != expectedSource || !filename.empty() || projection != expectedProjection || tiling != 1u
        || automatic != 1u || !hasBlend || !hasSource || hasFilename != (expectedSource == 1u)
        || !hasProjection || !hasTiling || !hasAutomatic || !hasMatrix || !number(b, 1.0)
        || b.take(8u) != expectedProjection || b.take(8u) != 1u || b.take(8u) != 1u) {
        return false;
    }
    for (std::size_t i = 0; i < 16u; ++i) {
        const double expected = scaled && (i == 0u || i == 5u) ? static_cast<double>(0.0208f)
            : (i == 0u || i == 5u || i == 10u || i == 15u) ? 1.0 : 0.0;
        if (matrix[i] != expected || !number(b, expected)) {
            return false;
        }
    }
    return b.take(8u) == expectedSource && (expectedSource == 0u || text(strings, filename)) && b.good;
}
} // namespace detail
inline bool observeMaterialProfile(const DRW_UnsupportedObject& raw, const DRW_Material& m,
                                   bool mechanical) noexcept {
    using namespace dx_raster_wire;
    const bool byLayer = m.handle == 0x96u;
    if (!byLayer && m.handle != 0x97u) return false;
    const auto extensionHandle = mechanical ? 0u : (byLayer ? 0x297u : 0x295u);
    if (m.tType != DRW::MATERIAL || m.parentHandle != 0x72u || !m.name.empty()
        || m.flags != 0u || m.reactorCount() != 1 || m.reactorHandles.size() != 1u
        || m.reactorHandles[0] != 0x72u || m.extensionDictionaryFlag() != (mechanical ? 1u : 0u)
        || m.xDictHandle != extensionHandle
        || m.hasDataStorageBinaryData() || m.hasDwgCommonLinkTail() || !m.appData.empty()
        || !m.extData.empty() || !m.pendingAppIdResolutions.empty() || !m.pendingLayerRefResolutions.empty()
        || !m.hasCompleteDwgSemanticFields() || !m.hasDwgRawData() || m.dwgRawDataVersion() != DRW::AC1024
        || m.dwgRawDataBitSize() != 516u || m.dwgRawData().size() != 65u || m.m_name != (byLayer ? "ByLayer" : "ByBlock")
        || !m.m_description.empty() || raw.m_version != DRW::AC1024 || raw.m_objectType != 505u
        || raw.m_handle != m.handle || raw.m_isEntity || !raw.m_isCustomClass || !raw.m_hasClassDefinition
        || raw.m_recordName != "MATERIAL" || raw.m_className != "AcDbMaterial"
        || raw.m_classAppName != "ObjectDBX Classes" || raw.m_parentHandle != 0x72u
        || raw.m_numReactors != 1 || raw.m_reactorHandles != m.reactorHandles || raw.m_xDictFlag != (mechanical ? 1u : 0u)
        || raw.m_xDictHandle != extensionHandle || raw.m_hasDataStorage || !raw.m_commonHandleDataValidated
        || !drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence) || raw.m_objectSize != (mechanical ? 93u : 96u)
        || raw.m_rawBytes.size() != raw.m_objectSize || raw.m_bodyBitSize != (mechanical ? 38u : 62u)) {
        return false;
    }
    detail::Bits b{raw.m_rawBytes.data(), 0u, 555u};
    if (b.take(2u) != 1u || b.take(8u) + 0x1f0u != 505u) {
        return false;
    }
    const auto self = dx_raster_wire::detail::handle(b, 0u, 0u, true);
    if (self.role != 0u || self.length != 1u || self.value != m.handle || b.shortValue() != 0u
        || b.longValue() != 1u || b.take(1u) != (mechanical ? 1u : 0u) || !b.good || b.pos != 39u) {
        return false;
    }
    // Bind the exposed opaque semantic-body shelf to the actual frame bits.
    for (std::uint32_t i = 0; i < 516u; ++i) {
        const unsigned rawBit = (raw.m_rawBytes[(39u + i) / 8u] >> (7u - (39u + i) % 8u)) & 1u;
        const unsigned shelfBit = (m.dwgRawData()[i / 8u] >> (7u - i % 8u)) & 1u;
        if (rawBit != shelfBit) {
            return false;
        }
    }
    detail::Bits strings{raw.m_rawBytes.data(), 555u, 689u};
    if (!detail::text(strings, m.m_name) || !detail::text(strings, m.m_description)
        || !detail::color(b, m.m_ambientColorMethod, m.m_ambientColorFactor, m.m_ambientColorValue,
                          m.m_hasAmbientColorMethod, m.m_hasAmbientColorFactor, m.m_hasAmbientColorValue)
        || !detail::color(b, m.m_diffuseColorMethod, m.m_diffuseColorFactor, m.m_diffuseColorValue,
                          m.m_hasDiffuseColorMethod, m.m_hasDiffuseColorFactor, m.m_hasDiffuseColorValue)) {
        return false;
    }
#define DX_BYBLOCK_MAP(lower, Upper, expected)                                                             \
    detail::mapper(b, strings, m.m_##lower##MapBlendFactor, m.m_##lower##MapSource,                        \
                   m.m_##lower##MapFileName, m.m_##lower##MapProjection, m.m_##lower##MapTiling,           \
                   m.m_##lower##MapAutoTransform, m.m_##lower##MapTransform,                               \
                   m.m_has##Upper##MapBlendFactor, m.m_has##Upper##MapSource, m.m_has##Upper##MapFileName, \
                   m.m_has##Upper##MapProjection, m.m_has##Upper##MapTiling,                               \
                   m.m_has##Upper##MapAutoTransform, m.m_has##Upper##MapTransform, expected, mechanical ? 2u : 1u)
    const bool decoded
        = DX_BYBLOCK_MAP(diffuse, Diffuse, 1u)
          && detail::color(b, m.m_specularColorMethod, m.m_specularColorFactor, m.m_specularColorValue,
                           m.m_hasSpecularColorMethod, m.m_hasSpecularColorFactor,
                           m.m_hasSpecularColorValue)
          && DX_BYBLOCK_MAP(specular, Specular, 1u) && m.m_hasSpecularGlossFactor
          && m.m_specularGlossFactor == 0.5 && detail::number(b, 0.5)
          && DX_BYBLOCK_MAP(reflection, Reflection, 1u) && m.m_hasOpacityPercent
          && m.m_opacityPercent == 1.0 && detail::number(b, 1.0) && DX_BYBLOCK_MAP(opacity, Opacity, 1u)
          && DX_BYBLOCK_MAP(bump, Bump, 1u) && m.m_hasRefractionIndex && m.m_refractionIndex == 1.0
          && detail::number(b, 1.0) && DX_BYBLOCK_MAP(refraction, Refraction, 0u) && m.m_translucence == 0.0
          && !m.m_hasTranslucence && detail::number(b, 0.0) && detail::number(b, 0.0)
          && m.m_reflectivity == 0.0 && !m.m_hasReflectivity && detail::number(b, 0.0)
          && m.m_illuminationModel == 0 && !m.m_hasIlluminationModel && b.longValue() == 0u
          && m.m_channelFlags == 63 && m.m_hasChannelFlags && b.longValue() == 63u && b.longValue() == 0u;
#undef DX_BYBLOCK_MAP
    if (!decoded || !b.good || b.pos != b.end || !strings.good || strings.pos != strings.end) {
        return false;
    }
    detail::Bits footer{raw.m_rawBytes.data(), 689u, 706u};
    if (footer.little(2u) != 134u || footer.take(1u) != 1u || !footer.good) {
        return false;
    }
    detail::Bits links{raw.m_rawBytes.data(), 706u, mechanical ? 744u : 768u};
    const auto owner = detail::handle(links, 4u, m.handle);
    const auto reactor = detail::handle(links, 4u, m.handle);
    if (!links.good || owner.role != 12u || owner.length != 1u || owner.value != 0x72u
        || reactor.role != 4u || reactor.length != 1u || reactor.value != 0x72u) return false;
    if (!mechanical) {
        const auto extension = detail::handle(links, 3u, m.handle);
        if (extension.role != 3u || extension.length != 2u || extension.value != extensionHandle) return false;
    }
    return links.take(6u) == 63u && links.good && links.pos == links.end;
}
inline bool observeMaterial(const DRW_UnsupportedObject& raw, const DRW_Material& m) noexcept {
    return observeMaterialProfile(raw, m, false);
}
inline bool observeMechanicalMaterial(const DRW_UnsupportedObject& raw, const DRW_Material& m) noexcept {
    return observeMaterialProfile(raw, m, true);
}

// Mechanical Global body only. The caller must independently verify its EED,
// same-import source identity, graph, and any staged output.
inline bool observeMechanicalGlobalBody(const DRW_UnsupportedObject& raw,
                                        const DRW_Material& m) noexcept {
    using namespace dx_raster_wire;
    if (m.tType != DRW::MATERIAL || m.handle != 0x98u || m.parentHandle != 0x72u
        || m.m_name != "Global" || !m.name.empty() || !m.m_description.empty()
        || m.flags != 0u || m.reactorCount() != 1 || m.reactorHandles.size() != 1u
        || m.reactorHandles[0] != 0x72u || m.extensionDictionaryFlag() != 0u
        || m.xDictHandle != 0x110u || m.hasDataStorageBinaryData()
        || m.hasDwgCommonLinkTail() || !m.appData.empty() || m.extData.size() != 7u
        || !m.pendingAppIdResolutions.empty() || !m.pendingLayerRefResolutions.empty()
        || !m.hasCompleteDwgSemanticFields() || !m.hasDwgRawData()
        || m.dwgRawDataVersion() != DRW::AC1024 || m.dwgRawDataBitSize() != 1028u
        || m.dwgRawData().size() != 129u
        || raw.m_version != DRW::AC1024 || raw.m_objectType != 505u
        || raw.m_handle != m.handle || raw.m_isEntity || !raw.m_isCustomClass
        || !raw.m_hasClassDefinition || raw.m_recordName != "MATERIAL"
        || raw.m_className != "AcDbMaterial" || raw.m_classAppName != "ObjectDBX Classes"
        || raw.m_parentHandle != 0x72u || raw.m_numReactors != 1
        || raw.m_reactorHandles != m.reactorHandles || raw.m_xDictFlag != 0u
        || raw.m_xDictHandle != 0x110u || raw.m_hasDataStorage
        || !raw.m_commonHandleDataValidated
        || !drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence)
        || raw.m_objectSize != 181u || raw.m_rawBytes.size() != 181u
        || raw.m_bodyBitSize != 60u) return false;
    detail::Bits body{raw.m_rawBytes.data(), 0u, 1253u};
    if (body.take(2u) != 1u || body.take(8u) + 0x1f0u != 505u) return false;
    const auto self = detail::handle(body, 0u, 0u, true);
    if (self.role != 0u || self.length != 1u || self.value != m.handle
        || body.pos != 26u || !body.skip(188u) || body.pos != 214u
        || body.longValue() != 1u || body.take(1u) != 0u || body.pos != 225u)
        return false;
    for (std::uint32_t i = 0u; i < 1028u; ++i) {
        const auto rawBit = (raw.m_rawBytes[(225u + i) / 8u] >> (7u - (225u + i) % 8u)) & 1u;
        const auto modelBit = (m.dwgRawData()[i / 8u] >> (7u - i % 8u)) & 1u;
        if (rawBit != modelBit) return false;
    }
    detail::Bits strings{raw.m_rawBytes.data(), 1253u, 1371u};
    if (!detail::text(strings, m.m_name) || !detail::text(strings, m.m_description)
        || !detail::color(body, m.m_ambientColorMethod, m.m_ambientColorFactor, m.m_ambientColorValue,
                          m.m_hasAmbientColorMethod, m.m_hasAmbientColorFactor, m.m_hasAmbientColorValue)
        || !detail::color(body, m.m_diffuseColorMethod, m.m_diffuseColorFactor, m.m_diffuseColorValue,
                          m.m_hasDiffuseColorMethod, m.m_hasDiffuseColorFactor, m.m_hasDiffuseColorValue)) return false;
#define DX_GLOBAL_MAP(lower, Upper, expected, scaled)                                                     \
    detail::mapper(body, strings, m.m_##lower##MapBlendFactor, m.m_##lower##MapSource,                    \
                   m.m_##lower##MapFileName, m.m_##lower##MapProjection, m.m_##lower##MapTiling,         \
                   m.m_##lower##MapAutoTransform, m.m_##lower##MapTransform,                             \
                   m.m_has##Upper##MapBlendFactor, m.m_has##Upper##MapSource, m.m_has##Upper##MapFileName, \
                   m.m_has##Upper##MapProjection, m.m_has##Upper##MapTiling,                             \
                   m.m_has##Upper##MapAutoTransform, m.m_has##Upper##MapTransform, expected, 2u, scaled)
    const bool decoded = DX_GLOBAL_MAP(diffuse, Diffuse, 1u, true)
        && detail::color(body, m.m_specularColorMethod, m.m_specularColorFactor, m.m_specularColorValue,
                         m.m_hasSpecularColorMethod, m.m_hasSpecularColorFactor, m.m_hasSpecularColorValue)
        && DX_GLOBAL_MAP(specular, Specular, 1u, false) && m.m_hasSpecularGlossFactor
        && m.m_specularGlossFactor == 0.5 && detail::number(body, 0.5)
        && DX_GLOBAL_MAP(reflection, Reflection, 1u, true) && m.m_hasOpacityPercent
        && m.m_opacityPercent == 1.0 && detail::number(body, 1.0)
        && DX_GLOBAL_MAP(opacity, Opacity, 1u, true) && DX_GLOBAL_MAP(bump, Bump, 1u, true)
        && m.m_hasRefractionIndex && m.m_refractionIndex == 1.0 && detail::number(body, 1.0)
        && DX_GLOBAL_MAP(refraction, Refraction, 0u, false) && m.m_translucence == 0.0
        && !m.m_hasTranslucence && detail::number(body, 0.0) && detail::number(body, 0.0)
        && m.m_reflectivity == 0.0 && !m.m_hasReflectivity && detail::number(body, 0.0)
        && m.m_illuminationModel == 0 && !m.m_hasIlluminationModel && body.longValue() == 0u
        && m.m_channelFlags == 63 && m.m_hasChannelFlags && body.longValue() == 63u && body.longValue() == 0u;
#undef DX_GLOBAL_MAP
    if (!decoded || !body.good || body.pos != body.end || !strings.good || strings.pos != strings.end) return false;
    detail::Bits footer{raw.m_rawBytes.data(), 1371u, 1388u};
    if (footer.little(2u) != 118u || footer.take(1u) != 1u || !footer.good) return false;
    detail::Bits links{raw.m_rawBytes.data(), 1388u, 1448u};
    const auto owner = detail::handle(links, 4u, m.handle);
    const auto reactor = detail::handle(links, 4u, m.handle);
    const auto extension = detail::handle(links, 3u, m.handle);
    return links.good && owner.role == 12u && owner.length == 1u && owner.value == 0x72u
        && reactor.role == 4u && reactor.length == 1u && reactor.value == 0x72u
        && extension.role == 3u && extension.length == 2u && extension.value == 0x110u
        && links.take(4u) == 15u && links.good && links.pos == links.end;
}
inline bool observeDictionary(const DRW_UnsupportedObject& raw, const DRW_Dictionary& source) noexcept {
    using dx_layerstates_dwg_wire::LinkKind;
    const bool root = source.handle == 0x72u;
    const bool byLayer = source.handle == 0x297u;
    if (!root && !byLayer && source.handle != 0x295u) {
        return false;
    }
    const std::uint32_t owner = root ? 0xcu : (byLayer ? 0x96u : 0x97u);
    if (source.tType != DRW::DICTIONARY || source.parentHandle != owner || !source.name.empty()
        || source.flags != 0u || source.reactorCount() != (root ? 1 : 0)
        || source.reactorHandles.size() != (root ? 1u : 0u) || (root && source.reactorHandles[0] != owner)
        || source.extensionDictionaryFlag() != 1u || source.xDictHandle != 0u
        || source.hasDataStorageBinaryData() || !source.appData.empty() || !source.extData.empty()
        || !source.pendingAppIdResolutions.empty() || !source.pendingLayerRefResolutions.empty()
        || source.countCap || !source.hasCompleteDwgEntries() || !source.hasDwgCommonLinkTail()
        || source.cloning != 1 || source.hardOwner != (root ? 0 : 1)
        || source.m_entries.size() != (root ? 9u : 1u) || raw.m_objectSize != (root ? 272u : 33u)
        || raw.m_rawBytes.size() != raw.m_objectSize || raw.m_bodyBitSize != (root ? 226u : 42u)
        || (raw.m_rawBytes.back() & 3u) != 3u) {
        return false;
    }
    static constexpr const char* names[]{
        "Brass - Satin 1", "Bronze - Satin 1",          "ByBlock",     "ByLayer", "Global", "Indoor Pool",
        "Plate - Blue",    "Plate - Riveted Flat Gray", "Rust - Heavy"};
    static constexpr std::uint32_t targets[]{0x2c2u, 0x2ccu, 0x97u,  0x96u, 0x98u,
                                             0x32eu, 0x2d6u, 0x2e0u, 0x2eau};
    for (std::size_t i = 0; i < source.m_entries.size(); ++i) {
        const auto& e = source.m_entries[i];
        if (e.m_name != (root ? names[i] : "FBXASSET") || e.m_handle != (root ? targets[i] : (byLayer ? 0x298u : 0x296u))
            || e.m_dxfHandleCode != DRW_Dictionary::Entry::DxfHandleCode::Unspecified) {
            return false;
        }
    }
    return dx_layerstates_dwg_wire::observeDictionary(
        raw, source, [](std::uint32_t, const std::string&) noexcept { return false; },
        [root](LinkKind kind, std::size_t index, std::uint8_t role, std::uint8_t length) noexcept {
            if (kind == LinkKind::Owner || kind == LinkKind::Reactor) {
                return index == 0u && (kind == LinkKind::Owner || root) && role == 4u && length == 1u;
            }
            return kind == LinkKind::Item && index < (root ? 9u : 1u) && role == 2u
                   && length == (root ? (targets[index] < 0x100u ? 1u : 2u) : 2u);
        });
}
inline bool observeXRecord(const DRW_UnsupportedObject& raw, const DRW_XRecord& source) noexcept {
    using namespace dx_raster_wire;
    const bool byLayer = source.handle == 0x298u;
    if (!byLayer && source.handle != 0x296u) return false;
    const auto extensionHandle = byLayer ? 0x297u : 0x295u;
    const std::uint32_t size = byLayer ? 24613u : 24611u;
    const std::uint32_t payloadEnd = byLayer ? 196857u : 196841u;
    const std::uint32_t handleStart = byLayer ? 196868u : 196852u;
    if (source.tType != DRW::XRECORD || source.parentHandle != extensionHandle
        || !source.name.empty() || source.flags != 0u || source.reactorCount() != 1
        || source.reactorHandles.size() != 1u || source.reactorHandles[0] != extensionHandle
        || source.extensionDictionaryFlag() != 1u || source.xDictHandle != 0u
        || source.hasDataStorageBinaryData() || !source.hasDwgCommonLinkTail() || !source.appData.empty()
        || !source.extData.empty() || !source.pendingAppIdResolutions.empty()
        || !source.pendingLayerRefResolutions.empty() || source.m_cloning != 1 || source.m_rawDataValid
        || !source.m_rawData.empty() || source.hasDwgRawDataSourceRecord() || !source.m_handleValues.empty()
        || !source.hasExactDxfCoordinateTuples() || source.m_values.size() != 193u
        || source.m_dataEntries.size() != 193u || raw.m_version != DRW::AC1024 || raw.m_objectType != 79u
        || raw.m_handle != source.handle || raw.m_isEntity || raw.m_isCustomClass
        || raw.m_hasClassDefinition || !raw.m_recordName.empty() || !raw.m_className.empty()
        || !raw.m_classAppName.empty() || raw.m_parentHandle != extensionHandle || raw.m_numReactors != 1
        || raw.m_reactorHandles != source.reactorHandles || raw.m_xDictFlag != 1u || raw.m_xDictHandle != 0u
        || raw.m_hasDataStorage || !raw.m_commonHandleDataValidated
        || !drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence) || raw.m_objectSize != size
        || raw.m_rawBytes.size() != size || raw.m_bodyBitSize != 36u) {
        return false;
    }
    detail::Bits body{raw.m_rawBytes.data(), 0u, handleStart};
    if (body.take(2u) != 0u || body.take(8u) != 79u) {
        return false;
    }
    const auto self = detail::handle(body, 0u, 0u, true);
    if (self.role != 0u || self.length != 2u || self.value != source.handle || body.shortValue() != 0u
        || body.longValue() != 1u || body.take(1u) != 1u || body.longValue() != (byLayer ? 24597u : 24595u) || body.pos != 81u) {
        return false;
    }
    detail::Bits payload{raw.m_rawBytes.data(), 81u, payloadEnd};
    const auto clean = [](const DRW_Variant& v) noexcept {
        return !v.dwgCmColorValue() && !v.isLayerRef() && !v.rawDwgLayerReference();
    };
    for (std::size_t i = 0; i < 193u; ++i) {
        const auto& a = source.m_dataEntries[i];
        const auto& b = source.m_values[i];
        const int code = i == 0u ? 70 : i == 1u ? 90 : i == 2u ? 71 : i == 3u ? 1 : 310;
        if (a.code() != code || b.code() != code || !clean(a) || !clean(b)
            || payload.little(2u) != static_cast<unsigned>(code)) {
            return false;
        }
        if (i < 3u) {
            const std::int32_t value = i == 1u ? 2070111275 : 1;
            if (a.type() != DRW_Variant::INTEGER || b.type() != DRW_Variant::INTEGER || a.i_val() != value
                || b.i_val() != value
                || payload.little(i == 1u ? 4u : 2u) != static_cast<std::uint32_t>(value)) {
                return false;
            }
        } else if (i == 3u) {
            if (a.type() != DRW_Variant::STRING || b.type() != DRW_Variant::STRING || !a.content.s
                || !b.content.s || a.content.s->size() != 36u || *a.content.s != *b.content.s
                || payload.little(2u) != 36u) {
                return false;
            }
            for (unsigned char c : *a.content.s) {
                if (c > 127u || payload.little(2u) != c) {
                    return false;
                }
            }
        } else {
            const std::size_t length = i == 192u ? (byLayer ? 64u : 62u) : 127u;
            if (a.type() != DRW_Variant::BINARY || b.type() != DRW_Variant::BINARY || !a.binary()
                || !b.binary() || a.binary()->size() != length || b.binary()->size() != length
                || payload.take(8u) != length) {
                return false;
            }
            for (std::size_t j = 0; j < length; ++j) {
                if (payload.take(8u) != (*a.binary())[j] || (*a.binary())[j] != (*b.binary())[j]) {
                    return false;
                }
            }
        }
        if (!payload.good) {
            return false;
        }
    }
    if (!payload.good || payload.pos != payload.end) {
        return false;
    }
    body.pos = payloadEnd;
    if (body.shortValue() != 1u || body.take(1u) != 0u || !body.good || body.pos != body.end) {
        return false;
    }
    detail::Bits links{raw.m_rawBytes.data(), handleStart, size * 8u};
    const auto owner = detail::handle(links, 4u, source.handle);
    const auto reactor = detail::handle(links, 4u, source.handle);
    return links.good && owner.role == 8u && owner.length == 0u && owner.value == extensionHandle
           && reactor.role == 4u && reactor.length == 2u && reactor.value == extensionHandle && links.take(4u) == 15u
           && links.good && links.pos == links.end;
}
} // namespace dx_ac1024_byblock_material_wire
#endif
