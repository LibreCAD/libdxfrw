#include "dx_iface.h"
#include "intern/dwgbufferw.h"
#include "intern/dxftablecontent.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>

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
};

namespace {
struct Tag {int code; std::string value;};
using Tags=std::vector<Tag>;
struct Probe final : dx_iface {
    std::vector<DRW_TableContentObject> content;
    std::vector<DRW_RawDxfObject> raw;
    unsigned unknown=0u;
    unsigned tableSources=0u, tableCallbacks=0u;
    bool optIn=true, forceOptIn=false, throwOnTable=false, mutateTable=false, paddedRaw=false;
    std::size_t largestTableSourceString=0u;
    void clear() { content.clear(); raw.clear(); unknown=0u; }
    void addTableContent(const DRW_TableContentObject& value) override {
        content.push_back(value); dx_iface::addTableContent(value);
    }
    void addRawDxfObject(const DRW_RawDxfObject& value) override {
        if (value.name=="TABLECONTENT" || value.name=="aCdBtAbLeCoNtEnT") raw.push_back(value);
        dx_iface::addRawDxfObject(value);
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
            if ((tag.code>=90 && tag.code<=99) || tag.code==420) word(static_cast<std::uint32_t>(std::stoll(tag.value)),4u);
            else if (tag.code==310) {
                out.put(static_cast<char>(tag.value.size()/2u));
                for (std::size_t i=0;i<tag.value.size();i+=2u)
                    out.put(static_cast<char>(std::stoul(tag.value.substr(i,2u),nullptr,16)));
            }
            else if (tag.code>=60 && tag.code<=79) word(static_cast<std::uint16_t>(std::stoi(tag.value)),2u);
            else if (tag.code>=170 && tag.code<=179) word(static_cast<std::uint16_t>(std::stoi(tag.value)),2u);
            else if (tag.code>=280 && tag.code<=289) word(static_cast<std::uint16_t>(std::stoi(tag.value)),2u);
            else if ((tag.code>=10 && tag.code<=59) || tag.code==140 || tag.code==144) {
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

int inspect(const char* path) {
    Probe p; dx_data data;
    const bool ok=p.fileImport(path,&data,false);
    const auto compound=dx_iface::tableSourceReport(data);
    std::cout<<"compound-state="<<static_cast<int>(compound.state)<<" tables="<<compound.tables
             <<" contents="<<compound.contents<<" geometries="<<compound.geometries
             <<" compounds="<<compound.compounds<<" records="<<compound.records
             <<" dictionary-candidates="<<compound.dictionaryCandidates<<" table-wrappers="<<compound.tableWrappers
             <<" groups="<<compound.groups<<" bytes="<<compound.retainedBytes<<'\n';
    std::cout<<"imported="<<ok<<" matched="<<p.content.size()<<" unknown="<<p.unknown<<'\n';
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
    const bool compound=argc==2 && std::string(argv[1])=="--compound-source";
    if (argc!=1 && !compound) return 2;
    const auto dir=std::filesystem::temp_directory_path()/("libdxfrw-table-content-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::error_code error; if (!std::filesystem::create_directory(dir,error) || error) return 1;
    const bool ok=compound?compoundTests(dir):tests(dir);
    if (ok) std::filesystem::remove_all(dir,error); else std::cerr<<"Evidence: "<<dir<<'\n';
    return ok?0:1;
}
