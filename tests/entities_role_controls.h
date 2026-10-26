// SPDX-License-Identifier: GPL-2.0-or-later
// Authored runtime drawings. Original selected ENTITIES target roles are not
// complete BLOCKS/dependency/live/output or native consumer/kernel evidence.
Tags entitiesRoleDocument(const std::string& version) {
    auto tags=layoutRoleDocument(version);const auto at=slot(tags,0,"EOF")-1u;
    Tags body{{0,"POINT"},{5,"101"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"1"},{20,"2"},
        {0,"LINE"},{5,"102"},{330,"B1"},{100,"AcDbEntity"},{67,"1"},{410,"Sheet 1"},{8,"0"},{100,"AcDbLine"},{10,"1"},{20,"2"},{11,"3"},{21,"4"},
        {0,"POLYLINE"},{5,"D100"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDb3dPolyline"},{66,"1"},{10,"0"},{20,"0"},{30,"0"},{70,"8"}};
    for(const char* id:{"D101","D102"})body.insert(body.end(),{{0,"VERTEX"},{5,id},{330,"D100"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbVertex"},{100,"AcDb3dPolylineVertex"},{10,"1"},{20,"2"},{30,"3"},{70,"32"}});
    body.insert(body.end(),{{0,"SEQEND"},{5,"D103"},{330,"D100"},{100,"AcDbEntity"},{8,"0"},
        {0,"INSERT"},{5,"E100"},{330,"B1"},{100,"AcDbEntity"},{67,"1"},{410,"Sheet 1"},{8,"0"},{100,"AcDbBlockReference"},{2,"UNRESOLVED-DEPENDENCY"},{66,"1"},{10,"1"},{20,"2"},
        {0,"ATTRIB"},{5,"E101"},{330,"E100"},{100,"AcDbEntity"},{67,"1"},{410,"Sheet 1"},{8,"0"},{100,"AcDbText"},{10,"0"},{20,"0"},{40,"1"},{1,"attribute"},{100,"AcDbAttribute"},{2,"TAG"},{70,"0"},
        {0,"SEQEND"},{5,"E102"},{330,"E100"},{100,"AcDbEntity"},{67,"1"},{410,"Sheet 1"},{8,"0"}});
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),body.begin(),body.end());return tags;
}
void entitiesRoleExtraBlock(Tags& tags,bool layout) {
    const auto at=slot(tags,0,"ENDTAB");Tags body{{0,"BLOCK_RECORD"},{5,"B2"},{330,"8"},{100,"AcDbSymbolTableRecord"},{2,layout?"*Paper_Space0":"Definition"},{100,"AcDbBlockTableRecord"},{70,"0"},{340,layout?"A2":"0"}};
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),body.begin(),body.end());
    if(layout) {
        appendBlockFields(tags,"43",{{3,"Sheet 2"},{350,"A2"}});
        const auto p=slot(tags,5,"A0")-1u;const Tags value{{0,"LAYOUT"},{5,"A2"},{330,"43"},{100,"AcDbPlotSettings"},{70,"0"},{76,"0"},{100,"AcDbLayout"},{1,"Sheet 2"},{70,"0"},{71,"2"},{330,"B2"}};
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(p),value.begin(),value.end());
    }
}
bool entitiesRoleTests(const std::filesystem::path& dir) {
    struct Stage {const char* name="profiles";bool done=false;~Stage(){if(!done)std::cerr<<"ENTITIES roles stage="<<name<<'\n';}} stage;
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::EntitiesRoleState;using R=A::EntitiesRoleReason;
    const auto path=dir/"entities-roles.dxf";unsigned profiles=0u,origins=0u,variants=0u,refusals=0u,contexts=0u,quotas=0u,retries=0u;long allocations=0;
    const auto complete=[](const dx_data& d){const auto r=dx_iface::entitiesRoleReport(d);return r.state==S::SelectedRolesReady&&r.selected==9u&&r.matched==9u
        &&r.primaries==4u&&r.children==5u&&r.model==5u&&r.paper==4u&&r.selectedRolesMatched&&!r.wholeDocumentMatched&&!r.nativeNamesQualified&&!r.ownerTargetsResolved&&!r.outputEligible;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding) {
        if(!writeBlockBinding(path,entitiesRoleDocument(version),encoding))return false;dx_iface reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||!complete(data)) {const auto r=dx_iface::entitiesRoleReport(data);std::cerr<<"profile="<<version<<'/'<<encoding<<" state="<<static_cast<int>(r.state)<<" matched="<<r.matched<<'\n';
            if(A::entitiesRole(data))for(const auto& row:A::entitiesRole(data)->rows)std::cerr<<row.self<<" reason="<<static_cast<int>(row.reason)<<'\n';return false;}
        const auto r=dx_iface::entitiesRoleReport(data);if(r.spacePresent!=4u||r.layoutPresent!=4u)return false;
        const auto wire=attdefWireTags(path,encoding==2u);const auto& audit=*A::entitiesRole(data);const auto& identity=*audit.graph->identity->source;
        for(const auto& row:audit.rows) {
            const auto& source=*audit.scopes[row.section];const auto& record=source.physical->records[row.record];const auto& member=source.members[row.record];
            const auto& physical=identity.records[row.physical];const auto& marker=wire[physical.marker.ordinal-1u];
            if(marker.text!=record.name||marker.begin!=record.marker.beginOffset||marker.end!=record.marker.endOffset)return false;
            const auto& owner=wire[record.spans[member.ownerGroup].ordinal-1u];if(owner.code!=330||std::stoul(owner.text,nullptr,16)!=row.wireOwner
                ||row.block!=(row.paper?0xB1u:0xB0u)||row.layout!=(row.paper?0xA1u:0xA0u))return false;
            ++origins;
        }
        ++profiles;
    }
    for(bool binary:{false,true}) {
        const auto good=entitiesRoleDocument("AC1032");
        stage.name="original/default/name/opaque/repeated-scope variants";
        for(unsigned kind=0u;kind<8u;++kind) {
            auto tags=good;
            if(kind==0u)tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,"102",410)));
            if(kind==1u)for(const char* id:{"E101","E102"})tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,id,410)));
            if(kind==2u) {const auto at=blockField(tags,"101",100)+1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{67,"0"},{410,"Model"}});}
            if(kind==3u)for(auto& t:tags) {if((t.code==1||t.code==3)&&t.value=="Sheet 1")t.value="Sheet-é";if(t.code==410)t.value="\\U+0053heet-\\U+00E9";}
            if(kind==4u)for(auto& t:tags)if(t.code==410)t.value="sheet 1";
            if(kind==5u)appendBlockFields(tags,"101",{{102,"{OPAQUE"},{67,"1"},{410,"false paper layout"},{330,"DEAD"},{102,"}"}});
            if(kind==6u) {const auto at=slot(tags,5,"D100")-1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{0,"ENDSEC"},{0,"SECTION"},{2,"ENTITIES"}});}
            if(kind==7u) {const auto at=slot(tags,0,"EOF");tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{0,"SECTION"},{2,"ENTITIES"},{0,"ENDSEC"}});}
            if(!write(path,tags,binary))return false;dx_iface reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!complete(data)){std::cerr<<"variant="<<kind<<'\n';return false;}
            if(kind==1u&&(dx_iface::entitiesCompoundReport(data).state!=dx_iface::EntitiesCompoundState::Unqualified||dx_iface::entitiesRoleReport(data).layoutPresent!=2u))return false;
            ++variants;
        }
        stage.name="source target/space/name/unsupported refusals";
        for(unsigned kind=0u;kind<13u;++kind) {
            auto tags=good;R expected=R::BodyNotReady;std::uint32_t target=0x101u;
            if(kind==0u||kind==1u){tags[blockField(tags,"101",330)].value=kind?"DEAD":"40";expected=R::OwnerMissing;}
            if(kind==2u){tags[blockField(tags,"101",330)].value="B1";expected=R::SpaceConflict;}
            if(kind==3u){tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,"102",67)));expected=R::SpaceConflict;target=0x102u;}
            if(kind==4u){appendBlockFields(tags,"101",{{410,"Sheet 1"}});expected=R::LayoutConflict;}
            if(kind==5u){tags[blockField(tags,"102",410)].value="Model";expected=R::LayoutConflict;target=0x102u;}
            if(kind==6u){entitiesRoleExtraBlock(tags,false);tags[blockField(tags,"101",330)].value="B2";expected=R::NotLayoutBlock;}
            if(kind==7u){entitiesRoleExtraBlock(tags,true);tags[blockField(tags,"102",330)].value="B2";tags[blockField(tags,"102",410)].value="Sheet 2";expected=R::InactivePaper;target=0x102u;}
            if(kind==8u){for(auto& t:tags)if((t.code==1||t.code==3)&&t.value=="Sheet 1")t.value="Sheet-é";tags[blockField(tags,"102",410)].value="SHEET-É";expected=R::LayoutConflict;target=0x102u;}
            if(kind==9u)appendBlockFields(tags,"101",{{67,"0"},{67,"0"}});
            if(kind==10u){tags[blockField(tags,"D101",330)].value="40";target=0xD101u;}
            if(kind==11u){tags[blockField(tags,"E101",410)].value="Model";target=0xE101u;}
            if(kind==12u){tags[blockField(tags,"102",410)].value="\\U+D800";expected=R::TextUnqualified;target=0x102u;}
            if(!write(path,tags,binary))return false;dx_iface reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::entitiesRoleReport(data).state!=S::Unqualified){std::cerr<<"refusal="<<kind<<'\n';return false;}
            const auto& rows=A::entitiesRole(data)->rows;const auto row=std::find_if(rows.begin(),rows.end(),[&](const auto& v){return v.self==target;});
            if(row==rows.end()||row->reason!=expected){std::cerr<<"reason kind="<<kind<<" actual="<<(row==rows.end()?-1:static_cast<int>(row->reason))<<'\n';return false;}
            if(!write(path,good,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++refusals;
        }
        stage.name="unsupported whole-scope and frozen editable separation";
        {auto tags=good;const auto at=slot(tags,0,"EOF")-1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{0,"ARC"},{5,"103"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbCircle"},{10,"0"},{20,"0"},{40,"1"},{100,"AcDbArc"},{50,"0"},{51,"90"}});
            if(!write(path,tags,binary))return false;dx_iface reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;
            const auto r=dx_iface::entitiesRoleReport(data);if(r.state!=S::Unqualified||r.unsupported!=1u||r.matched!=9u||!r.selectedRolesMatched||r.wholeDocumentMatched)return false;++contexts;}
        if(!write(path,good,binary))return false;dx_iface baseline;dx_data base;if(!baseline.fileImport(path.string(),&base,false)||!complete(base))return false;
        base.blockRecords.front().name="edited block";base.layouts.front().name="edited layout";base.mBlock->ent.front()->parentHandle=0x40u;
        if(!complete(base))return false;{SymbolAllocationFailure noAllocation(0);if(!complete(base))return false;}++contexts;
        {auto tags=good;tags[blockField(tags,"B0",2)].value="Not the model-space block";if(!write(path,tags,binary))return false;dx_iface reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||!dx_iface::layoutRoleReport(data).rolesMatched||dx_iface::entitiesRoleReport(data).state!=S::SourceNotReady)return false;++contexts;}
        {if(!write(path,layoutRoleDocument("AC1032"),binary))return false;dx_iface reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::entitiesRoleReport(data).state!=S::NoSelected||dx_iface::entitiesRoleReport(data).selectedRolesMatched)return false;++contexts;}
        if(!write(path,good,binary))return false;
        {EntitiesPrimaryProbe reader;reader.fault=0;dx_data data;if(!reader.fileImport(path.string(),&data,false)||dx_iface::entitiesRoleReport(data).state!=S::Unqualified)return false;
            const auto& rows=A::entitiesRole(data)->rows;if(rows.front().reason!=R::BodyNotReady)return false;
            reader.fault=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++refusals;}
        stage.name="quota/context/allocation recovery";
        for(bool work:{false,true})for(bool exact:{false,true}) {const auto r=dx_iface::entitiesRoleReport(base);dx_iface reader;dx_data data;A::entitiesRoleLimit(data,work,(work?r.work:r.bytes)-(exact?0u:1u));
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::entitiesRoleReport(data).state!=(exact?S::SelectedRolesReady:S::CaptureFailed))return false;++quotas;}
        for(unsigned kind=0u;kind<7u;++kind) {dx_iface reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;
            if(kind<2u)A::substituteEntitiesRole(data,kind==1u);else if(kind==2u)A::substituteLayoutRole(data,false);else if(kind==3u)A::substituteIdentity(data,0u);
            else if(kind>=5u)A::substituteEntitiesRoleBody(data,kind==6u);
            else {dx_data copy;A::copyBindingContext(copy,data);if(dx_iface::entitiesRoleReport(copy).state!=S::ContextMismatch)return false;++contexts;continue;}
            if(dx_iface::entitiesRoleReport(data).state!=S::ContextMismatch)return false;++contexts;}
        bool recovered=false;for(long fault=0;fault<2000;++fault) {{SymbolAllocationFailure failure(fault);A::repeatEntitiesRole(baseline,base);}
            if(complete(base)){recovered=true;break;}if(dx_iface::entitiesRoleReport(base).state!=S::CaptureFailed)return false;
            A::repeatEntitiesRole(baseline,base);if(!complete(base))return false;++allocations;}
        if(!recovered)return false;
        stage.name="failed tail and callback retry";
        for(bool callback:{false,true}) {auto tags=good;if(!callback)tags.pop_back();if(!write(path,tags,binary))return false;EntitiesPrimaryProbe reader;reader.throwPoint=callback;dx_data data;bool threw=false,read=false;
            try{read=reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
            if(read||threw!=callback||dx_iface::entitiesRoleReport(data).state!=S::ImportFailed)return false;
            reader.throwPoint=false;if(!write(path,good,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++retries;}
    }
    stage.name="scratch collision isolation and filesystem failure";
    std::error_code error;const auto first=reserveTestScratch(dir,"collision",error),second=reserveTestScratch(dir,"collision",error);
    if(first.empty()||second.empty()||first==second||error)return false;
    {std::ofstream occupied(dir/"occupied-0");occupied<<"preserve unrelated scratch sentinel";}
    const auto next=reserveTestScratch(dir,"occupied",error);if(next!=dir/"occupied-1"||error)return false;
    {std::ifstream in(dir/"occupied-0");if(std::string(std::istreambuf_iterator<char>(in),{})!="preserve unrelated scratch sentinel")return false;}
    if(!reserveTestScratch(dir/"missing-parent","failed",error).empty()||!error)return false;
    std::cout<<"ENTITIES roles profiles="<<profiles<<" independently walked origins="<<origins<<" variants="<<variants<<" refusals="<<refusals<<" contexts="<<contexts
        <<" quotas="<<quotas<<" allocation faults="<<allocations<<" tail/callback retries="<<retries<<'\n';stage.done=true;return true;
}
