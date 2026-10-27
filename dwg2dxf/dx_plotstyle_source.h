// SPDX-License-Identifier: GPL-2.0-or-later
// Included once after the original structural/resource audit helpers.
namespace {
bool plotSourceDictionaryName(std::string_view name) noexcept {
    return asciiIdentifierEquals(name,"DICTIONARYWDFLT")||asciiIdentifierEquals(name,"ACDBDICTIONARYWDFLT");
}
bool plotSourceModelsEqual(const DRW_DictionaryWithDefault& a,const DRW_DictionaryWithDefault& b,DRW::Version version) {
    if(!standardDwgCommonEqual(a,b,version)||a.hardOwner!=b.hardOwner||a.cloning!=b.cloning
        ||a.m_defaultEntryHandle!=b.m_defaultEntryHandle||a.m_entries.size()!=b.m_entries.size()
        ||a.hasCompleteDxfPayload()!=b.hasCompleteDxfPayload())return false;
    for(std::size_t i=0u;i<a.m_entries.size();++i)if(a.m_entries[i].m_name!=b.m_entries[i].m_name
        ||a.m_entries[i].m_handle!=b.m_entries[i].m_handle||a.m_entries[i].m_dxfHandleCode!=b.m_entries[i].m_dxfHandleCode)return false;
    return true;
}
template<class C> bool plotSourceDecode(std::string_view text,const C& context,std::string& decoded) {
    if(text.empty()||text.find('\0')!=std::string_view::npos)return false;
    DRW_TextCodec codec;codec.setVersion(context.codecVersion,true);codec.setCodePage(context.codecCodePage,true);
    codec.setDxfTextEncoding(context.textEncoding);codec.setDxfCodePagePolicy(context.codePagePolicy);
    decoded=codec.toUtf8(text);
    return !codec.lastDecodeObservation().issues&&!decoded.empty()&&decoded.find('\0')==std::string::npos;
}
template<class Input> bool plotSourceProfile(const Input& input,std::vector<dx_tableResourceReference>& refs) {
    using R=dx_tableResourceRole;const auto& raw=*input.raw;
    unsigned self=0u,owner=0u,stage=0u,object=0u,hard=0u,clone=0u,def=0u,reactors=0u,extension=0u,extensionHandles=0u;
    std::string_view application;std::string pending;std::set<std::string> names;std::size_t entry=0u;
    for(std::size_t i=0u;i<raw.groups.size();++i) {
        const auto& v=raw.groups[i];const int code=v.code();const auto text=dx_attdef_source::text(v);
        if(!dx_compound_source::scalarType(v))return false;
        if(code==999)continue;
        if(code==102) {
            if(stage||!self)return false;
            if(text=="}"){if(application.empty()||(application=="{ACAD_XDICTIONARY"&&extensionHandles!=1u))return false;application={};}
            else if(application.empty()&&text=="{ACAD_REACTORS"&&!reactors++){application=text;}
            else if(input.dictionary&&application.empty()&&text=="{ACAD_XDICTIONARY"&&!extension++){application=text;}
            else return false;
            continue;
        }
        if(!application.empty()) {
            if(code!=(application=="{ACAD_REACTORS"?330:360))return false;
            if(application=="{ACAD_XDICTIONARY"&&extensionHandles++)return false;
        } else if(code==100) {
            if(!input.dictionary)return false;
            if(text=="AcDbObject"){if(stage||object++)return false;}
            else if(text=="AcDbDictionary"){if(stage++)return false;}
            else if(text=="AcDbDictionaryWithDefault"){if(stage!=1u||!pending.empty())return false;stage=2u;}
            else return false;
            continue;
        } else if(!isDxfHandleCode(code)) {
            if(!input.dictionary||stage!=1u)return false;
            if(code==280){if(hard++||!pending.empty()||v.i_val()<0||v.i_val()>1||v.i_val()!=input.model->hardOwner)return false;}
            else if(code==281){if(clone++||!pending.empty()||v.i_val()<0||v.i_val()>5||v.i_val()!=input.model->cloning)return false;}
            else if(code==3){if(!pending.empty()||!plotSourceDecode(text,*input.context,pending))return false;
                std::string key;if(!dx_symbol_output::nameKey(pending,key)||!names.insert(std::move(key)).second)return false;}
            else return false;
            continue;
        }
        std::uint64_t handle=0u;if(!dx_standard_output::handleValue(v,handle)||handle>UINT32_MAX||!handle)return false;
        R role=R::Unknown;
        if(!application.empty())role=application=="{ACAD_REACTORS"?R::Reactor:R::ExtensionDictionary;
        else if(!stage&&code==5){if(self++||owner||handle!=raw.handle)return false;role=R::Self;}
        else if(!stage&&code==330){if(owner++||!self||handle!=raw.parentHandle)return false;role=R::Owner;}
        else if(input.dictionary&&stage==1u&&(code==350||code==360)) {
            if(pending.empty()||entry>=input.model->m_entries.size())return false;
            const auto& expected=input.model->m_entries[entry++];
            if(pending!=expected.m_name||handle!=expected.m_handle||code!=static_cast<int>(expected.m_dxfHandleCode))return false;
            pending.clear();role=code==350?R::DictionarySoftOwner:R::DictionaryHardOwner;
        } else if(input.dictionary&&stage==2u&&code==340){if(def++||handle!=input.model->m_defaultEntryHandle)return false;role=R::DictionaryDefault;}
        else return false;
        refs.push_back({role,i,code,static_cast<std::uint32_t>(handle)});
    }
    return application.empty()&&self==1u&&owner==1u&&pending.empty()
        &&(!input.dictionary||(stage==2u&&hard==1u&&clone==1u&&def==1u&&entry==input.model->m_entries.size()));
}
}

void dx_iface::observePlotStyleSource(const DRW_TableEntry* model,const DRW_RawDxfObject* raw) noexcept {
    if(!tableSourceCaptureOpen())return;auto& s=cData->m_tableDxfSource;if(s.plotSourceCaptureFailed)return;
    using V=dxfRW::DxfPlotStyleSourceView;using Input=dx_data::TableDxfSourceCapture::PlotSourceInput;
    using Context=dx_data::TableDxfSourceCapture::EntitiesReadContext;
    V view;dxfRW::DxfReadContext live;
    if(!s.entitiesReadInput||!s.entitiesReadInput->getDxfPlotStyleSourceView(view)
        ||(view.kind!=V::Kind::DictionaryWithDefault&&view.kind!=V::Kind::Placeholder))return;
    const auto fail=[&](){s.plotSourceCaptureFailed=true;};
    if(!s.entitiesReadInput->getDxfReadContext(live)||live.sourceVersion<DRW::AC1021||live.sourceVersion>DRW::AC1032)return;
    const bool dictionary=view.kind==V::Kind::DictionaryWithDefault;
    if(!view.source||!view.source->handle||view.source->m_version!=live.sourceVersion||view.source->hasRawValues==live.binary
        ||(dictionary?!plotSourceDictionaryName(view.source->name):!asciiIdentifierEquals(view.source->name,"ACDBPLACEHOLDER"))
        ||view.phase!=(model?V::Phase::Typed:V::Phase::Raw)||(raw&&raw!=view.source)||(model&&model!=view.model)
        ||(dictionary?(!view.typedAvailable||!view.model||typeid(*view.model)!=typeid(DRW_DictionaryWithDefault)):(model||view.model||view.typedAvailable))) {fail();return;}
    try {
        const auto id=view.source->handle;const auto state=std::make_pair(view.originalPendingXdata,view.originalCoordinateMask);
        if(model||!dictionary) {
            if(s.plotSourceInputs.count(id)||s.plotSourcePending.count(id)||s.plotSourceRecords>=s.maxPlotSourceRecords
                ||s.plotSourceGroups>s.maxPlotSourceGroups||view.source->groups.size()>s.maxPlotSourceGroups-s.plotSourceGroups
                ||live.codecCodePage.size()>128u){fail();return;}
            std::size_t bytes=sizeof(Input)+sizeof(dx_data::TableDxfSourceCapture::PlotSourcePending)+sizeof(dx_data::TableDxfSourceCapture::PlotSourceRecord)
                +sizeof(Context)+96u*sizeof(void*),rawBytes=0u,modelBytes=0u,items=0u;
            if(!estimateRawDxfObjectBytes(*view.source,rawBytes)||!addAssociativeSlotBytes(rawBytes,16u,bytes)
                ||(dictionary&&!estimateDictionaryWithDefaultTypedBytes(static_cast<const DRW_DictionaryWithDefault&>(*model),modelBytes,items))
                ||!addAssociativeRetainedBytes(modelBytes,bytes)||!addAssociativeSlotBytes(view.source->groups.size(),sizeof(dx_tableResourceReference)+128u,bytes)
                ||!addAssociativeRetainedBytes(live.codecCodePage.size(),bytes)||!addAssociativeRetainedBytes(live.declaration.size(),bytes)
                ||s.plotSourceBytes>s.maxPlotSourceBytes||bytes>s.maxPlotSourceBytes-s.plotSourceBytes
                ||s.plotSourceWork>s.maxPlotSourceWork||bytes>s.maxPlotSourceWork-s.plotSourceWork){fail();return;}
            s.plotSourceBytes+=bytes;s.plotSourceWork+=bytes;++s.plotSourceRecords;s.plotSourceGroups+=view.source->groups.size();
            auto context=std::make_shared<Context>();context->sourceVersion=live.sourceVersion;context->codecVersion=live.codecVersion;
            context->codecCodePage=live.codecCodePage;context->declaration=live.declaration;context->hasDeclaration=live.hasDeclaration;
            context->compatibilityProfile=live.compatibilityProfile;context->textEncoding=live.textEncoding;context->codePagePolicy=live.codePagePolicy;
            context->binary=live.binary;context->applyExtrusion=live.applyExtrusion;context->captured=true;
            dx_data::TableDxfSourceCapture::PlotSourcePending pending;auto& in=pending.input;
            in.container=cData;in.generation=s.generation;in.containerId=cData->m_entitiesContainerIdentity.value;in.dictionary=dictionary;
            in.originalState=state;in.context=std::move(context);in.raw=std::make_shared<const DRW_RawDxfObject>(*view.source);
            if(dictionary)in.model=std::make_shared<const DRW_DictionaryWithDefault>(static_cast<const DRW_DictionaryWithDefault&>(*model));
            pending.model=model;pending.raw=view.source;
            in.paired=!dictionary||(view.source->handle==model->handle&&view.source->parentHandle==model->parentHandle
                &&dxfBlockRecord::Decoder::observeCommonState(*model)==state);
            if(dictionary){s.plotSourcePending.emplace(id,std::move(pending));return;}
            in.index=s.plotSourceInputOwners.size();auto saved=std::make_shared<const Input>(std::move(in));
            s.plotSourceInputs.emplace(id,saved);s.plotSourceInputOwners.push_back(std::move(saved));return;
        }
        const auto found=s.plotSourcePending.find(id);if(found==s.plotSourcePending.end()){fail();return;}
        auto& pending=found->second;auto& in=pending.input;
        if(pending.model!=view.model||pending.raw!=raw||in.originalState!=state||!in.model||!in.raw
            ||in.context->sourceVersion!=live.sourceVersion||in.context->codecVersion!=live.codecVersion||in.context->codecCodePage!=live.codecCodePage
            ||in.context->declaration!=live.declaration||in.context->hasDeclaration!=live.hasDeclaration||in.context->binary!=live.binary
            ||in.context->compatibilityProfile!=live.compatibilityProfile||in.context->textEncoding!=live.textEncoding
            ||in.context->codePagePolicy!=live.codePagePolicy||in.context->applyExtrusion!=live.applyExtrusion){fail();return;}
        in.paired=in.paired&&retainedRawDxfObjectsEqual(*in.raw,*raw)&&dxfBlockRecord::Decoder::observeCommonState(*view.model)==state
            &&plotSourceModelsEqual(*in.model,static_cast<const DRW_DictionaryWithDefault&>(*view.model),live.sourceVersion);
        in.index=s.plotSourceInputOwners.size();auto saved=std::make_shared<const Input>(std::move(in));
        s.plotSourceInputs.emplace(id,saved);s.plotSourceInputOwners.push_back(std::move(saved));s.plotSourcePending.erase(found);
    }catch(...){fail();}
}

void dx_iface::finishPlotStyleSources(dx_data& data) noexcept {
    using A=dx_data::TableDxfSourceCapture::PlotSourceAudit;using S=A::State;using Record=dx_data::TableDxfSourceCapture::PlotSourceRecord;
    using I=DRW_DxfIdentitySource;auto& s=data.m_tableDxfSource;if(s.plotSourceCaptureFailed)return;
    try {
        std::size_t bytes=sizeof(A)+64u*sizeof(void*),work=bytes;
        const auto physicalCount=s.identityInput&&s.identityInput->source?s.identityInput->source->records.size():0u;
        if(!addAssociativeSlotBytes(physicalCount,128u,work)||!addAssociativeSlotBytes(s.structuralAudits.size(),128u,work)){s.plotSourceCaptureFailed=true;return;}
        if(s.plotSourceBytes>s.maxPlotSourceBytes||bytes>s.maxPlotSourceBytes-s.plotSourceBytes
            ||s.plotSourceWork>s.maxPlotSourceWork||work>s.maxPlotSourceWork-s.plotSourceWork){s.plotSourceCaptureFailed=true;return;}
        s.plotSourceBytes+=bytes;s.plotSourceWork+=work;
        auto a=std::make_shared<A>();a->container=&data;a->generation=s.generation;a->containerId=data.m_entitiesContainerIdentity.value;
        a->identity=s.identityInput;a->identities=s.identityAudit;
        const auto save=[&](S state){a->state=state;s.plotSourceAudit=std::move(a);s.plotSourceOwner=s.plotSourceAudit;};
        if(identitySourceReport(data).state!=IdentitySourceState::NamespaceReady||!a->identity||!a->identities){save(S::SourceNotReady);return;}
        const auto& physical=*a->identity->source;
        for(const auto& p:physical.records)if(p.kind==I::Kind::Object&&(plotSourceDictionaryName(p.name)||asciiIdentifierEquals(p.name,"ACDBPLACEHOLDER")))++a->physicalRecords;
        bool complete=a->physicalRecords==s.plotSourceInputs.size();
        for(const auto& item:s.plotSourceInputs) {
            auto row=std::make_shared<Record>();row->input=item.second;const auto& in=*item.second;
            row->bodyMatched=in.paired&&(!in.dictionary||(!in.originalState.first&&!in.originalState.second&&in.model&&in.model->hasCompleteDxfPayload()))
                &&plotSourceProfile(in,row->references);
            const auto id=a->identities->identities.find(item.first);
            if(id!=a->identities->identities.end()&&id->second<physical.records.size()) {
                const auto& p=physical.records[id->second];
                row->originBound=p.kind==I::Kind::Object&&p.section<physical.sections.size()&&physical.sections[p.section].kind==I::Section::Objects
                    &&(in.dictionary?plotSourceDictionaryName(p.name):asciiIdentifierEquals(p.name,"ACDBPLACEHOLDER"));
                if(row->originBound)row->physical=id->second;
            }
            complete=complete&&row->bodyMatched&&row->originBound;a->records.emplace(item.first,std::move(row));
        }
        if(!a->physicalRecords&&!s.plotSourceRecords){save(S::NoSources);return;}
        // Root inventories are shared immutable inputs, but their temporary
        // reference/codec workspace and scans still belong to this audit.
        std::size_t rootBytes=0u,rootWork=0u;
        for(const auto& item:s.structuralAudits)if(item.second&&item.second->input&&item.second->input->raw
            &&item.second->input->kind==dxfRW::DxfStructuralSourceView::Kind::RootDictionary) {
            const auto& raw=*item.second->input->raw;std::size_t estimate=0u;
            if(!addAssociativeSlotBytes(raw.groups.size(),128u,rootWork)||s.plotSourceWork>s.maxPlotSourceWork
                ||rootWork>s.maxPlotSourceWork-s.plotSourceWork||!estimateRawDxfObjectBytes(raw,estimate)
                ||!addAssociativeSlotBytes(estimate,8u,rootBytes)||!addAssociativeSlotBytes(estimate,16u,rootWork)) {s.plotSourceCaptureFailed=true;return;}
        }
        if(s.plotSourceBytes>s.maxPlotSourceBytes||rootBytes>s.maxPlotSourceBytes-s.plotSourceBytes
            ||s.plotSourceWork>s.maxPlotSourceWork||rootWork>s.maxPlotSourceWork-s.plotSourceWork){s.plotSourceCaptureFailed=true;return;}
        s.plotSourceBytes+=rootBytes;s.plotSourceWork+=rootWork;
        unsigned roots=0u;for(const auto& item:s.structuralAudits) {
            const auto& row=item.second;
            if(row&&row->originBound&&row->bodyMatched&&row->stateComplete&&row->input&&row->input->paired
                &&row->input->kind==dxfRW::DxfStructuralSourceView::Kind::RootDictionary){a->root=row;++roots;}
        }
        if(roots==1u) {
            const auto* root=dynamic_cast<const DRW_Dictionary*>(a->root->input->model.get());unsigned names=0u;
            std::vector<dx_tableResourceReference> refs;const auto& raw=*a->root->input->raw;
            bool sourceReady=root&&!root->parentHandle&&a->root->input->context&&spineSourceReferences(raw,true,refs);
            if(sourceReady)for(const auto& ref:refs) {
                if(ref.kind==dx_tableResourceRole::Unknown){sourceReady=false;break;}
                if(ref.kind!=dx_tableResourceRole::DictionarySoftOwner&&ref.kind!=dx_tableResourceRole::DictionaryHardOwner)continue;
                if(!ref.groupIndex||raw.groups[ref.groupIndex-1u].code()!=3){sourceReady=false;break;}
                std::string key;if(!plotSourceDecode(dx_attdef_source::text(raw.groups[ref.groupIndex-1u]),*a->root->input->context,key)){sourceReady=false;break;}
                if(asciiIdentifierEquals(key,"ACAD_PLOTSTYLENAME")) {
                    a->dictionary=ref.value;a->rootKeyGroup=ref.groupIndex-1u;a->rootTargetGroup=ref.groupIndex;++names;
                }
            }
            const auto found=a->records.find(a->dictionary);
            if(sourceReady&&names==1u&&found!=a->records.end()&&found->second->bodyMatched&&found->second->originBound&&found->second->input->dictionary) {
                const auto& in=*found->second->input;const auto& dictionary=*in.model;bool valid=in.raw->parentHandle==a->root->input->raw->handle;
                std::set<std::uint32_t> members;
                const auto validTarget=[&](std::uint32_t handle) {
                    const auto target=a->records.find(handle);
                    if(target==a->records.end()||!target->second->bodyMatched||!target->second->originBound||target->second->input->dictionary
                        ||target->second->input->raw->parentHandle!=a->dictionary)return false;
                    for(const auto& ref:target->second->references)if(ref.kind==dx_tableResourceRole::Reactor&&ref.value!=a->dictionary)return false;
                    return true;
                };
                for(const auto& entry:dictionary.m_entries) {
                    if(!validTarget(entry.m_handle))valid=false;
                    members.insert(entry.m_handle);
                }
                // A dictionary-wide fallback need not have a named entry.
                // Preserve the default role separately instead of inventing
                // membership or requiring it to duplicate an explicit key.
                if(valid&&validTarget(dictionary.m_defaultEntryHandle)) {
                    a->namedTargets=members.size();a->defaultTarget=dictionary.m_defaultEntryHandle;
                    members.insert(dictionary.m_defaultEntryHandle);a->plotTargets=std::move(members);
                }
            }
        }
        save(!a->physicalRecords&&!s.plotSourceRecords?S::NoSources:complete?S::SourcesReady:S::Unqualified);
    }catch(...){s.plotSourceCaptureFailed=true;}
}

dx_iface::PlotSourceReport dx_iface::plotStyleSourceReport(const dx_data& data) noexcept {
    using S=PlotSourceState;const auto& s=data.m_tableDxfSource;PlotSourceReport r;
    r.bytes=s.plotSourceBytes;r.work=s.plotSourceWork;r.records=s.plotSourceRecords;
    if(!s.generation||data.sourceFormat!=dx_source_format::DXF)return r;
    if(!s.closed){r.state=S::Reading;return r;}if(!s.successful){r.state=S::ImportFailed;return r;}
    if(s.contextFailed||s.generation!=1u||s.generation!=data.importGeneration||s.reader||s.entitiesReadInput){r.state=S::ContextMismatch;return r;}
    if(s.plotSourceCaptureFailed||!s.plotSourcePending.empty()){r.state=S::CaptureFailed;return r;}
    const auto& a=s.plotSourceAudit;if(!a)return r;
    const auto same=[](const auto& x,const auto& y){return x.get()==y.get()&&!x.owner_before(y)&&!y.owner_before(x);};
    if(!same(a,s.plotSourceOwner)||a->container!=&data||a->generation!=s.generation||a->containerId!=data.m_entitiesContainerIdentity.value
        ||!same(a->identity,s.identityInput)||!same(a->identities,s.identityAudit)||a->records.size()!=s.plotSourceInputs.size()
        ||s.plotSourceInputs.size()!=s.plotSourceInputOwners.size()){r.state=S::ContextMismatch;return r;}
    const auto identities=identitySourceReport(data);const auto structural=structuralSourceReport(data);
    if(identities.state==IdentitySourceState::ContextMismatch||structural.state==StructuralSourceState::ContextMismatch){r.state=S::ContextMismatch;return r;}
    if(identities.state!=IdentitySourceState::NamespaceReady||structural.state==StructuralSourceState::CaptureFailed){r.state=S::SourceNotReady;return r;}
    if(a->root){const auto found=s.structuralAudits.find(a->root->input->raw->handle);
        if(found==s.structuralAudits.end()||!same(a->root,found->second)){r.state=S::ContextMismatch;return r;}}
    for(const auto& item:a->records) {
        const auto& row=item.second;const auto found=s.plotSourceInputs.find(item.first);
        if(!row||!row->input||found==s.plotSourceInputs.end()||!same(row->input,found->second)||row->input->index>=s.plotSourceInputOwners.size()
            ||!same(row->input,s.plotSourceInputOwners[row->input->index])||row->input->container!=&data||row->input->generation!=s.generation
            ||row->input->containerId!=data.m_entitiesContainerIdentity.value||!row->input->context||row->input->context->sourceVersion!=s.version){r.state=S::ContextMismatch;return r;}
        if(row->input->dictionary)++r.dictionaries;else ++r.placeholders;if(row->bodyMatched)++r.bodyMatched;if(row->originBound)++r.originsBound;
    }
    r.state=a->state;r.physicalRecords=a->physicalRecords;r.plotTargets=a->plotTargets.size();r.namedTargets=a->namedTargets;r.defaultTarget=a->defaultTarget;
    r.originalSourcesMatched=r.state==S::SourcesReady&&r.records&&r.records==r.bodyMatched&&r.records==r.originsBound;return r;
}
