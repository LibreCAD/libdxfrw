// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include "libdwgr.h"
#include "intern/dxfwriter.h"
#include "intern/dwgbufferw.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>
namespace {
struct Reader:LC3CompatCheck,DRW_DxfSparseMaterialRawPolicy {
 bool enabled=true;
 bool wantsDxfSparseMaterialRawCapture() const noexcept override{return enabled;}
 std::vector<DRW_Material> materials;std::vector<DRW_RawDxfObject> raw;
 void addMaterial(const DRW_Material& v)override{materials.push_back(v);}
 void addRawDxfObject(const DRW_RawDxfObject& v)override{if(v.name=="MATERIAL")raw.push_back(v);}
};
struct LegacyReader:LC3CompatCheck {
 unsigned materialCallbacks=0,rawCallbacks=0;
 void addMaterial(const DRW_Material&)override{++materialCallbacks;}
 void addRawDxfObject(const DRW_RawDxfObject& v)override{if(v.name=="MATERIAL")++rawCallbacks;}
};
struct DxfWriter:LC3CompatCheck {
 dxfRW& out;DRW_Material value;DRW_RawDxfObject raw;bool useRaw=false,ok=false;
 explicit DxfWriter(dxfRW& w):out(w){}
 void writeObjects()override{ok=useRaw?out.writeRawDxfObject(&raw):out.writeMaterial(&value);}
 bool finalizeDxfWrite()override{return ok;}
};
struct DwgWriter:LC3CompatCheck {
 dwgRW& out;DRW_Material value;bool registered=false,written=false;
 explicit DwgWriter(dwgRW& w):out(w){}
 void writeDwgClasses()override{registered=out.registerMaterialObjectClass(&value);}
 void writeObjects()override{written=out.writeMaterial(&value);}
 bool finalizeDwgWrite()override{return true;}
};
struct DwgProbe:DRW_Material {using DRW_Material::encodeDwg;};
std::string bytes(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
bool source(const std::filesystem::path& path,bool binary,int mode,const char* name){
 std::ofstream f(path,std::ios::binary|std::ios::trunc);if(!f)return false;
 if(binary){const char signature[]="AutoCAD Binary DXF\r\n\x1a";f.write(signature,sizeof(signature));}
 std::unique_ptr<dxfWriter> w=binary?std::unique_ptr<dxfWriter>(new dxfWriterBinary(&f)):std::unique_ptr<dxfWriter>(new dxfWriterAscii(&f));
 const auto s=[&](int c,const char* v){return w->writeString(c,v);};
 const auto i=[&](int c,int v){return w->writeInt32(c,v);};
 const auto h=[&](int c,int v){return w->writeInt16(c,v);};
 if(mode==13){
  if(!s(0,"SECTION")||!s(2,"HEADER")||!s(9,"$ACADVER")||!s(1,"AC1032")||!s(0,"ENDSEC")
   ||!s(0,"SECTION")||!s(2,"OBJECTS")||!s(0,"MATERIAL")||!s(5,"97")
   ||!s(102,"{ACAD_XDICTIONARY")||!s(360,"188")||!s(102,"}")
   ||!s(102,"{ACAD_REACTORS")||!s(330,"72")||!s(102,"}")
   ||!s(330,"72")||!s(100,"AcDbMaterial")||!s(1,"ByBlock")||!i(94,63)
   ||!s(0,"MATERIAL")||!s(5,"96")||!s(330,"72")
   ||!s(100,"AcDbMaterial")||!s(1,"Global")||!s(2,"partial AC1032 material")
   ||!i(94,63)||!s(1001,"ACAD")||!s(1000,"raw-tail")
   ||!s(0,"ENDSEC")||!s(0,"EOF"))return false;
  return !w->hasWriteError();
 }
 if(!s(0,"SECTION")||!s(2,"HEADER")||!s(9,"$ACADVER")||!s(1,mode==6?"AC1024":"AC1027")||!s(0,"ENDSEC")
 ||!s(0,"SECTION")||!s(2,"OBJECTS")||!s(0,"MATERIAL")||!s(5,"F000")||!s(102,"{ACAD_REACTORS")
 ||!s(330,"E")||!s(102,"}")||!s(330,mode==9?"D":"E")||!s(100,"AcDbMaterial")||!s(1,mode==8?"OTHER":name)
 ||!h(71,1)||!i(91,mode==7?-1:-1023410177)||!h(72,1))return false;
 if(mode!=11 && !h(273,mode==2?0:1))return false;
 if(mode==3 && !h(71,1))return false;
 if(mode==4 && !s(2,"extra"))return false;
 if(!i(94,mode==1?62:63))return false;
 if(mode==5 && !s(2,"late"))return false;
 if(mode==12 && !i(94,63))return false;
 if(!s(0,"MATERIAL")||!s(5,"F001")||!s(330,"E")||!s(100,"AcDbMaterial")||!s(1,"following")
 ||!s(0,"ENDSEC")||!s(0,"EOF"))return false;
 return !w->hasWriteError();
}
}
int main(){
 const auto dir=std::filesystem::temp_directory_path()/("sparse-material-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(dir);
 struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{dir};
 for(bool binary:{false,true}){
  const auto inputPath=dir/(binary?"in-binary.dxf":"in-ascii.dxf");const auto inputName=inputPath.string();dxfRW input(inputName.c_str());
  Reader r;
  for(int mode:{1,2,3,4,5,6,7,8,9,11,12,0}){
   if(!source(inputPath,binary,mode,"ByBlock"))return 1;r.materials.clear();r.raw.clear();const bool ok=input.read(&r,false);
   if(mode!=0){if(ok||!r.materials.empty()){std::cerr<<"invalid accepted "<<mode<<'\n';return 1;}continue;}
   if(!ok||r.materials.size()!=2u||r.raw.size()!=2u){std::cerr<<"input missing\n";return 1;}
  }
  for(const char* name:{"ByLayer","Global"}){if(!source(inputPath,binary,0,name))return 1;Reader other;if(!input.read(&other,false)||other.materials.size()!=2u||!other.materials[0].requiresRawDxfMaterialReplay())return 1;}
  if(!source(inputPath,binary,0,"ByBlock"))return 1;
  LegacyReader legacy;if(input.read(&legacy,false)||legacy.materialCallbacks||legacy.rawCallbacks){std::cerr<<"legacy consumer admitted\n";return 1;}
  Reader opt;opt.enabled=false;if(input.read(&opt,false)||!opt.materials.empty()||!opt.raw.empty()){std::cerr<<"disabled facet admitted\n";return 1;}
  opt.enabled=true;if(!input.read(&opt,false)||opt.materials.size()!=2u||opt.raw.size()!=2u||!opt.materials[0].requiresRawDxfMaterialReplay()){std::cerr<<"facet retry failed\n";return 1;}
  const auto original=r.materials[0];
  if(!original.requiresRawDxfMaterialReplay()||original.hasDxfMaterialImplicitTail()
    ||!original.m_hasDiffuseColorMethod||original.m_diffuseColorMethod!=1u
    ||!original.m_hasDiffuseColorValue||original.m_diffuseColorValue!=0xC2FFFFFFu
    ||!original.m_hasDiffuseMapSource||original.m_diffuseMapSource!=1u
    ||original.m_hasRefractionMapSource||!original.m_hasChannelFlags||original.m_channelFlags!=63
    ||r.materials[1].requiresRawDxfMaterialReplay())return 1;
  DRW_Material copied=original;DRW_Material moved=std::move(copied);if(!moved.requiresRawDxfMaterialReplay())return 1;
  DRW_Material assigned;assigned=moved;if(!assigned.requiresRawDxfMaterialReplay())return 1;
  assigned.m_hasChannelFlags=false;if(!assigned.requiresRawDxfMaterialReplay())return 1;
  moved.reset();if(moved.requiresRawDxfMaterialReplay()||!original.requiresRawDxfMaterialReplay())return 1;
  DwgProbe probe;static_cast<DRW_Material&>(probe)=original;dwgBufferW data,strings,handles;
  if(probe.encodeDwg(DRW::AC1027,&data,&strings,&handles)||!data.data().empty()||!strings.data().empty()||!handles.data().empty())return 1;
  const auto dest=dir/(binary?"out-binary.dxf":"out-ascii.dxf");{std::ofstream f(dest);f<<"prior";}
  const auto destName=dest.string();dxfRW writer(destName.c_str());DxfWriter adapter(writer);adapter.value=original;adapter.raw=r.raw[0];
  if(writer.write(&adapter,DRW::AC1027,binary)||adapter.ok||bytes(dest)!="prior"){std::cerr<<"typed output was not refused\n";return 1;}
  adapter.useRaw=true;adapter.ok=false;if(!writer.write(&adapter,DRW::AC1027,binary)||!adapter.ok){std::cerr<<"raw retry failed\n";return 1;}
  Reader reread;dxfRW check(destName.c_str());if(!check.read(&reread,false)||reread.materials.size()!=1u||!reread.materials[0].requiresRawDxfMaterialReplay())return 1;
  std::vector<int> actual;for(const auto& g:reread.raw[0].groups)actual.push_back(g.code());
  const std::vector<int> expected={5,102,330,102,330,100,1,71,91,72,273,94};if(actual!=expected
    ||reread.raw[0].groups[7].i_val()!=1||reread.raw[0].groups[9].i_val()!=1
    ||reread.raw[0].groups[10].i_val()!=1||reread.raw[0].groups[11].i_val()!=63
    ||reread.materials[0].m_diffuseColorValue!=0xC2FFFFFFu)return 1;
  const auto dwgPath=dir/"out.dwg";{std::ofstream f(dwgPath);f<<"prior dwg";}
  const auto dwgName=dwgPath.string();dwgRW dwg(dwgName.c_str());DwgWriter dwgAdapter(dwg);dwgAdapter.value=original;
  if(dwg.write(&dwgAdapter,DRW::AC1027,true)||!dwgAdapter.registered||dwgAdapter.written||bytes(dwgPath)!="prior dwg")return 1;
  // A fresh manually authored MATERIAL has no qualified modern DWG body.
  // Exercise same-writer correction using the supported legacy codec instead.
  dwgAdapter.registered=dwgAdapter.written=false;
  if(dwg.write(&dwgAdapter,DRW::AC1018,true)||!dwgAdapter.registered||dwgAdapter.written||bytes(dwgPath)!="prior dwg")return 1;
  dwgAdapter.value.reset();dwgAdapter.value.handle=0xF000u;dwgAdapter.value.m_name="corrected";
  dwgAdapter.registered=dwgAdapter.written=false;
  if(!dwg.write(&dwgAdapter,DRW::AC1018,true)||!dwgAdapter.registered||!dwgAdapter.written){std::cerr<<"DWG retry failed\n";return 1;}
  if(!source(inputPath,binary,13,"ByBlock"))return 33;
  Reader ac1032;dxfRW ac1032Reader(inputName.c_str());
  if(!ac1032Reader.read(&ac1032,false)||ac1032.materials.size()!=2u
     ||ac1032.raw.size()!=2u||!ac1032.materials.front().requiresRawDxfMaterialReplay()
     ||!ac1032.materials.front().m_hasChannelFlags
     ||ac1032.materials.front().m_channelFlags!=63
     ||!ac1032.materials.back().requiresRawDxfMaterialReplay()
     ||ac1032.raw.front().groups.size()!=11u
     ||ac1032.raw.back().groups.size()!=8u
     ||ac1032.raw.back().groups.back().code()!=1000
     ||std::string(ac1032.raw.back().groups.back().c_str())!="raw-tail")return 34;
  LegacyReader ac1032Legacy;
  if(ac1032Reader.read(&ac1032Legacy,false)||ac1032Legacy.materialCallbacks
     ||ac1032Legacy.rawCallbacks)return 42;
  Reader ac1032Disabled;ac1032Disabled.enabled=false;
  if(ac1032Reader.read(&ac1032Disabled,false)||!ac1032Disabled.materials.empty()
     ||!ac1032Disabled.raw.empty())return 43;
  ac1032Disabled.enabled=true;
  if(!ac1032Reader.read(&ac1032Disabled,false)||ac1032Disabled.materials.size()!=2u
     ||ac1032Disabled.raw.size()!=2u
     ||!ac1032Disabled.materials.front().requiresRawDxfMaterialReplay())return 44;
  const std::vector<int> ac1032Expected={5,102,360,102,102,330,102,330,100,1,94};
  std::vector<int> ac1032Observed;
  for(const auto& group:ac1032.raw.front().groups)ac1032Observed.push_back(group.code());
  if(ac1032Observed!=ac1032Expected||ac1032.raw.front().groups.back().i_val()!=63)return 35;
  const auto ac1032Dest=dir/(binary?"ac1032-out-binary.dxf":"ac1032-out-ascii.dxf");
  {std::ofstream f(ac1032Dest);f<<"sentinel";}
  const auto ac1032DestName=ac1032Dest.string();dxfRW ac1032Writer(ac1032DestName.c_str());
  DxfWriter ac1032Adapter(ac1032Writer);ac1032Adapter.value=ac1032.materials.front();
  ac1032Adapter.raw=ac1032.raw.front();
  if(ac1032Writer.write(&ac1032Adapter,DRW::AC1032,binary)||ac1032Adapter.ok||bytes(ac1032Dest)!="sentinel")return 36;
  ac1032Adapter.useRaw=true;
  if(!ac1032Writer.write(&ac1032Adapter,DRW::AC1032,binary)||!ac1032Adapter.ok)return 37;
  Reader ac1032Readback;dxfRW ac1032Check(ac1032DestName.c_str());
  if(!ac1032Check.read(&ac1032Readback,false)||ac1032Readback.materials.size()!=1u
     ||ac1032Readback.raw.size()!=1u
     ||!ac1032Readback.materials.front().requiresRawDxfMaterialReplay()
     ||ac1032Readback.raw.front().groups.size()!=11u)return 38;
  const auto tailDest=dir/(binary?"ac1032-tail-binary.dxf":"ac1032-tail-ascii.dxf");
  {std::ofstream f(tailDest);f<<"sentinel";}
  const auto tailDestName=tailDest.string();dxfRW tailWriter(tailDestName.c_str());
  DxfWriter tailAdapter(tailWriter);tailAdapter.value=ac1032.materials.back();
  tailAdapter.raw=ac1032.raw.back();
  if(tailWriter.write(&tailAdapter,DRW::AC1032,binary)||tailAdapter.ok||bytes(tailDest)!="sentinel")return 39;
  tailAdapter.useRaw=true;
  if(!tailWriter.write(&tailAdapter,DRW::AC1032,binary)||!tailAdapter.ok)return 40;
  Reader tailReadback;dxfRW tailCheck(tailDestName.c_str());
  if(!tailCheck.read(&tailReadback,false)||tailReadback.materials.size()!=1u
     ||tailReadback.raw.size()!=1u
     ||!tailReadback.materials.front().requiresRawDxfMaterialReplay()
     ||tailReadback.raw.front().groups.size()!=8u
     ||tailReadback.raw.front().groups.back().code()!=1000
     ||std::string(tailReadback.raw.front().groups.back().c_str())!="raw-tail")return 41;
 }
 return 0;
}
