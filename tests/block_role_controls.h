// SPDX-License-Identifier: GPL-2.0-or-later
// Original scope/owner/content-space joins, not dependencies/live/output/native
// semantics. Every drawing is authored here at runtime.
Tags blockRoleDocument(const std::string& version) {
    auto tags=layoutRoleDocument(version);entitiesRoleExtraBlock(tags,true);
    const auto tableEnd=slot(tags,0,"ENDTAB");const Tags definition{{0,"BLOCK_RECORD"},{5,"B3"},{330,"8"},{100,"AcDbSymbolTableRecord"},{2,"Definition"},{100,"AcDbBlockTableRecord"},{70,"0"},{340,"0"}};
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(tableEnd),definition.begin(),definition.end());tags[blockField(tags,"8",70)].value="4";
    Tags blocks{{0,"SECTION"},{2,"BLOCKS"}};
    const auto add=[&](const char* owner,const char* begin,const char* end,const char* name,Tags body){
        blocks.insert(blocks.end(),{{0,"BLOCK"},{5,begin},{330,owner},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},{2,name},{70,"0"},{10,"0"},{20,"0"},{30,"0"}});
        blocks.insert(blocks.end(),body.begin(),body.end());blocks.insert(blocks.end(),{{0,"ENDBLK"},{5,end},{330,owner},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"}});
    };
    add("B0","F0","F1","*Model_Space",{});add("B1","F2","F3","*Paper_Space",{});
    Tags paper{{0,"POINT"},{5,"201"},{330,"B2"},{100,"AcDbEntity"},{67,"1"},{410,"Sheet 2"},{8,"0"},{100,"AcDbPoint"},{10,"1"},{20,"2"},
        {0,"LINE"},{5,"202"},{330,"B2"},{100,"AcDbEntity"},{67,"1"},{410,"Sheet 2"},{8,"0"},{100,"AcDbLine"},{10,"1"},{20,"2"},{11,"3"},{21,"4"},
        {0,"POLYLINE"},{5,"203"},{330,"B2"},{100,"AcDbEntity"},{67,"1"},{410,"Sheet 2"},{8,"0"},{100,"AcDb3dPolyline"},{70,"8"},{10,"0"},{20,"0"},
        {0,"VERTEX"},{5,"204"},{330,"203"},{100,"AcDbEntity"},{67,"1"},{410,"Sheet 2"},{8,"0"},{100,"AcDbVertex"},{100,"AcDb3dPolylineVertex"},{70,"32"},{10,"1"},{20,"2"},{30,"3"},
        {0,"SEQEND"},{5,"205"},{330,"203"},{100,"AcDbEntity"},{67,"1"},{410,"Sheet 2"},{8,"0"},
        {0,"INSERT"},{5,"206"},{330,"B2"},{100,"AcDbEntity"},{67,"1"},{410,"Sheet 2"},{8,"0"},{100,"AcDbBlockReference"},{2,"Definition"},{66,"1"},{10,"1"},{20,"2"},
        {0,"ATTRIB"},{5,"207"},{330,"206"},{100,"AcDbEntity"},{67,"1"},{410,"Sheet 2"},{8,"0"},{100,"AcDbText"},{10,"0"},{20,"0"},{40,"1"},{1,"attribute"},{100,"AcDbAttribute"},{2,"TAG"},{70,"0"},
        {0,"SEQEND"},{5,"208"},{330,"206"},{100,"AcDbEntity"},{67,"1"},{410,"Sheet 2"},{8,"0"}};
    add("B2","F4","F5","*Paper_Space0",paper);
    add("B3","F6","F7","Definition",{{0,"POINT"},{5,"301"},{330,"B3"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"1"},{20,"2"},
        {0,"POLYLINE"},{5,"302"},{330,"B3"},{100,"AcDbEntity"},{8,"0"},{100,"AcDb2dPolyline"},{70,"0"},{10,"0"},{20,"0"},
        {0,"SEQEND"},{5,"303"},{330,"302"},{100,"AcDbEntity"},{8,"0"}});
    blocks.push_back({0,"ENDSEC"});const auto at=slot(tags,0,"EOF");tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),blocks.begin(),blocks.end());return tags;
}
struct BlockRoleProbe final:dx_iface {
    bool selected=true,copyChildren=false,throwPoint=false;unsigned depth=0u;
    bool wantsDxfBlockSource() const override{return selected&&dx_iface::wantsDxfBlockSource();}
    bool wantsDxfBlockChildrenSource() const override{return selected&&dx_iface::wantsDxfBlockChildrenSource();}
    bool wantsDxfBlockDeliverySource() const override{return selected&&dx_iface::wantsDxfBlockDeliverySource();}
    void addBlock(const DRW_Block& b) override{++depth;dx_iface::addBlock(b);}
    void endBlock() override{if(depth)--depth;dx_iface::endBlock();}
    void addPoint(const DRW_Point& v) override{dx_iface::addPoint(v);if(throwPoint)throw std::runtime_error("original BLOCKS role callback");}
    void addDxfBlockChildrenSource(const DRW_DxfBlockChildrenSource& v) override{if(copyChildren){const auto other=v;dx_iface::addDxfBlockChildrenSource(other);}else dx_iface::addDxfBlockChildrenSource(v);}
};
bool blockRoleTests(const std::filesystem::path& dir) {
    struct Stage{const char* name="profiles";bool done=false;~Stage(){if(!done)std::cerr<<"BLOCKS roles stage="<<name<<'\n';}}stage;
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::BlockRoleState;using R=A::BlockRoleReason;
    const auto path=dir/"block-roles.dxf";unsigned profiles=0u,origins=0u,identityFields=0u,variants=0u,refusals=0u,contexts=0u,quotas=0u,failures=0u;long allocations=0;
    const auto complete=[](const dx_data& d){const auto r=dx_iface::blockRoleReport(d);return r.state==S::RolesReady&&r.scopes==4u&&r.records==19u&&r.published==4u&&r.qualifiedScopes==4u
        &&r.selected==19u&&r.matched==19u&&!r.unsupported&&r.endpoints==8u&&r.children==5u&&r.paper==8u&&r.definitions==3u&&r.physicalCensusComplete&&r.selectedRolesMatched
        &&!r.wholeDocumentMatched&&!r.ownerTargetsResolved&&!r.nativeNamesQualified&&!r.outputEligible;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding) {
        if(!writeBlockBinding(path,blockRoleDocument(version),encoding))return false;BlockRoleProbe reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||reader.depth||!complete(data)){const auto r=dx_iface::blockRoleReport(data);std::cerr<<version<<'/'<<encoding<<" state="<<static_cast<int>(r.state)<<" records="<<r.records<<" matched="<<r.matched<<'\n';
            if(A::blockRole(data)){for(const auto& scope:A::blockRole(data)->scopes)std::cerr<<"scope="<<scope.handle<<" reason="<<static_cast<int>(scope.reason)<<'\n';for(const auto& row:A::blockRole(data)->rows)std::cerr<<"row="<<row.self<<" reason="<<static_cast<int>(row.reason)<<'\n';}return false;}
        const auto& a=*A::blockRole(data);const auto& source=*a.identity->source;const auto wire=attdefWireTags(path,encoding==2u);
        for(const auto& row:a.rows){const auto& p=source.records[row.physical];const auto& w=wire[p.marker.ordinal-1u];
            if(w.text!=p.name||w.begin!=p.marker.beginOffset||w.end!=p.marker.endOffset||row.self==0u||row.parent>row.physical)return false;
            bool self=false,owner=false;unsigned depth=0u;
            for(std::size_t i=p.marker.ordinal;i<wire.size()&&wire[i].code!=0;++i){const auto& v=wire[i];
                if(v.code==102){if(v.text=="}"){if(!depth)return false;--depth;}else if(!v.text.empty()&&v.text.front()=='{')++depth;continue;}
                if(depth)continue;if(v.code==100&&v.text!="AcDbEntity")break;
                if(v.code==5){if(self||std::stoul(v.text,nullptr,16)!=row.self)return false;self=true;++identityFields;}
                if(v.code==330){if(owner||std::stoul(v.text,nullptr,16)!=row.wireOwner)return false;owner=true;++identityFields;}
            }
            if(!self||!owner||(row.endpoint&&row.paper&&(row.space||row.spacePresent)))return false;++origins;}
        ++profiles;
    }
    for(bool binary:{false,true}) {
        const auto good=blockRoleDocument("AC1032");if(!write(path,good,binary))return false;BlockRoleProbe baseline;dx_data base;if(!baseline.fileImport(path.string(),&base,false)||!complete(base))return false;
        stage.name="original source defaults, linked roles and endpoint metadata";
        for(unsigned kind=0u;kind<10u;++kind){auto tags=good;
            if(kind==0u)for(const char* id:{"204","205","207","208"})tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,id,410)));
            if(kind==1u)for(const char* id:{"205","208"})tags[blockField(tags,id,330)].value="B2";
            if(kind==2u)appendBlockFields(tags,"301",{{67,"0"}});
            if(kind==3u)appendBlockFields(tags,"301",{{102,"{OPAQUE"},{67,"1"},{410,"not a layout"},{330,"DEAD"},{102,"}"}});
            if(kind==4u)for(auto& t:tags)if(t.code==410)t.value="sheet 2";
            if(kind==5u)for(const char* id:{"F4","F5"})appendBlockFields(tags,id,{{410,"Sheet 2"}});
            if(kind==6u){const auto at=slot(tags,5,"F6")-1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{0,"ENDSEC"},{0,"SECTION"},{2,"BLOCKS"}});}
            if(kind==7u)tags[blockField(tags,"303",330)].value="B3";
            if(kind==8u)for(auto& t:tags){if((t.code==1||t.code==3)&&t.value=="Sheet 2")t.value="Sheet-é";if(t.code==410)t.value="\\U+0053heet-\\U+00E9";}
            if(kind==9u){const auto at=slot(tags,0,"EOF");tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{0,"SECTION"},{2,"BLOCKS"},{0,"ENDSEC"}});}
            if(!write(path,tags,binary))return false;BlockRoleProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!complete(data)){std::cerr<<"variant="<<kind<<'\n';return false;}
            ++variants;
        }
        stage.name="no scopes versus a layout-free ordinary definition";
        {if(!write(path,layoutRoleDocument("AC1032"),binary))return false;BlockRoleProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;const auto r=dx_iface::blockRoleReport(data);
            if(r.state!=S::NoScopes||r.scopes||!r.physicalCensusComplete||r.selectedRolesMatched)return false;++contexts;}
        {Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1032"},{0,"ENDSEC"},{0,"SECTION"},{2,"TABLES"},{0,"TABLE"},{2,"BLOCK_RECORD"},{5,"8"},{100,"AcDbSymbolTable"},{70,"1"},
                {0,"BLOCK_RECORD"},{5,"B3"},{330,"8"},{100,"AcDbSymbolTableRecord"},{2,"Definition"},{100,"AcDbBlockTableRecord"},{70,"0"},{340,"0"},{0,"ENDTAB"},{0,"ENDSEC"},{0,"SECTION"},{2,"BLOCKS"}};
            const auto first=slot(good,5,"F6")-1u;auto last=slot(good,5,"F7");while(good[last].code!=0)++last;
            tags.insert(tags.end(),good.begin()+static_cast<std::ptrdiff_t>(first),good.begin()+static_cast<std::ptrdiff_t>(last));tags.insert(tags.end(),{{0,"ENDSEC"},{0,"EOF"}});
            if(!write(path,tags,binary))return false;BlockRoleProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;const auto r=dx_iface::blockRoleReport(data);
            if(r.state!=S::RolesReady||r.scopes!=1u||r.records!=5u||r.matched!=5u||!r.physicalCensusComplete||!r.selectedRolesMatched||dx_iface::layoutRoleReport(data).state!=dx_iface::LayoutRoleState::NoLayouts)return false;++variants;}
        stage.name="additional inactive paper scope and duplicate definition";
        for(bool duplicate:{false,true}){auto tags=good;
            if(!duplicate){const auto at=slot(tags,0,"ENDTAB");const Tags block{{0,"BLOCK_RECORD"},{5,"B4"},{330,"8"},{100,"AcDbSymbolTableRecord"},{2,"*Paper_Space5"},{100,"AcDbBlockTableRecord"},{70,"0"},{340,"A3"}};
                tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),block.begin(),block.end());tags[blockField(tags,"8",70)].value="5";appendBlockFields(tags,"43",{{3,"Sheet 3"},{350,"A3"}});
                const auto p=slot(tags,5,"A0")-1u;const Tags layout{{0,"LAYOUT"},{5,"A3"},{330,"43"},{100,"AcDbPlotSettings"},{70,"0"},{76,"0"},{100,"AcDbLayout"},{1,"Sheet 3"},{70,"0"},{71,"3"},{330,"B4"}};
                tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(p),layout.begin(),layout.end());}
            const auto at=slot(tags,0,"EOF")-1u;Tags extra{{0,"BLOCK"},{5,"FA"},{330,duplicate?"B3":"B4"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},{2,duplicate?"Definition":"*Paper_Space5"},{70,"0"},{10,"0"},{20,"0"}};
            if(!duplicate)extra.insert(extra.end(),{{0,"POINT"},{5,"501"},{330,"B4"},{100,"AcDbEntity"},{67,"1"},{410,"Sheet 3"},{8,"0"},{100,"AcDbPoint"},{10,"1"},{20,"2"}});
            extra.insert(extra.end(),{{0,"ENDBLK"},{5,"FB"},{330,duplicate?"B3":"B4"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"}});tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),extra.begin(),extra.end());
            if(!write(path,tags,binary))return false;BlockRoleProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;const auto r=dx_iface::blockRoleReport(data);
            if(duplicate){if(r.state!=S::Unqualified||r.selectedRolesMatched||A::blockRole(data)->scopes[3].reason!=R::DuplicateDefinition||A::blockRole(data)->scopes[4].reason!=R::DuplicateDefinition)return false;++refusals;}
            else{if(r.state!=S::RolesReady||r.scopes!=5u||r.records!=22u||r.qualifiedScopes!=5u||r.paper!=9u||!r.physicalCensusComplete||!r.selectedRolesMatched)return false;++variants;}}
        stage.name="actual target, body, space and layout conflicts";
        for(unsigned kind=0u;kind<10u;++kind){auto tags=good;R expected=R::BodyNotReady;std::uint32_t id=0x201u;
            if(kind==0u){tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,"201",67)));expected=R::SpaceConflict;}
            if(kind==1u){tags[blockField(tags,"201",410)].value="Sheet 1";expected=R::LayoutConflict;}
            if(kind==2u){tags[blockField(tags,"205",410)].value="Model";expected=R::LayoutConflict;id=0x205u;}
            if(kind==3u){tags[blockField(tags,"201",330)].value="B3";}
            if(kind==4u){appendBlockFields(tags,"301",{{410,"Model"}});expected=R::LayoutUnresolved;id=0x301u;}
            if(kind==5u){tags[blockField(tags,"201",410)].value="\\U+D800";expected=R::TextUnqualified;}
            if(kind==6u){appendBlockFields(tags,"301",{{67,"1"}});expected=R::SpaceConflict;id=0x301u;}
            if(kind==7u){appendBlockFields(tags,"201",{{67,"1"}});}
            if(kind==8u){const auto begin=slot(tags,5,"201")-1u;auto end=begin+1u;while(tags[end].code!=0)++end;Tags moved(tags.begin()+static_cast<std::ptrdiff_t>(begin),tags.begin()+static_cast<std::ptrdiff_t>(end));
                for(auto& t:moved)if(t.code==330)t.value="B0";tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(begin),tags.begin()+static_cast<std::ptrdiff_t>(end));const auto at=slot(tags,5,"F1")-1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),moved.begin(),moved.end());expected=R::WrongPlacement;}
            if(kind==9u){for(auto& t:tags)if((t.code==1||t.code==3)&&t.value=="Sheet 2")t.value="Sheet-é";tags[blockField(tags,"201",410)].value="SHEET-É";expected=R::LayoutConflict;}
            if(!write(path,tags,binary))return false;BlockRoleProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||dx_iface::blockRoleReport(data).state!=S::Unqualified){std::cerr<<"refusal="<<kind<<'\n';return false;}
            if(kind!=3u&&kind!=7u){const auto& rows=A::blockRole(data)->rows;const auto at=std::find_if(rows.begin(),rows.end(),[&](const auto& row){return row.self==id;});if(at==rows.end()||at->reason!=expected){std::cerr<<"refusal reason="<<kind<<'\n';return false;}}
            if(!write(path,good,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++refusals;
        }
        stage.name="unsupported or dropped physical scope cannot hide";
        for(bool dropped:{false,true}){auto tags=good;if(dropped){const auto at=slot(tags,0,"EOF")-1u;const Tags extra{{0,"BLOCK"},{5,"F8"},{330,"FFFF"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},{2,""},{70,"0"},{10,"0"},{20,"0"},
                    {0,"POINT"},{5,"401"},{330,"FFFF"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"1"},{20,"2"},{0,"ENDBLK"},{5,"F9"},{330,"FFFF"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"}};tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),extra.begin(),extra.end());}
            else {const auto at=slot(tags,5,"F7")-1u;const Tags extra{{0,"ARC"},{5,"304"},{330,"B3"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbCircle"},{10,"0"},{20,"0"},{40,"1"},{100,"AcDbArc"},{50,"0"},{51,"90"}};tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),extra.begin(),extra.end());}
            if(!write(path,tags,binary))return false;BlockRoleProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;const auto r=dx_iface::blockRoleReport(data);
            if(r.state!=S::Unqualified||!r.physicalCensusComplete||r.selectedRolesMatched||r.scopes!=(dropped?5u:4u)||r.records!=(dropped?22u:20u)||r.unsupported!=(dropped?3u:1u))return false;++contexts;}
        if(!write(path,good,binary))return false;
        stage.name="provenance, quotas, allocation and whole-import guards";
        {BlockRoleProbe reader;reader.copyChildren=true;dx_data data;if(!reader.fileImport(path.string(),&data,false)||dx_iface::blockRoleReport(data).state!=S::Unqualified){std::cerr<<"copied children state="<<static_cast<int>(dx_iface::blockRoleReport(data).state)<<'\n';return false;}++contexts;}
        {BlockRoleProbe reader;reader.selected=false;dx_data data;if(!reader.fileImport(path.string(),&data,false)||dx_iface::blockRoleReport(data).state!=S::Unqualified||!dx_iface::blockRoleReport(data).physicalCensusComplete){std::cerr<<"opt-out state="<<static_cast<int>(dx_iface::blockRoleReport(data).state)<<'\n';return false;}++contexts;}
        for(bool work:{false,true})for(bool exact:{false,true}){const auto r=dx_iface::blockRoleReport(base);dx_data data;A::blockRoleLimit(data,work,(work?r.work:r.bytes)-(exact?0u:1u));BlockRoleProbe reader;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::blockRoleReport(data).state!=(exact?S::RolesReady:S::CaptureFailed))return false;++quotas;}
        for(unsigned kind=0u;kind<4u;++kind)for(bool alias:{false,true}){BlockRoleProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;A::substituteBlockRole(data,kind,alias);
            if(dx_iface::blockRoleReport(data).state!=S::ContextMismatch)return false;++contexts;}
        for(unsigned kind=0u;kind<3u;++kind){BlockRoleProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;
            if(kind==0u)A::substituteLayoutRole(data,false);else if(kind==1u)A::substituteIdentity(data,0u);else{dx_data copied;A::copyBindingContext(copied,data);if(dx_iface::blockRoleReport(copied).state!=S::ContextMismatch)return false;++contexts;continue;}
            if(dx_iface::blockRoleReport(data).state!=S::ContextMismatch)return false;++contexts;}
        {BlockRoleProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;A::transplantBlockRole(data,base);if(dx_iface::blockRoleReport(data).state!=S::ContextMismatch)return false;++contexts;}
        for(auto* block:base.blocks){block->name="edited live definition";for(auto* value:block->ent)value->parentHandle=0x40u;}base.layouts.front().name="edited layout";
        if(!complete(base))return false;{SymbolAllocationFailure noAllocation(0);if(!complete(base))return false;}++contexts;
        bool recovered=false;for(long fault=0;fault<2000;++fault){{SymbolAllocationFailure failure(fault);A::repeatBlockRole(baseline,base);}
            if(complete(base)){recovered=true;break;}if(dx_iface::blockRoleReport(base).state!=S::CaptureFailed)return false;A::repeatBlockRole(baseline,base);if(!complete(base))return false;++allocations;}
        if(!recovered)return false;
        stage.name="actual malformed physical boundaries";
        for(bool selected:{false,true})for(unsigned kind=0u;kind<3u;++kind){auto tags=good;
            if(kind==0u)eraseBlockRecord(tags,"F5");
            if(kind==1u){const auto at=slot(tags,5,"F0")-1u;const Tags orphan{{0,"POINT"},{5,"601"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"1"},{20,"2"}};tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),orphan.begin(),orphan.end());}
            if(kind==2u)tags[slot(tags,5,"F5")-1u].value="BLOCK";
            if(!write(path,tags,binary))return false;BlockRoleProbe reader;reader.selected=selected;dx_data data;const bool read=reader.fileImport(path.string(),&data,false);const auto report=dx_iface::blockRoleReport(data);
            if(read||reader.depth||A::blockRole(data)||report.state!=S::ImportFailed){std::cerr<<"boundary="<<kind<<" read="<<read<<" state="<<static_cast<int>(report.state)<<" census="<<report.physicalCensusComplete<<" depth="<<reader.depth<<'\n';return false;}
            reader.selected=true;if(!write(path,good,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++failures;}
        for(bool callback:{false,true}){auto tags=good;if(!callback)tags.pop_back();if(!write(path,tags,binary))return false;BlockRoleProbe reader;reader.throwPoint=callback;dx_data data;bool read=false,threw=false;
            try{read=reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
            if(read||threw!=callback||reader.depth||dx_iface::blockRoleReport(data).state!=S::ImportFailed)return false;reader.throwPoint=false;if(!write(path,good,binary))return false;
            dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++failures;}
    }
    std::cout<<"BLOCKS roles profiles="<<profiles<<" independently walked origins="<<origins<<" identity fields="<<identityFields<<" variants="<<variants<<" refusals="<<refusals<<" contexts="<<contexts<<" quotas="<<quotas<<" allocation faults="<<allocations<<" tail/callback retries="<<failures<<'\n';stage.done=true;return true;
}
