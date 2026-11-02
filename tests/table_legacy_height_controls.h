// SPDX-License-Identifier: GPL-2.0-or-later
// Selected legacy TABLE text-height override. Inputs here are authored from
// scratch; independent original records only identify the field-role profile.
bool legacyTableAlignmentTests(const std::filesystem::path& dir);
bool legacyTableBlockAlignmentTests(const std::filesystem::path& dir);
bool legacyTableBlockCountPresenceTests(const std::filesystem::path& dir);
bool legacyTableTextHeightTests(const std::filesystem::path& dir) {
    const auto source=dir/"legacy-height-source.dxf", output=dir/"legacy-height-output.dxf";
    const auto fail=[](const std::string& reason){std::cerr<<"legacy TABLE height: "<<reason<<'\n';return false;};
    const Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1018"},{0,"ENDSEC"},
        {0,"SECTION"},{2,"ENTITIES"},{0,"ACAD_TABLE"},{5,"4F2"},{100,"AcDbEntity"},{8,"0"},
        {100,"AcDbBlockReference"},{2,"*T1"},{10,"1"},{20,"2"},{30,"3"},{100,"AcDbTable"},
        {90,"2"},{91,"1"},{92,"1"},{141,"8"},{142,"20"},{171,"1"},{172,"0"},{173,"0"},
        {174,"0"},{175,"1"},{176,"1"},{177,"32"},{178,"0"},{145,"0"},{1,"source-text"},{140,"4"},
        {0,"ENDSEC"},{0,"EOF"}};
    unsigned sourceRoutes=0u,successes=0u,refusals=0u,retries=0u;
    for(bool inputBinary:{false,true}) {
        if(!write(source,tags,inputBinary))return fail("source write");
        dx_iface sourceReader;dx_data imported;
        if(!sourceReader.fileImport(source.string(),&imported,false)||imported.tables.size()!=1u
            ||imported.tables.front().m_content.m_rows.size()!=1u
            ||imported.tables.front().m_content.m_rows[0].m_cells.size()!=1u)return fail("source import");
        const auto& observed=imported.tables.front().m_content.m_rows[0].m_cells[0];
        if(observed.m_overrideFlags!=0x20u||observed.m_textHeight!=4.0)return fail("source fields");
        ++sourceRoutes;
        for(bool binary:{false,true})for(bool strict:{false,true}) {
            dx_data data;dx_iface writer;tableExportReuseDrawing(writer,data,0u,false);
            DRW_Block_Record modelSpace;modelSpace.handle=0x1fu;modelSpace.name="*Model_Space";
            data.blockRecords.push_back(modelSpace);
            auto& cell=data.tables.front().m_content.m_rows[0].m_cells[0];
            cell.m_overrideFlags=observed.m_overrideFlags;cell.m_textHeight=observed.m_textHeight;
            const auto identities=tableExportIdentities(data);
            const auto check=[&]() {
                dx_iface reader;dx_data readback;
                if(!reader.fileImport(output.string(),&readback,false)||readback.tables.size()!=1u
                    ||readback.tables.front().m_content.m_rows.size()!=1u
                    ||readback.tables.front().m_content.m_rows[0].m_cells.size()!=1u)return false;
                const auto& result=readback.tables.front().m_content.m_rows[0].m_cells[0];
                if(result.m_overrideFlags!=0x20u||result.m_textHeight!=4.0
                    ||result.m_contents.size()!=1u||result.m_contents[0].m_text!="reused-table")return false;
                const auto wire=attdefWireTags(output,binary);
                bool inTable=false,inCell=false;std::vector<int> codes;unsigned tableCount=0u;
                for(const auto& tag:wire) {
                    if(tag.code==0){inTable=tag.text=="ACAD_TABLE";inCell=false;if(inTable)++tableCount;continue;}
                    if(!inTable)continue;
                    if(tag.code==171)inCell=true;
                    if(!inCell)continue;
                    codes.push_back(tag.code);
                    if(tag.code==177&&(tag.kind!=AttdefWireTag::Integer||tag.integer!=32))return false;
                    if(tag.code==140&&(tag.kind!=AttdefWireTag::Real||tag.real!=4.0))return false;
                }
                return tableCount==1u&&codes==std::vector<int>{171,172,173,174,175,176,177,178,145,1,140};
            };
            if(!writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)||!check()
                ||identities!=tableExportIdentities(data))return fail("positive output/readback");
            ++successes;
            // Refuse inconsistent or broader profiles; the identical writer and
            // model must succeed after each caller correction.
            for(unsigned fault=0u;fault<9u;++fault) {
                if(fault==0u)cell.m_textHeight=0.0;
                if(fault==1u)cell.m_overrideFlags=0u;
                if(fault==2u)cell.m_overrideFlags=0x21u;
                if(fault==3u)cell.m_overrideFlags=0x10020u;
                if(fault==4u)cell.m_textHeight=-1.0;
                if(fault==5u)cell.m_textHeight=std::numeric_limits<double>::infinity();
                if(fault==6u)cell.m_textHeight=std::numeric_limits<double>::quiet_NaN();
                if(fault==7u)cell.m_backgroundEnabled=true;
                if(fault==8u)cell.m_borders[0].m_lineWeight=1;
                const auto invalidFlags=cell.m_overrideFlags;const double invalidHeight=cell.m_textHeight;
                if(!tableExportSentinel(output,true)||writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)
                    ||!tableExportSentinel(output)||identities!=tableExportIdentities(data)
                    ||cell.m_overrideFlags!=invalidFlags
                    ||(!std::isnan(invalidHeight)&&cell.m_textHeight!=invalidHeight)
                    ||(std::isnan(invalidHeight)&&!std::isnan(cell.m_textHeight)))return fail("refusal/caller preservation "+std::to_string(fault));
                ++refusals;
                cell.m_overrideFlags=0x20u;cell.m_textHeight=4.0;cell.m_backgroundEnabled=false;cell.m_borders[0].m_lineWeight=0;
                if(!writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)||!check()
                    ||identities!=tableExportIdentities(data))return fail("corrected retry "+std::to_string(fault));
                ++retries;
            }
        }
    }
    std::cout<<"legacy TABLE height source routes="<<sourceRoutes<<" outputs="<<successes
        <<" atomic refusals="<<refusals<<" corrected retries="<<retries<<'\n';
    return sourceRoutes==2u&&successes==8u&&refusals==72u&&retries==72u
        &&legacyTableAlignmentTests(dir)&&legacyTableBlockAlignmentTests(dir)&&legacyTableBlockCountPresenceTests(dir);
}

// Autodesk specifies the nine alignment values for table cells. The selected
// bit combinations are inferred from independent flag layouts and an original
// composite-mask cell, not claimed as native-producer or renderer evidence.
bool legacyTableAlignmentTests(const std::filesystem::path& dir) {
    const auto source=dir/"legacy-alignment-source.dxf",output=dir/"legacy-alignment-output.dxf";
    const auto fail=[](const std::string& reason){std::cerr<<"legacy TABLE alignment: "<<reason<<'\n';return false;};
    const auto sourceFor=[](std::uint32_t mask,int alignment) {
        Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1018"},{0,"ENDSEC"},
            {0,"SECTION"},{2,"ENTITIES"},{0,"ACAD_TABLE"},{5,"4F2"},{100,"AcDbEntity"},{8,"0"},
            {100,"AcDbBlockReference"},{2,"*T1"},{10,"1"},{20,"2"},{30,"3"},{100,"AcDbTable"},
            {90,"2"},{91,"1"},{92,"1"},{141,"8"},{142,"20"},{171,"1"},{172,"0"},{173,"0"},
            {174,"0"},{175,"1"},{176,"1"},{177,std::to_string(mask)},{178,"0"},{145,"0"},
            {1,"source-text"},{170,std::to_string(alignment)}};
        if(mask&0x20u)tags.push_back({140,"4"});
        tags.insert(tags.end(),{{0,"ENDSEC"},{0,"EOF"}});return tags;
    };
    unsigned inputs=0u,outputs=0u,refusals=0u,retries=0u;
    for(bool inputBinary:{false,true})for(std::uint32_t mask:{0x01u,0x21u})for(int alignment=1;alignment<=9;++alignment) {
        if(!write(source,sourceFor(mask,alignment),inputBinary))return fail("source write");
        dx_iface sourceReader;dx_data imported;
        if(!sourceReader.fileImport(source.string(),&imported,false)||imported.tables.size()!=1u
            ||imported.tables.front().m_content.m_rows.size()!=1u
            ||imported.tables.front().m_content.m_rows[0].m_cells.size()!=1u)return fail("source import");
        const auto& observed=imported.tables.front().m_content.m_rows[0].m_cells[0];
        const double height=mask&0x20u?4.0:0.0;
        if(observed.m_overrideFlags!=mask||observed.m_textAlignment!=alignment||observed.m_textHeight!=height)
            return fail("source fields");
        ++inputs;
        for(bool binary:{false,true})for(bool strict:{false,true}) {
            dx_data data;dx_iface writer;tableExportReuseDrawing(writer,data,0u,false);
            DRW_Block_Record modelSpace;modelSpace.handle=0x1fu;modelSpace.name="*Model_Space";
            data.blockRecords.push_back(modelSpace);
            auto& cell=data.tables.front().m_content.m_rows[0].m_cells[0];
            cell.m_overrideFlags=mask;cell.m_textAlignment=alignment;cell.m_textHeight=height;
            const auto identities=tableExportIdentities(data);
            const auto check=[&]() {
                dx_iface reader;dx_data readback;
                if(!reader.fileImport(output.string(),&readback,false)||readback.tables.size()!=1u
                    ||readback.tables.front().m_content.m_rows.size()!=1u
                    ||readback.tables.front().m_content.m_rows[0].m_cells.size()!=1u)return false;
                const auto& actual=readback.tables.front().m_content.m_rows[0].m_cells[0];
                if(actual.m_overrideFlags!=mask||actual.m_textAlignment!=alignment||actual.m_textHeight!=height
                    ||actual.m_contents.size()!=1u||actual.m_contents[0].m_text!="reused-table")return false;
                const auto wire=attdefWireTags(output,binary);
                bool inTable=false,inCell=false;std::vector<int> codes;unsigned tableCount=0u;
                for(const auto& tag:wire) {
                    if(tag.code==0){inTable=tag.text=="ACAD_TABLE";inCell=false;if(inTable)++tableCount;continue;}
                    if(!inTable)continue;
                    if(tag.code==171)inCell=true;
                    if(!inCell)continue;
                    codes.push_back(tag.code);
                    if(tag.code==177&&(tag.kind!=AttdefWireTag::Integer||tag.integer!=mask))return false;
                    if(tag.code==170&&(tag.kind!=AttdefWireTag::Integer||tag.integer!=alignment))return false;
                    if(tag.code==140&&(tag.kind!=AttdefWireTag::Real||tag.real!=height))return false;
                }
                std::vector<int> expected{171,172,173,174,175,176,177,178,145,1,170};
                if(mask&0x20u)expected.push_back(140);
                return tableCount==1u&&codes==expected;
            };
            if(!writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)||!check()
                ||identities!=tableExportIdentities(data))return fail("output/readback");
            ++outputs;
            // Repeat negative controls only once per route, not for every enum.
            if(alignment!=5)continue;
            for(unsigned fault=0u;fault<8u;++fault) {
                if(fault==0u)cell.m_textAlignment=0;
                if(fault==1u)cell.m_textAlignment=10;
                if(fault==2u)cell.m_textAlignment=-1;
                if(fault==3u)cell.m_overrideFlags|=0x04u;
                if(fault==4u)cell.m_overrideFlags=0u;
                if(fault==5u)cell.m_textHeight=mask&0x20u?0.0:4.0;
                if(fault==6u)cell.m_backgroundEnabled=true;
                if(fault==7u)cell.m_borders[0].m_lineWeight=1;
                const auto flags=cell.m_overrideFlags;const auto value=cell.m_textAlignment;const auto invalidHeight=cell.m_textHeight;
                if(!tableExportSentinel(output,true)||writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)
                    ||!tableExportSentinel(output)||identities!=tableExportIdentities(data)
                    ||cell.m_overrideFlags!=flags||cell.m_textAlignment!=value||cell.m_textHeight!=invalidHeight)
                    return fail("refusal/caller preservation "+std::to_string(fault));
                ++refusals;
                cell.m_overrideFlags=mask;cell.m_textAlignment=alignment;cell.m_textHeight=height;
                cell.m_backgroundEnabled=false;cell.m_borders[0].m_lineWeight=0;
                if(!writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)||!check()
                    ||identities!=tableExportIdentities(data))return fail("corrected retry "+std::to_string(fault));
                ++retries;
            }
        }
    }
    std::cout<<"legacy TABLE alignment sources="<<inputs<<" outputs="<<outputs
        <<" atomic refusals="<<refusals<<" corrected retries="<<retries<<'\n';
    return inputs==36u&&outputs==144u&&refusals==128u&&retries==128u;
}

// Source-witnessed AC1018 block-cell alignment; 179=0 physical presence is
// not yet retained. All test source and block definitions are locally authored.
bool legacyTableBlockAlignmentTests(const std::filesystem::path& dir) {
    const auto source=dir/"legacy-block-align-source.dxf",output=dir/"legacy-block-align-output.dxf";
    const auto fail=[](const std::string& why){std::cerr<<"legacy TABLE block alignment: "<<why<<'\n';return false;};
    const Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1018"},{0,"ENDSEC"},
        {0,"SECTION"},{2,"ENTITIES"},{0,"ACAD_TABLE"},{5,"4F2"},{100,"AcDbEntity"},{8,"0"},
        {100,"AcDbBlockReference"},{2,"*T1"},{10,"1"},{20,"2"},{30,"3"},{100,"AcDbTable"},
        {90,"2"},{91,"1"},{92,"1"},{141,"8"},{142,"20"},{171,"2"},{172,"0"},{173,"0"},
        {174,"1"},{175,"1"},{176,"1"},{177,"1"},{178,"0"},{145,"0"},{340,"601"},{144,"1.5"},{179,"0"},{170,"1"},
        {0,"ENDSEC"},{0,"EOF"}};
    unsigned sourceRoutes=0u,successes=0u,refusals=0u,retries=0u,modern=0u;
    for(bool inputBinary:{false,true}) {
        if(!write(source,tags,inputBinary))return fail("source write");
        dx_iface importer;dx_data imported;
        if(!importer.fileImport(source.string(),&imported,false)||imported.tables.size()!=1u
            ||!imported.tables.front().m_semanticContentComplete)return fail("source import");
        const auto sourceCell=imported.tables.front().m_content.m_rows[0].m_cells[0];
        if(sourceCell.m_type!=2||sourceCell.m_blockHandle!=0x601u||sourceCell.m_overrideFlags!=1u
            ||sourceCell.m_textAlignment!=1||sourceCell.m_blockScale!=1.5||!sourceCell.m_attributes.empty())return fail("source fields");
        ++sourceRoutes;
        for(bool binary:{false,true})for(bool strict:{false,true})for(int alignment:{1,9}) {
            dx_data data;dx_iface writer;tableExportReuseDrawing(writer,data,0u,false);
            DRW_Block_Record model;model.handle=0x1fu;model.name="*Model_Space";data.blockRecords.push_back(model);
            DRW_Block_Record record;record.handle=0x601u;record.name="TABLE_CELL_BLOCK";data.blockRecords.push_back(record);
            auto* block=new dx_ifaceBlock();block->name=record.name;block->parentHandle=record.handle;
            DRW_Line line;line.handle=0x602u;line.basePoint=DRW_Coord(1.,2.,0.);line.secPoint=DRW_Coord(3.,4.,0.);
            block->ent.push_back(new DRW_Line(line));data.blocks.push_back(block);
            auto& table=data.tables.front();table.m_content.m_rows[0].m_cells[0]=sourceCell;
            auto& cell=table.m_content.m_rows[0].m_cells[0];cell.m_textAlignment=alignment;
            const DRW_Table baseline(table);const auto identities=tableExportIdentities(data);
            const auto check=[&]() {
                dx_iface reader;dx_data after;
                if(!reader.fileImport(output.string(),&after,false)||after.tables.size()!=1u
                    ||!after.tables.front().m_semanticContentComplete)return false;
                const auto& got=after.tables.front().m_content.m_rows[0].m_cells[0];
                auto found=std::find_if(after.blockRecords.begin(),after.blockRecords.end(),[](const auto& b){return b.name=="TABLE_CELL_BLOCK";});
                if(found==after.blockRecords.end()||got.m_type!=2||got.m_blockHandle!=found->handle
                    ||got.m_contents.size()!=1u||got.m_contents[0].m_handle!=got.m_blockHandle
                    ||got.m_overrideFlags!=1u||got.m_textAlignment!=alignment||got.m_blockScale!=1.5)return false;
                bool hasBlock=false;for(const auto* b:after.blocks)if(b&&b->name==record.name&&b->parentHandle==found->handle)hasBlock=true;
                if(!hasBlock)return false;
                const auto wire=attdefWireTags(output,binary);bool inTable=false,inCell=false;std::vector<int> codes;unsigned tables=0;
                for(const auto& tag:wire) {
                    if(tag.code==0){inTable=tag.text=="ACAD_TABLE";inCell=false;if(inTable)++tables;continue;}
                    if(!inTable)continue;if(tag.code==171)inCell=true;if(!inCell)continue;codes.push_back(tag.code);
                    if(tag.code==177&&(tag.kind!=AttdefWireTag::Integer||tag.integer!=1))return false;
                    if(tag.code==170&&(tag.kind!=AttdefWireTag::Integer||tag.integer!=alignment))return false;
                }
                // The authored source has explicit 179=0, output omits it;
                // no absent-vs-zero semantics are claimed by this check.
                return tables==1u&&codes==std::vector<int>{171,172,173,174,175,176,177,178,145,340,144,170};
            };
            if(!writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)||!check()
                ||identities!=tableExportIdentities(data)||!DxIfaceTableSourceTestAccess::sameTableBody(baseline,table))return fail("positive");
            ++successes;
            for(unsigned fault=0u;fault<8u;++fault) {
                if(fault==0u)cell.m_textAlignment=0;
                if(fault==1u)cell.m_textAlignment=10;
                if(fault==2u)cell.m_overrideFlags=0u;
                if(fault==3u)cell.m_overrideFlags=0x21u;
                if(fault==4u)cell.m_overrideFlags=0x20u;
                if(fault==5u)cell.m_overrideFlags=0x41u;
                if(fault==6u){cell.m_blockHandle=0x700u;cell.m_contents[0].m_handle=0x700u;}
                const DRW_Table invalid(table);const auto saved=tableExportIdentities(data);
                const auto version=fault==7u?DRW::AC1015:DRW::AC1018;
                if(!tableExportSentinel(output,true)||writer.fileExport(output.string(),version,binary,&data,false,strict)
                    ||!tableExportSentinel(output)||saved!=tableExportIdentities(data)
                    ||!DxIfaceTableSourceTestAccess::sameTableBody(invalid,table))return fail("invalid "+std::to_string(fault));
                ++refusals;table=baseline;
                if(!writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)||!check()
                    ||identities!=tableExportIdentities(data)||!DxIfaceTableSourceTestAccess::sameTableBody(baseline,table))return fail("retry");
                ++retries;
            }
            if(!writer.fileExport(output.string(),DRW::AC1021,binary,&data,false,strict))return fail("modern compatibility");
            dx_iface modernReader;dx_data readback;
            if(!modernReader.fileImport(output.string(),&readback,false)||readback.tables.size()!=1u
                ||readback.tables.front().m_content.m_rows[0].m_cells[0].m_textAlignment!=alignment)return fail("modern readback");
            ++modern;
        }
    }
    std::cout<<"legacy TABLE block alignment source="<<sourceRoutes<<" outputs="<<successes<<" refusals="<<refusals<<" retries="<<retries<<" modern="<<modern<<'\n';
    return sourceRoutes==2u&&successes==16u&&refusals==128u&&retries==128u&&modern==16u;
}

// Selected physical 179=0 source spelling; no native default claim.
struct TableLegacyCountLateProbe final:dx_iface {
    bool mutateScale=false,mutateHandle=false,mutateVersion=false,mutateFormat=false,mutateGeneration=false,suppressHeader=false;
    unsigned sourceFault=0u;
    void addHeader(const DRW_Header* header) override {
        if(!suppressHeader)dx_iface::addHeader(header);
    }
    void addDxfTableSource(const DRW_Table& table,const DRW_RawDxfObject& raw) override {
        if(!sourceFault){dx_iface::addDxfTableSource(table,raw);return;}
        DRW_RawDxfObject copied(raw);
        if(sourceFault==1u)copied.m_version=DRW::AC1021;
        if(sourceFault==2u)++copied.handle;
        if(sourceFault==3u)++copied.parentHandle;
        dx_iface::addDxfTableSource(table,copied);
    }
    void writeEntities() override {
        auto& t=cData->tables.front();const auto oldHandle=t.handle;const double oldScale=t.m_content.m_rows[0].m_cells[0].m_blockScale;
        const auto version=cData->sourceVersion;const auto format=cData->sourceFormat;const auto generation=cData->importGeneration;
        if(mutateScale)t.m_content.m_rows[0].m_cells[0].m_blockScale=2.;
        if(mutateHandle)++t.handle;
        if(mutateVersion)cData->sourceVersion=DRW::AC1021;
        if(mutateFormat)cData->sourceFormat=dx_source_format::DWG;
        if(mutateGeneration)++cData->importGeneration;
        dx_iface::writeEntities();
        t.handle=oldHandle;t.m_content.m_rows[0].m_cells[0].m_blockScale=oldScale;
        cData->sourceVersion=version;cData->sourceFormat=format;cData->importGeneration=generation;
    }
};
struct TableLegacyCountDirectProbe final:dx_iface {
    std::vector<std::uint8_t> spellings;
    void writeEntities() override {
        auto& table=cData->tables.front();const auto sourceHandle=table.handle;
        if(dxfW->writeTableWithLegacySourcePresence(&table,spellings))
            DxIfaceTableSourceTestAccess::markTableEmitted(*this,sourceHandle);
    }
};
bool legacyTableBlockCountPresenceTests(const std::filesystem::path& dir) {
    const auto source=dir/"legacy-block-count-source.dxf",output=dir/"legacy-block-count-output.dxf";
    const auto fail=[](const std::string& why){std::cerr<<"legacy TABLE block count presence: "<<why<<'\n';return false;};
    unsigned successes=0u,lateRefusals=0u,retries=0u,invalids=0u;
    for(bool inputBinary:{false,true})for(bool outputBinary:{false,true})for(bool strict:{false,true})for(unsigned form=0;form<5u;++form) {
        // 0: explicit 179 only; 1: both zero groups; 2: 177 only;
        // 3: neither; 4: explicit zero 177 with a nonzero attribute count.
        const bool zeroFlag=form==1u||form==2u||form==4u, zeroCount=form<2u, attribute=form==4u;
        Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1018"},{0,"ENDSEC"},
            {0,"SECTION"},{2,"ENTITIES"},{0,"ACAD_TABLE"},{5,"4F2"},{330,"1F"},{100,"AcDbEntity"},{8,"0"},
            {100,"AcDbBlockReference"},{2,"*T1"},{10,"1"},{20,"2"},{30,"3"},{100,"AcDbTable"},
            {90,"2"},{91,"1"},{92,"1"},{141,"8"},{142,"20"},{171,"2"},{172,"0"},{173,"0"},
            {174,"1"},{175,"1"},{176,"1"},{177,zeroFlag?"0":"1"},{178,"0"},{145,"0"},{340,"601"},{144,"1.5"}};
        if(zeroCount)tags.push_back({179,"0"});
        if(attribute)tags.insert(tags.end(),{{179,"1"},{331,"603"},{300,"attribute value"}});
        if(!zeroFlag)tags.push_back({170,"1"});
        tags.insert(tags.end(),{{0,"ENDSEC"},{0,"EOF"}});
        if(!write(source,tags,inputBinary))return fail("source write");
        TableLegacyCountLateProbe io;dx_data data;
        if(!io.fileImport(source.string(),&data,false)||data.tables.size()!=1u||!data.tables.front().m_semanticContentComplete)
            return fail("source import");
        DRW_Block_Record model;model.handle=0x1fu;model.name="*Model_Space";data.blockRecords.push_back(model);
        DRW_Block_Record record;record.handle=0x601u;record.name="TABLE_CELL_BLOCK";data.blockRecords.push_back(record);
        auto* block=new dx_ifaceBlock();block->name=record.name;block->parentHandle=record.handle;
        if(attribute){DRW_Attdef def;def.handle=0x603u;def.parentHandle=record.handle;def.tag="TAG";def.text="DEFAULT";block->ent.push_back(new DRW_Attdef(def));}
        data.blocks.push_back(block);
        auto& table=data.tables.front();const DRW_Table baseline(table);const auto identities=tableExportIdentities(data);
        const auto check=[&]() {
            dx_iface reader;dx_data after;
            if(!reader.fileImport(output.string(),&after,false)||after.tables.size()!=1u||!after.tables.front().m_semanticContentComplete)return false;
            const auto& cell=after.tables.front().m_content.m_rows[0].m_cells[0];
            const auto provider=std::find_if(after.blockRecords.begin(),after.blockRecords.end(),[](const auto& v){return v.name=="TABLE_CELL_BLOCK";});
            if(provider==after.blockRecords.end()||cell.m_blockHandle!=provider->handle
                ||cell.m_attributes.size()!=(attribute?1u:0u)||cell.m_overrideFlags!=(zeroFlag?0u:1u))return false;
            bool inTable=false,inCell=false;unsigned n177=0,n179=0;int mask=-1,count=-1;std::size_t pos340=0,pos179=0,pos170=0,index=0;
            for(const auto& v:attdefWireTags(output,outputBinary)) {
                if(v.code==0){inTable=v.text=="ACAD_TABLE";inCell=false;continue;}
                if(!inTable)continue;if(v.code==171)inCell=true;if(!inCell)continue;++index;
                if(v.code==177){++n177;mask=static_cast<int>(v.integer);}if(v.code==179){++n179;count=static_cast<int>(v.integer);pos179=index;}
                if(v.code==340)pos340=index;if(v.code==170)pos170=index;
            }
            return n177==1u&&mask==(zeroFlag?0:1)&&n179==((zeroCount||attribute)?1u:0u)
                &&(!n179||(count==(attribute?1:0)&&pos340<pos179))
                &&(zeroFlag?pos170==0u:(pos170>pos340&&(!n179||pos170>pos179)));
        };
        // Form 3 has no selected zero; its prior source-agnostic spelling
        // intentionally omits 179. Other forms preserve explicit zeros.
        if(!io.fileExport(output.string(),DRW::AC1018,outputBinary,&data,false,strict)||!check()
            ||identities!=tableExportIdentities(data)||!DxIfaceTableSourceTestAccess::sameTableBody(baseline,table))return fail("positive form "+std::to_string(form));
        ++successes;
        if(form!=3u)for(unsigned fault=0;fault<2u;++fault) {
            io.mutateScale=fault==0u;io.mutateHandle=fault==1u;
            if(!tableExportSentinel(output,true)||io.fileExport(output.string(),DRW::AC1018,outputBinary,&data,false,strict)
                ||!tableExportSentinel(output)||identities!=tableExportIdentities(data)
                ||!DxIfaceTableSourceTestAccess::sameTableBody(baseline,table))return fail("late mutation");
            ++lateRefusals;io.mutateScale=io.mutateHandle=false;
            if(!io.fileExport(output.string(),DRW::AC1018,outputBinary,&data,false,strict)||!check())return fail("late retry");
            ++retries;
        }
        if(form==0u) {
            for(unsigned fault=0u;fault<6u;++fault) {
                const auto version=data.sourceVersion;const auto format=data.sourceFormat;const auto generation=data.importGeneration;
                if(fault==0u)data.sourceVersion=DRW::AC1021;
                if(fault==1u)data.sourceFormat=dx_source_format::DWG;
                io.mutateVersion=fault==2u;io.mutateFormat=fault==3u;
                if(fault==4u)++data.importGeneration;io.mutateGeneration=fault==5u;
                if(!tableExportSentinel(output,true)||io.fileExport(output.string(),DRW::AC1018,outputBinary,&data,false,strict)
                    ||!tableExportSentinel(output)||identities!=tableExportIdentities(data)
                    ||!DxIfaceTableSourceTestAccess::sameTableBody(baseline,table))return fail("source provenance mutation");
                data.sourceVersion=version;data.sourceFormat=format;data.importGeneration=generation;io.mutateVersion=io.mutateFormat=io.mutateGeneration=false;
                if(!io.fileExport(output.string(),DRW::AC1018,outputBinary,&data,false,strict)||!check())return fail("provenance retry");
            }
            for(unsigned fault=1u;fault<=4u;++fault) {
                io.sourceFault=fault;dx_data forged;
                if(!write(source,tags,inputBinary)||!io.fileImport(source.string(),&forged,false)||forged.tables.size()!=1u)return fail("forged import");
                forged.blockRecords.push_back(model);forged.blockRecords.push_back(record);
                auto* forgedBlock=new dx_ifaceBlock();forgedBlock->name=record.name;forgedBlock->parentHandle=record.handle;forged.blocks.push_back(forgedBlock);
                const auto saved=tableExportIdentities(forged);const DRW_Table before(forged.tables.front());
                if(!tableExportSentinel(output,true)||io.fileExport(output.string(),DRW::AC1018,outputBinary,&forged,false,strict)
                    ||!tableExportSentinel(output)||saved!=tableExportIdentities(forged)
                    ||!DxIfaceTableSourceTestAccess::sameTableBody(before,forged.tables.front()))return fail("forged source escaped");
                io.sourceFault=0u;dx_data fresh;
                if(!io.fileImport(source.string(),&fresh,false)||fresh.tables.size()!=1u)return fail("forged retry import");
                fresh.blockRecords.push_back(model);fresh.blockRecords.push_back(record);
                auto* freshBlock=new dx_ifaceBlock();freshBlock->name=record.name;freshBlock->parentHandle=record.handle;fresh.blocks.push_back(freshBlock);
                if(!io.fileExport(output.string(),DRW::AC1018,outputBinary,&fresh,false,strict))return fail("forged retry export");
            }
            if(!io.fileExport(output.string(),DRW::AC1018,outputBinary,&data,false,strict)||!check())return fail("provenance final retry");
            io.suppressHeader=true;dx_data noHeader;
            if(!io.fileImport(source.string(),&noHeader,false)||noHeader.tables.size()!=1u)return fail("suppressed header import");
            noHeader.blockRecords.push_back(model);noHeader.blockRecords.push_back(record);
            auto* noHeaderBlock=new dx_ifaceBlock();noHeaderBlock->name=record.name;noHeaderBlock->parentHandle=record.handle;noHeader.blocks.push_back(noHeaderBlock);
            if(!tableExportSentinel(output,true)||io.fileExport(output.string(),DRW::AC1018,outputBinary,&noHeader,false,strict)
                ||!tableExportSentinel(output))return fail("suppressed header escaped");
            io.suppressHeader=false;dx_data fresh;
            if(!io.fileImport(source.string(),&fresh,false)||fresh.tables.size()!=1u)return fail("header retry import");
            fresh.blockRecords.push_back(model);fresh.blockRecords.push_back(record);
            auto* freshBlock=new dx_ifaceBlock();freshBlock->name=record.name;freshBlock->parentHandle=record.handle;fresh.blocks.push_back(freshBlock);
            if(!io.fileExport(output.string(),DRW::AC1018,outputBinary,&fresh,false,strict))return fail("header retry export");
        }
        // A source that was already incomplete still takes the historical
        // compatible INSERT projection instead of the selected TABLE path.
        if(form==0u&&!strict) {
            auto incomplete=tags;const auto target=slot(incomplete,340,"601");incomplete.erase(incomplete.begin()+static_cast<std::ptrdiff_t>(target));
            dx_data partial;if(!write(source,incomplete,inputBinary)||!io.fileImport(source.string(),&partial,false)||partial.tables.size()!=1u
                ||partial.tables.front().m_semanticContentComplete)return fail("incomplete source import");
            partial.blockRecords.push_back(model);
            if(!io.fileExport(output.string(),DRW::AC1018,outputBinary,&partial,false,false))return fail("incomplete INSERT fallback");
            unsigned tables=0u,inserts=0u;
            for(const auto& v:attdefWireTags(output,outputBinary))if(v.code==0){if(v.text=="ACAD_TABLE")++tables;if(v.text=="INSERT")++inserts;}
            if(tables!=0u||inserts!=1u)return fail("incomplete projection shape");
            if(!io.fileExport(output.string(),DRW::AC1018,outputBinary,&data,false,strict)||!check())return fail("incomplete retry");
        }
        // Nested application-owned 179 is not a block count. Conversely,
        // a real host zero behind a nested subtype remains unqualified.
        if(form==0u||form==3u) {
            auto nested=tags;const auto sub=slot(nested,100,"AcDbTable");
            const Tags group=form==0u?Tags{{102,"{APP_SCOPE"},{100,"VendorSubtype"},{102,"}"}}
                :Tags{{102,"{APP_SCOPE"},{179,"0"},{102,"}"}};
            nested.insert(nested.begin()+static_cast<std::ptrdiff_t>(sub+1u),group.begin(),group.end());
            dx_data scoped;if(!write(source,nested,inputBinary)||!io.fileImport(source.string(),&scoped,false)||scoped.tables.size()!=1u)return fail("scoped import");
            scoped.blockRecords.push_back(model);scoped.blockRecords.push_back(record);
            auto* scopedBlock=new dx_ifaceBlock();scopedBlock->name=record.name;scopedBlock->parentHandle=record.handle;scoped.blocks.push_back(scopedBlock);
            if(form==0u) {
                if(!tableExportSentinel(output,true)||io.fileExport(output.string(),DRW::AC1018,outputBinary,&scoped,false,strict)
                    ||!tableExportSentinel(output))return fail("hidden zero escaped");
            } else if(!io.fileExport(output.string(),DRW::AC1018,outputBinary,&scoped,false,strict))return fail("nested count treated as host");
            if(!io.fileExport(output.string(),DRW::AC1018,outputBinary,&data,false,strict)||!check())return fail("scoped retry");
        }
        // Duplicate selected count is structurally ambiguous and must not
        // publish; correcting the source and retrying uses the same adapter.
        if(form==0u) {
            const auto count=slot(tags,179,"0");auto bad=tags;bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(count+1u),{179,"0"});
            dx_data rejected;if(!write(source,bad,inputBinary)||!io.fileImport(source.string(),&rejected,false)||rejected.tables.size()!=1u)return fail("duplicate import");
            rejected.blockRecords.push_back(model);rejected.blockRecords.push_back(record);
            auto* rejectedBlock=new dx_ifaceBlock();rejectedBlock->name=record.name;rejectedBlock->parentHandle=record.handle;rejected.blocks.push_back(rejectedBlock);
            if(!tableExportSentinel(output,true)||io.fileExport(output.string(),DRW::AC1018,outputBinary,&rejected,false,strict)||!tableExportSentinel(output))return fail("duplicate escaped");
            if(!io.fileExport(output.string(),DRW::AC1018,outputBinary,&data,false,strict)||!check())return fail("duplicate retry");
            ++invalids;
        }
    }
    // Direct mask API: valid block 179=0, invalid bits, conflicting zero
    // override, text-cell use, and a non-AC1018 target all fail atomically.
    {
        TableLegacyCountDirectProbe io;dx_data data;tableExportReuseDrawing(io,data,0u,false);
        DRW_Block_Record model;model.handle=0x1fu;model.name="*Model_Space";data.blockRecords.push_back(model);
        DRW_Block_Record record;record.handle=0x601u;record.name="TABLE_CELL_BLOCK";data.blockRecords.push_back(record);
        auto* block=new dx_ifaceBlock();block->name=record.name;block->parentHandle=record.handle;data.blocks.push_back(block);
        auto& table=data.tables.front();auto& cell=table.m_content.m_rows[0].m_cells[0];cell.m_type=2;cell.m_blockHandle=0x601u;
        cell.m_contents[0].m_type=4;cell.m_contents[0].m_handle=0x601u;cell.m_overrideFlags=1u;cell.m_textAlignment=1;
        const DRW_Table baseline(table);
        for(unsigned fault=0u;fault<4u;++fault) {
            table=baseline;io.spellings={fault==0u?std::uint8_t{4u}:fault==1u?std::uint8_t{3u}:std::uint8_t{2u}};
            if(fault==2u)table.m_content.m_rows[0].m_cells[0].m_type=1;
            const auto target=fault==3u?DRW::AC1021:DRW::AC1018;
            if(!tableExportSentinel(output,true)||io.fileExport(output.string(),target,false,&data,false,false)||!tableExportSentinel(output))return fail("direct invalid mask");
            table=baseline;io.spellings={2u};
            if(!io.fileExport(output.string(),DRW::AC1018,false,&data,false,false))return fail("direct mask retry");
            unsigned count=0u;bool inTable=false,inCell=false;
            for(const auto& tag:attdefWireTags(output,false)) {
                if(tag.code==0){inTable=tag.text=="ACAD_TABLE";inCell=false;continue;}
                if(!inTable)continue;if(tag.code==171)inCell=true;
                if(inCell&&tag.code==179&&tag.kind==AttdefWireTag::Integer&&tag.integer==0)++count;
            }
            if(count!=1u)return fail("direct zero count output");
        }
    }
    std::cout<<"legacy TABLE count presence outputs="<<successes<<" late="<<lateRefusals<<" retries="<<retries<<" duplicate="<<invalids<<'\n';
    return successes==40u&&lateRefusals==64u&&retries==64u&&invalids==8u;
}
