// SPDX-License-Identifier: GPL-2.0-or-later
// Optional source-profile probe. A passing census does not authorize export.
#include "dx_iface.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <set>
#include <unordered_map>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: ac1021_mtext_field_census candidate.dwg\n";
        return 2;
    }
    dx_data data;
    dx_iface iface;
    if (!iface.fileImport(argv[1], &data, false)
        || data.sourceFormat != dx_source_format::DWG
        || data.sourceVersion != DRW::AC1021 || data.fields.size() != 16u
        || data.fieldLists.size() != 1u) {
        std::cerr << "FIELD source/profile mismatch\n";
        return 1;
    }
    std::unordered_map<std::uint32_t, const DRW_Field*> fields;
    for (const DRW_Field& field : data.fields) {
        if (!fields.emplace(field.handle, &field).second
            || !field.hasCompleteDwgPayload()
            || !field.isDwgPayloadValid(DRW::AC1021)) {
            std::cerr << "duplicate or incomplete FIELD\n";
            return 1;
        }
    }
    const auto& list = data.fieldLists.front();
    const std::set<std::uint32_t> members(list.m_fieldHandles.begin(),
                                          list.m_fieldHandles.end());
    if (list.handle != 0xABu || list.parentHandle != 0xCu
        || !list.hasCompleteDwgEntries() || list.m_fieldHandles.size() != 16u
        || members.size() != 16u) {
        std::cerr << "FIELDLIST identity or membership mismatch\n";
        return 1;
    }
    constexpr std::array<std::array<std::uint32_t, 3>, 8> roots{{
        {{0xA9u, 0xCDu, 0xAAu}}, {{0xDDu, 0xDEu, 0xDCu}},
        {{0xE4u, 0xE5u, 0xE3u}}, {{0xEBu, 0xECu, 0xEAu}},
        {{0xF7u, 0xF8u, 0xF6u}}, {{0x100u, 0x101u, 0xFFu}},
        {{0x107u, 0x108u, 0x106u}}, {{0x10Eu, 0x10Fu, 0x10Du}}
    }};
    for (const auto& pair : roots) {
        const auto parent = fields.find(pair[0]);
        const auto child = fields.find(pair[1]);
        if (parent == fields.end() || child == fields.end()
            || members.count(pair[0]) != 1u || members.count(pair[1]) != 1u) {
            std::cerr << "missing FIELD pair\n";
            return 1;
        }
        const auto& p = *parent->second;
        const auto& c = *child->second;
        if (p.parentHandle != pair[2] || c.parentHandle != p.handle
            || p.m_childHandles.size() != 1u || p.m_childHandles[0] != c.handle
            || p.m_evaluatorId != "_text" || c.m_evaluatorId != "AcVar"
            || c.m_fieldCode != "\\AcVar Filename \\f \"%fn6\""
            || c.m_valueString != "mtext_complex_inline_codes.dxf"
            || p.m_valueString.find("Filename: mtext_complex_inline_codes.dxf")
                   == std::string::npos) {
            std::cerr << "FIELD evaluator, owner, expression or cache mismatch\n";
            return 1;
        }
    }
    std::cout << "16 complete typed FIELDs, 16 FIELDLIST slots, 8 parent/child pairs\n";
    return 0;
}
