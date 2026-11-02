// SPDX-License-Identifier: GPL-2.0-or-later
// Authored VIEWPORT bodies and original target graphs; no drawing fixtures.
void viewportLayoutSet(Tags& tags,const char* id,int code,const std::string& text) {
    auto at=slot(tags,5,id);while(tags[at].code!=100||tags[at].value!="AcDbLayout")++at;++at;
    while(tags[at].code!=0&&tags[at].code!=1001&&tags[at].code!=code)++at;
    if(tags[at].code==code)tags[at].value=text;else tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{code,text}});
}
void viewportBodyAppend(Tags& tags,const Tags& extra) {
    auto at=slot(tags,5,"E801");while(tags[at].code!=0&&tags[at].code!=1001)++at;
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),extra.begin(),extra.end());
}
Tags viewportDocument(const std::string& version,bool blocks,unsigned form) {
    auto tags=viewUcsDocument(version,blocks,48u,false);
    Tags body{{0,"VIEWPORT"},{5,"E801"},{330,blocks?"B2":"B1"},{100,"AcDbEntity"},{67,"1"},{410,blocks?"Sheet 2":"Sheet 1"},{8,"0"},{6,"CONTINUOUS"},
        {100,"AcDbViewport"},{10,"1"},{20,"2"},{30,"3"},{40,"20"},{41,"10"},{68,"1"},{69,"2"},{90,"32768"},{170,"0"},{331,"D301"}};
    if(form)body.insert(body.end(),{{12,"1"},{22,"2"},{13,"3"},{23,"4"},{14,"5"},{24,"6"},{15,"7"},{25,"8"},{16,"0"},{26,"0"},{36,"1"},{17,"9"},{27,"10"},{37,"11"},
        {42,"50"},{43,"1.25"},{44,"100"},{45,"15"},{50,"20"},{51,"30"},{72,"125"},{61,"5"},{1,"authored.ctb"},{281,"2"},{71,"1"},{74,"0"},
        {110,"1"},{120,"2"},{130,"3"},{111,"1"},{121,"0"},{131,"0"},{112,"0"},{122,"1"},{132,"0"},{79,"1"},{146,"4.5"},
        {332,"0"},{333,"0"},{348,"0"},{361,"0"},{292,"1"},{282,"1"},{141,"0.25"},{142,"0.75"},{63,"4"},{421,"1193046"},{431,"book$color"},{331,"D301"}});
    if(form==1u)body.insert(body.end(),{{345,"E711"},{346,"E712"}});
    if(form==2u)for(auto& value:body)if(value.code==292)value.value="0";
    body.insert(body.end(),{{1001,"REGISTERED_APP"},{1003,"0"},{1005,"705"},{1010,"1"},{1020,"2"},{1030,"3"},{1070,"7"}});
    const auto at=blocks?slot(tags,5,"F5")-1u:slot(tags,2,"ENTITIES")+1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),body.begin(),body.end());
    viewportLayoutSet(tags,blocks?"A2":"A1",331,"E801");return tags;
}
struct ViewportProbe:dx_iface {
    bool good=true,blocks=false,throwing=false;unsigned callbacks=0u,pending=0u;int fault=0,mutation=-1;
    void addViewport(const DRW_Viewport& value) override {
        dxfRW::DxfViewportSourceView view;const auto* input=cData?DxIfaceTableSourceTestAccess::readInput(*cData):nullptr;
        const auto before=symbolFailedAllocations;bool present=false;
        {SymbolAllocationFailure none(0);present=input&&input->getDxfViewportSourceView(view);}
        if(!present||view.model!=&value||view.inBlock!=blocks||symbolFailedAllocations!=before)good=false;
        if(view.originalPendingXdata||view.originalCoordinateMask)++pending;++callbacks;
        if(fault==1){const DRW_Viewport copy(value);dx_iface::addViewport(copy);return;}if(fault==2)return;
        auto& actual=const_cast<DRW_Viewport&>(value);const DRW_Viewport saved(value);
        if(mutation==0)actual.pswidth+=1.;if(mutation==1)actual.statusFlags^=1;if(mutation==2)actual.namedUcsHandle=0xFFFFu;
        if(mutation==3)actual.frozenLayerHandles.front()=0xFFFFu;if(mutation==4)actual.styleSheet="edited.ctb";if(mutation==5)actual.ambientColorName="edited";
        if(mutation==6)actual.brightness+=1.;if(mutation==7)actual.viewDir.y+=1.;if(mutation==8)actual.vpHeaderHandle=0xFFFFu;
        if(mutation==9)actual.ambientColorDwgIndex=7u;if(mutation==10)actual.fullVisualStyleHandle=0xFFFFu;if(mutation==11)actual.basePoint.z+=1.;
        dx_iface::addViewport(value);actual=saved;if(fault==3)dx_iface::addViewport(value);
        if(throwing)throw std::runtime_error("viewport typed callback");
    }
};
bool viewportSourceTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;const auto path=dir/"viewport-sources.dxf";
    unsigned profiles=0u,markers=0u,spans=0u,references=0u,refusals=0u,mutations=0u,quotas=0u,imports=0u,immutable=0u,pending=0u,reached=0u,unreached=0u,finalizations=0u,limits=0u,throws=0u,lifetimes=0u;
    const auto fail=[&](int line){std::cerr<<"VIEWPORT check line="<<line<<" profiles="<<profiles<<" refusals="<<refusals<<" mutations="<<mutations<<" quotas="<<quotas<<'\n';return false;};
    const auto ready=[](const dx_data& data){const auto r=dx_iface::dependencyReport(data);if(!r.originalTargetsMatched||!r.nameNamespacesComplete||r.ownerTargetsResolved||r.recursiveClosureMatched||r.outputEligible||!A::dependencies(data))return false;
        const auto found=A::dependencies(data)->nodes.find(0xE801u);return found!=A::dependencies(data)->nodes.end()&&found->second.entity;};
    const auto diagnose=[&](const dx_data& d){const auto r=dx_iface::dependencyReport(d);std::cerr<<"state="<<static_cast<int>(r.state)<<" entities="<<r.backedEntityRows<<'/'<<r.physicalEntityRows<<" first="<<std::hex<<r.firstSource<<" -> "<<r.firstTarget<<std::dec<<'\n';
        for(const auto& audit:A::entitiesAudits(d))for(const auto& proof:audit->proofs)if(audit->members.at(proof.first).self==0xE801u){std::cerr<<" proof="<<static_cast<int>(proof.state)<<" model="<<proof.modeledMatched<<" profile="<<proof.profilesMatched<<'\n';
            for(const auto& p:proof.profiles){std::cerr<<" profile reason="<<static_cast<int>(p.reason)<<" group="<<p.rejectedGroup;
                const auto& groups=audit->physical->records.at(proof.first).groups;if(p.rejectedGroup<groups.size()){const auto& g=groups[p.rejectedGroup];std::cerr<<" code="<<g.code()<<" type="<<g.type();if(g.type()==DRW_Variant::INTEGER)std::cerr<<" integer="<<g.i_val();}std::cerr<<'\n';}}
        if(A::dependencies(d))for(const auto& row:A::dependencies(d)->rows)if(row.source==0xE801u)std::cerr<<" code="<<row.code<<" role="<<static_cast<int>(row.role)<<" reason="<<static_cast<int>(row.reason)<<'\n';};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true})for(unsigned form=0u;form<3u;++form){
        ViewportProbe reader;reader.blocks=blocks;dx_data data;const auto tags=viewportDocument(version,blocks,form);
        if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!reader.good||reader.callbacks!=1u||!ready(data)){std::cerr<<version<<'/'<<encoding<<" blocks="<<blocks<<" form="<<form<<'\n';diagnose(data);return fail(__LINE__);}
        const auto& node=A::dependencies(data)->nodes.at(0xE801u);const auto& original=A::identityInput(data)->source->records.at(node.physical);const auto wire=attdefWireTags(path,encoding==2u);const auto& marker=wire.at(original.marker.ordinal-1u);
        if(marker.code||marker.text!="VIEWPORT"||marker.begin!=original.marker.beginOffset||marker.end!=original.marker.endOffset)return fail(__LINE__);++markers;
        const auto& groups=A::dependencyGroups(node);for(std::size_t i=0u;i<groups.size();++i){const auto& value=groups[i];const auto& field=wire.at(original.marker.ordinal+i);if(field.code!=value.code())return fail(__LINE__);
            if(field.kind==AttdefWireTag::Integer){if(value.type()!=DRW_Variant::INTEGER||value.i_val()!=field.integer)return fail(__LINE__);}
            else if(field.kind==AttdefWireTag::Real){const auto real=value.d_val();if(value.type()!=DRW_Variant::DOUBLE||std::memcmp(&real,&field.real,sizeof(real)))return fail(__LINE__);}
            else if(value.type()!=DRW_Variant::STRING||!value.content.s||*value.content.s!=field.text)return fail(__LINE__);++spans;}
        unsigned layers=0u;for(const auto& row:A::dependencies(data)->rows)if(row.source==0xE801u&&row.present){using R=dx_tableResourceRole;R expected=R::Unknown;
            switch(row.code){case 5:expected=R::Self;break;case 330:expected=R::Owner;break;case 8:case 1003:expected=R::LayerName;break;case 6:expected=R::LineTypeName;break;
            case 410:expected=R::LayoutName;break;case 331:expected=R::ViewportFrozenLayer;break;case 345:expected=R::LayoutNamedUcs;break;case 346:expected=R::LayoutBaseUcs;break;
            case 332:expected=R::ViewBackground;break;case 333:expected=R::ViewportShade;break;case 348:expected=R::VisualStyle;break;case 361:expected=R::ViewSun;break;
            case 1001:expected=R::AppIdName;break;case 1005:expected=R::XdataObjectId;break;default:return fail(__LINE__);}
            const auto& field=wire.at(original.marker.ordinal+row.group);if(row.role!=expected||field.code!=row.code)return fail(__LINE__);
            if(row.code==331){if(row.target!=0xD301u)return fail(__LINE__);++layers;}
            if(row.role==R::ViewportShade){const auto& mode=wire.at(original.marker.ordinal+row.modeGroup);if(mode.code!=170||mode.integer!=row.mode)return fail(__LINE__);}
            if((row.code>=320&&row.code<=369)||row.code==5||row.code==1005){std::uint32_t target=0u;const auto parsed=std::from_chars(field.text.data(),field.text.data()+field.text.size(),target,16);
                if(parsed.ec!=std::errc{}||parsed.ptr!=field.text.data()+field.text.size()||target!=row.target)return fail(__LINE__);}++references;}
        if(layers!=(form?2u:1u))return fail(__LINE__);const auto before=symbolFailedAllocations;{SymbolAllocationFailure none(0);if(!ready(data))return fail(__LINE__);}if(symbolFailedAllocations!=before)return fail(__LINE__);
        for(auto* entity:data.mBlock->ent)if(auto* v=dynamic_cast<DRW_Viewport*>(entity)){v->pswidth+=12.;v->namedUcsHandle=0xFFFFu;v->frozenLayerHandles.clear();}
        A::clearAdvisoryNames(data);if(!ready(data))return fail(__LINE__);++immutable;++profiles;
    }
    for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true}){
        const auto good=viewportDocument("AC1032",blocks,1u);const auto retry=[&](ViewportProbe& reader){reader.fault=0;reader.mutation=-1;reader.throwing=false;dx_data next;
            if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&next,false)||!ready(next))return fail(__LINE__);++imports;return true;};
        for(int mutation=0;mutation<12;++mutation){ViewportProbe reader;reader.blocks=blocks;reader.mutation=mutation;dx_data data;
            if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&data,false)||ready(data)||!retry(reader))return fail(__LINE__);++mutations;}
        for(unsigned fault=0u;fault<20u;++fault){auto tags=good;ViewportProbe reader;reader.blocks=blocks;bool parseFailure=false;
            if(fault<3u)reader.fault=static_cast<int>(fault+1u);
            else if(fault==3u)tags[blockField(tags,"E801",331)].value="FFFF";
            else if(fault==4u)tags[blockField(tags,"E801",331)].value="40";
            else if(fault==5u)tags[blockField(tags,"E801",345)].value="40";
            else if(fault==6u)tags[blockField(tags,"E801",345)].value="FFFF";
            else if(fault==7u)viewportBodyAppend(tags,{{99,"7"}});
            else if(fault==8u)tags[blockField(tags,"E801",79)].value="0";
            else if(fault==9u)tags[blockField(tags,"E801",333)].value="40";
            else if(fault==10u)viewportLayoutSet(tags,blocks?"A1":"A2",331,"E801");
            else if(fault==11u)tags[blockField(tags,"E801",67)].value="0";
            else if(fault==12u)appendBlockFields(tags,"E801",{{102,"{OPAQUE"},{330,"DEAD"},{102,"}"}});
            else if(fault==13u){appendBlockFields(tags,"E801",{{1001,"REGISTERED_APP"},{1010,"1"}});}
            else if(fault==14u){tags[blockField(tags,"E801",90)].value="98304";viewportBodyAppend(tags,{{340,"701"}});}
            else if(fault==15u)viewportBodyAppend(tags,{{335,"E801"}});
            else if(fault==16u){tags[blockField(tags,"E801",170)].value="4";}
            else if(fault==17u){for(unsigned i=0u;i<511u;++i)viewportBodyAppend(tags,{{331,"D301"}});parseFailure=true;}
            else if(fault==18u)tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,"E801",30)));
            else tags[blockField(tags,"E801",292)].value="2";
            dx_data data;if(!writeBlockBinding(path,tags,encoding))return fail(__LINE__);const bool parsed=reader.fileImport(path.string(),&data,false);
            if(parsed==parseFailure||ready(data)){std::cerr<<"fault="<<fault<<" parsed="<<parsed<<'\n';diagnose(data);return fail(__LINE__);}if(fault==13u){if(!reader.pending)return fail(__LINE__);++pending;}
            if(!retry(reader))return fail(__LINE__);++refusals;}
        {auto tags=good;for(unsigned i=0u;i<510u;++i)viewportBodyAppend(tags,{{331,"D301"}});ViewportProbe reader;reader.blocks=blocks;dx_data data;
            if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data)||!retry(reader))return fail(__LINE__);++limits;}
        {ViewportProbe reader;reader.blocks=blocks;reader.throwing=true;dx_data data;bool caught=false;
            try{if(reader.fileImport(path.string(),&data,false))return fail(__LINE__);}catch(const std::runtime_error&){caught=true;}
            if(!caught||dx_iface::dependencyReport(data).state!=dx_iface::DependencyState::ImportFailed||!retry(reader))return fail(__LINE__);++throws;}
        ViewportProbe reader;reader.blocks=blocks;dx_data base;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&base,false)||!ready(base))return fail(__LINE__);
        const auto use=A::attdefBodyUsage(base,blocks);for(unsigned which=0u;which<3u;++which)for(bool under:{false,true}){ViewportProbe codec;codec.blocks=blocks;dx_data data;A::attdefBodyLimit(data,blocks,which,use[which]-(under?1u:0u));
            if(!codec.fileImport(path.string(),&data,false)||ready(data)==under||!retry(codec))return fail(__LINE__);++quotas;}
        for(long offset=0;offset<64;++offset){const auto before=symbolFailedAllocations;{SymbolAllocationFailure fault(offset);A::repeatDependencies(reader,base);}
            if(symbolFailedAllocations!=before){if(dx_iface::dependencyReport(base).state!=dx_iface::DependencyState::CaptureFailed)return fail(__LINE__);++reached;}else{if(!ready(base))return fail(__LINE__);++unreached;}
            A::repeatDependencies(reader,base);if(!ready(base))return fail(__LINE__);++finalizations;}
    }
    for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true})for(bool throwing:{false,true}){
        Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1032"},{0,"ENDSEC"},{0,"SECTION"},{2,blocks?"BLOCKS":"ENTITIES"}};
        if(blocks)tags.insert(tags.end(),{{0,"BLOCK"},{5,"E800"},{330,"B2"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},{2,"Publication"},{70,"0"},{10,"0"},{20,"0"},{30,"0"}});
        tags.insert(tags.end(),{{0,"VIEWPORT"},{5,"E801"},{330,"B2"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbViewport"},{10,"1"},{20,"2"},{30,"3"},{40,"4"},{41,"5"},{68,"1"},{69,"2"}});
        if(blocks)tags.insert(tags.end(),{{0,"ENDBLK"},{5,"E802"},{330,"B2"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"}});tags.insert(tags.end(),{{0,"ENDSEC"},{0,"EOF"}});
        struct Public:dx_iface {dxfRW* input=nullptr;bool good=true,blocks=false,throwing=false;unsigned callbacks=0u;
            void addHeader(const DRW_Header*) override{}void addBlock(const DRW_Block&) override{}void endBlock() override{}
            void addViewport(const DRW_Viewport& v) override{dxfRW::DxfViewportSourceView view;dxfRW::DxfTableSourceView table;dxfRW::DxfViewUcsSourceView symbol;
                const auto before=symbolFailedAllocations;{SymbolAllocationFailure none(0);if(!input->getDxfViewportSourceView(view)||view.model!=&v||view.inBlock!=blocks||input->getDxfTableSourceView(table)||input->getDxfViewUcsSourceView(symbol))good=false;}
                if(symbolFailedAllocations!=before)good=false;++callbacks;if(throwing)throw std::runtime_error("public viewport");}
        } consumer;
        if(!writeBlockBinding(path,tags,encoding))return fail(__LINE__);dxfRW input(path.string().c_str());consumer.input=&input;consumer.blocks=blocks;consumer.throwing=throwing;
        dxfRW::DxfViewportSourceView view;DRW_Viewport sentinel;view.model=&sentinel;if(input.getDxfViewportSourceView(view)||view.model)return fail(__LINE__);bool caught=false;
        try{if(!input.read(&consumer,false))return fail(__LINE__);}catch(const std::runtime_error&){caught=true;}
        view.model=&sentinel;if(caught!=throwing||!consumer.good||consumer.callbacks!=1u||input.getDxfViewportSourceView(view)||view.model)return fail(__LINE__);++lifetimes;
    }
    std::cout<<"VIEWPORT original profiles="<<profiles<<"; original markers="<<markers<<"; scalar spans="<<spans<<"; immutable queries="<<immutable<<"; modeled field refusals="<<mutations
        <<"; callback/field/owner refusals="<<refusals<<"; pending original states="<<pending<<"; exact/one-under limits="<<quotas<<"; corrected imports="<<imports
        <<"; reached dependency failures="<<reached<<"; unreached offsets="<<unreached<<"; corrected finalizations="<<finalizations<<"; reference spans="<<references
        <<"; frozen-layer cap controls="<<limits<<"; callback throw/retries="<<throws<<"; public lifetimes="<<lifetimes<<'\n';
    return profiles==72u&&markers==72u&&spans==4200u&&references==864u&&immutable==72u&&mutations==72u&&refusals==120u&&pending==6u&&quotas==36u&&imports==240u&&reached>0u&&reached+unreached==384u&&finalizations==384u&&limits==6u&&throws==6u&&lifetimes==12u;
}
