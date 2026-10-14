/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DWGSAFETY_H
#define DWGSAFETY_H

#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

namespace dwgSafety {

/// Ordered set of disjoint, non-empty half-open intervals [start, end).
/// Insertions preserve adjacency and reject any overlap in O(log n). The
/// comparator/allocator template parameters keep the helper independently
/// testable without changing reader policy or format-specific window checks.
template <typename Compare = std::less<std::uint64_t>,
          typename Allocator = std::allocator<
              std::pair<const std::uint64_t, std::uint64_t>>>
class DisjointRangeIndex {
public:
    using Map = std::map<std::uint64_t, std::uint64_t, Compare, Allocator>;

    explicit DisjointRangeIndex(
        const Compare& compare = Compare{},
        const Allocator& allocator = Allocator{})
        : m_ranges(compare, allocator) {}

    bool insert(std::uint64_t start, std::uint64_t end) {
        if (start >= end)
            return false;
        const auto successor = m_ranges.lower_bound(start);
        if (successor != m_ranges.end() && successor->first < end)
            return false;
        if (successor != m_ranges.begin()
            && std::prev(successor)->second > start)
            return false;
        m_ranges.emplace_hint(successor, start, end);
        return true;
    }

    std::size_t size() const noexcept { return m_ranges.size(); }
    bool empty() const noexcept { return m_ranges.empty(); }
    void clear() noexcept { m_ranges.clear(); }

private:
    Map m_ranges;
};

/// Exact, case-sensitive name admission for unknown section descriptors.
/// The caller keeps its source-ordered vector separately for later replay.
template <typename Compare = std::less<std::string>,
          typename Allocator = std::allocator<std::string>>
class ExactNameIndex {
public:
    using Set = std::set<std::string, Compare, Allocator>;

    explicit ExactNameIndex(
        const Compare& compare = Compare{},
        const Allocator& allocator = Allocator{})
        : m_names(compare, allocator) {}

    bool insert(const std::string& name) {
        return m_names.insert(name).second;
    }

    std::size_t size() const noexcept { return m_names.size(); }
    bool empty() const noexcept { return m_names.empty(); }
    void clear() noexcept { m_names.clear(); }

private:
    Set m_names;
};

constexpr std::uint64_t MaxBufferSize = 0x2f000000;
// ODS describes HANDLES groups as 2032 bytes, but real legacy files and
// LibreDWG accept slightly larger groups. Keep the compatibility ceiling
// shared with the strict cross-project reader while bounding allocations.
constexpr std::uint16_t MaxHandleMapGroupSize = 2050;
constexpr std::uint64_t MaxPageCap = 0x144400;
// R2007 page maps are file-controlled repeated descriptors.  This is an
// allocation/resource limit, not a DWG semantic maximum.
constexpr std::uint64_t MaxPageCount = 1ULL << 20;
// A reactor list is a file-controlled repeated handle sequence.  The DWG
// format has no small semantic maximum, so this is a resource limit for the
// in-memory representation used by libdxfrw.
constexpr std::uint32_t MaxReactorCount = 1000000;
// POLYLINE and BLOCK_RECORD owned-object lists are file-controlled repeated
// handle sequences. Keep the allocation ceiling aligned with reactor lists;
// the enclosing object-body bound remains the primary format limit.
constexpr std::uint32_t MaxOwnedObjectCount = 1000000;
// A HANDLE map can contain many individually valid frames. Bound the total
// object-map work per read while allowing callers to raise the ceiling for
// intentionally larger drawings.
constexpr std::size_t MaxDwgReadObjectCount =
    static_cast<std::size_t>(MaxOwnedObjectCount);
// Journalled BLOCK delivery can emit a typed/raw pair plus a receipt per
// source, with separate semantic and receipt events for both delimiters.
constexpr std::uint32_t MaxBlockJournalEventCount =
    MaxOwnedObjectCount * 3u + 4u;
// EED chunks and their item streams are length-delimited by the DWG format,
// but a file may repeat them until the enclosing object ends. These limits
// bound the staged representation without changing the wire types.
constexpr std::uint32_t MaxEedChunks = 65536;
constexpr std::uint32_t MaxEedItems = 65536;
constexpr std::uint32_t MaxEedTotalItems = 1000000;
// Public integrity reports retain a bounded prefix so malformed input cannot
// turn one warning per page into an unbounded diagnostic allocation.
constexpr std::uint32_t MaxIntegrityDiagnostics = 1024;
// Fixture-only entity attribution retains a small bounded prefix. It is
// intentionally separate from public integrity reports.
constexpr std::uint32_t MaxEntityFailureDiagnostics = 64;

inline bool validReactorCount(std::int32_t count) {
    return count >= 0
        && static_cast<std::uint32_t>(count) <= MaxReactorCount;
}

inline bool validOwnedObjectCount(std::int32_t count, int remainingBytes) {
    return count >= 0
        && static_cast<std::uint32_t>(count) <= MaxOwnedObjectCount
        && remainingBytes > 0
        && static_cast<std::uint64_t>(count) + 1u
            <= static_cast<std::uint64_t>(remainingBytes);
}

inline bool add(std::uint64_t lhs, std::uint64_t rhs, std::uint64_t& result) {
    if (rhs > (std::numeric_limits<std::uint64_t>::max)() - lhs)
        return false;
    result = lhs + rhs;
    return true;
}

enum class SectionPageRangeResult : std::uint8_t {
    Inserted,
    Empty,
    Overflow,
    Overlap,
    ResourceFailure
};

/// Validate and add one version-specific logical section window. Callers pass
/// R2004's maxSize/size window or R2007's page uncompressed size; this helper
/// only owns checked half-open interval admission and allocation-failure
/// classification.
template <typename Index>
SectionPageRangeResult tryInsertSectionPageRange(
    Index& index, std::uint64_t start, std::uint64_t length) noexcept {
    if (length == 0)
        return SectionPageRangeResult::Empty;
    std::uint64_t end = 0;
    if (!add(start, length, end))
        return SectionPageRangeResult::Overflow;
    try {
        return index.insert(start, end)
            ? SectionPageRangeResult::Inserted
            : SectionPageRangeResult::Overlap;
    } catch (...) {
        return SectionPageRangeResult::ResourceFailure;
    }
}

enum class ExactNameInsertResult : std::uint8_t {
    Inserted,
    Duplicate,
    ResourceFailure
};

template <typename Index>
ExactNameInsertResult tryInsertExactName(
    Index& index, const std::string& name) noexcept {
    try {
        return index.insert(name)
            ? ExactNameInsertResult::Inserted
            : ExactNameInsertResult::Duplicate;
    } catch (...) {
        return ExactNameInsertResult::ResourceFailure;
    }
}

inline bool multiply(std::uint64_t lhs, std::uint64_t rhs,
                     std::uint64_t& result) {
    if (lhs != 0 && rhs > (std::numeric_limits<std::uint64_t>::max)() / lhs)
        return false;
    result = lhs * rhs;
    return true;
}

inline bool range(std::uint64_t offset, std::uint64_t length,
                  std::uint64_t limit) {
    return offset <= limit && length <= limit - offset;
}

inline bool alignUp8(std::uint64_t value, std::uint64_t& result) {
    std::uint64_t adjusted = 0;
    if (!add(value, 7, adjusted))
        return false;
    result = adjusted & ~std::uint64_t{7};
    return true;
}

// A section can omit all-zero pages.  Capacity therefore follows the
// declared logical size as well as the number of written pages.
inline bool sectionBufferCapacity(std::uint64_t sectionSize,
                                  std::uint64_t pageCount,
                                  std::uint64_t pageCap,
                                  std::uint64_t& result) {
    if (pageCap == 0)
        return false;

    std::uint64_t writtenCapacity = 0;
    if (!multiply(pageCount, pageCap, writtenCapacity))
        return false;

    std::uint64_t roundedSize = 0;
    if (!add(sectionSize, pageCap - 1, roundedSize))
        return false;
    const std::uint64_t roundedPages = roundedSize / pageCap;
    std::uint64_t logicalCapacity = 0;
    if (!multiply(roundedPages, pageCap, logicalCapacity))
        return false;

    result = writtenCapacity > logicalCapacity
        ? writtenCapacity : logicalCapacity;
    return true;
}

} // namespace dwgSafety

#endif // DWGSAFETY_H
