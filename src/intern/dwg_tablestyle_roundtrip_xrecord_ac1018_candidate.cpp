#include "dwg_tablestyle_roundtrip_xrecord_ac1018_candidate.h"
#include "dwgbuffer.h"
#include <limits>
#include <string>

namespace DwgTableStyleRoundtripXRecordAc1018Candidate {
namespace {
std::uint64_t bit(const dwgBuffer& b) noexcept { return b.getPosition()*8u+b.getBitPos(); }
bool bitLong(dwgBuffer& b,std::uint64_t end,std::int32_t& value) {
    const auto tag=b.get2Bits();
    if(tag==0u)value=static_cast<std::int32_t>(b.getRawLong32());
    else if(tag==1u)value=b.getRawChar8();
    else if(tag==2u)value=0;
    else return false;
    return b.isGood() && bit(b)<=end;
}
bool handle(dwgBuffer& b,std::uint8_t role,std::uint32_t& value) {
    const auto h=b.getHandle();
    if(!b.isGood() || h.rawCode!=role || h.size==0u || h.ref64==0u
        || h.ref64>std::numeric_limits<std::uint32_t>::max() || h.ref64==DRW::NoHandle)return false;
    value=static_cast<std::uint32_t>(h.ref64);return true;
}
bool number(dwgBuffer& b,std::uint64_t end,const DRW_Variant& v,int code,std::int32_t expected) {
    return end>=bit(b) && end-bit(b)>=48u && v.code()==code && v.type()==DRW_Variant::INTEGER
        && v.i_val()==expected && b.getRawShort16()==code
        && static_cast<std::int32_t>(b.getRawLong32())==expected && b.isGood();
}
bool text(dwgBuffer& b,std::uint64_t end,const DRW_Variant& v,int code,const char* expected) {
    const std::string value(expected);
    if(v.code()!=code || v.type()!=DRW_Variant::STRING || !v.stringEquals(value)
        || end<bit(b) || end-bit(b)<40u || b.getRawShort16()!=code)return false;
    const auto n=b.getRawShort16();const auto page=b.getRawChar8();
    if(!b.isGood() || n!=value.size() || page!=45u || bit(b)>end
        || static_cast<std::uint64_t>(n)*8u>end-bit(b))return false;
    for(unsigned i=0;i<n;++i)if(b.getRawChar8()!=static_cast<std::uint8_t>(value[i]))return false;
    return b.isGood();
}
}
bool inspect(const DRW_UnsupportedObject& raw,const DRW_XRecord& typed,Result& result) noexcept {
    result={};
    if(raw.m_version!=DRW::AC1018 || raw.m_objectType!=79 || raw.m_isEntity || raw.m_isCustomClass
        || raw.m_hasClassDefinition || raw.m_hasDataStorage || !raw.m_handle || raw.m_handle==DRW::NoHandle
        || raw.m_rawBytes.empty() || raw.m_rawBytes.size()>4096u || raw.m_objectSize!=raw.m_rawBytes.size()
        || raw.m_bodyBitSize!=0u || typed.tType!=DRW::XRECORD || typed.handle!=raw.m_handle
        || typed.m_dataEntries.size()!=10u || typed.hasDataStorageBinaryData() || !typed.extData.empty()
        || !typed.appData.empty() || typed.m_rawDataValid || typed.hasDwgRawDataSourceRecord() || !typed.m_rawData.empty())return false;
    try {
        Result c;dwgBuffer b(raw.m_rawBytes.data(),raw.m_rawBytes.size(),nullptr);
        const auto total=static_cast<std::uint64_t>(raw.m_rawBytes.size())*8u;
        if(b.getObjType(DRW::AC1018)!=79u)return false;
        c.dataEndBit=b.getRawLong32();
        if(!b.isGood() || c.dataEndBit>total || c.dataEndBit<bit(b)
            || !handle(b,0u,c.handle) || c.handle!=raw.m_handle
            || bit(b)>c.dataEndBit || b.getBitShort()!=0u)return false;
        std::int32_t reactors=0,size=0;
        if(!bitLong(b,c.dataEndBit,reactors) || reactors!=1 || b.getBit()!=1u
            || !b.isGood() || bit(b)>c.dataEndBit || !bitLong(b,c.dataEndBit,size)
            || size!=89 || static_cast<std::uint64_t>(size)*8u>c.dataEndBit-bit(b))return false;
        const auto payloadEnd=bit(b)+static_cast<std::uint64_t>(size)*8u;
        const auto& e=typed.m_dataEntries;
        for(std::size_t i=0;i<e.size();++i) {
            c.entries[i].begin=bit(b);bool ok=false;
            switch(i) {
            case 0:ok=text(b,payloadEnd,e[i],102,"ACAD_ROUNDTRIP_PRE2007_TABLESTYLE");break;
            case 1:ok=number(b,payloadEnd,e[i],90,4);break;
            case 2:ok=number(b,payloadEnd,e[i],91,0);break;
            case 3:ok=text(b,payloadEnd,e[i],1,"");break;
            case 4:ok=number(b,payloadEnd,e[i],92,4);break;
            case 5:ok=number(b,payloadEnd,e[i],93,0);break;
            case 6:ok=text(b,payloadEnd,e[i],2,"");break;
            case 7:ok=number(b,payloadEnd,e[i],94,4);break;
            case 8:ok=number(b,payloadEnd,e[i],95,0);break;
            case 9:ok=text(b,payloadEnd,e[i],3,"");break;
            }
            if(!ok)return false;
            c.entries[i].end=bit(b);
        }
        if(bit(b)!=payloadEnd || b.getBitShort()!=1u || !b.isGood() || bit(b)!=c.dataEndBit
            || typed.m_cloning!=1 || typed.reactorCount()!=1 || typed.extensionDictionaryFlag()!=1u
            || typed.xDictHandle!=0u || typed.reactorHandles.size()!=1u)return false;
        c.ownerSpan.begin=bit(b);
        if(!handle(b,4u,c.owner) || c.owner==c.handle || typed.parentHandle!=c.owner)return false;
        c.ownerSpan.end=bit(b);c.reactorSpan.begin=bit(b);
        if(!handle(b,4u,c.reactor) || c.reactor!=c.owner || typed.reactorHandles[0]!=c.reactor)return false;
        c.reactorSpan.end=bit(b);
        const bool common=drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence);
        if(raw.m_commonHandleDataValidated!=common)return false;
        if(common && (raw.m_commonLinkEvidence!=DRW_DwgCommonLinkEvidence::Validated
            || raw.m_parentHandle!=c.owner || raw.m_numReactors!=1 || raw.m_reactorHandles.size()!=1u
            || raw.m_reactorHandles[0]!=c.reactor || raw.m_xDictFlag!=1u || raw.m_xDictHandle!=DRW::NoHandle))return false;
        if(c.reactorSpan.end>total || total-c.reactorSpan.end>=8u)return false;
        while(bit(b)<total)if(b.getBit()!=0u || !b.isGood())return false;
        result=c;return true;
    } catch(...) {result={};return false;}
}
}
