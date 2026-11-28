#include "intern/dwgwriter27.h"
#include "intern/dwgwriter32.h"
#include "intern/dwgwriter24.h"
#include "intern/dwg_fixed_handles.h"
#include "intern/dwgobjectframe.h"
#include "intern/dwgbuffer.h"
#include "drw_header.h"
#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <iterator>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <new>
#include <vector>

static std::atomic<int> failAfter{-1};
void* operator new(std::size_t n){if(failAfter>=0 && failAfter-- == 0)throw std::bad_alloc();if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept{std::free(p);} void operator delete(void* p,std::size_t) noexcept{std::free(p);}

class DwgBlockRecordXrefWireTestAccess {
public:
 static bool emit(dwgWriter15& w,std::uint16_t type,std::uint32_t handle,bool vec,bool child) {
  if(vec){const std::vector<std::uint32_t> refs=child?std::vector<std::uint32_t>{0x40}:std::vector<std::uint32_t>{};return w.emitControlObject(type,handle,child?1u:0u,refs);}
  return child?w.emitControlObject(type,handle,1u,{0x40}):w.emitControlObject(type,handle,0u,{});
 }
 static bool frame(dwgWriter15& w,std::uint32_t handle,bool exact) {
  DRW::DwgObjectFrameReceipt r;if(!w.getLastDwgObjectFrame(r)||!r.valid||r.objectHandle!=handle
      ||r.classNumber!=(handle==DRW::DwgViewControlHandle?DRW::DwgViewControlObjectType:DRW::DwgUcsControlObjectType))return false;
  const auto it=std::find_if(w.m_objectMap.begin(),w.m_objectMap.end(),[&](const auto& x){return x.first==handle;});
  if(it==w.m_objectMap.end())return false;auto& bytes=w.m_buf.data();dwgBuffer buf(bytes.data(),bytes.size());DwgObjectFrame f;
  if(!f.readAt(buf,w.m_version,it->second))return false;
  if(!exact)return f.objectDataBitSize()!=35u && r.handleBitSize>0u;
  if(f.objectDataBitSize()!=35u||f.handleStreamBitSize()!=21u||f.body().size()!=7u||r.dataBitSize!=56u||r.handleBitSize!=0u||r.occurrences.size()!=3u)return false;
  for(std::size_t i=0;i<2;++i){const auto& o=r.occurrences[i+1];if(o.stream!=DRW::DwgObjectHandleStream::Handles||o.serializedSegment!=DRW::DwgObjectSerializedSegment::Data||o.token.startBit!=35u+i*8u||o.token.endBit!=43u+i*8u||o.token.code!=(i?3:4)||o.token.reference!=0u)return false;}
  dwgBuffer body(f.body().data(),f.body().size());if(!body.setPosition(4))return false;body.setBitPos(3);
  auto owner=body.getHandle(),xdict=body.getHandle();if(owner.code!=4||owner.ref||xdict.code!=3||xdict.ref)return false;
  for(int i=0;i<5;++i)if(body.getBit())return false;return body.isGood();
 }
 static bool spoof(dwgWriter15& w,std::uint16_t type,std::uint32_t handle,int mode) {
  auto& body=w.beginObject(handle);body.putObjType(DRW::AC1027,mode==0?DRW::DwgBlockControlObjectType:type);
  dwgHandle own;own.code=0;own.ref=handle;own.size=1;body.putHandle(own);body.putBitShort(0);body.putBitLong(0);body.putBit(0);body.putBit(0);body.putBitLong(0);body.putBit(0);
  auto& handles=w.m_objectHandles;handles.reset();dwgHandle owner;owner.code=4;owner.ref=0;owner.size=0;handles.putHandle(owner);dwgHandle xdict;xdict.code=3;xdict.ref=0;xdict.size=0;handles.putHandle(xdict);
  if(mode==1)body.data()[3]^=0x40u;
  if(mode==2)handles.data()[0]^=0x10u;
  if(mode==3)body.data()[4]^=0x20u;
  const auto before=w.buffer();w.finishObject();DRW::DwgObjectFrameReceipt frame;
  return w.objectWriteFailed() && w.buffer()==before && w.m_objectMap.empty()
   && w.m_currentHandle==0 && !w.getLastDwgObjectFrame(frame);
 }
 static bool collision(dwgWriter15& w,std::uint16_t type,std::uint32_t handle) {
  w.m_dwgClassInstanceHandleOwners.emplace(handle,500u);
  const auto before=w.buffer();const auto owners=w.m_dwgClassInstanceHandleOwners;
  const bool written=emit(w,type,handle,false,false);DRW::DwgObjectFrameReceipt frame;
  return !written && w.buffer()==before && w.m_objectMap.empty()
      && w.m_dwgClassInstanceHandleOwners==owners && !w.getLastDwgObjectFrame(frame)
      && w.objectWriteFailed();
 }
 static int fault(dwgWriter15& w,std::uint16_t type,std::uint32_t handle,bool vec,int index) {
  const auto before=w.buffer();const auto map=w.m_objectMap;const auto identities=w.m_emittedDwgObjectHandles;const auto controls=w.m_emittedDwgTableControlHandles;
  const auto generation=w.m_nextDwgObjectFrameGeneration;DRW::DwgObjectFrameReceipt old;const bool had=w.getLastDwgObjectFrame(old);
  failAfter=index;bool written=false;try{written=emit(w,type,handle,vec,false);}catch(...){failAfter=-1;return 0;}
  const bool triggered=failAfter<0;failAfter=-1;
  if(!triggered)return written&&frame(w,handle,true)?2:0;
  DRW::DwgObjectFrameReceipt now;const bool has=w.getLastDwgObjectFrame(now);
  return !written && w.buffer()==before && w.m_objectMap==map && w.m_emittedDwgObjectHandles==identities
   && w.m_emittedDwgTableControlHandles==controls && w.m_nextDwgObjectFrameGeneration==generation
   && w.m_currentHandle==0 && (!has || (had && now.generation==old.generation)) && w.objectWriteFailed()?1:0;
 }
};

template<class W> static bool normal(std::uint16_t type,std::uint32_t h,bool exact,bool child=false){std::ofstream out;DRW_Header head;W w(&out,&head);return DwgBlockRecordXrefWireTestAccess::emit(w,type,h,false,child)&&DwgBlockRecordXrefWireTestAccess::frame(w,h,exact);}
class FileWriter final:public LC3CompatCheck {
public: explicit FileWriter(dwgRW& o):out(o){}
 void writeEntities() override { if(fail){DRW_Polyline p;p.addVertex(DRW_Vertex(1,2,0,0));p.addVertex(DRW_Vertex(3,4,0,0));p.vertlist[0]->handle=p.vertlist[1]->handle=0x90;ok=out.writePolyline(&p);}else{DRW_Line l;l.basePoint={1,2,3};l.secPoint={4,5,6};ok=out.writeLine(&l);}}
 bool finalizeDwgWrite() override{return ok;}
 dwgRW& out;bool fail=true,ok=false;
};
static std::string read(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(){
 for(const auto pair:{std::pair<std::uint16_t,std::uint32_t>{DRW::DwgViewControlObjectType,DRW::DwgViewControlHandle},{DRW::DwgUcsControlObjectType,DRW::DwgUcsControlHandle}}){
  if(!normal<dwgWriter27>(pair.first,pair.second,true)||!normal<dwgWriter24>(pair.first,pair.second,false)||!normal<dwgWriter32>(pair.first,pair.second,false)||!normal<dwgWriter27>(pair.first,pair.second,false,true))return 1;
  {std::ofstream out;DRW_Header h;dwgWriter27 w(&out,&h);if(!DwgBlockRecordXrefWireTestAccess::collision(w,pair.first,pair.second))return 4;}
  for(int mode=0;mode<4;++mode){std::ofstream out;DRW_Header h;dwgWriter27 w(&out,&h);if(!DwgBlockRecordXrefWireTestAccess::spoof(w,pair.first,pair.second,mode))return 7;}
  for(bool vec:{false,true}) for(bool populated:{false,true}){int success=-1;
   for(int i=0;i<500;++i){std::ofstream out;DRW_Header h;dwgWriter27 w(&out,&h);
    if(populated && (!w.prepareDwgClassManifest()||!w.writeFileHeaderStub()||!w.writeDwgHeader()||!w.writeDwgClasses()))return 2;
    int r=DwgBlockRecordXrefWireTestAccess::fault(w,pair.first,pair.second,vec,i);if(r==2){success=i;break;}if(r!=1){std::fprintf(stderr,"fault %d vec=%d populated=%d handle=%u\n",i,vec,populated,pair.second);return 2;}}
   if(success<10)return 3;
  }
 }
 const auto path=std::filesystem::temp_directory_path()/("view-ucs-control-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dwg");
 struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove(p,e);}} cleanup{path};{std::ofstream o(path);o<<"sentinel";}
 dwgRW out(path.string().c_str());FileWriter writer(out);if(out.write(&writer,DRW::AC1027,true)||read(path)!="sentinel")return 5;
 writer.fail=false;if(!out.write(&writer,DRW::AC1027,true)||!writer.ok)return 6;
}
