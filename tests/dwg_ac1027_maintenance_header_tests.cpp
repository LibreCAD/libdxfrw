#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <vector>

namespace {
std::vector<std::uint8_t> read(const std::filesystem::path& p) {
 std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};
}
class Writer final:public LC3CompatCheck {
public:explicit Writer(dwgRW& w):out(w){}
 void writeEntities() override {
  if(fail){DRW_Polyline p;p.addVertex(DRW_Vertex(1,2,0,0));p.addVertex(DRW_Vertex(3,4,0,0));p.vertlist[0]->handle=p.vertlist[1]->handle=0x90;ok=out.writePolyline(&p);}
  else{DRW_Line l;l.basePoint={1,2,3};l.secPoint={4,5,6};ok=out.writeLine(&l);}
 }
 bool finalizeDwgWrite() override{return ok;}
 dwgRW& out;bool fail=true,ok=false;
};
class Reader final:public LC3CompatCheck {
public:void addLine(const DRW_Line& l) override{++count;good=l.basePoint.x==1&&l.basePoint.y==2&&l.basePoint.z==3&&l.secPoint.x==4&&l.secPoint.y==5&&l.secPoint.z==6;}
 int count=0;bool good=false;
};
bool aux(const std::vector<std::uint8_t>& b,std::uint8_t rawVersion,std::uint8_t expected) {
 const std::vector<std::uint8_t> prefix{0xff,0x77,0x01,rawVersion,0x00};
 const auto p=std::search(b.begin(),b.end(),prefix.begin(),prefix.end());
 if(p==b.end() || std::distance(p,b.end())<31 || std::search(p+1,b.end(),prefix.begin(),prefix.end())!=b.end())return false;
 for(auto pos:{5,25,29})if(p[pos]!=expected || p[pos+1]!=0)return false;
 return p[23]==rawVersion && p[24]==0 && p[27]==rawVersion && p[28]==0;
}
}
int main(){
 const auto root=std::filesystem::temp_directory_path()/("ac1027-maintenance-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directory(root);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} cleanup{root};
 for(const auto& spec : {std::pair<DRW::Version,std::uint8_t>{DRW::AC1018,25}, {DRW::AC1024,29}, {DRW::AC1027,31}, {DRW::AC1032,33}}){
  const auto path=root/(std::to_string(static_cast<int>(spec.first))+".dwg");{std::ofstream f(path);f<<"sentinel";}
  dwgRW w(path.string().c_str());Writer output(w);if(w.write(&output,spec.first,true)||read(path)!=std::vector<std::uint8_t>({'s','e','n','t','i','n','e','l'}))return 1;
  output.fail=false;if(!w.write(&output,spec.first,true)||!output.ok)return 2;
  const auto data=read(path);const bool targeted=spec.first==DRW::AC1027;
  if(data.size()<0x100 || data[11]!=(targeted?8u:0u) || data[12]!=0 || data[17]!=0 || data[18]!=(targeted?6u:0u) || !aux(data,spec.second,targeted?8u:0u))return 3;
  dwgRW input(path.string().c_str());Reader decoded;if(!input.read(&decoded,false)||decoded.count!=1||!decoded.good)return 4;
 }
 return 0;
}
