/* Bounded selected-field observation of a legacy LTYPE_CONTROL/ByBlock pair.
 * Supplied bodies/models are untrusted: no source/CRC provenance, complete
 * typed equality, sibling closure, semantic or output authority is implied.
 */
#ifndef DWG_LTYPE_CONTROL_BYBLOCK_AC1018_CANDIDATE_H
#define DWG_LTYPE_CONTROL_BYBLOCK_AC1018_CANDIDATE_H
#include "drw_objects.h"
#include <cstdint>
namespace DwgLTypeControlByBlockAc1018Candidate {
struct Ref {std::uint32_t target{0u};std::uint64_t startBit{0u},endBit{0u};};
struct Result {
    std::uint32_t control{0u},byBlock{0u},controlDataEndBit{0u},ltypeDataEndBit{0u};
    Ref ordinary,byBlockPhantom,byLayerPhantom,rowOwner;
    std::uint64_t controlHandleEndBit{0u},ltypeHandleEndBit{0u};
};
// The source H4 null applies only to this control profile. H2/H3 roles and
// order are wire facts; they do not imply DXF ownership or sibling existence.
// Failure clears result and no unobserved typed fields are certified.
bool inspect(const DRW_UnsupportedObject& control,const DRW_UnsupportedObject& byBlock,
             const DRW_LType& typed,Result&) noexcept;
// ODA v5.4.1 §20.4.57 normative order: ordinary entries, then ByLayer,
// then ByBlock. Separate from inspect(), which records the pinned anomalous
// source order. Both are bounded projections, not source tickets.
bool inspectNormativeOrder(const DRW_UnsupportedObject& control,const DRW_UnsupportedObject& byBlock,
                           const DRW_LType& typed,Result&) noexcept;
}
#endif
