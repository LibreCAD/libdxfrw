#include "dwg_table_entity_xdictionary_ac1018_candidate.h"
#include "dwgbuffer.h"
#include <limits>
namespace DwgTableEntityXDictionaryAc1018Candidate {
namespace {
std::uint64_t bit(const dwgBuffer& b) noexcept{return b.getPosition()*8u+b.getBitPos();}
bool handle(dwgBuffer& b,std::uint8_t role,std::uint32_t& value){
    const auto h=b.getHandle();if(!b.isGood() || h.rawCode!=role || !h.size || !h.ref64
        || h.ref64>std::numeric_limits<std::uint32_t>::max() || h.ref64==DRW::NoHandle)return false;
    value=static_cast<std::uint32_t>(h.ref64);return true;
}
}
bool inspect(const DRW_UnsupportedObject& raw,Result& result) noexcept {
    result={};
    if(raw.m_version!=DRW::AC1018 || raw.m_objectType<500 || raw.m_objectType>std::numeric_limits<std::int16_t>::max()
        || !raw.m_isEntity || !raw.m_isCustomClass || !raw.m_hasClassDefinition || raw.m_hasDataStorage
        || raw.m_recordName!="ACAD_TABLE" || raw.m_className!="AcDbTable"
        || !raw.m_handle || raw.m_handle==DRW::NoHandle || raw.m_bodyBitSize!=0u
        || raw.m_rawBytes.empty() || raw.m_rawBytes.size()>65536u || raw.m_objectSize!=raw.m_rawBytes.size())return false;
    try {
        Result candidate;dwgBuffer b(raw.m_rawBytes.data(),raw.m_rawBytes.size(),nullptr);
        const auto total=static_cast<std::uint64_t>(raw.m_rawBytes.size())*8u;
        if(b.getObjType(DRW::AC1018)!=raw.m_objectType)return false;
        candidate.dataEndBit=b.getRawLong32();
        if(!b.isGood() || candidate.dataEndBit>total || candidate.dataEndBit<bit(b)
            || !handle(b,0u,candidate.handle) || candidate.handle!=raw.m_handle
            || bit(b)>candidate.dataEndBit || b.getBitShort()!=0u || b.getBit()!=1u)return false;
        const auto proxyBytes=b.getRawLong32();
        if(!b.isGood() || bit(b)>candidate.dataEndBit || proxyBytes!=2104u
            || static_cast<std::uint64_t>(proxyBytes)*8u>candidate.dataEndBit-bit(b))return false;
        const auto after=bit(b)+static_cast<std::uint64_t>(proxyBytes)*8u;
        if(!b.setPosition(after/8u))return false;
        b.setBitPos(static_cast<std::uint8_t>(after%8u));
        // This source has no common owner, no reactors and a present xdict.
        if(b.get2Bits()!=2u || b.get2Bits()!=2u || b.getBit()!=0u || !b.isGood()
            || bit(b)>candidate.dataEndBit || b.getBitShort()!=256u || b.get2Bits()!=1u
            || b.get2Bits()!=0u || b.get2Bits()!=0u || b.getBitShort()!=0u
            || b.getRawChar8()!=29u || !b.isGood() || bit(b)>candidate.dataEndBit)return false;
        if(!b.setPosition(candidate.dataEndBit/8u))return false;
        b.setBitPos(static_cast<std::uint8_t>(candidate.dataEndBit%8u));
        candidate.dictionaryStartBit=bit(b);
        if(!handle(b,3u,candidate.dictionary))return false;
        candidate.dictionaryEndBit=bit(b);
        if(candidate.dictionary==candidate.handle || !handle(b,5u,candidate.layer)
            || !handle(b,5u,candidate.block) || !handle(b,5u,candidate.style))return false;
        result=candidate;return true;
    } catch(...) {result={};return false;}
}
}
