// Scratch-only cross-version POLYLINE probe. No DWG fixtures are stored in the repo.
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
        if (mode == "spline" || mode == "mixed") {
            DRW_Spline s; s.m_scenario=1; s.degree=2; s.ncontrol=3;
            s.knotslist={0.,0.,0.,1.,1.,1.}; s.normalVec={0.,0.,1.};
            s.controllist.push_back(std::make_shared<DRW_Coord>(0.,0.,0.));
            s.controllist.push_back(std::make_shared<DRW_Coord>(2.,4.,0.));
            s.controllist.push_back(std::make_shared<DRW_Coord>(5.,0.,0.));
            if (!writer.writeSpline(&s)) return;
        }
        if (mode == "hatch" || mode == "mixed") {
            DRW_Hatch h; h.name="SOLID"; h.solid=1; h.associative=0;
            auto loop=std::make_shared<DRW_HatchLoop>(2);
            auto p=std::make_shared<DRW_LWPolyline>(); p->flags=1;
            p->addVertex(DRW_Vertex2D(0.,0.,0.)); p->addVertex(DRW_Vertex2D(5.,0.,0.));
            p->addVertex(DRW_Vertex2D(5.,3.,0.)); p->addVertex(DRW_Vertex2D(0.,3.,0.));
            loop->objlist.push_back(p); loop->update(); h.appendLoop(loop);
            if (!writer.writeHatch(&h)) return;
        }
        if (mode == "polyline2d" || mode == "polyline3d" || mode == "mixed") {
            DRW_Polyline p; p.vertexcount=3; const bool is3d=mode=="polyline3d";
            if (is3d) p.flags=8;
            p.addVertex(DRW_Vertex(0.,0.,0.,0.)); p.addVertex(DRW_Vertex(5.,0.,is3d?1.:0.,0.));
            p.addVertex(DRW_Vertex(5.,3.,is3d?2.:0.,0.));
            if (is3d) for (auto& v:p.vertlist) v->flags=32;
            if (!writer.writePolyline(&p)) return;
        }
        ok = true;
    }
    bool finalizeDwgWrite() override { return ok; }
    dwgRW& writer; std::string mode; bool useMaterial, usePlot; bool ok = false;
};
int main(int argc,char**argv) {
    if(argc!=4 || std::filesystem::exists(argv[3]))return 2;
    const std::string mode=argv[1], revision=argv[2];
    if(mode!="polyline2d" && mode!="polyline3d") return 2;
    DRW::Version v=DRW::UNKNOWNV;
    if(revision=="AC1018")v=DRW::AC1018;
    if(revision=="AC1024")v=DRW::AC1024;
    if(revision=="AC1027")v=DRW::AC1027;
    if(revision=="AC1032")v=DRW::AC1032;
    if(v==DRW::UNKNOWNV)return 2;
    dwgRW writer(argv[3]); Probe probe(writer,mode,v==DRW::AC1024,true);
    bool written=writer.write(&probe,v,true);
    std::cout<<revision<<" "<<mode<<" written="<<written<<" entity="<<probe.ok<<"\n";
    return written&&probe.ok?0:1;
}
