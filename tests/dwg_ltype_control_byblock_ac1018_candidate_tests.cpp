#include "intern/dwg_ltype_control_byblock_ac1018_candidate.h"
#include "intern/dwgbufferw.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
namespace fault {long remaining=-1;std::size_t count=0;bool active=false;}
void* operator new(std::size_t n){if(fault::active)++fault::count;if(fault::remaining==0){fault::remaining=-1;throw std::bad_alloc();}
    if(fault::remaining>0)--fault::remaining;
    if(void* p=std::malloc(n?n:1u))return p;
    throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
namespace {
using DwgLTypeControlByBlockAc1018Candidate::Result;
struct Options {
    std::uint32_t control{5u},row{0x14u},other{0x15u},ordinary{0x16u};
    std::uint8_t nullRole{4u},ordinaryRole{2u},phantomRole{3u},otherRole{3u},ownerRole{4u},xrefRole{5u};
    int controlEnd{0},rowEnd{0};bool reverse{false},eed{false},reactors{false},xdict{false};
    bool invalidCount{false},reservedCount{false},badName{false},badArea{false},badPadding{false},negativeZero{false};
};
void h(dwgBufferW& b,std::uint8_t role,std::uint32_t value){dwgHandle ref;ref.code=role;ref.ref=value;b.putHandle(ref);}
DRW_UnsupportedObject control(const Options& o={}){dwgBufferW b;b.putObjType(DRW::AC1018,56u);auto end=b.bitCount();b.putRawLong32(0u);
    h(b,0u,o.control);b.putBitShort(o.eed?1u:0u);b.putBitLong(o.reactors?1:0);b.putBit(o.xdict?0u:1u);
    if(o.reservedCount)b.put2Bits(3u);else b.putBitLong(o.invalidCount?2:1);
    b.patchRawLong32AtBit(end,static_cast<std::uint32_t>(static_cast<std::int64_t>(b.bitCount())+o.controlEnd));
    h(b,o.nullRole,0u);h(b,o.ordinaryRole,o.ordinary);h(b,o.phantomRole,o.reverse?o.other:o.row);h(b,o.otherRole,o.reverse?o.row:o.other);b.alignToByte();
    DRW_UnsupportedObject r;r.m_version=DRW::AC1018;r.m_handle=o.control;r.m_objectType=56;r.m_rawBytes=b.data();
    if(o.badPadding)r.m_rawBytes.back()|=1u;
    r.m_objectSize=static_cast<std::uint32_t>(r.m_rawBytes.size());return r;}
DRW_UnsupportedObject row(const Options& o={},bool uppercaseName=false){dwgBufferW b;b.putObjType(DRW::AC1018,57u);auto end=b.bitCount();b.putRawLong32(0u);
    h(b,0u,o.row);b.putBitShort(0u);b.putBitLong(0);b.putBit(true);b.putBitShort(7u);
    const std::string name=o.badName?"ByBlokk":(uppercaseName?"BYBLOCK":"ByBlock");for(auto c:name)b.putRawChar8(static_cast<std::uint8_t>(c));
    b.putBit(true);b.putBitShort(0u);b.putBit(false);b.putBitShort(0u);b.putBitDouble(o.negativeZero?-0.:0.);b.putRawChar8(65u);b.putRawChar8(0u);
    for(int i=0;i<256;++i)b.putRawChar8(o.badArea&&i==255?1u:0u);
    b.patchRawLong32AtBit(end,static_cast<std::uint32_t>(static_cast<std::int64_t>(b.bitCount())+o.rowEnd));
    h(b,o.ownerRole,o.control);h(b,o.xrefRole,0u);b.alignToByte();DRW_UnsupportedObject r;r.m_version=DRW::AC1018;
    r.m_handle=o.row;r.m_objectType=57;r.m_rawBytes=b.data();r.m_objectSize=static_cast<std::uint32_t>(r.m_rawBytes.size());return r;}
DRW_LType typed(const Options& o={}){DRW_LType t;t.handle=o.row;t.parentHandle=o.control;t.name="ByBlock";t.flags=0x40;t.size=0;t.length=0.;t.alignment=65;
    t.setDwgCommonObjectState(0,1u,false);t.xrefBlockHandle.code=5u;t.xrefBlockHandle.rawCode=5u;return t;}
bool clear(const Result& r){const auto empty=[](const auto& a){return !a.target&&!a.startBit&&!a.endBit;};return !r.control&&!r.byBlock&&!r.controlDataEndBit&&!r.ltypeDataEndBit
    &&empty(r.ordinary)&&empty(r.byBlockPhantom)&&empty(r.byLayerPhantom)&&empty(r.rowOwner)&&!r.controlHandleEndBit&&!r.ltypeHandleEndBit;}
bool reject(const DRW_UnsupportedObject& c,const DRW_UnsupportedObject& r,const DRW_LType& t){Result v;v.control=1u;return !DwgLTypeControlByBlockAc1018Candidate::inspect(c,r,t,v)&&clear(v);}
bool valid(const Result& r){return r.control==5u&&r.byBlock==0x14u&&r.controlDataEndBit==73u&&r.ltypeDataEndBit==2201u
    &&r.ordinary.target==0x16u&&r.ordinary.startBit==81u&&r.ordinary.endBit==97u
    &&r.byBlockPhantom.target==0x14u&&r.byBlockPhantom.startBit==97u&&r.byBlockPhantom.endBit==113u
    &&r.byLayerPhantom.target==0x15u&&r.byLayerPhantom.startBit==113u&&r.byLayerPhantom.endBit==129u
    &&r.rowOwner.target==5u&&r.rowOwner.startBit==2201u&&r.rowOwner.endBit==2217u
    &&r.controlHandleEndBit==129u&&r.ltypeHandleEndBit==2225u;}
bool generated(){auto c=control();auto r=row();auto t=typed();Result v;
    if(!DwgLTypeControlByBlockAc1018Candidate::inspect(c,r,t,v)||!valid(v))return false;
    const auto controlBytes=c.m_rawBytes,rowBytes=r.m_rawBytes;
    for(std::size_t n=0;n<controlBytes.size();++n){c.m_rawBytes.assign(controlBytes.begin(),controlBytes.begin()+n);c.m_objectSize=static_cast<std::uint32_t>(n);if(!reject(c,r,t))return false;}
    c=control();for(std::size_t n=0;n<rowBytes.size();++n){r.m_rawBytes.assign(rowBytes.begin(),rowBytes.begin()+n);r.m_objectSize=static_cast<std::uint32_t>(n);if(!reject(c,r,t))return false;}
    r=row();c.m_objectType=57;if(!reject(c,r,t))return false;c=control();r.m_isCustomClass=true;if(!reject(c,r,t))return false;
    r=row();r.m_bodyBitSize=1u;if(!reject(c,r,t))return false;r=row();++r.m_objectSize;if(!reject(c,r,t))return false;
    Options o;o.nullRole=3u;if(!reject(control(o),row(o),typed(o)))return false;o={};o.ordinaryRole=3u;if(!reject(control(o),row(o),typed(o)))return false;
    o={};o.phantomRole=2u;if(!reject(control(o),row(o),typed(o)))return false;o={};o.otherRole=2u;if(!reject(control(o),row(o),typed(o)))return false;
    o={};o.ownerRole=3u;if(!reject(control(o),row(o),typed(o)))return false;o={};o.xrefRole=4u;if(!reject(control(o),row(o),typed(o)))return false;
    o={};o.reverse=true;c=control(o);r=row(o,true);t=typed(o);t.name="BYBLOCK";
    if(!reject(c,r,t))return false;
    if(!DwgLTypeControlByBlockAc1018Candidate::inspectNormativeOrder(c,r,t,v)
        ||v.byLayerPhantom.target!=0x15u||v.byLayerPhantom.startBit!=97u||v.byLayerPhantom.endBit!=113u
        ||v.byBlockPhantom.target!=0x14u||v.byBlockPhantom.startBit!=113u||v.byBlockPhantom.endBit!=129u)return false;
    c=control();r=row();t=typed();
    if(DwgLTypeControlByBlockAc1018Candidate::inspectNormativeOrder(c,r,t,v)||!clear(v))return false;
    o={};o.invalidCount=true;if(!reject(control(o),row(o),typed(o)))return false;
    o={};o.reservedCount=true;if(!reject(control(o),row(o),typed(o)))return false;o={};o.eed=true;if(!reject(control(o),row(o),typed(o)))return false;
    o={};o.reactors=true;if(!reject(control(o),row(o),typed(o)))return false;o={};o.xdict=true;if(!reject(control(o),row(o),typed(o)))return false;
    o={};o.badName=true;if(!reject(control(o),row(o),typed(o)))return false;o={};o.badArea=true;if(!reject(control(o),row(o),typed(o)))return false;
    o={};o.badPadding=true;if(!reject(control(o),row(o),typed(o)))return false;o={};o.negativeZero=true;if(!reject(control(o),row(o),typed(o)))return false;
    o={};o.controlEnd=-1;if(!reject(control(o),row(o),typed(o)))return false;o={};o.rowEnd=1;if(!reject(control(o),row(o),typed(o)))return false;
    o={};o.other=o.row;if(!reject(control(o),row(o),typed(o)))return false;o={};o.ordinary=0u;if(!reject(control(o),row(o),typed(o)))return false;
    c=control();r=row();t=typed();t.length=1.;if(!reject(c,r,t))return false;t=typed();t.parentHandle=7u;if(!reject(c,r,t))return false;
    t=typed();t.xrefBlockHandle.rawCode=4u;if(!reject(c,r,t))return false;
    t=typed();t.xrefBlockHandle.code=4u;if(!reject(c,r,t))return false;
    t=typed();if(!DwgLTypeControlByBlockAc1018Candidate::inspect(c,r,t,v))return false;
    t.path.push_back(0.5);if(!reject(c,r,t))return false;
    t=typed();c.m_commonHandleDataValidated=true;if(!reject(c,r,t))return false;c=control();c.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::NotApplicable);
    if(!DwgLTypeControlByBlockAc1018Candidate::inspect(c,r,t,v))return false;
    r.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::NotApplicable);if(!reject(c,r,t))return false;
    r.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::Validated);r.m_parentHandle=5u;r.m_numReactors=0;r.m_xDictFlag=1u;
    if(!DwgLTypeControlByBlockAc1018Candidate::inspect(c,r,t,v))return false;
    r.m_parentHandle=6u;if(!reject(c,r,t))return false;
    // The two sibling targets are only observed references; no existence or
    // semantics are inferred, and unrelated typed state is not certified.
    o={};o.other=0x99u;o.ordinary=0x98u;c=control(o);r=row(o);t=typed(o);
    t.pendingAppIdResolutions.push_back({});
    if(!DwgLTypeControlByBlockAc1018Candidate::inspect(c,r,t,v)
        ||v.ordinary.target!=0x98u||v.byLayerPhantom.target!=0x99u)return false;
    o={};o.control=0x105u;o.row=0x114u;o.other=0x115u;o.ordinary=0x116u;c=control(o);r=row(o);t=typed(o);
    if(!DwgLTypeControlByBlockAc1018Candidate::inspect(c,r,t,v)||v.control!=o.control||v.byBlock!=o.row||v.ordinary.target!=o.ordinary)return false;
    c=control();r=row();t=typed();fault::active=true;fault::count=0;bool ok=DwgLTypeControlByBlockAc1018Candidate::inspect(c,r,t,v);fault::active=false;
    const auto count=fault::count;if(!ok||!valid(v)||count>32u)return false;
    for(std::size_t i=0;i<count;++i){fault::remaining=static_cast<long>(i);v.control=99u;ok=DwgLTypeControlByBlockAc1018Candidate::inspect(c,r,t,v);fault::remaining=-1;
        if(ok||!clear(v)||c.m_rawBytes!=controlBytes||r.m_rawBytes!=rowBytes||!DwgLTypeControlByBlockAc1018Candidate::inspect(c,r,t,v))return false;}
    std::cout<<"generated LTYPE_CONTROL/ByBlock controls passed; allocation faults="<<count<<'\n';return true;}
bool physical(const char* a,const char* b){std::ifstream x(a,std::ios::binary),y(b,std::ios::binary);auto c=control(),r=row();
    c.m_rawBytes.assign(std::istreambuf_iterator<char>(x),{});r.m_rawBytes.assign(std::istreambuf_iterator<char>(y),{});
    c.m_objectSize=static_cast<std::uint32_t>(c.m_rawBytes.size());r.m_objectSize=static_cast<std::uint32_t>(r.m_rawBytes.size());Result v;
    if(!x||!y||!DwgLTypeControlByBlockAc1018Candidate::inspect(c,r,typed(),v)||!valid(v))return false;
    std::cout<<"caller-supplied bodies and reconstructed typed projection matched; provenance and CRC not checked\n";return true;}
}
int main(int argc,char**argv){return (argc!=1&&argc!=3)||!generated()||(argc==3&&!physical(argv[1],argv[2]))?1:0;}
