// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include "libdwgr.h"
#include "intern/dxfwriter.h"
#include "intern/dwgbufferw.h"
#include <algorithm>
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
 std::vector<DRW_Material> materials;std::vector<DRW_RawDxfObject> raw,rawDictionaries;
 std::vector<DRW_Dictionary> dictionaries;DRW_Dictionary root;
 std::vector<DRW_Layer> layers;std::string currentMaterial;
 void addDictionary(const DRW_Dictionary& d)override{dictionaries.push_back(d);}
 void addNamedObjectDictionary(const DRW_Dictionary& d)override{root=d;}
 void addLayer(const DRW_Layer& l)override{layers.push_back(l);}
 void addHeader(const DRW_Header* h)override{
  auto it=h->vars.find("$CMATERIAL");
  if(it!=h->vars.end()&&it->second&&it->second->type()==DRW_Variant::STRING&&it->second->c_str())
   currentMaterial=it->second->c_str();
 }
 void addMaterial(const DRW_Material& v)override{materials.push_back(v);}
 void addRawDxfObject(const DRW_RawDxfObject& v)override{
  if(v.name=="MATERIAL")raw.push_back(v);
  else if(v.name=="DICTIONARY")rawDictionaries.push_back(v);
 }
};
struct LegacyReader:LC3CompatCheck {
 unsigned materialCallbacks=0,rawCallbacks=0;
 void addMaterial(const DRW_Material&)override{++materialCallbacks;}
 void addRawDxfObject(const DRW_RawDxfObject& v)override{if(v.name=="MATERIAL")++rawCallbacks;}
};
struct HeaderSourceReader:LC3CompatCheck,DRW_DxfHeaderMaterialSourceHandler {
 bool enabled=true, headerAfterSource=false;
 std::vector<DRW_DxfHeaderMaterialSource> captured;
 bool wantsDxfHeaderMaterialSource()const override{return enabled;}
 void addDxfHeaderMaterialSource(const DRW_DxfHeaderMaterialSource& value)override{captured.push_back(value);}
 void addHeader(const DRW_Header*)override{headerAfterSource=!captured.empty();}
};
struct DxfWriter:LC3CompatCheck {
 dxfRW& out;DRW_Material value;DRW_RawDxfObject raw;bool useRaw=false,ok=false;
 explicit DxfWriter(dxfRW& w):out(w){}
 void writeObjects()override{ok=useRaw?out.writeRawDxfObject(&raw):out.writeMaterial(&value);}
 bool finalizeDxfWrite()override{return ok;}
};
struct MaterialGraphWriter:LC3CompatCheck {
 dxfRW& out;std::vector<DRW_RawDxfObject> raw;DRW_Dictionary dict;bool ok=true,extraLayer=false,headerEnabled=true;
 std::string headerTarget="20";
 explicit MaterialGraphWriter(dxfRW& w):out(w){}
 void writeHeader(DRW_Header& h)override{if(headerEnabled)h.addStr("$CMATERIAL",headerTarget,347);}
 void writeLayers()override{DRW_Layer layer;layer.name="RAWTEST";layer.handle=0x90;layer.materialHandle.ref=0x21;ok=out.writeLayer(&layer)&&ok;
  if(extraLayer){DRW_Layer other;other.name="OTHER";other.handle=0x91;other.materialHandle.ref=0x20;ok=out.writeLayer(&other)&&ok;}}
 void writeObjects()override{ok=out.writeDictionary(&dict)&&ok;for(auto& object:raw)ok=out.writeRawDxfObject(&object)&&ok;}
 bool finalizeDxfWrite()override{return ok;}
};
struct RootEntryWriter:LC3CompatCheck {
 dxfRW& out;bool ok=false;
 explicit RootEntryWriter(dxfRW& writer):out(writer){}
 void writeObjects()override{DRW_Dictionary child;child.handle=0xE;child.parentHandle=0xC;ok=out.writeDictionary(&child);}
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
bool physicalHeaderSource(const std::filesystem::path& path,bool binary,int mode){
 {
  std::ofstream f(path,std::ios::binary|std::ios::trunc);if(!f)return false;
  if(binary){const char signature[]="AutoCAD Binary DXF\r\n\x1a";f.write(signature,sizeof(signature));}
  std::unique_ptr<dxfWriter> w=binary?std::unique_ptr<dxfWriter>(new dxfWriterBinary(&f)):std::unique_ptr<dxfWriter>(new dxfWriterAscii(&f));
  const auto s=[&](int code,const char* text){return w->writeString(code,text);};
  if(!s(0,"SECTION")||!s(2,"HEADER")||!s(9,"$ACADVER")||!s(1,"AC1024"))return false;
  if(mode!=0){
   if(!s(9,mode==10?"CMATERIAL":"$CMATERIAL"))return false;
   if(mode==7&&!s(999,"between records"))return false;
   if(mode!=3&&!s(347,mode==4?"0":mode==5?"00000000000000A1":mode==8?"10000000000000000":"A1"))return false;
   if(mode==2&&(!s(9,"$CMATERIAL")||!s(347,"A2")))return false;
   if(mode==6&&!s(347,"A2"))return false;
   if(mode==9&&(!s(9,"CMATERIAL")||!s(347,"A1")))return false;
  }
  if(!s(0,"ENDSEC"))return false;
  if(mode==11&&(!s(0,"SECTION")||!s(2,"HEADER")||!s(9,"$CMATERIAL")||!s(347,"A2")||!s(0,"ENDSEC")))return false;
  if(!s(0,"EOF")||w->hasWriteError())return false;
 }
 if(mode==12){
  const auto original=bytes(path);std::string crlf;crlf.reserve(original.size()*2u);
  for(char c:original){if(c=='\n')crlf.push_back('\r');crlf.push_back(c);}
  std::ofstream f(path,std::ios::binary|std::ios::trunc);f.write(crlf.data(),static_cast<std::streamsize>(crlf.size()));
  if(!f)return false;
 }
 HeaderSourceReader sink;dxfRW input(path.string().c_str());
 if(!input.read(&sink,false))return mode==8&&sink.captured.empty();
 if(sink.captured.size()!=(mode==11?2u:1u)||!sink.headerAfterSource)return false;
 if(mode==11&&(sink.captured[1].status!=DRW_DxfHeaderMaterialSource::Status::Complete||
    sink.captured[1].handleLength!=2u||sink.captured[1].handleLexeme[1]!='2'))return false;
 const auto& source=sink.captured.front();
 const bool malformed=mode==2||mode==3||mode==6||mode==8||mode==9;
 if((source.status==DRW_DxfHeaderMaterialSource::Status::Malformed)!=malformed||
    source.version!=DRW::AC1024||source.binary!=binary||source.present!=(mode!=0))return false;
 if(!malformed&&mode!=0){
  const std::string expected=mode==4?"0":mode==5?"00000000000000A1":"A1";
  if(source.dollarName!=(mode!=10)||std::string(source.handleLexeme.data(),source.handleLength)!=expected)return false;
  const auto data=bytes(path);
  const auto valid=[&](const DRW_DxfPhysicalRecordSpan& span){return span.beginOffset<span.endOffset&&span.endOffset<=data.size();};
  if(!valid(source.sectionName)||!valid(source.variableName)||!valid(source.value)||!valid(source.endMarker)||
     source.sectionName.endOffset>source.variableName.beginOffset||source.variableName.endOffset>source.value.beginOffset||
     source.value.endOffset>source.endMarker.beginOffset)return false;
  const auto variable=data.substr(source.variableName.beginOffset,source.variableName.endOffset-source.variableName.beginOffset);
  const auto value=data.substr(source.value.beginOffset,source.value.endOffset-source.value.beginOffset);
  if(variable.find(mode==10?"CMATERIAL":"$CMATERIAL")==std::string::npos||value.find(expected)==std::string::npos)return false;
  if(binary&&(static_cast<unsigned char>(variable[0])!=9||static_cast<unsigned char>(variable[1])!=0||
              static_cast<unsigned char>(value[0])!=0x5b||static_cast<unsigned char>(value[1])!=1))return false;
  if((source.value.ordinal>source.variableName.ordinal+1u)!=(mode==7))return false;
 }
 HeaderSourceReader disabled;disabled.enabled=false;dxfRW again(path.string().c_str());
 return again.read(&disabled,false)&&disabled.captured.empty();
}
bool rootEntryVersion(const std::filesystem::path& output,bool binary,DRW::Version version){
 dxfRW writer(output.string().c_str());RootEntryWriter adapter(writer);
 writer.setHandleRemap({{0xEu,0xF010u}});
 writer.setRootDictEntries({{"TEST_ENTRY","F010"}});
 int role=350;
 if(!writer.setDxfObjectsWriteInspector([&](const auto& view){
  return writer.inspectDxfRootDictionaryEntry(view,"TEST_ENTRY",0xEu,role);
 }))return false;
 if(!writer.write(&adapter,version,binary)||!adapter.ok)return false;
 {std::ofstream f(output,std::ios::binary|std::ios::trunc);f<<"sentinel";}
 role=360;
 if(writer.write(&adapter,version,binary)||bytes(output)!="sentinel")return false;
 role=350;
 return writer.write(&adapter,version,binary)&&adapter.ok;
}
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
bool minimalChannelSource(const std::filesystem::path& path,bool binary){
 std::ofstream f(path,std::ios::binary|std::ios::trunc);if(!f)return false;
 if(binary){const char signature[]="AutoCAD Binary DXF\r\n\x1a";f.write(signature,sizeof(signature));}
 std::unique_ptr<dxfWriter> w=binary?std::unique_ptr<dxfWriter>(new dxfWriterBinary(&f)):std::unique_ptr<dxfWriter>(new dxfWriterAscii(&f));
 const auto s=[&](int c,const char* v){return w->writeString(c,v);};
 return s(0,"SECTION")&&s(2,"HEADER")&&s(9,"$ACADVER")&&s(1,"AC1021")&&s(0,"ENDSEC")
  &&s(0,"SECTION")&&s(2,"OBJECTS")&&s(0,"MATERIAL")&&s(5,"F040")&&s(330,"72")
  &&s(100,"AcDbMaterial")&&s(1,"RawOnly")&&w->writeInt32(94,63)
  &&s(0,"ENDSEC")&&s(0,"EOF")&&!w->hasWriteError();
}
// A locally generated ezdxf default-material shape. Mode 1 truncates its tail;
// mode 2 changes the final mask; mode 3 adds a group beyond the bounded shape.
bool ezdxfDefaultSource(const std::filesystem::path& path,bool binary,int mode,const char* handle="1F"){
 std::ofstream f(path,std::ios::binary|std::ios::trunc);if(!f)return false;
 if(binary){const char signature[]="AutoCAD Binary DXF\r\n\x1a";f.write(signature,sizeof(signature));}
 std::unique_ptr<dxfWriter> w=binary?std::unique_ptr<dxfWriter>(new dxfWriterBinary(&f)):std::unique_ptr<dxfWriter>(new dxfWriterAscii(&f));
 const auto s=[&](int c,const char* v){return w->writeString(c,v);};
 const auto h=[&](int c,int v){return w->writeInt16(c,v);};
 const auto i=[&](int c,int v){return w->writeInt32(c,v);};
 const auto d=[&](int c,double v){return w->writeDouble(c,v);};
 if(!s(0,"SECTION")||!s(2,"HEADER")||!s(9,"$ACADVER")||!s(1,mode==4?"AC1027":"AC1024")||!s(0,"ENDSEC")
  ||!s(0,"SECTION")||!s(2,"OBJECTS")||!s(0,"MATERIAL")||!s(5,handle)
  ||!s(102,"{ACAD_REACTORS")||!s(330,"E")||!s(102,"}")||!s(330,mode==5?"D":"E")
  ||!s(100,"AcDbMaterial")||!s(1,mode==6?"Other":mode==8?"ByLayer":mode==9?"Global":"ByBlock")||!s(2,"")||!h(70,0)||!d(40,1)
  ||!h(71,1)||!d(41,1)||!i(91,-1023410177)||!d(42,1)||!h(72,1)||!s(3,"")
  ||!h(73,1)||!h(74,1)||!h(75,1)||!d(44,0.5)||!h(73,mode==7?1:0)||!d(45,1)||!d(46,1)
  ||!h(77,1)||!s(4,"")||!h(78,1)||!h(79,1)||!h(170,1)||!d(48,1)||!h(171,1)
  ||!s(6,"")||!h(172,1)||!h(173,1)||!h(174,1)||!d(140,1)||!d(141,1)
  ||!h(175,1)||!s(7,"")||!h(176,1)||!h(177,1)||!h(178,1)||!d(143,1)
  ||!h(179,1)||!s(8,"")||!h(270,1)||!h(271,1)||!h(272,1)||!d(145,1)
  ||!d(146,1)||!h(273,1)||!s(9,"")||!h(274,1)||!h(275,1)||!h(276,1)
  ||!d(42,1)||!h(72,1)||!s(3,"")||!h(73,1)||!h(74,1)||!h(75,1))return false;
 if(mode!=1&&!i(94,mode==2?62:63))return false;
 if(mode==3&&!s(1000,"extra"))return false;
 return s(0,"ENDSEC")&&s(0,"EOF")&&!w->hasWriteError();
}
}
int main(){
 const auto dir=std::filesystem::temp_directory_path()/("sparse-material-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(dir);
 struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{dir};
 for(bool binary:{false,true})for(int mode=0;mode<=12;++mode){
  if(binary&&(mode==7||mode==12))continue; // comment and CRLF controls are ASCII
  if(!physicalHeaderSource(dir/"physical-header.dxf",binary,mode)){
   std::cerr<<"physical header source failed: binary="<<binary<<" mode="<<mode<<'\n';return 95;
  }
 }
 for(bool binary:{false,true})for(const auto version:{DRW::AC1014,DRW::AC1015,DRW::AC1018,DRW::AC1021,DRW::AC1024,DRW::AC1027,DRW::AC1032})
  if(!rootEntryVersion(dir/"root-entry.dxf",binary,version)){
   std::cerr<<"root version control failed: binary="<<binary<<" version="<<static_cast<int>(version)<<'\n';return 107;
  }
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
  adapter.useRaw=true;adapter.ok=false;
  const auto frozen=adapter.raw;
  std::vector<const DRW_RawDxfObject*> expectedFrames{&frozen};
  const dxfRW::DxfObjectsWriteView outside{{},DRW::AC1027,binary,{},dxfRW::DxfCompatibilityProfile::StandaloneSafe};
  if(writer.inspectDxfRawMaterialRecords(outside,expectedFrames))return 46;
  unsigned inspected=0;
  if(!writer.setDxfObjectsWriteInspector([&](const auto& view){++inspected;return writer.inspectDxfRawMaterialRecords(view,expectedFrames);}))return 47;
  if(!writer.write(&adapter,DRW::AC1027,binary)||!adapter.ok||inspected!=1u){std::cerr<<"raw retry failed\n";return 1;}
  {std::ofstream f(dest);f<<"prior";}
  adapter.raw.groups.back()=DRW_Variant(94,62);
  if(adapter.raw.hasRawValues)adapter.raw.rawValues.back()="62";
  if(writer.write(&adapter,DRW::AC1027,binary)||bytes(dest)!="prior")return 48;
  adapter.raw=frozen;
  if(!writer.write(&adapter,DRW::AC1027,binary))return 49;
  {std::ofstream f(dest);f<<"prior";}
  expectedFrames.push_back(&frozen);
  if(writer.write(&adapter,DRW::AC1027,binary)||bytes(dest)!="prior")return 50;
  expectedFrames.pop_back();
  if(!writer.write(&adapter,DRW::AC1027,binary))return 51;
  {std::ofstream f(dest);f<<"prior";}
  auto wrongSource=frozen;wrongSource.m_version=DRW::AC1024;expectedFrames[0]=&wrongSource;
  if(writer.write(&adapter,DRW::AC1027,binary)||bytes(dest)!="prior")return 78;
  wrongSource=frozen;wrongSource.name="OTHER";
  if(writer.write(&adapter,DRW::AC1027,binary)||bytes(dest)!="prior")return 79;
  expectedFrames[0]=&frozen;
  if(!writer.write(&adapter,DRW::AC1027,binary))return 80;
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
 for(bool binary:{false,true}){
  const auto inputPath=dir/(binary?"minimal-channel-binary.dxf":"minimal-channel-ascii.dxf");
  if(!minimalChannelSource(inputPath,binary))return 62;
  const auto inputName=inputPath.string();dxfRW input(inputName.c_str());
  LegacyReader legacy;if(input.read(&legacy,false)||legacy.materialCallbacks||legacy.rawCallbacks)return 63;
  Reader disabled;disabled.enabled=false;
  if(input.read(&disabled,false)||!disabled.materials.empty()||!disabled.raw.empty())return 64;
  disabled.enabled=true;
  if(!input.read(&disabled,false)||disabled.materials.size()!=1u||disabled.raw.size()!=1u
     ||!disabled.materials.front().requiresRawDxfMaterialReplay()
     ||!disabled.materials.front().m_hasChannelFlags||disabled.materials.front().m_channelFlags!=63)return 65;
 }
 for(bool binary:{false,true}){
  const auto inputPath=dir/(binary?"ezdxf-binary.dxf":"ezdxf-ascii.dxf");
  const auto inputName=inputPath.string();dxfRW input(inputName.c_str());Reader sink;
  for(int mode:{1,2,3,4,5,6,7,8,9,0}){
   if(!ezdxfDefaultSource(inputPath,binary,mode))return 45;
   sink.materials.clear();sink.raw.clear();const bool ok=input.read(&sink,false);
   if(mode>=1&&mode<=6){if(ok||!sink.materials.empty()||!sink.raw.empty())return 46;continue;}
   if(!ok||sink.materials.size()!=1u||sink.raw.size()!=1u
      ||!sink.materials[0].requiresRawDxfMaterialReplay()
      ||sink.materials[0].m_hasSpecularColorMethod
      ||sink.materials[0].m_hasChannelFlags
      ||sink.raw[0].groups.size()!=61u||sink.raw[0].groups[20].code()!=73
      ||sink.raw[0].groups[20].i_val()!=(mode==7?1:0)||sink.raw[0].groups.back().i_val()!=63)return 47;
  }
  LegacyReader legacy;if(input.read(&legacy,false)||legacy.materialCallbacks||legacy.rawCallbacks)return 48;
  Reader disabled;disabled.enabled=false;if(input.read(&disabled,false)||!disabled.raw.empty())return 49;
  disabled.enabled=true;if(!input.read(&disabled,false)||disabled.materials.size()!=1u)return 50;
  DwgProbe probe;static_cast<DRW_Material&>(probe)=sink.materials[0];dwgBufferW data,strings,handles;
  if(probe.encodeDwg(DRW::AC1024,&data,&strings,&handles)
     ||!data.data().empty()||!strings.data().empty()||!handles.data().empty())return 61;
  const auto dest=dir/(binary?"ezdxf-out-binary.dxf":"ezdxf-out-ascii.dxf");
  {std::ofstream f(dest);f<<"prior";}
  const auto destName=dest.string();dxfRW writer(destName.c_str());DxfWriter adapter(writer);
  adapter.value=sink.materials[0];adapter.raw=sink.raw[0];
  // 1F is the source material's handle but the writer reserves it for the
  // model-space BLOCK_RECORD; move the source identity before raw replay.
  writer.setHandleRemap({{0x1Fu,0xF000u}});
  if(writer.write(&adapter,DRW::AC1024,binary)||adapter.ok||bytes(dest)!="prior")return 51;
  const auto sourceFrame=adapter.raw;
  const std::vector<const DRW_RawDxfObject*> selected{&sourceFrame};
  if(!writer.setDxfObjectsWriteInspector([&](const auto& view){return writer.inspectDxfRawMaterialRecords(view,selected);}))return 76;
  adapter.useRaw=true;adapter.ok=false;
  if(!writer.write(&adapter,DRW::AC1024,binary)||!adapter.ok){
   const auto diagnostic=writer.getLastDiagnostic();
   std::cerr<<"raw retry: "<<diagnostic.code<<": "<<diagnostic.message<<'\n';return 52;
  }
  Reader reread;dxfRW check(destName.c_str());
  if(!check.read(&reread,false)||reread.raw.size()!=1u
     ||reread.raw[0].groups.size()!=61u||reread.raw[0].groups[20].code()!=73
     ||reread.raw[0].groups[20].i_val()!=0||!reread.materials[0].requiresRawDxfMaterialReplay())return 53;
  for(std::size_t index=0;index<sink.raw[0].groups.size();++index){
   const auto& a=sink.raw[0].groups[index];const auto& b=reread.raw[0].groups[index];
   if(a.code()!=b.code()||a.type()!=b.type())return 54;
   if(index==0){if(std::string(b.c_str())!="F000")return 55;continue;}
   if(a.type()==DRW_Variant::STRING&&std::string(a.c_str())!=std::string(b.c_str()))return 56;
   if(a.type()==DRW_Variant::INTEGER&&a.i_val()!=b.i_val())return 57;
   if(a.type()==DRW_Variant::DOUBLE&&a.d_val()!=b.d_val())return 58;
  }
  {std::ofstream f(dest);f<<"prior version";}
  adapter.ok=false;
  if(writer.write(&adapter,DRW::AC1027,binary)||adapter.ok||bytes(dest)!="prior version")return 59;
  adapter.ok=false;
  if(!writer.write(&adapter,DRW::AC1024,binary)||!adapter.ok)return 60;
 }
 // A selected graph with three raw-only materials and colliding source
 // handles. This verifies emitted references, not general graph admission.
 for(bool binary:{false,true}){
  const auto input=dir/(binary?"graph-source-binary.dxf":"graph-source-ascii.dxf");
  const auto output=dir/(binary?"graph-output-binary.dxf":"graph-output-ascii.dxf");
  dxfRW writer(output.string().c_str());MaterialGraphWriter graph(writer);
  const dxfRW::DxfHeaderWriteView inactive{"",DRW::AC1024,binary,"UTF-8",dxfRW::DxfCompatibilityProfile::StandaloneSafe};
  if(writer.inspectDxfHeaderMaterialReference(inactive,false,0u))return 102;
  if(writer.inspectDxfRootDictionaryEntry(inactive,"ACAD_MATERIAL",0xEu,350))return 103;
  const char* handles[]={"1F","20","21"};const int modes[]={0,8,9};
  for(int i=0;i<3;++i){
   if(!ezdxfDefaultSource(input,binary,modes[i],handles[i]))return 66;
   Reader read;dxfRW sourceReader(input.string().c_str());
   if(!sourceReader.read(&read,false)||read.raw.size()!=1u||read.materials.size()!=1u
      ||!read.materials.front().requiresRawDxfMaterialReplay())return 67;
   graph.raw.push_back(read.raw.front());
  }
  graph.dict.handle=0xE;graph.dict.parentHandle=0xA;
  graph.dict.m_entries={{"ByBlock",0x1F},{"ByLayer",0x20},{"Global",0x21}};
  writer.setHandleRemap({{0xA,0xC},{0xE,0xF010},{0x1F,0xF011},{0x20,0xF012},{0x21,0xF013}});
  writer.setRootDictEntries({{"ACAD_MATERIAL","F010"}});
  std::uint32_t headerSource=0x20;bool headerPresent=true,headerSeen=false;
  if(!writer.setDxfHeaderWriteInspector([&](const auto& view){
   headerSeen=true;std::string changed(view.bytes);const auto at=changed.find("$CMATERIAL");
   if(at!=std::string::npos){
    changed[at+1u]='c';auto bad=view;bad.bytes=changed;
    if(writer.inspectDxfHeaderMaterialReference(bad,headerPresent,headerSource))return false;
   }
   return writer.inspectDxfHeaderMaterialReference(view,headerPresent,headerSource);
  }))return 96;
  std::vector<dxfRW::DxfLayerMaterialReference> layerReferences{{"RAWTEST",0x90,0x21,true}};
  if(!writer.setDxfTablesWriteInspector([&](const auto& view){return writer.inspectDxfLayerMaterialReferences(view,layerReferences);}))return 87;
  std::vector<const DRW_RawDxfObject*> selected;
  for(const auto& raw:graph.raw)selected.push_back(&raw);
  std::uint32_t rootTarget=0xE;int rootRole=350;
  if(!writer.setDxfObjectsWriteInspector([&](const auto& view){
   std::string changed(view.bytes);const auto at=changed.find("ACAD_MATERIAL");
   if(at==std::string::npos)return false;
   changed[at]='a';auto bad=view;bad.bytes=changed;
   const bool altered=writer.inspectDxfRootDictionaryEntry(bad,"ACAD_MATERIAL",rootTarget,rootRole);
   const auto rejectsEdit=[&](const std::string& marker,std::size_t valueOffset){
    std::string edited(view.bytes);const auto where=edited.find(marker);
    if(where==std::string::npos||where>=at||where+valueOffset>=edited.size())return false;
    edited[where+valueOffset]='1';auto candidate=view;candidate.bytes=edited;
    return !writer.inspectDxfRootDictionaryEntry(candidate,"ACAD_MATERIAL",rootTarget,rootRole);
   };
   const bool ownerRejected=rejectsEdit(binary?std::string("\x4a\x01" "0\0",4):"330\n0\n",binary?2u:4u);
   const bool selfRejected=rejectsEdit(binary?std::string("\x05\x00" "C\0",4):"  5\nC\n",binary?2u:4u);
   const bool root=writer.inspectDxfRootDictionaryEntry(view,"ACAD_MATERIAL",rootTarget,rootRole);
   const bool materials=writer.inspectDxfRawMaterialRecords(view,selected);
   if(rootTarget==0xEu&&rootRole==350&&(!root||altered||!materials||!ownerRejected||!selfRejected)){
    std::cerr<<"root inspection: binary="<<binary<<" root="<<root<<" altered="<<altered
             <<" owner="<<ownerRejected<<" self="<<selfRejected<<" materials="<<materials<<'\n';
   }
   return !altered&&ownerRejected&&selfRejected&&root&&materials;
  }))return 77;
  if(!writer.write(&graph,DRW::AC1024,binary)||!graph.ok||!headerSeen)return 68;
  Reader check;dxfRW reread(output.string().c_str());
  if(!reread.read(&check,false)||check.raw.size()!=3u||check.materials.size()!=3u
     ||check.currentMaterial!="F012")return 69;
  bool rootEdge=false;for(const auto& e:check.root.m_entries)
   if(e.m_name=="ACAD_MATERIAL"&&e.m_handle==0xF010)rootEdge=true;
  if(!rootEdge)return 70;
  auto dict=std::find_if(check.dictionaries.begin(),check.dictionaries.end(),[](const auto& d){return d.handle==0xF010;});
  if(dict==check.dictionaries.end()||dict->parentHandle!=0xC||dict->m_entries.size()!=3u)return 71;
  for(std::size_t i=0;i<3;++i){
   const auto expected=static_cast<std::uint32_t>(0xF011+i);
   if(dict->m_entries[i].m_name!=check.materials[i].m_name
      ||dict->m_entries[i].m_handle!=expected||check.raw[i].handle!=expected
      ||check.raw[i].parentHandle!=0xF010||check.raw[i].groups.size()!=61u
      ||check.materials[i].reactorHandles!=std::vector<std::uint32_t>{0xF010}
      ||!check.materials[i].requiresRawDxfMaterialReplay())return 72;
  }
  auto layer=std::find_if(check.layers.begin(),check.layers.end(),[](const auto& l){return l.name=="RAWTEST";});
  std::uint32_t target=0;bool present=false;
  if(layer==check.layers.end()||!layer->getDxfMaterialHandle(target,present)||!present||target!=0xF013)return 73;
  {std::ofstream f(output);f<<"sentinel";}
  rootTarget=0x20;
  if(writer.write(&graph,DRW::AC1024,binary)||bytes(output)!="sentinel")return 104;
  rootTarget=0xE;rootRole=360;
  if(writer.write(&graph,DRW::AC1024,binary)||bytes(output)!="sentinel")return 105;
  rootRole=350;
  if(!writer.write(&graph,DRW::AC1024,binary))return 106;
  {std::ofstream f(output);f<<"sentinel";}
  headerSource=0x21;
  if(writer.write(&graph,DRW::AC1024,binary)||bytes(output)!="sentinel")return 97;
  headerSource=0x20;headerPresent=false;
  if(writer.write(&graph,DRW::AC1024,binary)||bytes(output)!="sentinel")return 98;
  headerPresent=true;
  if(!writer.write(&graph,DRW::AC1024,binary))return 99;
  graph.headerTarget="0";headerSource=0;
  if(!writer.write(&graph,DRW::AC1024,binary))return 100;
  graph.headerEnabled=false;headerPresent=false;
  if(!writer.write(&graph,DRW::AC1024,binary))return 101;
  graph.headerEnabled=true;graph.headerTarget="20";headerPresent=true;headerSource=0x20;
  {std::ofstream f(output);f<<"sentinel";}
  layerReferences.front().sourceMaterialHandle=0x20;
  if(writer.write(&graph,DRW::AC1024,binary)||bytes(output)!="sentinel")return 88;
  layerReferences.front().sourceMaterialHandle=0x21;
  if(!writer.write(&graph,DRW::AC1024,binary))return 89;
  {std::ofstream f(output);f<<"sentinel";}
  layerReferences.push_back(layerReferences.front());
  if(writer.write(&graph,DRW::AC1024,binary)||bytes(output)!="sentinel")return 90;
  layerReferences.pop_back();
  if(!writer.write(&graph,DRW::AC1024,binary))return 91;
  if(check.rawDictionaries.size()!=1u)return 81;
  selected.push_back(&check.rawDictionaries.front());
  if(!writer.setDxfObjectsWriteInspector([&](const auto& view){return writer.inspectDxfRawObjectRecords(view,selected);}))return 82;
  if(!writer.write(&graph,DRW::AC1024,binary)||!graph.ok)return 83;
  auto wrongDictionary=check.rawDictionaries.front();
  const auto entry=std::find_if(wrongDictionary.groups.begin(),wrongDictionary.groups.end(),
      [](const auto& value){return value.code()==3;});
  if(entry==wrongDictionary.groups.end())return 84;
  entry->addString(3,"changed");selected.back()=&wrongDictionary;
  {std::ofstream f(output);f<<"sentinel";}
  if(writer.write(&graph,DRW::AC1024,binary)||bytes(output)!="sentinel")return 85;
  selected.back()=&check.rawDictionaries.front();
  if(!writer.write(&graph,DRW::AC1024,binary))return 86;
  {std::ofstream f(output);f<<"sentinel";}
  graph.extraLayer=true;
  if(writer.write(&graph,DRW::AC1024,binary)||bytes(output)!="sentinel")return 92;
  layerReferences.push_back({"OTHER",0x91,0x20,true});
  if(!writer.write(&graph,DRW::AC1024,binary))return 93;
  graph.extraLayer=false;layerReferences.pop_back();
  if(!writer.write(&graph,DRW::AC1024,binary))return 94;
 }
 return 0;
}
