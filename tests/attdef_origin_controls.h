// SPDX-License-Identifier: GPL-2.0-or-later
class AttdefOriginProbe final:public BlockSourceProbe {
public:
    using A=DxIfaceTableSourceTestAccess;
    bool origins=true,premature=false;
    unsigned sourceBound=0u,pairBound=0u;
    bool wantsDxfBlockDeliverySource() const override {return origins&&dx_iface::wantsDxfBlockDeliverySource();}
    void addDxfAttdefSource(const DRW_Attdef& value,const DRW_RawDxfObject& raw,const DRW_DxfAttdefSourceContext& context) override {
        dx_iface::addDxfAttdefSource(value,raw,context);
        const auto* row=A::originResource(*cData,value.handle);
        if(row&&row->attdefOriginState==A::OriginState::SourceBound)++sourceBound;
        if(A::originMatched(*this,*cData,value.handle))premature=true;
    }
    void addAttDef(const DRW_Attdef& value) override {
        BlockSourceProbe::addAttDef(value);
        const auto* row=A::originResource(*cData,value.handle);
        if(row&&row->attdefOriginState==A::OriginState::PairBound)++pairBound;
        if(A::originMatched(*this,*cData,value.handle))premature=true;
    }
    void endBlock() override {
        BlockSourceProbe::endBlock();
        if(A::originMatched(*this,*cData))premature=true;
    }
};

BlockFixture originFixture(const std::string& version,unsigned mode,bool reconcile=true) {
    auto f=blockFixture(version,0u,reconcile);Tags child{{0,"ATTDEF"}};
    const auto row=body(mode);child.insert(child.end(),row.begin(),row.end());
    const auto end=std::find_if(f.document.begin(),f.document.end(),[](const Tag& t){return t.code==0&&t.value=="ENDBLK";});
    f.document.insert(end,child.begin(),child.end());return f;
}

bool attdefOriginTests(const std::filesystem::path& path) {
    using A=DxIfaceTableSourceTestAccess;using State=A::OriginState;
    unsigned profiles=0u,matched=0u,pending=0u,mutations=0u,quotas=0u,contexts=0u,replays=0u,mixed=0u;
    long allocations=0;std::size_t spans=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(unsigned mode=0u;mode<3u;++mode) {
        const bool binary=encoding==2u;const auto f=originFixture(version,mode);
        if(!write(path,f.document,binary,encoding==1u))return false;
        AttdefOriginProbe reader;dx_data data;
        const bool complete=mode==0u||version=="AC1032";
        if(!check(reader.fileImport(path.string(),&data,false)&&!A::originFailed(data)&&!reader.premature
            &&reader.sourceBound==1u&&reader.pairBound==1u&&A::originMatched(reader,data)==complete,
            "actual immutable resource binds original source/pair then qualified enclosing facts, not before import completion")) {
            const auto* row=A::originResource(data);
            std::cerr<<version<<" encoding="<<encoding<<" mode="<<mode<<" state="<<(row?static_cast<int>(row->attdefOriginState):-1)
                <<" source="<<reader.sourceBound<<" pair="<<reader.pairBound<<" origin failure="<<A::originFailed(data)
                <<" body failure="<<A::failed(data)<<" endpoint="<<A::blockEndpointProof(data)<<'\n';return false;
        }
        const auto* row=A::originResource(data);const auto* block=A::block(data);
        if(!row||!row->attdefOrigin||!block||row->rawTypedMatched||!row->attdefPairMatched||!row->attdefFieldsMatched)return false;
        const auto& origin=*row->attdefOrigin;
        if(!check(originalChildrenWire(path,binary,f,reader.childrenAlias,spans)&&origin.generation==1u&&origin.scopeId!=0u
            &&origin.scopeId==block->delivery->scopeId&&origin.block==0xB1u&&origin.blockRecord==0xB0u&&origin.handle==0xA1u&&origin.owner==0xB0u
            &&origin.dispatch==0u&&origin.sourceEvent==0u&&origin.typedEvent==1u&&origin.physical==0u&&origin.index==0u
            &&origin.begin.ordinal==reader.childrenAlias.records.front().marker.ordinal&&origin.end.ordinal==reader.childrenAlias.endMarker.ordinal
            &&origin.children==block->children&&origin.delivery==block->delivery&&origin.membership==block->membership
            &&origin.model==row->attdefModel&&origin.raw==row->raw
            &&origin.original==block->original&&origin.effective==block->effective&&origin.footer==block->footer
            &&origin.blockBegin==block->beginRaw&&origin.blockEnd==block->endRaw&&block->attdefOrigins==std::vector<std::uint32_t>{0xA1u},
            "binding pins the original physical range, exact staged pair, immutable endpoint and resource owners"))return false;
        if(row->attdefOriginState!=(complete?State::EnclosingMatched:State::PairBound))return false;
        if(complete)++matched;else ++pending;
        ++profiles;
    }
    for(bool binary:{false,true}) {
        const auto compound=mixedBlockChildrenFixture("AC1032");
        if(!write(path,compound.document,binary))return false;
        AttdefOriginProbe members;dx_data membership;
        if(!check(members.fileImport(path.string(),&membership,false)&&!members.premature
            &&members.sourceBound==2u&&members.pairBound==2u&&A::originMatched(members,membership,0xA1u)
            &&A::originMatched(members,membership,0xA2u),"separated ATTDEF resources bind their own original mixed-stream dispatches"))return false;
        const auto& first=*A::originResource(membership,0xA1u)->attdefOrigin;
        const auto& last=*A::originResource(membership,0xA2u)->attdefOrigin;
        auto& index=A::mutableOriginBlock(membership).attdefOrigins;
        if(first.dispatch!=0u||first.sourceEvent!=0u||first.typedEvent!=1u||first.physical!=0u||first.index!=0u
            ||last.dispatch!=4u||last.sourceEvent!=5u||last.typedEvent!=6u||last.physical!=9u||last.index!=1u
            ||index!=std::vector<std::uint32_t>{0xA1u,0xA2u})return false;
        std::swap(index[0],index[1]);
        if(!check(!A::originMatched(members,membership,0xA1u)&&!A::originMatched(members,membership,0xA2u),
            "reordered per-block membership cannot borrow another immutable resource origin"))return false;
        std::swap(index[0],index[1]);
        if(!A::originMatched(members,membership,0xA1u)||!A::originMatched(members,membership,0xA2u))return false;
        ++mixed;
        const auto f=originFixture("AC1032",0u);if(!write(path,f.document,binary))return false;
        AttdefOriginProbe reader;dx_data baseline;if(!reader.fileImport(path.string(),&baseline,false)||!A::originMatched(reader,baseline))return false;
        const auto usage=A::originUsage(baseline);const auto saved=A::originResource(baseline)->attdefOrigin;
        for(unsigned fault=0u;fault<27u;++fault) {
            auto changed=std::make_shared<A::Origin>(*saved);
            switch(fault) {
            case 0:++changed->generation;break;case 1:++changed->scopeId;break;case 2:++changed->block;break;case 3:++changed->blockRecord;break;
            case 4:++changed->handle;break;case 5:++changed->owner;break;case 6:++changed->dispatch;break;case 7:++changed->sourceEvent;break;
            case 8:++changed->typedEvent;break;case 9:++changed->physical;break;case 10:++changed->index;break;case 11:++changed->begin.ordinal;break;
            case 12:++changed->end.beginOffset;break;case 13:changed->sourceEvent=(std::numeric_limits<std::size_t>::max)();break;
            case 14:changed->delivery=std::make_shared<const DRW_DxfBlockDeliverySource>(*saved->delivery);break;
            case 15:changed->children=std::make_shared<const DRW_DxfBlockChildrenSource>(*saved->children);break;
            case 16:changed->model=std::make_shared<const DRW_Attdef>(*saved->model);break;
            case 17:changed->raw=std::make_shared<const DRW_RawDxfObject>(*saved->raw);break;
            case 18:changed->original=std::make_shared<const DRW_Block>(*saved->original);break;
            case 19:changed->effective=std::make_shared<const DRW_Block>(*saved->effective);break;
            case 20:changed->footer=std::make_shared<const DRW_Block>(*saved->footer);break;
            case 21:changed->blockBegin=std::make_shared<const DRW_RawDxfObject>(*saved->blockBegin);break;
            case 22:changed->blockEnd=std::make_shared<const DRW_RawDxfObject>(*saved->blockEnd);break;
            case 23:changed->model.reset();break;case 24:changed->physical=(std::numeric_limits<std::size_t>::max)();break;
            case 25:changed->dispatch=(std::numeric_limits<std::size_t>::max)();break;
            case 26:changed->membership=std::make_shared<const dx_block_membership::Audit>(*saved->membership);break;
            }
            A::mutableOriginResource(baseline).attdefOrigin=changed;
            if(!check(!A::originMatched(reader,baseline),"corrupted/stale/replaced origin tuple cannot borrow cached proof or a matching handle"))return false;
            A::mutableOriginResource(baseline).attdefOrigin=saved;
            if(!A::originMatched(reader,baseline))return false;++mutations;
        }
        for(unsigned dimension=0u;dimension<3u;++dimension) {
            AttdefOriginProbe bounded;dx_data exact;A::originLimit(exact,dimension,usage[dimension]);
            if(!check(bounded.fileImport(path.string(),&exact,false)&&A::originMatched(bounded,exact),"exact origin descriptor/index/work ceiling"))return false;
            dx_data below;A::originLimit(below,dimension,usage[dimension]-1u);
            if(!check(bounded.fileImport(path.string(),&below,false)&&A::originFailed(below)&&!A::originMatched(bounded,below),
                "one-under origin ceiling refuses evidence without changing ordinary import"))return false;
            dx_data retry;if(!bounded.fileImport(path.string(),&retry,false)||!A::originMatched(bounded,retry))return false;++quotas;
        }
        bool calibrated=false;
        for(long fault=0;fault<20&&!calibrated;++fault) {
            std::weak_ptr<const A::Origin> weak;
            {
                AttdefOriginProbe isolated;dx_data data;if(!isolated.fileImport(path.string(),&data,false)||!A::originMatched(isolated,data))return false;
                long used=0;const bool captured=A::recaptureOrigin(isolated,data,fault,used);weak=A::originResource(data)->attdefOrigin;
                if(captured){if(used!=fault||A::originMatched(isolated,data))return false;calibrated=true;}
                else {if(!A::originFailed(data))return false;++allocations;}
                dx_data corrected;if(!isolated.fileImport(path.string(),&corrected,false)||!A::originMatched(isolated,corrected))return false;
            }
            if(!weak.expired())return false;
        }
        if(!calibrated)return false;
        for(unsigned context=0u;context<4u;++context) {
            const auto doc=context==0u?BlockFixture{document("AC1032",body(0u),false),{},{} }:
                originFixture("AC1032",0u,context!=3u);
            if(!write(path,doc.document,binary))return false;
            AttdefOriginProbe unbound;if(context==1u)unbound.origins=false;if(context==2u)unbound.selected=false;
            dx_data data;if(!check(unbound.fileImport(path.string(),&data,false)&&!A::originMatched(unbound,data)
                &&!A::originFailed(data),"ENTITIES/opt-out/missing endpoint facts cannot invent enclosing source qualification"))return false;
            const auto* row=A::originResource(data);if(!row||row->rawTypedMatched)return false;
            if(row->attdefOriginState!=(context==0u?State::Standalone:context==3u?State::PairBound:State::Unavailable))return false;
            ++contexts;
        }
        if(!write(path,f.document,binary))return false;
        AttdefOriginProbe next;dx_data newer;if(!next.fileImport(path.string(),&newer,false)||!A::originMatched(next,newer))return false;
        const auto current=A::originResource(newer)->attdefOrigin;
        A::mutableOriginResource(newer).attdefOrigin=saved;
        if(!check(!A::originMatched(next,newer),"old identical-input origin cannot bind a new resource/physical/dispatch owner tuple"))return false;
        A::mutableOriginResource(newer).attdefOrigin=current;if(!A::originMatched(next,newer))return false;++replays;
        auto later=f;later.document.insert(later.document.end()-1,{{0,"SECTION"},{2,"ENTITIES"},{0,"POINT"},{5,"F1"},{10,"0"},{0,"ENDSEC"}});
        if(!write(path,later.document,binary))return false;
        AttdefOriginProbe failed;dx_data bad;
        if(!check(!failed.fileImport(path.string(),&bad,false)&&A::originResource(bad)
            &&A::originResource(bad)->attdefOriginState==State::EnclosingMatched&&!A::originMatched(failed,bad),
            "preliminary enclosing match cannot survive later whole-import failure"))return false;
        ++contexts;
    }
    std::cout<<"ATTDEF original-origin profiles="<<profiles<<"; enclosing matched="<<matched<<"; profile-pending="<<pending
        <<"; original scalar spans="<<spans<<"; tuple corruption/restoration="<<mutations<<"; exact/under/retries="<<quotas
        <<"; isolated allocation faults/retries="<<allocations<<"; context boundaries="<<contexts<<"; stale tuple refusals="<<replays
        <<"; mixed-stream member ordering/restoration="<<mixed<<'\n';
    return profiles==36u&&matched==18u&&pending==18u&&spans==1968u&&mutations==54u&&quotas==6u&&allocations==4
        &&contexts==10u&&replays==2u&&mixed==2u;
}
