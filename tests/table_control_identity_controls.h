// SPDX-License-Identifier: GPL-2.0-or-later
// TABLE controls share the actual read registry; physical source evidence is
// not owner, target, export or native CAD qualification.
bool tableControlIdentityTests(const std::filesystem::path& path) {
    using Source=DRW_DxfIdentitySource;
    struct Probe final:dx_iface {
        Source census;unsigned callbacks=0u,layers=0u,records=0u;
        void addDxfIdentitySource(const Source& s)override{census=s;++callbacks;dx_iface::addDxfIdentitySource(s);}
        void addLayer(const DRW_Layer& v)override{++layers;dx_iface::addLayer(v);}
        void addXRecord(const DRW_XRecord& v)override{++records;dx_iface::addXRecord(v);}
    };
    const auto check=[](bool ok,const char* why){if(!ok)std::cerr<<why<<'\n';return ok;};
    const auto header=[](const std::string& version) {
        return Tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"}};
    };
    unsigned matrices=0u,collisions=0u,retries=0u,scopes=0u,tolerant=0u;
    for(const std::string version:{"AC1018","AC1032"})for(bool binary:{false,true}) {
        Tags all=header(version);all.insert(all.end(),{{0,"SECTION"},{2,"TABLES"}});
        const std::array<const char*,10> names{{"VPORT","LTYPE","LAYER","STYLE","VIEW","UCS","APPID","DIMSTYLE","BLOCK_RECORD","FUTURE"}};
        for(std::size_t i=0;i<names.size();++i) {
            std::ostringstream id;id<<std::hex<<(0x100u+i);
            all.insert(all.end(),{{0,"TABLE"},{2,names[i]},{5,id.str()},{100,"AcDbSymbolTable"},{70,"0"},{0,"ENDTAB"}});
        }
        all.insert(all.end(),{{0,"ENDSEC"},{0,"EOF"}});if(!write(path,all,binary))return false;
        Probe reader;dx_data data;if(!check(reader.fileImport(path.string(),&data,false)&&reader.callbacks==1u
            &&reader.census.status==Source::Status::Complete,"all TABLE control readers import"))return false;
        unsigned controls=0u;
        for(const auto& c:reader.census.candidates) {
            if(c.record>=reader.census.records.size())return false;
            const auto& row=reader.census.records[c.record];
            if(row.kind==Source::Kind::TableControl&&c.code==5&&c.scope==Source::Scope::Outer
                &&c.admission==Source::Admission::Accepted)++controls;
        }
        if(!check(controls==names.size(),"known and unknown TABLE controls register their outer identity"))return false;++matrices;
        const auto section=[](bool first,bool collision) {
            const Tags table{{0,"SECTION"},{2,"TABLES"},{0,"TABLE"},{2,"LAYER"},{5,"10"},{100,"AcDbSymbolTable"},{70,"1"},
                {0,"LAYER"},{5,"11"},{330,"10"},{100,"AcDbSymbolTableRecord"},{2,"0"},{70,"0"},
                {100,"AcDbLayerTableRecord"},{62,"7"},{6,"CONTINUOUS"},{0,"ENDTAB"},{0,"ENDSEC"}};
            const Tags object{{0,"SECTION"},{2,"OBJECTS"},{0,"XRECORD"},{5,collision?"10":"20"},{330,"0"},
                {100,"AcDbXrecord"},{280,"1"},{1,"payload"},{0,"ENDSEC"}};
            Tags result;const auto add=[&](const Tags& values){result.insert(result.end(),values.begin(),values.end());};
            if(first){add(table);add(object);}else{add(object);add(table);}result.push_back({0,"EOF"});return result;
        };
        for(bool first:{false,true}) {
            auto bad=header(version);const auto payload=section(first,true);bad.insert(bad.end(),payload.begin(),payload.end());
            if(!write(path,bad,binary))return false;Probe rejected;dx_data failed;
            if(!check(!rejected.fileImport(path.string(),&failed,false)&&rejected.callbacks==0u,
                "TABLE control and XRECORD duplicate self must refuse"))return false;
            ++collisions;
            auto good=header(version);const auto fixed=section(first,false);good.insert(good.end(),fixed.begin(),fixed.end());
            if(!write(path,good,binary))return false;dx_data retry;rejected.callbacks=rejected.layers=rejected.records=0u;
            if(!check(rejected.fileImport(path.string(),&retry,false)&&rejected.layers==1u&&rejected.records==1u
                &&rejected.callbacks==1u&&rejected.census.status==Source::Status::Complete,
                "TABLE control duplicate corrected same-adapter retry"))return false;
            ++retries;
        }
        for(bool known:{false,true}) {
            auto tags=header(version);tags.insert(tags.end(),{{0,"SECTION"},{2,"TABLES"},{0,"TABLE"},{2,known?"LAYER":"FUTURE"},
                {102,"{CUSTOM"},{5,"20"},{102,"{NESTED"},{5,"FFFFFFFFFFFFFFFF"},{102,"}"},{102,"}"},
                {5,"10"},{100,"AcDbSymbolTable"},{70,"0"},{0,"ENDTAB"},{0,"ENDSEC"},
                {0,"SECTION"},{2,"OBJECTS"},{0,"XRECORD"},{5,"20"},{330,"0"},{100,"AcDbXrecord"},{280,"1"},{0,"ENDSEC"},{0,"EOF"}});
            if(!write(path,tags,binary))return false;Probe scoped;dx_data captured;
            if(!check(scoped.fileImport(path.string(),&captured,false)&&scoped.census.status==Source::Status::Complete,
                "TABLE control nested application payload stays out of registry"))return false;
            unsigned outer=0u,application=0u;
            for(const auto& c:scoped.census.candidates)if(c.record<scoped.census.records.size()
                &&scoped.census.records[c.record].kind==Source::Kind::TableControl) {
                if(c.lexeme=="10"&&c.scope==Source::Scope::Outer&&c.admission==Source::Admission::Accepted)++outer;
                else if((c.lexeme=="20"||c.lexeme=="FFFFFFFFFFFFFFFF")&&c.scope==Source::Scope::Application
                    &&c.admission==Source::Admission::Unobserved)++application;
                else return false;
            }
            if(!check(outer==1u&&application==2u,"TABLE header payload/outer roles stay distinct"))return false;++scopes;
        }
        for(unsigned mode=0u;mode<4u;++mode) {
            auto tags=header(version);tags.insert(tags.end(),{{0,"SECTION"},{2,"TABLES"},{0,"TABLE"},{2,"LAYER"}});
            if(mode==0u)tags.push_back({5,"0"});
            if(mode==2u)tags.push_back({5,"not-hex"});
            if(mode==3u)tags.push_back({5,"FFFFFFFFFFFFFFFF"});
            tags.insert(tags.end(),{{100,"AcDbSymbolTable"},{70,"0"},{0,"ENDTAB"},{0,"ENDSEC"},{0,"EOF"}});
            if(!write(path,tags,binary))return false;Probe probe;dx_data compatible;
            if(!check(probe.fileImport(path.string(),&compatible,false)&&probe.callbacks==1u,
                "TABLE null/absent/invalid/wide header behavior"))return false;
            if(mode!=1u) {
                if(!check(probe.census.candidates.size()==1u&&probe.census.candidates[0].admission==
                    (mode==0u?Source::Admission::Null:mode==2u?Source::Admission::Unobserved:Source::Admission::Accepted),
                    "TABLE header admission reports null, malformed or wide value"))return false;
            }else if(!probe.census.candidates.empty())return false;
            ++tolerant;
        }
    }
    std::cout<<"TABLE control matrices="<<matrices<<" collisions="<<collisions<<" retries="<<retries
        <<" nested scopes="<<scopes<<" legacy controls="<<tolerant<<'\n';
    return matrices==4u&&collisions==8u&&retries==8u&&scopes==8u&&tolerant==16u;
}
