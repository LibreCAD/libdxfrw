// SPDX-License-Identifier: GPL-2.0-or-later
// Internal MATERIAL body codec. Common identity, graph and publication stay
// with the caller. DXF roles follow Autodesk's MATERIAL group-code table;
// this helper is not a native DWG/renderer schema qualification.
#ifndef LIBDXFRW_DXFMATERIAL_H
#define LIBDXFRW_DXFMATERIAL_H
#include <algorithm>
#include <cmath>
#include <cstring>
#include "../drw_objects.h"
#include "dxfwriter.h"
namespace dxfMaterial {
inline bool validBody(const DRW_Material& value) {
    const auto* ent = &value;
    // The reused codes cannot be emitted without their actual boundary field.
    // Never invent a luminance/default value to disambiguate an incomplete model.
    const bool normalBoundary=ent->m_hasLuminance||ent->m_hasNormalMapStrength;
    if((!normalBoundary&&(ent->m_hasLuminanceMode||ent->m_hasNormalMapMethod))
        ||(!normalBoundary&&(ent->m_hasNormalMapBlendFactor||ent->m_hasNormalMapSource
            ||ent->m_hasNormalMapFileName||ent->m_hasNormalMapProjection||ent->m_hasNormalMapTiling
            ||ent->m_hasNormalMapAutoTransform||ent->m_hasNormalMapTransform
            ||ent->m_hasGlobalIlluminationMode||ent->m_hasFinalGatherMode||ent->m_hasMapUTile)))return false;
    if ((ent->m_hasAmbientColorMethod
            && ent->m_ambientColorMethod > 1u)
        || (ent->m_hasAmbientColorFactor
            && (!std::isfinite(ent->m_ambientColorFactor)
                || ent->m_ambientColorFactor < 0.0
                || ent->m_ambientColorFactor > 1.0))
        || (ent->m_hasDiffuseColorMethod
            && ent->m_diffuseColorMethod > 1u)
        || (ent->m_hasDiffuseColorFactor
            && (!std::isfinite(ent->m_diffuseColorFactor)
                || ent->m_diffuseColorFactor < 0.0
                || ent->m_diffuseColorFactor > 1.0))
        || (ent->m_hasDiffuseMapBlendFactor
            && !std::isfinite(ent->m_diffuseMapBlendFactor))
        || (ent->m_hasDiffuseMapSource
            && ent->m_diffuseMapSource > 1u)
        || (ent->m_hasDiffuseMapProjection
            && (ent->m_diffuseMapProjection < 1u
                || ent->m_diffuseMapProjection > 4u))
        || (ent->m_hasDiffuseMapTiling
            && (ent->m_diffuseMapTiling < 1u
                || ent->m_diffuseMapTiling > 3u))
        || (ent->m_hasDiffuseMapAutoTransform
            && (ent->m_diffuseMapAutoTransform & ~7u) != 0u)
        || (ent->m_hasDiffuseMapTransform
            && !std::all_of(ent->m_diffuseMapTransform.begin(),
                            ent->m_diffuseMapTransform.end(),
                [](double value) { return std::isfinite(value); }))
        || (ent->m_hasSpecularGlossFactor
            && !std::isfinite(ent->m_specularGlossFactor))
        || (ent->m_hasSpecularColorMethod
            && ent->m_specularColorMethod > 1u)
        || (ent->m_hasSpecularColorFactor
            && (!std::isfinite(ent->m_specularColorFactor)
                || ent->m_specularColorFactor < 0.0
                || ent->m_specularColorFactor > 1.0))
        || (ent->m_hasSpecularMapBlendFactor
            && !std::isfinite(ent->m_specularMapBlendFactor))
        || (ent->m_hasSpecularMapSource
            && ent->m_specularMapSource > 1u)
        || (ent->m_hasSpecularMapProjection
            && (ent->m_specularMapProjection < 1u
                || ent->m_specularMapProjection > 4u))
        || (ent->m_hasSpecularMapTiling
            && (ent->m_specularMapTiling < 1u
                || ent->m_specularMapTiling > 3u))
        || (ent->m_hasSpecularMapAutoTransform
            && (ent->m_specularMapAutoTransform & ~7u) != 0u)
        || (ent->m_hasSpecularMapTransform
            && !std::all_of(ent->m_specularMapTransform.begin(),
                            ent->m_specularMapTransform.end(),
                [](double value) { return std::isfinite(value); }))
        || (ent->m_hasReflectionMapBlendFactor
            && !std::isfinite(ent->m_reflectionMapBlendFactor))
        || (ent->m_hasReflectionMapSource
            && ent->m_reflectionMapSource > 1u)
        || (ent->m_hasReflectionMapProjection
            && (ent->m_reflectionMapProjection < 1u
                || ent->m_reflectionMapProjection > 4u))
        || (ent->m_hasReflectionMapTiling
            && (ent->m_reflectionMapTiling < 1u
                || ent->m_reflectionMapTiling > 3u))
        || (ent->m_hasReflectionMapAutoTransform
            && (ent->m_reflectionMapAutoTransform & ~7u) != 0u)
        || (ent->m_hasReflectionMapTransform
            && !std::all_of(ent->m_reflectionMapTransform.begin(),
                            ent->m_reflectionMapTransform.end(),
                [](double value) { return std::isfinite(value); }))
        || (ent->m_hasOpacityPercent
            && !std::isfinite(ent->m_opacityPercent))
        || (ent->m_hasOpacityMapBlendFactor
            && !std::isfinite(ent->m_opacityMapBlendFactor))
        || (ent->m_hasOpacityMapSource
            && ent->m_opacityMapSource > 1u)
        || (ent->m_hasOpacityMapProjection
            && (ent->m_opacityMapProjection < 1u
                || ent->m_opacityMapProjection > 4u))
        || (ent->m_hasOpacityMapTiling
            && (ent->m_opacityMapTiling < 1u
                || ent->m_opacityMapTiling > 3u))
        || (ent->m_hasOpacityMapAutoTransform
            && (ent->m_opacityMapAutoTransform & ~7u) != 0u)
        || (ent->m_hasOpacityMapTransform
            && !std::all_of(ent->m_opacityMapTransform.begin(),
                            ent->m_opacityMapTransform.end(),
                [](double value) { return std::isfinite(value); }))
        || (ent->m_hasBumpMapBlendFactor
            && !std::isfinite(ent->m_bumpMapBlendFactor))
        || (ent->m_hasBumpMapSource
            && ent->m_bumpMapSource > 1u)
        || (ent->m_hasBumpMapProjection
            && (ent->m_bumpMapProjection < 1u
                || ent->m_bumpMapProjection > 4u))
        || (ent->m_hasBumpMapTiling
            && (ent->m_bumpMapTiling < 1u
                || ent->m_bumpMapTiling > 3u))
        || (ent->m_hasBumpMapAutoTransform
            && (ent->m_bumpMapAutoTransform & ~7u) != 0u)
        || (ent->m_hasBumpMapTransform
            && !std::all_of(ent->m_bumpMapTransform.begin(),
                            ent->m_bumpMapTransform.end(),
                [](double value) { return std::isfinite(value); }))
        || (ent->m_hasRefractionIndex
            && !std::isfinite(ent->m_refractionIndex))
        || (ent->m_hasRefractionMapBlendFactor
            && !std::isfinite(ent->m_refractionMapBlendFactor))
        || (ent->m_hasRefractionMapSource
            && ent->m_refractionMapSource > 1u)
        || (ent->m_hasRefractionMapProjection
            && (ent->m_refractionMapProjection < 1u
                || ent->m_refractionMapProjection > 4u))
        || (ent->m_hasRefractionMapTiling
            && (ent->m_refractionMapTiling < 1u
                || ent->m_refractionMapTiling > 3u))
        || (ent->m_hasRefractionMapAutoTransform
            && (ent->m_refractionMapAutoTransform & ~7u) != 0u)
        || (ent->m_hasRefractionMapTransform
            && !std::all_of(ent->m_refractionMapTransform.begin(),
                            ent->m_refractionMapTransform.end(),
                [](double value) { return std::isfinite(value); }))
        || (ent->m_hasColorBleedScale
            && !std::isfinite(ent->m_colorBleedScale))
        || (ent->m_hasIndirectDumpScale
            && !std::isfinite(ent->m_indirectDumpScale))
        || (ent->m_hasReflectanceScale
            && !std::isfinite(ent->m_reflectanceScale))
        || (ent->m_hasTransmittanceScale
            && !std::isfinite(ent->m_transmittanceScale))
        || (ent->m_hasLuminance && !std::isfinite(ent->m_luminance))
        || (ent->m_hasNormalMapStrength
            && !std::isfinite(ent->m_normalMapStrength))
        || (ent->m_hasNormalMapBlendFactor
            && !std::isfinite(ent->m_normalMapBlendFactor))
        || (ent->m_hasNormalMapSource && ent->m_normalMapSource > 1u)
        || (ent->m_hasNormalMapProjection
            && (ent->m_normalMapProjection < 1u
                || ent->m_normalMapProjection > 4u))
        || (ent->m_hasNormalMapTiling
            && (ent->m_normalMapTiling < 1u
                || ent->m_normalMapTiling > 3u))
        || (ent->m_hasNormalMapAutoTransform
            && (ent->m_normalMapAutoTransform & ~7u) != 0u)
        || (ent->m_hasNormalMapTransform
            && !std::all_of(ent->m_normalMapTransform.begin(),
                            ent->m_normalMapTransform.end(),
                [](double value) { return std::isfinite(value); }))
        || (ent->m_hasTranslucence && !std::isfinite(ent->m_translucence))
        || (ent->m_hasReflectivity && !std::isfinite(ent->m_reflectivity))
        || ((ent->m_hasMapUTile || ent->m_hasTranslucence
                || ent->m_hasReflectivity || ent->m_hasIlluminationModel
                || ent->m_hasChannelFlags)
            && !ent->m_hasGenProcTableEnd
            && !ent->hasDxfMaterialImplicitTail()
            && !ent->hasCompleteDwgSemanticFields()))
        return false;
    return true;
}
inline bool writeBody(dxfWriter& output, const DRW_Material& value, bool binary) {
    if (!validBody(value)) return false;
    const auto* ent = &value;
    auto* writer = &output;
    const auto writeUnsignedColor = [&](int code, std::uint32_t value) {
        if (!binary) return writer->writeRawString(code, std::to_string(value));
        std::int32_t signedValue = 0;
        std::memcpy(&signedValue, &value, sizeof(value));
        return writer->writeInt32(code, signedValue);
    };
    writer->writeUtf8String(1, ent->m_name);
    writer->writeUtf8String(2, ent->m_description);
    if (ent->m_hasAmbientColorMethod)
        writer->writeInt16(70, ent->m_ambientColorMethod);
    if (ent->m_hasAmbientColorFactor)
        writer->writeDouble(40, ent->m_ambientColorFactor);
    if (ent->m_hasAmbientColorValue
        && !writeUnsignedColor(90, ent->m_ambientColorValue))
        return false;
    if (ent->m_hasDiffuseColorMethod)
        writer->writeInt16(71, ent->m_diffuseColorMethod);
    if (ent->m_hasDiffuseColorFactor)
        writer->writeDouble(41, ent->m_diffuseColorFactor);
    if (ent->m_hasDiffuseColorValue
        && !writeUnsignedColor(91, ent->m_diffuseColorValue))
        return false;
    if (ent->m_hasDiffuseMapBlendFactor)
        writer->writeDouble(42, ent->m_diffuseMapBlendFactor);
    if (ent->m_hasDiffuseMapSource)
        writer->writeInt16(72, ent->m_diffuseMapSource);
    if (ent->m_hasDiffuseMapFileName)
        writer->writeUtf8String(3, ent->m_diffuseMapFileName);
    if (ent->m_hasDiffuseMapProjection)
        writer->writeInt16(73, ent->m_diffuseMapProjection);
    if (ent->m_hasDiffuseMapTiling)
        writer->writeInt16(74, ent->m_diffuseMapTiling);
    if (ent->m_hasDiffuseMapAutoTransform)
        writer->writeInt16(75, ent->m_diffuseMapAutoTransform);
    if (ent->m_hasDiffuseMapTransform) {
        for (double value : ent->m_diffuseMapTransform)
            writer->writeDouble(43, value);
    }
    if (ent->m_hasSpecularGlossFactor)
        writer->writeDouble(44, ent->m_specularGlossFactor);
    if (ent->m_hasSpecularColorMethod)
        writer->writeInt16(76, ent->m_specularColorMethod);
    if (ent->m_hasSpecularColorFactor)
        writer->writeDouble(45, ent->m_specularColorFactor);
    if (ent->m_hasSpecularColorValue
        && !writeUnsignedColor(92, ent->m_specularColorValue))
        return false;
    if (ent->m_hasSpecularMapBlendFactor)
        writer->writeDouble(46, ent->m_specularMapBlendFactor);
    if (ent->m_hasSpecularMapSource)
        writer->writeInt16(77, ent->m_specularMapSource);
    if (ent->m_hasSpecularMapFileName)
        writer->writeUtf8String(4, ent->m_specularMapFileName);
    if (ent->m_hasSpecularMapProjection)
        writer->writeInt16(78, ent->m_specularMapProjection);
    if (ent->m_hasSpecularMapTiling)
        writer->writeInt16(79, ent->m_specularMapTiling);
    if (ent->m_hasSpecularMapAutoTransform)
        writer->writeInt16(170, ent->m_specularMapAutoTransform);
    if (ent->m_hasSpecularMapTransform) {
        for (double value : ent->m_specularMapTransform)
            writer->writeDouble(47, value);
    }
    if (ent->m_hasReflectionMapBlendFactor)
        writer->writeDouble(48, ent->m_reflectionMapBlendFactor);
    if (ent->m_hasReflectionMapSource)
        writer->writeInt16(171, ent->m_reflectionMapSource);
    if (ent->m_hasReflectionMapFileName)
        writer->writeUtf8String(6, ent->m_reflectionMapFileName);
    if (ent->m_hasReflectionMapProjection)
        writer->writeInt16(172, ent->m_reflectionMapProjection);
    if (ent->m_hasReflectionMapTiling)
        writer->writeInt16(173, ent->m_reflectionMapTiling);
    if (ent->m_hasReflectionMapAutoTransform)
        writer->writeInt16(174, ent->m_reflectionMapAutoTransform);
    if (ent->m_hasReflectionMapTransform) {
        for (double value : ent->m_reflectionMapTransform)
            writer->writeDouble(49, value);
    }
    if (ent->m_hasOpacityPercent)
        writer->writeDouble(140, ent->m_opacityPercent);
    if (ent->m_hasOpacityMapBlendFactor)
        writer->writeDouble(141, ent->m_opacityMapBlendFactor);
    if (ent->m_hasOpacityMapSource)
        writer->writeInt16(175, ent->m_opacityMapSource);
    if (ent->m_hasOpacityMapFileName)
        writer->writeUtf8String(7, ent->m_opacityMapFileName);
    if (ent->m_hasOpacityMapProjection)
        writer->writeInt16(176, ent->m_opacityMapProjection);
    if (ent->m_hasOpacityMapTiling)
        writer->writeInt16(177, ent->m_opacityMapTiling);
    if (ent->m_hasOpacityMapAutoTransform)
        writer->writeInt16(178, ent->m_opacityMapAutoTransform);
    if (ent->m_hasOpacityMapTransform) {
        for (double value : ent->m_opacityMapTransform)
            writer->writeDouble(142, value);
    }
    if (ent->m_hasBumpMapBlendFactor)
        writer->writeDouble(143, ent->m_bumpMapBlendFactor);
    if (ent->m_hasBumpMapSource)
        writer->writeInt16(179, ent->m_bumpMapSource);
    if (ent->m_hasBumpMapFileName)
        writer->writeUtf8String(8, ent->m_bumpMapFileName);
    if (ent->m_hasBumpMapProjection)
        writer->writeInt16(270, ent->m_bumpMapProjection);
    if (ent->m_hasBumpMapTiling)
        writer->writeInt16(271, ent->m_bumpMapTiling);
    if (ent->m_hasBumpMapAutoTransform)
        writer->writeInt16(272, ent->m_bumpMapAutoTransform);
    if (ent->m_hasBumpMapTransform) {
        for (double value : ent->m_bumpMapTransform)
            writer->writeDouble(144, value);
    }
    if (ent->m_hasRefractionIndex)
        writer->writeDouble(145, ent->m_refractionIndex);
    if (ent->m_hasRefractionMapBlendFactor)
        writer->writeDouble(146, ent->m_refractionMapBlendFactor);
    if (ent->m_hasRefractionMapSource)
        writer->writeInt16(273, ent->m_refractionMapSource);
    if (ent->m_hasRefractionMapFileName)
        writer->writeUtf8String(9, ent->m_refractionMapFileName);
    if (ent->m_hasRefractionMapProjection)
        writer->writeInt16(274, ent->m_refractionMapProjection);
    if (ent->m_hasRefractionMapTiling)
        writer->writeInt16(275, ent->m_refractionMapTiling);
    if (ent->m_hasRefractionMapAutoTransform)
        writer->writeInt16(276, ent->m_refractionMapAutoTransform);
    if (ent->m_hasRefractionMapTransform) {
        for (double value : ent->m_refractionMapTransform)
            writer->writeDouble(147, value);
    }
    if (ent->m_hasColorBleedScale)
        writer->writeDouble(460, ent->m_colorBleedScale);
    if (ent->m_hasIndirectDumpScale)
        writer->writeDouble(461, ent->m_indirectDumpScale);
    if (ent->m_hasReflectanceScale)
        writer->writeDouble(462, ent->m_reflectanceScale);
    if (ent->m_hasTransmittanceScale)
        writer->writeDouble(463, ent->m_transmittanceScale);
    if (ent->m_hasTwoSidedMaterial)
        writer->writeBool(290, ent->m_twoSidedMaterial);
    if (ent->m_hasLuminance)
        writer->writeDouble(464, ent->m_luminance);
    // Without luminance, put the supplied alternate boundary before reused
    // 270/271. Otherwise a reader can mistake them for bump mapper fields.
    if (!ent->m_hasLuminance && ent->m_hasNormalMapStrength)
        writer->writeDouble(465, ent->m_normalMapStrength);
    if (ent->m_hasLuminanceMode)
        writer->writeInt16(270, ent->m_luminanceMode);
    if (ent->m_hasNormalMapMethod)
        writer->writeInt16(271, ent->m_normalMapMethod);
    if (ent->m_hasLuminance && ent->m_hasNormalMapStrength)
        writer->writeDouble(465, ent->m_normalMapStrength);
    if (ent->m_hasNormalMapBlendFactor)
        writer->writeDouble(42, ent->m_normalMapBlendFactor);
    if (ent->m_hasNormalMapSource)
        writer->writeInt16(72, ent->m_normalMapSource);
    if (ent->m_hasNormalMapFileName)
        writer->writeUtf8String(3, ent->m_normalMapFileName);
    if (ent->m_hasNormalMapProjection)
        writer->writeInt16(73, ent->m_normalMapProjection);
    if (ent->m_hasNormalMapTiling)
        writer->writeInt16(74, ent->m_normalMapTiling);
    if (ent->m_hasNormalMapAutoTransform)
        writer->writeInt16(75, ent->m_normalMapAutoTransform);
    if (ent->m_hasNormalMapTransform) {
        for (double value : ent->m_normalMapTransform)
            writer->writeDouble(43, value);
    }
    if (ent->m_hasMaterialsAnonymous)
        writer->writeBool(293, ent->m_materialsAnonymous);
    if (ent->m_hasGlobalIlluminationMode)
        writer->writeInt16(272, ent->m_globalIlluminationMode);
    if (ent->m_hasFinalGatherMode)
        writer->writeInt16(273, ent->m_finalGatherMode);
    if (ent->m_hasGenProcTableEnd)
        writer->writeBool(292, ent->m_genProcTableEnd);
    if (ent->m_hasMapUTile)
        writer->writeInt16(270, ent->m_mapUTile);
    if (ent->m_hasTranslucence)
        writer->writeDouble(148, ent->m_translucence);
    if (ent->m_hasReflectivity)
        writer->writeDouble(468, ent->m_reflectivity);
    if (ent->m_hasIlluminationModel)
        writer->writeInt32(93, ent->m_illuminationModel);
    if (ent->m_hasChannelFlags)
        writer->writeInt32(94, ent->m_channelFlags);
    return !writer->hasWriteError();
}
template<class T> inline bool sameValue(const T& a, const T& b) noexcept { return a == b; }
inline bool sameValue(double a, double b) noexcept { return std::memcmp(&a, &b, sizeof(a)) == 0; }
inline bool sameValue(const std::array<double,16>& a, const std::array<double,16>& b) noexcept {
    for (std::size_t i=0; i<a.size(); ++i) if (!sameValue(a[i],b[i])) return false;
    return true;
}
// All public body values, including inactive payloads, are frozen against
// callback edits. Parser cursor counters are deliberately not editable state.
inline bool sameBodyState(const DRW_Material& a,
                          const DRW_Material& b) noexcept {
    if (!sameValue(a.m_name, b.m_name)) return false;
    if (!sameValue(a.m_description, b.m_description)) return false;
    if (!sameValue(a.m_ambientColorMethod, b.m_ambientColorMethod)) return false;
    if (!sameValue(a.m_ambientColorFactor, b.m_ambientColorFactor)) return false;
    if (!sameValue(a.m_ambientColorValue, b.m_ambientColorValue)) return false;
    if (!sameValue(a.m_hasAmbientColorMethod, b.m_hasAmbientColorMethod)) return false;
    if (!sameValue(a.m_hasAmbientColorFactor, b.m_hasAmbientColorFactor)) return false;
    if (!sameValue(a.m_hasAmbientColorValue, b.m_hasAmbientColorValue)) return false;
    if (!sameValue(a.m_diffuseColorMethod, b.m_diffuseColorMethod)) return false;
    if (!sameValue(a.m_diffuseColorFactor, b.m_diffuseColorFactor)) return false;
    if (!sameValue(a.m_diffuseColorValue, b.m_diffuseColorValue)) return false;
    if (!sameValue(a.m_hasDiffuseColorMethod, b.m_hasDiffuseColorMethod)) return false;
    if (!sameValue(a.m_hasDiffuseColorFactor, b.m_hasDiffuseColorFactor)) return false;
    if (!sameValue(a.m_hasDiffuseColorValue, b.m_hasDiffuseColorValue)) return false;
    if (!sameValue(a.m_diffuseMapBlendFactor, b.m_diffuseMapBlendFactor)) return false;
    if (!sameValue(a.m_diffuseMapSource, b.m_diffuseMapSource)) return false;
    if (!sameValue(a.m_diffuseMapFileName, b.m_diffuseMapFileName)) return false;
    if (!sameValue(a.m_diffuseMapProjection, b.m_diffuseMapProjection)) return false;
    if (!sameValue(a.m_diffuseMapTiling, b.m_diffuseMapTiling)) return false;
    if (!sameValue(a.m_diffuseMapAutoTransform, b.m_diffuseMapAutoTransform)) return false;
    if (!sameValue(a.m_hasDiffuseMapBlendFactor, b.m_hasDiffuseMapBlendFactor)) return false;
    if (!sameValue(a.m_hasDiffuseMapSource, b.m_hasDiffuseMapSource)) return false;
    if (!sameValue(a.m_hasDiffuseMapFileName, b.m_hasDiffuseMapFileName)) return false;
    if (!sameValue(a.m_hasDiffuseMapProjection, b.m_hasDiffuseMapProjection)) return false;
    if (!sameValue(a.m_hasDiffuseMapTiling, b.m_hasDiffuseMapTiling)) return false;
    if (!sameValue(a.m_hasDiffuseMapAutoTransform, b.m_hasDiffuseMapAutoTransform)) return false;
    if (!sameValue(a.m_diffuseMapTransform, b.m_diffuseMapTransform)) return false;
    if (!sameValue(a.m_hasDiffuseMapTransform, b.m_hasDiffuseMapTransform)) return false;
    if (!sameValue(a.m_specularGlossFactor, b.m_specularGlossFactor)) return false;
    if (!sameValue(a.m_specularColorMethod, b.m_specularColorMethod)) return false;
    if (!sameValue(a.m_specularColorFactor, b.m_specularColorFactor)) return false;
    if (!sameValue(a.m_specularColorValue, b.m_specularColorValue)) return false;
    if (!sameValue(a.m_hasSpecularGlossFactor, b.m_hasSpecularGlossFactor)) return false;
    if (!sameValue(a.m_hasSpecularColorMethod, b.m_hasSpecularColorMethod)) return false;
    if (!sameValue(a.m_hasSpecularColorFactor, b.m_hasSpecularColorFactor)) return false;
    if (!sameValue(a.m_hasSpecularColorValue, b.m_hasSpecularColorValue)) return false;
    if (!sameValue(a.m_specularMapBlendFactor, b.m_specularMapBlendFactor)) return false;
    if (!sameValue(a.m_specularMapSource, b.m_specularMapSource)) return false;
    if (!sameValue(a.m_specularMapFileName, b.m_specularMapFileName)) return false;
    if (!sameValue(a.m_specularMapProjection, b.m_specularMapProjection)) return false;
    if (!sameValue(a.m_specularMapTiling, b.m_specularMapTiling)) return false;
    if (!sameValue(a.m_specularMapAutoTransform, b.m_specularMapAutoTransform)) return false;
    if (!sameValue(a.m_hasSpecularMapBlendFactor, b.m_hasSpecularMapBlendFactor)) return false;
    if (!sameValue(a.m_hasSpecularMapSource, b.m_hasSpecularMapSource)) return false;
    if (!sameValue(a.m_hasSpecularMapFileName, b.m_hasSpecularMapFileName)) return false;
    if (!sameValue(a.m_hasSpecularMapProjection, b.m_hasSpecularMapProjection)) return false;
    if (!sameValue(a.m_hasSpecularMapTiling, b.m_hasSpecularMapTiling)) return false;
    if (!sameValue(a.m_hasSpecularMapAutoTransform, b.m_hasSpecularMapAutoTransform)) return false;
    if (!sameValue(a.m_specularMapTransform, b.m_specularMapTransform)) return false;
    if (!sameValue(a.m_hasSpecularMapTransform, b.m_hasSpecularMapTransform)) return false;
    if (!sameValue(a.m_reflectionMapBlendFactor, b.m_reflectionMapBlendFactor)) return false;
    if (!sameValue(a.m_reflectionMapSource, b.m_reflectionMapSource)) return false;
    if (!sameValue(a.m_reflectionMapFileName, b.m_reflectionMapFileName)) return false;
    if (!sameValue(a.m_reflectionMapProjection, b.m_reflectionMapProjection)) return false;
    if (!sameValue(a.m_reflectionMapTiling, b.m_reflectionMapTiling)) return false;
    if (!sameValue(a.m_reflectionMapAutoTransform, b.m_reflectionMapAutoTransform)) return false;
    if (!sameValue(a.m_hasReflectionMapBlendFactor, b.m_hasReflectionMapBlendFactor)) return false;
    if (!sameValue(a.m_hasReflectionMapSource, b.m_hasReflectionMapSource)) return false;
    if (!sameValue(a.m_hasReflectionMapFileName, b.m_hasReflectionMapFileName)) return false;
    if (!sameValue(a.m_hasReflectionMapProjection, b.m_hasReflectionMapProjection)) return false;
    if (!sameValue(a.m_hasReflectionMapTiling, b.m_hasReflectionMapTiling)) return false;
    if (!sameValue(a.m_hasReflectionMapAutoTransform, b.m_hasReflectionMapAutoTransform)) return false;
    if (!sameValue(a.m_reflectionMapTransform, b.m_reflectionMapTransform)) return false;
    if (!sameValue(a.m_hasReflectionMapTransform, b.m_hasReflectionMapTransform)) return false;
    if (!sameValue(a.m_opacityPercent, b.m_opacityPercent)) return false;
    if (!sameValue(a.m_opacityMapBlendFactor, b.m_opacityMapBlendFactor)) return false;
    if (!sameValue(a.m_opacityMapSource, b.m_opacityMapSource)) return false;
    if (!sameValue(a.m_opacityMapFileName, b.m_opacityMapFileName)) return false;
    if (!sameValue(a.m_opacityMapProjection, b.m_opacityMapProjection)) return false;
    if (!sameValue(a.m_opacityMapTiling, b.m_opacityMapTiling)) return false;
    if (!sameValue(a.m_opacityMapAutoTransform, b.m_opacityMapAutoTransform)) return false;
    if (!sameValue(a.m_hasOpacityPercent, b.m_hasOpacityPercent)) return false;
    if (!sameValue(a.m_hasOpacityMapBlendFactor, b.m_hasOpacityMapBlendFactor)) return false;
    if (!sameValue(a.m_hasOpacityMapSource, b.m_hasOpacityMapSource)) return false;
    if (!sameValue(a.m_hasOpacityMapFileName, b.m_hasOpacityMapFileName)) return false;
    if (!sameValue(a.m_hasOpacityMapProjection, b.m_hasOpacityMapProjection)) return false;
    if (!sameValue(a.m_hasOpacityMapTiling, b.m_hasOpacityMapTiling)) return false;
    if (!sameValue(a.m_hasOpacityMapAutoTransform, b.m_hasOpacityMapAutoTransform)) return false;
    if (!sameValue(a.m_opacityMapTransform, b.m_opacityMapTransform)) return false;
    if (!sameValue(a.m_hasOpacityMapTransform, b.m_hasOpacityMapTransform)) return false;
    if (!sameValue(a.m_bumpMapBlendFactor, b.m_bumpMapBlendFactor)) return false;
    if (!sameValue(a.m_bumpMapSource, b.m_bumpMapSource)) return false;
    if (!sameValue(a.m_bumpMapFileName, b.m_bumpMapFileName)) return false;
    if (!sameValue(a.m_bumpMapProjection, b.m_bumpMapProjection)) return false;
    if (!sameValue(a.m_bumpMapTiling, b.m_bumpMapTiling)) return false;
    if (!sameValue(a.m_bumpMapAutoTransform, b.m_bumpMapAutoTransform)) return false;
    if (!sameValue(a.m_hasBumpMapBlendFactor, b.m_hasBumpMapBlendFactor)) return false;
    if (!sameValue(a.m_hasBumpMapSource, b.m_hasBumpMapSource)) return false;
    if (!sameValue(a.m_hasBumpMapFileName, b.m_hasBumpMapFileName)) return false;
    if (!sameValue(a.m_hasBumpMapProjection, b.m_hasBumpMapProjection)) return false;
    if (!sameValue(a.m_hasBumpMapTiling, b.m_hasBumpMapTiling)) return false;
    if (!sameValue(a.m_hasBumpMapAutoTransform, b.m_hasBumpMapAutoTransform)) return false;
    if (!sameValue(a.m_bumpMapTransform, b.m_bumpMapTransform)) return false;
    if (!sameValue(a.m_hasBumpMapTransform, b.m_hasBumpMapTransform)) return false;
    if (!sameValue(a.m_refractionIndex, b.m_refractionIndex)) return false;
    if (!sameValue(a.m_refractionMapBlendFactor, b.m_refractionMapBlendFactor)) return false;
    if (!sameValue(a.m_refractionMapSource, b.m_refractionMapSource)) return false;
    if (!sameValue(a.m_refractionMapFileName, b.m_refractionMapFileName)) return false;
    if (!sameValue(a.m_refractionMapProjection, b.m_refractionMapProjection)) return false;
    if (!sameValue(a.m_refractionMapTiling, b.m_refractionMapTiling)) return false;
    if (!sameValue(a.m_refractionMapAutoTransform, b.m_refractionMapAutoTransform)) return false;
    if (!sameValue(a.m_hasRefractionIndex, b.m_hasRefractionIndex)) return false;
    if (!sameValue(a.m_hasRefractionMapBlendFactor, b.m_hasRefractionMapBlendFactor)) return false;
    if (!sameValue(a.m_hasRefractionMapSource, b.m_hasRefractionMapSource)) return false;
    if (!sameValue(a.m_hasRefractionMapFileName, b.m_hasRefractionMapFileName)) return false;
    if (!sameValue(a.m_hasRefractionMapProjection, b.m_hasRefractionMapProjection)) return false;
    if (!sameValue(a.m_hasRefractionMapTiling, b.m_hasRefractionMapTiling)) return false;
    if (!sameValue(a.m_hasRefractionMapAutoTransform, b.m_hasRefractionMapAutoTransform)) return false;
    if (!sameValue(a.m_refractionMapTransform, b.m_refractionMapTransform)) return false;
    if (!sameValue(a.m_hasRefractionMapTransform, b.m_hasRefractionMapTransform)) return false;
    if (!sameValue(a.m_colorBleedScale, b.m_colorBleedScale)) return false;
    if (!sameValue(a.m_indirectDumpScale, b.m_indirectDumpScale)) return false;
    if (!sameValue(a.m_reflectanceScale, b.m_reflectanceScale)) return false;
    if (!sameValue(a.m_transmittanceScale, b.m_transmittanceScale)) return false;
    if (!sameValue(a.m_hasColorBleedScale, b.m_hasColorBleedScale)) return false;
    if (!sameValue(a.m_hasIndirectDumpScale, b.m_hasIndirectDumpScale)) return false;
    if (!sameValue(a.m_hasReflectanceScale, b.m_hasReflectanceScale)) return false;
    if (!sameValue(a.m_hasTransmittanceScale, b.m_hasTransmittanceScale)) return false;
    if (!sameValue(a.m_twoSidedMaterial, b.m_twoSidedMaterial)) return false;
    if (!sameValue(a.m_hasTwoSidedMaterial, b.m_hasTwoSidedMaterial)) return false;
    if (!sameValue(a.m_materialsAnonymous, b.m_materialsAnonymous)) return false;
    if (!sameValue(a.m_hasMaterialsAnonymous, b.m_hasMaterialsAnonymous)) return false;
    if (!sameValue(a.m_luminance, b.m_luminance)) return false;
    if (!sameValue(a.m_luminanceMode, b.m_luminanceMode)) return false;
    if (!sameValue(a.m_normalMapMethod, b.m_normalMapMethod)) return false;
    if (!sameValue(a.m_normalMapStrength, b.m_normalMapStrength)) return false;
    if (!sameValue(a.m_hasLuminance, b.m_hasLuminance)) return false;
    if (!sameValue(a.m_hasLuminanceMode, b.m_hasLuminanceMode)) return false;
    if (!sameValue(a.m_hasNormalMapMethod, b.m_hasNormalMapMethod)) return false;
    if (!sameValue(a.m_hasNormalMapStrength, b.m_hasNormalMapStrength)) return false;
    if (!sameValue(a.m_normalMapBlendFactor, b.m_normalMapBlendFactor)) return false;
    if (!sameValue(a.m_normalMapSource, b.m_normalMapSource)) return false;
    if (!sameValue(a.m_normalMapFileName, b.m_normalMapFileName)) return false;
    if (!sameValue(a.m_normalMapProjection, b.m_normalMapProjection)) return false;
    if (!sameValue(a.m_normalMapTiling, b.m_normalMapTiling)) return false;
    if (!sameValue(a.m_normalMapAutoTransform, b.m_normalMapAutoTransform)) return false;
    if (!sameValue(a.m_hasNormalMapBlendFactor, b.m_hasNormalMapBlendFactor)) return false;
    if (!sameValue(a.m_hasNormalMapSource, b.m_hasNormalMapSource)) return false;
    if (!sameValue(a.m_hasNormalMapFileName, b.m_hasNormalMapFileName)) return false;
    if (!sameValue(a.m_hasNormalMapProjection, b.m_hasNormalMapProjection)) return false;
    if (!sameValue(a.m_hasNormalMapTiling, b.m_hasNormalMapTiling)) return false;
    if (!sameValue(a.m_hasNormalMapAutoTransform, b.m_hasNormalMapAutoTransform)) return false;
    if (!sameValue(a.m_normalMapTransform, b.m_normalMapTransform)) return false;
    if (!sameValue(a.m_hasNormalMapTransform, b.m_hasNormalMapTransform)) return false;
    if (!sameValue(a.m_globalIlluminationMode, b.m_globalIlluminationMode)) return false;
    if (!sameValue(a.m_finalGatherMode, b.m_finalGatherMode)) return false;
    if (!sameValue(a.m_hasGlobalIlluminationMode, b.m_hasGlobalIlluminationMode)) return false;
    if (!sameValue(a.m_hasFinalGatherMode, b.m_hasFinalGatherMode)) return false;
    if (!sameValue(a.m_genProcTableEnd, b.m_genProcTableEnd)) return false;
    if (!sameValue(a.m_hasGenProcTableEnd, b.m_hasGenProcTableEnd)) return false;
    if (!sameValue(a.m_mapUTile, b.m_mapUTile)) return false;
    if (!sameValue(a.m_hasMapUTile, b.m_hasMapUTile)) return false;
    if (!sameValue(a.m_translucence, b.m_translucence)) return false;
    if (!sameValue(a.m_hasTranslucence, b.m_hasTranslucence)) return false;
    if (!sameValue(a.m_reflectivity, b.m_reflectivity)) return false;
    if (!sameValue(a.m_hasReflectivity, b.m_hasReflectivity)) return false;
    if (!sameValue(a.m_illuminationModel, b.m_illuminationModel)) return false;
    if (!sameValue(a.m_hasIlluminationModel, b.m_hasIlluminationModel)) return false;
    if (!sameValue(a.m_channelFlags, b.m_channelFlags)) return false;
    if (!sameValue(a.m_hasChannelFlags, b.m_hasChannelFlags)) return false;
    return true;
}

inline bool sameState(const DRW_Material& a, const DRW_Material& b) noexcept {
    return sameBodyState(a, b)
        && a.hasCompleteDwgSemanticFields()
            == b.hasCompleteDwgSemanticFields()
        && a.hasDxfMaterialImplicitTail() == b.hasDxfMaterialImplicitTail()
        && a.hasDwgRawData()==b.hasDwgRawData() && a.dwgRawData()==b.dwgRawData()
        && a.dwgRawDataBitSize()==b.dwgRawDataBitSize() && a.dwgRawDataVersion()==b.dwgRawDataVersion();
}
} // namespace dxfMaterial
#endif
