// SPDX-License-Identifier: GPL-2.0-or-later
BlockFixture mixedBlockChildrenFixture(const std::string& version) {
    auto fixture=blockFixture(version,0u,true);Tags children;
    const auto add=[&](const char* name,const Tags& tags){children.push_back({0,name});children.insert(children.end(),tags.begin(),tags.end());};
    auto first=body(0u);first.insert(first.begin()+2,{999,"original comment before application"});add("ATTDEF",first);
    add("VENDOR_ENTITY",{{5,"C1"},{330,"B0"},{100,"AcDbEntity"},{8,"unknown-layer"},
        {102,"{OPAQUE"},{5,"DEAD"},{330,"FACE"},{999,"opaque comment"},{102,"}"},
        {320,"FFFFFFFFFFFFFFFF"},{version=="AC1021"?92:160,"3"},{310,"0102A3"},{1,"raw-\\U+00E9"}});
    add("POLYLINE",{{5,"D1"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDb2dPolyline"},{66,"1"},{70,"0"},{10,"0"},{20,"0"}});
    add("VERTEX",{{5,"D2"},{330,"D1"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbVertex"},{100,"AcDb2dVertex"},{10,"0"},{20,"0"},{70,"0"}});
    add("VERTEX",{{5,"D3"},{330,"D1"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbVertex"},{100,"AcDb2dVertex"},{10,"2"},{20,"3"},{70,"0"}});
    add("SEQEND",{{5,"D4"},{330,"D1"},{100,"AcDbEntity"},{8,"0"}});
    add("INSERT",{{5,"E1"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockReference"},{2,"RESOLVED"},{66,"1"},{10,"0"},{20,"0"}});
    auto attribute=body(0u);attribute[0].value="E2";attribute[1].value="E1";
    for(auto& tag:attribute)if(tag.code==100&&tag.value=="AcDbAttributeDefinition")tag.value="AcDbAttribute";
    attribute.erase(std::remove_if(attribute.begin(),attribute.end(),[](const Tag& t){return t.code==3&&t.value=="prompt-é";}),attribute.end());
    add("ATTRIB",attribute);add("SEQEND",{{5,"E3"},{330,"E1"},{100,"AcDbEntity"},{8,"0"}});
    auto last=body(0u);last[0].value="A2";last.push_back({999,"trailing original comment"});add("ATTDEF",last);
    const auto end=std::find_if(fixture.document.begin(),fixture.document.end(),[](const Tag& t){return t.code==0&&t.value=="ENDBLK";});
    fixture.document.insert(end,children.begin(),children.end());return fixture;
}

bool originalChildrenWire(const std::filesystem::path& path,bool binary,const BlockFixture& fixture,
    const DRW_DxfBlockChildrenSource& source,std::size_t& scalarSpans) {
    const auto wire=attdefWireTags(path,binary);
    if(wire.size()!=fixture.document.size()||source.binary!=binary||source.status!=DRW_DxfBlockChildrenSource::Status::Complete)return false;
    std::vector<std::size_t> markers;bool block=false;
    for(std::size_t i=0;i<wire.size();++i) {
        const auto& w=wire[i];const auto& t=fixture.document[i];
        if(w.code!=t.code)return false;
        if(w.kind==AttdefWireTag::Integer){if(w.integer!=std::stoll(t.value))return false;}
        else if(w.kind==AttdefWireTag::Real){if(w.real!=std::stod(t.value))return false;}
        else if(w.text!=t.value)return false;
        if(w.code!=0)continue;
        if(w.text=="BLOCK"){block=true;continue;}
        if(block){markers.push_back(i);if(w.text=="ENDBLK")break;}
    }
    if(markers.size()!=source.records.size()+1u)return false;
    const auto spanMatches=[](const DRW_DxfPhysicalRecordSpan& span,const AttdefWireTag& w,std::size_t index) {
        return span.ordinal==index+1u&&span.beginOffset==w.begin&&span.endOffset==w.end&&span.ignoredComment==(w.code==999);
    };
    if(!spanMatches(source.endMarker,wire[markers.back()],markers.back()))return false;
    std::size_t groups=0u,comments=0u;
    for(std::size_t r=0;r<source.records.size();++r) {
        const auto& record=source.records[r];const auto first=markers[r],end=markers[r+1u],n=end-first-1u;
        if(record.name!=wire[first].text||!spanMatches(record.marker,wire[first],first)
            ||record.endOrdinal!=end+1u||record.endOffset!=wire[end].begin
            ||record.groups.size()!=n||record.spans.size()!=n||record.rawValues.size()!=n)return false;
        for(std::size_t j=0;j<n;++j) {
            const auto& v=record.groups[j];const auto& w=wire[first+1u+j];const auto& t=fixture.document[first+1u+j];
            if(v.code()!=w.code||!spanMatches(record.spans[j],w,first+1u+j)
                ||(binary?!record.rawValues[j].empty():record.rawValues[j]!=t.value))return false;
            if(w.kind==AttdefWireTag::Integer) {
                if(v.type()==DRW_Variant::INTEGER64){if(v.i64_val()!=w.integer)return false;}
                else if(v.type()!=DRW_Variant::INTEGER||v.i_val()!=w.integer)return false;
            } else if(w.kind==AttdefWireTag::Real){if(v.type()!=DRW_Variant::DOUBLE||v.d_val()!=w.real)return false;}
            else if(v.type()!=DRW_Variant::STRING||!v.content.s||*v.content.s!=w.text)return false;
            ++groups;if(w.code==999)++comments;
        }
    }
    scalarSpans+=groups;return groups==source.groups&&comments==source.ignoredComments;
}

bool childObserverAllocationControls(const std::filesystem::path& path,long& faults) {
    using S=DRW_DxfBlockChildrenSource::Status;
    const Tags stream{{0,"LINE"},{5,"F1"},{999,"comment"},{1000,"\\U+00E9"},{160,"3"},{10,"1.25"},
        {0,"VENDOR_ENTITY"},{5,"F2"},{1000,"\\U+00F1"},{0,"ENDBLK"},{5,"F3"},{0,"ENDSEC"}};
    struct Count final:DxfReadRecordObserver {unsigned seen=0u,comments=0u;void observe(const DxfReadRecordView& v) noexcept override {++seen;if(v.ignoredComment)++comments;}};
    struct Inject final:DxfReadRecordObserver {
        dxfBlockChildren::Capture& capture;bool enabled;long remaining;
        Inject(dxfBlockChildren::Capture& c,bool e,long n):capture(c),enabled(e),remaining(n){}
        void observe(const DxfReadRecordView& v) noexcept override {
            if(enabled)allocationRemaining=remaining;
            capture.observe(v);
            if(enabled){remaining=allocationRemaining;allocationRemaining=-1;}
        }
    };
    for(bool binary:{false,true}) {
        if(!write(path,stream,binary))return false;
        const auto run=[&](unsigned phase,long schedule,long& allocations) {
            auto budget=std::make_shared<DxfReadRetainedBudget>(1048576u);bool ok=true;
            {
                std::ifstream input(path,std::ios::binary);if(binary)input.seekg(22);
                std::unique_ptr<dxfReader> reader;
                if(binary)reader=std::make_unique<dxfReaderBinary>(&input);else reader=std::make_unique<dxfReaderAscii>(&input);
                reader->setVersion("AC1032",true);reader->setCodePage("UTF-8");reader->setIgnoreComments(true);
                reader->setInputByteBudget(1048576u,binary?22u:0u);reader->setRetainedByteBudget(budget);
                Count previous;reader->exchangeRecordObserver(&previous);int code=0;
                if(!reader->readRec(&code)||code!=0||reader->takeString()!="LINE")return false;
                {
                    dxfBlockChildren::Capture capture(*reader);
                    if(phase==0u)allocationRemaining=schedule;
                    capture.start(0xB1u,0xB0u,{});
                    if(phase==0u){allocations=schedule-allocationRemaining;allocationRemaining=-1;}
                    Inject injection(capture,phase==1u,schedule);reader->exchangeRecordObserver(&injection);
                    while(reader->readRec(&code))if(code==1000) {
                        const auto text=reader->takeString();if(text!="é"&&text!="ñ")ok=false;
                    }
                    if(phase==1u)allocations=schedule-injection.remaining;
                    capture.detach();
                    if(reader->exchangeRecordObserver(nullptr)!=&previous)ok=false;
                    const auto& source=capture.source();
                    if(schedule==(std::numeric_limits<long>::max)()) {
                        ok=ok&&source.status==S::Complete&&source.records.size()==2u&&source.groups==7u
                            &&source.ignoredComments==1u&&source.endMarker.ordinal==10u
                            &&*source.records.front().groups[2].content.s=="\\U+00E9";
                    } else ok=ok&&source.status==S::Allocation;
                    ok=ok&&previous.seen==12u&&previous.comments==1u;
                }
                reader.reset();
            }
            return ok&&budget->used()==0u;
        };
        for(unsigned phase=0;phase<2u;++phase) {
            long count=0;
            if(!check(run(phase,(std::numeric_limits<long>::max)(),count)&&count>0&&count<1000,
                "observer seed/stream allocation calibration, original-before-take, chaining and byte lease release"))return false;
            for(long failure=0;failure<count;++failure) {
                long ignored=0;
                if(!check(run(phase,failure,ignored),"every observer allocation failure is sticky, detached and fully released"))return false;
                if(!run(phase,(std::numeric_limits<long>::max)(),ignored))return false;++faults;
            }
        }
    }
    return true;
}

bool blockChildrenSourceTests(const std::filesystem::path& path) {
    using A=DxIfaceTableSourceTestAccess;using S=DRW_DxfBlockChildrenSource::Status;
    unsigned profiles=0u,quotas=0u,readQuotas=0u,faults=0u,throws=0u,protocols=0u,facadeQuotas=0u,rootIdentities=0u,payloadOwners=0u;
    std::size_t spans=0u;long snapshots=0,observerFaults=0;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true})
     for(bool crlf:{false,true})for(unsigned shape=0;shape<3u;++shape) {
        if(binary&&crlf)continue;
        const auto fixture=shape==2u?mixedBlockChildrenFixture(version):blockFixture(version,shape,true);
        if(!write(path,fixture.document,binary,crlf))return false;
        BlockSourceProbe reader;reader.traceChildren=true;dx_data data;
        if(!check(reader.fileImport(path.string(),&data,false)&&A::childrenTransport(data),"actual original child stream import")) {
            std::cerr<<version<<" binary="<<binary<<" shape="<<shape<<" status="<<static_cast<unsigned>(reader.childrenAlias.status)
                <<" reports="<<reader.childReports<<" failed="<<A::childrenFailed(data)<<'\n';return false;
        }
        const auto* row=A::block(data);
        if(!check(originalChildrenWire(path,binary,fixture,*row->children,spans),"independent original child scalar values, markers, ordinals and exact byte spans"))return false;
        if(!check(row->children->records.size()==(shape==2u?10u:shape)
            &&row->children->ignoredComments==(shape==2u?3u:0u)
            &&reader.events==(shape==2u?std::vector<std::string>{"S","B","T","F","C","A","R","P","I","A","E"}
                :shape==1u?std::vector<std::string>{"S","B","T","F","C","A","E"}:std::vector<std::string>{"S","B","T","F","C","E"}),
            "physical compound/unknown inventory precedes staged callbacks without inferring child count from them"))return false;
        if(shape==2u) {
            const std::vector<std::string> names{"ATTDEF","VENDOR_ENTITY","POLYLINE","VERTEX","VERTEX","SEQEND","INSERT","ATTRIB","SEQEND","ATTDEF"};
            for(std::size_t i=0;i<names.size();++i)if(!check(row->children->records[i].name==names[i],"authored physical child/compound record order"))return false;
            const auto saved=*row->children->records.front().groups.front().content.s;
            reader.childrenAlias.records.front().groups.front().addString(5,"CHANGED");
            if(!check(*row->children->records.front().groups.front().content.s==saved,"immutable child inventory owns original scalars independently"))return false;
        }
        BlockSourceProbe plain;plain.childrenSelected=false;dx_data optout;
        if(!check(plain.fileImport(path.string(),&optout,false)&&plain.childReports==0u&&!A::block(optout)->children
            &&A::childrenUsage(optout)==std::array<std::size_t,4>{},"child facet opt-out has no inventory capture/charges"))return false;
        ++profiles;
    }
    for(bool binary:{false,true}) {
        const auto fixture=mixedBlockChildrenFixture("AC1032");if(!write(path,fixture.document,binary))return false;
        BlockSourceProbe baseline;dx_data measured;if(!baseline.fileImport(path.string(),&measured,false)||!A::childrenTransport(measured))return false;
        const auto usage=A::childrenUsage(measured);const auto captured=baseline.childrenAlias;
        for(bool owner:{false,true}) {
            BlockSourceProbe reader;reader.alterBegin=[=](DRW_Block& b){if(owner)b.parentHandle=0x99u;else b.handle=0x99u;};
            dx_data data;
            if(!check(reader.fileImport(path.string(),&data,false)&&reader.childReports==1u
                &&reader.childrenAlias.blockHandle==0xB1u&&reader.childrenAlias.blockRecord==0xB0u&&!A::childrenTransport(data),
                "child inventory root identity is original, never the altered consumer callback view"))return false;
            reader.alterBegin={};dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::childrenTransport(retry))return false;++rootIdentities;
        }
        for(unsigned dimension=0;dimension<4u;++dimension) {
            BlockSourceProbe reader;dx_data exact;A::childrenLimit(exact,dimension,usage[dimension]);
            if(!check(reader.fileImport(path.string(),&exact,false)&&A::childrenTransport(exact),"exact immutable child inventory quota"))return false;
            dx_data below;A::childrenLimit(below,dimension,usage[dimension]-1u);
            if(!check(reader.fileImport(path.string(),&below,false)&&A::childrenFailed(below)&&!A::childrenTransport(below),"one-below child snapshot quota fails closed"))return false;
            dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::childrenTransport(retry))return false;++quotas;
            const auto limit=dimension==0u?captured.records.size():dimension==1u?captured.bytes:dimension==2u?captured.groups:captured.work;
            const auto set=[&](BlockSourceProbe& p,std::size_t value){switch(dimension){case 0:p.childrenLimits.records=value;break;case 1:p.childrenLimits.bytes=value;break;case 2:p.childrenLimits.groups=value;break;case 3:p.childrenLimits.work=value;break;}};
            BlockSourceProbe raw;set(raw,limit);dx_data good;
            if(!check(raw.fileImport(path.string(),&good,false)&&A::childrenTransport(good),"exact reader child inventory ceiling"))return false;
            set(raw,limit-1u);dx_data bad;
            if(!check(raw.fileImport(path.string(),&bad,false)&&raw.childrenAlias.status==S::Quota&&A::childrenFailed(bad)
                &&!A::childrenTransport(bad),"own reader inventory ceiling refuses evidence without changing ordinary import"))return false;
            raw.childrenLimits={};dx_data recovered;if(!raw.fileImport(path.string(),&recovered,false)||!A::childrenTransport(recovered))return false;++readQuotas;
        }
        for(unsigned family=0;family<3u;++family) {
            constexpr std::size_t payload=16384u;
            BlockSourceProbe reader;reader.alterChildren=[=](DRW_DxfBlockChildrenSource& s) {
                auto& field=s.records.front().groups.front();
                if(family==0u){field=DRW_Variant(1000,std::string(payload,'q'));field.addInt(70,1);}
                if(family==1u){field=DRW_Variant(1004,std::vector<std::uint8_t>(payload,0xA5u));field.addDouble(10,1.25);}
                if(family==2u){field=DRW_Variant(1003,std::string(payload,'q'));field.setDwgRawLayerReference(0u,DRW::AC1032);field.content.s->assign("q");}
            };
            dx_data grown;
            if(!check(reader.fileImport(path.string(),&grown,false)&&A::childrenTransport(grown)
                &&A::childrenUsage(grown)[1]>=usage[1]+2u*(payload-16u),"all copied dormant string/binary/private raw-spelling owners grow the logical allowance"))return false;
            const auto charge=A::childrenUsage(grown)[1];dx_data exact;A::childrenLimit(exact,1u,charge);
            if(!check(reader.fileImport(path.string(),&exact,false)&&A::childrenTransport(exact),"hidden variant owner exact snapshot byte ceiling"))return false;
            dx_data below;A::childrenLimit(below,1u,charge-1u);
            if(!check(reader.fileImport(path.string(),&below,false)&&A::childrenFailed(below)&&!A::childrenTransport(below),"hidden variant owner one-below snapshot ceiling refuses before cloning"))return false;
            reader.alterChildren={};dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::childrenTransport(retry))return false;++payloadOwners;
        }
        for(unsigned fault=0;fault<14u;++fault) {
            BlockSourceProbe reader;reader.alterChildren=[=](DRW_DxfBlockChildrenSource& s){switch(fault){
                case 0:s.blockHandle=99u;break;case 1:s.blockRecord=99u;break;case 2:s.version=DRW::AC1015;break;
                case 3:s.status=S::Collecting;break;case 4:s.endMarker.ignoredComment=true;break;case 5:++s.endMarker.ordinal;break;
                case 6:s.records.erase(s.records.begin());break;case 7:s.records.front().spans.pop_back();break;
                case 8:++s.records.front().spans.front().ordinal;break;case 9:s.records.front().spans.front().endOffset=0u;break;
                case 10:s.records.front().rawValues.pop_back();break;case 11:s.records.front().groups.front()=DRW_Variant(10,DRW_Coord{});break;
                case 12:s.binary=!s.binary;break;case 13:++s.ignoredComments;break;}};
            dx_data bad;if(!check(reader.fileImport(path.string(),&bad,false)&&A::childrenFailed(bad)&&!A::childrenTransport(bad),"malformed/wrong-context child report is not retained as source transport"))return false;
            reader.alterChildren={};dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::childrenTransport(retry))return false;++faults;
        }
        for(int phase:{2,3,4,5,7}) {
            BlockSourceProbe reader;reader.throwPhase=phase;dx_data data;bool threw=false;
            try{(void)reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
            if(!check(threw&&reader.depth==0u&&!A::childrenTransport(data),"source/footer/child/cleanup failure invalidates original child candidate and balances scope"))return false;
            reader.throwPhase=0;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::childrenTransport(retry))return false;++throws;
        }
        for(unsigned fault=0;fault<6u;++fault) {
            BlockSourceProbe reader;if(fault==0u)reader.skipChildrenReport=true;if(fault==1u)reader.duplicateChildrenReport=true;
            auto document=fixture.document;
            if(fault==3u) {
                const auto footer=std::find_if(document.begin(),document.end(),[](const Tag& t){return t.code==0&&t.value=="ENDBLK";});
                std::find_if(footer,document.end(),[](const Tag& t){return t.code==100;})->value="BadEndClass";
            }
            if(fault==4u)document.insert(document.end()-1,{{0,"SECTION"},{2,"ENTITIES"},{0,"LINE"},{5,"not-hex"},{0,"ENDSEC"}});
            if(fault==5u)document=blockFixture("AC1032",1u,false,true).document;
            if(!write(path,document,binary))return false;
            dx_data data;const bool ok=reader.fileImport(path.string(),&data,false);
            if(fault==2u)reader.dx_iface::addDxfBlockChildrenSource(reader.childrenAlias);
            if(!check(!A::childrenTransport(data),"missing/duplicate/late, invalid/dropped scope and later input cannot qualify child inventory"))return false;
            if((fault==3u||fault==5u)&&!check(reader.childReports==0u,"invalid/dropped block publishes no child inventory"))return false;
            if(fault==3u||fault==4u){if(!check(!ok,"malformed footer or later input fails whole import"))return false;}
            else if(!ok)return false;
            reader.skipChildrenReport=reader.duplicateChildrenReport=false;if(!write(path,fixture.document,binary))return false;
            dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::childrenTransport(retry))return false;++protocols;
        }
        BlockSourceProbe calibration;calibration.allocationPhase=6;calibration.failAllocation=(std::numeric_limits<long>::max)();dx_data sample;
        if(!check(calibration.fileImport(path.string(),&sample,false)&&A::childrenTransport(sample)
            &&calibration.allocations>0&&calibration.allocations<2000,"child immutable snapshot allocation calibration"))return false;
        for(long failure=0;failure<calibration.allocations;++failure) {
            BlockSourceProbe reader;reader.allocationPhase=6;reader.failAllocation=failure;dx_data data;
            if(!check(reader.fileImport(path.string(),&data,false)&&A::childrenFailed(data)&&!A::childrenTransport(data),"child snapshot allocation failure stays unqualified"))return false;
            reader.allocationPhase=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::childrenTransport(retry))return false;++snapshots;
        }
        const auto small=blockFixture("AC1032",1u,true);if(!write(path,small.document,binary))return false;
        dxfRW codec(path.string().c_str());std::array<std::size_t,2> bytes{},nodes{};
        for(unsigned selected=0;selected<2u;++selected)for(bool node:{false,true}) {
            const auto attempt=[&](std::size_t limit) {
                codec.setDxfReadRetainedByteBudget(node?1048576u:limit);codec.setDxfReadTypedVariantNodeBudget(node?limit:10000u);
                BlockSourceProbe receiver;receiver.forceChildrenTransport=selected!=0u;
                bool ok=false;try{ok=codec.read(&receiver,false);}catch(...){}
                return ok&&(!selected||(receiver.childReports==1u&&receiver.childrenAlias.status==S::Complete));
            };
            std::size_t low=0u,high=node?10000u:1048576u;
            if(!attempt(high))return false;
            while(low<high){const auto middle=low+(high-low)/2u;if(attempt(middle))high=middle;else low=middle+1u;}
            if(!check(low&&attempt(low)&&!attempt(low-1u),"shared reader byte/node exact/one-below child capture boundary and same-codec recovery"))return false;
            (node?nodes:bytes)[selected]=low;++facadeQuotas;
        }
        if(!check(bytes[0]<bytes[1]&&nodes[0]<nodes[1],"child source opt-out adds no raw owner/variant charges"))return false;
    }
    if(!childObserverAllocationControls(path,observerFaults))return false;
    std::cout<<"BLOCK original children profiles="<<profiles<<"; original scalar spans="<<spans<<"; snapshot quota boundaries/retries="<<quotas
        <<"; reader quota boundaries/retries="<<readQuotas<<"; report faults/retries="<<faults<<"; callback faults/retries="<<throws
        <<"; protocol refusals/retries="<<protocols<<"; snapshot allocation faults/retries="<<snapshots
        <<"; observer allocation faults/retries="<<observerFaults<<"; same-codec quota boundaries="<<facadeQuotas
        <<"; original root identity faults/retries="<<rootIdentities<<"; hidden payload owner boundaries/retries="<<payloadOwners<<'\n';
    return profiles==36u&&spans==2712u&&quotas==8u&&readQuotas==8u&&faults==28u&&throws==10u&&protocols==12u&&facadeQuotas==8u&&rootIdentities==4u&&payloadOwners==6u;
}
