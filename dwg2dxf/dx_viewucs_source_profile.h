// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_VIEWUCS_SOURCE_PROFILE_H
#define DX_VIEWUCS_SOURCE_PROFILE_H
#include "dx_compound_source_profile.h"

// Independent original VIEW/UCS scalar/scope/reference inventory. No payload
// decoder, model normalization, native acceptance or output certificate.
namespace dx_viewucs_source {
enum class Role {None,Self,Owner,Name,Reactor,ExtensionDictionary,AppId,Layer,Object,NamedUcs,BaseUcs,Background,LiveSection,VisualStyle,Sun,Opaque};
struct Span {std::size_t group;int code;Role role;std::uint32_t target;};
struct Audit {bool complete{false};std::size_t rejected{0};std::vector<Span> spans;};
inline Audit audit(const DRW_RawDxfObject& raw,bool view) {
    Audit a;const auto reject=[&](std::size_t i){a.rejected=i;a.spans.clear();return std::move(a);};
    if(raw.m_version<DRW::AC1021||raw.m_version>DRW::AC1032||!raw.handle||!raw.parentHandle||!dx_block_membership::equal(raw.name,view?"VIEW":"UCS"))return reject(0u);
    a.spans.reserve(raw.groups.size());std::array<bool,1072> seen{};std::array<std::string_view,DRW::kMaxDxfApplicationGroupNesting> apps{};
    std::size_t depth=0u,xdepth=0u,options=0u;unsigned reserved=0u,optionMask=0u;int stage=0,point=0,next=0,hasUcs=-1,ortho=-1;bool xdata=false,option=false;
    const auto integer=[](const DRW_Variant& v,int lo,int hi){return v.type()==DRW_Variant::INTEGER&&v.i_val()>=lo&&v.i_val()<=hi;};
    for(std::size_t i=0u;i<raw.groups.size();++i) {
        const auto& value=raw.groups[i];const int code=value.code();const auto text=dx_attdef_source::text(value);Role role=Role::None;std::uint64_t target=0u;
        if(!dx_compound_source::scalarType(value))return reject(i);
        if(code==102){if(xdata)return reject(i);if(text=="}"){if(!depth)return reject(i);--depth;}
            else if(text.size()>1u&&text.front()=='{'&&depth<apps.size()){
                if(depth&&(apps[0]=="{ACAD_REACTORS"||apps[0]=="{ACAD_XDICTIONARY"))return reject(i);
                if(!depth&&(text=="{ACAD_REACTORS"||text=="{ACAD_XDICTIONARY")){const auto bit=text=="{ACAD_REACTORS"?1u:2u;if(reserved&bit)return reject(i);reserved|=bit;}apps[depth++]=text;
            }else return reject(i);
        }else if(depth){if(dx_attdef_source::handleCode(code)){if(!dx_attdef_source::hexHandle(value,target))return reject(i);role=Role::Opaque;}
            if(apps[0]=="{ACAD_REACTORS"){if(depth!=1u||code!=330||target>UINT32_MAX)return reject(i);role=Role::Reactor;}
            if(apps[0]=="{ACAD_XDICTIONARY"){if(depth!=1u||code!=360||seen[360]||target>UINT32_MAX)return reject(i);seen[360]=true;role=Role::ExtensionDictionary;}
        }else if(code>=1000){if(stage!=2)return reject(i);
            if(point){if(code!=next)return reject(i);next+=10;if(code==point+20)point=0;}
            else if(code>=1010&&code<=1013){point=code;next=code+10;}
            else if(code>=1020&&code<=1033)return reject(i);
            if(code==1001){if(xdepth||!dx_attdef_source::name(value))return reject(i);xdata=true;role=Role::AppId;}
            else if(!xdata)return reject(i);
            else if(code==1002){if(text=="{"){if(++xdepth>DRW::kMaxDxfApplicationGroupNesting)return reject(i);}else if(text=="}"){if(!xdepth)return reject(i);--xdepth;}else return reject(i);}
            else if(code==1003){if(!dx_attdef_source::name(value))return reject(i);role=Role::Layer;}
            else if(code==1005){if(!dx_attdef_source::hexHandle(value,target)||target>UINT32_MAX)return reject(i);role=Role::Object;}
            else if(code==1004){if(!dx_attdef_source::hexBytes(text)||text.size()>254u)return reject(i);}
            else if(code!=1000&&!(code>=1010&&code<=1013)&&!(code>=1020&&code<=1023)&&!(code>=1030&&code<=1033)&&!(code>=1040&&code<=1042)&&code!=1070&&code!=1071)return reject(i);
        }else {
            if(xdata)return reject(i);
            if(code==100){if(stage==0&&text=="AcDbSymbolTableRecord")stage=1;else if(stage==1&&text==(view?"AcDbViewTableRecord":"AcDbUCSTableRecord"))stage=2;else return reject(i);}
            else if(!view&&stage==2&&(code==71||code==13||code==23||code==33)) {
                if(code==71){if((option&&optionMask!=7u)||!integer(value,1,6)||++options>DRW_UCS::kMaxOrthographicOptions)return reject(i);option=true;optionMask=0u;}
                else {const auto bit=code==13?1u:code==23?2u:4u;if(!option||(optionMask&bit))return reject(i);optionMask|=bit;}
            }else {
                if(code<0||code>=1072||seen[code])return reject(i);seen[code]=true;
                if(code==5||code==330){if(stage>1||!dx_attdef_source::hexHandle(value,target)||target>UINT32_MAX||target!=(code==5?raw.handle:raw.parentHandle))return reject(i);role=code==5?Role::Self:Role::Owner;}
                else if(code==2){if(!stage||!dx_attdef_source::name(value))return reject(i);role=Role::Name;}
                else if(code==70){if(!stage||!integer(value,0,32767)||(value.i_val()&~(view?0x71:0x70)))return reject(i);}
                else if(stage!=2)return reject(i);
                else if(!view){switch(code){
                    case 10:case 20:case 30:case 11:case 21:case 31:case 12:case 22:case 32:case 146:break;
                    case 79:if(!integer(value,0,6))return reject(i);ortho=value.i_val();break;
                    case 346:role=Role::BaseUcs;break;default:return reject(i);}}
                else {switch(code){
                    case 10:case 20:case 11:case 21:case 31:case 12:case 22:case 32:case 40:case 41:case 42:case 43:case 44:case 50:
                    case 110:case 120:case 130:case 111:case 121:case 131:case 112:case 122:case 132:case 146:break;
                    case 71:if(!integer(value,0,23)||(value.i_val()&~0x17))return reject(i);break;
                    case 281:if(!integer(value,0,6))return reject(i);break;
                    case 72:if(!integer(value,0,1))return reject(i);hasUcs=value.i_val();break;
                    case 73:if(!integer(value,0,1))return reject(i);break;
                    case 79:if(!integer(value,0,6))return reject(i);ortho=value.i_val();break;
                    case 345:role=Role::NamedUcs;break;case 346:role=Role::BaseUcs;break;case 332:role=Role::Background;break;
                    case 334:role=Role::LiveSection;break;case 348:role=Role::VisualStyle;break;case 361:role=Role::Sun;break;
                    default:return reject(i);}}
                if(role!=Role::None&&dx_attdef_source::handleCode(code)&&(!dx_attdef_source::hexHandle(value,target)||target>UINT32_MAX))return reject(i);
            }
        }
        a.spans.push_back({i,code,role,static_cast<std::uint32_t>(target)});
    }
    const auto end=raw.groups.size();if(stage!=2||depth||point||xdepth||(option&&optionMask!=7u)||!seen[5]||!seen[330]||!seen[2]||!seen[70])return reject(end);
    const auto triple=[&](int first){return seen[first]&&seen[first+10]&&seen[first+20];};
    if(!seen[10]||!seen[20]||!triple(11)||!triple(12)||(!view&&!seen[30]))return reject(end);
    if(view){if(!seen[40]||!seen[41])return reject(end);
        bool ucs=false;for(int code:{110,120,130,111,121,131,112,122,132,79,146,345,346})ucs=ucs||seen[code];
        if(ucs&&hasUcs!=1)return reject(end);if(hasUcs==1&&(!triple(110)||!triple(111)||!triple(112)||!seen[79]||!seen[146]))return reject(end);
    }
    if(seen[346]&&(ortho<=0))return reject(end);
    a.complete=true;a.rejected=end;return a;
}
}
#endif
