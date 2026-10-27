// SPDX-License-Identifier: GPL-2.0-or-later
#include "intern/dxfblockrecord.h"
#include "intern/dxfrenderingsource.h"
#include <type_traits>
class DxIfaceTableSourceTestAccess {
public:
    static const dxfRW* renderingReader(const dx_data& data){return data.m_tableDxfSource.entitiesReadInput;}
    static auto& rendering(dx_data& data){return data.m_tableDxfSource;}
    static const auto& rendering(const dx_data& data){return data.m_tableDxfSource;}
};
namespace renderingPublicationControls {
using V=dxfRW::DxfRenderingSourceView;
using Tags=renderingProjectionControls::Tags;
using A=DxIfaceTableSourceTestAccess;
bool stateEqual(const V& a,const V& b) {
    return a.kind==b.kind&&a.originalPendingXdata==b.originalPendingXdata&&a.originalCoordinateMask==b.originalCoordinateMask
        &&a.originalVisualStyleScope==b.originalVisualStyleScope&&a.originalVisualStylePropertyTail==b.originalVisualStylePropertyTail
        &&a.originalRenderBaseScope==b.originalRenderBaseScope&&a.originalRenderCounts==b.originalRenderCounts
        &&a.originalRenderLongs==b.originalRenderLongs&&a.originalRenderBytes==b.originalRenderBytes
        &&a.originalRenderBools==b.originalRenderBools
        &&std::memcmp(a.originalRenderDoubles.data(),b.originalRenderDoubles.data(),4u*sizeof(double))==0;
}
Tags record(unsigned kind,unsigned form) {
    if(kind<7u) {
        auto tags=renderingProjectionControls::recipe(kind,form%3u,form==1u);
        if(form==2u)tags.insert(tags.end(),{{1010,"1"},{1020,"2"}});
        return tags;
    }
    Tags tags{{0,"VISUALSTYLE"},{5,"E902"},{330,"C"},{100,"AcDbObject"},{100,"AcDbVisualStyle"},
        {2,"authored-style"},{70,"8"},{71,"2"},{40,"0.25"},{290,"1"}};
    if(form)tags.insert(tags.end(),{{291,"0"},{70,"58"},{100,"VendorTail"},{4,"opaque-description"}});
    tags.insert(tags.end(),{{1001,"AUTHORED_APP"},{1000,"authored-xdata"}});
    if(form==2u)tags.insert(tags.end(),{{1010,"1"},{1020,"2"}});
    return tags;
}
struct Probe:dx_iface {
    dx_data direct;const dxfRW* input=nullptr;bool selected=true,good=true,throwTyped=false,throwRaw=false;
    bool mutate=false;unsigned fault=0u,typed=0u,raw=0u;
    std::uintptr_t typedModel=0u,typedRaw=0u;V original;std::string order;
    Probe(){cData=&direct;currentBlock=direct.mBlock;}
    bool wantsDxfResourceSource()const override{return selected;}
    const dxfRW* reader()const{return input?input:cData?A::renderingReader(*cData):nullptr;}
    void observe(const DRW_TableEntry* model,const DRW_RawDxfObject* carrier) {
        const auto* read=reader();V view;
        const bool present=read&&read->getDxfRenderingSourceView(view);
        if(present!=selected){good=false;return;}
        if(!present){if(view.model||view.source||view.kind!=V::Kind::None)good=false;return;}
        dxfRW::DxfResourceSourceView legacy;dxfRW::DxfPlotStyleSourceView plot;dxfRW::DxfViewUcsSourceView viewUcs;
        if(read->getDxfResourceSourceView(legacy)||read->getDxfPlotStyleSourceView(plot)||read->getDxfViewUcsSourceView(viewUcs))good=false;
        if(!view.source||!view.model||view.phase!=(model?V::Phase::Typed:V::Phase::Raw)
            ||(model&&view.model!=model)||(carrier&&view.source!=carrier))good=false;
        if(model){typedModel=reinterpret_cast<std::uintptr_t>(view.model);typedRaw=reinterpret_cast<std::uintptr_t>(view.source);
            original=view;original.model=nullptr;original.source=nullptr;original.phase=V::Phase::None;}
        else if(typedModel!=reinterpret_cast<std::uintptr_t>(view.model)||typedRaw!=reinterpret_cast<std::uintptr_t>(view.source)||!stateEqual(original,view))good=false;
    }
    void addRenderSettings(const DRW_RenderSettings& value)override {
        order+='T';++typed;observe(&value,nullptr);
        if(throwTyped)throw std::runtime_error("authored rendering typed callback");
        if(fault==1u){const DRW_RenderSettings copy(value);dx_iface::addRenderSettings(copy);return;}
        if(fault==2u)return;
        auto& actual=const_cast<DRW_RenderSettings&>(value);
        if(mutate){actual.m_name="typed-mutated-name";V repeated;if(!reader()->getDxfRenderingSourceView(repeated)||!stateEqual(original,repeated))good=false;}
        dx_iface::addRenderSettings(value);
        if(mutate)actual.m_name="raw-phase-mutated-name";
    }
    void addVisualStyle(const DRW_VisualStyle& value)override {
        order+='T';++typed;observe(&value,nullptr);
        if(throwTyped)throw std::runtime_error("authored visual typed callback");
        if(fault==1u){const DRW_VisualStyle copy(value);dx_iface::addVisualStyle(copy);return;}
        if(fault==2u)return;
        auto& actual=const_cast<DRW_VisualStyle&>(value);
        if(mutate){actual.desc="typed-mutated-description";V repeated;if(!reader()->getDxfRenderingSourceView(repeated)||!stateEqual(original,repeated))good=false;}
        dx_iface::addVisualStyle(value);
        if(mutate)actual.desc="raw-phase-mutated-description";
    }
    void addRawDxfObject(const DRW_RawDxfObject& value)override {
        order+='R';++raw;observe(nullptr,&value);
        if(throwRaw)throw std::runtime_error("authored rendering raw callback");
        if(fault==3u){const DRW_RawDxfObject copy(value);dx_iface::addRawDxfObject(copy);return;}
        if(fault==4u)return;
        dx_iface::addRawDxfObject(value);
        typedModel=typedRaw=0u;
    }
};
bool expected(const V& view,unsigned kind,unsigned form) {
    if(view.originalPendingXdata!=(form==2u)||view.originalCoordinateMask!=(form==2u?3u:0u))return false;
    if(kind==7u)return view.kind==V::Kind::VisualStyle&&view.originalVisualStyleScope==(form==0u)
        &&view.originalVisualStylePropertyTail==(form!=0u)&&view.originalRenderCounts==std::array<std::size_t,5>{};
    if(view.kind!=V::Kind::RenderSettings)return false;
    const std::array<std::size_t,5> counts=kind==0u?std::array<std::size_t,5>{1u,1u,3u,3u,4u}
        :kind==2u?std::array<std::size_t,5>{2u,2u,3u,0u,0u}
        :kind==5u?std::array<std::size_t,5>{1u,5u,3u,0u,0u}
        :kind==6u?std::array<std::size_t,5>{3u,3u,0u,0u,1u}
        :std::array<std::size_t,5>{3u,2u,form==2u?4u:5u,0u,0u};
    const bool base=form!=1u&&(kind==0u||kind==4u||kind==5u||kind==6u);
    if(view.originalRenderCounts!=counts||view.originalRenderBaseScope!=base||view.originalRenderLongs[0]!=1)return false;
    if(kind==0u)return view.originalRenderBytes==std::array<std::int32_t,3>{17,33,65}
        &&view.originalRenderDoubles==std::array<double,4>{12.5,37.5,25.,75.};
    return true;
}
void run(TestContext& test) {
    const auto path=std::filesystem::temp_directory_path()/(
        "libdxfrw-render-publication-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dxf");
    std::size_t profiles=0u,inputs=0u,refusals=0u,retries=0u,quotas=0u,snapshots=0u,pins=0u,pending=0u;
    const auto save=[&](const std::string& text){std::ofstream file(path,std::ios::binary);file.write(text.data(),static_cast<std::streamsize>(text.size()));};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)
    for(unsigned kind=0u;kind<8u;++kind)for(unsigned form=0u;form<3u;++form)for(bool selected:{false,true}) {
        const auto text=renderingProjectionControls::wire(record(kind,form),encoding==2u,version);save(text);
        dxfRW reader(path.string().c_str());Probe probe;probe.input=&reader;probe.selected=selected;
        std::string memory=text;V expired;expired.kind=V::Kind::VisualStyle;
        test.expect(!reader.getDxfRenderingSourceView(expired)&&expired.kind==V::Kind::None,"rendering view clears before a publication");
        const bool parsed=encoding==0u?reader.readAscii(&probe,false,memory):reader.read(&probe,false);
        const bool good=parsed&&probe.good&&probe.order=="TR"&&probe.typed==1u&&probe.raw==1u
            &&(!selected||expected(probe.original,kind,form));
        test.expect(good,"actual original rendering state and existing callback order across opt-in profiles");
        if(!good){std::cerr<<"render publication "<<version<<'/'<<encoding<<'/'<<kind<<'/'<<form<<'/'<<selected<<'\n';continue;}
        test.expect(!reader.getDxfRenderingSourceView(expired)&&!expired.model&&!expired.source&&expired.kind==V::Kind::None,
            "rendering borrowed view expires after the pair");++profiles;
    }
    for(unsigned kind:{0u,1u,2u,3u,7u})for(bool binary:{false,true})for(unsigned form:{0u,2u}) {
        const auto text=renderingProjectionControls::wire(record(kind,form),binary,"AC1027");save(text);
        dx_data data;Probe reader;const bool parsed=reader.fileImport(path.string(),&data,false);
        const auto report=dx_iface::renderingInputReport(data);
        const bool captured=parsed&&reader.good&&report.publicationInputsCaptured&&report.records==1u&&report.physical==1u
            &&report.pairs==1u&&report.rawStable==1u&&report.stateStable==1u&&report.origins==1u
            &&!report.fullModelsMatched&&!report.originalSourcesMatched&&!report.ownerTargetsResolved&&!report.outputEligible;
        test.expect(captured,"actual rendering publication inputs bind physical OBJECTS origins without semantic promotion");
        if(!captured){std::cerr<<"render input kind="<<kind<<" binary="<<binary<<" state="<<static_cast<int>(report.state)<<" physical="<<report.physical<<" records="<<report.records<<'\n';continue;}++inputs;
        const auto& state=A::rendering(data);const auto& input=*state.renderingInputs.begin()->second;
        test.expect(!input.original.model&&!input.original.source&&input.typedModel&&input.rawModel&&input.typedRaw&&input.rawRaw,
            "immutable rendering owners retain both callback snapshots and no expired borrowed pointers");
        if(form==2u){test.expect(input.original.originalPendingXdata&&input.original.originalCoordinateMask==3u
            &&dxfBlockRecord::Decoder::observeCommonState(*input.typedModel)==std::make_pair(true,std::uint8_t{3})
            &&dxfBlockRecord::Decoder::observeCommonState(*input.rawModel)==std::make_pair(true,std::uint8_t{3})
            &&input.typedModel->extData.back()!=input.rawModel->extData.back(),
            "original pending rendering state agrees with separately owned copied XDATA variants");++pending;}
        auto& mutableState=A::rendering(data);
        const auto clone=[](const auto& pointer){using T=std::remove_const_t<typename std::decay_t<decltype(pointer)>::element_type>;
            return std::make_shared<const T>(*pointer);};
        const auto originalAudit=mutableState.renderingAudit;
        mutableState.renderingAudit=clone(originalAudit);
        test.expect(dx_iface::renderingInputReport(data).state==dx_iface::RenderingInputState::ContextMismatch,
            "equal-value rendering audit replacement cannot replace its original owner");mutableState.renderingAudit=originalAudit;++pins;
        const auto originalInput=mutableState.renderingInputs.begin()->second;
        mutableState.renderingInputs.begin()->second=clone(originalInput);
        test.expect(dx_iface::renderingInputReport(data).state==dx_iface::RenderingInputState::ContextMismatch,
            "equal-value rendering input replacement cannot replace its original owner");mutableState.renderingInputs.begin()->second=originalInput;++pins;
        mutableState.renderingInputOwners.front()=std::decay_t<decltype(originalInput)>(originalInput.get(),[](const auto*){});
        test.expect(dx_iface::renderingInputReport(data).state==dx_iface::RenderingInputState::ContextMismatch,
            "same-address foreign rendering input owner cannot qualify");mutableState.renderingInputOwners.front()=originalInput;++pins;
        ++mutableState.renderingRecords;
        test.expect(dx_iface::renderingInputReport(data).state==dx_iface::RenderingInputState::ContextMismatch,
            "mutable rendering counters cannot replace the sealed input denominator");--mutableState.renderingRecords;++pins;
        test.expect(dx_iface::renderingInputReport(data).publicationInputsCaptured,"restored actual rendering owners recover report inspection");
        for(unsigned which=0u;which<4u;++which){dx_data limited;auto& limits=A::rendering(limited);
            const auto exact=which==0u?state.renderingRecords:which==1u?state.renderingGroups:which==2u?report.bytes:report.work;
            if(which==0u)limits.maxRenderingRecords=exact;else if(which==1u)limits.maxRenderingGroups=exact;else if(which==2u)limits.maxRenderingBytes=exact;else limits.maxRenderingWork=exact;
            Probe exactReader;test.expect(exactReader.fileImport(path.string(),&limited,false)&&dx_iface::renderingInputReport(limited).publicationInputsCaptured,
                "exact rendering input limit accepts");
            dx_data under;auto& underLimits=A::rendering(under);
            if(which==0u)underLimits.maxRenderingRecords=exact-1u;else if(which==1u)underLimits.maxRenderingGroups=exact-1u;else if(which==2u)underLimits.maxRenderingBytes=exact-1u;else underLimits.maxRenderingWork=exact-1u;
            Probe underReader;test.expect(underReader.fileImport(path.string(),&under,false)&&dx_iface::renderingInputReport(under).state==dx_iface::RenderingInputState::CaptureFailed,
                "one-under rendering capture limit stays unqualified while ordinary read remains available");++quotas;
        }
        for(unsigned fault=1u;fault<=4u;++fault){dx_data invalid;Probe broken;broken.fault=fault;
            test.expect(broken.fileImport(path.string(),&invalid,false)&&dx_iface::renderingInputReport(invalid).state==dx_iface::RenderingInputState::CaptureFailed,
                "copied or omitted rendering callback cannot substitute for the actual pair");++refusals;
            dx_data corrected;broken.fault=0u;test.expect(broken.fileImport(path.string(),&corrected,false)&&dx_iface::renderingInputReport(corrected).publicationInputsCaptured,
                "same adapter recovers with a genuine corrected rendering pair");++retries;
        }
        dx_data changed;Probe mutation;mutation.mutate=true;
        test.expect(mutation.fileImport(path.string(),&changed,false)&&!dx_iface::renderingInputReport(changed).fullModelsMatched,
            "callback model changes cannot promote full rendering equality");
        const auto& observed=*A::rendering(changed).renderingInputs.begin()->second;
        if(kind==7u){const auto& a=static_cast<const DRW_VisualStyle&>(*observed.typedModel);const auto& b=static_cast<const DRW_VisualStyle&>(*observed.rawModel);
            test.expect(a.desc=="typed-mutated-description"&&b.desc=="raw-phase-mutated-description","separate immutable visual snapshots preserve actual phase values");}
        else {const auto& a=static_cast<const DRW_RenderSettings&>(*observed.typedModel);const auto& b=static_cast<const DRW_RenderSettings&>(*observed.rawModel);
            test.expect(a.m_name=="typed-mutated-name"&&b.m_name=="raw-phase-mutated-name","separate immutable preset snapshots preserve actual phase values");}++snapshots;
    }
    for(bool rawThrow:{false,true}){
        const auto text=renderingProjectionControls::wire(record(7u,2u),false,"AC1027");save(text);
        dxfRW reader(path.string().c_str());Probe throwing;throwing.input=&reader;throwing.throwTyped=!rawThrow;throwing.throwRaw=rawThrow;
        std::string memory=text;bool caught=false;try{(void)reader.readAscii(&throwing,false,memory);}catch(const std::runtime_error&){caught=true;}
        V expired;test.expect(caught&&!reader.getDxfRenderingSourceView(expired)&&!expired.model&&!expired.source,
            "rendering view expires after typed/raw callback throw");
        Probe fixed;fixed.input=&reader;std::string corrected=text;test.expect(reader.readAscii(&fixed,false,corrected)&&fixed.good&&expected(fixed.original,7u,2u),
            "same reader accepts corrected rendering callbacks after throw");++retries;
        dx_data failed;Probe adapter;adapter.throwTyped=!rawThrow;adapter.throwRaw=rawThrow;bool adapterThrew=false;
        try{(void)adapter.fileImport(path.string(),&failed,false);}catch(const std::runtime_error&){adapterThrew=true;}
        test.expect(adapterThrew&&dx_iface::renderingInputReport(failed).state==dx_iface::RenderingInputState::ImportFailed
            &&A::rendering(failed).renderingPending.empty()&&!A::renderingReader(failed),
            "throwing adapter closes rendering input state and releases unfinished borrowed pairs");
        dx_data recovered;adapter.throwTyped=adapter.throwRaw=false;
        test.expect(adapter.fileImport(path.string(),&recovered,false)&&dx_iface::renderingInputReport(recovered).publicationInputsCaptured,
            "same adapter recovers from a throwing rendering callback");++retries;
    }
    std::error_code ignored;std::filesystem::remove(path,ignored);
    std::cout<<"Rendering publication profiles="<<profiles<<"; immutable input pairs="<<inputs<<"; callback refusals="<<refusals
        <<"; corrected retries="<<retries<<"; exact/one-under limits="<<quotas<<"; phase snapshot controls="<<snapshots
        <<"; owner/metadata substitutions="<<pins<<"; pending snapshot controls="<<pending<<'\n';
}
}
