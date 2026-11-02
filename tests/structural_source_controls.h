// SPDX-License-Identifier: GPL-2.0-or-later
// Actual-import producer/origin controls; no external drawing fixtures or
// ownership, native consumer, live/output or semantic support promotion.
struct StructuralSourceProbe final:dx_iface {
    using View=dxfRW::DxfStructuralSourceView;
    bool blocks=true,root=true,good=true;int skip=-1,copy=-1,duplicate=-1,throwAt=-1,mutate=-1,allocationAt=-1;
    long allocation=-1;unsigned phases=0u;std::vector<int> order;
    bool wantsDxfBlockRecordSource() const override {return blocks&&dx_iface::wantsDxfBlockRecordSource();}
    bool wantsDxfRootDictionarySource() const override {return root&&dx_iface::wantsDxfRootDictionarySource();}
    void inspect(const DRW_TableEntry& value,int phase,const DRW_RawDxfObject* raw=nullptr) {
        const auto* input=DxIfaceTableSourceTestAccess::readInput(*cData);View view;
        const bool selected=phase<2?blocks:root;
        if(!selected) {if(input&&input->getDxfStructuralSourceView(view))good=false;return;}
        if(!input||!input->getDxfStructuralSourceView(view)||view.model!=&value||(raw&&view.source!=raw)
            ||view.kind!=(phase<2?View::Kind::BlockRecord:View::Kind::RootDictionary)
            ||view.phase!=((phase==0||phase==3)?View::Phase::Source:View::Phase::Typed)
            ||structuralSourceReport(*cData).state!=StructuralSourceState::Reading)good=false;
        ++phases;order.push_back(phase);
    }
    template<class F>void invoke(int phase,F action) {
        if(skip==phase)return;
        if(allocationAt==phase){SymbolAllocationFailure failure(allocation);action();}
        else action();
        if(duplicate==phase)action();
        if(throwAt==phase)throw std::runtime_error("structural publication callback");
    }
    void addDxfBlockRecordSource(const DRW_Block_Record& row,const DRW_RawDxfObject& raw) override {
        inspect(row,0,&raw);invoke(0,[&]{if(copy==0){auto other=raw;dx_iface::addDxfBlockRecordSource(row,other);}else dx_iface::addDxfBlockRecordSource(row,raw);});
    }
    void addBlockRecord(const DRW_Block_Record& row) override {
        inspect(row,1);auto& actual=const_cast<DRW_Block_Record&>(row);const auto before=actual.insUnits;if(mutate==1)++actual.insUnits;
        invoke(1,[&]{if(copy==1){auto other=row;dx_iface::addBlockRecord(other);}else dx_iface::addBlockRecord(row);});actual.insUnits=before;
    }
    void addNamedObjectDictionary(const DRW_Dictionary& row) override {
        inspect(row,2);auto& actual=const_cast<DRW_Dictionary&>(row);const auto before=actual.cloning;if(mutate==2)++actual.cloning;
        invoke(2,[&]{if(copy==2){auto other=row;dx_iface::addNamedObjectDictionary(other);}else dx_iface::addNamedObjectDictionary(row);});actual.cloning=before;
    }
    void addDxfRootDictionarySource(const DRW_Dictionary& row,const DRW_RawDxfObject& raw) override {
        inspect(row,3,&raw);auto& actual=const_cast<DRW_Dictionary&>(row);const auto before=actual.cloning;if(mutate==3)++actual.cloning;
        invoke(3,[&]{if(copy==3){auto other=raw;dx_iface::addDxfRootDictionarySource(row,other);}else dx_iface::addDxfRootDictionarySource(row,raw);});actual.cloning=before;
    }
};
Tags structuralSourceDocument(const std::string& version) {
    auto tags=identitySourceDocument(version);auto at=slot(tags,2,"TABLES")+1u;
    while(tags[at].code!=0||tags[at].value!="ENDSEC")++at;
    Tags records{{0,"TABLE"},{2,"BLOCK_RECORD"},{5,"8"},{100,"AcDbSymbolTable"},{70,"2"}};
    for(const char* handle:{"B0","B1"}) {
        records.insert(records.end(),{{0,"BLOCK_RECORD"},{5,handle},{330,"8"},{100,"AcDbSymbolTableRecord"},{2,handle==std::string("B0")?"DEF":"*Paper_Space"},
            {100,"AcDbBlockTableRecord"},{70,"4"},{280,"1"},{281,"0"},{340,handle==std::string("B0")?"A0":"A1"},{310,"A500"},
            {102,"{BLKREFS"},{331,"30"},{102,"}"},{1001,"STRUCTURAL_APP"},{1003,"STRUCTURAL_LAYER"},{1005,"41"},
            {1010,"1.25"},{1020,"2.5"},{1030,"3.75"}});
    }
    records.push_back({0,"ENDTAB"});tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),records.begin(),records.end());
    return tags;
}
bool structuralPublicLifetime(unsigned& contexts) {
    struct Consumer final:dx_iface {
        dxfRW* input=nullptr;bool enabled=true,disable=false,good=true;int throwAt=-1;std::vector<int> calls;
        bool wantsDxfBlockRecordSource() const override {return enabled;}
        bool wantsDxfRootDictionarySource() const override {return enabled;}
        void addHeader(const DRW_Header*) override {}
        void check(const DRW_TableEntry& row,const DRW_RawDxfObject* raw,int phase) {
            dxfRW::DxfStructuralSourceView v;
            if(!input->getDxfStructuralSourceView(v)||v.model!=&row||(raw&&v.source!=raw)
                ||v.phase!=((phase==0||phase==3)?dxfRW::DxfStructuralSourceView::Phase::Source:dxfRW::DxfStructuralSourceView::Phase::Typed))good=false;
            calls.push_back(phase);if(disable)enabled=false;if(throwAt==phase)throw std::runtime_error("structural public lifetime");
        }
        void addBlockRecord(const DRW_Block_Record& r) override {if(enabled||disable)check(r,nullptr,1);}
        void addDxfBlockRecordSource(const DRW_Block_Record& r,const DRW_RawDxfObject& raw) override {check(r,&raw,0);}
        void addNamedObjectDictionary(const DRW_Dictionary& r) override {if(enabled)check(r,nullptr,2);}
        void addDxfRootDictionarySource(const DRW_Dictionary& r,const DRW_RawDxfObject& raw) override {check(r,&raw,3);}
    } consumer;
    dxfRW reader("unused");consumer.input=&reader;
    std::string document="0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1032\n0\nENDSEC\n0\nSECTION\n2\nTABLES\n0\nTABLE\n2\nBLOCK_RECORD\n5\n8\n0\nBLOCK_RECORD\n5\nB0\n330\n8\n2\nDEF\n0\nBLOCK_RECORD\n5\nB1\n330\n8\n2\nSECOND\n0\nENDTAB\n0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n0\nDICTIONARY\n5\n40\n330\n0\n100\nAcDbDictionary\n0\nENDSEC\n0\nEOF";
    for(int phase=-1;phase<6;++phase) {
        consumer.enabled=phase!=5;consumer.disable=phase==4;consumer.throwAt=phase<4?phase:-1;consumer.calls.clear();
        dxfRW::DxfStructuralSourceView view;view.model=reinterpret_cast<const DRW_TableEntry*>(1);
        if(reader.getDxfStructuralSourceView(view)||view.model||view.source||view.kind!=dxfRW::DxfStructuralSourceView::Kind::None)return false;
        bool threw=false;try{if(!reader.readAscii(&consumer,false,document))return false;}catch(const std::runtime_error&){threw=true;}
        if(threw!=(phase>=0&&phase<4)||reader.getDxfStructuralSourceView(view)||view.model||view.source||!consumer.good)return false;
        if(phase==-1&&consumer.calls!=std::vector<int>{0,1,0,1,2,3})return false;
        if(phase==4&&consumer.calls!=std::vector<int>{0,1,0,1})return false;
        if(phase==5&&!consumer.calls.empty())return false;++contexts;
    }
    return true;
}
bool structuralSourceTests(const std::filesystem::path& dir) {
    struct Stage {const char* name="matrix";bool done=false;~Stage(){if(!done)std::cerr<<"Structural source stage="<<name<<'\n';}} stage;
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::StructuralSourceState;
    const auto path=dir/"structural-source.dxf";unsigned profiles=0u,refusals=0u,quotas=0u,contexts=0u,exceptions=0u;long allocations=0;std::size_t origins=0u;
    const auto complete=[](const dx_data& data){const auto r=dx_iface::structuralSourceReport(data);return r.state==S::OriginsBound&&r.records==3u&&r.paired==3u
        &&r.bodyMatched==3u&&r.stateComplete==3u&&r.originsBound==3u&&r.namespaceReady&&!r.ownerTargetsResolved&&!r.outputEligible;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding) {
        auto tags=structuralSourceDocument(version);if(encoding==2u)tags.erase(std::remove_if(tags.begin(),tags.end(),[](const auto& t){return t.code==999;}),tags.end());
        if(!writeBlockBinding(path,tags,encoding))return false;StructuralSourceProbe reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||!complete(data)||!reader.good||reader.order!=std::vector<int>{0,1,0,1,2,3}) {
            const auto r=dx_iface::structuralSourceReport(data);std::cerr<<version<<'/'<<encoding<<" state="<<static_cast<int>(r.state)<<" pairs="<<r.paired<<" bodies="<<r.bodyMatched<<" complete="<<r.stateComplete<<" origins="<<r.originsBound<<'\n';return false;
        }
        const auto wire=attdefWireTags(path,encoding==2u);
        for(const auto& row:A::structuralAudits(data)) {
            const auto& audit=*row.second;const auto& source=*A::identityInput(data)->source;
            if(audit.physical>=source.records.size())return false;const auto& physical=source.records[audit.physical];
            const auto& marker=wire.at(physical.marker.ordinal-1u);
            if(marker.code||marker.text!=audit.input->raw->name||marker.begin!=physical.marker.beginOffset||marker.end!=physical.marker.endOffset)return false;
            if(audit.input->model.get()!=A::resources(data).at(row.first).model.get()||audit.input->raw.get()!=A::resources(data).at(row.first).raw.get())return false;++origins;
        }++profiles;
    }
    for(bool binary:{false,true}) {
        const auto good=structuralSourceDocument("AC1032");if(!write(path,good,binary))return false;StructuralSourceProbe baseline;dx_data base;
        if(!baseline.fileImport(path.string(),&base,false)||!complete(base))return false;
        stage.name="missing/copied/duplicate callback refusal";
        for(unsigned mode=0u;mode<3u;++mode)for(int phase=0;phase<4;++phase) {
            StructuralSourceProbe reader;if(mode==0u)reader.skip=phase;else if(mode==1u)reader.copy=phase;else reader.duplicate=phase;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::structuralSourceReport(data).state!=S::CaptureFailed)return false;
            reader.skip=reader.copy=reader.duplicate=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++refusals;
        }
        stage.name="typed/source mutation and original pending state";
        for(int phase=1;phase<=3;++phase) {
            StructuralSourceProbe reader;reader.mutate=phase;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::structuralSourceReport(data).state!=S::Unqualified) {
                const auto r=dx_iface::structuralSourceReport(data);std::cerr<<"mutated phase="<<phase<<" state="<<static_cast<int>(r.state)<<" paired="<<r.paired<<" bodies="<<r.bodyMatched<<'\n';return false;
            }
            reader.mutate=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++refusals;
        }
        for(bool dictionary:{false,true}) {
            auto tags=good;
            if(dictionary){const auto at=slot(tags,0,"XRECORD");tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{1001,"PENDING_APP"},{1010,"1"}});}
            else {const auto at=slot(tags,1030,"3.75");tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at));}
            if(!write(path,tags,binary))return false;StructuralSourceProbe reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::structuralSourceReport(data).state!=S::Unqualified) {
                const auto r=dx_iface::structuralSourceReport(data);std::cerr<<"pending dictionary="<<dictionary<<" state="<<static_cast<int>(r.state)<<" complete="<<r.stateComplete<<'\n';return false;
            }
            const auto h=dictionary?0x40u:0xB0u;const auto& input=*A::structuralInputs(data).at(h);
            if(!input.originalState.first||dxfBlockRecord::Decoder::observeCommonState(*input.model)!=input.originalState) {
                std::cerr<<"pending dictionary="<<dictionary<<" original="<<input.originalState.first<<" mask="<<static_cast<int>(input.originalState.second)<<" copied="<<dxfBlockRecord::Decoder::observeCommonState(*input.model).first<<'\n';return false;
            }
            if(!write(path,good,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++refusals;
        }
        stage.name="exact and one-under quotas";
        for(unsigned which=0u;which<3u;++which)for(bool exact:{false,true}) {
            StructuralSourceProbe reader;dx_data data;const auto r=dx_iface::structuralSourceReport(base);
            A::structuralLimit(data,which,(which==0u?r.records:which==1u?r.bytes:r.work)-(exact?0u:1u));
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::structuralSourceReport(data).state!=(exact?S::OriginsBound:S::CaptureFailed))return false;++quotas;
        }
        stage.name="owner/context/opt-out controls";
        for(unsigned which=0u;which<4u;++which) {
            StructuralSourceProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!complete(data))return false;
            A::substituteStructural(data,which);if(dx_iface::structuralSourceReport(data).state!=S::ContextMismatch)return false;++contexts;
        }
        for(unsigned which=0u;which<3u;++which) {
            StructuralSourceProbe reader;reader.blocks=which==1u;reader.root=which==2u;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::structuralSourceReport(data).state!=(which?S::OriginsBound:S::NotCaptured))return false;++contexts;
        }
        {StructuralSourceProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;dx_data copy;A::copyBindingContext(copy,data);
            if(dx_iface::structuralSourceReport(copy).state!=S::ContextMismatch)return false;++contexts;}
        {auto tags=good;tags[slot(tags,5,"30")].value="13";if(!write(path,tags,binary))return false;StructuralSourceProbe reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::structuralSourceReport(data).state!=S::ProducerPairsReady
                ||dx_iface::structuralSourceReport(data).originsBound)return false;
            ++contexts;if(!write(path,good,binary))return false;}
        stage.name="allocation recovery";
        for(int phase=0;phase<5;++phase) {
            bool recovered=false;
            for(long count=0;count<2000;++count) {
                StructuralSourceProbe reader;reader.allocationAt=phase<4?phase:-1;reader.allocation=count;dx_data data;
                if(!reader.fileImport(path.string(),&data,false))return false;
                if(phase==4){SymbolAllocationFailure failure(count);A::repeatStructuralAudit(reader,data);}
                const auto r=dx_iface::structuralSourceReport(data);if(r.state==S::OriginsBound){recovered=true;break;}
                if(r.state!=S::CaptureFailed&&r.state!=S::Unqualified)return false;
                if(phase==4){A::repeatStructuralAudit(reader,data);if(!complete(data))return false;}
                reader.allocationAt=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++allocations;
            }
            if(!recovered)return false;
        }
        stage.name="exceptions and failed tails";
        for(int phase=0;phase<5;++phase) {
            StructuralSourceProbe reader;reader.throwAt=phase<4?phase:-1;auto tags=good;if(phase==4)tags.pop_back();if(!write(path,tags,binary))return false;
            dx_data data;bool threw=false,read=false;try{read=reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
            if(read||threw!=(phase<4)||dx_iface::structuralSourceReport(data).state!=S::ImportFailed||!A::structuralAudits(data).empty())return false;
            reader.throwAt=-1;if(!write(path,good,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++exceptions;
        }
    }
    if(!structuralPublicLifetime(contexts))return false;
    std::cout<<"Structural source profiles="<<profiles<<" physical origins="<<origins<<" refusals="<<refusals<<" quotas="<<quotas
        <<" contexts="<<contexts<<" allocation faults="<<allocations<<" exceptions="<<exceptions<<'\n';stage.done=true;return true;
}
