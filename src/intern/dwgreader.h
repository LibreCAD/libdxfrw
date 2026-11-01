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

#ifndef DWGREADER_H
#define DWGREADER_H

#include "../drw_entities.h"
#include "../drw_objects.h"
#include "../drw_datastorage.h"
#include "../libdwgr.h"
#include "drw_textcodec.h"
#include "dwgbuffer.h"
#include "dwgsafety.h"
#include "dwgutil.h"
#include <algorithm>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class objHandle {
public:
  objHandle() = default;
  objHandle(std::uint32_t t, std::uint32_t h, std::uint32_t l,
            std::uint64_t ordinal = 0,
            DRW_DwgFrameOffsetSpace space = DRW_DwgFrameOffsetSpace::Unknown)
      : type{t}, handle{h}, loc{l}, sourceOrdinal{ordinal},
        sourceOffsetSpace{space} {}
  std::uint32_t type{0};
  std::uint32_t handle{0};
  std::uint32_t loc{0};
  std::uint64_t sourceOrdinal{0};
  DRW_DwgFrameOffsetSpace sourceOffsetSpace{DRW_DwgFrameOffsetSpace::Unknown};
};

namespace dwgReaderDetail {

/// Immutable ordering key for the bounded catch-all ENTITIES sweep. The
/// source map is unordered; retaining only this key lets the reader reproduce
/// its established sourceOrdinal/location/handle order without keeping
/// invalidatable map iterators across frame detach/erase operations.
struct DwgResidualEntityKey {
  std::uint64_t sourceOrdinal{0};
  std::uint32_t location{0};
  std::uint32_t handle{0};
};

inline bool residualEntityKeyLess(const DwgResidualEntityKey &lhs,
                                  const DwgResidualEntityKey &rhs) noexcept {
  if (lhs.sourceOrdinal != rhs.sourceOrdinal)
    return lhs.sourceOrdinal < rhs.sourceOrdinal;
  if (lhs.location != rhs.location)
    return lhs.location < rhs.location;
  return lhs.handle < rhs.handle;
}

/// Snapshot and sort a source-map identity set before the reader mutates the
/// map. `maxScratchBytes` accounts for the temporary key vector separately
/// from the already-admitted DWG source frames, including any old output
/// capacity retained through the atomic swap. On any admission/allocation
/// failure, `output` remains unchanged.
template <typename ObjectMap, typename Allocator>
bool buildDwgResidualEntityOrder(
    const ObjectMap &objects,
    std::vector<DwgResidualEntityKey, Allocator> &output,
    std::size_t maxEntries, std::size_t maxScratchBytes,
    std::size_t *comparisonCount = nullptr) noexcept {
  if (comparisonCount != nullptr)
    *comparisonCount = 0u;
  const std::size_t count = objects.size();
  const std::size_t maxKeyCapacity =
      maxScratchBytes / sizeof(DwgResidualEntityKey);
  if (count > maxEntries || count > maxKeyCapacity ||
      output.capacity() > maxKeyCapacity ||
      count > maxKeyCapacity - output.capacity()) {
    return false;
  }

  try {
    std::vector<DwgResidualEntityKey, Allocator> prepared(
        output.get_allocator());
    prepared.reserve(count);
    if (prepared.capacity() > maxKeyCapacity - output.capacity()) {
      return false;
    }
    for (const auto &entry : objects) {
      prepared.push_back({entry.second.sourceOrdinal, entry.second.loc,
                          entry.first});
    }
    std::sort(prepared.begin(), prepared.end(),
              [comparisonCount](const DwgResidualEntityKey &lhs,
                                const DwgResidualEntityKey &rhs) noexcept {
                if (comparisonCount != nullptr)
                  ++*comparisonCount;
                return residualEntityKeyLess(lhs, rhs);
              });
    output.swap(prepared);
    return true;
  } catch (...) {
    return false;
  }
}

/// A residual frame may be restored only for one bounded soft-failure retry.
/// The caller separately verifies that the restored map entry still has the
/// exact immutable source identity captured in its ordering key.
inline bool canRetryResidualEntity(
    bool firstAttempt, bool mapContainsSameSource, bool readSucceeded,
    bool frameFailure, bool hasCoverage,
    DRW_DwgFrameDisposition disposition,
    std::uint8_t publicationCount) noexcept {
  return firstAttempt && mapContainsSameSource && !readSucceeded &&
         !frameFailure && hasCoverage &&
         disposition == DRW_DwgFrameDisposition::Pending &&
         publicationCount == 0u;
}

inline bool fitsByteBudget(std::size_t retainedBytes,
                           std::size_t additionalBytes,
                           std::size_t limitBytes) noexcept {
  return retainedBytes <= limitBytes &&
         additionalBytes <= limitBytes - retainedBytes;
}

template <typename DeclaredHandles>
bool buildPolylineDeclaredIndex(
    const DeclaredHandles &declaredHandles,
    std::vector<std::pair<std::uint32_t, std::size_t>> &sortedIndex,
    std::vector<std::uint8_t> &stagedSlots,
    std::size_t maxBytes) noexcept {
  using HandleIndex = std::pair<std::uint32_t, std::size_t>;
  const std::size_t count = declaredHandles.size();
  if (count > static_cast<std::size_t>(dwgSafety::MaxOwnedObjectCount))
    return false;

  std::size_t priorOutputBytes = 0u;
  if (sortedIndex.capacity() > maxBytes / sizeof(HandleIndex))
    return false;
  priorOutputBytes = sortedIndex.capacity() * sizeof(HandleIndex);
  if (!fitsByteBudget(priorOutputBytes, stagedSlots.capacity(), maxBytes))
    return false;
  priorOutputBytes += stagedSlots.capacity();
  constexpr std::size_t bytesPerChild = sizeof(HandleIndex) + sizeof(std::uint8_t);
  if (count > (maxBytes - priorOutputBytes) / bytesPerChild)
    return false;

  std::vector<HandleIndex> index;
  std::vector<std::uint8_t> received;
  try {
    index.reserve(count);
    std::size_t slot = 0u;
    for (const std::uint32_t handle : declaredHandles) {
      index.emplace_back(handle, slot);
      ++slot;
    }
    std::sort(index.begin(), index.end());
    received.resize(count, 0u);
  } catch (...) {
    return false;
  }

  std::size_t actualBytes = priorOutputBytes;
  if (!fitsByteBudget(actualBytes, index.capacity() * sizeof(HandleIndex),
                      maxBytes)) {
    return false;
  }
  actualBytes += index.capacity() * sizeof(HandleIndex);
  if (!fitsByteBudget(actualBytes, received.capacity(), maxBytes))
    return false;

  sortedIndex.swap(index);
  stagedSlots.swap(received);
  return true;
}

template <typename Index, typename Less>
bool findPolylineDeclaredChildSlot(const Index &sortedIndex,
                                   std::uint32_t handle, std::size_t &slot,
                                   bool requireExactlyOnce,
                                   Less less) noexcept {
  if (handle == DRW::NoHandle)
    return false;
  const auto found = std::lower_bound(
      sortedIndex.cbegin(), sortedIndex.cend(), handle,
      [&less](const auto &entry, std::uint32_t target) {
        return less(entry.first, target);
      });
  if (found == sortedIndex.cend() || found->first != handle ||
      (requireExactlyOnce && (found + 1) != sortedIndex.cend() &&
       (found + 1)->first == handle)) {
    return false;
  }
  slot = found->second;
  return true;
}

// Charge an explicit, allocator-independent model for red-black tree nodes.
// Five pointer widths cover the parent/child links plus conservative node
// linkage/alignment overhead across the supported standard libraries;
// allocator metadata is not represented by this ceiling.
constexpr std::size_t PolylineTreeNodeLinkChargeBytes = 5u * sizeof(void *);
constexpr std::size_t PolylineClaimOwnerNodeChargeBytes =
    sizeof(std::uint32_t) + PolylineTreeNodeLinkChargeBytes;
constexpr std::size_t PolylineClaimHandleNodeChargeBytes =
    sizeof(std::pair<const std::uint32_t, std::set<std::uint32_t>>) +
    PolylineTreeNodeLinkChargeBytes;
constexpr std::size_t PolylineOwnershipClaimNodeChargeBytes =
    sizeof(std::pair<const std::uint32_t, std::uint32_t>) +
    PolylineTreeNodeLinkChargeBytes;
constexpr std::size_t PolylineOwnershipReferenceChargeBytes =
    sizeof(std::uint32_t);

// Preflight claims need only the first parent for each child: each later
// competing parent is marked invalid as it is observed. This avoids retaining
// a copy of every parent's child vector while keeping deterministic lookup.
// The accounting charges each scanned reference (including repeats) to bound
// work, plus conservative map/set node estimates for retained scratch state.
// It is an explicit policy model, not an allocator/RSS measurement.
class PolylineOwnershipPreflightIndex {
public:
  using FirstOwnerMap = std::map<std::uint32_t, std::uint32_t>;
  using InvalidOwnerSet = std::set<std::uint32_t>;

  bool registerClaim(std::uint32_t owner, std::uint32_t handle,
                     std::size_t otherRetainedBytes,
                     std::size_t maxBytes) noexcept {
    if (owner == DRW::NoHandle || handle == DRW::NoHandle ||
        !fitsByteBudget(otherRetainedBytes, m_chargedBytes, maxBytes)) {
      return false;
    }

    const auto first = m_firstOwner.find(handle);
    const bool newHandle = first == m_firstOwner.end();
    const bool conflict = !newHandle && first->second != owner;
    std::size_t additionalBytes = PolylineOwnershipReferenceChargeBytes;
    if (newHandle &&
        !fitsByteBudget(additionalBytes,
                        PolylineOwnershipClaimNodeChargeBytes,
                        (std::numeric_limits<std::size_t>::max)())) {
      return false;
    }
    if (newHandle)
      additionalBytes += PolylineOwnershipClaimNodeChargeBytes;

    const auto needsInvalidOwner = [this](std::uint32_t candidate) {
      return m_invalidOwners.find(candidate) == m_invalidOwners.end();
    };
    if (conflict && needsInvalidOwner(first->second)) {
      if (!fitsByteBudget(additionalBytes,
                          PolylineClaimOwnerNodeChargeBytes,
                          (std::numeric_limits<std::size_t>::max)())) {
        return false;
      }
      additionalBytes += PolylineClaimOwnerNodeChargeBytes;
    }
    if (conflict && needsInvalidOwner(owner)) {
      if (!fitsByteBudget(additionalBytes,
                          PolylineClaimOwnerNodeChargeBytes,
                          (std::numeric_limits<std::size_t>::max)())) {
        return false;
      }
      additionalBytes += PolylineClaimOwnerNodeChargeBytes;
    }
    if (!fitsByteBudget(otherRetainedBytes + m_chargedBytes,
                        additionalBytes, maxBytes)) {
      return false;
    }

    try {
      if (newHandle)
        m_firstOwner.emplace(handle, owner);
      if (conflict) {
        m_invalidOwners.insert(first->second);
        m_invalidOwners.insert(owner);
      }
    } catch (...) {
      return false;
    }
    m_chargedBytes += additionalBytes;
    ++m_referenceCount;
    return true;
  }

  bool markInvalid(std::uint32_t owner, std::size_t otherRetainedBytes,
                   std::size_t maxBytes) noexcept {
    if (owner == DRW::NoHandle ||
        !fitsByteBudget(otherRetainedBytes, m_chargedBytes, maxBytes)) {
      return false;
    }
    if (m_invalidOwners.find(owner) != m_invalidOwners.end())
      return true;
    if (!fitsByteBudget(otherRetainedBytes + m_chargedBytes,
                        PolylineClaimOwnerNodeChargeBytes, maxBytes)) {
      return false;
    }
    try {
      m_invalidOwners.insert(owner);
    } catch (...) {
      return false;
    }
    m_chargedBytes += PolylineClaimOwnerNodeChargeBytes;
    return true;
  }

  bool isInvalid(std::uint32_t owner) const noexcept {
    return m_invalidOwners.find(owner) != m_invalidOwners.end();
  }
  bool hasInvalidOwners() const noexcept { return !m_invalidOwners.empty(); }
  std::size_t invalidOwnerCount() const noexcept {
    return m_invalidOwners.size();
  }
  std::size_t referenceCount() const noexcept { return m_referenceCount; }
  std::size_t chargedBytes() const noexcept { return m_chargedBytes; }

private:
  FirstOwnerMap m_firstOwner;
  InvalidOwnerSet m_invalidOwners;
  std::size_t m_referenceCount{0u};
  std::size_t m_chargedBytes{0u};
};

// Reverse ownership index: for any vertex handle, locate every pending
// POLYLINE that declares it without scanning all pending parents. The ordered
// maps/sets give deterministic logarithmic lookup and retain every competing
// owner so the reader can reject and terminalize the same groups as before.
template <typename Compare = std::less<std::uint32_t>>
class PolylineDeclaredClaimIndex {
public:
  using OwnerSet = std::set<std::uint32_t, Compare>;
  using ClaimMap = std::map<std::uint32_t, OwnerSet, Compare>;

  explicit PolylineDeclaredClaimIndex(Compare compare = Compare{})
      : m_claims(compare) {}

  // Add one observed orphan-child owner. This single-entry operation is used
  // by the reader's inverse index so a later parent can test ownership in
  // logarithmic time without scanning every orphan vector.
  bool addOwner(std::uint32_t owner, std::uint32_t handle,
                std::size_t otherRetainedBytes, std::size_t maxBytes,
                std::size_t &addedBytes) noexcept {
    if (owner == DRW::NoHandle || handle == DRW::NoHandle ||
        !fitsByteBudget(otherRetainedBytes, m_bytes, maxBytes)) {
      return false;
    }

    auto existing = m_claims.find(handle);
    const bool newHandle = existing == m_claims.end();
    if (!newHandle && existing->second.find(owner) != existing->second.end())
      return false;

    std::size_t charge = PolylineClaimOwnerNodeChargeBytes;
    if (newHandle) {
      if (!fitsByteBudget(charge, PolylineClaimHandleNodeChargeBytes,
                          std::numeric_limits<std::size_t>::max())) {
        return false;
      }
      charge += PolylineClaimHandleNodeChargeBytes;
    }
    if (!fitsByteBudget(otherRetainedBytes, m_bytes, maxBytes) ||
        !fitsByteBudget(otherRetainedBytes + m_bytes, charge, maxBytes)) {
      return false;
    }

    typename ClaimMap::iterator claimIt = m_claims.end();
    bool insertedHandle = false;
    try {
      const auto claim = m_claims.try_emplace(handle, m_claims.key_comp());
      claimIt = claim.first;
      insertedHandle = claim.second;
      if (!claimIt->second.insert(owner).second) {
        if (insertedHandle && claimIt->second.empty())
          m_claims.erase(claimIt);
        return false;
      }
    } catch (...) {
      if (insertedHandle && claimIt != m_claims.end() &&
          claimIt->second.empty()) {
        m_claims.erase(claimIt);
      }
      return false;
    }
    m_bytes += charge;
    addedBytes = charge;
    return true;
  }

  // Removal is idempotent so cleanup can repair a partially observed state
  // without allocating. The returned charge is zero when this owner was not
  // present for the handle.
  bool removeOwner(std::uint32_t owner, std::uint32_t handle,
                   std::size_t &releasedBytes) noexcept {
    const auto claim = m_claims.find(handle);
    if (claim == m_claims.end() ||
        claim->second.find(owner) == claim->second.end()) {
      releasedBytes = 0u;
      return true;
    }
    std::size_t released = PolylineClaimOwnerNodeChargeBytes;
    const bool removesHandle = claim->second.size() == 1u;
    if (removesHandle)
      released += PolylineClaimHandleNodeChargeBytes;
    if (released > m_bytes)
      return false;
    claim->second.erase(owner);
    if (removesHandle) {
      m_claims.erase(claim);
    }
    m_bytes -= released;
    releasedBytes = released;
    return true;
  }

  template <typename Index>
  bool addParent(std::uint32_t owner, const Index &sortedIndex,
                 std::size_t otherRetainedBytes, std::size_t maxBytes,
                 std::size_t &addedBytes) noexcept {
    if (owner == DRW::NoHandle ||
        !fitsByteBudget(otherRetainedBytes, m_bytes, maxBytes)) {
      return false;
    }

    std::size_t newlyChargedBytes = 0u;
    bool havePrevious = false;
    std::uint32_t previousHandle = DRW::NoHandle;
    for (const auto &entry : sortedIndex) {
      const std::uint32_t handle = entry.first;
      if (havePrevious && handle == previousHandle)
        continue;
      havePrevious = true;
      previousHandle = handle;

      const auto existing = m_claims.find(handle);
      const bool newHandle = existing == m_claims.end();
      std::size_t charge = PolylineClaimOwnerNodeChargeBytes;
      if (newHandle) {
        if (!fitsByteBudget(charge, PolylineClaimHandleNodeChargeBytes,
                            std::numeric_limits<std::size_t>::max())) {
          rollbackParent(owner, sortedIndex);
          return false;
        }
        charge += PolylineClaimHandleNodeChargeBytes;
      } else if (existing->second.find(owner) != existing->second.end()) {
        rollbackParent(owner, sortedIndex);
        return false;
      }

      if (!fitsByteBudget(otherRetainedBytes, m_bytes, maxBytes) ||
          !fitsByteBudget(otherRetainedBytes + m_bytes, charge, maxBytes)) {
        rollbackParent(owner, sortedIndex);
        return false;
      }

      try {
        auto claim = m_claims.try_emplace(handle, m_claims.key_comp());
        try {
          if (!claim.first->second.insert(owner).second) {
            if (claim.second && claim.first->second.empty())
              m_claims.erase(claim.first);
            rollbackParent(owner, sortedIndex);
            return false;
          }
        } catch (...) {
          if (claim.second && claim.first->second.empty())
            m_claims.erase(claim.first);
          throw;
        }
      } catch (...) {
        rollbackParent(owner, sortedIndex);
        return false;
      }
      m_bytes += charge;
      newlyChargedBytes += charge;
    }
    addedBytes = newlyChargedBytes;
    return true;
  }

  template <typename Index>
  bool removeParent(std::uint32_t owner, const Index &sortedIndex,
                    std::size_t &releasedBytes) noexcept {
    std::size_t newlyReleasedBytes = 0u;
    bool havePrevious = false;
    std::uint32_t previousHandle = DRW::NoHandle;
    for (const auto &entry : sortedIndex) {
      const std::uint32_t handle = entry.first;
      if (havePrevious && handle == previousHandle)
        continue;
      havePrevious = true;
      previousHandle = handle;

      const auto claim = m_claims.find(handle);
      if (claim == m_claims.end() || claim->second.erase(owner) == 0u)
        continue;
      newlyReleasedBytes += PolylineClaimOwnerNodeChargeBytes;
      if (claim->second.empty()) {
        m_claims.erase(claim);
        newlyReleasedBytes += PolylineClaimHandleNodeChargeBytes;
      }
    }
    if (newlyReleasedBytes > m_bytes)
      return false;
    m_bytes -= newlyReleasedBytes;
    releasedBytes = newlyReleasedBytes;
    return true;
  }

  const OwnerSet *ownersFor(std::uint32_t handle) const noexcept {
    const auto claim = m_claims.find(handle);
    return claim == m_claims.end() ? nullptr : &claim->second;
  }

  bool containsParent(std::uint32_t owner, std::uint32_t handle) const noexcept {
    const OwnerSet *owners = ownersFor(handle);
    return owners != nullptr && owners->find(owner) != owners->end();
  }

  bool containsOwner(std::uint32_t owner, std::uint32_t handle) const noexcept {
    return containsParent(owner, handle);
  }

  const ClaimMap &entries() const noexcept { return m_claims; }
  std::size_t bytes() const noexcept { return m_bytes; }
  bool empty() const noexcept { return m_claims.empty(); }

  bool validateAccounting() const noexcept {
    std::size_t measuredBytes = 0u;
    for (const auto &entry : m_claims) {
      if (entry.second.empty() ||
          !fitsByteBudget(measuredBytes, PolylineClaimHandleNodeChargeBytes,
                          std::numeric_limits<std::size_t>::max())) {
        return false;
      }
      measuredBytes += PolylineClaimHandleNodeChargeBytes;
      if (entry.second.size() >
          (std::numeric_limits<std::size_t>::max() - measuredBytes) /
              PolylineClaimOwnerNodeChargeBytes) {
        return false;
      }
      measuredBytes += entry.second.size() * PolylineClaimOwnerNodeChargeBytes;
    }
    return measuredBytes == m_bytes;
  }

private:
  template <typename Index>
  void rollbackParent(std::uint32_t owner, const Index &sortedIndex) noexcept {
    std::size_t ignored = 0u;
    (void)removeParent(owner, sortedIndex, ignored);
  }

  ClaimMap m_claims;
  std::size_t m_bytes{0u};
};

// A late-arriving parent may adopt an orphan only when the observed child's
// owner matches it. `sortedIndex` is the parent's sorted handle/slot index;
// repeated declared handles are examined once.
template <typename Index, typename ClaimIndex>
bool hasForeignPolylineOrphanOwner(
    std::uint32_t owner, const Index &sortedIndex,
    const ClaimIndex &orphanOwners) noexcept {
  bool havePrevious = false;
  std::uint32_t previousHandle = DRW::NoHandle;
  for (const auto &entry : sortedIndex) {
    const std::uint32_t handle = entry.first;
    if (havePrevious && handle == previousHandle)
      continue;
    havePrevious = true;
    previousHandle = handle;
    const auto *owners = orphanOwners.ownersFor(handle);
    if (owners == nullptr)
      continue;
    for (const std::uint32_t observedOwner : *owners) {
      if (observedOwner != owner)
        return true;
    }
  }
  return false;
}

template <typename ClaimIndex>
bool hasForeignPolylineOrphanHandleOwner(
    std::uint32_t owner, std::uint32_t handle,
    const ClaimIndex &orphanOwners) noexcept {
  const auto *owners = orphanOwners.ownersFor(handle);
  if (owners == nullptr)
    return false;
  return std::any_of(owners->cbegin(), owners->cend(),
                     [owner](std::uint32_t observedOwner) {
                       return observedOwner != owner;
                     });
}

// Reserve for one more staged child without defeating vector's amortized
// growth. `reserve(size() + 1)` on every child can force a copy on each append
// on standard-library implementations that honor the requested capacity
// exactly. Keep the element ceiling explicit and clamp geometric growth to it.
template <typename Vector>
bool vectorCapacityForOneMore(const Vector &values, std::size_t maxCount,
                              std::size_t &targetCapacity) noexcept {
  if (values.size() >= maxCount || values.size() >= values.max_size())
    return false;
  const std::size_t needed = values.size() + 1u;
  if (needed <= values.capacity()) {
    targetCapacity = values.capacity();
    return true;
  }
  if (values.capacity() > maxCount)
    return false;

  std::size_t target = 1u;
  if (values.capacity() != 0u) {
    target = values.capacity() > maxCount / 2u
                 ? maxCount
                 : values.capacity() * 2u;
  }
  target = std::max(target, needed);
  if (target > maxCount || target > values.max_size())
    return false;
  targetCapacity = target;
  return true;
}

template <typename Vector>
bool reserveVectorForOneMore(Vector &values, std::size_t maxCount,
                             std::size_t *plannedCapacity = nullptr) noexcept {
  std::size_t targetCapacity = 0u;
  if (!vectorCapacityForOneMore(values, maxCount, targetCapacity))
    return false;
  if (plannedCapacity != nullptr)
    *plannedCapacity = targetCapacity;
  if (targetCapacity <= values.capacity())
    return true;
  const std::size_t needed = values.size() + 1u;
  try {
    values.reserve(targetCapacity);
  } catch (...) {
    return false;
  }
  return values.capacity() >= needed;
}

// unordered_map::reserve accepts an element count rather than a bucket count.
// Estimate the current element capacity using its configured load factor, then
// reserve geometrically with the same hard count ceiling as the staged owners.
template <typename UnorderedMap>
bool planUnorderedMapForOneMore(const UnorderedMap &values,
                                std::size_t maxCount,
                                std::size_t &target) noexcept {
  target = 0u;
  if (values.size() >= maxCount || values.size() >= values.max_size())
    return false;
  const std::size_t needed = values.size() + 1u;
  const long double available =
      static_cast<long double>(values.bucket_count()) *
      static_cast<long double>(values.max_load_factor());
  const std::size_t elementCapacity =
      available >= static_cast<long double>(maxCount)
          ? maxCount
          : static_cast<std::size_t>(available);
  if (needed <= elementCapacity)
    return true;

  const std::size_t geometric =
      elementCapacity == 0u
          ? 1u
          : (elementCapacity > maxCount / 2u ? maxCount
                                              : elementCapacity * 2u);
  target = std::max(needed, geometric);
  if (target > maxCount || target > values.max_size())
    return false;
  return true;
}

// Bound bucket-array growth before reserve. Standard-library implementations
// may round to primes or powers of two; allow up to 2x the minimum required
// count plus one, then verify the actual count after reserve and roll back an
// implementation that exceeds this explicit accounting policy.
template <typename UnorderedMap>
bool unorderedMapBucketGrowthUpperBound(const UnorderedMap &values,
                                        std::size_t targetElements,
                                        std::size_t &additionalBytes,
                                        std::size_t &transientRehashBytes)
    noexcept {
  additionalBytes = 0u;
  transientRehashBytes = 0u;
  if (targetElements == 0u)
    return true;
  const long double loadFactor =
      static_cast<long double>(values.max_load_factor());
  if (!(loadFactor > 0.0L))
    return false;
  const long double requiredBucketsExact =
      static_cast<long double>(targetElements) / loadFactor;
  const long double maxValue = static_cast<long double>(
      (std::numeric_limits<std::size_t>::max)());
  if (requiredBucketsExact > maxValue)
    return false;
  std::size_t requiredBuckets =
      static_cast<std::size_t>(requiredBucketsExact);
  if (static_cast<long double>(requiredBuckets) < requiredBucketsExact) {
    if (requiredBuckets == (std::numeric_limits<std::size_t>::max)())
      return false;
    ++requiredBuckets;
  }
  if (requiredBuckets >
      ((std::numeric_limits<std::size_t>::max)() - 1u) / 2u)
    return false;
  const std::size_t roundedUpperBound = requiredBuckets * 2u + 1u;
  const std::size_t upperBuckets =
      std::max(values.bucket_count(), roundedUpperBound);
  if (upperBuckets < values.bucket_count())
    return false;
  const std::size_t addedBuckets = upperBuckets - values.bucket_count();
  if (addedBuckets > (std::numeric_limits<std::size_t>::max)() /
                         sizeof(void *) ||
      upperBuckets > (std::numeric_limits<std::size_t>::max)() /
                         sizeof(void *)) {
    return false;
  }
  additionalBytes = addedBuckets * sizeof(void *);
  // During rehash, the old bucket array may remain live while the replacement
  // array is allocated. The retained charge is only the net growth; admission
  // must separately allow this conservative full replacement-array peak.
  transientRehashBytes = upperBuckets * sizeof(void *);
  return true;
}

template <typename UnorderedMap>
bool reserveUnorderedMapForOneMore(UnorderedMap &values,
                                   std::size_t maxCount) noexcept {
  std::size_t target = 0u;
  if (!planUnorderedMapForOneMore(values, maxCount, target))
    return false;
  if (target == 0u)
    return true;
  try {
    values.reserve(target);
  } catch (...) {
    return false;
  }
  return true;
}

template <typename UnorderedMap>
bool reserveUnorderedMapForOneMoreWithinBudget(
    UnorderedMap &values, std::size_t maxCount,
    std::size_t maxAdditionalBucketBytes,
    std::size_t maxTransientRehashBytes,
    std::size_t &actualAdditionalBucketBytes) noexcept {
  actualAdditionalBucketBytes = 0u;
  std::size_t target = 0u;
  if (!planUnorderedMapForOneMore(values, maxCount, target))
    return false;
  if (target == 0u)
    return true;

  std::size_t upperBoundBytes = 0u;
  std::size_t transientRehashBytes = 0u;
  if (!unorderedMapBucketGrowthUpperBound(values, target, upperBoundBytes,
                                          transientRehashBytes) ||
      upperBoundBytes > maxAdditionalBucketBytes ||
      transientRehashBytes > maxTransientRehashBytes) {
    return false;
  }

  const std::size_t oldBucketCount = values.bucket_count();
  try {
    values.reserve(target);
  } catch (...) {
    const std::size_t retainedBuckets = values.bucket_count();
    if (retainedBuckets > oldBucketCount &&
        retainedBuckets - oldBucketCount <=
            (std::numeric_limits<std::size_t>::max)() / sizeof(void *)) {
      actualAdditionalBucketBytes =
          (retainedBuckets - oldBucketCount) * sizeof(void *);
    }
    return false;
  }

  const std::size_t newBucketCount = values.bucket_count();
  if (newBucketCount >= oldBucketCount &&
      newBucketCount - oldBucketCount <=
          (std::numeric_limits<std::size_t>::max)() / sizeof(void *)) {
    actualAdditionalBucketBytes =
        (newBucketCount - oldBucketCount) * sizeof(void *);
  } else {
    return false;
  }
  if (newBucketCount - oldBucketCount <=
      upperBoundBytes / sizeof(void *))
    return actualAdditionalBucketBytes <= maxAdditionalBucketBytes;

  // The implementation rounded beyond our charged upper bound. Restore the
  // prior capacity if possible; if not, expose the retained bytes to the
  // caller so the reader can account them and fail closed.
  try {
    values.rehash(oldBucketCount);
  } catch (...) {
  }
  const std::size_t afterRollbackBuckets = values.bucket_count();
  actualAdditionalBucketBytes =
      afterRollbackBuckets > oldBucketCount &&
              afterRollbackBuckets - oldBucketCount <=
                  (std::numeric_limits<std::size_t>::max)() / sizeof(void *)
          ? (afterRollbackBuckets - oldBucketCount) * sizeof(void *)
          : 0u;
  return false;
}

// Build the POLYLINE's source-order mapping through a sorted handle/index
// scratch vector. This replaces one full staged-child scan per declared child
// while keeping the declaration vector authoritative and bounding temporary
// storage, including the caller's already-reserved output vectors.
template <typename DeclaredHandles, typename StagedVertices,
          typename HandleOf, typename Less>
bool buildPolylineVertexOrder(const DeclaredHandles &declaredHandles,
                              const StagedVertices &stagedVertices,
                              HandleOf handleOf, Less less,
                              std::vector<std::size_t> &orderedIndices,
                              std::size_t additionalScratchBytes,
                              std::size_t maxScratchBytes) noexcept {
  using HandleIndex = std::pair<std::uint32_t, std::size_t>;
  const std::size_t count = declaredHandles.size();
  if (count != stagedVertices.size() ||
      count > static_cast<std::size_t>(dwgSafety::MaxOwnedObjectCount) ||
      additionalScratchBytes > maxScratchBytes) {
    return false;
  }
  if (orderedIndices.capacity() >
      (maxScratchBytes - additionalScratchBytes) / sizeof(std::size_t)) {
    return false;
  }
  additionalScratchBytes +=
      orderedIndices.capacity() * sizeof(std::size_t);
  constexpr std::size_t scratchBytesPerChild =
      sizeof(HandleIndex) + sizeof(std::size_t) + sizeof(std::uint8_t);
  if (count >
      (maxScratchBytes - additionalScratchBytes) / scratchBytesPerChild) {
    return false;
  }

  std::vector<HandleIndex> stagedByHandle;
  std::vector<std::uint8_t> selected;
  std::vector<std::size_t> ordered;
  try {
    stagedByHandle.reserve(count);
    for (std::size_t index = 0; index < count; ++index)
      stagedByHandle.emplace_back(handleOf(stagedVertices[index]), index);
    std::sort(stagedByHandle.begin(), stagedByHandle.end(),
              [&less](const HandleIndex &lhs, const HandleIndex &rhs) {
                return less(lhs.first, rhs.first);
              });
    selected.resize(count, 0u);
    ordered.reserve(count);
  } catch (...) {
    return false;
  }

  std::size_t actualScratchBytes = additionalScratchBytes;
  const auto chargeCapacity = [&actualScratchBytes,
                               maxScratchBytes](std::size_t capacity,
                                                std::size_t elementBytes) {
    if (elementBytes != 0u &&
        capacity > (maxScratchBytes - actualScratchBytes) / elementBytes) {
      return false;
    }
    actualScratchBytes += capacity * elementBytes;
    return true;
  };
  if (!chargeCapacity(stagedByHandle.capacity(), sizeof(HandleIndex)) ||
      !chargeCapacity(selected.capacity(), sizeof(std::uint8_t)) ||
      !chargeCapacity(ordered.capacity(), sizeof(std::size_t))) {
    return false;
  }

  for (const std::uint32_t expected : declaredHandles) {
    if (expected == DRW::NoHandle)
      return false;
    const auto vertex = std::lower_bound(
        stagedByHandle.cbegin(), stagedByHandle.cend(), expected,
        [&less](const HandleIndex &candidate, std::uint32_t handle) {
          return less(candidate.first, handle);
        });
    if (vertex == stagedByHandle.cend() || vertex->first != expected ||
        ((vertex + 1) != stagedByHandle.cend() &&
         (vertex + 1)->first == expected) ||
        selected[vertex->second] != 0u) {
      return false;
    }
    selected[vertex->second] = 1u;
    ordered.push_back(vertex->second);
  }
  orderedIndices.swap(ordered);
  return true;
}

} // namespace dwgReaderDetail

/// File-level code-page identity captured from the DWG metadata envelope.
/// `present` distinguishes a missing field from an explicit undefined id;
/// `recognized` is true only when the id maps to a codec-backed ANSI_* name.
struct DwgSourceCodePage {
  bool present{false};
  std::uint16_t rawId{0};
  std::string name;
  bool recognized{false};
  bool primaryText{false};
};

struct DwgSourceFrameId {
  std::uint32_t handle{0};
  std::uint32_t offset{0};
  std::uint64_t ordinal{0};
  DRW_DwgFrameOffsetSpace offsetSpace{DRW_DwgFrameOffsetSpace::Unknown};

  [[nodiscard]] bool operator==(const DwgSourceFrameId &other) const noexcept {
    return handle == other.handle && offset == other.offset &&
           ordinal == other.ordinal && offsetSpace == other.offsetSpace;
  }
};

[[nodiscard]] inline DwgSourceFrameId
sourceFrameId(const objHandle &object) noexcept {
  return {object.handle, object.loc, object.sourceOrdinal,
          object.sourceOffsetSpace};
}

/// Map a DWG file-header codepage id (libreDWG codepages.h) to a
/// DRW_TextCodec ANSI codepage name.  Returns nullptr for unknown/unsupported
/// ids so the caller keeps the ANSI_1252 default (no worse than before).
const char *dwgCodePageName(std::uint16_t cp);

/// Inverse of dwgCodePageName(): map a DRW_TextCodec ANSI codepage name to
/// the DWG file-header codepage integer used on disk. Returns 30 (ANSI_1252)
/// for `nullptr`, the literal "ANSI_1252", or any name not in the round-trip
/// set. Used by writers to emit the codepage byte at the file-header offset
/// matching the reader's expected offset.
std::uint16_t dwgCodePageId(const char *name);

/// Locate the UTF-16 class-string stream from the R2007+ CLASSES footer.
/// `footerEndBit` points at the footer's end-bit field. The reader validates
/// every backward step and leaves `buffer` at the first string byte only on
/// success.
[[nodiscard]] bool readDwgClassStringFooter(dwgBuffer &buffer,
                                            std::uint64_t footerEndBit,
                                            std::uint64_t &stringStartBit,
                                            std::uint64_t &stringSize);

/// Decode an EED-attached pre-R2007 string from its source codepage to UTF-8.
/// `cp` is the DWG codepage integer carried per-EED-string (ODA DWG spec §28
/// "RS_BE codepage"). When `cp` resolves via dwgCodePageName(), build a
/// temporary AC1015-bound DRW_TextCodec for that codepage and decode through
/// it. Otherwise fall back to the file-level @p fallback decoder. Empty input
/// returns empty. R2007+ entity-EED strings are UTF-16LE and bypass this
/// helper entirely (caller handles them inline).
/// When a fallback codec exists, merge per-value decode and unknown/aliased
/// selection issues into its accumulated envelope without changing its file
/// codec or last primary-conversion snapshot on the named-codec path. Empty
/// values retain their declared-profile observation. This is not exhaustive
/// codepage/table qualification; no read/decoding policy is made stricter.
/// Legacy XRECORD application strings also use this value conversion for a
/// nonzero RC codepage, but retain their separate application-body framing
/// and established zero/file-default convention.
std::string decodeEedString(std::uint16_t cp, const std::string &raw,
                            DRW_TextCodec *fallback);

struct DwgEedHandleRef {
  std::size_t itemIndex{0};
  std::uint32_t handleRef{0};
};

struct DwgEedChunk {
  std::uint32_t appHandle{0};
  std::vector<DRW_Variant> items;
  std::vector<DwgEedHandleRef> layerRefs;
};

/// Read all EED chunks at the current DWG object-data position. The outer
/// BS length is followed by the APPID handle and a byte-exact item stream;
/// the output is replaced only after every chunk and item validates.
[[nodiscard]] bool
readDwgEed(DRW::Version version, dwgBuffer &buffer,
           std::vector<DwgEedChunk> &chunks,
           std::uint64_t endBit = (std::numeric_limits<std::uint64_t>::max)());

/// Read one or more handle references without publishing a partial result.
/// `offset` selects the DWG relative-handle transformation used by common
/// object/entity owner fields; ordinary reactor handles use false.
[[nodiscard]] bool readDwgHandleChecked(dwgBuffer &buffer,
                                        std::uint32_t baseHandle, bool offset,
                                        dwgHandle &handle);
[[nodiscard]] bool
readDwgHandleList(dwgBuffer &buffer, std::uint32_t baseHandle,
                  std::int32_t count, bool offset,
                  std::vector<std::uint32_t> *refs = nullptr);

// until 2000 = 2000-
// since 2004 except 2007 = 2004+
//  2007 = 2007
//  pages of section
/* 2000-: No pages, only sections
 * 2004+: Id, page number (index)
 *        size, size of page in file stream
 *        address, address in file stream
 *        dataSize, data size for this page
 *        startOffset, start offset for this page
 *        cSize, compressed size of data
 *        uSize, uncompressed size of data
 * 2007: page Id, pageCount & pages
 *       size, size in file
 *       dataSize
 *       startOffset, start position in decompressed data stream
 *       cSize, compressed size of data
 *       uSize, uncompressed size of data
 *       address, address in file stream
 * */
class dwgPageInfo {
public:
  dwgPageInfo() = default;
  dwgPageInfo(std::uint64_t i, std::uint64_t ad, std::uint64_t sz) {
    Id = i;
    address = ad;
    size = sz;
  }
  std::uint64_t Id{0};
  std::uint64_t address{0};     // in file stream, for rd18, rd21
  std::uint64_t size{0};        // in file stream, for rd18, rd21
  std::uint64_t dataSize{0};    // for rd18, rd21
  std::uint64_t startOffset{0}; // for rd18, rd21
  std::uint64_t cSize{0};       // compressed page size, for rd21
  std::uint64_t uSize{0};       // uncompressed page size, for rd21
  std::uint64_t checksum{0};    // R2007 decompressed page checksum
  std::uint64_t crc{0};         // R2007 compressed page CRC
};

// sections of file
/* 2000-: No pages, only section Id, size  & address in file
 * 2004+: Id, Section Id
 *        size, total size of uncompressed data
 *        pageCount & pages, number of pages in section
 *        maxSize, max decompressed Size per page
 *        compressed, (1 = no, 2 = yes, normally 2)
 *        encrypted, (0 = no, 1 = yes, 2 = unknown)
 *        name, read & stored but not used
 * 2007: same as 2004+ except encoding, saved in compressed field
 * */
class dwgSectionInfo {
public:
  dwgSectionInfo() = default;
  std::int32_t Id{-1};         // section Id, 2000-   rd15 rd18
  std::string name;            // section name rd18
  std::uint32_t compressed{1}; // is compressed? 1=no, 2=yes rd18,
                               // rd21(encoding)
  std::uint32_t encrypted{0};  // encrypted (doc: 0=no, 1=yes, 2=unkn) on read:
                               // objects 0 and encrypted yes rd18
  std::unordered_map<std::uint32_t, dwgPageInfo> pages; // index, size, offset
  std::uint64_t size{0}; // size of section,  2000- rd15, rd18, rd21 (data size)
  std::uint64_t pageCount{
      0}; // number of pages (dwgPageInfo) in section rd18, rd21
  std::uint64_t maxSize{0}; // max decompressed size (needed??) rd18 rd21
  std::uint64_t address{0}; // address (seek) , 2000-
};

//! Class to handle dwg obj control entries
/*!
 *  Class to handle dwg obj control entries
 *  @author Rallaz
 */
class DRW_ObjControl : public DRW_TableEntry {
public:
  DRW_ObjControl() { reset(); }

  void reset() {
    DRW_TableEntry::reset();
    handlesList.clear();
  }
  bool parseDwg(DRW::Version version, dwgBuffer *buf,
                std::uint32_t bs = 0) override;
  std::list<std::uint32_t> handlesList;
};

class DwgCodePageTestAccess;

class dwgReader {
  // friend the real class (not the legacy `using dwgR = dwgRW;` alias —
  // C++ does not allow `friend class <typedef-name>;`).
  friend class dwgRW;
  friend class DwgCodePageTestAccess;

public:
  using DwgObjectMap = std::unordered_map<std::uint32_t, objHandle>;

  /// Internal, bounded failure attribution used by DWG fixture tests. This
  /// deliberately does not surface through dwgRW's public read API.
  enum class DwgEntityFailurePhase : std::uint8_t {
    None,
    Classify,
    Frame,
    TypedBody,
    CommonHandles,
    Identity,
    Aggregate,
    BlockFinalize
  };

  struct DwgEntityFailureDiagnostic {
    std::uint32_t handle{DRW::NoHandle};
    std::int16_t type{-1};
    std::uint32_t blockRecordHandle{DRW::NoHandle};
    DwgEntityFailurePhase phase{DwgEntityFailurePhase::None};
  };

  dwgReader(std::unique_ptr<dwgBuffer> buffer, dwgRW *p)
      : fileBuf{std::move(buffer)}, parent{p},
        m_readObjectBudget{p != nullptr
                               ? p->dwgReadObjectBudget()
                               : dwgSafety::MaxDwgReadObjectCount} {
    std::size_t ownerMapBucketBytes = 0u;
    if (polylineOwnerMapBucketBytes(ownerMapBucketBytes) &&
        ownerMapBucketBytes <= dwgSafety::MaxPolylineStagingBytes) {
      m_polylineStagingBytes = ownerMapBucketBytes;
    } else {
      m_polylineStagingBytes = (std::numeric_limits<std::size_t>::max)();
      m_dwgFrameCoverageIntegrityViolation = true;
    }
    decoder.setVersion(DRW::AC1021, false); // default 2007 in utf8(no convert)
    decoder.setCodePage("UTF-16", false);
    //        blockCtrl=0; //RLZ: temporary
    //        blockCtrl=layerCtrl=styleCtrl=linetypeCtrl=viewCtrl=0;
    //        ucsCtrl=vportCtrl=appidCtrl=dimstyleCtrl=vpEntHeaderCtrl=0;
  }
  virtual ~dwgReader();

  [[nodiscard]] bool readObjectBudgetExceeded() const noexcept {
    return m_readObjectBudgetExceeded;
  }
  [[nodiscard]] std::size_t readObjectCount() const noexcept {
    return m_readObjectCount;
  }

protected:
  struct DwgSourceFrameLease;
  struct DwgFramePublicationArtifacts {
    constexpr DwgFramePublicationArtifacts(
        const DRW_DwgDictionaryMembership *dictionary = nullptr,
        const DRW_DwgTypedReference *reference = nullptr,
        const DRW_DwgBlockReachability *reachability = nullptr,
        const DRW_DwgGroupMembership *group = nullptr,
        const DRW_DwgSortEntsMembership *sortEnts = nullptr,
        const DRW_DwgFieldListMembership *fieldList = nullptr,
        const DRW_DwgDictionaryWithDefaultMembership *dictionaryWithDefault =
            nullptr,
        const DRW_DwgFieldPayloadReceipt *fieldPayload = nullptr) noexcept
        : dictionaryMembership(dictionary), typedReference(reference),
          blockReachability(reachability), groupMembership(group),
          sortEntsMembership(sortEnts), fieldListMembership(fieldList),
          dictionaryWithDefaultMembership(dictionaryWithDefault),
          fieldPayloadReceipt(fieldPayload) {}

    const DRW_DwgDictionaryMembership *dictionaryMembership;
    const DRW_DwgTypedReference *typedReference;
    const DRW_DwgBlockReachability *blockReachability;
    const DRW_DwgGroupMembership *groupMembership;
    const DRW_DwgSortEntsMembership *sortEntsMembership;
    const DRW_DwgFieldListMembership *fieldListMembership;
    const DRW_DwgDictionaryWithDefaultMembership
        *dictionaryWithDefaultMembership;
    const DRW_DwgFieldPayloadReceipt *fieldPayloadReceipt;
  };

  // FIELD-family callbacks require a parser-validated receipt. Keep both
  // typed carriers and their raw peer behind one publication boundary.
  struct DwgFieldFamilyPublicationOutputs {
    constexpr DwgFieldFamilyPublicationOutputs(
        const DRW_Field *fieldOutput = nullptr,
        const DRW_FieldList *fieldListOutput = nullptr,
        const DRW_UnsupportedObject *rawObjectOutput = nullptr) noexcept
        : field(fieldOutput), fieldList(fieldListOutput),
          rawObject(rawObjectOutput) {}

    const DRW_Field *field;
    const DRW_FieldList *fieldList;
    const DRW_UnsupportedObject *rawObject;
  };

  // Entity dispatch uses this narrow internal sink so BLOCK_RECORD parsing
  // can later stage owned value events without proxying the full interface.
  class DwgEntityOutput {
  public:
    struct Event {
      virtual ~Event() = default;
      virtual bool replay(dwgReader &reader, DRW_Interface &target) const = 0;
      [[nodiscard]] virtual bool hasSource() const noexcept = 0;
      [[nodiscard]] virtual DwgSourceFrameId source() const noexcept = 0;
      [[nodiscard]] virtual bool completesSource() const noexcept = 0;
    };

    class SourceScope {
    public:
      SourceScope(DwgEntityOutput &output,
                  const DwgSourceFrameId &source) noexcept;
      ~SourceScope();

      SourceScope(const SourceScope &) = delete;
      SourceScope &operator=(const SourceScope &) = delete;

    private:
      DwgEntityOutput &m_output;
      std::optional<DwgSourceFrameId> m_previous;
    };

    virtual ~DwgEntityOutput();

    void appendValue(
        const DRW_UnsupportedObject &value,
        void (DRW_Interface::*callback)(const DRW_UnsupportedObject &));

    template <typename T, typename Callback>
    void appendValue(const T &value, Callback callback);
    void appendUnsupportedObject(DRW_UnsupportedObject &&value);
    void appendModelerGeometry(DRW_ModelerGeometry &&value);
    [[nodiscard]] SourceScope
    bindSource(const DwgSourceFrameId &source) noexcept;
    [[nodiscard]] bool appendEndBlock();
    [[nodiscard]] bool
    appendFramePublication(dwgReader &reader,
                           const DRW_DwgFramePublication &publication,
                           DwgFramePublicationArtifacts artifacts =
                               DwgFramePublicationArtifacts());
    [[nodiscard]] bool appendFrameCompletion();

  protected:
    virtual bool append(std::unique_ptr<Event> event) = 0;

    std::optional<DwgSourceFrameId> m_activeSource;
  };

  class DwgImmediateEntityOutput;
  class DwgBlockJournalOutput final : public DwgEntityOutput {
  public:
    explicit DwgBlockJournalOutput(dwgReader &reader);

    [[nodiscard]] bool replay(DRW_Interface &target,
                              DwgSourceFrameId *activeSource = nullptr);
    [[nodiscard]] bool replayEvent(std::size_t index, DRW_Interface &target,
                                   DwgSourceFrameId *activeSource,
                                   bool *completesSource);
    [[nodiscard]] bool reserve(std::size_t eventCount);
    void truncate(std::size_t eventCount) noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

  private:
    bool append(std::unique_ptr<Event> event) override;

    dwgReader &m_reader;
    std::vector<std::unique_ptr<Event>> m_events;
  };

  struct DwgStagedClass {
    std::unique_ptr<DRW_Class> m_value;
    std::size_t m_coverageIndex = 0;
  };

  enum class DwgMappedEntityOutcome : std::uint8_t {
    PublishedSimple,
    JournalledSimple,
    StagedCompound,
    CommittedCompound,
    DeferredObject,
    Rejected
  };

  enum class DwgMappedEntityCompletion : std::uint8_t { Immediate, Journal };

  enum class DwgInsertTerminalReason : std::uint8_t {
    MalformedGroup,
    CallbackException,
    ReceiptFailure
  };

  // Test-only logical failure points for terminal marker claims. Production
  // readers retain None and never take a failure path.
  enum class DwgTerminalizerFailurePoint : std::uint8_t {
    None,
    BeforeSeqEndMarker,
    BeforeInsertOwnerMarker,
    BeforeOrphanOwnerMarker
  };

  void setTerminalizerFailurePointForTest(
      DwgTerminalizerFailurePoint point) noexcept {
    m_terminalizerFailurePointForTest = point;
  }
  [[nodiscard]] bool consumeTerminalizerFailurePointForTest(
      DwgTerminalizerFailurePoint point) noexcept {
    if (m_terminalizerFailurePointForTest != point)
      return false;
    m_terminalizerFailurePointForTest = DwgTerminalizerFailurePoint::None;
    return true;
  }

  // Test-only logical failure points for POLYLINE commit preparation.
  enum class DwgPolylinePrepareFailurePoint : std::uint8_t {
    None,
    BeforeReservation,
    BeforeChildMarker,
    BeforeSeqEndMarker
  };

  void setPolylinePrepareFailurePointForTest(
      DwgPolylinePrepareFailurePoint point) noexcept {
    m_polylinePrepareFailurePointForTest = point;
  }
  [[nodiscard]] bool consumePolylinePrepareFailurePointForTest(
      DwgPolylinePrepareFailurePoint point) noexcept {
    if (m_polylinePrepareFailurePointForTest != point)
      return false;
    m_polylinePrepareFailurePointForTest = DwgPolylinePrepareFailurePoint::None;
    return true;
  }

  // Test-only logical failure points while a parent adopts child-first
  // POLYLINE state.
  enum class DwgPolylineStageFailurePoint : std::uint8_t {
    None,
    BeforeParentStateReserve,
    BeforeParentStateInsert,
    BeforeFrameStaging,
    BeforeOrphanAdoption
  };

  void setPolylineStageFailurePointForTest(
      DwgPolylineStageFailurePoint point) noexcept {
    m_polylineStageFailurePointForTest = point;
  }
  [[nodiscard]] bool consumePolylineStageFailurePointForTest(
      DwgPolylineStageFailurePoint point) noexcept {
    if (m_polylineStageFailurePointForTest != point)
      return false;
    m_polylineStageFailurePointForTest = DwgPolylineStageFailurePoint::None;
    return true;
  }

  enum class DwgBlockJournalReservationFailurePoint : std::uint8_t {
    None,
    BeforeReserve
  };

  void setBlockJournalReservationFailurePointForTest(
      DwgBlockJournalReservationFailurePoint point,
      std::size_t occurrence = 1u) noexcept {
    m_blockJournalReservationFailurePointForTest = point;
    m_blockJournalReservationFailureOccurrenceForTest = occurrence;
  }
  [[nodiscard]] bool
  consumeBlockJournalReservationFailurePointForTest() noexcept {
    if (m_blockJournalReservationFailurePointForTest !=
            DwgBlockJournalReservationFailurePoint::BeforeReserve ||
        m_blockJournalReservationFailureOccurrenceForTest == 0u) {
      return false;
    }
    --m_blockJournalReservationFailureOccurrenceForTest;
    if (m_blockJournalReservationFailureOccurrenceForTest != 0u)
      return false;
    m_blockJournalReservationFailurePointForTest =
        DwgBlockJournalReservationFailurePoint::None;
    return true;
  }

  enum class DwgBlockJournalAdoptFailurePoint : std::uint8_t {
    None,
    BeforeTransfer
  };

  void setBlockJournalAdoptFailurePointForTest(
      DwgBlockJournalAdoptFailurePoint point,
      std::size_t occurrence = 1u) noexcept {
    m_blockJournalAdoptFailurePointForTest = point;
    m_blockJournalAdoptFailureOccurrenceForTest = occurrence;
  }
  [[nodiscard]] bool consumeBlockJournalAdoptFailurePointForTest() noexcept {
    if (m_blockJournalAdoptFailurePointForTest !=
            DwgBlockJournalAdoptFailurePoint::BeforeTransfer ||
        m_blockJournalAdoptFailureOccurrenceForTest == 0u) {
      return false;
    }
    --m_blockJournalAdoptFailureOccurrenceForTest;
    if (m_blockJournalAdoptFailureOccurrenceForTest != 0u)
      return false;
    m_blockJournalAdoptFailurePointForTest =
        DwgBlockJournalAdoptFailurePoint::None;
    return true;
  }

  [[nodiscard]] virtual bool readMetaData() = 0;
  [[nodiscard]] virtual bool readPreview() { return false; }
  [[nodiscard]] virtual bool readFileHeader() = 0;
  [[nodiscard]] virtual bool readDwgHeader(DRW_Header &hdr) = 0;
  [[nodiscard]] virtual bool readDwgTemplate(DRW_Header&) { return true; }
  [[nodiscard]] virtual bool readDwgClasses() = 0;
  [[nodiscard]] virtual bool readDwgHandles() = 0;
  [[nodiscard]] virtual bool readDwgTables(DRW_Header &hdr) = 0;
  [[nodiscard]] virtual bool readDwgBlocks(DRW_Interface &intfa) = 0;
  [[nodiscard]] virtual bool readDwgEntities(DRW_Interface &intfa) = 0;
  [[nodiscard]] virtual bool readDwgObjects(DRW_Interface &intfa) = 0;

  [[nodiscard]] virtual bool
  readDwgEntity(dwgBuffer *dbuf, objHandle &obj, DRW_Interface &intfa,
                bool *frameFailure = nullptr,
                DwgIntegrityAddressSpace offsetSpace =
                    DwgIntegrityAddressSpace::DecodedBuffer);
  [[nodiscard]] bool
  readDwgEntityWithOutput(dwgBuffer *dbuf, objHandle &obj, DRW_Interface &intfa,
                          DwgEntityOutput &output, bool *frameFailure = nullptr,
                          DwgIntegrityAddressSpace offsetSpace =
                              DwgIntegrityAddressSpace::DecodedBuffer);
  [[nodiscard]] bool readDwgObject(dwgBuffer *dbuf, objHandle &obj,
                                   DRW_Interface &intfa,
                                   bool *frameFailure = nullptr,
                                   DwgIntegrityAddressSpace offsetSpace =
                                       DwgIntegrityAddressSpace::DecodedBuffer);
  [[nodiscard]] bool publishDwgFramePublication(
      DRW_Interface &intfa, DRW_DwgFramePublication publication,
      DwgFramePublicationArtifacts artifacts = DwgFramePublicationArtifacts(),
      DwgFieldFamilyPublicationOutputs fieldOutputs =
          DwgFieldFamilyPublicationOutputs());
  void normalizeDwgFramePublication(DRW_DwgFramePublication &publication) const;
  [[nodiscard]] bool validateDwgFramePublicationStaticArtifacts(
      const DRW_DwgFramePublication &publication,
      DwgFramePublicationArtifacts artifacts) const noexcept;
  void finalizeDwgFrameCoverage(DRW_Interface &intfa, bool readCompleted);
  void finalizeDwgFrameCoverageNoThrow(DRW_Interface &intfa,
                                       bool readCompleted) noexcept;
  void beginDwgClassCoverage() noexcept;
  void prepareDwgClassSource(DRW_DwgClassSourceHandler *handler) noexcept;
  void captureDwgClassSource(const std::uint8_t *bytes, std::uint64_t size,
                            std::int32_t descriptor, std::uint64_t declaredSize) noexcept;
  void capturePhysicalDwgClassSource(dwgBuffer &buffer, std::uint64_t base,
                                    std::uint64_t size, std::int32_t descriptor) noexcept;
  void finalizeDwgClassSourceNoThrow(bool classReadCompleted) noexcept;
  [[nodiscard]] std::shared_ptr<std::vector<std::uint8_t>>
  allocateDwgClassSource(std::uint64_t size, std::int32_t descriptor,
                        DRW_DwgFrameOffsetSpace space, std::uint64_t base,
                        std::uint64_t declaredSize, bool backingValid) noexcept;
  void finalizeDwgClassCoverage(DRW_Interface &intfa, bool classReadCompleted);
  void finalizeDwgClassCoverageNoThrow(DRW_Interface &intfa,
                                       bool classReadCompleted) noexcept;
  [[nodiscard]] bool stageDwgClass(std::vector<DwgStagedClass> &stagedClasses,
                                   std::unique_ptr<DRW_Class> value,
                                   DRW_DwgClassCoverageEntry coverage);
  void
  recordDwgClassCoverageFailure(DRW_DwgClassCoverageEntry coverage,
                                DRW_DwgClassCoverageReason reason) noexcept;
  [[nodiscard]] DRW_DwgClassCoverageEntry
  makeDwgClassCoverageEntry(const DRW_Class &value,
                            std::int32_t sectionDescriptorId) const;
  [[nodiscard]] static bool
  dwgClassBitPosition(const dwgBuffer &buffer,
                      std::uint64_t &position) noexcept;
  [[nodiscard]] static bool
  setDwgClassBitRange(DRW_DwgClassBitRange &range, std::uint64_t start,
                      std::uint64_t end, DRW_DwgFrameOffsetSpace offsetSpace,
                      bool sectionRelative) noexcept;
  bool markDwgFrameOutcome(const DwgSourceFrameId &source,
                           DRW_DwgFrameDisposition disposition,
                           DRW_DwgFrameCoverageReason reason =
                               DRW_DwgFrameCoverageReason::None) noexcept;
  bool markDwgFrameOutcome(std::uint32_t handle,
                           DRW_DwgFrameDisposition disposition,
                           DRW_DwgFrameCoverageReason reason =
                               DRW_DwgFrameCoverageReason::None) noexcept;
  bool quarantineDwgFrame(std::uint32_t handle);
  bool quarantineDwgFrame(const DwgSourceFrameId &source);
  bool suppressDwgFrame(const DwgSourceFrameId &source, bool hasCoverage);
  bool suppressDwgFramePublication(const DRW_DwgFramePublication &publication);
  [[nodiscard]] DwgSourceFrameId
  sourceFrameIdForHandle(std::uint32_t handle) const noexcept;
  [[nodiscard]] bool
  reportDwgFrameTransitionFailure(const DwgSourceFrameId &source,
                                  std::uint64_t offset = 0,
                                  bool hasOffset = false) noexcept;
  void parseAttribs(DRW_Entity *e);
  void parseAttribs(DRW_TableEntry *e);
  std::string findTableName(DRW::TTYPE table, std::int32_t handle);
  std::string findAppIdName(std::uint32_t handle) const;

  void setCodePage(const std::string &c) { decoder.setCodePage(c, false); }
  std::string getCodePage() { return decoder.getCodePage(); }
  /// Canonical source byte-code-page name, independent of the primary text
  /// decoder (which remains UTF-16 for AC1021+).
  const std::string& getSourceCodePageName() const noexcept {
    return m_sourceCodePage.name;
  }
  /// Raw DWG envelope id. Valid only when hasSourceCodePageField() is true;
  /// callers must use that presence predicate because zero is undefined.
  std::uint16_t getSourceCodePageId() const noexcept {
    return m_sourceCodePage.rawId;
  }
  bool hasSourceCodePageField() const noexcept {
    return m_sourceCodePage.present;
  }
  bool hasSourceCodePage() const noexcept {
    return m_sourceCodePage.recognized;
  }
  /// Record one metadata envelope code-page id and configure the secondary
  /// byte codec. The state and codec are committed together on success.
  bool recordSourceCodePage(std::uint16_t id, bool present,
                            bool applyPrimaryCodec);
  [[nodiscard]] bool readDwgHeader(DRW_Header &hdr, dwgBuffer *buf,
                                   dwgBuffer *hBuf,
                                   DwgIntegrityAddressSpace offsetSpace =
                                       DwgIntegrityAddressSpace::DecodedBuffer,
                                   std::int32_t sectionId = -1,
                                   std::uint64_t sectionOffset = 0);
  [[nodiscard]] bool decodeDwgTemplate(DRW_Header& hdr,
                                       const std::uint8_t* data,
                                       std::size_t size,
                                       std::int32_t sectionId);
  [[nodiscard]] bool missingDwgTemplate();
  [[nodiscard]] bool readDwgHandles(
      dwgBuffer *dbuf, std::uint64_t offset, std::uint64_t size,
      std::uint64_t locationLimit = (std::numeric_limits<std::uint64_t>::max)(),
      DwgIntegrityAddressSpace offsetSpace = DwgIntegrityAddressSpace::None,
      std::int32_t sectionDescriptorId = -1);
  [[nodiscard]] bool consumeReadObjectBudget() noexcept {
    if (m_readObjectCount >= m_readObjectBudget) {
      m_readObjectBudgetExceeded = true;
      return false;
    }
    ++m_readObjectCount;
    return true;
  }
  void addIntegrityDiagnostic(DwgIntegrityDiagnostic diagnostic) noexcept;
  /// Record a bounded public read diagnostic from a version-specific reader.
  /// The legacy error enum still reports the enclosing section failure; this
  /// detail identifies semantic input that cannot be converted losslessly.
  void recordReadDiagnostic(DRW::OperationPhase phase,
                            DRW::OperationCause cause, const char *code,
                            const char *message, std::uint64_t offset = 0,
                            bool hasOffset = false) noexcept {
    if (parent == nullptr)
      return;
    try {
      parent->recordOperationDiagnostic(phase, cause, code, message, offset,
                                        hasOffset);
    } catch (...) {
      // Diagnostic allocation must never replace the legacy read result.
    }
  }
  void recordIntegrityDiagnostic(
      DwgIntegritySeverity severity, DwgIntegrityAddressSpace offsetSpace,
      DwgIntegrityPhase phase, DwgIntegrityCheckKind kind,
      std::int32_t logicalSectionId = -1, std::int32_t sectionDescriptorId = -1,
      const char *sectionName = nullptr, std::uint64_t pageId = 0,
      bool hasPageId = false, std::uint64_t offset = 0, bool hasOffset = false,
      std::uint32_t logicalHandle = 0, bool hasHandle = false,
      std::uint64_t expected = 0, std::uint64_t observed = 0,
      bool hasValues = false) noexcept;
  void recordObjectFrameFailure(const objHandle &object,
                                DwgIntegrityAddressSpace offsetSpace) noexcept;
  bool hasUsableDwgNodReference() const noexcept;
  void warnDwgNodReference(std::uint64_t expected, std::uint64_t observed,
                          const objHandle *object = nullptr,
                          DwgIntegrityAddressSpace offsetSpace =
                              DwgIntegrityAddressSpace::None) noexcept;
  void
  recordEntityFailure(const objHandle &object, std::int16_t type,
                      DwgEntityFailurePhase phase,
                      std::uint32_t blockRecordHandle = DRW::NoHandle) noexcept;
  [[nodiscard]] bool readDwgTables(DRW_Header &hdr, dwgBuffer *dbuf,
                                   DwgIntegrityAddressSpace offsetSpace =
                                       DwgIntegrityAddressSpace::DecodedBuffer);
  bool checkSentinel(dwgBuffer *buf, enum secEnum::DWGSection, bool start);
  //! Advance over the version-dependent bytes between the CLASSES CRC and
  //! its closing sentinel.  Real-world R18/R2007 files use both the legacy
  //! two-byte/no-tail layout and the newer eight-byte layout emitted by our
  //! writer.
  [[nodiscard]] bool readDwgClassesTail(dwgBuffer &buffer);
  [[nodiscard]] bool
  publishDwgClasses(std::vector<DwgStagedClass> &stagedClasses);

  [[nodiscard]] bool readDwgBlocks(DRW_Interface &intfa, dwgBuffer *dbuf,
                                   DwgIntegrityAddressSpace offsetSpace =
                                       DwgIntegrityAddressSpace::DecodedBuffer);
  [[nodiscard]] bool
  readDwgEntities(DRW_Interface &intfa, dwgBuffer *dbuf,
                  DwgIntegrityAddressSpace offsetSpace =
                      DwgIntegrityAddressSpace::DecodedBuffer);
  [[nodiscard]] bool
  readDwgObjects(DRW_Interface &intfa, dwgBuffer *dbuf,
                 DwgIntegrityAddressSpace offsetSpace =
                     DwgIntegrityAddressSpace::DecodedBuffer);
  [[nodiscard]] bool
  publishDeferredTableFramePublications(DRW_Interface &intfa);
  [[nodiscard]] bool
  publishDeferredRawObjects(DRW_Interface &intfa,
                            std::vector<DRW_UnsupportedObject> &objects);
  [[nodiscard]] bool validateDeferredCompoundState();
  void abandonDeferredCompoundState();
  //! Attach an AC1027+ DataStorage record to a parsed modeler/surface
  //! entity. Missing links are compatibility diagnostics, not parse errors.
  void linkDataStorage(DRW_Entity &entity);
  //! Finish DataStorage ownership accounting after all entity/object
  //! traversal has completed.
  void finalizeDataStorageLinks();
  class DwgBlockScopeTransaction;

  // Walk a block_record's child entities in firstEH..lastEH chain (pre-2004)
  // or entMap order (2004+) and dispatch each via readDwgEntity.  Used for
  // both named blocks (entities go into the active block) and modelspace /
  // paperspace (called post-endBlock so entities go into the interface's
  // modelspace container).
  bool walkBlockRecordEntities(DRW_Block_Record *bkr, dwgBuffer *dbuf,
                               DRW_Interface &intfa,
                               std::uint32_t expectedOwner = DRW::NoHandle,
                               std::uint32_t rawBlockOwner = DRW::NoHandle,
                               DwgIntegrityAddressSpace offsetSpace =
                                   DwgIntegrityAddressSpace::DecodedBuffer);
  bool walkJournalledBlockRecordEntities(
      DRW_Block_Record *bkr, dwgBuffer *dbuf, DRW_Interface &intfa,
      DwgBlockScopeTransaction &transaction,
      std::uint32_t expectedOwner = DRW::NoHandle,
      std::uint32_t rawBlockOwner = DRW::NoHandle,
      DwgIntegrityAddressSpace offsetSpace =
          DwgIntegrityAddressSpace::DecodedBuffer);

  struct DwgSourceFrameLease {
    objHandle object;
    DwgSourceFrameId source;
    bool hasCoverage = false;
  };

  struct DwgFrameClassification {
    enum class Route : std::uint8_t { Invalid, Entity, Object, BlockDelimiter };

    std::int16_t encodedType{-1};
    std::int16_t resolvedType{-1};
    const DRW_Class *resolvedClass{nullptr};
    std::uint32_t bodyByteSize{0};
    bool fixedObjectShell{false};
    Route route{Route::Invalid};
  };

  struct DwgFrameMapLease {
    enum class Origin : std::uint8_t { None, ObjectMap, DeferredObjectMap };

    objHandle object;
    DwgSourceFrameId source;
    Origin origin{Origin::None};
    bool hasCoverage = false;
    std::uint32_t bodyByteSize{0};
    std::optional<DwgFrameClassification> classification;
    std::optional<DwgObjectMap::node_type> node;

    DwgFrameMapLease() = default;
    DwgFrameMapLease(const DwgFrameMapLease &) = delete;
    DwgFrameMapLease &operator=(const DwgFrameMapLease &) = delete;
    DwgFrameMapLease(DwgFrameMapLease &&) noexcept = default;
    DwgFrameMapLease &operator=(DwgFrameMapLease &&) noexcept = default;

    [[nodiscard]] bool isDetached() const noexcept {
      return node.has_value() && !node->empty();
    }
  };

  class DwgBlockScopeTransaction final {
  public:
    explicit DwgBlockScopeTransaction(dwgReader &reader);
    ~DwgBlockScopeTransaction();

    DwgBlockScopeTransaction(const DwgBlockScopeTransaction &) = delete;
    DwgBlockScopeTransaction &
    operator=(const DwgBlockScopeTransaction &) = delete;

    [[nodiscard]] DwgBlockJournalOutput &output() noexcept;
    [[nodiscard]] bool reserveAdmission(std::size_t sourceCount,
                                        std::size_t eventCount,
                                        std::uint64_t bodyByteCount);
    [[nodiscard]] bool adopt(DwgFrameMapLease &lease);
    [[nodiscard]] bool releaseLast(const DwgSourceFrameId &expected,
                                   DwgFrameMapLease &lease) noexcept;
    [[nodiscard]] bool replay(DRW_Interface &target);
    [[nodiscard]] bool abort() noexcept;
    [[nodiscard]] std::size_t sourceCount() const noexcept;

  private:
    [[nodiscard]] bool reserveAdmissionImpl(std::size_t sourceCount,
                                            std::size_t eventCount,
                                            std::uint64_t bodyByteCount,
                                            bool consumeFailurePoint);
    [[nodiscard]] DwgFrameMapLease *
    findLease(const DwgSourceFrameId &source) noexcept;
    [[nodiscard]] bool retireLease(const DwgSourceFrameId &source) noexcept;
    [[nodiscard]] bool failLease(const DwgSourceFrameId &source) noexcept;

    dwgReader &m_reader;
    DwgBlockJournalOutput m_output;
    std::vector<DwgFrameMapLease> m_leases;
    std::unordered_map<std::uint32_t, std::size_t> m_leaseIndexes;
    std::uint64_t m_validatedBodyBytes{0};
    bool m_finished{false};
  };

  struct DwgFramePhaseSnapshot {
    enum class Destination : std::uint8_t {
      DeferredObjectMap,
      Published,
      Failed,
      Quarantined,
      Unresolved
    };

    DwgSourceFrameId source;
    DwgFrameClassification classification;
    DwgFrameMapLease::Origin origin{DwgFrameMapLease::Origin::None};
    Destination destination{Destination::Failed};
    DRW_DwgFrameDisposition disposition{DRW_DwgFrameDisposition::Pending};
  };

  struct DwgStagedFrame {
    std::optional<DwgFrameMapLease> lease;
    std::optional<DRW_DwgFramePublication> publication;

    DwgStagedFrame() = default;
    DwgStagedFrame(const DwgStagedFrame &) = delete;
    DwgStagedFrame &operator=(const DwgStagedFrame &) = delete;
    DwgStagedFrame(DwgStagedFrame &&) noexcept = default;
    DwgStagedFrame &operator=(DwgStagedFrame &&) noexcept = default;

    [[nodiscard]] bool hasDetachedLease() const noexcept {
      return lease.has_value() && lease->isDetached();
    }
  };

  struct StagedAttribState {
    std::shared_ptr<DRW_Attrib> entity;
    DwgStagedFrame frame;

    StagedAttribState() = default;
    StagedAttribState(const StagedAttribState &) = delete;
    StagedAttribState &operator=(const StagedAttribState &) = delete;
    StagedAttribState(StagedAttribState &&) noexcept = default;
    StagedAttribState &operator=(StagedAttribState &&) noexcept = default;
  };

  struct StagedAttributeLocation {
    std::uint32_t owner{DRW::NoHandle};
    std::size_t index{0u};
  };

  struct PendingInsertState {
    DRW_Insert entity;
    DwgStagedFrame frame;
    std::vector<StagedAttribState> attributes;
    // Modern INSERT membership is checked through a sorted, unique projection
    // built once when the owner arrives. The original handle order remains in
    // entity.attribHandles and continues to define callback ordering.
    std::vector<std::uint32_t> expectedAttributeHandles;

    PendingInsertState() = default;
    PendingInsertState(const PendingInsertState &) = delete;
    PendingInsertState &operator=(const PendingInsertState &) = delete;
    PendingInsertState(PendingInsertState &&) noexcept = default;
    PendingInsertState &operator=(PendingInsertState &&) noexcept = default;
  };

  struct OrphanAttribState {
    std::vector<StagedAttribState> attributes;

    OrphanAttribState() = default;
    OrphanAttribState(const OrphanAttribState &) = delete;
    OrphanAttribState &operator=(const OrphanAttribState &) = delete;
    OrphanAttribState(OrphanAttribState &&) noexcept = default;
    OrphanAttribState &operator=(OrphanAttribState &&) noexcept = default;
  };

  struct StagedSeqEndState {
    std::uint32_t owner = DRW::NoHandle;
    DwgStagedFrame frame;

    StagedSeqEndState() = default;
    StagedSeqEndState(const StagedSeqEndState &) = delete;
    StagedSeqEndState &operator=(const StagedSeqEndState &) = delete;
    StagedSeqEndState(StagedSeqEndState &&) noexcept = default;
    StagedSeqEndState &operator=(StagedSeqEndState &&) noexcept = default;
  };

  struct StagedVertexState {
    DRW_Vertex entity;
    DwgStagedFrame frame;

    StagedVertexState() = default;
    StagedVertexState(const StagedVertexState &) = delete;
    StagedVertexState &operator=(const StagedVertexState &) = delete;
    StagedVertexState(StagedVertexState &&) noexcept = default;
    StagedVertexState &operator=(StagedVertexState &&) noexcept = default;
  };

  struct PendingPolylineState {
    DRW_Polyline entity;
    DwgStagedFrame frame;
    std::vector<StagedVertexState> vertices;
    std::vector<std::pair<std::uint32_t, std::size_t>> declaredChildIndex;
    std::vector<std::uint8_t> stagedChildSlots;
    std::size_t retainedBytes{0};

    PendingPolylineState() = default;
    PendingPolylineState(const PendingPolylineState &) = delete;
    PendingPolylineState &operator=(const PendingPolylineState &) = delete;
    PendingPolylineState(PendingPolylineState &&) noexcept = default;
    PendingPolylineState &operator=(PendingPolylineState &&) noexcept = default;
  };

  struct OrphanPolylineVertexState {
    std::vector<StagedVertexState> vertices;
    std::set<std::uint32_t> childHandles;
    std::size_t retainedBytes{0};

    OrphanPolylineVertexState() = default;
    OrphanPolylineVertexState(const OrphanPolylineVertexState &) = delete;
    OrphanPolylineVertexState &
    operator=(const OrphanPolylineVertexState &) = delete;
    OrphanPolylineVertexState(OrphanPolylineVertexState &&) noexcept = default;
    OrphanPolylineVertexState &
    operator=(OrphanPolylineVertexState &&) noexcept = default;
  };

  struct DwgPreparedInsertCommit {
    std::vector<StagedAttribState *> orderedAttributes;
    std::vector<std::shared_ptr<DRW_Attrib>> orderedEntities;
    std::vector<std::uint32_t> claimedChildHandles;
    StagedSeqEndState *sequenceEnd{nullptr};
    bool claimedSeqEnd{false};
  };

  struct DwgPreparedPolylineCommit {
    std::vector<StagedVertexState *> orderedVertices;
    std::vector<std::uint32_t> claimedChildHandles;
    StagedSeqEndState *sequenceEnd{nullptr};
    bool claimedSeqEnd{false};
  };

  [[nodiscard]] bool borrowDwgSourceFrame(DwgObjectMap &map,
                                          DwgObjectMap::iterator it,
                                          DwgSourceFrameLease &lease);
  [[nodiscard]] bool takeDwgSourceFrame(DwgObjectMap &map,
                                        DwgObjectMap::iterator it,
                                        DwgSourceFrameLease &lease);
  [[nodiscard]] bool detachDwgSourceFrame(DwgObjectMap &map,
                                          DwgObjectMap::iterator it,
                                          DwgFrameMapLease &lease);
  [[nodiscard]] bool stageDetachedDwgSourceFrame(DwgFrameMapLease &lease);
  [[nodiscard]] bool restoreDwgSourceFrame(DwgFrameMapLease &lease);
  [[nodiscard]] bool deferDetachedDwgSourceFrame(DwgFrameMapLease &lease,
                                                 DwgObjectMap &target);
  [[nodiscard]] bool discardDetachedDwgSourceFrame(DwgFrameMapLease &lease);
  [[nodiscard]] bool deferDwgSourceFrame(const DwgSourceFrameLease &lease,
                                         DwgObjectMap &target);
  [[nodiscard]] bool
  classifyDwgSourceFrame(dwgBuffer *dbuf, const objHandle &object,
                         DwgFrameClassification &classification);
  [[nodiscard]] bool isMappedInsertSequenceEnd(dwgBuffer *dbuf,
                                               const objHandle &object);
  [[nodiscard]] bool preflightMappedPolylineOwnership(
      const std::vector<const DRW_Block_Record *> &records, dwgBuffer *dbuf);
  [[nodiscard]] static bool
  classificationsMatch(const DwgFrameClassification &expected,
                       const DwgFrameClassification &observed) noexcept;
  void
  recordDwgFramePhaseSnapshot(const DwgFrameMapLease &lease,
                              DwgFramePhaseSnapshot::Destination destination,
                              DRW_DwgFrameDisposition disposition) noexcept;
  [[nodiscard]] bool discardDwgSourceFrame(DwgObjectMap &map,
                                           DwgObjectMap::iterator it);
  [[nodiscard]] bool quarantineMappedDwgSourceFrame(std::uint32_t handle);
  [[nodiscard]] bool
  stageCurrentEntityFrame(DwgStagedFrame &frame,
                          const DRW_DwgFramePublication &publication);
  [[nodiscard]] bool validateStagedFrame(const DwgStagedFrame &frame);
  [[nodiscard]] bool validateStagedCompoundState();
  [[nodiscard]] bool hasPendingCompoundState() const noexcept;
  [[nodiscard]] bool
  hasPendingCompoundStateForBlock(const DRW_Block_Record &block) const;
  [[nodiscard]] bool abandonStagedCompoundState();
  [[nodiscard]] bool restoreStagedFrame(DwgStagedFrame &frame);
  [[nodiscard]] bool abandonStagedFrame(DwgStagedFrame &frame);
  [[nodiscard]] bool
  stageActiveEntityFrame(DwgStagedFrame &frame,
                         const DRW_DwgFramePublication &publication);
  [[nodiscard]] DwgMappedEntityOutcome
  stagePendingInsert(DRW_Insert &&insert,
                     const DRW_DwgFramePublication &publication,
                     DRW_Interface &intfa);
  [[nodiscard]] DwgMappedEntityOutcome
  stageMappedInsertAggregate(DRW_Insert &&insert,
                             const DRW_DwgFramePublication &publication,
                             dwgBuffer *dbuf, DRW_Interface &intfa,
                             DwgIntegrityAddressSpace offsetSpace);
  [[nodiscard]] DwgMappedEntityOutcome
  stageLegacyInsertAggregate(DRW_Insert &&insert,
                             const DRW_DwgFramePublication &publication,
                             dwgBuffer *dbuf, DRW_Interface &intfa,
                             DwgIntegrityAddressSpace offsetSpace);
  [[nodiscard]] DwgMappedEntityOutcome
  stageMappedPolylineAggregate(DRW_Polyline &&polyline,
                               const DRW_DwgFramePublication &publication,
                               dwgBuffer *dbuf, DRW_Interface &intfa,
                               DwgIntegrityAddressSpace offsetSpace);
  [[nodiscard]] DwgMappedEntityOutcome
  stagePendingAttribute(std::shared_ptr<DRW_Attrib> attribute,
                        const DRW_DwgFramePublication &publication,
                        DRW_Interface &intfa);
  [[nodiscard]] DwgMappedEntityOutcome
  stagePendingSeqEnd(std::uint32_t handle, std::uint32_t owner,
                     const DRW_DwgFramePublication &publication,
                     DRW_Interface &intfa);
  [[nodiscard]] DwgMappedEntityOutcome
  stagePendingPolyline(DRW_Polyline &&polyline,
                       const DRW_DwgFramePublication &publication,
                       DRW_Interface &intfa);
  [[nodiscard]] DwgMappedEntityOutcome
  stageLegacyPolylineChain(DRW_Polyline &&polyline,
                           const DRW_DwgFramePublication &publication,
                           dwgBuffer *dbuf, DRW_Interface &intfa,
                           DwgIntegrityAddressSpace offsetSpace);
  [[nodiscard]] DwgMappedEntityOutcome
  stagePendingPolylineVertex(DRW_Vertex &&vertex,
                             const DRW_DwgFramePublication &publication,
                             DRW_Interface &intfa);
  [[nodiscard]] DwgMappedEntityOutcome
  tryCommitPendingInsert(std::uint32_t handle, DRW_Interface &intfa);
  [[nodiscard]] DwgMappedEntityOutcome
  tryCommitPendingPolyline(std::uint32_t handle, DRW_Interface &intfa);
  [[nodiscard]] bool prepareInsertCommit(std::uint32_t handle,
                                         PendingInsertState &pending,
                                         DwgPreparedInsertCommit &prepared);
  [[nodiscard]] DwgMappedEntityOutcome
  deliverPreparedInsertCommit(std::uint32_t handle, PendingInsertState &pending,
                              DwgPreparedInsertCommit &prepared,
                              DRW_Interface &intfa);
  [[nodiscard]] DwgMappedEntityOutcome
  journalPreparedInsertCommit(std::uint32_t handle, PendingInsertState &pending,
                              DwgPreparedInsertCommit &prepared);
  [[nodiscard]] bool preparePolylineCommit(std::uint32_t handle,
                                           PendingPolylineState &pending,
                                           DwgPreparedPolylineCommit &prepared);
  [[nodiscard]] DwgMappedEntityOutcome deliverPreparedPolylineCommit(
      std::uint32_t handle, PendingPolylineState &pending,
      DwgPreparedPolylineCommit &prepared, DRW_Interface &intfa);
  [[nodiscard]] DwgMappedEntityOutcome
  journalPreparedPolylineCommit(std::uint32_t handle,
                                PendingPolylineState &pending,
                                DwgPreparedPolylineCommit &prepared);
  [[nodiscard]] bool
  claimInvalidSeqEndTerminalizerMarker(std::uint32_t handle,
                                       bool &inserted) noexcept;
  [[nodiscard]] bool claimInvalidInsertOwnerTerminalizerMarker(
      std::uint32_t owner, DwgTerminalizerFailurePoint failurePoint,
      bool &inserted) noexcept;
  void terminalizeInsertGroup(std::uint32_t handle,
                              DwgInsertTerminalReason reason);
  void abandonPendingInsertState(std::uint32_t handle);
  void terminalizeOrphanAttribOwner(std::uint32_t owner);
  void eraseStagedAttributeIndex(
      std::uint32_t owner,
      const std::vector<StagedAttribState> &attributes) noexcept;
  void releaseInsertExpectedAttributeCount(
      const PendingInsertState &pending) noexcept;
  void terminalizePendingPolylineState(std::uint32_t handle,
                                       DwgInsertTerminalReason reason);
  void terminalizeOrphanPolylineVertexOwner(std::uint32_t owner);
  [[nodiscard]] bool polylineOwnerMapBucketBytes(
      std::size_t &bytes) const noexcept;
  [[nodiscard]] bool reservePendingPolylineOwnerMapForOneMore(
      std::size_t stagedAdditionalBytes) noexcept;
  [[nodiscard]] bool reserveOrphanPolylineOwnerMapForOneMore(
      std::size_t stagedAdditionalBytes) noexcept;
  void releasePolylineStagingBytes(std::size_t &bytes) noexcept;
  bool addPolylineDeclaredClaims(
      std::uint32_t owner,
      const std::vector<std::pair<std::uint32_t, std::size_t>> &declaredIndex)
      noexcept;
  void removePolylineDeclaredClaims(
      std::uint32_t owner,
      const std::vector<std::pair<std::uint32_t, std::size_t>> &declaredIndex)
      noexcept;
  bool addPolylineOrphanClaim(std::uint32_t owner,
                              std::uint32_t child) noexcept;
  void removePolylineOrphanClaims(
      std::uint32_t owner,
      const std::set<std::uint32_t> &children) noexcept;
  [[nodiscard]] bool
  readMappedDwgEntity(dwgBuffer *dbuf, DwgFrameMapLease &lease,
                      DRW_Interface &intfa, bool *frameFailure,
                      DwgIntegrityAddressSpace offsetSpace,
                      DwgEntityOutput *output = nullptr,
                      DwgMappedEntityCompletion completion =
                          DwgMappedEntityCompletion::Immediate,
                      DwgBlockScopeTransaction *transaction = nullptr);
  [[nodiscard]] bool requiresLegacyCompoundHandling(
      dwgBuffer *dbuf, const objHandle &object,
      DwgFrameClassification *classification = nullptr);

public:
  DwgObjectMap ObjectMap;
  DwgObjectMap objObjectMap; // stores objects deferred from ENTITIES
  DwgObjectMap remainingMap; // debug-only unconsumed records
  std::vector<DRW_UnsupportedObject> m_deferredRawObjects;
  std::vector<DRW_DwgFramePublication> m_deferredTableFramePublications;
  std::unordered_map<std::uint32_t, DRW_LType *> ltypemap;
  std::unordered_map<std::uint32_t, DRW_Layer *> layermap;
  /// Layer / linetype names in file storage order (the handlesList iteration
  /// order). This is the index space the proxy-graphics ATTRIBUTE_LAYER(16) /
  /// ATTRIBUTE_LINETYPE(18) opcodes reference; each slot is index-aligned with
  /// the control's handlesList (empty string for a missing record). Verified
  /// byte-identical to the dwgread/LibreDWG layer order. See
  /// DRW_ProxyGraphicDecoder::decode.
  std::vector<std::string> m_layerNameOrder;
  std::vector<std::string> m_ltypeNameOrder;
  std::unordered_map<std::uint32_t, DRW_Block *> blockmap;
  std::unordered_map<std::uint32_t, DRW_Textstyle *> stylemap;
  std::unordered_map<std::uint32_t, DRW_Dimstyle *> dimstylemap;
  std::unordered_map<std::uint32_t, DRW_Vport *> vportmap;
  std::unordered_map<std::uint32_t, DRW_Block_Record *> blockRecordmap;

  /// Resolved DBCOLOR (AcDbColor) lookup, populated as the OBJECTS section
  /// is decoded.  Key: handle of the AcDbColor object.  Value: pair of
  /// (24-bit RGB, display name) — entities referencing this handle (via
  /// ENC flag 0x40) get color24 + colorName patched from this map after
  /// their parseDwg returns.  Names are formatted as "BOOK$ENTRY" when a
  /// book name is present, otherwise just the entry name.
  std::unordered_map<std::uint32_t, std::pair<std::int32_t, std::string>>
      dbColorMap;
  /// MLINESTYLE handle → style name. Populated as MLINESTYLE objects are
  /// parsed; consumed by the entryParse template hook to stamp
  /// styleName onto each MLINE entity post-parse (DXF code 2 / DWG 340
  /// resolves to a name only after the OBJECTS section is read).
  std::unordered_map<std::uint32_t, std::string> mlineStyleNameMap;

  /// SCALE (AcDbScale) handle → entry. Populated as SCALE objects are
  /// parsed in the OBJECTS section. Foundation for per-viewport-scale
  /// resolution: annotation-scaled MLEADER/MTEXT/DIMENSION entities
  /// reference these handles via their AcDbAnnotScaleObjectContextData
  /// chain.  scaleFactor() == drawingUnits / paperUnits.
  std::unordered_map<std::uint32_t, DRW_Scale> scaleMap;

  /// Per-entity parseDwg failures accumulated across readDwgBlocks /
  /// readDwgEntities and walkBlockRecordEntities.
  /// These are reported as warnings — they do not fail the section.
  /// Section-level (structural) failures still propagate via the
  /// bool return from each section method.
  size_t m_entityParseFailures = 0;
  /// Per-object parseDwg failures accumulated in readDwgObjects. Mirrors
  /// m_entityParseFailures — non-fatal warnings tracked for caller reporting.
  size_t m_objectParseFailures = 0;
  /// CLASSES-section CRC mismatches (warn-only compatibility policy). Zero
  /// CRC fields are treated as absent for legacy writers; non-zero
  /// mismatches are surfaced via dwgRW::getClassesCrcMismatch().
  size_t m_classesCrcMismatch = 0;
  /// R2007 header, system-page, and data-page integrity mismatches.
  /// These are diagnostics only: malformed ranges, RS failures, and
  /// decompression failures remain hard parse errors.
  size_t m_r2007CrcMismatch = 0;
  /// R2004 encrypted file-header CRC mismatches. The DWG specification
  /// defines this CRC over the complete decrypted 0x6c-byte header; a
  /// mismatch is diagnostic-only so compatible third-party files remain
  /// readable.
  size_t m_r2004CrcMismatch = 0;
  std::vector<DwgIntegrityDiagnostic> m_integrityDiagnostics;
  std::size_t m_integrityDiagnosticsDropped{0};
  /// Capped internal detail for fixture diagnosis. Counts above remain the
  /// public compatibility diagnostic; this list never affects parsing.
  std::vector<DwgEntityFailureDiagnostic> m_entityFailureDiagnostics;
  /// AC1027+ modeler/surface entities advertising DataStorage whose handle
  /// could not be resolved. This is a non-fatal diagnostic.
  size_t m_dataStorageLinkFailures = 0;
  /// AC1027+ DataStorage records not referenced by a parsed modeler/surface
  /// entity. This is a non-fatal diagnostic.
  size_t m_dataStorageOrphanRecords = 0;
  /// Payload bytes copied from the retained AcDs record index onto unique
  /// entity records. Bounded independently from the section's source copies.
  std::size_t m_dataStorageLinkedPayloadBytes{0};
  /// Custom-class entities (oType >= 500, recName not in our hardcoded
  /// dwgType map) that fell through readDwgEntity's default branch and
  /// got stuffed into objObjectMap.  Keyed by the DXF recName (eg
  /// "STDPART2D", "ACDBVISUALSTYLE", "ACMBOMROW").  These are
  /// vendor-extension entities — typically AutoCAD Mechanical / Civil
  /// proxy-capable graphics — whose geometry never reaches the
  /// renderer.  Surface to the user so they know what's missing.
  std::unordered_map<std::string, size_t> m_skippedCustomClasses;
  /// Count of render primitives recovered by decoding the cached proxy
  /// graphics of raw-net custom entities (STDPART2D, AEC_*, …) — these are
  /// emitted through the interface IN ADDITION to the raw object, so a
  /// non-zero value means previously-invisible geometry now renders.
  size_t m_decodedProxyPrimitives = 0;
  /// OBJECTS-section records that libdxfrw still cannot decode. Unlike
  /// m_skippedCustomClasses, this also includes non-graphical metadata such
  /// as reactors, filters, TABLECONTENT, dynamic-block graphs, etc.
  std::unordered_map<std::string, size_t> m_skippedUnsupportedObjects;
  std::vector<DRW_RawDwgSection> m_rawDwgSections;
  std::optional<dwgHandle> m_dwgNodReference;
  bool m_dwgNodPublished{false};
  bool m_dwgNodFailureReported{false};
  //! Typed AcDb:AcDsPrototype_1b index (PR-2a). Parallel to raw section store.
  std::vector<DRW_DataStorageSection> m_dataStorageSections;
  //! Preferred DataStorage records successfully linked while the entity
  //! stream is traversed. Record indexes distinguish duplicate revisions
  //! with the same numeric/canonical handle.
  std::set<std::pair<std::size_t, std::size_t>> m_dataStorageLinkedRecords;
  std::unordered_map<std::uint32_t, DRW_AppId *> appIdmap;
  std::unordered_map<std::uint32_t, DRW_View *> viewmap;
  std::unordered_map<std::uint32_t, DRW_UCS *> ucsmap;

  // INSERT handles whose ATTRIB sequence was structurally invalid. These
  // parents must never be published later by an out-of-order child.
  std::unordered_set<std::uint32_t> m_invalidInsertOwners;
  std::unordered_set<std::uint32_t> m_invalidPolylineOwners;
  // Children of a BLOCK_RECORD whose header or scope delimiters failed
  // validation must not be republished by the later catch-all entity sweep.
  std::unordered_set<std::uint32_t> m_quarantinedEntityHandles;
  std::unordered_set<std::uint32_t> m_invalidSeqEndHandles;
  // A sequence terminator can complete exactly one parent compound entity.
  // Keep validated-but-unconsumed and consumed states separate so a
  // duplicate parent reference cannot reuse the cached terminator.
  std::unordered_set<std::uint32_t> m_consumedSeqEndHandles;
  // Modern BLOCK_RECORD entMap entries retain compound children after their
  // owner consumes them. Remember those children so the ordered walk does
  // not report a successful child parse as a missing entity.
  std::unordered_set<std::uint32_t> m_consumedCompoundChildHandles;
  // Seeded from the validated HANDLE-map stream, not from ObjectMap's
  // unordered iteration. Every later map transition records a checked
  // disposition against this stable source identity.
  std::vector<DRW_DwgFrameCoverageEntry> m_dwgSourceFrameLedger;
  std::unordered_map<std::uint32_t, std::size_t> m_dwgSourceFrameIndexes;
  DRW_DwgFrameCoverageStatus m_dwgFrameCoverageStatus{
      DRW_DwgFrameCoverageStatus::NotAvailable};
  bool m_dwgFrameCoverageIntegrityViolation{false};
  bool m_dwgFrameCoveragePublished{false};
  DRW_DwgClassCoverageReport m_dwgClassCoverageReport;
  DRW_DwgClassSourceHandler *m_dwgClassSourceHandler{nullptr};
  DRW_DwgClassSourceReport m_dwgClassSourceReport;
  std::size_t m_dwgClassSourceByteLimit{DRW::kMaxDwgClassSourceBytes};
  bool m_dwgClassSourceRequested{false};
  bool m_dwgClassSourceRequestFailed{false};
  bool m_dwgClassSourceDelivered{false};
  //    std::uint32_t currBlock;
  std::uint8_t maintenanceVersion{0};
  // Application maintenance release version (file-header byte 0x12). This —
  // NOT maintenanceVersion (byte 0x0B, the "maintenance release version") —
  // is the field that gates the R2010+ hSize/bitsize_hi reads (libreDWG
  // calls byte 0x0B `is_maint` and reads its gate field `maint_version` from
  // 0x12). On ODA-converted files both bytes are > 3 so the distinction is
  // invisible, but genuine AutoCAD RTM files can have 0x0B <= 3 with 0x12 > 3.
  std::uint8_t appMaintenanceVersion{0};

protected:
  std::unique_ptr<dwgBuffer> fileBuf;
  dwgRW *parent{nullptr};
  DRW::Version version{DRW::UNKNOWNV};

  // A compound child owns its parsed representation, coverage receipt, and
  // detached source node together until one parent reaches a terminal result.
  std::unordered_map<std::uint32_t, PendingInsertState> m_pendingInsertStates;
  std::unordered_map<std::uint32_t, OrphanAttribState> m_orphanAttribStates;
  // One deterministic logarithmic lookup per staged ATTRIB. Its size is
  // admitted against MaxDwgReadObjectCount before every insertion.
  std::map<std::uint32_t, StagedAttributeLocation> m_stagedAttributeIndex;
  // Aggregate count of retained sorted expected-handle projections. Bounding
  // this as well as staged child nodes prevents many malformed INSERT owners
  // from multiplying per-owner index allocations.
  std::size_t m_stagedInsertExpectedAttributeCount{0u};
  std::unordered_map<std::uint32_t, PendingPolylineState>
      m_pendingPolylineStates;
  std::unordered_map<std::uint32_t, OrphanPolylineVertexState>
      m_orphanPolylineVertexStates;
  dwgReaderDetail::PolylineDeclaredClaimIndex<> m_polylineDeclaredClaims;
  // Inverse index for staged child-first POLYLINE vertices. A later-arriving
  // parent can detect foreign ownership without walking all orphan vectors.
  dwgReaderDetail::PolylineDeclaredClaimIndex<> m_polylineOrphanClaims;
  std::size_t m_polylineStagingBytes{0};
  std::unordered_map<std::uint32_t, StagedSeqEndState> m_stagedSeqEnds;
  DwgTerminalizerFailurePoint m_terminalizerFailurePointForTest{
      DwgTerminalizerFailurePoint::None};
  DwgPolylinePrepareFailurePoint m_polylinePrepareFailurePointForTest{
      DwgPolylinePrepareFailurePoint::None};
  DwgPolylineStageFailurePoint m_polylineStageFailurePointForTest{
      DwgPolylineStageFailurePoint::None};
  DwgBlockJournalReservationFailurePoint
      m_blockJournalReservationFailurePointForTest{
          DwgBlockJournalReservationFailurePoint::None};
  std::size_t m_blockJournalReservationFailureOccurrenceForTest{0};
  DwgBlockJournalAdoptFailurePoint m_blockJournalAdoptFailurePointForTest{
      DwgBlockJournalAdoptFailurePoint::None};
  std::size_t m_blockJournalAdoptFailureOccurrenceForTest{0};
  // Classification is immutable provenance while a mapped frame waits in
  // OBJECTS. It is moved back into a lease before destination dispatch.
  std::unordered_map<std::uint32_t, DwgFrameClassification>
      m_deferredFrameClassifications;
  // Probe-only routing evidence. This must remain observational: allocation
  // failure cannot affect map ownership, callbacks, or frame coverage.
  std::vector<DwgFramePhaseSnapshot> m_dwgFramePhaseSnapshots;

  // seeker (position) for the beginning sentinel of the image data (R13 to R15)
  std::uint32_t previewImagePos;

  // sections map
  std::unordered_map<int, dwgSectionInfo> sections;
  // Modern files may contain vendor-defined data sections that are not in
  // secEnum. Keep their validated descriptors staged separately so the
  // public raw-section callback can preserve them without pretending they
  // are one of the standard sections.
  std::vector<dwgSectionInfo> m_unknownSections;
  std::unordered_map<std::uint32_t, DRW_Class *> classesmap;

  // Aggregate object-map work budget for the current read operation. The
  // HANDLE stream is the first point where every physical object is known,
  // so enforcing the ceiling there bounds both staging and downstream work.
  std::size_t m_readObjectBudget {dwgSafety::MaxDwgReadObjectCount};
  std::size_t m_readObjectCount {0};
  bool m_readObjectBudgetExceeded {false};

protected:
  std::unordered_map<std::uint32_t, std::uint64_t> m_dwgClassNumberOrdinals;
  bool m_dwgClassCoveragePublished{false};
  bool m_dwgClassCoverageCaptureFailed{false};
  DRW_TextCodec decoder;
  DwgSourceCodePage m_sourceCodePage;

protected:
  //    std::uint32_t blockCtrl;
  std::uint32_t nextEntLink{0};
  std::uint32_t prevEntLink{0};
  bool nextEntLinkImplicit{false};
  // Set only while walking a named pre-2004 BLOCK_RECORD.  Modelspace and
  // paperspace legacy entities have no DWG-side owner handle.
  std::uint32_t expectedBlockEntityOwner{DRW::NoHandle};
  // Modern ObjectMap leftovers must not escape their BLOCK_RECORD.  A
  // nonzero common owner is structural evidence that the block walk missed
  // the entity, not a free-standing top-level entity.
  bool rejectOwnedEntityInSweep{false};
  // BLOCK_RECORD reachability context for opaque raw entities. This remains
  // set for model/paper-space walks even when expectedBlockEntityOwner is 0.
  std::uint32_t rawBlockEntityOwner{DRW::NoHandle};
  // Model/paper-space ownership is identified by the BLOCK_RECORD name, not
  // by format-specific handle values.
  bool ownerlessSpaceWalk{false};
  // ObjectMap key currently being decoded; typed entity callbacks must not
  // publish a body carrying a different object handle.
  std::uint32_t expectedParsedEntityHandle{DRW::NoHandle};
  bool parsedEntityHandleMismatch{false};
  bool parsedEntityOwnerMismatch{false};

  struct DwgEntityFramePublicationCapture {
    DRW_DwgFramePublication publication;
    bool typedViewParsed = false;
    bool rawViewIssued = false;
    bool rawViewHasTypedPeer = false;
  };
  DwgEntityFramePublicationCapture *m_activeEntityFrameCapture{nullptr};
  DwgFrameMapLease *m_activeEntityFrameLease{nullptr};
  DwgMappedEntityOutcome *m_activeMappedEntityOutcome{nullptr};
  DwgBlockScopeTransaction *m_activeBlockTransaction{nullptr};
  DwgEntityOutput *m_activeBlockOutput{nullptr};
  DwgEntityFailurePhase m_currentEntityFailurePhase{
      DwgEntityFailurePhase::None};

  template <typename T, typename Callback>
  void emitWithExtrusion(T &e, DwgEntityOutput &output, Callback callback) {
    if (parent && parent->applyExt) {
      if (!e.haveExtrusion &&
          (e.extPoint.x != 0.0 || e.extPoint.y != 0.0 || e.extPoint.z != 1.0)) {
        e.haveExtrusion = true;
      }
      e.applyExtrusion();
    }
    output.appendValue(e, callback);
  }

private:
  template <class T>
  bool entryParse(T &e, dwgBuffer &buff, std::uint32_t bs, bool &ret) {
    ret = e.parseDwg(version, &buff, bs);
    // Some legacy object parsers return true after an optional/common
    // handle helper has failed.  The bounded frame cursor is authoritative
    // for structural validity; never publish such a partially parsed
    // entity through the public callback API.
    ret = ret && buff.isGood();
    if (!ret)
      m_currentEntityFailurePhase = DwgEntityFailurePhase::TypedBody;
    if (ret && expectedParsedEntityHandle != DRW::NoHandle &&
        e.handle != expectedParsedEntityHandle) {
      ret = false;
      parsedEntityHandleMismatch = true;
      m_currentEntityFailurePhase = DwgEntityFailurePhase::Identity;
    }
    if (ret) {
      // ATTRIB, SEQEND, and VERTEX are compound children owned by an
      // INSERT or POLYLINE rather than the enclosing BLOCK_RECORD.
      // Validate ordinary block-level entities before a public callback
      // can observe a cross-block link.
      bool ownerMismatch = false;
      if (rejectOwnedEntityInSweep) {
        const auto ownerIt = blockRecordmap.find(e.parentHandle);
        const bool ownerClaimsEntity =
            ownerIt != blockRecordmap.end() && ownerIt->second != nullptr &&
            std::any_of(
                ownerIt->second->entMap.cbegin(),
                ownerIt->second->entMap.cend(),
                [&](std::uint32_t handle) { return handle == e.handle; });
        ownerMismatch =
            e.eType != DRW::ATTRIB && e.eType != DRW::SEQEND &&
            e.parentHandle != DRW::NoHandle
            // The R2004+ ownership list is authoritative. A stale
            // owner value without an entMap claim remains recoverable.
            && ownerClaimsEntity;
      } else if (expectedBlockEntityOwner != DRW::NoHandle) {
        ownerMismatch = e.eType != DRW::ATTRIB && e.eType != DRW::SEQEND &&
                        e.parentHandle != expectedBlockEntityOwner;
      } else if (ownerlessSpaceWalk) {
        ownerMismatch = e.eType != DRW::ATTRIB && e.eType != DRW::SEQEND &&
                        e.parentHandle != DRW::NoHandle;
      }
      if (ownerMismatch) {
        ret = false;
        parsedEntityOwnerMismatch = true;
        m_currentEntityFailurePhase = DwgEntityFailurePhase::Identity;
      }
    }
    if (ret) {
      parseAttribs(&e);
      if (m_activeEntityFrameCapture != nullptr &&
          e.handle == m_activeEntityFrameCapture->publication.m_handle) {
        DRW_DwgFramePublication &publication =
            m_activeEntityFrameCapture->publication;
        publication.setCommonLinkEvidence(drwDwgCommonLinkEvidenceForLinks(
            e.hasDwgCommonLinkTail(), e.parentHandle, e.reactorHandles,
            e.dwgReactorCount(), e.xDictHandle));
        publication.m_parentHandle = e.parentHandle;
        publication.m_reactorHandles = e.reactorHandles;
        publication.m_xDictHandle = e.xDictHandle;
        publication.m_numReactors = e.dwgReactorCount();
        publication.m_xDictFlag = e.dwgXDictionaryFlag();
        m_activeEntityFrameCapture->typedViewParsed = true;
      }
      nextEntLink = e.nextEntLink;
      prevEntLink = e.prevEntLink;
      nextEntLinkImplicit = version < DRW::AC1018 && e.haveNextLinks != 0;
      // Resolve AcDbColor reference (ENC flag 0x40) against the
      // OBJECTS-section DBCOLOR map populated by readDwgObject.
      // Patches color24 and colorName onto the entity for downstream
      // filters (DXF code 420 / 430).  Non-entity Ts that lack these
      // fields are still accepted because every T derives from
      // DRW_Entity which exposes them.
      if (e.acDbColorHandle != 0) {
        auto it = dbColorMap.find(e.acDbColorHandle);
        if (it != dbColorMap.end()) {
          // color24 patched only if not already inline-set by
          // ENC RGB (flag 0x80 path). Inline ENC name (flags
          // 0x41/0x42) takes precedence over the DBCOLOR target's
          // name — libreDWG model: these are entity-level
          // overrides distinct from the DBCOLOR's own name.
          if (it->second.first >= 0 && e.color24 == -1)
            e.color24 = it->second.first;
          if (!it->second.second.empty() && e.colorName.empty())
            e.colorName = it->second.second;
        }
      }
    }

    return ret;
  }
};

#endif // DWGREADER_H
