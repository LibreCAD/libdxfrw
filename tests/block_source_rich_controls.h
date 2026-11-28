// SPDX-License-Identifier: GPL-2.0-or-later
// These controls author fields and expected roles separately. No production
// profile helper is used to construct the expected ledger or original bytes.
void replaceBlockEndpoint(BlockFixture& fixture,bool footer,Tags tags) {
    auto marker=std::find_if(fixture.document.begin(),fixture.document.end(),[&](const Tag& t){return t.code==0&&t.value==(footer?"ENDBLK":"BLOCK");});
    if(marker==fixture.document.end())throw std::runtime_error("missing authored block endpoint");
    const auto index=static_cast<std::size_t>(marker-fixture.document.begin())+1u;
    const auto last=std::find_if(marker+1,fixture.document.end(),[](const Tag& t){return t.code==0;});
    fixture.document.erase(marker+1,last);
    fixture.document.insert(fixture.document.begin()+static_cast<std::ptrdiff_t>(index),tags.begin(),tags.end());
    (footer?fixture.end:fixture.begin)=std::move(tags);
}

BlockFixture richBlockFixture(const std::string& version) {
    auto fixture=blockFixture(version,1u,true);
    for(bool footer:{false,true}) {
        auto tags=footer?fixture.end:fixture.begin;
        Tags common{{6,"DASHED"},{347,"123"},{348,"124"},{390,"125"},{62,"3"},{370,"35"},
            {48,"1.25"},{60,"1"},{420,"1193046"},{430,"book$swatch-é"},{440,"1234"},{67,"0"},{284,"2"},
            {102,"{ACAD_REACTORS"},{330,"ABC"},{330,"ABD"},{102,"}"},
            {102,"{ACAD_XDICTIONARY"},{360,"DEF"},{102,"}"}};
        if(!footer)common.insert(common.end(),{{version=="AC1021"?92:160,"3"},{310,"0102"},{310,"A3"}});
        const auto subclass=std::find_if(tags.begin(),tags.end(),[&](const Tag& t){return t.code==100&&t.value==(footer?"AcDbBlockEnd":"AcDbBlockBegin");});
        tags.insert(subclass,common.begin(),common.end());
        if(!footer) {
            const auto appid=std::find_if(tags.begin(),tags.end(),[](const Tag& t){return t.code==1001&&t.value=="BEGIN_APP";});
            tags.insert(appid,{{39,"0.75"},{50,"37.5"},{210,"0.1"},{220,"0.2"},{230,"0.97"},{71,"1"}});
        }
        tags.insert(tags.end(),{{1002,"{"},{1005,"ABCD"},{1004,"0102"},{1040,"0.75"},
            {1071,"-4"},{1000,"XDATA-é"},{1002,"}"}});
        replaceBlockEndpoint(fixture,footer,std::move(tags));
    }
    return fixture;
}

std::vector<dx_attdef_source::Span> richBlockLedger(const std::string& version,bool footer) {
    using F=dx_attdef_source::Field;using R=dx_attdef_source::Role;using S=dx_attdef_source::Scope;
    auto result=blockSpanLedger(footer);
    std::vector<dx_attdef_source::Span> common;
    const auto add=[&](int code,F field,R role=R::None,std::uint64_t handle=0u){common.push_back({0u,code,S::Host,field,role,handle});};
    add(6,F::Common,R::LineTypeName);add(347,F::Common,R::Material,0x123u);
    add(348,F::Common,R::FullVisualStyle,0x124u);add(390,F::Common,R::PlotStyle,0x125u);
    for(int code:{62,370,48,60,420,430,440,67,284})add(code,F::Common);
    add(102,F::Application);add(330,F::Application,R::Reactor,0xABCu);
    add(330,F::Application,R::Reactor,0xABDu);add(102,F::Application);
    add(102,F::Application);add(360,F::Application,R::ExtensionDictionary,0xDEFu);add(102,F::Application);
    if(!footer){add(version=="AC1021"?92:160,F::Common);add(310,F::Common);add(310,F::Common);}
    result.insert(result.begin()+(footer?4:13),common.begin(),common.end());
    if(!footer) {
        common.clear();for(int code:{39,50,210,220,230,71})add(code,F::Block);
        result.insert(result.begin()+45,common.begin(),common.end());
    }
    common.clear();add(1002,F::Xdata);add(1005,F::Xdata,R::XdataObjectId,0xABCDu);
    for(int code:{1004,1040,1071,1000,1002})add(code,F::Xdata);
    result.insert(result.end(),common.begin(),common.end());
    for(std::size_t i=0;i<result.size();++i)result[i].groupIndex=i;
    return result;
}

bool exactBlockLedger(const dx_attdef_source::Audit& actual,const std::vector<dx_attdef_source::Span>& expected) {
    if(!actual.complete()||actual.spans.size()!=expected.size())return false;
    for(std::size_t i=0;i<expected.size();++i) {
        const auto& a=actual.spans[i];const auto& b=expected[i];
        if(a.groupIndex!=b.groupIndex||a.code!=b.code||a.scope!=b.scope||a.field!=b.field||a.role!=b.role||a.handle!=b.handle)return false;
    }
    return true;
}

bool blockEndpointRichTests(const std::filesystem::path& path) {
    using A=DxIfaceTableSourceTestAccess;using R=A::BlockReconciliation;
    unsigned profiles=0u,names=0u,fields=0u,contexts=0u,quotas=0u,negatives=0u;
    std::size_t spans=0u,corruptions=0u;long allocations=0;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true}) {
        const auto fixture=richBlockFixture(version);if(!write(path,fixture.document,binary))return false;
        BlockSourceProbe reader;dx_data data;
        if(!check(reader.fileImport(path.string(),&data,false)&&A::blockEndpointProof(data),"rich original endpoint replay/profile/reconciliation")) {
            const auto* row=A::block(data);std::cerr<<version<<" binary="<<binary;
            if(row)std::cerr<<" fields="<<row->beginFieldsMatched<<'/'<<row->footerFieldsMatched
                <<" profiles="<<static_cast<unsigned>(row->beginProfile.reason)<<'/'<<static_cast<unsigned>(row->endProfile.reason);
            std::cerr<<'\n';return false;
        }
        const auto* row=A::block(data);
        if(!check(blockOriginalWire(path,binary,fixture,*row->beginRaw,*row->endRaw,spans),"rich exact original-file scalar inventory"))return false;
        for(bool footer:{false,true}) {
            const auto& actual=footer?row->endProfile:row->beginProfile;
            const auto expected=richBlockLedger(version,footer);
            if(!check(exactBlockLedger(actual,expected),"independently authored rich field/reference roles"))return false;
            for(std::size_t i=0;i<expected.size();++i)for(unsigned fault=0;fault<6u;++fault) {
                auto bad=actual;
                auto& span=bad.spans[i];
                switch(fault) {
                case 0:++span.groupIndex;break;case 1:++span.code;break;
                case 2:span.scope=dx_attdef_source::Scope::Nested;break;
                case 3:span.field=span.field==dx_attdef_source::Field::Common?dx_attdef_source::Field::Identity:dx_attdef_source::Field::Common;break;
                case 4:span.role=span.role==dx_attdef_source::Role::None?dx_attdef_source::Role::Owner:dx_attdef_source::Role::None;break;
                case 5:++span.handle;break;
                }
                if(!check(!exactBlockLedger(bad,expected),"rich ledger refuses corrupted index/code/scope/field/role/reference"))return false;
                ++corruptions;
            }
        }
        if(!check(row->original->reactorHandles==std::vector<std::uint32_t>({0xABCu,0xABDu})
            &&row->footer->reactorHandles==row->original->reactorHandles&&row->original->xDictHandle==0xDEFu
            &&row->footer->xDictHandle==0xDEFu&&row->original->proxyGraphics==std::string("\x01\x02\xA3",3u)
            &&row->original->xrefUnloaded&&row->original->thickness==0.75
            &&std::abs(row->original->xAxisAngle-37.5/ARAD)<1e-15,"rich parsed endpoint values and reserved handle roles"))return false;
        const auto usage=A::blockUsage(data);
        for(unsigned dimension=0;dimension<usage.size();++dimension) {
            dx_data exact;A::blockLimit(exact,dimension,usage[dimension]);
            if(!check(reader.fileImport(path.string(),&exact,false)&&A::blockEndpointProof(exact),"rich exact aggregate endpoint quota"))return false;
            dx_data below;A::blockLimit(below,dimension,usage[dimension]-1u);
            if(!check(reader.fileImport(path.string(),&below,false)&&A::blockFailed(below)&&!A::blockEndpointProof(below),"rich one-below aggregate endpoint quota"))return false;
            dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::blockEndpointProof(retry))return false;++quotas;
        }
        ++profiles;
    }
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(bool binary:{false,true})for(unsigned fault=0;fault<12u;++fault) {
        auto fixture=richBlockFixture(version);auto tags=fixture.begin;
        auto at=std::find_if(tags.begin(),tags.end(),[](const Tag& t){return t.code==1001&&t.value=="BEGIN_APP";});
        if(fault==0u)std::find_if(tags.begin(),tags.end(),[](const Tag& t){return t.code==92||t.code==160;})->value="4";
        if(fault==1u)std::find_if(tags.begin(),tags.end(),[](const Tag& t){return t.code==92||t.code==160;})->code=version=="AC1021"?160:92;
        if(fault==2u)tags.insert(at,{version=="AC1021"?92:160,"3"});
        if(fault==3u) {
            const auto xdict=std::find_if(tags.begin(),tags.end(),[](const Tag& t){return t.code==102&&t.value=="{ACAD_XDICTIONARY";});
            tags.insert(xdict+1,{{102,"{NESTED"},{102,"}"}});
        }
        if(fault==4u) {
            const auto reactors=std::find_if(tags.begin(),tags.end(),[](const Tag& t){return t.code==102&&t.value=="{ACAD_REACTORS";});
            tags.insert(reactors+1,{1,"not a reactor handle"});
        }
        if(fault==5u)tags.erase(std::remove_if(tags.begin(),tags.end(),[](const Tag& t){return t.code==220;}),tags.end());
        if(fault==6u)tags.erase(std::remove_if(tags.begin(),tags.end(),[](const Tag& t){return t.code==210||t.code==220;}),tags.end());
        if(fault==7u)tags.insert(at,{349,"123"});
        if(fault==8u)std::find_if(tags.begin(),tags.end(),[](const Tag& t){return t.code==71;})->value="2";
        if(fault==9u)tags.insert(at,{50,"12"});
        if(fault==10u)at->value.clear();
        if(fault==11u) {
            const auto subclass=std::find_if(tags.begin(),tags.end(),[](const Tag& t){return t.code==100;});
            tags.insert(subclass,{1001,"EARLY_APP"});
        }
        replaceBlockEndpoint(fixture,false,std::move(tags));if(!write(path,fixture.document,binary))return false;
        BlockSourceProbe reader;dx_data data;const bool ok=reader.fileImport(path.string(),&data,false);
        if(!check(!ok||!A::blockEndpointProof(data),"rich unknown/lossy/partial grammar cannot become endpoint evidence")) {
            std::cerr<<"rich refusal escaped: "<<version<<" binary="<<binary<<" fault="<<fault<<'\n';return false;
        }
        const auto clean=richBlockFixture(version);if(!write(path,clean.document,binary))return false;
        dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::blockEndpointProof(retry))return false;++negatives;
    }
    for(bool binary:{false,true}) {
        const auto fixture=richBlockFixture("AC1032");if(!write(path,fixture.document,binary))return false;
        BlockSourceProbe calibration;calibration.allocationPhase=4;calibration.failAllocation=(std::numeric_limits<long>::max)();dx_data sample;
        if(!check(calibration.fileImport(path.string(),&sample,false)&&A::blockEndpointProof(sample)
            &&calibration.allocations>0&&calibration.allocations<2000,"rich replay/profile allocation calibration"))return false;
        for(long failure=0;failure<calibration.allocations;++failure) {
            BlockSourceProbe reader;reader.allocationPhase=4;reader.failAllocation=failure;dx_data data;
            if(!check(reader.fileImport(path.string(),&data,false)&&A::blockFailed(data)&&!A::blockEndpointProof(data),"rich replay/profile allocation failure stays sticky"))return false;
            reader.allocationPhase=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::blockEndpointProof(retry))return false;++allocations;
        }
    }
    for(bool binary:{false,true})for(unsigned variant=0;variant<7u;++variant) {
        auto fixture=blockFixture("AC1032",1u,true);auto tags=fixture.begin;
        if(variant==0u||variant==1u) {
            tags[14].value=variant==0u?"DECLARED-\\U+00E9":"DECLARED-é";
            tags[19].value=variant==0u?"DECLARED-é":"DECLARED-\\U+00E9";
        }
        if(variant==2u)tags[19].value="CONFLICT";
        if(variant==3u)tags.insert(tags.begin()+20,{3,"DECLARED"});
        if(variant==4u)tags.erase(tags.begin()+19);
        if(variant==5u)tags.erase(tags.begin()+14);
        if(variant==6u)tags[14].value="";
        replaceBlockEndpoint(fixture,false,std::move(tags));
        if(!write(path,fixture.document,binary))return false;
        BlockSourceProbe reader;dx_data data;const bool allowed=variant<=1u||variant==4u;
        if(!check(reader.fileImport(path.string(),&data,false)&&A::blockTransport(data)
            &&A::blockEndpointProof(data)==allowed,"decoded redundant names and tolerated default/conflicting source forms"))return false;
        const auto* row=A::block(data);
        if(variant<=1u&&!check(row->original->name=="DECLARED-é","escaped/literal original BLOCK name decodes identically"))return false;
        if(variant==2u&&!check(row->beginProfile.reason==dx_attdef_source::Reason::RedundantName,"conflicting redundant name explicitly refused"))return false;
        const auto clean=blockFixture("AC1032",1u,true);if(!write(path,clean.document,binary))return false;
        dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::blockEndpointProof(retry))return false;++names;
    }
    using Fault=std::pair<std::string,std::function<void(DRW_Block&)>>;
    std::vector<Fault> faults;
    for(const auto& fault:commonEntitySourceFaults())faults.emplace_back(fault.first,[f=fault.second](DRW_Block& b){f(b);});
    faults.emplace_back("cursor",[](DRW_Block& b){DrwEntityEncodeTestAccess::corruptCursor(b,false);});
    faults.emplace_back("cursor-mask",[](DRW_Block& b){DrwEntityEncodeTestAccess::corruptCursor(b,true);});
    for(unsigned axis=0;axis<3u;++axis) {
        faults.emplace_back("basePoint."+std::to_string(axis),[=](DRW_Block& b){if(axis==0)++b.basePoint.x;else if(axis==1)++b.basePoint.y;else ++b.basePoint.z;});
        faults.emplace_back("extPoint."+std::to_string(axis),[=](DRW_Block& b){if(axis==0)++b.extPoint.x;else if(axis==1)++b.extPoint.y;else ++b.extPoint.z;});
    }
    faults.emplace_back("thickness",[](DRW_Block& b){b.thickness+=1.0;});
    faults.emplace_back("xAxisAngle",[](DRW_Block& b){b.xAxisAngle+=1.0;});
    faults.emplace_back("name",[](DRW_Block& b){b.name="CHANGED";});
    faults.emplace_back("flags",[](DRW_Block& b){b.flags^=1;});
    faults.emplace_back("xrefUnloaded",[](DRW_Block& b){b.xrefUnloaded=!b.xrefUnloaded;});
    faults.emplace_back("xrefPath",[](DRW_Block& b){b.xrefPath="CHANGED";});
    faults.emplace_back("description",[](DRW_Block& b){b.description="CHANGED";});
    faults.emplace_back("hasDescription",[](DRW_Block& b){b.hasDescription=!b.hasDescription;});
    faults.emplace_back("isEnd",[](DRW_Block& b){b.setIsEnd(!b.getIsEnd());});
    faults.emplace_back("insUnits",[](DRW_Block& b){++b.insUnits;});
    faults.emplace_back("previewData",[](DRW_Block& b){b.previewData.push_back(7u);});
    if(!check(faults.size()==102u,"complete common/POINT/BLOCK field fault census"))return false;
    for(bool binary:{false,true})for(bool footer:{false,true})for(const auto& fault:faults) {
        const auto fixture=blockFixture("AC1032",1u,true);if(!write(path,fixture.document,binary))return false;
        BlockSourceProbe reader;(footer?reader.alterEnd:reader.alterBegin)=fault.second;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||A::blockEndpointProof(data)) {
            std::cerr<<"BLOCK field fault escaped endpoint refusal: "<<fault.first<<" footer="<<footer<<" binary="<<binary<<'\n';return false;
        }
        reader.alterBegin={};reader.alterEnd={};dx_data retry;
        if(!reader.fileImport(path.string(),&retry,false)||!A::blockEndpointProof(retry))return false;++fields;
    }
    for(bool binary:{false,true})for(unsigned context=0;context<5u;++context) {
        auto fixture=blockFixture("AC1032",1u,true);
        if(context<=1u) {
            auto tags=context==1u?fixture.end:fixture.begin;
            tags.erase(std::remove_if(tags.begin(),tags.end(),[](const Tag& t){return t.code==1030||t.code==1070;}),tags.end());
            replaceBlockEndpoint(fixture,context==1u,std::move(tags));
        }
        if(context==2u) {auto tags=fixture.end;tags[1].value="99";replaceBlockEndpoint(fixture,true,std::move(tags));}
        if(context==3u)fixture.document.insert(fixture.document.end()-1,{{0,"SECTION"},{2,"ENTITIES"},{0,"LINE"},{5,"not-hex"},{10,"1"},{0,"ENDSEC"}});
        if(!write(path,fixture.document,binary))return false;
        BlockSourceProbe reader;if(context==4u)reader.throwPhase=5;
        dx_data data;bool ok=false,threw=false;try{ok=reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
        if(!check(!A::blockEndpointProof(data),"pending original state, wrong owner or later failure cannot qualify endpoints"))return false;
        if(context<=2u&&!check(ok&&A::blockTransport(data),"tolerant ordinary import retained for partial XDATA/wrong owner"))return false;
        if(context==2u&&!check(A::block(data)->reconciliation==R::WrongOwner,"distinct tolerated footer owner explicitly refused by scoped reconciliation"))return false;
        if(context==3u&&!check(!ok,"later malformed entity fails whole import"))return false;
        if(context==4u&&!check(threw&&data.dxfReadOutcome.readThrew,"post-proof cleanup exception invalidates import"))return false;
        const auto clean=blockFixture("AC1032",1u,true);if(!write(path,clean.document,binary))return false;
        reader.throwPhase=0;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::blockEndpointProof(retry))return false;++contexts;
    }
    std::cout<<"BLOCK rich endpoint profiles="<<profiles<<"; original role spans="<<spans<<"; ledger corruptions="<<corruptions
        <<"; name profiles/retries="<<names<<"; field faults/retries="<<fields<<"; context faults/retries="<<contexts
        <<"; exact/below quotas/retries="<<quotas<<"; rich grammar refusals/retries="<<negatives
        <<"; rich replay allocation faults/retries="<<allocations<<'\n';
    return profiles==8u&&spans==848u&&corruptions==5088u&&names==14u&&fields==408u&&contexts==10u&&quotas==32u&&negatives==96u;
}
