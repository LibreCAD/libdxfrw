#include "dx_iface.h"
#include "intern/dwgbufferw.h"
#include "intern/dxftablecontent.h"

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

namespace {
struct Tag {int code; std::string value;};
using Tags=std::vector<Tag>;
struct Probe final : dx_iface {
    std::vector<DRW_TableContentObject> content;
    std::vector<DRW_RawDxfObject> raw;
    unsigned unknown=0u;
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
int inspect(const char* path) {
    Probe p; dx_data data;
    const bool ok=p.fileImport(path,&data,false);
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
    if (argc!=1) return 2;
    const auto dir=std::filesystem::temp_directory_path()/("libdxfrw-table-content-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::error_code error; if (!std::filesystem::create_directory(dir,error) || error) return 1;
    const bool ok=tests(dir);
    if (ok) std::filesystem::remove_all(dir,error); else std::cerr<<"Evidence: "<<dir<<'\n';
    return ok?0:1;
}
