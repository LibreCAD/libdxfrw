// Runtime-authored observation controls; never a source-namespace or graph claim.
bool legacyIdentityObservationTests(const std::filesystem::path& path) {
    using A=DxIfaceTableSourceTestAccess;
    using Source=DRW_DxfIdentitySource;
    struct Probe final:dx_iface {
        bool copy=false,duplicate=false,duplicateTail=false,throwAfter=false,checkActive=false,reenter=false;
        unsigned lateMutation=10u;long allocation=-1;std::filesystem::path inputPath;
        DRW_DxfIdentitySourceLimits limits;
        DRW_DxfIdentitySourceLimits dxfIdentitySourceLimits()const override{return limits;}
        void addDxfIdentitySource(const Source& source)override {
            if(copy){auto other=source;dx_iface::addDxfIdentitySource(other);}
            else if(allocation>=0){SymbolAllocationFailure failure(allocation);dx_iface::addDxfIdentitySource(source);}
            else dx_iface::addDxfIdentitySource(source);
            if(duplicate)dx_iface::addDxfIdentitySource(source);
            if(duplicateTail)dx_iface::addDxfIdentitySource(source);
            if(checkActive&&enableLegacyDxfIdentityObservation(false))throw std::runtime_error("active enable accepted");
            if(lateMutation<10u)A::mutateLegacyIdentity(*cData,lateMutation);
            if(reenter&&fileImport(inputPath.string(),cData,false))throw std::runtime_error("reentrant import accepted");
            if(throwAfter)throw std::runtime_error("legacy identity callback exception");
        }
    };
    unsigned positive=0,defaults=0,quota=0,refusals=0,retries=0,late=0;
    const auto gated=[](const dx_data& data){return !A::identityInput(data)&&!A::identityAudit(data)
        &&dx_iface::identitySourceReport(data).state!=dx_iface::IdentitySourceState::NamespaceReady
        &&!dx_iface::identitySourceReport(data).outputEligible&&!dx_iface::identitySourceReport(data).ownerTargetsResolved;};
    for(bool binary:{false,true}) {
        auto document=identitySourceDocument("AC1018");
        if(!write(path,document,binary))return false;
        Probe def;dx_data optout;A::identityLimit(optout,false,0u);A::identityLimit(optout,true,0u);
        if(!def.fileImport(path.string(),&optout,false)||A::legacyIdentity(optout)||A::legacyIdentityFailed(optout)||!gated(optout))return false;++defaults;
        Probe selected;if(!selected.enableLegacyDxfIdentityObservation())return false;selected.checkActive=true;dx_data data;
        if(!selected.fileImport(path.string(),&data,false)||A::legacyIdentityFailed(data)||!gated(data))return false;
        const auto& captured=A::legacyIdentity(data);
        if(!captured||captured->container!=&data||captured->generation!=data.importGeneration
            ||!captured->containerId||!captured->source||!captured->context||!captured->context->captured
            ||captured->context->sourceVersion!=DRW::AC1018||captured->context->codecVersion!=DRW::AC1015
            ||captured->context->codecCodePage!="ANSI_932"||captured->context->declaration!="ANSI_932"
            ||!captured->context->hasDeclaration||captured->context->binary!=binary
            ||captured->source->version!=DRW::AC1018||captured->source->binary!=binary
            ||captured->source->status!=Source::Status::Complete||!captured->source->eofMarker.ordinal
            ||captured->source->records.size()!=15u||captured->source->candidates.size()!=18u)return false;
        bool control=false,point=false,opaque=false;
        for(const auto& c:captured->source->candidates) {
            if(c.record>=captured->source->records.size())return false;
            const auto& row=captured->source->records[c.record];
            if(c.lexeme=="10"&&row.kind==Source::Kind::TableControl&&c.scope==Source::Scope::Outer&&c.admission==Source::Admission::Accepted)control=true;
            if(c.lexeme=="30"&&row.kind==Source::Kind::Entity&&c.scope==Source::Scope::Outer)point=true;
            if(c.lexeme=="not-a-handle"&&c.scope==Source::Scope::Application)opaque=true;
        }
        if(!control||!point||!opaque)return false;++positive;
        for(unsigned which=0;which<2u;++which)for(bool exact:{false,true}) {
            Probe limited;if(!limited.enableLegacyDxfIdentityObservation())return false;dx_data bounded;const auto limit=which?A::legacyIdentityWork(data):A::legacyIdentityBytes(data);
            if(!limit)return false;A::identityLimit(bounded,which!=0u,limit-(exact?0u:1u));
            if(!limited.fileImport(path.string(),&bounded,false)||A::legacyIdentityFailed(bounded)==exact
                ||bool(A::legacyIdentity(bounded))!=exact||!gated(bounded))return false;
            ++quota;
            if(!exact){dx_data retry;if(!limited.fileImport(path.string(),&retry,false)||A::legacyIdentityFailed(retry)||!A::legacyIdentity(retry)||!gated(retry))return false;++retries;}
        }
        for(unsigned fault=0;fault<4u;++fault) {
            Probe bad;if(!bad.enableLegacyDxfIdentityObservation())return false;bad.copy=fault==0u;bad.duplicate=fault==1u||fault==3u;bad.duplicateTail=fault==3u;bad.allocation=fault==2u?0:-1;dx_data rejected;
            if(!bad.fileImport(path.string(),&rejected,false)||!A::legacyIdentityFailed(rejected)||A::legacyIdentity(rejected)||!gated(rejected))return false;++refusals;
            bad.copy=bad.duplicate=bad.duplicateTail=false;bad.allocation=-1;dx_data retry;
            if(!bad.fileImport(path.string(),&retry,false)||A::legacyIdentityFailed(retry)||!A::legacyIdentity(retry)||!gated(retry))return false;++retries;
        }
        Probe observerLimited;if(!observerLimited.enableLegacyDxfIdentityObservation())return false;
        observerLimited.limits.records=0u;dx_data limitedSource;
        if(!observerLimited.fileImport(path.string(),&limitedSource,false)||!A::legacyIdentityFailed(limitedSource)
            ||A::legacyIdentity(limitedSource)||!gated(limitedSource))return false;
        observerLimited.limits={};dx_data recovered;
        if(!observerLimited.fileImport(path.string(),&recovered,false)||A::legacyIdentityFailed(recovered)
            ||!A::legacyIdentity(recovered)||!gated(recovered))return false;
        for(unsigned fault=0u;fault<11u;++fault) {
            Probe changed;if(!changed.enableLegacyDxfIdentityObservation())return false;
            changed.inputPath=path;changed.lateMutation=fault;changed.reenter=fault==10u;
            dx_data rejected;
            if(!changed.fileImport(path.string(),&rejected,false)||!A::legacyIdentityFailed(rejected)
                ||A::legacyIdentity(rejected)||!gated(rejected))return false;
            changed.lateMutation=10u;changed.reenter=false;dx_data retry;
            if(!changed.fileImport(path.string(),&retry,false)||A::legacyIdentityFailed(retry)
                ||!A::legacyIdentity(retry)||!gated(retry))return false;
            ++late;
        }
        // A published observation is immutable and outlives the codec, but
        // it must be discarded from the adapter when its import fails or a
        // new import reuses the same data context.
        Probe thrown;if(!thrown.enableLegacyDxfIdentityObservation())return false;
        thrown.throwAfter=true;dx_data failed;bool exception=false;
        try{(void)thrown.fileImport(path.string(),&failed,false);}catch(const std::runtime_error&){exception=true;}
        if(!exception||!A::legacyIdentityFailed(failed)||A::legacyIdentity(failed)||!gated(failed))return false;
        thrown.throwAfter=false;
        if(!thrown.fileImport(path.string(),&failed,false)||A::legacyIdentity(failed)||!A::legacyIdentityFailed(failed)||!gated(failed))return false;
        dx_data clean;if(!thrown.fileImport(path.string(),&clean,false)||!A::legacyIdentity(clean)||A::legacyIdentityFailed(clean)||!gated(clean))return false;
        const auto saved=A::legacyIdentity(clean);
        if(!saved->source||saved->source->records.size()!=15u||!saved->context||saved->context->binary!=binary)return false;
        thrown.addDxfIdentitySource(*saved->source); // callback outside the live reader
        if(A::legacyIdentity(clean)||!A::legacyIdentityFailed(clean)||!gated(clean))return false;
        if(!thrown.fileImport(path.string(),&clean,false)||A::legacyIdentity(clean)||!A::legacyIdentityFailed(clean)||!gated(clean))return false;
        if(!saved->source||saved->source->records.size()!=15u)return false; // snapshot remains alive, never authoritative
    }
    std::cout<<"legacy identity observations="<<positive<<" defaults="<<defaults<<" quota="<<quota<<" refusals="<<refusals<<" retries="<<retries<<" late="<<late<<'\n';
    return positive==2u&&defaults==2u&&quota==8u&&refusals==8u&&retries==12u&&late==22u;
}
