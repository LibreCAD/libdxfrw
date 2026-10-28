// SPDX-License-Identifier: GPL-2.0-or-later
// Selected AC1032 embedded ATTRIB/ATTDEF originals with equal scoped host
// alignments. Runtime-authored sources only; no external drawing fixtures.
AttdefLedger equalAlignmentLedger(bool definition,bool negativeZero,bool omitZ,bool embedded=true,bool lock=false) {
    using namespace dx_attdef_source;
    auto ledger=attdefLedger(embedded?3u:0u,false);
    const auto find=[&](Field field,int code) {
        for(std::size_t i=0u;i<ledger.expected.size();++i)
            if(ledger.expected[i].field==field&&ledger.expected[i].code==code)return i;
        throw std::runtime_error("equal-alignment ledger slot");
    };
    const auto erase=[&](std::size_t i) {
        ledger.tags.erase(ledger.tags.begin()+static_cast<std::ptrdiff_t>(i));
        ledger.expected.erase(ledger.expected.begin()+static_cast<std::ptrdiff_t>(i));
    };
    // Unlike the separate classic dual-280 qualification, this selected
    // original has no 280 at all. Change only before writing the source.
    erase(find(Field::Attribute,280));
    ledger.tags[find(Field::Text,72)].value="1"; // meaningful host alignment
    auto marker=ledger.expected.size();
    for(std::size_t i=0u;i<ledger.expected.size();++i)
        if(ledger.expected[i].field==Field::Subclass && ledger.tags[i].value=="AcDbAttributeDefinition") {marker=i;break;}
    if(marker==ledger.expected.size())throw std::runtime_error("equal-alignment marker");
    const Tags points{{11,negativeZero?"-0.0":"5"},{21,"6"},{31,"7"}};
    const auto insert=[&](std::size_t at,const Tags& values,Field field) {
        for(std::size_t i=0u;i<values.size();++i) {
            ledger.tags.insert(ledger.tags.begin()+static_cast<std::ptrdiff_t>(at+i),values[i]);
            ledger.expected.insert(ledger.expected.begin()+static_cast<std::ptrdiff_t>(at+i),
                Span{0u,values[i].code,Scope::Host,field,Role::None,0u});
        }
    };
    insert(marker,points,Field::Text);
    if(!embedded) {
        const auto after=find(Field::Attribute,74)+1u;insert(after,points,Field::Attribute);
    } else {
        ledger.tags[find(Field::Attribute,11)].value=points[0].value;
        ledger.tags[find(Field::Attribute,21)].value=points[1].value;
        ledger.tags[find(Field::Attribute,31)].value=points[2].value;
    }
    if(omitZ) {erase(find(Field::Text,31));erase(find(Field::Attribute,31));}
    if(lock)insert(find(Field::Attribute,71),Tags{{280,"1"}},Field::Attribute);
    if(!definition) {
        ledger.tags[0].value="E2";ledger.expected[0].handle=0xE2u;
        ledger.tags[1].value="E1";ledger.expected[1].handle=0xE1u;
        for(auto& tag:ledger.tags)if(tag.code==100&&tag.value=="AcDbAttributeDefinition")tag.value="AcDbAttribute";
        erase(find(Field::Attribute,3));
        if(embedded)ledger.tags[find(Field::Attribute,71)].value="2";
    }
    for(std::size_t i=0u;i<ledger.expected.size();++i)ledger.expected[i].groupIndex=i;
    return ledger;
}

bool equalAlignmentSpans(const std::vector<DRW_Variant>& groups,
        const dx_attdef_source::Audit& audit,const AttdefLedger& ledger) {
    using namespace dx_attdef_source;
    if(!audit.complete()||audit.form!=Form::Embedded101||audit.spans.size()!=groups.size()
        ||audit.spans.size()!=ledger.expected.size())return false;
    unsigned text=0u,attribute=0u;
    for(std::size_t i=0u;i<groups.size();++i) {
        const auto& actual=audit.spans[i];const auto& expected=ledger.expected[i];
        if(actual.groupIndex!=i||actual.code!=groups[i].code()||actual.code!=expected.code
            ||actual.scope!=expected.scope||actual.field!=expected.field||actual.role!=expected.role
            ||actual.handle!=expected.handle)return false;
        if(actual.code==11||actual.code==21||actual.code==31) {
            if(actual.field==Field::Text)++text;
            if(actual.field==Field::Attribute)++attribute;
        }
    }
    return (text==2u||text==3u)&&attribute==text;
}

bool equalAlignmentWriter(const std::filesystem::path& path,bool binary) {
    struct Output final:dx_iface {
        dx_data data;DRW_Insert parent;DRW_Attdef definition;
        Output() {
            cData=&data;currentBlock=data.mBlock;parent.handle=0xBEEFu;parent.name="RESOLVED";
            parent.basePoint={1,2,3};definition.handle=0xBABEu;definition.tag="EQUAL_TAG";
            definition.prompt="equal point";definition.text="definition";definition.height=1.;
            definition.alignH=DRW_Text::HCenter;definition.secPoint={5,6,7};definition.attVersion=1;
            definition.m_attributeType=4;definition.mtext=std::make_unique<DRW_MText>();
            definition.mtext->basePoint={10,20,30};definition.mtext->height=1.;definition.mtext->text="definition mtext";
            definition.mtext->alignH=static_cast<DRW_Text::HAlign>(1);
            auto child=std::make_shared<DRW_Attrib>();child->handle=0xCAFEu;child->parentHandle=parent.handle;
            child->tag="EQUAL_TAG";child->text="attribute";child->height=1.;child->alignH=DRW_Text::HCenter;
            child->secPoint={15,16,17};child->attVersion=1;child->m_attributeType=2;
            child->mtext=std::make_unique<DRW_MText>();child->mtext->basePoint={40,50,60};
            child->mtext->height=1.;child->mtext->text="attribute mtext";
            child->mtext->alignH=static_cast<DRW_Text::HAlign>(1);parent.attlist.push_back(child);
            parent.seqEndEntity=std::make_shared<DRW_SeqEnd>();parent.seqEndEntity->handle=0xCAFFu;
            parent.seqEndEntity->parentHandle=parent.handle;
        }
        void writeEntities()override{(void)dxfW->writeAttdef(&definition,DRW::DxfModelSpaceBlockRecordHandle);(void)dxfW->writeInsert(&parent);}
    } output;
    dxfRW writer(path.string().c_str());output.dxfW=&writer;if(!writer.write(&output,DRW::AC1032,binary))return false;
    const auto wire=attdefWireTags(path,binary);std::string record,scope;std::array<std::array<double,3>,3> points{};
    std::array<unsigned,3> masks{};unsigned checked=0u,locks=0u;std::uint32_t parent=0u,definition=0u,child=0u,footer=0u;
    const auto finish=[&]() {
        if(record!="ATTDEF"&&record!="ATTRIB")return true;
        const std::array<double,3> host=record=="ATTDEF"?std::array<double,3>{{5,6,7}}:std::array<double,3>{{15,16,17}};
        const std::array<double,3> nested=record=="ATTDEF"?std::array<double,3>{{10,20,30}}:std::array<double,3>{{40,50,60}};
        if(masks!=std::array<unsigned,3>{{7u,7u,7u}}||points[0]!=host||points[1]!=host||points[2]!=nested)return false;
        ++checked;return true;
    };
    for(const auto& field:wire) {
        if(field.code==0) {if(!finish())return false;record=field.text;scope.clear();points={};masks={};continue;}
        if(field.code==5&&(record=="ATTDEF"||record=="INSERT"||record=="ATTRIB"||record=="SEQEND")) {
            const auto id=static_cast<std::uint32_t>(std::stoul(field.text,nullptr,16));
            if(record=="ATTDEF")definition=id;else if(record=="INSERT")parent=id;else if(record=="ATTRIB")child=id;else footer=id;
        }
        if(field.code==330&&(record=="ATTDEF"||record=="INSERT"||record=="ATTRIB"||record=="SEQEND")) {
            const auto owner=static_cast<std::uint32_t>(std::stoul(field.text,nullptr,16));
            if(owner!=(record=="ATTDEF"||record=="INSERT"?DRW::DxfModelSpaceBlockRecordHandle:parent))return false;
        }
        if(record!="ATTDEF"&&record!="ATTRIB")continue;
        if(field.code==100||field.code==101)scope=field.text;
        if(field.code==280){if(field.integer!=0)return false;++locks;}
        int index=-1,component=-1;
        if(scope=="AcDbText"&&(field.code==11||field.code==21||field.code==31))index=0;
        if((scope=="AcDbAttribute"||scope=="AcDbAttributeDefinition")&&(field.code==11||field.code==21||field.code==31))index=1;
        if(scope=="Embedded Object"&&(field.code==10||field.code==20||field.code==30))index=2;
        if(index>=0) {
            component=(field.code==11||field.code==10)?0:(field.code==21||field.code==20)?1:2;
            if(field.kind!=AttdefWireTag::Real||(masks[index]&(1u<<component)))return false;
            masks[index]|=1u<<component;points[index][component]=field.real;
        }
    }
    if(!finish()||checked!=2u||locks!=2u||!parent||!definition||!child||!footer)return false;
    DRW_RawDxfObject selected;unsigned audited=0u;
    const auto auditSelected=[&]() {
        if(selected.name!="ATTDEF"&&selected.name!="ATTRIB")return true;
        const auto profile=dx_attdef_source::audit(selected,selected.name=="ATTDEF");
        if(!profile.complete()||profile.spans.size()!=selected.groups.size())return false;
        ++audited;return true;
    };
    for(const auto& field:wire) {
        if(field.code==0) {
            if(!auditSelected())return false;selected=DRW_RawDxfObject{};
            selected.name=field.text;selected.m_version=DRW::AC1032;
            if(field.text=="ATTDEF") {selected.handle=definition;selected.parentHandle=DRW::DxfModelSpaceBlockRecordHandle;}
            if(field.text=="ATTRIB") {selected.handle=child;selected.parentHandle=parent;}
            continue;
        }
        if(selected.name!="ATTDEF"&&selected.name!="ATTRIB")continue;
        if(field.kind==AttdefWireTag::Integer) {
            if(field.code==160)selected.groups.emplace_back(field.code,field.integer);
            else selected.groups.emplace_back(field.code,static_cast<std::int32_t>(field.integer));
        } else if(field.kind==AttdefWireTag::Real)selected.groups.emplace_back(field.code,field.real);
        else selected.groups.emplace_back(field.code,field.text);
    }
    if(!auditSelected()||audited!=2u)return false;
    dx_iface reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!data.mBlock)return false;
    unsigned readDefinitions=0u,readChildren=0u;
    for(const auto* entity:data.mBlock->ent) {
        if(const auto* d=dynamic_cast<const DRW_Attdef*>(entity)) {
            if(!d->mtext||d->handle!=definition||d->secPoint.x!=5||d->secPoint.y!=6||d->secPoint.z!=7
                ||d->mtext->basePoint.x!=10||d->mtext->basePoint.y!=20||d->mtext->basePoint.z!=30)return false;
            ++readDefinitions;
        }
        if(const auto* i=dynamic_cast<const DRW_Insert*>(entity)) {
            if(i->handle!=parent||i->attlist.size()!=1u||!i->attlist[0]||!i->attlist[0]->mtext
                ||i->attlist[0]->handle!=child||i->attlist[0]->secPoint.x!=15||i->attlist[0]->secPoint.y!=16
                ||i->attlist[0]->secPoint.z!=17||i->attlist[0]->mtext->basePoint.x!=40
                ||i->attlist[0]->mtext->basePoint.y!=50||i->attlist[0]->mtext->basePoint.z!=60
                ||!i->seqEndEntity||i->seqEndEntity->handle!=footer)return false;
            ++readChildren;
        }
    }
    return readDefinitions==1u&&readChildren==1u;
}


// Compatible output from the locally authored, lock=true input model. This
// checks transport and source profile, not independent native semantics.
bool equalAlignmentLockRoundTrip(const std::filesystem::path& path,bool binary,
        const DRW_Attdef* definition,const DRW_Insert* parent) {
    struct Output final:dx_iface {
        dx_data empty;DRW_Attdef definition;DRW_Insert parent;bool hasDefinition,good=true;
        Output(const DRW_Attdef* d,const DRW_Insert* p):hasDefinition(d!=nullptr) {
            cData=&empty;currentBlock=empty.mBlock;
            if(d)definition=*d;else parent=*p;
        }
        void writeAppId()override {
            DRW_AppId app;app.name="APP";good=dxfW->writeAppId(&app)&&good;
        }
        void writeEntities()override {
            good=(hasDefinition?dxfW->writeAttdef(&definition,DRW::DxfModelSpaceBlockRecordHandle):dxfW->writeInsert(&parent))&&good;
        }
    } output(definition,parent);
    const auto original=definition?snapshot(*definition):std::string{};
    const DRW_Attrib* attr=definition?static_cast<const DRW_Attrib*>(definition):parent->attlist.at(0).get();
    if(!attr||!attr->mtext||!attr->lockPosition) {std::cerr<<"lock roundtrip input model missing\n";return false;}
    const auto host=attr->secPoint,nested=attr->mtext->basePoint;const auto tag=attr->tag;
    dxfRW writer(path.string().c_str());output.dxfW=&writer;
    const bool wrote=writer.write(&output,DRW::AC1032,binary);
    if(!wrote||!output.good||(definition&&snapshot(*definition)!=original)) {
        const auto diagnostic=writer.getLastDiagnostic();
        std::cerr<<"lock roundtrip write="<<wrote<<" entity="<<output.good<<" code="<<diagnostic.code
            <<" detail="<<diagnostic.message<<" caller changed="<<(definition&&snapshot(*definition)!=original)<<'\n';return false;
    }
    if(!definition&&(!parent->attlist.at(0)->lockPosition||parent->attlist.at(0)->tag!=tag
        ||parent->attlist.at(0)->secPoint.x!=host.x||parent->attlist.at(0)->mtext->basePoint.x!=nested.x))return false;
    const auto wire=attdefWireTags(path,binary);DRW_RawDxfObject raw;unsigned locks=0u;
    const auto selected=definition?"ATTDEF":"ATTRIB";bool active=false,done=false;
    for(const auto& field:wire) {
        if(field.code==0) {if(active){done=true;break;}active=field.text==selected;continue;}
        if(!active)continue;
        if(field.code==5)raw.handle=static_cast<std::uint32_t>(std::stoul(field.text,nullptr,16));
        if(field.code==330&&!raw.parentHandle)raw.parentHandle=static_cast<std::uint32_t>(std::stoul(field.text,nullptr,16));
        if(field.code==280){if(field.integer!=1)return false;++locks;}
        if(field.kind==AttdefWireTag::Integer) {
            if(field.code==160)raw.groups.emplace_back(field.code,field.integer);
            else raw.groups.emplace_back(field.code,static_cast<std::int32_t>(field.integer));
        } else if(field.kind==AttdefWireTag::Real)raw.groups.emplace_back(field.code,field.real);
        else raw.groups.emplace_back(field.code,field.text);
    }
    raw.name=selected;raw.m_version=DRW::AC1032;
    const auto audited=dx_attdef_source::audit(raw,definition!=nullptr);
    if(!done||locks!=1u||!audited.complete()||audited.spans.size()!=raw.groups.size()) {
        std::cerr<<"lock roundtrip audit done="<<done<<" locks="<<locks<<" reason="<<static_cast<int>(audited.reason)
            <<" group="<<audited.rejectedGroup<<" spans="<<audited.spans.size()<<" groups="<<raw.groups.size()<<'\n';return false;
    }
    dx_iface reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!data.mBlock)return false;
    for(const auto* entity:data.mBlock->ent) {
        const DRW_Attrib* read=nullptr;
        if(definition)read=dynamic_cast<const DRW_Attdef*>(entity);
        else if(const auto* insert=dynamic_cast<const DRW_Insert*>(entity))
            if(insert->attlist.size()==1u)read=insert->attlist[0].get();
        if(read&&read->tag==tag&&read->lockPosition&&read->mtext
            &&read->secPoint.x==host.x&&read->secPoint.y==host.y&&read->secPoint.z==host.z
            &&read->mtext->basePoint.x==nested.x&&read->mtext->basePoint.y==nested.y
            &&read->mtext->basePoint.z==nested.z)return true;
    }
    std::cerr<<"lock roundtrip ordinary readback failed\n";return false;
}

bool equalAlignmentRefusals(const DRW_RawDxfObject& original,const AttdefLedger& ledger,
        bool definition,unsigned& refusals) {
    using namespace dx_attdef_source;
    const auto position=[&](Field field,int code) {
        for(std::size_t i=0u;i<ledger.expected.size();++i)
            if(ledger.expected[i].field==field&&ledger.expected[i].code==code)return i;
        throw std::runtime_error("equal profile position");
    };
    for(unsigned fault=0u;fault<13u;++fault) {
        auto raw=original;
        const auto erase=[&](std::size_t i){raw.groups.erase(raw.groups.begin()+static_cast<std::ptrdiff_t>(i));};
        Reason reason=Reason::IncompletePoint;std::size_t at=raw.groups.size();
        if(fault==0u) {at=position(Field::Attribute,11);raw.groups[at]=DRW_Variant(11,5.25);reason=Reason::DuplicateField;}
        if(fault==1u)erase(position(Field::Attribute,21));
        if(fault==2u)erase(position(Field::Attribute,31));
        if(fault==3u)erase(position(Field::Text,31));
        if(fault==4u) {at=position(Field::Attribute,11);raw.groups[at]=DRW_Variant(11,std::numeric_limits<double>::quiet_NaN());reason=Reason::InvalidValue;}
        if(fault==5u) {at=position(Field::Text,11);raw.groups[at]=DRW_Variant(11,std::numeric_limits<double>::infinity());reason=Reason::InvalidValue;}
        if(fault==6u) {raw.groups[position(Field::Text,11)]=DRW_Variant(11,0.0);at=position(Field::Attribute,11);raw.groups[at]=DRW_Variant(11,-0.0);reason=Reason::DuplicateField;}
        if(fault==7u) {raw.m_version=DRW::AC1027;at=position(Field::Attribute,71);reason=Reason::Revision;}
        if(fault==8u) {
            auto marker=std::find_if(raw.groups.begin(),raw.groups.end(),[&](const DRW_Variant& v){return v.code()==100&&v.type()==DRW_Variant::STRING&&v.content.s&&*v.content.s==(definition?"AcDbAttributeDefinition":"AcDbAttribute");});
            if(marker==raw.groups.end())return false;raw.groups.insert(marker+1,DRW_Variant(280,0));
            auto embedded=std::find_if(raw.groups.begin(),raw.groups.end(),[](const DRW_Variant& v){return v.code()==101&&v.type()==DRW_Variant::STRING&&v.content.s&&*v.content.s=="Embedded Object";});
            if(embedded==raw.groups.end())return false;raw.groups.insert(embedded,DRW_Variant(280,1));
            embedded=std::find_if(raw.groups.begin(),raw.groups.end(),[](const DRW_Variant& v){return v.code()==101&&v.type()==DRW_Variant::STRING&&v.content.s&&*v.content.s=="Embedded Object";});
            at=static_cast<std::size_t>(embedded-raw.groups.begin());reason=Reason::AmbiguousVersion;
        }
        if(fault==9u) {at=position(Field::EmbeddedMarker,101);raw.groups[at]=DRW_Variant(100,"Embedded Object");reason=Reason::AmbiguousVersion;}
        if(fault==10u) {
            auto marker=std::find_if(raw.groups.begin(),raw.groups.end(),[&](const DRW_Variant& v){return v.code()==100&&v.type()==DRW_Variant::STRING&&v.content.s&&*v.content.s==(definition?"AcDbAttributeDefinition":"AcDbAttribute");});
            if(marker==raw.groups.end())return false;raw.groups.insert(marker+1,DRW_Variant(280,0));
            at=raw.groups.size();reason=Reason::AmbiguousVersion;
        }
        if(fault==11u) {
            // The legacy embedded-first spelling has no attribute type or
            // reserved 72 before its subclass; reach the duplicate point guard.
            erase(position(Field::ReservedDefault,72));erase(position(Field::Attribute,71));
            auto first=std::find_if(raw.groups.begin(),raw.groups.end(),[&](const DRW_Variant& v){return v.code()==100&&v.type()==DRW_Variant::STRING&&v.content.s&&*v.content.s==(definition?"AcDbAttributeDefinition":"AcDbAttribute");});
            auto nested=std::find_if(raw.groups.begin(),raw.groups.end(),[](const DRW_Variant& v){return v.code()==101&&v.type()==DRW_Variant::STRING&&v.content.s&&*v.content.s=="Embedded Object";});
            auto after=std::find_if(raw.groups.begin(),raw.groups.end(),[](const DRW_Variant& v){return v.code()==102&&v.type()==DRW_Variant::STRING&&v.content.s&&*v.content.s=="{AFTER_ATTRIBUTE";});
            if(first==raw.groups.end()||nested==raw.groups.end()||after==raw.groups.end()||first>=nested||nested>=after)return false;
            std::rotate(first,nested,after);
            const auto attribute=std::find_if(raw.groups.begin(),raw.groups.end(),[&](const DRW_Variant& v){return v.code()==100&&v.type()==DRW_Variant::STRING&&v.content.s&&*v.content.s==(definition?"AcDbAttributeDefinition":"AcDbAttribute");});
            const auto point=std::find_if(attribute,raw.groups.end(),[](const DRW_Variant& v){return v.code()==11;});
            if(point==raw.groups.end())return false;at=static_cast<std::size_t>(point-raw.groups.begin());reason=Reason::DuplicateField;
        }
        if(fault==12u) {
            const auto after=position(Field::Attribute,74);erase(after);
            const auto before=position(Field::Attribute,71)-1u;
            raw.groups.insert(raw.groups.begin()+static_cast<std::ptrdiff_t>(before),DRW_Variant(280,1));
            raw.groups.insert(raw.groups.begin()+static_cast<std::ptrdiff_t>(before+1u),DRW_Variant(74,2));
            at=raw.groups.size();reason=Reason::AmbiguousVersion;
        }
        if(fault>=1u&&fault<=3u)at=raw.groups.size();
        if(fault==9u)at=raw.groups.size(); // Legacy spelling is outside the witnessed profile.
        const auto denied=dx_attdef_source::audit(raw,definition);
        if(!denied.complete()&&denied.spans.empty()&&denied.reason==reason&&denied.rejectedGroup==at) {
            ++refusals;continue;
        }
        std::cerr<<"equal-alignment refusal fault="<<fault<<" definition="<<definition
            <<" expected reason="<<static_cast<int>(reason)<<" group="<<at
            <<" actual reason="<<static_cast<int>(denied.reason)<<" group="<<denied.rejectedGroup
            <<" retained spans="<<denied.spans.size()<<'\n';
        return false;
    }
    for(int lock:{0,1}) {
        auto raw=original;const auto before=position(Field::Attribute,71);
        raw.groups.insert(raw.groups.begin()+static_cast<std::ptrdiff_t>(before),DRW_Variant(280,lock));
        const auto accepted=dx_attdef_source::audit(raw,definition);
        if(!check(accepted.complete()&&accepted.spans.size()==raw.groups.size()
            &&accepted.spans[before].field==Field::Attribute&&accepted.spans[before].code==280,
            "equal-alignment canonical single-lock source qualification"))return false;
    }
    return true;
}

bool equalAlignmentTests(const std::filesystem::path& path) {
    using A=DxIfaceTableSourceTestAccess;using namespace dx_attdef_source;
    unsigned originals=0u,compound=0u,refusals=0u,writer=0u;std::size_t spans=0u,compoundSpans=0u,comments=0u,originalSpans=0u;
    for(bool binary:{false,true})for(bool block:{false,true})for(unsigned mode=0u;mode<4u;++mode) {
        const auto ledger=equalAlignmentLedger(true,mode==1u,mode==2u,true,mode==3u);
        const auto tags=document("AC1032",ledger.tags,block);if(!write(path,tags,binary))return false;
        CaptureProbe reader;dx_data data;
        if(!check(reader.fileImport(path.string(),&data,false)&&!A::failed(data),"equal-alignment original ATTDEF import"))return false;
        const auto* row=A::resource(data);
        if(!check(row&&row->raw&&row->attdefModel&&row->attdefPairMatched&&row->attdefFieldsMatched
            &&!row->attdefFieldMismatch&&!row->rawTypedMatched&&row->attdefProfile.complete()
            &&row->attdefModel->mtext&&row->attdefModel->m_attributeType==4
            &&row->attdefModel->lockPosition==(mode==3u)
            &&equalAlignmentSpans(row->raw->groups,row->attdefProfile,ledger)
            &&attdefOriginalSpans(path,binary,tags,ledger,row->attdefProfile,spans)
            &&replayMatches(ledger.tags,"AC1032",snapshot(*row->attdefModel),false),"equal-alignment ATTDEF source spans/replay"))return false;
        if(!check(row->attdefModel->secPoint.y==6&&row->attdefModel->mtext->basePoint.x==10
            &&row->attdefModel->mtext->basePoint.y==20&&row->attdefModel->mtext->basePoint.z==30
            &&(mode!=1u||std::signbit(row->attdefModel->secPoint.x)),"host and nested typed alignments remain separate"))return false;
        if(mode==0u) {
            if(!equalAlignmentRefusals(*row->raw,ledger,true,refusals))return false;
            auto classic=equalAlignmentLedger(true,false,false,false);const auto classicTags=document("AC1032",classic.tags,block);
            if(!write(path,classicTags,binary))return false;CaptureProbe classicReader;dx_data classicData;
            if(!classicReader.fileImport(path.string(),&classicData,false)||!A::resource(classicData)||!A::resource(classicData)->raw)return false;
            const auto denied=dx_attdef_source::audit(*A::resource(classicData)->raw);
            if(!check(!denied.complete()&&denied.spans.empty()&&denied.reason==Reason::AmbiguousVersion
                &&denied.rejectedGroup==A::resource(classicData)->raw->groups.size(),"nonembedded duplicate alignment remains refused"))return false;
            ++refusals;
        }
        if(mode==3u)for(bool targetBinary:{false,true})
            if(!check(equalAlignmentLockRoundTrip(path,targetBinary,row->attdefModel.get(),nullptr),"lock ATTDEF compatible source round trip"))return false;
        ++originals;
    }
    for(bool binary:{false,true})for(unsigned mode=0u;mode<4u;++mode) {
        const auto ledger=equalAlignmentLedger(false,mode==1u,mode==2u,true,mode==3u);
        auto f=compoundFixture("AC1032",4u);replaceCompoundRecord(f.document,"E2","ATTRIB",ledger.tags);
        if(!write(path,f.document,binary))return false;CompoundBodyProbe reader;dx_data data;
        if(!check(reader.fileImport(path.string(),&data,false)&&A::membershipMatched(data)
            &&compoundProfileMatches(data,3u)&&compoundProfileInventory(data,3u,compoundSpans,comments)
            &&originalChildrenWire(path,binary,f,reader.childrenAlias,originalSpans),"equal-alignment original ATTRIB compound"))return false;
        const auto* body=A::compoundBody(data,3u);if(!body||!body->membership||body->profiles.size()!=3u)return false;
        const auto range=body->membership->ranges[3u];if(range.next!=range.first+3u)return false;
        const auto& physical=body->membership->children->records[range.first+1u];
        const auto* parent=dynamic_cast<const DRW_Insert*>(body->model.get());
        if(!check(physical.name=="ATTRIB"&&equalAlignmentSpans(physical.groups,body->profiles[1],ledger)
            &&parent&&parent->attlist.size()==1u&&parent->attlist[0]&&parent->attlist[0]->mtext
            &&parent->attlist[0]->m_attributeType==2&&parent->attlist[0]->lockPosition==(mode==3u)
            &&parent->attlist[0]->parentHandle==parent->handle
            &&(mode!=1u||std::signbit(parent->attlist[0]->secPoint.x))
            &&parent->attlist[0]->mtext->basePoint.x==10&&parent->attlist[0]->mtext->basePoint.y==20
            &&parent->attlist[0]->mtext->basePoint.z==30&&parent->seqEndEntity
            &&parent->seqEndEntity->parentHandle==parent->handle,"ATTRIB physical roles and exact compound owner"))return false;
        if(mode==0u) {
            DRW_RawDxfObject raw;raw.name=physical.name;raw.handle=0xE2u;raw.parentHandle=0xE1u;
            raw.m_version=DRW::AC1032;raw.groups=physical.groups;
            if(!equalAlignmentRefusals(raw,ledger,false,refusals))return false;
        }
        if(mode==3u)for(bool targetBinary:{false,true})
            if(!check(equalAlignmentLockRoundTrip(path,targetBinary,nullptr,parent),"lock ATTRIB compatible source round trip"))return false;
        ++compound;
    }
    for(bool binary:{false,true}){if(!check(equalAlignmentWriter(path,binary),"compatible embedded alignment writer/readback"))return false;++writer;}
    std::cout<<"Embedded equal-alignment ATTDEF originals="<<originals<<"; ATTRIB compounds="<<compound
        <<"; direct refusal controls="<<refusals<<"; source spans="<<spans<<"; compound spans="<<compoundSpans
        <<"; writer forms="<<writer<<'\n';
    return originals==16u&&compound==8u&&refusals==82u&&writer==2u;
}
