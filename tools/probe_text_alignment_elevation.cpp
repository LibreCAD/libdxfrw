#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <iostream>
#include <string>
class Probe final : public LC3CompatCheck, public DRW_DwgDefaultMaterialProvider, public DRW_DwgDefaultPlotStyleProvider {
public:
 explicit Probe(dwgRW& w,bool m):writer(w),mismatch(m){}
 bool wantsDwgDefaultMaterialGraph() const override {return true;}
 bool wantsDwgDefaultPlotStyleGraph() const override {return true;}
 bool dwgDefaultMaterialOpaqueReferencesAbsent(std::uint32_t a,std::uint32_t b,std::uint32_t c,std::uint32_t d) const override {return a&&b&&c&&d;}
 void writeEntities() override {DRW_Text t;t.basePoint={1.,2.,3.};t.secPoint={4.,5.,mismatch?7.:3.};t.height=2.5;t.text="WITNESS";t.angle=30.;t.alignH=DRW_Text::HCenter;t.alignV=DRW_Text::VMiddle;ok=writer.writeText(&t);}
 bool finalizeDwgWrite() override {return ok;}
 dwgRW& writer;bool mismatch,ok=false;
};
int main(int argc,char** argv) {if(argc!=3)return 2;dwgRW writer(argv[2]);Probe p(writer,std::string(argv[1])=="mismatch");bool w=writer.write(&p,DRW::AC1021,true);std::cout<<"write="<<w<<" text="<<p.ok<<'\n';return w&&p.ok?0:1;}
