/* Bounded AC1018 TABLE -> *T1 cache membership check. This is a same-import
 * graph observation only; it does not prove cached geometry semantics or
 * authorize TABLE output. */
#ifndef DWG_TABLE_CACHE_MEMBERSHIP_AC1018_CANDIDATE_H
#define DWG_TABLE_CACHE_MEMBERSHIP_AC1018_CANDIDATE_H

#include "../drw_entities.h"
#include "../drw_objects.h"
#include "../../dwg2dxf/dx_data.h"

namespace DwgTableCacheMembershipAc1018Candidate {
inline bool matches(const DRW_Table& table,
                    const std::vector<DRW_Block_Record>& records,
                    const std::list<dx_ifaceBlock*>& blocks) noexcept {
    if (table.handle == 0u || table.handle == DRW::NoHandle
        || table.name != "*T1"
        || !table.m_semanticContentComplete)
        return false;

    const DRW_Block_Record* record = nullptr;
    for (const auto& candidate : records) {
        if (candidate.name != table.name)
            continue;
        if (record || candidate.handle == 0u
            || candidate.handle == DRW::NoHandle)
            return false;
        record = &candidate;
    }
    if (!record)
        return false;

    const dx_ifaceBlock* block = nullptr;
    for (const auto* candidate : blocks) {
        if (!candidate || candidate->name != table.name)
            continue;
        if (block)
            return false;
        block = candidate;
    }
    if (!block || block->handle == 0u || block->handle == DRW::NoHandle
        || block->parentHandle != record->handle || !block->hasEndBlockData
        || block->endBlockData.parentHandle != record->handle
        || block->ent.size() != 15u
        || record->entityMapCount() != block->ent.size())
        return false;

    std::size_t mtext = 0u;
    std::size_t line = 0u;
    std::vector<std::uint32_t> childHandles;
    try {
        childHandles.reserve(block->ent.size());
    } catch (...) {
        return false;
    }
    for (auto* entity : block->ent) {
        if (!entity || entity->handle == 0u || entity->handle == DRW::NoHandle
            || entity->parentHandle != record->handle)
            return false;
        if (entity->eType == DRW::MTEXT)
            ++mtext;
        else if (entity->eType == DRW::LINE)
            ++line;
        else
            return false;
        try {
            childHandles.push_back(entity->handle);
        } catch (...) {
            return false;
        }
        for (auto* prior : block->ent) {
            if (prior == entity)
                break;
            if (prior && prior->handle == entity->handle)
                return false;
        }
    }
    return mtext == 7u && line == 8u
        && record->matchesEntityMap(childHandles);
}
}

#endif
