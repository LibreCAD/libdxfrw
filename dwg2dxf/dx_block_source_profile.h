// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_BLOCK_SOURCE_PROFILE_H
#define DX_BLOCK_SOURCE_PROFILE_H
#include "dx_attdef_source_profile.h"
#include "intern/dxfblock.h"

// Original scalar inventory only. Decoder replay, reconciliation, children,
// dependency/output and independent CAD semantics are separate evidence layers.
namespace dx_block_source {
using namespace dx_attdef_source;
inline Audit audit(const DRW_RawDxfObject& raw,bool footer,bool redundantNameMatches) {
    Audit out;
    const auto reject=[&](Reason why,std::size_t at){out.reason=why;out.rejectedGroup=at;out.spans.clear();return std::move(out);};
    if(raw.m_version<DRW::AC1021||raw.m_version>DRW::AC1032)return reject(Reason::Revision,0u);
    if(raw.name!=(footer?"ENDBLK":"BLOCK")||!raw.handle)return reject(Reason::Identity,0u);
    out.spans.reserve(raw.groups.size());
    std::array<bool,1072> seen{};
    std::array<std::string_view,DRW::kMaxDxfApplicationGroupNesting> applications{};
    std::size_t depth=0u,xdepth=0u,proxyBytes=0u;
    int stage=0,point=0,next=0;
    unsigned reserved=0u;
    std::int64_t proxy=-1;
    bool xdata=false;
    const auto integer=[](const DRW_Variant& v,int lo,int hi){return v.type()==DRW_Variant::INTEGER&&v.i_val()>=lo&&v.i_val()<=hi;};
    for(std::size_t i=0;i<raw.groups.size();++i) {
        const auto& v=raw.groups[i];const int code=v.code();const auto s=text(v);
        Field field=Field::Common;Role role=Role::None;std::uint64_t handle=0u;
        if(v.type()==DRW_Variant::DOUBLE&&!std::isfinite(v.d_val()))return reject(Reason::InvalidValue,i);
        if(code==102) {
            if(xdata)return reject(Reason::Xdata,i);
            field=Field::Application;
            if(s=="}"){if(!depth)return reject(Reason::Application,i);--depth;}
            else if(s.size()>1u&&s.front()=='{'&&depth<applications.size()) {
                if(depth&&(applications[0]=="{ACAD_REACTORS"||applications[0]=="{ACAD_XDICTIONARY"))return reject(Reason::Application,i);
                if(!depth&&(s=="{ACAD_REACTORS"||s=="{ACAD_XDICTIONARY")) {
                    const unsigned bit=s=="{ACAD_REACTORS"?1u:2u;
                    if(reserved&bit)return reject(Reason::DuplicateField,i);
                    reserved|=bit;
                }
                applications[depth++]=s;
            } else return reject(Reason::Application,i);
        } else if(depth) {
            field=Field::Application;
            if(handleCode(code)) {if(!hexHandle(v,handle))return reject(Reason::InvalidValue,i);role=Role::OpaqueHandle;}
            if(applications[0]=="{ACAD_REACTORS") {
                if(depth!=1u||code!=330||handle>UINT32_MAX)return reject(Reason::Application,i);
                role=Role::Reactor;
            } else if(applications[0]=="{ACAD_XDICTIONARY") {
                if(depth!=1u||code!=360||seen[360]||handle>UINT32_MAX)return reject(Reason::Application,i);
                seen[360]=true;role=Role::ExtensionDictionary;
            }
        } else if(code>=1000) {
            field=Field::Xdata;
            if(stage!=2)return reject(Reason::Xdata,i);
            if(point) {if(code!=next)return reject(Reason::IncompletePoint,i);next+=10;if(code==point+20)point=0;}
            else if(code>=1010&&code<=1013){point=code;next=code+10;}
            else if(code>=1020&&code<=1033)return reject(Reason::IncompletePoint,i);
            if(code==1001) {if(xdepth||!name(v))return reject(Reason::Xdata,i);xdata=true;role=Role::AppIdName;}
            else if(!xdata)return reject(Reason::Xdata,i);
            else if(code==1002) {
                if(s=="{"){if(++xdepth>DRW::kMaxDxfApplicationGroupNesting)return reject(Reason::Xdata,i);}
                else if(s=="}"){if(!xdepth)return reject(Reason::Xdata,i);--xdepth;}
                else return reject(Reason::Xdata,i);
            } else if(code==1003){if(!name(v))return reject(Reason::Xdata,i);role=Role::XdataLayerName;}
            else if(code==1005){if(!hexHandle(v,handle)||handle>UINT32_MAX)return reject(Reason::Xdata,i);role=Role::XdataObjectId;}
            else if(code==1004){if(!hexBytes(s)||s.size()>254u)return reject(Reason::Xdata,i);}
            else if(!dxfBlock::Decoder::xdataCode(code))return reject(Reason::UnknownField,i);
        } else {
            if(xdata)return reject(Reason::Xdata,i);
            if(code==100) {
                field=Field::Subclass;
                if(s=="AcDbEntity"&&stage==0)stage=1;
                else if(s==(footer?"AcDbBlockEnd":"AcDbBlockBegin")&&stage==1)stage=2;
                else return reject(Reason::Subclass,i);
            } else if(stage==0) {
                field=Field::Identity;
                if((code!=5&&code!=330)||seen[code]||!hexHandle(v,handle)||handle>UINT32_MAX)return reject(Reason::Identity,i);
                seen[code]=true;role=code==5?Role::Self:Role::Owner;
                if(handle!=(code==5?raw.handle:raw.parentHandle))return reject(Reason::Identity,i);
            } else {
                if(code<0||code>=static_cast<int>(seen.size()))return reject(Reason::UnknownField,i);
                if(seen[code])return reject(Reason::DuplicateField,i);
                seen[code]=true;
                bool block=false;
                if(stage==2&&!footer) {
                    field=Field::Block;block=true;
                    switch(code) {
                    case 2:if(!name(v))return reject(Reason::InvalidValue,i);role=Role::BlockName;break;
                    case 3:
                        if(!seen[2]||!redundantNameMatches)return reject(Reason::RedundantName,i);
                        role=Role::RedundantBlockName;break;
                    case 1:role=Role::XrefPath;break;
                    case 4:case 10:case 20:case 30:case 39:case 50:case 210:case 220:case 230:break;
                    case 70:if(!integer(v,0,127))return reject(Reason::InvalidValue,i);break;
                    case 71:if(!integer(v,0,1))return reject(Reason::InvalidValue,i);break;
                    default:block=false;field=Field::Common;break;
                    }
                }
                if(!block) {
                    switch(code) {
                    case 8:if(!name(v))return reject(Reason::InvalidValue,i);role=Role::LayerName;break;
                    case 6:if(!name(v))return reject(Reason::InvalidValue,i);role=Role::LineTypeName;break;
                    case 347:role=Role::Material;break;
                    case 348:role=Role::FullVisualStyle;break;
                    case 390:role=Role::PlotStyle;break;
                    case 360:role=Role::ExtensionDictionary;break;
                    case 48:case 62:case 420:case 430:case 440:break;
                    case 60:case 67:if(!integer(v,0,1))return reject(Reason::InvalidValue,i);break;
                    case 284:if(!integer(v,0,3))return reject(Reason::InvalidValue,i);break;
                    case 370:
                        if(v.type()!=DRW_Variant::INTEGER||DRW_LW_Conv::lineWidth2dxfInt(DRW_LW_Conv::dxfInt2lineWidth(v.i_val()))!=v.i_val())return reject(Reason::InvalidValue,i);
                        break;
                    case 92:case 160:
                        if(footer||(code==92)!=(raw.m_version==DRW::AC1021)||proxy!=-1)return reject(Reason::ProxyGraphics,i);
                        proxy=code==92?v.i_val():v.i64_val();
                        if(proxy<0||static_cast<std::uint64_t>(proxy)>DRW::kMaxDxfBinaryPayloadBytes)return reject(Reason::ProxyGraphics,i);
                        break;
                    case 310:
                        seen[code]=false;
                        if(footer||proxy<0||!hexBytes(s)||s.size()>254u)return reject(Reason::ProxyGraphics,i);
                        proxyBytes+=s.size()/2u;
                        if(proxyBytes>static_cast<std::uint64_t>(proxy))return reject(Reason::ProxyGraphics,i);
                        break;
                    default:return reject(Reason::UnknownField,i);
                    }
                    if(handleCode(code)&&(!hexHandle(v,handle)||handle>UINT32_MAX))return reject(Reason::InvalidValue,i);
                }
            }
        }
        out.spans.push_back({i,code,Scope::Host,field,role,handle});
    }
    const auto at=raw.groups.size();
    if(depth)return reject(Reason::Application,at);
    if(xdepth)return reject(Reason::Xdata,at);
    if(point)return reject(Reason::IncompletePoint,at);
    if(stage!=2||!seen[5]||!seen[330]||!seen[8])return reject(Reason::Identity,at);
    if(!footer&&(!seen[2]||!seen[70]||!seen[10]||!seen[20]
        ||((seen[210]||seen[220]||seen[230])&&(!seen[210]||!seen[220]))))return reject(Reason::IncompletePoint,at);
    if(proxy>=0&&proxyBytes!=static_cast<std::uint64_t>(proxy))return reject(Reason::ProxyGraphics,at);
    out.reason=Reason::Complete;out.rejectedGroup=at;return out;
}
}
#endif
