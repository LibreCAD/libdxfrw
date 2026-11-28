// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>

namespace {
constexpr double turn=6.283185307179586;
struct Capture : LC3CompatCheck {
 void addEllipse(const DRW_Ellipse& e) override { ellipse=e; ++ellipses; }
 void addPolyline(const DRW_Polyline& p) override { poly=p; ++polylines; }
 DRW_Ellipse ellipse; DRW_Polyline poly; int ellipses=0,polylines=0;
};
struct DerivedEllipse:DRW_Ellipse {};
struct Writer:LC3CompatCheck {
 explicit Writer(dxfRW& w):out(w){}
 void writeEntities() override { ok=out.writeEllipse(ellipse); }
 bool finalizeDxfWrite() override {return ok;}
 dxfRW& out; DRW_Ellipse* ellipse=nullptr; bool ok=false;
};
struct OptWriter:Writer,DRW_DxfR12CcwEllipseOptIn {using Writer::Writer;};
struct HatchWriter:LC3CompatCheck {
 explicit HatchWriter(dxfRW& w):out(w){}
 void writeEntities() override {
  DRW_Hatch h;h.name="SOLID";h.solid=1;
  auto path=std::make_shared<DRW_HatchLoop>(1);
  auto e=std::make_shared<DRW_Ellipse>();e->basePoint={2.,3.,0.};e->secPoint={4.,0.,0.};
  e->ratio=.5;e->staparam=5.5;e->endparam=.5;e->isccw=0;
  path->objlist.push_back(e);path->update();h.appendLoop(path);ok=out.writeHatch(&h);
 }
 bool finalizeDxfWrite() override {return ok;}
 dxfRW& out;bool ok=false;
};
struct OptHatchWriter:HatchWriter,DRW_DxfR12CcwEllipseOptIn {using HatchWriter::HatchWriter;};
struct HatchCapture:LC3CompatCheck {
 void addHatch(const DRW_Hatch* h) override {
  if(!h||h->looplist.size()!=1||h->looplist[0]->objlist.size()!=1)return;
  const auto* e=dynamic_cast<const DRW_Ellipse*>(h->looplist[0]->objlist[0].get());
  ok=e&&e->isccw==0&&std::fabs(e->staparam-5.5)<1e-12&&std::fabs(e->endparam-.5)<1e-12;
 }
 bool ok=false;
};
bool same(const DRW_Polyline& a,const DRW_Polyline& b) {
 if(a.flags!=b.flags||a.vertlist.size()!=b.vertlist.size())return false;
 for(std::size_t i=0;i<a.vertlist.size();++i){
  const auto& x=a.vertlist[i]->basePoint; const auto& y=b.vertlist[i]->basePoint;
  if(x.x!=y.x||x.y!=y.y||x.z!=y.z)return false;
 }
 return true;
}
bool writeRead(const std::filesystem::path& path, DRW_Ellipse& e, bool opted,
               bool binary, DRW_Polyline& p) {
 dxfRW out(path.string().c_str()); Writer legacy(out); OptWriter modern(out);
 Writer* a=opted?static_cast<Writer*>(&modern):&legacy;a->ellipse=&e;
 if(!out.write(a,DRW::AC1009,binary)||!a->ok)return false;
 dxfRW in(path.string().c_str()); Capture capture;
 if(!in.read(&capture,false)||capture.polylines!=1||capture.ellipses!=0)return false;
 p=capture.poly;return true;
}
bool run(){
 const auto dir=std::filesystem::temp_directory_path() / ("ellipse-wrap-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(dir);
 struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code ec;std::filesystem::remove_all(path,ec);}} cleanup{dir};
 // A selected native DXF entity supplies the consumer's exact OCS-independent
 // ellipse fields. The opt-in belongs to the separate output adapter.
 std::string source="0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1027\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nELLIPSE\n100\nAcDbEntity\n8\n0\n100\nAcDbEllipse\n10\n2\n20\n3\n30\n4\n11\n4\n21\n0\n31\n0\n40\n0.5\n41\n5.5\n42\n0.5\n210\n0\n220\n0\n230\n1\n0\nENDSEC\n0\nEOF\n";
 dxfRW readNative(nullptr);Capture capture;if(!readNative.readAscii(&capture,false,source)||capture.ellipses!=1)return false;
 for(bool binary:{false,true})for(int normal=0;normal<2;++normal)for(int ratioCase=0;ratioCase<2;++ratioCase){
  DRW_Ellipse e=capture.ellipse;e.extPoint=normal?DRW_Coord(0.,1.,0.):DRW_Coord(0.,0.,1.);e.ratio=ratioCase?2.:.5;
  const auto beforeStart=e.staparam,beforeEnd=e.endparam;
  DRW_Polyline old,corrected;
  if(!writeRead(dir/"legacy.dxf",e,false,binary,old)||!writeRead(dir/"opt.dxf",e,true,binary,corrected))return false;
  if(e.staparam!=beforeStart||e.endparam!=beforeEnd||corrected.vertlist.size()!=28u||old.vertlist.size()!=103u)return false;
  double minX=std::numeric_limits<double>::infinity(),maxX=-minX;
  for(const auto& v:corrected.vertlist){minX=std::min(minX,v->basePoint.x);maxX=std::max(maxX,v->basePoint.x);}
  if(minX<4.8||maxX<5.99)return false;
  const auto& start=corrected.vertlist.front()->basePoint;const auto& end=corrected.vertlist.back()->basePoint;
  if(std::fabs(start.x-(2.+4.*std::cos(5.5)))>1e-10||std::fabs(end.x-(2.+4.*std::cos(.5)))>1e-10)return false;
  // The legacy negative sweep takes the far side of the ellipse.
  if(old.vertlist[old.vertlist.size()/2]->basePoint.x>0.)return false;
 }
 // Full turn, equal-parameter, noncanonical and explicit clockwise controls
 // remain byte-for-byte equivalent as geometry, even for an opting adapter.
 for(bool binary:{false,true})for(int mode=0;mode<5;++mode){
  DRW_Ellipse e=capture.ellipse;
  if(mode==0){e.staparam=0.;e.endparam=turn;}
  if(mode==1){e.staparam=.5;e.endparam=.5;}
  if(mode==2){e.staparam=7.;e.endparam=2.;}
  if(mode==3)e.isccw=0;
  if(mode==4){e.staparam=turn;e.endparam=0.;}
  DRW_Polyline legacy,enabled;
  if(!writeRead(dir/"control0.dxf",e,false,binary,legacy)||!writeRead(dir/"control1.dxf",e,true,binary,enabled)||!same(legacy,enabled))return false;
 }
 // A derived API type and direct clockwise HATCH-edge-style tessellation keep
 // their pre-existing signed-sweep behavior.
 DerivedEllipse derived;static_cast<DRW_Ellipse&>(derived)=capture.ellipse;
 DRW_Polyline legacy,enabled;
 if(!writeRead(dir/"derived0.dxf",derived,false,false,legacy)||!writeRead(dir/"derived1.dxf",derived,true,false,enabled)||!same(legacy,enabled))return false;
 DRW_Ellipse edge=capture.ellipse;edge.isccw=0;DRW_Polyline edgeResult;edge.toPolyline(&edgeResult,128);
 if(edgeResult.vertlist.size()!=103u||edgeResult.vertlist[edgeResult.vertlist.size()/2]->basePoint.x>0.)return false;
 // The opt-in must not alter modern ELLIPSE records.
 for(bool binary:{false,true}){
  DRW_Ellipse e=capture.ellipse; dxfRW out((dir/"modern.dxf").string().c_str());OptWriter a(out);a.ellipse=&e;
  if(!out.write(&a,DRW::AC1027,binary)||!a.ok)return false;
  dxfRW in((dir/"modern.dxf").string().c_str());Capture c;if(!in.read(&c,false)||c.ellipses!=1||c.ellipse.staparam!=5.5||c.ellipse.endparam!=.5)return false;
 }
 // A real HATCH ellipse edge stays clockwise and keeps its parameters.
 for(bool binary:{false,true})for(bool opted:{false,true}) {
  const auto hp=dir/"hatch.dxf";dxfRW hatchOut(hp.string().c_str());
  HatchWriter legacyHatch(hatchOut);OptHatchWriter optedHatch(hatchOut);
  HatchWriter* h=opted?static_cast<HatchWriter*>(&optedHatch):&legacyHatch;
  if(!hatchOut.write(h,DRW::AC1027,binary)||!h->ok)return false;
  dxfRW hatchIn(hp.string().c_str());HatchCapture hc;
  if(!hatchIn.read(&hc,false)||!hc.ok)return false;
 }
 // Invalid input cannot replace the destination; corrected same-writer retry.
 const auto path=dir/"transaction.dxf";{std::ofstream f(path);f<<"sentinel";}
 dxfRW out(path.string().c_str());OptWriter a(out);DRW_Ellipse e=capture.ellipse;a.ellipse=&e;e.ratio=std::numeric_limits<double>::quiet_NaN();
 if(out.write(&a,DRW::AC1009,false))return false;
 std::ifstream f(path);const std::string saved{std::istreambuf_iterator<char>(f),{}};if(saved!="sentinel")return false;
 e.ratio=.5;if(!out.write(&a,DRW::AC1009,false)||!a.ok)return false;
 return true;
}
}
int main(){if(!run()){std::cerr<<"R12 ellipse wrap control failed\n";return 1;}return 0;}
