// SPDX-License-Identifier: GPL-2.0-or-later
// Original reference targets, with locally authored drawing recipes only.
Tags tableReferenceDocument(const std::string& version,bool blocks,unsigned form) {
    const bool cellBlock=form==16u;
    auto tags=tableValueProfileDocument(version,true,cellBlock?12u:7u);
    tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,"501",342)));
    if(!cellBlock)tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,"501",344)));
    tags[blockField(tags,"501",343)].value=blocks?"B3":"B0";
    const auto begin=slot(tags,2,"ENTITIES")+1u;
    const Tags entities{{0,"POINT"},{5,"701"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"1"},{20,"2"},
        {0,"LINE"},{5,"702"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbLine"},{10,"1"},{20,"2"},{30,"3"},{11,"4"},{21,"5"},{31,"6"}};
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(begin),entities.begin(),entities.end());
    if(cellBlock) {
        tags[blockField(tags,"501",340)].value="B3";
        const auto at=blockField(tags,"501",301);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{179,"1"},{331,"703"},{300,"cell-attribute"}});
        const auto body=attdefRoleBody("703","B3",false,"",0u);const auto end=slot(tags,5,"F7")-1u;
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),body.begin(),body.end());
    }else {
        constexpr const char* targets[]={"0","40","43","B0","A0","501","511","513","301","202","204","207","701","702","F6","F7"};
        auto at=blockField(tags,"501",301)+1u;while(tags[at].code!=330)++at;tags[at].value=targets[form];
    }
    if(!blocks) {
        const auto first=slot(tags,5,"501")-1u;auto last=slot(tags,5,"513")+1u;while(tags[last].code!=0)++last;
        Tags body(tags.begin()+static_cast<std::ptrdiff_t>(first),tags.begin()+static_cast<std::ptrdiff_t>(last));
        tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(first),tags.begin()+static_cast<std::ptrdiff_t>(last));
        if(body[2].code!=330)throw std::logic_error("authored TABLE owner position");body[2].value="B0";
        const auto at=slot(tags,2,"ENTITIES")+1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),body.begin(),body.end());
    }
    return tags;
}
bool tableReferenceTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::TableReferenceState;
    const auto path=dir/"table-references.dxf";unsigned profiles=0u,roles=0u,contexts=0u,quotas=0u,reached=0u,unreached=0u,retries=0u,refusals=0u,replays=0u,duplicates=0u,cycles=0u;
    const auto ready=[](const dx_data& data){const auto r=dx_iface::tableReferenceReport(data);return r.state==S::ReferencesReady&&r.sourceTargetsMatched
        &&r.references==r.matched&&!r.ownerTargetsResolved&&!r.outputEligible;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)
        for(bool blocks:{false,true})for(unsigned form=0u;form<17u;++form) {
            const auto good=tableReferenceDocument(version,blocks,form);dx_iface reader;dx_data data;
            if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data)) {
                const auto r=dx_iface::tableReferenceReport(data);std::cerr<<"TABLE references "<<version<<'/'<<encoding<<" blocks="<<blocks<<" form="<<form<<" state="<<static_cast<int>(r.state)
                    <<" refs="<<r.references<<" matched="<<r.matched<<" missing="<<r.missing<<" unavailable="<<r.unavailable<<" target="<<r.firstTarget<<'\n';return false;
            }
            const auto r=dx_iface::tableReferenceReport(data);
            if(r.tables!=1u||r.references!=(form==16u?3u:2u)||r.values!=(form==16u?0u:1u)||r.nullValues!=(form==0u?1u:0u)
                ||r.valueSourceTargetsMatched!=(form!=16u))return false;
            const auto& audit=*A::tableReferences(data);const auto& source=*A::identityInput(data)->source;
            const auto wire=attdefWireTags(path,encoding==2u);
            for(const auto& row:audit.rows)if(row.target) {
                if(row.physical>=source.records.size())return false;const auto& original=source.records[row.physical];
                const auto& marker=wire.at(original.marker.ordinal-1u);
                if(marker.code||marker.text!=original.name||marker.begin!=original.marker.beginOffset||marker.end!=original.marker.endOffset)return false;
                const auto target=audit.targets.find(row.target);
                if(target==audit.targets.end()||target->second.physical!=row.physical||target->second.ambiguous)return false;++roles;
            }
            ++profiles;
        }
    for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true}) {
        const auto good=tableReferenceDocument("AC1032",blocks,1u);
        const auto retry=[&](dx_iface& reader){dx_data next;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&next,false)||!ready(next))return false;++retries;return true;};
        for(unsigned fault=0u;fault<11u;++fault) {
            auto tags=good;
            if(fault==0u){auto at=blockField(tags,"501",301)+1u;while(tags[at].code!=330)++at;tags[at].value="FFFF";}
            else if(fault==1u||fault==2u)tags[blockField(tags,"501",343)].value=fault==1u?"0":"40";
            else if(fault==3u||fault==4u){const auto at=blockField(tags,"501",301);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{fault==3u?342:344,"40"}});}
            else if(fault==5u) {
                auto at=blockField(tags,"501",301)+1u;while(tags[at].code!=330)++at;tags[at].value="704";
                auto end=slot(tags,2,"OBJECTS")+1u;while(tags[end].code!=0||tags[end].value!="ENDSEC")++end;
                const Tags raw{{0,"UNKNOWN_OBJECT"},{5,"704"},{330,"40"},{100,"UnknownPayload"},{1,"opaque"}};
                tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),raw.begin(),raw.end());
            }else if(fault==6u) {
                tags=tableReferenceDocument("AC1032",blocks,16u);tags[blockField(tags,"501",340)].value="B0";
            }else if(fault==7u){tags=tableReferenceDocument("AC1032",blocks,16u);tags[blockField(tags,"501",331)].value="701";}
            else if(fault<10u) {
                const auto at=blockField(tags,"501",301);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{fault==8u?342:344,"704"}});
                auto end=slot(tags,2,"OBJECTS")+1u;while(tags[end].code!=0||tags[end].value!="ENDSEC")++end;
                const Tags raw=fault==8u?Tags{{0,"TABLESTYLE"},{5,"704"},{330,"40"},{100,"AcDbTableStyle"},{280,"0"},{3,"authored style"},{70,"0"},{71,"0"},{40,"1"},{41,"1"},{280,"0"},{281,"0"}}:
                    Tags{{0,"FIELD"},{5,"704"},{330,"40"},{100,"AcDbField"},{1,"AcExpr"},{2,"1+1"},{90,"0"},{93,"0"},{97,"0"}};
                tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),raw.begin(),raw.end());
            }else {
                auto at=blockField(tags,"501",301)+1u;while(tags[at].code!=330)++at;tags[at].value="FFFF";
                appendBlockFields(tags,"501",{{102,"{OPAQUE_TARGET"},{5,"FFFF"},{330,"FFFF"},{102,"}"}});
            }
            struct ReplayOnly:dx_iface {bool selected=true;bool wantsDxfResourceSource() const override{return selected&&dx_iface::wantsDxfResourceSource();}}reader;
            reader.selected=fault!=8u&&fault!=9u;dx_data data;
            if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false))return false;
            if(fault==8u||fault==9u) {
                const auto found=A::resources(data).find(0x704u);
                if(found==A::resources(data).end()||!found->second.rawTypedMatched||found->second.rawTypedMismatch) {
                    std::cerr<<"Authored target replay precondition fault="<<fault<<'\n';return false;
                }
                ++replays;
            }
            const auto r=dx_iface::tableReferenceReport(data);
            if(fault==6u) {
                const auto& dependencies=A::dependencies(data);bool refused=false;
                if(dependencies)for(const auto& row:dependencies->rows)
                    if(row.source==0x501u&&row.role==dx_tableResourceRole::TableCellAttributeDefinition&&row.target==0x703u
                        &&row.reason==dx_iface::DependencyReason::WrongOwner)refused=true;
                if(!refused||dx_iface::dependencyReport(data).originalTargetsMatched)return false;
            }
            if(r.state!=S::Unqualified||r.sourceTargetsMatched||r.ownerTargetsResolved||r.outputEligible
                ||(fault==0u||fault==10u?r.missing!=1u:fault==1u?r.nullRequired!=1u:fault==5u||fault==8u||fault==9u?r.unavailable!=1u:fault==6u?r.wrongCellBlock!=1u:r.wrongKind!=1u)
                ||!retry(reader)) {std::cerr<<"TABLE reference refusal fault="<<fault<<" state="<<static_cast<int>(r.state)<<'\n';return false;}++refusals;
        }
        {
            auto tags=tableReferenceDocument("AC1032",blocks,12u);const auto at=slot(tags,2,"ENTITIES")+1u;
            const Tags repeated{{0,"POINT"},{5,"701"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"9"},{20,"9"}};
            tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),repeated.begin(),repeated.end());dx_iface reader;dx_data data;
            if(!writeBlockBinding(path,tags,encoding))return false;
            const bool read=reader.fileImport(path.string(),&data,false);const auto duplicate=dx_iface::tableReferenceReport(data);
            const auto identities=dx_iface::identitySourceReport(data);
            if(!read||duplicate.state!=S::SourceNotReady||identities.state!=dx_iface::IdentitySourceState::Unqualified||!identities.duplicates
                ||duplicate.sourceTargetsMatched||duplicate.valueSourceTargetsMatched||duplicate.outputEligible||!retry(reader)) {
                std::cerr<<"Duplicate target read="<<read<<" state="<<static_cast<int>(duplicate.state)<<" identity="<<static_cast<int>(dx_iface::identitySourceReport(data).state)<<'\n';return false;
            }
            ++duplicates;
        }
        {
            auto tags=tableReferenceDocument("AC1032",blocks,5u);
            const auto first=slot(tags,5,"501")-1u,last=slot(tags,5,"511")-1u;
            Tags second(tags.begin()+static_cast<std::ptrdiff_t>(first),tags.begin()+static_cast<std::ptrdiff_t>(last));second[1].value="502";
            auto value=blockField(tags,"501",301)+1u;while(tags[value].code!=330)++value;tags[value].value="502";
            const auto at=slot(tags,5,"513")+1u;auto end=at;while(tags[end].code!=0)++end;
            tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),second.begin(),second.end());dx_iface reader;dx_data data;
            if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data))return false;
            const auto r=dx_iface::tableReferenceReport(data);if(r.tables!=2u||r.references!=4u||r.values!=2u||!r.valueSourceTargetsMatched||!retry(reader))return false;++cycles;
        }
        for(unsigned fault=0u;fault<22u;++fault) {
            dx_iface reader;dx_data data;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data))return false;
            if(fault<2u)A::substituteTableReferences(data,fault==1u);
            else if(fault<6u)A::substituteIdentity(data,fault-2u);
            else if(fault<10u)A::substituteStructural(data,fault-6u);
            else if(fault<12u)A::substituteEntitiesRole(data,fault==11u);
            else if(fault<14u)A::substituteTableBody(data,fault==13u);
            else if(fault==14u){dx_iface another;dx_data other;if(!another.fileImport(path.string(),&other,false)||!ready(other))return false;A::transplantTableReferences(data,other);}
            else if(fault<17u)A::substituteBlockRole(data,0u,fault==16u);
            else if(fault<19u) {
                auto& p=A::mutableLayoutAudits(data).begin()->second;
                if(fault==18u)p=std::decay_t<decltype(p)>(p.get(),[](const auto*){});
                else p=std::make_shared<std::decay_t<decltype(*p)>>(*p);
            }else if(fault==19u) {
                dx_iface another;dx_data other;if(!another.fileImport(path.string(),&other,false)||!ready(other))return false;
                A::mutableResource(data,0x40u)=A::resources(other).at(0x40u);
            }else if(fault==20u) {
                auto& p=A::mutableResource(data,0x40u).raw;p=std::make_shared<DRW_RawDxfObject>(*p);
            }else {
                auto& p=A::mutableResource(data,0x40u).model;p=std::decay_t<decltype(p)>(p.get(),[](const auto*){});
            }
            const auto r=dx_iface::tableReferenceReport(data);
            if(r.state!=S::ContextMismatch||r.sourceTargetsMatched||r.valueSourceTargetsMatched||r.outputEligible||!retry(reader))return false;++contexts;
        }
        dx_iface baseline;dx_data base;if(!writeBlockBinding(path,good,encoding)||!baseline.fileImport(path.string(),&base,false)||!ready(base))return false;
        const auto r=dx_iface::tableReferenceReport(base);const std::size_t uses[]={r.references,r.bytes,r.work};
        for(unsigned which=0u;which<3u;++which)for(bool under:{false,true}) {
            dx_iface reader;dx_data data;A::tableReferenceLimit(data,which,uses[which]-(under?1u:0u));
            if(!reader.fileImport(path.string(),&data,false)||ready(data)==under)return false;
            if(under&&dx_iface::tableReferenceReport(data).state!=S::CaptureFailed)return false;++quotas;
        }
        for(long offset=0;offset<64;++offset) {
            dx_iface reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!ready(data))return false;
            const auto before=symbolFailedAllocations;{SymbolAllocationFailure failure(offset);A::repeatTableReferences(reader,data);}
            if(symbolFailedAllocations!=before){if(dx_iface::tableReferenceReport(data).state!=S::CaptureFailed)return false;++reached;}
            else {if(!ready(data))return false;++unreached;}
            if(!retry(reader))return false;
        }
    }
    std::cout<<"TABLE original reference profiles="<<profiles<<"; original target markers="<<roles<<"; missing/null/kind/producer/cell-block refusals="<<refusals
        <<"; replay-only target refusals="<<replays<<"; duplicate target refusals="<<duplicates<<"; bounded mutual-reference controls="<<cycles<<"; immutable/cross-import refusals="<<contexts<<"; exact/one-under limits="<<quotas<<"; reached allocation failures="<<reached
        <<"; unreached offsets="<<unreached<<"; corrected imports="<<retries<<'\n';
    return profiles==408u&&roles>0u&&refusals==66u&&replays==12u&&duplicates==6u&&cycles==6u&&contexts==132u&&quotas==36u&&reached>0u&&reached+unreached==384u&&retries==594u;
}
