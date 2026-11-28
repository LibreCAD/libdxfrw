// SPDX-License-Identifier: GPL-2.0-or-later
// Witness-specific object-bit probe; page extraction remains in-tree.
#include "dwg2dxf/dx_data.h"
#include "dwg2dxf/dx_iface.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>

class Probe final : public dx_iface {
public:
    void addWipeoutVariables(const DRW_WipeoutVariables& value) override {
        ++typed;
        typedMatch = value.handle == 0x30u && value.parentHandle == 0xAu
            && value.m_displayFrame == 0u && value.reactorCount() == 1
            && value.reactorHandles == std::vector<std::uint32_t>{0xAu}
            && value.extensionDictionaryFlag() == 1u;
        dx_iface::addWipeoutVariables(value);
    }
    void addUnsupportedObject(const DRW_UnsupportedObject& value) override {
        if (value.m_recordName == "WIPEOUTVARIABLES") {
            ++raw;
            const auto& bytes = value.m_rawBytes;
            const auto bits = [&bytes](unsigned first, unsigned end) {
                std::uint32_t result = 0;
                for (unsigned at = first; at != end; ++at)
                    result = (result << 1u) | ((bytes[at / 8u] >> (7u - at % 8u)) & 1u);
                return result;
            };
            // AC1032 has a R2013+ data-storage bit. The frame is fixed to
            // this hashed witness; no unknown alternative encoding is accepted.
            rawMatch = value.m_version == DRW::AC1032 && value.m_handle == 0x30u
                && value.m_objectType == 514 && bytes.size() == 10u
                && value.m_bodyBitSize == 37u;
            if (rawMatch)
                rawMatch = bits(0, 10) == 0x112u && bits(10, 26) == 0x130u
                    && bits(26, 28) == 2u && bits(28, 38) == 0x101u
                    && bits(38, 39) == 1u && bits(39, 40) == 0u
                    && bits(40, 42) == 2u && bits(42, 43) == 0u
                    && bits(43, 59) == 0x410Au && bits(59, 75) == 0x410Au
                    && bits(75, 80) == 0u;
        }
        dx_iface::addUnsupportedObject(value);
    }
    unsigned typed = 0, raw = 0;
    bool typedMatch = false, rawMatch = false;
};

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    dx_data data;
    Probe reader;
    const bool imported = reader.fileImport(argv[1], &data, false);
    const auto& entries = data.namedObjectDictionary.m_entries;
    const bool root = data.hasNamedObjectDictionary && data.namedObjectDictionary.handle == 0xAu
        && std::count_if(entries.begin(), entries.end(), [](const DRW_Dictionary::Entry& e) {
            return e.m_name == "ACAD_WIPEOUT_VARS" && e.m_handle == 0x30u;
        }) == 1;
    const bool good = imported && data.sourceVersion == DRW::AC1032
        && reader.typed == 1u && reader.raw == 1u && reader.typedMatch && reader.rawMatch && root;
    std::cout << "AC1032 WIPEOUTVARIABLES source proof: " << (good ? "PASS" : "FAIL") << '\n';
    return good ? 0 : 1;
}
