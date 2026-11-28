/* Bounded observation of one AC1018 DATATABLE child-XRECORD profile.
 * This does not authenticate caller bytes or permit output. String codepage
 * 45 and the observed RS length are retained as wire facts, not semantics.
 */
#ifndef DWG_TABLE_CHILD_XRECORD_AC1018_CANDIDATE_H
#define DWG_TABLE_CHILD_XRECORD_AC1018_CANDIDATE_H

#include "drw_objects.h"
#include <cstdint>
#include <string>

namespace DwgTableChildXRecordAc1018Candidate {
struct Result {
    std::uint32_t handle{0u},owner{0u},dataEndBit{0u};
    std::uint64_t ownerStartBit{0u},ownerEndBit{0u};
    bool hasText{false};
    std::string text;
};
// Only the observed ordered five/six-pair wire shapes are accepted; text and
// distribution across children need not match the physical witness. On failure
// result is cleared. Neither legacy view is treated as the authoritative order.
bool inspect(const DRW_UnsupportedObject& raw,const DRW_XRecord& typed,Result& result) noexcept;
}
#endif
