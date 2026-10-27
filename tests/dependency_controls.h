// SPDX-License-Identifier: GPL-2.0-or-later
// Runtime-authored common/name/reference graphs; no external drawing payloads.
void dependencyCommon(Tags& tags,const char* handle,const Tags& extra) {
    auto at=slot(tags,5,handle)+1u;while(tags[at].code!=100)++at;++at;while(tags[at].code!=100&&tags[at].code!=0)++at;
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),extra.begin(),extra.end());
}
Tags dependencyDocument(const std::string& version,bool blocks,unsigned form) {
    auto tags=symbolOriginDocument(version,blocks,0u,48);
    // Earlier source-profile controls deliberately contain opaque handle
    // lookalikes. A complete reference graph must refuse those; the positive
    // recipe removes that group and tests an opaque group separately below.
    auto opaque=slot(tags,102,"{OPAQUE"),opaqueEnd=opaque+1u;while(tags[opaqueEnd].code!=102||tags[opaqueEnd].value!="}")++opaqueEnd;
    tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(opaque),tags.begin()+static_cast<std::ptrdiff_t>(opaqueEnd+1u));
    tags[blockField(tags,"D301",2)].value="0";
    for(int code:{347,390})tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,"D301",code)));
    const auto geometry=tags[blockField(tags,"501",343)].value;
    tags[blockField(tags,"501",2)].value=tags[blockField(tags,geometry.c_str(),2)].value;
    appendBlockFields(tags,"501",{{1001,"REGISTERED_APP"},{1003,"0"},{1005,"705"}});
    appendBlockFields(tags,"D301",{{1001,"REGISTERED_APP"},{1005,"701"}});
    appendBlockFields(tags,"D401",{{1001,"REGISTERED_APP"},{1003,"0"}});
    auto text=slot(tags,5,"511")+1u;while(tags[text].code!=100||tags[text].value!="AcDbAttribute")++text;
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(text),{{7,"STANDARD"}});
    if(form==1u) {
        tags[blockField(tags,"D301",2)].value="Layer-é";tags[blockField(tags,"D401",2)].value="App-é";
        tags[blockField(tags,"D201",2)].value="Style-é";tags[blockField(tags,"D101",2)].value="Line-é";
        tags[blockField(tags,"D301",6)].value="lInE-\\U+00E9";
        for(auto& v:tags) {
            if(v.code==8&&v.value=="0")v.value="lAyEr-\\U+00E9";
            if(v.code==1001&&v.value=="REGISTERED_APP")v.value="aPp-\\U+00E9";
            if(v.code==1003&&v.value=="0")v.value="lAyEr-\\U+00E9";
            if(v.code==7&&v.value=="STANDARD")v.value="sTyLe-\\U+00E9";
        }
    }
    if(form==2u) {
        tags[blockField(tags,"705",90)].value="1";const auto at=blockField(tags,"705",90)+1u;
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{360,"708"}});
        auto end=slot(tags,2,"OBJECTS")+1u;while(tags[end].code!=0||tags[end].value!="ENDSEC")++end;
        const Tags field{{0,"FIELD"},{5,"708"},{330,"705"},{100,"AcDbField"},{1,"AcExpr"},{2,"2+2"},{90,"0"},{93,"0"},{97,"0"}};
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),field.begin(),field.end());
    }
    if(form==3u) {
        dependencyCommon(tags,"501",{{6,"BYBLOCK"}});
        tags[blockField(tags,"705",97)].value="1";appendBlockFields(tags,"705",{{331,"706"}});
        appendBlockFields(tags,"706",{{320,"FFFF"},{330,"705"},{330,"0"}});
    }
    if(form==4u||form==5u) {
        appendBlockFields(tags,"D201",{{1071,"5570578"}});
        tags[blockField(tags,"D101",73)].value="2";tags[blockField(tags,"D101",40)].value="1.75";
        appendBlockFields(tags,"D101",{{49,"1.25"},{74,"2"},{75,"0"},{340,"D201"},{46,"0.75"},{50,"30"},{44,"0.125"},{45,"-0.25"},{9,"dash-é"},
            {49,"-0.5"},{74,"4"},{75,"7"},{340,"D202"},{46,"2"},{50,"45"},{44,"0"},{45,"0"}});
        auto at=slot(tags,5,"D201");while(tags[at].code!=0)++at;
        Tags shape{{0,"STYLE"},{5,"D202"},{330,"D1"},{100,"AcDbSymbolTableRecord"},{100,"AcDbTextStyleTableRecord"}};
        if(form==4u)shape.push_back({2,""});
        shape.insert(shape.end(),{{70,"1"},{40,"0"},{41,"1"},{50,"0"},{71,"0"},{42,"1"},{3,"authored-shape.shx"},{4,""}});
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),shape.begin(),shape.end());tags[blockField(tags,"D1",70)].value="2";
    }
    return tags;
}
bool dependencyTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::DependencyState;using R=dx_iface::DependencyReason;
    const auto path=dir/"original-dependencies.dxf";unsigned profiles=0u,markers=0u,spans=0u,resourceOnly=0u,refusals=0u,quotas=0u,contexts=0u,immutable=0u,reached=0u,unreached=0u,retries=0u;
    const auto ready=[](const dx_data& d){const auto r=dx_iface::dependencyReport(d);return r.state==S::OriginalTargetsMatched&&r.originalTargetsMatched
        &&r.nameNamespacesComplete&&r.physicalNamedRows==r.backedNamedRows&&r.references==r.matched&&!r.recursiveClosureMatched&&!r.ownerTargetsResolved&&!r.outputEligible;};
    const auto diagnose=[](const dx_data& d){const auto r=dx_iface::dependencyReport(d);std::cerr<<"Dependency state="<<static_cast<int>(r.state)<<" sources="<<r.sources<<" refs="<<r.references<<" matched="<<r.matched
        <<" named="<<r.backedNamedRows<<'/'<<r.physicalNamedRows<<" unavailable sources="<<r.unavailableSources<<" first="<<std::hex<<r.firstSource<<" -> "<<r.firstTarget<<std::dec<<'\n';
        if(A::dependencies(d))for(const auto& row:A::dependencies(d)->rows)if(row.reason!=R::Matched&&row.reason!=R::NullOptional&&row.reason!=R::Literal&&row.reason!=R::Inherited&&row.reason!=R::OriginalTableOwner)
            std::cerr<<"  from="<<std::hex<<row.source<<" to="<<row.target<<std::dec<<" code="<<row.code<<" role="<<static_cast<int>(row.role)<<" reason="<<static_cast<int>(row.reason)<<" key="<<row.key<<'\n';};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true})for(unsigned form=0u;form<6u;++form) {
        const auto tags=dependencyDocument(version,blocks,form);dx_iface reader;dx_data data;
        if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data)){std::cerr<<version<<'/'<<encoding<<" blocks="<<blocks<<" form="<<form<<'\n';diagnose(data);return false;}
        const auto before=symbolFailedAllocations;{SymbolAllocationFailure none(0);if(!ready(data))return false;}if(symbolFailedAllocations!=before)return false;
        const auto wire=attdefWireTags(path,encoding==2u);const auto& physical=*A::identityInput(data)->source;const auto& audit=*A::dependencies(data);
        for(const auto& node:audit.nodes) {
            const auto& p=physical.records.at(node.second.physical);const auto& marker=wire.at(p.marker.ordinal-1u);
            if(marker.code||marker.text!=p.name||marker.begin!=p.marker.beginOffset||marker.end!=p.marker.endOffset)return false;++markers;
        }
        for(const auto& row:audit.rows)if(row.present) {
            const auto& n=audit.nodes.at(row.source);const auto& original=physical.records.at(n.physical);const auto& scalar=wire.at(original.marker.ordinal+row.group);
            if(scalar.code!=row.code||A::dependencyGroups(n).at(row.group).code()!=row.code)return false;++spans;
        }
        A::clearAdvisoryNames(data);if(!ready(data))return false;A::repeatDependencies(reader,data);if(!ready(data))return false;
        for(auto& layer:data.layers)layer.name="edited consumer layer";for(auto& app:data.appIds)app.name="edited consumer app";
        if(!ready(data))return false;++immutable;++profiles;
    }
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding) {
        auto symbols=symbolOriginTables(48);symbols[blockField(symbols,"D301",2)].value="0";
        for(int code:{347,390})symbols.erase(symbols.begin()+static_cast<std::ptrdiff_t>(blockField(symbols,"D301",code)));
        Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"},{0,"SECTION"},{2,"TABLES"}};
        tags.insert(tags.end(),symbols.begin(),symbols.end());tags.insert(tags.end(),{{0,"ENDSEC"},{0,"SECTION"},{2,"OBJECTS"},{0,"DICTIONARY"},{5,"40"},{330,"0"},{100,"AcDbDictionary"},{280,"0"},{281,"1"}});
        const auto rich=resourceOriginObjects();tags.insert(tags.end(),rich.begin(),rich.end());tags.insert(tags.end(),{{0,"ENDSEC"},{0,"EOF"}});
        dx_iface reader;dx_data data;if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data)
            ||dx_iface::tableReferenceReport(data).state!=dx_iface::TableReferenceState::NoTables||!dx_iface::tableReferenceReport(data).targets){diagnose(data);return false;}++resourceOnly;
    }
    for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true}) {
        const auto good=dependencyDocument("AC1032",blocks,0u);
        const auto retry=[&](dx_iface& reader){dx_data next;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&next,false)||!ready(next)){diagnose(next);return false;}++retries;return true;};
        for(unsigned fault=0u;fault<28u;++fault) {
            auto tags=good;
            if(fault==0u) {auto at=slot(tags,5,"D301")-1u;auto end=at+1u;while(tags[end].code!=0)++end;tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at),tags.begin()+static_cast<std::ptrdiff_t>(end));}
            else if(fault==1u)tags[blockField(tags,"501",8)].value="MISSING_LAYER";
            else if(fault==2u)tags[blockField(tags,"501",1001)].value="MISSING_APP";
            else if(fault==3u)tags[blockField(tags,"511",7)].value="MISSING_STYLE";
            else if(fault==4u)tags[blockField(tags,"D301",6)].value="MISSING_LINE";
            else if(fault==5u)tags[blockField(tags,"501",2)].value="MISSING_BLOCK";
            else if(fault==6u) {
                auto first=slot(tags,5,"D301")-1u,end=first+1u;while(tags[end].code!=0)++end;
                Tags duplicate(tags.begin()+static_cast<std::ptrdiff_t>(first),tags.begin()+static_cast<std::ptrdiff_t>(end));duplicate[1].value="D302";
                tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),duplicate.begin(),duplicate.end());tags[blockField(tags,"D2",70)].value="2";
            }else if(fault==7u)tags[blockField(tags,"501",8)].value="\\U+D800";
            else if(fault==8u)tags[blockField(tags,"D301",330)].value="40";
            else if(fault==9u)tags[blockField(tags,"501",1005)].value="FFFF";
            else if(fault==10u){const auto at=blockField(tags,"D301",1001);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{347,"40"}});}
            else if(fault==11u) {
                tags[blockField(tags,"501",1005)].value="708";auto at=slot(tags,2,"OBJECTS")+1u;while(tags[at].code!=0||tags[at].value!="ENDSEC")++at;
                tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{0,"UNKNOWN_OBJECT"},{5,"708"},{330,"40"},{100,"UnknownPayload"},{1,"opaque"}});
            }else if(fault==12u) {
                const auto at=blockField(tags,"D401",100);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{102,"{OPAQUE"},{330,"0"},{102,"}"}});
            }else if(fault==13u){tags[blockField(tags,"706",330)].value="706";appendBlockFields(tags,"706",{{360,"706"}});}
            else if(fault==14u)appendBlockFields(tags,"706",{{360,"705"}});
            else if(fault==15u){tags[blockField(tags,"705",90)].value="1";const auto at=blockField(tags,"705",90)+1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{360,"706"}});}
            else if(fault==16u)appendBlockFields(tags,"D201",{{99,"7"}});
            else if(fault==17u){const auto at=blockField(tags,"D201",2);tags[at].value="\\U+D800";}
            else if(fault<20u){tags=dependencyDocument("AC1032",blocks,4u);auto at=blockField(tags,"D101",340);if(fault==18u)tags[at].value="D202";else{++at;while(tags[at].code!=340)++at;tags[at].value="D201";}}
            else if(fault==20u) {
                tags=dependencyDocument("AC1032",blocks,1u);auto first=slot(tags,5,"D301")-1u,end=first+1u;while(tags[end].code!=0)++end;
                Tags duplicate(tags.begin()+static_cast<std::ptrdiff_t>(first),tags.begin()+static_cast<std::ptrdiff_t>(end));duplicate[1].value="D302";duplicate[slot(duplicate,2)].value="LAYER-É";
                tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),duplicate.begin(),duplicate.end());tags[blockField(tags,"D2",70)].value="2";
            }else if(fault==21u)tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,"D301",6)));
            else if(fault==22u)appendBlockFields(tags,"D201",{{1001,"REGISTERED_APP"},{1002,"{"}});
            else if(fault==23u)appendBlockFields(tags,"D101",{{99,"7"}});
            else if(fault==24u)appendBlockFields(tags,"D201",{{1071,"1"},{1071,"2"}});
            else if(fault==25u){const auto at=blockField(tags,"D201",100);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{99,"7"}});}
            else if(fault==26u){tags=dependencyDocument("AC1032",blocks,1u);tags[blockField(tags,"501",8)].value="LAYER-É";}
            else {
                auto at=blockField(tags,"D301",1001);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{390,"D900"}});
                at=slot(tags,2,"OBJECTS")+1u;while(tags[at].code!=0||tags[at].value!="ENDSEC")++at;
                tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{0,"ACDBPLACEHOLDER"},{5,"D900"},{330,"40"}});
            }
            dx_iface reader;dx_data data;if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||ready(data)) {std::cerr<<"Dependency refusal fault="<<fault<<'\n';diagnose(data);return false;}
            if(fault==13u&&!dx_iface::dependencyReport(data).ownershipCycles){std::cerr<<"Hard cycle not inventoried\n";diagnose(data);return false;}
            const auto report=dx_iface::dependencyReport(data);
            if(fault==27u) {
                bool unavailable=false;if(A::dependencies(data))for(const auto& row:A::dependencies(data)->rows)
                    if(row.source==0xD301u&&row.code==390&&row.target==0xD900u&&row.reason==R::OriginalUnavailable)unavailable=true;
                if(!unavailable){std::cerr<<"Plot-style placeholder incorrectly classified\n";diagnose(data);return false;}
            }
            if((fault<6u&&!report.missing)||((fault==6u||fault==20u)&&!report.ambiguous)
                ||((fault==10u||fault==15u||fault==18u||fault==19u)&&!report.wrongKind)
                ||(fault==11u&&!report.unavailable)||(fault==12u&&!report.unknownRoles)||(fault==14u&&!report.wrongOwner)) {std::cerr<<"Unexercised refusal category="<<fault<<'\n';diagnose(data);return false;}
            if(!retry(reader))return false;++refusals;
        }
        struct ReplayOnly:dx_iface {bool selected=false;bool wantsDxfResourceSource() const override{return selected&&dx_iface::wantsDxfResourceSource();}} replay;
        dx_data replayData;if(!writeBlockBinding(path,good,encoding)||!replay.fileImport(path.string(),&replayData,false)||ready(replayData)
            ||!A::resources(replayData).at(0xD301u).rawTypedMatched)return false;
        replay.selected=true;if(!retry(replay))return false;++refusals;
        dx_iface baseline;dx_data base;if(!baseline.fileImport(path.string(),&base,false)||!ready(base))return false;
        const auto report=dx_iface::dependencyReport(base);const std::size_t uses[]={report.reservedRows,report.bytes,report.work};
        for(unsigned which=0u;which<3u;++which)for(bool under:{false,true}) {
            dx_iface reader;dx_data data;A::dependencyLimit(data,which,uses[which]-(under?1u:0u));
            if(!reader.fileImport(path.string(),&data,false)||(dx_iface::dependencyReport(data).state==S::OriginalTargetsMatched)==under||!retry(reader))return false;++quotas;
        }
        for(unsigned fault=0u;fault<7u;++fault) {
            dx_iface reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!ready(data))return false;
            if(fault<2u)A::substituteDependencies(data,fault!=0u);
            else if(fault==2u){dx_iface other;dx_data d;if(!other.fileImport(path.string(),&d,false)||!ready(d))return false;A::transplantDependencies(data,d);}
            else if(fault==3u)A::substituteResourceOrigin(data,0u,false);
            else if(fault==4u)A::substituteTableReferences(data,false);
            else if(fault==5u)A::mutableResource(data,0xD401u).references.front().value^=1u;
            else A::removeTableBody(data);
            if(dx_iface::dependencyReport(data).state!=S::ContextMismatch||!retry(reader))return false;++contexts;
        }
        for(long offset=0;offset<64;++offset) {
            dx_iface reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!ready(data))return false;const auto before=symbolFailedAllocations;
            {SymbolAllocationFailure fail(offset);A::repeatDependencies(reader,data);}
            if(symbolFailedAllocations!=before){if(dx_iface::dependencyReport(data).state!=S::CaptureFailed)return false;++reached;}
            else {if(!ready(data))return false;++unreached;}
            if(!retry(reader))return false;
        }
    }
    std::cout<<"Original dependency profiles="<<profiles<<"; original markers="<<markers<<"; physical reference spans="<<spans<<"; resource-only profiles="<<resourceOnly
        <<"; field/name/owner/producer refusals="<<refusals<<"; exact/one-under limits="<<quotas<<"; immutable/cross-import refusals="<<contexts<<"; advisory/live independence="<<immutable
        <<"; reached finalization failures="<<reached<<"; unreached offsets="<<unreached<<"; corrected imports="<<retries<<'\n';
    return profiles==144u&&markers>0u&&spans>0u&&resourceOnly==12u&&refusals==174u&&quotas==36u&&contexts==42u&&immutable==144u&&reached>0u&&reached+unreached==384u&&retries==636u;
}
