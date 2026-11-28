// SPDX-License-Identifier: GPL-2.0-or-later
// Locally authored controls for the optional AC1018 endpoint transport.
bool legacyBlockCaptureTests(const std::filesystem::path& path) {
    using A=DxIfaceTableSourceTestAccess;
    using Reason=dx_attdef_source::Reason;
    BlockFixture f;
    f.begin={{5,"B1"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},
        {2,"LEGACY_BLOCK"},{70,"0"},{10,"1"},{20,"2"},{30,"3"},{3,"LEGACY_BLOCK"},
        {1,std::string("caf\xE9",4u)}};
    f.end={{5,"B2"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"}};
    f.document={{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1018"},
        {9,"$DWGCODEPAGE"},{3,"ANSI_1252"},{0,"ENDSEC"},{0,"SECTION"},{2,"BLOCKS"},{0,"BLOCK"}};
    f.document.insert(f.document.end(),f.begin.begin(),f.begin.end());
    f.document.push_back({0,"ENDBLK"});f.document.insert(f.document.end(),f.end.begin(),f.end.end());
    f.document.insert(f.document.end(),{{0,"ENDSEC"},{0,"EOF"}});
    std::size_t spans=0u;unsigned captures=0u,defaults=0u,mutations=0u,quotas=0u,retries=0u;
    for(bool binary:{false,true}) {
        if(!write(path,f.document,binary))return false;
        BlockSourceProbe normal;dx_data plain;
        if(!check(normal.fileImport(path.string(),&plain,false)&&!A::block(plain)
            &&normal.sourceBegins==0u&&normal.sourceEnds==0u&&normal.depth==0u,
            "legacy block default opt-out"))return false;
        ++defaults;
        BlockSourceProbe selected;selected.forceTransport=true;dx_data data;
        if(!check(selected.fileImport(path.string(),&data,false)&&A::blockTransport(data)
            &&selected.sourceBegins==1u&&selected.sourceEnds==1u&&selected.depth==0u,
            "legacy block explicit source capture"))return false;
        const auto* block=A::block(data);
        if(!check(block&&block->beginRaw&&block->endRaw&&block->original&&block->effective&&block->footer
            &&block->context&&block->contextMatched&&block->context->captured
            &&block->context->sourceVersion==DRW::AC1018&&block->context->codecVersion==DRW::AC1015
            &&block->context->codecCodePage=="ANSI_1252"&&block->context->hasDeclaration
            &&block->context->declaration=="ANSI_1252"&&block->context->binary==binary&&!block->context->applyExtrusion
            &&block->original->xrefPath=="caf\xC3\xA9"&&block->beginFieldsMatched&&block->footerFieldsMatched
            &&block->beginProfile.complete()&&block->endProfile.complete()
            &&block->reconciliation!=A::BlockReconciliation::Matched&&!A::membershipMatched(data),
            "legacy block replay/profile is context-bound and does not certify provenance"))return false;
        if(!check(blockOriginalWire(path,binary,f,*block->beginRaw,*block->endRaw,spans),
            "legacy block captured originals match physical scalar spans"))return false;
        using namespace dx_attdef_source;
        const auto ledger=[](bool footer) {
            std::vector<Span> expected;
            const auto add=[&](int code,Field field,Role role=Role::None,std::uint64_t handle=0u) {
                expected.push_back({expected.size(),code,Scope::Host,field,role,handle});
            };
            add(5,Field::Identity,Role::Self,footer?0xB2u:0xB1u);add(330,Field::Identity,Role::Owner,0xB0u);
            add(100,Field::Subclass);add(8,Field::Common,Role::LayerName);add(100,Field::Subclass);
            if(!footer) {add(2,Field::Block,Role::BlockName);add(70,Field::Block);
                for(const int code:{10,20,30})add(code,Field::Block);
                add(3,Field::Block,Role::RedundantBlockName);add(1,Field::Block,Role::XrefPath);}
            return expected;
        };
        for(bool footer:{false,true}) {
            const auto& profile=footer?block->endProfile:block->beginProfile;const auto expected=ledger(footer);
            if(!check(profile.spans.size()==expected.size(),"legacy endpoint full source span count"))return false;
            for(std::size_t i=0;i<expected.size();++i) {
                const auto& a=profile.spans[i];const auto& b=expected[i];
                if(!check(a.groupIndex==b.groupIndex&&a.code==b.code&&a.scope==b.scope&&a.field==b.field
                    &&a.role==b.role&&a.handle==b.handle,"legacy endpoint exact source field and reference role"))return false;
            }
            const auto& original=footer?*block->endRaw:*block->beginRaw;
            const auto refusal=[&](std::size_t at,const DRW_Variant& field,Reason reason) {
                auto raw=original;raw.groups.insert(raw.groups.begin()+static_cast<std::ptrdiff_t>(at),field);
                const auto result=dx_block_source::audit(raw,footer,true);
                return check(result.reason==reason&&result.rejectedGroup==at&&result.spans.empty(),
                    "legacy endpoint exact unsupported-field refusal");
            };
            if(!refusal(4u,DRW_Variant(92,std::int32_t(1)),Reason::ProxyGraphics)
                ||!refusal(4u,DRW_Variant(160,std::int64_t(1)),Reason::ProxyGraphics)
                ||!refusal(4u,DRW_Variant(310,std::string("00")),Reason::ProxyGraphics)
                ||!refusal(4u,DRW_Variant(102,std::string("{CUSTOM")),Reason::Application)
                ||!refusal(4u,DRW_Variant(347,std::string("AB")),Reason::UnknownField)
                ||!refusal(original.groups.size(),DRW_Variant(1001,std::string("APP")),Reason::Xdata))return false;
            auto wrong=original;wrong.groups[3]=DRW_Variant(8,std::string("\xE9",1u));
            auto result=dx_block_source::audit(wrong,footer,true);
            if(!check(result.reason==Reason::InvalidValue&&result.rejectedGroup==3u&&result.spans.empty(),
                "legacy endpoint non-ASCII role name refused"))return false;
            wrong=original;wrong.groups[3]=DRW_Variant(8,std::int32_t(0));
            result=dx_block_source::audit(wrong,footer,true);
            if(!check(result.reason==Reason::InvalidValue&&result.rejectedGroup==3u&&result.spans.empty(),
                "legacy endpoint incorrect variant refused"))return false;
            wrong=original;wrong.groups.insert(wrong.groups.begin()+4,DRW_Variant(8,std::string("0")));
            result=dx_block_source::audit(wrong,footer,true);
            if(!check(result.reason==Reason::DuplicateField&&result.rejectedGroup==4u&&result.spans.empty(),
                "legacy endpoint duplicate field refused"))return false;
            wrong=original;wrong.groups.erase(wrong.groups.begin()+3);
            result=dx_block_source::audit(wrong,footer,true);
            if(!check(result.reason==Reason::Identity&&result.rejectedGroup==wrong.groups.size()&&result.spans.empty(),
                "legacy endpoint missing layer refused"))return false;
        }
        auto changed=*block->beginRaw;changed.groups[10]=DRW_Variant(3,std::string("OTHER"));
        const auto nameMismatch=dx_block_source::audit(changed,false,true);
        if(!check(nameMismatch.reason==Reason::RedundantName&&nameMismatch.rejectedGroup==10u&&nameMismatch.spans.empty(),
            "legacy endpoint mismatched redundant raw name refused even if caller agrees"))return false;
        changed=*block->beginRaw;changed.groups[7]=DRW_Variant(10,std::string("1"));
        const auto badCoordinate=dx_block_source::audit(changed,false,true);
        if(!check(badCoordinate.reason==Reason::InvalidValue&&badCoordinate.rejectedGroup==7u&&badCoordinate.spans.empty(),
            "legacy endpoint incorrect coordinate variant refused"))return false;
        for(const auto version:{DRW::AC1021,DRW::AC1024}) {
            auto modern=*block->beginRaw;modern.m_version=version;
            const int code=version==DRW::AC1021?92:160;
            modern.groups.insert(modern.groups.begin()+4,DRW_Variant(code,std::string("1")));
            const auto badCount=dx_block_source::audit(modern,false,true);
            if(!check(badCount.reason==Reason::ProxyGraphics&&badCount.rejectedGroup==4u&&badCount.spans.empty(),
                "modern endpoint proxy wrong count variant refused"))return false;
            modern.groups[4]=version==DRW::AC1021?DRW_Variant(92,std::int32_t(0)):DRW_Variant(160,std::int64_t(0));
            modern.groups.insert(modern.groups.begin()+5,DRW_Variant(310,std::int32_t(0)));
            const auto badChunk=dx_block_source::audit(modern,false,true);
            if(!check(badChunk.reason==Reason::ProxyGraphics&&badChunk.rejectedGroup==5u&&badChunk.spans.empty(),
                "modern endpoint proxy wrong chunk variant refused"))return false;
        }
        ++captures;
        for(bool footer:{false,true}) {
            BlockSourceProbe altered;altered.forceTransport=true;
            auto fault=[](DRW_RawDxfObject& raw) {
                for(std::size_t i=0;i<raw.groups.size();++i)if(raw.groups[i].code()==8) {
                    raw.groups[i].addString(8,"CHANGED");if(raw.hasRawValues)raw.rawValues[i]="CHANGED";return;
                }
            };
            (footer?altered.alterEndRaw:altered.alterBeginRaw)=fault;dx_data bad;
            if(!check(altered.fileImport(path.string(),&bad,false)&&A::blockTransport(bad)
                &&A::block(bad)&&!(footer?A::block(bad)->footerFieldsMatched:A::block(bad)->beginFieldsMatched)
                &&!A::blockEndpointProof(bad),"legacy block raw mutation cannot qualify replay"))return false;
            altered.alterBeginRaw={};altered.alterEndRaw={};dx_data restored;
            if(!check(altered.fileImport(path.string(),&restored,false)&&A::blockTransport(restored)
                &&A::block(restored)->beginFieldsMatched&&A::block(restored)->footerFieldsMatched,
                "legacy block corrected fresh-destination retry"))return false;
            ++mutations;++retries;
        }
        BlockSourceProbe limited;limited.forceTransport=true;dx_data constrained;A::blockLimit(constrained,1u,0u);
        if(!check(limited.fileImport(path.string(),&constrained,false)&&A::blockFailed(constrained)
            &&!A::blockEndpointProof(constrained)&&limited.depth==0u,"legacy block proof quota is unqualified"))return false;
        BlockSourceProbe noCapture;dx_data unconstrained;A::blockLimit(unconstrained,1u,0u);
        if(!check(noCapture.fileImport(path.string(),&unconstrained,false)&&!A::blockFailed(unconstrained)
            &&noCapture.sourceBegins==0u&&noCapture.depth==0u,"legacy default path avoids optional block quota"))return false;
        dx_data recovered;if(!check(limited.fileImport(path.string(),&recovered,false)&&A::blockTransport(recovered),
            "legacy block quota retry on fresh destination"))return false;
        ++quotas;++retries;
        struct ReaderProbe final:dx_iface {
            unsigned depth=0u,begins=0u,ends=0u;
            bool wantsDxfBlockSource()const override{return true;}
            void addHeader(const DRW_Header*)override{}
            void addDxfBlockBeginSource(const DRW_Block&,const DRW_RawDxfObject&)override{++begins;}
            void addDxfBlockEndSource(const DRW_Block&,const DRW_RawDxfObject&)override{++ends;}
            void addBlock(const DRW_Block&)override{++depth;}
            void addBlockEnd(const DRW_Block&)override{}
            void endBlock()override{--depth;}
        };
        dxfRW reader(path.string().c_str());reader.setDxfReadRetainedByteBudget(1u);ReaderProbe rejected;
        if(!check(!reader.read(&rejected,false)&&rejected.depth==0u,"legacy reader capture quota fails cleanly"))return false;
        reader.setDxfReadRetainedByteBudget(1024u*1024u);ReaderProbe retry;
        if(!check(reader.read(&retry,false)&&retry.depth==0u&&retry.begins==1u&&retry.ends==1u,
            "legacy same-reader capture quota retry"))return false;
        ++retries;
    }
    std::cout<<"AC1018 block opt-in="<<captures<<" default="<<defaults<<" spans="<<spans
        <<" mutations="<<mutations<<" quotas="<<quotas<<" retries="<<retries<<'\n';
    return captures==2u&&defaults==2u&&spans==34u&&mutations==4u&&quotas==2u&&retries==8u;
}
