// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_iface.h"
#include "intern/dxfattdef.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <new>
#include <sstream>
#include <stdexcept>

namespace {long allocationRemaining=-1;}
void* operator new(std::size_t size) {
    if(allocationRemaining==0)throw std::bad_alloc();
    if(allocationRemaining>0)--allocationRemaining;
    if(void* value=std::malloc(size?size:1u))return value;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void* value) noexcept {std::free(value);}
void operator delete[](void* value) noexcept {std::free(value);}
void operator delete(void* value,std::size_t) noexcept {std::free(value);}
void operator delete[](void* value,std::size_t) noexcept {std::free(value);}

namespace {
struct Tag {int code;std::string value;};
using Tags=std::vector<Tag>;
bool check(bool value,const char* label) {
    if(!value)std::cerr<<"ATTDEF source: "<<label<<'\n';
    return value;
}
Tags body(unsigned mode) {
    Tags result{{5,"A1"},{330,"B0"},{100,"AcDbEntity"},{8,"host-layer"},
        {102,"{HOST"},{5,"DEAD"},{330,"FACE"},{1,"opaque"},{2,"not-a-tag"},
        {3,"not-a-prompt"},{10,"99"},{20,"98"},{40,"97"},{1000,"opaque-\\U+00E9"},{102,"}"},
        {100,"AcDbText"},{10,"1.25"},{20,"2.5"},{30,"3.75"},{40,"4.5"},
        {1,"default-é"},{7,"STANDARD"},{11,"5"},{21,"6"},{31,"7"},
        {50,"15"},{210,"0"},{220,"1"},{230,"0"}};
    if(mode) {
        result.insert(result.end(),{{mode==1u?101:100,"Embedded Object"},
            {10,"10"},{20,"20"},{30,"30"},{40,"2.5"},{41,"12"},{46,"7"},
            {3,"FIRST_"},{1,"LAST-é"},{11,"0"},{21,"1"},{31,"0"},{71,"4"},
            {102,"{NESTED"},{5,"BEEF"},{330,"CAFE"},{1,"not-mtext"},{1000,"nested-\\U+00E9"},{102,"}"}});
    }
    result.insert(result.end(),{{100,"AcDbAttributeDefinition"},{3,"prompt-é"},{2,"TAG"},
        {70,"7"},{73,"5"},{74,"2"},{280,"1"},{1001,"APP"},{1003,"XDATA_LAYER"},
        {1010,"8"},{1020,"9"},{1030,"10"},{1070,"11"}});
    return result;
}
Tags document(const std::string& version,const Tags& row,bool block,bool dropped=false) {
    Tags result{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"},
        {0,"SECTION"},{2,block?"BLOCKS":"ENTITIES"}};
    if(block)result.insert(result.end(),{{0,"BLOCK"},{5,"B1"},{330,"B0"},{100,"AcDbEntity"},
        {8,"0"},{100,"AcDbBlockBegin"},{2,dropped?"":"GENERATED"},{70,"0"},{10,"0"},{20,"0"},{30,"0"}});
    result.push_back({0,"ATTDEF"});result.insert(result.end(),row.begin(),row.end());
    if(block)result.insert(result.end(),{{0,"ENDBLK"},{5,"B2"},{330,"B0"},
        {100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"}});
    result.insert(result.end(),{{0,"ENDSEC"},{0,"EOF"}});
    return result;
}
bool write(const std::filesystem::path& path,const Tags& tags,bool binary) {
    std::ofstream out(path,std::ios::binary);
    const auto word=[&](std::uint64_t value,unsigned bytes) {
        for(unsigned i=0;i<bytes;++i)out.put(static_cast<char>(value>>(8u*i)));
    };
    if(binary)out.write("AutoCAD Binary DXF\r\n\x1a\0",22);
    for(const auto& tag:tags) {
        if(!binary)out<<tag.code<<'\n'<<tag.value<<'\n';
        else {
            word(static_cast<unsigned>(tag.code),2u);
            if((tag.code>=60&&tag.code<=79)||(tag.code>=270&&tag.code<=289)||tag.code==1070)
                word(static_cast<std::uint16_t>(std::stoi(tag.value)),2u);
            else if((tag.code>=10&&tag.code<=59)||(tag.code>=210&&tag.code<=239)
                ||(tag.code>=1010&&tag.code<=1059)) {
                const double value=std::stod(tag.value);std::uint64_t bits=0u;
                std::memcpy(&bits,&value,sizeof(bits));word(bits,8u);
            } else {out<<tag.value;out.put('\0');}
        }
    }
    return out.good();
}
std::string snapshot(const DRW_Attdef& value) {
    std::ostringstream out;
    out.precision(17);
    const auto applicationData=[&](const DRW_Entity& entity) {
        for(const auto& group:entity.appData)for(const auto& field:group) {
            out<<'['<<field.code()<<':'<<field.type()<<':';
            if(field.type()==DRW_Variant::STRING)out<<field.c_str();
            else if(field.type()==DRW_Variant::INTEGER)out<<field.i_val();
            else if(field.type()==DRW_Variant::DOUBLE)out<<field.d_val();
            else throw std::runtime_error("unexpected authored application type");
            out<<']';
        }
    };
    out<<value.handle<<':'<<value.parentHandle<<':'<<value.layer<<':'<<value.text<<':'<<value.tag
        <<':'<<value.prompt<<':'<<unsigned(value.attribFlags)<<':'<<value.m_fieldLength
        <<':'<<value.lockPosition<<':'<<value.basePoint.x<<':'<<value.basePoint.y<<':'<<value.basePoint.z
        <<':'<<value.height<<':'<<value.style<<':'<<value.angle<<':'<<value.extPoint.y
        <<':'<<value.appData.size()<<':'<<value.extData.size()<<':'<<unsigned(value.m_attributeType);
    applicationData(value);
    if(value.mtext)out<<':'<<value.mtext->text<<':'<<value.mtext->angle<<':'<<value.mtext->height
        <<':'<<value.mtext->m_definedHeight<<':'<<value.mtext->appData.size()<<':'<<value.mtext->extData.size();
    if(value.mtext)applicationData(*value.mtext);
    return out.str();
}
class Probe final:public dx_iface,public DRW_DxfAttdefSourceHandler {
public:
    bool selected=true,disableOnSource=false,throwSource=false,throwTyped=false;
    mutable unsigned selections=0u;
    long failAllocation=-1;
    unsigned sourceCalls=0u,typedCalls=0u;
    unsigned depth=0u,begins=0u,ends=0u;
    std::vector<std::string> events,typed,sourceTyped;
    std::vector<DRW_RawDxfObject> raw;
    std::vector<DRW_DxfAttdefSourceContext> contexts;
    void addHeader(const DRW_Header*) override {}
    void addBlock(const DRW_Block&) override {++depth;++begins;events.push_back("B");}
    void endBlock() override {--depth;++ends;events.push_back("E");}
    void addBlockRecord(const DRW_Block_Record&) override {}
    bool wantsDxfAttdefSource() const override {
        ++selections;
        if(failAllocation>=0)allocationRemaining=failAllocation;
        return selected;
    }
    void addDxfAttdefSource(const DRW_Attdef& value,const DRW_RawDxfObject& source,
                           const DRW_DxfAttdefSourceContext& context) override {
        if(context.inBlock!=(depth!=0u))throw std::runtime_error("wrong callback block context");
        ++sourceCalls;
        events.push_back("S");raw.push_back(source);sourceTyped.push_back(snapshot(value));contexts.push_back(context);
        if(disableOnSource)selected=false;
        if(throwSource)throw std::runtime_error("injected source failure");
    }
    void addAttDef(const DRW_Attdef& value) override {
        ++typedCalls;
        events.push_back("T");typed.push_back(snapshot(value));
        if(throwTyped)throw std::runtime_error("injected typed failure");
    }
};
bool rawMatches(const DRW_RawDxfObject& raw,const Tags& row,bool binary) {
    if(raw.name!="ATTDEF"||raw.handle!=0xA1u||raw.parentHandle!=0xB0u
        ||raw.groups.size()!=row.size()||raw.rawValues.size()!=row.size()||raw.hasRawValues==binary)return false;
    for(std::size_t i=0;i<row.size();++i) {
        const auto& value=raw.groups[i];
        if(value.code()!=row[i].code||(!binary&&raw.rawValues[i]!=row[i].value))return false;
        if(value.type()==DRW_Variant::STRING) {
            if(!value.content.s||*value.content.s!=row[i].value)return false;
        } else if(value.type()==DRW_Variant::INTEGER) {
            if(value.i_val()!=std::stoi(row[i].value))return false;
        } else if(value.type()==DRW_Variant::DOUBLE) {
            if(value.d_val()!=std::stod(row[i].value))return false;
        } else return false;
        if(binary&&!raw.rawValues[i].empty())return false;
    }
    return true;
}
bool replayMatches(const Tags& row,const std::string& version,const std::string& expected,bool ext) {
    std::ostringstream encoded;
    for(const auto& tag:row)encoded<<tag.code<<'\n'<<tag.value<<'\n';
    std::istringstream input(encoded.str());
    std::unique_ptr<dxfReader> reader=std::make_unique<dxfReaderAscii>(&input);
    reader->setVersion(version,true);
    DRW_Attdef value;dxfAttdef::Decoder decoder(value);int code=0;
    while(reader->readRec(&code))if(!decoder.parseCode(code,reader))return false;
    return decoder.finish(ext)&&snapshot(value)==expected;
}
bool tests(const std::filesystem::path& path) {
    unsigned cells=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})
     for(bool binary:{false,true})for(bool block:{false,true})for(bool ext:{false,true})for(unsigned mode=0u;mode<3u;++mode) {
        const auto row=body(mode);const auto tags=document(version,row,block);
        if(!write(path,tags,binary))return false;
        dxfRW reader(path.string().c_str());Probe optIn;
        if(!check(reader.read(&optIn,ext)&&optIn.raw.size()==1u&&optIn.typed.size()==1u,"positive transport"))return false;
        const auto declared=version=="AC1021"?DRW::AC1021:version=="AC1024"?DRW::AC1024:
            version=="AC1027"?DRW::AC1027:DRW::AC1032;
        if(!check(optIn.typed==optIn.sourceTyped&&rawMatches(optIn.raw.front(),row,binary)
            &&optIn.raw.front().m_version==declared
            &&optIn.contexts.front().applyExtrusion==ext&&optIn.contexts.front().inBlock==block,
            "exact raw/context/finalized typed pair"))return false;
        if(!check(optIn.events==(block?std::vector<std::string>{"B","S","T","E"}
            :std::vector<std::string>{"S","T"}),"callback order"))return false;
        Probe optOut;optOut.selected=false;
        if(!check(reader.read(&optOut,ext)&&optOut.raw.empty()&&optOut.typed==optIn.typed
            &&replayMatches(row,version,optIn.typed.front(),ext),"opt-out and production replay equality"))return false;
        for(unsigned fault=0u;fault<5u;++fault) {
            auto badRow=row;
            // Missing mandatory fields cannot be supplied by opaque appData.
            if(fault<4u) {
                const int removed=fault==0u?1:fault==1u?2:fault==2u?20:40;
                unsigned applications=0u;
                for(auto it=badRow.begin();it!=badRow.end();) {
                    if(it->code==102)applications=it->value=="}"?applications-1u:applications+1u;
                    if(!applications&&it->code==removed)it=badRow.erase(it);else ++it;
                }
            }
            auto bad=document(version,badRow,block);
            if(fault==4u) {
                if(!block)continue;
                bool footer=false;
                for(auto& tag:bad) {
                    if(tag.code==0&&tag.value=="ENDBLK")footer=true;
                    if(footer&&tag.code==330){tag.value="not-a-handle";break;}
                }
            }
            if(!write(path,bad,binary))return false;
            Probe refused;
            const bool accepted=reader.read(&refused,ext);
            if(!check(!accepted&&refused.raw.empty()&&refused.typed.empty()
                &&refused.depth==0u,"malformed record/footer atomic refusal")) {
                std::cerr<<version<<" binary="<<binary<<" block="<<block<<" ext="<<ext<<" mode="<<mode
                    <<" fault="<<fault<<" accepted="<<accepted<<" raw="<<refused.raw.size()
                    <<" typed="<<refused.typed.size()<<" depth="<<refused.depth<<'\n';return false;
            }
            if(!write(path,tags,binary))return false;
            Probe corrected;
            if(!check(reader.read(&corrected,ext)&&corrected.raw.size()==1u,"same-facade corrected read"))return false;
        }
        ++cells;
     }
    // Requests are made before parsing, never re-evaluated at delayed flush.
    auto tags=document("AC1032",body(1u),true);
    auto second=body(2u);second.front().value="A2";
    auto footer=std::find_if(tags.begin(),tags.end(),[](const Tag& tag){return tag.code==0&&tag.value=="ENDBLK";});
    Tags extra{{0,"ATTDEF"}};extra.insert(extra.end(),second.begin(),second.end());
    tags.insert(footer,extra.begin(),extra.end());
    if(!write(path,tags,false))return false;
    dxfRW reader(path.string().c_str());Probe latched;latched.disableOnSource=true;
    const bool latchRead=reader.read(&latched,false);
    if(!check(latchRead&&latched.raw.size()==2u&&latched.typed.size()==2u
        &&latched.selections==2u,"per-record selection survives block flush")) {
        std::cerr<<"latch read="<<latchRead<<" raw="<<latched.raw.size()<<" typed="<<latched.typed.size()
            <<" selections="<<latched.selections<<" error="<<reader.getError()<<'\n';return false;
    }
    for(bool source:{false,true}) {
        Probe throwing;throwing.throwSource=source;throwing.throwTyped=!source;
        bool failed=false;try {failed=!reader.read(&throwing,false);}catch(const std::exception&){failed=true;}
        if(!check(failed&&throwing.depth==0u&&throwing.begins==throwing.ends
            &&throwing.raw.size()==1u&&throwing.typed.size()==(source?0u:1u),"throw closes block without later pairs"))return false;
        Probe retry;if(!check(reader.read(&retry,false)&&retry.raw.size()==2u,"throw corrected retry"))return false;
    }
    auto dropped=document("AC1032",body(1u),true,true);
    if(!write(path,dropped,false))return false;
    Probe drop;if(!check(reader.read(&drop,false)&&drop.events.empty(),"dropped block publishes no source pair"))return false;
    // Exercise aggregate raw node and logical text charges, and corrected reads.
    const auto quotaTags=document("AC1032",body(1u),true);
    if(!write(path,quotaTags,false))return false;
    reader.setDxfReadTypedVariantNodeBudget(12u);Probe nodes;
    if(!check(!reader.read(&nodes,false)&&nodes.raw.empty()&&nodes.typed.empty()&&nodes.depth==0u,"raw node quota refusal"))return false;
    reader.setDxfReadTypedVariantNodeBudget(10000u);
    reader.setDxfReadRetainedByteBudget(64u);Probe bytes;
    if(!check(!reader.read(&bytes,false)&&bytes.raw.empty()&&bytes.typed.empty()&&bytes.depth==0u,"logical byte quota refusal"))return false;
    reader.setDxfReadRetainedByteBudget(1024u*1024u);Probe recovered;
    if(!check(reader.read(&recovered,false)&&recovered.raw.size()==1u,"quota recovery"))return false;

    std::array<std::size_t,2> nodeMinimum{},byteMinimum{};
    for(unsigned selected=0u;selected<2u;++selected)for(bool nodeBudget:{false,true}) {
        std::size_t lower=0u,upper=1024u*1024u;
        const auto attempt=[&](std::size_t limit) {
            reader.setDxfReadRetainedByteBudget(nodeBudget?1024u*1024u:limit);
            reader.setDxfReadTypedVariantNodeBudget(nodeBudget?limit:10000u);
            Probe probe;probe.selected=selected!=0u;
            const bool ok=reader.read(&probe,false);
            if(probe.depth!=0u)throw std::runtime_error("quota left an unbalanced scope");
            return ok;
        };
        while(lower<upper) {
            const auto middle=lower+(upper-lower)/2u;
            if(attempt(middle))upper=middle;else lower=middle+1u;
        }
        if(!check(lower>0u&&attempt(lower)&&!attempt(lower-1u),"exact/below quota boundary"))return false;
        (nodeBudget?nodeMinimum:byteMinimum)[selected]=lower;
    }
    if(!check(nodeMinimum[0]<nodeMinimum[1]&&byteMinimum[0]<byteMinimum[1],
        "opt-out avoids additional source charges"))return false;
    reader.setDxfReadRetainedByteBudget(1024u*1024u);reader.setDxfReadTypedVariantNodeBudget(10000u);

    // Calibrate this one authored record, then refuse every allocation from
    // source selection through staging, callbacks and read completion. A
    // callback failure may deliver a prefix, but must close the public scope.
    Probe calibrated;calibrated.failAllocation=(std::numeric_limits<long>::max)();
    bool measured=false;
    try {measured=reader.read(&calibrated,false);}catch(...){}
    const long allocations=(std::numeric_limits<long>::max)()-allocationRemaining;
    allocationRemaining=-1;
    if(!check(measured&&allocations>0&&allocations<5000,"allocation calibration"))return false;
    for(long failure=0;failure<=allocations;++failure) {
        Probe probe;probe.failAllocation=failure;bool accepted=false;
        try {accepted=reader.read(&probe,false);}catch(...){}
        allocationRemaining=-1;
        if(!check(probe.depth==0u&&probe.begins==probe.ends&&probe.typedCalls<=probe.sourceCalls
            &&(!accepted||(probe.raw.size()==1u&&probe.typed.size()==1u)),"allocation refusal/publication/scope"))return false;
    }
    Probe retry;if(!check(reader.read(&retry,false)&&retry.raw.size()==1u,"allocation corrected retry"))return false;
    std::cout<<"ATTDEF transport routes="<<cells<<"; allocation schedules="<<allocations+1
        <<"; node minima="<<nodeMinimum[0]<<'/'<<nodeMinimum[1]
        <<"; byte minima="<<byteMinimum[0]<<'/'<<byteMinimum[1]
        <<"; malformed/retry, block/throw/opt-out and quotas: PASS\n";
    return true;
}
}
int main() {
    const auto scratch=std::filesystem::temp_directory_path()/("libdxfrw-attdef-source-"
        +std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if(!std::filesystem::create_directory(scratch))return 1;
    struct Cleanup {std::filesystem::path path;~Cleanup(){std::error_code ignored;std::filesystem::remove_all(path,ignored);}} cleanup{scratch};
    try {return tests(scratch/"generated.dxf")?0:1;}
    catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
