// SPDX-License-Identifier: GPL-2.0-or-later
// Runtime-authored TABLE replay controls; no external drawing payloads.
struct TableBodyProbe final:dx_iface {
    std::function<void(DRW_Table&)> mutation;bool beforeSource=false;std::uint32_t mutateHandle=0u;
    void addDxfTableSource(const DRW_Table& v,const DRW_RawDxfObject& raw) override {
        const bool selected=mutation&&(!mutateHandle||mutateHandle==v.handle);
        if(selected&&beforeSource)mutation(const_cast<DRW_Table&>(v));
        dx_iface::addDxfTableSource(v,raw);
        if(selected&&!beforeSource)mutation(const_cast<DRW_Table&>(v));
    }
};
Tags tableBodyRichDocument(const std::string& version,bool blocks) {
    auto tags=tableBlockSourceDocument(version,blocks,1u);
    tags[blockField(tags,"501",91)].value="2";tags[blockField(tags,"501",92)].value="2";
    const auto at=blockField(tags,"501",171);
    tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(at),{{141,"8.5"},{142,"9.75"},{11,"0.5"},{21,"0.25"},{31,"0.125"},
        {93,"3"},{94,"5"},{95,"9"},{96,"17"}});
    appendBlockFields(tags,"501",{{7,"Standard"},{140,"2.25"},{170,"3"},{177,"7"},{178,"2"},{145,"0.375"},
        {63,"12"},{64,"13"},{69,"14"},{65,"15"},{66,"16"},{68,"17"},{279,"25"},{275,"30"},{276,"35"},{278,"40"},
        {289,"0"},{285,"1"},{286,"0"},{288,"1"},{283,"1"},{173,"1"},{175,"1"},{176,"1"},{179,"1"},{331,"601"},{300,"cell-attribute-é"},
        {171,"2"},{340,"602"},{144,"1.75"},{179,"1"},{331,"603"},{300,"block-attribute-é"},
        {171,"1"},{301,"CELL_VALUE"},{90,"4"},{1,"value-é"},{304,"END_VALUE"},{344,"604"},
        {171,"1"},{1,"last-é"},{70,"1"},{40,"0.125"},{41,"0.25"},{280,"1"},{281,"1"}});
    return tags;
}
using TableBodyMutation=std::pair<const char*,std::function<void(DRW_Table&)>>;
std::vector<TableBodyMutation> tableBodyMutations() {
    std::vector<TableBodyMutation> out;
#define TB_MUTATE(label,expression) out.emplace_back(label,[](DRW_Table& t){expression;})
#define TB_NUMBER(field) TB_MUTATE(#field,++t.field)
#define TB_BOOL(field) TB_MUTATE(#field,t.field=!t.field)
    TB_NUMBER(m_valueFlag);TB_NUMBER(m_horizontalDirection.x);TB_NUMBER(m_horizontalDirection.y);TB_NUMBER(m_horizontalDirection.z);
    TB_BOOL(m_hasSemanticContent);TB_BOOL(m_semanticContentComplete);TB_NUMBER(m_dataVersion);TB_NUMBER(m_ownerBlockRecordHandle);
    TB_NUMBER(m_tableOverrideFlags);TB_NUMBER(m_borderColorOverrideFlags);TB_NUMBER(m_borderLineweightOverrideFlags);TB_NUMBER(m_borderVisibilityOverrideFlags);
    TB_NUMBER(m_flowDirection);TB_NUMBER(m_horizontalCellMargin);TB_NUMBER(m_verticalCellMargin);TB_BOOL(m_titleSuppressed);TB_BOOL(m_headerSuppressed);TB_NUMBER(m_tableStyleHandle);
    TB_MUTATE("content.name",t.m_content.m_name+="x");TB_MUTATE("content.description",t.m_content.m_description+="x");
    TB_NUMBER(m_content.m_tableStyleHandle);TB_NUMBER(m_content.m_overrideStyleType);TB_NUMBER(m_content.m_fieldHandles[0]);
    TB_MUTATE("column.name",t.m_content.m_columns[0].m_name+="x");TB_NUMBER(m_content.m_columns[0].m_styleId);
    TB_NUMBER(m_content.m_columns[0].m_overrideStyleType);TB_NUMBER(m_content.m_columns[0].m_width);
    TB_NUMBER(m_content.m_rows[0].m_styleId);TB_NUMBER(m_content.m_rows[0].m_overrideStyleType);TB_NUMBER(m_content.m_rows[0].m_height);
    TB_NUMBER(m_content.m_mergedRanges[0].m_topRow);TB_NUMBER(m_content.m_mergedRanges[0].m_leftColumn);
    TB_NUMBER(m_content.m_mergedRanges[0].m_bottomRow);TB_NUMBER(m_content.m_mergedRanges[0].m_rightColumn);
#define TB_CELL_NUMBER(field) TB_NUMBER(m_content.m_rows[0].m_cells[0].field)
#define TB_CELL_BOOL(field) TB_BOOL(m_content.m_rows[0].m_cells[0].field)
    TB_CELL_NUMBER(m_flags);TB_CELL_NUMBER(m_type);TB_CELL_NUMBER(m_edgeFlags);TB_CELL_BOOL(m_isMerged);TB_CELL_BOOL(m_autoFit);
    TB_CELL_NUMBER(m_mergedWidth);TB_CELL_NUMBER(m_mergedHeight);TB_CELL_NUMBER(m_rotation);
    TB_MUTATE("cell.tooltip",t.m_content.m_rows[0].m_cells[0].m_toolTip+="x");TB_MUTATE("cell.styleName",t.m_content.m_rows[0].m_cells[0].m_textStyleName+="x");
    TB_CELL_NUMBER(m_styleId);TB_CELL_NUMBER(m_overrideStyleType);TB_CELL_NUMBER(m_virtualEdgeFlags);TB_CELL_NUMBER(m_overrideFlags);
    TB_CELL_NUMBER(m_valueHandle);TB_CELL_NUMBER(m_textStyleHandle);TB_CELL_NUMBER(m_textStyleOverrideHandle);TB_CELL_NUMBER(m_blockHandle);
    TB_CELL_BOOL(m_hasLinkedData);TB_CELL_NUMBER(m_dataLinkHandle);TB_CELL_NUMBER(m_linkedDataRowCount);TB_CELL_NUMBER(m_linkedDataColumnCount);TB_CELL_NUMBER(m_linkedDataUnknown);
    TB_CELL_NUMBER(m_blockScale);TB_CELL_NUMBER(m_width);TB_CELL_NUMBER(m_height);TB_CELL_NUMBER(m_geometryFlags);TB_CELL_NUMBER(m_geometryHandle);TB_CELL_NUMBER(m_geometryRecordFlags);
    TB_CELL_NUMBER(m_geometryTopLeft.x);TB_CELL_NUMBER(m_geometryTopLeft.y);TB_CELL_NUMBER(m_geometryTopLeft.z);
    TB_CELL_NUMBER(m_geometryCenter.x);TB_CELL_NUMBER(m_geometryCenter.y);TB_CELL_NUMBER(m_geometryCenter.z);
    TB_CELL_NUMBER(m_contentWidth);TB_CELL_NUMBER(m_contentHeight);TB_CELL_NUMBER(m_geometryWidth);TB_CELL_NUMBER(m_geometryHeight);
    TB_CELL_NUMBER(m_textColorRgb);TB_CELL_NUMBER(m_textColorMethod);TB_CELL_NUMBER(m_fillColorRgb);TB_CELL_NUMBER(m_fillColorMethod);
    TB_CELL_BOOL(m_backgroundEnabled);TB_CELL_NUMBER(m_textAlignment);TB_CELL_NUMBER(m_textHeight);
    for(std::size_t i=0;i<6u;++i) {
        out.emplace_back("border.weight",[i](DRW_Table& t){++t.m_content.m_rows[0].m_cells[0].m_borders[i].m_lineWeight;});
        out.emplace_back("border.visible",[i](DRW_Table& t){auto& b=t.m_content.m_rows[0].m_cells[0].m_borders[i];b.m_visible=!b.m_visible;});
        out.emplace_back("border.rgb",[i](DRW_Table& t){++t.m_content.m_rows[0].m_cells[0].m_borders[i].m_colorRgb;});
        out.emplace_back("border.method",[i](DRW_Table& t){++t.m_content.m_rows[0].m_cells[0].m_borders[i].m_colorMethod;});
    }
    TB_CELL_NUMBER(m_attributes[0].m_attdefHandle);TB_CELL_NUMBER(m_attributes[0].m_index);TB_CELL_NUMBER(m_attributes[0].m_contentIndex);
    TB_MUTATE("cell.attribute.text",t.m_content.m_rows[0].m_cells[0].m_attributes[0].m_text+="x");
    TB_CELL_NUMBER(m_contents[0].m_type);TB_CELL_NUMBER(m_contents[0].m_handle);TB_MUTATE("cell.content.text",t.m_content.m_rows[0].m_cells[0].m_contents[0].m_text+="x");
    TB_CELL_NUMBER(m_contents[0].m_value.m_formatFlags);TB_CELL_NUMBER(m_contents[0].m_value.m_dataType);TB_CELL_NUMBER(m_contents[0].m_value.m_dataSize);
    TB_CELL_NUMBER(m_contents[0].m_value.m_unitType);TB_CELL_NUMBER(m_contents[0].m_value.m_handle);
    TB_MUTATE("cad.format",t.m_content.m_rows[0].m_cells[0].m_contents[0].m_value.m_formatString+="x");
    TB_MUTATE("cad.valueString",t.m_content.m_rows[0].m_cells[0].m_contents[0].m_value.m_valueString+="x");
    TB_MUTATE("cad.rawData",t.m_content.m_rows[0].m_cells[0].m_contents[0].m_value.m_rawData.push_back(42u));
    TB_MUTATE("cad.variant",t.m_content.m_rows[0].m_cells[0].m_contents[0].m_value.m_value=DRW_Variant(1,"different"));
    TB_MUTATE("rows.count",t.m_content.m_rows.pop_back());TB_MUTATE("columns.count",t.m_content.m_columns.pop_back());
    TB_MUTATE("cells.count",t.m_content.m_rows[0].m_cells.pop_back());TB_MUTATE("attributes.count",t.m_content.m_rows[0].m_cells[0].m_attributes.clear());
    TB_MUTATE("contents.count",t.m_content.m_rows[0].m_cells[0].m_contents.clear());TB_MUTATE("merged.count",t.m_content.m_mergedRanges.push_back({}));
    TB_MUTATE("subrecords.count",t.m_content.m_subrecordRanges.push_back({}));
    TB_MUTATE("INSERT.name",t.name+="x");TB_NUMBER(xscale);TB_NUMBER(yscale);TB_NUMBER(zscale);TB_NUMBER(angle);TB_NUMBER(colcount);TB_NUMBER(rowcount);TB_NUMBER(colspace);TB_NUMBER(rowspace);
    TB_NUMBER(blockRecH.code);TB_NUMBER(seqendH.ref64);TB_MUTATE("INSERT.attributeHandles",t.attribHandles.push_back({}));
    TB_MUTATE("linked.attribute.text",t.attlist[0]->text+="x");TB_MUTATE("linked.attribute.tag",t.attlist[0]->tag+="x");
    TB_NUMBER(attlist[0]->basePoint.z);TB_NUMBER(seqEndEntity->color);TB_NUMBER(basePoint.x);TB_NUMBER(extPoint.z);TB_NUMBER(thickness);TB_NUMBER(xAxisAngle);
    TB_MUTATE("common.layer",t.layer+="x");TB_NUMBER(color);TB_NUMBER(ltypeScale);TB_MUTATE("common.appData",t.appData.push_back({DRW_Variant(1,"extra")}));
    TB_MUTATE("signed-zero",t.m_content.m_rows[0].m_cells[0].m_width=-0.0);
#undef TB_CELL_BOOL
#undef TB_CELL_NUMBER
#undef TB_BOOL
#undef TB_NUMBER
#undef TB_MUTATE
    return out;
}
bool tableBodyComparatorControls(const DRW_Table& original,unsigned& count) {
    using A=DxIfaceTableSourceTestAccess;DRW_Table a(original);a.m_content.m_subrecordRanges.push_back({"range",7u,9u,DRW::AC1032,1u,true});
    for(unsigned i=0u;i<6u;++i) {DRW_Table b(a);auto& r=b.m_content.m_subrecordRanges.back();
        if(!A::sameTableBody(a,b))return false;
        switch(i){case 0:r.m_name+="x";break;case 1:++r.m_startBit;break;case 2:++r.m_bitSize;break;case 3:r.m_version=DRW::AC1027;break;case 4:++r.m_count;break;default:r.m_parseComplete=false;break;}
        if(A::sameTableBody(a,b))return false;++count;
    }
    const auto value=[](DRW_Table& t)->DRW_Variant&{return t.m_content.m_rows[0].m_cells[0].m_contents[0].m_value.m_value;};
    for(unsigned i=0u;i<7u;++i){DRW_Table left(a);auto& v=value(left);
        switch(i){case 0:v.addString(1,"text");break;case 1:v.addInt(70,42);break;case 2:v.addInt64(160,INT64_C(0x123456789abcdef));break;
        case 3:v.addDouble(40,-0.0);break;case 4:v.addCoord(10,{1.,2.,3.});break;case 5:v.addBinary(310,{0u,42u,255u});break;default:v=DRW_Variant();break;}
        DRW_Table right(left);if(!A::sameTableBody(left,right))return false;value(right).addInt(71,99);
        if(A::sameTableBody(left,right))return false;++count;
    }
    {DRW_Table left(a);value(left).addString(1003,"Layer");value(left).setDwgRawLayerReference(UINT64_C(0x123456789),DRW::AC1032);
        DRW_Table right(left);if(!A::sameTableBody(left,right))return false;value(right).setDwgRawLayerReference(UINT64_C(0x123456789),DRW::AC1027);
        if(A::sameTableBody(left,right))return false;++count;
        right=left;value(right).setLayerRefName("Layer");if(A::sameTableBody(left,right))return false;++count;}
    for(unsigned i=0u;i<8u;++i){DRW_Table left(a);DRW_DwgCmColor color;value(left).setDwgCmColor(color);DRW_Table right(left);
        if(!A::sameTableBody(left,right))return false;
        switch(i){case 0:++color.method;break;case 1:++color.flags;break;case 2:++color.index;break;case 3:++color.rgb24;break;
        case 4:color.hasRgbColor=!color.hasRgbColor;break;case 5:color.colorName="changed";break;case 6:color.bookName="changed";break;default:color.sourceVersion=DRW::AC1032;break;}
        value(right).setDwgCmColor(color);if(A::sameTableBody(left,right))return false;++count;
    }
    // Exact equality must not certify equal unknown concrete subclasses by
    // silently comparing only their known base fields. These are comparator
    // isolation checks, not parser acceptance or native payload evidence.
    struct RichTable:DRW_Table{};RichTable unknown;if(A::sameTableBody(unknown,unknown))return false;++count;
    struct RichAttribute:DRW_Attrib{};struct RichMText:DRW_MText{};struct RichFooter:DRW_SeqEnd{};
    for(unsigned i=0u;i<4u;++i){DRW_Table b(a);
        if(i==0u)b.attlist[0]=std::make_shared<RichAttribute>();else if(i==1u)b.attlist[0]->mtext=std::make_unique<RichMText>();
        else if(i==2u)b.seqEndEntity=std::make_shared<RichFooter>();else b.attlist[0].reset();
        if(A::sameTableBody(b,b))return false;++count;
    }
    return true;
}
bool tableBodyTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;using S=dx_iface::TableBodyState;
    struct Stage{const char* name="profiles";bool done=false;~Stage(){if(!done)std::cerr<<"TABLE body stage="<<name<<'\n';}}stage;
    const auto path=dir/"table-bodies.dxf";unsigned profiles=0u,rich=0u,mutated=0u,ownerForms=0u,quotas=0u,allocations=0u,contexts=0u,failures=0u,stubs=0u,comparators=0u,multiple=0u;
    const auto matched=[](const dx_data& d){const auto r=dx_iface::tableBodyReport(d);return r.state==S::ModeledMatched&&r.selected==1u&&r.matched==1u
        &&r.modeledMatched&&!r.profilesMatched&&!r.ownerTargetsResolved&&!r.outputEligible;};
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true})for(unsigned form=0u;form<3u;++form) {
        if(!writeBlockBinding(path,tableBlockSourceDocument(version,blocks,form),encoding))return false;dx_iface reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false))return false;const auto r=dx_iface::tableBodyReport(data);
        if(r.state!=S::ModeledMatched||r.selected!=1u||r.matched!=1u||!r.modeledMatched||r.profilesMatched||r.ownerTargetsResolved||r.outputEligible) {
            std::cerr<<"TABLE body "<<version<<'/'<<encoding<<" blocks="<<blocks<<" form="<<form<<" state="<<static_cast<int>(r.state);
            if(!A::tableBodies(data).empty())std::cerr<<" reason="<<static_cast<int>(A::tableBodies(data).begin()->second->state)
                <<" failedRecord="<<A::tableBodies(data).begin()->second->failedRecord;
            std::cerr<<'\n';return false;
        }
        ++profiles;
    }
    {dx_iface reader;dx_data data;if(!write(path,tableBodyRichDocument("AC1032",false),false)||!reader.fileImport(path.string(),&data,false)
        ||!tableBodyComparatorControls(*A::tableInputs(data).at(0x501u)->model,comparators))return false;}
    stage.name="rich grid/field/attributes and both footer-owner forms";
    for(const std::string version:{"AC1021","AC1024","AC1027","AC1032"})for(unsigned encoding=0u;encoding<3u;++encoding)for(bool blocks:{false,true}) {
        if(!writeBlockBinding(path,tableBodyRichDocument(version,blocks),encoding))return false;dx_iface reader;dx_data data;
        if(!reader.fileImport(path.string(),&data,false)||!matched(data)||dx_iface::tableBodyReport(data).semanticComplete!=1u)return false;
        ++rich;
        for(unsigned form=1u;form<3u;++form) {dx_data next;if(!writeBlockBinding(path,tableBlockSourceDocument(version,blocks,form,true),encoding)
                ||!reader.fileImport(path.string(),&next,false)||!matched(next))return false;
            ++ownerForms;}
        auto raw=tableBlockSourceDocument(version,blocks,0u);const auto first=blockField(raw,"501",280),last=blockField(raw,"501",1);
        raw.erase(raw.begin()+static_cast<std::ptrdiff_t>(first),raw.begin()+static_cast<std::ptrdiff_t>(last+1u));
        dx_data stub;if(!writeBlockBinding(path,raw,encoding)||!reader.fileImport(path.string(),&stub,false)||!matched(stub)
            ||dx_iface::tableBodyReport(stub).semanticComplete)return false;
        ++stubs;
    }
    const auto mutations=tableBodyMutations();
    for(bool binary:{false,true})for(bool blocks:{false,true}) {
        const auto good=tableBodyRichDocument("AC1032",blocks);if(!write(path,good,binary))return false;
        const auto retry=[&](dx_iface& reader){dx_data next;return write(path,good,binary)&&reader.fileImport(path.string(),&next,false)&&matched(next);};
        stage.name="actual original/typed modeled-field faults";
        for(bool before:{false,true})for(const auto& mutation:mutations) {
            TableBodyProbe reader;reader.mutation=mutation.second;reader.beforeSource=before;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::tableBodyReport(data).state!=S::Unqualified||matched(data)) {
                std::cerr<<"field="<<mutation.first<<" before="<<before<<" blocks="<<blocks<<" binary="<<binary<<'\n';return false;
            }
            const auto& b=*A::tableBodies(data).at(0x501u);
            if(b.state!=decltype(b.state)::ModelMismatch||b.typedMatched||b.sourceMatched==before)return false;
            reader.mutation={};if(!retry(reader))return false;++mutated;
        }
        stage.name="multiple TABLEs cannot hide a mismatched selected sibling";
        {auto tags=good;const auto first=slot(tags,5,"501")-1u;auto last=slot(tags,5,"513")+1u;
            while(last<tags.size()&&tags[last].code!=0)++last;
            Tags second(tags.begin()+static_cast<std::ptrdiff_t>(first),tags.begin()+static_cast<std::ptrdiff_t>(last));
            for(auto& field:second) {
                if(field.code==5){if(field.value=="501")field.value="521";else if(field.value=="511")field.value="531";else if(field.value=="512")field.value="532";else if(field.value=="513")field.value="533";}
                else if(field.code==330&&field.value=="501")field.value="521";
            }
            tags.insert(tags.begin()+static_cast<std::ptrdiff_t>(last),second.begin(),second.end());
            for(bool bad:{false,true}){TableBodyProbe reader;reader.mutateHandle=0x521u;if(bad)reader.mutation=[](DRW_Table& t){++t.m_horizontalDirection.x;};dx_data data;
                if(!write(path,tags,binary)||!reader.fileImport(path.string(),&data,false))return false;const auto r=dx_iface::tableBodyReport(data);
                if(r.selected!=2u||r.matched!=(bad?1u:2u)||r.modeledMatched==bad||r.state!=(bad?S::Unqualified:S::ModeledMatched))return false;
                reader.mutation={};if(!retry(reader))return false;++multiple;
            }
        }
        stage.name="pending original cursors and divergent private state";
        for(bool child:{false,true}) {auto tags=good;appendBlockFields(tags,child?"511":"501",{{1001,"APP"},{1010,"7"}});
            dx_iface reader;dx_data data;if(!write(path,tags,binary)||!reader.fileImport(path.string(),&data,false)
                ||dx_iface::tableBodyReport(data).state!=S::Unqualified||!retry(reader))return false;
            ++failures;}
        for(int field=0;field<14;++field){TableInputProbe reader;reader.stateFault=field;dx_data data;
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::tableBodyReport(data).state!=S::Unqualified)return false;
            reader.clear();if(!retry(reader))return false;++failures;}
        stage.name="exact/one-under records/bytes/work";
        dx_iface baseline;dx_data base;if(!baseline.fileImport(path.string(),&base,false)||!matched(base))return false;const auto usage=dx_iface::tableBodyReport(base);
        const std::array<std::size_t,3> caps{{usage.records,usage.bytes,usage.work}};
        for(unsigned which=0u;which<3u;++which)for(bool under:{false,true}) {dx_iface reader;dx_data data;A::tableBodyLimit(data,which,caps[which]-(under?1u:0u));
            if(!reader.fileImport(path.string(),&data,false)||dx_iface::tableBodyReport(data).state!=(under?S::CaptureFailed:S::ModeledMatched)||!retry(reader))return false;++quotas;}
        stage.name="reached replay-finalizer allocation faults and corrected rebuild";
        for(long fault=0;fault<32;++fault) {dx_iface reader;dx_data data;if(!reader.fileImport(path.string(),&data,false)||!matched(data))return false;
            const auto count=symbolFailedAllocations;{SymbolAllocationFailure fail(fault);A::rebuildTableBodies(reader,data);}
            if(symbolFailedAllocations==count||matched(data))return false;
            A::rebuildTableBodies(reader,data);if(!matched(data)||!retry(reader))return false;++allocations;
        }
        stage.name="immutable owner/context/container guards and live-edit independence";
        for(bool alias:{false,true}) {dx_iface reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;A::substituteTableBody(data,alias);
            if(dx_iface::tableBodyReport(data).state!=S::ContextMismatch)return false;++contexts;}
        {dx_iface reader;dx_data data;if(!reader.fileImport(path.string(),&data,false))return false;
            {SymbolAllocationFailure noAllocation(0);if(!matched(data))return false;}++contexts;
            data.tables.front().m_content.m_rows.clear();if(!matched(data))return false;++contexts;
            dx_data copy;A::copyBindingContext(copy,data);if(dx_iface::tableBodyReport(copy).state!=S::ContextMismatch)return false;++contexts;}
        {TableInputProbe reader;reader.facets=false;dx_data data;if(!reader.fileImport(path.string(),&data,false)
                ||dx_iface::tableBodyReport(data).state!=S::Unqualified)return false;
            ++contexts;}
        stage.name="callback exceptions and failed tails stay failed; same-adapter corrected imports";
        for(unsigned which=0u;which<3u;++which) {auto tags=good;if(which==2u)tags.pop_back();if(!write(path,tags,binary))return false;TableInputProbe reader;
            reader.throwSource=which==0u;reader.throwTyped=which==1u;dx_data data;bool read=false,threw=false;
            try{read=reader.fileImport(path.string(),&data,false);}catch(const std::runtime_error&){threw=true;}
            if(read||threw!=(which!=2u)||dx_iface::tableBodyReport(data).state!=S::ImportFailed)return false;
            reader.clear();if(!retry(reader))return false;++failures;
        }
    }
    {dx_iface reader;dx_data data;if(!write(path,entitiesRoleDocument("AC1032"),false)||!reader.fileImport(path.string(),&data,false)
        ||dx_iface::tableBodyReport(data).state!=S::NoInputs||dx_iface::tableBodyReport(data).modeledMatched)return false;}
    stage.done=true;std::cout<<"TABLE modeled replay profiles="<<profiles<<"; rich grids="<<rich<<"; owner forms="<<ownerForms<<"; raw stubs="<<stubs
        <<"; mutation kinds="<<mutations.size()<<"; original/typed faults="<<mutated<<"; quotas="<<quotas<<"; reached allocation faults="<<allocations
        <<"; owner/context controls="<<contexts<<"; pending/state/exception/tail retries="<<failures<<"; isolated comparator controls="<<comparators<<"; multi-TABLE controls="<<multiple<<'\n';
    return profiles==72u&&rich==24u&&ownerForms==48u&&stubs==24u&&mutated==mutations.size()*8u&&quotas==24u&&allocations==128u&&contexts==24u&&failures==76u&&comparators==28u&&multiple==8u;
}
