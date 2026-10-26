// SPDX-License-Identifier: GPL-2.0-or-later
#include "independent_dxf_scalar_walker.h"
std::size_t blockField(const Tags& tags,const char* handle,int code) {
    auto at=slot(tags,5,handle);for(;tags[at].code!=0;++at)if(tags[at].code==code)return at;
    throw std::runtime_error("missing authored block field");
}
void eraseBlockRecord(Tags& tags,const char* handle) {
    const auto first=slot(tags,5,handle)-1u;auto end=first+1u;while(tags[end].code!=0)++end;
    tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(first),tags.begin()+static_cast<std::ptrdiff_t>(end));
}
void appendBlockFields(Tags& tags,const char* handle,Tags extra) {
    auto at=slot(tags,5,handle);while(tags[at].code!=0)++at;
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),extra.begin(),extra.end());
}
void geometryOnlyBindings(Tags& tags) {
    for(auto& tag:tags)if(tag.code==330&&(tag.value=="D001"||tag.value=="D002"))tag.value="0";
}
void noBlockContentBindings(Tags& tags) {
    const auto at=slot(tags,340,"B100");if(tags[at-1u].code!=90||tags[at-1u].value!="4")throw std::runtime_error("missing authored block content");
    tags[at-1u].value="2";tags[at].value="F100";
}
Tags blockBindingSource(const std::string& version,unsigned form) {
    auto tags=bindingSource(version);if(!form)return tags;
    Tags children{{0,"POLYLINE"},{5,"D100"},{330,"B100"},{100,"AcDbEntity"},{8,"0"},
        {100,form==2u?"AcDb3dPolyline":form==3u?"AcDbPolyFaceMesh":"AcDb2dPolyline"},
        {66,"1"},{10,"0"},{20,"0"},{30,"4.5"},{70,form==2u?"8":form==3u?"64":"0"}};
    const auto vertex=[&](const char* handle,const char* x,const char* y) {
        return Tags{{0,"VERTEX"},{5,handle},{330,"D100"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbVertex"},
            {100,form==2u?"AcDb3dPolylineVertex":form==3u?"AcDbPolyFaceMeshVertex":"AcDb2dVertex"},
            {10,x},{20,y},{30,"6.25"},{70,form==2u?"32":form==3u?"192":"0"}};
    };
    for(const auto& v:{vertex("D101","-0.0","1.25"),vertex("D102","2.5","3.75")})children.insert(children.end(),v.begin(),v.end());
    if(form==3u) {
        const auto v=vertex("D104","4","5");children.insert(children.end(),v.begin(),v.end());
        children.insert(children.end(),{{0,"VERTEX"},{5,"D105"},{330,"D100"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbFaceRecord"},
            {20,"7"},{70,"128"},{71,"1"},{72,"-2"},{73,"3"},{74,"0"}});
    }
    children.insert(children.end(),{{0,"SEQEND"},{5,"D103"},{330,"D100"},{100,"AcDbEntity"},{8,"0"},
        {0,"INSERT"},{5,"E100"},{330,"B100"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockReference"},
        {2,"*T1"},{66,"1"},{10,"1"},{20,"2"},{30,"3"},{41,"1.5"},{42,"2.5"},{43,"3.5"},{50,"30"},
        {0,"ATTRIB"},{5,"E101"},{330,"E100"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbText"},
        {10,"0"},{20,"0"},{30,"1"},{40,"1.5"},{1,"attribute-é"}});
    if(form==4u)children.insert(children.end(),{{101,"Embedded Object"},{10,"5"},{20,"6"},{30,"7"},{40,"2.5"},{41,"9"},{1,"embedded-é"},
        {11,"0"},{21,"1"},{31,"0"},{71,"4"}});
    children.insert(children.end(),{{100,"AcDbAttribute"},{2,"CHILD"},{70,"0"},
        {0,"SEQEND"},{5,"E102"},{330,"E100"},{100,"AcDbEntity"},{8,"0"}});
    const auto at=slot(tags,5,"31")-1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),children.begin(),children.end());return tags;
}
bool writeBlockBinding(const std::filesystem::path& path,const Tags& tags,unsigned encoding) {
    if(encoding!=1u)return write(path,tags,encoding==2u);
    std::ofstream stream(path,std::ios::binary);for(const auto& t:tags)stream<<t.code<<"\r\n"<<t.value<<"\r\n";return stream.good();
}
bool blockBindingOriginalWire(const std::filesystem::path& path,bool binary,const Tags& tags,const dx_data& data,std::size_t& spans) {
    const auto wire=attdefWireTags(path,binary);if(wire.size()!=tags.size()) {
        std::cerr<<"block wire cardinality="<<wire.size()<<" authored="<<tags.size()<<'\n';return false;
    }
    for(const auto& row:DxIfaceTableSourceTestAccess::blockSources(data)) {
        if(!row.second.children)return false;const auto& source=*row.second.children;
        for(const auto& record:source.records) {
            if(record.groups.size()!=record.spans.size()||record.groups.size()!=record.rawValues.size())return false;
            const auto first=record.marker.ordinal-1u;
            if(first>=wire.size()||wire[first].code!=0||wire[first].text!=record.name||wire[first].begin!=record.marker.beginOffset||wire[first].end!=record.marker.endOffset)return false;
            for(std::size_t i=0;i<record.groups.size();++i) {
                const auto index=first+1u+i;if(index>=wire.size())return false;
                const auto& w=wire[index];const auto& expected=tags[index];const auto& p=record.spans[i];const auto& v=record.groups[i];
                if(w.code!=expected.code||w.code!=v.code()||p.ordinal!=index+1u||p.beginOffset!=w.begin||p.endOffset!=w.end||p.ignoredComment!=(w.code==999)
                    ||(binary?!record.rawValues[i].empty():record.rawValues[i]!=expected.value)) {
                    std::cerr<<"block wire scalar="<<record.name<<'/'<<index<<" code="<<w.code<<" raw="<<record.rawValues[i]<<'\n';return false;
                }
                if(w.kind==AttdefWireTag::Integer) {
                    if((v.type()!=DRW_Variant::INTEGER&&v.type()!=DRW_Variant::INTEGER64)
                        ||w.integer!=std::stoll(expected.value)||(v.type()==DRW_Variant::INTEGER64?v.i64_val():v.i_val())!=w.integer)return false;
                }else if(w.kind==AttdefWireTag::Real) {
                    if(v.type()!=DRW_Variant::DOUBLE)return false;
                    const double original=std::stod(expected.value),captured=v.d_val();
                    if(std::memcmp(&w.real,&original,sizeof(double))||std::memcmp(&captured,&original,sizeof(double)))return false;
                }else if(w.text!=expected.value||v.type()!=DRW_Variant::STRING||!v.content.s||*v.content.s!=w.text) {
                    std::cerr<<"block wire text="<<record.name<<'/'<<index<<" code="<<w.code<<" original="<<w.text<<" authored="<<expected.value<<" captured="<<(v.type()==DRW_Variant::STRING&&v.content.s?*v.content.s:"<wrong kind>")<<'\n';return false;
                }
                ++spans;
            }
            const auto end=record.endOrdinal-1u;if(end>=wire.size()||record.endOffset!=wire[end].begin)return false;
        }
        const auto end=source.endMarker.ordinal-1u;
        if(end>=wire.size()||wire[end].text!="ENDBLK"||wire[end].begin!=source.endMarker.beginOffset||wire[end].end!=source.endMarker.endOffset)return false;
    }
    return true;
}
bool tableBlockBindingTests(const std::filesystem::path& dir) {
    using S=dx_iface::TableBindingState;using A=DxIfaceTableSourceTestAccess;
    const auto path=dir/"block-binding.dxf";const auto none=(std::numeric_limits<std::size_t>::max)();
    unsigned profiles=0u,matched=0u,revisions=0u,wireCases=0u,callbacks=0u,quotas=0u,contexts=0u,exceptions=0u;long allocations=0;
    std::size_t spans=0u;unsigned lateAllocationFaults=0u;
    const auto valid=[&](const dx_data& data,std::size_t members) {
        const auto r=dx_iface::tableContentBindingReport(data);
        if(r.state==S::Bound&&r.directRolesBound&&r.blockSourceDefinitions==1u&&r.blockSourceMembers==members&&!r.outputEligible)return true;
        std::cerr<<"block binding state="<<static_cast<int>(r.state)<<" handle="<<r.firstHandle<<" block="<<r.firstSourceBlock<<" member="<<r.firstSourceMember
            <<" definitions="<<r.blockSourceDefinitions<<" direct="<<r.directRolesBound<<'\n';return false;
    };
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0;encoding<3u;++encoding)for(unsigned form=0;form<5u;++form) {
        const auto tags=blockBindingSource(version,form);if(!writeBlockBinding(path,tags,encoding))return false;
        Probe reader;reader.inspectDuringChild=true;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||reader.prematureBound) {
            std::cerr<<"block profile import="<<version<<'/'<<encoding<<'/'<<form<<" premature="<<reader.prematureBound<<'\n';return false;
        }
        const auto report=dx_iface::tableContentBindingReport(data);
        if(form==4u&&version!="AC1032") {
            if(report.state!=S::BlockMemberMismatch||!report.directRolesBound||report.firstSourceBlock!=0x30u||report.firstSourceMember!=7u||report.outputEligible) {
                std::cerr<<"block revision="<<version<<'/'<<encoding<<'/'<<form<<" state="<<static_cast<int>(report.state)<<" member="<<report.firstSourceMember<<'\n';return false;
            }++revisions;
        }else {if(!valid(data,form==0u?2u:form==3u?11u:9u))return false;++matched;}
        if(!A::provisionalAttdefs(data)||!blockBindingOriginalWire(path,encoding==2u,tags,data,spans)) {
            std::cerr<<"block original wire="<<version<<'/'<<encoding<<'/'<<form<<'\n';return false;
        }++profiles;
    }
    std::cout<<"Selected BLOCK source profiles="<<profiles<<"; matched="<<matched<<"; explicit revision gaps="<<revisions<<std::endl;
    const auto append=[](Tags& t,Tags record){const auto at=slot(t,5,"31")-1u;t.insert(t.begin()+static_cast<std::ptrdiff_t>(at),record.begin(),record.end());};
    struct Case {const char* name;std::function<void(Tags&)> change;S state;std::size_t member;std::size_t members=9u,definitions=1u;};
    const std::vector<Case> cases{
        {"missing definition",[](auto& t){geometryOnlyBindings(t);const auto a=slot(t,5,"30")-1u,b=slot(t,5,"32")-1u;t.erase(t.begin()+static_cast<std::ptrdiff_t>(a),t.begin()+static_cast<std::ptrdiff_t>(b));},S::BlockSourceMissing,none},
        {"duplicate definition",[](auto& t){const auto at=slot(t,5,"33")+1u;auto end=at;while(t[end].code!=0)++end;t.insert(t.begin()+static_cast<std::ptrdiff_t>(end),{{0,"BLOCK"},{5,"34"},{330,"B100"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},{2,"TABLE-BLOCK"},{70,"0"},{10,"0"},{20,"0"},{0,"ENDBLK"},{5,"35"},{330,"B100"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"}});},S::AmbiguousBlockDefinition,none},
        {"raw member",[append](auto& t){append(t,{{0,"VENDOR_ENTITY"},{5,"F200"},{330,"B100"},{100,"AcDbEntity"},{8,"0"},{1,"opaque"}});},S::BlockMemberUnsupported,9u},
        {"unproved primary",[append](auto& t){append(t,{{0,"POINT"},{5,"F200"},{330,"B100"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"1"},{20,"2"},{30,"3"}});},S::BlockMemberUnsupported,9u},
        {"no callback",[append](auto& t){geometryOnlyBindings(t);append(t,{{0,"DIMENSION"},{5,"F200"},{330,"B100"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbDimension"},{70,"7"}});},S::BlockMemberUnsupported,9u},
        {"unknown vertex scalar",[](auto& t){appendBlockFields(t,"D101",{{90,"123"}});},S::BlockMemberMismatch,3u},
        {"wrong polyline subclass",[](auto& t){const auto at=blockField(t,"D100",100);t[at+2u].value="AcDb3dPolyline";},S::BlockMemberMismatch,2u},
        {"parent paper only",[](auto& t){appendBlockFields(t,"D100",{{67,"1"}});},S::BlockSpaceMismatch,3u},
        {"vertex paper only",[](auto& t){appendBlockFields(t,"D101",{{67,"1"}});},S::BlockSpaceMismatch,3u},
        {"polyline footer paper",[](auto& t){appendBlockFields(t,"D103",{{67,"1"}});},S::BlockSpaceMismatch,5u},
        {"insert paper only",[](auto& t){appendBlockFields(t,"E100",{{67,"1"}});},S::BlockSpaceMismatch,7u},
        {"attribute paper only",[](auto& t){const auto at=blockField(t,"E101",100)+1u;t.insert(t.begin()+static_cast<std::ptrdiff_t>(at),{67,"1"});},S::BlockSpaceMismatch,7u},
        {"insert footer paper",[](auto& t){appendBlockFields(t,"E102",{{67,"1"}});},S::BlockSpaceMismatch,8u},
        {"all linked hosts paper",[](auto& t){for(const char* h:{"D100","D101","D102","D103","E100","E101","E102"}){const auto at=blockField(t,h,100)+1u;t.insert(t.begin()+static_cast<std::ptrdiff_t>(at),{67,"1"});}},S::Bound,none},
        {"footer record owners",[](auto& t){for(const char* h:{"D103","E102"})t[blockField(t,h,330)].value="B100";},S::Bound,none},
        {"empty insert",[](auto& t){eraseBlockRecord(t,"E101");eraseBlockRecord(t,"E102");const auto at=blockField(t,"E100",66);t.erase(t.begin()+static_cast<std::ptrdiff_t>(at));},S::Bound,none,7u},
        {"ATTDEF owner selection only",[](auto& t){noBlockContentBindings(t);},S::Bound,none},
        {"selected empty definition",[](auto& t){geometryOnlyBindings(t);t[slot(t,340,"B100")].value="B200";},S::Bound,none,0u},
        {"two selected definitions",[](auto& t){geometryOnlyBindings(t);const auto at=slot(t,340,"F100");t[at-1u].value="4";t[at].value="B200";},S::Bound,none,9u,2u},
        {"unselected raw definition",[](auto& t){const auto at=slot(t,5,"33")-1u;t.insert(t.begin()+static_cast<std::ptrdiff_t>(at),{{0,"VENDOR_ENTITY"},{5,"F200"},{330,"B200"},{100,"AcDbEntity"},{8,"0"},{1,"opaque"}});},S::Bound,none},
        {"endpoint scalar gap",[](auto& t){geometryOnlyBindings(t);appendBlockFields(t,"30",{{90,"1"}});},S::BlockSourceIncomplete,none},
        {"space owner metadata only",[](auto& t){geometryOnlyBindings(t);noBlockContentBindings(t);const auto a=slot(t,2,"BLOCKS")-1u,b=slot(t,2,"ENTITIES")-1u;t.erase(t.begin()+static_cast<std::ptrdiff_t>(a),t.begin()+static_cast<std::ptrdiff_t>(b));},S::Bound,none,0u,0u}
    };
    for(bool binary:{false,true}) {
        const auto good=blockBindingSource("AC1032",1u);Probe reader;
        for(const auto& c:cases) {
            auto tags=good;c.change(tags);if(!write(path,tags,binary))return false;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)) {std::cerr<<"block wire read="<<c.name<<'\n';return false;}
            const auto r=dx_iface::tableContentBindingReport(data);
            if(r.state!=c.state||!r.directRolesBound||r.outputEligible||(r.state!=S::Bound&&r.firstSourceMember!=c.member)) {
                std::cerr<<"block wire="<<c.name<<" state="<<static_cast<int>(r.state)<<" direct="<<r.directRolesBound<<" member="<<r.firstSourceMember<<'\n';return false;
            }
            if(c.state==S::Bound&&(r.blockSourceDefinitions!=c.definitions||r.blockSourceMembers!=c.members))return false;
            if(!write(path,good,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!valid(retry,9u))return false;++wireCases;
        }
        for(unsigned fault=0u;fault<6u;++fault) {
            reader.alterPolyline=fault<3u?std::function<void(DRW_Polyline&)>([&](auto& v){if(v.handle!=0xD100u)return;if(fault==0u)v.vertlist[0]->basePoint.x+=1;if(fault==1u)v.defstawidth+=1;if(fault==2u)v.seqEndEntity->layer="changed";}):nullptr;
            // TABLE also calls addInsert for its compatibility projection.
            // Mutate only this actual source compound, never that projection.
            reader.alterInsert=fault>=3u?std::function<void(DRW_Insert&)>([&](auto& v){if(v.handle!=0xE100u)return;if(fault==3u)v.xscale+=1;if(fault==4u)v.attlist[0]->text="changed";if(fault==5u)v.seqEndEntity->color=3;}):nullptr;
            dx_data bad;if(!reader.fileImport(path.string(),&bad,false))return false;const auto r=dx_iface::tableContentBindingReport(bad);
            if(r.state!=S::BlockMemberMismatch||!r.directRolesBound||r.outputEligible)return false;
            reader.alterPolyline={};reader.alterInsert={};dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!valid(retry,9u))return false;++callbacks;
        }
        dx_data baseline;if(!reader.fileImport(path.string(),&baseline,false)||!valid(baseline,9u))return false;
        const auto measured=dx_iface::tableContentBindingReport(baseline);
        for(unsigned dimension=0;dimension<2u;++dimension)for(bool exact:{false,true}) {
            dx_data limited;if(dimension)A::blockBindingLimit(limited,measured.blockSourceWorkspaceBytes-(exact?0u:1u));
            else A::resourceLimits(limited,100000u,4000000u,64u*1024u*1024u,measured.examinedSlots-(exact?0u:1u));
            if(!reader.fileImport(path.string(),&limited,false)||dx_iface::tableContentBindingReport(limited).state!=(exact?S::Bound:S::WorkLimit))return false;++quotas;
        }
        bool calibrated=false;
        for(long fault=0;fault<40&&!calibrated;++fault) {
            dx_iface::TableContentBindingReport failed;
            {SymbolAllocationFailure schedule(fault);failed=dx_iface::tableContentBindingReport(baseline);}
            if(failed.state==S::Bound)calibrated=true;
            else {if(failed.state!=S::CaptureFailed||failed.outputEligible)return false;++allocations;if(failed.directRolesBound)++lateAllocationFaults;}
            if(!valid(baseline,9u))return false;
        }
        if(!calibrated)return false;
        for(unsigned replaced=0u;replaced<2u;++replaced) {
            auto& resource=A::mutableResource(baseline,0xB100u);const auto savedModel=resource.model;const auto savedRaw=resource.raw;
            if(replaced)resource.raw=std::make_shared<const DRW_RawDxfObject>(*savedRaw);
            else resource.model=std::make_shared<const DRW_Block_Record>(*dynamic_cast<const DRW_Block_Record*>(savedModel.get()));
            if(dx_iface::tableContentBindingReport(baseline).state!=S::BlockSourceIncomplete)return false;
            resource.model=savedModel;resource.raw=savedRaw;if(!valid(baseline,9u))return false;++contexts;
        }
        dx_data copied;A::copyBindingContext(copied,baseline);if(dx_iface::tableContentBindingReport(copied).state==S::Bound)return false;++contexts;
        for(int phase=0;phase<2;++phase) {
            reader.throwChild=phase;dx_data failed;bool threw=false;
            try{(void)reader.fileImport(path.string(),&failed,false);}catch(const std::runtime_error&){threw=true;}
            if(!threw||dx_iface::tableContentBindingReport(failed).state==S::Bound)return false;
            reader.throwChild=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!valid(retry,9u))return false;++exceptions;
        }
        for(unsigned mode=0;mode<3u;++mode) {
            reader.definitionOptIn=mode!=0u;reader.childrenOptIn=mode!=1u;reader.deliveryOptIn=mode!=2u;
            dx_data failed;if(!reader.fileImport(path.string(),&failed,false)||dx_iface::tableContentBindingReport(failed).state==S::Bound)return false;
            reader.definitionOptIn=reader.childrenOptIn=reader.deliveryOptIn=true;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!valid(retry,9u))return false;++contexts;
        }
        if(!reader.fileImport(path.string(),&baseline,false)||dx_iface::tableContentBindingReport(baseline).state==S::Bound)return false;++contexts;
    }
    std::cout<<"Selected BLOCK bindings: profiles="<<profiles<<"; matched="<<matched<<"; revision refusals="<<revisions<<"; original member scalar spans="<<spans
        <<"; wire cases/retries="<<wireCases<<"; callback mismatches/retries="<<callbacks<<"; exact/under budgets="<<quotas
        <<"; observed query allocation faults="<<allocations<<"; after direct-role proof="<<lateAllocationFaults
        <<"; replaced/copied/opt-out/appended contexts="<<contexts<<"; exception retries="<<exceptions<<'\n';
    return profiles==60u&&matched==51u&&revisions==9u&&wireCases==cases.size()*2u&&callbacks==12u&&quotas==8u&&allocations>0&&lateAllocationFaults>0u&&contexts==14u&&exceptions==4u&&spans==5184u;
}
