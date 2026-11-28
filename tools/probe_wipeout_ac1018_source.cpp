// SPDX-License-Identifier: GPL-2.0-or-later
// Witness-specific AC1018 callback and source-bit probe. Page decompression,
// object framing and CRC validation are performed by the in-tree reader.
#include "dwg2dxf/dx_data.h"
#include "dwg2dxf/dx_iface.h"
#include "dwg2dxf/dx_raster_dwg_wire.h"
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
            rawMatch = value.m_version == DRW::AC1018 && value.m_handle == 0x30u
                && value.m_objectType == 513 && bytes.size() == 15u
                && value.m_bodyBitSize == 0u && value.m_isCustomClass
                && value.m_commonLinkEvidence == DRW_DwgCommonLinkEvidence::Unknown;
            if (rawMatch) {
                using namespace dx_raster_wire::detail;
                Bits body{bytes.data(), 0u, 120u};
                rawMatch = body.shortValue() == 513u && body.little(4u) == 81u
                    && handle(body, 0u, 0u, true).value == 0x30u
                    && body.shortValue() == 0u && body.longValue() == 1u
                    && body.take(1u) == 1u && body.shortValue() == 0u
                    && body.good && body.pos == 81u;
                const auto owner = handle(body, 4u, 0x30u);
                rawMatch = rawMatch && body.good && owner.role == 4u
                    && owner.value == 0xAu && body.pos == 97u;
                const auto reactor = handle(body, 4u, 0x30u);
                rawMatch = rawMatch && body.good && reactor.role == 4u
                    && reactor.value == 0xAu && body.pos == 113u
                    && body.take(7u) == 0u && body.good && body.pos == 120u;
            }
        }
        dx_iface::addUnsupportedObject(value);
    }
    unsigned typed = 0u, raw = 0u;
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
    const auto& classes = data.dwgClassCoverage.m_entries;
    const auto classCount = std::count_if(classes.begin(), classes.end(),
        [](const DRW_DwgClassCoverageEntry& e) {
            return e.m_classNumber == 513 && e.m_className == "AcDbWipeoutVariables"
                && e.m_recordName == "WIPEOUTVARIABLES" && e.m_instanceCount == 1;
        });
    const bool good = imported && data.sourceVersion == DRW::AC1018
        && data.dwgClassCoverage.m_complete && data.hasUnchangedDwgClassEvidence()
        && classCount == 1 && reader.typed == 1u && reader.raw == 1u
        && reader.typedMatch && reader.rawMatch && root;
    std::cout << "AC1018 WIPEOUTVARIABLES source proof: " << (good ? "PASS" : "FAIL") << '\n';
    return good ? 0 : 1;
}
