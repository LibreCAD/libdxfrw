#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <filesystem>
#include <iostream>
#include <string>
class Probe final : public LC3CompatCheck, public DRW_DwgDefaultMaterialProvider,
                    public DRW_DwgDefaultPlotStyleProvider {
public:
 Probe(dwgRW& out,std::string mode):w(out),mode_(std::move(mode)){}
 bool wantsDwgDefaultMaterialGraph() const override { return true; }
 bool wantsDwgDefaultPlotStyleGraph() const override { return true; }
 bool dwgDefaultMaterialOpaqueReferencesAbsent(std::uint32_t a,std::uint32_t b,std::uint32_t c,std::uint32_t d) const override { return a&&b&&c&&d; }
 bool entities(bool paper) {
  DRW_Line first; first.space=paper?DRW::PaperSpace:DRW::ModelSpace; first.basePoint={-3.,-1.,0.}; first.secPoint={-2.,-1.,0.};
  if(!w.writeLine(&first)) return false;
  DRW_Polyline p; p.space=first.space; const bool three=mode_.find("3d")!=std::string::npos;
  p.flags=three?8:0; p.addVertex(DRW_Vertex(0.,0.,0.,0.)); p.addVertex(DRW_Vertex(5.,0.,three?1.:0.,0.)); p.addVertex(DRW_Vertex(5.,3.,three?2.:0.,0.));
  if(three) for(auto &v:p.vertlist) v->flags=32;
  if(!w.writePolyline(&p)) return false;
  DRW_Line last; last.space=first.space; last.basePoint={7.,4.,0.}; last.secPoint={8.,4.,0.}; return w.writeLine(&last);
 }
 void writeBlocks() override {
  if(mode_.find("named")!=0) { blocksOk=true; return; }
  auto h=w.defineBlock("OWNER_TEST",DRW_Coord(0.,0.,0.));
  blocksOk=h && w.beginBlockContent(h) && entities(false) && w.endBlockContent();
 }
 void writeEntities() override {
  if(mode_.find("paper")==0) { DRW_Line line; line.basePoint={10.,20.,0.}; line.secPoint={11.,21.,0.}; entitiesOk=w.writeLine(&line)&&entities(true); }
  else { DRW_Insert ins; ins.name="OWNER_TEST"; ins.basePoint={10.,20.,0.}; entitiesOk=w.writeInsert(&ins); }
 }
 bool finalizeDwgWrite() override {return blocksOk&&entitiesOk;}
 dwgRW& w; std::string mode_; bool blocksOk=false,entitiesOk=false;
};
int main(int argc,char**argv){if(argc!=3||std::filesystem::exists(argv[2]))return 2;std::string mode=argv[1];if(mode!="paper2d"&&mode!="paper3d"&&mode!="named2d"&&mode!="named3d")return 2;dwgRW w(argv[2]);Probe p(w,mode);bool ok=w.write(&p,DRW::AC1021,true);std::cout<<mode<<" ok="<<ok<<" blocks="<<p.blocksOk<<" entities="<<p.entitiesOk<<'\n';return ok?0:1;}
