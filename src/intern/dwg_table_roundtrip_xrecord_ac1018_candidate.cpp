/* See header for the narrow source-observation scope. */
#include "dwg_table_roundtrip_xrecord_ac1018_candidate.h"
#include "dwgbuffer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace DwgTableRoundtripXRecordAc1018Candidate {
namespace {
constexpr std::size_t maxBodyBytes = 4096u;
constexpr std::uint32_t payloadBytes = 197u;
std::uint64_t bit(const dwgBuffer& b) noexcept {
    return b.getPosition()*8u+b.getBitPos();
}
bool bitLong(dwgBuffer& b, std::uint64_t end, std::int32_t& value) {
    const auto tag=b.get2Bits();
    if (tag==0u) value=static_cast<std::int32_t>(b.getRawLong32());
    else if (tag==1u) value=b.getRawChar8();
    else if (tag==2u) value=0;
    else return false;
    return b.isGood() && bit(b)<=end;
}
bool nonnullHandle(dwgBuffer& b, std::uint8_t role, std::uint32_t& value) {
    const dwgHandle h=b.getHandle();
    if (!b.isGood() || h.rawCode!=role || h.size==0u || h.ref64==0u
        || h.ref64>std::numeric_limits<std::uint32_t>::max()) return false;
    value=static_cast<std::uint32_t>(h.ref64);
    return true;
}
bool marker(dwgBuffer& b, std::uint64_t end, const DRW_Variant& value, const char* text) {
    const std::string expected(text);
    // This pinned producer uses an RS length. ODA v5.4.1 §20.4.104 instead
    // describes RC here, so this observation must not generalize that layout.
    if (value.code()!=102 || value.type()!=DRW_Variant::STRING
        || !value.stringEquals(expected) || b.getRawShort16()!=102u
        || b.getRawShort16()!=expected.size() || b.getRawChar8()!=45u) return false;
    for (const unsigned char byte:expected) if (b.getRawChar8()!=byte) return false;
    return b.isGood() && bit(b)<=end;
}
bool intPair(dwgBuffer& b, std::uint64_t end, const DRW_Variant& value,
             std::uint16_t group, std::int32_t expected) {
    if (value.code()!=group || value.type()!=DRW_Variant::INTEGER
        || value.i_val()!=expected || b.getRawShort16()!=group) return false;
    const auto wire=group==70u ? static_cast<std::int32_t>(b.getRawShort16())
                              : static_cast<std::int32_t>(b.getRawLong32());
    return b.isGood() && bit(b)<=end && wire==expected;
}
bool reference(dwgBuffer& b, std::uint64_t end, const DRW_Variant& value,
               std::uint16_t group, Reference& result) {
    result.startBit=bit(b);
    if (value.code()!=group || value.type()!=DRW_Variant::INTEGER64
        || b.getRawShort16()!=group) return false;
    const std::uint64_t wire=b.getRawLong64();
    if (!b.isGood() || bit(b)>end || wire==DRW::NoHandle
        || wire>std::numeric_limits<std::uint32_t>::max()
        || static_cast<std::uint64_t>(value.i64_val())!=wire) return false;
    result.groupCode=group;
    result.target=static_cast<std::uint32_t>(wire);
    result.endBit=bit(b);
    return true;
}
bool zeroPoint(dwgBuffer& b, std::uint64_t end, const DRW_Variant& value) {
    if (value.code()!=10 || value.type()!=DRW_Variant::COORD || value.coord()==nullptr
        || value.coord()->x!=0.0 || value.coord()->y!=0.0 || value.coord()->z!=0.0
        || std::signbit(value.coord()->x) || std::signbit(value.coord()->y)
        || std::signbit(value.coord()->z) || b.getRawShort16()!=10u) return false;
    for (int i=0;i<3;++i) if (b.getRawLong64()!=0u) return false;
    return b.isGood() && bit(b)<=end;
}
}

bool inspect(const DRW_UnsupportedObject& raw, const DRW_XRecord& typed,
             Result& result) noexcept {
    result=Result{};
    if (raw.m_version!=DRW::AC1018 || raw.m_objectType!=79 || raw.m_isEntity
        || raw.m_isCustomClass || raw.m_hasClassDefinition || raw.m_hasDataStorage
        || raw.m_handle==0u || raw.m_handle==DRW::NoHandle || raw.m_rawBytes.empty()
        || raw.m_rawBytes.size()>maxBodyBytes || raw.m_objectSize!=raw.m_rawBytes.size()
        || raw.m_bodyBitSize!=0u || typed.tType!=DRW::XRECORD
        || typed.handle!=raw.m_handle || typed.m_dataEntries.size()!=13u
        || typed.hasDataStorageBinaryData() || !typed.extData.empty() || !typed.appData.empty()
        || typed.m_rawDataValid || typed.hasDwgRawDataSourceRecord() || !typed.m_rawData.empty())
        return false;
    try {
        Result candidate;
        dwgBuffer b(raw.m_rawBytes.data(),raw.m_rawBytes.size(),nullptr);
        const auto total=static_cast<std::uint64_t>(raw.m_rawBytes.size())*8u;
        if (b.getObjType(DRW::AC1018)!=79u) return false;
        candidate.dataEndBit=b.getRawLong32();
        if (!b.isGood() || candidate.dataEndBit>total || candidate.dataEndBit<bit(b)
            || !nonnullHandle(b,0u,candidate.handle) || candidate.handle!=raw.m_handle
            || bit(b)>candidate.dataEndBit || b.getBitShort()!=0u) return false;
        std::int32_t reactors=0, size=0;
        if (!bitLong(b,candidate.dataEndBit,reactors) || reactors!=1
            || b.getBit()!=1u || !b.isGood() || bit(b)>candidate.dataEndBit
            || !bitLong(b,candidate.dataEndBit,size) || size!=static_cast<std::int32_t>(payloadBytes)) return false;
        candidate.payloadStartBit=bit(b);
        const std::uint64_t payloadEnd=candidate.payloadStartBit+payloadBytes*8u;
        if (payloadEnd>candidate.dataEndBit) return false;
        const auto& e=typed.m_dataEntries;
        if (!marker(b,payloadEnd,e[0],"ACAD_ROUNDTRIP_2008_TABLE_ENTITY")
            || !reference(b,payloadEnd,e[1],360u,candidate.references[0])
            || !intPair(b,payloadEnd,e[2],70u,2)
            || !intPair(b,payloadEnd,e[3],90u,1)
            || !zeroPoint(b,payloadEnd,e[4])
            || !intPair(b,payloadEnd,e[5],90u,0)
            || !intPair(b,payloadEnd,e[6],90u,2)
            || !reference(b,payloadEnd,e[7],361u,candidate.references[1])
            || !marker(b,payloadEnd,e[8],"ACAD_ROUNDTRIP_PRE2007_TABLE")
            || !intPair(b,payloadEnd,e[9],90u,3)
            || !intPair(b,payloadEnd,e[10],91u,3)
            || !marker(b,payloadEnd,e[11],"ACAD_ROUNDTRIP_PRE2007_TABLECELL")
            || !reference(b,payloadEnd,e[12],360u,candidate.references[2])
            || bit(b)!=payloadEnd) return false;
        candidate.payloadEndBit=bit(b);
        if (b.getBitShort()!=1u || !b.isGood() || bit(b)!=candidate.dataEndBit
            || typed.m_cloning!=1 || typed.reactorCount()!=1 || typed.extensionDictionaryFlag()!=1u
            || typed.xDictHandle!=0u || typed.reactorHandles.size()!=1u) return false;
        std::uint32_t reactor=0u;
        if (!nonnullHandle(b,4u,candidate.owner) || !nonnullHandle(b,4u,reactor)
            || candidate.owner!=reactor || candidate.owner==candidate.handle
            || typed.parentHandle!=candidate.owner || typed.reactorHandles[0]!=reactor) return false;
        for (std::size_t i=0;i<candidate.references.size();++i) {
            const auto target=candidate.references[i].target;
            if (target==candidate.handle || target==candidate.owner) return false;
            for (std::size_t j=0;j<i;++j) if (target==candidate.references[j].target) return false;
        }
        const bool common=drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence);
        if (raw.m_commonHandleDataValidated!=common) return false;
        if (common && (raw.m_commonLinkEvidence!=DRW_DwgCommonLinkEvidence::Validated
            || raw.m_parentHandle!=candidate.owner || raw.m_numReactors!=1
            || raw.m_reactorHandles.size()!=1u || raw.m_reactorHandles[0]!=reactor
            || raw.m_xDictFlag!=1u || raw.m_xDictHandle!=DRW::NoHandle)) return false;
        candidate.handleEndBit=bit(b);
        if (candidate.handleEndBit>total || total-candidate.handleEndBit>=8u) return false;
        while (bit(b)<total) if (b.getBit()!=0u || !b.isGood()) return false;
        result=candidate;
        return true;
    } catch (...) {result=Result{};return false;}
}

bool projectReferences(const DRW_UnsupportedObject& raw,
                       const DRW_XRecord& typed,
                       const std::array<std::uint32_t, 3>& mappedTargets,
                       std::vector<DRW_Variant>& projectedEntries,
                       Result& source) noexcept {
    source=Result{};
    try {
        Result inspected;
        if (!inspect(raw,typed,inspected)) return false;
        for (std::size_t i=0;i<mappedTargets.size();++i) {
            const auto target=mappedTargets[i];
            if (target==0u || target==DRW::NoHandle || target==inspected.handle
                || target==inspected.owner) return false;
            for (std::size_t j=0;j<i;++j)
                if (target==mappedTargets[j]) return false;
        }

        std::vector<DRW_Variant> candidate=typed.m_dataEntries;
        for (std::size_t i=0;i<mappedTargets.size();++i) {
            const auto entryIndex=i==0u ? 1u : (i==1u ? 7u : 12u);
            candidate[entryIndex]=DRW_Variant(
                inspected.references[i].groupCode,
                static_cast<std::uint64_t>(mappedTargets[i]));
            if (candidate[entryIndex].code()!=inspected.references[i].groupCode
                || candidate[entryIndex].type()!=DRW_Variant::INTEGER64
                || candidate[entryIndex].i64_val()!=mappedTargets[i]) return false;
        }
        projectedEntries.swap(candidate);
        source=inspected;
        return true;
    } catch (...) {source=Result{};return false;}
}
}
