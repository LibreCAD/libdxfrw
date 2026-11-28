// Scratch-only AC1021 geometry matrix. No DWG fixtures are stored in the repo.
#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <filesystem>
#include <iostream>
#include <string>

class Probe final : public LC3CompatCheck, public DRW_DwgDefaultMaterialProvider,
                    public DRW_DwgDefaultPlotStyleProvider {
public:
    Probe(dwgRW& output, const std::string& selected, bool material, bool plot) : writer(output), mode(selected), useMaterial(material), usePlot(plot) {}
    bool wantsDwgDefaultMaterialGraph() const override { return useMaterial; }
    bool wantsDwgDefaultPlotStyleGraph() const override { return usePlot; }
    bool dwgDefaultMaterialOpaqueReferencesAbsent(std::uint32_t a, std::uint32_t b,
        std::uint32_t c, std::uint32_t d) const override { return a && b && c && d; }
    void writeEntities() override {
        ok = false;
        if (mode == "line" || mode == "mixed") {
            DRW_Line line; line.basePoint={1.,2.,3.}; line.secPoint={4.,5.,6.};
            if (!writer.writeLine(&line)) return;
        }
        {
            DRW_Hatch hatch; hatch.name="SOLID"; hatch.solid=1; hatch.associative=0;
            auto loop=std::make_shared<DRW_HatchLoop>(1);
            if(mode=="ellipse") { auto e=std::make_shared<DRW_Ray>(); e->basePoint={3.,4.,0.}; e->secPoint={4.,0.,0.}; loop->objlist.push_back(e); }
            else { auto e=std::make_shared<DRW_Helix>(); e->degree=2; e->knotslist={0.,0.,0.,1.,1.,1.}; e->controllist={std::make_shared<DRW_Coord>(0.,0.,0.),std::make_shared<DRW_Coord>(1.,2.,0.),std::make_shared<DRW_Coord>(3.,0.,0.)}; e->turns=4.; e->radius=2.; loop->objlist.push_back(e); }
            loop->update(); hatch.appendLoop(loop);
            if (!writer.writeHatch(&hatch)) return;
        }
        DRW_Line next; next.basePoint={10.,11.,0.}; next.secPoint={12.,13.,0.}; if(!writer.writeLine(&next))return;
        ok = true;
    }
    bool finalizeDwgWrite() override { return ok; }
    dwgRW& writer; std::string mode; bool useMaterial, usePlot; bool ok = false;
};
int main(int argc, char** argv) {
    if (argc != 3 && argc != 5) return 2;
    if (std::filesystem::exists(argv[2])) return 2;
    if (argc == 5 && ((std::string(argv[3]) != "0" && std::string(argv[3]) != "1") || (std::string(argv[4]) != "0" && std::string(argv[4]) != "1"))) return 2;
    const std::string mode=argv[1];
    if (mode!="ellipse" && mode!="arc" && mode!="linear") return 2;
    dwgRW writer(argv[2]); Probe probe(writer,mode,argc == 3 || argv[3][0] == '1',argc == 3 || argv[4][0] == '1');
    const bool written=writer.write(&probe,DRW::AC1021,true);
    std::cout << mode << " written=" << written << " entity=" << probe.ok << '\n';
    return written && probe.ok ? 0 : 1;
}
