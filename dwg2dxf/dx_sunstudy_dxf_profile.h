// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_SUNSTUDY_DXF_PROFILE_H
#define DX_SUNSTUDY_DXF_PROFILE_H

// Exact, no-reference DXF-origin SUNSTUDY profile. Unknown tags and metadata
// cannot be represented by the typed writer and therefore do not qualify.
#include "drw_objects.h"
#include "dx_standard_output.h"
#include <cstdint>
#include <limits>
#include <string_view>

namespace dx_sunstudy_dxf_profile {
inline bool matchesDictionary(const DRW_RawDxfObject& raw, const DRW_Dictionary& value) noexcept {
    // A repeated writer pass may add the standard ACAD_GROUP and
    // ACAD_PLOTSTYLENAME root edges alongside the SUNSTUDY dictionary edge.
    // The adapter's graph gate validates the allowed names and targets.
    if (raw.name != "DICTIONARY" || raw.handle != value.handle || raw.parentHandle != value.parentHandle
        || value.m_entries.size() > 3u) return false;
    const auto& g = raw.groups;
    const auto handle = [](const DRW_Variant& item, int code, std::uint32_t expected) {
        std::uint64_t parsed = 0u;
        return item.code() == code && dx_standard_output::handleValue(item, parsed) && parsed == expected;
    };
    const auto text = [](const DRW_Variant& item, int code, std::string_view expected) {
        return item.code() == code && item.type() == DRW_Variant::STRING && item.c_str()
            && expected == item.c_str();
    };
    const auto integer = [](const DRW_Variant& item, int code, int expected) {
        return item.code() == code && item.type() == DRW_Variant::INTEGER && item.content.i == expected;
    };
    // The generated empty GROUP dictionary may omit 280; nonempty source
    // dictionaries explicitly retain it and each ordered 3/350 pair.
    if (value.m_entries.empty())
        return (g.size() == 4u || g.size() == 5u) && value.hardOwner == 0
            && handle(g[0], 5, value.handle) && handle(g[1], 330, value.parentHandle)
            && text(g[2], 100, "AcDbDictionary")
            && (g.size() == 4u ? integer(g[3], 281, value.cloning)
                               : integer(g[3], 280, 0) && integer(g[4], 281, value.cloning));
    if (g.size() != 5u + 2u * value.m_entries.size()
        || !handle(g[0], 5, value.handle) || !handle(g[1], 330, value.parentHandle)
        || !text(g[2], 100, "AcDbDictionary") || !integer(g[3], 280, value.hardOwner)
        || !integer(g[4], 281, value.cloning)) return false;
    for (std::size_t i = 0; i < value.m_entries.size(); ++i) {
        const auto& entry = value.m_entries[i];
        if (!text(g[5u + 2u * i], 3, entry.m_name)
            || entry.m_dxfHandleCode != DRW_Dictionary::Entry::DxfHandleCode::SoftOwner
            || !handle(g[6u + 2u * i], 350, entry.m_handle)) return false;
    }
    return true;
}
inline bool samePayload(const DRW_SunStudy& a, const DRW_SunStudy& b) noexcept {
    if (a.m_classVersion != b.m_classVersion || a.m_setupName != b.m_setupName
        || a.m_description != b.m_description || a.m_outputType != b.m_outputType
        || a.m_useSubset != b.m_useSubset || a.m_sheetSetName != b.m_sheetSetName
        || a.m_sheetSubsetName != b.m_sheetSubsetName
        || a.m_selectDatesFromCalendar != b.m_selectDatesFromCalendar
        || a.m_selectRangeOfDates != b.m_selectRangeOfDates || a.m_startTime != b.m_startTime
        || a.m_endTime != b.m_endTime || a.m_interval != b.m_interval
        || a.m_hours != b.m_hours || a.m_shadePlotType != b.m_shadePlotType
        || a.m_viewportCount != b.m_viewportCount || a.m_rowCount != b.m_rowCount
        || a.m_columnCount != b.m_columnCount || a.m_spacing != b.m_spacing
        || a.m_lockViewports != b.m_lockViewports || a.m_labelViewports != b.m_labelViewports
        || a.m_pageSetupWizardHandle != b.m_pageSetupWizardHandle || a.m_viewHandle != b.m_viewHandle
        || a.m_visualStyleHandle != b.m_visualStyleHandle || a.m_textStyleHandle != b.m_textStyleHandle
        || a.m_dates.size() != b.m_dates.size()) return false;
    for (std::size_t i = 0; i < a.m_dates.size(); ++i)
        if (a.m_dates[i].m_julianDay != b.m_dates[i].m_julianDay
            || a.m_dates[i].m_milliseconds != b.m_dates[i].m_milliseconds) return false;
    return true;
}
inline bool matches(const DRW_RawDxfObject& raw, const DRW_SunStudy& value) noexcept {
    if (raw.name != "SUNSTUDY" || value.tType != DRW::SUNSTUDY
        || raw.handle == 0u || raw.handle == DRW::NoHandle || raw.handle != value.handle
        || raw.parentHandle == 0u || raw.parentHandle == DRW::NoHandle
        || raw.parentHandle != value.parentHandle || value.xDictHandle != 0u
        || !value.extData.empty() || value.reactorHandles.size() != 1u
        || value.reactorHandles.front() != value.parentHandle
        || !value.hasCompleteDxfArrays() || value.m_outputType != 1
        || value.m_useSubset || !value.m_sheetSetName.empty() || !value.m_sheetSubsetName.empty()
        || value.m_selectRangeOfDates || value.m_startTime != 0 || value.m_endTime != 0
        || value.m_interval != 0 || value.m_pageSetupWizardHandle != 0u
        || value.m_viewHandle != 0u || value.m_visualStyleHandle != 0u
        || value.m_textStyleHandle != 0u || value.m_dates.size() > DRW_SunStudy::kMaxDateCount
        || value.m_hours.size() > DRW_SunStudy::kMaxHourCount)
        return false;
    std::size_t pos = 0u;
    const auto field = [&](int code) -> const DRW_Variant* {
        if (pos >= raw.groups.size() || raw.groups[pos].code() != code) return nullptr;
        return &raw.groups[pos++];
    };
    const auto string = [&](int code, std::string_view expected) {
        const auto* item = field(code);
        return item && item->type() == DRW_Variant::STRING && item->c_str()
            && expected == item->c_str();
    };
    const auto integer = [&](int code, std::int32_t expected) {
        const auto* item = field(code);
        return item && item->type() == DRW_Variant::INTEGER && item->content.i == expected;
    };
    const auto handle = [&](int code, std::uint32_t expected) {
        const auto* item = field(code);
        std::uint64_t parsed = 0u;
        return item && dx_standard_output::handleValue(*item, parsed) && parsed == expected;
    };
    if (!handle(5, value.handle)) return false;
    // Both the authored source and the writer's emitted ordering represent
    // exactly one parent and one reactor, with the same identity.
    const bool ownerFirst = pos < raw.groups.size() && raw.groups[pos].code() == 330;
    if ((ownerFirst && !handle(330, value.parentHandle))
        || !string(102, "{ACAD_REACTORS") || !handle(330, value.parentHandle)
        || !string(102, "}") || (!ownerFirst && !handle(330, value.parentHandle))
        || !string(100, "AcDbSunStudy")
        || !integer(90, value.m_classVersion) || !string(1, value.m_setupName)
        || !string(2, value.m_description) || !integer(70, value.m_outputType)
        || !integer(291, value.m_selectDatesFromCalendar ? 1 : 0)
        || !integer(91, static_cast<std::int32_t>(value.m_dates.size())))
        return false;
    for (const auto& date : value.m_dates)
        if (date.m_milliseconds < 0 || date.m_milliseconds % 1000 != 0
            || !integer(90, date.m_julianDay) || !integer(90, date.m_milliseconds / 1000))
            return false;
    if (!integer(292, 0) || !integer(73, static_cast<std::int32_t>(value.m_hours.size())))
        return false;
    for (const bool hour : value.m_hours)
        if (!integer(290, hour ? 1 : 0)) return false;
    const auto* spacing = [&]() -> const DRW_Variant* {
        if (!integer(74, value.m_shadePlotType) || !integer(75, value.m_viewportCount)
            || !integer(76, value.m_rowCount) || !integer(77, value.m_columnCount)) return nullptr;
        return field(40);
    }();
    return spacing && spacing->type() == DRW_Variant::DOUBLE && spacing->content.d == value.m_spacing
        && integer(293, value.m_lockViewports ? 1 : 0)
        && integer(294, value.m_labelViewports ? 1 : 0) && pos == raw.groups.size();
}
}
#endif
