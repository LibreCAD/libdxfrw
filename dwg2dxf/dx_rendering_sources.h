// SPDX-License-Identifier: GPL-2.0-or-later
// Included once after immutable rendering inputs and original dictionary helpers.
namespace {
bool renderingSourceFields(const DRW_RawDxfObject& raw,const DRW_DxfIdentitySource::Record& physical,
    const std::shared_ptr<const DRW_DxfObjectPhysicalSource>& source,DRW_DxfObjectPhysicalSource::Status status,
    std::vector<std::size_t>& fields) {
    using S=DRW_DxfObjectPhysicalSource::Status;
    const auto same=[](const auto& a,const auto& b){return std::tie(a.ordinal,a.beginOffset,a.endOffset,a.ignoredComment)
        ==std::tie(b.ordinal,b.beginOffset,b.endOffset,b.ignoredComment);};
    if(status!=S::Complete||!source||source->status!=status||source->version!=raw.m_version||source->binary==raw.hasRawValues
        ||source->name!=raw.name||!same(source->marker,physical.marker)||!same(source->endMarker,physical.endMarker)
        ||!physical.marker.ordinal||physical.endMarker.ordinal<=physical.marker.ordinal
        ||physical.endMarker.ordinal-physical.marker.ordinal-1u!=source->fields.size())return false;
    fields.clear();fields.reserve(raw.groups.size());auto ordinal=physical.marker.ordinal+1u,offset=physical.marker.endOffset;
    for(std::size_t index=0u;index<source->fields.size();++index){const auto& field=source->fields[index];const auto& span=field.span;
        if(field.code<=0||span.ordinal!=ordinal++||span.beginOffset<offset||span.endOffset<=span.beginOffset
            ||span.endOffset>physical.endMarker.beginOffset)return false;
        offset=span.endOffset;
        if(span.ignoredComment){if(field.code!=999)return false;continue;}
        if(fields.size()>=raw.groups.size()||field.code!=raw.groups[fields.size()].code())return false;
        fields.push_back(index);
    }
    return fields.size()==raw.groups.size();
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
    const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    const auto limit=[&](){s.renderingSourceLimited=true;};
    try {
        std::size_t bytes=sizeof(A),work=sizeof(A),groups=0u;
        if(!s.renderingAudit)return;
        for(const auto& item:s.renderingInputs)if(item.second&&item.second->typedRaw){
            std::size_t payload=0u;
            if(!addAssociativeRetainedBytes(item.second->typedRaw->groups.size(),groups)
                ||!estimateRawDxfObjectBytes(*item.second->typedRaw,payload)
                ||!addAssociativeSlotBytes(payload,8u,bytes)||!addAssociativeSlotBytes(payload,16u,work)
                ||(item.second->physical&&(!addAssociativeRetainedBytes(item.second->physical->bytes,bytes)
                    ||!addAssociativeRetainedBytes(item.second->physical->work,work)))){limit();return;}}
        if(!s.renderingInputs.empty())for(const auto& item:s.structuralAudits)if(item.second&&item.second->input&&item.second->input->physical)
            if(!addAssociativeRetainedBytes(item.second->input->physical->bytes,bytes)
                ||!addAssociativeRetainedBytes(item.second->input->physical->work,work)){limit();return;}
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
                if(!same(in.physical,in.physicalOwner)
                    ||!renderingSourceFields(*in.typedRaw,physical,in.physical,in.physicalStatus,row->physicalFields)){
                    row->profile.reason=dx_visualstyle_source::Reason::PhysicalEnvelope;row->profile.rejected=in.typedRaw->groups.size();row->profile.spans.clear();}
                const auto* typed=dynamic_cast<const DRW_VisualStyle*>(in.typedModel.get());const auto* raw=dynamic_cast<const DRW_VisualStyle*>(in.rawModel.get());
                row->bodyMatched=typed&&raw&&in.context&&dx_visualstyle_source::matches(row->profile,*in.typedRaw,*typed,in.typedVisualProperties,*in.context)
                    &&dx_visualstyle_source::matches(row->profile,*in.rawRaw,*raw,in.rawVisualProperties,*in.context);
                if(row->bodyMatched)for(const auto& span:row->profile.spans)if(span.role!=dx_visualstyle_source::Role::None)
                    row->references.push_back({renderingSourceRole(span.role),span.group,span.code,span.target});
            }else if(in.original.kind==RenderingView::Kind::RenderSettings&&original.modeledStateMatched&&original.replayDecoded
                &&!in.original.originalPendingXdata&&!in.original.originalCoordinateMask&&original.originBound){
                row->preset=dx_rendersettings_source::audit(*in.typedRaw);
                const auto& physical=audit->inputs->identity->source->records.at(row->physical);
                if(!same(in.physical,in.physicalOwner)
                    ||!renderingSourceFields(*in.typedRaw,physical,in.physical,in.physicalStatus,row->physicalFields)){
                    row->preset.reason=dx_visualstyle_source::Reason::PhysicalEnvelope;row->preset.spans.clear();}
                const auto* typed=dynamic_cast<const DRW_RenderSettings*>(in.typedModel.get());const auto* raw=dynamic_cast<const DRW_RenderSettings*>(in.rawModel.get());
                row->bodyMatched=typed&&raw&&in.context&&dx_rendersettings_source::matches(row->preset,*in.typedRaw,*typed,*in.context)
                    &&dx_rendersettings_source::matches(row->preset,*in.rawRaw,*raw,*in.context);
                if(row->bodyMatched)for(const auto& span:row->preset.spans)if(span.role!=dx_visualstyle_source::Role::None)
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
                &&same(root.physical,root.physicalOwner)
                &&renderingSourceFields(*root.raw,audit->inputs->identity->source->records.at(audit->root->physical),
                    root.physical,root.physicalStatus,audit->rootFields)
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
                    known=same(row->input->physical,row->input->physicalOwner)
                        &&renderingSourceFields(*row->input->raw,audit->inputs->identity->source->records.at(row->physical),
                            row->input->physical,row->input->physicalStatus,audit->dictionaryFields)
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
        const bool presets=std::any_of(audit->records.begin(),audit->records.end(),[](const auto& item){return item.second->preset.complete();});
        if(presets&&roots==1u){const auto& root=*audit->root->input;std::vector<dx_tableResourceReference> references;
            bool known=root.raw&&root.context&&root.raw->parentHandle==0u&&same(root.physical,root.physicalOwner)
                &&renderingSourceFields(*root.raw,audit->inputs->identity->source->records.at(audit->root->physical),root.physical,root.physicalStatus,audit->rootFields)
                &&spineSourceReferences(*root.raw,true,references);
            std::map<std::string,std::size_t> names;std::map<std::uint32_t,std::size_t> targets;std::map<std::size_t,std::string> keys;
            if(known)for(const auto& ref:references){if(ref.kind==Role::Unknown){known=false;break;}
                if(ref.kind!=Role::DictionarySoftOwner&&ref.kind!=Role::DictionaryHardOwner)continue;
                std::string key;if(!ref.groupIndex||root.raw->groups[ref.groupIndex-1u].code()!=3
                    ||!renderingSourceName(dx_attdef_source::text(root.raw->groups[ref.groupIndex-1u]),*root.context,key)){known=false;break;}
                std::string folded;if(!dx_symbol_output::nameKey(key,folded)){known=false;break;}++names[folded];++targets[ref.value];keys.emplace(ref.groupIndex,std::move(key));}
            if(known)for(const auto& ref:references){const auto key=keys.find(ref.groupIndex);if(key==keys.end()||!dx_rendering_roots::known(key->second))continue;
                std::string folded;if(!dx_symbol_output::nameKey(key->second,folded)||names.at(folded)!=1u||targets.at(ref.value)!=1u)continue;
                const auto found=s.structuralAudits.find(ref.value);if(found==s.structuralAudits.end())continue;const auto& dictionary=found->second;
                if(!dictionary||!dictionary->bodyMatched||!dictionary->stateComplete||!dictionary->originBound||!dictionary->input||!dictionary->input->paired
                    ||!dictionary->input->raw||!dictionary->input->context||!asciiIdentifierEquals(dictionary->input->raw->name,"DICTIONARY"))continue;
                auto binding=std::make_shared<dx_data::TableDxfSourceCapture::RenderingPresetRoot>();binding->dictionary=dictionary;binding->key=key->second;
                binding->keyGroup=ref.groupIndex-1u;binding->targetGroup=ref.groupIndex;
                const auto& in=*dictionary->input;std::vector<dx_tableResourceReference> members;
                if(!same(in.physical,in.physicalOwner)||!renderingSourceFields(*in.raw,audit->inputs->identity->source->records.at(dictionary->physical),
                    in.physical,in.physicalStatus,binding->fields)||!spineSourceReferences(*in.raw,true,members))continue;
                std::map<std::uint32_t,std::size_t> counts;std::set<std::string> memberKeys;bool membersKnown=true;
                for(const auto& member:members){if(member.kind==Role::Unknown){membersKnown=false;break;}
                    if(member.kind!=Role::DictionarySoftOwner&&member.kind!=Role::DictionaryHardOwner)continue;
                    std::string name,fold;if(!member.groupIndex||in.raw->groups[member.groupIndex-1u].code()!=3
                        ||!renderingSourceName(dx_attdef_source::text(in.raw->groups[member.groupIndex-1u]),*in.context,name)
                        ||!dx_symbol_output::nameKey(name,fold)||!memberKeys.insert(std::move(fold)).second){membersKnown=false;break;}++counts[member.value];}
                if(!membersKnown)continue;
                audit->presetRoots.emplace(ref.value,binding);
                for(const auto& member:members){if(member.kind!=Role::DictionarySoftOwner&&member.kind!=Role::DictionaryHardOwner)continue;
                    const auto target=audit->records.find(member.value);if(target==audit->records.end()||!target->second->preset.complete())continue;
                    auto value=std::const_pointer_cast<Record>(target->second);const auto* model=dynamic_cast<const DRW_RenderSettings*>(value->input->typedModel.get());
                    std::string name;if(!model||!renderingSourceName(dx_attdef_source::text(in.raw->groups[member.groupIndex-1u]),*in.context,name))continue;
                    value->presetRoot=binding;value->nameGroup=member.groupIndex-1u;value->targetGroup=member.groupIndex;
                    value->wrongOwner=in.raw->parentHandle!=root.raw->handle||value->input->typedRaw->parentHandle!=ref.value;
                    value->namedTarget=value->bodyMatched&&value->originBound&&!value->wrongOwner&&counts.at(member.value)==1u
                        &&dx_rendering_roots::allows(binding->key,model->m_kind)&&name==model->m_name;
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
        if(!row.input||!row.input->raw||!same(row.input->physical,row.input->physicalOwner)){report.state=S::ContextMismatch;return report;}
        const auto found=s.structuralAudits.find(row.input->raw->handle);
        if(found==s.structuralAudits.end()||!same(*pointer,found->second)){report.state=S::ContextMismatch;return report;}}
    if(structural.state==StructuralSourceState::CaptureFailed){report.state=S::SourceNotReady;return report;}
    for(const auto& item:audit->presetRoots){const auto& binding=item.second;
        if(!binding||!binding->dictionary||!binding->dictionary->input||!binding->dictionary->input->raw){report.state=S::ContextMismatch;return report;}
        const auto found=s.structuralAudits.find(item.first);
        if(found==s.structuralAudits.end()||!same(binding->dictionary,found->second)
            ||!same(binding->dictionary->input->physical,binding->dictionary->input->physicalOwner)){report.state=S::ContextMismatch;return report;}}
    for(const auto& item:audit->records){const auto& row=item.second;const auto found=s.renderingInputs.find(item.first);
        if(!row||found==s.renderingInputs.end()||!same(row->input,found->second)||row->index>=audit->owners.size()||!same(row,audit->owners[row->index])){
            report.state=S::ContextMismatch;return report;}
        if(!same(row->input->physical,row->input->physicalOwner)){report.state=S::ContextMismatch;return report;}
        const auto source=audit->inputs->rows.find(item.first);
        if(source==audit->inputs->rows.end()||!same(source->second.input,row->input)||source->second.physical!=row->physical||source->second.originBound!=row->originBound){report.state=S::ContextMismatch;return report;}
        if(row->presetRoot){const auto found=audit->presetRoots.find(row->presetRoot->dictionary->input->raw->handle);
            if(found==audit->presetRoots.end()||!same(row->presetRoot,found->second)){report.state=S::ContextMismatch;return report;}}
        ++report.records;if(row->bodyMatched){++report.profiles;report.fields+=row->preset.complete()?row->preset.spans.size():row->profile.spans.size();report.references+=row->references.size();}
        if (row->originBound)
          ++report.origins;
        if (row->namedTarget)
          ++report.namedTargets;
    }
    report.physical=audit->physicalRecords;report.state=audit->state;
    report.originalSourcesMatched=report.state==S::SourcesReady&&report.records&&report.records==report.profiles&&report.records==report.origins&&report.records==report.physical;
    return report;
}
