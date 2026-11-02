// Authored controls for text groups located in an attribute subclass.  No
// external drawing is included: each input is created from these literal tags.
#include "dx_iface.h"
#include "dx_data.h"
#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {
struct Tag { int code; std::string value; };

std::vector<Tag> sourceTags(const char* revision, bool embedded, int embeddedMarker = 101) {
    std::vector<Tag> tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,revision},{0,"ENDSEC"},
        {0,"SECTION"},{2,"TABLES"},{0,"TABLE"},{2,"BLOCK_RECORD"},{70,"1"},
        {0,"BLOCK_RECORD"},{5,"A0"},{100,"AcDbSymbolTableRecord"},{100,"AcDbBlockTableRecord"},{2,"B"},{70,"0"},
        {0,"ENDTAB"},{0,"ENDSEC"},{0,"SECTION"},{2,"BLOCKS"},
        {0,"BLOCK"},{5,"20"},{330,"A0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},
        {2,"B"},{70,"0"},{10,"0"},{20,"0"},{30,"0"}};
    const auto addAttribute = [&](bool definition) {
        tags.insert(tags.end(), {{0,definition?"ATTDEF":"ATTRIB"},{5,definition?"21":"11"},
            {330,definition?"A0":"10"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbText"},
            {10,"1"},{20,"2"},{30,"0"},{40,"1"},{1,"value"}});
        if (embedded) tags.insert(tags.end(), {{71,"2"},{72,"2"},{11,"12"},{21,"13"},{31,"0"}});
        tags.insert(tags.end(), {{100,definition?"AcDbAttributeDefinition":"AcDbAttribute"},
            {2,"TAG"},{70,"0"},{73,"0"},{74,"1"}});
        if (definition) tags.push_back({3,"prompt"});
        tags.insert(tags.end(), {{71,embedded ? (definition?"4":"2") : "2"},
            {72,embedded?"0":"2"},{11,"12"},{21,"13"},{31,"0"}});
        if (embedded) tags.insert(tags.end(), {{embeddedMarker,"Embedded Object"},{10,"21"},{20,"22"},{30,"0"},
            {40,"1"},{41,"4"},{46,"0"},{71,"1"},{72,"5"},{1,"multi"},{7,"Standard"},{73,"1"},{44,"1"}});
    };
    addAttribute(true);
    tags.insert(tags.end(), {{0,"ENDBLK"},{5,"22"},{330,"A0"},{100,"AcDbEntity"},{8,"0"},
        {100,"AcDbBlockEnd"},{0,"ENDSEC"},{0,"SECTION"},{2,"ENTITIES"},{0,"INSERT"},{5,"10"},
        {100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockReference"},{2,"B"},{66,"1"},{10,"0"},{20,"0"},{30,"0"}});
    addAttribute(false);
    tags.insert(tags.end(), {{0,"SEQEND"},{5,"12"},{330,"10"},{100,"AcDbEntity"},{8,"0"},
        {0,"ENDSEC"},{0,"EOF"}});
    return tags;
}

bool writeSource(const std::filesystem::path& path, const std::vector<Tag>& tags, bool binary) {
    std::ofstream stream(path,std::ios::binary);
    if (!stream) return false;
    auto word = [&](std::uint64_t value, unsigned size) {
        for (unsigned i=0;i<size;++i) stream.put(static_cast<char>(value >> (8u*i)));
    };
    if (binary) stream.write("AutoCAD Binary DXF\r\n\x1a\0",22);
    for (const auto& tag:tags) {
        if (!binary) stream << tag.code << '\n' << tag.value << '\n';
        else {
            word(static_cast<unsigned>(tag.code),2);
            if (tag.code >= 10 && tag.code <= 59) {
                const double value=std::stod(tag.value); std::uint64_t bits=0;
                std::memcpy(&bits,&value,sizeof bits); word(bits,8);
            } else if (tag.code >= 60 && tag.code <= 79) word(std::stoul(tag.value),2);
            else { stream << tag.value; stream.put('\0'); }
        }
    }
    return stream.good();
}

bool checkEntity(const DRW_Attrib& value, bool definition, bool embedded) {
    if (value.textgen != 2 || value.alignH != DRW_Text::HRight || value.alignV != DRW_Text::VBottom
        || value.secPoint.x != 12 || value.secPoint.y != 13 || value.secPoint.z != 0
        || value.tag != "TAG" || bool(value.mtext) != embedded) return false;
    if (embedded && (value.m_attributeType != (definition?4:2)
        || value.mtext->basePoint.x != 21 || value.mtext->basePoint.y != 22
        || value.mtext->basePoint.z != 0 || value.mtext->text != "multi")) return false;
    return true;
}

bool checkData(const dx_data& data, bool embedded) {
    unsigned definitions=0, children=0;
    for (const auto* block:data.blocks) if (block && block->name=="B")
        for (const auto* entity:block->ent) if (const auto* definition=dynamic_cast<const DRW_Attdef*>(entity)) {
            if (!checkEntity(*definition,true,embedded)) return false;
            ++definitions;
        }
    if (data.mBlock) for (const auto* entity:data.mBlock->ent)
        if (const auto* insert=dynamic_cast<const DRW_Insert*>(entity)) if (insert->name=="B")
            for (const auto& attribute:insert->attlist) {
                if (!attribute || !checkEntity(*attribute,false,embedded)) return false;
                ++children;
            }
    return definitions==1 && children==1;
}
} // namespace

int main() {
    const auto dir=std::filesystem::temp_directory_path()
        / ("libdxfrw-attribute-subclass-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::error_code error;
    if (!std::filesystem::create_directory(dir,error) || error) return 1;
    const std::vector<std::pair<DRW::Version,const char*>> versions{{DRW::AC1015,"AC1015"},
        {DRW::AC1018,"AC1018"},{DRW::AC1021,"AC1021"},{DRW::AC1024,"AC1024"},
        {DRW::AC1027,"AC1027"},{DRW::AC1032,"AC1032"}};
    unsigned cells=0;
    for (const auto& version:versions) for (bool inputBinary:{false,true}) {
        const bool embedded=version.first==DRW::AC1032;
        const auto input=dir/"source.dxf";
        dx_iface reader; dx_data data;
        if (!writeSource(input,sourceTags(version.second,embedded),inputBinary)
            || !reader.fileImport(input.string(),&data,false) || !checkData(data,embedded)) {
            std::cerr << "attribute source mismatch version=" << version.second << " binary=" << inputBinary << " in " << dir << '\n';
            return 1;
        }
        for (bool outputBinary:{false,true}) {
            const auto output=dir/"output.dxf"; dx_iface writer,readback; dx_data actual;
            if (!writer.fileExport(output.string(),version.first,outputBinary,&data,false)
                || !readback.fileImport(output.string(),&actual,false) || !checkData(actual,embedded)) {
                std::cerr << "attribute output mismatch version=" << version.second << " inputBinary=" << inputBinary
                          << " outputBinary=" << outputBinary << " in " << dir << '\n';
                return 1;
            }
            ++cells;
        }
    }
    // Earlier-version embedded objects are a tolerated input profile even
    // though the writer does not claim embedded output for those versions.
    // The host's Text fields precede the separate multiline header, so the
    // marker must select that header without replacing host justification.
    unsigned earlierEmbedded = 0;
    for (const auto& version : {std::pair<DRW::Version,const char*>{DRW::AC1015,"AC1015"},
                                std::pair<DRW::Version,const char*>{DRW::AC1027,"AC1027"}})
        for (bool binary : {false,true}) for (int marker : {100,101}) {
            const auto input=dir/"earlier-embedded-source.dxf";
            dx_iface reader; dx_data data;
            if (!writeSource(input,sourceTags(version.second,true,marker),binary)
                || !reader.fileImport(input.string(),&data,false) || !checkData(data,true)) {
                std::cerr << "earlier embedded attribute source mismatch version=" << version.second
                          << " binary=" << binary << " marker=" << marker << " in " << dir << '\n';
                return 1;
            }
            ++earlierEmbedded;
        }
    std::filesystem::remove_all(dir,error);
    std::cout << "Attribute subclass alignment: " << cells << " original and readback cells; "
              << earlierEmbedded << " earlier embedded input cells\n";
    return cells==24 && earlierEmbedded==8 ? 0 : 1;
}
