// SPDX-License-Identifier: GPL-2.0-or-later
// Authored dictionary280/350 ownership witness; no external drawings.
Tags effectiveOwnershipDocument(const std::string& version,bool blocks,bool hardFlag,bool hardCode,bool wrongOwner,bool withDefault=false) {
    if(withDefault) {
        auto tags=plotSourceDocument(version,blocks,0u);
        tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,"D301",390)));
        tags[blockField(tags,"E801",280)].value=hardFlag?"1":"0";
        tags[blockField(tags,"E801",350)].code=hardCode?360:350;
        auto at=blockField(tags,"E802",102),end=at+1u;while(tags[end].code!=102||tags[end].value!="}")++end;
        tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at),tags.begin()+static_cast<std::ptrdiff_t>(end+1u));
        tags[blockField(tags,"E802",330)].value=wrongOwner?"40":"E801";tags[blockField(tags,"40",280)].value="0";
        return tags;
    }
    auto tags=dependencyDocument(version,blocks,0u);tags[blockField(tags,"40",280)].value="0";
    appendBlockFields(tags,"40",{{3,"OwnershipScope"},{350,"E901"}});
    const Tags objects{{0,"DICTIONARY"},{5,"E901"},{330,"40"},{100,"AcDbDictionary"},{280,hardFlag?"1":"0"},{281,"1"},
        {3,"OwnedChild"},{hardCode?360:350,"E902"},{0,"DICTIONARY"},{5,"E902"},{330,wrongOwner?"40":"E901"},{100,"AcDbDictionary"},{280,"0"},{281,"1"}};
    auto at=slot(tags,2,"OBJECTS")+1u;while(tags[at].code!=0||tags[at].value!="ENDSEC")++at;
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),objects.begin(),objects.end());return tags;
}
const DxIfaceTableSourceTestAccess::DependencyRow* effectiveOwnershipRow(const dx_data& data,std::uint32_t source,std::uint32_t target) {
    const auto& audit=DxIfaceTableSourceTestAccess::dependencies(data);if(!audit)return nullptr;
    for(const auto& row:audit->rows)if(row.source==source&&row.target==target
        &&(row.role==dx_tableResourceRole::DictionarySoftOwner||row.role==dx_tableResourceRole::DictionaryHardOwner))return &row;
    return nullptr;
}
bool effectiveOwnershipTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;using O=A::Ownership;using R=dx_iface::DependencyReason;
    const auto path=dir/"effective-ownership.dxf";unsigned profiles=0u,markers=0u,spans=0u,immutable=0u,ownerControls=0u,ownerRefusals=0u,
        cycles=0u,cycleRefusals=0u,policyControls=0u,unknownPolicies=0u,quotas=0u,reached=0u,unreached=0u,imports=0u,finalizations=0u;
    const auto ready=[](const dx_data& data){const auto r=dx_iface::dependencyReport(data);
        return r.originalTargetsMatched&&!r.ownerTargetsResolved&&!r.recursiveClosureMatched&&!r.outputEligible;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true})
        for(bool withDefault:{false,true})for(bool flag:{false,true})for(bool code:{false,true}) {
        const auto tags=effectiveOwnershipDocument(version,blocks,flag,code,false,withDefault);dx_iface reader;dx_data data;
        const auto source=withDefault?0xE801u:0xE901u,target=withDefault?0xE802u:0xE902u;
        if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data)) {
            const auto r=dx_iface::dependencyReport(data);std::cerr<<"Effective ownership "<<version<<'/'<<encoding<<" blocks="<<blocks<<" default="<<withDefault<<" flag="<<flag<<" code="<<code
                <<" state="<<static_cast<int>(r.state)<<" first="<<std::hex<<r.firstSource<<" -> "<<r.firstTarget<<std::dec<<'\n';return false;
        }
        const auto* row=effectiveOwnershipRow(data,source,target);const auto expected=code?O::PhysicalHard:flag?O::DictionaryFlagHard:O::ExplicitSoft;
        if(!row||row->ownership!=expected||row->reason!=R::Matched||row->code!=(code?360:350)||row->dictionaryFlagGroup==DRW_DxfIdentitySource::None)return false;
        const auto before=symbolFailedAllocations;{SymbolAllocationFailure none(0);if(!ready(data))return false;}if(symbolFailedAllocations!=before)return false;
        const auto wire=attdefWireTags(path,encoding==2u);const auto& physical=*A::identityInput(data)->source;
        const auto id=A::identityAudit(data)->identities.at(source);const auto& record=physical.records.at(id);const auto& marker=wire.at(record.marker.ordinal-1u);
        if(marker.code||marker.text!=(withDefault?"DICTIONARYWDFLT":"DICTIONARY")||marker.begin!=record.marker.beginOffset||marker.end!=record.marker.endOffset)return false;++markers;
        const auto& scalar=wire.at(record.marker.ordinal+row->group);const auto& flagScalar=wire.at(record.marker.ordinal+row->dictionaryFlagGroup);
        std::uint32_t value=0u;const auto parsed=std::from_chars(scalar.text.data(),scalar.text.data()+scalar.text.size(),value,16);
        if(scalar.code!=(code?360:350)||parsed.ec!=std::errc{}||parsed.ptr!=scalar.text.data()+scalar.text.size()||value!=target||flagScalar.code!=280||flagScalar.integer!=(flag?1:0))return false;spans+=2u;
        if(withDefault)for(const auto& r:A::dependencies(data)->rows)if(r.source==source&&r.role==dx_tableResourceRole::DictionaryDefault&&r.ownership!=O::None)return false;
        for(auto& dictionary:data.dictionaries)dictionary.hardOwner^=1;
        for(auto& dictionary:data.dictionaryWithDefaults)dictionary.hardOwner^=1;
        if(!ready(data)||effectiveOwnershipRow(data,source,target)->ownership!=expected)return false;++immutable;++profiles;
    }
    for(unsigned encoding=0u;encoding<3u;++encoding) {
        const auto good=effectiveOwnershipDocument("AC1032",false,true,false,false);
        const auto retry=[&](dx_iface& reader){dx_data next;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&next,false)||!ready(next))return false;++imports;return true;};
        for(bool blocks:{false,true})for(bool withDefault:{false,true})for(bool flag:{false,true})for(bool code:{false,true}) {
            auto tags=effectiveOwnershipDocument("AC1032",blocks,flag,code,true,withDefault);dx_iface reader;dx_data data;
            if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false))return false;
            const auto* row=effectiveOwnershipRow(data,withDefault?0xE801u:0xE901u,withDefault?0xE802u:0xE902u);
            const bool hard=flag||code;
            if(!row||ready(data)==hard||row->reason!=(hard?R::WrongOwner:R::Matched)||(hard&&!dx_iface::dependencyReport(data).wrongOwner))return false;
            if(hard)++ownerRefusals;if(!retry(reader))return false;++ownerControls;
        }
        for(bool blocks:{false,true})for(bool flag:{false,true})for(bool code:{false,true}) {
            auto tags=effectiveOwnershipDocument("AC1032",blocks,flag,code,false);tags[blockField(tags,"E901",330)].value="E902";
            tags[blockField(tags,"E902",280)].value=flag?"1":"0";appendBlockFields(tags,"E902",{{3,"Back"},{code?360:350,"E901"}});
            dx_iface reader;dx_data data;if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false))return false;
            const bool hard=flag||code;const auto r=dx_iface::dependencyReport(data);
            if(ready(data)==hard||(hard?r.ownershipCycles<2u:r.ownershipCycles!=0u))return false;
            if(hard)++cycleRefusals;if(!retry(reader))return false;++cycles;
        }
        for(unsigned mode=0u;mode<3u;++mode)for(bool code:{false,true}) {
            auto tags=effectiveOwnershipDocument("AC1032",false,false,code,false);
            if(mode<2u)tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,"E901",280)));
            if(mode>0u){const auto at=blockField(tags,"E901",281);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{102,"{OWNERSHIP_HINT"},{280,"1"},{102,"}"}});}
            dx_iface reader;dx_data data;if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false))return false;
            const auto* row=effectiveOwnershipRow(data,0xE901u,0xE902u);const bool unknown=mode<2u&&!code;
            if(!row||ready(data)==unknown||row->ownership!=(code?O::PhysicalHard:unknown?O::Unqualified:O::ExplicitSoft)
                ||row->reason!=(unknown?R::OwnershipUnknown:R::Matched)||(mode<2u&&row->dictionaryFlagGroup!=DRW_DxfIdentitySource::None))return false;
            const auto* model=dynamic_cast<const DRW_Dictionary*>(A::resource(data,0xE901u));if(!model||model->hardOwner!=0)return false;
            if(unknown)++unknownPolicies;if(!retry(reader))return false;++policyControls;
        }
        for(bool withDefault:{false,true}) {
            const auto tags=effectiveOwnershipDocument("AC1032",false,true,false,false,withDefault);dx_iface reader;dx_data base;
            if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&base,false)||!ready(base))return false;
            const auto report=dx_iface::dependencyReport(base);const std::size_t limits[]{report.reservedRows,report.bytes,report.work};
            for(unsigned which=0u;which<3u;++which)for(bool under:{false,true}) {
                dx_iface codec;dx_data actual;A::dependencyLimit(actual,which,limits[which]-(under?1u:0u));
                if(!codec.fileImport(path.string(),&actual,false)||ready(actual)==under||(!under&&dx_iface::dependencyReport(actual).state!=dx_iface::DependencyState::OriginalTargetsMatched))return false;
                if(!retry(codec))return false;++quotas;
                if(!writeBlockBinding(path,tags,encoding))return false;
            }
            for(long offset=0;offset<64;++offset) {
                const auto before=symbolFailedAllocations;{SymbolAllocationFailure fault(offset);A::repeatDependencies(reader,base);}
                if(symbolFailedAllocations!=before){if(ready(base)||dx_iface::dependencyReport(base).state!=dx_iface::DependencyState::CaptureFailed)return false;++reached;}
                else {if(!ready(base))return false;++unreached;}
                A::repeatDependencies(reader,base);if(!ready(base))return false;++finalizations;
            }
        }
    }
    std::cout<<"Effective dictionary ownership profiles="<<profiles<<"; original markers="<<markers<<"; flag/reference spans="<<spans<<"; live-state independence="<<immutable
        <<"; owner controls="<<ownerControls<<"; owner refusals="<<ownerRefusals<<"; cycle controls="<<cycles<<"; cycle refusals="<<cycleRefusals
        <<"; absent/opaque policy controls="<<policyControls<<"; unknown policies="<<unknownPolicies<<"; exact/one-under limits="<<quotas
        <<"; reached finalization failures="<<reached<<"; unreached offsets="<<unreached<<"; corrected imports="<<imports<<"; corrected finalizations="<<finalizations<<'\n';
    return profiles==192u&&markers==192u&&spans==384u&&immutable==192u&&ownerControls==48u&&ownerRefusals==36u&&cycles==24u&&cycleRefusals==18u
        &&policyControls==18u&&unknownPolicies==6u&&quotas==36u&&reached>0u&&reached+unreached==384u&&imports==126u&&finalizations==384u;
}
