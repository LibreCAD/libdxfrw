// SPDX-License-Identifier: GPL-2.0-or-later
// Runtime-authored rooted plot-style source graphs. No external drawings.
Tags plotSourceDocument(const std::string& version,bool blocks,unsigned form) {
    auto tags=dependencyDocument(version,blocks,0u);
    appendBlockFields(tags,"40",{{3,"ACAD_PLOTSTYLENAME"},{350,"E801"}});
    const auto layer=blockField(tags,"D301",1001);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(layer),{{390,form==3u?"E804":"E802"}});
    Tags dictionary{{0,form==2u?"ACDBDICTIONARYWDFLT":"DICTIONARYWDFLT"},{5,"E801"},{330,"40"},{100,"AcDbDictionary"},{280,form==2u?"1":"0"},{281,"1"}};
    if(form!=1u)dictionary.insert(dictionary.end(),{{3,"Default-é"},{350,"E802"}});
    if(form>=2u)dictionary.insert(dictionary.end(),{{3,"Other"},{360,form==4u?"E802":"E803"}});
    if(form==2u)tags[slot(tags,3,"ACAD_PLOTSTYLENAME")].value="ACAD_\\U+0050LOTSTYLENAME";
    dictionary.insert(dictionary.end(),{{100,"AcDbDictionaryWithDefault"},{340,form==3u?"E804":"E802"}});
    Tags placeholders{{0,"ACDBPLACEHOLDER"},{5,"E802"},{102,"{ACAD_REACTORS"},{330,"E801"},{102,"}"},{330,"E801"}};
    if(form>=2u)placeholders.insert(placeholders.end(),{{0,"ACDBPLACEHOLDER"},{5,"E803"},{330,"E801"}});
    if(form==3u)placeholders.insert(placeholders.end(),{{0,"ACDBPLACEHOLDER"},{5,"E804"},{102,"{ACAD_REACTORS"},{330,"E801"},{330,"E801"},{102,"}"},{330,"E801"}});
    auto at=slot(tags,2,"OBJECTS")+1u;while(tags[at].code!=0||tags[at].value!="ENDSEC")++at;
    if(form==1u){placeholders.insert(placeholders.end(),dictionary.begin(),dictionary.end());dictionary=std::move(placeholders);}
    else dictionary.insert(dictionary.end(),placeholders.begin(),placeholders.end());
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),dictionary.begin(),dictionary.end());return tags;
}
struct PlotSourceProbe:dx_iface {
    using V=dxfRW::DxfPlotStyleSourceView;
    bool enabled=true,good=true,publicOnly=false,disableTyped=false;dxfRW* publicReader=nullptr;
    unsigned skip=0u,copy=0u,duplicate=0u,throwAt=0u,mutate=0u,faultPhase=0u,limitKind=0u;long allocation=-1;
    std::size_t limit=SIZE_MAX;unsigned callbacks=0u;const DRW_TableEntry* typed=nullptr;const DRW_RawDxfObject* original=nullptr;
    bool wantsDxfResourceSource() const override{return enabled&&(publicOnly||dx_iface::wantsDxfResourceSource());}
    void addHeader(const DRW_Header* value) override{if(publicOnly)return;dx_iface::addHeader(value);if(limit!=SIZE_MAX)DxIfaceTableSourceTestAccess::plotLimit(*cData,limitKind,limit);}
    void inspect(const DRW_TableEntry* model,const DRW_RawDxfObject* raw,unsigned phase) {
        const auto* reader=publicReader?publicReader:(cData?DxIfaceTableSourceTestAccess::readInput(*cData):nullptr);V view;
        const auto before=symbolFailedAllocations;bool present=false;
        {SymbolAllocationFailure none(0);present=reader&&reader->getDxfPlotStyleSourceView(view);
            dxfRW::DxfResourceSourceView legacy;if(reader&&reader->getDxfResourceSourceView(legacy))good=false;}
        const bool selected=enabled||(phase==2u&&original);
        if(symbolFailedAllocations!=before||present!=selected)good=false;
        if(present) {
            const bool dictionary=model||(raw&&(raw->name=="DICTIONARYWDFLT"||raw->name=="ACDBDICTIONARYWDFLT"));
            if(view.kind!=(dictionary?V::Kind::DictionaryWithDefault:V::Kind::Placeholder)||view.phase!=(model?V::Phase::Typed:V::Phase::Raw)
                ||(model&&view.model!=model)||(raw&&view.source!=raw)||view.typedAvailable!=dictionary||(!dictionary&&view.model))good=false;
            if(model){typed=model;original=view.source;}else if(dictionary&&(typed!=view.model||original!=view.source))good=false;
        }
        ++callbacks;(void)phase;
    }
    void addDictionaryWithDefault(const DRW_DictionaryWithDefault& value) override {
        if(value.handle!=0xE801u){if(!publicOnly)dx_iface::addDictionaryWithDefault(value);return;}
        inspect(&value,nullptr,1u);
        auto& originalModel=const_cast<DRW_DictionaryWithDefault&>(value);const auto state=dxfBlockRecord::Decoder::observeCommonState(value);const int hard=value.hardOwner;
        if(mutate==1u)DrwObjectEncodeTestAccess::commonMask(originalModel,static_cast<std::uint8_t>(state.second^1u));
        if(mutate==2u)originalModel.hardOwner^=1;
        const auto invoke=[&](){if(publicOnly)return;if(copy==1u){auto clone=value;dx_iface::addDictionaryWithDefault(clone);}else dx_iface::addDictionaryWithDefault(value);};
        if(skip!=1u){if(faultPhase==1u){SymbolAllocationFailure failure(allocation);invoke();}else invoke();if(duplicate==1u)invoke();}
        DrwObjectEncodeTestAccess::commonMask(originalModel,state.second);originalModel.hardOwner=hard;
        if(disableTyped)enabled=false;
        if(throwAt==1u)throw std::runtime_error("plot dictionary typed callback");
    }
    void addRawDxfObject(const DRW_RawDxfObject& value) override {
        if(value.handle<0xE801u||value.handle>0xE804u){if(!publicOnly)dx_iface::addRawDxfObject(value);return;}
        const unsigned phase=value.handle==0xE801u?2u:3u;inspect(nullptr,&value,phase);
        const auto invoke=[&](){if(publicOnly)return;if(copy==phase){auto clone=value;dx_iface::addRawDxfObject(clone);}else dx_iface::addRawDxfObject(value);};
        if(skip!=phase){if(faultPhase==phase){SymbolAllocationFailure failure(allocation);invoke();}else invoke();if(duplicate==phase)invoke();}
        if(value.handle==0xE801u){typed=nullptr;original=nullptr;}
        if(throwAt==phase)throw std::runtime_error("plot source raw callback");
    }
};
bool plotSourceTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::PlotSourceState;
    const auto path=dir/"plot-style-source.dxf";unsigned profiles=0u,markers=0u,spans=0u,refusals=0u,parserRefusals=0u,callbacks=0u,quotas=0u,contexts=0u,immutable=0u,retries=0u,reached=0u,proofFailures=0u,adjacentFailures=0u,unreached=0u,lifetimes=0u;
    const auto ready=[](const dx_data& data,unsigned form){const auto r=dx_iface::plotStyleSourceReport(data);
        const auto count=form==3u?4u:form>=2u?3u:2u;const auto targets=form==4u?1u:count-1u;
        return r.state==S::SourcesReady&&r.originalSourcesMatched&&r.records==count&&r.bodyMatched==count&&r.originsBound==count
            &&r.plotTargets==targets&&r.namedTargets==(form==1u?0u:form==2u||form==3u?2u:1u)&&r.defaultTarget==(form==3u?0xE804u:0xE802u)
            &&dx_iface::dependencyReport(data).originalTargetsMatched&&!r.ownerTargetsResolved&&!r.outputEligible;};
    const auto diagnose=[](const dx_data& data){const auto r=dx_iface::plotStyleSourceReport(data);const auto d=dx_iface::dependencyReport(data);
        std::cerr<<"Plot state="<<static_cast<int>(r.state)<<" records="<<r.records<<'/'<<r.physicalRecords<<" body="<<r.bodyMatched<<" origins="<<r.originsBound<<" targets="<<r.plotTargets
            <<" dependency="<<static_cast<int>(d.state)<<" refs="<<d.matched<<'/'<<d.references<<" first="<<std::hex<<d.firstSource<<" -> "<<d.firstTarget<<std::dec<<'\n';};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true})for(unsigned form=0u;form<5u;++form) {
        const auto tags=plotSourceDocument(version,blocks,form);PlotSourceProbe reader;dx_data data;
        if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data,form)||!reader.good){diagnose(data);return false;}
        const auto before=symbolFailedAllocations;{SymbolAllocationFailure none(0);if(!ready(data,form))return false;}if(symbolFailedAllocations!=before)return false;
        const auto wire=attdefWireTags(path,encoding==2u);const auto& physical=*A::identityInput(data)->source;
        for(const auto& item:A::plots(data)->records) {
            const auto& p=physical.records.at(item.second->physical);const auto& marker=wire.at(p.marker.ordinal-1u);
            if(marker.code||marker.text!=p.name||marker.begin!=p.marker.beginOffset||marker.end!=p.marker.endOffset)return false;++markers;
            for(const auto& ref:item.second->references){const auto& scalar=wire.at(p.marker.ordinal+ref.groupIndex);
                std::uint32_t handle=0u;const auto parsed=std::from_chars(scalar.text.data(),scalar.text.data()+scalar.text.size(),handle,16);
                if(scalar.code!=ref.code||parsed.ec!=std::errc{}||parsed.ptr!=scalar.text.data()+scalar.text.size()||handle!=ref.value)return false;++spans;}
        }
        {const auto& graph=*A::plots(data);if(!graph.root)return false;const auto& record=physical.records.at(graph.root->physical);
            const auto& key=wire.at(record.marker.ordinal+graph.rootKeyGroup);const auto& target=wire.at(record.marker.ordinal+graph.rootTargetGroup);
            if(key.code!=3||target.code!=350||target.text!="E801"||key.text!=*graph.root->input->raw->groups.at(graph.rootKeyGroup).content.s)return false;
            spans+=2u;}
        for(auto& dictionary:data.dictionaryWithDefaults)dictionary.m_defaultEntryHandle=0xFFFFu;
        for(auto& placeholder:data.rawPlaceholders)placeholder.parentHandle=0xFFFFu;
        if(!ready(data,form))return false;++immutable;++profiles;
    }
    for(unsigned encoding=0u;encoding<3u;++encoding) {
        const auto good=plotSourceDocument("AC1032",false,0u);
        const auto retry=[&](PlotSourceProbe& reader){reader.skip=reader.copy=reader.duplicate=reader.throwAt=reader.mutate=reader.faultPhase=0u;reader.limit=SIZE_MAX;reader.enabled=true;reader.disableTyped=false;
            dx_data next;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&next,false)||!ready(next,0u)){diagnose(next);return false;}++retries;return true;};
        for(unsigned fault=0u;fault<15u;++fault) {
            auto tags=good;
            if(fault==0u)tags[slot(tags,3,"ACAD_PLOTSTYLENAME")].value="MISSING_PLOT_DICTIONARY";
            else if(fault==1u)tags[blockField(tags,"E801",330)].value="701";
            else if(fault==2u){auto at=blockField(tags,"E802",330)+1u;while(tags[at].code!=330)++at;tags[at].value="40";}
            else if(fault==3u)tags[blockField(tags,"E801",340)].value="705";
            else if(fault==4u)tags[blockField(tags,"E801",350)].value="705";
            else if(fault==5u)appendBlockFields(tags,"E802",{{100,"UnknownPlaceholder"}});
            else if(fault==6u)appendBlockFields(tags,"E801",{{99,"3"}});
            else if(fault==7u)tags[blockField(tags,"E802",330)].value="40";
            else if(fault==8u)tags[blockField(tags,"E801",280)].value="2";
            else if(fault==9u)tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,"E801",280)));
            else if(fault==10u)tags[blockField(tags,"E801",3)].value="\\U+D800";
            else if(fault==11u){auto at=blockField(tags,"E801",100)+1u;while(tags[at].code!=100)++at;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{3,"Default-é"},{350,"E802"}});}
            else if(fault==12u){for(auto& tag:tags)if((tag.code==5||tag.code==330||tag.code==350)&&tag.value=="E801")tag.value="D";}
            else if(fault==13u)appendBlockFields(tags,"E801",{{280,"0"}});
            else {tags=plotSourceDocument("AC1032",false,4u);tags[blockField(tags,"D301",390)].value="E803";}
            PlotSourceProbe reader;dx_data data;if(!writeBlockBinding(path,tags,encoding))return false;
            const bool imported=reader.fileImport(path.string(),&data,false);
            if((fault==8u||fault==13u?imported:!imported)||dx_iface::dependencyReport(data).originalTargetsMatched){std::cerr<<"Plot field refusal="<<fault<<'\n';diagnose(data);return false;}
            if(!imported)++parserRefusals;
            if(fault==14u) {
                if(!dx_iface::plotStyleSourceReport(data).originalSourcesMatched||!A::tableReferences(data)->targets.count(0xE803u))return false;
                bool withheld=false;for(const auto& row:A::dependencies(data)->rows)if(row.role==dx_tableResourceRole::LayerPlotStyle&&row.target==0xE803u)
                    withheld=row.reason==dx_iface::DependencyReason::OriginalUnavailable;
                if(!withheld)return false;
            }
            if(!retry(reader))return false;++refusals;
        }
        for(unsigned phase=1u;phase<=3u;++phase)for(unsigned fault=0u;fault<4u;++fault) {
            PlotSourceProbe reader;if(fault==0u)reader.skip=phase;else if(fault==1u)reader.copy=phase;else if(fault==2u)reader.duplicate=phase;else reader.throwAt=phase;
            dx_data data;bool threw=false;try{if(!writeBlockBinding(path,good,encoding))return false;(void)reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
            if((fault==3u)!=threw||dx_iface::plotStyleSourceReport(data).originalSourcesMatched||dx_iface::dependencyReport(data).originalTargetsMatched)return false;
            if(!retry(reader))return false;++callbacks;
        }
        for(unsigned mutation=1u;mutation<=2u;++mutation) {
            PlotSourceProbe reader;reader.mutate=mutation;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::plotStyleSourceReport(data).originalSourcesMatched||dx_iface::dependencyReport(data).originalTargetsMatched)return false;
            if(!retry(reader))return false;++callbacks;
        }
        {PlotSourceProbe reader;reader.disableTyped=true;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||!reader.good||dx_iface::plotStyleSourceReport(data).records!=1u
                ||dx_iface::plotStyleSourceReport(data).originalSourcesMatched||dx_iface::dependencyReport(data).originalTargetsMatched)return false;
            if(!retry(reader))return false;++callbacks;}
        PlotSourceProbe baseline;dx_data base;if(!writeBlockBinding(path,good,encoding)||!baseline.fileImport(path.string(),&base,false)||!ready(base,0u))return false;
        const auto receipt=dx_iface::plotStyleSourceReport(base);const std::array<std::size_t,4> limits{receipt.records,A::plotGroups(base),receipt.bytes,receipt.work};
        for(unsigned which=0u;which<4u;++which)for(bool under:{false,true}) {
            PlotSourceProbe reader;reader.limitKind=which;reader.limit=limits[which]-(under?1u:0u);dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||ready(data,0u)==under||(!under&&dx_iface::plotStyleSourceReport(data).state!=S::SourcesReady))return false;
            if(!retry(reader))return false;++quotas;
        }
        for(bool alias:{false,true}) {
            auto saved=A::plots(base);A::substitutePlots(base,alias);
            if(dx_iface::plotStyleSourceReport(base).state!=S::ContextMismatch||dx_iface::dependencyReport(base).originalTargetsMatched)return false;
            A::restorePlots(base,saved);if(!ready(base,0u))return false;++contexts;
        }
        for(const auto id:{0xE801u,0xE802u})for(bool alias:{false,true}) {
            const auto saved=A::plotInputs(base).at(id);A::substitutePlotInput(base,id,alias);
            if(dx_iface::plotStyleSourceReport(base).state!=S::ContextMismatch||dx_iface::dependencyReport(base).originalTargetsMatched)return false;
            A::restorePlotInput(base,id,saved);if(!ready(base,0u))return false;++contexts;
        }
        {PlotSourceProbe reader;dx_data other;if(!reader.fileImport(path.string(),&other,false)||!ready(other,0u))return false;
            const auto saved=A::plots(base);A::transplantPlots(base,other);
            if(dx_iface::plotStyleSourceReport(base).state!=S::ContextMismatch||dx_iface::dependencyReport(base).originalTargetsMatched)return false;
            A::restorePlots(base,saved);if(!ready(base,0u))return false;++contexts;}
        for(unsigned phase=1u;phase<=3u;++phase)for(long offset=0;offset<24;++offset) {
            PlotSourceProbe reader;reader.faultPhase=phase;reader.allocation=offset;dx_data data;const auto before=symbolFailedAllocations;
            if(!reader.fileImport(path.string(),&data,false))return false;
            if(symbolFailedAllocations!=before){const auto r=dx_iface::plotStyleSourceReport(data);
                if(r.originalSourcesMatched){if(!A::adjacentPlotCaptureFailure(data))return false;++adjacentFailures;}
                else {if(r.state!=S::CaptureFailed||dx_iface::dependencyReport(data).originalTargetsMatched)return false;++proofFailures;}
                ++reached;}
            else {if(!ready(data,0u))return false;++unreached;}
            if(!retry(reader))return false;
        }
        for(bool enabled:{false,true}) {
            const Tags standalone{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1032"},{0,"ENDSEC"},{0,"SECTION"},{2,"OBJECTS"},
                {0,"DICTIONARYWDFLT"},{5,"E801"},{330,"40"},{100,"AcDbDictionary"},{280,"0"},{281,"1"},{3,"Normal"},{350,"E802"},
                {100,"AcDbDictionaryWithDefault"},{340,"E802"},{0,"ACDBPLACEHOLDER"},{5,"E802"},{330,"E801"},{0,"ENDSEC"},{0,"EOF"}};
            if(!writeBlockBinding(path,standalone,encoding))return false;
            PlotSourceProbe reader;reader.publicOnly=true;reader.enabled=enabled;dxfRW codec(path.string().c_str());reader.publicReader=&codec;
            PlotSourceProbe::V unused; // public view is cleared before/after every publication
            if(codec.getDxfPlotStyleSourceView(unused)||!codec.read(&reader,false)||!reader.good||codec.getDxfPlotStyleSourceView(unused)||unused.model||unused.source)return false;
            ++lifetimes;
        }
    }
    std::cout<<"Plot source profiles="<<profiles<<"; original markers="<<markers<<"; reference spans="<<spans<<"; field/graph refusals="<<refusals<<"; parser refusals="<<parserRefusals<<"; callback refusals="<<callbacks
        <<"; exact/one-under limits="<<quotas<<"; immutable owner controls="<<contexts<<"; live-state independence="<<immutable<<"; reached allocation failures="<<reached<<"; proof failures="<<proofFailures<<"; adjacent capture failures="<<adjacentFailures
        <<"; unreached offsets="<<unreached<<"; public lifetime controls="<<lifetimes<<"; corrected imports="<<retries<<'\n';
    return profiles==120u&&markers>0u&&spans>0u&&refusals==45u&&parserRefusals==6u&&callbacks==45u&&quotas==24u&&contexts==21u&&immutable==120u&&proofFailures>0u&&proofFailures+adjacentFailures==reached&&reached+unreached==216u&&lifetimes==6u&&retries==330u;
}
