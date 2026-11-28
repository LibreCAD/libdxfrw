// SPDX-License-Identifier: GPL-2.0-or-later
class DeliveryProbe final:public BlockSourceProbe {
public:
    using Source=DRW_DxfBlockDeliverySource;
    using Event=DRW_DxfBlockDeliveryEvent;
    bool deliverySelected=true;
    bool attdefSelected=true;
    int protocol=0,callbackFault=0,throwDelivery=0;
    long allocationFault=-1,allocationCount=0;
    unsigned reports=0,finished=0;
    Source alias;
    DRW_DxfBlockDeliveryLimits limits;
    std::vector<std::size_t> starts,stops;
    std::function<void(Source&)> alter;
    bool wantsDxfBlockDeliverySource() const override {return deliverySelected&&dx_iface::wantsDxfBlockDeliverySource();}
    bool wantsDxfAttdefSource() const override {return attdefSelected&&dx_iface::wantsDxfAttdefSource();}
    DRW_DxfBlockDeliveryLimits dxfBlockDeliveryLimits() const override {return limits;}
    void addDxfBlockDeliverySource(const Source& value) override {
        ++reports;alias=value;if(alter)alter(alias);
        if(allocationFault>=0)allocationRemaining=allocationFault;
        if(protocol!=1)dx_iface::addDxfBlockDeliverySource(alias);
        if(protocol==2)dx_iface::addDxfBlockDeliverySource(alias);
        if(allocationFault>=0){allocationCount=allocationFault-allocationRemaining;allocationRemaining=-1;}
        if(throwDelivery==1)throw std::runtime_error("delivery report fault");
    }
    void beginDxfBlockDelivery(std::uint64_t scope,std::uint32_t block,std::size_t index,const Event& value) override {
        starts.push_back(index);if(throwDelivery==2)throw std::runtime_error("delivery begin fault");
        if(protocol!=3)dx_iface::beginDxfBlockDelivery(protocol==11?scope+1u:scope,protocol==6?block+1u:block,protocol==5?index+1u:index,value);
        if(protocol==4)dx_iface::beginDxfBlockDelivery(scope,block,index,value);
    }
    void endDxfBlockDelivery(std::uint64_t scope,std::uint32_t block,std::size_t index,const Event& value) override {
        stops.push_back(index);if(throwDelivery==3)throw std::runtime_error("delivery end fault");
        if(protocol!=7)dx_iface::endDxfBlockDelivery(protocol==12?scope+1u:scope,block,index,value);
        if(protocol==8)dx_iface::endDxfBlockDelivery(scope,block,index,value);
    }
    void finishDxfBlockDelivery(std::uint64_t scope,std::uint32_t block) override {
        ++finished;if(throwDelivery==4)throw std::runtime_error("delivery finish fault");
        if(protocol!=9)dx_iface::finishDxfBlockDelivery(protocol==13?scope+1u:scope,block);
        if(protocol==10)dx_iface::finishDxfBlockDelivery(scope,block);
    }
    void addAttDef(const DRW_Attdef& value) override {
        if(callbackFault==1)return;
        if(callbackFault==10)cData->typedEntityCaptureFailed=true;
        if(callbackFault==11) {
            // Preserve the live parser cursor: this isolates modeled field
            // editing rather than the established copy-clears-cursor rule.
            const_cast<DRW_Attdef&>(value).basePoint.x+=1.0;
            BlockSourceProbe::addAttDef(value);
        } else if(callbackFault==3||callbackFault==4) {
            auto changed=value;if(callbackFault==3)++changed.handle;else ++changed.parentHandle;
            BlockSourceProbe::addAttDef(changed);
        } else BlockSourceProbe::addAttDef(value);
        if(callbackFault==2)BlockSourceProbe::addAttDef(value);
    }
    void addRawDxfEntity(const DRW_RawDxfObject& value) override {
        if(callbackFault==7)return;
        if(callbackFault==9)cData->rawDxfEntityCaptureFailed=true;
        auto changed=value;
        if(callbackFault==5)for(auto& v:changed.groups)if(v.code()==8){v.addString(8,"changed-layer");break;}
        BlockSourceProbe::addRawDxfEntity(changed);
    }
    void addPolyline(const DRW_Polyline& value) override {
        if(callbackFault==6){DRW_Point point;point.handle=value.handle;point.parentHandle=value.parentHandle;dx_iface::addPoint(point);return;}
        BlockSourceProbe::addPolyline(value);
    }
    void addDxfAttdefSource(const DRW_Attdef& value,const DRW_RawDxfObject& raw,const DRW_DxfAttdefSourceContext& context) override {
        auto changed=raw;
        if(callbackFault==8)for(auto& field:changed.groups)if(field.code()==3){field.addString(3,"changed-prompt");break;}
        dx_iface::addDxfAttdefSource(value,changed,context);
    }
};

bool deliveryProducerControls(const std::filesystem::path& path,long& allocationFaults,unsigned& malformed,unsigned& precharge) {
    using S=DRW_DxfBlockDeliverySource::Status;
    const Tags tags{{0,"POINT"},{5,"C1"},{0,"ENDBLK"}};
    for(bool binary:{false,true}) {
        if(!write(path,tags,binary))return false;
        const auto run=[&](long schedule,int fault,long& used) {
            auto budget=std::make_shared<DxfReadRetainedBudget>(1048576u);bool ok=true;long remaining=schedule;
            {
                std::ifstream input(path,std::ios::binary);if(binary)input.seekg(22);
                std::unique_ptr<dxfReader> reader=binary?std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
                    :std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
                reader->setVersion("AC1032",true);reader->setRetainedByteBudget(budget);
                reader->setInputByteBudget(1048576u,binary?22u:0u);
                int code=0;DxfReadRecordView first,next;
                if(!reader->readRec(&code)||code!=0||!reader->currentRecordView(first))return false;
                {
                    dxfBlockDelivery::Capture capture;
                    const auto injected=[&](auto call){allocationRemaining=remaining;call();remaining=allocationRemaining;allocationRemaining=-1;};
                    injected([&](){capture.start(*reader,0xB1u,0xB0u,{});});
                    auto altered=first;if(fault==1)++altered.ordinal;if(fault==2)altered.code=5;
                    if(fault==4)injected([&](){capture.end(first);});
                    if(fault==5)injected([&](){(void)capture.event(DRW_DxfBlockCallback::addPoint,DRW::POINT,0xC1u,0xB0u,true);});
                    injected([&](){capture.begin(altered);});
                    if(fault==3)injected([&](){capture.begin(first);});
                    injected([&](){(void)capture.event(DRW_DxfBlockCallback::addPoint,DRW::POINT,0xC1u,0xB0u,true);});
                    if(fault==8)capture.complete();
                    if(!reader->readRec(&code)||!reader->readRec(&code)||code!=0||!reader->currentRecordView(next))return false;
                    if(fault==6)next.ordinal=first.ordinal;if(fault==7)next.beginOffset=first.beginOffset;
                    capture.end(next);capture.complete();
                    const auto& source=capture.source();used=schedule-remaining;
                    if(fault)ok=source.status==S::InvalidBoundary;
                    else if(schedule==(std::numeric_limits<long>::max)())ok=source.status==S::Complete
                        &&source.dispatches.size()==1u&&source.events.size()==1u&&source.dispatches.front().begin.ordinal==1u
                        &&source.dispatches.front().end.ordinal==3u&&source.dispatches.front().events==1u;
                    else ok=source.status==S::Allocation;
                }
                reader.reset();
            }
            return ok&&budget->used()==0u;
        };
        long count=0;if(!check(run((std::numeric_limits<long>::max)(),0,count)&&count>0&&count<100,
            "calibrated origin producer allocations and logical lease teardown"))return false;
        for(long fault=0;fault<count;++fault) {
            long ignored=0;if(!check(run(fault,0,ignored)&&run((std::numeric_limits<long>::max)(),0,ignored),
                "producer allocation fault is explicit, leak-free in logical counters and recoverable"))return false;
            ++allocationFaults;
        }
        for(int fault=1;fault<=8;++fault) {
            long ignored=0;if(!check(run((std::numeric_limits<long>::max)(),fault,ignored),"malformed producer dispatch lifecycle cannot complete evidence"))return false;
            ++malformed;
        }
        {
            std::ifstream input(path,std::ios::binary);if(binary)input.seekg(22);
            std::unique_ptr<dxfReader> reader=binary?std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
                :std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
            int code=0;if(!reader->readRec(&code)||code!=0)return false;
            // Isolate the new capture header, not a full-input budget profile.
            auto zero=std::make_shared<DxfReadRetainedBudget>(0u);reader->setRetainedByteBudget(zero);
            dxfBlockDelivery::Capture capture;allocationRemaining=0;
            capture.start(*reader,0xB1u,0xB0u,{});allocationRemaining=-1;
            if(!check(capture.source().status==S::Quota&&zero->exceeded()&&zero->used()==0u,
                "shared capture-header quota is admitted before attempting lease/control allocation"))return false;
            ++precharge;
        }
    }
    return true;
}

bool blockPolymorphicDeliveryTests(const std::filesystem::path& path,unsigned& profiles) {
    using A=DxIfaceTableSourceTestAccess;
    class Probe final:public BlockSourceProbe {
    public:
        bool origins=true,matched=false;
        bool wantsDxfBlockDeliverySource() const override {return origins&&dx_iface::wantsDxfBlockDeliverySource();}
        void addText(const DRW_Text& value) override {
            if(const auto* r=dynamic_cast<const DRW_RText*>(&value))matched=r->m_rTextFlags==1&&r->text=="diesel";
            if(const auto* arc=dynamic_cast<const DRW_ArcAlignedText*>(&value))matched=arc->m_radius==5.0&&arc->m_textSize=="1.25"
                &&arc->m_fontName=="FONT"&&arc->text=="arc-text";
            dx_iface::addText(value);
        }
        void addDimRadial(const DRW_DimRadial* value) override {
            if(const auto* large=dynamic_cast<const DRW_DimLargeRadial*>(value))matched=large->overrideCenterPoint.x==6.0
                &&large->jogPoint.y==9.0&&large->jogAngle==0.75&&large->getChordPoint().x==4.0;
            dx_iface::addDimRadial(value);
        }
    };
    bool all=true;
    for(bool binary:{false,true})for(bool origins:{false,true})for(unsigned kind=0;kind<3u;++kind) {
        auto f=blockFixture("AC1032",0u,true);
        Tags row{{0,kind==0u?"RTEXT":kind==1u?"ARCALIGNEDTEXT":"LARGE_RADIAL_DIMENSION"},{5,"F1"},{330,"B0"},{100,"AcDbEntity"},{8,"0"}};
        if(kind==0u)row.insert(row.end(),{{100,"AcDbRText"},{1,"diesel"},{10,"2"},{20,"3"},{40,"4"},{70,"1"}});
        if(kind==1u)row.insert(row.end(),{{100,"AcDbArcAlignedText"},{1,"arc-text"},{10,"2"},{20,"3"},{40,"5"},{50,"0"},{51,"90"},{42,"1.25"},{2,"FONT"}});
        if(kind==2u)row.insert(row.end(),{{100,"AcDbDimension"},{100,"AcDbRadialDimensionLarge"},{13,"4"},{23,"5"},
            {14,"6"},{24,"7"},{15,"8"},{25,"9"},{40,"0.75"}});
        const auto end=std::find_if(f.document.begin(),f.document.end(),[](const Tag& t){return t.code==0&&t.value=="ENDBLK";});
        f.document.insert(end,row.begin(),row.end());if(!write(path,f.document,binary))return false;
        Probe reader;reader.origins=origins;dx_data data;
        const bool read=reader.fileImport(path.string(),&data,false);
        if(!check(read&&reader.matched&&(!origins||A::deliveryTransport(data)),"staged base callbacks preserve the original dynamic subtype and subtype fields")) {
            std::cerr<<row.front().value<<" binary="<<binary<<" origins="<<origins<<" read="<<read<<" matched="<<reader.matched<<'\n';all=false;
        }
        ++profiles;
    }
    return all;
}

bool blockDeliveryTests(const std::filesystem::path& path) {
    using A=DxIfaceTableSourceTestAccess;using C=DRW_DxfBlockCallback;using S=DRW_DxfBlockDeliverySource::Status;
    const auto spanMatches=[](const DRW_DxfPhysicalRecordSpan& a,const DRW_DxfPhysicalRecordSpan& b) {
        return a.ordinal==b.ordinal&&a.beginOffset==b.beginOffset&&a.endOffset==b.endOffset&&a.ignoredComment==b.ignoredComment;
    };
    unsigned profiles=0u,faults=0u,protocols=0u,callbacks=0u,throws=0u,quota=0u,producerQuota=0u,noCallbacks=0u,producerMalformed=0u,stale=0u,later=0u,fieldBoundary=0u,facetOptout=0u,polymorphic=0u,precharge=0u;
    long allocations=0,producerAllocations=0;std::size_t spans=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0;encoding<3u;++encoding)
     for(unsigned shape=0;shape<3u;++shape) {
        const bool binary=encoding==2u;auto f=shape==2u?mixedBlockChildrenFixture(version):blockFixture(version,shape,true);
        if(!write(path,f.document,binary,encoding==1u))return false;
        DeliveryProbe reader;dx_data data;
        if(!check(reader.fileImport(path.string(),&data,false)&&A::deliveryTransport(data)&&!A::failed(data)&&reader.depth==0u
            &&reader.reports==1u&&reader.finished==1u,"real source-to-staged dispatch and observed callback transport")) {
            std::cerr<<version<<" encoding="<<encoding<<" shape="<<shape<<" report="<<reader.reports<<" status="<<static_cast<int>(reader.alias.status)
                <<" events="<<reader.alias.events.size()<<" observed="<<reader.starts.size()<<" failed="<<A::deliveryFailed(data)<<'\n';return false;
        }
        if(!check(originalChildrenWire(path,binary,f,reader.childrenAlias,spans),"independent physical file anchors dispatch ranges"))return false;
        const auto& source=reader.alias;const auto* row=A::block(data);
        if(source.dispatches.size()!=(shape==2u?5u:shape)||source.events.size()!=(shape==2u?7u:shape*2u)
            ||!row->delivery||row->delivery.get()==&reader.alias)return false;
        const std::array<std::size_t,6> physical{{0u,1u,2u,6u,9u,10u}};
        const std::array<std::size_t,6> events{{0u,2u,3u,4u,5u,7u}};
        for(std::size_t i=0;i<source.dispatches.size();++i) {
            const auto first=shape==2u?physical[i]:i,end=shape==2u?physical[i+1u]:i+1u;
            const auto& dispatch=source.dispatches[i];const auto& next=end<reader.childrenAlias.records.size()
                ?reader.childrenAlias.records[end].marker:reader.childrenAlias.endMarker;
            if(!spanMatches(dispatch.begin,reader.childrenAlias.records[first].marker)
                ||!spanMatches(dispatch.end,next)||dispatch.firstEvent!=(shape==2u?events[i]:i*2u)
                ||dispatch.events!=(shape==2u?events[i+1u]-events[i]:2u))return false;
        }
        const std::array<C,7> kinds{{C::AttdefSource,C::addAttDef,C::addRawDxfEntity,C::addPolyline,C::addInsert,C::AttdefSource,C::addAttDef}};
        for(std::size_t i=0;i<source.events.size();++i) {
            if(reader.starts[i]!=i||reader.stops[i]!=i||source.events[i].callback!=(shape==2u?kinds[i]:(i%2u?C::addAttDef:C::AttdefSource)))return false;
        }
        reader.alias.events.clear();reader.alias.dispatches.clear();
        if(row->delivery->events.size()!=(shape==2u?7u:shape*2u))return false;
        ++profiles;
    }
    for(bool binary:{false,true}) {
        const auto f=mixedBlockChildrenFixture("AC1032");if(!write(path,f.document,binary))return false;
        DeliveryProbe baseline;dx_data base;if(!baseline.fileImport(path.string(),&base,false)||!A::deliveryTransport(base))return false;
        const auto usage=A::deliveryUsage(base);
        DeliveryProbe optout;optout.deliverySelected=false;dx_data ordinary;
        if(!check(optout.fileImport(path.string(),&ordinary,false)&&optout.reports==0u&&!A::deliveryFailed(ordinary)
            &&A::deliveryUsage(ordinary)==std::array<std::size_t,4>{0u,0u,0u,0u},"opt-out has no origin manifest snapshot charges"))return false;
        for(unsigned dimension=0;dimension<4u;++dimension) {
            dx_data exact;A::deliveryLimit(exact,dimension,usage[dimension]);DeliveryProbe reader;
            if(!check(reader.fileImport(path.string(),&exact,false)&&A::deliveryTransport(exact),"exact delivery snapshot/replay ceiling"))return false;
            dx_data below;A::deliveryLimit(below,dimension,usage[dimension]-1u);
            if(!check(reader.fileImport(path.string(),&below,false)&&A::deliveryFailed(below)&&!A::deliveryTransport(below),"one-under delivery snapshot/replay ceiling refuses evidence"))return false;
            dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::deliveryTransport(retry))return false;++quota;
            const auto n=dimension==0u?baseline.alias.dispatches.size():dimension==1u?baseline.alias.events.size()
                :dimension==2u?baseline.alias.bytes:baseline.alias.work;
            DeliveryProbe producer;auto set=[&](std::size_t value){switch(dimension){case 0:producer.limits.dispatches=value;break;
                case 1:producer.limits.events=value;break;case 2:producer.limits.bytes=value;break;case 3:producer.limits.work=value;break;}};
            set(n);dx_data accepted;if(!producer.fileImport(path.string(),&accepted,false)||!A::deliveryTransport(accepted))return false;
            set(n-1u);dx_data refused;
            if(!check(producer.fileImport(path.string(),&refused,false)&&producer.alias.status==S::Quota&&A::deliveryFailed(refused),
                "producer own quota reports failed origin evidence without changing ordinary import"))return false;
            producer.limits={};dx_data recovered;if(!producer.fileImport(path.string(),&recovered,false)||!A::deliveryTransport(recovered))return false;++producerQuota;
        }
        for(unsigned fault=0;fault<22u;++fault) {
            DeliveryProbe reader;reader.alter=[=](DRW_DxfBlockDeliverySource& s){switch(fault){
                case 0:s.blockHandle=99u;break;case 1:++s.blockRecord;break;case 2:s.version=DRW::AC1015;break;
                case 3:s.binary=!s.binary;break;case 4:s.status=S::Collecting;break;case 5:++s.endMarker.ordinal;break;
                case 6:s.dispatches.erase(s.dispatches.begin());break;case 7:++s.dispatches.front().begin.ordinal;break;
                case 8:++s.dispatches.front().end.beginOffset;break;case 9:++s.dispatches.front().firstEvent;break;
                case 10:++s.dispatches.front().events;break;case 11:s.events.pop_back();break;case 12:++s.events.front().dispatch;break;
                case 13:s.events.front().callback=C::Count;break;case 14:++s.events.front().handle;break;
                case 15:++s.events.front().owner;break;case 16:++s.events.front().entityType;break;case 17:s.events.front().hasValue=false;break;
                case 18:s.scopeId=0u;break;case 19:++s.scopeId;break;
                case 20:s.events.front().callback=C::addPoint;break;case 21:std::swap(s.events[0],s.events[1]);break;}};
            dx_data bad;if(!check(reader.fileImport(path.string(),&bad,false)&&A::deliveryFailed(bad)&&!A::deliveryTransport(bad),"malformed/stale/mutated origin manifest cannot qualify delivery"))return false;
            reader.alter={};dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::deliveryTransport(retry))return false;++faults;
        }
        for(int fault=1;fault<=13;++fault) {
            DeliveryProbe reader;reader.protocol=fault;dx_data bad;
            if(!check(reader.fileImport(path.string(),&bad,false)&&A::deliveryFailed(bad)&&!A::deliveryTransport(bad),"missing/duplicate/wrong-context replay protocol refuses evidence"))return false;
            reader.protocol=0;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::deliveryTransport(retry))return false;++protocols;
        }
        for(int fault=1;fault<=10;++fault) {
            DeliveryProbe reader;reader.callbackFault=fault;dx_data bad;
            if(!check(reader.fileImport(path.string(),&bad,false)&&A::deliveryFailed(bad)&&!A::deliveryTransport(bad),"omitted/duplicate/misrouted/mutated actual callback cannot match staged origin"))return false;
            reader.callbackFault=0;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::deliveryTransport(retry))return false;++callbacks;
        }
        DeliveryProbe edited;edited.callbackFault=11;dx_data sameIdentity;
        if(!check(edited.fileImport(path.string(),&sameIdentity,false)&&A::deliveryTransport(sameIdentity)&&A::failed(sameIdentity)
            &&A::resource(sameIdentity)&&!A::resource(sameIdentity)->attdefFieldsMatched,
            "origin/identity transport cannot replace frozen ATTDEF field/model proof for same-ID edits"))return false;
        ++fieldBoundary;
        DeliveryProbe noPair;noPair.attdefSelected=false;dx_data unpaired;
        if(!check(noPair.fileImport(path.string(),&unpaired,false)&&A::deliveryTransport(unpaired)&&noPair.alias.events.size()==5u
            &&std::none_of(noPair.alias.events.begin(),noPair.alias.events.end(),[](const DRW_DxfBlockDeliveryEvent& e){return e.callback==C::AttdefSource;}),
            "an opted-out source facet is not manufactured from a typed ATTDEF callback"))return false;
        ++facetOptout;
        for(int phase=1;phase<=4;++phase) {
            DeliveryProbe reader;reader.throwDelivery=phase;dx_data bad;bool threw=false;
            try{(void)reader.fileImport(path.string(),&bad,false);}catch(const std::runtime_error&){threw=true;}
            if(!check(threw&&reader.depth==0u&&!A::deliveryTransport(bad),"replay/report exceptions balance scope without certifying partial delivery"))return false;
            reader.throwDelivery=0;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::deliveryTransport(retry))return false;++throws;
        }
        bool calibrated=false;
        for(long fault=0;fault<100&&!calibrated;++fault) {
            DeliveryProbe reader;reader.allocationFault=fault;dx_data data;
            if(!reader.fileImport(path.string(),&data,false))return false;
            if(A::deliveryTransport(data)){if(reader.allocationCount!=fault)return false;calibrated=true;}
            else {if(!A::deliveryFailed(data))return false;++allocations;}
            reader.allocationFault=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::deliveryTransport(retry))return false;
        }
        if(!calibrated)return false;
        for(bool empty:{false,true}) {
            const auto same=empty?blockFixture("AC1032",0u,true):f;if(!write(path,same.document,binary))return false;
            DeliveryProbe reader;dx_data old;if(!reader.fileImport(path.string(),&old,false)||!A::deliveryTransport(old))return false;
            const auto oldManifest=reader.alias;
            reader.alter=[&](DRW_DxfBlockDeliverySource& value){value=oldManifest;};dx_data replayed;
            if(!check(reader.fileImport(path.string(),&replayed,false)&&A::deliveryFailed(replayed)&&!A::deliveryTransport(replayed),
                "stale manifest from identical input cannot borrow a new callback/empty-block scope"))return false;
            reader.alter={};dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::deliveryTransport(retry))return false;++stale;
        }
        auto broken=f;broken.document.insert(broken.document.end()-1,{{0,"SECTION"},{2,"ENTITIES"},{0,"POINT"},{5,"F1"},{10,"0"},{0,"ENDSEC"}});
        if(!write(path,broken.document,binary))return false;
        DeliveryProbe laterInput;dx_data failed;
        if(!check(!laterInput.fileImport(path.string(),&failed,false)&&A::block(failed)&&A::block(failed)->deliveryFinished
            &&!A::deliveryTransport(failed),"finished block delivery cannot certify a later failed whole import"))return false;
        ++later;
        if(!write(path,f.document,binary))return false;
        DeliveryProbe cleanup;cleanup.throwPhase=5;dx_data thrown;bool propagated=false;
        try{(void)cleanup.fileImport(path.string(),&thrown,false);}catch(const std::runtime_error&){propagated=true;}
        if(!check(propagated&&cleanup.depth==0u&&A::block(thrown)&&A::block(thrown)->deliveryFinished&&!A::deliveryTransport(thrown),
            "completed replay remains unqualified after scope cleanup exception"))return false;
        ++later;
        // Existing parser behavior for an unhandled DIMENSION subtype. Its
        // physical membership must remain explicit despite zero callbacks;
        // this is not a supported DIMENSION geometry/profile assertion.
        auto noCallback=blockFixture("AC1032",0u,true);
        const auto end=std::find_if(noCallback.document.begin(),noCallback.document.end(),[](const Tag& t){return t.code==0&&t.value=="ENDBLK";});
        noCallback.document.insert(end,{{0,"DIMENSION"},{5,"F1"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbDimension"},{70,"7"}});
        if(!write(path,noCallback.document,binary))return false;
        DeliveryProbe noEvent;dx_data skipped;
        if(!check(noEvent.fileImport(path.string(),&skipped,false)&&A::deliveryTransport(skipped)
            &&noEvent.alias.dispatches.size()==1u&&noEvent.alias.events.empty()&&noEvent.childrenAlias.records.size()==1u,
            "no-callback parser disposition remains an explicit original dispatch, not missing physical membership"))return false;
        ++noCallbacks;
    }
    if(!deliveryProducerControls(path,producerAllocations,producerMalformed,precharge))return false;
    if(!blockPolymorphicDeliveryTests(path,polymorphic))return false;
    std::cout<<"Block delivery profiles="<<profiles<<"; original scalar spans="<<spans<<"; manifest faults/retries="<<faults
        <<"; replay protocol faults/retries="<<protocols<<"; callback faults/retries="<<callbacks<<"; exceptions/retries="<<throws
        <<"; snapshot ceilings="<<quota<<"; producer ceilings="<<producerQuota<<"; snapshot allocation faults/retries="<<allocations
        <<"; zero-callback dispositions="<<noCallbacks<<"; producer allocation faults/retries="<<producerAllocations
        <<"; malformed producer lifecycle="<<producerMalformed<<"; stale manifest scopes/retries="<<stale<<"; later import/cleanup failures="<<later
        <<"; field-proof boundaries="<<fieldBoundary<<"; independent facet opt-outs="<<facetOptout<<"; polymorphic callback profiles="<<polymorphic
        <<"; header shared-quota precharge="<<precharge<<'\n';
    return profiles==36u&&spans==2712u&&faults==44u&&protocols==26u&&callbacks==20u&&throws==8u&&quota==8u&&producerQuota==8u
        &&allocations==6&&noCallbacks==2u&&producerAllocations==6&&producerMalformed==16u&&stale==4u&&later==4u&&fieldBoundary==2u&&facetOptout==2u&&polymorphic==12u&&precharge==2u;
}
