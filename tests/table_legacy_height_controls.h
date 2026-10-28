// SPDX-License-Identifier: GPL-2.0-or-later
// Selected legacy TABLE text-height override. Inputs here are authored from
// scratch; independent original records only identify the field-role profile.
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
    return sourceRoutes==2u&&successes==8u&&refusals==72u&&retries==72u;
}
