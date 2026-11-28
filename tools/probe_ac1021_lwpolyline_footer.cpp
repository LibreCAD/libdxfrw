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
        if (mode != "line") {
            DRW_LWPolyline p;
            auto a=p.addVertex(); a->x=0.; a->y=0.;
            auto b=p.addVertex(); b->x=5.; b->y=0.;
            auto c=p.addVertex(); c->x=5.; c->y=3.;
            if (mode=="bulge" || mode=="rich") {a->bulge=.25; b->bulge=-.5;}
            if (mode=="width" || mode=="rich") {a->stawidth=.2; a->endwidth=.4; b->stawidth=.4; b->endwidth=.1;}
            if (mode=="closed" || mode=="rich") p.flags=129;
            if (mode=="tilt" || mode=="rich") {p.elevation=2.; p.thickness=.5; p.extPoint={0.,1.,0.};}
            if (mode=="constant") p.width=.6;
            if (!writer.writeLWPolyline(&p)) return;
        }
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
    if (mode!="line" && mode!="bulge" && mode!="width" && mode!="closed" && mode!="tilt" && mode!="rich" && mode!="constant") return 2;
    dwgRW writer(argv[2]); Probe probe(writer,mode,argc == 3 || argv[3][0] == '1',argc == 3 || argv[4][0] == '1');
    const bool written=writer.write(&probe,DRW::AC1021,true);
    std::cout << mode << " written=" << written << " entity=" << probe.ok << '\n';
    return written && probe.ok ? 0 : 1;
}
