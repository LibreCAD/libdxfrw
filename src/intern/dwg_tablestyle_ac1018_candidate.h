/* Bounded observation of one AC1018 TABLESTYLE producer profile.
 * Caller-supplied bytes and typed models are untrusted: a match establishes
 * neither reader provenance nor dependency closure or output authority.
 * The typed comparison covers only selected modeled wire fields; it is not
 * full typed-model equality and does not inspect unrelated carrier state.
 */
#ifndef DWG_TABLESTYLE_AC1018_CANDIDATE_H
#define DWG_TABLESTYLE_AC1018_CANDIDATE_H
#include "drw_objects.h"
#include <array>
#include <cstdint>
namespace DwgTableStyleAc1018Candidate {
struct Result {
    std::uint16_t classNumber{0u};
    std::uint32_t handle{0u}, owner{0u}, tableReactor{0u}, xdictionary{0u}, dataEndBit{0u};
    std::array<std::uint32_t,3> textStyles{};
    std::uint64_t ownerStartBit{0u}, xdictionaryStartBit{0u}, handleEndBit{0u};
};
// Failure clears result. IDs and flags are interpreted only in this profile;
// CMC 0xc8 is retained as opaque, not given color semantics.
bool inspect(const DRW_UnsupportedObject&, const DRW_TableStyle&, Result&) noexcept;
}
#endif
