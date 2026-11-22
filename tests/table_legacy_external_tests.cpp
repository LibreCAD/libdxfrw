// SPDX-License-Identifier: GPL-2.0-or-later
// Optional, hash-selected external AC1012/AC1014 sources; no drawing is stored here.
#include "dx_data.h"
#include "dx_iface.h"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>

class Probe : public dx_iface {
public:
    std::map<std::uint32_t, DRW_Table> tables;
    std::map<std::uint32_t, DRW_UnsupportedObject> raw;
    std::map<std::uint32_t, DRW_Insert> inserts;
    void addTable(const DRW_Table& value) override {
        tables.emplace(value.handle, value);
        dx_iface::addTable(value);
    }
    void addInsert(const DRW_Insert& value) override {
        if (value.handle == 0x4F2u || value.handle == 0x528u || value.handle == 0xA35u)
            inserts.emplace(value.handle, value);
        dx_iface::addInsert(value);
    }
    void addUnsupportedObject(const DRW_UnsupportedObject& value) override {
        if (value.m_recordName == "ACAD_TABLE") raw.emplace(value.m_handle, value);
        dx_iface::addUnsupportedObject(value);
    }
};

class GridProbe final : public Probe, public DRW_DwgLegacyTableGridHandler {
public:
    bool optIn{false};
    std::map<std::uint32_t, DRW_Table> grids;
    bool wantsDwgLegacyTableGrid() const noexcept override { return optIn; }
    void addDwgLegacyTableGrid(const DRW_Table& value) override {
        grids.emplace(value.handle, value);
    }
};

int main(int argc, char** argv) {
    if (argc != 3 && argc != 4) return 2;
    struct Expected {
        std::uint32_t handle, block;
        double x, y;
        std::vector<double> widths, heights;
    };
    const std::vector<Expected> r14 = {
        {0x528u, 0x570u, 774.865723661128, 48.53878603809859,
         {34.19820493727295, 34.19820493727295, 34.19820493727295},
         {7.49103536721217, 6.3510952026364, 6.3510952026364,
          6.3510952026364, 6.41216342573868, 6.41216342573868,
          6.3510952026364}},
        {0xA35u, 0xA36u, 18.3905818828116, -140.70432957829854,
         {2.5, 2.5, 2.5, 2.5, 2.5},
         {0.45333333333333, 0.60675306957708, 0.60454297407913,
          0.60454297407913}}
    };
    const std::vector<Expected> r13 = {
        {0x4F2u, 0x4F3u, 3298.7947769117454, 839.6133480853896,
         std::vector<double>(9u, 63.5),
         {11.0, 9.0, 9.0, 9.0, 9.0, 9.0, 9.0, 9.0, 9.0, 9.0,
          9.0, 9.0, 9.0, 9.0, 9.0, 9.0, 9.0, 9.0, 9.0, 9.0}}
    };
    const std::string profile = argv[2];
    if (profile == "r14-attrs") {
        if (argc != 4) return 2;
        // Controlled derivative: the record parser can inspect the endpoint
        // handles, but the file reader must not publish an incomplete owner.
        for (const unsigned mode : {0u, 1u, 2u}) {
            Probe plain;
            GridProbe capable; capable.optIn = mode == 2u;
            Probe& reader = mode == 0u ? plain : static_cast<Probe&>(capable);
            dx_data data;
            (void)reader.fileImport(argv[1], &data, false);
            if (reader.tables.count(0x528u) || reader.inserts.count(0x528u)
                || reader.raw.count(0x528u) || capable.grids.count(0x528u)) {
                std::cerr << "uncommitted TABLE attribute compound was published\n";
                return 1;
            }
            if (reader.tables.count(0xA35u) != 1u) {
                std::cerr << "independent TABLE after rejected compound was not delivered\n";
                return 1;
            }
            reader.tables.clear(); reader.inserts.clear(); reader.raw.clear();
            capable.grids.clear();
            dx_data retry;
            if (!reader.fileImport(argv[3], &retry, false)
                || reader.tables.count(0x528u) != 1u
                || reader.inserts.count(0x528u) != 1u
                || reader.raw.count(0x528u) != 1u
                || capable.grids.count(0x528u) != (mode == 2u ? 1u : 0u)) {
                std::cerr << "TABLE callback did not recover on same-reader retry\n";
                return 1;
            }
        }
        std::cout << "R14 TABLE attribute compound remains fail-closed: PASS\n";
        return 0;
    }
    if (argc != 3) return 2;
    if (profile != "r13" && profile != "r14") return 2;
    const bool isR13 = profile == "r13";
    const auto& expected = isR13 ? r13 : r14;
    const auto version = isR13 ? DRW::AC1012 : DRW::AC1014;
    const int objectType = isR13 ? 528 : 523;
    const std::uint32_t style = isR13 ? 0x29Bu : 0x87u;
    for (const unsigned mode : {0u, 1u, 2u}) {
        Probe plain;
        GridProbe capable; capable.optIn = mode == 2u;
        Probe& reader = mode == 0u ? plain : static_cast<Probe&>(capable);
        const bool optIn = mode == 2u;
        dx_data data;
        if (!reader.fileImport(argv[1], &data, false)
            || reader.tables.size() != expected.size() || reader.raw.size() != expected.size()
            || reader.inserts.size() != expected.size()
            || capable.grids.size() != (optIn ? expected.size() : 0u)) return 1;
        for (const auto& row : expected) {
            const auto table = reader.tables.find(row.handle);
            const auto raw = reader.raw.find(row.handle);
            const auto insert = reader.inserts.find(row.handle);
            if (table == reader.tables.end() || raw == reader.raw.end()
                || insert == reader.inserts.end()) return 1;
            const auto& value = table->second;
            const auto& source = raw->second;
            const auto& projection = insert->second;
            if (value.blockRecH.ref != row.block
                || std::abs(value.basePoint.x - row.x) > 1e-10
                || std::abs(value.basePoint.y - row.y) > 1e-10
                || value.basePoint.z != 0.0 || value.xscale != 1.0
                || value.yscale != 1.0 || value.zscale != 1.0
                || value.angle != 0.0 || value.extPoint.x != 0.0
                || value.extPoint.y != 0.0 || value.extPoint.z != 1.0
                || value.colcount != 1 || value.rowcount != 1
                || !value.m_hasSemanticContent || value.m_semanticContentComplete
                || !value.m_content.m_columns.empty() || !value.m_content.m_rows.empty()
                || projection.blockRecH.ref != row.block
                || projection.colcount != 1 || projection.rowcount != 1
                || projection.basePoint.x != value.basePoint.x
                || projection.basePoint.y != value.basePoint.y
                || source.m_version != version || source.m_objectType != objectType
                || !source.m_isEntity || source.m_typedPayloadValidated
                || source.m_rawBytes.empty()) {
                std::cerr << profile << " TABLE 2D projection or raw peer mismatch for "
                          << std::hex << row.handle << '\n';
                return 1;
            }
            if (!optIn) continue;
            const auto selected = capable.grids.find(row.handle);
            if (selected == capable.grids.end()) return 1;
            const auto& grid = selected->second;
            if (!grid.m_hasSemanticContent || grid.m_semanticContentComplete
                || grid.colcount != 1 || grid.rowcount != 1
                || grid.m_valueFlag != 22 || grid.m_tableStyleHandle != style
                || grid.m_horizontalDirection.x != 1.0
                || grid.m_horizontalDirection.y != 0.0
                || grid.m_horizontalDirection.z != 0.0
                || grid.m_content.m_columns.size() != row.widths.size()
                || grid.m_content.m_rows.size() != row.heights.size()) return 1;
            for (std::size_t i = 0; i < row.widths.size(); ++i)
                if (std::abs(grid.m_content.m_columns[i].m_width - row.widths[i]) > 1e-10)
                    return 1;
            for (std::size_t i = 0; i < row.heights.size(); ++i)
                if (std::abs(grid.m_content.m_rows[i].m_height - row.heights[i]) > 1e-10
                    || grid.m_content.m_rows[i].m_cells.size() != row.widths.size())
                    return 1;
        }
    }
    std::cout << profile << " TABLE 2D projection, opt-in grid and raw peers: PASS\n";
    return 0;
}
