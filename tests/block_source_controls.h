// SPDX-License-Identifier: GPL-2.0-or-later
// Included in the ATTDEF probe's anonymous namespace to share its small wire
// writer and independent original-file walker, not production parser helpers.
struct BlockFixture {Tags document,begin,end;};
BlockFixture blockFixture(const std::string& version,unsigned children,bool reconcile=true,bool dropped=false) {
    BlockFixture f;
    f.document={{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"}};
    if(reconcile) {
        const Tags table{{0,"SECTION"},{2,"TABLES"},{0,"TABLE"},{2,"BLOCK_RECORD"},{5,"B9"},
            {100,"AcDbSymbolTable"},{70,"1"},{0,"BLOCK_RECORD"},{5,"B0"},{330,"B9"},
            {100,"AcDbSymbolTableRecord"},{100,"AcDbBlockTableRecord"},{2,"RESOLVED"},
            {70,"4"},{280,"1"},{281,"0"},{310,"010203"},{0,"ENDTAB"},{0,"ENDSEC"}};
        f.document.insert(f.document.end(),table.begin(),table.end());
    }
    f.begin={{5,"B1"},{330,"B0"},{100,"AcDbEntity"},{8,"BEGIN_LAYER"},
        {102,"{CUSTOM"},{5,"DEAD"},{330,"FACE"},{1,"opaque begin"},{1001,"OPAQUE"},
        {102,"{INNER"},{360,"CAFE"},{102,"}"},{102,"}"},
        {100,"AcDbBlockBegin"},{2,dropped?"":"DECLARED"},{70,"2"},{10,"1.25"},{20,"2.5"},{30,"3.75"},
        {3,"DECLARED"},{1,"relative-xref"},{4,"description-é"},
        {1001,"BEGIN_APP"},{1003,"XDATA_LAYER"},{1010,"1"},{1020,"2"},{1030,"3"},{1070,"4"}};
    f.end={{5,"B2"},{330,"B0"},{100,"AcDbEntity"},{8,"END_LAYER"},{100,"AcDbBlockEnd"},
        {102,"{TAIL"},{5,"BEEF"},{330,"CAFE"},{1,"opaque footer"},{102,"}"},
        {1001,"END_APP"},{1010,"5"},{1020,"6"},{1030,"7"},{1070,"8"}};
    f.document.insert(f.document.end(),{{0,"SECTION"},{2,"BLOCKS"},{0,"BLOCK"}});
    f.document.insert(f.document.end(),f.begin.begin(),f.begin.end());
    for(unsigned n=0u;n<children;++n) {
        auto row=body(0u);row[0].value=n?"A2":"A1";
        f.document.push_back({0,"ATTDEF"});f.document.insert(f.document.end(),row.begin(),row.end());
    }
    f.document.push_back({0,"ENDBLK"});f.document.insert(f.document.end(),f.end.begin(),f.end.end());
    f.document.insert(f.document.end(),{{0,"ENDSEC"},{0,"EOF"}});return f;
}

class BlockSourceProbe final:public dx_iface {
public:
    bool selected=true,forceTransport=false,disableOnBegin=false,skipBegin=false,skipEnd=false,duplicateBegin=false,duplicateEnd=false;
    bool throwCleanupAlso=false;
    int throwPhase=0,allocationPhase=-1;long failAllocation=-1,allocations=0;
    long injectOnSelect=-1;
    unsigned depth=0,begins=0,ends=0,sourceBegins=0,sourceEnds=0,children=0;
    std::vector<std::string> events;
    DRW_RawDxfObject beginAlias,endAlias;
    std::function<void(DRW_Block&)> alterBegin,alterEnd;
    std::function<void(DRW_RawDxfObject&)> alterBeginRaw,alterEndRaw;
    void addHeader(const DRW_Header* header) override {if(cData)dx_iface::addHeader(header);}
    bool wantsDxfBlockSource() const override {
        if(injectOnSelect>=0)allocationRemaining=injectOnSelect;
        return selected&&(forceTransport||dx_iface::wantsDxfBlockSource());
    }
    template<class F> void allocating(int phase,F f) {
        if(allocationPhase==phase&&failAllocation>=0)allocationRemaining=failAllocation;
        f();
        if(allocationPhase==phase&&failAllocation>=0) {allocations=failAllocation-allocationRemaining;allocationRemaining=-1;}
    }
    void addDxfBlockBeginSource(const DRW_Block& value,const DRW_RawDxfObject& raw) override {
        ++sourceBegins;events.push_back("S");beginAlias=raw;
        if(alterBeginRaw)alterBeginRaw(beginAlias);
        if(!skipBegin)allocating(0,[&](){dx_iface::addDxfBlockBeginSource(value,beginAlias);});
        if(duplicateBegin)dx_iface::addDxfBlockBeginSource(value,beginAlias);
        if(disableOnBegin)selected=false;
        if(throwPhase==1)throw std::runtime_error("block begin source fault");
    }
    void addDxfBlockEndSource(const DRW_Block& value,const DRW_RawDxfObject& raw) override {
        ++sourceEnds;events.push_back("T");endAlias=raw;
        if(alterEndRaw)alterEndRaw(endAlias);
        if(!skipEnd)allocating(2,[&](){dx_iface::addDxfBlockEndSource(value,endAlias);});
        if(duplicateEnd)dx_iface::addDxfBlockEndSource(value,endAlias);
        if(throwPhase==2)throw std::runtime_error("block end source fault");
    }
    void addBlock(const DRW_Block& value) override {
        ++depth;++begins;events.push_back("B");
        if(alterBegin)alterBegin(const_cast<DRW_Block&>(value));
        allocating(1,[&](){dx_iface::addBlock(value);});
        if(throwPhase==6)throw std::runtime_error("block typed begin fault");
    }
    void addBlockEnd(const DRW_Block& value) override {
        events.push_back("F");if(alterEnd)alterEnd(const_cast<DRW_Block&>(value));
        allocating(3,[&](){dx_iface::addBlockEnd(value);});
        if(throwPhase==3)throw std::runtime_error("block typed footer fault");
    }
    void addAttDef(const DRW_Attdef& value) override {
        ++children;events.push_back("A");dx_iface::addAttDef(value);
        if(throwPhase==4)throw std::runtime_error("block child fault");
    }
    void endBlock() override {
        --depth;++ends;events.push_back("E");if(cData)allocating(4,[&](){dx_iface::endBlock();});
        if(throwPhase==5)throw std::runtime_error("block cleanup fault");
        if(throwCleanupAlso)throw std::logic_error("secondary block cleanup fault");
    }
};

bool blockRawMatches(const DRW_RawDxfObject& raw,const Tags& tags,bool binary,const char* name,std::uint32_t handle) {
    if(raw.name!=name||raw.handle!=handle||raw.parentHandle!=0xB0u||raw.hasRawValues==binary
        ||raw.groups.size()!=tags.size()||raw.rawValues.size()!=tags.size())return false;
    for(std::size_t i=0;i<tags.size();++i) {
        const auto& v=raw.groups[i];const auto& tag=tags[i];
        if(v.code()!=tag.code||(!binary&&raw.rawValues[i]!=tag.value)||(binary&&!raw.rawValues[i].empty()))return false;
        if(v.type()==DRW_Variant::STRING) {if(!v.content.s||*v.content.s!=tag.value)return false;}
        else if(v.type()==DRW_Variant::DOUBLE) {if(v.d_val()!=std::stod(tag.value))return false;}
        else if(v.type()==DRW_Variant::INTEGER) {if(v.i_val()!=std::stoi(tag.value))return false;}
        else if(v.type()==DRW_Variant::INTEGER64) {if(v.i64_val()!=std::stoll(tag.value))return false;}
        else return false;
    }
    return true;
}

bool blockOriginalWire(const std::filesystem::path& path,bool binary,const BlockFixture& fixture,
                       const DRW_RawDxfObject& begin,const DRW_RawDxfObject& end,std::size_t& spans) {
    const auto wire=attdefWireTags(path,binary);
    if(wire.size()!=fixture.document.size()||!blockRawMatches(begin,fixture.begin,binary,"BLOCK",0xB1u)
        ||!blockRawMatches(end,fixture.end,binary,"ENDBLK",0xB2u))return false;
    std::size_t first=wire.size(),last=wire.size();
    for(std::size_t i=0;i<wire.size();++i) {
        const auto& tag=wire[i];const auto& expected=fixture.document[i];
        if(tag.code!=expected.code||tag.begin>=tag.valueBegin||tag.valueBegin>=tag.end
            ||(i&&wire[i-1u].end!=tag.begin))return false;
        if(tag.kind==AttdefWireTag::Integer) {if(tag.integer!=std::stoll(expected.value))return false;}
        else if(tag.kind==AttdefWireTag::Real) {if(tag.real!=std::stod(expected.value))return false;}
        else if(tag.text!=expected.value)return false;
        if(tag.code==0&&tag.text=="BLOCK")first=i+1u;
        if(tag.code==0&&tag.text=="ENDBLK")last=i+1u;
    }
    if(first>=wire.size()||last>=wire.size())return false;
    for(std::size_t i=0;i<begin.groups.size();++i)if(wire[first+i].code!=begin.groups[i].code())return false;
    for(std::size_t i=0;i<end.groups.size();++i)if(wire[last+i].code!=end.groups[i].code())return false;
    spans+=begin.groups.size()+end.groups.size();return true;
}

bool blockSourceTests(const std::filesystem::path& path) {
    using A=DxIfaceTableSourceTestAccess;
    unsigned profiles=0,optouts=0,malformed=0,callbacks=0,throws=0,quotas=0,contexts=0,corruptions=0,codecQuotas=0;
    std::size_t spans=0;long allocations=0,codecAllocations=0;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})
     for(bool binary:{false,true})for(bool reconcile:{false,true})for(unsigned count:{0u,1u,2u}) {
        const auto fixture=blockFixture(version,count,reconcile);
        if(!write(path,fixture.document,binary))return false;
        BlockSourceProbe reader;dx_data data;
        if(!check(reader.fileImport(path.string(),&data,false)&&A::blockTransport(data),"actual block source envelope import"))return false;
        const auto* row=A::block(data);
        std::vector<std::string> expected{"S","B","T","F"};
        for(unsigned i=0;i<count;++i)expected.push_back("A");expected.push_back("E");
        if(!check(reader.events==expected&&reader.depth==0&&reader.sourceBegins==1&&reader.sourceEnds==1
            &&reader.children==count,"latched source/typed/staged-child order and balanced scope"))return false;
        if(!check(blockRawMatches(*row->beginRaw,fixture.begin,binary,"BLOCK",0xB1u)
            &&blockRawMatches(*row->endRaw,fixture.end,binary,"ENDBLK",0xB2u)
            &&row->original->name=="DECLARED"&&row->original->insUnits==0&&row->original->previewData.empty()
            &&row->effective->name==(reconcile?"RESOLVED":"DECLARED")&&row->effective->insUnits==(reconcile?4:0)
            &&row->effective->previewData.size()==(reconcile?3u:0u)&&row->footer->getIsEnd()
            &&row->original->handle!=0xDEADu&&row->footer->parentHandle!=0xCAFEu,
            "original identity/raw body survives reconciliation and opaque handle lookalikes"))return false;
        if(!check(blockOriginalWire(path,binary,fixture,*row->beginRaw,*row->endRaw,spans),"independent original-file begin/footer tag spans"))return false;
        for(unsigned fault=0;fault<4u;++fault) {
            auto corrupted=*row->beginRaw;
            if(fault==0)corrupted.groups.front().addString(5,"ABCD");
            if(fault==1)corrupted.name="ENDBLK";
            if(fault==2)std::swap(corrupted.groups[1],corrupted.groups[7]);
            if(fault==3)corrupted.parentHandle=0x99u;
            std::size_t ignored=0;
            if(!check(!blockOriginalWire(path,binary,fixture,corrupted,*row->endRaw,ignored),"original-file oracle detects body/family/order/metadata corruption"))return false;
            ++corruptions;
        }
        reader.beginAlias.groups.clear();reader.endAlias.groups.clear();
        data.blocks.front()->name="EDITED";data.blocks.front()->endBlockData.layer="EDITED";
        if(!check(row->beginRaw->groups.size()==fixture.begin.size()&&row->endRaw->groups.size()==fixture.end.size()
            &&row->effective->name!=(data.blocks.front()->name)&&row->footer->layer=="END_LAYER",
            "immutable original/effective/footer owners are independent from aliases and editable blocks"))return false;
        BlockSourceProbe optout;optout.selected=false;dx_data plain;
        if(!check(optout.fileImport(path.string(),&plain,false)&&!A::block(plain)
            &&optout.sourceBegins==0&&optout.sourceEnds==0&&optout.children==count,
            "opt-out preserves typed consumers without block snapshots"))return false;
        ++profiles;++optouts;
    }
    for(bool binary:{false,true}) {
        for(unsigned context=0;context<8u;++context) {
            const bool reconcile=context==1u||context==2u;
            const bool dropped=context<=3u;
            auto special=blockFixture("AC1032",1u,reconcile,dropped);
            if(context==2u||context==3u) {
                const auto name=std::find_if(special.document.begin(),special.document.end(),[](const Tag& t){return t.code==2&&t.value.empty();});
                special.document.erase(name);
            }
            if(context==4u||context==5u) {
                auto marker=std::find_if(special.document.begin(),special.document.end(),[&](const Tag& t){return t.code==0&&t.value==(context==4u?"BLOCK":"ENDBLK");});
                for(auto at=marker+1;at!=special.document.end()&&at->code!=0;) {
                    if(at->code==1030||at->code==1070)at=special.document.erase(at);else ++at;
                }
            }
            if(context==6u) {
                const auto marker=std::find_if(special.document.begin(),special.document.end(),[](const Tag& t){return t.code==0&&t.value=="ENDBLK";});
                std::find_if(marker,special.document.end(),[](const Tag& t){return t.code==330;})->value="99";
            }
            if(context==7u)special=blockFixture("AC1015",1u,true);
            if(!write(path,special.document,binary))return false;
            BlockSourceProbe reader;dx_data data;
            if(!check(reader.fileImport(path.string(),&data,false),"dropped/reconciled/pending/legacy block compatibility"))return false;
            const auto* row=A::block(data);
            if(context==0u||context==3u||context==7u) {
                if(!check(!row&&reader.sourceBegins==0u&&reader.sourceEnds==0u,
                    "dropped or legacy block does not synthesize source envelopes"))return false;
            } else {
                if(!check(A::blockTransport(data),"context transport pairing is distinct from qualification"))return false;
                if(context==1u||context==2u) {
                    if(!check(row->original->name==(context==1u?"":"*U0")&&row->effective->name=="RESOLVED",
                        "empty/absent original names remain distinct from effective BLOCK_RECORD names"))return false;
                }
                if(context==4u||context==5u) {
                    const auto state=context==4u?row->originalCursor:row->footerCursor;
                    const auto& copied=context==4u?*row->original:*row->footer;
                    if(!check(state.present&&state.coordinateMask==3u&&!dxfAttdef::Decoder::entityCursor(copied).present,
                        "original incomplete XDATA state cannot be laundered by model copying"))return false;
                }
                if(context==6u&&!check(row->original->parentHandle==0xB0u&&row->footer->parentHandle==0x99u,
                    "tolerated owner mismatch is retained as evidence, not reconciled or qualified"))return false;
            }
            ++contexts;
        }
        auto fixture=blockFixture("AC1032",1u,true);
        if(!write(path,fixture.document,binary))return false;
        BlockSourceProbe latched;latched.disableOnBegin=true;dx_data data;
        if(!check(latched.fileImport(path.string(),&data,false)&&A::blockTransport(data)&&latched.sourceEnds==1,
            "selection is latched even when observer opts out after begin"))return false;
        for(unsigned fault=0;fault<9u;++fault) {
            BlockSourceProbe reader;
            if(fault==0)reader.skipBegin=true;
            if(fault==1)reader.skipEnd=true;
            if(fault==2)reader.duplicateBegin=true;
            if(fault==3)reader.duplicateEnd=true;
            if(fault==4)reader.alterBegin=[](DRW_Block& v){v.basePoint.x+=1.0;};
            if(fault==5)reader.alterEnd=[](DRW_Block& v){v.layer="WRONG";};
            if(fault==6)reader.alterBegin=[](DRW_Block& v){v.flags^=1;};
            if(fault==7)reader.alterEnd=[](DRW_Block& v){v.parentHandle=0x99u;};
            if(fault==8)reader.alterBegin=[](DRW_Block& v){v.handle=0x99u;};
            dx_data bad;
            if(!check(reader.fileImport(path.string(),&bad,false)&&A::blockFailed(bad)&&!A::blockTransport(bad),
                "missing/duplicate/changed block callback cannot complete source transport"))return false;
            reader.skipBegin=reader.skipEnd=reader.duplicateBegin=reader.duplicateEnd=false;reader.alterBegin={};reader.alterEnd={};
            dx_data retry;
            if(!check(reader.fileImport(path.string(),&retry,false)&&A::blockTransport(retry),"block callback fresh-data retry"))return false;
            ++callbacks;
        }
        for(bool cleanup:{false,true})for(int phase=1;phase<=6;++phase) {
            BlockSourceProbe reader;reader.throwPhase=phase;reader.throwCleanupAlso=cleanup;dx_data failed;bool threw=false;
            try {(void)reader.fileImport(path.string(),&failed,false);}catch(const std::runtime_error&){threw=true;}
            if(!check(threw&&!A::blockTransport(failed)&&failed.dxfReadOutcome.readThrew&&reader.depth==0u
                &&reader.ends==(phase==1?0u:1u),"source/typed begin/footer/child/cleanup exception preserves import failure and balanced scope")) {
                std::cerr<<"phase="<<phase<<" depth="<<reader.depth<<" ends="<<reader.ends<<'\n';return false;
            }
            reader.throwPhase=0;reader.throwCleanupAlso=false;dx_data retry;
            if(!check(reader.fileImport(path.string(),&retry,false)&&A::blockTransport(retry),"throwing block callback recovery"))return false;
            ++throws;
        }
        for(unsigned fault=0;fault<5u;++fault) {
            auto bad=fixture.document;
            const auto footer=std::find_if(bad.begin(),bad.end(),[](const Tag& t){return t.code==0&&t.value=="ENDBLK";});
            if(fault==0)bad.erase(footer,bad.end()-1);
            if(fault==1)std::find_if(footer,bad.end(),[](const Tag& t){return t.code==5;})->value="not-hex";
            if(fault==2)std::find_if(footer,bad.end(),[](const Tag& t){return t.code==100;})->value="AcDbBad";
            if(fault==3)bad.insert(footer+1,{5,"B2"});
            if(fault==4)std::find_if(footer,bad.end(),[](const Tag& t){return t.code==330;})->value="";
            if(!write(path,bad,binary))return false;
            BlockSourceProbe reader;dx_data failed;
            if(!check(!reader.fileImport(path.string(),&failed,false)&&!A::blockTransport(failed)
                &&reader.children==0u&&reader.sourceEnds==0u&&reader.depth==0u,"bad footer leaves only preliminary begin and no staged children"))return false;
            if(!write(path,fixture.document,binary))return false;
            dx_data retry;
            if(!check(reader.fileImport(path.string(),&retry,false)&&A::blockTransport(retry),"malformed footer corrected retry"))return false;
            ++malformed;
        }
    }
    const auto fixture=blockFixture("AC1032",1u,true);if(!write(path,fixture.document,false))return false;
    BlockSourceProbe measured;dx_data baseline;
    if(!measured.fileImport(path.string(),&baseline,false)||!A::blockTransport(baseline))return false;
    const auto usage=A::blockUsage(baseline);
    {
        BlockSourceProbe reader;dx_data reused;
        if(!reader.fileImport(path.string(),&reused,false)||!A::blockTransport(reused))return false;
        (void)reader.fileImport(path.string(),&reused,false);
        if(!check(!A::blockTransport(reused),"appended destination cannot refresh old block evidence into a new import generation"))return false;
    }
    for(bool footer:{false,true}) {
        BlockSourceProbe reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;
        const auto* row=A::block(data);
        if(footer)reader.dx_iface::addDxfBlockEndSource(*row->footer,*row->endRaw);
        else reader.dx_iface::addDxfBlockBeginSource(*row->original,*row->beginRaw);
        if(!check(A::blockFailed(data)&&!A::blockTransport(data),"late block source callback is a sticky context failure"))return false;
        dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::blockTransport(retry))return false;
    }
    for(unsigned dimension=0;dimension<usage.size();++dimension) {
        BlockSourceProbe reader;dx_data exact;A::blockLimit(exact,dimension,usage[dimension]);
        if(!check(reader.fileImport(path.string(),&exact,false)&&A::blockTransport(exact),"exact block source quota"))return false;
        dx_data below;A::blockLimit(below,dimension,usage[dimension]-1u);
        if(!check(reader.fileImport(path.string(),&below,false)&&A::blockFailed(below)&&!A::blockTransport(below),"below block source quota"))return false;
        dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!A::blockTransport(retry))return false;++quotas;
    }
    for(int phase=0;phase<4;++phase) {
        BlockSourceProbe calibration;calibration.allocationPhase=phase;calibration.failAllocation=(std::numeric_limits<long>::max)();dx_data sample;
        if(!check(calibration.fileImport(path.string(),&sample,false)&&A::blockTransport(sample)
            &&calibration.allocations>0&&calibration.allocations<2000,"block snapshot allocation calibration"))return false;
        for(long failure=0;failure<calibration.allocations;++failure) {
            BlockSourceProbe reader;reader.allocationPhase=phase;reader.failAllocation=failure;dx_data failed;
            if(!check(reader.fileImport(path.string(),&failed,false)&&!A::blockTransport(failed)
                &&(A::blockFailed(failed)||failed.typedEntityCaptureFailed),"block allocation failure cannot masquerade as combined source/consumer completion"))return false;
            reader.allocationPhase=-1;dx_data retry;
            if(!check(reader.fileImport(path.string(),&retry,false)&&A::blockTransport(retry),"block allocation fresh-data recovery"))return false;
            ++allocations;
        }
    }
    for(bool binary:{false,true}) {
        if(!write(path,fixture.document,binary))return false;
        dxfRW codec(path.string().c_str());
        std::array<std::size_t,2> bytes{},nodes{};
        for(unsigned selected=0;selected<2u;++selected)for(bool node:{false,true}) {
            const auto attempt=[&](std::size_t n) {
                codec.setDxfReadRetainedByteBudget(node?1048576u:n);
                codec.setDxfReadTypedVariantNodeBudget(node?n:10000u);
                BlockSourceProbe probe;probe.forceTransport=true;probe.selected=selected!=0;
                const bool accepted=codec.read(&probe,false);
                if(probe.depth)throw std::runtime_error("codec quota left an open block");
                return accepted;
            };
            std::size_t low=0,high=1048576u;
            while(low<high) {const auto middle=low+(high-low)/2u;if(attempt(middle))high=middle;else low=middle+1u;}
            if(!check(low&&attempt(low)&&!attempt(low-1u),"exact/below raw block reader quota with same-codec recovery"))return false;
            (node?nodes:bytes)[selected]=low;++codecQuotas;
        }
        if(!check(bytes[0]<bytes[1]&&nodes[0]<nodes[1],"opt-out adds no raw block text/node charges"))return false;
        codec.setDxfReadRetainedByteBudget(1048576u);codec.setDxfReadTypedVariantNodeBudget(10000u);
        BlockSourceProbe measured;measured.forceTransport=true;measured.injectOnSelect=(std::numeric_limits<long>::max)();
        bool accepted=false;try {accepted=codec.read(&measured,false);}catch(...){}
        const long count=(std::numeric_limits<long>::max)()-allocationRemaining;allocationRemaining=-1;
        if(!check(accepted&&count>0&&count<5000,"source-selection through read-completion allocation calibration"))return false;
        for(long failure=0;failure<=count;++failure) {
            BlockSourceProbe probe;probe.forceTransport=true;probe.injectOnSelect=failure;bool ok=false;
            try {ok=codec.read(&probe,false);}catch(...){}
            allocationRemaining=-1;
            if(!check(probe.depth==0u&&probe.begins==probe.ends
                &&(!ok||(probe.sourceBegins==1u&&probe.sourceEnds==1u&&probe.children==1u
                    &&blockRawMatches(probe.beginAlias,fixture.begin,binary,"BLOCK",0xB1u)
                    &&blockRawMatches(probe.endAlias,fixture.end,binary,"ENDBLK",0xB2u))),
                "raw capture/staging/callback allocation refusal leaves no open or apparently complete partial scope"))return false;
            BlockSourceProbe retry;retry.forceTransport=true;
            if(!check(codec.read(&retry,false)&&retry.sourceBegins==1u&&retry.sourceEnds==1u&&retry.depth==0u,
                "corrected same-codec read after each raw allocation schedule"))return false;
            ++codecAllocations;
        }
    }
    std::cout<<"BLOCK source profiles="<<profiles<<"; opt-outs="<<optouts<<"; callback faults/retries="<<callbacks
        <<"; exception routes="<<throws<<"; malformed footer retries="<<malformed<<"; quota boundaries="<<quotas
        <<"; allocation faults/retries="<<allocations<<"; context routes="<<contexts<<"; original spans="<<spans
        <<"; oracle corruptions="<<corruptions<<"; reader quota boundaries="<<codecQuotas
        <<"; raw allocation schedules/retries="<<codecAllocations<<'\n';
    return profiles==48u&&optouts==48u&&callbacks==18u&&throws==24u&&malformed==10u&&quotas==4u
        &&contexts==16u&&corruptions==192u&&codecQuotas==8u;
}
