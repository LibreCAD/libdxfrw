/* Bounded, opt-in inspection of one observed AC1018 DATATABLE body profile.
 *
 * This is a partial candidate projection of untrusted caller-supplied bytes.
 * It does not authenticate source provenance, establish general DATATABLE
 * semantics, qualify graph closure, or authorize writing/replaying a frame.
 */
#ifndef DWG_DATATABLE_AC1018_CANDIDATE_H
#define DWG_DATATABLE_AC1018_CANDIDATE_H

#include "drw_objects.h"
#include <cstdint>
#include <vector>

namespace DwgDataTableAc1018Candidate {

struct Result {
    std::uint16_t classNumber{0u};
    std::uint16_t rawHeaderValue{0u}; // first class-specific BS; semantics unqualified
    std::uint32_t handle{0u};
    std::uint32_t parentHandle{0u};
    std::uint32_t dataEndBit{0u};
    std::uint64_t handleEndBit{0u};
    std::vector<double> firstColumn;
    std::vector<std::int32_t> secondColumn;
    std::vector<std::int32_t> thirdColumn;
    std::vector<std::uint32_t> ownedChildren;
};

// Every failure clears result. Only the observed [BD, BL, BL, H] column
// profile with empty names, no EED/reactors/extension and complete framing is
// accepted. Limits are internal work budgets, not DWG format maxima.
bool inspect(const DRW_UnsupportedObject& raw, Result& result) noexcept;
}
#endif
