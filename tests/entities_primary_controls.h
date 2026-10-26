// SPDX-License-Identifier: GPL-2.0-or-later
// Locally authored runtime recipes only. The separate scalar walker observes
// original transport/roles, not independent native CAD/kernel geometry semantics.
struct EntitiesPrimaryProbe final:dx_iface {
    int fault=-1;bool throwPoint=false,throwLine=false,premature=false;
    void addPoint(const DRW_Point& value) override {
        if(entitiesPrimaryReport(*cData).state==EntitiesPrimaryState::ProofsReady)premature=true;
        if(value.handle!=0x101u){dx_iface::addPoint(value);return;}
        auto& v=const_cast<DRW_Point&>(value);double* field=nullptr;
        switch(fault) {
        case 0:field=&v.basePoint.x;break;case 1:field=&v.basePoint.y;break;case 2:field=&v.basePoint.z;break;
        case 3:field=&v.thickness;break;case 4:field=&v.xAxisAngle;break;
        case 5:field=&v.extPoint.x;break;case 6:field=&v.extPoint.y;break;case 7:field=&v.extPoint.z;break;
        }
        const double original=field?*field:0.;if(field)*field+=0.125;
        try{dx_iface::addPoint(value);}catch(...){if(field)*field=original;throw;}
        if(field)*field=original;
        if(throwPoint)throw std::runtime_error("primary point callback");
    }
    void addLine(const DRW_Line& value) override {
        auto& v=const_cast<DRW_Line&>(value);double* field=nullptr;
        switch(fault) {
        case 8:field=&v.basePoint.x;break;case 9:field=&v.basePoint.y;break;case 10:field=&v.basePoint.z;break;
        case 11:field=&v.secPoint.x;break;case 12:field=&v.secPoint.y;break;case 13:field=&v.secPoint.z;break;
        case 14:field=&v.thickness;break;case 15:field=&v.extPoint.x;break;case 16:field=&v.extPoint.y;break;case 17:field=&v.extPoint.z;break;
        }
        const double original=field?*field:0.;if(field)*field+=0.125;
        try{dx_iface::addLine(value);}catch(...){if(field)*field=original;throw;}
        if(field)*field=original;
        if(throwLine)throw std::runtime_error("primary line callback");
    }
};

Tags entitiesPrimaryDocument(const std::string& version,unsigned form) {
    auto t=entitiesSourceDocument(version);
    if(form) {
        appendBlockFields(t,"101",{{39,"1.5"},{50,"90"},{210,"0.25"},{220,"0.5"},{230,"0.75"}});
        appendBlockFields(t,"102",{{39,"2.5"},{210,"0.125"},{220,"0.25"},{230,"0.5"}});
        for(const char* id:{"101","102","F201"}) {
            const auto at=blockField(t,id,100)+1u;
            t.insert(t.begin()+static_cast<std::ptrdiff_t>(at),{{6,"DASHED"},{62,"3"},{48,"0.75"},{370,"35"},{420,"1193046"},
                {430,"color-é"},{440,"33554559"},{60,"1"},{347,"C9"},{348,"CA"},{390,"CB"},{284,"2"},
                {102,"{ACAD_REACTORS"},{330,"F8"},{102,"}"},{102,"{ACAD_XDICTIONARY"},{360,"F9"},{102,"}"}});
            appendBlockFields(t,id,{{1001,"PRIMARY_APP"},{1003,"PRIMARY_LAYER"},{1005,"C9"},{1004,"A5FF"},
                {1010,"-0.0"},{1020,"2.25"},{1030,"-3.5"},{1070,"17"},{1071,"2147483647"}});
        }
    }
    if(form==2u) {
        // Exercise optional defaults and original scalar ordering, not a
        // canonical writer reconstruction. Move only within the same record.
        for(const char* id:{"101","102"}) {
            const auto at=blockField(t,id,10);const auto field=t[at];t.erase(t.begin()+static_cast<std::ptrdiff_t>(at));
            t.insert(t.begin()+static_cast<std::ptrdiff_t>(blockField(t,id,30)+1u),field);
        }
    }
    return t;
}

bool entitiesPrimaryWire(const std::filesystem::path& path,bool binary,const dx_data& data,std::size_t& spans,unsigned& roles) {
    const auto wire=attdefWireTags(path,binary);
    for(const auto& audit:DxIfaceTableSourceTestAccess::entitiesAudits(data))for(const auto& proof:audit->proofs) {
        if(!proof.primary||!proof.profilesMatched)continue;
        if(proof.next-proof.first!=1u||proof.profiles.size()!=1u)return false;
        const auto& p=proof.profiles[0];const auto& record=audit->physical->records[proof.first];
        if(p.spans.size()!=record.groups.size())return false;
        for(std::size_t j=0;j<p.spans.size();++j) {
            const auto& s=p.spans[j];const auto& physical=record.spans[j];
            if(s.groupIndex!=j||s.code!=record.groups[j].code()||!physical.ordinal||physical.ordinal>wire.size())return false;
            const auto& w=wire[physical.ordinal-1u];const auto& v=record.groups[j];
            if(w.code!=s.code||w.begin!=physical.beginOffset||w.end!=physical.endOffset)return false;
            if(w.kind==AttdefWireTag::Real) {const auto d=v.d_val();if(v.type()!=DRW_Variant::DOUBLE||std::memcmp(&d,&w.real,sizeof(d)))return false;}
            else if(w.kind==AttdefWireTag::Integer) {if(w.integer!=(v.type()==DRW_Variant::INTEGER64?v.i64_val():v.i_val()))return false;}
            else if(w.text!=dx_attdef_source::text(v))return false;
            using R=dx_attdef_source::Role;
            if(s.role==R::Self||s.role==R::Owner||s.role==R::Reactor||s.role==R::ExtensionDictionary||s.role==R::Material
                ||s.role==R::FullVisualStyle||s.role==R::PlotStyle||s.role==R::XdataObjectId||s.role==R::OpaqueHandle) {
                std::uint64_t h=0u;const auto converted=std::from_chars(w.text.data(),w.text.data()+w.text.size(),h,16);
                if(converted.ec!=std::errc{}||converted.ptr!=w.text.data()+w.text.size()||h!=s.handle)return false;++roles;
            }
            ++spans;
        }
    }
    return true;
}

bool entitiesPrimaryTests(const std::filesystem::path& dir) {
    struct Stage {const char* name="matrix";bool finished=false;~Stage(){if(!finished)std::cerr<<"ENTITIES primary stage="<<name<<'\n';}} stage;
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::EntitiesPrimaryState;using P=A::EntitiesCompoundProof::State;
    const auto path=dir/"entities-primaries.dxf";unsigned profiles=0u,mutations=0u,refusals=0u,variants=0u,limits=0u,contexts=0u,exceptions=0u,roles=0u,required=0u,dispositions=0u;
    std::size_t spans=0u;long allocations=0;
    const auto complete=[](const dx_data& data){const auto r=dx_iface::entitiesPrimaryReport(data);return r.state==S::ProofsReady&&r.primaries==3u
        &&r.matched==3u&&r.modeledMatched==3u&&r.profilesMatched==3u&&!r.outputEligible&&!r.ownerTargetsResolved;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0;encoding<3u;++encoding)for(unsigned form=0;form<3u;++form) {
        auto tags=entitiesPrimaryDocument(version,form);if(!writeBlockBinding(path,tags,encoding))return false;EntitiesPrimaryProbe reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||reader.premature||!complete(data)||!entitiesPrimaryWire(path,encoding==2u,data,spans,roles)) {
            const auto r=dx_iface::entitiesPrimaryReport(data);std::cerr<<"profile="<<version<<'/'<<encoding<<'/'<<form<<" state="<<static_cast<int>(r.state)<<" matched="<<r.matched<<'\n';
            for(const auto& a:A::entitiesAudits(data))for(const auto& p:a->proofs)if(p.primary) {
                std::cerr<<" dispatch="<<p.dispatch<<" state="<<static_cast<int>(p.state)<<'\n';
                for(const auto& f:p.profiles)if(!f.complete())std::cerr<<" reason="<<static_cast<int>(f.reason)<<" group="<<f.rejectedGroup<<'\n';
            }
            return false;
        }
        const auto& original=(*A::entitiesDeliveries(data)[0].observed)[0];const auto* point=dynamic_cast<const DRW_Point*>(original.model.get());
        const auto* line=dynamic_cast<const DRW_Line*>((*A::entitiesDeliveries(data)[0].observed)[1].model.get());
        if(!point||point->basePoint.y!=1.25||!std::signbit(point->basePoint.x)||point->basePoint.z!=2.5||!original.compoundStatesCaptured
            ||original.compoundStates.size()!=1u||(form&&(point->thickness!=1.5||point->xAxisAngle!=90./ARAD||point->extPoint.x!=0.25)))return false;
        if(!line||line->basePoint.x!=1.||line->basePoint.y!=2.||line->basePoint.z!=3.
            ||line->secPoint.x!=4.||line->secPoint.y!=5.||line->secPoint.z!=6.
            ||(form&&(line->thickness!=2.5||line->extPoint.x!=0.125||line->extPoint.y!=0.25||line->extPoint.z!=0.5)))return false;
        ++profiles;
    }
    struct Case {const char* name;std::function<void(Tags&)> change;P state;std::size_t dispatch=0u;bool importFails=false;};
    const std::vector<Case> invalid{
        {"unknown point field",[](auto& t){appendBlockFields(t,"101",{{90,"17"}});},P::ProfileMismatch},
        {"line point-only angle",[](auto& t){appendBlockFields(t,"102",{{50,"30"}});},P::ProfileMismatch,1u},
        {"point line-only end",[](auto& t){appendBlockFields(t,"101",{{11,"4"},{21,"5"}});},P::ProfileMismatch},
        {"duplicate coordinate",[](auto& t){appendBlockFields(t,"101",{{10,"1"}});},P::ProfileMismatch},
        {"wrong subclass",[](auto& t){auto at=blockField(t,"101",100);while(t[at].value!="AcDbPoint")++at;t[at].value="AcDbLine";},P::ProfileMismatch},
        {"point self owner",[](auto& t){t[blockField(t,"101",330)].value="101";},P::Owner},
        {"line zero owner",[](auto& t){t[blockField(t,"102",330)].value="0";},P::Identity,1u},
        {"duplicate root identity",[](auto& t){t[blockField(t,"F201",5)].value="101";},P::DuplicateIdentity},
        {"pending point XDATA",[](auto& t){appendBlockFields(t,"101",{{1001,"APP"},{1010,"1"}});},P::SourceStateMismatch},
        {"pending line XDATA",[](auto& t){appendBlockFields(t,"102",{{1001,"APP"},{1010,"1"},{1020,"2"}});},P::SourceStateMismatch,1u},
        {"invalid space",[](auto& t){t[blockField(t,"102",67)].value="2";},P::ProfileMismatch,1u,true}
    };
    for(bool binary:{false,true}) {
        const auto good=entitiesPrimaryDocument("AC1032",0u);if(!write(path,good,binary))return false;
        stage.name="field mutations";
        for(int field=0;field<18;++field) {
            EntitiesPrimaryProbe reader;reader.fault=field;dx_data data;
            if(!reader.fileImport(path.string(),&data,false))return false;const auto r=dx_iface::entitiesPrimaryReport(data);
            if(r.state!=S::Unqualified||r.matched!=2u||A::entitiesAudits(data)[0]->proofs[field<8?0u:1u].state!=P::ModelMismatch
                ||dx_iface::entitiesCompoundReport(data).state!=dx_iface::EntitiesCompoundState::ProofsReady)return false;
            reader.fault=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++mutations;
        }
        stage.name="source refusals";
        for(const auto& c:invalid) {
            auto tags=good;c.change(tags);if(!write(path,tags,binary))return false;EntitiesPrimaryProbe reader;dx_data data;
            const bool imported=reader.fileImport(path.string(),&data,false);
            if(c.importFails?imported||dx_iface::entitiesPrimaryReport(data).state!=S::ImportFailed
                :!imported||dx_iface::entitiesPrimaryReport(data).state!=S::Unqualified||A::entitiesAudits(data)[0]->proofs[c.dispatch].state!=c.state) {
                std::cerr<<"source refusal="<<c.name<<" state="<<static_cast<int>(dx_iface::entitiesPrimaryReport(data).state)<<'\n';return false;
            }
            if(!c.importFails&&c.state==P::SourceStateMismatch) {
                const auto& source=(*A::entitiesDeliveries(data)[0].observed)[c.dispatch];
                if(source.compoundStates.size()!=1u||source.compoundStates[0].hasNoPendingXdata()
                    ||!dxfCompound::Decoder::observe(*source.model).hasNoPendingXdata())return false;
            }
            if(!write(path,good,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++refusals;
        }
        stage.name="original variants";
        for(unsigned kind=0;kind<5u;++kind) {
            auto tags=good;
            if(kind==0u)appendBlockFields(tags,"101",{{230,"-1"}}); // omitted X/Y retain explicit defaults
            if(kind==1u)appendBlockFields(tags,"101",{{410,"Not a resolved layout"}});
            if(kind==2u) {tags[blockField(tags,"102",67)].value="0";}
            if(kind==3u) {appendBlockFields(tags,"101",{{102,"{OPAQUE"},{10,"9"},{20,"8"},{330,"DEAD"},{410,"not host"},{102,"}"}});}
            if(kind==4u) {appendBlockFields(tags,"101",{{1001,"APP"},{1010,"1"},{1020,"2"},{1030,"3"}});}
            if(!write(path,tags,binary))return false;EntitiesPrimaryProbe reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||!complete(data)||!entitiesPrimaryWire(path,binary,data,spans,roles)) {
                std::cerr<<"variant="<<kind<<" state="<<static_cast<int>(dx_iface::entitiesPrimaryReport(data).state)<<'\n';return false;
            }
            if(kind==3u) {
                const auto* p=dynamic_cast<const DRW_Point*>((*A::entitiesDeliveries(data)[0].observed)[0].model.get());
                if(!p||p->basePoint.y!=1.25||p->basePoint.x!=0.||!std::signbit(p->basePoint.x))return false;
            }
            ++variants;
        }
        stage.name="unchanged required-coordinate admission";
        for(unsigned field=0;field<6u;++field) {
            auto tags=good;const char* id=field<2u?"101":"102";
            const std::array<int,6> codes{{10,20,10,20,11,21}};
            tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,id,codes[field])));
            if(!write(path,tags,binary))return false;EntitiesPrimaryProbe reader;dx_data data;
            if(reader.fileImport(path.string(),&data,false)||dx_iface::entitiesPrimaryReport(data).state!=S::ImportFailed)return false;
            if(!write(path,good,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++required;
        }
        stage.name="unavailable and unselected bodies";
        {
            EntitiesOriginProbe reader;reader.richerPoint=true;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;
            const auto r=dx_iface::entitiesPrimaryReport(data);
            if(r.state!=S::Unqualified||r.unavailable!=1u||r.matched!=2u||r.outputEligible)return false;++dispositions;
        }
        for(unsigned kind=0;kind<4u;++kind) {
            Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1032"},{0,"ENDSEC"},{0,"SECTION"},{2,"ENTITIES"}};
            if(kind==1u)tags.insert(tags.end(),{{0,"VENDOR_ENTITY"},{5,"101"},{330,"1F"},{100,"AcDbEntity"},{8,"0"},{100,"VendorData"},{1,"opaque"}});
            if(kind==2u)tags.insert(tags.end(),{{0,"DIMENSION"},{5,"101"},{330,"1F"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbDimension"},{70,"7"}});
            if(kind==3u)tags.insert(tags.end(),{{0,"CIRCLE"},{5,"101"},{330,"1F"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbCircle"},{10,"1"},{20,"2"},{40,"3"}});
            tags.insert(tags.end(),{{0,"ENDSEC"},{0,"EOF"}});if(!write(path,tags,binary))return false;EntitiesPrimaryProbe reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false))return false;const auto r=dx_iface::entitiesPrimaryReport(data);
            if(r.state!=S::NoPrimaries||r.primaries||r.matched||r.outputEligible||r.ownerTargetsResolved)return false;++dispositions;
        }
        if(!write(path,good,binary))return false;EntitiesPrimaryProbe baseline;dx_data base;if(!baseline.fileImport(path.string(),&base,false)||!complete(base))return false;
        const auto measured=dx_iface::entitiesPrimaryReport(base);stage.name="allowances";
        for(unsigned dimension=0;dimension<3u;++dimension)for(bool below:{false,true}) {
            const auto cap=dimension==0u?measured.records:dimension==1u?measured.bytes:measured.work;
            EntitiesPrimaryProbe reader;dx_data data;A::entitiesCompoundLimit(data,dimension,cap-(below?1u:0u));
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::entitiesPrimaryReport(data).state!=(below?S::CaptureFailed:S::ProofsReady))return false;++limits;
        }
        stage.name="allocator controls";bool calibrated=false;
        for(long fail=0;fail<3000&&!calibrated;++fail) {
            {SymbolAllocationFailure schedule(fail);A::repeatEntitiesAudit(baseline,base);}
            const auto state=dx_iface::entitiesPrimaryReport(base).state;
            if(state==S::ProofsReady)calibrated=true;
            else {if(state!=S::CaptureFailed)return false;++allocations;}
        }
        if(!calibrated||!complete(base))return false;
        dx_data retry;if(!baseline.fileImport(path.string(),&retry,false)||!complete(retry))return false;
        stage.name="live edits and copied owners";
        for(auto* entity:base.mBlock->ent)if(auto* point=dynamic_cast<DRW_Point*>(entity))point->basePoint.x=777.;
        if(!complete(base))return false;++contexts;
        dx_data copied;A::copyBindingContext(copied,base);if(dx_iface::entitiesPrimaryReport(copied).state!=S::ContextMismatch)return false;++contexts;
        auto& audit=A::mutableEntitiesAudits(base)[0];const auto owner=audit;audit=std::make_shared<const std::decay_t<decltype(*owner)>>(*owner);
        if(dx_iface::entitiesPrimaryReport(base).state!=S::ContextMismatch)return false;audit=owner;if(!complete(base))return false;++contexts;
        stage.name="callback and tail exceptions";
        for(unsigned phase=0;phase<3u;++phase) {
            EntitiesPrimaryProbe reader;reader.throwPoint=phase==0u;reader.throwLine=phase==1u;dx_data data;bool threw=false;
            if(phase==2u){auto tags=good;tags.erase(tags.end()-2);if(!write(path,tags,binary))return false;}
            try{if(reader.fileImport(path.string(),&data,false))return false;}catch(const std::runtime_error&){threw=true;}
            if((phase<2u&&!threw)||dx_iface::entitiesPrimaryReport(data).state!=S::ImportFailed)return false;
            if(!write(path,good,binary))return false;reader.throwPoint=reader.throwLine=false;dx_data corrected;
            if(!reader.fileImport(path.string(),&corrected,false)||!complete(corrected))return false;++exceptions;
        }
    }
    stage.finished=true;
    std::cout<<"ENTITIES primary profiles="<<profiles<<" scalar spans="<<spans<<" reference roles="<<roles<<" field edits/retries="<<mutations
        <<" original refusals/retries="<<refusals<<" variants="<<variants<<" allowances="<<limits<<" allocation faults="<<allocations
        <<" contexts="<<contexts<<" exception/tail retries="<<exceptions<<" required-coordinate/retries="<<required<<" explicit dispositions="<<dispositions<<'\n';
    return profiles==36u&&spans>0u&&roles>0u&&mutations==36u&&refusals==invalid.size()*2u&&variants==10u&&limits==12u&&allocations>0&&contexts==6u&&exceptions==6u&&required==12u&&dispositions==10u;
}
