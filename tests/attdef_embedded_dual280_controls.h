// SPDX-License-Identifier: GPL-2.0-or-later
// Runtime-authored controls: independently witnessed embedded version-zero /
// lock pair, combined with separately witnessed equal host alignment roles.
AttdefLedger embeddedDual280Ledger(bool definition,bool lock) {
    using namespace dx_attdef_source;
    auto ledger=equalAlignmentLedger(definition,false,false,true,true);
    for(std::size_t i=0u;i<ledger.tags.size();++i) {
        if(ledger.tags[i].code==280&&ledger.expected[i].field==Field::Attribute)
            ledger.tags[i].value=lock?"1":"0";
        if(ledger.tags[i].code==100&&ledger.tags[i].value==(definition?"AcDbAttributeDefinition":"AcDbAttribute")) {
            ledger.tags.insert(ledger.tags.begin()+static_cast<std::ptrdiff_t>(i+1u),{280,"0"});
            ledger.expected.insert(ledger.expected.begin()+static_cast<std::ptrdiff_t>(i+1u),
                Span{0u,280,Scope::Host,Field::ReservedDefault,Role::None,0u});
            ++i;
        }
    }
    for(std::size_t i=0u;i<ledger.expected.size();++i)ledger.expected[i].groupIndex=i;
    return ledger;
}

bool embeddedDual280Controls(const DRW_RawDxfObject& original,const AttdefLedger& ledger,
        bool definition,unsigned& rejects,unsigned& classics,unsigned& singles) {
    using namespace dx_attdef_source;
    const auto slot=[&](Field field,int code) {
        for(std::size_t i=0u;i<ledger.expected.size();++i)
            if(ledger.expected[i].field==field&&ledger.expected[i].code==code)return i;
        throw std::runtime_error("embedded dual-280 slot");
    };
    const auto version=slot(Field::ReservedDefault,280),lock=slot(Field::Attribute,280);
    for(unsigned kind=0u;kind<23u;++kind) {
        auto raw=original;auto reason=Reason::AmbiguousVersion;std::size_t at=lock;
        if(kind==0u)raw.groups[version]=DRW_Variant(280,1); // unknown version
        if(kind==1u)raw.groups[lock]=DRW_Variant(280,2); // invalid lock
        if(kind==2u){raw.groups.insert(raw.groups.begin()+static_cast<std::ptrdiff_t>(lock+1u),DRW_Variant(280,1));at=lock+1u;}
        if(kind==3u) { // move version out of its immediate subclass position
            auto value=raw.groups[version];raw.groups.erase(raw.groups.begin()+static_cast<std::ptrdiff_t>(version));
            raw.groups.insert(raw.groups.begin()+static_cast<std::ptrdiff_t>(version+1u),value);
        }
        if(kind==4u) { // second 280 before required flags
            auto value=raw.groups[lock];raw.groups.erase(raw.groups.begin()+static_cast<std::ptrdiff_t>(lock));
            raw.groups.insert(raw.groups.begin()+static_cast<std::ptrdiff_t>(version+1u),value);at=version+1u;
        }
        if(kind==5u) {at=slot(Field::Attribute,11);raw.groups[at]=DRW_Variant(11,5.25);reason=Reason::DuplicateField;}
        if(kind==6u) {raw.groups.erase(raw.groups.begin()+static_cast<std::ptrdiff_t>(slot(Field::Attribute,31)));at=raw.groups.size();reason=Reason::IncompletePoint;}
        if(kind==7u) {const auto marker=slot(Field::EmbeddedMarker,101);raw.groups[marker]=DRW_Variant(100,"Embedded Object");at=marker;}
        if(kind==8u) {raw.groups[slot(Field::Text,11)]=DRW_Variant(11,0.0);at=slot(Field::Attribute,11);
            raw.groups[at]=DRW_Variant(11,-0.0);reason=Reason::DuplicateField;}
        if(kind>=9u&&kind<=13u) { // move second lock past a type/reserved/point component
            const int code=kind==9u?71:kind==10u?72:kind==11u?11:kind==12u?21:31;
            const auto field=kind==10u?Field::ReservedDefault:Field::Attribute;
            const auto target=slot(field,code);auto value=raw.groups[lock];
            raw.groups.erase(raw.groups.begin()+static_cast<std::ptrdiff_t>(lock));
            raw.groups.insert(raw.groups.begin()+static_cast<std::ptrdiff_t>(target),value);
            at=slot(Field::EmbeddedMarker,101);
        }
        if(kind==14u) { // vertical justification after the second 280
            const auto target=slot(Field::Attribute,74);auto value=raw.groups[target];
            raw.groups.erase(raw.groups.begin()+static_cast<std::ptrdiff_t>(target));
            raw.groups.insert(raw.groups.begin()+static_cast<std::ptrdiff_t>(lock),value);
            at=slot(Field::EmbeddedMarker,101);
        }
        if(kind==15u) { // the witnessed embedded dual grammar requires reserved 72=0
            const auto reserved=slot(Field::ReservedDefault,72);
            raw.groups.erase(raw.groups.begin()+static_cast<std::ptrdiff_t>(reserved));
            at=slot(Field::EmbeddedMarker,101)-1u;
        }
        if(kind==16u) { // reserved field precedes the type
            const auto target=slot(Field::Attribute,71),source=slot(Field::ReservedDefault,72);
            auto value=raw.groups[source];raw.groups.erase(raw.groups.begin()+static_cast<std::ptrdiff_t>(source));
            raw.groups.insert(raw.groups.begin()+static_cast<std::ptrdiff_t>(target),value);at=slot(Field::EmbeddedMarker,101);
        }
        if(kind>=17u) { // each component before the type or before the reserved field
            const unsigned offset=kind-17u;const int code=offset%3u==0u?11:offset%3u==1u?21:31;
            const auto source=slot(Field::Attribute,code);
            const auto target=offset<3u?slot(Field::Attribute,71):slot(Field::ReservedDefault,72);
            auto value=raw.groups[source];raw.groups.erase(raw.groups.begin()+static_cast<std::ptrdiff_t>(source));
            raw.groups.insert(raw.groups.begin()+static_cast<std::ptrdiff_t>(target),value);at=slot(Field::EmbeddedMarker,101);
        }
        const auto denied=audit(raw,definition);
        if(denied.complete()||!denied.spans.empty()||denied.reason!=reason||denied.rejectedGroup!=at) {
            std::cerr<<"embedded dual refusal kind="<<kind<<" definition="<<definition
                <<" expected="<<static_cast<int>(reason)<<'/'<<at<<" actual="
                <<static_cast<int>(denied.reason)<<'/'<<denied.rejectedGroup<<'\n';return false;
        }
        ++rejects;
    }
    // A classic 2D record has the same two lock/version roles, but no
    // embedded duplicate alignment or nested body. Do not infer 3D support.
    auto classic=original;classic.groups.resize(lock+1u);
    const auto twoD=audit(classic,definition);
    if(!check(twoD.complete()&&twoD.form==Form::Classic&&twoD.spans[version].field==Field::ReservedDefault
        &&twoD.spans[lock].field==Field::Attribute,"classic dual-280 remains qualified"))return false;
    ++classics;
    // Classic attribute-only alignment is valid without multiline type or
    // reserved 72: it must not inherit the embedded ordering restrictions.
    auto attrOnly=classic;
    for(const int code:{31,21,11}) {
        const auto index=slot(Field::Text,code);
        attrOnly.groups.erase(attrOnly.groups.begin()+static_cast<std::ptrdiff_t>(index));
    }
    attrOnly.groups.emplace_back(11,5.0);attrOnly.groups.emplace_back(21,6.0);
    const auto alignedClassic=audit(attrOnly,definition);
    if(!check(alignedClassic.complete()&&alignedClassic.form==Form::Classic
        &&alignedClassic.spans[version-3u].field==Field::ReservedDefault
        &&alignedClassic.spans[lock-3u].field==Field::Attribute
        &&alignedClassic.spans[alignedClassic.spans.size()-2u].field==Field::Attribute
        &&alignedClassic.spans.back().field==Field::Attribute,
        "classic dual-280 attribute-only XY remains qualified"))return false;
    ++classics;
    // Singleton and absent 280 controls retain their earlier source profiles.
    for(bool one:{false,true}) {
        auto single=original;single.groups.erase(single.groups.begin()+static_cast<std::ptrdiff_t>(version));
        if(!one)single.groups.erase(single.groups.begin()+static_cast<std::ptrdiff_t>(lock-1u));
        const auto result=audit(single,definition);
        if(!check(result.complete()&&result.form==Form::Embedded101,"embedded singleton/no-280 remains qualified"))return false;
        ++singles;
    }
    return true;
}

bool embeddedDual280Tests(const std::filesystem::path& path) {
    using A=DxIfaceTableSourceTestAccess;using namespace dx_attdef_source;
    unsigned definitions=0u,children=0u,rejects=0u,classics=0u,singles=0u,roundtrips=0u;
    std::size_t spans=0u,compoundSpans=0u,comments=0u,originalSpans=0u;
    for(bool binary:{false,true})for(bool block:{false,true})for(bool lock:{false,true}) {
        const auto ledger=embeddedDual280Ledger(true,lock);const auto tags=document("AC1032",ledger.tags,block);
        if(!write(path,tags,binary))return false;CaptureProbe reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||A::failed(data))return false;const auto* row=A::resource(data);
        if(!check(row&&row->raw&&row->attdefModel&&row->attdefPairMatched&&row->attdefFieldsMatched
            &&!row->attdefFieldMismatch&&row->attdefProfile.complete()&&row->attdefModel->lockPosition==lock
            &&equalAlignmentSpans(row->raw->groups,row->attdefProfile,ledger)
            &&attdefOriginalSpans(path,binary,tags,ledger,row->attdefProfile,spans),"embedded dual ATTDEF source and spans"))return false;
        if(!embeddedDual280Controls(*row->raw,ledger,true,rejects,classics,singles))return false;
        if(lock)for(bool outputBinary:{false,true}) {
            if(!check(equalAlignmentLockRoundTrip(path,outputBinary,row->attdefModel.get(),nullptr),"embedded dual ATTDEF compatible output"))return false;
            ++roundtrips;
        }
        ++definitions;
    }
    for(bool binary:{false,true})for(bool lock:{false,true}) {
        const auto ledger=embeddedDual280Ledger(false,lock);auto fixture=compoundFixture("AC1032",4u);
        replaceCompoundRecord(fixture.document,"E2","ATTRIB",ledger.tags);
        if(!write(path,fixture.document,binary))return false;CompoundBodyProbe reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||!A::membershipMatched(data)
            ||!compoundProfileMatches(data,3u)||!compoundProfileInventory(data,3u,compoundSpans,comments)
            ||!originalChildrenWire(path,binary,fixture,reader.childrenAlias,originalSpans))return false;
        const auto* body=A::compoundBody(data,3u);if(!body||!body->membership||body->profiles.size()!=3u)return false;
        const auto range=body->membership->ranges[3u];const auto& physical=body->membership->children->records[range.first+1u];
        const auto* parent=dynamic_cast<const DRW_Insert*>(body->model.get());
        if(!check(physical.name=="ATTRIB"&&equalAlignmentSpans(physical.groups,body->profiles[1],ledger)
            &&parent&&parent->attlist.size()==1u&&parent->attlist[0]&&parent->attlist[0]->mtext
            &&parent->attlist[0]->lockPosition==lock&&parent->attlist[0]->parentHandle==parent->handle
            &&parent->seqEndEntity&&parent->seqEndEntity->parentHandle==parent->handle,"embedded dual ATTRIB compound source"))return false;
        DRW_RawDxfObject raw;raw.name=physical.name;raw.handle=0xE2u;raw.parentHandle=0xE1u;raw.m_version=DRW::AC1032;raw.groups=physical.groups;
        if(!embeddedDual280Controls(raw,ledger,false,rejects,classics,singles))return false;
        if(lock)for(bool outputBinary:{false,true}) {
            if(!check(equalAlignmentLockRoundTrip(path,outputBinary,nullptr,parent),"embedded dual ATTRIB compatible output"))return false;
            ++roundtrips;
        }
        ++children;
    }
    std::cout<<"Embedded dual280 ATTDEF="<<definitions<<" ATTRIB="<<children<<" refusals="<<rejects
        <<" classic="<<classics<<" singleton/absent="<<singles<<" roundtrips="<<roundtrips
        <<" source spans="<<spans<<" compound spans="<<compoundSpans<<'\n';
    return definitions==8u&&children==4u&&rejects==276u&&classics==24u&&singles==24u&&roundtrips==12u;
}
