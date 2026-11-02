// SPDX-License-Identifier: GPL-2.0-or-later
struct EntitiesOriginProbe final:dx_iface {
    bool enabled=true,mutateAfter=false,premature=false,dropEnd=false,richerPoint=false,richerPolyline=false,richerInsert=false;
    long fault=-1,hookFault=-1;int throwPhase=0;unsigned calls=0u;
    std::function<void(DRW_DxfEntitiesDeliverySource&)> alter;
    std::function<void(DRW_DxfEntitiesDeliveryEvent&)> alterBegin,alterEnd;
    std::function<void(DRW_RawDxfObject&)> alterRaw;
    void addRawDxfEntity(const DRW_RawDxfObject& value) override {
        if(alterRaw){auto copy=value;alterRaw(copy);dx_iface::addRawDxfEntity(copy);}else dx_iface::addRawDxfEntity(value);
    }
    void addPoint(const DRW_Point& value) override {
        if(richerPoint&&value.handle==0x101u) {
            struct Rich final:DRW_Point {explicit Rich(const DRW_Point& v):DRW_Point(v){}std::string custom="unqualified";};
            Rich input(value);dx_iface::addPoint(input);
        }else dx_iface::addPoint(value);
    }
    void addPolyline(const DRW_Polyline& value) override {
        if(richerPolyline) {
            struct Rich final:DRW_Vertex {explicit Rich(const DRW_Vertex& v):DRW_Vertex(v){}std::string custom="unqualified";};
            auto copy=value;copy.vertlist[0]=std::make_shared<Rich>(*copy.vertlist[0]);dx_iface::addPolyline(copy);
        }else dx_iface::addPolyline(value);
    }
    void addInsert(const DRW_Insert& value) override {
        if(richerInsert&&value.handle==0xE100u) {
            struct Rich final:DRW_Attrib {explicit Rich(const DRW_Attrib& v):DRW_Attrib(v){}std::string custom="unqualified";};
            auto copy=value;copy.attlist[0]=std::make_shared<Rich>(*copy.attlist[0]);dx_iface::addInsert(copy);
        }else dx_iface::addInsert(value);
    }
    bool wantsDxfEntitiesDeliverySource() const override {return enabled&&dx_iface::wantsDxfEntitiesDeliverySource();}
    void beginDxfEntitiesDelivery(std::uint64_t scope,std::size_t index,const DRW_DxfEntitiesDeliveryEvent& e) override {
        if(throwPhase==1)throw std::runtime_error("origin begin");
        auto copy=e;if(alterBegin)alterBegin(copy);SymbolAllocationFailure schedule(hookFault);dx_iface::beginDxfEntitiesDelivery(scope,index,copy);
    }
    void endDxfEntitiesDelivery(std::uint64_t scope,std::size_t index,const DRW_DxfEntitiesDeliveryEvent& e) override {
        if(throwPhase==2)throw std::runtime_error("origin end");
        if(dropEnd)return;auto copy=e;if(alterEnd)alterEnd(copy);dx_iface::endDxfEntitiesDelivery(scope,index,copy);
    }
    void addDxfEntitiesDeliverySource(const DRW_DxfEntitiesDeliverySource& value) override {
        ++calls;if(entitiesDeliveryReport(*cData).state==EntitiesDeliveryState::ObservedTransportComplete)premature=true;
        if(throwPhase==3)throw std::runtime_error("origin before final");
        if(alter){auto copy=value;alter(copy);dx_iface::addDxfEntitiesDeliverySource(copy);}
        else {SymbolAllocationFailure schedule(fault);dx_iface::addDxfEntitiesDeliverySource(value);}
        if(mutateAfter&&!value.events.empty())const_cast<DRW_DxfEntitiesDeliverySource&>(value).events[0].handle=0xFEEDu;
        if(entitiesDeliveryReport(*cData).state==EntitiesDeliveryState::ObservedTransportComplete)premature=true;
        if(throwPhase==4)throw std::runtime_error("origin after final");
    }
};
bool entitiesOriginWire(const std::filesystem::path& path,bool binary,const dx_data& data,std::size_t& joins) {
    const auto wire=attdefWireTags(path,binary);
    for(const auto& row:DxIfaceTableSourceTestAccess::entitiesDeliveries(data)) {
        const auto& source=*row.delivery;const auto& p=*row.physical;std::size_t record=0u,event=0u;
        if(source.scopeId!=p.scopeId||source.events.size()!=row.observed->size())return false;
        for(std::size_t i=0;i<source.dispatches.size();++i) {
            const auto& d=source.dispatches[i];const auto begin=d.begin.ordinal-1u,end=d.end.ordinal-1u;
            if(begin>=wire.size()||end>=wire.size()||wire[begin].code!=0||wire[end].code!=0
                ||wire[begin].begin!=d.begin.beginOffset||wire[begin].end!=d.begin.endOffset
                ||wire[end].begin!=d.end.beginOffset||wire[end].end!=d.end.endOffset||d.firstEvent!=event)return false;
            if(record>=p.records.size()||p.records[record].marker.ordinal!=d.begin.ordinal)return false;
            while(record<p.records.size()&&p.records[record].marker.ordinal<d.end.ordinal)++record;
            for(std::size_t j=0;j<d.events;++j,++event) {
                if(event>=source.events.size())return false;const auto& e=source.events[event];const auto& observed=(*row.observed)[event];
                if(e.dispatch!=i||e.callback!=observed.event.callback||e.handle!=observed.event.handle||e.owner!=observed.event.owner)return false;
            }
            ++joins;
        }
        if(record!=p.records.size()||event!=source.events.size())return false;
    }
    return true;
}
bool entitiesOriginTests(const std::filesystem::path& dir) {
    struct Stage final {const char* name="source matrix";bool finished=false;~Stage(){if(!finished)std::cerr<<"ENTITIES origin stage="<<name<<'\n';}} stage;
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::EntitiesDeliveryState;using B=dx_iface::TableBindingState;
    const auto path=dir/"entities-origins.dxf";unsigned routes=0u,faults=0u,limits=0u,contexts=0u,exceptions=0u,aliases=0u,tableOrigins=0u,hookFaults=0u,aborts=0u,missing=0u,dispositions=0u,teardowns=0u,rawFaults=0u,dormant=0u,emptyOrNoCallback=0u;
    long allocations=0;std::size_t joins=0u;
    const auto complete=[](const dx_data& d){const auto r=dx_iface::entitiesDeliveryReport(d);return r.state==S::ObservedTransportComplete&&!r.outputEligible;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0;encoding<3u;++encoding) {
        if(!writeBlockBinding(path,entitiesSourceDocument(version),encoding))return false;EntitiesOriginProbe reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||!entitiesOriginWire(path,encoding==2u,data,joins))return false;
        const auto r=dx_iface::entitiesDeliveryReport(data);
        if(r.state!=S::ObservedTransportComplete||reader.premature||r.sections!=2u||r.dispatches!=7u||r.events!=6u
            ||r.valuesObserved!=6u||r.models!=5u||r.rawBodies!=1u||r.outputEligible) {
            std::cerr<<"origin state="<<static_cast<int>(r.state)<<" sections="<<r.sections<<" dispatch="<<r.dispatches<<" events="<<r.events
                <<" values="<<r.valuesObserved<<" models="<<r.models<<" raw="<<r.rawBodies<<" pending="<<A::entitiesPending(data).scope<<'\n';return false;
        }
        const auto& rows=A::entitiesDeliveries(data);
        const auto* point=dynamic_cast<const DRW_Point*>((*rows[0].observed)[0].model.get());
        if(!point||point->basePoint.x!=0.||!std::signbit(point->basePoint.x)||point->basePoint.y!=1.25
            ||(*rows[0].observed)[4].raw->groups.empty())return false;
        ++routes;
    }
    const std::vector<std::function<void(DRW_DxfEntitiesDeliverySource&)>> invalid{
        [](auto& v){v.status=DRW_DxfEntitiesDeliverySource::Status::Collecting;},[](auto& v){++v.scopeId;},
        [](auto& v){v.version=DRW::AC1015;},[](auto& v){v.binary=!v.binary;},
        [](auto& v){++v.beginMarker.ordinal;},[](auto& v){++v.beginMarker.beginOffset;},[](auto& v){v.endMarker.ignoredComment=true;},
        [](auto& v){++v.endMarker.endOffset;},[](auto& v){--v.completedEvents;},
        [](auto& v){v.dispatches.pop_back();},[](auto& v){v.events.pop_back();},
        [](auto& v){++v.dispatches[0].firstEvent;},[](auto& v){++v.dispatches[0].events;},
        [](auto& v){v.dispatches[1].begin=v.dispatches[0].begin;},[](auto& v){v.dispatches[0].end=v.dispatches[1].end;},
        [](auto& v){++v.events[0].dispatch;},[](auto& v){v.events[0].callback=DRW_DxfEntitiesCallback::addCircle;},
        [](auto& v){v.events[0].origin=DRW_DxfEntitiesDeliveryEvent::Origin::ProxyRendering;},
        [](auto& v){++v.events[0].entityType;},[](auto& v){++v.events[0].handle;},[](auto& v){++v.events[0].owner;},
        [](auto& v){v.events[0].hasValue=false;}
    };
    for(bool binary:{false,true}) {
        const auto tags=entitiesSourceDocument("AC1032");if(!write(path,tags,binary))return false;
        stage.name="malformed final reports";
        for(const auto& change:invalid) {
            EntitiesOriginProbe reader;reader.alter=[&](auto& v){if(!v.events.empty())change(v);};dx_data failed;
            if(!reader.fileImport(path.string(),&failed,false)||dx_iface::entitiesDeliveryReport(failed).state!=S::CaptureFailed
                ||dx_iface::entitiesSourceReport(failed).state!=dx_iface::EntitiesSourceState::TransportComplete)return false;
            reader.alter={};dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++faults;
        }
        EntitiesOriginProbe baseline;dx_data base;if(!baseline.fileImport(path.string(),&base,false)||!complete(base))return false;
        const auto measured=dx_iface::entitiesDeliveryReport(base);
        stage.name="forged raw carriers";
        const std::vector<std::function<void(DRW_RawDxfObject&)>> rawInvalid{
            [](auto& raw){raw.m_version=DRW::AC1015;},
            [](auto& raw){raw.groups[0].addCoord(5,DRW_Coord(1,2,3));},
            [](auto& raw){for(auto& field:raw.groups)if(field.code()==1003)field.setDwgRawLayerReference(1u,DRW::AC1032);},
            [](auto& raw){raw.groups[0].addInt(0,1);},
            [](auto& raw){raw.rawValues.pop_back();}
        };
        unsigned rawKind=0u;
        for(const auto& change:rawInvalid) {
            auto input=tags;if(rawKind==2u)appendBlockFields(input,"F100",{{1001,"APP"},{1003,"1"}});
            if(!write(path,input,binary))return false;
            EntitiesOriginProbe reader;reader.alterRaw=change;dx_data failed;
            const bool imported=reader.fileImport(path.string(),&failed,false);
            if(!imported||dx_iface::entitiesDeliveryReport(failed).state!=S::CaptureFailed) {
                std::cerr<<"forged raw kind="<<rawKind<<" imported="<<imported<<" state="<<static_cast<int>(dx_iface::entitiesDeliveryReport(failed).state)<<'\n';return false;
            }
            reader.alterRaw={};dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++rawFaults;++rawKind;
        }
        if(!write(path,tags,binary))return false;
        stage.name="dormant raw owner boundaries";
        {
            const auto& original=*(*A::entitiesDeliveries(base)[0].observed)[4].raw;
            auto at=std::find_if(original.groups.begin(),original.groups.end(),[](const auto& field){return field.code()==160;});
            if(at==original.groups.end())return false;const auto index=static_cast<std::size_t>(at-original.groups.begin());
            DRW_Variant padded(160,std::string(1024u,'x'));padded.addBinary(160,std::vector<std::uint8_t>(2048u,0xA5u));padded.addInt64(160,at->i64_val());
            std::size_t before=0u,after=0u;if(!at->estimateRetainedPayloadBytes(65536u,before)||!padded.estimateRetainedPayloadBytes(65536u,after)||after<=before)return false;
            for(bool below:{false,true}) {
                EntitiesOriginProbe reader;reader.alterRaw=[&](auto& raw){raw.groups[index]=padded;};dx_data data;
                A::entitiesDeliveryLimit(data,3u,measured.bytes+2u*(after-before)-(below?1u:0u));
                if(!reader.fileImport(path.string(),&data,false)||dx_iface::entitiesDeliveryReport(data).state!=(below?S::CaptureFailed:S::ObservedTransportComplete)) {
                    const auto r=dx_iface::entitiesDeliveryReport(data);std::cerr<<"dormant below="<<below<<" state="<<static_cast<int>(r.state)<<" charged="<<r.bytes<<" expected="<<measured.bytes+2u*(after-before)<<'\n';return false;
                }++dormant;
            }
        }
        const std::array<std::size_t,5> cap{{measured.sections,measured.dispatches,measured.events,measured.bytes,measured.work}};
        stage.name="aggregate budgets";
        for(unsigned which=0;which<cap.size();++which)for(bool below:{false,true}) {
            EntitiesOriginProbe reader;dx_data data;A::entitiesDeliveryLimit(data,which,cap[which]-(below?1u:0u));
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::entitiesDeliveryReport(data).state!=(below?S::CaptureFailed:S::ObservedTransportComplete)) {
                std::cerr<<"origin limit="<<which<<'/'<<below<<'\n';return false;
            }
            ++limits;
        }
        stage.name="allocation faults";
        for(bool hook:{false,true}) {
            bool calibrated=false;
            for(long failure=0;failure<100&&!calibrated;++failure) {
                EntitiesOriginProbe reader;if(hook)reader.hookFault=failure;else reader.fault=failure;dx_data data;
                if(!reader.fileImport(path.string(),&data,false))return false;
                if(complete(data))calibrated=true;
                else {
                    if(dx_iface::entitiesDeliveryReport(data).state!=S::CaptureFailed)return false;
                    if(hook)++hookFaults;else ++allocations;
                    reader.hookFault=reader.fault=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;
                }
            }
            if(!calibrated)return false;
        }
        stage.name="callback exceptions";
        for(int phase=1;phase<=4;++phase) {
            EntitiesOriginProbe reader;reader.throwPhase=phase;dx_data data;bool threw=false;
            try{if(reader.fileImport(path.string(),&data,false))return false;}catch(const std::runtime_error&){threw=true;}
            if(!threw||dx_iface::entitiesDeliveryReport(data).state!=S::ImportFailed)return false;
            reader.throwPhase=0;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++exceptions;
        }
        stage.name="missing hook controls";
        for(unsigned kind=0;kind<4u;++kind) {
            EntitiesOriginProbe reader;
            if(kind==0u)reader.enabled=false;
            if(kind==1u)reader.dropEnd=true;
            if(kind==2u)reader.alterBegin=[](auto& e){++e.handle;};
            if(kind==3u)reader.alterEnd=[](auto& e){e.origin=DRW_DxfEntitiesDeliveryEvent::Origin::ProxyRendering;};
            dx_data data;if(!reader.fileImport(path.string(),&data,false)||dx_iface::entitiesDeliveryReport(data).state!=(kind?S::CaptureFailed:S::NotCaptured))return false;++missing;
        }
        stage.name="late contexts";
        for(unsigned kind=0;kind<3u;++kind) {
            EntitiesOriginProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!complete(data))return false;
            const auto& delivery=*A::entitiesDeliveries(data)[0].delivery;
            if(kind==0u)reader.addDxfEntitiesDeliverySource(delivery);
            else if(kind==1u)reader.beginDxfEntitiesDelivery(delivery.scopeId,0u,delivery.events[0]);
            else reader.endDxfEntitiesDelivery(delivery.scopeId,0u,delivery.events[0]);
            if(dx_iface::entitiesDeliveryReport(data).state!=S::CaptureFailed)return false;
            dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++contexts;
        }
        stage.name="copied and substituted owners";
        {
            dx_data copied;A::copyBindingContext(copied,base);
            if(dx_iface::entitiesDeliveryReport(copied).state!=S::ContextMismatch)return false;++contexts;
            auto& row=A::mutableEntitiesDeliveries(base)[0];
            const auto physical=row.physical;row.physical=std::make_shared<const DRW_DxfEntitiesSource>(*physical);
            if(dx_iface::entitiesDeliveryReport(base).state!=S::ContextMismatch)return false;row.physical=physical;if(!complete(base))return false;++contexts;
            const auto delivery=row.delivery;row.delivery=std::make_shared<const DRW_DxfEntitiesDeliverySource>(*delivery);
            if(dx_iface::entitiesDeliveryReport(base).state!=S::ContextMismatch)return false;row.delivery=delivery;if(!complete(base))return false;++contexts;
            const auto observed=row.observed;row.observed=std::make_shared<const std::decay_t<decltype(*observed)>>(*observed);
            if(dx_iface::entitiesDeliveryReport(base).state!=S::ContextMismatch)return false;row.observed=observed;if(!complete(base))return false;++contexts;
            row.physical=std::shared_ptr<const DRW_DxfEntitiesSource>(physical.get(),[](const auto*){});
            if(dx_iface::entitiesDeliveryReport(base).state!=S::ContextMismatch)return false;row.physical=physical;if(!complete(base))return false;++contexts;
            row.delivery=std::shared_ptr<const DRW_DxfEntitiesDeliverySource>(delivery.get(),[](const auto*){});
            if(dx_iface::entitiesDeliveryReport(base).state!=S::ContextMismatch)return false;row.delivery=delivery;if(!complete(base))return false;++contexts;
            row.observed=std::shared_ptr<const std::decay_t<decltype(*observed)>>(observed.get(),[](const auto*){});
            if(dx_iface::entitiesDeliveryReport(base).state!=S::ContextMismatch)return false;row.observed=observed;if(!complete(base))return false;++contexts;
            row.container=nullptr;if(dx_iface::entitiesDeliveryReport(base).state!=S::ContextMismatch)return false;row.container=&base;if(!complete(base))return false;++contexts;
            ++row.generation;if(dx_iface::entitiesDeliveryReport(base).state!=S::ContextMismatch)return false;--row.generation;if(!complete(base))return false;++contexts;
            dx_data appended;EntitiesOriginProbe reader;if(!reader.fileImport(path.string(),&appended,false)||!reader.fileImport(path.string(),&appended,false)
                ||dx_iface::entitiesDeliveryReport(appended).state!=S::ContextMismatch)return false;
            ++contexts;
            reader.addDxfEntitiesDeliverySource(*delivery);if(dx_iface::entitiesDeliveryReport(appended).state!=S::ContextMismatch)return false;++contexts;
        }
        {
            alignas(dx_data) unsigned char storage[sizeof(dx_data)];
            const auto destroy=[](dx_data* value){value->~dx_data();};dx_data backup;std::uint64_t firstId=0u;
            {
                std::unique_ptr<dx_data,decltype(destroy)> source(new(storage)dx_data,destroy);EntitiesOriginProbe reader;
                if(!reader.fileImport(path.string(),source.get(),false)||!complete(*source)||!A::entitiesIdentityCopyIsIndependent(*source))return false;
                firstId=A::entitiesContainerId(*source);A::copyBindingContext(backup,*source);
            }
            std::unique_ptr<dx_data,decltype(destroy)> reused(new(storage)dx_data,destroy);A::copyBindingContext(*reused,backup);
            const auto physicalState=dx_iface::entitiesSourceReport(*reused).state;
            if(!firstId||firstId==A::entitiesContainerId(*reused)||physicalState!=dx_iface::EntitiesSourceState::ContextMismatch
                ||dx_iface::entitiesDeliveryReport(*reused).state!=S::ContextMismatch) {
                std::cerr<<"reused container address: physical state="<<static_cast<int>(physicalState)<<" expected ContextMismatch\n";return false;
            }
            ++contexts;
        }
        stage.name="immutable live edits";
        {
            EntitiesOriginProbe reader;reader.mutateAfter=true;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!complete(data))return false;
            const auto& row=A::entitiesDeliveries(data)[0];const auto& observed=*row.observed;
            if(row.delivery->events[0].handle!=0x101u)return false;
            auto* point=dynamic_cast<DRW_Point*>(data.mBlock->ent.front());const auto* frozen=dynamic_cast<const DRW_Point*>(observed[0].model.get());
            if(!point||!frozen)return false;point->basePoint.y=99.;if(frozen->basePoint.y!=1.25)return false;
            DRW_Polyline* poly=nullptr;DRW_Insert* insert=nullptr;
            for(auto* e:data.mBlock->ent){if(e->eType==DRW::POLYLINE)poly=dynamic_cast<DRW_Polyline*>(e);if(e->eType==DRW::INSERT)insert=dynamic_cast<DRW_Insert*>(e);}
            const auto* frozenPoly=dynamic_cast<const DRW_Polyline*>(observed[2].model.get());const auto* frozenInsert=dynamic_cast<const DRW_Insert*>(observed[3].model.get());
            if(!poly||!insert||!frozenPoly||!frozenInsert)return false;
            poly->vertlist.front()->basePoint.x=99.;insert->attlist.front()->text="edited";
            data.mBlock->rawEntities[0].record.name="changed";
            if(frozenPoly->vertlist.front()->basePoint.x!=1.||frozenInsert->attlist.front()->text!="attribute-é"
                ||observed[4].raw->name!="VENDOR_ENTITY"||!complete(data))return false;
            ++aliases;
        }
        stage.name="unqualified body dispositions";
        {
            for(unsigned shape=0;shape<3u;++shape) {
                EntitiesOriginProbe reader;reader.richerPoint=shape==0u;reader.richerPolyline=shape==1u;reader.richerInsert=shape==2u;dx_data data;
                if(!reader.fileImport(path.string(),&data,false)||!complete(data))return false;
                const auto& value=(*A::entitiesDeliveries(data)[0].observed)[shape==0u?0u:shape==1u?2u:3u];
                if(!value.valueObserved||!value.modelUnavailable||value.model||dx_iface::entitiesDeliveryReport(data).models!=4u)return false;++dispositions;
            }
            const Tags ignored{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1032"},{0,"ENDSEC"},{0,"SECTION"},{2,"ENTITIES"},
                {0,"CAMERA"},{5,"101"},{330,"1F"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbCamera"},{340,"0"},{0,"ENDSEC"},{0,"EOF"}};
            if(!write(path,ignored,binary))return false;dx_iface compatible;dx_data noBody;
            if(!compatible.fileImport(path.string(),&noBody,false)||!complete(noBody))return false;
            const auto r=dx_iface::entitiesDeliveryReport(noBody);
            if(r.events!=1u||r.valuesObserved||r.models||r.rawBodies||r.outputEligible)return false;++dispositions;
            if(!write(path,tags,binary))return false;
        }
        for(bool noCallback:{false,true}) {
            Tags input{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1032"},{0,"ENDSEC"},{0,"SECTION"},{2,"ENTITIES"}};
            if(noCallback)input.insert(input.end(),{{0,"DIMENSION"},{5,"101"},{330,"1F"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbDimension"},{70,"7"}});
            input.insert(input.end(),{{0,"ENDSEC"},{0,"EOF"}});
            if(!write(path,input,binary))return false;dx_iface reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||!complete(data))return false;const auto r=dx_iface::entitiesDeliveryReport(data);
            if(r.sections!=1u||r.dispatches!=(noCallback?1u:0u)||r.events||r.models||r.rawBodies||r.valuesObserved)return false;++emptyOrNoCallback;
        }
        if(!write(path,tags,binary))return false;
        stage.name="owner teardown";
        {
            std::weak_ptr<const DRW_Entity> model;std::weak_ptr<const DRW_RawDxfObject> raw;
            std::weak_ptr<const DRW_DxfEntitiesDeliverySource> delivery;EntitiesOriginProbe reader;
            {dx_data owned;if(!reader.fileImport(path.string(),&owned,false)||!complete(owned))return false;
                const auto& row=A::entitiesDeliveries(owned)[0];model=(*row.observed)[0].model;raw=(*row.observed)[4].raw;delivery=row.delivery;}
            if(!model.expired()||!raw.expired()||!delivery.expired())return false;++teardowns;
        }
        stage.name="aborted imports";
        for(unsigned kind=0;kind<2u;++kind) {
            auto broken=tags;
            if(!kind)broken.resize(slot(broken,5,"F201")+1u);
            else broken.erase(broken.end()-2);
            if(!write(path,broken,binary))return false;EntitiesOriginProbe reader;dx_data data;
            if(reader.fileImport(path.string(),&data,false)||dx_iface::entitiesDeliveryReport(data).state!=S::ImportFailed)return false;
            if(!write(path,tags,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++aborts;
        }
        stage.name="selected TABLE origin gates";
        if(!write(path,bindingSource("AC1032"),binary))return false;EntitiesOriginProbe tables;dx_data data;
        if(!tables.fileImport(path.string(),&data,false)||!complete(data))return false;
        auto binding=dx_iface::tableContentBindingReport(data);
        if(binding.state!=B::Bound||binding.entitiesDeliveredTablesMatched!=1u||binding.outputEligible)return false;++tableOrigins;
        EntitiesOriginProbe absent;absent.enabled=false;dx_data noDelivery;if(!absent.fileImport(path.string(),&noDelivery,false))return false;
        binding=dx_iface::tableContentBindingReport(noDelivery);
        if(binding.state!=B::EntitiesDeliveryMissing||!binding.directRolesBound||binding.outputEligible)return false;++tableOrigins;
        EntitiesOriginProbe unfinished;unfinished.dropEnd=true;dx_data partial;if(!unfinished.fileImport(path.string(),&partial,false))return false;
        binding=dx_iface::tableContentBindingReport(partial);
        if(binding.state!=B::EntitiesDeliveryIncomplete||!binding.directRolesBound||binding.outputEligible)return false;++tableOrigins;
        auto& origin=A::mutableEntitiesDeliveries(data)[0];const auto owner=origin.delivery;
        origin.delivery=std::make_shared<const DRW_DxfEntitiesDeliverySource>(*owner);binding=dx_iface::tableContentBindingReport(data);
        if(binding.state!=B::EntitiesDeliveryMismatch||!binding.directRolesBound||binding.outputEligible)return false;++tableOrigins;
        origin.delivery=owner;if(dx_iface::tableContentBindingReport(data).state!=B::Bound)return false;++tableOrigins;
        struct CopyInsert final:dx_iface {
            unsigned projections=0u;
            void addInsert(const DRW_Insert& v) override {++projections;DRW_Insert copy(v);dx_iface::addInsert(copy);}
        } compatible;
        dx_data copiedProjection;if(!compatible.fileImport(path.string(),&copiedProjection,false)||!complete(copiedProjection))return false;
        binding=dx_iface::tableContentBindingReport(copiedProjection);
        if(binding.state!=B::Bound||binding.entitiesDeliveredTablesMatched!=1u||compatible.projections!=1u
            ||dx_iface::entitiesDeliveryReport(copiedProjection).events!=2u||dx_iface::entitiesDeliveryReport(copiedProjection).models!=2u)return false;
        ++tableOrigins;
    }
    std::cout<<"ENTITIES immutable origins: routes="<<routes<<"; independent dispatch joins="<<joins<<"; malformed final/retries="<<faults
        <<"; exact/one-under aggregate limits="<<limits<<"; observed final allocation faults="<<allocations<<"; hook allocation faults="<<hookFaults
        <<"; context/owner/restoration controls="<<contexts<<"; callback exception/retries="<<exceptions<<"; immutable edit contexts="<<aliases
        <<"; missing/unfinished hook contexts="<<missing<<"; abort/late-failure retries="<<aborts<<"; selected TABLE origin gates="<<tableOrigins
        <<"; explicit unqualified body dispositions="<<dispositions<<"; owner teardown contexts="<<teardowns<<"; forged raw/retries="<<rawFaults<<"; dormant-owner boundaries="<<dormant
        <<"; empty/no-callback-only scopes="<<emptyOrNoCallback<<'\n';
    stage.finished=true;
    return routes==12u&&joins==84u&&faults==44u&&limits==20u&&allocations>0&&hookFaults>0&&contexts==30u&&exceptions==8u&&aliases==2u&&missing==8u&&aborts==4u&&tableOrigins==12u&&dispositions==8u&&teardowns==2u&&rawFaults==10u&&dormant==4u&&emptyOrNoCallback==4u;
}
