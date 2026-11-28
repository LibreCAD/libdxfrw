#include "intern/dwg_tablegeometry_ac1018_candidate.h"
#include "intern/dwgbufferw.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <new>

namespace fault { long remaining = -1; std::size_t count = 0; bool active = false; }
void* operator new(std::size_t n) {
    if (fault::active) ++fault::count;
    if (fault::remaining == 0) { fault::remaining = -1; throw std::bad_alloc(); }
    if (fault::remaining > 0) --fault::remaining;
    if (void* p = std::malloc(n ? n : 1u)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

namespace {
using DwgTableGeometryAc1018Candidate::Result;
struct Options {
    std::int32_t rows{3}, columns{3}, cells{9};
    std::uint16_t classNumber{515u};
    std::uint32_t owner{0xbcu};
    std::uint8_t selfRole{0u}, ownerRole{4u}, cellRole{4u};
    int badCellIndex{1}, dataEndAdjustment{0};
    bool eed{false}, reactor{false}, extension{false};
    bool badFlag{false}, badWidth{false}, badHeight{false}, badContents{false};
    bool nonNullCell{false}, reservedLong{false}, reservedCellLong{false};
    bool reservedDouble{false}, nonfinite{false}, lateRoleFailure{false};
};
void handle(dwgBufferW& b, std::uint8_t role, std::uint32_t value) {
    dwgHandle h; h.code=role; h.ref=value; b.putHandle(h);
}
DRW_UnsupportedObject build(const Options& o={}) {
    dwgBufferW b;
    b.putObjType(DRW::AC1018,o.classNumber);
    const auto endOffset=b.bitCount(); b.putRawLong32(0u);
    handle(b,o.selfRole,0xbau);
    b.putBitShort(o.eed ? 1u : 0u);
    b.putBitLong(o.reactor ? 1 : 0); b.putBit(o.extension ? 0u : 1u);
    if (o.reservedLong) b.put2Bits(3u); else b.putBitLong(o.rows);
    b.putBitLong(o.columns); b.putBitLong(o.cells);
    for (int i=0;i<9;++i) {
        if (o.reservedCellLong && i==o.badCellIndex) b.put2Bits(3u);
        else b.putBitLong(o.badFlag && i==o.badCellIndex ? 1 : 0);
        if (o.reservedDouble && i==o.badCellIndex) b.put2Bits(3u);
        else b.putBitDouble(o.nonfinite && i==o.badCellIndex ? std::numeric_limits<double>::quiet_NaN()
                              : (o.badWidth && i==o.badCellIndex ? 2.0 : 0.0));
        b.putBitDouble(o.badHeight && i==o.badCellIndex ? 3.0 : 0.0);
        b.putBitLong(o.badContents && i==o.badCellIndex ? 1 : 0);
    }
    b.patchRawLong32AtBit(endOffset,static_cast<std::uint32_t>(
        static_cast<std::int64_t>(b.bitCount()) + o.dataEndAdjustment));
    handle(b,o.ownerRole,o.owner);
    for (int i=0;i<9;++i) handle(b,o.lateRoleFailure && i==8 ? 5u : o.cellRole,
                                 o.nonNullCell && i==o.badCellIndex ? 0xcau : 0u);
    b.alignToByte();
    DRW_UnsupportedObject raw; raw.m_version=DRW::AC1018; raw.m_isCustomClass=true;
    raw.m_hasClassDefinition=true; raw.m_recordName="TABLEGEOMETRY";
    raw.m_className="AcDbTableGeometry"; raw.m_objectType=o.classNumber; raw.m_handle=0xbau;
    raw.m_rawBytes=b.data(); raw.m_objectSize=static_cast<std::uint32_t>(raw.m_rawBytes.size());
    return raw;
}
bool clear(const Result& r) {
    if (r.classNumber || r.handle || r.parentHandle || r.dataEndBit || r.handleEndBit
        || r.parentHandleStartBit || r.parentHandleEndBit) return false;
    for (const auto& cell:r.cells)
        if (cell.startBit || cell.endBit || cell.handleStartBit || cell.handleEndBit
            || cell.flags || cell.widthWithGap!=0.0
            || cell.heightWithGap!=0.0 || cell.unknownHandle) return false;
    return true;
}
bool refused(const DRW_UnsupportedObject& raw) {
    Result result; result.handle=7u; result.cells[4].startBit=3u;
    return !DwgTableGeometryAc1018Candidate::inspect(raw,result) && clear(result);
}
bool matches(const Result& result) {
    if (result.classNumber!=515u || result.handle!=0xbau || result.parentHandle!=0xbcu
        || result.dataEndBit!=173u || result.parentHandleStartBit!=173u
        || result.parentHandleEndBit!=189u || result.handleEndBit!=261u) return false;
    for (std::size_t i=0;i<result.cells.size();++i) {
        const auto& cell=result.cells[i];
        if (cell.startBit!=101u+8u*i || cell.endBit!=109u+8u*i
            || cell.handleStartBit!=189u+8u*i || cell.handleEndBit!=197u+8u*i || cell.flags!=0
            || cell.widthWithGap!=0.0 || cell.heightWithGap!=0.0 || cell.unknownHandle) return false;
    }
    return true;
}
bool generated() {
    auto raw=build(); const auto original=raw.m_rawBytes; Result result;
    if (!DwgTableGeometryAc1018Candidate::inspect(raw,result) || !matches(result)) return false;
    for (std::size_t n=0;n<original.size();++n) {
        raw.m_rawBytes.assign(original.begin(),original.begin()+n);
        raw.m_objectSize=static_cast<std::uint32_t>(n);
        if (!refused(raw)) return false;
    }
    raw=build(); raw.m_recordName="OTHER"; if (!refused(raw)) return false;
    raw=build(); raw.m_className="Other"; if (!refused(raw)) return false;
    raw=build(); raw.m_isEntity=true; if (!refused(raw)) return false;
    raw=build(); raw.m_isCustomClass=false; if (!refused(raw)) return false;
    raw=build(); raw.m_hasClassDefinition=false; if (!refused(raw)) return false;
    raw=build(); raw.m_hasDataStorage=true; if (!refused(raw)) return false;
    raw=build(); raw.m_objectType=516; if (!refused(raw)) return false;
    raw=build(); raw.m_handle=0u; if (!refused(raw)) return false;
    raw=build(); raw.m_handle=DRW::NoHandle; if (!refused(raw)) return false;
    raw=build(); raw.m_handle=0xbbu; if (!refused(raw)) return false;
    raw=build(); raw.m_version=DRW::AC1021; if (!refused(raw)) return false;
    raw=build(); raw.m_bodyBitSize=1u; if (!refused(raw)) return false;
    raw=build(); ++raw.m_objectSize; if (!refused(raw)) return false;
    raw=build(); raw.m_rawBytes.resize(4097u); raw.m_objectSize=4097u; if (!refused(raw)) return false;
    raw=build(); raw.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::Validated);
    raw.m_parentHandle=0xbcu; raw.m_xDictFlag=1u;
    if (!DwgTableGeometryAc1018Candidate::inspect(raw,result) || !matches(result)) return false;
    raw.m_parentHandle=0xbbu; if (!refused(raw)) return false;
    raw.m_parentHandle=0xbcu; raw.m_xDictFlag=0u; if (!refused(raw)) return false;
    raw.m_xDictFlag=1u; raw.m_reactorHandles.push_back(0xbbu); if (!refused(raw)) return false;
    raw=build(); raw.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::ValidatedAbsent);
    if (!refused(raw)) return false;
    raw=build(); raw.m_commonHandleDataValidated=true; if (!refused(raw)) return false;
    Options o; o.rows=2; if (!refused(build(o))) return false;
    o={}; o.classNumber=612u; o.owner=0xddu;
    if (!DwgTableGeometryAc1018Candidate::inspect(build(o),result)
        || result.classNumber!=612u || result.parentHandle!=0xddu) return false;
    o={}; o.classNumber=499u; if (!refused(build(o))) return false;
    o={}; o.owner=0u; if (!refused(build(o))) return false;
    o={}; o.owner=0xbau; if (!refused(build(o))) return false;
    o={}; o.columns=2; if (!refused(build(o))) return false;
    o={}; o.cells=8; if (!refused(build(o))) return false;
    o={}; o.selfRole=5; if (!refused(build(o))) return false;
    o={}; o.ownerRole=5; if (!refused(build(o))) return false;
    o={}; o.cellRole=5; if (!refused(build(o))) return false;
    o={}; o.lateRoleFailure=true; if (!refused(build(o))) return false;
    o={}; o.nonNullCell=true; if (!refused(build(o))) return false;
    o={}; o.eed=true; if (!refused(build(o))) return false;
    o={}; o.reactor=true; if (!refused(build(o))) return false;
    o={}; o.extension=true; if (!refused(build(o))) return false;
    o={}; o.badFlag=true; if (!refused(build(o))) return false;
    o={}; o.badWidth=true; if (!refused(build(o))) return false;
    o={}; o.badHeight=true; if (!refused(build(o))) return false;
    o={}; o.badContents=true; if (!refused(build(o))) return false;
    o={}; o.reservedLong=true; if (!refused(build(o))) return false;
    o={}; o.reservedCellLong=true; o.badCellIndex=8; if (!refused(build(o))) return false;
    o={}; o.reservedDouble=true; if (!refused(build(o))) return false;
    o={}; o.nonfinite=true; if (!refused(build(o))) return false;
    o={}; o.dataEndAdjustment=-1; if (!refused(build(o))) return false;
    o={}; o.dataEndAdjustment=1; if (!refused(build(o))) return false;
    raw=build(); raw.m_rawBytes.push_back(0u); ++raw.m_objectSize; if (!refused(raw)) return false;
    raw=build(); raw.m_rawBytes.back()|=1u; if (!refused(raw)) return false;
    raw=build(); raw.m_className="Other"; result.handle=8u;
    if (DwgTableGeometryAc1018Candidate::inspect(raw,result) || !clear(result)) return false;
    raw.m_className="AcDbTableGeometry";
    if (!DwgTableGeometryAc1018Candidate::inspect(raw,result) || !matches(result)) return false;
    raw=build(); fault::active=true; fault::count=0u;
    const bool ok=DwgTableGeometryAc1018Candidate::inspect(raw,result);
    fault::active=false; const auto count=fault::count;
    if (!ok || !matches(result) || count>32u) return false;
    for (std::size_t i=0;i<count;++i) {
        result.handle=1u; fault::remaining=static_cast<long>(i);
        const bool accepted=DwgTableGeometryAc1018Candidate::inspect(raw,result);
        fault::remaining=-1;
        if (accepted || !clear(result) || raw.m_rawBytes!=original) return false;
        if (!DwgTableGeometryAc1018Candidate::inspect(raw,result) || !matches(result)) return false;
    }
    std::cout<<"generated geometry controls passed; allocation faults="<<count<<'\n';
    return true;
}
bool physical(const char* path) {
    std::ifstream input(path,std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});
    if (!input || bytes.empty()) return false;
    auto raw=build(); raw.m_rawBytes=std::move(bytes);
    raw.m_objectSize=static_cast<std::uint32_t>(raw.m_rawBytes.size());
    Result result;
    if (!DwgTableGeometryAc1018Candidate::inspect(raw,result) || !matches(result)) return false;
    std::cout<<"caller-supplied body fields and expected spans matched\n";
    return true;
}
}
int main(int argc,char** argv) {
    if (argc==3 && (std::string(argv[1])=="--emit-body"
                    || std::string(argv[1])=="--emit-bad-role")) {
        Options options;
        if (std::string(argv[1])=="--emit-bad-role") options.cellRole=5u;
        const auto raw=build(options);
        std::ofstream output(argv[2],std::ios::binary|std::ios::trunc);
        output.write(reinterpret_cast<const char*>(raw.m_rawBytes.data()),
                     static_cast<std::streamsize>(raw.m_rawBytes.size()));
        return output ? 0 : 1;
    }
    if (argc>2 || !generated() || (argc==2 && !physical(argv[1]))) return 1;
    return 0;
}
