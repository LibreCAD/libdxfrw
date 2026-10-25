#include "dx_iface.h"
#include "intern/dwgbufferw.h"
#include "intern/dxftablecontent.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <new>
#include <sstream>
#include <stdexcept>

// Scoped only around immutable report inspection below. No source fixture,
// reader, callback or assertion is built while allocation failure is armed.
namespace {
long symbolAllocationsBeforeFailure=-1;
struct SymbolAllocationFailure {
    explicit SymbolAllocationFailure(long count){symbolAllocationsBeforeFailure=count;}
    ~SymbolAllocationFailure(){symbolAllocationsBeforeFailure=-1;}
};
}
void* operator new(std::size_t size) {
    if(symbolAllocationsBeforeFailure==0)throw std::bad_alloc();
    if(symbolAllocationsBeforeFailure>0)--symbolAllocationsBeforeFailure;
    if(void* value=std::malloc(size?size:1u))return value;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void* value) noexcept {std::free(value);}
void operator delete[](void* value) noexcept {std::free(value);}
void operator delete(void* value,std::size_t) noexcept {std::free(value);}
void operator delete[](void* value,std::size_t) noexcept {std::free(value);}

class DrwObjectEncodeTestAccess {
public:
    static void pendingDictionaryEntry(DRW_Dictionary& row) {row.m_pendingEntryName="unfinished";}
};

class DxIfaceTableSourceTestAccess {
public:
    static void limits(dx_data& data, std::size_t records, std::size_t groups,
                       std::size_t bytes, std::size_t work=4000000u) {
        auto& source=data.m_tableDxfSource;
        source.maxRecords=records; source.maxGroups=groups;
        source.maxBytes=bytes; source.maxWork=work;
    }
    static const DRW_Table& table(const dx_data& data) {return data.m_tableDxfSource.tables.begin()->second.typed;}
    static const DRW_RawDxfObject& rawTable(const dx_data& data) {return data.m_tableDxfSource.tables.begin()->second.raw;}
    static const DRW_TableContentObject& content(const dx_data& data) {return data.m_tableDxfSource.contents.begin()->second;}
    static const DRW_TableGeometry& geometry(const dx_data& data) {return data.m_tableDxfSource.geometries.begin()->second;}
    static const DRW_XRecord& wrapper(const dx_data& data) {return data.m_tableDxfSource.xrecords.begin()->second;}
    static const std::vector<dxfTableContent::Reference>& references(const dx_data& data) {
        return data.m_tableDxfSource.contentReferences.begin()->second;
    }
    static void proofLimit(dx_data& data,std::size_t limit) {data.m_tableDxfSource.maxContentProofWork=limit;}
    static void resourceLimits(dx_data& data,std::size_t records,std::size_t items,std::size_t bytes,std::size_t work) {
        auto& source=data.m_tableDxfSource;
        source.maxResourceRecords=records;source.maxResourceItems=items;
        source.maxResourceBytes=bytes;source.maxBindingWork=work;
    }
    static const DRW_TableEntry* resource(const dx_data& data,std::uint32_t handle) {
        return data.m_tableDxfSource.resources.at(handle).model.get();
    }
    static const auto& mapResource(const dx_data& data,std::uint32_t handle=0xC120u) {
        return data.m_tableDxfSource.resources.at(handle);
    }
    static const auto& resources(const dx_data& data) {return data.m_tableDxfSource.resources;}
    static void resourceProofLimits(dx_data& data,std::size_t records,std::size_t groups,std::size_t bytes,std::size_t work) {
        auto& s=data.m_tableDxfSource;s.maxResourceProofRecords=records;s.maxResourceProofGroups=groups;
        s.maxResourceProofBytes=bytes;s.maxResourceProofWork=work;
    }
};

namespace {
struct Tag {int code; std::string value;};
using Tags=std::vector<Tag>;
struct Probe final : dx_iface {
    std::vector<DRW_TableContentObject> content;
    std::vector<DRW_RawDxfObject> raw;
    unsigned unknown=0u;
    unsigned tableSources=0u, tableCallbacks=0u;
    unsigned mapCallbacks=0u, rawMapCallbacks=0u;
    bool optIn=true, forceOptIn=false, throwOnTable=false, mutateTable=false, paddedRaw=false;
    std::size_t largestTableSourceString=0u;
    std::function<void(DRW_TableContentObject&)> alterContent;
    std::function<void(DRW_RawDxfObject&)> alterContentRaw;
    std::function<void(DRW_CellStyleMap&)> alterMap;
    std::function<void(DRW_RawDxfObject&)> alterMapRaw;
    bool skipMapRaw=false,duplicateMapRaw=false;
    std::function<void(DRW_TableStyle&)> alterStyle;
    std::function<void(DRW_RawDxfObject&)> alterStyleRaw;
    bool skipStyleRaw=false,duplicateStyleRaw=false;
    bool keepMutableAliases=false;
    bool duplicateStyleCallback=false;
    bool symbolOptIn=true, forceSymbols=false, observeSymbols=false;
    bool skipSymbolRaw=false, duplicateSymbolRaw=false, throwSymbolRaw=false;
    std::uint32_t skipSymbolTypedHandle=0u;
    std::function<void(DRW_RawDxfObject&)> alterSymbolRaw;
    std::function<void(DRW_Textstyle&)> alterTextStyle;
    std::function<void(DRW_LType&)> alterLType;
    std::function<void(DRW_Field&)> alterField;
    std::function<void(DRW_DataLink&)> alterDataLink;
    std::function<void(DRW_RawDxfObject&)> alterAuxRaw;
    std::uint32_t skipAuxRaw=0u,duplicateAuxRaw=0u,skipAuxTyped=0u,duplicateAuxTyped=0u;
    std::uint32_t throwAuxTyped=0u;
    unsigned fieldCallbacks=0u,dataLinkCallbacks=0u;
    std::function<void(DRW_Dictionary&)> alterDictionary;
    std::function<void(DRW_XRecord&)> alterXRecord;
    std::function<void(DRW_RawDxfObject&)> alterSpineRaw;
    std::uint32_t skipSpineRaw=0u,duplicateSpineRaw=0u,skipSpineTyped=0u,duplicateSpineTyped=0u;
    bool rootOptIn=true,throwRootSource=false,wrongSpineThenOriginal=false;
    long rootSourceAllocation=-1;
    unsigned rootSources=0u;
    std::vector<std::string> symbolEvents;
    std::shared_ptr<DRW_TableContentDxfMetadata> mutableAlias;
    std::shared_ptr<DRW_TableStyleContentFormat> mutableFormatAlias;
    void clear() { content.clear(); raw.clear(); symbolEvents.clear(); unknown=0u; }
    void addTableContent(const DRW_TableContentObject& value) override {
        auto copy=value;
        if (alterContent || keepMutableAliases) {
            auto metadata=std::make_shared<DRW_TableContentDxfMetadata>(*value.m_dxfMetadata);
            for (auto& row:metadata->m_rows) for (auto& cell:row.m_cells) for (auto& format:cell.m_contentFormats)
                if (format) {
                    auto mutableFormat=std::make_shared<DRW_TableStyleContentFormat>(*format);
                    if (keepMutableAliases) mutableFormatAlias=mutableFormat;
                    format=std::move(mutableFormat);
                }
            if (keepMutableAliases) mutableAlias=metadata;
            copy.m_dxfMetadata=std::move(metadata);
            if (alterContent) alterContent(copy);
        }
        content.push_back(copy); dx_iface::addTableContent(copy);
    }
    void addRawDxfObject(const DRW_RawDxfObject& value) override {
        if(value.name=="DICTIONARY"||value.name=="XRECORD") {
            if(value.handle==skipSpineRaw)return;
            if(wrongSpineThenOriginal){auto wrong=value;wrong.name="UNRECOGNIZED_RESOURCE";dx_iface::addRawDxfObject(wrong);}
            if(alterSpineRaw){auto copy=value;alterSpineRaw(copy);dx_iface::addRawDxfObject(copy);}else dx_iface::addRawDxfObject(value);
            if(value.handle==duplicateSpineRaw)dx_iface::addRawDxfObject(value);
            return;
        }
        if(value.name=="FIELD"||value.name=="ACDBFIELD"||value.name=="ACAD_FIELD"||value.name=="DATALINK") {
            if(skipAuxRaw&&value.handle==skipAuxRaw)return;
            if(alterAuxRaw){auto copy=value;alterAuxRaw(copy);dx_iface::addRawDxfObject(copy);}
            else dx_iface::addRawDxfObject(value);
            if(duplicateAuxRaw&&value.handle==duplicateAuxRaw)dx_iface::addRawDxfObject(value);
            return;
        }
        if(value.name=="TABLESTYLE") {
            if(skipStyleRaw)return;
            if(alterStyleRaw){auto copy=value;alterStyleRaw(copy);dx_iface::addRawDxfObject(copy);if(duplicateStyleRaw)dx_iface::addRawDxfObject(value);return;}
            if(duplicateStyleRaw)dx_iface::addRawDxfObject(value);
        }
        if (value.name=="CELLSTYLEMAP") ++rawMapCallbacks;
        if (value.name=="CELLSTYLEMAP") {
            if (skipMapRaw) return;
            if (alterMapRaw) {auto copy=value;alterMapRaw(copy);dx_iface::addRawDxfObject(copy);if(duplicateMapRaw)dx_iface::addRawDxfObject(value);return;}
            if (duplicateMapRaw) dx_iface::addRawDxfObject(value);
        }
        if ((value.name=="TABLECONTENT" || value.name=="aCdBtAbLeCoNtEnT") && alterContentRaw) {
            auto copy=value; alterContentRaw(copy); raw.push_back(copy); dx_iface::addRawDxfObject(copy); return;
        }
        if (value.name=="TABLECONTENT" || value.name=="aCdBtAbLeCoNtEnT") raw.push_back(value);
        dx_iface::addRawDxfObject(value);
    }
    void addCellStyleMap(const DRW_CellStyleMap& value) override {
        ++mapCallbacks;
        if(alterMap){auto copy=value;alterMap(copy);dx_iface::addCellStyleMap(copy);}
        else dx_iface::addCellStyleMap(value);
    }
    void addTableStyle(const DRW_TableStyle& value) override {
        if(alterStyle){auto copy=value;alterStyle(copy);dx_iface::addTableStyle(copy);}
        else dx_iface::addTableStyle(value);
    }
    void addUnmodeledDxfObject(const DRW_RawDxfObject& value) override {
        ++unknown; dx_iface::addUnmodeledDxfObject(value);
    }
    bool wantsDxfTableSource() const override {return optIn && (forceOptIn || dx_iface::wantsDxfTableSource());}
    void addDxfTableSource(const DRW_Table& table,const DRW_RawDxfObject& source) override {
        ++tableSources;
        for (const auto& group:source.groups) if (group.type()==DRW_Variant::STRING && group.content.s)
            largestTableSourceString=std::max(largestTableSourceString,group.content.s->size());
        if (paddedRaw) {
            auto padded=source;
            padded.groups.emplace_back(1,std::string("x\0",2u)+std::string(128u*1024u,'q'));
            padded.rawValues.emplace_back("x");
            dx_iface::addDxfTableSource(table,padded);
        } else dx_iface::addDxfTableSource(table,source);
        if (throwOnTable) throw std::runtime_error("injected source callback failure");
    }
    void addTable(const DRW_Table& table) override {
        ++tableCallbacks;
        if (mutateTable) const_cast<DRW_Table&>(table).name="mutated-after-source-capture";
        dx_iface::addTable(table);
    }
    void addTextStyle(const DRW_Textstyle& style) override {
        if(style.handle==skipSymbolTypedHandle && skipSymbolTypedHandle!=0u)return;
        if(observeSymbols)symbolEvents.push_back("typed:STYLE");
        if(alterTextStyle){auto copy=style;alterTextStyle(copy);dx_iface::addTextStyle(copy);}
        else dx_iface::addTextStyle(style);
        if (duplicateStyleCallback) dx_iface::addTextStyle(style);
    }
    void addLType(const DRW_LType& value) override {
        if(value.handle==skipSymbolTypedHandle && skipSymbolTypedHandle!=0u)return;
        if(observeSymbols)symbolEvents.push_back("typed:LTYPE");
        if(alterLType){auto copy=value;alterLType(copy);dx_iface::addLType(copy);}
        else dx_iface::addLType(value);
    }
    void addField(const DRW_Field& value) override {
        ++fieldCallbacks;if(skipAuxTyped&&value.handle==skipAuxTyped)return;
        if(alterField){auto copy=value;alterField(copy);dx_iface::addField(copy);}else dx_iface::addField(value);
        if(duplicateAuxTyped&&value.handle==duplicateAuxTyped)dx_iface::addField(value);
        if(throwAuxTyped&&value.handle==throwAuxTyped)throw std::runtime_error("injected FIELD callback exception");
    }
    void addDataLink(const DRW_DataLink& value) override {
        ++dataLinkCallbacks;if(skipAuxTyped&&value.handle==skipAuxTyped)return;
        if(alterDataLink){auto copy=value;alterDataLink(copy);dx_iface::addDataLink(copy);}else dx_iface::addDataLink(value);
        if(duplicateAuxTyped&&value.handle==duplicateAuxTyped)dx_iface::addDataLink(value);
        if(throwAuxTyped&&value.handle==throwAuxTyped)throw std::runtime_error("injected DATALINK callback exception");
    }
    bool wantsDxfSymbolSource() const override {
        return symbolOptIn && (forceSymbols || dx_iface::wantsDxfSymbolSource());
    }
    bool wantsDxfRootDictionarySource() const override {return rootOptIn&&dx_iface::wantsDxfRootDictionarySource();}
    void addDxfRootDictionarySource(const DRW_Dictionary& row,const DRW_RawDxfObject& raw) override {
        ++rootSources;
        if(throwRootSource)throw std::runtime_error("injected root source failure");
        if(raw.handle==skipSpineRaw)return;
        if(alterSpineRaw){auto copy=raw;alterSpineRaw(copy);dx_iface::addDxfRootDictionarySource(row,copy);}
        else {SymbolAllocationFailure failure(rootSourceAllocation);dx_iface::addDxfRootDictionarySource(row,raw);}
        if(raw.handle==duplicateSpineRaw)dx_iface::addDxfRootDictionarySource(row,raw);
    }
    void dictionary(const DRW_Dictionary& row,bool root) {
        if(row.handle==skipSpineTyped)return;
        auto copy=row;if(alterDictionary)alterDictionary(copy);
        if(root)dx_iface::addNamedObjectDictionary(copy);else dx_iface::addDictionary(copy);
        if(row.handle==duplicateSpineTyped) {
            if(root)dx_iface::addNamedObjectDictionary(copy);else dx_iface::addDictionary(copy);
        }
    }
    void addNamedObjectDictionary(const DRW_Dictionary& row) override {dictionary(row,true);}
    void addDictionary(const DRW_Dictionary& row) override {dictionary(row,false);}
    void addXRecord(const DRW_XRecord& row) override {
        if(row.handle==skipSpineTyped)return;
        auto copy=row;if(alterXRecord)alterXRecord(copy);dx_iface::addXRecord(copy);
        if(row.handle==duplicateSpineTyped)dx_iface::addXRecord(copy);
    }
    void addDxfSymbolSource(const DRW_TableEntry& row,const DRW_RawDxfObject& source) override {
        if(observeSymbols)symbolEvents.push_back("raw:"+source.name);
        if(skipSymbolRaw)return;
        if(alterSymbolRaw){auto copy=source;alterSymbolRaw(copy);dx_iface::addDxfSymbolSource(row,copy);}
        else dx_iface::addDxfSymbolSource(row,source);
        if(duplicateSymbolRaw)dx_iface::addDxfSymbolSource(row,source);
        if(throwSymbolRaw)throw std::runtime_error("injected symbol source failure");
    }
};
void format(Tags& tags) {
    tags.insert(tags.end(),{{300,"CONTENTFORMAT"},{1,"CONTENTFORMAT_BEGIN"},{90,"3"},{91,"256"},
        {92,"4"},{93,"0"},{300,"fmt-é"},{40,"0.125"},{140,"2"},{94,"5"},{62,"0"},{420,"1193046"},
        {340,"C101"},{144,"3.25"},{309,"CONTENTFORMAT_END"}});
}
void style(Tags& tags,int kind,bool rich) {
    tags.insert(tags.end(),{{1,"TABLEFORMAT_BEGIN"},{90,std::to_string(kind)},{170,rich?"1":"0"}});
    if (rich) {
        tags.insert(tags.end(),{{91,"512"},{92,"32768"},{62,"257"},{93,"1"}}); format(tags);
        tags.insert(tags.end(),{{171,"1"},{301,"MARGIN"},{1,"CELLMARGIN_BEGIN"},{40,"1"},{40,"2"},
            {40,"3"},{40,"4"},{40,"5"},{40,"6"},{309,"CELLMARGIN_END"},{94,"1"},{95,"1"},
            {302,"GRIDFORMAT"},{1,"GRIDFORMAT_BEGIN"},{90,"63"},{91,"2"},{62,"7"},{92,"17"},
            {340,"C102"},{93,"1"},{40,"0.25"},{309,"GRIDFORMAT_END"}});
    }
    tags.push_back({309,"TABLEFORMAT_END"});
}
void custom(Tags& tags,int datum,bool checksum) {
    tags.insert(tags.end(),{{91,std::to_string(datum)},{301,"CUSTOMDATA"},{1,"DATAMAP_BEGIN"},{90,checksum?"1":"0"}});
    if (checksum) tags.insert(tags.end(),{{300,"CHECKSUM"},{301,"DATAMAP_VALUE"},{93,"2"},{90,"2"},
        {140,"1127"},{94,"0"},{300,""},{302,""},{304,"ACVALUE_END"}});
    tags.push_back({309,"DATAMAP_END"});
}
Tags source(const std::string& version,const std::string& type) {
    Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"},{0,"SECTION"},{2,"OBJECTS"},
        {0,type},{5,"A0"},{330,"B0"},{100,"AcDbLinkedData"},{1,"Name-é"},{300,"Description"},
        {100,"AcDbLinkedTableData"},{90,"1"},{300,"COLUMN"},{1,"LINKEDTABLEDATACOLUMN_BEGIN"},{300,"Column-é"}};
    custom(tags,-11,true);
    tags.insert(tags.end(),{{309,"LINKEDTABLEDATACOLUMN_END"},{1,"FORMATTEDTABLEDATACOLUMN_BEGIN"},{300,"COLUMNTABLEFORMAT"}});
    style(tags,3,true);
    tags.insert(tags.end(),{{309,"FORMATTEDTABLEDATACOLUMN_END"},{1,"TABLECOLUMN_BEGIN"},{90,"1"},{40,"22.5"},
        {309,"TABLECOLUMN_END"},{91,"1"},{301,"ROW"},{1,"LINKEDTABLEDATAROW_BEGIN"},{90,"1"},
        {300,"CELL"},{1,"LINKEDTABLEDATACELL_BEGIN"},{90,"65"},{300,"Tooltip-é"}});
    custom(tags,-21,true);
    tags.insert(tags.end(),{{92,"1"},{340,"F001"},{93,"2"},{94,"3"},{96,"-5"},{95,"3"},
        {302,"CONTENT"},{1,"CELLCONTENT_BEGIN"},{90,"1"},{300,"VALUE"},{93,"6"},{90,"4"},
        {1,"TABLEFORMAT_END ^J text-é"},{94,"4"},{300,"%s"},{302,"shown-é"},{304,"ACVALUE_END"},
        {91,"0"},{309,"CELLCONTENT_END"},{1,"FORMATTEDCELLCONTENT_BEGIN"},{170,"1"}});
    format(tags);
    tags.insert(tags.end(),{{309,"FORMATTEDCELLCONTENT_END"},{302,"CONTENT"},{1,"CELLCONTENT_BEGIN"},
        {90,"2"},{340,"F100"},{91,"1"},{330,"D001"},{301,"attribute-one"},{92,"1"},
        {309,"CELLCONTENT_END"},{1,"FORMATTEDCELLCONTENT_BEGIN"},{170,"0"},{309,"FORMATTEDCELLCONTENT_END"},
        {302,"CONTENT"},{1,"CELLCONTENT_BEGIN"},{90,"4"},{340,"B100"},{91,"1"},{330,"D002"},
        {301,"attribute-two"},{92,"2"},{309,"CELLCONTENT_END"},{1,"FORMATTEDCELLCONTENT_BEGIN"},
        {170,"0"},{309,"FORMATTEDCELLCONTENT_END"},{309,"LINKEDTABLEDATACELL_END"},
        {1,"FORMATTEDTABLEDATACELL_BEGIN"},{300,"CELLTABLEFORMAT"}});
    style(tags,1,true);
    tags.insert(tags.end(),{{309,"FORMATTEDTABLEDATACELL_END"},{1,"TABLECELL_BEGIN"},{90,"101"},
        {91,"1"},{91,"3"},{40,"14.5"},{41,"6.25"},{94,"2"},{330,"A700"}});
    for (int i=0;i!=2;++i) tags.insert(tags.end(),{{10,std::to_string(i+1)},{20,"2"},{30,"3"},
        {11,"4"},{21,"5"},{31,"6"},{43,"7"},{44,"8"},{45,"9"},{46,"10"},{95,"-7"}});
    tags.push_back({309,"TABLECELL_END"}); custom(tags,-31,true);
    tags.insert(tags.end(),{{309,"LINKEDTABLEDATAROW_END"},{1,"FORMATTEDTABLEDATAROW_BEGIN"},{300,"ROWTABLEFORMAT"}});
    style(tags,2,false);
    tags.insert(tags.end(),{{309,"FORMATTEDTABLEDATAROW_END"},{1,"TABLEROW_BEGIN"},{90,"2"},{40,"8.5"},
        {309,"TABLEROW_END"},{92,"4660"},{100,"AcDbFormattedTableData"},{300,"TABLEFORMAT"}});
    style(tags,4,false);
    tags.insert(tags.end(),{{90,"1"},{91,"0"},{92,"0"},{93,"0"},{94,"0"},
        {100,"AcDbTableContent"},{340,"C100"},{0,"ENDSEC"},{0,"EOF"}});
    return tags;
}
bool write(const std::filesystem::path& path,const Tags& tags,bool binary) {
    std::ofstream out(path,std::ios::binary);
    const auto word=[&](std::uint64_t value,unsigned bytes) {for (unsigned i=0;i<bytes;++i) out.put(static_cast<char>(value>>(8u*i)));};
    if (binary) out.write("AutoCAD Binary DXF\r\n\x1a\0",22);
    for (const auto& tag:tags) {
        if (!binary) out<<tag.code<<'\n'<<tag.value<<'\n';
        else {
            word(static_cast<unsigned>(tag.code),2u);
            if ((tag.code>=90 && tag.code<=99) || tag.code==420 || tag.code==1071) word(static_cast<std::uint32_t>(std::stoll(tag.value)),4u);
            else if(tag.code>=160&&tag.code<=169)word(static_cast<std::uint64_t>(std::stoll(tag.value)),8u);
            else if (tag.code==310 || tag.code==1004) {
                out.put(static_cast<char>(tag.value.size()/2u));
                for (std::size_t i=0;i<tag.value.size();i+=2u)
                    out.put(static_cast<char>(std::stoul(tag.value.substr(i,2u),nullptr,16)));
            }
            else if ((tag.code>=60 && tag.code<=79) || tag.code==1070) word(static_cast<std::uint16_t>(std::stoi(tag.value)),2u);
            else if (tag.code>=170 && tag.code<=179) word(static_cast<std::uint16_t>(std::stoi(tag.value)),2u);
            else if (tag.code>=270 && tag.code<=289) word(static_cast<std::uint16_t>(std::stoi(tag.value)),2u);
            else if ((tag.code>=10 && tag.code<=59) || tag.code==140 || tag.code==144 || (tag.code>=1010 && tag.code<=1059)) {
                const double value=std::stod(tag.value); std::uint64_t bits=0u; std::memcpy(&bits,&value,8u); word(bits,8u);
            } else {out<<tag.value; out.put('\0');}
        }
    }
    return out.good();
}
std::size_t slot(const Tags& tags,int code,std::string_view text={}) {
    for (std::size_t i=0;i<tags.size();++i) if (tags[i].code==code && (text.empty() || tags[i].value==text)) return i;
    throw std::runtime_error("mutation slot missing");
}
bool expected(const Probe& p,bool binary) {
    if (p.content.size()!=1u || p.raw.size()!=1u || p.unknown!=0u) return false;
    const auto& o=p.content.front();
    if (!o.m_parseComplete || o.m_semanticContentComplete || !o.m_dxfMetadata || o.handle!=0xA0u || o.parentHandle!=0xB0u
        || o.m_content.m_name!="Name-é" || o.m_content.m_columns.size()!=1u || o.m_content.m_rows.size()!=1u
        || o.m_content.m_columns[0].m_width!=22.5 || o.m_content.m_rows[0].m_height!=8.5
        || o.m_content.m_tableStyleHandle!=0xC100u || o.m_content.m_mergedRanges.size()!=1u) return false;
    const auto& c=o.m_content.m_rows[0].m_cells[0]; const auto& m=*o.m_dxfMetadata;
    const std::string content=binary?"TABLEFORMAT_END ^J text-é":"TABLEFORMAT_END \n text-é";
    if (c.m_contents.size()!=3u || c.m_contents[0].m_text!=content || c.m_contents[0].m_value.m_valueString!="shown-é"
        || c.m_contents[1].m_handle!=0xF100u || c.m_contents[2].m_handle!=0xB100u
        || c.m_dataLinkHandle!=0xF001u || c.m_linkedDataUnknown!=-5 || c.m_linkedDataRowCount!=2
        || c.m_linkedDataColumnCount!=3 || c.m_attributes.size()!=2u || c.m_attributes[1].m_contentIndex!=2u
        || c.m_attributes[0].m_attdefHandle!=0xD001u || c.m_geometryHandle!=0xA700u) return false;
    if (m.m_linkedDataUnknown92!=4660 || m.m_columns[0].m_customData!=-11 || m.m_rows[0].m_scope.m_customData!=-31
        || m.m_rows[0].m_cells[0].m_scope.m_customData!=-21) return false;
    for (const auto* scope:{&m.m_columns[0],&m.m_rows[0].m_scope,&m.m_rows[0].m_cells[0].m_scope}) {
        if (scope->m_customItems.size()!=1u || scope->m_customItems[0].m_value.m_formatFlags!=2
            || scope->m_customItems[0].m_value.m_value.d_val()!=1127) return false;
    }
    const auto& cell=m.m_rows[0].m_cells[0];
    if (cell.m_geometry.size()!=2u || cell.m_geometry[1].m_topLeft.x!=2 || cell.m_geometry[1].m_topLeft.z!=3
        || cell.m_geometry[1].m_unknown!=-7 || cell.m_contentFormats.size()!=3u || !cell.m_contentFormats[0]
        || cell.m_contentFormats[1] || cell.m_contentFormats[0]->m_textStyleHandle!=0xC101u
        || cell.m_contentFormats[0]->m_contentColorCmc.m_rgb!=0x123456
        || cell.m_scope.m_style.m_marginVerticalSpacing!=6 || cell.m_scope.m_style.m_borders.size()!=1u
        || cell.m_scope.m_style.m_borders[0].m_lineTypeHandle!=0xC102u) return false;
    DRW_TableContentObject copy=o;
    if (copy.m_dxfMetadata!=o.m_dxfMetadata) return false;
    copy.m_semanticContentComplete=true;
    if (copy.validateDwgPayload(DRW::AC1021)) return false;
    struct Encoder : DRW_TableContentObject {using DRW_TableContentObject::encodeDwg;};
    Encoder encoder; static_cast<DRW_TableContentObject&>(encoder)=copy;
    dwgBufferW body,strings,handles; body.putRawChar8(0xA1); strings.putRawChar8(0xB2); handles.putRawChar8(0xC3);
    if (encoder.encodeDwg(DRW::AC1021,&body,&strings,&handles)
        || body.data()!=std::vector<std::uint8_t>{0xA1} || strings.data()!=std::vector<std::uint8_t>{0xB2}
        || handles.data()!=std::vector<std::uint8_t>{0xC3} || body.bitPos() || strings.bitPos() || handles.bitPos()) return false;
    copy.reset();
    return !copy.m_dxfMetadata && !copy.m_parseComplete && o.m_dxfMetadata!=nullptr;
}
bool tests(const std::filesystem::path& dir) {
    const auto path=dir/"source.dxf", output=dir/"output.dxf";
    const auto fail=[](const char* text) {std::cerr<<text<<'\n'; return false;};
    const std::vector<std::function<void(Tags&)>> faults{
        [](Tags& t){t[slot(t,90)].value="1001";}, [](Tags& t){t[slot(t,90)].value="-1";},
        [](Tags& t){t[slot(t,91,"1")].value="10001";},
        [](Tags& t){t[slot(t,1,"DATAMAP_BEGIN")].value="UNKNOWN_BEGIN";},
        [](Tags& t){t.erase(t.begin()+static_cast<std::ptrdiff_t>(slot(t,304)));},
        [](Tags& t){t[slot(t,95,"3")].value="10001";},
        [](Tags& t){t[slot(t,1,"TABLEFORMAT_END ^J text-é")-1u].value="128";},
        [](Tags& t){t[slot(t,1,"TABLEFORMAT_END ^J text-é")-2u].value="7";},
        [](Tags& t){t[slot(t,170,"1")].value="2";},
        [](Tags& t){t[slot(t,171)].value="2";},
        [](Tags& t){t[slot(t,94,"1")].value="7";},
        [](Tags& t){t[slot(t,340,"C101")].value="10000C101";},
        [](Tags& t){t[slot(t,330,"D001")].value="10000D001";},
        [](Tags& t){t.insert(t.end()-2,{100,"FutureSubclass"});},
        [](Tags& t){t[slot(t,309,"FORMATTEDTABLEDATACELL_END")].value="FORMATTEDTABLEDATAROW_END";},
        [](Tags& t){t[slot(t,340,"C100")-2u].value="1";},
        [](Tags& t){t[slot(t,100,"AcDbLinkedTableData")].value="AcDbFutureLinkedData";},
        [](Tags& t){t.insert(t.begin()+static_cast<std::ptrdiff_t>(slot(t,100)),{{102,"{ACAD_REACTORS"},{330,"AB"},{102,"}"}});}
    };
    unsigned routes=0u, fallbacks=0u, refusals=0u, values=0u;
    for (const auto* version:{"AC1021","AC1024","AC1027","AC1032"}) for (bool binary:{false,true})
        for (const auto* type:{"TABLECONTENT","aCdBtAbLeCoNtEnT"}) {
            const auto tags=source(version,type); Probe p; dx_data data;
            if (!write(path,tags,binary) || !p.fileImport(path.string(),&data,false) || !expected(p,binary))
                return fail("rich TABLECONTENT source projection differs");
            ++routes;
            for (const auto& mutate:faults) {
                auto broken=tags; mutate(broken); p.clear(); dx_data bad;
                if (!write(path,broken,binary) || !p.fileImport(path.string(),&bad,false)
                    || !p.content.empty() || p.raw.size()!=1u || p.unknown!=1u || !bad.standaloneTableContentPresent)
                    return fail("unmatched TABLECONTENT published partial fields or lost fallback");
                p.clear(); dx_data restored;
                if (!write(path,tags,binary) || !p.fileImport(path.string(),&restored,false) || !expected(p,binary)
                    || !bad.standaloneTableContentPresent) return fail("corrected TABLECONTENT import failed");
                ++fallbacks;
            }
            for (bool strict:{false,true}) {
                const std::string sentinel="PRESERVE_CONTENT_FAILURE";
                {std::ofstream out(output,std::ios::binary); out<<sentinel;}
                dx_iface exporter;
                if (exporter.fileExport(output.string(),DRW::AC1032,!binary,&data,false,strict))
                    return fail("rich TABLECONTENT source escaped output guard");
                std::ifstream in(output,std::ios::binary);
                if (std::string(std::istreambuf_iterator<char>(in),{})!=sentinel) return fail("failed export changed destination");
                dx_data clean; dx_iface author; author.cData=&clean; author.currentBlock=clean.mBlock;
                DRW_Line line; line.basePoint={1,2,3}; line.secPoint={4,5,6}; author.addLine(line);
                if (!exporter.fileExport(output.string(),DRW::AC1032,!binary,&clean,false,strict))
                    return fail("same exporter did not recover after rich-source refusal");
                dx_data readback; dx_iface actual;
                if (!actual.fileImport(output.string(),&readback,false) || readback.mBlock->ent.size()!=1u)
                    return fail("corrected output was not readable");
                const auto* readLine=dynamic_cast<const DRW_Line*>(readback.mBlock->ent.front());
                if (!readLine || readLine->basePoint.z!=3 || readLine->secPoint.z!=6 || !data.standaloneTableContentPresent)
                    return fail("corrected readback changed geometry or prior-container loss state");
                ++refusals;
            }
        }
    const std::vector<Tags> variants{
        {{93,"2"},{90,"0"},{91,"-3"}}, {{93,"2"},{90,"1"},{91,"-2147483648"}},
        {{93,"2"},{90,"2"},{140,"1.25"}}, {{93,"6"},{90,"4"},{1,"scalar-é"}},
        {{93,"2"},{90,"8"},{92,"8"},{310,"01020304"},{310,"05060708"}},
        {{93,"2"},{90,"16"},{92,"16"},{11,"1.25"},{21,"2.5"}},
        {{93,"2"},{90,"32"},{92,"24"},{11,"1.25"},{21,"2.5"},{31,"3.75"}},
        {{93,"2"},{90,"64"},{330,"D111"}}, {{93,"1"},{90,"512"}}, {{93,"3"},{90,"4"}}
    };
    for (bool binary:{false,true}) for (std::size_t i=0;i<variants.size();++i) {
        auto tags=source("AC1032","TABLECONTENT");
        const auto first=slot(tags,1,"TABLEFORMAT_END ^J text-é")-2u;
        std::size_t last=first; while (tags[last].code!=304) ++last;
        Tags replacement=variants[i];
        replacement.insert(replacement.end(),{{94,"0"},{300,""},{302,""},{304,"ACVALUE_END"}});
        tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(first),tags.begin()+static_cast<std::ptrdiff_t>(last+1u));
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(first),replacement.begin(),replacement.end());
        Probe p; dx_data data;
        if (!write(path,tags,binary) || !p.fileImport(path.string(),&data,false) || p.content.size()!=1u || p.unknown)
            return fail("primitive CAD value source failed");
        const auto& value=p.content[0].m_content.m_rows[0].m_cells[0].m_contents[0].m_value;
        if (value.m_formatFlags!=std::stoi(variants[i][0].value) || value.m_dataType!=std::stoi(variants[i][1].value)) return false;
        const bool correct=i==0?value.m_value.type()==DRW_Variant::INTEGER && value.m_value.i_val()==-3
            :i==1?value.m_value.type()==DRW_Variant::INTEGER && value.m_value.i_val()==(-2147483647-1)
            :i==2?value.m_value.type()==DRW_Variant::DOUBLE && value.m_value.d_val()==1.25
            :i==3?value.m_value.type()==DRW_Variant::STRING && *value.m_value.content.s=="scalar-é"
            :i==4?value.m_dataSize==8u && value.m_rawData==std::vector<std::uint8_t>({1,2,3,4,5,6,7,8})
            :i==5 || i==6?value.m_value.type()==DRW_Variant::COORD && value.m_value.content.v->x==1.25
                && value.m_value.content.v->y==2.5 && value.m_value.content.v->z==(i==5?0:3.75)
            :i==7?value.m_handle==0xD111u:value.m_value.type()==DRW_Variant::INVALID;
        if (!correct) return fail("primitive CAD value width/type/presence differs");
        ++values;
    }
    auto tags=source("AC1032","TABLECONTENT"); tags[slot(tags,1,"Name-é")].value=std::string(16000u,'r');
    if (!write(path,tags,false)) return false;
    dxfRW reader(path.string().c_str()); reader.setDxfReadRetainedByteBudget(40000u);
    Probe probe; dx_data bounded; probe.cData=&bounded; probe.currentBlock=bounded.mBlock;
    if (reader.read(&probe,false) || !probe.content.empty() || !probe.raw.empty()) return fail("typed text budget published a partial cache");
    reader.setDxfReadRetainedByteBudget(2u*1024u*1024u); probe.clear(); dx_data restored;
    probe.cData=&restored; probe.currentBlock=restored.mBlock;
    if (!reader.read(&probe,false) || probe.content.size()!=1u || probe.content[0].m_content.m_name.size()!=16000u)
        return fail("retained-byte budget recovery failed");
    // A consumed raw body cannot turn a typed-node exhaustion into a successful
    // fallback callback. Both owners are part of the same read operation.
    reader.setDxfReadTypedVariantNodeBudget(1u); probe.clear(); dx_data nodeLimited;
    probe.cData=&nodeLimited; probe.currentBlock=nodeLimited.mBlock;
    if (reader.read(&probe,false) || !probe.content.empty() || !probe.raw.empty())
        return fail("typed-node exhaustion published a cache");
    reader.setDxfReadTypedVariantNodeBudget(100000u); probe.clear(); dx_data nodeRestored;
    probe.cData=&nodeRestored; probe.currentBlock=nodeRestored.mBlock;
    if (!reader.read(&probe,false) || probe.content.size()!=1u) return fail("typed-node budget retry failed");
    std::istringstream stream;
    dxfReaderAscii nodeReader(&stream); nodeReader.setVersion("AC1032",true);
    auto budget=std::make_shared<DxfReadRetainedBudget>(1024u*1024u);
    nodeReader.setRetainedByteBudget(budget); nodeReader.setTypedVariantNodeBudget(0u);
    DRW_TableContentObject unmatched;
    const bool matched=dxfTableContent::Decoder(probe.raw.front(),nodeReader).decode(unmatched);
    if (matched
        || !nodeReader.typedVariantNodeBudgetExceeded() || unmatched.m_parseComplete
        || unmatched.m_dxfMetadata || budget->used()!=0u)
        return fail("post-capture typed-node failure leaked metadata or retained-byte ownership");
    std::cout<<"TABLECONTENT: "<<routes<<" rich projections; "<<fallbacks<<" fallback/retry pairs; "
             <<refusals<<" atomic export refusals/readbacks; "<<values<<" primitive value routes; byte/node refusal/recovery\n";
    return routes==16u && fallbacks==288u && refusals==32u && values==20u;
}
Tags compoundSource(const std::string& version,bool aliases) {
    Tags result{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"},
        {0,"SECTION"},{2,"ENTITIES"},{0,"ACAD_TABLE"},{5,"4F2"},
        {102,"{ACAD_XDICTIONARY"},{360,"B1"},{102,"}"},{330,"1F"},
        {100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockReference"},{2,"*T1"},
        {10,"1"},{20,"2"},{30,"3"},{100,"AcDbTable"},{90,"2"},{91,"1"},{92,"1"},
        {0,"ENDSEC"},{0,"SECTION"},{2,"OBJECTS"},
        {0,"DICTIONARY"},{5,"C"},{330,"0"},{100,"AcDbDictionary"},{280,"0"},{281,"1"},
        {0,"DICTIONARY"},{5,"B1"},{330,"4F2"},{100,"AcDbDictionary"},{280,"1"},{281,"1"},
        {3,"ACAD_XREC_ROUNDTRIP"},{360,"B0"},
        {0,"XRECORD"},{5,"B0"},{102,"{ACAD_REACTORS"},{330,"B1"},{102,"}"},{330,"B1"},
        {100,"AcDbXrecord"},{280,"1"},{102,"ACAD_ROUNDTRIP_2008_TABLE_ENTITY"},
        {360,"A0"},{70,"2"},{90,"1"},{10,"0"},{20,"0"},{30,"0"},{90,"0"},{90,"19"},{361,"A1"}};
    const auto content=source(version,aliases?"aCdBtAbLeCoNtEnT":"TABLECONTENT");
    const auto begin=slot(content,0,aliases?"aCdBtAbLeCoNtEnT":"TABLECONTENT");
    result.insert(result.end(),content.begin()+static_cast<std::ptrdiff_t>(begin),content.end()-2);
    result.insert(result.end(),{{0,aliases?"aCdBtAbLeGeOmEtRy":"TABLEGEOMETRY"},{5,"A1"},{330,"B0"},
        {100,"AcDbTableGeometry"},{90,"1"},{91,"1"},{92,"1"},{93,"5"},{40,"22.5"},
        {41,"8.5"},{330,"A700"},{94,"1"},{10,"1"},{20,"2"},{30,"3"},
        {11,"4"},{21,"5"},{31,"6"},{43,"7"},{44,"8"},{45,"9"},{46,"10"},{95,"-7"},
        {0,"ENDSEC"},{0,"EOF"}});
    return result;
}

bool compoundTests(const std::filesystem::path& dir) {
    using State=dx_iface::TableSourceState;
    const auto path=dir/"compound.dxf",output=dir/"refused.dxf";
    const auto fail=[](const std::string& text) {std::cerr<<text<<'\n'; return false;};
    const std::vector<std::function<void(Tags&)>> faults{
        [](Tags& t){t[slot(t,360,"B1")].value="BAD";},
        [](Tags& t){t[slot(t,330,"4F2")].value="4F3";},
        [](Tags& t){t[slot(t,3,"ACAD_XREC_ROUNDTRIP")].value="UNKNOWN_KEY";},
        [](Tags& t){t[slot(t,360,"B0")].code=350;},
        [](Tags& t){t[slot(t,360,"B0")].value="A0";},
        [](Tags& t){t[slot(t,330,"B1")].value="4F2"; t[slot(t,330,"B1")].value="4F2";},
        [](Tags& t){t[slot(t,102,"ACAD_ROUNDTRIP_2008_TABLE_ENTITY")].value="UNKNOWN_TABLE_ENTITY";},
        [](Tags& t){t[slot(t,360,"A0")].code=340;},
        [](Tags& t){t[slot(t,361,"A1")].code=360;},
        [](Tags& t){t[slot(t,360,"A0")].value="A1"; t[slot(t,361,"A1")].value="A0";},
        [](Tags& t){t[slot(t,361,"A1")].value="A0";},
        [](Tags& t){t[slot(t,360,"A0")].value="0";},
        [](Tags& t){t[slot(t,361,"A1")].value="1000000A1";},
        [](Tags& t){t[slot(t,330,"B0")].value="B1";},
        [](Tags& t){t[slot(t,91,"1")].value="2";},
        [](Tags& t){t.insert(t.begin()+static_cast<std::ptrdiff_t>(slot(t,361,"A1")),{90,"123"});},
        [](Tags& t){const auto b=slot(t,0,"XRECORD"); const auto e=slot(t,0,"TABLECONTENT");
            Tags copy(t.begin()+static_cast<std::ptrdiff_t>(b),t.begin()+static_cast<std::ptrdiff_t>(e));
            t.insert(t.begin()+static_cast<std::ptrdiff_t>(e),copy.begin(),copy.end());},
        [](Tags& t){t[slot(t,5,"A1")].value="A0";},
        [](Tags& t){t[slot(t,5,"4F2")].value="1000004F2";},
        [](Tags& t){t[slot(t,330,"1F")].value="10000001F";},
        [](Tags& t){t[slot(t,330,"1F")].value="4F2";},
        [](Tags& t){t[slot(t,330,"1F")].value="A0";},
        [](Tags& t){t[slot(t,330,"1F")].value="0";},
        [](Tags& t){t[slot(t,309,"TABLECELL_END")].value="FUTURE_CELL_END";},
        [](Tags& t){const auto b=slot(t,0,"TABLECONTENT"); const auto e=slot(t,0,"TABLEGEOMETRY");
            t.erase(t.begin()+static_cast<std::ptrdiff_t>(b),t.begin()+static_cast<std::ptrdiff_t>(e));},
        [](Tags& t){const auto b=slot(t,0,"TABLEGEOMETRY");
            t.erase(t.begin()+static_cast<std::ptrdiff_t>(b),t.end()-2);},
        [](Tags& t){t.insert(t.end()-2,{{0,"DICTIONARY"},{5,"B2"},{330,"4F2"},
            {100,"AcDbDictionary"},{280,"1"},{281,"1"},{3,"ACAD_XREC_ROUNDTRIP"},{360,"B0"}});},
        [](Tags& t){t.erase(t.end()-2);}
    };
    unsigned routes=0u,negative=0u,refusals=0u,budgets=0u,extraRoutes=0u;
    std::size_t retained=0u,groups=0u,work=0u;
    for (const auto* version:{"AC1021","AC1024","AC1027","AC1032"})
    for (bool binary:{false,true}) for (bool aliases:{false,true}) {
        const Tags valid=compoundSource(version,aliases);
        if (!write(path,valid,binary)) return fail("could not author compound");
        Probe reader; dx_data data;
        if (!reader.fileImport(path.string(),&data,false)) return fail("valid compound import failed");
        const auto report=dx_iface::tableSourceReport(data);
        if (report.state!=State::StructurallyComplete || report.outputEligible || report.compounds!=1u
            || report.tables!=1u || report.contents!=1u || report.geometries!=1u || report.records!=10u
            || reader.tableSources!=1u || reader.tableCallbacks!=1u || data.tables.size()!=1u)
            return fail("compound correlation failed: state="+std::to_string(static_cast<int>(report.state))
                +" records="+std::to_string(report.records));
        retained=report.retainedBytes; groups=report.groups; work=report.examinedSlots;
        const auto& raw=DxIfaceTableSourceTestAccess::rawTable(data);
        const auto& content=DxIfaceTableSourceTestAccess::content(data);
        const auto& wrapper=DxIfaceTableSourceTestAccess::wrapper(data);
        const auto& geometry=DxIfaceTableSourceTestAccess::geometry(data);
        if (raw.name!="ACAD_TABLE" || raw.groups.size()!=16u || content.m_content.m_name!="Name-é"
            || !content.m_dxfMetadata || content.m_dxfMetadata->m_rows[0].m_cells[0].m_geometry.size()!=2u
            || wrapper.m_dataEntries.size()!=8u || wrapper.m_dataEntries[6].i_val()!=19
            || geometry.m_cells[0].m_contents[0].m_topLeft.z!=3)
            return fail("compound retained payload changed");
        ++routes;
        for (bool strict:{false,true}) {
            {std::ofstream sentinel(output,std::ios::binary); sentinel<<"keep destination";}
            dx_iface exporter;
            if (exporter.fileExport(output.string(),data.sourceVersion,binary,&data,false,strict))
                return fail("source correlation enabled unproved output");
            std::ifstream saved(output,std::ios::binary);
            if (std::string(std::istreambuf_iterator<char>(saved),{})!="keep destination")
                return fail("refusal replaced destination");
            dx_data clean; DRW_Line line; line.basePoint={1,2,3}; line.secPoint={4,5,6};
            reader.cData=&clean; reader.currentBlock=clean.mBlock; reader.addLine(line);
            if (!exporter.fileExport(output.string(),data.sourceVersion,binary,&clean,false,strict))
                return fail("same exporter retry failed");
            dx_iface verifier; dx_data restored;
            if (!verifier.fileImport(output.string(),&restored,false) || restored.mBlock->ent.size()!=1u)
                return fail("corrected output readback failed");
            const auto* parsed=dynamic_cast<const DRW_Line*>(restored.mBlock->ent.front());
            if (!parsed || parsed->basePoint.x!=1 || parsed->basePoint.y!=2 || parsed->basePoint.z!=3
                || parsed->secPoint.x!=4 || parsed->secPoint.y!=5 || parsed->secPoint.z!=6)
                return fail("retry LINE changed");
            ++refusals;
        }
        // Wrong-family and duplicate-record controls use canonical type names;
        // aliases are already independently covered by the positive routes.
        for (const auto& fault:faults) {
            Tags bad=compoundSource(version,false); fault(bad);
            if (!write(path,bad,binary)) return fail("could not author fault");
            dx_data rejected;
            (void)reader.fileImport(path.string(),&rejected,false);
            if (dx_iface::tableSourceReport(rejected).state==State::StructurallyComplete)
                return fail("malformed graph acquired structural readiness fault="+std::to_string(negative%faults.size()));
            if (!write(path,valid,binary)) return false;
            dx_data retry;
            if (!reader.fileImport(path.string(),&retry,false)
                || dx_iface::tableSourceReport(retry).state!=State::StructurallyComplete)
                return fail("same importer fresh-state retry failed");
            ++negative;
        }
        for (unsigned limit=0;limit!=5u;++limit) {
            dx_data limited;
            DxIfaceTableSourceTestAccess::limits(limited,limit==0u?9u:100000u,
                limit==1u?groups-1u:4000000u,limit==2u?retained-1u:64u*1024u*1024u,
                limit==3u?work-1u:4000000u);
            if (limit==4u) DxIfaceTableSourceTestAccess::limits(limited,10u,groups,retained,work);
            if (!reader.fileImport(path.string(),&limited,false)) return fail("capture budget made tolerant import fail");
            const auto state=dx_iface::tableSourceReport(limited).state;
            if (state!=(limit==4u?State::StructurallyComplete:limit==3u?State::WorkLimit:State::CaptureFailed))
                return fail("capture boundary mismatch limit="+std::to_string(limit)+" state="+std::to_string(static_cast<int>(state)));
            ++budgets;
        }
        // The same key belongs to other entity families, too. Such a
        // dictionary is a retained candidate, not another TABLE wrapper.
        Tags other=valid;
        other.insert(other.end()-2,{{0,"DICTIONARY"},{5,"B2"},{330,"33"},
            {100,"AcDbDictionary"},{280,"1"},{281,"1"},{3,"ACAD_XREC_ROUNDTRIP"},{360,"B3"},
            {0,"XRECORD"},{5,"B3"},{330,"B2"},{100,"AcDbXrecord"},{280,"1"},
            {102,"OTHER_ENTITY_ROUNDTRIP"},{70,"4"}});
        if (!write(path,other,binary)) return false;
        dx_data otherData;
        if (!reader.fileImport(path.string(),&otherData,false)
            || dx_iface::tableSourceReport(otherData).state!=State::StructurallyComplete
            || dx_iface::tableSourceReport(otherData).dictionaryCandidates!=2u)
            return fail("unrelated roundtrip dictionary confused table selection");
        ++extraRoutes;
        // OBJECTS before ENTITIES must correlate after the actual import,
        // rather than rely on whether the parent callback already happened.
        const auto entities=slot(valid,2,"ENTITIES")-1u, objects=slot(valid,2,"OBJECTS")-1u;
        Tags reordered(valid.begin(),valid.begin()+static_cast<std::ptrdiff_t>(entities));
        reordered.insert(reordered.end(),valid.begin()+static_cast<std::ptrdiff_t>(objects),valid.end()-1);
        reordered.insert(reordered.end(),valid.begin()+static_cast<std::ptrdiff_t>(entities),valid.begin()+static_cast<std::ptrdiff_t>(objects));
        reordered.push_back({0,"EOF"});
        if (!write(path,reordered,binary)) return false;
        dx_data reorderedData;
        if (!reader.fileImport(path.string(),&reorderedData,false)
            || dx_iface::tableSourceReport(reorderedData).state!=State::StructurallyComplete)
            return fail("source capture depended on section callback order");
        ++extraRoutes;
        Tags multiple=valid;
        Tags secondTable(valid.begin()+static_cast<std::ptrdiff_t>(entities+2u),valid.begin()+static_cast<std::ptrdiff_t>(objects-1u));
        Tags secondObjects(valid.begin()+static_cast<std::ptrdiff_t>(slot(valid,5,"B1")-1u),valid.end()-2);
        for (auto* tags:{&secondTable,&secondObjects}) for (auto& tag:*tags) {
            if (tag.code!=5 && tag.code!=330 && tag.code!=360 && tag.code!=361) continue;
            for (const auto& mapping:std::array<std::pair<const char*,const char*>,5>{{
                    {"4F2","4F3"},{"B1","B4"},{"B0","B5"},{"A0","A2"},{"A1","A3"}}})
                if (tag.value==mapping.first) {tag.value=mapping.second; break;}
        }
        multiple.insert(multiple.end()-2,secondObjects.begin(),secondObjects.end());
        multiple.insert(multiple.begin()+static_cast<std::ptrdiff_t>(objects-1u),secondTable.begin(),secondTable.end());
        if (!write(path,multiple,binary)) return false;
        dx_data multipleData;
        if (!reader.fileImport(path.string(),&multipleData,false)
            || dx_iface::tableSourceReport(multipleData).state!=State::StructurallyComplete
            || dx_iface::tableSourceReport(multipleData).compounds!=2u)
            return fail("multiple table compounds failed to correlate");
        ++extraRoutes;
        Tags attached=valid;
        attached.insert(attached.begin()+static_cast<std::ptrdiff_t>(objects-1u),{
            {0,"ATTRIB"},{5,"D0"},{330,"4F2"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbText"},
            {10,"0"},{20,"0"},{30,"0"},{40,"1"},{1,"attribute"},{100,"AcDbAttribute"},{2,"tag"},{70,"0"},
            {0,"SEQEND"},{5,"D1"},{330,"4F2"},{100,"AcDbEntity"},{8,"0"}});
        if (!write(path,attached,binary)) return false;
        dx_data attachedData;
        if (!reader.fileImport(path.string(),&attachedData,false)
            || dx_iface::tableSourceReport(attachedData).state!=State::UnsupportedProfile
            || DxIfaceTableSourceTestAccess::rawTable(attachedData).groups.size()!=16u
            || DxIfaceTableSourceTestAccess::table(attachedData).attlist.size()!=1u
            || !DxIfaceTableSourceTestAccess::table(attachedData).seqEndEntity)
            return fail("parent raw capture claimed separate attribute/footer records");
        ++extraRoutes;
    }
    const auto valid=compoundSource("AC1032",false); if (!write(path,valid,false)) return false;
    Probe reader; dx_data accumulated;
    if (!reader.fileImport(path.string(),&accumulated,false) || !reader.fileImport(path.string(),&accumulated,false)
        || dx_iface::tableSourceReport(accumulated).state!=State::ContextMismatch)
        return fail("appended generations were qualified");
    dx_data fresh; if (!reader.fileImport(path.string(),&fresh,false)) return false;
    ++fresh.importGeneration;
    if (dx_iface::tableSourceReport(fresh).state!=State::ContextMismatch) return fail("edited generation was qualified");
    dx_data throwing; reader.throwOnTable=true;
    bool threw=false; try {(void)reader.fileImport(path.string(),&throwing,false);} catch (const std::runtime_error&) {threw=true;}
    if (!threw || dx_iface::tableSourceReport(throwing).state!=State::ImportFailed) return fail("callback unwind left open receipt");
    reader.throwOnTable=false; dx_data recovered;
    if (!reader.fileImport(path.string(),&recovered,false)
        || dx_iface::tableSourceReport(recovered).state!=State::StructurallyComplete) return fail("exception recovery failed");
    Probe legacy; legacy.optIn=false; dx_data legacyData;
    if (!legacy.fileImport(path.string(),&legacyData,false) || legacy.tableSources!=0u || legacy.tableCallbacks!=1u
        || legacyData.tables.size()!=1u || dx_iface::tableSourceReport(legacyData).state==State::StructurallyComplete)
        return fail("non-opt-in consumer path changed");
    Probe mutation; mutation.mutateTable=true; dx_data mutated;
    if (!mutation.fileImport(path.string(),&mutated,false)
        || DxIfaceTableSourceTestAccess::table(mutated).name!="*T1"
        || mutated.tables.front().name!="mutated-after-source-capture") return fail("source baseline followed typed callback mutation");
    mutation.content.front().m_content.m_name="edited consumer copy";
    mutation.content.front().m_content.m_rows[0].m_cells[0].m_contents[0].m_text="edited nested copy";
    mutation.content.front().m_dxfMetadata.reset();
    mutated.xRecords.front().m_dataEntries[6]=DRW_Variant(90,25);
    if (DxIfaceTableSourceTestAccess::content(mutated).m_content.m_name!="Name-é"
        || DxIfaceTableSourceTestAccess::content(mutated).m_content.m_rows[0].m_cells[0].m_contents[0].m_text!="TABLEFORMAT_END \n text-é"
        || !DxIfaceTableSourceTestAccess::content(mutated).m_dxfMetadata
        || DxIfaceTableSourceTestAccess::wrapper(mutated).m_dataEntries[6].i_val()!=19)
        return fail("owned baseline followed editable consumer payload");
    // No live-source/output equivalence is claimed by this structural report.
    if (dx_iface::tableSourceReport(mutated).state!=State::StructurallyComplete
        || dx_iface::tableSourceReport(mutated).outputEligible)
        return fail("frozen structural report was confused with live/output admission");
    dx_data direct; reader.cData=&direct; reader.currentBlock=direct.mBlock;
    reader.addDxfTableSource(DxIfaceTableSourceTestAccess::table(recovered),DxIfaceTableSourceTestAccess::rawTable(recovered));
    if (dx_iface::tableSourceReport(direct).state!=State::ContextMismatch) return fail("direct source callback fabricated import");
    Probe padded; padded.paddedRaw=true; dx_data byteLimited;
    DxIfaceTableSourceTestAccess::limits(byteLimited,100000u,4000000u,64u*1024u);
    if (!padded.fileImport(path.string(),&byteLimited,false)
        || dx_iface::tableSourceReport(byteLimited).state!=State::CaptureFailed
        || dx_iface::tableSourceReport(byteLimited).tables!=0u)
        return fail("adapter copied uncharged bytes beyond embedded NUL");
    dx_data scanLimited; DxIfaceTableSourceTestAccess::limits(scanLimited,100000u,4000000u,64u*1024u*1024u,8u);
    if (!reader.fileImport(path.string(),&scanLimited,false)
        || dx_iface::tableSourceReport(scanLimited).state!=State::CaptureFailed)
        return fail("candidate selection work was not cumulatively bounded");
    for (bool binary:{false,true}) {
        Tags parentOnly(valid.begin(),valid.begin()+static_cast<std::ptrdiff_t>(slot(valid,2,"OBJECTS")-1u));
        parentOnly.insert(parentOnly.end()-1,{300,std::string(100000u,'r')});
        parentOnly.push_back({0,"EOF"});
        if (!write(path,parentOnly,binary)) return false;
        Probe sourceProbe; sourceProbe.forceOptIn=true; dx_data limited;
        sourceProbe.cData=&limited; sourceProbe.currentBlock=limited.mBlock;
        // Leave room for the existing current-token + worst-case semantic
        // output reservation. Only the additional retained raw parent should
        // cross this boundary; ordinary token acquisition is not free.
        const std::size_t byteBudget=650000u;
        dxfRW publicReader(path.string().c_str()); publicReader.setDxfReadRetainedByteBudget(byteBudget);
        if (publicReader.read(&sourceProbe,false) || sourceProbe.tableSources!=0u || sourceProbe.tableCallbacks!=0u)
            return fail("raw TABLE budget published a partial parent callback");
        publicReader.setDxfReadRetainedByteBudget(2u*1024u*1024u); dx_data corrected;
        sourceProbe.cData=&corrected; sourceProbe.currentBlock=corrected.mBlock;
        if (!publicReader.read(&sourceProbe,false) || sourceProbe.tableSources!=1u || sourceProbe.tableCallbacks!=1u
            || sourceProbe.largestTableSourceString!=100000u)
            return fail("same public reader raw-parent budget retry failed");
        Probe noCopy; noCopy.optIn=false; dx_data typedOnly;
        noCopy.cData=&typedOnly; noCopy.currentBlock=typedOnly.mBlock;
        publicReader.setDxfReadRetainedByteBudget(byteBudget);
        const bool typedRead=publicReader.read(&noCopy,false);
        if (!typedRead || noCopy.tableSources!=0u || noCopy.tableCallbacks!=1u)
            return fail("non-opt-in callback boundary failed: binary="+std::to_string(binary)
                +" error="+std::to_string(publicReader.getError())+" source="+std::to_string(noCopy.tableSources)
                +" typed="+std::to_string(noCopy.tableCallbacks));
    }
    std::cout<<"TABLE compound: "<<routes<<" source routes; "<<negative<<" graph refusal/fresh-state retries; "
        <<refusals<<" atomic export refusals/LINE readbacks; "<<budgets<<" budget boundaries; "<<extraRoutes
        <<" candidate/order/multiple/attribute routes; generation/unwind/2D/frozen-source/NUL/selection controls\n";
    return routes==16u && negative==448u && refusals==32u && budgets==80u && extraRoutes==64u;
}

DRW_TableContentDxfMetadata& mutableMetadata(DRW_TableContentObject& o) {
    // Only called on freshly allocated mutable test companions, never on
    // the adapter's private const baseline or the library's original object.
    return *const_cast<DRW_TableContentDxfMetadata*>(o.m_dxfMetadata.get());
}

bool contentProofTests(const std::filesystem::path& dir) {
    using Kind=dxfTableContent::ReferenceKind;
    using State=dx_iface::TableSourceState;
    const auto path=dir/"proof.dxf";
    const auto fail=[](const std::string& text) {std::cerr<<text<<'\n';return false;};
    std::vector<std::function<void(DRW_TableContentObject&)>> faults;
#define OBJECT_INT(member) faults.emplace_back([](DRW_TableContentObject& o){++o.member;})
#define CORE_INT(member) faults.emplace_back([](DRW_TableContentObject& o){++o.m_content.member;})
#define CELL_INT(member) faults.emplace_back([](DRW_TableContentObject& o){++o.m_content.m_rows[0].m_cells[0].member;})
#define CELL_BOOL(member) faults.emplace_back([](DRW_TableContentObject& o){auto& v=o.m_content.m_rows[0].m_cells[0].member;v=!v;})
    OBJECT_INT(flags); OBJECT_INT(xDictHandle);
    CORE_INT(m_tableStyleHandle); CORE_INT(m_overrideStyleType);
    for (const auto& edit:std::vector<std::function<void(DRW_TableContentObject&)>>{
        [](auto& o){o.m_parseComplete=false;},[](auto& o){o.m_semanticContentComplete=true;},
        [](auto& o){o.name="injected";},[](auto& o){o.reactorHandles.push_back(1u);},
        [](auto& o){o.m_content.m_name+="changed";},[](auto& o){o.m_content.m_description+="changed";},
        [](auto& o){o.m_content.m_fieldHandles.push_back(5u);},[](auto& o){o.m_content.m_subrecordRanges.emplace_back();},
        [](auto& o){o.m_content.m_columns[0].m_name+="changed";},[](auto& o){++o.m_content.m_columns[0].m_width;},
        [](auto& o){++o.m_content.m_columns[0].m_styleId;},[](auto& o){++o.m_content.m_columns[0].m_overrideStyleType;},
        [](auto& o){++o.m_content.m_rows[0].m_height;},[](auto& o){++o.m_content.m_rows[0].m_styleId;},
        [](auto& o){++o.m_content.m_rows[0].m_overrideStyleType;},[](auto& o){++o.m_content.m_mergedRanges[0].m_rightColumn;},
        [](auto& o){o.m_content.m_rows[0].m_cells[0].m_toolTip+="changed";},
        [](auto& o){o.m_content.m_rows[0].m_cells[0].m_textStyleName+="changed";},
        [](auto& o){o.m_content.m_rows[0].m_cells[0].m_geometryTopLeft.x=-0.0;},
        [](auto& o){o.m_content.m_rows[0].m_cells[0].m_geometryCenter.z=1;},
        [](auto& o){++o.m_content.m_rows[0].m_cells[0].m_borders[5].m_lineWeight;},
        [](auto& o){o.m_content.m_rows[0].m_cells[0].m_borders[5].m_visible=false;},
        [](auto& o){++o.m_content.m_rows[0].m_cells[0].m_borders[5].m_colorRgb;},
        [](auto& o){++o.m_content.m_rows[0].m_cells[0].m_borders[5].m_colorMethod;},
        [](auto& o){++o.m_content.m_rows[0].m_cells[0].m_attributes[0].m_attdefHandle;},
        [](auto& o){++o.m_content.m_rows[0].m_cells[0].m_attributes[0].m_index;},
        [](auto& o){++o.m_content.m_rows[0].m_cells[0].m_attributes[0].m_contentIndex;},
        [](auto& o){o.m_content.m_rows[0].m_cells[0].m_attributes[0].m_text+="changed";},
        [](auto& o){++o.m_content.m_rows[0].m_cells[0].m_contents[0].m_type;},
        [](auto& o){++o.m_content.m_rows[0].m_cells[0].m_contents[0].m_handle;},
        [](auto& o){o.m_content.m_rows[0].m_cells[0].m_contents[0].m_text+="changed";},
        [](auto& o){o.m_dxfMetadata.reset();}}) faults.push_back(edit);
    CELL_INT(m_flags);CELL_INT(m_type);CELL_INT(m_edgeFlags);CELL_BOOL(m_isMerged);CELL_BOOL(m_autoFit);
    CELL_INT(m_mergedWidth);CELL_INT(m_mergedHeight);CELL_INT(m_rotation);CELL_INT(m_styleId);CELL_INT(m_overrideStyleType);
    CELL_INT(m_virtualEdgeFlags);CELL_INT(m_overrideFlags);CELL_INT(m_valueHandle);CELL_INT(m_textStyleHandle);
    CELL_INT(m_textStyleOverrideHandle);CELL_INT(m_blockHandle);CELL_BOOL(m_hasLinkedData);CELL_INT(m_dataLinkHandle);
    CELL_INT(m_linkedDataRowCount);CELL_INT(m_linkedDataColumnCount);CELL_INT(m_linkedDataUnknown);CELL_INT(m_blockScale);
    CELL_INT(m_width);CELL_INT(m_height);CELL_INT(m_geometryFlags);CELL_INT(m_geometryHandle);CELL_INT(m_geometryRecordFlags);
    CELL_INT(m_contentWidth);CELL_INT(m_contentHeight);CELL_INT(m_geometryWidth);CELL_INT(m_geometryHeight);
    CELL_INT(m_textColorRgb);CELL_INT(m_textColorMethod);CELL_INT(m_fillColorRgb);CELL_INT(m_fillColorMethod);
    CELL_BOOL(m_backgroundEnabled);CELL_INT(m_textAlignment);CELL_INT(m_textHeight);
#undef OBJECT_INT
#undef CORE_INT
#undef CELL_INT
#undef CELL_BOOL
    using Meta=DRW_TableContentDxfMetadata;
    const std::vector<std::function<void(Meta&)>> metadataFaults{
        [](auto& m){++m.m_linkedDataUnknown92;},[](auto& m){++m.m_columns[0].m_customData;},
        [](auto& m){m.m_columns[0].m_customItems[0].m_name+="changed";},
        [](auto& m){++m.m_rows[0].m_scope.m_customData;},[](auto& m){++m.m_rows[0].m_cells[0].m_scope.m_customData;},
        [](auto& m){++m.m_rows[0].m_cells[0].m_geometry[1].m_topLeft.z;},
        [](auto& m){++m.m_rows[0].m_cells[0].m_geometry[1].m_center.y;},
        [](auto& m){++m.m_rows[0].m_cells[0].m_geometry[1].m_contentWidth;},
        [](auto& m){++m.m_rows[0].m_cells[0].m_geometry[1].m_contentHeight;},
        [](auto& m){++m.m_rows[0].m_cells[0].m_geometry[1].m_width;},
        [](auto& m){++m.m_rows[0].m_cells[0].m_geometry[1].m_height;},
        [](auto& m){++m.m_rows[0].m_cells[0].m_geometry[1].m_unknown;},
        [](auto& m){m.m_rows[0].m_cells[0].m_contentFormats[0].reset();},
        [](auto& m){m.m_rows[0].m_cells[0].m_contentFormats[1]=std::make_shared<DRW_TableStyleContentFormat>();},
        [](auto& m){++m.m_tableStyle.m_type;},[](auto& m){m.m_rows[0].m_scope.m_customItems.clear();}
    };
    for (const auto& edit:metadataFaults) faults.emplace_back([edit](auto& o){edit(mutableMetadata(o));});
    const std::vector<std::function<void(DRW_CadValue&)>> valueFaults{
        [](auto& v){++v.m_formatFlags;},[](auto& v){++v.m_dataType;},[](auto& v){++v.m_dataSize;},
        [](auto& v){++v.m_unitType;},[](auto& v){v.m_value=DRW_Variant(91,1);},
        [](auto& v){v.m_formatString+="changed";},[](auto& v){v.m_valueString+="changed";},
        [](auto& v){++v.m_handle;},[](auto& v){v.m_rawData.push_back(7u);}
    };
    for (const auto& edit:valueFaults) {
        faults.emplace_back([edit](auto& o){edit(o.m_content.m_rows[0].m_cells[0].m_contents[0].m_value);});
        faults.emplace_back([edit](auto& o){edit(mutableMetadata(o).m_columns[0].m_customItems[0].m_value);});
    }
    const std::vector<std::function<void(DRW_TableStyleCellStyle&)>> styleFaults{
        [](auto& s){++s.m_type;},[](auto& s){s.m_hasData=!s.m_hasData;},
        [](auto& s){++s.m_propertyOverrideFlags;},[](auto& s){++s.m_mergeFlags;},
        [](auto& s){++s.m_backgroundColor;},[](auto& s){s.m_backgroundColorCmc.m_name="injected";},
        [](auto& s){++s.m_contentLayout;},[](auto& s){++s.m_marginOverrideFlags;},
        [](auto& s){++s.m_verticalMargin;},[](auto& s){++s.m_horizontalMargin;},
        [](auto& s){++s.m_bottomMargin;},[](auto& s){++s.m_rightMargin;},
        [](auto& s){++s.m_marginHorizontalSpacing;},[](auto& s){++s.m_marginVerticalSpacing;},
        [](auto& s){++s.m_id;},[](auto& s){++s.m_styleClass;},[](auto& s){s.m_name="injected";},
        [](auto& s){++s.m_borders[0].m_edgeFlags;},[](auto& s){++s.m_borders[0].m_propertyOverrideFlags;},
        [](auto& s){++s.m_borders[0].m_borderType;},[](auto& s){++s.m_borders[0].m_color;},
        [](auto& s){++s.m_borders[0].m_lineWeight;},[](auto& s){++s.m_borders[0].m_lineTypeHandle;},
        [](auto& s){s.m_borders[0].m_visible=0;},[](auto& s){++s.m_borders[0].m_doubleLineSpacing;},
        [](auto& s){s.m_borders[0].m_colorCmc.m_bookName="injected";}
    };
    for (const auto& edit:styleFaults) faults.emplace_back([edit](auto& o){edit(mutableMetadata(o).m_columns[0].m_style);});
    const std::vector<std::function<void(DRW_TableStyleContentFormat&)>> formatFaults{
        [](auto& f){++f.m_propertyOverrideFlags;},[](auto& f){++f.m_propertyFlags;},
        [](auto& f){++f.m_valueDataType;},[](auto& f){++f.m_valueUnitType;},
        [](auto& f){f.m_valueFormatString+="changed";},[](auto& f){++f.m_rotation;},
        [](auto& f){++f.m_blockScale;},[](auto& f){++f.m_cellAlignment;},[](auto& f){++f.m_contentColor;},
        [](auto& f){++f.m_textStyleHandle;},[](auto& f){++f.m_textHeight;},
        [](auto& f){++f.m_contentColorCmc.m_rgb;},[](auto& f){++f.m_contentColorCmc.m_method;},
        [](auto& f){++f.m_contentColorCmc.m_flags;},[](auto& f){++f.m_contentColorCmc.m_reservedIndex;},
        [](auto& f){f.m_contentColorCmc.m_name="injected";},[](auto& f){f.m_contentColorCmc.m_bookName="injected";}
    };
    for (const auto& edit:formatFaults) {
        faults.emplace_back([edit](auto& o){edit(mutableMetadata(o).m_columns[0].m_style.m_contentFormat);});
        faults.emplace_back([edit](auto& o){edit(*const_cast<DRW_TableStyleContentFormat*>(
            mutableMetadata(o).m_rows[0].m_cells[0].m_contentFormats[0].get()));});
    }
    unsigned routes=0u,mismatches=0u,wireFailures=0u,valueRoleRoutes=0u,primitiveRoutes=0u;
    for (const char* version:{"AC1021","AC1024","AC1027","AC1032"}) for (bool binary:{false,true}) {
        const auto valid=compoundSource(version,false); if (!write(path,valid,binary)) return false;
        Probe probe; dx_data baseline;
        if (!probe.fileImport(path.string(),&baseline,false)) return fail("proof source import failed");
        const auto report=dx_iface::tableSourceReport(baseline);
        if (report.state!=State::StructurallyComplete || report.contentRawMatches!=1u
            || report.nonnullUnknownContentReferences!=1u || report.outputEligible) return fail("canonical proof absent");
        const auto& refs=DxIfaceTableSourceTestAccess::references(baseline);
        std::array<unsigned,static_cast<std::size_t>(Kind::Count)> expected{{1,1,3,2,1,1,1,2,1,0,1,3}};
        if (refs.size()!=17u) return fail("reference roles missing");
        std::size_t previous=0u;
        for (std::size_t i=0;i<refs.size();++i) {
            const auto& r=refs[i];
            const auto& group=probe.raw[0].groups[r.groupIndex];
            if ((i && r.groupIndex<=previous) || group.code()!=r.code) return fail("role span order changed");
            previous=r.groupIndex;
            if (r.kind==Kind::LocalStyleId && (group.type()!=DRW_Variant::INTEGER
                    || group.i_val()!=static_cast<int>(r.value))) return fail("local style ID treated as handle");
        }
        for (std::size_t k=0;k<expected.size();++k)
            if (report.contentReferenceSlots[k]!=expected[k]) return fail("role census differs");
        std::size_t rawHandleSlots=0u;
        for (const auto& group:probe.raw[0].groups) if (isDxfHandleCode(group.code())) ++rawHandleSlots;
        if (rawHandleSlots+expected[static_cast<std::size_t>(Kind::LocalStyleId)]!=refs.size())
            return fail("admitted source has an unclassified handle slot");
        ++routes;
        for (const auto& edit:faults) {
            probe.alterContent=edit; dx_data rejected;
            if (!probe.fileImport(path.string(),&rejected,false)) return fail("typed mutation changed read outcome");
            const auto result=dx_iface::tableSourceReport(rejected);
            if (result.state!=State::RawTypedMismatch || result.contentRawMatches!=0u || result.outputEligible)
                return fail("typed mutation acquired source proof index="+std::to_string(mismatches%faults.size()));
            probe.alterContent={}; dx_data restored;
            if (!probe.fileImport(path.string(),&restored,false)
                || dx_iface::tableSourceReport(restored).state!=State::StructurallyComplete)
                return fail("canonical-source same-facade recovery failed");
            ++mismatches;
        }
        const std::vector<std::function<void(DRW_RawDxfObject&)>> wireFaults{
            [](auto& raw){raw.hasRawValues=!raw.hasRawValues;},
            [](auto& raw){raw.groups[3]=DRW_Variant(1,std::string("different name"));},
            [](auto& raw){raw.rawValues[3]="different physical spelling";},
            [](auto& raw){raw.rawValues[5]="1\n0\nEOF";},
            [](auto& raw){raw.groups.pop_back();raw.rawValues.pop_back();}
        };
        for (const auto& edit:wireFaults) {
            probe.alterContentRaw=edit; dx_data rejected;
            if (!probe.fileImport(path.string(),&rejected,false)
                || dx_iface::tableSourceReport(rejected).state!=State::RawTypedMismatch)
                return fail("raw/value or encoding mismatch acquired canonical proof");
            probe.alterContentRaw={}; ++wireFailures;
        }
        dx_data limited; DxIfaceTableSourceTestAccess::proofLimit(limited,report.contentProofSlots-1u);
        if (!probe.fileImport(path.string(),&limited,false)
            || dx_iface::tableSourceReport(limited).state!=State::CaptureFailed) return fail("proof-work cap bypassed");
        dx_data exact; DxIfaceTableSourceTestAccess::proofLimit(exact,report.contentProofSlots);
        if (!probe.fileImport(path.string(),&exact,false)
            || dx_iface::tableSourceReport(exact).state!=State::StructurallyComplete) return fail("exact proof-work boundary failed");
        for (unsigned mode=0u;mode<4u;++mode) {
            Tags variant=valid;
            if (mode==0u) {
                const auto start=slot(variant,0,"TABLECONTENT")+1u;
                for (const auto& r:refs) {
                    if (r.kind==Kind::Self || r.kind==Kind::Owner) continue;
                    if (variant[start+r.groupIndex].code!=r.code) return fail("reference span was not exact source slot");
                    variant[start+r.groupIndex].value="0";
                }
            } else {
                const auto valueStart=slot(variant,301,"DATAMAP_VALUE");
                variant[valueStart+2u].value="64";
                variant[valueStart+3u].code=330; variant[valueStart+3u].value=mode==1u?"F001":"0";
                if (mode==3u) {
                    variant[valueStart+1u].value="1";
                    variant.erase(variant.begin()+static_cast<std::ptrdiff_t>(valueStart+3u));
                }
            }
            if (!write(path,variant,binary)) return false;
            dx_data valueData;
            if (!probe.fileImport(path.string(),&valueData,false)) return fail("value role control import failed");
            const auto observed=dx_iface::tableSourceReport(valueData);
            if (observed.state!=State::StructurallyComplete
                || observed.contentReferenceSlots[static_cast<std::size_t>(Kind::ObjectId)]!=(mode==1u||mode==2u?1u:0u)
                || (mode==0u && observed.nonnullUnknownContentReferences!=0u))
                return fail("null/suppressed/ObjectID/local-ID role differs");
            ++valueRoleRoutes;
        }
        for (int type:{0,2,8,16,32}) {
            Tags primitive=valid;
            const auto valueSlot=slot(primitive,1,"TABLEFORMAT_END ^J text-é");
            primitive[valueSlot-2u].value="0"; primitive[valueSlot-1u].value=std::to_string(type);
            primitive.erase(primitive.begin()+static_cast<std::ptrdiff_t>(valueSlot));
            Tags payload;
            if (type==0) payload={{91,"-1234"}};
            else if (type==2) payload={{140,"-0.0"}};
            else if (type==8) payload={{92,"8"},{310,"010203AB04CD05EF"}};
            else if (type==16) payload={{92,"16"},{11,"1.25"},{21,"-2.5"}};
            else payload={{92,"24"},{11,"1.25"},{21,"-2.5"},{31,"3.75"}};
            primitive.insert(primitive.begin()+static_cast<std::ptrdiff_t>(valueSlot),payload.begin(),payload.end());
            if (!write(path,primitive,binary)) return false;
            dx_data primitiveData;
            if (!probe.fileImport(path.string(),&primitiveData,false)
                || dx_iface::tableSourceReport(primitiveData).state!=State::StructurallyComplete)
                return fail("primitive exact payload source proof failed type="+std::to_string(type));
            ++primitiveRoutes;
        }
    }
    if (!write(path,compoundSource("AC1032",false),false)) return false;
    Probe aliases; aliases.keepMutableAliases=true; dx_data frozen;
    if (!aliases.fileImport(path.string(),&frozen,false)) return false;
    aliases.mutableAlias->m_columns[0].m_customData=777;
    aliases.mutableAlias->m_rows[0].m_cells[0].m_geometry[0].m_topLeft.z=999;
    aliases.mutableFormatAlias->m_textStyleHandle=0xDEADu;
    const auto& retained=DxIfaceTableSourceTestAccess::content(frozen);
    if (retained.m_dxfMetadata.get()==aliases.mutableAlias.get()
        || retained.m_dxfMetadata->m_columns[0].m_customData!=-11
        || retained.m_dxfMetadata->m_rows[0].m_cells[0].m_geometry[0].m_topLeft.z!=3
        || retained.m_dxfMetadata->m_rows[0].m_cells[0].m_contentFormats[0]->m_textStyleHandle!=0xC101u
        || dx_iface::tableSourceReport(frozen).state!=State::StructurallyComplete)
        return fail("mutable metadata/format alias changed the frozen source proof");
    struct RejectingSink final : dxfTableContent::ReferenceSink {
        unsigned accepted=0u;
        bool add(const dxfTableContent::Reference&) override {if (accepted==2u)return false; ++accepted;return true;}
    } sink;
    std::istringstream stream; dxfReaderAscii decoder(&stream); decoder.setVersion("AC1032",true);
    auto budget=std::make_shared<DxfReadRetainedBudget>(64u*1024u*1024u); decoder.setRetainedByteBudget(budget);
    DRW_TableContentObject unpublished; bool decoded=false;
    {dxfTableContent::Decoder source(aliases.raw[0],decoder,&sink);decoded=source.decode(unpublished);}
    if (decoded || sink.accepted!=2u || unpublished.m_parseComplete || unpublished.m_dxfMetadata || budget->used()!=0u)
        return fail("failed reference observer published partial source or leaked text lease");
    std::cout<<"TABLECONTENT source proof: "<<routes<<" exact role routes; "<<faults.size()<<" typed fault kinds; "
        <<mismatches<<" mutation/restoration pairs; "<<wireFailures<<" raw/encoding refusals; "<<valueRoleRoutes
        <<" null/ObjectID/suppressed/local-ID routes; "<<primitiveRoutes
        <<" primitive payload routes; exact work boundaries; mutable-alias and observer rollback\n";
    return routes==8u && mismatches==faults.size()*routes && wireFailures==40u && valueRoleRoutes==32u && primitiveRoutes==40u;
}

Tags bindingSource(const std::string& version) {
    Tags tags=compoundSource(version,false);
    tags[slot(tags,330,"A700")].value="0"; // do not guess the unknown content geometry role
    const Tags tables{{0,"SECTION"},{2,"TABLES"},
        {0,"TABLE"},{2,"BLOCK_RECORD"},{5,"10"},{330,"0"},{100,"AcDbSymbolTable"},{70,"3"},
        {0,"BLOCK_RECORD"},{5,"1F"},{330,"10"},{100,"AcDbSymbolTableRecord"},{100,"AcDbBlockTableRecord"},{2,"*Model_Space"},{70,"0"},
        {0,"BLOCK_RECORD"},{5,"B100"},{330,"10"},{100,"AcDbSymbolTableRecord"},{100,"AcDbBlockTableRecord"},{2,"TABLE-BLOCK"},{70,"0"},
        {0,"BLOCK_RECORD"},{5,"B200"},{330,"10"},{100,"AcDbSymbolTableRecord"},{100,"AcDbBlockTableRecord"},{2,"*T1"},{70,"0"},{0,"ENDTAB"},
        {0,"TABLE"},{2,"STYLE"},{5,"20"},{330,"0"},{100,"AcDbSymbolTable"},{70,"1"},
        {0,"STYLE"},{5,"C101"},{330,"20"},{100,"AcDbSymbolTableRecord"},{100,"AcDbTextStyleTableRecord"},
        {2,"TableFont-é"},{70,"0"},{40,"0"},{41,"1"},{50,"0"},{71,"0"},{42,"1"},{3,"txt"},{4,""},{0,"ENDTAB"},
        {0,"TABLE"},{2,"LTYPE"},{5,"21"},{330,"0"},{100,"AcDbSymbolTable"},{70,"1"},
        {0,"LTYPE"},{5,"C102"},{330,"21"},{100,"AcDbSymbolTableRecord"},{100,"AcDbLinetypeTableRecord"},
        {2,"TABLE-LTYPE"},{70,"0"},{3,"binding line type"},{72,"65"},{73,"0"},{40,"0"},{0,"ENDTAB"},{0,"ENDSEC"}};
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(slot(tags,2,"ENTITIES")-1u),tables.begin(),tables.end());
    Tags blocks{{0,"SECTION"},{2,"BLOCKS"},{0,"BLOCK"},{5,"30"},{330,"B100"},
        {100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},{2,"TABLE-BLOCK"},{70,"0"},
        {10,"0"},{20,"0"},{30,"0"},{3,"TABLE-BLOCK"},{1,""}};
    for (const auto* handle:{"D001","D002"}) blocks.insert(blocks.end(),{
        {0,"ATTDEF"},{5,handle},{330,"B100"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbText"},
        {10,"0"},{20,"0"},{30,"0"},{40,"1"},{1,"default"},{100,"AcDbAttributeDefinition"},
        {2,handle},{3,"prompt"},{70,"0"}});
    blocks.insert(blocks.end(),{{0,"ENDBLK"},{5,"31"},{330,"B100"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"},
        {0,"BLOCK"},{5,"32"},{330,"B200"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},
        {2,"*T1"},{70,"0"},{10,"0"},{20,"0"},{30,"0"},{3,"*T1"},{1,""},
        {0,"ENDBLK"},{5,"33"},{330,"B200"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"},{0,"ENDSEC"}});
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(slot(tags,2,"ENTITIES")-1u),blocks.begin(),blocks.end());
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(slot(tags,5,"B1")-1u),{{3,"ACAD_TABLESTYLE"},{350,"C130"}});
    Tags objects{{0,"DICTIONARY"},{5,"C130"},{330,"C"},{100,"AcDbDictionary"},{280,"1"},{281,"1"},
        {3,"binding-table-style"},{360,"C100"},
        {0,"TABLESTYLE"},{5,"C100"},{102,"{ACAD_XDICTIONARY"},{360,"C110"},{102,"}"},{330,"C130"},
        {100,"AcDbTableStyle"},{280,"0"},{3,"binding style description"},{70,"0"},{71,"0"},
        {40,"1"},{41,"1"},{280,"0"},{281,"0"},
        {0,"DICTIONARY"},{5,"C110"},{330,"C100"},{100,"AcDbDictionary"},{280,"1"},{281,"1"},
        {3,"ACAD_ROUNDTRIP_2008_TABLESTYLE_CELLSTYLEMAP"},{360,"C120"},
        {0,"CELLSTYLEMAP"},{5,"C120"},{330,"C110"},{100,"AcDbCellStyleMap"},{90,"3"}};
    for (int id:{1,2,101}) {
        objects.push_back({300,"CELLSTYLE"});style(objects,1,false);
        objects.insert(objects.end(),{{1,"CELLSTYLE_BEGIN"},{90,std::to_string(id)},{91,"1"},
            {300,"local-"+std::to_string(id)},{309,"CELLSTYLE_END"}});
    }
    objects.insert(objects.end(),{{0,"FIELD"},{5,"F100"},{330,"A0"},{100,"AcDbField"},{1,"AcExpr"},
        {2,"1+1"},{90,"0"},{93,"0"},{97,"0"},
        {0,"DATALINK"},{5,"F001"},{330,"C"},{100,"AcDbDataLink"},{1,"EXCEL"},
        {300,"binding data link"},{301,"tooltip"},{302,"local.xlsx"},{90,"0"},{91,"0"},{92,"0"},{93,"0"},{94,"0"}});
    tags.insert(tags.end()-2,objects.begin(),objects.end());
    return tags;
}

bool bindingTests(const std::filesystem::path& dir) {
    using State=dx_iface::TableBindingState;
    const auto path=dir/"binding.dxf";
    const auto fail=[](const std::string& text){std::cerr<<text<<'\n';return false;};
    const auto eraseRecord=[](Tags& tags,const std::string& handle) {
        const auto begin=slot(tags,5,handle)-1u;
        auto end=begin+1u;while(end<tags.size() && tags[end].code!=0)++end;
        tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(begin),tags.begin()+static_cast<std::ptrdiff_t>(end));
    };
    const auto mapId=[](Tags& tags,int oldId,int newId) {
        for(std::size_t i=0;i+1u<tags.size();++i)
            if(tags[i].code==1 && tags[i].value=="CELLSTYLE_BEGIN" && tags[i+1u].value==std::to_string(oldId)) {
                tags[i+1u].value=std::to_string(newId);return;
            }
        throw std::runtime_error("missing map ID");
    };
    const auto newStyle=[](Tags& tags,const std::string& handle,const std::string& name) {
        const auto begin=slot(tags,0,"STYLE");auto end=begin+1u;while(tags[end].code!=0)++end;
        Tags copy(tags.begin()+static_cast<std::ptrdiff_t>(begin),tags.begin()+static_cast<std::ptrdiff_t>(end));
        for(auto& tag:copy){if(tag.code==5)tag.value=handle;if(tag.code==2)tag.value=name;}
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),copy.begin(),copy.end());
    };
    const std::vector<std::pair<std::function<void(Tags&)>,State>> faults{
        {[eraseRecord](auto& t){eraseRecord(t,"C101");},State::MissingTarget},
        {[eraseRecord](auto& t){eraseRecord(t,"C102");},State::MissingTarget},
        {[eraseRecord](auto& t){eraseRecord(t,"C100");},State::MissingTarget},
        {[eraseRecord](auto& t){eraseRecord(t,"C120");},State::MissingTarget},
        {[eraseRecord](auto& t){eraseRecord(t,"F100");},State::MissingTarget},
        {[eraseRecord](auto& t){eraseRecord(t,"F001");},State::MissingTarget},
        {[eraseRecord](auto& t){eraseRecord(t,"D002");},State::MissingTarget},
        {[](auto& t){t[slot(t,340,"C101")].value="C102";},State::WrongFamily},
        {[](auto& t){t[slot(t,340,"C102")].value="C101";},State::WrongFamily},
        {[](auto& t){t[slot(t,340,"C100")].value="C101";},State::WrongFamily},
        {[](auto& t){t[slot(t,340,"F100")].value="F001";},State::WrongFamily},
        {[](auto& t){t[slot(t,340,"B100")].value="C101";},State::WrongFamily},
        {[](auto& t){t[slot(t,330,"D002")].value="C101";},State::WrongFamily},
        {[](auto& t){t[slot(t,340,"F001")].value="F100";},State::WrongFamily},
        {[](auto& t){t[slot(t,330,"1F")].value="C101";},State::WrongFamily},
        {[](auto& t){t[slot(t,360,"C120")].value="C101";},State::WrongFamily},
        {[](auto& t){t[slot(t,3,"ACAD_ROUNDTRIP_2008_TABLESTYLE_CELLSTYLEMAP")].value="WRONG_KEY";},State::InvalidNamespace},
        {[](auto& t){t[slot(t,330,"C100")].value="B1";},State::InvalidNamespace},
        {[](auto& t){const auto i=slot(t,5,"C120");t[i+1u].value="B1";},State::InvalidNamespace},
        {[mapId](auto& t){mapId(t,101,102);},State::InvalidNamespace},
        {[mapId,newStyle](auto& t){mapId(t,101,102);newStyle(t,"65","object-not-local-id");},State::InvalidNamespace},
        {[newStyle](auto& t){newStyle(t,"C103","tablefont-é");},State::AmbiguousIdentity},
        {[](auto& t){const auto i=slot(t,5,"D002");t[i+1u].value="B200";},State::WrongOwner},
        {[](auto& t){t[slot(t,5,"C101")].value="C102";},State::SourceNotReady}, // duplicate physical self identity
        {[](auto& t){const auto begin=slot(t,1,"TABLECELL_BEGIN");t[begin+7u].value="C101";},State::UnknownRole},
        {[](auto& t){t[slot(t,340,"F100")].value="0";},State::MissingTarget},
        {[](auto& t){t[slot(t,340,"F001")].value="0";},State::MissingTarget},
        {[](auto& t){t[slot(t,340,"B100")].value="0";},State::MissingTarget},
        {[](auto& t){t[slot(t,340,"C100")].value="0";},State::MissingTarget},
        {[](auto& t){const auto i=slot(t,360,"C120")+1u;
            t.insert(t.begin()+static_cast<std::ptrdiff_t>(i),{{3,"ACAD_ROUNDTRIP_2008_TABLESTYLE_CELLSTYLEMAP"},{360,"C120"}});},State::InvalidNamespace}
    };
    unsigned routes=0u,negative=0u,boundaries=0u,variants=0u,refusals=0u;
    for (const char* version:{"AC1021","AC1024","AC1027","AC1032"}) for (bool binary:{false,true}) {
        const auto valid=bindingSource(version);
        if (!write(path,valid,binary)) return false;
        Probe probe;dx_data data;
        if (!probe.fileImport(path.string(),&data,false)) return fail("binding source import failed");
        const auto report=dx_iface::tableContentBindingReport(data);
        if (report.state!=State::Bound || report.outputEligible || report.localStyleUses!=3u)
            return fail("source bindings failed: state="+std::to_string(static_cast<int>(report.state))
                +" first="+std::to_string(report.firstHandle)+" resources="+std::to_string(report.resourceRecords));
        ++routes;
        for(std::size_t index=0;index<faults.size();++index) {
            auto bad=valid;faults[index].first(bad);
            if(!write(path,bad,binary))return false;
            dx_data rejected;(void)probe.fileImport(path.string(),&rejected,false);
            const auto failed=dx_iface::tableContentBindingReport(rejected);
            if(failed.state!=faults[index].second || failed.outputEligible)
                return fail("binding fault index="+std::to_string(index)+" state="+std::to_string(static_cast<int>(failed.state))
                    +" expected="+std::to_string(static_cast<int>(faults[index].second)));
            if(!write(path,valid,binary))return false;
            dx_data corrected;
            if(!probe.fileImport(path.string(),&corrected,false)
                ||dx_iface::tableContentBindingReport(corrected).state!=State::Bound) return fail("binding same-facade retry failed");
            ++negative;
        }
        for(unsigned limit=0;limit<8u;++limit) {
            dx_data limited;
            DxIfaceTableSourceTestAccess::resourceLimits(limited,
                limit==0u?report.resourceRecords-1u:limit==4u?report.resourceRecords:100000u,
                limit==1u?report.resourceItems-1u:limit==5u?report.resourceItems:4000000u,
                limit==2u?report.retainedBytes-1u:limit==6u?report.retainedBytes:64u*1024u*1024u,
                limit==3u?report.examinedSlots-1u:limit==7u?report.examinedSlots:4000000u);
            if(!probe.fileImport(path.string(),&limited,false))return fail("optional resource budget changed import outcome");
            const auto observed=dx_iface::tableContentBindingReport(limited);
            if(observed.state!=(limit<3u?State::CaptureFailed:limit==3u?State::WorkLimit:State::Bound))
                return fail("resource boundary failed index="+std::to_string(limit));
            ++boundaries;
        }
        for(unsigned mode=0;mode<5u;++mode) {
            auto varied=valid;
            if(mode==0u)varied[slot(varied,360,"C120")].code=350;
            if(mode==1u)newStyle(varied,"65","object-not-local-id");
            if(mode==2u) {
                const auto begin=slot(varied,301,"DATAMAP_VALUE");varied[begin+2u].value="64";
                varied[begin+3u]={330,"F001"};
            }
            if(mode==3u) {
                // Explicit null format/linetype references retain slots and inheritance.
                varied[slot(varied,340,"C101")].value="0";varied[slot(varied,340,"C102")].value="0";
            }
            if(mode==4u) {
                const int id=std::string(version)=="AC1021"?3:4;
                const auto cell=slot(varied,1,"TABLECELL_BEGIN");varied[cell+1u].value=std::to_string(id);
                mapId(varied,101,id);
            }
            if(!write(path,varied,binary))return false;
            dx_data variant;
            if(!probe.fileImport(path.string(),&variant,false)
                ||dx_iface::tableContentBindingReport(variant).state!=State::Bound) return fail("positive namespace variant failed mode="+std::to_string(mode));
            ++variants;
        }
        if(!write(path,valid,binary))return false;
        dx_data frozen;if(!probe.fileImport(path.string(),&frozen,false))return false;
        const auto* style=dynamic_cast<const DRW_Textstyle*>(DxIfaceTableSourceTestAccess::resource(frozen,0xC101u));
        frozen.textStyles.front().name="edited live style";frozen.cellStyleMaps.front().m_cellStyles[0].m_id=999;
        frozen.dictionaries.back().parentHandle=0u;
        if(!style||style->name!="TableFont-é"||dx_iface::tableContentBindingReport(frozen).state!=State::Bound)
            return fail("editable state changed frozen resource identities");
        for(bool strict:{false,true}) {
            const auto output=dir/"binding-refused.dxf";{std::ofstream out(output);out<<"keep";}
            dx_iface exporter;
            if(exporter.fileExport(output.string(),data.sourceVersion,binary,&data,false,strict))return fail("binding certificate enabled output");
            std::ifstream old(output);if(std::string(std::istreambuf_iterator<char>(old),{})!="keep")return fail("binding refusal replaced destination");
            dx_data clean;dx_iface author;author.cData=&clean;author.currentBlock=clean.mBlock;
            DRW_Line line;line.basePoint={1,2,3};line.secPoint={4,5,6};author.addLine(line);
            if(!exporter.fileExport(output.string(),data.sourceVersion,binary,&clean,false,strict))
                return fail("binding refusal poisoned same-exporter retry");
            dx_data readback;dx_iface reader;
            if(!reader.fileImport(output.string(),&readback,false)||readback.mBlock->ent.size()!=1u)
                return fail("binding refusal retry readback failed");
            const auto* actual=dynamic_cast<const DRW_Line*>(readback.mBlock->ent.front());
            if(!actual||actual->basePoint.z!=3||actual->secPoint.z!=6)
                return fail("binding refusal retry changed line coordinates");
            ++refusals;
        }
    }
    if(!write(path,bindingSource("AC1032"),false))return false;
    Probe duplicate;duplicate.duplicateStyleCallback=true;dx_data repeated;
    if(!duplicate.fileImport(path.string(),&repeated,false)
        ||dx_iface::tableContentBindingReport(repeated).state!=State::CaptureFailed) return fail("duplicate callback resource IDs were qualified");
    duplicate.duplicateStyleCallback=false;dx_data clean;
    if(!duplicate.fileImport(path.string(),&clean,false)||dx_iface::tableContentBindingReport(clean).state!=State::Bound)return fail("duplicate callback recovery failed");
    if(!duplicate.fileImport(path.string(),&clean,false)
        ||dx_iface::tableContentBindingReport(clean).state!=State::SourceNotReady)return fail("appended generation acquired resource bindings");
    std::cout<<"TABLECONTENT binding: "<<routes<<" resource routes; "<<negative<<" fault/retry pairs; "
        <<boundaries<<" boundaries; "<<variants<<" positive variants; "<<refusals<<" destination-preserving refusals; frozen/duplicate/generation controls\n";
    return routes==8u&&negative==faults.size()*routes&&boundaries==64u&&variants==40u&&refusals==16u;
}

// Authored locally from scratch. Exercise the witnessed rich map shape without
// copying either external drawing's payload, names or numeric style values.
Tags richMapSource(const std::string& version) {
    auto tags=bindingSource(version);
    const auto begin=slot(tags,5,"C120")-1u;
    auto end=begin+1u;while(tags[end].code!=0)++end;
    Tags map{{0,"CELLSTYLEMAP"},{5,"C120"},{102,"{ACAD_REACTORS"},{330,"C110"},{102,"}"},
        {330,"C110"},{100,"AcDbCellStyleMap"},{90,"3"}};
    for(int id:{1,2,101}) {
        map.push_back({300,"CELLSTYLE"});
        const auto start=map.size();style(map,5,true);
        // Make every rich map carry all six explicit edge slots.
        const auto tail=map.size()-1u;
        for(std::size_t i=start;i<tail;++i) {
            if(map[i].code==94 && map[i].value=="1")map[i].value="6";
            if(map[i].code==92 && map[i].value=="4")map[i].value="512";
        }
        Tags extra;
        for(int edge:{2,4,8,16,32})extra.insert(extra.end(),{
            {95,std::to_string(edge)},{302,"GRIDFORMAT"},{1,"GRIDFORMAT_BEGIN"},
            {90,"63"},{91,"2"},{62,"7"},{92,"17"},{340,"C102"},{93,"1"},{40,"0.25"},{309,"GRIDFORMAT_END"}});
        map.insert(map.begin()+static_cast<std::ptrdiff_t>(tail),extra.begin(),extra.end());
        map.insert(map.end(),{{1,"CELLSTYLE_BEGIN"},{90,std::to_string(id)},{91,"1"},
            {300,"generated-rich-"+std::to_string(id)},{309,"CELLSTYLE_END"}});
    }
    tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(begin),tags.begin()+static_cast<std::ptrdiff_t>(end));
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(begin),map.begin(),map.end());
    return tags;
}

bool mapProfileTests(const std::filesystem::path& dir) {
    const auto fail=[](const std::string& text){std::cerr<<text<<'\n';return false;};
    const auto path=dir/"map-profile.dxf";
    const auto mapSlot=[](const Tags& tags,int code,const std::string& value) {
        for(auto i=slot(tags,5,"C120");i<tags.size() && (tags[i].code!=0);++i)
            if(tags[i].code==code && tags[i].value==value)return i;
        throw std::runtime_error("map mutation slot missing");
    };
    const auto payloadMatches=[](const DRW_CellStyleMap& map) {
        if(!map.finalizeDxf()||map.handle!=0xC120u||map.parentHandle!=0xC110u
            ||map.reactorHandles!=std::vector<std::uint32_t>{0xC110u}||map.m_cellStyles.size()!=3u)return false;
        const auto plainColor=[](const DRW_TableStyleCmcColor& color) {
            return color.m_rgb==-1&&color.m_name.empty()&&color.m_bookName.empty()
                &&color.m_method==0&&color.m_flags==0&&color.m_reservedIndex==0;
        };
        const int ids[]{1,2,101};
        for(unsigned i=0;i<3u;++i) {
            const auto& s=map.m_cellStyles[i];const auto& f=s.m_contentFormat;
            if(s.m_type!=5||!s.m_hasData||s.m_propertyOverrideFlags!=512u||s.m_mergeFlags!=32768u
                ||s.m_backgroundColor!=257||!plainColor(s.m_backgroundColorCmc)||s.m_contentLayout!=1u
                ||s.m_marginOverrideFlags!=1||s.m_verticalMargin!=1||s.m_horizontalMargin!=2
                ||s.m_bottomMargin!=3||s.m_rightMargin!=4||s.m_marginHorizontalSpacing!=5||s.m_marginVerticalSpacing!=6
                ||s.m_id!=ids[i]||s.m_styleClass!=1||s.m_name!="generated-rich-"+std::to_string(ids[i])||s.m_borders.size()!=6u
                ||f.m_propertyOverrideFlags!=3u||f.m_propertyFlags!=256u||f.m_valueDataType!=512||f.m_valueUnitType!=0
                ||f.m_valueFormatString!="fmt-é"||f.m_rotation!=0.125||f.m_blockScale!=2||f.m_cellAlignment!=5
                ||f.m_contentColor!=0||f.m_contentColorCmc.m_rgb!=1193046||f.m_contentColorCmc.m_method!=0xC2u
                ||f.m_contentColorCmc.m_flags!=0||f.m_contentColorCmc.m_reservedIndex!=0
                ||!f.m_contentColorCmc.m_name.empty()||!f.m_contentColorCmc.m_bookName.empty()
                ||f.m_textStyleHandle!=0xC101u||f.m_textHeight!=3.25)return false;
            for(unsigned edge=0;edge<6u;++edge) {
                const auto& b=s.m_borders[edge];
                if(b.m_edgeFlags!=static_cast<int>(1u<<edge)||b.m_propertyOverrideFlags!=63||b.m_borderType!=2
                    ||b.m_color!=7||!plainColor(b.m_colorCmc)||b.m_lineWeight!=17||b.m_lineTypeHandle!=0xC102u
                    ||b.m_visible!=1||b.m_doubleLineSpacing!=0.25)return false;
            }
        }
        return true;
    };
    const std::vector<std::function<void(Tags&)>> faults{
        [mapSlot](auto& t){t[mapSlot(t,62,"257")].value="258";},
        [mapSlot](auto& t){t[mapSlot(t,62,"257")].value="-1";},
        [mapSlot](auto& t){const auto i=mapSlot(t,62,"257");t.insert(t.begin()+static_cast<std::ptrdiff_t>(i+1u),{420,"1193046"});},
        [mapSlot](auto& t){const auto i=mapSlot(t,62,"257");t.insert(t.begin()+static_cast<std::ptrdiff_t>(i+1u),{430,"unproved-none-name"});},
        [mapSlot](auto& t){const auto i=mapSlot(t,62,"257");t.insert(t.begin()+static_cast<std::ptrdiff_t>(i+1u),{431,"unproved-none-book"});},
        [mapSlot](auto& t){t[mapSlot(t,62,"0")].value="257";},
        [mapSlot](auto& t){t[mapSlot(t,62,"7")].value="257";},
        [mapSlot](auto& t){t[mapSlot(t,90,"3")].value="4";},
        [mapSlot](auto& t){const auto i=mapSlot(t,1,"CELLSTYLE_BEGIN");t[i+1u].value="2";},
        [mapSlot](auto& t){const auto i=mapSlot(t,1,"CELLSTYLE_BEGIN");t[i+1u].value="5";},
        [mapSlot](auto& t){const auto i=mapSlot(t,1,"CELLSTYLE_BEGIN");t[i+1u].value="0";},
        [mapSlot](auto& t){const auto i=mapSlot(t,1,"CELLSTYLE_BEGIN");t[i+1u].value="-1";},
        [mapSlot](auto& t){t[mapSlot(t,309,"GRIDFORMAT_END")].value="UNKNOWN_GRID_END";},
        [mapSlot](auto& t){const auto i=mapSlot(t,340,"C102");t.erase(t.begin()+static_cast<std::ptrdiff_t>(i));},
        [mapSlot](auto& t){const auto i=mapSlot(t,94,"6");t[i].value="7";},
        [mapSlot](auto& t){const auto i=mapSlot(t,309,"CELLSTYLE_END");t.insert(t.begin()+static_cast<std::ptrdiff_t>(i),{100,"UnknownMapSubclass"});}
    };
    unsigned routes=0u,negative=0u,revisionChecks=0u,versionless=0u,budgetRetries=0u;
    for(const char* revision:{"AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true}) {
        const auto good=richMapSource(revision);Probe probe;dx_data data;
        if(!write(path,good,binary)||!probe.fileImport(path.string(),&data,false)||probe.mapCallbacks!=1u||probe.rawMapCallbacks!=1u
            ||data.cellStyleMaps.size()!=1u||!payloadMatches(data.cellStyleMaps.front())
            ||dx_iface::tableContentBindingReport(data).state!=dx_iface::TableBindingState::Bound)return fail("rich map source payload/binding failed");
        ++routes;
        for(std::size_t index=0;index<faults.size();++index) {
            auto bad=good;faults[index](bad);dx_data rejected;
            const auto callbacks=probe.mapCallbacks,raw=probe.rawMapCallbacks;
            if(!write(path,bad,binary)||!probe.fileImport(path.string(),&rejected,false)||probe.mapCallbacks!=callbacks
                ||probe.rawMapCallbacks!=raw+1u||!rejected.cellStyleMaps.empty()
                ||dx_iface::tableContentBindingReport(rejected).state!=dx_iface::TableBindingState::MissingTarget)
                return fail("malformed map published typed data index="+std::to_string(index));
            dx_data corrected;
            if(!write(path,good,binary)||!probe.fileImport(path.string(),&corrected,false)
                ||corrected.cellStyleMaps.size()!=1u||!payloadMatches(corrected.cellStyleMaps.front())
                ||dx_iface::tableContentBindingReport(corrected).state!=dx_iface::TableBindingState::Bound)return fail("map profile corrected import failed");
            ++negative;
        }
        // A valid ID 4 is revision-specific, not a text codec tier decision.
        auto id4=good;
        const auto cell=slot(id4,1,"TABLECELL_BEGIN");id4[cell+1u].value="4";
        const auto identity=mapSlot(id4,1,"CELLSTYLE_BEGIN");id4[identity+1u].value="4";
        // Local ID 101 must remain in the map, while ID 1 is no longer used.
        const auto column=slot(id4,1,"TABLECOLUMN_BEGIN");id4[column+1u].value="4";
        dx_data versioned;const auto callbacks=probe.mapCallbacks;
        const bool modern=std::string(revision)!="AC1021";
        if(!write(path,id4,binary)||!probe.fileImport(path.string(),&versioned,false)
            ||probe.mapCallbacks!=callbacks+(modern?1u:0u)
            ||dx_iface::tableContentBindingReport(versioned).state!=(modern?dx_iface::TableBindingState::Bound:dx_iface::TableBindingState::SourceNotReady))
            return fail("declared-revision ID 4 check failed");
        ++revisionChecks;
        // Direct/internal readers with no declared source revision stay at
        // the conservative codec tier; do not silently assume R24.
        struct ExposedMap:DRW_CellStyleMap{using DRW_CellStyleMap::parseCode;};
        std::ostringstream body;
        const auto begin=mapSlot(id4,5,"C120");auto end=begin;while(id4[end].code!=0)++end;
        for(auto i=begin;i<end;++i)body<<id4[i].code<<'\n'<<id4[i].value<<'\n';
        std::istringstream stream(body.str());std::unique_ptr<dxfReader> reader(new dxfReaderAscii(&stream));ExposedMap direct;
        int code=0;while(reader->readRec(&code))if(!direct.parseCode(code,reader))return false;
        if(reader->hasSourceVersion()||direct.finalizeDxf())return fail("versionless direct map assumed R24");
        ++versionless;
        // A public-reader retention failure cannot publish a partial typed or
        // raw map. Reuse the same codec for the explicitly increased-budget retry.
        Tags isolated{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,revision},{0,"ENDSEC"},
            {0,"SECTION"},{2,"OBJECTS"}};
        const auto record=slot(good,5,"C120")-1u;auto recordEnd=record+1u;
        while(good[recordEnd].code!=0)++recordEnd;
        isolated.insert(isolated.end(),good.begin()+static_cast<std::ptrdiff_t>(record),good.begin()+static_cast<std::ptrdiff_t>(recordEnd));
        isolated.insert(isolated.end(),{{0,"ENDSEC"},{0,"EOF"}});
        isolated[mapSlot(isolated,300,"generated-rich-1")].value=std::string(16000u,'q');
        if(!write(path,isolated,binary))return false;
        dxfRW codec(path.string().c_str());codec.setDxfReadRetainedByteBudget(4096u);
        Probe bounded;dx_data shortBudget;bounded.cData=&shortBudget;bounded.currentBlock=shortBudget.mBlock;
        if(codec.read(&bounded,false)||bounded.mapCallbacks||bounded.rawMapCallbacks)
            return fail("short public budget published a partial map");
        codec.setDxfReadRetainedByteBudget(2u*1024u*1024u);
        dx_data restored;bounded.cData=&restored;bounded.currentBlock=restored.mBlock;
        if(!codec.read(&bounded,false)||bounded.mapCallbacks!=1u||bounded.rawMapCallbacks!=1u
            ||restored.cellStyleMaps.size()!=1u||restored.cellStyleMaps.front().m_cellStyles[0].m_name.size()!=16000u)
            return fail("public map budget retry failed");
        ++budgetRetries;
    }
    DRW_CellStyleMap native;DRW_TableStyleCellStyle nativeStyle;
    nativeStyle.m_type=5;nativeStyle.m_hasData=true;nativeStyle.m_backgroundColor=256;
    nativeStyle.m_contentFormat.m_cellAlignment=5;nativeStyle.m_id=101;nativeStyle.m_styleClass=1;
    native.m_cellStyles.push_back(nativeStyle);
    if(!native.validateDwgPayload(DRW::AC1021))return fail("native guard control was not initially writable");
    native.m_cellStyles[0].m_backgroundColor=257;
    if(native.validateDwgPayload(DRW::AC1021))return fail("DXF no-color source profile widened native CMC output");
    std::cout<<"CELLSTYLEMAP source: "<<routes<<" rich routes; "<<negative<<" malformed/recovery pairs; "
        <<revisionChecks<<" declared revision checks; "<<versionless<<" versionless refusals; "<<budgetRetries<<" budget retries; native guard retained\n";
    return routes==8u&&negative==faults.size()*routes&&revisionChecks==8u&&versionless==8u&&budgetRetries==8u;
}

bool mapSourceProofTests(const std::filesystem::path& dir) {
    using S=dx_iface::TableBindingState;using R=dx_tableResourceRole;
    const auto fail=[](const std::string& text){std::cerr<<text<<'\n';return false;};
    const auto path=dir/"map-source-proof.dxf";
    std::vector<std::function<void(DRW_CellStyleMap&)>> mutations{
        [](auto& m){m.name="unserialized base name";},[](auto& m){m.flags=1;},
        [](auto& m){m.parentHandle=0;},[](auto& m){m.xDictHandle=0xABu;},
        [](auto& m){m.reactorHandles.push_back(0xABu);},
        [](auto& m){m.setDwgCommonObjectState(m.reactorCount()+1,m.extensionDictionaryFlag(),m.hasDataStorageBinaryData());},
        [](auto& m){m.setDwgCommonObjectState(m.reactorCount(),m.extensionDictionaryFlag()^1u,m.hasDataStorageBinaryData());},
        [](auto& m){m.setDwgCommonObjectState(m.reactorCount(),m.extensionDictionaryFlag(),true);},
        [](auto& m){m.setDwgCommonLinkTailValidated(true);},
        [](auto& m){m.pendingAppIdResolutions.push_back({0u,1u,0u,false});},
        [](auto& m){m.pendingLayerRefResolutions.push_back({0u,1u,0u,false});},
        [](auto& m){m.appData.front().front().addString(300,"changed common app data");},
        [](auto& m){for(auto* v:m.extData)if(v && v->code()==1000){v->addString(1000,"changed XDATA");break;}},
        [](auto& m){DRW_DwgSubrecordRange r{};m.m_subrecordRanges.push_back(r);},
        [](auto& m){m.m_cellStyles.pop_back();}
    };
    for(unsigned index=0;index<3u;++index) {
        const auto integer=[&](int DRW_TableStyleCellStyle::* member) {
            mutations.push_back([=](auto& m){++(m.m_cellStyles[index].*member);});
        };
        for(auto field:{&DRW_TableStyleCellStyle::m_type,&DRW_TableStyleCellStyle::m_backgroundColor,
            &DRW_TableStyleCellStyle::m_marginOverrideFlags,&DRW_TableStyleCellStyle::m_id,&DRW_TableStyleCellStyle::m_styleClass})integer(field);
        for(auto field:{&DRW_TableStyleCellStyle::m_propertyOverrideFlags,&DRW_TableStyleCellStyle::m_mergeFlags,&DRW_TableStyleCellStyle::m_contentLayout})
            mutations.push_back([=](auto& m){++(m.m_cellStyles[index].*field);});
        for(auto field:{&DRW_TableStyleCellStyle::m_verticalMargin,&DRW_TableStyleCellStyle::m_horizontalMargin,
            &DRW_TableStyleCellStyle::m_bottomMargin,&DRW_TableStyleCellStyle::m_rightMargin,
            &DRW_TableStyleCellStyle::m_marginHorizontalSpacing,&DRW_TableStyleCellStyle::m_marginVerticalSpacing})
            mutations.push_back([=](auto& m){(m.m_cellStyles[index].*field)+=0.5;});
        mutations.push_back([=](auto& m){m.m_cellStyles[index].m_hasData=false;});
        mutations.push_back([=](auto& m){m.m_cellStyles[index].m_name+=" edited";});
        for(auto field:{&DRW_TableStyleContentFormat::m_propertyOverrideFlags,&DRW_TableStyleContentFormat::m_propertyFlags})
            mutations.push_back([=](auto& m){++(m.m_cellStyles[index].m_contentFormat.*field);});
        for(auto field:{&DRW_TableStyleContentFormat::m_valueDataType,&DRW_TableStyleContentFormat::m_valueUnitType,
            &DRW_TableStyleContentFormat::m_cellAlignment,&DRW_TableStyleContentFormat::m_contentColor})
            mutations.push_back([=](auto& m){++(m.m_cellStyles[index].m_contentFormat.*field);});
        for(auto field:{&DRW_TableStyleContentFormat::m_rotation,&DRW_TableStyleContentFormat::m_blockScale,&DRW_TableStyleContentFormat::m_textHeight})
            mutations.push_back([=](auto& m){(m.m_cellStyles[index].m_contentFormat.*field)+=0.5;});
        mutations.push_back([=](auto& m){m.m_cellStyles[index].m_contentFormat.m_valueFormatString+=" edited";});
        mutations.push_back([=](auto& m){++m.m_cellStyles[index].m_contentFormat.m_textStyleHandle;});
        const auto colorMutations=[&](bool background) {
            const auto color=[=](DRW_CellStyleMap& m)->DRW_TableStyleCmcColor& {
                return background?m.m_cellStyles[index].m_backgroundColorCmc:m.m_cellStyles[index].m_contentFormat.m_contentColorCmc;
            };
            mutations.push_back([=](auto& m){++color(m).m_rgb;});
            mutations.push_back([=](auto& m){++color(m).m_method;});
            mutations.push_back([=](auto& m){++color(m).m_flags;});
            mutations.push_back([=](auto& m){++color(m).m_reservedIndex;});
            mutations.push_back([=](auto& m){color(m).m_name="changed name";});
            mutations.push_back([=](auto& m){color(m).m_bookName="changed book";});
        };
        colorMutations(true);colorMutations(false);
        for(unsigned border=0;border<6u;++border) {
            for(auto field:{&DRW_TableStyleBorder::m_edgeFlags,&DRW_TableStyleBorder::m_propertyOverrideFlags,
                &DRW_TableStyleBorder::m_borderType,&DRW_TableStyleBorder::m_color,&DRW_TableStyleBorder::m_lineWeight,&DRW_TableStyleBorder::m_visible})
                mutations.push_back([=](auto& m){++(m.m_cellStyles[index].m_borders[border].*field);});
            mutations.push_back([=](auto& m){++m.m_cellStyles[index].m_borders[border].m_lineTypeHandle;});
            mutations.push_back([=](auto& m){m.m_cellStyles[index].m_borders[border].m_doubleLineSpacing+=0.5;});
            mutations.push_back([=](auto& m){++m.m_cellStyles[index].m_borders[border].m_colorCmc.m_rgb;});
            mutations.push_back([=](auto& m){++m.m_cellStyles[index].m_borders[border].m_colorCmc.m_method;});
            mutations.push_back([=](auto& m){++m.m_cellStyles[index].m_borders[border].m_colorCmc.m_flags;});
            mutations.push_back([=](auto& m){++m.m_cellStyles[index].m_borders[border].m_colorCmc.m_reservedIndex;});
            mutations.push_back([=](auto& m){m.m_cellStyles[index].m_borders[border].m_colorCmc.m_name="changed name";});
            mutations.push_back([=](auto& m){m.m_cellStyles[index].m_borders[border].m_colorCmc.m_bookName="changed book";});
        }
        mutations.push_back([=](auto& m){m.m_cellStyles[index].m_borders.pop_back();});
    }
    const std::vector<std::function<void(DRW_RawDxfObject&)>> rawFaults{
        [](auto& r){r.rawValues.pop_back();},[](auto& r){r.hasRawValues=!r.hasRawValues;},
        [](auto& r){r.parentHandle=0;},[](auto& r){r.m_version=DRW::AC1018;},
        [](auto& r){r.groups.back().addString(309,"UNKNOWN_END");},
        [](auto& r){r.groups.pop_back();r.rawValues.pop_back();},
        [](auto& r){r.groups[0].addString(5,"C121");},
        [](auto& r){r.groups.emplace_back(100,"UnknownSubclass");r.rawValues.push_back("UnknownSubclass");},
        [](auto& r){r.rawValues[0]="wrong scalar";},
        [](auto& r){r.groups.emplace_back(10,DRW_Coord(1,2,3));r.rawValues.push_back("1");},
        [](auto& r){r.groups.front().setLayerRefName("not raw scalar metadata");}
    };
    unsigned routes=0u,negative=0u,rawRefusals=0u,boundaries=0u,sourceControls=0u;
    for(const char* revision:{"AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true}) {
        auto tags=richMapSource(revision);const auto mapStart=slot(tags,5,"C120");auto mapEnd=mapStart;
        while(tags[mapEnd].code!=0)++mapEnd;
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(mapEnd),{{1001,"PROOF_APP"},{1000,"common ^J text-é"},
            {1002,"{"},{1003,"PROOF_LAYER"},{1005,"C101"},{1004,"01020304"},{1070,"12"},{1002,"}"}});
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(mapStart+1u),{{102,"{PROOF_META"},{300,"application payload"},{102,"}"}});
        if(!write(path,tags,binary))return false;
        Probe probe;dx_data clean;
        if(!probe.fileImport(path.string(),&clean,false))return fail("map proof source import failed");
        const auto report=dx_iface::tableContentBindingReport(clean);
        const auto& frozen=DxIfaceTableSourceTestAccess::mapResource(clean);
        if(report.state!=S::Bound||report.sourceMapMatches!=1u||report.sourceStyleMatches!=1u||report.resourceProofRecords!=11u||!frozen.rawTypedMatched
            ||frozen.references.empty()||!frozen.raw)return fail("map canonical proof failed");
        std::array<unsigned,11> roles{};unsigned handles=0u;
        for(const auto& ref:frozen.references) {
            ++roles[static_cast<unsigned>(ref.kind)];const auto& group=frozen.raw->groups.at(ref.groupIndex);
            if(group.code()!=ref.code)return fail("map reference span differs");
            if(ref.kind==R::LocalStyleDefinition) {if(group.i_val()!=static_cast<int>(ref.value))return false;}
            else if(ref.kind!=R::AppIdName&&ref.kind!=R::LayerName) {
                if(group.type()!=DRW_Variant::STRING||std::stoul(*group.content.s,nullptr,16)!=ref.value)return false;
                ++handles;
            }
        }
        const auto actualHandles=std::count_if(frozen.raw->groups.begin(),frozen.raw->groups.end(),[](const auto& v){return isDxfHandleCode(v.code());});
        if(handles!=actualHandles||roles[static_cast<unsigned>(R::TextStyle)]!=3u||roles[static_cast<unsigned>(R::LineType)]!=18u
            ||roles[static_cast<unsigned>(R::LocalStyleDefinition)]!=3u||roles[static_cast<unsigned>(R::AppIdName)]!=1u
            ||roles[static_cast<unsigned>(R::LayerName)]!=1u||roles[static_cast<unsigned>(R::XdataObjectId)]!=1u)return fail("map role census differs");
        ++routes;
        for(std::size_t i=0;i<mutations.size();++i) {
            probe.alterMap=mutations[i];dx_data bad;
            if(!probe.fileImport(path.string(),&bad,false)||dx_iface::tableContentBindingReport(bad).state==S::Bound
                ||DxIfaceTableSourceTestAccess::mapResource(bad).rawTypedMatched
                ||!DxIfaceTableSourceTestAccess::mapResource(bad).references.empty())return fail("typed map mutation escaped index="+std::to_string(i));
            probe.alterMap={};dx_data restored;
            if(!probe.fileImport(path.string(),&restored,false)||dx_iface::tableContentBindingReport(restored).state!=S::Bound)return fail("map proof same-facade recovery failed");
            probe.clear(); // Keep repeated proof controls from retaining every callback observation.
            ++negative;
        }
        for(std::size_t i=0;i<rawFaults.size();++i) {
            probe.alterMapRaw=rawFaults[i];dx_data bad;
            if(!probe.fileImport(path.string(),&bad,false)||dx_iface::tableContentBindingReport(bad).state==S::Bound
                ||!DxIfaceTableSourceTestAccess::mapResource(bad).references.empty())return fail("raw map mutation escaped index="+std::to_string(i));
            if(i+2u>=rawFaults.size() && DxIfaceTableSourceTestAccess::mapResource(bad).raw)
                return fail("unsupported raw map payload copied before refusal");
            probe.alterMapRaw={};dx_data restored;
            if(!probe.fileImport(path.string(),&restored,false)||dx_iface::tableContentBindingReport(restored).state!=S::Bound)return fail("raw map recovery failed");
            probe.clear();
            ++rawRefusals;
        }
        for(unsigned i=0;i<8u;++i) {
            dx_data limited;
            DxIfaceTableSourceTestAccess::resourceProofLimits(limited,
                i==0?report.resourceProofRecords-1u:i==4?report.resourceProofRecords:100000u,
                i==1?report.resourceProofGroups-1u:i==5?report.resourceProofGroups:4000000u,
                i==2?report.resourceProofBytes-1u:i==6?report.resourceProofBytes:64u*1024u*1024u,
                i==3?report.resourceProofWork-1u:i==7?report.resourceProofWork:4000000u);
            if(!probe.fileImport(path.string(),&limited,false)
                ||dx_iface::tableContentBindingReport(limited).state!=(i<4?S::CaptureFailed:S::Bound))return fail("map proof boundary failed index="+std::to_string(i));
            ++boundaries;
        }
        for(unsigned mode=0;mode<3u;++mode) {
            probe.skipMapRaw=mode==0u;probe.duplicateMapRaw=mode!=0u;dx_data bad;
            if(mode==2u)probe.alterMapRaw=rawFaults.back();
            if(!probe.fileImport(path.string(),&bad,false)||dx_iface::tableContentBindingReport(bad).state!=(mode==0u?S::ResourceProofMissing:S::CaptureFailed))return fail("missing/duplicate map source qualified");
            probe.skipMapRaw=false;probe.duplicateMapRaw=false;probe.alterMapRaw={};dx_data restored;
            if(!probe.fileImport(path.string(),&restored,false)||dx_iface::tableContentBindingReport(restored).state!=S::Bound)return fail("map source callback recovery failed");
            ++sourceControls;
        }
        // A consistent but unestablished common handle role is still unknown,
        // even though its raw/typed payload can be proved exactly.
        auto unknown=tags;const auto app=slot(unknown,300,"application payload");
        unknown.insert(unknown.begin()+static_cast<std::ptrdiff_t>(app+1u),{330,"C101"});
        if(!write(path,unknown,binary))return false;
        dx_data opaque;if(!probe.fileImport(path.string(),&opaque,false))return false;
        const auto unknownReport=dx_iface::tableContentBindingReport(opaque);
        if(unknownReport.state!=S::UnknownRole||unknownReport.sourceMapMatches!=1u||unknownReport.unknownResourceReferences!=1u)
            return fail("unknown common role acquired target qualification");
        if(!write(path,tags,binary))return false;
        clean.cellStyleMaps.front().m_cellStyles[0].m_name="live edit";
        clean.rawCellStyleMaps.front().groups.clear();
        const auto again=dx_iface::tableContentBindingReport(clean);
        if(again.state!=S::Bound||again.resourceProofWork!=report.resourceProofWork||!DxIfaceTableSourceTestAccess::mapResource(clean).raw->groups.size())return fail("editable carrier changed immutable source proof");
    }
    std::cout<<"CELLSTYLEMAP proof: "<<routes<<" routes; "<<mutations.size()<<" typed mutation kinds; "<<negative<<" mismatch/recovery pairs; "
        <<rawRefusals<<" raw refusals/recoveries; "<<boundaries<<" boundaries; "<<sourceControls<<" callback/recovery controls; exact roles and immutable cache\n";
    return routes==8u&&negative==mutations.size()*routes&&rawRefusals==rawFaults.size()*routes&&boundaries==64u&&sourceControls==24u;
}

Tags legacyTableStyleSource(const std::string& revision) {
    auto tags=richMapSource(revision);
    const auto begin=slot(tags,5,"C100");auto end=begin+1u;while(tags[end].code!=0)++end;
    for(auto i=begin;i<end;++i)if(tags[i].code==3)tags[i].value="local table ^J description-é";
    Tags rows;
    for(unsigned row=0;row<3u;++row) {
        rows.insert(rows.end(),{{7,"TableFont-é"},{140,std::to_string(row+2)+".5"},{170,"5"},
            {62,"256"},{420,"1193046"},{430,"Ink-é"},{431,"Book-é"},
            {63,"7"},{283,"1"},{90,"512"},{91,"0"},{1,"row ^J format-é"}});
        for(int edge=0;edge<6;++edge)rows.insert(rows.end(),{{274+edge,std::to_string(17+edge)},
            {284+edge,std::to_string(edge%2)},{64+edge,"7"},{420,"1193046"}});
    }
    rows.insert(rows.end(),{{1001,"STYLE_PROOF_APP"},{1000,"common ^J metadata-é"},{1003,"STYLE_PROOF_LAYER"},{1005,"C101"}});
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),rows.begin(),rows.end());
    return tags;
}

bool tableStyleProofTests(const std::filesystem::path& dir) {
    using S=dx_iface::TableBindingState;using R=dx_tableResourceRole;
    const auto fail=[](const std::string& text){std::cerr<<text<<'\n';return false;};
    const auto path=dir/"table-style-proof.dxf";
    std::vector<std::function<void(DRW_TableStyle&)>> mutations{
        [](auto& s){s.m_name+="changed";},[](auto& s){static_cast<DRW_TableEntry&>(s).name="changed";},
        [](auto& s){++s.flags;},[](auto& s){++s.m_flags;},[](auto& s){++s.m_flowDirection;},
        [](auto& s){s.m_horizontalCellMargin+=0.5;},[](auto& s){s.m_verticalCellMargin+=0.5;},
        [](auto& s){s.m_titleSuppressed=!s.m_titleSuppressed;},[](auto& s){s.m_headerSuppressed=!s.m_headerSuppressed;},
        [](auto& s){++s.m_unknownHandle;},[](auto& s){s.reactorHandles.push_back(0xC110u);},
        [](auto& s){s.setDwgCommonLinkTailValidated(true);},
        [](auto& s){s.setDwgCommonObjectState(s.reactorCount(),s.extensionDictionaryFlag(),true);},
        [](auto& s){s.setDwgCommonObjectState(s.reactorCount()+1,s.extensionDictionaryFlag(),false);},
        [](auto& s){s.setDwgCommonObjectState(s.reactorCount(),s.extensionDictionaryFlag()^1u,false);},
        [](auto& s){s.pendingAppIdResolutions.push_back({0u,1u,0u,false});},
        [](auto& s){s.pendingLayerRefResolutions.push_back({0u,1u,0u,false});},
        [](auto& s){s.extData[0]->addString(1001,"changed XDATA");},
        [](auto& s){s.m_tableCellStyle.m_name="unserialized native name";},
        [](auto& s){s.m_tableCellStyle.m_contentFormat.m_rotation=-0.0;},
        [](auto& s){s.m_cellStyles.emplace_back();},[](auto& s){s.m_subrecordRanges.emplace_back();},
        [](auto& s){s.m_rowStyles.pop_back();},
        [](auto& s){s.m_rowStyles.push_back(s.m_rowStyles.front());}
    };
    for(unsigned row=0;row<3u;++row) {
        mutations.push_back([=](auto& s){s.m_rowStyles[row].m_textStyleName+="changed";});
        mutations.push_back([=](auto& s){++s.m_rowStyles[row].m_textStyleHandle;});
        mutations.push_back([=](auto& s){s.m_rowStyles[row].m_textHeight+=0.5;});
        mutations.push_back([=](auto& s){s.m_rowStyles[row].m_hasBackgroundColor=false;});
        mutations.push_back([=](auto& s){s.m_rowStyles[row].m_valueFormatString+="changed";});
        for(auto field:{&DRW_TableStyleRowStyle::m_textAlignment,&DRW_TableStyleRowStyle::m_textColor,&DRW_TableStyleRowStyle::m_fillColor,
            &DRW_TableStyleRowStyle::m_valueDataType,&DRW_TableStyleRowStyle::m_valueUnitType})
            mutations.push_back([=](auto& s){++(s.m_rowStyles[row].*field);});
        for(bool text:{false,true}) {
            const auto color=[=](DRW_TableStyle& s)->DRW_TableStyleCmcColor& {
                return text?s.m_rowStyles[row].m_textColorCmc:s.m_rowStyles[row].m_fillColorCmc;
            };
            mutations.push_back([=](auto& s){++color(s).m_rgb;});
            mutations.push_back([=](auto& s){++color(s).m_method;});
            mutations.push_back([=](auto& s){++color(s).m_flags;});
            mutations.push_back([=](auto& s){++color(s).m_reservedIndex;});
            mutations.push_back([=](auto& s){color(s).m_name+="changed";});
            mutations.push_back([=](auto& s){color(s).m_bookName+="changed";});
        }
        for(unsigned edge=0;edge<6u;++edge) {
            for(auto field:{&DRW_TableStyleBorder::m_edgeFlags,&DRW_TableStyleBorder::m_propertyOverrideFlags,
                &DRW_TableStyleBorder::m_borderType,&DRW_TableStyleBorder::m_color,&DRW_TableStyleBorder::m_lineWeight,&DRW_TableStyleBorder::m_visible})
                mutations.push_back([=](auto& s){++(s.m_rowStyles[row].m_borders[edge].*field);});
            mutations.push_back([=](auto& s){++s.m_rowStyles[row].m_borders[edge].m_lineTypeHandle;});
            mutations.push_back([=](auto& s){s.m_rowStyles[row].m_borders[edge].m_doubleLineSpacing+=0.5;});
            mutations.push_back([=](auto& s){++s.m_rowStyles[row].m_borders[edge].m_colorCmc.m_rgb;});
            mutations.push_back([=](auto& s){++s.m_rowStyles[row].m_borders[edge].m_colorCmc.m_method;});
            mutations.push_back([=](auto& s){++s.m_rowStyles[row].m_borders[edge].m_colorCmc.m_flags;});
            mutations.push_back([=](auto& s){++s.m_rowStyles[row].m_borders[edge].m_colorCmc.m_reservedIndex;});
            mutations.push_back([=](auto& s){s.m_rowStyles[row].m_borders[edge].m_colorCmc.m_name="changed";});
            mutations.push_back([=](auto& s){s.m_rowStyles[row].m_borders[edge].m_colorCmc.m_bookName="changed";});
        }
        mutations.push_back([=](auto& s){s.m_rowStyles[row].m_borders.pop_back();});
    }
    const std::vector<std::function<void(DRW_RawDxfObject&)>> rawFaults{
        [](auto& r){r.rawValues.pop_back();},[](auto& r){r.hasRawValues=!r.hasRawValues;},
        [](auto& r){r.parentHandle=0;},[](auto& r){r.m_version=DRW::AC1018;},
        [](auto& r){r.groups[0].addString(5,"C121");},
        [](auto& r){r.groups.emplace_back(100,"UnknownSubclass");r.rawValues.push_back("UnknownSubclass");},
        [](auto& r){r.rawValues[0]="different scalar";},
        [](auto& r){for(std::size_t i=0;i<r.groups.size();++i)if(r.groups[i].code()==281){r.groups.erase(r.groups.begin()+static_cast<std::ptrdiff_t>(i));r.rawValues.erase(r.rawValues.begin()+static_cast<std::ptrdiff_t>(i));break;}},
        [](auto& r){for(auto& g:r.groups)if(g.code()==3){g.addString(3,"changed description");break;}},
        [](auto& r){r.groups.emplace_back(10,DRW_Coord(1,2,3));r.rawValues.push_back("1");},
        [](auto& r){r.groups.front().setLayerRefName("not raw scalar metadata");}
    };
    unsigned routes=0u,negative=0u,rawRefusals=0u,boundaries=0u,callbacks=0u,dependencies=0u,accounting=0u;
    for(const char* revision:{"AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true}) {
        const auto good=legacyTableStyleSource(revision);
        if(!write(path,good,binary))return false;
        Probe probe;dx_data clean;
        if(!probe.fileImport(path.string(),&clean,false))return fail("TABLESTYLE source import failed");
        const auto report=dx_iface::tableContentBindingReport(clean);
        const auto& frozen=DxIfaceTableSourceTestAccess::mapResource(clean,0xC100u);
        const auto* style=dynamic_cast<const DRW_TableStyle*>(frozen.model.get());
        const std::string description=binary?"local table ^J description-é":"local table \n description-é";
        if(report.state!=S::Bound||report.sourceStyleMatches!=1u||report.sourceMapMatches!=1u||report.boundResourceReferences!=24u
            ||!frozen.rawTypedMatched||!style||style->m_rowStyles.size()!=3u||style->m_name!=description
            ||style->dxfOriginalDescription()!=description||style->dxfOriginalDescriptionRaw()!="local table ^J description-é")return fail("TABLESTYLE canonical source differs");
        unsigned names=0u;for(const auto& ref:frozen.references) {
            if(frozen.raw->groups[ref.groupIndex].code()!=ref.code)return false;
            if(ref.kind==R::TextStyleName)++names;
        }
        if(names!=3u||frozen.referenceNameKeys.size()!=3u)return fail("TABLESTYLE name reference census differs");
        ++routes;
        for(std::size_t i=0;i<mutations.size();++i) {
            probe.alterStyle=mutations[i];dx_data bad;
            if(!probe.fileImport(path.string(),&bad,false)||dx_iface::tableContentBindingReport(bad).state==S::Bound
                ||DxIfaceTableSourceTestAccess::mapResource(bad,0xC100u).rawTypedMatched
                ||!DxIfaceTableSourceTestAccess::mapResource(bad,0xC100u).references.empty())return fail("TABLESTYLE typed mutation escaped index="+std::to_string(i));
            probe.alterStyle={};dx_data restored;
            if(!probe.fileImport(path.string(),&restored,false)||dx_iface::tableContentBindingReport(restored).state!=S::Bound)return fail("TABLESTYLE mutation recovery failed");
            probe.clear();
            ++negative;
        }
        for(std::size_t i=0;i<rawFaults.size();++i) {
            probe.alterStyleRaw=rawFaults[i];dx_data bad;
            if(!probe.fileImport(path.string(),&bad,false)||dx_iface::tableContentBindingReport(bad).state==S::Bound
                ||!DxIfaceTableSourceTestAccess::mapResource(bad,0xC100u).references.empty())return fail("TABLESTYLE raw mutation escaped index="+std::to_string(i));
            if(i+2u>=rawFaults.size() && DxIfaceTableSourceTestAccess::mapResource(bad,0xC100u).raw)
                return fail("unsupported raw TABLESTYLE payload copied before refusal");
            probe.alterStyleRaw={};dx_data restored;
            if(!probe.fileImport(path.string(),&restored,false)||dx_iface::tableContentBindingReport(restored).state!=S::Bound)return fail("TABLESTYLE raw recovery failed");
            probe.clear();
            ++rawRefusals;
        }
        for(unsigned i=0;i<8u;++i) {
            dx_data limited;DxIfaceTableSourceTestAccess::resourceProofLimits(limited,
                i==0?report.resourceProofRecords-1u:i==4?report.resourceProofRecords:100000u,
                i==1?report.resourceProofGroups-1u:i==5?report.resourceProofGroups:4000000u,
                i==2?report.resourceProofBytes-1u:i==6?report.resourceProofBytes:64u*1024u*1024u,
                i==3?report.resourceProofWork-1u:i==7?report.resourceProofWork:4000000u);
            if(!probe.fileImport(path.string(),&limited,false)||dx_iface::tableContentBindingReport(limited).state!=(i<4u?S::CaptureFailed:S::Bound))return fail("TABLESTYLE proof boundary failed");
            ++boundaries;
        }
        for(unsigned i=0;i<3u;++i) {
            probe.skipStyleRaw=i==0u;probe.duplicateStyleRaw=i!=0u;dx_data bad;
            if(i==2u)probe.alterStyleRaw=rawFaults.back();
            if(!probe.fileImport(path.string(),&bad,false)||dx_iface::tableContentBindingReport(bad).state!=(i==0u?S::ResourceProofMissing:S::CaptureFailed))return fail("TABLESTYLE callback gap qualified");
            probe.skipStyleRaw=false;probe.duplicateStyleRaw=false;probe.alterStyleRaw={};dx_data restored;
            if(!probe.fileImport(path.string(),&restored,false)||dx_iface::tableContentBindingReport(restored).state!=S::Bound)return false;
            ++callbacks;
        }
        for(unsigned mode=0;mode<11u;++mode) {
            auto varied=good;S expected=S::Bound;
            const auto mapReference=[&](const std::string& handle) {
                for(auto i=slot(varied,5,"C120");i<varied.size() && varied[i].code!=0;++i)
                    if(varied[i].code==340 && varied[i].value==handle)return i;
                throw std::runtime_error("missing map dependency slot");
            };
            if(mode==0u){varied[slot(varied,7,"TableFont-é")].value="missing font";expected=S::MissingTarget;}
            if(mode==1u){varied[slot(varied,7,"TableFont-é")].value="tABLEFONT-é";}
            if(mode==2u){varied[mapReference("C101")].value="C102";expected=S::WrongFamily;}
            if(mode==3u){varied[mapReference("C102")].value="C101";expected=S::WrongFamily;}
            if(mode==4u){const auto s=slot(varied,5,"C101")-1u;auto e=s+1u;while(varied[e].code!=0)++e;
                varied.erase(varied.begin()+static_cast<std::ptrdiff_t>(s),varied.begin()+static_cast<std::ptrdiff_t>(e));expected=S::MissingTarget;}
            if(mode==5u){varied[slot(varied,7,"TableFont-é")].value="";expected=S::InvalidNamespace;}
            if(mode==6u){varied[mapReference("C101")].value="0";}
            if(mode==7u){varied[mapReference("C102")].value="0";}
            if(mode==8u){varied[mapReference("C101")].value="FFFF";expected=S::MissingTarget;}
            if(mode==9u){const auto begin=slot(varied,0,"STYLE");auto end=begin+1u;while(varied[end].code!=0)++end;
                Tags duplicate(varied.begin()+static_cast<std::ptrdiff_t>(begin),varied.begin()+static_cast<std::ptrdiff_t>(end));
                for(auto& t:duplicate){if(t.code==5)t.value="C103";if(t.code==2)t.value="tablefont-é";}
                varied.insert(varied.begin()+static_cast<std::ptrdiff_t>(end),duplicate.begin(),duplicate.end());expected=S::AmbiguousIdentity;}
            if(mode==10u){const auto i=slot(varied,1001,"STYLE_PROOF_APP");varied.insert(varied.begin()+static_cast<std::ptrdiff_t>(i),{340,"C101"});expected=S::UnknownRole;}
            if(!write(path,varied,binary))return false;
            dx_data data;if(!probe.fileImport(path.string(),&data,false)||dx_iface::tableContentBindingReport(data).state!=expected)return fail("resource format dependency mode="+std::to_string(mode));
            if(!write(path,good,binary))return false;
            dx_data restored;if(!probe.fileImport(path.string(),&restored,false)||dx_iface::tableContentBindingReport(restored).state!=S::Bound)return false;
            ++dependencies;
        }
        // Strip only the private description baseline by constructing an
        // otherwise equal authored model. Its two string copies must be charged.
        DRW_TableStyle authored;static_cast<DRW_TableEntry&>(authored)=static_cast<const DRW_TableEntry&>(*style);
        authored.m_name=style->m_name;authored.m_flowDirection=style->m_flowDirection;authored.m_flags=style->m_flags;
        authored.m_horizontalCellMargin=style->m_horizontalCellMargin;authored.m_verticalCellMargin=style->m_verticalCellMargin;
        authored.m_titleSuppressed=style->m_titleSuppressed;authored.m_headerSuppressed=style->m_headerSuppressed;
        authored.m_unknownHandle=style->m_unknownHandle;authored.m_tableCellStyle=style->m_tableCellStyle;
        authored.m_rowStyles=style->m_rowStyles;authored.m_cellStyles=style->m_cellStyles;authored.m_subrecordRanges=style->m_subrecordRanges;
        dx_data authoredData;dx_iface author;author.cData=&authoredData;author.addTableStyle(authored);
        const auto extra=style->dxfOriginalDescription().size()+style->dxfOriginalDescriptionRaw().size()+2u;
        if(clean.tableStylesRetainedBytes!=authoredData.tableStylesRetainedBytes+extra)return fail("TABLESTYLE baseline strings were not accounted exactly");
        ++accounting;
        clean.tableStyles.front().m_rowStyles[0].m_textStyleName="live changed font";
        clean.rawTableStyles.front().groups.clear();
        const auto again=dx_iface::tableContentBindingReport(clean);
        if(again.state!=S::Bound||again.resourceProofWork!=report.resourceProofWork)return fail("mutable TABLESTYLE changed immutable proof");
    }
    std::cout<<"TABLESTYLE proof: "<<routes<<" routes; "<<mutations.size()<<" typed mutation kinds; "<<negative<<" mismatch/recovery pairs; "
        <<rawRefusals<<" raw pairs; "<<boundaries<<" boundaries; "<<callbacks<<" callback recoveries; "<<dependencies<<" dependency routes; "<<accounting<<" baseline accounting controls\n";
    return routes==8u&&negative==mutations.size()*routes&&rawRefusals==rawFaults.size()*routes&&boundaries==64u&&callbacks==24u&&dependencies==88u&&accounting==8u;
}

// All source bodies in this test are authored locally, not copied from corpus
// drawings. Complex dash metadata exercises the public normalized projection;
// its STYLE dependencies do not establish full common/transitive qualification.
Tags complexSymbolSource(const std::string& revision) {
    auto good=legacyTableStyleSource(revision);
    const auto line=slot(good,5,"C102");auto lineEnd=line;while(good[lineEnd].code!=0)++lineEnd;
    good[slot(good,73,"0")].value="2";
    const Tags segments{{49,"1.25"},{74,"2"},{75,"0"},{340,"C101"},{46,"0.75"},
        {50,"30"},{44,"0.125"},{45,"-0.25"},{9,"dash-é"},
        {49,"-0.5"},{74,"4"},{75,"7"},{340,"C103"},{46,"2"},{50,"45"},{44,"0"},{45,"0"}};
    good.insert(good.begin()+static_cast<std::ptrdiff_t>(lineEnd),segments.begin(),segments.end());
    const auto text=slot(good,5,"C101");auto textEnd=text;while(good[textEnd].code!=0)++textEnd;
    const Tags shape{{0,"STYLE"},{5,"C103"},{330,"20"},{100,"AcDbSymbolTableRecord"},
        {100,"AcDbTextStyleTableRecord"},{2,""},{70,"1"},{40,"0"},{41,"1"},
        {50,"0"},{71,"0"},{42,"1"},{3,"local-shape.shx"},{4,""}};
    good.insert(good.begin()+static_cast<std::ptrdiff_t>(textEnd),shape.begin(),shape.end());
    return good;
}

bool symbolSourceProofTests(const std::filesystem::path& dir) {
    using S=dx_iface::TableBindingState;
    const auto fail=[](const std::string& text){std::cerr<<text<<'\n';return false;};
    const auto path=dir/"symbol-source-proof.dxf";
    const std::vector<std::function<void(DRW_Textstyle&)>> styleFaults{
        [](auto& s){s.font+="changed";},[](auto& s){s.bigFont+="changed";},
        [](auto& s){++s.genFlag;},[](auto& s){++s.fontFamily;},
        [](auto& s){s.height+=0.5;},[](auto& s){s.width+=0.5;},
        [](auto& s){s.oblique+=0.5;},[](auto& s){s.lastHeight+=0.5;},
        [](auto& s){++s.xrefBlockHandle.code;},[](auto& s){++s.xrefBlockHandle.size;},
        [](auto& s){++s.xrefBlockHandle.ref;},[](auto& s){++s.xrefBlockHandle.ref64;},
        [](auto& s){++s.flags;},[](auto& s){s.reactorHandles.push_back(0xC110u);},
        [](auto& s){s.setDwgCommonLinkTailValidated(true);}
    };
    const std::vector<std::function<void(DRW_LType&)>> lineFaults{
        [](auto& s){s.desc+="changed";},[](auto& s){++s.size;},[](auto& s){++s.alignment;},
        [](auto& s){s.length+=0.5;},[](auto& s){s.path[0]+=0.5;},
        [](auto& s){s.segments[0].length+=0.5;},[](auto& s){++s.segments[0].complexShapeCode;},
        [](auto& s){++s.segments[0].shapeFlags;},[](auto& s){s.segments[0].text+="changed";},
        [](auto& s){s.segments[0].xOffset+=0.5;},[](auto& s){s.segments[0].yOffset+=0.5;},
        [](auto& s){s.segments[0].scale+=0.5;},[](auto& s){s.segments[0].rotation+=0.5;},
        [](auto& s){++s.segments[0].styleHandle.code;},[](auto& s){++s.segments[0].styleHandle.size;},
        [](auto& s){++s.segments[0].styleHandle.ref;},[](auto& s){++s.segments[0].styleHandle.ref64;},
        [](auto& s){s.shapeHandles.push_back(0xC101u);},[](auto& s){++s.xrefBlockHandle.code;},
        [](auto& s){++s.xrefBlockHandle.size;},[](auto& s){++s.xrefBlockHandle.ref;},
        [](auto& s){++s.xrefBlockHandle.ref64;},[](auto& s){++s.flags;},
        [](auto& s){s.reactorHandles.push_back(0xC110u);},
        [](auto& s){s.setDwgCommonLinkTailValidated(true);}
    };
    const std::vector<std::function<void(DRW_RawDxfObject&)>> rawFaults{
        [](auto& r){r.m_version=DRW::AC1015;},[](auto& r){r.name="LAYER";},
        [](auto& r){++r.handle;},[](auto& r){++r.parentHandle;},
        [](auto& r){r.groups[0].addString(5,"FFFF");},
        [](auto& r){for(auto& g:r.groups)if(g.code()==100){g.addString(100,"UnknownSubclass");break;}},
        [](auto& r){for(std::size_t i=0;i<r.groups.size();++i)if(r.groups[i].code()==70){r.groups.erase(r.groups.begin()+static_cast<std::ptrdiff_t>(i));if(r.hasRawValues)r.rawValues.erase(r.rawValues.begin()+static_cast<std::ptrdiff_t>(i));break;}},
        [](auto& r){r.groups.emplace_back(70,0);if(r.hasRawValues)r.rawValues.push_back("0");},
        [](auto& r){if(r.hasRawValues)r.rawValues[0]="wrong scalar";else r.hasRawValues=true;},
        [](auto& r){r.groups.emplace_back(10,DRW_Coord(1,2,3));if(r.hasRawValues)r.rawValues.push_back("1");},
        [](auto& r){r.groups.front().setLayerRefName("not a raw scalar");}
    };
    unsigned routes=0u,typed=0u,raw=0u,callbacks=0u,boundaries=0u,transport=0u;
    for(const char* revision:{"AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true}) {
        const auto good=complexSymbolSource(revision);
        if(!write(path,good,binary))return false;
        Probe probe;probe.observeSymbols=true;dx_data clean;
        if(!probe.fileImport(path.string(),&clean,false))return fail("symbol source import failed");
        const auto report=dx_iface::tableContentBindingReport(clean);
        const std::vector<std::string> events{"raw:STYLE","typed:STYLE","raw:STYLE","typed:STYLE","raw:LTYPE","typed:LTYPE"};
        if(report.state!=S::Bound||report.resourceProofRecords!=12u||report.symbolProofUses==0u||probe.symbolEvents!=events)
            return fail("symbol canonical proof/order failed");
        for(auto handle:{0xC101u,0xC102u,0xC103u}) {
            const auto& frozen=DxIfaceTableSourceTestAccess::mapResource(clean,handle);
            if(!frozen.raw||!frozen.rawTypedMatched||frozen.awaitingSymbol||frozen.references.empty())return fail("symbol snapshot missing");
            for(const auto& ref:frozen.references)if(frozen.raw->groups.at(ref.groupIndex).code()!=ref.code)return fail("symbol span differs");
        }
        const auto* ltype=dynamic_cast<const DRW_LType*>(DxIfaceTableSourceTestAccess::resource(clean,0xC102u));
        if(!ltype||ltype->segments.size()!=2u||ltype->length!=1.75||ltype->segments[1].styleHandle.ref!=0xC103u)
            return fail("complex LTYPE normalization changed");
        ++routes;
        for(unsigned family=0;family<2u;++family) {
            const auto count=family?lineFaults.size():styleFaults.size();
            for(std::size_t i=0;i<count;++i) {
                if(family)probe.alterLType=lineFaults[i];else probe.alterTextStyle=styleFaults[i];
                dx_data bad;
                if(!probe.fileImport(path.string(),&bad,false)||dx_iface::tableContentBindingReport(bad).state==S::Bound)
                    return fail("symbol typed mutation escaped family="+std::to_string(family)+" index="+std::to_string(i));
                probe.alterTextStyle={};probe.alterLType={};dx_data restored;
                if(!probe.fileImport(path.string(),&restored,false)||dx_iface::tableContentBindingReport(restored).state!=S::Bound)return fail("symbol typed recovery failed");
                probe.clear();++typed;
            }
        }
        for(const auto& fault:rawFaults) {
            probe.alterSymbolRaw=fault;dx_data bad;
            if(!probe.fileImport(path.string(),&bad,false)||dx_iface::tableContentBindingReport(bad).state==S::Bound)return fail("symbol raw mutation qualified");
            probe.alterSymbolRaw={};dx_data restored;
            if(!probe.fileImport(path.string(),&restored,false)||dx_iface::tableContentBindingReport(restored).state!=S::Bound)return fail("symbol raw recovery failed");
            probe.clear();++raw;
        }
        for(unsigned mode=0;mode<4u;++mode) {
            probe.skipSymbolRaw=mode==0u;probe.duplicateSymbolRaw=mode==1u||mode==2u;
            if(mode==2u)probe.alterSymbolRaw=rawFaults.back();
            probe.symbolOptIn=mode!=3u;dx_data bad;
            if(!probe.fileImport(path.string(),&bad,false)||dx_iface::tableContentBindingReport(bad).state==S::Bound)return fail("missing/duplicate/optout symbol qualified");
            probe.skipSymbolRaw=false;probe.duplicateSymbolRaw=false;probe.alterSymbolRaw={};probe.symbolOptIn=true;
            dx_data restored;if(!probe.fileImport(path.string(),&restored,false)||dx_iface::tableContentBindingReport(restored).state!=S::Bound)return false;
            probe.clear();++callbacks;
        }
        for(unsigned i=0;i<8u;++i) {
            dx_data limited;DxIfaceTableSourceTestAccess::resourceProofLimits(limited,
                i==0?report.resourceProofRecords-1u:i==4?report.resourceProofRecords:100000u,
                i==1?report.resourceProofGroups-1u:i==5?report.resourceProofGroups:4000000u,
                i==2?report.resourceProofBytes-1u:i==6?report.resourceProofBytes:64u*1024u*1024u,
                i==3?report.resourceProofWork-1u:i==7?report.resourceProofWork:4000000u);
            if(!probe.fileImport(path.string(),&limited,false)||dx_iface::tableContentBindingReport(limited).state!=(i<4u?S::CaptureFailed:S::Bound))return fail("symbol cumulative proof cap failed");
            ++boundaries;
        }
        probe.throwSymbolRaw=true;dx_data thrown;bool threw=false;
        try{(void)probe.fileImport(path.string(),&thrown,false);}catch(const std::runtime_error&){threw=true;}
        if(!threw||dx_iface::tableContentBindingReport(thrown).state==S::Bound)return fail("symbol callback throw qualified");
        probe.throwSymbolRaw=false;dx_data retry;
        if(!probe.fileImport(path.string(),&retry,false)||dx_iface::tableContentBindingReport(retry).state!=S::Bound)return fail("symbol exception recovery failed");
        const auto& snapshot=DxIfaceTableSourceTestAccess::mapResource(retry,0xC101u);
        probe.addDxfSymbolSource(*snapshot.model,*snapshot.raw);
        if(dx_iface::tableContentBindingReport(retry).state==S::Bound)return fail("late symbol callback qualified");
        // Direct public-reader transport: default opt-out costs no raw variant
        // nodes. Opt-in is latched for the table and fails atomically on a bad
        // later row or missing ENDTAB, then succeeds on the same reader retry.
        for(const char* family:{"STYLE","LTYPE"}) {
            Tags minimal{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,revision},{0,"ENDSEC"},{0,"SECTION"},{2,"TABLES"}};
            const auto table=slot(good,2,family)-1u;auto end=table;while(!(good[end].code==0&&good[end].value=="ENDTAB"))++end;
            minimal.insert(minimal.end(),good.begin()+static_cast<std::ptrdiff_t>(table),good.begin()+static_cast<std::ptrdiff_t>(end+1u));
            minimal.insert(minimal.end(),{{0,"ENDSEC"},{0,"EOF"}});
            if(!write(path,minimal,binary))return false;
            Probe direct;direct.observeSymbols=true;direct.symbolOptIn=false;dx_data directData;direct.cData=&directData;
            dxfRW codec(path.string().c_str());codec.setDxfReadTypedVariantNodeBudget(2u);
            if(!codec.read(&direct,false)||direct.symbolEvents.empty()||direct.symbolEvents.front().find("typed:")!=0u)return fail("symbol optout charged raw nodes");
            direct.clear();direct.symbolOptIn=true;direct.forceSymbols=true;
            if(codec.read(&direct,false)||!direct.symbolEvents.empty())return fail("symbol raw node exhaustion published batch");
            codec.setDxfReadTypedVariantNodeBudget(100000u);
            if(!codec.read(&direct,false)||direct.symbolEvents.empty()||direct.symbolEvents.front().find("raw:")!=0u)return fail("symbol public-reader retry failed");
            for(unsigned fault=0;fault<2u;++fault) {
                auto bad=minimal;const auto endtab=slot(bad,0,"ENDTAB");
                if(fault==0u)bad[endtab].value="ENDSEC";
                else {
                    const auto row=slot(bad,0,family);
                    const Tags duplicate(bad.begin()+static_cast<std::ptrdiff_t>(row),bad.begin()+static_cast<std::ptrdiff_t>(endtab));
                    bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(endtab),duplicate.begin(),duplicate.end());
                }
                direct.clear();if(!write(path,bad,binary)||codec.read(&direct,false)||!direct.symbolEvents.empty())return fail("malformed symbol batch published callbacks");
                direct.clear();if(!write(path,minimal,binary)||!codec.read(&direct,false)||direct.symbolEvents.empty())return fail("malformed symbol batch retry failed");
                ++transport;
            }
        }
    }
    std::cout<<"symbol source: "<<routes<<" routes; "<<typed<<" typed pairs; "<<raw<<" raw pairs; "<<callbacks
        <<" callback pairs; "<<boundaries<<" exact/below caps; "<<transport<<" malformed batch/retry controls\n";
    return routes==8u&&typed==320u&&raw==88u&&callbacks==32u&&boundaries==64u&&transport==32u;
}

bool complexSymbolBindingTests(const std::filesystem::path& dir) {
    using S=dx_iface::TableBindingState;using R=dx_tableResourceRole;
    const auto fail=[](const std::string& text){std::cerr<<text<<'\n';return false;};
    const auto path=dir/"complex-symbol-binding.dxf";
    const auto element=[](const Tags& tags,int code,bool shape=false) {
        auto i=slot(tags,5,"C102");while(tags[i].code!=49)++i;
        if(shape){++i;while(tags[i].code!=49)++i;}
        for(++i;i<tags.size()&&tags[i].code!=0&&tags[i].code!=49;++i)if(tags[i].code==code)return i;
        throw std::runtime_error("missing complex segment mutation slot");
    };
    const auto removeRow=[](Tags& tags,const char* handle) {
        const auto begin=slot(tags,5,handle)-1u;auto end=begin+1u;while(tags[end].code!=0)++end;
        tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(begin),tags.begin()+static_cast<std::ptrdiff_t>(end));
    };
    std::vector<std::pair<std::function<void(Tags&)>,S>> faults{
        {[element](auto& t){t[element(t,340,true)].value="C199";},S::MissingTarget},
        {[element](auto& t){t[element(t,340,true)].value="0";},S::MissingTarget},
        {[element](auto& t){t[element(t,340,true)].value="F100";},S::WrongFamily},
        {[element](auto& t){t[element(t,340,true)].value="C102";},S::WrongFamily},
        {[element](auto& t){t[element(t,340,true)].value="C101";},S::WrongSymbolKind},
        {[element](auto& t){t[element(t,340)].value="C103";},S::WrongSymbolKind},
        {[removeRow](auto& t){removeRow(t,"C103");},S::MissingTarget},
        {[](auto& t){const auto i=slot(t,5,"C103");t[i+4u].value="not-a-shape";t[i+5u].value="0";},S::WrongSymbolKind},
        {[element](auto& t){t[element(t,75)].value="7";},S::ResourceRawTypedMismatch},
        {[element](auto& t){t[element(t,74)].value="6";},S::ResourceRawTypedMismatch},
        {[](auto& t){const auto i=slot(t,5,"C102");auto end=i;while(t[end].code!=0)++end;
            t.insert(t.begin()+static_cast<std::ptrdiff_t>(end),{341,"C101"});},S::UnknownRole},
        {[](auto& t){t[slot(t,73,"2")].value="3";},S::SourceNotReady}
    };
    for(int code:{340,74,75,50,9})faults.push_back({[element,code](auto& t){
        const auto i=element(t,code);t.erase(t.begin()+static_cast<std::ptrdiff_t>(i));},code==74?S::SourceNotReady:S::ResourceRawTypedMismatch});
    for(int code:{340,74,75,50,9,44,45,46})faults.push_back({[element,code](auto& t){
        const auto i=element(t,code);const auto copy=t[i];t.insert(t.begin()+static_cast<std::ptrdiff_t>(i),copy);},S::ResourceRawTypedMismatch});
    unsigned routes=0u,pairs=0u,variants=0u,callbacks=0u,boundaries=0u,refusals=0u,allocations=0u;
    for(const char* revision:{"AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true}) {
        const auto good=complexSymbolSource(revision);Probe probe;dx_data clean;
        if(!write(path,good,binary)||!probe.fileImport(path.string(),&clean,false))return false;
        const auto report=dx_iface::tableContentBindingReport(clean);
        if(report.state!=S::Bound||report.lineTypeDependencyRows!=1u||report.complexTextStyleBindings!=1u
            ||report.complexShapeStyleBindings!=1u||report.outputEligible)return fail("complex symbol closure failed");
        const auto& frozen=DxIfaceTableSourceTestAccess::mapResource(clean,0xC102u);
        unsigned text=0u,shape=0u;for(const auto& ref:frozen.references) {
            if(ref.kind==R::LTypeTextStyle)++text;
            if(ref.kind==R::LTypeShapeStyle)++shape;
        }
        if(text!=1u||shape!=1u)return fail("complex source roles differ");
        bool allocationSweepComplete=false;
        for(long i=0;i<16;++i) {
            dx_iface::TableContentBindingReport observed;
            {SymbolAllocationFailure fault(i);observed=dx_iface::tableContentBindingReport(clean);}
            if(observed.state==S::Bound){allocationSweepComplete=true;break;}
            if(observed.state!=S::CaptureFailed||observed.outputEligible)return fail("allocation failure escaped bounded inspection");
            if(dx_iface::tableContentBindingReport(clean).state!=S::Bound)return fail("allocation fault poisoned frozen report retry");
            ++allocations;
        }
        if(!allocationSweepComplete)return fail("complex report allocation sweep did not finish");
        ++routes;
        for(std::size_t i=0;i<faults.size();++i) {
            auto bad=good;faults[i].first(bad);dx_data rejected;
            if(!write(path,bad,binary))return false;
            const bool imported=probe.fileImport(path.string(),&rejected,false);
            if(imported==(faults[i].second==S::SourceNotReady))return fail("complex source read outcome changed index="+std::to_string(i));
            const auto failed=dx_iface::tableContentBindingReport(rejected);
            if(failed.state!=faults[i].second||failed.outputEligible)return fail("complex dependency fault index="+std::to_string(i)+" actual="+std::to_string(static_cast<int>(failed.state)));
            dx_data restored;if(!write(path,good,binary)||!probe.fileImport(path.string(),&restored,false)
                ||dx_iface::tableContentBindingReport(restored).state!=S::Bound)return fail("complex same-facade correction failed");
            probe.clear();++pairs;
        }
        for(unsigned mode=0;mode<7u;++mode) {
            auto tags=good;
            if(mode==0u){tags[element(tags,74)].value="3";tags[element(tags,74,true)].value="5";}
            if(mode==1u){tags[element(tags,74)].value="10";tags[element(tags,74,true)].value="12";}
            if(mode==2u)std::swap(tags[element(tags,74,true)],tags[element(tags,340,true)]);
            if(mode==3u)for(int code:{44,45,46}){const auto i=element(tags,code);tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(i));}
            if(mode==4u) {
                const auto end=slot(tags,2,"OBJECTS");for(std::size_t i=0;i<end;++i)
                    if(tags[i].code==340&&tags[i].value=="C102")tags[i].value="0";
            }
            if(mode==5u) {
                const auto begin=slot(tags,5,"C102")-1u;auto end=begin+1u;while(tags[end].code!=0)++end;
                Tags other(tags.begin()+static_cast<std::ptrdiff_t>(begin),tags.begin()+static_cast<std::ptrdiff_t>(end));
                other[slot(other,5,"C102")].value="C104";other[slot(other,2,"TABLE-LTYPE")].value="UNSELECTED-LTYPE";
                other[slot(other,340,"C103")].value="FFFF";
                tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),other.begin(),other.end());
            }
            if(mode==6u) {
                const auto begin=slot(tags,5,"C103");tags[begin+4u].value="named-shape-entry";
            }
            dx_data variant;if(!write(path,tags,binary)||!probe.fileImport(path.string(),&variant,false))return false;
            const auto actual=dx_iface::tableContentBindingReport(variant);
            if(actual.state!=S::Bound||actual.lineTypeDependencyRows!=1u||actual.complexTextStyleBindings!=1u||actual.complexShapeStyleBindings!=1u)
                return fail("complex positive variant failed mode="+std::to_string(mode));
            probe.clear();++variants;
        }
        if(!write(path,good,binary))return false;
        for(unsigned mode=0;mode<3u;++mode) {
            if(mode==0u)probe.skipSymbolTypedHandle=0xC103u;
            if(mode==1u)probe.alterSymbolRaw=[](auto& r){if(r.handle==0xC103u)r.m_version=DRW::AC1015;};
            if(mode==2u)probe.alterTextStyle=[](auto& s){if(s.handle==0xC103u)s.font+="modified";};
            dx_data bad;if(!probe.fileImport(path.string(),&bad,false)||dx_iface::tableContentBindingReport(bad).state==S::Bound)return fail("unproved shape target qualified");
            probe.skipSymbolTypedHandle=0u;probe.alterSymbolRaw={};probe.alterTextStyle={};dx_data retry;
            if(!probe.fileImport(path.string(),&retry,false)||dx_iface::tableContentBindingReport(retry).state!=S::Bound)return fail("shape callback correction failed");
            probe.clear();++callbacks;
        }
        for(bool exact:{false,true}) {
            dx_data limited;DxIfaceTableSourceTestAccess::resourceLimits(limited,100000u,4000000u,64u*1024u*1024u,report.examinedSlots-(exact?0u:1u));
            if(!probe.fileImport(path.string(),&limited,false)||dx_iface::tableContentBindingReport(limited).state!=(exact?S::Bound:S::WorkLimit))return fail("complex worklist bound failed");
            ++boundaries;
        }
        clean.lineTypes.front().segments.clear();clean.textStyles.back().flags=0;
        if(dx_iface::tableContentBindingReport(clean).state!=S::Bound)return fail("live edit altered frozen dependency receipt");
        for(bool strict:{false,true}) {
            const auto output=dir/"complex-refused.dxf";{std::ofstream out(output);out<<"preserve";}
            dx_iface exporter;if(exporter.fileExport(output.string(),clean.sourceVersion,binary,&clean,false,strict))return fail("complex proof promoted output");
            std::ifstream old(output);if(std::string(std::istreambuf_iterator<char>(old),{})!="preserve")return fail("complex refusal replaced destination");
            dx_data plain;dx_iface author;author.cData=&plain;author.currentBlock=plain.mBlock;
            DRW_Line line;line.basePoint={1,2,3};line.secPoint={4,5,6};author.addLine(line);
            if(!exporter.fileExport(output.string(),clean.sourceVersion,binary,&plain,false,strict))return fail("complex refusal poisoned exporter retry");
            dx_data readback;dx_iface reader;
            if(!reader.fileImport(output.string(),&readback,false)||readback.mBlock->ent.size()!=1u)return fail("complex retry readback failed");
            const auto* actual=dynamic_cast<const DRW_Line*>(readback.mBlock->ent.front());
            if(!actual||actual->basePoint.z!=3||actual->secPoint.z!=6)return fail("complex retry changed coordinates");
            ++refusals;
        }
    }
    std::cout<<"complex LTYPE bindings: "<<routes<<" routes; "<<faults.size()<<" fault kinds; "<<pairs<<" fault/recovery pairs; "
        <<variants<<" variants; "<<callbacks<<" callback corrections; "<<boundaries<<" work boundaries; "
        <<allocations<<" allocation refusal/retries; "<<refusals<<" atomic refusal/retry readbacks\n";
    return routes==8u&&pairs==faults.size()*routes&&variants==56u&&callbacks==24u&&boundaries==16u&&allocations>=16u&&refusals==16u;
}

// Authored from scratch. FIELD children cover all retained payload categories;
// DATALINK connection text is inert and must never cause external access.
Tags auxiliaryResourceSource(const std::string& revision) {
    auto tags=complexSymbolSource(revision);
    const auto replace=[&](const char* handle,const Tags& body) {
        const auto begin=slot(tags,5,handle)-1u;auto end=begin+1u;while(tags[end].code!=0)++end;
        tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(begin),tags.begin()+static_cast<std::ptrdiff_t>(end));
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(begin),body.begin(),body.end());
    };
    Tags field{{0,"FIELD"},{5,"F100"},{102,"{FIELD_META"},{300,"common-é"},{102,"}"},
        {102,"{ACAD_REACTORS"},{330,"A0"},{102,"}"},{102,"{ACAD_XDICTIONARY"},{360,"C110"},{102,"}"},
        {330,"A0"},{100,"AcDbObject"},{100,"AcDbField"},{1,"AcExpr"},{2,"first-é"},{3," + tail ^J code"},
        {4,"outer-format"},{90,"2"},{360,"F101"},{360,"F102"},{97,"2"},{331,"B100"},{331,"C101"},
        {91,"3"},{92,"4"},{94,"5"},{95,"6"},{96,"7"},{300,"status ^J message-é"},{93,"8"}};
    const int types[]{0,1,2,4,8,16,32,64};
    for(unsigned i=0;i<8u;++i) {
        field.insert(field.end(),{{6,"key-"+std::to_string(i)},{93,"0"},{90,std::to_string(types[i])}});
        if(i==0u||i==1u)field.push_back({91,i?"-9":"11"});
        if(i==2u)field.push_back({140,"2.25"});
        if(i==3u)field.insert(field.end(),{{1,"first-é"},{2," + second ^J chunk"}});
        if(i==4u)field.insert(field.end(),{{92,"4"},{310,"00FF"},{310,"1080"}});
        if(i==5u)field.insert(field.end(),{{92,"16"},{11,"1.25"},{21,"-2.5"}});
        if(i==6u)field.insert(field.end(),{{92,"24"},{11,"1"},{21,"2"},{31,"3"}});
        if(i==7u)field.push_back({330,"C101"});
        field.insert(field.end(),{{94,"0"},{300,"fmt-"+std::to_string(i)},{302,"cache-é"},{304,"ACVALUE_END"}});
    }
    field.insert(field.end(),{{7,"ACFD_FIELD_VALUE"},{93,"0"},{90,"64"},{330,"F001"},{94,"0"},
        {300,"object-format"},{302,"cached-object"},{304,"ACVALUE_END"},
        {301,"display-é"},{9," + tail ^J display"},{98,"31"},
        {1001,"FIELD_APP"},{1000,"field xdata-é"},{1003,"FIELD_LAYER"},{1005,"C101"}});
    replace("F100",field);
    const Tags dataLink{{0,"DATALINK"},{5,"F001"},{102,"{ACAD_REACTORS"},{330,"C"},{102,"}"},
        {330,"C"},{100,"AcDbObject"},{100,"AcDbDataLink"},{1,"EXCEL"},{300,"link description-é"},
        {301,"tooltip ^J text"},{302,"never-open://authored-control.xlsx"},{90,"7"},{91,"511"},{92,"-3"},
        {170,"2026"},{171,"10"},{172,"25"},{173,"23"},{174,"59"},{175,"1"},{176,"250"},{177,"3"},
        {93,"19"},{304,"ready ^J status-é"},{94,"2"},{305,"CUSTOMDATA"},
        {330,"F100"},{304,"field ref-é"},{330,"C101"},{304,"style ref-é"},{309,"DATAMAP_END"},{360,"F200"},
        {1001,"LINK_APP"},{1000,"link xdata-é"},{1003,"LINK_LAYER"},{1005,"C101"}};
    replace("F001",dataLink);
    return tags;
}

Tags spineResourceSource(const std::string& revision) {
    auto tags=auxiliaryResourceSource(revision);
    const auto check=slot(tags,300,"CHECKSUM");
    tags[check+3u]={90,"64"};tags[check+4u]={330,"E200"};
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(slot(tags,5,"B1")-1u),{{3,"spine-é"},{350,"E100"}});
    const Tags extra={{0,"DICTIONARY"},{5,"E100"},{102,"{ACAD_REACTORS"},{330,"C101"},{102,"}"},{330,"C"},
        {100,"AcDbDictionary"},{280,"0"},{281,"2"},{3,"data-é"},{350,"E200"},{3,"style-é"},{360,"C101"},
        {1001,"SPINE_APP"},{1000,"dictionary xdata-é"},{1003,"SPINE_LAYER"},{1005,"C101"},
        {0,"XRECORD"},{5,"E200"},{102,"{ACAD_REACTORS"},{330,"E100"},{102,"}"},
        {102,"{ACAD_XDICTIONARY"},{360,"E100"},{102,"}"},{330,"E100"},{100,"AcDbXrecord"},{280,"2"},
        {1,"arbitrary ^J text-é"},{100,"application class string"},{102,"{ACAD_REACTORS"},{330,"C101"},{102,"}"},
        {280,"4"},{40,"-0.0"},{160,"9007199254740993"},{10,"1"},{20,"2"},{30,"3"},{310,"00FF1080"},
        {320,"C101"},{329,"0"},{339,"0"},{340,"C101"},{349,"0"},{350,"C101"},{359,"0"},{360,"C101"},{369,"0"},
        {1001,"SPINE_APP"},{1000,"xrecord xdata-é"},{1003,"SPINE_LAYER"},{1005,"C101"}};
    tags.insert(tags.end()-2,extra.begin(),extra.end());return tags;
}

bool spineSourceProofTests(const std::filesystem::path& dir) {
    using S=dx_iface::TableBindingState;using R=dx_tableResourceRole;
    const auto path=dir/"spine-proof.dxf";
    const auto fail=[](const std::string& message){std::cerr<<message<<'\n';return false;};
    std::vector<std::function<void(DRW_Dictionary&)>> dictionaryFaults={
        [](auto& r){++r.cloning;},[](auto& r){r.hardOwner=1;},[](auto& r){r.countCap.emplace();},
        [](auto& r){r.m_entries[0].m_name+="changed";},[](auto& r){++r.m_entries[0].m_handle;},
        [](auto& r){r.m_entries[0].m_dxfHandleCode=DRW_Dictionary::Entry::DxfHandleCode::HardOwner;},
        [](auto& r){r.m_entries.pop_back();},[](auto& r){std::swap(r.m_entries[0],r.m_entries[1]);},
        [](auto& r){++r.flags;},[](auto& r){r.name="invented";},[](auto& r){++r.xDictHandle;},
        [](auto& r){r.reactorHandles.push_back(0xC101u);},[](auto& r){r.setDwgCommonLinkTailValidated(true);},
        [](auto& r){r.setDwgCommonObjectState(r.reactorCount(),r.extensionDictionaryFlag(),true);},
        [](auto& r){r.extData[0]->addString(1001,"different-app");}
    };
    dictionaryFaults.push_back([](auto& r){DrwObjectEncodeTestAccess::pendingDictionaryEntry(r);});
    std::vector<std::function<void(DRW_XRecord&)>> xrecordFaults={
        [](auto& r){++r.m_cloning;},[](auto& r){r.m_values[0].addString(1,"different");},
        [](auto& r){r.m_values.pop_back();},[](auto& r){r.m_handleValues[0].first=340;},
        [](auto& r){++r.m_handleValues[0].second;},[](auto& r){r.m_handleValues.pop_back();},
        [](auto& r){r.m_dataEntries[0].addString(1,"different");},[](auto& r){r.m_dataEntries.pop_back();},
        [](auto& r){std::swap(r.m_dataEntries[0],r.m_dataEntries[1]);},
        [](auto& r){for(auto& v:r.m_values)if(v.type()==DRW_Variant::BINARY){v.addBinary(v.code(),{1,2});break;}},
        [](auto& r){for(auto& v:r.m_dataEntries)if(v.type()==DRW_Variant::COORD){v.setCoordZ(4.0);break;}},
        [](auto& r){for(auto& v:r.m_dataEntries)if(v.code()==40){v.addDouble(40,0.0);break;}},
        [](auto& r){for(auto& v:r.m_dataEntries)if(v.code()==160){v.addInt64(160,9007199254740992LL);break;}},
        [](auto& r){r.m_rawData.push_back(17u);},[](auto& r){r.m_rawDataValid=true;},
        [](auto& r){++r.flags;},[](auto& r){r.name="invented";},[](auto& r){++r.xDictHandle;},
        [](auto& r){r.reactorHandles.push_back(0xC101u);},[](auto& r){r.setDwgCommonLinkTailValidated(true);},
        [](auto& r){r.extData[0]->addString(1001,"different-app");}
    };
    const std::vector<std::function<void(DRW_RawDxfObject&)>> rawFaults={
        [](auto& r){r.m_version=DRW::AC1015;},[](auto& r){r.name="WRONG_FAMILY";},
        [](auto& r){++r.parentHandle;},[](auto& r){r.groups.front().addString(5,"FFFF");},
        [](auto& r){if(r.hasRawValues)r.rawValues[0]="inconsistent";else r.hasRawValues=true;},
        [](auto& r){r.groups.emplace_back(281,std::int32_t{2});if(r.hasRawValues)r.rawValues.emplace_back("2");},
        [](auto& r){r.groups.front().addCoord(5,DRW_Coord());},
        [](auto& r){r.groups.front().setLayerRefName("not an original scalar");}
    };
    unsigned routes=0u,typed=0u,callbacks=0u,limits=0u,wire=0u,raw=0u,allocations=0u;
    for(const char* revision:{"AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true}) {
        const auto tags=spineResourceSource(revision);if(!write(path,tags,binary))return false;
        Probe probe;dx_data clean;if(!probe.fileImport(path.string(),&clean,false))return fail("spine import failed");
        const auto report=dx_iface::tableContentBindingReport(clean);
        if(report.state!=S::Bound||!report.dictionaryProofUses||!report.xRecordProofUses||probe.rootSources!=1u)
            return fail("spine canonical binding failed state="+std::to_string(static_cast<int>(report.state)));
        const auto matched=[&](const dx_data& data,std::uint32_t handle) {
            const auto& rows=DxIfaceTableSourceTestAccess::resources(data);const auto it=rows.find(handle);
            return it!=rows.end()&&it->second.rawTypedMatched&&!it->second.rawTypedMismatch;
        };
        for(const auto handle:{0xCu,0xB1u,0xB0u,0xC110u,0xE100u,0xE200u})if(!matched(clean,handle))return fail("missing spine proof");
        const auto& resource=DxIfaceTableSourceTestAccess::mapResource(clean,0xE200u);
        std::array<unsigned,26> roles{};
        for(const auto& ref:resource.references) {
            const auto& tag=resource.raw->groups.at(ref.groupIndex);
            if(tag.code()!=ref.code)return fail("spine reference position differs");
            ++roles.at(static_cast<unsigned>(ref.kind));
        }
        if(roles[static_cast<unsigned>(R::Reactor)]!=1u||roles[static_cast<unsigned>(R::ExtensionDictionary)]!=1u
            ||roles[static_cast<unsigned>(R::XRecordSoftPointer)]!=2u||roles[static_cast<unsigned>(R::XRecordHandle)]!=2u
            ||roles[static_cast<unsigned>(R::XRecordHardPointer)]!=2u||roles[static_cast<unsigned>(R::XRecordSoftOwner)]!=2u
            ||roles[static_cast<unsigned>(R::XRecordHardOwner)]!=2u||roles[static_cast<unsigned>(R::Unknown)])return fail("spine role census differs");
        ++routes;
        for(const auto handle:{0xC110u,0xB0u}) {
            if(handle==0xC110u)probe.alterDictionary=[](auto& r){if(r.handle==0xC110u)++r.cloning;};
            else probe.alterXRecord=[](auto& r){if(r.handle==0xB0u)++r.m_cloning;};
            dx_data mismatch;
            if(!probe.fileImport(path.string(),&mismatch,false)||dx_iface::tableContentBindingReport(mismatch).state!=S::ResourceRawTypedMismatch)
                return fail("selected dictionary/wrapper payload lacked binding proof");
            probe.alterDictionary={};probe.alterXRecord={};
        }
        for(unsigned family=0u;family<2u;++family)for(std::size_t i=0;i<(family?xrecordFaults.size():dictionaryFaults.size());++i) {
            if(family)probe.alterXRecord=[&,i](auto& r){if(r.handle==0xE200u)xrecordFaults[i](r);};
            else probe.alterDictionary=[&,i](auto& r){if(r.handle==0xE100u)dictionaryFaults[i](r);};
            dx_data bad;if(!probe.fileImport(path.string(),&bad,false)||matched(bad,family?0xE200u:0xE100u))return fail("typed spine mutation escaped "+std::to_string(family)+"/"+std::to_string(i));
            const auto& row=DxIfaceTableSourceTestAccess::mapResource(bad,family?0xE200u:0xE100u);
            if(!row.references.empty())return fail("partial spine receipt");
            probe.alterDictionary={};probe.alterXRecord={};dx_data restored;
            if(!probe.fileImport(path.string(),&restored,false)||!matched(restored,0xE100u)||!matched(restored,0xE200u)
                ||dx_iface::tableContentBindingReport(restored).state!=S::Bound)return fail("spine corrected retry failed");
            ++typed;
        }
        for(const auto handle:{0xCu,0xB1u,0xB0u,0xC110u,0xE100u,0xE200u})for(unsigned mode=0u;mode<4u;++mode) {
            probe.skipSpineRaw=mode==0u?handle:0u;probe.duplicateSpineRaw=mode==1u?handle:0u;
            probe.skipSpineTyped=mode==2u?handle:0u;probe.duplicateSpineTyped=mode==3u?handle:0u;
            dx_data bad;if(!probe.fileImport(path.string(),&bad,false))return false;
            const auto failure=dx_iface::tableContentBindingReport(bad);
            if(mode==0u&&matched(bad,handle))return fail("missing source callback qualified handle="+std::to_string(handle));
            if(mode==1u&&failure.state==S::Bound)return fail("duplicate source callback qualified");
            if((handle==0xB1u||handle==0xB0u||handle==0xC110u||handle==0xE200u)&&failure.state==S::Bound)return fail("selected spine callback gap bound");
            probe.skipSpineRaw=probe.duplicateSpineRaw=probe.skipSpineTyped=probe.duplicateSpineTyped=0u;
            dx_data restored;if(!probe.fileImport(path.string(),&restored,false)||dx_iface::tableContentBindingReport(restored).state!=S::Bound)return false;
            ++callbacks;
        }
        probe.wrongSpineThenOriginal=true;dx_data collision;
        if(!probe.fileImport(path.string(),&collision,false)||dx_iface::tableContentBindingReport(collision).state!=S::CaptureFailed)return fail("mismatched-family then original repaired proof");
        probe.wrongSpineThenOriginal=false;
        for(const auto handle:{0xC110u,0xE200u})for(const auto& fault:rawFaults) {
            probe.alterSpineRaw=[handle,fault](auto& r){if(r.handle==handle)fault(r);};
            dx_data bad;
            if(!probe.fileImport(path.string(),&bad,false)||matched(bad,handle)||dx_iface::tableContentBindingReport(bad).state==S::Bound)
                return fail("raw spine mutation qualified");
            const auto& row=DxIfaceTableSourceTestAccess::mapResource(bad,handle);
            if(!row.references.empty())return fail("raw spine mutation published partial references");
            probe.alterSpineRaw={};dx_data restored;
            if(!probe.fileImport(path.string(),&restored,false)||dx_iface::tableContentBindingReport(restored).state!=S::Bound)return false;
            ++raw;
        }
        for(long schedule=0;schedule<6;++schedule) {
            probe.rootSourceAllocation=schedule;dx_data interrupted;
            if(!probe.fileImport(path.string(),&interrupted,false))return fail("source allocation refused ordinary import");
            if(!matched(interrupted,0xCu)&&dx_iface::tableContentBindingReport(interrupted).state==S::Bound)return fail("partial root allocation qualified");
            probe.rootSourceAllocation=-1;dx_data restored;
            if(!probe.fileImport(path.string(),&restored,false)||!matched(restored,0xCu)||dx_iface::tableContentBindingReport(restored).state!=S::Bound)return false;
            ++allocations;
        }
        for(unsigned i=0u;i<8u;++i) {
            dx_data bounded;DxIfaceTableSourceTestAccess::resourceProofLimits(bounded,
                i==0?report.resourceProofRecords-1u:i==4?report.resourceProofRecords:100000u,
                i==1?report.resourceProofGroups-1u:i==5?report.resourceProofGroups:4000000u,
                i==2?report.resourceProofBytes-1u:i==6?report.resourceProofBytes:64u*1024u*1024u,
                i==3?report.resourceProofWork-1u:i==7?report.resourceProofWork:4000000u);
            if(!probe.fileImport(path.string(),&bounded,false)||dx_iface::tableContentBindingReport(bounded).state!=(i<4u?S::CaptureFailed:S::Bound))return fail("spine aggregate bound mismatch");
            ++limits;
        }
        for(unsigned mode=0u;mode<4u;++mode) {
            auto bad=tags;
            const auto key=slot(bad,3,"data-é");
            if(mode==0u)bad[key+1u].code=340;
            if(mode==1u)bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(key),{100,"AcDbDictionary"});
            if(mode==2u)bad[slot(bad,160,"9007199254740993")].value="9007199254740992";
            if(mode==3u)bad[slot(bad,160,"9007199254740993")+2u].code=21;
            if(!write(path,bad,binary))return false;
            dx_data data;const bool ok=probe.fileImport(path.string(),&data,false);
            if(mode==2u) {if(!ok||!matched(data,0xE200u))return fail("valid application scalar change rejected");}
            else if(ok&&matched(data,mode<2u?0xE100u:0xE200u))return fail("incomplete/duplicate spine source qualified");
            ++wire;
        }
        if(!write(path,tags,binary))return false;
        Probe optOut;optOut.rootOptIn=false;dx_data noRoot;
        if(!optOut.fileImport(path.string(),&noRoot,false)||optOut.rootSources||matched(noRoot,0xCu))return fail("root opt-out copied source");
        probe.throwRootSource=true;dx_data interrupted;bool threw=false;
        try{(void)probe.fileImport(path.string(),&interrupted,false);}catch(const std::runtime_error&){threw=true;}
        if(!threw||dx_iface::tableContentBindingReport(interrupted).state==S::Bound)return fail("root source exception qualified");
        probe.throwRootSource=false;dx_data recovered;
        if(!probe.fileImport(path.string(),&recovered,false)||dx_iface::tableContentBindingReport(recovered).state!=S::Bound)return false;
    }
    std::cout<<"spine source proof: "<<routes<<" routes; "<<typed<<" typed mismatch/recovery pairs; "<<callbacks
        <<" callback corrections; "<<raw<<" raw mismatch/recovery pairs; "<<allocations<<" allocation schedules; "
        <<limits<<" exact/below aggregate limits; "<<wire<<" wire controls; root opt-out/throw/retry and mismatched-family recovery refusal\n";
    return routes==8u&&typed==(dictionaryFaults.size()+xrecordFaults.size())*routes&&callbacks==192u&&raw==128u&&allocations==48u&&limits==64u&&wire==32u;
}

bool auxiliarySourceProofTests(const std::filesystem::path& dir) {
    using S=dx_iface::TableBindingState;using R=dx_tableResourceRole;
    const auto fail=[](const std::string& text){std::cerr<<text<<'\n';return false;};
    const auto path=dir/"auxiliary-source-proof.dxf";
    const auto rowSlot=[](const Tags& tags,const char* handle,int code,std::string_view value={}) {
        for(auto i=slot(tags,5,handle);i<tags.size()&&tags[i].code!=0;++i)
            if(tags[i].code==code&&(value.empty()||tags[i].value==value))return i;
        throw std::runtime_error("missing auxiliary row mutation slot");
    };
    std::vector<std::function<void(DRW_Field&)>> fieldFaults{
        [](auto& f){f.m_evaluatorId+="changed";},[](auto& f){f.m_fieldCode+="changed";},[](auto& f){f.m_formatString+="changed";},
        [](auto& f){++f.m_evaluationOptionFlags;},[](auto& f){++f.m_filingOptionFlags;},[](auto& f){++f.m_fieldStateFlags;},
        [](auto& f){++f.m_evaluationStatusFlags;},[](auto& f){++f.m_evaluationErrorCode;},[](auto& f){f.m_evaluationErrorMessage+="changed";},
        [](auto& f){f.m_valueString+="changed";},[](auto& f){++f.m_valueStringLength;},
        [](auto& f){++f.m_childHandles[0];},[](auto& f){f.m_childHandles.pop_back();},
        [](auto& f){++f.m_objectHandles[0];},[](auto& f){f.m_objectHandles.pop_back();},[](auto& f){f.m_childValues.pop_back();},
        [](auto& f){++f.flags;},[](auto& f){f.name="injected-name";},[](auto& f){++f.xDictHandle;},
        [](auto& f){f.reactorHandles.push_back(0xC101u);},[](auto& f){f.setDwgCommonLinkTailValidated(true);},
        [](auto& f){f.setDwgCommonObjectState(f.reactorCount(),f.extensionDictionaryFlag(),true);},
        [](auto& f){f.appData.front().push_back(DRW_Variant(300,"injected"));},
        [](auto& f){f.extData.front()->addString(1001,"different-app");}
    };
    const std::vector<std::function<void(DRW_CadValue&)>> valueFaults{
        [](auto& v){++v.m_formatFlags;},[](auto& v){++v.m_dataType;},[](auto& v){++v.m_dataSize;},
        [](auto& v){++v.m_unitType;},[](auto& v){v.m_formatString+="changed";},[](auto& v){v.m_valueString+="changed";},
        [](auto& v){++v.m_handle;},[](auto& v){v.m_rawData.push_back(17u);},
        [](auto& v){v.m_value.addInt(331,99);},[](auto& v){DRW_DwgCmColor color;color.flags=1u;v.m_value.setDwgCmColor(color);}
    };
    for(const auto& fault:valueFaults) {
        fieldFaults.push_back([fault](auto& f){fault(f.m_value);});
        fieldFaults.push_back([fault](auto& f){fault(f.m_childValues[0].m_value);});
    }
    for(unsigned i=0;i<8u;++i)fieldFaults.push_back([i](auto& f){f.m_childValues[i].m_key+="changed";});
    fieldFaults.push_back([](auto& f){f.m_childValues[0].m_value.m_value.addInt(91,12);});
    fieldFaults.push_back([](auto& f){f.m_childValues[1].m_value.m_value.addInt(91,-10);});
    fieldFaults.push_back([](auto& f){f.m_childValues[2].m_value.m_value.addDouble(140,-0.0);});
    fieldFaults.push_back([](auto& f){f.m_childValues[3].m_value.m_value.addString(1,"changed");});
    fieldFaults.push_back([](auto& f){auto& v=f.m_childValues[3].m_value.m_value;v.setLayerRefName(*v.content.s);});
    fieldFaults.push_back([](auto& f){f.m_childValues[4].m_value.m_rawData[0]^=1u;});
    fieldFaults.push_back([](auto& f){f.m_childValues[4].m_value.m_value.addBinary(310,{1,2,3,4});});
    fieldFaults.push_back([](auto& f){f.m_childValues[5].m_value.m_value.setCoordZ(-0.0);});
    fieldFaults.push_back([](auto& f){f.m_childValues[6].m_value.m_value.setCoordY(4.0);});
    fieldFaults.push_back([](auto& f){++f.m_childValues[7].m_value.m_handle;});
    const std::vector<std::function<void(DRW_DataLink&)>> linkFaults{
        [](auto& l){l.m_dataAdapter+="changed";},[](auto& l){l.m_description+="changed";},[](auto& l){l.m_tooltip+="changed";},
        [](auto& l){l.m_connectionString+="changed";},[](auto& l){++l.m_option;},[](auto& l){++l.m_updateOption;},
        [](auto& l){++l.m_value92;},[](auto& l){++l.m_year;},[](auto& l){++l.m_month;},[](auto& l){++l.m_day;},
        [](auto& l){++l.m_hour;},[](auto& l){++l.m_minute;},[](auto& l){++l.m_seconds;},[](auto& l){++l.m_milliseconds;},
        [](auto& l){++l.m_pathOption;},[](auto& l){++l.m_value93;},[](auto& l){l.m_updateStatus+="changed";},
        [](auto& l){++l.m_customDataCount;},[](auto& l){l.m_customData.pop_back();},[](auto& l){++l.m_hardOwnerHandle;},
        [](auto& l){++l.m_customData[0].m_targetHandle;},[](auto& l){l.m_customData[1].m_text+="changed";},
        [](auto& l){++l.m_customData[0].m_dxfRetainedTextBytes;},[](auto& l){++l.flags;},
        [](auto& l){l.name="injected-name";},[](auto& l){l.reactorHandles.push_back(0xC101u);},
        [](auto& l){l.setDwgCommonLinkTailValidated(true);},[](auto& l){l.extData.front()->addString(1001,"different-app");}
    };
    const std::vector<std::function<void(DRW_RawDxfObject&)>> rawFaults{
        [](auto& r){r.m_version=DRW::AC1015;},[](auto& r){r.name="UNKNOWN_RESOURCE";},
        [](auto& r){++r.handle;},[](auto& r){++r.parentHandle;},[](auto& r){r.groups[0].addString(5,"FFFF");},
        [](auto& r){r.groups.emplace_back(100,"UnknownSubclass");r.rawValues.emplace_back("UnknownSubclass");},
        [](auto& r){if(r.hasRawValues)r.rawValues[0]="wrong scalar";else r.hasRawValues=true;},
        [](auto& r){for(std::size_t i=0;i<r.groups.size();++i)if(r.groups[i].code()==1){r.groups.erase(r.groups.begin()+static_cast<std::ptrdiff_t>(i));r.rawValues.erase(r.rawValues.begin()+static_cast<std::ptrdiff_t>(i));break;}},
        [](auto& r){r.groups.emplace_back(10,DRW_Coord(1,2,3));r.rawValues.emplace_back("1");},
        [](auto& r){r.groups.front().setLayerRefName("not a raw scalar");}
    };
    unsigned routes=0u,typed=0u,raw=0u,callbacks=0u,boundaries=0u,refusals=0u,wire=0u,variants=0u,publicBudgets=0u;
    for(const char* revision:{"AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true}) {
        const auto good=auxiliaryResourceSource(revision);Probe probe;dx_data clean;
        if(!write(path,good,binary)||!probe.fileImport(path.string(),&clean,false))return fail("rich auxiliary source import failed");
        const auto report=dx_iface::tableContentBindingReport(clean);
        if(report.state!=S::Bound||report.fieldProofUses==0u||report.dataLinkProofUses==0u||report.resourceProofRecords!=12u)return fail("auxiliary canonical proof failed");
        const auto& field=DxIfaceTableSourceTestAccess::mapResource(clean,0xF100u);
        const auto& link=DxIfaceTableSourceTestAccess::mapResource(clean,0xF001u);
        if(!field.rawTypedMatched||!link.rawTypedMatched)return false;
        std::array<unsigned,19> roles{};
        for(const auto* resource:{&field,&link})for(const auto& ref:resource->references) {
            if(resource->raw->groups.at(ref.groupIndex).code()!=ref.code)return fail("auxiliary source position differs");
            if(isDxfHandleCode(ref.code)) {
                const auto& scalar=resource->raw->groups[ref.groupIndex];
                if(scalar.type()!=DRW_Variant::STRING||std::stoull(*scalar.content.s,nullptr,16)!=ref.value)return fail("auxiliary source handle differs");
            }
            ++roles.at(static_cast<unsigned>(ref.kind));
        }
        if(roles[static_cast<unsigned>(R::FieldChild)]!=2u||roles[static_cast<unsigned>(R::FieldObject)]!=2u
            ||roles[static_cast<unsigned>(R::FieldValueObject)]!=2u||roles[static_cast<unsigned>(R::DataLinkCustomTarget)]!=2u
            ||roles[static_cast<unsigned>(R::DataLinkHardOwner)]!=1u)return fail("auxiliary reference census differs");
        ++routes;
        for(unsigned family=0;family<2u;++family) {
            const auto count=family?linkFaults.size():fieldFaults.size();
            for(std::size_t i=0;i<count;++i) {
                if(family)probe.alterDataLink=linkFaults[i];else probe.alterField=fieldFaults[i];
                dx_data bad;if(!probe.fileImport(path.string(),&bad,false)||dx_iface::tableContentBindingReport(bad).state==S::Bound)
                    return fail("auxiliary typed mutation escaped family="+std::to_string(family)+" index="+std::to_string(i));
                const auto& resources=DxIfaceTableSourceTestAccess::resources(bad);
                const auto found=resources.find(family?0xF001u:0xF100u);
                if(found!=resources.end()&&(found->second.rawTypedMatched||!found->second.references.empty()))return fail("partial auxiliary typed proof published");
                probe.alterField={};probe.alterDataLink={};dx_data restored;
                if(!probe.fileImport(path.string(),&restored,false)||dx_iface::tableContentBindingReport(restored).state!=S::Bound)return fail("auxiliary typed correction failed");
                probe.clear();++typed;
            }
            const std::uint32_t handle=family?0xF001u:0xF100u;
            for(const auto& fault:rawFaults) {
                probe.alterAuxRaw=[fault,handle](auto& r){if(r.handle==handle)fault(r);};dx_data bad;
                if(!probe.fileImport(path.string(),&bad,false)||dx_iface::tableContentBindingReport(bad).state==S::Bound)return fail("auxiliary raw fault qualified");
                const auto& resource=DxIfaceTableSourceTestAccess::mapResource(bad,handle);
                if(resource.rawTypedMatched||!resource.references.empty())return fail("partial auxiliary raw proof published");
                probe.alterAuxRaw={};dx_data restored;
                if(!probe.fileImport(path.string(),&restored,false)||dx_iface::tableContentBindingReport(restored).state!=S::Bound)return fail("auxiliary raw correction failed");
                probe.clear();++raw;
            }
            for(unsigned mode=0;mode<5u;++mode) {
                probe.skipAuxRaw=mode==0u?handle:0u;probe.duplicateAuxRaw=(mode==1u||mode==4u)?handle:0u;
                probe.skipAuxTyped=mode==2u?handle:0u;probe.duplicateAuxTyped=mode==3u?handle:0u;
                if(mode==4u)probe.alterAuxRaw=[handle](auto& r){if(r.handle==handle)r.groups.front().setLayerRefName("not raw");};
                dx_data bad;if(!probe.fileImport(path.string(),&bad,false)||dx_iface::tableContentBindingReport(bad).state==S::Bound)return fail("auxiliary callback gap qualified");
                const auto& original=family?link:field;probe.addRawDxfObject(*original.raw);
                if(dx_iface::tableContentBindingReport(bad).state==S::Bound)return fail("late raw callback repaired source evidence");
                probe.skipAuxRaw=probe.duplicateAuxRaw=probe.skipAuxTyped=probe.duplicateAuxTyped=0u;probe.alterAuxRaw={};
                dx_data restored;if(!probe.fileImport(path.string(),&restored,false)||dx_iface::tableContentBindingReport(restored).state!=S::Bound)return false;
                probe.clear();++callbacks;
            }
            probe.throwAuxTyped=handle;dx_data interrupted;bool threw=false;
            try{(void)probe.fileImport(path.string(),&interrupted,false);}catch(const std::runtime_error&){threw=true;}
            if(!threw||dx_iface::tableContentBindingReport(interrupted).state==S::Bound)return fail("auxiliary callback exception qualified");
            probe.throwAuxTyped=0u;dx_data corrected;
            if(!probe.fileImport(path.string(),&corrected,false)||dx_iface::tableContentBindingReport(corrected).state!=S::Bound)return fail("auxiliary exception recovery failed");
            probe.clear();++callbacks;
        }
        for(unsigned mode=0;mode<8u;++mode) {
            auto bad=good;const bool fieldFault=mode>=4u;const auto start=slot(bad,5,fieldFault?"F100":"F001");
            auto end=start;while(bad[end].code!=0)++end;
            if(mode==0u)bad.erase(bad.begin()+static_cast<std::ptrdiff_t>(rowSlot(bad,"F001",309,"DATAMAP_END")));
            if(mode==1u)bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(rowSlot(bad,"F001",309,"DATAMAP_END")),{330,"C102"});
            if(mode==2u)for(auto i=start;i<end;++i)if(bad[i].code==94){bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(i),{94,"2"});break;}
            if(mode==3u)bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(end),{1,"ignored second adapter"});
            if(mode==4u)bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(end),{1,"duplicate evaluator"});
            if(mode==5u)for(auto i=start;i<end;++i)if(bad[i].code==2){bad.erase(bad.begin()+static_cast<std::ptrdiff_t>(i));break;}
            if(mode==6u||mode==7u) {
                const auto tail=rowSlot(bad,mode==6u?"F100":"F001",1001);
                bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(tail),{341,"C101"});
            }
            dx_data rejected;if(!write(path,bad,binary)||!probe.fileImport(path.string(),&rejected,false)
                ||dx_iface::tableContentBindingReport(rejected).state==S::Bound)return fail("auxiliary wire profile fault qualified mode="+std::to_string(mode));
            if(dx_iface::tableSourceReport(rejected).state!=dx_iface::TableSourceState::StructurallyComplete
                ||(mode>=6u&&dx_iface::tableContentBindingReport(rejected).state!=S::UnknownRole))return fail("auxiliary wire fault affected the wrong source scope");
            dx_data retry;if(!write(path,good,binary)||!probe.fileImport(path.string(),&retry,false)||dx_iface::tableContentBindingReport(retry).state!=S::Bound)return fail("auxiliary wire recovery failed");
            probe.clear();++wire;
        }
        for(unsigned mode=0;mode<5u;++mode) {
            auto tags=good;
            if(mode==0u)tags[slot(tags,0,"FIELD")].value="ACAD_FIELD";
            if(mode==4u)tags[slot(tags,0,"FIELD")].value="ACDBFIELD";
            if(mode==1u) {
                const auto start=slot(tags,5,"F100");auto end=start;while(tags[end].code!=0)++end;
                for(auto i=end;i-->start;)if((tags[i].code==93&&tags[i].value=="0")||(tags[i].code==304&&tags[i].value=="ACVALUE_END"))tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(i));
            }
            if(mode==2u) {
                auto i=rowSlot(tags,"F001",305,"CUSTOMDATA");tags[i]={1,"DATAMAP_BEGIN"};
                i=rowSlot(tags,"F001",309,"DATAMAP_END");tags[i]={1,"DATAMAP_END"};
            }
            if(mode==3u) {
                const auto i=rowSlot(tags,"F100",100,"AcDbObject");tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(i));
            }
            dx_data variant;if(!write(path,tags,binary)||!probe.fileImport(path.string(),&variant,false)
                ||dx_iface::tableContentBindingReport(variant).state!=S::Bound)return fail("auxiliary compatibility variant failed mode="+std::to_string(mode));
            probe.clear();++variants;
        }
        if(!write(path,good,binary))return false;
        for(unsigned i=0;i<8u;++i) {
            dx_data limited;DxIfaceTableSourceTestAccess::resourceProofLimits(limited,
                i==0?report.resourceProofRecords-1u:i==4?report.resourceProofRecords:100000u,
                i==1?report.resourceProofGroups-1u:i==5?report.resourceProofGroups:4000000u,
                i==2?report.resourceProofBytes-1u:i==6?report.resourceProofBytes:64u*1024u*1024u,
                i==3?report.resourceProofWork-1u:i==7?report.resourceProofWork:4000000u);
            if(!probe.fileImport(path.string(),&limited,false)||dx_iface::tableContentBindingReport(limited).state!=(i<4u?S::CaptureFailed:S::Bound))return fail("auxiliary cumulative proof boundary failed");
            ++boundaries;
        }
        clean.fields.front().m_childValues[4].m_value.m_rawData.clear();clean.dataLinks.front().m_customData.clear();
        if(dx_iface::tableContentBindingReport(clean).state!=S::Bound)return fail("public edit changed immutable auxiliary evidence");
        for(bool strict:{false,true}) {
            const auto output=dir/"auxiliary-refused.dxf";{std::ofstream out(output);out<<"preserve";}
            dx_iface exporter;if(exporter.fileExport(output.string(),clean.sourceVersion,binary,&clean,false,strict))return fail("auxiliary proof promoted output");
            std::ifstream old(output);if(std::string(std::istreambuf_iterator<char>(old),{})!="preserve")return fail("auxiliary refusal replaced destination");
            dx_data plain;dx_iface author;author.cData=&plain;author.currentBlock=plain.mBlock;
            DRW_Line line;line.basePoint={1,2,3};line.secPoint={4,5,6};author.addLine(line);
            if(!exporter.fileExport(output.string(),clean.sourceVersion,binary,&plain,false,strict))return fail("auxiliary refusal poisoned retry");
            dx_data readback;dx_iface reader;
            if(!reader.fileImport(output.string(),&readback,false)||readback.mBlock->ent.size()!=1u)return fail("auxiliary retry readback failed");
            const auto* actual=dynamic_cast<const DRW_Line*>(readback.mBlock->ent.front());
            if(!actual||actual->basePoint.z!=3||actual->secPoint.z!=6)return fail("auxiliary retry changed coordinates");
            ++refusals;
        }
        Tags minimal{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,revision},{0,"ENDSEC"},{0,"SECTION"},{2,"OBJECTS"}};
        const auto begin=slot(good,5,"F100")-1u;auto end=begin+1u;while(good[end].code!=0)++end;
        minimal.insert(minimal.end(),good.begin()+static_cast<std::ptrdiff_t>(begin),good.begin()+static_cast<std::ptrdiff_t>(end));
        minimal.insert(minimal.end(),{{0,"ENDSEC"},{0,"EOF"}});
        if(!write(path,minimal,binary))return false;
        Probe direct;dx_data directData;direct.cData=&directData;dxfRW codec(path.string().c_str());
        codec.setDxfReadTypedVariantNodeBudget(2u);
        if(codec.read(&direct,false)||direct.fieldCallbacks)return fail("FIELD node exhaustion published a callback");
        codec.setDxfReadTypedVariantNodeBudget(100000u);
        if(!codec.read(&direct,false)||direct.fieldCallbacks!=1u)return fail("FIELD node exhaustion retry failed");
        ++publicBudgets;
        codec.setDxfReadRetainedByteBudget(64u);
        if(codec.read(&direct,false)||direct.fieldCallbacks!=1u)return fail("FIELD byte exhaustion published a callback");
        codec.setDxfReadRetainedByteBudget(2u*1024u*1024u);
        if(!codec.read(&direct,false)||direct.fieldCallbacks!=2u)return fail("FIELD byte exhaustion retry failed");
        ++publicBudgets;
    }
    std::cout<<"auxiliary source proof: "<<routes<<" routes; "<<fieldFaults.size()<<" FIELD + "<<linkFaults.size()<<" DATALINK mutation kinds; "
        <<typed<<" typed pairs; "<<raw<<" raw pairs; "<<callbacks<<" callback corrections; "<<wire<<" wire pairs; "<<variants<<" variants; "
        <<boundaries<<" proof boundaries; "<<publicBudgets<<" reader budget retries; "<<refusals<<" atomic refusal/retry readbacks\n";
    return routes==8u&&typed==(fieldFaults.size()+linkFaults.size())*routes&&raw==160u&&callbacks==96u&&wire==64u&&variants==40u
        &&boundaries==64u&&publicBudgets==16u&&refusals==16u;
}

int inspect(const char* path) {
    Probe p; dx_data data;
    const bool ok=p.fileImport(path,&data,false);
    const auto compound=dx_iface::tableSourceReport(data);
    const auto bindings=dx_iface::tableContentBindingReport(data);
    std::cout<<"binding-state="<<static_cast<int>(bindings.state)<<" first="<<std::hex<<bindings.firstHandle
        <<std::dec<<" resources="<<bindings.resourceRecords<<" items="<<bindings.resourceItems
        <<" bytes="<<bindings.retainedBytes<<" bound="<<bindings.boundReferences
        <<" null="<<bindings.nullReferences<<" local="<<bindings.localStyleUses<<" absent="<<bindings.absentStyleUses<<'\n';
    std::cout<<"resource-proof records="<<bindings.resourceProofRecords<<" groups="<<bindings.resourceProofGroups
        <<" bytes="<<bindings.resourceProofBytes<<" work="<<bindings.resourceProofWork<<" map-matches="<<bindings.sourceMapMatches
        <<" style-matches="<<bindings.sourceStyleMatches<<" bound-formats="<<bindings.boundResourceReferences
        <<" inherited-formats="<<bindings.inheritedResourceReferences<<" symbol-proof-uses="<<bindings.symbolProofUses
        <<" ltype-dependency-rows="<<bindings.lineTypeDependencyRows<<" complex-text-bindings="<<bindings.complexTextStyleBindings
        <<" complex-shape-bindings="<<bindings.complexShapeStyleBindings
        <<" field-proof-uses="<<bindings.fieldProofUses<<" datalink-proof-uses="<<bindings.dataLinkProofUses
        <<" dictionary-proof-uses="<<bindings.dictionaryProofUses<<" xrecord-proof-uses="<<bindings.xRecordProofUses
        <<" unknown-references="<<bindings.unknownResourceReferences<<'\n';
    std::cout<<"compound-state="<<static_cast<int>(compound.state)<<" tables="<<compound.tables
             <<" contents="<<compound.contents<<" geometries="<<compound.geometries
             <<" compounds="<<compound.compounds<<" records="<<compound.records
             <<" dictionary-candidates="<<compound.dictionaryCandidates<<" table-wrappers="<<compound.tableWrappers
             <<" groups="<<compound.groups<<" bytes="<<compound.retainedBytes<<'\n';
    std::cout<<"content-raw-matches="<<compound.contentRawMatches<<" proof-slots="<<compound.contentProofSlots
             <<" nonnull-unknown-content-references="<<compound.nonnullUnknownContentReferences<<" role-counts=";
    for (const auto count:compound.contentReferenceSlots) std::cout<<count<<',';
    std::cout<<'\n';
    if (compound.contentRawMatches==1u) {
        std::map<std::pair<unsigned,std::uint32_t>,unsigned> ids;
        for (const auto& ref:DxIfaceTableSourceTestAccess::references(data))
            ++ids[{static_cast<unsigned>(ref.kind),ref.value}];
        for (const auto& id:ids) std::cout<<"source-role="<<id.first.first<<" id="<<std::hex<<id.first.second
            <<std::dec<<" slots="<<id.second<<'\n';
        for (const auto& ref:DxIfaceTableSourceTestAccess::references(data))
            std::cout<<"source-reference kind="<<static_cast<unsigned>(ref.kind)<<" index="<<ref.groupIndex
                <<" code="<<ref.code<<" value="<<std::hex<<ref.value<<std::dec<<'\n';
    }
    std::cout<<"imported="<<ok<<" matched="<<p.content.size()<<" unknown="<<p.unknown<<'\n';
    const auto resourceReferences=[&](const DRW_TableEntry& row,const char* prefix,const auto& resource) {
        for(const auto& ref:resource.references)std::cout<<prefix<<"-reference handle="<<std::hex<<row.handle<<std::dec
            <<" kind="<<static_cast<unsigned>(ref.kind)<<" index="<<ref.groupIndex<<" code="<<ref.code<<" value="<<std::hex<<ref.value<<std::dec<<'\n';
    };
    const auto symbol=[&](const DRW_TableEntry& row,const char* prefix,const auto& resource) {
        std::cout<<"symbol-row handle="<<std::hex<<row.handle<<std::dec<<" family="<<prefix<<" flags="<<row.flags<<'\n';
        resourceReferences(row,prefix,resource);
        if(const auto* ltype=dynamic_cast<const DRW_LType*>(&row))for(std::size_t i=0;i<ltype->segments.size();++i) {
            const auto& segment=ltype->segments[i];
            std::cout<<"symbol-segment handle="<<std::hex<<row.handle<<std::dec<<" index="<<i<<" flags="<<segment.shapeFlags
                <<" shape="<<segment.complexShapeCode<<" style="<<std::hex<<segment.styleHandle.ref<<std::dec<<'\n';
        }
    };
    for(const auto& item:DxIfaceTableSourceTestAccess::resources(data)) {
        const auto& resource=item.second;
        if(!resource.rawTypedMatched||!resource.model)continue;
        if(resource.family==dx_tableResourceFamily::TextStyle)symbol(*resource.model,"STYLE",resource);
        if(resource.family==dx_tableResourceFamily::LineType)symbol(*resource.model,"LTYPE",resource);
        if(resource.family==dx_tableResourceFamily::Field)resourceReferences(*resource.model,"FIELD",resource);
        if(resource.family==dx_tableResourceFamily::DataLink)resourceReferences(*resource.model,"DATALINK",resource);
        if(resource.family==dx_tableResourceFamily::Dictionary)resourceReferences(*resource.model,"DICTIONARY",resource);
        if(resource.family==dx_tableResourceFamily::XRecord)resourceReferences(*resource.model,"XRECORD",resource);
    }
    if(bindings.sourceStyleMatches)for(const auto& style:data.tableStyles) {
        const auto& resource=DxIfaceTableSourceTestAccess::mapResource(data,style.handle);
        if(!resource.rawTypedMatched)continue;
        std::cout<<"TABLESTYLE source-handle="<<std::hex<<style.handle<<std::dec<<" rows="<<style.m_rowStyles.size()<<'\n';
        for(const auto& ref:resource.references)std::cout<<"style-reference handle="<<std::hex<<style.handle<<std::dec
            <<" kind="<<static_cast<unsigned>(ref.kind)<<" index="<<ref.groupIndex<<" code="<<ref.code<<" value="<<std::hex<<ref.value<<std::dec<<'\n';
    }
    for(const auto& map:data.cellStyleMaps) {
        std::cout<<"CELLSTYLEMAP handle="<<std::hex<<map.handle<<" owner="<<map.parentHandle<<std::dec
            <<" styles="<<map.m_cellStyles.size()<<" complete="<<map.finalizeDxf()<<'\n';
        for(const auto& s:map.m_cellStyles)std::cout<<"map-style id="<<s.m_id<<" type="<<s.m_type<<" class="<<s.m_styleClass
            <<" data="<<s.m_hasData<<" background="<<s.m_backgroundColor<<" borders="<<s.m_borders.size()<<'\n';
        if(bindings.sourceMapMatches)for(const auto& ref:DxIfaceTableSourceTestAccess::mapResource(data,map.handle).references)
            std::cout<<"map-reference handle="<<std::hex<<map.handle<<std::dec<<" kind="<<static_cast<unsigned>(ref.kind)
                <<" index="<<ref.groupIndex<<" code="<<ref.code<<" value="<<std::hex<<ref.value<<std::dec<<'\n';
    }
    for (const auto& o:p.content) {
        unsigned cells=0u,items=0u,contents=0u,styles=0u,formats=0u;
        for (const auto& col:o.m_dxfMetadata->m_columns) items+=static_cast<unsigned>(col.m_customItems.size());
        for (const auto& row:o.m_dxfMetadata->m_rows) {
            items+=static_cast<unsigned>(row.m_scope.m_customItems.size());
            for (const auto& cell:row.m_cells) {
                ++cells; items+=static_cast<unsigned>(cell.m_scope.m_customItems.size());
                styles+=cell.m_scope.m_style.m_hasData?1u:0u;
                contents+=static_cast<unsigned>(cell.m_contentFormats.size());
                for (const auto& format:cell.m_contentFormats) formats+=format?1u:0u;
            }
        }
        std::cout<<"TABLECONTENT handle="<<std::hex<<o.handle<<" owner="<<o.parentHandle<<std::dec
                 <<" columns="<<o.m_content.m_columns.size()<<" rows="<<o.m_content.m_rows.size()
                 <<" cells="<<cells<<" custom-items="<<items<<" contents="<<contents<<" cell-overrides="<<styles
                 <<" content-formats="<<formats<<" semantic-core="<<o.m_semanticContentComplete<<'\n';
    }
    return ok?0:1;
}
} // namespace
int main(int argc,char** argv) {
    if (argc==3 && std::string(argv[1])=="--inspect") return inspect(argv[2]);
    if(argc==5 && (std::string(argv[1])=="--generate-symbol-source"||std::string(argv[1])=="--generate-auxiliary-source"||std::string(argv[1])=="--generate-spine-source")) {
        const std::string revision=argv[2],encoding=argv[3];
        if((revision!="AC1021"&&revision!="AC1024"&&revision!="AC1027"&&revision!="AC1032") || (encoding!="ascii"&&encoding!="binary"))return 2;
        const auto tags=std::string(argv[1])=="--generate-symbol-source"?complexSymbolSource(revision):
            std::string(argv[1])=="--generate-spine-source"?spineResourceSource(revision):auxiliaryResourceSource(revision);
        return write(argv[4],tags,encoding=="binary")?0:1;
    }
    const bool compound=argc==2 && std::string(argv[1])=="--compound-source";
    const bool proof=argc==2 && std::string(argv[1])=="--compound-content-proof";
    const bool bindings=argc==2 && std::string(argv[1])=="--compound-bindings";
    const bool maps=argc==2 && std::string(argv[1])=="--cell-style-map-source";
    const bool mapProof=argc==2 && std::string(argv[1])=="--cell-style-map-proof";
    const bool styleProof=argc==2 && std::string(argv[1])=="--table-style-proof";
    const bool symbolProof=argc==2 && std::string(argv[1])=="--symbol-source-proof";
    const bool complexBindings=argc==2 && std::string(argv[1])=="--complex-symbol-bindings";
    const bool auxiliaryProof=argc==2 && std::string(argv[1])=="--auxiliary-source-proof";
    const bool spineProof=argc==2 && std::string(argv[1])=="--spine-source-proof";
    if (argc!=1 && !compound && !proof && !bindings && !maps && !mapProof && !styleProof && !symbolProof && !complexBindings && !auxiliaryProof && !spineProof) return 2;
    const auto dir=std::filesystem::temp_directory_path()/("libdxfrw-table-content-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::error_code error; if (!std::filesystem::create_directory(dir,error) || error) return 1;
    const bool ok=compound?compoundTests(dir):proof?contentProofTests(dir):bindings?bindingTests(dir):maps?mapProfileTests(dir):mapProof?mapSourceProofTests(dir):styleProof?tableStyleProofTests(dir):symbolProof?symbolSourceProofTests(dir):complexBindings?complexSymbolBindingTests(dir):auxiliaryProof?auxiliarySourceProofTests(dir):spineProof?spineSourceProofTests(dir):tests(dir);
    if (ok) std::filesystem::remove_all(dir,error); else std::cerr<<"Evidence: "<<dir<<'\n';
    return ok?0:1;
}
