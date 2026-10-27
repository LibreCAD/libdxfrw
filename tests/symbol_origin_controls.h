// SPDX-License-Identifier: GPL-2.0-or-later
// Authored symbol tables; no external drawing fixtures/derivatives.
Tags symbolOriginTables(int flags) {
    return {{0,"TABLE"},{2,"LTYPE"},{5,"D0"},{330,"0"},{100,"AcDbSymbolTable"},{70,"1"},
        {0,"LTYPE"},{5,"D101"},{330,"D0"},{100,"AcDbSymbolTableRecord"},{100,"AcDbLinetypeTableRecord"},{2,"CONTINUOUS"},{70,"0"},{3,"authored continuous"},{72,"65"},{73,"0"},{40,"0"},{0,"ENDTAB"},
        {0,"TABLE"},{2,"STYLE"},{5,"D1"},{330,"0"},{100,"AcDbSymbolTable"},{70,"1"},
        {0,"STYLE"},{5,"D201"},{330,"D1"},{100,"AcDbSymbolTableRecord"},{100,"AcDbTextStyleTableRecord"},{2,"STANDARD"},{70,"0"},{40,"0"},{41,"1"},{50,"0"},{71,"0"},{42,"1"},{3,"txt"},{4,""},{0,"ENDTAB"},
        {0,"TABLE"},{2,"LAYER"},{5,"D2"},{330,"0"},{100,"AcDbSymbolTable"},{70,"1"},
        {0,"LAYER"},{5,"D301"},{330,"D2"},{100,"AcDbSymbolTableRecord"},{100,"AcDbLayerTableRecord"},{2,"SOURCE_LAYER"},{70,"0"},{62,"-3"},{6,"CONTINUOUS"},{290,"0"},{370,"25"},{420,"1122867"},{430,"BOOK$ENTRY"},{347,"40"},{390,"43"},{0,"ENDTAB"},
        {0,"TABLE"},{2,"APPID"},{5,"D3"},{330,"0"},{100,"AcDbSymbolTable"},{70,"1"},
        {0,"APPID"},{5,"D401"},{330,"D3"},{100,"AcDbSymbolTableRecord"},{100,"AcDbRegAppTableRecord"},{2,"REGISTERED_APP"},{70,std::to_string(flags)},{71,"0"},{0,"ENDTAB"}};
}
Tags symbolOriginDocument(const std::string& version,bool blocks,unsigned target,int flags) {
    auto tags=resourceOriginDocument(version,blocks,0u);const auto tables=symbolOriginTables(flags);const auto at=slot(tags,2,"TABLES")+1u;
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),tables.begin(),tables.end());
    const char* targets[]={"D101","D201","D301","D401"};auto value=blockField(tags,"501",301)+1u;while(tags[value].code!=330)++value;tags[value].value=targets[target];
    return tags;
}
struct SymbolOriginProbe:dx_iface {
    using V=dxfRW::DxfResourceSourceView;bool enabled=true,publicationEnabled=true,good=true,publicOnly=false,expectView=true,disableSource=false;
    int skip=0,copy=0,duplicate=0,mutate=0,cursor=0,throwAt=0;unsigned callbacks=0u,originalPending=0u;dxfRW* publicReader=nullptr;
    const DRW_TableEntry* model=nullptr;const DRW_RawDxfObject* raw=nullptr;
    bool wantsDxfSymbolSource() const override{return enabled&&(publicOnly||dx_iface::wantsDxfSymbolSource());}
    bool wantsDxfNamespaceSource() const override{return enabled&&(publicOnly||dx_iface::wantsDxfNamespaceSource());}
    bool wantsDxfResourceSource() const override{return publicationEnabled&&(publicOnly||dx_iface::wantsDxfResourceSource());}
    void addHeader(const DRW_Header* h) override{if(!publicOnly)dx_iface::addHeader(h);}
    void inspect(const DRW_TableEntry& v,const DRW_RawDxfObject* source,unsigned kind,unsigned phase) {
        const auto* reader=publicReader?publicReader:(cData?DxIfaceTableSourceTestAccess::readInput(*cData):nullptr);V view;
        const auto before=symbolFailedAllocations;bool present=false;
        {SymbolAllocationFailure noAllocation(0);present=reader&&reader->getDxfResourceSourceView(view);}
        if(present!=expectView||symbolFailedAllocations!=before)good=false;
        if(present) {
            if(static_cast<unsigned>(view.kind)!=kind||static_cast<unsigned>(view.phase)!=phase||view.model!=&v||!view.source||(source&&view.source!=source))good=false;
            if(phase==3u){model=&v;raw=view.source;if(view.originalPendingXdata||view.originalCoordinateMask)++originalPending;}
            else if(model!=&v||raw!=view.source)good=false;
        }
        ++callbacks;
    }
    template<class Call>void source(const DRW_TableEntry& v,const DRW_RawDxfObject& r,unsigned kind,Call call) {
        inspect(v,&r,kind,3u);auto& actual=const_cast<DRW_TableEntry&>(v);const auto flags=actual.flags;const auto state=dxfBlockRecord::Decoder::observeCommonState(v);
        if(mutate==3)actual.flags^=1;if(cursor==3)DrwObjectEncodeTestAccess::commonMask(actual,static_cast<std::uint8_t>(state.second^1u));
        if(skip!=3){if(copy==3){const DRW_RawDxfObject clone(r);call(v,clone);}else call(v,r);if(duplicate==3)call(v,r);}
        actual.flags=flags;DrwObjectEncodeTestAccess::commonMask(actual,state.second);
        if(disableSource){enabled=false;publicationEnabled=false;}if(throwAt==3)throw std::runtime_error("symbol source publication");
    }
    void addDxfSymbolSource(const DRW_TableEntry& v,const DRW_RawDxfObject& r) override {
        source(v,r,v.tType==DRW::STYLE?5u:6u,[&](const auto& x,const auto& y){dx_iface::addDxfSymbolSource(x,y);});
    }
    void addDxfNamespaceSource(const DRW_TableEntry& v,const DRW_RawDxfObject& r) override {
        source(v,r,v.tType==DRW::LAYER?7u:8u,[&](const auto& x,const auto& y){dx_iface::addDxfNamespaceSource(x,y);});
    }
    template<class Model,class Call>void typed(const Model& v,unsigned kind,Call call) {
        inspect(v,nullptr,kind,1u);auto& actual=const_cast<Model&>(v);const auto flags=actual.flags;const auto state=dxfBlockRecord::Decoder::observeCommonState(v);
        if(mutate==1)actual.flags^=1;if(cursor==1)DrwObjectEncodeTestAccess::commonMask(actual,static_cast<std::uint8_t>(state.second^1u));
        if(skip!=1){if(copy==1){const Model clone(v);call(clone);}else call(v);if(duplicate==1)call(v);}
        actual.flags=flags;DrwObjectEncodeTestAccess::commonMask(actual,state.second);model=nullptr;raw=nullptr;
        if(throwAt==1)throw std::runtime_error("symbol typed publication");
    }
    void addTextStyle(const DRW_Textstyle& v) override{typed(v,5u,[&](const auto& x){dx_iface::addTextStyle(x);});}
    void addLType(const DRW_LType& v) override{typed(v,6u,[&](const auto& x){dx_iface::addLType(x);});}
    void addLayer(const DRW_Layer& v) override{typed(v,7u,[&](const auto& x){dx_iface::addLayer(x);});}
    void addAppId(const DRW_AppId& v) override{typed(v,8u,[&](const auto& x){dx_iface::addAppId(x);});}
};
bool symbolOriginTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::ResourceOriginState;
    const auto path=dir/"symbol-origins.dxf";unsigned profiles=0u,markers=0u,refusals=0u,pending=0u,unmodeled=0u,quotas=0u,contexts=0u,reached=0u,unreached=0u,retries=0u,flags=0u,lifetimes=0u,optouts=0u,outputs=0u,outputRefusals=0u;
    const auto ready=[](const dx_data& d){const auto r=dx_iface::resourceOriginReport(d);const auto refs=dx_iface::tableReferenceReport(d);
        return r.state==S::OriginsBound&&r.records==8u&&r.paired==8u&&r.stateComplete==8u&&r.bodyMatched==8u&&r.originsBound==8u
            &&refs.sourceTargetsMatched&&refs.valueSourceTargetsMatched&&!refs.ownerTargetsResolved&&!refs.outputEligible;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true})
        for(int flag:{0,16,32,48,64,80,96,112})for(unsigned target=0u;target<4u;++target) {
            SymbolOriginProbe reader;dx_data data;if(!writeBlockBinding(path,symbolOriginDocument(version,blocks,target,flag),encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data)||!reader.good||reader.callbacks!=8u) {
                const auto r=dx_iface::resourceOriginReport(data);std::cerr<<"Symbol origins "<<version<<'/'<<encoding<<" blocks="<<blocks<<" flag="<<flag<<" target="<<target<<" state="<<static_cast<int>(r.state)<<" records="<<r.records<<" paired="<<r.paired<<" bodies="<<r.bodyMatched<<" origins="<<r.originsBound<<'\n';return false;
            }
            const auto* app=dynamic_cast<const DRW_AppId*>(A::resources(data).at(0xD401u).model.get());
            if(!app||app->flags!=flag||!DrwObjectEncodeTestAccess::appIdDxfValid(*app)||DrwObjectEncodeTestAccess::appIdNativeValid(*app)!=((flag&32)==0))return false;
            const auto wire=attdefWireTags(path,encoding==2u);const auto& source=*A::identityInput(data)->source;
            for(const auto& item:A::resourceOrigins(data)) {
                const auto& a=*item.second;const auto& original=source.records.at(a.physical);const auto& w=wire.at(original.marker.ordinal-1u);
                if(w.code||w.text!=original.name||w.begin!=original.marker.beginOffset||w.end!=original.marker.endOffset||!a.input->sourceModel)return false;++markers;
            }
            ++profiles;
        }
    const char* ids[]={"D101","D201","D301","D401"};
    for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true}) {
        const auto good=symbolOriginDocument("AC1032",blocks,0u,48);const auto retry=[&](dx_iface& reader){dx_data next;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&next,false)||!ready(next))return false;++retries;return true;};
        for(unsigned mode=0u;mode<12u;++mode) {
            SymbolOriginProbe reader;const int phase=mode%2u?1:3;
            if(mode<2u)reader.skip=phase;else if(mode<4u)reader.copy=phase;else if(mode<6u)reader.duplicate=phase;else if(mode<8u)reader.mutate=phase;else if(mode<10u)reader.cursor=phase;else reader.throwAt=phase;
            dx_data data;if(!writeBlockBinding(path,good,encoding))return false;try{(void)reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){}
            const auto r=dx_iface::resourceOriginReport(data);const auto expected=mode>=10u?S::ImportFailed:mode>=6u?S::Unqualified:S::CaptureFailed;
            if(r.state!=expected||dx_iface::tableReferenceReport(data).sourceTargetsMatched||!A::resourcePendingEmpty(data)) {std::cerr<<"Symbol refusal mode="<<mode<<" state="<<static_cast<int>(r.state)<<'\n';return false;}
            reader.skip=reader.copy=reader.duplicate=reader.mutate=reader.cursor=reader.throwAt=0;if(!retry(reader))return false;++refusals;
        }
        for(unsigned family=0u;family<4u;++family) {
            auto tags=symbolOriginDocument("AC1032",blocks,family,48);appendBlockFields(tags,ids[family],{{1001,"REGISTERED_APP"},{1010,"1"}});SymbolOriginProbe reader;dx_data data;
            if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!reader.originalPending||ready(data)||dx_iface::tableReferenceReport(data).valueSourceTargetsMatched||!retry(reader))return false;++pending;
        }
        for(unsigned fault=0u;fault<8u;++fault) {
            const auto family=fault==6u?3u:fault==7u?2u:fault%2u+2u;auto tags=symbolOriginDocument("AC1032",blocks,family,48);
            if(fault<2u)appendBlockFields(tags,ids[family],{{99,"7"}});
            else if(fault<4u)tags[blockField(tags,ids[family],330)].value="40";
            else if(fault<6u)appendBlockFields(tags,ids[family],{{70,family==3u?"48":"0"}});
            else tags[blockField(tags,ids[family],fault==6u?71:370)].value=fault==6u?"7":"-7";
            SymbolOriginProbe reader;dx_data data;if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||ready(data)||dx_iface::tableReferenceReport(data).valueSourceTargetsMatched||!retry(reader))return false;++unmodeled;
        }
        {SymbolOriginProbe reader;reader.enabled=false;reader.expectView=false;dx_data data;
            if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&data,false)||!reader.good||reader.callbacks!=4u||dx_iface::resourceOriginReport(data).records!=4u||ready(data)||dx_iface::tableReferenceReport(data).valueSourceTargetsMatched)return false;++optouts;}
        SymbolOriginProbe baseline;dx_data base;if(!baseline.fileImport(path.string(),&base,false)||!ready(base))return false;const auto r=dx_iface::resourceOriginReport(base);const std::size_t uses[]={r.records,r.bytes,r.work};
        for(unsigned which=0u;which<3u;++which)for(bool under:{false,true}) {
            SymbolOriginProbe reader;dx_data data;A::resourceOriginLimit(data,which,uses[which]-(under?1u:0u));
            if(!reader.fileImport(path.string(),&data,false)||(dx_iface::resourceOriginReport(data).state==S::OriginsBound)==under||!retry(reader))return false;++quotas;
        }
        for(unsigned fault=0u;fault<5u;++fault) {
            SymbolOriginProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!ready(data))return false;
            if(fault<4u)A::substituteResourceOrigin(data,fault/2u,fault%2u!=0u);else {dx_iface otherReader;dx_data other;if(!otherReader.fileImport(path.string(),&other,false)||!ready(other))return false;A::transplantResourceOrigins(data,other);}
            if(dx_iface::resourceOriginReport(data).state!=S::ContextMismatch||dx_iface::tableReferenceReport(data).sourceTargetsMatched||!retry(reader))return false;++contexts;
        }
        for(long offset=0;offset<32;++offset) {
            SymbolOriginProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!ready(data))return false;const auto before=symbolFailedAllocations;
            {SymbolAllocationFailure fail(offset);A::repeatResourceOrigins(reader,data);}
            if(symbolFailedAllocations!=before){if(dx_iface::resourceOriginReport(data).state!=S::CaptureFailed)return false;++reached;}else{if(dx_iface::resourceOriginReport(data).state!=S::OriginsBound)return false;++unreached;}
            if(!retry(reader))return false;
        }
    }
    for(bool binary:{false,true})for(unsigned family=0u;family<4u;++family)for(unsigned mode=0u;mode<6u;++mode) {
        auto tables=symbolOriginTables(48);const auto first=slot(tables,2,family==0u?"LTYPE":family==1u?"STYLE":family==2u?"LAYER":"APPID")-1u;auto end=first+1u;
        while(tables[end].code!=0||tables[end].value!="ENDTAB")++end;++end;
        Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1032"},{0,"ENDSEC"},{0,"SECTION"},{2,"TABLES"}};
        tags.insert(tags.end(),tables.begin()+static_cast<std::ptrdiff_t>(first),tables.begin()+static_cast<std::ptrdiff_t>(end));tags.insert(tags.end(),{{0,"ENDSEC"},{0,"EOF"}});
        if(!write(path,tags,binary))return false;dxfRW input(path.string().c_str());SymbolOriginProbe consumer;consumer.publicOnly=true;consumer.publicReader=&input;consumer.enabled=mode!=4u;consumer.publicationEnabled=mode!=5u;consumer.expectView=mode<4u;
        consumer.throwAt=mode==1u?3:mode==2u?1:0;consumer.disableSource=mode==3u;dxfRW::DxfResourceSourceView view;
        if(input.getDxfResourceSourceView(view))return false;bool threw=false;try{if(!input.read(&consumer,false))return false;}catch(const std::runtime_error&){threw=true;}
        if(threw!=(mode==1u||mode==2u)||!consumer.good||input.getDxfResourceSourceView(view)||view.model||view.source)return false;++lifetimes;
    }
    for(int flag:{0,16,32,48,64,80,96,112}) {
        DRW_AppId app;app.flags=flag;if(!DrwObjectEncodeTestAccess::appIdDxfValid(app)||DrwObjectEncodeTestAccess::appIdNativeValid(app)!=((flag&32)==0))return false;++flags;
    }
    // Check the widened DXF domain through actual output, independent physical
    // fields and re-import. Invalid bits still refuse before destination change;
    // the same exporter must recover with the documented flags restored.
    const auto output=dir/"appid-flags-output.dxf";
    for(bool binary:{false,true}) {
        dx_iface exporter;
        const auto roundtrip=[&](dx_data& data,int flag) {
            if(!exporter.fileExport(output.string(),DRW::AC1032,binary,&data,false))return false;
            dx_iface reader;dx_data readback;if(!reader.fileImport(output.string(),&readback,false))return false;
            const auto app=std::find_if(readback.appIds.begin(),readback.appIds.end(),[](const auto& v){return v.name=="REGISTERED_APP";});
            if(app==readback.appIds.end()||app->flags!=flag)return false;
            const auto wire=attdefWireTags(output,binary);unsigned fields=0u;
            for(std::size_t i=0u;i<wire.size();++i)if(wire[i].code==0&&wire[i].text=="APPID") {
                bool selected=false;std::size_t end=i+1u;while(end<wire.size()&&wire[end].code!=0){if(wire[end].code==2&&wire[end].text=="REGISTERED_APP")selected=true;++end;}
                if(selected)for(std::size_t n=i+1u;n<end;++n)if(wire[n].code==70){if(wire[n].integer!=flag)return false;++fields;}
            }
            return fields==1u;
        };
        for(int flag:{0,16,32,48,64,80,96,112}) {
            dx_data data;DRW_AppId app;app.name="REGISTERED_APP";app.flags=flag;data.appIds.push_back(app);
            if(!roundtrip(data,flag))return false;++outputs;
        }
        for(int flag:{-1,128}) {
            dx_data data;DRW_AppId app;app.name="REGISTERED_APP";app.flags=flag;data.appIds.push_back(app);
            const std::string sentinel="KEEP_INVALID_APPID_DESTINATION";{std::ofstream out(output,std::ios::binary);out<<sentinel;}
            if(DrwObjectEncodeTestAccess::appIdDxfValid(app)||DrwObjectEncodeTestAccess::appIdNativeValid(app)
                ||exporter.fileExport(output.string(),DRW::AC1032,binary,&data,false))return false;
            {std::ifstream in(output,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(in),{})!=sentinel)return false;}
            data.appIds.front().flags=48;if(!roundtrip(data,48))return false;++outputRefusals;
        }
    }
    std::cout<<"Symbol original profiles="<<profiles<<"; original markers="<<markers<<"; callback/cursor/throw refusals="<<refusals<<"; pre-staging pending refusals="<<pending
        <<"; field/owner refusals="<<unmodeled<<"; default-off controls="<<optouts<<"; exact/one-under limits="<<quotas<<"; immutable/cross-import refusals="<<contexts
        <<"; reached finalization failures="<<reached<<"; unreached offsets="<<unreached<<"; public lifetimes="<<lifetimes<<"; APPID flag domains="<<flags<<"; corrected imports="<<retries
        <<"; APPID physical output/readbacks="<<outputs<<"; invalid output/corrected retries="<<outputRefusals<<'\n';
    return profiles==768u&&markers==6144u&&refusals==72u&&pending==24u&&unmodeled==48u&&optouts==6u&&quotas==36u&&contexts==30u&&reached>0u&&reached+unreached==192u&&lifetimes==48u&&flags==8u&&retries==402u&&outputs==16u&&outputRefusals==4u;
}
