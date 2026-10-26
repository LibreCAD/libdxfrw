// SPDX-License-Identifier: GPL-2.0-or-later
bool compoundProfileMatches(const dx_data& data,std::size_t dispatch) {
    using A=DxIfaceTableSourceTestAccess;
    if(A::compoundSourceMatched(data,dispatch))return true;
    std::cerr<<"compound profile dispatch="<<dispatch<<" modeled="<<A::compoundMatched(data,dispatch)<<" capture="<<A::compoundFailed(data);
    if(const auto* body=A::compoundBody(data,dispatch))for(std::size_t i=0;i<body->profiles.size();++i)
        std::cerr<<" member "<<i<<" reason="<<static_cast<int>(body->profiles[i].reason)<<" at="<<body->profiles[i].rejectedGroup;
    std::cerr<<'\n';return false;
}
bool compoundProfileInventory(const dx_data& data,std::size_t dispatch,std::size_t& spans,std::size_t& comments) {
    using A=DxIfaceTableSourceTestAccess;using namespace dx_attdef_source;
    const auto* body=A::compoundBody(data,dispatch);if(!body)return false;
    const auto& audit=*body->membership;const auto range=audit.ranges[dispatch];
    if(body->profiles.size()!=range.next-range.first)return false;
    for(std::size_t r=0;r<body->profiles.size();++r) {
        const auto& profile=body->profiles[r];if(!profile.complete())continue;
        const auto& physical=audit.children->records[range.first+r];const auto& member=audit.members[range.first+r];
        if(profile.spans.size()!=physical.groups.size())return false;
        unsigned depth=0u;std::string application;bool face=false;
        for(std::size_t i=0;i<profile.spans.size();++i) {
            const auto& p=profile.spans[i];const auto& v=physical.groups[i];
            if(p.groupIndex!=i||p.code!=v.code())return false;
            if(physical.spans[i].ignoredComment) {
                if(p.scope!=Scope::Ignored||p.field!=Field::IgnoredComment||p.role!=Role::None)return false;++comments;
            }else if(p.scope==Scope::Ignored||p.field==Field::IgnoredComment)return false;
            if(physical.spans[i].ignoredComment){++spans;continue;}
            if(v.code()==102) {
                const std::string token=v.c_str();
                if(token=="}"){if(!depth)return false;--depth;}
                else {if(!depth)application=token;++depth;}
            }else {
                Role expected=Role::None;
                if(depth) {
                    if(dx_attdef_source::handleCode(v.code()))expected=application=="{ACAD_REACTORS"?Role::Reactor:
                        application=="{ACAD_XDICTIONARY"?Role::ExtensionDictionary:Role::OpaqueHandle;
                    if(p.field!=Field::Application)return false;
                }else {
                    switch(v.code()) {
                    case 5:expected=Role::Self;break;case 330:expected=Role::Owner;break;
                    case 8:expected=Role::LayerName;break;case 6:expected=Role::LineTypeName;break;case 7:expected=Role::TextStyleName;break;
                    case 347:expected=Role::Material;break;case 348:expected=Role::FullVisualStyle;break;
                    case 390:expected=Role::PlotStyle;break;case 360:expected=Role::ExtensionDictionary;break;
                    case 1001:expected=Role::AppIdName;break;case 1003:expected=Role::XdataLayerName;break;case 1005:expected=Role::XdataObjectId;break;
                    case 2:if(physical.name=="INSERT"||physical.name=="MINSERT")expected=Role::BlockName;break;
                    case 100:if(std::string(v.c_str())=="AcDbFaceRecord")face=true;break;
                    default:break;
                    }
                    if((physical.name=="POLYLINE"&&(v.code()==66||v.code()==10||v.code()==20))
                        ||(face&&(v.code()==10||v.code()==20||v.code()==30)))if(p.field!=Field::ComputedIgnored)return false;
                }
                if(p.role!=expected)return false;
                if(expected==Role::Self||expected==Role::Owner||expected==Role::Material||expected==Role::FullVisualStyle
                    ||expected==Role::PlotStyle||expected==Role::Reactor||expected==Role::ExtensionDictionary||expected==Role::XdataObjectId||expected==Role::OpaqueHandle)
                    if(p.handle!=std::stoull(v.c_str(),nullptr,16))return false;
            }
            if(i==member.selfGroup&&(p.role!=Role::Self||p.handle!=member.self))return false;
            if(i==member.ownerGroup&&(p.role!=Role::Owner||p.handle!=member.owner))return false;
            if(p.field==Field::Compound&&p.code==2&&physical.name=="INSERT"&&p.role!=Role::BlockName)return false;
            ++spans;
        }
    }
    return true;
}
void compoundAppend(Tags& tags,const char* id,Tags extra) {
    auto at=membershipSlot(tags,id,5);while(tags[at].code!=0&&tags[at].code!=1001)++at;
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),extra.begin(),extra.end());
}
bool compoundSourceProfileTests(const std::filesystem::path& path) {
    using A=DxIfaceTableSourceTestAccess;using namespace dx_attdef_source;
    unsigned profiles=0u,complete=0u,revisions=0u,variants=0u,faults=0u,contexts=0u,scalarFaults=0u,spanFaults=0u;
    std::size_t spans=0u,comments=0u,originalSpans=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0;encoding<3u;++encoding)for(unsigned form=0;form<7u;++form) {
        const auto f=compoundFixture(version,form);if(!write(path,f.document,encoding==2u,encoding==1u))return false;
        CompoundBodyProbe reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||!A::compoundMatched(data,2u)||!A::compoundMatched(data,3u)
            ||!compoundProfileMatches(data,2u)||!originalChildrenWire(path,encoding==2u,f,reader.childrenAlias,originalSpans))return false;
        ++complete;const bool supported=(form!=4u&&form!=5u)||version=="AC1032";
        if(supported){if(!compoundProfileMatches(data,3u))return false;++complete;}
        else {
            const auto* body=A::compoundBody(data,3u);
            if(A::compoundSourceMatched(data,3u)||!body||body->profiles[1].reason!=Reason::Revision)return false;++revisions;
        }
        if(!compoundProfileInventory(data,2u,spans,comments)||!compoundProfileInventory(data,3u,spans,comments)
            ||A::originResource(data)->rawTypedMatched)return false;
        ++profiles;
    }
    std::cout<<"Compound physical initial profiles="<<profiles<<"; complete bodies="<<complete<<"; documented revision refusals="<<revisions<<std::endl;
    for(bool binary:{false,true}) {
        CompoundBodyProbe reader;
        for(unsigned mode=0;mode<10u;++mode) {
            auto f=compoundFixture("AC1032",mode==0u?4u:mode==7u?3u:0u);
            if(mode==0u) {
                // Every scalar gets adjacent ignored records, including inside
                // application, embedded MTEXT, XDATA coordinates and the tail.
                for(std::size_t j=f.document.size();j-->0u;)if(f.document[j].code==0&&f.document[j].value!="ENDBLK") {
                    const auto& name=f.document[j].value;
                    if(name!="POLYLINE"&&name!="VERTEX"&&name!="INSERT"&&name!="ATTRIB"&&name!="SEQEND")continue;
                    auto end=j+1u;while(f.document[end].code!=0)++end;
                    for(auto i=end;i-->j+1u;)f.document.insert(f.document.begin()+static_cast<std::ptrdiff_t>(i+1u),{{999,"ignored-1"},{999,"ignored-2"}});
                }
            }
            if(mode==1u)for(const char* id:{"D1","D2","E1","E3"}) {
                const auto at=membershipSlot(f.document,id,330);const auto owner=f.document[at];f.document.erase(f.document.begin()+static_cast<std::ptrdiff_t>(at));
                const auto after=membershipSlot(f.document,id,100)+1u;f.document.insert(f.document.begin()+static_cast<std::ptrdiff_t>(after),owner);
            }
            if(mode==2u)compoundAppend(f.document,"D2",{{102,"{CUSTOM"},{100,"AcDbFaceRecord"},{5,"DEAD"},{330,"BEEF"},{320,"FFFFFFFFFFFFFFFF"},
                {8,"not-layer"},{6,"not-line-type"},{347,"FFFFFFFF00000001"},{348,"100000000"},{10,"91"},{102,"{INNER"},{70,"128"},{102,"}"},{102,"}"}});
            if(mode==3u)f.document[membershipSlot(f.document,"D1",66)].value="2";
            if(mode==4u) {
                eraseMembershipRecord(f.document,"E2");eraseMembershipRecord(f.document,"E3");
                const auto at=membershipSlot(f.document,"E1",66);f.document.erase(f.document.begin()+static_cast<std::ptrdiff_t>(at));
            }
            if(mode==5u)eraseMembershipRecord(f.document,"E2");
            if(mode==6u) {
                f.document[membershipSlot(f.document,"E1",5)-1u].value="MINSERT";
                compoundAppend(f.document,"E1",{{70,"3"},{71,"2"},{44,"4.5"},{45,"-2.5"},{41,"-1.5"},{42,"2.5"},{43,"3.5"},{50,"75"}});
            }
            if(mode==7u) {const auto at=membershipSlot(f.document,"D6",20);f.document.erase(f.document.begin()+static_cast<std::ptrdiff_t>(at));}
            if(mode==8u)f.document[membershipSlot(f.document,"E1",2)].value="RES\\U+004FLVED";
            if(mode==9u)f.document[membershipSlot(f.document,"D1",10)].value="+000.000";
            if(!write(path,f.document,binary))return false;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||!compoundProfileMatches(data,2u)||!compoundProfileMatches(data,3u)
                ||!compoundProfileInventory(data,2u,spans,comments)||!compoundProfileInventory(data,3u,spans,comments)
                ||!originalChildrenWire(path,binary,f,reader.childrenAlias,originalSpans)) {
                std::cerr<<"compound positive mode="<<mode<<'\n';
                return false;
            }
            ++variants;
        }
        struct Fault {const char* id;int code;const char* value;const char* old;Reason reason;};
        const Fault cases[]{
            {"D1",90,"1",nullptr,Reason::UnknownField},{"D2",90,"1",nullptr,Reason::UnknownField},{"D3",90,"1",nullptr,Reason::UnknownField},
            {"D4",90,"1",nullptr,Reason::UnknownField},{"E1",90,"1",nullptr,Reason::UnknownField},{"E2",90,"1",nullptr,Reason::UnknownField},{"E3",90,"1",nullptr,Reason::UnknownField},
            {"D1",70,"0",nullptr,Reason::DuplicateField},{"D2",10,"0",nullptr,Reason::DuplicateField},{"D4",8,"0",nullptr,Reason::DuplicateField},
            {"E1",2,"RESOLVED",nullptr,Reason::DuplicateField},{"E2",2,"TAG",nullptr,Reason::DuplicateField},
            {"D1",100,"AcDb3dPolyline","AcDb2dPolyline",Reason::Subclass},{"D2",100,"AcDb3dPolylineVertex","AcDb2dVertex",Reason::Subclass},
            {"E1",100,"VendorReference","AcDbBlockReference",Reason::Subclass},{"D1",10,"0.25","0",Reason::InvalidValue},
            {"D1",75,"7",nullptr,Reason::InvalidValue},{"E1",70,"0",nullptr,Reason::InvalidValue},{"E2",3,"host prompt",nullptr,Reason::UnknownField},
            {"E2",100,"AcDbXrecord","AcDbAttribute",Reason::UnmodeledSecondary},{"E2",280,"0",nullptr,Reason::AmbiguousVersion},
            {"E2",71,"4",nullptr,Reason::UnknownField},{"E2",71,"2",nullptr,Reason::InvalidValue}
        };
        for(const auto& fault:cases) {
            auto f=compoundFixture("AC1032",0u);
            if(fault.old) {
                auto at=membershipSlot(f.document,fault.id,5);bool found=false;
                for(;f.document[at].code!=0;++at)if(f.document[at].code==fault.code&&f.document[at].value==fault.old){f.document[at].value=fault.value;found=true;break;}
                if(!found)return false;
            }else compoundAppend(f.document,fault.id,{{fault.code,fault.value}});
            if(!write(path,f.document,binary))return false;dx_data data;
            const auto dispatch=fault.id[0]=='D'?2u:3u,unrelated=dispatch==2u?3u:2u;
            if(!check(reader.fileImport(path.string(),&data,false)&&A::compoundMatched(data,dispatch)&&!A::compoundFailed(data)
                &&!A::compoundSourceMatched(data,dispatch)&&A::compoundSourceMatched(data,unrelated),"physical field gaps refuse source completeness, not compatible modeled import")) {
                std::cerr<<"compound wire fault="<<fault.id<<" code="<<fault.code<<" value="<<fault.value<<'\n';return false;
            }
            bool reason=false;for(const auto& p:A::compoundBody(data,dispatch)->profiles)if(p.reason==fault.reason)reason=true;
            if(!check(reason,"physical refusal has the exact expected reason"))return false;
            auto fixed=compoundFixture("AC1032",0u);if(!write(path,fixed.document,binary))return false;
            dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::compoundSourceMatched(retry,dispatch))return false;++faults;
        }
        auto good=compoundFixture("AC1032",0u);if(!write(path,good.document,binary))return false;
        dx_data data;if(!reader.fileImport(path.string(),&data,false)||!A::compoundSourceMatched(data,2u))return false;
        const auto old=A::mutableOriginBlock(data).compoundBodies.at(2u);
        dx_data newer;if(!reader.fileImport(path.string(),&newer,false)||!A::compoundSourceMatched(newer,2u))return false;
        const auto current=A::mutableOriginBlock(newer).compoundBodies.at(2u);A::mutableOriginBlock(newer).compoundBodies[2u]=old;
        if(A::compoundSourceMatched(newer,2u))return false;A::mutableOriginBlock(newer).compoundBodies[2u]=current;
        if(!A::compoundSourceMatched(newer,2u))return false;++contexts;
        dx_data copied;A::copyMembershipContext(copied,newer);if(A::compoundSourceMatched(copied,2u))return false;++contexts;
        const auto& physical=old->membership->children->records[old->membership->ranges[2u].first];
        DRW_RawDxfObject raw;raw.name=physical.name;raw.handle=0xD1u;raw.parentHandle=0xB0u;raw.m_version=DRW::AC1032;raw.groups=physical.groups;
        for(unsigned kind=0u;kind<5u;++kind) {
            auto corrupt=raw;
            const auto i=std::find_if(corrupt.groups.begin(),corrupt.groups.end(),[](const DRW_Variant& v){return v.code()==10;});
            if(kind==0u)*i=DRW_Variant(10,1);
            if(kind==1u)*i=DRW_Variant(10,UTF8STRING("0"));
            if(kind==2u)*i=DRW_Variant(10,DRW_Coord{0,0,0});
            if(kind==3u)*i=DRW_Variant(10,(std::numeric_limits<double>::quiet_NaN)());
            if(kind==4u)corrupt.groups.push_back(DRW_Variant(160,1));
            if(dx_compound_source::audit(corrupt).reason!=Reason::InvalidValue)return false;++scalarFaults;
        }
        for(unsigned kind=0u;kind<5u;++kind) {
            auto p=old->profiles.front();auto broken=physical;
            if(kind==0u)broken.spans.pop_back();
            if(kind==1u)broken.spans.front().ignoredComment=true;
            if(kind==2u)p.spans.pop_back();
            if(kind==3u)p.spans.push_back(p.spans.back());
            if(kind==4u)p.spans.front().code=-1;
            if(dx_compound_source::restorePhysicalIndices(p,broken))return false;++spanFaults;
        }
    }
    std::cout<<"Compound physical profiles="<<profiles<<"; complete bodies="<<complete<<"; revision refusals="<<revisions
        <<"; independent original scalar spans="<<originalSpans<<"; complete profile scalar spans="<<spans<<"; ignored comments="<<comments
        <<"; positive variants="<<variants<<"; wire faults/retries="<<faults<<"; stale/copied contexts="<<contexts<<"; forged scalar types="<<scalarFaults
        <<"; corrupt physical spans="<<spanFaults<<'\n';
    return profiles==84u&&complete==150u&&revisions==18u&&variants==20u&&faults==46u&&contexts==4u&&scalarFaults==10u&&spanFaults==10u
        &&originalSpans==23592u&&spans==12216u&&comments==416u;
}
