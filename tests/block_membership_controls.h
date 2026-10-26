// SPDX-License-Identifier: GPL-2.0-or-later
std::size_t membershipSlot(const Tags& tags,const char* handle,int code) {
    for(std::size_t i=1u;i<tags.size();++i)if(tags[i-1u].code==0&&tags[i].code==5&&tags[i].value==handle) {
        for(;i<tags.size()&&tags[i].code!=0;++i)if(tags[i].code==code)return i;
    }
    throw std::runtime_error("missing membership field");
}
void eraseMembershipRecord(Tags& tags,const char* handle) {
    const auto first=membershipSlot(tags,handle,5)-1u;auto end=first+1u;
    while(end<tags.size()&&tags[end].code!=0)++end;
    tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(first),tags.begin()+static_cast<std::ptrdiff_t>(end));
}
class MembershipProbe final:public BlockSourceProbe {
public:
    bool deliveryWanted=true,premature=false;int fault=0;
    bool wantsDxfBlockDeliverySource() const override {return deliveryWanted&&dx_iface::wantsDxfBlockDeliverySource();}
    void addPolyline(const DRW_Polyline& value) override {
        if(fault>=1&&fault<=6) {
            auto changed=value;
            if(fault==1)changed.vertlist.pop_back();if(fault==2)std::swap(changed.vertlist[0],changed.vertlist[1]);
            if(fault==3)changed.vertlist[0].reset();if(fault==4)changed.seqEndEntity.reset();
            if(fault==5)++changed.seqEndEntity->parentHandle;if(fault==6)++changed.vertlist[0]->handle;
            BlockSourceProbe::addPolyline(changed);
        }else if(fault==11) {
            auto changed=value;changed.vertlist[0]->basePoint.x+=3.0;BlockSourceProbe::addPolyline(changed);
        }else BlockSourceProbe::addPolyline(value);
        if(DxIfaceTableSourceTestAccess::membershipMatched(*cData))premature=true;
    }
    void addInsert(const DRW_Insert& value) override {
        if(fault>=7&&fault<=10) {
            auto changed=value;if(fault==7)changed.attlist.clear();if(fault==8)changed.seqEndEntity.reset();
            if(fault==9)++changed.attlist[0]->parentHandle;if(fault==10)changed.attlist[0].reset();
            BlockSourceProbe::addInsert(changed);
        }else if(fault==12) {
            DRW_Point wrong;wrong.eType=value.eType;wrong.handle=value.handle;wrong.parentHandle=value.parentHandle;
            dx_iface::addPoint(wrong); // same public type/IDs, wrong actual class/route
        }else BlockSourceProbe::addInsert(value);
        if(DxIfaceTableSourceTestAccess::membershipMatched(*cData))premature=true;
    }
    void endBlock() override {
        BlockSourceProbe::endBlock();if(DxIfaceTableSourceTestAccess::membershipMatched(*cData))premature=true;
    }
};

bool blockMembershipTests(const std::filesystem::path& path) {
    using A=DxIfaceTableSourceTestAccess;namespace M=dx_block_membership;
    unsigned profiles=0u,variants=0u,emptyParentRoutes=0u,wireFaults=0u,admissionRefusals=0u,callbacks=0u,quotas=0u,contexts=0u,stale=0u,copied=0u,dispositions=0u,orphans=0u,exceptions=0u,bodyBoundaries=0u;
    std::size_t spans=0u;long allocations=0;
    const auto good=[&](const dx_data& data){
        if(A::membershipMatched(data))return true;
        const auto* block=A::block(data);std::cerr<<"membership capture failure="<<A::membershipFailed(data);
        if(block&&block->membership) {
            for(const auto& m:block->membership->members)std::cerr<<" member "<<m.self<<":"<<static_cast<int>(m.reason);
            for(const auto& r:block->membership->ranges)std::cerr<<" range "<<r.first<<":"<<static_cast<int>(r.reason);
            std::cerr<<" observed="<<block->membershipObserved;
        }
        std::cerr<<'\n';return false;
    };
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0;encoding<3u;++encoding)for(unsigned shape=0;shape<3u;++shape) {
        const auto f=shape==2u?mixedBlockChildrenFixture(version):blockFixture(version,shape,true);
        if(!write(path,f.document,encoding==2u,encoding==1u))return false;
        MembershipProbe reader;dx_data data;
        if(!check(reader.fileImport(path.string(),&data,false)&&good(data)&&!reader.premature
            &&originalChildrenWire(path,encoding==2u,f,reader.childrenAlias,spans),"actual original structural membership, independent physical scalar spans and successful replay"))return false;
        const auto* block=A::block(data);const auto& audit=*block->membership;
        if(audit.container!=&data||audit.children!=block->children||audit.delivery!=block->delivery||audit.begin!=block->original||audit.end!=block->footer)return false;
        if(shape==2u) {
            const std::array<std::uint32_t,10> self{{0xA1u,0xC1u,0xD1u,0xD2u,0xD3u,0xD4u,0xE1u,0xE2u,0xE3u,0xA2u}};
            const std::array<std::size_t,10> dispatch{{0u,1u,2u,2u,2u,2u,3u,3u,3u,4u}},parent{{0u,1u,2u,2u,2u,2u,6u,6u,6u,9u}};
            const std::array<M::Role,10> role{{M::Role::Primary,M::Role::Primary,M::Role::Primary,M::Role::Vertex,M::Role::Vertex,
                M::Role::SequenceEnd,M::Role::Primary,M::Role::Attribute,M::Role::SequenceEnd,M::Role::Primary}};
            if(audit.members.size()!=10u||audit.ranges.size()!=5u)return false;
            for(std::size_t i=0;i<self.size();++i) {
                const auto& member=audit.members[i];const bool primary=parent[i]==i;
                if(member.self!=self[i]||member.dispatch!=dispatch[i]||member.parent!=parent[i]||member.role!=role[i]
                    ||member.reason!=M::Reason::Matched||member.selfGroup!=0u||member.ownerGroup!=1u
                    ||member.owner!=(primary?0xB0u:self[parent[i]])
                    ||member.ownerRole!=(primary?M::Owner::BlockRecord:M::Owner::CompoundParent))return false;
            }
        }
        ++profiles;
    }
    for(bool binary:{false,true}) {
        const auto f=mixedBlockChildrenFixture("AC1032");
        for(unsigned mode=0;mode<8u;++mode) {
            auto varied=f;
            if(mode==0u)for(const char* id:{"D4","E3"})varied.document[membershipSlot(varied.document,id,330)].value="B0";
            if(mode==1u) {
                const auto i=membershipSlot(varied.document,"D2",330);const auto owner=varied.document[i];
                varied.document.erase(varied.document.begin()+static_cast<std::ptrdiff_t>(i));
                const auto at=membershipSlot(varied.document,"D2",100)+1u;
                varied.document.insert(varied.document.begin()+static_cast<std::ptrdiff_t>(at),owner);
            }
            if(mode==2u) {
                const auto at=membershipSlot(varied.document,"D2",100);
                varied.document.insert(varied.document.begin()+static_cast<std::ptrdiff_t>(at),{{102,"{OPAQUE"},{5,"DEAD"},{330,"FACE"},
                    {102,"{NESTED"},{100,"AcDbEntity"},{5,"B0"},{102,"}"},{102,"}"}});
            }
            if(mode==3u)varied.document[membershipSlot(varied.document,"D1",66)].value="2"; // obsolete, ignored
            if(mode==4u) {
                eraseMembershipRecord(varied.document,"E2");eraseMembershipRecord(varied.document,"E3");
                const auto i=membershipSlot(varied.document,"E1",66);varied.document.erase(varied.document.begin()+static_cast<std::ptrdiff_t>(i));
            }
            if(mode==5u)eraseMembershipRecord(varied.document,"E2"); // explicit empty chain
            if(mode==6u)varied.document[membershipSlot(varied.document,"E1",5)-1u].value="MINSERT";
            if(mode==7u) {
                const auto first=membershipSlot(varied.document,"E2",5)-1u;auto end=first+1u;
                while(varied.document[end].code!=0)++end;
                Tags copy(varied.document.begin()+static_cast<std::ptrdiff_t>(first),varied.document.begin()+static_cast<std::ptrdiff_t>(end));
                copy[1].value="E4";varied.document.insert(varied.document.begin()+static_cast<std::ptrdiff_t>(end),copy.begin(),copy.end());
            }
            if(!write(path,varied.document,binary))return false;
            MembershipProbe reader;dx_data data;
            if(!check(reader.fileImport(path.string(),&data,false)&&good(data),"observed footer owner forms, order-independent common IDs, opaque scopes and declared/default compound variants")) {
                std::cerr<<"membership positive mode="<<mode<<'\n';return false;
            }
            if(mode==0u&&(A::block(data)->membership->members[5].ownerRole!=M::Owner::BlockRecord
                ||A::block(data)->membership->members[5].parent!=2u))return false;
            ++variants;
        }
        auto emptyInsert=f;eraseMembershipRecord(emptyInsert.document,"E2");eraseMembershipRecord(emptyInsert.document,"E3");
        const auto flag=membershipSlot(emptyInsert.document,"E1",66);
        emptyInsert.document.erase(emptyInsert.document.begin()+static_cast<std::ptrdiff_t>(flag));
        if(!write(path,emptyInsert.document,binary))return false;
        MembershipProbe wrongClass;wrongClass.fault=12;dx_data routed;
        if(!check(wrongClass.fileImport(path.string(),&routed,false)&&!A::membershipMatched(routed)
            &&!A::originMatched(wrongClass,routed),"standalone INSERT cannot borrow the same eType/IDs delivered through a POINT class"))return false;
        wrongClass.fault=0;dx_data correctedRoute;
        if(!wrongClass.fileImport(path.string(),&correctedRoute,false)||!good(correctedRoute))return false;
        ++emptyParentRoutes;
        for(unsigned fault=0;fault<16u;++fault) {
            auto bad=f;auto& tags=bad.document;
            if(fault<4u)tags[membershipSlot(tags,fault==0u?"C1":fault==1u?"D2":fault==2u?"E2":"D4",330)].value="BEEF";
            if(fault==4u)tags[membershipSlot(tags,"D3",5)].value="D2";
            if(fault==5u)tags[membershipSlot(tags,"D1",5)].value="B0";
            if(fault==6u)tags[membershipSlot(tags,"D4",5)].value="B2";
            if(fault==7u||fault==8u) {const auto at=membershipSlot(tags,"D2",fault==7u?5:330);tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at));}
            if(fault==9u||fault==10u) {const auto at=membershipSlot(tags,"D2",fault==9u?5:330);const auto tag=tags[at];tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),tag);}
            if(fault==11u||fault==12u) {
                auto end=membershipSlot(tags,"D2",5);while(tags[end].code!=0)++end;
                tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),{fault==11u?5:330,"FFFF"});
            }
            if(fault==13u||fault==15u)tags[membershipSlot(tags,"E1",66)].value=fault==13u?"0":"2";
            if(fault==14u) {const auto at=membershipSlot(tags,"E1",66);tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at));}
            if(!write(path,tags,binary))return false;
            MembershipProbe reader;dx_data data;
            const bool imported=reader.fileImport(path.string(),&data,false);
            // A SEQEND sharing ENDBLK's identity is already refused by the
            // reader's duplicate endpoint identity check, not this audit.
            if(!check(imported!=(fault==6u)&&!A::membershipMatched(data)&&!A::originMatched(reader,data),
                "wrong/duplicate/missing original IDs, body aliases and follow flags cannot qualify structural roles or selected ATTDEF origin")) {
                std::cerr<<"membership wire fault="<<fault<<'\n';return false;
            }
            if(!imported)++admissionRefusals;
            if(!write(path,f.document,binary))return false;
            dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!good(retry))return false;++wireFaults;
        }
        if(!write(path,f.document,binary))return false;
        MembershipProbe baselineReader;dx_data baseline;if(!baselineReader.fileImport(path.string(),&baseline,false)||!good(baseline))return false;
        const auto usage=A::membershipUsage(baseline);const auto saved=A::block(baseline)->membership;
        dx_data borrowed;A::copyMembershipContext(borrowed,baseline);
        if(A::membershipMatched(borrowed)||A::originMatched(baselineReader,borrowed))return false;
        ++copied;
        for(int fault=1;fault<=11;++fault) {
            MembershipProbe reader;reader.fault=fault;dx_data data;
            if(!reader.fileImport(path.string(),&data,false))return false;
            if(fault==11) {if(!good(data))return false;++bodyBoundaries;}
            else {if(!A::membershipFailed(data)||A::membershipMatched(data)||A::originMatched(reader,data))return false;++callbacks;}
            reader.fault=0;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!good(retry))return false;
        }
        for(unsigned dimension=0;dimension<3u;++dimension) {
            MembershipProbe reader;dx_data exact;A::membershipLimit(exact,dimension,usage[dimension]);
            if(!reader.fileImport(path.string(),&exact,false)||!good(exact))return false;
            dx_data below;A::membershipLimit(below,dimension,usage[dimension]-1u);
            if(!reader.fileImport(path.string(),&below,false)||!A::membershipFailed(below)||A::membershipMatched(below)||A::originMatched(reader,below))return false;
            dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!good(retry))return false;++quotas;
        }
        bool calibrated=false;
        for(long fault=0;fault<20&&!calibrated;++fault) {
            std::weak_ptr<const M::Audit> weak;
            {
                MembershipProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!good(data))return false;
                long used=0;const bool captured=A::recaptureMembership(reader,data,fault,used);weak=A::block(data)->membership;
                if(captured){if(used!=fault||A::membershipMatched(data))return false;calibrated=true;}
                else {if(!A::membershipFailed(data))return false;++allocations;}
                dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!good(retry))return false;
            }
            if(!weak.expired())return false;
        }
        if(!calibrated)return false;
        MembershipProbe next;dx_data newer;if(!next.fileImport(path.string(),&newer,false)||!good(newer))return false;
        const auto current=A::block(newer)->membership;A::mutableOriginBlock(newer).membership=saved;
        if(A::membershipMatched(newer))return false;A::mutableOriginBlock(newer).membership=current;if(!good(newer))return false;++stale;
        for(int phase=1;phase<=4;++phase) {
            DeliveryProbe reader;reader.throwDelivery=phase;dx_data data;bool threw=false;
            try{(void)reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
            if(!threw||reader.depth||A::membershipMatched(data))return false;
            reader.throwDelivery=0;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!good(retry))return false;++exceptions;
        }
        for(unsigned mode=0;mode<2u;++mode) {
            MembershipProbe reader;if(mode)reader.deliveryWanted=false;else reader.selected=false;
            dx_data data;if(!reader.fileImport(path.string(),&data,false)||A::membershipMatched(data)
                ||A::membershipFailed(data)||(A::block(data)&&A::block(data)->membership))return false;
            ++contexts;
        }
        auto later=f;later.document.insert(later.document.end()-1,{{0,"SECTION"},{2,"ENTITIES"},{0,"POINT"},{5,"F1"},{10,"0"},{0,"ENDSEC"}});
        if(!write(path,later.document,binary))return false;
        MembershipProbe failed;dx_data bad;if(failed.fileImport(path.string(),&bad,false)||A::membershipMatched(bad)
            ||!A::block(bad)||!A::block(bad)->membership||!A::block(bad)->membership->complete)return false;
        ++contexts;
        if(!write(path,f.document,binary))return false;
        DeliveryProbe cleanup;cleanup.throwPhase=5;dx_data thrown;bool propagated=false;
        try{(void)cleanup.fileImport(path.string(),&thrown,false);}catch(const std::runtime_error&){propagated=true;}
        if(!propagated||cleanup.depth||A::membershipMatched(thrown))return false;++contexts;
        auto skipped=blockFixture("AC1032",0u,true);
        const auto end=std::find_if(skipped.document.begin(),skipped.document.end(),[](const Tag& t){return t.code==0&&t.value=="ENDBLK";});
        skipped.document.insert(end,{{0,"DIMENSION"},{5,"F1"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbDimension"},{70,"7"}});
        if(!write(path,skipped.document,binary))return false;
        MembershipProbe noCallback;dx_data absent;if(!noCallback.fileImport(path.string(),&absent,false)||!A::deliveryTransport(absent)
            ||A::membershipMatched(absent)||A::block(absent)->membership->ranges.front().reason!=M::Reason::Disposition)return false;
        ++dispositions;
        for(const char* type:{"VERTEX","ATTRIB","SEQEND"}) {
            auto orphan=blockFixture("AC1032",0u,true);
            const auto at=std::find_if(orphan.document.begin(),orphan.document.end(),[](const Tag& t){return t.code==0&&t.value=="ENDBLK";});
            orphan.document.insert(at,{{0,type},{5,"F1"},{330,"B0"},{100,"AcDbEntity"},{8,"0"}});
            if(!write(path,orphan.document,binary))return false;
            MembershipProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||A::membershipMatched(data))return false;++orphans;
        }
    }
    std::cout<<"Block membership profiles="<<profiles<<"; original scalar spans="<<spans<<"; positive variants="<<variants
        <<"; original wire faults/retries="<<wireFaults<<"; existing format admission refusals="<<admissionRefusals
        <<"; actual compound callback faults/retries="<<callbacks<<"; empty-parent class/route refusals="<<emptyParentRoutes
        <<"; exact/under/retries="<<quotas<<"; isolated allocation faults/retries="<<allocations<<"; stale owners/restorations="<<stale
        <<"; copied-context refusals="<<copied
        <<"; exception/retries="<<exceptions<<"; opt-out/later-failure contexts="<<contexts<<"; no-callback dispositions="<<dispositions
        <<"; orphan refusals="<<orphans<<"; body-proof boundaries="<<bodyBoundaries<<'\n';
    return profiles==36u&&spans==2712u&&variants==16u&&emptyParentRoutes==2u&&wireFaults==32u&&admissionRefusals==2u&&callbacks==20u&&quotas==6u&&allocations==8
        &&stale==2u&&copied==2u&&exceptions==8u&&contexts==8u&&dispositions==2u&&orphans==6u&&bodyBoundaries==2u;
}
