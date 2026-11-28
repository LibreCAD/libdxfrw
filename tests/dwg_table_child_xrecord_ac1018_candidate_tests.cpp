#include "table_child_candidate_fixture.h"
#include <algorithm>
#include <cstdlib>
#include <new>
namespace fault {long remaining=-1;std::size_t count=0;bool active=false;}
void* operator new(std::size_t n){if(fault::active)++fault::count;if(fault::remaining==0){fault::remaining=-1;throw std::bad_alloc();}if(fault::remaining>0)--fault::remaining;if(void* p=std::malloc(n?n:1u))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p) noexcept{std::free(p);}void operator delete[](void* p) noexcept{std::free(p);}void operator delete(void* p,std::size_t) noexcept{std::free(p);}void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
using namespace DwgTableChildXRecordAc1018Candidate;
bool refused(const ChildFixture::Data& f){Result r;r.handle=1u;r.text="x";return !inspect(f.raw,f.typed,r)&&!r.handle&&r.text.empty();}
int main(){
    auto f=ChildFixture::build();Result r;if(!inspect(f.raw,f.typed,r)||r.handle!=0xbeu||r.owner!=0xbdu||!r.hasText||r.text!="Title")return 1;
    const auto original=f.raw.m_rawBytes;
    for(std::size_t i=0;i<original.size();++i){f.raw.m_rawBytes.assign(original.begin(),original.begin()+i);f.raw.m_objectSize=static_cast<std::uint32_t>(i);if(!refused(f))return 2;}
    ChildFixture::Options o;o.empty=true;f=ChildFixture::build(o);if(!inspect(f.raw,f.typed,r)||r.hasText||!r.text.empty())return 3;
    o={};o.role=5u;if(!refused(ChildFixture::build(o)))return 4;
    o={};o.page=30u;if(!refused(ChildFixture::build(o)))return 5;
    o={};o.owner=o.handle;if(!refused(ChildFixture::build(o)))return 6;
    o={};o.value=std::string(65,'x');if(!refused(ChildFixture::build(o)))return 7;
    f=ChildFixture::build();f.typed.m_dataEntries[2]=DRW_Variant(1,UTF8STRING("wrong"));if(!refused(f))return 8;
    f=ChildFixture::build();std::swap(f.typed.m_dataEntries[0],f.typed.m_dataEntries[1]);if(!refused(f))return 9;
    f=ChildFixture::build();f.typed.handle++;if(!refused(f))return 10;
    f=ChildFixture::build();f.typed.m_values.emplace_back(90,std::int32_t{9});f.typed.m_handleValues.emplace_back(360,42u);
    if(!inspect(f.raw,f.typed,r))return 11;
    f=ChildFixture::build();fault::count=0;fault::active=true;const bool good=inspect(f.raw,f.typed,r);fault::active=false;
    const auto count=fault::count;if(!good||count==0u)return 12;
    for(std::size_t i=0;i<count;++i){fault::remaining=static_cast<long>(i);if(!refused(f))return 13;fault::remaining=-1;if(!inspect(f.raw,f.typed,r))return 14;}
    return 0;
}
