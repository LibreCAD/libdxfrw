// SPDX-License-Identifier: GPL-2.0-or-later
std::string compoundHexBytes(std::size_t n) {
    constexpr char digits[]="0123456789ABCDEF";std::string out;out.reserve(n*2u);
    for(std::size_t i=0;i<n;++i){const auto byte=static_cast<unsigned>((i*37u)%256u);out+=digits[byte>>4u];out+=digits[byte&15u];}
    return out;
}
void replaceCompoundRecord(Tags& tags,const char* id,const char* name,Tags fields) {
    const auto first=membershipSlot(tags,id,5)-1u;auto end=first+1u;while(tags[end].code!=0)++end;
    fields.insert(fields.begin(),{0,name});tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(first),tags.begin()+static_cast<std::ptrdiff_t>(end));
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(first),fields.begin(),fields.end());
}
BlockFixture compoundFixture(const std::string& version,unsigned form) {
    auto f=mixedBlockChildrenFixture(version);
    if(form==1u||form==2u||form==3u) {
        const int flags=form==1u?8:form==2u?16:64;
        replaceCompoundRecord(f.document,"D1","POLYLINE",{{5,"D1"},{330,"B0"},{100,"AcDbEntity"},{8,"PL"},
            {100,form==1u?"AcDb3dPolyline":form==2u?"AcDbPolygonMesh":"AcDbPolyFaceMesh"},
            {70,std::to_string(flags)},{10,"0"},{20,"0"},{30,"2.5"},{40,"0.25"},{41,"0.5"},
            {71,form==3u?"3":"2"},{72,form==3u?"1":"2"},{73,"4"},{74,"5"},{75,"6"}});
        const auto vertex=[&](const char* id,const char* x,const char* y) {
            return Tags{{0,"VERTEX"},{5,id},{330,"D1"},{100,"AcDbEntity"},{8,"VL"},{100,"AcDbVertex"},
                {100,form==1u?"AcDb3dPolylineVertex":form==2u?"AcDbPolygonMeshVertex":"AcDbPolyFaceMeshVertex"},
                {10,x},{20,y},{30,"3.75"},{70,form==1u?"32":form==2u?"64":"192"},{91,"9"}};
        };
        auto a=vertex("D2","-0.0","1.25"),b=vertex("D3","2.5","3.75");a.erase(a.begin());b.erase(b.begin());
        replaceCompoundRecord(f.document,"D2","VERTEX",a);replaceCompoundRecord(f.document,"D3","VERTEX",b);
        if(form!=1u) {
            auto extra=vertex("D5","4.5","5.5");
            const auto last=form==2u?vertex("D6","6.5","7.5"):
                Tags{{0,"VERTEX"},{5,"D6"},{330,"D1"},{100,"AcDbEntity"},{8,"FACE"},{100,"AcDbFaceRecord"},
                    {20,"7"},{70,"128"},{71,"1"},{72,"-2"},{73,"3"},{74,"0"}};
            extra.insert(extra.end(),last.begin(),last.end());
            const auto at=membershipSlot(f.document,"D4",5)-1u;f.document.insert(f.document.begin()+static_cast<std::ptrdiff_t>(at),extra.begin(),extra.end());
        }
    }
    if(form==4u||form==5u) {
        auto attribute=body(form==4u?1u:2u);attribute[0].value="E2";attribute[1].value="E1";
        for(auto& tag:attribute)if(tag.code==100&&tag.value=="AcDbAttributeDefinition")tag.value="AcDbAttribute";
        attribute.erase(std::remove_if(attribute.begin(),attribute.end(),[](const Tag& t){return t.code==3&&t.value=="prompt-é";}),attribute.end());
        replaceCompoundRecord(f.document,"E2","ATTRIB",attribute);
    }
    if(form==6u) {
        f.document.insert(f.document.begin()+4,{{9,"$DWGCODEPAGE"},{3,"ANSI_932"}});
        for(const char* id:{"D1","D2","D3","D4","E1","E2","E3"}) {
            const auto at=membershipSlot(f.document,id,100)+1u;
            const Tags common{{6,"DASHED"},{62,"3"},{48,"0.75"},{370,"35"},{420,"1193046"},{430,"color-é"},
                {440,"33554559"},{60,"1"},{67,"1"},{347,"C9"},{348,"CA"},{390,"CB"},{284,"2"},
                {102,"{ACAD_REACTORS"},{330,"F8"},{102,"}"},{102,"{ACAD_XDICTIONARY"},{360,"F9"},{102,"}"},
                {version=="AC1021"?92:160,"128"},{310,compoundHexBytes(127u)},{310,"AA"}};
            f.document.insert(f.document.begin()+static_cast<std::ptrdiff_t>(at),common.begin(),common.end());
            auto end=membershipSlot(f.document,id,5);while(f.document[end].code!=0)++end;
            const Tags xdata{{1001,"BODY_APP"},{1003,"BODY_LAYER"},{1005,"C9"},{1004,compoundHexBytes(127u)},{1004,"AA"},
                {1010,"-0.0"},{1020,"2.25"},{1030,"-3.5"},{1070,"17"},{1071,"2147483647"}};
            f.document.insert(f.document.begin()+static_cast<std::ptrdiff_t>(end),xdata.begin(),xdata.end());
        }
    }
    return f;
}

class CompoundBodyProbe final:public BlockSourceProbe {
public:
    template<class T> struct Extended final:T {explicit Extended(const T& v):T(v){}int extra=17;};
    int fault=-1,subclass=-1,throwBody=-1;bool premature=false;
    void addPolyline(const DRW_Polyline& value) override {
        if(fault>=0&&fault<12) {
            auto x=value;
            switch(fault) {
            case 0:x.basePoint.z+=1.0;break;case 1:x.flags^=1;break;case 2:x.defstawidth+=0.25;break;
            case 3:x.vertlist[0]->basePoint.x+=1.0;break;case 4:x.vertlist[0]->bulge+=0.5;break;
            case 5:++x.vertlist[0]->identifier;break;case 6:x.vertlist[0]->flags^=1;break;
            case 7:x.vertlist[0]->setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex2D);break;
            case 8:x.seqEndEntity->layer="changed-footer";break;case 9:x.seqEndEntity->hasDataStoragePayloadMarker=true;break;
            case 10:x.setDwgSeqEndHandle(0xF0u);break;case 11:x.reactorHandles.push_back(0xF1u);break;
            }
            BlockSourceProbe::addPolyline(x);
        }else if(subclass==0) {auto x=value;x.vertlist[0]=std::make_shared<Extended<DRW_Vertex>>(*x.vertlist[0]);BlockSourceProbe::addPolyline(x);}
        else if(subclass==1) {auto x=value;x.seqEndEntity=std::make_shared<Extended<DRW_SeqEnd>>(*x.seqEndEntity);BlockSourceProbe::addPolyline(x);}
        else if(subclass==5) {Extended<DRW_Polyline> x(value);BlockSourceProbe::addPolyline(x);}
        else BlockSourceProbe::addPolyline(value);
        if(DxIfaceTableSourceTestAccess::compoundMatched(*cData,2u))premature=true;
        if(throwBody==0)throw std::runtime_error("compound polyline replay callback fault");
    }
    void addInsert(const DRW_Insert& value) override {
        if(fault>=12) {
            auto x=value;auto& a=*x.attlist[0];
            switch(fault) {
            case 12:x.xscale+=1;break;case 13:x.yscale+=1;break;case 14:x.zscale+=1;break;case 15:x.angle+=0.5;break;
            case 16:++x.colcount;break;case 17:++x.rowcount;break;case 18:x.colspace+=1;break;case 19:x.rowspace+=1;break;
            case 20:x.name="changed-target";break;case 21:a.text="changed-text";break;case 22:a.tag="CHANGED_TAG";break;
            case 23:a.attribFlags^=1;break;case 24:a.style="changed-style";break;case 25:++a.m_fieldLength;break;
            case 26:a.lockPosition=!a.lockPosition;break;case 27:a.mtext->text="changed-embedded";break;
            case 28:a.mtext->m_backgroundScale+=0.5;break;case 29:a.mtext->m_r2018ColumnHeights.push_back(1.25);break;
            case 30:a.m_r2018OuterAnnotativeData.push_back(1u);break;case 31:++x.seqEndEntity->plotStyle;break;
            case 32:a.mtext->appData.front().push_back(DRW_Variant(1,UTF8STRING("extra-application")));break;
            case 33:++a.mtext->fullVisualStyleHandle;break;case 34:++x.blockRecH.ref;break;
            case 35:x.attribHandles.push_back(dwgHandle{});break;
            case 36:a.extData.push_back(std::make_shared<DRW_Variant>(1000,UTF8STRING("extra-xdata")));break;
            case 37:a.mtext.reset();break;case 38:x.proxyGraphics.push_back(1u);break;case 39:x.dataStorageData.push_back(1u);break;
            }
            BlockSourceProbe::addInsert(x);
        }else if(subclass==2) {auto x=value;x.attlist[0]=std::make_shared<Extended<DRW_Attrib>>(*x.attlist[0]);BlockSourceProbe::addInsert(x);}
        else if(subclass==3) {auto x=value;x.seqEndEntity=std::make_shared<Extended<DRW_SeqEnd>>(*x.seqEndEntity);BlockSourceProbe::addInsert(x);}
        else if(subclass==4) {auto x=value;x.attlist[0]->mtext=std::make_unique<Extended<DRW_MText>>(*x.attlist[0]->mtext);BlockSourceProbe::addInsert(x);}
        else if(subclass==6) {Extended<DRW_Insert> x(value);BlockSourceProbe::addInsert(x);}
        else BlockSourceProbe::addInsert(value);
        if(DxIfaceTableSourceTestAccess::compoundMatched(*cData,3u))premature=true;
        if(throwBody==1)throw std::runtime_error("compound insert replay callback fault");
    }
    void endBlock() override {
        BlockSourceProbe::endBlock();if(DxIfaceTableSourceTestAccess::compoundMatched(*cData,2u))premature=true;
    }
};

bool compoundBodyTests(const std::filesystem::path& path) {
    using A=DxIfaceTableSourceTestAccess;using State=A::CompoundBody::State;
    unsigned profiles=0u,mutations=0u,quotas=0u,aliases=0u,variants=0u,pending=0u,opaque=0u,contexts=0u,stale=0u,subclasses=0u,exceptions=0u;
    long allocations=0;std::size_t spans=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0;encoding<3u;++encoding)for(unsigned form=0;form<7u;++form) {
        const auto f=compoundFixture(version,form);if(!write(path,f.document,encoding==2u,encoding==1u))return false;
        CompoundBodyProbe reader;dx_data data;
        if(!check(reader.fileImport(path.string(),&data,false)&&!reader.premature&&!A::compoundFailed(data)&&A::compoundMatched(data,2u)&&A::compoundMatched(data,3u)
            &&originalChildrenWire(path,encoding==2u,f,reader.childrenAlias,spans),"actual frozen compound parent/children replay exact original modeled fields")) {
            std::cerr<<version<<" encoding="<<encoding<<" form="<<form<<" capture="<<A::compoundFailed(data);
            for(auto dispatch:{2u,3u})if(const auto* row=A::compoundBody(data,dispatch))std::cerr<<" dispatch "<<dispatch<<" state="<<static_cast<int>(row->state);
            std::cerr<<'\n';return false;
        }
        const auto* attribute=A::compoundBody(data,3u);
        if(!check(attribute&&!attribute->sourceStates[1].attributeSubclass&&attribute->replayStates[1].attributeSubclass,
            "actual delivered and original replay attribute parser phases stay distinct"))return false;
        ++profiles;
    }
    std::cout<<"Compound body positive profiles="<<profiles<<"; original scalar spans="<<spans<<std::endl;
    for(bool binary:{false,true}) {
        const auto f=compoundFixture("AC1032",4u);if(!write(path,f.document,binary))return false;
        for(int fault=0;fault<40;++fault) {
            CompoundBodyProbe reader;reader.fault=fault;dx_data data;
            const auto rejected=fault<12?2u:3u,unaffected=fault<12?3u:2u;
            if(!check(reader.fileImport(path.string(),&data,false)&&A::membershipMatched(data)&&!A::compoundFailed(data)
                &&!A::compoundMatched(data,rejected)&&A::compoundMatched(data,unaffected),"same-ID field/native/common/nested edits fail modeled proof while original structure and unrelated body stay distinct")) {
                std::cerr<<"compound fault="<<fault<<'\n';return false;
            }
            const auto* body=A::compoundBody(data,rejected);
            if(!check(body&&(body->state==State::ModelMismatch||body->state==State::SourceStateMismatch),"modeled body rejection records its distinct reason")) {
                std::cerr<<"compound fault="<<fault<<" state="<<(body?static_cast<int>(body->state):-1)<<'\n';return false;
            }
            reader.fault=-1;dx_data retry;if(!check(reader.fileImport(path.string(),&retry,false)&&A::compoundMatched(retry,rejected),"same facade corrected field retry qualifies"))return false;
            ++mutations;
        }
        for(int subtype=0;subtype<7;++subtype) {
            CompoundBodyProbe changed;changed.subclass=subtype;dx_data data;
            if(!check(changed.fileImport(path.string(),&data,false)&&A::membershipMatched(data)&&A::compoundFailed(data)
                &&!A::compoundMatched(data,2u)&&!A::compoundMatched(data,3u),"extended concrete models refuse evidence instead of silently slicing"))return false;
            changed.subclass=-1;dx_data retry;if(!changed.fileImport(path.string(),&retry,false)||!A::compoundMatched(retry,3u))return false;++subclasses;
        }
        for(int phase=0;phase<2;++phase) {
            CompoundBodyProbe throwing;throwing.throwBody=phase;dx_data data;bool threw=false;
            try {(void)throwing.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
            if(!check(threw&&throwing.depth==0u&&!A::compoundMatched(data,2u)&&!A::compoundMatched(data,3u)
                &&A::compoundBody(data,2u)&&A::compoundBody(data,2u)->state==State::ModeledReplayMatched,"later callback exceptions close scopes and revoke whole-import facts"))return false;
            throwing.throwBody=-1;dx_data retry;if(!throwing.fileImport(path.string(),&retry,false)||!A::compoundMatched(retry,3u))return false;++exceptions;
        }
        CompoundBodyProbe reader;dx_data baseline;if(!reader.fileImport(path.string(),&baseline,false))return false;
        const auto usage=A::compoundUsage(baseline);const auto old=A::block(baseline)->compoundBodies.at(2u);
        for(auto* block:baseline.blocks)for(auto* entity:block->ent) {
            if(auto* pl=dynamic_cast<DRW_Polyline*>(entity))pl->vertlist[0]->basePoint.x+=7.0;
            if(auto* in=dynamic_cast<DRW_Insert*>(entity))in->attlist[0]->mtext->text="live edit";
        }
        if(!A::compoundMatched(baseline,2u)||!A::compoundMatched(baseline,3u))return false;
        const auto* frozen=dynamic_cast<const DRW_Polyline*>(old->model.get());
        if(!check(frozen&&frozen->vertlist[0]->basePoint.x==0.0,"public live edits cannot mutate immutable original body"))return false;++aliases;
        for(unsigned dimension=0;dimension<3u;++dimension) {
            dx_data exact;A::compoundLimit(exact,dimension,usage[dimension]);
            if(!check(reader.fileImport(path.string(),&exact,false)&&A::compoundMatched(exact,2u)&&A::compoundMatched(exact,3u),"exact body allowance admits both selected compounds")) {
                std::cerr<<"compound budget dimension="<<dimension<<" allowance="<<usage[dimension]<<" failed="<<A::compoundFailed(exact)
                    <<" observed="<<A::compoundUsage(exact)[dimension];
                for(auto dispatch:{2u,3u})if(const auto* row=A::compoundBody(exact,dispatch))std::cerr<<" dispatch "<<dispatch<<" state="<<static_cast<int>(row->state);
                std::cerr<<'\n';return false;
            }
            dx_data below;A::compoundLimit(below,dimension,usage[dimension]-1u);
            if(!check(reader.fileImport(path.string(),&below,false)&&A::compoundFailed(below)&&!A::compoundMatched(below,2u)&&!A::compoundMatched(below,3u),"one-under body allowance refuses evidence without breaking compatible import"))return false;
            dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::compoundMatched(retry,3u))return false;++quotas;
        }
        std::cout<<"Compound body encoding="<<binary<<" field faults="<<mutations<<" quotas="<<quotas<<std::endl;
        for(const auto dispatch:{2u,3u}) {
        bool calibrated=false;
        for(long fault=0;fault<600&&!calibrated;++fault) {
            std::weak_ptr<const A::CompoundBody> weak;
            {
                CompoundBodyProbe isolated;dx_data data;if(!isolated.fileImport(path.string(),&data,false)||!A::compoundMatched(data,2u))return false;
                long used=0;const bool captured=A::recaptureCompound(isolated,data,dispatch,fault,used);
                const auto at=A::block(data)->compoundBodies.find(dispatch);if(at!=A::block(data)->compoundBodies.end())weak=at->second;
                if(captured){if(used!=fault||A::compoundMatched(data,dispatch))return false;calibrated=true;}
                else {if(!A::compoundFailed(data))return false;++allocations;}
                dx_data retry;if(!isolated.fileImport(path.string(),&retry,false)||!A::compoundMatched(retry,2u))return false;
            }
            if(!weak.expired())return false;
        }
        if(!check(calibrated,"compound body allocation fault sweep reaches a successful capture"))return false;
        std::cout<<"Compound body encoding="<<binary<<" dispatch="<<dispatch<<" cumulative allocation faults="<<allocations<<std::endl;
        }
        dx_data newer;if(!reader.fileImport(path.string(),&newer,false)||!A::compoundMatched(newer,2u))return false;
        const auto current=A::block(newer)->compoundBodies.at(2u);A::mutableOriginBlock(newer).compoundBodies[2u]=old;
        if(A::compoundMatched(newer,2u))return false;A::mutableOriginBlock(newer).compoundBodies[2u]=current;
        if(!A::compoundMatched(newer,2u))return false;++stale;
        dx_data borrowed;A::copyMembershipContext(borrowed,newer);if(A::compoundMatched(borrowed,2u))return false;++contexts;
        for(unsigned mode=0;mode<3u;++mode) {
            auto varied=f;
            if(mode!=2u)eraseMembershipRecord(varied.document,"E2");
            if(mode==0u) {
                eraseMembershipRecord(varied.document,"E3");const auto at=membershipSlot(varied.document,"E1",66);
                varied.document.erase(varied.document.begin()+static_cast<std::ptrdiff_t>(at));
            }
            if(mode==2u) {
                varied.document[membershipSlot(varied.document,"E1",5)-1u].value="MINSERT";
                auto end=membershipSlot(varied.document,"E1",5);while(varied.document[end].code!=0)++end;
                varied.document.insert(varied.document.begin()+static_cast<std::ptrdiff_t>(end),{{70,"3"},{71,"2"},{44,"4"},{45,"5"},{41,"1.5"},{42,"2.5"},{43,"3.5"}});
            }
            if(!write(path,varied.document,binary))return false;
            dx_data data;if(!reader.fileImport(path.string(),&data,false)||!A::compoundMatched(data,3u))return false;++variants;
        }
        for(const char* id:{"D2","E2"}) {
            auto broken=f;auto end=membershipSlot(broken.document,id,5);while(broken.document[end].code!=0)++end;
            broken.document.insert(broken.document.begin()+static_cast<std::ptrdiff_t>(end),{{1010,"5"}});
            if(!write(path,broken.document,binary))return false;
            dx_data data;const auto dispatch=std::string(id)=="D2"?2u:3u;
            if(!reader.fileImport(path.string(),&data,false)||!A::membershipMatched(data)||A::compoundMatched(data,dispatch)
                ||!A::compoundBody(data,dispatch)||A::compoundBody(data,dispatch)->state!=State::SourceStateMismatch)return false;
            ++pending;
        }
        auto unknown=f;auto end=membershipSlot(unknown.document,"D2",5);while(unknown.document[end].code!=0)++end;
        unknown.document.insert(unknown.document.begin()+static_cast<std::ptrdiff_t>(end),{{90,"123"}});
        if(!write(path,unknown.document,binary))return false;
        dx_data unmodeled;if(!reader.fileImport(path.string(),&unmodeled,false)||!A::compoundMatched(unmodeled,2u)
            ||A::originResource(unmodeled)->rawTypedMatched)return false;
        ++opaque;
        auto later=f;later.document.insert(later.document.end()-1,{{0,"SECTION"},{2,"ENTITIES"},{0,"POINT"},{5,"F1"},{10,"0"},{0,"ENDSEC"}});
        if(!write(path,later.document,binary))return false;
        dx_data failed;if(reader.fileImport(path.string(),&failed,false)||A::compoundMatched(failed,2u)
            ||!A::compoundBody(failed,2u)||A::compoundBody(failed,2u)->state!=State::ModeledReplayMatched)return false;
        ++contexts;
        if(!write(path,f.document,binary))return false;
        for(unsigned mode=0;mode<2u;++mode) {
            BlockSourceProbe noSource;noSource.selected=mode!=0u;noSource.childrenSelected=mode==0u;
            dx_data data;if(!noSource.fileImport(path.string(),&data,false)||A::compoundMatched(data,2u)||A::compoundBody(data,2u))return false;++contexts;
        }
    }
    std::cout<<"Compound body profiles="<<profiles<<"; original scalar spans="<<spans<<"; same-ID field faults/retries="<<mutations
        <<"; immutable aliases="<<aliases<<"; quota exact/under/retries="<<quotas<<"; isolated allocation faults/retries="<<allocations
        <<"; stale owner/restorations="<<stale<<"; INSERT variants="<<variants<<"; unfinished XDATA="<<pending
        <<"; modeled-versus-profile boundaries="<<opaque<<"; copied/later/opt-out contexts="<<contexts
        <<"; extended-class refusals/retries="<<subclasses<<"; callback exceptions/retries="<<exceptions<<'\n';
    return profiles==84u&&spans==19524u&&mutations==80u&&aliases==2u&&quotas==6u&&allocations>0&&stale==2u
        &&variants==6u&&pending==4u&&opaque==2u&&contexts==8u&&subclasses==14u&&exceptions==4u;
}
