/* Opt-in candidate observer; see header for scope and provenance limits. */
#include "dwg_datatable_ac1018_candidate.h"
#include "dwgbuffer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace DwgDataTableAc1018Candidate {
namespace {
constexpr std::size_t maxBodyBytes=1024u*1024u;
constexpr std::int32_t maxRows=4096;

std::uint64_t bit(const dwgBuffer& b) noexcept {
    return b.getPosition()*8u+b.getBitPos();
}
bool longValue(dwgBuffer& b,std::uint64_t end,std::int32_t& value) {
    const auto tag=b.get2Bits();
    if (tag==0u) value=static_cast<std::int32_t>(b.getRawLong32());
    else if (tag==1u) value=b.getRawChar8();
    else if (tag==2u) value=0;
    else return false; // ODA BL code 11 is reserved.
    return b.isGood() && bit(b)<=end;
}
bool shortValue(dwgBuffer& b,std::uint64_t end,std::uint16_t& value) {
    value=b.getBitShort();
    return b.isGood() && bit(b)<=end;
}
bool emptyText(dwgBuffer& b,std::uint64_t end) {
    std::uint16_t length=0;
    return shortValue(b,end,length) && length==0u;
}
bool handle(dwgBuffer& b,std::uint8_t role,std::uint32_t& value) {
    const dwgHandle h=b.getHandle();
    if (!b.isGood() || h.rawCode!=role || h.size==0u
        || h.ref64==0u || h.ref64>std::numeric_limits<std::uint32_t>::max()) return false;
    value=static_cast<std::uint32_t>(h.ref64);
    return true;
}
}

bool inspect(const DRW_UnsupportedObject& raw,Result& result) noexcept {
    result=Result{};
    if (raw.m_version!=DRW::AC1018 || raw.m_isEntity || !raw.m_isCustomClass
        || !raw.m_hasClassDefinition || raw.m_hasDataStorage
        || raw.m_recordName!="DATATABLE" || raw.m_className!="AcDbDataTable"
        || raw.m_objectType<500 || raw.m_objectType>std::numeric_limits<std::uint16_t>::max()
        || raw.m_handle==0u || raw.m_handle==DRW::NoHandle || raw.m_rawBytes.empty()
        || raw.m_rawBytes.size()>maxBodyBytes || raw.m_objectSize!=raw.m_rawBytes.size()
        // This compatibility field carries the R2010+ handle-stream size;
        // AC1018 has no such prefix and the reader reports zero.
        || raw.m_bodyBitSize!=0u) return false;
    try {
        Result candidate;
        dwgBuffer b(raw.m_rawBytes.data(),raw.m_rawBytes.size(),nullptr);
        const auto total=static_cast<std::uint64_t>(raw.m_rawBytes.size())*8u;
        candidate.classNumber=b.getObjType(DRW::AC1018);
        candidate.dataEndBit=b.getRawLong32();
        if (!b.isGood() || candidate.classNumber!=raw.m_objectType
            || candidate.dataEndBit>total || candidate.dataEndBit<bit(b)) return false;
        if (!handle(b,0u,candidate.handle) || candidate.handle!=raw.m_handle
            || bit(b)>candidate.dataEndBit) return false;
        std::uint16_t eed=0;
        std::int32_t reactors=0;
        if (!shortValue(b,candidate.dataEndBit,eed) || eed!=0u
            || !longValue(b,candidate.dataEndBit,reactors) || reactors!=0
            || b.getBit()!=1u || !b.isGood() || bit(b)>candidate.dataEndBit
            || !shortValue(b,candidate.dataEndBit,candidate.rawHeaderValue)) return false;
        std::int32_t columns=0,rows=0;
        if (!longValue(b,candidate.dataEndBit,columns) || columns!=4
            || !longValue(b,candidate.dataEndBit,rows) || rows<=0 || rows>maxRows
            || !emptyText(b,candidate.dataEndBit)) return false;
        const auto count=static_cast<std::size_t>(rows);
        candidate.firstColumn.reserve(count); candidate.secondColumn.reserve(count);
        candidate.thirdColumn.reserve(count); candidate.ownedChildren.reserve(count);
        for (int column=0;column<4;++column) {
            std::int32_t kind=0;
            if (!longValue(b,candidate.dataEndBit,kind) || !emptyText(b,candidate.dataEndBit)
                || kind!=(column==0 ? 2 : column==3 ? 6 : 1)) return false;
            if (column==3) break;
            for (std::int32_t row=0;row<rows;++row) {
                if (column==0) {
                    const double value=b.getBitDouble();
                    if (!b.isGood() || bit(b)>candidate.dataEndBit || !std::isfinite(value)) return false;
                    candidate.firstColumn.push_back(value);
                } else {
                    std::int32_t value=0;
                    if (!longValue(b,candidate.dataEndBit,value)) return false;
                    (column==1 ? candidate.secondColumn : candidate.thirdColumn).push_back(value);
                }
            }
        }
        if (bit(b)!=candidate.dataEndBit || !handle(b,4u,candidate.parentHandle)
            || candidate.parentHandle==candidate.handle) return false;
        const bool commonValidated=drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence);
        if (raw.m_commonHandleDataValidated!=commonValidated) return false;
        // Unknown evidence is not a claim and cannot supply provenance. When
        // a caller marks common links validated, it must agree with the bytes.
        if (commonValidated && (raw.m_commonLinkEvidence!=DRW_DwgCommonLinkEvidence::Validated
            || raw.m_parentHandle!=candidate.parentHandle || raw.m_numReactors!=0
            || !raw.m_reactorHandles.empty() || raw.m_xDictFlag!=1u
            || raw.m_xDictHandle!=DRW::NoHandle)) return false;
        for (std::int32_t row=0;row<rows;++row) {
            std::uint32_t child=0;
            if (!handle(b,3u,child) || child==candidate.handle || child==candidate.parentHandle) return false;
            candidate.ownedChildren.push_back(child);
        }
        candidate.handleEndBit=bit(b);
        if (candidate.handleEndBit>total || total-candidate.handleEndBit>=8u) return false;
        while (bit(b)<total) if (b.getBit()!=0u || !b.isGood()) return false;
        auto unique=candidate.ownedChildren;
        std::sort(unique.begin(),unique.end());
        if (std::adjacent_find(unique.cbegin(),unique.cend())!=unique.cend()) return false;
        result=std::move(candidate);
        return true;
    } catch (...) { result=Result{}; return false; }
}
}
