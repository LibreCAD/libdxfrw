#include "intern/dwg_table_entity_xdictionary_ac1018_candidate.h"
#include "intern/dwgbufferw.h"
#include <cstdlib>
#include <new>
#include <fstream>
#include <string>
namespace fault {long remaining=-1;std::size_t count=0;bool active=false;}
void* operator new(std::size_t n){if(fault::active)++fault::count;if(fault::remaining==0){fault::remaining=-1;throw std::bad_alloc();}if(fault::remaining>0)--fault::remaining;if(void* p=std::malloc(n?n:1u))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p) noexcept{std::free(p);}void operator delete[](void* p) noexcept{std::free(p);}void operator delete(void* p,std::size_t) noexcept{std::free(p);}void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
namespace {
using namespace DwgTableEntityXDictionaryAc1018Candidate;
struct Options {std::uint16_t type{513u};std::uint32_t identity{0x9cu},dictionary{0xbbu},proxy{2104u};std::uint8_t role{3u};bool missing{false},reactors{false},reserved{false};};
void handle(dwgBufferW& b,std::uint8_t code,std::uint32_t ref){dwgHandle h;h.code=code;h.ref=ref;b.putHandle(h);}
DRW_UnsupportedObject build(const Options& o={}) {
    dwgBufferW b;b.putObjType(DRW::AC1018,o.type);const auto end=b.bitCount();b.putRawLong32(0u);
    handle(b,0u,o.identity);b.putBitShort(0u);b.putBit(1u);b.putRawLong32(o.proxy);
    for(unsigned i=0;i<2104u;++i)b.putRawChar8(0xa5u);
    b.put2Bits(2u);if(o.reserved)b.put2Bits(3u);else b.putBitLong(o.reactors?1:0);b.putBit(o.missing?1u:0u);
    b.putBitShort(256u);b.put2Bits(1u);b.put2Bits(0u);b.put2Bits(0u);b.putBitShort(0u);b.putRawChar8(29u);
    for(unsigned i=0;i<11u;++i)b.putBit(0u);
    b.patchRawLong32AtBit(end,static_cast<std::uint32_t>(b.bitCount()));handle(b,o.role,o.dictionary);
    handle(b,5u,0x10u);handle(b,5u,0x9du);handle(b,5u,0x6bu);b.alignToByte();
    DRW_UnsupportedObject r;r.m_version=DRW::AC1018;r.m_objectType=o.type;r.m_handle=o.identity;
    r.m_isEntity=r.m_isCustomClass=r.m_hasClassDefinition=true;r.m_recordName="ACAD_TABLE";r.m_className="AcDbTable";
    r.m_rawBytes=b.data();r.m_objectSize=static_cast<std::uint32_t>(r.m_rawBytes.size());return r;
}
bool refused(const DRW_UnsupportedObject& raw){Result r;r.handle=1u;return !inspect(raw,r)&&!r.handle;}
}
int main(int argc,char**argv){
    if(argc==3 && std::string(argv[1])=="--emit-body") {const auto sample=build();std::ofstream f(argv[2],std::ios::binary|std::ios::trunc);
        f.write(reinterpret_cast<const char*>(sample.m_rawBytes.data()),sample.m_rawBytes.size());return f?0:1;}
    if(argc!=1)return 17;
    auto raw=build();Result r;if(!inspect(raw,r)||r.handle!=0x9cu||r.dictionary!=0xbbu||r.layer!=0x10u||r.block!=0x9du||r.style!=0x6bu)return 1;
    const auto original=raw.m_rawBytes;
    for(std::size_t i=0;i<original.size();++i){raw.m_rawBytes.assign(original.begin(),original.begin()+i);raw.m_objectSize=static_cast<std::uint32_t>(i);if(!refused(raw))return 2;}
    Options o;o.role=4u;if(!refused(build(o)))return 3;o={};o.missing=true;if(!refused(build(o)))return 4;
    o={};o.reactors=true;if(!refused(build(o)))return 5;o={};o.reserved=true;if(!refused(build(o)))return 6;
    o={};o.proxy=0u;if(!refused(build(o)))return 7;o={};o.proxy=0xffffffffu;if(!refused(build(o)))return 8;
    o={};o.dictionary=0u;if(!refused(build(o)))return 9;o={};o.dictionary=o.identity;if(!refused(build(o)))return 10;
    raw=build();++raw.m_handle;if(!refused(raw))return 11;raw=build();raw.m_className="Other";if(!refused(raw))return 12;
    raw=build();raw.m_rawBytes.resize(65537u);raw.m_objectSize=65537u;if(!refused(raw))return 13;
    raw=build();fault::count=0;fault::active=true;const bool measured=inspect(raw,r);fault::active=false;
    const auto count=fault::count;if(!measured||!count)return 14;
    for(std::size_t i=0;i<count;++i){fault::remaining=static_cast<long>(i);if(!refused(raw))return 15;fault::remaining=-1;if(!inspect(raw,r))return 16;}
    return 0;
}
