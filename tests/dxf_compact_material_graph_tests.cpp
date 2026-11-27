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
static bool defaultSource(const std::filesystem::path& path, bool binary, bool collision, DRW::Version version, bool multiPlot=false, const char* xrecordReactor=nullptr) {
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
 if(!s(0,"SECTION")||!s(2,"HEADER")||!s(9,"$ACADVER")||!s(1,versionName)
    ||!s(9,"$CMATERIAL")||!s(347,blockHandle)||!s(0,"ENDSEC")
    ||!s(0,"SECTION")||!s(2,"TABLES")||!s(0,"TABLE")||!s(2,"LAYER")
    ||(multiPlot&&(!s(5,"2")||!s(330,"0")||!s(100,"AcDbSymbolTable")))||!h(70,multiPlot?2:1)
    ||!s(0,"LAYER")||!s(5,"90")||(multiPlot&&!s(330,"2"))||!s(100,"AcDbSymbolTableRecord")||!s(100,"AcDbLayerTableRecord")
    ||!s(2,"RAWTEST")||!h(70,0)||!h(62,7)||!s(6,"CONTINUOUS")||!s(347,blockHandle)
    ||(multiPlot&&!s(390,"71")))return false;
 if(multiPlot&&(!s(0,"LAYER")||!s(5,"91")||!s(330,"2")||!s(100,"AcDbSymbolTableRecord")
    ||!s(100,"AcDbLayerTableRecord")||!s(2,"0")||!h(70,0)||!h(62,7)
    ||!s(6,"CONTINUOUS")||!s(347,blockHandle)||!s(390,"71")))return false;
 if(!s(0,"ENDTAB")||!s(0,"ENDSEC")||!s(0,"SECTION")||!s(2,"OBJECTS")
    ||!s(0,"DICTIONARY")||!s(5,"C")||!s(330,"0")||!s(100,"AcDbDictionary")
    ||!h(280,0)||!h(281,1)||(multiPlot&&(!s(3,"ACAD_GROUP")||!s(350,"D")))
    ||!s(3,"ACAD_MATERIAL")||!s(350,"67")
    ||(multiPlot&&(!s(3,"ACAD_PLOTSTYLENAME")||!s(350,"70")
                    ||!s(3,"CODEX_GRAPH")||!s(350,"72"))))return false;
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
    ||!s(0,"DICTIONARY")||!s(5,"72")||!s(330,"C")||!s(100,"AcDbDictionary")
    ||!h(280,0)||!h(281,1)||!s(3,"RECORD")||!s(350,"73")
    ||!s(0,"XRECORD")||!s(5,"73")
    ||(xrecordReactor&&(!s(102,"{ACAD_REACTORS")||!s(330,xrecordReactor)||!s(102,"}")))
    ||!s(330,"72")||!s(100,"AcDbXrecord")
    ||!h(280,1)||!s(1,"payload")||!s(340,"72")))return false;
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
enum class WideSourceVariant { Wide, Narrow, WrongDictionaryOwner, WrongRecordOwner,
    WrongReference, UnknownReference, DuplicateSelf, DuplicateEntry, DuplicateNameCase, DuplicateTarget,
    MissingEnd, InvalidHandle, AlternateRole, AliasCollision, ExternalWideReference, BadSeed, MissingSeed };
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
            &&(!text(9,"$HANDSEED")||!text(5,narrow?"100":variant==WideSourceVariant::BadSeed?"100000011":"100000012")))
        ||!text(0,"ENDSEC")||!text(0,"SECTION")||!text(2,"OBJECTS")
        ||!text(0,"DICTIONARY")||!text(5,"000a")||!text(330,"0")
        ||!text(100,"AcDbDictionary")||!number(281,1)||!text(3,"CODEX_GRAPH")
        ||!text(variant==WideSourceVariant::AlternateRole?360:350,variant==WideSourceVariant::InvalidHandle?"10000000000000000":dict))return {};
    if(variant==WideSourceVariant::DuplicateEntry&&(!text(3,"CODEX_GRAPH")||!text(350,dict)))return {};
    if(variant==WideSourceVariant::DuplicateNameCase&&(!text(3,"codex_graph")||!text(350,"B")))return {};
    if(variant==WideSourceVariant::DuplicateTarget&&(!text(3,"ALIAS")||!text(350,dict)))return {};
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
 if(argc==1&&!wideSourceTest())return 36;
 return 0;}
