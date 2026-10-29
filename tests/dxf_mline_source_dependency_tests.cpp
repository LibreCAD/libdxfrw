// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_iface.h"
#include "dx_mline_dictionary_source_live.h"
#include "dx_mline_layer_source_live.h"
#include "dx_mline_ltype_source_live.h"
#include "test_scratch.h"
#include "intern/dxfwriter.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <list>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
class DxWholeSourceTestAccess {
public:
    static auto& capture(dx_data& data){return data.m_tableDxfSource;}
    static void limits(dx_iface& a,WholeSourceV2::Limits value){a.m_wholeLimits=value;}
    static std::size_t peak(const dx_data& data){const auto& p=data.m_tableDxfSource.wholeInput;return p&&p->source?p->source->peakBytes():0u;}
};
class DxIfaceMlineDependencyTestAccess {
public:
    static bool rootDictionary(const dx_iface& a,const dx_data& d,std::size_t work,std::size_t* used) noexcept{return a.mlineRootDictionarySourceLiveProof(d,work,used);}
    static bool styleDictionary(const dx_iface& a,const dx_data& d,std::size_t work,std::size_t* used) noexcept{return a.mlineStyleDictionarySourceLiveProof(d,work,used);}
    static bool styleBodies(const dx_iface& a,const dx_data& d,std::size_t work,std::size_t* used) noexcept{return a.mlineStyleBodiesSourceLiveProof(d,work,used);}
    static bool layers(const dx_iface& a,const dx_data& d,std::size_t work,std::size_t* used) noexcept{return a.mlineLayerSourceLiveProof(d,work,used);}
    static bool ltypes(const dx_iface& a,const dx_data& d,std::size_t work,std::size_t* used) noexcept{return a.mlineLtypeSourceLiveProof(d,work,used);}
    static dx_mline_source_dependency::Proof qualify(const dx_iface& a,const dx_data& data,
            dx_mline_source_dependency::Limits limits={}) noexcept{return a.mlineDependencySourceProof(data,limits);}
};
class Consumer:public dx_iface {
public: bool import(const std::string& path,dx_data& data){return fileImport(path,&data,false);}
};
class LayerMutationAtFinalizeConsumer final:public Consumer {
public:
    dx_data* mutationTarget{nullptr};
    bool injectAtFinalize{false},injected{false};
    bool finalizeDxfWrite() override {
        if(!injectAtFinalize)return dx_iface::finalizeDxfWrite();
        if(!mutationTarget||mutationTarget->layers.empty())return false;
        auto& appData=mutationTarget->layers.front().appData;
        appData.push_back({{102,std::string("{TEST_HIDDEN")},{340,std::string("D2")},{102,std::string("}")}});
        injected=true;
        const bool result=dx_iface::finalizeDxfWrite();
        appData.clear();
        return result;
    }
};
class DictionaryMutationAtFinalizeConsumer final:public Consumer {
public:
    dx_data* mutationTarget{nullptr};
    bool injectAtFinalize{false},injected{false};
    bool finalizeDxfWrite() override {
        if(!injectAtFinalize)return dx_iface::finalizeDxfWrite();
        if(!mutationTarget||mutationTarget->dictionaries.empty()
            ||mutationTarget->dictionaries.front().m_entries.empty())return false;
        auto& entry=mutationTarget->dictionaries.front().m_entries.front();
        const auto savedHandle=entry.m_handle;
        entry.m_handle=0xBu;
        injected=true;
        const bool result=dx_iface::finalizeDxfWrite();
        entry.m_handle=savedHandle;
        return result;
    }
};
class SpoofingConsumer:public Consumer {
public:
    bool corrupt=false,skip=false;
    void wholeSourcePending(const void* reader,const WholeSourceV2::Source& source,
            const WholeSourceV2::Ticket& ticket) noexcept override {
        if(skip)return;
        if(corrupt){const auto field=source.field(0);const auto value=source.value(field);
            if(!value.empty())const_cast<char*>(value.data())[0]^=1;}
        dx_iface::wholeSourcePending(reader,source,ticket);
    }
};
// All test inputs are generated locally; no drawing fixture is committed.
static std::string sourceText() {
    const std::pair<int,const char*> tags[] = {
        {  0, "SECTION"}, {  2, "HEADER"}, {  9, "$ACADVER"}, {  1, "AC1027"},
        {  9, "$DWGCODEPAGE"}, {  3, "ANSI_1252"}, {  0, "ENDSEC"}, {  0, "SECTION"},
        {  2, "TABLES"}, {  0, "TABLE"}, {  2, "LTYPE"}, {  5, "100"},
        {100, "AcDbSymbolTable"}, { 70, "    2"}, {  0, "LTYPE"}, {  5, "101"},
        {330, "100"}, {100, "AcDbSymbolTableRecord"}, {100, "AcDbLinetypeTableRecord"}, {  2, "ByLayer"},
        { 70, "    0"}, {  3, ""}, { 72, "   65"}, { 73, "    0"},
        { 40, "0"}, {  0, "LTYPE"}, {  5, "102"}, {330, "100"},
        {100, "AcDbSymbolTableRecord"}, {100, "AcDbLinetypeTableRecord"}, {  2, "Continuous"}, { 70, "    0"},
        {  3, ""}, { 72, "   65"}, { 73, "    0"}, { 40, "0"},
        {  0, "ENDTAB"}, {  0, "TABLE"}, {  2, "LAYER"}, {  5, "1E0"},
        {100, "AcDbSymbolTable"}, { 70, "    1"}, {  0, "LAYER"}, {  5, "1E1"},
        {330, "1E0"}, {100, "AcDbSymbolTableRecord"}, {100, "AcDbLayerTableRecord"}, {  2, "0"},
        { 70, "    0"}, { 62, "    7"}, {  6, "Continuous"}, {  0, "ENDTAB"},
        {  0, "TABLE"}, {  2, "BLOCK_RECORD"}, {  5, "200"}, {100, "AcDbSymbolTable"},
        { 70, "    3"}, {  0, "BLOCK_RECORD"}, {  5, "201"}, {330, "200"},
        {100, "AcDbSymbolTableRecord"}, {100, "AcDbBlockTableRecord"}, {  2, "*Model_Space"}, { 70, "    0"},
        {  0, "BLOCK_RECORD"}, {  5, "202"}, {330, "200"}, {100, "AcDbSymbolTableRecord"},
        {100, "AcDbBlockTableRecord"}, {  2, "*Paper_Space"}, { 70, "    0"}, {  0, "BLOCK_RECORD"},
        {  5, "203"}, {330, "200"}, {100, "AcDbSymbolTableRecord"}, {100, "AcDbBlockTableRecord"},
        {  2, "AuthorityBlock"}, { 70, "    0"}, {  0, "ENDTAB"}, {  0, "ENDSEC"},
        {  0, "SECTION"}, {  2, "BLOCKS"}, {  0, "BLOCK"}, {  5, "310"},
        {330, "201"}, {100, "AcDbEntity"}, {  8, "0"}, {100, "AcDbBlockBegin"},
        {  2, "*Model_Space"}, { 70, "    0"}, { 10, "0"}, { 20, "0"},
        { 30, "0"}, {  3, "*Model_Space"}, {  1, ""}, {  0, "ENDBLK"},
        {  5, "311"}, {330, "201"}, {100, "AcDbEntity"}, {  8, "0"},
        {100, "AcDbBlockEnd"}, {  0, "BLOCK"}, {  5, "312"}, {330, "202"},
        {100, "AcDbEntity"}, {  8, "0"}, {100, "AcDbBlockBegin"}, {  2, "*Paper_Space"},
        { 70, "    0"}, { 10, "0"}, { 20, "0"}, { 30, "0"},
        {  3, "*Paper_Space"}, {  1, ""}, {  0, "ENDBLK"}, {  5, "313"},
        {330, "202"}, {100, "AcDbEntity"}, {  8, "0"}, {100, "AcDbBlockEnd"},
        {  0, "BLOCK"}, {  5, "300"}, {330, "203"}, {100, "AcDbEntity"},
        {  8, "0"}, {100, "AcDbBlockBegin"}, {  2, "AuthorityBlock"}, { 70, "    0"},
        { 10, "0"}, { 20, "0"}, { 30, "0"}, {  3, "AuthorityBlock"},
        {  1, ""}, {  0, "MLINE"}, {  5, "D101"}, {330, "203"},
        {100, "AcDbEntity"}, { 62, "    2"}, { 48, "1.25"}, {  8, "0"},
        {100, "AcDbMline"}, {  2, "OLD_ENTITY_NAME"}, {340, "D"}, { 40, "1"},
        { 70, "    1"}, { 71, "    1"}, { 72, "    2"}, { 73, "    2"},
        { 10, "4"}, { 20, "2"}, { 30, "3"}, {210, "0"},
        {220, "0"}, {230, "1"}, { 11, "4"}, { 21, "2"},
        { 31, "3"}, { 12, "1"}, { 22, "0"}, { 32, "0"},
        { 13, "0"}, { 23, "1"}, { 33, "0"}, { 74, "    2"},
        { 41, "0.5"}, { 41, "0"}, { 75, "    0"}, { 74, "    2"},
        { 41, "-0.25"}, { 41, "0"}, { 75, "    0"}, { 11, "14"},
        { 21, "2"}, { 31, "3"}, { 12, "1"}, { 22, "0"},
        { 32, "0"}, { 13, "0"}, { 23, "1"}, { 33, "0"},
        { 74, "    2"}, { 41, "0.5"}, { 41, "0"}, { 75, "    0"},
        { 74, "    2"}, { 41, "-0.25"}, { 41, "0"}, { 75, "    0"},
        {  0, "ENDBLK"}, {  5, "301"}, {330, "203"}, {100, "AcDbEntity"},
        {  8, "0"}, {100, "AcDbBlockEnd"}, {  0, "ENDSEC"}, {  0, "SECTION"},
        {  2, "ENTITIES"}, {  0, "MLINE"}, {  5, "D100"}, {330, "201"},
        {100, "AcDbEntity"}, { 62, "    2"}, { 48, "1.25"}, {  8, "0"},
        {100, "AcDbMline"}, {  2, "OLD_ENTITY_NAME"}, {340, "D"}, { 40, "1"},
        { 70, "    1"}, { 71, "    1"}, { 72, "    2"}, { 73, "    2"},
        { 10, "1"}, { 20, "2"}, { 30, "3"}, {210, "0"},
        {220, "0"}, {230, "1"}, { 11, "1"}, { 21, "2"},
        { 31, "3"}, { 12, "1"}, { 22, "0"}, { 32, "0"},
        { 13, "0"}, { 23, "1"}, { 33, "0"}, { 74, "    2"},
        { 41, "0.5"}, { 41, "0"}, { 75, "    0"}, { 74, "    2"},
        { 41, "-0.25"}, { 41, "0"}, { 75, "    0"}, { 11, "11"},
        { 21, "2"}, { 31, "3"}, { 12, "1"}, { 22, "0"},
        { 32, "0"}, { 13, "0"}, { 23, "1"}, { 33, "0"},
        { 74, "    2"}, { 41, "0.5"}, { 41, "0"}, { 75, "    0"},
        { 74, "    2"}, { 41, "-0.25"}, { 41, "0"}, { 75, "    0"},
        {  0, "ENDSEC"}, {  0, "SECTION"}, {  2, "OBJECTS"}, {  0, "DICTIONARY"},
        {  5, "C"}, {330, "0"}, {100, "AcDbDictionary"}, {280, "    0"},
        {281, "    1"}, {  3, "ACAD_MLINESTYLE"}, {350, "A"}, {  0, "DICTIONARY"},
        {  5, "A"}, {330, "C"}, {100, "AcDbDictionary"}, {280, "    0"},
        {281, "    1"}, {  3, "EFFECTIVE_STYLE"}, {350, "D"}, {  3, "RETARGET_STYLE"},
        {350, "D2"}, {  0, "MLINESTYLE"}, {  5, "D"}, {102, "{ACAD_REACTORS"},
        {330, "A"}, {102, "}"}, {330, "A"}, {100, "AcDbMlineStyle"},
        {  2, "OLD_BODY_NAME"}, { 70, "    2"}, {  3, "source description caf\u00e9^ A\\U+005CU+1234"}, { 62, "  256"},
        { 51, "90"}, { 52, "90"}, { 71, "    2"}, { 49, "0.5"},
        { 62, "    1"}, {  6, "ByLayer"}, { 49, "-0.25"}, { 62, "    3"},
        {  6, "Continuous"}, {  0, "MLINESTYLE"}, {  5, "D2"}, {102, "{ACAD_REACTORS"},
        {330, "A"}, {102, "}"}, {330, "A"}, {100, "AcDbMlineStyle"},
        {  2, "RETARGET_BODY"}, { 70, "    0"}, {  3, "retarget description"}, { 62, "    6"},
        { 51, "90"}, { 52, "90"}, { 71, "    2"}, { 49, "0.5"},
        { 62, "    5"}, {  6, "ByLayer"}, { 49, "-0.25"}, { 62, "    7"},
        {  6, "Continuous"}, {  0, "ENDSEC"}, {  0, "EOF"},
    };
    std::ostringstream out;for(const auto& tag:tags)out<<tag.first<<'\n'<<tag.second<<'\n';return out.str();
}
static bool writeFile(const std::filesystem::path& path,const std::string& bytes){
    std::ofstream out(path,std::ios::binary);out.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));return static_cast<bool>(out);
}
static std::string readFile(const std::filesystem::path& path){
    std::ifstream in(path,std::ios::binary);return {(std::istreambuf_iterator<char>(in)),{}};
}
static bool encodeBinary(const std::string& text,std::string& result){
    std::ostringstream bytes(std::ios::binary);bytes.write("AutoCAD Binary DXF\r\n\x1a\0",22);
    const bool r12=text.find("AC1009")!=std::string::npos;
    std::unique_ptr<dxfWriter> writer=r12?std::unique_ptr<dxfWriter>(new dxfWriterBinaryR12(&bytes))
        :std::unique_ptr<dxfWriter>(new dxfWriterBinary(&bytes));
    std::istringstream input(text);std::string code,value;
    while(std::getline(input,code)&&std::getline(input,value)) {
        const auto n=std::stoi(code);bool ok=false;
        // The production binary writer deliberately rejects 999. This local
        // physical decoder probe emits the two-byte code directly; it does not
        // establish native validity of binary 999 comments.
        if(n==999&&!r12){const char marker[]={static_cast<char>(0xe7),static_cast<char>(0x03)};
            bytes.write(marker,2);bytes.write(value.c_str(),static_cast<std::streamsize>(value.size()+1u));continue;}
        switch(dxfValueKindForCode(n)) {
        case DxfValueKind::Str:ok=writer->writeString(n,value);break;
        case DxfValueKind::Dbl:ok=writer->writeDouble(n,std::stod(value));break;
        case DxfValueKind::I16:ok=writer->writeInt16(n,std::stoi(value));break;
        case DxfValueKind::I32:ok=writer->writeInt32(n,std::stoi(value));break;
        case DxfValueKind::I64:ok=writer->writeInt64(n,std::stoll(value));break;
        case DxfValueKind::Bln:ok=writer->writeBool(n,std::stoi(value)!=0);break;
        default:return false;
        }
        if(!ok)return false;
    }
    result=bytes.str();return static_cast<bool>(bytes);
}
using dx_mline_source_dependency::Result;
using Tags=std::vector<std::pair<int,std::string>>;
static Tags parseTags(const std::string& text){Tags out;std::istringstream in(text);std::string code,value;while(std::getline(in,code)&&std::getline(in,value))out.emplace_back(std::stoi(code),value);return out;}
static std::string serialize(const Tags& tags){std::ostringstream out;for(const auto& x:tags)out<<x.first<<'\n'<<x.second<<'\n';return out.str();}
static std::size_t record(const Tags& a,std::string_view name,std::string_view id){
    for(std::size_t i=0;i<a.size();++i)if(a[i].first==0&&a[i].second==name){auto j=i+1;for(;j<a.size()&&a[j].first!=0;++j)if(a[j].first==5&&a[j].second==id)return i;}
    throw std::runtime_error("missing record");
}
static std::size_t field(const Tags& a,std::size_t record,int code,unsigned ordinal=0){
    for(std::size_t i=record+1;i<a.size()&&a[i].first!=0;++i)if(a[i].first==code){if(!ordinal)return i;--ordinal;}
    throw std::runtime_error("missing field");
}
static std::size_t afterRecord(const Tags& a,std::size_t rec){auto i=rec+1u;while(i<a.size()&&a[i].first!=0)++i;return i;}
static std::string selectedMlineSourceText(){
    auto tags=parseTags(sourceText());
    for(const char* id:{"D100","D101"})tags[field(tags,record(tags,"MLINE",id),2)].second="EFFECTIVE_STYLE";
    tags[field(tags,record(tags,"MLINESTYLE","D"),2)].second="EFFECTIVE_STYLE";
    tags[field(tags,record(tags,"MLINESTYLE","D2"),2)].second="RETARGET_STYLE";
    return serialize(tags);
}
static void replaceAllHandles(Tags& a) {
    for(auto& [code,text]:a)if(code==5||code==330||code==340||code==350) {
        std::uint32_t value=0;if(!dx_mline_source_dependency::handle(text,value,true))throw std::runtime_error("handle");if(!value)continue;
        std::ostringstream out;out<<std::uppercase<<std::hex<<(value+0x10000u);text=out.str();
    }
}
static void renameSymbols(Tags& a) {
    for(auto& [code,text]:a){if(code==8&&text=="0")text="DrawingLayer";
        else if(code==2&&text=="0")text="DrawingLayer";
        else if((code==2||code==6)&&text=="Continuous")text="SolidPattern";
        else if((code==2||code==6)&&text=="ByLayer")text="LayerPattern";
        else if((code==2||code==3)&&text=="AuthorityBlock")text="DetailBlock";
        else if((code==2||code==3)&&text=="EFFECTIVE_STYLE")text="NewStyle";
        else if((code==2||code==3)&&text=="RETARGET_STYLE")text="SecondStyle";}
}
static void insertLine(Tags& a) {
    const auto where=afterRecord(a,record(a,"MLINE","D100"));
    const Tags line={{0,"LINE"},{5,"D200"},{330,"201"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbLine"},{10,"0"},{20,"0"},{30,"0"},{11,"4"},{21,"3"},{31,"2"}};
    a.insert(a.begin()+static_cast<std::ptrdiff_t>(where),line.begin(),line.end());
}
struct Scenario {std::string name;Tags tags;bool admit;};
// Selected original-to-live LAYER relation, not writer authorization.
class LayerSpoofConsumer final : public Consumer {
public:
  bool spoof = false;
  void addDxfNamespaceSource(const DRW_TableEntry &model,
                             const DRW_RawDxfObject &raw) override {
    if (spoof && model.handle == 0x1E1u) {
      auto &layer =
          const_cast<DRW_Layer &>(static_cast<const DRW_Layer &>(model));
      layer.color = 3;
      auto &mutableRaw = const_cast<DRW_RawDxfObject &>(raw);
      for (std::size_t i = 0u; i < mutableRaw.groups.size(); ++i)
        if (mutableRaw.groups[i].code() == 62) {
          mutableRaw.groups[i] = DRW_Variant(62, 3);
          if (mutableRaw.hasRawValues)
            mutableRaw.rawValues[i] = "3";
        }
    }
    dx_iface::addDxfNamespaceSource(model, raw);
  }
};
static int testMlineLayerLive() {
  std::error_code ec;
  const auto dir = libdxfrw_test::reserveTestScratch(
      std::filesystem::temp_directory_path(), "mline-layer-live", ec);
  if (ec || dir.empty())
    return 1;
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {
      std::error_code ignored;
      std::filesystem::remove_all(path, ignored);
    }
  } cleanup{dir};
  auto original = parseTags(sourceText());
  for (const char *id : {"D100", "D101"})
    original[field(original, record(original, "MLINE", id), 2)].second =
        "EFFECTIVE_STYLE";
  original[field(original, record(original, "MLINESTYLE", "D"), 2)].second =
      "EFFECTIVE_STYLE";
  original[field(original, record(original, "MLINESTYLE", "D2"), 2)].second =
      "RETARGET_STYLE";
  const auto table = record(original, "TABLE", "1E0");
  original[field(original, table, 70)].second = "2";
  const auto begin = record(original, "LAYER", "1E1"),
             end = afterRecord(original, begin);
  Tags extra(original.begin() + static_cast<std::ptrdiff_t>(begin),
             original.begin() + static_cast<std::ptrdiff_t>(end));
  extra[field(extra, 0u, 5)].second = "1E2";
  extra[field(extra, 0u, 2)].second = "SECOND";
  original.insert(original.begin() + static_cast<std::ptrdiff_t>(end),
                  extra.begin(), extra.end());
  unsigned profiles = 0u, mutations = 0u, bodyTamper = 0u, retries = 0u,
           limits = 0u;
  for (bool binary : {false, true}) {
    std::string bytes = serialize(original);
    if (binary) {
      std::string encoded;
      if (!encodeBinary(bytes, encoded))
        return 2;
      bytes = std::move(encoded);
    }
    const auto path = dir / (binary ? "two-binary.dxf" : "two-ascii.dxf");
    if (!writeFile(path, bytes))
      return 3;
    for (bool spoof : {false, true}) {
      LayerSpoofConsumer adapter;
      adapter.spoof = spoof;
      dx_data data;
      if (!adapter.enableWholeDxfSourceObservation() ||
          !adapter.enableLegacyDxfIdentityObservation() ||
          !adapter.import(path.string(), data))
        return 4;
      const auto source =
          DxIfaceMlineDependencyTestAccess::qualify(adapter, data);
      auto &capture = DxWholeSourceTestAccess::capture(data);
      const auto it = capture.resourceOriginAudits.find(0x1E1u);
      if (source.result != Result::SourceCandidate ||
          it == capture.resourceOriginAudits.end() || !it->second ||
          !it->second->input || !it->second->input->raw ||
          !it->second->input->sourceModel || !capture.wholeInput ||
          !capture.identityInput || !capture.identityInput->source)
        return 5;
      const auto &audit = *it->second;
      const auto &whole = *capture.wholeInput->source;
      if (audit.physical >= capture.identityInput->source->records.size() ||
          !audit.originBound || !audit.bodyMatched || !audit.stateComplete)
        return 6;
      const auto &record =
          capture.identityInput->source->records[audit.physical];
      const auto body =
          mline_layer_relation::compare(whole, record, *audit.input->raw);
      if ((body.result == mline_layer_relation::Result::Match) == spoof)
        return 7;
      std::size_t charged = 0u;
      const auto check = [&](std::size_t work, std::size_t *used = nullptr) {
        return DxIfaceMlineDependencyTestAccess::layers(adapter, data, work,
                                                        used);
      };
      if (check(512u * 1024u * 1024u, &charged) == spoof)
        return 8;
      if (spoof) {
        adapter.spoof = false;
        dx_data fresh;
        if (!adapter.import(path.string(), fresh) ||
            !DxIfaceMlineDependencyTestAccess::layers(
                adapter, fresh, 512u * 1024u * 1024u, nullptr))
          return 9;
        ++retries;
        continue;
      }
      std::size_t refused = 123u;
      if (!charged || check(charged - 1u, &refused) || refused ||
          !check(charged))
        return 10;
      ++limits;
      mline_layer_relation::Limits limit;
      limit.work = body.charged - 1u;
      if (mline_layer_relation::compare(whole, record, *audit.input->raw, limit)
              .result != mline_layer_relation::Result::Limit)
        return 11;
      limit.work = body.charged;
      if (mline_layer_relation::compare(whole, record, *audit.input->raw, limit)
              .result != mline_layer_relation::Result::Match)
        return 12;
      ++limits;
      for (std::size_t i = 0u; i < audit.input->raw->groups.size(); ++i) {
        auto altered = *audit.input->raw;
        const auto &v = altered.groups[i];
        const int code = v.code();
        if (v.type() == DRW_Variant::STRING && v.content.s)
          altered.groups[i] = DRW_Variant(code, *v.content.s + "X");
        else if (v.type() == DRW_Variant::INTEGER)
          altered.groups[i] = DRW_Variant(code, v.i_val() + 1);
        else if (v.type() == DRW_Variant::INTEGER64)
          altered.groups[i] = DRW_Variant(code, v.i64_val() + 1);
        else if (v.type() == DRW_Variant::DOUBLE)
          altered.groups[i] = DRW_Variant(code, v.d_val() + 1.0);
        else
          return 13;
        if (mline_layer_relation::compare(whole, record, altered).result !=
            mline_layer_relation::Result::Refused)
          return 14;
        ++bodyTamper;
        if (altered.hasRawValues) {
          altered = *audit.input->raw;
          altered.rawValues[i] += " ";
          if (mline_layer_relation::compare(whole, record, altered).result !=
              mline_layer_relation::Result::Refused)
            return 15;
          ++bodyTamper;
        }
      }
      auto &first = data.layers.front();
      const auto savedFirst = first;
      const auto change = [&](const auto &modify) {
        modify(first);
        const bool rejected = !check(512u * 1024u * 1024u);
        first = savedFirst;
        if (!rejected || !check(charged))
          return false;
        ++mutations;
        ++retries;
        return true;
      };
      if (!change([](auto &v) { v.name = "OTHER"; }) ||
          !change([](auto &v) { v.handle = 0x1E2u; }) ||
          !change([](auto &v) { v.parentHandle = 0x1E2u; }) ||
          !change([](auto &v) { v.flags ^= 1; }) ||
          !change([](auto &v) { v.lineType = "ByLayer"; }) ||
          !change([](auto &v) { v.color = 3; }) ||
          !change([](auto &v) { v.color24 = 0; }) ||
          !change([](auto &v) { v.colorName = "BOOK"; }) ||
          !change([](auto &v) { v.plotF = !v.plotF; }) ||
          !change([](auto &v) { v.handleMaterialS = "D"; }) ||
          !change([](auto &v) { v.handlePlotS = "D"; }) ||
          !change([](auto &v) { v.xDictHandle = 0xDu; }) ||
          !change([](auto &v) { v.reactorHandles.push_back(0xDu); }) ||
          !change([](auto &v) {
            v.appData.push_back({{102, std::string("{TEST_HIDDEN")},
                                 {340, std::string("D")},
                                 {102, std::string("}")}});
          }))
        return 16;
      auto second = std::next(data.layers.begin());
      const auto savedSecond = *second;
      second->handle = first.handle;
      if (check(512u * 1024u * 1024u))
        return 17;
      *second = savedSecond;
      if (!check(charged))
        return 18;
      ++mutations;
      ++retries;
      data.layers.erase(second);
      if (check(512u * 1024u * 1024u))
        return 19;
      data.layers.push_back(savedSecond);
      if (!check(charged))
        return 20;
      ++mutations;
      ++retries;
      auto added = savedSecond;
      added.handle = 0x1E3u;
      added.name = "THIRD";
      data.layers.push_back(added);
      if (check(512u * 1024u * 1024u))
        return 21;
      data.layers.pop_back();
      if (!check(charged))
        return 22;
      ++mutations;
      ++retries;
      ++profiles;
    }
    Consumer ordinary;
    dx_data plain;
    if (!ordinary.import(path.string(), plain) ||
        DxIfaceMlineDependencyTestAccess::layers(ordinary, plain,
                                                 512u * 1024u * 1024u, nullptr))
      return 23;
    const auto output =
        dir / (binary ? "default-binary.dxf" : "default-ascii.dxf");
    if (!ordinary.fileExport(output.string(), DRW::AC1027, binary, &plain,
                             false, true))
      return 24;
  }
  std::cout << "layer source/live profiles=" << profiles
            << " mutations=" << mutations << " body-tamper=" << bodyTamper
            << " limits=" << limits << " retries=" << retries << '\n';
  return profiles == 2u && mutations == 34u && limits == 4u && retries == 36u
             ? 0
             : 25;
}

// Selected source/live LTYPE relation, not writer authorization.
class LtypeSpoofConsumer final : public Consumer {
public:
  bool spoof = false;
  void addDxfSymbolSource(const DRW_TableEntry &model,
                          const DRW_RawDxfObject &raw) override {
    if (spoof && model.handle == 0x102u) {
      auto &type =
          const_cast<DRW_LType &>(static_cast<const DRW_LType &>(model));
      type.desc = "SPOOF";
      auto &source = const_cast<DRW_RawDxfObject &>(raw);
      for (std::size_t i = 0u; i < source.groups.size(); ++i)
        if (source.groups[i].code() == 3) {
          source.groups[i] = DRW_Variant(3, std::string("SPOOF"));
          if (source.hasRawValues)
            source.rawValues[i] = "SPOOF";
        }
    }
    dx_iface::addDxfSymbolSource(model, raw);
  }
};
static int testMlineLtypeLive() {
  std::error_code ec;
  const auto dir = libdxfrw_test::reserveTestScratch(
      std::filesystem::temp_directory_path(), "mline-ltype-live", ec);
  if (ec || dir.empty())
    return 1;
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {
      std::error_code ignored;
      std::filesystem::remove_all(path, ignored);
    }
  } cleanup{dir};
  auto original = parseTags(sourceText());
  for (const char *id : {"D100", "D101"})
    original[field(original, record(original, "MLINE", id), 2)].second =
        "EFFECTIVE_STYLE";
  original[field(original, record(original, "MLINESTYLE", "D"), 2)].second =
      "EFFECTIVE_STYLE";
  original[field(original, record(original, "MLINESTYLE", "D2"), 2)].second =
      "RETARGET_STYLE";
  unsigned profiles = 0u, mutations = 0u, bodyTamper = 0u, retries = 0u,
           limits = 0u;
  for (bool binary : {false, true}) {
    std::string bytes = serialize(original);
    if (binary) {
      std::string encoded;
      if (!encodeBinary(bytes, encoded))
        return 2;
      bytes = std::move(encoded);
    }
    const auto path = dir / (binary ? "source-binary.dxf" : "source-ascii.dxf");
    if (!writeFile(path, bytes))
      return 3;
    for (bool spoof : {false, true}) {
      LtypeSpoofConsumer adapter;
      adapter.spoof = spoof;
      dx_data data;
      if (!adapter.enableWholeDxfSourceObservation() ||
          !adapter.enableLegacyDxfIdentityObservation() ||
          !adapter.import(path.string(), data))
        return 4;
      const auto candidate =
          DxIfaceMlineDependencyTestAccess::qualify(adapter, data);
      auto &capture = DxWholeSourceTestAccess::capture(data);
      const auto it = capture.resourceOriginAudits.find(0x102u);
      if (candidate.result != Result::SourceCandidate ||
          it == capture.resourceOriginAudits.end() || !it->second ||
          !it->second->input || !it->second->input->raw ||
          !it->second->input->sourceModel || !capture.wholeInput ||
          !capture.identityInput || !capture.identityInput->source)
        return 5;
      const auto &audit = *it->second;
      const auto &whole = *capture.wholeInput->source;
      if (audit.physical >= capture.identityInput->source->records.size() ||
          !audit.originBound || !audit.bodyMatched || !audit.stateComplete)
        return 6;
      const auto &physical =
          capture.identityInput->source->records[audit.physical];
      const auto body =
          mline_ltype_relation::compare(whole, physical, *audit.input->raw);
      if ((body.result == mline_ltype_relation::Result::Match) == spoof)
        return 7;
      std::size_t charged = 0u;
      const auto check = [&](std::size_t work, std::size_t *used = nullptr) {
        return DxIfaceMlineDependencyTestAccess::ltypes(adapter, data, work,
                                                        used);
      };
      if (check(512u * 1024u * 1024u, &charged) == spoof)
        return 8;
      if (spoof) {
        adapter.spoof = false;
        dx_data fresh;
        if (!adapter.import(path.string(), fresh) ||
            !DxIfaceMlineDependencyTestAccess::ltypes(
                adapter, fresh, 512u * 1024u * 1024u, nullptr))
          return 9;
        ++retries;
        continue;
      }
      std::size_t refused = 123u;
      if (!charged || check(charged - 1u, &refused) || refused ||
          !check(charged))
        return 10;
      ++limits;
      mline_ltype_relation::Limits limit;
      limit.work = body.charged - 1u;
      if (mline_ltype_relation::compare(whole, physical, *audit.input->raw,
                                        limit)
              .result != mline_ltype_relation::Result::Limit)
        return 11;
      limit.work = body.charged;
      if (mline_ltype_relation::compare(whole, physical, *audit.input->raw,
                                        limit)
              .result != mline_ltype_relation::Result::Match)
        return 12;
      ++limits;
      for (std::size_t i = 0u; i < audit.input->raw->groups.size(); ++i) {
        auto altered = *audit.input->raw;
        const auto &v = altered.groups[i];
        const int code = v.code();
        if (v.type() == DRW_Variant::STRING && v.content.s)
          altered.groups[i] = DRW_Variant(code, *v.content.s + "X");
        else if (v.type() == DRW_Variant::INTEGER)
          altered.groups[i] = DRW_Variant(code, v.i_val() + 1);
        else if (v.type() == DRW_Variant::INTEGER64)
          altered.groups[i] = DRW_Variant(code, v.i64_val() + 1);
        else if (v.type() == DRW_Variant::DOUBLE)
          altered.groups[i] = DRW_Variant(code, v.d_val() + 1.0);
        else
          return 13;
        if (mline_ltype_relation::compare(whole, physical, altered).result !=
            mline_ltype_relation::Result::Refused)
          return 14;
        ++bodyTamper;
        if (altered.hasRawValues) {
          altered = *audit.input->raw;
          altered.rawValues[i] += " ";
          if (mline_ltype_relation::compare(whole, physical, altered).result !=
              mline_ltype_relation::Result::Refused)
            return 15;
          ++bodyTamper;
        }
      }
      auto first = data.lineTypes.begin();
      if (first == data.lineTypes.end())
        return 16;
      auto second = std::next(first);
      if (second == data.lineTypes.end())
        return 16;
      const auto saved = *second;
      const auto change = [&](const auto &modify) {
        modify(*second);
        const bool rejected = !check(512u * 1024u * 1024u);
        *second = saved;
        if (!rejected || !check(charged))
          return false;
        ++mutations;
        ++retries;
        return true;
      };
      if (!change([](auto &v) { v.name = "OTHER"; }) ||
          !change([](auto &v) { v.handle = 0x101u; }) ||
          !change([](auto &v) { v.parentHandle = 0x101u; }) ||
          !change([](auto &v) { v.flags ^= 1; }) ||
          !change([](auto &v) { v.desc = "OTHER"; }) ||
          !change([](auto &v) { v.size = 1; }) ||
          !change([](auto &v) { v.length = 2.; }) ||
          !change([](auto &v) { v.alignment = 66; }) ||
          !change([](auto &v) { v.path.push_back(1.); }) ||
          !change([](auto &v) { v.segments.emplace_back(); }) ||
          !change([](auto &v) { v.shapeHandles.push_back(1); }) ||
          !change([](auto &v) { v.xDictHandle = 0xDu; }) ||
          !change([](auto &v) { v.reactorHandles.push_back(0xDu); }) ||
          !change([](auto &v) {
            v.appData.push_back({{102, std::string("{TEST_HIDDEN")},
                                 {340, std::string("D")},
                                 {102, std::string("}")}});
          }))
        return 17;
      auto reset = saved;
      reset.updateValues(saved.name, saved.desc, saved.size, saved.length,
                         saved.path);
      reset.handle = saved.handle;
      reset.parentHandle = saved.parentHandle;
      reset.flags = saved.flags;
      if (reset.name != saved.name || reset.desc != saved.desc ||
          reset.size != saved.size || reset.length != saved.length ||
          reset.alignment != saved.alignment ||
          saved.sameSelectedDxfEmptyPatternState(reset))
        return 18;
      *second = reset;
      if (check(512u * 1024u * 1024u))
        return 19;
      *second = saved;
      if (!check(charged))
        return 20;
      ++mutations;
      ++retries;
      second->handle = first->handle;
      if (check(512u * 1024u * 1024u))
        return 21;
      *second = saved;
      if (!check(charged))
        return 22;
      ++mutations;
      ++retries;
      data.lineTypes.erase(second);
      if (check(512u * 1024u * 1024u))
        return 23;
      data.lineTypes.push_back(saved);
      if (!check(charged))
        return 24;
      ++mutations;
      ++retries;
      auto extra = saved;
      extra.handle = 0x103u;
      extra.name = "THIRD";
      data.lineTypes.push_back(extra);
      if (check(512u * 1024u * 1024u))
        return 25;
      data.lineTypes.pop_back();
      if (!check(charged))
        return 26;
      ++mutations;
      ++retries;
      ++profiles;
    }
    Consumer ordinary;
    dx_data plain;
    if (!ordinary.import(path.string(), plain) ||
        DxIfaceMlineDependencyTestAccess::ltypes(ordinary, plain,
                                                 512u * 1024u * 1024u, nullptr))
      return 27;
    const auto output =
        dir / (binary ? "default-binary.dxf" : "default-ascii.dxf");
    if (!ordinary.fileExport(output.string(), DRW::AC1027, binary, &plain,
                             false, true))
      return 28;
  }
  std::cout << "ltype source/live profiles=" << profiles
            << " mutations=" << mutations << " body-tamper=" << bodyTamper
            << " limits=" << limits << " retries=" << retries << '\n';
  return profiles == 2u && mutations == 36u && limits == 4u && retries == 38u
             ? 0
             : 29;
}

static int testMlineRootDictionaryLive() {
  std::error_code ec;
  const auto dir = libdxfrw_test::reserveTestScratch(
      std::filesystem::temp_directory_path(), "mline-root-dictionary-live", ec);
  if (ec || dir.empty())
    return 1;
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {
      std::error_code ignored;
      std::filesystem::remove_all(path, ignored);
    }
  } cleanup{dir};
  auto tags = parseTags(sourceText());
  for (const char *id : {"D100", "D101"})
    tags[field(tags, record(tags, "MLINE", id), 2)].second = "EFFECTIVE_STYLE";
  tags[field(tags, record(tags, "MLINESTYLE", "D"), 2)].second =
      "EFFECTIVE_STYLE";
  tags[field(tags, record(tags, "MLINESTYLE", "D2"), 2)].second =
      "RETARGET_STYLE";
  const auto text = serialize(tags);
  unsigned profiles = 0u, mutations = 0u, bodyTamper = 0u,
           hiddenState = 0u, retries = 0u, limits = 0u;
  for (bool binary : {false, true}) {
    std::string bytes = text;
    if (binary) {
      std::string encoded;
      if (!encodeBinary(text, encoded))
        return 2;
      bytes = std::move(encoded);
    }
    const auto path = dir / (binary ? "source-binary.dxf" : "source-ascii.dxf");
    if (!writeFile(path, bytes))
      return 3;
    Consumer adapter;
    dx_data data;
    if (!adapter.enableWholeDxfSourceObservation() ||
        !adapter.enableLegacyDxfIdentityObservation() ||
        !adapter.import(path.string(), data) ||
        DxIfaceMlineDependencyTestAccess::qualify(adapter, data).result !=
            Result::SourceCandidate)
      return 4;
    std::size_t charged = 0u;
    const auto check = [&](std::size_t work, std::size_t *used = nullptr) {
      return DxIfaceMlineDependencyTestAccess::rootDictionary(
          adapter, data, work, used);
    };
    if (!check(512u * 1024u * 1024u, &charged) || !charged)
      return 5;
    std::size_t refused = 123u;
    if (check(charged - 1u, &refused) || refused || !check(charged))
      return 6;
    ++limits;
    auto &live = data.namedObjectDictionary;
    const auto saved = live;
    const auto change = [&](const auto &modify) {
      modify(live);
      if (DxIfaceMlineDependencyTestAccess::qualify(adapter, data).result !=
              Result::SourceCandidate ||
          check(512u * 1024u * 1024u))
        return false;
      live = saved;
      if (!check(charged))
        return false;
      ++mutations;
      ++retries;
      return true;
    };
    using EntryCode = DRW_Dictionary::Entry::DxfHandleCode;
    if (!change([](auto &v) { v.cloning ^= 1; }) ||
        !change([](auto &v) { v.hardOwner ^= 1; }) ||
        !change([](auto &v) { v.m_entries.front().m_name = "OTHER_ROOT"; }) ||
        !change([](auto &v) { v.m_entries.front().m_handle = 0xBu; }) ||
        !change([](auto &v) { v.m_entries.front().m_dxfHandleCode = EntryCode::HardOwner; }) ||
        !change([](auto &v) { v.m_entries.clear(); }) ||
        !change([](auto &v) { v.name = "OTHER_DICTIONARY"; }) ||
        !change([](auto &v) { v.appData.push_back({{102, std::string("{NEW")}, {102, std::string("}")}}); }))
      return 7;

    auto &capture = DxWholeSourceTestAccess::capture(data);
    const auto audit = capture.structuralAudits.find(0xCu);
    if (audit == capture.structuralAudits.end() || !audit->second ||
        !audit->second->input || !audit->second->input->raw ||
        !audit->second->input->model)
      return 8;
    const auto &whole = *capture.wholeInput->source;
    const auto &identity = *capture.identityInput->source;
    if (audit->second->physical >= identity.records.size())
      return 9;
    auto raw = *audit->second->input->raw;
    bool changedRaw = false;
    for (std::size_t i = 0u; i < raw.groups.size(); ++i) {
      if (raw.groups[i].code() == 280 &&
          raw.groups[i].type() == DRW_Variant::INTEGER) {
        raw.groups[i] = DRW_Variant(280, 1);
        if (raw.hasRawValues)
          raw.rawValues[i] = "1";
        changedRaw = true;
        break;
      }
    }
    if (!changedRaw ||
        DxIfaceMlineDependencyTestAccess::qualify(adapter, data).result !=
            Result::SourceCandidate ||
        dx_mline_dictionary_source_live::equal(
            whole, identity.records[audit->second->physical],
            audit->second->input->physical,
            audit->second->input->physicalStatus, raw) ||
        !check(charged))
      return 10;
    ++bodyTamper;

    const auto &original = static_cast<const DRW_Dictionary &>(
        *audit->second->input->model);
    auto resetOriginal = original;
    resetOriginal.reset();
    resetOriginal.handle = original.handle;
    resetOriginal.parentHandle = original.parentHandle;
    resetOriginal.name = original.name;
    resetOriginal.cloning = original.cloning;
    resetOriginal.hardOwner = original.hardOwner;
    resetOriginal.m_entries = original.m_entries;
    if (original.sameSelectedDxfEntryState(resetOriginal) || !check(charged))
      return 11;
    ++hiddenState;
    ++profiles;

    Consumer plain;
    dx_data unobserved;
    if (!plain.import(path.string(), unobserved) ||
        DxIfaceMlineDependencyTestAccess::rootDictionary(
            plain, unobserved, 512u * 1024u * 1024u, nullptr))
      return 13;
  }
  std::cout << "root dictionary source/live profiles=" << profiles
            << " mutations=" << mutations << " body-tamper=" << bodyTamper
            << " hidden-state=" << hiddenState << " limits=" << limits
            << " retries=" << retries << '\n';
  return profiles == 2u && mutations == 16u && bodyTamper == 2u &&
                 hiddenState == 2u && limits == 2u && retries == 16u
             ? 0
             : 14;
}

static int testMlineStyleDictionaryLive() {
  std::error_code ec;
  const auto dir = libdxfrw_test::reserveTestScratch(
      std::filesystem::temp_directory_path(), "mline-style-dictionary-live", ec);
  if (ec || dir.empty())
    return 1;
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {
      std::error_code ignored;
      std::filesystem::remove_all(path, ignored);
    }
  } cleanup{dir};
  auto tags = parseTags(sourceText());
  for (const char *id : {"D100", "D101"})
    tags[field(tags, record(tags, "MLINE", id), 2)].second = "EFFECTIVE_STYLE";
  tags[field(tags, record(tags, "MLINESTYLE", "D"), 2)].second =
      "EFFECTIVE_STYLE";
  tags[field(tags, record(tags, "MLINESTYLE", "D2"), 2)].second =
      "RETARGET_STYLE";
  const auto text = serialize(tags);
  unsigned profiles = 0u, mutations = 0u, retries = 0u, rawTamper = 0u,
           hiddenState = 0u, limits = 0u, listRefusals = 0u;
  for (bool binary : {false, true}) {
    std::string bytes = text;
    if (binary) {
      std::string encoded;
      if (!encodeBinary(text, encoded))
        return 2;
      bytes = std::move(encoded);
    }
    const auto path = dir / (binary ? "source-binary.dxf" : "source-ascii.dxf");
    if (!writeFile(path, bytes))
      return 3;
    Consumer adapter;
    dx_data data;
    if (!adapter.enableWholeDxfSourceObservation() ||
        !adapter.enableLegacyDxfIdentityObservation() ||
        !adapter.import(path.string(), data) || data.dictionaries.size() != 1u ||
        DxIfaceMlineDependencyTestAccess::qualify(adapter, data).result !=
            Result::SourceCandidate)
      return 4;
    std::size_t charged = 0u;
    const auto check = [&](std::size_t work, std::size_t *used = nullptr) {
      return DxIfaceMlineDependencyTestAccess::styleDictionary(
          adapter, data, work, used);
    };
    if (!check(512u * 1024u * 1024u, &charged) || !charged ||
        !DxIfaceMlineDependencyTestAccess::rootDictionary(
            adapter, data, charged, nullptr))
      return 5;
    std::size_t refused = 123u;
    if (check(charged - 1u, &refused) || refused || !check(charged))
      return 6;
    ++limits;
    auto &live = data.dictionaries.front();
    const auto saved = live;
    const auto change = [&](const auto &modify) {
      modify(live);
      if (DxIfaceMlineDependencyTestAccess::qualify(adapter, data).result !=
              Result::SourceCandidate ||
          !DxIfaceMlineDependencyTestAccess::rootDictionary(
              adapter, data, charged, nullptr) ||
          check(512u * 1024u * 1024u))
        return false;
      live = saved;
      if (!check(charged))
        return false;
      ++mutations;
      ++retries;
      return true;
    };
    using EntryCode = DRW_Dictionary::Entry::DxfHandleCode;
    if (!change([](auto &v) { v.cloning ^= 1; }) ||
        !change([](auto &v) { v.hardOwner ^= 1; }) ||
        !change([](auto &v) { v.name = "OTHER_STYLE_DICTIONARY"; }) ||
        !change([](auto &v) { v.m_entries.front().m_name = "OTHER_STYLE"; }) ||
        !change([](auto &v) { v.m_entries.front().m_handle = 0xBu; }) ||
        !change([](auto &v) { v.m_entries.front().m_dxfHandleCode = EntryCode::HardOwner; }) ||
        !change([](auto &v) { v.m_entries.clear(); }) ||
        !change([](auto &v) { v.appData.push_back({{102, std::string("{NEW")}, {102, std::string("}")}}); }) ||
        !change([](auto &v) { v.xDictHandle = 0xBu; }) ||
        !change([](auto &v) { v.m_entries.push_back({"EXTRA_STYLE", 0xBu}); }))
      return 7;

    auto &capture = DxWholeSourceTestAccess::capture(data);
    const auto audit = capture.structuralAudits.find(0xAu);
    if (audit == capture.structuralAudits.end() || !audit->second ||
        !audit->second->input || !audit->second->input->raw ||
        !audit->second->input->model || !capture.identityInput ||
        !capture.identityInput->source ||
        audit->second->physical >= capture.identityInput->source->records.size())
      return 8;
    auto raw = *audit->second->input->raw;
    bool changedRaw = false;
    for (std::size_t i = 0u; i < raw.groups.size(); ++i) {
      if (raw.groups[i].code() == 350 &&
          raw.groups[i].type() == DRW_Variant::STRING &&
          raw.groups[i].content.s) {
        raw.groups[i] = DRW_Variant(350, "B");
        if (raw.hasRawValues)
          raw.rawValues[i] = "B";
        changedRaw = true;
        break;
      }
    }
    if (!changedRaw ||
        dx_mline_dictionary_source_live::equal(
            *capture.wholeInput->source,
            capture.identityInput->source->records[audit->second->physical],
            audit->second->input->physical,
            audit->second->input->physicalStatus, raw) ||
        !check(charged))
      return 9;
    ++rawTamper;

    const auto original = live;
    live.reset();
    live.handle = original.handle;
    live.parentHandle = original.parentHandle;
    live.name = original.name;
    live.cloning = original.cloning;
    live.hardOwner = original.hardOwner;
    live.m_entries = original.m_entries;
    if (DxIfaceMlineDependencyTestAccess::qualify(adapter, data).result !=
            Result::SourceCandidate ||
        !DxIfaceMlineDependencyTestAccess::rootDictionary(
            adapter, data, charged, nullptr) ||
        check(512u * 1024u * 1024u))
      return 10;
    live = original;
    if (!check(charged))
      return 11;
    ++hiddenState;

    const auto only = live;
    data.dictionaries.clear();
    if (!DxIfaceMlineDependencyTestAccess::rootDictionary(
            adapter, data, charged, nullptr) ||
        check(512u * 1024u * 1024u))
      return 12;
    data.dictionaries.push_back(only);
    if (!check(charged))
      return 13;
    ++listRefusals;

    Consumer plain;
    dx_data unobserved;
    if (!plain.import(path.string(), unobserved) ||
        DxIfaceMlineDependencyTestAccess::styleDictionary(
            plain, unobserved, 512u * 1024u * 1024u, nullptr))
      return 14;
    ++profiles;
  }
  std::cout << "style dictionary source/live profiles=" << profiles
            << " mutations=" << mutations << " raw-tamper=" << rawTamper
            << " hidden-state=" << hiddenState << " limits=" << limits
            << " list-refusals=" << listRefusals << " retries=" << retries << '\n';
  return profiles == 2u && mutations == 20u && rawTamper == 2u &&
                 hiddenState == 2u && limits == 2u && listRefusals == 2u &&
                 retries == 20u
             ? 0
             : 15;
}

static int testMlineStyleBodiesLive() {
  std::error_code ec;
  const auto dir = libdxfrw_test::reserveTestScratch(
      std::filesystem::temp_directory_path(), "mline-style-bodies-live", ec);
  if (ec || dir.empty())
    return 1;
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {
      std::error_code ignored;
      std::filesystem::remove_all(path, ignored);
    }
  } cleanup{dir};
  auto tags = parseTags(sourceText());
  for (const char *id : {"D100", "D101"})
    tags[field(tags, record(tags, "MLINE", id), 2)].second = "EFFECTIVE_STYLE";
  tags[field(tags, record(tags, "MLINESTYLE", "D"), 2)].second =
      "EFFECTIVE_STYLE";
  tags[field(tags, record(tags, "MLINESTYLE", "D2"), 2)].second =
      "RETARGET_STYLE";
  const auto text = serialize(tags);
  unsigned profiles = 0u, mutations = 0u, retries = 0u, rawTamper = 0u,
           hiddenState = 0u, limits = 0u, listRefusals = 0u,
           boundedRefusals = 0u;
  for (bool binary : {false, true}) {
    std::string bytes = text;
    if (binary) {
      std::string encoded;
      if (!encodeBinary(text, encoded))
        return 2;
      bytes = std::move(encoded);
    }
    const auto path = dir / (binary ? "source-binary.dxf" : "source-ascii.dxf");
    if (!writeFile(path, bytes))
      return 3;
    Consumer adapter;
    dx_data data;
    if (!adapter.enableWholeDxfSourceObservation() ||
        !adapter.enableLegacyDxfIdentityObservation() ||
        !adapter.import(path.string(), data) ||
        DxIfaceMlineDependencyTestAccess::qualify(adapter, data).result !=
            Result::SourceCandidate)
      return 4;
    std::size_t charged = 0u;
    const auto check = [&](std::size_t work, std::size_t *used = nullptr) {
      return DxIfaceMlineDependencyTestAccess::styleBodies(
          adapter, data, work, used);
    };
    if (!check(512u * 1024u * 1024u, &charged) || !charged ||
        !DxIfaceMlineDependencyTestAccess::styleDictionary(
            adapter, data, charged, nullptr))
      return 5;
    std::size_t refused = 123u;
    if (check(charged - 1u, &refused) || refused || !check(charged))
      return 6;
    ++limits;

    const auto change = [&](std::uint32_t handle, const auto &modify) {
      auto found = std::find_if(data.mLineStyles.begin(), data.mLineStyles.end(),
          [handle](const auto &value) { return value.handle == handle; });
      if (found == data.mLineStyles.end())
        return false;
      const auto saved = *found;
      modify(*found);
      if (DxIfaceMlineDependencyTestAccess::qualify(adapter, data).result !=
              Result::SourceCandidate ||
          !DxIfaceMlineDependencyTestAccess::styleDictionary(
              adapter, data, charged, nullptr) ||
          check(512u * 1024u * 1024u))
        return false;
      *found = saved;
      if (!check(charged))
        return false;
      ++mutations;
      ++retries;
      return true;
    };
    const auto mutateStyle = [&](std::uint32_t handle) {
      return change(handle, [](auto &v) { v.name += "_changed"; }) &&
          change(handle, [](auto &v) { v.description += " changed"; }) &&
          change(handle, [](auto &v) { v.flags ^= 1; }) &&
          change(handle, [](auto &v) { v.startAngle += 1.0; }) &&
          change(handle, [](auto &v) { v.endAngle += 1.0; }) &&
          change(handle, [](auto &v) { v.fillColor = 7; }) &&
          change(handle, [](auto &v) { v.elements.front().offset += 0.125; }) &&
          change(handle, [](auto &v) { v.elements.front().color = 9; }) &&
          change(handle, [](auto &v) { v.elements.front().linetype = "ByBlock"; }) &&
          change(handle, [](auto &v) { v.handle = 0xEu; }) &&
          change(handle, [](auto &v) { v.parentHandle = 0xBu; });
    };
    if (!mutateStyle(0xDu) || !mutateStyle(0xD2u))
      return 7;

    auto raw = std::find_if(data.rawNamedResourceObjects.begin(),
        data.rawNamedResourceObjects.end(), [](const auto &value) {
          return value.handle == 0xDu && value.name == "MLINESTYLE";
        });
    if (raw == data.rawNamedResourceObjects.end())
      return 8;
    const auto savedRaw = *raw;
    constexpr std::size_t oversizedBytes = 256u * 1024u + 1u;
    bool oversizedRaw = false;
    for (std::size_t i = 0u; i < raw->groups.size(); ++i) {
      if (raw->groups[i].code() == 3 &&
          raw->groups[i].type() == DRW_Variant::STRING &&
          raw->groups[i].content.s) {
        raw->groups[i] = DRW_Variant(3, std::string(oversizedBytes, 'X'));
        if (raw->hasRawValues)
          raw->rawValues[i].assign(oversizedBytes, 'X');
        oversizedRaw = true;
        break;
      }
    }
    if (!oversizedRaw || check(512u * 1024u * 1024u))
      return 18;
    *raw = savedRaw;
    if (!check(charged))
      return 19;
    ++boundedRefusals;

    bool changedRaw = false;
    for (std::size_t i = 0u; i < raw->groups.size(); ++i) {
      if (raw->groups[i].code() == 3 &&
          raw->groups[i].type() == DRW_Variant::STRING &&
          raw->groups[i].content.s) {
        raw->groups[i] = DRW_Variant(3, "changed source description");
        if (raw->hasRawValues)
          raw->rawValues[i] = "changed source description";
        changedRaw = true;
        break;
      }
    }
    if (!changedRaw || check(512u * 1024u * 1024u))
      return 9;
    *raw = savedRaw;
    if (!check(charged))
      return 10;
    ++rawTamper;

    auto first = std::find_if(data.mLineStyles.begin(), data.mLineStyles.end(),
        [](const auto &value) { return value.handle == 0xDu; });
    if (first == data.mLineStyles.end())
      return 11;
    const auto savedDescription = first->description;
    first->description.assign(oversizedBytes, 'X');
    if (check(512u * 1024u * 1024u))
      return 20;
    first->description = savedDescription;
    if (!check(charged))
      return 21;
    ++boundedRefusals;

    const auto savedStyle = *first;
    first->reset();
    static_cast<DRW_TableEntry &>(*first) =
        static_cast<const DRW_TableEntry &>(savedStyle);
    first->flags = savedStyle.flags;
    first->startAngle = savedStyle.startAngle;
    first->endAngle = savedStyle.endAngle;
    first->fillColor = savedStyle.fillColor;
    first->fillColor24 = savedStyle.fillColor24;
    first->fillColorMethod = savedStyle.fillColorMethod;
    first->fillColorFlags = savedStyle.fillColorFlags;
    first->fillColorName = savedStyle.fillColorName;
    first->fillColorBookName = savedStyle.fillColorBookName;
    first->fillColorDwgIndex = savedStyle.fillColorDwgIndex;
    first->name = savedStyle.name;
    first->description = savedStyle.description;
    first->elements = savedStyle.elements;
    if (DxIfaceMlineDependencyTestAccess::qualify(adapter, data).result !=
            Result::SourceCandidate ||
        !DxIfaceMlineDependencyTestAccess::styleDictionary(
            adapter, data, charged, nullptr) ||
        check(512u * 1024u * 1024u))
      return 12;
    *first = savedStyle;
    if (!check(charged))
      return 13;
    ++hiddenState;

    const auto savedStyles = data.mLineStyles;
    data.mLineStyles.pop_back();
    if (check(512u * 1024u * 1024u))
      return 14;
    data.mLineStyles = savedStyles;
    if (!check(charged))
      return 15;
    ++listRefusals;

    Consumer plain;
    dx_data unobserved;
    if (!plain.import(path.string(), unobserved) ||
        DxIfaceMlineDependencyTestAccess::styleBodies(
            plain, unobserved, 512u * 1024u * 1024u, nullptr))
      return 16;
    ++profiles;
  }
  std::cout << "MLINESTYLE body source/live profiles=" << profiles
            << " mutations=" << mutations << " raw-tamper=" << rawTamper
            << " hidden-state=" << hiddenState << " limits=" << limits
            << " list-refusals=" << listRefusals
            << " bounded-refusals=" << boundedRefusals
            << " retries=" << retries << '\n';
  return profiles == 2u && mutations == 44u && rawTamper == 2u &&
                 hiddenState == 2u && limits == 2u && listRefusals == 2u &&
                 boundedRefusals == 4u && retries == 44u
             ? 0
             : 17;
}

static int testDeletedMlineConsumerGeometryGuard() {
  std::error_code ec;
  const auto dir = libdxfrw_test::reserveTestScratch(
      std::filesystem::temp_directory_path(), "mline-deleted-consumer-guard", ec);
  if (ec || dir.empty())
    return 1;
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {
      std::error_code ignored;
      std::filesystem::remove_all(path, ignored);
    }
  } cleanup{dir};
  unsigned profiles = 0u, refusals = 0u, retries = 0u, readbacks = 0u;
  unsigned movedProfiles = 0u, movedRefusals = 0u, movedRetries = 0u, movedReadbacks = 0u;
  for (bool sourceBinary : {false, true}) {
    auto tags = parseTags(sourceText());
    for (const char *id : {"D100", "D101"}) {
      const auto row = record(tags, "MLINE", id);
      tags[field(tags, row, 2)].second = "RETARGET_STYLE";
      tags[field(tags, row, 340)].second = "D2";
    }
    tags[field(tags, record(tags, "MLINESTYLE", "D"), 2)].second =
        "EFFECTIVE_STYLE";
    tags[field(tags, record(tags, "MLINESTYLE", "D2"), 2)].second =
        "RETARGET_STYLE";
    const auto text = serialize(tags);
    std::string source = text;
    if (sourceBinary) {
      if (!encodeBinary(text, source))
        return 2;
    }
    const auto input = dir / (sourceBinary ? "source-binary.dxf" : "source-ascii.dxf");
    if (!writeFile(input, source))
      return 3;
    for (bool outputBinary : {false, true}) {
      const auto output = dir / (std::string(sourceBinary ? "binary-source-output-" : "ascii-source-output-") +
          (outputBinary ? "binary.dxf" : "ascii.dxf"));
      Consumer adapter;
      dx_data data;
      if (!adapter.enableWholeDxfSourceObservation() ||
          !adapter.enableLegacyDxfIdentityObservation() ||
          !adapter.import(input.string(), data))
        return 4;
      const auto sourceProof = DxIfaceMlineDependencyTestAccess::qualify(adapter, data);
      if (sourceProof.result != Result::SourceCandidate) {
        std::cerr << "MLINE removed-consumer source classification="
                  << static_cast<int>(sourceProof.result) << '\n';
        return 4;
      }
      auto style = std::find_if(data.mLineStyles.begin(), data.mLineStyles.end(),
          [](const auto &value) { return value.handle == 0xD2u; });
      if (style == data.mLineStyles.end() || style->elements.size() != 2u)
        return 5;
      const auto originalOffset = style->elements.front().offset;
      style->elements.front().offset = originalOffset + 0.25;

      std::vector<std::pair<dx_ifaceBlock *, std::list<DRW_Entity *>>> savedEntities;
      const auto removeConsumers = [&](dx_ifaceBlock *block) {
        if (!block)
          return;
        savedEntities.emplace_back(block, block->ent);
        block->ent.remove_if([](const DRW_Entity *entity) {
          return entity && entity->eType == DRW::MLINE;
        });
      };
      removeConsumers(data.mBlock);
      for (auto *block : data.blocks)
        removeConsumers(block);
      std::size_t removed = 0u;
      for (const auto &saved : savedEntities)
        for (const auto *entity : saved.second)
          if (entity && entity->eType == DRW::MLINE)
            ++removed;
      if (removed != 2u)
        return 6;

      const std::string sentinel = "MLINE_DELETED_CONSUMER_GEOMETRY_DESTINATION";
      if (!writeFile(output, sentinel))
        return 7;
      const bool exported = adapter.fileExport(output.string(), DRW::AC1027,
          outputBinary, &data, false, true);
      if (exported || readFile(output) != sentinel ||
          style->elements.front().offset != originalOffset + 0.25)
        return 8;
      ++profiles;
      ++refusals;

      for (auto &saved : savedEntities)
        saved.first->ent = saved.second;
      style->elements.front().offset = originalOffset;
      if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
              &data, false, true))
        return 9;
      Consumer verifier;
      dx_data readback;
      if (!verifier.import(output.string(), readback))
        return 10;
      std::size_t consumers = 0u;
      const auto countConsumers = [&](const dx_ifaceBlock *block) {
        if (block)
          for (const auto *entity : block->ent)
            if (entity && entity->eType == DRW::MLINE)
              ++consumers;
      };
      countConsumers(readback.mBlock);
      for (const auto *block : readback.blocks)
        countConsumers(block);
      const auto restoredStyle = std::find_if(readback.mLineStyles.begin(),
          readback.mLineStyles.end(), [](const auto &value) {
            return value.handle == 0xD2u;
          });
      if (consumers != 2u || restoredStyle == readback.mLineStyles.end() ||
          restoredStyle->elements.size() != 2u ||
          restoredStyle->elements.front().offset != originalOffset)
        return 11;
      ++retries;
      ++readbacks;

      const auto moved = std::find_if(data.mBlock->ent.begin(), data.mBlock->ent.end(),
          [](const DRW_Entity *entity) { return entity && entity->handle == 0xD100u; });
      const auto destinationBlock = std::find_if(data.blocks.begin(), data.blocks.end(),
          [](const dx_ifaceBlock *block) { return block && block->handle == 0x300u; });
      if (moved == data.mBlock->ent.end() || destinationBlock == data.blocks.end()) {
        std::cerr << "MLINE moved-consumer setup missing: modelSpace="
                  << static_cast<bool>(data.mBlock) << " moved="
                  << (moved != data.mBlock->ent.end()) << " destination="
                  << (destinationBlock != data.blocks.end()) << " blocks="
                  << data.blocks.size() << '\n';
        return 20;
      }
      auto *movedEntity = *moved;
      const auto originalOwner = movedEntity->parentHandle;
      data.mBlock->ent.erase(moved);
      (*destinationBlock)->ent.push_back(movedEntity);
      movedEntity->parentHandle = 0x203u;
      style->elements.front().offset = originalOffset + 0.25;
      const std::string movedSentinel = "MLINE_MOVED_CONSUMER_GEOMETRY_DESTINATION";
      if (!writeFile(output, movedSentinel))
        return 21;
      if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
              &data, false, true) || readFile(output) != movedSentinel ||
          style->elements.front().offset != originalOffset + 0.25)
        return 22;
      ++movedProfiles;
      ++movedRefusals;

      (*destinationBlock)->ent.remove(movedEntity);
      data.mBlock->ent.push_back(movedEntity);
      movedEntity->parentHandle = originalOwner;
      style->elements.front().offset = originalOffset;
      if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
              &data, false, true))
        return 23;
      Consumer movedVerifier;
      dx_data movedReadback;
      if (!movedVerifier.import(output.string(), movedReadback))
        return 24;
      std::size_t movedConsumers = 0u;
      const auto countMovedConsumers = [&](const dx_ifaceBlock *block) {
        if (block)
          for (const auto *entity : block->ent)
            if (entity && entity->eType == DRW::MLINE)
              ++movedConsumers;
      };
      countMovedConsumers(movedReadback.mBlock);
      for (const auto *block : movedReadback.blocks)
        countMovedConsumers(block);
      const auto movedStyle = std::find_if(movedReadback.mLineStyles.begin(),
          movedReadback.mLineStyles.end(), [](const auto &value) {
            return value.handle == 0xD2u;
          });
      if (movedConsumers != 2u || movedStyle == movedReadback.mLineStyles.end() ||
          movedStyle->elements.size() != 2u ||
          movedStyle->elements.front().offset != originalOffset)
        return 25;
      ++movedRetries;
      ++movedReadbacks;
    }
  }
  unsigned unusedProfiles = 0u;
  for (bool sourceBinary : {false, true}) {
    const auto text = selectedMlineSourceText();
    std::string source = text;
    if (sourceBinary && !encodeBinary(text, source))
      return 13;
    const auto input = dir / (sourceBinary ? "unused-source-binary.dxf" : "unused-source-ascii.dxf");
    if (!writeFile(input, source))
      return 14;
    for (bool outputBinary : {false, true}) {
      const auto output = dir / (std::string(sourceBinary ? "unused-binary-source-output-" : "unused-ascii-source-output-") +
          (outputBinary ? "binary.dxf" : "ascii.dxf"));
      Consumer adapter;
      dx_data data;
      if (!adapter.enableWholeDxfSourceObservation() ||
          !adapter.enableLegacyDxfIdentityObservation() ||
          !adapter.import(input.string(), data))
        return 15;
      auto style = std::find_if(data.mLineStyles.begin(), data.mLineStyles.end(),
          [](const auto &value) { return value.handle == 0xD2u; });
      if (style == data.mLineStyles.end() || style->elements.size() != 2u)
        return 16;
      const auto expectedOffset = style->elements.front().offset + 0.25;
      style->elements.front().offset = expectedOffset;
      std::size_t removed = 0u;
      const auto removeConsumers = [&](dx_ifaceBlock *block) {
        if (!block)
          return;
        block->ent.remove_if([&](const DRW_Entity *entity) {
          if (entity && entity->eType == DRW::MLINE) {
            ++removed;
            return true;
          }
          return false;
        });
      };
      removeConsumers(data.mBlock);
      for (auto *block : data.blocks)
        removeConsumers(block);
      if (removed != 2u || !adapter.fileExport(output.string(), DRW::AC1027,
              outputBinary, &data, false, true))
        return 17;
      Consumer verifier;
      dx_data readback;
      if (!verifier.import(output.string(), readback))
        return 18;
      std::size_t consumers = 0u;
      const auto countConsumers = [&](const dx_ifaceBlock *block) {
        if (block)
          for (const auto *entity : block->ent)
            if (entity && entity->eType == DRW::MLINE)
              ++consumers;
      };
      countConsumers(readback.mBlock);
      for (const auto *block : readback.blocks)
        countConsumers(block);
      const auto restoredStyle = std::find_if(readback.mLineStyles.begin(),
          readback.mLineStyles.end(), [](const auto &value) {
            return value.handle == 0xD2u;
          });
      if (consumers != 0u || restoredStyle == readback.mLineStyles.end() ||
          restoredStyle->elements.size() != 2u ||
          restoredStyle->elements.front().offset != expectedOffset)
        return 19;
      ++unusedProfiles;
    }
  }
  std::cout << "MLINE deleted-consumer geometry guard profiles=" << profiles
            << " refusals=" << refusals << " retries=" << retries
            << " readbacks=" << readbacks << " moved-profiles=" << movedProfiles
            << " moved-refusals=" << movedRefusals << " moved-retries=" << movedRetries
            << " moved-readbacks=" << movedReadbacks
            << " unused-style-controls=" << unusedProfiles << '\n';
  return profiles == 4u && refusals == 4u && retries == 4u && readbacks == 4u
      && movedProfiles == 4u && movedRefusals == 4u && movedRetries == 4u
      && movedReadbacks == 4u && unusedProfiles == 4u
      ? 0 : 12;
}

static int testMlineRawSectionGeometryGuard() {
  std::error_code ec;
  const auto dir = libdxfrw_test::reserveTestScratch(
      std::filesystem::temp_directory_path(), "mline-raw-section-geometry-guard", ec);
  if (ec || dir.empty())
    return 1;
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {
      std::error_code ignored;
      std::filesystem::remove_all(path, ignored);
    }
  } cleanup{dir};
  unsigned profiles = 0u, refusals = 0u, retries = 0u, readbacks = 0u;
  for (bool sourceBinary : {false, true}) {
    const auto text = selectedMlineSourceText();
    std::string source = text;
    if (sourceBinary && !encodeBinary(text, source))
      return 2;
    const auto input = dir / (sourceBinary ? "source-binary.dxf" : "source-ascii.dxf");
    if (!writeFile(input, source))
      return 3;
    for (bool outputBinary : {false, true}) {
      const auto output = dir / (std::string(sourceBinary ? "binary-source-output-" : "ascii-source-output-") +
          (outputBinary ? "binary.dxf" : "ascii.dxf"));
      Consumer adapter;
      dx_data data;
      if (!adapter.enableWholeDxfSourceObservation() ||
          !adapter.enableLegacyDxfIdentityObservation() ||
          !adapter.import(input.string(), data))
        return 4;
      auto style = std::find_if(data.mLineStyles.begin(), data.mLineStyles.end(),
          [](const auto &value) { return value.handle == 0xD2u; });
      if (style == data.mLineStyles.end() || style->elements.size() != 2u)
        return 5;
      const auto expectedOffset = style->elements.front().offset + 0.25;
      style->elements.front().offset = expectedOffset;
      DRW_RawDxfSection raw;
      raw.m_name = "CUSTOM_DEPENDENCY";
      raw.m_version = DRW::AC1027;
      raw.m_hasRawValues = true;
      raw.m_groups.emplace_back(0, "OPAQUE_RECORD");
      raw.m_groups.emplace_back(340, "D2");
      raw.m_rawValues = {"OPAQUE_RECORD", "D2"};
      data.rawDxfSections.push_back(raw);

      const std::string sentinel = "MLINE_RAW_SECTION_DESTINATION";
      if (!writeFile(output, sentinel))
        return 6;
      if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
              &data, false, true) || readFile(output) != sentinel ||
          style->elements.front().offset != expectedOffset ||
          data.rawDxfSections.size() != 1u ||
          data.rawDxfSections.front().m_groups.size() != 2u ||
          data.rawDxfSections.front().m_groups[1].c_str() == nullptr ||
          std::string(data.rawDxfSections.front().m_groups[1].c_str()) != "D2")
        return 7;
      ++profiles;
      ++refusals;

      data.rawDxfSections.clear();
      if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
              &data, false, true))
        return 8;
      Consumer verifier;
      dx_data readback;
      if (!verifier.import(output.string(), readback) || !readback.rawDxfSections.empty())
        return 9;
      const auto restoredStyle = std::find_if(readback.mLineStyles.begin(),
          readback.mLineStyles.end(), [](const auto &value) {
            return value.handle == 0xD2u;
          });
      if (restoredStyle == readback.mLineStyles.end() ||
          restoredStyle->elements.size() != 2u ||
          restoredStyle->elements.front().offset != expectedOffset)
        return 10;
      ++retries;
      ++readbacks;
    }
  }
  std::cout << "MLINE raw-section geometry guard profiles=" << profiles
            << " refusals=" << refusals << " retries=" << retries
            << " readbacks=" << readbacks << '\n';
  return profiles == 4u && refusals == 4u && retries == 4u && readbacks == 4u
      ? 0 : 11;
}

static int testMlineLayerCarrierGeometryGuard() {
  std::error_code ec;
  const auto dir = libdxfrw_test::reserveTestScratch(
      std::filesystem::temp_directory_path(), "mline-layer-carrier-geometry-guard", ec);
  if (ec || dir.empty())
    return 1;
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {
      std::error_code ignored;
      std::filesystem::remove_all(path, ignored);
    }
  } cleanup{dir};
  unsigned profiles = 0u, refusals = 0u, lateRefusals = 0u, retries = 0u, readbacks = 0u;
  for (bool sourceBinary : {false, true}) {
    const auto text = selectedMlineSourceText();
    std::string source = text;
    if (sourceBinary && !encodeBinary(text, source))
      return 2;
    const auto input = dir / (sourceBinary ? "source-binary.dxf" : "source-ascii.dxf");
    if (!writeFile(input, source))
      return 3;
    for (bool outputBinary : {false, true}) {
      const auto output = dir / (std::string(sourceBinary ? "binary-source-output-" : "ascii-source-output-") +
          (outputBinary ? "binary.dxf" : "ascii.dxf"));
      LayerMutationAtFinalizeConsumer adapter;
      dx_data data;
      if (!adapter.enableWholeDxfSourceObservation() ||
          !adapter.enableLegacyDxfIdentityObservation() ||
          !adapter.import(input.string(), data))
        return 4;
      adapter.mutationTarget = &data;
      auto style = std::find_if(data.mLineStyles.begin(), data.mLineStyles.end(),
          [](const auto &value) { return value.handle == 0xD2u; });
      auto layer = std::find_if(data.layers.begin(), data.layers.end(),
          [](const auto &value) { return value.handle == 0x1E1u; });
      if (style == data.mLineStyles.end() || style->elements.size() != 2u ||
          layer == data.layers.end() || !layer->appData.empty())
        return 5;
      const auto expectedOffset = style->elements.front().offset + 0.25;
      style->elements.front().offset = expectedOffset;
      layer->appData.push_back({{102, std::string("{TEST_HIDDEN")},
                                {340, std::string("D2")},
                                {102, std::string("}")}});

      const std::string sentinel = "MLINE_LAYER_CARRIER_DESTINATION";
      if (!writeFile(output, sentinel))
        return 6;
      if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
              &data, false, true) || readFile(output) != sentinel ||
          style->elements.front().offset != expectedOffset ||
          layer->appData.size() != 1u || layer->appData.front().size() != 3u ||
          layer->appData.front().back().c_str() == nullptr ||
          std::string(layer->appData.front().back().c_str()) != "}")
        return 7;
      ++profiles;
      ++refusals;

      layer->appData.clear();
      if (!writeFile(output, sentinel))
        return 11;
      adapter.injectAtFinalize = true;
      if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
              &data, false, true) || readFile(output) != sentinel ||
          !adapter.injected || !layer->appData.empty() ||
          style->elements.front().offset != expectedOffset)
        return 12;
      ++lateRefusals;
      adapter.injectAtFinalize = false;
      if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
              &data, false, true))
        return 8;
      Consumer verifier;
      dx_data readback;
      if (!verifier.import(output.string(), readback) ||
          std::any_of(readback.layers.begin(), readback.layers.end(),
              [](const auto &value) { return !value.appData.empty(); }))
        return 9;
      const auto restoredStyle = std::find_if(readback.mLineStyles.begin(),
          readback.mLineStyles.end(), [](const auto &value) {
            return value.handle == 0xD2u;
          });
      if (restoredStyle == readback.mLineStyles.end() ||
          restoredStyle->elements.size() != 2u ||
          restoredStyle->elements.front().offset != expectedOffset)
        return 10;
      ++retries;
      ++readbacks;
    }
  }
  std::cout << "MLINE LAYER-carrier geometry guard profiles=" << profiles
            << " refusals=" << refusals << " late-refusals=" << lateRefusals
            << " retries=" << retries
            << " readbacks=" << readbacks << '\n';
  return profiles == 4u && refusals == 4u && lateRefusals == 4u
      && retries == 4u && readbacks == 4u
      ? 0 : 11;
}

static int testMlineDictionaryCarrierGeometryGuard() {
  std::error_code ec;
  const auto dir = libdxfrw_test::reserveTestScratch(
      std::filesystem::temp_directory_path(), "mline-dictionary-carrier-geometry-guard", ec);
  if (ec || dir.empty())
    return 1;
  struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {
      std::error_code ignored;
      std::filesystem::remove_all(path, ignored);
    }
  } cleanup{dir};
  unsigned profiles = 0u, rootRefusals = 0u, childRefusals = 0u;
  unsigned lateRefusals = 0u, retries = 0u, readbacks = 0u;
  const auto sameEntry = [](const DRW_Dictionary::Entry& a,
                            const DRW_Dictionary::Entry& b) {
    return a.m_name == b.m_name && a.m_handle == b.m_handle &&
        a.m_dxfHandleCode == b.m_dxfHandleCode;
  };
  for (bool sourceBinary : {false, true}) {
    const auto text = selectedMlineSourceText();
    std::string source = text;
    if (sourceBinary && !encodeBinary(text, source))
      return 2;
    const auto input = dir / (sourceBinary ? "source-binary.dxf" : "source-ascii.dxf");
    if (!writeFile(input, source))
      return 3;
    for (bool outputBinary : {false, true}) {
      const auto output = dir / (std::string(sourceBinary ? "binary-source-output-" : "ascii-source-output-") +
          (outputBinary ? "binary.dxf" : "ascii.dxf"));
      DictionaryMutationAtFinalizeConsumer adapter;
      dx_data data;
      if (!adapter.enableWholeDxfSourceObservation() ||
          !adapter.enableLegacyDxfIdentityObservation() ||
          !adapter.import(input.string(), data) || !data.hasNamedObjectDictionary ||
          data.namedObjectDictionary.m_entries.size() != 1u ||
          data.dictionaries.size() != 1u || data.dictionaries.front().m_entries.empty())
        return 4;
      adapter.mutationTarget = &data;
      auto style = std::find_if(data.mLineStyles.begin(), data.mLineStyles.end(),
          [](const auto &value) { return value.handle == 0xD2u; });
      if (style == data.mLineStyles.end() || style->elements.size() != 2u)
        return 5;
      const auto expectedOffset = style->elements.front().offset + 0.25;
      style->elements.front().offset = expectedOffset;
      const auto savedRootEntry = data.namedObjectDictionary.m_entries.front();
      const auto savedChildEntries = data.dictionaries.front().m_entries;
      const auto savedChildEntry = savedChildEntries.front();
      if (savedRootEntry.m_name != "ACAD_MLINESTYLE")
        return 6;

      const std::string sentinel = "MLINE_DICTIONARY_CARRIER_DESTINATION";
      if (!writeFile(output, sentinel))
        return 7;
      data.namedObjectDictionary.m_entries.front().m_handle = 0xBu;
      if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
              &data, false, true) || readFile(output) != sentinel ||
          data.namedObjectDictionary.m_entries.front().m_handle != 0xBu ||
          style->elements.front().offset != expectedOffset)
        return 8;
      ++rootRefusals;
      data.namedObjectDictionary.m_entries.front() = savedRootEntry;

      if (!writeFile(output, sentinel))
        return 9;
      data.dictionaries.front().m_entries.front().m_handle = 0xBu;
      if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
              &data, false, true) || readFile(output) != sentinel ||
          data.dictionaries.front().m_entries.front().m_handle != 0xBu ||
          style->elements.front().offset != expectedOffset)
        return 10;
      ++childRefusals;
      data.dictionaries.front().m_entries.front() = savedChildEntry;

      if (!writeFile(output, sentinel))
        return 11;
      adapter.injectAtFinalize = true;
      if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
              &data, false, true) || readFile(output) != sentinel ||
          !adapter.injected ||
          !sameEntry(data.dictionaries.front().m_entries.front(), savedChildEntry) ||
          style->elements.front().offset != expectedOffset)
        return 12;
      ++lateRefusals;
      adapter.injectAtFinalize = false;
      if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
              &data, false, true))
        return 13;
      Consumer verifier;
      dx_data readback;
      if (!verifier.import(output.string(), readback))
        return 14;
      if (!readback.hasNamedObjectDictionary)
        return 14;
      const auto outputRootEntry = std::find_if(
          readback.namedObjectDictionary.m_entries.begin(),
          readback.namedObjectDictionary.m_entries.end(),
          [&](const auto& entry) { return entry.m_name == savedRootEntry.m_name; });
      if (outputRootEntry == readback.namedObjectDictionary.m_entries.end() ||
          std::count_if(readback.namedObjectDictionary.m_entries.begin(),
              readback.namedObjectDictionary.m_entries.end(), [&](const auto& entry) {
                return entry.m_name == savedRootEntry.m_name;
              }) != 1u ||
          outputRootEntry->m_dxfHandleCode != savedRootEntry.m_dxfHandleCode)
        return 14;
      const auto outputDictionary = std::find_if(readback.dictionaries.begin(),
          readback.dictionaries.end(), [&](const auto& dictionary) {
            return dictionary.handle == outputRootEntry->m_handle;
          });
      if (outputDictionary == readback.dictionaries.end() ||
          std::count_if(readback.dictionaries.begin(), readback.dictionaries.end(),
              [&](const auto& dictionary) {
                return dictionary.handle == outputRootEntry->m_handle;
              }) != 1u ||
          outputDictionary->m_entries.size() != savedChildEntries.size())
        return 14;
      for (const auto& expectedEntry : savedChildEntries) {
        const auto actualEntry = std::find_if(outputDictionary->m_entries.begin(),
            outputDictionary->m_entries.end(), [&](const auto& entry) {
              return entry.m_name == expectedEntry.m_name;
            });
        const auto targetStyle = std::find_if(readback.mLineStyles.begin(),
            readback.mLineStyles.end(), [&](const auto& value) {
              return value.name == expectedEntry.m_name;
            });
        if (actualEntry == outputDictionary->m_entries.end() ||
            actualEntry->m_dxfHandleCode != expectedEntry.m_dxfHandleCode ||
            targetStyle == readback.mLineStyles.end() ||
            actualEntry->m_handle != targetStyle->handle)
          return 14;
      }
      const auto restoredStyle = std::find_if(readback.mLineStyles.begin(),
          readback.mLineStyles.end(), [](const auto &value) {
            return value.handle == 0xD2u;
          });
      if (restoredStyle == readback.mLineStyles.end() ||
          restoredStyle->elements.size() != 2u ||
          restoredStyle->elements.front().offset != expectedOffset)
        return 15;
      ++profiles;
      ++retries;
      ++readbacks;
    }
  }
  std::cout << "MLINE dictionary-carrier geometry guard profiles=" << profiles
            << " root-refusals=" << rootRefusals
            << " child-refusals=" << childRefusals
            << " late-refusals=" << lateRefusals
            << " retries=" << retries << " readbacks=" << readbacks << '\n';
  return profiles == 4u && rootRefusals == 4u && childRefusals == 4u &&
      lateRefusals == 4u && retries == 4u && readbacks == 4u
      ? 0 : 16;
}

int main(int argc,char** argv) {
    if(argc==2&&std::string(argv[1])=="--mline-dictionary-carrier-geometry-guard")return testMlineDictionaryCarrierGeometryGuard();
    if(argc==2&&std::string(argv[1])=="--mline-layer-carrier-geometry-guard")return testMlineLayerCarrierGeometryGuard();
    if(argc==2&&std::string(argv[1])=="--mline-raw-section-geometry-guard")return testMlineRawSectionGeometryGuard();
    if(argc==2&&std::string(argv[1])=="--mline-deleted-consumer-geometry-guard")return testDeletedMlineConsumerGeometryGuard();
    if(argc==2&&std::string(argv[1])=="--mline-style-body-source-live")return testMlineStyleBodiesLive();
    if(argc==2&&std::string(argv[1])=="--mline-style-dictionary-source-live")return testMlineStyleDictionaryLive();
    if(argc==2&&std::string(argv[1])=="--mline-root-dictionary-source-live")return testMlineRootDictionaryLive();
    if(argc==2&&std::string(argv[1])=="--layer-source-live")return testMlineLayerLive();
    if(argc==2&&std::string(argv[1])=="--ltype-source-live")return testMlineLtypeLive();
    if(argc!=1)return 26;
    const auto staleNames=parseTags(sourceText());auto original=staleNames;
    for(const char* id:{"D100","D101"})original[field(original,record(original,"MLINE",id),2)].second="EFFECTIVE_STYLE";
    original[field(original,record(original,"MLINESTYLE","D"),2)].second="EFFECTIVE_STYLE";
    original[field(original,record(original,"MLINESTYLE","D2"),2)].second="RETARGET_STYLE";
    const auto dir=std::filesystem::temp_directory_path()/("mline-source-dependency-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(dir);
    struct Cleanup{std::filesystem::path dir;~Cleanup(){std::error_code ec;std::filesystem::remove_all(dir,ec);}} cleanup{dir};
    std::vector<Scenario> scenarios{{"clean",original,true}};
    {auto a=original;replaceAllHandles(a);scenarios.push_back({"changed-handles",std::move(a),true});}
    {auto a=original;renameSymbols(a);scenarios.push_back({"changed-names",std::move(a),true});}
    {auto a=original;insertLine(a);scenarios.push_back({"ordinary-line",std::move(a),true});}
    {auto a=original;a[field(a,record(a,"MLINESTYLE","D2"),70)].second="1907";scenarios.push_back({"documented-style-flags",std::move(a),true});}
    {auto a=original;insertLine(a);const auto r=record(a,"LINE","D200");const auto end=afterRecord(a,r);const Tags templateLine(a.begin()+static_cast<std::ptrdiff_t>(r),a.begin()+static_cast<std::ptrdiff_t>(end));
        auto pos=end;for(unsigned i=1;i<20u;++i){auto copy=templateLine;std::ostringstream name;name<<std::uppercase<<std::hex<<(0xD200u+i);copy[1].second=name.str();
            a.insert(a.begin()+static_cast<std::ptrdiff_t>(pos),copy.begin(),copy.end());pos+=copy.size();}scenarios.push_back({"multi-line",std::move(a),true});}
    scenarios.push_back({"stale-redundant-names",staleNames,false});
    {auto a=original;a[field(a,record(a,"MLINE","D100"),2)].second="RETARGET_STYLE";scenarios.push_back({"mline-name-other-style",std::move(a),false});}
    {auto a=original;a[field(a,record(a,"MLINE","D100"),2)].second="UNBOUND_STYLE";scenarios.push_back({"mline-name-unbound",std::move(a),false});}
    {auto a=original;a[field(a,record(a,"MLINE","D100"),2)].second="effective_style";scenarios.push_back({"mline-name-case",std::move(a),false});}
    {auto a=original;a[field(a,record(a,"LAYER","1E1"),6)].second="continuous";scenarios.push_back({"linetype-name-case",std::move(a),false});}
    {auto a=original;a[field(a,record(a,"MLINESTYLE","D"),2)].second="UNBOUND_STYLE";scenarios.push_back({"style-name-mismatch",std::move(a),false});}
    {auto a=original;a[field(a,record(a,"MLINESTYLE","D"),70)].second="130";scenarios.push_back({"unknown-style-bit-128",std::move(a),false});}
    {auto a=original;a[field(a,record(a,"MLINESTYLE","D"),70)].second="2050";scenarios.push_back({"unknown-style-bit-2048",std::move(a),false});}
    {auto a=original;a[field(a,record(a,"MLINESTYLE","D"),3)].second=std::string(256u,'A');scenarios.push_back({"long-style-description",std::move(a),false});}
    {auto a=original;for(auto& [code,text]:a)if((code==2||code==3)&&text=="EFFECTIVE_STYLE")text=std::string(33u,'N');scenarios.push_back({"long-mline-name",std::move(a),false});}
    {auto a=original;const auto i=field(a,record(a,"LAYER","1E1"),5);a.insert(a.begin()+static_cast<std::ptrdiff_t>(i+1u),{{102,"{TEST_HIDDEN"},{340,"D"},{102,"}"}});scenarios.push_back({"hidden-340",std::move(a),false});}
    {auto a=original;const auto i=afterRecord(a,record(a,"LAYER","1E1"));a.insert(a.begin()+static_cast<std::ptrdiff_t>(i),{{340,"D"}});scenarios.push_back({"unknown-layer-field",std::move(a),false});}
    {auto a=original;const auto i=afterRecord(a,record(a,"LAYER","1E1"));a.insert(a.begin()+static_cast<std::ptrdiff_t>(i),{{1001,"VENDOR"},{1005,"D"}});scenarios.push_back({"xdata",std::move(a),false});}
    {auto a=original;a[field(a,record(a,"DICTIONARY","C"),350)].second="1E1";scenarios.push_back({"wrong-root-target",std::move(a),false});}
    {auto a=original;const auto i=afterRecord(a,record(a,"DICTIONARY","C"));a.insert(a.begin()+static_cast<std::ptrdiff_t>(i),{{3,"EXTRA"},{350,"D2"}});scenarios.push_back({"multi-entry-root",std::move(a),false});}
    {auto a=original;a[field(a,record(a,"MLINE","D100"),340)].second="101";scenarios.push_back({"wrong-style-type",std::move(a),false});}
    {auto a=original;a[field(a,record(a,"MLINE","D100"),330)].second="203";scenarios.push_back({"wrong-model-owner",std::move(a),false});}
    {auto a=original;a[field(a,record(a,"MLINESTYLE","D"),330,0)].second="C";scenarios.push_back({"wrong-reactor",std::move(a),false});}
    {auto a=original;a[field(a,record(a,"LAYER","1E1"),6)].second="MISSING";scenarios.push_back({"missing-linetype",std::move(a),false});}
    {auto a=original;a[field(a,record(a,"BLOCK_RECORD","202"),2)].second="*Model_Space";scenarios.push_back({"duplicate-block-name",std::move(a),false});}
    {auto a=original;for(auto& [code,text]:a)if((code==2||code==3)&&text=="*Model_Space")text="*model_space";scenarios.push_back({"model-name-case",std::move(a),false});}
    {auto a=original;a[field(a,record(a,"LTYPE","101"),330)].second="1E0";scenarios.push_back({"wrong-table-owner",std::move(a),false});}
    {auto a=original;const auto table=record(a,"TABLE","100");a[field(a,table,70)].second="1";scenarios.push_back({"table-max-understated",std::move(a),false});}
    {auto a=original;a[field(a,record(a,"BLOCK","300"),70)].second="4";a[field(a,record(a,"BLOCK","300"),1)].second="external.dwg";scenarios.push_back({"external-block",std::move(a),false});}
    {auto a=original;const auto i=afterRecord(a,record(a,"MLINE","D100"));a.insert(a.begin()+static_cast<std::ptrdiff_t>(i),{{0,"ACAD_PROXY_ENTITY"},{5,"D300"},{330,"201"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbProxyEntity"},{340,"D"}});scenarios.push_back({"opaque-proxy",std::move(a),false});}
    {auto a=original;auto i=a.end()-1;a.insert(i,{{0,"SECTION"},{2,"VENDOR"},{1,"unqualified"},{0,"ENDSEC"}});scenarios.push_back({"unknown-section",std::move(a),false});}
    {auto a=original;const auto i=field(a,record(a,"LAYER","1E1"),5);a.insert(a.begin()+static_cast<std::ptrdiff_t>(i+1u),{{999,"source comment"}});scenarios.push_back({"comment",std::move(a),true});}
    unsigned candidates=0,refusals=0,noAuthority=0,importRefusals=0,quotaChecks=0,retries=0;
    std::size_t overflow=0u;if(dx_mline_source_dependency::sum((std::numeric_limits<std::size_t>::max)(),1u,overflow)
        ||dx_mline_source_dependency::product((std::numeric_limits<std::size_t>::max)(),2u,overflow))return 12;
    for(bool binary:{false,true})for(const auto& item:scenarios) {
        const auto path=dir/(item.name+(binary?"-binary.dxf":"-ascii.dxf"));const auto text=serialize(item.tags);std::string bytes;
        if(binary){if(!encodeBinary(text,bytes))return 1;}else bytes=text;if(!writeFile(path,bytes))return 2;
        auto adapter=std::make_unique<Consumer>();auto data=std::make_unique<dx_data>();adapter->enableWholeDxfSourceObservation();
        if(!adapter->import(path.string(),*data)) {if(item.admit)return 3;++importRefusals;continue;}
        const auto proof=DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data);
        const bool passed=proof.result==Result::SourceCandidate;
        std::cout<<(binary?"binary ":"ascii ")<<item.name<<" status="<<static_cast<int>(proof.result)<<" fields="<<proof.fields<<" nodes="<<proof.records<<"\n";
        if(passed!=item.admit)return 4;
        if(passed){++candidates;auto limits=dx_mline_source_dependency::Limits{};limits.work=proof.work;
            if(DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data,limits).result!=Result::SourceCandidate)return 5;
            --limits.work;if(DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data,limits).result!=Result::Limit)return 6;
            limits={};limits.fields=proof.fields;if(DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data,limits).result!=Result::SourceCandidate)return 24;
            --limits.fields;if(DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data,limits).result!=Result::Limit)return 7;
            limits={};limits.records=proof.records;if(DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data,limits).result!=Result::SourceCandidate)return 25;
            --limits.records;if(DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data,limits).result!=Result::Limit)return 8;
            const auto peak=DxWholeSourceTestAccess::peak(*data);if(!peak)return 26;
            limits={};limits.retainedPeak=peak;if(DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data,limits).result!=Result::SourceCandidate)return 27;
            --limits.retainedPeak;if(DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data,limits).result!=Result::Limit)return 28;
            limits={};limits.work=0u;if(DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data,limits).result!=Result::Limit)return 29;
            ++quotaChecks;
        }else {if(proof.result==Result::NoAuthority)++noAuthority;else ++refusals;auto retry=std::make_unique<dx_data>();const auto clean=dir/(binary?"clean-binary.dxf":"clean-ascii.dxf");
            if(!adapter->import(clean.string(),*retry))return 9;if(DxIfaceMlineDependencyTestAccess::qualify(*adapter,*retry).result!=Result::SourceCandidate)return 10;++retries;}
    }
    for(bool binary:{false,true}) {
        const auto clean=dir/(binary?"clean-binary.dxf":"clean-ascii.dxf");
        auto adapter=std::make_unique<Consumer>();adapter->enableWholeDxfSourceObservation();auto data=std::make_unique<dx_data>();
        DxWholeSourceTestAccess::limits(*adapter,WholeSourceV2::Limits{0u,64u*1024u*1024u,128u*1024u*1024u});
        if(!adapter->import(clean.string(),*data)||DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data).result!=Result::NoAuthority)return 13;
        DxWholeSourceTestAccess::limits(*adapter,{});data=std::make_unique<dx_data>();if(!adapter->import(clean.string(),*data))return 14;
        if(DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data).result!=Result::SourceCandidate)return 15;
        // This guard qualifies only original source, not edits to the live
        // model. A separate preflight must reject such changes before writing.
        if(data->namedObjectDictionary.m_entries.empty()||!data->mBlock)return 21;
        auto& entry=data->namedObjectDictionary.m_entries.front();const auto savedTarget=entry.m_handle;entry.m_handle=0u;
        if(DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data).result!=Result::SourceCandidate)return 22;entry.m_handle=savedTarget;
        const auto savedEntities=data->mBlock->ent;data->mBlock->ent.clear();
        if(DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data).result!=Result::SourceCandidate)return 23;data->mBlock->ent=savedEntities;
        const auto generation=data->importGeneration;++data->importGeneration;if(DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data).result!=Result::NoAuthority)return 16;data->importGeneration=generation;
        const auto version=data->sourceVersion;data->sourceVersion=DRW::UNKNOWNV;
        if(DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data).result!=Result::NoAuthority)return 30;data->sourceVersion=version;
        auto unrelated=std::make_unique<dx_data>();
        if(DxIfaceMlineDependencyTestAccess::qualify(*adapter,*unrelated).result!=Result::NoAuthority)return 31;
        std::ifstream in(clean,std::ios::binary);std::string bytes((std::istreambuf_iterator<char>(in)),{});bytes+="TRAILING";
        const auto extra=dir/(binary?"trailing-binary.dxf":"trailing-ascii.dxf");if(!writeFile(extra,bytes))return 17;
        data=std::make_unique<dx_data>();if(!adapter->import(extra.string(),*data)||DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data).result!=Result::NoAuthority)return 18;
        data=std::make_unique<dx_data>();if(!adapter->import(clean.string(),*data))return 19;
        if(DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data).result!=Result::SourceCandidate)return 20;
    }
    for(bool binary:{false,true})for(bool corrupt:{false,true}) {
        const auto clean=dir/(binary?"clean-binary.dxf":"clean-ascii.dxf");auto adapter=std::make_unique<SpoofingConsumer>();
        adapter->enableWholeDxfSourceObservation();adapter->corrupt=corrupt;adapter->skip=!corrupt;
        auto data=std::make_unique<dx_data>();if(!adapter->import(clean.string(),*data)
            ||DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data).result!=Result::NoAuthority)return 32;
        adapter->corrupt=adapter->skip=false;data=std::make_unique<dx_data>();
        if(!adapter->import(clean.string(),*data)||DxIfaceMlineDependencyTestAccess::qualify(*adapter,*data).result!=Result::SourceCandidate)return 33;
    }
    std::cout<<"candidates="<<candidates<<" refusals="<<refusals<<" no-authority="<<noAuthority<<" read-refusals="<<importRefusals<<" quota-controls="<<quotaChecks<<" retries="<<retries<<" callback-controls=4 live-mutation-not-qualified=demonstrated"<<'\n';
    if(candidates!=14u||refusals+noAuthority+importRefusals!=52u||quotaChecks!=14u||retries!=refusals+noAuthority)return 11;
}
