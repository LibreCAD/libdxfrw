// SPDX-License-Identifier: GPL-2.0-or-later
// Test-local captured scalar reconstruction. This is not adapter proof,
// strict source grammar, owner closure or output admission.
bool legacyBlockRecordModelEqual(const DRW_Block_Record& a,const DRW_Block_Record& b) {
    const auto real=[](double x,double y){return std::memcmp(&x,&y,sizeof(x))==0;};
    if(a.tType!=b.tType||a.handle!=b.handle||a.parentHandle!=b.parentHandle||a.name!=b.name
        ||a.flags!=b.flags||a.xDictHandle!=b.xDictHandle||a.reactorHandles!=b.reactorHandles
        ||a.reactorCount()!=b.reactorCount()||a.extensionDictionaryFlag()!=b.extensionDictionaryFlag()
        ||a.hasDataStorageBinaryData()!=b.hasDataStorageBinaryData()||a.hasDwgCommonLinkTail()!=b.hasDwgCommonLinkTail()
        ||!a.pendingAppIdResolutions.empty()||!b.pendingAppIdResolutions.empty()
        ||!a.pendingLayerRefResolutions.empty()||!b.pendingLayerRefResolutions.empty()
        ||a.insUnits!=b.insUnits||a.canExplode!=b.canExplode||a.blockScaling!=b.blockScaling
        ||a.layoutHandle!=b.layoutHandle||a.insertCount!=b.insertCount||a.insertHandles!=b.insertHandles||a.previewData!=b.previewData
        ||a.xrefUnloaded!=b.xrefUnloaded||a.xrefPath!=b.xrefPath||a.description!=b.description
        ||a.xrefIndex!=b.xrefIndex||!real(a.basePoint.x,b.basePoint.x)||!real(a.basePoint.y,b.basePoint.y)||!real(a.basePoint.z,b.basePoint.z)
        ||a.xrefBlockHandle.code!=b.xrefBlockHandle.code||a.xrefBlockHandle.size!=b.xrefBlockHandle.size
        ||a.xrefBlockHandle.ref!=b.xrefBlockHandle.ref||a.xrefBlockHandle.ref64!=b.xrefBlockHandle.ref64
        ||a.appData.size()!=b.appData.size()||a.extData.size()!=b.extData.size()
        ||!dxfBlockRecord::Decoder::hasNoNativeLinks(a)||!dxfBlockRecord::Decoder::hasNoNativeLinks(b))return false;
    auto ga=a.appData.begin(),gb=b.appData.begin();
    for(;ga!=a.appData.end();++ga,++gb) {
        if(ga->size()!=gb->size())return false;
        auto va=ga->begin(),vb=gb->begin();
        for(;va!=ga->end();++va,++vb)if(!legacyVariantEqual(*va,*vb))return false;
    }
    for(std::size_t i=0;i<a.extData.size();++i) {
        if((a.extData[i]==nullptr)!=(b.extData[i]==nullptr))return false;
        if(a.extData[i]&&!legacyVariantEqual(*a.extData[i],*b.extData[i]))return false;
    }
    return true;
}
LegacyCapturedReplay legacyBlockRecordReplay(const DRW_RawDxfObject& raw,const DRW_Block_Record& model,
    std::pair<bool,std::uint8_t> state,const std::string& page) {
    LegacyCapturedReplay result;if(raw.groups.size()!=raw.rawValues.size())return result;
    std::ostringstream out(std::ios::out|std::ios::binary);dxfWriterBinary writer(&out);
    for(std::size_t i=0;i<raw.groups.size();++i) {
        const auto& v=raw.groups[i];const int code=v.code();
        if(raw.hasRawValues){out<<code<<'\n'<<raw.rawValues[i]<<'\n';continue;}
        if(!raw.rawValues[i].empty())return result;
        if(v.type()==DRW_Variant::STRING&&v.content.s){if(!writer.writeRawString(code,*v.content.s))return result;}
        else if(v.type()==DRW_Variant::INTEGER){if(!writer.writeInt16(code,v.i_val()))return result;}
        else return result;
    }
    const auto bytes=out.str();dxfRW::DxfObjectsWriteView view{};view.bytes=bytes;view.version=raw.m_version;
    view.binary=!raw.hasRawValues;view.codePage=page;
    result.transport=dx_standard_output::rawScalarTransportEqual(raw.groups,bytes,view,
        [](std::uint32_t x,std::uint32_t& y){y=x;return true;},false,1024u*1024u,true);
    if(!result.transport)return result;
    dx_standard_output::InputBuffer buffer(bytes);std::istream in(&buffer);
    auto reader=dx_standard_output::readerFor(in,view,std::make_shared<DxfReadRetainedBudget>(1024u*1024u));
    if(!reader)return result;DRW_Block_Record parsed;dxfBlockRecord::Decoder decoder(parsed,reader);int code=0;
    while(buffer.position()<bytes.size())if(!reader->readRec(&code)||code==0||!decoder.parseCode(code,reader))return result;
    if(!decoder.finish(raw.m_version)||reader->recordCount()!=raw.groups.size())return result;
    result.decoded=true;result.state=dxfBlockRecord::Decoder::observeCommonState(parsed)==state&&!state.first&&!state.second;
    result.model=legacyBlockRecordModelEqual(parsed,model);return result;
}
bool legacyBlockRecordObservationTests(const std::filesystem::path& path) {
    struct Observer final:dx_iface {
        dxfRW* reader=nullptr;bool enabled=true,sourceLive=false,typedLive=false;
        unsigned sources=0u,typed=0u;DRW_RawDxfObject raw;DRW_Block_Record original,delivered;
        std::pair<bool,std::uint8_t> sourceState{},typedState{};dxfRW::DxfReadContext context{};
        std::string page,declaration;
        void addHeader(const DRW_Header*)override{}
        bool wantsDxfBlockRecordSource()const override{return enabled;}
        void addDxfBlockRecordSource(const DRW_Block_Record& value,const DRW_RawDxfObject& body)override {
            ++sources;dxfRW::DxfStructuralSourceView view;dxfRW::DxfReadContext live;
            sourceLive=reader&&reader->getDxfStructuralSourceView(view)&&view.model==&value&&view.source==&body
                &&view.kind==dxfRW::DxfStructuralSourceView::Kind::BlockRecord
                &&view.phase==dxfRW::DxfStructuralSourceView::Phase::Source&&reader->getDxfReadContext(live);
            if(sourceLive){context=live;page=std::string(live.codecCodePage);declaration=std::string(live.declaration);}
            sourceState=dxfBlockRecord::Decoder::observeCommonState(value);raw=body;original=value;
        }
        void addBlockRecord(const DRW_Block_Record& value)override {
            ++typed;typedState=dxfBlockRecord::Decoder::observeCommonState(value);delivered=value;
            dxfRW::DxfStructuralSourceView view;
            typedLive=reader&&reader->getDxfStructuralSourceView(view)&&view.model==&value
                &&view.kind==dxfRW::DxfStructuralSourceView::Kind::BlockRecord
                &&view.phase==dxfRW::DxfStructuralSourceView::Phase::Typed;
        }
    };
    const Tags row{{5,"A1"},{102,"{ACAD_XDICTIONARY"},{360,"D0"},{102,"}"},{330,"B0"},
        {100,"AcDbSymbolTableRecord"},{100,"AcDbBlockTableRecord"},{2,"MY_BLOCK"},{340,"0"},
        {102,"{BLKREFS"},{331,"E0"},{102,"}"},{1001,"TEST_APP"},{1000,std::string("caf\xE9",4u)},
        {1002,"{"},{1070,"1"},{1070,"4"},{1002,"}"}};
    Tags document{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1018"},{9,"$DWGCODEPAGE"},{3,"ANSI_1252"},
        {0,"ENDSEC"},{0,"SECTION"},{2,"TABLES"},{0,"TABLE"},{2,"BLOCK_RECORD"},{5,"B0"},
        {100,"AcDbSymbolTable"},{70,"1"},{0,"BLOCK_RECORD"}};
    document.insert(document.end(),row.begin(),row.end());document.insert(document.end(),{{0,"ENDTAB"},{0,"ENDSEC"},{0,"EOF"}});
    unsigned observations=0u,optouts=0u,replays=0u,wrongPages=0u,mutations=0u;
    for(bool binary:{false,true}) {
        if(!write(path,document,binary))return false;dxfRW reader(path.string().c_str());Observer observer;observer.reader=&reader;
        if(!check(reader.read(&observer,false)&&observer.sources==1u&&observer.typed==1u&&observer.sourceLive&&observer.typedLive
            &&observer.context.sourceVersion==DRW::AC1018&&observer.context.codecVersion==DRW::AC1015
            &&observer.page=="ANSI_1252"&&observer.context.hasDeclaration&&observer.declaration=="ANSI_1252"
            &&observer.context.binary==binary&&!observer.context.applyExtrusion
            &&observer.raw.m_version==DRW::AC1018&&observer.raw.hasRawValues!=binary&&observer.raw.groups.size()==row.size()
            &&observer.sourceState==observer.typedState&&legacyBlockRecordModelEqual(observer.original,observer.delivered)
            &&observer.original.handle==0xA1u&&observer.original.parentHandle==0xB0u&&observer.original.xDictHandle==0xD0u
            &&observer.original.layoutHandle==0u&&observer.original.insertHandles==std::vector<std::uint32_t>{0xE0u},
            "legacy block record live source/typed context and model"))return false;
        const auto replay=legacyBlockRecordReplay(observer.raw,observer.original,observer.sourceState,observer.page);
        if(!check(replay.transport&&replay.decoded&&replay.state&&replay.model,"legacy block record captured scalar replay"))return false;
        const auto wrong=legacyBlockRecordReplay(observer.raw,observer.original,observer.sourceState,"ANSI_1251");
        if(!check(wrong.transport&&wrong.decoded&&wrong.state&&!wrong.model,"legacy block record wrong codec sensitivity"))return false;
        ++replays;++wrongPages;
        for(int code:{360,1000}) {
            auto altered=observer.raw;for(std::size_t i=0;i<altered.groups.size();++i)if(altered.groups[i].code()==code){
                const std::string value=code==360?"D1":"changed";altered.groups[i].addString(code,value);
                if(altered.hasRawValues)altered.rawValues[i]=value;break;}
            const auto fault=legacyBlockRecordReplay(altered,observer.original,observer.sourceState,observer.page);
            if(!check(!fault.transport||(fault.decoded&&!fault.model),"legacy block record raw mutation sensitivity"))return false;
            ++mutations;
        }
        Observer no;no.reader=&reader;no.enabled=false;
        if(!check(reader.read(&no,false)&&!no.sources&&no.typed==1u
            &&legacyBlockRecordModelEqual(no.delivered,observer.delivered),"legacy block record source facet opt-out"))return false;
        ++optouts;++observations;
    }
    std::cout<<"AC1018 BLOCK_RECORD observations="<<observations<<" opt-outs="<<optouts
        <<" captured-replays="<<replays<<" wrong-pages="<<wrongPages<<" mutations="<<mutations<<'\n';
    return observations==2u&&optouts==2u&&replays==2u&&wrongPages==2u&&mutations==4u;
}
