// SPDX-License-Identifier: GPL-2.0-or-later
// Original TABLE source publication and structural membership only. Parent/body
// fields,common targets,dependencies,live edits/output/native semantics stay open.
Tags tableBlockSourceDocument(const std::string& version,bool blocks,unsigned form,bool footerToBlock=false) {
    auto tags=blocks?blockRoleDocument(version):entitiesRoleDocument(version);
    Tags body{{0,"ACAD_TABLE"},{5,"501"},{330,blocks?"B3":"B0"},{100,"AcDbEntity"},{8,"0"},
        {102,"{OPAQUE"},{5,"DEAD"},{330,"BEEF"},{410,"false-layout"},{102,"}"},
        {100,"AcDbBlockReference"},{2,"*T1"},{10,"1.25"},{20,"2.5"},{30,"3.75"},
        {100,"AcDbTable"},{280,"2"},{90,"2"},{91,"1"},{92,"1"},{141,"4.5"},{142,"6.25"},
        {171,"1"},{172,"0"},{173,"0"},{174,"0"},{1,"cell-é"}};
    if(form==1u)for(const char* id:{"511","512"})body.insert(body.end(),{{0,"ATTRIB"},{5,id},{330,"501"},{100,"AcDbEntity"},{8,"0"},
        {100,"AcDbText"},{10,"1"},{20,"2"},{40,"3"},{1,"value-é"},{100,"AcDbAttribute"},{2,id},{70,"0"}});
    if(form)body.insert(body.end(),{{0,"SEQEND"},{5,"513"},{330,footerToBlock?(blocks?"B3":"B0"):"501"},{100,"AcDbEntity"},{8,"0"}});
    const auto at=blocks?slot(tags,5,"F7")-1u:slot(tags,0,"EOF")-1u;
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),body.begin(),body.end());return tags;
}
struct TableBlockSourceProbe final:dx_iface {
    bool selected=true,bad=false,publicOnly=false,throwSource=false,throwTyped=false;unsigned depth=0u,sources=0u,typed=0u,projections=0u;
    int fault=0;long allocation=-1;bool allocationReached=false;unsigned moveChecks=0u;
    const DRW_Table* sourcePointer=nullptr;decltype(DrwEntityEncodeTestAccess::tableCursor(DRW_Table{})) sourceCursor;
    dxfCompound::SourceState sourceState;
    bool wantsDxfTableSource() const override{return selected&&(publicOnly||dx_iface::wantsDxfTableSource());}
    void addHeader(const DRW_Header* v) override{if(!publicOnly)dx_iface::addHeader(v);}
    bool wantsDxfBlockSource() const override{return !publicOnly&&dx_iface::wantsDxfBlockSource();}
    bool wantsDxfBlockChildrenSource() const override{return !publicOnly&&dx_iface::wantsDxfBlockChildrenSource();}
    bool wantsDxfBlockDeliverySource() const override{return !publicOnly&&dx_iface::wantsDxfBlockDeliverySource();}
    void addBlock(const DRW_Block& v) override{++depth;if(!publicOnly)dx_iface::addBlock(v);}
    void endBlock() override{if(depth)--depth;if(!publicOnly)dx_iface::endBlock();}
    void addDxfTableSource(const DRW_Table& v,const DRW_RawDxfObject& raw) override {
        ++sources;sourcePointer=&v;sourceCursor=DrwEntityEncodeTestAccess::tableCursor(v);sourceState=dxfCompound::Decoder::observe(v);
        if(raw.handle!=v.handle||raw.parentHandle!=v.parentHandle||raw.name!="ACAD_TABLE")bad=true;
        for(const auto& field:raw.groups)if(field.code()==0)bad=true;
        if(v.m_content.m_rows.size()!=1u||v.m_content.m_columns.size()!=1u||v.m_content.m_rows[0].m_cells.size()!=1u
            ||v.m_content.m_rows[0].m_cells[0].m_contents.size()!=1u||v.m_content.m_rows[0].m_cells[0].m_contents[0].m_text!="cell-é")bad=true;
        for(const auto& a:v.attlist)if(!a||!dxfCompound::Decoder::observe(*a).attributeSubclass)bad=true;
        // A copied snapshot sanitizes parser state, whereas moving that owned
        // snapshot must preserve it and transfer all dynamic owners without new.
        DRW_Table copy(v);const auto cursor=DrwEntityEncodeTestAccess::tableCursor(copy);
        if(cursor==sourceCursor)bad=true;
        const auto* rows=copy.m_content.m_rows.data();const auto* attributes=copy.attlist.data();
        {SymbolAllocationFailure noAllocation(0);DRW_Table moved(std::move(copy));DRW_Table assigned;assigned=std::move(moved);
            if(assigned.eType!=DRW::TABLE||DrwEntityEncodeTestAccess::tableCursor(assigned)!=cursor||assigned.m_content.m_rows.data()!=rows||assigned.attlist.data()!=attributes)bad=true;}
        moveChecks+=2u;
        if(!publicOnly){dxfRW::DxfBlockSourceView view;const auto* input=DxIfaceTableSourceTestAccess::readInput(*cData);
            if(depth){SymbolAllocationFailure noAllocation(0);if(!input||!input->getDxfBlockSourceView(view)||view.phase!=dxfRW::DxfBlockSourceView::Phase::CallbackValue
                    ||view.entity!=&v||view.raw!=&raw||!view.event||view.event->callback!=DRW_DxfBlockCallback::TableSource)bad=true;}}
        if(!publicOnly&&fault!=3) {
            if(fault==1){const DRW_Table copy(v);dx_iface::addDxfTableSource(copy,raw);}
            else if(fault==2){const DRW_RawDxfObject copy(raw);dx_iface::addDxfTableSource(v,copy);}
            else {const auto before=symbolFailedAllocations;SymbolAllocationFailure fail(allocation);dx_iface::addDxfTableSource(v,raw);allocationReached=symbolFailedAllocations!=before;}
            if(fault==4)dx_iface::addDxfTableSource(v,raw);
        }
        if(throwSource)throw std::runtime_error("TABLE source pair callback");
    }
    void addTable(const DRW_Table& v) override {
        ++typed;if(selected&&(&v!=sourcePointer||DrwEntityEncodeTestAccess::tableCursor(v)!=sourceCursor||!(dxfCompound::Decoder::observe(v)==sourceState)))bad=true;
        if(selected&&(std::get<1>(DrwEntityEncodeTestAccess::tableCursor(v))!=1||std::get<2>(DrwEntityEncodeTestAccess::tableCursor(v))!=1))bad=true;
        if(v.m_content.m_rows.size()!=1u||v.m_content.m_columns.size()!=1u||v.m_content.m_rows[0].m_cells.size()!=1u
            ||v.m_content.m_rows[0].m_cells[0].m_contents.size()!=1u||v.m_content.m_rows[0].m_cells[0].m_contents[0].m_text!="cell-é")bad=true;
        if(selected)for(const auto& a:v.attlist)if(!a||!dxfCompound::Decoder::observe(*a).attributeSubclass)bad=true;
        if(!publicOnly&&depth){dxfRW::DxfBlockSourceView view;const auto* input=DxIfaceTableSourceTestAccess::readInput(*cData);
            SymbolAllocationFailure noAllocation(0);if(!input||!input->getDxfBlockSourceView(view)||view.phase!=dxfRW::DxfBlockSourceView::Phase::CallbackValue
                ||view.entity!=&v||view.raw||!view.event||view.event->callback!=DRW_DxfBlockCallback::addTable)bad=true;}
        if(!publicOnly&&fault!=6){if(fault==5){const DRW_Table copy(v);dx_iface::addTable(copy);}else if(fault==8)dx_iface::addInsert(v);else dx_iface::addTable(v);
            if(fault==7)dx_iface::addTable(v);}
        sourcePointer=nullptr;if(throwTyped)throw std::runtime_error("TABLE typed pair callback");
    }
    void addInsert(const DRW_Insert& v) override{if(v.handle==0x501u)++projections;dx_iface::addInsert(v);}
    void clearFaults(){fault=0;allocation=-1;selected=true;throwSource=throwTyped=false;bad=false;sources=typed=projections=0u;sourcePointer=nullptr;}
};
bool tableBlockSourceTests(const std::filesystem::path& dir) {
    struct Stage{const char* value="profiles";bool done=false;~Stage(){if(!done)std::cerr<<"TABLE source stage="<<value<<'\n';}}stage;
    using A=DxIfaceTableSourceTestAccess;const auto path=dir/"table-block-source.dxf";unsigned profiles=0u,publicRoutes=0u,ownerForms=0u,refusals=0u,quotas=0u,failures=0u,allocations=0u,moves=0u,contexts=0u;std::size_t spans=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{true,false})for(unsigned form=0u;form<3u;++form){
        if(!writeBlockBinding(path,tableBlockSourceDocument(version,blocks,form),encoding))return false;TableBlockSourceProbe reader;dx_data data;
        const bool read=reader.fileImport(path.string(),&data,false);
        if(!read||reader.bad||reader.depth||reader.sources!=1u||reader.typed!=1u||reader.projections!=1u){std::cerr<<"TABLE pair "<<version<<'/'<<encoding<<" blocks="<<blocks<<" form="<<form<<" read="<<read<<" source="<<reader.sources<<" typed="<<reader.typed<<" bad="<<reader.bad<<'\n';return false;}
        if(data.tables.size()!=1u||A::table(data).attlist.size()!=(form==1u?2u:0u)||static_cast<bool>(A::table(data).seqEndEntity)!=(form!=0u)){std::cerr<<"TABLE retained form="<<form<<'\n';return false;}
        if(blocks){const auto& row=A::blockSources(data).at(0xF6u);
            if(dx_iface::blockOriginReport(data).state!=dx_iface::BlockOriginState::OriginsBound||!row.membership||!row.membership->complete||row.membershipObserved!=row.delivery->events.size()){
                std::cerr<<"TABLE origins form="<<form<<" state="<<static_cast<int>(dx_iface::blockOriginReport(data).state)<<" membership="<<(row.membership&&row.membership->complete)<<" observed="<<row.membershipObserved<<" events="<<row.delivery->events.size()<<'\n';return false;}
            const auto& d=row.delivery->dispatches.back();if(d.events!=2u||row.delivery->events[d.firstEvent].callback!=DRW_DxfBlockCallback::TableSource
                ||row.delivery->events[d.firstEvent+1u].callback!=DRW_DxfBlockCallback::addTable)return false;
            if(!blockBindingOriginalWire(path,encoding==2u,tableBlockSourceDocument(version,blocks,form),data,spans))return false;
            const auto report=dx_iface::blockRoleReport(data);if(report.state!=dx_iface::BlockRoleState::RolesReady||report.wholeDocumentMatched||report.outputEligible)return false;
        }else if(dx_iface::entitiesDeliveryReport(data).state!=dx_iface::EntitiesDeliveryState::ObservedTransportComplete)return false;
        moves+=reader.moveChecks;++profiles;
    }
    for(bool binary:{false,true}) {
        const auto good=tableBlockSourceDocument("AC1032",true,1u);
        const auto complete=[](const dx_data& d){const auto& row=A::blockSources(d).at(0xF6u);return dx_iface::blockOriginReport(d).state==dx_iface::BlockOriginState::OriginsBound
            &&row.membership&&row.membership->complete&&row.membershipObserved==row.delivery->events.size();};
        const auto retry=[&](TableBlockSourceProbe& reader){reader.clearFaults();dx_data next;return write(path,good,binary)&&reader.fileImport(path.string(),&next,false)&&!reader.bad&&!reader.depth&&reader.sources==1u&&reader.typed==1u&&reader.projections==1u&&complete(next);};
        stage.value="plain source facet independent of converter/block observers";
        for(bool selected:{false,true})for(unsigned form=0u;form<3u;++form) {
            const auto full=tableBlockSourceDocument("AC1032",true,form);const auto first=slot(full,5,"501")-1u,last=slot(full,5,"F7")-1u;
            Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1032"},{0,"ENDSEC"},{0,"SECTION"},{2,"BLOCKS"},
                {0,"BLOCK"},{5,"F6"},{330,"B3"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},{2,"Definition"},{70,"0"},{10,"0"},{20,"0"}};
            tags.insert(tags.end(),full.begin()+static_cast<std::ptrdiff_t>(first),full.begin()+static_cast<std::ptrdiff_t>(last));
            tags.insert(tags.end(),{{0,"ENDBLK"},{5,"F7"},{330,"B3"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"},{0,"ENDSEC"},{0,"EOF"}});
            if(!write(path,tags,binary))return false;TableBlockSourceProbe receiver;receiver.publicOnly=true;receiver.selected=selected;dxfRW input(path.string().c_str());
            if(!input.read(&receiver,false)||receiver.bad||receiver.depth||receiver.sources!=(selected?1u:0u)||receiver.typed!=1u||receiver.projections)return false;
            moves+=receiver.moveChecks;++publicRoutes;
        }
        stage.value="both observed footer owners and explicit unsupported body";
        for(unsigned form=1u;form<3u;++form){if(!write(path,tableBlockSourceDocument("AC1032",true,form,true),binary))return false;TableBlockSourceProbe reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||reader.bad||!complete(data)||A::table(data).seqEndEntity->parentHandle!=0xB3u)return false;++ownerForms;}
        stage.value="converter opt-out and original pending XDATA state";
        {if(!write(path,good,binary))return false;TableBlockSourceProbe reader;reader.selected=false;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||reader.bad||reader.sources||reader.typed!=1u||reader.projections!=1u||!complete(data))return false;++contexts;}
        {auto tags=good;appendBlockFields(tags,"501",{{1001,"APP"},{1010,"7"}});if(!write(path,tags,binary))return false;TableBlockSourceProbe reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||reader.bad||!complete(data)||reader.sourceState.hasNoPendingXdata()
                ||!dxfCompound::Decoder::observe(A::table(data)).hasNoPendingXdata())return false;
            ++contexts;
        }
        stage.value="copied/missing/duplicate source and typed publications";
        if(!write(path,good,binary))return false;
        for(int fault=1;fault<=8;++fault){TableBlockSourceProbe reader;reader.fault=fault;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||complete(data)||!retry(reader)){std::cerr<<"TABLE publication fault="<<fault<<'\n';return false;}++refusals;}
        stage.value="source snapshot exact/one-under limits";
        TableBlockSourceProbe baseline;dx_data base;if(!baseline.fileImport(path.string(),&base,false)||!complete(base))return false;
        const auto used=dx_iface::tableSourceReport(base);
        for(unsigned which=0u;which<3u;++which)for(bool under:{false,true}){TableBlockSourceProbe reader;dx_data data;
            A::limits(data,which==0u?used.records-(under?1u:0u):100000u,which==1u?used.groups-(under?1u:0u):4000000u,which==2u?used.retainedBytes-(under?1u:0u):64u*1024u*1024u);
            if(!reader.fileImport(path.string(),&data,false)||reader.bad||reader.typed!=1u
                ||(dx_iface::tableSourceReport(data).state==dx_iface::TableSourceState::CaptureFailed)!=under||!retry(reader))return false;
            ++quotas;
        }
        stage.value="reached source-snapshot allocation faults";
        for(long fault=0;fault<16;++fault){TableBlockSourceProbe reader;reader.allocation=fault;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||!reader.allocationReached||dx_iface::tableSourceReport(data).state!=dx_iface::TableSourceState::CaptureFailed||!retry(reader))return false;++allocations;}
        stage.value="malformed linked chain and whole-import callback/tail retries";
        for(unsigned kind=0u;kind<7u;++kind){auto tags=good;TableBlockSourceProbe reader;
            if(kind==0u)eraseBlockRecord(tags,"513");if(kind==1u)eraseBlockRecord(tags,"F7");
            if(kind==2u)tags[blockField(tags,"511",330)].value="DEAD";
            if(kind==3u)tags[blockField(tags,"513",330)].value="DEAD";
            if(kind==4u)reader.throwSource=true;if(kind==5u)reader.throwTyped=true;if(kind==6u)tags.pop_back();
            if(!write(path,tags,binary))return false;dx_data data;bool read=false,threw=false;try{read=reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
            if(reader.depth||threw!=(kind==4u||kind==5u))return false;
            if(kind==2u||kind==3u){if(!read||complete(data))return false;}
            else if(read||dx_iface::blockOriginReport(data).state!=dx_iface::BlockOriginState::ImportFailed)return false;
            if(!retry(reader))return false;++failures;}
    }
    stage.done=true;std::cout<<"TABLE source profiles="<<profiles<<"; independent BLOCKS scalar spans="<<spans<<"; public selected/opt-out routes="<<publicRoutes
        <<"; footer owner variants="<<ownerForms<<"; copied/missing/duplicate refusals="<<refusals<<"; exact/under source quotas="<<quotas<<"; reached allocation faults="<<allocations
        <<"; callback/tail/linked failure retries="<<failures<<"; allocation-free move checks="<<moves<<"; opt-out/pending-state contexts="<<contexts<<'\n';
    return profiles==72u&&spans>0u&&publicRoutes==12u&&ownerForms==4u&&refusals==16u&&quotas==12u&&allocations==32u&&failures==14u&&moves>0u&&contexts==4u;
}
