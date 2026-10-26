// SPDX-License-Identifier: GPL-2.0-or-later
// Runtime-authored scalar scope controls, not full CadValue qualification.
Tags tableValueScopeDocument(const std::string& version,bool blocks,int flags,int units,bool headerAfter=false) {
    auto tags=tableProfileDocument(version,blocks,false,false,1u);
    const auto at=blockField(tags,"501",1);
    Tags value{{301,"CELL_VALUE"},{93,std::to_string(flags)},{90,"4"}};
    if(!(flags&1))value.push_back({1,"value-é"});
    value.insert(value.end(),{{94,std::to_string(units)},{304,"ACVALUE_END"}});
    tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at));
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),value.begin(),value.end());
    if(headerAfter)appendBlockFields(tags,"501",{{93,"4660"},{94,"22136"},{95,"9"},{96,"17"}});
    else {
        const auto header=blockField(tags,"501",171);
        tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(header),{{93,"4660"},{94,"22136"},{95,"9"},{96,"17"}});
    }
    return tags;
}
bool tableValueScopeTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;
    const auto path=dir/"table-value-scopes.dxf";unsigned profiles=0u,mutations=0u,retries=0u,writes=0u;
    const auto fields=[](const DRW_Table& table,int flags,int units) {
        if(table.m_tableOverrideFlags!=4660u||table.m_borderColorOverrideFlags!=22136u
            ||table.m_borderLineweightOverrideFlags!=9u||table.m_borderVisibilityOverrideFlags!=17u
            ||table.m_content.m_rows.size()!=1u||table.m_content.m_rows[0].m_cells.size()!=1u)return false;
        const auto& cell=table.m_content.m_rows[0].m_cells[0];
        return cell.m_contents.size()==1u&&cell.m_contents[0].m_value.m_formatFlags==flags
            &&cell.m_contents[0].m_value.m_unitType==units;
    };
    const int flagValues[]={0,1,2,6,2147483647},unitValues[]={0,1,2,4,8};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)
        for(bool blocks:{false,true})for(bool after:{false,true})for(unsigned i=0u;i<5u;++i) {
            const auto tags=tableValueScopeDocument(version,blocks,flagValues[i],unitValues[i],after);
            dx_iface reader;dx_data data;if(!writeBlockBinding(path,tags,encoding)||!reader.fileImport(path.string(),&data,false))return false;
            const auto original=A::tableInputs(data).find(0x501u),typed=A::tableTypedInputs(data).find(0x501u);
            if(original==A::tableInputs(data).end()||typed==A::tableTypedInputs(data).end()
                ||!fields(*original->second->model,flagValues[i],unitValues[i])||!fields(*typed->second->model,flagValues[i],unitValues[i])
                ||data.tables.size()!=1u||!fields(data.tables.front(),flagValues[i],unitValues[i])) {
                std::cerr<<"TABLE value metadata scope "<<version<<'/'<<encoding<<" blocks="<<blocks<<" after="<<after<<" flags="<<flagValues[i]<<" units="<<unitValues[i]<<'\n';return false;
            }
            const auto body=dx_iface::tableBodyReport(data);
            if(!body.modeledMatched||body.profilesMatched||body.ownerTargetsResolved||body.outputEligible)return false;
            DRW_Table copy(data.tables.front()),assigned;assigned=copy;DRW_Table moved(std::move(assigned));
            if(!fields(copy,flagValues[i],unitValues[i])||!fields(moved,flagValues[i],unitValues[i]))return false;
            ++profiles;
        }
    for(bool binary:{false,true})for(bool blocks:{false,true}) {
        const auto good=tableValueScopeDocument("AC1032",blocks,6,8);
        const auto retry=[&](dx_iface& reader) {dx_data next;return write(path,good,binary)&&reader.fileImport(path.string(),&next,false)
            &&next.tables.size()==1u&&fields(next.tables.front(),6,8);};
        for(bool before:{false,true})for(bool unit:{false,true}) {
            TableBodyProbe reader;reader.beforeSource=before;
            reader.mutation=[unit](DRW_Table& table) {auto& value=table.m_content.m_rows[0].m_cells[0].m_contents[0].m_value;
                if(unit)++value.m_unitType;else ++value.m_formatFlags;};
            dx_data data;if(!write(path,good,binary)||!reader.fileImport(path.string(),&data,false)
                ||dx_iface::tableBodyReport(data).modeledMatched||dx_iface::tableBodyReport(data).outputEligible)return false;
            reader.mutation={};if(!retry(reader))return false;++mutations;
        }
        for(int code:{93,94}) {
            auto tags=good;const auto marker=blockField(tags,"501",301);
            auto field=marker+1u;while(field<tags.size()&&tags[field].code!=code)++field;
            if(field==tags.size())return false;tags[field].value="-1";
            dx_iface reader;dx_data data;if(!write(path,tags,binary)||reader.fileImport(path.string(),&data,false)||!retry(reader))return false;
            ++retries;
        }
        // A failed tail never turns a correctly scoped partial observation into
        // a successful original certificate; corrected same-adapter use works.
        auto tail=good;tail.pop_back();dx_iface reader;dx_data partial;
        if(!write(path,tail,binary)||reader.fileImport(path.string(),&partial,false)
            ||dx_iface::tableBodyReport(partial).modeledMatched||!retry(reader))return false;
        ++retries;
    }
    const auto output=dir/"table-value-writer.dxf";
    for(const auto version:{DRW::AC1021,DRW::AC1024,DRW::AC1027,DRW::AC1032})for(bool binary:{false,true}) {
        dx_data data;dx_iface writer;writer.cData=&data;writer.currentBlock=data.mBlock;
        DRW_Table table;table.handle=0x500u;table.parentHandle=0x1Fu;table.name="*T500";
        table.m_hasSemanticContent=table.m_semanticContentComplete=true;
        table.m_content.m_columns.push_back({"",2.0});table.m_content.m_rows.resize(1u);
        auto& row=table.m_content.m_rows.front();row.m_height=1.0;row.m_cells.resize(1u);row.m_cells[0].m_type=1;
        DRW_TableCellContent content;content.m_type=1;content.m_text="preserved-text";row.m_cells[0].m_contents.push_back(content);
        writer.addTable(table);
        const auto readback=[&]() {dx_iface reader;dx_data next;return reader.fileImport(output.string(),&next,false)&&next.tables.size()==1u
            &&next.tables.front().m_content.m_rows.size()==1u
            &&next.tables.front().m_content.m_rows[0].m_cells[0].m_contents[0].m_text=="preserved-text";};
        const bool exported=writer.fileExport(output.string(),version,binary,&data,false);
        if(!exported||!readback()) {
            std::cerr<<"TABLE value writer baseline version="<<version<<" binary="<<binary<<" exported="<<exported<<'\n';return false;
        }
        for(unsigned field=0u;field<7u;++field) {
            dx_data candidate;dx_iface facade;facade.cData=&candidate;facade.currentBlock=candidate.mBlock;facade.addTable(table);
            auto& value=candidate.tables.front().m_content.m_rows[0].m_cells[0].m_contents[0].m_value;
            switch(field) {
            case 0:value.m_formatFlags=2;break;case 1:value.m_unitType=8;break;case 2:value.m_dataSize=8u;break;
            case 3:value.m_formatString="0.00";break;case 4:value.m_valueString="display";break;
            case 5:value.m_handle=0x600u;break;default:value.m_rawData={0x12u,0x34u};break;
            }
            const std::string sentinel="KEEP_TABLE_VALUE_DESTINATION";{std::ofstream out(output,std::ios::binary);out<<sentinel;}
            const bool accepted=facade.fileExport(output.string(),version,binary,&candidate,false);
            std::ifstream in(output,std::ios::binary);const std::string actual(std::istreambuf_iterator<char>(in),{});
            if(accepted||actual!=sentinel){std::cerr<<"TABLE writer dropped value metadata field="<<field<<" version="<<version<<" binary="<<binary<<'\n';return false;}
            candidate.tables.front().m_content.m_rows[0].m_cells[0].m_contents[0].m_value=DRW_CadValue{};
            const bool corrected=facade.fileExport(output.string(),version,binary,&candidate,false);
            if(!corrected||!readback()) {
                std::cerr<<"TABLE value writer corrected field="<<field<<" version="<<version<<" binary="<<binary<<" exported="<<corrected<<'\n';return false;
            }
            ++writes;
        }
    }
    std::cout<<"TABLE value metadata scopes="<<profiles<<"; actual field mutations="<<mutations<<"; malformed/tail retries="<<retries
        <<"; atomic metadata write refusals and corrected readbacks="<<writes<<'\n';
    return profiles==240u&&mutations==16u&&retries==12u&&writes==56u;
}
