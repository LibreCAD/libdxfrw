// Scratch-only physical import observation; this does not seal provenance or
// authorize output. Build against a matching adapter/library checkout.
#include "dx_iface.h"
#include "intern/dwg_table_roundtrip_xrecord_ac1018_candidate.h"
#include <iostream>
#include <memory>

class Probe final : public dx_iface {
public:
    void addXRecord(const DRW_XRecord& value) override {
        dx_iface::addXRecord(value);
        if (value.handle != 0xbcu) return;
        if (typedSeen) { duplicate = true; return; }
        typed = value;
        typedSeen = true;
    }
    void addUnsupportedObject(const DRW_UnsupportedObject& value) override {
        dx_iface::addUnsupportedObject(value);
        if (value.m_handle != 0xbcu) return;
        if (rawSeen) { duplicate = true; return; }
        raw = value;
        rawSeen = true;
    }
    DRW_XRecord typed;
    DRW_UnsupportedObject raw;
    bool typedSeen{false}, rawSeen{false}, duplicate{false};
};
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    auto probe = std::make_unique<Probe>();
    auto data = std::make_unique<dx_data>();
    const bool read = probe->fileImport(argv[1], data.get(), false);
    DwgTableRoundtripXRecordAc1018Candidate::Result result;
    const bool matched = read && !probe->duplicate && probe->typedSeen && probe->rawSeen
        && DwgTableRoundtripXRecordAc1018Candidate::inspect(probe->raw, probe->typed, result);
    std::cout << "read=" << read << " raw=" << probe->rawSeen << " typed=" << probe->typedSeen
              << " matched=" << matched << " refs=" << std::hex << result.references[0].target
              << ',' << result.references[1].target << ',' << result.references[2].target
              << std::dec << '\n';
    return matched ? 0 : 1;
}
