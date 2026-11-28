/* Untrusted bounded AC1018 TABLESTYLE extension-DICTIONARY observation.
 * Its two ordered numeric targets are not authenticated object identities.
 */
#ifndef DWG_TABLESTYLE_EXTENSION_DICTIONARY_AC1018_CANDIDATE_H
#define DWG_TABLESTYLE_EXTENSION_DICTIONARY_AC1018_CANDIDATE_H
#include "drw_objects.h"
#include <cstdint>
namespace DwgTableStyleExtensionDictionaryAc1018Candidate {
struct Result {
    std::uint32_t handle{0u},owner{0u},roundtrip{0u},cellStyleMap{0u},dataEndBit{0u};
    std::uint64_t ownerStartBit{0u},ownerEndBit{0u},roundtripStartBit{0u},roundtripEndBit{0u},mapStartBit{0u},mapEndBit{0u};
};
bool inspect(const DRW_UnsupportedObject& raw,const DRW_Dictionary& typed,Result& result) noexcept;
inline bool matchesSelected(const Result& result,std::uint32_t owner,std::uint32_t first,std::uint32_t second) noexcept {
    return result.owner==owner && result.roundtrip==first && result.cellStyleMap==second;
}
}
#endif
