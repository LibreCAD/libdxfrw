// SPDX-License-Identifier: GPL-2.0-or-later
// Runtime-authored TABLE geometry name/handle graphs; no external drawings.
Tags tableGeometryDependencyDocument(const std::string& version,bool blocks,unsigned spelling=0u) {
    auto tags=dependencyDocument(version,blocks,0u);
    const std::string name=spelling==2u?"*T-é":"*T17";
    auto at=slot(tags,5,"8");while(tags[at].code!=0||tags[at].value!="ENDTAB")++at;
    const Tags record{{0,"BLOCK_RECORD"},{5,"B8"},{330,"8"},{100,"AcDbSymbolTableRecord"},{100,"AcDbBlockTableRecord"},{2,name},{70,"1"},{340,"0"}};
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),record.begin(),record.end());
    tags[blockField(tags,"8",70)].value=std::to_string(std::stoi(tags[blockField(tags,"8",70)].value)+1);
    at=slot(tags,2,"BLOCKS")+1u;while(tags[at].code!=0||tags[at].value!="ENDSEC")++at;
    const Tags definition{{0,"BLOCK"},{5,"F8"},{330,"B8"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},{2,name},{70,"1"},{10,"0"},{20,"0"},
        {0,"ENDBLK"},{5,"F9"},{330,"B8"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"}};
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),definition.begin(),definition.end());
    tags[blockField(tags,"501",2)].value=spelling==1u?"*t17":spelling==2u?"*t-\\U+00E9":name;
    tags[blockField(tags,"501",343)].value="B8";return tags;
}
bool tableGeometryDependencyTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;using R=dx_iface::DependencyReason;using S=dx_iface::DependencyState;using Role=dx_tableResourceRole;
    const auto path=dir/"table-geometry-dependencies.dxf";unsigned profiles=0u,markers=0u,spans=0u,immutable=0u,refusals=0u,mismatches=0u,
        unavailable=0u,prerequisites=0u,quotas=0u,contexts=0u,throws=0u,reached=0u,unreached=0u,imports=0u,finalizations=0u;
    const auto fail=[&](int line){std::cerr<<"TABLE geometry check line="<<line<<" profiles="<<profiles<<" markers="<<markers<<" spans="<<spans
        <<" refusals="<<refusals<<" quotas="<<quotas<<" contexts="<<contexts<<" throws="<<throws<<" finalizations="<<finalizations<<'\n';return false;};
    const auto ready=[](const dx_data& d){const auto r=dx_iface::dependencyReport(d);return r.originalTargetsMatched&&r.geometryPairs==1u&&r.geometryMatched==1u
        &&!r.geometryMismatches&&!r.geometryUnavailable&&!r.ownerTargetsResolved&&!r.recursiveClosureMatched&&!r.outputEligible;};
    const auto rows=[](const dx_data& d){std::pair<const A::DependencyRow*,const A::DependencyRow*> out{};
        if(A::dependencies(d))for(const auto& row:A::dependencies(d)->rows)if(row.source==0x501u){if(row.role==Role::BlockName)out.first=&row;if(row.role==Role::TableGeometryBlock)out.second=&row;}return out;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true})for(unsigned spelling=0u;spelling<3u;++spelling) {
        const auto tags=tableGeometryDependencyDocument(version,blocks,spelling);dx_iface reader;dx_data data;
        if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data)) {
            const auto r=dx_iface::dependencyReport(data);std::cerr<<"TABLE geometry profile "<<version<<'/'<<encoding<<" blocks="<<blocks<<" spelling="<<spelling
                <<" state="<<static_cast<int>(r.state)<<" pairs="<<r.geometryPairs<<" first="<<std::hex<<r.firstSource<<" -> "<<r.firstTarget<<std::dec<<'\n';return fail(__LINE__);
        }
        const auto pair=rows(data);if(!pair.first||!pair.second)return fail(__LINE__);
        for(const auto* row:{pair.first,pair.second})if(!row->geometryCoupled||row->target!=0xB8u||row->coupledTarget!=0xB8u||row->reason!=R::Matched)return fail(__LINE__);
        if(pair.first->coupledGroup!=pair.second->group||pair.second->coupledGroup!=pair.first->group||pair.first->code!=2||pair.second->code!=343)return fail(__LINE__);
        // Independently bind both physical fields and the original target name.
        const auto wire=attdefWireTags(path,encoding==2u);const auto& source=*A::identityInput(data)->source;const auto& audit=*A::dependencies(data);
        for(const auto id:{0x501u,0xB8u}){const auto& record=source.records.at(audit.nodes.at(id).physical);const auto& marker=wire.at(record.marker.ordinal-1u);
            if(marker.code||marker.text!=(id==0x501u?"ACAD_TABLE":"BLOCK_RECORD")||marker.begin!=record.marker.beginOffset||marker.end!=record.marker.endOffset)return fail(__LINE__);++markers;}
        const auto& table=source.records.at(audit.nodes.at(0x501u).physical);
        const auto& name=wire.at(table.marker.ordinal+pair.first->group);const auto& handle=wire.at(table.marker.ordinal+pair.second->group);
        if(name.code!=2||name.text!=tags[blockField(tags,"501",2)].value||handle.code!=343||handle.text!="B8")return fail(__LINE__);
        const auto& target=audit.nodes.at(0xB8u);const auto& original=source.records.at(target.physical);bool targetName=false;
        for(std::size_t i=0u;i<A::dependencyGroups(target).size();++i)if(A::dependencyGroups(target)[i].code()==2) {
            const auto& field=wire.at(original.marker.ordinal+i);if(field.code!=2||field.text!=(spelling==2u?"*T-é":"*T17"))return fail(__LINE__);targetName=true;}
        if(!targetName)return fail(__LINE__);spans+=3u;
        const auto before=symbolFailedAllocations;{SymbolAllocationFailure none(0);if(!ready(data))return fail(__LINE__);}if(symbolFailedAllocations!=before)return fail(__LINE__);
        for(auto& record:data.blockRecords)record.name="edited live name";
        A::clearAdvisoryNames(data);if(!ready(data))return fail(__LINE__);A::repeatDependencies(reader,data);if(!ready(data))return fail(__LINE__);++immutable;++profiles;
    }
    for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true}) {
        const auto good=tableGeometryDependencyDocument("AC1032",blocks);
        const auto retry=[&](dx_iface& reader){dx_data next;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&next,false)||!ready(next))return fail(__LINE__);++imports;return true;};
        for(unsigned fault=0u;fault<12u;++fault) {
            auto tags=good;
            if(fault==0u)tags[blockField(tags,"501",343)].value="B0";
            else if(fault==1u)tags[blockField(tags,"501",2)].value="Definition";
            else if(fault==2u||fault==8u)tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,"501",343)));
            else if(fault==3u)tags[blockField(tags,"501",343)].value="0";
            else if(fault==4u)tags[blockField(tags,"501",343)].value="FFFF";
            else if(fault==5u)tags[blockField(tags,"501",343)].value="D301";
            else if(fault==6u)tags[blockField(tags,"501",2)].value="MISSING_GEOMETRY";
            else if(fault==7u){const auto first=slot(tags,5,"B8")-1u;auto end=first+1u;while(tags[end].code!=0)++end;
                Tags duplicate(tags.begin()+static_cast<std::ptrdiff_t>(first),tags.begin()+static_cast<std::ptrdiff_t>(end));duplicate[1].value="B9";
                tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),duplicate.begin(),duplicate.end());
                tags[blockField(tags,"8",70)].value=std::to_string(std::stoi(tags[blockField(tags,"8",70)].value)+1);}
            else if(fault==9u){tags=tableGeometryDependencyDocument("AC1032",blocks,2u);tags[blockField(tags,"501",2)].value="*T-É";}
            else if(fault==10u)tags[blockField(tags,"501",2)].value="*Model_Space";
            else {const auto at=blockField(tags,"501",343);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{343,"B8"}});}
            if(fault==8u){const auto at=blockField(tags,"501",280);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{102,"{GEOMETRY_HINT"},{2,"*T17"},{343,"B8"},{102,"}"}});}
            dx_iface reader;dx_data data;if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||ready(data))return fail(__LINE__);
            const auto r=dx_iface::dependencyReport(data);const auto pair=rows(data);
            if(fault==0u||fault==1u||fault==10u) {
                if(r.originalTargetsMatched||r.geometryPairs!=1u||r.geometryMatched||r.geometryMismatches!=1u||r.geometryUnavailable||!pair.first||!pair.second
                    ||pair.first->reason!=R::CoupledTargetMismatch||pair.second->reason!=R::CoupledTargetMismatch||pair.first->target==pair.second->target
                    ||pair.first->coupledTarget!=pair.second->target||pair.second->coupledTarget!=pair.first->target)return fail(__LINE__);
                ++mismatches;
            }else if(fault!=7u&&fault!=11u) {
                if(r.originalTargetsMatched||r.geometryPairs!=1u||r.geometryMatched||r.geometryMismatches||r.geometryUnavailable!=1u||!pair.first) {
                    std::cerr<<"TABLE geometry refusal fault="<<fault<<" state="<<static_cast<int>(r.state)<<" pairs="<<r.geometryPairs<<" matched="<<r.geometryMatched
                        <<" unavailable="<<r.geometryUnavailable<<" target-state="<<static_cast<int>(dx_iface::tableReferenceReport(data).state)<<" ambiguous="<<r.ambiguous<<'\n';
                    return fail(__LINE__);
                }
                if(fault==2u||fault==8u){if(pair.second||pair.first->coupledGroup!=DRW_DxfIdentitySource::None||pair.first->reason!=R::CoupledTargetUnavailable)return fail(__LINE__);}
                if(fault==3u&&(!pair.second||pair.second->reason!=R::NullRequired))return fail(__LINE__);
                if(fault==4u&&(!pair.second||pair.second->reason!=R::MissingTarget))return fail(__LINE__);
                if(fault==5u&&(!pair.second||pair.second->reason!=R::WrongKind))return fail(__LINE__);
                if(fault==6u&&pair.first->reason!=R::MissingName)return fail(__LINE__);
                if(fault==9u&&pair.first->reason!=R::NameSourceUnavailable)return fail(__LINE__);++unavailable;
            }else {
                // A duplicate geometry namespace or physical343 already fails
                // an original-body prerequisite; it supplies no coupled proof.
                if(r.state!=S::SourceNotReady||r.originalTargetsMatched||r.geometryPairs||pair.first||pair.second)return fail(__LINE__);
                ++prerequisites;
            }
            if(!retry(reader))return fail(__LINE__);++refusals;
        }
        dx_iface reader;dx_data base;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&base,false)||!ready(base))return fail(__LINE__);
        const auto report=dx_iface::dependencyReport(base);const std::size_t limits[]{report.reservedRows,report.bytes,report.work};
        for(unsigned which=0u;which<3u;++which)for(bool under:{false,true}) {
            dx_iface codec;dx_data actual;A::dependencyLimit(actual,which,limits[which]-(under?1u:0u));
            if(!codec.fileImport(path.string(),&actual,false)||ready(actual)==under||(under&&dx_iface::dependencyReport(actual).state!=S::CaptureFailed)||!retry(codec))return fail(__LINE__);++quotas;
        }
        for(unsigned fault=0u;fault<4u;++fault) {
            dx_data actual;if(!reader.fileImport(path.string(),&actual,false)||!ready(actual))return fail(__LINE__);
            if(fault<2u)A::substituteDependencies(actual,fault!=0u);
            else if(fault==2u){dx_iface other;dx_data d;if(!other.fileImport(path.string(),&d,false)||!ready(d))return fail(__LINE__);A::transplantDependencies(actual,d);}
            else A::substituteTableReferences(actual,false);
            if(dx_iface::dependencyReport(actual).state!=S::ContextMismatch||!retry(reader))return fail(__LINE__);++contexts;
        }
        struct Throwing:dx_iface {unsigned phase=0u;
            void addDxfTableSource(const DRW_Table& v,const DRW_RawDxfObject& raw) override{dx_iface::addDxfTableSource(v,raw);if(phase==1u)throw std::runtime_error("geometry source callback");}
            void addTable(const DRW_Table& v) override{dx_iface::addTable(v);if(phase==2u)throw std::runtime_error("geometry typed callback");}
        } throwing;
        for(unsigned phase=1u;phase<=2u;++phase){throwing.phase=phase;dx_data failed;bool caught=false;
            try{if(throwing.fileImport(path.string(),&failed,false))return fail(__LINE__);}catch(const std::runtime_error&){caught=true;}
            if(!caught||dx_iface::dependencyReport(failed).state!=S::ImportFailed)return fail(__LINE__);
            throwing.phase=0u;if(!retry(throwing))return fail(__LINE__);++throws;}
        for(long offset=0;offset<64;++offset) {
            const auto before=symbolFailedAllocations;{SymbolAllocationFailure fault(offset);A::repeatDependencies(reader,base);}
            if(symbolFailedAllocations!=before){if(ready(base)||dx_iface::dependencyReport(base).state!=S::CaptureFailed)return fail(__LINE__);++reached;}
            else {if(!ready(base))return fail(__LINE__);++unreached;}
            A::repeatDependencies(reader,base);if(!ready(base))return fail(__LINE__);++finalizations;
        }
    }
    std::cout<<"TABLE geometry dependency profiles="<<profiles<<"; original markers="<<markers<<"; physical pair/target spans="<<spans<<"; live-state independence="<<immutable
        <<"; refusals="<<refusals<<"; mismatched pairs="<<mismatches<<"; unavailable pairs="<<unavailable<<"; prerequisite refusals="<<prerequisites<<"; exact/one-under limits="<<quotas<<"; context refusals="<<contexts
        <<"; callback throws="<<throws<<"; reached finalization failures="<<reached<<"; unreached offsets="<<unreached<<"; corrected imports="<<imports<<"; corrected finalizations="<<finalizations<<'\n';
    return profiles==72u&&markers==144u&&spans==216u&&immutable==72u&&refusals==72u&&mismatches==18u&&unavailable==42u&&prerequisites==12u&&quotas==36u&&contexts==24u&&throws==12u
        &&reached>0u&&reached+unreached==384u&&imports==144u&&finalizations==384u;
}
