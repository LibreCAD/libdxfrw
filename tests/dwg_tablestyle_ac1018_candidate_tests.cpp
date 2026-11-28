#include "intern/dwg_tablestyle_ac1018_candidate.h"
#include "intern/dwgbufferw.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
namespace fault {long left=-1;std::size_t count=0;bool active=false;}
void* operator new(std::size_t n){if(fault::active)++fault::count;if(fault::left==0){fault::left=-1;throw std::bad_alloc();}if(fault::left>0)--fault::left;if(auto p=std::malloc(n?n:1u))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept{std::free(p);} void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);} void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
namespace {
using DwgTableStyleAc1018Candidate::Result;
struct Options {std::uint8_t ownerRole=4u,reactorRole=4u,xdictRole=3u,textRole=5u;std::uint32_t owner=0x6au,table=0x9cu,xdict=0xdbu,text=0x11u;int adjustment=0;bool reverse=false,badColor=false,badName=false;};
void handle(dwgBufferW& b,std::uint8_t role,std::uint32_t value){dwgHandle h;h.code=role;h.ref=value;b.putHandle(h);}
void color(dwgBufferW& b,std::uint32_t v){b.putBitShort(0u);b.putBitLong(static_cast<std::int32_t>(v));b.putRawChar8(0u);}
DRW_UnsupportedObject build(const Options& o={}) {
    dwgBufferW b;b.putObjType(DRW::AC1018,505u);const auto end=b.bitCount();b.putRawLong32(0u);
    handle(b,0u,0x6bu);b.putBitShort(0u);b.putBitLong(2);b.putBit(false);
    b.putBitShort(8u);for(const auto c:std::string(o.badName?"Standarx":"Standard"))b.putRawChar8(static_cast<std::uint8_t>(c));b.putBitShort(0u);b.putBitShort(0u);
    b.putBitDouble(1.5);b.putBitDouble(1.5);b.putBit(false);b.putBit(false);
    const double heights[]{4.5,6.0,4.5};const int align[]{2,5,5};
    for(std::size_t i=0;i<3u;++i){b.putBitDouble(heights[i]);b.putBitShort(align[i]);color(b,0xc1000000u);
        color(b,o.badColor && i==1u?0xc8000001u:0xc8000000u);b.putBit(false);
        for(int j=0;j<6;++j){b.putBitShort(65534u);b.putBit(true);color(b,0xc1000000u);}}
    b.patchRawLong32AtBit(end,static_cast<std::uint32_t>(static_cast<std::int64_t>(b.bitCount())+o.adjustment));
    handle(b,o.ownerRole,o.owner);handle(b,o.reactorRole,o.reverse?o.table:o.owner);
    handle(b,4u,o.reverse?o.owner:o.table);handle(b,o.xdictRole,o.xdict);
    for(int i=0;i<3;++i)handle(b,o.textRole,o.text);
    b.alignToByte();DRW_UnsupportedObject r;r.m_version=DRW::AC1018;r.m_objectType=505;
    r.m_handle=0x6bu;r.m_isCustomClass=r.m_hasClassDefinition=true;r.m_recordName="TABLESTYLE";r.m_className="AcDbTableStyle";
    r.m_rawBytes=b.data();r.m_objectSize=static_cast<std::uint32_t>(r.m_rawBytes.size());return r;
}
DRW_TableStyle typed(const Options& o={}) {
    DRW_TableStyle t;t.handle=0x6bu;t.parentHandle=o.owner;t.xDictHandle=o.xdict;t.reactorHandles={o.owner,o.table};
    t.setDwgCommonObjectState(2,0u,false);t.m_name="Standard";t.m_horizontalCellMargin=t.m_verticalCellMargin=1.5;
    const double heights[]{4.5,6.0,4.5};const int align[]{2,5,5};
    for(std::size_t i=0;i<3u;++i){DRW_TableStyleRowStyle row;row.m_textStyleHandle=o.text;row.m_textHeight=heights[i];row.m_textAlignment=align[i];
        row.m_fillColor=256;row.m_textColorCmc.m_method=0xc1u;row.m_fillColorCmc.m_method=0xc8u;row.m_fillColorCmc.setRawDwgPackedValue(0xc8000000u);
        for(int j=0;j<6;++j){DRW_TableStyleBorder e;e.m_edgeFlags=1<<j;e.m_lineWeight=-2;e.m_visible=1;e.m_colorCmc.m_method=0xc1u;row.m_borders.push_back(e);}t.m_rowStyles.push_back(row);}return t;
}
bool clear(const Result& r){return !r.classNumber&&!r.handle&&!r.owner&&!r.tableReactor&&!r.xdictionary&&!r.dataEndBit&&!r.ownerStartBit&&!r.xdictionaryStartBit&&!r.handleEndBit&&r.textStyles==std::array<std::uint32_t,3>{};}
bool reject(const DRW_UnsupportedObject& r,const DRW_TableStyle& t){Result out;out.handle=1;return !DwgTableStyleAc1018Candidate::inspect(r,t,out)&&clear(out);}
bool valid(const Result& r){return r.classNumber==505u&&r.handle==0x6bu&&r.owner==0x6au&&r.tableReactor==0x9cu&&r.xdictionary==0xdbu&&r.dataEndBit==1920u&&r.ownerStartBit==1920u&&r.xdictionaryStartBit==1968u&&r.handleEndBit==2032u&&r.textStyles==std::array<std::uint32_t,3>{{0x11u,0x11u,0x11u}};}
bool generated(){auto raw=build();auto model=typed();Result result;if(!DwgTableStyleAc1018Candidate::inspect(raw,model,result)||!valid(result))return false;
    const auto original=raw.m_rawBytes;for(std::size_t n=0;n<original.size();++n){raw.m_rawBytes.assign(original.begin(),original.begin()+n);raw.m_objectSize=static_cast<std::uint32_t>(n);if(!reject(raw,model))return false;}
    raw=build();raw.m_className="Other";if(!reject(raw,model))return false;raw=build();raw.m_bodyBitSize=1u;if(!reject(raw,model))return false;
    raw=build();raw.m_isEntity=true;if(!reject(raw,model))return false;raw=build();raw.m_objectType=499;if(!reject(raw,model))return false;
    raw=build();raw.m_handle=0u;if(!reject(raw,model))return false;raw=build();raw.m_version=DRW::AC1021;if(!reject(raw,model))return false;
    raw=build();raw.m_hasDataStorage=true;if(!reject(raw,model))return false;raw=build();++raw.m_objectSize;if(!reject(raw,model))return false;
    raw=build();raw.m_rawBytes.push_back(0u);++raw.m_objectSize;if(!reject(raw,model))return false;
    Options o;o.ownerRole=5u;if(!reject(build(o),typed(o)))return false;o={};o.reactorRole=5u;if(!reject(build(o),typed(o)))return false;
    o={};o.xdictRole=4u;if(!reject(build(o),typed(o)))return false;o={};o.textRole=4u;if(!reject(build(o),typed(o)))return false;
    o={};o.reverse=true;if(!reject(build(o),typed(o)))return false;o={};o.badColor=true;if(!reject(build(o),typed(o)))return false;
    o={};o.badName=true;if(!reject(build(o),typed(o)))return false;o={};o.adjustment=-1;if(!reject(build(o),typed(o)))return false;
    o={};o.adjustment=1;if(!reject(build(o),typed(o)))return false;
    o={};o.table=0x6au;if(!reject(build(o),typed(o)))return false;o={};o.xdict=0u;if(!reject(build(o),typed(o)))return false;
    raw=build();model=typed();model.m_rowStyles[1].m_textHeight=4.5;if(!reject(raw,model))return false;
    model=typed();model.m_rowStyles[2].m_borders[5].m_visible=0;if(!reject(raw,model))return false;
    // This observer is a selected wire projection, not full typed equality.
    model=typed();model.m_rowStyles[0].m_textStyleName="unobserved";model.m_r24ClassVersion=7u;
    if(!DwgTableStyleAc1018Candidate::inspect(raw,model,result))return false;
    model=typed();model.reactorHandles[1]=0x77u;if(!reject(raw,model))return false;
    model=typed();model.m_rowStyles[1].m_fillColorCmc.setRawDwgPackedValue(0xc8000001u);if(!reject(raw,model))return false;
    model=typed();model.m_rowStyles[2].m_textStyleHandle=0x12u;if(!reject(raw,model))return false;
    model=typed();raw.m_commonHandleDataValidated=true;if(!reject(raw,model))return false;
    raw=build();raw.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::Validated);raw.m_parentHandle=0x6au;raw.m_numReactors=2;raw.m_reactorHandles={0x6au,0x9cu};raw.m_xDictHandle=0xdbu;raw.m_xDictFlag=0u;
    if(!DwgTableStyleAc1018Candidate::inspect(raw,model,result))return false;raw.m_xDictHandle=0xddu;if(!reject(raw,model))return false;
    o={};o.owner=0x16au;o.table=0x19cu;o.xdict=0x1dbu;o.text=0x111u;raw=build(o);model=typed(o);
    if(!DwgTableStyleAc1018Candidate::inspect(raw,model,result)||result.owner!=o.owner||result.tableReactor!=o.table||result.xdictionary!=o.xdict||result.textStyles[0]!=o.text)return false;
    raw=build();model=typed();fault::active=true;fault::count=0;bool ok=DwgTableStyleAc1018Candidate::inspect(raw,model,result);fault::active=false;auto count=fault::count;if(!ok||!valid(result)||count>32)return false;
    for(std::size_t i=0;i<count;++i){fault::left=static_cast<long>(i);result.handle=8u;ok=DwgTableStyleAc1018Candidate::inspect(raw,model,result);fault::left=-1;if(ok||!clear(result)||raw.m_rawBytes!=original||!DwgTableStyleAc1018Candidate::inspect(raw,model,result))return false;}
    std::cout<<"generated TABLESTYLE controls passed; allocation faults="<<count<<'\n';return true;}
bool physical(const char* path){std::ifstream in(path,std::ios::binary);auto raw=build();raw.m_rawBytes.assign(std::istreambuf_iterator<char>(in),{});raw.m_objectSize=static_cast<std::uint32_t>(raw.m_rawBytes.size());Result r;
    if(!in||!DwgTableStyleAc1018Candidate::inspect(raw,typed(),r)||!valid(r))return false;
    std::cout<<"caller-supplied body and reconstructed typed profile matched; provenance not checked\n";return true;}
}
int main(int argc,char** argv){
    if(argc==3 && std::string(argv[1])=="--emit-body") {const auto r=build();std::ofstream out(argv[2],std::ios::binary);out.write(reinterpret_cast<const char*>(r.m_rawBytes.data()),static_cast<std::streamsize>(r.m_rawBytes.size()));return out?0:1;}
    return argc>2||!generated()||(argc==2&&!physical(argv[1]))?1:0;
}
