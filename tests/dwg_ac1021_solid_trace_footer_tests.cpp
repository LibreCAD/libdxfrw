// SPDX-License-Identifier: GPL-2.0-or-later
#include "drw_entities.h"
#include "drw_header.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgutil.h"
#include "intern/dwgwriter15.h"
#include "intern/dwgwriter21.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <memory>
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
   if(!w.encodeEntity(e))return false;
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
  DRW_Line line;line.basePoint={10.,11.,0.};line.secPoint={12.,13.,0.};if(!w.encodeEntity(&line))return false;
  std::vector<std::uint8_t> b;if(!frame(w,line.handle,b))return false;
  struct LineProbe:DRW_Line{using DRW_Line::parseDwg;} p;dwgBuffer in(b.data(),b.size());
  if(!p.parseDwg(DRW::AC1021,&in,0u)||!in.isGood()||p.basePoint.x!=10.||p.basePoint.y!=11.||p.secPoint.x!=12.||p.secPoint.y!=13.)return false;
  DRW_Trace invalid;invalid.extData.push_back(std::make_shared<DRW_Variant>(1000,"missing appid"));const auto before=w.buffer();
  if(w.encodeEntity(&invalid)||w.buffer()!=before)return false;invalid.extData.clear();return w.encodeEntity(&invalid);
 }
};
int main(){if(!DwgBlockRecordXrefWireTestAccess::run()){std::cerr<<"SOLID/TRACE footer check failed\n";return 1;}return 0;}
