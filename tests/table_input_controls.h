// SPDX-License-Identifier: GPL-2.0-or-later
// Actual producer identities and pre-copy observations only. Full modeled
// TABLE replay,scalar profiles,targets,dependencies/live/output remain open.
struct TableInputProbe final:dx_iface {
    bool selected=true,facets=true,bad=false,throwSource=false,throwTyped=false,alterField=false,contextProbe=false,contextViewAllocationFree=false;
    int fault=0,stateFault=-1;long allocation=-1;bool allocationReached=false;
    unsigned sources=0u,typed=0u,depth=0u;const DRW_RawDxfObject* publishedRaw=nullptr;
    dxfTable::SourceState before;
    bool wantsDxfTableSource() const override{return selected&&dx_iface::wantsDxfTableSource();}
    bool wantsDxfBlockSource() const override{return facets&&dx_iface::wantsDxfBlockSource();}
    bool wantsDxfBlockChildrenSource() const override{return facets&&dx_iface::wantsDxfBlockChildrenSource();}
    bool wantsDxfBlockDeliverySource() const override{return facets&&dx_iface::wantsDxfBlockDeliverySource();}
    bool wantsDxfEntitiesSource() const override{return facets&&dx_iface::wantsDxfEntitiesSource();}
    bool wantsDxfEntitiesDeliverySource() const override{return facets&&dx_iface::wantsDxfEntitiesDeliverySource();}
    void addBlock(const DRW_Block& v) override{++depth;dx_iface::addBlock(v);}
    void endBlock() override{if(depth)--depth;dx_iface::endBlock();}
    void inspect(const DRW_Table& v,const DRW_RawDxfObject* raw,bool source) {
        const auto* input=DxIfaceTableSourceTestAccess::readInput(*cData);dxfRW::DxfTableSourceView view;
        {SymbolAllocationFailure noAllocation(0);
            const bool active=input&&input->getDxfTableSourceView(view);
            if(selected?(!active||view.model!=&v||view.inBlock!=(depth!=0u)
                ||view.phase!=(source?dxfRW::DxfTableSourceView::Phase::Source:dxfRW::DxfTableSourceView::Phase::Typed)
                ||(raw&&view.source!=raw)||(!source&&view.source!=publishedRaw)):active)bad=true;
            if(tableInputReport(*cData).state!=TableInputState::Reading||tableInputReport(*cData).originalPairsMatched)bad=true;
        }
        if(source)publishedRaw=raw;
    }
    void addDxfTableSource(const DRW_Table& v,const DRW_RawDxfObject& raw) override {
        ++sources;inspect(v,&raw,true);before=dxfTable::Decoder::observe(v);
        if(contextProbe){dxfRW::DxfReadContext context;const auto* input=DxIfaceTableSourceTestAccess::readInput(*cData);const auto count=symbolFailedAllocations;
            {SymbolAllocationFailure fail(0);if(!input||!input->getDxfReadContext(context)||context.sourceVersion!=DRW::AC1032
                ||context.codecCodePage.empty()||context.declaration.size()!=96u||!context.hasDeclaration)bad=true;}
            contextViewAllocationFree=symbolFailedAllocations==count;
        }
        if(fault!=3) {
            if(fault==1){const DRW_Table copy(v);dx_iface::addDxfTableSource(copy,raw);}
            else if(fault==2){const DRW_RawDxfObject copy(raw);dx_iface::addDxfTableSource(v,copy);}
            else {const auto count=symbolFailedAllocations;SymbolAllocationFailure fail(allocation);dx_iface::addDxfTableSource(v,raw);allocationReached=count!=symbolFailedAllocations;}
            if(fault==4)dx_iface::addDxfTableSource(v,raw);
        }
        auto changed=before;
        switch(stateFault){case 0:++changed.subclass;break;case 1:++changed.rows;break;case 2:++changed.columns;break;case 3:++changed.currentCell;break;
        case 4:++changed.rowHeights;break;case 5:++changed.columnWidths;break;case 6:++changed.nextCell;break;case 7:changed.inValue=!changed.inValue;break;
        case 8:changed.dataVersion=!changed.dataVersion;break;case 9:changed.properties=!changed.properties;break;case 10:changed.unsupported=!changed.unsupported;break;
        case 11:++changed.attributesExpected;break;case 12:++changed.attributesRead;break;case 13:++changed.attributesContent;break;default:break;}
        if(stateFault>=0)DrwEntityEncodeTestAccess::tableState(const_cast<DRW_Table&>(v),changed);
        if(alterField)const_cast<DRW_Table&>(v).m_horizontalDirection.x=42.;
        if(throwSource)throw std::runtime_error("TABLE input source callback");
    }
    void addTable(const DRW_Table& v) override {
        ++typed;inspect(v,nullptr,false);
        if(fault!=6){if(fault==5){const DRW_Table copy(v);dx_iface::addTable(copy);}else if(fault==8)dx_iface::addInsert(v);else dx_iface::addTable(v);
            if(fault==7)dx_iface::addTable(v);}
        if(stateFault>=0)DrwEntityEncodeTestAccess::tableState(const_cast<DRW_Table&>(v),before);
        if(alterField)const_cast<DRW_Table&>(v).m_horizontalDirection.x=0.;
        publishedRaw=nullptr;if(throwTyped)throw std::runtime_error("TABLE input typed callback");
    }
    void clear(){fault=0;stateFault=-1;allocation=-1;selected=facets=true;throwSource=throwTyped=alterField=contextProbe=bad=false;sources=typed=0u;publishedRaw=nullptr;}
};
struct TablePublicationReceiver final:dx_iface {
    dxfRW* input=nullptr;bool selected=true,bad=false;unsigned depth=0u,sources=0u,typed=0u;int throwing=0;
    const DRW_Table* model=nullptr;const DRW_RawDxfObject* raw=nullptr;
    bool wantsDxfTableSource() const override{return selected;}
    bool wantsDxfBlockSource() const override{return false;}
    bool wantsDxfBlockChildrenSource() const override{return false;}
    bool wantsDxfBlockDeliverySource() const override{return false;}
    bool wantsDxfEntitiesSource() const override{return false;}
    bool wantsDxfEntitiesDeliverySource() const override{return false;}
    void addHeader(const DRW_Header*) override{}
    void addBlock(const DRW_Block&) override{++depth;}
    void endBlock() override{if(depth)--depth;}
    void inspect(const DRW_Table& value,bool source) {
        dxfRW::DxfTableSourceView view;SymbolAllocationFailure fail(0);
        const bool active=input&&input->getDxfTableSourceView(view);
        if(selected?(!active||view.model!=&value||view.inBlock!=(depth!=0u)
            ||view.phase!=(source?dxfRW::DxfTableSourceView::Phase::Source:dxfRW::DxfTableSourceView::Phase::Typed)
            ||(!source&&(view.model!=model||view.source!=raw))):active)bad=true;
    }
    void addDxfTableSource(const DRW_Table& value,const DRW_RawDxfObject& source) override {
        ++sources;model=&value;raw=&source;inspect(value,true);if(throwing==1)throw std::runtime_error("TABLE public source view");
    }
    void addTable(const DRW_Table& value) override {++typed;inspect(value,false);if(throwing==2)throw std::runtime_error("TABLE public typed view");}
};
Tags tablePublicationDocument(bool blocks) {
    const auto full=tableBlockSourceDocument("AC1032",true,1u);const auto begin=slot(full,5,"501")-1u,end=slot(full,5,"F7")-1u;
    Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1032"},{0,"ENDSEC"},{0,"SECTION"},{2,blocks?"BLOCKS":"ENTITIES"}};
    if(blocks)tags.insert(tags.end(),{{0,"BLOCK"},{5,"F6"},{330,"B3"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},{2,"Definition"},{70,"0"},{10,"0"},{20,"0"}});
    tags.insert(tags.end(),full.begin()+static_cast<std::ptrdiff_t>(begin),full.begin()+static_cast<std::ptrdiff_t>(end));
    if(blocks)tags.insert(tags.end(),{{0,"ENDBLK"},{5,"F7"},{330,"B3"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"}});
    tags.insert(tags.end(),{{0,"ENDSEC"},{0,"EOF"}});return tags;
}
bool tableInputTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::TableInputState;
    struct Stage{const char* name="profiles";bool done=false;~Stage(){if(!done)std::cerr<<"TABLE inputs stage="<<name<<'\n';}}stage;
    const auto path=dir/"table-inputs.dxf";unsigned profiles=0u,states=0u,refusals=0u,contexts=0u,quotas=0u,allocations=0u,failures=0u,boundaries=0u,views=0u,contextFailures=0u;
    const auto complete=[](const dx_data& d){const auto r=dx_iface::tableInputReport(d);return r.state==S::OriginalPairsReady&&r.sources==1u&&r.typed==1u&&r.paired==1u
        &&r.originalPairsMatched&&r.cleanStates&&!r.modeledMatched&&!r.profilesMatched&&!r.ownerTargetsResolved&&!r.outputEligible;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true})for(unsigned form=0u;form<3u;++form){
        if(!writeBlockBinding(path,tableBlockSourceDocument(version,blocks,form),encoding))return false;TableInputProbe reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||reader.bad||reader.depth||!complete(data)){std::cerr<<version<<'/'<<encoding<<" blocks="<<blocks<<" form="<<form<<" state="<<static_cast<int>(dx_iface::tableInputReport(data).state)<<'\n';return false;}
        const auto& p=*A::tableInputs(data).at(0x501u);const auto& t=*A::tableTypedInputs(data).at(0x501u);
        if(p.typed||!t.typed||t.source!=A::tableInputs(data).at(0x501u)||p.raw!=t.raw||p.context!=t.context||p.state.rows!=1||p.state.columns!=1||p.state.nextCell!=1u
            ||p.state.rowHeights!=1u||p.state.columnWidths!=1u||!p.state.dataVersion||p.inBlock!=blocks||p.linked.size()!=(form==1u?3u:form?1u:0u)
            ||dxfTable::Decoder::observe(*p.model).rows!=-1||!A::tablePendingEmpty(data))return false;
        if(form==1u&&( !p.linked[0].attributeSubclass||dxfCompound::Decoder::observe(*p.model->attlist[0]).attributeSubclass))return false;
        ++profiles;
    }
    for(bool binary:{false,true})for(bool blocks:{false,true}) {
        const auto good=tableBlockSourceDocument("AC1032",blocks,1u);
        const auto retry=[&](TableInputProbe& reader){reader.clear();dx_data next;return write(path,good,binary)&&reader.fileImport(path.string(),&next,false)&&!reader.bad&&!reader.depth&&complete(next);};
        stage.name="every original private TABLE state field";
        if(!write(path,good,binary))return false;
        for(int field=0;field<14;++field){TableInputProbe reader;reader.stateFault=field;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||reader.bad||dx_iface::tableInputReport(data).state!=S::Unqualified||!retry(reader))return false;++states;}
        stage.name="producer identities and callback phases";
        for(int fault=1;fault<=8;++fault){TableInputProbe reader;reader.fault=fault;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||reader.bad||dx_iface::tableInputReport(data).state!=(fault==4?S::CaptureFailed:S::Unqualified)||!retry(reader)){std::cerr<<"input fault="<<fault<<" blocks="<<blocks
                <<" state="<<static_cast<int>(dx_iface::tableInputReport(data).state)<<" deliveries="<<A::entitiesDeliveries(data).size()<<'\n';return false;}++refusals;}
        stage.name="view opt-out and context-owner independence";
        for(bool sourceFacet:{false,true}){TableInputProbe reader;reader.selected=sourceFacet;reader.facets=!sourceFacet;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||reader.bad||dx_iface::tableInputReport(data).state!=(sourceFacet?S::OriginalPairsReady:S::Unqualified))return false;++contexts;}
        stage.name="snapshot ownership and retained fields";
        for(bool typed:{false,true})for(bool alias:{false,true}){TableInputProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;A::substituteTableInput(data,typed,alias);
            if(dx_iface::tableInputReport(data).state!=S::ContextMismatch||!retry(reader))return false;++contexts;}
        for(bool alias:{false,true}){TableInputProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;A::substituteTableContext(data,blocks,alias);
            if(dx_iface::tableInputReport(data).state!=S::ContextMismatch)return false;++contexts;}
        {TableInputProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;
            data.tables.front().m_content.m_rows.clear();data.tables.front().parentHandle=0x40u;data.headerC.clearDxfCodePageDeclaration();
            if(!complete(data)||A::tableInputs(data).at(0x501u)->model->m_content.m_rows.size()!=1u)return false;
            {SymbolAllocationFailure fail(0);if(!complete(data))return false;}++contexts;
            dx_data copy;A::copyBindingContext(copy,data);if(dx_iface::tableInputReport(copy).state!=S::ContextMismatch)return false;++contexts;}
        {TableInputProbe first,second;dx_data a,b;if(!first.fileImport(path.string(),&a,false)||!second.fileImport(path.string(),&b,false))return false;A::transplantTableInput(a,b);
            if(dx_iface::tableInputReport(a).state!=S::ContextMismatch)return false;++contexts;}
        stage.name="actual field observations are not modeled equality";
        {TableInputProbe reader;reader.alterField=true;dx_data data;if(!reader.fileImport(path.string(),&data,false)||reader.bad||!complete(data)
                ||A::tableInputs(data).at(0x501u)->model->m_horizontalDirection.x==A::tableTypedInputs(data).at(0x501u)->model->m_horizontalDirection.x)return false;
            ++boundaries;
        }
        for(bool child:{false,true}){auto tags=good;appendBlockFields(tags,child?"511":"501",{{1001,"APP"},{1010,"7"}});if(!write(path,tags,binary))return false;TableInputProbe reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||reader.bad||dx_iface::tableInputReport(data).state!=S::OriginalPairsReady||dx_iface::tableInputReport(data).cleanStates
                ||(child?A::tableInputs(data).at(0x501u)->linked[0].hasNoPendingXdata():A::tableInputs(data).at(0x501u)->state.hasNoPendingXdata()))return false;
            const auto& p=*A::tableInputs(data).at(0x501u);
            if(child?!dxfCompound::Decoder::observe(*p.model->attlist[0]).hasNoPendingXdata():!dxfTable::Decoder::observe(*p.model).hasNoPendingXdata())return false;
            ++boundaries;
        }
        stage.name="borrowed context and bounded owned-context copy failure/recovery";
        for(bool capture:{false,true}){auto tags=good;const auto at=slot(tags,0,"ENDSEC");const std::string declaration(96u,'X');
            tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{9,"$DWGCODEPAGE"},{3,declaration}});
            if(!write(path,tags,binary))return false;TableInputProbe reader;reader.contextProbe=!capture;reader.facets=!capture;reader.allocation=capture?0:-1;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||reader.bad||(capture?(!reader.allocationReached||dx_iface::tableInputReport(data).state!=S::CaptureFailed)
                :(!reader.contextViewAllocationFree||!complete(data)||A::tableInputs(data).at(0x501u)->context->declaration!=declaration)))return false;
            if(!retry(reader))return false;++contextFailures;
        }
        stage.name="record/byte/work exact and one-under boundaries";
        if(!write(path,good,binary))return false;TableInputProbe baseline;dx_data base;if(!baseline.fileImport(path.string(),&base,false)||!complete(base))return false;
        const auto used=dx_iface::tableInputReport(base);const std::array<std::size_t,3> caps{{used.records,used.bytes,used.work}};
        for(unsigned which=0u;which<3u;++which)for(bool under:{false,true}){TableInputProbe reader;dx_data data;A::tableInputLimit(data,which,caps[which]-(under?1u:0u));
            if(!reader.fileImport(path.string(),&data,false)||reader.bad||dx_iface::tableInputReport(data).state!=(under?S::CaptureFailed:S::OriginalPairsReady)||!retry(reader))return false;++quotas;}
        stage.name="reached actual source snapshot allocation failures";
        for(long fault=0;fault<20;++fault){TableInputProbe reader;reader.allocation=fault;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||!reader.allocationReached||dx_iface::tableInputReport(data).state!=S::CaptureFailed||!A::tablePendingEmpty(data)||!retry(reader))return false;++allocations;}
        stage.name="whole-read tail and callback exception recovery";
        for(unsigned which=0u;which<3u;++which){auto tags=good;if(which==2u)tags.pop_back();if(!write(path,tags,binary))return false;TableInputProbe reader;reader.throwSource=which==0u;reader.throwTyped=which==1u;
            dx_data data;bool read=false,threw=false;try{read=reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
            if(read||reader.depth||threw!=(which!=2u)||dx_iface::tableInputReport(data).state!=S::ImportFailed||!A::tablePendingEmpty(data)||!retry(reader))return false;++failures;}
    }
    stage.name="public callback-only lifetime,throw cleanup and reader reuse";
    for(bool binary:{false,true})for(bool blocks:{false,true})for(unsigned mode=0u;mode<4u;++mode) {
        if(!write(path,tablePublicationDocument(blocks),binary))return false;dxfRW input(path.string().c_str());TablePublicationReceiver receiver;receiver.input=&input;
        receiver.throwing=mode<3u?static_cast<int>(mode):0;receiver.selected=mode!=3u;
        const auto cleared=[&](){dxfRW::DxfTableSourceView view;DRW_Table dummy;DRW_RawDxfObject raw;view.phase=dxfRW::DxfTableSourceView::Phase::Typed;view.model=&dummy;view.source=&raw;view.inBlock=true;
            SymbolAllocationFailure fail(0);return !input.getDxfTableSourceView(view)&&view.phase==dxfRW::DxfTableSourceView::Phase::None&&!view.model&&!view.source&&!view.inBlock;};
        if(!cleared())return false;bool read=false,threw=false;try{read=input.read(&receiver,false);}catch(const std::runtime_error&){threw=true;}
        if(receiver.bad||receiver.depth||read!=(mode==0u||mode==3u)||threw!=(mode==1u||mode==2u)||!cleared())return false;
        receiver.throwing=0;receiver.bad=false;receiver.sources=receiver.typed=0u;receiver.model=nullptr;receiver.raw=nullptr;
        if(!input.read(&receiver,false)||receiver.bad||receiver.depth||receiver.sources!=(receiver.selected?1u:0u)||receiver.typed!=1u||!cleared())return false;++views;
    }
    {if(!write(path,entitiesRoleDocument("AC1032"),false))return false;TableInputProbe reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||dx_iface::tableInputReport(data).state!=S::NoInputs||dx_iface::tableInputReport(data).originalPairsMatched)return false;++contexts;}
    stage.done=true;std::cout<<"TABLE inputs profiles="<<profiles<<"; original state mutations="<<states<<"; producer refusals="<<refusals<<"; context/owner controls="<<contexts
        <<"; field/pending boundaries="<<boundaries<<"; exact/under quotas="<<quotas<<"; reached allocation faults="<<allocations<<"; callback/tail retries="<<failures<<"; public lifetime/reuse routes="<<views
        <<"; borrowed/owned long-declaration context controls="<<contextFailures<<'\n';
    return profiles==72u&&states==56u&&refusals==32u&&contexts==45u&&boundaries==12u&&quotas==24u&&allocations==80u&&failures==12u&&views==16u&&contextFailures==8u;
}
