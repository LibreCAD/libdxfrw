#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "dwg2dxf/dx_iface.h"

namespace {

// Authored from scratch with ezdxf 1.4.4 api.body_from_mesh(cube()) and a
// 2-by-3 planar rectangle, export_sat(version=700). The date alone is fixed.
// No outside drawing bytes. These are carrier controls, not kernel evidence.
// Plain SAT SHA256: cube 7b4ff7f43548b5c654402bde60ae7ef3171174f4ac732cc04d8ea09884badc39;
// region ee3ca81c07435202a2cf836988e2912a5bb2f27ffbfeae9b314509342e2591bc.
const char cubeSat[] = R"SAT(700 0 1 0
@25 ezdxf v1.4.4 ACIS Builder @12 ACIS 32.0 NT @24 Thu Oct 15 00:00:00 2026
1 9.9999999999999995e-007 1e-010
body $-1 -1 $-1 $1 $-1 $-1 #
lump $-1 -1 $-1 $-1 $2 $0 #
shell $-1 -1 $-1 $-1 $-1 $3 $-1 $1 #
face $-1 -1 $-1 $6 $4 $2 $-1 $5 forward single #
loop $-1 -1 $-1 $-1 $7 $3 #
plane-surface $-1 -1 $-1 -0.5 -0.5 -0.5 0 0 -1 0 1 0 forward_v I I I I #
face $-1 -1 $-1 $10 $8 $2 $-1 $9 forward single #
coedge $-1 -1 $-1 $13 $12 $14 $11 forward $4 $-1 #
loop $-1 -1 $-1 $-1 $15 $6 #
plane-surface $-1 -1 $-1 0.5 -0.5 -0.5 1 0 0 0 1 0 forward_v I I I I #
face $-1 -1 $-1 $18 $16 $2 $-1 $17 forward single #
edge $-1 -1 $-1 $19 0 $20 1 $7 $21 forward @7 unknown #
coedge $-1 -1 $-1 $7 $23 $24 $22 reversed $4 $-1 #
coedge $-1 -1 $-1 $23 $7 $26 $25 reversed $4 $-1 #
coedge $-1 -1 $-1 $29 $28 $7 $11 reversed $27 $-1 #
coedge $-1 -1 $-1 $32 $31 $23 $30 forward $8 $-1 #
loop $-1 -1 $-1 $-1 $33 $10 #
plane-surface $-1 -1 $-1 -0.5 0.5 -0.5 0 1 0 0 0 1 forward_v I I I I #
face $-1 -1 $-1 $35 $27 $2 $-1 $34 forward single #
vertex $-1 -1 $-1 $11 $36 #
vertex $-1 -1 $-1 $11 $37 #
straight-curve $-1 -1 $-1 -0.5 -0.5 -0.5 0 1 0 I I #
edge $-1 -1 $-1 $19 0 $38 1 $12 $39 forward @7 unknown #
coedge $-1 -1 $-1 $12 $13 $15 $30 reversed $4 $-1 #
coedge $-1 -1 $-1 $42 $41 $12 $22 forward $40 $-1 #
edge $-1 -1 $-1 $43 0 $20 1 $13 $44 forward @7 unknown #
coedge $-1 -1 $-1 $33 $45 $13 $25 forward $16 $-1 #
loop $-1 -1 $-1 $-1 $29 $18 #
coedge $-1 -1 $-1 $14 $47 $33 $46 reversed $27 $-1 #
coedge $-1 -1 $-1 $47 $14 $41 $48 forward $27 $-1 #
edge $-1 -1 $-1 $38 0 $43 1 $23 $49 forward @7 unknown #
coedge $-1 -1 $-1 $15 $51 $42 $50 reversed $8 $-1 #
coedge $-1 -1 $-1 $51 $15 $45 $52 forward $8 $-1 #
coedge $-1 -1 $-1 $53 $26 $28 $46 forward $16 $-1 #
plane-surface $-1 -1 $-1 -0.5 -0.5 -0.5 -1 0 0 0 0 1 forward_v I I I I #
face $-1 -1 $-1 $55 $40 $2 $-1 $54 forward single #
point $-1 -1 $-1 -0.5 -0.5 -0.5 #
point $-1 -1 $-1 -0.5 0.5 -0.5 #
vertex $-1 -1 $-1 $30 $56 #
straight-curve $-1 -1 $-1 -0.5 -0.5 -0.5 1 0 0 I I #
loop $-1 -1 $-1 $-1 $24 $35 #
coedge $-1 -1 $-1 $24 $57 $29 $48 reversed $40 $-1 #
coedge $-1 -1 $-1 $57 $24 $31 $50 forward $40 $-1 #
vertex $-1 -1 $-1 $25 $58 #
straight-curve $-1 -1 $-1 0.5 0.5 -0.5 -1 0 0 I I #
coedge $-1 -1 $-1 $26 $53 $32 $52 reversed $16 $-1 #
edge $-1 -1 $-1 $20 0 $59 1 $33 $60 forward @7 unknown #
coedge $-1 -1 $-1 $28 $29 $62 $61 forward $27 $-1 #
edge $-1 -1 $-1 $19 0 $63 1 $29 $64 forward @7 unknown #
straight-curve $-1 -1 $-1 0.5 -0.5 -0.5 0 1 0 I I #
edge $-1 -1 $-1 $38 0 $65 1 $31 $66 forward @7 unknown #
coedge $-1 -1 $-1 $31 $32 $68 $67 reversed $8 $-1 #
edge $-1 -1 $-1 $43 0 $69 1 $32 $70 forward @7 unknown #
coedge $-1 -1 $-1 $45 $33 $72 $71 reversed $16 $-1 #
plane-surface $-1 -1 $-1 -0.5 -0.5 -0.5 0 -1 0 1 0 0 forward_v I I I I #
face $-1 -1 $-1 $-1 $73 $2 $-1 $74 forward single #
point $-1 -1 $-1 0.5 -0.5 -0.5 #
coedge $-1 -1 $-1 $41 $42 $76 $75 reversed $40 $-1 #
point $-1 -1 $-1 0.5 0.5 -0.5 #
vertex $-1 -1 $-1 $46 $77 #
straight-curve $-1 -1 $-1 -0.5 0.5 -0.5 0 0 1 I I #
edge $-1 -1 $-1 $63 0 $59 1 $47 $78 forward @7 unknown #
coedge $-1 -1 $-1 $76 $72 $47 $61 reversed $73 $-1 #
vertex $-1 -1 $-1 $48 $79 #
straight-curve $-1 -1 $-1 -0.5 -0.5 -0.5 0 0 1 I I #
vertex $-1 -1 $-1 $67 $80 #
straight-curve $-1 -1 $-1 0.5 -0.5 -0.5 0 0 1 I I #
edge $-1 -1 $-1 $65 0 $69 1 $51 $81 forward @7 unknown #
coedge $-1 -1 $-1 $72 $76 $51 $67 forward $73 $-1 #
vertex $-1 -1 $-1 $52 $82 #
straight-curve $-1 -1 $-1 0.5 0.5 -0.5 0 0 1 I I #
edge $-1 -1 $-1 $69 0 $59 1 $53 $83 forward @7 unknown #
coedge $-1 -1 $-1 $62 $68 $53 $71 forward $73 $-1 #
loop $-1 -1 $-1 $-1 $76 $55 #
plane-surface $-1 -1 $-1 -0.5 -0.5 0.5 0 0 1 1 0 0 forward_v I I I I #
edge $-1 -1 $-1 $63 0 $65 1 $57 $84 forward @7 unknown #
coedge $-1 -1 $-1 $68 $62 $57 $75 forward $73 $-1 #
point $-1 -1 $-1 -0.5 0.5 0.5 #
straight-curve $-1 -1 $-1 -0.5 -0.5 0.5 0 1 0 I I #
point $-1 -1 $-1 -0.5 -0.5 0.5 #
point $-1 -1 $-1 0.5 -0.5 0.5 #
straight-curve $-1 -1 $-1 0.5 -0.5 0.5 0 1 0 I I #
point $-1 -1 $-1 0.5 0.5 0.5 #
straight-curve $-1 -1 $-1 0.5 0.5 0.5 -1 0 0 I I #
straight-curve $-1 -1 $-1 -0.5 -0.5 0.5 1 0 0 I I #
End-of-ACIS-data
)SAT";

const char regionSat[] = R"SAT(700 0 1 0
@25 ezdxf v1.4.4 ACIS Builder @12 ACIS 32.0 NT @24 Thu Oct 15 00:00:00 2026
1 9.9999999999999995e-007 1e-010
body $-1 -1 $-1 $2 $-1 $1 #
transform $-1 -1 1 0 0 0 1 0 0 0 1 1 1.5 0 1 no_rotate no_reflect no_shear #
lump $-1 -1 $-1 $-1 $3 $0 #
shell $-1 -1 $-1 $-1 $-1 $4 $-1 $2 #
face $-1 -1 $-1 $-1 $5 $3 $-1 $6 forward double out #
loop $-1 -1 $-1 $-1 $7 $4 #
plane-surface $-1 -1 $-1 -1 -1.5 0 0 0 1 1 0 0 forward_v I I I I #
coedge $-1 -1 $-1 $10 $9 $-1 $8 forward $5 $-1 #
edge $-1 -1 $-1 $11 0 $12 2 $7 $13 forward @7 unknown #
coedge $-1 -1 $-1 $7 $15 $-1 $14 reversed $5 $-1 #
coedge $-1 -1 $-1 $15 $7 $-1 $16 forward $5 $-1 #
vertex $-1 -1 $-1 $8 $17 #
vertex $-1 -1 $-1 $8 $18 #
straight-curve $-1 -1 $-1 -1 -1.5 0 1 0 0 I I #
edge $-1 -1 $-1 $11 0 $19 3 $9 $20 forward @7 unknown #
coedge $-1 -1 $-1 $9 $10 $-1 $21 forward $5 $-1 #
edge $-1 -1 $-1 $12 0 $22 3 $10 $23 forward @7 unknown #
point $-1 -1 $-1 -1 -1.5 0 #
point $-1 -1 $-1 1 -1.5 0 #
vertex $-1 -1 $-1 $21 $24 #
straight-curve $-1 -1 $-1 -1 -1.5 0 0 1 0 I I #
edge $-1 -1 $-1 $22 0 $19 2 $15 $25 forward @7 unknown #
vertex $-1 -1 $-1 $16 $26 #
straight-curve $-1 -1 $-1 1 -1.5 0 0 1 0 I I #
point $-1 -1 $-1 -1 1.5 0 #
straight-curve $-1 -1 $-1 1 1.5 0 -1 0 0 I I #
point $-1 -1 $-1 1 1.5 0 #
End-of-ACIS-data
)SAT";

void setCarrier(DRW_ModelerGeometry& entity, const char* sat) {
    // The model retains the encoded carrier, not plaintext SAT. Apply the
    // existing ODA SAT-v1 involution, leaving caret escaping to the writer.
    std::istringstream input(sat);
    std::string line;
    while (std::getline(input, line)) {
        if (!entity.m_rawBytes.empty()) entity.m_rawBytes.push_back('\n');
        const std::size_t start = entity.m_rawBytes.size();
        for (unsigned char value : line)
            entity.m_rawBytes.push_back(value > 0x20u ? 0x9Fu - value : value);
        entity.m_dxfPayloadChunks.push_back({1, start, line.size()});
    }
    entity.m_modelerVersion = 1;
}

struct StoredRow {
    DRW::ETYPE type;
    std::string owner, target;
    std::vector<std::uint8_t> carrier;
    DRW_Coord placement;
    bool operator==(const StoredRow& other) const {
        return type == other.type && owner == other.owner && target == other.target
            && carrier == other.carrier && placement.x == other.placement.x
            && placement.y == other.placement.y && placement.z == other.placement.z;
    }
};

struct Census {
    std::map<std::uint32_t, StoredRow> rows;
    std::map<std::string, std::pair<std::uint32_t, std::uint32_t>> definitions;
    std::set<std::string> reachable;
    std::size_t records = 0, edges = 0, bytes = 0, visits = 0;
    bool operator==(const Census& other) const {
        return rows == other.rows && definitions == other.definitions
            && reachable == other.reachable && records == other.records
            && edges == other.edges && bytes == other.bytes && visits == other.visits;
    }
};

struct Limits { std::size_t records = 64, edges = 64, bytes = 65536; };

bool collect(const dx_data& data, Census& output, Limits limits = {}) {
    // Test-only bounded identity/denominator oracle. It neither diagnoses the
    // converter nor expands geometry/instances. Publish only a complete census.
    Census staged;
    std::map<std::uint32_t, std::string> owners;
    std::set<std::uint32_t> identities;
    if (data.mBlock == nullptr) return false;
    for (const auto* block : data.blocks) {
        if (!block || staged.definitions.size() >= limits.records
            || block->name.empty() || block->handle == 0
            || block->parentHandle == 0 || block->parentHandle == DRW::NoHandle
            || !identities.insert(block->handle).second
            || !identities.insert(block->parentHandle).second
            || !owners.emplace(block->parentHandle, block->name).second
            || !staged.definitions.emplace(block->name,
                    std::make_pair(block->handle, block->parentHandle)).second)
            return false;
    }
    const auto append = [&](const dx_ifaceBlock& block, const std::string& owner) {
        for (const DRW_Entity* entity : block.ent) {
            if (!entity || staged.records >= limits.records
                || entity->handle == 0 || !identities.insert(entity->handle).second)
                return false;
            if (entity->parentHandle != DRW::NoHandle) {
                const auto found = owners.find(entity->parentHandle);
                if (found == owners.end() || found->second != owner) return false;
            }
            StoredRow row{entity->eType, owner, {}, {}, {}};
            if (const auto* modeler = dynamic_cast<const DRW_ModelerGeometry*>(entity)) {
                if (modeler->m_rawBytes.empty() || staged.bytes > limits.bytes
                    || modeler->m_rawBytes.size() > limits.bytes - staged.bytes)
                    return false;
                staged.bytes += modeler->m_rawBytes.size();
                row.carrier = modeler->m_rawBytes;
            } else if (const auto* insert = dynamic_cast<const DRW_Insert*>(entity)) {
                if (staged.edges >= limits.edges
                    || staged.definitions.count(insert->name) != 1) return false;
                ++staged.edges;
                row.target = insert->name;
                row.placement = insert->basePoint;
            } else if (entity->eType != DRW::LINE) return false;
            ++staged.records;
            staged.rows.emplace(entity->handle, std::move(row));
        }
        return true;
    };
    if (!append(*data.mBlock, "*MODEL_SPACE")) return false;
    for (const auto* block : data.blocks)
        if (!append(*block, block->name)) return false;
    std::map<std::string, std::vector<std::string>> adjacency;
    for (const auto& entry : staged.rows)
        if (!entry.second.target.empty())
            adjacency[entry.second.owner].push_back(entry.second.target);
    std::vector<std::string> queue{"*MODEL_SPACE"};
    while (!queue.empty()) {
        const std::string owner = std::move(queue.back());
        queue.pop_back();
        ++staged.visits;
        const auto found = adjacency.find(owner);
        if (found == adjacency.end()) continue;
        for (const std::string& target : found->second)
            if (staged.reachable.insert(target).second) queue.push_back(target);
    }
    output = std::move(staged);
    return true;
}

std::unique_ptr<dx_data> makeDrawing(bool reverseBlocks, bool reverseEntities) {
    auto data = std::make_unique<dx_data>();
    std::map<std::string, dx_ifaceBlock*> blocks;
    for (const char* name : {"OnlyCube", "Mixed", "Left", "Right", "Root", "Unused"}) {
        auto block = std::make_unique<dx_ifaceBlock>();
        block->name = name;
        block->handle = 0x1000u + static_cast<std::uint32_t>(blocks.size());
        block->parentHandle = block->handle + 0x1000u;
        blocks.emplace(name, block.get());
        data->blocks.push_back(block.release());
    }
    const auto body = [](dx_ifaceBlock& owner, std::uint32_t id,
                         DRW::ETYPE type, const char* payload) {
        auto entity = std::make_unique<DRW_ModelerGeometry>(type);
        entity->handle = id;
        if (owner.handle != 0) entity->parentHandle = owner.parentHandle;
        setCarrier(*entity, payload);
        owner.ent.push_back(entity.release());
    };
    const auto insert = [](dx_ifaceBlock& owner, std::uint32_t id,
                           const char* name, double x) {
        auto entity = std::make_unique<DRW_Insert>();
        entity->handle = id;
        if (owner.handle != 0) entity->parentHandle = owner.parentHandle;
        entity->name = name;
        entity->basePoint = {x, x + 2, 0};
        owner.ent.push_back(entity.release());
    };
    const auto line = [](dx_ifaceBlock& owner, std::uint32_t id) {
        auto entity = std::make_unique<DRW_Line>();
        entity->handle = id;
        if (owner.handle != 0) entity->parentHandle = owner.parentHandle;
        entity->secPoint = {1, 2, 3};
        owner.ent.push_back(entity.release());
    };
    body(*blocks.at("OnlyCube"), 0x900, DRW::E3DSOLID, cubeSat);
    body(*blocks.at("Mixed"), 0x300, DRW::REGION, regionSat);
    line(*blocks.at("Mixed"), 0x180);
    insert(*blocks.at("Left"), 0x710, "OnlyCube", 1);
    insert(*blocks.at("Right"), 0x110, "OnlyCube", 2);
    insert(*blocks.at("Root"), 0x520, "Left", 3);
    insert(*blocks.at("Root"), 0x210, "Right", 4);
    insert(*blocks.at("Root"), 0x620, "Mixed", 5);
    body(*blocks.at("Unused"), 0x120, DRW::BODY, regionSat);
    body(*data->mBlock, 0x450, DRW::BODY, cubeSat);
    insert(*data->mBlock, 0x600, "Root", 3);
    insert(*data->mBlock, 0x220, "Root", 10);
    line(*data->mBlock, 0x880);
    if (reverseBlocks) data->blocks.reverse();
    if (reverseEntities) {
        data->mBlock->ent.reverse();
        for (auto* block : data->blocks) block->ent.reverse();
    }
    return data;
}

} // namespace

bool runModelerBlockCensusTests() {
    bool passed = true;
    std::size_t positives = 0, negatives = 0;
    auto source = makeDrawing(false, false);
    Census expected;
    DRW_ModelerGeometry cube(DRW::E3DSOLID), region(DRW::REGION);
    setCarrier(cube, cubeSat); setCarrier(region, regionSat);
    // Literal graph expectation, not another call to the collector being
    // tested. This locks each owner/type/target/payload/placement independently.
    expected.definitions = {{"OnlyCube", {0x1000, 0x2000}}, {"Mixed", {0x1001, 0x2001}},
                            {"Left", {0x1002, 0x2002}}, {"Right", {0x1003, 0x2003}},
                            {"Root", {0x1004, 0x2004}}, {"Unused", {0x1005, 0x2005}}};
    expected.rows = {
        {0x900, {DRW::E3DSOLID, "OnlyCube", {}, cube.m_rawBytes, {}}},
        {0x300, {DRW::REGION, "Mixed", {}, region.m_rawBytes, {}}},
        {0x180, {DRW::LINE, "Mixed", {}, {}, {}}},
        {0x710, {DRW::INSERT, "Left", "OnlyCube", {}, {1, 3, 0}}},
        {0x110, {DRW::INSERT, "Right", "OnlyCube", {}, {2, 4, 0}}},
        {0x520, {DRW::INSERT, "Root", "Left", {}, {3, 5, 0}}},
        {0x210, {DRW::INSERT, "Root", "Right", {}, {4, 6, 0}}},
        {0x620, {DRW::INSERT, "Root", "Mixed", {}, {5, 7, 0}}},
        {0x120, {DRW::BODY, "Unused", {}, region.m_rawBytes, {}}},
        {0x450, {DRW::BODY, "*MODEL_SPACE", {}, cube.m_rawBytes, {}}},
        {0x600, {DRW::INSERT, "*MODEL_SPACE", "Root", {}, {3, 5, 0}}},
        {0x220, {DRW::INSERT, "*MODEL_SPACE", "Root", {}, {10, 12, 0}}},
        {0x880, {DRW::LINE, "*MODEL_SPACE", {}, {}, {}}}
    };
    expected.reachable = {"OnlyCube", "Mixed", "Left", "Right", "Root"};
    expected.records = 13; expected.edges = 7; expected.visits = 6;
    expected.bytes = 2 * (cube.m_rawBytes.size() + region.m_rawBytes.size());
    if (cube.m_rawBytes == region.m_rawBytes) return false;
    for (bool blocks : {false, true}) for (bool entities : {false, true}) {
        const auto data = makeDrawing(blocks, entities);
        Census observed;
        const bool equal = collect(*data, observed) && observed == expected;
        if (equal) ++positives;
        passed = equal && passed;
    }
    for (int mutation = 0; mutation < 7; ++mutation) {
        Census changed = expected;
        if (mutation == 0) std::swap(changed.rows.at(0x900).carrier, changed.rows.at(0x300).carrier);
        if (mutation == 1) changed.rows.at(0x900).owner = "Unused";
        if (mutation == 2) changed.rows.at(0x710).target = "Mixed";
        if (mutation == 3) changed.rows.at(0x220).placement.x = 3;
        if (mutation == 4) changed.rows.at(0x900).type = DRW::BODY;
        if (mutation == 5) changed.rows.at(0x900).carrier.clear();
        if (mutation == 6) changed.definitions.at("OnlyCube").second = 0x2FFF;
        const bool rejects = !(changed == expected) && changed.rows.size() == expected.rows.size();
        if (rejects) ++negatives;
        passed = rejects && passed;
    }
    auto* firstBlock = source->blocks.front();
    auto* solid = firstBlock->ent.front();
    for (int mutation = 0; mutation < 4; ++mutation) {
        const auto handle = solid->handle;
        const auto owner = solid->parentHandle;
        const std::string name = firstBlock->name;
        if (mutation == 0) solid->handle = 0x300;
        if (mutation == 1) solid->parentHandle = 0xEEEE;
        if (mutation == 2) firstBlock->name = "MissingCube";
        if (mutation == 3) solid->parentHandle = 0x2001; // Existing wrong owner.
        Census unchanged = expected;
        const bool rejects = !collect(*source, unchanged) && unchanged == expected;
        if (rejects) ++negatives;
        passed = rejects && passed;
        solid->handle = handle; solid->parentHandle = owner; firstBlock->name = name;
    }
    Limits exact{expected.records, expected.edges, expected.bytes};
    for (int resource = 0; resource < 3; ++resource) {
        Limits limits = exact;
        if (resource == 0) --limits.records;
        if (resource == 1) --limits.edges;
        if (resource == 2) --limits.bytes;
        Census unchanged = expected;
        const bool rejects = !collect(*source, unchanged, limits) && unchanged == expected;
        if (rejects) ++negatives;
        passed = rejects && passed;
        const bool retry = collect(*source, unchanged, exact) && unchanged == expected;
        if (retry) ++positives;
        passed = retry && passed;
    }
    // An INSERT reference cycle is not a hard-ownership cycle. This stored
    // census terminates by visiting each definition once, without expansion.
    auto left = std::next(source->blocks.begin(), 2);
    auto* cycle = static_cast<DRW_Insert*>((*left)->ent.front());
    cycle->name = "Root";
    Census cyclic;
    const bool boundedCycle = collect(*source, cyclic) && cyclic.visits == 6
        && cyclic.records == 13 && cyclic.edges == 7 && cyclic.reachable.size() == 5;
    if (boundedCycle) ++positives;
    passed = boundedCycle && passed;
    std::cout << "Modeler stored-block census B1: " << (passed ? "PASS" : "FAIL")
              << " (positive=" << positives << ", negative=" << negatives << ")\n";
    return passed;
}
