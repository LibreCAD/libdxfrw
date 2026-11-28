// SPDX-License-Identifier: GPL-2.0-or-later
// Uses the owning probe's generated documents and independent original-file walker.
bool rawShellScopeTests(const std::filesystem::path& path) {
    unsigned profiles=0u,refusals=0u,tolerances=0u,nesting=0u,limits=0u,exceptions=0u;
    const std::array<const char*,3> names{{"BLOCKVISIBILITYPARAMETER","ACSH_HISTORY_NODE","ACSH_EXTRUSION_CLASS"}};
    const auto matches=[](const RawScopeProbe& sink,const char* name,std::uint32_t handle) {
        if(sink.objects.size()!=1u||sink.objects.front().handle!=handle||sink.objects.front().parentHandle!=0xB0u)return false;
        if(std::string_view(name)=="BLOCKVISIBILITYPARAMETER")return sink.dynamicObjects.size()==1u&&sink.historyObjects.empty()
            &&sink.dynamicObjects.front().handle==handle&&sink.dynamicObjects.front().parentHandle==0xB0u
            &&sink.dynamicObjects.front().m_recordName==name;
        return sink.historyObjects.size()==1u&&sink.dynamicObjects.empty()&&sink.historyObjects.front().handle==handle
            &&sink.historyObjects.front().parentHandle==0xB0u&&sink.historyObjects.front().m_recordName==name;
    };
    const Tags clean{{5,"C1"},{330,"B0"},{100,"AcDbObject"},{100,"AcDbOpaqueShell"},{1,"unmodeled"}};
    for(const std::string version:{"AC1012","AC1014","AC1015","AC1018","AC1021","AC1024","AC1027","AC1032"})
     for(bool binary:{false,true})for(const auto* name:names)for(unsigned shape=0u;shape<7u;++shape) {
        auto row=clean;
        const Tags app{{102,"{OPAQUE"},{5,shape==0u?"C2":shape==1u?"C1":shape==2u?"not-a-handle":"FFFFFFFFFFFFFFFF"},
            {330,"FACE"},{102,"{INNER"},{5,shape==0u?"F001":"opaque-inner"},{100,"AcDbShSweepBase"},{102,"}"},{102,"}"}};
        row.insert(shape==4u?row.begin():row.end(),app.begin(),app.end());
        if(shape==5u)row.insert(row.end()-1,{{92,"-1"},{310,"DEAD"},{70,"-1"}});
        if(shape==6u){row.erase(row.begin());row.push_back({5,"C1"});}
        const auto f=rawScopeDocument(version,2u,name,row);if(!write(path,f.document,binary))return false;
        dxfRW codec(path.string().c_str());RawScopeProbe sink;
        if(!check(codec.read(&sink,false)&&sink.points==std::vector<std::uint32_t>{0xC2u}&&matches(sink,name,0xC1u)
            &&rawScopeCarrierMatches(sink.objects.front(),row,binary),"specialized shell identity/owner excludes opaque groups without changing exact source")) {
            std::cerr<<name<<" "<<version<<" binary="<<binary<<" shape="<<shape
                <<" following handle="<<(sink.points.empty()?0u:sink.points.front())<<'\n';return false;
        }
        ++profiles;
    }
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true})for(const auto* name:names) {
        for(unsigned fault=0u;fault<4u;++fault) {
            auto row=clean;
            if(fault==0u)row.insert(row.begin()+1,{5,"C1"});
            if(fault==1u)row.front().value="not-a-handle";
            if(fault==2u)row.push_back({102,"{UNCLOSED"});
            if(fault==3u)row.push_back({102,"}"});
            auto f=rawScopeDocument(version,2u,name,row);if(!write(path,f.document,binary))return false;
            dxfRW codec(path.string().c_str());RawScopeProbe sink;
            if(!check(!codec.read(&sink,false)&&sink.objects.empty()&&sink.dynamicObjects.empty()&&sink.historyObjects.empty(),
                "invalid shell outer handle/scope fails before typed or raw publication"))return false;
            f=rawScopeDocument(version,2u,name,clean);if(!write(path,f.document,binary))return false;
            RawScopeProbe retry;
            if(!check(codec.read(&retry,false)&&matches(retry,name,0xC1u)&&retry.points==std::vector<std::uint32_t>{0xC2u},
                "same-codec specialized-shell corrected retry"))return false;
            ++refusals;
        }
        for(bool nullSelf:{false,true}) {
            auto row=clean;if(nullSelf)row.front().value="0";else row.erase(row.begin());
            row.insert(row.end(),{{102,"{OPAQUE"},{5,"C2"},{102,"}"}});
            const auto f=rawScopeDocument(version,2u,name,row);if(!write(path,f.document,binary))return false;
            dxfRW codec(path.string().c_str());RawScopeProbe sink;
            if(!check(codec.read(&sink,false)&&matches(sink,name,DRW::NoHandle)&&sink.points==std::vector<std::uint32_t>{0xC2u},
                "existing shell absent/null-self tolerance must not invent a typed identity or consume a later handle"))return false;
            ++tolerances;
        }
    }
    for(bool binary:{false,true})for(const auto* name:names) {
        auto row=clean;
        for(unsigned i=0;i<64u;++i)row.push_back({102,"{OPAQUE"});
        row.push_back({5,"not-a-handle"});
        for(unsigned i=0;i<64u;++i)row.push_back({102,"}"});
        auto good=rawScopeDocument("AC1032",2u,name,row);if(!write(path,good.document,binary))return false;
        dxfRW codec(path.string().c_str());RawScopeProbe exact;
        if(!check(codec.read(&exact,false)&&matches(exact,name,0xC1u),"specialized raw shell admits the existing 64-level nesting limit"))return false;
        auto bad=row;bad.insert(bad.begin()+clean.size(),{102,"{ONE_TOO_MANY"});bad.push_back({102,"}"});
        const auto malformed=rawScopeDocument("AC1032",2u,name,bad);if(!write(path,malformed.document,binary))return false;
        RawScopeProbe refused;
        if(!check(!codec.read(&refused,false)&&refused.objects.empty()&&refused.dynamicObjects.empty()&&refused.historyObjects.empty(),
            "65-level shell nesting fails before either callback"))return false;
        if(!write(path,good.document,binary))return false;
        RawScopeProbe retry;if(!check(codec.read(&retry,false)&&matches(retry,name,0xC1u),"corrected maximum-depth same-codec retry"))return false;
        ++nesting;
        for(unsigned kind=0u;kind<2u;++kind) {
            const auto configure=[&](dxfRW& reader,std::size_t n) {
                if(kind==0u)reader.setDxfReadRetainedByteBudget(n);else reader.setDxfReadTypedVariantNodeBudget(n);
            };
            std::size_t lower=0u,upper=kind==0u?1048576u:1024u;
            {dxfRW generous(path.string().c_str());configure(generous,upper);RawScopeProbe sink;
                if(!generous.read(&sink,false))return false;}
            while(lower<upper) {
                const auto middle=lower+(upper-lower)/2u;
                dxfRW probe(path.string().c_str());configure(probe,middle);RawScopeProbe sink;
                if(probe.read(&sink,false))upper=middle;else lower=middle+1u;
            }
            if(lower==0u)return false;
            dxfRW bounded(path.string().c_str());configure(bounded,lower);RawScopeProbe accepted;
            if(!check(bounded.read(&accepted,false)&&matches(accepted,name,0xC1u),"exact calibrated shell byte/node ceiling admits real source"))return false;
            configure(bounded,lower-1u);RawScopeProbe under;
            if(!check(!bounded.read(&under,false)&&bounded.getLastDiagnostic().cause==DRW::OperationCause::ResourceLimit,
                "one-under calibrated shell byte/node ceiling refuses the operation"))return false;
            configure(bounded,lower);RawScopeProbe restored;
            if(!check(bounded.read(&restored,false)&&matches(restored,name,0xC1u),"same-facade exact-ceiling retry resets admission state"))return false;
            ++limits;
        }
        for(unsigned callback:{1u,2u}) {
            RawScopeProbe throwing;throwing.throwShell=callback;
            bool propagated=false;
            try {(void)codec.read(&throwing,false);}catch(const std::runtime_error&){propagated=true;}
            if(!check(propagated,"specialized shell typed/raw callback exception retains existing propagation policy"))return false;
            RawScopeProbe corrected;if(!check(codec.read(&corrected,false)&&matches(corrected,name,0xC1u)
                &&corrected.points==std::vector<std::uint32_t>{0xC2u},"same-codec clean retry after shell callback exception"))return false;
            ++exceptions;
        }
    }
    std::cout<<"Specialized shell application profiles="<<profiles<<"; refusals/retries="<<refusals<<"; absent/null-self tolerance="<<tolerances
        <<"; maximum-depth boundaries/retries="<<nesting<<"; byte/node exact-under-retries="<<limits<<"; callback exceptions/retries="<<exceptions<<'\n';
    return profiles==336u&&refusals==96u&&tolerances==48u&&nesting==6u&&limits==12u&&exceptions==12u;
}

Tags acshProjectionBody() {
    Tags row{{100,"AcDbShSweepBase"}};
    for(int i=1;i<=16;++i)row.push_back({46,std::to_string(i)});
    for(int i=1;i<=16;++i)row.push_back({47,std::to_string(-i)});
    row.insert(row.end(),{{70,"1"},{71,"2"},{290,"1"},{292,"1"},{293,"0"},{294,"0"},{295,"1"},{296,"0"},
        {11,"3.5"},{21,"-2"},{31,"9"}});
    return row;
}

bool acshProjectionScopeTests(const std::filesystem::path& path) {
    unsigned complete=0u,incomplete=0u;std::size_t spans=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true})for(unsigned shape:{4u,0u,1u,2u,3u,5u,6u}) {
        Tags row{{5,"C1"},{330,"B0"},{100,"AcDbObject"}};auto body=acshProjectionBody();
        if(shape==1u)body.insert(body.begin()+1,{{102,"{OPAQUE"},{100,"AcDbUnrelated"},{102,"}"}});
        if(shape==2u)body.insert(body.begin()+1,{{102,"{OPAQUE"},{46,"999"},{70,"-1"},{11,"99"},{102,"}"}});
        if(shape==3u)body.insert(body.begin()+1,{{102,"{OPAQUE"},{102,"{INNER"},{100,"AcDbShSweepBase"},{102,"}"},{102,"}"}});
        if(shape==4u){body.insert(body.begin(),{102,"{OPAQUE"});body.push_back({102,"}"});}
        if(shape==5u) {
            body.erase(body.begin()+16); // Sixteenth real transform scalar missing.
            body.insert(body.begin()+1,{{102,"{OPAQUE"},{46,"16"},{102,"}"}});
        }
        if(shape==6u)body.insert(body.begin()+1,{{102,"{OPAQUE"},{5,"not-a-handle"},{100,"AcDbUnrelated"},{102,"}"}});
        row.insert(row.end(),body.begin(),body.end());row.push_back({100,"AcDbShExtrusion"});
        const auto f=rawScopeDocument(version,2u,"ACSH_EXTRUSION_CLASS",row);if(!write(path,f.document,binary))return false;
        dxfRW codec(path.string().c_str());RawScopeProbe sink;
        const bool accepted=codec.read(&sink,false);
        if(!check(accepted&&sink.historyObjects.size()==1u&&sink.objects.size()==1u
            &&sink.points==std::vector<std::uint32_t>{0xC2u}&&rawScopeCarrierMatches(sink.objects.front(),row,binary),
            "opaque/partial ACSH projection stays diagnostic and preserves the complete raw shell")) {
            std::cerr<<version<<" binary="<<binary<<" shape="<<shape<<" read="<<accepted<<" typed="<<sink.historyObjects.size()
                <<" raw="<<sink.objects.size()<<" points="<<sink.points.size()<<'\n';return false;
        }
        const bool expected=shape!=4u&&shape!=5u;
        const auto& p=sink.historyObjects.front().m_extrusionDxfProjection;
        if(!check(p.m_transformAndOptionFieldsComplete==expected,"only outer AcDbShSweepBase fields can complete the ACSH projection")) {
            std::cerr<<version<<" binary="<<binary<<" shape="<<shape<<" complete="<<p.m_transformAndOptionFieldsComplete<<'\n';return false;
        }
        if(expected) {
            for(std::size_t i=0;i<16u;++i)if(p.m_sweepEntityTransform[i]!=static_cast<double>(i+1u)
                ||p.m_pathEntityTransform[i]!=-static_cast<double>(i+1u))return false;
            if(!check(p.m_alignOption==1&&p.m_miterOption==2&&p.m_isSolid&&p.m_hasAlignStart&&!p.m_bank&&!p.m_basePointSet
                &&p.m_sweepEntityTransformComputed&&!p.m_pathEntityTransformComputed
                &&p.m_point.x==3.5&&p.m_point.y==-2.0&&p.m_point.z==9.0,"complete outer ACSH transform/options/flags/point stay unchanged"))return false;
            ++complete;
        } else {
            if(!check(std::all_of(p.m_sweepEntityTransform.begin(),p.m_sweepEntityTransform.end(),[](double v){return v==0.0;})
                &&std::all_of(p.m_pathEntityTransform.begin(),p.m_pathEntityTransform.end(),[](double v){return v==0.0;})
                &&p.m_alignOption==0&&p.m_miterOption==0&&!p.m_isSolid&&!p.m_hasAlignStart&&!p.m_bank&&!p.m_basePointSet
                &&!p.m_sweepEntityTransformComputed&&!p.m_pathEntityTransformComputed,
                "refused projection cannot publish a partial transform/option payload"))return false;
            ++incomplete;
        }
        const auto wire=attdefWireTags(path,binary);
        if(wire.size()!=f.document.size())return false;
        for(std::size_t i=0;i<wire.size();++i) {
            if(wire[i].code!=f.document[i].code||wire[i].begin>=wire[i].end)return false;
            if(wire[i].kind==AttdefWireTag::Real) {if(wire[i].real!=std::stod(f.document[i].value))return false;}
            else if(wire[i].kind==AttdefWireTag::Integer) {if(wire[i].integer!=std::stoll(f.document[i].value))return false;}
            else if(wire[i].text!=f.document[i].value)return false;
            ++spans;
        }
    }
    std::cout<<"ACSH projection outer complete="<<complete<<"; opaque/partial incomplete="<<incomplete<<"; independent original scalar spans="<<spans<<'\n';
    return complete==40u&&incomplete==16u&&spans==4088u;
}
