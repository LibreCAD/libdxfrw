// SPDX-License-Identifier: GPL-2.0-or-later
// Locally authored canonical primary styles and real original owner graphs.
Tags renderingSourceStyle(bool internal) {
    return {{0,"VISUALSTYLE"},{5,"EA01"},{330,"EB00"},{102,"{ACAD_REACTORS"},{330,"705"},{102,"}"},
        {102,"{ACAD_XDICTIONARY"},{360,"EB10"},{102,"}"},{100,"AcDbObject"},{100,"AcDbVisualStyle"},{2,"authored-style"},
        {70,"8"},{71,"3"},{72,"2"},{73,"4"},{90,"3"},{62,"7"},{63,"256"},{421,"1122867"},{74,"2"},{91,"5"},{64,"4"},{65,"5"},
        {75,"2"},{175,"3"},{92,"7"},{66,"6"},{76,"4"},{77,"3"},{78,"2"},{67,"7"},{79,"2"},{170,"1"},{171,"9"},{290,"1"},
        {174,"1"},{93,"7"},{173,"2"},{40,"0.25"},{41,"0.75"},{42,"0.5"},{43,"0.625"},{44,"1.25"},{291,internal?"1":"0"},
        {1001,"REGISTERED_APP"},{1003,"0"},{1005,"705"},{1002,"{"},{1010,"1"},{1020,"2"},{1030,"3"},{1002,"}"}};
}
Tags renderingSourceDocument(const std::string& version,bool blocks,bool internal) {
    auto tags=viewportDocument(version,blocks,1u);
    appendBlockFields(tags,"40",{{3,"ACAD_VISUALSTYLE"},{350,"EB00"}});
    auto view=slot(tags,5,"E701");while(tags[view].code!=0&&tags[view].code!=1001)++view;
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(view),{{348,"EA01"}});
    for(auto at=slot(tags,5,"E801");tags[at].code!=0;++at)if(tags[at].code==348)tags[at].value="EA01";
    dependencyCommon(tags,"701",{{348,"EA01"}});
    layoutDependencySet(tags,"AcDbPlotSettings",76,"4");layoutDependencySet(tags,"AcDbPlotSettings",333,"EA01");
    layoutDependencySet(tags,"AcDbLayout",333,"EA01");
    Tags objects{{0,"DICTIONARY"},{5,"EB00"},{330,"40"},{100,"AcDbDictionary"},{280,"1"},{281,"1"},{3,"AuthoredStyle"},{350,"EA01"},
        {0,"DICTIONARY"},{5,"EB10"},{330,"EA01"},{100,"AcDbDictionary"},{280,"1"},{281,"1"}};
    const auto style=renderingSourceStyle(internal);objects.insert(objects.end(),style.begin(),style.end());
    auto at=slot(tags,2,"OBJECTS")+1u;while(tags[at].code!=0||tags[at].value!="ENDSEC")++at;
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),objects.begin(),objects.end());return tags;
}
void renderingStyleSet(Tags& tags,int code,const std::string& value) {
    auto at=slot(tags,5,"EA01");while(tags[at].code!=0){if(tags[at].code==code){tags[at].value=value;return;}++at;}
}
bool renderingSourceTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::RenderingSourceState;using R=dx_iface::DependencyReason;
    using Role=dx_tableResourceRole;using Field=dx_visualstyle_source::Field;
    const auto path=dir/"rendering-sources.dxf";
    std::size_t profiles=0u,fields=0u,dictionaryFields=0u,references=0u,targets=0u,refusals=0u,retries=0u,quotas=0u,bindings=0u,contexts=0u,immutable=0u,reached=0u,unreached=0u,finalizations=0u,prerequisites=0u,texts=0u;
    const auto fail=[&](int line){std::cerr<<"Rendering source check line="<<line<<" profiles="<<profiles<<" fields="<<fields<<" refusals="<<refusals<<" bindings="<<bindings<<'\n';return false;};
    const auto ready=[](const dx_data& data){const auto source=dx_iface::renderingSourceReport(data);const auto dependency=dx_iface::dependencyReport(data);
        return source.originalSourcesMatched&&source.records==1u&&source.profiles==1u&&source.namedTargets==1u&&source.origins==1u
            &&dependency.originalTargetsMatched&&!source.ownerTargetsResolved&&!source.recursiveClosureMatched&&!source.outputEligible
            &&!dependency.ownerTargetsResolved&&!dependency.recursiveClosureMatched&&!dependency.outputEligible;};
    const auto diagnose=[&](const dx_data& data){const auto source=dx_iface::renderingSourceReport(data);const auto dependency=dx_iface::dependencyReport(data);
        std::cerr<<" rendering="<<static_cast<int>(source.state)<<" profiles="<<source.profiles<<" named="<<source.namedTargets<<" dependency="<<static_cast<int>(dependency.state)
            <<" first="<<std::hex<<dependency.firstSource<<" -> "<<dependency.firstTarget<<std::dec<<'\n';
        const auto tr=dx_iface::tableReferenceReport(data);const auto body=dx_iface::tableBodyReport(data);
        std::cerr<<" table references="<<static_cast<int>(tr.state)<<" body="<<static_cast<int>(body.state)<<" model/profile="<<body.modeledMatched<<'/'<<body.profilesMatched
            <<" structural="<<static_cast<int>(dx_iface::structuralSourceReport(data).state)<<" layout="<<static_cast<int>(dx_iface::layoutSourceReport(data).state)
            <<" entity="<<static_cast<int>(dx_iface::entitiesRoleReport(data).state)<<" blocks="<<static_cast<int>(dx_iface::blockRoleReport(data).state)<<'\n';
        for(const auto& item:A::mutableLayoutAudits(const_cast<dx_data&>(data)))if(item.second)std::cerr<<" layout="<<std::hex<<item.first<<std::dec
            <<" state="<<static_cast<int>(item.second->state)<<" model="<<item.second->modeledMatched<<" profile="<<static_cast<int>(item.second->profile.reason)<<" reject="<<item.second->profile.rejectedGroup<<'\n';
        if(A::renderingSources(data))for(const auto& item:A::renderingSources(data)->records)std::cerr<<" profile="<<static_cast<int>(item.second->profile.reason)<<" rejected="<<item.second->profile.rejected<<'\n';
        if(A::dependencies(data))for(const auto& row:A::dependencies(data)->rows)if(row.reason!=R::Matched&&row.reason!=R::NullOptional&&row.reason!=R::Inherited&&row.reason!=R::Literal&&row.reason!=R::OriginalTableOwner&&row.reason!=R::Inactive)
            std::cerr<<" source="<<std::hex<<row.source<<" target="<<row.target<<std::dec<<" code="<<row.code<<" reason="<<static_cast<int>(row.reason)<<'\n';};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true})for(bool internal:{false,true}){
        const auto tags=renderingSourceDocument(version,blocks,internal);dx_iface reader;dx_data data;
        if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data)){std::cerr<<version<<'/'<<encoding<<'/'<<blocks<<'/'<<internal<<'\n';diagnose(data);return fail(__LINE__);}
        const auto& row=*A::renderingSources(data)->records.at(0xEA01u);const auto& raw=*row.input->typedRaw;
        const auto& physical=A::identityInput(data)->source->records.at(row.physical);const auto wire=attdefWireTags(path,encoding==2u);const auto& marker=wire.at(physical.marker.ordinal-1u);
        if(marker.code||marker.text!="VISUALSTYLE"||marker.begin!=physical.marker.beginOffset||marker.end!=physical.marker.endOffset||row.profile.spans.size()!=raw.groups.size())return fail(__LINE__);
        const auto& audit=*A::renderingSources(data);
        const auto dictionarySpan=[&](const auto& owner,std::size_t keyIndex,std::size_t targetIndex,const char* expectedKey,const char* expectedTarget){
            const auto& original=A::identityInput(data)->source->records.at(owner.physical);
            const auto& start=wire.at(original.marker.ordinal-1u);
            const auto& key=wire.at(original.marker.ordinal+keyIndex);const auto& target=wire.at(original.marker.ordinal+targetIndex);
            if(start.code||start.text!="DICTIONARY"||start.begin!=original.marker.beginOffset||start.end!=original.marker.endOffset
                ||key.code!=3||key.kind!=AttdefWireTag::String||key.text!=expectedKey
                ||target.code!=350||target.kind!=AttdefWireTag::String||target.text!=expectedTarget
                ||key.begin<start.end||key.end<=key.begin||target.begin<key.end||target.end<=target.begin||target.end>original.endMarker.beginOffset
                ||owner.input->raw->groups.at(keyIndex).code()!=3||owner.input->raw->groups.at(targetIndex).code()!=350)return false;
            dictionaryFields+=2u;return true;};
        if(!dictionarySpan(*audit.root,audit.rootKeyGroup,audit.rootTargetGroup,"ACAD_VISUALSTYLE","EB00")
            ||!dictionarySpan(*audit.dictionary,row.nameGroup,row.targetGroup,"AuthoredStyle","EA01"))return fail(__LINE__);
        for(const auto& span:row.profile.spans){const auto& value=raw.groups.at(span.group);const auto& field=wire.at(physical.marker.ordinal+span.group);
            if(field.code!=span.code||value.code()!=span.code||field.begin<marker.end||field.end<=field.begin)return fail(__LINE__);
            if(field.kind==AttdefWireTag::Integer){if(value.type()!=DRW_Variant::INTEGER||value.i_val()!=field.integer)return fail(__LINE__);}
            else if(field.kind==AttdefWireTag::Real){const auto found=value.d_val();if(value.type()!=DRW_Variant::DOUBLE||std::memcmp(&found,&field.real,sizeof(double)))return fail(__LINE__);}
            else if(value.type()!=DRW_Variant::STRING||!value.content.s||*value.content.s!=field.text)return fail(__LINE__);++fields;}
        for(const auto& ref:row.references){const auto& span=row.profile.spans.at(ref.groupIndex);Role expected=Role::Unknown;
            if(ref.code==5)expected=Role::Self;else if(ref.code==330)expected=span.field==Field::Application?Role::Reactor:Role::Owner;
            else if(ref.code==360)expected=Role::ExtensionDictionary;else if(ref.code==1001)expected=Role::AppIdName;else if(ref.code==1003)expected=Role::LayerName;else if(ref.code==1005)expected=Role::XdataObjectId;
            if(ref.kind!=expected||span.code!=ref.code)return fail(__LINE__);++references;}
        for(const auto& ref:A::dependencies(data)->rows)if(ref.target==0xEA01u&&(ref.role==Role::VisualStyle||ref.role==Role::LayoutShadePlot)){
            if(ref.reason!=R::Matched)return fail(__LINE__);++targets;}
        const auto before=symbolFailedAllocations;{SymbolAllocationFailure none(0);if(!ready(data))return fail(__LINE__);}if(before!=symbolFailedAllocations)return fail(__LINE__);
        ++immutable;
        // Test the independent binder for every primary field, without using
        // production DTO code tables or the shared payload decoder.
        for(const auto& span:row.profile.spans)if(span.field==Field::Primary){auto changed=raw;auto& value=changed.groups.at(span.group);
            if(value.type()==DRW_Variant::INTEGER)value=DRW_Variant(value.code(),value.i_val()==0?1:0);
            else if(value.type()==DRW_Variant::DOUBLE)value=DRW_Variant(value.code(),value.d_val()+1.);
            else value=DRW_Variant(value.code(),UTF8STRING("other-description"));
            const auto profile=dx_visualstyle_source::audit(changed);
            if(dx_visualstyle_source::matches(profile,changed,static_cast<const DRW_VisualStyle&>(*row.input->typedModel),row.input->typedVisualProperties,*row.input->context))return fail(__LINE__);++bindings;}
        ++profiles;
    }
    const auto good=renderingSourceDocument("AC1027",false,false);
    for(unsigned encoding=0u;encoding<3u;++encoding){auto escaped=good;renderingStyleSet(escaped,2,"STYLE_TEXT_PLACEHOLDER");
        escaped[slot(escaped,3,"ACAD_VISUALSTYLE")].value="ROOT_KEY_PLACEHOLDER";
        escaped[slot(escaped,3,"AuthoredStyle")].value="STYLE_KEY_PLACEHOLDER";
        if(!writeBlockBinding(path,escaped,encoding))return fail(__LINE__);
        std::string bytes;{std::ifstream input(path,std::ios::binary);bytes.assign(std::istreambuf_iterator<char>(input),{});}
        for(const auto& value:std::array<std::pair<std::string,std::string>,3>{{{"STYLE_TEXT_PLACEHOLDER","LINE^JTEXT"},{"ROOT_KEY_PLACEHOLDER","ACAD_\\U+0056ISUALSTYLE"},{"STYLE_KEY_PLACEHOLDER","STYLE\\U+0058"}}}){
            const auto at=bytes.find(value.first);if(at==std::string::npos||bytes.find(value.first,at+1u)!=std::string::npos)return fail(__LINE__);bytes.replace(at,value.first.size(),value.second);}
        {std::ofstream output(path,std::ios::binary);output.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));}
        dx_iface reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!ready(data))return fail(__LINE__);
        const auto& model=static_cast<const DRW_VisualStyle&>(*A::renderingSources(data)->records.at(0xEA01u)->input->typedModel);
        if(model.desc!=(encoding==2u?"LINE^JTEXT":"LINE\nTEXT"))return fail(__LINE__);++texts;
    }
    for(unsigned encoding=0u;encoding<3u;++encoding)for(unsigned kind=0u;kind<16u;++kind){auto bad=good;
        if(kind==0u)renderingStyleSet(bad,71,"0"); // valid changed source remains valid; no guessed property defaults
        if(kind==1u)bad.erase(bad.begin()+static_cast<std::ptrdiff_t>(slot(bad,5,"EA01")+11u));
        if(kind==2u){auto at=slot(bad,5,"EA01");while(bad[at].code!=1001)++at;bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(at),{{70,"0"}});} // opaque extension, including zero
        if(kind==3u)renderingStyleSet(bad,330,"EB10");
        if(kind==4u)bad[slot(bad,3,"ACAD_VISUALSTYLE")].value="MISSING_STYLES";
        if(kind==5u)bad[slot(bad,5,"EB00")+1u].value="EB10";
        if(kind==6u)bad[slot(bad,3,"AuthoredStyle")+1u].value="705";
        if(kind==7u)renderingStyleSet(bad,1001,"UNREGISTERED_APP");
        if(kind==8u)renderingStyleSet(bad,1003,"MISSING_LAYER");
        if(kind==9u)renderingStyleSet(bad,1005,"FFFF");
        if(kind==10u)renderingStyleSet(bad,360,"705");
        if(kind==11u){const auto at=slot(bad,5,"EA01");bad[at-1u].value="ACDBVISUALSTYLE";}
        if(kind==12u)layoutDependencySet(bad,"AcDbPlotSettings",76,"5");
        if(kind==13u)bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(slot(bad,5,"EA01")+10u),{{999,"authored physical gap"}});
        if(kind==14u)bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(slot(bad,3,"ACAD_VISUALSTYLE")),{{999,"authored root gap"}});
        if(kind==15u)bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(slot(bad,3,"AuthoredStyle")),{{999,"authored dictionary gap"}});
        dx_iface reader;dx_data data;if(!writeBlockBinding(path,bad,encoding)||!reader.fileImport(path.string(),&data,false))return fail(__LINE__);
        if(kind==0u){if(!ready(data))return fail(__LINE__);}else{if(ready(data))return fail(__LINE__);++refusals;}
        if(kind==12u){bool found=false;if(A::dependencies(data))for(const auto& row:A::dependencies(data)->rows)
            if(row.source==0xA0u&&row.role==Role::LayoutShadePlot&&row.reason==R::WrongKind)found=true;
            if(!found)return fail(__LINE__);}
        if(kind==13u){const auto& record=*A::renderingSources(data)->records.at(0xEA01u);
            if(!dx_iface::renderingModelReport(data).modeledStateMatched||record.profile.reason!=dx_visualstyle_source::Reason::PhysicalEnvelope)return fail(__LINE__);}
        if(kind>=14u){const auto source=dx_iface::renderingSourceReport(data);
            if(!dx_iface::renderingModelReport(data).modeledStateMatched||!source.originalSourcesMatched||source.namedTargets)return fail(__LINE__);}
        dx_data fixed;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&fixed,false)||!ready(fixed))return fail(__LINE__);++retries;
    }
    for(unsigned encoding=0u;encoding<3u;++encoding){dx_iface reader;dx_data data;
        if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data))return fail(__LINE__);
        const auto source=dx_iface::renderingSourceReport(data);
        for(bool work:{false,true}){const auto exact=work?source.work:source.bytes;A::renderingSourceLimit(data,work,exact);A::repeatRenderingSources(reader,data);A::joinRenderingSources(reader,data);
            if(!ready(data))return fail(__LINE__);A::renderingSourceLimit(data,work,exact-1u);A::repeatRenderingSources(reader,data);
            if(dx_iface::renderingSourceReport(data).state!=S::Limited||dx_iface::tableReferenceReport(data).state!=dx_iface::TableReferenceState::ContextMismatch)return fail(__LINE__);
            A::renderingSourceLimit(data,work,work?128u*1024u*1024u:64u*1024u*1024u);A::repeatRenderingSources(reader,data);A::joinRenderingSources(reader,data);
            if(!ready(data))return fail(__LINE__);++quotas;++retries;}
        for(unsigned which:{1u,2u}){A::unavailableRenderingReplay(data,which);
            if(dx_iface::renderingSourceReport(data).state!=S::SourceNotReady||dx_iface::dependencyReport(data).originalTargetsMatched)return fail(__LINE__);
            A::repeatRenderingSources(reader,data);if(dx_iface::renderingSourceReport(data).state!=S::SourceNotReady)return fail(__LINE__);++prerequisites;
            A::unavailableRenderingReplay(data,0u);A::repeatRenderingSources(reader,data);A::joinRenderingSources(reader,data);if(!ready(data))return fail(__LINE__);++retries;}
        const auto saved=A::renderingSources(data);using Ptr=std::decay_t<decltype(saved)>;
        for(bool alias:{false,true}){
            A::mutableRenderingSources(data)=alias?Ptr(saved.get(),[](const auto*){}):std::make_shared<const std::remove_const_t<typename Ptr::element_type>>(*saved);
            if(dx_iface::renderingSourceReport(data).state!=S::ContextMismatch||dx_iface::dependencyReport(data).originalTargetsMatched)return fail(__LINE__);++contexts;
            A::mutableRenderingSources(data)=saved;if(!ready(data))return fail(__LINE__);++retries;}
        dx_data other;if(!reader.fileImport(path.string(),&other,false)||!ready(other))return fail(__LINE__);
        A::mutableRenderingSources(data)=A::renderingSources(other);
        if(dx_iface::renderingSourceReport(data).state!=S::ContextMismatch||dx_iface::dependencyReport(data).originalTargetsMatched)return fail(__LINE__);++contexts;
        A::mutableRenderingSources(data)=saved;if(!ready(data))return fail(__LINE__);++retries;
        for(auto& value:data.visualStyles){value.desc="edited live style";value.parentHandle=0xFFFFu;value.m_body.edgeWidth+=99;}
        data.rawVisualStyleObjects.clear();A::joinRenderingSources(reader,data);if(!ready(data))return fail(__LINE__);
    }
    for(unsigned encoding:{0u,2u}){dx_iface reader;dx_data data;auto large=good;
        renderingStyleSet(large,2,std::string(192u,'D'));large[slot(large,3,"AuthoredStyle")].value=std::string(192u,'K');
        if(!writeBlockBinding(path,large,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data))return fail(__LINE__);
        const auto queryBefore=symbolFailedAllocations;{SymbolAllocationFailure none(0);if(!ready(data))return fail(__LINE__);}if(queryBefore!=symbolFailedAllocations)return fail(__LINE__);++immutable;
        unsigned untouched=0u;for(long offset=0;offset<256;++offset){const auto before=symbolFailedAllocations;
            {SymbolAllocationFailure fault(offset);A::attemptRenderingSources(reader,data);}
            if(symbolFailedAllocations!=before){if(dx_iface::renderingSourceReport(data).state!=S::CaptureFailed||dx_iface::dependencyReport(data).originalTargetsMatched)return fail(__LINE__);++reached;untouched=0u;}
            else{if(!dx_iface::renderingSourceReport(data).originalSourcesMatched||dx_iface::dependencyReport(data).originalTargetsMatched)return fail(__LINE__);++unreached;++untouched;}
            A::repeatRenderingSources(reader,data);A::joinRenderingSources(reader,data);if(!ready(data))return fail(__LINE__);++finalizations;
            if(untouched==2u)break;if(offset==255)return fail(__LINE__);}
    }
    std::cout<<"Rendering source profiles="<<profiles<<"; independent physical fields="<<fields<<"; dictionary key/target fields="<<dictionaryFields<<"; reference roles="<<references<<"; rooted target uses="<<targets
        <<"; primary binder refusals="<<bindings<<"; source/owner/dependency refusals="<<refusals<<"; corrected retries="<<retries<<"; exact/one-under limits="<<quotas
        <<"; context refusals="<<contexts<<"; prerequisite refusals="<<prerequisites<<"; literal text controls="<<texts<<"; immutable queries="<<immutable<<"; reached allocations="<<reached<<"; unreached offsets="<<unreached<<"; corrected finalizations="<<finalizations<<'\n';
    return profiles==48u&&fields==2496u&&dictionaryFields==192u&&references==profiles*7u&&targets==profiles*5u&&bindings==profiles*34u&&refusals==45u&&retries==69u&&quotas==6u&&contexts==9u&&prerequisites==6u&&texts==3u&&immutable==50u&&reached&&unreached==4u&&finalizations==reached+unreached;
}
