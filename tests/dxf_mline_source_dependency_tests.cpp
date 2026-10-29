// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_iface.h"
#include "dx_mline_layer_source_live.h"
#include "test_scratch.h"
#include "intern/dxfwriter.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
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
    static bool layers(const dx_iface& a,const dx_data& d,std::size_t work,std::size_t* used) noexcept{return a.mlineLayerSourceLiveProof(d,work,used);}
    static dx_mline_source_dependency::Proof qualify(const dx_iface& a,const dx_data& data,
            dx_mline_source_dependency::Limits limits={}) noexcept{return a.mlineDependencySourceProof(data,limits);}
};
class Consumer:public dx_iface {
public: bool import(const std::string& path,dx_data& data){return fileImport(path,&data,false);}
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

int main(int argc,char** argv) {
    if(argc==2&&std::string(argv[1])=="--layer-source-live")return testMlineLayerLive();
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
