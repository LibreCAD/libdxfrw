// SPDX-License-Identifier: GPL-2.0-or-later
bool legacyAttdefScopeTests(const std::filesystem::path& path) {
    using A=DxIfaceTableSourceTestAccess;using S=DRW_DxfIdentitySource;
    struct Probe final:dx_iface {
        bool active=true;unsigned fault=0u;bool duplicateBlockTargetIdentity=false;
        bool wantsDxfBlockSource()const override{return active;}
        bool wantsDxfBlockChildrenSource()const override{return active;}
        bool wantsDxfBlockDeliverySource()const override{return active;}
        bool wantsDxfAttdefSource()const override{return active;}
        void addDxfIdentitySource(const S& value)override {
            auto& source=const_cast<S&>(value);auto saved=source;
            if(fault){
                auto at=std::find_if(source.candidates.begin(),source.candidates.end(),[](const auto& c){return c.scope==S::Scope::Outer&&c.lexeme=="A1";});
                if(at==source.candidates.end())throw std::runtime_error("missing selected candidate");
                const auto pos=static_cast<std::size_t>(at-source.candidates.begin()),row=at->record;
                switch(fault){
                case 1:source.candidates.push_back(*at);break;
                case 2:source.candidates[pos].scope=S::Scope::Application;break;
                case 3:source.candidates[pos].admission=S::Admission::Duplicate;break;
                case 4:source.records[row].name="LINE";break;
                case 5:++source.records[row].marker.beginOffset;break;
                case 6:source.records[row].section=source.sections.size();break;
                case 7:source.records[row].endMarker.ordinal++;break;
                case 8:source.records[row].firstCandidate=source.candidates.size();break;
                case 9:source.unmatchedAdmissions=1u;break;
                case 10:source.records[row].kind=S::Kind::Unclassified;break;
                case 11:source.candidates[pos].span.beginOffset=source.records[row].marker.beginOffset;break;
                case 12:if(source.binary)++source.candidates[pos].span.endOffset;else source.candidates[pos].rawValue="A2";break;
                case 13:++source.candidates[pos].repeatedAdmissions;break;
                default:throw std::runtime_error("bad test fault");
                }
            }
            if(duplicateBlockTargetIdentity) {
                const auto target=std::find_if(source.candidates.begin(),source.candidates.end(),[&](const auto& candidate) {
                    return candidate.code==5&&candidate.lexeme=="A7E"&&candidate.record<source.records.size()
                        &&source.records[candidate.record].kind==S::Kind::TableRecord
                        &&source.records[candidate.record].name=="BLOCK_RECORD";
                });
                if(target==source.candidates.end())throw std::runtime_error("missing selected BLOCK_RECORD target candidate");
                const auto targetRecord=source.records[target->record];
                const auto duplicateRecordIndex=source.records.size();
                auto duplicateRecord=targetRecord;duplicateRecord.firstCandidate=source.candidates.size();
                duplicateRecord.candidates=1u;source.records.push_back(std::move(duplicateRecord));
                auto duplicateCandidate=*target;duplicateCandidate.record=duplicateRecordIndex;
                source.candidates.push_back(std::move(duplicateCandidate));
            }
            dx_iface::addDxfIdentitySource(value);source=std::move(saved);
        }
    };
    const Tags tableRow{{5,"B0"},{102,"{ACAD_XDICTIONARY"},{360,"D0"},{102,"}"},{330,"B9"},
        {100,"AcDbSymbolTableRecord"},{100,"AcDbBlockTableRecord"},{2,"MY_BLOCK"},{340,"0"},
        {102,"{BLKREFS"},{331,"E0"},{102,"}"},{1001,"TEST_APP"},{1000,"x"},{1002,"{"},{1070,"1"},{1070,"4"},{1002,"}"}};
    const Tags attr{{5,"A1"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbText"},
        {10,"0"},{20,"0"},{30,"0"},{40,"5"},{1,"default"},{100,"AcDbAttributeDefinition"},
        {3,"prompt"},{2,"ATTINFO"},{70,"0"},{1001,"AcDbAttr"},{1000,"x"},{1070,"0"},{1070,"1"}};
    Tags doc{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1018"},{9,"$DWGCODEPAGE"},{3,"ANSI_1252"},
        {0,"ENDSEC"},{0,"SECTION"},{2,"TABLES"},{0,"TABLE"},{2,"APPID"},{5,"A9"},{100,"AcDbSymbolTable"},{70,"5"},
        {0,"APPID"},{5,"A8"},{330,"A9"},{100,"AcDbSymbolTableRecord"},{100,"AcDbRegAppTableRecord"},{2,"TEST_APP"},{70,"0"},
        {0,"APPID"},{5,"A7"},{330,"A9"},{100,"AcDbSymbolTableRecord"},{100,"AcDbRegAppTableRecord"},{2,"AcDbAttr"},{70,"0"},
        {0,"APPID"},{5,"A5"},{330,"A9"},{100,"AcDbSymbolTableRecord"},{100,"AcDbRegAppTableRecord"},{2,"AcDbDynamicBlockGUID"},{70,"0"},
        {0,"APPID"},{5,"A6"},{330,"A9"},{100,"AcDbSymbolTableRecord"},{100,"AcDbRegAppTableRecord"},{2,"ACAD"},{70,"0"},
        {0,"APPID"},{5,"A3"},{330,"A9"},{100,"AcDbSymbolTableRecord"},{100,"AcDbRegAppTableRecord"},{2,"AcDbBlockRepBTag"},{70,"0"},{0,"ENDTAB"},
        {0,"TABLE"},{2,"BLOCK_RECORD"},{5,"B9"},{100,"AcDbSymbolTable"},{70,"4"},{0,"BLOCK_RECORD"}};
    doc.insert(doc.end(),tableRow.begin(),tableRow.end());
    doc.insert(doc.end(),{{0,"BLOCK_RECORD"},{5,"C1"},{102,"{ACAD_XDICTIONARY"},{360,"D1"},{102,"}"},
        {330,"B9"},{100,"AcDbSymbolTableRecord"},
        {100,"AcDbBlockTableRecord"},{2,"*U20"},{340,"0"},
        {102,"{BLKREFS"},{331,"B1"},{102,"}"},
        {1001,"AcDbDynamicBlockGUID"},{1000,"{AF4CD1DE-22F5-46CB-967C-41B95DEED31F}"},
        {1001,"ACAD"},{1000,"DesignCenter Data"},{1002,"{"},{1070,"1"},{1070,"4"},{1002,"}"},
        {0,"BLOCK_RECORD"},{5,"A91"},{102,"{ACAD_XDICTIONARY"},{360,"A92"},{102,"}"},
        {330,"B9"},{100,"AcDbSymbolTableRecord"},{100,"AcDbBlockTableRecord"},{2,"*U18"},{340,"0"},
        {102,"{BLKREFS"},{331,"E1"},{102,"}"},{1001,"AcDbBlockRepBTag"},{1070,"1"},{1005,"A7E"},
        {0,"BLOCK_RECORD"},{5,"A7E"},{102,"{ACAD_XDICTIONARY"},{360,"A7F"},{102,"}"},
        {330,"B9"},{100,"AcDbSymbolTableRecord"},{100,"AcDbBlockTableRecord"},{2,"*U17"},{340,"0"},
        {102,"{BLKREFS"},{331,"E2"},{102,"}"},{1001,"TEST_APP"},{1000,"target"},
        {0,"ENDTAB"},{0,"ENDSEC"},{0,"SECTION"},{2,"BLOCKS"},{0,"BLOCK"},
        {5,"B1"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},{2,"MY_BLOCK"},
        {70,"0"},{10,"0"},{20,"0"},{30,"0"},{3,"MY_BLOCK"},{1,""},{0,"ATTDEF"}});
    doc.insert(doc.end(),attr.begin(),attr.end());doc.insert(doc.end(),{{0,"ENDBLK"},{5,"B2"},{330,"B0"},
        {100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"},{0,"ENDSEC"},{0,"SECTION"},{2,"OBJECTS"},
        {0,"DICTIONARY"},{5,"C"},{330,"0"},{100,"AcDbDictionary"},{280,"0"},{281,"1"},
        {0,"DICTIONARY"},{5,"D0"},{330,"B0"},{100,"AcDbDictionary"},{280,"1"},{281,"1"},
        {0,"DICTIONARY"},{5,"D1"},{330,"C1"},{100,"AcDbDictionary"},{280,"1"},{281,"1"},
        {3,"ACAD_SORTENTS"},{360,"D4"},{3,"AcDbBlockTableRecordData"},{360,"D2"},
        {0,"SORTENTSTABLE"},{5,"D4"},{102,"{ACAD_REACTORS"},{330,"D1"},{102,"}"},
        {330,"D1"},{100,"AcDbSortentsTable"},{330,"C1"},
        {0,"DICTIONARY"},{5,"D2"},{102,"{ACAD_REACTORS"},{330,"D1"},{102,"}"},
        {330,"D1"},{100,"AcDbDictionary"},{280,"1"},{281,"1"},{3,"Edit"},{360,"D3"},
        {0,"XRECORD"},{5,"D3"},{102,"{ACAD_REACTORS"},{330,"D2"},{102,"}"},
        {330,"D2"},{100,"AcDbXrecord"},{280,"1"},{1070,"0"},{1070,"1"},{1070,"1"},
        {0,"ENDSEC"},{0,"EOF"}});
    Tags outputDoc=doc;
    const auto eraseRecord=[&](const char* name,const char* handle) {
        auto first=outputDoc.end();
        for(auto it=outputDoc.begin();it!=outputDoc.end();++it)
            if(it->code==0&&it->value==name&&std::next(it)!=outputDoc.end()
                &&std::next(it)->code==5&&std::next(it)->value==handle) {first=it;break;}
        if(first==outputDoc.end())return false;
        auto end=std::find_if(std::next(first),outputDoc.end(),[](const Tag& t){return t.code==0;});
        if(end==outputDoc.end())return false;
        outputDoc.erase(first,end);return true;
    };
    if(!eraseRecord("BLOCK_RECORD","C1")||!eraseRecord("BLOCK_RECORD","A91")
        ||!eraseRecord("BLOCK_RECORD","A7E")||!eraseRecord("DICTIONARY","D1")
        ||!eraseRecord("SORTENTSTABLE","D4")||!eraseRecord("DICTIONARY","D2")
        ||!eraseRecord("XRECORD","D3"))return false;
    auto blockRecordTable=outputDoc.end();
    for(auto it=outputDoc.begin();it!=outputDoc.end();++it)
        if(it->code==0&&it->value=="TABLE"&&std::next(it)!=outputDoc.end()
            &&std::next(it)->code==2&&std::next(it)->value=="BLOCK_RECORD") {blockRecordTable=it;break;}
    if(blockRecordTable==outputDoc.end())return false;
    auto blockRecordCount=std::find_if(std::next(blockRecordTable),outputDoc.end(),[](const Tag& t){return t.code==70;});
    if(blockRecordCount==outputDoc.end())return false;blockRecordCount->value="1";
    const auto findRecord=[](Tags& tags,const char* name,const char* handle) {
        for(auto it=tags.begin();it!=tags.end();++it)
            if(it->code==0&&it->value==name&&std::next(it)!=tags.end()
                &&std::next(it)->code==5&&std::next(it)->value==handle)return it;
        return tags.end();
    };
    const auto setAppIdTableCount=[](Tags& tags,const char* count) {
        for(auto table=tags.begin();table!=tags.end();++table) {
            if(table->code!=0||table->value!="TABLE")continue;
            auto end=std::find_if(std::next(table),tags.end(),[](const Tag& tag){return tag.code==0;});
            if(end==tags.end())return false;
            auto name=std::find_if(std::next(table),end,[](const Tag& tag){return tag.code==2;});
            if(name==end||name->value!="APPID")continue;
            auto entries=std::find_if(std::next(name),end,[](const Tag& tag){return tag.code==70;});
            if(entries==end)return false;entries->value=count;return true;
        }
        return false;
    };
    unsigned positives=0u,negatives=0u,retries=0u,quotas=0u,defaults=0u,ownerFaults=0u,aliases=0u,shared=0u,indexFaults=0u,dictionaryFaults=0u,liveModelFaults=0u,dependencyChains=0u,dependencyFaults=0u,sortEntsFaults=0u,ownerChainFaults=0u;
    unsigned appIdJoins=0u,appIdRefusals=0u,appIdRetries=0u;
    unsigned xdataJoins=0u,xdataRefusals=0u,xdataRetries=0u;
    for(bool binary:{false,true}){
        if(!write(path,doc,binary))return false;
        Probe plain;plain.active=false;dx_data absent;if(!plain.fileImport(path.string(),&absent,false)||A::legacyJoin(absent)||A::legacyJoinFailed(absent)
            ||A::legacyXDictionaryChainBound(absent))return false;
        ++defaults;
        Probe selected;if(!selected.enableLegacyDxfIdentityObservation())return false;dx_data chainData;
        if(!selected.fileImport(path.string(),&chainData,false))return false;
        if(!A::legacyJoin(chainData)||!A::legacyExtensionDictionaryBound(chainData)||!A::legacyXDictionaryChainBound(chainData)
            ||A::legacyXDictionaryChainFailed(chainData)||!A::legacyNoPromotion(chainData)) {
            const auto capture=A::legacyXDictionaryCaptureFlags(chainData);
            const auto discovery=A::legacyXDictionaryDiscovery(chainData);
            const auto ownerState=A::legacyXDictionaryOwnerState(chainData);
            std::cerr<<"selected AC1018 chain join failed (binary="<<binary<<") chains="
                <<A::legacyXDictionaryChainCount(chainData)<<" chainFailed="<<A::legacyXDictionaryChainFailed(chainData)
                <<" structuralFailed="<<capture[0]<<" resourceFailed="<<capture[1]<<" resourceProofFailed="<<capture[2]
                <<" rootOwner="<<discovery[0]<<" childOwner="<<discovery[1]<<" rootStructural="<<discovery[2]
                <<" childStructural="<<discovery[3]<<" rootResource="<<discovery[4]<<" childResource="<<discovery[5]
                <<" xrecordResource="<<discovery[6]<<" descendantEdges="<<discovery[7]
                <<" ownerStructural="<<discovery[8]<<" ownerResource="<<discovery[9]<<" blockRecords="<<discovery[10]
                <<" sortentsOwner="<<discovery[11]<<" sortentsTyped="<<discovery[12]<<" sortentsRaw="<<discovery[13]
                <<" ownerReplay="<<ownerState[0]<<" ownerProfile="<<ownerState[1]<<" ownerXdictSpan="<<ownerState[2]
                <<" ownerXdictSpanCount="<<ownerState[3]<<" ownerModelXdict="<<ownerState[4]
                <<" oldJoin="<<A::legacyJoin(chainData)<<" oldDictionary="<<A::legacyExtensionDictionaryBound(chainData)
                <<" noPromotion="<<A::legacyNoPromotion(chainData)<<'\n';
            return false;
        }
        ++dependencyChains;
        ++positives;
        Probe appIdProbe;
        if(!appIdProbe.enableWholeDxfSourceObservation()
            ||!appIdProbe.enableLegacyDxfIdentityObservation())return false;
        dx_data appIdData;
        if(!appIdProbe.fileImport(path.string(),&appIdData,false)
            ||!A::legacyBlockRecordAppIdJoinBound(appIdData)
            ||!A::legacyBlockRecordAppIdSingleJoinBound(appIdData)
            ||!A::legacyBlockRecordAppIdXdataJoinBound(appIdData)
            ||A::legacyBlockRecordAppIdJoinCount(appIdData)!=2u
            ||!A::legacyBlockRecordAppIdQuotaControls(appIdData)
            ||A::legacyBlockRecordAppIdCaptureFailed(appIdData)) {
            std::cerr<<"selected AC1018 BLOCK_RECORD APPID source join failed (binary="<<binary<<") joins="
                <<A::legacyBlockRecordAppIdJoinCount(appIdData)<<" captureFailed="
                <<A::legacyBlockRecordAppIdCaptureFailed(appIdData)<<'\n';
            return false;
        }
        ++appIdJoins;
        ++xdataJoins;
        for(unsigned fault=0u;fault<9u;++fault) {
            auto changed=doc;Probe invalid;
            if(fault==0u) {
                auto first=findRecord(changed,"APPID","A3");if(first==changed.end())return false;
                auto end=std::find_if(std::next(first),changed.end(),[](const Tag& tag){return tag.code==0;});
                if(end==changed.end())return false;changed.erase(first,end);
                if(!setAppIdTableCount(changed,"4"))return false;
            } else if(fault==1u) {
                auto first=findRecord(changed,"APPID","A3");if(first==changed.end())return false;
                auto end=std::find_if(std::next(first),changed.end(),[](const Tag& tag){return tag.code==0;});
                auto tableEnd=std::find_if(std::next(first),changed.end(),[](const Tag& tag){return tag.code==0&&tag.value=="ENDTAB";});
                if(end==changed.end()||tableEnd==changed.end())return false;
                Tags duplicate(first,end);
                auto handle=std::find_if(duplicate.begin(),duplicate.end(),[](const Tag& tag){return tag.code==5;});
                if(handle==duplicate.end())return false;handle->value="F00E";
                changed.insert(tableEnd,duplicate.begin(),duplicate.end());
                if(!setAppIdTableCount(changed,"6"))return false;
            } else if(fault==2u||fault==3u) {
                auto first=findRecord(changed,"APPID","A3");if(first==changed.end())return false;
                auto end=std::find_if(std::next(first),changed.end(),[](const Tag& tag){return tag.code==0;});
                auto value=std::find_if(std::next(first),end,[&](const Tag& tag){return tag.code==(fault==2u?330:2);});
                if(end==changed.end()||value==end)return false;value->value=fault==2u?"A8":"OTHER_APP";
            } else if(fault>=4u&&fault<=7u) {
                auto owner=findRecord(changed,"BLOCK_RECORD","A91");if(owner==changed.end())return false;
                auto end=std::find_if(std::next(owner),changed.end(),[](const Tag& tag){return tag.code==0;});
                auto handle=std::find_if(std::next(owner),end,[](const Tag& tag){return tag.code==1005;});
                if(end==changed.end()||handle==end)return false;
                if(fault==4u)handle->value="DEADB";
                else if(fault==5u)handle->value="A5";
                else if(fault==6u)handle->value="B0";
                else changed.insert(end,*handle);
            } else invalid.duplicateBlockTargetIdentity=true;
            if(!write(path,changed,binary)
                ||!invalid.enableWholeDxfSourceObservation()
                ||!invalid.enableLegacyDxfIdentityObservation())return false;
            dx_data rejected;
            if(!invalid.fileImport(path.string(),&rejected,false)
                ||!A::legacyBlockRecordAppIdXdataJoinAbsent(rejected)
                ||A::legacyBlockRecordAppIdCaptureFailed(rejected)) {
                std::cerr<<"BLOCK_RECORD EED group-1005 dependency fault was admitted or capture failed: "
                    <<fault<<" (binary="<<binary<<")\n";return false;
            }
            ++xdataRefusals;
            if(!write(path,doc,binary))return false;
            invalid.duplicateBlockTargetIdentity=false;
            dx_data retry;
            if(!invalid.fileImport(path.string(),&retry,false)
                ||!A::legacyBlockRecordAppIdXdataJoinBound(retry)
                ||A::legacyBlockRecordAppIdCaptureFailed(retry)) {
                std::cerr<<"BLOCK_RECORD EED group-1005 same-adapter retry failed: "
                    <<fault<<" (binary="<<binary<<")\n";return false;
            }
            ++xdataRetries;
        }
        for(unsigned fault=0u;fault<8u;++fault) {
            auto changed=doc;
            if(fault==0u) {
                auto first=findRecord(changed,"APPID","A6");if(first==changed.end())return false;
                auto end=std::find_if(std::next(first),changed.end(),[](const Tag& tag){return tag.code==0;});
                if(end==changed.end())return false;changed.erase(first,end);
                if(!setAppIdTableCount(changed,"4"))return false;
            } else if(fault==1u) {
                auto first=findRecord(changed,"APPID","A6");if(first==changed.end())return false;
                auto end=std::find_if(std::next(first),changed.end(),[](const Tag& tag){return tag.code==0;});
                auto tableEnd=std::find_if(std::next(first),changed.end(),[](const Tag& tag){return tag.code==0&&tag.value=="ENDTAB";});
                if(end==changed.end()||tableEnd==changed.end())return false;
                Tags duplicate(first,end);
                auto handle=std::find_if(duplicate.begin(),duplicate.end(),[](const Tag& tag){return tag.code==5;});
                if(handle==duplicate.end())return false;handle->value="F00D";
                changed.insert(tableEnd,duplicate.begin(),duplicate.end());
                if(!setAppIdTableCount(changed,"6"))return false;
            } else if(fault==2u) {
                auto first=findRecord(changed,"APPID","A6");if(first==changed.end())return false;
                auto end=std::find_if(std::next(first),changed.end(),[](const Tag& tag){return tag.code==0;});
                auto owner=std::find_if(std::next(first),end,[](const Tag& tag){return tag.code==330;});
                if(end==changed.end()||owner==end)return false;owner->value="A8";
            } else if(fault==3u) {
                auto first=findRecord(changed,"APPID","A6");if(first==changed.end())return false;
                auto end=std::find_if(std::next(first),changed.end(),[](const Tag& tag){return tag.code==0;});
                auto name=std::find_if(std::next(first),end,[](const Tag& tag){return tag.code==2;});
                if(end==changed.end()||name==end)return false;name->value="OTHER_APP";
            } else if(fault==4u) {
                auto first=findRecord(changed,"APPID","A5");if(first==changed.end())return false;
                auto end=std::find_if(std::next(first),changed.end(),[](const Tag& tag){return tag.code==0;});
                if(end==changed.end())return false;changed.erase(first,end);
                if(!setAppIdTableCount(changed,"4"))return false;
            } else if(fault==5u) {
                auto first=findRecord(changed,"APPID","A5");if(first==changed.end())return false;
                auto end=std::find_if(std::next(first),changed.end(),[](const Tag& tag){return tag.code==0;});
                auto tableEnd=std::find_if(std::next(first),changed.end(),[](const Tag& tag){return tag.code==0&&tag.value=="ENDTAB";});
                if(end==changed.end()||tableEnd==changed.end())return false;
                Tags duplicate(first,end);
                auto handle=std::find_if(duplicate.begin(),duplicate.end(),[](const Tag& tag){return tag.code==5;});
                if(handle==duplicate.end())return false;handle->value="F00D";
                changed.insert(tableEnd,duplicate.begin(),duplicate.end());
                if(!setAppIdTableCount(changed,"6"))return false;
            } else if(fault==6u) {
                auto first=findRecord(changed,"APPID","A5");if(first==changed.end())return false;
                auto end=std::find_if(std::next(first),changed.end(),[](const Tag& tag){return tag.code==0;});
                auto owner=std::find_if(std::next(first),end,[](const Tag& tag){return tag.code==330;});
                if(end==changed.end()||owner==end)return false;owner->value="A8";
            } else {
                auto first=findRecord(changed,"APPID","A5");if(first==changed.end())return false;
                auto end=std::find_if(std::next(first),changed.end(),[](const Tag& tag){return tag.code==0;});
                auto name=std::find_if(std::next(first),end,[](const Tag& tag){return tag.code==2;});
                if(end==changed.end()||name==end)return false;name->value="WRONG_DYNAMIC_GUID_APP";
            }
            if(!write(path,changed,binary))return false;
            Probe invalid;
            if(!invalid.enableWholeDxfSourceObservation()
                ||!invalid.enableLegacyDxfIdentityObservation())return false;
            dx_data rejected;
            if(!invalid.fileImport(path.string(),&rejected,false)
                ||!A::legacyBlockRecordAppIdJoinAbsent(rejected)
                ||A::legacyBlockRecordAppIdCaptureFailed(rejected)) {
                std::cerr<<"BLOCK_RECORD APPID dependency fault was admitted or capture failed: "
                    <<fault<<" (binary="<<binary<<")\n";return false;
            }
            ++appIdRefusals;
            if(!write(path,doc,binary))return false;
            dx_data retry;
            if(!invalid.fileImport(path.string(),&retry,false)
                ||!A::legacyBlockRecordAppIdJoinBound(retry)
                ||A::legacyBlockRecordAppIdCaptureFailed(retry)) {
                std::cerr<<"BLOCK_RECORD APPID same-adapter retry failed: "<<fault
                    <<" (binary="<<binary<<")\n";return false;
            }
            ++appIdRetries;
        }
        if(!write(path,outputDoc,binary))return false;
        dx_data data;
        if(!selected.fileImport(path.string(),&data,false)||!A::legacyJoin(data)
            ||A::legacyXDictionaryChainBound(data))return false;
        const auto outputPath=path.parent_path()/(binary?"legacy-xdictionary-live-model-binary.dxf":"legacy-xdictionary-live-model-ascii.dxf");
        dx_iface exporter;
        auto liveDictionary=std::find_if(data.dictionaries.begin(),data.dictionaries.end(),
            [](const DRW_Dictionary& value){return value.handle==0xD0u;});
        if(liveDictionary==data.dictionaries.end())return false;
        const auto ownerFlag=liveDictionary->hardOwner;
        const std::string sentinel="existing destination must survive preflight refusal";
        {
            std::ofstream destination(outputPath,std::ios::binary|std::ios::trunc);
            if(!destination)return false;
            destination.write(sentinel.data(),static_cast<std::streamsize>(sentinel.size()));
            if(!destination)return false;
        }
        liveDictionary->hardOwner=0;
        const bool refused=!exporter.fileExport(outputPath.string(),DRW::AC1018,binary,&data,false,false);
        liveDictionary->hardOwner=ownerFlag;
        if(!refused)return false;
        {
            std::ifstream destination(outputPath,std::ios::binary);
            const std::string contents((std::istreambuf_iterator<char>(destination)),
                std::istreambuf_iterator<char>());
            if(contents!=sentinel)return false;
        }
        if(!exporter.fileExport(outputPath.string(),DRW::AC1018,binary,&data,false,false))return false;
        Probe outputReader;outputReader.enableLegacyDxfIdentityObservation();dx_data outputData;
        if(!outputReader.fileImport(outputPath.string(),&outputData,false)) {
            std::cerr<<"selected XDICTIONARY output readback failed (binary="<<binary<<")\n";return false;
        }
        if(!A::legacyExtensionDictionaryOutput(outputData)) {
            std::cerr<<"selected XDICTIONARY output proof failed (binary="<<binary<<") blockRecords="
                <<outputData.blockRecords.size()<<" dictionaries="<<outputData.dictionaries.size()<<'\n';
            for(const auto& record:outputData.blockRecords)std::cerr<<"  BLOCK_RECORD "<<record.name
                <<" handle="<<record.handle<<" xdict="<<record.xDictHandle<<'\n';
            for(const auto& dictionary:outputData.dictionaries)std::cerr<<"  DICTIONARY handle="
                <<dictionary.handle<<" owner="<<dictionary.parentHandle<<" hard="
                <<dictionary.hardOwner<<" clone="<<dictionary.cloning<<'\n';
            return false;
        }
        liveDictionary=std::find_if(data.dictionaries.begin(),data.dictionaries.end(),
            [](const DRW_Dictionary& value){return value.handle==0xD0u;});
        if(liveDictionary==data.dictionaries.end())return false;
        liveDictionary->hardOwner=0;
        if(A::legacyExtensionDictionaryBound(data))return false;
        liveDictionary->hardOwner=ownerFlag;
        if(!A::legacyExtensionDictionaryBound(data))return false;
        ++liveModelFaults;
        data.dictionaries.push_back(*liveDictionary);
        if(A::legacyExtensionDictionaryBound(data))return false;
        data.dictionaries.pop_back();
        if(!A::legacyExtensionDictionaryBound(data))return false;
        ++liveModelFaults;
        // Exercise the index directly as well as the independently authenticated
        // callback: mutated callback data is rejected before index construction.
        const auto* physical=A::legacyCensus(data);if(!physical)return false;
        const auto qualified=[](const S& candidate) {
            dx_legacy_identity_join::Index index;
            return dx_legacy_identity_join::buildIndex(candidate,index)
                &&dx_legacy_identity_join::record(candidate,index,0xA1u,S::Kind::Entity,S::Section::Blocks,"ATTDEF",1u);
        };
        if(!qualified(*physical))return false;
        for(unsigned fault=0u;fault<5u;++fault){auto changed=*physical;
            auto at=std::find_if(changed.candidates.begin(),changed.candidates.end(),[](const auto& c){return c.scope==S::Scope::Outer&&c.lexeme=="A1";});
            if(at==changed.candidates.end())return false;
            if(fault==0u)++at->repeatedAdmissions;
            if(fault==1u)at->repeatedAdmissions=0u;
            if(fault==2u){auto other=*at;other.lexeme="000000000000000A1";other.rawValue=binary?"":other.lexeme;
                other.record=S::NoIndex;other.scope=S::Scope::Unframed;changed.candidates.push_back(std::move(other));}
            if(fault==3u){const auto record=at->record;const auto pos=changed.records[record].firstCandidate+changed.records[record].candidates;
                auto other=*at;other.lexeme="DAB";other.rawValue=binary?"":other.lexeme;
                changed.candidates.insert(changed.candidates.begin()+static_cast<std::ptrdiff_t>(pos),std::move(other));
                ++changed.records[record].candidates;for(auto& row:changed.records)if(&row!=&changed.records[record]&&row.firstCandidate>=pos)++row.firstCandidate;}
            if(fault==4u){auto other=std::find_if(changed.candidates.begin(),changed.candidates.end(),[](const auto& c){return c.scope==S::Scope::Outer&&c.lexeme=="B1";});
                if(other==changed.candidates.end())return false;other->lexeme="000000000000000A1";other->rawValue=binary?"":other->lexeme;}
            if(qualified(changed))return false;++indexFaults;
        }
        const auto usage=A::legacyJoinUsage(data);
        (void)selected.fileImport(path.string(),&data,false);
        if(A::legacyJoin(data))return false;
        dx_data fresh;if(!selected.fileImport(path.string(),&fresh,false)||!A::legacyJoin(fresh))return false;++retries;
        for(unsigned i=0;i<2u;++i)for(bool exact:{false,true}) {
            Probe bounded;bounded.enableLegacyDxfIdentityObservation();dx_data d;if(!usage[i])return false;
            A::legacyJoinLimit(d,i,usage[i]-(exact?0u:1u));
            if(!bounded.fileImport(path.string(),&d,false)||A::legacyJoinFailed(d)==exact||A::legacyJoin(d)!=exact)return false;++quotas;
            if(!exact){dx_data retry;if(!bounded.fileImport(path.string(),&retry,false)||!A::legacyJoin(retry))return false;++retries;}
        }
        for(unsigned fault=1;fault<=13;++fault){
            Probe bad;bad.enableLegacyDxfIdentityObservation();bad.fault=fault;dx_data d;
            if(!bad.fileImport(path.string(),&d,false)||A::legacyJoin(d)) {std::cerr<<"fault="<<fault<<'\n';return false;}++negatives;
            bad.fault=0u;dx_data retry;if(!bad.fileImport(path.string(),&retry,false)||!A::legacyJoin(retry))return false;++retries;
        }
        {
            auto variant=doc;const auto at=std::find_if(variant.begin(),variant.end(),[](const Tag& t){return t.code==0&&t.value=="ENDBLK";});
            if(at==variant.end())return false;
            variant.insert(at,{{0,"POINT"},{5,"000000000000000A1"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},
                {100,"AcDbPoint"},{10,"0"},{20,"0"},{30,"0"}});
            if(!write(path,variant,binary))return false;
            Probe collision;collision.enableLegacyDxfIdentityObservation();dx_data rejected;
            if(!collision.fileImport(path.string(),&rejected,false)||A::legacyJoin(rejected))return false;++aliases;
            if(!write(path,doc,binary))return false;dx_data retry;
            if(!collision.fileImport(path.string(),&retry,false)||!A::legacyJoin(retry))return false;++retries;
        }
        {
            auto variant=doc;const auto at=std::find_if(variant.begin(),variant.end(),[](const Tag& t){return t.code==0&&t.value=="ENDBLK";});
            if(at==variant.end())return false;Tags extra;
            for(unsigned n=0u;n<15u;++n){auto child=attr;std::ostringstream handle;handle<<std::hex<<std::uppercase<<(0xF0u+n);
                child.front().value=handle.str();extra.push_back({0,"ATTDEF"});extra.insert(extra.end(),child.begin(),child.end());}
            variant.insert(at,extra.begin(),extra.end());if(!write(path,variant,binary))return false;
            Probe many;many.enableLegacyDxfIdentityObservation();dx_data baseline;
            if(!many.fileImport(path.string(),&baseline,false)||A::legacyJoinCount(baseline)!=16u)return false;
            const auto bound=A::legacyJoinUsage(baseline);if(!bound[1])return false;dx_data limited;A::legacyJoinLimit(limited,1u,bound[1]-1u);
            if(!many.fileImport(path.string(),&limited,false)||!A::legacyJoinFailed(limited)||A::legacyJoinCount(limited))return false;
            dx_data corrected;if(!many.fileImport(path.string(),&corrected,false)||A::legacyJoinCount(corrected)!=16u)return false;
            if(!write(path,doc,binary))return false;++shared;++retries;
        }
        for(const std::string target:{"ATTDEF","BLOCK_RECORD","BLOCK"}) {
            auto changed=doc;std::string current;bool altered=false;
            for(auto& tag:changed){if(tag.code==0)current=tag.value;
                if(!altered&&current==target&&tag.code==330){tag.value="B8";altered=true;}}
            if(!altered||!write(path,changed,binary))return false;
            Probe wrong;wrong.enableLegacyDxfIdentityObservation();dx_data rejected;
            if(!wrong.fileImport(path.string(),&rejected,false)||A::legacyJoin(rejected))return false;++ownerFaults;
            if(!write(path,doc,binary))return false;dx_data retry;
            if(!wrong.fileImport(path.string(),&retry,false)||!A::legacyJoin(retry))return false;++retries;
        }
        for(unsigned fault=0u;fault<3u;++fault) {
            auto changed=doc;
            auto begin=changed.end();
            for(auto at=changed.begin();at!=changed.end();++at)
                if(at->code==0&&at->value=="DICTIONARY"&&std::next(at)!=changed.end()
                    &&std::next(at)->code==5&&std::next(at)->value=="D0") {begin=at;break;}
            if(begin==changed.end())return false;
            if(fault==0u) {
                auto end=std::find_if(std::next(begin),changed.end(),[](const Tag& t){return t.code==0;});
                if(end==changed.end())return false;changed.erase(begin,end);
            } else if(fault==1u) {
                auto owner=std::find_if(std::next(begin),changed.end(),[](const Tag& t){return t.code==330;});
                if(owner==changed.end())return false;owner->value="B9";
            } else begin->value="XRECORD";
            if(!write(path,changed,binary))return false;
            Probe wrong;wrong.enableLegacyDxfIdentityObservation();dx_data rejected;
            if(!wrong.fileImport(path.string(),&rejected,false)||A::legacyJoin(rejected)||!A::legacyJoinFailed(rejected))return false;
            if(!write(path,doc,binary))return false;dx_data retry;
            if(!wrong.fileImport(path.string(),&retry,false)||!A::legacyJoin(retry)||!A::legacyExtensionDictionaryBound(retry))return false;
            ++dictionaryFaults;++retries;
        }
        for(unsigned fault=0u;fault<6u;++fault) {
            auto changed=doc;
            auto first=changed.end();
            for(auto it=changed.begin();it!=changed.end();++it)
                if(it->code==0&&it->value==(fault<=2u?"DICTIONARY":"XRECORD")
                    &&std::next(it)!=changed.end()&&std::next(it)->code==5
                    &&std::next(it)->value==(fault<=2u?"D2":"D3")) {first=it;break;}
            if(first==changed.end())return false;
            if(fault==0u) {auto end=std::find_if(std::next(first),changed.end(),[](const Tag& t){return t.code==0;});
                if(end==changed.end())return false;changed.erase(first,end);}
            else if(fault==1u) {auto owner=std::find_if(std::next(first),changed.end(),[](const Tag& t){return t.code==330;});
                if(owner==changed.end())return false;owner->value="C1";}
            else if(fault==2u) {auto target=std::find_if(std::next(first),changed.end(),[](const Tag& t){return t.code==360;});
                if(target==changed.end())return false;target->code=350;}
            else if(fault==3u) {auto end=std::find_if(std::next(first),changed.end(),[](const Tag& t){return t.code==0;});
                if(end==changed.end())return false;changed.erase(first,end);}
            else if(fault==4u) {auto owner=std::find_if(std::next(first),changed.end(),[](const Tag& t){return t.code==330;});
                if(owner==changed.end())return false;owner->value="D1";}
            else {auto payload=std::find_if(std::next(first),changed.end(),[](const Tag& t){return t.code==1070;});
                if(payload==changed.end())return false;payload->value="2";}
            if(!write(path,changed,binary))return false;
            Probe invalid;invalid.enableLegacyDxfIdentityObservation();dx_data rejected;
            if(!invalid.fileImport(path.string(),&rejected,false)) {std::cerr<<"nested dictionary fault control did not parse: "<<fault<<'\n';return false;}
            if(A::legacyXDictionaryChainBound(rejected)){std::cerr<<"nested dictionary fault control was admitted: "<<fault<<'\n';return false;}
            if(!write(path,doc,binary))return false;dx_data retry;
            if(!invalid.fileImport(path.string(),&retry,false)||!A::legacyXDictionaryChainBound(retry)
                ||A::legacyXDictionaryChainFailed(retry))return false;
            ++dependencyFaults;++retries;
        }
        for(unsigned fault=0u;fault<4u;++fault) {
            auto changed=doc;
            if(fault==0u) {
                auto first=std::find_if(changed.begin(),changed.end(),[](const Tag& t){return t.code==0&&t.value=="SORTENTSTABLE";});
                if(first==changed.end())return false;
                auto end=std::find_if(std::next(first),changed.end(),[](const Tag& t){return t.code==0;});
                if(end==changed.end())return false;changed.erase(first,end);
            } else if(fault==1u||fault==2u) {
                auto first=std::find_if(changed.begin(),changed.end(),[](const Tag& t){return t.code==0&&t.value=="SORTENTSTABLE";});
                if(first==changed.end())return false;
                auto body=std::find_if(std::next(first),changed.end(),[](const Tag& t){return t.code==100&&t.value=="AcDbSortentsTable";});
                if(body==changed.end())return false;
                auto owner=fault==1u?std::find_if(std::next(body),changed.end(),[](const Tag& t){return t.code==330;})
                    :std::find_if(std::next(first),body,[](const Tag& t){return t.code==330;});
                if(owner==changed.end())return false;owner->value="D2";
            } else {
                auto first=changed.end();
                for(auto at=changed.begin();at!=changed.end();++at)
                    if(at->code==0&&at->value=="DICTIONARY"&&std::next(at)!=changed.end()
                        &&std::next(at)->code==5&&std::next(at)->value=="D1") {first=at;break;}
                if(first==changed.end())return false;
                auto target=std::find_if(std::next(first),changed.end(),[](const Tag& t){return t.code==360;});
                if(target==changed.end())return false;target->code=350;
            }
            if(!write(path,changed,binary))return false;
            Probe invalid;invalid.enableLegacyDxfIdentityObservation();dx_data rejected;
            if(!invalid.fileImport(path.string(),&rejected,false)) {std::cerr<<"SORTENTSTABLE fault control did not parse: "<<fault<<'\n';return false;}
            if(A::legacyXDictionaryChainBound(rejected)){std::cerr<<"SORTENTSTABLE fault control was admitted: "<<fault<<'\n';return false;}
            if(!write(path,doc,binary))return false;dx_data retry;
            if(!invalid.fileImport(path.string(),&retry,false)||!A::legacyXDictionaryChainBound(retry)
                ||A::legacyXDictionaryChainFailed(retry))return false;
            ++sortEntsFaults;++retries;
        }
        for(unsigned fault=0u;fault<2u;++fault) {
            auto changed=doc;auto first=changed.end();
            for(auto at=changed.begin();at!=changed.end();++at)
                if(at->code==0&&at->value=="BLOCK_RECORD"&&std::next(at)!=changed.end()
                    &&std::next(at)->code==5&&std::next(at)->value=="C1") {first=at;break;}
            if(first==changed.end())return false;
            auto end=std::find_if(std::next(first),changed.end(),[](const Tag& t){return t.code==0;});
            if(end==changed.end())return false;
            const int targetCode=fault==0u?330:360;
            auto target=std::find_if(std::next(first),end,[targetCode](const Tag& t){return t.code==targetCode;});
            if(target==end)return false;
            target->value=fault==0u?"B8":"D0";
            if(!write(path,changed,binary))return false;
            Probe invalid;invalid.enableLegacyDxfIdentityObservation();dx_data rejected;
            if(!invalid.fileImport(path.string(),&rejected,false)
                ||A::legacyXDictionaryChainBound(rejected)) {
                std::cerr<<"BLOCK_RECORD XDICTIONARY owner fault was admitted: "<<fault<<'\n';return false;
            }
            if(!write(path,doc,binary))return false;dx_data retry;
            if(!invalid.fileImport(path.string(),&retry,false)||!A::legacyXDictionaryChainBound(retry)
                ||A::legacyXDictionaryChainFailed(retry))return false;
            ++ownerChainFaults;++retries;
        }
    }
    std::cout<<"AC1018 selected origin join profiles="<<positives<<" xdictionary chains="<<dependencyChains
        <<" chain faults="<<dependencyFaults<<" SORTENTSTABLE faults="<<sortEntsFaults
        <<" owner-chain faults="<<ownerChainFaults
        <<" BLOCK_RECORD APPID joins="<<appIdJoins<<" refusal controls="<<appIdRefusals
        <<" same-adapter retries="<<appIdRetries
        <<" BLOCK_RECORD APPID/XDATA joins="<<xdataJoins<<" refusal controls="<<xdataRefusals
        <<" same-adapter retries="<<xdataRetries
        <<" live-model-controls="<<liveModelFaults<<" default="<<defaults
        <<" negative="<<negatives<<" quota="<<quotas<<" owner="<<ownerFaults<<" aliases="<<aliases
        <<" shared="<<shared<<" index="<<indexFaults<<" dictionary="<<dictionaryFaults<<" retries="<<retries<<'\n';
    return positives==2u&&dependencyChains==2u&&dependencyFaults==12u&&sortEntsFaults==8u&&ownerChainFaults==4u
        &&appIdJoins==2u&&appIdRefusals==16u&&appIdRetries==16u
        &&xdataJoins==2u&&xdataRefusals==18u&&xdataRetries==18u&&liveModelFaults==4u&&defaults==2u
        &&negatives==26u&&quotas==8u&&ownerFaults==6u&&aliases==2u&&shared==2u&&indexFaults==10u
        &&dictionaryFaults==6u&&retries==72u;
}
