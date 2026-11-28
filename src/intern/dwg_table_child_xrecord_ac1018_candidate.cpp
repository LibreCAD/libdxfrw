#include "dwg_table_child_xrecord_ac1018_candidate.h"
#include "dwgbuffer.h"
#include <limits>

namespace DwgTableChildXRecordAc1018Candidate {
namespace {
std::uint64_t bit(const dwgBuffer& b) noexcept {return b.getPosition()*8u+b.getBitPos();}
bool bitLong(dwgBuffer& b,std::uint64_t end,std::int32_t& value) {
    const auto tag=b.get2Bits();
    if(tag==0u)value=static_cast<std::int32_t>(b.getRawLong32());
    else if(tag==1u)value=b.getRawChar8();
    else if(tag==2u)value=0;
    else return false;
    return b.isGood() && bit(b)<=end;
}
bool number(dwgBuffer& b,std::uint64_t end,const DRW_Variant& v,int code,std::int32_t expected) {
    return v.code()==code && v.type()==DRW_Variant::INTEGER && v.i_val()==expected
        && b.getRawShort16()==code && static_cast<std::int32_t>(b.getRawLong32())==expected
        && b.isGood() && bit(b)<=end;
}
bool text(dwgBuffer& b,std::uint64_t end,const DRW_Variant& v,int code,std::string& value) {
    if(v.code()!=code || v.type()!=DRW_Variant::STRING || b.getRawShort16()!=code)return false;
    const auto n=b.getRawShort16();const auto page=b.getRawChar8();
    if(!b.isGood() || n>64u || page!=45u || bit(b)>end || static_cast<std::uint64_t>(n)*8u>end-bit(b))return false;
    value.clear();value.reserve(n);
    for(unsigned i=0;i<n;++i) {
        const auto c=b.getRawChar8();if(c<0x20u || c>0x7eu)return false;
        value.push_back(static_cast<char>(c));
    }
    return b.isGood() && v.stringEquals(value);
}
bool handle(dwgBuffer& b,std::uint8_t role,std::uint32_t& value) {
    const auto h=b.getHandle();
    if(!b.isGood() || h.rawCode!=role || h.size==0u || h.ref64==0u
        || h.ref64>std::numeric_limits<std::uint32_t>::max() || h.ref64==DRW::NoHandle)return false;
    value=static_cast<std::uint32_t>(h.ref64);return true;
}
}
bool inspect(const DRW_UnsupportedObject& raw,const DRW_XRecord& typed,Result& result) noexcept {
    result={};
    if(raw.m_version!=DRW::AC1018 || raw.m_objectType!=79 || raw.m_isEntity || raw.m_isCustomClass
        || raw.m_hasClassDefinition || raw.m_hasDataStorage || !raw.m_handle || raw.m_handle==DRW::NoHandle
        || raw.m_rawBytes.empty() || raw.m_rawBytes.size()>4096u || raw.m_objectSize!=raw.m_rawBytes.size()
        || raw.m_bodyBitSize!=0u || typed.tType!=DRW::XRECORD || typed.handle!=raw.m_handle
        || (typed.m_dataEntries.size()!=5u && typed.m_dataEntries.size()!=6u)
        || typed.hasDataStorageBinaryData() || !typed.extData.empty() || !typed.appData.empty()
        || typed.m_rawDataValid || typed.hasDwgRawDataSourceRecord() || !typed.m_rawData.empty())return false;
    try {
        Result candidate;dwgBuffer b(raw.m_rawBytes.data(),raw.m_rawBytes.size(),nullptr);
        const auto total=static_cast<std::uint64_t>(raw.m_rawBytes.size())*8u;
        if(b.getObjType(DRW::AC1018)!=79u)return false;
        candidate.dataEndBit=b.getRawLong32();
        if(!b.isGood() || candidate.dataEndBit>total || candidate.dataEndBit<bit(b)
            || !handle(b,0u,candidate.handle) || candidate.handle!=raw.m_handle
            || bit(b)>candidate.dataEndBit || b.getBitShort()!=0u)return false;
        std::int32_t reactors=0,size=0;
        if(!bitLong(b,candidate.dataEndBit,reactors) || reactors!=0 || b.getBit()!=1u
            || !b.isGood() || bit(b)>candidate.dataEndBit || !bitLong(b,candidate.dataEndBit,size)
            || size<0 || size>512 || static_cast<std::uint64_t>(size)*8u>candidate.dataEndBit-bit(b))return false;
        const auto payloadEnd=bit(b)+static_cast<std::uint64_t>(size)*8u;
        const auto& e=typed.m_dataEntries;candidate.hasText=e.size()==6u;
        if(!number(b,payloadEnd,e[0],93,candidate.hasText?4:7)
            || !number(b,payloadEnd,e[1],90,candidate.hasText?4:0))return false;
        std::size_t i=2u;std::string group1,group300,group302;
        if(candidate.hasText && (!text(b,payloadEnd,e[i++],1,group1) || group1.empty()))return false;
        if(!number(b,payloadEnd,e[i++],94,0) || !text(b,payloadEnd,e[i++],300,group300)
            || !group300.empty() || !text(b,payloadEnd,e[i++],302,group302)
            || (candidate.hasText?group302!=group1:!group302.empty()) || bit(b)!=payloadEnd)return false;
        if(b.getBitShort()!=1u || !b.isGood() || bit(b)!=candidate.dataEndBit
            || typed.m_cloning!=1 || typed.reactorCount()!=0 || typed.extensionDictionaryFlag()!=1u
            || typed.xDictHandle!=0u || !typed.reactorHandles.empty())return false;
        candidate.ownerStartBit=bit(b);
        if(!handle(b,4u,candidate.owner) || candidate.owner==candidate.handle || typed.parentHandle!=candidate.owner)return false;
        candidate.ownerEndBit=bit(b);
        const bool common=drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence);
        if(raw.m_commonHandleDataValidated!=common)return false;
        if(common && (raw.m_commonLinkEvidence!=DRW_DwgCommonLinkEvidence::Validated
            || raw.m_parentHandle!=candidate.owner || raw.m_numReactors!=0 || !raw.m_reactorHandles.empty()
            || raw.m_xDictFlag!=1u || raw.m_xDictHandle!=DRW::NoHandle))return false;
        if(candidate.ownerEndBit>total || total-candidate.ownerEndBit>=8u)return false;
        while(bit(b)<total)if(b.getBit()!=0u || !b.isGood())return false;
        candidate.text=std::move(group1);result=std::move(candidate);return true;
    } catch(...) {result={};return false;}
}
}
