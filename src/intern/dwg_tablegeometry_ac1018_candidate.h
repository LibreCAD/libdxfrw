/* Bounded opt-in inspection of an observed AC1018 TABLEGEOMETRY profile.
 * ODA dates TABLEGEOMETRY to 2008, so this is out-of-version producer data.
 * This candidate does not authenticate caller bytes or authorize output.
 */
#ifndef DWG_TABLEGEOMETRY_AC1018_CANDIDATE_H
#define DWG_TABLEGEOMETRY_AC1018_CANDIDATE_H

#include "drw_objects.h"
#include <array>
#include <cstdint>

namespace DwgTableGeometryAc1018Candidate {
struct Cell {
    std::uint64_t startBit{0u};
    std::uint64_t endBit{0u};
    std::uint64_t handleStartBit{0u};
    std::uint64_t handleEndBit{0u};
    std::int32_t flags{0};
    double widthWithGap{0.0};
    double heightWithGap{0.0};
    std::uint32_t unknownHandle{0u};
};
struct Result {
    std::uint16_t classNumber{0u};
    std::uint32_t handle{0u};
    std::uint32_t parentHandle{0u};
    std::uint32_t dataEndBit{0u};
    std::uint64_t parentHandleStartBit{0u};
    std::uint64_t parentHandleEndBit{0u};
    std::uint64_t handleEndBit{0u};
    std::array<Cell,9> cells{};
};
// Accepts only a 3x3 zero-geometry/null-reference profile and exact boundaries.
// A successful inspection is a candidate, not a verified reader-origin frame.
// On every failure result is cleared; input and ordinary callbacks are unchanged.
bool inspect(const DRW_UnsupportedObject& raw, Result& result) noexcept;
}
#endif
