// SPDX-License-Identifier: GPL-2.0-or-later
// Independent literal preset bodies; no external drawing data or conversions.
namespace renderPresetControls {
using A=DxIfaceTableSourceTestAccess;using Shape=dx_rendersettings_source::Shape;using Reason=dx_iface::DependencyReason;
const char* name(unsigned form){return form<2u?"RENDERSETTINGS":form<5u?"RAPIDRTRENDERSETTINGS":"MENTALRAYRENDERSETTINGS";}
const char* root(unsigned form){return form<2u?"ACAD_RENDER_SETTINGS":form<5u?"ACAD_RENDER_RAPIDRT_SETTINGS":form==5u?"ACAD_RENDER_PLOT_SETTINGS":"ACAD_RENDER_SETTINGS";}
Tags record(unsigned form) {
    Tags tags{{0,name(form)},{5,"EA02"},{330,"EE00"},{102,"{ACAD_REACTORS"},{330,"705"},{102,"}"},
        {102,"{ACAD_XDICTIONARY"},{360,"EE10"},{102,"}"},{100,"AcDbObject"},{100,"AcDbRenderSettings"},{90,form==5u?"1":"2"},{1,"AuthoredPreset"}};
    if(form==5u)tags.insert(tags.end(),{{290,"1"},{90,"7"},{290,"0"},{290,"1"},{1,"authored-preview.png"}});
    else {tags.insert(tags.end(),{{290,"1"},{290,"0"},{290,"1"},{290,"0"},{1,"authored-preview.png"},{1,"authored render description"},{90,"19"}});
        if(form==1u||form==3u||form==7u||form==9u)tags.push_back({290,"1"});}
    if(form>=2u&&form<5u){tags.insert(tags.end(),{{100,"AcDbRapidRTRenderSettings"},{90,"2"},{70,"1"},{90,"13"},{90,"17"},{70,"2"},{70,"3"},{40,"1.375"},{40,"2.875"}});
        if(form==4u)tags.push_back({290,"0"});}
    if(form>=5u){tags.insert(tags.end(),{{100,"AcDbMentalRayRenderSettings"},{90,form>=8u?"2":"1"},{90,"-2"},{90,"3"},{70,"2"},
        {40,"1.375"},{40,"2.875"},{40,"0.05"},{40,"0.15"},{40,"0.25"},{40,"0.35"},{70,"2"},{290,"0"},{290,"1"},
        {90,"2"},{90,"3"},{90,"4"},{290,"1"},{90,"113"},{290,"1"},{40,"1.625"},{90,"227"},{90,"5"},{90,"6"},{90,"7"},
        {290,"1"},{90,"337"},{290,"1"},{290,"0"},{290,"1"},{40,"2.625"},{40,"3.625"},{40,"4.625"},{70,"1"},{70,"2"},
        {40,"5.625"},{70,"1"},{70,"0"},{290,"1"},{1,"authored-stats.mi"},{90,"64"},{70,"3"},{90,"768"}});
        if(form>=8u)tags.insert(tags.end(),{{290,"1"},{40,"8.125"}});}
    tags.insert(tags.end(),{{1001,"REGISTERED_APP"},{1003,"0"},{1005,"705"},{1002,"{"},{1010,"4"},{1020,"5"},{1030,"6"},{1002,"}"}});return tags;
}
Tags document(const std::string& version,bool blocks,unsigned form,bool commented) {
    auto tags=renderingSourceDocument(version,blocks,false);appendBlockFields(tags,"40",{{3,root(form)},{350,"EE00"}});
    layoutDependencySet(tags,"AcDbPlotSettings",76,"5");layoutDependencySet(tags,"AcDbPlotSettings",333,"EA02");layoutDependencySet(tags,"AcDbLayout",333,"EA02");
    Tags objects{{0,"DICTIONARY"},{5,"EE00"},{330,"40"},{100,"AcDbDictionary"},{280,"1"},{281,"1"},{3,"AuthoredPreset"},{350,"EA02"},
        {0,"DICTIONARY"},{5,"EE10"},{330,"EA02"},{100,"AcDbDictionary"},{280,"1"},{281,"1"}};
    const auto preset=record(form);objects.insert(objects.end(),preset.begin(),preset.end());
    auto at=slot(tags,2,"OBJECTS")+1u;while(tags[at].code!=0||tags[at].value!="ENDSEC")++at;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),objects.begin(),objects.end());
    if(commented)for(const char* handle:{"40","EE00","EA02"}){const auto begin=slot(tags,5,handle);auto end=begin;while(tags[end].code!=0)++end;
        while(end>begin)tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(--end),{{999,"authored preset physical comment"}});}
    return tags;
}
bool ready(const dx_data& data){const auto source=dx_iface::renderingSourceReport(data);const auto graph=dx_iface::dependencyReport(data);
    return source.originalSourcesMatched&&source.records==2u&&source.namedTargets==2u&&graph.originalTargetsMatched
        &&!source.ownerTargetsResolved&&!source.recursiveClosureMatched&&!source.outputEligible&&!graph.outputEligible;}
Tags exportDocument(unsigned form) {
    Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1027"},{0,"ENDSEC"},{0,"SECTION"},{2,"OBJECTS"},
        {0,"DICTIONARY"},{5,"40"},{330,"0"},{100,"AcDbDictionary"},{280,"1"},{281,"1"},{3,root(form)},{350,"EE00"},
        {3,"ACAD_PLOTSETTINGS"},{350,"EF00"},{0,"DICTIONARY"},{5,"EE00"},{330,"40"},{100,"AcDbDictionary"},{280,"1"},{281,"1"},{3,"AuthoredPreset"},{350,"EA02"},
        {0,"DICTIONARY"},{5,"EF00"},{330,"40"},{100,"AcDbDictionary"},{280,"1"},{281,"1"},{3,"AuthoredPage"},{350,"EF01"},
        {0,"DICTIONARY"},{5,"EE10"},{330,"EA02"},{100,"AcDbDictionary"},{280,"1"},{281,"1"}};
    auto preset=record(form); // This self-contained export recipe has no entity/application dependencies.
    auto x=slot(preset,1001,"REGISTERED_APP");preset.erase(preset.begin()+static_cast<std::ptrdiff_t>(x),preset.end());
    auto reactor=slot(preset,102,"{ACAD_REACTORS");preset.erase(preset.begin()+static_cast<std::ptrdiff_t>(reactor),preset.begin()+static_cast<std::ptrdiff_t>(reactor+3u));
    tags.insert(tags.end(),preset.begin(),preset.end());
    tags.insert(tags.end(),{{0,"PLOTSETTINGS"},{5,"EF01"},{330,"EF00"},{100,"AcDbObject"},{100,"AcDbPlotSettings"},{1,"AuthoredPage"},
        {2,"DWG To PDF.pc3"},{4,"ISO_A4"},{76,"5"},{333,"EA02"},{0,"ENDSEC"},{0,"EOF"}});return tags;
}
struct TypedWriter:dx_iface {
    dx_data data;dxfRW& output;DRW_RenderSettings value;bool wrote=false;
    TypedWriter(dxfRW& target,const DRW_RenderSettings& v):output(target),value(v){cData=&data;currentBlock=data.mBlock;}
    void writeHeader(DRW_Header& header)override{header.vars.clear();}
    void writeBlocks()override{}void writeBlockRecords()override{}void writeEntities()override{}
    void writeLTypes()override{}void writeLayers()override{}void writeTextstyles()override{}
    void writeVports()override{}void writeDimstyles()override{}void writeAppId()override{}
    void writeObjects()override{wrote=output.writeRenderSettings(&value,value.m_kind==DRW_RenderSettings::Settings?"RENDERSETTINGS":
        value.m_kind==DRW_RenderSettings::RapidRT?"RAPIDRTRENDERSETTINGS":"MENTALRAYRENDERSETTINGS");}
};
struct TypedReader:dx_iface {
    dx_data data;std::vector<DRW_RenderSettings> values;
    TypedReader(){cData=&data;currentBlock=data.mBlock;}
    void addRenderSettings(const DRW_RenderSettings& v)override{values.push_back(v);}
};
bool tests(const std::filesystem::path& dir) {
    const auto path=dir/"preset-input.dxf",output=dir/"preset-output.dxf";
    std::size_t profiles=0u,fields=0u,roles=0u,binders=0u,uses=0u,comments=0u,refusals=0u,retries=0u,exports=0u,typed=0u,rollback=0u,queries=0u,limits=0u,reached=0u,unreached=0u,finalizations=0u;
    const auto fail=[&](int line){std::cerr<<"Preset check line="<<line<<" profiles="<<profiles<<" exports="<<exports<<" refusals="<<refusals<<'\n';return false;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true})for(unsigned form=0u;form<10u;++form){
        const bool commented=(form%2u)==static_cast<unsigned>(blocks);const auto tags=document(version,blocks,form,commented);dx_iface reader;dx_data data;
        if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data))return fail(__LINE__);
        const auto& row=*A::renderingSources(data)->records.at(0xEA02u);const auto& raw=*row.input->typedRaw;
        const auto wire=attdefWireTags(path,encoding==2u);const auto& physical=*row.input->physical;
        if(!row.preset.complete()||!row.presetRoot||row.presetRoot->key!=root(form)||row.physicalFields.size()!=raw.groups.size())return fail(__LINE__);
        const auto shape=form<2u?Shape::Base:form<5u?Shape::RapidRT:form==5u?Shape::LegacyMentalRay:form<8u?Shape::ModernMentalRay:Shape::ExtendedMentalRay;
        if(row.preset.shape!=shape||row.preset.predefined!=(form==1u||form==3u||form==4u||form==7u||form==9u))return fail(__LINE__);
        for(const auto& span:row.preset.spans){const auto& observed=physical.fields.at(row.physicalFields.at(span.group));const auto& field=wire.at(observed.span.ordinal-1u);
            if(field.code!=span.code||field.begin!=observed.span.beginOffset||field.end!=observed.span.endOffset||raw.groups.at(span.group).code()!=span.code)return fail(__LINE__);++fields;
            if(span.role!=dx_visualstyle_source::Role::None)++roles;
            if(span.field==dx_visualstyle_source::Field::Primary){auto changed=raw;auto& value=changed.groups.at(span.group);
                if(value.type()==DRW_Variant::INTEGER)value=DRW_Variant(value.code(),value.i_val()?0:1);
                else if(value.type()==DRW_Variant::DOUBLE)value=DRW_Variant(value.code(),value.d_val()+0.125);
                else value=DRW_Variant(value.code(),UTF8STRING("different authored scalar"));
                const auto profile=dx_rendersettings_source::audit(changed);
                if(dx_rendersettings_source::matches(profile,changed,static_cast<const DRW_RenderSettings&>(*row.input->typedModel),*row.input->context))return fail(__LINE__);++binders;}
        }
        for(const auto& use:A::dependencies(data)->rows)if(use.target==0xEA02u&&use.role==dx_tableResourceRole::LayoutShadePlot){if(use.reason!=Reason::Matched)return fail(__LINE__);++uses;}
        const auto before=symbolFailedAllocations;{SymbolAllocationFailure none(0);if(!ready(data))return fail(__LINE__);}if(before!=symbolFailedAllocations)return fail(__LINE__);++queries;
        if(commented)++comments;++profiles;
    }
    for(unsigned encoding=0u;encoding<3u;++encoding)for(unsigned fault=0u;fault<12u;++fault){auto tags=document("AC1027",false,3u,false);
        if(fault==0u)tags[slot(tags,3,root(3u))].value="ACAD_RENDER_SETTINGS";
        if(fault==1u)tags[slot(tags,3,"AuthoredPreset")].value="WRONG_PRESET_NAME";
        if(fault==2u)tags[slot(tags,5,"EE00")+1u].value="EB00";
        if(fault==3u)tags[slot(tags,5,"EA02")+1u].value="EB00";
        if(fault==4u)appendBlockFields(tags,"40",{{3,"PRESET_ROOT_ALIAS"},{350,"EE00"}});
        if(fault==5u){auto at=slot(tags,5,"EA02");while(tags[at].code!=90)++at;tags[at].value="99";}
        if(fault==6u){const auto at=slot(tags,100,"AcDbRapidRTRenderSettings");tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{290,"0"}});}
        if(fault==7u){auto at=slot(tags,5,"EA02");while(tags[at].code!=40)++at;tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at));}
        if(fault==8u){auto at=slot(tags,5,"EA02");while(tags[at].code!=1001)++at;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{70,"7"}});}
        if(fault==9u){auto at=slot(tags,5,"EA02");tags[at-1u].value="ACDBRAPIDRTRENDERSETTINGS";}
        if(fault==10u){auto at=slot(tags,5,"EA02");while(tags[at].code!=1001)++at;tags[at].value="UNREGISTERED_PRESET_APP";}
        if(fault==11u)tags[slot(tags,3,"AuthoredPreset")+1u].value="EA01";
        dx_iface reader;dx_data data;if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||ready(data))return fail(__LINE__);++refusals;
        dx_data fixed;if(!writeBlockBinding(path,document("AC1027",false,3u,false),encoding)||!reader.fileImport(path.string(),&fixed,false)||!ready(fixed))return fail(__LINE__);++retries;
    }
    for(unsigned encoding=0u;encoding<3u;++encoding){dx_iface reader;dx_data data;
        if(!writeBlockBinding(path,document("AC1027",false,3u,true),encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data))return fail(__LINE__);
        const auto report=dx_iface::renderingSourceReport(data);
        for(bool work:{false,true}){const auto exact=work?report.work:report.bytes;A::renderingSourceLimit(data,work,exact);A::repeatRenderingSources(reader,data);A::joinRenderingSources(reader,data);
            if(!ready(data))return fail(__LINE__);A::renderingSourceLimit(data,work,exact-1u);A::repeatRenderingSources(reader,data);
            if(dx_iface::renderingSourceReport(data).state!=dx_iface::RenderingSourceState::Limited||dx_iface::dependencyReport(data).originalTargetsMatched)return fail(__LINE__);++limits;
            A::renderingSourceLimit(data,work,work?128u*1024u*1024u:64u*1024u*1024u);A::repeatRenderingSources(reader,data);A::joinRenderingSources(reader,data);if(!ready(data))return fail(__LINE__);++retries;}
    }
    for(unsigned encoding:{0u,2u}){auto tags=document("AC1027",false,8u,true);
        const std::string longName(192u,'P');tags[slot(tags,3,"AuthoredPreset")].value=longName;tags[slot(tags,1,"AuthoredPreset")].value=longName;
        tags[slot(tags,1,"authored render description")].value=std::string(192u,'D');dx_iface reader;dx_data data;
        if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data))return fail(__LINE__);
        unsigned untouched=0u;for(long offset=0;offset<256;++offset){const auto before=symbolFailedAllocations;
            {SymbolAllocationFailure fault(offset);A::attemptRenderingSources(reader,data);}
            if(symbolFailedAllocations!=before){if(dx_iface::renderingSourceReport(data).state!=dx_iface::RenderingSourceState::CaptureFailed||dx_iface::dependencyReport(data).originalTargetsMatched)return fail(__LINE__);++reached;untouched=0u;}
            else{if(!dx_iface::renderingSourceReport(data).originalSourcesMatched||dx_iface::dependencyReport(data).originalTargetsMatched)return fail(__LINE__);++unreached;++untouched;}
            A::repeatRenderingSources(reader,data);A::joinRenderingSources(reader,data);if(!ready(data))return fail(__LINE__);++finalizations;
            if(untouched==2u)break;if(offset==255)return fail(__LINE__);}
    }
    for(unsigned form=0u;form<10u;++form)for(unsigned encoding:{0u,2u}){
        const auto tags=exportDocument(form);dx_iface reader;dx_data data;if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||data.renderSettings.size()!=1u)return fail(__LINE__);
        const auto model=data.renderSettings.front();
        const auto verify=[&](){dx_iface next;dx_data reopened;if(!next.fileImport(output.string(),&reopened,false)||reopened.renderSettings.size()!=1u)return false;
            const auto& value=reopened.renderSettings.front();return value.m_name==model.m_name&&value.m_longs==model.m_longs&&value.m_strings==model.m_strings
                &&value.m_bools==model.m_bools&&value.m_doubles==model.m_doubles&&value.m_shorts==model.m_shorts
                &&value.m_hasPredefinedPresent==model.m_hasPredefinedPresent&&value.m_hasPredefined==model.m_hasPredefined;};
        if(!reader.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||!verify())return fail(__LINE__);++exports;
        for(unsigned fault=0u;fault<5u;++fault){const auto original=data.renderSettings.front();auto& value=data.renderSettings.front();
            if(fault==0u)value.m_fogEnabled=!value.m_fogEnabled;
            if(fault==1u)value.m_longs.back()+=1;
            if(fault==2u)value.m_strings.back()="edited scalar";
            if(fault==3u)value.m_bools.back()=!value.m_bools.back();
            if(fault==4u)value.m_hasPredefinedPresent=!value.m_hasPredefinedPresent;
            {std::ofstream sentinel(output,std::ios::binary);sentinel<<"KEEP_PRESET_DESTINATION";}
            if(reader.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false))return fail(__LINE__);
            std::ifstream preserved(output,std::ios::binary);const std::string kept((std::istreambuf_iterator<char>(preserved)),{});if(kept!="KEEP_PRESET_DESTINATION")return fail(__LINE__);++rollback;
            value=original;if(!reader.fileExport(output.string(),DRW::AC1027,encoding==2u,&data,false)||!verify())return fail(__LINE__);++retries;
        }
        dxfRW target(output.string().c_str());TypedWriter writer(target,model);
        if(!target.write(&writer,DRW::AC1027,encoding==2u)||!writer.wrote)return fail(__LINE__);
        TypedReader actual;dxfRW input(output.string().c_str());if(!input.read(&actual,false)||actual.values.size()!=1u
            ||!dxfRendering::Observation::render(model,actual.values.front()))return fail(__LINE__);
        ++typed;
    }
    std::cout<<"Render preset source profiles="<<profiles<<"; independent fields="<<fields<<"; handle/name roles="<<roles<<"; scalar binder refusals="<<binders
        <<"; LAYOUT target uses="<<uses<<"; commented profiles="<<comments<<"; immutable queries="<<queries<<"; schema/root refusals="<<refusals
        <<"; raw export/readbacks="<<exports<<"; typed write/readbacks="<<typed<<"; destination rollbacks="<<rollback<<"; exact/one-under source limits="<<limits
        <<"; reached finalizer failures="<<reached<<"; unreached offsets="<<unreached<<"; corrected finalizations="<<finalizations<<"; corrected retries="<<retries<<'\n';
    return profiles==240u&&roles==profiles*7u&&uses==profiles*2u&&comments==120u&&queries==profiles&&refusals==36u&&exports==20u&&typed==20u&&rollback==100u
        &&limits==6u&&reached&&unreached==4u&&finalizations==reached+unreached&&retries==142u;
}
}
