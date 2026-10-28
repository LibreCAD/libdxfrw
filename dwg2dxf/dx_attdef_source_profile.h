// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_ATTDEF_SOURCE_PROFILE_H
#define DX_ATTDEF_SOURCE_PROFILE_H

#include "drw_objects.h"
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <vector>

// Converter-private audit of original scalar tags, NOT another payload decoder
// or a DWG/renderer/target certificate. Modeled replay is a separate prerequisite.
// Refuse unmodeled fields rather than allowing the tolerant decoder to erase
// them. Names and opaque data remain owned by raw.groups at each exact index.
// Complete means every scalar is accounted for (including explicitly ignored
// computed/default values). OpaqueHandle is NOT a qualified target role; block,
// resource, output and independent semantic qualification remain separate.
namespace dx_attdef_source {
enum class Scope : std::uint8_t { Host, Nested, Ignored };
enum class Field : std::uint8_t {
    Identity, Subclass, Common, Text, Attribute, EmbeddedMarker, MText,
    ComputedIgnored, Application, Xdata, ReservedDefault, Block, Compound, IgnoredComment, Primary,
    Table, TableCell, TableValue, TableOverride
};
enum class Role : std::uint8_t {
    None, Self, Owner, LayerName, LineTypeName, TextStyleName, Material,
    FullVisualStyle, PlotStyle, Reactor, ExtensionDictionary, AppIdName,
    XdataLayerName, XdataObjectId, OpaqueHandle, BlockName, RedundantBlockName, XrefPath, LayoutName,
    TableStyle, TableGeometryBlock, TableCellBlock, TableCellField, TableCellAttributeDefinition,
    TableValueObject,ViewportFrozenLayer,ViewportClip,ViewportNamedUcs,ViewportBaseUcs,ViewportBackground,ViewportShade,ViewportVisualStyle,ViewportSun
};
enum class Form : std::uint8_t { Classic, Embedded101, EmbeddedLegacy100 };
enum class Reason : std::uint8_t {
    NotChecked, Complete, Revision, Identity, Subclass, UnknownField,
    DuplicateField, InvalidValue, IncompletePoint, Application, Xdata,
    ProxyGraphics, UnmodeledSecondary, AmbiguousVersion, RedundantName
};
struct Span {
    std::size_t groupIndex;
    int code;
    Scope scope;
    Field field;
    Role role;
    std::uint64_t handle;
};
struct Audit {
    Reason reason{Reason::NotChecked};
    std::size_t rejectedGroup{0};
    Form form{Form::Classic};
    std::vector<Span> spans;
    bool complete() const noexcept {return reason==Reason::Complete;}
};

inline bool handleCode(int code) noexcept {
    return code==5||code==105||(code>=320&&code<=369)
        ||(code>=390&&code<=399)||code==480||code==481||code==1005;
}
inline std::string_view text(const DRW_Variant& v) noexcept {
    return v.type()==DRW_Variant::STRING&&v.content.s?std::string_view(*v.content.s):std::string_view{};
}
inline bool name(const DRW_Variant& v) noexcept {
    const auto s=text(v);return !s.empty()&&s.find('\0')==std::string_view::npos;
}
inline bool hexHandle(const DRW_Variant& v,std::uint64_t& result) noexcept {
    const auto s=text(v);
    if(s.empty())return false;
    const auto p=std::from_chars(s.data(),s.data()+s.size(),result,16);
    return p.ec==std::errc{}&&p.ptr==s.data()+s.size();
}
inline bool hexBytes(std::string_view s) noexcept {
    if(s.size()%2u)return false;
    for(char c:s)if(!((c>='0'&&c<='9')||(c>='A'&&c<='F')||(c>='a'&&c<='f')))return false;
    return true;
}

inline Audit audit(const DRW_RawDxfObject& raw,bool definition=true,bool allowLayoutName=false) {
    Audit out;
    const auto reject=[&](Reason reason,std::size_t at) {
        out.reason=reason;out.rejectedGroup=at;out.spans.clear();return std::move(out);
    };
    if(raw.m_version<DRW::AC1021||raw.m_version>DRW::AC1032)return reject(Reason::Revision,0u);
    if(raw.name!=(definition?"ATTDEF":"ATTRIB")||!raw.handle)return reject(Reason::Identity,0u);
    out.spans.reserve(raw.groups.size()); // caller precharges the complete bound
    enum Stage { Header, Common, Text, Attribute, Embedded } stage=Header;
    bool entity=false,hostText=false,attribute=false,nested=false,xdata=false;
    std::array<std::array<bool,1072>,5> seen{};
    std::array<unsigned,2> reserved{};
    std::array<std::int64_t,2> proxy{{-1,-1}};
    std::array<std::size_t,2> proxyBytes{};
    std::array<std::string_view,DRW::kMaxDxfApplicationGroupNesting> applications{};
    std::size_t depth=0u,xdepth=0u;
    std::size_t attributeMarker=raw.groups.size(),firstAttribute280=raw.groups.size();
    bool dualAttribute280=false,qualifiedSingleLock280=false,verticalAfterLock280=false;
    std::array<std::size_t,3> textAlignment{{raw.groups.size(),raw.groups.size(),raw.groups.size()}};
    bool redundantHostAlignment=false;
    int pointCode=0,pointNext=0;
    int attributeType=1;
    bool nestedText=false;
    const auto integer=[](const DRW_Variant& v,int low,int high) {
        return v.type()==DRW_Variant::INTEGER&&v.i_val()>=low&&v.i_val()<=high;
    };
    const auto pointComplete=[](const auto& slots,int x,bool required) {
        return (!slots[x]&&!slots[x+10]&&!slots[x+20]&&!required)
            ||(slots[x]&&slots[x+10]); // Z may use the documented default.
    };
    for(std::size_t i=0;i<raw.groups.size();++i) {
        const auto& value=raw.groups[i];const int code=value.code();const auto string=text(value);
        Scope scope=nested?Scope::Nested:Scope::Host;
        Field field=Field::Common;Role role=Role::None;std::uint64_t handle=0u;
        const auto slot=scope==Scope::Host?0u:1u;
        if(value.type()==DRW_Variant::DOUBLE&&!std::isfinite(value.d_val()))return reject(Reason::InvalidValue,i);
        if(code==102) {
            if(xdata)return reject(Reason::Xdata,i);
            field=Field::Application;
            if(string=="}") {
                if(!depth)return reject(Reason::Application,i);
                --depth;
            } else if(string.size()>1u&&string.front()=='{'&&depth<applications.size()) {
                if(depth&&(applications[0]=="{ACAD_REACTORS"||applications[0]=="{ACAD_XDICTIONARY"))
                    return reject(Reason::Application,i);
                if(!depth&&(string=="{ACAD_REACTORS"||string=="{ACAD_XDICTIONARY")) {
                    const unsigned bit=string=="{ACAD_REACTORS"?1u:2u;
                    if(reserved[slot]&bit)return reject(Reason::DuplicateField,i);
                    reserved[slot]|=bit;
                }
                applications[depth++]=string;
            } else return reject(Reason::Application,i);
        } else if(depth) {
            field=Field::Application;
            if(handleCode(code)) {
                if(!hexHandle(value,handle))return reject(Reason::InvalidValue,i);
                role=Role::OpaqueHandle;
            }
            if(applications[0]=="{ACAD_REACTORS") {
                if(depth!=1u||code!=330||handle>UINT32_MAX)return reject(Reason::Application,i);
                role=Role::Reactor;
            } else if(applications[0]=="{ACAD_XDICTIONARY") {
                if(depth!=1u||code!=360||seen[slot][360]||handle>UINT32_MAX)return reject(Reason::Application,i);
                seen[slot][360]=true;role=Role::ExtensionDictionary;
            }
        } else if(code>=1000) {
            scope=Scope::Host;field=Field::Xdata; // production never routes XDATA to embedded MTEXT
            if(!attribute)return reject(Reason::Xdata,i);
            if(pointCode) {
                if(code!=pointNext)return reject(Reason::IncompletePoint,i);
                pointNext+=10;
                if(code==pointCode+20)pointCode=0;
            } else if(code>=1010&&code<=1013) {pointCode=code;pointNext=code+10;}
            else if((code>=1020&&code<=1033))return reject(Reason::IncompletePoint,i);
            if(code==1001) {
                if(xdepth||!name(value))return reject(Reason::Xdata,i);
                xdata=true;role=Role::AppIdName;
            } else if(!xdata)return reject(Reason::Xdata,i);
            else if(code==1002) {
                if(string=="{"){if(++xdepth>DRW::kMaxDxfApplicationGroupNesting)return reject(Reason::Xdata,i);}
                else if(string=="}"){if(!xdepth)return reject(Reason::Xdata,i);--xdepth;}
                else return reject(Reason::Xdata,i);
            } else if(code==1003) {if(!name(value))return reject(Reason::Xdata,i);role=Role::XdataLayerName;}
            else if(code==1005) {if(!hexHandle(value,handle)||handle>UINT32_MAX)return reject(Reason::Xdata,i);role=Role::XdataObjectId;}
            else if(code==1004) {if(!hexBytes(string)||string.size()>254u)return reject(Reason::Xdata,i);}
            else if(code!=1000&&!(code>=1010&&code<=1013)&&!(code>=1020&&code<=1023)
                &&!(code>=1030&&code<=1033)&&!(code>=1040&&code<=1042)&&code!=1070&&code!=1071)
                return reject(Reason::UnknownField,i);
        } else {
            if(xdata)return reject(Reason::Xdata,i);
            if(code==100||code==101) {
                field=Field::Subclass;
                if(string=="AcDbXrecord")return reject(Reason::UnmodeledSecondary,i);
                if(string=="Embedded Object") {
                    // A later embedded body must not inherit the classic
                    // dual-280 qualification selected before this marker.
                    if(dualAttribute280)return reject(Reason::AmbiguousVersion,i);
                    if(nested||!hostText||(stage!=Text&&stage!=Attribute))return reject(Reason::Subclass,i);
                    // Earlier versions remain accepted by plain consumers, but
                    // compatibility parsing is not a documented multiline profile.
                    if(raw.m_version!=DRW::AC1032)return reject(Reason::Revision,i);
                    nested=true;scope=Scope::Nested;stage=Embedded;field=Field::EmbeddedMarker;
                    out.form=code==101?Form::Embedded101:Form::EmbeddedLegacy100;
                } else if(code!=100)return reject(Reason::Subclass,i);
                else if(string=="AcDbEntity"&&!entity&&stage==Header) {entity=true;stage=Common;scope=Scope::Host;}
                else if(string=="AcDbText"&&entity&&!hostText&&stage==Common) {hostText=true;stage=Text;scope=Scope::Host;}
                else if(string==(definition?"AcDbAttributeDefinition":"AcDbAttribute")&&hostText&&!attribute&&(stage==Text||stage==Embedded)) {
                    attribute=true;stage=Attribute;scope=Scope::Host;attributeMarker=i;
                } else return reject(Reason::Subclass,i);
            } else if(stage==Header) {
                field=Field::Identity;scope=Scope::Host;
                if((code!=5&&code!=330)||seen[4][code]||!hexHandle(value,handle)||handle>UINT32_MAX)
                    return reject(Reason::Identity,i);
                seen[4][code]=true;role=code==5?Role::Self:Role::Owner;
                if(handle!=(code==5?raw.handle:raw.parentHandle))return reject(Reason::Identity,i);
            } else {
                auto& slots=seen[stage==Common?0u:stage==Text?2u:stage==Attribute?3u:1u];
                if(code<0||code>=static_cast<int>(slots.size()))return reject(Reason::UnknownField,i);
                if(slots[code]) {
                    // The witnessed classic attribute grammar has a version-zero
                    // prefix immediately after the subclass marker and a
                    // separate lock flag after the required attribute fields.
                    if(code!=280||stage!=Attribute||nested
                        ||(raw.m_version!=DRW::AC1024&&raw.m_version!=DRW::AC1027&&raw.m_version!=DRW::AC1032)
                        ||dualAttribute280||firstAttribute280!=attributeMarker+1u
                        ||firstAttribute280>=out.spans.size()||!integer(raw.groups[firstAttribute280],0,0)
                        ||!seen[3][2]||(definition&&!seen[3][3])||!seen[3][70]||!integer(value,0,1))
                        return reject(code==280?Reason::AmbiguousVersion:Reason::DuplicateField,i);
                    out.spans[firstAttribute280].field=Field::ReservedDefault;
                    dualAttribute280=true;
                } else if(stage==Attribute&&code==280) {
                    firstAttribute280=i;
                    // A single flag after the required attribute fields is
                    // the writer's lock-position role. A leading version
                    // marker remains ambiguous in the embedded profile.
                    qualifiedSingleLock280=seen[3][2]&&seen[3][70]&&seen[3][73]
                        &&(!definition||seen[3][3])&&!seen[3][71]&&!seen[3][72]
                        &&!seen[3][11]&&!seen[3][21]&&!seen[3][31];
                }
                slots[code]=true;
                if(stage==Attribute) {
                    scope=Scope::Host;field=Field::Attribute;
                    switch(code) {
                    case 2:if(!name(value)||string.find_first_of(" \t\r\n")!=std::string_view::npos)return reject(Reason::InvalidValue,i);break;
                    case 3:if(!definition)return reject(Reason::UnknownField,i);break;
                    case 70:if(!integer(value,0,15))return reject(Reason::InvalidValue,i);break;
                    case 73:if(!integer(value,0,65535))return reject(Reason::InvalidValue,i);break;
                    case 74:
                        if(!integer(value,0,3))return reject(Reason::InvalidValue,i);
                        if(seen[3][280])verticalAfterLock280=true;
                        break;
                    case 280:if(!integer(value,0,1))return reject(Reason::InvalidValue,i);break;
                    case 71:
                        if(raw.m_version!=DRW::AC1032)return reject(Reason::Revision,i);
                        if(nested||(!integer(value,1,1)&&!integer(value,definition?4:2,definition?4:2)))return reject(Reason::UnknownField,i);
                        attributeType=value.i_val();
                        break;
                    case 72:
                        if(raw.m_version!=DRW::AC1032)return reject(Reason::Revision,i);
                        if(nested||!integer(value,0,0))return reject(Reason::UnknownField,i);
                        field=Field::ReservedDefault;break;
                    case 11:case 21:case 31:
                        if(nested)return reject(Reason::DuplicateField,i);
                        if(seen[2][code]) {
                            const auto index=static_cast<std::size_t>((code-11)/10);
                            if(raw.m_version!=DRW::AC1032 || textAlignment[index]>=raw.groups.size()
                                || value.type()!=DRW_Variant::DOUBLE
                                || raw.groups[textAlignment[index]].type()!=DRW_Variant::DOUBLE)
                                return reject(Reason::DuplicateField,i);
                            const double first=raw.groups[textAlignment[index]].d_val();
                            const double second=value.d_val();
                            if(!std::isfinite(first)||!std::isfinite(second)||first!=second
                                ||(first==0.0&&std::signbit(first)!=std::signbit(second)))
                                return reject(Reason::DuplicateField,i);
                            redundantHostAlignment=true;
                        }
                        break;
                    default:return reject(Reason::UnknownField,i);
                    }
                } else if(stage==Text||stage==Embedded) {
                    scope=stage==Text?Scope::Host:Scope::Nested;field=stage==Text?Field::Text:Field::MText;
                    bool common=false;
                    switch(code) {
                    case 1:if(stage==Embedded)nestedText=true;break;
                    case 3:
                        if(stage!=Embedded||nestedText)return reject(Reason::UnknownField,i);
                        slots[code]=false;break; // ordered continuation chunks, no overwrite
                    case 7:if(!name(value))return reject(Reason::InvalidValue,i);role=Role::TextStyleName;break;
                    case 11:case 21:case 31:
                        if(stage==Text)textAlignment[static_cast<std::size_t>((code-11)/10)]=i;
                        break;
                    case 10:case 20:case 30:case 210:case 220:case 230:case 39:case 50:case 51:break;
                    case 40:if(value.type()!=DRW_Variant::DOUBLE||value.d_val()<=0.0)return reject(Reason::InvalidValue,i);break;
                    case 41:if(value.type()!=DRW_Variant::DOUBLE||value.d_val()<0.0)return reject(Reason::InvalidValue,i);break;
                    case 71:if(!integer(value,stage==Text?0:1,stage==Text?6:9))return reject(Reason::InvalidValue,i);break;
                    case 72:
                        if(stage==Text?!integer(value,0,5):(!integer(value,1,1)&&!integer(value,3,3)&&!integer(value,5,5)))
                            return reject(Reason::InvalidValue,i);
                        break;
                    case 42:case 43:
                        if(stage!=Embedded)return reject(Reason::UnknownField,i);
                        field=Field::ComputedIgnored;break; // explicitly read-only in DXF reference
                    case 73:
                        if(stage!=Embedded||!integer(value,1,2))return reject(Reason::UnknownField,i);
                        break;
                    case 44:case 45:case 46:case 63:case 90:case 421:case 431:case 441:
                        if(stage!=Embedded)return reject(Reason::UnknownField,i);
                        break;
                    default:common=stage==Embedded; if(!common)return reject(Reason::UnknownField,i);break;
                    }
                    if(!common) {out.spans.push_back({i,code,scope,field,role,handle});continue;}
                    field=Field::Common;
                }
                if(stage==Common||field==Field::Common) {
                    switch(code) {
                    case 8:if(!name(value))return reject(Reason::InvalidValue,i);role=Role::LayerName;break;
                    case 6:if(!name(value))return reject(Reason::InvalidValue,i);role=Role::LineTypeName;break;
                    case 410:if(!allowLayoutName||!name(value))return reject(Reason::UnknownField,i);role=Role::LayoutName;break;
                    case 347:role=Role::Material;break;
                    case 348:role=Role::FullVisualStyle;break;
                    case 390:role=Role::PlotStyle;break;
                    case 360:role=Role::ExtensionDictionary;break;
                    case 370:
                        if(value.type()!=DRW_Variant::INTEGER
                            ||DRW_LW_Conv::lineWidth2dxfInt(DRW_LW_Conv::dxfInt2lineWidth(value.i_val()))!=value.i_val())
                            return reject(Reason::InvalidValue,i);
                        break;
                    case 48:case 62:case 420:case 430:case 440:break;
                    case 60:case 67:if(!integer(value,0,1))return reject(Reason::InvalidValue,i);break;
                    case 284:if(!integer(value,0,3))return reject(Reason::InvalidValue,i);break;
                    case 92:case 160:
                        if((code==92)!=(raw.m_version==DRW::AC1021)||proxy[slot]!=-1)
                            return reject(Reason::ProxyGraphics,i);
                        proxy[slot]=code==92?value.i_val():value.i64_val();
                        if(proxy[slot]<0||static_cast<std::uint64_t>(proxy[slot])>DRW::kMaxDxfBinaryPayloadBytes)
                            return reject(Reason::ProxyGraphics,i);
                        break;
                    case 310:
                        slots[code]=false;
                        if(proxy[slot]<0||!hexBytes(string)||string.size()>254u)return reject(Reason::ProxyGraphics,i);
                        proxyBytes[slot]+=string.size()/2u;
                        if(proxyBytes[slot]>static_cast<std::uint64_t>(proxy[slot]))return reject(Reason::ProxyGraphics,i);
                        break;
                    default:return reject(Reason::UnknownField,i);
                    }
                    if(handleCode(code)&&(!hexHandle(value,handle)||handle>UINT32_MAX))return reject(Reason::InvalidValue,i);
                }
            }
        }
        out.spans.push_back({i,code,scope,field,role,handle});
    }
    const auto end=raw.groups.size();
    if(depth)return reject(Reason::Application,end);
    if(xdepth)return reject(Reason::Xdata,end);
    if(pointCode)return reject(Reason::IncompletePoint,end);
    if(!entity||!hostText||!attribute||!seen[4][5]||!seen[4][330])return reject(Reason::Identity,end);
    if(attributeType!=1&&!nested)return reject(Reason::InvalidValue,end);
    if(redundantHostAlignment) {
        if(!nested||out.form!=Form::Embedded101||attributeType!=(definition?4:2)
            ||(seen[3][280]&&(!qualifiedSingleLock280||dualAttribute280||verticalAfterLock280)))
            return reject(Reason::AmbiguousVersion,end);
        for(const int code:{11,21,31})
            if(seen[2][code]!=seen[3][code])return reject(Reason::IncompletePoint,end);
    }
    if(!seen[0][8]||!seen[2][1]||!seen[2][40]||!seen[3][2]||(definition&&!seen[3][3]))return reject(Reason::UnknownField,end);
    if(!pointComplete(seen[2],10,true)||!pointComplete(seen[2],11,false)
        ||!pointComplete(seen[2],210,false)||!pointComplete(seen[3],11,false))return reject(Reason::IncompletePoint,end);
    if(nested&&(!nestedText||!seen[1][40]||!pointComplete(seen[1],10,true)
        ||!pointComplete(seen[1],11,false)||!pointComplete(seen[1],210,false)))return reject(Reason::IncompletePoint,end);
    for(std::size_t s=0;s<2u;++s)
        if(proxy[s]>=0&&proxyBytes[s]!=static_cast<std::uint64_t>(proxy[s]))return reject(Reason::ProxyGraphics,end);
    out.reason=Reason::Complete;out.rejectedGroup=end;return out;
}
} // namespace dx_attdef_source
#endif
