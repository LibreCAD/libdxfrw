#include "intern/dwg_datatable_ac1018_candidate.h"
#include "intern/dwgbufferw.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <new>

namespace fault { long remaining=-1; std::size_t count=0; bool active=false; }
void* operator new(std::size_t n) {
    if (fault::active) ++fault::count;
    if (fault::remaining==0) {fault::remaining=-1;throw std::bad_alloc();}
    if (fault::remaining>0) --fault::remaining;
    if (void* p=std::malloc(n?n:1u)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) {return ::operator new(n);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void operator delete[](void* p,std::size_t) noexcept {std::free(p);}

namespace {
using DwgDataTableAc1018Candidate::Result;
struct Options {
    std::int32_t rows{3}; int badKind{-1};
    std::uint16_t headerValue{2u};
    std::uint8_t selfRole{0u},parentRole{4u},childRole{3u};
    bool eed{false},reactor{false},extension{false},nonemptyName{false};
    bool duplicate{false},nonfinite{false},reservedLong{false},reservedDouble{false};
};
void putHandle(dwgBufferW& b,std::uint8_t role,std::uint32_t value) {
    dwgHandle h; h.code=role; h.ref=value; b.putHandle(h);
}
DRW_UnsupportedObject build(const Options& o={}) {
    dwgBufferW b;
    b.putObjType(DRW::AC1018,517u);
    const auto endPos=b.bitCount(); b.putRawLong32(0u);
    putHandle(b,o.selfRole,0xbdu);
    b.putBitShort(o.eed?1u:0u);
    b.putBitLong(o.reactor?1:0); b.putBit(o.extension?0u:1u);
    b.putBitShort(o.headerValue);
    if (o.reservedLong) b.put2Bits(3u); else b.putBitLong(4);
    b.putBitLong(o.rows); b.putBitShort(o.nonemptyName?1u:0u);
    if (o.nonemptyName) b.putRawChar8('x');
    const auto actualRows=o.rows>0 && o.rows<=9 ? o.rows : 0;
    for (int col=0;col<4;++col) {
        b.putBitLong(col==o.badKind?7:col==0?2:col==3?6:1); b.putBitShort(0u);
        if (col==3) break;
        for (int row=0;row<actualRows;++row) {
            if (col==0 && o.reservedDouble && row==0) b.put2Bits(3u);
            else if (col==0) b.putBitDouble(o.nonfinite && row==0
                ?std::numeric_limits<double>::quiet_NaN():static_cast<double>(row+2));
            else b.putBitLong(col*100+row);
        }
    }
    b.patchRawLong32AtBit(endPos,static_cast<std::uint32_t>(b.bitCount()));
    putHandle(b,o.parentRole,0xbcu);
    for (int row=0;row<actualRows;++row)
        putHandle(b,o.childRole,0xbeu+static_cast<std::uint32_t>(o.duplicate?0:row));
    b.alignToByte();
    DRW_UnsupportedObject raw; raw.m_version=DRW::AC1018; raw.m_isCustomClass=true;
    raw.m_hasClassDefinition=true;
    raw.m_recordName="DATATABLE"; raw.m_className="AcDbDataTable";
    raw.m_objectType=517; raw.m_handle=0xbdu; raw.m_rawBytes=b.data();
    raw.m_objectSize=static_cast<std::uint32_t>(raw.m_rawBytes.size());
    return raw;
}
bool cleared(const Result& r) {
    return r.classNumber==0u && r.handle==0u && r.parentHandle==0u
        && r.firstColumn.empty() && r.secondColumn.empty()
        && r.thirdColumn.empty() && r.ownedChildren.empty();
}
bool refused(const DRW_UnsupportedObject& raw) {
    Result r; r.handle=5u; r.ownedChildren.push_back(8u);
    return !DwgDataTableAc1018Candidate::inspect(raw,r) && cleared(r);
}
bool generated() {
    auto raw=build(); Result result;
    if (!DwgDataTableAc1018Candidate::inspect(raw,result)
        || result.classNumber!=517u || result.rawHeaderValue!=2u || result.handle!=0xbdu
        || result.parentHandle!=0xbcu || result.firstColumn!=std::vector<double>({2.,3.,4.})
        || result.secondColumn!=std::vector<std::int32_t>({100,101,102})
        || result.thirdColumn!=std::vector<std::int32_t>({200,201,202})
        || result.ownedChildren!=std::vector<std::uint32_t>({0xbeu,0xbfu,0xc0u})) return false;
    const auto original=raw.m_rawBytes;
    for (std::size_t n=0;n<original.size();++n) {
        raw.m_rawBytes.assign(original.begin(),original.begin()+n);
        raw.m_objectSize=static_cast<std::uint32_t>(n);
        if (!refused(raw)) return false;
    }
    raw=build(); raw.m_className="Other"; if (!refused(raw)) return false;
    raw=build(); raw.m_isCustomClass=false; if (!refused(raw)) return false;
    raw=build(); raw.m_hasClassDefinition=false; if (!refused(raw)) return false;
    raw=build(); raw.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::Validated);
    raw.m_parentHandle=0xbcu; raw.m_xDictFlag=1u;
    if (!DwgDataTableAc1018Candidate::inspect(raw,result)) return false;
    raw.m_parentHandle=0xbbu; if (!refused(raw)) return false;
    raw.m_parentHandle=0xbcu; raw.m_xDictFlag=0u; if (!refused(raw)) return false;
    raw.m_xDictFlag=1u; raw.m_reactorHandles.push_back(0xbau); if (!refused(raw)) return false;
    raw=build(); raw.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::ValidatedAbsent);
    if (!refused(raw)) return false;
    raw=build(); raw.m_objectType=518; if (!refused(raw)) return false;
    raw=build(); raw.m_handle=0xbeu; if (!refused(raw)) return false;
    raw=build(); raw.m_version=DRW::AC1021; if (!refused(raw)) return false;
    raw=build(); raw.m_bodyBitSize=1u; if (!refused(raw)) return false;
    raw=build(); raw.m_hasDataStorage=true; if (!refused(raw)) return false;
    raw=build(); ++raw.m_objectSize; if (!refused(raw)) return false;
    raw=build(); raw.m_rawBytes.resize(1024u*1024u+1u); raw.m_objectSize=static_cast<std::uint32_t>(raw.m_rawBytes.size());
    if (!refused(raw)) return false;
    Options o; o.badKind=2; if (!refused(build(o))) return false;
    o={}; o.selfRole=5; if (!refused(build(o))) return false;
    o={}; o.parentRole=5; if (!refused(build(o))) return false;
    o={}; o.childRole=4; if (!refused(build(o))) return false;
    o={}; o.eed=true; if (!refused(build(o))) return false;
    o={}; o.reactor=true; if (!refused(build(o))) return false;
    o={}; o.extension=true; if (!refused(build(o))) return false;
    o={}; o.nonemptyName=true; if (!refused(build(o))) return false;
    o={}; o.nonfinite=true; if (!refused(build(o))) return false;
    o={}; o.duplicate=true; if (!refused(build(o))) return false;
    o={}; o.reservedLong=true; if (!refused(build(o))) return false;
    o={}; o.reservedDouble=true; if (!refused(build(o))) return false;
    o={}; o.headerValue=3; if (!DwgDataTableAc1018Candidate::inspect(build(o),result) || result.rawHeaderValue!=3u) return false;
    o={}; o.rows=-1; if (!refused(build(o))) return false;
    o={}; o.rows=0; if (!refused(build(o))) return false;
    o={}; o.rows=4097; if (!refused(build(o))) return false;
    raw=build(); raw.m_rawBytes.push_back(0u); ++raw.m_objectSize;
    if (!refused(raw)) return false;
    raw=build(); raw.m_rawBytes.back()|=1u;
    if (!refused(raw)) return false;
    raw=build(); fault::active=true; fault::count=0;
    if (!DwgDataTableAc1018Candidate::inspect(raw,result)) return false;
    fault::active=false; const auto count=fault::count;
    if (count==0u || count>64u) return false;
    for (std::size_t i=0;i<count;++i) {
        result.handle=1u; fault::remaining=static_cast<long>(i);
        const bool ok=DwgDataTableAc1018Candidate::inspect(raw,result);
        fault::remaining=-1;
        if (ok || !cleared(result) || raw.m_rawBytes!=original) return false;
        if (!DwgDataTableAc1018Candidate::inspect(raw,result)) return false;
    }
    std::cout<<"generated bounded controls passed; allocation faults="<<count<<'\n';
    return true;
}
bool physical(const char* path) {
    std::ifstream input(path,std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});
    if (!input || bytes.empty()) return false;
    auto raw=build(); raw.m_rawBytes=std::move(bytes);
    raw.m_objectSize=static_cast<std::uint32_t>(raw.m_rawBytes.size());
    Result result;
    if (!DwgDataTableAc1018Candidate::inspect(raw,result) || result.dataEndBit!=877u
        || result.handleEndBit!=1037u || result.rawHeaderValue!=2u
        || result.parentHandle!=0xbcu || result.ownedChildren.size()!=9u
        || result.firstColumn!=std::vector<double>({1579.,0.,0.,49.,50.,51.,52.,53.,54.})
        || result.secondColumn!=std::vector<std::int32_t>({262144,0,0,262144,262144,262144,262144,262144,262144})
        || result.thirdColumn!=std::vector<std::int32_t>(9,0)) return false;
    for (std::size_t i=0;i<9u;++i) if (result.ownedChildren[i]!=0xbeu+i) return false;
    std::cout<<"physical candidate fields and source spans matched\n";
    return true;
}
}
int main(int argc,char**argv) {
    if (argc==3 && (std::string(argv[1])=="--emit-body" || std::string(argv[1])=="--emit-bad-role" || std::string(argv[1])=="--emit-nine")) {
        Options o;if(std::string(argv[1])=="--emit-bad-role")o.childRole=4u;
        if(std::string(argv[1])=="--emit-nine")o.rows=9;
        const auto raw=build(o);std::ofstream f(argv[2],std::ios::binary|std::ios::trunc);
        f.write(reinterpret_cast<const char*>(raw.m_rawBytes.data()),raw.m_rawBytes.size());
        return f?0:1;
    }
    if (argc>2 || !generated() || (argc==2 && !physical(argv[1]))) return 1;
    return 0;
}
