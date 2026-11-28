// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include "libdwgr.h"
#include "intern/dwgbufferw.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
namespace {
struct Adapter : LC3CompatCheck {
 dxfRW* writer=nullptr;std::vector<DRW_Light> lights;bool ok=true;
 void addLight(const DRW_Light& v) override { lights.push_back(v); }
 void writeEntities() override { for(auto& v:lights) if(!writer->writeLight(&v))ok=false; }
 bool finalizeDxfWrite() override {return ok;}
};
struct LightProbe : DRW_Light {using DRW_Light::encodeDwg;};
struct DwgAdapter : LC3CompatCheck {
 dwgRW& output;DRW_Light light;bool registered=false,written=false;
 explicit DwgAdapter(dwgRW& value):output(value){}
 void writeDwgClasses() override {registered=output.registerDwgEntityClassInstance(DRW_Light::kDwgClassNum,0xF600u);}
 void writeEntities() override {light.handle=0xF600u;written=output.writeLight(&light);}
 bool finalizeDwgWrite() override {return true;}
};
std::string bytes(const std::filesystem::path& p) {std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
bool read(const std::filesystem::path& p,Adapter& a) {const auto s=p.string();dxfRW r(s.c_str());a.lights.clear();return r.read(&a,false);}
}
int main(){
 const auto dir=std::filesystem::temp_directory_path()/("light421-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(dir);
 struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} cleanup{dir};
 Adapter a;
 for(int rgb:{-1,0,0x123456,0xffffff}) {DRW_Light l;l.m_type=2;l.m_name="LIGHT";l.m_status=true;l.m_color=7;l.m_colorRgb=rgb;a.lights.push_back(l);}
 for(bool binary:{false,true}) {
  const auto path=dir/(binary?"binary.dxf":"ascii.dxf");
  const auto pathText=path.string();dxfRW codec(pathText.c_str());a.writer=&codec;
  const auto attempt=[&](){a.ok=true;return codec.write(&a,DRW::AC1027,binary);};
  if(!attempt()){std::cerr<<"write\n";return 1;}
  Adapter b;if(!read(path,b)||b.lights.size()!=4u){std::cerr<<"read\n";return 1;}
  for(std::size_t i=0;i<4u;++i) if(b.lights[i].m_color!=7u || b.lights[i].m_colorRgb!=a.lights[i].m_colorRgb || b.lights[i].m_colorDwgMetadataPresent){std::cerr<<"color "<<i<<"\n";return 1;}
  const auto good=bytes(path); a.lights[2].m_colorRgb=0x1000000;
  if(attempt() || bytes(path)!=good){std::cerr<<"invalid atomic\n";return 1;}
  a.lights[2].m_colorRgb=0x123456;
  if(!attempt()){std::cerr<<"retry\n";return 1;}
  a.lights[2].m_colorDwgMetadataPresent=true;
  if(attempt()){std::cerr<<"DWG CMC was admitted\n";return 1;}
  a.lights[2].m_colorDwgMetadataPresent=false;
  a.lights[2].m_colorMethod=0xC2u;
  if(attempt()){std::cerr<<"unqualified color method admitted\n";return 1;}
  a.lights[2].m_colorMethod=0u;
  a.lights[2].m_colorRgb=-2;
  if(attempt()){std::cerr<<"invalid negative RGB admitted\n";return 1;}
  a.lights[2].m_colorRgb=0x123456;
  if(!attempt()){std::cerr<<"metadata retry\n";return 1;}
 }
 const auto duplicate=dir/"duplicate.dxf"; const auto original=bytes(dir/"ascii.dxf");
 const auto at=original.find("421\n"); if(at==std::string::npos){std::cerr<<"421 tag absent\n";return 1;}
 auto malformed=original;malformed.insert(at,"421\n0\n");{std::ofstream f(duplicate);f<<malformed;}
 Adapter bad;if(read(duplicate,bad)){std::cerr<<"duplicate accepted\n";return 1;}
 const auto valueStart=original.find('\n',at)+1u;const auto valueEnd=original.find('\n',valueStart);
 if(valueStart==0u || valueEnd==std::string::npos)return 1;
 for(const auto* value:{"-1","16777216"}) {
  auto invalid=original;invalid.replace(valueStart,valueEnd-valueStart,value);
  {std::ofstream f(duplicate);f<<invalid;}
  if(read(duplicate,bad)){std::cerr<<"out of range source accepted\n";return 1;}
 }
 Adapter imported;if(!read(dir/"ascii.dxf",imported)||imported.lights.size()!=4u)return 1;
 LightProbe probe;static_cast<DRW_Light&>(probe)=imported.lights[2];dwgBufferW body;
 if(probe.encodeDwg(DRW::AC1027,&body)||!body.data().empty()){std::cerr<<"DXF RGB+ACI admitted to DWG\n";return 1;}
 probe.m_colorRgb=0;if(probe.encodeDwg(DRW::AC1027,&body)||!body.data().empty()){std::cerr<<"black RGB+ACI admitted to DWG\n";return 1;}
 const auto dwgPath=dir/"rejected.dwg";{std::ofstream f(dwgPath,std::ios::binary);f<<"prior destination";}
 dwgRW dwg(dwgPath.string().c_str());DwgAdapter output(dwg);output.light=imported.lights[2];output.light.parentHandle=DRW::NoHandle;
 const bool published=dwg.write(&output,DRW::AC1027,true);
 if(published || !output.registered || output.written || bytes(dwgPath)!="prior destination") {std::cerr<<"DWG ignored-failure published="<<published<<" registered="<<output.registered<<" written="<<output.written<<"\n";return 1;}
 output.light.m_colorRgb=-1;output.registered=output.written=false;
 const bool retried=dwg.write(&output,DRW::AC1027,true);
 if(!retried||!output.registered||!output.written){std::cerr<<"DWG retry "<<retried<<" "<<output.registered<<" "<<output.written<<"\n";return 1;}
 return 0;
}
