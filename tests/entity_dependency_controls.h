// SPDX-License-Identifier: GPL-2.0-or-later
// Locally authored original entity dependency graphs, never drawing fixtures.
constexpr const char* entityDependencyHandles[]={"201","202","203","204","206","207","208","301","302","303","703","F4","F5","F6","F7","701","702"};
Tags entityDependencyDocument(const std::string& version,bool blocks,unsigned form) {
    auto tags=tableGeometryDependencyDocument(version,blocks,form==1u?2u:0u);
    auto value=blockField(tags,"501",301)+1u;while(tags[value].code!=330)++value;tags[value].value="701";
    const Tags definition{{0,"ATTDEF"},{5,"703"},{330,blocks?"B3":"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbText"},
        {10,"1"},{20,"2"},{40,"1.25"},{1,"default"},{7,"STANDARD"},{100,"AcDbAttributeDefinition"},{3,"prompt"},{2,"TAG"},{70,"0"}};
    auto at=blocks?slot(tags,5,"F7")-1u:slot(tags,2,"ENTITIES")+1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),definition.begin(),definition.end());
    at=slot(tags,5,"207");while(tags[at].code!=100||tags[at].value!="AcDbAttribute")++at;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{7,"STANDARD"}});
    for(const char* handle:entityDependencyHandles) {
        if(form!=2u)dependencyCommon(tags,handle,{{6,form==1u?"BYBLOCK":"CONTINUOUS"}});
        appendBlockFields(tags,handle,{{1001,"REGISTERED_APP"},{1003,"0"},{1005,"705"}});
    }
    dependencyCommon(tags,"701",{{102,"{ACAD_REACTORS"},{330,"705"},{102,"}"}});
    if(form==1u){tags[blockField(tags,"D301",2)].value="Layer-é";for(auto& t:tags)if((t.code==8||t.code==1003)&&t.value=="0")t.value="lAyEr-\\U+00E9";}
    if(form==2u){dependencyCommon(tags,"701",{{102,"{ACAD_XDICTIONARY"},{360,"E901"},{102,"}"}});
        const Tags objects{{0,"DICTIONARY"},{5,"E901"},{330,"701"},{100,"AcDbDictionary"},{280,"1"},{281,"1"},{3,"OwnedNote"},{350,"E902"},
            {0,"XRECORD"},{5,"E902"},{330,"E901"},{100,"AcDbXrecord"},{280,"1"},{1,"authored-note"}};
        at=slot(tags,2,"OBJECTS")+1u;while(tags[at].code!=0||tags[at].value!="ENDSEC")++at;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),objects.begin(),objects.end());}
    return tags;
}
bool entityDependencyTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;using R=dx_iface::DependencyReason;using S=dx_iface::DependencyState;
    const auto path=dir/"entity-dependencies.dxf";unsigned profiles=0u,markers=0u,spans=0u,immutable=0u,refusals=0u,quotas=0u,contexts=0u,throws=0u,reached=0u,unreached=0u,imports=0u,finalizations=0u;
    std::array<unsigned,11> kinds{};const std::array<std::string_view,11> names{{"POINT","LINE","POLYLINE","INSERT","ATTRIB","VERTEX","ATTDEF","BLOCK","ENDBLK","SEQEND","ACAD_TABLE"}};
    const auto fail=[&](int line){std::cerr<<"Entity dependency check line="<<line<<" profiles="<<profiles<<" refusals="<<refusals<<" quotas="<<quotas
        <<" contexts="<<contexts<<" throws="<<throws<<" finalizations="<<finalizations<<'\n';return false;};
    const auto ready=[](const dx_data& d){const auto r=dx_iface::dependencyReport(d);return r.originalTargetsMatched&&r.physicalEntityRows&&r.physicalEntityRows==r.backedEntityRows
        &&r.entitySources==28u&&r.entitySources==r.backedEntityRows&&r.entityReferences&&!r.ownerTargetsResolved&&!r.recursiveClosureMatched&&!r.outputEligible;};
    const auto refusal=[&](const dx_data& d,std::uint32_t handle,int code,R reason){if(!A::dependencies(d))return false;
        for(const auto& row:A::dependencies(d)->rows)if(row.source==handle&&row.code==code&&row.reason==reason)return true;return false;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true})for(unsigned form=0u;form<3u;++form) {
        const auto tags=entityDependencyDocument(version,blocks,form);dx_iface reader;dx_data data;
        if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data)) {
            const auto r=dx_iface::dependencyReport(data);std::cerr<<"Entity dependency profile "<<version<<'/'<<encoding<<" blocks="<<blocks<<" form="<<form<<" state="<<static_cast<int>(r.state)
                <<" entities="<<r.backedEntityRows<<'/'<<r.physicalEntityRows<<" unavailable="<<r.unavailableSources<<" first="<<std::hex<<r.firstSource<<" -> "<<r.firstTarget<<std::dec<<'\n';
            if(A::dependencies(data))for(const auto& row:A::dependencies(data)->rows)if(row.reason!=R::Matched&&row.reason!=R::NullOptional&&row.reason!=R::Inherited&&row.reason!=R::Literal&&row.reason!=R::OriginalTableOwner)
                std::cerr<<" source="<<std::hex<<row.source<<" target="<<row.target<<std::dec<<" code="<<row.code<<" reason="<<static_cast<int>(row.reason)<<'\n';
            return fail(__LINE__);
        }
        const auto& audit=*A::dependencies(data);const auto& physical=*A::identityInput(data)->source;const auto wire=attdefWireTags(path,encoding==2u);
        for(const auto& item:audit.nodes)if(item.second.entity) {
            const auto& node=item.second;const auto& record=physical.records.at(node.physical);const auto& marker=wire.at(record.marker.ordinal-1u);
            if(marker.code||marker.text!=record.name||marker.begin!=record.marker.beginOffset||marker.end!=record.marker.endOffset)return fail(__LINE__);
            const auto kind=std::find(names.begin(),names.end(),record.name);if(kind==names.end())return fail(__LINE__);++kinds[static_cast<std::size_t>(kind-names.begin())];++markers;
            bool owner=false;for(const auto& row:audit.rows)if(row.source==node.handle&&row.code==330&&row.role==dx_tableResourceRole::Owner){if(row.target!=node.originalOwner)return fail(__LINE__);owner=true;}
            if(!owner)return fail(__LINE__);
        }
        for(const auto& row:audit.rows)if(row.present&&audit.nodes.at(row.source).entity) {
            const auto& node=audit.nodes.at(row.source);const auto& record=physical.records.at(node.physical);const auto& field=wire.at(record.marker.ordinal+row.group);
            if(field.code!=row.code||A::dependencyGroups(node).at(row.group).code()!=row.code)return fail(__LINE__);++spans;
        }
        const auto before=symbolFailedAllocations;{SymbolAllocationFailure none(0);if(!ready(data))return fail(__LINE__);}if(symbolFailedAllocations!=before)return fail(__LINE__);
        for(auto* entity:data.mBlock->ent)entity->layer="edited live layer";
        for(auto& layer:data.layers)layer.name="edited live symbol";A::clearAdvisoryNames(data);if(!ready(data))return fail(__LINE__);
        A::repeatDependencies(reader,data);if(!ready(data))return fail(__LINE__);++immutable;++profiles;
    }
    for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true}) {
        const auto good=entityDependencyDocument("AC1032",blocks,0u);
        const auto retry=[&](dx_iface& reader){dx_data next;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&next,false)||!ready(next))return fail(__LINE__);++imports;return true;};
        for(const char* handle:entityDependencyHandles) {
            auto tags=good;tags[blockField(tags,handle,8)].value="MISSING_ENTITY_LAYER";dx_iface reader;dx_data data;
            if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||ready(data)
                ||!refusal(data,static_cast<std::uint32_t>(std::stoul(handle,nullptr,16)),8,R::MissingName)||!retry(reader))return fail(__LINE__);
            ++refusals;
        }
        for(unsigned fault=0u;fault<12u;++fault) {
            auto tags=good;int code=0;R reason=R::OriginalUnavailable;std::uint32_t handle=0x701u;
            if(fault==0u){tags[blockField(tags,"701",6)].value="MISSING_LINE";code=6;reason=R::MissingName;}
            else if(fault==1u){tags[blockField(tags,"207",7)].value="MISSING_STYLE";code=7;handle=0x207u;reason=R::MissingName;}
            else if(fault==2u){tags[blockField(tags,"701",1001)].value="MISSING_APP";code=1001;reason=R::MissingName;}
            else if(fault==3u){tags[blockField(tags,"701",1003)].value="MISSING_LAYER";code=1003;reason=R::MissingName;}
            else if(fault==4u){tags[blockField(tags,"701",1005)].value="FFFF";code=1005;reason=R::MissingTarget;}
            else if(fault==5u||fault==7u){code=fault==5u?347:348;dependencyCommon(tags,"701",{{code,"40"}});reason=R::WrongKind;}
            else if(fault==6u){code=390;dependencyCommon(tags,"701",{{390,"E999"}});auto at=slot(tags,2,"OBJECTS")+1u;while(tags[at].code!=0||tags[at].value!="ENDSEC")++at;
                tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{0,"ACDBPLACEHOLDER"},{5,"E999"},{330,"40"}});}
            else if(fault==8u){code=360;dependencyCommon(tags,"701",{{102,"{ACAD_XDICTIONARY"},{360,"40"},{102,"}"}});reason=R::WrongOwner;}
            else if(fault==9u){code=330;auto at=blockField(tags,"701",102)+1u;tags[at].value="FFFF";reason=R::MissingTarget;}
            else if(fault==10u){code=330;dependencyCommon(tags,"701",{{102,"{OPAQUE"},{330,"DEAD"},{102,"}"}});reason=R::UnknownRole;}
            else {auto at=slot(tags,2,"ENTITIES")+1u;while(tags[at].code!=0||tags[at].value!="ENDSEC")++at;
                tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{0,"VENDOR_ENTITY"},{5,"709"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"VendorBody"},{1,"opaque"}});}
            dx_iface reader;dx_data data;if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||ready(data))return fail(__LINE__);
            const auto r=dx_iface::dependencyReport(data);
            if(fault==11u){if(!r.unavailableSources||r.backedEntityRows+1u!=r.physicalEntityRows)return fail(__LINE__);}
            else if(!refusal(data,handle,code,reason))return fail(__LINE__);
            if(!retry(reader))return fail(__LINE__);++refusals;
        }
        dx_iface reader;dx_data base;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&base,false)||!ready(base))return fail(__LINE__);
        const auto report=dx_iface::dependencyReport(base);const std::size_t limits[]{report.reservedRows,report.bytes,report.work};
        for(unsigned which=0u;which<3u;++which)for(bool under:{false,true}){dx_iface codec;dx_data actual;A::dependencyLimit(actual,which,limits[which]-(under?1u:0u));
            if(!codec.fileImport(path.string(),&actual,false)||ready(actual)==under||(under&&dx_iface::dependencyReport(actual).state!=S::CaptureFailed)||!retry(codec))return fail(__LINE__);++quotas;}
        for(unsigned fault=0u;fault<6u;++fault){dx_data actual;if(!reader.fileImport(path.string(),&actual,false)||!ready(actual))return fail(__LINE__);
            if(fault<2u)A::substituteEntitiesRole(actual,fault!=0u);else if(fault<4u)A::substituteBlockRole(actual,0u,fault==3u);
            else if(fault==4u)A::substituteEntitiesRoleBody(actual,false);else A::substituteBlockRole(actual,3u,false);
            if(dx_iface::dependencyReport(actual).state!=S::ContextMismatch||!retry(reader))return fail(__LINE__);++contexts;}
        struct Throwing:dx_iface {unsigned phase=0u;
            void addPoint(const DRW_Point& v) override{dx_iface::addPoint(v);if(phase==1u&&v.handle==0x701u)throw std::runtime_error("entity dependency point");}
            void addDxfAttdefSource(const DRW_Attdef& v,const DRW_RawDxfObject& raw,const DRW_DxfAttdefSourceContext& c) override{dx_iface::addDxfAttdefSource(v,raw,c);if(phase==2u&&v.handle==0x703u)throw std::runtime_error("entity dependency attdef");}
        } throwing;
        for(unsigned phase=1u;phase<=2u;++phase){throwing.phase=phase;dx_data failed;bool caught=false;try{if(throwing.fileImport(path.string(),&failed,false))return fail(__LINE__);}catch(const std::runtime_error&){caught=true;}
            if(!caught||dx_iface::dependencyReport(failed).state!=S::ImportFailed)return fail(__LINE__);throwing.phase=0u;if(!retry(throwing))return fail(__LINE__);++throws;}
        for(long offset=0;offset<64;++offset){const auto before=symbolFailedAllocations;{SymbolAllocationFailure fault(offset);A::repeatDependencies(reader,base);}
            if(symbolFailedAllocations!=before){if(ready(base)||dx_iface::dependencyReport(base).state!=S::CaptureFailed)return fail(__LINE__);++reached;}
            else {if(!ready(base))return fail(__LINE__);++unreached;}A::repeatDependencies(reader,base);if(!ready(base))return fail(__LINE__);++finalizations;}
    }
    std::cout<<"Original entity dependency profiles="<<profiles<<"; original entity markers="<<markers<<"; physical reference spans="<<spans<<"; live-state independence="<<immutable
        <<"; field/target/unsupported refusals="<<refusals<<"; exact/one-under limits="<<quotas<<"; context refusals="<<contexts<<"; callback throws="<<throws
        <<"; reached finalization failures="<<reached<<"; unreached offsets="<<unreached<<"; corrected imports="<<imports<<"; corrected finalizations="<<finalizations<<'\n';
    return profiles==72u&&markers==2016u&&spans==10416u&&immutable==72u&&refusals==174u&&quotas==36u&&contexts==36u&&throws==12u&&reached>0u&&reached+unreached==384u&&imports==258u&&finalizations==384u
        &&std::all_of(kinds.begin(),kinds.end(),[](unsigned count){return count>0u;});
}
