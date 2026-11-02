// SPDX-License-Identifier: GPL-2.0-or-later
Tags blockNameInsert(const char* self,const char* owner,const std::string& name) {
    return {{0,"INSERT"},{5,self},{330,owner},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockReference"},{2,name},{10,"0"},{20,"0"},{30,"0"}};
}
void blockNameAppend(Tags& tags,const char* footer,const Tags& body) {
    const auto at=slot(tags,5,footer)-1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),body.begin(),body.end());
}
void blockNameAdd(Tags& tags,const char* record,const char* self,const char* footer,const std::string& name) {
    auto at=slot(tags,0,"ENDTAB");tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{
        {0,"BLOCK_RECORD"},{5,record},{330,"10"},{100,"AcDbSymbolTableRecord"},{100,"AcDbBlockTableRecord"},{2,name},{70,"0"}});
    at=slot(tags,2,"BLOCK_RECORD");while(tags[at].code!=70)++at;tags[at].value=std::to_string(std::stoi(tags[at].value)+1);
    at=slot(tags,5,"33");while(tags[at].code!=0)++at;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{
        {0,"BLOCK"},{5,self},{330,record},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},{2,name},{70,"0"},{10,"0"},{20,"0"},{30,"0"},
        {0,"ENDBLK"},{5,footer},{330,record},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"}});
}
Tags blockNameSource(const std::string& version) {
    auto tags=blockBindingSource(version,1u);blockNameAdd(tags,"B300","34","35","leaf-é");
    blockNameAppend(tags,"33",blockNameInsert("E200","B200","leaf-é"));return tags;
}
std::string blockNameHex(std::uint32_t value) {std::ostringstream text;text<<std::hex<<std::uppercase<<value;return text.str();}
Tags blockNameChain(unsigned depth) {
    auto tags=blockBindingSource("AC1032",1u);Tags rows,blocks;
    for(unsigned i=0;i<depth;++i) {
        const auto record=blockNameHex(0x100000u+i),self=blockNameHex(0x200000u+2u*i),footer=blockNameHex(0x200001u+2u*i);
        const auto name="chain-"+std::to_string(i);
        rows.insert(rows.end(),{{0,"BLOCK_RECORD"},{5,record},{330,"10"},{100,"AcDbSymbolTableRecord"},{100,"AcDbBlockTableRecord"},{2,name},{70,"0"}});
        blocks.insert(blocks.end(),{{0,"BLOCK"},{5,self},{330,record},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},{2,name},{70,"0"},{10,"0"},{20,"0"},{30,"0"}});
        if(i+1u<depth) {
            const auto insert=blockNameHex(0x300000u+i);const auto body=blockNameInsert(insert.c_str(),record.c_str(),"chain-"+std::to_string(i+1u));
            blocks.insert(blocks.end(),body.begin(),body.end());
        }
        blocks.insert(blocks.end(),{{0,"ENDBLK"},{5,footer},{330,record},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"}});
    }
    auto at=slot(tags,0,"ENDTAB");tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),rows.begin(),rows.end());
    at=slot(tags,2,"BLOCK_RECORD");while(tags[at].code!=70)++at;tags[at].value=std::to_string(depth+3u);
    blockNameAppend(tags,"33",blockNameInsert("E200","B200","chain-0"));
    at=slot(tags,5,"33");while(tags[at].code!=0)++at;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),blocks.begin(),blocks.end());return tags;
}
bool tableBlockNameTests(const std::filesystem::path& dir) {
    using S=dx_iface::TableBindingState;using A=DxIfaceTableSourceTestAccess;
    const auto path=dir/"block-names.dxf";const auto none=(std::numeric_limits<std::size_t>::max)();
    unsigned routes=0u,wireCases=0u,variants=0u,quotas=0u,owners=0u,deep=0u,defaultCaptureLimits=0u;long allocations=0;
    const auto valid=[](const dx_data& data,std::size_t definitions=3u,std::size_t members=10u,std::size_t names=2u) {
        const auto r=dx_iface::tableContentBindingReport(data);
        if(r.state==S::Bound&&r.directRolesBound&&r.blockNameGraphAcyclic&&r.blockSourceDefinitions==definitions
            &&r.blockSourceMembers==members&&r.blockNameReferences==names&&r.blockNameBindings==names&&!r.outputEligible)return true;
        std::cerr<<"block name state="<<static_cast<int>(r.state)<<" handle="<<r.firstHandle<<" block="<<r.firstSourceBlock<<" member="<<r.firstSourceMember
            <<" definitions="<<r.blockSourceDefinitions<<" members="<<r.blockSourceMembers<<" references="<<r.blockNameReferences<<" bindings="<<r.blockNameBindings<<'\n';return false;
    };
    struct Case {const char* label;std::function<void(Tags&)> change;S state;std::uint32_t block;std::size_t member;};
    const std::vector<Case> cases{
        {"root dangling name",[](auto& t){t[blockField(t,"E100",2)].value="missing";},S::MissingTarget,0x30u,6u},
        {"deep dangling name",[](auto& t){t[blockField(t,"E200",2)].value="missing";},S::MissingTarget,0x32u,0u},
        {"name is not a handle",[](auto& t){t[blockField(t,"E200",2)].value="B300";},S::MissingTarget,0x32u,0u},
        {"name is not a local ID",[](auto& t){t[blockField(t,"E200",2)].value="101";},S::MissingTarget,0x32u,0u},
        {"ambiguous name",[](auto& t){blockNameAdd(t,"B400","36","37","LEAF-é");},S::AmbiguousIdentity,0x32u,0u},
        {"fold-only Unicode alias",[](auto& t){t[blockField(t,"E200",2)].value="leaf-É";},S::BlockNameCollationUnqualified,0x32u,0u},
        {"missing leaf definition",[](auto& t){eraseBlockRecord(t,"34");eraseBlockRecord(t,"35");},S::BlockSourceMissing,0u,none},
        {"raw leaf",[](auto& t){blockNameAppend(t,"35",{{0,"VENDOR_ENTITY"},{5,"E300"},{330,"B300"},{100,"AcDbEntity"},{8,"0"},{1,"opaque"}});},S::BlockMemberUnsupported,0x34u,0u},
        {"primary leaf",[](auto& t){blockNameAppend(t,"35",{{0,"POINT"},{5,"E300"},{330,"B300"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"1"},{20,"2"},{30,"3"}});},S::BlockMemberUnsupported,0x34u,0u},
        {"leaf endpoint gap",[](auto& t){appendBlockFields(t,"34",{{90,"1"}});},S::BlockSourceIncomplete,0x34u,none},
        {"self reference",[](auto& t){t[blockField(t,"E200",2)].value="*T1";},S::BlockReferenceCycle,0x32u,0u},
        {"two-node cycle",[](auto& t){t[blockField(t,"E200",2)].value="TABLE-BLOCK";},S::BlockReferenceCycle,0x32u,0u},
        {"three-node cycle",[](auto& t){blockNameAppend(t,"35",blockNameInsert("E300","B300","TABLE-BLOCK"));},S::BlockReferenceCycle,0x34u,0u},
        {"nested source gap",[](auto& t){appendBlockFields(t,"E200",{{90,"1"}});},S::BlockMemberMismatch,0x32u,0u},
        {"MINSERT marker without grid",[](auto& t){t[blockField(t,"E200",100)+2u].value="AcDbMInsertBlock";},S::BlockMemberMismatch,0x32u,0u},
        {"unsupported derived marker",[](auto& t){t[blockField(t,"E200",100)+2u].value="VendorBlockReference";},S::BlockMemberMismatch,0x32u,0u}
    };
    struct Variant {const char* label;std::function<void(Tags&)> change;std::size_t members=10u,names=2u;};
    const std::vector<Variant> positives{
        {"ASCII case alias",[](auto& t){t[blockField(t,"E200",2)].value="LEAF-é";}},
        {"repeated dependency",[](auto& t){blockNameAppend(t,"33",blockNameInsert("E201","B200","leaf-é"));},11u,3u},
        {"shared diamond dependency",[](auto& t){blockNameAppend(t,"31",blockNameInsert("E201","B100","leaf-é"));},11u,3u},
        {"MINSERT names one block",[](auto& t){t[blockField(t,"E200",100)+2u].value="AcDbMInsertBlock";appendBlockFields(t,"E200",{{70,"2"},{71,"3"},{44,"4"},{45,"5"}});}},
        {"MINSERT one axis",[](auto& t){t[blockField(t,"E200",100)+2u].value="AcDbMInsertBlock";appendBlockFields(t,"E200",{{70,"2"},{44,"4"}});}},
        {"legacy MINSERT alias",[](auto& t){t[slot(t,5,"E200")-1u].value="MINSERT";appendBlockFields(t,"E200",{{70,"2"},{71,"3"},{44,"4"},{45,"5"}});}},
        {"literal handle-looking name",[](auto& t){for(auto& v:t)if(v.code==2&&v.value=="leaf-é")v.value="B300";}},
        {"long exact UTF8 name",[](auto& t){for(auto& v:t)if(v.code==2&&v.value=="leaf-é")v.value=std::string(120u,'x')+"-é";}}
    };
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0;encoding<3u;++encoding) {
        const auto good=blockNameSource(version);Probe reader;dx_data baseline;
        if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&baseline,false)||!valid(baseline))return false;++routes;
        for(const auto& c:cases) {
            auto tags=good;c.change(tags);dx_data data;if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false))return false;
            const auto r=dx_iface::tableContentBindingReport(data);
            if(r.state!=c.state||!r.directRolesBound||r.blockNameGraphAcyclic||r.firstSourceBlock!=c.block||r.firstSourceMember!=c.member||r.outputEligible) {
                std::cerr<<"block name case="<<c.label<<" revision="<<version<<" encoding="<<encoding<<" state="<<static_cast<int>(r.state)
                    <<" block="<<r.firstSourceBlock<<" member="<<r.firstSourceMember<<" direct="<<r.directRolesBound<<'\n';return false;
            }
            dx_data retry;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&retry,false)||!valid(retry))return false;++wireCases;
        }
        for(const auto& c:positives) {
            auto tags=good;c.change(tags);dx_data data;if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!valid(data,3u,c.members,c.names)) {
                std::cerr<<"block name variant="<<c.label<<'\n';return false;
            }++variants;
        }
    }
    std::cout<<"Nested BLOCK matrix routes="<<routes<<"; wire retries="<<wireCases<<"; variants="<<variants<<std::endl;
    for(bool binary:{false,true}) {
        auto good=blockNameSource("AC1032");positives.back().change(good);Probe reader;dx_data baseline;
        if(!write(path,good,binary)||!reader.fileImport(path.string(),&baseline,false)||!valid(baseline))return false;
        const auto measured=dx_iface::tableContentBindingReport(baseline);
        for(unsigned dimension=0;dimension<2u;++dimension)for(bool exact:{false,true}) {
            dx_data data;if(dimension)A::blockBindingLimit(data,measured.blockSourceWorkspaceBytes-(exact?0u:1u));
            else A::resourceLimits(data,100000u,4000000u,64u*1024u*1024u,measured.examinedSlots-(exact?0u:1u));
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::tableContentBindingReport(data).state!=(exact?S::Bound:S::WorkLimit))return false;++quotas;
        }
        bool calibrated=false;
        for(long fault=0;fault<80&&!calibrated;++fault) {
            dx_iface::TableContentBindingReport r;{SymbolAllocationFailure schedule(fault);r=dx_iface::tableContentBindingReport(baseline);}
            if(r.state==S::Bound)calibrated=true;
            else {if(r.state!=S::CaptureFailed||r.outputEligible||r.blockNameGraphAcyclic)return false;++allocations;}
            if(!valid(baseline))return false;
        }
        if(!calibrated)return false;
        for(bool raw:{false,true}) {
            auto& target=A::mutableResource(baseline,0xB300u);const auto model=target.model;const auto source=target.raw;
            if(raw)target.raw=std::make_shared<const DRW_RawDxfObject>(*source);
            else target.model=std::make_shared<const DRW_Block_Record>(*dynamic_cast<const DRW_Block_Record*>(model.get()));
            const auto r=dx_iface::tableContentBindingReport(baseline);
            if(r.state!=S::BlockSourceIncomplete||r.firstSourceBlock!=0x34u||!r.directRolesBound||r.outputEligible)return false;
            target.model=model;target.raw=source;if(!valid(baseline))return false;++owners;
        }
        // Mutable consumer names cannot rewrite historical original targets.
        for(auto& record:baseline.blockRecords)if(record.handle==0xB300u)record.name="edited-name";
        if(!valid(baseline)) {std::cerr<<"block name mutable consumer state\n";return false;}
        constexpr unsigned depth=256u;const auto chain=blockNameChain(depth);dx_data linked;
        // This authored stress tree exceeds the default endpoint-audit work
        // allowance, not the reader's admission policy. Keep that refusal and
        // use an explicitly bounded larger TEST allowance to exercise DFS.
        dx_data bounded;
        if(!write(path,chain,binary)||!reader.fileImport(path.string(),&bounded,false)
            ||!A::blockEndpointWorkRefused(bounded)||dx_iface::tableContentBindingReport(bounded).state==S::Bound)return false;
        ++defaultCaptureLimits;
        A::blockEndpointWorkLimit(linked,16u*1024u*1024u);
        if(!reader.fileImport(path.string(),&linked,false)||!valid(linked,depth+2u,depth+9u,depth+1u)) {
            std::cerr<<"block name deep chain binary="<<binary<<'\n';return false;
        }
        for(bool cycle:{false,true}) {
            auto tags=chain;const auto footer=blockNameHex(0x200001u+2u*(depth-1u)),record=blockNameHex(0x100000u+depth-1u);
            blockNameAppend(tags,footer.c_str(),cycle?blockNameInsert("E300",record.c_str(),"*T1"):
                Tags{{0,"VENDOR_ENTITY"},{5,"E300"},{330,record},{100,"AcDbEntity"},{8,"0"},{1,"opaque"}});
            dx_data failed;A::blockEndpointWorkLimit(failed,16u*1024u*1024u);
            if(!write(path,tags,binary)||!reader.fileImport(path.string(),&failed,false))return false;
            const auto r=dx_iface::tableContentBindingReport(failed);
            if(r.state!=(cycle?S::BlockReferenceCycle:S::BlockMemberUnsupported)||r.outputEligible||r.firstSourceBlock!=0x200000u+2u*(depth-1u))return false;
            dx_data retry;A::blockEndpointWorkLimit(retry,16u*1024u*1024u);
            if(!write(path,chain,binary)||!reader.fileImport(path.string(),&retry,false)||!valid(retry,depth+2u,depth+9u,depth+1u))return false;++deep;
        }
    }
    std::cout<<"Nested BLOCK name bindings: source routes="<<routes<<"; wire faults/retries="<<wireCases<<"; positive variants="<<variants
        <<"; exact/under budgets="<<quotas<<"; observed query allocation faults="<<allocations<<"; target-owner substitutions="<<owners
        <<"; 256-link tail failures/retries="<<deep<<"; default endpoint-work refusals="<<defaultCaptureLimits<<'\n';
    return routes==12u&&wireCases==cases.size()*routes&&variants==positives.size()*routes&&quotas==8u&&allocations>0&&owners==4u&&deep==4u&&defaultCaptureLimits==2u;
}
