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
        if(role==Role::Self||role==Role::Owner||role==Role::OpaqueHandle||role==Role::XdataObjectId)if(a.handle!=std::stoull(w.text,nullptr,16))return false;
        ++spans;
    }return depth==0u;
}
bool attdefRoleTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;struct Stage{const char* value="profiles";bool done=false;~Stage(){if(!done)std::cerr<<"ATTDEF roles stage="<<value<<'\n';}}stage;
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
