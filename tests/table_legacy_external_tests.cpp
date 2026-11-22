// SPDX-License-Identifier: GPL-2.0-or-later
// Optional, hash-selected external AC1014 source; no drawing is stored here.
#include "dx_data.h"
#include "dx_iface.h"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
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
        if (value.handle == 0x528u || value.handle == 0xA35u)
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
    if (argc != 2) return 2;
    struct Expected {
        std::uint32_t handle, block;
        double x, y;
        std::vector<double> widths, heights;
    };
    const Expected expected[] = {
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
    for (const unsigned mode : {0u, 1u, 2u}) {
        Probe plain;
        GridProbe capable; capable.optIn = mode == 2u;
        Probe& reader = mode == 0u ? plain : static_cast<Probe&>(capable);
        const bool optIn = mode == 2u;
        dx_data data;
        if (!reader.fileImport(argv[1], &data, false)
            || reader.tables.size() != 2u || reader.raw.size() != 2u
            || reader.inserts.size() != 2u
            || capable.grids.size() != (optIn ? 2u : 0u)) return 1;
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
                || source.m_version != DRW::AC1014 || source.m_objectType != 523
                || !source.m_isEntity || source.m_typedPayloadValidated
                || source.m_rawBytes.empty()) {
                std::cerr << "AC1014 TABLE 2D projection or raw peer mismatch for "
                          << std::hex << row.handle << '\n';
                return 1;
            }
            if (!optIn) continue;
            const auto selected = capable.grids.find(row.handle);
            if (selected == capable.grids.end()) return 1;
            const auto& grid = selected->second;
            if (!grid.m_hasSemanticContent || grid.m_semanticContentComplete
                || grid.colcount != 1 || grid.rowcount != 1
                || grid.m_valueFlag != 22 || grid.m_tableStyleHandle != 0x87u
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
    std::cout << "AC1014 TABLE 2D projection, opt-in grid and raw peers: PASS\n";
    return 0;
}
