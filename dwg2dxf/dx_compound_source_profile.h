// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_COMPOUND_SOURCE_PROFILE_H
#define DX_COMPOUND_SOURCE_PROFILE_H
#include "dx_attdef_source_profile.h"
#include "dx_block_membership.h"
#include "intern/dxfcode.h"

// Original scalar role inventory, not another geometry decoder or a target,
// semantic, editable-state or output certificate. Modeled replay is required
// separately. The raw carrier excludes only observer-identified comments;
// restorePhysicalIndices restores their exact original inventory positions.
namespace dx_compound_source {
using namespace dx_attdef_source;
inline bool scalarType(const DRW_Variant& value) noexcept {
    if(value.isLayerRef()||value.rawDwgLayerReference()!=0u||value.dwgCmColorValue())return false;
    switch(dxfValueKindForCode(value.code())) {
    case DxfValueKind::Dbl:return value.type()==DRW_Variant::DOUBLE&&std::isfinite(value.d_val());
    case DxfValueKind::I64:return value.type()==DRW_Variant::INTEGER64;
    case DxfValueKind::I16:return value.type()==DRW_Variant::INTEGER&&value.i_val()>=-32768&&value.i_val()<=32767;
    case DxfValueKind::I32:return value.type()==DRW_Variant::INTEGER;
    case DxfValueKind::Bln:return value.type()==DRW_Variant::INTEGER&&(value.i_val()==0||value.i_val()==1);
    case DxfValueKind::Str:case DxfValueKind::Bin:case DxfValueKind::Unknown:
        return value.type()==DRW_Variant::STRING&&value.content.s&&value.content.s->find('\0')==std::string::npos;
    }
    return false;
}
inline Audit audit(const DRW_RawDxfObject& raw,const DRW_Polyline* parent=nullptr,bool allowLayoutName=false,bool allowPrimary=false) {
    Audit out;
    const auto reject=[&](Reason why,std::size_t at){out.reason=why;out.rejectedGroup=at;out.spans.clear();return std::move(out);};
    for(std::size_t i=0;i<raw.groups.size();++i)if(!scalarType(raw.groups[i]))return reject(Reason::InvalidValue,i);
    if(raw.name=="ATTRIB")return dx_attdef_source::audit(raw,false,allowLayoutName);
    if(raw.m_version<DRW::AC1021||raw.m_version>DRW::AC1032)return reject(Reason::Revision,0u);
    const bool polyline=dx_block_membership::equal(raw.name,"POLYLINE"),vertex=dx_block_membership::equal(raw.name,"VERTEX");
    const bool insert=dx_block_membership::equal(raw.name,"INSERT")||dx_block_membership::equal(raw.name,"MINSERT");
    const bool footer=dx_block_membership::equal(raw.name,"SEQEND");
    const bool pointRoot=allowPrimary&&dx_block_membership::equal(raw.name,"POINT");
    const bool lineRoot=allowPrimary&&dx_block_membership::equal(raw.name,"LINE");
    if((!polyline&&!vertex&&!insert&&!footer&&!pointRoot&&!lineRoot)||!raw.handle||(vertex&&!parent))return reject(Reason::Identity,0u);
    out.spans.reserve(raw.groups.size()); // entire bound precharged by caller
    std::array<bool,1072> seen{};
    std::array<std::string_view,DRW::kMaxDxfApplicationGroupNesting> applications{};
    std::size_t depth=0u,xdepth=0u,proxyBytes=0u;unsigned reserved=0u;
    int stage=0,point=0,next=0,flags=0,columns=1,rows=1;std::int64_t proxy=-1;
    bool xdata=false,vertexMarker=false;std::string_view subtype;
    const auto integer=[](const DRW_Variant& v,int lo,int hi){return v.type()==DRW_Variant::INTEGER&&v.i_val()>=lo&&v.i_val()<=hi;};
    const auto pair=[&](int x,bool required){return (!seen[x]&&!seen[x+10]&&!seen[x+20]&&!required)||(seen[x]&&seen[x+10]);};
    for(std::size_t i=0;i<raw.groups.size();++i) {
        const auto& v=raw.groups[i];const int code=v.code();const auto s=text(v);
        Field field=Field::Common;Role role=Role::None;std::uint64_t handle=0u;
        if(v.type()==DRW_Variant::DOUBLE&&!std::isfinite(v.d_val()))return reject(Reason::InvalidValue,i);
        if(code==102) {
            if(xdata)return reject(Reason::Xdata,i);field=Field::Application;
            if(s=="}"){if(!depth)return reject(Reason::Application,i);--depth;}
            else if(s.size()>1u&&s.front()=='{'&&depth<applications.size()) {
                if(depth&&(applications[0]=="{ACAD_REACTORS"||applications[0]=="{ACAD_XDICTIONARY"))return reject(Reason::Application,i);
                if(!depth&&(s=="{ACAD_REACTORS"||s=="{ACAD_XDICTIONARY")) {
                    const unsigned bit=s=="{ACAD_REACTORS"?1u:2u;
                    if(reserved&bit)return reject(Reason::DuplicateField,i);reserved|=bit;
                }
                applications[depth++]=s;
            }else return reject(Reason::Application,i);
        } else if(depth) {
            field=Field::Application;
            if(handleCode(code)){if(!hexHandle(v,handle))return reject(Reason::InvalidValue,i);role=Role::OpaqueHandle;}
            if(applications[0]=="{ACAD_REACTORS") {
                if(depth!=1u||code!=330||handle>UINT32_MAX)return reject(Reason::Application,i);role=Role::Reactor;
            }else if(applications[0]=="{ACAD_XDICTIONARY") {
                if(depth!=1u||code!=360||seen[360]||handle>UINT32_MAX)return reject(Reason::Application,i);
                seen[360]=true;role=Role::ExtensionDictionary;
            }
        } else if(code>=1000) {
            field=Field::Xdata;
            if(stage!=(footer?1:2)||(!footer&&subtype.empty()))return reject(Reason::Xdata,i);
            if(point){if(code!=next)return reject(Reason::IncompletePoint,i);next+=10;if(code==point+20)point=0;}
            else if(code>=1010&&code<=1013){point=code;next=code+10;}
            else if(code>=1020&&code<=1033)return reject(Reason::IncompletePoint,i);
            if(code==1001){if(xdepth||!name(v))return reject(Reason::Xdata,i);xdata=true;role=Role::AppIdName;}
            else if(!xdata)return reject(Reason::Xdata,i);
            else if(code==1002) {
                if(s=="{"){if(++xdepth>DRW::kMaxDxfApplicationGroupNesting)return reject(Reason::Xdata,i);}
                else if(s=="}"){if(!xdepth)return reject(Reason::Xdata,i);--xdepth;}
                else return reject(Reason::Xdata,i);
            }else if(code==1003){if(!name(v))return reject(Reason::Xdata,i);role=Role::XdataLayerName;}
            else if(code==1005){if(!hexHandle(v,handle)||handle>UINT32_MAX)return reject(Reason::Xdata,i);role=Role::XdataObjectId;}
            else if(code==1004){if(!hexBytes(s)||s.size()>254u)return reject(Reason::Xdata,i);}
            else if(code!=1000&&!(code>=1010&&code<=1013)&&!(code>=1020&&code<=1023)
                &&!(code>=1030&&code<=1033)&&!(code>=1040&&code<=1042)&&code!=1070&&code!=1071)return reject(Reason::UnknownField,i);
        } else {
            if(xdata)return reject(Reason::Xdata,i);
            if(code==100) {
                field=Field::Subclass;
                if(s=="AcDbEntity"&&stage==0)stage=1;
                else if(vertex&&stage==1&&s=="AcDbVertex"){vertexMarker=true;stage=2;}
                else if(!footer&&subtype.empty()&&((stage==1&&!vertexMarker)||(vertex&&vertexMarker&&stage==2))) {
                    bool known=false;
                    if(polyline)known=s=="AcDb2dPolyline"||s=="AcDb3dPolyline"||s=="AcDbPolygonMesh"||s=="AcDbPolyFaceMesh";
                    if(insert)known=s=="AcDbBlockReference"||s=="AcDbMInsertBlock";
                    if(pointRoot)known=s=="AcDbPoint";
                    if(lineRoot)known=s=="AcDbLine";
                    if(vertex)known=vertexMarker?(s=="AcDb2dVertex"||s=="AcDb3dPolylineVertex"||s=="AcDbPolygonMeshVertex"||s=="AcDbPolyFaceMeshVertex"):s=="AcDbFaceRecord";
                    if(!known)return reject(Reason::Subclass,i);subtype=s;stage=2;
                }else return reject(Reason::Subclass,i);
            } else if((code==5||code==330)&&stage<=1) {
                field=Field::Identity;
                if(seen[code]||!hexHandle(v,handle)||handle>UINT32_MAX||handle!=(code==5?raw.handle:raw.parentHandle))return reject(Reason::Identity,i);
                seen[code]=true;role=code==5?Role::Self:Role::Owner;
            } else {
                if(stage==0||code<0||code>=static_cast<int>(seen.size()))return reject(Reason::UnknownField,i);
                if(seen[code])return reject(Reason::DuplicateField,i);seen[code]=true;
                bool body=false;
                if(stage==2&&!subtype.empty()) {
                    field=Field::Compound;body=true;
                    if(pointRoot||lineRoot) {
                        field=Field::Primary;
                        switch(code) {
                        case 10:case 20:case 30:case 39:case 210:case 220:case 230:break;
                        case 50:if(!pointRoot)body=false;break;
                        case 11:case 21:case 31:if(!lineRoot)body=false;break;
                        default:body=false;break;
                        }
                    }else if(polyline) {
                        switch(code) {
                        case 66:field=Field::ComputedIgnored;break; // obsolete, not INSERT follow semantics
                        case 10:case 20:
                            if(v.type()!=DRW_Variant::DOUBLE||v.d_val()!=0.0)return reject(Reason::InvalidValue,i);
                            field=Field::ComputedIgnored;break;
                        case 30:case 39:case 40:case 41:case 210:case 220:case 230:break;
                        case 70:if(!integer(v,0,255))return reject(Reason::InvalidValue,i);flags=v.i_val();break;
                        case 71:case 72:case 73:case 74:if(!integer(v,0,65535))return reject(Reason::InvalidValue,i);break;
                        case 75:if(!integer(v,0,0)&&!integer(v,5,6)&&!integer(v,8,8))return reject(Reason::InvalidValue,i);break;
                        default:body=false;break;
                        }
                    }else if(vertex) {
                        switch(code) {
                        case 10:case 20:case 30:case 40:case 41:case 42:case 50:break;
                        case 70:if(!integer(v,0,255))return reject(Reason::InvalidValue,i);flags=v.i_val();break;
                        case 71:case 72:case 73:case 74:if(!integer(v,-32768,32767))return reject(Reason::InvalidValue,i);break;
                        case 91:if(v.type()!=DRW_Variant::INTEGER)return reject(Reason::InvalidValue,i);break;
                        default:body=false;break;
                        }
                    }else {
                        switch(code) {
                        case 2:if(!name(v))return reject(Reason::InvalidValue,i);role=Role::BlockName;break;
                        case 10:case 20:case 30:case 41:case 42:case 43:case 44:case 45:case 50:case 210:case 220:case 230:break;
                        case 66:if(!integer(v,0,1))return reject(Reason::InvalidValue,i);break;
                        case 70:case 71:
                            if(!integer(v,1,32767))return reject(Reason::InvalidValue,i);
                            (code==70?columns:rows)=v.i_val();break;
                        default:body=false;break;
                        }
                    }
                }
                if(!body) {
                    field=Field::Common;
                    switch(code) {
                    case 8:if(!name(v))return reject(Reason::InvalidValue,i);role=Role::LayerName;break;
                    case 6:if(!name(v))return reject(Reason::InvalidValue,i);role=Role::LineTypeName;break;
                    case 410:if(!allowLayoutName||!name(v))return reject(Reason::UnknownField,i);role=Role::LayoutName;break;
                    case 347:role=Role::Material;break;case 348:role=Role::FullVisualStyle;break;
                    case 390:role=Role::PlotStyle;break;case 360:role=Role::ExtensionDictionary;break;
                    case 48:case 62:case 420:case 430:case 440:break;
                    case 60:case 67:if(!integer(v,0,1))return reject(Reason::InvalidValue,i);break;
                    case 284:if(!integer(v,0,3))return reject(Reason::InvalidValue,i);break;
                    case 370:
                        if(v.type()!=DRW_Variant::INTEGER||DRW_LW_Conv::lineWidth2dxfInt(DRW_LW_Conv::dxfInt2lineWidth(v.i_val()))!=v.i_val())return reject(Reason::InvalidValue,i);
                        break;
                    case 92:case 160:
                        if((code==92)!=(raw.m_version==DRW::AC1021)||proxy!=-1)return reject(Reason::ProxyGraphics,i);
                        proxy=code==92?v.i_val():v.i64_val();
                        if(proxy<0||static_cast<std::uint64_t>(proxy)>DRW::kMaxDxfBinaryPayloadBytes)return reject(Reason::ProxyGraphics,i);
                        break;
                    case 310:
                        seen[code]=false;
                        if(proxy<0||!hexBytes(s)||s.size()>254u)return reject(Reason::ProxyGraphics,i);
                        proxyBytes+=s.size()/2u;if(proxyBytes>static_cast<std::uint64_t>(proxy))return reject(Reason::ProxyGraphics,i);
                        break;
                    default:return reject(Reason::UnknownField,i);
                    }
                    if(handleCode(code)&&(!hexHandle(v,handle)||handle>UINT32_MAX))return reject(Reason::InvalidValue,i);
                }
            }
        }
        out.spans.push_back({i,code,Scope::Host,field,role,handle});
    }
    const auto end=raw.groups.size();
    if(depth)return reject(Reason::Application,end);if(xdepth)return reject(Reason::Xdata,end);if(point)return reject(Reason::IncompletePoint,end);
    if(stage!=(footer?1:2)||!seen[5]||!seen[330]||!seen[8]||(!footer&&subtype.empty()))return reject(Reason::Identity,end);
    if(proxy>=0&&proxyBytes!=static_cast<std::uint64_t>(proxy))return reject(Reason::ProxyGraphics,end);
    if(pointRoot||lineRoot) {
        if(!pair(10,true)||(lineRoot&&!pair(11,true)))return reject(Reason::IncompletePoint,end);
    }else if(polyline) {
        const auto expected=(flags&64)?"AcDbPolyFaceMesh":(flags&16)?"AcDbPolygonMesh":(flags&8)?"AcDb3dPolyline":"AcDb2dPolyline";
        if(subtype!=expected||((flags&64)&&((flags&24)!=0))||((flags&16)&&((flags&8)!=0)))return reject(Reason::Subclass,end);
        if(!pair(10,true)||!pair(210,false))return reject(Reason::IncompletePoint,end);
    }else if(vertex) {
        const bool face=(parent->flags&64)&&(flags&128)&&!(flags&64);
        const auto expected=(parent->flags&64)?(face?"AcDbFaceRecord":"AcDbPolyFaceMeshVertex"):
            (parent->flags&16)?"AcDbPolygonMeshVertex":(parent->flags&8)?"AcDb3dPolylineVertex":"AcDb2dVertex";
        const int mode=flags&224,expectedMode=(parent->flags&64)?(face?128:192):(parent->flags&16)?64:(parent->flags&8)?32:0;
        if(subtype!=expected||mode!=expectedMode)return reject(Reason::Subclass,end);
        if(face) {
            for(auto& span:out.spans)if(span.field==Field::Compound&&(span.code==10||span.code==20||span.code==30))span.field=Field::ComputedIgnored;
            if(!seen[71]||!seen[72]||!seen[73])return reject(Reason::IncompletePoint,end);
        }else if(!pair(10,true))return reject(Reason::IncompletePoint,end);
    }else if(insert) {
        if(!seen[2]||!pair(10,true)||!pair(210,false))return reject(Reason::IncompletePoint,end);
        // Canonical multi-insert output uses this subclass when either grid
        // dimension exceeds one; missing dimensions retain their default one.
        // The already admitted base-class/grid compatibility shape is kept.
        if(subtype=="AcDbMInsertBlock"&&columns==1&&rows==1)return reject(Reason::Subclass,end);
    }
    out.reason=Reason::Complete;out.rejectedGroup=end;return out;
}

inline bool restorePhysicalIndices(Audit& profile,const DRW_DxfPhysicalEntitySource& physical) {
    const auto n=physical.groups.size();
    if(physical.spans.size()!=n)return false;
    if(!profile.complete()) {
        std::size_t k=0u;
        for(std::size_t j=0;j<n;++j)if(!physical.spans[j].ignoredComment&&k++==profile.rejectedGroup){profile.rejectedGroup=j;return true;}
        profile.rejectedGroup=n;return true;
    }
    std::size_t expected=0u;
    for(std::size_t j=0;j<n;++j) {
        if(physical.spans[j].ignoredComment){if(physical.groups[j].code()!=999)return false;}
        else ++expected;
    }
    std::size_t k=profile.spans.size();if(k!=expected)return false;profile.spans.resize(n);
    // Backward in-place merge is linear and cannot overwrite an unread span.
    for(std::size_t j=n;j-->0u;) {
        if(physical.spans[j].ignoredComment)profile.spans[j]={j,physical.groups[j].code(),Scope::Ignored,Field::IgnoredComment,Role::None,0u};
        else {
            if(!k)return false;auto span=profile.spans[--k];span.groupIndex=j;
            if(span.code!=physical.groups[j].code())return false;profile.spans[j]=span;
        }
    }
    profile.rejectedGroup=n;return k==0u;
}
}
#endif
