// SPDX-License-Identifier: GPL-2.0-or-later
// All documents below are generated from in-tree, locally authored tag recipes.
// The scalar walker is independent of the production/replay decoder. Neither
// recipe nor same-decoder replay is independent CAD/kernel semantic evidence.
struct EntitiesCompoundProbe final:dx_iface {
    int fault=-1,throwPhase=0;bool premature=false;
    void addPolyline(const DRW_Polyline& value) override {
        if(entitiesCompoundReport(*cData).state==EntitiesCompoundState::ProofsReady)premature=true;
        // Change just one actual callback field, preserving its original parser
        // cursors. A whole-model copy here would reset those cursors and test a
        // different gate before reaching modeled equality.
        auto& x=const_cast<DRW_Polyline&>(value);
        const auto undo=[&] {
            switch(fault) {
            case 0:x.basePoint.z-=1.;break;case 1:x.flags^=1;break;
            case 2:x.vertlist[0]->basePoint.x-=1.;break;case 3:x.vertlist[0]->bulge-=0.5;break;
            case 4:--x.vertlist[0]->identifier;break;case 5:x.defstawidth-=0.25;break;
            case 6:x.seqEndEntity->layer="0";break;
            }
        };
        switch(fault) {
        case 0:x.basePoint.z+=1.;break;case 1:x.flags^=1;break;
        case 2:x.vertlist[0]->basePoint.x+=1.;break;case 3:x.vertlist[0]->bulge+=0.5;break;
        case 4:++x.vertlist[0]->identifier;break;case 5:x.defstawidth+=0.25;break;
        case 6:x.seqEndEntity->layer="changed-footer";break;
        }
        try{dx_iface::addPolyline(value);}catch(...){undo();throw;}undo();
        if(throwPhase==1)throw std::runtime_error("ENTITIES compound callback fault");
    }
    void addInsert(const DRW_Insert& value) override {
        auto& x=const_cast<DRW_Insert&>(value);const auto name=x.name;
        const auto undo=[&] {
            switch(fault) {
            case 7:x.xscale-=1.;break;case 8:x.angle-=0.5;break;case 9:--x.colcount;break;
            case 10:x.name=name;break;case 11:x.attlist[0]->text="attribute-é";break;
            case 12:x.attlist[0]->attribFlags^=1;break;case 13:x.seqEndEntity->layer="0";break;
            }
        };
        switch(fault) {
        case 7:x.xscale+=1.;break;case 8:x.angle+=0.5;break;case 9:++x.colcount;break;
        case 10:x.name="changed-target";break;case 11:x.attlist[0]->text="changed-text";break;
        case 12:x.attlist[0]->attribFlags^=1;break;case 13:x.seqEndEntity->layer="changed-footer";break;
        }
        try{dx_iface::addInsert(value);}catch(...){undo();throw;}undo();
        if(throwPhase==2)throw std::runtime_error("ENTITIES insert callback fault");
    }
};

Tags entitiesCompoundDocument(const std::string& version,unsigned form) {
    auto tags=entitiesSourceDocument(version);if(!form)return tags;
    const auto blocks=blockBindingSource(version,std::min(form,4u));
    const auto first=slot(blocks,5,"D100")-1u,next=slot(blocks,5,"31")-1u;
    Tags children(blocks.begin()+static_cast<std::ptrdiff_t>(first),blocks.begin()+static_cast<std::ptrdiff_t>(next));
    bool paper=false;
    for(std::size_t i=0;i<children.size();++i) {
        if(children[i].code==0&&children[i].value=="INSERT")paper=true;
        if(children[i].code==330&&children[i].value=="B100")children[i].value=paper?"2F":"1F";
        if(paper&&children[i].code==100&&children[i].value=="AcDbEntity") {
            children.insert(children.begin()+static_cast<std::ptrdiff_t>(i+1u),{67,"1"});++i;
        }
    }
    const auto a=slot(tags,5,"D100")-1u,b=slot(tags,5,"F100")-1u;
    tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(a),tags.begin()+static_cast<std::ptrdiff_t>(b));
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(a),children.begin(),children.end());
    if(form==6u) {
        for(auto& tag:tags)if(tag.code==100&&tag.value=="AcDbBlockReference")tag.value="AcDbMInsertBlock";
        appendBlockFields(tags,"E100",{{70,"2"},{71,"3"},{44,"4"},{45,"5"}});
    }
    if(form>=5u) {
        tags.insert(tags.begin()+4,{{9,"$DWGCODEPAGE"},{3,"ANSI_932"}});
        // Form 5/6 use the embedded R2018 recipe, plus original common and XDATA
        // roles. This does not resolve these names/handles to actual targets.
        for(const char* id:{"D100","D101","D102","D103","E100","E101","E102"}) {
            const auto at=blockField(tags,id,100)+1u;
            tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{6,"DASHED"},{62,"3"},{48,"0.75"},{370,"35"},
                {420,"1193046"},{430,"color-é"},{440,"33554559"},{347,"C9"},{348,"CA"},{390,"CB"},
                {102,"{CUSTOM"},{330,"DEAD"},{410,"fake layout"},{102,"}"}});
            appendBlockFields(tags,id,{{1001,"BODY_APP"},{1003,"BODY_LAYER"},{1005,"C9"},
                {1010,"-0.0"},{1020,"2.25"},{1030,"-3.5"},{1070,"17"}});
        }
    }
    return tags;
}

bool entitiesCompoundWire(const std::filesystem::path& path,bool binary,const dx_data& data,std::size_t& spans) {
    const auto wire=attdefWireTags(path,binary);
    for(const auto& owner:DxIfaceTableSourceTestAccess::entitiesAudits(data))for(const auto& proof:owner->proofs) {
        if(!proof.compound||!proof.profilesMatched)continue;
        for(std::size_t i=proof.first;i<proof.next;++i) {
            const auto& record=owner->physical->records[i];const auto& profile=proof.profiles[i-proof.first];
            if(profile.spans.size()!=record.groups.size())return false;
            for(std::size_t j=0;j<profile.spans.size();++j) {
                const auto& s=profile.spans[j];if(s.groupIndex!=j||s.code!=record.groups[j].code())return false;
                const auto& physical=record.spans[j];if(!physical.ordinal||physical.ordinal>wire.size())return false;
                const auto& w=wire[physical.ordinal-1u];const auto& v=record.groups[j];
                if(w.code!=s.code||w.begin!=physical.beginOffset||w.end!=physical.endOffset)return false;
                if(w.kind==AttdefWireTag::Real) {const auto d=v.d_val();if(v.type()!=DRW_Variant::DOUBLE||std::memcmp(&d,&w.real,sizeof(d)))return false;}
                else if(w.kind==AttdefWireTag::Integer) {if(w.integer!=(v.type()==DRW_Variant::INTEGER64?v.i64_val():v.i_val()))return false;}
                else if(w.text!=dx_attdef_source::text(v))return false;
                using R=dx_attdef_source::Role;
                if(s.role==R::Self||s.role==R::Owner) {
                    std::uint64_t h=0u;const auto parsed=std::from_chars(w.text.data(),w.text.data()+w.text.size(),h,16);
                    if(parsed.ec!=std::errc{}||parsed.ptr!=w.text.data()+w.text.size()||h!=s.handle
                        ||h!=(s.role==R::Self?owner->members[i].self:owner->members[i].owner))return false;
                }
                ++spans;
            }
        }
    }
    return true;
}

bool entitiesReadContextControls(const std::filesystem::path& path,unsigned& contexts) {
    struct Consumer final:dx_iface {
        dxfRW* input=nullptr;unsigned calls=0u;bool good=true,change=true;
        DRW::DxfCodePagePolicy policy=DRW::DxfCodePagePolicy::ExactNamed;
        std::string codePage,declaration;
        void addHeader(const DRW_Header*) override {}
        void addPoint(const DRW_Point&) override {
            dxfRW::DxfReadContext c;good=good&&input->getDxfReadContext(c)&&c.sourceVersion==DRW::AC1032
                &&c.codecVersion==DRW::AC1021&&c.hasDeclaration&&c.declaration=="ANSI_932"&&c.codePagePolicy==policy
                &&c.textEncoding==DRW::DxfTextEncoding::Declared&&!c.binary&&!c.applyExtrusion;
            codePage=c.codecCodePage;declaration=c.declaration;++calls;
            if(change) {
                input->setDxfCodePagePolicy(DRW::DxfCodePagePolicy::Compatible);
                input->setDxfTextEncoding(DRW::DxfTextEncoding::LegacyLibreCadUtf8);
                dxfRW::DxfReadContext current;good=good&&input->getDxfReadContext(current)&&current.codePagePolicy==policy
                    &&current.textEncoding==DRW::DxfTextEncoding::Declared;
            }
        }
    } consumer;
    Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1032"},{9,"$DWGCODEPAGE"},{3,"ANSI_932"},{0,"ENDSEC"},
        {0,"SECTION"},{2,"ENTITIES"},{0,"POINT"},{5,"101"},{8,"0"},{10,"1"},{20,"2"},{0,"ENDSEC"},{0,"EOF"}};
    std::ostringstream out;for(const auto& t:tags)out<<t.code<<'\n'<<t.value<<'\n';auto document=out.str();
    dxfRW reader(path.string().c_str());consumer.input=&reader;dxfRW::DxfReadContext context;context.binary=true;context.declaration="stale";
    if(reader.getDxfReadContext(context)||context.binary||!context.declaration.empty())return false;++contexts;
    reader.setDxfCodePagePolicy(consumer.policy);
    if(!reader.readAscii(&consumer,false,document)||!consumer.good||consumer.calls!=1u||consumer.codePage.empty()||consumer.declaration!="ANSI_932")return false;
    if(reader.getDxfReadContext(context)||context.sourceVersion!=DRW::UNKNOWNV||!context.codecCodePage.empty())return false;++contexts;
    consumer.change=false;consumer.policy=DRW::DxfCodePagePolicy::Compatible;
    reader.setDxfTextEncoding(DRW::DxfTextEncoding::Declared);
    if(!reader.readAscii(&consumer,false,document)||!consumer.good||consumer.calls!=2u)return false;++contexts;
    return true;
}

bool entitiesCompoundTests(const std::filesystem::path& dir) {
    struct Stage {const char* name="matrix";bool finished=false;~Stage(){if(!finished)std::cerr<<"ENTITIES compound stage="<<name<<'\n';}} stage;
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::EntitiesCompoundState;using P=A::EntitiesCompoundProof::State;
    const auto path=dir/"entities-compounds.dxf";unsigned profiles=0u,refusals=0u,mutations=0u,limits=0u,contexts=0u,exceptions=0u,variants=0u,emptyChains=0u,dispositions=0u;
    std::size_t spans=0u;long allocations=0;
    const auto complete=[](const dx_data& d){const auto r=dx_iface::entitiesCompoundReport(d);return r.state==S::ProofsReady&&r.compounds==2u&&r.matched==2u&&!r.outputEligible&&!r.ownerTargetsResolved;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0;encoding<3u;++encoding)for(unsigned form=0;form<7u;++form) {
        auto tags=entitiesCompoundDocument(version,form);if(!writeBlockBinding(path,tags,encoding))return false;
        EntitiesCompoundProbe reader;dx_data data;const bool imported=reader.fileImport(path.string(),&data,false);
        const auto r=dx_iface::entitiesCompoundReport(data);const auto expected=form>=4u&&version!="AC1032"?S::Unqualified:S::ProofsReady;
        if(!imported||reader.premature||r.state!=expected||r.sections!=2u||r.compounds!=2u||r.otherPrimaries!=3u||r.raw!=1u||r.noCallback!=1u
            ||r.outputEligible||r.ownerTargetsResolved||!entitiesCompoundWire(path,encoding==2u,data,spans)) {
            std::cerr<<"profile="<<version<<'/'<<encoding<<'/'<<form<<" import="<<imported<<" state="<<static_cast<int>(r.state)<<" compounds="<<r.compounds<<" matches="<<r.matched<<'\n';
            for(const auto& audit:A::entitiesAudits(data)) {
                for(const auto& p:audit->proofs)if(p.compound) {
                    std::cerr<<" dispatch="<<p.dispatch<<" state="<<static_cast<int>(p.state)<<" modeled="<<p.modeledMatched<<" profiles="<<p.profilesMatched<<" record="<<p.failedRecord<<'\n';
                    for(const auto& a:p.profiles)if(!a.complete())std::cerr<<" profile reason="<<static_cast<int>(a.reason)<<" group="<<a.rejectedGroup<<'\n';
                }
            }
            return false;
        }
        const auto& observed=*A::entitiesDeliveries(data)[0].observed;
        const auto& in=observed[3];if(!in.compoundStatesCaptured||!in.compoundStates[1].attributeSubclass
            ||dxfCompound::Decoder::observe(*static_cast<const DRW_Insert*>(in.model.get())->attlist[0]).attributeSubclass)return false;
        ++profiles;
    }
    stage.name="empty completed chains and missing footers";
    for(const std::string version:{"AC1015","AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0;encoding<3u;++encoding) {
        auto tags=entitiesSourceDocument(version);eraseBlockRecord(tags,"D101");eraseBlockRecord(tags,"D102");
        if(!writeBlockBinding(path,tags,encoding))return false;EntitiesCompoundProbe reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false))return false;
        const DRW_Polyline* empty=nullptr;for(const auto* e:data.mBlock->ent)if(const auto* pl=dynamic_cast<const DRW_Polyline*>(e))empty=pl;
        if(!empty||!empty->vertlist.empty()||!empty->seqEndEntity||empty->seqEndEntity->handle!=0xD103u
            ||(version!="AC1015"&&!complete(data)))return false;
        auto broken=tags;eraseBlockRecord(broken,"D103");if(!writeBlockBinding(path,broken,encoding))return false;dx_data failed;
        if(reader.fileImport(path.string(),&failed,false))return false;
        if(!writeBlockBinding(path,tags,encoding))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false))return false;++emptyChains;
    }
    stage.name="live context API";if(!entitiesReadContextControls(path,contexts))return false;
    const auto common=[](Tags& tags,const char* id,Tags fields) {
        const auto at=blockField(tags,id,100)+1u;
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),fields.begin(),fields.end());
    };
    struct Case {const char* name;std::function<void(Tags&)> change;P state;std::size_t dispatch=2u;};
    const std::vector<Case> invalid{
        {"root self owner",[](auto& t){t[blockField(t,"D100",330)].value="D100";},P::Owner},
        {"child owner",[](auto& t){t[blockField(t,"D101",330)].value="1F";},P::Owner},
        {"footer owner",[](auto& t){t[blockField(t,"E102",330)].value="1F";},P::Owner,3u},
        {"zero root owner",[](auto& t){t[blockField(t,"D100",330)].value="0";},P::Identity},
        {"duplicate vertex",[](auto& t){t[blockField(t,"D102",5)].value="D101";},P::DuplicateIdentity},
        {"unknown vertex field",[](auto& t){appendBlockFields(t,"D101",{{90,"123"}});},P::ProfileMismatch},
        {"partial root XDATA",[](auto& t){appendBlockFields(t,"D100",{{1001,"APP"},{1010,"1"}});},P::SourceStateMismatch},
        {"partial vertex XDATA",[](auto& t){appendBlockFields(t,"D101",{{1001,"APP"},{1010,"1"},{1020,"2"}});},P::SourceStateMismatch},
        {"child paper only",[common](auto& t){common(t,"D101",{{67,"1"}});},P::SpaceMismatch},
        {"root layout only",[common](auto& t){common(t,"D100",{{410,"Model"}});},P::LayoutUnresolved},
        {"conflicting layouts",[common](auto& t){for(const char* h:{"D100","D101","D102","D103"})common(t,h,{{410,h==std::string("D101")?"Paper":"Model"}});},P::SpaceMismatch},
        {"duplicate layout",[common](auto& t){common(t,"D100",{{410,"Model"},{410,"Model"}});},P::ProfileMismatch}
    };
    for(bool binary:{false,true}) {
        const auto good=entitiesSourceDocument("AC1032");
        stage.name="callback field edits";
        if(!write(path,good,binary))return false;
        for(int field=0;field<14;++field) {
            EntitiesCompoundProbe reader;reader.fault=field;dx_data data;
            if(!reader.fileImport(path.string(),&data,false))return false;
            const auto r=dx_iface::entitiesCompoundReport(data);const auto& p=A::entitiesAudits(data)[0]->proofs[field<7?2u:3u];
            if(r.state!=S::Unqualified||r.matched!=1u||p.state!=P::ModelMismatch) {
                std::cerr<<"field="<<field<<" state="<<static_cast<int>(r.state)<<" proof="<<static_cast<int>(p.state)<<'\n';return false;
            }
            reader.fault=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++mutations;
        }
        stage.name="original source refusals";
        for(const auto& c:invalid) {
            auto tags=good;c.change(tags);if(!write(path,tags,binary))return false;EntitiesCompoundProbe reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::entitiesCompoundReport(data).state!=S::Unqualified
                ||A::entitiesAudits(data)[0]->proofs[c.dispatch].state!=c.state) {
                std::cerr<<"refusal="<<c.name<<" report="<<static_cast<int>(dx_iface::entitiesCompoundReport(data).state);
                if(!A::entitiesAudits(data).empty())std::cerr<<" proof="<<static_cast<int>(A::entitiesAudits(data)[0]->proofs[c.dispatch].state);
                std::cerr<<'\n';return false;
            }
            if(!write(path,good,binary))return false;dx_data retry;
            if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++refusals;
        }
        stage.name="positive linked variants";
        for(unsigned kind=0;kind<7u;++kind) {
            auto tags=good;
            if(kind==0u)for(const char* h:{"D100","D101","D102","D103"})common(tags,h,{{410,"Model"}});
            if(kind==1u)for(const char* h:{"E100","E101","E102"})common(tags,h,{{410,"Layout 2"}});
            if(kind==2u)for(const char* h:{"D100","D101","D102","D103"})common(tags,h,{{102,"{CUSTOM"},{410,"not a host layout"},{102,"}"}});
            if(kind==3u) {eraseBlockRecord(tags,"D101");eraseBlockRecord(tags,"D102");}
            if(kind==4u) {eraseBlockRecord(tags,"E101");eraseBlockRecord(tags,"E102");tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,"E100",66)));}
            if(kind==5u) {eraseBlockRecord(tags,"E101");}
            if(kind==6u) {tags[blockField(tags,"D103",330)].value="1F";tags[blockField(tags,"E102",330)].value="2F";}
            if(!write(path,tags,binary))return false;EntitiesCompoundProbe reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||!complete(data)||!entitiesCompoundWire(path,binary,data,spans)) {
                std::cerr<<"linked variant="<<kind<<" state="<<static_cast<int>(dx_iface::entitiesCompoundReport(data).state)<<'\n';return false;
            }++variants;
        }
        if(!write(path,good,binary))return false;EntitiesCompoundProbe baseline;dx_data base;
        if(!baseline.fileImport(path.string(),&base,false)||!complete(base))return false;
        stage.name="exact logical allowances";const auto measured=dx_iface::entitiesCompoundReport(base);
        const std::array<std::size_t,3> caps{{measured.records,measured.bytes,measured.work}};
        for(unsigned which=0;which<3u;++which)for(bool below:{false,true}) {
            EntitiesCompoundProbe reader;dx_data data;A::entitiesCompoundLimit(data,which,caps[which]-(below?1u:0u));
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::entitiesCompoundReport(data).state!=(below?S::CaptureFailed:S::ProofsReady)
                ||dx_iface::entitiesDeliveryReport(data).state!=dx_iface::EntitiesDeliveryState::ObservedTransportComplete) {
                std::cerr<<"allowance="<<which<<'/'<<below<<" state="<<static_cast<int>(dx_iface::entitiesCompoundReport(data).state)<<'\n';return false;
            }++limits;
        }
        stage.name="audit allocation faults";bool calibrated=false;
        for(long failure=0;failure<2000&&!calibrated;++failure) {
            {SymbolAllocationFailure schedule(failure);A::repeatEntitiesAudit(baseline,base);}
            const auto state=dx_iface::entitiesCompoundReport(base).state;
            if(state==S::ProofsReady)calibrated=true;
            else {
                if(state!=S::CaptureFailed) {std::cerr<<"allocation="<<failure<<" state="<<static_cast<int>(state)<<'\n';return false;}
                ++allocations;
            }
        }
        if(!calibrated||!complete(base))return false;
        dx_data allocationRetry;EntitiesCompoundProbe retryReader;
        if(!retryReader.fileImport(path.string(),&allocationRetry,false)||!complete(allocationRetry))return false;
        stage.name="pinned cache and read-context owners";
        auto& audit=A::mutableEntitiesAudits(base)[0];const auto original=audit;
        audit=std::make_shared<const std::decay_t<decltype(*original)>>(*original);
        if(dx_iface::entitiesCompoundReport(base).state!=S::ContextMismatch)return false;audit=original;++contexts;
        audit=std::shared_ptr<const std::decay_t<decltype(*original)>>(original.get(),[](const auto*){});
        if(dx_iface::entitiesCompoundReport(base).state!=S::ContextMismatch)return false;audit=original;++contexts;
        auto& row=A::mutableEntitiesDeliveries(base)[0];const auto context=row.context;
        row.context=std::make_shared<const std::decay_t<decltype(*context)>>(*context);
        if(dx_iface::entitiesCompoundReport(base).state!=S::ContextMismatch)return false;row.context=context;++contexts;
        if(!complete(base))return false;
        dx_data copy;A::copyBindingContext(copy,base);if(dx_iface::entitiesCompoundReport(copy).state!=S::ContextMismatch)return false;++contexts;
        for(auto* entity:base.mBlock->ent)if(auto* pl=dynamic_cast<DRW_Polyline*>(entity))pl->vertlist.front()->basePoint.x=777.;
        base.headerC.addStr("$DWGCODEPAGE","ANSI_1252",3);
        if(!complete(base))return false;++contexts;
        stage.name="scope-local codec context";
        {
            auto tags=good;tags.insert(tags.begin()+4,{{9,"$DWGCODEPAGE"},{3,"ANSI_932"}});
            const auto at=slot(tags,2,"CUSTOM_SECTION")-1u;
            tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1032"},
                {9,"$DWGCODEPAGE"},{3,"ANSI_1252"},{0,"ENDSEC"},{0,"SECTION"},{2,"ENTITIES"},{0,"ENDSEC"}});
            if(!write(path,tags,binary))return false;EntitiesCompoundProbe reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false,DRW::DxfCodePagePolicy::ExactNamed)||!complete(data))return false;
            const auto& rows=A::entitiesDeliveries(data);
            if(rows.size()!=3u||rows[0].context->declaration!="ANSI_932"||rows[1].context->declaration!="ANSI_932"
                ||rows[2].context->declaration!="ANSI_1252"||rows[0].context->codePagePolicy!=DRW::DxfCodePagePolicy::ExactNamed
                ||data.headerC.dxfCodePageDeclaration()!="ANSI_1252")return false;
            ++contexts;
        }
        if(!write(path,good,binary))return false;
        stage.name="unsupported body subtypes";
        for(bool polyline:{false,true}) {
            EntitiesOriginProbe reader;reader.richerPolyline=polyline;reader.richerInsert=!polyline;dx_data data;
            if(!reader.fileImport(path.string(),&data,false))return false;const auto r=dx_iface::entitiesCompoundReport(data);
            if(r.state!=S::Unqualified||r.unavailable!=1u||r.matched!=1u||r.outputEligible)return false;++dispositions;
        }
        stage.name="owner release";
        {
            using Audit=std::decay_t<decltype(*original)>;using Context=std::decay_t<decltype(*context)>;
            std::weak_ptr<const Audit> weakAudit;std::weak_ptr<const Context> weakContext;EntitiesCompoundProbe reader;
            {dx_data data;if(!reader.fileImport(path.string(),&data,false)||!complete(data))return false;
                weakAudit=A::entitiesAudits(data)[0];weakContext=A::entitiesDeliveries(data)[0].context;}
            if(!weakAudit.expired()||!weakContext.expired())return false;++contexts;
        }
        stage.name="callback exceptions and failed tail";
        for(int phase=1;phase<=2;++phase) {
            EntitiesCompoundProbe reader;reader.throwPhase=phase;dx_data data;bool threw=false;
            try{(void)reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
            if(!threw||dx_iface::entitiesCompoundReport(data).state!=S::ImportFailed||!A::entitiesAudits(data).empty())return false;
            reader.throwPhase=0;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++exceptions;
        }
        for(unsigned kind=0;kind<2u;++kind) {
            auto tags=good;if(!kind)tags.resize(slot(tags,5,"F201")+1u);else tags.erase(tags.end()-2);
            if(!write(path,tags,binary))return false;EntitiesCompoundProbe reader;dx_data data;
            if(reader.fileImport(path.string(),&data,false)||dx_iface::entitiesCompoundReport(data).state!=S::ImportFailed||!A::entitiesAudits(data).empty())return false;
            if(!write(path,good,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++exceptions;
        }
    }
    stage.finished=true;
    std::cout<<"ENTITIES compound profiles="<<profiles<<" spans="<<spans<<" contexts="<<contexts<<" mutations="<<mutations
        <<" refusals="<<refusals<<" limits="<<limits<<" allocations="<<allocations<<" exceptions="<<exceptions<<" variants="<<variants
        <<" empty chain/retries="<<emptyChains<<" unqualified subtypes="<<dispositions<<'\n';
    return profiles==84u&&spans>0u&&mutations==28u&&refusals==invalid.size()*2u&&limits==12u&&allocations>0&&exceptions==8u&&variants==14u&&emptyChains==15u&&dispositions==4u;
}
