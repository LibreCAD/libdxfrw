// SPDX-License-Identifier: GPL-2.0-or-later
// Included once after the original source-body and namespace helpers.
namespace {
dx_tableResourceRole viewUcsRole(dx_viewucs_source::Role role) noexcept {
    using S=dx_viewucs_source::Role;using R=dx_tableResourceRole;
    switch(role){case S::Self:return R::Self;case S::Owner:return R::Owner;case S::Reactor:return R::Reactor;
    case S::ExtensionDictionary:return R::ExtensionDictionary;case S::AppId:return R::AppIdName;case S::Layer:return R::LayerName;case S::Object:return R::XdataObjectId;
    case S::NamedUcs:return R::LayoutNamedUcs;case S::BaseUcs:return R::LayoutBaseUcs;case S::Background:return R::ViewBackground;
    case S::LiveSection:return R::ViewLiveSection;case S::VisualStyle:return R::VisualStyle;case S::Sun:return R::ViewSun;default:return R::Unknown;}
}
}
void dx_iface::addDxfViewUcsSource(const DRW_TableEntry& model,const DRW_RawDxfObject& raw){observeViewUcsSource(&model,&raw);}
void dx_iface::observeViewUcsSource(const DRW_TableEntry* model,const DRW_RawDxfObject* raw) noexcept {
  if (!tableSourceCaptureOpen())
    return;
  auto &s = cData->m_tableDxfSource;
  if (s.viewUcsCaptureFailed)
    return;
  using V = dxfRW::DxfViewUcsSourceView;
  using C = dx_data::TableDxfSourceCapture::EntitiesReadContext;
  using Input = dx_data::TableDxfSourceCapture::ViewUcsInput;
  V view;
  dxfRW::DxfReadContext live;
  if (!s.entitiesReadInput ||
      !s.entitiesReadInput->getDxfViewUcsSourceView(view))
    return;
  const auto fail = [&]() { s.viewUcsCaptureFailed = true; };
  if (!s.entitiesReadInput->getDxfReadContext(live) ||
      live.sourceVersion < DRW::AC1021 || live.sourceVersion > DRW::AC1032)
    return;
  const bool isView = view.kind == V::Kind::View;
  if (!model || model != view.model || !view.source ||
      (raw && raw != view.source) ||
      view.phase != (raw ? V::Phase::Source : V::Phase::Typed) ||
      !model->handle || view.source->handle != model->handle ||
      view.source->parentHandle != model->parentHandle ||
      view.source->m_version != live.sourceVersion ||
      view.source->hasRawValues == live.binary ||
      !asciiIdentifierEquals(view.source->name, isView ? "VIEW" : "UCS") ||
      (isView ? typeid(*model) != typeid(DRW_View)
              : typeid(*model) != typeid(DRW_UCS))) {
    fail();
    return;
  }
    const auto state=std::make_pair(view.originalPendingXdata,view.originalCoordinateMask);
    try {
        if(raw){
            if(s.viewUcsPending.count(model->handle)||s.viewUcsInputs.count(model->handle)||s.viewUcsRecords>=s.maxViewUcsRecords
                ||s.viewUcsGroups>s.maxViewUcsGroups||raw->groups.size()>s.maxViewUcsGroups-s.viewUcsGroups||live.codecCodePage.size()>128u){fail();return;}
            std::size_t bytes=sizeof(Input)+sizeof(dx_data::TableDxfSourceCapture::ViewUcsPending)+sizeof(dx_data::TableDxfSourceCapture::ViewUcsRecord)+sizeof(C)+96u*sizeof(void*),rawBytes=0u,modelBytes=0u,items=0u;
            if(!estimateRawDxfObjectBytes(*raw,rawBytes)||!(isView?estimateViewTypedBytes(static_cast<const DRW_View&>(*model),modelBytes,items):estimateUcsTypedBytes(static_cast<const DRW_UCS&>(*model),modelBytes,items))
                ||!addAssociativeSlotBytes(rawBytes,16u,bytes)||!addAssociativeSlotBytes(modelBytes,4u,bytes)||!addAssociativeSlotBytes(raw->groups.size(),sizeof(dx_viewucs_source::Span)+sizeof(dx_tableResourceReference)+128u,bytes)
                ||!addAssociativeRetainedBytes(live.codecCodePage.size()+live.declaration.size(),bytes)||s.viewUcsBytes>s.maxViewUcsBytes||bytes>s.maxViewUcsBytes-s.viewUcsBytes
                ||s.viewUcsWork>s.maxViewUcsWork||bytes>s.maxViewUcsWork-s.viewUcsWork){fail();return;}
            s.viewUcsBytes+=bytes;s.viewUcsWork+=bytes;s.viewUcsInputBytes=s.viewUcsBytes;s.viewUcsInputWork=s.viewUcsWork;s.viewUcsGroups+=raw->groups.size();++s.viewUcsRecords;
            auto context=std::make_shared<C>();context->sourceVersion=live.sourceVersion;context->codecVersion=live.codecVersion;context->codecCodePage=live.codecCodePage;context->declaration=live.declaration;
            context->hasDeclaration=live.hasDeclaration;context->compatibilityProfile=live.compatibilityProfile;context->textEncoding=live.textEncoding;context->codePagePolicy=live.codePagePolicy;
            context->binary=live.binary;context->applyExtrusion=live.applyExtrusion;context->captured=true;
            dx_data::TableDxfSourceCapture::ViewUcsPending pending;auto& in=pending.input;in.container=cData;in.generation=s.generation;in.containerId=cData->m_entitiesContainerIdentity.value;
            in.view=isView;in.state=state;in.payloadComplete=view.originalPayloadComplete;in.context=std::move(context);in.raw=std::make_shared<const DRW_RawDxfObject>(*raw);
            if(isView)in.model=std::make_shared<const DRW_View>(static_cast<const DRW_View&>(*model));else in.model=std::make_shared<const DRW_UCS>(static_cast<const DRW_UCS&>(*model));
            in.paired=dxfViewUcs::Decoder::observe(*model)==state;pending.model=model;pending.raw=raw;s.viewUcsPending.emplace(model->handle,std::move(pending));return;
        }
        const auto found=s.viewUcsPending.find(model->handle);if(found==s.viewUcsPending.end()){fail();return;}
        auto& pending=found->second;auto& in=pending.input;const auto& c=*in.context;
        if(pending.model!=model||pending.raw!=view.source||in.view!=isView||in.state!=state||in.payloadComplete!=view.originalPayloadComplete
            ||std::tie(c.sourceVersion,c.codecVersion,c.codecCodePage,c.declaration,c.hasDeclaration,c.compatibilityProfile,c.textEncoding,c.codePagePolicy,c.binary,c.applyExtrusion)
                !=std::tie(live.sourceVersion,live.codecVersion,live.codecCodePage,live.declaration,live.hasDeclaration,live.compatibilityProfile,live.textEncoding,live.codePagePolicy,live.binary,live.applyExtrusion)){fail();return;}
        in.paired=in.paired&&dxfViewUcs::Decoder::observe(*model)==state&&retainedRawDxfObjectsEqual(*in.raw,*view.source)
            &&dxfViewUcs::Decoder::sameModel(*in.model,*model,[&](const auto& a,const auto& b){return standardDwgCommonEqual(a,b,live.sourceVersion);});
        in.index=s.viewUcsInputOwners.size();auto saved=std::make_shared<const Input>(std::move(in));s.viewUcsInputs.emplace(model->handle,saved);s.viewUcsInputOwners.push_back(std::move(saved));s.viewUcsPending.erase(found);
    }catch(...){fail();}
}
void dx_iface::finishViewUcsSources(dx_data& data) noexcept {
    auto& s=data.m_tableDxfSource;if(s.viewUcsCaptureFailed)return;using A=dx_data::TableDxfSourceCapture::ViewUcsAudit;using S=A::State;using R=dx_data::TableDxfSourceCapture::ViewUcsRecord;using I=DRW_DxfIdentitySource;
    try {
        const auto count=s.identityInput&&s.identityInput->source?s.identityInput->source->records.size():0u;std::size_t cost=sizeof(A)+128u;
        if(!addAssociativeSlotBytes(count,128u,cost)||s.viewUcsBytes>s.maxViewUcsBytes||sizeof(A)>s.maxViewUcsBytes-s.viewUcsBytes||s.viewUcsWork>s.maxViewUcsWork||cost>s.maxViewUcsWork-s.viewUcsWork){s.viewUcsCaptureFailed=true;return;}
        s.viewUcsBytes+=sizeof(A);s.viewUcsWork+=cost;auto audit=std::make_shared<A>();audit->container=&data;audit->generation=s.generation;audit->containerId=data.m_entitiesContainerIdentity.value;
        audit->identity=s.identityInput;audit->identities=s.identityAudit;const auto save=[&](S state){audit->state=state;s.viewUcsAudit=std::move(audit);s.viewUcsOwner=s.viewUcsAudit;};
        if(identitySourceReport(data).state!=IdentitySourceState::NamespaceReady){save(S::SourceNotReady);return;}
        const auto& physical=*audit->identity->source;for(const auto& p:physical.records)if(p.kind==I::Kind::TableRecord&&(asciiIdentifierEquals(p.name,"VIEW")||asciiIdentifierEquals(p.name,"UCS")))++audit->physicalRecords;
        bool complete=audit->physicalRecords==s.viewUcsInputs.size();
        for(const auto& item:s.viewUcsInputs){auto row=std::make_shared<R>();row->input=item.second;const auto& in=*row->input;
            row->stateComplete=in.paired&&in.payloadComplete&&!in.state.first&&!in.state.second;row->profile=dx_viewucs_source::audit(*in.raw,in.view);
            if(row->stateComplete&&row->profile.complete){
                std::string wire;if(!tableResourceSourceBody(*in.raw,s.maxViewUcsBytes,wire)){s.viewUcsCaptureFailed=true;return;}
                dxfRW::DxfObjectsWriteView view{};view.bytes=wire;view.version=in.context->sourceVersion;view.binary=in.context->binary;view.codePage=in.context->codecCodePage;view.compatibilityProfile=in.context->compatibilityProfile;
                dx_standard_output::InputBuffer buffer(wire);std::istream input(&buffer);auto budget=std::make_shared<DxfReadRetainedBudget>(s.maxViewUcsBytes);auto reader=dx_standard_output::readerFor(input,view,budget);
                if(!reader){s.viewUcsCaptureFailed=true;return;}reader->setDxfCodePagePolicy(in.context->codePagePolicy);reader->setDxfTextEncoding(in.context->textEncoding);reader->setSemanticStringMode(true);
                reader->setRecordBudget(in.raw->groups.size());reader->setTypedVariantNodeBudget(in.raw->groups.size());reader->setDecodeWorkBudget(s.maxViewUcsWork-s.viewUcsWork);
                const auto replay=[&](auto& model){dxfViewUcs::Decoder decoder(model);int code=0;while(buffer.position()<wire.size())if(!reader->readRec(&code)||!code||!decoder.parseCode(code,reader))return false;
                    return decoder.finish()&&reader->recordCount()==in.raw->groups.size()&&!budget->exceeded()&&!reader->decodeWorkBudgetExceeded()
                        &&!reader->recordBudgetExceeded()&&!reader->typedVariantNodeBudgetExceeded()&&!reader->tokenByteBudgetExceeded()&&!reader->inputByteBudgetExceeded()
                        &&dxfViewUcs::Decoder::observe(model)==in.state&&dxfViewUcs::Decoder::sameModel(model,*in.model,[&](const auto& a,const auto& b){return standardDwgCommonEqual(a,b,in.context->sourceVersion);});};
                if(in.view){DRW_View parsed;row->bodyMatched=replay(parsed);}else {DRW_UCS parsed;row->bodyMatched=replay(parsed);}
                if(!addAssociativeRetainedBytes(reader->decodeWorkCount(),s.viewUcsWork)||s.viewUcsWork>s.maxViewUcsWork){s.viewUcsCaptureFailed=true;return;}
            }
            for(const auto& span:row->profile.spans)if(span.role!=dx_viewucs_source::Role::None&&span.role!=dx_viewucs_source::Role::Name)row->references.push_back({viewUcsRole(span.role),span.group,span.code,span.target});
            const auto id=audit->identities->identities.find(item.first),owner=audit->identities->identities.find(in.raw->parentHandle);
            if(id!=audit->identities->identities.end()&&id->second<physical.records.size()){
                const auto& p=physical.records[id->second];row->originBound=p.kind==I::Kind::TableRecord&&asciiIdentifierEquals(p.name,in.view?"VIEW":"UCS")&&p.table<physical.records.size()
                    &&physical.records[p.table].kind==I::Kind::TableControl&&asciiIdentifierEquals(physical.records[p.table].tableName,in.view?"VIEW":"UCS")&&physical.records[p.table].section==p.section
                    &&p.section<physical.sections.size()&&physical.sections[p.section].kind==I::Section::Tables&&owner!=audit->identities->identities.end()&&owner->second==p.table;
                if(row->originBound)row->physical=id->second;}
            complete=complete&&row->stateComplete&&row->bodyMatched&&row->originBound;audit->records.emplace(item.first,std::move(row));}
        save(!audit->physicalRecords&&!s.viewUcsRecords?S::NoSources:complete?S::SourcesReady:S::Unqualified);
    }catch(...){s.viewUcsCaptureFailed=true;}
}
dx_iface::ViewUcsReport dx_iface::viewUcsSourceReport(const dx_data& data) noexcept {
    using A=dx_data::TableDxfSourceCapture::ViewUcsAudit;using S=A::State;ViewUcsReport r;const auto& s=data.m_tableDxfSource;r.records=s.viewUcsRecords;r.bytes=s.viewUcsBytes;r.work=s.viewUcsWork;
    if (!s.generation || data.sourceFormat != dx_source_format::DXF)
      return r;
    if (!s.closed) {
      r.state = S::Reading;
      return r;
    }
    if (!s.successful) {
      r.state = S::ImportFailed;
      return r;
    }
    if(s.contextFailed||s.generation!=1u||s.generation!=data.importGeneration||s.reader||s.entitiesReadInput){r.state=S::ContextMismatch;return r;}
    if(s.viewUcsCaptureFailed||!s.viewUcsPending.empty()){r.state=S::CaptureFailed;return r;}
    const auto& a=s.viewUcsAudit;if(!a)return r;const auto same=[](const auto& x,const auto& y){return x.get()==y.get()&&!x.owner_before(y)&&!y.owner_before(x);};
    if(!same(a,s.viewUcsOwner)||a->container!=&data||a->generation!=s.generation||a->containerId!=data.m_entitiesContainerIdentity.value||!same(a->identity,s.identityInput)||!same(a->identities,s.identityAudit)
        ||s.viewUcsInputs.size()!=s.viewUcsInputOwners.size()||a->records.size()!=s.viewUcsInputs.size()){r.state=S::ContextMismatch;return r;}
    const auto identity=identitySourceReport(data);if(identity.state==IdentitySourceState::ContextMismatch){r.state=S::ContextMismatch;return r;}if(identity.state!=IdentitySourceState::NamespaceReady){r.state=S::SourceNotReady;return r;}
    for(const auto& item:a->records){const auto& row=*item.second;const auto& in=*row.input;const auto found=s.viewUcsInputs.find(item.first);
        if(found==s.viewUcsInputs.end()||!same(row.input,found->second)||in.index>=s.viewUcsInputOwners.size()||!same(row.input,s.viewUcsInputOwners[in.index])
            ||in.container!=&data||in.generation!=s.generation||in.containerId!=data.m_entitiesContainerIdentity.value||!in.context||in.context->sourceVersion!=s.version){r.state=S::ContextMismatch;return r;}
        if (in.paired)
          ++r.paired;
        if (row.bodyMatched)
          ++r.bodies;
        if (row.originBound)
          ++r.origins;
    }
    r.physical=a->physicalRecords;r.state=a->state;r.originalSourcesMatched=r.state==S::SourcesReady&&r.records&&r.records==r.bodies&&r.records==r.origins;return r;
}
