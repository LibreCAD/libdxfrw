// Scratch-only AC1027 header geometry matrix. No DWG fixtures are stored in the repo.
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
        if (mode == "point") {
            DRW_Point point; point.basePoint={3.,4.,5.};
            if (!writer.writePoint(&point)) return;
        }
        if (mode == "circle" || mode == "circle_tilt" || mode == "circle_negative" || mode == "mixed") {
            DRW_Circle circle; circle.basePoint={7.,8.,2.}; circle.radious=2.5;
            if (mode == "circle_tilt") {circle.extPoint={0.,1.,0.}; circle.thickness=1.25;}
            if (mode == "circle_negative") {circle.extPoint={0.,0.,-1.}; circle.thickness=1.25;}
            if (!writer.writeCircle(&circle)) return;
        }
        if (mode == "arc" || mode == "arc_tilt" || mode == "arc_negative" || mode == "mixed") {
            DRW_Arc arc; arc.basePoint={10.,11.,3.}; arc.radious=4.;
            arc.staangle=0.3490658503988659; arc.endangle=2.007128639793479;
            if (mode == "arc_tilt") {arc.extPoint={0.,1.,0.}; arc.thickness=0.5;}
            if (mode == "arc_negative") {arc.extPoint={0.,0.,-1.}; arc.thickness=0.5;}
            if (!writer.writeArc(&arc)) return;
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
    if (mode!="line" && mode!="point" && mode!="circle" && mode!="circle_tilt"
        && mode!="circle_negative" && mode!="arc" && mode!="arc_tilt"
        && mode!="arc_negative" && mode!="mixed") return 2;
    dwgRW writer(argv[2]); Probe probe(writer,mode,argc == 3 || argv[3][0] == '1',argc == 3 || argv[4][0] == '1');
    const bool written=writer.write(&probe,DRW::AC1027,true);
    std::cout << mode << " written=" << written << " entity=" << probe.ok << '\n';
    return written && probe.ok ? 0 : 1;
}
