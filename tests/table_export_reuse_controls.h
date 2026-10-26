// SPDX-License-Identifier: GPL-2.0-or-later
// Original TABLE and 2D projection identities survive actual output attempts.
// All drawing recipes are authored here; no external fixture or CAD semantics.
DRW_Table tableExportReuseModel(unsigned shape=0u,bool block=false) {
    DRW_Table table;table.handle=0x500u;table.parentHandle=block?0xB01u:0x1Fu;table.name="*T500";
    table.m_hasSemanticContent=table.m_semanticContentComplete=true;
    table.m_content.m_columns.push_back({"",2.0});table.m_content.m_rows.resize(1u);
    auto& row=table.m_content.m_rows.front();row.m_height=1.0;row.m_cells.resize(1u);row.m_cells[0].m_type=1;
    DRW_TableCellContent content;content.m_type=1;content.m_text="reused-table";row.m_cells[0].m_contents.push_back(content);
    if(shape==1u) {
        auto attribute=std::make_shared<DRW_Attrib>();attribute->handle=0x501u;attribute->parentHandle=table.handle;
        attribute->tag="TABLE_TAG";attribute->text="attribute-value";attribute->height=1.0;table.attlist.push_back(attribute);
    }
    if(shape) {
        table.seqEndEntity=std::make_shared<DRW_SeqEnd>();table.seqEndEntity->handle=0x502u;
        table.seqEndEntity->parentHandle=shape==2u?table.parentHandle:table.handle;
    }
    return table;
}
void tableExportReuseDrawing(dx_iface& writer,dx_data& data,unsigned shape,bool block) {
    writer.cData=&data;writer.currentBlock=data.mBlock;
    if(block) {
        DRW_Block_Record record;record.handle=0xB01u;record.name="TABLE_OWNER";data.blockRecords.push_back(record);
        DRW_Block definition;definition.handle=0xB00u;definition.parentHandle=record.handle;definition.name=record.name;writer.addBlock(definition);
    }
    writer.addTable(tableExportReuseModel(shape,block));
    if(block)writer.endBlock();
}
using TableExportIdentities=std::vector<std::tuple<const void*,std::uint32_t,std::uint32_t>>;
TableExportIdentities tableExportIdentities(const dx_data& data) {
    TableExportIdentities result;
    const auto append=[&](const DRW_Entity& entity) {
        result.emplace_back(&entity,entity.handle,entity.parentHandle);
        if(const auto* insert=dynamic_cast<const DRW_Insert*>(&entity)) {
            for(const auto& attribute:insert->attlist)if(attribute)result.emplace_back(attribute.get(),attribute->handle,attribute->parentHandle);
            if(insert->seqEndEntity)result.emplace_back(insert->seqEndEntity.get(),insert->seqEndEntity->handle,insert->seqEndEntity->parentHandle);
        }
    };
    for(const auto& table:data.tables)append(table);
    for(const auto* entity:data.mBlock->ent)if(entity)append(*entity);
    for(const auto* block:data.blocks)if(block)for(const auto* entity:block->ent)if(entity)append(*entity);
    return result;
}
void tableExportTrace(const dx_data& data,const char* phase) {
    std::cerr<<"TABLE reuse "<<phase<<std::hex;
    for(const auto& id:tableExportIdentities(data))std::cerr<<" ["<<std::get<1>(id)<<'/'<<std::get<2>(id)<<']';
    std::cerr<<std::dec<<'\n';
}
bool tableExportReadback(const std::filesystem::path& path,bool binary,unsigned shape,bool block) {
    dx_iface reader;dx_data next;if(!reader.fileImport(path.string(),&next,false)||next.tables.size()!=1u)return false;
    const auto& table=next.tables.front();
    if(table.m_content.m_rows.size()!=1u||table.m_content.m_rows[0].m_cells.size()!=1u
        ||table.m_content.m_rows[0].m_cells[0].m_contents.size()!=1u
        ||table.m_content.m_rows[0].m_cells[0].m_contents[0].m_text!="reused-table"
        ||table.attlist.size()!=(shape==1u?1u:0u)||static_cast<bool>(table.seqEndEntity)!=(shape!=0u))return false;
    if(shape==1u&&(table.attlist[0]->parentHandle!=table.handle||table.attlist[0]->tag!="TABLE_TAG"
        ||table.attlist[0]->text!="attribute-value"))return false;
    if(shape&&table.seqEndEntity->parentHandle!=table.handle)return false;
    // Separate wire walker: verify one TABLE, no synthetic *T INSERT, its
    // actual enclosing BLOCK_RECORD and emitted linked-owner references.
    const auto tags=attdefWireTags(path,binary);std::map<std::uint64_t,std::string> owners;
    std::string record,name;std::uint64_t self=0u,owner=0u,tableOwner=0u;unsigned tables=0u,projections=0u,attributes=0u,footers=0u;
    bool valid=true;
    const auto finish=[&]() {
        if(record=="BLOCK_RECORD")owners.emplace(self,name);
        else if(record=="ACAD_TABLE"){++tables;if(self!=table.handle||owner!=table.parentHandle)valid=false;tableOwner=owner;}
        else if(record=="INSERT"&&name=="*T500")++projections;
        else if(record=="ATTRIB"||record=="SEQEND") {
            if(owner!=table.handle)valid=false;
            if(record=="ATTRIB")++attributes;else ++footers;
        }
    };
    const auto handle=[&](std::string_view text) {
        std::uint64_t value=0u;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),value,16);
        if(text.empty()||parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size())valid=false;
        return value;
    };
    for(const auto& tag:tags) {
        if(tag.code==0){finish();record=tag.text;name.clear();self=owner=0u;}
        else if(record=="BLOCK_RECORD"||record=="ACAD_TABLE"||record=="ATTRIB"||record=="SEQEND"||record=="INSERT") {
            if(tag.code==5)self=handle(tag.text);
            else if(tag.code==330)owner=handle(tag.text);
            else if(tag.code==2)name=tag.text;
        }
    }
    finish();const auto target=owners.find(tableOwner);
    return valid&&tables==1u&&!projections&&attributes==(shape==1u?1u:0u)&&footers==(shape?1u:0u)
        &&target!=owners.end()&&(block?target->second=="TABLE_OWNER":target->second=="*Model_Space");
}
bool tableExportSentinel(const std::filesystem::path& path,bool create=false) {
    const std::string bytes="PRESERVE_TABLE_REUSE_DESTINATION";
    if(create){std::ofstream out(path,std::ios::binary);out<<bytes;return static_cast<bool>(out);}
    std::ifstream in(path,std::ios::binary);return std::string(std::istreambuf_iterator<char>(in),{})==bytes;
}
struct TableExportFaults final:dx_iface {
    bool late=false,sawTable=false;long allocation=-1;bool reached=false;
    void writeEntities() override {
        const auto before=symbolFailedAllocations;
        {SymbolAllocationFailure fail(allocation);dx_iface::writeEntities();}
        reached=symbolFailedAllocations!=before;
    }
    void writeObjects() override {
        sawTable=DxIfaceTableSourceTestAccess::emittedTables(*this)==1u;
        if(late)throw std::runtime_error("late TABLE export failure");
        dx_iface::writeObjects();
    }
};
bool tableExportReuseTests(const std::filesystem::path& dir) {
    using A=DxIfaceTableSourceTestAccess;
    struct Stage{const char* name="positive reuse";bool done=false;~Stage(){if(!done)std::cerr<<"TABLE reuse stage="<<name<<'\n';}}stage;
    const std::array<DRW::Version,5> versions{{DRW::AC1018,DRW::AC1021,DRW::AC1024,DRW::AC1027,DRW::AC1032}};
    const auto seed=dir/"reuse-seed.dxf",output=dir/"reuse-output.dxf",other=dir/"reuse-other.dxf";
    unsigned profiles=0u,successes=0u,refusals=0u,late=0u,allocationFailures=0u,unreached=0u,mismatches=0u;
    for(std::size_t revision=0u;revision<versions.size();++revision)for(bool binary:{false,true})for(bool block:{false,true})
        for(unsigned shape=0u;shape<3u;++shape)for(bool imported:{false,true}) {
            dx_data authored,parsed;dx_iface writer,reader;tableExportReuseDrawing(writer,authored,shape,block);
            if(imported&&(!writer.fileExport(seed.string(),versions[revision],binary,&authored,false)
                ||!reader.fileImport(seed.string(),&parsed,false)))return false;
            auto& data=imported?parsed:authored;const auto identities=tableExportIdentities(data);const DRW_Table model(data.tables.front());
            const auto sourceVersion=data.sourceVersion;const auto sourceFormat=data.sourceFormat;const auto report=dx_iface::tableBodyReport(data);
            const auto unchanged=[&]() {const auto after=dx_iface::tableBodyReport(data);return identities==tableExportIdentities(data)
                &&A::sameTableBody(model,data.tables.front())&&data.sourceVersion==sourceVersion&&data.sourceFormat==sourceFormat
                &&after.state==report.state&&after.modeledMatched==report.modeledMatched&&after.profilesMatched==report.profilesMatched
                &&after.ownerTargetsResolved==report.ownerTargetsResolved&&after.outputEligible==report.outputEligible;};
            for(unsigned pass=0u;pass<3u;++pass) {
                const bool encoding=pass==1u?!binary:binary;const auto version=pass==1u?versions[(revision+1u)%versions.size()]:versions[revision];
                const auto& destination=pass?other:output;
                if(pass==2u) {
                    data.tables.front().m_content.m_rows[0].m_cells[0].m_contents[0].m_value.m_formatFlags=2;
                    if(!tableExportSentinel(destination,true)||writer.fileExport(destination.string(),version,encoding,&data,false))return false;
                    data.tables.front().m_content.m_rows[0].m_cells[0].m_contents[0].m_value.m_formatFlags=0;
                    if(!tableExportSentinel(destination)||!unchanged())return false;++refusals;
                }
                if(!writer.fileExport(destination.string(),version,encoding,&data,false)||!unchanged()
                    ||!tableExportReadback(destination,encoding,shape,block)) {
                    std::cerr<<"TABLE reuse revision="<<revision<<" binary="<<binary<<" block="<<block<<" shape="<<shape<<" imported="<<imported<<" pass="<<pass<<'\n';
                    tableExportTrace(data,"failed state");return false;
                }
                ++successes;
            }
            ++profiles;
        }
    stage.name="late callback exception and corrected same-model export";
    for(bool binary:{false,true})for(bool block:{false,true}) {
        dx_data data;TableExportFaults writer;tableExportReuseDrawing(writer,data,1u,block);const auto identities=tableExportIdentities(data);
        writer.late=true;
        if(!tableExportSentinel(output,true)||writer.fileExport(output.string(),DRW::AC1032,binary,&data,false)
            ||!writer.sawTable||identities!=tableExportIdentities(data)||!tableExportSentinel(output))return false;
        writer.late=false;writer.sawTable=false;
        if(!writer.fileExport(output.string(),DRW::AC1032,binary,&data,false)||!writer.sawTable
            ||identities!=tableExportIdentities(data)||!tableExportReadback(output,binary,1u,block))return false;
        ++late;
    }
    stage.name="reached finite ENTITIES callback allocation faults and corrected exports";
    for(bool binary:{false,true})for(long offset=0;offset<64;++offset) {
        dx_data data;TableExportFaults writer;tableExportReuseDrawing(writer,data,1u,false);const auto identities=tableExportIdentities(data);
        writer.allocation=offset;const auto failed=symbolFailedAllocations;
        if(!tableExportSentinel(output,true))return false;
        const bool written=writer.fileExport(output.string(),DRW::AC1032,binary,&data,false);
        const bool reached=symbolFailedAllocations!=failed;
        if(identities!=tableExportIdentities(data)||written==reached
            ||(reached?!tableExportSentinel(output):!tableExportReadback(output,binary,1u,false)))return false;
        writer.allocation=-1;writer.reached=false;
        if(!writer.fileExport(output.string(),DRW::AC1032,binary,&data,false)||identities!=tableExportIdentities(data)
            ||!tableExportReadback(output,binary,1u,false))return false;
        if(reached)++allocationFailures;else ++unreached;
    }
    stage.name="inconsistent TABLE/projection identities stay refused";
    for(bool binary:{false,true})for(unsigned fault=0u;fault<4u;++fault) {
        dx_data data;dx_iface writer;tableExportReuseDrawing(writer,data,0u,false);
        auto& table=data.tables.front();auto* projection=dynamic_cast<DRW_Insert*>(data.mBlock->ent.front());if(!projection)return false;
        const auto handle=table.handle,parent=table.parentHandle,projected=projection->handle;const auto name=projection->name;
        if(fault==0u)++table.handle;else if(fault==1u)++table.parentHandle;else if(fault==2u)++projection->handle;else projection->name="different";
        const auto invalid=tableExportIdentities(data);
        if(!tableExportSentinel(output,true)||writer.fileExport(output.string(),DRW::AC1032,binary,&data,false)
            ||invalid!=tableExportIdentities(data)||!tableExportSentinel(output))return false;
        table.handle=handle;table.parentHandle=parent;projection->handle=projected;projection->name=name;
        if(!writer.fileExport(output.string(),DRW::AC1032,binary,&data,false)||!tableExportReadback(output,binary,0u,false))return false;
        ++mismatches;
    }
    stage.done=true;
    std::cout<<"TABLE reuse profiles="<<profiles<<"; repeated/cross-encoding-version exports and readbacks="<<successes
        <<"; post-success metadata refusal/correction="<<refusals<<"; late callback failures="<<late
        <<"; reached allocation failures="<<allocationFailures<<"; unreached offsets="<<unreached<<"; projection mismatches="<<mismatches<<'\n';
    return profiles==120u&&successes==360u&&refusals==120u&&late==4u&&allocationFailures>0u&&allocationFailures+unreached==128u&&mismatches==8u;
}
