#include "dx_iface.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

bool fail(const std::string& message) {
    std::cerr << message << '\n';
    return false;
}

bool generate(const std::string& path, bool binary, bool dangling = false,
              bool uppercaseFixed = false) {
    std::ofstream output(path, std::ios::binary);
    if (!output)
        return fail("could not create literal block-owner control");
    std::unique_ptr<dxfWriter> writer;
    if (binary) {
        output.write("AutoCAD Binary DXF\r\n\x1a\0", 22);
        writer = std::make_unique<dxfWriterBinary>(&output);
    } else {
        writer = std::make_unique<dxfWriterAscii>(&output);
    }
    writer->setVersion("AC1027", true);
    const auto text = [&writer](int code, const std::string& value) {
        writer->writeString(code, value);
    };
    const auto section = [&text](const char* name) {
        text(0, "SECTION"); text(2, name);
    };
    section("HEADER");
    text(9, "$ACADVER"); text(1, "AC1027"); text(0, "ENDSEC");
    section("TABLES");
    text(0, "TABLE"); text(2, "APPID"); text(5, "9");
    text(330, "0"); text(100, "AcDbSymbolTable"); writer->writeInt16(70, 1);
    text(0, "APPID"); text(5, "D0"); text(330, "9");
    text(100, "AcDbSymbolTableRecord"); text(100, "AcDbRegAppTableRecord");
    text(2, "VERTEX_APP"); writer->writeInt16(70, 0); text(0, "ENDTAB");
    text(0, "TABLE"); text(2, "BLOCK_RECORD"); text(5, "1");
    text(330, "0"); text(100, "AcDbSymbolTable"); writer->writeInt16(70, 5);
    const std::pair<const char*, const char*> records[] = {
        {"17", uppercaseFixed ? "*MODEL_SPACE" : "*Model_Space"},
        {"1B", uppercaseFixed ? "*PAPER_SPACE" : "*Paper_Space"},
        {"1F", "Named"}, {"2A", "*U1"}, {"2B", "*Paper_Space0"},
    };
    for (const auto& item : records) {
        text(0, "BLOCK_RECORD"); text(5, item.first); text(330, "1");
        text(100, "AcDbSymbolTableRecord"); text(100, "AcDbBlockTableRecord");
        text(2, item.second); writer->writeInt16(70, 4);
        writer->writeInt16(280, 0); writer->writeInt16(281, 1);
        if (std::string(item.second) == "Named")
            text(310, "01020304");
    }
    text(0, "ENDTAB"); text(0, "ENDSEC");
    const auto line = [&writer, &text](const char* handle, const char* owner,
                                       double sentinel, bool paper = false) {
        text(0, "LINE"); text(5, handle); text(330, owner);
        text(100, "AcDbEntity"); text(8, "0");
        if (paper) writer->writeInt16(67, 1);
        text(100, "AcDbLine");
        writer->writeDouble(10, sentinel); writer->writeDouble(20, -2.5);
        writer->writeDouble(30, 3.75); writer->writeDouble(11, sentinel + 1);
        writer->writeDouble(21, 5.25); writer->writeDouble(31, -6.75);
    };
    const auto compoundPolyline = [&writer, &text](const char* owner) {
        text(0, "POLYLINE"); text(5, "C0"); text(330, owner);
        text(100, "AcDbEntity"); text(8, "0"); text(100, "AcDb3dPolyline");
        writer->writeInt16(66, 1); writer->writeInt16(70, 8);
        writer->writeDouble(10, 0); writer->writeDouble(20, 0); writer->writeDouble(30, 0);
        for (int vertex = 0; vertex != 2; ++vertex) {
            const std::string handle = vertex == 0 ? "C1" : "C2";
            text(0, "VERTEX"); text(5, handle); text(330, "C0");
            text(100, "AcDbEntity"); text(8, "0"); text(100, "AcDbVertex");
            text(100, "AcDb3dPolylineVertex"); writer->writeInt16(70, 32);
            writer->writeDouble(10, 51 + vertex); writer->writeDouble(20, 52 + vertex);
            writer->writeDouble(30, 53 + vertex);
            writer->writeInt32(91, 70003 + vertex);
        }
        text(0, "SEQEND"); text(5, "C3"); text(330, "C0");
        text(100, "AcDbEntity"); text(8, "0");
    };
    const auto viewport = [&writer, &text](const char* owner) {
        text(0, "VIEWPORT"); text(5, "C4"); text(330, owner);
        text(100, "AcDbEntity"); text(8, "0"); text(100, "AcDbViewport");
        writer->writeDouble(10, 4); writer->writeDouble(20, 5); writer->writeDouble(30, 0);
        writer->writeDouble(40, 205); writer->writeDouble(41, 156);
        writer->writeInt16(68, 1); writer->writeInt16(69, 42);
        writer->writeDouble(12, 128.5); writer->writeDouble(22, 97.5);
        writer->writeDouble(16, 0); writer->writeDouble(26, 0); writer->writeDouble(36, 1);
        writer->writeDouble(17, 0); writer->writeDouble(27, 0); writer->writeDouble(37, 0);
        writer->writeDouble(45, 50);
    };
    section("BLOCKS");
    int index = 0;
    for (const auto& item : records) {
        const std::string blockHandle = std::to_string(80 + index * 2);
        const std::string endHandle = std::to_string(81 + index * 2);
        text(0, "BLOCK"); text(5, blockHandle); text(330, item.first);
        text(100, "AcDbEntity"); text(8, "0");
        text(100, "AcDbBlockBegin"); text(2, item.second);
        writer->writeInt16(70, std::string(item.second) == "*U1" ? 1 : 0);
        writer->writeDouble(10, 0); writer->writeDouble(20, 0); writer->writeDouble(30, 0);
        text(3, item.second); text(1, "");
        if (std::string(item.second) == "Named") {
            line("B0", item.first, 101.25);
            compoundPolyline(item.first);
        }
        if (std::string(item.second) == "*U1") line("B1", item.first, 201.25);
        if (std::string(item.first) == "17") line("B2", item.first, 301.25);
        if (std::string(item.first) == "1B") {
            line("B3", item.first, 401.25, true);
            viewport(item.first);
        }
        text(0, "ENDBLK"); text(5, endHandle); text(330, item.first);
        text(100, "AcDbEntity"); text(8, "0"); text(100, "AcDbBlockEnd");
        ++index;
    }
    text(0, "ENDSEC"); section("ENTITIES");
    line("A0", dangling ? "DEAD" : "17", 1.25);
    line("A1", "1B", 11.25, true);
    line("A2", "2B", 21.25, true);
    text(0, "POLYLINE"); text(5, "A3"); text(330, "17");
    text(100, "AcDbEntity"); text(8, "0"); text(100, "AcDb3dPolyline");
    writer->writeInt16(66, 1); writer->writeInt16(70, 8);
    writer->writeDouble(10, 0); writer->writeDouble(20, 0); writer->writeDouble(30, 0);
    text(1001, "VERTEX_APP"); text(1000, "parent-xdata");
    for (int vertex = 0; vertex != 2; ++vertex) {
        text(0, "VERTEX"); text(5, vertex == 0 ? "A4" : "A5"); text(330, "A3");
        text(102, "{VERTEX_META"); text(1, "vertex-application-" + std::to_string(vertex));
        text(320, "DEADBEEF"); text(102, "}");
        text(100, "AcDbEntity"); text(8, "0"); text(100, "AcDbVertex");
        text(100, "AcDb3dPolylineVertex"); writer->writeInt16(70, 32);
        writer->writeDouble(10, vertex + 1); writer->writeDouble(20, vertex + 2);
        writer->writeDouble(30, vertex + 3);
        writer->writeInt32(91, vertex + 70001);
        text(1001, "VERTEX_APP"); text(1000, "vertex-xdata-" + std::to_string(vertex));
    }
    text(0, "SEQEND"); text(5, "A6"); text(330, "A3");
    text(100, "AcDbEntity"); text(8, "0");
    text(0, "INSERT"); text(5, "A7"); text(330, "17");
    text(100, "AcDbEntity"); text(8, "0"); text(100, "AcDbBlockReference");
    text(2, "Named"); writer->writeInt16(66, 1);
    writer->writeDouble(10, 501.25); writer->writeDouble(20, 2.5);
    writer->writeDouble(30, -3.75);
    text(1001, "VERTEX_APP"); text(1000, "next-record-xdata");
    text(0, "ATTRIB"); text(5, "A8"); text(330, "A7");
    text(100, "AcDbEntity"); text(8, "0"); text(100, "AcDbText");
    writer->writeDouble(10, 501.25); writer->writeDouble(20, 2.5);
    writer->writeDouble(30, -3.75); writer->writeDouble(40, 2.5);
    text(1, "attribute sentinel"); text(100, "AcDbAttribute");
    text(2, "OWNER_TAG"); writer->writeInt16(70, 0);
    text(0, "SEQEND"); text(5, "A9"); text(330, "A7");
    text(100, "AcDbEntity"); text(8, "0");
    text(0, "ENDSEC"); text(0, "EOF");
    return !writer->hasWriteError() && output.good();
}

struct Record {
    std::string type, handle, owner, name, section;
    double x {0};
    std::int32_t identifier {0};
    std::int32_t viewportId {0};
    std::vector<std::pair<int, std::string>> xdata;
    std::vector<std::string> appData;
};

bool verify(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    char signature[22] {};
    input.read(signature, sizeof signature);
    const bool binary = std::string(signature, sizeof signature)
        == std::string("AutoCAD Binary DXF\r\n\x1a\0", 22);
    input.clear(); input.seekg(binary ? 22 : 0);
    std::unique_ptr<dxfReader> reader;
    if (binary) reader = std::make_unique<dxfReaderBinary>(&input);
    else reader = std::make_unique<dxfReaderAscii>(&input);
    std::vector<Record> records;
    Record current;
    std::string section;
    int code = 0;
    bool eof = false;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!current.type.empty()) records.push_back(current);
            current = Record{}; current.type = reader->getString();
            if (current.type == "ENDSEC") {
                current.section = section;
                section.clear();
            } else if (current.type != "SECTION") {
                current.section = section;
            }
            eof = current.type == "EOF";
        } else if (code == 5) current.handle = reader->getString();
        else if (code == 330) current.owner = reader->getString();
        else if (code == 2) {
            current.name = reader->getString();
            if (current.type == "SECTION")
                section = current.name;
        }
        else if (code == 10) current.x = reader->getDouble();
        else if (code == 91) current.identifier = reader->getInt32();
        else if (code == 69) current.viewportId = reader->getInt32();
        else if (code == 1000 || code == 1001)
            current.xdata.emplace_back(code, reader->getString());
        else if (code == 102 || code == 1 || code == 320)
            current.appData.push_back(reader->getString());
    }
    if (!eof) {
        std::cerr << "Raw scan stopped at code " << code << " in " << path << '\n';
        return fail("block-owner control did not reach its literal EOF");
    }
    std::map<std::string, std::string> blocks;
    std::map<std::string, std::vector<std::string>> blockDefinitions;
    std::set<std::string> identities;
    std::size_t blockRecordRows = 0;
    std::size_t blockRows = 0;
    for (const auto& record : records) {
        if (record.type == "BLOCK_RECORD") {
            ++blockRecordRows;
            blocks[record.name] = record.handle;
        }
        if (record.type == "BLOCK" && record.section == "BLOCKS") {
            ++blockRows;
            blockDefinitions[record.name].push_back(record.owner);
        }
        if (record.type != "SECTION" && !record.handle.empty()
            && !identities.insert(record.handle).second)
            return fail("emitted identities collide");
    }
    if (blockRecordRows != 5u || blockRows != blockRecordRows
        || blocks.size() != 5u || blockDefinitions.size() != blocks.size()
        || blocks["*Model_Space"] != "1F"
        || blocks["*Paper_Space"] != "1E" || blocks["Named"] == "1F"
        || blocks["*Paper_Space0"] == blocks["*Paper_Space"]) {
        std::cerr << "Raw records=" << records.size() << " stop code=" << code << '\n';
        for (const auto& record : records)
            std::cerr << record.type << ':' << record.name << ' ';
        std::cerr << '\n';
        for (const auto& block : blocks)
            std::cerr << "BLOCK_RECORD " << block.first << "=" << block.second << '\n';
        return fail("space, named or anonymous BLOCK_RECORD identity changed");
    }
    for (const auto& block : blocks) {
        const auto found = blockDefinitions.find(block.first);
        if (found == blockDefinitions.end() || found->second.size() != 1u
            || found->second.front() != block.second)
            return fail("BLOCK/BLOCK_RECORD names or owner identities do not balance");
    }
    std::map<std::string, std::size_t> blockDefinitionChildCounts;
    std::map<std::string, std::vector<std::string>> blockChildTypes;
    std::map<std::string, std::size_t> ownedEntityCounts;
    std::map<std::string, std::string> blockNamesByHandle;
    for (const auto& block : blocks)
        blockNamesByHandle[block.second] = block.first;
    std::string activeBlockName;
    std::string activeBlockOwner;
    for (const auto& record : records) {
        if (record.section == "BLOCKS") {
            if (record.type == "BLOCK") {
                if (!activeBlockName.empty())
                    return fail("nested BLOCK header appeared before previous ENDBLK");
                activeBlockName = record.name;
                activeBlockOwner = record.owner;
            } else if (record.type == "ENDBLK") {
                if (activeBlockName.empty() || record.owner != activeBlockOwner)
                    return fail("ENDBLK owner does not close its active block definition");
                activeBlockName.clear();
                activeBlockOwner.clear();
            } else if (!activeBlockName.empty()
                       && (record.type == "LINE" || record.type == "POLYLINE"
                           || record.type == "INSERT" || record.type == "VIEWPORT")) {
                if (record.owner != activeBlockOwner)
                    return fail("block child owner differs from active BLOCK_RECORD");
                ++blockDefinitionChildCounts[activeBlockName];
                blockChildTypes[activeBlockName].push_back(record.type);
                if (record.type == "VIEWPORT" && record.viewportId != 42)
                    return fail("paper-space viewport child lost its distinct ID");
            }
        } else if (record.section == "ENTITIES"
                   && (record.type == "LINE" || record.type == "POLYLINE"
                       || record.type == "INSERT" || record.type == "VIEWPORT")) {
            const auto owner = blockNamesByHandle.find(record.owner);
            if (owner == blockNamesByHandle.end())
                return fail("ENTITIES record has no owning BLOCK_RECORD census key");
            ++ownedEntityCounts[owner->second];
            if (record.type == "VIEWPORT" && owner->second == "*Paper_Space"
                && record.viewportId != 42)
                return fail("paper-space viewport child lost its distinct ID");
        }
    }
    if (!activeBlockName.empty()
        || blockDefinitionChildCounts["*Model_Space"] != 0u
        || blockDefinitionChildCounts["*Paper_Space"] != 0u
        || blockDefinitionChildCounts["Named"] != 2u
        || blockDefinitionChildCounts["*U1"] != 1u
        || blockDefinitionChildCounts["*Paper_Space0"] != 0u
        || blockChildTypes["Named"] != std::vector<std::string>({"LINE", "POLYLINE"})
        || ownedEntityCounts["*Model_Space"] != 4u
        || ownedEntityCounts["*Paper_Space"] != 3u
        || ownedEntityCounts["*Paper_Space0"] != 1u
        || ownedEntityCounts["Named"] != 0u || ownedEntityCounts["*U1"] != 0u)
        return fail("per-BLOCK child census or paper-space viewport ownership changed");

    int lines = 0, vertices = 0, attributes = 0, sequences = 0, inserts = 0;
    int viewports = 0;
    std::string sequenceParent;
    for (const auto& record : records) {
        if (record.type == "POLYLINE" || record.type == "INSERT") {
            sequenceParent = record.handle;
            const std::string expectedOwner = record.section == "BLOCKS"
                ? blocks["Named"] : blocks["*Model_Space"];
            if (record.owner != expectedOwner)
                return fail("sequence parent owner does not name its source block");
            if (record.type == "INSERT") {
                ++inserts;
                if (record.name != "Named")
                    return fail("model-space INSERT target changed");
            }
            if (record.section == "ENTITIES") {
                const std::vector<std::pair<int, std::string>> expected{
                    {1001, "VERTEX_APP"}, {1000, record.type == "POLYLINE"
                        ? "parent-xdata" : "next-record-xdata"}};
                if (record.xdata != expected)
                    return fail("parent/next-record XDATA scope changed");
            } else if (!record.xdata.empty()) {
                return fail("block compound parent received another record's XDATA");
            }
        }
        if (record.type == "VIEWPORT" && record.owner == blocks["*Paper_Space"])
            ++viewports;
        if (record.type == "LINE") {
            const std::string owner = record.x == 1.25 || record.x == 301.25 ? blocks["*Model_Space"]
                : record.x == 11.25 || record.x == 401.25 ? blocks["*Paper_Space"]
                : record.x == 21.25 ? blocks["*Paper_Space0"]
                : record.x == 101.25 ? blocks["Named"] : blocks["*U1"];
            if (record.owner != owner)
                return fail("literal LINE owner does not name its emitted BLOCK_RECORD");
            ++lines;
        } else if (record.type == "VERTEX" || record.type == "ATTRIB"
                   || record.type == "SEQEND") {
            if (sequenceParent.empty() || record.owner != sequenceParent)
                return fail("minted sequence-child owner was remapped twice");
            if (record.type == "VERTEX") {
                const std::map<double, std::int32_t> expectedIdentifiers{
                    {1.0, 70001}, {2.0, 70002}, {51.0, 70003}, {52.0, 70004}};
                const auto expectedIdentifier = expectedIdentifiers.find(record.x);
                if (expectedIdentifier == expectedIdentifiers.end()
                    || record.identifier != expectedIdentifier->second)
                    return fail("classic VERTEX I32 identifier changed through conversion");
                if (record.x == 1.0 || record.x == 2.0) {
                    const std::string index = record.x == 1.0 ? "0" : "1";
                    if (record.xdata != std::vector<std::pair<int, std::string>>{
                            {1001, "VERTEX_APP"}, {1000, "vertex-xdata-" + index}})
                        return fail("distinct classic VERTEX XDATA is missing or merged");
                    if (record.appData != std::vector<std::string>{"{VERTEX_META",
                            "vertex-application-" + index, "DEADBEEF", "}"})
                        return fail("VERTEX common application scope changed");
                } else if (!record.xdata.empty() || !record.appData.empty()) {
                    return fail("block VERTEX inherited unrelated XDATA/application data");
                }
                ++vertices;
            }
            else if (record.type == "ATTRIB") ++attributes;
            else ++sequences;
        }
    }
    return lines == 7 && vertices == 4 && attributes == 1
        && inserts == 1 && sequences == 3 && viewports == 1
        ? true : fail("sentinel entity/sequence count changed");
}

bool verifyAnonymousBlockChild(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    char signature[22] {};
    input.read(signature, sizeof signature);
    const bool binary = std::string(signature, sizeof signature)
        == std::string("AutoCAD Binary DXF\r\n\x1a\0", 22);
    input.clear(); input.seekg(binary ? 22 : 0);
    std::unique_ptr<dxfReader> reader;
    if (binary) reader = std::make_unique<dxfReaderBinary>(&input);
    else reader = std::make_unique<dxfReaderAscii>(&input);
    std::string section;
    std::vector<Record> records;
    Record current;
    int code = 0;
    bool eof = false;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!current.type.empty()) records.push_back(current);
            current = Record{};
            current.type = reader->getString();
            if (current.type == "ENDSEC") {
                current.section = section;
                section.clear();
            } else if (current.type != "SECTION") {
                current.section = section;
            }
            eof = current.type == "EOF";
        } else if (code == 2) {
            current.name = reader->getString();
            if (current.type == "SECTION") section = current.name;
        } else if (code == 5) current.handle = reader->getString();
        else if (code == 330) current.owner = reader->getString();
        else if (code == 10) current.x = reader->getDouble();
    }
    if (!eof)
        return fail("anonymous-block output did not reach EOF");

    std::string recordHandle;
    std::size_t blockRecordCount = 0;
    std::size_t blockHeaderCount = 0;
    std::size_t childLineCount = 0;
    std::size_t sentinelLineCount = 0;
    bool childLeakedToEntities = false;
    for (const Record& record : records) {
        if (record.type == "BLOCK_RECORD" && record.name == "*D999") {
            recordHandle = record.handle;
            ++blockRecordCount;
        } else if (record.type == "BLOCK" && record.section == "BLOCKS"
                   && record.name == "*D999") {
            ++blockHeaderCount;
            if (record.owner != recordHandle)
                return fail("anonymous BLOCK header owner differs from its emitted record");
        } else if (record.type == "LINE" && record.x == 901.0) {
            ++sentinelLineCount;
            if (record.section == "ENTITIES")
                childLeakedToEntities = true;
            if (record.section == "BLOCKS" && record.owner == recordHandle)
                ++childLineCount;
        }
    }
    return !recordHandle.empty() && blockRecordCount == 1u
        && blockHeaderCount == 1u && childLineCount == 1u
        && sentinelLineCount == 1u
        && !childLeakedToEntities
        ? true : fail("anonymous *D child was dropped, duplicated or re-homed");
}

bool runLocal(const std::filesystem::path& directory) {
    for (unsigned control = 0; control != 4; ++control) {
        const bool uppercaseFixed = control >= 2;
        const bool binary = control % 2 != 0;
        const std::string prefix = uppercaseFixed ? "uppercase " : "mixed case ";
        const auto input = directory / (prefix + (binary ? "input binary.dxf" : "input ascii.dxf"));
        const auto output = directory / (prefix + (binary ? "output binary.dxf" : "output ascii.dxf"));
        if (!generate(input.string(), binary, false, uppercaseFixed)) return false;
        dx_data data;
        dx_iface iface;
        if (!iface.fileImport(input.string(), &data, false))
            return fail("literal source control failed to import");
        if (data.blockRecords.size() != 5)
            return fail("converter did not retain source BLOCK_RECORD callbacks");
        const auto& named = data.blockRecords[2];
        if (named.handle != 0x1Fu || named.name != "Named" || named.insUnits != 4
            || named.canExplode || named.blockScaling != 1
            || named.previewData != std::vector<std::uint8_t>({1, 2, 3, 4}))
            return fail("source BLOCK_RECORD metadata was not copied intact");
        if (!iface.fileExport(output.string(), DRW::AC1027, binary, &data, false)
            || !verify(output.string())) return false;
    }
    return true;
}

bool testMetadataCopy() {
    dx_data data;
    dx_iface iface;
    iface.cData = &data;
    DRW_Block_Record source;
    source.handle = 0x17; source.name = "*Model_Space";
    source.insUnits = 4; source.layoutHandle = 0xAB;
    source.description = "source description"; source.xrefPath = "external.dwg";
    source.insertHandles = {0xA1, 0xA2}; source.reactorHandles = {0xD1};
    source.xDictHandle = 0xD2;
    source.extData.push_back(new DRW_Variant(1001, "SOURCE_APP"));
    iface.addBlockRecord(source);
    source.extData.front()->addString(1001, "MUTATED");
    const auto& copy = data.blockRecords.front();
    return copy.handle == 0x17 && copy.layoutHandle == 0xAB
        && copy.description == "source description" && copy.xrefPath == "external.dwg"
        && copy.insertHandles == std::vector<std::uint32_t>({0xA1, 0xA2})
        && copy.reactorHandles == std::vector<std::uint32_t>({0xD1})
        && copy.xDictHandle == 0xD2 && copy.extData.front() != source.extData.front()
        && copy.extData.front()->c_str() == std::string("SOURCE_APP")
        ? true : fail("BLOCK_RECORD copy lost owned metadata or aliasing safety");
}

bool testStrictRejection(const std::filesystem::path& directory) {
    for (bool binary : {false, true}) {
        dx_data data;
        DRW_Block_Record model;
        model.handle = 0x17; model.name = "*Model_Space";
        data.blockRecords.push_back(model);
        auto* line = new DRW_Line;
        line->handle = 0xA0; line->parentHandle = 0xDEAD;
        line->secPoint = DRW_Coord(1, 2, 3);
        data.mBlock->ent.push_back(line);
        const auto output = directory / (binary ? "reject binary.dxf" : "reject ascii.dxf");
        const std::string original = "original destination bytes\n";
        { std::ofstream stream(output, std::ios::binary); stream << original; }
        dx_iface iface;
        if (iface.fileExport(output.string(), DRW::AC1027, binary, &data, false, true))
            return fail("strict converter accepted an unresolved explicit owner");
        std::ifstream stream(output, std::ios::binary);
        const std::string retained((std::istreambuf_iterator<char>(stream)), {});
        if (retained != original || line->handle != 0xA0)
            return fail("owner rejection changed destination or caller identity");
    }
    return true;
}

bool testAnonymousBlockChildOwnership(const std::filesystem::path& directory) {
    for (bool binary : {false, true}) {
        dx_data data;
        DRW_Block_Record model;
        model.handle = 0x17; model.name = "*Model_Space";
        data.blockRecords.push_back(model);
        auto* anonymous = new dx_ifaceBlock;
        anonymous->name = "*D999";
        anonymous->parentHandle = 0xDEADu; // No source BLOCK_RECORD has this ID.
        auto* line = new DRW_Line;
        line->handle = 0xA0u; line->parentHandle = 0xDEADu;
        line->basePoint = DRW_Coord(901, 902, 903);
        line->secPoint = DRW_Coord(904, 905, 906);
        anonymous->ent.push_back(line);
        data.blocks.push_back(anonymous);

        const auto output = directory / (binary
            ? "anonymous block identity binary.dxf"
            : "anonymous block identity ascii.dxf");
        dx_iface iface;
        const bool exported = iface.fileExport(
            output.string(), DRW::AC1027, binary, &data, false, true);
        if (!exported || !verifyAnonymousBlockChild(output.string())
            || anonymous->ent.size() != 1u
            || anonymous->ent.front() != line || line->parentHandle != 0xDEADu
            || !data.mBlock->ent.empty()) {
            return fail("anonymous *D ownership changed caller graph or output");
        }
    }
    return true;
}

class OverrideSink final : public dx_iface {
public:
    void writeEntities() override {
        DRW_Attdef attribute;
        attribute.handle = 0xA0;
        attribute.parentHandle = 0x1F; // This source ID maps to Named, not Model.
        attribute.tag = "OVERRIDE"; attribute.text = "sentinel";
        attribute.height = 2.5;
        dxfW->writeAttdef(&attribute, DRW::DxfModelSpaceBlockRecordHandle);
    }
};

bool verifyOwner(const std::filesystem::path& path, const std::string& type,
                 const std::string& expected) {
    std::ifstream input(path);
    std::unique_ptr<dxfReader> reader = std::make_unique<dxfReaderAscii>(&input);
    std::string current;
    int code = 0;
    while (reader->readRec(&code)) {
        if (code == 0) current = reader->getString();
        else if (current == type && code == 330)
            return reader->getString() == expected;
    }
    return false;
}

bool testMintedOverrideAndLegacyFallback(const std::filesystem::path& directory) {
    dx_data data;
    DRW_Block_Record named;
    named.handle = 0x1F; named.name = "Named";
    data.blockRecords.push_back(named);
    auto* block = new dx_ifaceBlock;
    block->name = "Named"; block->parentHandle = named.handle;
    data.blocks.push_back(block);
    OverrideSink sink;
    const auto overridePath = directory / "minted override.dxf";
    if (!sink.fileExport(overridePath.string(), DRW::AC1027, false, &data, false, true)
        || !verifyOwner(overridePath, "ATTDEF", "1F"))
        return fail("already-minted owner override was translated as a source ID");

    dx_data legacy;
    auto* line = new DRW_Line;
    line->secPoint = DRW_Coord(1, 2, 3);
    legacy.mBlock->ent.push_back(line);
    dx_iface iface;
    const auto legacyPath = directory / "legacy fallback.dxf";
    if (!iface.fileExport(legacyPath.string(), DRW::AC1027, false, &legacy, false)
        || !verifyOwner(legacyPath, "LINE", "1F"))
        return fail("metadata-free legacy writer lost its modelspace fallback");
    line->parentHandle = 0xA7;
    const auto explicitLegacyPath = directory / "legacy explicit owner.dxf";
    if (!iface.fileExport(explicitLegacyPath.string(), DRW::AC1027, false, &legacy, false)
        || !verifyOwner(explicitLegacyPath, "LINE", "A7"))
        return fail("metadata-free legacy writer changed an explicit caller owner");
    return true;
}

bool testSourceIdentityGuards(const std::filesystem::path& directory) {
    // A full callback and a BLOCK parent alias may repeat the same identity.
    // Conflicting aliases must fail instead of choosing whichever came first.
    for (bool conflictingName : {false, true}) {
        dx_data data;
        DRW_Block_Record first;
        first.handle = 0x17; first.name = "*Model_Space";
        data.blockRecords.push_back(first);
        DRW_Block_Record second = first;
        if (conflictingName) second.name = "*Paper_Space";
        else second.handle = 0x18;
        data.blockRecords.push_back(second);
        const auto output = directory / (conflictingName
            ? "conflicting source names.dxf" : "conflicting source handles.dxf");
        dx_iface iface;
        if (iface.fileExport(output.string(), DRW::AC1027, false, &data, false, true)
            || std::filesystem::exists(output))
            return fail("ambiguous source BLOCK_RECORD identities published output");
    }

    dx_data data;
    DRW_Block_Record named;
    named.handle = 0x1F; named.name = "Named";
    data.blockRecords.push_back(named);
    DRW_Block_Record other;
    other.handle = 0x30; other.name = "Other";
    data.blockRecords.push_back(other);
    for (const DRW_Block_Record& record : data.blockRecords) {
        auto* block = new dx_ifaceBlock;
        block->name = record.name; block->parentHandle = record.handle;
        if (record.name == "Named") {
            auto* line = new DRW_Line;
            line->secPoint = DRW_Coord(1, 2, 3);
            // Parent zero requests the active emitted block. Its handle30
            // also happens to be a different source block's identity.
            block->ent.push_back(line);
        }
        data.blocks.push_back(block);
    }
    dx_iface iface;
    const auto output = directory / "active block handle.dxf";
    if (!iface.fileExport(output.string(), DRW::AC1027, false, &data, false, true)
        || !verifyOwner(output, "LINE", "30"))
        return fail("active emitted block identity was translated twice");
    return true;
}

// Literal codec vectors, not independently qualified/evaluable ACIS geometry.
// The same records exercise direct delivery and the private BLOCK event queue.
enum class CallbackFooter { Complete, Missing, WrongSubclass };

bool generateCallbackControl(const std::filesystem::path& path, bool binary,
                             bool inBlock, CallbackFooter footer = CallbackFooter::Complete,
                             bool surfaceOnly = false) {
    std::ofstream output(path, std::ios::binary);
    if (!output) return false;
    std::unique_ptr<dxfWriter> writer;
    if (binary) {
        output.write("AutoCAD Binary DXF\r\n\x1a\0", 22);
        writer = std::make_unique<dxfWriterBinary>(&output);
    } else writer = std::make_unique<dxfWriterAscii>(&output);
    writer->setVersion("AC1027", true);
    const auto text = [&writer](int code, const std::string& value) {
        writer->writeString(code, value);
    };
    const auto integer = [&writer](int code, int value) {
        if (code >= 90 && code <= 99) writer->writeInt32(code, value);
        else writer->writeInt16(code, value);
    };
    const auto real = [&writer](int code, double value) { writer->writeDouble(code, value); };
    const auto boolean = [&writer](int code, bool value) { writer->writeBool(code, value); };
    const auto section = [&text](const char* name) { text(0, "SECTION"); text(2, name); };
    const auto common = [&text](const char* type, const std::string& handle, const char* owner) {
        text(0, type); text(5, handle); text(330, owner);
        text(100, "AcDbEntity"); text(8, "0");
    };
    const auto matrix = [&real](int code, double sentinel) {
        for (unsigned i = 0; i != 16; ++i)
            real(code, i == 3 ? sentinel : (i % 5 == 0 ? 1.0 : 0.0));
    };
    section("HEADER"); text(9, "$ACADVER"); text(1, "AC1027"); text(0, "ENDSEC");
    section("TABLES");
    text(0, "TABLE"); text(2, "BLOCK_RECORD"); text(5, "1");
    text(330, "0"); text(100, "AcDbSymbolTable"); integer(70, 2);
    for (const auto& item : {std::pair<const char*, const char*>{"17", "*Model_Space"},
                             {"1F", "Witness"}}) {
        text(0, "BLOCK_RECORD"); text(5, item.first); text(330, "1");
        text(100, "AcDbSymbolTableRecord"); text(100, "AcDbBlockTableRecord");
        text(2, item.second); integer(70, 0);
    }
    text(0, "ENDTAB"); text(0, "ENDSEC");
    section(inBlock ? "BLOCKS" : "ENTITIES");
    if (inBlock) {
        common("BLOCK", "80", "1F"); text(100, "AcDbBlockBegin");
        text(2, "Witness"); integer(70, 0); real(10, 0); real(20, 0); real(30, 0);
        text(3, "Witness"); text(1, "");
    }
    const char* owner = inBlock ? "1F" : "17";
    const char* names[] = {"PLANESURFACE", "EXTRUDEDSURFACE", "REVOLVEDSURFACE",
                           "SWEPTSURFACE", "LOFTEDSURFACE", "NURBSSURFACE"};
    const char* subclasses[] = {"AcDbPlaneSurface", "AcDbExtrudedSurface",
        "AcDbRevolvedSurface", "AcDbSweptSurface", "AcDbLoftedSurface", "AcDbNurbSurface"};
    for (unsigned i = 0; i != 6; ++i) {
        common(names[i], "A" + std::to_string(i), owner);
        integer(92, 2); text(310, "AABB");
        text(100, "AcDbModelerGeometry"); integer(70, 1);
        // The CLI generator uses empty shells so writer carrier policy does
        // not confound the concrete-copy regression. Ordinary controls retain
        // binary common payloads and all four typed/raw callback routes.
        if (!surfaceOnly) text(310, "010203");
        text(100, "AcDbSurface"); integer(71, static_cast<int>(i + 2));
        integer(72, static_cast<int>(i + 3)); text(100, subclasses[i]);
        if (i == 1) {
            integer(90, 12345); integer(90, 2); text(310, "BEEF");
            real(10, 1); real(20, 2); real(30, 3); matrix(40, 4.25); real(42, 0.75);
        } else if (i == 2) {
            integer(90, 23456); integer(90, 2); text(310, "CAFE");
            real(10, 1); real(20, 2); real(30, 3); real(11, 0); real(21, 0); real(31, 1);
            real(40, 2.75); matrix(42, 5.25); boolean(291, true);
        } else if (i == 3) {
            integer(90, 34567); integer(90, 2); text(310, "1234");
            integer(90, 45678); integer(90, 2); text(310, "5678");
            matrix(40, 6.25); matrix(41, 7.25); matrix(46, 9.25); matrix(47, 10.25);
            real(48, 1.75); boolean(293, true);
        } else if (i == 4) {
            matrix(40, 8.25); integer(70, 2); real(41, 0.25);
            integer(90, 2); text(310, "9ABC"); boolean(294, true);
        } else if (i == 5) {
            integer(170, 123); boolean(290, true);
            for (int point = 0; point != 4; ++point) {
                real(10 + point, point + 1); real(20 + point, point + 2); real(30 + point, point + 3);
            }
        }
    }
    if (!surfaceOnly) {
    common("TOLERANCE", "A6", owner); text(100, "AcDbFcf");
    text(1, "tolerance sentinel"); text(3, "STANDARD"); real(10, 1); real(20, 2); real(30, 3);
    common("CAMERA", "A7", owner); text(100, "AcDbCamera"); text(340, "DEAD");
    common("GEOPOSITIONMARKER", "A8", owner); text(100, "AcDbGeoPositionMarker");
    integer(90, 7); real(10, 1); real(20, 2); real(30, 3); text(1, "geo sentinel");
    real(40, 2.5); real(40, 0.5);
    common("SECTIONOBJECT", "A9", owner); text(100, "AcDbSection");
    integer(90, 1); text(1, "section sentinel"); real(40, 4.5); real(41, -2.5);
    integer(92, 2);
    for (int point = 0; point != 2; ++point) {
        real(11, point + 1); real(21, point + 2); real(31, point + 3);
    }
    integer(93, 1); real(12, 4); real(22, 5); real(32, 6);
    }
    const auto line = [&common, &text, &real](const char* handle, const char* lineOwner, double x) {
        common("LINE", handle, lineOwner); text(100, "AcDbLine");
        real(10, x); real(20, 2); real(30, 3); real(11, x + 1); real(21, 5); real(31, 6);
    };
    line("AA", owner, 101.25);
    if (inBlock) {
        if (footer != CallbackFooter::Missing) {
            common("ENDBLK", "81", "1F");
            text(100, footer == CallbackFooter::WrongSubclass ? "AcDbLine" : "AcDbBlockEnd");
        }
        text(0, "ENDSEC"); section("ENTITIES");
    }
    line("AB", "17", 201.25); text(0, "ENDSEC"); text(0, "EOF");
    return !writer->hasWriteError() && output.good();
}

class BlockCallbackSink final : public dx_iface {
public:
    void addHeader(const DRW_Header*) override {}
    void addBlockRecord(const DRW_Block_Record&) override {}
    void addBlock(const DRW_Block&) override { events.push_back("BLOCK"); ++blocks; }
    void endBlock() override {
        events.push_back("ENDBLK"); ++ends;
        if (throwCleanup) throw std::logic_error("secondary block cleanup");
    }
    void addLine(const DRW_Line& data) override {
        events.push_back(data.basePoint.x == 101.25 ? "inner LINE" : "outer LINE");
    }
    void addSurface(const DRW_Surface* data) override {
        if (data == nullptr) { completeTypes = false; return; }
        switch (data->eType) {
        case DRW::PLANESURFACE: copySurface<DRW_PlaneSurface>(data, "PLANESURFACE"); break;
        case DRW::EXTRUDEDSURFACE: copySurface<DRW_ExtrudedSurface>(data, "EXTRUDEDSURFACE"); break;
        case DRW::REVOLVEDSURFACE: copySurface<DRW_RevolvedSurface>(data, "REVOLVEDSURFACE"); break;
        case DRW::SWEPTSURFACE: copySurface<DRW_SweptSurface>(data, "SWEPTSURFACE"); break;
        case DRW::LOFTEDSURFACE: copySurface<DRW_LoftedSurface>(data, "LOFTEDSURFACE"); break;
        case DRW::NURBSURFACE: copySurface<DRW_NurbsSurface>(data, "NURBSSURFACE"); break;
        default: completeTypes = false; break;
        }
        if (throwSurface) throwReplay();
    }
    void addTolerance(const DRW_Tolerance& data) override { events.push_back("TOLERANCE"); tolerance = data; }
    void addCamera(const DRW_Camera& data) override { events.push_back("CAMERA"); camera = data; }
    void addGeoPositionMarker(const DRW_GeoPositionMarker& data) override { events.push_back("GEOPOSITIONMARKER"); geo = data; }
    void addSectionObject(const DRW_SectionObject& data) override { events.push_back("SECTIONOBJECT"); section = data; }
    void addRawDxfEntity(const DRW_RawDxfObject& data) override {
        events.push_back("raw " + data.name); raw.push_back(data);
        if (throwRaw && data.name == "SECTIONOBJECT") throwReplay();
    }
    bool completeTypes {true}, throwSurface {false}, throwRaw {false};
    bool throwCleanup {false}, unknownException {false};
    int blocks {0}, ends {0};
    std::vector<std::string> events;
    std::vector<std::shared_ptr<DRW_Surface>> surfaces;
    DRW_Tolerance tolerance;
    DRW_Camera camera;
    DRW_GeoPositionMarker geo;
    DRW_SectionObject section;
    std::vector<DRW_RawDxfObject> raw;
private:
    void throwReplay() const {
        if (unknownException) throw 42;
        throw std::runtime_error("primary block replay");
    }

    template <typename T>
    void copySurface(const DRW_Surface* data, const char* name) {
        events.push_back(name);
        const auto* typed = dynamic_cast<const T*>(data);
        if (typed == nullptr) { completeTypes = false; return; }
        surfaces.push_back(std::make_shared<T>(*typed));
    }
};

bool readCallbackControl(dxfRW& reader, BlockCallbackSink& sink,
                         const std::filesystem::path& path, bool binary, bool ext) {
    if (binary) return reader.read(&sink, ext);
    std::ifstream stream(path, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(stream)), {});
    const std::string original = content;
    const bool ok = reader.readAscii(&sink, ext, content);
    return ok && content == original;
}

bool verifyCallbackCopies(const BlockCallbackSink& sink, bool inBlock) {
    std::vector<std::string> expected {"PLANESURFACE", "EXTRUDEDSURFACE", "REVOLVEDSURFACE",
        "SWEPTSURFACE", "LOFTEDSURFACE", "NURBSSURFACE", "TOLERANCE", "CAMERA",
        "GEOPOSITIONMARKER", "raw GEOPOSITIONMARKER", "SECTIONOBJECT", "raw SECTIONOBJECT", "inner LINE"};
    if (inBlock) { expected.insert(expected.begin(), "BLOCK"); expected.push_back("ENDBLK"); }
    expected.push_back("outer LINE");
    if (sink.events != expected || !sink.completeTypes || sink.surfaces.size() != 6
        || sink.blocks != int(inBlock) || sink.ends != int(inBlock))
        return fail("BLOCK callback type/count/order differs from direct delivery");
    for (std::size_t i = 0; i != sink.surfaces.size(); ++i) {
        const auto& value = *sink.surfaces[i];
        if (value.handle != 0xA0u + i || value.parentHandle != (inBlock ? 0x1Fu : 0x17u)
            || value.numProxyGraph != 2 || value.proxyGraphics != std::string("\xAA\xBB", 2)
            || value.rawAcisData != std::vector<std::uint8_t>({1, 2, 3})
            || value.dxfPayloadChunks.size() != 1 || value.dxfPayloadChunks[0].m_length != 3
            || value.modelerFormatVersion != 1 || value.uIsolines != int(i + 2)
            || value.vIsolines != int(i + 3))
            return fail("queued surface lost base metadata or owned carrier bytes");
    }
    const auto* extruded = dynamic_cast<const DRW_ExtrudedSurface*>(sink.surfaces[1].get());
    const auto* revolved = dynamic_cast<const DRW_RevolvedSurface*>(sink.surfaces[2].get());
    const auto* swept = dynamic_cast<const DRW_SweptSurface*>(sink.surfaces[3].get());
    const auto* lofted = dynamic_cast<const DRW_LoftedSurface*>(sink.surfaces[4].get());
    const auto* nurbs = dynamic_cast<const DRW_NurbsSurface*>(sink.surfaces[5].get());
    if (dynamic_cast<const DRW_PlaneSurface*>(sink.surfaces[0].get()) == nullptr
        || extruded == nullptr || extruded->classId != 12345 || extruded->draftAngle != 0.75
        || extruded->extrudedTransform[3] != 4.25
        || extruded->dxfBinaryData != std::vector<std::uint8_t>({0xBE, 0xEF})
        || revolved == nullptr || revolved->id != 23456 || revolved->revolveAngle != 2.75
        || revolved->transform[3] != 5.25 || !revolved->closeToAxis
        || revolved->dxfBinaryData != std::vector<std::uint8_t>({0xCA, 0xFE})
        || swept == nullptr || swept->sweepEntityId != 34567 || swept->pathEntityId != 45678
        || swept->sweepEntityTransformed[3] != 6.25 || swept->pathEntityTransformed[3] != 7.25
        || swept->sweepEntityTransform[3] != 9.25 || swept->pathEntityTransform[3] != 10.25
        || swept->scaleFactor != 1.75 || !swept->bank
        || swept->sweepData != std::vector<std::uint8_t>({0x12, 0x34})
        || swept->pathData != std::vector<std::uint8_t>({0x56, 0x78})
        || lofted == nullptr || lofted->loftEntityTransform[3] != 8.25
        || lofted->startDraftAngle != 0.25 || !lofted->closedSurfaces
        || lofted->dxfReferenceData.size() != 2
        || lofted->dxfReferenceData[1].binary() == nullptr
        || *lofted->dxfReferenceData[1].binary() != std::vector<std::uint8_t>({0x9A, 0xBC})
        || nurbs == nullptr || nurbs->short170 != 123 || !nurbs->cvHullDisplay
        || nurbs->vvec2.z != 6)
        return fail("queued surface lost concrete subtype fields or owned arrays");
    return sink.tolerance.text == "tolerance sentinel" && sink.camera.m_viewHandle == 0xDEADu
        && sink.geo.m_notes == "geo sentinel" && sink.geo.m_landingGap == 0.5
        && sink.section.m_verts.size() == 2 && sink.section.m_verts[1].z == 4
        && sink.section.m_blVerts.size() == 1 && sink.section.m_blVerts[0].z == 6
        && sink.raw.size() == 2 && sink.raw[0].handle == 0xA8u && sink.raw[1].handle == 0xA9u
        ? true : fail("typed/raw BLOCK companions lost metadata or cardinality");
}

bool testQueuedBlockCallbacks(const std::filesystem::path& directory) {
    for (bool binary : {false, true}) for (bool ext : {false, true}) {
        for (bool inBlock : {false, true}) {
            const auto path = directory / "queued callbacks.dxf";
            if (!generateCallbackControl(path, binary, inBlock)) return false;
            BlockCallbackSink sink;
            dxfRW reader(path.string().c_str());
            if (!readCallbackControl(reader, sink, path, binary, ext)
                || reader.getError() != DRW::BAD_NONE || !verifyCallbackCopies(sink, inBlock))
                return fail("queued callback control failed to read");
            // Retained snapshots are inspected after parser locals and queue expire.
            // A second import on the same facade must not alias or invalidate them.
            BlockCallbackSink second;
            if (!readCallbackControl(reader, second, path, binary, ext)
                || !verifyCallbackCopies(second, inBlock) || !verifyCallbackCopies(sink, inBlock)
                || sink.surfaces[1].get() == second.surfaces[1].get())
                return fail("queued callback snapshots or same-facade reuse changed");
        }
        const auto path = directory / "queued callbacks failure.dxf";
        dxfRW reader(path.string().c_str());
        for (const auto footer : {CallbackFooter::Missing, CallbackFooter::WrongSubclass}) {
            if (!generateCallbackControl(path, binary, true, footer)) return false;
            BlockCallbackSink malformed;
            if (readCallbackControl(reader, malformed, path, binary, ext)
                || reader.getError() != DRW::BAD_READ_BLOCKS
                || malformed.events != std::vector<std::string>{"BLOCK", "ENDBLK"})
                return fail("malformed ENDBLK published staged children or unbalanced scope");
            if (!generateCallbackControl(path, binary, true)) return false;
            BlockCallbackSink corrected;
            if (!readCallbackControl(reader, corrected, path, binary, ext)
                || !verifyCallbackCopies(corrected, true))
                return fail("corrected BLOCK did not reuse the failed facade");
        }
        for (bool unknown : {false, true}) for (bool cleanupThrows : {false, true})
                for (bool late : {false, true}) {
            BlockCallbackSink throwing;
            throwing.throwSurface = !late; throwing.throwRaw = late;
            throwing.unknownException = unknown;
            throwing.throwCleanup = cleanupThrows;
            bool primaryPreserved = false;
            try {
                const bool ignored = readCallbackControl(reader, throwing, path, binary, ext);
                (void)ignored;
            } catch (const std::runtime_error& exception) {
                primaryPreserved = !unknown && std::string(exception.what()) == "primary block replay";
            } catch (int value) { primaryPreserved = unknown && value == 42; }
            catch (...) {}
            const std::vector<std::string> expected = late
                ? std::vector<std::string>{"BLOCK", "PLANESURFACE", "EXTRUDEDSURFACE", "REVOLVEDSURFACE",
                    "SWEPTSURFACE", "LOFTEDSURFACE", "NURBSSURFACE", "TOLERANCE", "CAMERA",
                    "GEOPOSITIONMARKER", "raw GEOPOSITIONMARKER", "SECTIONOBJECT", "raw SECTIONOBJECT", "ENDBLK"}
                : std::vector<std::string>{"BLOCK", "PLANESURFACE", "ENDBLK"};
            if (!primaryPreserved || throwing.blocks != 1 || throwing.ends != 1
                || throwing.events != expected)
                return fail("BLOCK replay did not balance once and preserve its primary exception");
            BlockCallbackSink retry;
            if (!readCallbackControl(reader, retry, path, binary, ext)
                || !verifyCallbackCopies(retry, true))
                return fail("BLOCK replay exception prevented corrected same-facade retry");
        }
        BlockCallbackSink cleanupOnly;
        cleanupOnly.throwCleanup = true;
        bool cleanupPreserved = false;
        try {
            const bool ignored = readCallbackControl(reader, cleanupOnly, path, binary, ext);
            (void)ignored;
        } catch (const std::logic_error& exception) {
            cleanupPreserved = std::string(exception.what()) == "secondary block cleanup";
        } catch (...) {}
        if (!cleanupPreserved || cleanupOnly.blocks != 1 || cleanupOnly.ends != 1
            || cleanupOnly.events.back() != "ENDBLK" || cleanupOnly.surfaces.size() != 6)
            return fail("ordinary ENDBLK exception was swallowed or invoked twice");
        BlockCallbackSink retry;
        if (!readCallbackControl(reader, retry, path, binary, ext) || !verifyCallbackCopies(retry, true))
            return fail("ordinary ENDBLK exception prevented same-facade retry");
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 3) {
        const std::string mode = argv[1];
        if (mode == "--generate") return generate(argv[2], false, false, true) ? 0 : 1;
        if (mode == "--generate-binary") return generate(argv[2], true, false, true) ? 0 : 1;
        if (mode == "--generate-dangling") return generate(argv[2], false, true, true) ? 0 : 1;
        if (mode == "--generate-dangling-binary") return generate(argv[2], true, true, true) ? 0 : 1;
        if (mode == "--generate-callbacks") return generateCallbackControl(argv[2], false, true, CallbackFooter::Complete, true) ? 0 : 1;
        if (mode == "--generate-callbacks-binary") return generateCallbackControl(argv[2], true, true, CallbackFooter::Complete, true) ? 0 : 1;
        if (mode == "--verify") return verify(argv[2]) ? 0 : 1;
        return 2;
    }
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-block-owners-" + std::to_string(nonce));
    std::filesystem::create_directory(directory);
    const bool success = runLocal(directory) && testMetadataCopy()
        && testStrictRejection(directory) && testAnonymousBlockChildOwnership(directory)
        && testMintedOverrideAndLegacyFallback(directory)
        && testSourceIdentityGuards(directory) && testQueuedBlockCallbacks(directory);
    if (success) std::filesystem::remove_all(directory);
    else std::cerr << "Evidence: " << directory << '\n';
    return success ? 0 : 1;
}
