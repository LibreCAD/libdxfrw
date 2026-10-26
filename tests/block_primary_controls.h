// SPDX-License-Identifier: GPL-2.0-or-later
// Original delivered models plus original-wire shared replay and scalar spans;
// not complete block/space/dependency/live/output/native semantic evidence.
void blockPrimaryAppend(Tags& tags,const char* handle,const Tags& fields) {
    auto at=membershipSlot(tags,handle,5);while(tags[at].code!=0)++at;
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),fields.begin(),fields.end());
}
BlockFixture blockPrimaryFixture(const std::string& version,bool rich) {
    auto f=blockFixture(version,0u,true);f.document.insert(f.document.begin()+4,{{9,"$DWGCODEPAGE"},{3,"ANSI_932"}});
    Tags children{{0,"POINT"},{5,"C1"},{330,"B0"},{100,"AcDbEntity"},{8,"POINT_LAYER"},{100,"AcDbPoint"},{10,"-0.0"},{20,"1.25"},
        {0,"LINE"},{5,"C2"},{330,"B0"},{100,"AcDbEntity"},{8,"LINE_LAYER"},{100,"AcDbLine"},{10,"1"},{20,"2"},{11,"4"},{21,"5"}};
    const auto end=std::find_if(f.document.begin(),f.document.end(),[](const Tag& t){return t.code==0&&t.value=="ENDBLK";});
    f.document.insert(end,children.begin(),children.end());
    if(rich)for(const char* id:{"C1","C2"}) {
        const auto at=membershipSlot(f.document,id,100)+1u;
        f.document.insert(f.document.begin()+static_cast<std::ptrdiff_t>(at),{{6,"DASHED"},{62,"3"},{48,"0.75"},{370,"35"},
            {420,"1193046"},{430,"color-é"},{440,"33554559"},{347,"F1"},{348,"F2"},{390,"F3"},
            {102,"{ACAD_REACTORS"},{330,"F8"},{102,"}"},{102,"{ACAD_XDICTIONARY"},{360,"F9"},{102,"}"}});
        blockPrimaryAppend(f.document,id,{{30,"3.5"},{39,"2.5"},{210,"0.25"},{220,"0.5"},{230,"0.75"},
            {1001,"PRIMARY_APP"},{1000,"common ^J text-é"},{1003,"PRIMARY_LAYER"},{1005,"F4"},{1004,"A5FF"},
            {1010,"-0.0"},{1020,"2.25"},{1030,"-3.5"},{1070,"17"},{1071,"2147483647"}});
        if(std::string(id)=="C1") {
            const auto p=membershipSlot(f.document,id,1001);f.document.insert(f.document.begin()+static_cast<std::ptrdiff_t>(p),{50,"90"});
        } else {
            const auto p=membershipSlot(f.document,id,1001);f.document.insert(f.document.begin()+static_cast<std::ptrdiff_t>(p),{31,"6"});
        }
    }
    return f;
}
struct BlockPrimaryProbe final:BlockSourceProbe {
    int field=-1,subclass=-1;bool clearHeader=false,throwPoint=false,throwLine=false,premature=false;
    void addHeader(const DRW_Header* header) override {
        BlockSourceProbe::addHeader(header);if(clearHeader)cData->headerC.clearDxfCodePageDeclaration();
    }
    template<class T>struct Extended:T {explicit Extended(const T& value):T(value){}int extra=7;};
    void addPoint(const DRW_Point& value) override {
        auto& v=const_cast<DRW_Point&>(value);double* changed=nullptr;
        switch(field){case 0:changed=&v.basePoint.x;break;case 1:changed=&v.basePoint.y;break;case 2:changed=&v.basePoint.z;break;
        case 3:changed=&v.thickness;break;case 4:changed=&v.xAxisAngle;break;case 5:changed=&v.extPoint.x;break;case 6:changed=&v.extPoint.y;break;case 7:changed=&v.extPoint.z;break;}
        const double before=changed?*changed:0.;if(changed)*changed+=0.125;
        const auto original=v.fullVisualStyleHandle;if(field==18)++v.fullVisualStyleHandle;
        try{if(subclass==0){Extended<DRW_Point> other(value);dx_iface::addPoint(other);}else dx_iface::addPoint(value);}
        catch(...){if(changed)*changed=before;v.fullVisualStyleHandle=original;throw;}
        if(changed)*changed=before;v.fullVisualStyleHandle=original;
        if(DxIfaceTableSourceTestAccess::compoundMatched(*cData,0u))premature=true;
        if(throwPoint)throw std::runtime_error("BLOCK POINT callback");
    }
    void addLine(const DRW_Line& value) override {
        auto& v=const_cast<DRW_Line&>(value);double* changed=nullptr;
        switch(field){case 8:changed=&v.basePoint.x;break;case 9:changed=&v.basePoint.y;break;case 10:changed=&v.basePoint.z;break;
        case 11:changed=&v.secPoint.x;break;case 12:changed=&v.secPoint.y;break;case 13:changed=&v.secPoint.z;break;case 14:changed=&v.thickness;break;
        case 15:changed=&v.extPoint.x;break;case 16:changed=&v.extPoint.y;break;case 17:changed=&v.extPoint.z;break;}
        const double before=changed?*changed:0.;if(changed)*changed+=0.125;
        const auto original=v.fullVisualStyleHandle;if(field==19)++v.fullVisualStyleHandle;
        try{if(subclass==1){Extended<DRW_Line> other(value);dx_iface::addLine(other);}else dx_iface::addLine(value);}
        catch(...){if(changed)*changed=before;v.fullVisualStyleHandle=original;throw;}
        if(changed)*changed=before;v.fullVisualStyleHandle=original;
        if(DxIfaceTableSourceTestAccess::compoundMatched(*cData,1u))premature=true;
        if(throwLine)throw std::runtime_error("BLOCK LINE callback");
    }
};
bool blockPrimaryTests(const std::filesystem::path& path) {
    struct Stage{const char* name="profiles";bool done=false;~Stage(){if(!done)std::cerr<<"BLOCK primary stage="<<name<<'\n';}} stage;
    using A=DxIfaceTableSourceTestAccess;using S=A::CompoundBody::State;unsigned profiles=0u,mutations=0u,refusals=0u,variants=0u,contexts=0u,quotas=0u,exceptions=0u;long allocations=0;std::size_t spans=0u;
    const auto complete=[](const dx_data& d){return A::membershipMatched(d)&&A::compoundMatched(d,0u)&&A::compoundMatched(d,1u)
        &&A::compoundSourceMatched(d,0u)&&A::compoundSourceMatched(d,1u)&&!A::compoundFailed(d);};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool rich:{false,true}) {
        const auto f=blockPrimaryFixture(version,rich);if(!write(path,f.document,encoding==2u,encoding==1u))return false;BlockPrimaryProbe reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||!complete(data)||reader.premature||!originalChildrenWire(path,encoding==2u,f,reader.childrenAlias,spans))return false;
        const auto expectedVersion=version=="AC1021"?DRW::AC1021:version=="AC1024"?DRW::AC1024:version=="AC1027"?DRW::AC1027:DRW::AC1032;
        const auto* block=A::block(data);if(!block->contextMatched||!block->context||block->context!=block->contextOwner||block->context->declaration!="ANSI_932"
            ||block->context->sourceVersion!=expectedVersion||block->context->binary!=(encoding==2u))return false;
        for(std::size_t d=0;d<2u;++d) {
            const auto* body=A::compoundBody(data,d);const auto& physical=block->children->records[d];
            if(!body||body->context!=block->context||body->profiles.size()!=1u||body->profiles[0].spans.size()!=physical.groups.size())return false;
            for(std::size_t i=0;i<physical.groups.size();++i)if(body->profiles[0].spans[i].groupIndex!=i||body->profiles[0].spans[i].code!=physical.groups[i].code())return false;
        }
        const auto* point=dynamic_cast<const DRW_Point*>(A::compoundBody(data,0u)->model.get());const auto* line=dynamic_cast<const DRW_Line*>(A::compoundBody(data,1u)->model.get());
        if(!point||!line||!std::signbit(point->basePoint.x)||point->basePoint.y!=1.25||line->secPoint.x!=4.||line->secPoint.y!=5.
            ||(rich&&(point->xAxisAngle!=90./ARAD||line->secPoint.z!=6.)))return false;
        ++profiles;
    }
    for(bool binary:{false,true}) {
        const auto f=blockPrimaryFixture("AC1032",true);if(!write(path,f.document,binary))return false;
        stage.name="field and concrete-class faults";
        for(int field=0;field<20;++field) {BlockPrimaryProbe reader;reader.field=field;dx_data data;const auto rejected=field<8||field==18?0u:1u;
            if(!reader.fileImport(path.string(),&data,false)||!A::membershipMatched(data)||A::compoundFailed(data)||A::compoundMatched(data,rejected)
                ||!A::compoundMatched(data,1u-rejected)||A::compoundBody(data,rejected)->state!=S::ModelMismatch)return false;
            reader.field=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++mutations;}
        for(int kind=0;kind<2;++kind){BlockPrimaryProbe reader;reader.subclass=kind;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!A::compoundFailed(data)||A::compoundMatched(data,0u))return false;
            reader.subclass=-1;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++refusals;}
        stage.name="original pending/profile/layout controls";
        for(unsigned kind=0;kind<6u;++kind) {
            auto varied=blockPrimaryFixture("AC1032",false);const auto dispatch=kind&1u;const char* id=dispatch?"C2":"C1";
            if(kind<2u)blockPrimaryAppend(varied.document,id,{{1001,"PENDING"},{1010,"1"}});
            if(kind==2u||kind==3u)blockPrimaryAppend(varied.document,id,{{90,"17"}});
            if(kind==4u||kind==5u)blockPrimaryAppend(varied.document,id,{{410,"UNRESOLVED-LAYOUT"}});
            if(!write(path,varied.document,binary))return false;BlockPrimaryProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!A::membershipMatched(data))return false;
            const auto* body=A::compoundBody(data,dispatch);if(!body||A::compoundSourceMatched(data,dispatch))return false;
            if(kind<2u&&(body->state!=S::SourceStateMismatch||!body->sourceStates[0].hasNoPendingXdata()||body->replayStates[0].hasNoPendingXdata()))return false;
            if(kind>=2u&&!A::compoundMatched(data,dispatch))return false;
            if(kind<4u&&kind>=2u&&body->profilesComplete)return false;
            if(kind>=4u&&(!body->profilesComplete||body->profiles[0].reason!=dx_attdef_source::Reason::Complete))return false;
            if(!write(path,f.document,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++refusals;
        }
        stage.name="frozen contexts, owner substitutions and live separation";
        {BlockPrimaryProbe reader;reader.clearHeader=true;dx_data data;if(!reader.fileImport(path.string(),&data,false,DRW::DxfCodePagePolicy::ExactNamed)||!complete(data)
            ||data.headerC.hasDxfCodePageDeclaration()||!A::block(data)->context->hasDeclaration||A::block(data)->context->codePagePolicy!=DRW::DxfCodePagePolicy::ExactNamed){std::cerr<<"cleared header context control failed\n";return false;}
            ++contexts;}
        stage.name="coordinate-order and opaque variants";
        for(unsigned kind=0u;kind<2u;++kind){auto varied=f;
            if(!kind){const auto at=membershipSlot(varied.document,"C1",10);const auto value=varied.document[at];varied.document.erase(varied.document.begin()+static_cast<std::ptrdiff_t>(at));
                varied.document.insert(varied.document.begin()+static_cast<std::ptrdiff_t>(membershipSlot(varied.document,"C1",20)+1u),value);}
            else {const auto at=membershipSlot(varied.document,"C1",100)+1u;
                varied.document.insert(varied.document.begin()+static_cast<std::ptrdiff_t>(at),{{102,"{OPAQUE"},{5,"DEAD"},{330,"FACE"},{410,"not host layout"},{67,"1"},{102,"}"}});}
            if(!write(path,varied.document,binary))return false;BlockPrimaryProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!complete(data)){std::cerr<<"variant="<<kind<<'\n';return false;}++variants;}
        if(!write(path,f.document,binary))return false;
        stage.name="substituted body/context owners and live separation";
        for(unsigned kind=0u;kind<4u;++kind){BlockPrimaryProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;
            if(kind<2u)A::substituteBlockBody(data,0u,kind==1u);else A::substituteBlockContext(data,kind==3u);
            if(A::compoundMatched(data,0u))return false;++contexts;}
        BlockPrimaryProbe baseline;dx_data base;if(!baseline.fileImport(path.string(),&base,false)||!complete(base))return false;
        for(auto* block:base.blocks)for(auto* entity:block->ent)entity->layer="edited live layer";
        base.headerC.clearDxfCodePageDeclaration();if(!complete(base))return false;++contexts;
        const auto usage=A::compoundUsage(base);
        for(unsigned dimension=0;dimension<3u;++dimension)for(bool exact:{false,true}){BlockPrimaryProbe reader;dx_data data;A::compoundLimit(data,dimension,usage[dimension]-(exact?0u:1u));
            if(!reader.fileImport(path.string(),&data,false)||complete(data)!=exact||A::compoundFailed(data)==exact)return false;++quotas;}
        stage.name="observed isolated allocation recovery";
        for(std::size_t dispatch=0;dispatch<2u;++dispatch){bool recovered=false;for(long fault=0;fault<600;++fault){BlockPrimaryProbe reader;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||!complete(data))return false;long used=0;
            if(A::recaptureCompound(reader,data,dispatch,fault,used)){recovered=true;break;}
            if(!A::compoundFailed(data))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++allocations;
        }if(!recovered)return false;}
        stage.name="callback/tail failure and actual retry";
        for(unsigned kind=0u;kind<3u;++kind){auto varied=f;if(kind==2u)varied.document.pop_back();if(!write(path,varied.document,binary))return false;BlockPrimaryProbe reader;reader.throwPoint=kind==0u;reader.throwLine=kind==1u;dx_data data;bool threw=false,read=false;
            try{read=reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
            if(read||threw!=(kind<2u)||A::compoundMatched(data,0u)||reader.depth)return false;
            reader.throwPoint=reader.throwLine=false;if(!write(path,f.document,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++exceptions;}
        // No requirement/default widening: original missing required POINT/
        // LINE coordinates must still refuse ordinary parsing.
        stage.name="unchanged required-coordinate admission";
        for(unsigned field=0u;field<6u;++field){auto varied=f;const std::array<int,6> code{{10,20,10,20,11,21}};const auto at=membershipSlot(varied.document,field<2u?"C1":"C2",code[field]);varied.document.erase(varied.document.begin()+static_cast<std::ptrdiff_t>(at));
            if(!write(path,varied.document,binary))return false;BlockPrimaryProbe reader;dx_data data;if(reader.fileImport(path.string(),&data,false)||A::compoundMatched(data,0u))return false;
            if(!write(path,f.document,binary))return false;dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!complete(retry))return false;++refusals;}
    }
    std::cout<<"BLOCK primary profiles="<<profiles<<" original scalar spans="<<spans<<" field faults="<<mutations<<" refusals="<<refusals<<" contexts="<<contexts
        <<" quotas="<<quotas<<" allocation faults="<<allocations<<" failure/retries="<<exceptions<<" variants="<<variants<<'\n';stage.done=true;return true;
}
