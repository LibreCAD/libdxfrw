// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_LEGACY_FRAME_CENSUS_H
#define DX_LEGACY_FRAME_CENSUS_H

#include "libdwgr.h"
#include <algorithm>
#include <cstdint>
#include <map>
#include <vector>

namespace dx_legacy_frame_census {

// Optional, bounded evidence from one reader attempt. A sealed census describes
// published object-map frame types; it does not authorize output or prove that
// an unused CLASS can be discarded. Unknown/incomplete capture fails closed.
class Capture {
public:
    static constexpr std::size_t MaxFrames = 65536u;

    void begin(std::uint64_t generation, std::uint64_t attempt) noexcept {
        rows_.clear();
        generation_ = generation;
        attempt_ = attempt;
        version_ = DRW::UNKNOWNV;
        valid_ = generation != 0u && attempt != 0u;
        sealed_ = false;
    }

    void observe(const DRW_DwgFramePublication& value,
                 std::uint64_t generation, std::uint64_t attempt) noexcept {
        if (!valid_ || sealed_)
            return invalidate();
        if (generation != generation_ || attempt != attempt_
            || (value.m_version != DRW::AC1012 && value.m_version != DRW::AC1014)
            || (version_ != DRW::UNKNOWNV && value.m_version != version_)
            || value.m_handle == 0u || value.m_handle == DRW::NoHandle
            || value.m_encodedType < 0 || !value.m_hasSourceLocation
            || value.m_sourceOffsetSpace != DRW_DwgFrameOffsetSpace::PhysicalFile
            || rows_.size() >= MaxFrames)
            return invalidate();
        try {
            const Row row{value.m_encodedType, value.m_sourceOffset,
                          value.m_sourceMapOrdinal};
            if (!rows_.emplace(value.m_handle, row).second)
                return invalidate();
            version_ = value.m_version;
        } catch (...) {
            invalidate();
        }
    }

    void seal(DRW::Version version, bool readSucceeded,
              const DwgReadFrameObservations& frames) noexcept {
        if (!valid_ || sealed_ || !readSucceeded || version != version_
            || (version != DRW::AC1012 && version != DRW::AC1014)
            || frames.status != DRW_DwgFrameCoverageStatus::FinalizedComplete
            || frames.sourceFrames == 0u || frames.sourceFrames != rows_.size()
            || frames.unrecognizedDispositions != 0u
            || frames.dispositions[static_cast<std::size_t>(
                DRW_DwgFrameDisposition::Published)] != frames.sourceFrames)
            return invalidate();
        for (std::size_t index = 0u; index < frames.dispositions.size(); ++index)
            if (index != static_cast<std::size_t>(DRW_DwgFrameDisposition::Published)
                && frames.dispositions[index] != 0u)
                return invalidate();
        try {
            std::vector<std::uint32_t> offsets;
            std::vector<std::uint64_t> ordinals;
            offsets.reserve(rows_.size());
            ordinals.reserve(rows_.size());
            for (const auto& row : rows_) {
                offsets.push_back(row.second.offset);
                ordinals.push_back(row.second.ordinal);
            }
            std::sort(offsets.begin(), offsets.end());
            std::sort(ordinals.begin(), ordinals.end());
            if (std::adjacent_find(offsets.begin(), offsets.end()) != offsets.end()
                || std::adjacent_find(ordinals.begin(), ordinals.end()) != ordinals.end())
                return invalidate();
            sealed_ = true;
        } catch (...) {
            invalidate();
        }
    }

    bool count(std::uint64_t generation, std::uint64_t attempt,
               DRW::Version version, int classNumber,
               std::size_t& result) const noexcept {
        if (!valid_ || !sealed_ || generation != generation_ || attempt != attempt_
            || version != version_ || classNumber < 500 || classNumber > 65535)
            return false;
        std::size_t staged = 0u;
        for (const auto& row : rows_)
            if (row.second.type == classNumber)
                ++staged;
        result = staged;
        return true;
    }

private:
    struct Row {
        int type;
        std::uint32_t offset;
        std::uint64_t ordinal;
    };
    void invalidate() noexcept {
        rows_.clear();
        valid_ = false;
        sealed_ = false;
    }
    std::map<std::uint32_t, Row> rows_;
    std::uint64_t generation_{0u};
    std::uint64_t attempt_{0u};
    DRW::Version version_{DRW::UNKNOWNV};
    bool valid_{false};
    bool sealed_{false};
};
} // namespace dx_legacy_frame_census
#endif
