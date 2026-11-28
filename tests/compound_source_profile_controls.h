// SPDX-License-Identifier: GPL-2.0-or-later
bool compoundProfileMatches(const dx_data& data,std::size_t dispatch) {
    using A=DxIfaceTableSourceTestAccess;
    if(A::compoundSourceMatched(data,dispatch))return true;
    std::cerr<<"compound profile dispatch="<<dispatch<<" modeled="<<A::compoundMatched(data,dispatch)<<" capture="<<A::compoundFailed(data);
    if(const auto* body=A::compoundBody(data,dispatch))for(std::size_t i=0;i<body->profiles.size();++i)
        std::cerr<<" member "<<i<<" reason="<<static_cast<int>(body->profiles[i].reason)<<" at="<<body->profiles[i].rejectedGroup;
    std::cerr<<'\n';return false;
}
bool compoundProfileInventory(const dx_data& data,std::size_t dispatch,std::size_t& spans,std::size_t& comments) {
    using A=DxIfaceTableSourceTestAccess;using namespace dx_attdef_source;
    const auto* body=A::compoundBody(data,dispatch);if(!body)return false;
    const auto& audit=*body->membership;const auto range=audit.ranges[dispatch];
    if(body->profiles.size()!=range.next-range.first)return false;
    for(std::size_t r=0;r<body->profiles.size();++r) {
        const auto& profile=body->profiles[r];if(!profile.complete())continue;
        const auto& physical=audit.children->records[range.first+r];const auto& member=audit.members[range.first+r];
        if(profile.spans.size()!=physical.groups.size())return false;
        unsigned depth=0u;std::string application;bool face=false;
        for(std::size_t i=0;i<profile.spans.size();++i) {
            const auto& p=profile.spans[i];const auto& v=physical.groups[i];
            if(p.groupIndex!=i||p.code!=v.code())return false;
            if(physical.spans[i].ignoredComment) {
                if(p.scope!=Scope::Ignored||p.field!=Field::IgnoredComment||p.role!=Role::None)return false;++comments;
            }else if(p.scope==Scope::Ignored||p.field==Field::IgnoredComment)return false;
            if(physical.spans[i].ignoredComment){++spans;continue;}
            if(v.code()==102) {
                const std::string token=v.c_str();
                if(token=="}"){if(!depth)return false;--depth;}
                else {if(!depth)application=token;++depth;}
            }else {
                Role expected=Role::None;
                if(depth) {
                    if(dx_attdef_source::handleCode(v.code()))expected=application=="{ACAD_REACTORS"?Role::Reactor:
                        application=="{ACAD_XDICTIONARY"?Role::ExtensionDictionary:Role::OpaqueHandle;
                    if(p.field!=Field::Application)return false;
                }else {
                    switch(v.code()) {
                    case 5:expected=Role::Self;break;case 330:expected=Role::Owner;break;
                    case 8:expected=Role::LayerName;break;case 6:expected=Role::LineTypeName;break;case 7:expected=Role::TextStyleName;break;
                    case 347:expected=Role::Material;break;case 348:expected=Role::FullVisualStyle;break;
                    case 390:expected=Role::PlotStyle;break;case 360:expected=Role::ExtensionDictionary;break;
                    case 1001:expected=Role::AppIdName;break;case 1003:expected=Role::XdataLayerName;break;case 1005:expected=Role::XdataObjectId;break;
                    case 2:if(physical.name=="INSERT"||physical.name=="MINSERT")expected=Role::BlockName;break;
                    case 100:if(std::string(v.c_str())=="AcDbFaceRecord")face=true;break;
                    default:break;
                    }
                    if((physical.name=="POLYLINE"&&(v.code()==66||v.code()==10||v.code()==20))
                        ||(face&&(v.code()==10||v.code()==20||v.code()==30)))if(p.field!=Field::ComputedIgnored)return false;
                }
                if(p.role!=expected)return false;
                if(expected==Role::Self||expected==Role::Owner||expected==Role::Material||expected==Role::FullVisualStyle
                    ||expected==Role::PlotStyle||expected==Role::Reactor||expected==Role::ExtensionDictionary||expected==Role::XdataObjectId||expected==Role::OpaqueHandle)
                    if(p.handle!=std::stoull(v.c_str(),nullptr,16))return false;
            }
            if(i==member.selfGroup&&(p.role!=Role::Self||p.handle!=member.self))return false;
            if(i==member.ownerGroup&&(p.role!=Role::Owner||p.handle!=member.owner))return false;
            if(p.field==Field::Compound&&p.code==2&&physical.name=="INSERT"&&p.role!=Role::BlockName)return false;
            ++spans;
        }
    }
    return true;
}
void compoundAppend(Tags& tags,const char* id,Tags extra) {
    auto at=membershipSlot(tags,id,5);while(tags[at].code!=0&&tags[at].code!=1001)++at;
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),extra.begin(),extra.end());
}
BlockFixture compoundDual280Fixture(const std::string& version,bool lock,unsigned embedded=0u) {
    auto fixture=compoundFixture(version,embedded==1u?4u:embedded==2u?5u:0u);
    auto& tags=fixture.document;auto marker=membershipSlot(tags,"E2",100);
    while(tags[marker].value!="AcDbAttribute")++marker;
    auto last=marker+1u;while(tags[last].code!=280)++last;tags[last].value=lock?"1":"0";
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(marker+1u),{{280,"0"},{102,"{VERSION_SHADOW"},
        {280,"7"},{3,"not-a-prompt"},{2,"NOT_TAG"},{70,"15"},{102,"}"}});
    if(embedded>=3u) {
        // Reuse a complete, previously qualified MTEXT body after the dual
        // pair; an early !nested check alone cannot reject this sequence.
        const auto source=body(embedded==3u?1u:2u);Tags nested;bool selected=false;
        for(const auto& tag:source) {
            if((tag.code==100||tag.code==101)&&tag.value=="Embedded Object")selected=true;
            if(selected&&tag.code==100&&tag.value=="AcDbAttributeDefinition")break;
            if(selected)nested.push_back(tag);
        }
        if(nested.empty())throw std::runtime_error("dual280 complete embedded body");
        compoundAppend(tags,"E2",nested);
    }
    return fixture;
}
std::array<std::size_t,2> compoundDual280Slots(const Tags& tags) {
    auto at=membershipSlot(tags,"E2",100);while(tags[at].value!="AcDbAttribute")++at;
    std::array<std::size_t,2> slots{{tags.size(),tags.size()}};unsigned count=0u,depth=0u;
    for(++at;tags[at].code!=0;++at) {
        if(tags[at].code==102){depth=tags[at].value=="}"?depth-1u:depth+1u;continue;}
        if(!depth&&tags[at].code==280){if(count==2u)throw std::runtime_error("authored dual280 tuple count");slots[count++]=at;}
    }
    if(count!=2u)throw std::runtime_error("authored dual280 tuple missing");
    return slots;
}
template<class Physical>
bool compoundDual280Roles(const Physical& physical,const dx_attdef_source::Audit& audit,bool lock) {
    using namespace dx_attdef_source;
    if(!audit.complete()||audit.form!=Form::Classic||physical.name!="ATTRIB"||audit.spans.size()!=physical.groups.size())return false;
    std::array<std::size_t,2> indices{{physical.groups.size(),physical.groups.size()}};std::size_t marker=physical.groups.size();
    unsigned depth=0u,count=0u;bool attribute=false;
    for(std::size_t i=0u;i<physical.groups.size();++i) {
        const auto& value=physical.groups[i];if(physical.spans[i].ignoredComment)continue;
        if(value.code()==102){depth=std::string(value.c_str())=="}"?depth-1u:depth+1u;continue;}
        if(depth) {if(value.code()==280&&audit.spans[i].field!=Field::Application)return false;continue;}
        if(value.code()==100){attribute=std::string(value.c_str())=="AcDbAttribute";if(attribute)marker=i;}
        if(attribute&&value.code()==3)return false; // no owning ATTDEF prompt
        if(attribute&&value.code()==280){if(count==2u)return false;indices[count++]=i;}
    }
    if(count!=2u||indices[0]!=marker+1u)return false;
    for(unsigned n=0u;n<2u;++n) {
        const auto i=indices[n];const auto& span=audit.spans[i];const auto& value=physical.groups[i];
        if(span.groupIndex!=i||span.code!=280||span.scope!=Scope::Host||span.role!=Role::None||span.handle
            ||span.field!=(n?Field::Attribute:Field::ReservedDefault)||value.i_val()!=(n?int(lock):0))return false;
    }
    return true;
}
bool compoundDual280Writer(const std::filesystem::path& path,DRW::Version version,bool binary,bool lock) {
    // A clean modeled INSERT writer control; it certifies normalization and
    // ordinary readback, not original-byte replay or document dependency closure.
    struct Output final:dx_iface {
        dx_data data;DRW_Insert parent;DRW_Attdef definition;
        explicit Output(bool locked) {
            cData=&data;currentBlock=data.mBlock;parent.handle=0xBEEFu;parent.name="RESOLVED";parent.basePoint={1,2,3};
            definition.handle=0xBABEu;definition.tag="DUAL_TAG";definition.prompt="version zero";definition.text="default";
            definition.height=1.;definition.basePoint={-1,-2,-3};definition.attVersion=0;definition.lockPosition=locked;
            auto child=std::make_shared<DRW_Attrib>();child->handle=0xCAFEu;child->parentHandle=parent.handle;child->tag="DUAL_TAG";
            child->text="value";child->height=1.;child->basePoint={4,5,6};child->attVersion=0;child->lockPosition=locked;parent.attlist.push_back(child);
            parent.seqEndEntity=std::make_shared<DRW_SeqEnd>();parent.seqEndEntity->handle=0xCAFFu;parent.seqEndEntity->parentHandle=parent.handle;
        }
        void writeEntities()override{(void)dxfW->writeAttdef(&definition,DRW::DxfModelSpaceBlockRecordHandle);(void)dxfW->writeInsert(&parent);}
    } output(lock);
    dxfRW writer(path.string().c_str());output.dxfW=&writer;
    if(!writer.write(&output,version,binary))return false;
    const auto wire=attdefWireTags(path,binary);unsigned parents=0u,children=0u,footers=0u,locks=0u,owners=0u,parentOwners=0u,definitions=0u,definitionLocks=0u,definitionOwners=0u;std::string record;bool attribute=false;
    std::uint32_t parent=0u,child=0u,footer=0u,definition=0u;
    for(const auto& field:wire) {
        if(field.code==0){record=field.text;attribute=false;if(record=="INSERT")++parents;if(record=="ATTRIB")++children;if(record=="SEQEND")++footers;if(record=="ATTDEF")++definitions;continue;}
        if(record=="ATTDEF") {
            if(field.code==5)definition=static_cast<std::uint32_t>(std::stoul(field.text,nullptr,16));
            if(field.code==330){if(std::stoul(field.text,nullptr,16)!=DRW::DxfModelSpaceBlockRecordHandle)return false;++definitionOwners;}
            if(field.code==100)attribute=field.text=="AcDbAttributeDefinition";
            if(field.code==280){if(!attribute||field.kind!=AttdefWireTag::Integer||field.integer!=int(lock))return false;++definitionLocks;}
            continue;
        }
        if(record!="INSERT"&&record!="ATTRIB"&&record!="SEQEND")continue;
        if(field.code==5){const auto id=static_cast<std::uint32_t>(std::stoul(field.text,nullptr,16));if(record=="INSERT")parent=id;else if(record=="ATTRIB")child=id;else footer=id;}
        if(field.code==330) {
            if(record=="INSERT"){if(std::stoul(field.text,nullptr,16)!=DRW::DxfModelSpaceBlockRecordHandle)return false;++parentOwners;}
            else {if(std::stoul(field.text,nullptr,16)!=parent)return false;++owners;}
        }
        if(record=="ATTRIB") {
            if(field.code==100)attribute=field.text=="AcDbAttribute";
            if(field.code==280){if(!attribute||field.kind!=AttdefWireTag::Integer||field.integer!=int(lock))return false;++locks;}
        }
    }
    if(parents!=1u||children!=1u||footers!=1u||locks!=1u||owners!=2u||parentOwners!=1u||definitions!=1u||definitionOwners!=1u||definitionLocks!=1u
        ||!definition||!parent||!child||!footer||parent==child||child==footer||parent==footer||definition==parent||definition==child||definition==footer)return false;
    dx_iface reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;unsigned found=0u,foundDefinition=0u;
    for(const auto* entity:data.mBlock->ent)if(const auto* value=dynamic_cast<const DRW_Attdef*>(entity)) {
        if(value->handle!=definition||value->parentHandle!=DRW::DxfModelSpaceBlockRecordHandle||value->attVersion!=0||value->lockPosition!=lock
            ||value->tag!="DUAL_TAG"||value->prompt!="version zero"||value->text!="default"||value->height!=1.
            ||value->basePoint.x!=-1.||value->basePoint.y!=-2.||value->basePoint.z!=-3.)return false;
        ++foundDefinition;
    }
    for(const auto* entity:data.mBlock->ent)if(const auto* value=dynamic_cast<const DRW_Insert*>(entity)) {
        if(value->handle!=parent||value->parentHandle!=DRW::DxfModelSpaceBlockRecordHandle||value->attlist.size()!=1u||!value->attlist[0]||value->attlist[0]->handle!=child||value->attlist[0]->parentHandle!=parent
            ||value->attlist[0]->tag!="DUAL_TAG"||value->attlist[0]->text!="value"||value->attlist[0]->attVersion!=0||value->attlist[0]->lockPosition!=lock
            ||value->basePoint.x!=1.||value->basePoint.y!=2.||value->basePoint.z!=3.||value->attlist[0]->height!=1.
            ||value->attlist[0]->basePoint.x!=4.||value->attlist[0]->basePoint.y!=5.||value->attlist[0]->basePoint.z!=6.
            ||!value->seqEndEntity||value->seqEndEntity->handle!=footer||value->seqEndEntity->parentHandle!=parent)return false;
        ++found;
    }
    return found==1u&&foundDefinition==1u;
}
bool compoundDual280SourceTests(const std::filesystem::path& path) {
    using A=DxIfaceTableSourceTestAccess;using namespace dx_attdef_source;
    unsigned profiles=0u,roleCorruptions=0u,refusals=0u,retries=0u,singles=0u,legacyRefusals=0u,mutations=0u,writerForms=0u;std::size_t spans=0u,comments=0u,originalSpans=0u;
    const auto positive=[&](const BlockFixture& fixture,bool binary,bool lock,CompoundBodyProbe& reader,dx_data& data) {
        if(!write(path,fixture.document,binary)||!reader.fileImport(path.string(),&data,false)||!A::membershipMatched(data)
            ||!compoundProfileMatches(data,3u)||!compoundProfileInventory(data,3u,spans,comments)
            ||!originalChildrenWire(path,binary,fixture,reader.childrenAlias,originalSpans))return false;
        const auto& body=*A::compoundBody(data,3u);const auto& membership=*body.membership;const auto range=membership.ranges[3u];
        const auto* parent=dynamic_cast<const DRW_Insert*>(body.model.get());
        if(range.next-range.first!=3u||body.profiles.size()!=3u||!parent||parent->handle!=0xE1u||parent->parentHandle!=0xB0u||parent->attlist.size()!=1u
            ||!parent->attlist[0]||parent->attlist[0]->handle!=0xE2u||parent->attlist[0]->parentHandle!=0xE1u||parent->attlist[0]->attVersion!=0
            ||parent->attlist[0]->lockPosition!=lock||parent->attlist[0]->mtext||!parent->seqEndEntity||parent->seqEndEntity->handle!=0xE3u
            ||parent->seqEndEntity->parentHandle!=0xE1u||body.sourceStates.size()!=3u||body.replayStates.size()!=3u)return false;
        const std::array<std::uint32_t,3> ids{{0xE1u,0xE2u,0xE3u}};
        for(unsigned n=0u;n<3u;++n) {
            const auto& member=membership.members[range.first+n];
            if(member.self!=ids[n]||member.owner!=(n?0xE1u:0xB0u)||member.parent!=range.first||member.dispatch!=3u
                ||member.role!=(n==0u?dx_block_membership::Role::Primary:n==1u?dx_block_membership::Role::Attribute:dx_block_membership::Role::SequenceEnd)
                ||member.reason!=dx_block_membership::Reason::Matched)return false;
        }
        return compoundDual280Roles(membership.children->records[range.first+1u],body.profiles[1],lock);
    };
    for(const std::string version:{"AC1024","AC1027","AC1032"})for(bool binary:{false,true})for(bool lock:{false,true}) {
        const auto fixture=compoundDual280Fixture(version,lock);CompoundBodyProbe reader;dx_data data;
        if(!check(positive(fixture,binary,lock,reader,data),"classic ATTRIB dual280 exact original compound replay and field roles"))return false;
        const auto& body=*A::compoundBody(data,3u);const auto range=body.membership->ranges[3u];const auto& physical=body.membership->children->records[range.first+1u];
        for(unsigned kind=0u;kind<4u;++kind) {
            auto changed=body.profiles[1];std::array<std::size_t,2> indices{{0u,0u}};
            for(const auto& span:changed.spans) {
                if(span.code==280&&span.field==Field::ReservedDefault)indices[0]=span.groupIndex;
                else if(span.code==280&&span.field==Field::Attribute)indices[1]=span.groupIndex;
            }
            if(kind==0u)changed.spans[indices[0]].field=Field::Attribute;
            if(kind==1u)changed.spans[indices[1]].field=Field::ReservedDefault;
            if(kind==2u)std::swap(changed.spans[indices[0]].groupIndex,changed.spans[indices[1]].groupIndex);
            if(kind==3u)changed.spans[indices[0]].scope=Scope::Nested;
            if(compoundDual280Roles(physical,changed,lock))return false;
            ++roleCorruptions;
        }
        ++profiles;
    }
    for(const std::string version:{"AC1024","AC1027","AC1032"})for(bool binary:{false,true})for(unsigned kind=0u;kind<14u;++kind) {
        auto fixture=compoundDual280Fixture(version,true,kind>=10u?kind-9u:0u);auto& tags=fixture.document;const auto role=compoundDual280Slots(tags);
        if(kind==0u)tags[role[0]].value="1";if(kind==1u)tags[role[0]].value="-1";if(kind==2u)tags[role[1]].value="2";
        if(kind==3u){tags[role[0]].value="1";tags[role[1]].value="0";}
        if(kind==4u) {
            const auto prefix=tags[role[0]];tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(role[0]));unsigned depth=0u;std::size_t tag=tags.size();
            for(auto at=role[0];tags[at].code!=0;++at) {
                if(tags[at].code==102){depth=tags[at].value=="}"?depth-1u:depth+1u;continue;}
                if(!depth&&tags[at].code==2){tag=at;break;}
            }
            if(tag==tags.size())throw std::runtime_error("dual280 owning ATTRIB tag");
            tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(tag+1u),prefix);
        }
        if(kind==5u)tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(role[1]+1u),{280,"1"});
        if(kind==6u)tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(role[0]+1u),{280,"0"});
        if(kind==7u||kind==8u) {
            unsigned depth=0u;auto at=role[0]+1u;for(;tags[at].code!=0;++at) {
                if(tags[at].code==102){depth=tags[at].value=="}"?depth-1u:depth+1u;continue;}
                if(!depth&&tags[at].code==(kind==7u?2:70)){tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at));break;}
            }
        }
        if(kind==9u)tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(role[1]),{{100,"AcDbXrecord"},{280,"0"}});
        if(!write(path,tags,binary))return false;CompoundBodyProbe reader;dx_data data;const bool imported=reader.fileImport(path.string(),&data,false);
        if(A::compoundSourceMatched(data,3u)){std::cerr<<"ATTRIB malformed dual accepted version="<<version<<" binary="<<binary<<" kind="<<kind<<'\n';return false;}
        if(kind>=10u) {
            const auto* body=A::compoundBody(data,3u);const auto expected=kind<12u&&version!="AC1032"?Reason::Revision:Reason::AmbiguousVersion;
            if(!imported||!body||body->profiles.size()!=3u||body->profiles[1].reason!=expected||!body->profiles[1].spans.empty())return false;
        }
        ++refusals;const auto corrected=compoundDual280Fixture(version,true);dx_data retry;
        if(!positive(corrected,binary,true,reader,retry))return false;
        ++retries;
    }
    for(bool binary:{false,true})for(bool lock:{false,true}) {
        auto fixture=compoundFixture("AC1021",0u);fixture.document[membershipSlot(fixture.document,"E2",280)].value=lock?"1":"0";
        if(!write(path,fixture.document,binary))return false;CompoundBodyProbe reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||!compoundProfileMatches(data,3u))return false;
        const auto& body=*A::compoundBody(data,3u);const auto* parent=dynamic_cast<const DRW_Insert*>(body.model.get());unsigned fields=0u;
        if(!parent||parent->attlist.size()!=1u||parent->attlist[0]->attVersion!=0||parent->attlist[0]->lockPosition!=lock)return false;
        for(const auto& span:body.profiles[1].spans)if(span.code==280&&span.field==Field::Attribute)++fields;
        if(fields!=1u)return false;
        ++singles;
        const auto dual=compoundDual280Fixture("AC1021",lock);if(!write(path,dual.document,binary))return false;dx_data bad;
        if(!reader.fileImport(path.string(),&bad,false)||A::compoundSourceMatched(bad,3u)||!A::compoundBody(bad,3u)
            ||A::compoundBody(bad,3u)->profiles[1].reason!=Reason::AmbiguousVersion)return false;
        ++legacyRefusals;dx_data retry;if(!write(path,fixture.document,binary)||!reader.fileImport(path.string(),&retry,false)||!A::compoundSourceMatched(retry,3u))return false;
    }
    for(bool binary:{false,true}) {
        const auto fixture=compoundDual280Fixture("AC1032",true);CompoundBodyProbe reader;reader.fault=26;dx_data bad;
        if(!write(path,fixture.document,binary)||!reader.fileImport(path.string(),&bad,false)||A::compoundMatched(bad,3u)||A::compoundSourceMatched(bad,3u))return false;
        reader.fault=-1;dx_data corrected;if(!positive(fixture,binary,true,reader,corrected))return false;
        ++mutations;
    }
    for(const auto version:{DRW::AC1024,DRW::AC1027,DRW::AC1032})for(bool binary:{false,true})for(bool lock:{false,true}) {
        if(!compoundDual280Writer(path,version,binary,lock)){std::cerr<<"ATTRIB canonical writer failed version="<<version<<" binary="<<binary<<" lock="<<lock<<'\n';return false;}
        ++writerForms;
    }
    std::cout<<"ATTRIB dual280 compounds profiles="<<profiles<<"; role corruptions="<<roleCorruptions<<"; malformed/retries="<<refusals<<'/'<<retries
        <<"; AC1021 single/dual="<<singles<<'/'<<legacyRefusals<<"; callback lock mutations="<<mutations<<"; canonical single-lock writer forms="<<writerForms
        <<"; independent original scalar spans="<<originalSpans<<"; complete profile spans="<<spans<<"; comments="<<comments<<'\n';
    return profiles==12u&&roleCorruptions==48u&&refusals==84u&&retries==84u&&singles==4u&&legacyRefusals==4u&&mutations==2u&&writerForms==12u;
}
bool compoundSourceProfileTests(const std::filesystem::path& path) {
    using A=DxIfaceTableSourceTestAccess;using namespace dx_attdef_source;
    if(!compoundDual280SourceTests(path))return false;
    unsigned profiles=0u,complete=0u,revisions=0u,variants=0u,faults=0u,contexts=0u,scalarFaults=0u,spanFaults=0u;
    std::size_t spans=0u,comments=0u,originalSpans=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0;encoding<3u;++encoding)for(unsigned form=0;form<7u;++form) {
        const auto f=compoundFixture(version,form);if(!write(path,f.document,encoding==2u,encoding==1u))return false;
        CompoundBodyProbe reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||!A::compoundMatched(data,2u)||!A::compoundMatched(data,3u)
            ||!compoundProfileMatches(data,2u)||!originalChildrenWire(path,encoding==2u,f,reader.childrenAlias,originalSpans))return false;
        ++complete;const bool supported=(form!=4u&&form!=5u)||version=="AC1032";
        if(supported){if(!compoundProfileMatches(data,3u))return false;++complete;}
        else {
            const auto* body=A::compoundBody(data,3u);
            if(A::compoundSourceMatched(data,3u)||!body||body->profiles[1].reason!=Reason::Revision)return false;++revisions;
        }
        if(!compoundProfileInventory(data,2u,spans,comments)||!compoundProfileInventory(data,3u,spans,comments)
            ||A::originResource(data)->rawTypedMatched)return false;
        ++profiles;
    }
    std::cout<<"Compound physical initial profiles="<<profiles<<"; complete bodies="<<complete<<"; documented revision refusals="<<revisions<<std::endl;
    for(bool binary:{false,true}) {
        CompoundBodyProbe reader;
        for(unsigned mode=0;mode<10u;++mode) {
            auto f=compoundFixture("AC1032",mode==0u?4u:mode==7u?3u:0u);
            if(mode==0u) {
                // Every scalar gets adjacent ignored records, including inside
                // application, embedded MTEXT, XDATA coordinates and the tail.
                for(std::size_t j=f.document.size();j-->0u;)if(f.document[j].code==0&&f.document[j].value!="ENDBLK") {
                    const auto& name=f.document[j].value;
                    if(name!="POLYLINE"&&name!="VERTEX"&&name!="INSERT"&&name!="ATTRIB"&&name!="SEQEND")continue;
                    auto end=j+1u;while(f.document[end].code!=0)++end;
                    for(auto i=end;i-->j+1u;)f.document.insert(f.document.begin()+static_cast<std::ptrdiff_t>(i+1u),{{999,"ignored-1"},{999,"ignored-2"}});
                }
            }
            if(mode==1u)for(const char* id:{"D1","D2","E1","E3"}) {
                const auto at=membershipSlot(f.document,id,330);const auto owner=f.document[at];f.document.erase(f.document.begin()+static_cast<std::ptrdiff_t>(at));
                const auto after=membershipSlot(f.document,id,100)+1u;f.document.insert(f.document.begin()+static_cast<std::ptrdiff_t>(after),owner);
            }
            if(mode==2u)compoundAppend(f.document,"D2",{{102,"{CUSTOM"},{100,"AcDbFaceRecord"},{5,"DEAD"},{330,"BEEF"},{320,"FFFFFFFFFFFFFFFF"},
                {8,"not-layer"},{6,"not-line-type"},{347,"FFFFFFFF00000001"},{348,"100000000"},{10,"91"},{102,"{INNER"},{70,"128"},{102,"}"},{102,"}"}});
            if(mode==3u)f.document[membershipSlot(f.document,"D1",66)].value="2";
            if(mode==4u) {
                eraseMembershipRecord(f.document,"E2");eraseMembershipRecord(f.document,"E3");
                const auto at=membershipSlot(f.document,"E1",66);f.document.erase(f.document.begin()+static_cast<std::ptrdiff_t>(at));
            }
            if(mode==5u)eraseMembershipRecord(f.document,"E2");
            if(mode==6u) {
                f.document[membershipSlot(f.document,"E1",5)-1u].value="MINSERT";
                compoundAppend(f.document,"E1",{{70,"3"},{71,"2"},{44,"4.5"},{45,"-2.5"},{41,"-1.5"},{42,"2.5"},{43,"3.5"},{50,"75"}});
            }
            if(mode==7u) {const auto at=membershipSlot(f.document,"D6",20);f.document.erase(f.document.begin()+static_cast<std::ptrdiff_t>(at));}
            if(mode==8u)f.document[membershipSlot(f.document,"E1",2)].value="RES\\U+004FLVED";
            if(mode==9u)f.document[membershipSlot(f.document,"D1",10)].value="+000.000";
            if(!write(path,f.document,binary))return false;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||!compoundProfileMatches(data,2u)||!compoundProfileMatches(data,3u)
                ||!compoundProfileInventory(data,2u,spans,comments)||!compoundProfileInventory(data,3u,spans,comments)
                ||!originalChildrenWire(path,binary,f,reader.childrenAlias,originalSpans)) {
                std::cerr<<"compound positive mode="<<mode<<'\n';
                return false;
            }
            ++variants;
        }
        struct Fault {const char* id;int code;const char* value;const char* old;Reason reason;};
        const Fault cases[]{
            {"D1",90,"1",nullptr,Reason::UnknownField},{"D2",90,"1",nullptr,Reason::UnknownField},{"D3",90,"1",nullptr,Reason::UnknownField},
            {"D4",90,"1",nullptr,Reason::UnknownField},{"E1",90,"1",nullptr,Reason::UnknownField},{"E2",90,"1",nullptr,Reason::UnknownField},{"E3",90,"1",nullptr,Reason::UnknownField},
            {"D1",70,"0",nullptr,Reason::DuplicateField},{"D2",10,"0",nullptr,Reason::DuplicateField},{"D4",8,"0",nullptr,Reason::DuplicateField},
            {"E1",2,"RESOLVED",nullptr,Reason::DuplicateField},{"E2",2,"TAG",nullptr,Reason::DuplicateField},
            {"D1",100,"AcDb3dPolyline","AcDb2dPolyline",Reason::Subclass},{"D2",100,"AcDb3dPolylineVertex","AcDb2dVertex",Reason::Subclass},
            {"E1",100,"VendorReference","AcDbBlockReference",Reason::Subclass},{"D1",10,"0.25","0",Reason::InvalidValue},
            {"D1",75,"7",nullptr,Reason::InvalidValue},{"E1",70,"0",nullptr,Reason::InvalidValue},{"E2",3,"host prompt",nullptr,Reason::UnknownField},
            {"E2",100,"AcDbXrecord","AcDbAttribute",Reason::UnmodeledSecondary},{"E2",280,"0",nullptr,Reason::AmbiguousVersion},
            {"E2",71,"4",nullptr,Reason::UnknownField},{"E2",71,"2",nullptr,Reason::InvalidValue}
        };
        for(const auto& fault:cases) {
            auto f=compoundFixture("AC1032",0u);
            if(fault.old) {
                auto at=membershipSlot(f.document,fault.id,5);bool found=false;
                for(;f.document[at].code!=0;++at)if(f.document[at].code==fault.code&&f.document[at].value==fault.old){f.document[at].value=fault.value;found=true;break;}
                if(!found)return false;
            }else compoundAppend(f.document,fault.id,{{fault.code,fault.value}});
            if(!write(path,f.document,binary))return false;dx_data data;
            const auto dispatch=fault.id[0]=='D'?2u:3u,unrelated=dispatch==2u?3u:2u;
            if(!check(reader.fileImport(path.string(),&data,false)&&A::compoundMatched(data,dispatch)&&!A::compoundFailed(data)
                &&!A::compoundSourceMatched(data,dispatch)&&A::compoundSourceMatched(data,unrelated),"physical field gaps refuse source completeness, not compatible modeled import")) {
                std::cerr<<"compound wire fault="<<fault.id<<" code="<<fault.code<<" value="<<fault.value<<'\n';return false;
            }
            bool reason=false;for(const auto& p:A::compoundBody(data,dispatch)->profiles)if(p.reason==fault.reason)reason=true;
            if(!check(reason,"physical refusal has the exact expected reason"))return false;
            auto fixed=compoundFixture("AC1032",0u);if(!write(path,fixed.document,binary))return false;
            dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::compoundSourceMatched(retry,dispatch))return false;++faults;
        }
        auto good=compoundFixture("AC1032",0u);if(!write(path,good.document,binary))return false;
        dx_data data;if(!reader.fileImport(path.string(),&data,false)||!A::compoundSourceMatched(data,2u))return false;
        const auto old=A::mutableOriginBlock(data).compoundBodies.at(2u);
        dx_data newer;if(!reader.fileImport(path.string(),&newer,false)||!A::compoundSourceMatched(newer,2u))return false;
        const auto current=A::mutableOriginBlock(newer).compoundBodies.at(2u);A::mutableOriginBlock(newer).compoundBodies[2u]=old;
        if(A::compoundSourceMatched(newer,2u))return false;A::mutableOriginBlock(newer).compoundBodies[2u]=current;
        if(!A::compoundSourceMatched(newer,2u))return false;++contexts;
        dx_data copied;A::copyMembershipContext(copied,newer);if(A::compoundSourceMatched(copied,2u))return false;++contexts;
        const auto& physical=old->membership->children->records[old->membership->ranges[2u].first];
        DRW_RawDxfObject raw;raw.name=physical.name;raw.handle=0xD1u;raw.parentHandle=0xB0u;raw.m_version=DRW::AC1032;raw.groups=physical.groups;
        for(unsigned kind=0u;kind<5u;++kind) {
            auto corrupt=raw;
            const auto i=std::find_if(corrupt.groups.begin(),corrupt.groups.end(),[](const DRW_Variant& v){return v.code()==10;});
            if(kind==0u)*i=DRW_Variant(10,1);
            if(kind==1u)*i=DRW_Variant(10,UTF8STRING("0"));
            if(kind==2u)*i=DRW_Variant(10,DRW_Coord{0,0,0});
            if(kind==3u)*i=DRW_Variant(10,(std::numeric_limits<double>::quiet_NaN)());
            if(kind==4u)corrupt.groups.push_back(DRW_Variant(160,1));
            if(dx_compound_source::audit(corrupt).reason!=Reason::InvalidValue)return false;++scalarFaults;
        }
        for(unsigned kind=0u;kind<5u;++kind) {
            auto p=old->profiles.front();auto broken=physical;
            if(kind==0u)broken.spans.pop_back();
            if(kind==1u)broken.spans.front().ignoredComment=true;
            if(kind==2u)p.spans.pop_back();
            if(kind==3u)p.spans.push_back(p.spans.back());
            if(kind==4u)p.spans.front().code=-1;
            if(dx_compound_source::restorePhysicalIndices(p,broken))return false;++spanFaults;
        }
    }
    std::cout<<"Compound physical profiles="<<profiles<<"; complete bodies="<<complete<<"; revision refusals="<<revisions
        <<"; independent original scalar spans="<<originalSpans<<"; complete profile scalar spans="<<spans<<"; ignored comments="<<comments
        <<"; positive variants="<<variants<<"; wire faults/retries="<<faults<<"; stale/copied contexts="<<contexts<<"; forged scalar types="<<scalarFaults
        <<"; corrupt physical spans="<<spanFaults<<'\n';
    return profiles==84u&&complete==150u&&revisions==18u&&variants==20u&&faults==46u&&contexts==4u&&scalarFaults==10u&&spanFaults==10u
        &&originalSpans==23592u&&spans==12216u&&comments==416u;
}
