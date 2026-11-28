/* Opt-in observation of a bounded, out-of-version AC1018 TABLECONTENT profile.
 * Input is untrusted, not a reader-origin or CRC certificate. The result is
 * partial and is never authorization for output, semantic or native claims.
 */
#ifndef DWG_TABLECONTENT_AC1018_CANDIDATE_H
#define DWG_TABLECONTENT_AC1018_CANDIDATE_H

#include "drw_entities.h"
#include <cstdint>
#include <vector>

namespace DwgTableContentAc1018Candidate {
enum class HandleKind : std::uint8_t { TextStyle, BorderLinetype, TableStyle };
struct TextSpan {
    std::uint64_t startBit{0u},endBit{0u};
    std::vector<std::uint8_t> bytes;
};
struct HandleSpan {
    HandleKind kind{HandleKind::TextStyle};
    std::uint64_t startBit{0u},endBit{0u},reference{0u};
    std::uint8_t role{0u},size{0u};
};
struct Result {
    std::uint16_t classNumber{0u};
    std::uint32_t handle{0u},parentHandle{0u},dataEndBit{0u};
    std::uint64_t handleEndBit{0u};
    std::uint32_t workUnits{0u}; // bounded observer work; not a format field
    DRW_TableContent content; // partial projection; styles/custom maps omitted
    std::vector<std::uint8_t> rawBody;
    std::vector<TextSpan> textSpans;
    std::vector<HandleSpan> handleSpans;
};
// Every refusal or allocation failure clears the result. The sole admitted
// shape is 3x3, 45 bounded ASCII TV fields, the independently traced null
// text-style/border roles and a nonnull TABLESTYLE; no output is enabled.
bool inspect(const DRW_UnsupportedObject& raw,Result& result) noexcept;
}
#endif
