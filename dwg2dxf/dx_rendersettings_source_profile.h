// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_RENDERSETTINGS_SOURCE_PROFILE_H
#define DX_RENDERSETTINGS_SOURCE_PROFILE_H
#include "dx_visualstyle_source_profile.h"

// Independent DXF schemas. Do not call the production decoder, writer-profile
// selector or writer's positional code tables to establish original evidence.
namespace dx_rendersettings_source {
using Field=dx_visualstyle_source::Field;using Role=dx_visualstyle_source::Role;
using Span=dx_visualstyle_source::Span;using Reason=dx_visualstyle_source::Reason;
enum class Shape {Unavailable,Base,RapidRT,LegacyMentalRay,ModernMentalRay,ExtendedMentalRay};
struct Audit {
    Reason reason{Reason::Identity};Shape shape{Shape::Unavailable};DRW_RenderSettings::Kind kind{DRW_RenderSettings::Settings};
    std::size_t rejected{0};bool predefined{false},derivedPredefined{false};
    std::vector<std::size_t> base,derived,body;std::vector<Span> spans;
    bool complete()const noexcept{return reason==Reason::Matched;}
};
inline bool codes(const DRW_RawDxfObject& raw,const std::vector<std::size_t>& fields,std::initializer_list<int> expected) noexcept {
  if (fields.size() != expected.size())
    return false;
  auto at = fields.begin();
  for (int code : expected)
    if (raw.groups[*at++].code() != code)
      return false;
  return true;
}
inline Audit audit(const DRW_RawDxfObject& raw) {
    Audit a;const auto reject=[&](Reason why,std::size_t at){a.reason=why;a.rejected=at;a.spans.clear();return std::move(a);};
    if(raw.m_version<DRW::AC1021||raw.m_version>DRW::AC1032)return reject(Reason::Revision,0u);
    if(!raw.handle||!raw.parentHandle)return reject(Reason::Identity,0u);
    if(dx_block_membership::equal(raw.name,"RAPIDRTRENDERSETTINGS"))a.kind=DRW_RenderSettings::RapidRT;
    else if(dx_block_membership::equal(raw.name,"MENTALRAYRENDERSETTINGS"))a.kind=DRW_RenderSettings::MentalRay;
    else if(!dx_block_membership::equal(raw.name,"RENDERSETTINGS"))return reject(Reason::Identity,0u);
    a.spans.reserve(raw.groups.size());std::array<std::string_view,DRW::kMaxDxfApplicationGroupNesting> apps{};
    std::size_t depth=0u,xdepth=0u,extensions=0u;unsigned reserved=0u,self=0u,owner=0u;int stage=0,point=0,next=0;bool xdata=false;
    for(std::size_t i=0u;i<raw.groups.size();++i){const auto& v=raw.groups[i];const auto text=dx_attdef_source::text(v);const int code=v.code();
        Field field=Field::Primary;Role role=Role::None;std::uint64_t target=0u;
        if(!dx_compound_source::scalarType(v))return reject(Reason::Value,i);
        if(code==999)field=Field::Comment;
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
        }else if(code>=1000){field=Field::Xdata;if(stage<2||(a.kind!=DRW_RenderSettings::Settings&&stage!=3))return reject(Reason::Xdata,i);
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
            if(code==100){field=Field::Subclass;
                if(!stage&&text=="AcDbObject")stage=1;
                else if(stage<2&&text=="AcDbRenderSettings")stage=2;
                else if(stage==2&&((a.kind==DRW_RenderSettings::RapidRT&&text=="AcDbRapidRTRenderSettings")
                    ||(a.kind==DRW_RenderSettings::MentalRay&&text=="AcDbMentalRayRenderSettings")))stage=3;
                else return reject(Reason::Subclass,i);
            }else if(stage<2&&(code==5||code==330)){field=Field::Identity;
                if(!dx_attdef_source::hexHandle(v,target)||target>UINT32_MAX||target!=(code==5?raw.handle:raw.parentHandle)
                    ||(code==5?self++:owner++))return reject(Reason::Identity,i);
                role=code==5?Role::Self:Role::Owner;
            }else if(stage<2)return reject(Reason::Subclass,i);
            else{(stage==2?a.base:a.derived).push_back(i);a.body.push_back(i);}
        }
        a.spans.push_back({i,code,field,role,static_cast<std::uint32_t>(target)});
    }
    const auto end=raw.groups.size();if(depth||xdepth||point||self!=1u||owner!=1u||a.base.empty())return reject(Reason::MissingField,end);
    const bool modern=codes(raw,a.base,{90,1,290,290,290,290,1,1,90})||codes(raw,a.base,{90,1,290,290,290,290,1,1,90,290});
    const bool legacy=codes(raw,a.base,{90,1,290,90,290,290,1});
    if(!modern&&!legacy)return reject(Reason::UnknownField,end);
    const auto baseVersion=raw.groups[a.base[0]].i_val();if(baseVersion!=1&&baseVersion!=2)return reject(Reason::Revision,a.base[0]);
    if(modern)a.predefined=a.base.size()==10u;
    if(a.kind==DRW_RenderSettings::Settings){if(!modern||stage!=2||!a.derived.empty())return reject(Reason::MissingField,end);a.shape=Shape::Base;}
    else if(a.kind==DRW_RenderSettings::RapidRT){
        const bool primary=codes(raw,a.derived,{90,70,90,90,70,70,40,40});
        const bool extra=codes(raw,a.derived,{90,70,90,90,70,70,40,40,290});
        if(!modern||stage!=3||(!primary&&!extra)||(a.predefined&&extra))return reject(Reason::MissingField,end);
        if(raw.groups[a.derived[0]].i_val()!=2)return reject(Reason::Revision,a.derived[0]);
        a.derivedPredefined=extra;a.predefined=a.predefined||extra;a.shape=Shape::RapidRT;
    }else{
        const bool primary=codes(raw,a.derived,{90,90,90,70,40,40,40,40,40,40,70,290,290,90,90,90,290,90,290,40,90,90,90,90,290,90,290,290,290,40,40,40,70,70,40,70,70,290,1,90,70,90});
        const bool extra=codes(raw,a.derived,{90,90,90,70,40,40,40,40,40,40,70,290,290,90,90,90,290,90,290,40,90,90,90,90,290,90,290,290,290,40,40,40,70,70,40,70,70,290,1,90,70,90,290,40});
        if(stage!=3||(!modern&&!legacy)||(!primary&&!extra))return reject(Reason::MissingField,end);
        const auto version=raw.groups[a.derived[0]].i_val();
        if(legacy){if(baseVersion!=1||version!=1||!primary)return reject(Reason::Revision,a.derived[0]);a.shape=Shape::LegacyMentalRay;}
        else if(version==2&&extra)a.shape=Shape::ExtendedMentalRay;
        else if(version==1&&primary)a.shape=Shape::ModernMentalRay;
        else return reject(Reason::Revision,a.derived[0]);
    }
    if(dx_attdef_source::text(raw.groups[a.base[1]]).empty())return reject(Reason::Value,a.base[1]);
    a.reason=Reason::Matched;a.rejected=end;return a;
}
template<class Context>bool matches(const Audit& a,const DRW_RawDxfObject& raw,const DRW_RenderSettings& model,const Context& context) {
    if(!a.complete()||model.m_kind!=a.kind||model.handle!=raw.handle||model.parentHandle!=raw.parentHandle||model.hasDwgRawData()
        ||!model.dwgRawData().empty()||!model.m_bytes.empty())return false;
    DRW_TextCodec codec;codec.setVersion(context.codecVersion,true);codec.setCodePage(context.codecCodePage,true);
    codec.setDxfTextEncoding(context.textEncoding);codec.setDxfCodePagePolicy(context.codePagePolicy);
    std::size_t longs=0u,shorts=0u,bools=0u,reals=0u,strings=0u;
    for(auto index:a.body){const auto& v=raw.groups[index];
        switch(v.code()){
        case 90:if(longs>=model.m_longs.size()||model.m_longs[longs++]!=v.i_val())return false;break;
        case 70:if(shorts>=model.m_shorts.size()||model.m_shorts[shorts++]!=v.i_val())return false;break;
        case 290:if(bools>=model.m_bools.size()||static_cast<int>(model.m_bools[bools++])!=v.i_val())return false;break;
        case 40:{const auto value=v.d_val();if(reals>=model.m_doubles.size()||std::memcmp(&value,&model.m_doubles[reals++],sizeof(double)))return false;break;}
        case 1:{const auto value=codec.toUtf8(dx_attdef_source::text(v),!context.binary);if(codec.lastDecodeObservation().issues||strings>=model.m_strings.size()||model.m_strings[strings++]!=value)return false;break;}
        default:return false;}
    }
    const bool legacy=a.shape==Shape::LegacyMentalRay;
    return longs==model.m_longs.size()&&shorts==model.m_shorts.size()&&bools==model.m_bools.size()&&reals==model.m_doubles.size()&&strings==model.m_strings.size()
        &&model.m_classVersion==model.m_longs[0]&&model.m_name==model.m_strings[0]&&model.m_environmentImageFilename==model.m_strings[1]
        &&model.m_description==(legacy?UTF8STRING{}:model.m_strings[2])&&model.m_displayIndex==(legacy?0:model.m_longs[1])
        &&model.m_fogEnabled==model.m_bools[0]&&model.m_fogBackgroundEnabled==(!legacy&&model.m_bools[1])
        &&model.m_backfacesEnabled==model.m_bools[legacy?1u:2u]&&model.m_environmentImageEnabled==model.m_bools[legacy?2u:3u]
        &&model.m_hasPredefinedPresent==a.predefined&&model.m_hasPredefined==(a.predefined&&model.m_bools[4])
        &&!model.m_fogColorR&&!model.m_fogColorG&&!model.m_fogColorB&&!model.m_fogDensityNear&&!model.m_fogDensityFar
        &&!model.m_fogDistanceNear&&!model.m_fogDistanceFar&&!model.m_procedure&&!model.m_destination;
}
}
#endif
