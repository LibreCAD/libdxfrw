// SPDX-License-Identifier: GPL-2.0-or-later

AttdefLedger attdefXrecordLedger(std::uint8_t clone) {
    using S=dx_attdef_source::Scope;
    using F=dx_attdef_source::Field;
    using R=dx_attdef_source::Role;
    AttdefLedger ledger;
    const auto add=[&](int code,std::string value,S scope,F field,
                       R role=R::None,std::uint64_t handle=0u) {
        const auto index=ledger.tags.size();
        ledger.tags.push_back({code,std::move(value)});
        ledger.expected.push_back({index,code,scope,field,role,handle});
    };
    add(5,"A1",S::Host,F::Identity,R::Self,0xA1u);
    add(330,"B0",S::Host,F::Identity,R::Owner,0xB0u);
    add(100,"AcDbEntity",S::Host,F::Subclass);
    add(8,"0",S::Host,F::Common,R::LayerName);
    add(100,"AcDbText",S::Host,F::Subclass);
    add(10,"1",S::Host,F::Text);add(20,"2",S::Host,F::Text);
    add(30,"3",S::Host,F::Text);add(40,"4",S::Host,F::Text);
    add(1,"DEFAULT",S::Host,F::Text);add(7,"STANDARD",S::Host,F::Text,R::TextStyleName);
    add(100,"AcDbAttributeDefinition",S::Host,F::Subclass);
    add(3,"PROMPT",S::Host,F::Attribute);add(2,"TAG",S::Host,F::Attribute);
    add(70,"0",S::Host,F::Attribute);add(73,"0",S::Host,F::Attribute);
    add(74,"0",S::Host,F::Attribute);add(280,"1",S::Host,F::Attribute);
    add(100,"AcDbXrecord",S::Host,F::AttributeXrecord);
    add(280,std::to_string(clone),S::Host,F::AttributeXrecord);
    return ledger;
}

bool attdefXrecordTests(const std::filesystem::path& input,
                       const std::filesystem::path& output) {
    using A=DxIfaceTableSourceTestAccess;
    const auto versionValue=[](const std::string& version) {
        if(version=="AC1021")return DRW::AC1021;
        if(version=="AC1024")return DRW::AC1024;
        if(version=="AC1027")return DRW::AC1027;
        if(version=="AC1032")return DRW::AC1032;
        return DRW::UNKNOWNV;
    };
    std::size_t sourceSpans=0u;
    unsigned sourceProfiles=0u,outputProfiles=0u;
    const auto editable=[](dx_data& data)->DRW_Attdef* {
        DRW_Attdef* result=nullptr;
        const auto visit=[&](dx_ifaceBlock* block) {
            if(block)for(auto* entity:block->ent)
                if(auto* value=dynamic_cast<DRW_Attdef*>(entity)) {
                    if(result)throw std::runtime_error("duplicate Xrecord ATTDEF model");
                    result=value;
                }
        };
        visit(data.mBlock);for(auto* block:data.blocks)visit(block);
        return result;
    };
    const auto outputHasXrecord=[](const std::filesystem::path& path,bool binary,
                                   std::uint8_t clone)->bool {
        const auto wire=attdefWireTags(path,binary);
        std::size_t begin=wire.size(),end=wire.size(),attribute=wire.size(),xrecord=wire.size();
        for(std::size_t i=0;i<wire.size();++i) {
            if(wire[i].code==0&&wire[i].text=="ATTDEF") {
                begin=i+1u;end=wire.size();
                for(std::size_t j=begin;j<wire.size();++j)
                    if(wire[j].code==0){end=j;break;}
                break;
            }
        }
        for(std::size_t i=begin;i<end;++i) {
            if(wire[i].code==100&&wire[i].text=="AcDbAttributeDefinition")
                attribute=i;
            if(wire[i].code==100&&wire[i].text=="AcDbXrecord") {
                xrecord=i;break;
            }
        }
        return begin<end&&attribute<xrecord&&xrecord+1u<end
            &&wire[xrecord+1u].code==280
            &&wire[xrecord+1u].kind==AttdefWireTag::Integer
            &&wire[xrecord+1u].integer==clone;
    };
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})
     for(bool binary:{false,true})for(std::uint8_t clone:{std::uint8_t{0},std::uint8_t{1}}) {
        const auto ledger=attdefXrecordLedger(clone);
        const auto tags=document(version,ledger.tags,false);
        if(!write(input,tags,binary))return false;
        CaptureProbe reader;dx_data data;
        if(!check(reader.fileImport(input.string(),&data,false)&&!A::failed(data),
            "AcDbXrecord source import"))return false;
        const auto* row=A::resource(data);
        if(!row||!row->raw||!row->attdefModel||!row->attdefProfile.complete()
            ||!row->attdefPairMatched||!row->attdefFieldsMatched||row->attdefFieldMismatch
            ||!row->attdefModel->hasDxfXrecord
            ||row->attdefModel->keepDuplicateRecords!=clone
            ||!row->attdefModel->lockPosition) {
            std::cerr<<"Xrecord profile version="<<version<<" binary="<<binary
                <<" clone="<<unsigned(clone)
                <<" row="<<(row!=nullptr)<<" raw="<<(row&&row->raw)
                <<" model="<<(row&&row->attdefModel)
                <<" complete="<<(row&&row->attdefProfile.complete())
                <<" reason="<<(row?static_cast<unsigned>(row->attdefProfile.reason):999u)
                <<" rejected="<<(row?row->attdefProfile.rejectedGroup:0u)
                <<" paired="<<(row&&row->attdefPairMatched)
                <<" fields="<<(row&&row->attdefFieldsMatched)
                <<" mismatch="<<(row&&row->attdefFieldMismatch)
                <<" xrecord="<<(row&&row->attdefModel&&row->attdefModel->hasDxfXrecord)
                <<" cloneValue="<<(row&&row->attdefModel?unsigned(row->attdefModel->keepDuplicateRecords):999u)
                <<" lock="<<(row&&row->attdefModel&&row->attdefModel->lockPosition)<<'\n';
            if(row&&row->raw)for(std::size_t i=0;i<row->raw->groups.size();++i) {
                if(i+2u<row->attdefProfile.rejectedGroup
                    ||i>row->attdefProfile.rejectedGroup+2u)continue;
                const auto& field=row->raw->groups[i];std::cerr<<"  ["<<i<<"] "<<field.code()<<" ";
                if(field.type()==DRW_Variant::STRING&&field.content.s)std::cerr<<*field.content.s;
                else if(field.type()==DRW_Variant::INTEGER)std::cerr<<field.i_val();
                std::cerr<<'\n';
            }
            return false;
        }
        if(!check(row&&row->raw&&row->attdefModel&&row->attdefProfile.complete()
            &&row->attdefPairMatched&&row->attdefFieldsMatched&&!row->attdefFieldMismatch
            &&row->attdefModel->hasDxfXrecord
            &&row->attdefModel->keepDuplicateRecords==clone
            &&row->attdefModel->lockPosition,
            "duplicate group-280 roles remain separate in the source and typed models"))return false;
        if(!attdefOriginalSpans(input,binary,tags,ledger,row->attdefProfile,
                                sourceSpans))return false;
        const auto* model=editable(data);
        if(!check(model&&model->hasDxfXrecord
            &&model->keepDuplicateRecords==clone&&model->lockPosition,
            "editable ATTDEF preserves Xrecord presence, clone policy and lock state"))return false;
        ++sourceProfiles;

        // Exercise the real entity writer across ASCII/binary conversion, then
        // inspect the emitted bytes independently and read them through a fresh
        // adapter. Generated inputs stay in the per-test scratch directory.
        dx_iface exporter;
        const bool outputBinary=!binary;
        if(!check(exporter.fileExport(output.string(),versionValue(version),
                                      outputBinary,&data,false),
            "AcDbXrecord cross-encoding export"))return false;
        if(!check(outputHasXrecord(output,outputBinary,clone),
            "independent output walker sees exact AcDbXrecord clone field"))return false;
        CaptureProbe outputReader;dx_data roundtrip;
        const bool outputImported=outputReader.fileImport(output.string(),&roundtrip,false);
        if(!check(outputImported&&!A::failed(roundtrip),"AcDbXrecord output reimport"))return false;
        const auto* emitted=A::anyAttdefResource(roundtrip);
        if(!emitted||!emitted->attdefProfile.complete()
            ||!emitted->attdefModel||!emitted->attdefModel->hasDxfXrecord
            ||emitted->attdefModel->keepDuplicateRecords!=clone
            ||!emitted->attdefModel->lockPosition) {
            std::cerr<<"Xrecord output version="<<version<<" binary="<<outputBinary
                <<" clone="<<unsigned(clone)<<" imported="<<outputImported
                <<" complete="<<(emitted&&emitted->attdefProfile.complete())
                <<" reason="<<(emitted?static_cast<unsigned>(emitted->attdefProfile.reason):999u)
                <<" rejected="<<(emitted?emitted->attdefProfile.rejectedGroup:0u)
                <<" model="<<(emitted&&emitted->attdefModel)
                <<" present="<<(emitted&&emitted->attdefModel&&emitted->attdefModel->hasDxfXrecord)
                <<" cloneValue="<<(emitted&&emitted->attdefModel?unsigned(emitted->attdefModel->keepDuplicateRecords):999u)
                <<" lock="<<(emitted&&emitted->attdefModel&&emitted->attdefModel->lockPosition)<<'\n';
        }
        if(!check(emitted&&emitted->attdefProfile.complete()
            &&emitted->attdefModel&&emitted->attdefModel->hasDxfXrecord
            &&emitted->attdefModel->keepDuplicateRecords==clone
            &&emitted->attdefModel->lockPosition,
            "fresh typed readback preserves the Xrecord fields"))return false;
        ++outputProfiles;
      }

    // A policy outside LibreDWG's ATTDEF field domain is refused by source
    // admission. Recovery on the same reader proves refusal is per attempt.
    auto invalid=attdefXrecordLedger(2u);
    const auto invalidTags=document("AC1024",invalid.tags,false);
    if(!write(input,invalidTags,false))return false;
    CaptureProbe retryReader;dx_data rejected;
    const bool accepted=retryReader.fileImport(input.string(),&rejected,false);
    const auto* rejectedRow=A::resource(rejected);
    if(!check(!accepted||!rejectedRow||!rejectedRow->attdefProfile.complete(),
        "invalid Xrecord clone policy cannot publish a complete profile"))return false;
    const auto corrected=attdefXrecordLedger(1u);
    const auto correctedTags=document("AC1024",corrected.tags,false);
    if(!write(input,correctedTags,false))return false;
    dx_data retry;
    if(!check(retryReader.fileImport(input.string(),&retry,false)
        &&A::resource(retry)&&A::resource(retry)->attdefProfile.complete()
        &&A::resource(retry)->attdefModel->keepDuplicateRecords==1,
        "corrected source retries through the same adapter"))return false;

    // A retained Xrecord cannot be down-converted to a version that has no
    // matching subclass. Check destination preservation and same-writer retry.
    auto* retryModel=editable(retry);
    if(!retryModel||!retryModel->hasDxfXrecord)return false;
    {
        std::ofstream sentinel(output,std::ios::binary|std::ios::trunc);
        sentinel<<"KEEP";
    }
    dx_iface exportRetry;
    if(!check(!exportRetry.fileExport(output.string(),DRW::AC1015,false,&retry,false),
        "pre-AC1018 Xrecord down-conversion refused"))return false;
    {
        std::ifstream sentinel(output,std::ios::binary);
        std::string content((std::istreambuf_iterator<char>(sentinel)),{});
        if(!check(content=="KEEP","failed downgrade preserves destination bytes"))return false;
    }
    if(!check(exportRetry.fileExport(output.string(),DRW::AC1024,false,&retry,false)
        &&outputHasXrecord(output,false,1),
        "same-writer corrected-version retry emits the clone field"))return false;
    std::cout<<"ATTDEF AcDbXrecord source profiles="<<sourceProfiles
        <<"; exact original spans="<<sourceSpans<<"; output/readback profiles="
        <<outputProfiles<<"; invalid-policy and downgrade retries=2\n";
    return sourceProfiles==16u&&outputProfiles==16u&&sourceSpans>0u;
}
