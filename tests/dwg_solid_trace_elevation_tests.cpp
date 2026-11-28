// SPDX-License-Identifier: GPL-2.0-or-later
#include "drw_entities.h"
#include "libdwgr.h"
#include "lc3_compat_check.cpp"
#include "drw_header.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgutil.h"
#include "intern/dwgwriter15.h"
#include "intern/dwgwriter21.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <cstdint>
#include <iostream>
#include <memory>
#include <limits>
#include <vector>
class DwgBlockRecordXrefWireTestAccess {
public:
 static bool frame(dwgWriter15& w,std::uint32_t handle,std::vector<std::uint8_t>& body) {
  const auto it=std::find_if(w.m_objectMap.cbegin(),w.m_objectMap.cend(),[handle](const auto& p){return p.first==handle;});
  if(it==w.m_objectMap.cend())return false;
  const auto& bytes=w.m_buf.data();if(it->second>=bytes.size())return false;
  dwgBuffer f(const_cast<std::uint8_t*>(bytes.data()+it->second),bytes.size()-it->second);
  const auto length=f.getModularShort();const auto offset=f.getPosition();
  if(!f.isGood()||length<=0||offset+static_cast<std::size_t>(length)+2u>bytes.size()-it->second)return false;
  const auto crcOffset=offset+static_cast<std::size_t>(length);
  dwgBuffer payload(const_cast<std::uint8_t*>(bytes.data()+it->second),crcOffset);
  dwgBuffer crc(const_cast<std::uint8_t*>(bytes.data()+it->second+crcOffset),2u);
  if(payload.crc8(0xC0C1u,0,static_cast<std::int32_t>(crcOffset))!=crc.getRawShort16()||!crc.isGood())return false;
  body.assign(bytes.begin()+it->second+offset,bytes.begin()+it->second+crcOffset);return true;
 }
 static bool run() {
  DRW_Header header;dwgWriter21 w(nullptr,&header);
  if(!w.prepareDwgClassManifest()||!w.writeFileHeaderStub()||!w.writeDwgHeader()||!w.writeDwgClasses()||!w.writeDwgObjects())return false;
  struct SolidProbe:DRW_Solid {bool common(dwgBuffer& b){return DRW_Entity::parseDwgCommon(DRW::AC1021,&b,0u);}using DRW_Solid::parseDwg;};
  struct TraceProbe:DRW_Trace {bool common(dwgBuffer& b){return DRW_Entity::parseDwgCommon(DRW::AC1021,&b,0u);}using DRW_Trace::parseDwg;};
  bool alignedSolid=false,alignedTrace=false;
  for(unsigned type=0;type<2;++type)for(unsigned tilt=0;tilt<2;++tilt)for(unsigned thick=0;thick<2;++thick){
   DRW_Solid solid;DRW_Trace trace;DRW_Trace* e=type==0?static_cast<DRW_Trace*>(&solid):&trace;
   e->basePoint={1.,2.,3.};e->secPoint={4.,2.,3.};e->thirdPoint={1.,4.,3.};e->fourPoint={4.,4.,3.};e->extPoint=tilt?DRW_Coord(0.,1.,0.):DRW_Coord(0.,0.,1.);e->thickness=thick?.5:0.;
   // Invalid individual OCS Z components must not consume a frame or handle.
   for (int coordinate = 0; coordinate < 4; ++coordinate) {
    for (double value : {4., std::numeric_limits<double>::infinity(),
                         std::numeric_limits<double>::quiet_NaN()}) {
     if (coordinate == 0 && value == 4.) continue;
     double* fields[] = {&e->basePoint.z, &e->secPoint.z, &e->thirdPoint.z, &e->fourPoint.z};
     const double old = *fields[coordinate]; *fields[coordinate] = value;
     const auto before = w.buffer();
     if (w.encodeEntity(e) || w.buffer() != before) return false;
     *fields[coordinate] = old;
    }
   }
   if (!w.encodeEntity(e)) return false;
   DRW::DwgObjectFrameReceipt rec;if(!w.getLastDwgObjectFrame(rec)||!rec.valid||rec.objectHandle!=e->handle)return false;
   std::vector<std::uint8_t> body;if(!frame(w,e->handle,body))return false;
   dwgBuffer raw(body.data(),body.size());if(raw.getObjType(DRW::AC1021)!=(type==0?dwgType::SOLID:dwgType::TRACE))return false;
   const auto boundary=raw.getRawLong32();SolidProbe sp;TraceProbe tp;dwgBuffer scalar(body.data(),body.size());
   if(!(type==0?sp.common(scalar):tp.common(scalar)))return false;
   if(scalar.getThickness(true)!=(thick?.5:0.)||scalar.getBitDouble()!=3.)return false;
   for(double v:{1.,2.,4.,2.,1.,4.,4.,4.})if(scalar.getRawDouble()!=v)return false;
   const auto normal=scalar.getExtrusion(true);if(normal.x!=0.||normal.y!=(tilt?1.:0.)||normal.z!=(tilt?0.:1.))return false;
   const auto end=scalar.getPosition()*8u+scalar.getBitPos();
   if(!scalar.isGood()||boundary!=((end+8u)&~std::uint64_t{7u}))return false;
   if((end&7u)==0u){if(type==0)alignedSolid=true;else alignedTrace=true;}
   for(auto bit=end;bit<boundary;++bit)if((body[bit/8u]>>(7u-bit%8u))&1u)return false;
   const auto parse=[&](std::vector<std::uint8_t>& b){dwgBuffer in(b.data(),b.size());const bool ok=type==0?sp.parseDwg(DRW::AC1021,&in,0u):tp.parseDwg(DRW::AC1021,&in,0u);const auto& p=type==0?static_cast<const DRW_Trace&>(sp):static_cast<const DRW_Trace&>(tp);return ok&&in.isGood()&&p.basePoint.x==1.&&p.basePoint.y==2.&&p.basePoint.z==3.&&p.secPoint.x==4.&&p.thirdPoint.y==4.&&p.fourPoint.x==4.&&p.thickness==(thick?.5:0.)&&p.extPoint.y==(tilt?1.:0.);};
   if(!parse(body))return false;auto truncated=body;truncated.pop_back();if(parse(truncated)||!parse(body))return false;
   if((end&7u)==0u){auto legacy=body;legacy.erase(legacy.begin()+static_cast<std::ptrdiff_t>(end/8u));const auto old=static_cast<std::uint32_t>(end);
    for(std::size_t i=0;i<32u;++i){const auto pos=10u+i;const auto mask=static_cast<std::uint8_t>(1u<<(7u-pos%8u));const auto source=static_cast<unsigned>((i/8u)*8u+7u-i%8u);if((old>>source)&1u)legacy[pos/8u]|=mask;else legacy[pos/8u]&=static_cast<std::uint8_t>(~mask);}
    if(!parse(legacy)||!parse(body))return false;
   }
  }
  if(!alignedSolid||!alignedTrace)return false;
  // Legacy 2D-corner consumers leave all secondary Z values at zero while
  // setting the shared elevation.  The fourth point may also duplicate the
  // third point to represent a triangle. Both must remain writable.
  for (int type=0;type<2;++type) for (int representation=0;representation<2;++representation) {
   DRW_Solid solid; DRW_Trace trace; DRW_Trace* e=type==0?static_cast<DRW_Trace*>(&solid):&trace;
   e->basePoint={1.,2.,3.};e->secPoint={4.,2.,0.};e->thirdPoint={1.,4.,0.};
   e->fourPoint=representation==0?DRW_Coord(4.,4.,-0.):e->thirdPoint;
   e->extPoint={0.,1.,0.};
   if(!w.encodeEntity(e))return false;
   std::vector<std::uint8_t> body;if(!frame(w,e->handle,body))return false;
   SolidProbe sp; TraceProbe tp; dwgBuffer in(body.data(),body.size());
   if(!(type==0?sp.parseDwg(DRW::AC1021,&in,0u):tp.parseDwg(DRW::AC1021,&in,0u)))return false;
   const auto& p=type==0?static_cast<const DRW_Trace&>(sp):static_cast<const DRW_Trace&>(tp);
   if(p.basePoint.z!=3.||p.secPoint.z!=3.||p.thirdPoint.z!=3.||p.fourPoint.z!=3.
      ||p.extPoint.y!=1.||p.extPoint.z!=0.)return false;
  }
  // 3DFACE derives from TRACE but stores four distinct WCS Z coordinates.
  DRW_3Dface face; face.basePoint={0.,0.,1.};face.secPoint={4.,0.,2.};
  face.thirdPoint={0.,4.,3.};face.fourPoint={4.,4.,4.};face.invisibleflag=5;
  if(!w.encodeEntity(&face))return false;
  std::vector<std::uint8_t> faceBody;if(!frame(w,face.handle,faceBody))return false;
  struct FaceProbe:DRW_3Dface{using DRW_3Dface::parseDwg;} faceParsed;
  dwgBuffer faceIn(faceBody.data(),faceBody.size());
  if(!faceParsed.parseDwg(DRW::AC1021,&faceIn,0u)||!faceIn.isGood()
     ||faceParsed.basePoint.z!=1.||faceParsed.secPoint.z!=2.
     ||faceParsed.thirdPoint.z!=3.||faceParsed.fourPoint.z!=4.
     ||faceParsed.invisibleflag!=5)return false;
  DRW_Line line;line.basePoint={10.,11.,0.};line.secPoint={12.,13.,0.};if(!w.encodeEntity(&line))return false;
  std::vector<std::uint8_t> b;if(!frame(w,line.handle,b))return false;
  struct LineProbe:DRW_Line{using DRW_Line::parseDwg;} p;dwgBuffer in(b.data(),b.size());
  if(!p.parseDwg(DRW::AC1021,&in,0u)||!in.isGood()||p.basePoint.x!=10.||p.basePoint.y!=11.||p.secPoint.x!=12.||p.secPoint.y!=13.)return false;
  DRW_Trace invalid;invalid.extData.push_back(std::make_shared<DRW_Variant>(1000,"missing appid"));const auto before=w.buffer();
  if(w.encodeEntity(&invalid)||w.buffer()!=before)return false;invalid.extData.clear();return w.encodeEntity(&invalid);
 }
};
bool directCodecControl() {
 struct SolidProbe : DRW_Solid { using DRW_Solid::encodeDwg; };
 struct TraceProbe : DRW_Trace { using DRW_Trace::encodeDwg; };
 for (auto version : {DRW::AC1015,DRW::AC1018,DRW::AC1021,DRW::AC1024,DRW::AC1027,DRW::AC1032}) {
  for (int type=0;type<2;++type) {
   SolidProbe solid; TraceProbe trace;
   DRW_Trace& e=type==0?static_cast<DRW_Trace&>(solid):trace;
   e.basePoint={1.,2.,3.};e.secPoint={4.,2.,5.};e.thirdPoint={1.,4.,3.};e.fourPoint={4.,4.,3.};
   dwgBufferW body, strings, handles; body.putBit(1);
   const auto old=body.data(); const auto oldBit=body.bitCount();
   const bool result=type==0?solid.encodeDwg(version,&body,0u,&strings,&handles):trace.encodeDwg(version,&body,0u,&strings,&handles);
   if(result||body.data()!=old||body.bitCount()!=oldBit||strings.bitCount()!=0u||handles.bitCount()!=0u)return false;
  }
 }
 return true;
}
class FacadeControl final : public LC3CompatCheck {
public:
 explicit FacadeControl(dwgRW& writer) : out(writer) {}
 void writeEntities() override {
  DRW_Line first; first.basePoint={1.,1.,0.}; first.secPoint={2.,2.,0.};
  if(!out.writeLine(&first)) { ok=false; return; }
  ok = inherited ? (solid ? out.writeLine(&s) : out.writeLine(&t))
                 : (solid ? out.writeSolid(&s) : out.writeTrace(&t));
 }
 bool finalizeDwgWrite() override { return true; }
 dwgRW& out; DRW_Solid s; DRW_Trace t; bool solid=true; bool inherited=false; bool ok=false;
};
// A consumer-defined TRACE subclass may supply a custom nonplanar codec.
class CustomTrace : public DRW_Trace {
protected:
 bool encodeDwg(DRW::Version v,dwgBufferW* b,std::uint32_t bits,
                dwgBufferW* strings,dwgBufferW* handles) override {
  return DRW_Line::encodeDwg(v,b,bits,strings,handles);
 }
};
class CustomTraceControl final : public LC3CompatCheck {
public:
 explicit CustomTraceControl(dwgRW& writer):out(writer){}
 void writeEntities() override { ok=out.writeTrace(&trace); }
 bool finalizeDwgWrite() override { return true; }
 dwgRW& out; CustomTrace trace; bool ok=false;
};
class CustomLineReader final : public LC3CompatCheck {
public:
 void addLine(const DRW_Line& line) override { ok=line.basePoint.z==1.&&line.secPoint.z==2.; }
 bool ok=false;
};
class FaceControl final : public LC3CompatCheck {
public:
 explicit FaceControl(dwgRW& writer) : out(writer) {}
 void writeEntities() override { ok=out.writeTrace(&face); }
 bool finalizeDwgWrite() override { return true; }
 dwgRW& out; DRW_3Dface face; bool ok=false;
};
class FaceReader final : public LC3CompatCheck {
public:
 void add3dFace(const DRW_3Dface& face) override {
  ok=face.basePoint.z==1.&&face.secPoint.z==2.&&face.thirdPoint.z==3.
      &&face.fourPoint.z==4.&&face.invisibleflag==5;
 }
 bool ok=false;
};
bool wholeFileControl() {
 const auto dir=std::filesystem::temp_directory_path()/("solid-elevation-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(dir);
 struct Cleanup {std::filesystem::path p;~Cleanup(){std::error_code ec;std::filesystem::remove_all(p,ec);}} cleanup{dir};
 for (int type=0;type<2;++type) for (int inherited=0;inherited<2;++inherited) {
  const auto path=dir/(std::to_string(type)+std::to_string(inherited)+".dwg");
  {std::ofstream f(path,std::ios::binary);f<<"sentinel";}
  dwgRW out(path.string().c_str()); FacadeControl writer(out); writer.solid=(type==0); writer.inherited=(inherited!=0);
  DRW_Trace& e=writer.solid?static_cast<DRW_Trace&>(writer.s):writer.t;
  e.basePoint={1.,2.,3.};e.secPoint={4.,2.,4.};e.thirdPoint={1.,4.,3.};e.fourPoint={4.,4.,3.};
  const auto originalHandle=e.handle;
  if(out.write(&writer,DRW::AC1021,true)||e.handle!=originalHandle)return false;
  std::ifstream f(path,std::ios::binary);const std::string contents{std::istreambuf_iterator<char>(f),{}};
  if(contents!="sentinel")return false;
  e.secPoint.z=3.;
  if(!out.write(&writer,DRW::AC1021,true)||!writer.ok)return false;
 }
 const auto facePath=dir/"face.dwg";
 dwgRW faceOut(facePath.string().c_str()); FaceControl faceWriter(faceOut);
 faceWriter.face.basePoint={0.,0.,1.};faceWriter.face.secPoint={4.,0.,2.};
 faceWriter.face.thirdPoint={0.,4.,3.};faceWriter.face.fourPoint={4.,4.,4.};faceWriter.face.invisibleflag=5;
 if(!faceOut.write(&faceWriter,DRW::AC1021,true)||!faceWriter.ok)return false;
 dwgRW faceIn(facePath.string().c_str()); FaceReader faceReader;
 if(!faceIn.read(&faceReader,false)||!faceReader.ok)return false;
 const auto customPath=dir/"custom.dwg";
 dwgRW customOut(customPath.string().c_str()); CustomTraceControl custom(customOut);
 custom.trace.basePoint={0.,0.,1.};custom.trace.secPoint={4.,0.,2.};
 if(!customOut.write(&custom,DRW::AC1021,true)||!custom.ok)return false;
 dwgRW customIn(customPath.string().c_str()); CustomLineReader customReader;
 if(!customIn.read(&customReader,false)||!customReader.ok)return false;
 return true;
}
int main(){if(!directCodecControl()||!DwgBlockRecordXrefWireTestAccess::run()||!wholeFileControl()){std::cerr<<"SOLID/TRACE elevation check failed\n";return 1;}return 0;}
