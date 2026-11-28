/* Observation of one AC1018 TABLESTYLE extension XRECORD wire profile.
 * Caller-supplied bytes are not authenticated, and this does not permit output.
 * The observed RS lengths and codepage byte 45 do not establish text semantics.
 */
#ifndef DWG_TABLESTYLE_ROUNDTRIP_XRECORD_AC1018_CANDIDATE_H
#define DWG_TABLESTYLE_ROUNDTRIP_XRECORD_AC1018_CANDIDATE_H
#include "drw_objects.h"
#include <array>
#include <cstdint>
namespace DwgTableStyleRoundtripXRecordAc1018Candidate {
struct Span { std::uint64_t begin{0u}, end{0u}; };
struct Result {
    std::uint32_t handle{0u}, owner{0u}, reactor{0u}, dataEndBit{0u};
    std::array<Span,10> entries{};
    Span ownerSpan{}, reactorSpan{};
};
// Exact observed values/order only. m_dataEntries is authoritative; legacy
// mirrors are ignored. Failure clears result. No source identity is implied.
bool inspect(const DRW_UnsupportedObject&, const DRW_XRecord&, Result&) noexcept;
}
#endif
