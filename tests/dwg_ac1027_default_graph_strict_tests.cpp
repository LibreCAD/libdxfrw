// Historical positive/graph controls remain runnable via an explicit switch;
// they cannot pass until the selected source graph is fully qualified.
#include "dx_iface.h"
#include "dx_data.h"
#include "libdwgr.h"
#include "lc3_compat_check.cpp"
#include <algorithm>
#include <cstdint>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <functional>
class DxIfaceTableSourceTestAccess {
public:
 static auto& plots(dx_data& d) { return d.m_tableDxfSource.plotSourceAudit; }
 static auto& origin(dx_data& d, unsigned h) { return d.m_tableDxfSource.resourceOriginAudits.at(h); }
};
class DxIfaceStandardDwgCaptureTestAccess {
public:
 static DRW_UnsupportedObject& frame(dx_data& data, unsigned handle) { return data.m_opaqueDwgSourceFrames.at({data.importGeneration,handle}); }
 static DRW_UnsupportedObject& standard(dx_data& data, unsigned handle) { return data.m_standardDwgObjectFrames.at({data.importGeneration,handle}).raw; }
 static std::size_t qualified(const dx_data& data) { return data.m_opaqueDwgOutputQualifiedFrameIdentities.size(); }
 static std::optional<DRW_DwgAc1027LayerSource>& layerSource(dx_data& data) { return data.m_ac1027DefaultLayerSource; }
 static bool& layerDuplicate(dx_data& data) { return data.m_ac1027DefaultLayerDuplicate; }
 static std::uint64_t& layerGeneration(dx_data& data) { return data.m_ac1027DefaultLayerGeneration; }
 static auto& layerPublication(dx_data& data) { return data.m_ac1027LayerPublication; }
 static std::optional<dwgHandle>& headerMaterial(dx_data& data) { return data.m_ac1027HeaderMaterialSource; }
 static bool& headerDuplicate(dx_data& data) { return data.m_ac1027HeaderMaterialDuplicate; }
 static std::uint64_t& headerGeneration(dx_data& data) { return data.m_ac1027HeaderMaterialGeneration; }
};
// Mutate literal staged DXF bytes without a producer fixture. Binary strings
// include the two-byte group code and NUL terminator explicitly.
static std::string rawTag(int code,const std::string& value,bool binary) {
 if(!binary) { const std::string c=std::to_string(code);return std::string(3u-c.size(),' ')+c+"\n"+value+"\n"; }
 std::string out;out.push_back(static_cast<char>(code&255));out.push_back(static_cast<char>((code>>8)&255));out+=value;out.push_back('\0');return out;
}
static bool mutateRepeatSource(std::string& data,bool binary,const std::string& name,int mode) {
 const auto row=rawTag(0,"CLASS",binary);const auto endsec=rawTag(0,"ENDSEC",binary);
 const auto marker=rawTag(1,name,binary);const auto at=data.find(marker);if(at==std::string::npos)return false;
 const auto begin=data.rfind(row,at);const auto next=data.find(row,at);const auto last=data.find(endsec,at);
 if(begin==std::string::npos||last==std::string::npos)return false;
 const auto end=next==std::string::npos?last:std::min(next,last);if(end<=at)return false;
 if(mode==0) {
  const auto key=binary?std::string("\x5b\x00",2):std::string(" 91\n");
  const auto pos=data.find(key,at);if(pos==std::string::npos||pos>=end)return false;
  if(binary){if(pos+6u>end)return false;data[pos+2u]=char(99);data[pos+3u]=data[pos+4u]=data[pos+5u]=0;}
  else {const auto lineEnd=data.find('\n',pos+key.size());if(lineEnd==std::string::npos||lineEnd>=end)return false;data.replace(pos+key.size(),lineEnd-pos-key.size(),"   99");}
 } else if(mode==1) data.insert(end,rawTag(300,"unmodeled",binary));
 else if(mode==2) data.erase(begin,end-begin);
 else if(mode==3) data.insert(end,data.substr(begin,end-begin));
 else return false;
 return true;
}
static void flip(DRW_UnsupportedObject& frame,unsigned delta) { const auto bit=frame.m_rawBytes.size()*8u-frame.m_bodyBitSize+delta; frame.m_rawBytes.at(bit/8)^=static_cast<unsigned char>(1u<<(7u-bit%8)); }
static std::string contents(const std::string& p){std::ifstream in(p,std::ios::binary);return std::string(std::istreambuf_iterator<char>(in),{});}
class Provider final : public LC3CompatCheck, public DRW_DwgDefaultMaterialProvider, public DRW_DwgDefaultPlotStyleProvider {
public:
 explicit Provider(dwgRW& w): writer(w) {}
 bool wantsDwgDefaultMaterialGraph() const override { return true; }
 bool wantsDwgDefaultPlotStyleGraph() const override { return true; }
 bool dwgDefaultMaterialOpaqueReferencesAbsent(std::uint32_t a,std::uint32_t b,std::uint32_t c,std::uint32_t d) const override { return a && b && c && d; }
 void writeEntities() override { DRW_Line line;line.basePoint={1.,2.,3.};line.secPoint={4.,5.,6.};ok=writer.writeLine(&line); }
 bool finalizeDwgWrite() override { return ok; }
 dwgRW& writer; bool ok=false;
};
static int historicalPositiveControls(){
 const auto base=std::filesystem::temp_directory_path() / ("libdxfrw-ac1027-strict-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(base);
 const std::string input=(base/"line.dwg").string(); dwgRW dwg(input.c_str());Provider provider(dwg);if(!dwg.write(&provider,DRW::AC1027,true)||!provider.ok)return 2;
 dx_data data; dx_iface reader; if(!reader.fileImport(input,&data,false) || data.materials.size()!=3 || data.dictionaryWithDefaults.size()!=1 || data.placeholders.size()!=1)return 3;
 auto& mat=data.materials.front(); auto& matRaw=DxIfaceStandardDwgCaptureTestAccess::frame(data,mat.handle);
 auto materialDictIt=std::find_if(data.dictionaries.begin(),data.dictionaries.end(),[&](const DRW_Dictionary& d){return d.handle==mat.parentHandle;});
 if(materialDictIt==data.dictionaries.end())return 26;
 auto& materialDict=*materialDictIt;
 auto& materialDictRaw=DxIfaceStandardDwgCaptureTestAccess::standard(data,materialDict.handle);
 auto& dict=data.dictionaryWithDefaults.front(); auto& dictRaw=DxIfaceStandardDwgCaptureTestAccess::frame(data,dict.handle);
 auto& place=data.placeholders.front(); auto& placeRaw=DxIfaceStandardDwgCaptureTestAccess::frame(data,place.handle);
 // Public callbacks invoked after the read lease cannot create or alter source evidence.
 const auto priorHeader=DxIfaceStandardDwgCaptureTestAccess::headerMaterial(data);
 const auto priorLayer=DxIfaceStandardDwgCaptureTestAccess::layerSource(data);
 if(!priorHeader || !priorLayer)return 53;
 dwgHandle forged=*priorHeader;forged.ref=1u;reader.addDwgHeaderMaterialSource(DRW::AC1027,forged);
 auto forgedLayer=*priorLayer;forgedLayer.m_raw.m_handle=1u;reader.addDwgAc1027DefaultLayerSource(forgedLayer);
 if(DxIfaceStandardDwgCaptureTestAccess::headerDuplicate(data)
     || DxIfaceStandardDwgCaptureTestAccess::layerDuplicate(data)
     || DxIfaceStandardDwgCaptureTestAccess::headerMaterial(data)->ref!=priorHeader->ref
     || DxIfaceStandardDwgCaptureTestAccess::layerSource(data)->m_raw.m_handle!=priorLayer->m_raw.m_handle)return 54;
 for(bool bin: {false,true}) {
  dx_iface writer; const std::string path=(base / (bin?"binary.dxf":"ascii.dxf")).string();
  auto write=[&](){return writer.fileExport(path,DRW::AC1027,bin,&data,false,true,dx_dxfSourceTextPolicy::RequireValidSource,DRW::DxfTextWritePolicy::RequireLossless,dx_dxfProxyGraphicsPolicy::FailIfNotPreserved,dx_dwgReadCompletenessPolicy::RequireComplete);};
  const auto attempt=[&](const char* label,const std::function<void()>& mutate,const std::function<void()>& restore){
   {std::ofstream out(path,std::ios::binary|std::ios::trunc);out<<"sentinel";} mutate(); const bool accepted=write(); const bool kept=contents(path)=="sentinel";restore();const bool retry=write();
   std::cerr << (bin?"binary ":"ascii ") << label << " accepted="<<accepted<<" kept="<<kept<<" retry="<<retry<<'\n'; return !accepted && kept && retry;
  };
  if(!write() || DxIfaceStandardDwgCaptureTestAccess::qualified(data)!=8u)return 10;
  auto& integrity=data.dwgReadOutcome.observations.integrity;const auto savedIntegrity=integrity;
  if(!attempt("dropped integrity row",[&]{integrity.reportedUpstreamDropped=1u;},[&]{integrity=savedIntegrity;}))return 67;
  if(!attempt("unresolved entity reference",[&]{integrity.retained=1u;integrity.rows[0].severity=DwgIntegritySeverity::Error;integrity.rows[0].kind=DwgIntegrityCheckKind::EntityLayerReference;},[&]{integrity=savedIntegrity;}))return 68;
  {std::ofstream out(path,std::ios::binary|std::ios::trunc);out<<"sentinel";}
  if(writer.fileExport(path,DRW::AC1024,bin,&data,false,true,dx_dxfSourceTextPolicy::RequireValidSource,DRW::DxfTextWritePolicy::RequireLossless,dx_dxfProxyGraphicsPolicy::FailIfNotPreserved,dx_dwgReadCompletenessPolicy::RequireComplete) || contents(path)!="sentinel" || !write())return 27;
  for (auto& item:data.materials) {
   auto& wire=DxIfaceStandardDwgCaptureTestAccess::frame(data,item.handle);
   const auto flags=item.flags;
   if(!attempt("material common flags",[&]{item.flags^=1;},[&]{item.flags=flags;}))return 11;
   if(!attempt("material owner role all",[&]{flip(wire,1);},[&]{flip(wire,1);}))return 12;
   if(!attempt("material reactor role all",[&]{flip(wire,17);},[&]{flip(wire,17);}))return 13;
  }
  if(!attempt("material dictionary entry role",[&]{flip(materialDictRaw,16+2);},[&]{flip(materialDictRaw,16+2);}))return 14;
  if(!attempt("material dictionary name",[&]{materialDict.m_entries[0].m_name="ByWrong";},[&]{materialDict.m_entries[0].m_name="ByBlock";}))return 15;
  if(!attempt("plot entry role",[&]{flip(dictRaw,16+2);},[&]{flip(dictRaw,16+2);}))return 16;
  auto savedClasses=data.dwgClassCoverage.m_entries;
  if(!attempt("missing class row",[&]{data.dwgClassCoverage.m_entries.clear();},[&]{data.dwgClassCoverage.m_entries=savedClasses;}))return 17;
  if(!attempt("incomplete opaque capture",[&]{data.dwgReadOutcome.opaqueSourceCaptureFailed=true;},[&]{data.dwgReadOutcome.opaqueSourceCaptureFailed=false;}))return 18;
  auto savedNod=data.namedObjectDictionary.m_entries;
  if(!attempt("missing NOD key",[&]{data.namedObjectDictionary.m_entries.clear();},[&]{data.namedObjectDictionary.m_entries=savedNod;}))return 19;
  const auto plotKey=std::find_if(savedNod.begin(),savedNod.end(),[](const DRW_Dictionary::Entry& e){return e.m_name=="ACAD_PLOTSTYLENAME";});
  const auto materialKey=std::find_if(savedNod.begin(),savedNod.end(),[](const DRW_Dictionary::Entry& e){return e.m_name=="ACAD_MATERIAL";});
  if(plotKey==savedNod.end() || materialKey==savedNod.end())return 20;
  if(!attempt("plot NOD alias",[&]{auto alias=*plotKey;alias.m_name="PLOT_ALIAS";data.namedObjectDictionary.m_entries.push_back(alias);},[&]{data.namedObjectDictionary.m_entries=savedNod;}))return 21;
  if(!attempt("material NOD alias",[&]{auto alias=*materialKey;alias.m_name="MATERIAL_ALIAS";data.namedObjectDictionary.m_entries.push_back(alias);},[&]{data.namedObjectDictionary.m_entries=savedNod;}))return 22;
  if(!attempt("duplicate plot NOD key",[&]{data.namedObjectDictionary.m_entries.push_back(*plotKey);},[&]{data.namedObjectDictionary.m_entries=savedNod;}))return 23;
  if(!attempt("material class count",[&]{for(auto& c:data.dwgClassCoverage.m_entries)if(c.m_recordName=="MATERIAL")++c.m_instanceCount;},[&]{data.dwgClassCoverage.m_entries=savedClasses;}))return 24;
  if(!attempt("plot class row",[&]{for(auto& c:data.dwgClassCoverage.m_entries)if(c.m_recordName=="ACDBDICTIONARYWDFLT")c.m_classNumber++;},[&]{data.dwgClassCoverage.m_entries=savedClasses;}))return 25;
  auto& layer=data.layers.front(); const auto layerPlot=layer.plotStyleHandle; const auto layerMaterial=layer.materialHandle;
  if(!attempt("layer plot source role",[&]{layer.plotStyleHandle.code=4;},[&]{layer.plotStyleHandle=layerPlot;}))return 28;
  if(!attempt("layer plot source target",[&]{layer.plotStyleHandle.ref=1;},[&]{layer.plotStyleHandle=layerPlot;}))return 29;
  if(!attempt("layer material source role",[&]{layer.materialHandle.code=4;},[&]{layer.materialHandle=layerMaterial;}))return 30;
  if(!attempt("layer material source target",[&]{layer.materialHandle.ref=1;},[&]{layer.materialHandle=layerMaterial;}))return 31;
  const auto materialText=layer.handleMaterialS; const auto plotText=layer.handlePlotS;
  if(!attempt("layer material source string",[&]{layer.handleMaterialS="1";},[&]{layer.handleMaterialS=materialText;}))return 32;
  if(!attempt("layer plot source string",[&]{layer.handlePlotS="1";},[&]{layer.handlePlotS=plotText;}))return 33;
  auto& layerEvidence=DxIfaceStandardDwgCaptureTestAccess::layerSource(data);const auto savedLayerEvidence=layerEvidence;
  if(!savedLayerEvidence)return 40;
  if(!attempt("layer receipt missing",[&]{layerEvidence.reset();},[&]{layerEvidence=savedLayerEvidence;}))return 41;
  if(!attempt("layer receipt duplicate",[&]{DxIfaceStandardDwgCaptureTestAccess::layerDuplicate(data)=true;},[&]{DxIfaceStandardDwgCaptureTestAccess::layerDuplicate(data)=false;}))return 42;
  if(!attempt("layer receipt generation",[&]{++DxIfaceStandardDwgCaptureTestAccess::layerGeneration(data);},[&]{--DxIfaceStandardDwgCaptureTestAccess::layerGeneration(data);}))return 43;
  auto& publication=DxIfaceStandardDwgCaptureTestAccess::layerPublication(data);const auto savedPublication=publication;
  if(!attempt("layer publication missing",[&]{publication.valid=false;},[&]{publication=savedPublication;}))return 44;
  if(!attempt("layer publication duplicate",[&]{publication.duplicate=true;},[&]{publication=savedPublication;}))return 45;
  if(!attempt("layer publication offset",[&]{++publication.offset;},[&]{publication=savedPublication;}))return 46;
  if(!attempt("layer publication ordinal",[&]{++publication.ordinal;},[&]{publication=savedPublication;}))return 47;
  if(!attempt("layer receipt handle",[&]{++layerEvidence->m_raw.m_handle;},[&]{layerEvidence=savedLayerEvidence;}))return 48;
  if(!attempt("layer unknown tail role",[&]{flip(layerEvidence->m_raw,16+8+16+16+16+3);},[&]{layerEvidence=savedLayerEvidence;}))return 49;
  if(!attempt("layer unknown relative nonnull",[&]{flip(layerEvidence->m_raw,16+8+16+16+16+2);flip(layerEvidence->m_raw,16+8+16+16+16+3);},[&]{layerEvidence=savedLayerEvidence;}))return 55;
  const auto oldUnknown=layer.unknownHandle;
  if(!attempt("layer unknown typed",[&]{layer.unknownHandle.ref=1u;},[&]{layer.unknownHandle=oldUnknown;}))return 56;
  if(!attempt("layer xref tail role",[&]{flip(layerEvidence->m_raw,16+3);},[&]{layerEvidence=savedLayerEvidence;}))return 50;
  if(!attempt("layer xref flags bits",[&]{layerEvidence->m_raw.m_rawBytes.at(4)^=0x40;},[&]{layerEvidence=savedLayerEvidence;}))return 51;
  if(!attempt("layer discarded flags bits",[&]{layerEvidence->m_raw.m_rawBytes.at(5)^=0x01;},[&]{layerEvidence=savedLayerEvidence;}))return 52;
  auto& header=DxIfaceStandardDwgCaptureTestAccess::headerMaterial(data); const auto savedHeader=header;
  if(!savedHeader)return 34;
  if(!attempt("header missing",[&]{header.reset();},[&]{header=savedHeader;}))return 35;
  if(!attempt("header role",[&]{header->code=4;},[&]{header=savedHeader;}))return 36;
  if(!attempt("header target",[&]{header->ref=1;},[&]{header=savedHeader;}))return 37;
  if(!attempt("header duplicate",[&]{DxIfaceStandardDwgCaptureTestAccess::headerDuplicate(data)=true;},[&]{DxIfaceStandardDwgCaptureTestAccess::headerDuplicate(data)=false;}))return 38;
  if(!attempt("header generation",[&]{++DxIfaceStandardDwgCaptureTestAccess::headerGeneration(data);},[&]{--DxIfaceStandardDwgCaptureTestAccess::headerGeneration(data);}))return 39;
  if(!attempt("material typed",[&]{mat.m_diffuseColorFactor=.25;},[&]{mat.m_diffuseColorFactor=1.;}))return 4;
  if(!attempt("material body",[&]{matRaw.m_rawBytes[10]^=1;},[&]{matRaw.m_rawBytes[10]^=1;}))return 5;
  if(!attempt("material owner role",[&]{flip(matRaw,1);},[&]{flip(matRaw,1);}))return 6;
  if(!attempt("plot default role",[&]{flip(dictRaw,16+16+3);},[&]{flip(dictRaw,16+16+3);}))return 7;
  if(!attempt("placeholder owner role",[&]{flip(placeRaw,3);},[&]{flip(placeRaw,3);}))return 8;
  if(!attempt("plot scalar",[&]{dict.cloning=2;},[&]{dict.cloning=1;}))return 9;
 }
 // Repeat the exact strict output through the ordinary DXF input route. The
 // prior graph test has already qualified these files; equality after each
 // same-encoding pass also checks every HEADER/TABLES/OBJECTS field.
 for (bool binary : {false, true}) {
  const auto source=(base/(binary?"binary.dxf":"ascii.dxf")).string();
  dx_iface in; dx_data repeat; if(!in.fileImport(source,&repeat,false))return 60;
  if(repeat.layers.size()!=1u || repeat.dictionaryWithDefaults.size()!=1u
     || repeat.rawPlaceholders.size()!=1u)return 61;
  dx_iface out; const auto target=(base/(binary?"repeat-binary.dxf":"repeat-ascii.dxf")).string();
  auto save=[&]{return out.fileExport(target,DRW::AC1027,binary,&repeat,false,true,
     dx_dxfSourceTextPolicy::RequireValidSource,DRW::DxfTextWritePolicy::RequireLossless,
     dx_dxfProxyGraphicsPolicy::FailIfNotPreserved,dx_dwgReadCompletenessPolicy::RequireComplete);};
  auto failRetry=[&](const auto& bad,const auto& fix){
   {std::ofstream sentinel(target,std::ios::binary|std::ios::trunc);sentinel<<"sentinel";}
   bad(); const bool failed=!save(), intact=contents(target)=="sentinel"; fix();
   return failed&&intact&&save()&&contents(target)==contents(source);
  };
  if(!save()||contents(target)!=contents(source))return 62;
  auto& layer=repeat.layers.front();const auto plot=layer.handlePlotS;
  if(!failRetry([&]{layer.handlePlotS="7FFF";},[&]{layer.handlePlotS=plot;}))return 63;
  if(!failRetry([&]{layer.handlePlotS="F";},[&]{layer.handlePlotS=plot;}))return 64;
  auto& dict=repeat.dictionaryWithDefaults.front();const auto def=dict.m_defaultEntryHandle;
  if(!failRetry([&]{dict.m_defaultEntryHandle=0x7FFF;},[&]{dict.m_defaultEntryHandle=def;}))return 65;
  auto& root=repeat.namedObjectDictionary;const auto entries=root.m_entries;
  if(!failRetry([&]{root.m_entries.clear();},[&]{root.m_entries=entries;}))return 66;
  auto& placeholder=repeat.rawPlaceholders.front();const auto owner=placeholder.parentHandle;
  if(!failRetry([&]{placeholder.parentHandle=0x7FFF;},[&]{placeholder.parentHandle=owner;}))return 67;
  const auto savedClasses=repeat.dxfClasses;
  for(std::size_t i=0;i<savedClasses.size();++i) {
   if(!failRetry([&]{++repeat.dxfClasses[i].instanceCount;},[&]{repeat.dxfClasses=savedClasses;}))return 77;
  }
  if(!failRetry([&]{repeat.dxfClasses.clear();},[&]{repeat.dxfClasses=savedClasses;}))return 78;
  auto& rawDict=repeat.rawDictionaryWithDefaults.front();const auto rawName=rawDict.name;
  if(!failRetry([&]{rawDict.name="DICTIONARYWDFLT";},[&]{rawDict.name=rawName;}))return 79;
  auto& plotProof=DxIfaceTableSourceTestAccess::plots(repeat);const auto originalPlotProof=plotProof;
  if(!failRetry([&]{plotProof.reset();},[&]{plotProof=originalPlotProof;}))return 70;
  auto& layerProof=DxIfaceTableSourceTestAccess::origin(repeat,layer.handle);const auto originalLayerProof=layerProof;
  if(!failRetry([&]{layerProof.reset();},[&]{layerProof=originalLayerProof;}))return 71;
  auto& liveMaterial=repeat.materials.front();const auto diffuse=liveMaterial.m_diffuseColorFactor;
  if(!failRetry([&]{liveMaterial.m_diffuseColorFactor=.25;},[&]{liveMaterial.m_diffuseColorFactor=diffuse;}))return 100;
  const auto material=layer.handleMaterialS;
  if(!failRetry([&]{layer.handleMaterialS="7FFF";},[&]{layer.handleMaterialS=material;}))return 72;
  {std::ofstream sentinel(target,std::ios::binary|std::ios::trunc);sentinel<<"sentinel";}
  if(out.fileExport(target,DRW::AC1024,binary,&repeat,false,true,
     dx_dxfSourceTextPolicy::RequireValidSource,DRW::DxfTextWritePolicy::RequireLossless,
     dx_dxfProxyGraphicsPolicy::FailIfNotPreserved,dx_dwgReadCompletenessPolicy::RequireComplete)
     ||contents(target)!="sentinel"||!save())return 73;
  const auto cross=(base/(binary?"cross-ascii.dxf":"cross-binary.dxf")).string();
  if(!out.fileExport(cross,DRW::AC1027,!binary,&repeat,false,true,
     dx_dxfSourceTextPolicy::RequireValidSource,DRW::DxfTextWritePolicy::RequireLossless,
     dx_dxfProxyGraphicsPolicy::FailIfNotPreserved,dx_dwgReadCompletenessPolicy::RequireComplete))return 74;
  dx_iface crossReader;dx_data crossData;if(!crossReader.fileImport(cross,&crossData,false)
     ||crossData.layers.size()!=1u||crossData.dictionaryWithDefaults.size()!=1u||crossData.rawPlaceholders.size()!=1u
     ||crossData.layers.front().handlePlotS!=plot||crossData.layers.front().handleMaterialS!=material
     ||crossData.dictionaryWithDefaults.front().m_defaultEntryHandle!=def
     ||crossData.rawPlaceholders.front().parentHandle!=dict.handle)return 75;
  std::uint32_t sourceMaterial=0,crossMaterial=0;bool sourcePresent=false,crossPresent=false;
  if(!repeat.headerC.getDxfMaterialReferenceHandle(sourceMaterial,sourcePresent)
     ||!crossData.headerC.getDxfMaterialReferenceHandle(crossMaterial,crossPresent)
     ||!sourcePresent||!crossPresent||sourceMaterial!=crossMaterial)return 76;
  // The historical non-strict facade path remains unchanged; its literal
  // F fallback is not evidence of plot-style graph closure.
  if(!out.fileExport(target,DRW::AC1027,binary,&repeat,false,false,
     dx_dxfSourceTextPolicy::RequireValidSource,DRW::DxfTextWritePolicy::RequireLossless,
     dx_dxfProxyGraphicsPolicy::FailIfNotPreserved,dx_dwgReadCompletenessPolicy::RequireComplete))return 98;
  dx_iface legacyReader;dx_data legacy;if(!legacyReader.fileImport(target,&legacy,false)
      ||legacy.layers.size()!=1u||legacy.layers.front().handlePlotS!="F"||!save())return 99;
  const auto malformedInput=(base/(binary?"bad-binary.dxf":"bad-ascii.dxf")).string();
  auto physicalRefusal=[&](const std::string& bytes) {
   {std::ofstream f(malformedInput,std::ios::binary|std::ios::trunc);f.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));}
   {std::ofstream sentinel(target,std::ios::binary|std::ios::trunc);sentinel<<"sentinel";}
   dx_iface badReader;dx_data bad; const bool imported=badReader.fileImport(malformedInput,&bad,false);
   const bool rejected=!imported||!out.fileExport(target,DRW::AC1027,binary,&bad,false,true,
       dx_dxfSourceTextPolicy::RequireValidSource,DRW::DxfTextWritePolicy::RequireLossless,
       dx_dxfProxyGraphicsPolicy::FailIfNotPreserved,dx_dwgReadCompletenessPolicy::RequireComplete);
   return rejected&&contents(target)=="sentinel"&&save()&&contents(target)==contents(source);
  };
  for(const std::string name:{"MATERIAL","ACDBDICTIONARYWDFLT","ACDBPLACEHOLDER"}) {
   for(int mode=0;mode<4;++mode) {auto bad=contents(source);if(!mutateRepeatSource(bad,binary,name,mode)||!physicalRefusal(bad))return 80+mode;}
  }
  { auto bad=contents(source);const auto old=rawTag(390,plot,binary);
    const auto pos=bad.find(old);if(pos==std::string::npos)return 88;
    bad.replace(pos,old.size(),rawTag(390,"7FFF",binary));if(!physicalRefusal(bad))return 89; }
  { auto bad=contents(source);const auto old=rawTag(350,plot,binary);
    const auto pos=bad.find(old);if(pos==std::string::npos)return 92;
    bad.replace(pos,old.size(),rawTag(360,plot,binary));if(!physicalRefusal(bad))return 93; }
  { auto bad=contents(source);const auto object=rawTag(0,"ACDBDICTIONARYWDFLT",binary);
    const auto subclass=rawTag(100,"AcDbDictionary",binary);const auto objectPos=bad.find(object);
    const auto pos=objectPos==std::string::npos?objectPos:bad.find(subclass,objectPos);
    if(pos==std::string::npos)return 101;
    const std::string flag=binary?std::string("\x18\x01\x00\x00",4):std::string("280\n    0\n");
    bad.insert(pos+subclass.size(),flag);if(!physicalRefusal(bad))return 102; }
  { auto bad=contents(source);const auto old=rawTag(1,"ACDBDICTIONARYWDFLT",binary);
    const auto pos=bad.find(old);if(pos==std::string::npos)return 90;
    bad.replace(pos,old.size(),rawTag(1,"DICTIONARYWDFLT",binary));if(!physicalRefusal(bad))return 91; }
  { auto bad=contents(source);const auto old=rawTag(0,"ACDBDICTIONARYWDFLT",binary);
    const auto pos=bad.find(old);if(pos==std::string::npos)return 84;
    bad.replace(pos,old.size(),rawTag(0,"DICTIONARYWDFLT",binary));if(!physicalRefusal(bad))return 85; }
  for(unsigned occurrence=0u;occurrence<3u;++occurrence) {
    auto bad=contents(source);const auto marker=rawTag(100,"AcDbMaterial",binary);
    std::size_t pos=0u;for(unsigned i=0;i<=occurrence;++i) {pos=bad.find(marker,pos);if(pos==std::string::npos)return 94;if(i<occurrence)pos+=marker.size();}
    bad.insert(pos+marker.size(),rawTag(300,"unmodeled",binary));if(!physicalRefusal(bad))return 95;
  }
  { auto bad=contents(source);const auto marker=rawTag(100,"AcDbLayerTableRecord",binary);
    const auto pos=bad.find(marker);if(pos==std::string::npos)return 96;
    bad.insert(pos+marker.size(),rawTag(300,"unmodeled",binary));if(!physicalRefusal(bad))return 97; }
  for(unsigned occurrence=0u;occurrence<3u;++occurrence) {
    auto bad=contents(source);const auto marker=rawTag(100,"AcDbDictionary",binary);
    std::size_t pos=0u;for(unsigned i=0;i<=occurrence;++i) {pos=bad.find(marker,pos);if(pos==std::string::npos)return 86;if(i<occurrence)pos+=marker.size();}
    bad.insert(pos+marker.size(),rawTag(300,"unmodeled",binary));if(!physicalRefusal(bad))return 87;
  }
  dx_iface secondReader;dx_data second;if(!secondReader.fileImport(target,&second,false))return 68;
  dx_iface secondWriter;const auto target2=(base/(binary?"repeat2-binary.dxf":"repeat2-ascii.dxf")).string();
  if(!secondWriter.fileExport(target2,DRW::AC1027,binary,&second,false,true,
      dx_dxfSourceTextPolicy::RequireValidSource,DRW::DxfTextWritePolicy::RequireLossless,
      dx_dxfProxyGraphicsPolicy::FailIfNotPreserved,dx_dwgReadCompletenessPolicy::RequireComplete)
      ||contents(target2)!=contents(source))return 69;
 }
 std::filesystem::remove_all(base);
 return 0;
}

class GuardProvider final : public LC3CompatCheck, public DRW_DwgDefaultMaterialProvider, public DRW_DwgDefaultPlotStyleProvider {
public:
 explicit GuardProvider(dwgRW& w): writer(w) {}
 bool wantsDwgDefaultMaterialGraph() const override { return true; }
 bool wantsDwgDefaultPlotStyleGraph() const override { return true; }
 bool dwgDefaultMaterialOpaqueReferencesAbsent(std::uint32_t a,std::uint32_t b,std::uint32_t c,std::uint32_t d) const override { return a && b && c && d; }
 void writeEntities() override { DRW_Line line;line.basePoint={1.,2.,3.};line.secPoint={4.,5.,6.};ok=writer.writeLine(&line); }
 bool finalizeDwgWrite() override { return ok; }
 dwgRW& writer;bool ok=false;
};
class ReentrantGuardWriter final : public dx_iface {
public:
 dx_data* data{nullptr};std::string input;bool attempted{false},imported{false};
 void writeHeader(DRW_Header& header) override {
  attempted=true;dx_iface other;imported=other.fileImport(input,data,false);
  dx_iface::writeHeader(header);
 }
 bool finalizeDxfWrite() override { return true; }
};
int main(int argc,char** argv) {
 if(argc==2 && std::string(argv[1])=="--historical-positive-controls")return historicalPositiveControls();
 const auto base=std::filesystem::temp_directory_path() / ("libdxfrw-ac1027-guard-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(base);
 const std::string input=(base/"line.dwg").string();dwgRW dwg(input.c_str());GuardProvider provider(dwg);
 if(!dwg.write(&provider,DRW::AC1027,true)||!provider.ok)return 2;
 dx_data data;dx_iface reader;if(!reader.fileImport(input,&data,false)||data.materials.size()!=3u||data.dictionaryWithDefaults.size()!=1u)return 3;
 if(!DxIfaceStandardDwgCaptureTestAccess::headerMaterial(data)||!DxIfaceStandardDwgCaptureTestAccess::layerSource(data))return 4;
 for(bool bin:{false,true}) {
  dx_iface writer;const auto output=(base/(bin?"binary.dxf":"ascii.dxf")).string();
  const auto save=[&](dx_dwgReadCompletenessPolicy p,bool owners) {return writer.fileExport(output,DRW::AC1027,bin,&data,false,owners,dx_dxfSourceTextPolicy::RequireValidSource,DRW::DxfTextWritePolicy::RequireLossless,dx_dxfProxyGraphicsPolicy::FailIfNotPreserved,p);};
  const auto blocked=[&](dx_dwgReadCompletenessPolicy p,bool owners) {
   {std::ofstream out(output,std::ios::binary|std::ios::trunc);out<<"sentinel";}
   return !save(p,owners)&&contents(output)=="sentinel";
  };
  for(auto mode:{std::make_pair(dx_dwgReadCompletenessPolicy::RequireComplete,false),std::make_pair(dx_dwgReadCompletenessPolicy::RequireComplete,true),std::make_pair(dx_dwgReadCompletenessPolicy::AllowPartial,false),std::make_pair(dx_dwgReadCompletenessPolicy::AllowPartial,true),std::make_pair(dx_dwgReadCompletenessPolicy::Legacy,true)}) {
   if(!blocked(mode.first,mode.second)||!blocked(mode.first,mode.second))return 5;
   const auto format=data.sourceFormat;const auto version=data.sourceVersion;const auto generation=data.importGeneration;
   data.sourceFormat=dx_source_format::Unknown;data.sourceVersion=DRW::UNKNOWNV;++data.importGeneration;
   if(!blocked(mode.first,mode.second))return 13;
   data.sourceFormat=format;data.sourceVersion=version;data.importGeneration=generation;
   auto savedMaterials=std::move(data.materials);if(!blocked(mode.first,mode.second))return 6;data.materials=std::move(savedMaterials);
   auto savedDictionaries=std::move(data.dictionaryWithDefaults);if(!blocked(mode.first,mode.second))return 7;data.dictionaryWithDefaults=std::move(savedDictionaries);
   auto header=DxIfaceStandardDwgCaptureTestAccess::headerMaterial(data);DxIfaceStandardDwgCaptureTestAccess::headerMaterial(data).reset();
   if(!blocked(mode.first,mode.second))return 8;
   DxIfaceStandardDwgCaptureTestAccess::headerMaterial(data)=header;
   auto layer=DxIfaceStandardDwgCaptureTestAccess::layerSource(data);DxIfaceStandardDwgCaptureTestAccess::layerSource(data).reset();
   if(!blocked(mode.first,mode.second))return 9;
   DxIfaceStandardDwgCaptureTestAccess::layerSource(data)=layer;
   auto& layerGeneration=DxIfaceStandardDwgCaptureTestAccess::layerGeneration(data);
   auto& headerGeneration=DxIfaceStandardDwgCaptureTestAccess::headerGeneration(data);
   auto& layerDuplicate=DxIfaceStandardDwgCaptureTestAccess::layerDuplicate(data);
   const auto oldLayerGeneration=layerGeneration,oldHeaderGeneration=headerGeneration;
   const bool oldDuplicate=layerDuplicate;
   layerGeneration=0u;headerGeneration=0u;layerDuplicate=true;
   if(!blocked(mode.first,mode.second))return 16;
   layerGeneration=oldLayerGeneration;headerGeneration=oldHeaderGeneration;layerDuplicate=oldDuplicate;
  }
  // Legacy without owner checks remains on its prior unguarded route; this
  // particular source may still fail later, for unrelated output limits.
  (void)writer.fileExport(output,DRW::AC1027,bin,&data,false,false);
 }
 {
  dx_data fresh;ReentrantGuardWriter out;out.data=&fresh;out.input=input;
  const auto target=(base/"reentrant.dxf").string();const auto generation=fresh.importGeneration;
  const bool wrote=out.fileExport(target,DRW::AC1027,false,&fresh,false,false,
      dx_dxfSourceTextPolicy::Compatible,DRW::DxfTextWritePolicy::RequireLossless,
      dx_dxfProxyGraphicsPolicy::FailIfNotPreserved,dx_dwgReadCompletenessPolicy::RequireComplete);
  if(!wrote||!out.attempted||out.imported||fresh.importGeneration!=generation)return 11;
  dx_iface reimport;if(!reimport.fileImport(target,&data,false))return 14;
  dx_iface check;const auto blockedTarget=(base/"after-reimport.dxf").string();
  {std::ofstream sentinel(blockedTarget,std::ios::binary|std::ios::trunc);sentinel<<"sentinel";}
  if(check.fileExport(blockedTarget,DRW::AC1027,false,&data,false,false,dx_dxfSourceTextPolicy::RequireValidSource,
       DRW::DxfTextWritePolicy::RequireLossless,dx_dxfProxyGraphicsPolicy::FailIfNotPreserved,
       dx_dwgReadCompletenessPolicy::RequireComplete)||contents(blockedTarget)!="sentinel")return 15;
  dx_iface after;if(!after.fileImport(input,&fresh,false))return 12;
 }
 std::filesystem::remove_all(base);return 0;
}
