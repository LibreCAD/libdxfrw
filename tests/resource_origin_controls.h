// SPDX-License-Identifier: GPL-2.0-or-later
// Authored rich resources; no external drawing fixtures or payload derivatives.
Tags resourceOriginObjects(const char* field="FIELD") {
    return {{0,"TABLESTYLE"},{5,"704"},{330,"40"},{100,"AcDbTableStyle"},{280,"0"},{3,"authored style-é"},{70,"0"},{71,"0"},{40,"1"},{41,"1"},{280,"0"},{281,"0"},
        {0,field},{5,"705"},{330,"40"},{100,"AcDbField"},{1,"AcExpr"},{2,"1+1"},{90,"0"},{93,"0"},{97,"0"},
        {0,"XRECORD"},{5,"706"},{330,"40"},{100,"AcDbXrecord"},{280,"1"},{1,"value-é"},{10,"1"},{20,"2"},{30,"3"},
        {0,"DATALINK"},{5,"707"},{330,"40"},{100,"AcDbDataLink"},{1,"EXCEL"},{300,"authored data link"},{301,"tooltip"},{302,"local.xlsx"},
        {90,"0"},{91,"0"},{92,"0"},{93,"0"},{94,"0"}};
}
Tags resourceOriginDocument(const std::string& version,bool blocks,unsigned target,const char* field="FIELD") {
    auto tags=tableReferenceDocument(version,blocks,1u);auto end=slot(tags,2,"OBJECTS")+1u;while(tags[end].code!=0||tags[end].value!="ENDSEC")++end;
    const auto objects=resourceOriginObjects(field);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),objects.begin(),objects.end());
    const auto at=blockField(tags,"501",301);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{342,"704"},{344,"705"}});
    auto value=blockField(tags,"501",301)+1u;while(tags[value].code!=330)++value;tags[value].value=std::to_string(704u+target);
    return tags;
}
struct ResourceOriginProbe final:dx_iface {
    using V=dxfRW::DxfResourceSourceView;
    bool enabled=true,good=true,expectView=true,disableTyped=false,publicOnly=false;
    int skip=0,copy=0,duplicate=0,mutate=0,common=0,throwAt=0,allocationPhase=0;long allocation=-1;
    unsigned callbacks=0u;dxfRW* publicReader=nullptr;
    const DRW_TableEntry* original=nullptr;const DRW_RawDxfObject* raw=nullptr;
    std::vector<std::pair<unsigned,unsigned>> order;
    bool wantsDxfResourceSource() const override{return enabled&&(publicOnly||dx_iface::wantsDxfResourceSource());}
    void addHeader(const DRW_Header* v) override{if(!publicOnly)dx_iface::addHeader(v);}
    void inspect(const DRW_TableEntry* model,const DRW_RawDxfObject* source,unsigned kind,unsigned phase) {
        const auto* reader=publicReader?publicReader:(cData?DxIfaceTableSourceTestAccess::readInput(*cData):nullptr);V view;
        const auto before=symbolFailedAllocations;bool present=false;
        {SymbolAllocationFailure noAllocation(0);present=reader&&reader->getDxfResourceSourceView(view);}
        if(symbolFailedAllocations!=before||present!=expectView)good=false;
        if(present) {
            if(static_cast<unsigned>(view.kind)!=kind||static_cast<unsigned>(view.phase)!=phase||!view.model||!view.source
                ||(model&&view.model!=model)||(source&&view.source!=source))good=false;
            if(phase==1u){original=view.model;raw=view.source;}
            else if(view.typedAvailable&&(view.model!=original||view.source!=raw))good=false;
        }
        ++callbacks;order.emplace_back(kind,phase);
    }
    template<class Model,class Call>void typed(const Model& row,unsigned kind,Call call) {
        inspect(&row,nullptr,kind,1u);auto& value=const_cast<Model&>(row);const int flags=value.flags;
        const auto state=dxfBlockRecord::Decoder::observeCommonState(row);
        if(mutate==1)value.flags^=1;
        if(common==1)DrwObjectEncodeTestAccess::commonMask(value,static_cast<std::uint8_t>(state.second^1u));
        const auto invoke=[&](){if(copy==1){const Model clone(row);call(clone);}else call(row);};
        if(skip!=1){if(allocationPhase==1){SymbolAllocationFailure failure(allocation);invoke();}else invoke();if(duplicate==1)invoke();}
        value.flags=flags;DrwObjectEncodeTestAccess::commonMask(value,state.second);
        if(disableTyped)enabled=false;
        if(throwAt==1)throw std::runtime_error("resource typed publication");
    }
    void addTableStyle(const DRW_TableStyle& v) override{typed(v,1u,[&](const auto& row){dx_iface::addTableStyle(row);});}
    void addField(const DRW_Field& v) override{typed(v,2u,[&](const auto& row){dx_iface::addField(row);});}
    void addXRecord(const DRW_XRecord& v) override{typed(v,3u,[&](const auto& row){dx_iface::addXRecord(row);});}
    void addDataLink(const DRW_DataLink& v) override{typed(v,4u,[&](const auto& row){dx_iface::addDataLink(row);});}
    void addRawDxfObject(const DRW_RawDxfObject& v) override {
        unsigned kind=v.handle==0x704u?1u:v.handle==0x705u?2u:v.handle==0x706u?3u:v.handle==0x707u?4u:0u;
        if(!kind){dx_iface::addRawDxfObject(v);return;}
        inspect(nullptr,&v,kind,2u);
        const auto invoke=[&](){if(copy==2){const DRW_RawDxfObject clone(v);dx_iface::addRawDxfObject(clone);}else dx_iface::addRawDxfObject(v);};
        if(skip!=2){if(allocationPhase==2){SymbolAllocationFailure failure(allocation);invoke();}else invoke();if(duplicate==2)invoke();}
        original=nullptr;raw=nullptr;if(throwAt==2)throw std::runtime_error("resource raw publication");
    }
};
bool resourceOriginTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::ResourceOriginState;const auto path=dir/"resource-origins.dxf";
    unsigned profiles=0u,markers=0u,refusals=0u,contexts=0u,quotas=0u,reached=0u,unreached=0u,retries=0u,lifetimes=0u,pending=0u,rawOnly=0u;
    const auto ready=[](const dx_data& data){const auto r=dx_iface::resourceOriginReport(data);const auto refs=dx_iface::tableReferenceReport(data);
        return r.state==S::OriginsBound&&r.records==4u&&r.paired==4u&&r.stateComplete==4u&&r.bodyMatched==4u&&r.originsBound==4u
            &&refs.sourceTargetsMatched&&refs.valueSourceTargetsMatched&&refs.references==4u&&!r.ownerTargetsResolved&&!r.outputEligible&&!refs.outputEligible;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)
        for(bool blocks:{false,true})for(const char* alias:{"FIELD","ACDBFIELD","ACAD_FIELD"})for(unsigned target=0u;target<4u;++target) {
            const auto tags=resourceOriginDocument(version,blocks,target,alias);ResourceOriginProbe reader;dx_data data;
            if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data)||!reader.good||reader.callbacks!=8u) {
                const auto r=dx_iface::resourceOriginReport(data);const auto refs=dx_iface::tableReferenceReport(data);
                std::cerr<<"Resource origins "<<version<<'/'<<encoding<<" blocks="<<blocks<<" alias="<<alias<<" target="<<target<<" state="<<static_cast<int>(r.state)
                    <<" records="<<r.records<<" paired="<<r.paired<<" body="<<r.bodyMatched<<" origins="<<r.originsBound<<" refs="<<static_cast<int>(refs.state)<<'\n';return false;
            }
            const auto wire=attdefWireTags(path,encoding==2u);const auto& physical=*A::identityInput(data)->source;
            for(const auto& item:A::resourceOrigins(data)) {
                const auto& a=*item.second;const auto& record=physical.records.at(a.physical);const auto& marker=wire.at(record.marker.ordinal-1u);
                if(marker.code||marker.text!=record.name||marker.begin!=record.marker.beginOffset||marker.end!=record.marker.endOffset
                    ||a.input->model.get()!=A::resources(data).at(item.first).model.get()||a.input->raw.get()!=A::resources(data).at(item.first).raw.get())return false;
                ++markers;
            }
            for(unsigned i=0u;i<8u;++i)if(reader.order[i]!=std::make_pair(i/2u+1u,i%2u+1u))return false;++profiles;
        }
    for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true}) {
        const auto good=resourceOriginDocument("AC1032",blocks,0u);
        const auto retry=[&](dx_iface& reader){dx_data next;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&next,false)||!ready(next))return false;++retries;return true;};
        for(unsigned mode=0u;mode<10u;++mode) {
            ResourceOriginProbe reader;const int phase=mode%2u+1u;
            if(mode<2u)reader.skip=phase;else if(mode<4u)reader.copy=phase;else if(mode<6u)reader.duplicate=phase;
            else if(mode==6u)reader.mutate=1;else if(mode==7u)reader.common=1;else reader.throwAt=phase;
            dx_data data;if(!writeBlockBinding(path,good,encoding))return false;bool read=false;
            try{read=reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){}
            const auto r=dx_iface::resourceOriginReport(data);
            if(mode>=8u?(r.state!=S::ImportFailed):(mode>=6u?(r.state!=S::Unqualified):(r.state!=S::CaptureFailed))) {
                std::cerr<<"Resource origin refusal mode="<<mode<<" state="<<static_cast<int>(r.state)<<" read="<<read<<'\n';return false;
            }
            if(dx_iface::tableReferenceReport(data).sourceTargetsMatched||!A::resourcePendingEmpty(data))return false;
            reader.skip=reader.copy=reader.duplicate=reader.mutate=reader.common=reader.throwAt=0;
            if(!retry(reader))return false;++refusals;
        }
        for(unsigned fault=0u;fault<5u;++fault) {
            ResourceOriginProbe reader;dx_data data;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data))return false;
            if(fault<4u)A::substituteResourceOrigin(data,fault/2u,fault%2u!=0u);
            else {dx_iface another;dx_data other;if(!another.fileImport(path.string(),&other,false)||!ready(other))return false;A::transplantResourceOrigins(data,other);}
            if(dx_iface::resourceOriginReport(data).state!=S::ContextMismatch||dx_iface::tableReferenceReport(data).sourceTargetsMatched||!retry(reader))return false;++contexts;
        }
        {auto tags=good;appendBlockFields(tags,"704",{{1001,"APP"},{1010,"1"}});ResourceOriginProbe reader;dx_data data;
            if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||dx_iface::resourceOriginReport(data).state!=S::Unqualified||!retry(reader))return false;++pending;}
        {auto tags=good;appendBlockFields(tags,"704",{{7,"STANDARD"}});ResourceOriginProbe reader;dx_data data;
            if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!reader.good||reader.callbacks!=7u)return false;
            const auto r=dx_iface::resourceOriginReport(data);const auto refs=dx_iface::tableReferenceReport(data);
            if(r.rawOnly!=1u||r.records!=3u||r.originsBound!=3u||refs.sourceTargetsMatched||refs.unavailable!=2u||!retry(reader))return false;++rawOnly;}
        ResourceOriginProbe baseline;dx_data base;if(!writeBlockBinding(path,good,encoding)||!baseline.fileImport(path.string(),&base,false)||!ready(base))return false;
        const auto r=dx_iface::resourceOriginReport(base);const std::size_t uses[]={r.records,r.bytes,r.work};
        for(unsigned which=0u;which<3u;++which)for(bool under:{false,true}) {
            ResourceOriginProbe reader;dx_data data;A::resourceOriginLimit(data,which,uses[which]-(under?1u:0u));
            if(!reader.fileImport(path.string(),&data,false)||(dx_iface::resourceOriginReport(data).state==S::OriginsBound)==under||!retry(reader))return false;++quotas;
        }
        for(long offset=0;offset<32;++offset) {
            ResourceOriginProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!ready(data))return false;
            const auto before=symbolFailedAllocations;{SymbolAllocationFailure fail(offset);A::repeatResourceOrigins(reader,data);}
            if(symbolFailedAllocations!=before){if(dx_iface::resourceOriginReport(data).state!=S::CaptureFailed)return false;++reached;}
            else {if(dx_iface::resourceOriginReport(data).state!=S::OriginsBound)return false;++unreached;}
            if(!retry(reader))return false;
        }
    }
    // Public view lifetime/latched selection over one complete FIELD pair.
    for(bool binary:{false,true})for(unsigned mode=0u;mode<5u;++mode) {
        auto objects=resourceOriginObjects();const auto first=slot(objects,5,"705")-1u,last=slot(objects,5,"706")-1u;
        Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1032"},{0,"ENDSEC"},{0,"SECTION"},{2,"OBJECTS"}};
        tags.insert(tags.end(),objects.begin()+static_cast<std::ptrdiff_t>(first),objects.begin()+static_cast<std::ptrdiff_t>(last));tags.insert(tags.end(),{{0,"ENDSEC"},{0,"EOF"}});
        if(!write(path,tags,binary))return false;dxfRW input(path.string().c_str());ResourceOriginProbe consumer;consumer.publicOnly=true;consumer.publicReader=&input;
        consumer.enabled=mode!=4u;consumer.expectView=mode!=4u;consumer.throwAt=mode==1u?1:mode==2u?2:0;consumer.disableTyped=mode==3u;
        dxfRW::DxfResourceSourceView view;view.model=reinterpret_cast<const DRW_TableEntry*>(1);
        if(input.getDxfResourceSourceView(view)||view.model||view.source)return false;
        bool threw=false;try{if(!input.read(&consumer,false))return false;}catch(const std::runtime_error&){threw=true;}
        if(threw!=(mode==1u||mode==2u)||!consumer.good||input.getDxfResourceSourceView(view)||view.model||view.source)return false;++lifetimes;
    }
    std::cout<<"Resource original profiles="<<profiles<<"; original markers="<<markers<<"; callback/cursor/throw refusals="<<refusals
        <<"; immutable/cross-import refusals="<<contexts<<"; pending-state refusals="<<pending<<"; raw-only fallbacks="<<rawOnly<<"; exact/one-under limits="<<quotas
        <<"; reached finalization failures="<<reached<<"; unreached offsets="<<unreached<<"; public lifetimes="<<lifetimes<<"; corrected imports="<<retries<<'\n';
    return profiles==288u&&markers==1152u&&refusals==60u&&contexts==30u&&pending==6u&&rawOnly==6u&&quotas==36u&&reached>0u&&reached+unreached==192u&&lifetimes==10u&&retries==330u;
}
