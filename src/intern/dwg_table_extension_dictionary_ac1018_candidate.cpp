#include "dwg_table_extension_dictionary_ac1018_candidate.h"
#include "dwgbuffer.h"
#include <limits>
namespace DwgTableExtensionDictionaryAc1018Candidate {
namespace {
std::uint64_t bit(const dwgBuffer& b) noexcept{return b.getPosition()*8u+b.getBitPos();}
bool bitLong(dwgBuffer& b,std::uint64_t end,std::int32_t& value){
    const auto tag=b.get2Bits();if(tag==0u)value=static_cast<std::int32_t>(b.getRawLong32());
    else if(tag==1u)value=b.getRawChar8();else if(tag==2u)value=0;else return false;
    return b.isGood() && bit(b)<=end;
}
bool handle(dwgBuffer& b,std::uint8_t role,std::uint32_t& value){
    const auto h=b.getHandle();if(!b.isGood() || h.rawCode!=role || !h.size || !h.ref64
        || h.ref64>std::numeric_limits<std::uint32_t>::max() || h.ref64==DRW::NoHandle)return false;
    value=static_cast<std::uint32_t>(h.ref64);return true;
}
}
bool inspect(const DRW_UnsupportedObject& raw,const DRW_Dictionary& typed,Result& result) noexcept {
    result={};
    if(raw.m_version!=DRW::AC1018 || raw.m_objectType!=42 || raw.m_isEntity || raw.m_isCustomClass
        || raw.m_hasClassDefinition || raw.m_hasDataStorage || !raw.m_handle || raw.m_handle==DRW::NoHandle
        || raw.m_rawBytes.empty() || raw.m_rawBytes.size()>4096u || raw.m_objectSize!=raw.m_rawBytes.size()
        || raw.m_bodyBitSize!=0u || typed.tType!=DRW::DICTIONARY || typed.handle!=raw.m_handle
        || !typed.hasCompleteDwgEntries() || typed.m_entries.size()!=1u
        || typed.m_entries[0].m_name!="ACAD_XREC_ROUNDTRIP" || typed.cloning!=1 || typed.hardOwner!=1
        || typed.hasDataStorageBinaryData() || !typed.extData.empty() || !typed.appData.empty())return false;
    try {
        Result candidate;dwgBuffer b(raw.m_rawBytes.data(),raw.m_rawBytes.size(),nullptr);
        const auto total=static_cast<std::uint64_t>(raw.m_rawBytes.size())*8u;
        if(b.getObjType(DRW::AC1018)!=42u)return false;
        candidate.dataEndBit=b.getRawLong32();
        if(!b.isGood() || candidate.dataEndBit>total || candidate.dataEndBit<bit(b)
            || !handle(b,0u,candidate.handle) || candidate.handle!=raw.m_handle
            || bit(b)>candidate.dataEndBit || b.getBitShort()!=0u)return false;
        std::int32_t reactors=0,count=0;
        if(!bitLong(b,candidate.dataEndBit,reactors) || reactors!=0 || b.getBit()!=1u
            || !b.isGood() || bit(b)>candidate.dataEndBit || !bitLong(b,candidate.dataEndBit,count) || count!=1
            || b.getBitShort()!=1u || !b.isGood() || bit(b)>candidate.dataEndBit
            || b.getRawChar8()!=1u || !b.isGood() || bit(b)>candidate.dataEndBit
            || b.getBitShort()!=19u || !b.isGood() || bit(b)>candidate.dataEndBit
            || 19u*8u>candidate.dataEndBit-bit(b))return false;
        for(const char c:std::string("ACAD_XREC_ROUNDTRIP"))if(b.getRawChar8()!=static_cast<std::uint8_t>(c))return false;
        if(!b.isGood() || bit(b)!=candidate.dataEndBit || typed.reactorCount()!=0 || typed.extensionDictionaryFlag()!=1u
            || typed.xDictHandle!=0u || !typed.reactorHandles.empty())return false;
        candidate.ownerStartBit=bit(b);if(!handle(b,4u,candidate.owner))return false;candidate.ownerEndBit=bit(b);
        candidate.rootStartBit=bit(b);if(!handle(b,2u,candidate.root))return false;candidate.rootEndBit=bit(b);
        if(candidate.owner==candidate.handle || candidate.root==candidate.handle || candidate.owner==candidate.root
            || typed.parentHandle!=candidate.owner || typed.m_entries[0].m_handle!=candidate.root)return false;
        const bool common=drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence);
        if(raw.m_commonHandleDataValidated!=common)return false;
        if(common && (raw.m_commonLinkEvidence!=DRW_DwgCommonLinkEvidence::Validated
            || raw.m_parentHandle!=candidate.owner || raw.m_numReactors!=0 || !raw.m_reactorHandles.empty()
            || raw.m_xDictFlag!=1u || raw.m_xDictHandle!=DRW::NoHandle))return false;
        if(candidate.rootEndBit>total || total-candidate.rootEndBit>=8u)return false;
        while(bit(b)<total)if(b.getBit()!=0u || !b.isGood())return false;
        result=candidate;return true;
    } catch(...) {result={};return false;}
}
}
