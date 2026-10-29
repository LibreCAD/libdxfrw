// SPDX-License-Identifier: GPL-2.0-or-later
// SORTENTSTABLE sort keys are physical payload, not database object identities.
bool identitySortentsTests(const std::filesystem::path& path) {
    using Source=DRW_DxfIdentitySource;using Scope=Source::Scope;
    const auto check=[](bool good,const char* why){if(!good)std::cerr<<"SORTENTSTABLE identity: "<<why<<'\n';return good;};
    struct Probe final:dx_iface {
        Source observed;DRW_SortEntsTable table;unsigned calls=0u,typed=0u;
        void addDxfIdentitySource(const Source& value) override {observed=value;++calls;dx_iface::addDxfIdentitySource(value);}
        void addSortEntsTable(const DRW_SortEntsTable& value) override {table=value;++typed;dx_iface::addSortEntsTable(value);}
    };
    // Direct observation exercises malformed and embedded scopes without
    // depending on whether a typed parser accepts an invalid drawing.
    {
        std::istringstream stream;dxfReaderAscii input(&stream);dxfIdentitySource::Capture capture(input);
        capture.start(DRW_DxfIdentitySourceLimits{});std::size_t ordinal=0u;
        const auto send=[&](int code,std::string_view value) {DxfReadRecordView v;v.code=code;v.kind=DxfReadRecordView::Text;v.text=value;v.raw=value;v.ordinal=++ordinal;v.beginOffset=ordinal*10u;v.endOffset=v.beginOffset+5u;capture.observe(v);};
        send(0,"SECTION");send(2,"OBJECTS");send(0,"SORTENTSTABLE");send(5,"41");send(102,"{APP");send(100,"AcDbSortentsTable");send(5,"42");send(102,"}");send(5,"43");send(100,"AcDbSortentsTable");send(331,"44");send(5,"45");send(102,"{APP");send(5,"46");send(102,"}");send(1001,"APP");send(5,"47");send(0,"ENDSEC");send(0,"EOF");capture.finish();
        const auto& seen=capture.source();
        if(!check(seen.status==Source::Status::Complete&&seen.candidates.size()==6u
            &&seen.candidates[0].scope==Scope::Outer&&seen.candidates[1].scope==Scope::Application
            &&seen.candidates[2].scope==Scope::Outer&&seen.candidates[3].scope==Scope::SortOrder
            &&seen.candidates[4].scope==Scope::Application&&seen.candidates[5].scope==Scope::Xdata,
            "marker, application and XDATA boundaries"))return false;
    }
    unsigned routes=0u,duplicates=0u,retries=0u;
    for(const std::string version:{"AC1018","AC1032"})for(bool binary:{false,true}) {
        const Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"},
            {0,"SECTION"},{2,"OBJECTS"},{0,"DICTIONARY"},{5,"40"},{330,"0"},{100,"AcDbDictionary"},{280,"0"},{281,"1"},
            {0,"SORTENTSTABLE"},{5,"41"},{102,"{APP"},{5,"44"},{102,"}"},{330,"40"},{100,"AcDbSortentsTable"},{330,"1F"},
            {331,"42"},{5,"41"},{331,"43"},{5,"42"},{1001,"APP"},{1000,"payload"},{0,"ENDSEC"},
            {0,"SECTION"},{2,"ENTITIES"},{0,"POINT"},{5,"42"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"0"},{20,"0"},{30,"0"},{0,"ENDSEC"},{0,"EOF"}};
        if(!write(path,tags,binary))return false;Probe p;dx_data data;
        if(!check(p.fileImport(path.string(),&data,false)&&p.calls==1u&&p.typed==1u&&p.observed.status==Source::Status::Complete
            &&p.table.handle==0x41u&&p.table.m_entityHandles==std::vector<std::uint32_t>({0x42u,0x43u})
            &&p.table.m_sortHandles==std::vector<std::uint32_t>({0x41u,0x42u}),"valid parse and typed sort keys"))return false;
        unsigned outer=0u,app=0u,sort=0u,point=0u;
        for(const auto& c:p.observed.candidates) {
            if(c.record>=p.observed.records.size())return false;const auto& r=p.observed.records[c.record];
            if(r.name=="SORTENTSTABLE"&&c.scope==Scope::Outer&&c.lexeme=="41")++outer;
            else if(r.name=="SORTENTSTABLE"&&c.scope==Scope::Application&&c.lexeme=="44")++app;
            else if(r.name=="SORTENTSTABLE"&&c.scope==Scope::SortOrder&&(c.lexeme=="41"||c.lexeme=="42")&&c.admission==Source::Admission::Unobserved)++sort;
            else if(r.name=="POINT"&&c.scope==Scope::Outer&&c.lexeme=="42"&&c.admission==Source::Admission::Accepted)++point;
        }
        const auto report=dx_iface::identitySourceReport(data);
        if(!check(outer==1u&&app==1u&&sort==2u&&point==1u&&(version=="AC1018"?report.state==dx_iface::IdentitySourceState::NotCaptured:
            report.state==dx_iface::IdentitySourceState::Unqualified&&!report.outputEligible&&!report.ownerTargetsResolved),"scope and fail-closed namespace"))return false;
        ++routes;
        auto bad=tags;auto pt=std::find_if(bad.begin(),bad.end(),[](const Tag& t){return t.code==0&&t.value=="POINT";});
        auto id=std::find_if(pt,bad.end(),[](const Tag& t){return t.code==5;});id->value="41";
        if(!write(path,bad,binary))return false;Probe dup;dx_data duplicate;
        if(!check(dup.fileImport(path.string(),&duplicate,false)&&dup.calls==1u,"duplicate soft import"))return false;
        unsigned collision=0u;for(const auto& c:dup.observed.candidates)if(c.record<dup.observed.records.size()&&dup.observed.records[c.record].name=="POINT"
            &&c.scope==Scope::Outer&&c.admission==Source::Admission::Duplicate)++collision;
        const auto dupReport=dx_iface::identitySourceReport(duplicate);
        if(!check(collision==1u&&(version=="AC1018"?dupReport.state==dx_iface::IdentitySourceState::NotCaptured:
            dupReport.state==dx_iface::IdentitySourceState::Unqualified&&dupReport.duplicateAdmissions>=1u),"true outer duplicate"))return false;
        ++duplicates;
        for(bool duplicateMarker:{false,true}) {
            auto malformed=tags;auto marker=std::find_if(malformed.begin(),malformed.end(),[](const Tag& t){return t.code==100&&t.value=="AcDbSortentsTable";});
            if(duplicateMarker)malformed.insert(marker+1,{100,"AcDbSortentsTable"});else malformed.erase(marker);
            if(!write(path,malformed,binary))return false;Probe rejected;dx_data broken;
            if(!check(!rejected.fileImport(path.string(),&broken,false),"missing or duplicate subclass refuses"))return false;
            if(!write(path,tags,binary))return false;dx_data repaired;
            if(!check(rejected.fileImport(path.string(),&repaired,false)&&rejected.calls==1u&&rejected.typed==1u,"subclass corrected retry"))return false;
        }
        if(!write(path,tags,binary))return false;dx_data retry;
        if(!check(dup.fileImport(path.string(),&retry,false)&&dup.calls==2u&&dup.typed==2u,"same-facade corrected retry"))return false;
        ++retries;
    }
    std::cout<<"SORTENTSTABLE identity routes="<<routes<<" duplicates="<<duplicates<<" retries="<<retries<<'\n';
    return routes==4u&&duplicates==4u&&retries==4u;
}
