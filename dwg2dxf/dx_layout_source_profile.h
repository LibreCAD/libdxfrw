// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_LAYOUT_SOURCE_PROFILE_H
#define DX_LAYOUT_SOURCE_PROFILE_H
#include "dx_compound_source_profile.h"

// Independent scalar/subclass/role inventory, NOT a payload decoder, native
// acceptance result or target/output certificate. Indices are raw body groups
// (reader-ignored comments excluded), not reconstructed canonical writer fields.
namespace dx_layout_source {
enum class Scope : std::uint8_t {Header,Plot,Layout,Application,Xdata};
enum class Role : std::uint8_t {None,Self,Owner,Reactor,ExtensionDictionary,BlockRecord,Viewport,NamedUcs,BaseUcs,ShadePlot,
    LayoutName,PlotViewName,AppIdName,XdataLayerName,XdataObjectId,OpaqueHandle};
enum class Reason : std::uint8_t {NotChecked,Complete,Revision,Identity,Subclass,UnknownField,DuplicateField,InvalidValue,Application,Xdata,IncompletePoint};
struct Span {std::size_t groupIndex;int code;Scope scope;Role role;std::uint64_t handle;};
struct Audit {
    Reason reason{Reason::NotChecked};std::size_t rejectedGroup{0};std::vector<Span> spans;
    bool complete() const noexcept {return reason==Reason::Complete;}
};
inline Audit audit(const DRW_RawDxfObject& raw) {
    Audit out;const auto reject=[&](Reason reason,std::size_t i){out.reason=reason;out.rejectedGroup=i;out.spans.clear();return std::move(out);};
    if(raw.m_version<DRW::AC1021||raw.m_version>DRW::AC1032)return reject(Reason::Revision,0u);
    if(!dx_block_membership::equal(raw.name,"LAYOUT")||!raw.handle||!raw.parentHandle)return reject(Reason::Identity,0u);
    out.spans.reserve(raw.groups.size());
    std::array<std::array<bool,1072>,3> seen{};Scope stage=Scope::Header;
    std::array<std::string_view,DRW::kMaxDxfApplicationGroupNesting> applications{};
    std::size_t depth=0u,xdepth=0u;bool xdata=false,shadeSeen=false;std::uint64_t shade=0u;
    unsigned reserved=0u;int point=0,next=0;
    const auto integer=[](const DRW_Variant& v,int lo,int hi){return v.type()==DRW_Variant::INTEGER&&v.i_val()>=lo&&v.i_val()<=hi;};
    for(std::size_t i=0;i<raw.groups.size();++i) {
        const auto& v=raw.groups[i];const int code=v.code();const auto text=dx_attdef_source::text(v);
        if(!dx_compound_source::scalarType(v))return reject(Reason::InvalidValue,i);
        Scope scope=stage;Role role=Role::None;std::uint64_t handle=0u;
        if(code==102) {
            scope=Scope::Application;if(xdata)return reject(Reason::Xdata,i);
            if(text=="}"){if(!depth)return reject(Reason::Application,i);--depth;}
            else if(text.size()>1u&&text.front()=='{'&&depth<applications.size()) {
                if(depth&&(applications[0]=="{ACAD_REACTORS"||applications[0]=="{ACAD_XDICTIONARY"))return reject(Reason::Application,i);
                if(!depth&&(text=="{ACAD_REACTORS"||text=="{ACAD_XDICTIONARY")) {
                    const unsigned bit=text=="{ACAD_REACTORS"?1u:2u;
                    if (reserved & bit)
                      return reject(Reason::DuplicateField, i);
                    reserved |= bit;
                }
                applications[depth++]=text;
            }else return reject(Reason::Application,i);
        }else if(depth) {
            scope=Scope::Application;
            if(dx_attdef_source::handleCode(code)) {
              if (!dx_attdef_source::hexHandle(v, handle))
                return reject(Reason::InvalidValue, i);
              role = Role::OpaqueHandle;
            }
            if(applications[0]=="{ACAD_REACTORS") {
              if (depth != 1u || code != 330 || handle > UINT32_MAX)
                return reject(Reason::Application, i);
              role = Role::Reactor;
            }else if(applications[0]=="{ACAD_XDICTIONARY") {
                if(depth!=1u||code!=360||seen[0][360]||handle>UINT32_MAX)return reject(Reason::Application,i);
                seen[0][360]=true;role=Role::ExtensionDictionary;
            }
        }else if(code>=1000) {
            scope=Scope::Xdata;if(stage!=Scope::Layout)return reject(Reason::Xdata,i);
            if(point){if(code!=next)return reject(Reason::IncompletePoint,i);next+=10;if(code==point+20)point=0;}
            else if(code>=1010&&code<=1013){point=code;next=code+10;}
            else if(code>=1020&&code<=1033)return reject(Reason::IncompletePoint,i);
            if(code==1001){if(xdepth||!dx_attdef_source::name(v))return reject(Reason::Xdata,i);xdata=true;role=Role::AppIdName;}
            else if(!xdata)return reject(Reason::Xdata,i);
            else if(code==1002) {
                if(text=="{"){if(++xdepth>DRW::kMaxDxfApplicationGroupNesting)return reject(Reason::Xdata,i);}
                else if(text=="}"){if(!xdepth)return reject(Reason::Xdata,i);--xdepth;}
                else return reject(Reason::Xdata,i);
            }else if(code==1003){if(!dx_attdef_source::name(v))return reject(Reason::Xdata,i);role=Role::XdataLayerName;}
            else if(code==1005){if(!dx_attdef_source::hexHandle(v,handle)||handle>UINT32_MAX)return reject(Reason::Xdata,i);role=Role::XdataObjectId;}
            else if(code==1004){if(!dx_attdef_source::hexBytes(text)||text.size()>254u)return reject(Reason::Xdata,i);}
            else if(code!=1000&&!(code>=1010&&code<=1013)&&!(code>=1020&&code<=1023)&&!(code>=1030&&code<=1033)
                &&!(code>=1040&&code<=1042)&&code!=1070&&code!=1071)return reject(Reason::UnknownField,i);
        }else {
            if(xdata)return reject(Reason::Xdata,i);
            if(code==100) {
                if(stage==Scope::Header&&text=="AcDbPlotSettings")stage=Scope::Plot;
                else if(stage==Scope::Plot&&text=="AcDbLayout")stage=Scope::Layout;
                else return reject(Reason::Subclass,i);
                scope=stage;
            }else {
                if(code<0||code>=1072)return reject(Reason::UnknownField,i);
                auto& fields=seen[static_cast<unsigned>(stage)];
                if (fields[code])
                  return reject(Reason::DuplicateField, i);
                fields[code] = true;
                if(stage==Scope::Header) {
                    if(code!=5&&code!=330)return reject(Reason::UnknownField,i);
                    if(!dx_attdef_source::hexHandle(v,handle)||handle>UINT32_MAX||handle!=(code==5?raw.handle:raw.parentHandle))return reject(Reason::Identity,i);
                    role=code==5?Role::Self:Role::Owner;
                }else if(stage==Scope::Plot) {
                    switch(code) {
                    case 1:case 2:case 4:case 7:break;
                    case 6:role=Role::PlotViewName;break;
                    case 40:case 41:case 42:case 43:case 44:case 45:case 46:case 47:case 48:case 49:
                    case 140:case 141:case 142:case 143:case 147:case 148:case 149:break;
                    case 70:if(!integer(v,0,32767))return reject(Reason::InvalidValue,i);break;
                    case 72:if(!integer(v,0,2))return reject(Reason::InvalidValue,i);break;
                    case 73:if(!integer(v,0,3))return reject(Reason::InvalidValue,i);break;
                    // Modern ObjectARX includes VisualStyle=4/RenderPreset=5;
                    // the older DXF prose table lists only modes 0..3.
                    case 74:case 76:case 77:if(!integer(v,0,5))return reject(Reason::InvalidValue,i);break;
                    case 75:if(!integer(v,0,32))return reject(Reason::InvalidValue,i);break;
                    case 78:if(!integer(v,0,32767))return reject(Reason::InvalidValue,i);break;
                    case 333:role=Role::ShadePlot;break;
                    default:return reject(Reason::UnknownField,i);
                    }
                }else {
                    switch(code) {
                    case 1:if(!dx_attdef_source::name(v))return reject(Reason::InvalidValue,i);role=Role::LayoutName;break;
                    case 70:if(!integer(v,0,3))return reject(Reason::InvalidValue,i);break;
                    case 71:if(!integer(v,0,INT32_MAX))return reject(Reason::InvalidValue,i);break;
                    case 76:if(!integer(v,0,6))return reject(Reason::InvalidValue,i);break;
                    case 10:case 20:case 11:case 21:case 12:case 22:case 32:case 13:case 23:case 33:
                    case 14:case 24:case 34:case 15:case 25:case 35:case 16:case 26:case 36:case 17:case 27:case 37:case 146:break;
                    case 330:role=Role::BlockRecord;break;case 331:role=Role::Viewport;break;
                    case 345:role=Role::NamedUcs;break;case 346:role=Role::BaseUcs;break;case 333:role=Role::ShadePlot;break;
                    default:return reject(Reason::UnknownField,i);
                    }
                }
                if(role==Role::ShadePlot||role==Role::BlockRecord||role==Role::Viewport||role==Role::NamedUcs||role==Role::BaseUcs) {
                    if(!dx_attdef_source::hexHandle(v,handle)||handle>UINT32_MAX||(role==Role::BlockRecord&&!handle))return reject(Reason::InvalidValue,i);
                    if(role==Role::ShadePlot){if(shadeSeen&&shade!=handle)return reject(Reason::InvalidValue,i);shadeSeen=true;shade=handle;}
                }
            }
        }
        out.spans.push_back({i,code,scope,role,handle});
    }
    const auto end=raw.groups.size();
    if (depth)
      return reject(Reason::Application, end);
    if (point || xdepth)
      return reject(Reason::Xdata, end);
    if(stage!=Scope::Layout||!seen[0][5]||!seen[0][330]||!seen[2][1]||!seen[2][330])return reject(Reason::Identity,end);
    out.reason=Reason::Complete;out.rejectedGroup=end;return out;
}
}
#endif
