/* Bounded selected-field observation of an AC1018 producer backsave profile.
 * Caller bytes and typed models are untrusted: a match is not source/CRC
 * authentication, full typed-model equality, graph closure or output authority.
 * ODA v5.4.1 introduces CELLSTYLEMAP in 2008; do not generalize this profile.
 */
#ifndef DWG_CELLSTYLEMAP_AC1018_CANDIDATE_H
#define DWG_CELLSTYLEMAP_AC1018_CANDIDATE_H
#include "drw_objects.h"
#include <array>
#include <cstdint>
namespace DwgCellStyleMapAc1018Candidate {
struct Ref {std::uint32_t target{0u};std::uint64_t startBit{0u},endBit{0u};};
struct Style {
    int id{0},styleClass{0};std::uint64_t startBit{0u},endBit{0u};
    Ref textStyle;std::array<Ref,6> linetypes{};
};
struct Result {
    std::uint16_t classNumber{0u};std::uint32_t handle{0u},owner{0u},dataEndBit{0u};
    std::uint64_t ownerStartBit{0u},handleEndBit{0u};std::array<Style,3> styles{};
};
// Styles remain in wire order [3,1,2]. CMC 0xc8 stays an opaque packed word.
// Failure clears result. Unobserved typed fields are not certified.
bool inspect(const DRW_UnsupportedObject&,const DRW_CellStyleMap&,Result&) noexcept;
}
#endif
