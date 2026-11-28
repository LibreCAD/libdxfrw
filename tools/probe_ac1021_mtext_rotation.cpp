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
        if (mode == "ellipse" || mode == "mixed") {
            DRW_Ellipse e; e.basePoint={2.,3.,4.}; e.secPoint={4.,0.,0.};
            e.ratio=.5; e.staparam=.2; e.endparam=2.4;
            if (!writer.writeEllipse(&e)) return;
        }
        if (mode == "lwpolyline" || mode == "mixed") {
            DRW_LWPolyline p; p.elevation=2.;
            p.addVertex(DRW_Vertex2D(0.,0.,0.)); p.addVertex(DRW_Vertex2D(5.,0.,0.));
            p.addVertex(DRW_Vertex2D(5.,3.,0.));
            if (!writer.writeLWPolyline(&p)) return;
        }
        if (mode == "text" || mode == "mixed") {
            DRW_MText t;
#if MTEXT_NORMAL == 1
            t.extPoint={0.,1.,0.};
#elif MTEXT_NORMAL == 2
            t.extPoint={0.,0.,-1.};
#else
            t.extPoint={0.,0.,1.};
#endif
            t.basePoint={3.,4.,2.}; t.height=2.; t.widthscale=10.; t.text="HELLO"; t.angle=30.; t.alignH=static_cast<DRW_Text::HAlign>(1);
            if (!writer.writeMText(&t)) return;
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
    if (mode!="line" && mode!="ellipse" && mode!="lwpolyline" && mode!="text" && mode!="mixed") return 2;
    dwgRW writer(argv[2]); Probe probe(writer,mode,argc == 3 || argv[3][0] == '1',argc == 3 || argv[4][0] == '1');
    const bool written=writer.write(&probe,DRW::AC1021,true);
    std::cout << mode << " written=" << written << " entity=" << probe.ok << '\n';
    return written && probe.ok ? 0 : 1;
}
