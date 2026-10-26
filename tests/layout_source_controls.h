// SPDX-License-Identifier: GPL-2.0-or-later
// Runtime-generated, locally authored fields, not external CAD fixtures.
Tags layoutSourceBody(const char* id,const char* block) {
    return {{0,"LAYOUT"},{5,id},{330,"D0"},{102,"{ACAD_REACTORS"},{330,"F8"},{102,"}"},
        {102,"{OPAQUE"},{330,"FFFFFFFFFFFFFFFF"},{100,"AcDbLayout"},{1,"not the name"},{102,"}"},
        {100,"AcDbPlotSettings"},{1,"page-é"},{2,"printer-é"},{4,"paper-é"},{6,"view-é"},{7,"styles-é"},
        {40,"1.25"},{41,"2.25"},{42,"3.25"},{43,"4.25"},{44,"210"},{45,"297"},{46,"5.25"},{47,"6.25"},
        {48,"7.25"},{49,"8.25"},{140,"9.25"},{141,"10.25"},{142,"11.25"},{143,"12.25"},
        {70,"48"},{72,"1"},{73,"2"},{74,"4"},{75,"17"},{76,"3"},{77,"5"},{78,"600"},
        {147,"0.125"},{148,"13.25"},{149,"14.25"},{333,"D6"},
        {100,"AcDbLayout"},{1,std::string("layout-")+id},{70,"3"},{71,"2"},
        {10,"-0.0"},{20,"15.25"},{11,"16.25"},{21,"17.25"},{12,"18.25"},{22,"19.25"},{32,"20.25"},
        {13,"21.25"},{23,"22.25"},{33,"23.25"},{14,"24.25"},{24,"25.25"},{34,"26.25"},
        {15,"27.25"},{25,"28.25"},{35,"29.25"},{16,"0.5"},{26,"0.25"},{36,"0.125"},
        {17,"0.125"},{27,"0.5"},{37,"0.25"},{146,"30.25"},{76,"4"},
        {330,block},{331,"D3"},{345,"D4"},{346,"D5"},{333,"D6"},
        {1001,"LAYOUT_APP"},{1003,"LAYOUT_LAYER"},{1005,"D7"},{1004,"A5FF"},{1010,"1.25"},{1020,"2.25"},{1030,"3.25"}};
}
Tags layoutSourceDocument(const std::string& version) {
    Tags t{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{9,"$DWGCODEPAGE"},{3,"ANSI_932"},{0,"ENDSEC"},
        {0,"SECTION"},{2,"OBJECTS"},{0,"DICTIONARY"},{5,"C"},{330,"0"},{100,"AcDbDictionary"},{281,"1"}};
    for(const auto& body:{layoutSourceBody("A100","B100"),layoutSourceBody("A101","B101")})t.insert(t.end(),body.begin(),body.end());
    t.insert(t.end(),{{0,"ENDSEC"},{0,"EOF"}});return t;
}
struct LayoutSourceProbe final:dx_iface {
    bool enabled=true,skipSource=false,skipTyped=false,sourceCopy=false,typedCopy=false,duplicate=false,phaseGood=true,premature=false;
    int throwPhase=0;long fault=-1;unsigned sources=0u,typed=0u,rawLayouts=0u;
    std::array<std::size_t,2> captured{};
    std::function<std::function<void()>(DRW_Layout&)> change;
    bool wantsDxfLayoutSource() const override {return enabled&&dx_iface::wantsDxfLayoutSource();}
    void addRawDxfObject(const DRW_RawDxfObject& raw) override {if(raw.name=="LAYOUT")++rawLayouts;dx_iface::addRawDxfObject(raw);}
    void addDxfLayoutSource(const DRW_Layout& value,const DRW_RawDxfObject& raw) override {
        ++sources;dxfRW::DxfLayoutSourceView view;const auto* input=DxIfaceTableSourceTestAccess::readInput(*cData);
        phaseGood=phaseGood&&input&&input->getDxfLayoutSourceView(view)&&view.phase==dxfRW::DxfLayoutSourceView::Phase::Source&&view.model==&value&&view.source==&raw;
        if(layoutSourceReport(*cData).state==LayoutSourceState::ProofsReady)premature=true;
        if(throwPhase==1)throw std::runtime_error("layout source before");
        auto undo=change?change(const_cast<DRW_Layout&>(value)):std::function<void()>{};
        if(!skipSource) {
            SymbolAllocationFailure schedule(fault);
            if(sourceCopy){auto copy=raw;dx_iface::addDxfLayoutSource(value,copy);}else dx_iface::addDxfLayoutSource(value,raw);
            if(duplicate)dx_iface::addDxfLayoutSource(value,raw);
        }
        if(undo)undo();if(throwPhase==2)throw std::runtime_error("layout source after");
    }
    void addLayout(const DRW_Layout& value) override {
        ++typed;dxfRW::DxfLayoutSourceView view;const auto* input=DxIfaceTableSourceTestAccess::readInput(*cData);
        const bool active=input&&input->getDxfLayoutSourceView(view);
        phaseGood=phaseGood&&(enabled?(active&&view.phase==dxfRW::DxfLayoutSourceView::Phase::Typed&&view.model==&value&&view.source):!active);
        auto undo=change?change(const_cast<DRW_Layout&>(value)):std::function<void()>{};
        if(!skipTyped){if(typedCopy){DRW_Layout copy(value);dx_iface::addLayout(copy);}else dx_iface::addLayout(value);}
        if(undo)undo();captured=DxIfaceTableSourceTestAccess::layoutAllowance(*cData);
        if(throwPhase==3)throw std::runtime_error("layout typed after");
    }
};
bool layoutSourceWire(const std::filesystem::path& path,bool binary,const dx_data& data,std::size_t& spans,unsigned& roles) {
    const auto wire=attdefWireTags(path,binary);
    for(const auto& entry:DxIfaceTableSourceTestAccess::layoutAudits(data)) {
        const auto& audit=*entry.second;const auto& raw=*audit.input->raw;if(!audit.profile.complete())return false;
        auto at=std::find_if(wire.begin(),wire.end(),[&](const auto& w){return w.code==5&&std::stoul(w.text,nullptr,16)==entry.first;});
        if(at==wire.end())return false;const auto first=static_cast<std::size_t>(at-wire.begin());
        if(raw.groups.size()!=audit.profile.spans.size())return false;
        for(std::size_t j=0;j<raw.groups.size();++j) {
            if(first+j>=wire.size())return false;const auto& w=wire[first+j];const auto& v=raw.groups[j];const auto& p=audit.profile.spans[j];
            if(w.code!=v.code()||p.code!=w.code||p.groupIndex!=j)return false;
            if(w.kind==AttdefWireTag::Real){const auto d=v.d_val();if(v.type()!=DRW_Variant::DOUBLE||std::memcmp(&d,&w.real,sizeof(d)))return false;}
            else if(w.kind==AttdefWireTag::Integer){if(w.integer!=v.i_val())return false;}
            else if(w.text!=dx_attdef_source::text(v))return false;
            using R=dx_layout_source::Role;
            if(p.role==R::Self||p.role==R::Owner||p.role==R::BlockRecord||p.role==R::Viewport||p.role==R::ShadePlot||p.role==R::NamedUcs
                ||p.role==R::BaseUcs||p.role==R::Reactor||p.role==R::OpaqueHandle||p.role==R::XdataObjectId) {
                if(std::stoull(w.text,nullptr,16)!=p.handle)return false;++roles;
            }++spans;
        }
    }
    return true;
}
bool layoutPublicationViewControls(const std::filesystem::path& path,unsigned& contexts) {
    struct Consumer final:dx_iface {
        dxfRW* input=nullptr;bool enabled=true,disable=false,good=true;int throwPhase=0;unsigned source=0u,typed=0u;bool capturing=false;
        bool wantsDxfLayoutSource() const override {return enabled;}
        void addHeader(const DRW_Header*) override {}
        void addNamedObjectDictionary(const DRW_Dictionary&) override {}
        void addRawDxfObject(const DRW_RawDxfObject&) override {}
        void addDxfLayoutSource(const DRW_Layout& v,const DRW_RawDxfObject& raw) override {
            dxfRW::DxfLayoutSourceView view;good=good&&input->getDxfLayoutSourceView(view)&&view.phase==dxfRW::DxfLayoutSourceView::Phase::Source
                &&view.model==&v&&view.source==&raw;++source;capturing=true;
            if(disable)enabled=false;if(throwPhase==1)throw std::runtime_error("public source view");
        }
        void addLayout(const DRW_Layout& v) override {
            dxfRW::DxfLayoutSourceView view;const bool active=input->getDxfLayoutSourceView(view);
            good=good&&(capturing?(active&&view.phase==dxfRW::DxfLayoutSourceView::Phase::Typed&&view.model==&v&&view.source):!active);
            ++typed;capturing=false;if(throwPhase==2)throw std::runtime_error("public typed view");
        }
    } consumer;
    dxfRW reader(path.string().c_str());consumer.input=&reader;
    std::ostringstream text;for(const auto& tag:layoutSourceDocument("AC1032"))text<<tag.code<<'\n'<<tag.value<<'\n';auto document=text.str();
    for(unsigned kind=0;kind<4u;++kind) {
        consumer.enabled=kind!=3u;consumer.disable=kind==2u;consumer.source=consumer.typed=0u;consumer.capturing=false;consumer.throwPhase=kind==1u?1:0;
        dxfRW::DxfLayoutSourceView view;view.phase=dxfRW::DxfLayoutSourceView::Phase::Typed;
        if(reader.getDxfLayoutSourceView(view)||view.phase!=dxfRW::DxfLayoutSourceView::Phase::None||view.model||view.source)return false;
        bool threw=false;try{if(!reader.readAscii(&consumer,false,document))return false;}catch(const std::runtime_error&){threw=true;}
        if(threw!=(kind==1u)||!consumer.good||reader.getDxfLayoutSourceView(view)||view.phase!=dxfRW::DxfLayoutSourceView::Phase::None||view.model||view.source)return false;
        if(kind==0u&&(consumer.source!=2u||consumer.typed!=2u))return false;
        if(kind==2u&&(consumer.source!=1u||consumer.typed!=2u))return false;
        if(kind==3u&&(consumer.source||consumer.typed!=2u))return false;++contexts;
    }
    return true;
}
bool layoutSourceTests(const std::filesystem::path& dir) {
    struct Stage {const char* name="matrix";bool finished=false;~Stage(){if(!finished)std::cerr<<"LAYOUT source stage="<<name<<'\n';}} stage;
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::LayoutSourceState;
    const auto path=dir/"layout-source.dxf";unsigned profiles=0u,roles=0u,mutations=0u,refusals=0u,limits=0u,exceptions=0u,contexts=0u,modernModes=0u;std::size_t spans=0u;long allocations=0;
    const auto complete=[](const dx_data& data){const auto r=dx_iface::layoutSourceReport(data);return r.state==S::ProofsReady&&r.records==2u&&r.matched==2u&&r.paired==2u&&r.modeledMatched==2u&&r.profilesMatched==2u&&!r.outputEligible&&!r.ownerTargetsResolved;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0;encoding<3u;++encoding) {
        if(!writeBlockBinding(path,layoutSourceDocument(version),encoding))return false;LayoutSourceProbe reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||!complete(data)||!reader.phaseGood||reader.premature||reader.rawLayouts
            ||reader.sources!=2u||reader.typed!=2u||!layoutSourceWire(path,encoding==2u,data,spans,roles)) {
            const auto r=dx_iface::layoutSourceReport(data);std::cerr<<version<<'/'<<encoding<<" state="<<static_cast<int>(r.state)<<" matched="<<r.matched<<" paired="<<r.paired<<'\n';
            for(const auto& a:A::layoutAudits(data))std::cerr<<"handle="<<a.first<<" state="<<static_cast<int>(a.second->state)<<" profile="<<static_cast<int>(a.second->profile.reason)<<" group="<<a.second->profile.rejectedGroup<<'\n';
            return false;
        }
        const auto& in=*A::layoutInputs(data).at(0xA100u).input;const auto& m=*in.model;
        const std::array<double,40> decoded{{m.marginLeft,m.marginBottom,m.marginRight,m.marginTop,m.paperWidth,m.paperHeight,
            m.plotOriginX,m.plotOriginY,m.windowMinX,m.windowMinY,m.windowMaxX,m.windowMaxY,m.realWorldUnits,m.drawingUnits,
            m.scaleFactor,m.paperImageOriginX,m.paperImageOriginY,m.limMinX,m.limMinY,m.limMaxX,m.limMaxY,m.elevation,
            m.insPoint.x,m.insPoint.y,m.insPoint.z,m.ucsOrigin.x,m.ucsOrigin.y,m.ucsOrigin.z,m.extMin.x,m.extMin.y,m.extMin.z,
            m.extMax.x,m.extMax.y,m.extMax.z,m.ucsXAxis.x,m.ucsXAxis.y,m.ucsXAxis.z,m.ucsYAxis.x,m.ucsYAxis.y,m.ucsYAxis.z}};
        const std::array<double,40> expected{{1.25,2.25,3.25,4.25,210.,297.,5.25,6.25,7.25,8.25,9.25,10.25,11.25,12.25,
            0.125,13.25,14.25,-0.0,15.25,16.25,17.25,30.25,18.25,19.25,20.25,21.25,22.25,23.25,24.25,25.25,26.25,
            27.25,28.25,29.25,0.5,0.25,0.125,0.125,0.5,0.25}};
        for(std::size_t i=0;i<expected.size();++i)if(std::memcmp(&decoded[i],&expected[i],sizeof(double)))return false;
        if(std::array<int,12>{{m.plotLayoutFlags,m.paperUnits,m.plotRotation,m.plotType,m.scaleType,m.shadePlotMode,
                m.shadePlotResLevel,m.shadePlotCustomDPI,m.layoutFlags,m.tabOrder,m.orthoViewType,m.viewportCount}}
            !=std::array<int,12>{{48,1,2,4,17,3,5,600,3,2,4,0}})return false;
        if(m.printerConfig!="printer-é"||m.paperSize!="paper-é"||m.plotViewName!="view-é"||m.currentStyleSheet!="styles-é"
            ||m.shadePlotHandle.ref!=0xD6u||m.lastActiveViewportHandle.ref!=0xD3u||m.namedUcsHandle.ref!=0xD4u||m.baseUcsHandle.ref!=0xD5u
            ||m.plotViewHandle.ref||!m.viewportHandles.empty())return false;
        if(in.state.subclass!=1||dxfLayout::Decoder::observe(m).subclass!=0||m.parentHandle!=0xD0u||m.paperSpaceBlockRecordHandle.ref!=0xB100u
            ||m.pageSetupName!="page-é"||m.name!="layout-A100"||m.shadePlotMode!=3||m.orthoViewType!=4||m.plotLayoutFlags!=48||m.layoutFlags!=3
            ||m.marginLeft!=1.25||m.extMax.z!=29.25||!std::signbit(m.limMinX)||in.context->declaration!="ANSI_932")return false;
        ++profiles;
    }
    for(bool binary:{false,true}) {
        const auto good=layoutSourceDocument("AC1032");if(!write(path,good,binary))return false;
        stage.name="modern documented shade modes";
        for(const char* mode:{"4","5"}) {
            auto tags=good;for(auto& tag:tags)if(tag.code==76&&tag.value=="3")tag.value=mode;
            if(!write(path,tags,binary))return false;LayoutSourceProbe reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||!complete(data)||!layoutSourceWire(path,binary,data,spans,roles))return false;++modernModes;
        }
        if(!write(path,good,binary))return false;
        stage.name="whole modeled fields";
        for(unsigned field=0;field<66u;++field) {
            LayoutSourceProbe reader;reader.change=[field](DRW_Layout& v)->std::function<void()> {
                const std::array<double*,40> real{{&v.marginLeft,&v.marginBottom,&v.marginRight,&v.marginTop,&v.paperWidth,&v.paperHeight,
                    &v.plotOriginX,&v.plotOriginY,&v.windowMinX,&v.windowMinY,&v.windowMaxX,&v.windowMaxY,&v.realWorldUnits,&v.drawingUnits,
                    &v.scaleFactor,&v.paperImageOriginX,&v.paperImageOriginY,&v.limMinX,&v.limMinY,&v.limMaxX,&v.limMaxY,&v.elevation,
                    &v.insPoint.x,&v.insPoint.y,&v.insPoint.z,&v.ucsOrigin.x,&v.ucsOrigin.y,&v.ucsOrigin.z,
                    &v.extMin.x,&v.extMin.y,&v.extMin.z,&v.extMax.x,&v.extMax.y,&v.extMax.z,
                    &v.ucsXAxis.x,&v.ucsXAxis.y,&v.ucsXAxis.z,&v.ucsYAxis.x,&v.ucsYAxis.y,&v.ucsYAxis.z}};
                if(field<real.size()){auto* p=real[field];const auto before=*p;*p+=0.125;return [p,before]{*p=before;};}
                const std::array<int*,12> integer{{&v.plotLayoutFlags,&v.paperUnits,&v.plotRotation,&v.plotType,&v.scaleType,&v.shadePlotMode,
                    &v.shadePlotResLevel,&v.shadePlotCustomDPI,&v.layoutFlags,&v.tabOrder,&v.orthoViewType,&v.viewportCount}};
                if(field<52u){auto* p=integer[field-40u];const auto before=*p;++*p;return [p,before]{*p=before;};}
                const std::array<UTF8STRING*,6> text{{&v.pageSetupName,&v.printerConfig,&v.paperSize,&v.plotViewName,&v.currentStyleSheet,&v.name}};
                if(field<58u){auto* p=text[field-52u];const auto before=*p;*p+="-edited";return [p,before]{*p=before;};}
                const std::array<dwgHandle*,6> handles{{&v.plotViewHandle,&v.shadePlotHandle,&v.paperSpaceBlockRecordHandle,&v.lastActiveViewportHandle,&v.baseUcsHandle,&v.namedUcsHandle}};
                if(field<64u){auto* p=handles[field-58u];const auto before=*p;++p->ref;return [p,before]{*p=before;};}
                if(field==64u){v.viewportHandles.push_back(0xF0u);return [&v]{v.viewportHandles.pop_back();};}
                v.reactorHandles.push_back(0xF1u);return [&v]{v.reactorHandles.pop_back();};
            };
            dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;
            const auto r=dx_iface::layoutSourceReport(data);
            if(r.state!=S::Unqualified||r.modeledMatched||r.matched) {std::cerr<<"modeled field="<<field<<" state="<<static_cast<int>(r.state)<<'\n';return false;}
            reader.change={};dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++mutations;
        }
        stage.name="source profile refusals";
        for(unsigned kind=0;kind<7u;++kind) {
            auto tags=good;
            if(kind==0u)appendBlockFields(tags,"A100",{{90,"17"}});
            if(kind==1u) {const auto at=slot(tags,1,"layout-A100");tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at+1u),{1,"layout-A100"});}
            if(kind==2u) {const auto at=slot(tags,100,"AcDbPlotSettings");tags[at].value="ForeignPlot";}
            if(kind==3u) {const auto at=slot(tags,1,"layout-A100");tags[at].value="";}
            if(kind==4u) {const auto at=slot(tags,330,"B100");tags[at].value="0";}
            if(kind==5u) {const auto name=slot(tags,1,"layout-A100");
                auto shade=name;while(tags[shade].code!=333)++shade;tags[shade].value="D8";}
            if(kind==6u) {auto at=slot(tags,5,"A100");while(tags[at].code!=0)++at;tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at-1u));}
            if(!write(path,tags,binary))return false;LayoutSourceProbe reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::layoutSourceReport(data).state!=S::Unqualified
                ||dx_iface::layoutSourceReport(data).matched!=1u) {std::cerr<<"source profile="<<kind<<" state="<<static_cast<int>(dx_iface::layoutSourceReport(data).state)<<'\n';return false;}
            if(!write(path,good,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++refusals;
        }
        stage.name="publication context controls";
        for(unsigned kind=0;kind<6u;++kind) {
            LayoutSourceProbe reader;reader.enabled=kind!=0u;reader.skipSource=kind==1u;reader.skipTyped=kind==2u;
            reader.sourceCopy=kind==3u;reader.typedCopy=kind==4u;reader.duplicate=kind==5u;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::layoutSourceReport(data).state!=(kind?S::CaptureFailed:S::NotCaptured))return false;
            reader.enabled=true;reader.skipSource=reader.skipTyped=reader.sourceCopy=reader.typedCopy=reader.duplicate=false;
            dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++contexts;
        }
        if(!write(path,good,binary))return false;LayoutSourceProbe baseline;dx_data base;if(!baseline.fileImport(path.string(),&base,false)||!complete(base))return false;
        stage.name="logical limits";const auto measured=dx_iface::layoutSourceReport(base);
        const std::array<std::size_t,4> cap{{measured.records,measured.groups,measured.bytes,measured.work}};
        for(unsigned which=0;which<4u;++which)for(bool below:{false,true}) {
            LayoutSourceProbe reader;dx_data data;A::layoutLimits(data,which,cap[which]-(below?1u:0u));
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::layoutSourceReport(data).state!=(below?S::CaptureFailed:S::ProofsReady)) {
                std::cerr<<"limit="<<which<<'/'<<below<<" state="<<static_cast<int>(dx_iface::layoutSourceReport(data).state)<<'\n';return false;
            }++limits;
        }
        stage.name="isolated audit allocation faults";bool calibrated=false;
        for(long failure=0;failure<2000&&!calibrated;++failure) {
            {SymbolAllocationFailure schedule(failure);A::repeatLayoutAudit(baseline,base,baseline.captured);}
            const auto state=dx_iface::layoutSourceReport(base).state;
            if(state==S::ProofsReady)calibrated=true;else {if(state!=S::CaptureFailed)return false;++allocations;}
        }
        if(!calibrated||!complete(base))return false;
        stage.name="source capture allocation faults";calibrated=false;
        for(long failure=0;failure<300&&!calibrated;++failure) {
            LayoutSourceProbe reader;reader.fault=failure;dx_data data;
            if(!reader.fileImport(path.string(),&data,false))return false;
            if(complete(data))calibrated=true;
            else {if(dx_iface::layoutSourceReport(data).state!=S::CaptureFailed)return false;++allocations;
                reader.fault=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;}
        }
        if(!calibrated)return false;
        stage.name="immutable cache and instance owners";
        auto& audit=A::mutableLayoutAudits(base).at(0xA100u);const auto owner=audit;
        audit=std::make_shared<const std::decay_t<decltype(*owner)>>(*owner);
        if(dx_iface::layoutSourceReport(base).state!=S::ContextMismatch)return false;audit=owner;++contexts;
        audit=std::shared_ptr<const std::decay_t<decltype(*owner)>>(owner.get(),[](const auto*){});
        if(dx_iface::layoutSourceReport(base).state!=S::ContextMismatch)return false;audit=owner;++contexts;
        dx_data copy;A::copyBindingContext(copy,base);if(dx_iface::layoutSourceReport(copy).state!=S::ContextMismatch)return false;++contexts;
        base.layouts[0].name="live edit";base.layouts[0].paperSpaceBlockRecordHandle.ref=0xFFFFu;if(!complete(base))return false;++contexts;
        stage.name="callback exceptions";
        for(int phase=1;phase<=3;++phase) {
            LayoutSourceProbe reader;reader.throwPhase=phase;dx_data data;bool threw=false;
            try{(void)reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
            if(!threw||dx_iface::layoutSourceReport(data).state!=S::ImportFailed||A::readInput(data))return false;
            reader.throwPhase=0;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++exceptions;
        }
        stage.name="late read failure";
        auto broken=good;broken.erase(broken.end()-2);if(!write(path,broken,binary))return false;LayoutSourceProbe reader;dx_data failed;
        if(reader.fileImport(path.string(),&failed,false)||dx_iface::layoutSourceReport(failed).state!=S::ImportFailed||!A::layoutAudits(failed).empty())return false;
        if(!write(path,good,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++exceptions;
        stage.name="native reserved handle widths";
        for(bool dictionary:{false,true}) {
            auto tags=good;
            if(dictionary) {const auto at=slot(tags,100,"AcDbPlotSettings");tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{102,"{ACAD_XDICTIONARY"},{360,"FFFFFFFFFFFFFFFF"},{102,"}"}});}
            else tags[slot(tags,330,"F8")].value="FFFFFFFFFFFFFFFF";
            if(!write(path,tags,binary))return false;LayoutSourceProbe native;dx_data failedNative;
            if(native.fileImport(path.string(),&failedNative,false)||dx_iface::layoutSourceReport(failedNative).state!=S::ImportFailed)return false;
            if(!write(path,good,binary))return false;dx_data retryNative;if(!native.fileImport(path.string(),&retryNative,false)||!complete(retryNative))return false;++exceptions;
        }
    }
    stage.name="public view lifetime and latched selection";if(!layoutPublicationViewControls(path,contexts))return false;
    stage.finished=true;
    std::cout<<"LAYOUT profiles="<<profiles<<" scalar spans="<<spans<<" roles="<<roles<<" mutations="<<mutations<<" refusals="<<refusals
        <<" limits="<<limits<<" allocations="<<allocations<<" exceptions="<<exceptions<<" contexts="<<contexts<<" modern mode variants="<<modernModes<<'\n';
    return profiles==12u&&spans>0u&&roles>0u&&mutations==132u&&refusals==14u&&limits==16u&&allocations>0&&contexts==24u&&exceptions==12u&&modernModes==4u;
}
