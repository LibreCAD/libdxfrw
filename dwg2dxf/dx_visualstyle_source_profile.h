// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_VISUALSTYLE_SOURCE_PROFILE_H
#define DX_VISUALSTYLE_SOURCE_PROFILE_H
#include "dx_compound_source_profile.h"
#include "intern/drw_textcodec.h"
#include <cstring>

// Independent canonical primary-field inventory. This does not call the DXF
// model decoder or use its code tables. Extension/foreign/legacy shapes remain
// unqualified, with their original raw inputs retained by the caller.
namespace dx_visualstyle_source {
using P=DRW_DxfVisualStyleProperties;
static_assert(static_cast<std::size_t>(P::Integer::Count)==28u&&static_cast<std::size_t>(P::Real::Count)==5u,"Update the independent source schema explicitly");
enum class Field {Identity,Subclass,Primary,Application,Xdata,Comment};
enum class Role {None,Self,Owner,Reactor,ExtensionDictionary,AppId,Layer,Object,Opaque};
enum class Reason {Matched,Revision,Identity,Subclass,UnknownField,Duplicate,MissingField,Application,Xdata,Value,Extension,PhysicalEnvelope};
struct Span {std::size_t group;int code;Field field;Role role;std::uint32_t target;};
struct Audit {
    static constexpr std::size_t Absent=std::numeric_limits<std::size_t>::max();
    Reason reason{Reason::Identity};std::size_t rejected{0},description{Absent},tailGroups{0};
    std::array<std::size_t,28> integers;std::array<std::size_t,5> reals;std::vector<Span> spans;
    Audit(){integers.fill(Absent);reals.fill(Absent);}
    bool complete()const noexcept{return reason==Reason::Matched;}
};
inline std::optional<P::Integer> integer(int code) noexcept {
    using I=P::Integer;
    switch(code){case 70:return I::Type;case 71:return I::FaceLightingModel;case 72:return I::FaceLightingQuality;
    case 73:return I::FaceColorMode;case 90:return I::FaceModifiers;case 62:return I::Color62;case 63:return I::Color63;
    case 421:return I::FaceMonoRgb;case 74:return I::EdgeModel;case 91:return I::EdgeStyle;case 64:return I::EdgeIntersectionColor;
    case 65:return I::EdgeObscuredColor;case 75:return I::EdgeObscuredLinetype;case 175:return I::EdgeIntersectionLinetype;
    case 92:return I::EdgeModifiers;case 66:return I::EdgeColor;case 76:return I::EdgeWidth;case 77:return I::EdgeOverhang;
    case 78:return I::EdgeJitter;case 67:return I::EdgeSilhouetteColor;case 79:return I::EdgeSilhouetteWidth;case 170:return I::EdgeHaloGap;
    case 171:return I::EdgeIsolines;case 290:return I::EdgeHidePrecision;case 174:return I::EdgeStyleApply;case 93:return I::DisplaySettings;
    case 173:return I::ShadowType;case 291:return I::InternalOnly;default:return {};}
}
inline std::optional<P::Real> real(int code) noexcept {
    switch(code){case 40:return P::Real::FaceOpacity;case 41:return P::Real::FaceSpecular;case 42:return P::Real::EdgeCreaseAngle;
    case 43:return P::Real::EdgeOpacity;case 44:return P::Real::Brightness;default:return {};}
}
inline Audit audit(const DRW_RawDxfObject& raw) {
    Audit a;const auto reject=[&](Reason why,std::size_t at){a.reason=why;a.rejected=at;a.spans.clear();return std::move(a);};
    if(raw.m_version<DRW::AC1021||raw.m_version>DRW::AC1032)return reject(Reason::Revision,0u);
    if(!raw.handle||!raw.parentHandle||!dx_block_membership::equal(raw.name,"VISUALSTYLE"))return reject(Reason::Identity,0u);
    a.spans.reserve(raw.groups.size());std::array<std::string_view,DRW::kMaxDxfApplicationGroupNesting> apps{};
    std::size_t depth=0u,xdepth=0u,extensions=0u;unsigned reserved=0u,self=0u,owner=0u;int stage=0,point=0,next=0;
    bool tail=false,xdata=false;
    for(std::size_t i=0u;i<raw.groups.size();++i){const auto& v=raw.groups[i];const auto text=dx_attdef_source::text(v);const int code=v.code();
        Field field=Field::Primary;Role role=Role::None;std::uint64_t target=0u;
        if(!dx_compound_source::scalarType(v))return reject(Reason::Value,i);
        if(tail&&!depth&&code!=102)++a.tailGroups;
        if(code==999){field=Field::Comment;}
        else if(code==102){field=Field::Application;if(xdata)return reject(Reason::Application,i);
            if(text=="}"){if(!depth||(apps[0]=="{ACAD_XDICTIONARY"&&extensions!=1u))return reject(Reason::Application,i);--depth;}
            else if(text.size()>1u&&text.front()=='{'&&depth<apps.size()){
                if(depth&&(apps[0]=="{ACAD_REACTORS"||apps[0]=="{ACAD_XDICTIONARY"))return reject(Reason::Application,i);
                if(!depth&&(text=="{ACAD_REACTORS"||text=="{ACAD_XDICTIONARY")){const auto bit=text=="{ACAD_REACTORS"?1u:2u;if(reserved&bit)return reject(Reason::Duplicate,i);reserved|=bit;}
                apps[depth++]=text;
            }else return reject(Reason::Application,i);
        }else if(depth){field=Field::Application;
            if(dx_attdef_source::handleCode(code)){if(!dx_attdef_source::hexHandle(v,target)||target>UINT32_MAX)return reject(Reason::Value,i);role=Role::Opaque;}
            if(apps[0]=="{ACAD_REACTORS"){if(depth!=1u||code!=330)return reject(Reason::Application,i);role=Role::Reactor;}
            if(apps[0]=="{ACAD_XDICTIONARY"){if(depth!=1u||code!=360||extensions++)return reject(Reason::Application,i);role=Role::ExtensionDictionary;}
        }else if(code>=1000){field=Field::Xdata;if(stage!=2||!tail)return reject(Reason::Xdata,i);
            if(point){if(code!=next)return reject(Reason::Xdata,i);next+=10;if(code==point+20)point=0;}
            else if(code>=1010&&code<=1013){point=code;next=code+10;}
            else if(code>=1020&&code<=1033)return reject(Reason::Xdata,i);
            if(code==1001){if(xdepth||!dx_attdef_source::name(v))return reject(Reason::Xdata,i);xdata=true;role=Role::AppId;}
            else if(!xdata)return reject(Reason::Xdata,i);
            else if(code==1002){if(text=="{"){if(++xdepth>DRW::kMaxDxfApplicationGroupNesting)return reject(Reason::Xdata,i);}else if(text=="}"){if(!xdepth)return reject(Reason::Xdata,i);--xdepth;}else return reject(Reason::Xdata,i);}
            else if(code==1003){if(!dx_attdef_source::name(v))return reject(Reason::Xdata,i);role=Role::Layer;}
            else if(code==1005){if(!dx_attdef_source::hexHandle(v,target)||target>UINT32_MAX)return reject(Reason::Xdata,i);role=Role::Object;}
            else if(code==1004){if(!dx_attdef_source::hexBytes(text)||text.size()>254u)return reject(Reason::Xdata,i);}
            else if(code!=1000&&!(code>=1010&&code<=1013)&&!(code>=1020&&code<=1023)&&!(code>=1030&&code<=1033)&&!(code>=1040&&code<=1042)&&code!=1070&&code!=1071)return reject(Reason::Xdata,i);
        }else{
            if(xdata)return reject(Reason::Xdata,i);
            if(code==100){field=Field::Subclass;if(!stage&&text=="AcDbObject")stage=1;else if(stage<2&&text=="AcDbVisualStyle")stage=2;else return reject(tail?Reason::Extension:Reason::Subclass,i);}
            else if(stage<2&&(code==5||code==330)){field=Field::Identity;
                if(!dx_attdef_source::hexHandle(v,target)||target>UINT32_MAX||target!=(code==5?raw.handle:raw.parentHandle)
                    ||(code==5?self++:owner++))return reject(Reason::Identity,i);
                role=code==5?Role::Self:Role::Owner;
            }else if(stage!=2)return reject(Reason::Subclass,i);
            else if(tail)return reject(Reason::Extension,i);
            else if(code==2){if(a.description!=Audit::Absent)return reject(Reason::Duplicate,i);a.description=i;}
            else if(const auto slot=integer(code)){auto& position=a.integers[static_cast<std::size_t>(*slot)];if(position!=Audit::Absent)return reject(Reason::Duplicate,i);position=i;if(code==291)tail=true;}
            else if(const auto slot=real(code)){auto& position=a.reals[static_cast<std::size_t>(*slot)];if(position!=Audit::Absent)return reject(Reason::Duplicate,i);position=i;}
            else return reject(Reason::UnknownField,i);
        }
        a.spans.push_back({i,code,field,role,static_cast<std::uint32_t>(target)});
    }
    const auto end=raw.groups.size();if(stage!=2||!tail||depth||xdepth||point||self!=1u||owner!=1u||a.description==Audit::Absent)return reject(Reason::MissingField,end);
    for(auto position:a.integers)if(position==Audit::Absent)return reject(Reason::MissingField,end);
    for(auto position:a.reals)if(position==Audit::Absent)return reject(Reason::MissingField,end);
    a.reason=Reason::Matched;a.rejected=end;return a;
}
inline bool integerModel(const DRW_VisualStyle& m,int code,int value) noexcept {
    const auto& b=m.m_body;
    switch(code){case 70:return m.type==value;case 71:return b.faceLightingModel==value;case 72:return b.faceLightingQuality==value;
    case 73:return b.faceColorMode==value;case 90:return b.faceModifier==value;case 62:case 63:case 421:return true;
    case 74:return b.edgeModel==value;case 91:return b.edgeStyle==value;case 64:return b.edgeIntersectionColor==value;case 65:return b.edgeObscuredColor==value;
    case 75:return b.edgeObscuredLtype==value;case 175:return b.edgeIntersectionLtype==value;case 92:return b.edgeModifier==value;case 66:return b.edgeColor==value;
    case 76:return b.edgeWidth==value;case 77:return b.edgeOverhang==value;case 78:return b.edgeJitter==value;case 67:return b.edgeSilhouetteColor==value;
    case 79:return b.edgeSilhouetteWidth==value;case 170:return b.edgeHaloGap==value;case 171:return b.edgeIsolines==value;case 290:return static_cast<int>(b.edgeDoHidePrecision)==value;
    case 174:return b.edgeStyleApply==value;case 93:return b.displaySettings==value;case 173:return b.displayShadowType==value;case 291:return static_cast<int>(b.internalOnly)==value;default:return false;}
}
inline double realModel(const DRW_VisualStyle& m,int code) noexcept {
    switch(code){case 40:return m.m_body.faceOpacity;case 41:return m.m_body.faceSpecular;case 42:return m.m_body.edgeCreaseAngle;
    case 43:return m.m_body.edgeOpacity;case 44:return m.m_body.displayBrightness;default:return 0.;}
}
template<class Context>bool matches(const Audit& a,const DRW_RawDxfObject& raw,const DRW_VisualStyle& model,const P& properties,const Context& context) {
    if(!a.complete()||model.m_bodyDecoded||model.handle!=raw.handle||model.parentHandle!=raw.parentHandle||model.m_body.faceMonoColor
        ||model.m_body.m_encoding.companionCount||model.m_body.m_encoding.colorCount||properties.invalidBoolean
        ||!properties.inVisualStyle||!properties.propertyTail||properties.descriptionOccurrences!=1u||properties.legacyDescriptionOccurrences||properties.tailGroups!=a.tailGroups)return false;
    DRW_TextCodec codec;codec.setVersion(context.codecVersion,true);codec.setCodePage(context.codecCodePage,true);
    codec.setDxfTextEncoding(context.textEncoding);codec.setDxfCodePagePolicy(context.codePagePolicy);
    const auto desc=codec.toUtf8(dx_attdef_source::text(raw.groups.at(a.description)),!context.binary);
    if(codec.lastDecodeObservation().issues||desc!=model.desc)return false;
    for(std::size_t i=0u;i<a.integers.size();++i){const auto& v=raw.groups.at(a.integers[i]);if(properties.integerOccurrences[i]!=1u||properties.integers[i]!=v.i_val()||!integerModel(model,v.code(),v.i_val()))return false;}
    for(std::size_t i=0u;i<a.reals.size();++i){const auto& v=raw.groups.at(a.reals[i]);const auto value=v.d_val(),modeled=realModel(model,v.code());
        if(properties.realOccurrences[i]!=1u||std::memcmp(&value,&properties.reals[i],sizeof(double))||std::memcmp(&value,&modeled,sizeof(double)))return false;}
    return true;
}
}
#endif
