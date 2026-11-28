/* Untrusted AC1018 TABLE entity common-prefix observer. This does not bind
 * source provenance, editable TABLE models, whole semantics or output. */
#ifndef DWG_TABLE_ENTITY_XDICTIONARY_AC1018_CANDIDATE_H
#define DWG_TABLE_ENTITY_XDICTIONARY_AC1018_CANDIDATE_H
#include "drw_objects.h"
#include <cstdint>
namespace DwgTableEntityXDictionaryAc1018Candidate {
struct Result {
    std::uint32_t handle{0u},dictionary{0u},dataEndBit{0u};
    std::uint32_t layer{0u},block{0u},style{0u};
    std::uint64_t dictionaryStartBit{0u},dictionaryEndBit{0u};
};
// Only the source-observed common-selector profile is accepted. No tail or
// TABLE content completeness is claimed. Failure clears the result.
bool inspect(const DRW_UnsupportedObject& raw,Result& result) noexcept;
}
#endif
