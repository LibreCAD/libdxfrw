// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "dwg2dxf/dx_iface.h"
#include "dwg2dxf/dx_material_body.h"
#include "dwg2dxf/dx_wide_dictionary_source.h"
#include "intern/dxfwriter.h"
#include "libdwgr.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
struct Old : LC3CompatCheck {int typed=0,raw=0;DRW_Material last;void addMaterial(const DRW_Material& m)override{last=m;++typed;}void addRawDxfObject(const DRW_RawDxfObject& o)override{if(o.name=="MATERIAL")++raw;}};
struct Facet : Old, DRW_DxfSparseMaterialRawPolicy {bool enabled=true;bool wantsDxfSparseMaterialRawCapture()const noexcept override{return enabled;}};
struct DirectWriter:LC3CompatCheck {dxfRW& writer;DRW_Material material;bool ok=false;explicit DirectWriter(dxfRW& w):writer(w){}void writeObjects()override{ok=writer.writeMaterial(&material);}bool finalizeDxfWrite()override{return ok;}};
struct DirectDwgWriter:LC3CompatCheck {dwgRW& writer;DRW_Material material;bool registered=false,ok=false;explicit DirectDwgWriter(dwgRW& w):writer(w){}void writeDwgClasses()override{registered=writer.registerMaterialObjectClass(&material);}void writeObjects()override{ok=writer.writeMaterial(&material);}bool finalizeDwgWrite()override{return true;}};
struct Unaware : dx_iface {};
struct Exporter : dx_iface {bool wantsDxfSparseMaterialRawCapture()const noexcept override{return true;}std::function<void()> late;void writeObjects()override{dx_iface::writeObjects();if(late)late();}};
class DxWideProjectionTestAccess {
public:
 static bool incomplete(const dx_data& data) noexcept {
  return data.m_dxfWideProjectionRequired&&!data.m_dxfWideProjection;
 }
};
static std::string bytes(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
static bool source(const std::filesystem::path& p,bool binary,int mode=0){
 std::ofstream f(p,std::ios::binary|std::ios::trunc);if(binary)f<<"AutoCAD Binary DXF\r\n"<<char(26)<<'\0';std::unique_ptr<dxfWriter>w=binary?std::unique_ptr<dxfWriter>(new dxfWriterBinary(&f)):std::unique_ptr<dxfWriter>(new dxfWriterAscii(&f));w->setVersion(mode==7?"AC1024":"AC1027",true);
 const auto s=[&](int c,const char*v){return w->writeString(c,v);};const auto h=[&](int c,int v){return w->writeInt16(c,v);};const auto i=[&](int c,int v){return w->writeInt32(c,v);};
 if(!s(0,"SECTION")||!s(2,"HEADER")||!s(9,"$ACADVER")||!s(1,mode==7?"AC1024":"AC1027")||!s(0,"ENDSEC")||!s(0,"SECTION")||!s(2,"OBJECTS")||!s(0,"DICTIONARY")||!s(5,"C")||!s(330,"0")||!s(100,"AcDbDictionary")||!h(280,0)||!h(281,1)||!s(3,"ACAD_MATERIAL")||!s(350,"67")||!s(0,"DICTIONARY")||!s(5,"67")||!s(102,"{ACAD_REACTORS")||!s(330,"C")||!s(102,"}")||!s(330,"C")||!s(100,"AcDbDictionary")||!h(281,1))return false;
 for(const auto& x:std::vector<std::pair<const char*,const char*>>{{"ByBlock","69"},{"ByLayer","68"},{"Global","6A"}})if(!s(3,x.first)||!s(350,x.second))return false;
 for(const auto& x:std::vector<std::pair<const char*,const char*>>{{"ByBlock","69"},{"ByLayer","68"},{"Global","6A"}}){if(!s(0,"MATERIAL")||!s(5,x.second)||!s(102,"{ACAD_REACTORS")||!s(330,mode==4?"66":"67")||!s(102,"}")||!s(330,"67")||!s(100,"AcDbMaterial")||!s(1,mode==5?"Other":x.first))return false;if(mode==8&&(!h(71,1)||!i(91,-1023410177)))return false;if(!h(72,mode==2?0:1))return false;if(mode==8&&!h(273,1))return false;if(mode==6&&!s(2,"extra"))return false;if(!i(94,mode==1?126:mode==8?63:127))return false;if(mode==3&&!i(94,127))return false;}
 return s(0,"ENDSEC")&&s(0,"EOF")&&!w->hasWriteError();}
static bool graph(const dx_data& d){if(!d.hasNamedObjectDictionary||d.materials.size()!=3||d.rawMaterialObjects.size()!=3)return false;const auto root=d.namedObjectDictionary.handle;const DRW_Dictionary* dict=nullptr;for(const auto& x:d.dictionaries)if(x.handle==0x67)dict=&x;if(!dict||dict->parentHandle!=root||dict->m_entries.size()!=3)return false;for(const auto& m:d.materials){if(!m.requiresRawDxfMaterialReplay()||m.m_hasDiffuseMapSource||m.m_channelFlags!=127||m.parentHandle!=dict->handle||m.reactorHandles.size()!=1||m.reactorHandles[0]!=dict->handle)return false;bool found=false;for(const auto& e:dict->m_entries)if(e.m_handle==m.handle&&e.m_name==m.m_name)found=true;if(!found)return false;}for(const auto& r:d.rawMaterialObjects){if(r.groups.size()!=9||r.groups[7].code()!=72||r.groups[7].i_val()!=1||r.groups[8].code()!=94||r.groups[8].i_val()!=127||r.parentHandle!=dict->handle)return false;}return true;}

// Locally authored AC1024/AC1027 default-material carriers. No drawing fixture is
// stored in the repository; the ambiguous second group 73 stays opaque.
static bool defaultSource(const std::filesystem::path& path, bool binary, bool collision, DRW::Version version, bool multiPlot=false, const char* xrecordReactor=nullptr, bool wideGraph=false, const char* extraWideReference=nullptr, bool duplicateLayerHandle=false, unsigned extraWide=0u, bool sharedRecord=false, unsigned crossReference=0u, int sharedReferenceCode=340) {
 std::ofstream f(path,std::ios::binary|std::ios::trunc);
 if(binary)f<<"AutoCAD Binary DXF\r\n"<<char(26)<<'\0';
 std::unique_ptr<dxfWriter> w=binary?std::unique_ptr<dxfWriter>(new dxfWriterBinary(&f)):std::unique_ptr<dxfWriter>(new dxfWriterAscii(&f));
 const char* versionName=version==DRW::AC1024?"AC1024":"AC1027";
 w->setVersion(versionName,true);
 const auto s=[&](int c,const char* v){return w->writeString(c,v);};
 const auto h=[&](int c,int v){return w->writeInt16(c,v);};
 const auto i=[&](int c,int v){return w->writeInt32(c,v);};
 const auto d=[&](int c,double v){return w->writeDouble(c,v);};
 const char* blockHandle=collision?"1F":"69";
 const char* graphHandle=wideGraph?"100000010":"72";
 const auto hex=[](std::uint64_t n){std::ostringstream out;out<<std::uppercase<<std::hex<<n;return out.str();};
 const auto seed=hex(UINT64_C(0x100000020)+extraWide*16u);
 const char* recordHandle=wideGraph?"100000011":"73";
 if(!s(0,"SECTION")||!s(2,"HEADER")||!s(9,"$ACADVER")||!s(1,versionName)
    ||(wideGraph&&(!s(9,"$HANDSEED")||!s(5,extraWide?seed.c_str():sharedRecord?"100000014":"100000012")))
    ||!s(9,"$CMATERIAL")||!s(347,blockHandle)||!s(0,"ENDSEC")
    ||!s(0,"SECTION")||!s(2,"TABLES")||!s(0,"TABLE")||!s(2,"LAYER")
    ||(multiPlot&&(!s(5,"2")||!s(330,"0")||!s(100,"AcDbSymbolTable")))||!h(70,multiPlot?2:1)
    ||!s(0,"LAYER")||!s(5,"90")||(multiPlot&&!s(330,"2"))||!s(100,"AcDbSymbolTableRecord")||!s(100,"AcDbLayerTableRecord")
    ||!s(2,"RAWTEST")||!h(70,0)||!h(62,7)||!s(6,"CONTINUOUS")||!s(347,blockHandle)
    ||(multiPlot&&!s(390,"71")))return false;
 if(multiPlot&&(!s(0,"LAYER")||!s(5,duplicateLayerHandle?"90":"91")||!s(330,"2")||!s(100,"AcDbSymbolTableRecord")
    ||!s(100,"AcDbLayerTableRecord")||!s(2,"0")||!h(70,0)||!h(62,7)
    ||!s(6,"CONTINUOUS")||!s(347,blockHandle)||!s(390,"71")))return false;
 if(!s(0,"ENDTAB")||!s(0,"ENDSEC")||!s(0,"SECTION")||!s(2,"OBJECTS")
    ||!s(0,"DICTIONARY")||!s(5,"C")||!s(330,"0")||!s(100,"AcDbDictionary")
    ||!h(280,0)||!h(281,1)||(multiPlot&&(!s(3,"ACAD_GROUP")||!s(350,"D")))
    ||!s(3,"ACAD_MATERIAL")||!s(350,"67")
    ||(multiPlot&&(!s(3,"ACAD_PLOTSTYLENAME")||!s(350,"70")
                    ||!s(3,"CODEX_GRAPH")||!s(350,graphHandle))))return false;
 for(unsigned n=0;n<extraWide;++n) {
  const auto name="CODEX_GRAPH_"+std::to_string(n+2u), handle=hex(UINT64_C(0x100000020)+n*16u);
  if(!s(3,name.c_str())||!s(350,handle.c_str()))return false;
 }
 if(multiPlot&&(!s(0,"DICTIONARY")||!s(5,"D")||!s(330,"C")
    ||!s(100,"AcDbDictionary")||!h(280,0)||!h(281,1)))return false;
 if(!s(0,"DICTIONARY")||!s(5,"67")
    ||(!multiPlot&&(!s(102,"{ACAD_REACTORS")||!s(330,"C")||!s(102,"}")))
    ||!s(330,"C")||!s(100,"AcDbDictionary")||!h(281,1))return false;
 for(const auto& x:std::vector<std::pair<const char*,const char*>>{{"ByBlock",blockHandle},{"ByLayer","68"},{"Global","6A"}})
  if(!s(3,x.first)||!s(350,x.second))return false;
 for(const auto& x:std::vector<std::pair<const char*,const char*>>{{"ByBlock",blockHandle},{"ByLayer","68"},{"Global","6A"}}) {
  if(!s(0,"MATERIAL")||!s(5,x.second)||!s(102,"{ACAD_REACTORS")||!s(330,"67")||!s(102,"}")
    ||!s(330,"67")||!s(100,"AcDbMaterial")||!s(1,x.first)||!s(2,"")||!h(70,0)||!d(40,1)
    ||!h(71,1)||!d(41,1)||!i(91,-1023410177)||!d(42,1)||!h(72,1)||!s(3,"")
    ||!h(73,1)||!h(74,1)||!h(75,1)||!d(44,0.5)||!h(73,0)||!d(45,1)||!d(46,1)
    ||!h(77,1)||!s(4,"")||!h(78,1)||!h(79,1)||!h(170,1)||!d(48,1)||!h(171,1)
    ||!s(6,"")||!h(172,1)||!h(173,1)||!h(174,1)||!d(140,1)||!d(141,1)
    ||!h(175,1)||!s(7,"")||!h(176,1)||!h(177,1)||!h(178,1)||!d(143,1)
    ||!h(179,1)||!s(8,"")||!h(270,1)||!h(271,1)||!h(272,1)||!d(145,1)
    ||!d(146,1)||!h(273,1)||!s(9,"")||!h(274,1)||!h(275,1)||!h(276,1)
    ||!d(42,1)||!h(72,1)||!s(3,"")||!h(73,1)||!h(74,1)||!h(75,1)||!i(94,63))return false;
 }
 if(multiPlot&&(!s(0,"ACDBDICTIONARYWDFLT")||!s(5,"70")||!s(330,"C")
    ||!s(100,"AcDbDictionary")||!h(281,1)||!s(3,"Normal")||!s(350,"71")
    ||!s(100,"AcDbDictionaryWithDefault")||!s(340,"71")
    ||!s(0,"ACDBPLACEHOLDER")||!s(5,"71")||!s(330,"70")
    ||!s(0,"DICTIONARY")||!s(5,graphHandle)||!s(330,"C")||!s(100,"AcDbDictionary")
    ||!h(280,0)||!h(281,1)||!s(3,"RECORD")||!s(350,recordHandle)
    ||(sharedRecord&&(!s(3,"SHARED_RECORD")||!s(350,"100000013")))
    ||!s(0,"XRECORD")||!s(5,recordHandle)
    ||(xrecordReactor&&(!s(102,"{ACAD_REACTORS")||!s(330,xrecordReactor)||!s(102,"}")))
    ||!s(330,graphHandle)||!s(100,"AcDbXrecord")
    ||!h(280,1)||!s(1,"payload")||!s(340,graphHandle)
    ||(extraWideReference&&!s(341,extraWideReference))))return false;
 if(sharedRecord&&(!s(0,"XRECORD")||!s(5,"100000013")||!s(330,graphHandle)
    ||!s(100,"AcDbXrecord")||!h(280,1)||!s(1,"shared payload")
    ||!s(sharedReferenceCode,crossReference==2u?"100000021":crossReference?"100000020":graphHandle)))return false;
 for(unsigned n=0;n<extraWide;++n) {
  const auto name="RECORD_"+std::to_string(n+2u), handle=hex(UINT64_C(0x100000020)+n*16u), record=hex(UINT64_C(0x100000021)+n*16u);
  if(!s(0,"DICTIONARY")||!s(5,handle.c_str())||!s(330,"C")||!s(100,"AcDbDictionary")||!h(280,0)||!h(281,1)||!s(3,name.c_str())||!s(350,record.c_str())
    ||!s(0,"XRECORD")||!s(5,record.c_str())||!s(330,handle.c_str())||!s(100,"AcDbXrecord")||!h(280,1)||!s(1,"payload")||!s(340,handle.c_str()))return false;
 }
 return s(0,"ENDSEC")&&s(0,"EOF")&&!w->hasWriteError();
}

static bool defaultGraph(const dx_data& data, DRW::Version version) {
 if(!data.hasNamedObjectDictionary||data.materials.size()!=3||data.rawMaterialObjects.size()!=3)return false;
 for(const auto& material:data.materials)
  if(!material.requiresRawDxfMaterialReplay()||material.parentHandle!=0x67u)return false;
 for(const auto& raw:data.rawMaterialObjects)
  if(raw.m_version!=version||raw.groups.size()!=61u||raw.groups.back().code()!=94||raw.groups.back().i_val()!=63)return false;
 return true;
}

static bool defaultRoundTrip(const std::filesystem::path& in,const std::filesystem::path& out,bool binary,bool collision,DRW::Version version) {
 if(!defaultSource(in,binary,collision,version))return false;
 dx_data data;Exporter io;
 if(!io.fileImport(in.string(),&data,false)||!defaultGraph(data,version))return false;
 const auto write=[&](){return io.fileExport(out.string(),version,binary,&data,false,true);};
 const auto verify=[&](){
  dx_data check;dx_iface reader;
  if(!reader.fileImport(out.string(),&check,false)||!defaultGraph(check,version))return false;
  std::uint32_t header=0,layerHandle=0;bool headerPresent=false,layerPresent=false;
  if(!check.headerC.getDxfMaterialReferenceHandle(header,headerPresent)||!headerPresent||!header)return false;
  const auto material=std::find_if(check.materials.begin(),check.materials.end(),
      [&](const auto& item){return item.handle==header&&item.m_name=="ByBlock";});
  const auto layer=std::find_if(check.layers.begin(),check.layers.end(),
      [](const auto& item){return item.name=="RAWTEST";});
  return material!=check.materials.end()&&layer!=check.layers.end()
      &&layer->getDxfMaterialHandle(layerHandle,layerPresent)&&layerPresent&&layerHandle==header;
 };
 if(!write()||!verify())return false;
 const auto reject=[&](const std::function<void()>& mutate,const std::function<void()>& restore){
  std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";mutate();
  const bool refused=!write();restore();return refused&&bytes(out)=="sentinel"&&write()&&verify();};
 auto& raw=data.rawMaterialObjects.front();const auto original=raw;
 if(!dxfMaterial::hasEzdxfDefaultShape(raw.groups))return false;
 {auto bad=raw.groups;bad[dxfMaterial::ezdxfDefaultPrefixSize]=DRW_Variant(73,2);
  if(dxfMaterial::hasEzdxfDefaultShape(bad))return false;
  bad=raw.groups;bad[dxfMaterial::ezdxfDefaultPrefixSize]=DRW_Variant(73,std::string("0"));
  if(dxfMaterial::hasEzdxfDefaultShape(bad))return false;}
 if(!reject([&](){raw.groups.back()=DRW_Variant(94,62);},[&](){raw=original;}))return false;
 auto& material=data.materials.front();const auto name=material.m_name;
 if(!reject([&](){material.m_name="changed";},[&](){material.m_name=name;}))return false;
 const auto factor=material.m_diffuseColorFactor;
 if(!reject([&](){material.m_diffuseColorFactor=factor+0.25;},[&](){material.m_diffuseColorFactor=factor;}))return false;
 auto& dict=data.dictionaries.front();const auto entries=dict.m_entries;
 if(!reject([&](){dict.m_entries.pop_back();},[&](){dict.m_entries=entries;}))return false;
 io.late=[&](){raw.groups.back()=DRW_Variant(94,62);};
 std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";
 const bool lateRefused=!write();io.late={};raw=original;
 if(!lateRefused||bytes(out)!="sentinel"||!write()||!verify())return false;
 std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";
 if(io.fileExport(out.string(),version==DRW::AC1024?DRW::AC1027:DRW::AC1024,binary,&data,false,true)||bytes(out)!="sentinel")return false;
 std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";
 if(io.fileExport(out.string(),version,!binary,&data,false,true)||bytes(out)!="sentinel")return false;
 return write()&&verify();
}

static bool multiPlotRoundTrip(const std::filesystem::path& in,const std::filesystem::path& out,bool binary,bool collision) {
 if(!defaultSource(in,binary,collision,DRW::AC1027,true))return false;
 dx_data data;Exporter io;
 if(!io.fileImport(in.string(),&data,false)||data.layers.size()!=2u
    ||data.dictionaryWithDefaults.size()!=1u||data.rawDictionaryWithDefaults.size()!=1u
    ||data.rawPlaceholders.size()!=1u||data.xRecords.size()!=1u)return false;
 const auto write=[&](){return io.fileExport(out.string(),DRW::AC1027,binary,&data,false,true);};
 const auto verify=[&](){
  dx_data decoded;dx_iface reader;
  if(!reader.fileImport(out.string(),&decoded,false)||decoded.layers.size()!=2u
     ||decoded.dictionaryWithDefaults.size()!=1u||decoded.rawPlaceholders.size()!=1u
     ||decoded.xRecords.size()!=1u)return false;
  const auto& dictionary=decoded.dictionaryWithDefaults.front();
  if(dictionary.m_entries.size()!=1u||dictionary.m_entries.front().m_name!="Normal"
     ||dictionary.m_entries.front().m_handle!=dictionary.m_defaultEntryHandle
     ||decoded.rawPlaceholders.front().handle!=dictionary.m_defaultEntryHandle
     ||decoded.rawPlaceholders.front().parentHandle!=dictionary.handle)return false;
  for(const auto& layer:decoded.layers){std::uint32_t target=0u;bool present=false;
   if(!layer.getDxfPlotStyleHandle(target,present)||!present||target!=dictionary.m_defaultEntryHandle)return false;}
  const auto& record=decoded.xRecords.front();const auto owner=record.parentHandle;
  if(record.m_dataEntries.size()!=2u||record.m_dataEntries[0].code()!=1
     ||record.m_dataEntries[1].code()!=340
     ||record.m_dataEntries[1].type()!=DRW_Variant::INTEGER64
     ||record.m_dataEntries[1].i64_val()!=static_cast<std::int64_t>(owner))return false;
  return true;
 };
 if(!write()||!verify()){std::cerr<<"multi-layer plot graph initial output failed\n";return false;}
 const auto reject=[&](const std::function<void()>& mutate,const std::function<void()>& restore){
  std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";mutate();
  const bool refused=!write();restore();return refused&&bytes(out)=="sentinel"&&write()&&verify();};
 auto& layer=data.layers.front();const auto originalPlot=layer.handlePlotS;
 if(!reject([&](){layer.handlePlotS="72";},[&](){layer.handlePlotS=originalPlot;}))return false;
 auto& dictionary=data.dictionaryWithDefaults.front();const auto originalDefault=dictionary.m_defaultEntryHandle;
 if(!reject([&](){dictionary.m_defaultEntryHandle=0x72u;},[&](){dictionary.m_defaultEntryHandle=originalDefault;}))return false;
 auto& raw=data.rawDictionaryWithDefaults.front();const auto saved=raw;
 if(!reject([&](){raw.groups.back()=DRW_Variant(340,std::string("72"));},[&](){raw=saved;}))return false;
 auto& root=data.namedObjectDictionary;const auto rootEntries=root.m_entries;
 if(!reject([&](){root.m_entries.push_back(root.m_entries.front());},[&](){root.m_entries=rootEntries;}))return false;
 if(!reject([&](){root.m_entries.back().m_dxfHandleCode=DRW_Dictionary::Entry::DxfHandleCode::HardOwner;},[&](){root.m_entries=rootEntries;}))return false;
 auto child=std::find_if(data.dictionaries.begin(),data.dictionaries.end(),[](const auto& value){return value.handle==0x67u;});
 if(child==data.dictionaries.end()||child->m_entries.empty())return false;
 const auto childEntries=child->m_entries;
 if(!reject([&](){child->m_entries.front().m_handle=0x71u;},[&](){child->m_entries=childEntries;}))return false;
 auto& record=data.xRecords.front();const auto savedRecord=record;
 if(!reject([&](){record.xDictHandle=0x74u;},[&](){record=savedRecord;}))return false;
 if(!reject([&](){record.reactorHandles.push_back(0x74u);},[&](){record=savedRecord;}))return false;
 io.late=[&](){record.m_dataEntries[0]=DRW_Variant(1,std::string("changed"));};
 std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";
 const bool refusedRecord=!write();io.late={};record=savedRecord;
 if(!refusedRecord||bytes(out)!="sentinel"||!write()||!verify())return false;
 io.late=[&](){layer.handlePlotS="72";};std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";
 const bool refused=!write();io.late={};layer.handlePlotS=originalPlot;
 if(!refused||bytes(out)!="sentinel"||!write()||!verify())return false;
 std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";
 if(io.fileExport(out.string(),DRW::AC1024,binary,&data,false,true)||bytes(out)!="sentinel")return false;
 return write()&&verify();
}

static bool multiPlotReactorRefusal(const std::filesystem::path& in,const std::filesystem::path& out,bool binary) {
 Exporter io;
 for(const char* target:{"C","DEAD"}) {
  if(!defaultSource(in,binary,false,DRW::AC1027,true,target))return false;
  dx_data data;
  if(!io.fileImport(in.string(),&data,false))return false;
  std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";
  if(io.fileExport(out.string(),DRW::AC1027,binary,&data,false,true)||bytes(out)!="sentinel")return false;
  if(!defaultSource(in,binary,false,DRW::AC1027,true))return false;
  dx_data corrected;
  if(!io.fileImport(in.string(),&corrected,false)
     ||!io.fileExport(out.string(),DRW::AC1027,binary,&corrected,false,true))return false;
 }
 return true;
}

static bool wideGraphRoundTrip(const std::filesystem::path& in,const std::filesystem::path& out,bool binary,bool collision) {
 if(!defaultSource(in,binary,collision,DRW::AC1027,true,nullptr,true))return false;
 dx_data legacy;Exporter legacyReader;
 if(legacyReader.fileImport(in.string(),&legacy,false))return false;
 for(bool whole:{false,true}) {
  dx_data observed;Exporter observer;
  if(!observer.enableDxfWideGraphProjection()
      ||!(whole?observer.enableWholeDxfSourceObservation():observer.enableLegacyDxfIdentityObservation())
      ||observer.fileImport(in.string(),&observed,false))return false;
 }
 dx_data data;Exporter io;
 if(!io.enableDxfWideGraphProjection()||!io.fileImport(in.string(),&data,false)
    ||data.layers.size()!=2u||data.xRecords.size()!=1u)return false;
 // The later export must use the captured source, not reopen a changed file.
 std::ofstream(in,std::ios::binary|std::ios::trunc)<<"changed after import";
 const auto write=[&](){return io.fileExport(out.string(),DRW::AC1027,binary,&data,false,true);};
 const auto verify=[&](){
  dx_data decoded;dx_iface reader;
  if(!reader.fileImport(out.string(),&decoded,false)||decoded.layers.size()!=2u||decoded.xRecords.size()!=1u)return false;
  std::uint32_t dictionary=0u,record=0u;
  for(const auto& entry:decoded.namedObjectDictionary.m_entries)if(entry.m_name=="CODEX_GRAPH")dictionary=entry.m_handle;
  for(const auto& item:decoded.dictionaries)if(item.handle==dictionary&&item.parentHandle==decoded.namedObjectDictionary.handle)
   for(const auto& entry:item.m_entries)if(entry.m_name=="RECORD")record=entry.m_handle;
  if(!dictionary||!record||decoded.xRecords.front().handle!=record||decoded.xRecords.front().parentHandle!=dictionary)return false;
  unsigned references=0u;
  for(const auto& item:decoded.xRecords.front().m_dataEntries)if(item.code()==340) {
   if(item.type()!=DRW_Variant::INTEGER64||item.i64_val()!=static_cast<std::int64_t>(dictionary))return false;
   ++references;
  }
  return references==1u;
 };
 if(!write()||!verify())return false;
 const auto reject=[&](const std::function<void()>& mutate,const std::function<void()>& restore){
  std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";mutate();
  const bool refused=!write();restore();return refused&&bytes(out)=="sentinel"&&write()&&verify();
 };
 auto& root=data.namedObjectDictionary;const auto savedEntries=root.m_entries;
 auto target=std::find_if(root.m_entries.begin(),root.m_entries.end(),[](const auto& e){return e.m_name=="CODEX_GRAPH";});
 if(target==root.m_entries.end()||!reject([&](){target->m_handle=0x71u;},[&](){root.m_entries=savedEntries;}))return false;
 auto& xrecord=data.xRecords.front();const auto savedRecord=xrecord;
 io.late=[&](){xrecord.m_dataEntries[0]=DRW_Variant(1,std::string("changed"));};
 std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";
 const bool lateRefused=!write();io.late={};xrecord=savedRecord;
 if(!lateRefused||bytes(out)!="sentinel"||!write()||!verify())return false;
 for(const auto mode:{0,1,2}) {
  std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";
  const auto version=mode==0?DRW::AC1024:DRW::AC1027;
  const bool outputBinary=mode==1?!binary:binary;
  const bool strict=mode!=2;
  if(io.fileExport(out.string(),version,outputBinary,&data,false,strict)||bytes(out)!="sentinel"
      ||!write()||!verify())return false;
 }
 return true;
}

static bool wideGraphTwoChains(const std::filesystem::path& in,const std::filesystem::path& out,bool binary,bool collision) {
 if(!defaultSource(in,binary,collision,DRW::AC1027,true,nullptr,true,nullptr,false,true))return false;
 dx_data data;Exporter io;
 if(!io.enableDxfWideGraphProjection()||!io.fileImport(in.string(),&data,false)||data.xRecords.size()!=2u)return false;
 std::ofstream(in,std::ios::binary|std::ios::trunc)<<"changed after import";
 const auto write=[&](){return io.fileExport(out.string(),DRW::AC1027,binary,&data,false,true);};
 const auto verify=[&](){
  dx_data decoded;dx_iface reader;
  if(!reader.fileImport(out.string(),&decoded,false)||decoded.xRecords.size()!=2u)return false;
  std::unordered_set<std::uint32_t> identities;
  for(const auto& pair:std::vector<std::pair<const char*,const char*>>{{"CODEX_GRAPH","RECORD"},{"CODEX_GRAPH_2","RECORD_2"}}) {
   std::uint32_t dictionary=0u,record=0u;
   for(const auto& entry:decoded.namedObjectDictionary.m_entries)if(entry.m_name==pair.first)dictionary=entry.m_handle;
   for(const auto& item:decoded.dictionaries)if(item.handle==dictionary&&item.parentHandle==decoded.namedObjectDictionary.handle)
    for(const auto& entry:item.m_entries)if(entry.m_name==pair.second)record=entry.m_handle;
   if(!dictionary||!record||!identities.insert(dictionary).second||!identities.insert(record).second)return false;
   const DRW_XRecord* found=nullptr;
   for(const auto& item:decoded.xRecords)if(item.handle==record){if(found)return false;found=&item;}
   if(!found||found->parentHandle!=dictionary)return false;
   unsigned refs=0u;for(const auto& item:found->m_dataEntries)if(item.code()==340) {
    if(item.type()!=DRW_Variant::INTEGER64||item.i64_val()!=static_cast<std::int64_t>(dictionary))return false;
    ++refs;
   }
   if(refs!=1u)return false;
  }
  return true;
 };
 if(!write()||!verify())return false;
 auto& root=data.namedObjectDictionary;const auto savedEntries=root.m_entries;
 auto second=std::find_if(root.m_entries.begin(),root.m_entries.end(),[](const auto& entry){return entry.m_name=="CODEX_GRAPH_2";});
 if(second==root.m_entries.end())return false;
 std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";second->m_handle=0x71u;
 const bool refused=!write();root.m_entries=savedEntries;
 if(!refused||bytes(out)!="sentinel"||!write()||!verify())return false;
 auto& record=data.xRecords.back();const auto savedRecord=record;
 io.late=[&](){record.m_dataEntries[0]=DRW_Variant(1,std::string("changed"));};
 std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";
 const bool lateRefused=!write();io.late={};record=savedRecord;
 if(!lateRefused||bytes(out)!="sentinel"||!write()||!verify())return false;
 return true;
}

static bool wideGraphSharedDictionary(const std::filesystem::path& in,const std::filesystem::path& out,bool binary,bool collision,unsigned crossReference,int sharedReferenceCode=340) {
 if(!defaultSource(in,binary,collision,DRW::AC1027,true,nullptr,true,nullptr,false,1u,true,crossReference,sharedReferenceCode))return false;
 dx_data data;Exporter io;
 if(!io.enableDxfWideGraphProjection()||!io.fileImport(in.string(),&data,false)||data.xRecords.size()!=3u)return false;
 const auto write=[&](){return io.fileExport(out.string(),DRW::AC1027,binary,&data,false,true);};
 const auto verify=[&](){
  dx_data decoded;dx_iface reader;
  if(!reader.fileImport(out.string(),&decoded,false)||decoded.xRecords.size()!=3u)return false;
  std::uint32_t dictionary=0u,otherDictionary=0u;
  for(const auto& entry:decoded.namedObjectDictionary.m_entries) {
   if(entry.m_name=="CODEX_GRAPH")dictionary=entry.m_handle;
   if(entry.m_name=="CODEX_GRAPH_2")otherDictionary=entry.m_handle;
  }
  if(!otherDictionary)return false;
  std::uint32_t otherRecord=0u;
  for(const auto& item:decoded.dictionaries)if(item.handle==otherDictionary)
   for(const auto& entry:item.m_entries)if(entry.m_name=="RECORD_2")otherRecord=entry.m_handle;
  if(!otherRecord)return false;
  const DRW_Dictionary* graph=nullptr;
  for(const auto& item:decoded.dictionaries)if(item.handle==dictionary){if(graph)return false;graph=&item;}
  if(!graph||graph->parentHandle!=decoded.namedObjectDictionary.handle||graph->m_entries.size()!=2u)return false;
  std::unordered_set<std::uint32_t> identities;
  for(const auto* name:{"RECORD","SHARED_RECORD"}) {
   std::uint32_t id=0u;
   for(const auto& entry:graph->m_entries)if(entry.m_name==name)id=entry.m_handle;
   if(!id||!identities.insert(id).second)return false;
   const DRW_XRecord* record=nullptr;
   for(const auto& item:decoded.xRecords)if(item.handle==id){if(record)return false;record=&item;}
   if(!record||record->parentHandle!=dictionary)return false;
   const auto code=std::string(name)=="SHARED_RECORD"?sharedReferenceCode:340;
   unsigned refs=0u;for(const auto& item:record->m_dataEntries)if(item.code()==code) {
    const auto expected=std::string(name)=="SHARED_RECORD"&&crossReference
        ?(crossReference==2u?otherRecord:otherDictionary):dictionary;
    if(item.type()!=DRW_Variant::INTEGER64||item.i64_val()!=static_cast<std::int64_t>(expected))return false;
    ++refs;
   }
   if(refs!=1u)return false;
  }
  return true;
 };
 if(!write()||!verify())return false;
 auto& record=*std::next(data.xRecords.begin());const auto saved=record;
 std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";record.parentHandle=0x71u;
 const bool refused=!write();record=saved;
 if(!refused||bytes(out)!="sentinel"||!write()||!verify())return false;
 auto pointer=std::find_if(record.m_dataEntries.begin(),record.m_dataEntries.end(),
     [&](const auto& value){return value.code()==sharedReferenceCode;});
 if(pointer==record.m_dataEntries.end()||pointer->type()!=DRW_Variant::INTEGER64)return false;
 const auto target=pointer->i64_val();
 std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";
 *pointer=DRW_Variant(sharedReferenceCode==340?341:340,target);
 const bool roleRefused=!write();record=saved;
 if(!roleRefused||bytes(out)!="sentinel"||!write()||!verify())return false;
 io.late=[&](){record.m_dataEntries[0]=DRW_Variant(1,std::string("changed"));};
 std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";
 const bool lateRefused=!write();io.late={};record=saved;
 return lateRefused&&bytes(out)=="sentinel"&&write()&&verify();
}

static bool wideGraphCrossReferenceRetry(const std::filesystem::path& in,const std::filesystem::path& out,bool binary) {
 Exporter io;if(!io.enableDxfWideGraphProjection())return false;
 if(!defaultSource(in,binary,false,DRW::AC1027,true,nullptr,true,nullptr,false,0u,true,true))return false;
 dx_data rejected;std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";
 if(io.fileImport(in.string(),&rejected,false)||bytes(out)!="sentinel")return false;
 // Some independent readers consume an XRECORD body group 330 as an owner.
 if(!defaultSource(in,binary,false,DRW::AC1027,true,nullptr,true,nullptr,false,1u,true,true,330))return false;
 if(io.fileImport(in.string(),&rejected,false)||bytes(out)!="sentinel")return false;
 if(!defaultSource(in,binary,false,DRW::AC1027,true,nullptr,true,nullptr,false,1u,true,true,331))return false;
 if(io.fileImport(in.string(),&rejected,false)||bytes(out)!="sentinel")return false;
 if(!defaultSource(in,binary,false,DRW::AC1027,true,nullptr,true,nullptr,false,1u,true,true,332))return false;
 dx_data accepted;
 return io.fileImport(in.string(),&accepted,false)
     &&io.fileExport(out.string(),DRW::AC1027,binary,&accepted,false,true);
}

static bool wideGraphChainBoundary(const std::filesystem::path& in,const std::filesystem::path& out,bool binary) {
 Exporter io;if(!io.enableDxfWideGraphProjection())return false;
 if(!defaultSource(in,binary,false,DRW::AC1027,true,nullptr,true,nullptr,false,8u))return false;
 dx_data rejected;std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";
 if(io.fileImport(in.string(),&rejected,false)||bytes(out)!="sentinel")return false;
 if(!defaultSource(in,binary,false,DRW::AC1027,true,nullptr,true,nullptr,false,7u))return false;
 dx_data accepted;
 if(!io.fileImport(in.string(),&accepted,false)||accepted.xRecords.size()!=8u
    ||!io.fileExport(out.string(),DRW::AC1027,binary,&accepted,false,true))return false;
 dx_data decoded;dx_iface reader;
 if(!reader.fileImport(out.string(),&decoded,false)||decoded.xRecords.size()!=8u)return false;
 std::unordered_set<std::uint32_t> seen;
 for(unsigned n=1u;n<=8u;++n) {
  const auto suffix=n==1u?std::string():"_"+std::to_string(n);
  std::uint32_t dictionary=0u,record=0u;
  for(const auto& entry:decoded.namedObjectDictionary.m_entries)
   if(entry.m_name=="CODEX_GRAPH"+suffix)dictionary=entry.m_handle;
  for(const auto& item:decoded.dictionaries)if(item.handle==dictionary&&item.parentHandle==decoded.namedObjectDictionary.handle)
   for(const auto& entry:item.m_entries)if(entry.m_name=="RECORD"+suffix)record=entry.m_handle;
  if(!dictionary||!record||!seen.insert(dictionary).second||!seen.insert(record).second)return false;
  const DRW_XRecord* found=nullptr;
  for(const auto& item:decoded.xRecords)if(item.handle==record){if(found)return false;found=&item;}
  if(!found||found->parentHandle!=dictionary)return false;
  unsigned refs=0u;for(const auto& item:found->m_dataEntries)if(item.code()==340) {
   if(item.type()!=DRW_Variant::INTEGER64||item.i64_val()!=static_cast<std::int64_t>(dictionary))return false;
   ++refs;
  }
  if(refs!=1u)return false;
 }
 return true;
}

static bool wideGraphImportRetry(const std::filesystem::path& in,const std::filesystem::path& out,bool binary) {
 if(!defaultSource(in,binary,false,DRW::AC1027,true,nullptr,true,"100000020"))return false;
 Exporter io;if(!io.enableDxfWideGraphProjection())return false;
 std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";
 dx_data bad;if(io.fileImport(in.string(),&bad,false)||bytes(out)!="sentinel")return false;
 if(!defaultSource(in,binary,false,DRW::AC1027,true,nullptr,true))return false;
 dx_data corrected;
 if(!io.fileImport(in.string(),&corrected,false)
    ||!io.fileExport(out.string(),DRW::AC1027,binary,&corrected,false,true))return false;
 dx_data reopened;dx_iface reader;
 if(!reader.fileImport(out.string(),&reopened,false)||reopened.xRecords.size()!=1u)return false;
 // A failed reimport invalidates the proof without making projected data safe
 // for a legacy/non-strict export; a fresh corrected import remains usable.
 std::ofstream(in,std::ios::binary|std::ios::trunc)<<"invalid";
 if(io.fileImport(in.string(),&corrected,false))return false;
 for(bool strict:{false,true}) {
  std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";
  if(io.fileExport(out.string(),DRW::AC1027,binary,&corrected,false,strict)||bytes(out)!="sentinel")return false;
 }
 if(!defaultSource(in,binary,false,DRW::AC1027,true,nullptr,true))return false;
 dx_data fresh;
 return io.fileImport(in.string(),&fresh,false)
     &&io.fileExport(out.string(),DRW::AC1027,binary,&fresh,false,true);
}

static bool wideGraphSameFile(const std::filesystem::path& in,bool binary) {
 if(!defaultSource(in,binary,false,DRW::AC1027,true,nullptr,true))return false;
 dx_data data;Exporter io;
 if(!io.enableDxfWideGraphProjection()||!io.fileImport(in.string(),&data,false)
    ||!io.fileExport(in.string(),DRW::AC1027,binary,&data,false,true))return false;
 const auto saved=bytes(in);if(saved.empty())return false;
 auto& root=data.namedObjectDictionary;const auto entries=root.m_entries;
 auto selected=std::find_if(root.m_entries.begin(),root.m_entries.end(),[](const auto& e){return e.m_name=="CODEX_GRAPH";});
 if(selected==root.m_entries.end())return false;
 selected->m_handle=0x71u;
 if(io.fileExport(in.string(),DRW::AC1027,binary,&data,false,true)||bytes(in)!=saved)return false;
 root.m_entries=entries;
 if(!io.fileExport(in.string(),DRW::AC1027,binary,&data,false,true))return false;
 dx_data reopened;dx_iface reader;
 return reader.fileImport(in.string(),&reopened,false)&&reopened.xRecords.size()==1u;
}

static bool wideGraphFailedProjection(const std::filesystem::path& in,const std::filesystem::path& out,bool binary) {
 if(!defaultSource(in,binary,false,DRW::AC1027,true,nullptr,true,nullptr,true))return false;
 dx_data partial;Exporter io;if(!io.enableDxfWideGraphProjection())return false;
 if(io.fileImport(in.string(),&partial,false)||!DxWideProjectionTestAccess::incomplete(partial))return false;
 for(bool strict:{false,true}) {
  std::ofstream(out,std::ios::binary|std::ios::trunc)<<"sentinel";
  if(io.fileExport(out.string(),DRW::AC1027,binary,&partial,false,strict)||bytes(out)!="sentinel")return false;
 }
 if(!defaultSource(in,binary,false,DRW::AC1027,true,nullptr,true))return false;
 dx_data corrected;
 return io.fileImport(in.string(),&corrected,false)
     &&io.fileExport(out.string(),DRW::AC1027,binary,&corrected,false,true);
}

static bool bufferReadTest(const std::filesystem::path& path) {
 dxfRW codec(path.string().c_str());
 for(bool binary:{false,true}) {
  if(!source(path,binary))return false;
  std::string content=bytes(path);if(content.empty())return false;
  Facet capture;
  if(!codec.readBuffer(&capture,false,content)||capture.typed!=3||capture.raw!=3
     ||codec.getSourceVersion()!=DRW::AC1027)return false;
  const auto originalRetained=codec.dxfReadRetainedByteBudget();
  codec.setDxfReadRetainedByteBudget(content.size()-1u);
  Facet limited;
  if(codec.readBuffer(&limited,false,content)||limited.typed||limited.raw)return false;
  codec.setDxfReadRetainedByteBudget(originalRetained);
  const auto originalInput=codec.dxfReadInputByteBudget();
  codec.setDxfReadInputByteBudget(content.size()-1u);
  Facet truncated;
  if(codec.readBuffer(&truncated,false,content))return false;
  codec.setDxfReadInputByteBudget(originalInput);
  struct Nested : Facet {
   dxfRW* codec{nullptr};const std::string* data{nullptr};bool attempted{false},accepted{false};
   void addMaterial(const DRW_Material& value) override {
    if(!attempted){attempted=true;Facet nested;accepted=codec->readBuffer(&nested,false,*data);}
    Facet::addMaterial(value);
   }
  } nested;
  nested.codec=&codec;nested.data=&content;
  if(!codec.readBuffer(&nested,false,content)||!nested.attempted||nested.accepted
     ||nested.typed!=3||nested.raw!=3)return false;
  if(!binary){Facet ascii;if(!codec.readAscii(&ascii,false,content)||ascii.typed!=3||ascii.raw!=3)return false;}
 }
 std::ostringstream r12(std::ios::binary);
 r12.write("AutoCAD Binary DXF\r\n\x1a\0",22);
 dxfWriterBinaryR12 writer(&r12);writer.setVersion("AC1009",true);
 if(!writer.writeString(0,"SECTION")||!writer.writeString(2,"HEADER")
    ||!writer.writeString(9,"$ACADVER")||!writer.writeString(1,"AC1009")
    ||!writer.writeString(0,"ENDSEC")||!writer.writeString(0,"SECTION")
    ||!writer.writeString(2,"ENTITIES")||!writer.writeString(0,"LINE")
    ||!writer.writeString(8,"0")||!writer.writeDouble(10,1)||!writer.writeDouble(20,2)
    ||!writer.writeDouble(11,3)||!writer.writeDouble(21,4)
    ||!writer.writeString(0,"ENDSEC")||!writer.writeString(0,"EOF")||writer.hasWriteError())return false;
 struct LineCapture : LC3CompatCheck {unsigned lines{0};void addLine(const DRW_Line&)override{++lines;}} line;
 const auto r12bytes=r12.str();
 if(!codec.readBuffer(&line,false,r12bytes)||line.lines!=1u||codec.getSourceVersion()!=DRW::AC1009)return false;
 const std::string incomplete=r12bytes.substr(0u,22u);LineCapture failed;
 if(codec.readBuffer(&failed,false,incomplete)||failed.lines)return false;
 LineCapture retry;if(!codec.readBuffer(&retry,false,r12bytes)||retry.lines!=1u)return false;
 return true;
}
enum class WideSourceVariant { Wide, Narrow, WrongDictionaryOwner, WrongRecordOwner,
    WrongReference, UnknownReference, DuplicateSelf, DuplicateEntry, DuplicateNameCase, DuplicateTarget,
    MissingEnd, InvalidHandle, AlternateRole, AliasCollision, ExternalWideReference, BadSeed, MissingSeed, MultipleWideGraphs };
static std::string wideSourceBytes(bool binary,WideSourceVariant variant) {
    std::ostringstream out(std::ios::binary);
    if(binary)out.write("AutoCAD Binary DXF\r\n\x1a\0",22);
    std::unique_ptr<dxfWriter> writer=binary?std::unique_ptr<dxfWriter>(new dxfWriterBinary(&out))
        :std::unique_ptr<dxfWriter>(new dxfWriterAscii(&out));
    writer->setVersion("AC1027",true);
    const auto text=[&](int c,const char* v){return writer->writeString(c,v);};
    const auto number=[&](int c,int v){return writer->writeInt16(c,v);};
    const bool narrow=variant==WideSourceVariant::Narrow;
    const char* dict=narrow?"0000002f":"0000000100000010";
    const char* record=narrow?"30":"100000011";
    if(!text(0,"SECTION")||!text(2,"HEADER")||!text(9,"$ACADVER")||!text(1,"AC1027")
        ||(variant!=WideSourceVariant::MissingSeed
            &&(!text(9,"$HANDSEED")||!text(5,narrow?"100":variant==WideSourceVariant::BadSeed?"100000011":
                variant==WideSourceVariant::MultipleWideGraphs?"100000022":"100000012")))
        ||!text(0,"ENDSEC")||!text(0,"SECTION")||!text(2,"OBJECTS")
        ||!text(0,"DICTIONARY")||!text(5,"000a")||!text(330,"0")
        ||!text(100,"AcDbDictionary")||!number(281,1)||!text(3,"CODEX_GRAPH")
        ||!text(variant==WideSourceVariant::AlternateRole?360:350,variant==WideSourceVariant::InvalidHandle?"10000000000000000":dict))return {};
    if(variant==WideSourceVariant::DuplicateEntry&&(!text(3,"CODEX_GRAPH")||!text(350,dict)))return {};
    if(variant==WideSourceVariant::DuplicateNameCase&&(!text(3,"codex_graph")||!text(350,"B")))return {};
    if(variant==WideSourceVariant::DuplicateTarget&&(!text(3,"ALIAS")||!text(350,dict)))return {};
    if(variant==WideSourceVariant::MultipleWideGraphs&&(!text(3,"OTHER_GRAPH")||!text(350,"100000020")))return {};
    if(!text(0,"DICTIONARY")||!text(5,dict)||!text(330,variant==WideSourceVariant::WrongDictionaryOwner?"B":"A")
        ||!text(100,"AcDbDictionary")||!number(280,1)||!number(281,1)
        ||!text(3,"CODEX_RECORD")||!text(350,record)
        ||!text(0,"XRECORD")||!text(5,record)||!text(330,variant==WideSourceVariant::WrongRecordOwner?"A":dict)
        ||!text(100,"AcDbXrecord")||!number(280,1)||!text(1,"literal 100000010 untouched")
        ||!text(340,variant==WideSourceVariant::WrongReference?"A":dict))return {};
    if(variant==WideSourceVariant::UnknownReference&&!text(341,"A"))return {};
    if(variant==WideSourceVariant::DuplicateSelf&&(!text(0,"XRECORD")||!text(5,record)
        ||!text(330,dict)||!text(100,"AcDbXrecord")||!number(280,1)))return {};
    if(variant==WideSourceVariant::AliasCollision)for(const char* handle:{"1","2"})
        if(!text(0,"XRECORD")||!text(5,handle)||!text(330,"A")
            ||!text(100,"AcDbXrecord")||!number(280,1))return {};
    if(variant==WideSourceVariant::ExternalWideReference
        &&(!text(0,"XRECORD")||!text(5,"20")||!text(330,"A")
            ||!text(100,"AcDbXrecord")||!number(280,1)||!text(340,dict)))return {};
    if(variant==WideSourceVariant::MultipleWideGraphs
        &&(!text(0,"DICTIONARY")||!text(5,"100000020")||!text(330,"A")
            ||!text(100,"AcDbDictionary")||!number(281,1)||!text(3,"OTHER_RECORD")||!text(350,"100000021")
            ||!text(0,"XRECORD")||!text(5,"100000021")||!text(330,"100000020")
            ||!text(100,"AcDbXrecord")||!number(280,1)||!text(340,"100000020")))return {};
    if(!text(0,"ENDSEC")||(variant!=WideSourceVariant::MissingEnd&&!text(0,"EOF"))||writer->hasWriteError())return {};
    return out.str();
}
static bool runWideSourceCase(bool binary,WideSourceVariant variant,bool expected) {
    auto bytes=wideSourceBytes(binary,variant);if(bytes.empty())return false;
    std::istringstream input(bytes,std::ios::binary);
    dx_wide_dictionary_source::Graph graph;
    graph.root=123u;graph.rootSelf.text="stale";
    const bool ok=dx_wide_dictionary_source::capture(input,"CODEX_GRAPH","CODEX_RECORD",graph);
    if(ok!=expected)return false;
    if(!ok)return graph.root==0u&&graph.rootSelf.text.empty();
    const bool narrow=variant==WideSourceVariant::Narrow;
    const std::uint64_t dictionary=narrow?0x2fu:UINT64_C(0x100000010);
    const std::uint64_t record=narrow?0x30u:UINT64_C(0x100000011);
    return graph.binary==binary&&graph.root==0xau&&graph.dictionary==dictionary&&graph.record==record
        &&graph.rootSelf.text=="000a"&&graph.rootOwner.text=="0"
        &&graph.rootEntry.text==(narrow?"0000002f":"0000000100000010")
        &&graph.dictionarySelf.text==graph.rootEntry.text&&graph.dictionaryOwner.text=="A"
        &&graph.dictionaryEntry.text==(narrow?"30":"100000011")
        &&graph.recordSelf.text==graph.dictionaryEntry.text&&graph.recordOwner.text==graph.rootEntry.text
        &&graph.recordReference.text==graph.rootEntry.text
        &&graph.rootEntry.raw==graph.rootEntry.text&&graph.recordReference.raw==graph.recordReference.text
        &&graph.rootEntry.code==(variant==WideSourceVariant::AlternateRole?360:350)
        &&graph.dictionaryEntry.code==350&&graph.recordReference.code==340
        &&graph.rootSelf.ordinal<graph.rootEntry.ordinal&&graph.rootEntry.ordinal<graph.dictionarySelf.ordinal
        &&graph.dictionarySelf.ordinal<graph.dictionaryEntry.ordinal&&graph.dictionaryEntry.ordinal<graph.recordSelf.ordinal
        &&graph.recordSelf.ordinal<graph.recordReference.ordinal&&graph.recordReference.begin<graph.recordReference.end;
}
struct WideBufferCapture : LC3CompatCheck {
    std::uint32_t root{0},dictionary{0},record{0},reference{0};
    unsigned roots{0},dictionaries{0},records{0};
    void addNamedObjectDictionary(const DRW_Dictionary& value) override {
        ++roots;root=value.handle;
        for(const auto& entry:value.m_entries)if(entry.m_name=="CODEX_GRAPH")dictionary=entry.m_handle;
    }
    void addDictionary(const DRW_Dictionary& value) override {
        if(value.handle!=dictionary||value.parentHandle!=root)return;
        ++dictionaries;for(const auto& entry:value.m_entries)if(entry.m_name=="CODEX_RECORD")record=entry.m_handle;
    }
    void addXRecord(const DRW_XRecord& value) override {
        if(value.handle!=record||value.parentHandle!=dictionary)return;
        ++records;for(const auto& item:value.m_dataEntries)
            if(item.code()==340&&item.type()==DRW_Variant::INTEGER64)reference=static_cast<std::uint32_t>(item.i64_val());
    }
};
static bool wideSourceTest() {
    for(bool binary:{false,true}) {
        for(auto v:{WideSourceVariant::Wide,WideSourceVariant::Narrow,WideSourceVariant::AlternateRole})
            if(!runWideSourceCase(binary,v,true))return false;
        for(auto v:{WideSourceVariant::WrongDictionaryOwner,WideSourceVariant::WrongRecordOwner,
                    WideSourceVariant::WrongReference,WideSourceVariant::UnknownReference,
                    WideSourceVariant::DuplicateSelf,WideSourceVariant::DuplicateEntry,
                    WideSourceVariant::DuplicateNameCase,WideSourceVariant::DuplicateTarget,
                    WideSourceVariant::MissingEnd,WideSourceVariant::InvalidHandle})
            if(!runWideSourceCase(binary,v,false))return false;
        dx_wide_dictionary_source::Graph state;
        auto good=wideSourceBytes(binary,WideSourceVariant::Wide);
        auto bad=wideSourceBytes(binary,WideSourceVariant::WrongReference);
        std::istringstream first(good,std::ios::binary),invalid(bad,std::ios::binary),retry(good,std::ios::binary);
        if(!dx_wide_dictionary_source::capture(first,"CODEX_GRAPH","CODEX_RECORD",state)
            ||dx_wide_dictionary_source::capture(invalid,"CODEX_GRAPH","CODEX_RECORD",state)
            ||state.root!=0u||!state.rootSelf.text.empty()
            ||!dx_wide_dictionary_source::capture(retry,"CODEX_GRAPH","CODEX_RECORD",state))return false;
        for(auto v:{WideSourceVariant::Wide,WideSourceVariant::AliasCollision,WideSourceVariant::AlternateRole}) {
            auto original=wideSourceBytes(binary,v);
            dx_wide_dictionary_source::Normalized normalized;
            if(!dx_wide_dictionary_source::normalize(original,"CODEX_GRAPH","CODEX_RECORD",normalized)
                ||normalized.source.root!=0xau||normalized.source.dictionary!=UINT64_C(0x100000010)
                ||normalized.source.record!=UINT64_C(0x100000011)||normalized.root!=0xau
                ||normalized.dictionary!=(v==WideSourceVariant::AliasCollision?3u:1u)
                ||normalized.record!=(v==WideSourceVariant::AliasCollision?4u:2u)
                ||normalized.handSeed<=normalized.root||normalized.handSeed<=normalized.dictionary
                ||normalized.handSeed<=normalized.record||normalized.bytes==original
                ||normalized.bytes.find("literal 100000010 untouched")==std::string::npos)return false;
            dx_wide_dictionary_source::Graph projected;
            std::istringstream check(normalized.bytes,std::ios::binary);
            if(!dx_wide_dictionary_source::capture(check,"CODEX_GRAPH","CODEX_RECORD",projected)
                ||projected.root!=normalized.root||projected.dictionary!=normalized.dictionary
                ||projected.record!=normalized.record||projected.recordReference.code!=340
                ||projected.rootEntry.code!=(v==WideSourceVariant::AlternateRole?360:350))return false;
            WideBufferCapture delivered;dxfRW memory("");
            if(!memory.readBuffer(&delivered,false,normalized.bytes)||delivered.roots!=1u
                ||delivered.dictionaries!=1u||delivered.records!=1u||delivered.root!=normalized.root
                ||delivered.dictionary!=normalized.dictionary||delivered.record!=normalized.record
                ||delivered.reference!=normalized.dictionary)return false;
            dx_wide_dictionary_source::Normalized discovered;
            if(!dx_wide_dictionary_source::normalize(original,"","",discovered)
                ||discovered.bytes!=normalized.bytes||discovered.source.dictionaryKey!="CODEX_GRAPH"
                ||discovered.source.recordKey!="CODEX_RECORD")return false;
        }
        dx_wide_dictionary_source::Normalized normalized;
        normalized.bytes="stale";
        for(auto v:{WideSourceVariant::ExternalWideReference,WideSourceVariant::BadSeed,WideSourceVariant::MissingSeed,
                    WideSourceVariant::WrongReference,WideSourceVariant::InvalidHandle,WideSourceVariant::Narrow}) {
            const auto source=wideSourceBytes(binary,v);
            if(dx_wide_dictionary_source::normalize(source,"CODEX_GRAPH","CODEX_RECORD",normalized)
                ||!normalized.bytes.empty()||normalized.dictionary!=0u)return false;
        }
        const auto original=wideSourceBytes(binary,WideSourceVariant::Wide);
        const auto multiple=wideSourceBytes(binary,WideSourceVariant::MultipleWideGraphs);
        if(!dx_wide_dictionary_source::normalize(multiple,"","",normalized)
            ||normalized.chains.size()!=2u||normalized.source.chains.size()!=2u
            ||normalized.chains[0].dictionary==normalized.chains[1].dictionary
            ||normalized.chains[0].record==normalized.chains[1].record
            ||normalized.source.chains[1].dictionaryKey!="OTHER_GRAPH"
            ||normalized.source.chains[1].recordKey!="OTHER_RECORD")return false;
        // Explicitly selecting only one chain must not silently lose the other.
        if(dx_wide_dictionary_source::normalize(multiple,"CODEX_GRAPH","CODEX_RECORD",normalized)
            ||!normalized.bytes.empty())return false;
        if(!dx_wide_dictionary_source::normalize(original,"CODEX_GRAPH","CODEX_RECORD",normalized))return false;
        const std::string_view aliasedInput(normalized.bytes);
        if(dx_wide_dictionary_source::normalize(aliasedInput,"CODEX_GRAPH","CODEX_RECORD",normalized)
            ||!normalized.bytes.empty()
            ||!dx_wide_dictionary_source::normalize(original,"CODEX_GRAPH","CODEX_RECORD",normalized))return false;
        const std::string tooLarge(8u*1024u*1024u+1u,'x');
        if(dx_wide_dictionary_source::normalize(tooLarge,"CODEX_GRAPH","CODEX_RECORD",normalized)
            ||!normalized.bytes.empty())return false;
    }
    return true;
}
int main(int argc,char**argv){auto dir=std::filesystem::temp_directory_path()/("compact-material-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(dir);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{dir};auto in=dir/"in.dxf",out=dir/"out.dxf";
 for(bool binary:{false,true}){if(argc==2){std::filesystem::copy_file(argv[1],in,std::filesystem::copy_options::overwrite_existing);}else if(argc!=1||!source(in,binary))return 1;
  dxfRW direct(in.string().c_str());Old old;if(direct.read(&old,false)||old.typed||old.raw)return 2;Facet f;f.enabled=false;if(direct.read(&f,false)||f.typed||f.raw)return 3;f.enabled=true;if(!direct.read(&f,false)||f.typed!=3||f.raw!=3)return 4;
  DRW_Material copied=f.last;DRW_Material moved=std::move(copied);if(!moved.requiresRawDxfMaterialReplay())return 20;std::ofstream(out,std::ios::trunc)<<"sentinel";dxfRW standalone(out.string().c_str());DirectWriter typed(standalone);typed.material=moved;if(standalone.write(&typed,DRW::AC1027,binary)||typed.ok||bytes(out)!="sentinel")return 21;dwgRW dwg(out.string().c_str());DirectDwgWriter typedDwg(dwg);typedDwg.material=moved;if(dwg.write(&typedDwg,DRW::AC1027,true)||!typedDwg.registered||typedDwg.ok||bytes(out)!="sentinel")return 22;
  dx_data unavailable;Unaware unaware;if(unaware.fileImport(in.string(),&unavailable,false)||!unavailable.materials.empty()||!unavailable.rawMaterialObjects.empty())return 23;
  dx_data data;Exporter io;if(!io.fileImport(in.string(),&data,false)||!graph(data)){std::cerr<<"import failed "<<binary<<" materials="<<data.materials.size()<<" raw="<<data.rawMaterialObjects.size()<<" dict="<<data.dictionaries.size()<<"\n";return 5;}
  const auto exportFile=[&](){return io.fileExport(out.string(),DRW::AC1027,binary,&data,false,true);};const auto verify=[&](){dx_data check;dx_iface reader;return reader.fileImport(out.string(),&check,false)&&graph(check);};
  if(!exportFile()||!verify())return 6;
  {auto malformed=data.rawMaterialObjects.front();malformed.hasRawValues=true;malformed.rawValues.clear();DRW_RawDxfObject output;UTF8STRING name;const auto match=[](const DRW_Material&,const DRW_Material&,DRW::Version){return true;};if(dx_material_body::reconcile(malformed,malformed,data.materials.front(),output,name,binary,match))return 32;}
  const auto reject=[&](const std::function<void()>& mutate,const std::function<void()>& restore){std::ofstream(out,std::ios::trunc)<<"sentinel";mutate();const bool refused=!exportFile();restore();return refused&&bytes(out)=="sentinel"&&exportFile()&&verify();};
  for(auto& m:data.materials){auto owner=m.parentHandle;if(!reject([&](){m.parentHandle=0x66;},[&](){m.parentHandle=owner;}))return 7;auto reactor=m.reactorHandles;if(!reject([&](){m.reactorHandles[0]=0x66;},[&](){m.reactorHandles=reactor;}))return 8;auto flags=m.m_channelFlags;if(!reject([&](){m.m_channelFlags=63;},[&](){m.m_channelFlags=flags;}))return 9;if(!reject([&](){m.m_hasDiffuseMapSource=true;},[&](){m.m_hasDiffuseMapSource=false;}))return 31;}
  for(auto& raw:data.rawMaterialObjects){auto original=raw;if(!reject([&](){raw.groups.back()=DRW_Variant(94,126);},[&](){raw=original;}))return 10;for(std::size_t slot:{2u,4u})if(!reject([&](){raw.groups[slot]=DRW_Variant(340,std::string("67"));},[&](){raw=original;}))return 17;}
  auto removedRaw=data.rawMaterialObjects.back();if(!reject([&](){data.rawMaterialObjects.pop_back();},[&](){data.rawMaterialObjects.push_back(removedRaw);}))return 18;
  auto removed=data.materials.back();if(!reject([&](){data.materials.pop_back();},[&](){data.materials.push_back(removed);} ))return 11;
  auto& dict=data.dictionaries.front();auto edges=dict.m_entries;if(!reject([&](){dict.m_entries.pop_back();},[&](){dict.m_entries=edges;}))return 12;
  if(!reject([&](){dict.m_entries[1].m_handle=dict.m_entries[0].m_handle;},[&](){dict.m_entries=edges;}))return 24;
  if(!reject([&](){dict.m_entries[1].m_name=dict.m_entries[0].m_name;},[&](){dict.m_entries=edges;}))return 25;
  auto rootEdges=data.namedObjectDictionary.m_entries;if(!reject([&](){data.namedObjectDictionary.m_entries.clear();},[&](){data.namedObjectDictionary.m_entries=rootEdges;}))return 26;
  if(!reject([&](){data.namedObjectDictionary.m_entries.push_back({"ALIAS",0x67});},[&](){data.namedObjectDictionary.m_entries=rootEdges;}))return 27;
  auto saved=data.rawMaterialObjects.front();io.late=[&](){data.rawMaterialObjects.front().groups.back()=DRW_Variant(94,126);};std::ofstream(out,std::ios::trunc)<<"sentinel";const bool failed=!exportFile();io.late={};data.rawMaterialObjects.front()=saved;if(!failed||bytes(out)!="sentinel"||!exportFile()||!verify())return 13;
  auto savedFlags=data.materials.front().m_channelFlags;io.late=[&](){data.materials.front().m_channelFlags=126;};std::ofstream(out,std::ios::trunc)<<"sentinel";const bool typedFailed=!exportFile();io.late={};data.materials.front().m_channelFlags=savedFlags;if(!typedFailed||bytes(out)!="sentinel"||!exportFile()||!verify())return 19;
  std::ofstream(out,std::ios::trunc)<<"sentinel";if(io.fileExport(out.string(),DRW::AC1024,binary,&data,false,true)||bytes(out)!="sentinel"||!exportFile()||!verify())return 14;
  if(argc==1){if(!source(in,binary,8))return 28;dx_data prior;Exporter priorIo;if(!priorIo.fileImport(in.string(),&prior,false)||prior.materials.size()!=3)return 29;std::ofstream(out,std::ios::trunc)<<"sentinel";if(priorIo.fileExport(out.string(),DRW::AC1027,binary,&prior,false,true)||bytes(out)!="sentinel")return 30;for(int mode:{1,2,3,4,5,6,7}){if(!source(in,binary,mode))return 15;dx_data rejected;dx_iface rd;if(rd.fileImport(in.string(),&rejected,false))return 16;}}
 }
 if(argc==1)for(bool binary:{false,true})for(bool collision:{false,true})
  for(const auto version:{DRW::AC1024,DRW::AC1027})
   if(!defaultRoundTrip(in,out,binary,collision,version))return 33;
 if(argc==1)for(bool binary:{false,true})for(bool collision:{false,true})
  if(!multiPlotRoundTrip(in,out,binary,collision))return 34;
 if(argc==1)for(bool binary:{false,true})
  if(!multiPlotReactorRefusal(in,out,binary))return 35;
 if(argc==1)for(bool binary:{false,true})for(bool collision:{false,true}) {
  if(!wideGraphRoundTrip(in,out,binary,collision))return 38;
  if(!wideGraphTwoChains(in,out,binary,collision))return 39;
  if(!wideGraphSharedDictionary(in,out,binary,collision,false)
     ||!wideGraphSharedDictionary(in,out,binary,collision,1u)
     ||!wideGraphSharedDictionary(in,out,binary,collision,2u)
     ||!wideGraphSharedDictionary(in,out,binary,collision,2u,341)
     ||!wideGraphSharedDictionary(in,out,binary,collision,1u,349)
     ||!wideGraphSharedDictionary(in,out,binary,collision,2u,332)
     ||!wideGraphSharedDictionary(in,out,binary,collision,1u,339))return 41;
 }
 if(argc==1)for(bool binary:{false,true}) {
  if(!wideGraphChainBoundary(in,out,binary))return 40;
  if(!wideGraphCrossReferenceRetry(in,out,binary))return 42;
 }
 if(argc==1)for(bool binary:{false,true})
  if(!wideGraphImportRetry(in,out,binary))return 39;
 if(argc==1)for(bool binary:{false,true})
  if(!wideGraphSameFile(in,binary))return 40;
 if(argc==1)for(bool binary:{false,true})
  if(!wideGraphFailedProjection(in,out,binary))return 41;
 if(argc==1&&!wideSourceTest())return 36;
 if(argc==1&&!bufferReadTest(in))return 37;
 return 0;}
