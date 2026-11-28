// SPDX-License-Identifier: GPL-2.0-or-later
// Scratch-only selected CIRCLE/ARC replay. This is not a whole-drawing copy.
#include "../tests/lc3_compat_check.cpp"
#include "drw_dxf_extrusion.h"
#include "libdwgr.h"
#include <iostream>
#include <vector>

class Probe final : public LC3CompatCheck, public DRW_DwgDefaultMaterialProvider,
                    public DRW_DwgDefaultPlotStyleProvider {
public:
    explicit Probe(dwgRW& output) : writer(output) {}
    void addCircle(const DRW_Circle& value) override { circles.push_back(value); }
    void addArc(const DRW_Arc& value) override { arcs.push_back(value); }
    bool wantsDwgDefaultMaterialGraph() const override { return true; }
    bool wantsDwgDefaultPlotStyleGraph() const override { return true; }
    bool dwgDefaultMaterialOpaqueReferencesAbsent(std::uint32_t a, std::uint32_t b,
            std::uint32_t c, std::uint32_t d) const override { return a && b && c && d; }
    void dwgDefaultMaterialGraphPlanned(std::uint32_t, std::uint32_t,
                                        std::uint32_t, std::uint32_t) override {}
    void writeHeader(DRW_Header& header) override {
        for (const char* key : {"$CMATERIAL", "CMATERIAL"}) {
            const auto found = header.vars.find(key);
            if (found != header.vars.end()) { delete found->second; header.vars.erase(found); }
        }
    }
    void writeEntities() override {
        accepted = false;
        for (const auto& value : circles) {
            DRW_Circle converted;
            if (!DRW_DxfLegacyExtrusion::toNativeOcs(value, converted)) return;
            converted.handle = 0;
            if (!writer.writeCircle(&converted)) return;
        }
        for (const auto& value : arcs) {
            DRW_Arc converted;
            if (!DRW_DxfLegacyExtrusion::toNativeOcs(value, converted)) return;
            converted.handle = 0;
            if (!writer.writeArc(&converted)) return;
        }
        accepted = true;
    }
    bool finalizeDwgWrite() override { return accepted; }
    dwgRW& writer;
    std::vector<DRW_Circle> circles;
    std::vector<DRW_Arc> arcs;
    bool accepted = false;
};

int main(int argc, char** argv) {
    if (argc != 3) return 2;
    dwgRW writer(argv[2]);
    Probe probe(writer);
    dwgR reader(argv[1]);
    if (!reader.read(&probe, true) || probe.circles.size() != 2u || probe.arcs.size() != 2u
        || !writer.write(&probe, DRW::AC1024, true) || !probe.accepted) {
        std::cerr << "selected CIRCLE/ARC replay failed\n";
        return 1;
    }
    std::cout << "selected AC1024 CIRCLE/ARC replay PASS\n";
}
