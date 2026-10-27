// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFRENDERSETTINGS_H
#define DXFRENDERSETTINGS_H

#include "drw_objects.h"
#include "dxfreader.h"
#include "dxfwriter.h"
#include <array>
#include <cmath>
#include <string_view>

// Keep the positional vectors for existing consumers, but project named fields
// only from their original base subclass. Derived and foreign repeated codes
// cannot supply missing base fields. This is not a full rendering certificate.
namespace dxfRenderSettings {
enum class PresetProfile { Unavailable, RapidRT, LegacyMentalRay, ModernMentalRay };

inline PresetProfile presetProfile(const DRW_RenderSettings& value) noexcept {
    if (value.hasDwgRawData() || !value.m_bytes.empty()) return PresetProfile::Unavailable;
    for (double number : value.m_doubles)
        if (!std::isfinite(number)) return PresetProfile::Unavailable;
    for (auto number : value.m_shorts)
        if (number < -32768 || number > 32767) return PresetProfile::Unavailable;
    if (value.m_kind == DRW_RenderSettings::RapidRT && value.m_longs.size() == 5u
        && value.m_strings.size() == 3u && value.m_shorts.size() == 3u
        && value.m_doubles.size() == 2u && (value.m_bools.size() == 4u || value.m_bools.size() == 5u))
        return PresetProfile::RapidRT;
    if (value.m_kind == DRW_RenderSettings::MentalRay && value.m_longs.size() == 16u
        && value.m_shorts.size() == 7u && value.m_doubles.size() == 11u) {
        if (value.m_strings.size() == 3u && value.m_bools.size() == 12u)
            return PresetProfile::LegacyMentalRay;
        if (value.m_strings.size() == 4u && (value.m_bools.size() == 13u || value.m_bools.size() == 14u))
            return PresetProfile::ModernMentalRay;
    }
    return PresetProfile::Unavailable;
}

inline bool presetProjectionMatches(const DRW_RenderSettings& value,
                                     PresetProfile profile) noexcept {
    if (profile == PresetProfile::Unavailable) return false;
    const bool legacy = profile == PresetProfile::LegacyMentalRay;
    const bool predefined = !legacy && value.m_bools.size()
        == (profile == PresetProfile::RapidRT ? 5u : 14u);
    return value.m_classVersion == value.m_longs[0]
        && value.m_name == value.m_strings[0]
        && value.m_environmentImageFilename == value.m_strings[1]
        && (legacy ? value.m_description.empty()
                   : value.m_description == value.m_strings[2])
        && value.m_displayIndex == (legacy ? 0 : value.m_longs[1])
        && value.m_fogEnabled == value.m_bools[0]
        && value.m_fogBackgroundEnabled == (!legacy && value.m_bools[1])
        && value.m_backfacesEnabled == value.m_bools[legacy ? 1u : 2u]
        && value.m_environmentImageEnabled == value.m_bools[legacy ? 2u : 3u]
        && value.m_hasPredefinedPresent == predefined
        && value.m_hasPredefined == (predefined && value.m_bools[4]);
}

inline void writePresetBody(dxfWriter& writer, const DRW_RenderSettings& value,
                            PresetProfile profile) {
    std::size_t longs=0u, strings=0u, bools=0u, shorts=0u, doubles=0u;
    const auto emit=[&](int code) {
        switch(code) {
        case 90: writer.writeInt32(code,value.m_longs[longs++]); break;
        case 1: writer.writeUtf8String(code,value.m_strings[strings++]); break;
        case 290: writer.writeBool(code,value.m_bools[bools++]); break;
        case 70: writer.writeInt16(code,value.m_shorts[shorts++]); break;
        case 40: writer.writeDouble(code,value.m_doubles[doubles++]); break;
        default: break;
        }
    };
    writer.writeString(100,"AcDbRenderSettings");
    if (profile == PresetProfile::LegacyMentalRay) {
        for (int code : {90,1,290,90,290,290,1}) emit(code);
    } else {
        for (int code : {90,1,290,290,290,290,1,1,90}) emit(code);
        if (value.m_bools.size() == (profile == PresetProfile::RapidRT ? 5u : 14u)) emit(290);
    }
    if (profile == PresetProfile::RapidRT) {
        writer.writeString(100,"AcDbRapidRTRenderSettings");
        for (int code : {90,70,90,90,70,70,40,40}) emit(code);
    } else {
        writer.writeString(100,"AcDbMentalRayRenderSettings");
        for (int code : {90,90,90,70,40,40,40,40,40,40,70,290,290,90,90,90,
                         290,90,290,40,90,90,90,90,290,90,290,290,290,40,40,40,
                         70,70,40,70,70,290,1,90,70,90}) emit(code);
    }
}

class Decoder {
    DRW_RenderSettings& value;
    bool base = true; // Compatibility for markerless DXF bodies.
    std::size_t strings = 0u, longs = 0u, bools = 0u, bytes = 0u, doubles = 0u;
    std::array<std::int32_t, 3> baseLongs{};
    std::array<bool, 5> baseBools{};
    std::array<std::int32_t, 3> baseBytes{};
    std::array<double, 4> baseDoubles{};

    bool preset() const noexcept {
        return value.m_kind == DRW_RenderSettings::Settings
            || value.m_kind == DRW_RenderSettings::RapidRT
            || value.m_kind == DRW_RenderSettings::MentalRay;
    }
    std::string_view baseSubclass() const noexcept {
        switch (value.m_kind) {
        case DRW_RenderSettings::Global: return "AcDbRenderGlobal";
        case DRW_RenderSettings::Environment: return "AcDbRenderEnvironment";
        case DRW_RenderSettings::Entry: return "AcDbRenderEntry";
        default: return "AcDbRenderSettings";
        }
    }
public:
    struct Snapshot {
        bool base{false};
        // Strings, longs90, bools290, bytes280 and doubles40 in the base scope.
        std::array<std::size_t,5> counts{};
        std::array<std::int32_t,3> longs{},bytes{};
        std::array<bool,5> bools{};
        std::array<double,4> doubles{};
    };
    Snapshot snapshot() const noexcept {
        return {base,{strings,longs,bools,bytes,doubles},baseLongs,baseBytes,baseBools,baseDoubles};
    }
    explicit Decoder(DRW_RenderSettings& model) noexcept : value(model) {}
    bool parseCode(int code, const std::unique_ptr<dxfReader>& reader) {
        if (code == 100) base = reader->getStringView() == baseSubclass();
        unsigned roles = 0u;
        if (base && code == 1) {
            if (strings == 0u) roles |= 1u;
            if ((preset() && strings == 1u)
                || (value.m_kind == DRW_RenderSettings::Environment
                    && strings == 0u)) roles |= 2u;
            if (preset() && strings == 2u) roles |= 4u;
        }
        if (!value.parseDxfValue(code, reader, roles)) return false;
        if (!base) return true;
        switch (code) {
        case 1: ++strings; break;
        case 90:
            if (longs < baseLongs.size()) baseLongs[longs] = reader->getInt32();
            ++longs; break;
        case 290:
            if (bools < baseBools.size()) baseBools[bools] = reader->getInt32() != 0;
            ++bools; break;
        case 280:
            if (bytes < baseBytes.size()) baseBytes[bytes] = reader->getInt32();
            ++bytes; break;
        case 40:
            if (doubles < baseDoubles.size()) baseDoubles[doubles] = reader->getDouble();
            ++doubles; break;
        default: break;
        }
        return true;
    }
    void finish() noexcept {
        value.m_classVersion = baseLongs[0];
        if (preset()) {
            // Legacy MentalRay DXF has a texture-quality90, three290 flags
            // and two strings. The modern shared base has four flags, three
            // strings and display-index90. Native DWG uses its own finalizer.
            const bool legacy = value.m_kind == DRW_RenderSettings::MentalRay
                && strings == 2u && longs == 2u && bools == 3u;
            value.m_fogEnabled = baseBools[0]; // historical materials alias
            value.m_fogBackgroundEnabled = !legacy && baseBools[1];
            value.m_backfacesEnabled = baseBools[legacy ? 1u : 2u];
            value.m_environmentImageEnabled = baseBools[legacy ? 2u : 3u];
            value.m_displayIndex = legacy ? 0 : baseLongs[1];
            value.m_hasPredefinedPresent = strings == 3u && longs == 2u && bools == 5u;
            value.m_hasPredefined = value.m_hasPredefinedPresent && baseBools[4];
        } else if (value.m_kind == DRW_RenderSettings::Environment) {
            value.m_fogEnabled = baseBools[0];
            value.m_fogBackgroundEnabled = baseBools[1];
            value.m_environmentImageEnabled = baseBools[2];
            value.m_fogColorR = baseBytes[0];
            value.m_fogColorG = baseBytes[1];
            value.m_fogColorB = baseBytes[2];
            value.m_fogDensityNear = baseDoubles[0];
            value.m_fogDensityFar = baseDoubles[1];
            value.m_fogDistanceNear = baseDoubles[2];
            value.m_fogDistanceFar = baseDoubles[3];
        } else if (value.m_kind == DRW_RenderSettings::Global) {
            value.m_procedure = baseLongs[1];
            value.m_destination = baseLongs[2];
        }
    }
};
}
#endif
