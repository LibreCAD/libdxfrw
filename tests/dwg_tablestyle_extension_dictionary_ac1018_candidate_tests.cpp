#include "intern/dwg_tablestyle_extension_dictionary_ac1018_candidate.h"
#include "intern/dwgbufferw.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <new>
#include <string>
namespace fault {long remaining=-1;std::size_t count=0;bool active=false;}
void* operator new(std::size_t n){if(fault::active)++fault::count;if(fault::remaining==0){fault::remaining=-1;throw std::bad_alloc();}if(fault::remaining>0)--fault::remaining;if(void* p=std::malloc(n?n:1u))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p) noexcept{std::free(p);}void operator delete[](void* p) noexcept{std::free(p);}void operator delete(void* p,std::size_t) noexcept{std::free(p);}void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
namespace {
using namespace DwgTableStyleExtensionDictionaryAc1018Candidate;
struct Options{std::uint32_t self{0xdbu},owner{0x6bu},first{0xdcu},second{0xddu};std::uint8_t ownerRole{4u},firstRole{2u},secondRole{2u};bool reverse{false},reserved{false};};
struct Fixture{DRW_UnsupportedObject raw;DRW_Dictionary typed;};
void handle(dwgBufferW& b,std::uint8_t role,std::uint32_t value){dwgHandle h;h.code=role;h.ref=value;b.putHandle(h);}
void text(dwgBufferW& b,const char* value){const std::string s(value);b.putBitShort(static_cast<std::uint16_t>(s.size()));for(auto c:s)b.putRawChar8(static_cast<std::uint8_t>(c));}
Fixture build(const Options& o={}) {
 Fixture f;dwgBufferW b;b.putObjType(DRW::AC1018,42u);const auto end=b.bitCount();b.putRawLong32(0u);
 handle(b,0u,o.self);b.putBitShort(0u);b.putBitLong(0);b.putBit(1u);if(o.reserved)b.put2Bits(3u);else b.putBitLong(2);
 b.putBitShort(1u);b.putRawChar8(1u);const char* a="ACAD_XREC_ROUNDTRIP";const char* c="ACAD_ROUNDTRIP_2008_TABLESTYLE_CELLSTYLEMAP";
 text(b,o.reverse?c:a);text(b,o.reverse?a:c);b.patchRawLong32AtBit(end,static_cast<std::uint32_t>(b.bitCount()));
 handle(b,o.ownerRole,o.owner);handle(b,o.firstRole,o.first);handle(b,o.secondRole,o.second);b.alignToByte();
 f.raw.m_version=DRW::AC1018;f.raw.m_objectType=42;f.raw.m_handle=o.self;f.raw.m_rawBytes=b.data();f.raw.m_objectSize=static_cast<std::uint32_t>(f.raw.m_rawBytes.size());
 f.typed.handle=o.self;f.typed.parentHandle=o.owner;f.typed.cloning=1;f.typed.hardOwner=1;f.typed.setDwgCommonObjectState(0,1u,false);
 f.typed.m_entries.push_back({o.reverse?c:a,o.first});f.typed.m_entries.push_back({o.reverse?a:c,o.second});return f;
}
bool refused(const Fixture& f){Result r;r.handle=1;return !inspect(f.raw,f.typed,r)&&!r.handle;}
}
int main(int argc,char**argv){
 if(argc==3 && (std::string(argv[1])=="--emit-body" || std::string(argv[1])=="--emit-bad-role"
     || std::string(argv[1])=="--emit-swapped" || std::string(argv[1])=="--emit-reversed")){
  Options o;if(std::string(argv[1])=="--emit-bad-role")o.firstRole=3u;
  if(std::string(argv[1])=="--emit-swapped")std::swap(o.first,o.second);
  if(std::string(argv[1])=="--emit-reversed")o.reverse=true;const auto f=build(o);std::ofstream out(argv[2],std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char*>(f.raw.m_rawBytes.data()),f.raw.m_rawBytes.size());return out?0:1;}
 if(argc!=1)return 2;auto f=build();Result r;
 if(!inspect(f.raw,f.typed,r)||r.handle!=0xdbu||r.owner!=0x6bu||r.roundtrip!=0xdcu||r.cellStyleMap!=0xddu
    ||r.dataEndBit!=607u||r.ownerStartBit!=607u||r.roundtripStartBit!=623u||r.mapStartBit!=639u||r.mapEndBit!=655u)return 3;
 const auto original=f.raw.m_rawBytes;for(std::size_t i=0;i<original.size();++i){f.raw.m_rawBytes.assign(original.begin(),original.begin()+i);f.raw.m_objectSize=static_cast<std::uint32_t>(i);if(!refused(f))return 4;}
 Options o;o.ownerRole=3;if(!refused(build(o)))return 5;o={};o.firstRole=3;if(!refused(build(o)))return 6;o={};o.secondRole=5;if(!refused(build(o)))return 7;
 o={};o.reverse=true;if(!refused(build(o)))return 8;o={};o.reserved=true;if(!refused(build(o)))return 9;
 o={};o.first=o.second;if(!refused(build(o)))return 10;o={};o.first=o.owner;if(!refused(build(o)))return 11;
 f=build();std::swap(f.typed.m_entries[0],f.typed.m_entries[1]);if(!refused(f))return 12;
 f=build();f.typed.hardOwner=0;if(!refused(f))return 13;f=build();f.typed.parentHandle++;if(!refused(f))return 14;
 f=build();f.raw.m_rawBytes.resize(4097u);f.raw.m_objectSize=4097u;if(!refused(f))return 15;
 f=build();f.raw.m_rawBytes.back()|=1u;if(!refused(f))return 16;
 f=build();fault::count=0;fault::active=true;const bool good=inspect(f.raw,f.typed,r);fault::active=false;const auto count=fault::count;if(!good||!count)return 17;
 for(std::size_t i=0;i<count;++i){fault::remaining=static_cast<long>(i);if(!refused(f))return 18;fault::remaining=-1;if(!inspect(f.raw,f.typed,r))return 19;}
 return 0;
}
