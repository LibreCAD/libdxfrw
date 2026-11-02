// SPDX-License-Identifier: GPL-2.0-or-later
// All drawing values/recipes are authored here, not external fixture derivatives.
Tags tableValueProfileDocument(const std::string& version,bool blocks,unsigned form) {
    constexpr int types[]={1,2,4,8,8,16,32,64,0,1,32,8,4,2,2,4,64,2};
    const int flags=form==8u?7:(form>=9u&&form<=11u?5:(form==12u?1:(form==17u?4:6)));
    auto tags=tableValuePayloadDocument(version,blocks,types[form],flags);
    const auto field=[&](int code){auto i=blockField(tags,"501",301)+1u;while(i<tags.size()&&tags[i].code!=code)++i;return i;};
    if(form==2u||form==15u) {
        const auto at=field(1);tags[at]={2,"payload"};tags[at+1u]={1,"-é"};
        if(form==15u){tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at));tags[at].value="";}
    }
    if(form==4u){const auto at=field(92);tags[at].value="16";tags[at+1u].value="00112233445566778899AABBCCDDEEFF";
        tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at+2u));}
    if(form==5u||form==6u)tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(field(92)));
    if(form==13u||form==14u)tags[field(94)].value=form==13u?"16":"32";
    if(flags&1){tags[field(94)].value="0";tags[field(300)].value="";tags[field(302)].value="";}
    if(form==12u){tags[blockField(tags,"501",171)].value="2";const auto at=blockField(tags,"501",301);
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{340,"F00"},{144,"1.5"}});}
    if(form==7u||form==16u) {
        tags[field(330)].value=form==16u?"0":"FFEE1122";
        const auto at=blockField(tags,"501",301);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{344,"F11"}});
    }
    return tags;
}
bool tableValueProfileTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;using namespace dx_attdef_source;
    const auto path=dir/"table-value-profiles.dxf";
    unsigned positives=0u,types=0u,missing=0u,duplicate=0u,unknown=0u,grammar=0u,roles=0u,contexts=0u,quotas=0u,reached=0u,unreached=0u,retries=0u;
    std::size_t spans=0u;
    const auto ready=[](const dx_data& data){const auto r=dx_iface::tableBodyReport(data);
        return r.modeledMatched&&r.profilesMatched&&r.semanticComplete==1u&&!r.ownerTargetsResolved&&!r.outputEligible;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)
        for(bool blocks:{false,true})for(unsigned form=0u;form<18u;++form) {
            dx_iface reader;dx_data data;const auto tags=tableValueProfileDocument(version,blocks,form);
            if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data)) {
                std::cerr<<"TABLE Value profile version="<<version<<" encoding="<<encoding<<" blocks="<<blocks<<" form="<<form<<'\n';
                if(!A::tableBodies(data).empty()) {
                    for(const auto& p:A::tableBodies(data).begin()->second->profiles)
                        std::cerr<<"profile reason="<<static_cast<int>(p.reason)<<" group="<<p.rejectedGroup<<'\n';
                }
                return false;
            }
            const auto& body=*A::tableBodies(data).at(0x501u);
            for(const auto* table:{body.source->model.get(),body.typed->model.get()}) {
                const auto& v=table->m_content.m_rows[0].m_cells[0].m_contents[0].m_value;
                const bool empty=form>=8u&&form<=12u;
                if(v.m_unitType!=(empty?0:(form==13u?16:(form==14u?32:8)))
                    ||v.m_formatString!=(empty?"":"value-format-é")||v.m_valueString!=(empty?"":"display-cache-é"))return false;
            }
            const auto& records=blocks?body.source->blockPhysical->records:body.entitiesPhysical->records;
            const auto wire=attdefWireTags(path,encoding==2u);
            for(auto i=body.first;i<body.next;++i)if(!tableProfileWire(wire,records[i],body.profiles[i-body.first],spans))return false;
            if(form==7u||form==16u) {
                const auto& profile=body.profiles.front();unsigned owner=0u,field=0u,value=0u;
                for(const auto& span:profile.spans) {
                    if(span.role==Role::Owner){if(span.handle!=(blocks?0xB3u:0xB0u))return false;++owner;}
                    if(span.role==Role::TableCellField){if(span.code!=344||span.handle!=0xF11u)return false;++field;}
                    if(span.role==Role::TableValueObject){if(span.code!=330||span.handle!=(form==16u?0u:0xFFEE1122u))return false;++value;}
                }
                const auto check=[&](const DRW_Table& t){const auto& c=t.m_content.m_rows[0].m_cells[0].m_contents[0];
                    return t.parentHandle==(blocks?0xB3u:0xB0u)&&c.m_handle==0xF11u&&c.m_value.m_handle==(form==16u?0u:0xFFEE1122u);};
                if(owner!=1u||field!=1u||value!=1u||!check(*body.source->model)||!check(*body.typed->model))return false;++roles;
            }
            ++positives;
        }
    // Original raw mutations bypass the tolerant reader. Malformed duplicate,
    // absent, mistyped and extra tags cannot disappear behind replay equality.
    for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true})for(unsigned form=0u;form<18u;++form) {
        const auto good=tableValueProfileDocument("AC1032",blocks,form);dx_iface reader;dx_data data;
        if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data))return false;
        const auto& original=*A::tableInputs(data).at(0x501u)->raw;
        bool value=false;
        for(std::size_t i=0u;i<original.groups.size();++i) {
            const int code=original.groups[i].code();if(code==301)value=true;if(!value)continue;
            auto raw=original;
            if(raw.groups[i].type()==DRW_Variant::STRING)raw.groups[i].addInt(code,1);else raw.groups[i].addString(code,"wrong-type");
            if(dx_table_source::audit(raw).complete())return false;++types;
            raw=original;DRW_Variant extra(99,7);raw.groups.insert(raw.groups.begin()+static_cast<std::ptrdiff_t>(i),extra);
            if(dx_table_source::audit(raw).complete())return false;++unknown;
            if(code!=2) {
                raw=original;raw.groups.erase(raw.groups.begin()+static_cast<std::ptrdiff_t>(i));
                if(dx_table_source::audit(raw).complete()) {std::cerr<<"Value missing accepted form="<<form<<" code="<<code<<'\n';return false;}++missing;
            }
            if(code!=2&&code!=310) {
                raw=original;raw.groups.insert(raw.groups.begin()+static_cast<std::ptrdiff_t>(i),original.groups[i]);
                if(dx_table_source::audit(raw).complete())return false;++duplicate;
            }
            if(code==304)break;
        }
        // Corrected source must pass through the same adapter after each batch.
        dx_data corrected;if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&corrected,false)||!ready(corrected))return false;++retries;
    }
    for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true}) {
        for(unsigned fault=0u;fault<30u;++fault) {
            const unsigned form=fault>=12u&&fault<=15u?4u:(fault==16u||fault==23u?6u:(fault==17u?2u:
                (fault==18u||fault==19u||fault==29u?7u:(fault==20u||fault==21u?12u:0u))));
            dx_iface reader;dx_data data;const auto good=tableValueProfileDocument("AC1032",blocks,form);
            if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data))return false;
            auto raw=*A::tableInputs(data).at(0x501u)->raw;
            const auto field=[&](int code){std::size_t i=0u;while(i<raw.groups.size()&&raw.groups[i].code()!=301)++i;
                ++i;while(i<raw.groups.size()&&raw.groups[i].code()!=code)++i;return i;};
            switch(fault) {
            case 0:raw.groups[field(93)].addInt(93,8);break;
            case 1:raw.groups[field(93)].addInt(93,-1);break;
            case 2:raw.groups[field(94)].addInt(94,64);break;
            case 3:raw.groups[field(94)].addInt(94,3);break;
            case 4:case 5:case 6:case 7:raw.groups[field(90)].addInt(90,fault==4u?128:(fault==5u?256:(fault==6u?512:0)));break;
            case 8:raw.groups[field(93)].addInt(93,1);break;
            case 9:std::swap(raw.groups[field(93)],raw.groups[field(90)]);break;
            case 10:std::swap(raw.groups[field(94)],raw.groups[field(300)]);break;
            case 11:std::swap(raw.groups[field(302)],raw.groups[field(300)]);break;
            case 12:raw.groups[field(92)].addInt(92,8);break;
            case 13:raw.groups[field(310)].addString(310,"AAB");break;
            case 14:raw.groups[field(310)].addString(310,"AZ");break;
            case 15:raw.groups[field(310)].addString(310,std::string(256u,'A'));break;
            case 16:raw.groups.erase(raw.groups.begin()+static_cast<std::ptrdiff_t>(field(31)));break;
            case 17:std::swap(raw.groups[field(2)],raw.groups[field(1)]);break;
            case 18:raw.groups[field(330)].addString(330,"100000000");break;
            case 19:raw.groups[field(330)].addString(330,"not-hex");break;
            case 20:raw.groups[field(300)].addString(300,"unproved-block-format");break;
            case 21:raw.groups[field(93)].addInt(93,5);break;
            case 22:raw.groups[field(94)].addInt(94,-1);break;
            case 23:raw.groups.insert(raw.groups.begin()+static_cast<std::ptrdiff_t>(field(11)),DRW_Variant(92,24));break;
            case 24:raw.groups.insert(raw.groups.begin()+static_cast<std::ptrdiff_t>(field(304)+1u),DRW_Variant(330,"FFEE1122"));break;
            case 25:raw.groups.insert(raw.groups.begin()+static_cast<std::ptrdiff_t>(field(90)),DRW_Variant(100,"AcDbEntity"));break;
            case 26:raw.groups.insert(raw.groups.begin()+static_cast<std::ptrdiff_t>(field(90)),DRW_Variant(102,"{opaque"));break;
            case 27:raw.groups.insert(raw.groups.begin()+static_cast<std::ptrdiff_t>(field(90)),DRW_Variant(1001,"APP"));break;
            case 28:raw.groups[field(304)].addString(304,"END_VALUE");break;
            default:raw.groups[field(330)].addString(344,"FFEE1122");break;
            }
            if(dx_table_source::audit(raw).complete()){std::cerr<<"Value grammar accepted fault="<<fault<<'\n';return false;}
            if(!dx_table_source::audit(*A::tableInputs(data).at(0x501u)->raw).complete())return false;++grammar;
        }
        // Byte-identical handles/records in another import are not the
        // original callback/control-block identity for this TABLE body.
        for(unsigned fault=0u;fault<7u;++fault) {
            const auto good=tableValueProfileDocument("AC1032",blocks,7u);dx_iface reader;dx_data data,other;
            if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&data,false)||!ready(data))return false;
            if(fault<2u)A::substituteTableBody(data,fault==1u);
            else if(fault<6u)A::substituteTableInput(data,fault>=4u,fault%2u!=0u);
            else {dx_iface another;if(!another.fileImport(path.string(),&other,false)||!ready(other))return false;A::transplantTableInput(data,other);}
            const auto r=dx_iface::tableBodyReport(data);
            if(r.state!=dx_iface::TableBodyState::ContextMismatch||r.modeledMatched||r.profilesMatched||r.outputEligible)return false;
            dx_data corrected;if(!reader.fileImport(path.string(),&corrected,false)||!ready(corrected))return false;++contexts;++retries;
        }
        const auto good=tableValueProfileDocument("AC1032",blocks,4u);dx_iface reader;dx_data base;
        if(!writeBlockBinding(path,good,encoding)||!reader.fileImport(path.string(),&base,false)||!ready(base))return false;
        const auto report=dx_iface::tableBodyReport(base);const std::size_t uses[]={report.records,report.bytes,report.work};
        for(unsigned which=0u;which<3u;++which)for(bool under:{false,true}) {
            dx_data data;A::tableBodyLimit(data,which,uses[which]-(under?1u:0u));
            if(!reader.fileImport(path.string(),&data,false)||ready(data)==under)return false;
            const auto r=dx_iface::tableBodyReport(data);
            if(under&&(r.state!=dx_iface::TableBodyState::CaptureFailed||r.profilesMatched||r.outputEligible))return false;++quotas;
        }
        for(long offset=0;offset<16;++offset) {
            dx_data data;if(!reader.fileImport(path.string(),&data,false)||!ready(data))return false;
            const auto before=symbolFailedAllocations;
            {SymbolAllocationFailure fail(offset);A::rebuildTableBodies(reader,data);}
            if(symbolFailedAllocations!=before)++reached;else {if(!ready(data))return false;++unreached;}
            if(dx_iface::tableBodyReport(data).outputEligible)return false;
            dx_data corrected;if(!reader.fileImport(path.string(),&corrected,false)||!ready(corrected))return false;++retries;
        }
    }
    std::cout<<"TABLE independent Value profiles="<<positives<<"; physical spans="<<spans<<"; owner/FIELD/value roles="<<roles
        <<"; mistyped="<<types<<"; missing="<<missing<<"; duplicate="<<duplicate<<"; unknown="<<unknown<<"; grammar refusal/corrections="<<grammar<<"; immutable/cross-import refusals="<<contexts
        <<"; exact/one-under proof limits="<<quotas<<"; reached allocation faults="<<reached<<"; unreached offsets="<<unreached<<"; corrected retries="<<retries<<'\n';
    return positives==432u&&roles==48u&&types>0u&&missing>0u&&duplicate>0u&&unknown>0u&&grammar==180u&&contexts==42u&&quotas==36u&&reached>0u&&reached+unreached==96u&&retries==246u;
}
