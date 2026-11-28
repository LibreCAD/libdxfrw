// SPDX-License-Identifier: GPL-2.0-or-later
// Scratch-only AC1024 material/plotstyle opt-in geometry probe.
#include "../tests/lc3_compat_check.cpp"
#include "libdwgr.h"
#include <iostream>

struct Probe : LC3CompatCheck, DRW_DwgDefaultMaterialProvider, DRW_DwgDefaultPlotStyleProvider {
    explicit Probe(dwgRW& value) : writer(value) {}
    bool wantsDwgDefaultMaterialGraph() const override { return true; }
    bool wantsDwgDefaultPlotStyleGraph() const override { return true; }
    bool dwgDefaultMaterialOpaqueReferencesAbsent(std::uint32_t a,std::uint32_t b,
            std::uint32_t c,std::uint32_t d) const override { return a && b && c && d; }
    void dwgDefaultMaterialGraphPlanned(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) override {}
    void writeHeader(DRW_Header& header) override {
        for(const auto* key : {"$CMATERIAL", "CMATERIAL"}) {
            const auto found=header.vars.find(key);
            if(found!=header.vars.end()) { delete found->second;header.vars.erase(found); }
        }
    }
    void writeEntities() override {
        DRW_Line line;line.basePoint={1.,2.,3.};line.secPoint={4.,5.,6.};
        DRW_Circle circle;circle.basePoint={7.,8.,2.};circle.radious=2.5;circle.extPoint={0.,1.,0.};
        DRW_Arc arc;arc.basePoint={10.,11.,3.};arc.radious=4.;arc.staangle=.25;arc.endangle=1.25;arc.extPoint={0.,1.,0.};
        accepted=writer.writeLine(&line)&&writer.writeCircle(&circle)&&writer.writeArc(&arc);
    }
    dwgRW& writer;bool accepted=false;
};

int main(int argc,char** argv) {
    if(argc!=2){std::cerr<<"usage: probe_ac1024_tilted_geometry <output.dwg>\n";return 2;}
    dwgRW writer(argv[1]);Probe probe(writer);
    if(!writer.write(&probe,DRW::AC1024,true)||!probe.accepted){std::cerr<<"write failed\n";return 1;}
    return 0;
}
