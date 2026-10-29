// SPDX-License-Identifier: GPL-2.0-or-later
// Physical identity scope is independent of XRECORD value or output support.
bool identityXrecordTests(const std::filesystem::path& path) {
    using Source=DRW_DxfIdentitySource;using Scope=Source::Scope;
    const auto check=[](bool ok,const char* message){if(!ok)std::cerr<<message<<'\n';return ok;};
    struct Probe final:dx_iface {
        Source observed;unsigned calls=0u;bool enabled=true;DRW_DxfIdentitySourceLimits limits;
        bool wantsDxfIdentitySource()const override{return enabled&&dx_iface::wantsDxfIdentitySource();}
        DRW_DxfIdentitySourceLimits dxfIdentitySourceLimits()const override{return limits;}
        void addDxfIdentitySource(const Source& value)override {
            observed=value;++calls;dx_iface::addDxfIdentitySource(value);
        }
    };
    // Feed the observer directly to exercise 64-bit opaque candidates and
    // duplicate outer identities independently of typed parser admission.
    {
        std::istringstream stream;dxfReaderAscii input(&stream);dxfIdentitySource::Capture capture(input);
        capture.start(DRW_DxfIdentitySourceLimits{});std::size_t ordinal=0u;
        const auto send=[&](int code,std::string_view value) {
            DxfReadRecordView v;v.code=code;v.kind=DxfReadRecordView::Text;v.text=value;v.raw=value;
            v.ordinal=++ordinal;v.beginOffset=ordinal*10u;v.endOffset=v.beginOffset+5u;capture.observe(v);
        };
        send(0,"SECTION");send(2,"OBJECTS");send(0,"XRECORD");send(5,"41");send(5,"41");
        send(100,"AcDbXrecord");send(102,"{ATTRRECORD");send(5,"FFFFFFFFFFFFFFFF");
        send(105,"FFFFFFFFFFFFFFFE");send(102,"ATTRRECORD}");send(1001,"APP");send(1000,"value");
        send(0,"ENDSEC");send(0,"EOF");capture.finish();const auto& observed=capture.source();
        if(!check(observed.status==Source::Status::Complete&&observed.candidates.size()==4u
            &&observed.candidates[0].scope==Scope::Outer&&observed.candidates[1].scope==Scope::Outer
            &&observed.candidates[0].lexeme==observed.candidates[1].lexeme
            &&observed.candidates[2].scope==Scope::OpaqueBody&&observed.candidates[2].lexeme=="FFFFFFFFFFFFFFFF"
            &&observed.candidates[3].scope==Scope::OpaqueBody&&observed.candidates[3].lexeme=="FFFFFFFFFFFFFFFE",
            "XRECORD wide payload and duplicate outer identities stay distinct"))return false;
    }
    unsigned sources=0u,opaque=0u,unqualified=0u,quotas=0u,retries=0u,failed=0u;
    for(const std::string version:{"AC1018","AC1032"})for(bool binary:{false,true}) {
        const Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"},
            {0,"SECTION"},{2,"OBJECTS"},{0,"DICTIONARY"},{5,"40"},{330,"0"},{100,"AcDbDictionary"},{280,"0"},{281,"1"},
            {0,"XRECORD"},{5,"41"},{102,"{PRE"},{5,"44"},{102,"}"},{330,"40"},{100,"AcDbXrecord"},{280,"1"},
            {102,"{ATTRRECORD"},{100,"AcDbXrecord"},{5,"42"},{105,"43"},{102,"ATTRRECORD}"},
            {102,"{NESTED"},{102,"}"},{1001,"APP"},{1000,"payload"},{0,"ENDSEC"},{0,"EOF"}};
        if(!write(path,tags,binary))return false;Probe reader;dx_data data;
        if(!check(reader.fileImport(path.string(),&data,false)&&reader.calls==1u
            &&reader.observed.status==Source::Status::Complete&&reader.observed.eofMarker.ordinal,
            "XRECORD payload does not corrupt identity stream"))return false;
        const auto& source=reader.observed;std::size_t outer=0u,application=0u,body=0u;
        for(const auto& c:source.candidates) {
            if(c.record>=source.records.size()||source.records[c.record].name!="XRECORD")continue;
            if(c.code==5&&c.lexeme=="41"&&c.scope==Scope::Outer)++outer;
            else if(c.code==5&&c.lexeme=="44"&&c.scope==Scope::Application)++application;
            // The current raw XRECORD reader can report body code 5 as an
            // accepted registry admission. Its physical scope still forbids
            // treating that payload value as an outer identity.
            else if(((c.code==5&&c.lexeme=="42")||(c.code==105&&c.lexeme=="43"))
                &&c.scope==Scope::OpaqueBody)++body;
            else {std::cerr<<"unexpected XRECORD candidate code="<<c.code<<" lexeme="<<c.lexeme
                <<" scope="<<static_cast<int>(c.scope)<<" admission="<<static_cast<int>(c.admission)<<'\n';return false;}
        }
        if(!check(outer==1u&&application==1u&&body==2u,"XRECORD self, actual application and opaque values have distinct scopes"))return false;
        const auto report=dx_iface::identitySourceReport(data);
        if(version=="AC1018") {
            if(!check(report.state==dx_iface::IdentitySourceState::NotCaptured,"legacy identity authority remains gated"))return false;
        } else if(!check(report.state==dx_iface::IdentitySourceState::Unqualified&&report.unknown>=2u
            &&!report.outputEligible&&!report.ownerTargetsResolved,"opaque XRECORD handles cannot promote namespace"))return false;
        ++sources;opaque+=body;++unqualified;
        auto malformed=tags;const auto pre=std::find_if(malformed.begin(),malformed.end(),[](const Tag& t){return t.code==102&&t.value=="}";});
        pre->value="MALFORMED}";if(!write(path,malformed,binary))return false;
        Probe invalid;dx_data invalidData;const bool accepted=invalid.fileImport(path.string(),&invalidData,false);
        if(!check(!accepted||invalid.observed.status!=Source::Status::Complete,
            "malformed real application scope cannot gain complete identity"))return false;
        auto bad=tags;const auto app=std::find_if(bad.begin(),bad.end(),[](const Tag& t){return t.code==1001;});
        bad.insert(app+1,{102,"{AFTER_XDATA"});if(!write(path,bad,binary))return false;
        Probe rejected;dx_data broken;
        if(!check(!rejected.fileImport(path.string(),&broken,false)&&rejected.calls==0u,
            "XRECORD 1001 starts trailing XDATA; later 102 refuses"))return false;
        ++failed;if(!write(path,tags,binary))return false;
        dx_data retry;if(!check(rejected.fileImport(path.string(),&retry,false)&&rejected.calls==1u
            &&rejected.observed.status==Source::Status::Complete,"XRECORD corrected same-adapter retry"))return false;
        ++retries;
        Probe limited;limited.limits.candidates=1u;dx_data constrained;
        if(!check(limited.fileImport(path.string(),&constrained,false)&&limited.calls==1u
            &&limited.observed.status==Source::Status::Quota,"XRECORD identity observation quota remains non-fatal"))return false;
        limited.limits=DRW_DxfIdentitySourceLimits{};dx_data recovered;
        if(!check(limited.fileImport(path.string(),&recovered,false)&&limited.calls==2u
            &&limited.observed.status==Source::Status::Complete,"XRECORD identity quota corrected retry"))return false;
        ++quotas;++retries;
    }
    std::cout<<"XRECORD identity sources="<<sources<<" opaque candidates="<<opaque<<" unqualified="<<unqualified
        <<" failed tails="<<failed<<" quotas="<<quotas<<" retries="<<retries<<'\n';
    return sources==4u&&opaque==8u&&unqualified==4u&&failed==4u&&quotas==4u&&retries==8u;
}
