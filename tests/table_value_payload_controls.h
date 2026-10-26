// SPDX-License-Identifier: GPL-2.0-or-later
// Authored structured Value envelopes, distinct from legacy TABLE strings.
Tags tableValuePayloadDocument(const std::string& version,bool block,int type,int flags=0) {
    auto tags=tableProfileDocument(version,block,false,false,1u);
    const auto first=blockField(tags,"501",171);
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(first),{{11,"1"},{21,"2"},{31,"3"},{93,"17"},{94,"33"}});
    const auto text=blockField(tags,"501",1);
    Tags value{{91,"19"},{92,"23"},{140,"1.25"},{301,"CELL_VALUE"},{93,std::to_string(flags)},{90,std::to_string(type)}};
    if(!(flags&1))switch(type) {
    case 0:case 1:value.push_back({91,"-17"});break;
    case 2:value.push_back({140,"-2.5"});break;
    case 4:value.insert(value.end(),{{1,"payload"},{2,"-é"}});break;
    case 8:value.insert(value.end(),{{92,"8"},{310,"00112233"},{310,"AABBCCDD"}});break;
    case 16:value.insert(value.end(),{{92,"16"},{11,"1.5"},{21,"-2.5"}});break;
    case 32:value.insert(value.end(),{{92,"24"},{11,"1.5"},{21,"-2.5"},{31,"3.5"}});break;
    case 64:value.push_back({330,"FFEE1122"});break;
    case 512:value.insert(value.end(),{{92,"3"},{310,"0123FF"}});break;
    default:break;
    }
    value.insert(value.end(),{{94,"8"},{300,"value-format-é"},{302,"display-cache-é"},{304,"ACVALUE_END"}});
    tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(text));
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(text),value.begin(),value.end());return tags;
}
struct TableValueOperation {const char* name;Tags prefix;Tag target;std::size_t extra;};
std::vector<TableValueOperation> tableValueOperations() {
    const auto prefix=[](int type) {return Tags{{5,"501"},{330,"B0"},{100,"AcDbEntity"},{8,"0"},
        {100,"AcDbBlockReference"},{2,"*T"},{10,"0"},{20,"0"},{100,"AcDbTable"},
        {91,"1"},{92,"1"},{141,"1"},{142,"1"},{171,"1"},{301,"CELL_VALUE"},{93,"0"},{90,std::to_string(type)}};};
    auto text=prefix(4);text.push_back({1,"old"});
    auto format=text;format.push_back({94,"0"});
    auto cache=format;cache.push_back({300,"F"});
    auto close=cache;close.push_back({302,"cache"});
    auto raw=prefix(8);raw.insert(raw.end(),{{92,"8"},{310,"00112233"}});
    auto binary=prefix(8);binary.insert(binary.end(),{{92,"8"},{310,"00112233AABBCCDD"},{94,"0"},{300,"F"},{302,"cache"}});
    return {{"payload append",text,{2,std::string(512u,'x')},512u},
        {"format",format,{300,std::string(512u,'f')},512u},
        {"cache",cache,{302,std::string(512u,'c')},512u},
        {"raw bytes",raw,{310,"AABBCCDD"},4u},
        {"string close",close,{304,"ACVALUE_END"},3u},
        {"binary close",binary,{304,"ACVALUE_END"},13u}};
}
std::size_t tableValueOwned(const DRW_Table& model) {
    std::size_t bytes=model.name.size()+model.layer.size();
    for(const auto& row:model.m_content.m_rows)for(const auto& cell:row.m_cells)for(const auto& c:cell.m_contents) {
        const auto& v=c.m_value;bytes+=c.m_text.size()+v.m_formatString.size()+v.m_valueString.size()+v.m_rawData.size();
        if(v.m_value.type()==DRW_Variant::STRING)bytes+=v.m_value.content.s->size();
        if(v.m_value.type()==DRW_Variant::BINARY)bytes+=v.m_value.binary()->size();
    }
    return bytes;
}
bool tableValueOperation(const std::filesystem::path& path,const TableValueOperation& operation,bool binary,bool under,
                         long offset,unsigned& reached,unsigned& unreached) {
    auto tags=operation.prefix;tags.push_back(operation.target);if(!write(path,tags,binary))return false;
    auto budget=std::make_shared<DxfReadRetainedBudget>(1024u*1024u);DRW_Table model;
    {
        std::ifstream input(path,std::ios::binary);if(binary)input.seekg(22);
        std::unique_ptr<dxfReader> reader=binary?std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
            :std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
        reader->setVersion("AC1032",true);reader->setRetainedByteBudget(budget);dxfTable::Decoder decoder(model);int code=0;
        for(std::size_t i=0u;i<operation.prefix.size();++i)if(!reader->readRec(&code)||!decoder.parseCode(code,reader))return false;
        if(!reader->readRec(&code))return false;
        const auto available=operation.extra-(under?1u:0u);const auto pressure=offset<0?budget->limit()-budget->used()-available:0u;
        if(!budget->reserve(pressure))return false;
        const auto owned=DrwEntityEncodeTestAccess::tableRetained(model),failures=symbolFailedAllocations;const DRW_Table before(model);
        bool accepted=false;try {SymbolAllocationFailure fail(offset);accepted=decoder.parseCode(code,reader);}catch(const std::bad_alloc&) {}
        budget->release(pressure);
        if(offset<0&&(accepted==under||budget->exceeded()!=under)) {
            std::cerr<<"TABLE value admission "<<operation.name<<" binary="<<binary<<" under="<<under<<" accepted="<<accepted<<'\n';return false;
        }
        if(!accepted&&(DrwEntityEncodeTestAccess::tableRetained(model)!=owned||!DxIfaceTableSourceTestAccess::sameTableBody(before,model))) {
            std::cerr<<"TABLE value failed operation changed model/owner "<<operation.name<<" offset="<<offset<<'\n';return false;
        }
        if(offset>=0) {if(symbolFailedAllocations!=failures)++reached;else ++unreached;}
    }
    if(DrwEntityEncodeTestAccess::tableRetained(model)!=tableValueOwned(model)||budget->used()!=tableValueOwned(model)) {
        std::cerr<<"TABLE value owner teardown "<<operation.name<<" owned="<<DrwEntityEncodeTestAccess::tableRetained(model)<<" payload="<<tableValueOwned(model)<<" budget="<<budget->used()<<'\n';return false;
    }
    model=DRW_Table{};return budget->used()==0u;
}
bool tableValueNodeBoundaries(const std::filesystem::path& path,unsigned& count) {
    const auto operations=tableValueOperations();
    const int types[]={1,2,4,16,64};
    const Tag targets[]={{91,"-17"},{140,"-2.5"},{1,"value"},{11,"1.25"},{330,"F00"}};
    for(unsigned which=0u;which<6u;++which)for(bool binary:{false,true})for(std::size_t nodes:{0u,1u}) {
        auto prefix=which==5u?operations.back().prefix:operations.front().prefix;
        if(which!=5u){prefix.pop_back();prefix.back().value=std::to_string(types[which]);}
        auto tags=prefix;tags.push_back(which==5u?Tag{304,"ACVALUE_END"}:targets[which]);if(!write(path,tags,binary))return false;
        std::ifstream input(path,std::ios::binary);if(binary)input.seekg(22);
        auto budget=std::make_shared<DxfReadRetainedBudget>(1024u*1024u);DRW_Table model;
        std::unique_ptr<dxfReader> reader=binary?std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
            :std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
        reader->setVersion("AC1032",true);reader->setRetainedByteBudget(budget);dxfTable::Decoder decoder(model);int code=0;
        for(std::size_t i=0u;i<prefix.size();++i)if(!reader->readRec(&code)||!decoder.parseCode(code,reader))return false;
        if(!reader->readRec(&code))return false;
        reader->setTypedVariantNodeBudget(nodes);const DRW_Table before(model);
        if(decoder.parseCode(code,reader)!=(nodes==1u)
            ||(!nodes&&!DxIfaceTableSourceTestAccess::sameTableBody(before,model)))return false;
        ++count;
    }
    return true;
}
bool tableValueBoundaryImports(const std::filesystem::path& path,unsigned& refusals,unsigned& associated,unsigned& aligned) {
    const auto after=[](const Tags& tags,int code) {auto at=blockField(tags,"501",301)+1u;while(at<tags.size()&&tags[at].code!=code)++at;return at;};
    for(bool binary:{false,true})for(bool block:{false,true}) {
        const auto good=tableValuePayloadDocument("AC1032",block,1);
        const auto retry=[&](dx_iface& reader) {dx_data fixed;return write(path,good,binary)&&reader.fileImport(path.string(),&fixed,false)
            &&fixed.tables.size()==1u&&fixed.tables.front().m_semanticContentComplete;};
        for(unsigned fault=0u;fault<16u;++fault) {
            auto tags=tableValuePayloadDocument("AC1032",block,fault>=4u&&fault<=7u?8:1);
            bool accepted=true;const auto marker=blockField(tags,"501",301);
            switch(fault) {
            case 0:tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(after(tags,304)));break;
            case 1:{const auto at=after(tags,90);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),tags[at]);break;}
            case 2:tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(after(tags,91)),{{140,"99"}});break;
            case 3:tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(marker+1u),{{100,"AcDbEntity"},{330,"F00"}});break;
            case 4:{const auto first=after(tags,310);tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(first+1u));break;}
            case 5:tags[after(tags,92)].value="7";accepted=false;break;
            case 6:tags[after(tags,92)].value="-1";accepted=false;break;
            case 7:tags[after(tags,92)].value=std::to_string(DRW::kMaxDxfBinaryPayloadBytes+1u);accepted=false;break;
            case 8:tags=tableValuePayloadDocument("AC1032",block,64);tags[after(tags,330)].value="100000000";accepted=false;break;
            case 9:tags[after(tags,94)].value="32";break;
            case 10:tags[after(tags,93)].value="4";break;
            case 11:tags[after(tags,93)].value="1";break;
            case 12:{const auto units=after(tags,94),cache=after(tags,302);const auto value=tags[cache];tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(cache));tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(units),value);break;}
            case 13:tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(after(tags,90)));break;
            case 14:tags[after(tags,90)].value="128";break;
            default:tags=tableValuePayloadDocument("AC1032",block,32);tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(after(tags,21)));break;
            }
            dx_iface reader;dx_data data;if(!write(path,tags,binary)||reader.fileImport(path.string(),&data,false)!=accepted
                ||(accepted&&(data.tables.size()!=1u||data.tables.front().m_semanticContentComplete
                    ||data.tables.front().parentHandle!=(block?0xB3u:0xB0u)))||!retry(reader)) {
                std::cerr<<"TABLE value malformed fault="<<fault<<" binary="<<binary<<" block="<<block<<'\n';return false;
            }
            ++refusals;
        }
        // A real suppressed string Value belongs to the block content, not to
        // a fabricated text-content/ATTDEF slot.
        auto tags=tableValuePayloadDocument("AC1032",block,4,1);
        tags[blockField(tags,"501",171)].value="2";
        const auto at=blockField(tags,"501",301);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{340,"F00"},{144,"1.5"}});
        tags[after(tags,94)].value="0";tags[after(tags,300)].value="";tags[after(tags,302)].value="";
        dx_iface reader;dx_data data;if(!write(path,tags,binary)||!reader.fileImport(path.string(),&data,false)||!data.tables.front().m_semanticContentComplete)return false;
        const auto& cell=data.tables.front().m_content.m_rows[0].m_cells[0];
        if(cell.m_type!=2||cell.m_blockHandle!=0xF00u||cell.m_contents.size()!=1u||!cell.m_attributes.empty())return false;
        const auto& content=cell.m_contents.front();
        if(content.m_type!=4||content.m_handle!=0xF00u||content.m_value.m_dataType!=4||content.m_value.m_formatFlags!=1
            ||content.m_value.m_unitType||content.m_value.m_value.type()!=DRW_Variant::INVALID||!content.m_text.empty())return false;
        ++associated;
        // Recover at the next physical cell after an unclosed Value, and keep
        // a following 3D LINE aligned without turning the TABLE complete.
        tags=good;tags[blockField(tags,"501",92)].value="2";
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(blockField(tags,"501",171)),{{142,"1"}});
        const auto close=after(tags,304);tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(close));
        appendBlockFields(tags,"501",{{171,"1"},{1,"next-cell"}});
        auto end=slot(tags,5,"513")+1u;while(end<tags.size()&&tags[end].code!=0)++end;
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(end),{{0,"LINE"},{5,"701"},{330,block?"B3":"B0"},{100,"AcDbEntity"},{8,"0"},
            {100,"AcDbLine"},{10,"1"},{20,"2"},{30,"7"},{11,"4"},{21,"5"},{31,"10"}});
        dx_data next;if(!write(path,tags,binary)||!reader.fileImport(path.string(),&next,false)||next.tables.front().m_semanticContentComplete
            ||next.tables.front().m_content.m_rows[0].m_cells[1].m_contents[0].m_text!="next-cell")return false;
        const DRW_Line* line=nullptr;
        const auto find=[&](const dx_ifaceBlock* scope){for(const auto* e:scope->ent)if(e->handle==0x701u)line=dynamic_cast<const DRW_Line*>(e);};
        find(next.mBlock);for(const auto* scope:next.blocks)find(scope);
        if(!line||line->basePoint.z!=7.0||line->secPoint.z!=10.0||!retry(reader))return false;
        ++aligned;
    }
    return true;
}
bool tableValuePayloadTests(const std::filesystem::path& dir) {
    const auto path=dir/"table-value-payload.dxf";unsigned profiles=0u,markers=0u,bounds=0u,reached=0u,unreached=0u,nodes=0u,refusals=0u,associated=0u,aligned=0u;
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool block:{false,true})
        for(int type:{0,1,2,4,8,16,32,64,512}) {
            dx_iface reader;dx_data data;
            if(!writeBlockBinding(path,tableValuePayloadDocument(version,block,type),encoding)||!reader.fileImport(path.string(),&data,false)
                ||data.tables.size()!=1u){std::cerr<<"TABLE payload import type="<<type<<" version="<<version<<" encoding="<<encoding<<" block="<<block<<'\n';return false;}
            const auto& table=data.tables.front();const auto& cell=table.m_content.m_rows[0].m_cells[0];
            if(table.parentHandle!=(block?0xB3u:0xB0u)||table.m_tableOverrideFlags!=17u||table.m_borderColorOverrideFlags!=33u||table.m_horizontalDirection.x!=1.0
                ||table.m_horizontalDirection.y!=2.0||table.m_horizontalDirection.z!=3.0||cell.m_overrideFlags!=19u||cell.m_flags!=23||cell.m_textHeight!=1.25
                ||cell.m_contents.size()!=1u)return false;
            const auto& content=cell.m_contents[0];const auto& value=content.m_value;
            if(value.m_dataType!=type||value.m_formatFlags!=0||value.m_unitType!=8||value.m_formatString!="value-format-é"
                ||value.m_valueString!="display-cache-é"||content.m_text!=(type==4?"payload-é":"display-cache-é"))return false;
            if(type==0||type==1){if(value.m_value.type()!=DRW_Variant::INTEGER||value.m_value.i_val()!=-17)return false;}
            else if(type==2){if(value.m_value.type()!=DRW_Variant::DOUBLE||value.m_value.d_val()!=-2.5)return false;}
            else if(type==4){if(value.m_value.type()!=DRW_Variant::STRING||!value.m_value.content.s||*value.m_value.content.s!="payload-é")return false;}
            else if(type==8||type==512) {
                const std::vector<std::uint8_t> expected=type==8?std::vector<std::uint8_t>{0,0x11,0x22,0x33,0xAA,0xBB,0xCC,0xDD}:std::vector<std::uint8_t>{1,0x23,0xFF};
                if(value.m_dataSize!=expected.size()||value.m_rawData!=expected||value.m_value.type()!=DRW_Variant::BINARY
                    ||!value.m_value.binary()||*value.m_value.binary()!=expected)return false;
            }else if(type==16||type==32) {
                if(value.m_dataSize!=(type==16?16u:24u)||value.m_value.type()!=DRW_Variant::COORD||!value.m_value.coord()
                    ||value.m_value.coord()->x!=1.5||value.m_value.coord()->y!=-2.5||value.m_value.coord()->z!=(type==16?0.0:3.5))return false;
            }else if(value.m_handle!=0xFFEE1122u||value.m_value.type()!=DRW_Variant::INTEGER
                ||static_cast<std::uint32_t>(value.m_value.i_val())!=0xFFEE1122u)return false;
            if(table.m_semanticContentComplete!=(type!=0&&type!=512))return false;
            const auto report=dx_iface::tableBodyReport(data);
            if(!report.modeledMatched||report.profilesMatched||report.ownerTargetsResolved||report.outputEligible)return false;
            ++profiles;
        }
    for(bool binary:{false,true})for(bool block:{false,true})for(bool structured:{false,true}) {
        auto tags=structured?tableValuePayloadDocument("AC1032",block,1):tableProfileDocument("AC1032",block,false,true);
        tags[blockField(tags,"501",304)].value="END_VALUE";dx_iface reader;dx_data data;
        if(!write(path,tags,binary)||!reader.fileImport(path.string(),&data,false)||data.tables.size()!=1u
            ||data.tables.front().m_semanticContentComplete)return false;
        tags[blockField(tags,"501",304)].value="ACVALUE_END";dx_data corrected;
        if(!write(path,tags,binary)||!reader.fileImport(path.string(),&corrected,false)
            ||!corrected.tables.front().m_semanticContentComplete)return false;
        ++markers;
    }
    for(const auto& operation:tableValueOperations())for(bool binary:{false,true}) {
        for(bool under:{false,true}){if(!tableValueOperation(path,operation,binary,under,-1,reached,unreached))return false;++bounds;}
        for(long offset=0;offset<8;++offset)if(!tableValueOperation(path,operation,binary,false,offset,reached,unreached))return false;
    }
    if(!tableValueNodeBoundaries(path,nodes)||!tableValueBoundaryImports(path,refusals,associated,aligned))return false;
    std::cout<<"TABLE structured value profiles="<<profiles<<"; canonical marker refusals/corrections="<<markers
        <<"; exact/one-under value byte limits="<<bounds<<"; reached value allocation faults="<<reached<<"; unreached offsets="<<unreached
        <<"; node limits="<<nodes<<"; malformed envelope retries="<<refusals<<"; associated empty block values="<<associated<<"; cell/LINE alignment retries="<<aligned<<'\n';
    return profiles==216u&&markers==8u&&bounds==24u&&reached>0u&&reached+unreached==96u&&nodes==24u&&refusals==64u&&associated==4u&&aligned==4u;
}
