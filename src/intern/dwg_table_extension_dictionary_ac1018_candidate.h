/* Untrusted, opt-in observation of one AC1018 TABLE extension dictionary.
 * This is one-sided wire evidence; no TABLE link, provenance or output claim.
 */
#ifndef DWG_TABLE_EXTENSION_DICTIONARY_AC1018_CANDIDATE_H
#define DWG_TABLE_EXTENSION_DICTIONARY_AC1018_CANDIDATE_H
#include "drw_objects.h"
#include <cstdint>
namespace DwgTableExtensionDictionaryAc1018Candidate {
struct Result {
    std::uint32_t handle{0u},owner{0u},root{0u},dataEndBit{0u};
    std::uint64_t ownerStartBit{0u},ownerEndBit{0u},rootStartBit{0u},rootEndBit{0u};
};
bool inspect(const DRW_UnsupportedObject& raw,const DRW_Dictionary& typed,Result& result) noexcept;
}
#endif
