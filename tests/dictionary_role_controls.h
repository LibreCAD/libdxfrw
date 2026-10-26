// SPDX-License-Identifier: GPL-2.0-or-later
// Locally authored runtime drawings only; source graph facts are not native
// name collation, entity-space, output or external CAD/kernel qualification.
struct DictionarySourceProbe final:dx_iface {
    bool enabled=true,good=true,rawEqualCopy=false,forged=false;int skip=-1,copy=-1,duplicate=-1,throwAt=-1,allocationAt=-1;
    long allocation=-1;unsigned typed=0u,raw=0u,sources=0u;std::vector<int> order;
    bool wantsDxfDictionarySource() const override {return enabled&&dx_iface::wantsDxfDictionarySource();}
    void check(const DRW_TableEntry* model,const DRW_RawDxfObject* body,int phase) {
        const auto* input=DxIfaceTableSourceTestAccess::readInput(*cData);dxfRW::DxfStructuralSourceView view;
        const bool active=input&&input->getDxfStructuralSourceView(view);
        if(enabled?(!active||view.kind!=dxfRW::DxfStructuralSourceView::Kind::Dictionary
            ||view.phase!=(phase==0?dxfRW::DxfStructuralSourceView::Phase::Typed:phase==1?dxfRW::DxfStructuralSourceView::Phase::Raw:dxfRW::DxfStructuralSourceView::Phase::Source)
            ||(model&&view.model!=model)||(body&&view.source!=body)):active)good=false;
        if(layoutRoleReport(*cData).rolesMatched)good=false;order.push_back(phase);
    }
    template<class F>void call(int phase,F action) {
        if(skip==phase)return;
        if(allocationAt==phase){SymbolAllocationFailure failure(allocation);action();}else action();
        if(duplicate==phase)action();if(throwAt==phase)throw std::runtime_error("dictionary publication callback");
    }
    void addDictionary(const DRW_Dictionary& value) override {
        ++typed;check(&value,nullptr,0);if(forged)++const_cast<DRW_Dictionary&>(value).cloning;
        call(0,[&]{if(copy==0){const auto other=value;dx_iface::addDictionary(other);}else dx_iface::addDictionary(value);});
    }
    void addRawDxfObject(const DRW_RawDxfObject& value) override {
        if(value.name!="DICTIONARY"){dx_iface::addRawDxfObject(value);return;}
        ++raw;check(nullptr,&value,1);
        call(1,[&]{if(rawEqualCopy||forged){auto other=value;if(forged)for(std::size_t i=0;i<other.groups.size();++i)if(other.groups[i].code()==281){other.groups[i]=DRW_Variant(281,std::int32_t(1));if(other.hasRawValues)other.rawValues[i]="1";}dx_iface::addRawDxfObject(other);}else dx_iface::addRawDxfObject(value);});
    }
    void addDxfDictionarySource(const DRW_Dictionary& value,const DRW_RawDxfObject& source) override {
        ++sources;check(&value,&source,2);const auto* input=DxIfaceTableSourceTestAccess::readInput(*cData);bool delivered=false;
        if(!input||!input->getDxfDictionaryRawDelivery(delivered)||delivered!=(value.handle!=0xCu&&value.handle!=0xDu))good=false;
        call(2,[&]{if(copy==2){const auto other=source;dx_iface::addDxfDictionarySource(value,other);}else dx_iface::addDxfDictionarySource(value,source);});
    }
};
Tags dictionarySourceDocument(const std::string& version,const char* handle="43") {
    auto tags=structuralSourceDocument(version);appendBlockFields(tags,"40",{{3,"ACAD_LAYOUT"},{350,handle}});
    const auto at=slot(tags,0,"XRECORD");const Tags body{{0,"DICTIONARY"},{5,handle},{330,"40"},{100,"AcDbDictionary"},{280,"0"},{281,"0"},{3,"Model"},{350,"A0"},{3,"Sheet 1"},{360,"A1"}};
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),body.begin(),body.end());return tags;
}
bool dictionaryPublicLifetime(unsigned& contexts) {
    struct Consumer final:dx_iface {
        dxfRW* input=nullptr;bool enabled=true,disable=false,good=true;mutable bool selected=false;int throwAt=-1;std::vector<int> order;
        bool wantsDxfDictionarySource() const override {selected=enabled;return enabled;}
        void addHeader(const DRW_Header*) override {}
        void addNamedObjectDictionary(const DRW_Dictionary&) override {}
        void check(const DRW_TableEntry* model,const DRW_RawDxfObject* raw,int phase) {
            dxfRW::DxfStructuralSourceView v;const bool active=input->getDxfStructuralSourceView(v);
            if(selected?(!active||v.kind!=dxfRW::DxfStructuralSourceView::Kind::Dictionary
                ||v.phase!=(phase==0?dxfRW::DxfStructuralSourceView::Phase::Typed:phase==1?dxfRW::DxfStructuralSourceView::Phase::Raw:dxfRW::DxfStructuralSourceView::Phase::Source)
                ||(model&&v.model!=model)||(raw&&v.source!=raw)):active)good=false;
            bool delivered=true;const bool delivery=input->getDxfDictionaryRawDelivery(delivered);
            if(phase==2) {if(!delivery||delivered!=(model->handle!=0xDu))good=false;}
            else if(delivery||delivered)good=false;
            if(selected)order.push_back(phase);if(phase==0&&disable)enabled=false;
            if(phase==throwAt)throw std::runtime_error("public dictionary publication");
        }
        void addDictionary(const DRW_Dictionary& row) override {check(&row,nullptr,0);}
        void addRawDxfObject(const DRW_RawDxfObject& raw) override {check(nullptr,&raw,1);}
        void addDxfDictionarySource(const DRW_Dictionary& row,const DRW_RawDxfObject& raw) override {check(&row,&raw,2);}
    } consumer;
    dxfRW reader("unused");consumer.input=&reader;
    std::string document="0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1032\n0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n0\nDICTIONARY\n5\n40\n330\n0\n100\nAcDbDictionary\n0\nDICTIONARY\n5\n43\n330\n40\n100\nAcDbDictionary\n0\nDICTIONARY\n5\nD\n330\n40\n100\nAcDbDictionary\n0\nENDSEC\n0\nEOF";
    for(unsigned kind=0u;kind<6u;++kind) {
        consumer.enabled=kind!=5u;consumer.disable=kind==4u;consumer.throwAt=kind>=1u&&kind<=3u?static_cast<int>(kind)-1:-1;consumer.order.clear();
        dxfRW::DxfStructuralSourceView view;bool delivered=true;
        if(reader.getDxfStructuralSourceView(view)||reader.getDxfDictionaryRawDelivery(delivered)||delivered)return false;
        bool threw=false;try{if(!reader.readAscii(&consumer,false,document))return false;}catch(const std::runtime_error&){threw=true;}
        delivered=true;if(threw!=(kind>=1u&&kind<=3u)||!consumer.good||reader.getDxfStructuralSourceView(view)
            ||view.model||view.source||reader.getDxfDictionaryRawDelivery(delivered)||delivered)return false;
        if(kind==0u&&consumer.order!=std::vector<int>{0,1,2,0,2})return false;
        if(kind==4u&&consumer.order!=std::vector<int>{0,1,2})return false;
        if(kind==5u&&!consumer.order.empty())return false;++contexts;
    }
    return true;
}
bool dictionarySourceTests(const std::filesystem::path& dir) {
    struct Stage {const char* name="matrix";bool done=false;~Stage(){if(!done)std::cerr<<"Dictionary source stage="<<name<<'\n';}} stage;
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::StructuralSourceState;const auto path=dir/"dictionary-source.dxf";
    unsigned profiles=0u,fixed=0u,refusals=0u,contexts=0u,exceptions=0u;long allocations=0u;
    const auto complete=[](const dx_data& data){const auto r=dx_iface::structuralSourceReport(data);return r.state==S::OriginsBound&&r.records==4u&&r.paired==4u&&r.originsBound==4u&&!r.outputEligible&&!r.ownerTargetsResolved;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding) {
        auto tags=dictionarySourceDocument(version);if(encoding==2u)tags.erase(std::remove_if(tags.begin(),tags.end(),[](const auto& t){return t.code==999;}),tags.end());
        if(!writeBlockBinding(path,tags,encoding))return false;DictionarySourceProbe reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||!complete(data)||!reader.good||reader.order!=std::vector<int>{0,1,2})return false;
        const auto& input=*A::structuralInputs(data).at(0x43u);if(input.kind!=dxfRW::DxfStructuralSourceView::Kind::Dictionary||input.raw.get()!=A::resources(data).at(0x43u).raw.get())return false;++profiles;
    }
    for(bool binary:{false,true}) {
        const auto good=dictionarySourceDocument("AC1032");
        stage.name="fixed raw suppression and opt-out";
        for(const char* handle:{"C","D"}) {
            if(!write(path,dictionarySourceDocument("AC1032",handle),binary))return false;DictionarySourceProbe reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||!complete(data)||!reader.good||reader.raw||reader.order!=std::vector<int>{0,2})return false;++fixed;
        }
        if(!write(path,good,binary))return false;
        {DictionarySourceProbe reader;reader.enabled=false;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!reader.good||reader.sources||reader.raw!=1u
            ||dx_iface::structuralSourceReport(data).records!=3u)return false;
            ++contexts;}
        {DictionarySourceProbe reader;reader.rawEqualCopy=true;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!complete(data))return false;++contexts;}
        stage.name="missing/copy/duplicate phases";
        for(unsigned mode=0u;mode<3u;++mode)for(int phase=0;phase<3;++phase) {
            if(mode==1u&&phase==1)continue;DictionarySourceProbe reader;if(mode==0u)reader.skip=phase;else if(mode==1u)reader.copy=phase;else reader.duplicate=phase;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::structuralSourceReport(data).state!=S::CaptureFailed) {
                const auto r=dx_iface::structuralSourceReport(data);std::cerr<<"mode="<<mode<<" phase="<<phase<<" state="<<static_cast<int>(r.state)<<" paired="<<r.paired<<" bodies="<<r.bodyMatched<<'\n';return false;
            }
            reader.skip=reader.copy=reader.duplicate=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++refusals;
        }
        stage.name="consistent forged typed/raw arguments";
        {DictionarySourceProbe reader;reader.forged=true;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!A::resources(data).at(0x43u).rawTypedMatched
            ||dx_iface::structuralSourceReport(data).state!=S::Unqualified||A::structuralInputs(data).at(0x43u)->paired) {
                std::cerr<<"forged binary="<<binary<<" body="<<A::resources(data).at(0x43u).rawTypedMatched<<" state="<<static_cast<int>(dx_iface::structuralSourceReport(data).state)<<'\n';return false;
            }
            reader.forged=false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++refusals;}
        stage.name="allocation recovery";
        for(int phase=0;phase<3;++phase) {
            bool recovered=false;for(long count=0;count<2000;++count) {
                DictionarySourceProbe reader;reader.allocationAt=phase;reader.allocation=count;dx_data data;
                if(!reader.fileImport(path.string(),&data,false))return false;const auto r=dx_iface::structuralSourceReport(data);
                if(r.state==S::OriginsBound){recovered=true;break;}if(r.state!=S::CaptureFailed&&r.state!=S::Unqualified)return false;
                reader.allocationAt=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++allocations;
            }if(!recovered)return false;
        }
        stage.name="exception and failed tail recovery";
        for(int phase=0;phase<4;++phase) {
            DictionarySourceProbe reader;reader.throwAt=phase<3?phase:-1;auto tags=good;if(phase==3)tags.pop_back();if(!write(path,tags,binary))return false;
            dx_data data;bool threw=false,read=false;try{read=reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
            if(read||threw!=(phase<3)||dx_iface::structuralSourceReport(data).state!=S::ImportFailed||!A::structuralAudits(data).empty())return false;
            reader.throwAt=-1;if(!write(path,good,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++exceptions;
        }
    }
    if(!dictionaryPublicLifetime(contexts))return false;
    std::cout<<"Dictionary source profiles="<<profiles<<" fixed suppressions="<<fixed<<" refusals="<<refusals<<" contexts="<<contexts<<" allocation faults="<<allocations<<" exceptions="<<exceptions<<'\n';stage.done=true;return true;
}
Tags layoutRoleDocument(const std::string& version,const char* dictionary="43") {
    Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"},{0,"SECTION"},{2,"TABLES"},
        {0,"TABLE"},{2,"BLOCK_RECORD"},{5,"8"},{100,"AcDbSymbolTable"},{70,"2"}};
    for(const char* id:{"B0","B1"})tags.insert(tags.end(),{{0,"BLOCK_RECORD"},{5,id},{330,"8"},{100,"AcDbSymbolTableRecord"},
        {2,std::string(id)=="B0"?"*Model_Space":"*Paper_Space"},{100,"AcDbBlockTableRecord"},{70,"0"},{280,"1"},{281,"0"},{340,std::string(id)=="B0"?"A0":"A1"}});
    tags.insert(tags.end(),{{0,"ENDTAB"},{0,"ENDSEC"},{0,"SECTION"},{2,"OBJECTS"},{0,"DICTIONARY"},{5,"40"},{330,"0"},{100,"AcDbDictionary"},{281,"1"},{3,"ACAD_LAYOUT"},{350,dictionary},
        {0,"DICTIONARY"},{5,dictionary},{330,"40"},{100,"AcDbDictionary"},{281,"1"},{3,"Model"},{350,"A0"},{3,"Sheet 1"},{360,"A1"}});
    for(const char* id:{"A0","A1"})tags.insert(tags.end(),{{0,"LAYOUT"},{5,id},{330,dictionary},{100,"AcDbPlotSettings"},{70,"0"},{76,"0"},
        {100,"AcDbLayout"},{1,std::string(id)=="A0"?"Model":"Sheet 1"},{70,"0"},{71,std::string(id)=="A0"?"0":"1"},{330,std::string(id)=="A0"?"B0":"B1"}});
    tags.insert(tags.end(),{{0,"ENDSEC"},{0,"SECTION"},{2,"ENTITIES"},{0,"ENDSEC"},{0,"EOF"}});return tags;
}
bool blockRecordOpaqueScopeTests(const std::filesystem::path& path,unsigned& controls) {
    for(bool binary:{false,true})for(bool previous:{false,true})for(unsigned mode=0u;mode<4u;++mode) {
        Tags tags{{102,"{OPAQUE"},{330,"FFFFFFFFFFFFFFFF"}};
        if(mode==0u)tags.insert(tags.end(),{{102,"{NESTED"},{340,"FFFFFFFF00000001"},{5,"not-a-self-handle"},{102,"}"},{102,"}"},{330,"1234"}});
        if(mode==2u)tags.push_back({102,"invalid-control"});
        if(!write(path,tags,binary))return false;
        std::ifstream input(path,std::ios::binary);if(binary)input.seekg(22);
        std::unique_ptr<dxfReader> reader=binary?std::unique_ptr<dxfReader>(new dxfReaderBinary(&input)):
            std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
        reader->setVersion("AC1032",true);reader->setAllowWideHandleLexemes(previous);
        DRW_Block_Record row;bool rejected=false,threw=false;
        try {
            dxfBlockRecord::Decoder decoder(row,reader);
            for(std::size_t i=0;i<tags.size();++i) {
                int code=0;if(!reader->readRec(&code))return false;
                if(!decoder.parseCode(code,reader)){rejected=true;break;}
                if(i==0u&&!reader->allowsWideHandleLexemes())return false;
                if(mode==3u&&i==1u)throw std::runtime_error("opaque decoder unwind");
            }
            if(mode==0u&&(decoder.inApplication()||row.parentHandle!=0x1234u||reader->allowsWideHandleLexemes()!=previous))return false;
            if(mode==1u&&(!decoder.inApplication()||!decoder.reset()||decoder.inApplication()||reader->allowsWideHandleLexemes()!=previous))return false;
        }catch(const std::runtime_error&){threw=true;}
        if(rejected!=(mode==2u)||threw!=(mode==3u)||reader->allowsWideHandleLexemes()!=previous)return false;
        ++controls;
    }
    return true;
}
bool layoutRoleTests(const std::filesystem::path& dir) {
    struct Stage {const char* name="matrix";bool done=false;~Stage(){if(!done)std::cerr<<"Layout roles stage="<<name<<'\n';}} stage;
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::LayoutRoleState;const auto path=dir/"layout-roles.dxf";
    unsigned profiles=0u,refusals=0u,quotas=0u,contexts=0u,variants=0u,opaque=0u,nativeRefusals=0u,outputs=0u;long allocations=0u;
    const auto complete=[](const dx_data& d){const auto r=dx_iface::layoutRoleReport(d);return r.state==S::Matched&&r.rolesMatched&&r.layouts==2u&&r.blocks==2u&&r.associations==2u
        &&r.root==0x40u&&r.layoutDictionary==0x43u&&!r.nativeNamesQualified&&!r.ownerTargetsResolved&&!r.outputEligible;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding) {
        if(!writeBlockBinding(path,layoutRoleDocument(version),encoding))return false;dx_iface reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||!complete(data)) {const auto r=dx_iface::layoutRoleReport(data);std::cerr<<version<<'/'<<encoding<<" state="<<static_cast<int>(r.state)<<" first="<<r.firstHandle<<'\n';return false;}++profiles;
    }
    for(bool binary:{false,true}) {
        const auto good=layoutRoleDocument("AC1032");if(!write(path,good,binary))return false;dx_iface baseline;dx_data base;if(!baseline.fileImport(path.string(),&base,false)||!complete(base))return false;
        stage.name="reciprocal graph refusal/retry";
        for(unsigned kind=0u;kind<14u;++kind) {
            auto tags=good;
            if(kind==0u)tags[blockField(tags,"B0",340)].value="A1";
            if(kind==1u) {auto at=blockField(tags,"A1",330)+1u;while(tags[at].code!=330)++at;tags[at].value="B0";}
            if(kind==2u)tags[blockField(tags,"43",330)].value="41";
            if(kind==3u)tags[blockField(tags,"A0",330)].value="44";
            if(kind==4u)tags[slot(tags,350,"A0")].value="A1";
            if(kind==5u)tags[slot(tags,3,"Sheet 1")].value="Other";
            if(kind==6u)appendBlockFields(tags,"40",{{3,"ALIAS"},{350,"43"}});
            if(kind==7u)tags[slot(tags,3,"ACAD_LAYOUT")].value="MISSING_LAYOUT";
            if(kind==8u) {const auto at=slot(tags,0,"EOF");tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{0,"SECTION"},{2,"OBJECTS"},{0,"DICTIONARY"},{5,"44"},{330,"0"},{100,"AcDbDictionary"},{281,"1"},{0,"ENDSEC"}});}
            if(kind==9u)eraseBlockRecord(tags,"43");
            if(kind==10u) {const auto at=slot(tags,0,"ENDTAB");tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{0,"BLOCK_RECORD"},{5,"B2"},{330,"8"},{100,"AcDbSymbolTableRecord"},{2,"EXTRA"},{100,"AcDbBlockTableRecord"},{70,"0"},{340,"A0"}});}
            if(kind==11u)tags[slot(tags,5,"B1")].value="B3";
            if(kind==12u)tags[blockField(tags,"B0",330)].value="40";
            if(kind==13u)tags[blockField(tags,"B0",330)].value="0";
            if(!write(path,tags,binary))return false;dx_iface reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::layoutRoleReport(data).rolesMatched) {std::cerr<<"refusal="<<kind<<'\n';return false;}
            const auto state=dx_iface::layoutRoleReport(data).state;
            if((kind==8u&&state!=S::AmbiguousRoot)||(kind==10u&&state!=S::ReferenceMismatch)||(kind==12u&&state!=S::WrongOwner))return false;
            if(!write(path,good,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++refusals;
        }
        stage.name="quota boundaries and ownership contexts";
        for(bool work:{false,true})for(bool exact:{false,true}) {
            dx_iface reader;dx_data data;const auto r=dx_iface::layoutRoleReport(base);A::layoutRoleLimit(data,work,(work?r.work:r.bytes)-(exact?0u:1u));
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::layoutRoleReport(data).state!=(exact?S::Matched:S::CaptureFailed))return false;++quotas;
        }
        for(unsigned kind=0u;kind<5u;++kind) {
            dx_iface reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;
            if(kind<2u)A::substituteLayoutRole(data,kind==1u);else if(kind==2u)A::substituteStructural(data,0u);else if(kind==3u)A::substituteIdentity(data,0u);
            else {dx_data copied;A::copyBindingContext(copied,data);if(dx_iface::layoutRoleReport(copied).state!=S::ContextMismatch)return false;++contexts;continue;}
            if(dx_iface::layoutRoleReport(data).state!=S::ContextMismatch)return false;++contexts;
        }
        stage.name="isolated audit allocation recovery";
        bool recovered=false;
        for(long count=0;count<2000;++count) {
            {SymbolAllocationFailure failure(count);A::repeatLayoutRole(baseline,base);}
            const auto r=dx_iface::layoutRoleReport(base);if(r.state==S::Matched){recovered=true;break;}
            if(r.state!=S::CaptureFailed)return false;A::repeatLayoutRole(baseline,base);if(!complete(base))return false;
            dx_iface retryReader;dx_data retry;if(!retryReader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++allocations;
        }if(!recovered)return false;
        stage.name="literal names and fixed source dictionary variants";
        for(unsigned kind=0u;kind<4u;++kind) {
            auto tags=kind==2u?layoutRoleDocument("AC1032","D"):good;
            if(kind==0u){tags[slot(tags,3,"Sheet 1")].value="sheet 1";}
            if(kind==1u){tags[slot(tags,3,"Sheet 1")].value="Sheet-é";tags[slot(tags,1,"Sheet 1")].value="Sheet-é";}
            if(kind==3u)appendBlockFields(tags,"B0",{{102,"{OPAQUE"},{330,"FFFFFFFFFFFFFFFF"},{5,"opaque-not-handle"},{102,"}"}});
            if(!write(path,tags,binary))return false;dx_iface reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||dx_iface::layoutRoleReport(data).rolesMatched!=(kind!=3u)
                ||dx_iface::layoutRoleReport(data).nativeNamesQualified) {std::cerr<<"variant="<<kind<<" binary="<<binary<<'\n';return false;}
            // Compatible opaque import is not independent semantic/body proof.
            if(kind==3u&&dx_iface::structuralSourceReport(data).state!=dx_iface::StructuralSourceState::Unqualified)return false;
            ++variants;
        }
        stage.name="opaque restoration and strict native reference retry";
        for(unsigned kind=0u;kind<6u;++kind) {
            auto tags=good;Tags fields;
            if(kind==0u)fields={{102,"{OPAQUE"},{330,"FFFFFFFFFFFFFFFF"},{102,"}"},{330,"100000000"}};
            if(kind==1u)fields={{102,"{BLKREFS"},{331,"100000000"},{102,"}"}};
            if(kind==2u)fields={{102,"{ACAD_XDICTIONARY"},{360,"100000000"},{102,"}"}};
            if(kind==3u)fields={{102,"{ACAD_REACTORS"},{330,"100000000"},{102,"}"}};
            if(kind==4u)fields={{102,"{OPAQUE"},{330,"FFFFFFFFFFFFFFFF"},{102,"}"},{340,"100000000"}};
            if(kind==5u)fields={{102,"{OPAQUE"},{330,"FFFFFFFFFFFFFFFF"}};
            appendBlockFields(tags,"B0",fields);if(!write(path,tags,binary))return false;dx_iface reader;dx_data data;
            if(reader.fileImport(path.string(),&data,false)||!data.blockRecords.empty()||dx_iface::layoutRoleReport(data).state!=S::ImportFailed)return false;
            if(!write(path,good,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++nativeRefusals;
        }
        stage.name="live export/readback and reverse-alias failure";
        if(!write(path,good,binary))return false;dx_iface reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!complete(data))return false;
        const auto output=dir/(binary?"roles-binary.dxf":"roles-ascii.dxf");
        if(!reader.fileExport(output.string(),DRW::AC1032,binary,&data,false))return false;dx_iface rereader;dx_data reread;
        if(!rereader.fileImport(output.string(),&reread,false)||reread.layouts.size()!=2u)return false;++outputs;
        DRW_Block_Record extra;extra.handle=0xB2u;extra.parentHandle=0x8u;extra.name="EXTRA";extra.layoutHandle=0xA0u;data.blockRecords.push_back(extra);
        const std::string sentinel="preserve role destination";{std::ofstream out(output,std::ios::binary|std::ios::trunc);out<<sentinel;}
        std::ostringstream diagnostic;auto* previous=std::cerr.rdbuf(diagnostic.rdbuf());bool rejected=false;
        try{rejected=!reader.fileExport(output.string(),DRW::AC1032,binary,&data,true);}catch(...){std::cerr.rdbuf(previous);throw;}
        std::cerr.rdbuf(previous);
        if(!rejected||diagnostic.str().find("BLOCK_RECORD has a missing or nonreciprocal LAYOUT target")==std::string::npos)return false;
        {std::ifstream in(output,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(in),{})!=sentinel)return false;}
        data.blockRecords.pop_back();if(!reader.fileExport(output.string(),DRW::AC1032,binary,&data,false)||!complete(data))return false;++outputs;
    }
    if(!blockRecordOpaqueScopeTests(dir/"block-record-opaque.dxf",opaque))return false;
    std::cout<<"Layout roles profiles="<<profiles<<" refusals="<<refusals<<" quotas="<<quotas<<" contexts="<<contexts<<" allocation faults="<<allocations<<" variants="<<variants
        <<" opaque restoration="<<opaque<<" native refusal/retries="<<nativeRefusals<<" outputs="<<outputs<<'\n';stage.done=true;return true;
}
