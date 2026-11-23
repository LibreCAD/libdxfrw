#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgwriter21.h"
#include "drw_header.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <limits>
struct ViewportProbe:DRW_Viewport {using DRW_Viewport::parseDwg;using DRW_Viewport::encodeDwg;void setDwgType(std::uint16_t value){oType=value;}bool common(dwgBuffer& b){return DRW_Entity::parseDwgCommon(DRW::AC1021,&b,0u);}};
static bool appendBits(dwgBufferW& output, const dwgBufferW& input) {
 dwgBuffer reader(input.data().data(),input.data().size());
 for(std::uint64_t bit=0;bit<input.bitCount();++bit){
  if(!reader.setPosition(static_cast<std::size_t>(bit/8u)))return false;
  reader.setBitPos(static_cast<std::uint8_t>(bit%8u));
  if(!reader.isGood())return false;
  output.putBit(reader.getBit());
  if(!reader.isGood()||!output.isGood())return false;
 }
 return true;
}
static bool viewportHandleRoles(){
 constexpr DRW::Version version=DRW::AC1018;
 ViewportProbe source;
 source.handle=0x700u;source.parentHandle=0x710u;source.setDwgType(34);
 source.frozenLayerHandles={0x720u};source.clipBoundaryHandle=0x730u;
 source.namedUcsHandle=0x740u;source.baseUcsHandle=0x750u;
 dwgBufferW body,handles;
 if(!source.encodeDwg(version,&body,0,nullptr,&handles))return false;
 const auto encodedOccurrences=handles.handleOccurrences();
 auto buildInput=[&](std::uint32_t target,std::uint8_t replacement,
                     bool alter,dwgBufferW& combined,
                     std::uint32_t& handleBits){
  dwgBufferW handleCopy=handles;
  if(alter){
   bool found=false;
   for(const auto& occurrence:encodedOccurrences){
    if(occurrence.reference!=target)continue;
    const std::size_t byte=static_cast<std::size_t>(occurrence.startBit/8u);
    if((occurrence.startBit%8u)!=0u||byte>=handleCopy.data().size())return false;
    handleCopy.data()[byte]=static_cast<std::uint8_t>(
        (replacement<<4u)|(handleCopy.data()[byte]&0x0Fu));
    found=true;break;
   }
   if(!found)return false;
  }
  combined.reset();
  if(!appendBits(combined,body)||!appendBits(combined,handleCopy))return false;
  handleBits=static_cast<std::uint32_t>(handleCopy.bitCount());
  return true;
 };
 struct RoleCase{std::uint32_t reference;std::uint8_t wrongCode;const char* label;};
 const RoleCase cases[]={{0x720u,5u,"frozen layer soft pointer"},
                         {0x730u,5u,"clip boundary soft pointer"},
                         {0x740u,4u,"named UCS hard pointer"},
                         {0x750u,4u,"base UCS hard pointer"}};
 for(const auto& role:cases){
  dwgBufferW malformed;std::uint32_t bits=0;
  if(!buildInput(role.reference,role.wrongCode,true,malformed,bits))return false;
  const auto malformedBytes=malformed.data();
  dwgBuffer bad(malformedBytes.data(),malformedBytes.size());
  ViewportProbe parsed;parsed.handle=0xDEADu;
  if(parsed.parseDwg(version,&bad,bits)||bad.isGood()
     ||parsed.handle!=0u||!parsed.frozenLayerHandles.empty()
     ||parsed.clipBoundaryHandle!=0u||parsed.namedUcsHandle!=0u
     ||parsed.baseUcsHandle!=0u)return false;
  dwgBufferW corrected;bits=0;
  if(!buildInput(0u,0u,false,corrected,bits))return false;
  const auto correctedBytes=corrected.data();
  dwgBuffer retry(correctedBytes.data(),correctedBytes.size());
  if(!parsed.parseDwg(version,&retry,bits)||!retry.isGood()
     ||parsed.handle!=source.handle||parsed.frozenLayerHandles.size()!=1u
     ||parsed.frozenLayerHandles[0]!=source.frozenLayerHandles[0]
     ||parsed.clipBoundaryHandle!=source.clipBoundaryHandle
     ||parsed.namedUcsHandle!=source.namedUcsHandle
     ||parsed.baseUcsHandle!=source.baseUcsHandle)return false;
 }
 dwgBufferW typedNullHandles;
 bool typedNullFound=false;
 for(const auto& occurrence:encodedOccurrences){
  if(occurrence.reference!=0x720u)continue;
  const std::size_t byte=static_cast<std::size_t>(occurrence.startBit/8u);
  if((occurrence.startBit%8u)!=0u||byte>=handles.data().size())
   return false;
  const std::uint8_t size=static_cast<std::uint8_t>(
      handles.data()[byte]&0x0Fu);
  if(byte+size>=handles.data().size())return false;
  typedNullHandles.putBytes(handles.data().data(),byte);
  typedNullHandles.putRawChar8(0x50u); // typed hard-pointer null
  const std::size_t suffix=byte+1u+size;
  if(suffix<handles.data().size())
   typedNullHandles.putBytes(handles.data().data()+suffix,
                             handles.data().size()-suffix);
  if(!typedNullHandles.isGood())return false;
  typedNullFound=true;break;
 }
 if(!typedNullFound)return false;
 dwgBufferW typedNull;std::uint32_t typedNullBits=0;
 if(!appendBits(typedNull,body)||!appendBits(typedNull,typedNullHandles))
  return false;
 typedNullBits=static_cast<std::uint32_t>(typedNullHandles.bitCount());
 const auto typedNullBytes=typedNull.data();
 dwgBuffer typedNullInput(typedNullBytes.data(),typedNullBytes.size());
 ViewportProbe typedNullParsed;
 if(!typedNullParsed.parseDwg(version,&typedNullInput,typedNullBits)
    ||!typedNullInput.isGood()||typedNullParsed.frozenLayerHandles.size()!=1u
    ||typedNullParsed.frozenLayerHandles[0]!=0u
    ||typedNullParsed.clipBoundaryHandle!=source.clipBoundaryHandle
    ||typedNullParsed.namedUcsHandle!=source.namedUcsHandle
    ||typedNullParsed.baseUcsHandle!=source.baseUcsHandle)return false;
 dwgBufferW relative;std::uint32_t relativeBits=0;
 if(!buildInput(0x720u,0x0Au,true,relative,relativeBits))return false;
 const auto relativeBytes=relative.data();
 dwgBuffer relativeInput(relativeBytes.data(),relativeBytes.size());
 ViewportProbe relativeParsed;
 if(!relativeParsed.parseDwg(version,&relativeInput,relativeBits)
    ||!relativeInput.isGood()||relativeParsed.frozenLayerHandles.size()!=1u
    ||relativeParsed.frozenLayerHandles[0]!=0xE20u)return false;
 return true;
}
struct LegacyWriterStub:dwgWriter15 {explicit LegacyWriterStub(DRW_Header* h):dwgWriter15(nullptr,h){}int calls=0;bool encodeEntity(DRW_Entity*) override{++calls;return false;}};
class DwgBlockRecordXrefWireTestAccess {public:
 static bool legacyFacade(DRW::Version version){DRW_Header h;dwgRW out("");auto writer=std::make_unique<LegacyWriterStub>(&h);auto* observed=writer.get();out.version=version;out.writer=std::move(writer);DRW_Viewport v;v.snapAngle=30.*std::numeric_limits<double>::denorm_min();const bool result=out.writeViewport(&v);return !result&&observed->calls==1;}
 static bool frame(dwgWriter15& w,std::uint32_t id,std::vector<std::uint8_t>& b){auto i=std::find_if(w.m_objectMap.begin(),w.m_objectMap.end(),[id](const auto& x){return x.first==id;});if(i==w.m_objectMap.end())return false;const auto& d=w.buffer();dwgBuffer in(d.data()+i->second,d.size()-i->second);auto n=in.getModularShort();auto start=in.getPosition();if(!in.isGood()||n<=0||start+static_cast<std::size_t>(n)+2>d.size()-i->second)return false;b.assign(d.begin()+i->second+start,d.begin()+i->second+start+n);return true;}
};
static bool ac1021HardPointerClipBoundaryCompatibility(){
 constexpr std::uint32_t clipTarget=0x12345678u;
 DRW_Header header;dwgWriter21 writer(nullptr,&header);
 if(!writer.prepareDwgClassManifest()||!writer.writeFileHeaderStub()
    ||!writer.writeDwgHeader()||!writer.writeDwgClasses()
    ||!writer.writeDwgObjects())return false;
 ViewportProbe source;source.clipBoundaryHandle=clipTarget;
 source.snapAngle=30.;source.twistAngle=45.;
 if(!writer.encodeEntity(&source))return false;
 std::vector<std::uint8_t> frame;
 if(!DwgBlockRecordXrefWireTestAccess::frame(writer,source.handle,frame))
  return false;
 ViewportProbe softParsed;dwgBuffer softInput(frame.data(),frame.size());
 if(!softParsed.parseDwg(DRW::AC1021,&softInput,0u)||!softInput.isGood()
    ||softParsed.clipBoundaryHandle!=clipTarget)return false;
 const std::array<std::uint8_t,5> softHandle={0x44u,0x12u,0x34u,0x56u,0x78u};
 auto hardFrame=frame;
 const auto occurrence=std::search(hardFrame.begin(),hardFrame.end(),
                                   softHandle.begin(),softHandle.end());
 if(occurrence==hardFrame.end())return false;
 if(std::search(std::next(occurrence),hardFrame.end(),
                softHandle.begin(),softHandle.end())!=hardFrame.end())
  return false;
 *occurrence=static_cast<std::uint8_t>(0x54u|(*occurrence&0x0Fu));
 dwgBuffer hardInput(hardFrame.data(),hardFrame.size());
 ViewportProbe hardParsed;
 return hardParsed.parseDwg(DRW::AC1021,&hardInput,0u)
     &&hardInput.isGood()&&hardParsed.handle==source.handle
     &&hardParsed.clipBoundaryHandle==clipTarget;
}
static bool wireAndOverflow(){DRW_Header h;dwgWriter21 w(nullptr,&h);if(!w.prepareDwgClassManifest()||!w.writeFileHeaderStub()||!w.writeDwgHeader()||!w.writeDwgClasses()||!w.writeDwgObjects())return false;DRW_Viewport v;v.snapAngle=30.;v.twistAngle=45.;v.viewTarget={4,5,6};v.viewDir={1,2,3};v.centerPX=9.;v.viewHeight=10.;v.viewLength=50.;if(!w.encodeEntity(&v))return false;std::vector<std::uint8_t> body;if(!DwgBlockRecordXrefWireTestAccess::frame(w,v.handle,body))return false;ViewportProbe parsed;dwgBuffer scalar(body.data(),body.size());if(!parsed.common(scalar))return false;(void)scalar.get3BitDouble();(void)scalar.getBitDouble();(void)scalar.getBitDouble();auto target=scalar.get3BitDouble();auto direction=scalar.get3BitDouble();if(target.x!=4.||target.y!=5.||target.z!=6.||direction.x!=1.||direction.y!=2.||direction.z!=3.)return false;const auto twist=scalar.getPosition()*8u+scalar.getBitPos();if(std::fabs(scalar.getBitDouble()-45./ARAD)>1e-12)return false;for(int i=0;i<4;++i)(void)scalar.getBitDouble();const auto snap=scalar.getPosition()*8u+scalar.getBitPos();if(std::fabs(scalar.getBitDouble()-30./ARAD)>1e-12||scalar.getRawDouble()!=9.||!scalar.isGood())return false;dwgBuffer good(body.data(),body.size());if(!parsed.parseDwg(DRW::AC1021,&good,0)||std::fabs(parsed.snapAngle-30.)>1e-10||std::fabs(parsed.twistAngle-45.)>1e-10)return false;for(auto pos:{twist,snap}){auto bytes=body;dwgBufferW huge;huge.putBitDouble(1.e308);if(huge.bitCount()!=66u)return false;for(std::size_t i=0;i<66u;++i){auto mask=static_cast<std::uint8_t>(0x80u>>((pos+i)%8u));if((huge.data()[i/8u]>>(7u-i%8u))&1u)bytes[(pos+i)/8u]|=mask;else bytes[(pos+i)/8u]&=static_cast<std::uint8_t>(~mask);}dwgBuffer bad(bytes.data(),bytes.size());if(parsed.parseDwg(DRW::AC1021,&bad,0)||parsed.snapAngle!=0.||parsed.twistAngle!=0.)return false;dwgBuffer retry(body.data(),body.size());if(!parsed.parseDwg(DRW::AC1021,&retry,0)||std::fabs(parsed.snapAngle-30.)>1e-10||std::fabs(parsed.twistAngle-45.)>1e-10)return false;}return true;}
struct CustomViewport:DRW_Viewport {bool encodeDwg(DRW::Version,dwgBufferW*,std::uint32_t,dwgBufferW*,dwgBufferW*) override{return false;}};
struct Adapter:LC3CompatCheck {dwgRW* out=nullptr;bool bad=false,tilt=false,badTwist=false,derived=false,viaPoint=false;int count=0;DRW_Viewport got,value;CustomViewport sub;
 void writeEntities() override {DRW_Viewport& v=derived?static_cast<DRW_Viewport&>(sub):value;v.space=DRW::PaperSpace;v.basePoint={5,5,0};v.pswidth=10.;v.psheight=8.;v.snapAngle=bad&&!badTwist?30.*std::numeric_limits<double>::denorm_min():30.;v.twistAngle=bad&&badTwist?30.*std::numeric_limits<double>::denorm_min():45.;v.statusFlags=16;v.renderMode=2;if(tilt){v.viewDir={1,2,3};v.viewTarget={4,5,6};}if(viaPoint)(void)out->writePoint(&v);else (void)out->writeViewport(&v);}
 void addViewport(const DRW_Viewport& v) override {got=v;++count;}
};
int main(){for(auto version:{DRW::AC1012,DRW::AC1014}){if(!DwgBlockRecordXrefWireTestAccess::legacyFacade(version))return 12;ViewportProbe zero,tiny;tiny.snapAngle=30.*std::numeric_limits<double>::denorm_min();dwgBufferW z,zs,zh,t,ts,th;const bool a=zero.encodeDwg(version,&z,0,&zs,&zh),b=tiny.encodeDwg(version,&t,0,&ts,&th);if(a!=b||z.data()!=t.data()||zs.data()!=ts.data()||zh.data()!=th.data()||z.bitCount()!=t.bitCount())return 13;}if(!wireAndOverflow()){std::cerr<<"wire/overflow failed\n";return 1;}if(!viewportHandleRoles()){std::cerr<<"handle role validation/retry failed\n";return 14;}if(!ac1021HardPointerClipBoundaryCompatibility()){std::cerr<<"AC1021 clip-boundary compatibility failed\n";return 15;}
 for(int which=0;which<2;++which)for(double value:{std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::denorm_min(),30.*std::numeric_limits<double>::denorm_min()}){ViewportProbe v;(which?v.snapAngle:v.twistAngle)=value;dwgBufferW b,s,h;b.putRawChar8(0xa5);b.putBit(1);s.putRawChar8(0x5a);h.putRawChar8(0x3c);auto bb=b.data(),ss=s.data(),hh=h.data();auto n=b.bitCount();if(v.encodeDwg(DRW::AC1021,&b,0,&s,&h)||bb!=b.data()||ss!=s.data()||hh!=h.data()||n!=b.bitCount())return 2;(which?v.snapAngle:v.twistAngle)=30.;if(!v.encodeDwg(DRW::AC1021,&b,0,&s,&h))return 3;}
 auto dir=std::filesystem::temp_directory_path()/("viewport-angles-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directory(dir);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} cleanup{dir};
 for(auto version:{DRW::AC1015,DRW::AC1018,DRW::AC1021,DRW::AC1024,DRW::AC1027,DRW::AC1032})for(bool tilt:{false,true}) {auto p=dir/(std::to_string(int(version))+(tilt?"t":"p")+".dwg");dwgRW out(p.string().c_str());Adapter a;a.out=&out;a.tilt=tilt;if(!out.write(&a,version,true)){std::cerr<<"write "<<int(version)<<"\n";return 4;}dwgRW in(p.string().c_str());Adapter r;if(!in.read(&r,false)||r.count!=1||std::fabs(r.got.snapAngle-30.)>1e-10||std::fabs(r.got.twistAngle-45.)>1e-10||r.got.statusFlags!=16||r.got.renderMode!=2||(tilt&&(r.got.viewDir.x!=1||r.got.viewDir.y!=2||r.got.viewDir.z!=3||r.got.viewTarget.x!=4||r.got.viewTarget.y!=5||r.got.viewTarget.z!=6))){std::cerr<<"read "<<int(version)<<" count="<<r.count<<" flags="<<r.got.statusFlags<<" render="<<r.got.renderMode<<"\n";return 5;}}
 auto p=dir/"sentinel.dwg";{std::ofstream f(p);f<<"sentinel";}dwgRW out(p.string().c_str());for(bool via:{false,true})for(bool twist:{false,true}){Adapter bad;bad.out=&out;bad.bad=true;bad.badTwist=twist;bad.viaPoint=via;const auto oldHandle=bad.value.handle,oldOwner=bad.value.parentHandle;if(out.write(&bad,DRW::AC1021,true))return 6;if(bad.value.handle!=oldHandle||bad.value.parentHandle!=oldOwner)return 7;{std::ifstream f(p);std::string x((std::istreambuf_iterator<char>(f)),{});if(x!="sentinel")return 8;}}Adapter ok;ok.out=&out;if(!out.write(&ok,DRW::AC1021,true))return 9;Adapter custom;custom.out=&out;custom.bad=true;custom.derived=true;if(!out.write(&custom,DRW::AC1021,true))return 10;
 std::cout<<"VIEWPORT angle controls pass\n";
}
