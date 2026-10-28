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

bool legacyBlockRecordAdapterTests(const std::filesystem::path& path) {
    using A=DxIfaceTableSourceTestAccess;
    struct Probe final:dx_iface {
        bool selected=false;unsigned fault=0u;
        bool wantsDxfBlockSource()const override {
            if(fault==4u)throw std::runtime_error("legacy block source selection");return selected;
        }
        void addDxfBlockRecordSource(const DRW_Block_Record& value,const DRW_RawDxfObject& raw)override {
            auto& mutableRaw=const_cast<DRW_RawDxfObject&>(raw);DRW_Variant saved;
            std::size_t index=raw.groups.size();
            if(fault==1u)for(std::size_t i=0;i<raw.groups.size();++i)if(raw.groups[i].code()==360) {
                index=i;saved=raw.groups[i];mutableRaw.groups[i].addString(360,"D1");break;
            }
            dx_iface::addDxfBlockRecordSource(value,raw);
            if(index<raw.groups.size())mutableRaw.groups[index]=saved;
            if(fault==3u&&cData)A::corruptStructuralContext(*cData,value.handle);
        }
        void addBlockRecord(const DRW_Block_Record& value)override {
            auto& mutableValue=const_cast<DRW_Block_Record&>(value);const auto saved=mutableValue.insUnits;
            if(fault==2u)++mutableValue.insUnits;
            dx_iface::addBlockRecord(value);mutableValue.insUnits=saved;
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
    unsigned captures=0u,defaults=0u,refusals=0u,quotas=0u,retries=0u,unknowns=0u;
    const auto unqualified=[](const dx_data& data) {
        const auto* row=A::resource(data);const auto* audit=A::structuralAudit(data);
        return row&&!row->rawTypedMatched&&row->rawTypedMismatch
            &&(!audit||(!audit->bodyMatched&&!audit->originBound));
    };
    for(bool binary:{false,true}) {
        if(!write(path,document,binary))return false;
        Probe normal;dx_data baseline;A::structuralLimit(baseline,0u,0u);A::structuralLimit(baseline,1u,0u);A::structuralLimit(baseline,2u,0u);
        if(!check(normal.fileImport(path.string(),&baseline,false)&&!A::structuralFailed(baseline)
            &&!A::structuralRecord(baseline)&&unqualified(baseline),"legacy block record default path avoids optional structural quota"))return false;
        ++defaults;
        Probe selected;selected.selected=true;dx_data data;
        if(!check(selected.fileImport(path.string(),&data,false)&&!A::structuralFailed(data)&&unqualified(data),
            "legacy block record opt-in import remains unqualified"))return false;
        const auto* input=A::structuralRecord(data);const auto* resource=A::resource(data);
        if(!check(input&&resource&&input->paired&&input->legacyBlockRecordReplayMatched
            &&input->legacyBlockRecordProfile.complete()&&input->model==resource->model
            &&input->raw==resource->raw&&input->context&&input->context->captured
            &&input->context->sourceVersion==DRW::AC1018&&input->context->codecVersion==DRW::AC1015
            &&input->context->codecCodePage=="ANSI_1252"&&input->context->hasDeclaration
            &&input->context->declaration=="ANSI_1252"&&input->context->binary==binary&&!input->context->applyExtrusion
            &&!input->originalState.first&&!input->originalState.second,
            "legacy block record adapter captured replay and context"))return false;
        using namespace dx_legacy_block_record_source;
        const auto& profile=input->legacyBlockRecordProfile;
        const std::array<Field,18> fields{{Field::Identity,Field::Application,Field::Application,Field::Application,
            Field::Identity,Field::Subclass,Field::Subclass,Field::Name,Field::Layout,Field::Application,
            Field::Application,Field::Application,Field::Xdata,Field::Xdata,Field::Xdata,Field::Xdata,Field::Xdata,Field::Xdata}};
        const std::array<Role,18> roles{{Role::Self,Role::None,Role::ExtensionDictionary,Role::None,Role::Owner,
            Role::None,Role::None,Role::BlockName,Role::Layout,Role::None,Role::BlockInsert,Role::None,
            Role::AppIdName,Role::None,Role::None,Role::None,Role::None,Role::None}};
        const auto wire=attdefWireTags(path,binary);std::size_t start=wire.size();
        for(std::size_t i=0;i<wire.size();++i)if(wire[i].code==0&&wire[i].text=="BLOCK_RECORD")start=i+1u;
        if(!check(profile.spans.size()==row.size()&&start+row.size()<=wire.size(),"legacy block record profile/wire counts"))return false;
        for(std::size_t i=0;i<row.size();++i) {
            const auto& span=profile.spans[i];const auto& physical=wire[start+i];const auto& value=input->raw->groups[i];
            const std::uint32_t handle=i==0?0xA1u:i==2?0xD0u:i==4?0xB0u:i==10?0xE0u:0u;
            if(!check(span.index==i&&span.code==row[i].code&&span.field==fields[i]&&span.role==roles[i]
                &&span.handle==handle&&physical.code==row[i].code&&physical.begin<physical.valueBegin
                &&physical.valueBegin<physical.end&&(!i||wire[start+i-1u].end==physical.begin),
                "legacy block record independent physical span and role"))return false;
            if(physical.kind==AttdefWireTag::String&&(physical.text!=row[i].value||value.type()!=DRW_Variant::STRING
                ||!value.content.s||*value.content.s!=physical.text))return false;
            if(physical.kind==AttdefWireTag::Integer&&(value.type()!=DRW_Variant::INTEGER
                ||value.i_val()!=physical.integer||physical.integer!=std::stoll(row[i].value)))return false;
        }
        if(!legacyBlockRecordProfileDirectControls(*input->raw))return false;
        ++captures;
        for(unsigned fault:{1u,2u,3u,4u}) {
            Probe broken;broken.selected=true;broken.fault=fault;dx_data bad;
            if(!check(broken.fileImport(path.string(),&bad,false)&&unqualified(bad)
                &&(A::structuralFailed(bad)||!A::structuralRecord(bad)
                   ||!A::structuralRecord(bad)->legacyBlockRecordReplayMatched),
                "legacy block record raw/model/context mutation refuses replay"))return false;
            broken.fault=0u;dx_data retry;
            if(!check(broken.fileImport(path.string(),&retry,false)&&!A::structuralFailed(retry)
                &&A::structuralRecord(retry)&&A::structuralRecord(retry)->legacyBlockRecordReplayMatched
                &&A::structuralRecord(retry)->legacyBlockRecordProfile.complete()&&unqualified(retry),
                "legacy block record mutation corrected fresh-destination retry"))return false;
            ++refusals;++retries;
        }
        const auto usage=A::structuralUsage(data);
        for(unsigned kind=0u;kind<usage.size();++kind) {
            dx_data exact;A::structuralLimit(exact,kind,usage[kind]);
            if(!check(selected.fileImport(path.string(),&exact,false)&&!A::structuralFailed(exact)
                &&A::structuralRecord(exact)&&A::structuralRecord(exact)->legacyBlockRecordReplayMatched
                &&A::structuralRecord(exact)->legacyBlockRecordProfile.complete(),
                "legacy block record exact structural budget"))return false;
            dx_data below;A::structuralLimit(below,kind,usage[kind]-1u);
            if(!check(selected.fileImport(path.string(),&below,false)&&A::structuralFailed(below)&&unqualified(below),
                "legacy block record one-below structural budget"))return false;
            dx_data retry;if(!check(selected.fileImport(path.string(),&retry,false)&&!A::structuralFailed(retry)
                &&A::structuralRecord(retry)&&A::structuralRecord(retry)->legacyBlockRecordReplayMatched
                &&A::structuralRecord(retry)->legacyBlockRecordProfile.complete(),
                "legacy block record budget corrected retry"))return false;
            ++quotas;++retries;
        }
        auto extra=document;const auto pos=std::find_if(extra.begin(),extra.end(),[](const Tag& t){return t.code==1001;});
        extra.insert(pos,{300,"unmodeled"});if(!write(path,extra,binary))return false;
        dx_data unsupported;
        if(!check(selected.fileImport(path.string(),&unsupported,false)&&A::structuralRecord(unsupported)
            &&A::structuralRecord(unsupported)->legacyBlockRecordReplayMatched
            &&!A::structuralRecord(unsupported)->legacyBlockRecordProfile.complete()
            &&A::structuralRecord(unsupported)->legacyBlockRecordProfile.reason==Reason::Xdata
            &&A::structuralRecord(unsupported)->legacyBlockRecordProfile.rejectedGroup==12u&&unqualified(unsupported),
            "legacy modeled replay alone does not qualify unknown source fields"))return false;
        ++unknowns;
    }
    std::cout<<"AC1018 BLOCK_RECORD adapter="<<captures<<" defaults="<<defaults<<" corruptions="<<refusals
        <<" budgets="<<quotas<<" retries="<<retries<<" unmodeled="<<unknowns<<'\n';
    return captures==2u&&defaults==2u&&refusals==8u&&quotas==6u&&retries==14u&&unknowns==2u;
}
