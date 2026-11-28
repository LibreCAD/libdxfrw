// SPDX-License-Identifier: GPL-2.0-or-later
// Reader-owned publication identity is separate from retained body equality,
// original physical namespace binding, owner targets and native semantics.
struct BlockOriginProbe final:dx_iface {
    using P=dxfRW::DxfBlockSourceView::Phase;
    bool enabled=true,publicOnly=false,good=true,forged=false,selected=true,disableOnBegin=false;dxfRW* externalInput=nullptr;
    int copy=-1,skip=-1,duplicate=-1,throwAt=-1;unsigned depth=0u;std::vector<P> phases;
    const DRW_Block* pendingModel=nullptr;const DRW_RawDxfObject* pendingRaw=nullptr;
    bool wantsDxfBlockSource() const override{return enabled&&selected&&(publicOnly||dx_iface::wantsDxfBlockSource());}
    bool wantsDxfBlockChildrenSource() const override{return enabled&&selected&&(publicOnly||dx_iface::wantsDxfBlockChildrenSource());}
    bool wantsDxfBlockDeliverySource() const override{return enabled&&selected&&(publicOnly||dx_iface::wantsDxfBlockDeliverySource());}
    void addHeader(const DRW_Header* value) override{if(!publicOnly)dx_iface::addHeader(value);}
    void addBlockRecord(const DRW_Block_Record& value) override{if(!publicOnly)dx_iface::addBlockRecord(value);}
    void inspect(P phase,const DRW_Block* block=nullptr,const DRW_RawDxfObject* source=nullptr,
        const DRW_DxfBlockChildrenSource* physical=nullptr,const DRW_DxfBlockDeliverySource* delivery=nullptr,
        const DRW_DxfBlockDeliveryEvent* event=nullptr,const DRW_Entity* entity=nullptr,const DRW_RawDxfObject* raw=nullptr) {
        const auto* input=publicOnly?externalInput:DxIfaceTableSourceTestAccess::blockReadInput(*cData);dxfRW::DxfBlockSourceView v;
        const auto saved=allocationRemaining;allocationRemaining=0;const bool active=input&&input->getDxfBlockSourceView(v);allocationRemaining=saved;
        if(enabled?(!active||v.phase!=phase||v.blockHandle!=0xB1u||v.blockRecord!=0xB0u
            ||(block&&v.block!=block)||(source&&v.source!=source)||(physical&&v.physical!=physical)||(delivery&&v.delivery!=delivery)
            ||(event&&v.event!=event)||(entity&&v.entity!=entity)||(raw&&v.raw!=raw)):active)good=false;
        if(active) {
            if(phase==P::BeginSource||phase==P::EndSource){pendingModel=v.block;pendingRaw=v.source;}
            if(phase==P::BeginTyped||phase==P::EndTyped){if(v.block!=pendingModel||v.source!=pendingRaw)good=false;pendingModel=nullptr;pendingRaw=nullptr;}
            if(phase>=P::Manifest&&phase<=P::Finish&&(!v.scope||!v.delivery||!v.physical))good=false;
        }
        if(!publicOnly&&blockOriginReport(*cData).state==BlockOriginState::OriginsBound)good=false;
        phases.push_back(phase);
    }
    template<class F>void call(P phase,F action) {
        const auto p=static_cast<int>(phase);if(skip!=p) {if(!publicOnly)action();if(duplicate==p&&!publicOnly)action();}
        if(throwAt==p)throw std::runtime_error("BLOCK origin publication fault");
    }
    void addDxfBlockBeginSource(const DRW_Block& b,const DRW_RawDxfObject& r) override {
        inspect(P::BeginSource,&b,&r);call(P::BeginSource,[&]{if(copy==1){auto other=r;dx_iface::addDxfBlockBeginSource(b,other);}else dx_iface::addDxfBlockBeginSource(b,r);});
        if(disableOnBegin)selected=false;
    }
    void addBlock(const DRW_Block& b) override {
        ++depth;inspect(P::BeginTyped,&b);call(P::BeginTyped,[&]{if(copy==2){auto other=b;dx_iface::addBlock(other);}else dx_iface::addBlock(b);});
    }
    void addDxfBlockEndSource(const DRW_Block& b,const DRW_RawDxfObject& r) override {
        inspect(P::EndSource,&b,&r);call(P::EndSource,[&]{if(copy==3){auto other=r;dx_iface::addDxfBlockEndSource(b,other);}else dx_iface::addDxfBlockEndSource(b,r);});
    }
    void addBlockEnd(const DRW_Block& b) override {
        inspect(P::EndTyped,&b);call(P::EndTyped,[&]{if(copy==4){auto other=b;dx_iface::addBlockEnd(other);}else dx_iface::addBlockEnd(b);});
    }
    void addDxfBlockChildrenSource(const DRW_DxfBlockChildrenSource& value) override {
        inspect(P::Children,nullptr,nullptr,&value);call(P::Children,[&]{if(copy==5||forged){auto other=value;
            if(forged){auto& record=other.records[0];for(std::size_t i=0u;i<record.groups.size();++i)if(record.groups[i].code()==10){record.groups[i]=DRW_Variant(10,7.25);if(!other.binary)record.rawValues[i]="7.25";}}
            dx_iface::addDxfBlockChildrenSource(other);}else dx_iface::addDxfBlockChildrenSource(value);});
    }
    void addDxfBlockDeliverySource(const DRW_DxfBlockDeliverySource& value) override {
        inspect(P::Manifest,nullptr,nullptr,nullptr,&value);call(P::Manifest,[&]{if(copy==6){auto other=value;dx_iface::addDxfBlockDeliverySource(other);}else dx_iface::addDxfBlockDeliverySource(value);});
    }
    void beginDxfBlockDelivery(std::uint64_t scope,std::uint32_t block,std::size_t i,const DRW_DxfBlockDeliveryEvent& value) override {
        inspect(P::CallbackBegin,nullptr,nullptr,nullptr,nullptr,&value);call(P::CallbackBegin,[&]{if(copy==7){auto other=value;dx_iface::beginDxfBlockDelivery(scope,block,i,other);}else dx_iface::beginDxfBlockDelivery(scope,block,i,value);});
    }
    void addPoint(const DRW_Point& value) override {
        inspect(P::CallbackValue,nullptr,nullptr,nullptr,nullptr,nullptr,&value);
        auto& actual=const_cast<DRW_Point&>(value);const auto x=actual.basePoint.x;if(forged)actual.basePoint.x=7.25;
        try{call(P::CallbackValue,[&]{if(copy==8){auto other=value;dx_iface::addPoint(other);}else dx_iface::addPoint(value);});}
        catch(...){actual.basePoint.x=x;throw;}actual.basePoint.x=x;
    }
    void addLine(const DRW_Line& value) override {
        inspect(P::CallbackValue,nullptr,nullptr,nullptr,nullptr,nullptr,&value);
        call(P::CallbackValue,[&]{if(copy==8){auto other=value;dx_iface::addLine(other);}else dx_iface::addLine(value);});
    }
    void addRawDxfEntity(const DRW_RawDxfObject& value) override {
        inspect(P::CallbackValue,nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,&value);
        call(P::CallbackValue,[&]{if(copy==8){auto other=value;dx_iface::addRawDxfEntity(other);}else dx_iface::addRawDxfEntity(value);});
    }
    void addDxfAttdefSource(const DRW_Attdef& value,const DRW_RawDxfObject& raw,const DRW_DxfAttdefSourceContext& context) override {
        inspect(P::CallbackValue,nullptr,nullptr,nullptr,nullptr,nullptr,&value,&raw);
        call(P::CallbackValue,[&]{if(copy==8){auto other=raw;dx_iface::addDxfAttdefSource(value,other,context);}else dx_iface::addDxfAttdefSource(value,raw,context);});
    }
    void addAttDef(const DRW_Attdef& value) override {
        inspect(P::CallbackValue,nullptr,nullptr,nullptr,nullptr,nullptr,&value);
        call(P::CallbackValue,[&]{if(copy==8){auto other=value;dx_iface::addAttDef(other);}else dx_iface::addAttDef(value);});
    }
    void addPolyline(const DRW_Polyline& value) override {
        inspect(P::CallbackValue,nullptr,nullptr,nullptr,nullptr,nullptr,&value);
        call(P::CallbackValue,[&]{if(copy==8){auto other=value;dx_iface::addPolyline(other);}else dx_iface::addPolyline(value);});
    }
    void addInsert(const DRW_Insert& value) override {
        inspect(P::CallbackValue,nullptr,nullptr,nullptr,nullptr,nullptr,&value);
        call(P::CallbackValue,[&]{if(copy==8){auto other=value;dx_iface::addInsert(other);}else dx_iface::addInsert(value);});
    }
    void endDxfBlockDelivery(std::uint64_t scope,std::uint32_t block,std::size_t i,const DRW_DxfBlockDeliveryEvent& value) override {
        inspect(P::CallbackEnd,nullptr,nullptr,nullptr,nullptr,&value);call(P::CallbackEnd,[&]{if(copy==9){auto other=value;dx_iface::endDxfBlockDelivery(scope,block,i,other);}else dx_iface::endDxfBlockDelivery(scope,block,i,value);});
    }
    void finishDxfBlockDelivery(std::uint64_t scope,std::uint32_t block) override {
        inspect(P::Finish);call(P::Finish,[&]{dx_iface::finishDxfBlockDelivery(scope,block);});
    }
    void endBlock() override {
        if(depth)--depth;else good=false;inspect(P::Close);pendingModel=nullptr;pendingRaw=nullptr;
        call(P::Close,[&]{dx_iface::endBlock();});
    }
};

bool blockOriginLifetime(const std::filesystem::path& path,unsigned& cases) {
    const auto f=blockPrimaryFixture("AC1032",false);std::ostringstream wire;for(const auto& t:f.document)wire<<t.code<<'\n'<<t.value<<'\n';
    dxfRW input(path.string().c_str());
    for(int phase=0;phase<=11;++phase){BlockOriginProbe probe;probe.publicOnly=true;probe.externalInput=&input;probe.throwAt=phase?phase:-1;
        dxfRW::DxfBlockSourceView v;v.blockHandle=1u;if(input.getDxfBlockSourceView(v)||v.blockHandle||v.block||v.source)return false;
        auto content=wire.str();bool threw=false,read=false;try{read=input.readAscii(&probe,false,content);}catch(const std::runtime_error&){threw=true;}
        if(threw!=(phase!=0)||read!=(phase==0)||!probe.good||probe.depth||input.getDxfBlockSourceView(v)||v.block||v.source||v.physical||v.delivery||v.event||v.entity||v.raw)return false;++cases;
    }
    return true;
}
bool blockOriginTests(const std::filesystem::path& path) {
    struct Stage{const char* name="profiles";bool done=false;~Stage(){if(!done)std::cerr<<"BLOCK origins stage="<<name<<'\n';}} stage;
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::BlockOriginState;using P=BlockOriginProbe::P;
    unsigned profiles=0u,compoundProfiles=0u,origins=0u,copies=0u,refusals=0u,contexts=0u,quotas=0u,exceptions=0u,variants=0u,lifetimes=0u;long allocations=0;
    const auto complete=[](const dx_data& d){const auto r=dx_iface::blockOriginReport(d);return r.state==S::OriginsBound&&r.blocks==1u&&r.publications==1u&&r.bound==1u&&!r.ownerTargetsResolved&&!r.outputEligible;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0;encoding<3u;++encoding)for(bool empty:{false,true}) {
        const auto f=empty?blockFixture(version,0u,true):blockPrimaryFixture(version,false);if(!write(path,f.document,encoding==2u,encoding==1u))return false;BlockOriginProbe reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||!complete(data)||!reader.good||reader.depth){const auto r=dx_iface::blockOriginReport(data);std::cerr<<version<<'/'<<encoding<<" empty="<<empty<<" state="<<static_cast<int>(r.state)<<" flags="<<(A::block(data)?A::block(data)->originPhases:0u)<<'\n';return false;}
        const std::vector<P> expected=empty?std::vector<P>{P::BeginSource,P::BeginTyped,P::EndSource,P::EndTyped,P::Children,P::Manifest,P::Finish,P::Close}:
            std::vector<P>{P::BeginSource,P::BeginTyped,P::EndSource,P::EndTyped,P::Children,P::Manifest,P::CallbackBegin,P::CallbackValue,P::CallbackEnd,P::CallbackBegin,P::CallbackValue,P::CallbackEnd,P::Finish,P::Close};
        if(reader.phases!=expected||dx_iface::blockOriginReport(data).events!=(empty?0u:2u))return false;
        const auto& audit=*A::blockOrigins(data).at(0xB1u);const auto& source=*audit.identity->source;const auto wire=attdefWireTags(path,encoding==2u);
        for(std::size_t i=audit.first;i<=audit.last;++i){const auto& r=source.records[i];const auto& w=wire[r.marker.ordinal-1u];
            if(w.text!=r.name||w.begin!=r.marker.beginOffset||w.end!=r.marker.endOffset)return false;++origins;}
        ++profiles;
    }
    stage.name="ATTDEF source pairs and compound subtype publications";
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0;encoding<3u;++encoding)for(unsigned form=0u;form<7u;++form) {
        auto f=compoundFixture(version,form);f.document.erase(std::remove_if(f.document.begin(),f.document.end(),[](const Tag& t){return t.code==999;}),f.document.end());
        if(!write(path,f.document,encoding==2u,encoding==1u))return false;BlockOriginProbe reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||!complete(data)||!reader.good||reader.depth){std::cerr<<"compound origin="<<version<<'/'<<encoding<<'/'<<form<<" state="<<static_cast<int>(dx_iface::blockOriginReport(data).state)<<'\n';return false;}
        ++compoundProfiles;
    }
    for(bool binary:{false,true}) {
        const auto f=blockPrimaryFixture("AC1032",false);if(!write(path,f.document,binary))return false;
        stage.name="equal copied publication/model/event arguments";
        for(int phase=1;phase<=9;++phase){BlockOriginProbe reader;reader.copy=phase;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!reader.good
            ||dx_iface::blockOriginReport(data).state!=S::Unqualified||!A::compoundMatched(data,0u)||!A::compoundMatched(data,1u)){std::cerr<<"copy phase="<<phase<<'\n';return false;}
            reader.copy=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++copies;}
        stage.name="missing/duplicate phases and consistent source/model forgery";
        for(bool duplicate:{false,true})for(int phase=1;phase<=(duplicate?10:11);++phase){BlockOriginProbe reader;if(duplicate)reader.duplicate=phase;else reader.skip=phase;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::blockOriginReport(data).state==S::OriginsBound){std::cerr<<"protocol="<<duplicate<<'/'<<phase<<'\n';return false;}
            reader.duplicate=reader.skip=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry)){std::cerr<<"protocol retry="<<duplicate<<'/'<<phase<<'\n';return false;}++refusals;}
        {BlockOriginProbe reader;reader.forged=true;dx_data data;const bool imported=reader.fileImport(path.string(),&data,false);
            if(!imported||!A::compoundSourceMatched(data,0u)||dx_iface::blockOriginReport(data).state!=S::Unqualified){std::cerr<<"forged import="<<imported<<" body="<<A::compoundMatched(data,0u)<<" source="<<A::compoundSourceMatched(data,0u)<<" origin="<<static_cast<int>(dx_iface::blockOriginReport(data).state)<<'\n';return false;}
            const auto* point=dynamic_cast<const DRW_Point*>(A::compoundBody(data,0u)->model.get());if(!point||point->basePoint.x!=7.25){std::cerr<<"forged point="<<(point?point->basePoint.x:0.0)<<'\n';return false;}
            reader.forged=false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++refusals;}
        stage.name="raw callback and immutable physical views";
        for(bool copied:{false,true}){auto raw=f;const auto at=std::find_if(raw.document.begin(),raw.document.end(),[](const Tag& t){return t.code==0&&t.value=="ENDBLK";});
            raw.document.insert(at,{{0,"VENDOR_ENTITY"},{5,"C3"},{330,"B0"},{100,"AcDbEntity"},{8,"RAW"},{100,"VendorData"},{1,"opaque"}});
            if(!write(path,raw.document,binary))return false;BlockOriginProbe reader;if(copied)reader.copy=8;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||!reader.good||complete(data)==copied||dx_iface::blockOriginReport(data).events!=(copied?0u:3u))return false;++variants;}
        if(!write(path,f.document,binary))return false;BlockOriginProbe baseline;dx_data base;if(!baseline.fileImport(path.string(),&base,false)||!complete(base))return false;
        stage.name="quota and owner controls";
        for(bool work:{false,true})for(bool exact:{false,true}){const auto r=dx_iface::blockOriginReport(base);dx_data data;A::blockOriginLimit(data,work,(work?r.work:r.bytes)-(exact?0u:1u));BlockOriginProbe reader;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::blockOriginReport(data).state!=(exact?S::OriginsBound:S::CaptureFailed))return false;++quotas;}
        for(unsigned kind=0u;kind<5u;++kind){BlockOriginProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;
            if(kind<2u)A::substituteBlockOrigin(data,kind==1u);else if(kind<4u)A::substituteBlockContext(data,kind==3u);
            else{dx_data copied;A::copyMembershipContext(copied,data);if(dx_iface::blockOriginReport(copied).state!=S::ContextMismatch)return false;++contexts;continue;}
            if(dx_iface::blockOriginReport(data).state!=S::ContextMismatch)return false;++contexts;}
        for(unsigned kind=0u;kind<7u;++kind)for(bool alias:{false,true}){BlockOriginProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;
            A::substituteBlockOriginInput(data,kind,alias);if(dx_iface::blockOriginReport(data).state!=S::ContextMismatch)return false;++contexts;}
        {BlockOriginProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;A::transplantBlockOrigin(data,base);
            if(dx_iface::blockOriginReport(data).state!=S::ContextMismatch)return false;++contexts;}
        for(auto* block:base.blocks)for(auto* entity:block->ent)entity->layer="edited live layer";if(!complete(base))return false;++contexts;
        stage.name="isolated audit allocation recovery";const auto before=A::beforeBlockOriginAudit(base);bool recovered=false;
        for(long fault=0;fault<100;++fault){allocationRemaining=fault;A::repeatBlockOrigins(baseline,base,before);allocationRemaining=-1;
            if(complete(base)){recovered=true;break;}if(dx_iface::blockOriginReport(base).state!=S::CaptureFailed)return false;
            A::repeatBlockOrigins(baseline,base,before);if(!complete(base))return false;BlockOriginProbe reader;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++allocations;}
        if(!recovered)return false;
        stage.name="callback exception and failed tail recovery";
        for(int phase=1;phase<=12;++phase){auto varied=f;if(phase==12)varied.document.pop_back();if(!write(path,varied.document,binary))return false;BlockOriginProbe reader;reader.throwAt=phase<12?phase:-1;dx_data data;bool threw=false,read=false;
            try{read=reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
            if(read||threw!=(phase<12)||reader.depth||dx_iface::blockOriginReport(data).state!=S::ImportFailed)return false;
            reader.throwAt=-1;if(!write(path,f.document,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++exceptions;}
        {BlockOriginProbe reader;reader.enabled=false;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!reader.good||dx_iface::blockOriginReport(data).state!=S::NotCaptured)return false;++contexts;}
        {BlockOriginProbe reader;reader.disableOnBegin=true;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!reader.good||!complete(data))return false;++contexts;}
        stage.name="two original block scopes in one namespace";
        {auto multi=f;const auto begin=std::find_if(multi.document.begin(),multi.document.end(),[](const Tag& t){return t.code==0&&t.value=="BLOCK";});
            const auto end=std::find_if(begin,multi.document.end(),[](const Tag& t){return t.code==0&&t.value=="ENDSEC";});Tags extra(begin,end);
            const std::map<std::string,std::string> names{{"B0","F0"},{"B1","F1"},{"B2","F2"},{"C1","F3"},{"C2","F4"}};
            for(auto& t:extra){if(t.code==5||t.code==330){const auto at=names.find(t.value);if(at!=names.end())t.value=at->second;}if((t.code==2||t.code==3)&&t.value=="DECLARED")t.value="SECOND_DECLARED";}
            multi.document.insert(end,extra.begin(),extra.end());const auto tableEnd=std::find_if(multi.document.begin(),multi.document.end(),[](const Tag& t){return t.code==0&&t.value=="ENDTAB";});
            const Tags record{{0,"BLOCK_RECORD"},{5,"F0"},{330,"B9"},{100,"AcDbSymbolTableRecord"},{100,"AcDbBlockTableRecord"},{2,"SECOND"},{70,"0"}};
            multi.document.insert(tableEnd,record.begin(),record.end());
            const auto tableHandle=std::find_if(multi.document.begin(),multi.document.end(),[](const Tag& t){return t.code==5&&t.value=="B9";});
            const auto tableCount=std::find_if(tableHandle,multi.document.end(),[](const Tag& t){return t.code==70;});if(tableCount==multi.document.end())return false;tableCount->value="2";
            if(!write(path,multi.document,binary))return false;dx_iface reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false))return false;const auto r=dx_iface::blockOriginReport(data);
            if(r.state!=S::OriginsBound||r.blocks!=2u||r.publications!=2u||r.bound!=2u||r.events!=4u)return false;
            const auto& a=*A::blockOrigins(data).at(0xB1u);const auto& b=*A::blockOrigins(data).at(0xF1u);
            if(a.first>=a.last||a.last>=b.first||b.first>=b.last||a.delivery->scopeId==b.delivery->scopeId||a.children==b.children||a.context==b.context)return false;++variants;}
    }
    stage.name="public borrowed-view lifetime and unwind";if(!blockOriginLifetime(path,lifetimes))return false;
    std::cout<<"BLOCK origins profiles="<<profiles<<" compound profiles="<<compoundProfiles<<" independent markers="<<origins<<" copied refusals="<<copies<<" protocol/forgery refusals="<<refusals<<" contexts="<<contexts
        <<" quotas="<<quotas<<" allocation faults="<<allocations<<" exceptions/tails="<<exceptions<<" raw variants="<<variants<<" public lifetimes="<<lifetimes<<'\n';stage.done=true;return true;
}
