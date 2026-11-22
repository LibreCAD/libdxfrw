// SPDX-License-Identifier: GPL-2.0-or-later
// Optional, hash-selected external AC1014 source; no drawing is stored here.
#include "dx_data.h"
#include "dx_iface.h"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>

class Probe final : public dx_iface {
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

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    Probe reader;
    dx_data data;
    if (!reader.fileImport(argv[1], &data, false)) return 1;
    struct Expected { std::uint32_t handle, block; double x, y; };
    const Expected expected[] = {
        {0x528u, 0x570u, 774.865723661128, 48.53878603809859},
        {0xA35u, 0xA36u, 18.3905818828116, -140.70432957829854}
    };
    if (reader.tables.size() != 2u || reader.raw.size() != 2u
        || reader.inserts.size() != 2u) return 1;
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
            || !value.m_hasSemanticContent || value.m_semanticContentComplete
            || projection.blockRecH.ref != row.block
            || projection.basePoint.x != value.basePoint.x
            || projection.basePoint.y != value.basePoint.y
            || source.m_version != DRW::AC1014 || source.m_objectType != 523
            || !source.m_isEntity || source.m_typedPayloadValidated
            || source.m_rawBytes.empty()) {
            std::cerr << "AC1014 TABLE source or 2D projection mismatch for "
                      << std::hex << row.handle << '\n';
            return 1;
        }
    }
    std::cout << "AC1014 TABLE prefixes, 2D projections and raw peers: PASS\n";
    return 0;
}
