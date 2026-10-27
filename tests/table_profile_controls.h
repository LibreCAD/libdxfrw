// SPDX-License-Identifier: GPL-2.0-or-later
// Independent physical TABLE roles; all drawing recipes are runtime-authored.
Tags tableProfileDocument(const std::string& version,bool blocks,bool paper,bool rich,unsigned form=1u) {
    auto tags=rich?tableBodyRichDocument(version,blocks):tableBlockSourceDocument(version,blocks,form);
    if(rich) {
        // The modeled-replay fixture intentionally includes a text-cell
        // attribute and duplicate merge flag. Neither is independent semantic
        // evidence: use the documented block-cell attribute shape here.
        const auto at=blockField(tags,"501",179);tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at),tags.begin()+static_cast<std::ptrdiff_t>(at+3u));
        const auto merged=blockField(tags,"501",173);tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(merged));
        tags[blockField(tags,"501",304)].value="ACVALUE_END";
    }
    const auto head=blockField(tags,"501",280);
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(head),{{342,"C1"},{343,"F00"}});
    if(paper) {
        const auto first=slot(tags,5,"501")-1u;auto last=slot(tags,5,rich||form?"513":"501")+1u;
        while(last<tags.size()&&tags[last].code!=0)++last;
        Tags body(tags.begin()+static_cast<std::ptrdiff_t>(first),tags.begin()+static_cast<std::ptrdiff_t>(last));
        tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(first),tags.begin()+static_cast<std::ptrdiff_t>(last));
        for(std::size_t i=0u;i<body.size();++i) {
            if(body[i].code==330&&(body[i].value=="B3"||body[i].value=="B0"))body[i].value=blocks?"B2":"B1";
            if(body[i].code==100&&body[i].value=="AcDbEntity") {
                body.insert(body.begin()+static_cast<std::ptrdiff_t>(i+1u),{{67,"1"},{410,blocks?"Sheet 2":"Sheet 1"}});i+=2u;
            }
        }
        const auto at=blocks?slot(tags,5,"F5")-1u:slot(tags,0,"EOF")-1u;
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),body.begin(),body.end());
    }
    return tags;
}
// A separate original-file walker checks offsets, types, values and roles.
// It does not call dxfReader, the TABLE decoder or any source-profile helper.
bool tableProfileWire(const std::vector<AttdefWireTag>& wire,const DRW_DxfPhysicalEntitySource& physical,
                      const dx_attdef_source::Audit& profile,std::size_t& count) {
    using namespace dx_attdef_source;
    if(!profile.complete()||profile.spans.size()!=physical.groups.size())return false;
    unsigned depth=0u;std::string application;int stage=0;bool table=physical.name=="ACAD_TABLE",inValue=false,firstValue=false,structured=false;
    for(std::size_t i=0u;i<physical.groups.size();++i) {
        const auto& p=physical.spans[i];const auto& span=profile.spans[i];const auto& value=physical.groups[i];
        if(!p.ordinal||p.ordinal>wire.size())return false;const auto& w=wire[p.ordinal-1u];
        if(span.groupIndex!=i||span.code!=w.code||value.code()!=w.code||p.beginOffset!=w.begin||p.endOffset!=w.end)return false;
        if(p.ignoredComment){if(span.scope!=Scope::Ignored||span.field!=Field::IgnoredComment)return false;++count;continue;}
        if(w.kind==AttdefWireTag::Integer) {if((value.type()!=DRW_Variant::INTEGER&&value.type()!=DRW_Variant::INTEGER64)
                ||(value.type()==DRW_Variant::INTEGER64?value.i64_val():value.i_val())!=w.integer)return false;}
        else if(w.kind==AttdefWireTag::Real) {const auto real=value.d_val();if(value.type()!=DRW_Variant::DOUBLE||std::memcmp(&w.real,&real,sizeof(real)))return false;}
        else if(value.type()!=DRW_Variant::STRING||!value.content.s||*value.content.s!=w.text)return false;
        Role role=Role::None;
        if(w.code==102) {
            if(w.text=="}"){if(!depth)return false;--depth;}
            else {if(!depth)application=w.text;++depth;}
            if(span.field!=Field::Application)return false;
        }else if(depth) {
            if((w.code>=320&&w.code<=369)||w.code==5||w.code==1005)role=Role::OpaqueHandle;
            if(application=="{ACAD_REACTORS"&&w.code==330)role=Role::Reactor;
            if(application=="{ACAD_XDICTIONARY"&&w.code==360)role=Role::ExtensionDictionary;
            if(span.field!=Field::Application)return false;
        }else {
            if(table&&stage==3&&w.code==301&&w.text=="CELL_VALUE"){inValue=true;firstValue=true;structured=false;}
            else if(firstValue){structured=w.code==93;firstValue=false;}
            if(w.code==100){if(w.text=="AcDbEntity")stage=1;else if(w.text=="AcDbBlockReference")stage=2;else if(w.text=="AcDbTable")stage=3;}
            switch(w.code) {
            case 5:role=Role::Self;break;case 330:role=inValue&&structured?Role::TableValueObject:Role::Owner;break;case 8:role=Role::LayerName;break;case 6:role=Role::LineTypeName;break;
            case 7:role=Role::TextStyleName;break;case 410:role=Role::LayoutName;break;case 347:role=Role::Material;break;case 348:role=Role::FullVisualStyle;break;
            case 390:role=Role::PlotStyle;break;case 360:role=Role::ExtensionDictionary;break;case 1001:role=Role::AppIdName;break;
            case 1003:role=Role::XdataLayerName;break;case 1005:role=Role::XdataObjectId;break;
            case 2:if(table&&stage==2)role=Role::BlockName;break;
            case 342:role=Role::TableStyle;break;case 343:role=Role::TableGeometryBlock;break;case 340:role=Role::TableCellBlock;break;
            case 344:role=Role::TableCellField;break;case 331:role=Role::TableCellAttributeDefinition;break;
            default:break;
            }
            if(inValue&&span.field!=Field::TableValue)return false;
            if(w.code==304){inValue=false;structured=false;}
        }
        if(span.scope!=Scope::Host||span.role!=role)return false;
        if(role==Role::Self||role==Role::Owner||role==Role::OpaqueHandle||role==Role::Reactor||role==Role::ExtensionDictionary
            ||role==Role::Material||role==Role::FullVisualStyle||role==Role::PlotStyle||role==Role::XdataObjectId
            ||role==Role::TableStyle||role==Role::TableGeometryBlock||role==Role::TableCellBlock||role==Role::TableCellField||role==Role::TableCellAttributeDefinition||role==Role::TableValueObject)
            if(span.handle!=std::stoull(w.text,nullptr,16))return false;
        ++count;
    }
    return !depth&&!inValue;
}
bool tableProfileTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;using BS=dx_iface::BlockRoleState;using ES=dx_iface::EntitiesRoleState;
    struct Stage{const char* name="profiles";bool done=false;~Stage(){if(!done)std::cerr<<"TABLE profile stage="<<name<<'\n';}}stage;
    const auto path=dir/"table-profiles.dxf";unsigned profiles=0u,refusals=0u,types=0u,contexts=0u,quotas=0u,allocations=0u,unreached=0u,census=0u,targets=0u,extensions=0u,proxyBoundary=0u,shapes=0u;std::size_t spans=0u,extensionSpans=0u,shapeSpans=0u;
    const auto ready=[](const dx_data& d,bool blocks) {
        const auto body=dx_iface::tableBodyReport(d);if(!body.modeledMatched||!body.profilesMatched||body.ownerTargetsResolved||body.outputEligible)return false;
        if(blocks){const auto r=dx_iface::blockRoleReport(d);return r.state==BS::RolesReady&&r.physicalCensusComplete&&r.selectedRolesMatched&&!r.unsupported
            &&!r.wholeDocumentMatched&&!r.ownerTargetsResolved&&!r.outputEligible;}
        const auto r=dx_iface::entitiesRoleReport(d);return r.state==ES::SelectedRolesReady&&r.selectedRolesMatched&&!r.unsupported
            &&!r.wholeDocumentMatched&&!r.ownerTargetsResolved&&!r.outputEligible;
    };
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true})for(bool paper:{false,true})for(bool rich:{false,true}) {
        const auto tags=tableProfileDocument(version,blocks,paper,rich);if(!writeBlockBinding(path,tags,encoding))return false;dx_iface reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false))return false;const auto report=dx_iface::tableBodyReport(data);const auto& body=*A::tableBodies(data).at(0x501u);
        const bool roles=ready(data,blocks);
        if(!report.modeledMatched||!report.profilesMatched||!roles||report.ownerTargetsResolved||report.outputEligible) {
            std::cerr<<"TABLE profile "<<version<<'/'<<encoding<<" blocks="<<blocks<<" paper="<<paper<<" rich="<<rich<<" modeled="<<report.modeledMatched
                <<" profiles="<<report.profilesMatched<<" roles="<<roles;
            for(const auto& p:body.profiles)std::cerr<<" reason="<<static_cast<int>(p.reason)<<" group="<<p.rejectedGroup;
            std::cerr<<'\n';return false;
        }
        const auto wire=attdefWireTags(path,encoding==2u);const auto& records=blocks?body.source->blockPhysical->records:body.entitiesPhysical->records;
        if(body.profiles.size()!=body.next-body.first)return false;
        for(auto i=body.first;i<body.next;++i)if(!tableProfileWire(wire,records[i],body.profiles[i-body.first],spans))return false;
        ++profiles;
    }
    for(bool binary:{false,true})for(bool blocks:{false,true}) {
        const auto good=tableProfileDocument("AC1032",blocks,false,true);
        const auto retry=[&](dx_iface& reader){dx_data next;return write(path,good,binary)&&reader.fileImport(path.string(),&next,false)&&ready(next,blocks);};
        stage.name="parent-only,empty footer to block,and raw-stub boundaries";
        for(unsigned shape=0u;shape<3u;++shape) {auto tags=tableProfileDocument("AC1032",blocks,false,false,shape==1u?2u:0u);
            if(shape==1u)tags[blockField(tags,"513",330)].value=blocks?"B3":"B0";
            if(shape==2u){const auto first=blockField(tags,"501",280),last=blockField(tags,"501",1);
                tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(first),tags.begin()+static_cast<std::ptrdiff_t>(last+1u));}
            dx_iface reader;dx_data data;if(!write(path,tags,binary)||!reader.fileImport(path.string(),&data,false)||!ready(data,blocks)
                ||dx_iface::tableBodyReport(data).semanticComplete!=(shape==2u?0u:1u))return false;
            const auto& body=*A::tableBodies(data).at(0x501u);const auto& records=blocks?body.source->blockPhysical->records:body.entitiesPhysical->records;
            const auto wire=attdefWireTags(path,binary);for(auto i=body.first;i<body.next;++i)if(!tableProfileWire(wire,records[i],body.profiles[i-body.first],shapeSpans))return false;
            ++shapes;
        }
        stage.name="reserved/opaque applications,XDATA,ignored comments and proxy chunk boundaries";
        for(unsigned shape=0u;shape<4u;++shape) {if(shape==2u&&binary)continue;auto tags=good;
            if(shape==0u){const auto at=blockField(tags,"501",8)+1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),
                {{102,"{ACAD_REACTORS"},{330,"F11"},{330,"F12"},{102,"}"},{102,"{ACAD_XDICTIONARY"},{360,"F13"},{102,"}"}});}
            else if(shape==1u)appendBlockFields(tags,"501",{{1001,"APP"},{1003,"XDATA_LAYER"},{1005,"F14"},{1010,"1"},{1020,"2"},{1030,"3"},{1004,"AAff"},{1070,"7"}});
            else if(shape==2u){const auto at=blockField(tags,"501",171);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{999,"ignored original comment"}});}
            else {const auto at=blockField(tags,"501",8)+1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{160,"128"},{310,std::string(254u,'A')},{310,"BB"}});}
            dx_iface reader;dx_data data;if(!write(path,tags,binary)||!reader.fileImport(path.string(),&data,false)||!ready(data,blocks)) {
                std::cerr<<"extensions shape="<<shape<<" blocks="<<blocks<<" binary="<<binary<<'\n';return false;}
            const auto& body=*A::tableBodies(data).at(0x501u);const auto& records=blocks?body.source->blockPhysical->records:body.entitiesPhysical->records;
            const auto wire=attdefWireTags(path,binary);
            if(shape==3u&&std::none_of(wire.begin(),wire.end(),[](const auto& t){return t.code==310&&t.text.size()==254u;}))return false;
            for(auto i=body.first;i<body.next;++i)if(!tableProfileWire(wire,records[i],body.profiles[i-body.first],extensionSpans))return false;
            ++extensions;
            if(shape==3u) {
                tags[blockField(tags,"501",160)].value="129";tags[blockField(tags,"501",310)].value=std::string(256u,'A');
                dx_data over;if(!write(path,tags,binary))return false;const auto overWire=attdefWireTags(path,binary);
                if(std::none_of(overWire.begin(),overWire.end(),[](const auto& t){return t.code==310&&t.text.size()==256u;}))return false;
                (void)reader.fileImport(path.string(),&over,false);
                if(dx_iface::tableBodyReport(over).profilesMatched||ready(over,blocks)||!retry(reader))return false;++proxyBoundary;
            }
        }
        if(!write(path,good,binary))return false;dx_iface baseline;dx_data base;if(!baseline.fileImport(path.string(),&base,false)||!ready(base,blocks))return false;
        stage.name="every parent scalar type, independent of decoder tolerance";
        const auto& source=*A::tableInputs(base).at(0x501u)->raw;
        for(std::size_t i=0u;i<source.groups.size();++i) {auto raw=source;auto& value=raw.groups[i];const int code=value.code();
            if(value.type()==DRW_Variant::STRING)value.addInt(code,1);else value.addString(code,"not-a-number");
            if(dx_table_source::audit(raw).complete())return false;++types;
        }
        for(const char* name:{"TABLECONTENT","TABLEGEOMETRY","TABLE"}) {auto raw=source;raw.name=name;if(dx_table_source::audit(raw).complete())return false;++types;}
        stage.name="unmodeled,duplicate,subclass/count/value/attribute grammar is not replay equality";
        for(unsigned fault=0u;fault<24u;++fault) {auto tags=good;
            switch(fault) {
            case 0:appendBlockFields(tags,"501",{{99,"7"}});break;
            case 1:appendBlockFields(tags,"501",{{100,"AcDbTableContent"}});break;
            case 2:tags[blockField(tags,"501",100)].value="NotAcDbEntity";break;
            case 3:tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,"501",141)));break;
            case 4:{const auto at=blockField(tags,"501",172);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),tags[at]);break;}
            case 5:tags[blockField(tags,"501",304)].value="END_VALUE";break;
            case 6:{const auto at=blockField(tags,"501",301)+1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{330,"F00"}});break;}
            case 7:{const auto at=blockField(tags,"501",301)+1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{93,"1"},{94,"0"}});break;}
            case 8:{const auto at=blockField(tags,"501",301)+1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{11,"1"},{21,"2"},{31,"3"}});break;}
            case 9:tags[blockField(tags,"501",179)].value="2";break;
            case 10:tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,"501",300)));break;
            case 11:{const auto at=blockField(tags,"501",171)+1u;tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{179,"1"},{331,"F00"},{300,"not-block-attribute"}});break;}
            case 12:appendBlockFields(tags,"501",{{7,"unsupported-table-level-style"}});break;
            case 13:appendBlockFields(tags,"501",{{274,"25"},{284,"0"}});break;
            case 14:appendBlockFields(tags,"501",{{97,"4"},{98,"1"},{4,"unsupported-table-format"}});break;
            case 15:tags[blockField(tags,"501",171)].value="92";break;
            default:{const int kinds[]={0,1,2,8,16,32,64,512};auto at=blockField(tags,"501",301)+1u;
                if(tags[at].code!=90)return false;tags[at].value=std::to_string(kinds[fault-16u]);break;}
            }
            dx_iface reader;dx_data data;if(!write(path,tags,binary)||!reader.fileImport(path.string(),&data,false)) {
                std::cerr<<"grammar import fault="<<fault<<" blocks="<<blocks<<" binary="<<binary<<'\n';return false;
            }
            const auto r=dx_iface::tableBodyReport(data);
            const auto input=A::tableInputs(data).find(0x501u);
            if(input==A::tableInputs(data).end()||dx_table_source::audit(*input->second->raw).complete()
                ||(!r.modeledMatched&&(!blocks||fault!=2u))||r.profilesMatched||ready(data,blocks)||r.outputEligible||!retry(reader)) {
                std::cerr<<"grammar fault="<<fault<<" blocks="<<blocks<<" modeled="<<r.modeledMatched<<" profile="<<r.profilesMatched<<'\n';return false;
            }
            ++refusals;
        }
        stage.name="actual original owner/space/layout targets, not geometry-block or opaque lookalikes";
        for(unsigned fault=0u;fault<8u;++fault) {auto tags=good;
            if(fault<2u)tags[blockField(tags,"501",330)].value=fault?"C1":"FFFF";
            else if(fault==2u)appendBlockFields(tags,"501",{{67,"1"}});
            else if(fault==3u)appendBlockFields(tags,"501",{{410,"Sheet 1"}});
            else if(fault==4u)appendBlockFields(tags,"511",{{67,"1"}});
            else if(fault==5u)appendBlockFields(tags,"511",{{410,"Sheet 2"}});
            else tags[blockField(tags,fault==6u?"511":"513",330)].value="FFFF";
            dx_iface reader;dx_data data;if(!write(path,tags,binary))return false;const bool read=reader.fileImport(path.string(),&data,false);
            if(!read||ready(data,blocks)||(blocks?dx_iface::blockRoleReport(data).selectedRolesMatched:dx_iface::entitiesRoleReport(data).state==ES::SelectedRolesReady)||!retry(reader)) {
                std::cerr<<"target fault="<<fault<<" blocks="<<blocks<<" binary="<<binary<<" read="<<read<<" state="
                    <<(blocks?static_cast<int>(dx_iface::blockRoleReport(data).state):static_cast<int>(dx_iface::entitiesRoleReport(data).state))<<'\n';return false;
            }
            ++targets;
        }
        for(unsigned shape=0u;shape<2u;++shape) {auto tags=good;if(!shape)appendBlockFields(tags,"501",{{410,"Model"}});
            else tags[blockField(tags,"501",343)].value="DEADBEEF";
            const bool expected=!blocks||shape!=0u;
            dx_iface reader;dx_data data;if(!write(path,tags,binary)||!reader.fileImport(path.string(),&data,false)||ready(data,blocks)!=expected||!retry(reader)) {
                std::cerr<<"target shape="<<shape<<" blocks="<<blocks<<" expected="<<expected<<'\n';return false;
            }++targets;}
        stage.name="unsupported or omitted primaries remain in the census";
        for(unsigned fault=0u;fault<3u;++fault) {auto tags=good;
            const auto at=blocks?slot(tags,5,"F7")-1u:slot(tags,0,"EOF")-1u;
            const Tags extra{{0,fault==0u?"CIRCLE":fault==1u?"TABLECONTENT":"UNSUPPORTED_PRIMARY"},{5,"701"},{330,blocks?"B3":"B0"},
                {100,"AcDbEntity"},{8,"0"},{100,"AcDbCircle"},{10,"1"},{20,"2"},{40,"3"}};
            tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),extra.begin(),extra.end());dx_iface reader;dx_data data;
            if(!write(path,tags,binary)||!reader.fileImport(path.string(),&data,false)||ready(data,blocks)
                ||!dx_iface::tableBodyReport(data).profilesMatched)return false;
            if(blocks){const auto r=dx_iface::blockRoleReport(data);if(!r.physicalCensusComplete||!r.unsupported||r.wholeDocumentMatched||r.outputEligible)return false;}
            else {const auto r=dx_iface::entitiesRoleReport(data);if(!r.unsupported||r.wholeDocumentMatched||r.outputEligible)return false;}
            if(!retry(reader))return false;++census;
        }
        stage.name="body/input/owner substitutions, missing maps and copied containers";
        if(!write(path,good,binary))return false;
        for(unsigned fault=0u;fault<5u;++fault) {dx_iface reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;
            if(fault<2u)A::substituteTableBody(data,fault!=0u);else if(fault==2u)A::removeTableBody(data);
            else if(fault==3u)A::substituteTableInput(data,false,false);else A::substituteTableContext(data,blocks,true);
            if((blocks?dx_iface::blockRoleReport(data).state!=BS::ContextMismatch:dx_iface::entitiesRoleReport(data).state!=ES::ContextMismatch)||!retry(reader))return false;++contexts;
        }
        {dx_iface reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;
            {SymbolAllocationFailure noAllocation(0);if(!ready(data,blocks))return false;}++contexts;
            data.tables.front().m_content.m_rows.clear();if(!ready(data,blocks))return false;++contexts;
            dx_data copy;A::copyBindingContext(copy,data);if(ready(copy,blocks))return false;++contexts;
        }
        {TableInputProbe reader;reader.selected=false;dx_data data;if(!reader.fileImport(path.string(),&data,false)||ready(data,blocks))return false;++contexts;}
        stage.name="exact/one-under role bytes/work and reached role-finalizer faults";
        const auto usage=blocks?std::array<std::size_t,2>{dx_iface::blockRoleReport(base).bytes,dx_iface::blockRoleReport(base).work}:
            std::array<std::size_t,2>{dx_iface::entitiesRoleReport(base).bytes,dx_iface::entitiesRoleReport(base).work};
        for(unsigned which=0u;which<2u;++which)for(bool under:{false,true}) {dx_iface reader;dx_data data;
            if(blocks)A::blockRoleLimit(data,which!=0u,usage[which]-(under?1u:0u));else A::entitiesRoleLimit(data,which!=0u,usage[which]-(under?1u:0u));
            if(!reader.fileImport(path.string(),&data,false)||ready(data,blocks)==under||!retry(reader)) {
                std::cerr<<"role quota blocks="<<blocks<<" binary="<<binary<<" which="<<which<<" under="<<under<<" cap="<<usage[which]
                    <<" state="<<(blocks?static_cast<int>(dx_iface::blockRoleReport(data).state):static_cast<int>(dx_iface::entitiesRoleReport(data).state))<<'\n';return false;
            }++quotas;
        }
        for(long fault=0;fault<16;++fault) {dx_iface reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!ready(data,blocks))return false;
            const auto count=symbolFailedAllocations;{SymbolAllocationFailure fail(fault);if(blocks)A::repeatBlockRole(reader,data);else A::repeatEntitiesRole(reader,data);}
            const bool reached=count!=symbolFailedAllocations;
            if(ready(data,blocks)==reached) {std::cerr<<"role allocation blocks="<<blocks<<" binary="<<binary<<" offset="<<fault<<" reached="<<reached<<'\n';return false;}
            if(blocks)A::repeatBlockRole(reader,data);else A::repeatEntitiesRole(reader,data);
            if(!ready(data,blocks)||!retry(reader)){std::cerr<<"role allocation recovery blocks="<<blocks<<" binary="<<binary<<" offset="<<fault<<'\n';return false;}
            if(reached)++allocations;else ++unreached;
        }
    }
    stage.done=true;std::cout<<"TABLE independent profiles="<<profiles<<"; original scalar spans="<<spans<<"; scalar type/schema refusals="<<types<<"; grammar refusals="<<refusals
        <<"; target controls="<<targets<<"; census controls="<<census<<"; context guards="<<contexts<<"; role quotas="<<quotas<<"; reached role allocation faults="<<allocations<<"; unreached offsets="<<unreached
        <<"; extended profiles="<<extensions<<"; extended scalar spans="<<extensionSpans<<"; proxy one-over boundary controls="<<proxyBoundary<<"; shape controls="<<shapes<<"; shape scalar spans="<<shapeSpans<<'\n';
    return profiles==96u&&spans==8208u&&refusals==96u&&types>0u&&targets==40u&&census==12u&&contexts==36u&&quotas==16u&&allocations>0u&&allocations+unreached==64u&&extensions==14u&&extensionSpans>0u&&proxyBoundary==4u&&shapes==12u&&shapeSpans>0u;
}
