// SPDX-License-Identifier: GPL-2.0-or-later
namespace visualStylePropertyControls {
using P=DRW_DxfVisualStyleProperties;using I=P::Integer;using R=P::Real;
using A=DxIfaceTableSourceTestAccess;using Tags=renderingProjectionControls::Tags;
// Literal independent source inventory; do not derive fixture codes/expectations
// from the production decoder's dispatch tables.
const std::array<int,28> integerCodes{70,71,72,73,90,62,63,421,74,91,64,65,75,175,92,66,76,77,78,67,79,170,171,290,174,93,173,291};
const std::array<int,28> integerValues{8,3,2,4,3,7,256,1122867,2,5,4,5,2,3,7,6,4,3,2,7,2,1,9,1,1,7,2,0};
const std::array<int,5> realCodes{40,41,42,43,44};
const std::array<double,5> realValues{0.25,0.75,0.5,0.625,1.25};
Tags primary(bool zero=false) {
    Tags tags{{2,zero?"":"primary-style"}};
    for(std::size_t i=0u;i<integerCodes.size()-1u;++i)tags.push_back({integerCodes[i],std::to_string(zero?0:integerValues[i])});
    for(std::size_t i=0u;i<realCodes.size();++i)tags.push_back({realCodes[i],zero?"-0.0":std::to_string(realValues[i])});
    return tags;
}
Tags record(unsigned form) {
    Tags tags{{0,"VISUALSTYLE"},{5,"EA01"},{330,"C"},{100,"AcDbObject"}};
    if(form==3u)tags={{0,"VISUALSTYLE"},{5,"EA01"},{330,"C"}}; // legacy markerless projection
    else if(form==2u){tags.push_back({100,"VendorProperties"});const auto junk=primary(true);tags.insert(tags.end(),junk.begin(),junk.end());tags.push_back({291,"1"});}
    if(form!=3u)tags.push_back({100,"AcDbVisualStyle"});
    const auto fields=primary(form==6u);if(form==5u)tags.insert(tags.end(),{{2,"primary-style"},{70,"8"}});else tags.insert(tags.end(),fields.begin(),fields.end());
    if(form==8u)for(auto& tag:tags)if(tag.code==2){tag.code=4;break;}
    if(form==4u)tags.insert(tags.end(),{{71,"0"},{2,"duplicate-style"}});
    if(form==7u)tags.push_back({290,"2"});
    tags.insert(tags.end(),{{102,"{AUTHORED_APP"},{71,"99"},{421,"16777215"},{102,"}"}});
    tags.push_back({291,"0"});
    if(form==1u){tags.push_back({100,"AcDbVisualStyle"});const auto tail=primary(true);tags.insert(tags.end(),tail.begin(),tail.end());tags.insert(tags.end(),{{291,"1"},{176,"1"}});}
    tags.insert(tags.end(),{{1001,"AUTHORED_APP"},{1000,"authored-xdata"},{1010,"1"},{1020,"2"}});
    return tags;
}
bool body(const DRW_VisualStyle& value,unsigned form) {
    if(value.m_bodyDecoded||value.m_body.faceMonoColor||value.m_body.m_encoding.colorCount||value.m_body.hasR2013bExpansion)return false;
    const bool zero=form==6u,absent=form==5u;const auto& b=value.m_body;
    const auto ints=std::make_tuple(b.faceLightingModel,b.faceLightingQuality,b.faceColorMode,b.faceModifier,b.edgeModel,b.edgeStyle,
        b.edgeIntersectionColor,b.edgeObscuredColor,b.edgeObscuredLtype,b.edgeIntersectionLtype,b.edgeModifier,b.edgeColor,
        b.edgeWidth,b.edgeOverhang,b.edgeJitter,b.edgeSilhouetteColor,b.edgeSilhouetteWidth,b.edgeHaloGap,b.edgeIsolines,
        b.edgeDoHidePrecision,b.edgeStyleApply,b.displaySettings,b.displayShadowType,b.internalOnly);
    const auto expected=(zero||absent)?std::make_tuple(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,false,0,0,0,false)
        :std::make_tuple(form==4u?0:3,2,4,3,2,5,4,5,2,3,7,6,4,3,2,7,2,1,9,true,1,7,2,false);
    if(ints!=expected||value.type!=(zero?0:8)||value.desc!=(zero?"":form==4u?"duplicate-style":"primary-style"))return false;
    const double actual[]{b.faceOpacity,b.faceSpecular,b.edgeCreaseAngle,b.edgeOpacity,b.displayBrightness};
    for(std::size_t i=0u;i<5u;++i){const auto wanted=zero?-0.0:absent?0.0:realValues[i];if(std::memcmp(&wanted,&actual[i],sizeof(double)))return false;}
    return true;
}
bool properties(const P& value,unsigned form) {
    if(!value.inVisualStyle||!value.propertyTail||value.invalidBoolean!=(form==7u)
        ||value.descriptionOccurrences!=(form==8u?0u:form==4u?2u:1u)||value.legacyDescriptionOccurrences!=(form==8u?1u:0u))return false;
    for(std::size_t i=0u;i<28u;++i){const bool absent=form==5u&&i!=0u&&i!=27u;
        const auto count=absent?0u:((form==4u&&i==1u)||(form==7u&&i==23u))?2u:1u;
        const auto wanted=absent||form==6u||(form==4u&&i==1u)?0:(form==7u&&i==23u)?2:integerValues[i];
        if(value.integerOccurrences[i]!=count||value.integers[i]!=wanted)return false;
    }
    for(std::size_t i=0u;i<5u;++i){const auto wanted=form==6u?-0.0:form==5u?0.0:realValues[i];
        if(value.realOccurrences[i]!=(form==5u?0u:1u)||std::memcmp(&wanted,&value.reals[i],sizeof(double)))return false;}
    return true;
}
bool literalRawMatches(const DRW_RawDxfObject& raw,const Tags& tags) {
    if(raw.groups.size()+1u!=tags.size())return false;
    for(std::size_t i=0u;i<raw.groups.size();++i){const auto& actual=raw.groups[i];const auto& literal=tags[i+1u];
        if(actual.code()!=literal.code)return false;
        if(actual.type()==DRW_Variant::STRING){if(!actual.content.s||*actual.content.s!=literal.text)return false;}
        else if(actual.type()==DRW_Variant::INTEGER){if(actual.i_val()!=std::stoi(literal.text))return false;}
        else if(actual.type()==DRW_Variant::DOUBLE){const auto wanted=std::stod(literal.text),found=actual.d_val();if(std::memcmp(&wanted,&found,sizeof(double)))return false;}
        else return false;
    }
    return true;
}
struct Probe:dx_iface {
    dx_data direct;
    bool selected=true,good=true,throwTyped=false,throwRaw=false;unsigned form=0u,typed=0u,raw=0u;
    const dxfRW* input=nullptr;P saved;std::uintptr_t modelId=0u,sourceId=0u,propertiesId=0u;
    Probe(){cData=&direct;currentBlock=direct.mBlock;}
    bool wantsDxfResourceSource()const override{return selected;}
    const dxfRW* reader()const{return input?input:cData?A::renderingReader(*cData):nullptr;}
    void observe(const DRW_VisualStyle* model,const DRW_RawDxfObject* source) {
        dxfRW::DxfVisualStylePropertiesView view;const auto* in=reader();const bool present=in&&in->getDxfVisualStylePropertiesView(view);
        if(present!=selected){good=false;return;}
        if(!present){if(view.model||view.source||view.properties||view.phase!=dxfRW::DxfRenderingSourceView::Phase::None)good=false;return;}
        if(!view.model||!view.source||!view.properties||(model&&model!=view.model)||(source&&source!=view.source)
            ||view.phase!=(model?dxfRW::DxfRenderingSourceView::Phase::Typed:dxfRW::DxfRenderingSourceView::Phase::Raw)
            ||!properties(*view.properties,form)||!body(*view.model,form))good=false;
        if(model){saved=*view.properties;modelId=reinterpret_cast<std::uintptr_t>(view.model);sourceId=reinterpret_cast<std::uintptr_t>(view.source);propertiesId=reinterpret_cast<std::uintptr_t>(view.properties);}
        else if(modelId!=reinterpret_cast<std::uintptr_t>(view.model)||sourceId!=reinterpret_cast<std::uintptr_t>(view.source)
            ||propertiesId!=reinterpret_cast<std::uintptr_t>(view.properties)||!dxfRendering::propertiesEqual(saved,*view.properties))good=false;
    }
    void addVisualStyle(const DRW_VisualStyle& value)override{++typed;if(!body(value,form)){good=false;static unsigned shown=0u;if(shown++<4u){const auto& b=value.m_body;
        std::cerr<<"property body diagnostic form="<<form<<" type="<<value.type<<" desc="<<value.desc<<" decoded="<<value.m_bodyDecoded<<" colors="<<b.faceMonoColor<<'/'<<b.m_encoding.colorCount
            <<" face="<<b.faceLightingModel<<','<<b.faceLightingQuality<<','<<b.faceColorMode<<','<<b.faceModifier<<" edge="<<b.edgeModel<<','<<b.edgeStyle<<','<<b.edgeIntersectionColor<<','<<b.edgeObscuredColor
            <<','<<b.edgeObscuredLtype<<','<<b.edgeIntersectionLtype<<','<<b.edgeModifier<<','<<b.edgeColor<<','<<b.edgeWidth<<','<<b.edgeOverhang<<','<<b.edgeJitter<<','<<b.edgeSilhouetteColor<<','<<b.edgeSilhouetteWidth
            <<','<<b.edgeHaloGap<<','<<b.edgeIsolines<<','<<b.edgeDoHidePrecision<<','<<b.edgeStyleApply<<" display="<<b.displaySettings<<','<<b.displayShadowType<<','<<b.internalOnly
            <<" reals="<<b.faceOpacity<<','<<b.faceSpecular<<','<<b.edgeCreaseAngle<<','<<b.edgeOpacity<<','<<b.displayBrightness<<'\n';}}
        observe(&value,nullptr);
        if(throwTyped)throw std::runtime_error("authored property typed throw");dx_iface::addVisualStyle(value);}
    void addRawDxfObject(const DRW_RawDxfObject& value)override{++raw;observe(nullptr,&value);
        if(throwRaw)throw std::runtime_error("authored property raw throw");dx_iface::addRawDxfObject(value);}
};
void run(TestContext& test) {
    test.expect(P::integerCodes==integerCodes&&P::realCodes==realCodes,"public property enum/code bindings match independent specification inventory");
    const auto path=std::filesystem::temp_directory_path()/("libdxfrw-visual-properties-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dxf");
    std::size_t profiles=0u,lifetimes=0u,rawFields=0u,retries=0u,metadataRefusals=0u,cursorClears=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true})for(bool selected:{false,true})for(unsigned form=0u;form<9u;++form){
        const auto tags=record(form);const auto wire=renderingProjectionControls::wire(tags,binary,version);
        {std::ofstream output(path,std::ios::binary);output.write(wire.data(),static_cast<std::streamsize>(wire.size()));}
        Probe reader;reader.form=form;reader.selected=selected;dx_data data;
        const bool imported=reader.fileImport(path.string(),&data,false);const auto report=dx_iface::renderingModelReport(data);
        const bool good=imported&&reader.good&&reader.typed==1u&&reader.raw==1u&&!report.propertyCoverageMatched&&!report.originalSourcesMatched&&!report.outputEligible;
        test.expect(good,"literal primary properties preserve values/presence/colors/scope and raw pairs over opt-in/out");
        if(!good){std::cerr<<"visual properties "<<version<<'/'<<binary<<'/'<<selected<<'/'<<form<<" good="<<reader.good<<" report="<<static_cast<int>(report.state)<<'\n';continue;}
        if(selected){const auto& input=*A::rendering(data).renderingInputs.at(0xEA01u);
            test.expect(input.visualPropertiesCaptured&&properties(input.typedVisualProperties,form)&&properties(input.rawVisualProperties,form),"both actual primary property observations are owned");
            if(form!=7u)test.expect(report.modeledStateMatched,"complete retained primary model and metadata replay exactly");
            test.expect(literalRawMatches(*input.typedRaw,tags)&&literalRawMatches(*input.rawRaw,tags),"both complete raw property carriers retain every literal field in order");
            if(input.typedRaw->groups.size()!=tags.size()-1u)std::cerr<<"property raw groups="<<input.typedRaw->groups.size()<<" expected="<<tags.size()-1u<<'\n';
            rawFields+=input.typedRaw->groups.size();}
        ++profiles;
    }
    for(bool binary:{false,true})for(unsigned phase:{1u,2u}){
        const auto wire=renderingProjectionControls::wire(record(1u),binary,"AC1027");
        {std::ofstream output(path,std::ios::binary);output.write(wire.data(),static_cast<std::streamsize>(wire.size()));}
        Probe reader;reader.form=1u;reader.throwTyped=phase==1u;reader.throwRaw=phase==2u;dxfRW source(path.string().c_str());reader.input=&source;
        dxfRW::DxfVisualStylePropertiesView expired;bool caught=false;
        test.expect(!source.getDxfVisualStylePropertiesView(expired)&&!expired.properties,"primary property view clears before read");
        try{(void)source.read(&reader,false);}catch(const std::runtime_error&){caught=true;}
        test.expect(caught&&reader.good&&!source.getDxfVisualStylePropertiesView(expired)&&!expired.properties&&!expired.model&&!expired.source,"primary property view expires after typed/raw throw");
        reader.throwTyped=reader.throwRaw=false;reader.typed=reader.raw=0u;
        test.expect(source.read(&reader,false)&&reader.good&&reader.typed==1u&&reader.raw==1u&&!source.getDxfVisualStylePropertiesView(expired),"same reader accepts corrected property callback retry");++lifetimes;++retries;
    }
    // Change separately owned captured metadata through the existing friend seam;
    // exact model equality alone cannot hide loss/substitution of source roles.
    const auto wire=renderingProjectionControls::wire(record(0u),false,"AC1027");
    {std::ofstream output(path,std::ios::binary);output.write(wire.data(),static_cast<std::streamsize>(wire.size()));}
    for(bool raw:{false,true})for(unsigned field=0u;field<5u;++field){Probe reader;dx_data data;
        test.expect(reader.fileImport(path.string(),&data,false),"original property metadata mutation baseline imports");
        auto& state=A::rendering(data);const auto original=state.renderingInputs.at(0xEA01u);
        auto changed=std::make_shared<std::remove_const_t<typename std::decay_t<decltype(original)>::element_type>>(*original);
        auto& p=raw?changed->rawVisualProperties:changed->typedVisualProperties;
        if(field<3u)++p.integers[5u+field];else if(field==3u)++p.integerOccurrences[5u];else p.propertyTail=false;
        state.renderingInputs.at(0xEA01u)=changed;state.renderingInputOwners[changed->index]=changed;A::repeatRendering(reader,data);
        test.expect(!dx_iface::renderingModelReport(data).modeledStateMatched,"metadata role/presence/scope mutation refuses original modeled equality");++metadataRefusals;
        state.renderingInputs.at(0xEA01u)=original;state.renderingInputOwners[original->index]=original;A::repeatRendering(reader,data);
        test.expect(dx_iface::renderingModelReport(data).modeledStateMatched,"original metadata restores qualification from the same input pair");++retries;
    }
    for(bool binary:{false,true})for(const auto& scalar:primary()){
        if(scalar.code==2||scalar.code==70||scalar.code==62||scalar.code==63||scalar.code==421)continue;
        Tags tags{{0,"VISUALSTYLE"},{5,"EA01"},{330,"C"},{100,"AcDbVisualStyle"},
            {1001,"AUTHORED_APP"},{1010,"1"},{1020,"2"},scalar};
        const auto text=renderingProjectionControls::wire(tags,binary,"AC1027");
        {std::ofstream output(path,std::ios::binary);output.write(text.data(),static_cast<std::streamsize>(text.size()));}
        dx_iface reader;dx_data data;const bool imported=reader.fileImport(path.string(),&data,false);
        test.expect(imported&&dx_iface::renderingModelReport(data).modeledStateMatched,"new scalar property preserves common pending-XDATA cleanup and exact replay");
        if(imported){const auto& input=*A::rendering(data).renderingInputs.at(0xEA01u);
            test.expect(!input.original.originalPendingXdata&&!input.original.originalCoordinateMask,"new primary scalar closes the prior incomplete XDATA cursor");}
        ++cursorClears;
    }
    std::error_code ignored;std::filesystem::remove(path,ignored);
    std::cout<<"VisualStyle primary profiles="<<profiles<<"; independently retained raw fields="<<rawFields<<"; callback lifetime retries="<<lifetimes
        <<"; metadata refusals="<<metadataRefusals<<"; corrected retries="<<retries<<"; pending cursor controls="<<cursorClears<<'\n';
}
}
