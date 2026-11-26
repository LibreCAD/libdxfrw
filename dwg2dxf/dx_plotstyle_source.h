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
        &&(!input.dictionary||(stage==2u&&(hard==1u||(input.model->hardOwner==0&&std::none_of(refs.begin(),refs.end(),[](const dx_tableResourceReference& ref){return ref.kind==R::DictionaryHardOwner;})))&&clone==1u&&def==1u&&entry==input.model->m_entries.size()));
}
}

void dx_iface::observePlotStyleSource(const DRW_TableEntry* model,const DRW_RawDxfObject* raw) noexcept {
  if (!tableSourceCaptureOpen())
    return;
  auto &s = cData->m_tableDxfSource;
  if (s.plotSourceCaptureFailed)
    return;
  using V = dxfRW::DxfPlotStyleSourceView;
  using Input = dx_data::TableDxfSourceCapture::PlotSourceInput;
  using Context = dx_data::TableDxfSourceCapture::EntitiesReadContext;
  V view;
  dxfRW::DxfReadContext live;
  if (!s.entitiesReadInput ||
      !s.entitiesReadInput->getDxfPlotStyleSourceView(view) ||
      (view.kind != V::Kind::DictionaryWithDefault &&
       view.kind != V::Kind::Placeholder))
    return;
  const auto fail = [&]() { s.plotSourceCaptureFailed = true; };
  if (!s.entitiesReadInput->getDxfReadContext(live) ||
      (live.sourceVersion != DRW::AC1015 && !(live.sourceVersion==DRW::AC1018 && legacyPlotAc1018Enabled()) &&
       (live.sourceVersion < DRW::AC1021 || live.sourceVersion > DRW::AC1032)))
    return;
  const bool dictionary = view.kind == V::Kind::DictionaryWithDefault;
  if (!view.source || !view.source->handle ||
      view.source->m_version != live.sourceVersion ||
      view.source->hasRawValues == live.binary ||
      (dictionary
           ? !plotSourceDictionaryName(view.source->name)
           : !asciiIdentifierEquals(view.source->name, "ACDBPLACEHOLDER")) ||
      view.phase != (model ? V::Phase::Typed : V::Phase::Raw) ||
      (raw && raw != view.source) || (model && model != view.model) ||
      (dictionary ? (!view.typedAvailable || !view.model ||
                     typeid(*view.model) != typeid(DRW_DictionaryWithDefault))
                  : (model || view.model || view.typedAvailable))) {
    fail();
    return;
  }
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
                &&DRW::internal::DxfBlockRecordDecoder::observeCommonState(*model)==state);
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
        in.paired=in.paired&&retainedRawDxfObjectsEqual(*in.raw,*raw)&&DRW::internal::DxfBlockRecordDecoder::observeCommonState(*view.model)==state
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
        if(identitySourceReportInternal(data, true).state!=IdentitySourceState::NamespaceReady||!a->identity||!a->identities){save(S::SourceNotReady);return;}
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
            if(row&&(data.sourceVersion==DRW::AC1018?row->legacyPlotRootOriginBound:row->originBound)
                &&(data.sourceVersion==DRW::AC1018?row->legacyPlotRootBodyMatched:row->bodyMatched)&&row->stateComplete&&row->input&&row->input->paired
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
    if(s.plotSourceCaptureFailed||!s.plotSourcePending.empty()
        ||((data.sourceVersion==DRW::AC1015||s.legacyPlotAc1018Selected)&&(s.structuralCaptureFailed||s.resourceCaptureFailed
            ||s.resourceProofCaptureFailed||s.resourceOriginCaptureFailed||!s.resourceOriginPending.empty()
            ||!s.structuralPending.empty()))){r.state=S::CaptureFailed;return r;}
    const auto& a=s.plotSourceAudit;if(!a)return r;
    const auto same=[](const auto& x,const auto& y){return x.get()==y.get()&&!x.owner_before(y)&&!y.owner_before(x);};
    if(!same(a,s.plotSourceOwner)||a->container!=&data||a->generation!=s.generation||a->containerId!=data.m_entitiesContainerIdentity.value
        ||!same(a->identity,s.identityInput)||!same(a->identities,s.identityAudit)||a->records.size()!=s.plotSourceInputs.size()
        ||s.plotSourceInputs.size()!=s.plotSourceInputOwners.size()){r.state=S::ContextMismatch;return r;}
    const auto identities=identitySourceReportInternal(data, true);const auto structural=structuralSourceReport(data);
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
    r.originalSourcesMatched=r.state==S::SourcesReady&&r.records&&r.records==r.bodyMatched&&r.records==r.originsBound;
    if((data.sourceVersion==DRW::AC1015||s.legacyPlotAc1018Selected||(data.sourceVersion>=DRW::AC1021&&data.sourceVersion<=DRW::AC1032&&s.legacyPlotClassCount==2u)) && s.legacyPlotClassValid && s.legacyPlotClassCount==2u
        &&s.legacyPlotClassSeen[0]&&s.legacyPlotClassSeen[1]&&s.identityInput&&s.identityInput->source) {
        const auto& sections=s.identityInput->source->sections;
        r.legacyClassRowsMatched=sections.size()<=64u && std::count_if(sections.begin(),sections.end(),[](const auto& section) {
            return section.kind==DRW_DxfIdentitySource::Section::Classes;
        })==1;
        if(r.legacyClassRowsMatched && !data.dxfClassCaptureFailed && data.dxfClassRetainedRecords==2u && data.dxfClasses.size()==2u) {
            const char* records[]={s.legacyPlotClassDictionaryAlias?"DICTIONARYWDFLT":"ACDBDICTIONARYWDFLT","ACDBPLACEHOLDER"};
            const char* names[]={"AcDbDictionaryWithDefault","AcDbPlaceHolder"};
            std::array<bool,2> seen{{false,false}};bool valid=true;
            for(const auto& value:data.dxfClasses) {
                unsigned index=2u;for(unsigned i=0u;i<2u;++i)if(value.recName==records[i])index=i;
                if(index==2u || seen[index]) {valid=false;break;}
                seen[index]=true;
                if(value.className!=names[index] || value.appName!="ObjectDBX Classes" || value.proxyFlag!=0
                    ||value.instanceCount!=(data.sourceVersion==DRW::AC1015?0:1) ||value.wasaProxyFlag!=0 ||value.entityFlag!=0) {valid=false;break;}
            }
            r.legacyLiveClassesMatched=valid&&seen[0]&&seen[1];
        }
    }
    // The exact generated default-plot object shape is source evidence,
    // not permission to export. Preserve every modeled and raw source field
    // for the later transactional writer and its staged output proof.
    if((data.sourceVersion==DRW::AC1015||s.legacyPlotAc1018Selected||(data.sourceVersion>=DRW::AC1021&&data.sourceVersion<=DRW::AC1032&&s.legacyPlotClassCount==2u)) && r.originalSourcesMatched && a->root && a->root->input
        &&a->root->input->model && a->dictionary==0xEu && a->defaultTarget==0xFu
        &&a->namedTargets==1u && a->records.size()==2u) try {
        const auto dictionary=a->records.find(a->dictionary), placeholder=a->records.find(a->defaultTarget);
        if(dictionary!=a->records.end() && placeholder!=a->records.end() && dictionary->second && placeholder->second
            &&dictionary->second->input && placeholder->second->input && dictionary->second->input->raw
            &&placeholder->second->input->raw && dictionary->second->input->model) {
            const auto& raw=*dictionary->second->input->raw;const auto& place=*placeholder->second->input->raw;
            const auto& model=*dictionary->second->input->model;
            const auto handle=[](const DRW_Variant& v,int code,std::uint32_t expected) {
                std::uint64_t actual=0u;return v.code()==code&&dx_standard_output::handleValue(v,actual)&&actual==expected;
            };
            const auto text=[](const DRW_Variant& v,int code,std::string_view expected) {
                return v.code()==code&&v.type()==DRW_Variant::STRING&&dx_attdef_source::text(v)==expected;
            };
            const auto integer=[](const DRW_Variant& v,int code,int expected) {
                return v.code()==code&&v.type()==DRW_Variant::INTEGER&&v.i_val()==expected;
            };
            const auto* root=dynamic_cast<const DRW_Dictionary*>(a->root->input->model.get());
            r.legacyBodiesMatched=root&&root->handle==0xCu&&!root->parentHandle
                &&raw.name=="ACDBDICTIONARYWDFLT"&&raw.handle==0xEu&&raw.parentHandle==root->handle
                &&raw.groups.size()==8u&&handle(raw.groups[0],5,0xEu)&&handle(raw.groups[1],330,0xCu)
                &&text(raw.groups[2],100,"AcDbDictionary")&&integer(raw.groups[3],281,1)
                &&text(raw.groups[4],3,"Normal")&&handle(raw.groups[5],350,0xFu)
                &&text(raw.groups[6],100,"AcDbDictionaryWithDefault")&&handle(raw.groups[7],340,0xFu)
                &&place.name=="ACDBPLACEHOLDER"&&place.handle==0xFu&&place.parentHandle==0xEu
                &&place.groups.size()==2u&&handle(place.groups[0],5,0xFu)&&handle(place.groups[1],330,0xEu)
                &&model.handle==0xEu&&model.parentHandle==0xCu&&model.hardOwner==0&&model.cloning==1
                &&model.m_defaultEntryHandle==0xFu&&model.m_entries.size()==1u&&model.m_entries[0].m_name=="Normal"
                &&model.m_entries[0].m_handle==0xFu&&model.m_entries[0].m_dxfHandleCode==DRW_Dictionary::Entry::DxfHandleCode::SoftOwner
                &&model.hasCompleteDxfPayload()&&model.extData.empty()&&model.appData.empty()
                &&model.reactorHandles.empty()&&!model.xDictHandle;
            if(r.legacyBodiesMatched && data.hasNamedObjectDictionary && root->m_entries.size()<=64u
                &&data.dictionaryWithDefaults.size()==1u && data.rawDictionaryWithDefaults.size()==1u
                &&data.rawPlaceholders.size()==1u && data.placeholders.empty()
                &&data.hasSingleSuccessfulDictionaryDefaultImport()) {
                r.legacyLiveBodiesMatched=mlineDictionaryKeysEqual(*root,data.namedObjectDictionary,{},data.sourceVersion)
                    &&plotSourceModelsEqual(model,data.dictionaryWithDefaults.front(),data.sourceVersion)
                    &&retainedRawDxfObjectsEqual(raw,data.rawDictionaryWithDefaults.front())
                    &&retainedRawDxfObjectsEqual(place,data.rawPlaceholders.front());
            }
        }
    } catch (...) { r.legacyBodiesMatched=false;r.legacyLiveBodiesMatched=false; }
    // Source-only inventory for bounded legacy LAYER rows. This deliberately
    // does not grant target-reference or output authority.
    if ((data.sourceVersion==DRW::AC1015||s.legacyPlotAc1018Selected)
        &&r.legacyBodiesMatched&&s.identityInput&&s.identityInput->source
        &&s.identityInput->source->records.size()<=4096u
        &&s.identityAudit&&!s.resourceCaptureFailed&&!s.resourceProofCaptureFailed
        &&!s.resourceOriginCaptureFailed&&s.resourceOriginPending.empty()) try {
        const auto& physical=s.identityInput->source->records;
        const auto count=static_cast<std::size_t>(std::count_if(physical.begin(),physical.end(),[](const auto& row) {
            return row.kind==DRW_DxfIdentitySource::Kind::TableRecord&&asciiIdentifierEquals(row.name,"LAYER");
        }));
        if(count>0u&&count<=64u&&count==s.resourceOriginRecords
            &&count==s.resourceOriginInputs.size()&&count==s.resourceOriginAudits.size()
            &&count==s.resourceOriginInputOwners.size()&&count==s.resourceOriginAuditOwners.size()) {
            bool sources=true,live=data.layers.size()==count;
            std::set<std::uint32_t> liveHandles;
            for(const auto& layer:data.layers)if(!liveHandles.insert(layer.handle).second)live=false;
            for(const auto& item:s.resourceOriginAudits) {
                const auto& audit=item.second;const auto input=s.resourceOriginInputs.find(item.first);
                const auto resource=s.resources.find(item.first);
                if(!audit||!audit->input||input==s.resourceOriginInputs.end()||resource==s.resources.end()
                    ||!same(audit->input,input->second)||!audit->input->raw||!audit->input->model||!audit->input->sourceModel
                    ||!same(audit->input->raw,resource->second.raw)||!same(audit->input->model,resource->second.model)
                    ||audit->input->index>=s.resourceOriginInputOwners.size()||audit->index>=s.resourceOriginAuditOwners.size()
                    ||!same(audit->input,s.resourceOriginInputOwners[audit->input->index])||!same(audit,s.resourceOriginAuditOwners[audit->index])
                    ||!same(audit->identities,s.identityAudit)||audit->input->container!=&data||audit->input->generation!=s.generation
                    ||audit->input->containerId!=data.m_entitiesContainerIdentity.value||!audit->input->context
                    ||audit->input->context->sourceVersion!=data.sourceVersion||audit->input->raw->m_version!=data.sourceVersion
                    ||audit->input->kind!=dxfRW::DxfResourceSourceView::Kind::Layer||!audit->input->paired
                    ||!audit->stateComplete||!audit->legacyPlotLayerBodyMatched||!audit->legacyPlotLayerOriginBound
                    ||!resource->second.legacyPlotLayerRawTypedMatched||audit->physical>=physical.size()
                    ||physical[audit->physical].kind!=DRW_DxfIdentitySource::Kind::TableRecord
                    ||!asciiIdentifierEquals(physical[audit->physical].name,"LAYER")) {sources=false;break;}
                const auto* original=dynamic_cast<const DRW_Layer*>(audit->input->sourceModel.get());
                const auto row=std::find_if(data.layers.begin(),data.layers.end(),[&](const auto& value){return value.handle==item.first;});
                if(!original||row==data.layers.end()||!layerSourceEqual(*original,*row,data.sourceVersion))live=false;
            }
            r.legacyLayerInventoryMatched=sources;
            r.legacyLiveLayerInventoryMatched=sources&&live;
            if(r.legacyLiveLayerInventoryMatched) {
                bool references=true;
                for(const auto& item:s.resourceOriginAudits) {
                    unsigned found=0u;
                    for(const auto& value:item.second->input->raw->groups)if(value.code()==390) {
                        std::uint64_t handle=0u;
                        if(++found!=1u||!dx_standard_output::handleValue(value,handle)
                            ||(handle!=0u&&handle!=a->defaultTarget))references=false;
                    }
                    if(found!=1u)references=false;
                }
                r.legacyLayerReferencesMatched=references;
            }
        }
    } catch (...) {r.legacyLayerInventoryMatched=false;r.legacyLiveLayerInventoryMatched=false;r.legacyLayerReferencesMatched=false;}
    // The selected source has exactly one physical LAYER and one plot-style
    // reference. This comparison still grants no object or output authority.
    if((data.sourceVersion==DRW::AC1015||s.legacyPlotAc1018Selected||(data.sourceVersion>=DRW::AC1021&&data.sourceVersion<=DRW::AC1032&&s.legacyPlotClassCount==2u)) && r.legacyBodiesMatched && s.identityInput && s.identityInput->source
        &&s.identityInput->source->records.size()<=4096u
        &&!s.resourceCaptureFailed &&!s.resourceProofCaptureFailed &&!s.resourceOriginCaptureFailed &&s.resourceOriginPending.empty()
        &&((data.sourceVersion>=DRW::AC1021 && s.resourceOriginInputs.size()==s.resourceOriginAudits.size()
            &&s.resourceOriginInputOwners.size()==s.resourceOriginInputs.size() &&s.resourceOriginAuditOwners.size()==s.resourceOriginAudits.size())
           ||(data.sourceVersion<DRW::AC1021 && s.resourceOriginRecords==1u &&s.resourceOriginInputs.size()==1u
            &&s.resourceOriginAudits.size()==1u &&s.resourceOriginInputOwners.size()==1u &&s.resourceOriginAuditOwners.size()==1u))) try {
        const auto& physical=s.identityInput->source->records;
        const auto count=std::count_if(physical.begin(),physical.end(),[](const auto& row) {
            return row.kind==DRW_DxfIdentitySource::Kind::TableRecord && asciiIdentifierEquals(row.name,"LAYER");
        });
        const auto input=s.resourceOriginInputs.find(0x10u);
        const auto auditIt=s.resourceOriginAudits.find(0x10u);
        if(input==s.resourceOriginInputs.end()||auditIt==s.resourceOriginAudits.end())return r;
        const auto& audit=auditIt->second;
        const auto resource=s.resources.find(input->first);
        if(count==1 && audit && audit->input && audit->input->raw && audit->input->sourceModel
            &&input->first==0x10u && auditIt->first==input->first
            &&same(audit->input,input->second) && resource!=s.resources.end()
            &&resource->second.family==dx_tableResourceFamily::Layer
            &&(data.sourceVersion>=DRW::AC1021?(resource->second.rawTypedMatched&&!resource->second.rawTypedMismatch):resource->second.legacyPlotLayerRawTypedMatched)
            &&same(audit->input->raw,resource->second.raw) && same(audit->input->model,resource->second.model)
            &&audit->input->context && audit->input->context->captured
            &&audit->input->context->sourceVersion==data.sourceVersion && audit->input->context->codecVersion==(data.sourceVersion>=DRW::AC1021?DRW::AC1021:DRW::AC1015)
            &&audit->input->context->binary!=audit->input->raw->hasRawValues && audit->input->raw->m_version==data.sourceVersion
            &&audit->physical<physical.size()
            &&audit->input->kind==dxfRW::DxfResourceSourceView::Kind::Layer && audit->input->paired
            &&(data.sourceVersion>=DRW::AC1021?(audit->originBound&&audit->bodyMatched):(audit->legacyPlotLayerOriginBound&&audit->legacyPlotLayerBodyMatched))&&audit->stateComplete
            &&audit->input->index<s.resourceOriginInputOwners.size() &&audit->index<s.resourceOriginAuditOwners.size()
            &&same(audit,s.resourceOriginAuditOwners[audit->index])
            &&same(audit->input,s.resourceOriginInputOwners[audit->input->index])&&same(audit->identities,s.identityAudit)
            &&audit->input->container==&data&&audit->input->generation==s.generation
            &&audit->input->containerId==data.m_entitiesContainerIdentity.value) {
            const auto* layer=dynamic_cast<const DRW_Layer*>(audit->input->sourceModel.get());
            const auto& raw=*audit->input->raw;
            const auto handle=[](const DRW_Variant& v,int code,std::uint32_t expected) {
                std::uint64_t actual=0u;return v.code()==code&&dx_standard_output::handleValue(v,actual)&&actual==expected;
            };
            const auto text=[](const DRW_Variant& v,int code,std::string_view expected) {
                return v.code()==code&&v.type()==DRW_Variant::STRING&&dx_attdef_source::text(v)==expected;
            };
            const auto integer=[](const DRW_Variant& v,int code,int expected) {
                return v.code()==code&&v.type()==DRW_Variant::INTEGER&&v.i_val()==expected;
            };
            r.legacyLayerMatched=layer&&raw.name=="LAYER"&&raw.handle==0x10u&&raw.parentHandle==2u
                &&raw.groups.size()==11u&&handle(raw.groups[0],5,0x10u)&&handle(raw.groups[1],330,2u)
                &&text(raw.groups[2],100,"AcDbSymbolTableRecord")&&text(raw.groups[3],100,"AcDbLayerTableRecord")
                &&text(raw.groups[4],2,"0")&&integer(raw.groups[5],70,0)&&integer(raw.groups[6],62,7)
                &&text(raw.groups[7],6,"CONTINUOUS")&&integer(raw.groups[8],290,1)
                &&integer(raw.groups[9],370,-3)&&handle(raw.groups[10],390,0xFu)
                &&layer->handle==0x10u&&layer->parentHandle==2u&&layer->handlePlotS=="F";
            if(r.legacyLayerMatched && data.layers.size()==1u)
                r.legacyLiveLayerMatched=layerSourceEqual(*layer,data.layers.front(),data.sourceVersion);
        }
    } catch (...) {r.legacyLayerMatched=false;r.legacyLiveLayerMatched=false;}
    return r;
}
