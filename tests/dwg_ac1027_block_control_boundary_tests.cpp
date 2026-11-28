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
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <new>
#include <string>
#include <vector>

static std::atomic<int> failAfter{-1};
void* operator new(std::size_t n) {
 if(failAfter>=0 && failAfter-- == 0) throw std::bad_alloc();
 if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();
}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}

class DwgBlockRecordXrefWireTestAccess {
public:
 static bool frame(dwgWriter15& w,bool exact) {
  DRW::DwgObjectFrameReceipt r;if(!w.getLastDwgObjectFrame(r)||!r.valid||r.objectHandle!=1||r.classNumber!=DRW::DwgBlockControlObjectType)return false;
  const auto found=std::find_if(w.m_objectMap.begin(),w.m_objectMap.end(),[](const auto& e){return e.first==1u;});
  if(found==w.m_objectMap.end())return false;
  auto& data=w.m_buf.data();dwgBuffer bytes(data.data(),data.size());DwgObjectFrame f;
  if(!f.readAt(bytes,w.m_version,found->second))return false;
  if(!exact)return f.objectDataBitSize()!=35u && r.handleBitSize>0u;
  if(f.objectDataBitSize()!=35u||f.handleStreamBitSize()!=53u||f.body().size()!=11u||r.dataBitSize!=88u||r.handleBitSize!=0u||r.occurrences.size()!=5u)return false;
  const std::array<std::uint64_t,4> starts{{35,43,51,67}},ends{{43,51,67,83}},refs{{0,0,23,24}};
  const std::array<std::uint8_t,4> roles{{4,3,3,3}};
  for(std::size_t i=0;i<4;++i){const auto& o=r.occurrences[i+1];if(o.stream!=DRW::DwgObjectHandleStream::Handles||o.serializedSegment!=DRW::DwgObjectSerializedSegment::Data||o.token.startBit!=starts[i]||o.token.endBit!=ends[i]||o.token.code!=roles[i]||o.token.reference!=refs[i])return false;}
  dwgBuffer body(f.body().data(),f.body().size());if(!body.setPosition(4))return false;body.setBitPos(3);
  for(std::size_t i=0;i<4;++i){auto h=body.getHandle();if(h.code!=roles[i]||h.ref!=refs[i])return false;}
  for(int i=0;i<5;++i)if(body.getBit()!=0)return false;
  return body.isGood();
 }
 static int allocationFault(dwgWriter27& w,int failIndex) {
  auto& body=w.beginObject(1);body.putObjType(DRW::AC1027,DRW::DwgBlockControlObjectType);
  dwgHandle own;own.code=0;own.ref=1;own.size=1;body.putHandle(own);body.putBitShort(0);body.putBitLong(0);body.putBit(0);body.putBit(0);body.putBitLong(0);body.putBit(0);
  auto& handles=w.m_objectHandles;handles.reset();
  const auto put=[&](std::uint8_t code,std::uint32_t ref){dwgHandle h;h.code=code;h.ref=ref;h.size=ref?1:0;handles.putHandle(h);};
  put(4,0);put(3,0);put(3,23);put(3,24);
  body.data().shrink_to_fit();if(body.bitCount()!=35||handles.bitCount()!=48||body.data().capacity()!=body.data().size())return false;
  if(failIndex==-2) body.data()[3] ^= 0x40u; // own handle reference bit 25
  if(failIndex==-3) handles.data()[0] ^= 0x10u; // owner role 4 -> 5
  if(failIndex==-4) body.data()[4] ^= 0x20u; // absent marker -> present
  if(failIndex==-5) w.m_dwgClassInstanceHandleOwners.emplace(1u,500u);
  const auto before=w.buffer();const auto mapBefore=w.m_objectMap;
  const auto emittedBefore=w.m_emittedDwgObjectHandles;
  const auto classBefore=w.m_dwgClassInstanceHandleOwners;
  const auto generationBefore=w.m_nextDwgObjectFrameGeneration;
  failAfter=failIndex<0?-1:failIndex;
  try{static_cast<dwgWriter15&>(w).finishObject();}catch(...){failAfter=-1;return false;}
  const bool triggered=failIndex<0 || failAfter<0;failAfter=-1;DRW::DwgObjectFrameReceipt r;
  if (!triggered)
   return !static_cast<dwgWriter15&>(w).objectWriteFailed() && frame(w,true) ? 2 : 0;
  return static_cast<dwgWriter15&>(w).objectWriteFailed() && w.buffer()==before
   && !w.getLastDwgObjectFrame(r) && w.m_objectMap==mapBefore
   && w.m_emittedDwgObjectHandles==emittedBefore
   && w.m_dwgClassInstanceHandleOwners==classBefore
   && w.m_currentHandle==0u && w.m_nextDwgObjectFrameGeneration==generationBefore ? 1 : 0;
 }
};

class FileWriter final:public LC3CompatCheck {
public: explicit FileWriter(dwgRW& o):out(o){}
 void writeEntities() override { if(fail){DRW_Polyline p;p.addVertex(DRW_Vertex(1,2,0,0));p.addVertex(DRW_Vertex(3,4,0,0));p.vertlist[0]->handle=p.vertlist[1]->handle=0x90;ok=out.writePolyline(&p);}else{DRW_Line l;l.basePoint={1,2,3};l.secPoint={4,5,6};ok=out.writeLine(&l);}}
 bool finalizeDwgWrite() override{return ok;}
 dwgRW& out;bool fail=true,ok=false;
};
static std::string read(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
template<class W> static bool writeControl(bool exact){std::ofstream out;DRW_Header h;W w(&out,&h);return w.prepareDwgClassManifest()&&w.writeFileHeaderStub()&&w.writeDwgHeader()&&w.writeDwgClasses()&&w.writeDwgObjects()&&w.emitDeferredBlockControl()&&DwgBlockRecordXrefWireTestAccess::frame(w,exact);}
int main(){
 if(!writeControl<dwgWriter27>(true)||!writeControl<dwgWriter24>(false)||!writeControl<dwgWriter32>(false))return 1;
 {std::ofstream out;DRW_Header h;dwgWriter27 w(&out,&h);
  if(!w.prepareDwgClassManifest()||!w.writeFileHeaderStub()||!w.writeDwgHeader()||!w.writeDwgClasses()
     || !w.defineBlock("NAMED",DRW_Coord()) || !w.writeDwgObjects() || !w.emitDeferredBlockControl()
     || !DwgBlockRecordXrefWireTestAccess::frame(w,false))return 5;
 }
 for(int i : {-5,-4,-3,-2}){std::ofstream out;DRW_Header h;dwgWriter27 w(&out,&h);if(!w.prepareDwgClassManifest()||!w.writeFileHeaderStub()||!w.writeDwgHeader()||!w.writeDwgClasses()||!w.writeDwgObjects()||DwgBlockRecordXrefWireTestAccess::allocationFault(w,i)!=1){std::fprintf(stderr,"negative %d failed\n",i);return 2;}}
 for(bool populated : {false,true}) {
  int firstSuccess=-1;
  for(int i=0;i<80;++i){std::ofstream out;DRW_Header h;dwgWriter27 w(&out,&h);
   if(populated && (!w.prepareDwgClassManifest()||!w.writeFileHeaderStub()||!w.writeDwgHeader()||!w.writeDwgClasses()||!w.writeDwgObjects()))return 2;
   const int result=DwgBlockRecordXrefWireTestAccess::allocationFault(w,i);if(result==2){firstSuccess=i;break;}if(result!=1){std::fprintf(stderr,"fault %d populated=%d failed\n",i,populated);return 2;}}
  if(firstSuccess<(populated?12:16)){std::fprintf(stderr,"first success %d populated=%d\n",firstSuccess,populated);return 2;}
 }
 const auto path=std::filesystem::temp_directory_path()/("block-control-boundary-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dwg");
 struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove(p,e);}} cleanup{path};{std::ofstream o(path);o<<"sentinel";}
 dwgRW output(path.string().c_str());FileWriter w(output);if(output.write(&w,DRW::AC1027,true)||read(path)!="sentinel")return 3;
 w.fail=false;if(!output.write(&w,DRW::AC1027,true)||!w.ok)return 4;
}
