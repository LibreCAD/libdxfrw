#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
const std::array<std::uint8_t,16> begin = {{0xcf,0x7b,0x1f,0x23,0xfd,0xde,0x38,0xa9,0x5f,0x7c,0x68,0xb8,0x4e,0x6d,0x33,0x5f}};
const std::array<std::uint8_t,16> end = {{0x30,0x84,0xe0,0xdc,0x02,0x21,0xc7,0x56,0xa0,0x83,0x97,0x47,0xb1,0x92,0xcc,0xa0}};
std::uint32_t get32(const std::uint8_t* p) {
 return std::uint32_t(p[0]) | (std::uint32_t(p[1])<<8) | (std::uint32_t(p[2])<<16) | (std::uint32_t(p[3])<<24);
}
std::uint16_t crc16(const std::uint8_t* p,std::size_t n) {
 std::uint16_t c=0xc0c1;
 for(std::size_t i=0;i<n;++i) { c^=p[i]; for(unsigned j=0;j<8;++j) c=static_cast<std::uint16_t>((c>>1)^((c&1)?0xa001:0)); }
 return c;
}
std::vector<std::uint8_t> read(const std::filesystem::path& p) {
 std::ifstream f(p,std::ios::binary); return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
}
bool envelope(const std::vector<std::uint8_t>& b,bool modern) {
 const auto s=std::search(b.begin(),b.end(),begin.begin(),begin.end());
 if(s==b.end())return false;
 const auto after=s+16;
 if(std::distance(after,b.end())<8)return false;
 const auto e=std::search(after,b.end(),end.begin(),end.end());
 if(e==b.end() || std::distance(after,e)<6)return false;
 const auto size=get32(&*after);
 const auto crcPos=e-2;
 if(static_cast<std::size_t>(std::distance(after,crcPos)) != size+(modern?8u:4u))return false;
 if(modern && get32(&*(after+4))!=0)return false;
 const auto expected=static_cast<std::uint16_t>(crcPos[0] | (std::uint16_t(crcPos[1])<<8));
 return crc16(&*after,static_cast<std::size_t>(std::distance(after,crcPos)))==expected;
}
class Writer final:public LC3CompatCheck {
public:
 explicit Writer(dwgRW& w):out(w){}
 void writeEntities() override {
  if(fail) {
   DRW_Polyline p;p.addVertex(DRW_Vertex(1,2,0,0));p.addVertex(DRW_Vertex(3,4,0,0));
   p.vertlist[0]->handle=p.vertlist[1]->handle=0x90; ok=out.writePolyline(&p);
  } else { DRW_Line p;p.basePoint={1,2,3};p.secPoint={4,5,6};ok=out.writeLine(&p); }
 }
 bool finalizeDwgWrite() override{return ok;}
 dwgRW& out;bool fail=true,ok=false;
};
class Reader final:public LC3CompatCheck {
public:
 void addLine(const DRW_Line& p) override { handle=p.handle;good=p.basePoint.x==1&&p.basePoint.y==2&&p.basePoint.z==3&&p.secPoint.x==4&&p.secPoint.y==5&&p.secPoint.z==6; }
 void addHeader(const DRW_Header* h) override {
  handseed=h->getHandSeed();
  const std::array<const char*,4> names{{"CLAYER","TEXTSTYLE","CELTYPE","DIMSTYLE"}};
  for(std::size_t i=0;i<names.size();++i) {
   const auto it=h->vars.find(names[i]);
   if(it!=h->vars.end()&&it->second&&it->second->type()==DRW_Variant::STRING&&it->second->content.s)
    current[i]=*it->second->content.s;
  }
 }
 bool good=false;std::uint32_t handseed=0,handle=0;std::array<std::string,4> current{};
};
}
int main(int argc,char** argv) {
 if(argc==2)return envelope(read(argv[1]),true)?0:1;
 if(argc!=1)return 2;
 const auto root=std::filesystem::temp_directory_path()/("modern-header-size-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(root);
 struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} cleanup{root};
 for(auto v:{DRW::AC1018,DRW::AC1024,DRW::AC1027,DRW::AC1032}) {
  const auto path=root/(std::to_string(static_cast<int>(v))+".dwg");const std::string sentinel="prior destination";
  {std::ofstream f(path,std::ios::binary);f<<sentinel;}
  dwgRW output(path.string().c_str());Writer w(output);
  if(output.write(&w,v,true)||read(path)!=std::vector<std::uint8_t>(sentinel.begin(),sentinel.end())) { std::cerr<<"refusal "<<static_cast<int>(v)<<'\n';return 1; }
  w.fail=false;if(!output.write(&w,v,true)||!w.ok) { std::cerr<<"retry "<<static_cast<int>(v)<<'\n';return 1; }
  auto bytes=read(path);if(!envelope(bytes,v>=DRW::AC1027)) { std::cerr<<"envelope "<<static_cast<int>(v)<<'\n';return 1; }
  auto wrong=bytes; const auto start=std::search(wrong.begin(),wrong.end(),begin.begin(),begin.end());if(start==wrong.end())return 1;
  *(start+16)^=1;if(envelope(wrong,v>=DRW::AC1027))return 1;
  dwgRW input(path.string().c_str());Reader r;if(!input.read(&r,false)||!r.good || r.handle==0u || r.handseed<=r.handle
      || r.current!=std::array<std::string,4>{{"0","STANDARD","BYLAYER","STANDARD"}}) {
   std::cerr<<"native readback failed "<<static_cast<int>(v)<<" seed="<<r.handseed<<" handle="<<r.handle<<" current="<<r.current[0]<<','<<r.current[1]<<','<<r.current[2]<<','<<r.current[3]<<'\n';return 1;
  }
  unsigned compatibility=0,unclaimed=0,headerCrc=0;
  for(const auto& d:input.getIntegrityDiagnostics()) {
   if(d.kind==DwgIntegrityCheckKind::HeaderSectionCompatibility)++compatibility;
   if(d.kind==DwgIntegrityCheckKind::HeaderSectionUnclaimedData)++unclaimed;
   if(d.kind==DwgIntegrityCheckKind::HeaderSectionCrc)++headerCrc;
  }
  if(compatibility!=(v>=DRW::AC1027?1u:0u)||unclaimed||headerCrc) {
   std::cerr<<"unexpected header diagnostics "<<static_cast<int>(v)<<'\n';return 1;
  }
 }
}
