#include "intern/dwg_cellstylemap_ac1018_candidate.h"
#include "intern/dwgbufferw.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <new>
namespace fault {long left=-1;std::size_t count=0;bool active=false;}
void* operator new(std::size_t n){if(fault::active)++fault::count;if(fault::left==0){fault::left=-1;throw std::bad_alloc();}
    if(fault::left>0)--fault::left;if(auto p=std::malloc(n?n:1u))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
namespace {
using DwgCellStyleMapAc1018Candidate::Result;
struct Options {
    std::uint8_t ownerRole{4u},reactorRole{4u},textRole{5u},borderRole{5u};
    std::uint32_t owner{0xdbu},text{0x11u},ltype{0x14u};
    int adjustment{0};bool reordered{false},lateBorder{false},badColor{false},badPadding{false};
    bool eed{false},xdict{false},reactors{false},reserved{false},negativeZero{false};
};
void handle(dwgBufferW& b,std::uint8_t role,std::uint32_t value){dwgHandle h;h.code=role;h.ref=value;b.putHandle(h);}
void cmc(dwgBufferW& b,std::uint32_t value){b.putBitShort(0u);b.putBitLong(static_cast<std::int32_t>(value));b.putRawChar8(0u);}
void text(dwgBufferW& b,const char* value){const std::string s(value);b.putBitShort(static_cast<std::uint16_t>(s.size()));for(auto c:s)b.putRawChar8(static_cast<std::uint8_t>(c));}
DRW_UnsupportedObject build(const Options& o={}) {
    dwgBufferW b;b.putObjType(DRW::AC1018,508u);const auto end=b.bitCount();b.putRawLong32(0u);
    handle(b,0u,0xddu);b.putBitShort(o.eed?1u:0u);b.putBitLong(o.reactors?0:1);b.putBit(o.xdict?0u:1u);b.putBitLong(3);
    const int ids[]{3,1,2},classes[]{2,1,1},align[]{2,5,5},merge[]{0,32768,0};
    const double heights[]{4.5,6.0,4.5};const char* names[]{"_DATA","_TITLE","_HEADER"};
    for(int i=0;i<3;++i){b.putBitLong(5);b.putBitShort(1u);b.putBitLong(0);b.putBitLong(merge[i]);
        cmc(b,o.badColor&&i==1?0xc8000001u:0xc8000000u);b.putBitLong(1);
        b.putBitLong(0);b.putBitLong(0);b.putBitLong(4);b.putBitLong(0);text(b,"");
        b.putBitDouble(o.negativeZero&&i==1?-0.0:0.0);b.putBitDouble(1.0);b.putBitLong(align[i]);
        cmc(b,0xc1000000u);b.putBitDouble(heights[i]);b.putBitShort(1u);
        for(int j=0;j<6;++j)b.putBitDouble(1.5);b.putBitLong(6);
        for(int j=0;j<6;++j){if(o.reserved&&i==2&&j==5)b.put2Bits(3u);else b.putBitLong(1<<j);
            b.putBitLong(0);b.putBitLong(1);cmc(b,0xc1000000u);b.putBitLong(-2);b.putBitLong(0);b.putBitDouble(0.045);}
        b.putBitLong(o.reordered&&i==0?1:ids[i]);b.putBitLong(classes[i]);text(b,names[i]);}
    b.patchRawLong32AtBit(end,static_cast<std::uint32_t>(static_cast<std::int64_t>(b.bitCount())+o.adjustment));
    handle(b,o.ownerRole,o.owner);handle(b,o.reactorRole,o.owner);
    for(int i=0;i<3;++i){handle(b,o.textRole,o.text);for(int j=0;j<6;++j)handle(b,o.lateBorder&&i==2&&j==5?4u:o.borderRole,o.ltype);}
    b.alignToByte();DRW_UnsupportedObject r;r.m_version=DRW::AC1018;r.m_objectType=508;r.m_handle=0xddu;
    r.m_isCustomClass=r.m_hasClassDefinition=true;r.m_recordName="CELLSTYLEMAP";r.m_className="AcDbCellStyleMap";
    r.m_rawBytes=b.data();if(o.badPadding)r.m_rawBytes.back()|=1u;r.m_objectSize=static_cast<std::uint32_t>(r.m_rawBytes.size());return r;
}
DRW_CellStyleMap typed(const Options& o={}) {
    DRW_CellStyleMap t;t.handle=0xddu;t.parentHandle=o.owner;t.reactorHandles={o.owner};t.setDwgCommonObjectState(1,1u,false);
    const int ids[]{3,1,2},classes[]{2,1,1},align[]{2,5,5},merge[]{0,32768,0};
    const double heights[]{4.5,6.0,4.5};const char* names[]{"_DATA","_TITLE","_HEADER"};
    for(int i=0;i<3;++i){DRW_TableStyleCellStyle s;s.m_id=ids[i];s.m_styleClass=classes[i];s.m_name=names[i];s.m_type=5;s.m_hasData=true;
        s.m_mergeFlags=static_cast<std::uint32_t>(merge[i]);s.m_backgroundColor=256;s.m_backgroundColorCmc.m_method=0xc8u;
        s.m_backgroundColorCmc.setRawDwgPackedValue(0xc8000000u);s.m_contentLayout=1;s.m_marginOverrideFlags=1;
        s.m_verticalMargin=s.m_horizontalMargin=s.m_bottomMargin=s.m_rightMargin=s.m_marginHorizontalSpacing=s.m_marginVerticalSpacing=1.5;
        auto& f=s.m_contentFormat;f.m_valueDataType=4;f.m_blockScale=1.;f.m_cellAlignment=align[i];f.m_textHeight=heights[i];
        f.m_textStyleHandle=o.text;f.m_contentColorCmc.m_method=0xc1u;
        for(int j=0;j<6;++j){DRW_TableStyleBorder e;e.m_edgeFlags=1<<j;e.m_borderType=1;e.m_lineWeight=-2;
            e.m_lineTypeHandle=o.ltype;e.m_doubleLineSpacing=0.045;e.m_colorCmc.m_method=0xc1u;s.m_borders.push_back(e);}t.m_cellStyles.push_back(s);}return t;
}
bool clear(const Result& r){if(r.classNumber||r.handle||r.owner||r.dataEndBit||r.ownerStartBit||r.handleEndBit)return false;
    for(const auto& s:r.styles){if(s.id||s.styleClass||s.startBit||s.endBit||s.textStyle.target||s.textStyle.startBit||s.textStyle.endBit)return false;
        for(const auto& h:s.linetypes)if(h.target||h.startBit||h.endBit)return false;}return true;}
bool reject(const DRW_UnsupportedObject& raw,const DRW_CellStyleMap& t){Result r;r.handle=1u;return !DwgCellStyleMapAc1018Candidate::inspect(raw,t,r)&&clear(r);}
bool valid(const Result& r){if(r.classNumber!=508u||r.handle!=0xddu||r.owner!=0xdbu||r.dataEndBit!=5287u||r.ownerStartBit!=5287u||r.handleEndBit!=5655u)return false;
    const int ids[]{3,1,2},cls[]{2,1,1};const std::uint64_t begins[]{89,1803,3557},ends[]{1803,3557,5287};
    for(std::size_t i=0;i<3u;++i){const auto& s=r.styles[i];if(s.id!=ids[i]||s.styleClass!=cls[i]||s.startBit!=begins[i]||s.endBit!=ends[i]
        ||s.textStyle.target!=0x11u||s.textStyle.startBit!=5319u+i*112u||s.textStyle.endBit!=5335u+i*112u)return false;
        for(std::size_t j=0;j<6u;++j)if(s.linetypes[j].target!=0x14u||s.linetypes[j].startBit!=5335u+i*112u+j*16u
            ||s.linetypes[j].endBit!=5351u+i*112u+j*16u)return false;}return true;}
bool generated(){auto raw=build();auto model=typed();Result result;if(!DwgCellStyleMapAc1018Candidate::inspect(raw,model,result)||!valid(result))return false;
    const auto original=raw.m_rawBytes;for(std::size_t n=0;n<original.size();++n){raw.m_rawBytes.assign(original.begin(),original.begin()+n);raw.m_objectSize=static_cast<std::uint32_t>(n);if(!reject(raw,model))return false;}
    raw=build();raw.m_className="Other";if(!reject(raw,model))return false;raw=build();raw.m_bodyBitSize=1u;if(!reject(raw,model))return false;
    raw=build();raw.m_isEntity=true;if(!reject(raw,model))return false;raw=build();raw.m_version=DRW::AC1021;if(!reject(raw,model))return false;
    raw=build();++raw.m_objectSize;if(!reject(raw,model))return false;raw=build();raw.m_rawBytes.push_back(0u);++raw.m_objectSize;if(!reject(raw,model))return false;
    Options o;o.ownerRole=5u;if(!reject(build(o),typed(o)))return false;o={};o.reactorRole=5u;if(!reject(build(o),typed(o)))return false;
    o={};o.textRole=4u;if(!reject(build(o),typed(o)))return false;o={};o.borderRole=4u;if(!reject(build(o),typed(o)))return false;
    o={};o.lateBorder=true;if(!reject(build(o),typed(o)))return false;o={};o.reordered=true;if(!reject(build(o),typed(o)))return false;
    o={};o.badColor=true;if(!reject(build(o),typed(o)))return false;o={};o.badPadding=true;if(!reject(build(o),typed(o)))return false;
    o={};o.eed=true;if(!reject(build(o),typed(o)))return false;o={};o.xdict=true;if(!reject(build(o),typed(o)))return false;
    o={};o.reactors=true;if(!reject(build(o),typed(o)))return false;o={};o.reserved=true;if(!reject(build(o),typed(o)))return false;
    o={};o.negativeZero=true;if(!reject(build(o),typed(o)))return false;o={};o.adjustment=-1;if(!reject(build(o),typed(o)))return false;
    o={};o.adjustment=1;if(!reject(build(o),typed(o)))return false;o={};o.ltype=0u;if(!reject(build(o),typed(o)))return false;
    raw=build();model=typed();model.m_cellStyles[1].m_contentFormat.m_textHeight=4.5;if(!reject(raw,model))return false;
    model=typed();model.m_cellStyles[2].m_borders[5].m_lineTypeHandle=0x15u;if(!reject(raw,model))return false;
    model=typed();model.m_cellStyles[1].m_backgroundColorCmc.setRawDwgPackedValue(0xc8000001u);if(!reject(raw,model))return false;
    model=typed();model.m_cellStyles[0].m_borders[0].m_colorCmc.m_rgb=0;if(!reject(raw,model))return false;
    model=typed();model.m_subrecordRanges.push_back({});if(!DwgCellStyleMapAc1018Candidate::inspect(raw,model,result))return false;
    model=typed();raw.m_commonHandleDataValidated=true;if(!reject(raw,model))return false;
    raw=build();raw.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::Validated);raw.m_parentHandle=0xdbu;raw.m_numReactors=1;
    raw.m_reactorHandles={0xdbu};raw.m_xDictFlag=1u;if(!DwgCellStyleMapAc1018Candidate::inspect(raw,model,result))return false;
    raw.m_parentHandle=0xdcu;if(!reject(raw,model))return false;
    o={};o.owner=0x1dbu;o.text=0x111u;o.ltype=0x114u;raw=build(o);model=typed(o);
    if(!DwgCellStyleMapAc1018Candidate::inspect(raw,model,result)||result.owner!=o.owner
        ||result.styles[0].textStyle.target!=o.text||result.styles[2].linetypes[5].target!=o.ltype)return false;
    raw=build();model=typed();fault::active=true;fault::count=0;bool ok=DwgCellStyleMapAc1018Candidate::inspect(raw,model,result);fault::active=false;
    const auto count=fault::count;if(!ok||!valid(result)||count>32u)return false;
    for(std::size_t i=0;i<count;++i){fault::left=static_cast<long>(i);result.handle=9u;ok=DwgCellStyleMapAc1018Candidate::inspect(raw,model,result);fault::left=-1;
        if(ok||!clear(result)||raw.m_rawBytes!=original||!DwgCellStyleMapAc1018Candidate::inspect(raw,model,result))return false;}
    std::cout<<"generated CELLSTYLEMAP controls passed; allocation faults="<<count<<'\n';return true;}
bool physical(const char* path){std::ifstream in(path,std::ios::binary);auto raw=build();raw.m_rawBytes.assign(std::istreambuf_iterator<char>(in),{});
    raw.m_objectSize=static_cast<std::uint32_t>(raw.m_rawBytes.size());Result r;
    if(!in||!DwgCellStyleMapAc1018Candidate::inspect(raw,typed(),r)||!valid(r))return false;
    std::cout<<"caller-supplied body and reconstructed typed projection matched; provenance and CRC not checked\n";return true;}
}
int main(int argc,char** argv){if(argc==3&&std::string(argv[1])=="--emit-body"){const auto raw=build();std::ofstream out(argv[2],std::ios::binary);
    out.write(reinterpret_cast<const char*>(raw.m_rawBytes.data()),static_cast<std::streamsize>(raw.m_rawBytes.size()));return out?0:1;}
    return argc>2||!generated()||(argc==2&&!physical(argv[1]))?1:0;}
