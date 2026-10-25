// SPDX-License-Identifier: GPL-2.0-or-later
// Authored semantic ledger, separate from both shared decoder and profile pass.
std::vector<dx_attdef_source::Span> blockSpanLedger(bool footer) {
    using F=dx_attdef_source::Field;using R=dx_attdef_source::Role;using S=dx_attdef_source::Scope;
    std::vector<dx_attdef_source::Span> result;
    const auto add=[&](int code,F field,R role=R::None,std::uint64_t value=0u){result.push_back({result.size(),code,S::Host,field,role,value});};
    add(5,F::Identity,R::Self,footer?0xB2u:0xB1u);add(330,F::Identity,R::Owner,0xB0u);
    add(100,F::Subclass);add(8,F::Common,R::LayerName);
    if(footer) {
        add(100,F::Subclass);add(102,F::Application);add(5,F::Application,R::OpaqueHandle,0xBEEFu);
        add(330,F::Application,R::OpaqueHandle,0xCAFEu);add(1,F::Application);add(102,F::Application);
        add(1001,F::Xdata,R::AppIdName);
        for(int code:{1010,1020,1030,1070})add(code,F::Xdata);
    } else {
        add(102,F::Application);add(5,F::Application,R::OpaqueHandle,0xDEADu);
        add(330,F::Application,R::OpaqueHandle,0xFACEu);add(1,F::Application);add(1001,F::Application);
        add(102,F::Application);add(360,F::Application,R::OpaqueHandle,0xCAFEu);add(102,F::Application);add(102,F::Application);
        add(100,F::Subclass);add(2,F::Block,R::BlockName);add(70,F::Block);
        for(int code:{10,20,30})add(code,F::Block);
        add(3,F::Block,R::RedundantBlockName);add(1,F::Block,R::XrefPath);add(4,F::Block);
        add(1001,F::Xdata,R::AppIdName);add(1003,F::Xdata,R::XdataLayerName);
        for(int code:{1010,1020,1030,1070})add(code,F::Xdata);
    }
    return result;
}
bool blockLedgerMatches(const dx_attdef_source::Audit& audit,bool footer) {
    const auto expected=blockSpanLedger(footer);
    if(!audit.complete()||audit.spans.size()!=expected.size())return false;
    for(std::size_t i=0;i<expected.size();++i) {
        const auto& a=audit.spans[i];const auto& b=expected[i];
        if(a.groupIndex!=b.groupIndex||a.code!=b.code||a.scope!=b.scope||a.field!=b.field||a.role!=b.role||a.handle!=b.handle)return false;
    }
    return true;
}
#include "block_source_rich_controls.h"

bool blockSourceProofTests(const std::filesystem::path& path) {
    using A=DxIfaceTableSourceTestAccess;using R=A::BlockReconciliation;
    unsigned profiles=0u,bindings=0u,faults=0u,rawFaults=0u,reconcileFaults=0u;std::size_t spans=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})
     for(bool binary:{false,true})for(bool reconcile:{false,true})for(unsigned children:{0u,1u,2u}) {
        const auto fixture=blockFixture(version,children,reconcile);
        if(!write(path,fixture.document,binary))return false;
        BlockSourceProbe reader;dx_data data;
        if(!check(reader.fileImport(path.string(),&data,false)&&A::blockTransport(data),"block proof actual import"))return false;
        const auto* row=A::block(data);
        if(!check(row->beginFieldsMatched&&row->footerFieldsMatched&&row->beginProfile.complete()&&row->endProfile.complete()
            &&row->reconciliation==(reconcile?R::Matched:R::MissingBlockRecord),"shared replay, strict profiles and scoped reconciliation")) {
            std::cerr<<"begin="<<row->beginFieldsMatched<<" end="<<row->footerFieldsMatched<<" profile="
                <<static_cast<unsigned>(row->beginProfile.reason)<<'/'<<static_cast<unsigned>(row->endProfile.reason)
                <<" reconciliation="<<static_cast<unsigned>(row->reconciliation)<<'\n';return false;
        }
        if(!check(blockOriginalWire(path,binary,fixture,*row->beginRaw,*row->endRaw,spans)
            &&blockLedgerMatches(row->beginProfile,false)&&blockLedgerMatches(row->endProfile,true),
            "manual role ledger matches every independent original-file scalar span"))return false;
        ++profiles;if(reconcile)++bindings;
    }
    if(!blockEndpointRichTests(path))return false;
    for(bool binary:{false,true})for(bool footer:{false,true}) {
        const auto clean=blockFixture("AC1032",1u,true);
        for(unsigned fault=0;fault<8u;++fault) {
            auto bad=clean;
            const auto marker=std::find_if(bad.document.begin(),bad.document.end(),[&](const Tag& t){return t.code==0&&t.value==(footer?"ENDBLK":"BLOCK");});
            const auto first=static_cast<std::size_t>(marker-bad.document.begin())+1u;
            auto last=std::find_if(bad.document.begin()+static_cast<std::ptrdiff_t>(first),bad.document.end(),[](const Tag& t){return t.code==0;});
            auto at=std::find_if(bad.document.begin()+static_cast<std::ptrdiff_t>(first),last,[](const Tag& t){return t.code==1001&&t.value!="OPAQUE";});
            if(fault==0)bad.document.insert(at,{300,"unmodeled"});
            if(fault==1)bad.document.insert(at,{{62,"3"},{62,"4"}});
            if(fault==2)bad.document.insert(at,{100,"UnknownSubclass"});
            if(fault==3)bad.document.insert(at,{370,"17"});
            if(fault==4) {
                auto y=std::find_if(at,last,[](const Tag& t){return t.code==1020;});bad.document.insert(y,{1070,"9"});
            }
            if(fault==5)bad.document.insert(at,{{102,"{ACAD_XDICTIONARY"},{360,"D1"},{360,"D2"},{102,"}"}});
            if(fault==6)bad.document.insert(at,{1002,"{"});
            if(fault==7)bad.document.insert(at,{5,"B1"});
            if(!write(path,bad.document,binary))return false;
            BlockSourceProbe reader;dx_data data;const bool ok=reader.fileImport(path.string(),&data,false);
            const auto* row=A::block(data);
            if(!check(!ok||A::blockFailed(data)||!row||!row->beginFieldsMatched||!row->footerFieldsMatched
                ||!row->beginProfile.complete()||!row->endProfile.complete(),"unmodeled/duplicate/partial scalar cannot qualify through replay agreement"))return false;
            if(!write(path,clean.document,binary))return false;
            dx_data retry;if(!check(reader.fileImport(path.string(),&retry,false)&&A::blockEndpointProof(retry),"profile corrected fresh-data retry"))return false;
            ++faults;
        }
        for(unsigned fault=0;fault<3u;++fault) {
            if(!write(path,clean.document,binary))return false;
            BlockSourceProbe reader;
            auto alter=[=](DRW_RawDxfObject& raw) {
                const int code=fault==0?8:fault==1?1001:1;
                for(std::size_t i=0;i<raw.groups.size();++i)if(raw.groups[i].code()==code) {
                    raw.groups[i].addString(code,"CHANGED");if(raw.hasRawValues)raw.rawValues[i]="CHANGED";break;
                }
            };
            (footer?reader.alterEndRaw:reader.alterBeginRaw)=alter;dx_data data;
            if(!check(reader.fileImport(path.string(),&data,false)&&A::blockTransport(data)&&!A::blockEndpointProof(data),"consistent raw spelling/scalar change cannot masquerade as modeled agreement"))return false;
            reader.alterBeginRaw={};reader.alterEndRaw={};dx_data retry;
            if(!check(reader.fileImport(path.string(),&retry,false)&&A::blockEndpointProof(retry),"raw mismatch corrected retry"))return false;++rawFaults;
        }
    }
    for(bool binary:{false,true})for(unsigned fault=0;fault<4u;++fault) {
        const auto fixture=blockFixture("AC1032",1u,true);if(!write(path,fixture.document,binary))return false;
        BlockSourceProbe reader;
        if(fault==0)reader.alterBegin=[](DRW_Block& b){b.name="WRONG";};
        if(fault==1)reader.alterBegin=[](DRW_Block& b){b.insUnits=5;};
        if(fault==2)reader.alterBegin=[](DRW_Block& b){b.previewData.push_back(7);};
        if(fault==3)reader.alterEndRaw=[](DRW_RawDxfObject& b){b.m_version=DRW::AC1021;};
        dx_data data;
        if(!check(reader.fileImport(path.string(),&data,false)&&!A::blockEndpointProof(data),"actual effective values/version cannot bypass immutable BLOCK_RECORD reconciliation"))return false;
        reader.alterBegin={};reader.alterEndRaw={};dx_data retry;
        if(!reader.fileImport(path.string(),&retry,false)||!A::blockEndpointProof(retry))return false;++reconcileFaults;
    }
    const auto fixture=blockFixture("AC1032",1u,true);if(!write(path,fixture.document,false))return false;
    BlockSourceProbe measured;measured.allocationPhase=4;measured.failAllocation=(std::numeric_limits<long>::max)();dx_data sample;
    if(!check(measured.fileImport(path.string(),&sample,false)&&A::blockEndpointProof(sample)&&measured.allocations>0,
        "bounded replay/profile allocation calibration"))return false;
    for(long failure=0;failure<measured.allocations;++failure) {
        BlockSourceProbe reader;reader.allocationPhase=4;reader.failAllocation=failure;dx_data data;
        if(!check(reader.fileImport(path.string(),&data,false)&&A::blockFailed(data)&&!A::blockEndpointProof(data),"replay/profile allocation failure is sticky and unqualified"))return false;
        reader.allocationPhase=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::blockEndpointProof(retry))return false;
    }
    std::cout<<"BLOCK endpoint proof profiles="<<profiles<<"; scoped reconciliations="<<bindings<<"; physical role spans="<<spans
        <<"; profile refusals/retries="<<faults<<"; raw mismatches/retries="<<rawFaults<<"; reconciliation faults/retries="<<reconcileFaults
        <<"; replay allocation faults/retries="<<measured.allocations<<'\n';
    return profiles==48u&&bindings==24u&&spans==2064u&&faults==32u&&rawFaults==12u&&reconcileFaults==8u;
}
