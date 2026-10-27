// SPDX-License-Identifier: GPL-2.0-or-later
// Included once after immutable rendering inputs and original dictionary helpers.
namespace {
bool renderingSourceEnvelope(const DRW_RawDxfObject& raw,const DRW_DxfIdentitySource::Record& physical) noexcept {
    // Raw callbacks omit ignored comments. Without per-field observations,
    // only a complete no-gap envelope establishes raw-index correspondence.
    return physical.marker.ordinal&&physical.endMarker.ordinal>physical.marker.ordinal
        &&physical.endMarker.ordinal-physical.marker.ordinal-1u==raw.groups.size();
}
template<class Context>bool renderingSourceName(std::string_view text,const Context& context,std::string& decoded) {
    if(text.empty()||text.find('\0')!=std::string_view::npos)return false;
    DRW_TextCodec codec;codec.setVersion(context.codecVersion,true);codec.setCodePage(context.codecCodePage,true);
    codec.setDxfTextEncoding(context.textEncoding);codec.setDxfCodePagePolicy(context.codePagePolicy);
    decoded=codec.toUtf8(text,!context.binary);
    if(codec.lastDecodeObservation().issues||decoded.empty())return false;
    for(const unsigned char value:decoded)if(value<32u||value==127u)return false;
    return true;
}
dx_tableResourceRole renderingSourceRole(dx_visualstyle_source::Role role) noexcept {
    using P=dx_visualstyle_source::Role;using R=dx_tableResourceRole;
    switch(role){case P::Self:return R::Self;case P::Owner:return R::Owner;case P::Reactor:return R::Reactor;
    case P::ExtensionDictionary:return R::ExtensionDictionary;case P::AppId:return R::AppIdName;
    case P::Layer:return R::LayerName;case P::Object:return R::XdataObjectId;default:return R::Unknown;}
}
}
void dx_iface::finishRenderingSources(dx_data& data) noexcept {
    using A=dx_data::TableDxfSourceCapture::RenderingSourceAudit;using S=A::State;
    using Record=dx_data::TableDxfSourceCapture::RenderingSourceRecord;using Role=dx_tableResourceRole;
    auto& s=data.m_tableDxfSource;if(s.renderingSourceFailed||s.renderingSourceLimited)return;
    const auto limit=[&](){s.renderingSourceLimited=true;};
    try {
        std::size_t bytes=sizeof(A),work=sizeof(A),groups=0u;
        if(!s.renderingAudit)return;
        for(const auto& item:s.renderingInputs)if(item.second&&item.second->typedRaw){
            std::size_t payload=0u;
            if(!addAssociativeRetainedBytes(item.second->typedRaw->groups.size(),groups)
                ||!estimateRawDxfObjectBytes(*item.second->typedRaw,payload)
                ||!addAssociativeSlotBytes(payload,8u,bytes)||!addAssociativeSlotBytes(payload,16u,work)){limit();return;}}
        if(!addAssociativeSlotBytes(s.renderingInputs.size(),sizeof(Record)+128u,bytes)
            ||!addAssociativeSlotBytes(groups,sizeof(dx_visualstyle_source::Span)+sizeof(dx_tableResourceReference)+128u,bytes)
            ||(!s.renderingInputs.empty()&&(!addAssociativeSlotBytes(s.structuralAudits.size(),128u,bytes)
                ||!addAssociativeSlotBytes(s.structuralBytes,4u,bytes)))
            ||!addAssociativeSlotBytes(groups,256u,work)||!addAssociativeRetainedBytes(bytes,work)
            ||s.renderingSourceBytes>s.maxRenderingSourceBytes||bytes>s.maxRenderingSourceBytes-s.renderingSourceBytes
            ||s.renderingSourceWork>s.maxRenderingSourceWork||work>s.maxRenderingSourceWork-s.renderingSourceWork){limit();return;}
        s.renderingSourceBytes+=bytes;s.renderingSourceWork+=work;
        auto audit=std::make_shared<A>();audit->container=&data;audit->generation=s.generation;audit->containerId=data.m_entitiesContainerIdentity.value;
        audit->inputs=s.renderingAudit;audit->physicalRecords=s.renderingAudit->physicalRecords;
        const auto save=[&](S state){audit->state=state;s.renderingSourceAudit=std::move(audit);s.renderingSourceOwner=s.renderingSourceAudit;};
        const auto input=renderingInputReport(data);const auto model=renderingModelReport(data);
        if(input.state==RenderingInputState::ContextMismatch){save(S::ContextMismatch);return;}
        if(input.state==RenderingInputState::CaptureFailed||input.state==RenderingInputState::SourceNotReady
            ||model.state==RenderingModelState::CaptureFailed||model.state==RenderingModelState::Limited){save(S::SourceNotReady);return;}
        bool complete=true;
        for(const auto& item:audit->inputs->rows){const auto& original=item.second;const auto& in=*original.input;
            auto row=std::make_shared<Record>();row->input=original.input;row->physical=original.physical;row->originBound=original.originBound;
            row->index=audit->owners.size();
            if(in.original.kind==RenderingView::Kind::VisualStyle&&original.modeledStateMatched&&original.replayDecoded&&in.visualPropertiesCaptured
                &&!in.original.originalPendingXdata&&!in.original.originalCoordinateMask&&original.originBound){
                row->profile=dx_visualstyle_source::audit(*in.typedRaw);
                const auto& physical=audit->inputs->identity->source->records.at(row->physical);
                // The existing raw callback omits ignored comments. Without
                // per-field physical observations, a gap cannot be assigned
                // to a raw index. Do not invent ordinal/offset correspondence.
                if(!renderingSourceEnvelope(*in.typedRaw,physical)){
                    row->profile.reason=dx_visualstyle_source::Reason::PhysicalEnvelope;row->profile.rejected=in.typedRaw->groups.size();row->profile.spans.clear();}
                const auto* typed=dynamic_cast<const DRW_VisualStyle*>(in.typedModel.get());const auto* raw=dynamic_cast<const DRW_VisualStyle*>(in.rawModel.get());
                row->bodyMatched=typed&&raw&&in.context&&dx_visualstyle_source::matches(row->profile,*in.typedRaw,*typed,in.typedVisualProperties,*in.context)
                    &&dx_visualstyle_source::matches(row->profile,*in.rawRaw,*raw,in.rawVisualProperties,*in.context);
                if(row->bodyMatched)for(const auto& span:row->profile.spans)if(span.role!=dx_visualstyle_source::Role::None)
                    row->references.push_back({renderingSourceRole(span.role),span.group,span.code,span.target});
            }
            complete=complete&&row->bodyMatched&&row->originBound;
            audit->owners.push_back(row);audit->records.emplace(item.first,std::move(row));
        }
        if(audit->records.empty()&&!audit->physicalRecords){save(S::NoSources);return;}
        // Root and dictionary inputs are already bounded/owned. Their scans,
        // name decoding and temporary reference maps are charged above too.
        unsigned roots=0u;for(const auto& item:s.structuralAudits){const auto& row=item.second;
            if(row&&row->bodyMatched&&row->stateComplete&&row->originBound&&row->input&&row->input->paired
                &&row->input->kind==dxfRW::DxfStructuralSourceView::Kind::RootDictionary){audit->root=row;++roots;}}
        if(roots==1u){const auto& root=*audit->root->input;std::vector<dx_tableResourceReference> references;unsigned names=0u;std::uint32_t dictionary=0u;
            bool known=root.raw&&root.context&&root.raw->parentHandle==0u
                &&renderingSourceEnvelope(*root.raw,audit->inputs->identity->source->records.at(audit->root->physical))
                &&spineSourceReferences(*root.raw,true,references);
            if(known)for(const auto& ref:references){if(ref.kind==Role::Unknown){known=false;break;}
                if(ref.kind!=Role::DictionarySoftOwner&&ref.kind!=Role::DictionaryHardOwner)continue;
                if(!ref.groupIndex||root.raw->groups[ref.groupIndex-1u].code()!=3){known=false;break;}
                std::string key;if(!renderingSourceName(dx_attdef_source::text(root.raw->groups[ref.groupIndex-1u]),*root.context,key)){known=false;break;}
                if(asciiIdentifierEquals(key,"ACAD_VISUALSTYLE")){dictionary=ref.value;++names;audit->rootKeyGroup=ref.groupIndex-1u;audit->rootTargetGroup=ref.groupIndex;}}
            const auto found=s.structuralAudits.find(dictionary);
            if(known&&names==1u&&found!=s.structuralAudits.end()){const auto& row=found->second;
                if(row&&row->bodyMatched&&row->stateComplete&&row->originBound&&row->input&&row->input->paired&&row->input->raw&&row->input->context
                    &&asciiIdentifierEquals(row->input->raw->name,"DICTIONARY")){
                    audit->dictionary=row;references.clear();
                    known=renderingSourceEnvelope(*row->input->raw,audit->inputs->identity->source->records.at(row->physical))
                        &&spineSourceReferences(*row->input->raw,true,references);
                    std::map<std::uint32_t,std::size_t> counts;std::set<std::string> keys;
                    if(known)for(const auto& ref:references){if(ref.kind==Role::Unknown){known=false;break;}
                        if(ref.kind!=Role::DictionarySoftOwner&&ref.kind!=Role::DictionaryHardOwner)continue;
                        if(!ref.groupIndex||row->input->raw->groups[ref.groupIndex-1u].code()!=3){known=false;break;}
                        std::string key,folded;if(!renderingSourceName(dx_attdef_source::text(row->input->raw->groups[ref.groupIndex-1u]),*row->input->context,key)
                            ||!dx_symbol_output::nameKey(key,folded)||!keys.insert(std::move(folded)).second){known=false;break;}
                        ++counts[ref.value];}
                    if(known)for(const auto& ref:references){if(ref.kind!=Role::DictionarySoftOwner&&ref.kind!=Role::DictionaryHardOwner)continue;
                        const auto target=audit->records.find(ref.value);if(target==audit->records.end())continue;
                        // Mutable construction owner only; sealed records below
                        // are const. No borrowed model/raw pointer is retained.
                        auto value=std::const_pointer_cast<Record>(target->second);
                        value->wrongOwner=row->input->raw->parentHandle!=root.raw->handle||value->input->typedRaw->parentHandle!=dictionary;
                        value->namedTarget=value->bodyMatched&&value->originBound&&!value->wrongOwner&&counts.at(ref.value)==1u;
                        value->nameGroup=ref.groupIndex-1u;value->targetGroup=ref.groupIndex;}
                }
            }
        }
        save(audit->records.empty()&&!audit->physicalRecords?S::NoSources:complete&&audit->records.size()==audit->physicalRecords?S::SourcesReady:S::Unqualified);
    }catch(...){s.renderingSourceFailed=true;}
}

dx_iface::RenderingSourceReport dx_iface::renderingSourceReport(const dx_data& data) noexcept {
    using S=RenderingSourceState;RenderingSourceReport report;const auto& s=data.m_tableDxfSource;
    report.bytes=s.renderingSourceBytes;report.work=s.renderingSourceWork;
    const auto input=renderingInputReport(data);const auto model=renderingModelReport(data);
    switch(input.state){case RenderingInputState::NotCaptured:return report;case RenderingInputState::Reading:report.state=S::Reading;return report;
    case RenderingInputState::ImportFailed:report.state=S::ImportFailed;return report;case RenderingInputState::ContextMismatch:report.state=S::ContextMismatch;return report;
    case RenderingInputState::CaptureFailed:case RenderingInputState::SourceNotReady:report.state=S::SourceNotReady;return report;default:break;}
    if(s.renderingSourceFailed){report.state=S::CaptureFailed;return report;}if(s.renderingSourceLimited){report.state=S::Limited;return report;}
    const auto& audit=s.renderingSourceAudit;if(!audit)return report;
    const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    if(!same(audit,s.renderingSourceOwner)||audit->container!=&data||audit->generation!=s.generation||audit->containerId!=data.m_entitiesContainerIdentity.value
        ||!same(audit->inputs,s.renderingAudit)){
        report.state=S::ContextMismatch;return report;}
    if(model.state==RenderingModelState::CaptureFailed||model.state==RenderingModelState::Limited){report.state=S::SourceNotReady;return report;}
    if(audit->records.size()!=audit->owners.size()||audit->records.size()!=s.renderingInputs.size()){report.state=S::ContextMismatch;return report;}
    const auto structural=structuralSourceReport(data);
    if(structural.state==StructuralSourceState::ContextMismatch){report.state=S::ContextMismatch;return report;}
    for(const auto* pointer:{&audit->root,&audit->dictionary})if(*pointer){const auto& row=**pointer;
        if(!row.input||!row.input->raw){report.state=S::ContextMismatch;return report;}
        const auto found=s.structuralAudits.find(row.input->raw->handle);
        if(found==s.structuralAudits.end()||!same(*pointer,found->second)){report.state=S::ContextMismatch;return report;}}
    if(structural.state==StructuralSourceState::CaptureFailed){report.state=S::SourceNotReady;return report;}
    for(const auto& item:audit->records){const auto& row=item.second;const auto found=s.renderingInputs.find(item.first);
        if(!row||found==s.renderingInputs.end()||!same(row->input,found->second)||row->index>=audit->owners.size()||!same(row,audit->owners[row->index])){
            report.state=S::ContextMismatch;return report;}
        const auto source=audit->inputs->rows.find(item.first);
        if(source==audit->inputs->rows.end()||!same(source->second.input,row->input)||source->second.physical!=row->physical||source->second.originBound!=row->originBound){report.state=S::ContextMismatch;return report;}
        ++report.records;if(row->bodyMatched){++report.profiles;report.fields+=row->profile.spans.size();report.references+=row->references.size();}
        if(row->originBound)++report.origins;if(row->namedTarget)++report.namedTargets;
    }
    report.physical=audit->physicalRecords;report.state=audit->state;
    report.originalSourcesMatched=report.state==S::SourcesReady&&report.records&&report.records==report.profiles&&report.records==report.origins&&report.records==report.physical;
    return report;
}
