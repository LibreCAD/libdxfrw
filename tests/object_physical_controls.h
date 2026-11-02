// SPDX-License-Identifier: GPL-2.0-or-later
// Actual OBJECTS observer/publication controls; runtime-authored sources only.
namespace objectPhysicalControls {
using A=DxIfaceTableSourceTestAccess;using Source=DRW_DxfObjectPhysicalSource;
using V=dxfRW::DxfObjectPhysicalSourceView;
bool ready(const dx_data& data) {
    const auto s=dx_iface::renderingSourceReport(data);const auto d=dx_iface::dependencyReport(data);
    return s.originalSourcesMatched&&s.namedTargets==1u&&d.originalTargetsMatched
        &&!s.ownerTargetsResolved&&!s.recursiveClosureMatched&&!s.outputEligible&&!d.outputEligible;
}
std::size_t comments(Tags& tags,unsigned form) {
    const auto before=tags.size();
    const auto put=[&](std::size_t at){tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{999,"locally authored observed comment"}});};
    if(form==1u){auto at=slot(tags,5,"EA01");while(tags[at].code!=2)++at;put(at);}
    if(form==2u){auto at=slot(tags,5,"EA01");while(tags[at].code!=102||tags[at].value!="{ACAD_REACTORS")++at;put(at+1u);}
    if(form==3u){auto at=slot(tags,5,"EA01");while(tags[at].code!=1003)++at;put(at);}
    if(form==4u)put(slot(tags,3,"ACAD_VISUALSTYLE"));
    if(form==5u)put(slot(tags,3,"AuthoredStyle"));
    if(form==6u)for(const char* handle:{"40","EB00","EA01"}){
        const auto begin=slot(tags,5,handle);auto end=begin;while(tags[end].code!=0)++end;
        while(end>begin)put(--end);
    }
    return tags.size()-before;
}
struct Limits:dx_iface,DRW_DxfObjectPhysicalSourcePolicy {
    DRW_DxfObjectPhysicalSourceLimits limits;
    DRW_DxfObjectPhysicalSourceLimits dxfObjectPhysicalSourceLimits()const noexcept override{return limits;}
};
struct Probe:dx_iface,DRW_DxfObjectPhysicalSourcePolicy {
    dx_data data;const dxfRW* input=nullptr;bool selected=true,good=true;
    mutable std::size_t policies=0u;std::size_t queries=0u;std::uint32_t throwHandle=0u;V::Phase throwPhase=V::Phase::None;
    std::map<std::uint32_t,std::shared_ptr<const Source>> frames;
    std::map<std::uint32_t,std::pair<std::uintptr_t,std::uintptr_t>> pointers;
    Probe(){cData=&data;currentBlock=data.mBlock;}
    bool wantsDxfResourceSource()const override{return selected;}
    bool wantsDxfRootDictionarySource()const override{return selected;}
    bool wantsDxfDictionarySource()const override{return selected;}
    DRW_DxfObjectPhysicalSourceLimits dxfObjectPhysicalSourceLimits()const noexcept override{++policies;return {};}
    void observe(bool target,const DRW_TableEntry* model,const DRW_RawDxfObject* raw,V::Phase phase) {
        V view;const auto before=symbolFailedAllocations;bool present=false;
        {SymbolAllocationFailure none(0);present=input&&input->getDxfObjectPhysicalSourceView(view);}
        if(before!=symbolFailedAllocations||present!=(selected&&target)){good=false;return;}
        if(!present){if(view.kind!=V::Kind::None||view.model||view.source||view.physical)good=false;return;}
        if(!view.model||!view.source||view.phase!=phase||(model&&view.model!=model)||(raw&&view.source!=raw)
            ||view.status!=Source::Status::Complete||!view.physical||view.physical->status!=view.status){good=false;return;}
        const auto id=view.source->handle;const auto pointer=std::make_pair(reinterpret_cast<std::uintptr_t>(view.model),reinterpret_cast<std::uintptr_t>(view.source));
        const auto found=frames.find(id);
        if(found==frames.end()){frames.emplace(id,view.physical);pointers.emplace(id,pointer);}
        else if(found->second.get()!=view.physical.get()||found->second.owner_before(view.physical)||view.physical.owner_before(found->second)
            ||pointers.at(id)!=pointer)good=false;
        ++queries;
        if(id==throwHandle&&phase==throwPhase)throw std::runtime_error("authored physical publication throw");
    }
    void addNamedObjectDictionary(const DRW_Dictionary& v)override{observe(true,&v,nullptr,V::Phase::Typed);}
    void addDictionary(const DRW_Dictionary& v)override{observe(true,&v,nullptr,V::Phase::Typed);}
    void addDxfRootDictionarySource(const DRW_Dictionary& v,const DRW_RawDxfObject& r)override{observe(true,&v,&r,V::Phase::Source);}
    void addDxfDictionarySource(const DRW_Dictionary& v,const DRW_RawDxfObject& r)override{observe(true,&v,&r,V::Phase::Source);}
    void addVisualStyle(const DRW_VisualStyle& v)override{observe(true,&v,nullptr,V::Phase::Typed);}
    void addRenderSettings(const DRW_RenderSettings& v)override{observe(true,&v,nullptr,V::Phase::Typed);}
    void addRawDxfObject(const DRW_RawDxfObject& r)override{observe(r.name=="DICTIONARY"||r.name=="VISUALSTYLE"||r.name=="RENDERSETTINGS",nullptr,&r,V::Phase::Raw);}
    void reset(){frames.clear();pointers.clear();policies=queries=0u;good=true;throwHandle=0u;throwPhase=V::Phase::None;}
};
bool test(const std::filesystem::path& dir) {
    const auto path=dir/"object-physical.dxf";
    std::size_t profiles=0u,fields=0u,mapped=0u,ignored=0u,named=0u,limits=0u,retries=0u,queries=0u,publications=0u,throws=0u,mutations=0u,reached=0u,unreached=0u,corrected=0u;
    const auto fail=[&](int line){std::cerr<<"Object physical check line="<<line<<" profiles="<<profiles<<" fields="<<fields<<" limits="<<limits<<" throws="<<throws<<'\n';return false;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true})for(unsigned form=0u;form<7u;++form){
        auto tags=renderingSourceDocument(version,blocks,false);const auto authored=comments(tags,form);dx_iface reader;dx_data data;
        if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data))return fail(__LINE__);
        const auto wire=attdefWireTags(path,encoding==2u);const auto& audit=*A::renderingSources(data);const auto& style=*audit.records.at(0xEA01u);
        std::size_t observed=0u;
        const auto inspect=[&](const auto& input,std::size_t physical,const auto& map){
            if(!input.physical||input.physicalStatus!=Source::Status::Complete)return false;const auto& source=*input.physical;
            const auto& original=A::identityInput(data)->source->records.at(physical);
            const auto& raw=input.raw; // structural input; rendering uses the explicit overload below
            if(map.size()!=raw->groups.size()||source.marker.ordinal!=original.marker.ordinal||source.endMarker.ordinal!=original.endMarker.ordinal)return false;
            for(const auto& field:source.fields){const auto& actual=wire.at(field.span.ordinal-1u);
                if(actual.code!=field.code||actual.begin!=field.span.beginOffset||actual.end!=field.span.endOffset||field.span.ignoredComment!=(field.code==999))return false;
                ++fields;if(field.span.ignoredComment){++ignored;++observed;}}
            for(std::size_t i=0u;i<map.size();++i){if(source.fields.at(map[i]).code!=raw->groups[i].code())return false;++mapped;}
            return true;};
        // Keep rendering/structural ownership separate while sharing the same
        // independently scanned physical-field assertions.
        struct Rendering {std::shared_ptr<const Source> physical;Source::Status physicalStatus;std::shared_ptr<const DRW_RawDxfObject> raw;};
        Rendering rendering{style.input->physical,style.input->physicalStatus,style.input->typedRaw};
        if(!inspect(rendering,style.physical,style.physicalFields)||!inspect(*audit.root->input,audit.root->physical,audit.rootFields)
            ||!inspect(*audit.dictionary->input,audit.dictionary->physical,audit.dictionaryFields))return fail(__LINE__);
        if(observed!=authored)return fail(__LINE__);
        for(unsigned which=0u;which<2u;++which){const bool root=which==0u;
            const auto pair=root?std::make_pair(audit.rootKeyGroup,audit.rootTargetGroup):std::make_pair(style.nameGroup,style.targetGroup);
            const auto& source=root?*audit.root->input->physical:*audit.dictionary->input->physical;const auto& map=root?audit.rootFields:audit.dictionaryFields;
            const auto& key=wire.at(source.fields.at(map.at(pair.first)).span.ordinal-1u);const auto& target=wire.at(source.fields.at(map.at(pair.second)).span.ordinal-1u);
            if(key.code!=3||key.text!=(root?"ACAD_VISUALSTYLE":"AuthoredStyle")||target.code!=350||target.text!=(root?"EB00":"EA01"))return fail(__LINE__);named+=2u;}
        const auto before=symbolFailedAllocations;{SymbolAllocationFailure none(0);if(!ready(data))return fail(__LINE__);}if(before!=symbolFailedAllocations)return fail(__LINE__);++queries;++profiles;
    }
    auto good=renderingSourceDocument("AC1027",false,false);comments(good,6u);
    for(unsigned encoding=0u;encoding<3u;++encoding){dx_iface reader;dx_data base;
        if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&base,false)||!ready(base))return fail(__LINE__);
        const auto source=A::renderingSources(base)->records.at(0xEA01u)->input->physical;
        for(unsigned which=0u;which<3u;++which){Limits limited;const auto exact=which==0u?source->fields.size():which==1u?source->bytes:source->work;
            const auto set=[&](std::size_t value){if(which==0u)limited.limits.fields=value;else if(which==1u)limited.limits.bytes=value;else limited.limits.work=value;};
            set(exact);dx_data fits;if(!limited.fileImport(path.string(),&fits,false)||!ready(fits))return fail(__LINE__);
            set(exact-1u);dx_data under;if(!limited.fileImport(path.string(),&under,false)||ready(under)||!dx_iface::renderingModelReport(under).modeledStateMatched)return fail(__LINE__);
            if(A::renderingSources(under)->records.at(0xEA01u)->input->physicalStatus!=Source::Status::Quota)return fail(__LINE__);++limits;
            limited.limits={};dx_data fixed;if(!limited.fileImport(path.string(),&fixed,false)||!ready(fixed))return fail(__LINE__);++retries;}
        for(unsigned owner=0u;owner<3u;++owner)for(unsigned fault=0u;fault<6u;++fault){
            const auto& audit=*A::renderingSources(base);
            auto physical=std::const_pointer_cast<Source>(owner==0u?source:owner==1u?audit.root->input->physical:audit.dictionary->input->physical);
            const auto original=*physical;
            if(fault==0u)++physical->fields.front().span.ordinal;
            if(fault==1u)physical->fields.front().span.beginOffset=physical->marker.beginOffset;
            if(fault==2u){auto at=physical->fields.begin();while(at->span.ignoredComment)++at;at->code=999;}
            if(fault==3u)physical->fields.front().span.ignoredComment=false;
            if(fault==4u)++physical->endMarker.endOffset;
            if(fault==5u)physical->binary=!physical->binary;
            A::repeatRenderingSources(reader,base);A::joinRenderingSources(reader,base);if(ready(base))return fail(__LINE__);++mutations;
            *physical=original;A::repeatRenderingSources(reader,base);A::joinRenderingSources(reader,base);if(!ready(base))return fail(__LINE__);++retries;}
        for(bool selected:{false,true}){Probe probe;probe.selected=selected;dxfRW input(path.string().c_str());probe.input=&input;
            if(!input.read(&probe,false)||!probe.good||(selected&&probe.policies!=probe.frames.size())||(!selected&&(probe.policies||!probe.frames.empty())))return fail(__LINE__);
            V expired;if(input.getDxfObjectPhysicalSourceView(expired)||expired.physical||expired.model||expired.source)return fail(__LINE__);queries+=probe.queries;++publications;}
        for(const auto& value:std::array<std::pair<std::uint32_t,V::Phase>,7>{{{0x40u,V::Phase::Typed},{0x40u,V::Phase::Source},
            {0xEB00u,V::Phase::Typed},{0xEB00u,V::Phase::Raw},{0xEB00u,V::Phase::Source},{0xEA01u,V::Phase::Typed},{0xEA01u,V::Phase::Raw}}}){
            Probe probe;probe.throwHandle=value.first;probe.throwPhase=value.second;dxfRW input(path.string().c_str());probe.input=&input;bool caught=false;
            try{if(input.read(&probe,false))return fail(__LINE__);}catch(const std::runtime_error&){caught=true;}
            V expired;if(!caught||!probe.good||input.getDxfObjectPhysicalSourceView(expired)||expired.physical)return fail(__LINE__);
            const auto retained=probe.frames.at(value.first);if(!retained||retained->status!=Source::Status::Complete)return fail(__LINE__);++throws;
            probe.reset();if(!input.read(&probe,false)||!probe.good||probe.policies!=probe.frames.size()||retained->status!=Source::Status::Complete)return fail(__LINE__);++retries;}
    }
    // Isolate faults to the actual collector. The real reader and both parent
    // observer chains run outside the allocation injector.
    std::string literal="0\nVISUALSTYLE\n5\nEA01\n";
    for(unsigned i=0u;i<64u;++i)literal+="999\nobserved comment\n70\n"+std::to_string(i)+"\n";
    literal+="0\nENDSEC\n";
    const auto run=[&](long offset,bool& failed){
        Probe policy;std::istringstream stream(literal);dxfReaderAscii reader(&stream);reader.setVersion("AC1027",true);reader.setIgnoreComments(true);
        auto budget=std::make_shared<DxfReadRetainedBudget>(1024u*1024u);reader.setRetainedByteBudget(budget);
        struct Counter:DxfReadRecordObserver {std::size_t records=0u;void observe(const DxfReadRecordView&)noexcept override{++records;}} parent,identity;
        reader.exchangeRecordObserver(&parent);reader.exchangeIdentityObserver(&identity);int code=-1;if(!reader.readRec(&code)||code)return false;
        std::optional<dxfObjectPhysicalSource::Capture> capture;long remaining=offset;const auto before=symbolFailedAllocations;
        {SymbolAllocationFailure fault(remaining);capture.emplace(reader,policy,true,std::string_view("VISUALSTYLE"));remaining=symbolAllocationsBeforeFailure;}
        struct Fault:DxfReadRecordObserver {dxfObjectPhysicalSource::Capture& capture;long& remaining;
            Fault(dxfObjectPhysicalSource::Capture& c,long& r):capture(c),remaining(r){}
            void observe(const DxfReadRecordView& value)noexcept override{SymbolAllocationFailure fault(remaining);capture.observe(value);remaining=symbolAllocationsBeforeFailure;}
        } fault(*capture,remaining);
        const auto previous=reader.exchangeRecordObserver(&fault);
        if(previous!=&*capture)return false;
        while(reader.readRec(&code))if(!code)break;
        if(code||parent.records!=131u||identity.records!=parent.records)return false;
        failed=symbolFailedAllocations!=before;const auto status=capture->sourceStatus();auto retained=capture->source();
        if(status!=(failed?Source::Status::Allocation:Source::Status::Complete))return false;
        reader.exchangeRecordObserver(previous);capture.reset();
        if(reader.objectPhysicalPublication()||reader.exchangeRecordObserver(nullptr)!=&parent||reader.exchangeIdentityObserver(nullptr)!=&identity)return false;
        if(!failed){if(!retained||retained->fields.size()!=129u||budget->used()<retained->bytes)return false;
            const auto used=budget->used(),bytes=retained->bytes;retained.reset();if(budget->used()!=used-bytes)return false;}
        return true;};
    unsigned untouched=0u;for(long offset=0;offset<64;++offset){bool failed=false;if(!run(offset,failed))return fail(__LINE__);
        if(failed){++reached;untouched=0u;}else{++unreached;++untouched;}
        bool correctedFailure=false;if(!run(-1,correctedFailure)||correctedFailure)return fail(__LINE__);++corrected;if(untouched==2u)break;if(offset==63)return fail(__LINE__);}
    std::cout<<"Object physical profiles="<<profiles<<"; observed fields="<<fields<<"; mapped raw groups="<<mapped<<"; ignored comments="<<ignored<<"; named fields="<<named
        <<"; exact/one-under limits="<<limits<<"; mapping refusals="<<mutations<<"; public opt-in profiles="<<publications<<"; immutable getters/queries="<<queries
        <<"; callback throws="<<throws<<"; corrected imports="<<retries<<"; reached collector failures="<<reached<<"; unreached offsets="<<unreached<<"; corrected collectors="<<corrected<<'\n';
    return profiles==168u&&ignored&&named==672u&&limits==9u&&mutations==54u&&publications==6u&&throws==21u&&retries==84u&&reached&&unreached==2u&&corrected==reached+unreached;
}
}
