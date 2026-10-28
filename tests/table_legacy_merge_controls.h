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
bool legacyTableMergeTests(const std::filesystem::path& dir) {
    const auto path=dir/"legacy-merge.dxf";
    const auto fail=[](const std::string& reason){std::cerr<<"legacy TABLE merge: "<<reason<<'\n';return false;};
    const std::vector<DRW_TableMergedRange> expected{{0,0,0,2},{1,0,2,2}};
    unsigned positives=0u,refusals=0u,retries=0u,modern=0u,boundaries=0u,allocationRetries=0u;
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
        for(unsigned mode=0u;mode<22u;++mode) {
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
            else bad.erase(bad.begin()+2,bad.begin()+4); // no declared source version
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
    }
    std::cout<<"legacy TABLE merge positives="<<positives<<" malformed="<<refusals
        <<" retries="<<retries<<" modern="<<modern<<" boundaries="<<boundaries
        <<" reached finalization allocation retries="<<allocationRetries<<'\n';
    return positives==2u&&refusals==44u&&retries==44u&&modern==4u&&boundaries==4u&&allocationRetries==2u;
}
