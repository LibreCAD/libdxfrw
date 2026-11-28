// SPDX-License-Identifier: GPL-2.0-or-later
struct ApplicationWidthEntity:DRW_Line {
    bool common(int code,const std::unique_ptr<dxfReader>& reader){return DRW_Entity::parseCode(code,reader);}
    bool group(int code,const std::unique_ptr<dxfReader>& reader){return DRW_Entity::parseDxfGroups(code,reader);}
};
struct ApplicationWidthTable:DRW_Layer {
    bool common(int code,const std::unique_ptr<dxfReader>& reader){return DRW_TableEntry::parseCode(code,reader);}
    bool group(int code,const std::unique_ptr<dxfReader>& reader){return DRW_TableEntry::parseDxfGroups(code,reader);}
};
bool applicationHandleWidthTests(const std::filesystem::path& path) {
    unsigned positives=0u,refusals=0u,exceptions=0u,retries=0u,literal=0u,fullRefusals=0u;long allocations=0;
    const Tags positive{{102,"{CUSTOM"},{320,"FFFFFFFFFFFFFFFF"},{330,"0123456789ABCDEF"},{340,"FFFFFFFF00000001"},
        {360,"0000000100000000"},{1005,"123456789ABCDEF0"},{102,"{ACAD_REACTORS"},{330,"FEDCBA9876543210"},{102,"}"},{102,"}"}};
    for(bool binary:{false,true}) {
        const auto drawingPath=path.parent_path()/"wide-app-drawing.dxf";
        const auto tablePath=path.parent_path()/"wide-app-table.dxf",badPath=path.parent_path()/"wide-app-bad.dxf";
        const auto tableDrawing=[](const Tags& payload) {
            Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1032"},{0,"ENDSEC"},
                {0,"SECTION"},{2,"TABLES"},{0,"TABLE"},{2,"LAYER"},{5,"B9"},{100,"AcDbSymbolTable"},{70,"1"},
                {0,"LAYER"},{5,"B0"},{330,"B9"},{100,"AcDbSymbolTableRecord"},{100,"AcDbLayerTableRecord"},
                {2,"BODY_LAYER"},{70,"0"},{62,"7"},{6,"CONTINUOUS"}};
            tags.insert(tags.end(),payload.begin(),payload.end());tags.insert(tags.end(),{{0,"ENDTAB"},{0,"ENDSEC"},{0,"EOF"}});return tags;
        };
        auto f=compoundFixture("AC1032",0u);compoundAppend(f.document,"D2",positive);
        if(!write(drawingPath,f.document,binary)||!write(tablePath,tableDrawing(positive),binary))return false;
        CompoundBodyProbe facade;
        const auto retry=[&](bool tableRoute=false){
            dx_data data;++retries;if(!facade.fileImport((tableRoute?tablePath:drawingPath).string(),&data,false)||facade.depth)return false;
            if(!tableRoute)return compoundProfileMatches(data,2u);
            return data.layers.size()==1u&&data.layers.front().parentHandle==0xB9u&&!data.layers.front().appData.empty()
                &&data.layers.front().reactorHandles.empty()&&!data.layers.front().xDictHandle;
        };
        if(!retry())return false;
        for(bool table:{false,true})for(bool outerWide:{false,true}) {
            unsigned phase=0u,detail=0u;
            const auto exercise=[&](auto model) {
                auto tags=positive;tags.insert(tags.end(),{{330,"1234"},{347,"100000000"}});
                if(!write(path,tags,binary))return false;
                std::ifstream input(path,std::ios::binary);if(binary)input.seekg(22);
                std::unique_ptr<dxfReader> reader=binary?std::unique_ptr<dxfReader>(new dxfReaderBinary(&input)):
                    std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
                reader->setVersion("AC1032",true);reader->setAllowWideHandleLexemes(outerWide);
                int code=0;if(!reader->readRec(&code)||code!=102||!model.common(code,reader)||reader->allowsWideHandleLexemes()!=outerWide
                    ||model.appData.size()!=1u||!model.reactorHandles.empty()||model.xDictHandle)return false;
                unsigned handles=0u;
                for(const auto& value:model.appData.front())if(isDxfHandleCode(value.code())) {
                    const auto at=std::find_if(positive.begin(),positive.end(),[&](const Tag& t){return t.code==value.code()&&t.value==value.c_str();});
                    if(at==positive.end()||value.type()!=DRW_Variant::STRING)return false;++handles;
                }
                if(handles!=6u||!reader->readRec(&code)||code!=330||!model.common(code,reader)||model.parentHandle!=0x1234u)return false;
                const bool readWide=reader->readRec(&code);
                if(outerWide) {if(!readWide||reader->requireTypedHandleString()||reader->lastInvalidHandleCode()!=347)return false;}
                else if(readWide||reader->lastInvalidHandleCode()!=347)return false;
                ++positives;
                for(unsigned fault=0u;fault<8u;++fault) {
                    phase=1u;detail=fault;
                    Tags bad;
                    if(fault==0u)bad={{102,"{ACAD_REACTORS"},{330,"100000000"},{102,"}"}};
                    if(fault==1u)bad={{102,"{ACAD_XDICTIONARY"},{360,"100000000"},{102,"}"}};
                    if(fault==2u)bad={{102,"{ACAD_XDICTIONARY"},{360,"12"},{360,"100000000"},{102,"}"}};
                    if(fault==3u)bad={{102,"{CUSTOM"},{320,"1FFFFFFFFFFFFFFFF"},{102,"}"}};
                    if(fault==4u)bad={{102,"{CUSTOM"},{320,"NOTHEX"},{102,"}"}};
                    if(fault==5u)bad={{102,"{CUSTOM"},{320,"FFFFFFFFFFFFFFFF"}};
                    if(fault==6u)bad={{102,"{CUSTOM"},{102,"not-a-control"}};
                    if(fault==7u)bad={{102,"{CUSTOM"},{0,"POINT"}};
                    if(!write(path,bad,binary))return false;
                    std::ifstream stream(path,std::ios::binary);if(binary)stream.seekg(22);
                    std::unique_ptr<dxfReader> decoder=binary?std::unique_ptr<dxfReader>(new dxfReaderBinary(&stream)):
                        std::unique_ptr<dxfReader>(new dxfReaderAscii(&stream));
                    decoder->setVersion("AC1032",true);decoder->setAllowWideHandleLexemes(outerWide);
                    std::decay_t<decltype(model)> refused;
                    if(!decoder->readRec(&code)||code!=102||refused.common(code,decoder)||decoder->allowsWideHandleLexemes()!=outerWide||!refused.appData.empty())return false;
                    if(fault<=4u&&decoder->lastInvalidHandleCode()!=(fault==0u?330:fault<=2u?360:320))return false;
                    if(!outerWide) {
                        auto badDrawing=f;if(!table)compoundAppend(badDrawing.document,"D2",bad);
                        if(!write(badPath,table?tableDrawing(bad):badDrawing.document,binary))return false;
                        dx_data failed;
                        if(facade.fileImport(badPath.string(),&failed,false)||facade.depth||failed.dxfReadOutcome.operationSucceeded
                            ||(table&&!failed.layers.empty()))return false;
                        ++fullRefusals;
                    }
                    if(!retry(table))return false;++refusals;
                }
                // An I/O-mask exception must restore the caller's prior mode.
                const Tags truncated{{102,"{CUSTOM"},{320,"FFFFFFFFFFFFFFFF"}};
                phase=2u;
                if(!write(path,truncated,binary))return false;
                std::ifstream stream(path,std::ios::binary);if(binary)stream.seekg(22);
                std::unique_ptr<dxfReader> decoder=binary?std::unique_ptr<dxfReader>(new dxfReaderBinary(&stream)):
                    std::unique_ptr<dxfReader>(new dxfReaderAscii(&stream));
                decoder->setVersion("AC1032",true);decoder->setAllowWideHandleLexemes(outerWide);
                if(!decoder->readRec(&code)||code!=102)return false;
                stream.exceptions(std::ios::failbit|std::ios::badbit);
                std::decay_t<decltype(model)> throwing;bool threw=false;
                try{(void)throwing.group(code,decoder);}catch(const std::ios_base::failure&){threw=true;}
                if(!threw||decoder->allowsWideHandleLexemes()!=outerWide||!throwing.appData.empty()) {
                    std::cerr<<"application mask threw="<<threw<<" restored="<<decoder->allowsWideHandleLexemes()<<" groups="<<throwing.appData.size()<<" flags="<<stream.rdstate()<<'\n';return false;
                }
                if(!retry(table))return false;++exceptions;
                bool calibrated=false;
                for(long fault=0;fault<200&&!calibrated;++fault) {
                    phase=3u;detail=static_cast<unsigned>(fault);
                    if(!write(path,positive,binary))return false;
                    std::ifstream data(path,std::ios::binary);if(binary)data.seekg(22);
                    std::unique_ptr<dxfReader> valueReader=binary?std::unique_ptr<dxfReader>(new dxfReaderBinary(&data)):
                        std::unique_ptr<dxfReader>(new dxfReaderAscii(&data));
                    valueReader->setVersion("AC1032",true);valueReader->setAllowWideHandleLexemes(outerWide);
                    if(!valueReader->readRec(&code)||code!=102)return false;
                    std::decay_t<decltype(model)> value;bool parsed=false;
                    allocationRemaining=fault;
                    try{parsed=value.common(code,valueReader);}catch(const std::bad_alloc&){}
                    const long used=fault-allocationRemaining;allocationRemaining=-1;
                    if(valueReader->allowsWideHandleLexemes()!=outerWide)return false;
                    if(parsed){if(used!=fault||value.appData.size()!=1u)return false;calibrated=true;}
                    else{if(!value.appData.empty())return false;++allocations;}
                    if(!retry(table))return false;
                }
                return calibrated;
            };
            const bool passed=table?exercise(ApplicationWidthTable{}):exercise(ApplicationWidthEntity{});
            if(!passed){std::cerr<<"application width binary="<<binary<<" table="<<table<<" outer="<<outerWide<<" phase="<<phase<<" detail="<<detail<<'\n';return false;}
        }
    }
    { // Exact source literals are not the default output-remap contract.
        using namespace dx_standard_output;
        const std::string wire="320\nFFFFFFFFFFFFFFFF\n";
        const std::vector<DRW_Variant> source{DRW_Variant(320,UTF8STRING("FFFFFFFFFFFFFFFF"))};
        dxfRW::DxfObjectsWriteView view{};view.bytes=wire;view.version=DRW::AC1032;view.binary=false;view.codePage="UTF-8";
        unsigned mapped=0u;const auto map=[&](std::uint32_t key,std::uint32_t& target){++mapped;target=key;return true;};
        if(rawScalarTransportEqual(source,wire,view,map)||!rawScalarTransportEqual(source,wire,view,map,false,1024u,true)||mapped)return false;++literal;
        const std::string alias="330\n00000001\n";const std::vector<DRW_Variant> canonical{DRW_Variant(330,UTF8STRING("1"))};view.bytes=alias;
        if(!rawScalarTransportEqual(canonical,alias,view,map)||rawScalarTransportEqual(canonical,alias,view,map,false,1024u,true)||mapped!=1u)return false;++literal;
    }
    std::cout<<"Application wide-handle positives="<<positives<<"; typed/malformed refusals="<<refusals<<"; stream exceptions="<<exceptions
        <<"; observed allocation faults="<<allocations<<"; corrected actual DXF imports="<<retries<<"; full entity/table refusals="<<fullRefusals
        <<"; literal/output boundaries="<<literal<<'\n';
    return positives==8u&&refusals==64u&&exceptions==8u&&allocations>0&&literal==2u&&fullRefusals==32u;
}
