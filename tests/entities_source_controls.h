// SPDX-License-Identifier: GPL-2.0-or-later
struct EntitiesProbe final:dx_iface {
    bool optIn=true,force=false,disableDuringPoint=false,mutateAfter=false,premature=false;
    int throwSource=-1;bool throwPoint=false;long failCopy=-1;
    unsigned calls=0u;DRW_DxfEntitiesSourceLimits limits{};
    std::array<std::size_t,4> measured{};
    std::function<void(DRW_DxfEntitiesSource&)> alter;
    std::vector<std::string> events;
    bool wantsDxfEntitiesSource() const override {return optIn&&(force||dx_iface::wantsDxfEntitiesSource());}
    DRW_DxfEntitiesSourceLimits dxfEntitiesSourceLimits() const override {return limits;}
    void addDxfEntitiesSource(const DRW_DxfEntitiesSource& value) override {
        ++calls;events.push_back("source");measured={value.records.size(),value.groups,value.bytes,value.work};
        if(dx_iface::entitiesSourceReport(*cData).state==EntitiesSourceState::TransportComplete)premature=true;
        if(throwSource==0)throw std::runtime_error("ENTITIES source callback before capture");
        if(alter){auto copy=value;alter(copy);dx_iface::addDxfEntitiesSource(copy);}
        else {SymbolAllocationFailure schedule(failCopy);dx_iface::addDxfEntitiesSource(value);}
        if(dx_iface::entitiesSourceReport(*cData).state==EntitiesSourceState::TransportComplete)premature=true;
        if(mutateAfter&&!value.records.empty())const_cast<DRW_DxfEntitiesSource&>(value).records.front().name="changed-after-capture";
        if(throwSource==1)throw std::runtime_error("ENTITIES source callback after capture");
    }
    void addPoint(const DRW_Point& value) override {
        events.push_back("point");dx_iface::addPoint(value);
        if(value.handle==0x101u&&disableDuringPoint)optIn=false;
        if(throwPoint&&value.handle==0x101u)throw std::runtime_error("ENTITIES point callback");
    }
    void addLine(const DRW_Line& value) override {events.push_back("line");dx_iface::addLine(value);}
    void addPolyline(const DRW_Polyline& value) override {events.push_back("polyline");dx_iface::addPolyline(value);}
    void addInsert(const DRW_Insert& value) override {events.push_back("insert");dx_iface::addInsert(value);}
    void addRawDxfEntity(const DRW_RawDxfObject& value) override {events.push_back("raw");dx_iface::addRawDxfEntity(value);}
};
Tags entitiesSourceDocument(const std::string& version) {
    Tags t{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"},
        {0,"SECTION"},{2,"BLOCKS"},{0,"BLOCK"},{5,"B1"},{330,"B100"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},{2,"NOT-ENTITIES"},{70,"0"},{10,"0"},{20,"0"},
        {0,"POINT"},{5,"1A"},{330,"B100"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"0"},{20,"0"},
        {0,"ENDBLK"},{5,"B2"},{330,"B100"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"},{0,"ENDSEC"},
        {0,"SECTION"},{2,"ENTITIES"},{999,"outside interval"},
        {0,"POINT"},{5,"101"},{330,"1F"},{100,"AcDbEntity"},{8,"0"},
        {102,"{CUSTOM"},{5,"DEAD"},{330,"BEEF"},{320,"FFFFFFFFFFFFFFFF"},{999,"inside application"},{102,"}"},
        {100,"AcDbPoint"},{10,"-0.0"},{20,"1.25"},{30,"2.5"},{999,"point tail"},
        {0,"LINE"},{5,"102"},{330,"2F"},{100,"AcDbEntity"},{67,"1"},{8,"0"},{100,"AcDbLine"},{10,"1"},{20,"2"},{30,"3"},{11,"4"},{21,"5"},{31,"6"},
        {0,"POLYLINE"},{5,"D100"},{330,"1F"},{100,"AcDbEntity"},{8,"0"},{100,"AcDb3dPolyline"},{66,"1"},{10,"0"},{20,"0"},{30,"0"},{70,"8"}};
    for(const char* h:{"D101","D102"})t.insert(t.end(),{{0,"VERTEX"},{5,h},{330,"D100"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbVertex"},{100,"AcDb3dPolylineVertex"},{10,"1"},{20,"2"},{30,"3"},{70,"32"}});
    t.insert(t.end(),{{0,"SEQEND"},{5,"D103"},{330,"D100"},{100,"AcDbEntity"},{8,"0"},
        {0,"INSERT"},{5,"E100"},{330,"2F"},{100,"AcDbEntity"},{67,"1"},{8,"0"},{100,"AcDbBlockReference"},{2,"UNRESOLVED-ON-PURPOSE"},{66,"1"},{10,"1"},{20,"2"},{30,"3"},
        {0,"ATTRIB"},{5,"E101"},{330,"E100"},{100,"AcDbEntity"},{67,"1"},{8,"0"},{100,"AcDbText"},{10,"0"},{20,"0"},{40,"1"},{1,"attribute-é"},{100,"AcDbAttribute"},{2,"TAG"},{70,"0"},
        {0,"SEQEND"},{5,"E102"},{330,"E100"},{100,"AcDbEntity"},{67,"1"},{8,"0"},
        {0,"VENDOR_ENTITY"},{5,"F100"},{330,"1F"},{100,"AcDbEntity"},{8,"0"},{100,"VendorData"},{1,"raw-é"},{160,"4294967296"},{10,"-0.0"},{999,"raw tail"},
        {0,"DIMENSION"},{5,"F200"},{330,"1F"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbDimension"},{70,"7"},
        {0,"POINT"},{5,"F201"},{330,"1F"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"8"},{20,"9"},{0,"ENDSEC"},
        {0,"SECTION"},{2,"ENTITIES"},{0,"ENDSEC"},
        {0,"SECTION"},{2,"CUSTOM_SECTION"},{0,"POINT"},{5,"FFFF"},{1,"not an entity callback"},{0,"ENDSEC"},{0,"EOF"}});return t;
}
bool entitiesOriginalWire(const std::filesystem::path& path,bool binary,const Tags& tags,const dx_data& data,std::size_t& spans) {
    const auto wire=attdefWireTags(path,binary);if(wire.size()!=tags.size())return false;
    const auto& scopes=DxIfaceTableSourceTestAccess::entitiesSources(data);if(scopes.size()!=2u)return false;
    for(const auto& owner:scopes) {
        if(!owner.physical)return false;const auto& s=*owner.physical;
        const auto begin=s.beginMarker.ordinal-1u,end=s.endMarker.ordinal-1u;
        if(begin>=wire.size()||end>=wire.size()||wire[begin].code!=0||wire[end].text!="ENDSEC"
            ||wire[begin].begin!=s.beginMarker.beginOffset||wire[begin].end!=s.beginMarker.endOffset
            ||wire[end].begin!=s.endMarker.beginOffset||wire[end].end!=s.endMarker.endOffset)return false;
        for(const auto& r:s.records) {
            const auto first=r.marker.ordinal-1u;
            if(first>=wire.size()||wire[first].text!=r.name||wire[first].begin!=r.marker.beginOffset||wire[first].end!=r.marker.endOffset)return false;
            if(r.groups.size()!=r.spans.size()||r.groups.size()!=r.rawValues.size())return false;
            for(std::size_t j=0;j<r.groups.size();++j) {
                const auto i=first+j+1u;if(i>=wire.size())return false;const auto& w=wire[i];const auto& v=r.groups[j];const auto& p=r.spans[j];
                if(w.code!=tags[i].code||v.code()!=w.code||p.ordinal!=i+1u||p.beginOffset!=w.begin||p.endOffset!=w.end
                    ||(binary?!r.rawValues[j].empty():r.rawValues[j]!=tags[i].value))return false;
                if(w.kind==AttdefWireTag::Integer) {
                    if((v.type()!=DRW_Variant::INTEGER&&v.type()!=DRW_Variant::INTEGER64)||w.integer!=std::stoll(tags[i].value)
                        ||(v.type()==DRW_Variant::INTEGER64?v.i64_val():v.i_val())!=w.integer)return false;
                }else if(w.kind==AttdefWireTag::Real) {
                    if(v.type()!=DRW_Variant::DOUBLE)return false;const auto d=v.d_val(),original=std::stod(tags[i].value);
                    if(std::memcmp(&w.real,&d,sizeof(double))||std::memcmp(&d,&original,sizeof(double)))return false;
                }else if(v.type()!=DRW_Variant::STRING||!v.content.s||*v.content.s!=w.text||w.text!=tags[i].value)return false;
                ++spans;
            }
            const auto next=r.endOrdinal-1u;if(next>=wire.size()||r.endOffset!=wire[next].begin)return false;
        }
    }
    return scopes[0].physical->records.size()==12u&&scopes[0].physical->beginMarker.ordinal==slot(tags,5,"101")
        &&scopes[1].physical->records.empty()&&scopes[1].physical->beginMarker.ordinal==scopes[1].physical->endMarker.ordinal;
}
bool entitiesCollectorControls(long& allocations) {
    struct Counter final:DxfReadRecordObserver {
        unsigned calls=0u,comments=0u;
        void observe(const DxfReadRecordView& v) noexcept override {++calls;if(v.ignoredComment)++comments;}
    };
    {
        Counter previous;std::istringstream input("0\nPOINT\n5\n101\n10\n1.25\n999\ninside\n0\nENDSEC\n0\nEOF\n");
        auto budget=std::make_shared<DxfReadRetainedBudget>(1024u*1024u);
        auto reader=std::make_unique<dxfReaderAscii>(&input);reader->setVersion("AC1032",true);reader->setRetainedByteBudget(budget);
        reader->setIgnoreComments(true);reader->exchangeRecordObserver(&previous);int code=0;
        if(!reader->readRec(&code)||code!=0)return false;
        {
            dxfEntitiesSource::Capture capture(*reader);capture.start({});
            while(capture.source().status==DRW_DxfEntitiesSource::Status::Collecting&&reader->readRec(&code)){}
            if(capture.source().status!=DRW_DxfEntitiesSource::Status::Complete||capture.source().groups!=3u
                ||capture.source().ignoredComments!=1u||previous.calls!=5u||previous.comments!=1u)return false;
            capture.detach();if(reader->exchangeRecordObserver(&previous)!=&previous)return false;
        }
        if(!reader->readRec(&code)||code!=0||previous.calls!=6u)return false;reader.reset();if(budget->used()!=0u)return false;
    }
    const std::string payload(128u,'x');bool calibrated=false;
    for(long fault=0;fault<100&&!calibrated;++fault) {
        Counter previous;std::istringstream input("0\nPOINT\n");auto budget=std::make_shared<DxfReadRetainedBudget>(1024u*1024u);
        auto reader=std::make_unique<dxfReaderAscii>(&input);reader->setVersion("AC1032",true);reader->setRetainedByteBudget(budget);
        reader->exchangeRecordObserver(&previous);int code=0;if(!reader->readRec(&code)||code!=0)return false;
        const auto initial=budget->used();
        DxfReadRecordView id;id.code=5;id.ordinal=2u;id.beginOffset=8u;id.endOffset=14u;id.text=id.raw="101";
        DxfReadRecordView text;text.code=1;text.ordinal=3u;text.beginOffset=14u;text.endOffset=145u;text.text=text.raw=payload;
        DxfReadRecordView end;end.code=0;end.ordinal=4u;end.beginOffset=145u;end.endOffset=154u;end.text=end.raw="ENDSEC";
        {
            dxfEntitiesSource::Capture capture(*reader);
            {SymbolAllocationFailure schedule(fault);capture.start({});capture.observe(id);capture.observe(text);capture.observe(end);}
            if(capture.source().status==DRW_DxfEntitiesSource::Status::Complete)calibrated=true;
            else {if(capture.source().status!=DRW_DxfEntitiesSource::Status::Allocation)return false;++allocations;}
        }
        if(reader->exchangeRecordObserver(&previous)!=&previous||budget->used()!=initial)return false;
        reader.reset();if(budget->used()!=0u)return false;
    }
    return calibrated&&allocations>0;
}
bool entitiesPublicMemoryControls(const std::filesystem::path& path,unsigned& routes) {
    struct Consumer final:dx_iface {
        unsigned points=0u,calls=0u;bool badOrder=false;DRW_DxfEntitiesSource snapshot;
        void addHeader(const DRW_Header*) override {}
        void addPoint(const DRW_Point&) override {++points;}
        bool wantsDxfEntitiesSource() const override {return true;}
        void addDxfEntitiesSource(const DRW_DxfEntitiesSource& s) override {if(points!=1u)badOrder=true;++calls;snapshot=s;}
    } consumer;
    const auto document=[](const std::string& v) {
        Tags t{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,v},{0,"ENDSEC"},{0,"SECTION"},{2,"ENTITIES"},
            {0,"POINT"},{5,"101"},{8,"0"},{10,"1"},{20,"2"},{0,"ENDSEC"},{0,"EOF"}};return t;
    };
    dxfRW reader(path.string().c_str());std::uint64_t previous=0u;
    for(const std::string v:{"AC1009","AC1015","AC1021","AC1024","AC1027","AC1032"})for(bool crlf:{false,true}) {
        std::ostringstream out;const auto newline=crlf?"\r\n":"\n";for(const auto& t:document(v))out<<t.code<<newline<<t.value<<newline;
        auto input=out.str();consumer.points=consumer.calls=0u;
        if(!reader.readAscii(&consumer,false,input)||consumer.points!=1u||consumer.calls!=1u||consumer.badOrder
            ||consumer.snapshot.status!=DRW_DxfEntitiesSource::Status::Complete||consumer.snapshot.binary
            ||consumer.snapshot.scopeId<=previous||consumer.snapshot.records.size()!=1u)return false;
        const auto marker=std::string("0")+newline+"POINT"+newline;
        if(consumer.snapshot.beginMarker.beginOffset!=input.find(marker)
            ||consumer.snapshot.endMarker.beginOffset!=input.rfind(std::string("0")+newline+"ENDSEC"+newline))return false;
        previous=consumer.snapshot.scopeId;++routes;
    }
    if(!write(path,document("AC1032"),true))return false;consumer.points=consumer.calls=0u;
    if(!reader.read(&consumer,false)||consumer.calls!=1u||!consumer.snapshot.binary)return false;++routes;
    std::ostringstream out;for(const auto& t:document("AC1032"))out<<t.code<<'\n'<<t.value<<'\n';auto input=out.str();consumer.points=consumer.calls=0u;
    if(!reader.readAscii(&consumer,false,input)||consumer.calls!=1u||consumer.snapshot.binary||consumer.badOrder)return false;++routes;
    return routes==14u;
}
bool rawEntityProxyClassControls(const std::filesystem::path& path,unsigned& profiles,unsigned& retries) {
    struct Consumer final:dx_iface {
        unsigned depth=0u,points=0u;std::vector<DRW_RawDxfObject> raw;std::vector<DRW_Circle> circles;
        void addHeader(const DRW_Header*) override {}
        void addBlock(const DRW_Block&) override {++depth;}
        void addBlockEnd(const DRW_Block&) override {}
        void endBlock() override {--depth;}
        void addPoint(const DRW_Point&) override {++points;}
        void addCircle(const DRW_Circle& c) override {circles.push_back(c);}
        void addRawDxfEntity(const DRW_RawDxfObject& r) override {raw.push_back(r);}
    };
    // Locally authored recipe, same existing OP_CIRCLE framing as the in-tree
    // proxy controls; this checks scope separation, not independent CAD/kernel
    // correctness or additional proxy primitive admission.
    std::string recipe(8u,'\0');const auto word=[&](std::uint64_t n,unsigned bytes){for(unsigned i=0;i<bytes;++i)recipe.push_back(static_cast<char>(n>>(8u*i)));};
    word(64u,4u);word(2u,4u);for(double d:{2.,3.,4.,5.,0.,0.,1.}) {std::uint64_t bits=0;std::memcpy(&bits,&d,sizeof(d));word(bits,8u);}
    std::string hex;constexpr char digits[]="0123456789ABCDEF";
    for(unsigned char c:recipe){hex+=digits[c>>4u];hex+=digits[c&15u];}
    const auto document=[](const std::string& version,const Tags& row,bool block) {
        Tags t{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"},{0,"SECTION"},{2,block?"BLOCKS":"ENTITIES"}};
        if(block)t.insert(t.end(),{{0,"BLOCK"},{5,"B1"},{330,"B100"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},{2,"CELL"},{70,"0"},{10,"0"},{20,"0"}});
        t.push_back({0,"VENDOR_ENTITY"});t.insert(t.end(),row.begin(),row.end());
        t.insert(t.end(),{{0,"POINT"},{5,"C2"},{330,"B100"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"0"},{20,"0"}});
        if(block)t.insert(t.end(),{{0,"ENDBLK"},{5,"B2"},{330,"B100"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"}});
        t.insert(t.end(),{{0,"ENDSEC"},{0,"EOF"}});return t;
    };
    const auto exact=[](const DRW_RawDxfObject& raw,const Tags& row,bool binary) {
        if(raw.handle!=0xC1u||raw.parentHandle!=0xB100u||raw.groups.size()!=row.size()||raw.rawValues.size()!=row.size()||raw.hasRawValues==binary)return false;
        for(std::size_t i=0;i<row.size();++i) {
            const auto& v=raw.groups[i];const auto& t=row[i];if(v.code()!=t.code||(binary?!raw.rawValues[i].empty():raw.rawValues[i]!=t.value))return false;
            if(v.type()==DRW_Variant::STRING){if(!v.content.s||*v.content.s!=t.value)return false;}
            else if(v.type()==DRW_Variant::INTEGER64){if(v.i64_val()!=std::stoll(t.value))return false;}
            else if(v.type()!=DRW_Variant::INTEGER||v.i_val()!=std::stoi(t.value))return false;
        }
        return true;
    };
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true})for(bool block:{false,true}) {
        const int countCode=version=="AC1021"?92:160;
        const auto rowFor=[&](unsigned mode) {
            Tags row{{5,"C1"},{330,"B100"}};if(mode!=5u)row.push_back({100,"AcDbEntity"});row.push_back({8,"HOST"});
            if(mode==3u)row.insert(row.end(),{{102,"{OPAQUE"},{100,"VendorData"},{160,"4294967296"},{92,"-1"},{310,hex},{102,"}"}});
            if(mode==2u||mode==3u||mode==4u||mode==5u)row.insert(row.end(),{{countCode,"72"},{310,hex}});
            if(mode==6u)row.insert(row.end(),{{countCode,"72"},{310,hex.substr(0,72u)}});
            if(mode!=5u&&mode!=8u)row.push_back({100,"VendorData"});
            if(mode==8u)row.push_back({101,"Embedded Object"});
            if(mode==4u)row.push_back({100,"AcDbEntity"}); // cannot reopen a foreign subclass
            if(mode==0u||mode==2u||mode==4u)row.push_back({160,"4294967296"});
            if(mode==1u)row.insert(row.end(),{{92,"-1"},{310,hex}});
            if(mode==6u)row.push_back({310,hex.substr(72u)});
            if(mode==7u||mode==8u)row.insert(row.end(),{{countCode,"72"},{310,hex}});
            return row;
        };
        for(unsigned mode=0;mode<9u;++mode) {
            const auto row=rowFor(mode);const auto tags=document(version,row,block);if(!write(path,tags,binary))return false;
            dxfRW reader(path.string().c_str());Consumer receiver;const auto circles=mode>=2u&&mode<=5u?1u:0u;
            if(!reader.read(&receiver,false)||receiver.depth||receiver.points!=1u||receiver.raw.size()!=1u
                ||receiver.circles.size()!=circles||!exact(receiver.raw.front(),row,binary)) {
                std::cerr<<"raw proxy class="<<version<<'/'<<binary<<'/'<<block<<'/'<<mode<<" circles="<<receiver.circles.size()<<'\n';return false;
            }
            if(circles&&(receiver.circles.front().basePoint.x!=2.||receiver.circles.front().basePoint.y!=3.
                ||receiver.circles.front().basePoint.z!=4.||receiver.circles.front().radious!=5.||receiver.circles.front().layer!="HOST"))return false;
            ++profiles;
        }
        Tags invalid{{5,"C1"},{330,"B100"},{100,"AcDbEntity"},{8,"HOST"},{countCode,"67108865"},{100,"VendorData"}};
        if(!write(path,document(version,invalid,block),binary))return false;dxfRW reader(path.string().c_str());Consumer failed;
        if(reader.read(&failed,false)||failed.depth||!failed.raw.empty()||failed.points||!failed.circles.empty())return false;
        const auto fixed=rowFor(0u);if(!write(path,document(version,fixed,block),binary))return false;Consumer retry;
        if(!reader.read(&retry,false)||retry.depth||retry.points!=1u||retry.raw.size()!=1u||!retry.circles.empty()||!exact(retry.raw.front(),fixed,binary))return false;
        ++retries;
    }
    return profiles==144u&&retries==16u;
}
bool entitiesSourceTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::EntitiesSourceState;using B=dx_iface::TableBindingState;
    const auto path=dir/"entities-source.dxf";
    const auto complete=[](const dx_data& d){const auto r=dx_iface::entitiesSourceReport(d);return r.state==S::TransportComplete&&!r.outputEligible;};
    unsigned routes=0u,refusals=0u,aggregate=0u,primitive=0u,contexts=0u,exceptions=0u,tableJoins=0u,aborted=0u,legacyRoutes=0u,memoryRoutes=0u,proxyProfiles=0u,proxyRetries=0u;
    long allocations=0,collectorAllocations=0;std::size_t spans=0u;
    if(!entitiesCollectorControls(collectorAllocations)) {std::cerr<<"ENTITIES collector component\n";return false;}
    if(!entitiesPublicMemoryControls(path,memoryRoutes)) {std::cerr<<"ENTITIES public memory controls\n";return false;}
    if(!rawEntityProxyClassControls(path,proxyProfiles,proxyRetries))return false;
    for(const std::string v:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0;encoding<3u;++encoding) {
        const auto tags=entitiesSourceDocument(v);EntitiesProbe reader;dx_data data;
        if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!complete(data)||reader.premature
            ||!entitiesOriginalWire(path,encoding==2u,tags,data,spans)) {std::cerr<<"ENTITIES matrix="<<v<<'/'<<encoding<<'\n';return false;}
        const std::vector<std::string> expected{"point","point","line","polyline","insert","raw","point","source","source"};
        if(reader.events!=expected)return false;
        reader.optIn=false;reader.events.clear();dx_data legacy;
        if(!reader.fileImport(path.string(),&legacy,false)||dx_iface::entitiesSourceReport(legacy).state!=S::NotCaptured)return false;
        auto ordinary=expected;ordinary.resize(ordinary.size()-2u);if(reader.events!=ordinary)return false;
        ++routes;
    }
    struct Fault {const char* label;std::function<void(DRW_DxfEntitiesSource&)> alter;};
    const std::vector<Fault> faults{
        {"incomplete status",[](auto& s){s.status=DRW_DxfEntitiesSource::Status::Collecting;}},
        {"zero scope",[](auto& s){s.scopeId=0u;}},{"wrong version",[](auto& s){s.version=DRW::AC1015;}},
        {"begin ordinal",[](auto& s){++s.beginMarker.ordinal;}},{"begin comment",[](auto& s){s.beginMarker.ignoredComment=true;}},
        {"end ordinal",[](auto& s){++s.endMarker.ordinal;}},{"span ordinal",[](auto& s){++s.records[0].spans[0].ordinal;}},
        {"span bound",[](auto& s){s.records[0].spans[0].endOffset=s.records[0].spans[0].beginOffset;}},
        {"parallel cardinality",[](auto& s){s.records[0].rawValues.pop_back();}},
        {"group counter",[](auto& s){++s.groups;}},{"comment counter",[](auto& s){++s.ignoredComments;}},
        {"forged scalar",[](auto& s){s.records[0].groups[0]=DRW_Variant(5,DRW_Coord(1,2,3));}},
        {"embedded marker",[](auto& s){s.records[0].groups[0]=DRW_Variant(0,std::string("POINT"));}}
    };
    for(bool binary:{false,true}) {
        const auto tags=entitiesSourceDocument("AC1032");if(!write(path,tags,binary))return false;
        EntitiesProbe reader;
        for(const auto& f:faults) {
            reader.alter=[&](auto& s){if(!s.records.empty())f.alter(s);};dx_data failed;
            if(!reader.fileImport(path.string(),&failed,false)||dx_iface::entitiesSourceReport(failed).state!=S::CaptureFailed) {std::cerr<<"ENTITIES carrier fault="<<f.label<<'\n';return false;}
            reader.alter={};dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++refusals;
        }
        dx_data baseline;if(!reader.fileImport(path.string(),&baseline,false)||!complete(baseline))return false;
        const auto measured=dx_iface::entitiesSourceReport(baseline);
        const std::array<std::size_t,5> caps{measured.sections,measured.chargedRecords,measured.groups,measured.bytes,measured.work};
        for(unsigned which=0;which<caps.size();++which)for(bool exact:{false,true}) {
            dx_data data;A::entitiesLimit(data,which,caps[which]-(exact?0u:1u));
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::entitiesSourceReport(data).state!=(exact?S::TransportComplete:S::CaptureFailed)) {std::cerr<<"ENTITIES aggregate="<<which<<'/'<<exact<<'\n';return false;}++aggregate;
        }
        auto one=tags;const auto first=slot(one,2,"ENTITIES"),second=slot(Tags(one.begin()+static_cast<std::ptrdiff_t>(first+1u),one.end()),2,"ENTITIES")+first+1u;
        one.erase(one.begin()+static_cast<std::ptrdiff_t>(second-1u),one.begin()+static_cast<std::ptrdiff_t>(second+2u));
        if(!write(path,one,binary))return false;dx_data single;if(!reader.fileImport(path.string(),&single,false)||!complete(single))return false;
        const auto own=reader.measured;
        for(unsigned which=0;which<4u;++which)for(bool exact:{false,true}) {
            reader.limits={};if(which==0u)reader.limits.records=own[which]-(exact?0u:1u);if(which==1u)reader.limits.groups=own[which]-(exact?0u:1u);
            if(which==2u)reader.limits.bytes=own[which]-(exact?0u:1u);if(which==3u)reader.limits.work=own[which]-(exact?0u:1u);
            dx_data data;if(!reader.fileImport(path.string(),&data,false)||dx_iface::entitiesSourceReport(data).state!=(exact?S::TransportComplete:S::CaptureFailed))return false;++primitive;
        }
        reader.limits={};bool calibrated=false;
        for(long fault=0;fault<600&&!calibrated;++fault) {
            reader.failCopy=fault;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;
            const auto r=dx_iface::entitiesSourceReport(data);
            if(r.state==S::TransportComplete)calibrated=true;else {if(r.state!=S::CaptureFailed||r.outputEligible)return false;++allocations;}
            reader.failCopy=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;
        }
        if(!calibrated)return false;
        dx_data copied;A::copyBindingContext(copied,single);if(dx_iface::entitiesSourceReport(copied).state!=S::ContextMismatch)return false;++contexts;
        reader.mutateAfter=true;dx_data frozen;
        if(!reader.fileImport(path.string(),&frozen,false)||!complete(frozen)
            ||A::entitiesSources(frozen).front().physical->records.front().name!="POINT")return false;
        reader.mutateAfter=false;++contexts;
        for(int phase=0;phase<3;++phase) {
            reader.throwSource=phase<2?phase:-1;reader.throwPoint=phase==2;dx_data failed;bool threw=false;
            try{(void)reader.fileImport(path.string(),&failed,false);}catch(const std::runtime_error&){threw=true;}
            if(!threw||dx_iface::entitiesSourceReport(failed).state==S::TransportComplete)return false;
            reader.throwSource=-1;reader.throwPoint=false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++exceptions;
        }
        reader.disableDuringPoint=true;reader.calls=0u;dx_data latched;
        if(!write(path,tags,binary)||!reader.fileImport(path.string(),&latched,false)||!complete(latched)||reader.calls!=1u
            ||A::entitiesSources(latched).size()!=1u)return false;
        reader.disableDuringPoint=false;reader.optIn=true;++contexts;
        dx_data appended;if(!reader.fileImport(path.string(),&appended,false)||!reader.fileImport(path.string(),&appended,false)
            ||dx_iface::entitiesSourceReport(appended).state!=S::ContextMismatch)return false;
        ++contexts;
        dx_data stale;if(!reader.fileImport(path.string(),&stale,false)||!complete(stale))return false;
        reader.addDxfEntitiesSource(*A::entitiesSources(stale).front().physical);
        if(dx_iface::entitiesSourceReport(stale).state!=S::CaptureFailed)return false;++contexts;
        std::weak_ptr<const DRW_DxfEntitiesSource> weak;
        {dx_data owned;if(!reader.fileImport(path.string(),&owned,false)||!complete(owned))return false;weak=A::entitiesSources(owned).front().physical;}
        if(!weak.expired())return false;++contexts;
        for(unsigned fault=0;fault<5u;++fault) {
            auto broken=one;
            if(fault==0u) {const auto at=slot(broken,2,"CUSTOM_SECTION")-2u;broken.erase(broken.begin()+static_cast<std::ptrdiff_t>(at));}
            if(fault==1u)broken.resize(slot(broken,5,"F201")+1u);
            if(fault==2u) {const auto at=blockField(broken,"F100",100)+2u;broken.insert(broken.begin()+static_cast<std::ptrdiff_t>(at),{160,"4294967296"});} // actual common proxy length, not vendor payload
            if(fault==3u)eraseBlockRecord(broken,"E102");
            if(fault==4u)broken.erase(broken.end()-2);
            reader.calls=0u;dx_data failed;if(!write(path,broken,binary))return false;
            if(reader.fileImport(path.string(),&failed,false)||dx_iface::entitiesSourceReport(failed).state!=S::ImportFailed
                ||reader.calls!=(fault==4u?1u:0u)) {std::cerr<<"ENTITIES abort="<<fault<<" reports="<<reader.calls<<'\n';return false;}
            dx_data retry;if(!write(path,one,binary)||!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++aborted;
        }
        const auto binding=bindingSource("AC1032");if(!write(path,binding,binary))return false;Probe paired;dx_data table;
        if(!paired.fileImport(path.string(),&table,false)||dx_iface::tableContentBindingReport(table).state!=B::Bound
            ||dx_iface::tableContentBindingReport(table).entitiesPhysicalTablesMatched!=1u)return false;
        ++tableJoins;
        reader.alter=[](auto& s){for(auto& r:s.records)if(r.name=="ACAD_TABLE")for(auto& g:r.groups)if(g.code()==10){g=DRW_Variant(10,99.0);return;}};
        dx_data changed;
        if(!reader.fileImport(path.string(),&changed,false)||!complete(changed)
            ||!dx_iface::tableContentBindingReport(changed).directRolesBound||dx_iface::tableContentBindingReport(changed).state!=B::EntitiesSourceMismatch)return false;
        ++tableJoins;
        reader.alter={};reader.optIn=false;dx_data absent;
        if(!reader.fileImport(path.string(),&absent,false)
            ||!dx_iface::tableContentBindingReport(absent).directRolesBound||dx_iface::tableContentBindingReport(absent).state!=B::EntitiesSourceMissing)return false;
        ++tableJoins;
        reader.optIn=true;
        for(unsigned fault=0;fault<3u;++fault) {
            reader.alter=[&](auto& s){for(auto& r:s.records)if(r.name=="ACAD_TABLE") {
                if(fault==0u)r.groups[0]=DRW_Variant(5,std::string("4F3"));
                if(fault==1u)r.groups[0]=DRW_Variant(5,std::string("4f2"));
                if(fault==2u)for(auto& g:r.groups)if(g.code()==330){g=DRW_Variant(330,std::string("B100"));break;}
            }};
            dx_data failed;if(!reader.fileImport(path.string(),&failed,false)||!complete(failed))return false;
            const auto r=dx_iface::tableContentBindingReport(failed);
            if(!r.directRolesBound||r.state!=B::EntitiesSourceMismatch||r.outputEligible)return false;
            reader.alter={};dx_data retry;if(!reader.fileImport(path.string(),&retry,false)
                ||dx_iface::tableContentBindingReport(retry).state!=B::Bound)return false;
            ++tableJoins;
        }
        dx_data pinned;if(!reader.fileImport(path.string(),&pinned,false))return false;
        auto& owner=A::mutableEntitiesSources(pinned).front();const auto saved=owner.physical;
        owner.physical=std::make_shared<const DRW_DxfEntitiesSource>(*saved);
        if(dx_iface::tableContentBindingReport(pinned).state!=B::EntitiesSourceMismatch)return false;
        owner.physical=saved;if(dx_iface::tableContentBindingReport(pinned).state!=B::Bound)return false;++tableJoins;
    }
    for(const std::string v:{"AC1009","AC1015"}) {
        Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,v},{0,"ENDSEC"},{0,"SECTION"},{2,"ENTITIES"},
            {0,"POINT"},{5,"101"},{8,"0"},{10,"1"},{20,"2"},{0,"ENDSEC"},{0,"EOF"}};
        EntitiesProbe reader;reader.force=true;dx_data data;
        if(!write(path,tags,false)||!reader.fileImport(path.string(),&data,false)||!complete(data))return false;++legacyRoutes;
    }
    std::cout<<"ENTITIES source: routes="<<routes<<"; original scalar spans="<<spans<<"; malformed carrier/retries="<<refusals
        <<"; aggregate boundaries="<<aggregate<<"; collector boundaries="<<primitive<<"; observed copy allocation faults="<<allocations
        <<"; copied/alias/latched/appended/stale/teardown contexts="<<contexts<<"; exceptions/retries="<<exceptions<<"; selected TABLE physical joins="<<tableJoins
        <<"; aborted section/import retries="<<aborted<<"; legacy ASCII routes="<<legacyRoutes<<"; isolated collector allocation faults="<<collectorAllocations
        <<"; public readAscii/file-reuse routes="<<memoryRoutes<<"; raw proxy class profiles="<<proxyProfiles<<"; genuine common-count refusals/retries="<<proxyRetries<<'\n';
    return routes==12u&&spans==1344u&&refusals==faults.size()*2u&&aggregate==20u&&primitive==16u&&allocations>0&&contexts==12u
        &&exceptions==6u&&tableJoins==14u&&aborted==10u&&legacyRoutes==2u&&collectorAllocations>0&&memoryRoutes==14u&&proxyProfiles==144u&&proxyRetries==16u;
}
