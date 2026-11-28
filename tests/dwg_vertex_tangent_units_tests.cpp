#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgwriter21.h"
#include "drw_header.h"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <iostream>
#include <string>

struct VertexProbe : DRW_Vertex { using DRW_Vertex::encodeDwg; using DRW_Vertex::parseDwg; void type2D(){oType=0x0A;} bool common(dwgBuffer& b){return DRW_Entity::parseDwgCommon(DRW::AC1021,&b,0u);} };
class DwgBlockRecordXrefWireTestAccess { public:
 static bool frame(dwgWriter15& w, std::uint32_t h, std::vector<std::uint8_t>& b) {auto i=std::find_if(w.m_objectMap.begin(),w.m_objectMap.end(),[h](const auto& x){return x.first==h;});if(i==w.m_objectMap.end())return false;const auto& data=w.buffer();dwgBuffer f(data.data()+i->second,data.size()-i->second);const auto n=f.getModularShort();const auto start=f.getPosition();if(!f.isGood()||n<=0||start+static_cast<std::size_t>(n)+2>data.size()-i->second)return false;b.assign(data.begin()+i->second+start,data.begin()+i->second+start+n);return true;}
};
struct Adapter : LC3CompatCheck, DRW_DwgDefaultMaterialProvider, DRW_DwgDefaultPlotStyleProvider {
 dwgRW* out=nullptr; bool ok=false; bool bad=false; int count=0; double angles[3]={}; int flags[3]={};
 bool wantsDwgDefaultMaterialGraph() const override { return false; }
 bool wantsDwgDefaultPlotStyleGraph() const override { return false; }
 bool dwgDefaultMaterialOpaqueReferencesAbsent(std::uint32_t a,std::uint32_t b,std::uint32_t c,std::uint32_t d) const override {return a&&b&&c&&d;}
 void writeEntities() override { DRW_Polyline p; for(double x:{0.,5.,8.}) p.addVertex(DRW_Vertex(x,2.,0.,0.)); p.vertlist[0]->flags=2; p.vertlist[0]->tgdir=bad?30.*std::numeric_limits<double>::denorm_min():45.; p.vertlist[1]->flags=2; p.vertlist[1]->tgdir=-60.; p.vertlist[2]->flags=0; p.vertlist[2]->tgdir=20.; ok=out->writePolyline(&p); }
 bool finalizeDwgWrite() override {return ok;}
 void addPolyline(const DRW_Polyline& p) override {if(p.vertlist.size()!=3) return; for(int i=0;i<3;++i){angles[i]=p.vertlist[i]->tgdir;flags[i]=p.vertlist[i]->flags;} ++count;}
};
int main() {
 { DRW_Header h;dwgWriter21 w(nullptr,&h);if(!w.prepareDwgClassManifest()||!w.writeFileHeaderStub()||!w.writeDwgHeader()||!w.writeDwgClasses()||!w.writeDwgObjects())return 11;DRW_Vertex v;v.flags=2;v.tgdir=45.;if(!w.encodeEntity(&v))return 12;std::vector<std::uint8_t> bytes;if(!DwgBlockRecordXrefWireTestAccess::frame(w,v.handle,bytes))return 13;VertexProbe p;dwgBuffer scalar(bytes.data(),bytes.size());if(!p.common(scalar))return 17; if(scalar.getRawChar8()!=2)return 18;(void)scalar.get3BitDouble();(void)scalar.getBitDouble();(void)scalar.getBitDouble();(void)scalar.getBitDouble();if(std::fabs(scalar.getBitDouble()-std::acos(-1.)/4.)>1e-12)return 19;p.type2D();dwgBuffer first(bytes.data(),bytes.size());if(!p.parseDwg(DRW::AC1021,&first,0,0)||std::fabs(p.tgdir-45)>1e-10)return 14;auto cut=bytes;cut.pop_back();p.type2D();dwgBuffer trunc(cut.data(),cut.size());if(p.parseDwg(DRW::AC1021,&trunc,0,0)||p.tgdir!=0.)return 15;p.type2D();dwgBuffer again(bytes.data(),bytes.size());if(!p.parseDwg(DRW::AC1021,&again,0,0)||std::fabs(p.tgdir-45)>1e-10)return 16;}
 const auto dir=std::filesystem::temp_directory_path()/("vertex-tangent-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 if(!std::filesystem::create_directory(dir))return 20; struct Cleanup {std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} cleanup{dir};
 for(auto v:{DRW::AC1018,DRW::AC1021,DRW::AC1024,DRW::AC1027,DRW::AC1032}) {
  auto path=dir/("tangent-check-"+std::to_string(int(v))+".dwg"); std::filesystem::remove(path); dwgRW writer(path.string().c_str());Adapter a;a.out=&writer;
  if(!writer.write(&a,v,true)){std::cerr<<"write version "<<int(v)<<'\n';return 1;} Adapter b;dwgRW reader(path.string().c_str());if(!reader.read(&b,false)||b.count!=1||b.flags[0]!=2||b.flags[1]!=2||b.flags[2]!=0||std::fabs(b.angles[0]-45)>1e-10||std::fabs(b.angles[1]+60)>1e-10||std::fabs(b.angles[2]-20)>1e-10){std::cerr<<"read version "<<int(v)<<'\n';return 2;} std::filesystem::remove(path);
 }
 { auto path=dir/"tangent-failure-retry.dwg"; {std::ofstream o(path);o<<"sentinel";} dwgRW out(path.string().c_str());Adapter a;a.out=&out;a.bad=true;if(out.write(&a,DRW::AC1021,true))return 8;std::ifstream in(path);std::string contents((std::istreambuf_iterator<char>(in)),{});if(contents!="sentinel")return 9;a.bad=false;if(!out.write(&a,DRW::AC1021,true))return 10;std::filesystem::remove(path);}
 VertexProbe p;p.tgdir=std::numeric_limits<double>::denorm_min();p.setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex2D);dwgBufferW b,h;if(p.encodeDwg(DRW::AC1021,&b,0,nullptr,&h)||!b.data().empty()||!h.data().empty())return 3;p.tgdir=30.*std::numeric_limits<double>::denorm_min(); if(p.encodeDwg(DRW::AC1021,&b,0,nullptr,&h)||!b.data().empty()||!h.data().empty())return 21;p.tgdir=45.; if(!p.encodeDwg(DRW::AC1021,&b,0,nullptr,&h))return 4;
 VertexProbe v3; v3.setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex3D); v3.flags=32; v3.tgdir=45.; dwgBufferW a1,h1; if(!v3.encodeDwg(DRW::AC1021,&a1,0,nullptr,&h1))return 5; v3.tgdir=90.; dwgBufferW a2,h2; if(!v3.encodeDwg(DRW::AC1021,&a2,0,nullptr,&h2)||a1.data()!=a2.data()||h1.data()!=h2.data())return 6;
 VertexProbe bad; bad.tgdir=std::numeric_limits<double>::infinity();dwgBufferW bb,bh;if(bad.encodeDwg(DRW::AC1021,&bb,0,nullptr,&bh)||!bb.data().empty())return 7;
 std::cout<<"five versions, 3D and failure retry pass\n";
}
