#ifndef DXFTABLEREFERENCES_H
#define DXFTABLEREFERENCES_H

#include <cstddef>
#include <cstdint>

namespace dxfTableContent {
// Schema roles, not target-existence, semantic or output certificates. Local
// style IDs are integer namespace members, never object handles to remap.
enum class ReferenceKind : std::uint8_t {
    Self, Owner, TextStyle, LineType, TableStyle, Field, BlockRecord,
    AttributeDefinition, DataLink, ObjectId, UnknownGeometry, LocalStyleId, Count
};
struct Reference {
    ReferenceKind kind{ReferenceKind::Self};
    std::size_t groupIndex{0};
    int code{0};
    std::uint32_t value{0};
};
class ReferenceSink {
public:
    // Events are tentative until decode returns true. The caller owns their
    // budget and must discard all events on false/exception, never publish a
    // partial role receipt as a complete parse.
    virtual ~ReferenceSink() = default;
    virtual bool add(const Reference& reference) = 0;
};
} // namespace dxfTableContent
#endif
