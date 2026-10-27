// SPDX-License-Identifier: GPL-2.0-or-later
// Runtime-authored VIEW/UCS targets, not external drawing fixtures.
Tags viewUcsTables(unsigned flags,bool rich) {
    Tags tags{{0,"TABLE"},{2,"VIEW"},{5,"E700"},{330,"0"},{100,"AcDbSymbolTable"},{70,"1"},
        {0,"VIEW"},{5,"E701"},{330,"E700"},{100,"AcDbSymbolTableRecord"},{100,"AcDbViewTableRecord"},{2,"LOOKUP-é"},{70,std::to_string(flags|(rich?1u:0u))},
        {40,"20"},{41,"30"},{10,"1"},{20,"2"},{11,"0"},{21,"0"},{31,"1"},{12,"0"},{22,"0"},{32,"0"},
        {42,"50"},{43,"0"},{44,"100"},{50,"15"},{71,"0"},{281,"0"},{72,"1"},{73,"1"},
        {110,"0"},{120,"0"},{130,"0"},{111,"1"},{121,"0"},{131,"0"},{112,"0"},{122,"1"},{132,"0"},{79,"1"},{146,"2.5"},{345,"E711"},{346,"E712"},{0,"ENDTAB"},
        {0,"TABLE"},{2,"UCS"},{5,"E710"},{330,"0"},{100,"AcDbSymbolTable"},{70,"2"}};
    for(const char* handle:{"E711","E712"}) {
        const bool named=handle==std::string("E711");tags.insert(tags.end(),{{0,"UCS"},{5,handle},{330,"E710"},{100,"AcDbSymbolTableRecord"},{100,"AcDbUCSTableRecord"},{2,named?"UCS-é":"BASE"},{70,std::to_string(flags)},
            {10,"0"},{20,"0"},{30,"0"},{11,"1"},{21,"0"},{31,"0"},{12,"0"},{22,"1"},{32,"0"},{79,named?"1":"0"},{146,"0"}});
        if(named)tags.push_back({346,"E712"});
        if(rich)tags.insert(tags.end(),{{71,"5"},{13,"7"},{23,"8"},{33,"9"},{71,"2"},{13,"1"},{23,"2"},{33,"3"}});
    }
    tags.push_back({0,"ENDTAB"});return tags;
}
Tags viewUcsDocument(const std::string& version,bool blocks,unsigned flags,bool rich) {
    auto tags=layoutDependencyDocument(version,blocks,0u);const auto tables=viewUcsTables(flags,rich);auto at=slot(tags,2,"TABLES")+1u;
    while(tags[at].code!=0||tags[at].value!="ENDSEC")++at;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),tables.begin(),tables.end());
    layoutDependencySet(tags,"AcDbPlotSettings",74,"3");layoutDependencySet(tags,"AcDbPlotSettings",6,rich?"lOoKuP-\\U+00E9":"LOOKUP-é");
    layoutDependencySet(tags,"AcDbLayout",76,"1");layoutDependencySet(tags,"AcDbLayout",345,"E711");layoutDependencySet(tags,"AcDbLayout",346,"E712");
    for(const char* handle:{"E701","E711","E712"})appendBlockFields(tags,handle,{{1001,"REGISTERED_APP"},{1003,"0"},{1005,"705"},{1010,"1"},{1020,"2"},{1030,"3"},{1070,"7"}});
    return tags;
}
struct ViewUcsProbe:dx_iface {
    using V=dxfRW::DxfViewUcsSourceView;
    bool enabled=true,publicOnly=false,expectView=true,good=true,disableSource=false;
    int skip=0,copy=0,duplicate=0,mutate=0,cursor=0,throwAt=0,allocationPhase=0;long allocation=-1;unsigned callbacks=0u;bool allocationReached=false;
    dxfRW* publicReader=nullptr;const DRW_TableEntry* model=nullptr;const DRW_RawDxfObject* raw=nullptr;
    bool wantsDxfViewUcsSource() const override{return enabled&&(publicOnly||dx_iface::wantsDxfViewUcsSource());}
    void addHeader(const DRW_Header* h) override{if(!publicOnly)dx_iface::addHeader(h);}
    void inspect(const DRW_TableEntry& v,const DRW_RawDxfObject* source,unsigned phase) {
        const auto* input=publicReader?publicReader:(cData?DxIfaceTableSourceTestAccess::readInput(*cData):nullptr);V view;
        const auto before=symbolFailedAllocations;bool present=false;dxfRW::DxfResourceSourceView legacy;dxfRW::DxfPlotStyleSourceView plot;legacy.model=plot.model=&v;legacy.source=plot.source=source;
        {SymbolAllocationFailure none(0);present=input&&input->getDxfViewUcsSourceView(view);
            if(input&&(input->getDxfResourceSourceView(legacy)||input->getDxfPlotStyleSourceView(plot)))good=false;}
        if(present!=expectView||symbolFailedAllocations!=before||legacy.model||legacy.source||plot.model||plot.source)good=false;
        if(present){if(view.model!=&v||!view.source||(source&&view.source!=source)||static_cast<unsigned>(view.phase)!=phase
                ||view.kind!=(dynamic_cast<const DRW_View*>(&v)?V::Kind::View:V::Kind::Ucs)||!view.originalPayloadComplete)good=false;
            if(phase==3u){model=&v;raw=view.source;}else if(model!=&v||raw!=view.source)good=false;}
        ++callbacks;
    }
    void addDxfViewUcsSource(const DRW_TableEntry& v,const DRW_RawDxfObject& r) override {
        inspect(v,&r,3u);auto& actual=const_cast<DRW_TableEntry&>(v);const auto flags=actual.flags;const auto state=dxfViewUcs::Decoder::observe(v);
        if(allocationPhase==3&&v.handle==0xE701u){const auto before=symbolFailedAllocations;{SymbolAllocationFailure fault(allocation);dx_iface::addDxfViewUcsSource(v,r);}allocationReached=symbolFailedAllocations!=before;return;}
        if(mutate==3)actual.flags^=16;if(cursor==3)DrwObjectEncodeTestAccess::commonMask(actual,static_cast<std::uint8_t>(state.second^1u));
        if(!publicOnly&&skip!=3){if(copy==3){const DRW_RawDxfObject clone(r);dx_iface::addDxfViewUcsSource(v,clone);}else dx_iface::addDxfViewUcsSource(v,r);
            if(duplicate==3)dx_iface::addDxfViewUcsSource(v,r);}
        actual.flags=flags;DrwObjectEncodeTestAccess::commonMask(actual,state.second);
        if(disableSource)enabled=false;if(throwAt==3)throw std::runtime_error("VIEW/UCS source publication");
    }
    template<class Model,class Call>void typed(const Model& v,Call call) {
        inspect(v,nullptr,1u);auto& actual=const_cast<Model&>(v);const auto flags=actual.flags;const auto state=dxfViewUcs::Decoder::observe(v);
        // Isolate the actual observer from later legacy live-container copies.
        if(allocationPhase==1&&v.handle==0xE701u){const auto before=symbolFailedAllocations;{SymbolAllocationFailure fault(allocation);DxIfaceTableSourceTestAccess::observeViewUcs(*this,v);}allocationReached=symbolFailedAllocations!=before;model=nullptr;raw=nullptr;return;}
        if(mutate==1)actual.flags^=16;if(cursor==1)DrwObjectEncodeTestAccess::commonMask(actual,static_cast<std::uint8_t>(state.second^1u));
        if(!publicOnly&&skip!=1){if(copy==1){const Model clone(v);call(clone);}else call(v);if(duplicate==1)call(v);}
        actual.flags=flags;DrwObjectEncodeTestAccess::commonMask(actual,state.second);model=nullptr;raw=nullptr;
        if(throwAt==1)throw std::runtime_error("VIEW/UCS typed publication");
    }
    void addView(const DRW_View& v) override{typed(v,[&](const auto& x){dx_iface::addView(x);});}
    void addUCS(const DRW_UCS& v) override{typed(v,[&](const auto& x){dx_iface::addUCS(x);});}
    void resetFaults(){skip=copy=duplicate=mutate=cursor=throwAt=allocationPhase=0;allocation=-1;enabled=expectView=true;}
};
bool viewUcsSourceTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::ViewUcsState;using Role=dx_viewucs_source::Role;
    const auto path=dir/"view-ucs-sources.dxf";unsigned profiles=0u,markers=0u,immutable=0u,pairs=0u,fields=0u,quotas=0u,contexts=0u,imports=0u,lifetimes=0u,outputs=0u,outputRefusals=0u,reached=0u,unreached=0u,finalizations=0u,optional=0u,captureFailures=0u,captureUnreached=0u;std::size_t spans=0u;
    const auto fail=[&](int line){std::cerr<<"VIEW/UCS check line="<<line<<" profiles="<<profiles<<" pairs="<<pairs<<" fields="<<fields<<" quotas="<<quotas<<" contexts="<<contexts<<" outputs="<<outputs<<'\n';return false;};
    const auto ready=[](const dx_data& d){const auto s=dx_iface::viewUcsSourceReport(d);const auto r=dx_iface::dependencyReport(d);return s.originalSourcesMatched&&s.records==3u&&s.paired==3u&&s.bodies==3u&&s.origins==3u
        &&r.originalTargetsMatched&&r.nameNamespacesComplete&&!r.ownerTargetsResolved&&!r.recursiveClosureMatched&&!r.outputEligible;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true})for(unsigned flags:{0u,32u,112u})for(bool rich:{false,true}) {
        const auto tags=viewUcsDocument(version,blocks,flags,rich);ViewUcsProbe reader;dx_data data;
        if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data)||!reader.good||reader.callbacks!=6u) {
            const auto s=dx_iface::viewUcsSourceReport(data);const auto r=dx_iface::dependencyReport(data);
            std::cerr<<"VIEW/UCS profile "<<version<<'/'<<encoding<<" blocks="<<blocks<<" flags="<<flags<<" rich="<<rich<<" source="<<static_cast<int>(s.state)
                <<" bodies="<<s.bodies<<" origins="<<s.origins<<" dependency="<<static_cast<int>(r.state)<<" first="<<std::hex<<r.firstSource<<" -> "<<r.firstTarget<<std::dec<<'\n';
            if(A::viewUcs(data))for(const auto& p:A::viewUcs(data)->records)std::cerr<<" record="<<std::hex<<p.first<<std::dec<<" profile="<<p.second->profile.complete<<" rejected="<<p.second->profile.rejected<<" body="<<p.second->bodyMatched<<'\n';
            return false;
        }
        const auto wire=attdefWireTags(path,encoding==2u);const auto& physical=*A::identityInput(data)->source;
        for(const auto& item:A::viewUcs(data)->records){const auto& row=*item.second;const auto& original=physical.records.at(row.physical);const auto& marker=wire.at(original.marker.ordinal-1u);
            if(marker.code||marker.text!=(row.input->view?"VIEW":"UCS")||marker.begin!=original.marker.beginOffset||marker.end!=original.marker.endOffset)return fail(__LINE__);++markers;
            if(row.profile.spans.size()!=row.input->raw->groups.size())return fail(__LINE__);
            for(const auto& span:row.profile.spans){const auto& w=wire.at(original.marker.ordinal+span.group);const auto& v=row.input->raw->groups.at(span.group);
                if(w.code!=span.code||w.code!=v.code())return fail(__LINE__);
                if(w.kind==AttdefWireTag::Integer){if(v.type()!=DRW_Variant::INTEGER||v.i_val()!=w.integer)return fail(__LINE__);}
                else if(w.kind==AttdefWireTag::Real){const auto value=v.d_val();if(v.type()!=DRW_Variant::DOUBLE||std::memcmp(&value,&w.real,sizeof(value)))return fail(__LINE__);}
                else if(v.type()!=DRW_Variant::STRING||!v.content.s||*v.content.s!=w.text)return fail(__LINE__);
                Role role=Role::None;switch(w.code){case 5:role=Role::Self;break;case 330:role=Role::Owner;break;case 2:role=Role::Name;break;case 345:role=Role::NamedUcs;break;
                    case 346:role=Role::BaseUcs;break;case 1001:role=Role::AppId;break;case 1003:role=Role::Layer;break;case 1005:role=Role::Object;break;default:break;}
                if(span.role!=role)return fail(__LINE__);if(role==Role::Self||role==Role::Owner||role==Role::NamedUcs||role==Role::BaseUcs||role==Role::Object){
                    std::uint32_t target=0u;const auto parsed=std::from_chars(w.text.data(),w.text.data()+w.text.size(),target,16);
                    if(parsed.ec!=std::errc{}||parsed.ptr!=w.text.data()+w.text.size()||target!=span.target)return fail(__LINE__);}
                ++spans;
            }
        }
        const auto before=symbolFailedAllocations;{SymbolAllocationFailure none(0);if(!ready(data))return fail(__LINE__);}if(symbolFailedAllocations!=before)return fail(__LINE__);
        for(auto& v:data.views){v.name="edited live view";v.namedUCS_ID=0xFFFFu;v.hasUCS=false;}
        for(auto& u:data.ucsList){u.name="edited live UCS";u.origin.x+=12.;u.orthographicOptions.clear();}
        A::clearAdvisoryNames(data);if(!ready(data))return fail(__LINE__);++immutable;++profiles;
    }
    for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true}) {
        const auto good=viewUcsDocument("AC1032",blocks,48u,true);
        const auto retry=[&](dx_iface& reader){dx_data next;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&next,false)||!ready(next))return fail(__LINE__);++imports;return true;};
        for(unsigned mode=0u;mode<13u;++mode){ViewUcsProbe reader;const int phase=mode%2u?1:3;
            if(mode<2u)reader.skip=phase;else if(mode<4u)reader.copy=phase;else if(mode<6u)reader.duplicate=phase;else if(mode<8u)reader.mutate=phase;else if(mode<10u)reader.cursor=phase;
            else if(mode<12u)reader.throwAt=phase;else {reader.enabled=false;reader.expectView=false;}
            dx_data data;bool caught=false;if(!writeBlockBinding(path,good,encoding))return fail(__LINE__);
            try{if(!reader.fileImport(path.string(),&data,false))return fail(__LINE__);}catch(const std::runtime_error&){caught=true;}
            const auto report=dx_iface::viewUcsSourceReport(data);const auto expected=mode>=10u&&mode<12u?S::ImportFailed:mode>=6u?S::Unqualified:S::CaptureFailed;
            if(caught!=(mode>=10u&&mode<12u)||report.state!=expected||ready(data)||!reader.good){std::cerr<<"pair mode="<<mode<<" state="<<static_cast<int>(report.state)<<'\n';return fail(__LINE__);}
            reader.resetFaults();if(!retry(reader))return fail(__LINE__);++pairs;
        }
        for(unsigned fault=0u;fault<15u;++fault){auto tags=good;bool parseFailure=false;
            if(fault==0u)appendBlockFields(tags,"E701",{{99,"7"}});
            else if(fault==1u)tags[blockField(tags,"E701",330)].value="E710";
            else if(fault==2u)tags[blockField(tags,"E701",72)].value="0";
            else if(fault==3u){tags[blockField(tags,"E711",79)].value="0";}
            else if(fault==4u)tags[blockField(tags,"E701",345)].value="FFFF";
            else if(fault==5u)tags[blockField(tags,"E701",345)].value="40";
            else if(fault==6u)tags[blockField(tags,"E701",1001)].value="MISSING_APP";
            else if(fault==7u)tags[blockField(tags,"E711",1003)].value="MISSING_LAYER";
            else if(fault==8u){const auto at=slot(tags,5,"E701")-1u;auto end=at+1u;while(tags[end].code!=0)++end;Tags clone(tags.begin()+static_cast<std::ptrdiff_t>(at),tags.begin()+static_cast<std::ptrdiff_t>(end));
                clone[1u].value="E702";tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),clone.begin(),clone.end());}
            else if(fault==9u)layoutDependencySet(tags,"AcDbPlotSettings",6,"LOOKUP-É");
            else if(fault==10u){const auto at=blockField(tags,"E711",33);tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at));parseFailure=true;}
            else if(fault==11u)appendBlockFields(tags,"E701",{{102,"{OPAQUE"},{330,"DEAD"},{102,"}"}});
            else if(fault==12u){const auto at=blockField(tags,"E712",1001);tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at),tags.begin()+static_cast<std::ptrdiff_t>(at+7u));appendBlockFields(tags,"E712",{{345,"E711"}});parseFailure=true;}
            else appendBlockFields(tags,fault==13u?"E701":"E711",{{1001,"REGISTERED_APP"},{1010,"1"}});
            ViewUcsProbe reader;dx_data data;if(!writeBlockBinding(path,tags,encoding))return fail(__LINE__);const bool parsed=reader.fileImport(path.string(),&data,false);
            if(parsed==parseFailure||ready(data)){std::cerr<<"field fault="<<fault<<" parsed="<<parsed<<'\n';return fail(__LINE__);}
            if(!retry(reader))return fail(__LINE__);++fields;
        }
        dx_iface reader;dx_data base;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&base,false)||!ready(base))return fail(__LINE__);
        const auto report=dx_iface::viewUcsSourceReport(base);const std::size_t limits[]{report.records,A::viewUcsGroups(base),report.bytes,report.work};
        for(unsigned which=0u;which<4u;++which)for(bool under:{false,true}){dx_iface codec;dx_data actual;A::viewUcsLimit(actual,which,limits[which]-(under?1u:0u));
            if(!codec.fileImport(path.string(),&actual,false)||ready(actual)==under||(under&&dx_iface::viewUcsSourceReport(actual).state!=S::CaptureFailed)||!retry(codec))return fail(__LINE__);++quotas;}
        for(unsigned fault=0u;fault<6u;++fault){dx_data actual;if(!reader.fileImport(path.string(),&actual,false)||!ready(actual))return fail(__LINE__);
            if(fault<4u)A::substituteViewUcs(actual,fault/2u,fault%2u!=0u);else {dx_iface other;dx_data d;if(!other.fileImport(path.string(),&d,false)||!ready(d))return fail(__LINE__);A::transplantViewUcs(actual,d,fault==5u);}
            if(dx_iface::viewUcsSourceReport(actual).state!=S::ContextMismatch||dx_iface::dependencyReport(actual).originalTargetsMatched||!retry(reader))return fail(__LINE__);++contexts;}
        for(int phase:{3,1})for(long offset=0;offset<8;++offset){ViewUcsProbe probe;probe.allocationPhase=phase;probe.allocation=offset;dx_data actual;
            if(!probe.fileImport(path.string(),&actual,false))return fail(__LINE__);
            if(probe.allocationReached){if(dx_iface::viewUcsSourceReport(actual).state!=S::CaptureFailed||ready(actual))return fail(__LINE__);++captureFailures;}
            else {if(!ready(actual))return fail(__LINE__);++captureUnreached;}
            probe.resetFaults();if(!retry(probe))return fail(__LINE__);}
        for(unsigned form=0u;form<4u;++form){auto tags=good;
            if(form==0u){tags[blockField(tags,"E701",72)].value="0";const auto first=blockField(tags,"E701",110),last=blockField(tags,"E701",346);tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(first),tags.begin()+static_cast<std::ptrdiff_t>(last+1u));}
            else if(form<=2u){auto at=blockField(tags,"E701",346);tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at));at=blockField(tags,"E701",345);tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at));if(form==2u)tags[blockField(tags,"E701",79)].value="0";}
            else for(const char* handle:{"E701","E711","E712"})tags[slot(tags,5,handle)-1u].value=handle==std::string("E701")?"vIeW":"uCs";
            dx_data actual;if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&actual,false)||!ready(actual)){
                const auto source=dx_iface::viewUcsSourceReport(actual);const auto dependency=dx_iface::dependencyReport(actual);
                std::cerr<<"optional form="<<form<<" source="<<static_cast<int>(source.state)<<" bodies="<<source.bodies<<" dependency="<<static_cast<int>(dependency.state)<<" first="<<std::hex<<dependency.firstSource<<" -> "<<dependency.firstTarget<<std::dec<<'\n';return fail(__LINE__);}++optional;}
        if(!writeBlockBinding(path,good,encoding))return fail(__LINE__);
        for(long offset=0;offset<64;++offset){const auto before=symbolFailedAllocations;{SymbolAllocationFailure fault(offset);A::repeatViewUcs(reader,base);}
            if(symbolFailedAllocations!=before){if(dx_iface::viewUcsSourceReport(base).state!=S::CaptureFailed)return fail(__LINE__);++reached;}
            else {if(!dx_iface::viewUcsSourceReport(base).originalSourcesMatched)return fail(__LINE__);++unreached;}
            A::repeatViewUcs(reader,base);if(!dx_iface::viewUcsSourceReport(base).originalSourcesMatched)return fail(__LINE__);++finalizations;}
    }
    for(bool binary:{false,true})for(bool ucs:{false,true})for(unsigned mode=0u;mode<5u;++mode){auto tables=viewUcsTables(48u,true);
        const auto first=slot(tables,2,ucs?"UCS":"VIEW")-1u;auto end=first+1u;while(tables[end].code!=0||tables[end].value!="ENDTAB")++end;++end;
        Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1032"},{0,"ENDSEC"},{0,"SECTION"},{2,"TABLES"}};
        tags.insert(tags.end(),tables.begin()+static_cast<std::ptrdiff_t>(first),tables.begin()+static_cast<std::ptrdiff_t>(end));tags.insert(tags.end(),{{0,"ENDSEC"},{0,"EOF"}});
        if(!write(path,tags,binary))return fail(__LINE__);dxfRW input(path.string().c_str());ViewUcsProbe consumer;consumer.publicOnly=true;consumer.publicReader=&input;
        consumer.enabled=mode!=4u;consumer.expectView=mode<4u;consumer.throwAt=mode==1u?3:mode==2u?1:0;consumer.disableSource=mode==3u;ViewUcsProbe::V view;
        DRW_View sentinel;DRW_RawDxfObject rawSentinel;view.model=&sentinel;view.source=&rawSentinel;
        if(input.getDxfViewUcsSourceView(view)||view.model||view.source)return fail(__LINE__);bool caught=false;try{if(!input.read(&consumer,false))return fail(__LINE__);}catch(const std::runtime_error&){caught=true;}
        view.model=&sentinel;view.source=&rawSentinel;
        if(caught!=(mode==1u||mode==2u)||!consumer.good||input.getDxfViewUcsSourceView(view)||view.model||view.source)return fail(__LINE__);++lifetimes;
    }
    const auto output=dir/"view-ucs-flags-output.dxf";
    for(bool binary:{false,true}){dx_iface exporter;const auto roundtrip=[&](dx_data& data,int flag){if(!exporter.fileExport(output.string(),DRW::AC1032,binary,&data,false)){std::cerr<<"VIEW/UCS export flags="<<flag<<'\n';return false;}
            dx_iface reader;dx_data readback;if(!reader.fileImport(output.string(),&readback,false)||readback.views.size()!=1u||readback.ucsList.size()!=1u
                ||readback.views.front().flags!=(flag|1)||readback.ucsList.front().flags!=flag)return false;
            const auto wire=attdefWireTags(output,binary);unsigned found=0u;for(std::size_t i=0u;i<wire.size();++i)if(wire[i].code==0&&(wire[i].text=="VIEW"||wire[i].text=="UCS")){
                const bool view=wire[i].text=="VIEW";for(std::size_t n=i+1u;n<wire.size()&&wire[n].code!=0;++n)if(wire[n].code==70){if(wire[n].integer!=(view?(flag|1):flag))return false;++found;}}
            return found==2u;};
        for(int flag:{0,16,32,48,64,80,96,112}){dx_data data;DRW_View view;view.name="FLAGS_VIEW";view.handle=0xE701u;view.flags=flag|1;DRW_UCS ucs;ucs.name="FLAGS_UCS";ucs.handle=0xE711u;ucs.flags=flag;
            if(!DrwObjectEncodeTestAccess::viewDxfValid(view)||!DrwObjectEncodeTestAccess::ucsDxfValid(ucs)
                ||DrwObjectEncodeTestAccess::viewNativeValid(view)!=((flag&32)==0)||DrwObjectEncodeTestAccess::ucsNativeValid(ucs)!=((flag&32)==0))return fail(__LINE__);
            data.views.push_back(view);data.ucsList.push_back(ucs);if(!roundtrip(data,flag))return fail(__LINE__);++outputs;}
        for(bool ucs:{false,true})for(int flag:{-1,128}){dx_data data;DRW_View view;view.name="FLAGS_VIEW";view.handle=0xE701u;view.flags=ucs?49:flag;DRW_UCS item;item.name="FLAGS_UCS";item.handle=0xE711u;item.flags=ucs?flag:48;
            data.views.push_back(view);data.ucsList.push_back(item);const std::string sentinel="KEEP_INVALID_VIEW_UCS_DESTINATION";{std::ofstream out(output,std::ios::binary);out<<sentinel;}
            if(exporter.fileExport(output.string(),DRW::AC1032,binary,&data,false))return fail(__LINE__);
            {std::ifstream in(output,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(in),{})!=sentinel)return fail(__LINE__);}
            data.views.front().flags=49;data.ucsList.front().flags=48;if(!roundtrip(data,48))return fail(__LINE__);++outputRefusals;}
    }
    std::cout<<"VIEW/UCS source profiles="<<profiles<<"; original markers="<<markers<<"; scalar/reference spans="<<spans<<"; live-state independence="<<immutable
        <<"; callback/state/throw refusals="<<pairs<<"; field/namespace refusals="<<fields<<"; exact/one-under limits="<<quotas<<"; context refusals="<<contexts
        <<"; reached finalization failures="<<reached<<"; unreached offsets="<<unreached<<"; corrected imports="<<imports<<"; corrected finalizations="<<finalizations
        <<"; public lifetimes="<<lifetimes<<"; optional/case profiles="<<optional<<"; capture allocation failures="<<captureFailures<<"; capture unreached offsets="<<captureUnreached
        <<"; flag output/readbacks="<<outputs<<"; output refusal/retries="<<outputRefusals<<'\n';
    return profiles==144u&&markers==432u&&spans==14544u&&immutable==144u&&pairs==78u&&fields==90u&&quotas==48u&&contexts==36u&&reached>0u&&reached+unreached==384u
        &&imports==348u&&finalizations==384u&&lifetimes==20u&&optional==24u&&captureFailures>0u&&captureFailures+captureUnreached==96u&&outputs==16u&&outputRefusals==8u;
}
