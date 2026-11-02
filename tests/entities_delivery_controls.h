// SPDX-License-Identifier: GPL-2.0-or-later
struct EntitiesDeliveryProbe final:dx_iface {
    using C=DRW_DxfEntitiesCallback;using E=DRW_DxfEntitiesDeliveryEvent;
    bool enabled=true,physical=true,disableDuringPoint=false,bad=false,tableOpt=true,attdefOpt=true;
    int throwPhase=0;unsigned begins=0u,ends=0u,physicalCalls=0u,projections=0u;
    bool active=false;std::uint64_t activeScope=0u;std::size_t activeIndex=0u;E current;
    DRW_DxfEntitiesDeliveryLimits limits{};
    std::vector<DRW_DxfEntitiesDeliverySource> reports;
    std::vector<std::string> ordinary;
    bool wantsDxfEntitiesSource() const override {return physical&&dx_iface::wantsDxfEntitiesSource();}
    bool wantsDxfEntitiesDeliverySource() const override {return enabled;}
    DRW_DxfEntitiesDeliveryLimits dxfEntitiesDeliveryLimits() const override {return limits;}
    bool wantsDxfTableSource() const override {return tableOpt&&dx_iface::wantsDxfTableSource();}
    bool wantsDxfAttdefSource() const override {return attdefOpt&&dx_iface::wantsDxfAttdefSource();}
    void beginDxfEntitiesDelivery(std::uint64_t scope,std::size_t index,const E& event) override {
        if(active||!scope||index!=scopeBegins)bad=true;
        active=true;activeScope=scope;activeIndex=index;current=event;++begins;++scopeBegins;
        if(throwPhase==1)throw std::runtime_error("ENTITIES delivery begin");
        dx_iface::beginDxfEntitiesDelivery(scope,index,event);
    }
    void endDxfEntitiesDelivery(std::uint64_t scope,std::size_t index,const E& event) override {
        if(!active||scope!=activeScope||index!=activeIndex||event.callback!=current.callback||event.origin!=current.origin)bad=true;
        if(throwPhase==3)throw std::runtime_error("ENTITIES delivery end");
        dx_iface::endDxfEntitiesDelivery(scope,index,event);
        active=false;++ends;
    }
    void addDxfEntitiesSource(const DRW_DxfEntitiesSource& source) override {
        if(active)bad=true;++physicalCalls;dx_iface::addDxfEntitiesSource(source);
        if(throwPhase==4)throw std::runtime_error("ENTITIES physical callback");
    }
    void addDxfEntitiesDeliverySource(const DRW_DxfEntitiesDeliverySource& source) override {
        if(active||source.completedEvents!=source.events.size()||!physicalCalls)bad=true;
        reports.push_back(source);scopeBegins=0u;
        dx_iface::addDxfEntitiesDeliverySource(source);
        if(throwPhase==5)throw std::runtime_error("ENTITIES final callback");
    }
    void resetObservation() {
        active=false;activeScope=0u;activeIndex=0u;begins=ends=physicalCalls=projections=0u;scopeBegins=0u;
        bad=false;reports.clear();ordinary.clear();
    }
    void note(C callback,const char* text) {
        ordinary.emplace_back(text);
        if(active&&current.callback!=callback) {
            if(current.callback==C::addTable&&callback==C::addInsert)++projections;
            else bad=true;
        }
    }
    void addPoint(const DRW_Point& v) override {
        note(C::addPoint,"point");if(active&&disableDuringPoint)enabled=false;
        if(active&&throwPhase==2)throw std::runtime_error("ENTITIES ordinary callback");dx_iface::addPoint(v);
    }
    void addLine(const DRW_Line& v) override {note(C::addLine,"line");dx_iface::addLine(v);}
    void addCircle(const DRW_Circle& v) override {note(C::addCircle,"circle");if(throwPhase==6)throw std::runtime_error("proxy circle");dx_iface::addCircle(v);}
    void addProxyEntity(const DRW_ProxyEntity& v) override {note(C::addProxyEntity,"proxy");dx_iface::addProxyEntity(v);}
    void addPolyline(const DRW_Polyline& v) override {note(C::addPolyline,"polyline");dx_iface::addPolyline(v);}
    void addInsert(const DRW_Insert& v) override {note(C::addInsert,"insert");dx_iface::addInsert(v);}
    void addTable(const DRW_Table& v) override {note(C::addTable,"table");dx_iface::addTable(v);}
    void addAttDef(const DRW_Attdef& v) override {note(C::addAttDef,"attdef");dx_iface::addAttDef(v);}
    void addRawDxfEntity(const DRW_RawDxfObject& v) override {note(C::addRawDxfEntity,"raw");dx_iface::addRawDxfEntity(v);}
    void addDxfTableSource(const DRW_Table& v,const DRW_RawDxfObject& raw) override {
        if(active&&(current.callback!=C::TableSource||current.origin!=E::Origin::SourceObserver))bad=true;
        dx_iface::addDxfTableSource(v,raw);
    }
    void addDxfAttdefSource(const DRW_Attdef& v,const DRW_RawDxfObject& raw,const DRW_DxfAttdefSourceContext& context) override {
        if(active&&(current.callback!=C::AttdefSource||current.origin!=E::Origin::SourceObserver))bad=true;
        dx_iface::addDxfAttdefSource(v,raw,context);
    }
private:
    std::size_t scopeBegins=0u;
};
bool entitiesDeliveryOriginalWire(const std::filesystem::path& path,bool binary,const dx_data& data,const EntitiesDeliveryProbe& probe,std::size_t& joins) {
    using S=DRW_DxfEntitiesDeliverySource::Status;using C=DRW_DxfEntitiesCallback;
    const auto wire=attdefWireTags(path,binary);const auto& physical=DxIfaceTableSourceTestAccess::entitiesSources(data);
    if(probe.bad||probe.active||probe.begins!=probe.ends||probe.reports.size()!=physical.size())return false;
    const auto spanEqual=[](const auto& a,const auto& b){return a.ordinal==b.ordinal&&a.beginOffset==b.beginOffset&&a.endOffset==b.endOffset&&a.ignoredComment==b.ignoredComment;};
    for(std::size_t i=0;i<physical.size();++i) {
        const auto& p=*physical[i].physical;const auto& s=probe.reports[i];
        if(s.status!=S::Complete||s.scopeId!=p.scopeId||s.version!=p.version||s.binary!=p.binary
            ||!spanEqual(s.beginMarker,p.beginMarker)||!spanEqual(s.endMarker,p.endMarker)||s.completedEvents!=s.events.size())return false;
        std::size_t record=0u,event=0u;
        for(std::size_t j=0;j<s.dispatches.size();++j) {
            const auto& d=s.dispatches[j];if(record>=p.records.size()||!spanEqual(d.begin,p.records[record].marker)||d.firstEvent!=event)return false;
            const auto first=record;while(record<p.records.size()&&p.records[record].marker.ordinal<d.end.ordinal)++record;
            const auto end=record==p.records.size()?p.endMarker:p.records[record].marker;
            if(!spanEqual(d.end,end)||record==first||d.begin.ordinal>wire.size()||d.end.ordinal>wire.size()
                ||wire[d.begin.ordinal-1u].code!=0||wire[d.begin.ordinal-1u].begin!=d.begin.beginOffset
                ||wire[d.end.ordinal-1u].begin!=d.end.beginOffset)return false;
            const auto& name=p.records[first].name;
            if(name=="POLYLINE"&&record-first!=4u)return false;
            if(name=="INSERT"&&record-first!=3u)return false;
            if(name=="DIMENSION"&&d.events!=0u)return false;
            for(std::size_t k=0;k<d.events;++k,++event) {
                if(event>=s.events.size())return false;const auto& e=s.events[event];
                if(e.dispatch!=j||!e.hasValue||e.origin!=DRW_DxfEntitiesDeliveryEvent::Origin::Parser
                    ||e.callback==C::TableSource||e.callback==C::AttdefSource)return false;
            }
            ++joins;
        }
        if(record!=p.records.size()||event!=s.events.size())return false;
    }
    return probe.reports[0].dispatches.size()==7u&&probe.reports[0].events.size()==6u&&probe.reports[1].dispatches.empty();
}
bool entitiesDeliveryAllocationControls(long& faults) {
    using S=DRW_DxfEntitiesDeliverySource::Status;bool calibrated=false;
    for(long failure=0;failure<100&&!calibrated;++failure) {
        auto budget=std::make_shared<DxfReadRetainedBudget>(1024u*1024u);
        std::istringstream input("0\nPOINT\n0\nENDSEC\n");auto reader=std::make_unique<dxfReaderAscii>(&input);
        reader->setVersion("AC1032",true);reader->setRetainedByteBudget(budget);int code=0;
        if(!reader->readRec(&code))return false;DxfReadRecordView first;if(!reader->currentRecordView(first))return false;
        DRW_DxfEntitiesSource physical;physical.status=DRW_DxfEntitiesSource::Status::Collecting;physical.scopeId=1u;
        physical.version=reader->getSourceVersion();physical.beginMarker={first.ordinal,first.beginOffset,first.endOffset,false};
        const auto initial=budget->used();
        {
            dxfEntitiesDelivery::Capture capture;
            {SymbolAllocationFailure schedule(failure);capture.start(*reader,physical,{});capture.begin(*reader);
                const auto index=capture.event(DRW_DxfEntitiesCallback::addPoint,DRW_DxfEntitiesDeliveryEvent::Origin::Parser,0,1u,2u,true);
                if(index!=dxfEntitiesDelivery::Capture::NoEvent)capture.returned(index);}
            if(capture.source().status==S::Collecting)calibrated=true;
            else {if(capture.source().status!=S::Allocation)return false;++faults;}
        }
        if(budget->used()!=initial)return false;reader.reset();if(budget->used()!=0u)return false;
    }
    return calibrated&&faults>0;
}
bool entitiesDeliveryPublicControls(const std::filesystem::path& path,unsigned& routes,unsigned& retries) {
    struct Consumer final:dx_iface {
        int phase=0;unsigned points=0u,begins=0u,ends=0u,sources=0u,finals=0u;
        DRW_DxfEntitiesDeliverySource last;
        void addHeader(const DRW_Header*) override {}
        void addPoint(const DRW_Point&) override {++points;if(phase==2)throw std::runtime_error("point");}
        bool wantsDxfEntitiesSource() const override {return true;}
        void addDxfEntitiesSource(const DRW_DxfEntitiesSource&) override {++sources;if(phase==4)throw std::runtime_error("source");}
        bool wantsDxfEntitiesDeliverySource() const override {return true;}
        void beginDxfEntitiesDelivery(std::uint64_t,std::size_t,const DRW_DxfEntitiesDeliveryEvent&) override {++begins;if(phase==1)throw std::runtime_error("begin");}
        void endDxfEntitiesDelivery(std::uint64_t,std::size_t,const DRW_DxfEntitiesDeliveryEvent&) override {++ends;if(phase==3)throw std::runtime_error("end");}
        void addDxfEntitiesDeliverySource(const DRW_DxfEntitiesDeliverySource& s) override {++finals;last=s;if(phase==5)throw std::runtime_error("final");}
        void reset(){points=begins=ends=sources=finals=0u;last={};}
    } receiver;
    const auto document=[](const std::string& version) {return Tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"},
        {0,"SECTION"},{2,"ENTITIES"},{0,"POINT"},{5,"101"},{8,"0"},{10,"1"},{20,"2"},{0,"ENDSEC"},{0,"EOF"}};};
    dxfRW reader(path.string().c_str());std::uint64_t previous=0u;
    const auto valid=[&]() {
        if(receiver.points!=1u||receiver.begins!=1u||receiver.ends!=1u||receiver.sources!=1u||receiver.finals!=1u
            ||receiver.last.status!=DRW_DxfEntitiesDeliverySource::Status::Complete||receiver.last.completedEvents!=1u
            ||receiver.last.scopeId<=previous)return false;
        previous=receiver.last.scopeId;return true;
    };
    for(const std::string version:{"AC1009","AC1015","AC1021","AC1024","AC1027","AC1032"})for(bool crlf:{false,true}) {
        std::ostringstream out;for(const auto& tag:document(version))out<<tag.code<<(crlf?"\r\n":"\n")<<tag.value<<(crlf?"\r\n":"\n");
        auto input=out.str();receiver.reset();if(!reader.readAscii(&receiver,false,input)||!valid())return false;++routes;
    }
    for(bool binary:{false,true}) {
        if(!write(path,document("AC1032"),binary))return false;
        for(int phase=1;phase<=5;++phase) {
            receiver.reset();receiver.phase=phase;bool threw=false;
            try {if(reader.read(&receiver,false))return false;}catch(const std::runtime_error&){threw=true;}
            if(!threw||(phase!=5&&receiver.finals))return false;
            receiver.reset();receiver.phase=0;if(!reader.read(&receiver,false)||!valid())return false;++retries;
        }
    }
    return routes==12u&&retries==10u;
}
bool entitiesDeliveryPolymorphicControls(unsigned& routes) {
    struct Target final:dx_iface {
        const void* expected=nullptr;bool good=true;unsigned calls=0u;
        void addText(const DRW_Text& value) override {
            good=good&&(&value==expected)&&(dynamic_cast<const DRW_RText*>(&value)||dynamic_cast<const DRW_ArcAlignedText*>(&value));++calls;
        }
        void addDimRadial(const DRW_DimRadial* value) override {good=good&&value==expected&&dynamic_cast<const DRW_DimLargeRadial*>(value);++calls;}
        void addSurface(const DRW_Surface* value) override {good=good&&value==expected;++calls;}
        void addSpline(const DRW_Spline* value) override {good=good&&value==expected;++calls;}
        void addPolyline(const DRW_Polyline& value) override {good=good&&(&value==expected);++calls;}
    } target;
    struct Handler final:DRW_DxfEntitiesDeliveryHandler {
        unsigned begins=0u,ends=0u;
        void beginDxfEntitiesDelivery(std::uint64_t,std::size_t,const DRW_DxfEntitiesDeliveryEvent&) override {++begins;}
        void endDxfEntitiesDelivery(std::uint64_t,std::size_t,const DRW_DxfEntitiesDeliveryEvent&) override {++ends;}
        void addDxfEntitiesDeliverySource(const DRW_DxfEntitiesDeliverySource&) override {}
    } handler;
    std::istringstream input("0\nPOINT\n0\nENDSEC\n");dxfReaderAscii reader(&input);reader.setVersion("AC1032",true);int code=0;
    if(!reader.readRec(&code))return false;DxfReadRecordView v;if(!reader.currentRecordView(v))return false;
    DRW_DxfEntitiesSource physical;physical.status=DRW_DxfEntitiesSource::Status::Collecting;physical.scopeId=1u;physical.version=reader.getSourceVersion();
    physical.beginMarker={v.ordinal,v.beginOffset,v.endOffset,false};
    dxfEntitiesDelivery::Capture capture;capture.start(reader,physical,{});capture.begin(reader);dxfEntitiesDelivery::Sink sink(target,capture,&handler);
    DRW_RText rtext;target.expected=&rtext;sink.addText(rtext);
    DRW_ArcAlignedText arc;target.expected=&arc;sink.addText(arc);
    DRW_DimLargeRadial radial;target.expected=&radial;sink.addDimRadial(&radial);
    DRW_PlaneSurface plane;DRW_ExtrudedSurface extruded;DRW_RevolvedSurface revolved;DRW_SweptSurface swept;DRW_LoftedSurface lofted;DRW_NurbsSurface nurbs;
    const std::array<const DRW_Surface*,6> surfaces{{&plane,&extruded,&revolved,&swept,&lofted,&nurbs}};
    for(const auto* surface:surfaces) {target.expected=surface;sink.addSurface(surface);}
    DRW_Spline spline;target.expected=&spline;sink.addSpline(&spline);target.expected=nullptr;sink.addSpline(nullptr);
    DRW_Polyline polyline;target.expected=&polyline;sink.addPolyline(polyline);
    if(!reader.readRec(&code)||!reader.currentRecordView(v))return false;capture.end(reader);
    physical.status=DRW_DxfEntitiesSource::Status::Complete;physical.endMarker={v.ordinal,v.beginOffset,v.endOffset,false};capture.complete(physical);
    routes=target.calls;
    return target.good&&routes==12u&&handler.begins==routes&&handler.ends==routes&&capture.source().status==DRW_DxfEntitiesDeliverySource::Status::Complete
        &&capture.source().completedEvents==routes&&!capture.source().events[10u].hasValue;
}
bool entitiesDeliveryProxyControls(const std::filesystem::path& path,unsigned& routes,unsigned& retries) {
    using C=DRW_DxfEntitiesCallback;using O=DRW_DxfEntitiesDeliveryEvent::Origin;
    std::string bytes(8u,'\0');const auto word=[&](std::uint64_t n,unsigned width){for(unsigned i=0;i<width;++i)bytes.push_back(static_cast<char>(n>>(i*8u)));};
    word(64u,4u);word(2u,4u);for(double d:{2.,3.,4.,5.,0.,0.,1.}) {std::uint64_t bits=0;std::memcpy(&bits,&d,sizeof(d));word(bits,8u);}
    std::string hex;constexpr char digits[]="0123456789ABCDEF";for(unsigned char b:bytes){hex+=digits[b>>4u];hex+=digits[b&15u];}
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true}) {
        Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"},{0,"SECTION"},{2,"ENTITIES"},
            {0,"ACAD_PROXY_ENTITY"},{5,"101"},{330,"1F"},{100,"AcDbEntity"},{8,"HOST"},{100,"AcDbProxyEntity"},
            {90,"498"},{91,"500"},{95,"1032"},{70,"1"},{92,"72"},{310,hex},{93,"0"},{94,"0"},
            {0,"POINT"},{5,"102"},{330,"1F"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"1"},{20,"2"},{0,"ENDSEC"},{0,"EOF"}};
        if(!write(path,tags,binary))return false;EntitiesDeliveryProbe receiver;dx_data data;
        if(!receiver.fileImport(path.string(),&data,false)||receiver.bad||receiver.reports.size()!=1u)return false;
        const auto& s=receiver.reports.front();const std::vector<C> expected{C::addCircle,C::addProxyEntity,C::addRawDxfEntity,C::addPoint};
        if(s.status!=DRW_DxfEntitiesDeliverySource::Status::Complete||s.events.size()!=expected.size())return false;
        for(std::size_t i=0;i<expected.size();++i)if(s.events[i].callback!=expected[i]||s.events[i].origin!=(i?O::Parser:O::ProxyRendering))return false;
        ++routes;
        receiver.resetObservation();receiver.throwPhase=6;dx_data failed;bool threw=false;
        try{if(receiver.fileImport(path.string(),&failed,false))return false;}catch(const std::runtime_error&){threw=true;}
        if(!threw||!receiver.reports.empty())return false;receiver.resetObservation();receiver.throwPhase=0;dx_data retry;
        if(!receiver.fileImport(path.string(),&retry,false)||receiver.bad||receiver.reports[0].status!=DRW_DxfEntitiesDeliverySource::Status::Complete)return false;++retries;
    }
    return routes==8u&&retries==8u;
}
bool entitiesDeliveryTests(const std::filesystem::path& dir) {
    using S=DRW_DxfEntitiesDeliverySource::Status;using C=DRW_DxfEntitiesCallback;using O=DRW_DxfEntitiesDeliveryEvent::Origin;
    const auto path=dir/"entities-delivery.dxf";unsigned routes=0u,quota=0u,retries=0u,facetRoutes=0u,publicRoutes=0u,publicRetries=0u,latched=0u,aborts=0u,polymorphic=0u,proxy=0u,proxyRetries=0u;std::size_t joins=0u;long allocations=0;
    if(!entitiesDeliveryAllocationControls(allocations))return false;
    if(!entitiesDeliveryPublicControls(path,publicRoutes,publicRetries))return false;
    if(!entitiesDeliveryPolymorphicControls(polymorphic))return false;
    if(!entitiesDeliveryProxyControls(path,proxy,proxyRetries))return false;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0;encoding<3u;++encoding) {
        const auto tags=entitiesSourceDocument(version);if(!writeBlockBinding(path,tags,encoding))return false;
        EntitiesDeliveryProbe probe;dx_data data;
        if(!probe.fileImport(path.string(),&data,false)||!entitiesDeliveryOriginalWire(path,encoding==2u,data,probe,joins))return false;
        EntitiesDeliveryProbe disabled;disabled.enabled=false;dx_data old;
        if(!disabled.fileImport(path.string(),&old,false)||!disabled.reports.empty()||disabled.begins||disabled.ordinary!=probe.ordinary)return false;
        EntitiesDeliveryProbe missing;missing.physical=false;dx_data noPhysical;
        if(!missing.fileImport(path.string(),&noPhysical,false)||missing.begins||!missing.reports.empty()||missing.ordinary!=probe.ordinary)return false;
        ++routes;
    }
    for(bool binary:{false,true}) {
        const auto tags=entitiesSourceDocument("AC1032");if(!write(path,tags,binary))return false;
        EntitiesDeliveryProbe baseline;dx_data base;if(!baseline.fileImport(path.string(),&base,false))return false;
        const auto measured=baseline.reports.front();
        EntitiesDeliveryProbe latch;latch.disableDuringPoint=true;dx_data latchedData;
        if(!latch.fileImport(path.string(),&latchedData,false)||latch.bad||latch.reports.size()!=1u||latch.reports[0].status!=S::Complete
            ||latch.reports[0].events.size()!=6u||latch.physicalCalls!=2u)return false;
        ++latched;
        for(unsigned which=0;which<4u;++which)for(bool below:{false,true}) {
            EntitiesDeliveryProbe probe;probe.limits={measured.dispatches.size(),measured.events.size(),measured.bytes,measured.work};
            if(below){if(which==0u)--probe.limits.dispatches;else if(which==1u)--probe.limits.events;else if(which==2u)--probe.limits.bytes;else --probe.limits.work;}
            dx_data data;
            if(!probe.fileImport(path.string(),&data,false)||probe.reports.size()!=2u||probe.reports[0].status!=(below?S::Quota:S::Complete)
                ||probe.ordinary!=baseline.ordinary||probe.active||probe.bad)return false;
            ++quota;
        }
        for(int phase=1;phase<=5;++phase) {
            EntitiesDeliveryProbe probe;probe.throwPhase=phase;dx_data failed;
            bool threw=false;try {if(probe.fileImport(path.string(),&failed,false))return false;}
            catch(const std::runtime_error&){threw=true;}
            if(!threw||(phase!=5&&!probe.reports.empty())||!failed.dxfReadOutcome.readThrew)return false;
            probe.throwPhase=0;probe.resetObservation();dx_data retry;
            if(!probe.fileImport(path.string(),&retry,false)||probe.bad||probe.reports.size()!=2u||probe.reports[0].status!=S::Complete)return false;
            ++retries;
        }
        for(unsigned fault=0;fault<3u;++fault) {
            auto broken=tags;
            if(fault==0u)broken.resize(slot(broken,5,"F201")+1u);
            else if(fault==1u) {const auto first=slot(broken,5,"D103")-1u;auto end=first+1u;while(broken[end].code!=0)++end;
                broken.erase(broken.begin()+static_cast<std::ptrdiff_t>(first),broken.begin()+static_cast<std::ptrdiff_t>(end));}
            else {auto pos=slot(broken,2,"ENTITIES");while(++pos<broken.size()&&!(broken[pos].code==0&&broken[pos].value=="ENDSEC")){}
                broken[pos].value="EOF";broken.resize(pos+1u);}
            if(!write(path,broken,binary))return false;EntitiesDeliveryProbe rejected;dx_data failed;
            if(rejected.fileImport(path.string(),&failed,false)||!rejected.reports.empty())return false;
            if(!write(path,tags,binary))return false;rejected.resetObservation();dx_data retry;
            if(!rejected.fileImport(path.string(),&retry,false)||rejected.bad||rejected.reports[0].status!=S::Complete)return false;++aborts;
        }
        auto rich=bindingSource("AC1032");auto at=slot(rich,2,"ENTITIES");while(++at<rich.size()&&!(rich[at].code==0&&rich[at].value=="ENDSEC")){}
        const Tags attdef{{0,"ATTDEF"},{5,"AFF0"},{330,"1F"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbText"},
            {10,"0"},{20,"0"},{40,"1"},{1,"default"},{100,"AcDbAttributeDefinition"},{2,"TAG"},{3,"prompt"},{70,"0"}};
        rich.insert(rich.begin()+static_cast<std::ptrdiff_t>(at),attdef.begin(),attdef.end());
        if(!write(path,rich,binary))return false;
        for(unsigned mask=0;mask<4u;++mask) {
            EntitiesDeliveryProbe facets;facets.tableOpt=mask&1u;facets.attdefOpt=mask&2u;dx_data data;
            if(!facets.fileImport(path.string(),&data,false)||facets.bad||facets.reports.size()!=1u||facets.projections!=1u)return false;
            const auto& s=facets.reports.front();std::vector<C> expected;
            if(mask&1u)expected.push_back(C::TableSource);expected.push_back(C::addTable);
            if(mask&2u)expected.push_back(C::AttdefSource);expected.push_back(C::addAttDef);
            if(s.status!=S::Complete||s.events.size()!=expected.size())return false;
            for(std::size_t i=0;i<expected.size();++i)if(s.events[i].callback!=expected[i]
                ||s.events[i].origin!=((expected[i]==C::TableSource||expected[i]==C::AttdefSource)?O::SourceObserver:O::Parser))return false;
            ++facetRoutes;
        }
    }
    std::cout<<"ENTITIES delivery: routes="<<routes<<"; independent dispatch joins="<<joins<<"; exact/one-under limits="<<quota
        <<"; facade callback exception/retries="<<retries<<"; TABLE/ATTDEF facet routes="<<facetRoutes<<"; isolated allocation faults="<<allocations
        <<"; public readAscii routes="<<publicRoutes<<"; same-reader exception/retries="<<publicRetries<<"; latched sections="<<latched<<"; abort/retries="<<aborts
        <<"; polymorphic/pointer routes="<<polymorphic<<"; typed proxy routes="<<proxy<<"; typed proxy exception/retries="<<proxyRetries<<'\n';
    return routes==12u&&joins==84u&&quota==16u&&retries==10u&&facetRoutes==8u&&latched==2u&&aborts==6u;
}
