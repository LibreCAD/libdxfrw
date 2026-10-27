// SPDX-License-Identifier: GPL-2.0-or-later
// Reuse the existing executable's scoped allocation injector. Only actual
// publication observation and immutable report queries are faulted.
struct RenderingFailureProbe:dx_iface {
    unsigned phase=0u;long offset=-1;bool good=true;
    void observe(const DRW_TableEntry* model,const DRW_RawDxfObject* raw) {
        using A=DxIfaceTableSourceTestAccess;
        const auto* input=cData?A::readInput(*cData):nullptr;
        dxfRW::DxfRenderingSourceView view;bool present=false;
        {SymbolAllocationFailure none(0);present=input&&input->getDxfRenderingSourceView(view);}
        if(!present||(model&&view.model!=model)||(raw&&view.source!=raw))good=false;
        if(offset>=0&&phase==(model?1u:2u)) {
            SymbolAllocationFailure fault(offset);A::observeRendering(*this,model,raw);
        } else A::observeRendering(*this,model,raw);
    }
    void addVisualStyle(const DRW_VisualStyle& model)override{observe(&model,nullptr);}
    void addRenderSettings(const DRW_RenderSettings& model)override{observe(&model,nullptr);}
    void addRawDxfObject(const DRW_RawDxfObject& raw)override{observe(nullptr,&raw);}
};
bool renderingInputFailureTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::RenderingInputState;
    const auto path=dir/"authored-rendering-input.dxf";std::size_t reached=0u,unreached=0u,retries=0u,queries=0u;
    const auto fail=[&](int line){std::cerr<<"Rendering allocation control line="<<line<<" reached="<<reached<<" retries="<<retries<<'\n';return false;};
    for(bool visual:{false,true})for(bool binary:{false,true}) {
        const std::string payload(192u,'P');
        Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1027"},{0,"ENDSEC"},{0,"SECTION"},{2,"OBJECTS"},
            {0,visual?"VISUALSTYLE":"RENDERENVIRONMENT"},{5,"D901"},{330,"0"},{100,"AcDbObject"},
            {100,visual?"AcDbVisualStyle":"AcDbRenderEnvironment"}};
        if(visual)tags.insert(tags.end(),{{2,payload},{70,"8"},{40,"0.25"},{291,"0"}});
        else tags.insert(tags.end(),{{90,"1"},{290,"1"},{290,"0"},{280,"17"},{280,"33"},{280,"65"},{40,"12.5"},{40,"37.5"},{40,"25"},{40,"75"},{290,"1"},{1,payload}});
        tags.insert(tags.end(),{{1001,"AUTHORED_APP"},{1000,payload},{0,"ENDSEC"},{0,"EOF"}});
        if(!write(path,tags,binary))return fail(__LINE__);
        RenderingFailureProbe baseline;dx_data clean;
        if(!baseline.fileImport(path.string(),&clean,false)||!baseline.good||!dx_iface::renderingInputReport(clean).publicationInputsCaptured)return fail(__LINE__);
        dx_iface::RenderingInputReport report;
        const auto before=symbolFailedAllocations;
        {SymbolAllocationFailure none(0);report=dx_iface::renderingInputReport(clean);}
        if(symbolFailedAllocations!=before||!report.publicationInputsCaptured||report.fullModelsMatched||report.originalSourcesMatched||report.outputEligible)return fail(__LINE__);++queries;
        for(unsigned phase:{1u,2u}) {
            unsigned untouched=0u;
            for(long offset=0;offset<256;++offset) {
                RenderingFailureProbe reader;reader.phase=phase;reader.offset=offset;dx_data damaged;
                const auto failedBefore=symbolFailedAllocations;
                if(!reader.fileImport(path.string(),&damaged,false)||!reader.good)return fail(__LINE__);
                const bool failed=symbolFailedAllocations!=failedBefore;
                const auto observed=dx_iface::renderingInputReport(damaged);
                if(!A::renderingPending(damaged).empty())return fail(__LINE__);
                if(failed) {
                    if(observed.state!=S::CaptureFailed||observed.publicationInputsCaptured||observed.fullModelsMatched||observed.originalSourcesMatched||observed.outputEligible)return fail(__LINE__);
                    ++reached;untouched=0u;
                } else {
                    if(!observed.publicationInputsCaptured)return fail(__LINE__);
                    ++unreached;++untouched;
                }
                dx_data fixed;reader.offset=-1;
                if(!reader.fileImport(path.string(),&fixed,false)||!reader.good||!dx_iface::renderingInputReport(fixed).publicationInputsCaptured)return fail(__LINE__);++retries;
                if(untouched==2u)break;
                if(offset==255)return fail(__LINE__); // finite calibration must close
            }
        }
    }
    std::cout<<"Rendering input allocation profiles=4; immutable queries="<<queries<<"; reached failures="<<reached
        <<"; unreached offsets="<<unreached<<"; corrected imports="<<retries<<'\n';
    return reached!=0u&&unreached==16u&&retries==reached+unreached;
}

bool renderingModelFailureTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::RenderingModelState;
    const auto path=dir/"authored-rendering-model.dxf";std::size_t reached=0u,unreached=0u,retries=0u,queries=0u;
    const auto fail=[&](int line,long offset=-1){std::cerr<<"Rendering model allocation control line="<<line<<" offset="<<offset
        <<" reached="<<reached<<" retries="<<retries<<'\n';return false;};
    for(bool visual:{false,true})for(bool binary:{false,true}) {
        const std::string payload(192u,'P');
        Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1027"},{0,"ENDSEC"},{0,"SECTION"},{2,"OBJECTS"},
            {0,visual?"VISUALSTYLE":"RENDERENVIRONMENT"},{5,"D901"},{330,"0"},{100,"AcDbObject"},
            {100,visual?"AcDbVisualStyle":"AcDbRenderEnvironment"}};
        if(visual)tags.insert(tags.end(),{{2,payload},{70,"8"},{40,"0.25"},{291,"0"}});
        else tags.insert(tags.end(),{{90,"1"},{290,"1"},{290,"0"},{280,"17"},{280,"33"},{280,"65"},{40,"12.5"},{40,"37.5"},{40,"25"},{40,"75"},{290,"1"},{1,payload}});
        tags.insert(tags.end(),{{1001,"AUTHORED_APP"},{1000,payload},{1010,"1"},{1020,"2"},{0,"ENDSEC"},{0,"EOF"}});
        if(!write(path,tags,binary))return fail(__LINE__);
        RenderingFailureProbe reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||!reader.good||!dx_iface::renderingModelReport(data).modeledStateMatched)return fail(__LINE__);
        const auto before=symbolFailedAllocations;dx_iface::RenderingModelReport report;
        {SymbolAllocationFailure none(0);report=dx_iface::renderingModelReport(data);}
        if(symbolFailedAllocations!=before||!report.modeledStateMatched||report.propertyCoverageMatched||report.originalSourcesMatched||report.outputEligible)return fail(__LINE__);++queries;
        unsigned untouched=0u;
        for(long offset=0;offset<256;++offset) {
            auto row=A::renderingReplayRow(data);A::resetRenderingReplay(data);
            const auto failedBefore=symbolFailedAllocations;
            {SymbolAllocationFailure fault(offset);A::replayRendering(reader,data,row);}
            const bool failed=symbolFailedAllocations!=failedBefore;const auto observed=dx_iface::renderingModelReport(data);
            if(!dx_iface::renderingInputReport(data).publicationInputsCaptured)return fail(__LINE__,offset);
            if(failed) {
                if(observed.state!=S::CaptureFailed||observed.modeledStateMatched||observed.originalSourcesMatched||observed.outputEligible)return fail(__LINE__,offset);
                ++reached;untouched=0u;
            }else{
                if(!row.replayDecoded||!row.modeledStateMatched||!row.replayedModel||!observed.modeledStateMatched)return fail(__LINE__,offset);
                ++unreached;++untouched;
            }
            A::repeatRenderingModels(reader,data);
            if(!dx_iface::renderingModelReport(data).modeledStateMatched)return fail(__LINE__,offset);++retries;
            if(untouched==2u)break;
            if(offset==255)return fail(__LINE__,offset);
        }
    }
    std::cout<<"Rendering model allocation profiles=4; immutable queries="<<queries<<"; reached failures="<<reached
        <<"; unreached offsets="<<unreached<<"; corrected finalizations="<<retries<<'\n';
    return reached!=0u&&unreached==8u&&retries==reached+unreached;
}
