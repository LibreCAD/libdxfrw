#include "dwg_cellstylemap_ac1018_candidate.h"
#include "dwgbuffer.h"
#include <cmath>
#include <limits>
namespace DwgCellStyleMapAc1018Candidate {
namespace {
std::uint64_t bit(const dwgBuffer& b) noexcept{return b.getPosition()*8u+b.getBitPos();}
bool bound(const dwgBuffer& b,std::uint64_t end){return b.isGood() && bit(b)<=end;}
bool bl(dwgBuffer& b,std::uint64_t end,std::int32_t expected) {
    const auto tag=b.get2Bits();std::int32_t v=0;
    if(tag==0u)v=static_cast<std::int32_t>(b.getRawLong32());else if(tag==1u)v=b.getRawChar8();
    else if(tag!=2u)return false;
    return bound(b,end) && v==expected;
}
bool bs(dwgBuffer& b,std::uint64_t end,std::uint16_t expected){return b.getBitShort()==expected && bound(b,end);}
bool bd(dwgBuffer& b,std::uint64_t end,double expected) {
    const auto tag=b.get2Bits();double v=0.0;
    if(tag==0u)v=b.getRawDouble();else if(tag==1u)v=1.0;else if(tag!=2u)return false;
    return bound(b,end)&&std::isfinite(v)&&v==expected&&(v!=0.0||!std::signbit(v));
}
bool tv(dwgBuffer& b,std::uint64_t end,const char* expected) {
    std::size_t n=0;while(expected[n])++n;if(!bs(b,end,static_cast<std::uint16_t>(n)))return false;
    for(std::size_t i=0;i<n;++i)if(b.getRawChar8()!=static_cast<std::uint8_t>(expected[i]))return false;
    return bound(b,end);
}
bool cmc(dwgBuffer& b,std::uint64_t end,std::uint32_t expected,const DRW_TableStyleCmcColor& c) {
    if(!bs(b,end,0u)||!bl(b,end,static_cast<std::int32_t>(expected))||b.getRawChar8()!=0u
        || !bound(b,end)||c.m_method!=(expected>>24u)||c.m_flags||c.m_reservedIndex
        || !c.m_name.empty()||!c.m_bookName.empty())return false;
    return expected==0xc8000000u ? c.hasRawDwgPackedValue()&&c.rawDwgPackedValue()==expected : c.m_rgb==-1;
}
bool handle(dwgBuffer& b,std::uint8_t role,Ref& out) {
    out.startBit=bit(b);const auto h=b.getHandle();
    if(!b.isGood()||h.rawCode!=role||!h.size||!h.ref64||h.ref64>std::numeric_limits<std::uint32_t>::max())return false;
    out.target=static_cast<std::uint32_t>(h.ref64);out.endBit=bit(b);return true;
}
}
bool inspect(const DRW_UnsupportedObject& raw,const DRW_CellStyleMap& typed,Result& result) noexcept {
    result={};
    if(raw.m_version!=DRW::AC1018||raw.m_isEntity||!raw.m_isCustomClass||!raw.m_hasClassDefinition
        ||raw.m_hasDataStorage||raw.m_recordName!="CELLSTYLEMAP"||raw.m_className!="AcDbCellStyleMap"
        ||raw.m_objectType<500||raw.m_objectType>std::numeric_limits<std::int16_t>::max()
        ||!raw.m_handle||raw.m_handle==DRW::NoHandle||raw.m_rawBytes.empty()||raw.m_rawBytes.size()>4096u
        ||raw.m_objectSize!=raw.m_rawBytes.size()||raw.m_bodyBitSize!=0u
        ||typed.tType!=DRW::CELLSTYLEMAP||typed.handle!=raw.m_handle||typed.hasDataStorageBinaryData()
        ||!typed.extData.empty()||!typed.appData.empty()||typed.reactorCount()!=1
        ||typed.extensionDictionaryFlag()!=1u||typed.xDictHandle!=0u||typed.reactorHandles.size()!=1u
        ||typed.m_cellStyles.size()!=3u)return false;
    try {
        Result r;dwgBuffer b(raw.m_rawBytes.data(),raw.m_rawBytes.size(),nullptr);
        const auto total=static_cast<std::uint64_t>(raw.m_rawBytes.size())*8u;
        r.classNumber=b.getObjType(DRW::AC1018);r.dataEndBit=b.getRawLong32();Ref self;
        if(r.classNumber!=raw.m_objectType||r.dataEndBit>total||r.dataEndBit<bit(b)
            ||!handle(b,0u,self)||self.target!=raw.m_handle||bit(b)>r.dataEndBit
            ||!bs(b,r.dataEndBit,0u)||!bl(b,r.dataEndBit,1)||b.getBit()!=1u
            ||!bound(b,r.dataEndBit)||!bl(b,r.dataEndBit,3))return false;
        r.handle=self.target;
        const int ids[]{3,1,2},classes[]{2,1,1},align[]{2,5,5};
        const int merge[]{0,32768,0};const double height[]{4.5,6.0,4.5};
        const char* names[]{"_DATA","_TITLE","_HEADER"};
        for(std::size_t i=0;i<3u;++i) {
            const auto& s=typed.m_cellStyles[i];const auto& f=s.m_contentFormat;auto& out=r.styles[i];out.startBit=bit(b);
            if(s.m_id!=ids[i]||s.m_styleClass!=classes[i]||s.m_name!=names[i]
                ||s.m_type!=5||!s.m_hasData||s.m_propertyOverrideFlags||s.m_mergeFlags!=static_cast<unsigned>(merge[i])
                ||s.m_backgroundColor!=256||s.m_contentLayout!=1u||s.m_marginOverrideFlags!=1
                ||f.m_propertyOverrideFlags||f.m_propertyFlags||f.m_valueDataType!=4||f.m_valueUnitType!=0
                ||!f.m_valueFormatString.empty()||f.m_rotation!=0.0||std::signbit(f.m_rotation)
                ||f.m_blockScale!=1.0||f.m_cellAlignment!=align[i]||f.m_contentColor!=0||f.m_textHeight!=height[i]
                ||!bl(b,r.dataEndBit,5)||!bs(b,r.dataEndBit,1u)||!bl(b,r.dataEndBit,0)
                ||!bl(b,r.dataEndBit,merge[i])||!cmc(b,r.dataEndBit,0xc8000000u,s.m_backgroundColorCmc)
                ||!bl(b,r.dataEndBit,1)||!bl(b,r.dataEndBit,0)||!bl(b,r.dataEndBit,0)
                ||!bl(b,r.dataEndBit,4)||!bl(b,r.dataEndBit,0)||!tv(b,r.dataEndBit,"")
                ||!bd(b,r.dataEndBit,0.0)||!bd(b,r.dataEndBit,1.0)||!bl(b,r.dataEndBit,align[i])
                ||!cmc(b,r.dataEndBit,0xc1000000u,f.m_contentColorCmc)||!bd(b,r.dataEndBit,height[i])
                ||!bs(b,r.dataEndBit,1u))return false;
            const double margins[]{s.m_verticalMargin,s.m_horizontalMargin,s.m_bottomMargin,s.m_rightMargin,
                                   s.m_marginHorizontalSpacing,s.m_marginVerticalSpacing};
            for(double m:margins)if(m!=1.5||!bd(b,r.dataEndBit,1.5))return false;
            if(s.m_borders.size()!=6u||!bl(b,r.dataEndBit,6))return false;
            for(std::size_t j=0;j<6u;++j) {
                const auto& edge=s.m_borders[j];const int mask=static_cast<int>(1u<<j);
                if(edge.m_edgeFlags!=mask||edge.m_propertyOverrideFlags||edge.m_borderType!=1||edge.m_color!=0
                    ||edge.m_lineWeight!=-2||edge.m_visible!=0||edge.m_doubleLineSpacing!=0.045
                    ||!bl(b,r.dataEndBit,mask)||!bl(b,r.dataEndBit,0)||!bl(b,r.dataEndBit,1)
                    ||!cmc(b,r.dataEndBit,0xc1000000u,edge.m_colorCmc)||!bl(b,r.dataEndBit,-2)
                    ||!bl(b,r.dataEndBit,0)||!bd(b,r.dataEndBit,0.045))return false;
            }
            if(!bl(b,r.dataEndBit,ids[i])||!bl(b,r.dataEndBit,classes[i])||!tv(b,r.dataEndBit,names[i]))return false;
            out.id=ids[i];out.styleClass=classes[i];out.endBit=bit(b);
        }
        if(bit(b)!=r.dataEndBit)return false;
        Ref owner,reactor;r.ownerStartBit=bit(b);
        if(!handle(b,4u,owner)||!handle(b,4u,reactor)||owner.target!=reactor.target
            ||owner.target==r.handle||typed.parentHandle!=owner.target||typed.reactorHandles[0]!=owner.target)return false;
        r.owner=owner.target;
        for(std::size_t i=0;i<3u;++i) {
            auto& s=r.styles[i];const auto& t=typed.m_cellStyles[i];
            if(!handle(b,5u,s.textStyle)||s.textStyle.target!=t.m_contentFormat.m_textStyleHandle
                ||s.textStyle.target==r.handle||s.textStyle.target==r.owner)return false;
            for(std::size_t j=0;j<6u;++j)if(!handle(b,5u,s.linetypes[j])
                ||s.linetypes[j].target!=t.m_borders[j].m_lineTypeHandle||s.linetypes[j].target==r.handle
                ||s.linetypes[j].target==r.owner||s.linetypes[j].target==s.textStyle.target)return false;
        }
        r.handleEndBit=bit(b);if(r.handleEndBit>total||total-r.handleEndBit>=8u)return false;
        while(bit(b)<total)if(b.getBit()!=0u||!b.isGood())return false;
        const bool validated=drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence);
        if(raw.m_commonHandleDataValidated!=validated)return false;
        if(validated&&(raw.m_commonLinkEvidence!=DRW_DwgCommonLinkEvidence::Validated
            ||raw.m_parentHandle!=r.owner||raw.m_numReactors!=1||raw.m_reactorHandles!=typed.reactorHandles
            ||raw.m_xDictFlag!=1u||raw.m_xDictHandle!=0u))return false;
        result=r;return true;
    } catch(...) {result={};return false;}
}
}
