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
        if (std::string(item.second) == "Named") line("B0", item.first, 101.25);
        if (std::string(item.second) == "*U1") line("B1", item.first, 201.25);
        if (std::string(item.first) == "17") line("B2", item.first, 301.25);
        if (std::string(item.first) == "1B") line("B3", item.first, 401.25, true);
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
    std::string type, handle, owner, name;
    double x {0};
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
    int code = 0;
    bool eof = false;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!current.type.empty()) records.push_back(current);
            current = Record{}; current.type = reader->getString();
            eof = current.type == "EOF";
        } else if (code == 5) current.handle = reader->getString();
        else if (code == 330) current.owner = reader->getString();
        else if (code == 2) current.name = reader->getString();
        else if (code == 10) current.x = reader->getDouble();
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
    std::set<std::string> identities;
    for (const auto& record : records) {
        if (record.type == "BLOCK_RECORD") blocks[record.name] = record.handle;
        if (record.type != "SECTION" && !record.handle.empty()
            && !identities.insert(record.handle).second)
            return fail("emitted identities collide");
    }
    if (blocks.size() != 5 || blocks["*Model_Space"] != "1F"
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
    int lines = 0, vertices = 0, attributes = 0, sequences = 0, inserts = 0;
    std::string sequenceParent;
    for (const auto& record : records) {
        if (record.type == "POLYLINE" || record.type == "INSERT") {
            sequenceParent = record.handle;
            if (record.owner != blocks["*Model_Space"])
                return fail("sequence parent owner does not name modelspace");
            if (record.type == "INSERT") ++inserts;
            const std::vector<std::pair<int, std::string>> expected{
                {1001, "VERTEX_APP"}, {1000, record.type == "POLYLINE"
                    ? "parent-xdata" : "next-record-xdata"}};
            if (record.xdata != expected)
                return fail("parent/next-record XDATA scope changed");
        }
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
                const std::string index = std::to_string(vertices);
                if (record.xdata != std::vector<std::pair<int, std::string>>{
                        {1001, "VERTEX_APP"}, {1000, "vertex-xdata-" + index}})
                    return fail("distinct classic VERTEX XDATA is missing or merged");
                if (record.appData != std::vector<std::string>{"{VERTEX_META",
                        "vertex-application-" + index, "DEADBEEF", "}"})
                    return fail("VERTEX common application scope changed");
                ++vertices;
            }
            else if (record.type == "ATTRIB") ++attributes;
            else ++sequences;
        }
    }
    return lines == 7 && vertices == 2 && attributes == 1
        && inserts == 1 && sequences == 2
        ? true : fail("sentinel entity/sequence count changed");
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

} // namespace

int main(int argc, char** argv) {
    if (argc == 3) {
        const std::string mode = argv[1];
        if (mode == "--generate") return generate(argv[2], false, false, true) ? 0 : 1;
        if (mode == "--generate-binary") return generate(argv[2], true, false, true) ? 0 : 1;
        if (mode == "--generate-dangling") return generate(argv[2], false, true, true) ? 0 : 1;
        if (mode == "--generate-dangling-binary") return generate(argv[2], true, true, true) ? 0 : 1;
        if (mode == "--verify") return verify(argv[2]) ? 0 : 1;
        return 2;
    }
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-block-owners-" + std::to_string(nonce));
    std::filesystem::create_directory(directory);
    const bool success = runLocal(directory) && testMetadataCopy()
        && testStrictRejection(directory) && testMintedOverrideAndLegacyFallback(directory)
        && testSourceIdentityGuards(directory);
    if (success) std::filesystem::remove_all(directory);
    else std::cerr << "Evidence: " << directory << '\n';
    return success ? 0 : 1;
}
