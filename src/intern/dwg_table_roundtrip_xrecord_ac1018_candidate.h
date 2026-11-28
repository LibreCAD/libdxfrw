/* Opt-in observer for one physical AC1018 TABLE-roundtrip XRECORD profile.
 * Raw input is untrusted. Success does not establish origin, graph closure,
 * text semantics, native interoperability, or permission to write a drawing.
 */
#ifndef DWG_TABLE_ROUNDTRIP_XRECORD_AC1018_CANDIDATE_H
#define DWG_TABLE_ROUNDTRIP_XRECORD_AC1018_CANDIDATE_H

#include "drw_objects.h"
#include <array>
#include <cstdint>
#include <vector>

namespace DwgTableRoundtripXRecordAc1018Candidate {
struct Reference {
    int groupCode{0};
    std::uint32_t target{0u};
    std::uint64_t startBit{0u};
    std::uint64_t endBit{0u};
};
struct Result {
    std::uint32_t handle{0u};
    std::uint32_t owner{0u};
    std::uint32_t dataEndBit{0u};
    std::uint64_t payloadStartBit{0u};
    std::uint64_t payloadEndBit{0u};
    std::uint64_t handleEndBit{0u};
    // Group 360/361 identify DXF pair codes whose DWG payload is a raw
    // little-endian 64-bit value; these are not DWG H owner-role encodings.
    std::array<Reference,3> references{};
};
// Cross-check the observed source wire profile against authoritative ordered
// m_dataEntries. Legacy m_values/m_handleValues are not mirror requirements.
// Invalid input clears result and leaves both input objects unchanged.
bool inspect(const DRW_UnsupportedObject& raw, const DRW_XRecord& typed,
             Result& result) noexcept;

// Build the exact ordered XRECORD payload with only its three observed
// group-360/361 values replaced. This is a mechanical projection helper; it
// does not authorize source replay or drawing output. The caller must hold
// the sealed source capability and prove target identities/roles separately.
// On failure, projectedEntries is unchanged and source is cleared.
bool projectReferences(const DRW_UnsupportedObject& raw,
                       const DRW_XRecord& typed,
                       const std::array<std::uint32_t, 3>& mappedTargets,
                       std::vector<DRW_Variant>& projectedEntries,
                       Result& source) noexcept;
}
#endif
