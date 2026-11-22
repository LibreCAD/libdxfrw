// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_LAYOUT_ASSOCIATIONS_H
#define DX_LAYOUT_ASSOCIATIONS_H

#include "drw_base.h"
#include <cstdint>
#include <map>
#include <string>

namespace dx_layout_associations {

enum class Mode { Reciprocal, LegacyDwgForwardOnly };

// The block-header layout handle is absent on the R13/R14 DWG wire. DXF
// sources and R2000+ DWG sources still require the reciprocal edge.
inline Mode sourceMode(bool dwg, DRW::Version version) noexcept {
    return dwg && (version == DRW::AC1012 || version == DRW::AC1014)
        ? Mode::LegacyDwgForwardOnly : Mode::Reciprocal;
}

// Resolve source associations without changing either the source models or
// the caller's result on failure. A successful result is not output approval:
// source-frame, dictionary, target-format, and output checks remain required.
template<class Layouts, class Records>
bool resolve(const Layouts& layouts, const Records& records, Mode mode,
             std::map<std::uint32_t, std::uint32_t>& associations,
             std::string& failure) {
    std::map<std::uint32_t, std::uint32_t> staged;
    for (const auto& item : layouts) {
        const auto& layout = *item.second;
        const auto handle = layout.paperSpaceBlockRecordHandle.ref;
        const auto record = records.find(handle);
        if (!handle || record == records.end()) {
            failure = "LAYOUT associated BLOCK_RECORD is null or missing";
            return false;
        }
        if (mode == Mode::Reciprocal
            && record->second->layoutHandle != layout.handle) {
            failure = "LAYOUT and BLOCK_RECORD backlinks disagree";
            return false;
        }
        if (!staged.emplace(handle, layout.handle).second) {
            failure = "multiple LAYOUT records share one BLOCK_RECORD";
            return false;
        }
    }
    for (const auto& item : records) {
        if (!item.second->layoutHandle)
            continue;
        if (mode == Mode::LegacyDwgForwardOnly) {
            failure = "R13/R14 DWG BLOCK_RECORD has an unexpected layout backlink";
            return false;
        }
        const auto layout = layouts.find(item.second->layoutHandle);
        if (layout == layouts.end()
            || layout->second->paperSpaceBlockRecordHandle.ref != item.first) {
            failure = "BLOCK_RECORD has a missing or nonreciprocal LAYOUT target";
            return false;
        }
    }
    associations.swap(staged);
    return true;
}
} // namespace dx_layout_associations
#endif
