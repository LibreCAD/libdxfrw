#include "dwg_ltype_control_byblock_ac1018_candidate.h"
#include "dwgbuffer.h"
#include <cmath>
#include <limits>
namespace DwgLTypeControlByBlockAc1018Candidate {
namespace {
std::uint64_t bit(const dwgBuffer& b) noexcept{return b.getPosition()*8u+b.getBitPos();}
bool within(const dwgBuffer& b,std::uint64_t end){return b.isGood()&&bit(b)<=end;}
bool bs(dwgBuffer& b,std::uint64_t end,std::uint16_t value){return b.getBitShort()==value&&within(b,end);}
bool bl(dwgBuffer& b,std::uint64_t end,std::int32_t value){const auto tag=b.get2Bits();std::int32_t x=0;
    if(tag==0u)x=static_cast<std::int32_t>(b.getRawLong32());else if(tag==1u)x=b.getRawChar8();else if(tag!=2u)return false;
    return within(b,end)&&x==value;}
bool bdZero(dwgBuffer& b,std::uint64_t end){const auto tag=b.get2Bits();double x=0.0;
    if(tag==0u)x=b.getRawDouble();else if(tag==1u)x=1.0;else if(tag!=2u)return false;
    return within(b,end)&&std::isfinite(x)&&x==0.0&&!std::signbit(x);}
bool handle(dwgBuffer& b,std::uint8_t role,Ref& out,bool nullOnly=false){out.startBit=bit(b);const auto h=b.getHandle();
    if(!b.isGood()||h.rawCode!=role||h.ref64>std::numeric_limits<std::uint32_t>::max())return false;
    if(nullOnly ? (h.size!=0u||h.ref64!=0u) : (h.size==0u||h.ref64==0u))return false;
    out.target=static_cast<std::uint32_t>(h.ref64);out.endBit=bit(b);return true;}
bool padding(dwgBuffer& b,std::uint64_t total){if(bit(b)>total||total-bit(b)>=8u)return false;
    while(bit(b)<total) {if(b.getBit()!=0u||!b.isGood())return false;}
    return true;}
bool metadata(const DRW_UnsupportedObject& raw,int type){return raw.m_version==DRW::AC1018&&raw.m_objectType==type
    &&!raw.m_isEntity&&!raw.m_isCustomClass&&!raw.m_hasClassDefinition&&!raw.m_hasDataStorage
    &&raw.m_handle!=0u&&raw.m_handle!=DRW::NoHandle&&!raw.m_rawBytes.empty()&&raw.m_rawBytes.size()<=4096u
    &&raw.m_objectSize==raw.m_rawBytes.size()&&raw.m_bodyBitSize==0u;}
bool common(dwgBuffer& b,const DRW_UnsupportedObject& raw,int type,std::uint32_t& end){
    const auto total=static_cast<std::uint64_t>(raw.m_rawBytes.size())*8u;
    if(b.getObjType(DRW::AC1018)!=type)return false;
    end=b.getRawLong32();Ref self;
    return b.isGood()&&end<=total&&end>=bit(b)&&handle(b,0u,self)&&self.target==raw.m_handle
        &&within(b,end)&&bs(b,end,0u)&&bl(b,end,0)&&b.getBit()==1u&&within(b,end);
}
}
namespace {
bool inspectOrder(const DRW_UnsupportedObject& control,const DRW_UnsupportedObject& row,
                  const DRW_LType& typed,Result& result,bool normative) noexcept {
    result={};
    const bool byBlockName=typed.name=="ByBlock"||(normative&&typed.name=="BYBLOCK");
    if(!metadata(control,56)||!metadata(row,57)||control.m_handle==row.m_handle
        ||typed.tType!=DRW::LTYPE||typed.handle!=row.m_handle||typed.parentHandle!=control.m_handle
        ||!byBlockName||!typed.desc.empty()||typed.flags!=0x40||typed.size!=0
        ||typed.length!=0.0||std::signbit(typed.length)||typed.alignment!=65||!typed.path.empty()
        ||!typed.segments.empty()||!typed.shapeHandles.empty()||typed.xrefBlockHandle.ref64!=0u
        ||typed.xrefBlockHandle.ref!=0u||typed.xrefBlockHandle.size!=0u
        ||!((typed.xrefBlockHandle.code==5u&&typed.xrefBlockHandle.rawCode==5u)
            ||(typed.xrefBlockHandle.code==0u&&typed.xrefBlockHandle.rawCode==0u))
        ||typed.reactorCount()!=0||!typed.reactorHandles.empty()
        ||typed.xDictHandle!=0u||typed.hasDataStorageBinaryData()||!typed.extData.empty()||!typed.appData.empty())return false;
    try {
        Result r;dwgBuffer c(control.m_rawBytes.data(),control.m_rawBytes.size(),nullptr);
        const auto controlTotal=static_cast<std::uint64_t>(control.m_rawBytes.size())*8u;
        if(!common(c,control,56,r.controlDataEndBit)||!bl(c,r.controlDataEndBit,1)
            ||bit(c)!=r.controlDataEndBit)return false;
        Ref nullOwner,firstPhantom,secondPhantom;
        if(!handle(c,4u,nullOwner,true)||!handle(c,2u,r.ordinary)
            ||!handle(c,3u,firstPhantom)||!handle(c,3u,secondPhantom))return false;
        if(normative){r.byLayerPhantom=firstPhantom;r.byBlockPhantom=secondPhantom;}
        else {r.byBlockPhantom=firstPhantom;r.byLayerPhantom=secondPhantom;}
        r.controlHandleEndBit=bit(c);if(!padding(c,controlTotal))return false;
        r.control=control.m_handle;r.byBlock=row.m_handle;
        if(r.byBlockPhantom.target!=r.byBlock||r.ordinary.target==r.control||r.ordinary.target==r.byBlock
            ||r.byLayerPhantom.target==r.control||r.byLayerPhantom.target==r.byBlock
            ||r.byLayerPhantom.target==r.ordinary.target)return false;
        dwgBuffer b(row.m_rawBytes.data(),row.m_rawBytes.size(),nullptr);
        const auto total=static_cast<std::uint64_t>(row.m_rawBytes.size())*8u;
        if(!common(b,row,57,r.ltypeDataEndBit)||!bs(b,r.ltypeDataEndBit,7u))return false;
        const char* name=normative?"BYBLOCK":"ByBlock";
        for(std::size_t i=0;i<7u;++i)if(b.getRawChar8()!=static_cast<std::uint8_t>(name[i]))return false;
        if(!within(b,r.ltypeDataEndBit)||b.getBit()!=1u||!bs(b,r.ltypeDataEndBit,0u)||b.getBit()!=0u
            ||!bs(b,r.ltypeDataEndBit,0u)||!bdZero(b,r.ltypeDataEndBit)||b.getRawChar8()!=65u
            ||b.getRawChar8()!=0u||!within(b,r.ltypeDataEndBit))return false;
        for(std::size_t i=0;i<256u;++i)if(b.getRawChar8()!=0u||!within(b,r.ltypeDataEndBit))return false;
        if(bit(b)!=r.ltypeDataEndBit||!handle(b,4u,r.rowOwner)||r.rowOwner.target!=r.control)return false;
        Ref nullXref;if(!handle(b,5u,nullXref,true))return false;
        r.ltypeHandleEndBit=bit(b);if(!padding(b,total))return false;
        const bool cv=drwDwgCommonLinkEvidenceValidated(control.m_commonLinkEvidence);
        if(control.m_commonHandleDataValidated!=cv||cv
            ||(control.m_commonLinkEvidence!=DRW_DwgCommonLinkEvidence::Unknown
                &&control.m_commonLinkEvidence!=DRW_DwgCommonLinkEvidence::NotApplicable))return false;
        const bool rv=drwDwgCommonLinkEvidenceValidated(row.m_commonLinkEvidence);
        if(row.m_commonHandleDataValidated!=rv
            ||(!rv&&row.m_commonLinkEvidence!=DRW_DwgCommonLinkEvidence::Unknown))return false;
        if(rv&&(row.m_commonLinkEvidence!=DRW_DwgCommonLinkEvidence::Validated
            ||row.m_parentHandle!=r.control||row.m_numReactors!=0||!row.m_reactorHandles.empty()
            ||row.m_xDictFlag!=1u||row.m_xDictHandle!=0u))return false;
        result=r;return true;
    }catch(...){result={};return false;}
}
}
bool inspect(const DRW_UnsupportedObject& control,const DRW_UnsupportedObject& row,
             const DRW_LType& typed,Result& result) noexcept {
    return inspectOrder(control,row,typed,result,false);
}
bool inspectNormativeOrder(const DRW_UnsupportedObject& control,const DRW_UnsupportedObject& row,
                           const DRW_LType& typed,Result& result) noexcept {
    return inspectOrder(control,row,typed,result,true);
}
}
