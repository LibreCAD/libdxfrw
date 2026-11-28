#include "intern/dwg_tablecontent_ac1018_candidate.h"
#include "intern/dwgbufferw.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>

namespace fault {long remaining=-1;std::size_t count=0,largest=0;bool active=false;}
void* operator new(std::size_t n) {
    if (fault::active) {++fault::count;if(n>fault::largest)fault::largest=n;}
    if (fault::remaining==0) {fault::remaining=-1;throw std::bad_alloc();}
    if (fault::remaining>0) --fault::remaining;
    if (void* p=std::malloc(n?n:1u)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}

namespace {
using DwgTableContentAc1018Candidate::Result;
using DwgTableContentAc1018Candidate::HandleKind;
struct Expected {
    std::vector<DwgTableContentAc1018Candidate::TextSpan> text;
    std::vector<DwgTableContentAc1018Candidate::HandleSpan> handles;
};
Expected* expected=nullptr;
struct Options {
    std::uint8_t selfRole{0u},ownerRole{4u},textRole{5u},borderRole{5u},styleRole{5u};
    bool badTextRef{false},badStyleRef{false},badPadding{false};
    bool namedColor{false},nonAscii{false},embeddedNul{false},reservedColumnCount{false};
    bool reservedColorRgb{false},reservedDouble{false};
    bool eed{false},reactor{false},extension{false};
    int oversizedDataType{0};
    int columnCount{3},rowCount{3},cellCount{3};
    bool longText{false},manyCustom{false};
    bool oneLongText{false};
};
void putHandle(dwgBufferW& b,std::uint8_t role,std::uint32_t value) {
    dwgHandle h; h.code=role; h.ref=value; b.putHandle(h);
}
void trackedHandle(dwgBufferW& b,std::uint8_t role,std::uint32_t value,HandleKind kind) {
    DwgTableContentAc1018Candidate::HandleSpan span;
    span.kind=kind;span.startBit=b.bitCount();span.role=role;span.reference=value;
    for(auto v=value;v;v>>=8)++span.size;
    putHandle(b,role,value);span.endBit=b.bitCount();
    if(expected)expected->handles.push_back(span);
}
void text(dwgBufferW& b,const std::string& value) {
    DwgTableContentAc1018Candidate::TextSpan span;span.startBit=b.bitCount();
    b.putBitShort(static_cast<std::uint16_t>(value.size()));
    for (unsigned char c:value) b.putRawChar8(c);
    if(expected) {
        span.endBit=b.bitCount();span.bytes.assign(value.begin(),value.end());
        expected->text.push_back(std::move(span));
    }
}
void color(dwgBufferW& b,const Options& o,bool allowNamed=false) {
    b.putBitShort(0u);
    if (o.reservedColorRgb) b.put2Bits(3u); else b.putBitLong(0);
    b.putRawChar8(o.namedColor && allowNamed?1u:0u);
    if (o.namedColor && allowNamed) text(b,"color");
}
void format(dwgBufferW& b,dwgBufferW& h,const Options& o) {
    for(int i=0;i<4;++i)b.putBitLong(0);
    text(b,o.longText?std::string(1000,'a'):std::string()); b.putBitDouble(0.); b.putBitDouble(1.); b.putBitLong(0);
    color(b,o,true); trackedHandle(h,o.textRole,o.badTextRef?1u:0u,HandleKind::TextStyle); b.putBitDouble(1.);
}
void style(dwgBufferW& b,dwgBufferW& h,const Options& o,bool present,int borders) {
    b.putBitLong(present?4:3); b.putBitShort(present?1u:0u);
    if (!present)return;
    b.putBitLong(0); b.putBitLong(0); color(b,o); b.putBitLong(0); format(b,h,o);
    b.putBitShort(0u); b.putBitLong(borders);
    for (int j=0;j<borders;++j) {
        b.putBitLong(1); b.putBitLong(0); b.putBitLong(0); color(b,o);
        b.putBitLong(0); trackedHandle(h,o.borderRole,0u,HandleKind::BorderLinetype); b.putBitLong(0); b.putBitDouble(0.);
    }
}
void append(dwgBufferW& dst,const dwgBufferW& src) {
    for (std::uint64_t i=0;i<src.bitCount();++i)
        dst.putBit((src.data()[i>>3]>>(7u-(i&7u)))&1u);
}
DRW_UnsupportedObject build(const Options& o={},Expected* output=nullptr) {
    struct Scope {Expected* previous; explicit Scope(Expected* e):previous(expected){expected=e;} ~Scope(){expected=previous;}} scope(output);
    dwgBufferW b,h;
    b.putObjType(DRW::AC1018,514u); const auto endPos=b.bitCount(); b.putRawLong32(0u);
    putHandle(b,o.selfRole,0xb9u); b.putBitShort(o.eed?1u:0u);
    b.putBitLong(o.reactor?1:0); b.putBit(o.extension?0u:1u);
    putHandle(h,o.ownerRole,0xbcu);
    text(b,""); text(b,"");
    if (o.reservedColumnCount) b.put2Bits(3u); else b.putBitLong(o.columnCount);
    for (int col=0;col<3;++col) {
        text(b,""); b.putBitLong(0); b.putBitLong(0); style(b,h,o,false,0);
        b.putBitLong(0);
        if(o.reservedDouble && col==0)b.put2Bits(3u);else b.putBitDouble(152.);
    }
    b.putBitLong(o.rowCount);
    for (int row=0;row<3;++row) {
        b.putBitLong(o.cellCount);
        for (int col=0;col<3;++col) {
            const int index=row*3+col; const bool populated=index<7;
            const int customCount=populated?(o.manyCustom?6:1):0;
            b.putBitLong(0); text(b,o.oneLongText && index==0?std::string(4096,'a'):std::string());
            b.putBitLong(0); b.putBitLong(customCount);
            if (populated) {
                for(int j=0;j<customCount;++j) {
                    text(b,"checksum"); b.putBitLong(o.oversizedDataType?o.oversizedDataType:2);
                    if(o.oversizedDataType)b.putBitLong(16*1024*1024); else b.putBitDouble(1.);
                }
            }
            b.putBitLong(0); b.putBitLong(populated?1:0);
            if (populated) {
                b.putBitLong(1); b.putBitLong(4);
                std::string value=std::to_string(index);
                if (o.nonAscii && index==0) value.assign(1,static_cast<char>(0x80));
                if (o.embeddedNul && index==0) value=std::string("A\0B",3);
                else value.push_back('\0');
                text(b,value); b.putBitLong(0); b.putBitShort(1u); format(b,h,o);
            }
            style(b,h,o,true,index<6?4:3); b.putBitLong(0); b.putBitLong(0);
        }
        b.putBitLong(0); b.putBitLong(0); style(b,h,o,false,0);
        b.putBitLong(0); b.putBitDouble(9.);
    }
    b.putBitLong(0); style(b,h,o,true,0); b.putBitLong(1);
    b.putBitLong(0);b.putBitLong(0);b.putBitLong(0);b.putBitLong(2);
    const auto dataEnd=b.bitCount();
    b.patchRawLong32AtBit(endPos,static_cast<std::uint32_t>(dataEnd));
    trackedHandle(h,o.styleRole,o.badStyleRef?0u:0x6bu,HandleKind::TableStyle);
    if(output)for(auto& span:output->handles){span.startBit+=dataEnd;span.endBit+=dataEnd;}
    append(b,h);
    if(o.badPadding) {b.putBit(1u);} b.alignToByte();
    DRW_UnsupportedObject raw; raw.m_version=DRW::AC1018; raw.m_isCustomClass=true;
    raw.m_hasClassDefinition=true; raw.m_recordName="TABLECONTENT";
    raw.m_className="AcDbTableContent"; raw.m_objectType=514; raw.m_handle=0xb9u;
    raw.m_rawBytes=b.data(); raw.m_objectSize=static_cast<std::uint32_t>(raw.m_rawBytes.size());
    return raw;
}
bool clear(const Result& r) {
    return r.handle==0u && r.parentHandle==0u && r.rawBody.empty()
        && r.textSpans.empty() && r.handleSpans.empty() && r.content.m_rows.empty();
}
bool refused(const DRW_UnsupportedObject& raw) {
    Result r; r.handle=1u; r.rawBody.push_back(8u);
    return !DwgTableContentAc1018Candidate::inspect(raw,r) && clear(r);
}
bool generated() {
    Expected expectedFields;auto raw=build({},&expectedFields); const auto baseline=raw.m_rawBytes; Result r;
    if (!DwgTableContentAc1018Candidate::inspect(raw,r)
        || r.handle!=0xb9u || r.parentHandle!=0xbcu || r.rawBody!=baseline
        || r.content.m_columns.size()!=3u || r.content.m_rows.size()!=3u
        || r.textSpans.size()!=45u || r.handleSpans.size()!=51u
        || r.workUnits==0u || r.workUnits>12000u) return false;
    if(r.textSpans.size()!=expectedFields.text.size() || r.handleSpans.size()!=expectedFields.handles.size())return false;
    for(std::size_t i=0;i<r.textSpans.size();++i) {
        const auto& a=r.textSpans[i];const auto& b=expectedFields.text[i];
        if(a.startBit!=b.startBit || a.endBit!=b.endBit || a.bytes!=b.bytes)return false;
    }
    for(std::size_t i=0;i<r.handleSpans.size();++i) {
        const auto& a=r.handleSpans[i];const auto& b=expectedFields.handles[i];
        if(a.kind!=b.kind || a.startBit!=b.startBit || a.endBit!=b.endBit
            || a.role!=b.role || a.size!=b.size || a.reference!=b.reference)return false;
    }
    std::size_t nul=0; for (const auto& s:r.textSpans) if (!s.bytes.empty() && s.bytes.back()==0u) ++nul;
    if (nul!=7u) return false;
    // Sample every recorded text/handle boundary plus envelope boundaries.
    std::vector<std::size_t> cuts{0u,1u,2u,baseline.size()/2u,baseline.size()-1u,
                                  static_cast<std::size_t>(r.dataEndBit/8u)};
    for(const auto& s:r.textSpans) cuts.push_back(static_cast<std::size_t>(s.endBit/8u));
    for(const auto& s:r.handleSpans) cuts.push_back(static_cast<std::size_t>(s.endBit/8u));
    std::sort(cuts.begin(),cuts.end()); cuts.erase(std::unique(cuts.begin(),cuts.end()),cuts.end());
    for (std::size_t n:cuts) {
        if(n>=baseline.size())continue;
        raw.m_rawBytes.assign(baseline.begin(),baseline.begin()+n);
        raw.m_objectSize=static_cast<std::uint32_t>(n);
        if (!refused(raw)) return false;
    }
    raw=build(); raw.m_className="Other"; if(!refused(raw))return false;
    raw=build(); raw.m_hasClassDefinition=false; if(!refused(raw))return false;
    raw=build(); raw.m_isEntity=true; if(!refused(raw))return false;
    raw=build(); raw.m_hasDataStorage=true; if(!refused(raw))return false;
    raw=build(); raw.m_version=DRW::AC1021; if(!refused(raw))return false;
    raw=build(); raw.m_objectType=515; if(!refused(raw))return false;
    raw=build(); raw.m_handle=0xbau; if(!refused(raw))return false;
    raw=build(); raw.m_bodyBitSize=1u; if(!refused(raw))return false;
    raw=build(); raw.m_rawBytes.resize(65537u); raw.m_objectSize=65537u; if(!refused(raw))return false;
    raw=build(); raw.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::Validated);
    raw.m_parentHandle=0xbcu;raw.m_xDictFlag=1u;
    if(!DwgTableContentAc1018Candidate::inspect(raw,r))return false;
    raw.m_parentHandle=0xbbu;if(!refused(raw))return false;
    Options o; o.selfRole=5u;if(!refused(build(o)))return false;
    o={};o.eed=true;if(!refused(build(o)))return false;
    o={};o.reactor=true;if(!refused(build(o)))return false;
    o={};o.extension=true;if(!refused(build(o)))return false;
    o={};o.ownerRole=5u;if(!refused(build(o)))return false;
    o={};o.textRole=4u;if(!refused(build(o)))return false;
    o={};o.borderRole=4u;if(!refused(build(o)))return false;
    o={};o.styleRole=4u;if(!refused(build(o)))return false;
    o={};o.badTextRef=true;if(!refused(build(o)))return false;
    o={};o.badStyleRef=true;if(!refused(build(o)))return false;
    o={};o.badPadding=true;if(!refused(build(o)))return false;
    o={};o.namedColor=true;if(!refused(build(o)))return false;
    o={};o.reservedColorRgb=true;if(!refused(build(o)))return false;
    o={};o.reservedDouble=true;if(!refused(build(o)))return false;
    for (const int kind:{16,512}) {
        o={};o.oversizedDataType=kind;raw=build(o);
        fault::largest=0;fault::active=true;const bool safe=refused(raw);fault::active=false;
        if(!safe || fault::largest>=1024u*1024u)return false;
    }
    o={};o.nonAscii=true;if(!refused(build(o)))return false;
    o={};o.embeddedNul=true;if(!refused(build(o)))return false;
    o={};o.reservedColumnCount=true;if(!refused(build(o)))return false;
    for(int which=0;which<3;++which) {
        o={};if(which==0)o.columnCount=6;else if(which==1)o.rowCount=6;else o.cellCount=6;
        raw=build(o);fault::count=0;fault::active=true;const bool safe=refused(raw);fault::active=false;
        if(!safe || fault::count>2000u)return false;
    }
    for(int which=0;which<2;++which) {
        o={};if(which==0)o.longText=true;else o.manyCustom=true;
        raw=build(o);fault::count=0;fault::active=true;const bool safe=refused(raw);fault::active=false;
        if(!safe || fault::count>15000u)return false;
    }
    raw=build(); raw.m_rawBytes.push_back(0u); ++raw.m_objectSize; if(!refused(raw))return false;
    raw=build(); fault::count=0;fault::active=true;
    if (!DwgTableContentAc1018Candidate::inspect(raw,r))return false;
    fault::active=false;const auto count=fault::count;
    if (count<8u) return false;
    std::vector<std::size_t> offsets{0u,1u,2u,16u,count/4u,count/2u,3u*count/4u,count-1u};
    std::sort(offsets.begin(),offsets.end()); offsets.erase(std::unique(offsets.begin(),offsets.end()),offsets.end());
    for (const auto i:offsets) {
        r.handle=1u;fault::remaining=static_cast<long>(i);
        const bool ok=DwgTableContentAc1018Candidate::inspect(raw,r);fault::remaining=-1;
        if(ok || !clear(r) || raw.m_rawBytes!=baseline)return false;
        if(!DwgTableContentAc1018Candidate::inspect(raw,r))return false;
    }
    std::cout<<"generated 3x3 controls passed; sampled allocation faults="<<offsets.size()<<" of "<<count
             <<" work units="<<r.workUnits<<'\n';
    return true;
}
bool physical(const char* path) {
    std::ifstream f(path,std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(f)),{});
    if(!f||bytes.empty())return false;
    auto raw=build();raw.m_rawBytes=std::move(bytes);raw.m_objectSize=static_cast<std::uint32_t>(raw.m_rawBytes.size());
    Result r;if(!DwgTableContentAc1018Candidate::inspect(raw,r)
       || r.dataEndBit!=13313u || r.handleEndBit!=13745u || r.parentHandle!=0xbcu
       || r.content.m_tableStyleHandle!=0x6bu || r.textSpans.size()!=45u || r.handleSpans.size()!=51u)return false;
    std::size_t nul=0;for(const auto& s:r.textSpans)if(!s.bytes.empty()&&s.bytes.back()==0u)++nul;
    if(nul!=7u)return false;
    std::cout<<"physical TABLECONTENT source spans matched; trailing NUL TVs="<<nul
             <<" work units="<<r.workUnits<<'\n';return true;
}
}
int main(int argc,char**argv){
    if(argc==3 && (std::string(argv[1])=="--emit-body" || std::string(argv[1])=="--emit-large-body")) {
        Options o;if(std::string(argv[1])=="--emit-large-body")o.oneLongText=true;
        const auto raw=build(o);std::ofstream f(argv[2],std::ios::binary|std::ios::trunc);
        f.write(reinterpret_cast<const char*>(raw.m_rawBytes.data()),raw.m_rawBytes.size());
        return f?0:1;
    }
    return argc<=2 && generated() && (argc==1 || physical(argv[1]))?0:1;
}
