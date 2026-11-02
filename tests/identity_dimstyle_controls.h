// SPDX-License-Identifier: GPL-2.0-or-later
// Group 105 is DIMSTYLE's self identity; legacy group 5 is a DIMBLK name.
bool identityDimstyleTests(const std::filesystem::path& path) {
    using Source=DRW_DxfIdentitySource;using Scope=Source::Scope;
    struct Probe final:dx_iface {
        bool observe=true;Source source;unsigned census=0u,styles=0u,points=0u;DRW_Dimstyle style;DRW_Point point;
        bool wantsDxfIdentitySource()const override{return observe&&dx_iface::wantsDxfIdentitySource();}
        void addDxfIdentitySource(const Source& value)override {source=value;++census;dx_iface::addDxfIdentitySource(value);}
        void addDimStyle(const DRW_Dimstyle& value)override {style=value;++styles;dx_iface::addDimStyle(value);}
        void addPoint(const DRW_Point& value)override {point=value;++points;dx_iface::addPoint(value);}
    };
    const auto check=[](bool ok,const char* why){if(!ok)std::cerr<<"DIMSTYLE identity: "<<why<<'\n';return ok;};
    const auto make=[](const std::string& version) {
        return Tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"},
            {0,"SECTION"},{2,"TABLES"},{0,"TABLE"},{2,"DIMSTYLE"},{5,"12"},{100,"AcDbSymbolTable"},{70,"1"},
            {0,"DIMSTYLE"},{105,"41"},{102,"{APP"},{105,"42"},{5,"opaque"},{102,"}"},
            {330,"12"},{100,"AcDbSymbolTableRecord"},{2,"STANDARD"},{70,"0"},{100,"AcDbDimStyleTableRecord"},
            {5,"NOT_A_HANDLE"},{40,"1"},{0,"ENDTAB"},{0,"ENDSEC"},
            {0,"SECTION"},{2,"ENTITIES"},{0,"POINT"},{5,"42"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"0"},{20,"0"},{30,"0"},{0,"ENDSEC"},{0,"EOF"}};
    };
    unsigned valid=0u,later=0u,refusals=0u,retries=0u;
    for(const std::string version:{"AC1018","AC1032"})for(bool binary:{false,true})for(bool observe:{false,true}) {
        auto tags=make(version);if(!write(path,tags,binary))return false;Probe p;p.observe=observe;dx_data data;
        if(!check(p.fileImport(path.string(),&data,false)&&p.styles==1u&&p.points==1u&&p.style.handle==0x41u
            &&p.style.dimblk=="NOT_A_HANDLE"&&p.point.handle==0x42u&&p.census==(observe?1u:0u),"valid source and DIMBLK name"))return false;
        if(observe) {
            unsigned self=0u,app=0u;for(const auto& c:p.source.candidates)if(c.record<p.source.records.size()&&p.source.records[c.record].name=="DIMSTYLE") {
                if(c.code==105&&c.lexeme=="41"&&c.scope==Scope::Outer&&c.admission==Source::Admission::Accepted)++self;
                if(c.code==105&&c.lexeme=="42"&&c.scope==Scope::Application&&c.admission==Source::Admission::Unobserved)++app;
            }
            if(!check(self==1u&&app==1u&&p.source.status==Source::Status::Complete,"outer and application admissions"))return false;
        }
        ++valid;
        auto collision=tags;const auto point=slot(collision,0,"POINT");
        auto pointId=std::find_if(collision.begin()+static_cast<std::ptrdiff_t>(point),collision.end(),[](const Tag& v){return v.code==5;});pointId->value="41";
        if(!write(path,collision,binary))return false;Probe dup;dup.observe=observe;dx_data duplicate;
        if(!check(dup.fileImport(path.string(),&duplicate,false)&&dup.styles==1u&&dup.points==1u&&dup.point.handle==DRW::NoHandle,"later entity duplicate is softened"))return false;
        if(observe) {
            unsigned count=0u;for(const auto& c:dup.source.candidates)if(c.record<dup.source.records.size()&&dup.source.records[c.record].name=="POINT"
                &&c.scope==Scope::Outer&&c.admission==Source::Admission::Duplicate)++count;
            const auto report=dx_iface::identitySourceReport(duplicate);
            if(!check(count==1u&&(version=="AC1018"?report.state==dx_iface::IdentitySourceState::NotCaptured:
                report.state==dx_iface::IdentitySourceState::Unqualified&&report.duplicateAdmissions>=1u),"later duplicate evidence"))return false;
        }
        ++later;
        for(unsigned fault=0u;fault<4u;++fault) {
            auto bad=tags;const auto id=slot(bad,105,"41");
            if(fault==0u)bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(id+1u),{105,"41"});
            if(fault==1u)bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(id+1u),{105,"43"});
            if(fault==2u)bad[id].value="INVALID";
            if(fault==3u) {
                const auto dim=slot(bad,0,"DIMSTYLE");
                const Tags prefix{{0,"TABLE"},{2,"LAYER"},{5,"10"},{100,"AcDbSymbolTable"},{70,"1"},
                    {0,"LAYER"},{5,"41"},{330,"10"},{100,"AcDbSymbolTableRecord"},{2,"0"},{70,"0"},{100,"AcDbLayerTableRecord"},{62,"7"},{6,"CONTINUOUS"},{0,"ENDTAB"}};
                const auto table=slot(bad,0,"TABLE");bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(table),prefix.begin(),prefix.end());(void)dim;
            }
            if(!write(path,bad,binary))return false;Probe failed;failed.observe=observe;dx_data broken;
            if(!check(!failed.fileImport(path.string(),&broken,false)&&failed.styles==0u,"duplicate or invalid identity refuses"))return false;
            ++refusals;
            if(!write(path,tags,binary))return false;dx_data repaired;
            if(!check(failed.fileImport(path.string(),&repaired,false)&&failed.styles==1u&&failed.point.handle==0x42u,"same-facade retry"))return false;
            ++retries;
        }
    }
    // Legacy R12 symbol entries may legitimately omit identities. No new
    // registration is attempted when group 105 is absent.
    auto legacy=make("AC1009");legacy.erase(legacy.begin()+static_cast<std::ptrdiff_t>(slot(legacy,105,"41")));
    if(!write(path,legacy,false))return false;Probe r12;dx_data old;
    if(!check(r12.fileImport(path.string(),&old,false)&&r12.styles==1u&&r12.style.dimblk=="NOT_A_HANDLE","R12 absent identity"))return false;
    std::cout<<"DIMSTYLE identity valid="<<valid<<" later="<<later<<" refusals="<<refusals<<" retries="<<retries<<" R12=1\n";
    return valid==8u&&later==8u&&refusals==32u&&retries==32u;
}
