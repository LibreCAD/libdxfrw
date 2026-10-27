// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_TABLE_SOURCE_PROFILE_H
#define DX_TABLE_SOURCE_PROFILE_H
#include "intern/dxfparserlimits.h"
#include "dx_compound_source_profile.h"
#include "dx_table_value_source_profile.h"

// Independent original ACAD_TABLE scalar/scope/reference inventory. No decoder,
// model projection, target lookup, TABLECONTENT schema or CAD semantic claim.
// Field roles follow Autodesk's TABLE reference and are kept distinct even
// where a tolerant parser happens to reuse one modeled field for two scopes.
// Keep the numerical DXF reference's 127-byte binary limit; the TABLE page's
// conflicting 256-character wording is not permission to widen parser input.
namespace dx_table_source {
using namespace dx_attdef_source;
inline Audit audit(const DRW_RawDxfObject& raw) {
    Audit out;
    const auto reject=[&](Reason why,std::size_t at){out.reason=why;out.rejectedGroup=at;out.spans.clear();return std::move(out);};
    if(raw.m_version<DRW::AC1021||raw.m_version>DRW::AC1032)return reject(Reason::Revision,0u);
    if(!dx_block_membership::equal(raw.name,"ACAD_TABLE")||!raw.handle)return reject(Reason::Identity,0u);
    out.spans.reserve(raw.groups.size());
    std::array<bool,1072> common{},block{},table{},cell{},properties{};
    std::array<std::string_view,DRW::kMaxDxfApplicationGroupNesting> applications{};
    std::size_t depth=0u,xdepth=0u,proxyBytes=0u,rowHeights=0u,columnWidths=0u,cells=0u;
    unsigned reserved=0u;int stage=0,rows=-1,columns=-1,type=0,attributes=-1,readAttributes=0,point=0,next=0;
    std::int64_t proxy=-1;bool xdata=false,inValue=false,overrides=false,attributeText=false,cellText=false,valueClosed=false;
    bool valueFirst=false,structured=false,cellValue=false;ValueProfile value;
    const auto integer=[](const DRW_Variant& v,int lo,int hi){return v.type()==DRW_Variant::INTEGER&&v.i_val()>=lo&&v.i_val()<=hi;};
    const auto pair=[](const auto& seen,int x,bool required){return (!seen[x]&&!seen[x+10]&&!seen[x+20]&&!required)||(seen[x]&&seen[x+10]);};
    const auto finishCell=[&](){return !cells||(!inValue&&!attributeText&&(attributes<0||attributes==readAttributes)
        &&((type==1&&(cellText||cellValue||cell[344]))||(type==2&&cell[340])));};
    for(std::size_t i=0u;i<raw.groups.size();++i) {
        const auto& v=raw.groups[i];const int code=v.code();const auto s=text(v);
        if(!dx_compound_source::scalarType(v))return reject(Reason::InvalidValue,i);
        Field field=Field::Common;Role role=Role::None;std::uint64_t handle=0u;
        if(code==102) {
            if(xdata||inValue)return reject(Reason::Application,i);field=Field::Application;
            if(s=="}"){if(!depth)return reject(Reason::Application,i);--depth;}
            else if(s.size()>1u&&s.front()=='{'&&depth<applications.size()) {
                if(depth&&(applications[0]=="{ACAD_REACTORS"||applications[0]=="{ACAD_XDICTIONARY"))return reject(Reason::Application,i);
                if(!depth&&(s=="{ACAD_REACTORS"||s=="{ACAD_XDICTIONARY")) {
                    const unsigned bit=s=="{ACAD_REACTORS"?1u:2u;if(reserved&bit)return reject(Reason::DuplicateField,i);reserved|=bit;
                }
                applications[depth++]=s;
            }else return reject(Reason::Application,i);
        }else if(depth) {
            field=Field::Application;
            if(handleCode(code)){if(!hexHandle(v,handle))return reject(Reason::InvalidValue,i);role=Role::OpaqueHandle;}
            if(applications[0]=="{ACAD_REACTORS") {
                if(depth!=1u||code!=330||handle>UINT32_MAX)return reject(Reason::Application,i);role=Role::Reactor;
            }else if(applications[0]=="{ACAD_XDICTIONARY") {
                if(depth!=1u||code!=360||common[360]||handle>UINT32_MAX)return reject(Reason::Application,i);common[360]=true;role=Role::ExtensionDictionary;
            }
        }else if(code>=1000) {
            field=Field::Xdata;if(stage!=3||inValue||!finishCell())return reject(Reason::Xdata,i);
            if(point){if(code!=next)return reject(Reason::IncompletePoint,i);next+=10;if(code==point+20)point=0;}
            else if(code>=1010&&code<=1013){point=code;next=code+10;}
            else if(code>=1020&&code<=1033)return reject(Reason::IncompletePoint,i);
            if(code==1001){if(xdepth||!name(v))return reject(Reason::Xdata,i);xdata=true;role=Role::AppIdName;}
            else if(!xdata)return reject(Reason::Xdata,i);
            else if(code==1002) {
                if(s=="{"){if(++xdepth>DRW::kMaxDxfApplicationGroupNesting)return reject(Reason::Xdata,i);}
                else if(s=="}"){if(!xdepth)return reject(Reason::Xdata,i);--xdepth;}else return reject(Reason::Xdata,i);
            }else if(code==1003){if(!name(v))return reject(Reason::Xdata,i);role=Role::XdataLayerName;}
            else if(code==1005){if(!hexHandle(v,handle)||handle>UINT32_MAX)return reject(Reason::Xdata,i);role=Role::XdataObjectId;}
            else if(code==1004){if(!hexBytes(s)||s.size()>254u)return reject(Reason::Xdata,i);}
            else if(code!=1000&&!(code>=1010&&code<=1013)&&!(code>=1020&&code<=1023)
                &&!(code>=1030&&code<=1033)&&!(code>=1040&&code<=1042)&&code!=1070&&code!=1071)return reject(Reason::UnknownField,i);
        }else {
            if(xdata)return reject(Reason::Xdata,i);
            if(code==100) {
                if(inValue)return reject(Reason::Subclass,i);
                field=Field::Subclass;
                if(stage==0&&s=="AcDbEntity")stage=1;
                else if(stage==1&&s=="AcDbBlockReference")stage=2;
                else if(stage==2&&s=="AcDbTable")stage=3;
                else return reject(Reason::Subclass,i);
            }else if((code==5||code==330)&&stage<=1) {
                field=Field::Identity;
                if(common[code]||!hexHandle(v,handle)||handle>UINT32_MAX||handle!=(code==5?raw.handle:raw.parentHandle))return reject(Reason::Identity,i);
                common[code]=true;role=code==5?Role::Self:Role::Owner;
            }else {
                if(!stage||code<0||code>=static_cast<int>(common.size()))return reject(Reason::UnknownField,i);
                bool body=false;
                if(stage==2) {
                    field=Field::Block;body=true;
                    if(block[code])return reject(Reason::DuplicateField,i);block[code]=true;
                    switch(code) {
                    case 2:if(!name(v))return reject(Reason::InvalidValue,i);role=Role::BlockName;break;
                    case 10:case 20:case 30:case 41:case 42:case 43:case 50:case 210:case 220:case 230:break;
                    case 66:if(!integer(v,0,1))return reject(Reason::InvalidValue,i);break;
                    default:body=false;break;
                    }
                }else if(stage==3) {
                    body=true;field=cells?Field::TableCell:Field::Table;
                    if(inValue) {
                        field=Field::TableValue;
                        if(valueFirst){structured=code==93;valueFirst=false;}
                        if(structured) {
                            if(!value.take(v,role,handle))return reject(Reason::UnmodeledSecondary,i);
                            if(value.closed()){inValue=false;valueClosed=true;cellValue=true;}
                        }else if(type!=1)return reject(Reason::UnmodeledSecondary,i);
                        else if(code==304){if(s!="ACVALUE_END"||!cellText)return reject(Reason::UnmodeledSecondary,i);inValue=false;valueClosed=true;}
                        else if(code==1||code==2||code==302||code==303){if(type!=1)return reject(Reason::UnmodeledSecondary,i);cellText=true;}
                        else if(code==90) {
                            if(cell[90])return reject(Reason::DuplicateField,i);
                            // Legacy envelopes have only the string profile;
                            // a typed declaration requires the structured form.
                            if(!integer(v,4,4))return reject(Reason::UnmodeledSecondary,i);cell[90]=true;
                        }
                        else return reject(Reason::UnmodeledSecondary,i);
                    }else if(code==171) {
                        if(overrides||rows<0||columns<0||static_cast<std::uint64_t>(rows)*columns<=cells||!finishCell())return reject(Reason::InvalidValue,i);
                        if(!integer(v,1,2))return reject(Reason::UnmodeledSecondary,i);
                        ++cells;type=v.i_val();cell.fill(false);cell[171]=true;attributes=-1;readAttributes=0;attributeText=false;cellText=false;valueClosed=false;cellValue=false;
                    }else if(code==280&&!table[280]&&rows<0&&columns<0&&!overrides) {
                        field=Field::Table;if(!integer(v,0,65535))return reject(Reason::InvalidValue,i);table[280]=true;
                    }else if(code==70||code==40||code==41||code==280||code==281) {
                        field=Field::TableOverride;
                        if(inValue||!finishCell()||properties[code])return reject(Reason::DuplicateField,i);properties[code]=true;overrides=true;
                        if((code==280||code==281)&&!integer(v,0,1))return reject(Reason::InvalidValue,i);
                        if(code==70&&!integer(v,0,1))return reject(Reason::InvalidValue,i);
                    }else if(overrides&&(code==7||code==140||code==170||code==63||code==64||code==65||code==66||code==68||code==69
                        ||(code>=274&&code<=279)||(code>=283&&code<=289)||code==97||code==98||code==4))return reject(Reason::UnmodeledSecondary,i);
                    else if((code==91&&rows<0)||(code==92&&columns<0)) {
                        field=Field::Table;
                        if(cells||!integer(v,0,200000))return reject(Reason::InvalidValue,i);
                        (code==91?rows:columns)=v.i_val();table[code]=true;
                        if(rows>=0&&columns>=0&&static_cast<std::uint64_t>(rows)*columns>200000u)return reject(Reason::InvalidValue,i);
                    }else if(code==141||code==142) {
                        field=Field::Table;
                        if(cells||overrides||rows<0||columns<0)return reject(Reason::InvalidValue,i);
                        if((code==141?++rowHeights:++columnWidths)>static_cast<std::size_t>(code==141?rows:columns))return reject(Reason::DuplicateField,i);
                    }else if(code==342||code==343||code==11||code==21||code==31||code==90||code==93||code==94||code==95||code==96) {
                        field=Field::Table;
                        if(table[code]||overrides)return reject(Reason::DuplicateField,i);table[code]=true;
                        if(code==342)role=Role::TableStyle;if(code==343)role=Role::TableGeometryBlock;
                        if((code>=90&&code<=96)&&!integer(v,0,INT32_MAX))return reject(Reason::InvalidValue,i);
                    }else if(cells&&!overrides) {
                        if(code==2||code==303){if(type!=1||cell[1]||valueClosed)return reject(Reason::InvalidValue,i);cellText=true;}
                        else if(code==1||code==302){if(type!=1||cell[1]||cell[302]||valueClosed)return reject(Reason::DuplicateField,i);cellText=true;cell[code]=true;}
                        else if(code==301){if(s!="CELL_VALUE"||cell[301]||cellText)return reject(Reason::UnmodeledSecondary,i);
                            cell[301]=true;inValue=true;valueFirst=true;structured=false;value=ValueProfile{};value.block=type==2;field=Field::TableValue;}
                        else if(code==331||code==300) {
                            if(type!=2||attributes<0)return reject(Reason::UnmodeledSecondary,i);
                            if(code==331){if(attributeText||readAttributes>=attributes)return reject(Reason::InvalidValue,i);attributeText=true;++readAttributes;role=Role::TableCellAttributeDefinition;}
                            else {if(!attributeText)return reject(Reason::InvalidValue,i);attributeText=false;}
                        }else {
                            if(cell[code])return reject(Reason::DuplicateField,i);cell[code]=true;
                            switch(code) {
                            case 340:if(type!=2)return reject(Reason::InvalidValue,i);role=Role::TableCellBlock;break;
                            case 344:if(type!=1)return reject(Reason::InvalidValue,i);role=Role::TableCellField;break;
                            case 179:if(type!=2||!cell[340]||!integer(v,0,32767))return reject(Reason::InvalidValue,i);attributes=v.i_val();break;
                            case 7:if(!name(v))return reject(Reason::InvalidValue,i);role=Role::TextStyleName;break;
                            case 173:case 174:case 283:case 289:case 285:case 286:case 288:if(!integer(v,0,1))return reject(Reason::InvalidValue,i);break;
                            case 172:case 175:case 176:case 177:case 178:case 91:case 92:case 170:if(!integer(v,0,INT32_MAX))return reject(Reason::InvalidValue,i);break;
                            case 144:if(type!=2)return reject(Reason::InvalidValue,i);break;
                            case 145:case 140:case 63:case 64:case 69:case 65:case 66:case 68:case 279:case 275:case 276:case 278:break;
                            default:body=false;break;
                            }
                        }
                    }else body=false;
                }
                if(!body) {
                    field=Field::Common;
                    if(common[code]&&code!=310)return reject(Reason::DuplicateField,i);common[code]=true;
                    switch(code) {
                    case 8:if(!name(v))return reject(Reason::InvalidValue,i);role=Role::LayerName;break;
                    case 6:if(!name(v))return reject(Reason::InvalidValue,i);role=Role::LineTypeName;break;
                    case 410:if(!name(v))return reject(Reason::InvalidValue,i);role=Role::LayoutName;break;
                    case 347:role=Role::Material;break;case 348:role=Role::FullVisualStyle;break;case 390:role=Role::PlotStyle;break;case 360:role=Role::ExtensionDictionary;break;
                    case 48:case 62:case 420:case 430:case 440:break;
                    case 60:case 67:if(!integer(v,0,1))return reject(Reason::InvalidValue,i);break;
                    case 284:if(!integer(v,0,3))return reject(Reason::InvalidValue,i);break;
                    case 370:if(DRW_LW_Conv::lineWidth2dxfInt(DRW_LW_Conv::dxfInt2lineWidth(v.i_val()))!=v.i_val())return reject(Reason::InvalidValue,i);break;
                    case 92:case 160:
                        if(stage!=1||(code==92)!=(raw.m_version==DRW::AC1021)||proxy!=-1)return reject(Reason::ProxyGraphics,i);
                        proxy=code==92?v.i_val():v.i64_val();if(proxy<0||static_cast<std::uint64_t>(proxy)>DRW::kMaxDxfBinaryPayloadBytes)return reject(Reason::ProxyGraphics,i);break;
                    case 310:
                        if(stage!=1||proxy<0||!hexBytes(s)||s.size()>254u)return reject(Reason::ProxyGraphics,i);proxyBytes+=s.size()/2u;
                        if(proxyBytes>static_cast<std::uint64_t>(proxy))return reject(Reason::ProxyGraphics,i);break;
                    default:return reject(Reason::UnknownField,i);
                    }
                }
                if(role!=Role::None&&handleCode(code)&&(!hexHandle(v,handle)||handle>UINT32_MAX))return reject(Reason::InvalidValue,i);
            }
        }
        out.spans.push_back({i,code,Scope::Host,field,role,handle});
    }
    const auto end=raw.groups.size();
    if(depth)return reject(Reason::Application,end);if(xdepth||inValue)return reject(Reason::Xdata,end);if(point)return reject(Reason::IncompletePoint,end);
    if(stage!=3||!common[5]||!common[330]||!common[8]||!block[2]||!pair(block,10,true)||!pair(block,210,false)||!pair(table,11,false))return reject(Reason::Identity,end);
    if(proxy>=0&&proxyBytes!=static_cast<std::uint64_t>(proxy))return reject(Reason::ProxyGraphics,end);
    if(rows>=0||columns>=0||cells) {
        if(rows<0||columns<0||cells!=static_cast<std::uint64_t>(rows)*columns||!finishCell()
            ||rowHeights!=static_cast<std::size_t>(rows)||columnWidths!=static_cast<std::size_t>(columns))return reject(Reason::InvalidValue,end);
    }
    out.reason=Reason::Complete;out.rejectedGroup=end;return out;
}
}
#endif
