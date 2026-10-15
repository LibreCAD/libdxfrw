#include <algorithm>
#include <cstdint>
#include <chrono>
#include <filesystem>
#include <fstream>
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
#include "intern/dxfreader.h"

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

Census expectedDrawing() {
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
    return expected;
}

} // namespace

bool runModelerBlockCensusTests() {
    bool passed = true;
    std::size_t positives = 0, negatives = 0;
    auto source = makeDrawing(false, false);
    const Census expected = expectedDrawing();
    if (expected.rows.at(0x900).carrier == expected.rows.at(0x300).carrier) return false;
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

namespace {

std::string ownerName(const std::string& name) {
    if (name == "*Model_Space" || name == "*MODEL_SPACE") return "*MODEL_SPACE";
    if (name == "*Paper_Space" || name == "*PAPER_SPACE") return "*PAPER_SPACE";
    return name;
}

bool fixedSpace(const std::string& name) {
    return ownerName(name) == "*MODEL_SPACE" || ownerName(name) == "*PAPER_SPACE";
}

struct PhysicalRow {
    std::string type, section, block, name;
    std::uint32_t handle = 0, owner = 0;
    unsigned handles = 0, owners = 0, coordinateMask = 0, modelerVersions = 0, histories = 0;
    int modelerVersion = 0;
    std::uint32_t history = 0;
    DRW_Coord point, end;
    std::vector<std::uint8_t> carrier;
    std::vector<std::string> subclasses;
};

struct PhysicalGraph {
    std::map<std::string, std::uint32_t> blockRecords;
    std::map<std::string, std::pair<std::uint32_t, std::uint32_t>> blocks, footers;
    std::map<std::uint32_t, StoredRow> rows;
    std::map<std::uint32_t, std::uint32_t> owners;
};

bool physicalGraph(const std::filesystem::path& path, PhysicalGraph& result) {
    std::error_code error;
    const auto size = std::filesystem::file_size(path, error);
    if (error || size > 131072) return false; // Authored profile, not a format maximum.
    std::ifstream input(path, std::ios::binary);
    char signature[22]{};
    input.read(signature, sizeof(signature));
    const bool binary = std::string(signature, sizeof(signature))
        == std::string("AutoCAD Binary DXF\r\n\x1a\0", 22);
    input.clear(); input.seekg(binary ? 22 : 0);
    std::unique_ptr<dxfReader> reader;
    if (binary) reader = std::make_unique<dxfReaderBinary>(&input);
    else reader = std::make_unique<dxfReaderAscii>(&input);
    PhysicalGraph staged;
    PhysicalRow row;
    std::set<std::uint32_t> identities;
    std::set<std::string> sections;
    std::string section, block;
    bool sectionName = false;
    unsigned appDepth = 0;
    const auto finish = [&]() {
        if (row.type.empty()) return true;
        if (appDepth != 0 || row.handles > 1 || row.owners > 1) return false;
        if (row.handles != 0 && (row.handle == 0 || !identities.insert(row.handle).second))
            return false;
        if (row.section == "TABLES" && row.type == "BLOCK_RECORD")
            return row.handles == 1 && !row.name.empty()
                && staged.blockRecords.emplace(ownerName(row.name), row.handle).second;
        if (row.section == "BLOCKS" && row.type == "BLOCK") {
            if (!block.empty() || row.handles != 1 || row.owners != 1 || row.name.empty())
                return false;
            block = ownerName(row.name);
            return staged.blocks.emplace(block, std::make_pair(row.handle, row.owner)).second;
        }
        if (row.section == "BLOCKS" && row.type == "ENDBLK") {
            if (block.empty() || row.handles != 1 || row.owners != 1) return false;
            const bool inserted = staged.footers.emplace(
                block, std::make_pair(row.handle, row.owner)).second;
            block.clear();
            return inserted;
        }
        if (row.section != "ENTITIES" && row.section != "BLOCKS") return true;
        if (row.handles != 1 || row.owners != 1 || staged.rows.size() >= 64
            || (row.section == "BLOCKS" && (block.empty() || fixedSpace(block))))
            return false;
        DRW::ETYPE type;
        if (row.type == "3DSOLID") type = DRW::E3DSOLID;
        else if (row.type == "REGION") type = DRW::REGION;
        else if (row.type == "BODY") type = DRW::BODY;
        else if (row.type == "INSERT") type = DRW::INSERT;
        else if (row.type == "LINE") type = DRW::LINE;
        else return false;
        const bool modeler = type == DRW::E3DSOLID || type == DRW::REGION || type == DRW::BODY;
        if (modeler) {
            std::vector<std::string> wanted{"AcDbEntity", "AcDbModelerGeometry"};
            if (type == DRW::E3DSOLID) wanted.push_back("AcDb3dSolid");
            if (row.subclasses != wanted || row.modelerVersions != 1 || row.modelerVersion != 1
                || row.carrier.empty() || row.histories != (type == DRW::E3DSOLID ? 1u : 0u)
                || row.history != 0) return false;
        } else if (type == DRW::INSERT) {
            if (row.coordinateMask != 7u || row.name.empty()) return false;
        } else if (row.coordinateMask != 63u || row.point.x != 0 || row.point.y != 0
                   || row.point.z != 0 || row.end.x != 1 || row.end.y != 2 || row.end.z != 3)
            return false;
        const std::string owner = row.section == "ENTITIES" ? "*MODEL_SPACE" : block;
        StoredRow stored{type, owner, type == DRW::INSERT ? row.name : std::string{},
                         row.carrier, type == DRW::INSERT ? row.point : DRW_Coord{}};
        return staged.owners.emplace(row.handle, row.owner).second
            && staged.rows.emplace(row.handle, std::move(stored)).second;
    };
    int code = 0;
    std::size_t pairs = 0;
    while (++pairs <= 10000 && reader->readRec(&code)) {
        if (code == 0) {
            if (!finish()) {
                std::cerr << "  raw record: " << row.section << '/' << row.type
                          << " handle=" << row.handle << " owner=" << row.owner
                          << " bytes=" << row.carrier.size() << " coordinates=" << row.coordinateMask
                          << " name=" << row.name << '\n';
                return false;
            }
            row = {};
            const std::string type = reader->getString();
            if (type == "SECTION") {
                if (!section.empty() || sectionName || !block.empty()) return false;
                sectionName = true;
            } else if (type == "ENDSEC") {
                if (section.empty() || !block.empty()) return false;
                section.clear();
            } else if (type == "EOF") {
                if (!section.empty() || sectionName || !block.empty()
                    || sections.count("BLOCKS") != 1 || sections.count("ENTITIES") != 1
                    || sections.count("TABLES") != 1 || input.peek() != std::char_traits<char>::eof())
                    return false;
                result = std::move(staged);
                return true;
            } else {
                if (section.empty() || sectionName) return false;
                row.type = type; row.section = section;
            }
            continue;
        }
        if (sectionName) {
            if (code != 2 || !sections.insert(reader->getString()).second) return false;
            section = reader->getString(); sectionName = false;
            continue;
        }
        if (row.type.empty()) continue; // HEADER variables are not record identities.
        if (code == 102) {
            const std::string marker = reader->getString();
            if (!marker.empty() && marker.front() == '{') ++appDepth;
            else if (marker == "}" && appDepth != 0) --appDepth;
            else return false;
            continue;
        }
        if (appDepth != 0) continue;
        // DIMSTYLE's self identity is105; its group5 is a legacy arrow-block
        // name. Do not turn an empty name into a null/duplicate self handle.
        if ((code == 5 && row.type != "DIMSTYLE") || (code == 105 && row.type == "DIMSTYLE")) {
            row.handle = reader->getHandleString(); ++row.handles;
        }
        else if (code == 330) { row.owner = reader->getHandleString(); ++row.owners; }
        else if (code == 2) row.name = reader->getString();
        else if (code == 100) row.subclasses.push_back(reader->getString());
        else if (code == 70) { row.modelerVersion = reader->getInt32(); ++row.modelerVersions; }
        else if (code == 350) { row.history = reader->getHandleString(); ++row.histories; }
        else if (row.type == "LINE" || row.type == "INSERT") {
            unsigned bit = 0;
            double* coordinate = nullptr;
            if (code == 10) { coordinate = &row.point.x; bit = 1; }
            if (code == 20) { coordinate = &row.point.y; bit = 2; }
            if (code == 30) { coordinate = &row.point.z; bit = 4; }
            if (code == 11) { coordinate = &row.end.x; bit = 8; }
            if (code == 21) { coordinate = &row.end.y; bit = 16; }
            if (code == 31) { coordinate = &row.end.z; bit = 32; }
            if (coordinate) {
                if ((row.coordinateMask & bit) != 0) return false;
                row.coordinateMask |= bit; *coordinate = reader->getDouble();
            }
        } else if ((row.type == "3DSOLID" || row.type == "REGION" || row.type == "BODY")
                   && (code == 1 || code == 3)) {
            if (code == 1 && !row.carrier.empty()) row.carrier.push_back('\n');
            const std::string text = reader->getString();
            if (text.size() > 255 || row.carrier.size() + text.size() > 65536) return false;
            for (std::size_t i = 0; i < text.size(); ++i) {
                row.carrier.push_back(static_cast<std::uint8_t>(text[i]));
                if (text[i] == '^' && i + 1 < text.size() && text[i + 1] == ' ') ++i;
            }
        }
    }
    return false;
}

bool comparePhysical(const PhysicalGraph& graph, std::map<std::uint32_t, std::uint32_t>& bindings) {
    const Census expected = expectedDrawing();
    if (graph.rows.size() != expected.rows.size() || graph.blockRecords.size() != 8
        || graph.blocks.size() != 8 || graph.footers.size() != 8) return false;
    for (const auto& block : graph.blocks) {
        const auto table = graph.blockRecords.find(block.first);
        const auto footer = graph.footers.find(block.first);
        if (table == graph.blockRecords.end() || footer == graph.footers.end()
            || block.second.second != table->second || footer->second.second != table->second
            || (!fixedSpace(block.first) && expected.definitions.count(block.first) != 1))
            return false;
    }
    std::map<std::uint32_t, std::uint32_t> staged;
    for (const auto& entry : graph.rows) {
        const auto owner = graph.blockRecords.find(entry.second.owner);
        const auto physicalOwner = graph.owners.find(entry.first);
        if (owner == graph.blockRecords.end() || physicalOwner == graph.owners.end()
            || physicalOwner->second != owner->second
            || (!entry.second.target.empty() && graph.blocks.count(entry.second.target) != 1))
            return false;
        std::uint32_t source = 0;
        for (const auto& wanted : expected.rows)
            if (entry.second == wanted.second) {
                if (source != 0) return false;
                source = wanted.first;
            }
        if (source == 0 || !staged.emplace(source, entry.first).second) return false;
    }
    if (staged.size() != expected.rows.size()) return false;
    for (const auto& definition : expected.definitions) {
        const auto block = graph.blocks.find(definition.first);
        const auto record = graph.blockRecords.find(definition.first);
        if (block == graph.blocks.end() || record == graph.blockRecords.end()
            || !staged.emplace(definition.second.first, block->second.first).second
            || !staged.emplace(definition.second.second, record->second).second) return false;
    }
    bindings = std::move(staged);
    return true;
}

bool typedMatches(const std::filesystem::path& path, const PhysicalGraph& graph) {
    dx_data data;
    dx_iface importer;
    if (!importer.fileImport(path.string(), &data, false) || data.sourceVersion != DRW::AC1024
        || data.blockRecords.size() != graph.blockRecords.size()) {
        std::cerr << "  typed header/records: version=" << data.sourceVersion
                  << " records=" << data.blockRecords.size() << '/' << graph.blockRecords.size() << '\n';
        return false;
    }
    std::set<std::uint32_t> tableIds;
    for (const auto& record : data.blockRecords) {
        const auto found = graph.blockRecords.find(ownerName(record.name));
        if (found == graph.blockRecords.end() || found->second != record.handle
            || !tableIds.insert(record.handle).second) return false;
    }
    std::set<std::uint32_t> seen;
    const auto entities = [&](const dx_ifaceBlock& block, const std::string& owner) {
        for (const auto* entity : block.ent) {
            if (!entity || !seen.insert(entity->handle).second) return false;
            const auto found = graph.rows.find(entity->handle);
            if (found == graph.rows.end() || found->second.owner != owner
                || graph.owners.at(entity->handle) != entity->parentHandle) {
                std::cerr << "  typed entity identity/owner: " << entity->handle << " parent=" << entity->parentHandle << " owner=" << owner << '\n';
                return false;
            }
            StoredRow row{entity->eType, owner, {}, {}, {}};
            if (const auto* modeler = dynamic_cast<const DRW_ModelerGeometry*>(entity)) {
                if (modeler->m_modelerVersion != 1 || modeler->m_historyHandle != 0) return false;
                row.carrier = modeler->m_rawBytes;
                // processModelerGeometry supplies one terminal LF outside the
                // retained group-1/3 chunk ranges. Normalize only that framing
                // byte; internal whitespace and every encoded byte stay exact.
                if (row.carrier.size() == found->second.carrier.size() + 1
                    && row.carrier.back() == '\n') row.carrier.pop_back();
            } else if (const auto* insert = dynamic_cast<const DRW_Insert*>(entity)) {
                row.target = insert->name; row.placement = insert->basePoint;
            } else if (const auto* line = dynamic_cast<const DRW_Line*>(entity)) {
                if (line->basePoint.x != 0 || line->basePoint.y != 0 || line->basePoint.z != 0
                    || line->secPoint.x != 1 || line->secPoint.y != 2 || line->secPoint.z != 3)
                    return false;
            } else return false;
            if (!(row == found->second)) {
                std::cerr << "  typed entity fields: " << entity->handle << " type=" << row.type
                          << " raw-type=" << found->second.type << " bytes=" << row.carrier.size()
                          << '/' << found->second.carrier.size() << " target=" << row.target << '/' << found->second.target << '\n';
                return false;
            }
        }
        return true;
    };
    if (!entities(*data.mBlock, "*MODEL_SPACE") || data.blocks.size() != graph.blocks.size()) {
        std::cerr << "  typed model/blocks: " << data.mBlock->ent.size()
                  << " blocks=" << data.blocks.size() << '/' << graph.blocks.size() << '\n';
        return false;
    }
    std::set<std::string> blockNames;
    for (const auto* block : data.blocks) {
        if (!block) return false;
        if (!blockNames.insert(ownerName(block->name)).second) return false;
        const auto found = graph.blocks.find(ownerName(block->name));
        if (found == graph.blocks.end() || found->second.first != block->handle
            || found->second.second != block->parentHandle
            || !entities(*block, ownerName(block->name))) {
            std::cerr << "  typed block: " << block->name << " handle=" << block->handle
                      << " parent=" << block->parentHandle << " entities=" << block->ent.size();
            if (found != graph.blocks.end()) std::cerr << " raw=" << found->second.first << '/' << found->second.second;
            std::cerr << '\n';
            return false;
        }
    }
    return seen.size() == graph.rows.size();
}

bool verifyOutput(const std::filesystem::path& path,
                  std::map<std::uint32_t, std::uint32_t>* outputBindings = nullptr) {
    PhysicalGraph graph;
    std::map<std::uint32_t, std::uint32_t> bindings;
    if (!physicalGraph(path, graph)) { std::cerr << "Modeler block raw scan failed: " << path << '\n'; return false; }
    if (!comparePhysical(graph, bindings)) { std::cerr << "Modeler block owner/carrier binding failed: " << path << '\n'; return false; }
    if (!typedMatches(path, graph)) { std::cerr << "Modeler block typed handoff failed: " << path << '\n'; return false; }
    if (outputBindings) *outputBindings = std::move(bindings);
    return true;
}

std::size_t physicalOracleControls(const std::filesystem::path& path) {
    PhysicalGraph original;
    std::map<std::uint32_t, std::uint32_t> bindings;
    if (!physicalGraph(path, original) || !comparePhysical(original, bindings)
        || bindings.size() != 25) return 0;
    std::size_t rejected = 0;
    for (int mutation = 0; mutation < 6; ++mutation) {
        PhysicalGraph changed = original;
        const auto solid = bindings.at(0x900), region = bindings.at(0x300);
        if (mutation == 0) changed.owners.at(solid) = changed.blockRecords.at("Unused");
        if (mutation == 1) changed.blocks.at("OnlyCube").second = changed.blockRecords.at("Mixed");
        if (mutation == 2) changed.footers.at("OnlyCube").second = changed.blockRecords.at("Mixed");
        if (mutation == 3) std::swap(changed.rows.at(solid).carrier, changed.rows.at(region).carrier);
        if (mutation == 4) changed.rows.at(bindings.at(0x710)).target = "Unused";
        if (mutation == 5) changed.blockRecords.at("OnlyCube") = 0xFFFF;
        auto unchanged = bindings;
        if (!comparePhysical(changed, unchanged) && unchanged == bindings
            && changed.rows.size() == original.rows.size()) ++rejected;
    }
    return rejected; // Oracle sensitivity, never converter rejection evidence.
}

} // namespace

bool writeModelerBlockFile(const std::filesystem::path& path, bool binary) {
    auto source = makeDrawing(true, true);
    dx_iface exporter;
    return exporter.fileExport(path.string(), DRW::AC1024, binary, source.get(), false, true);
}

bool verifyModelerBlockFile(const std::filesystem::path& path) { return verifyOutput(path); }

bool writeModelerBlockMissingTargetFile(const std::filesystem::path& path, bool binary) {
    auto source = makeDrawing(false, false);
    auto left = std::next(source->blocks.begin(), 2);
    static_cast<DRW_Insert*>((*left)->ent.front())->name = "MissingCube";
    dx_iface exporter;
    // Deliberately author a dangling-reference negative through the tolerant
    // adapter route. This is not a valid-format or support-positive fixture.
    return exporter.fileExport(path.string(), DRW::AC1024, binary, source.get(), false, false);
}

bool runModelerBlockFailureTests(const std::filesystem::path& parent, bool keep) {
    const auto directory = parent / ("libdxfrw-modeler-block-failures-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    bool passed = true;
    std::size_t rejected = 0, retries = 0, compatibility = 0;
    for (bool binary : {false, true}) {
        const auto tolerant = directory / (binary ? "tolerant-binary.dxf" : "tolerant-ascii.dxf");
        // Default false remains tolerant; this deliberately dangling profile
        // calibrates legacy API behavior, not valid-file support.
        auto source = makeDrawing(false, false);
        auto left = std::next(source->blocks.begin(), 2);
        static_cast<DRW_Insert*>((*left)->ent.front())->name = "MissingCube";
        dx_iface adapter;
        PhysicalGraph graph;
        bool ok = adapter.fileExport(tolerant.string(), DRW::AC1024, binary, source.get(), false)
            && physicalGraph(tolerant, graph) && graph.rows.size() == 13;
        std::size_t dangling = 0;
        for (const auto& row : graph.rows)
            if (row.second.owner == "Left" && row.second.target == "MissingCube") ++dangling;
        ok = dangling == 1 && ok;
        if (ok) ++compatibility;
        passed = ok && passed;
        dx_data legacy;
        auto insert = std::make_unique<DRW_Insert>();
        insert->name = "MissingR12";
        legacy.mBlock->ent.push_back(insert.release());
        const auto r12 = directory / (binary ? "r12-binary.dxf" : "r12-ascii.dxf");
        dx_data imported;
        dx_iface reader;
        ok = adapter.fileExport(r12.string(), DRW::AC1009, binary, &legacy, false, true)
            && reader.fileImport(r12.string(), &imported, false)
            && imported.mBlock->ent.size() == 1;
        if (ok) {
            const auto* value = dynamic_cast<const DRW_Insert*>(imported.mBlock->ent.front());
            ok = value != nullptr && value->name == "MISSINGR12";
        }
        if (ok) ++compatibility;
        passed = ok && passed;
    }
    for (bool binary : {false, true}) for (bool existing : {false, true}) for (int fault = 0; fault < 7; ++fault) {
        auto source = makeDrawing(false, false);
        auto* solid = static_cast<DRW_ModelerGeometry*>(source->blocks.front()->ent.front());
        DRW::Version version = DRW::AC1024;
        if (fault == 0) {
            auto left = std::next(source->blocks.begin(), 2);
            static_cast<DRW_Insert*>((*left)->ent.front())->name = "MissingCube";
        }
        if (fault == 1) solid->parentHandle = 0xEEEE;
        if (fault == 2) version = DRW::AC1027; // Legacy inline SAT has no admitted modern carrier here.
        if (fault == 3) {
            solid->m_rawBytes = {0, 0xFF};
            solid->m_dxfPayloadChunks = {{310, 0, 2}};
        }
        if (fault == 4) solid->handle = 0x300; // Duplicate the mixed REGION's identity.
        if (fault == 5)
            static_cast<DRW_Insert*>(*std::next(source->mBlock->ent.begin()))->name = "MissingRoot";
        if (fault == 6) version = DRW::AC1009;
        const auto path = directory / (std::to_string(binary) + std::to_string(existing)
                                      + std::to_string(fault) + ".dxf");
        const std::string sentinel = "existing modeler block destination\n";
        if (existing) { std::ofstream output(path, std::ios::binary); output << sentinel; }
        dx_iface adapter;
        std::ostringstream diagnostic;
        bool refused = false;
        {
            struct RestoreCerr {
                std::streambuf* previous;
                ~RestoreCerr() { std::cerr.rdbuf(previous); }
            } restore{std::cerr.rdbuf(diagnostic.rdbuf())};
            refused = !adapter.fileExport(path.string(), version, binary, source.get(), false, true);
        }
        const char* expectedDiagnostic = fault == 0 || fault == 5 ? "unresolved-insert-target"
            : fault == 1 ? "unresolved-block-owner"
            : fault == 6 ? "unsupported-modeler-target"
            : fault == 2 || fault == 3 ? "unrepresentable-modeler-carrier" : "emit-failure";
        refused = refused && diagnostic.str().find(expectedDiagnostic) != std::string::npos;
        bool untouched = !std::filesystem::exists(path);
        if (existing) {
            std::ifstream input(path, std::ios::binary);
            const std::string contents((std::istreambuf_iterator<char>(input)), {});
            untouched = contents == sentinel;
        }
        if (refused && untouched) ++rejected;
        else std::cerr << "Modeler converter negative failed: fault=" << fault
                       << " binary=" << binary << " existing=" << existing
                       << " refused=" << refused << " destination-unchanged=" << untouched << '\n';
        passed = refused && untouched && passed;
        // Generic direct writers retain their caller-mutation contract. This
        // checks the same adapter after failure with explicitly fresh data,
        // not an unimplemented whole-model identity rollback guarantee.
        auto corrected = makeDrawing(false, false);
        const bool retried = adapter.fileExport(path.string(), DRW::AC1024, binary,
                                                corrected.get(), false, true) && verifyOutput(path);
        if (retried) ++retries;
        passed = retried && passed;
    }
    std::cout << "Modeler block converter failures B3: " << (passed ? "PASS" : "FAIL")
              << " (rejected=" << rejected << ", fresh-data retries=" << retries
              << ", compatibility=" << compatibility << ")\n";
    if (passed && !keep) std::filesystem::remove_all(directory);
    else std::cout << "Modeler failure evidence: " << directory << '\n';
    return passed;
}

bool runModelerBlockCodecTests(const std::filesystem::path& parent, bool keep) {
    const auto directory = parent / ("libdxfrw-modeler-block-codec-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    bool passed = true;
    std::size_t verified = 0, oracleRejections = 0;
    for (bool binary : {false, true}) for (bool blocks : {false, true}) for (bool entities : {false, true}) {
        const auto path = directory / (std::to_string(binary) + std::to_string(blocks)
                                      + std::to_string(entities) + ".dxf");
        auto source = makeDrawing(blocks, entities);
        std::map<std::uint32_t, const DRW_Entity*> original;
        for (const auto* entity : source->mBlock->ent) original.emplace(entity->handle, entity);
        for (const auto* block : source->blocks)
            for (const auto* entity : block->ent) original.emplace(entity->handle, entity);
        dx_iface exporter;
        std::map<std::uint32_t, std::uint32_t> emitted;
        bool ok = exporter.fileExport(path.string(), DRW::AC1024, binary, source.get(), false, true)
            && verifyOutput(path, &emitted);
        for (const auto& entry : original)
            ok = emitted.count(entry.first) == 1 && emitted[entry.first] == entry.second->handle && ok;
        if (ok) ++verified;
        passed = ok && passed;
        if (!blocks && !entities) oracleRejections += physicalOracleControls(path);
        dx_data imported;
        dx_iface importer;
        const auto second = directory / (path.stem().string() + "-second.dxf");
        ok = importer.fileImport(path.string(), &imported, false)
            && importer.fileExport(second.string(), DRW::AC1024, binary, &imported, false, true)
            && verifyOutput(second);
        if (ok) ++verified;
        passed = ok && passed;
    }
    passed = oracleRejections == 12 && passed;
    std::cout << "Modeler block codec handoff B2: " << (passed ? "PASS" : "FAIL")
              << " (verified=" << verified << ", oracle_rejections=" << oracleRejections << ")\n";
    if (passed && !keep) std::filesystem::remove_all(directory);
    else std::cout << "Modeler block codec evidence: " << directory << '\n';
    return passed;
}
