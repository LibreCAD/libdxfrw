// SPDX-License-Identifier: GPL-2.0-or-later
#include "libdxfrw.h"
#include "dx_iface.h"
#include "intern/dxfwriter.h"
#include "lc3_compat_check.cpp"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>

namespace {
struct Capture : LC3CompatCheck {
    void addMText(const DRW_MText& data) override {
        ++count; base = data;
        if (const auto* found = dynamic_cast<const DRW_MTextLegacyColumns*>(&data))
            columns = std::make_unique<DRW_MTextLegacyColumns>(*found);
    }
    void addAttDef(const DRW_Attdef& data) override {
        definition = std::make_unique<DRW_Attdef>(data);
    }
    int count = 0; DRW_MText base; std::unique_ptr<DRW_MTextLegacyColumns> columns;
    std::unique_ptr<DRW_Attdef> definition;
};
struct Exporter : LC3CompatCheck {
    explicit Exporter(dxfRW& codec):out(codec){}
    void writeEntities() override {
        if (entry==1) ok=out.writeText(value);
        else if (entry==2) ok=out.writeLine(value);
        else if (entry==3) ok=out.writePoint(value);
        else if (entry==4) ok=out.writeAttdef(definition);
        else ok=out.writeMText(value);
    }
    bool finalizeDxfWrite() override {return ignore || ok;}
    dxfRW& out; DRW_MText* value = nullptr; DRW_Attdef* definition = nullptr;
    bool ok = false, ignore = false; int entry = 0;
};
struct DwgExporter : dx_iface {
    explicit DwgExporter(dwgRW& codec):out(codec){}
    void writeHeader(DRW_Header& h) override {h.vars.clear();}
    void writeDwgClasses() override {}
    void collectDwgAppIds() override {}
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {
        if (entry==1) (void)out.writeText(value);
        else if (entry==2) (void)out.writeLine(value);
        else if (entry==3) (void)out.writePoint(value);
        else (void)out.writeMText(value);
    }
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeViews() override {}
    void writeUCSs() override {}
    void writeDimstyles() override {}
    void writeAppId() override {}
    void writeObjects() override {}
    dwgRW& out; DRW_MText* value = nullptr; int entry = 0;
};
std::string contents(const std::filesystem::path& path) {
    std::ifstream input(path,std::ios::binary);
    return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
}
bool make(const std::filesystem::path& path, bool binary, int mode) {
    std::ofstream file(path,std::ios::binary|std::ios::trunc);
    if (binary) file.write("AutoCAD Binary DXF\r\n\x1a\0",22);
    std::unique_ptr<dxfWriter> w = binary ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&file))
                                         : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&file));
    const char* version = mode == 14 ? "AC1032" : "AC1027";
    w->setVersion(version,true);
    w->writeString(0,"SECTION"); w->writeString(2,"HEADER"); w->writeString(9,"$ACADVER"); w->writeString(1,version); w->writeString(0,"ENDSEC");
    w->writeString(0,"SECTION"); w->writeString(2,"ENTITIES");
    w->writeString(0,"MTEXT"); w->writeString(100,"AcDbEntity"); w->writeString(8,"0"); w->writeDouble(48,3.5);
    w->writeString(100,"AcDbMText"); w->writeDouble(10,0.);w->writeDouble(20,0.);w->writeDouble(40,1.);w->writeString(1,"Column text"); w->writeDouble(50,30.);
    if (mode == 11) { w->writeString(100,"ForeignSubclass"); w->writeInt16(75,123); }
    if (mode == 15) w->writeDouble(48,7.5); // legacy misplaced common 48
    if (mode != 1 && mode != 11 && mode != 14 && mode != 15) {
        if (mode == 2) w->writeInt16(76,2); // unsupported order before type
        if (mode == 3 || mode == 7) w->writeDouble(48,12.5); // ambiguous before type
        w->writeInt16(75,2);
        if (mode == 16) {
            w->writeDouble(49,2.5);w->writeDouble(48,12.5);w->writeInt16(79,0);
            w->writeInt16(78,0);w->writeInt16(76,2);
        } else {
            w->writeInt16(76,2);w->writeInt16(78,0);w->writeInt16(79,0);
            if (mode != 3) w->writeDouble(48,12.5);
            if (mode != 10) w->writeDouble(49,2.5);
        }
        if (mode == 12) {
            w->writeDouble(50,256.);
            for (int i=0;i<256;++i) w->writeDouble(50,static_cast<double>(i+1));
        } else if (mode == 4) w->writeDouble(50,2.5); // nonintegral count
        else if (mode == 9) w->writeDouble(50,4097.); // resource bound
        else { w->writeDouble(50,2.); w->writeDouble(50,10.);
               if (mode == 13) w->writeDouble(11,1.); // interrupt height run
               if (mode != 5) w->writeDouble(50,20.); }
        if (mode == 8) w->writeDouble(50,42.); // extra ambiguous group
        if (mode == 6) w->writeString(101,"Embedded Object");
    }
    if (mode == 14) {
        w->writeString(101,"Embedded Object"); w->writeInt16(70,1);
        w->writeDouble(10,1.);w->writeDouble(20,0.);w->writeDouble(30,0.);
        w->writeDouble(11,0.);w->writeDouble(21,0.);w->writeDouble(31,0.);
        w->writeDouble(40,10.);w->writeDouble(41,20.);w->writeDouble(42,22.5);w->writeDouble(43,20.);
        w->writeDouble(44,10.);w->writeDouble(45,2.5);w->writeInt16(71,2);w->writeInt16(72,2);
        w->writeInt16(73,0);w->writeInt16(74,0);w->writeDouble(46,10.);w->writeDouble(46,20.);
    }
    w->writeString(1001,"ACAD"); w->writeString(1000,"kept-xdata");
    w->writeString(0,"ENDSEC"); w->writeString(0,"EOF");
    return !w->hasWriteError() && file.good();
}
bool check(bool binary, const std::filesystem::path& inputPath,
           const std::filesystem::path& outputPath) {
    if (!make(inputPath,binary,0)) return false;
    dxfRW input(inputPath.string().c_str()); Capture capture;
    if (!input.read(&capture,false) || capture.count!=1 || !capture.columns
        || capture.base.ltypeScale!=3.5 || capture.base.angle!=30.
        || !capture.columns->columnSource || capture.columns->columnSource->version!=DRW::AC1027
        || capture.columns->columnSource->binary!=binary
        || capture.columns->columnSource->groups.size()!=9u
        || capture.columns->columnSource->groups[4].code!=48 || capture.columns->columnSource->groups[4].value!=12.5
        || capture.columns->columnSource->groups[6].code!=50 || capture.columns->columnSource->groups[6].value!=2.
        || capture.columns->columnSource->groups[8].value!=20. || capture.base.extData.size()!=2u) return false;
    // A callback that only knows the original MTEXT API still receives the
    // correct common fields; its deliberate base copy discards column detail.
    dxfRW output(outputPath.string().c_str()); Exporter exporter(output); exporter.value=capture.columns.get();
    exporter.ignore=true;
    for (int entry : {0,1,2,3}) {
        capture.columns->eType = entry==1 ? DRW::TEXT : entry==2 ? DRW::LINE : entry==3 ? DRW::POINT : DRW::MTEXT;
        exporter.entry=entry;
        {std::ofstream existing(outputPath,std::ios::binary|std::ios::trunc); existing<<"sentinel";}
        if (output.write(&exporter,DRW::AC1027,binary) || exporter.ok || contents(outputPath)!="sentinel") return false;
    }
    capture.columns->eType=DRW::MTEXT;
    exporter.entry=0; exporter.ignore=false; exporter.value=&capture.base;
    if (!output.write(&exporter,DRW::AC1027,binary) || !exporter.ok) return false;
    dxfRW reread(outputPath.string().c_str()); Capture plain;
    if (!reread.read(&plain,false) || plain.count!=1 || plain.columns
        || plain.base.ltypeScale!=3.5 || plain.base.angle!=30.) return false;
    // A legacy API value can carry both a nonzero direction vector and an
    // angle. DXF group 11 is the authoritative orientation in that case, so
    // emit the vector without a redundant group 50. Preserve the direction
    // and derived angle in ASCII and Binary readback.
    DRW_MText directed; directed.text="direction"; directed.height=2.;
    directed.angle=30.; directed.secPoint={0.8660254037844386,0.5,0.};
    dxfRW directionOutput(outputPath.string().c_str());
    Exporter directionExporter(directionOutput); directionExporter.value=&directed;
    if (!directionOutput.write(&directionExporter,DRW::AC1027,binary)
        || !directionExporter.ok) return false;
    const std::string directionBytes=contents(outputPath);
    if (!binary) {
        const std::size_t entity=directionBytes.find("MTEXT");
        const std::size_t end=directionBytes.find("ENDSEC",entity);
        if (entity==std::string::npos || end==std::string::npos
            || directionBytes.substr(entity,end-entity).find("\n 50\n")!=std::string::npos)
            return false;
    }
    dxfRW directionInput(outputPath.string().c_str()); Capture directionRead;
    if (!directionInput.read(&directionRead,false) || directionRead.count!=1
        || std::abs(directionRead.base.secPoint.x-directed.secPoint.x)>1e-10
        || std::abs(directionRead.base.secPoint.y-directed.secPoint.y)>1e-10
        || std::abs(directionRead.base.angle-30.)>1e-10) return false;
    DRW_Attdef nested; nested.text="fallback"; nested.tag="TAG";
    nested.prompt="Prompt"; nested.height=1.;
    nested.mtext=std::make_unique<DRW_MText>();
    nested.mtext->text="embedded"; nested.mtext->height=2.;
    nested.mtext->angle=30.;
    nested.mtext->secPoint={0.8660254037844386,0.5,0.};
    dxfRW embeddedOutput(outputPath.string().c_str());
    Exporter embeddedExporter(embeddedOutput); embeddedExporter.entry=4;
    embeddedExporter.definition=&nested;
    if (!embeddedOutput.write(&embeddedExporter,DRW::AC1032,binary)
        || !embeddedExporter.ok) return false;
    const std::string embeddedBytes=contents(outputPath);
    if (!binary) {
        const std::size_t embedded=embeddedBytes.find("Embedded Object");
        const std::size_t end=embeddedBytes.find("ENDSEC",embedded);
        if (embedded==std::string::npos || end==std::string::npos
            || embeddedBytes.substr(embedded,end-embedded).find("\n 50\n")!=std::string::npos)
            return false;
    }
    dxfRW embeddedInput(outputPath.string().c_str()); Capture embeddedRead;
    if (!embeddedInput.read(&embeddedRead,false) || !embeddedRead.definition
        || !embeddedRead.definition->mtext
        || embeddedRead.definition->mtext->text!="embedded"
        || std::abs(embeddedRead.definition->mtext->angle-30.)>1e-10
        || std::abs(embeddedRead.definition->mtext->secPoint.x
                    -nested.mtext->secPoint.x)>1e-10
        || std::abs(embeddedRead.definition->mtext->secPoint.y
                    -nested.mtext->secPoint.y)>1e-10) return false;
    // The stock adapter must retain the derived carrier, not slice it, and
    // refuse unsupported output before replacing the destination.
    dx_data imported; dx_iface adapter;
    if (!adapter.fileImport(inputPath.string(),&imported,false) || imported.mBlock->ent.size()!=1u
        || dynamic_cast<DRW_MTextLegacyColumns*>(imported.mBlock->ent.front())==nullptr) return false;
    {std::ofstream existing(outputPath,std::ios::binary|std::ios::trunc); existing<<"sentinel";}
    if (adapter.fileExport(outputPath.string(),DRW::AC1027,binary,&imported,false)
        || contents(outputPath)!="sentinel") return false;
    // Even if a legacy callback ignores the returned false, a source-column
    // entity without XDATA cannot disappear from an otherwise successful DWG.
    capture.columns->extData.clear(); capture.base.extData.clear();
    dwgRW dwg(outputPath.string().c_str()); DwgExporter dwgExporter(dwg);
    dwgExporter.value=capture.columns.get();
    for (int entry : {0,1,2,3}) {
        capture.columns->eType = entry==1 ? DRW::TEXT : entry==2 ? DRW::LINE : entry==3 ? DRW::POINT : DRW::MTEXT;
        dwgExporter.entry=entry;
        {std::ofstream existing(outputPath,std::ios::binary|std::ios::trunc); existing<<"sentinel";}
        if (dwg.write(&dwgExporter,DRW::AC1018,true) || contents(outputPath)!="sentinel") return false;
    }
    capture.columns->eType=DRW::MTEXT;
    dwgExporter.entry=0; dwgExporter.value=&capture.base;
    if (!dwg.write(&dwgExporter,DRW::AC1018,true)) return false;
    if (contents(outputPath).substr(0,6)!="AC1018") return false;
    for (int mode : {1,2,3,4,5,6,7,8,9,10,11,13,14,15,16}) {
        if (!make(inputPath,binary,mode)) return false;
        dxfRW probe(inputPath.string().c_str()); Capture result;
        const bool read = probe.read(&result,false);
        if ((mode == 1 || mode == 11) && (!read || result.count!=1 || result.columns
                           || result.base.ltypeScale!=3.5 || result.base.angle!=30.)) return false;
        if (mode == 15 && (!read || result.count!=1 || result.columns
                           || result.base.ltypeScale!=7.5 || result.base.angle!=30.)) return false;
        if (mode == 14 && (!read || result.count!=1 || result.columns
            || !result.base.m_r2018DxfColumnContextPresent
            || result.base.m_r2018ColumnHeights.size()!=2u
            || result.base.m_r2018ColumnHeights[1]!=20.)) return false;
        if (mode == 14) {
            dxfRW modernOutput(outputPath.string().c_str()); Exporter modernWriter(modernOutput);
            modernWriter.value=&result.base;
            if (!modernOutput.write(&modernWriter,DRW::AC1032,binary) || !modernWriter.ok) return false;
            dxfRW modernInput(outputPath.string().c_str()); Capture modernRead;
            if (!modernInput.read(&modernRead,false) || modernRead.count!=1 || modernRead.columns
                || !modernRead.base.m_r2018DxfColumnContextPresent
                || modernRead.base.m_r2018ColumnHeights.size()!=2u
                || modernRead.base.m_r2018ColumnHeights[1]!=20.
                || modernRead.base.ltypeScale!=3.5 || modernRead.base.angle!=30.) return false;
        }
        if (mode == 16 && (!read || !result.columns || !result.columns->columnSource
            || result.columns->columnSource->groups.size()!=9u
            || result.columns->columnSource->groups[1].code!=49
            || result.columns->columnSource->groups[5].code!=76
            || result.base.ltypeScale!=3.5 || result.base.angle!=30.)) return false;
        if (mode != 1 && mode != 11 && mode != 14 && mode != 15 && mode != 16 && (read || result.count!=0)) return false;
    }
    // The ordinary small profile fits the same logical budget under which a
    // larger retained column array must fail; the same codec then retries.
    if (!make(inputPath,binary,1)) return false;
    dxfRW budgeted(inputPath.string().c_str()); budgeted.setDxfReadRetainedByteBudget(1024u);
    Capture small;
    if (!budgeted.read(&small,false) || small.count!=1) return false;
    if (!make(inputPath,binary,12)) return false;
    Capture refused;
    if (budgeted.read(&refused,false) || refused.count!=0
        || budgeted.getLastDiagnostic().cause!=DRW::OperationCause::ResourceLimit) return false;
    budgeted.setDxfReadRetainedByteBudget(1024u*1024u); Capture retried;
    if (!budgeted.read(&retried,false) || !retried.columns || !retried.columns->columnSource
        || retried.columns->columnSource->groups.size()!=263u) return false;
    const auto shared = retried.columns->columnSource;
    DRW_MTextLegacyColumns copy=*retried.columns;
    retried.columns.reset();
    if (copy.columnSource!=shared || shared->groups.back().value!=256.) return false;
    return true;
}
}
int main(){
    const auto base=std::filesystem::temp_directory_path()/("mtext-columns-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto input=base.string()+"-in.dxf",output=base.string()+"-out.dxf";
    struct Cleanup { std::string a,b; ~Cleanup(){std::error_code e;std::filesystem::remove(a,e);std::filesystem::remove(b,e);} } cleanup{input,output};
    for(bool binary:{false,true}) if(!check(binary,input,output)) {std::cerr<<"MTEXT column control failed, binary="<<binary<<'\n';return 1;}
    return 0;
}
