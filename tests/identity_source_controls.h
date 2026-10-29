// SPDX-License-Identifier: GPL-2.0-or-later
// Runtime-authored drawings only. Namespace observations are not body/target,
// independent CAD/kernel, editable/live or output qualification.
struct IdentitySourceProbe final:dx_iface {
    bool enabled=true,copy=false,duplicate=false,throwSource=false,throwPoint=false,good=true;
    long allocation=-1;unsigned callbacks=0u;DRW_DxfIdentitySourceLimits limits;
    DRW_DxfIdentitySource::Status status{DRW_DxfIdentitySource::Status::NotStarted};
    std::size_t sourceBytes=0u,sourceWork=0u,sourceRecords=0u,sourceCandidates=0u,sourceSections=0u;
    bool wantsDxfIdentitySource() const override {return enabled&&dx_iface::wantsDxfIdentitySource();}
    DRW_DxfIdentitySourceLimits dxfIdentitySourceLimits() const override {return limits;}
    void addPoint(const DRW_Point& point) override {
        if(throwPoint)throw std::runtime_error("identity point callback");
        dx_iface::addPoint(point);
    }
    void addDxfIdentitySource(const DRW_DxfIdentitySource& source) override {
        ++callbacks;status=source.status;sourceBytes=source.bytes;sourceWork=source.work;
        sourceRecords=source.records.size();sourceCandidates=source.candidates.size();sourceSections=source.sections.size();
        const auto* input=DxIfaceTableSourceTestAccess::readInput(*cData);
        if(!input||input->getDxfIdentitySourceView()!=&source||identitySourceReport(*cData).state!=IdentitySourceState::Reading)good=false;
        if(copy) {const auto other=source;dx_iface::addDxfIdentitySource(other);}
        else if(allocation>=0) {SymbolAllocationFailure failure(allocation);dx_iface::addDxfIdentitySource(source);}
        else dx_iface::addDxfIdentitySource(source);
        if(duplicate)dx_iface::addDxfIdentitySource(source);
        if(throwSource)throw std::runtime_error("identity source callback");
    }
};
Tags identitySourceDocument(const std::string& version) {
    return {{0,"SECTION"},{2,"HEADER"},{999,"header comment"},{9,"$ACADVER"},{1,version},
        {9,"$DWGCODEPAGE"},{3,"ANSI_932"},{9,"$HANDSEED"},{5,"FFFFFFFFFFFFFFFF"},{0,"ENDSEC"},
        {0,"SECTION"},{2,"CLASSES"},{0,"ENDSEC"},{0,"SECTION"},{2,"TABLES"},
        {0,"TABLE"},{2,"LAYER"},{5,"10"},{100,"AcDbSymbolTable"},{70,"1"},
        {0,"LAYER"},{5,"11"},{330,"10"},{100,"AcDbSymbolTableRecord"},{2,"0"},{70,"0"},{100,"AcDbLayerTableRecord"},{62,"7"},{6,"CONTINUOUS"},{0,"ENDTAB"},
        {0,"TABLE"},{2,"DIMSTYLE"},{5,"12"},{100,"AcDbSymbolTable"},{70,"1"},
        {0,"DIMSTYLE"},{105,"13"},{330,"12"},{100,"AcDbSymbolTableRecord"},{2,"STANDARD"},{70,"0"},{100,"AcDbDimStyleTableRecord"},{5,"NOT_A_HANDLE"},{40,"1"},{0,"ENDTAB"},
        {0,"TABLE"},{2,"FUTURE"},{5,"14"},{100,"AcDbSymbolTable"},{70,"1"},
        {0,"FUTURE"},{5,"15"},{330,"14"},{100,"AcDbSymbolTableRecord"},{2,"future"},{70,"0"},{0,"ENDTAB"},{0,"ENDSEC"},
        {0,"SECTION"},{2,"BLOCKS"},{0,"BLOCK"},{5,"20"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},{2,"DEF"},{70,"0"},{10,"0"},{20,"0"},{30,"0"},
        {0,"POINT"},{5,"21"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"1"},{20,"2"},{30,"3"},
        {0,"ENDBLK"},{5,"22"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"},{0,"ENDSEC"},
        {0,"SECTION"},{2,"ENTITIES"},{999,"inside source"},
        {0,"POINT"},{5,"30"},{330,"B1"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"4"},{20,"5"},{30,"6"},
        {0,"LINE"},{5,"31"},{102,"{OPAQUE"},{5,"not-a-handle"},{105,"FFFFFFFFFFFFFFFE"},{102,"}"},
        {330,"B1"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbLine"},{10,"0"},{20,"0"},{11,"1"},{21,"1"},
        {0,"FUTURE_ENTITY"},{5,"32"},{330,"B1"},{100,"AcDbEntity"},{8,"0"},{100,"FutureClass"},{1,"opaque body"},{0,"ENDSEC"},
        {0,"SECTION"},{2,"OBJECTS"},{0,"DICTIONARY"},{5,"40"},{330,"0"},{100,"AcDbDictionary"},{280,"0"},{281,"1"},
        {0,"XRECORD"},{5,"41"},{330,"40"},{100,"AcDbXrecord"},{280,"1"},{1,"payload"},{0,"ENDSEC"},
        {0,"SECTION"},{2,"ENTITIES"},{0,"POINT"},{5,"33"},{330,"B1"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"7"},{20,"8"},{30,"9"},{0,"ENDSEC"},{0,"EOF"}};
}
bool identitySourceWire(const std::filesystem::path& path,bool binary,const dx_data& data,std::size_t& spans) {
    using A=DxIfaceTableSourceTestAccess;const auto& source=*A::identityInput(data)->source;
    const auto wire=attdefWireTags(path,binary);
    if(source.scalars!=wire.size()||source.ignoredComments!=(binary?0u:2u)||source.sections.size()!=7u||source.records.size()!=15u||source.candidates.size()!=18u)return false;
    const auto physical=[&](const DRW_DxfPhysicalRecordSpan& span,int code,const std::string& text) {
        if(!span.ordinal||span.ordinal>wire.size())return false;const auto& w=wire[span.ordinal-1u];
        return w.code==code&&w.text==text&&w.begin==span.beginOffset&&w.end==span.endOffset;
    };
    for(const auto& section:source.sections)if(!physical(section.marker,0,"SECTION")||!physical(section.endMarker,0,"ENDSEC"))return false;
    for(const auto& record:source.records) {
        if(!physical(record.marker,0,record.name)||!record.endMarker.ordinal||record.endMarker.ordinal>wire.size())return false;
        const auto& end=wire[record.endMarker.ordinal-1u];
        if(end.code||end.begin!=record.endMarker.beginOffset||end.end!=record.endMarker.endOffset)return false;++spans;
    }
    for(const auto& candidate:source.candidates) {
        if(!physical(candidate.span,candidate.code,candidate.lexeme)||candidate.span.ignoredComment)return false;
        if(binary?!candidate.rawValue.empty():candidate.rawValue!=candidate.lexeme)return false;++spans;
    }
    const std::array<std::uint64_t,15> expected{{0x10,0x11,0x12,0x13,0x14,0x15,0x20,0x21,0x22,0x30,0x31,0x32,0x40,0x41,0x33}};
    const auto& index=A::identityAudit(data)->identities;
    for(std::size_t i=0;i<expected.size();++i)if(index.at(expected[i])!=i)return false;
    using Source=DRW_DxfIdentitySource;
    for(const auto& c:source.candidates) {
        if(c.lexeme=="13"&&(c.code!=105||c.admission!=Source::Admission::Accepted))return false;
        if(c.lexeme=="NOT_A_HANDLE"&&(c.code!=5||c.admission!=Source::Admission::None))return false;
        if(c.lexeme=="10"||c.lexeme=="12"||c.lexeme=="14"||c.lexeme=="15")if(c.admission!=Source::Admission::None)return false;
        if(c.lexeme=="not-a-handle"||c.lexeme=="FFFFFFFFFFFFFFFE")if(c.scope!=Source::Scope::Application||c.admission!=Source::Admission::None)return false;
    }
    return physical(source.eofMarker,0,"EOF");
}
bool identityPublicLifetime(unsigned& contexts) {
    struct Consumer final:dx_iface {
        dxfRW* input=nullptr;bool enabled=true,disableAtHeader=false,throwSource=false,good=true;unsigned calls=0u;
        bool wantsDxfIdentitySource() const override {return enabled;}
        void addHeader(const DRW_Header*) override {if(disableAtHeader)enabled=false;}
        void addDxfIdentitySource(const DRW_DxfIdentitySource& source) override {
            ++calls;if(input->getDxfIdentitySourceView()!=&source||source.status!=DRW_DxfIdentitySource::Status::Complete)good=false;
            if(throwSource)throw std::runtime_error("public identity publication");
        }
    } consumer;
    dxfRW reader("unused");consumer.input=&reader;
    std::string document="999\nbefore section\n0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1032\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nENDSEC\n0\nEOF";
    for(unsigned phase=0u;phase<5u;++phase) {
        consumer.enabled=phase!=3u;consumer.disableAtHeader=phase==2u;consumer.throwSource=phase==1u;consumer.calls=0u;
        if(reader.getDxfIdentitySourceView())return false;bool threw=false;
        try{if(!reader.readAscii(&consumer,false,document))return false;}catch(const std::runtime_error&){threw=true;}
        if(threw!=(phase==1u)||reader.getDxfIdentitySourceView()||!consumer.good||consumer.calls!=(phase==3u?0u:1u))return false;++contexts;
    }
    return true;
}
#include "identity_xrecord_controls.h"
#include "xrecord_payload_identity_controls.h"
#include "imagedef_identity_controls.h"
#include "identity_sortents_controls.h"
#include "identity_dimstyle_controls.h"
bool identitySourceTests(const std::filesystem::path& dir) {
    struct Stage {const char* name="matrix";bool done=false;~Stage(){if(!done)std::cerr<<"Identity source stage="<<name<<'\n';}} stage;
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::IdentitySourceState;
    const auto path=dir/"identity-source.dxf";unsigned profiles=0u,variants=0u,refusals=0u,limits=0u,contexts=0u,exceptions=0u;std::size_t spans=0u;long allocations=0;
    if(!identityXrecordTests(path))return false;
    if(!xrecordPayloadIdentityTests(path))return false;
    if(!imageDefIdentityTests(path))return false;
    if(!identitySortentsTests(path))return false;
    if(!identityDimstyleTests(path))return false;
    const auto complete=[](const dx_data& d) {const auto r=dx_iface::identitySourceReport(d);
        return r.state==S::NamespaceReady&&r.records==15u&&r.identities==15u&&r.admitted==11u&&r.unobserved==4u
            &&r.opaque==2u&&r.dimstyleNames==1u&&!r.outputEligible&&!r.ownerTargetsResolved;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding) {
        auto tags=identitySourceDocument(version);
        if(encoding==2u)tags.erase(std::remove_if(tags.begin(),tags.end(),[](const auto& t){return t.code==999;}),tags.end());
        if(!writeBlockBinding(path,tags,encoding))return false;IdentitySourceProbe reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||!complete(data)||!reader.good||reader.callbacks!=1u||!identitySourceWire(path,encoding==2u,data,spans)) {
            const auto r=dx_iface::identitySourceReport(data);std::cerr<<version<<'/'<<encoding<<" state="<<static_cast<int>(r.state)<<" records="<<r.records<<" candidates="<<r.candidates<<" admitted="<<r.admitted<<" unobserved="<<r.unobserved<<" unknown="<<r.unknown<<'\n';return false;
        }++profiles;
    }
    for(bool binary:{false,true}) {
        const auto good=identitySourceDocument("AC1032");if(!write(path,good,binary))return false;IdentitySourceProbe baseline;dx_data base;
        if(!baseline.fileImport(path.string(),&base,false)||!complete(base))return false;
        stage.name="wide/raw/no-callback/XDATA/mixed-case/nested variants";
        for(unsigned kind=0u;kind<6u;++kind) {
            auto tags=good;
            if(kind==0u)tags[slot(tags,5,"32")].value="FFFFFFFFFFFFFFFF";
            if(kind==1u)tags[slot(tags,5,"32")].value="0000000000000032";
            if(kind==2u) {const auto at=slot(tags,5,"30")+1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{1001,"APP"},{1005,"33"}});}
            if(kind==3u) {tags[slot(tags,0,"DIMSTYLE")].value="dimstyle";tags[slot(tags,2,"DIMSTYLE")].value="dimstyle";}
            if(kind==4u) {const auto at=slot(tags,5,"not-a-handle");tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{102,"{INNER"});tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at+3u),{102,"}"});}
            if(kind==5u) {const auto at=slot(tags,0,"FUTURE_ENTITY");tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{0,"DIMENSION"},{5,"34"},{330,"B1"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbDimension"},{70,"7"}});}
            if(!write(path,tags,binary))return false;IdentitySourceProbe reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::identitySourceReport(data).state!=S::NamespaceReady) {std::cerr<<"variant="<<kind<<'\n';return false;}
            const auto& index=A::identityAudit(data)->identities;
            if(index.size()!=(kind==5u?16u:15u)||index.count(kind==0u?UINT64_MAX:0x32u)!=1u||(kind==0u&&index.count(UINT32_MAX)))return false;++variants;
        }
        if(!write(path,good,binary))return false;
        stage.name="physical namespace refusals";
        for(unsigned kind=0;kind<8u;++kind) {
            auto tags=good;
            if(kind<3u) {const auto at=slot(tags,5,"30");tags[at].value=kind==0u?"13":kind==1u?"10":"21";}
            if(kind==3u) {const auto at=slot(tags,5,"30");tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at));}
            if(kind==4u) {const auto at=slot(tags,5,"30");tags[at].value="0";}
            if(kind==5u) {const auto at=slot(tags,5,"30");tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at+1u),{5,"30"});}
            if(kind==6u) {const auto at=slot(tags,0,"EOF");tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{0,"SECTION"},{2,"FUTURE_SECTION"},{5,"F001"},{0,"ENDSEC"}});}
            if(kind==7u) {const auto at=slot(tags,5,"30");tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at+1u),{105,"F002"});}
            if(!write(path,tags,binary))return false;IdentitySourceProbe reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::identitySourceReport(data).state!=S::Unqualified) {std::cerr<<"refusal="<<kind<<'\n';return false;}
            const auto report=dx_iface::identitySourceReport(data);
            if(kind<3u&&(report.duplicates!=1u||report.duplicateAdmissions!=((kind==0u||kind==2u)?1u:0u)))return false;
            if(!write(path,good,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++refusals;
        }
        stage.name="exact and one-under quotas";
        for(unsigned which=0;which<7u;++which)for(bool exact:{false,true}) {
            IdentitySourceProbe reader;dx_data data;const auto r=dx_iface::identitySourceReport(base);
            if(which==0u)reader.limits.records=baseline.sourceRecords-(exact?0u:1u);
            if(which==1u)reader.limits.candidates=baseline.sourceCandidates-(exact?0u:1u);
            if(which==2u)reader.limits.bytes=baseline.sourceBytes-(exact?0u:1u);
            if(which==3u)reader.limits.work=baseline.sourceWork-(exact?0u:1u);
            if(which==4u||which==5u)A::identityLimit(data,which==5u,(which==5u?r.work:r.bytes)-(exact?0u:1u));
            if(which==6u)reader.limits.sections=baseline.sourceSections-(exact?0u:1u);
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::identitySourceReport(data).state!=(exact?S::NamespaceReady:S::CaptureFailed)) {std::cerr<<"quota="<<which<<'/'<<exact<<'\n';return false;}++limits;
        }
        stage.name="copy/context guards";
        for(unsigned which=0;which<3u;++which) {
            IdentitySourceProbe reader;reader.enabled=which!=0u;reader.copy=which==1u;reader.duplicate=which==2u;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::identitySourceReport(data).state!=(which?S::CaptureFailed:S::NotCaptured))return false;++contexts;
        }
        for(unsigned which=0;which<5u;++which) {
            IdentitySourceProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!complete(data))return false;
            if(which<4u)A::substituteIdentity(data,which);
            else {dx_data copy;A::copyBindingContext(copy,data);if(dx_iface::identitySourceReport(copy).state!=S::ContextMismatch)return false;++contexts;continue;}
            if(dx_iface::identitySourceReport(data).state!=S::ContextMismatch)return false;++contexts;
        }
        stage.name="source-copy allocation and audit recovery";
        for(bool audit:{false,true}) {
            bool recovered=false;
            for(long count=0;count<2000;++count) {
                IdentitySourceProbe reader;reader.allocation=audit?-1:count;dx_data data;
                if(!reader.fileImport(path.string(),&data,false))return false;
                if(audit){SymbolAllocationFailure failure(count);A::repeatIdentityAudit(reader,data);}
                const auto r=dx_iface::identitySourceReport(data);
                if(r.state==S::NamespaceReady){recovered=true;break;}
                if(r.state!=S::CaptureFailed)return false;
                if(audit){A::repeatIdentityAudit(reader,data);if(!complete(data))return false;}
                reader.allocation=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++allocations;
            }
            if(!recovered)return false;
        }
        stage.name="exceptions and failed tails";
        for(unsigned which=0;which<3u;++which) {
            IdentitySourceProbe reader;reader.throwSource=which==0u;reader.throwPoint=which==1u;
            auto tags=good;if(which==2u)tags.pop_back();if(!write(path,tags,binary))return false;
            dx_data data;bool threw=false,read=false;try{read=reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
            if(read||threw!=(which<2u)||dx_iface::identitySourceReport(data).state!=S::ImportFailed||A::identityAudit(data))return false;
            if(which!=0u&&reader.callbacks)return false;
            reader.throwSource=reader.throwPoint=false;if(!write(path,good,binary))return false;dx_data retry;
            if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++exceptions;
        }
    }
    if(!identityPublicLifetime(contexts))return false;
    std::cout<<"Identity source profiles="<<profiles<<" variants="<<variants<<" spans="<<spans<<" refusals="<<refusals<<" quotas="<<limits
        <<" contexts="<<contexts<<" allocation faults="<<allocations<<" exceptions="<<exceptions<<'\n';stage.done=true;return true;
}
