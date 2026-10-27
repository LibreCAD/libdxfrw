// SPDX-License-Identifier: GPL-2.0-or-later
// Authored original LAYOUT dependency recipes, not drawing fixtures.
std::size_t layoutDependencyField(const Tags& tags,const char* scope,int code) {
    auto at=slot(tags,5,"A0");while(tags[at].code!=100||tags[at].value!=scope)++at;++at;
    while(tags[at].code!=0&&tags[at].code!=100&&tags[at].code!=1001){if(tags[at].code==code)return at;++at;}return at;
}
void layoutDependencySet(Tags& tags,const char* scope,int code,const std::string& value) {
    const auto at=layoutDependencyField(tags,scope,code);if(tags[at].code==code)tags[at].value=value;else tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{code,value}});
}
void layoutDependencyErase(Tags& tags,const char* scope,int code) {
    const auto at=layoutDependencyField(tags,scope,code);if(tags[at].code==code)tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at));
}
void layoutDependencyHeader(Tags& tags,const Tags& fields) {
    auto at=slot(tags,5,"A0");while(tags[at].code!=100)++at;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),fields.begin(),fields.end());
}
Tags layoutDependencyDocument(const std::string& version,bool blocks,unsigned form) {
    auto tags=entityDependencyDocument(version,blocks,form==3u?1u:0u);
    for(const char* handle:{"A0","A1","A2"})appendBlockFields(tags,handle,{{1001,"REGISTERED_APP"},{1003,form==3u?"lAyEr-\\U+00E9":"0"},{1005,"705"}});
    layoutDependencyHeader(tags,{{102,"{ACAD_REACTORS"},{330,"705"},{102,"}"}});
    layoutDependencySet(tags,"AcDbLayout",331,"0");
    if(form!=2u) {
        layoutDependencySet(tags,"AcDbPlotSettings",74,form==0u?"0":form==1u?"5":"4");
        layoutDependencySet(tags,"AcDbPlotSettings",6,form==1u?"":"stale-view-é");
        layoutDependencySet(tags,"AcDbPlotSettings",76,form==1u?"3":"0");
        layoutDependencySet(tags,"AcDbPlotSettings",333,"0");
        layoutDependencySet(tags,"AcDbLayout",333,"0");
    }else layoutDependencyErase(tags,"AcDbPlotSettings",76);
    layoutDependencySet(tags,"AcDbLayout",76,form==1u?"1":form==2u?"6":"0");
    if(form==0u||form==3u){layoutDependencySet(tags,"AcDbLayout",345,"0");layoutDependencySet(tags,"AcDbLayout",346,"0");}
    if(form==2u){layoutDependencyHeader(tags,{{102,"{ACAD_XDICTIONARY"},{360,"E901"},{102,"}"}});
        auto at=slot(tags,2,"OBJECTS")+1u;while(tags[at].code!=0||tags[at].value!="ENDSEC")++at;
        const Tags objects{{0,"DICTIONARY"},{5,"E901"},{330,"A0"},{100,"AcDbDictionary"},{280,"0"},{281,"1"}};
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),objects.begin(),objects.end());}
    if(form==3u)layoutDependencyHeader(tags,{{102,"{PLOT_HINT"},{74,"3"},{76,"4"},{102,"}"}});
    return tags;
}
bool layoutDependencyTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;using R=dx_iface::DependencyReason;using S=dx_iface::DependencyState;using Role=dx_tableResourceRole;
    const auto path=dir/"layout-dependencies.dxf";unsigned profiles=0u,markers=0u,spans=0u,modes=0u,immutable=0u,refusals=0u,prerequisites=0u,quotas=0u,contexts=0u,throws=0u,reached=0u,unreached=0u,imports=0u,finalizations=0u;
    const auto fail=[&](int line){std::cerr<<"LAYOUT dependency check line="<<line<<" profiles="<<profiles<<" refusals="<<refusals<<" quotas="<<quotas<<" contexts="<<contexts<<" throws="<<throws<<" finalizations="<<finalizations<<'\n';return false;};
    const auto ready=[](const dx_data& d){const auto r=dx_iface::dependencyReport(d);return r.originalTargetsMatched&&r.layoutSources==3u&&r.physicalLayoutRows==3u&&r.backedLayoutRows==3u
        &&r.layoutReferences&&!r.ownerTargetsResolved&&!r.recursiveClosureMatched&&!r.outputEligible;};
    const auto refusal=[](const dx_data& d,int code,R reason){if(!A::dependencies(d))return false;for(const auto& row:A::dependencies(d)->rows)if(row.source==0xA0u&&row.code==code&&row.reason==reason)return true;return false;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true})for(unsigned form=0u;form<4u;++form) {
        dx_iface reader;dx_data data;const auto tags=layoutDependencyDocument(version,blocks,form);
        if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data)) {
            const auto r=dx_iface::dependencyReport(data);std::cerr<<"LAYOUT dependency profile "<<version<<'/'<<encoding<<" blocks="<<blocks<<" form="<<form<<" state="<<static_cast<int>(r.state)
                <<" layouts="<<r.backedLayoutRows<<'/'<<r.physicalLayoutRows<<" first="<<std::hex<<r.firstSource<<" -> "<<r.firstTarget<<std::dec<<'\n';return fail(__LINE__);
        }
        const auto& audit=*A::dependencies(data);const auto& source=*A::identityInput(data)->source;const auto wire=attdefWireTags(path,encoding==2u);
        for(const auto& item:audit.nodes)if(item.second.layout){const auto& record=source.records.at(item.second.physical);const auto& marker=wire.at(record.marker.ordinal-1u);
            if(marker.code||marker.text!="LAYOUT"||marker.begin!=record.marker.beginOffset||marker.end!=record.marker.endOffset)return fail(__LINE__);++markers;
            unsigned owner=0u,block=0u;
            for(const auto& row:audit.rows)if(row.source==item.first){if(row.role==Role::LayoutName||row.code==1)return fail(__LINE__);
                if(row.role==Role::Owner){if(row.target!=0x43u)return fail(__LINE__);++owner;}
                if(row.role==Role::LayoutBlockRecord){if(row.code!=330||row.target!=(item.first==0xA0u?0xB0u:item.first==0xA1u?0xB1u:0xB2u))return fail(__LINE__);++block;}
                if(row.present){const auto& field=wire.at(record.marker.ordinal+row.group);if(field.code!=row.code||A::dependencyGroups(item.second).at(row.group).code()!=row.code)return fail(__LINE__);
                    if((row.code>=320&&row.code<=369)||row.code==1005){std::uint32_t target=0u;const auto parsed=std::from_chars(field.text.data(),field.text.data()+field.text.size(),target,16);
                        if(parsed.ec!=std::errc{}||parsed.ptr!=field.text.data()+field.text.size()||target!=row.target)return fail(__LINE__);}++spans;}
                if(row.modeGroup!=DRW_DxfIdentitySource::None){const auto& mode=wire.at(record.marker.ordinal+row.modeGroup);if(mode.code!=(row.role==Role::PlotViewName?74:76)||mode.integer!=row.mode)return fail(__LINE__);++modes;}
            }
            if(owner!=1u||block!=1u)return fail(__LINE__);
        }
        if(form!=2u&&!dx_iface::dependencyReport(data).inactiveReferences)return fail(__LINE__);
        const auto before=symbolFailedAllocations;{SymbolAllocationFailure none(0);if(!ready(data))return fail(__LINE__);}if(symbolFailedAllocations!=before)return fail(__LINE__);
        for(auto& layout:data.layouts){layout.plotType=3;layout.shadePlotMode=5;layout.orthoViewType=0;layout.name="edited live layout";layout.plotViewName="edited view";}
        A::clearAdvisoryNames(data);if(!ready(data))return fail(__LINE__);A::repeatDependencies(reader,data);if(!ready(data))return fail(__LINE__);++immutable;++profiles;
    }
    for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true}) {
        const auto good=layoutDependencyDocument("AC1032",blocks,0u);
        const auto retry=[&](dx_iface& reader){dx_data next;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&next,false)||!ready(next))return fail(__LINE__);++imports;return true;};
        for(unsigned fault=0u;fault<24u;++fault){auto tags=good;int code=0;R reason=R::OriginalUnavailable;
            if(fault==0u){code=1001;tags[blockField(tags,"A0",code)].value="MISSING_APP";reason=R::MissingName;}
            else if(fault==1u){code=1003;tags[blockField(tags,"A0",code)].value="MISSING_LAYER";reason=R::MissingName;}
            else if(fault==2u){code=1005;tags[blockField(tags,"A0",code)].value="FFFF";reason=R::MissingTarget;}
            else if(fault==3u){code=330;const auto at=blockField(tags,"A0",102)+1u;tags[at].value="FFFF";reason=R::MissingTarget;}
            else if(fault==4u){code=360;layoutDependencyHeader(tags,{{102,"{ACAD_XDICTIONARY"},{360,"40"},{102,"}"}});reason=R::WrongOwner;}
            else if(fault==5u){code=330;layoutDependencyHeader(tags,{{102,"{OPAQUE"},{330,"DEAD"},{102,"}"}});reason=R::UnknownRole;}
            else if(fault<=9u){code=6;layoutDependencySet(tags,"AcDbPlotSettings",74,"3");
                if(fault==6u)layoutDependencyErase(tags,"AcDbPlotSettings",6);else layoutDependencySet(tags,"AcDbPlotSettings",6,fault==7u?"":"LOOKUP");
                reason=fault<8u?R::NullRequired:R::MissingName;
                if(fault==9u){layoutDependencyErase(tags,"AcDbPlotSettings",74);layoutDependencyHeader(tags,{{102,"{PLOT_HINT"},{74,"3"},{102,"}"}});reason=R::OriginalUnavailable;}}
            else if(fault<=14u){code=333;layoutDependencySet(tags,"AcDbPlotSettings",76,fault==11u||fault==13u?"5":"4");
                for(const char* scope:{"AcDbPlotSettings","AcDbLayout"}){if(fault==10u)layoutDependencyErase(tags,scope,333);else layoutDependencySet(tags,scope,333,fault==11u||fault==14u?"0":"40");}
                reason=fault<12u?R::NullRequired:fault==12u?R::WrongKind:R::OriginalUnavailable;
                if(fault==14u)layoutDependencyErase(tags,"AcDbPlotSettings",76);}
            else if(fault<=17u){code=fault==15u?345:346;layoutDependencySet(tags,"AcDbLayout",code,fault==17u?"0":"40");layoutDependencySet(tags,"AcDbLayout",76,"1");reason=R::WrongKind;
                if(fault==17u){layoutDependencyErase(tags,"AcDbLayout",76);reason=R::OriginalUnavailable;}}
            else if(fault<=19u){code=331;layoutDependencySet(tags,"AcDbLayout",331,fault==18u?"701":"FFFF");reason=fault==18u?R::WrongKind:R::MissingTarget;}
            else if(fault==20u){code=345;layoutDependencySet(tags,"AcDbLayout",345,"E701");auto at=slot(tags,2,"TABLES")+1u;while(tags[at].code!=0||tags[at].value!="ENDSEC")++at;
                const Tags ucs{{0,"TABLE"},{2,"UCS"},{5,"E700"},{330,"0"},{100,"AcDbSymbolTable"},{70,"1"},{0,"UCS"},{5,"E701"},{330,"E700"},{100,"AcDbSymbolTableRecord"},{100,"AcDbUCSTableRecord"},{2,"LOOKUP"},{70,"0"},
                    {10,"0"},{20,"0"},{30,"0"},{11,"1"},{21,"0"},{31,"0"},{12,"0"},{22,"1"},{32,"0"},{79,"0"},{146,"0"},{0,"ENDTAB"}};tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),ucs.begin(),ucs.end());}
            else if(fault==21u){code=331;layoutDependencySet(tags,"AcDbLayout",331,"709");auto at=slot(tags,2,"ENTITIES")+1u;
                const Tags viewport{{0,"VIEWPORT"},{5,"709"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbViewport"},{10,"0"},{20,"0"},{30,"0"},{40,"10"},{41,"10"},{68,"1"},{69,"2"}};tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),viewport.begin(),viewport.end());}
            else if(fault==22u)layoutDependencySet(tags,"AcDbLayout",333,"F00");
            else {code=333;for(const char* scope:{"AcDbPlotSettings","AcDbLayout"})layoutDependencySet(tags,scope,333,"FFFF");}
            struct CallbackOnlyUcs:dx_iface {bool selected=true;bool wantsDxfViewUcsSource() const override{return selected&&dx_iface::wantsDxfViewUcsSource();}} reader;
            reader.selected=fault!=20u;dx_data data;if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||ready(data))return fail(__LINE__);
            if(fault==22u){if(dx_iface::dependencyReport(data).state!=S::SourceNotReady)return fail(__LINE__);++prerequisites;}
            else if(!refusal(data,code,reason)){const auto r=dx_iface::dependencyReport(data);std::cerr<<"LAYOUT refusal fault="<<fault<<" state="<<static_cast<int>(r.state)<<" code="<<code<<" expected="<<static_cast<int>(reason)<<'\n';return fail(__LINE__);}
            if(fault==23u&&A::layoutInputs(data).at(0xA0u).input->model->shadePlotHandle.ref!=0xFFFFu)return fail(__LINE__);
            reader.selected=true;if(!retry(reader))return fail(__LINE__);++refusals;
        }
        dx_iface reader;dx_data base;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&base,false)||!ready(base))return fail(__LINE__);
        const auto report=dx_iface::dependencyReport(base);const std::size_t limits[]{report.reservedRows,report.bytes,report.work};
        for(unsigned which=0u;which<3u;++which)for(bool under:{false,true}){dx_iface codec;dx_data actual;A::dependencyLimit(actual,which,limits[which]-(under?1u:0u));
            if(!codec.fileImport(path.string(),&actual,false)||ready(actual)==under||(under&&dx_iface::dependencyReport(actual).state!=S::CaptureFailed)||!retry(codec))return fail(__LINE__);++quotas;}
        for(unsigned fault=0u;fault<6u;++fault){dx_data actual;if(!reader.fileImport(path.string(),&actual,false)||!ready(actual))return fail(__LINE__);
            if(fault<2u){auto& p=A::mutableLayoutAudits(actual).at(0xA0u);if(fault)p=std::decay_t<decltype(p)>(p.get(),[](const auto*){});else p=std::make_shared<const std::decay_t<decltype(*p)>>(*p);}
            else if(fault<4u)A::substituteLayoutDependencyInput(actual,fault==3u);
            else if(fault==4u){dx_iface other;dx_data d;if(!other.fileImport(path.string(),&d,false)||!ready(d))return fail(__LINE__);A::mutableLayoutAudits(actual).at(0xA0u)=A::layoutAudits(d).at(0xA0u);}
            else A::substituteLayoutRole(actual,false);
            if(dx_iface::dependencyReport(actual).state!=S::ContextMismatch||!retry(reader))return fail(__LINE__);++contexts;}
        struct Throwing:dx_iface {unsigned phase=0u;
            void addDxfLayoutSource(const DRW_Layout& v,const DRW_RawDxfObject& r) override{dx_iface::addDxfLayoutSource(v,r);if(phase==1u&&v.handle==0xA0u)throw std::runtime_error("layout dependency source");}
            void addLayout(const DRW_Layout& v) override{dx_iface::addLayout(v);if(phase==2u&&v.handle==0xA0u)throw std::runtime_error("layout dependency typed");}
        } throwing;
        for(unsigned phase=1u;phase<=2u;++phase){throwing.phase=phase;dx_data failed;bool caught=false;try{if(throwing.fileImport(path.string(),&failed,false))return fail(__LINE__);}catch(const std::runtime_error&){caught=true;}
            if(!caught||dx_iface::dependencyReport(failed).state!=S::ImportFailed)return fail(__LINE__);throwing.phase=0u;if(!retry(throwing))return fail(__LINE__);++throws;}
        for(long offset=0;offset<64;++offset){const auto before=symbolFailedAllocations;{SymbolAllocationFailure fault(offset);A::repeatDependencies(reader,base);}
            if(symbolFailedAllocations!=before){if(ready(base)||dx_iface::dependencyReport(base).state!=S::CaptureFailed)return fail(__LINE__);++reached;}else {if(!ready(base))return fail(__LINE__);++unreached;}
            A::repeatDependencies(reader,base);if(!ready(base))return fail(__LINE__);++finalizations;}
    }
    std::cout<<"Original LAYOUT dependency profiles="<<profiles<<"; original markers="<<markers<<"; reference spans="<<spans<<"; mode spans="<<modes<<"; live-state independence="<<immutable
        <<"; field/mode/target refusals="<<refusals<<"; prerequisite refusals="<<prerequisites<<"; exact/one-under limits="<<quotas<<"; context refusals="<<contexts<<"; callback throws="<<throws
        <<"; reached finalization failures="<<reached<<"; unreached offsets="<<unreached<<"; corrected imports="<<imports<<"; corrected finalizations="<<finalizations<<'\n';
    return profiles==96u&&markers==288u&&spans==1968u&&modes==312u&&immutable==96u&&refusals==144u&&prerequisites==6u&&quotas==36u&&contexts==36u&&throws==12u&&reached>0u&&reached+unreached==384u&&imports==228u&&finalizations==384u;
}
