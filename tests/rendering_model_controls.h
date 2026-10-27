// SPDX-License-Identifier: GPL-2.0-or-later
#include <functional>
namespace renderingModelControls {
using A=DxIfaceTableSourceTestAccess;
struct Probe:dx_iface {
    unsigned phase=1u;
    std::function<void(DRW_VisualStyle&)> visual;
    std::function<void(DRW_RenderSettings&)> render;
    template<class T,class F>void changed(const T& value,F& mutation,const std::function<void()>& deliver) {
        auto& actual=const_cast<T&>(value);const T saved(value);
        if(mutation)mutation(actual);deliver();actual=saved;
    }
    void addVisualStyle(const DRW_VisualStyle& value)override {
        if(phase==1u&&visual)changed(value,visual,[&](){dx_iface::addVisualStyle(value);});
        else dx_iface::addVisualStyle(value);
    }
    void addRenderSettings(const DRW_RenderSettings& value)override {
        if(phase==1u&&render)changed(value,render,[&](){dx_iface::addRenderSettings(value);});
        else dx_iface::addRenderSettings(value);
    }
    void addRawDxfObject(const DRW_RawDxfObject& raw)override {
        dxfRW::DxfRenderingSourceView view;const auto* input=cData?A::renderingReader(*cData):nullptr;
        if(phase==2u&&input&&input->getDxfRenderingSourceView(view)) {
            if(view.kind==dxfRW::DxfRenderingSourceView::Kind::VisualStyle&&visual) {
                changed(static_cast<const DRW_VisualStyle&>(*view.model),visual,[&](){dx_iface::addRawDxfObject(raw);});return;
            }
            if(view.kind==dxfRW::DxfRenderingSourceView::Kind::RenderSettings&&render) {
                changed(static_cast<const DRW_RenderSettings&>(*view.model),render,[&](){dx_iface::addRawDxfObject(raw);});return;
            }
        }
        dx_iface::addRawDxfObject(raw);
    }
};
std::vector<std::function<void(DRW_VisualStyle&)>> visualMutations() {
    using B=DRW_VisualStyleBody;
    std::vector<std::function<void(DRW_VisualStyle&)>> edits;
    // Inventories are independent test inputs. Every member is changed through
    // an actual callback, then restored, against unchanged literal DXF bytes.
    for(int B::* field:{&B::faceLightingModel,&B::faceLightingQuality,&B::faceColorMode,&B::faceModifier,&B::faceMonoColor,
        &B::edgeModel,&B::edgeStyle,&B::edgeIntersectionColor,&B::edgeObscuredColor,&B::edgeObscuredLtype,&B::edgeIntersectionLtype,
        &B::edgeModifier,&B::edgeColor,&B::edgeWidth,&B::edgeOverhang,&B::edgeJitter,&B::edgeSilhouetteColor,&B::edgeSilhouetteWidth,
        &B::edgeHaloGap,&B::edgeIsolines,&B::edgeStyleApply,&B::displaySettings,&B::displayShadowType,&B::extLightingModel,
        &B::blProp25,&B::blProp28,&B::cProp29,&B::blProp2a,&B::blProp2b,&B::cProp2c,&B::blProp2e,&B::blProp2f,&B::blProp30,&B::blProp32,&B::cProp33,&B::edgeWiggle})
        edits.push_back([field](auto& value){++(value.m_body.*field);});
    for(bool B::* field:{&B::edgeDoHidePrecision,&B::internalOnly,&B::hasR2013bExpansion,&B::bProp1c,&B::bProp1d,&B::bProp1e,&B::bProp1f,
        &B::bProp20,&B::bProp21,&B::bProp22,&B::bProp23,&B::bProp24,&B::bProp2d,&B::bProp31,&B::bProp37})
        edits.push_back([field](auto& value){value.m_body.*field=!(value.m_body.*field);});
    for(double B::* field:{&B::faceOpacity,&B::faceSpecular,&B::edgeCreaseAngle,&B::edgeOpacity,&B::displayBrightness,&B::bdProp26,&B::bdProp27,&B::bdProp34,&B::bdProp38,&B::bdProp39})
        edits.push_back([field](auto& value){value.m_body.*field=-0.0;}); // bit equality must distinguish signed zero
    edits.push_back([](auto& value){value.desc="changed-description";});edits.push_back([](auto& value){++value.type;});
    edits.push_back([](auto& value){value.m_bodyDecoded=!value.m_bodyDecoded;});
    edits.push_back([](auto& value){value.m_body.strokes="unrepresented-strokes";});
    for(std::size_t i=0u;i<DRW_VisualStyleEncoding::kPropertyCount;++i)
        edits.push_back([i](auto& value){++value.m_body.m_encoding.companions[i];});
    for(std::size_t i=0u;i<DRW_VisualStyleEncoding::kColorCount;++i)for(unsigned field=0u;field<8u;++field)
        edits.push_back([i,field](auto& value){auto& color=value.m_body.m_encoding.colors[i];
            if(field==0u)color.present=!color.present;else if(field==1u)color.modern=!color.modern;else if(field==2u)++color.rawIndex;
            else if(field==3u)++color.rawRgb;else if(field==4u)++color.flags;else if(field==5u)++color.normalizedIndex;
            else if(field==6u)color.name="changed-color";else color.bookName="changed-book";});
    edits.push_back([](auto& value){++value.m_body.m_encoding.companionCount;});
    edits.push_back([](auto& value){++value.m_body.m_encoding.colorCount;});
    edits.push_back([](auto& value){value.m_body.m_encoding.hasSourceIsolines=true;});
    edits.push_back([](auto& value){++value.m_body.m_encoding.sourceIsolines;});
    return edits;
}
std::vector<std::function<void(DRW_RenderSettings&)>> renderMutations() {
    using R=DRW_RenderSettings;std::vector<std::function<void(R&)>> edits;
    for(std::int32_t R::* field:{&R::m_classVersion,&R::m_displayIndex,&R::m_procedure,&R::m_destination})
        edits.push_back([field](auto& value){++(value.*field);});
    for(int R::* field:{&R::m_fogColorR,&R::m_fogColorG,&R::m_fogColorB})
        edits.push_back([field](auto& value){++(value.*field);});
    for(bool R::* field:{&R::m_fogEnabled,&R::m_fogBackgroundEnabled,&R::m_backfacesEnabled,&R::m_environmentImageEnabled,&R::m_hasPredefined,&R::m_hasPredefinedPresent})
        edits.push_back([field](auto& value){value.*field=!(value.*field);});
    for(double R::* field:{&R::m_fogDensityNear,&R::m_fogDensityFar,&R::m_fogDistanceNear,&R::m_fogDistanceFar})
        edits.push_back([field](auto& value){value.*field+=1.;});
    for(UTF8STRING R::* field:{&R::m_name,&R::m_environmentImageFilename,&R::m_description})
        edits.push_back([field](auto& value){value.*field="changed-text";});
    edits.push_back([](auto& value){++value.m_longs.front();});
    edits.push_back([](auto& value){value.m_strings.front()="changed-vector-text";});
    for(std::size_t i=0u;i<3u;++i)edits.push_back([i](auto& value){value.m_bools[i]=!value.m_bools[i];});
    edits.push_back([](auto& value){value.m_shorts.push_back(1);});
    for(std::size_t i=0u;i<3u;++i)edits.push_back([i](auto& value){++value.m_bytes[i];});
    for(std::size_t i=0u;i<4u;++i)edits.push_back([i](auto& value){value.m_doubles[i]+=1.;});
    edits.push_back([](auto& value){(void)value.setDwgRawData({},0u,DRW::AC1027);});
    return edits;
}
void run(TestContext& test) {
    const auto path=std::filesystem::temp_directory_path()/("libdxfrw-render-model-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dxf");
    const auto save=[&](const auto& tags,bool binary,const std::string& version){const auto text=renderingProjectionControls::wire(tags,binary,version);
        std::ofstream output(path,std::ios::binary);output.write(text.data(),static_cast<std::streamsize>(text.size()));};
    const auto ready=[](const dx_data& data){const auto report=dx_iface::renderingModelReport(data);
        return report.modeledStateMatched&&report.records==1u&&report.decoded==1u&&report.matched==1u
            &&!report.propertyCoverageMatched&&!report.originalSourcesMatched&&!report.ownerTargetsResolved&&!report.outputEligible;};
    std::size_t profiles=0u,fields=0u,retries=0u,commonFields=0u,quotas=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true})
    for(unsigned kind=0u;kind<8u;++kind)for(unsigned form=0u;form<3u;++form){
        save(renderingPublicationControls::record(kind,form),binary,version);Probe reader;dx_data data;
        const bool good=reader.fileImport(path.string(),&data,false)&&ready(data);
        test.expect(good,"original complete retained rendering state replays against both real callback snapshots");
        if(!good){const auto report=dx_iface::renderingModelReport(data);std::cerr<<"render model="<<version<<'/'<<binary<<'/'<<kind<<'/'<<form
            <<" state="<<static_cast<int>(report.state)<<" decoded="<<report.decoded<<" matched="<<report.matched<<'\n';continue;}++profiles;
    }
    for(bool visual:{false,true})for(unsigned phase:{1u,2u}) {
        save(renderingPublicationControls::record(visual?7u:0u,2u),false,"AC1027");
        const auto exercise=[&](Probe& reader){dx_data changed;const bool imported=reader.fileImport(path.string(),&changed,false);
            const auto report=dx_iface::renderingModelReport(changed);
            test.expect(imported&&!report.modeledStateMatched&&!report.propertyCoverageMatched&&!report.originalSourcesMatched&&!report.outputEligible,
                "actual callback field mutation cannot masquerade as original modeled state");
            reader.visual={};reader.render={};dx_data corrected;
            test.expect(reader.fileImport(path.string(),&corrected,false)&&ready(corrected),"same adapter accepts corrected original rendering fields");++retries;};
        if(visual)for(const auto& edit:visualMutations()){Probe reader;reader.phase=phase;reader.visual=edit;exercise(reader);++fields;}
        else for(const auto& edit:renderMutations()){Probe reader;reader.phase=phase;reader.render=edit;exercise(reader);++fields;}
        for(unsigned field=0u;field<10u;++field){Probe reader;reader.phase=phase;
            const auto edit=[field](DRW_TableEntry& value){if(field<3u)DrwObjectEncodeTestAccess::renderingHeader(value,field);
                else if(field==3u)value.setDwgCommonObjectState(1,0,false);else if(field==4u)value.setDwgCommonObjectState(0,1,false);
                else if(field==5u)value.setDwgCommonObjectState(0,0,true);else if(field==6u)value.setDwgCommonLinkTailValidated(true);
                else if(field==7u)value.name="changed-common-name";else if(field==8u)++value.flags;else DrwObjectEncodeTestAccess::renderingCursor(value);};
            if(visual)reader.visual=edit;else reader.render=edit;exercise(reader);++commonFields;}
    }
    for(bool binary:{false,true})for(bool visual:{false,true}){
        save(renderingPublicationControls::record(visual?7u:0u,0u),binary,"AC1027");dx_data data;Probe reader;
        test.expect(reader.fileImport(path.string(),&data,false)&&ready(data),"render replay quota baseline is available");
        const auto report=dx_iface::renderingModelReport(data);auto& state=A::rendering(data);
        for(bool work:{false,true}){auto& cap=work?state.maxRenderingReplayWork:state.maxRenderingReplayBytes;
            const auto original=cap,exact=work?report.work:report.bytes;cap=exact;A::repeatRendering(reader,data);
            test.expect(ready(data),"exact rendering model replay allowance accepts");cap=exact-1u;A::repeatRendering(reader,data);
            const auto refused=dx_iface::renderingModelReport(data);
            test.expect(dx_iface::renderingInputReport(data).publicationInputsCaptured&&refused.state==dx_iface::RenderingModelState::Limited
                &&!refused.modeledStateMatched&&!refused.originalSourcesMatched,"one-under replay allowance refuses qualification while preserving actual inputs");
            cap=original;A::repeatRendering(reader,data);test.expect(ready(data),"corrected model allowance reuses the same immutable original inputs");++quotas;}
    }
    std::error_code ignored;std::filesystem::remove(path,ignored);
    std::cout<<"Rendering modeled replay profiles="<<profiles<<"; actual field refusals="<<fields<<"; common/header/cursor refusals="<<commonFields
        <<"; corrected imports="<<retries<<"; exact/one-under replay allowances="<<quotas<<'\n';
}
}
