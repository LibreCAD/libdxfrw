#include "dwg_tablestyle_ac1018_candidate.h"
#include "dwgbuffer.h"
#include <cmath>
#include <limits>
namespace DwgTableStyleAc1018Candidate {
namespace {
std::uint64_t bit(const dwgBuffer& b) noexcept {return b.getPosition()*8u+b.getBitPos();}
bool bound(const dwgBuffer& b,std::uint64_t end) {return b.isGood() && bit(b)<=end;}
bool bl(dwgBuffer& b,std::uint64_t end,std::int32_t& value) {
    const auto t=b.get2Bits();
    if(t==0u)value=static_cast<std::int32_t>(b.getRawLong32());
    else if(t==1u)value=b.getRawChar8();else if(t==2u)value=0;else return false;
    return bound(b,end);
}
bool bd(dwgBuffer& b,std::uint64_t end,double value) {
    const auto t=b.get2Bits();double got=0.0;
    if(t==0u)got=b.getRawDouble();else if(t==1u)got=1.0;else if(t!=2u)return false;
    return bound(b,end) && std::isfinite(got) && got==value && (got!=0.0 || !std::signbit(got));
}
bool cmc(dwgBuffer& b,std::uint64_t end,std::uint32_t value,const DRW_TableStyleCmcColor& c) {
    std::int32_t raw=0;
    if(b.getBitShort()!=0u || !bl(b,end,raw) || static_cast<std::uint32_t>(raw)!=value
        || b.getRawChar8()!=0u || !bound(b,end) || c.m_method!=(value>>24u)
        || c.m_flags || c.m_reservedIndex || !c.m_name.empty() || !c.m_bookName.empty())return false;
    return value==0xc8000000u ? c.hasRawDwgPackedValue() && c.rawDwgPackedValue()==value : c.m_rgb==-1;
}
bool handle(dwgBuffer& b,std::uint8_t role,std::uint32_t& out) {
    const auto h=b.getHandle();
    if(!b.isGood() || h.rawCode!=role || h.size==0u || h.ref64==0u
        || h.ref64>std::numeric_limits<std::uint32_t>::max())return false;
    out=static_cast<std::uint32_t>(h.ref64);return true;
}
}
bool inspect(const DRW_UnsupportedObject& raw,const DRW_TableStyle& typed,Result& result) noexcept {
    result={};
    if(raw.m_version!=DRW::AC1018 || raw.m_isEntity || !raw.m_isCustomClass
        || !raw.m_hasClassDefinition || raw.m_hasDataStorage || raw.m_recordName!="TABLESTYLE"
        || raw.m_className!="AcDbTableStyle" || raw.m_objectType<500
        || raw.m_objectType>std::numeric_limits<std::int16_t>::max() || !raw.m_handle
        || raw.m_handle==DRW::NoHandle || raw.m_rawBytes.empty() || raw.m_rawBytes.size()>4096u
        || raw.m_objectSize!=raw.m_rawBytes.size() || raw.m_bodyBitSize!=0u
        || typed.tType!=DRW::TABLESTYLE || typed.handle!=raw.m_handle
        || !typed.extData.empty() || !typed.appData.empty() || typed.hasDataStorageBinaryData()
        || typed.m_name!="Standard" || typed.m_flowDirection!=0 || typed.m_flags!=0
        || typed.m_horizontalCellMargin!=1.5 || typed.m_verticalCellMargin!=1.5
        || typed.m_titleSuppressed || typed.m_headerSuppressed || typed.m_r24PrefixPresent
        || !typed.m_cellStyles.empty() || typed.m_tableCellStyle.m_hasData
        || typed.m_rowStyles.size()!=3u || typed.reactorCount()!=2
        || typed.extensionDictionaryFlag()!=0u || typed.reactorHandles.size()!=2u)return false;
    try {
        Result r;dwgBuffer b(raw.m_rawBytes.data(),raw.m_rawBytes.size(),nullptr);
        const auto total=static_cast<std::uint64_t>(raw.m_rawBytes.size())*8u;
        r.classNumber=b.getObjType(DRW::AC1018);r.dataEndBit=b.getRawLong32();
        if(r.classNumber!=raw.m_objectType || r.dataEndBit>total || r.dataEndBit<bit(b)
            || !handle(b,0u,r.handle) || r.handle!=raw.m_handle || bit(b)>r.dataEndBit
            || b.getBitShort()!=0u)return false;
        std::int32_t reactors=0;
        if(!bl(b,r.dataEndBit,reactors) || reactors!=2 || b.getBit()!=0u || !bound(b,r.dataEndBit))return false;
        // ASCII TV bytes are explicitly bounded; no non-ASCII encoding claim.
        if(b.getBitShort()!=8u)return false;
        for(const auto c:std::string("Standard"))if(b.getRawChar8()!=static_cast<std::uint8_t>(c))return false;
        if(!bound(b,r.dataEndBit) || b.getBitShort()!=0u || b.getBitShort()!=0u
            || !bd(b,r.dataEndBit,1.5) || !bd(b,r.dataEndBit,1.5)
            || b.getBit()!=0u || b.getBit()!=0u || !bound(b,r.dataEndBit))return false;
        const std::array<double,3> heights{{4.5,6.0,4.5}};
        const std::array<int,3> alignments{{2,5,5}};
        for(std::size_t i=0;i<3u;++i) {
            const auto& row=typed.m_rowStyles[i];
            if(row.m_textHeight!=heights[i] || row.m_textAlignment!=alignments[i]
                || row.m_textColor!=0 || row.m_fillColor!=256 || row.m_hasBackgroundColor
                || row.m_valueDataType!=0 || row.m_valueUnitType!=0 || !row.m_valueFormatString.empty()
                || row.m_borders.size()!=6u || !bd(b,r.dataEndBit,heights[i])
                || b.getBitShort()!=static_cast<unsigned>(alignments[i])
                || !cmc(b,r.dataEndBit,0xc1000000u,row.m_textColorCmc)
                || !cmc(b,r.dataEndBit,0xc8000000u,row.m_fillColorCmc)
                || b.getBit()!=0u || !bound(b,r.dataEndBit))return false;
            for(std::size_t j=0;j<6u;++j) {
                const auto& e=row.m_borders[j];
                if(e.m_edgeFlags!=static_cast<int>(1u<<j) || e.m_lineWeight!=-2 || e.m_visible!=1
                    || e.m_color!=0 || e.m_propertyOverrideFlags || e.m_borderType || e.m_lineTypeHandle
                    || e.m_doubleLineSpacing!=0.0 || b.getBitShort()!=65534u || b.getBit()!=1u
                    || !cmc(b,r.dataEndBit,0xc1000000u,e.m_colorCmc))return false;
            }
        }
        if(bit(b)!=r.dataEndBit)return false;
        r.ownerStartBit=bit(b);std::uint32_t firstReactor=0u;
        if(!handle(b,4u,r.owner) || !handle(b,4u,firstReactor) || r.owner!=firstReactor
            || !handle(b,4u,r.tableReactor))return false;
        r.xdictionaryStartBit=bit(b);
        if(!handle(b,3u,r.xdictionary))return false;
        for(auto& s:r.textStyles)if(!handle(b,5u,s))return false;
        r.handleEndBit=bit(b);
        if(!b.isGood() || r.handleEndBit!=total || r.owner==r.handle || r.tableReactor==r.handle
            || r.tableReactor==r.owner || r.xdictionary==r.handle || r.xdictionary==r.owner
            || r.xdictionary==r.tableReactor || r.textStyles[0]!=r.textStyles[1]
            || r.textStyles[0]!=r.textStyles[2] || r.textStyles[0]==r.handle
            || r.textStyles[0]==r.owner || r.textStyles[0]==r.tableReactor || r.textStyles[0]==r.xdictionary
            || typed.parentHandle!=r.owner || typed.xDictHandle!=r.xdictionary
            || typed.reactorHandles[0]!=r.owner || typed.reactorHandles[1]!=r.tableReactor)return false;
        for(std::size_t i=0;i<3u;++i)if(typed.m_rowStyles[i].m_textStyleHandle!=r.textStyles[i])return false;
        const bool validated=drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence);
        if(raw.m_commonHandleDataValidated!=validated)return false;
        if(validated && (raw.m_commonLinkEvidence!=DRW_DwgCommonLinkEvidence::Validated
            || raw.m_parentHandle!=r.owner || raw.m_numReactors!=2 || raw.m_xDictFlag!=0u
            || raw.m_xDictHandle!=r.xdictionary || raw.m_reactorHandles!=typed.reactorHandles))return false;
        result=r;return true;
    } catch(...) {result={};return false;}
}
}
