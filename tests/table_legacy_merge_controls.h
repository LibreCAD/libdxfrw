// SPDX-License-Identifier: GPL-2.0-or-later
// Readiness controls for a selected AC1018 merge profile. The generated source
// is authored from scratch; the anchor/covered interpretation is inferred from
// an original composite-mask sample and is not a native rendering certificate.
// Include and register only alongside the implementation under test.
Tags legacyMergeDocument(const std::string& version,int rows,int columns) {
    Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"},
        {0,"SECTION"},{2,"ENTITIES"},{0,"ACAD_TABLE"},{5,"4F2"},
        {100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockReference"},{2,"*T1"},
        {10,"1"},{20,"2"},{30,"3"},{100,"AcDbTable"},
        {90,"2"},{91,std::to_string(rows)},{92,std::to_string(columns)}};
    for(int row=0;row<rows;++row)tags.push_back({141,"8"});
    for(int col=0;col<columns;++col)tags.push_back({142,"20"});
    for(int row=0;row<rows;++row)for(int col=0;col<columns;++col) {
        // Row 0 is a horizontal merge; rows 1–2 form a second rectangle.
        const bool anchor=col==0&&(row==0||row==1);
        const bool covered=!anchor;
        const int width=anchor?columns:1;
        const int height=anchor&&row==1?rows-1:1;
        tags.insert(tags.end(),{{171,"1"},{172,"0"},{173,covered?"1":"0"},{174,"0"},
            {175,std::to_string(width)},{176,std::to_string(height)},{177,"0"},{178,"0"},{145,"0"},
            {1,anchor?(row==0?"top":"body"):""}});
    }
    tags.insert(tags.end(),{{0,"ENDSEC"},{0,"EOF"}});return tags;
}
std::size_t legacyMergeCellField(const Tags& tags,unsigned cell,int code) {
    unsigned current=0u;bool found=false;
    for(std::size_t i=0;i<tags.size();++i) {
        if(tags[i].code==171) {if(current++==cell)found=true;else if(found)break;}
        else if(found&&tags[i].code==code)return i;
        else if(found&&tags[i].code==0)break;
    }
    throw std::runtime_error("legacy merge field absent");
}
bool legacyMergeRanges(const DRW_Table& t,const std::vector<DRW_TableMergedRange>& expected) {
    if(t.m_content.m_mergedRanges.size()!=expected.size())return false;
    for(std::size_t i=0;i<expected.size();++i) {
        const auto& a=t.m_content.m_mergedRanges[i];const auto& b=expected[i];
        if(a.m_topRow!=b.m_topRow||a.m_leftColumn!=b.m_leftColumn
            ||a.m_bottomRow!=b.m_bottomRow||a.m_rightColumn!=b.m_rightColumn)return false;
    }
    return true;
}
bool legacyMergeAllocationRetry(const std::filesystem::path& path,bool binary) {
    const auto full=legacyMergeDocument("AC1018",3,3);
    const auto begin=slot(full,0,"ACAD_TABLE")+1u;
    auto end=begin;while(end<full.size()&&full[end].code!=0)++end;
    const Tags fragment(full.begin()+static_cast<std::ptrdiff_t>(begin),
                        full.begin()+static_cast<std::ptrdiff_t>(end));
    if(!write(path,fragment,binary))return false;
    DRW_Table model;dxfTable::Decoder decoder(model);
    const auto parse=[&]() {
        std::ifstream input(path,std::ios::binary);if(binary)input.seekg(22);
        std::unique_ptr<dxfReader> reader=binary?std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
            :std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
        reader->setVersion("AC1018",true);int code=0;
        for(std::size_t i=0u;i<fragment.size();++i)
            if(!reader->readRec(&code)||!decoder.parseCode(code,reader))return false;
        return true;
    };
    if(!parse()||!model.m_content.m_mergedRanges.empty())return false;
    const auto before=symbolFailedAllocations;
    {SymbolAllocationFailure fail(0);decoder.finish();}
    if(symbolFailedAllocations==before||model.m_semanticContentComplete
        ||!model.m_content.m_mergedRanges.empty())return false;
    model=DRW_Table{};
    if(!parse())return false;
    decoder.finish();
    return model.m_semanticContentComplete
        &&legacyMergeRanges(model,{{0,0,0,2},{1,0,2,2}});
}
bool legacyMergeSparseAllocation(const std::filesystem::path& path,bool binary) {
    const auto full=legacyMergeDocument("AC1018",1,1000);
    const auto begin=slot(full,0,"ACAD_TABLE")+1u;
    auto end=begin;while(end<full.size()&&full[end].code!=0)++end;
    const Tags fragment(full.begin()+static_cast<std::ptrdiff_t>(begin),
                        full.begin()+static_cast<std::ptrdiff_t>(end));
    if(!write(path,fragment,binary))return false;
    DRW_Table model;dxfTable::Decoder decoder(model);
    std::ifstream input(path,std::ios::binary);if(binary)input.seekg(22);
    std::unique_ptr<dxfReader> reader=binary?std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
        :std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
    reader->setVersion("AC1018",true);int code=0;
    for(std::size_t i=0u;i<fragment.size();++i)
        if(!reader->readRec(&code)||!decoder.parseCode(code,reader))return false;
    const auto before=symbolFailedAllocations;
    // Only the merge finalizer is constrained: a single 1000-cell merge
    // should not require any allocation larger than this bounded allowance.
    {SymbolLargeAllocationFailure limit(2048u);decoder.finish();}
    return symbolFailedAllocations==before&&model.m_semanticContentComplete
        &&legacyMergeRanges(model,{{0,0,0,999}});
}
bool legacyTableMergeWriteTests(const std::filesystem::path& dir);
bool legacyTableNestedPresenceTests(const std::filesystem::path& dir);
bool legacyTableMergeTests(const std::filesystem::path& dir) {
    const auto path=dir/"legacy-merge.dxf";
    const auto fail=[](const std::string& reason){std::cerr<<"legacy TABLE merge: "<<reason<<'\n';return false;};
    const std::vector<DRW_TableMergedRange> expected{{0,0,0,2},{1,0,2,2}};
    unsigned positives=0u,refusals=0u,retries=0u,modern=0u,boundaries=0u,allocationRetries=0u,sparse=0u;
    for(bool binary:{false,true}) {
        const auto good=legacyMergeDocument("AC1018",3,3);
        dx_iface reader;dx_data first;
        if(!write(path,good,binary)||!reader.fileImport(path.string(),&first,false)||first.tables.size()!=1u
            ||!first.tables.front().m_semanticContentComplete||!legacyMergeRanges(first.tables.front(),expected))
            return fail("source grid/ranges");
        const auto& table=first.tables.front();
        if(table.m_content.m_rows.size()!=3u||table.m_content.m_rows[0].m_cells.size()!=3u
            ||table.m_content.m_rows[0].m_cells[0].m_isMerged
            ||table.m_content.m_rows[0].m_cells[0].m_mergedWidth!=3
            ||!table.m_content.m_rows[0].m_cells[1].m_isMerged
            ||table.m_content.m_rows[1].m_cells[0].m_isMerged
            ||table.m_content.m_rows[1].m_cells[0].m_mergedHeight!=2)return fail("raw descriptor changed");
        DRW_Table copied(table),assigned;assigned=copied;DRW_Table moved(std::move(assigned));
        if(!legacyMergeRanges(copied,expected)||!legacyMergeRanges(moved,expected))return fail("copy/assignment/move");
        const auto sourceCopy=dxfTable::Decoder::observe(copied),sourceMoved=dxfTable::Decoder::observe(moved);
        if(!(sourceCopy==sourceMoved))return fail("copy/reset observed state differs");
        ++positives;
        // Every rejection is followed by a corrected read on the same facade.
        // Incomplete records may be delivered diagnostically, but must publish no
        // partial inferred ranges and cannot assert semantic completeness.
        for(unsigned mode=0u;mode<23u;++mode) {
            auto bad=good;
            if(mode<12u) {
                const bool duplicate=mode>=6u;const unsigned offset=mode%6u;
                const unsigned index=offset/3u?1u:0u;const int codes[]={173,175,176};
                const auto pos=legacyMergeCellField(bad,index,codes[offset%3u]);
                if(duplicate) {
                    const auto duplicateField=bad[pos];
                    bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(pos),duplicateField);
                }
                else bad.erase(bad.begin()+static_cast<std::ptrdiff_t>(pos));
            } else if(mode==12u)bad[legacyMergeCellField(bad,0u,175)].value="4"; // right out of bounds
            else if(mode==13u)bad[legacyMergeCellField(bad,3u,176)].value="4"; // bottom out of bounds
            else if(mode==14u)bad[legacyMergeCellField(bad,1u,173)].value="0"; // missing covered cell
            else if(mode==15u)bad[legacyMergeCellField(bad,1u,175)].value="2"; // covered cell spans
            else if(mode==16u)bad[legacyMergeCellField(bad,0u,175)].value="2"; // orphan covered cell
            else if(mode==17u) { // competing anchor overlaps the first rectangle
                bad[legacyMergeCellField(bad,1u,173)].value="0";
                bad[legacyMergeCellField(bad,1u,175)].value="2";
                bad[legacyMergeCellField(bad,1u,176)].value="2";
            } else if(mode==18u)bad[legacyMergeCellField(bad,0u,176)].value="0"; // invalid span
            else if(mode==19u) { // one missing cell before ENDSEC
                const auto start=legacyMergeCellField(bad,8u,173)-2u;
                auto end=start+1u;while(end<bad.size()&&bad[end].code!=0)++end;
                bad.erase(bad.begin()+static_cast<std::ptrdiff_t>(start),bad.begin()+static_cast<std::ptrdiff_t>(end));
            } else if(mode==20u)bad[3].value="AC1015"; // unqualified source version
            else if(mode==21u)bad.erase(bad.begin()+2,bad.begin()+4); // no declared source version
            else bad.insert(bad.end()-2,{171,"1"}); // bare excess cell descriptor
            if(!write(path,bad,binary))return fail("malformed fixture write");
            dx_data rejected;const bool accepted=reader.fileImport(path.string(),&rejected,false);
            if(accepted) {
                if(rejected.tables.size()!=1u||rejected.tables.front().m_semanticContentComplete
                    ||!rejected.tables.front().m_content.m_mergedRanges.empty())
                    return fail("malformed profile published partial range "+std::to_string(mode));
            } else if(!rejected.tables.empty())return fail("failed import published TABLE callback");
            ++refusals;
            dx_data next;if(!write(path,good,binary)||!reader.fileImport(path.string(),&next,false)
                ||next.tables.size()!=1u||!next.tables.front().m_semanticContentComplete
                ||!legacyMergeRanges(next.tables.front(),expected))return fail("same-reader retry "+std::to_string(mode));
            ++retries;
        }
        // Keep the already accepted explicit-anchor representation on both
        // legacy and modern inputs, even though it differs from the selected
        // original producer's anchor/covered spelling.
        for(const std::string version:{"AC1018","AC1021"}) {
            auto contemporary=legacyMergeDocument(version,3,3);
            for(unsigned i=0u;i<9u;++i) {
                contemporary[legacyMergeCellField(contemporary,i,173)].value=i==0u?"1":"0";
                contemporary[legacyMergeCellField(contemporary,i,175)].value=i==0u?"2":"1";
                contemporary[legacyMergeCellField(contemporary,i,176)].value="1";
            }
            dx_data modernData;if(!write(path,contemporary,binary)||!reader.fileImport(path.string(),&modernData,false)
                ||modernData.tables.size()!=1u||!legacyMergeRanges(modernData.tables.front(),{{0,0,0,1}}))
                return fail("explicit-anchor compatibility "+version);
            ++modern;
        }
        // Exact legal width (1000) and over-bound width (1001), without a
        // giant fixture or a quadratic nested-range matrix.
        auto wide=legacyMergeDocument("AC1018",2,1000);
        const std::vector<DRW_TableMergedRange> wideRanges{{0,0,0,999},{1,0,1,999}};
        dx_data exact;if(!write(path,wide,binary)||!reader.fileImport(path.string(),&exact,false)
            ||exact.tables.size()!=1u||!exact.tables.front().m_semanticContentComplete
            ||!legacyMergeRanges(exact.tables.front(),wideRanges))return fail("exact width boundary");
        ++boundaries;
        wide[legacyMergeCellField(wide,0u,175)].value="1001";
        dx_data excess;if(!write(path,wide,binary)||reader.fileImport(path.string(),&excess,false)||!excess.tables.empty())
            return fail("over-bound width accepted");
        dx_data after;if(!write(path,good,binary)||!reader.fileImport(path.string(),&after,false)
            ||after.tables.size()!=1u||!legacyMergeRanges(after.tables.front(),expected))return fail("boundary retry");
        ++boundaries;
        if(!legacyMergeAllocationRetry(path,binary))return fail("reached finalization allocation retry");
        ++allocationRetries;
        if(!legacyMergeSparseAllocation(path,binary))return fail("sparse merge allocation allowance");
        ++sparse;
    }
    std::cout<<"legacy TABLE merge positives="<<positives<<" malformed="<<refusals
        <<" retries="<<retries<<" modern="<<modern<<" boundaries="<<boundaries
        <<" reached finalization allocation retries="<<allocationRetries<<" sparse allocation="<<sparse<<'\n';
    return positives==2u&&refusals==46u&&retries==46u&&modern==4u&&boundaries==4u&&allocationRetries==2u&&sparse==2u
        &&legacyTableMergeWriteTests(dir)&&legacyTableNestedPresenceTests(dir);
}

bool legacyTableMergeOverrideComposition(const std::filesystem::path& dir);
bool legacyTableMergeUnoccupiedSpanTests(const std::filesystem::path& dir);
bool explicitTableMergeRangeValidationTests(const std::filesystem::path& dir);
bool tableLegacyPresenceControls(const std::filesystem::path& dir);
// Emitted field and topology controls are for the selected AC1018 profile;
// the inferred source grammar is not a native rendering certificate.
bool legacyTableMergeWriteTests(const std::filesystem::path& dir) {
    const auto source=dir/"legacy-merge-write-source.dxf",output=dir/"legacy-merge-write-output.dxf";
    const auto fail=[](const std::string& why){std::cerr<<"legacy TABLE merge writer: "<<why<<'\n';return false;};
    const std::vector<DRW_TableMergedRange> ranges{{0,0,0,2},{1,0,2,2}};
    unsigned profiles=0u,refusals=0u,retries=0u,explicitControls=0u,fallbacks=0u;
    for(bool inputBinary:{false,true}) {
        const auto tags=legacyMergeDocument("AC1018",3,3);dx_iface importer;dx_data input;
        if(!write(source,tags,inputBinary)||!importer.fileImport(source.string(),&input,false)
            ||input.tables.size()!=1u||!input.tables.front().m_semanticContentComplete
            ||!legacyMergeRanges(input.tables.front(),ranges))return fail("source import");
        for(bool binary:{false,true})for(bool strict:{false,true}) {
            dx_data data;dx_iface writer;writer.cData=&data;writer.currentBlock=data.mBlock;
            DRW_Block_Record modelSpace;modelSpace.handle=0x1fu;modelSpace.name="*Model_Space";
            data.blockRecords.push_back(modelSpace);
            DRW_Table selected(input.tables.front());selected.handle=0x500u;selected.parentHandle=0x1fu;selected.name="*T500";
            writer.addTable(selected);
            auto& table=data.tables.front();const DRW_Table baseline(table);const auto identities=tableExportIdentities(data);
            const auto check=[&]() {
                dx_iface reader;dx_data readback;
                if(!reader.fileImport(output.string(),&readback,false)||readback.tables.size()!=1u
                    ||!readback.tables.front().m_semanticContentComplete
                    ||!legacyMergeRanges(readback.tables.front(),ranges))return false;
                const auto& actual=readback.tables.front();
                if(actual.m_content.m_rows.size()!=3u)return false;
                for(std::size_t row=0u;row<3u;++row) {
                    if(actual.m_content.m_rows[row].m_cells.size()!=3u)return false;
                    for(std::size_t column=0u;column<3u;++column) {
                        const auto& c=actual.m_content.m_rows[row].m_cells[column];
                        const bool anchor=column==0u&&(row==0u||row==1u);
                        if(c.m_isMerged==anchor||c.m_mergedWidth!=(anchor?3:1)
                            ||c.m_mergedHeight!=(anchor&&row==1u?2:1))return false;
                    }
                }
                const auto wire=attdefWireTags(output,binary);
                bool tableBody=false;unsigned tables=0u;std::vector<std::array<int,3>> descriptors;
                for(const auto& tag:wire) {
                    if(tag.code==0){tableBody=tag.text=="ACAD_TABLE";if(tableBody)++tables;continue;}
                    if(!tableBody)continue;
                    if(tag.code==171)descriptors.push_back({{-1,-1,-1}});
                    else if(!descriptors.empty()&&(tag.code==173||tag.code==175||tag.code==176)) {
                        if(tag.kind!=AttdefWireTag::Integer)return false;
                        const unsigned field=tag.code==173?0u:(tag.code==175?1u:2u);
                        if(descriptors.back()[field]!=-1)return false;
                        descriptors.back()[field]=static_cast<int>(tag.integer);
                    }
                }
                if(tables!=1u||descriptors.size()!=9u)return false;
                for(std::size_t i=0u;i<descriptors.size();++i) {
                    const bool anchor=i==0u||i==3u;
                    const std::array<int,3> expected{{anchor?0:1,anchor?3:1,i==3u?2:1}};
                    if(descriptors[i]!=expected)return false;
                }
                return true;
            };
            if(!writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)||!check()
                ||identities!=tableExportIdentities(data)||!DxIfaceTableSourceTestAccess::sameTableBody(baseline,table))
                return fail("selected output/readback");
            ++profiles;
            for(unsigned fault=0u;fault<12u;++fault) {
                if(fault==0u)table.m_content.m_mergedRanges.clear();
                if(fault==1u)table.m_content.m_mergedRanges.pop_back();
                if(fault==2u){const auto range=table.m_content.m_mergedRanges.front();table.m_content.m_mergedRanges.push_back(range);}
                if(fault==3u)--table.m_content.m_mergedRanges[0].m_rightColumn;
                if(fault==4u)table.m_content.m_mergedRanges[0].m_rightColumn=3;
                if(fault==5u)table.m_content.m_rows[0].m_cells[1].m_isMerged=false;
                if(fault==6u)table.m_content.m_rows[0].m_cells[1].m_mergedWidth=2;
                if(fault==7u)table.m_content.m_rows[0].m_cells[0].m_mergedWidth=0;
                if(fault==8u)table.m_semanticContentComplete=false;
                if(fault==9u)table.m_content.m_rows[0].m_cells[0].m_overrideFlags=0x4000u;
                if(fault==10u)table.m_content.m_rows[0].m_cells[0].m_borders[0].m_visible=false;
                const DRW_Table invalid(table);const auto invalidIdentities=tableExportIdentities(data);
                const DRW::Version target=fault==11u?DRW::AC1021:DRW::AC1018;
                if(!tableExportSentinel(output,true))return fail("sentinel creation");
                const bool exported=writer.fileExport(output.string(),target,binary,&data,false,strict);
                if(invalidIdentities!=tableExportIdentities(data)
                    ||!DxIfaceTableSourceTestAccess::sameTableBody(invalid,table))
                    return fail("invalid profile changed caller "+std::to_string(fault));
                if(fault==8u) {
                    // The adapter intentionally retains its historical INSERT
                    // projection for an incomplete TABLE snapshot.
                    if(strict) {
                        // This authored projection has no corresponding block;
                        // strict ownership mode rejects the unresolved target.
                        if(exported||!tableExportSentinel(output))return fail("strict incomplete TABLE target");
                        ++refusals;
                    } else {
                        if(!exported)return fail("incomplete TABLE projection failed");
                        const auto wire=attdefWireTags(output,binary);unsigned tables=0u,projections=0u;
                        bool insert=false;
                        for(const auto& tag:wire) {
                            if(tag.code==0){insert=tag.text=="INSERT";if(tag.text=="ACAD_TABLE")++tables;}
                            else if(insert&&tag.code==2&&tag.text=="*T500")++projections;
                        }
                        if(tables!=0u||projections!=1u)return fail("incomplete TABLE projection output");
                        ++fallbacks;
                    }
                } else {
                    if(exported||!tableExportSentinel(output))
                        return fail("invalid profile or target escaped "+std::to_string(fault));
                    ++refusals;
                }
                table=baseline;
                if(!writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)||!check()
                    ||identities!=tableExportIdentities(data)||!DxIfaceTableSourceTestAccess::sameTableBody(baseline,table))
                    return fail("same-writer corrected retry "+std::to_string(fault));
                ++retries;
            }
            // Existing AC1018 explicit-anchor representation remains writable.
            DRW_Table previous(baseline);previous.m_content.m_mergedRanges={{0,0,0,1}};
            for(std::size_t row=0u;row<3u;++row)for(std::size_t column=0u;column<3u;++column) {
                auto& cell=previous.m_content.m_rows[row].m_cells[column];const bool anchor=row==0u&&column==0u;
                cell.m_isMerged=anchor;cell.m_mergedWidth=anchor?2:1;cell.m_mergedHeight=1;
            }
            table=previous;
            if(!writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict))return fail("explicit-anchor output");
            dx_iface reader;dx_data readback;
            if(!reader.fileImport(output.string(),&readback,false)||readback.tables.size()!=1u
                ||!legacyMergeRanges(readback.tables.front(),{{0,0,0,1}}))return fail("explicit-anchor readback");
            ++explicitControls;
        }
    }
    std::cout<<"legacy TABLE merge writer profiles="<<profiles<<" refusals="<<refusals
        <<" corrected retries="<<retries<<" explicit-anchor controls="<<explicitControls<<" incomplete projections="<<fallbacks<<'\n';
    return profiles==8u&&refusals==92u&&retries==96u&&explicitControls==8u&&fallbacks==4u
        &&legacyTableMergeOverrideComposition(dir)&&legacyTableMergeUnoccupiedSpanTests(dir)
        &&explicitTableMergeRangeValidationTests(dir)&&tableLegacyPresenceControls(dir);
}
// Composition of the already admitted legacy merge and text override profiles.
// Raw field correspondence is tested; no native appearance is claimed.
bool legacyTableMergeOverrideComposition(const std::filesystem::path& dir) {
    const auto inputPath=dir/"legacy-merge-composition-source.dxf",output=dir/"legacy-merge-composition-output.dxf";
    const auto fail=[](const std::string& why){std::cerr<<"legacy TABLE merge composition: "<<why<<'\n';return false;};
    const std::vector<DRW_TableMergedRange> ranges{{0,0,0,2},{1,0,2,2}};
    unsigned positives=0u,refusals=0u,retries=0u;
    for(bool inputBinary:{false,true}) {
        auto tags=legacyMergeDocument("AC1018",3,3);
        // Insert in reverse cell order to keep each physical source slot stable.
        for(unsigned index:{3u,1u,0u}) {
            const int mask=index==0u?32:33;
            tags[legacyMergeCellField(tags,index,177)].value=std::to_string(mask);
            const auto text=legacyMergeCellField(tags,index,1);
            Tags extra;
            if(index!=0u)extra.push_back({170,index==1u?"5":"9"});
            extra.push_back({140,index==3u?"3":"4"});
            tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(text+1u),extra.begin(),extra.end());
        }
        dx_iface importer;dx_data source;
        if(!write(inputPath,tags,inputBinary)||!importer.fileImport(inputPath.string(),&source,false)
            ||source.tables.size()!=1u||!source.tables.front().m_semanticContentComplete
            ||!legacyMergeRanges(source.tables.front(),ranges))return fail("source import");
        for(bool binary:{false,true})for(bool strict:{false,true}) {
            dx_data data;dx_iface writer;writer.cData=&data;writer.currentBlock=data.mBlock;
            DRW_Block_Record modelSpace;modelSpace.handle=0x1fu;modelSpace.name="*Model_Space";
            data.blockRecords.push_back(modelSpace);
            DRW_Table selected(source.tables.front());selected.handle=0x500u;selected.parentHandle=0x1fu;selected.name="*T500";
            writer.addTable(selected);auto& table=data.tables.front();
            const DRW_Table baseline(table);const auto identities=tableExportIdentities(data);
            const auto check=[&]() {
                dx_iface reader;dx_data readback;
                if(!reader.fileImport(output.string(),&readback,false)||readback.tables.size()!=1u
                    ||!readback.tables.front().m_semanticContentComplete||!legacyMergeRanges(readback.tables.front(),ranges))return false;
                const auto& actual=readback.tables.front();
                if(actual.m_content.m_rows.size()!=3u)return false;
                const auto& a=actual.m_content.m_rows[0].m_cells[0];const auto& b=actual.m_content.m_rows[0].m_cells[1];
                const auto& c=actual.m_content.m_rows[1].m_cells[0];
                if(a.m_overrideFlags!=0x20u||a.m_textHeight!=4.0||a.m_textAlignment!=0
                    ||b.m_overrideFlags!=0x21u||b.m_textHeight!=4.0||b.m_textAlignment!=5
                    ||c.m_overrideFlags!=0x21u||c.m_textHeight!=3.0||c.m_textAlignment!=9)return false;
                struct Fields{int mask=-1,alignment=-1;double height=-1.0;};
                std::vector<Fields> fields;bool inTable=false;unsigned tables=0u;
                for(const auto& tag:attdefWireTags(output,binary)) {
                    if(tag.code==0){inTable=tag.text=="ACAD_TABLE";if(inTable)++tables;continue;}
                    if(!inTable)continue;
                    if(tag.code==171)fields.push_back({});
                    else if(!fields.empty()&&tag.code==177){if(fields.back().mask!=-1)return false;fields.back().mask=static_cast<int>(tag.integer);}
                    else if(!fields.empty()&&tag.code==170){if(fields.back().alignment!=-1)return false;fields.back().alignment=static_cast<int>(tag.integer);}
                    else if(!fields.empty()&&tag.code==140){if(fields.back().height!=-1.0)return false;fields.back().height=tag.real;}
                }
                if(tables!=1u||fields.size()!=9u)return false;
                for(std::size_t i=0u;i<fields.size();++i) {
                    const int mask=i==0u?32:((i==1u||i==3u)?33:-1);
                    const int alignment=i==1u?5:(i==3u?9:-1);
                    const double height=i==3u?3.0:((i==0u||i==1u)?4.0:-1.0);
                    if(fields[i].mask!=mask||fields[i].alignment!=alignment||fields[i].height!=height)return false;
                }
                return true;
            };
            if(!writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)||!check()
                ||identities!=tableExportIdentities(data)||!DxIfaceTableSourceTestAccess::sameTableBody(baseline,table))
                return fail("combined output/readback");
            ++positives;
            for(unsigned fault=0u;fault<4u;++fault) {
                if(fault==0u)table.m_content.m_rows[0].m_cells[1].m_textHeight=0.0;
                if(fault==1u)table.m_content.m_rows[0].m_cells[1].m_textAlignment=0;
                if(fault==2u)table.m_content.m_rows[0].m_cells[0].m_textHeight=0.0;
                if(fault==3u)table.m_content.m_rows[0].m_cells[0].m_overrideFlags|=0x4000u;
                const DRW_Table invalid(table);const auto saved=tableExportIdentities(data);
                if(!tableExportSentinel(output,true)||writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)
                    ||!tableExportSentinel(output)||saved!=tableExportIdentities(data)
                    ||!DxIfaceTableSourceTestAccess::sameTableBody(invalid,table))return fail("invalid override escaped "+std::to_string(fault));
                ++refusals;table=baseline;
                if(!writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)||!check()
                    ||identities!=tableExportIdentities(data)||!DxIfaceTableSourceTestAccess::sameTableBody(baseline,table))
                    return fail("corrected retry "+std::to_string(fault));
                ++retries;
            }
        }
    }
    std::cout<<"legacy TABLE merge/override compositions="<<positives<<" refusals="<<refusals<<" retries="<<retries<<'\n';
    return positives==8u&&refusals==32u&&retries==32u;
}

// An unoccupied cell in the inferred AC1018 profile must already have unit
// spans; otherwise emitting 175/176 as 1 would silently alter its typed values.
bool legacyTableMergeUnoccupiedSpanTests(const std::filesystem::path& dir) {
    const auto inputPath=dir/"legacy-merge-unoccupied-source.dxf",output=dir/"legacy-merge-unoccupied-output.dxf";
    const auto fail=[](const std::string& why){std::cerr<<"legacy TABLE merge unoccupied span: "<<why<<'\n';return false;};
    unsigned positives=0u,refusals=0u,retries=0u;
    for(bool inputBinary:{false,true}) {
        auto tags=legacyMergeDocument("AC1018",1,3);
        tags[legacyMergeCellField(tags,0u,175)].value="2";
        tags[legacyMergeCellField(tags,2u,173)].value="0";
        dx_iface importer;dx_data source;
        if(!write(inputPath,tags,inputBinary)||!importer.fileImport(inputPath.string(),&source,false)
            ||source.tables.size()!=1u||!source.tables.front().m_semanticContentComplete
            ||!legacyMergeRanges(source.tables.front(),{{0,0,0,1}}))return fail("source import");
        for(bool binary:{false,true})for(bool strict:{false,true}) {
            dx_data data;dx_iface writer;writer.cData=&data;writer.currentBlock=data.mBlock;
            DRW_Block_Record modelSpace;modelSpace.handle=0x1fu;modelSpace.name="*Model_Space";
            data.blockRecords.push_back(modelSpace);
            DRW_Table selected(source.tables.front());selected.handle=0x500u;selected.parentHandle=0x1fu;selected.name="*T500";
            writer.addTable(selected);auto& table=data.tables.front();const DRW_Table baseline(table);
            const auto identities=tableExportIdentities(data);
            const auto check=[&]() {
                dx_iface reader;dx_data after;
                if(!reader.fileImport(output.string(),&after,false)||after.tables.size()!=1u
                    ||!after.tables.front().m_semanticContentComplete
                    ||!legacyMergeRanges(after.tables.front(),{{0,0,0,1}})
                    ||after.tables.front().m_content.m_rows.size()!=1u
                    ||after.tables.front().m_content.m_rows[0].m_cells.size()!=3u)return false;
                const auto& cell=after.tables.front().m_content.m_rows[0].m_cells[2];
                return !cell.m_isMerged&&cell.m_mergedWidth==1&&cell.m_mergedHeight==1;
            };
            if(!writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)||!check()
                ||identities!=tableExportIdentities(data)||!DxIfaceTableSourceTestAccess::sameTableBody(baseline,table))
                return fail("unit-span baseline");
            ++positives;
            for(bool width:{false,true}) {
                auto& cell=table.m_content.m_rows[0].m_cells[2];
                if(width)cell.m_mergedWidth=0;else cell.m_mergedHeight=0;
                const DRW_Table invalid(table);const auto saved=tableExportIdentities(data);
                if(!tableExportSentinel(output,true)||writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)
                    ||!tableExportSentinel(output)||saved!=tableExportIdentities(data)
                    ||!DxIfaceTableSourceTestAccess::sameTableBody(invalid,table))return fail("zero span escaped or changed caller");
                ++refusals;table=baseline;
                if(!writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)||!check()
                    ||identities!=tableExportIdentities(data)||!DxIfaceTableSourceTestAccess::sameTableBody(baseline,table))
                    return fail("corrected retry");
                ++retries;
            }
        }
    }
    std::cout<<"legacy TABLE merge unoccupied spans="<<positives<<" zero-span refusals="<<refusals<<" retries="<<retries<<'\n';
    return positives==8u&&refusals==16u&&retries==16u;
}

// The explicit-anchor compatibility spelling must also reject malformed
// live rectangles before writing, and preserve the caller for retry.
bool explicitTableMergeRangeValidationTests(const std::filesystem::path& dir) {
    const auto input=dir/"explicit-merge-range-source.dxf", output=dir/"explicit-merge-range-output.dxf";
    const auto fail=[](const std::string& why){std::cerr<<"explicit TABLE merge ranges: "<<why<<'\n';return false;};
    unsigned positives=0u,refusals=0u,retries=0u;
    for(bool inputBinary:{false,true}) {
        const auto tags=legacyMergeDocument("AC1018",3,3);dx_iface importer;dx_data source;
        if(!write(input,tags,inputBinary)||!importer.fileImport(input.string(),&source,false)
            ||source.tables.size()!=1u||!source.tables.front().m_semanticContentComplete)return fail("source");
        for(bool binary:{false,true})for(bool strict:{false,true}) {
            dx_data data;dx_iface writer;writer.cData=&data;writer.currentBlock=data.mBlock;
            DRW_Block_Record modelSpace;modelSpace.handle=0x1fu;modelSpace.name="*Model_Space";data.blockRecords.push_back(modelSpace);
            DRW_Table selected(source.tables.front());selected.handle=0x500u;selected.parentHandle=0x1fu;selected.name="*T500";
            for(auto& row:selected.m_content.m_rows)for(auto& cell:row.m_cells) {
                cell.m_isMerged=false;cell.m_mergedWidth=1;cell.m_mergedHeight=1;
            }
            selected.m_content.m_rows[0].m_cells[0].m_isMerged=true;
            selected.m_content.m_rows[0].m_cells[0].m_mergedWidth=2;
            selected.m_content.m_mergedRanges={{0,0,0,1}};writer.addTable(selected);
            auto& table=data.tables.front();const DRW_Table baseline(table);const auto identities=tableExportIdentities(data);
            const auto check=[&]() {
                dx_iface reader;dx_data after;
                return reader.fileImport(output.string(),&after,false)&&after.tables.size()==1u
                    &&after.tables.front().m_semanticContentComplete
                    &&legacyMergeRanges(after.tables.front(),{{0,0,0,1}});
            };
            if(!writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)||!check())return fail("baseline");
            ++positives;
            // No-merge tables retain the established unmerged output path.
            table.m_content.m_mergedRanges.clear();
            table.m_content.m_rows[0].m_cells[0].m_isMerged=false;
            table.m_content.m_rows[0].m_cells[0].m_mergedWidth=1;
            dx_iface unmergedReader;dx_data unmerged;
            if(!writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)
                ||!unmergedReader.fileImport(output.string(),&unmerged,false)
                ||unmerged.tables.size()!=1u||!unmerged.tables.front().m_semanticContentComplete
                ||!unmerged.tables.front().m_content.m_mergedRanges.empty())return fail("unmerged baseline");
            table=baseline;
            for(unsigned fault=0u;fault<8u;++fault) {
                auto& cells=table.m_content.m_rows[0].m_cells;
                if(fault==0u) { cells[0].m_isMerged=false;cells[0].m_mergedWidth=1;cells[1].m_isMerged=true;cells[1].m_mergedWidth=3;table.m_content.m_mergedRanges={{0,1,0,3}}; }
                if(fault==1u) { cells[0].m_isMerged=false;cells[0].m_mergedWidth=1;auto& anchor=table.m_content.m_rows[1].m_cells[0];anchor.m_isMerged=true;anchor.m_mergedHeight=3;table.m_content.m_mergedRanges={{1,0,3,0}}; }
                if(fault==2u) { cells[1].m_isMerged=true;cells[1].m_mergedWidth=2;table.m_content.m_mergedRanges.push_back({0,1,0,2}); }
                if(fault==3u) { cells[2].m_isMerged=true;table.m_content.m_mergedRanges.push_back({0,0,0,1}); }
                if(fault==4u) table.m_content.m_mergedRanges[0].m_leftColumn=-1;
                if(fault==5u) table.m_content.m_mergedRanges[0].m_rightColumn=0;
                if(fault==6u) cells[2].m_mergedWidth=0;
                if(fault==7u) cells[2].m_mergedHeight=0;
                const DRW_Table invalid(table);const auto saved=tableExportIdentities(data);
                if(!tableExportSentinel(output,true)||writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)
                    ||!tableExportSentinel(output)||saved!=tableExportIdentities(data)
                    ||!DxIfaceTableSourceTestAccess::sameTableBody(invalid,table))return fail("invalid geometry escaped/changed caller "+std::to_string(fault));
                ++refusals;table=baseline;
                if(!writer.fileExport(output.string(),DRW::AC1018,binary,&data,false,strict)||!check()
                    ||identities!=tableExportIdentities(data)||!DxIfaceTableSourceTestAccess::sameTableBody(baseline,table))return fail("corrected retry");
                ++retries;
            }
        }
    }
    std::cout<<"explicit TABLE merge baselines="<<positives<<" invalid="<<refusals<<" retries="<<retries<<'\n';
    return positives==8u&&refusals==64u&&retries==64u;
}

// Authored source-presence controls; no native rendering semantics are inferred.
struct TableLegacyPresenceMutationProbe final:dx_iface {
    bool mutate=false;
    void addDxfTableSource(const DRW_Table& v,const DRW_RawDxfObject& raw) override {
        if(mutate)const_cast<DRW_Table&>(v).m_content.m_rows[0].m_cells[0].m_contents.front().m_text+="X";
        dx_iface::addDxfTableSource(v,raw);
    }
};
struct TableLegacyDirectPresenceProbe final:dx_iface {
    std::vector<std::uint8_t> spellings;
    void writeEntities() override {
        if(cData->tables.size()!=1u) { (void)dxfW->writeTable(nullptr); return; }
        auto& table=cData->tables.front();const auto sourceHandle=table.handle;
        if(dxfW->writeTableWithLegacyZeroOverrides(&table,spellings))
            DxIfaceTableSourceTestAccess::markTableEmitted(*this,sourceHandle);
    }
};
bool tableLegacyPresenceControls(const std::filesystem::path& dir) {
    const auto source=dir/"legacy-presence-source.dxf", out=dir/"legacy-presence-out.dxf";
    const auto fail=[](const char* why){std::cerr<<"legacy TABLE zero presence: "<<why<<'\n';return false;};
    unsigned positives=0, refusals=0, retries=0;
    for(bool sourceBinary:{false,true}) {
        auto tags=legacyMergeDocument("AC1018",1,3);
        tags[legacyMergeCellField(tags,0,175)].value="2";
        tags[legacyMergeCellField(tags,2,173)].value="0";
        tags[legacyMergeCellField(tags,0,1)].value=std::string("Caf")+char(0xE9);
        const auto headerEnd=std::find_if(tags.begin(),tags.end(),[](const auto& t){return t.code==0&&t.value=="ENDSEC";});
        if(headerEnd==tags.end())return fail("header end");
        tags.insert(headerEnd,{{9,"$DWGCODEPAGE"},{3,"ANSI_1252"}});
        auto at=legacyMergeCellField(tags,1,177);
        tags.erase(tags.begin()+static_cast<std::ptrdiff_t>(at),tags.begin()+static_cast<std::ptrdiff_t>(at+2));
        const auto self=std::find_if(tags.begin(),tags.end(),[](const auto& t){return t.code==5&&t.value=="4F2";});
        if(self==tags.end())return fail("source handle");
        tags.insert(self+1,{330,"1F"});
        if(!write(source,tags,sourceBinary))return fail("source output");
        for(bool outputBinary:{false,true})for(bool strict:{false,true}) {
            dx_iface io;dx_data data;
            if(!io.fileImport(source.string(),&data,false)||data.tables.size()!=1u
                ||!data.tables.front().m_semanticContentComplete)return fail("source import");
            DRW_Block_Record model;model.handle=0x1fu;model.name="*Model_Space";data.blockRecords.push_back(model);
            const auto identities=tableExportIdentities(data);auto& table=data.tables.front();const DRW_Table baseline(table);
            const auto check=[&]() {
                const auto wire=attdefWireTags(out,outputBinary);
                bool body=false;unsigned current=0,tables=0,owners=0;std::vector<std::vector<int>> fields;
                for(const auto& t:wire) {
                    if(t.code==0){body=t.text=="ACAD_TABLE";if(body)++tables;continue;}
                    if(!body)continue;
                    if(t.code==330){if(t.text!="1F")return false;++owners;}
                    if(t.code==171){fields.emplace_back();++current;}
                    if(current&& (t.code==177||t.code==178)) {
                        if(t.kind!=AttdefWireTag::Integer||t.integer!=0)return false;
                        fields.back().push_back(t.code);
                    }
                }
                dx_iface reader;dx_data back;
                return tables==1u&&owners==1u&&fields.size()==3u&&fields[0]==std::vector<int>{177,178}
                    &&fields[1].empty()&&fields[2]==std::vector<int>{177,178}
                    &&reader.fileImport(out.string(),&back,false)&&back.tables.size()==1u
                    &&back.tables.front().m_semanticContentComplete
                    &&back.tables.front().m_content.m_rows[0].m_cells[0].m_contents.front().m_text
                        ==baseline.m_content.m_rows[0].m_cells[0].m_contents.front().m_text
                    &&legacyMergeRanges(back.tables.front(),{{0,0,0,1}});
            };
            if(!io.fileExport(out.string(),DRW::AC1018,outputBinary,&data,false,strict)||!check()
                ||identities!=tableExportIdentities(data)||!DxIfaceTableSourceTestAccess::sameTableBody(baseline,table))return fail("exact output");
            ++positives;
            // An identity-preserving body mutation cannot inherit stale source
            // presence. Correcting it must permit the same io/data retry.
            for(unsigned fault=0;fault<3u;++fault) {
                if(fault==0)table.m_content.m_rows[0].m_cells[0].m_contents.front().m_text+="X";
                if(fault==1)table.m_semanticContentComplete=false;
                if(fault==2)++table.handle;
                const DRW_Table changed(table);const auto saved=tableExportIdentities(data);
                if(!tableExportSentinel(out,true)||io.fileExport(out.string(),DRW::AC1018,outputBinary,&data,false,strict)
                    ||!tableExportSentinel(out)||saved!=tableExportIdentities(data)
                    ||!DxIfaceTableSourceTestAccess::sameTableBody(changed,table))return fail("stale mutation");
                ++refusals;table=baseline;
                if(!io.fileExport(out.string(),DRW::AC1018,outputBinary,&data,false,strict)||!check())return fail("retry");
                ++retries;
            }
        }
        // Ambiguous or incomplete override spelling cannot be authenticated.
        for(unsigned fault=0u;fault<4u;++fault) {
            auto bad=tags;const auto flag=legacyMergeCellField(bad,0,177);
            if(fault==0u)bad.insert(bad.begin()+static_cast<std::ptrdiff_t>(flag),{177,"0"});
            if(fault==1u)bad.erase(bad.begin()+static_cast<std::ptrdiff_t>(flag+1u));
            if(fault==2u)std::swap(bad[flag],bad[flag+1u]);
            if(fault==3u)bad[flag+1u].value="1";
            if(!write(source,bad,sourceBinary))return fail("invalid source output");
            dx_iface io;dx_data broken;
            if(!io.fileImport(source.string(),&broken,false)||broken.tables.size()!=1u)return fail("invalid source import");
            DRW_Block_Record model;model.handle=0x1fu;model.name="*Model_Space";broken.blockRecords.push_back(model);
            if(!tableExportSentinel(out,true)||io.fileExport(out.string(),DRW::AC1018,sourceBinary,&broken,false,false)
                ||!tableExportSentinel(out))return fail("invalid source escaped");
            dx_data fixed;if(!write(source,tags,sourceBinary)||!io.fileImport(source.string(),&fixed,false)
                ||fixed.tables.size()!=1u)return fail("invalid source retry import");
            fixed.blockRecords.push_back(model);
            if(!io.fileExport(out.string(),DRW::AC1018,sourceBinary,&fixed,false,false))return fail("invalid source retry export");
        }
        // The source-work limit is cumulative across selected TABLE records,
        // rather than resetting the replay budget for each one.
        if(!sourceBinary) {
            using A=DxIfaceTableSourceTestAccess;dx_iface io;dx_data one;
            if(!write(source,tags,false)||!io.fileImport(source.string(),&one,false))return fail("work baseline import");
            const auto work=A::tableSourceWork(one);
            if(!work||work>=4000000u)return fail("work baseline accounting");
            dx_data oneLimited;A::limits(oneLimited,100000u,4000000u,64u*1024u*1024u,work);
            if(!io.fileImport(source.string(),&oneLimited,false)
                ||dx_iface::tableSourceReport(oneLimited).state==dx_iface::TableSourceState::CaptureFailed)
                return fail("single table work budget");
            dx_data oneBelow;A::limits(oneBelow,100000u,4000000u,64u*1024u*1024u,work-1u);
            if(!io.fileImport(source.string(),&oneBelow,false)
                ||dx_iface::tableSourceReport(oneBelow).state!=dx_iface::TableSourceState::CaptureFailed)
                return fail("single table one-below work budget");
            auto two=tags;const auto begin=slot(two,0,"ACAD_TABLE");
            const auto last=std::find_if(two.begin()+static_cast<std::ptrdiff_t>(begin),two.end(),[](const auto& t){return t.code==0&&t.value=="ENDSEC";});
            if(last==two.end())return fail("work source end");
            const auto end=static_cast<std::size_t>(last-two.begin());
            Tags second(two.begin()+static_cast<std::ptrdiff_t>(begin),last);
            for(auto& field:second)if(field.code==5)field.value="4F3";else if(field.code==2)field.value="*T2";
            two.insert(two.begin()+static_cast<std::ptrdiff_t>(end),second.begin(),second.end());
            dx_data limited;A::limits(limited,100000u,4000000u,64u*1024u*1024u,work);
            if(!write(source,two,false)||!io.fileImport(source.string(),&limited,false)
                ||dx_iface::tableSourceReport(limited).state!=dx_iface::TableSourceState::CaptureFailed)return fail("cumulative work budget");
            DRW_Block_Record model;model.handle=0x1fu;model.name="*Model_Space";limited.blockRecords.push_back(model);
            if(!tableExportSentinel(out,true)||io.fileExport(out.string(),DRW::AC1018,false,&limited,false,false)
                ||!tableExportSentinel(out))return fail("work failure output");
            dx_data fresh;if(!write(source,tags,false)||!io.fileImport(source.string(),&fresh,false))return fail("work retry import");
            fresh.blockRecords.push_back(model);
            if(!io.fileExport(out.string(),DRW::AC1018,false,&fresh,false,false))return fail("work retry export");
        }
        // A subclass can mutate the parser-owned model before forwarding the
        // callback. Bounded source re-decode must detect it, and a fresh import
        // through the same adapter must recover.
        {
            TableLegacyPresenceMutationProbe io;io.mutate=true;dx_data data;
            if(!io.fileImport(source.string(),&data,false)||data.tables.size()!=1u)return fail("mutated import");
            DRW_Block_Record model;model.handle=0x1fu;model.name="*Model_Space";data.blockRecords.push_back(model);
            if(!tableExportSentinel(out,true)||io.fileExport(out.string(),DRW::AC1018,false,&data,false,false)
                ||!tableExportSentinel(out))return fail("source mutation escaped");
            io.mutate=false;dx_data retry;
            if(!io.fileImport(source.string(),&retry,false)||retry.tables.size()!=1u)return fail("source retry import");
            retry.blockRecords.push_back(model);
            if(!io.fileExport(out.string(),DRW::AC1018,false,&retry,false,false))return fail("source retry export");
        }
    }
    // Direct callers receive ordinary transactional rejection for malformed
    // vector length, values, nonzero flag conflicts and unsupported target.
    {
        auto tags=legacyMergeDocument("AC1018",1,3);
        tags[legacyMergeCellField(tags,0,175)].value="2";
        tags[legacyMergeCellField(tags,2,173)].value="0";
        if(!write(source,tags,false))return fail("direct source");
        dx_iface importer;dx_data input;if(!importer.fileImport(source.string(),&input,false)||input.tables.size()!=1u)return fail("direct import");
        for(bool binary:{false,true}) {
            TableLegacyDirectPresenceProbe direct;dx_data data;direct.cData=&data;direct.currentBlock=data.mBlock;
            DRW_Block_Record model;model.handle=0x1fu;model.name="*Model_Space";data.blockRecords.push_back(model);
            DRW_Table selected(input.tables.front());selected.handle=0x500u;selected.parentHandle=0x1fu;selected.name="*T500";
            direct.addTable(selected);const DRW_Table baseline(data.tables.front());
            for(unsigned fault=0u;fault<3u;++fault) {
                direct.spellings=fault==0u?std::vector<std::uint8_t>{1u}:std::vector<std::uint8_t>{1u,0u,fault==1u?std::uint8_t{2u}:std::uint8_t{1u}};
                if(fault==2u)data.tables.front().m_content.m_rows[0].m_cells[2].m_overrideFlags=1u;
                if(!tableExportSentinel(out,true)||direct.fileExport(out.string(),DRW::AC1018,binary,&data,false,false)
                    ||!tableExportSentinel(out))return fail("direct invalid escaped");
                data.tables.front()=baseline;direct.spellings={1u,0u,1u};
                if(!direct.fileExport(out.string(),DRW::AC1018,binary,&data,false,false))return fail("direct retry");
            }
            data.tables.front()=baseline;direct.spellings={1u,0u,1u};
            if(!tableExportSentinel(out,true)||direct.fileExport(out.string(),DRW::AC1021,binary,&data,false,false)
                ||!tableExportSentinel(out))return fail("direct wrong target");
        }
    }
    return positives==8u&&refusals==24u&&retries==24u;
}

// Application groups may reuse 100 and 177 for their own payload. Only host
// fields select the source-presence path.
bool legacyTableNestedPresenceTests(const std::filesystem::path& dir) {
    const auto source=dir/"legacy-nested-source.dxf",output=dir/"legacy-nested-output.dxf";
    const auto fail=[](const char* why){std::cerr<<"legacy TABLE nested presence: "<<why<<'\n';return false;};
    unsigned accepted=0u,refused=0u,retries=0u;
    for(bool inputBinary:{false,true})for(bool outputBinary:{false,true})for(bool strict:{false,true}) {
        auto make=[](bool hostZero) {
            auto tags=legacyMergeDocument("AC1018",1,1);
            const auto handle=slot(tags,5,"4F2");tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(handle+1u),{330,"1F"});
            const auto table=slot(tags,100,"AcDbTable");
            const Tags app=hostZero?Tags{{102,"{APP_SCOPE"},{100,"VendorSubtype"},{102,"}"}}
                :Tags{{102,"{APP_SCOPE"},{177,"0"},{102,"}"}};
            tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(table+1u),app.begin(),app.end());
            const auto flag=legacyMergeCellField(tags,0u,177);tags[flag].value=hostZero?"0":"32";
            if(!hostZero) {const auto text=legacyMergeCellField(tags,0u,1);tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(text+1u),{140,"4"});}
            return tags;
        };
        const auto good=make(false),bad=make(true);dx_iface io;dx_data data;
        if(!write(source,good,inputBinary)||!io.fileImport(source.string(),&data,false)||data.tables.size()!=1u
            ||!data.tables.front().m_semanticContentComplete)return fail("nonzero host import");
        DRW_Block_Record record;record.handle=0x1fu;record.name="*Model_Space";data.blockRecords.push_back(record);
        if(!io.fileExport(output.string(),DRW::AC1018,outputBinary,&data,false,strict))return fail("nested zero treated as host");
        dx_iface check;dx_data result;
        if(!check.fileImport(output.string(),&result,false)||result.tables.size()!=1u
            ||result.tables.front().m_content.m_rows[0].m_cells[0].m_overrideFlags!=32u
            ||result.tables.front().m_content.m_rows[0].m_cells[0].m_textHeight!=4.0)return fail("nonzero host readback");
        ++accepted;
        dx_data invalid;
        if(!write(source,bad,inputBinary)||!io.fileImport(source.string(),&invalid,false)||invalid.tables.size()!=1u)
            return fail("nested subtype import");
        invalid.blockRecords.push_back(record);const auto ids=tableExportIdentities(invalid);const DRW_Table baseline(invalid.tables.front());
        if(!tableExportSentinel(output,true)||io.fileExport(output.string(),DRW::AC1018,outputBinary,&invalid,false,strict)
            ||!tableExportSentinel(output)||ids!=tableExportIdentities(invalid)
            ||!DxIfaceTableSourceTestAccess::sameTableBody(baseline,invalid.tables.front()))return fail("hidden host zero escaped");
        ++refused;dx_data retry;
        if(!write(source,good,inputBinary)||!io.fileImport(source.string(),&retry,false)||retry.tables.size()!=1u)return fail("retry import");
        retry.blockRecords.push_back(record);
        if(!io.fileExport(output.string(),DRW::AC1018,outputBinary,&retry,false,strict))return fail("retry export");
        ++retries;
    }
    std::cout<<"legacy TABLE nested accepted="<<accepted<<" refused="<<refused<<" retries="<<retries<<'\n';
    return accepted==8u&&refused==8u&&retries==8u;
}
