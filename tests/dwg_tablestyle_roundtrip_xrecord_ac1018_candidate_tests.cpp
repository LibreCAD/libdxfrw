#include "intern/dwg_tablestyle_roundtrip_xrecord_ac1018_candidate.h"
#include "intern/dwgbufferw.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <new>
namespace fault { long remaining=-1;std::size_t count=0;bool active=false; }
void* operator new(std::size_t n){if(fault::active)++fault::count;if(fault::remaining==0){fault::remaining=-1;throw std::bad_alloc();}if(fault::remaining>0)--fault::remaining;if(void* p=std::malloc(n?n:1u))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p) noexcept{std::free(p);}void operator delete[](void* p) noexcept{std::free(p);}void operator delete(void* p,std::size_t) noexcept{std::free(p);}void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
using namespace DwgTableStyleRoundtripXRecordAc1018Candidate;
namespace {
struct Options {std::uint32_t owner{0xdbu},reactor{0xdbu};std::uint8_t ownerRole{4u},reactorRole{4u},page{45u};bool reservedLength{false},eed{false},extension{false},extraReactor{false};};
struct Data {DRW_UnsupportedObject raw;DRW_XRecord typed;Result expected;};
void handle(dwgBufferW& b,unsigned role,std::uint32_t value){dwgHandle h;h.code=role;h.ref=value;b.putHandle(h);}
void integer(dwgBufferW& b,unsigned code,int value){b.putRawShort16(code);b.putRawLong32(static_cast<std::uint32_t>(value));}
void text(dwgBufferW& b,unsigned code,const std::string& value,unsigned page){b.putRawShort16(code);b.putRawShort16(static_cast<std::uint16_t>(value.size()));b.putRawChar8(page);for(char c:value)b.putRawChar8(static_cast<std::uint8_t>(c));}
Data build(const Options& o={}) {
    Data f;dwgBufferW payload;auto& e=f.typed.m_dataEntries;
    const auto addText=[&](int code,const std::string& value){const auto i=e.size();f.expected.entries[i].begin=payload.bitCount();text(payload,code,value,o.page);f.expected.entries[i].end=payload.bitCount();e.emplace_back(code,UTF8STRING(value));};
    const auto addInt=[&](int code,int value){const auto i=e.size();f.expected.entries[i].begin=payload.bitCount();integer(payload,code,value);f.expected.entries[i].end=payload.bitCount();e.emplace_back(code,std::int32_t{value});};
    addText(102,"ACAD_ROUNDTRIP_PRE2007_TABLESTYLE");addInt(90,4);addInt(91,0);addText(1,"");addInt(92,4);addInt(93,0);addText(2,"");addInt(94,4);addInt(95,0);addText(3,"");
    dwgBufferW b;b.putObjType(DRW::AC1018,79u);const auto end=b.bitCount();b.putRawLong32(0u);
    handle(b,0u,0xdcu);b.putBitShort(o.eed?1u:0u);b.putBitLong(o.extraReactor?2:1);b.putBit(o.extension?0u:1u);if(o.reservedLength)b.put2Bits(3u);else b.putBitLong(static_cast<int>(payload.size()));
    for(auto& span:f.expected.entries){span.begin+=b.bitCount();span.end+=b.bitCount();}
    b.putBytes(payload.data().data(),payload.size());b.putBitShort(1u);f.expected.dataEndBit=static_cast<std::uint32_t>(b.bitCount());b.patchRawLong32AtBit(end,f.expected.dataEndBit);
    f.expected.ownerSpan.begin=b.bitCount();handle(b,o.ownerRole,o.owner);f.expected.ownerSpan.end=b.bitCount();
    f.expected.reactorSpan.begin=b.bitCount();handle(b,o.reactorRole,o.reactor);f.expected.reactorSpan.end=b.bitCount();b.alignToByte();
    f.expected.handle=0xdcu;f.expected.owner=o.owner;f.expected.reactor=o.reactor;
    f.raw.m_version=DRW::AC1018;f.raw.m_objectType=79;f.raw.m_handle=0xdcu;f.raw.m_rawBytes=b.data();f.raw.m_objectSize=static_cast<std::uint32_t>(b.size());
    f.typed.handle=0xdcu;f.typed.parentHandle=o.owner;f.typed.reactorHandles={o.reactor};f.typed.setDwgCommonObjectState(1,1u,false);f.typed.m_cloning=1;return f;
}
bool equal(const Result& a,const Result& b){if(a.handle!=b.handle || a.owner!=b.owner || a.reactor!=b.reactor || a.dataEndBit!=b.dataEndBit || a.ownerSpan.begin!=b.ownerSpan.begin || a.ownerSpan.end!=b.ownerSpan.end || a.reactorSpan.begin!=b.reactorSpan.begin || a.reactorSpan.end!=b.reactorSpan.end)return false;for(std::size_t i=0;i<a.entries.size();++i)if(a.entries[i].begin!=b.entries[i].begin || a.entries[i].end!=b.entries[i].end)return false;return true;}
bool refused(const Data& f){Result r;r.handle=1u;r.entries[0].begin=5u;return !inspect(f.raw,f.typed,r) && r.handle==0u && r.entries[0].begin==0u;}
}
int main(int argc,char** argv){
    auto f=build();Result r;if(!inspect(f.raw,f.typed,r)||!equal(r,f.expected))return 1;
    if(argc==2){std::ifstream in(argv[1],std::ios::binary);if(!in)return 2;f.raw.m_rawBytes={std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};f.raw.m_objectSize=static_cast<std::uint32_t>(f.raw.m_rawBytes.size());return inspect(f.raw,f.typed,r)&&equal(r,f.expected)?0:3;}
    const auto bytes=f.raw.m_rawBytes;for(std::size_t i=0;i<bytes.size();++i){f.raw.m_rawBytes.assign(bytes.begin(),bytes.begin()+i);f.raw.m_objectSize=static_cast<std::uint32_t>(i);if(!refused(f))return 4;}
    Options o;o.ownerRole=5u;if(!refused(build(o)))return 5;o={};o.reactorRole=5u;if(!refused(build(o)))return 6;o={};o.reactor=0xdau;if(!refused(build(o)))return 7;o={};o.owner=0xdcu;if(!refused(build(o)))return 8;o={};o.page=30u;if(!refused(build(o)))return 9;
    f=build();std::swap(f.typed.m_dataEntries[1],f.typed.m_dataEntries[2]);if(!refused(f))return 10;
    f=build();f.typed.m_dataEntries[0]=DRW_Variant(102,UTF8STRING("wrong"));if(!refused(f))return 11;
    f=build();f.typed.reactorHandles[0]=0xdau;if(!refused(f))return 12;
    f=build();f.typed.m_values.emplace_back(90,std::int32_t{9});f.typed.m_handleValues.emplace_back(360,42u);if(!inspect(f.raw,f.typed,r))return 13;
    f=build();f.raw.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::Validated);f.raw.m_parentHandle=0xdbu;f.raw.m_numReactors=1;f.raw.m_reactorHandles={0xdbu};f.raw.m_xDictFlag=1u;f.raw.m_xDictHandle=DRW::NoHandle;if(!inspect(f.raw,f.typed,r))return 14;f.raw.m_parentHandle=0xdau;if(!refused(f))return 15;
    o={};o.reservedLength=true;if(!refused(build(o)))return 19;o={};o.eed=true;if(!refused(build(o)))return 20;o={};o.extraReactor=true;if(!refused(build(o)))return 21;o={};o.extension=true;if(!refused(build(o)))return 22;
    f=build();f.raw.m_commonHandleDataValidated=true;if(!refused(f))return 23;
    f=build();f.raw.m_rawBytes.back()|=1u;if(!refused(f))return 24;
    f=build();f.raw.m_rawBytes.push_back(0u);++f.raw.m_objectSize;if(!refused(f))return 25;
    f=build();f.typed.m_cloning=2;if(!refused(f))return 26;
    f=build();f.raw.m_rawBytes.resize(4097u);f.raw.m_objectSize=4097u;if(!refused(f))return 27;
    f=build();f.raw.m_version=DRW::AC1021;if(!refused(f))return 28;
    f=build();f.raw.m_rawBytes[14]^=0x40u;if(!refused(f))return 29;
    f=build();fault::count=0;fault::active=true;const bool good=inspect(f.raw,f.typed,r);fault::active=false;const auto n=fault::count;if(!good||!n)return 16;
    for(std::size_t i=0;i<n;++i){fault::remaining=static_cast<long>(i);if(!refused(f))return 17;fault::remaining=-1;if(!inspect(f.raw,f.typed,r))return 18;}
    return 0;
}
