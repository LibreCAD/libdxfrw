// SPDX-License-Identifier: GPL-2.0-or-later
// Runtime-authored original ATTDEF body/pair/role evidence, not dependency,
// edited/output, native consumer or geometry qualification.
Tags attdefRoleBody(const char* id,const char* owner,bool paper,const char* layout,unsigned mode) {
    Tags t{{0,"ATTDEF"},{5,id},{330,owner},{100,"AcDbEntity"}};
    if(paper)t.insert(t.end(),{{67,"1"},{410,layout}});
    t.insert(t.end(),{{8,"host-layer"},{102,"{OPAQUE"},{330,"DEAD"},{410,"false-layout"},{67,"1"},{1,"false-text"},{102,"}"},
        {100,"AcDbText"},{10,"1.25"},{20,"2.5"},{30,"3.75"},{40,"4.5"},{1,"default-é"},{7,"STANDARD"},
        {11,"5"},{21,"6"},{31,"7"},{50,"15"},{210,"0"},{220,"1"},{230,"0"}});
    if(mode)t.insert(t.end(),{{mode==1u?101:100,"Embedded Object"},{10,"10"},{20,"20"},{30,"30"},{40,"2.5"},{41,"12"},{46,"7"},
        {3,"FIRST_"},{1,"LAST-é"},{11,"0"},{21,"1"},{31,"0"},{71,"4"},
        {102,"{NESTED"},{330,"CAFE"},{410,"not-a-layout"},{67,"0"},{1,"not-mtext"},{102,"}"}});
    t.insert(t.end(),{{100,"AcDbAttributeDefinition"},{3,"prompt-é"},{2,"TAG"},{70,"7"},{73,"5"},{74,"2"},{280,"1"},
        {1001,"APP"},{1003,"XDATA_LAYER"},{1005,"40"},{1010,"8"},{1020,"9"},{1030,"10"},{1070,"11"}});return t;
}
Tags attdefRoleDocument(const std::string& version,bool blocks,unsigned mode) {
    auto t=blocks?blockRoleDocument(version):entitiesRoleDocument(version);
    const auto add=[&](const char* id,const char* owner,bool paper,const char* layout,const char* end){const auto body=attdefRoleBody(id,owner,paper,layout,mode);
        const auto at=blocks?slot(t,5,end)-1u:slot(t,0,"EOF")-1u;t.insert(t.begin()+static_cast<std::ptrdiff_t>(at),body.begin(),body.end());};
    add("501",blocks?"B3":"B0",false,"", "F7");add("502",blocks?"B2":"B1",true,blocks?"Sheet 2":"Sheet 1","F5");return t;
}
bool attdefRolesReady(const dx_data& d,bool blocks) {
    if(blocks){const auto r=dx_iface::blockRoleReport(d);return r.state==dx_iface::BlockRoleState::RolesReady&&r.records==21u&&r.selected==21u&&r.matched==21u
        &&r.physicalCensusComplete&&r.selectedRolesMatched&&!r.wholeDocumentMatched&&!r.ownerTargetsResolved&&!r.outputEligible;}
    const auto r=dx_iface::entitiesRoleReport(d);return r.state==dx_iface::EntitiesRoleState::SelectedRolesReady&&r.selected==11u&&r.matched==11u&&r.primaries==6u&&r.children==5u
        &&r.selectedRolesMatched&&!r.wholeDocumentMatched&&!r.ownerTargetsResolved&&!r.outputEligible;
}
struct AttdefRoleProbe final:dx_iface {
    bool sourceSelected=true;unsigned sourceFault=0u,typedFault=0u,depth=0u;int mutation=-1;long allocation=-1,observedAllocations=0;
    bool throwSource=false,throwTyped=false,allocationReached=false;
    bool wantsDxfAttdefSource() const override{return sourceSelected&&dx_iface::wantsDxfAttdefSource();}
    void addBlock(const DRW_Block& value) override{++depth;dx_iface::addBlock(value);}
    void endBlock() override{if(depth)--depth;dx_iface::endBlock();}
    void addDxfAttdefSource(const DRW_Attdef& value,const DRW_RawDxfObject& raw,const DRW_DxfAttdefSourceContext& c) override {
        if(value.handle!=0x501u){dx_iface::addDxfAttdefSource(value,raw,c);return;}
        if(sourceFault==1u){const DRW_Attdef copy(value);const DRW_RawDxfObject copiedRaw(raw);dx_iface::addDxfAttdefSource(copy,copiedRaw,c);}
        else if(sourceFault!=2u)dx_iface::addDxfAttdefSource(value,raw,c);
        if(sourceFault==3u)dx_iface::addDxfAttdefSource(value,raw,c);
        if(throwSource)throw std::runtime_error("ATTDEF role source callback");
    }
    void addAttDef(const DRW_Attdef& original) override {
        if(original.handle!=0x501u){dx_iface::addAttDef(original);return;}
        if(allocation>=0){const auto before=symbolFailedAllocations;SymbolAllocationFailure fault(allocation);dx_iface::addAttDef(original);
            observedAllocations=allocation-symbolAllocationsBeforeFailure;allocationReached=symbolFailedAllocations!=before;return;}
        if(typedFault==1u){const DRW_Attdef copy(original);dx_iface::addAttDef(copy);return;}
        if(typedFault==2u)return;
        auto& value=const_cast<DRW_Attdef&>(original);std::string* text=nullptr;double* real=nullptr;
        if(mutation==0)text=&value.text;if(mutation==1)text=&value.prompt;if(mutation==2)text=&value.tag;if(mutation==5)text=&value.style;
        if(mutation==7&&value.mtext)text=&value.mtext->text;
        if(mutation==3)real=&value.basePoint.x;if(mutation==4)real=&value.height;
        if(mutation==8&&value.mtext)real=&value.mtext->widthscale;if(mutation==9&&value.mtext)real=&value.mtext->secPoint.y;
        std::string savedText;double savedReal=0.;const auto flags=value.attribFlags;
        if(text){savedText=*text;*text="changed actual callback field";}if(real){savedReal=*real;*real+=1.;}if(mutation==6)value.attribFlags^=1u;
        dx_iface::addAttDef(value);
        if(text)text->swap(savedText);if(real)*real=savedReal;value.attribFlags=flags;
        if(typedFault==3u)dx_iface::addAttDef(value);
        if(throwTyped)throw std::runtime_error("ATTDEF role typed callback");
    }
    void clearFaults(){sourceSelected=true;sourceFault=typedFault=0u;mutation=-1;allocation=-1;throwSource=throwTyped=false;}
};
// This walker checks original bytes/positions and reference roles independently
// of the production ATTDEF decoder, source profile and common-role extractor.
template<class Physical>
bool attdefRoleWire(const std::vector<AttdefWireTag>& wire,const Physical& p,const dx_attdef_source::Audit& profile,std::size_t& spans) {
    using namespace dx_attdef_source;if(!profile.complete()||p.groups.size()!=profile.spans.size())return false;
    std::vector<std::size_t> attribute280;std::size_t marker=p.groups.size();unsigned application=0u;bool attribute=false;
    for(std::size_t i=0u;i<p.groups.size();++i) {
        const auto& span=p.spans[i];if(!span.ordinal||span.ordinal>wire.size())return false;const auto& tag=wire[span.ordinal-1u];
        if(span.ignoredComment)continue;
        if(tag.code==102){if(tag.text=="}"){if(!application)return false;--application;}else ++application;continue;}
        if(application)continue;
        if(tag.code==100){attribute=tag.text=="AcDbAttributeDefinition"||tag.text=="AcDbAttribute";if(attribute)marker=i;}
        if(attribute&&tag.code==280)attribute280.push_back(i);
    }
    const bool dual=attribute280.size()==2u;
    if(dual) {
        const auto first=attribute280[0],last=attribute280[1];auto next=marker+1u;
        while(next<p.spans.size()&&p.spans[next].ignoredComment)++next;
        const auto& version=wire[p.spans[first].ordinal-1u];const auto& lock=wire[p.spans[last].ordinal-1u];
        if(first!=next||version.kind!=AttdefWireTag::Integer||version.integer!=0||lock.kind!=AttdefWireTag::Integer||lock.integer<0||lock.integer>1)return false;
    }
    bool nested=false;unsigned depth=0u;
    for(std::size_t i=0;i<p.groups.size();++i){const auto& s=p.spans[i];const auto& v=p.groups[i];const auto& a=profile.spans[i];
        if(!s.ordinal||s.ordinal>wire.size())return false;const auto& w=wire[s.ordinal-1u];
        if(a.groupIndex!=i||a.code!=w.code||v.code()!=w.code||s.beginOffset!=w.begin||s.endOffset!=w.end)return false;
        if(s.ignoredComment){if(a.scope!=Scope::Ignored||a.field!=Field::IgnoredComment)return false;++spans;continue;}
        if(w.kind==AttdefWireTag::Integer){if((v.type()!=DRW_Variant::INTEGER&&v.type()!=DRW_Variant::INTEGER64)||(v.type()==DRW_Variant::INTEGER64?v.i64_val():v.i_val())!=w.integer)return false;}
        else if(w.kind==AttdefWireTag::Real){const auto real=v.d_val();if(v.type()!=DRW_Variant::DOUBLE||std::memcmp(&w.real,&real,sizeof(real)))return false;}
        else if(v.type()!=DRW_Variant::STRING||!v.content.s||*v.content.s!=w.text)return false;
        Role role=Role::None;
        if(w.code==102){if(w.text=="}"){if(!depth)return false;--depth;}else ++depth;if(a.field!=Field::Application)return false;}
        else if(depth){if((w.code>=320&&w.code<=369)||w.code==5||w.code==1005)role=Role::OpaqueHandle;if(a.field!=Field::Application)return false;}
        else {
            if((w.code==100||w.code==101)&&w.text=="Embedded Object")nested=true;
            if(w.code==100&&w.text=="AcDbAttributeDefinition")nested=false;
            switch(w.code){case 5:role=Role::Self;break;case 330:role=Role::Owner;break;case 8:role=Role::LayerName;break;
            case 7:role=Role::TextStyleName;break;case 410:role=Role::LayoutName;break;case 1001:role=Role::AppIdName;break;
            case 1003:role=Role::XdataLayerName;break;case 1005:role=Role::XdataObjectId;break;default:break;}
        }
        if(a.scope!=(nested?Scope::Nested:Scope::Host)||a.role!=role)return false;
        if(w.code==280&&!depth&&a.scope==Scope::Host) {
            const auto expected=dual&&i==attribute280.front()?Field::ReservedDefault:Field::Attribute;
            if(a.field!=expected)return false;
        }
        if(role==Role::Self||role==Role::Owner||role==Role::OpaqueHandle||role==Role::XdataObjectId)if(a.handle!=std::stoull(w.text,nullptr,16))return false;
        ++spans;
    }return depth==0u;
}
std::array<std::size_t,2> attdefDualRoleSlots(const Tags& tags,const char* id) {
    const auto begin=slot(tags,5,id);unsigned depth=0u;bool attribute=false;std::array<std::size_t,2> found{{tags.size(),tags.size()}};unsigned count=0u;
    for(std::size_t i=begin;i<tags.size()&&(i==begin||tags[i].code!=0);++i) {
        const auto& field=tags[i];if(field.code==102){if(field.value=="}"){if(!depth)throw std::runtime_error("dual280 role application depth");--depth;}else ++depth;continue;}
        if(depth)continue;if(field.code==100)attribute=field.value=="AcDbAttributeDefinition";
        if(attribute&&field.code==280){if(count==2u)throw std::runtime_error("dual280 role source count");found[count++]=i;}
    }
    if(count!=2u)throw std::runtime_error("dual280 role source slots");return found;
}
Tags attdefDualRoleDocument(const std::string& version,bool blocks,bool lock,bool binary) {
    auto tags=attdefRoleDocument(version,blocks,0u);
    for(const char* id:{"501","502"}) {
        const auto begin=slot(tags,5,id);std::size_t marker=tags.size(),last=tags.size();unsigned depth=0u;bool attribute=false;
        for(std::size_t i=begin;i<tags.size()&&(i==begin||tags[i].code!=0);++i) {
            const auto& field=tags[i];if(field.code==102){if(field.value=="}")--depth;else ++depth;continue;}if(depth)continue;
            if(field.code==100){attribute=field.value=="AcDbAttributeDefinition";if(attribute)marker=i;}
            if(attribute&&field.code==280)last=i;
        }
        if(marker==tags.size()||last==tags.size())throw std::runtime_error("dual280 role authored body");tags[last].value=lock?"1":"0";
        if(!binary)tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(last),{999,"ignored original lock-span boundary"});
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(marker+1u),{280,"0"});
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(marker+2u),{{102,"{VERSION_SHADOW"},{280,"7"},{3,"not-prompt"},{2,"NOT_TAG"},{70,"15"},{102,"}"}});
    }
    // The enclosing source contains a genuine INSERT/ATTRIB/SEQEND chain.
    // Its ATTRIB has no prompt; its opaque shadows cannot provide required fields.
    const char* child=blocks?"207":"E101";
    auto marker=blockField(tags,child,100);while(tags[marker].value!="AcDbAttribute")++marker;
    auto end=marker+1u;while(tags[end].code!=0)++end;
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),{280,lock?"1":"0"});
    if(!binary)tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),{999,"original ATTRIB lock span"});
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(marker+1u),{{280,"0"},{102,"{ATTR_VERSION_SHADOW"},
        {280,"7"},{3,"not-prompt"},{2,"NOT_TAG"},{70,"15"},{102,"}"}});
    return tags;
}
bool attdefDualRoleWriter(const std::filesystem::path& path,DRW::Version version,bool binary,bool lock) {
    // Canonical writer normalization only: this clean authored model is not a
    // conversion/closure certificate for the original role fixture document.
    struct Output final:dx_iface {
        dx_data data;DRW_Attdef value;
        explicit Output(bool locked){cData=&data;currentBlock=data.mBlock;value.handle=0xBEEFu;value.tag="DUAL_TAG";value.prompt="version zero";value.text="default";
            value.height=1;value.basePoint={1,2,3};value.attVersion=0;value.lockPosition=locked;}
        void writeEntities()override{(void)dxfW->writeAttdef(&value,DRW::DxfModelSpaceBlockRecordHandle);}
    } output(lock);
    dxfRW writer(path.string().c_str());output.dxfW=&writer;if(!writer.write(&output,version,binary)||output.value.attVersion!=0||output.value.lockPosition!=lock) { return false; }
    const auto wire=attdefWireTags(path,binary);bool selected=false,attribute=false;unsigned fields=0u,records=0u,owners=0u;
    for(const auto& field:wire){if(field.code==0){selected=field.text=="ATTDEF";attribute=false;if(selected)++records;continue;}if(!selected)continue;
        if(field.code==100)attribute=field.text=="AcDbAttributeDefinition";
        if(field.code==330){if(std::stoul(field.text,nullptr,16)!=DRW::DxfModelSpaceBlockRecordHandle) { return false; }++owners;}
        if(field.code==280){if(!attribute||field.kind!=AttdefWireTag::Integer||field.integer!=int(lock)) { return false; }++fields;}}
    if(records!=1u||owners!=1u||fields!=1u) { return false; }dx_iface reader;dx_data actual;if(!reader.fileImport(path.string(),&actual,false)) { return false; }unsigned matched=0u;
    for(const auto* entity:actual.mBlock->ent)if(const auto* value=dynamic_cast<const DRW_Attdef*>(entity)) {
        if(value->attVersion!=0||value->lockPosition!=lock||value->tag!="DUAL_TAG"||value->prompt!="version zero") { return false; }++matched;
    }
    return matched==1u;
}
bool attdefDual280RoleTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;using namespace dx_attdef_source;
    const auto path=dir/"attdef-dual280-roles.dxf";unsigned profiles=0u,corruptions=0u,refusals=0u,retries=0u,writerForms=0u,compoundChildren=0u;std::size_t spans=0u;
    const auto ready=[&](const std::string& version,bool binary,bool blocks,bool lock,AttdefRoleProbe& reader,dx_data& data) {
        const auto failed=[&](const char* stage,std::uint32_t id=0u){
            std::cerr<<"ATTDEF dual280 role stage="<<stage<<" version="<<version<<" binary="<<binary<<" blocks="<<blocks<<" lock="<<lock<<" id="<<id
                <<" B="<<static_cast<int>(dx_iface::blockRoleReport(data).state)<<" E="<<static_cast<int>(dx_iface::entitiesRoleReport(data).state)<<'\n';
            for(const auto& item:A::resources(data))if(item.first==0x501u||item.first==0x502u){const auto& r=item.second;
                std::cerr<<" resource="<<item.first<<" pair="<<r.attdefPairMatched<<" fields="<<r.attdefFieldsMatched<<" profile="<<static_cast<int>(r.attdefProfile.reason)
                    <<" rejected="<<r.attdefProfile.rejectedGroup<<" lock="<<(r.attdefModel?r.attdefModel->lockPosition:false)<<'\n';}
            return false;
        };
        if(!write(path,attdefDualRoleDocument(version,blocks,lock,binary),binary))return failed("authoring");
        if(!reader.fileImport(path.string(),&data,false))return failed("ordinary import");
        if(!attdefRolesReady(data,blocks))return failed("enclosing role report");
        const auto wire=attdefWireTags(path,binary);
        for(auto id:{0x501u,0x502u}) {
            const auto& r=A::mapResource(data,id);
            if(!r.attdefPairMatched||!r.attdefFieldsMatched||r.attdefFieldMismatch||r.rawTypedMatched||!r.attdefModel||r.attdefModel->attVersion!=0
                ||r.attdefModel->lockPosition!=lock||r.attdefModel->mtext||!r.attdefState.attributeSubclass)return failed("original pair/model",id);
            // The generic resource profile deliberately refuses owning layout410.
            // The enclosing physical role audit qualifies that separate context.
            if(id==0x501u){if(!r.attdefProfile.complete()||r.attdefProfile.form!=Form::Classic)return failed("strict model-space resource profile",id);}
            else if(r.attdefProfile.complete()||r.attdefProfile.reason!=Reason::UnknownField||!r.attdefProfile.spans.empty()
                ||!r.raw||r.attdefProfile.rejectedGroup>=r.raw->groups.size()||r.raw->groups[r.attdefProfile.rejectedGroup].code()!=410)
                return failed("strict paper-space resource boundary",id);
            const auto inspect=[&](const auto& physical,const auto& audit) {
                if(!audit.complete()||audit.form!=Form::Classic)return failed("layout-qualified physical profile",id);
                unsigned versions=0u,locks=0u;for(const auto& field:audit.spans)if(field.code==280&&field.scope==Scope::Host) {
                    if(field.groupIndex>=physical.groups.size())return failed("physical field index",id);
                    if(field.field==Field::ReservedDefault){if(physical.groups[field.groupIndex].i_val()!=0)return failed("version literal",id);++versions;}
                    else if(field.field==Field::Attribute){if(physical.groups[field.groupIndex].i_val()!=int(lock))return failed("lock literal",id);++locks;}
                }
                if(versions!=1u||locks!=1u)return failed("physical dual280 field census",id);
                if(!attdefRoleWire(wire,physical,audit,spans))return failed("independent original wire spans",id);
                for(const auto field:{Field::ReservedDefault,Field::Attribute}) {
                    auto changed=audit;bool found=false;for(auto& tag:changed.spans)if(tag.code==280&&tag.field==field){tag.field=field==Field::Attribute?Field::ReservedDefault:Field::Attribute;found=true;break;}
                    std::size_t ignored=0u;if(!found||attdefRoleWire(wire,physical,changed,ignored))return failed("independent role mutation sensitivity",id);
                    ++corruptions;
                }
                return true;
            };
            bool found=false;
            if(blocks)for(const auto& source:A::blockSources(data))for(const auto& item:source.second.compoundBodies) {
                const auto& body=*item.second;if(body.model->handle!=id)continue;
                const auto range=body.membership->ranges[item.first];if(found||!body.profilesComplete||body.profiles.size()!=1u||body.attdef.model!=r.attdefModel
                    ||!inspect(body.membership->children->records[range.first],body.profiles.front())) { return false; }found=true;
            }
            else for(const auto& audit:A::entitiesAudits(data))for(const auto& body:audit->proofs)if(audit->members[body.first].self==id) {
                if(found||!body.attributeDefinition||!body.modeledMatched||!body.profilesMatched||body.profiles.size()!=1u||body.attdef.model!=r.attdefModel
                    ||!inspect(audit->physical->records[body.first],body.profiles.front())) { return false; }found=true;
            }
            if(!found)return failed("missing enclosing body proof",id);
        }
        const std::uint32_t parent=blocks?0x206u:0xE100u,child=blocks?0x207u:0xE101u,footer=blocks?0x208u:0xE102u;
        const auto inspectChild=[&](const auto& physical,const auto& profile,const auto& member,const auto& parentMember,const auto& footerMember) {
            if(physical.name!="ATTRIB"||member.self!=child||member.owner!=parent||member.role!=dx_block_membership::Role::Attribute
                ||parentMember.self!=parent||footerMember.self!=footer||footerMember.owner!=parent||footerMember.role!=dx_block_membership::Role::SequenceEnd
                ||member.parent!=parentMember.parent||footerMember.parent!=parentMember.parent||member.dispatch!=parentMember.dispatch||footerMember.dispatch!=parentMember.dispatch
                ||member.reason!=dx_block_membership::Reason::Matched||parentMember.reason!=dx_block_membership::Reason::Matched||footerMember.reason!=dx_block_membership::Reason::Matched
                ||!profile.complete()||!attdefRoleWire(wire,physical,profile,spans))return failed("original INSERT/ATTRIB/SEQEND membership and wire",child);
            unsigned versions=0u,locks=0u;for(const auto& field:profile.spans)if(field.code==280&&field.scope==Scope::Host) {
                if(field.field==Field::ReservedDefault){if(physical.groups[field.groupIndex].i_val()!=0)return failed("ATTRIB version literal",child);++versions;}
                else if(field.field==Field::Attribute){if(physical.groups[field.groupIndex].i_val()!=int(lock))return failed("ATTRIB lock literal",child);++locks;}
            }
            if(versions!=1u||locks!=1u)return failed("ATTRIB exact ordered dual roles",child);
            for(const auto field:{Field::ReservedDefault,Field::Attribute}) {
                auto changed=profile;bool found=false;for(auto& span:changed.spans)if(span.code==280&&span.field==field){span.field=field==Field::Attribute?Field::ReservedDefault:Field::Attribute;found=true;break;}
                std::size_t ignored=0u;if(!found||attdefRoleWire(wire,physical,changed,ignored))return failed("ATTRIB same-code role corruption sensitivity",child);
                ++corruptions;
            }
            return true;
        };
        bool childFound=false;
        if(blocks)for(const auto& source:A::blockSources(data))for(const auto& item:source.second.compoundBodies) {
            const auto& body=*item.second;const auto* model=dynamic_cast<const DRW_Insert*>(body.model.get());if(!model||model->handle!=parent)continue;
            const auto range=body.membership->ranges[item.first];
            if(childFound||body.state!=decltype(body.state)::ModeledReplayMatched||!body.profilesComplete||body.profiles.size()!=3u||range.next-range.first!=3u||model->attlist.size()!=1u||!model->attlist[0]
                ||model->attlist[0]->handle!=child||model->attlist[0]->parentHandle!=parent||model->attlist[0]->lockPosition!=lock||model->attlist[0]->attVersion!=0
                ||!model->seqEndEntity||model->seqEndEntity->handle!=footer||model->seqEndEntity->parentHandle!=parent
                ||!inspectChild(body.membership->children->records[range.first+1u],body.profiles[1],body.membership->members[range.first+1u],
                    body.membership->members[range.first],body.membership->members[range.first+2u]))return failed("frozen BLOCKS attribute compound",child);
            childFound=true;
        }
        else for(const auto& audit:A::entitiesAudits(data))for(const auto& body:audit->proofs)if(audit->members[body.first].self==parent) {
            if(childFound||!body.compound||!body.modeledMatched||!body.profilesMatched||body.profiles.size()!=3u||body.next-body.first!=3u
                ||!inspectChild(audit->physical->records[body.first+1u],body.profiles[1],audit->members[body.first+1u],audit->members[body.first],audit->members[body.first+2u]))
                return failed("frozen ENTITIES attribute compound",child);
            childFound=true;
        }
        if(!childFound)return failed("missing original attribute compound",child);
        ++compoundChildren;
        unsigned ordinary=0u,ordinaryChildren=0u;const auto live=[&](const dx_ifaceBlock* block){
            if(block)for(const auto* entity:block->ent)if(const auto* value=dynamic_cast<const DRW_Attdef*>(entity)) {
                if(value->handle==0x501u||value->handle==0x502u){if(value->attVersion!=0||value->lockPosition!=lock) return false;++ordinary;}
            }
            if(block)for(const auto* entity:block->ent)if(const auto* value=dynamic_cast<const DRW_Insert*>(entity))if(value->handle==parent) {
                if(value->attlist.size()!=1u||!value->attlist[0]||value->attlist[0]->handle!=child||value->attlist[0]->parentHandle!=parent
                    ||value->attlist[0]->attVersion!=0||value->attlist[0]->lockPosition!=lock||!value->seqEndEntity||value->seqEndEntity->handle!=footer
                    ||value->seqEndEntity->parentHandle!=parent)return false;
                ++ordinaryChildren;
            }
            return true;
        };
        if(!live(data.mBlock)) { return false; }for(const auto* block:data.blocks)if(!live(block)) { return false; }
        return (ordinary==2u&&ordinaryChildren==1u)||failed("ordinary retained callback count");
    };
    for(const std::string version:{"AC1024","AC1027","AC1032"})for(bool binary:{false,true})for(bool blocks:{false,true})for(bool lock:{false,true}) {
        AttdefRoleProbe reader;dx_data data;if(!ready(version,binary,blocks,lock,reader,data)) { return false; }++profiles;
    }
    // Count mutation sensitivity once per positive source, not again on retries.
    const auto positiveCorruptions=corruptions;
    for(const std::string version:{"AC1024","AC1027","AC1032"})for(bool binary:{false,true})for(bool blocks:{false,true})for(unsigned kind=0u;kind<10u;++kind) {
        auto tags=attdefDualRoleDocument(version,blocks,true,binary);const auto at=attdefDualRoleSlots(tags,"501");
        if(kind==0u)tags[at[0]].value="1";if(kind==1u)tags[at[0]].value="-1";if(kind==2u)tags[at[1]].value="2";
        if(kind==3u){const auto version=tags[at[0]];tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at[0]));
            unsigned depth=0u;std::size_t prompt=tags.size();const auto first=slot(tags,5,"501");
            for(std::size_t i=first;i<tags.size()&&(i==first||tags[i].code!=0);++i){if(tags[i].code==102){depth=tags[i].value=="}"?depth-1u:depth+1u;continue;}
                if(!depth&&tags[i].code==3){prompt=i;break;}}
            if(prompt==tags.size())throw std::runtime_error("dual280 owning prompt missing");tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(prompt+1u),version);}
        if(kind==4u)tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at[0]+1u),{280,"0"});
        if(kind==5u)tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at[1]+1u),{280,"1"});
        if(kind==6u){unsigned depth=0u;const auto first=slot(tags,5,"501");for(std::size_t i=first;i<tags.size()&&(i==first||tags[i].code!=0);++i){if(tags[i].code==102){depth=tags[i].value=="}"?depth-1u:depth+1u;continue;}
            if(!depth&&tags[i].code==70){tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(i));break;}}}
        if(kind==7u)tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at[1]),{{100,"AcDbXrecord"},{280,"0"}});
        if(kind==8u){tags[at[0]].value="1";tags[at[1]].value="0";}
        if(kind==9u)tags[slot(tags,9,"$ACADVER")+1u].value="AC1021";
        if(!write(path,tags,binary)) { return false; }AttdefRoleProbe reader;dx_data data;(void)reader.fileImport(path.string(),&data,false);
        if(attdefRolesReady(data,blocks)){std::cerr<<"ATTDEF dual280 malformed role accepted kind="<<kind<<" binary="<<binary<<" blocks="<<blocks<<'\n';return false;}
        ++refusals;reader.clearFaults();dx_data corrected;
        if(!ready(version,binary,blocks,true,reader,corrected)) { return false; }++retries;
    }
    for(const auto version:{DRW::AC1024,DRW::AC1027,DRW::AC1032})for(bool binary:{false,true})for(bool lock:{false,true}) {
        if(!attdefDualRoleWriter(dir/"attdef-dual280-canonical-output.dxf",version,binary,lock)){std::cerr<<"ATTDEF canonical writer form failed binary="<<binary<<" lock="<<lock<<'\n';return false;}
        ++writerForms;
    }
    std::cout<<"ATTDEF dual280 roles profiles="<<profiles<<"; independent scalar spans="<<spans<<"; positive role corruptions="<<positiveCorruptions
        <<"; original compound children="<<compoundChildren<<"; malformed/retries="<<refusals<<'/'<<retries<<"; canonical single-lock writer forms="<<writerForms<<'\n';
    return profiles==24u&&positiveCorruptions==144u&&compoundChildren==144u&&refusals==120u&&retries==120u&&writerForms==12u;
}

bool attdefRoleTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;struct Stage{const char* value="profiles";bool done=false;~Stage(){if(!done)std::cerr<<"ATTDEF roles stage="<<value<<'\n';}}stage;
    if(!attdefDual280RoleTests(dir))return false;
    const auto path=dir/"attdef-roles.dxf";unsigned profiles=0u,qualified=0u,revisions=0u,variants=0u,mutations=0u,pairs=0u,contexts=0u,quotas=0u,failures=0u,allocations=0u,liveCopyFailures=0u,destinationGuards=0u;std::size_t spans=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true})for(unsigned mode=0u;mode<3u;++mode){
        if(!writeBlockBinding(path,attdefRoleDocument(version,blocks,mode),encoding))return false;dx_iface reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false))return false;const bool admitted=!mode||version=="AC1032";
        if(attdefRolesReady(data,blocks)!=admitted){std::cerr<<version<<'/'<<encoding<<" blocks="<<blocks<<" mode="<<mode<<" B="<<static_cast<int>(dx_iface::blockRoleReport(data).state)<<" E="<<static_cast<int>(dx_iface::entitiesRoleReport(data).state)<<'\n';
            if(A::blockRole(data))for(const auto& r:A::blockRole(data)->rows)if(r.self==0x501u||r.self==0x502u)std::cerr<<r.self<<" reason="<<static_cast<int>(r.reason)<<'\n';
            if(A::entitiesRole(data))for(const auto& r:A::entitiesRole(data)->rows)if(r.self==0x501u||r.self==0x502u)std::cerr<<r.self<<" reason="<<static_cast<int>(r.reason)<<'\n';return false;}
        const auto wire=attdefWireTags(path,encoding==2u);
        for(auto id:{0x501u,0x502u}){const auto& resource=A::mapResource(data,id);
            if(!resource.attdefPairMatched||!resource.attdefFieldsMatched||resource.attdefFieldMismatch||resource.rawTypedMatched||!resource.attdefState.attributeSubclass
                ||dxfAttdef::Decoder::observe(*resource.attdefModel).attributeSubclass||!resource.attdefReadContext||resource.attdefReadContext->binary!=(encoding==2u)
                ||resource.attdefModel->prompt!="prompt-é"||resource.attdefModel->tag!="TAG"||resource.attdefModel->text!="default-é"
                ||(mode&&(!resource.attdefModel->mtext||resource.attdefModel->mtext->text!="FIRST_LAST-é")))return false;
            if(blocks){const auto& row=A::blockSources(data).at(id==0x501u?0xF6u:0xF4u);bool found=false;
                for(const auto& b:row.compoundBodies) {if(b.second->model->handle==id){const auto& body=*b.second;const auto range=body.membership->ranges[b.first];
                    if(body.state!=decltype(body.state)::ModeledReplayMatched||body.profilesComplete!=admitted||body.attdef.model!=resource.attdefModel||body.sourceStates.size()!=1u
                        ||!body.sourceStates.front().attributeSubclass||!body.replayStates.front().attributeSubclass)return false;
                    if(admitted&&!attdefRoleWire(wire,body.membership->children->records[range.first],body.profiles.front(),spans))return false;found=true;}}
                if(!found)return false;
            }else{bool found=false;for(const auto& a:A::entitiesAudits(data)){for(const auto& p:a->proofs){if(a->members[p.first].self==id){
                    if(!p.attributeDefinition||p.primary||p.compound||!p.modeledMatched||p.profilesMatched!=admitted||p.attdef.model!=resource.attdefModel)return false;
                    if(admitted&&!attdefRoleWire(wire,a->physical->records[p.first],p.profiles.front(),spans))return false;found=true;}}}
                if(!found)return false;}
        }
        ++profiles;if(admitted)++qualified;else ++revisions;
    }
    for(bool binary:{false,true})for(bool blocks:{false,true}) {
        const auto good=attdefRoleDocument("AC1032",blocks,1u);
        const auto retry=[&](AttdefRoleProbe& reader){reader.clearFaults();dx_data next;return write(path,good,binary)&&reader.fileImport(path.string(),&next,false)&&!reader.depth&&attdefRolesReady(next,blocks);};
        stage.value="original field/default/context variants";
        for(unsigned kind=0u;kind<(binary?7u:8u);++kind){auto tags=good;
            if(kind==0u)tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,"502",410)));
            if(kind==1u){const auto at=blockField(tags,"501",100)+1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{67,"0"}});}
            if(kind==2u)tags[blockField(tags,"502",410)].value=blocks?"sheet 2":"sheet 1";
            if(kind==3u)tags[blockField(tags,"502",410)].value=blocks?"\\U+0053heet 2":"\\U+0053heet 1";
            if(kind==4u)tags[blockField(tags,"501",10)].value="-0.0";
            if(kind==5u){const auto at=slot(tags,0,"ENDSEC");tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{9,"$DWGCODEPAGE"},{3,"ANSI_1252"}});}
            if(kind==7u)for(const char* id:{"501","502"}){const auto at=slot(tags,5,id);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{999,"original ignored comment"}});}
            if(!write(path,tags,binary))return false;AttdefRoleProbe reader;dx_data data;
            const auto policy=kind==6u?DRW::DxfCodePagePolicy::ExactNamed:DRW::DxfCodePagePolicy::Compatible;
            if(!reader.fileImport(path.string(),&data,false,policy)||!attdefRolesReady(data,blocks)
                ||A::mapResource(data,0x501u).attdefReadContext->codePagePolicy!=policy){std::cerr<<"variant="<<kind<<" blocks="<<blocks<<'\n';return false;}++variants;
        }
        stage.value="field and source role refusals with corrected imports";
        for(unsigned kind=0u;kind<8u;++kind){auto tags=good;
            if(kind==0u)tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,"502",67)));
            if(kind==1u)tags[blockField(tags,"502",410)].value="Model";
            if(kind==2u)tags[blockField(tags,"501",330)].value="DEAD";
            if(kind==3u){const auto at=blockField(tags,"501",100)+1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{410,blocks?"Model":"Sheet 1"}});}
            if(kind==4u){const auto at=blockField(tags,"501",100)+1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{67,"0"},{67,"0"}});}
            if(kind==5u){const auto at=blockField(tags,"501",100)+1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{90,"1"}});}
            if(kind==6u)tags[blockField(tags,"501",40)].value="-1";
            if(kind==7u){const auto at=blockField(tags,"501",100)+1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{100,"AcDbXrecord"}});}
            if(!write(path,tags,binary))return false;AttdefRoleProbe reader;dx_data data;(void)reader.fileImport(path.string(),&data,false);
            if(attdefRolesReady(data,blocks)||!retry(reader)){std::cerr<<"refusal="<<kind<<" blocks="<<blocks<<'\n';return false;}++failures;
        }
        if(!write(path,good,binary))return false;
        stage.value="actual callback field mutations and pair substitutions";
        for(int kind=0;kind<10;++kind){AttdefRoleProbe reader;reader.mutation=kind;dx_data data;(void)reader.fileImport(path.string(),&data,false);
            if(attdefRolesReady(data,blocks)||!retry(reader)){std::cerr<<"mutation="<<kind<<" blocks="<<blocks<<'\n';return false;}++mutations;}
        for(unsigned kind=0u;kind<7u;++kind){AttdefRoleProbe reader;
            if(kind==0u)reader.sourceSelected=false;else if(kind<=3u)reader.sourceFault=kind;else reader.typedFault=kind-3u;
            dx_data data;(void)reader.fileImport(path.string(),&data,false);
            if(attdefRolesReady(data,blocks)||!retry(reader)){std::cerr<<"pair="<<kind<<" blocks="<<blocks<<'\n';return false;}++pairs;}
        stage.value="immutable owner and pair prerequisites";
        for(unsigned which=0u;which<(blocks?4u:3u);++which)for(bool alias:{false,true}){AttdefRoleProbe reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||!attdefRolesReady(data,blocks))return false;A::substituteAttdef(data,0x501u,which,alias);
            const bool mismatch=blocks?dx_iface::blockRoleReport(data).state==dx_iface::BlockRoleState::ContextMismatch:dx_iface::entitiesRoleReport(data).state==dx_iface::EntitiesRoleState::ContextMismatch;
            if(!mismatch||!retry(reader)){std::cerr<<"owner="<<which<<" blocks="<<blocks<<'\n';return false;}++contexts;}
        for(unsigned which=0u;which<4u;++which){AttdefRoleProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;auto& r=A::mutableResource(data,0x501u);
            if(which==0u)r.attdefPairMatched=false;if(which==1u)r.attdefFieldsMatched=false;if(which==2u)r.awaitingAttdef=true;if(which==3u)r.attdefState.attributeSubclass=false;
            if(attdefRolesReady(data,blocks)||!retry(reader))return false;++contexts;}
        {AttdefRoleProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;
            const auto edit=[](dx_ifaceBlock* b){if(b)for(auto* e:b->ent)if(auto* a=dynamic_cast<DRW_Attdef*>(e)){a->prompt="edited";a->parentHandle=0x40u;if(a->mtext)a->mtext->text="edited nested";}};
            edit(data.mBlock);for(auto* b:data.blocks)edit(b);data.layouts.front().name="edited live layout";
            if(!attdefRolesReady(data,blocks))return false;{SymbolAllocationFailure fault(0);if(!attdefRolesReady(data,blocks))return false;}++contexts;
            dx_data copy;A::copyBindingContext(copy,data);if(attdefRolesReady(copy,blocks))return false;++contexts;}
        stage.value="bounded body and enclosing role audit";
        AttdefRoleProbe baseline;dx_data base;if(!baseline.fileImport(path.string(),&base,false)||!attdefRolesReady(base,blocks))return false;
        const auto usage=A::attdefBodyUsage(base,blocks);
        for(unsigned which=0u;which<3u;++which)for(bool under:{false,true}){AttdefRoleProbe reader;dx_data data;A::attdefBodyLimit(data,blocks,which,usage[which]-(under?1u:0u));
            if(!reader.fileImport(path.string(),&data,false)||attdefRolesReady(data,blocks)==under){std::cerr<<"quota="<<which<<" under="<<under<<" blocks="<<blocks<<'\n';return false;}++quotas;}
        for(bool work:{false,true})for(bool under:{false,true}){const auto cap=blocks?(work?dx_iface::blockRoleReport(base).work:dx_iface::blockRoleReport(base).bytes):(work?dx_iface::entitiesRoleReport(base).work:dx_iface::entitiesRoleReport(base).bytes);
            AttdefRoleProbe reader;dx_data data;if(blocks)A::blockRoleLimit(data,work,cap-(under?1u:0u));else A::entitiesRoleLimit(data,work,cap-(under?1u:0u));
            if(!reader.fileImport(path.string(),&data,false)||attdefRolesReady(data,blocks)==under)return false;++quotas;}
        stage.value="observed typed callback allocation faults";
        AttdefRoleProbe calibrated;calibrated.allocation=(std::numeric_limits<long>::max)();dx_data measured;
        if(!calibrated.fileImport(path.string(),&measured,false)||!attdefRolesReady(measured,blocks)||calibrated.observedAllocations<32)return false;
        std::vector<long> schedule;for(long n=0;n<16;++n)schedule.push_back(n);
        for(long n=1;n<=8;++n)schedule.push_back(calibrated.observedAllocations*n/8-1);
        std::sort(schedule.begin(),schedule.end());schedule.erase(std::unique(schedule.begin(),schedule.end()),schedule.end());
        for(long fault:schedule){AttdefRoleProbe reader;reader.allocation=fault;dx_data data;const bool read=reader.fileImport(path.string(),&data,false);
            const bool ready=attdefRolesReady(data,blocks);
            if(ready){
                // ENTITIES observes original producer values before cloning the
                // editable entity. A later editable-copy failure must not erase
                // real original proof, nor may it authorize output.
                if(blocks||!read||!reader.allocationReached||!data.typedEntityCaptureFailed
                    ||dx_iface::entitiesCompoundReport(data).state!=dx_iface::EntitiesCompoundState::ProofsReady)return false;
                const auto destination=dir/"attdef-allocation-destination.dxf";
                if(!write(destination,{{0,"EOF"}},false))return false;
                const auto bytes=[&](){std::ifstream input(destination,std::ios::binary);return std::string(std::istreambuf_iterator<char>(input),{});};
                const auto original=bytes();
                for(bool strict:{false,true}) {
                    if(reader.fileExport(destination.string(),DRW::AC1032,binary,&data,false,strict,
                        strict?dx_dxfSourceTextPolicy::RequireValidSource:dx_dxfSourceTextPolicy::Compatible)||bytes()!=original)return false;
                    ++destinationGuards;
                }
                ++liveCopyFailures;
            }
            const bool recovered=retry(reader);
            if(!reader.allocationReached||!recovered){std::cerr<<"allocation="<<fault<<" blocks="<<blocks<<" observed="<<reader.observedAllocations<<" calibrated="<<calibrated.observedAllocations
                <<" read="<<read<<" ready="<<ready<<" recovered="<<recovered<<" typedFailure="<<data.typedEntityCaptureFailed
                <<" B="<<static_cast<int>(dx_iface::blockRoleReport(data).state)<<" E="<<static_cast<int>(dx_iface::entitiesRoleReport(data).state)<<'\n';return false;}++allocations;}
        stage.value="whole-import callback and tail recovery";
        for(unsigned kind=0u;kind<3u;++kind){auto tags=good;if(kind==2u)tags.pop_back();if(!write(path,tags,binary))return false;AttdefRoleProbe reader;reader.throwSource=kind==0u;reader.throwTyped=kind==1u;
            dx_data data;bool read=false,threw=false;try{read=reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
            if(read||threw!=(kind!=2u)||reader.depth||attdefRolesReady(data,blocks)||!retry(reader))return false;++failures;}
    }
    stage.done=true;std::cout<<"ATTDEF roles profiles="<<profiles<<"; qualified="<<qualified<<"; revision refusals="<<revisions<<"; independent scalar spans="<<spans
        <<"; variants="<<variants<<"; actual field mutations="<<mutations<<"; pair refusals="<<pairs<<"; context guards="<<contexts<<"; exact/under quotas="<<quotas<<"; allocation faults="<<allocations
        <<"; original proof with later live-copy failure="<<liveCopyFailures<<"; destination guards="<<destinationGuards<<"; failure/retries="<<failures<<'\n';
    return profiles==72u&&qualified==36u&&revisions==36u&&spans>0u&&mutations==40u&&pairs==28u&&quotas==40u&&allocations>=64u&&liveCopyFailures>0u&&destinationGuards==2u*liveCopyFailures&&failures==44u;
}
