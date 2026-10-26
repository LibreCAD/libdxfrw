// SPDX-License-Identifier: GPL-2.0-or-later
class RawScopeProbe final:public dx_iface {
public:
    std::vector<DRW_RawDxfObject> entities,objects;
    std::vector<DRW_RawDxfSection> sections;
    std::vector<DRW_ProxyEntity> proxyEntities;
    std::vector<DRW_ProxyObject> proxyObjects;
    std::vector<std::uint32_t> points;
    DRW_DxfBlockChildrenSource children;
    dxfRW* output=nullptr;
    bool written=true,inventoryMatched=false;
    unsigned depth=0u,reports=0u;
    void addHeader(const DRW_Header*) override {}
    void addBlock(const DRW_Block&) override {++depth;}
    void addBlockEnd(const DRW_Block&) override {}
    void endBlock() override {--depth;}
    bool wantsDxfBlockChildrenSource() const override {return true;}
    void addDxfBlockChildrenSource(const DRW_DxfBlockChildrenSource& s) override {children=s;++reports;}
    void addRawDxfEntity(const DRW_RawDxfObject& s) override {entities.push_back(s);}
    void addRawDxfObject(const DRW_RawDxfObject& s) override {objects.push_back(s);}
    void addRawDxfSection(const DRW_RawDxfSection& s) override {sections.push_back(s);}
    void addProxyEntity(const DRW_ProxyEntity& s) override {proxyEntities.push_back(s);}
    void addProxyObject(const DRW_ProxyObject& s) override {proxyObjects.push_back(s);}
    void addPoint(const DRW_Point& s) override {points.push_back(s.handle);}
    void writeHeader(DRW_Header&) override {}
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeAppId() override {}
    void writeViews() override {}
    void writeUCSs() override {}
    void writeObjects() override {if(output)for(auto& object:objects)if(!output->writeRawDxfObject(&object))written=false;}
    bool finalizeDxfWrite() override {
        if(!output)return true;
        inventoryMatched=output->dxfWriteRecordHasIdentity(0xC1u,"VENDOR_OBJECT","OBJECTS")
            &&output->dxfWriteRecordHandleCount(0xC1u)==1u&&output->dxfWriteRecordHandleCount(0xC2u)==0u;
        return written&&inventoryMatched;
    }
};

BlockFixture rawScopeDocument(const std::string& version,unsigned context,const char* name,const Tags& row,bool following=true) {
    const Tags point{{0,"POINT"},{5,"C2"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"0"},{20,"0"},{30,"0"}};
    BlockFixture f;
    if(context==0u) {
        f=blockFixture(version,0u,false);
        const auto end=std::find_if(f.document.begin(),f.document.end(),[](const Tag& t){return t.code==0&&t.value=="ENDBLK";});
        Tags children{{0,name}};children.insert(children.end(),row.begin(),row.end());
        if(following)children.insert(children.end(),point.begin(),point.end());
        f.document.insert(end,children.begin(),children.end());return f;
    }
    f.document={{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"},
        {0,"SECTION"},{2,context==1u?"ENTITIES":context==2u?"OBJECTS":"LOCAL_SECTION"},{0,name}};
    f.document.insert(f.document.end(),row.begin(),row.end());
    if(context==1u&&following)f.document.insert(f.document.end(),point.begin(),point.end());
    f.document.push_back({0,"ENDSEC"});
    if(context!=1u&&following){f.document.insert(f.document.end(),{{0,"SECTION"},{2,"ENTITIES"}});f.document.insert(f.document.end(),point.begin(),point.end());f.document.push_back({0,"ENDSEC"});}
    f.document.push_back({0,"EOF"});return f;
}

bool rawScopeCarrierMatches(const DRW_RawDxfObject& raw,const Tags& expected,bool binary) {
    if(raw.groups.size()!=expected.size()||raw.rawValues.size()!=expected.size()||raw.hasRawValues==binary)return false;
    for(std::size_t i=0;i<expected.size();++i) {
        const auto& v=raw.groups[i];const auto& t=expected[i];
        if(v.code()!=t.code||(binary?!raw.rawValues[i].empty():raw.rawValues[i]!=t.value))return false;
        if(v.type()==DRW_Variant::STRING){if(!v.content.s||*v.content.s!=t.value)return false;}
        else if(v.type()==DRW_Variant::DOUBLE){if(v.d_val()!=std::stod(t.value))return false;}
        else if(v.type()==DRW_Variant::INTEGER64){if(v.i64_val()!=std::stoll(t.value))return false;}
        else if(v.type()!=DRW_Variant::INTEGER||v.i_val()!=std::stoi(t.value))return false;
    }
    return true;
}

bool rawApplicationIdentityTests(const std::filesystem::path& path,bool missingOnly=false) {
    unsigned profiles=0u,refusals=0u,legacy=0u,parity=0u,softened=0u;std::size_t spans=0u;
    const Tags clean{{5,"C1"},{330,"B0"},{100,"AcDbEntity"},{8,"HOST"},{102,"{APP"},{1,"opaque"},{102,"}"}};
    if(!missingOnly)for(const std::string version:{"AC1012","AC1014","AC1015","AC1018","AC1021","AC1024","AC1027","AC1032"})
     for(bool binary:{false,true})for(unsigned context=0u;context<4u;++context)for(unsigned shape=0u;shape<7u;++shape) {
        Tags row{{5,"C1"},{330,"B0"},{100,"AcDbEntity"},{8,"HOST"},
            {102,"{APP"},{5,shape==0u?"C2":shape==1u?"C1":shape==2u?"not-a-handle":"FFFFFFFFFFFFFFFF"},
            {330,"FACE"},{102,"{INNER"},{5,shape==3u?"C2":shape==0u?"F001":"opaque-inner"},{102,"}"},{102,"}"}};
        if(shape==4u){const Tags opaque{{102,"{APP"},{5,"C2"},{102,"}"}};row=opaque;row.insert(row.end(),clean.begin(),clean.begin()+4);}
        if(shape==5u)row.insert(row.end()-1,{{92,"-1"},{310,"DEAD"}});
        if(shape==6u){row.erase(row.begin());row.push_back({5,"C1"});}
        const auto fixture=rawScopeDocument(version,context,"VENDOR_ENTITY",row);
        if(!write(path,fixture.document,binary))return false;
        dxfRW codec(path.string().c_str());RawScopeProbe receiver;
        if(!check(codec.read(&receiver,false)&&receiver.depth==0u&&receiver.points==std::vector<std::uint32_t>{0xC2u},
            "opaque/repeated/wide/non-handle application group 5 cannot reserve a global identity")) {
            std::cerr<<version<<" binary="<<binary<<" context="<<context<<" shape="<<shape
                <<" point handle="<<(receiver.points.empty()?0u:receiver.points.front())<<'\n';return false;
        }
        if(context<=2u) {
            const auto& raws=context==2u?receiver.objects:receiver.entities;
            if(!check(raws.size()==1u&&raws.front().handle==0xC1u&&raws.front().parentHandle==0xB0u
                &&rawScopeCarrierMatches(raws.front(),row,binary),"raw self/owner convenience fields exclude application data while source remains complete"))return false;
        } else if(!check(receiver.sections.size()==1u,"unknown section remains preserved"))return false;
        if(context==0u&&!check(originalChildrenWire(path,binary,fixture,receiver.children,spans),"independent original inventory includes all opaque fields, not invented object identities"))return false;
        ++profiles;
    }
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true})
     for(unsigned context=0u;context<3u;++context)for(unsigned fault=0u;fault<4u;++fault) {
        auto row=clean;
        if(fault==0u)row.erase(row.begin());
        if(fault==1u)row.front().value="0";
        if(fault==2u)row.insert(row.begin()+1,{5,"C1"});
        if(fault==3u)row.insert(row.begin()+4,{{102,"{APP2"},{102,"}"}});
        row.insert(row.end()-1,{5,"C2"});
        if(fault==3u)row.push_back({5,"C1"}); // a real repeated outer identity, not opaque C2
        const auto bad=rawScopeDocument(version,context,"VENDOR_ENTITY",row,fault>=2u);
        if(!write(path,bad.document,binary))return false;
        dxfRW codec(path.string().c_str());RawScopeProbe rejected;
        const bool accepted=codec.read(&rejected,false);
        if(!check(!accepted&&rejected.depth==0u,"missing/null/duplicate outer self cannot be supplied by an application handle")) {
            std::cerr<<version<<" binary="<<binary<<" context="<<context<<" fault="<<fault<<" accepted="<<accepted<<" depth="<<rejected.depth<<'\n';return false;
        }
        const auto good=rawScopeDocument(version,context,"VENDOR_ENTITY",clean);if(!write(path,good.document,binary))return false;
        RawScopeProbe retry;if(!check(codec.read(&retry,false)&&retry.points==std::vector<std::uint32_t>{0xC2u},"same-codec corrected raw identity retry"))return false;
        ++refusals;
    }
    if(!missingOnly) {
        for(bool binary:{false,true}) {
            const Tags row{{5,"C2"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"1"},{20,"2"},{30,"0"}};
            const auto f=rawScopeDocument("AC1032",1u,"POINT",row);if(!write(path,f.document,binary))return false;
            dxfRW codec(path.string().c_str());RawScopeProbe receiver;
            if(!check(codec.read(&receiver,false)&&receiver.points==std::vector<std::uint32_t>{0xC2u,DRW::NoHandle},
                "existing repeated typed-entity identity softening remains compatible"))return false;
            ++softened;
        }
        for(unsigned context:{1u,2u}) {
            const Tags row{{330,"B0"},{102,"{APP"},{5,"not-a-handle"},{102,"}"}};
            const auto f=rawScopeDocument("AC1009",context,"VENDOR_ENTITY",row,false);if(!write(path,f.document,false))return false;
            dxfRW codec(path.string().c_str());RawScopeProbe receiver;
            if(!check(codec.read(&receiver,false)&&(context==1u?receiver.entities:receiver.objects).front().handle==DRW::NoHandle,
                "legacy absent identity tolerance does not invent an identity from opaque application data"))return false;
            ++legacy;
        }
        for(bool binary:{false,true})for(unsigned scope=0;scope<4u;++scope) {
            auto f=blockFixture("AC1032",1u,true);
            if(scope<=1u){auto tags=scope==1u?f.end:f.begin;std::find_if(tags.begin(),tags.end(),[](const Tag& t){return t.code==5&&t.value!="B1"&&t.value!="B2";})->value="not-a-handle";replaceBlockEndpoint(f,scope==1u,std::move(tags));}
            if(scope==2u) {
                const auto a=std::find_if(f.document.begin(),f.document.end(),[](const Tag& t){return t.code==0&&t.value=="ATTDEF";});
                std::find_if(a,f.document.end(),[](const Tag& t){return t.code==5&&t.value=="DEAD";})->value="not-a-handle";
            }
            if(scope==3u) {
                const auto marker=std::find_if(f.document.begin(),f.document.end(),[](const Tag& t){return t.code==100&&t.value=="AcDbSymbolTableRecord";});
                f.document.insert(marker,{{102,"{APP"},{5,"not-a-handle"},{102,"}"}});
            }
            if(!write(path,f.document,binary))return false;
            BlockSourceProbe receiver;dx_data data;
            if(!check(receiver.fileImport(path.string(),&data,false),"opt-in BLOCK/ENDBLK/ATTDEF/BLOCK_RECORD application capture retains ordinary admission"))return false;
            ++parity;
        }
    }
    std::cout<<"Raw application identity profiles="<<profiles<<"; original inventory scalar spans="<<spans<<"; outer identity refusals/retries="<<refusals
        <<"; legacy tolerance="<<legacy<<"; typed/source application parity="<<parity<<"; typed duplicate softening="<<softened<<'\n';
    return profiles==(missingOnly?0u:448u)&&refusals==96u&&(missingOnly||(spans==2096u&&legacy==2u&&parity==8u&&softened==2u));
}

bool rawApplicationWriterTests(const std::filesystem::path& path) {
    unsigned inventories=0u,roundtrips=0u,refusals=0u;
    for(bool binary:{false,true})for(const std::string opaque:{"C2","not-a-handle"}) {
        std::ostringstream stream(std::ios::out|std::ios::binary);
        std::unique_ptr<dxfWriter> writer=binary?std::unique_ptr<dxfWriter>(new dxfWriterBinary(&stream))
            :std::unique_ptr<dxfWriter>(new dxfWriterAscii(&stream));
        writer->setRecordInventoryEnabled(true,8u);
        if(!writer->writeString(0,"XRECORD"))return false;
        const auto prefix=stream.str();
        {
            DxfWriterRecordScope aborted(*writer);
            if(!writer->writeString(102,"{ABORTED")||!writer->writeString(5,"C2")
                ||!writer->writeString(330,"FACE"))return false;
            writer->markWriteError();
        }
        if(!check(stream.str()==prefix&&!writer->hasWriteError()&&writer->recordHandleCount(0xC2u)==0u,
            "aborted application bytes and tentative identity/depth are rolled back"))return false;
        {
            DxfWriterRecordScope retry(*writer);
            if(!writer->writeString(102,"{APP")||!writer->writeString(5,opaque)
                ||!writer->writeString(330,"FACE")||!writer->writeString(102,"}")
                ||!writer->writeString(5,"C1")||!writer->writeString(330,"B0")
                ||!writer->writeString(100,"AcDbXrecord")||!retry.commit())return false;
        }
        if(!check(writer->finalizeRecordInventory()&&!writer->hasWriteError()
            &&writer->recordHandleCount(0xC1u)==1u&&writer->recordHandleCount(0xC2u)==0u
            &&writer->recordHandleHasOwner(0xC1u,0xB0u),
            "writer inventory uses outer identity/owner after opaque application fields and rollback"))return false;
        ++inventories;
    }
    for(bool sourceBinary:{false,true})for(bool binary:{false,true})
     for(const std::string opaque:{"C2","C1","not-a-handle","FFFFFFFFFFFFFFFF"}) {
        const Tags row{{102,"{APP"},{5,opaque},{102,"}"},{5,"C1"},{330,"0"},{100,"AcDbCustom"},{1,"unchanged"}};
        const auto source=rawScopeDocument("AC1024",2u,"VENDOR_OBJECT",row,false);
        if(!write(path,source.document,sourceBinary))return false;
        RawScopeProbe exporter;
        {dxfRW reader(path.string().c_str());if(!reader.read(&exporter,false)||exporter.objects.size()!=1u)return false;}
        const auto original=exporter.objects.front();
        const auto destination=path.parent_path()/"raw-application-output.dxf";
        dxfRW codec(destination.string().c_str());codec.setDxfWriteRecordInventoryEnabled(true,1024u);
        exporter.output=&codec;
        const bool exported=codec.write(&exporter,DRW::AC1024,binary);
        if(!check(exported&&exporter.written&&exporter.inventoryMatched,
            "public raw-object writer validates/emits opaque group 5 without treating it as an identity")) {
            std::cerr<<"source binary="<<sourceBinary<<" output binary="<<binary<<" opaque="<<opaque<<" exported="<<exported
                <<" callback="<<exporter.written<<" error="<<codec.getError()<<" C1="<<codec.dxfWriteRecordHandleCount(0xC1u)
                <<" C2="<<codec.dxfWriteRecordHandleCount(0xC2u)<<'\n';return false;
        }
        RawScopeProbe reopened;dxfRW reader(destination.string().c_str());
        if(!check(reader.read(&reopened,false)&&reopened.objects.size()==1u
            &&rawScopeCarrierMatches(reopened.objects.front(),row,binary),"raw object reread retains exact application and outer fields"))return false;
        const auto wire=attdefWireTags(destination,binary);
        const auto marker=std::find_if(wire.begin(),wire.end(),[](const auto& t){return t.code==0&&t.text=="VENDOR_OBJECT";});
        if(marker==wire.end()||static_cast<std::size_t>(wire.end()-marker)<=row.size())return false;
        for(std::size_t i=0;i<row.size();++i)if(!check(marker[1u+i].code==row[i].code&&marker[1u+i].text==row[i].value,
            "independent output walker verifies exact opaque and outer scalar spellings"))return false;
        ++roundtrips;
        exporter.objects.front().groups.erase(exporter.objects.front().groups.begin()+3);
        exporter.objects.front().rawValues.erase(exporter.objects.front().rawValues.begin()+3);
        const std::string sentinel="existing destination must survive";
        {std::ofstream file(destination,std::ios::binary|std::ios::trunc);file<<sentinel;if(!file.good())return false;}
        exporter.written=true;
        if(!check(!codec.write(&exporter,DRW::AC1024,binary)&&!exporter.written
            &&codec.dxfWriteRecordHandleCount(0xC1u)==0u,"application-only self is refused without publishing inventory"))return false;
        {std::ifstream file(destination,std::ios::binary);const std::string bytes{std::istreambuf_iterator<char>(file),{}};
            if(!check(bytes==sentinel,"refused raw identity preserves the destination"))return false;}
        exporter.objects.front()=original;exporter.written=true;exporter.inventoryMatched=false;
        if(!check(codec.write(&exporter,DRW::AC1024,binary)&&exporter.written
            &&exporter.inventoryMatched,"same-codec corrected raw-object write retry"))return false;
        ++refusals;
    }
    std::cout<<"Raw application writer inventories/rollbacks="<<inventories<<"; independent output/readback profiles="<<roundtrips
        <<"; destination-preserving refusals/retries="<<refusals<<'\n';
    return inventories==4u&&roundtrips==16u&&refusals==16u;
}

Tags proxyScopeRow(bool entity,unsigned foreign) {
    Tags row{{5,"C1"},{330,"B0"},{100,"AcDbEntity"},{8,"HOST"},{6,"DASHED"},{62,"3"},{370,"35"},{48,"1.25"},
        {60,"0"},{67,"0"},{347,"123"},{390,"456"},{420,"1193046"},{430,"HOST-COLOR"},{440,"1234"},
        {102,"{ACAD_REACTORS"},{330,"ABC"},{102,"}"},{102,"{ACAD_XDICTIONARY"},{360,"DEF"},{102,"}"},
        {100,entity?"AcDbProxyEntity":"AcDbProxyObject"},{90,entity?"498":"499"},{91,"500"},{95,"1032"},{70,"1"}};
    if(entity)row.insert(row.end(),{{92,"2"},{310,"0102"},{93,"8"},{310,"A3"},{96,"1"},{311,"B4"}});
    else row.insert(row.end(),{{93,"8"},{311,"A3"}});
    row.insert(row.end(),{{330,"F00"},{94,"0"}});
    if(foreign==0u)row.insert(row.end(),{{102,"{OPAQUE"},{8,"FALSE"},{6,"FALSE"},{62,"7"},{370,"50"},{48,"4"},
        {60,"1"},{67,"1"},{347,"BAD"},{390,"BAD"},{420,"7"},{430,"FALSE"},{440,"7"},{102,"}"}});
    if(foreign==1u)row.insert(row.end(),{{102,"{OPAQUE"},{5,"not-a-handle"},{90,"1"},{91,"2"},{92,"-1"},{93,"99"},{94,"1"},
        {95,"7"},{70,"0"},{310,"DEAD"},{311,"BEEF"},{330,"CAFE"},{100,"AcDbUnrelated"},{102,"}"}});
    if(foreign==2u)row.insert(row.end(),{{102,"{OPAQUE"},{102,"{ACAD_REACTORS"},{330,"BAD"},{102,"}"},
        {102,"{ACAD_XDICTIONARY"},{360,"BAD"},{102,"}"},{102,"}"}});
    if(foreign==4u)row[1].value="0";
    if(foreign==5u)row[1].value="100000001";
    if(foreign==6u)for(auto& tag:row)if(tag.code==347||tag.code==390)tag.value="100000001";
    return row;
}

bool rawProxyScopeTests(const std::filesystem::path& path) {
    unsigned profiles=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true})
     for(bool entity:{false,true})for(unsigned foreign=0u;foreign<7u;++foreign) {
        const auto row=proxyScopeRow(entity,foreign);
        const auto fixture=rawScopeDocument(version,entity?0u:2u,entity?"ACAD_PROXY_ENTITY":"ACAD_PROXY_OBJECT",row);
        if(!write(path,fixture.document,binary))return false;
        dxfRW codec(path.string().c_str());RawScopeProbe receiver;
        if(!check(codec.read(&receiver,false)&&receiver.points==std::vector<std::uint32_t>{0xC2u},"proxy application payload cannot alter host grammar/admission"))return false;
        const auto& raws=entity?receiver.entities:receiver.objects;
        const auto owner=foreign==4u||foreign==5u?0u:0xB0u;
        if(!check(raws.size()==1u&&raws.front().handle==0xC1u&&raws.front().parentHandle==owner
            &&rawScopeCarrierMatches(raws.front(),row,binary),"proxy raw ownership and opaque source remain exact"))return false;
        const DRW_ProxyObject* object=nullptr;const DRW_ProxyEntity* graphics=nullptr;
        if(entity){if(receiver.proxyEntities.size()!=1u)return false;graphics=&receiver.proxyEntities.front();}
        else {if(receiver.proxyObjects.size()!=1u)return false;object=&receiver.proxyObjects.front();}
        const auto& ids=entity?graphics->m_objectIdRefs:object->m_objectIdRefs;
        if(!check((entity?graphics->parentHandle:object->parentHandle)==owner
            &&(entity?graphics->reactorHandles:object->reactorHandles)==std::vector<std::uint32_t>{0xABCu}
            &&(entity?graphics->xDictHandle:object->xDictHandle)==0xDEFu&&ids.size()==1u&&ids.front().m_handle==0xF00u,
            "common header owner/reserved references remain distinct from proxy object-ID list and opaque nested groups"))return false;
        if(entity&&!check(graphics->layer=="HOST"&&graphics->lineType=="DASHED"&&graphics->color==3&&graphics->visible
            &&graphics->space==DRW::ModelSpace&&graphics->material==(foreign==6u?0u:0x123u)&&graphics->plotStyle==(foreign==6u?0u:0x456u)
            &&graphics->color24==1193046&&graphics->colorName=="HOST-COLOR"&&graphics->transparency==1234
            &&graphics->ltypeScale==1.25&&graphics->proxyGraphics==std::string("\x01\x02",2u)
            &&graphics->m_entityData==std::vector<std::uint8_t>{0xA3u},"proxy common fields and payload are immune to application data"))return false;
        if(!entity&&!check(object->m_objectData==std::vector<std::uint8_t>{0xA3u},"proxy object payload ignores opaque foreign byte/count fields"))return false;
        const auto metadata=[](const auto& value){return value.m_hasProxyClassId&&value.m_proxyClassId==500
            &&value.m_hasProxyDrawingFormat&&value.m_proxyDrawingFormat==1032&&value.m_hasFromDxf&&value.m_fromDxf;};
        if(!check(entity?metadata(*graphics):metadata(*object),"opaque scalars do not rewrite proxy class/format/origin metadata"))return false;
        ++profiles;
    }
    std::cout<<"Raw proxy application scope profiles="<<profiles<<'\n';return profiles==112u;
}
