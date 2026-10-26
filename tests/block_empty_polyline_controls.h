// SPDX-License-Identifier: GPL-2.0-or-later
// Structural admission of an already accepted parent+SEQEND, not a claim that
// an empty polyline has useful geometry or is accepted by every CAD producer.
BlockFixture emptyBlockPolylineFixture(const std::string& version,bool threeD,bool blockOwner,int follow=1) {
    auto f=blockFixture(version,0u,true);
    Tags children{{0,"POLYLINE"},{5,"D1"},{330,"B0"},{100,"AcDbEntity"},{8,"PL"},
        {100,threeD?"AcDb3dPolyline":"AcDb2dPolyline"},{70,threeD?"8":"0"},{10,"0"},{20,"0"},{30,"2.5"}};
    if(follow>=0)children.push_back({66,std::to_string(follow)});
    children.insert(children.end(),{{0,"SEQEND"},{5,"D4"},{330,blockOwner?"B0":"D1"},{100,"AcDbEntity"},{8,"TAIL"}});
    const auto at=std::find_if(f.document.begin(),f.document.end(),[](const Tag& t){return t.code==0&&t.value=="ENDBLK";});
    f.document.insert(at,children.begin(),children.end());return f;
}
struct EmptyBlockPolylineProbe final:dx_iface {
    int fault=-1;unsigned calls=0u;
    void addPolyline(const DRW_Polyline& value) override {
        ++calls;auto& actual=const_cast<DRW_Polyline&>(value);const auto footer=actual.seqEndEntity;
        const auto handle=footer?footer->handle:0u,owner=footer?footer->parentHandle:0u;auto layer=footer?footer->layer:UTF8STRING{};
        const auto count=actual.vertlist.size();
        const auto restore=[&](){actual.vertlist.resize(count);actual.seqEndEntity=footer;if(footer){footer->handle=handle;footer->parentHandle=owner;footer->layer=std::move(layer);}};
        try {
            if(fault==0)actual.seqEndEntity.reset();if(fault==1&&footer)++footer->handle;
            if(fault==2&&footer)footer->parentHandle=0xFFFFu;if(fault==3)actual.vertlist.push_back(nullptr);
            if(fault==4&&footer)footer->layer="changed footer layer";
            if(fault==5){const auto copied=value;dx_iface::addPolyline(copied);}else dx_iface::addPolyline(value);
        }catch(...){restore();throw;}restore();
    }
};
bool emptyBlockPolylineTests(const std::filesystem::path& path) {
    struct Stage{const char* name="profiles";bool done=false;~Stage(){if(!done)std::cerr<<"Empty BLOCKS POLYLINE stage="<<name<<'\n';}}stage;
    using A=DxIfaceTableSourceTestAccess;using M=dx_block_membership::Role;using O=dx_block_membership::Owner;
    using S=dx_iface::BlockOriginState;
    unsigned profiles=0u,scalars=0u,wireFaults=0u,admissionRefusals=0u,callbackFaults=0u,quotas=0u,exceptions=0u;
    const auto good=[](const dx_data& d){return dx_iface::blockOriginReport(d).state==S::OriginsBound
        &&A::membershipMatched(d)&&A::compoundSourceMatched(d,0u)&&!A::compoundFailed(d);};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)
        for(bool threeD:{false,true})for(bool blockOwner:{false,true})for(int follow:{-1,0,2}) {
            const auto f=emptyBlockPolylineFixture(version,threeD,blockOwner,follow);if(!write(path,f.document,encoding==2u,encoding==1u))return false;
            BlockOriginProbe reader;dx_data data;const bool read=reader.fileImport(path.string(),&data,false);
            if(!read||!reader.good||!good(data)){const auto* b=A::block(data);std::cerr<<version<<'/'<<encoding<<'/'<<threeD<<'/'<<blockOwner<<'/'<<follow
                <<" import="<<read<<" origin="<<static_cast<int>(dx_iface::blockOriginReport(data).state)<<" membership="<<A::membershipMatched(data)
                <<" range="<<(b&&b->membership?static_cast<int>(b->membership->ranges.front().reason):-1)<<'\n';return false;}
            const auto& block=*A::block(data);const auto& members=*block.membership;const auto* body=A::compoundBody(data,0u);
            const auto* polyline=body?dynamic_cast<const DRW_Polyline*>(body->model.get()):nullptr;
            if(!polyline||!polyline->vertlist.empty()||!polyline->seqEndEntity||polyline->seqEndEntity->handle!=0xD4u
                ||polyline->seqEndEntity->parentHandle!=(blockOwner?0xB0u:0xD1u)||body->sourceStates.size()!=2u||body->profiles.size()!=2u
                ||members.members.size()!=2u||members.ranges.size()!=1u||members.ranges[0].first||members.ranges[0].next!=2u
                ||members.members[0].role!=M::Primary||members.members[1].role!=M::SequenceEnd||members.members[1].parent!=0u
                ||members.members[1].ownerRole!=(blockOwner?O::BlockRecord:O::CompoundParent)||dx_iface::blockOriginReport(data).events!=1u)return false;
            const auto wire=attdefWireTags(path,encoding==2u);
            for(const auto& child:block.children->records)for(std::size_t i=0u;i<child.groups.size();++i){const auto& span=child.spans[i];const auto& w=wire[span.ordinal-1u];
                if(w.code!=child.groups[i].code()||w.begin!=span.beginOffset||w.end!=span.endOffset)return false;++scalars;}
            ++profiles;
        }
    for(bool binary:{false,true})for(bool blockOwner:{false,true}) {
        const auto f=emptyBlockPolylineFixture("AC1032",false,blockOwner);if(!write(path,f.document,binary))return false;
        BlockOriginProbe baseline;dx_data base;if(!baseline.fileImport(path.string(),&base,false)||!good(base))return false;
        stage.name="missing/foreign/duplicate physical footer identities";
        for(unsigned fault=0u;fault<7u;++fault){auto varied=f;
            if(fault==0u)eraseMembershipRecord(varied.document,"D4");
            if(fault==1u)varied.document[membershipSlot(varied.document,"D4",330)].value="FFFF";
            if(fault==2u)varied.document[membershipSlot(varied.document,"D4",5)].value="D1";
            if(fault==3u||fault==4u){const auto at=membershipSlot(varied.document,"D4",fault==3u?5:330);varied.document.erase(varied.document.begin()+static_cast<std::ptrdiff_t>(at));}
            if(fault==5u)varied.document[membershipSlot(varied.document,"D1",330)].value="FFFF";
            if(fault==6u){const auto at=membershipSlot(varied.document,"D4",330);const auto copy=varied.document[at];varied.document.insert(varied.document.begin()+static_cast<std::ptrdiff_t>(at),copy);}
            if(!write(path,varied.document,binary))return false;BlockOriginProbe reader;dx_data data;const bool imported=reader.fileImport(path.string(),&data,false);
            if(A::membershipMatched(data)||A::compoundSourceMatched(data,0u)||(fault==0u&&imported))return false;if(!imported)++admissionRefusals;
            if(!write(path,f.document,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!good(retry))return false;++wireFaults;
        }
        stage.name="staged footer identity/body and equal copied parent";
        for(int fault=0;fault<6;++fault){EmptyBlockPolylineProbe reader;reader.fault=fault;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||reader.calls!=1u)return false;
            const bool membership=A::membershipMatched(data),body=A::compoundSourceMatched(data,0u);const auto origin=dx_iface::blockOriginReport(data).state;
            if(fault<4?(membership||body||origin!=S::OriginsBound):fault==4?(!membership||body||origin!=S::OriginsBound):(!membership||!body||origin!=S::Unqualified))return false;
            reader.fault=-1;reader.calls=0u;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!good(retry))return false;++callbackFaults;
        }
        stage.name="exact/one-under membership and body quotas";
        for(bool body:{false,true})for(unsigned dimension=0u;dimension<3u;++dimension)for(bool below:{false,true}) {
            const auto usage=body?A::compoundUsage(base):A::membershipUsage(base);if(!usage[dimension])return false;
            BlockOriginProbe reader;dx_data data;if(body)A::compoundLimit(data,dimension,usage[dimension]-(below?1u:0u));else A::membershipLimit(data,dimension,usage[dimension]-(below?1u:0u));
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::blockOriginReport(data).state!=S::OriginsBound
                ||(below?(body?(!A::compoundFailed(data)||!A::membershipMatched(data)||A::compoundSourceMatched(data,0u)):(!A::membershipFailed(data)||A::membershipMatched(data))):!good(data)))return false;
            dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!good(retry))return false;++quotas;
        }
        stage.name="callback/tail failure and corrected import";
        for(int phase=1;phase<=12;++phase){auto varied=f;if(phase==12)varied.document.pop_back();if(!write(path,varied.document,binary))return false;
            BlockOriginProbe reader;reader.throwAt=phase<12?phase:-1;dx_data data;bool threw=false,read=false;
            try{read=reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
            if(read||threw!=(phase<12)||reader.depth||A::membershipMatched(data)||A::compoundSourceMatched(data,0u)||dx_iface::blockOriginReport(data).state!=S::ImportFailed)return false;
            reader.throwAt=-1;if(!write(path,f.document,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!good(retry))return false;++exceptions;
        }
    }
    std::cout<<"Empty BLOCKS POLYLINE profiles="<<profiles<<" independent scalar spans="<<scalars<<" wire faults/retries="<<wireFaults<<" existing admission refusals="<<admissionRefusals
        <<" callback faults/retries="<<callbackFaults<<" quota exact/under/retries="<<quotas<<" exceptions/tails/retries="<<exceptions<<'\n';stage.done=true;
    return profiles==144u&&wireFaults==28u&&callbackFaults==24u&&quotas==48u&&exceptions==48u;
}
