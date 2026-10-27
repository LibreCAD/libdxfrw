// SPDX-License-Identifier: GPL-2.0-or-later
// Included once after the source/context/identity and size-estimation helpers.
#include "intern/dxfrenderingsource.h"
#include "intern/dxfrendersettings.h"
namespace {
using RenderingView=dxfRW::DxfRenderingSourceView;
RenderingView ownedRenderingState(RenderingView view) noexcept {
    view.model=nullptr;view.source=nullptr;view.phase=RenderingView::Phase::None;
    return view;
}
bool renderingStatesEqual(const RenderingView& a,const RenderingView& b) noexcept {
    return std::tie(a.kind,a.originalPendingXdata,a.originalCoordinateMask,
        a.originalVisualStyleScope,a.originalVisualStylePropertyTail,a.originalRenderBaseScope,
        a.originalRenderCounts,a.originalRenderLongs,a.originalRenderBytes,a.originalRenderBools)
        ==std::tie(b.kind,b.originalPendingXdata,b.originalCoordinateMask,
        b.originalVisualStyleScope,b.originalVisualStylePropertyTail,b.originalRenderBaseScope,
        b.originalRenderCounts,b.originalRenderLongs,b.originalRenderBytes,b.originalRenderBools)
        &&std::memcmp(a.originalRenderDoubles.data(),b.originalRenderDoubles.data(),sizeof(double)*4u)==0;
}
bool renderingVisualStyleName(std::string_view name) noexcept {
    return asciiIdentifierEquals(name,"VISUALSTYLE")||asciiIdentifierEquals(name,"ACDBVISUALSTYLE")||asciiIdentifierEquals(name,"ACDB_VISUALSTYLE_CLASS");
}
bool renderingName(std::string_view name) noexcept {
    if(renderingVisualStyleName(name))return true;
    for(auto kind:{DRW_RenderSettings::Settings,DRW_RenderSettings::Global,DRW_RenderSettings::Environment,
                  DRW_RenderSettings::Entry,DRW_RenderSettings::RapidRT,DRW_RenderSettings::MentalRay})
        if(renderSettingsNameMatchesKind(name,kind))return true;
    return false;
}
bool renderingModelState(const RenderingView& view) noexcept {
    if(!view.model)return false;
    const auto common=dxfBlockRecord::Decoder::observeCommonState(*view.model);
    if(common!=std::make_pair(view.originalPendingXdata,view.originalCoordinateMask))return false;
    if(view.kind==RenderingView::Kind::VisualStyle) {
        const auto* style=dynamic_cast<const DRW_VisualStyle*>(view.model);
        return style&&dxfRendering::Observation::visualStyle(*style)
            ==std::make_pair(view.originalVisualStyleScope,view.originalVisualStylePropertyTail);
    }
    return dynamic_cast<const DRW_RenderSettings*>(view.model)!=nullptr;
}
std::shared_ptr<const DRW_TableEntry> renderingSnapshot(const RenderingView& view) {
    if(view.kind==RenderingView::Kind::VisualStyle)
        return std::make_shared<const DRW_VisualStyle>(static_cast<const DRW_VisualStyle&>(*view.model));
    return std::make_shared<const DRW_RenderSettings>(static_cast<const DRW_RenderSettings&>(*view.model));
}
template<class Context>bool renderingContextEqual(const Context& a,const dxfRW::DxfReadContext& b) noexcept {
    return std::tie(a.sourceVersion,a.codecVersion,a.codecCodePage,a.declaration,a.hasDeclaration,a.compatibilityProfile,
        a.textEncoding,a.codePagePolicy,a.binary,a.applyExtrusion)
        ==std::tie(b.sourceVersion,b.codecVersion,b.codecCodePage,b.declaration,b.hasDeclaration,b.compatibilityProfile,
        b.textEncoding,b.codePagePolicy,b.binary,b.applyExtrusion);
}
}

void dx_iface::observeRenderingInput(const DRW_TableEntry* model,const DRW_RawDxfObject* raw) noexcept {
    if(!tableSourceCaptureOpen())return;
    auto& s=cData->m_tableDxfSource;if(s.renderingCaptureFailed)return;
    using Input=dx_data::TableDxfSourceCapture::RenderingInput;
    using Pending=dx_data::TableDxfSourceCapture::RenderingPending;
    using Context=dx_data::TableDxfSourceCapture::EntitiesReadContext;
    RenderingView view;dxfRW::DxfReadContext live;
    if(!s.entitiesReadInput||!s.entitiesReadInput->getDxfRenderingSourceView(view))return;
    const auto fail=[&](){s.renderingCaptureFailed=true;};
    if(!s.entitiesReadInput->getDxfReadContext(live)||live.sourceVersion<DRW::AC1021||live.sourceVersion>DRW::AC1032)return;
    if(!view.model||!view.source||!view.source->handle||view.source->m_version!=live.sourceVersion
        ||view.source->hasRawValues==live.binary||view.source->handle!=view.model->handle||view.source->parentHandle!=view.model->parentHandle
        ||view.phase!=(model?RenderingView::Phase::Typed:RenderingView::Phase::Raw)
        ||(model&&model!=view.model)||(raw&&raw!=view.source)||!renderingName(view.source->name)) {fail();return;}
    const bool visual=view.kind==RenderingView::Kind::VisualStyle;
    dxfRW::DxfObjectPhysicalSourceView physical;
    if(!s.entitiesReadInput->getDxfObjectPhysicalSourceView(physical)||physical.model!=view.model||physical.source!=view.source
        ||physical.kind!=(visual?decltype(physical.kind)::VisualStyle:decltype(physical.kind)::RenderSettings)
        ||physical.phase!=(model?decltype(physical.phase)::Typed:decltype(physical.phase)::Raw)){fail();return;}
    dxfRW::DxfVisualStylePropertiesView properties;
    if(visual&&(!s.entitiesReadInput->getDxfVisualStylePropertiesView(properties)
        ||properties.model!=view.model||properties.source!=view.source||properties.phase!=view.phase||!properties.properties)){fail();return;}
    if(visual?typeid(*view.model)!=typeid(DRW_VisualStyle)
        :(typeid(*view.model)!=typeid(DRW_RenderSettings)
          ||!renderSettingsNameMatchesKind(view.source->name,static_cast<const DRW_RenderSettings&>(*view.model).m_kind))) {fail();return;}
    try {
        const auto id=view.source->handle;
        if(model) {
            if(s.renderingPending.count(id)||s.renderingInputs.count(id)||s.renderingRecords>=s.maxRenderingRecords
                ||s.renderingGroups>s.maxRenderingGroups||view.source->groups.size()>s.maxRenderingGroups-s.renderingGroups
                ||live.codecCodePage.size()>128u){fail();return;}
            std::size_t bytes=sizeof(Input)+sizeof(Pending)+sizeof(Context)+96u*sizeof(void*),modelBytes=0u,rawBytes=0u,values=0u;
            if(!estimateRawDxfObjectBytes(*view.source,rawBytes)
                ||!(visual?estimateVisualStyleTypedBytes(static_cast<const DRW_VisualStyle&>(*model),modelBytes)
                    :estimateRenderSettingsTypedBytes(static_cast<const DRW_RenderSettings&>(*model),modelBytes,values))
                ||!addAssociativeSlotBytes(rawBytes,4u,bytes)||!addAssociativeSlotBytes(modelBytes,4u,bytes)
                ||!addAssociativeSlotBytes(view.source->groups.size(),128u,bytes)
                ||(physical.physical&&!addAssociativeRetainedBytes(physical.physical->bytes,bytes))
                ||!addAssociativeRetainedBytes(live.codecCodePage.size(),bytes)||!addAssociativeRetainedBytes(live.declaration.size(),bytes)
                ||s.renderingBytes>s.maxRenderingBytes||bytes>s.maxRenderingBytes-s.renderingBytes
                ||s.renderingWork>s.maxRenderingWork||bytes>s.maxRenderingWork-s.renderingWork){fail();return;}
            s.renderingBytes+=bytes;s.renderingWork+=bytes;++s.renderingRecords;s.renderingGroups+=view.source->groups.size();
            auto context=std::make_shared<Context>();context->sourceVersion=live.sourceVersion;context->codecVersion=live.codecVersion;
            context->codecCodePage=live.codecCodePage;context->declaration=live.declaration;context->hasDeclaration=live.hasDeclaration;
            context->compatibilityProfile=live.compatibilityProfile;context->textEncoding=live.textEncoding;context->codePagePolicy=live.codePagePolicy;
            context->binary=live.binary;context->applyExtrusion=live.applyExtrusion;context->captured=true;
            Pending pending;auto& in=pending.input;in.container=cData;in.generation=s.generation;in.containerId=cData->m_entitiesContainerIdentity.value;
            in.original=ownedRenderingState(view);in.context=std::move(context);in.typedModel=renderingSnapshot(view);
            in.physical=physical.physical;in.physicalOwner=in.physical;in.physicalStatus=physical.status;
            in.visualPropertiesCaptured=visual;if(visual)in.typedVisualProperties=*properties.properties;
            in.typedRaw=std::make_shared<const DRW_RawDxfObject>(*view.source);in.stateStable=renderingModelState(view);
            pending.model=model;pending.raw=view.source;s.renderingPending.emplace(id,std::move(pending));return;
        }
        const auto found=s.renderingPending.find(id);if(found==s.renderingPending.end()){fail();return;}
        auto& pending=found->second;auto& in=pending.input;
        if(pending.model!=view.model||pending.raw!=raw||!renderingStatesEqual(in.original,view)
            ||in.physicalStatus!=physical.status||in.physical.get()!=physical.physical.get()
            ||in.physical.owner_before(physical.physical)||physical.physical.owner_before(in.physical)
            ||!in.context||!renderingContextEqual(*in.context,live)){fail();return;}
        // Raw-phase callback mutations can be larger than the original typed
        // snapshot. Admit their actual owned size before making the second copy.
        std::size_t rawBytes=0u,modelBytes=0u,values=0u,cost=0u;
        if(!estimateRawDxfObjectBytes(*raw,rawBytes)
            ||!(visual?estimateVisualStyleTypedBytes(static_cast<const DRW_VisualStyle&>(*view.model),modelBytes)
                :estimateRenderSettingsTypedBytes(static_cast<const DRW_RenderSettings&>(*view.model),modelBytes,values))
            ||!addAssociativeSlotBytes(rawBytes,2u,cost)||!addAssociativeSlotBytes(modelBytes,2u,cost)
            ||s.renderingBytes>s.maxRenderingBytes||cost>s.maxRenderingBytes-s.renderingBytes
            ||s.renderingWork>s.maxRenderingWork||cost>s.maxRenderingWork-s.renderingWork){fail();return;}
        s.renderingBytes+=cost;s.renderingWork+=cost;
        in.rawModel=renderingSnapshot(view);in.rawRaw=std::make_shared<const DRW_RawDxfObject>(*raw);
        if(visual)in.rawVisualProperties=*properties.properties;
        in.actualPair=true;in.rawStable=retainedRawDxfObjectsEqual(*in.typedRaw,*in.rawRaw);
        in.stateStable=in.stateStable&&renderingModelState(view);in.index=s.renderingInputOwners.size();
        auto saved=std::make_shared<const Input>(std::move(in));s.renderingInputs.emplace(id,saved);
        s.renderingInputOwners.push_back(std::move(saved));s.renderingPending.erase(found);
        s.renderingInputBytes=s.renderingBytes;s.renderingInputWork=s.renderingWork;
    }catch(...){fail();}
}

void dx_iface::replayRenderingInput(dx_data& data,dx_data::TableDxfSourceCapture::RenderingInputRow& row) noexcept {
    auto& s=data.m_tableDxfSource;
    if(s.renderingReplayFailed||s.renderingReplayLimited||!row.input)return;
    const auto& in=*row.input;
    if(!in.actualPair||!in.rawStable||!in.stateStable||!row.originBound||!in.context||!in.typedModel||!in.rawModel||!in.typedRaw||!in.rawRaw)return;
    const bool visual=in.original.kind==RenderingView::Kind::VisualStyle;
    const auto limit=[&](){s.renderingReplayLimited=true;};
    try {
        std::size_t rawBytes=0u,modelBytes=0u,values=0u,bytes=1024u,work=1024u;
        if(!estimateRawDxfObjectBytes(*in.typedRaw,rawBytes)
            ||!(visual?estimateVisualStyleTypedBytes(static_cast<const DRW_VisualStyle&>(*in.typedModel),modelBytes)
                :estimateRenderSettingsTypedBytes(static_cast<const DRW_RenderSettings&>(*in.typedModel),modelBytes,values))
            ||!addAssociativeSlotBytes(rawBytes,8u,bytes)||!addAssociativeSlotBytes(modelBytes,4u,bytes)
            ||!addAssociativeSlotBytes(in.typedRaw->groups.size(),128u,work)||!addAssociativeRetainedBytes(bytes,work)
            ||s.renderingReplayBytes>s.maxRenderingReplayBytes||bytes>s.maxRenderingReplayBytes-s.renderingReplayBytes
            ||s.renderingReplayWork>s.maxRenderingReplayWork||work>s.maxRenderingReplayWork-s.renderingReplayWork){limit();return;}
        s.renderingReplayBytes+=bytes;s.renderingReplayWork+=work;++s.renderingReplayRecords;
        std::string wire;
        if(!tableResourceSourceBody(*in.typedRaw,s.maxRenderingReplayBytes,wire)){s.renderingReplayFailed=true;return;}
        dxfRW::DxfObjectsWriteView profile{};profile.bytes=wire;profile.version=in.context->sourceVersion;
        profile.binary=in.context->binary;profile.codePage=in.context->codecCodePage;profile.compatibilityProfile=in.context->compatibilityProfile;
        dx_standard_output::InputBuffer buffer(wire);std::istream input(&buffer);
        auto budget=std::make_shared<DxfReadRetainedBudget>(bytes);auto reader=dx_standard_output::readerFor(input,profile,budget);
        if(!reader){s.renderingReplayFailed=true;return;}
        const char* codecVersion=nullptr;for(const auto& name:DRW::dwgVersionStrings)if(name.second==in.context->codecVersion)codecVersion=name.first;
        if(!codecVersion)return;
        reader->setVersion(codecVersion,true);reader->setDxfTextEncoding(in.context->textEncoding);
        reader->setDxfCodePagePolicy(in.context->codePagePolicy);reader->setSemanticStringMode(true);
        reader->setRecordBudget(in.typedRaw->groups.size());reader->setTypedVariantNodeBudget(in.typedRaw->groups.size());
        reader->setDecodeWorkBudget(s.maxRenderingReplayWork-s.renderingReplayWork);
        const auto common=[&](const auto& a,const auto& b){return standardDwgCommonEqual(a,b,in.context->sourceVersion);};
        const auto decode=[&](auto& model,auto&& parse,auto&& finish,auto&& equal) {
            int code=0;bool good=true;
            while(buffer.position()<wire.size())if(!reader->readRec(&code)||!code||!parse(code)){good=false;break;}
            if(good)finish();
            if(budget->exceeded()||reader->decodeWorkBudgetExceeded()||reader->recordBudgetExceeded()
                ||reader->typedVariantNodeBudgetExceeded()||reader->tokenByteBudgetExceeded()||reader->inputByteBudgetExceeded())limit();
            if(!addAssociativeRetainedBytes(reader->decodeWorkCount(),s.renderingReplayWork)
                ||s.renderingReplayWork>s.maxRenderingReplayWork){limit();return;}
            if(!good||reader->recordCount()!=in.typedRaw->groups.size()){s.renderingReplayFailed=true;return;}
            if(s.renderingReplayLimited)return;
            const auto state=dxfBlockRecord::Decoder::observeCommonState(model);
            row.replayedState.kind=in.original.kind;row.replayedState.originalPendingXdata=state.first;
            row.replayedState.originalCoordinateMask=state.second;
            row.replayDecoded=true;
            row.modeledStateMatched=renderingStatesEqual(in.original,row.replayedState)
                &&common(model,*in.typedModel)&&common(model,*in.rawModel)&&equal(*in.typedModel)&&equal(*in.rawModel);
            using Model=std::decay_t<decltype(model)>;
            row.replayedModel=std::make_shared<const Model>(std::move(model));
        };
        if(visual){DRW_VisualStyle model;dxfRendering::VisualStyleDecoder decoder(model);
            decode(model,[&](int code){return decoder.parseCode(code,reader);},[&](){
                row.replayedVisualProperties=decoder.snapshot();
                const auto state=dxfRendering::Observation::visualStyle(model);row.replayedState.originalVisualStyleScope=state.first;
                row.replayedState.originalVisualStylePropertyTail=state.second;
            },[&](const DRW_TableEntry& value){const auto* other=dynamic_cast<const DRW_VisualStyle*>(&value);
                return other&&dxfRendering::Observation::visual(model,*other)&&in.visualPropertiesCaptured
                    &&dxfRendering::propertiesEqual(row.replayedVisualProperties,in.typedVisualProperties)
                    &&dxfRendering::propertiesEqual(row.replayedVisualProperties,in.rawVisualProperties);});
        }else{DRW_RenderSettings model;model.m_kind=static_cast<const DRW_RenderSettings&>(*in.typedModel).m_kind;
            dxfRenderSettings::Decoder decoder(model);
            decode(model,[&](int code){return decoder.parseCode(code,reader);},[&](){decoder.finish();const auto state=decoder.snapshot();
                row.replayedState.originalRenderBaseScope=state.base;row.replayedState.originalRenderCounts=state.counts;
                row.replayedState.originalRenderLongs=state.longs;row.replayedState.originalRenderBytes=state.bytes;
                row.replayedState.originalRenderBools=state.bools;row.replayedState.originalRenderDoubles=state.doubles;
            },[&](const DRW_TableEntry& value){const auto* other=dynamic_cast<const DRW_RenderSettings*>(&value);return other&&dxfRendering::Observation::render(model,*other);});
        }
    }catch(...){s.renderingReplayFailed=true;}
}

void dx_iface::finishRenderingInputs(dx_data& data) noexcept {
    auto& s=data.m_tableDxfSource;if(s.renderingCaptureFailed)return;
    using A=dx_data::TableDxfSourceCapture::RenderingInputAudit;using S=A::State;using I=DRW_DxfIdentitySource;
    try {
        const auto count=s.identityInput&&s.identityInput->source?s.identityInput->source->records.size():0u;
        std::size_t work=sizeof(A),bytes=sizeof(A);
        if(!addAssociativeSlotBytes(count,128u,work)
            ||!addAssociativeSlotBytes(s.renderingInputs.size(),sizeof(dx_data::TableDxfSourceCapture::RenderingInputRow)+64u,bytes)
            ||!addAssociativeSlotBytes(s.renderingInputs.size(),128u,work)){s.renderingCaptureFailed=true;return;}
        if(s.renderingBytes>s.maxRenderingBytes||bytes>s.maxRenderingBytes-s.renderingBytes
            ||s.renderingWork>s.maxRenderingWork||work>s.maxRenderingWork-s.renderingWork){s.renderingCaptureFailed=true;return;}
        s.renderingBytes+=bytes;s.renderingWork+=work;
        auto audit=std::make_shared<A>();audit->container=&data;audit->generation=s.generation;audit->containerId=data.m_entitiesContainerIdentity.value;
        audit->identity=s.identityInput;audit->identities=s.identityAudit;
        const auto save=[&](S state){audit->state=state;s.renderingAudit=std::move(audit);s.renderingAuditOwner=s.renderingAudit;};
        if(identitySourceReport(data).state!=IdentitySourceState::NamespaceReady||!audit->identity||!audit->identities){save(S::SourceNotReady);return;}
        const auto& physical=*audit->identity->source;
        for(const auto& record:physical.records)if(record.kind==I::Kind::Object&&renderingName(record.name))++audit->physicalRecords;
        bool complete=audit->physicalRecords==s.renderingInputs.size();
        for(const auto& item:s.renderingInputs){const auto& in=*item.second;dx_data::TableDxfSourceCapture::RenderingInputRow row;row.input=item.second;
            const auto found=audit->identities->identities.find(item.first);
            if(found!=audit->identities->identities.end()&&found->second<physical.records.size()){
                const auto& record=physical.records[found->second];row.originBound=record.kind==I::Kind::Object
                    &&asciiIdentifierEquals(record.name,in.typedRaw->name)&&record.section<physical.sections.size()
                    &&physical.sections[record.section].kind==I::Section::Objects;
                if(row.originBound)row.physical=found->second;}
            complete=complete&&in.actualPair&&in.rawStable&&in.stateStable&&row.originBound;
            replayRenderingInput(data,row);
            audit->rows.emplace(item.first,std::move(row));}
        save(!audit->physicalRecords&&!s.renderingRecords?S::NoInputs:complete?S::InputsCaptured:S::Unqualified);
    }catch(...){s.renderingCaptureFailed=true;}
}

dx_iface::RenderingInputReport dx_iface::renderingInputReport(const dx_data& data) noexcept {
    using S=RenderingInputState;RenderingInputReport report;const auto& s=data.m_tableDxfSource;
    report.records=s.renderingRecords;report.bytes=s.renderingBytes;report.work=s.renderingWork;
    if(!s.generation||data.sourceFormat!=dx_source_format::DXF)return report;
    if(!s.closed){report.state=S::Reading;return report;}if(!s.successful){report.state=S::ImportFailed;return report;}
    if(s.contextFailed||s.generation!=1u||s.generation!=data.importGeneration||s.reader||s.entitiesReadInput){report.state=S::ContextMismatch;return report;}
    if(s.renderingCaptureFailed||!s.renderingPending.empty()){report.state=S::CaptureFailed;return report;}
    const auto& audit=s.renderingAudit;if(!audit)return report;
    const auto same=[](const auto& a,const auto& b){return a.get()==b.get()&&!a.owner_before(b)&&!b.owner_before(a);};
    if(!same(audit,s.renderingAuditOwner)||audit->container!=&data||audit->generation!=s.generation||audit->containerId!=data.m_entitiesContainerIdentity.value
        ||!same(audit->identity,s.identityInput)||!same(audit->identities,s.identityAudit)||audit->rows.size()!=s.renderingInputs.size()
        ||s.renderingInputs.size()!=s.renderingInputOwners.size()||s.renderingRecords!=s.renderingInputs.size()){
        report.state=S::ContextMismatch;return report;}
    const auto identity=identitySourceReport(data);if(identity.state!=IdentitySourceState::NamespaceReady){report.state=identity.state==IdentitySourceState::ContextMismatch?S::ContextMismatch:S::SourceNotReady;return report;}
    for(const auto& item:audit->rows){const auto& row=item.second;const auto found=s.renderingInputs.find(item.first);
        if(found==s.renderingInputs.end()||!same(row.input,found->second)||!row.input){report.state=S::ContextMismatch;return report;}
        const auto& in=*row.input;
        if(in.index>=s.renderingInputOwners.size()||!same(row.input,s.renderingInputOwners[in.index])||in.container!=&data
            ||in.generation!=s.generation||in.containerId!=data.m_entitiesContainerIdentity.value||!in.context||in.context->sourceVersion!=s.version
            ||in.original.model||in.original.source||in.original.phase!=RenderingView::Phase::None
            ||!in.typedModel||!in.rawModel||!in.typedRaw||!in.rawRaw){report.state=S::ContextMismatch;return report;}
        if(in.actualPair)++report.pairs;if(in.rawStable)++report.rawStable;if(in.stateStable)++report.stateStable;if(row.originBound)++report.origins;
    }
    report.physical=audit->physicalRecords;report.state=audit->state;
    report.publicationInputsCaptured=report.state==S::InputsCaptured&&report.records!=0u
        &&report.records==report.physical&&report.records==report.pairs&&report.records==report.rawStable
        &&report.records==report.stateStable&&report.records==report.origins;
    return report;
}

dx_iface::RenderingModelReport dx_iface::renderingModelReport(const dx_data& data) noexcept {
    using S=RenderingModelState;using I=RenderingInputState;RenderingModelReport report;
    const auto input=renderingInputReport(data);const auto& s=data.m_tableDxfSource;
    report.records=input.records;report.bytes=s.renderingReplayBytes;report.work=s.renderingReplayWork;
    switch(input.state){
    case I::NotCaptured:return report;
    case I::Reading:report.state=S::Reading;return report;
    case I::ImportFailed:report.state=S::ImportFailed;return report;
    case I::ContextMismatch:report.state=S::ContextMismatch;return report;
    case I::CaptureFailed:report.state=S::CaptureFailed;return report;
    case I::SourceNotReady:report.state=S::SourceNotReady;return report;
    case I::NoInputs:report.state=S::NoInputs;return report;
    default:break;
    }
    if(s.renderingReplayFailed){report.state=S::CaptureFailed;return report;}
    if(s.renderingReplayLimited){report.state=S::Limited;return report;}
    for(const auto& item:s.renderingAudit->rows){const auto& row=item.second;
        if(row.replayDecoded&&row.replayedModel)++report.decoded;
        if(row.modeledStateMatched&&row.replayDecoded&&row.replayedModel)++report.matched;
    }
    report.modeledStateMatched=input.publicationInputsCaptured&&report.records==report.decoded&&report.records==report.matched;
    report.state=report.modeledStateMatched?S::ModeledStateMatched:S::Unqualified;
    return report;
}
