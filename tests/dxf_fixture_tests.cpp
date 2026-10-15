#include <cstddef>
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <clocale>
#include <cmath>
#include <memory>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "drw_entities.h"
#include "drw_objects.h"
#include "dwg2dxf/dx_data.h"
#include "dwg2dxf/dx_iface.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"
#include "intern/drw_textcodec.h"
#include "intern/dwgbufferw.h"

namespace {

struct TestContext {
    int failures {0};

    void expect(bool condition, const char* label) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << label << '\n';
        }
    }
};

class FixtureInterface final : public dx_iface {
public:
    std::vector<DRW_RawDxfObject> rawEntities;
    std::vector<DRW_RawDxfObject> rawObjects;
    std::vector<DRW_Class> classes;
    std::vector<DRW_Dictionary> dictionaries;
    std::vector<DRW_Material> materials;
    std::vector<DRW_XRecord> xrecords;
    std::vector<DRW_Block_Record> blockRecords;
    std::vector<DRW_Scale> scales;
    std::vector<DRW_DictionaryVar> dictionaryVars;
    std::vector<DRW_Group> groups;
    std::vector<DRW_PlotSettings> plotSettings;
    std::vector<DRW_Layout> layouts;

    void addPlotSettings(const DRW_PlotSettings* value) override {
        if (value != nullptr)
            plotSettings.push_back(*value);
    }

    void addLayout(const DRW_Layout& value) override {
        layouts.push_back(value);
    }

    void addBlockRecord(const DRW_Block_Record& value) override {
        blockRecords.push_back(value);
    }

    void addRawDxfEntity(const DRW_RawDxfObject& value) override {
        rawEntities.push_back(value);
    }

    void addRawDxfObject(const DRW_RawDxfObject& value) override {
        rawObjects.push_back(value);
    }

    void addDxfClass(const DRW_Class& value) override {
        classes.push_back(value);
    }

    void addDictionary(const DRW_Dictionary& value) override {
        dictionaries.push_back(value);
    }

    void addMaterial(const DRW_Material& value) override {
        materials.push_back(value);
    }

    void addXRecord(const DRW_XRecord& value) override {
        xrecords.push_back(value);
    }

    void addScale(const DRW_Scale& value) override {
        scales.push_back(value);
    }

    void addDictionaryVar(const DRW_DictionaryVar& value) override {
        dictionaryVars.push_back(value);
    }

    void addGroup(const DRW_Group& value) override {
        groups.push_back(value);
    }

    /// Point the writer at data that was just read, for a round trip.
    void attach(dx_data* data) {
        cData = data;
        currentBlock = data != nullptr ? data->mBlock : nullptr;
    }
};

std::filesystem::path fixturePath(const char* name) {
#ifdef LIBDXFRW_DXF_FIXTURE_DIR
    return std::filesystem::path(LIBDXFRW_DXF_FIXTURE_DIR) / name;
#else
    return std::filesystem::path("tests/fixtures/dxf") / name;
#endif
}

bool importFixture(const char* name, FixtureInterface& interface_,
                   dx_data& data) {
    return interface_.fileImport(fixturePath(name).string(), &data, false);
}

bool importTemporaryDxf(const char* stem, const std::string& contents,
                        FixtureInterface& interface_, dx_data& data) {
    const std::filesystem::path temporary =
        std::filesystem::temp_directory_path()
        / (std::string("libdxfrw-") + stem + ".dxf");
    std::error_code error;
    std::filesystem::remove(temporary, error);
    {
        std::ofstream output(temporary, std::ios::binary);
        if (!output)
            return false;
        output << contents;
        if (!output)
            return false;
    }
    const bool imported = interface_.fileImport(temporary.string(), &data, false);
    std::filesystem::remove(temporary, error);
    return imported;
}

bool writeHiddenZeroHeightAttribDxf(const std::filesystem::path& path,
                                    bool binary) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream)
        return false;
    if (binary)
        stream.write("AutoCAD Binary DXF\r\n\x1a\0", 22);
    std::unique_ptr<dxfWriter> writer = binary
        ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&stream))
        : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&stream));
    writer->setVersion("AC1021", true);
    const auto string = [&writer](int code, const char* value) {
        return writer->writeString(code, value);
    };
    const auto integer = [&writer](int code, int value) {
        return writer->writeInt16(code, value);
    };
    const auto real = [&writer](int code, double value) {
        return writer->writeDouble(code, value);
    };

    const bool written = string(0, "SECTION") && string(2, "HEADER")
        && string(9, "$ACADVER") && string(1, "AC1021")
        && string(0, "ENDSEC")
        && string(0, "SECTION") && string(2, "ENTITIES")
        && string(0, "INSERT") && string(100, "AcDbEntity")
        && string(8, "0") && string(100, "AcDbBlockReference")
        && string(2, "LOCAL_ATTRIB_OWNER")
        && real(10, 1.0) && real(20, 2.0) && real(30, 3.0)
        && integer(66, 1)
        && string(0, "ATTRIB") && string(100, "AcDbEntity")
        && string(8, "0") && string(100, "AcDbText")
        && real(10, 4.0) && real(20, 5.0) && real(30, 6.0)
        && real(40, 0.0) && string(1, "HIDDEN_ZERO_VALUE")
        && string(7, "STANDARD") && string(100, "AcDbAttribute")
        && integer(70, 1) && string(2, "HIDDEN_ZERO_TAG")
        && string(0, "SEQEND") && string(8, "0")
        && string(0, "LINE") && string(100, "AcDbEntity")
        && string(8, "0") && string(100, "AcDbLine")
        && real(10, 10.0) && real(20, 20.0) && real(30, 30.0)
        && real(11, 40.0) && real(21, 50.0) && real(31, 60.0)
        && string(0, "ENDSEC") && string(0, "EOF");
    stream.flush();
    return written && !writer->hasWriteError() && stream.good();
}

const DRW_Layer* findNonDefaultLayer(const dx_data& data) {
    for (const DRW_Layer& layer : data.layers) {
        if (layer.name != "0")
            return &layer;
    }
    return nullptr;
}

const DRW_MText* findMText(const dx_data& data) {
    if (data.mBlock == nullptr)
        return nullptr;
    for (const DRW_Entity* entity : data.mBlock->ent) {
        if (entity != nullptr && entity->eType == DRW::MTEXT)
            return static_cast<const DRW_MText*>(entity);
    }
    return nullptr;
}

/// Every group of one record kind in a written DXF, as (code -> values).
std::vector<std::pair<int, std::string>> groupsOfRecord(
    const std::filesystem::path& path, const std::string& recordName) {
    std::vector<std::pair<int, std::string>> groups;
    std::ifstream file(path);
    std::string code;
    std::string value;
    std::string record;

    const auto trim = [](std::string& text) {
        while (!text.empty() && (text.back() == '\r' || text.back() == ' '))
            text.pop_back();
        std::size_t start = text.find_first_not_of(" \t");
        if (start == std::string::npos)
            start = text.size();
        text.erase(0, start);
    };

    while (std::getline(file, code) && std::getline(file, value)) {
        trim(code);
        trim(value);
        if (code == "0") {
            record = value;
            continue;
        }
        if (record != recordName)
            continue;
        try {
            groups.emplace_back(std::stoi(code), value);
        } catch (const std::exception&) {
            // not a numeric group code
        }
    }

    return groups;
}

bool literalMeshWireFieldsMatch(const std::filesystem::path& path,
                                bool binary) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return false;
    std::unique_ptr<dxfReader> reader;
    if (binary) {
        constexpr char signature[] = "AutoCAD Binary DXF\r\n\x1a\0";
        std::array<char, 22> actual{};
        stream.read(actual.data(), static_cast<std::streamsize>(actual.size()));
        if (!stream || !std::equal(actual.begin(), actual.end(), signature))
            return false;
        reader = std::make_unique<dxfReaderBinary>(&stream);
    } else {
        reader = std::make_unique<dxfReaderAscii>(&stream);
    }

    const std::vector<std::pair<int, double>> expected{
        {71, 2.0}, {72, 1.0}, {94, 2.0}, {95, 2.0},
        {140, 0.25}, {140, 0.75}};
    std::vector<std::pair<int, double>> actualFields;
    bool inMesh = false;
    bool sawMesh = false;
    bool sawFollowingLine = false;
    bool sawEof = false;
    int code = 0;
    while (reader->readRec(&code)) {
        if (code == 0) {
            const std::string name = reader->getString();
            if (inMesh && name == "LINE")
                sawFollowingLine = true;
            if (name == "EOF") {
                sawEof = true;
                break;
            }
            inMesh = name == "MESH";
            sawMesh = sawMesh || inMesh;
            continue;
        }
        if (!inMesh)
            continue;
        if (code == 71 || code == 72 || code == 94 || code == 95)
            actualFields.emplace_back(code, reader->getInt32());
        else if (code == 140)
            actualFields.emplace_back(code, reader->getDouble());
    }
    return sawEof && sawMesh && sawFollowingLine
        && actualFields == expected;
}

struct DxfTableSnapshot {
    int declaredCapacity {-1};
    std::vector<std::string> rowKinds;
    std::vector<std::string> rowNames;
    bool ended {false};
};

bool readDxfTableSnapshot(const std::filesystem::path& path, bool binary,
                          const std::string& wanted,
                          DxfTableSnapshot& snapshot) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return false;
    std::unique_ptr<dxfReader> reader;
    if (binary) {
        constexpr char signature[] = "AutoCAD Binary DXF\r\n\x1a\0";
        std::array<char, 22> actual{};
        stream.read(actual.data(), static_cast<std::streamsize>(actual.size()));
        if (!stream || !std::equal(actual.begin(), actual.end(), signature))
            return false;
        reader = std::make_unique<dxfReaderBinary>(&stream);
    } else {
        reader = std::make_unique<dxfReaderAscii>(&stream);
    }

    bool inTableHead = false;
    bool inTableRows = false;
    bool matchesWanted = false;
    bool captureRowName = false;
    int code = 0;
    while (reader->readRec(&code)) {
        if (code == 0) {
            const std::string name = reader->getString();
            if (name == "EOF")
                break;
            if (name == "TABLE") {
                inTableHead = true;
                inTableRows = false;
                matchesWanted = false;
                captureRowName = false;
                continue;
            }
            if (inTableHead) {
                inTableHead = false;
                inTableRows = true;
                if (matchesWanted && name == "ENDTAB") {
                    snapshot.ended = true;
                    return true;
                }
                if (matchesWanted) {
                    snapshot.rowKinds.push_back(name);
                    captureRowName = true;
                }
                continue;
            }
            if (inTableRows && matchesWanted) {
                if (name == "ENDTAB") {
                    snapshot.ended = true;
                    return true;
                }
                snapshot.rowKinds.push_back(name);
                captureRowName = true;
            }
            continue;
        }
        if (inTableHead && code == 2)
            matchesWanted = reader->getString() == wanted;
        else if (inTableHead && code == 70 && matchesWanted)
            snapshot.declaredCapacity = reader->getInt32();
        else if (inTableRows && matchesWanted
                 && captureRowName && code == 2)
            snapshot.rowNames.push_back(reader->getString());
    }
    return false;
}

std::string groupValue(const std::vector<std::pair<int, std::string>>& groups,
                       int code);

bool hasGroup(const std::vector<std::pair<int, std::string>>& groups, int code) {
    for (const auto& group : groups) {
        if (group.first == code)
            return true;
    }
    return false;
}

// Generate from public models, rather than checking in a drawing. The same
// sentinel is available to independent readers through main's generator mode.
bool generateTableOwnerDxf(const std::filesystem::path& output,
                           DRW::Version version, bool binary) {
    class TableOwnerInterface final : public dx_iface {
    public:
        void writeViews() override {
            DRW_View view;
            view.name = "OWNER_VIEW";
            dxfW->writeView(&view);
        }
        void writeUCSs() override {
            DRW_UCS ucs;
            ucs.name = "OWNER_UCS";
            dxfW->writeUCS(&ucs);
        }
    } interface_;
    dx_data data;
    DRW_LType lineType;
    lineType.name = "OWNER_LTYPE";
    data.lineTypes.push_back(lineType);
    DRW_Layer layer;
    layer.name = "OWNER_LAYER";
    data.layers.push_back(layer);
    DRW_Textstyle style;
    style.name = "OWNER_STYLE";
    style.parentHandle = 0xDEADu; // The emitted table, not a source owner, wins.
    style.reactorHandles.push_back(0x12u); // Distinct from the STYLE owner.
    data.textStyles.push_back(style);
    DRW_Vport active;
    active.name = "*ACTIVE";
    data.VPorts.push_back(active);
    DRW_Vport viewport;
    viewport.name = "OWNER_VPORT";
    viewport.parentHandle = 0xDEADu;
    data.VPorts.push_back(viewport);
    DRW_Dimstyle dimstyle;
    dimstyle.name = "OWNER_DIMSTYLE";
    data.dimStyles.push_back(dimstyle);
    DRW_AppId appId;
    appId.name = "OWNER_APPID";
    data.appIds.push_back(appId);
    auto block = std::make_unique<dx_ifaceBlock>();
    block->name = "OWNER_BLOCK";
    data.blocks.push_back(block.release());
    return interface_.fileExport(output.string(), version, binary, &data,
                                 false);
}

bool generateMTextOrientationDxf(const std::filesystem::path& path, bool binary,
                                 bool redundantScope = false) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (binary) stream.write("AutoCAD Binary DXF\r\n\032\0", 22);
    std::unique_ptr<dxfWriter> tags;
    if (binary) tags = std::make_unique<dxfWriterBinary>(&stream);
    else tags = std::make_unique<dxfWriterAscii>(&stream);
    tags->setVersion("AC1032", true);
    const auto point = [&](int code, DRW_Coord p) {
        tags->writeDouble(code, p.x); tags->writeDouble(code + 10, p.y);
        tags->writeDouble(code + 20, p.z);
    };
    tags->writeString(0, "SECTION"); tags->writeString(2, "HEADER");
    tags->writeString(9, "$ACADVER"); tags->writeString(1, "AC1032");
    tags->writeString(0, "ENDSEC"); tags->writeString(0, "SECTION");
    tags->writeString(2, "ENTITIES");
    for (int id = 0; id != 8; ++id) {
        const int order = id % 4;
        const DRW_Coord direction = id < 4 ? DRW_Coord{0, 1, 0} : DRW_Coord{0, -.8, .6};
        tags->writeString(0, "MTEXT"); tags->writeString(5, std::to_string(1000 + id));
        tags->writeString(100, "AcDbEntity"); tags->writeString(8, "0");
        tags->writeString(100, "AcDbMText"); point(10, {1, 2, 3});
        tags->writeDouble(40, 2); tags->writeDouble(41, 10);
        tags->writeInt16(71, 1); tags->writeInt16(72, 1);
        tags->writeString(1, "ORIENT_" + std::to_string(id));
        if (id >= 4) point(210, {0, .6, .8});
        if (order == 1 || order == 2) point(11, direction);
        if (order != 1) tags->writeDouble(50, 30);
        if (order == 3) point(11, direction);
        if (redundantScope) {
            tags->writeString(101, "Embedded Object");
            // These are redundant column-context values, not main orientation.
            point(11, {1, 0, 0}); tags->writeDouble(50, 2);
            tags->writeDouble(50, 18); tags->writeDouble(50, 24);
        }
    }
    tags->writeString(0, "LINE"); tags->writeString(5, "2000");
    tags->writeString(100, "AcDbEntity"); tags->writeString(8, "0");
    tags->writeString(100, "AcDbLine"); point(10, {71, 72, 73}); point(11, {74, 75, 76});
    tags->writeString(0, "ENDSEC"); tags->writeString(0, "EOF");
    return !tags->hasWriteError() && stream.good();
}

bool mtextOrientationMatches(const dx_data& data, bool edited = false) {
    if (data.mBlock->ent.size() != 9u) return false;
    int id = 0;
    for (const auto* entity : data.mBlock->ent) {
        if (id == 8) {
            const auto* line = dynamic_cast<const DRW_Line*>(entity);
            return line && line->basePoint.x == 71 && line->secPoint.z == 76;
        }
        const auto* text = dynamic_cast<const DRW_MText*>(entity);
        const int order = id % 4;
        double expected = order == 0 || order == 2 ? 30 : id < 4 ? 90 : -90;
        if (edited && id == 1) expected = 45;  // angle-only edit wins over retained vector
        if (edited && id == 2) expected = 0;   // vector-only edit wins over retained angle
        if (edited && id == 3) expected = 45;  // simultaneous edits use vector priority
        if (!text || text->text != "ORIENT_" + std::to_string(id)
            || std::abs(text->angle - expected) > 1e-10
            || text->basePoint.x != 1 || text->height != 2) return false;
        if (!edited && (order == 1 || order == 3)) {
            const DRW_Coord expectedDirection = id < 4 ? DRW_Coord{0, 1, 0} : DRW_Coord{0, -.8, .6};
            if (text->secPoint.x != expectedDirection.x || text->secPoint.y != expectedDirection.y
                || text->secPoint.z != expectedDirection.z) return false;
        }
        ++id;
    }
    return false;
}

bool verifyMTextOrientationDxf(const std::filesystem::path& path, bool edited = false) {
    dx_data data;
    dx_iface reader;
    return reader.fileImport(path.string(), &data, false) && mtextOrientationMatches(data, edited);
}

std::vector<DRW_MText*> embeddedOrientationTexts(dx_data& data) {
    std::vector<DRW_MText*> result;
    for (auto* entity : data.mBlock->ent) {
        if (auto* insert = dynamic_cast<DRW_Insert*>(entity)) {
            for (auto& attribute : insert->attlist)
                if (attribute && attribute->mtext) result.push_back(attribute->mtext.get());
        } else if (auto* attribute = dynamic_cast<DRW_Attdef*>(entity)) {
            if (attribute->mtext) result.push_back(attribute->mtext.get());
        }
    }
    return result;
}

// Exercise the core ATTDEF callback/writer explicitly: the CLI's absent
// ATTDEF cache/emission route is separate7F work, not this orientation fix.
class EmbeddedOrientationInterface final : public dx_iface {
public:
    void addAttDef(const DRW_Attdef& value) override {
        currentBlock->ent.push_back(new DRW_Attdef(value));
    }
    void writeEntities() override {
        for (auto* entity : cData->mBlock->ent) {
            if (auto* attribute = dynamic_cast<DRW_Attdef*>(entity))
                dxfW->writeAttdef(attribute);
            else writeEntity(entity);
        }
    }
};

void testEmbeddedMTextOrientation(TestContext& t, bool binary,
                                  const std::filesystem::path& output) {
    std::string source = "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1032\n0\nENDSEC\n"
        "0\nSECTION\n2\nBLOCKS\n0\nBLOCK\n5\n3000\n100\nAcDbEntity\n8\n0\n"
        "100\nAcDbBlockBegin\n2\nORIENT_BLOCK\n70\n0\n10\n0\n20\n0\n30\n0\n"
        "3\nORIENT_BLOCK\n0\nENDBLK\n5\n3001\n100\nAcDbEntity\n8\n0\n"
        "100\nAcDbBlockEnd\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
        "0\nINSERT\n5\n3002\n100\nAcDbEntity\n8\n0\n100\nAcDbBlockReference\n"
        "66\n1\n2\nORIENT_BLOCK\n10\n0\n20\n0\n30\n0\n";
    for (int id = 0; id != 8; ++id) {
        if (id == 4) source += "0\nSEQEND\n5\n3003\n100\nAcDbEntity\n8\n0\n";
        source += std::string("0\n") + (id < 4 ? "ATTRIB" : "ATTDEF")
            + "\n5\n" + std::to_string(4000 + id)
            + "\n100\nAcDbEntity\n8\n0\n100\nAcDbText\n10\n4\n20\n5\n30\n6\n"
              "40\n2\n1\nfallback\n100\nEmbedded Object\n10\n1\n20\n2\n30\n3\n"
              "40\n2\n41\n10\n71\n1\n72\n1\n1\nEMBED_" + std::to_string(id) + "\n";
        const int order = id % 4;
        if (order == 1 || order == 2) source += "11\n0\n21\n1\n31\n0\n";
        if (order != 1) source += "50\n30\n";
        if (order == 3) source += "11\n0\n21\n1\n31\n0\n";
        source += std::string("100\n") + (id < 4 ? "AcDbAttribute" : "AcDbAttributeDefinition")
            + "\n2\nTAG" + std::to_string(id) + "\n70\n0\n";
        if (id >= 4) source += "3\nprompt\n";
    }
    source += "0\nENDSEC\n0\nEOF\n";
    EmbeddedOrientationInterface reader;
    dx_data data;
    const auto input = std::filesystem::path(output.string() + "-embedded-input.dxf");
    { std::ofstream file(input, std::ios::binary); file << source; }
    const bool imported = reader.fileImport(input.string(), &data, false);
    std::error_code error;
    std::filesystem::remove(input, error);
    const auto texts = embeddedOrientationTexts(data);
    bool sourceValid = imported && texts.size() == 8u;
    int id = 0;
    for (const auto* text : texts) {
        const int order = id++ % 4;
        sourceValid = text->angle == (order == 0 || order == 2 ? 30 : 90) && sourceValid;
    }
    t.expect(sourceValid, "legacy embedded ATTRIB/ATTDEF routes finalize their own last-wins orientation");
    EmbeddedOrientationInterface writer;
    const bool written = writer.fileExport(output.string(), DRW::AC1032, binary, &data, false);
    dx_data decoded;
    EmbeddedOrientationInterface outputReader;
    bool retained = written && outputReader.fileImport(output.string(), &decoded, false);
    const auto decodedTexts = embeddedOrientationTexts(decoded);
    retained = decodedTexts.size() == 8u && retained;
    id = 0;
    for (const auto* text : decodedTexts) {
        const int order = id++ % 4;
        retained = text->angle == (order == 0 || order == 2 ? 30 : 90) && retained;
    }
    t.expect(retained, "embedded MTEXT writer shares parsed active-orientation selection");
    for (auto* text : texts) {
        *text = DRW_MText{};
        text->text = "FRESH_EMBEDDED"; text->height = 2;
        text->angle = 30; text->secPoint = {0, 1, 0};
    }
    EmbeddedOrientationInterface freshWriter;
    const bool freshWritten = freshWriter.fileExport(output.string(), DRW::AC1032, binary, &data, false);
    dx_data freshDecoded;
    EmbeddedOrientationInterface freshReader;
    bool freshValid = freshWritten && freshReader.fileImport(output.string(), &freshDecoded, false);
    const auto freshTexts = embeddedOrientationTexts(freshDecoded);
    freshValid = freshTexts.size() == 8u && freshValid;
    for (const auto* text : freshTexts) freshValid = text->angle == 30 && freshValid;
    for (const auto* text : texts) freshValid = text->angle == 30 && text->secPoint.y == 1 && freshValid;
    t.expect(freshValid, "fresh embedded API objects retain legacy vector-before-angle ordering without mutation");
}

void testMTextOrientationPrecedence(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    for (bool binary : {false, true}) {
        const auto stem = std::filesystem::temp_directory_path()
            / ("libdxfrw-mtext-orientation-" + std::to_string(nonce)
               + (binary ? "-binary" : "-ascii"));
        const auto source = std::filesystem::path(stem.string() + "-source.dxf");
        const auto output = std::filesystem::path(stem.string() + "-output.dxf");
        t.expect(generateMTextOrientationDxf(source, binary)
                     && verifyMTextOrientationDxf(source),
                 "literal MTEXT four orientation orders retain public angle and tilted WCS vector");
        dx_data data;
        dx_iface reader;
        const bool read = reader.fileImport(source.string(), &data, false);
        std::vector<std::pair<double, DRW_Coord>> before;
        for (const auto* entity : data.mBlock->ent)
            if (const auto* text = dynamic_cast<const DRW_MText*>(entity))
                before.emplace_back(text->angle, text->secPoint);
        dx_iface writer;
        t.expect(read && writer.fileExport(output.string(), DRW::AC1032, binary, &data, false)
                     && verifyMTextOrientationDxf(output),
                 "MTEXT API export preserves parsed last-wins orientation");
        std::size_t id = 0;
        bool unchanged = true;
        for (const auto* entity : data.mBlock->ent) {
            if (const auto* text = dynamic_cast<const DRW_MText*>(entity)) {
                const auto& snapshot = before.at(id++);
                unchanged = text->angle == snapshot.first && text->secPoint.x == snapshot.second.x
                    && text->secPoint.y == snapshot.second.y && text->secPoint.z == snapshot.second.z && unchanged;
            }
        }
        t.expect(unchanged, "MTEXT orientation selection never mutates caller geometry");
        dx_data copies;
        for (const auto* entity : data.mBlock->ent) {
            if (const auto* text = dynamic_cast<const DRW_MText*>(entity)) {
                DRW_MText copied(*text);
                DRW_MText moved(std::move(copied));
                DRW_MText assigned;
                assigned = moved;
                DRW_MText moveAssigned;
                moveAssigned = std::move(assigned);
                copies.mBlock->ent.push_back(new DRW_MText(moveAssigned));
            } else if (const auto* line = dynamic_cast<const DRW_Line*>(entity)) {
                copies.mBlock->ent.push_back(new DRW_Line(*line));
            }
        }
        dx_iface copyWriter;
        t.expect(copyWriter.fileExport(output.string(), DRW::AC1032, binary, &copies, false)
                     && verifyMTextOrientationDxf(output),
                 "MTEXT copy/move/assignment retain parsed orientation provenance");
        id = 0;
        for (auto* entity : data.mBlock->ent) {
            if (auto* text = dynamic_cast<DRW_MText*>(entity)) {
                if (id == 1) text->angle = 45;
                if (id == 2) text->secPoint = {1, 0, 0};
                if (id == 3) { text->angle = 60; text->secPoint = {1, 1, 0}; }
                ++id;
            }
        }
        dx_iface editWriter;
        t.expect(editWriter.fileExport(output.string(), DRW::AC1032, binary, &data, false)
                     && verifyMTextOrientationDxf(output, true),
                 "MTEXT read-then-edit angle/vector/both selection is deterministic");
        t.expect(generateMTextOrientationDxf(source, binary, true)
                     && verifyMTextOrientationDxf(source),
                 "redundant column scope direction/height50 cannot change main MTEXT orientation");
        dx_data fresh;
        for (int kind = 0; kind != 3; ++kind) {
            auto* text = new DRW_MText();
            text->text = "FRESH_" + std::to_string(kind); text->height = 2;
            if (kind != 1) text->angle = 30;
            if (kind != 0) text->secPoint = {0, 1, 0};
            fresh.mBlock->ent.push_back(text);
        }
        dx_iface freshWriter;
        const bool freshWritten = freshWriter.fileExport(output.string(), DRW::AC1032, binary, &fresh, false);
        dx_data freshDecoded;
        dx_iface freshReader;
        bool freshValid = freshWritten && freshReader.fileImport(output.string(), &freshDecoded, false)
            && freshDecoded.mBlock->ent.size() == 3u;
        int freshId = 0;
        for (const auto* entity : freshDecoded.mBlock->ent) {
            const auto* text = dynamic_cast<const DRW_MText*>(entity);
            freshValid = text && text->angle == (freshId++ == 0 ? 30 : 90) && freshValid;
        }
        freshId = 0;
        for (const auto* entity : fresh.mBlock->ent) {
            const auto* text = static_cast<const DRW_MText*>(entity);
            freshValid = text->angle == (freshId++ == 1 ? 0 : 30) && freshValid;
        }
        t.expect(freshValid, "fresh API MTEXT angle/vector/conflicting fields retain legacy behavior without mutation");
        testEmbeddedMTextOrientation(t, binary, output);
        std::error_code error;
        std::filesystem::remove(source, error); std::filesystem::remove(output, error);
    }
}

const std::vector<int>& expectedClassicVertexIdentifiers() {
    static const std::vector<int> values {
        0, 70001, std::numeric_limits<std::int32_t>::max(),
        80001, 80002, 0, 90001, 90002, 90003, 90004,
        100001, 100002, 100003, 100010, 100011};
    return values;
}

// Literal modern DXF controls, independent of writePolyline's emission rules.
bool generateClassicVertexIdentifiers(const std::filesystem::path& path, bool binary) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (binary) stream.write("AutoCAD Binary DXF\r\n\032\0", 22);
    std::unique_ptr<dxfWriter> tags;
    if (binary) tags = std::make_unique<dxfWriterBinary>(&stream);
    else tags = std::make_unique<dxfWriterAscii>(&stream);
    tags->setVersion("AC1024", true);
    const auto point = [&](int code, DRW_Coord value) {
        tags->writeDouble(code, value.x); tags->writeDouble(code + 10, value.y);
        tags->writeDouble(code + 20, value.z);
    };
    tags->writeString(0, "SECTION"); tags->writeString(2, "HEADER");
    tags->writeString(9, "$ACADVER"); tags->writeString(1, "AC1024");
    tags->writeString(0, "ENDSEC"); tags->writeString(0, "SECTION");
    tags->writeString(2, "ENTITIES");
    std::size_t identifierIndex = 0;
    for (int family : {0, 8, 16, 64}) {
        tags->writeString(0, "POLYLINE"); tags->writeString(100, "AcDbEntity");
        tags->writeString(8, "0");
        tags->writeString(100, family == 0 ? "AcDb2dPolyline" : family == 8
            ? "AcDb3dPolyline" : family == 16 ? "AcDbPolygonMesh" : "AcDbPolyFaceMesh");
        point(10, {0, 0, family == 0 ? 3.0 : 0.0}); tags->writeInt16(70, family);
        if (family == 0) point(210, {0, .6, .8});
        if (family == 16) { tags->writeInt16(71, 2); tags->writeInt16(72, 2); }
        if (family == 64) { tags->writeInt16(71, 3); tags->writeInt16(72, 2); }
        const int count = family == 16 ? 4 : family == 64 ? 5 : 3;
        for (int index = 0; index != count; ++index) {
            const bool face = family == 64 && index >= 3;
            tags->writeString(0, "VERTEX"); tags->writeString(100, "AcDbEntity");
            tags->writeString(8, "0");
            if (!face) tags->writeString(100, "AcDbVertex");
            tags->writeString(100, face ? "AcDbFaceRecord" : family == 0
                ? "AcDb2dVertex" : family == 8 ? "AcDb3dPolylineVertex"
                : family == 16 ? "AcDbPolygonMeshVertex" : "AcDbPolyFaceMeshVertex");
            point(10, face ? DRW_Coord{} : DRW_Coord{index + 1.0, index == 1 ? 4.0 : index + 2.0,
                  family == 0 ? 0.0 : index + 3.0});
            tags->writeInt16(70, face ? 128 : family == 0 ? 0 : family == 8 ? 32
                                            : family == 16 ? 64 : 192);
            if (face) {
                tags->writeInt16(71, index == 3 ? 1 : 2);
                tags->writeInt16(72, index == 3 ? -2 : 0);
                tags->writeInt16(73, index == 3 ? 3 : 123); // inactive after first zero
            }
            const int identifier = expectedClassicVertexIdentifiers()[identifierIndex++];
            if (identifier != 0) tags->writeInt32(91, identifier);
        }
        tags->writeString(0, "SEQEND"); tags->writeString(8, "0");
    }
    tags->writeString(0, "LINE"); tags->writeString(100, "AcDbEntity");
    tags->writeString(8, "0"); tags->writeString(100, "AcDbLine");
    point(10, {71, 72, 73}); point(11, {74, 75, 76});
    tags->writeString(0, "ENDSEC"); tags->writeString(0, "EOF");
    return !tags->hasWriteError() && stream.good();
}

std::vector<int> classicVertexIdentifiers(const dx_data& data) {
    std::vector<int> identifiers;
    for (const auto* entity : data.mBlock->ent)
        if (const auto* polyline = dynamic_cast<const DRW_Polyline*>(entity))
            for (const auto& vertex : polyline->vertlist)
                identifiers.push_back(vertex->identifier);
    return identifiers;
}

bool verifyClassicVertexIdentifiers(const std::filesystem::path& path) {
    dx_data data; dx_iface reader;
    if (!reader.fileImport(path.string(), &data, false)
        || data.mBlock->ent.size() != 5u
        || classicVertexIdentifiers(data) != expectedClassicVertexIdentifiers()) return false;
    const std::vector<DRW_Entity*> entities(data.mBlock->ent.begin(), data.mBlock->ent.end());
    const auto* planar = dynamic_cast<const DRW_Polyline*>(entities[0]);
    const auto* spatial = dynamic_cast<const DRW_Polyline*>(entities[1]);
    const auto* mesh = dynamic_cast<const DRW_Polyline*>(entities[2]);
    const auto* polyface = dynamic_cast<const DRW_Polyline*>(entities[3]);
    return planar && planar->flags == 0 && planar->basePoint.z == 3
        && planar->extPoint.y == .6 && planar->extPoint.z == .8
        && spatial && spatial->flags == 8 && spatial->vertlist[2]->basePoint.z == 5
        && mesh && mesh->flags == 16 && mesh->vertexcount == 2 && mesh->facecount == 2
        && polyface && polyface->flags == 64 && polyface->vertlist.size() == 5u
        && polyface->vertlist[3]->vindex1 == 1 && polyface->vertlist[3]->vindex2 == -2
        && polyface->vertlist[3]->vindex3 == 3 && polyface->vertlist[3]->vindex4 == 0
        && polyface->vertlist[4]->vindex1 == 2 && polyface->vertlist[4]->vindex2 == 0
        && data.mBlock->ent.back()->eType == DRW::LINE;
}

bool literalClassicIdentifiersMatch(const std::filesystem::path& path, bool binary,
                                    const std::vector<int>& expected,
                                    DRW::Version version = DRW::AC1024) {
    std::ifstream stream(path, std::ios::binary);
    if (binary) stream.seekg(22);
    std::unique_ptr<dxfReader> tags;
    if (binary && version <= DRW::AC1009) tags = std::make_unique<dxfReaderBinaryR12>(&stream);
    else if (binary) tags = std::make_unique<dxfReaderBinary>(&stream);
    else tags = std::make_unique<dxfReaderAscii>(&stream);
    std::vector<int> identifiers;
    bool vertex = false;
    int identifier = 0, code = 0;
    while (tags->readRec(&code)) {
        if (code == 0) {
            if (vertex) identifiers.push_back(identifier);
            vertex = tags->getString() == "VERTEX";
            identifier = 0;
            if (tags->getString() == "EOF") return identifiers == expected;
        } else if (vertex && code == 91) identifier = tags->getInt32();
    }
    return false;
}

void testClassicVertexIdentifierRetention(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    for (bool binary : {false, true}) {
        const auto source = std::filesystem::temp_directory_path()
            / ("libdxfrw-vertex-ids-" + std::to_string(nonce) + (binary ? "-bin.dxf" : "-asc.dxf"));
        const auto output = source.parent_path() / (source.stem().string() + "-out.dxf");
        t.expect(generateClassicVertexIdentifiers(source, binary)
                     && verifyClassicVertexIdentifiers(source),
                 "literal classic VERTEX identifiers and geometry are parsed");
        dx_data data; dx_iface reader, writer;
        t.expect(reader.fileImport(source.string(), &data, false), "classic ID input imports");
        for (auto version : {DRW::AC1024, DRW::AC1032}) {
            t.expect(writer.fileExport(output.string(), version, binary, &data, false)
                         && verifyClassicVertexIdentifiers(output)
                         && literalClassicIdentifiersMatch(output, binary, expectedClassicVertexIdentifiers()),
                     "modern classic IDs survive public and literal read-back with full I32 width");
            t.expect(classicVertexIdentifiers(data) == expectedClassicVertexIdentifiers(),
                     "identifier serialization does not mutate caller IDs");
        }
        const auto* sourcePolyface = static_cast<const DRW_Polyline*>(*std::next(data.mBlock->ent.begin(), 3));
        t.expect(sourcePolyface->vertlist[4]->vindex3 == 123,
                 "identifier emission preserves inactive caller face indices");
        dx_data copied;
        for (const auto* entity : data.mBlock->ent)
            if (const auto* polyline = dynamic_cast<const DRW_Polyline*>(entity))
                copied.mBlock->ent.push_back(new DRW_Polyline(*polyline));
        auto* edited = static_cast<DRW_Polyline*>(copied.mBlock->ent.front());
        edited->vertlist[1]->identifier = 80002;
        auto expected = expectedClassicVertexIdentifiers(); expected[1] = 80002;
        t.expect(writer.fileExport(output.string(), DRW::AC1024, binary, &copied, false)
                     && literalClassicIdentifiersMatch(output, binary, expected)
                     && classicVertexIdentifiers(data) == expectedClassicVertexIdentifiers(),
                 "copied/read-edited IDs serialize without modifying original children");
        for (auto older : {DRW::AC1009, DRW::AC1014, DRW::AC1021}) {
            { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "unchanged"; }
            t.expect(!writer.fileExport(output.string(), older, binary, &data, false),
                     "unqualified older target explicitly rejects nonzero modern VERTEX IDs");
            std::ifstream preserved(output, std::ios::binary);
            t.expect(std::string(std::istreambuf_iterator<char>(preserved), {}) == "unchanged"
                         && classicVertexIdentifiers(data) == expectedClassicVertexIdentifiers(),
                     "older-target identifier rejection preserves destination and caller IDs");
        }
        for (auto* entity : copied.mBlock->ent)
            for (const auto& vertex : static_cast<DRW_Polyline*>(entity)->vertlist) vertex->identifier = 0;
        t.expect(writer.fileExport(output.string(), DRW::AC1009, binary, &copied, false)
                     && literalClassicIdentifiersMatch(output, binary, std::vector<int>(expected.size(), 0), DRW::AC1009),
                 "default-ID older geometry remains supported after rejected export");
        std::error_code error;
        std::filesystem::remove(source, error); std::filesystem::remove(output, error);
    }
}

// Component omission controls exercise the existing reader policy, not a
// claim that malformed coordinate children are valid interoperable DXF.
bool generateClassicVertexPresence(const std::filesystem::path& path, bool binary,
                                   int family, int missingIndex = -1,
                                   int missingCode = 0, bool faceFirst = false,
                                   int faceLocation = 0) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (binary) stream.write("AutoCAD Binary DXF\r\n\032\0", 22);
    std::unique_ptr<dxfWriter> tags;
    if (binary) tags = std::make_unique<dxfWriterBinary>(&stream);
    else tags = std::make_unique<dxfWriterAscii>(&stream);
    tags->setVersion("AC1024", true);
    const auto point = [&](int code, DRW_Coord value) {
        tags->writeDouble(code, value.x); tags->writeDouble(code + 10, value.y);
        tags->writeDouble(code + 20, value.z);
    };
    const auto line = [&](double value) {
        tags->writeString(0, "LINE"); tags->writeString(100, "AcDbEntity");
        tags->writeString(8, "0"); tags->writeString(100, "AcDbLine");
        point(10, {value, 2, 3}); point(11, {value + 1, 4, 5});
    };
    tags->writeString(0, "SECTION"); tags->writeString(2, "HEADER");
    tags->writeString(9, "$ACADVER"); tags->writeString(1, "AC1024");
    tags->writeString(0, "ENDSEC"); tags->writeString(0, "SECTION");
    tags->writeString(2, "ENTITIES"); line(101);
    tags->writeString(0, "POLYLINE"); tags->writeString(100, "AcDbEntity");
    tags->writeString(8, "0"); tags->writeString(100, family == 0 ? "AcDb2dPolyline"
        : family == 8 ? "AcDb3dPolyline" : family == 16 ? "AcDbPolygonMesh" : "AcDbPolyFaceMesh");
    point(10, {}); tags->writeInt16(70, family);
    if (family == 16) { tags->writeInt16(71, 1); tags->writeInt16(72, 3); }
    if (family == 64) { tags->writeInt16(71, 3); tags->writeInt16(72, 1); }
    int coordinate = 0;
    for (int record = 0; record != (family == 64 ? 4 : 3); ++record) {
        const bool face = family == 64 && record == (faceFirst ? 0 : 3);
        tags->writeString(0, "VERTEX"); tags->writeString(100, "AcDbEntity");
        tags->writeString(8, "0");
        if (!face) tags->writeString(100, "AcDbVertex");
        tags->writeString(100, face ? "AcDbFaceRecord" : family == 0 ? "AcDb2dVertex"
            : family == 8 ? "AcDb3dPolylineVertex" : family == 16
            ? "AcDbPolygonMeshVertex" : "AcDbPolyFaceMeshVertex");
        if (face) {
            if (faceLocation == 0) point(10, {});
            else if (faceLocation == 2) tags->writeDouble(10, 999); // ignored partial location
            tags->writeInt16(70, 128); tags->writeInt16(71, 1);
            tags->writeInt16(72, -2); tags->writeInt16(73, 3);
        } else {
            for (int code : {10, 20, 30})
                if (coordinate != missingIndex || code != missingCode)
                    tags->writeDouble(code, coordinate + (code == 10 ? 1.0 : code == 20 ? 2.0 : 3.0));
            tags->writeInt16(70, family == 0 ? 0 : family == 8 ? 32 : family == 16 ? 64 : 192);
            ++coordinate;
        }
    }
    tags->writeString(0, "SEQEND"); tags->writeString(8, "0"); line(201);
    tags->writeString(0, "ENDSEC"); tags->writeString(0, "EOF");
    return !tags->hasWriteError() && stream.good();
}

bool classicPresenceMatches(const dx_data& data, int family, int optionalZ = -1) {
    if (data.mBlock->ent.size() != 3u) return false;
    const auto* first = dynamic_cast<const DRW_Line*>(data.mBlock->ent.front());
    const auto* last = dynamic_cast<const DRW_Line*>(data.mBlock->ent.back());
    const auto* polyline = dynamic_cast<const DRW_Polyline*>(*std::next(data.mBlock->ent.begin()));
    if (!first || !last || first->basePoint.x != 101 || last->basePoint.x != 201
        || !polyline || polyline->flags != family) return false;
    int coordinate = 0, faces = 0;
    for (const auto& vertex : polyline->vertlist) {
        if (family == 64 && (vertex->flags & 192) == 128) {
            if (vertex->vindex1 != 1 || vertex->vindex2 != -2 || vertex->vindex3 != 3) return false;
            ++faces;
        } else {
            if (vertex->basePoint.x != coordinate + 1.0 || vertex->basePoint.y != coordinate + 2.0
                || vertex->basePoint.z != (coordinate == optionalZ ? 0.0 : coordinate + 3.0)) return false;
            ++coordinate;
        }
    }
    return coordinate == 3 && faces == (family == 64 ? 1 : 0);
}

bool verifyClassicVertexPresence(const std::filesystem::path& path) {
    dx_data data; dx_iface reader;
    return reader.fileImport(path.string(), &data, false) && classicPresenceMatches(data, 8);
}

void testClassicVertexPresenceBoundaries(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    for (bool binary : {false, true}) {
        const auto path = std::filesystem::temp_directory_path()
            / ("libdxfrw-vertex-presence-" + std::to_string(nonce) + (binary ? "-bin.dxf" : "-asc.dxf"));
        for (int family : {0, 8, 16, 64}) {
            for (int coordinate = 0; coordinate != 3; ++coordinate) {
                for (int code : {10, 20}) {
                    t.expect(generateClassicVertexPresence(path, binary, family, coordinate, code),
                             "generate omitted-component coordinate-child control");
                    dx_data invalid; FixtureInterface capture; capture.attach(&invalid);
                    const std::string filename = path.string(); dxfRW reader(filename.c_str());
                    t.expect(!reader.read(&capture, false) && reader.getError() == DRW::BAD_CODE_PARSED
                                 && invalid.mBlock->ent.size() == 1u
                                 && invalid.mBlock->ent.front()->eType == DRW::LINE,
                             "every first/middle/last coordinate child requires its own X/Y pair without partial parent callback");
                    dx_data complete; capture.attach(&complete);
                    t.expect(generateClassicVertexPresence(path, binary, family)
                                 && reader.read(&capture, false) && classicPresenceMatches(complete, family),
                             "same reader accepts complete sequence after omitted-component failure");
                }
                dx_data sparse; dx_iface reader;
                t.expect(generateClassicVertexPresence(path, binary, family, coordinate, 30)
                             && reader.fileImport(path.string(), &sparse, false)
                             && classicPresenceMatches(sparse, family, coordinate),
                         "existing optional-Z zero-default policy holds for each coordinate child");
            }
        }
        for (bool first : {false, true}) {
            for (int location : {0, 1, 2}) {
                dx_data data; dx_iface reader, writer;
                t.expect(generateClassicVertexPresence(path, binary, 64, -1, 0, first, location)
                             && reader.fileImport(path.string(), &data, false)
                             && classicPresenceMatches(data, 64),
                         "first/later POLYFACE faces tolerate ignored complete/absent/partial locations");
                t.expect(writer.fileExport(path.string(), DRW::AC1024, binary, &data, false),
                         "ignored face location remains canonically writable");
                dx_data canonical;
                t.expect(reader.fileImport(path.string(), &canonical, false)
                             && classicPresenceMatches(canonical, 64),
                         "canonical face output keeps signed topology and next-record boundary");
            }
        }
        std::error_code error; std::filesystem::remove(path, error);
    }
}

// Author group76 independently of the production writer, which canonicalizes
// it from the actual array. Stale values exercise documented OPEN tolerance.
bool generateLeaderAdvisoryCounts(const std::filesystem::path& path, bool binary,
                                  int limitCount = -1) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (binary) stream.write("AutoCAD Binary DXF\r\n\032\0", 22);
    std::unique_ptr<dxfWriter> tags;
    if (binary) tags = std::make_unique<dxfWriterBinary>(&stream);
    else tags = std::make_unique<dxfWriterAscii>(&stream);
    tags->setVersion("AC1024", true);
    const auto point = [&](int code, DRW_Coord value) {
        tags->writeDouble(code, value.x); tags->writeDouble(code + 10, value.y);
        tags->writeDouble(code + 20, value.z);
    };
    tags->writeString(0, "SECTION"); tags->writeString(2, "HEADER");
    tags->writeString(9, "$ACADVER"); tags->writeString(1, "AC1024");
    tags->writeString(0, "ENDSEC"); tags->writeString(0, "SECTION");
    tags->writeString(2, "ENTITIES");
    const std::array<int, 5> declared {{2, 0, 99, -1, 99}};
    for (int index = 0; index != (limitCount < 0 ? 5 : 1); ++index) {
        tags->writeString(0, "LEADER"); tags->writeString(100, "AcDbEntity");
        tags->writeString(8, "0"); tags->writeInt16(62, 2 + index);
        tags->writeString(100, "AcDbLeader"); tags->writeString(3, "STANDARD");
        tags->writeInt16(71, 1); tags->writeInt16(72, index == 4 ? 1 : 0);
        tags->writeInt16(73, 3); tags->writeInt16(74, 1); tags->writeInt16(75, 0);
        tags->writeDouble(40, 2); tags->writeDouble(41, 4);
        if (limitCount >= 0) tags->writeInt16(76, 0); // independent of actual payload limit
        else if (declared[index] >= 0) tags->writeInt16(76, declared[index]);
        if (limitCount >= 0) {
            for (int vertex = 0; vertex != limitCount; ++vertex)
                point(10, {vertex + 1.0, 2, 3});
        } else {
            point(10, {1, 2, 3}); point(10, {4, 6, 0});
            if (index == 4) point(10, {8, 9, -2.25});
        }
        tags->writeInt16(77, 7); point(210, {0, .6, .8}); point(211, {1, 0, 0});
    }
    tags->writeString(0, "LINE"); tags->writeString(100, "AcDbEntity");
    tags->writeString(8, "0"); tags->writeString(100, "AcDbLine");
    point(10, {71, 72, 73}); point(11, {74, 75, 76});
    tags->writeString(0, "ENDSEC"); tags->writeString(0, "EOF");
    return !tags->hasWriteError() && stream.good();
}

bool leaderAdvisoryCountsMatch(const dx_data& data, bool canonical) {
    if (data.mBlock->ent.size() != 6u) return false;
    const std::array<int, 5> declared {{2, 0, 99, 0, 99}};
    int index = 0;
    for (const auto* entity : data.mBlock->ent) {
        if (index == 5) {
            const auto* line = dynamic_cast<const DRW_Line*>(entity);
            return entity->eType == DRW::LINE && line && line->basePoint.x == 71
                && line->basePoint.y == 72 && line->basePoint.z == 73
                && line->secPoint.x == 74 && line->secPoint.y == 75 && line->secPoint.z == 76;
        }
        const auto* leader = dynamic_cast<const DRW_Leader*>(entity);
        const int actual = index == 4 ? 3 : 2;
        if (!leader || leader->vertexlist.size() != static_cast<std::size_t>(actual)
            || leader->vertnum != (canonical ? actual : declared[index])
            || leader->style != "STANDARD" || leader->color != 2 + index
            || leader->arrow != 1 || leader->leadertype != (index == 4 ? 1 : 0)
            || leader->flag != 3 || leader->hookline != 1 || leader->hookflag != 0
            || leader->textheight != 2 || leader->textwidth != 4 || leader->coloruse != 7
            || leader->extrusionPoint.x != 0 || leader->extrusionPoint.y != .6
            || leader->extrusionPoint.z != .8 || leader->horizdir.x != 1
            || leader->horizdir.y != 0 || leader->horizdir.z != 0) return false;
        const std::array<DRW_Coord, 3> expected {{{1, 2, 3}, {4, 6, 0}, {8, 9, -2.25}}};
        for (int vertex = 0; vertex != actual; ++vertex) {
            const auto& point = *leader->vertexlist[vertex];
            if (point.x != expected[vertex].x || point.y != expected[vertex].y
                || point.z != expected[vertex].z) return false;
        }
        ++index;
    }
    return false;
}

bool verifyLeaderAdvisoryCounts(const std::filesystem::path& path) {
    dx_data data; dx_iface reader;
    return reader.fileImport(path.string(), &data, false) && leaderAdvisoryCountsMatch(data, true);
}

bool literalLeaderCountsMatch(const std::filesystem::path& path, bool binary) {
    std::ifstream stream(path, std::ios::binary);
    if (binary) stream.seekg(22);
    std::unique_ptr<dxfReader> tags;
    if (binary) tags = std::make_unique<dxfReaderBinary>(&stream);
    else tags = std::make_unique<dxfReaderAscii>(&stream);
    std::vector<int> counts;
    bool leader = false; int code = 0;
    while (tags->readRec(&code)) {
        if (code == 0) {
            leader = tags->getString() == "LEADER";
            if (tags->getString() == "EOF") return counts == std::vector<int>{2, 2, 2, 2, 3};
        } else if (leader && code == 76) counts.push_back(tags->getInt32());
    }
    return false;
}

void testLeaderAdvisoryCounts(TestContext& t) {
    for (const std::string& body : {
            std::string("76\nnot-an-integer\n10\n1\n20\n2\n"),
            std::string("76\n2147483648\n10\n1\n20\n2\n"),
            std::string("76\n0\n10\nnan\n20\n2\n")}) {
        std::string contents = "0\nSECTION\n2\nENTITIES\n0\nLEADER\n8\n0\n3\nSTANDARD\n"
            + body + "0\nENDSEC\n0\nEOF\n";
        dx_data invalid; FixtureInterface capture; capture.attach(&invalid); dxfRW reader("");
        t.expect(!reader.readAscii(&capture, false, contents) && invalid.mBlock->ent.empty(),
                 "advisory LEADER count preserves lexical/width and finite-coordinate rejection");
    }
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    for (bool binary : {false, true}) {
        const auto path = std::filesystem::temp_directory_path()
            / ("libdxfrw-leader-count-" + std::to_string(nonce) + (binary ? "-bin.dxf" : "-asc.dxf"));
        const auto output = path.parent_path() / (path.stem().string() + "-out.dxf");
        t.expect(generateLeaderAdvisoryCounts(path, binary), "generate literal advisory LEADER counts");
        for (bool applyExt : {false, true}) {
            dx_data data; FixtureInterface capture; capture.attach(&data);
            const std::string filename = path.string(); dxfRW reader(filename.c_str());
            t.expect(reader.read(&capture, applyExt) && leaderAdvisoryCountsMatch(data, false),
                     "exact/stale/absent LEADER counts retain actual geometry under both extrusion modes");
        }
        auto data = std::make_unique<dx_data>(); dx_iface reader, writer;
        t.expect(reader.fileImport(path.string(), data.get(), false), "advisory LEADER source imports");
        for (int pass = 0; pass != 2; ++pass) {
            t.expect(writer.fileExport(output.string(), DRW::AC1024, binary, data.get(), false)
                         && verifyLeaderAdvisoryCounts(output) && literalLeaderCountsMatch(output, binary),
                     "LEADER writes canonical actual counts while retaining tested fields through repeat conversion");
            if (pass == 0) t.expect(leaderAdvisoryCountsMatch(*data, false), "canonical output does not overwrite caller declared counts");
            auto reopened = std::make_unique<dx_data>();
            t.expect(reader.fileImport(output.string(), reopened.get(), false), "canonical LEADER output reopens");
            data = std::move(reopened);
        }
        for (int count : {4999, 5000, 5001}) {
            t.expect(generateLeaderAdvisoryCounts(path, binary, count), "generate actual LEADER payload limit control");
            dx_data limited; FixtureInterface capture; capture.attach(&limited);
            const std::string filename = path.string(); dxfRW reused(filename.c_str());
            const bool imported = reused.read(&capture, false);
            if (count > 5000) {
                t.expect(!imported && reused.getError() == DRW::BAD_CODE_PARSED && limited.mBlock->ent.empty(),
                         "over-limit actual LEADER payload rejects before parent or following LINE callback");
            } else {
                const auto* leader = limited.mBlock->ent.empty() ? nullptr
                    : dynamic_cast<const DRW_Leader*>(limited.mBlock->ent.front());
                t.expect(imported && limited.mBlock->ent.size() == 2u && leader
                             && leader->vertnum == 0 && leader->vertexlist.size() == static_cast<std::size_t>(count),
                         "actual LEADER limit-minus-one/limit ignores stale declared count");
            }
            dx_data complete; capture.attach(&complete);
            t.expect(generateLeaderAdvisoryCounts(path, binary) && reused.read(&capture, false)
                         && leaderAdvisoryCountsMatch(complete, false),
                     "same reader accepts complete advisory LEADER controls after limit read/failure");
        }
        std::error_code error;
        std::filesystem::remove(path, error); std::filesystem::remove(output, error);
    }
}

struct TableOwnerRecord {
    std::string kind;
    std::vector<std::pair<int, std::string>> groups;
};

// Decode framing only: no document reader can repair, create or re-parent the
// table records before these literal graph assertions see them.
bool readTableOwnerRecords(const std::filesystem::path& path, bool binary,
                           std::vector<TableOwnerRecord>& records,
                           bool retainCoordinatePresence = false,
                           bool allowWideHandles = false,
                           bool retainDictionaryEntries = false) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return false;
    std::unique_ptr<dxfReader> reader;
    if (binary) {
        constexpr char signature[] = "AutoCAD Binary DXF\r\n\x1a";
        std::array<char, sizeof(signature)> actual{};
        stream.read(actual.data(), actual.size());
        if (!stream || !std::equal(actual.begin(), actual.end(), signature))
            return false;
        reader = std::make_unique<dxfReaderBinary>(&stream);
    } else {
        reader = std::make_unique<dxfReaderAscii>(&stream);
    }
    reader->setAllowWideHandleLexemes(allowWideHandles);
    int code = 0;
    while (reader->readRec(&code)) {
        if (code == 0) {
            const std::string kind = reader->getString();
            if (kind == "EOF")
                return true;
            records.push_back({kind, {}});
        } else if (!records.empty()) {
            if (retainCoordinatePresence && (code == 30 || code == 31)) {
                // Presence only, not a numeric coordinate projection. Opt in
                // so graph-only callers keep their original field inventory.
                records.back().groups.emplace_back(code, "present");
            } else if (code == 77 || code == 78
                       || code == 281 || code == 282 || code == 283) {
                // Literal scalar values, before a document reader can supply
                // defaults or repair an object graph.
                records.back().groups.emplace_back(
                    code, std::to_string(reader->getInt32()));
            } else if ((retainDictionaryEntries && (code == 3 || code == 350))
                || code == 1 || code == 2 || code == 5 || code == 105
                || code == 100 || code == 102 || code == 330 || code == 331 || code == 340 || code == 360
                || code == 1000 || code == 1001 || code == 1005)
                records.back().groups.emplace_back(code, reader->getString());
        }
    }
    return false;
}

void testDictionaryKeyAdmission(TestContext& t) {
    class DictionaryInterface final : public dx_iface {
    public:
        void attach(dx_data& data) { cData = &data; currentBlock = data.mBlock; }
        void writeEntities() override {
            if (withImage) {
                DRW_Image image;
                image.sizeu = image.sizev = 8;
                image.secPoint = {1, 0, 0};
                image.vVector = {0, 1, 0};
                imageWritten = dxfW->writeImage(&image, "dictionary-key-control.png") != nullptr;
            }
        }
        void writeObjects() override {
            ++objectCalls;
            before = dxfW->highWaterHandle();
            if (route == 0) written = dxfW->writeDictionary(&dictionary);
            else if (rawRoute) written = dxfW->writeRawDxfObject(&raw);
            else written = true;
            after = dxfW->highWaterHandle();
            if (!written) return;
            std::set<std::uint32_t> emitted;
            for (const auto& entry : dictionary.m_entries) {
                if (!emitted.insert(entry.m_handle).second) continue;
                DRW_Scale scale;
                scale.handle = entry.m_handle;
                scale.parentHandle = route == 2 ? 0xCu : dictionary.handle;
                scale.name = "target label " + std::to_string(entry.m_handle);
                written = dxfW->writeScale(&scale) && written;
            }
        }
        DRW_Dictionary dictionary;
        DRW_RawDxfObject raw;
        int route = 0, objectCalls = 0;
        bool written = false, withImage = false, imageWritten = true, rawRoute = false;
        std::uint32_t before = 0, after = 0;
    };
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-dictionary-keys-" + std::to_string(stamp) + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{path};
    const auto contents = [&]() {
        std::ifstream input(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input), {});
    };
    const auto entriesOf = [](const std::vector<TableOwnerRecord>& records,
                              const std::string& handle) {
        std::vector<std::pair<int, std::string>> entries;
        for (const auto& record : records) {
            if (record.kind != "DICTIONARY" || groupValue(record.groups, 5) != handle) continue;
            for (const auto& group : record.groups)
                if (group.first == 3 || group.first == 350 || group.first == 360)
                    entries.push_back(group);
        }
        return entries;
    };
    for (bool binary : {false, true}) {
        for (int route : {0, 1, 2}) {
            const int caseCount = route == 2 ? 8 : route == 0 ? 6 : 5;
            for (int kind = 0; kind != caseCount; ++kind) {
                const std::string cell = " route" + std::to_string(route) + " case" + std::to_string(kind)
                    + (binary ? " binary" : " ASCII");
                dx_data data;
                DictionaryInterface adapter;
                adapter.attach(data);
                adapter.route = route;
                adapter.dictionary.handle = 0x900u;
                adapter.dictionary.parentHandle = 0xCu;
                adapter.dictionary.hardOwner = 1;
                adapter.dictionary.m_entries = {{"Alpha", 0x901u}, {"Beta", 0x902u}};
                if (kind == 1) adapter.dictionary.m_entries[1].m_name = "Alpha";
                if (kind == 2) adapter.dictionary.m_entries[1].m_name = "aLPHa";
                if (kind == 3) adapter.dictionary.m_entries[1].m_handle = 0x901u;
                if (kind == 4) {
                    adapter.dictionary.m_entries[0].m_name = "É";
                    adapter.dictionary.m_entries[1].m_name = "é";
                }
                if (route == 2 && kind >= 5)
                    adapter.dictionary.m_entries[0].m_name = kind == 5 ? "acad_group" : "acad_image_dict";
                adapter.withImage = route == 2 && kind == 6;
                const auto original = adapter.dictionary;
                const std::string filename = path.string();
                dxfRW writer(filename.c_str());
                adapter.dxfW = &writer;
                const auto configure = [&]() {
                    std::vector<std::pair<std::string, std::string>> roots;
                    if (route == 2) {
                        for (const auto& entry : adapter.dictionary.m_entries)
                            roots.emplace_back(entry.m_name, writer.toHexStrHandle(entry.m_handle));
                    } else {
                        roots.emplace_back("DictionaryAlias", "900");
                    }
                    writer.setRootDictEntries(roots);
                    writer.setNamedDictObjects(route == 1 ? std::vector<DRW_Dictionary>{adapter.dictionary}
                                                          : std::vector<DRW_Dictionary>{});
                };
                configure();
                if (route == 0 && kind == 5)
                    writer.setHandleRemap({{0x901u, 0xA00u}, {0x902u, 0xA00u}});
                const bool invalid = kind == 1 || kind == 2 || kind == 3
                    || (route == 2 && (kind == 5 || kind == 6)) || (route == 0 && kind == 5);
                { std::ofstream output(path, std::ios::binary); output << "dictionary destination\n"; }
                const bool result = writer.write(&adapter, DRW::AC1027, binary);
                t.expect(result == !invalid, ("authored dictionary admission distinguishes unique/colliding keys and owning targets" + cell).c_str());
                t.expect(adapter.dictionary.handle == original.handle
                             && adapter.dictionary.parentHandle == original.parentHandle
                             && adapter.dictionary.m_entries[0].m_name == original.m_entries[0].m_name
                             && adapter.dictionary.m_entries[1].m_name == original.m_entries[1].m_name
                             && adapter.dictionary.m_entries[1].m_handle == original.m_entries[1].m_handle,
                         "dictionary admission preserves caller identities and key spelling");
                if (invalid) {
                    t.expect(contents() == "dictionary destination\n"
                                 && (route == 0 ? adapter.objectCalls == 1 && !adapter.written && adapter.before == adapter.after
                                                : adapter.objectCalls == 0),
                             "dictionary rejection precedes record/identity mutation and preserves destination");
                    adapter.dictionary.m_entries = {{"Alpha", 0x901u}, {"Beta", 0x902u}};
                    writer.setHandleRemap({});
                    configure();
                    adapter.objectCalls = 0;
                    t.expect(writer.write(&adapter, DRW::AC1027, binary),
                             "corrected dictionary retries on the same writer without consumed identity");
                }
                if (result || invalid) {
                    std::vector<TableOwnerRecord> records;
                    t.expect(readTableOwnerRecords(path, binary, records, false, false, true),
                             "dictionary admission output has complete literal framing");
                    const auto entries = entriesOf(records, route == 2 ? "C" : "900");
                    const auto& expected = adapter.dictionary.m_entries;
                    t.expect(entries.size() >= 4
                                 && entries[entries.size() - 4].second == expected[0].m_name
                                 && entries[entries.size() - 3].second == writer.toHexStrHandle(expected[0].m_handle)
                                 && entries[entries.size() - 2].second == expected[1].m_name
                                 && entries.back().second == writer.toHexStrHandle(expected[1].m_handle),
                             "literal dictionary output retains distinct original keys and target identities");
                    FixtureInterface reader;
                    dx_data imported;
                    t.expect(reader.fileImport(path.string(), &imported, false) && reader.scales.size() == 2
                                 && reader.scales[0].name == "target label 2305"
                                 && reader.scales[1].name == "target label 2306",
                             ("dictionary keys remain independent of target object names" + cell).c_str());
                }
                adapter.dxfW = nullptr;
            }
        }
        // Raw carriers and tolerant imports retain repeated keys/targets and
        // mixed350/360 spelling; they are not authored-dictionary admission.
        for (int kind : {0, 1, 2}) {
            dx_data data;
            DictionaryInterface adapter;
            adapter.attach(data);
            adapter.route = 1;
            adapter.rawRoute = true;
            adapter.dictionary.handle = 0x900u;
            adapter.dictionary.m_entries = {{"Alias", 0x901u},
                {kind == 1 ? "Alias" : "Other", kind == 2 ? 0x901u : 0x902u}};
            adapter.raw.name = "DICTIONARY";
            adapter.raw.handle = 0x900u;
            adapter.raw.parentHandle = 0xCu;
            adapter.raw.groups = {{5, std::string("900")}, {330, std::string("C")},
                {100, std::string("AcDbDictionary")}, {280, 0}, {281, 1},
                {3, adapter.dictionary.m_entries[0].m_name}, {350, std::string("901")},
                {3, adapter.dictionary.m_entries[1].m_name},
                {360, kind == 2 ? std::string("901") : std::string("902")}};
            const std::string filename = path.string();
            dxfRW writer(filename.c_str());
            adapter.dxfW = &writer;
            writer.setRootDictEntries({{"RawAlias", "900"}});
            t.expect(writer.write(&adapter, DRW::AC1027, binary) && adapter.written,
                     "raw dictionary replay preserves malformed-but-bounded aliases without authored-key tightening");
            FixtureInterface reader;
            dx_data imported;
            t.expect(reader.fileImport(path.string(), &imported, false), "dictionary import remains tolerant of key/target aliases");
            const auto raw = std::find_if(reader.rawObjects.begin(), reader.rawObjects.end(),
                                         [](const auto& value) { return value.handle == 0x900u; });
            t.expect(raw != reader.rawObjects.end(), "named dictionary retains its raw companion");
            std::vector<TableOwnerRecord> records;
            t.expect(readTableOwnerRecords(path, binary, records, false, false, true), "raw dictionary framing reaches EOF");
            t.expect(entriesOf(records, "900") == std::vector<std::pair<int, std::string>>{
                         {3, adapter.dictionary.m_entries[0].m_name}, {350, "901"},
                         {3, adapter.dictionary.m_entries[1].m_name}, {360, kind == 2 ? "901" : "902"}},
                     "raw dictionary keys, cardinality and mixed ownership codes remain exact");
            adapter.dxfW = nullptr;
        }
    }
}

void testImageAdjustmentAdmission(TestContext& t,
                                  const std::filesystem::path* retained = nullptr,
                                  bool retainedBinary = false) {
    class RasterInterface final : public dx_iface {
    public:
        void writeEntities() override {
            before = dxfW->highWaterHandle();
            rasterWritten = wipeout ? dxfW->writeWipeout(&mask)
                : dxfW->writeImage(&image, "adjustment-control.png") != nullptr;
            after = dxfW->highWaterHandle();
            suffixWritten = dxfW->writeLine(&suffix);
            afterSuffix = dxfW->highWaterHandle();
        }
        DRW_Image image;
        DRW_Wipeout mask;
        DRW_Line suffix;
        bool wipeout{false}, rasterWritten{false}, suffixWritten{false};
        std::uint32_t before{0}, after{0}, afterSuffix{0};
    } adapter;
    class RasterReader final : public dx_iface {
    public:
        void addImage(const DRW_Image* image) override {
            if (image != nullptr) rasters.push_back(*image);
            dx_iface::addImage(image);
        }
        void addWipeout(const DRW_Wipeout* mask) override {
            if (mask != nullptr) rasters.push_back(*mask);
        }
        void addLine(const DRW_Line& line) override {
            lines.push_back(line);
            dx_iface::addLine(line);
        }
        std::vector<DRW_Image> rasters;
        std::vector<DRW_Line> lines;
    };
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-image-adjustments-" + std::to_string(nonce) + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{path};
    const std::string filename = path.string();
    for (const bool binary : {false, true}) {
        for (const bool wipeout : {false, true}) {
            for (int field = 0; field != 3; ++field) {
                for (const int value : {-1, 0, 100, 101, 255, 256}) {
                    dx_data source;
                    source.headerC.addInt("$MEASUREMENT", 1, 70);
                    const auto* const headerValue = source.headerC.vars.at("$MEASUREMENT");
                    adapter.cData = &source;
                    adapter.currentBlock = source.mBlock;
                    adapter.image = DRW_Image();
                    adapter.mask = DRW_Wipeout();
                    adapter.wipeout = wipeout;
                    DRW_Image& raster = wipeout ? static_cast<DRW_Image&>(adapter.mask) : adapter.image;
                    raster.handle = 0xB100u;
                    raster.secPoint = {1, 0, 0};
                    raster.vVector = {0, 1, 0};
                    raster.sizeu = 4;
                    raster.sizev = 3;
                    raster.appData = {{{102, std::string("{IMAGE_CONTROL")},
                        {1, std::string("caller-metadata")}, {102, std::string("}")}}};
                    if (wipeout) raster.clipPath = {{0, 0, 0}, {4, 0, 0}, {0, 3, 0}};
                    int* adjustment = field == 0 ? &raster.brightness
                        : field == 1 ? &raster.contrast : &raster.fade;
                    *adjustment = value;
                    const DRW_Image before(raster);
                    adapter.suffix = DRW_Line();
                    adapter.suffix.handle = 0xB200u;
                    adapter.suffix.basePoint = {34, 35, 36};
                    adapter.suffix.secPoint = {37, 38, 39};
                    { std::ofstream seed(path, std::ios::binary); seed << "raster destination sentinel\n"; }
                    dxfRW writer(filename.c_str());
                    adapter.dxfW = &writer;
                    t.expect(writer.reserveHandle(0xE200u), "raster control reserves a persistent identity");
                    writer.setHandleRemap({{0xE100u, 0xE101u}});
                    const bool valid = value >= 0 && value <= 100;
                    t.expect(writer.write(&adapter, DRW::AC1027, binary) == valid
                                 && adapter.rasterWritten == valid && adapter.suffixWritten == valid,
                             "DXF IMAGE/WIPEOUT adjustments admit exactly 0..100 in both encodings");
                    const auto headerUnchanged = [&]() {
                        return source.headerC.vars.size() == 1
                            && source.headerC.vars.at("$MEASUREMENT") == headerValue
                            && headerValue->type() == DRW_Variant::INTEGER
                            && headerValue->content.i == 1;
                    };
                    t.expect(headerUnchanged() && *adjustment == value
                                 && raster.ref == before.ref && raster.m_imageDefReactorHandle == before.m_imageDefReactorHandle
                                 && raster.appData.size() == 1 && raster.appData.front().size() == 3
                                 && std::string(raster.appData.front().front().c_str()) == "{IMAGE_CONTROL"
                                 && std::string(std::next(raster.appData.front().begin())->c_str()) == "caller-metadata"
                                 && raster.clipPath.size() == before.clipPath.size(),
                             "raster admission retains caller payload/carriers and source HEADER ownership");
                    if (!valid) {
                        std::ifstream preserved(path, std::ios::binary);
                        t.expect(std::string(std::istreambuf_iterator<char>(preserved), {}) == "raster destination sentinel\n"
                                     && adapter.before == adapter.after && adapter.after == adapter.afterSuffix
                                     && raster.handle == 0xB100u && adapter.suffix.handle == 0xB200u,
                                 "rejected raster allocates no identities/reactors or suffix and preserves destination");
                        const auto* context = writer.getWritingContext();
                        t.expect(context->sourceHandleToMintedMap.empty()
                                     && context->writtenSourceEntityHandles.empty()
                                     && writer.remapHandle(0xE100u) == 0xE101u
                                     && writer.getError() == DRW::BAD_OPEN
                                     && writer.getLastDiagnostic().code == "emit-failure",
                                 "raster failure drops derived identities but preserves setup and first diagnostic");
                        preserved.close();
                        *adjustment = 0;
                        t.expect(writer.write(&adapter, DRW::AC1027, binary)
                                     && adapter.rasterWritten && adapter.suffixWritten && headerUnchanged()
                                     && writer.getError() == DRW::BAD_NONE,
                                 "correcting only adjustment permits exact same-writer same-model retry");
                    }
                    std::vector<TableOwnerRecord> records;
                    t.expect(readTableOwnerRecords(path, binary, records), "raster scalar framing reaches EOF");
                    const auto record = std::find_if(records.begin(), records.end(), [wipeout](const auto& entry) {
                        return entry.kind == (wipeout ? "WIPEOUT" : "IMAGE");
                    });
                    t.expect(record != records.end()
                                 && groupValue(record->groups, 281) == std::to_string(raster.brightness)
                                 && groupValue(record->groups, 282) == std::to_string(raster.contrast)
                                 && groupValue(record->groups, 283) == std::to_string(raster.fade),
                             "accepted boundaries retain exact I16 adjustment tags without clamping");
                    for (const std::string kind : {"IMAGEDEF", "IMAGEDEF_REACTOR"}) {
                        t.expect(std::count_if(records.begin(), records.end(), [&kind](const auto& entry) {
                            return entry.kind == kind;
                        }) == (wipeout ? 0 : 1), "only accepted IMAGE publishes one definition and reactor");
                    }
                    RasterReader reader;
                    dx_data decoded;
                    t.expect(reader.fileImport(filename, &decoded, false) && reader.rasters.size() == 1
                                 && reader.lines.size() == 1 && reader.lines.front().basePoint.x == 34,
                             "accepted raster plus suffix LINE reaches public callbacks");
                    if (reader.rasters.size() == 1) {
                        const auto& read = reader.rasters.front();
                        t.expect(read.brightness == raster.brightness && read.contrast == raster.contrast
                                     && read.fade == raster.fade,
                                 "public DXF raster readback keeps valid stored adjustment values");
                    }
                    if (retained != nullptr && !wipeout && binary == retainedBinary && field == 2 && value == 100)
                        std::filesystem::copy_file(path, *retained, std::filesystem::copy_options::overwrite_existing);
                    adapter.dxfW = nullptr;
                }
            }
        }
    }
}

std::vector<DRW_Wipeout> wipeoutPayloadControls() {
    std::vector<DRW_Wipeout> masks;
    for (const bool polygon : {false, true}) for (const bool inverted : {false, true}) {
        DRW_Wipeout mask;
        const double index = static_cast<double>(masks.size());
        mask.handle = 0xC100u + static_cast<std::uint32_t>(masks.size());
        mask.basePoint = {13 + index, 17, 19};
        mask.secPoint = {8, 1, 2}; mask.vVector = {3, 6, 4};
        mask.sizeu = 16; mask.sizev = 12; mask.m_displayProps = 7;
        mask.clip = 1; mask.brightness = 59; mask.contrast = 61; mask.fade = 7;
        mask.m_clipBoundaryType = polygon ? 2 : 1; mask.clipMode = inverted;
        mask.clipPath = polygon
            ? std::vector<DRW_Coord>{{-0.5, -0.5}, {8.5, -0.5}, {5.5, 10.5}, {-0.5, -0.5}}
            : std::vector<DRW_Coord>{{1.5, 2.5}, {9.5, 8.5}};
        mask.appData = {{{102, std::string("{MASK_METADATA")},
            {100, std::string("AcDbRasterImage")}, {1, std::string("owned application data")},
            {102, std::string("}")}}};
        mask.extData.push_back(std::make_shared<DRW_Variant>(1001, "MASK_APP"));
        mask.extData.push_back(std::make_shared<DRW_Variant>(1000, "mask " + std::to_string(masks.size())));
        masks.push_back(mask);
    }
    return masks;
}

class WipeoutPayloadWriter final : public dx_iface {
public:
    void writeEntities() override {
        before = dxfW->highWaterHandle();
        accepted = true;
        for (auto& mask : masks) accepted = dxfW->writeWipeout(&mask) && accepted;
        after = dxfW->highWaterHandle();
        if (withImage) {
            DRW_Image image;
            image.m_classVersion = 6; image.secPoint = {1, 0, 0}; image.vVector = {0, 1, 0};
            image.sizeu = 16; image.sizev = 12;
            image.clipPath = {{1.5, 2.5}, {9.5, 8.5}};
            accepted = (dxfW->writeImage(&image, "image-control.png") != nullptr) && accepted;
        }
        suffixAccepted = dxfW->writeLine(&suffix);
    }
    std::vector<DRW_Wipeout> masks {wipeoutPayloadControls()};
    DRW_Line suffix;
    bool withImage {true}, accepted {false}, suffixAccepted {false};
    std::uint32_t before {0}, after {0};
};

bool generateWipeoutPayload(const std::filesystem::path& path, bool binary) {
    dx_data data;
    DRW_AppId app; app.name = "MASK_APP"; data.appIds.push_back(app);
    WipeoutPayloadWriter sink;
    sink.suffix.basePoint = {34, 35, 36}; sink.suffix.secPoint = {37, 38, 39};
    sink.cData = &data; sink.currentBlock = data.mBlock;
    const std::string name = path.string(); dxfRW writer(name.c_str()); sink.dxfW = &writer;
    return writer.write(&sink, DRW::AC1024, binary) && sink.accepted && sink.suffixAccepted;
}

bool generateWipeoutBlockPayload(const std::filesystem::path& path, bool binary) {
    dx_data data;
    DRW_AppId app; app.name = "MASK_APP"; data.appIds.push_back(app);
    DRW_Block_Record record; record.handle = 0xC600u; record.name = "MaskBlock";
    data.blockRecords.push_back(record);
    auto* block = new dx_ifaceBlock;
    block->name = record.name; block->handle = 0xC500u; block->parentHandle = record.handle;
    data.blocks.push_back(block);
    for (auto mask : wipeoutPayloadControls()) {
        mask.parentHandle = record.handle;
        block->ent.push_back(new DRW_Wipeout(mask));
    }
    auto* inside = new DRW_Line;
    inside->basePoint = {34, 35, 36}; inside->secPoint = {37, 38, 39};
    inside->parentHandle = record.handle; block->ent.push_back(inside);
    auto* insert = new DRW_Insert; insert->name = record.name; data.mBlock->ent.push_back(insert);
    auto* outside = new DRW_Line;
    outside->basePoint = {44, 45, 46}; outside->secPoint = {47, 48, 49}; data.mBlock->ent.push_back(outside);
    dx_iface exporter;
    return exporter.fileExport(path.string(), DRW::AC1024, binary, &data, false);
}

// Physical tags before document loading/audit can repair or default values.
struct RasterPayloadTags {
    std::string kind;
    std::vector<std::string> markers;
    std::vector<int> codes;
    int classVersion {-1}, boundaryType {-1}, count {-1}, clipMode {-1};
};

bool rasterPayloadTags(const std::filesystem::path& path, bool binary,
                       std::vector<RasterPayloadTags>& records, int& definitions) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    std::unique_ptr<dxfReader> reader;
    if (binary) { input.seekg(22); reader = std::make_unique<dxfReaderBinary>(&input); }
    else reader = std::make_unique<dxfReaderAscii>(&input);
    int code = 0, appDepth = 0;
    bool payload = false;
    RasterPayloadTags* current = nullptr;
    while (reader->readRec(&code)) {
        if (code == 0) {
            const std::string kind = reader->getString();
            if (kind == "EOF") return true;
            if (kind == "IMAGEDEF" || kind == "IMAGEDEF_REACTOR") ++definitions;
            current = nullptr; appDepth = 0; payload = false;
            if (kind == "WIPEOUT" || kind == "IMAGE") {
                records.push_back({kind, {}, {}}); current = &records.back();
            }
        } else if (current != nullptr) {
            if (code == 102) {
                const std::string value = reader->getString();
                if (!value.empty() && value.front() == '{') ++appDepth;
                else if (value == "}") --appDepth;
                continue;
            }
            if (appDepth != 0) continue;
            if (code == 100) {
                const std::string marker = reader->getString();
                if (marker != "AcDbEntity") { current->markers.push_back(marker); payload = true; }
            } else if (payload) {
                current->codes.push_back(code);
                if (code == 90) current->classVersion = reader->getInt32();
                else if (code == 71) current->boundaryType = reader->getInt32();
                else if (code == 91) current->count = reader->getInt32();
                else if (code == 290) current->clipMode = reader->getBool() ? 1 : 0;
            }
        }
    }
    return false;
}

bool sameWipeoutPayload(const DRW_Wipeout& value, const DRW_Wipeout& expected) {
    const auto sameCoord = [](const DRW_Coord& a, const DRW_Coord& b) {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    };
    if (!sameCoord(value.basePoint, expected.basePoint) || !sameCoord(value.secPoint, expected.secPoint)
        || !sameCoord(value.vVector, expected.vVector) || value.sizeu != expected.sizeu
        || value.sizev != expected.sizev || value.m_displayProps != expected.m_displayProps
        || value.clip != expected.clip || value.brightness != expected.brightness
        || value.contrast != expected.contrast || value.fade != expected.fade
        || value.m_clipBoundaryType != expected.m_clipBoundaryType || value.clipMode != expected.clipMode
        || value.clipPath.size() != expected.clipPath.size() || value.ref != 0
        || value.m_imageDefReactorHandle != 0 || value.appData.size() != expected.appData.size()
        || value.extData.size() != expected.extData.size()) return false;
    for (std::size_t i = 0; i != value.clipPath.size(); ++i)
        if (!sameCoord(value.clipPath[i], expected.clipPath[i])) return false;
    for (std::size_t i = 0; i != value.extData.size(); ++i)
        if (value.extData[i]->code() != expected.extData[i]->code()
            || std::string(value.extData[i]->c_str()) != expected.extData[i]->c_str()) return false;
    auto actualGroup = value.appData.begin();
    for (const auto& expectedGroup : expected.appData) {
        if (actualGroup->size() != expectedGroup.size()) return false;
        auto actual = actualGroup->begin();
        for (const auto& wanted : expectedGroup) {
            std::string actualText = actual->c_str(), expectedText = wanted.c_str();
            // The established model strips the opening brace from the first
            // 102 application name and the writer restores it on output.
            if (actual->code() == 102 && !actualText.empty() && actualText.front() == '{') actualText.erase(0, 1);
            if (wanted.code() == 102 && !expectedText.empty() && expectedText.front() == '{') expectedText.erase(0, 1);
            if (actual->code() != wanted.code() || actualText != expectedText) return false;
            ++actual;
        }
        ++actualGroup;
    }
    return true;
}

bool coherentRasterTags(const RasterPayloadTags& record, const DRW_Wipeout* mask) {
    const std::size_t vertices = mask != nullptr ? mask->clipPath.size() : 2;
    std::vector<int> expected {90, 10, 20, 30, 11, 21, 31, 12, 22, 32, 13, 23,
                              340, 70, 280, 281, 282, 283, 360, 71, 91};
    for (std::size_t j = 0; j != vertices; ++j) { expected.push_back(14); expected.push_back(24); }
    expected.push_back(290);
    if (mask != nullptr) { expected.push_back(1001); expected.push_back(1000); }
    return record.markers == std::vector<std::string>{mask != nullptr ? "AcDbWipeout" : "AcDbRasterImage"}
        && record.codes == expected && record.classVersion == (mask != nullptr ? 0 : 6)
        && record.boundaryType == (mask != nullptr ? mask->m_clipBoundaryType : 1)
        && record.count == int(vertices) && record.clipMode == (mask != nullptr ? int(mask->clipMode) : 0);
}

bool verifyWipeoutPayload(const std::filesystem::path& path, bool binary) {
    std::vector<RasterPayloadTags> records;
    int definitions = 0;
    if (!rasterPayloadTags(path, binary, records, definitions) || records.size() != 5 || definitions != 2)
        return false; // Only the positive IMAGE control owns a definition/reactor.
    const auto masks = wipeoutPayloadControls();
    for (std::size_t i = 0; i != records.size(); ++i) {
        if (!coherentRasterTags(records[i], i < masks.size() ? &masks[i] : nullptr)) return false;
    }
    dx_data data; dx_iface reader;
    if (!reader.fileImport(path.string(), &data, false) || data.mBlock->ent.size() != 6) return false;
    auto entity = data.mBlock->ent.begin();
    for (const auto& expected : masks) {
        const auto* mask = dynamic_cast<const DRW_Wipeout*>(*entity++);
        if (mask == nullptr || !sameWipeoutPayload(*mask, expected)) return false;
    }
    const auto* image = dynamic_cast<const DRW_Image*>(*entity++);
    const auto* suffix = dynamic_cast<const DRW_Line*>(*entity);
    return image != nullptr && image->eType == DRW::IMAGE && suffix != nullptr
        && suffix->eType == DRW::LINE && suffix->basePoint.x == 34 && suffix->secPoint.z == 39;
}

bool verifyWipeoutBlockPayload(const std::filesystem::path& path, bool binary) {
    std::vector<RasterPayloadTags> records; int definitions = 0;
    const auto masks = wipeoutPayloadControls();
    if (!rasterPayloadTags(path, binary, records, definitions) || records.size() != 4 || definitions != 0) return false;
    for (std::size_t i = 0; i != records.size(); ++i)
        if (!coherentRasterTags(records[i], &masks[i])) return false;
    dx_data data; dx_iface reader;
    if (!reader.fileImport(path.string(), &data, false) || data.mBlock->ent.size() != 2 || !data.images.empty()) return false;
    const auto block = std::find_if(data.blocks.begin(), data.blocks.end(), [](const auto* value) {
        return value->name == "MaskBlock";
    });
    const auto record = std::find_if(data.blockRecords.begin(), data.blockRecords.end(), [](const auto& value) {
        return value.name == "MaskBlock";
    });
    if (block == data.blocks.end() || record == data.blockRecords.end() || (*block)->ent.size() != 5) return false;
    auto entity = (*block)->ent.begin();
    for (const auto& expected : masks) {
        const auto* mask = dynamic_cast<const DRW_Wipeout*>(*entity++);
        if (mask == nullptr || !sameWipeoutPayload(*mask, expected) || mask->parentHandle != record->handle) return false;
    }
    const auto* inside = dynamic_cast<const DRW_Line*>(*entity);
    const auto* outside = dynamic_cast<const DRW_Line*>(data.mBlock->ent.back());
    return inside != nullptr && inside->basePoint.x == 34 && outside != nullptr && outside->basePoint.x == 44;
}

bool generateLegacyWipeoutPayload(const std::filesystem::path& path, bool binary, bool split) {
    std::ofstream output(path, std::ios::binary);
    if (!output) return false;
    std::unique_ptr<dxfWriter> writer;
    if (binary) {
        output.write("AutoCAD Binary DXF\r\n\x1a\0", 22);
        writer = std::make_unique<dxfWriterBinary>(&output);
    } else writer = std::make_unique<dxfWriterAscii>(&output);
    writer->setVersion("AC1024", true);
    const auto text = [&writer](int code, const std::string& value) { writer->writeString(code, value); };
    const auto integer = [&writer](int code, int value) { writer->writeInt16(code, value); };
    const auto real = [&writer](int code, double value) { writer->writeDouble(code, value); };
    const auto mask = wipeoutPayloadControls()[3];
    text(0, "SECTION"); text(2, "HEADER"); text(9, "$ACADVER"); text(1, "AC1024"); text(0, "ENDSEC");
    text(0, "SECTION"); text(2, "ENTITIES"); text(0, "WIPEOUT"); text(5, "C103");
    text(100, "AcDbEntity"); text(8, "0");
    text(102, "{MASK_METADATA"); text(100, "AcDbRasterImage");
    text(1, "owned application data"); text(102, "}");
    text(100, "AcDbRasterImage");
    if (!split) writer->writeInt32(90, 0);
    real(10, mask.basePoint.x); real(20, mask.basePoint.y); real(30, mask.basePoint.z);
    real(11, mask.secPoint.x); real(21, mask.secPoint.y); real(31, mask.secPoint.z);
    real(12, mask.vVector.x); real(22, mask.vVector.y); real(32, mask.vVector.z);
    real(13, mask.sizeu); real(23, mask.sizev); text(340, "0"); integer(70, mask.m_displayProps);
    integer(280, mask.clip); integer(281, mask.brightness); integer(282, mask.contrast);
    integer(283, mask.fade); text(360, "0");
    if (split) { text(100, "AcDbWipeout"); writer->writeInt32(90, 0); }
    integer(71, mask.m_clipBoundaryType); writer->writeInt32(91, static_cast<std::int32_t>(mask.clipPath.size()));
    for (const auto& point : mask.clipPath) { real(14, point.x); real(24, point.y); }
    writer->writeBool(290, mask.clipMode); text(1001, "MASK_APP"); text(1000, "mask 3");
    text(0, "LINE"); text(5, "C200"); text(100, "AcDbEntity"); text(8, "0"); text(100, "AcDbLine");
    real(10, 34); real(20, 35); real(30, 36); real(11, 37); real(21, 38); real(31, 39);
    text(0, "ENDSEC"); text(0, "EOF");
    return !writer->hasWriteError() && output.good();
}

void testWipeoutSubclassPayload(TestContext& t) {
    const int initialFailures = t.failures;
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-wipeout-payload-" + std::to_string(nonce));
    std::filesystem::create_directory(directory);
    for (const bool binary : {false, true}) {
        const auto path = directory / "source.dxf";
        t.expect(generateWipeoutPayload(path, binary), "generated WIPEOUT masking payload writes");
        t.expect(verifyWipeoutPayload(path, binary),
                 "WIPEOUT owns one ordered payload with rectangle/polygon/mode/XDATA and IMAGE unchanged");
        auto input = path;
        for (int pass = 0; pass != 2; ++pass) {
            dx_data data; dx_iface converter;
            const auto output = directory / ("repeat" + std::to_string(pass) + ".dxf");
            t.expect(converter.fileImport(input.string(), &data, false)
                         && converter.fileExport(output.string(), DRW::AC1024, binary, &data, false)
                         && verifyWipeoutPayload(output, binary),
                     "two WIPEOUT conversions retain literal scope and exact pixel-space masking payload");
            input = output;
        }
        const auto blockInput = directory / "named block.dxf";
        t.expect(generateWipeoutBlockPayload(blockInput, binary) && verifyWipeoutBlockPayload(blockInput, binary),
                 "concrete WIPEOUT callback retains named BLOCK ownership outside the image-link list");
        input = blockInput;
        for (int pass = 0; pass != 2; ++pass) {
            dx_data data; dx_iface converter;
            const auto output = directory / ("block repeat" + std::to_string(pass) + ".dxf");
            t.expect(converter.fileImport(input.string(), &data, false)
                         && converter.fileExport(output.string(), DRW::AC1024, binary, &data, false)
                         && verifyWipeoutBlockPayload(output, binary),
                     "two actual adapter conversions retain BLOCK masks, raw scope, ownership and following LINE");
            input = output;
        }
        for (const bool split : {false, true}) {
            const auto legacy = directory / "legacy layout.dxf";
            const auto canonical = directory / "canonical layout.dxf";
            dx_data data; dx_iface converter;
            t.expect(generateLegacyWipeoutPayload(legacy, binary, split)
                         && converter.fileImport(legacy.string(), &data, false)
                         && data.mBlock->ent.size() == 2,
                     "documented RasterImage and old split WIPEOUT input remain readable in both encodings");
            const auto* mask = data.mBlock->ent.empty() ? nullptr
                : dynamic_cast<const DRW_Wipeout*>(data.mBlock->ent.front());
            t.expect(mask != nullptr && sameWipeoutPayload(*mask, wipeoutPayloadControls()[3]),
                     "old WIPEOUT layouts retain polygon/inversion and common application/XDATA payload");
            std::vector<RasterPayloadTags> tags; int definitions = 0;
            t.expect(converter.fileExport(canonical.string(), DRW::AC1024, binary, &data, false)
                         && rasterPayloadTags(canonical, binary, tags, definitions) && tags.size() == 1
                         && tags.front().markers == std::vector<std::string>{"AcDbWipeout"}
                         && tags.front().count == 4 && tags.front().clipMode == 1 && definitions == 0,
                     "old WIPEOUT layouts canonicalize to one complete marker without IMAGE objects");
        }
        for (const DRW::Version version : {DRW::AC1015, DRW::AC1018, DRW::AC1021, DRW::AC1024}) {
            dx_data data; WipeoutPayloadWriter sink;
            sink.withImage = false; sink.masks = {wipeoutPayloadControls()[1]};
            sink.suffix.handle = 0xC200u;
            sink.cData = &data; sink.currentBlock = data.mBlock;
            const auto output = directory / "older target.dxf";
            { std::ofstream seed(output, std::ios::binary); seed << "mask destination sentinel\n"; }
            const std::string name = output.string(); dxfRW writer(name.c_str()); sink.dxfW = &writer;
            t.expect(writer.reserveHandle(0xE200u), "WIPEOUT old-target control reserves persistent identity");
            writer.setHandleRemap({{0xE100u, 0xE101u}});
            const auto original = sink.masks.front();
            const bool modern = version > DRW::AC1021;
            t.expect(writer.write(&sink, version, binary) == modern && sink.accepted == modern
                         && sink.suffixAccepted == modern,
                     "inverted WIPEOUT explicitly rejects pre-R2010 output rather than omitting mode");
            if (!modern) {
                std::ifstream stream(output, std::ios::binary);
                const auto* context = writer.getWritingContext();
                t.expect(std::string(std::istreambuf_iterator<char>(stream), {}) == "mask destination sentinel\n"
                             && sink.before == sink.after && sink.masks.front().handle == original.handle
                             && sameWipeoutPayload(sink.masks.front(), original)
                             && sink.suffix.handle == 0xC200u && context->sourceHandleToMintedMap.empty()
                             && context->writtenSourceEntityHandles.empty() && writer.remapHandle(0xE100u) == 0xE101u,
                         "older inverted WIPEOUT rejection preserves caller/allocator/maps/destination");
                sink.masks.front().clipMode = false;
                t.expect(writer.write(&sink, version, binary) && sink.accepted && sink.suffixAccepted,
                         "same writer accepts corrected normal WIPEOUT on the older target");
                std::vector<RasterPayloadTags> tags; int definitions = 0;
                t.expect(rasterPayloadTags(output, binary, tags, definitions) && tags.size() == 1
                             && tags.front().markers == std::vector<std::string>{"AcDbWipeout"}
                             && tags.front().clipMode == -1 && definitions == 0,
                         "older normal WIPEOUT keeps coherent payload without mode or IMAGEDEF synthesis");
            }
        }
    }
    if (t.failures == initialFailures) {
        std::error_code error; std::filesystem::remove_all(directory, error);
    } else std::cerr << "WIPEOUT evidence: " << directory << '\n';
}

void testSortEntsKeyRoles(TestContext& t) {
    class SortInterface final : public dx_iface {
    public:
        void writeHeader(DRW_Header& header) override {
            dx_iface::writeHeader(header);
            dxfW->setRootDictEntries({{"RESEARCH_SORT_KEYS", "600"}});
            std::map<std::uint32_t, std::uint32_t> remap{{0x601u, 0x701u},
                {0x602u, 0x702u}, {0x603u, 0x703u}};
            if (!plannedOwner)
                remap.emplace(0x500u, 0x1Fu);
            dxfW->setHandleRemap(remap);
            dxfW->reserveHandle(0x703u);
            std::vector<DRW_RawDxfSection> sections;
            if (sectionControl) {
                // Namespace-only raw-section control, not a CAD draw-order
                // graph. Its following non-SORTENTSTABLE record must restore
                // normal group-5 translation even with the same marker text.
                DRW_RawDxfSection section;
                section.m_name = "SORT_KEY_SCOPE_CONTROL";
                section.m_version = dxfW->getVersion();
                section.m_groups = {{0, std::string("SORTENTSTABLE")},
                    {5, std::string("602")}, {330, std::string("600")},
                    {100, std::string("AcDbSortentsTable")}, {330, std::string("500")},
                    {331, std::string("A000")}, {5, std::string("601")},
                    {0, std::string("RESEARCH_OTHER")}, {5, std::string("603")},
                    {330, std::string("601")}, {100, std::string("AcDbSortentsTable")},
                    {5, std::string("601")}};
                sections.push_back(std::move(section));
            }
            dxfW->setRawDxfSections(sections);
        }
        void writeEntities() override {
            dxfW->markSourceHandleAmbiguous(0xABCu);
            for (int i = 0; i != 2; ++i) {
                const int index = reverse ? 1 - i : i;
                const std::uint32_t source = 0xA000u + static_cast<std::uint32_t>(index);
                if (sourceMap) {
                    DRW_Line line;
                    line.handle = source;
                    line.basePoint = DRW_Coord(index + 1, 0, 0);
                    line.secPoint = DRW_Coord(index + 1, 1, 0);
                    entitiesWritten = dxfW->writeLine(&line) && entitiesWritten;
                } else {
                    // A real no-source-map control, with already-target-bound
                    // entity identities rather than dangling 331 references.
                    DRW_RawDxfObject line;
                    line.name = "LINE";
                    line.handle = source;
                    line.groups = {{5, dxfW->toHexStrHandle(source)},
                        {330, std::string("1F")}, {100, std::string("AcDbEntity")},
                        {8, std::string("0")}, {100, std::string("AcDbLine")},
                        {10, double(index + 1)}, {20, 0.0}, {30, 0.0},
                        {11, double(index + 1)}, {21, 1.0}, {31, 0.0}};
                    entitiesWritten = dxfW->writeRawDxfObject(&line) && entitiesWritten;
                }
            }
        }
        void writeObjects() override {
            DRW_Dictionary owner;
            owner.handle = 0x600u;
            owner.parentHandle = 0xCu;
            owner.hardOwner = 1;
            owner.m_entries.push_back({"ACAD_SORTENTS", 0x601u});
            dictionaryWritten = dxfW->writeDictionary(&owner);
            if (explicitReference) {
                dxfW->setHandleRemap({{0x500u, 0x1Fu}, {0x601u, 0x701u},
                    {0x602u, 0x702u}, {0x603u, 0x703u},
                    {0xDEADu, dxfW->remapEntityHandle(0xA001u)}});
            }
            before = dxfW->highWaterHandle();
            result = rawRoute ? dxfW->writeRawDxfObject(&raw)
                              : dxfW->writeSortEntsTable(&sort);
            after = dxfW->highWaterHandle();
        }
        DRW_SortEntsTable sort;
        DRW_RawDxfObject raw;
        bool rawRoute{false}, reverse{false}, sourceMap{true}, sectionControl{false};
        bool plannedOwner{false};
        bool explicitReference{false};
        bool result{false}, entitiesWritten{true}, dictionaryWritten{false};
        std::uint32_t before{0}, after{0};
    } adapter;
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-sort-keys-" + std::to_string(nonce) + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{path};
    const std::string filename = path.string();
    const std::vector<std::array<std::string, 2>> controls{
        {"0000a000", "1000"}, {"a000", "a001"}, {"601", "500"},
        {"dead", "dead"}, {"0", "10"}, {"ffffffff", "1000"},
        {"abc", "1000"}, {"100000001", "a000"}};
    const auto hex = [](const std::string& text) {
        std::uint64_t value = 0;
        std::from_chars(text.data(), text.data() + text.size(), value, 16);
        return value;
    };
    for (const DRW::Version version : {DRW::AC1018, DRW::AC1024}) {
        for (const bool binary : {false, true}) {
            for (const bool rawRoute : {false, true}) {
                for (const bool reverse : {false, true}) {
                    for (const bool sourceMap : {false, true}) {
                        for (const auto& keys : controls) {
                            const bool wide = hex(keys[0]) > std::numeric_limits<std::uint32_t>::max();
                            if (wide && !rawRoute)
                                continue; // 7T: the typed API still has a 32-bit boundary.
                            dx_data source;
                            adapter.cData = &source;
                            adapter.currentBlock = source.mBlock;
                            adapter.rawRoute = rawRoute;
                            adapter.reverse = reverse;
                            adapter.sourceMap = sourceMap;
                            adapter.sectionControl = rawRoute && reverse && sourceMap
                                && keys[0] == "0000a000";
                            adapter.plannedOwner = keys[0] == "601";
                            adapter.explicitReference = false;
                            adapter.entitiesWritten = true;
                            adapter.sort.reset();
                            adapter.sort.handle = 0x601u;
                            adapter.sort.parentHandle = 0x600u;
                            adapter.sort.m_blockOwnerHandle = 0x500u;
                            adapter.sort.m_entityHandles = {0xA000u, 0xA001u};
                            adapter.sort.m_sortHandles = {static_cast<std::uint32_t>(hex(keys[0])),
                                                         static_cast<std::uint32_t>(hex(keys[1]))};
                            adapter.raw = DRW_RawDxfObject{};
                            adapter.raw.name = "SORTENTSTABLE";
                            adapter.raw.handle = 0x601u;
                            adapter.raw.m_version = version;
                            adapter.raw.groups = {{5, std::string("0601")},
                                {330, std::string("600")}, {100, std::string("AcDbSortentsTable")},
                                {330, std::string("0500")}, {331, std::string("0000a000")},
                                {5, keys[0]}, {331, std::string("0000a001")}, {5, keys[1]}};
                            adapter.raw.hasRawValues = !binary;
                            if (!binary) {
                                for (const auto& group : adapter.raw.groups)
                                    adapter.raw.rawValues.emplace_back(group.c_str());
                            }
                            const auto rawBefore = adapter.raw.groups;
                            const auto rawValuesBefore = adapter.raw.rawValues;
                            const auto entitiesBefore = adapter.sort.m_entityHandles;
                            const auto keysBefore = adapter.sort.m_sortHandles;
                            dxfRW writer(filename.c_str());
                            if (adapter.plannedOwner) {
                                DRW_Block_Record block;
                                block.handle = 0x500u;
                                block.name = "*Model_Space";
                                writer.setSourceBlockRecords({block});
                            }
                            adapter.dxfW = &writer;
                            t.expect(writer.write(&adapter, version, binary) && adapter.result
                                         && adapter.entitiesWritten && adapter.dictionaryWritten,
                                     "SORTENTSTABLE zero/shared/colliding/ambiguous-value keys write");
                            std::vector<TableOwnerRecord> records;
                            t.expect(readTableOwnerRecords(path, binary, records, false, wide),
                                     "SORTENTSTABLE literal framing reaches EOF, including raw wide keys");
                            const auto record = std::find_if(records.begin(), records.end(),
                                [](const auto& value) { return value.kind == "SORTENTSTABLE"; });
                            t.expect(record != records.end(), "actual SORTENTSTABLE record is emitted");
                            if (record != records.end()) {
                                bool body = false;
                                std::vector<std::string> emittedKeys, emittedEntities;
                                for (const auto& tag : record->groups) {
                                    if (tag == std::make_pair(100, std::string("AcDbSortentsTable")))
                                        body = true;
                                    else if (body && tag.first == 5)
                                        emittedKeys.push_back(tag.second);
                                    else if (body && tag.first == 331)
                                        emittedEntities.push_back(tag.second);
                                    else if (body && tag.first == 330)
                                        t.expect(hex(tag.second) == 0x1Fu,
                                                 "SORTENTSTABLE block owner remains an actual translated reference");
                                }
                                t.expect(groupValue(record->groups, 5) == "701"
                                             && emittedKeys.size() == 2 && emittedEntities.size() == 2,
                                         "header identity remaps independently of complete body key pairs");
                                for (std::size_t i = 0; i < 2 && i < emittedKeys.size()
                                     && i < emittedEntities.size(); ++i) {
                                    t.expect(hex(emittedKeys[i]) == hex(keys[i])
                                                 && (!rawRoute || emittedKeys[i] == keys[i]),
                                             "numeric sort keys retain values and raw lexemes without identity translation");
                                    t.expect(hex(emittedEntities[i]) == writer.remapEntityHandle(
                                                 0xA000u + static_cast<std::uint32_t>(i)),
                                             "entity references resolve after either source map insertion order");
                                }
                            }
                            if (adapter.sectionControl) {
                                const auto scoped = std::find_if(records.begin(), records.end(),
                                    [](const auto& value) {
                                        return value.kind == "SORTENTSTABLE"
                                            && groupValue(value.groups, 5) == "702";
                                    });
                                const auto next = std::find_if(records.begin(), records.end(),
                                    [](const auto& value) { return value.kind == "RESEARCH_OTHER"; });
                                t.expect(scoped != records.end()
                                             && scoped->groups.back() == std::make_pair(5, std::string("601"))
                                             && next != records.end() && groupValue(next->groups, 5) == "703"
                                             && next->groups.back() == std::make_pair(5, std::string("701")),
                                         "raw record boundaries reset the narrowly scoped sort-key exception");
                            }
                            t.expect(adapter.sort.handle == 0x601u
                                         && adapter.sort.m_blockOwnerHandle == 0x500u
                                         && adapter.sort.m_entityHandles == entitiesBefore
                                         && adapter.sort.m_sortHandles == keysBefore
                                         && adapter.raw.groups.size() == rawBefore.size()
                                         && adapter.raw.rawValues == rawValuesBefore
                                         && std::string(adapter.raw.groups[5].c_str()) == keys[0],
                                     "SORTENTSTABLE emission does not mutate caller identities or key arrays");
                            adapter.dxfW = nullptr;
                        }
                    }
                }
            }
        }
    }
    // Complete effective redraw order relative to unlisted entities is not
    // asserted here: their implicit identity keys still need the 7G.3 planner.
    for (const bool rawRoute : {false, true}) {
        for (const bool binary : {false, true}) {
            for (const int failure : {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10}) {
                dx_data source;
                adapter.cData = &source;
                adapter.currentBlock = source.mBlock;
                adapter.rawRoute = rawRoute;
                adapter.reverse = false;
                adapter.sourceMap = true;
                adapter.sectionControl = false;
                adapter.plannedOwner = false;
                adapter.explicitReference = false;
                adapter.sort.reset();
                adapter.sort.handle = 0x601u;
                adapter.sort.parentHandle = 0x600u;
                adapter.sort.m_blockOwnerHandle = 0x500u;
                adapter.sort.m_entityHandles = {0xA000u,
                    failure == 1 || failure == 3 || failure == 5 || failure == 8 ? 0xDEADu
                    : failure == 2 || failure == 4 ? 0xABCu : failure == 10 ? 0u : 0xA001u};
                if (failure == 6 || failure == 7 || failure == 9)
                    adapter.sort.handle = 0;
                adapter.sort.m_sortHandles = failure == 0 ? std::vector<std::uint32_t>{0xABCu}
                                                        : std::vector<std::uint32_t>{0xABCu, 0u};
                adapter.raw = DRW_RawDxfObject{};
                adapter.raw.name = "SORTENTSTABLE";
                adapter.raw.handle = 0x601u;
                adapter.raw.m_version = DRW::AC1024;
                adapter.raw.groups = {{5, std::string("601")}, {330, std::string("600")},
                    {100, std::string("AcDbSortentsTable")}, {330, std::string("500")},
                    {331, std::string("A000")}}; // incomplete pair
                if (failure == 1)
                    adapter.raw.groups.push_back({331, std::string("A001")}); // duplicate pending entity
                else if (failure == 2)
                    adapter.raw.groups.push_back({5, std::string("not-a-handle")});
                else if (failure >= 3) {
                    adapter.raw.groups.push_back({5, std::string("abc")});
                    adapter.raw.groups.push_back({331, std::string("A001")});
                    adapter.raw.groups.push_back({5, std::string("0")});
                    if (failure == 3 || failure == 4 || failure == 5 || failure == 10)
                        adapter.raw.groups[4] = DRW_Variant(331,
                            std::string(failure == 3 ? "DEAD" : failure == 4 ? "ABC"
                                : failure == 10 ? "0" : "100000001"));
                    else if (failure == 6)
                        adapter.raw.groups[0] = DRW_Variant(5, std::string("0"));
                    else if (failure == 7)
                        adapter.raw.groups.erase(adapter.raw.groups.begin());
                    else if (failure == 8) {
                        adapter.raw.groups.resize(5);
                        adapter.raw.groups.push_back({100, std::string("OtherSubclass")});
                        adapter.raw.groups.push_back({5, std::string("601")});
                    } else {
                        adapter.raw.groups.erase(adapter.raw.groups.begin());
                        adapter.raw.groups.insert(adapter.raw.groups.begin(), {
                            {102, std::string("{IDENTITY_NOT_HERE")},
                            {5, std::string("601")}, {102, std::string("}")}});
                    }
                }
                { std::ofstream original(path, std::ios::binary); original << "sort destination unchanged\n"; }
                dxfRW writer(filename.c_str());
                adapter.dxfW = &writer;
                const auto callerEntities = adapter.sort.m_entityHandles;
                const auto callerKeys = adapter.sort.m_sortHandles;
                const auto callerHandle = adapter.sort.handle;
                const auto rawProjection = [](const DRW_RawDxfObject& raw) {
                    std::vector<std::pair<int, std::string>> values;
                    for (const auto& group : raw.groups)
                        values.emplace_back(group.code(), group.c_str());
                    return values;
                };
                const auto callerRaw = rawProjection(adapter.raw);
                t.expect(!writer.write(&adapter, DRW::AC1024, binary) && !adapter.result
                             && adapter.before == adapter.after
                             && adapter.sort.m_entityHandles == callerEntities
                             && adapter.sort.m_sortHandles == callerKeys
                             && adapter.sort.handle == callerHandle
                             && rawProjection(adapter.raw) == callerRaw,
                         "invalid SORTENTSTABLE pairs/references reject before allocator/caller mutation");
                std::ifstream preserved(path, std::ios::binary);
                t.expect(std::string(std::istreambuf_iterator<char>(preserved), {})
                             == "sort destination unchanged\n",
                         "invalid SORTENTSTABLE leaves the destination byte-identical");
                preserved.close();
                adapter.sort.handle = 0x601u;
                adapter.sort.m_entityHandles = {0xA000u, 0xA001u};
                adapter.sort.m_sortHandles = {0xABCu, 0u};
                adapter.raw.groups = {{5, std::string("601")}, {330, std::string("600")},
                    {100, std::string("AcDbSortentsTable")}, {330, std::string("500")},
                    {331, std::string("A000")}, {5, std::string("abc")},
                    {331, std::string("A001")}, {5, std::string("0")}};
                t.expect(writer.write(&adapter, DRW::AC1024, binary) && adapter.result,
                         "correcting only invalid SORTENTSTABLE fields permits same-writer retry");
                adapter.dxfW = nullptr;
            }
        }
    }
    for (const bool binary : {false, true}) {
        for (const int transport : {0, 1, 2, 3}) {
            dx_data source;
            adapter.cData = &source;
            adapter.currentBlock = source.mBlock;
            adapter.rawRoute = true;
            adapter.reverse = false;
            adapter.sourceMap = transport >= 2;
            adapter.sectionControl = false;
            adapter.plannedOwner = transport == 3;
            adapter.explicitReference = transport == 2;
            adapter.raw = DRW_RawDxfObject{};
            adapter.raw.name = "SORTENTSTABLE";
            adapter.raw.handle = 0x601u;
            adapter.raw.m_version = DRW::AC1024;
            adapter.raw.groups = {{5, std::string("601")}, {330, std::string("600")},
                {100, std::string("AcDbSortentsTable")}, {330, std::string("500")},
                {331, std::string(transport == 1 ? "100000001" : transport == 3 ? "A000" : "DEAD")},
                {5, std::string("601")}};
            if (transport == 3) {
                adapter.raw.groups.insert(adapter.raw.groups.end(), {
                    {102, std::string("{BODY_APP")}, {5, std::string("601")},
                    {330, std::string("500")}, {331, std::string("DEAD")},
                    {100, std::string("OtherSubclass")}, {102, std::string("}")},
                    {331, std::string("A001")}, {5, std::string("601")},
                    {100, std::string("OtherSubclass")}, {5, std::string("601")},
                    {330, std::string("500")}, {331, std::string("DEAD")}});
            }
            dxfRW writer(filename.c_str());
            if (adapter.plannedOwner) {
                DRW_Block_Record block;
                block.handle = 0x500u;
                block.name = "*Model_Space";
                writer.setSourceBlockRecords({block});
            }
            adapter.dxfW = &writer;
            t.expect(writer.write(&adapter, DRW::AC1024, binary) && adapter.result,
                     "explicit bindings and deliberate no-map raw32/wide transport remain available");
            std::vector<TableOwnerRecord> records;
            t.expect(readTableOwnerRecords(path, binary, records, false, true),
                     "raw namespace boundary controls retain literal framing");
            const auto record = std::find_if(records.begin(), records.end(),
                [](const auto& value) { return value.kind == "SORTENTSTABLE"; });
            t.expect(record != records.end(), "raw namespace boundary record is emitted");
            if (record != records.end()) {
                std::vector<std::string> references, keys, owners;
                for (const auto& group : record->groups) {
                    if (group.first == 331) references.push_back(group.second);
                    if (group.first == 5) keys.push_back(group.second);
                    if (group.first == 330) owners.push_back(group.second);
                }
                if (transport < 2) {
                    t.expect(references == std::vector<std::string>{transport == 1 ? "100000001" : "DEAD"},
                             "no-map unresolved and wide entity references are raw transport only, not resolution evidence");
                } else if (transport == 2) {
                    t.expect(references.size() == 1 && hex(references.front()) == writer.remapEntityHandle(0xA001u),
                             "an explicit actual-entity binding is accepted during source identity remapping");
                } else {
                    t.expect(keys == std::vector<std::string>{"701", "601", "701", "601", "701"}
                                 && owners == std::vector<std::string>{"600", "1F", "500", "500"}
                                 && references.size() == 4 && references[1] == "DEAD"
                                 && references[3] == "DEAD",
                             "102 payloads and later100 subclasses leave body keys/owner roles narrowly bounded");
                }
            }
            adapter.dxfW = nullptr;
        }
    }
}

void testSortEntsCarrierAgreement(TestContext& t) {
    class AgreementInterface final : public dx_iface {
    public:
        void writeHeader(DRW_Header& header) override {
            dx_iface::writeHeader(header);
            dxfW->setHandleRemap({{0x601u, 0x701u}, {0x500u, 0x1Fu}});
            if (!sectionRoute)
                dxfW->setRootDictEntries({{"RESEARCH_SORT", "600"}});
            dxfW->setRawDxfSections(sectionRoute
                ? std::vector<DRW_RawDxfSection>{section} : std::vector<DRW_RawDxfSection>{});
        }
        void writeEntities() override {
            DRW_RawDxfObject line;
            line.name = "LINE";
            line.handle = 0xA000u;
            line.groups = {{5, std::string("A000")}, {330, std::string("1F")},
                {100, std::string("AcDbEntity")}, {8, std::string("0")},
                {100, std::string("AcDbLine")}, {10, 1.0}, {20, 0.0}, {30, 0.0},
                {11, 2.0}, {21, 0.0}, {31, 0.0}};
            // A legitimate active map with no numeric change must not let a
            // different raw 331 spelling evade actual-reference preflight.
            sourceWritten = dxfW->writeRawDxfObject(&line)
                && dxfW->bindSourceEntityHandle(0xA000u, 0xA000u);
        }
        void writeObjects() override {
            if (sectionRoute)
                return; // Namespace controls, not a reciprocal CAD graph.
            DRW_Dictionary dictionary;
            dictionary.handle = 0x600u;
            dictionary.parentHandle = 0xCu;
            dictionary.hardOwner = 1;
            dictionary.m_entries.push_back({"ACAD_SORTENTS", 0x601u});
            ownerWritten = dxfW->writeDictionary(&dictionary);
            before = dxfW->highWaterHandle();
            leafWritten = dxfW->writeRawDxfObject(&raw);
            after = dxfW->highWaterHandle();
        }
        DRW_RawDxfObject raw;
        DRW_RawDxfSection section;
        bool sectionRoute{false}, sourceWritten{false}, ownerWritten{false}, leafWritten{false};
        std::uint32_t before{0}, after{0};
    } adapter;
    const std::vector<DRW_Variant> groups{{5, std::string("601")}, {330, std::string("600")},
        {102, std::string("{HEADER_APP")}, {330, std::string("600")}, {102, std::string("}")},
        {100, std::string("AcDbSortentsTable")}, {330, std::string("1F")},
        {331, std::string("A000")}, {5, std::string("601")},
        {102, std::string("{BODY_APP")}, {330, std::string("600")}, {331, std::string("DEAD")},
        {5, std::string("601")}, {100, std::string("OtherSubclass")}, {102, std::string("}")},
        {100, std::string("OtherSubclass")}, {5, std::string("601")},
        {330, std::string("500")}, {331, std::string("DEAD")}};
    const auto spellings = [](const std::vector<DRW_Variant>& values) {
        std::vector<UTF8STRING> result;
        for (const auto& value : values) {
            std::string text = value.c_str();
            if (value.code() == 5 || value.code() == 330 || value.code() == 331) {
                for (char& ch : text) if (ch >= 'A' && ch <= 'F') ch += 'a' - 'A';
                text = "00" + text;
            }
            result.push_back(std::move(text));
        }
        return result;
    };
    const auto projection = [](const std::vector<DRW_Variant>& values) {
        std::vector<std::pair<int, std::string>> result;
        for (const auto& value : values) result.emplace_back(value.code(), value.c_str());
        return result;
    };
    std::vector<DRW_Variant> sectionGroups{{0, std::string("RESEARCH_BEFORE")},
        {5, std::string("DEAD")}, {100, std::string("OtherSubclass")}, {331, std::string("DEAD")},
        {0, std::string("SORTENTSTABLE")}};
    sectionGroups.insert(sectionGroups.end(), groups.begin(), groups.end());
    const std::size_t following = sectionGroups.size();
    sectionGroups.insert(sectionGroups.end(), {{0, std::string("RESEARCH_AFTER")},
        {5, std::string("DEAD")}, {100, std::string("OtherSubclass")}, {331, std::string("DEAD")}});
    auto sectionValues = spellings(sectionGroups);
    sectionValues[0] = "RESEARCH_BEFORE_RAW";
    sectionValues[1] = sectionValues[3] = "BEEF";
    sectionValues[2] = "AcDbSortentsTable";
    sectionValues[following + 1] = sectionValues[following + 3] = "BEEF";
    sectionValues[following + 2] = "AcDbSortentsTable";
    const auto path = std::filesystem::temp_directory_path() / ("libdxfrw-sort-agreement-"
        + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{path};
    const std::string filename = path.string();
    for (const bool binary : {false, true}) {
        for (int control = 0; control != 15; ++control) {
            dx_data source;
            adapter.cData = &source;
            adapter.currentBlock = source.mBlock;
            adapter.sectionRoute = control >= 11;
            adapter.raw = DRW_RawDxfObject{};
            adapter.raw.name = "SORTENTSTABLE";
            adapter.raw.handle = 0x601u;
            adapter.raw.m_version = DRW::AC1024;
            adapter.raw.groups = groups;
            adapter.raw.rawValues = spellings(groups);
            adapter.raw.hasRawValues = true;
            adapter.section = DRW_RawDxfSection{};
            adapter.section.m_name = "SORT_AGREEMENT_CONTROL";
            adapter.section.m_version = DRW::AC1024;
            adapter.section.m_groups = sectionGroups;
            adapter.section.m_rawValues = sectionValues;
            adapter.section.m_hasRawValues = true;
            switch (control) {
            case 1: adapter.raw.rawValues[0] = "602"; break;
            case 2: adapter.raw.rawValues[1] = "DEAD"; break;
            case 3: adapter.raw.rawValues[6] = "500"; break;
            case 4: adapter.raw.rawValues[7] = "DEAD"; break;
            case 5: adapter.raw.rawValues[8] = "DEAD"; break;
            case 6: adapter.raw.rawValues[5] = "OtherSubclass"; break;
            case 7: adapter.raw.groups[5] = DRW_Variant(100, std::string("OtherSubclass")); break;
            case 8: adapter.raw.rawValues[9] = "{OTHER_APP"; break;
            case 9: adapter.raw.rawValues[10] = "DEAD"; break;
            case 10: adapter.raw.rawValues[15] = "AcDbSortentsTable"; break;
            case 12: adapter.section.m_rawValues[4] = "RESEARCH_OTHER"; break;
            case 13: adapter.section.m_groups[4] = DRW_Variant(0, std::string("RESEARCH_OTHER")); break;
            case 14: adapter.section.m_rawValues[following] = "RESEARCH_AFTER_RAW"; break;
            default: break;
            }
            const auto callerGroups = projection(adapter.raw.groups);
            const auto callerValues = adapter.raw.rawValues;
            const auto callerSection = projection(adapter.section.m_groups);
            const auto callerSectionValues = adapter.section.m_rawValues;
            { std::ofstream original(path, std::ios::binary); original << "carrier destination preserved\n"; }
            dxfRW writer(filename.c_str());
            adapter.dxfW = &writer;
            const bool expected = binary ? control != 7 : control == 0 || control == 11;
            const bool wrote = writer.write(&adapter, DRW::AC1024, binary);
            t.expect(wrote == expected && adapter.sourceWritten,
                     "known SORT carrier disagreement rejects; binary rawValues remain inactive");
            t.expect(projection(adapter.raw.groups) == callerGroups && adapter.raw.rawValues == callerValues
                         && projection(adapter.section.m_groups) == callerSection
                         && adapter.section.m_rawValues == callerSectionValues,
                     "SORT agreement preflight preserves both caller representations");
            if (!expected) {
                if (!adapter.sectionRoute)
                    t.expect(!adapter.leafWritten && adapter.ownerWritten && adapter.before == adapter.after,
                             "SORT disagreement rejects without leaf allocator mutation");
                std::ifstream preserved(path, std::ios::binary);
                t.expect(std::string(std::istreambuf_iterator<char>(preserved), {})
                             == "carrier destination preserved\n",
                         "SORT disagreement preserves the existing destination");
                preserved.close();
                adapter.raw.groups = groups;
                adapter.raw.rawValues = spellings(groups);
                adapter.section.m_groups = sectionGroups;
                adapter.section.m_rawValues = sectionValues;
                t.expect(writer.write(&adapter, DRW::AC1024, binary),
                         "correcting only inconsistent SORT fields permits same-writer retry");
            }
            std::vector<TableOwnerRecord> records;
            t.expect(readTableOwnerRecords(path, binary, records), "SORT agreement controls retain literal framing");
            const auto record = std::find_if(records.begin(), records.end(),
                [](const auto& value) { return value.kind == "SORTENTSTABLE"; });
            if (!(binary && control == 13)) {
                t.expect(record != records.end() && groupValue(record->groups, 5) == "701",
                         "SORT header identity follows ordinary remapping");
                if (record != records.end()) {
                    const auto body = std::find(record->groups.begin(), record->groups.end(),
                        std::make_pair(100, std::string("AcDbSortentsTable")));
                    t.expect(body != record->groups.end() && std::distance(body, record->groups.end()) >= 4
                                 && *(body + 1) == std::make_pair(330, binary ? std::string("1F") : std::string("001f"))
                                 && *(body + 2) == std::make_pair(331, binary ? std::string("A000") : std::string("00a000"))
                                 && *(body + 3) == std::make_pair(5, binary ? std::string("601") : std::string("00601")),
                             "equivalent numeric spellings retain actual references and opaque sort keys");
                }
            }
            if (adapter.sectionRoute) {
                const auto next = std::find_if(records.begin(), records.end(),
                    [](const auto& value) { return value.kind == "RESEARCH_AFTER"; });
                t.expect(next != records.end() && groupValue(next->groups, 5) == (binary ? "DEAD" : "BEEF"),
                         "leaving known SORT resets agreement without tightening unrelated raw fields");
                const auto before = std::find_if(records.begin(), records.end(), [binary](const auto& value) {
                    return value.kind == (binary ? "RESEARCH_BEFORE" : "RESEARCH_BEFORE_RAW");
                });
                t.expect(before != records.end() && groupValue(before->groups, 5) == (binary ? "DEAD" : "BEEF"),
                         "unrelated raw record/marker spellings before SORT retain their existing policy");
            }
            adapter.dxfW = nullptr;
        }
    }
}

void testPlotCustomDpiAdmission(TestContext& t) {
    class PlotInterface final : public dx_iface {
    public:
        void writeHeader(DRW_Header& header) override {
            dx_iface::writeHeader(header);
            dxfW->setRootDictEntries({
                {layoutObject ? "ACAD_LAYOUT" : "ACAD_PLOTSETTINGS", "900"}});
        }

        void writeEntities() override {
            // Ordinary geometry remains in ENTITIES, not smuggled into
            // OBJECTS as a purported next-record framing control.
            for (const double x : {12.0, 34.0}) {
                DRW_Line line;
                line.basePoint = DRW_Coord(x, 2, 3);
                line.secPoint = DRW_Coord(x + 1, 4, 5);
                geometryWritten = dxfW->writeLine(&line) && geometryWritten;
            }
        }

        void writeObjects() override {
            // Make the record reachable through a defined named dictionary.
            // Reserving only the owner leaves the leaf reservation/allocation
            // itself observable at the typed writer's admission boundary.
            if (!dxfW->reserveHandle(0x900u))
                return;
            emittedHandle = inputHandle != 0 ? inputHandle : dxfW->highWaterHandle();
            DRW_Dictionary dictionary;
            dictionary.handle = 0x900u;
            dictionary.parentHandle = 0xCu;
            dictionary.hardOwner = 1;
            dictionary.m_entries.push_back({"DPI_CONTROL", emittedHandle});
            dictionaryWritten = dxfW->writeDictionary(&dictionary);
            before = dxfW->highWaterHandle();
            objectWritten = layoutObject ? dxfW->writeLayout(&layout)
                                         : dxfW->writePlotSettings(&plot);
            after = dxfW->highWaterHandle();
        }

        DRW_PlotSettings plot;
        DRW_Layout layout;
        bool layoutObject{false};
        std::uint32_t inputHandle{0}, emittedHandle{0}, before{0}, after{0};
        bool objectWritten{false}, dictionaryWritten{false}, geometryWritten{true};
    } adapter;

    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-plot-dpi-" + std::to_string(nonce) + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code error;
            std::filesystem::remove(path, error);
        }
    } cleanup{path};
    const std::string filename = path.string();
    const auto configure = [&t](auto& entry, int resolution, int dpi,
                                std::uint32_t handle) {
        entry.reset();
        entry.handle = handle;
        entry.parentHandle = 0x900u;
        entry.name = "DPI_CONTROL";
        entry.shadePlotResLevel = resolution;
        entry.shadePlotCustomDPI = dpi;
        entry.reactorHandles = {0x900u};
        entry.appData.push_back({{102, std::string("{DPI_META")},
            {1, std::string("caller-plot-metadata")}, {102, std::string("}")}});
        t.expect(entry.addExtData(std::make_unique<DRW_Variant>(
                     1001, std::string("DPI_APP")))
                     && entry.addExtData(std::make_unique<DRW_Variant>(
                         1000, std::string("trailing-dpi-metadata"))),
                 "plot DPI control registers its common metadata");
    };

    for (const DRW::Version version : {DRW::AC1015, DRW::AC1018, DRW::AC1024}) {
        for (const bool binary : {false, true}) {
            for (const bool layoutObject : {false, true}) {
                for (const int resolution : {2, 5}) {
                    for (const int dpi : {-1, 0, 99, 100, 32767, 32768}) {
                        for (const std::uint32_t handle : {0u, 0xA000u}) {
                            dx_data source;
                            DRW_AppId app;
                            app.name = "DPI_APP";
                            source.appIds.push_back(app);
                            adapter.cData = &source;
                            adapter.currentBlock = source.mBlock;
                            adapter.layoutObject = layoutObject;
                            adapter.inputHandle = handle;
                            adapter.objectWritten = false;
                            adapter.dictionaryWritten = false;
                            adapter.geometryWritten = true;
                            configure(adapter.plot, resolution, dpi, handle);
                            configure(adapter.layout, resolution, dpi, handle);
                            adapter.layout.paperSpaceBlockRecordHandle.ref = 0x1Eu;
                            const bool emitted = version > DRW::AC1015;
                            const bool rejected = emitted && resolution == 5
                                && (dpi < 100 || dpi > 32767);
                            {
                                std::ofstream original(path, std::ios::binary);
                                original << "existing plot destination\n";
                            }
                            dxfRW writer(filename.c_str());
                            adapter.dxfW = &writer;
                            const bool written = writer.write(&adapter, version, binary);
                            t.expect(written == !rejected && adapter.objectWritten == !rejected
                                         && adapter.geometryWritten && adapter.dictionaryWritten,
                                     "only emitted Custom DPI outside 100..32767 rejects");
                            const auto unchanged = [handle, resolution](const auto& entry, int expectedDpi) {
                                return entry.handle == handle && entry.parentHandle == 0x900u
                                    && entry.shadePlotResLevel == resolution
                                    && entry.shadePlotCustomDPI == expectedDpi
                                    && entry.name == "DPI_CONTROL"
                                    && entry.reactorHandles == std::vector<std::uint32_t>{0x900u}
                                    && entry.appData.size() == 1 && entry.appData.front().size() == 3
                                    && entry.extData.size() == 2 && entry.extData[1] != nullptr
                                    && entry.extData[1]->c_str() != nullptr
                                    && std::string(entry.extData[1]->c_str()) == "trailing-dpi-metadata";
                            };
                            t.expect(unchanged(adapter.plot, dpi) && unchanged(adapter.layout, dpi),
                                     "plot admission never modifies caller identity, fields or carriers");
                            int expectedDpi = dpi;
                            if (rejected) {
                                t.expect(adapter.before == adapter.after,
                                         "invalid Custom DPI cannot reserve or allocate a leaf handle");
                                std::ifstream preserved(path, std::ios::binary);
                                t.expect(std::string(std::istreambuf_iterator<char>(preserved), {})
                                             == "existing plot destination\n",
                                         "invalid Custom DPI leaves the existing destination byte-identical");
                                preserved.close();
                                adapter.plot.shadePlotCustomDPI = 100;
                                adapter.layout.shadePlotCustomDPI = 100;
                                expectedDpi = 100;
                                t.expect(writer.write(&adapter, version, binary) && adapter.objectWritten,
                                         "correcting only DPI permits exact same-writer same-model retry");
                                t.expect(unchanged(adapter.plot, 100) && unchanged(adapter.layout, 100),
                                         "corrected retry retains caller identity and common metadata");
                            }
                            adapter.dxfW = nullptr;

                            std::vector<TableOwnerRecord> records;
                            t.expect(readTableOwnerRecords(path, binary, records),
                                     "literal plot DPI record framing reaches the final EOF");
                            const std::string kind = layoutObject ? "LAYOUT" : "PLOTSETTINGS";
                            const auto record = std::find_if(records.begin(), records.end(),
                                [&kind](const auto& value) { return value.kind == kind; });
                            t.expect(record != records.end(), "accepted plot DPI writes its actual typed record");
                            // Inactive 32768 is deliberately NOT a valid-format or
                            // semantic-parity positive: preserve 7X's existing
                            // unsigned-bit-pattern policy without imposing the
                            // Custom minimum on inactive metadata.
                            const int wireDpi = binary && expectedDpi == 32768 ? -32768 : expectedDpi;
                            if (record != records.end()) {
                                t.expect(emitted
                                    ? groupValue(record->groups, 77) == std::to_string(resolution)
                                        && groupValue(record->groups, 78) == std::to_string(wireDpi)
                                    : !hasGroup(record->groups, 77) && !hasGroup(record->groups, 78),
                                    "literal active fields obey emitted-version policy; inactive overflow is legacy counterevidence");
                                t.expect(groupValue(record->groups, 5) == writer.toHexStrHandle(adapter.emittedHandle)
                                             && groupValue(record->groups, 1001) == "DPI_APP"
                                             && groupValue(record->groups, 1000) == "trailing-dpi-metadata"
                                             && hasGroup(record->groups, 102),
                                         "accepted plot DPI retains identity and trailing common carriers");
                            }

                            FixtureInterface reader;
                            dx_data decoded;
                            t.expect(reader.fileImport(filename, &decoded, false),
                                     "accepted plot DPI reads through public document callbacks");
                            const auto verify = [&t, emitted, resolution, wireDpi, &adapter](const auto& values) {
                                t.expect(values.size() == 1, "plot DPI callback is delivered exactly once");
                                if (values.size() != 1)
                                    return;
                                const auto& value = values.front();
                                t.expect(value.shadePlotResLevel == (emitted ? resolution : 0)
                                             && value.shadePlotCustomDPI == (emitted ? wireDpi : 0)
                                             && value.handle == adapter.emittedHandle
                                             && value.parentHandle == 0x900u
                                             && value.reactorHandles == std::vector<std::uint32_t>{0x900u}
                                             && value.extData.size() == 2,
                                         "public plot callback preserves stored values, owner and common metadata");
                            };
                            if (layoutObject)
                                verify(reader.layouts);
                            else
                                verify(reader.plotSettings);
                            const auto owner = std::find_if(reader.dictionaries.begin(), reader.dictionaries.end(),
                                [](const auto& value) { return value.handle == 0x900u; });
                            t.expect(owner != reader.dictionaries.end() && owner->parentHandle == 0xCu
                                         && owner->m_entries.size() == 1
                                         && owner->m_entries.front().m_handle == adapter.emittedHandle,
                                     "plot DPI control has a defined root-owned dictionary edge");
                            t.expect(decoded.mBlock->ent.size() == 2
                                         && decoded.mBlock->ent.back()->eType == DRW::LINE
                                         && static_cast<const DRW_Line*>(decoded.mBlock->ent.back())->basePoint.x == 34,
                                     "plot DPI admission preserves the suffix LINE's actual geometry");
                        }
                    }
                }
            }
        }
    }

    // Typed writer admission is not reader validation. Feed stored group
    // values directly to the existing DXF parsers, including invalid active
    // API values, to protect tolerant import from an accidental shared guard.
    class PlotParser final : public DRW_PlotSettings {
    public:
        using DRW_PlotSettings::parseCode;
    };
    class LayoutParser final : public DRW_Layout {
    public:
        using DRW_Layout::parseCode;
    };
    for (const bool binary : {false, true}) {
        for (const int resolution : {2, 5}) {
            for (const int dpi : {-1, 0, 99, 100, 32767, 32768}) {
                std::ostringstream bytes;
                std::unique_ptr<dxfWriter> tags;
                if (binary)
                    tags = std::make_unique<dxfWriterBinary>(&bytes);
                else
                    tags = std::make_unique<dxfWriterAscii>(&bytes);
                t.expect(tags->writeString(100, "AcDbPlotSettings")
                             && tags->writeInt16(77, resolution)
                             && tags->writeInt16(78, dpi),
                         "stored plot DPI control encodes literal DXF fields");
                std::istringstream input(bytes.str());
                std::unique_ptr<dxfReader> reader;
                if (binary)
                    reader = std::make_unique<dxfReaderBinary>(&input);
                else
                    reader = std::make_unique<dxfReaderAscii>(&input);
                PlotParser plot;
                LayoutParser layout;
                int code = 0;
                int count = 0;
                while (reader->readRec(&code)) {
                    t.expect(plot.parseCode(code, reader) && layout.parseCode(code, reader),
                             "DXF plot and layout import stay tolerant of stored active/inactive DPI");
                    ++count;
                }
                const int storedDpi = binary && dpi == 32768 ? -32768 : dpi;
                t.expect(count == 3 && plot.shadePlotResLevel == resolution
                             && layout.shadePlotResLevel == resolution
                             && plot.shadePlotCustomDPI == storedDpi
                             && layout.shadePlotCustomDPI == storedDpi,
                         "DXF parsers retain stored DPI instead of clamping, defaulting or discarding");
            }
        }
    }
}

std::vector<std::string> tableRecordOwners(const TableOwnerRecord& record) {
    std::vector<std::string> owners;
    int appDepth = 0;
    for (const auto& group : record.groups) {
        if (group.first == 102) {
            if (!group.second.empty() && group.second.front() == '{')
                ++appDepth;
            else if (group.second == "}" && appDepth > 0)
                --appDepth;
        } else if (group.first == 330 && appDepth == 0) {
            owners.push_back(group.second);
        }
    }
    return owners;
}

void testTableRecordOwners(TestContext& t) {
    const TableOwnerRecord scopedControl{
        "STYLE", {{330, "3"}, {102, "{ACAD_REACTORS"}, {330, "2"},
                  {330, "0"}, {102, "}"}}};
    t.expect(tableRecordOwners(scopedControl)
                 == std::vector<std::string>{"3"},
             "table owner extraction excludes application pointers and nulls");
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch()
                           .count();
    const std::array<DRW::Version, 4> versions{
        DRW::AC1014, DRW::AC1015, DRW::AC1021, DRW::AC1032};
    for (const DRW::Version version : versions) {
        for (const bool binary : {false, true}) {
            const auto path = std::filesystem::temp_directory_path()
                / ("libdxfrw-table-owners-" + std::to_string(nonce) + "-"
                   + std::to_string(static_cast<int>(version))
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            struct Cleanup {
                std::filesystem::path path;
                ~Cleanup() {
                    std::error_code error;
                    std::filesystem::remove(path, error);
                }
            } cleanup{path};
            const bool written = generateTableOwnerDxf(path, version, binary);
            t.expect(written, "table owner sentinel writes from scratch");
            if (!written)
                continue;
            std::vector<TableOwnerRecord> records;
            const bool decoded = readTableOwnerRecords(path, binary, records);
            t.expect(decoded, "table owner raw framing reaches EOF");
            if (!decoded)
                continue;
            std::map<std::string, std::string> tableHandles;
            std::map<std::string, std::size_t> recordCounts;
            std::string table;
            for (const auto& record : records) {
                if (record.kind == "TABLE") {
                    table = groupValue(record.groups, 2);
                    tableHandles[table] = groupValue(record.groups, 5);
                    t.expect(!tableHandles[table].empty(),
                             "each emitted table has an identity");
                    const auto owners = tableRecordOwners(record);
                    t.expect(version == DRW::AC1014
                                 ? (owners.empty()
                                    || owners == std::vector<std::string>{"0"})
                                 : owners == std::vector<std::string>{"0"},
                             "TABLE control root is null, not an entry owner");
                } else if (record.kind == "ENDTAB") {
                    table.clear();
                } else if (!table.empty() && record.kind == table) {
                    ++recordCounts[table];
                    const auto owners = tableRecordOwners(record);
                    // Existing R14 output omits owners for BLOCK_RECORD and
                    // APPID. Keep that version policy in
                    // this narrow correction; every emitted owner must agree.
                    const bool legacyOwnerOmitted = version == DRW::AC1014
                        && (table == "BLOCK_RECORD" || table == "APPID");
                    t.expect(legacyOwnerOmitted
                                 ? owners.empty()
                                 : owners == std::vector<std::string>{
                                       tableHandles[table]},
                             "table record owner equals its emitted TABLE");
                    const int selfCode = table == "DIMSTYLE" ? 105 : 5;
                    t.expect(!groupValue(record.groups, selfCode).empty(),
                             "table entry uses its correct self handle code");
                    if (table == "STYLE"
                        && groupValue(record.groups, 2) == "OWNER_STYLE") {
                        t.expect(std::find(record.groups.begin(),
                                           record.groups.end(),
                                           std::make_pair(330,
                                                          std::string("12")))
                                     != record.groups.end(),
                                 "STYLE reactor remains distinct from its owner");
                    }
                }
            }
            t.expect(tableHandles.size() == 9 && recordCounts.size() == 9,
                     "owner matrix covers every emitted table family");
            const std::map<std::string, std::size_t> expectedCounts{
                {"VPORT", 2}, {"LTYPE", 4}, {"LAYER", 2}, {"STYLE", 2},
                {"VIEW", 1}, {"UCS", 1}, {"APPID", 2}, {"DIMSTYLE", 2},
                {"BLOCK_RECORD", 3}};
            t.expect(recordCounts == expectedCounts,
                     "custom and default table records are all retained");
        }
    }
}

std::string groupValue(const std::vector<std::pair<int, std::string>>& groups,
                       int code) {
    for (const auto& group : groups) {
        if (group.first == code)
            return group.second;
    }
    return {};
}

const DRW_Text* findText(const dx_data& data) {
    if (data.mBlock == nullptr)
        return nullptr;
    for (const DRW_Entity* entity : data.mBlock->ent) {
        if (entity != nullptr && entity->eType == DRW::TEXT)
            return static_cast<const DRW_Text*>(entity);
    }
    return nullptr;
}

void testBig5(TestContext& t) {
    FixtureInterface interface_;
    dx_data data;
    t.expect(importFixture("big5_traditional.dxf", interface_, data),
             "Big5 fixture imports");
    const DRW_Layer* layer = findNonDefaultLayer(data);
    t.expect(layer != nullptr && layer->name ==
                 "\xE5\x9C\x96\xE5\xB1\xA4",
             "Big5 layer name decodes to UTF-8");
    const DRW_Text* text = findText(data);
    t.expect(text != nullptr && text->text ==
                 "\xE4\xB8\xAD\xE6\x96\x87\xE6\xB8\xAC\xE8\xA9\xA6",
             "Big5 text decodes to UTF-8");
}

void testBig5Hkscs(TestContext& t) {
    FixtureInterface interface_;
    dx_data data;
    t.expect(importFixture("big5_hkscs.dxf", interface_, data),
             "Big5-HKSCS fixture imports");
    const DRW_Layer* layer = findNonDefaultLayer(data);
    t.expect(layer != nullptr && layer->name ==
                 "\xE9\xBE\x98\xE4\x92\x91",
             "Big5-HKSCS layer name decodes to UTF-8");
    const DRW_Text* text = findText(data);
    t.expect(text != nullptr && text->text ==
                 "\xC3\x8A\xCC\x84\xE4\x92\x91\xF0\xA0\x80\xA1",
             "Big5-HKSCS pair and supplementary text decode to UTF-8");
}

void testUhc(TestContext& t) {
    FixtureInterface interface_;
    dx_data data;
    t.expect(importFixture("uhc_korean.dxf", interface_, data),
             "UHC fixture imports");
    const DRW_Layer* layer = findNonDefaultLayer(data);
    t.expect(layer != nullptr && layer->name ==
                 "\xEB\x8F\x84\xEB\xA9\xB4",
             "UHC layer name decodes to UTF-8");
    const DRW_Text* text = findText(data);
    t.expect(text != nullptr && text->text ==
                 "\xED\x95\x9C\xEA\xB8\x80\xEB\x8F\x84\xEB\xA9\xB4",
             "UHC text decodes to UTF-8");
}

void testBlockPreview(TestContext& t) {
    FixtureInterface interface_;
    dx_data data;
    t.expect(importFixture("block_record_preview_r2007.dxf", interface_, data),
             "BLOCK_RECORD preview fixture imports");
    bool found = false;
    for (const dx_ifaceBlock* block : data.blocks) {
        if (block != nullptr && block->name == "PREVIEW_BLOCK")
            found = true;
    }
    t.expect(found, "BLOCK_RECORD preview retains the named block");
}

void testBlockRecordXdata(TestContext& t) {
    std::ifstream input(fixturePath("block_record_preview_r2007.dxf"));
    const std::string marker = "2\nPREVIEW_BLOCK\n";
    const std::string xdata = "1001\nQCAD\n1000\nCAMTOOLPATH\n";
    std::string contents((std::istreambuf_iterator<char>(input)),
                         std::istreambuf_iterator<char>());
    const std::size_t markerPosition = contents.find(marker);
    t.expect(markerPosition != std::string::npos,
             "BLOCK_RECORD XDATA test locates its source record");
    if (markerPosition == std::string::npos)
        return;
    contents.insert(markerPosition, xdata);

    FixtureInterface interface_;
    dx_data data;
    t.expect(importTemporaryDxf("block-record-xdata-test", contents,
                                interface_, data),
             "BLOCK_RECORD XDATA fixture imports");

    const DRW_Block_Record* record = nullptr;
    for (const DRW_Block_Record& candidate : interface_.blockRecords) {
        if (candidate.name == "PREVIEW_BLOCK") {
            record = &candidate;
            break;
        }
    }
    bool foundApplication = false;
    bool foundText = false;
    if (record != nullptr) {
        for (const DRW_Variant* value : record->extData) {
            if (value == nullptr)
                continue;
            if (value->code() == 1001 && value->type() == DRW_Variant::STRING)
                foundApplication = std::string(value->c_str()) == "QCAD";
            if (value->code() == 1000 && value->type() == DRW_Variant::STRING)
                foundText = std::string(value->c_str()) == "CAMTOOLPATH";
        }
    }
    t.expect(record != nullptr,
             "BLOCK_RECORD XDATA publishes the named table entry");
    t.expect(foundApplication && foundText,
             "BLOCK_RECORD XDATA preserves application and text items");
}

std::vector<DRW_Variant> orderedPointXdata() {
    return {{1001, std::string("POINTS_A")},
            {1010, DRW_Coord(1, 2, 3)}, {1010, DRW_Coord(4, 5, 6)},
            {1000, std::string("sentinel")},
            {1011, DRW_Coord(7, 8, 9)}, {1012, DRW_Coord(-2, 4, 8)},
            {1013, DRW_Coord(0, 1, 0)},
            {1001, std::string("POINTS_B")},
            {1010, DRW_Coord(11, 12, 13)}, {1010, DRW_Coord(14, 15, 16)}};
}

const DRW_Variant* xdataPointer(const DRW_Variant* value) { return value; }
const DRW_Variant* xdataPointer(const std::shared_ptr<DRW_Variant>& value) {
    return value.get();
}

template<class Values>
bool equalPointXdata(const Values& actual,
                     const std::vector<DRW_Variant>& expected) {
    if (actual.size() != expected.size())
        return false;
    for (std::size_t index = 0; index < expected.size(); ++index) {
        const DRW_Variant* value = xdataPointer(actual[index]);
        const DRW_Variant& wanted = expected[index];
        if (value == nullptr || value->type() != wanted.type()
            || value->code() != wanted.code())
            return false;
        if (wanted.type() == DRW_Variant::STRING) {
            if (std::string(value->c_str()) != wanted.c_str())
                return false;
        } else if (wanted.type() == DRW_Variant::COORD) {
            const DRW_Coord& a = *value->coord();
            const DRW_Coord& b = *wanted.coord();
            if (a.x != b.x || a.y != b.y || a.z != b.z)
                return false;
        } else {
            return false;
        }
    }
    return true;
}

class TablePointParser : public DRW_Layer {
public:
    bool parseValue(int code, const std::unique_ptr<dxfReader>& reader) {
        return DRW_Layer::parseCode(code, reader);
    }
    void resetValues() { DRW_TableEntry::reset(); }
};

class EntityPointParser : public DRW_Line {
public:
    bool parseValue(int code, const std::unique_ptr<dxfReader>& reader) {
        return DRW_Line::parseCode(code, reader);
    }
    void resetValues() { DRW_Entity::reset(); }
};

// These are physical wire tags, not a semantic writer round-trip: coordinate
// component order, code families and interrupted tuples remain literal.
template<class Parser>
bool parsePointTags(Parser& parser, const std::vector<DRW_Variant>& tags,
                    bool binary, const std::string& version = "AC1032",
                    const std::string& codepage = "ANSI_1252") {
    std::ostringstream bytes;
    std::unique_ptr<dxfWriter> writer;
    if (binary)
        writer = std::make_unique<dxfWriterBinary>(&bytes);
    else
        writer = std::make_unique<dxfWriterAscii>(&bytes);
    for (const DRW_Variant& tag : tags) {
        if (tag.type() == DRW_Variant::STRING) {
            if (!writer->writeString(tag.code(), tag.c_str()))
                return false;
        } else if (tag.type() == DRW_Variant::DOUBLE) {
            if (!writer->writeDouble(tag.code(), tag.d_val()))
                return false;
        } else if (tag.type() == DRW_Variant::INTEGER) {
            if (!writer->writeInt16(tag.code(), tag.i_val()))
                return false;
        } else if (tag.type() == DRW_Variant::COORD) {
            const DRW_Coord& point = *tag.coord();
            if (!writer->writeDouble(tag.code(), point.x)
                || !writer->writeDouble(tag.code() + 10, point.y)
                || !writer->writeDouble(tag.code() + 20, point.z))
                return false;
        } else {
            return false;
        }
    }
    std::istringstream input(bytes.str());
    std::unique_ptr<dxfReader> reader;
    if (binary)
        reader = std::make_unique<dxfReaderBinary>(&input);
    else
        reader = std::make_unique<dxfReaderAscii>(&input);
    reader->setVersion(version, true);
    reader->setCodePage(codepage);
    int code = 0;
    while (reader->readRec(&code)) {
        if (!parser.parseValue(code, reader))
            return false;
    }
    return true;
}

bool generatePointXdataDxf(const std::filesystem::path& path, bool binary) {
    dx_data data;
    for (const char* name : {"POINTS_A", "POINTS_B"}) {
        DRW_AppId app;
        app.name = name;
        data.appIds.push_back(app);
    }
    DRW_Layer layer;
    layer.name = "XDATA_POINTS";
    layer.color = 3;
    DRW_Textstyle style;
    style.name = "XDATA_STYLE";
    style.font = "txt";
    auto line = std::make_unique<DRW_Line>();
    line->layer = layer.name;
    line->basePoint = DRW_Coord(1, 2, 3);
    line->secPoint = DRW_Coord(5, 8, 13);
    for (const DRW_Variant& value : orderedPointXdata()) {
        if (!layer.addExtData(std::make_unique<DRW_Variant>(value))
            || !style.addExtData(std::make_unique<DRW_Variant>(value)))
            return false;
        line->extData.push_back(std::make_shared<DRW_Variant>(value));
    }
    data.layers.push_back(layer);
    data.textStyles.push_back(style);
    data.mBlock->ent.push_back(line.release());
    FixtureInterface interface_;
    interface_.attach(&data);
    return interface_.fileExport(path.string(), DRW::AC1032, binary, &data, false);
}

template<class Parser>
void testPointCursorTolerance(TestContext& t, bool binary) {
    Parser parser;
    const std::vector<DRW_Variant> unordered{
        {1021, 2.0}, {1031, 3.0}, {1011, 1.0},
        {1032, 6.0}, {1022, 5.0}, {1012, 4.0}};
    t.expect(parsePointTags(parser, unordered, binary)
                 && equalPointXdata(parser.extData,
                     {{1011, DRW_Coord(1, 2, 3)}, {1012, DRW_Coord(4, 5, 6)}}),
             "out-of-order XDATA components retain their coordinate family");

    // Malformed sources are tolerance controls, not valid-input parity cells.
    parser.resetValues();
    const std::vector<DRW_Variant> interrupted{
        {1010, 1.0}, {1021, 2.0}, {1031, 3.0},
        {1001, std::string("NEXT_APP")}, {1031, 4.0},
        {1000, std::string("boundary")}, {1021, 5.0},
        {1011, 6.0}, {1021, 7.0}, {1031, 8.0},
        {1012, 9.0}, {1012, 10.0}, {1022, 11.0}, {1032, 12.0},
        {1010, 13.0}, {10, 80.0}, {1020, 14.0},
        {62, 3}, {1030, 15.0}};
    const std::vector<DRW_Variant> preserved{
        {1010, DRW_Coord(1, 0, 0)}, {1011, DRW_Coord(0, 2, 3)},
        {1001, std::string("NEXT_APP")}, {1011, DRW_Coord(0, 0, 4)},
        {1000, std::string("boundary")}, {1011, DRW_Coord(6, 5, 0)},
        {1011, DRW_Coord(0, 7, 8)}, {1012, DRW_Coord(9, 0, 0)},
        {1012, DRW_Coord(10, 11, 12)}, {1010, DRW_Coord(13, 0, 0)},
        {1010, DRW_Coord(0, 14, 0)}, {1010, DRW_Coord(0, 0, 15)}};
    t.expect(parsePointTags(parser, interrupted, binary)
                 && equalPointXdata(parser.extData, preserved),
             "incomplete/mixed/duplicate points never mutate earlier tuples or APPIDs");
    parser.resetValues();
    t.expect(parsePointTags(parser, {{1010, 90.0}}, binary),
             "truncated point remains tolerated");
    parser.resetValues();
    t.expect(parsePointTags(parser, {{1033, 3.0}, {1023, 2.0}, {1013, 1.0}}, binary)
                 && equalPointXdata(parser.extData, {{1013, DRW_Coord(1, 2, 3)}}),
             "reset clears incomplete XDATA cursor before operation reuse");
}

void testPointCursorCopies(TestContext& t, bool binary) {
    TablePointParser original;
    t.expect(parsePointTags(original, {{1010, 1.0}, {1020, 2.0}}, binary),
             "partial table point parses before copy");
    TablePointParser copied(original);
    TablePointParser assigned;
    assigned = original;
    original.resetValues();
    t.expect(parsePointTags(copied, {{1030, 3.0}}, binary)
                 && equalPointXdata(copied.extData,
                     {{1010, DRW_Coord(1, 2, 0)}, {1010, DRW_Coord(0, 0, 3)}})
                 && parsePointTags(assigned, {{1030, 4.0}}, binary)
                 && equalPointXdata(assigned.extData,
                     {{1010, DRW_Coord(1, 2, 0)}, {1010, DRW_Coord(0, 0, 4)}}),
             "copied table data owns its partial tuple without crossing read operations");
    TablePointParser moved;
    t.expect(parsePointTags(moved, {{1012, 5.0}}, binary),
             "partial table point parses before move");
    TablePointParser destination(std::move(moved));
    moved.resetValues();
    t.expect(parsePointTags(destination, {{1032, 7.0}, {1022, 6.0}}, binary)
                 && equalPointXdata(destination.extData,
                     {{1012, DRW_Coord(5, 0, 0)}, {1012, DRW_Coord(0, 6, 7)}}),
             "moved table data cannot carry its point cursor into a new reader");
    EntityPointParser entity;
    t.expect(parsePointTags(entity, {{1010, 8.0}}, binary),
             "partial entity point parses before copy");
    EntityPointParser entityCopy(entity);
    entity.resetValues();
    t.expect(parsePointTags(entityCopy, {{1030, 9.0}}, binary)
                 && equalPointXdata(entityCopy.extData,
                     {{1010, DRW_Coord(8, 0, 0)}, {1010, DRW_Coord(0, 0, 9)}}),
             "entity copy discards transient cursor without aliasing source data");
}

void testOrderedPointXdata(TestContext& t) {
    const auto expected = orderedPointXdata();
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    for (const bool binary : {false, true}) {
        testPointCursorTolerance<TablePointParser>(t, binary);
        testPointCursorTolerance<EntityPointParser>(t, binary);
        testPointCursorCopies(t, binary);
        TablePointParser table;
        EntityPointParser entity;
        t.expect(parsePointTags(table, expected, binary)
                     && equalPointXdata(table.extData, expected),
                 "table XDATA retains seven ordered points and two APPIDs");
        t.expect(parsePointTags(entity, expected, binary)
                     && equalPointXdata(entity.extData, expected),
                 "entity XDATA retains seven ordered points and two APPIDs");

        const auto path = std::filesystem::temp_directory_path()
            / ("libdxfrw-ordered-xdata-" + std::to_string(nonce)
               + (binary ? "-binary.dxf" : "-ascii.dxf"));
        std::error_code error;
        const bool written = generatePointXdataDxf(path, binary);
        t.expect(written, "ordered XDATA source writes from scratch");
        FixtureInterface interface_;
        dx_data imported;
        const bool read = written
            && interface_.fileImport(path.string(), &imported, false);
        t.expect(read, "ordered XDATA source imports");
        if (read) {
            const DRW_Layer* layer = findNonDefaultLayer(imported);
            t.expect(layer != nullptr && equalPointXdata(layer->extData, expected),
                     "LAYER callback retains ordered XDATA point roles");
            bool styleFound = false;
            for (const DRW_Textstyle& style : imported.textStyles) {
                if (style.name == "XDATA_STYLE")
                    styleFound = equalPointXdata(style.extData, expected);
            }
            t.expect(styleFound, "STYLE callback inherits ordered XDATA fix");
            const DRW_Entity* line = imported.mBlock->ent.empty()
                ? nullptr : imported.mBlock->ent.front();
            t.expect(line != nullptr && equalPointXdata(line->extData, expected),
                     "LINE callback remains an unaffected ordered control");
        }
        std::filesystem::remove(path, error);
    }
}

void testXdataTextCodecs(TestContext& t) {
    struct TextCase {
        std::string codepage;
        std::string legacy;
        std::string utf8;
    };
    const std::array<TextCase, 2> cases{{
        {"ANSI_1252", std::string("caf\xE9"), u8"café"},
        {"ANSI_1251", std::string("\xC6\xB8\xEB\xF2\xFB\xE9"), u8"Жёлтый"}}};
    for (const auto& item : cases) {
        for (const bool modern : {false, true}) {
            const std::string source = modern ? item.utf8 : item.legacy;
            const std::vector<DRW_Variant> physical{
                {1001, std::string("STRINGS")}, {1000, source + "\\U+4E00"},
                {1003, source}, {1002, std::string("{")},
                {1004, std::string("00E9FF")}, {1005, std::string("0")},
                {1002, std::string("}")}};
            const std::vector<DRW_Variant> semantic{
                {1001, std::string("STRINGS")}, {1000, item.utf8 + u8"一"},
                {1003, item.utf8}, {1002, std::string("{")},
                {1004, std::string("00E9FF")}, {1005, std::string("0")},
                {1002, std::string("}")}};
            for (const bool binary : {false, true}) {
                TablePointParser table;
                EntityPointParser entity;
                const std::string version = modern ? "AC1021" : "AC1018";
                t.expect(parsePointTags(table, physical, binary, version, item.codepage)
                             && equalPointXdata(table.extData, semantic),
                         "table XDATA decodes source text once by DXF version/codepage");
                t.expect(parsePointTags(entity, physical, binary, version, item.codepage)
                             && equalPointXdata(entity.extData, semantic),
                         "entity XDATA decodes text without changing control/binary/handles");
                std::ostringstream bytes;
                std::unique_ptr<dxfWriter> writer;
                if (binary)
                    writer = std::make_unique<dxfWriterBinary>(&bytes);
                else
                    writer = std::make_unique<dxfWriterAscii>(&bytes);
                writer->setVersion(version, true);
                writer->setCodePage(item.codepage);
                t.expect(writer->writeUtf8String(1, item.utf8 + u8"一")
                             && writer->writeUtf8String(1000, item.utf8 + u8"一"),
                         "ordinary TEXT and XDATA use the same semantic UTF8 encoding contract");
                std::istringstream input(bytes.str());
                std::unique_ptr<dxfReader> reader;
                if (binary)
                    reader = std::make_unique<dxfReaderBinary>(&input);
                else
                    reader = std::make_unique<dxfReaderAscii>(&input);
                const std::string encoded = modern ? item.utf8 + u8"一"
                    : item.legacy + "\\U+4E00";
                for (const int wanted : {1, 1000}) {
                    int code = 0;
                    t.expect(reader->readRec(&code) && code == wanted
                                 && reader->getRawValue() == encoded,
                             "emitted text bytes follow revision rather than legacy modern header codepage");
                }
            }
        }
    }
}

void testNamedColorCodecs(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    unsigned index = 0;
    for (const bool modern : {false, true}) {
        for (const bool binary : {false, true}) {
            dx_data data;
            data.headerC.addStr("$DWGCODEPAGE", "ANSI_1251", 3);
            auto line = std::make_unique<DRW_Line>();
            line->secPoint.x = 1;
            line->colorName = u8"BOOK$Ж";
            line->color24 = 0x123456;
            data.mBlock->ent.push_back(line.release());
            const auto path = std::filesystem::temp_directory_path()
                / ("libdxfrw-color-codec-" + std::to_string(nonce)
                   + "-" + std::to_string(index++) + ".dxf");
            dx_iface writer;
            t.expect(writer.fileExport(path.string(), modern ? DRW::AC1021 : DRW::AC1018,
                                       binary, &data, false), "named color writes through versioned codec");
            std::ifstream stream(path, std::ios::binary);
            if (binary) stream.seekg(22);
            std::unique_ptr<dxfReader> tags = binary
                ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&stream))
                : std::unique_ptr<dxfReader>(new dxfReaderAscii(&stream));
            int code = 0;
            bool found = false;
            while (tags->readRec(&code)) {
                if (code == 430) {
                    found = true;
                    t.expect(tags->getRawValue() == (modern ? u8"BOOK$Ж" : "BOOK$\xC6"),
                             "named color literal bytes follow target revision, not legacy metadata");
                }
            }
            t.expect(found, "named color raw control is present");
            tags.reset(); stream.close();
            dx_data read;
            dx_iface reader;
            const bool imported = reader.fileImport(path.string(), &read, false);
            t.expect(imported && read.mBlock->ent.size() == 1
                         && read.mBlock->ent.front()->colorName == u8"BOOK$Ж",
                     "typed named color decodes exactly once");
            std::error_code error; std::filesystem::remove(path, error);
        }
    }
}

void testRawXdataTextBytes(TestContext& t) {
    class RawTextWriter final : public dx_iface {
    public:
        DRW_RawDxfObject value;
        bool wrote = false;
        void writeEntities() override { wrote = dxfW->writeRawDxfObject(&value); }
    };
    const auto textTagsMatch = [](const DRW_RawDxfObject& carrier,
                                  const std::string& legacy) {
        std::vector<std::pair<int, std::string>> values;
        for (const auto& group : carrier.groups) {
            if (group.code() == 1000 || group.code() == 1003) {
                if (group.type() != DRW_Variant::STRING)
                    return false;
                values.emplace_back(group.code(), group.c_str());
            }
        }
        return values == std::vector<std::pair<int, std::string>>{
            {1000, legacy + "\\U+4E00"}, {1003, legacy}};
    };
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    for (const bool binary : {false, true}) {
        std::ostringstream bytes;
        if (binary)
            bytes.write("AutoCAD Binary DXF\r\n\x1A\0", 22);
        std::unique_ptr<dxfWriter> writer;
        if (binary)
            writer = std::make_unique<dxfWriterBinary>(&bytes);
        else
            writer = std::make_unique<dxfWriterAscii>(&bytes);
        const std::string legacy("\xC6\xB8\xEB\xF2\xFB\xE9");
        const std::vector<std::pair<int, std::string>> fields{
            {0, "SECTION"}, {2, "HEADER"}, {9, "$ACADVER"}, {1, "AC1018"},
            {9, "$DWGCODEPAGE"}, {3, "ANSI_1251"}, {0, "ENDSEC"},
            {0, "SECTION"}, {2, "ENTITIES"}, {0, "LOCAL_RAW_TEXT"},
            {5, "A0"}, {1001, "STRINGS"}, {1000, legacy + "\\U+4E00"},
            {1003, legacy}, {1002, "{"}, {1004, "00E9FF"}, {1005, "0"},
            {1002, "}"}, {0, "ENDSEC"}, {0, "EOF"}};
        bool written = true;
        for (const auto& field : fields)
            written = writer->writeString(field.first, field.second) && written;
        writer.reset();
        const auto path = std::filesystem::temp_directory_path()
            / ("libdxfrw-raw-xdata-text-" + std::to_string(nonce)
               + (binary ? "-binary.dxf" : "-ascii.dxf"));
        { std::ofstream stream(path, std::ios::binary); stream << bytes.str(); }
        FixtureInterface interface_;
        dx_data data;
        const bool read = written && interface_.fileImport(path.string(), &data, false);
        t.expect(read && interface_.rawEntities.size() == 1,
                 "raw XDATA encoding carrier imports without semantic coercion");
        if (read && interface_.rawEntities.size() == 1) {
            const auto& carrier = interface_.rawEntities.front();
            t.expect(textTagsMatch(carrier, legacy),
                     "raw XDATA retains exactly one ordered 1000/1003 pair and literal source bytes");
            for (const int missing : {1000, 1003}) {
                DRW_RawDxfObject mutation(carrier);
                mutation.groups.erase(std::remove_if(mutation.groups.begin(),
                    mutation.groups.end(), [missing](const DRW_Variant& value) {
                        return value.code() == missing;
                    }), mutation.groups.end());
                t.expect(!textTagsMatch(mutation, legacy),
                         "raw text retention predicate rejects a missing semantic tag");
            }
            DRW_RawDxfObject duplicate(carrier);
            duplicate.groups.emplace_back(1000, legacy + "\\U+4E00");
            t.expect(!textTagsMatch(duplicate, legacy),
                     "raw text retention predicate rejects duplicate semantic tags");
            DRW_RawDxfObject reordered(carrier);
            std::reverse(reordered.groups.begin(), reordered.groups.end());
            t.expect(!textTagsMatch(reordered, legacy),
                     "raw text retention predicate rejects reversed semantic tag order");
            for (const bool outputBinary : {false, true}) {
                const auto output = std::filesystem::path(path.string()
                    + (outputBinary ? "-to-binary.dxf" : "-to-ascii.dxf"));
                RawTextWriter replay;
                replay.value = interface_.rawEntities.front();
                dx_data outputData;
                outputData.headerC.addStr("$DWGCODEPAGE", "ANSI_1251", 3);
                const bool exported = replay.fileExport(output.string(), DRW::AC1018,
                    outputBinary, &outputData, false);
                FixtureInterface rereader;
                dx_data reread;
                t.expect(exported && replay.wrote
                             && rereader.fileImport(output.string(), &reread, false)
                             && rereader.rawEntities.size() == 1,
                         "version-bound raw carrier replays across both wire encodings");
                if (rereader.rawEntities.size() == 1) {
                    t.expect(textTagsMatch(rereader.rawEntities.front(), legacy),
                             "raw replay retains exact semantic tag count/order and source bytes");
                }
                std::error_code error;
                std::filesystem::remove(output, error);
            }
        }
        std::error_code error;
        std::filesystem::remove(path, error);
    }
}

class HeaderOrderCapture final : public dx_iface {
public:
    DRW_Header value;
    std::vector<std::string> texts;
    std::vector<DRW_Line> lines;
    int headerCount = 0;
    void addHeader(const DRW_Header* data) override {
        value = *data;
        ++headerCount;
    }
    void addText(const DRW_Text& data) override { texts.push_back(data.text); }
    void addLine(const DRW_Line& data) override { lines.push_back(data); }
};

class CapturedLinearDimension : public DRW_DimLinear {
public:
    explicit CapturedLinearDimension(const DRW_DimLinear& value)
        : DRW_DimLinear(value) {}

    DRW_Coord auxiliaryPoint15() const { return getPt5(); }
};

class LegacyDimensionOrderCapture final : public dx_iface {
public:
    LegacyDimensionOrderCapture() {
        cData = &data_;
        currentBlock = data_.mBlock;
    }

    std::vector<CapturedLinearDimension> dimensions;
    std::vector<DRW_Line> lines;

    void addDimLinear(const DRW_DimLinear* value) override {
        if (value != nullptr)
            dimensions.emplace_back(*value);
    }

    void addLine(const DRW_Line& value) override {
        lines.push_back(value);
    }

private:
    dx_data data_;
};

std::string legacyDimensionOrderInput(bool pointsBeforeType) {
    std::ostringstream bytes;
    const auto tag = [&bytes](int code, const char* value) {
        bytes << code << '\n' << value << '\n';
    };
    const auto dimensionPoints = [&tag]() {
        tag(13, "13.25"); tag(23, "23.5"); tag(33, "33.75");
        tag(14, "14.25"); tag(24, "24.5"); tag(34, "34.75");
        tag(15, "15.25"); tag(25, "25.5"); tag(35, "35.75");
    };

    tag(0, "SECTION"); tag(2, "HEADER");
    tag(9, "$ACADVER"); tag(1, "AC1009");
    tag(0, "ENDSEC");
    tag(0, "SECTION"); tag(2, "ENTITIES");
    tag(0, "DIMENSION"); tag(8, "0"); tag(2, "*D1");
    tag(3, "STANDARD");
    if (pointsBeforeType)
        dimensionPoints();
    tag(70, "0");
    if (!pointsBeforeType)
        dimensionPoints();
    tag(0, "LINE"); tag(8, "0");
    tag(10, "101"); tag(20, "102"); tag(30, "103");
    tag(11, "104"); tag(21, "105"); tag(31, "106");
    tag(0, "ENDSEC"); tag(0, "EOF");
    return bytes.str();
}

void testLegacyDimensionPointOrder(TestContext& t) {
    for (const bool pointsBeforeType : {true, false}) {
        LegacyDimensionOrderCapture capture;
        dxfRW reader(nullptr);
        std::string input = legacyDimensionOrderInput(pointsBeforeType);
        const bool read = reader.readAscii(&capture, false, input);
        const std::string order = pointsBeforeType ? "before" : "after";
        t.expect(read && capture.dimensions.size() == 1,
                 ("AC1009 DIMENSION with points " + order
                  + " group70 dispatches as linear").c_str());
        t.expect(read && capture.dimensions.size() == 1
                     && capture.dimensions.front().type == 0
                     && capture.dimensions.front().getDef1Point().x == 13.25
                     && capture.dimensions.front().getDef1Point().y == 23.5
                     && capture.dimensions.front().getDef1Point().z == 33.75
                     && capture.dimensions.front().getDef2Point().x == 14.25
                     && capture.dimensions.front().getDef2Point().y == 24.5
                     && capture.dimensions.front().getDef2Point().z == 34.75
                     && capture.dimensions.front().auxiliaryPoint15().x == 15.25
                     && capture.dimensions.front().auxiliaryPoint15().y == 25.5
                     && capture.dimensions.front().auxiliaryPoint15().z == 35.75,
                 ("AC1009 DIMENSION retains reordered 13/14/15 coordinates "
                  + order + " group70").c_str());
        t.expect(read && capture.lines.size() == 1
                     && capture.lines.front().basePoint.x == 101.0
                     && capture.lines.front().basePoint.z == 103.0
                     && capture.lines.front().secPoint.x == 104.0
                     && capture.lines.front().secPoint.z == 106.0,
                 ("AC1009 DIMENSION group order preserves following LINE "
                  + order + " group70").c_str());
    }
}

void testHiddenZeroHeightAttrib(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now()
        .time_since_epoch().count();
    for (const bool binary : {false, true}) {
        const std::filesystem::path path =
            std::filesystem::temp_directory_path()
            / ("libdxfrw-hidden-zero-attrib-" + std::to_string(nonce)
               + (binary ? "-binary.dxf" : "-ascii.dxf"));
        const bool written = writeHiddenZeroHeightAttribDxf(path, binary);
        FixtureInterface importer;
        dx_data data;
        const bool imported = written
            && importer.fileImport(path.string(), &data, false);
        std::error_code error;
        std::filesystem::remove(path, error);

        t.expect(imported,
                 binary ? "generated binary hidden zero-height ATTRIB imports"
                        : "generated ASCII hidden zero-height ATTRIB imports");
        const DRW_Insert* insert = imported && data.mBlock != nullptr
            && !data.mBlock->ent.empty()
            ? dynamic_cast<const DRW_Insert*>(data.mBlock->ent.front())
            : nullptr;
        t.expect(insert != nullptr && insert->attlist.size() == 1
                     && insert->attlist.front() != nullptr
                     && insert->attlist.front()->tag == "HIDDEN_ZERO_TAG"
                     && insert->attlist.front()->text == "HIDDEN_ZERO_VALUE"
                     && insert->attlist.front()->height == 0.0
                     && (insert->attlist.front()->attribFlags & 1u) != 0,
                 binary ? "binary ATTRIB preserves hidden zero height and values"
                        : "ASCII ATTRIB preserves hidden zero height and values");
        const DRW_Line* followingLine = imported && data.mBlock != nullptr
            && data.mBlock->ent.size() == 2u
            ? dynamic_cast<const DRW_Line*>(data.mBlock->ent.back())
            : nullptr;
        t.expect(followingLine != nullptr
                     && followingLine->basePoint.x == 10.0
                     && followingLine->basePoint.y == 20.0
                     && followingLine->secPoint.x == 40.0
                     && followingLine->secPoint.z == 60.0,
                 binary ? "binary ATTRIB SEQEND preserves following LINE"
                        : "ASCII ATTRIB SEQEND preserves following LINE");
    }
}

void testMeshRawFieldTags(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now()
        .time_since_epoch().count();
    for (const bool binary : {false, true}) {
        const std::filesystem::path path =
            std::filesystem::temp_directory_path()
            / ("libdxfrw-mesh-wire-" + std::to_string(nonce)
               + (binary ? "-binary.dxf" : "-ascii.dxf"));
        std::error_code error;
        std::filesystem::remove(path, error);

        dx_data source;
        auto mesh = std::make_unique<DRW_Mesh>();
        mesh->version = 2;
        mesh->blendCrease = true;
        mesh->subdivisionLevel = 7;
        mesh->vertices = {{0.0, 0.0, 0.0}, {2.0, 0.0, 0.0},
                          {0.0, 3.0, 0.0}};
        mesh->faces = {{0, 1, 2}};
        mesh->edges = {{0, 1}, {1, 2}};
        mesh->creases = {0.25, 0.75};
        source.mBlock->ent.push_back(mesh.release());
        auto* following = new DRW_Line;
        following->basePoint = {10.0, 20.0, 30.0};
        following->secPoint = {40.0, 50.0, 60.0};
        source.mBlock->ent.push_back(following);

        dx_iface writer;
        const bool exported = writer.fileExport(
            path.string(), DRW::AC1021, binary, &source, false);
        t.expect(exported,
                 binary ? "generated binary MESH exports"
                        : "generated ASCII MESH exports");
        t.expect(exported && literalMeshWireFieldsMatch(path, binary),
                 binary ? "binary MESH raw 71/72/94/95/140 fields retain order and LINE framing"
                        : "ASCII MESH raw 71/72/94/95/140 fields retain order and LINE framing");

        dx_data decoded;
        dx_iface reader;
        const bool imported = exported
            && reader.fileImport(path.string(), &decoded, false);
        const DRW_Mesh* decodedMesh = imported && decoded.mBlock != nullptr
            && !decoded.mBlock->ent.empty()
            ? dynamic_cast<const DRW_Mesh*>(decoded.mBlock->ent.front())
            : nullptr;
        t.expect(decodedMesh != nullptr && decodedMesh->version == 2
                     && decodedMesh->blendCrease
                     && decodedMesh->subdivisionLevel == 7
                     && decodedMesh->vertices.size() == 3u
                     && decodedMesh->vertices[1].x == 2.0
                     && decodedMesh->vertices[2].y == 3.0
                     && decodedMesh->faces
                            == std::vector<std::vector<std::int32_t>>{{0, 1, 2}}
                     && decodedMesh->edges
                            == std::vector<std::pair<std::int32_t, std::int32_t>>{
                                {0, 1}, {1, 2}}
                     && decodedMesh->creases
                            == std::vector<double>{0.25, 0.75},
                 binary ? "binary MESH typed self-read retains generated fields"
                        : "ASCII MESH typed self-read retains generated fields");
        const DRW_Line* decodedLine = imported && decoded.mBlock != nullptr
            && decoded.mBlock->ent.size() == 2u
            ? dynamic_cast<const DRW_Line*>(decoded.mBlock->ent.back())
            : nullptr;
        t.expect(decodedLine != nullptr
                     && decodedLine->basePoint.x == 10.0
                     && decodedLine->secPoint.z == 60.0,
                 binary ? "binary MESH reader publishes following LINE"
                        : "ASCII MESH reader publishes following LINE");
        std::filesystem::remove(path, error);
    }
}

void testDynamicTableCapacityBasics(TestContext& t) {
    class CapacityInterface final : public dx_iface {
    public:
        void writeViews() override {
            ++viewCalls;
            DRW_View first;
            first.name = "CAPACITY_VIEW_A";
            firstViewWritten = dxfW->writeView(&first);
            DRW_View second;
            second.name = "CAPACITY_VIEW_B";
            secondViewWritten = dxfW->writeView(&second);
        }

        void writeUCSs() override {
            ++ucsCalls;
            DRW_UCS ucs;
            ucs.name = "CAPACITY_UCS_A";
            ucsWritten = dxfW->writeUCS(&ucs);
        }

        int viewCalls {0};
        int ucsCalls {0};
        bool firstViewWritten {false};
        bool secondViewWritten {false};
        bool ucsWritten {false};
    } interface_;

    const auto nonce = std::chrono::steady_clock::now()
        .time_since_epoch().count();
    const std::filesystem::path path =
        std::filesystem::temp_directory_path()
        / ("libdxfrw-table-capacity-basics-" + std::to_string(nonce)
           + ".dxf");
    std::error_code error;
    std::filesystem::remove(path, error);
    dx_data data;
    const bool exported = interface_.fileExport(
        path.string(), DRW::AC1021, false, &data, false);
    t.expect(exported && interface_.viewCalls == 1
                 && interface_.ucsCalls == 1
                 && interface_.firstViewWritten && interface_.secondViewWritten
                 && interface_.ucsWritten,
             "dynamic VIEW/UCS callbacks execute once and emit all requested rows");

    DxfTableSnapshot views;
    DxfTableSnapshot ucs;
    DxfTableSnapshot layers;
    t.expect(exported && readDxfTableSnapshot(path, false, "VIEW", views)
                 && views.declaredCapacity == 2
                 && views.rowKinds == std::vector<std::string>{"VIEW", "VIEW"}
                 && views.rowNames == std::vector<std::string>{
                     "CAPACITY_VIEW_A", "CAPACITY_VIEW_B"},
             "VIEW group70 matches both emitted callback records");
    t.expect(exported && readDxfTableSnapshot(path, false, "UCS", ucs)
                 && ucs.declaredCapacity == 1
                 && ucs.rowKinds == std::vector<std::string>{"UCS"}
                 && ucs.rowNames == std::vector<std::string>{"CAPACITY_UCS_A"},
             "UCS group70 matches its emitted callback record");
    t.expect(exported && readDxfTableSnapshot(path, false, "LAYER", layers)
                 && layers.declaredCapacity == 1
                 && layers.rowKinds == std::vector<std::string>{"LAYER"}
                 && layers.rowNames == std::vector<std::string>{"0"},
             "implicit default LAYER is included in its capacity");
    std::filesystem::remove(path, error);
}

std::string headerOrderBytes(
    bool binary, const std::vector<std::pair<int, std::string>>& fields,
    const std::string& text = "", const std::string& version = "AC1018") {
    std::ostringstream bytes;
    if (binary)
        bytes.write("AutoCAD Binary DXF\r\n\x1A\0", 22);
    std::unique_ptr<dxfWriter> writer = binary
        ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&bytes))
        : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&bytes));
    writer->setVersion(version, true);
    writer->writeString(0, "SECTION");
    writer->writeString(2, "HEADER");
    for (const auto& field : fields) {
        if (field.first == 70)
            writer->writeInt16(field.first, std::stoi(field.second));
        else
            writer->writeString(field.first, field.second);
    }
    writer->writeString(0, "ENDSEC");
    if (!text.empty()) {
        writer->writeString(0, "SECTION");
        writer->writeString(2, "ENTITIES");
        writer->writeString(0, "TEXT");
        writer->writeString(5, "A0");
        writer->writeString(8, "0");
        writer->writeDouble(10, 1.0);
        writer->writeDouble(20, 2.0);
        writer->writeDouble(30, 3.0);
        writer->writeDouble(40, 2.5);
        writer->writeString(1, text);
        writer->writeString(0, "LINE");
        writer->writeString(5, "A1");
        writer->writeString(8, "0");
        writer->writeDouble(10, 4.0);
        writer->writeDouble(20, 5.0);
        writer->writeDouble(30, 6.0);
        writer->writeDouble(11, 7.0);
        writer->writeDouble(21, 8.0);
        writer->writeDouble(31, 9.0);
        writer->writeString(0, "ENDSEC");
    }
    writer->writeString(0, "EOF");
    return bytes.str();
}

bool headerStringIs(const DRW_Header& header, const char* key,
                    const std::string& expected) {
    const auto found = header.vars.find(key);
    return found != header.vars.end() && found->second != nullptr
        && found->second->type() == DRW_Variant::STRING
        && *found->second->content.s == expected;
}

bool headerIntIs(const DRW_Header& header, const char* key, int expected) {
    const auto found = header.vars.find(key);
    return found != header.vars.end() && found->second != nullptr
        && found->second->type() == DRW_Variant::INTEGER
        && found->second->content.i == expected;
}

bool readHeaderOrderFile(const std::string& bytes, HeaderOrderCapture& capture) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-header-order-" + std::to_string(nonce) + ".dxf");
    { std::ofstream output(path, std::ios::binary); output << bytes; }
    dxfRW reader(path.string().c_str());
    const bool read = reader.read(&capture, false);
    std::error_code error;
    std::filesystem::remove(path, error);
    return read;
}

void testHeaderMetadataOrder(TestContext& t) {
    const std::string legacy("\xC6");
    const std::string semantic("\xD0\x96");
    const std::vector<std::vector<std::pair<int, std::string>>> headers{
        {{9, "$ACADVER"}, {1, "AC1018"}, {9, "$DWGCODEPAGE"}, {3, "ANSI_1251"},
         {9, "$CLAYER"}, {8, legacy}},
        {{9, "$DWGCODEPAGE"}, {3, "ANSI_1251"}, {9, "$ACADVER"}, {1, "AC1018"},
         {9, "$CLAYER"}, {8, legacy}},
        {{9, "$CLAYER"}, {8, legacy}, {9, "$ACADVER"}, {1, "AC1018"},
         {9, "$DWGCODEPAGE"}, {3, "ANSI_1251"}},
        {{9, "$CLAYER"}, {8, semantic}, {9, "$DWGCODEPAGE"}, {3, "ANSI_1251"},
         {9, "$ACADVER"}, {1, "AC1021"}},
        {{9, "$DWGCODEPAGE"}, {3, "ANSI_1252"}, {9, "$CLAYER"}, {8, legacy},
         {9, "$DWGCODEPAGE"}, {3, "ANSI_1251"}, {9, "$ACADVER"}, {1, "AC1018"}}};
    for (bool binary : {false, true}) {
        for (std::size_t index = 0; index < headers.size(); ++index) {
            const bool modern = index == 3;
            std::string bytes = headerOrderBytes(binary, headers[index],
                modern ? semantic : legacy, modern ? "AC1021" : "AC1018");
            HeaderOrderCapture capture;
            bool read = false;
            read = readHeaderOrderFile(bytes, capture);
            t.expect(read && capture.headerCount == 1
                         && headerStringIs(capture.value, "$CLAYER", semantic)
                         && capture.texts == std::vector<std::string>{semantic},
                     "file HEADER text and later TEXT use final metadata in any valid order");
            t.expect(read && capture.lines.size() == 1
                         && capture.lines.front().basePoint.x == 4.0
                         && capture.lines.front().secPoint.z == 9.0,
                     "file HEADER metadata finalization preserves geometry");
            if (!binary) {
                HeaderOrderCapture memory;
                dxfRW reader(nullptr);
                t.expect(reader.readAscii(&memory, false, bytes)
                             && headerStringIs(memory.value, "$CLAYER", semantic)
                             && memory.texts == std::vector<std::string>{semantic},
                         "readAscii HEADER and later TEXT agree with file route");
            }
        }
    }
    for (const std::string version : {"AC1009", "AC1014"}) {
        for (bool before : {false, true}) {
            std::vector<std::pair<int, std::string>> fields{
                {9, "$ACADVER"}, {1, version}};
            const std::vector<std::pair<int, std::string>> legacyUnit{
                {9, "$DIMUNIT"}, {70, "4"}};
            fields.insert(before ? fields.begin() : fields.end(),
                          legacyUnit.begin(), legacyUnit.end());
            for (bool binary : {false, true}) {
                std::string bytes = headerOrderBytes(binary, fields, "", version);
                HeaderOrderCapture capture;
                t.expect(readHeaderOrderFile(bytes, capture)
                             && headerIntIs(capture.value, "$DIMUNIT", 4)
                             && headerIntIs(capture.value, "$DIMLUNIT", 4)
                             && capture.value.dxfSourceVersion()
                                    == (version == "AC1009" ? DRW::AC1009 : DRW::AC1014)
                             && capture.value.dxfDimunitAliasState()
                                    == DRW_Header::DxfDimunitAliasState::Derived,
                         "legacy source and derived alias are independent of HEADER order and wire encoding");
            }
        }
    }
}

void testHeaderMetadataBoundaries(TestContext& t) {
    using AliasState = DRW_Header::DxfDimunitAliasState;
    const std::string legacy("\xC6");
    const std::string semantic("\xD0\x96");
    for (bool binary : {false, true}) {
        // Every currently supported semantic HEADER string role is deferred;
        // handle390 is deliberately not a text-codepage field.
        std::vector<std::pair<int, std::string>> fields;
        const std::vector<int> codes{1, 2, 3, 6, 7, 8};
        for (int code : codes) {
            fields.emplace_back(9, "$LOCAL_TEXT_" + std::to_string(code));
            fields.emplace_back(code, legacy);
        }
        fields.insert(fields.end(), {{9, "$ACADVER"}, {1, "AC1018"},
            {9, "$DWGCODEPAGE"}, {3, "ANSI_1251"},
            {9, "$LOCAL_HANDLE"}, {390, "A0"}});
        HeaderOrderCapture capture;
        t.expect(readHeaderOrderFile(headerOrderBytes(binary, fields), capture),
                 "all supported HEADER text roles parse with late metadata");
        for (int code : codes)
            t.expect(headerStringIs(capture.value,
                         ("$LOCAL_TEXT_" + std::to_string(code)).c_str(), semantic),
                     "each deferred semantic HEADER field decodes exactly once");
        t.expect(headerStringIs(capture.value, "$LOCAL_HANDLE", "A0"),
                 "HEADER handle spelling is not transcoded as semantic text");

        std::vector<std::pair<int, std::string>> lateMetadata{{9, "$CLAYER"}, {8, legacy}};
        for (const char* key : {"$PROJECTNAME", "$HYPERLINKBASE", "$MENU", "$STYLESHEET", "$LASTSAVEDBY"}) {
            lateMetadata.emplace_back(9, key);
            lateMetadata.emplace_back(1, std::string(230, 'x'));
        }
        lateMetadata.insert(lateMetadata.end(), {{9, "$ACADVER"}, {1, "AC1018"},
            {9, "$DWGCODEPAGE"}, {3, "ANSI_1251"}});
        HeaderOrderCapture lateCapture;
        t.expect(readHeaderOrderFile(headerOrderBytes(binary, lateMetadata, legacy), lateCapture)
                     && headerStringIs(lateCapture.value, "$CLAYER", semantic)
                     && lateCapture.texts == std::vector<std::string>{semantic},
                 "metadata beyond independent binary oracle prescan still finalizes local HEADER and TEXT");

        const std::string western("\xE9");
        HeaderOrderCapture noPage;
        t.expect(readHeaderOrderFile(headerOrderBytes(binary,
                     {{9, "$CLAYER"}, {8, western}, {9, "$ACADVER"}, {1, "AC1018"}}, western), noPage)
                     && headerStringIs(noPage.value, "$CLAYER", u8"é")
                     && noPage.texts == std::vector<std::string>{u8"é"},
                 "known legacy source without codepage keeps existing CP1252 fallback");

        // Repeated HEADER sections are invalid DXF, but historically tolerated.
        // Previously finalized strings must not be decoded a second time.
        std::string firstHeader = headerOrderBytes(binary,
            {{9, "$ACADVER"}, {1, "AC1018"}, {9, "$DWGCODEPAGE"}, {3, "ANSI_1251"},
             {9, "$CLAYER"}, {8, legacy}});
        const std::string secondHeader = headerOrderBytes(binary,
            {{9, "$PROJECTNAME"}, {1, legacy}});
        firstHeader.resize(firstHeader.size() - (binary ? 6u : 8u));
        firstHeader += secondHeader.substr(binary ? 22u : 0u);
        HeaderOrderCapture repeated;
        t.expect(readHeaderOrderFile(firstHeader, repeated) && repeated.headerCount == 2
                     && headerStringIs(repeated.value, "$CLAYER", semantic)
                     && headerStringIs(repeated.value, "$PROJECTNAME", semantic),
                 "tolerated duplicate HEADER does not re-decode prior semantic strings");

        for (int modernValue : {0, 2, 4}) {
            for (bool modernFirst : {false, true}) {
                std::vector<std::pair<int, std::string>> units{
                    {9, "$ACADVER"}, {1, "AC1014"},
                    {9, "$DIMUNIT"}, {70, "6"}, {9, "$DIMUNIT"}, {70, "4"}};
                const std::vector<std::pair<int, std::string>> modern{
                    {9, "$DIMLUNIT"}, {70, std::to_string(modernValue)}};
                units.insert(modernFirst ? units.begin() : units.end(),
                             modern.begin(), modern.end());
                HeaderOrderCapture explicitModern;
                t.expect(readHeaderOrderFile(headerOrderBytes(binary, units, "", "AC1014"),
                                               explicitModern)
                             && headerIntIs(explicitModern.value, "$DIMUNIT", 4)
                             && headerIntIs(explicitModern.value, "$DIMLUNIT", modernValue)
                             && explicitModern.value.dxfDimunitAliasState() == AliasState::None,
                         "last legacy value is retained and explicit modern always wins including zero/default/equal alias");
            }
        }
        for (const std::string finalVersion : {"AC9999", "AC1009"}) {
            const std::vector<std::pair<int, std::string>> protectedVersion{
                {9, "$ACADVER"}, {1, "AC1021"}, {9, "$ACADVER"}, {1, finalVersion},
                {9, "$DIMUNIT"}, {70, "4"}};
            HeaderOrderCapture protectedCapture;
            t.expect(readHeaderOrderFile(headerOrderBytes(binary, protectedVersion, "", "AC1021"),
                                           protectedCapture)
                         && protectedCapture.value.dxfSourceVersion() == DRW::AC1021
                         && headerStringIs(protectedCapture.value, "$ACADVER", finalVersion)
                         && protectedCapture.value.vars.count("$DIMLUNIT") == 0,
                     "lower/unknown duplicate source declarations cannot fabricate legacy alias interpretation");
        }
        for (const std::vector<std::pair<int, std::string>>& metadata : {
                std::vector<std::pair<int, std::string>>{},
                std::vector<std::pair<int, std::string>>{{9, "$ACADVER"}, {1, "AC9999"}}}) {
            auto fieldsWithoutVersion = metadata;
            fieldsWithoutVersion.insert(fieldsWithoutVersion.end(),
                {{9, "$DIMUNIT"}, {70, "4"}, {9, "$CLAYER"}, {8, legacy},
                 {9, "$DWGCODEPAGE"}, {3, "ANSI_1251"}});
            HeaderOrderCapture unknownCapture;
            t.expect(readHeaderOrderFile(headerOrderBytes(binary, fieldsWithoutVersion), unknownCapture)
                         && unknownCapture.value.dxfSourceVersion() == DRW::UNKNOWNV
                         && unknownCapture.value.vars.count("$DIMLUNIT") == 0
                         && headerStringIs(unknownCapture.value, "$CLAYER", semantic),
                     "missing/unknown metadata preserves existing codepage fallback without guessing legacy alias");
        }
    }

    dxfRW reusable(nullptr);
    std::string complete = headerOrderBytes(false, {{9, "$CLAYER"}, {8, legacy},
        {9, "$DWGCODEPAGE"}, {3, "ANSI_1251"}, {9, "$ACADVER"}, {1, "AC1018"}});
    const std::size_t records = static_cast<std::size_t>(std::count(complete.begin(), complete.end(), '\n') / 2);
    for (std::size_t budget : {records - 1, records, records + 1}) {
        HeaderOrderCapture capture;
        std::string bytes = complete;
        reusable.setDxfReadRecordBudget(budget);
        const bool read = reusable.readAscii(&capture, false, bytes);
        t.expect(read == (budget >= records),
                 "one-pass HEADER finalization consumes each physical record exactly once");
        if (read)
            t.expect(headerStringIs(capture.value, "$CLAYER", semantic),
                     "exact record budget still produces finalized callback text");
    }
    HeaderOrderCapture partial;
    std::string truncated = "0\nSECTION\n2\nHEADER\n9\n$CLAYER\n8\n" + legacy + "\n";
    reusable.setDxfReadRecordBudget(records + 1);
    t.expect(!reusable.readAscii(&partial, false, truncated) && partial.headerCount == 0,
             "incomplete HEADER publishes no partial decoded state");
    HeaderOrderCapture retried;
    t.expect(reusable.readAscii(&retried, false, complete)
                 && headerStringIs(retried.value, "$CLAYER", semantic),
             "failed then successful read reuses no stale deferred HEADER state");
}

void testHeaderAliasProvenance(TestContext& t) {
    using AliasState = DRW_Header::DxfDimunitAliasState;
    std::string bytes = headerOrderBytes(false, {{9, "$DIMUNIT"}, {70, "4"},
        {9, "$ACADVER"}, {1, "AC1014"}}, "", "AC1014");
    HeaderOrderCapture capture;
    dxfRW reader(nullptr);
    t.expect(reader.readAscii(&capture, false, bytes), "alias provenance source imports");
    const DRW_Header original(capture.value);
    DRW_Header copied(original);
    DRW_Header assigned;
    assigned = original;
    DRW_Header moved(std::move(copied));
    DRW_Header moveAssigned;
    moveAssigned = std::move(assigned);
    for (const DRW_Header* value : std::array<const DRW_Header*, 3>{&original, &moved, &moveAssigned})
        t.expect(value->dxfSourceVersion() == DRW::AC1014
                     && value->dxfDimunitAliasState() == AliasState::Derived
                     && headerIntIs(*value, "$DIMUNIT", 4)
                     && headerIntIs(*value, "$DIMLUNIT", 4),
                 "copy/move/assignment retain source and alias provenance independently");
    t.expect(copied.dxfSourceVersion() == DRW::UNKNOWNV
                 && copied.dxfDimunitAliasState() == AliasState::None
                 && assigned.dxfSourceVersion() == DRW::UNKNOWNV,
             "moved-from header does not retain a source/alias claim");

    DRW_Header edited(original);
    edited.vars.at("$DIMUNIT")->addInt(70, 6);
    t.expect(edited.dxfDimunitAliasState() == AliasState::StaleSource,
             "direct legacy edit does not silently promote stale derived alias");
    edited = original;
    edited.addInt("$DIMUNIT", 6, 70);
    t.expect(edited.dxfDimunitAliasState() == AliasState::StaleSource,
             "helper legacy replacement invalidates source snapshot without guessing alias meaning");
    edited = original;
    delete edited.vars.at("$DIMUNIT");
    edited.vars.erase("$DIMUNIT");
    t.expect(edited.dxfDimunitAliasState() == AliasState::StaleSource,
             "removed legacy source is visible as stale derived alias");
    edited = original;
    edited.vars.at("$DIMLUNIT")->addInt(70, 2);
    t.expect(edited.dxfDimunitAliasState() == AliasState::None,
             "changed direct alias is explicit modern input");
    edited = original;
    edited.addInt("$DIMLUNIT", 4, 70);
    t.expect(edited.dxfDimunitAliasState() == AliasState::None,
             "same-value helper alias replacement explicitly asserts modern meaning");
    edited = original;
    edited.clearDxfDimunitAliasProvenance();
    t.expect(edited.dxfDimunitAliasState() == AliasState::None
                 && headerIntIs(edited, "$DIMUNIT", 4) && headerIntIs(edited, "$DIMLUNIT", 4),
             "same-value direct edit has explicit opt-in without deleting source data");
    edited = original;
    edited = DRW_Header{};
    t.expect(edited.vars.empty() && edited.dxfSourceVersion() == DRW::UNKNOWNV
                 && edited.dxfDimunitAliasState() == AliasState::None,
             "reset/from-API header carries no stale DXF provenance");
}

void testAdapterAcceptedDxfVersion(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-accepted-source-version-" + std::to_string(nonce) + ".dxf");
    for (const bool binary : {false, true}) {
        for (const std::string last : {"AC1009", "AC9999", "AC1032"}) {
            const auto expected = last == "AC1032" ? DRW::AC1032 : DRW::AC1021;
            const std::string bytes = headerOrderBytes(binary,
                {{9, "$ACADVER"}, {1, "AC1021"}, {9, "$ACADVER"}, {1, last}},
                "source-version-control", "AC1021");
            { std::ofstream output(path, std::ios::binary); output << bytes; }
            dx_iface adapter;
            dx_data data;
            t.expect(adapter.fileImport(path.string(), &data, false)
                         && data.sourceVersion == expected
                         && data.headerC.dxfSourceVersion() == expected
                         && headerStringIs(data.headerC, "$ACADVER", last)
                         && data.mBlock->ent.size() == 2,
                     "CLI adapter uses accepted highest source version while retaining last raw metadata");
        }
        { std::ofstream output(path, std::ios::binary);
          output << headerOrderBytes(binary, {}, "absent-source-version"); }
        dx_iface adapter;
        dx_data data;
        t.expect(adapter.fileImport(path.string(), &data, false)
                     && data.headerC.dxfSourceVersion() == DRW::UNKNOWNV
                     && data.sourceVersion == DRW::AC1021,
                 "absent source declaration retains explicit reader-version fallback");
    }
    std::error_code error;
    std::filesystem::remove(path, error);
}

void testConsumerSourceVersionSnapshot(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-consumer-source-version-" + std::to_string(nonce) + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{path};
    dxfRW codec(path.string().c_str());
    t.expect(codec.getSourceVersion() == DRW::UNKNOWNV,
             "new facade has no invented source declaration");
    struct VersionCase {
        const char* first;
        const char* last;
        DRW::Version source;
        DRW::Version decoded;
    };
    const VersionCase cases[]{
        {"AC1006", "", DRW::AC1006, DRW::AC1009},
        {"AC1009", "", DRW::AC1009, DRW::AC1009},
        {"AC1012", "", DRW::AC1012, DRW::AC1015},
        {"AC1014", "", DRW::AC1014, DRW::AC1015},
        {"AC1015", "", DRW::AC1015, DRW::AC1015},
        {"AC1018", "", DRW::AC1018, DRW::AC1015},
        {"AC1021", "", DRW::AC1021, DRW::AC1021},
        {"AC1024", "", DRW::AC1024, DRW::AC1021},
        {"AC1027", "", DRW::AC1027, DRW::AC1021},
        {"AC1032", "", DRW::AC1032, DRW::AC1021},
        {"AC9999", "", DRW::UNKNOWNV, DRW::UNKNOWNV},
        {"", "", DRW::UNKNOWNV, DRW::AC1021},
        // Repeats are robustness controls, not valid-file qualification.
        {"AC1024", "AC1009", DRW::AC1024, DRW::AC1021},
        {"AC1024", "AC9999", DRW::AC1024, DRW::AC1021},
        {"AC1018", "AC1032", DRW::AC1032, DRW::AC1021},
        {"AC9999", "AC1018", DRW::AC1018, DRW::AC1015}
    };
    for (const auto& item : cases) {
        std::vector<std::pair<int, std::string>> fields;
        if (*item.first != '\0')
            fields = {{9, "$ACADVER"}, {1, item.first}};
        if (*item.last != '\0') {
            fields.emplace_back(9, "$ACADVER");
            fields.emplace_back(1, item.last);
        }
        for (int entry = 0; entry != 3; ++entry) {
            // headerOrderBytes uses the R13+ binary tag width. Actual R12
            // binary writer/readback is checked with the reservation cases.
            if (entry == 2 && item.source != DRW::UNKNOWNV
                && item.source <= DRW::AC1009)
                continue;
            std::string bytes = headerOrderBytes(entry == 2, fields);
            HeaderOrderCapture capture;
            if (entry != 0) {
                std::ofstream output(path, std::ios::binary);
                output << bytes;
            }
            const bool accepted = entry == 0
                ? codec.readAscii(&capture, false, bytes)
                : codec.read(&capture, false);
            t.expect(accepted && capture.headerCount == 1
                         && capture.value.dxfSourceVersion() == item.source
                         && codec.getSourceVersion() == item.source
                         && codec.getVersion() == item.decoded,
                     "source getter retains accepted revision, not normalized codec version, on every read entry");
        }
    }
    std::string known = headerOrderBytes(false,
        {{9, "$ACADVER"}, {1, "AC1024"}}, "source-version-callback");
    HeaderOrderCapture capture;
    t.expect(codec.readAscii(&capture, false, known)
                 && codec.getSourceVersion() == DRW::AC1024,
             "known source snapshot is available after reader cleanup");
    t.expect(!codec.readAscii(nullptr, false, known)
                 && codec.getSourceVersion() == DRW::AC1024,
             "null ASCII pre-session rejection preserves the previous snapshot");
    t.expect(!codec.read(nullptr, false)
                 && codec.getSourceVersion() == DRW::UNKNOWNV,
             "admitted file read clears source before null interface rejection");
    t.expect(codec.readAscii(&capture, false, known), "source restored before open failure");
    std::error_code error;
    std::filesystem::remove(path, error);
    t.expect(!codec.read(&capture, false)
                 && codec.getError() == DRW::BAD_OPEN
                 && codec.getSourceVersion() == DRW::UNKNOWNV,
             "missing file does not expose the previous source snapshot");
    std::string beforeHeader = "not-a-group\nvalue\n";
    t.expect(!codec.readAscii(&capture, false, beforeHeader)
                 && codec.getSourceVersion() == DRW::UNKNOWNV,
             "failure before an accepted declaration reports unknown source");
    std::string afterHeader =
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1024\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n0\nLINE\n10\n0\n20\n0\n11\n1\n"
        "0\nENDSEC\n0\nEOF\n"; // The LINE is invalid: required end-Y group 21 is absent.
    for (const bool file : {false, true}) {
        if (file) { std::ofstream output(path, std::ios::binary); output << afterHeader; }
        HeaderOrderCapture rejected;
        const bool accepted = file ? codec.read(&rejected, false)
            : codec.readAscii(&rejected, false, afterHeader);
        t.expect(!accepted && rejected.headerCount == 1 && rejected.lines.empty()
                     && codec.getSourceVersion() == DRW::AC1024,
                 "returned parse failure snapshots accepted source without claiming read success");
    }
    class CallbackCapture final : public dx_iface {
    public:
        CallbackCapture(dxfRW& owner, TestContext& context)
            : codec(owner), test(context) {}
        void addHeader(const DRW_Header* header) override {
            test.expect(header->dxfSourceVersion() == DRW::AC1024
                            && codec.getSourceVersion() == DRW::UNKNOWNV,
                        "HEADER knows accepted source while facade snapshot is not yet published");
        }
        void addText(const DRW_Text&) override {}
        void addLine(const DRW_Line&) override {
            ++lines;
            test.expect(codec.getSourceVersion() == DRW::UNKNOWNV,
                        "callback-time facade getter follows bundled post-parse snapshot timing");
            if (throwing)
                throw std::runtime_error("source-version callback");
            std::string nested = headerOrderBytes(false, {{9, "$ACADVER"}, {1, "AC1018"}});
            const auto error = codec.getError();
            const auto diagnostic = codec.getLastDiagnostic();
            test.expect(!codec.readAscii(this, false, nested)
                            && !codec.read(this, false)
                            && !codec.write(this, DRW::AC1018, false)
                            && codec.getSourceVersion() == DRW::UNKNOWNV
                            && codec.getError() == error
                            && codec.getLastDiagnostic().code == diagnostic.code,
                        "rejected nested operations leave the active source generation untouched");
            dxfRW other(nullptr);
            HeaderOrderCapture otherCapture;
            test.expect(other.readAscii(&otherCapture, false, nested)
                            && other.getSourceVersion() == DRW::AC1018
                            && codec.getSourceVersion() == DRW::UNKNOWNV,
                        "different facade snapshots independently during the outer callback");
        }
        dxfRW& codec;
        TestContext& test;
        bool throwing{false};
        int lines{0};
    };
    for (const bool file : {false, true}) {
        { std::ofstream output(path, std::ios::binary); output << known; }
        CallbackCapture nested(codec, t);
        t.expect((file ? codec.read(&nested, false) : codec.readAscii(&nested, false, known))
                     && nested.lines == 1 && codec.getSourceVersion() == DRW::AC1024,
                 "completed outer read publishes its source after nested rejection");
        CallbackCapture throwing(codec, t);
        throwing.throwing = true;
        bool caught = false;
        try {
            if (file) (void)codec.read(&throwing, false);
            else (void)codec.readAscii(&throwing, false, known);
        } catch (const std::runtime_error&) { caught = true; }
        t.expect(caught && throwing.lines == 1
                     && codec.getSourceVersion() == DRW::UNKNOWNV,
                 "callback exception preserves propagation but publishes no completed source snapshot");
        t.expect(!codec.readAscii(nullptr, false, known)
                     && codec.getSourceVersion() == DRW::UNKNOWNV,
                 "early rejection does not inspect an interrupted owned reader");
        HeaderOrderCapture retry;
        t.expect(codec.readAscii(&retry, false, known)
                     && retry.lines.size() == 1
                     && codec.getSourceVersion() == DRW::AC1024,
                 "same facade recovers its source snapshot after callback failure");
    }
}

void testExplicitReservationQuery(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-reservation-query-" + std::to_string(nonce) + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{path};
    class ReservationWriter final : public dx_iface {
    public:
        ReservationWriter(dx_data& data, TestContext& context) : test(context) {
            cData = &data;
            currentBlock = data.mBlock;
        }
        void writeEntities() override {
            const auto block = dxfW->getBlockRecordHandleToWrite("QueryBlock");
            test.expect(dxfW->isReservedHandle(0x10u)
                            && (modern ? (block != 0 && !dxfW->isReservedHandle(block)
                                && dxfW->isReservedHandle(block + 1)
                                && dxfW->isReservedHandle(block + 2)) : block == 0),
                        "query includes internal fixed/BLOCK reservations but excludes minted BLOCK_RECORD");
            generated = dxfW->allocHandle();
            DRW_Line line;
            line.secPoint = {1, 2, 3};
            record = dxfW->writeLine(&line);
            emitted = line.handle;
            const auto before = dxfW->highWaterHandle();
            test.expect(generated != 0 && emitted != 0 && record
                            && !dxfW->isReservedHandle(generated)
                            && !dxfW->isReservedHandle(emitted)
                            && dxfW->isReservedHandle(0xD000u)
                            && !dxfW->isReservedHandle(before + 10)
                            && dxfW->highWaterHandle() == before,
                        "reservation query distinguishes explicit/generated/unreserved without advancing allocation");
            if (fail)
                (void)dxfW->writeLine(nullptr);
        }
        bool finalizeDxfWrite() override {
            ++finalizers;
            test.expect(dxfW->isReservedHandle(0xD000u)
                            && !dxfW->isReservedHandle(emitted),
                        "read-only reservation query remains available during finalization");
            return true;
        }
        TestContext& test;
        bool modern{false};
        bool fail{false};
        bool record{false};
        std::uint32_t generated{0};
        std::uint32_t emitted{0};
        int finalizers{0};
    };
    for (const auto version : {DRW::AC1009, DRW::AC1024}) {
        for (const bool binary : {false, true}) {
            dxfRW codec(path.string().c_str());
            const dxfRW& query = codec;
            t.expect(!query.isReservedHandle(0x10u) && !query.isReservedHandle(0xD000u),
                     "fresh allocator has no inferred high-water or fixed reservations");
            t.expect(codec.reserveHandle(0xD000u) && codec.reserveHandle(0xD000u),
                     "explicit reservation remains idempotent");
            const auto before = codec.highWaterHandle();
            const auto generated = codec.allocHandle();
            t.expect(query.isReservedHandle(0xD000u) && generated == before
                         && !query.isReservedHandle(generated)
                         && !query.isReservedHandle(1u),
                     "pre-write minted handles are not explicit reservations");
            t.expect(codec.reserveHandle(generated) && query.isReservedHandle(generated),
                     "an explicit reservation can promote a previously minted handle");
            std::string source = headerOrderBytes(false, {{9, "$ACADVER"}, {1, "AC1018"}});
            HeaderOrderCapture sourceCapture;
            t.expect(codec.readAscii(&sourceCapture, false, source),
                     "reservation writer has a distinct accepted source revision");
            dx_data data;
            auto* block = new dx_ifaceBlock();
            block->name = "QueryBlock";
            data.blocks.push_back(block);
            ReservationWriter adapter(data, t);
            adapter.dxfW = &codec;
            adapter.modern = version > DRW::AC1009;
            for (const bool fail : {false, true, false}) {
                adapter.fail = fail;
                const auto finalizers = adapter.finalizers;
                t.expect(codec.write(&adapter, version, binary) == !fail
                             && adapter.record
                             && adapter.finalizers == finalizers + (fail ? 0 : 1)
                             && query.isReservedHandle(0xD000u)
                             && query.isReservedHandle(generated)
                             && !query.isReservedHandle(adapter.generated)
                             && !query.isReservedHandle(adapter.emitted)
                             && codec.getSourceVersion() == DRW::AC1018,
                         "success/failure/corrected same-writer retry preserve explicit reservations and last read source");
            }
            FixtureInterface capture;
            dx_data readback;
            capture.attach(&readback);
            dxfRW rereader(path.string().c_str());
            t.expect(rereader.read(&capture, false)
                         && rereader.getSourceVersion() == version,
                     "actual R12/modern ASCII/binary writer output retains declared source revision");
        }
    }
    dxfRW exhausted(path.string().c_str());
    const auto before = exhausted.highWaterHandle();
    const auto maximum = (std::numeric_limits<std::uint32_t>::max)();
    t.expect(!exhausted.reserveHandle(maximum)
                 && !exhausted.isReservedHandle(maximum)
                 && exhausted.highWaterHandle() == before,
             "failed reservation is not recorded or inferred from the high-water mark");
    dx_data data;
    FixtureInterface adapter;
    adapter.attach(&data);
    t.expect(!exhausted.write(&adapter, DRW::AC1024, false)
                 && !exhausted.isReservedHandle(maximum)
                 && exhausted.getSourceVersion() == DRW::UNKNOWNV,
             "reservation admission failure does not invent an explicit reservation or source version");
}

void testDxfReferenceResolver(TestContext& t) {
    class ResolverInterface final : public dx_iface {
    public:
        void attach(dx_data& data) {
            cData = &data;
            currentBlock = data.mBlock;
        }
        void writeEntities() override {
            DRW_Line direct;
            direct.layer = "RESOLVER_LAYER";
            direct.secPoint = {1, 2, 3};
            direct.reactorHandles = {0x10u, 0x20u};
            direct.xDictHandle = 0x20u;
            direct.extData.push_back(std::make_shared<DRW_Variant>(
                1001, std::string("RESOLVER_APP")));
            for (const std::string& value : {"0", "0000", "10"})
                direct.extData.push_back(
                    std::make_shared<DRW_Variant>(1005, value));
            if (includeMalformedXdata)
                direct.extData.push_back(
                    std::make_shared<DRW_Variant>(1005, "not-hex"));
            directWritten = dxfW->writeLine(&direct);

            DRW_Line grouped;
            grouped.layer = "RESOLVER_LAYER";
            grouped.basePoint = {4, 5, 6};
            grouped.secPoint = {7, 8, 9};
            grouped.appData.push_back({
                {102, std::string("{ACAD_REACTORS")},
                {330, std::string("10")}, {330, std::string("20")},
                {102, std::string("}")}});
            grouped.appData.push_back({
                {102, std::string("{ACAD_XDICTIONARY")},
                {360, std::string("21")}, {102, std::string("}")}});
            grouped.appData.push_back({
                {102, std::string("{FOREIGN_META")},
                {330, std::string("10")}, {102, std::string("}")}});
            groupedWritten = dxfW->writeLine(&grouped);
        }
        bool directWritten {false};
        bool groupedWritten {false};
        bool includeMalformedXdata {false};
    };

    const auto stamp = std::chrono::steady_clock::now()
                           .time_since_epoch().count();
    for (const bool binary : {false, true}) {
        const auto path = std::filesystem::temp_directory_path()
            / ("libdxfrw-reference-resolver-" + std::to_string(stamp)
               + (binary ? "-binary.dxf" : "-ascii.dxf"));
        struct Cleanup {
            std::filesystem::path path;
            ~Cleanup() {
                std::error_code error;
                std::filesystem::remove(path, error);
            }
        } cleanup{path};

        dx_data data;
        DRW_Layer layer;
        layer.name = "RESOLVER_LAYER";
        layer.reactorHandles = {0x10u, 0x20u};
        layer.xDictHandle = 0x20u;
        data.layers.push_back(layer);
        DRW_Layer groupedLayer;
        groupedLayer.name = "RESOLVER_GROUPED_LAYER";
        groupedLayer.appData.push_back({
            {102, std::string("{ACAD_REACTORS")},
            {330, std::string("10")}, {330, std::string("20")},
            {102, std::string("}")}});
        groupedLayer.appData.push_back({
            {102, std::string("{ACAD_XDICTIONARY")},
            {360, std::string("21")}, {102, std::string("}")}});
        data.layers.push_back(groupedLayer);
        ResolverInterface adapter;
        adapter.attach(data);
        const std::string filename = path.string();
        dxfRW writer(filename.c_str());
        adapter.dxfW = &writer;

        std::map<std::uint32_t, std::size_t> calls;
        const auto firstResolver = [](std::uint32_t) { return 0x99u; };
        writer.setReferenceResolver(firstResolver);
        writer.setHandleRemap(
            {{0x10u, 0x44u}, {0x22u, 0x44u}, {0x32u, 0x55u}});
        writer.setReferenceResolver([&calls](std::uint32_t source) {
            ++calls[source];
            if (source == 0x10u) return 0x22u;
            if (source == 0x20u) return 0u;
            if (source == 0x21u) return 0x32u;
            if (source == 0x22u || source == 0x32u) return 0x33u;
            return source;
        });

        t.expect(writer.write(&adapter, DRW::AC1027, binary)
                     && adapter.directWritten && adapter.groupedWritten,
                 "reference resolver writes direct, app-data and table edges");
        std::vector<TableOwnerRecord> records;
        t.expect(readTableOwnerRecords(path, binary, records),
                 "reference resolver output has complete literal framing");
        std::vector<const TableOwnerRecord*> lines;
        const TableOwnerRecord* resolvedLayer = nullptr;
        const TableOwnerRecord* resolvedGroupedLayer = nullptr;
        for (const TableOwnerRecord& record : records) {
            if (record.kind == "LINE")
                lines.push_back(&record);
            if (record.kind == "LAYER"
                && groupValue(record.groups, 2) == "RESOLVER_LAYER")
                resolvedLayer = &record;
            if (record.kind == "LAYER"
                && groupValue(record.groups, 2) == "RESOLVER_GROUPED_LAYER")
                resolvedGroupedLayer = &record;
        }
        const auto appValues = [](const TableOwnerRecord& record,
                                  const std::string& opener, int code) {
            std::vector<std::string> values;
            bool inside = false;
            for (const auto& group : record.groups) {
                if (group.first == 102 && group.second == opener) {
                    inside = true;
                    continue;
                }
                if (inside && group.first == 102 && group.second == "}") {
                    inside = false;
                    continue;
                }
                if (inside && group.first == code)
                    values.push_back(group.second);
            }
            return values;
        };
        if (lines.size() == 2) {
            t.expect(appValues(*lines[0], "{ACAD_REACTORS", 330)
                         == std::vector<std::string>{"22"}
                         && appValues(*lines[0], "{ACAD_XDICTIONARY", 360).empty(),
                     "direct optional references map once and omit zero targets");
            t.expect(appValues(*lines[1], "{ACAD_REACTORS", 330)
                         == std::vector<std::string>{"22"}
                         && appValues(*lines[1], "{ACAD_XDICTIONARY", 360)
                             == std::vector<std::string>{"32"}
                         && appValues(*lines[1], "{FOREIGN_META", 330)
                             == std::vector<std::string>{"10"},
                     "only known 102 reference groups resolve; outputs are terminal");
            std::vector<std::string> xdata;
            for (const auto& group : lines[0]->groups)
                if (group.first == 1005)
                    xdata.push_back(group.second);
            t.expect(xdata == std::vector<std::string>{"0", "0000", "22"},
                     "XDATA retains literal zero spellings and resolves valid nonzero handles");
        } else {
            t.expect(false, "resolver sentinel contains both entity records");
        }
        t.expect(resolvedLayer != nullptr
                     && appValues(*resolvedLayer, "{ACAD_REACTORS", 330)
                         == std::vector<std::string>{"22"}
                     && appValues(*resolvedLayer, "{ACAD_XDICTIONARY", 360).empty(),
                 "table-record optional references resolve and omit absent targets");
        t.expect(resolvedGroupedLayer != nullptr
                     && appValues(*resolvedGroupedLayer, "{ACAD_REACTORS", 330)
                         == std::vector<std::string>{"22"}
                     && appValues(*resolvedGroupedLayer,
                                  "{ACAD_XDICTIONARY", 360)
                         == std::vector<std::string>{"32"},
                 "table-record known application groups resolve exactly once");
        t.expect(calls == std::map<std::uint32_t, std::size_t>{
                     {0x10u, 5u}, {0x20u, 6u}, {0x21u, 2u}},
                 "reference callbacks run once per nonzero audited reference only");

        const auto opaquePath = std::filesystem::temp_directory_path()
            / ("libdxfrw-reference-resolver-opaque-" + std::to_string(stamp)
               + (binary ? "-binary.dxf" : "-ascii.dxf"));
        struct OpaqueCleanup {
            std::filesystem::path path;
            ~OpaqueCleanup() {
                std::error_code error;
                std::filesystem::remove(path, error);
            }
        } opaqueCleanup{opaquePath};
        dx_data opaqueData;
        ResolverInterface opaqueAdapter;
        opaqueAdapter.attach(opaqueData);
        opaqueAdapter.includeMalformedXdata = true;
        const std::string opaqueFilename = opaquePath.string();
        dxfRW opaqueWriter(opaqueFilename.c_str());
        opaqueAdapter.dxfW = &opaqueWriter;
        opaqueWriter.setReferenceResolver(
            [](std::uint32_t source) { return source; });
        const bool opaqueWritten = opaqueWriter.write(
            &opaqueAdapter, DRW::AC1027, binary);
        std::ifstream opaqueStream(opaquePath, std::ios::binary);
        const std::string opaqueBytes(
            std::istreambuf_iterator<char>(opaqueStream), {});
        t.expect(opaqueWritten
                     && opaqueBytes.find("not-hex") != std::string::npos,
                 "resolver mode preserves malformed opaque XDATA spellings verbatim");
    }

    // A resolver cannot destroy or replace itself while executing inside a
    // write. The failed attempt leaves the old callback available for retry.
    const auto mutationPath = std::filesystem::temp_directory_path()
        / ("libdxfrw-reference-resolver-mutation-" + std::to_string(stamp)
           + ".dxf");
    struct MutationCleanup {
        std::filesystem::path path;
        ~MutationCleanup() {
            std::error_code error;
            std::filesystem::remove(path, error);
        }
    } mutationCleanup{mutationPath};
    class OneEdgeInterface final : public dx_iface {
    public:
        void writeEntities() override {
            DRW_Line line;
            line.secPoint = {1, 2, 3};
            line.reactorHandles = {0x10u};
            written = dxfW->writeLine(&line);
        }
        bool written {false};
    } activeAdapter;
    dx_data activeData;
    activeAdapter.cData = &activeData;
    activeAdapter.currentBlock = activeData.mBlock;
    const std::string mutationFilename = mutationPath.string();
    dxfRW mutationWriter(mutationFilename.c_str());
    activeAdapter.dxfW = &mutationWriter;
    bool replaceDuringCallback = true;
    mutationWriter.setReferenceResolver([&](std::uint32_t source) {
        if (replaceDuringCallback)
            mutationWriter.setReferenceResolver({});
        return source == 0x10u ? 0x20u : source;
    });
    {
        std::ofstream destination(mutationPath, std::ios::binary);
        destination << "resolver mutation sentinel\n";
    }
    t.expect(!mutationWriter.write(&activeAdapter, DRW::AC1027, false),
             "attempted resolver replacement during callback rejects write");
    {
        std::ifstream destination(mutationPath, std::ios::binary);
        t.expect(std::string(std::istreambuf_iterator<char>(destination), {})
                     == "resolver mutation sentinel\n",
                 "active resolver mutation preserves the published destination");
    }
    replaceDuringCallback = false;
    t.expect(mutationWriter.write(&activeAdapter, DRW::AC1027, false)
                 && activeAdapter.written,
             "failed active-mutation attempt retries with the original resolver");
    mutationWriter.setReferenceResolver({});
    t.expect(mutationWriter.write(&activeAdapter, DRW::AC1027, false),
             "resolver may be cleared between writes");

    const auto exceptionPath = std::filesystem::temp_directory_path()
        / ("libdxfrw-reference-resolver-exception-" + std::to_string(stamp)
           + ".dxf");
    struct ExceptionCleanup {
        std::filesystem::path path;
        ~ExceptionCleanup() {
            std::error_code error;
            std::filesystem::remove(path, error);
        }
    } exceptionCleanup{exceptionPath};
    OneEdgeInterface exceptionAdapter;
    dx_data exceptionData;
    exceptionAdapter.cData = &exceptionData;
    exceptionAdapter.currentBlock = exceptionData.mBlock;
    const std::string exceptionFilename = exceptionPath.string();
    dxfRW exceptionWriter(exceptionFilename.c_str());
    exceptionAdapter.dxfW = &exceptionWriter;
    bool throwFromResolver = true;
    std::size_t oldResolverCalls = 0;
    std::size_t replacementResolverCalls = 0;
    auto ownedCapture = std::make_shared<int>(7);
    std::weak_ptr<int> observedCapture = ownedCapture;
    exceptionWriter.setReferenceResolver(
        [capture = ownedCapture, &throwFromResolver, &oldResolverCalls](
            std::uint32_t source) {
            (void)capture;
            ++oldResolverCalls;
            if (throwFromResolver)
                throw std::runtime_error("resolver sentinel");
            return source == 0x10u ? 0x20u : source;
        });
    ownedCapture.reset();
    {
        std::ofstream destination(exceptionPath, std::ios::binary);
        destination << "resolver exception sentinel\n";
    }
    t.expect(!exceptionWriter.write(&exceptionAdapter, DRW::AC1027, false)
                 && oldResolverCalls == 1u && !observedCapture.expired(),
             "throwing resolver fails transaction while codec owns its closure");
    {
        std::ifstream destination(exceptionPath, std::ios::binary);
        t.expect(std::string(std::istreambuf_iterator<char>(destination), {})
                     == "resolver exception sentinel\n",
                 "resolver exception preserves the published destination");
    }
    exceptionWriter.setReferenceResolver(
        [&replacementResolverCalls](std::uint32_t source) {
            ++replacementResolverCalls;
            return source == 0x10u ? 0x33u : source;
        });
    t.expect(observedCapture.expired(),
             "replacing a resolver releases the prior owned closure");
    t.expect(exceptionWriter.write(&exceptionAdapter, DRW::AC1027, false)
                 && replacementResolverCalls == 1u,
             "replacement resolver supports retry after callback exception");
    std::vector<TableOwnerRecord> retryRecords;
    t.expect(readTableOwnerRecords(exceptionPath, false, retryRecords),
             "resolver exception retry produces complete DXF framing");
    const auto retryLine = std::find_if(retryRecords.begin(), retryRecords.end(),
        [](const TableOwnerRecord& record) { return record.kind == "LINE"; });
    if (retryLine != retryRecords.end()) {
        bool insideReactors = false;
        std::vector<std::string> resolvedHandles;
        for (const auto& group : retryLine->groups) {
            if (group.first == 102 && group.second == "{ACAD_REACTORS")
                insideReactors = true;
            else if (insideReactors && group.first == 102 && group.second == "}")
                insideReactors = false;
            else if (insideReactors && group.first == 330)
                resolvedHandles.push_back(group.second);
        }
        t.expect(resolvedHandles == std::vector<std::string>{"33"},
                 "retry emits the replacement resolver result, not stale callback state");
    } else {
        t.expect(false, "resolver exception retry contains its LINE record");
    }

    const auto finalizePath = std::filesystem::temp_directory_path()
        / ("libdxfrw-reference-resolver-finalize-" + std::to_string(stamp)
           + ".dxf");
    struct FinalizeCleanup {
        std::filesystem::path path;
        ~FinalizeCleanup() {
            std::error_code error;
            std::filesystem::remove(path, error);
        }
    } finalizeCleanup{finalizePath};
    class FinalizeResolverInterface final : public dx_iface {
    public:
        void writeEntities() override {
            DRW_Line line;
            line.secPoint = {1, 2, 3};
            line.reactorHandles = {0x10u};
            written = dxfW->writeLine(&line);
        }
        bool finalizeDxfWrite() override {
            if (attemptResolverChange)
                dxfW->setReferenceResolver({});
            return true;
        }
        bool attemptResolverChange {true};
        bool written {false};
    } finalizeAdapter;
    dx_data finalizeData;
    finalizeAdapter.cData = &finalizeData;
    finalizeAdapter.currentBlock = finalizeData.mBlock;
    const std::string finalizeFilename = finalizePath.string();
    dxfRW finalizeWriter(finalizeFilename.c_str());
    finalizeAdapter.dxfW = &finalizeWriter;
    finalizeWriter.setReferenceResolver(
        [](std::uint32_t source) { return source == 0x10u ? 0x20u : source; });
    {
        std::ofstream destination(finalizePath, std::ios::binary);
        destination << "resolver finalize sentinel\n";
    }
    t.expect(!finalizeWriter.write(&finalizeAdapter, DRW::AC1027, false),
             "resolver cannot be changed during publication finalization");
    {
        std::ifstream destination(finalizePath, std::ios::binary);
        t.expect(std::string(std::istreambuf_iterator<char>(destination), {})
                     == "resolver finalize sentinel\n",
                 "finalizer resolver mutation preserves the published destination");
    }
    finalizeAdapter.attemptResolverChange = false;
    t.expect(finalizeWriter.write(&finalizeAdapter, DRW::AC1027, false)
                 && finalizeAdapter.written,
             "finalizer mutation rejection does not prevent a later write retry");
}

void testDxfWriteBlockRemapPhase(TestContext& t) {
    constexpr std::uint32_t sourceBlockRecord = 0xA200u;
    constexpr std::uint32_t dictionaryHandle = 0xB100u;
    constexpr std::uint32_t callerOverride = 0xC100u;
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-block-remap-" + std::to_string(nonce) + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{path};

    class BlockRemapWriter final : public dx_iface {
    public:
        void writeBlockRecords() override {
            precollected = dxfW->writeBlockRecord("MappedBlock");
            if (registerTooEarly)
                dxfW->addDxfWriteHandleRemap(sourceHandle, 0x555u);
        }

        void writeBlocks() override {
            const auto target = dxfW->getBlockRecordHandleToWrite("MappedBlock");
            targetHandle = target;
            beforeRegistration = dxfW->remapHandle(sourceHandle);
            if (registerMapping) {
                const std::uint32_t source = invalidSource ? 0u : sourceHandle;
                const std::uint32_t emitted = invalidTarget ? 0u : target;
                dxfW->addDxfWriteHandleRemap(source, emitted);
                if (duplicate)
                    dxfW->addDxfWriteHandleRemap(sourceHandle, target);
                if (conflict)
                    dxfW->addDxfWriteHandleRemap(sourceHandle, target + 1u);
            }
            afterRegistration = dxfW->remapHandle(sourceHandle);
            DRW_Block block;
            block.name = "MappedBlock";
            blockWritten = dxfW->writeBlock(&block);
            if (failAfterRegistration)
                (void)dxfW->writeLine(nullptr);
            if (throwAfterRegistration)
                throw std::runtime_error("block-remap callback failure");
        }

        void writeEntities() override {}

        std::uint32_t sourceHandle{0xA200u};
        bool registerMapping{true};
        bool registerTooEarly{false};
        bool duplicate{false};
        bool conflict{false};
        bool invalidSource{false};
        bool invalidTarget{false};
        bool failAfterRegistration{false};
        bool throwAfterRegistration{false};
        bool precollected{false};
        bool blockWritten{false};
        std::uint32_t targetHandle{DRW::NoHandle};
        std::uint32_t beforeRegistration{0};
        std::uint32_t afterRegistration{0};
    };

    const auto dictionary = [=]() {
        DRW_Dictionary value;
        value.handle = dictionaryHandle;
        value.parentHandle = static_cast<int>(sourceBlockRecord);
        return value;
    };
    const auto ownerOfDictionary = [&path](const std::string& handle) {
        std::ifstream input(path, std::ios::binary);
        dxfReaderAscii reader(&input);
        int code = 0;
        bool inDictionary = false;
        bool matchingHandle = false;
        while (reader.readRec(&code)) {
            const std::string value = reader.getRawValue();
            if (code == 0) {
                inDictionary = value == "DICTIONARY";
                matchingHandle = false;
            } else if (inDictionary && code == 5) {
                matchingHandle = value == handle;
            } else if (inDictionary && matchingHandle && code == 330) {
                return value;
            }
        }
        return std::string{};
    };
    const auto configure = [&dictionary](dxfRW& codec) {
        codec.setNamedDictObjects({dictionary()});
    };

    dxfRW codec(path.string().c_str());
    configure(codec);
    dx_data data;
    BlockRemapWriter adapter;
    adapter.cData = &data;
    adapter.currentBlock = data.mBlock;
    adapter.dxfW = &codec;
    adapter.duplicate = true;
    const bool firstWrite = codec.write(&adapter, DRW::AC1024, false);
    t.expect(firstWrite && adapter.precollected && adapter.blockWritten
                 && adapter.targetHandle != DRW::NoHandle
                 && adapter.beforeRegistration == sourceBlockRecord
                 && adapter.afterRegistration == adapter.targetHandle
                 && codec.remapHandle(sourceBlockRecord) == adapter.targetHandle
                 && ownerOfDictionary(codec.toHexStrHandle(dictionaryHandle))
                        == codec.toHexStrHandle(adapter.targetHandle),
             "derived block remap is accepted after BLOCK_RECORD precollection and rewrites object owners");

    adapter.registerMapping = false;
    const bool nextWrite = codec.write(&adapter, DRW::AC1024, false);
    t.expect(nextWrite && adapter.beforeRegistration == sourceBlockRecord
                 && adapter.afterRegistration == sourceBlockRecord
                 && ownerOfDictionary(codec.toHexStrHandle(dictionaryHandle))
                        == codec.toHexStrHandle(sourceBlockRecord),
             "a new write clears prior derived mappings while preserving caller setup");

    adapter.registerMapping = true;
    adapter.failAfterRegistration = true;
    const bool failedWrite = codec.write(&adapter, DRW::AC1024, false);
    t.expect(!failedWrite
                 && codec.remapHandle(sourceBlockRecord) == sourceBlockRecord,
             "failed block emission discards its derived handle remaps");
    adapter.failAfterRegistration = false;
    const bool retryWrite = codec.write(&adapter, DRW::AC1024, false);
    t.expect(retryWrite && adapter.beforeRegistration == sourceBlockRecord
                 && adapter.afterRegistration == adapter.targetHandle,
             "same-writer retry starts with a clean derived-remap generation");

    adapter.conflict = true;
    const bool conflictWrite = codec.write(&adapter, DRW::AC1024, false);
    t.expect(!conflictWrite
                 && codec.remapHandle(sourceBlockRecord) == sourceBlockRecord,
             "conflicting duplicate derived mappings reject transactionally");
    adapter.conflict = false;
    adapter.registerTooEarly = true;
    const bool earlyWrite = codec.write(&adapter, DRW::AC1024, false);
    t.expect(!earlyWrite,
             "writeBlockRecords cannot register BLOCK_RECORD handle remaps before writeBlocks");
    adapter.registerTooEarly = false;
    adapter.invalidSource = true;
    t.expect(!codec.write(&adapter, DRW::AC1024, false),
             "null source handle cannot be registered as a block remap");
    adapter.invalidSource = false;
    adapter.invalidTarget = true;
    t.expect(!codec.write(&adapter, DRW::AC1024, false),
             "null emitted handle cannot be registered as a block remap");
    adapter.invalidTarget = false;
    const bool recoveredWrite = codec.write(&adapter, DRW::AC1024, false);
    t.expect(recoveredWrite,
             "phase rejection does not poison a subsequent valid write");

    dxfRW explicitCodec(path.string().c_str());
    configure(explicitCodec);
    explicitCodec.setHandleRemap({{sourceBlockRecord, callerOverride}});
    dx_data explicitData;
    BlockRemapWriter explicitAdapter;
    explicitAdapter.cData = &explicitData;
    explicitAdapter.currentBlock = explicitData.mBlock;
    explicitAdapter.dxfW = &explicitCodec;
    const bool explicitWrite = explicitCodec.write(
        &explicitAdapter, DRW::AC1024, false);
    t.expect(explicitWrite && explicitAdapter.targetHandle != callerOverride
                 && explicitAdapter.afterRegistration == callerOverride
                 && explicitCodec.remapHandle(sourceBlockRecord) == callerOverride
                 && ownerOfDictionary(explicitCodec.toHexStrHandle(dictionaryHandle))
                        == explicitCodec.toHexStrHandle(callerOverride),
             "caller-supplied remap wins a collision with derived BLOCK_RECORD identity");
    explicitAdapter.registerMapping = false;
    const bool explicitRetry = explicitCodec.write(
        &explicitAdapter, DRW::AC1024, false);
    t.expect(explicitRetry
                 && explicitAdapter.beforeRegistration == callerOverride
                 && explicitCodec.remapHandle(sourceBlockRecord) == callerOverride
                 && ownerOfDictionary(explicitCodec.toHexStrHandle(dictionaryHandle))
                        == explicitCodec.toHexStrHandle(callerOverride),
             "caller remaps persist across writes independently of derived-map reset");

    dxfRW throwingCodec(path.string().c_str());
    configure(throwingCodec);
    dx_data throwingData;
    BlockRemapWriter throwingAdapter;
    throwingAdapter.cData = &throwingData;
    throwingAdapter.currentBlock = throwingData.mBlock;
    throwingAdapter.dxfW = &throwingCodec;
    throwingAdapter.throwAfterRegistration = true;
    t.expect(!throwingCodec.write(&throwingAdapter, DRW::AC1024, false)
                 && throwingCodec.remapHandle(sourceBlockRecord)
                        == sourceBlockRecord,
             "callback exception releases the phase guard and derived map");
    throwingAdapter.throwAfterRegistration = false;
    t.expect(throwingCodec.write(&throwingAdapter, DRW::AC1024, false),
             "same writer recovers after a throwing remap callback");
}

void testDxfWriteLeftOutReporting(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-left-out-" + std::to_string(nonce) + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{path};

    class OmissionAdapter final : public dx_iface {
    public:
        void writeEntities() override {
            if (!emitUnsupported)
                return;
            (void)dxfW->writeLight(&light);
            (void)dxfW->writeLight(&light);
            (void)dxfW->writeMPolygon(&polygon);
        }

        bool emitUnsupported {true};
        DRW_Light light;
        DRW_MPolygon polygon;
    };

    {
        std::ofstream previous(path, std::ios::binary | std::ios::trunc);
        previous << "previous published drawing\n";
    }

    dxfRW codec(path.string().c_str());
    dx_data data;
    OmissionAdapter adapter;
    adapter.cData = &data;
    adapter.currentBlock = data.mBlock;
    adapter.dxfW = &codec;

    t.expect(!codec.writeLight(nullptr) && codec.leftOut().empty(),
             "a rejected direct writer call outside an admitted attempt is not reported as an omission");
    const bool rejected = !codec.write(&adapter, DRW::AC1009, false);
    const auto& report = codec.leftOut();
    const auto light = report.find("LIGHT (not in this DXF version)");
    const auto mp = report.find("MPOLYGON (not in this DXF version)");
    t.expect(rejected && report.size() == 2
                 && light != report.end() && light->second == 2
                 && mp != report.end() && mp->second == 1,
             "strict target rejection reports each encountered omission by record and count");
    {
        std::ifstream previous(path, std::ios::binary);
        t.expect(std::string(std::istreambuf_iterator<char>(previous), {})
                     == "previous published drawing\n",
                 "omission diagnostics do not publish a lossy or partial destination");
    }

    const auto* reportAddress = &report;
    adapter.emitUnsupported = false;
    t.expect(codec.write(&adapter, DRW::AC1015, false)
                 && reportAddress == &codec.leftOut() && report.empty(),
             "the borrowed report object remains stable and resets on the next admitted write");

    adapter.emitUnsupported = true;
    t.expect(!codec.write(&adapter, DRW::AC1009, false) && report.size() == 2,
             "a later failed attempt publishes only its own observed omissions");
    t.expect(!codec.write(&adapter, DRW::UNKNOWNV, false) && report.empty(),
             "an early rejected attempt clears stale omission counts");
}

using TextEncoding = DRW::DxfTextEncoding;

void testDxfTextEncodingCodecs(TestContext& t) {
    const std::string source = u8"café";
    const std::string declared = u8"cafÃ©";
    const std::string escaped = source + "\\U+4E2D\\M+5D6D0";
    for (const DRW::Version version : {DRW::UNKNOWNV, DRW::AC1004,
            DRW::AC1006, DRW::AC1009, DRW::AC1012, DRW::AC1014,
            DRW::AC1015, DRW::AC1018, DRW::AC1021, DRW::AC1024,
            DRW::AC1027, DRW::AC1032}) {
        for (const std::string page : {"", "ANSI_1252", "ANSI_1251", "ANSI_936"}) {
            for (const bool dxf : {false, true}) {
                DRW_TextCodec ordinary;
                DRW_TextCodec recovery;
                recovery.setDxfTextEncoding(TextEncoding::LegacyLibreCadUtf8);
                ordinary.setVersion(version, dxf);
                recovery.setVersion(version, dxf);
                if (!page.empty()) {
                    ordinary.setCodePage(page, dxf);
                    recovery.setCodePage(page, dxf);
                }
                const bool eligible = dxf && version >= DRW::AC1006
                    && version <= DRW::AC1018
                    && (page.empty() || page == "ANSI_1252");
                const std::string ordinaryValue = ordinary.toUtf8(escaped);
                t.expect(recovery.toUtf8(escaped)
                             == (eligible ? source + u8"中中" : ordinaryValue),
                         "recovery applies only to known legacy default/1252 DXF, not DWG or other pages");
                if (eligible)
                    t.expect(ordinaryValue == declared + u8"中中",
                             "Declared remains actual CP1252, visibly distinct from opt-in recovery");
                t.expect(recovery.getSourceVersion() == ordinary.getSourceVersion()
                             && recovery.hasSourceVersion() == ordinary.hasSourceVersion()
                             && recovery.getCodePage() == ordinary.getCodePage()
                             && recovery.fromUtf8(source) == ordinary.fromUtf8(source),
                         "input mode leaves source metadata and output conversion unchanged");
            }
        }
    }

    DRW_TextCodec recovery;
    recovery.setDxfTextEncoding(TextEncoding::LegacyLibreCadUtf8);
    recovery.setCodePage("ANSI_1251", true);
    recovery.setVersion(DRW::AC1018, true);
    t.expect(recovery.toUtf8(source) == declared,
             "internal version default cannot enable recovery after explicit non-1252 page");
    recovery.setCodePage("ANSI_1252", true);
    t.expect(recovery.toUtf8(source) == source,
             "later explicit 1252 declaration enables recovery again");
    DRW_TextCodec ordinary;
    ordinary.setVersion(DRW::AC1018, true);
    ordinary.swap(recovery);
    t.expect(ordinary.toUtf8(source) == source && recovery.toUtf8(source) == declared,
             "codec swap carries mode and eligibility together");
    DRW_TextCodec moved(std::move(ordinary));
    t.expect(moved.toUtf8(source) == source,
             "codec move preserves mode and eligibility");
}

class TextEncodingCapture final : public dx_iface {
public:
    DRW_Header value;
    std::vector<std::string> texts;
    std::vector<std::pair<int, std::string>> xdataText;
    std::vector<std::string> appIds;
    std::vector<DRW_RawDxfObject> raw;
    dxfRW* facade {nullptr};
    bool switchAtHeader {false};
    bool throwAtText {false};
    bool nested {false};
    bool nestedPreserved {true};
    bool nestedResult {true};
    unsigned nestedCalls {0};

    void addHeader(const DRW_Header* header) override {
        value = *header;
        if (switchAtHeader)
            facade->setDxfTextEncoding(TextEncoding::Declared);
    }
    void addText(const DRW_Text& text) override {
        texts.push_back(text.text);
        for (const auto& value : text.extData) {
            if (value != nullptr && (value->code() == 1000 || value->code() == 1003)
                && value->type() == DRW_Variant::STRING)
                xdataText.emplace_back(value->code(), value->c_str());
        }
        if (nested) {
            ++nestedCalls;
            const auto before = facade->getLastDiagnostic();
            const auto beforeError = facade->getError();
            std::string invalid("not a DXF");
            nestedResult = facade->readAscii(nullptr, false, invalid);
            const auto after = facade->getLastDiagnostic();
            nestedPreserved = nestedPreserved && before.operation == after.operation
                && before.phase == after.phase && before.cause == after.cause
                && before.code == after.code && before.message == after.message
                && before.offset == after.offset && before.handle == after.handle
                && before.hasOffset == after.hasOffset && before.hasHandle == after.hasHandle
                && before.secondary.size() == after.secondary.size()
                && beforeError == facade->getError();
        }
        if (throwAtText) {
            facade->setDxfTextEncoding(TextEncoding::LegacyLibreCadUtf8);
            throw std::runtime_error("text-mode callback sentinel");
        }
    }
    void addRawDxfEntity(const DRW_RawDxfObject& entity) override {
        raw.push_back(entity);
    }
    void addAppId(const DRW_AppId& app) override { appIds.push_back(app.name); }
};

std::string textEncodingBytes(unsigned route,
    const std::vector<std::pair<int, std::string>>& metadata,
    const std::string& source) {
    std::ostringstream bytes;
    if (route >= 2)
        bytes.write("AutoCAD Binary DXF\r\n\x1A\0", 22);
    std::unique_ptr<dxfWriter> writer;
    if (route == 3)
        writer = std::make_unique<dxfWriterBinaryR12>(&bytes);
    else if (route == 2)
        writer = std::make_unique<dxfWriterBinary>(&bytes);
    else
        writer = std::make_unique<dxfWriterAscii>(&bytes);
    writer->writeString(0, "SECTION");
    writer->writeString(2, "HEADER");
    // Deliberately before all metadata: deferred HEADER interpretation must
    // use the same input mode and final metadata as later typed TEXT.
    writer->writeString(9, "$CLAYER");
    writer->writeString(8, source);
    for (const auto& field : metadata)
        writer->writeString(field.first, field.second);
    writer->writeString(0, "ENDSEC");
    writer->writeString(0, "SECTION");
    writer->writeString(2, "TABLES");
    writer->writeString(0, "TABLE");
    writer->writeString(2, "APPID");
    writer->writeString(5, "C0");
    writer->writeString(330, "0");
    writer->writeString(100, "AcDbSymbolTable");
    writer->writeInt16(70, 2);
    unsigned appHandle = 0xC1u;
    for (const char* app : {"TEXT_MODE_TYPED", "TEXT_MODE_RAW"}) {
        writer->writeString(0, "APPID");
        writer->writeString(5, appHandle++ == 0xC1u ? "C1" : "C2");
        writer->writeString(330, "C0");
        writer->writeString(100, "AcDbSymbolTableRecord");
        writer->writeString(100, "AcDbRegAppTableRecord");
        writer->writeString(2, app);
        writer->writeInt16(70, 0);
    }
    writer->writeString(0, "ENDTAB");
    writer->writeString(0, "ENDSEC");
    writer->writeString(0, "SECTION");
    writer->writeString(2, "ENTITIES");
    for (const char* handle : {"A0", "A1"}) {
        writer->writeString(0, "TEXT");
        writer->writeString(5, handle);
        writer->writeString(8, "0");
        writer->writeDouble(10, 1.0);
        writer->writeDouble(20, 2.0);
        writer->writeDouble(40, 2.5);
        writer->writeString(1, source);
        writer->writeString(1001, "TEXT_MODE_TYPED");
        writer->writeString(1000, source);
        writer->writeString(1003, source);
    }
    writer->writeString(0, "LOCAL_ENCODING_RAW");
    writer->writeString(5, "B0");
    writer->writeString(1001, "TEXT_MODE_RAW");
    writer->writeString(1000, source);
    writer->writeString(0, "ENDSEC");
    writer->writeString(0, "EOF");
    return bytes.str();
}

bool readTextEncodingBytes(dxfRW& reader, TextEncodingCapture& capture,
    unsigned route, std::string bytes, const std::filesystem::path& path) {
    capture.facade = &reader;
    if (route == 0)
        return reader.readAscii(&capture, false, bytes);
    { std::ofstream output(path, std::ios::binary); output << bytes; }
    return reader.read(&capture, false);
}

void testDxfTextEncodingReads(TestContext& t) {
    struct EncodingCase {
        std::vector<std::pair<int, std::string>> metadata;
        std::string declared;
        bool eligible;
        DRW::Version version;
        std::string page;
    };
    const std::string source = u8"café";
    const std::string declared = u8"cafÃ©";
    DRW_TextCodec cyrillic;
    cyrillic.setCodePage("ANSI_1251", true);
    const std::string cp1251 = cyrillic.toUtf8(source);
    const std::vector<EncodingCase> cases{
        {{{9, "$ACADVER"}, {1, "AC1018"}}, declared, true, DRW::AC1018, ""},
        {{{9, "$DWGCODEPAGE"}, {3, "ANSI_1252"}, {9, "$ACADVER"}, {1, "AC1018"}},
         declared, true, DRW::AC1018, "ANSI_1252"},
        {{{9, "$ACADVER"}, {1, "AC1018"}, {9, "$DWGCODEPAGE"}, {3, "ANSI_1252"}},
         declared, true, DRW::AC1018, "ANSI_1252"},
        {{{9, "$DWGCODEPAGE"}, {3, "ANSI_1251"}, {9, "$ACADVER"}, {1, "AC1018"}},
         cp1251, false, DRW::AC1018, "ANSI_1251"},
        {{{9, "$ACADVER"}, {1, "AC1018"}, {9, "$DWGCODEPAGE"}, {3, "ANSI_1251"}},
         cp1251, false, DRW::AC1018, "ANSI_1251"},
        {{{9, "$DWGCODEPAGE"}, {3, "ANSI_1251"}, {9, "$DWGCODEPAGE"}, {3, "ANSI_1252"},
          {9, "$ACADVER"}, {1, "AC1018"}}, declared, true, DRW::AC1018, "ANSI_1252"},
        {{{9, "$DWGCODEPAGE"}, {3, "ANSI_1252"}, {9, "$DWGCODEPAGE"}, {3, "ANSI_1251"},
          {9, "$ACADVER"}, {1, "AC1018"}}, cp1251, false, DRW::AC1018, "ANSI_1251"},
        {{{9, "$DWGCODEPAGE"}, {3, "ANSI_1251"}, {9, "$ACADVER"}, {1, "AC1021"}},
         source, false, DRW::AC1021, "ANSI_1251"},
        {{{9, "$ACADVER"}, {1, "AC1009"}}, declared, true, DRW::AC1009, ""},
        {{{9, "$DWGCODEPAGE"}, {3, "ANSI_1251"}, {9, "$ACADVER"}, {1, "AC1009"}},
         cp1251, false, DRW::AC1009, "ANSI_1251"},
        {{{9, "$ACADVER"}, {1, "AC1021"}, {9, "$ACADVER"}, {1, "AC1009"},
          {9, "$DWGCODEPAGE"}, {3, "ANSI_1252"}}, source, false, DRW::AC1021, "ANSI_1252"},
        {{{9, "$ACADVER"}, {1, "AC1018"}, {9, "$ACADVER"}, {1, "AC9999"},
          {9, "$DWGCODEPAGE"}, {3, "ANSI_1252"}}, declared, true, DRW::AC1018, "ANSI_1252"},
        {{{9, "$ACADVER"}, {1, "AC1004"}, {9, "$DWGCODEPAGE"}, {3, "ANSI_1252"}},
         declared, false, DRW::AC1004, "ANSI_1252"},
        {{{9, "$ACADVER"}, {1, "AC9999"}, {9, "$DWGCODEPAGE"}, {3, "ANSI_1252"}},
         declared, false, DRW::UNKNOWNV, "ANSI_1252"},
        {{{9, "$DWGCODEPAGE"}, {3, "ANSI_1252"}},
         declared, false, DRW::UNKNOWNV, "ANSI_1252"}};
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-text-encoding-" + std::to_string(nonce) + ".dxf");
    for (unsigned route = 0; route != 4; ++route) {
        for (const auto& item : cases) {
            const std::string bytes = textEncodingBytes(route, item.metadata, source);
            // Fresh facade proves default Declared; same facade then proves
            // persistence and explicit switching in both directions.
            dxfRW reader(path.string().c_str());
            for (unsigned mode = 0; mode != 4; ++mode) {
                if (mode != 0)
                    reader.setDxfTextEncoding(mode <= 2
                        ? TextEncoding::LegacyLibreCadUtf8 : TextEncoding::Declared);
                TextEncodingCapture capture;
                const std::string expected = mode == 1 || mode == 2
                    ? (item.eligible ? source : item.declared) : item.declared;
                const bool read = readTextEncodingBytes(reader, capture, route, bytes, path);
                t.expect(read && capture.texts == std::vector<std::string>(2, expected)
                             && headerStringIs(capture.value, "$CLAYER", expected),
                         "all four reader routes snapshot persistent input mode and finalize HEADER consistently");
                t.expect(capture.xdataText == std::vector<std::pair<int, std::string>>{
                             {1000, expected}, {1003, expected},
                             {1000, expected}, {1003, expected}},
                         "typed XDATA semantic getter uses the same mode exactly once with exact count/order");
                t.expect(capture.appIds == std::vector<std::string>{"TEXT_MODE_TYPED", "TEXT_MODE_RAW"},
                         "typed and raw XDATA applications are explicitly registered in source tables");
                t.expect(capture.value.dxfSourceVersion() == item.version
                             && (item.page.empty()
                                 ? capture.value.vars.count("$DWGCODEPAGE") == 0
                                 : headerStringIs(capture.value, "$DWGCODEPAGE", item.page)),
                         "mode does not rewrite source version or codepage metadata");
                bool retained = capture.raw.size() == 1;
                if (retained) {
                    bool found = false;
                    for (std::size_t index = 0; index < capture.raw.front().groups.size(); ++index) {
                        const auto& group = capture.raw.front().groups[index];
                        if (group.code() == 1000) {
                            found = std::string(group.c_str()) == source;
                            if (route < 2)
                                found = found && capture.raw.front().rawValues[index] == source;
                        }
                    }
                    retained = found;
                }
                t.expect(retained, "input recovery never transcodes opaque raw carrier bytes");
            }
        }
    }
    std::error_code error;
    std::filesystem::remove(path, error);
}

void testDxfTextEncodingLifecycle(TestContext& t) {
    const std::string source = u8"café";
    const std::string declared = u8"cafÃ©";
    const std::vector<std::pair<int, std::string>> metadata{
        {9, "$ACADVER"}, {1, "AC1018"}, {9, "$DWGCODEPAGE"}, {3, "ANSI_1252"}};
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-text-mode-lifecycle-" + std::to_string(nonce) + ".dxf");
    for (unsigned route = 0; route != 4; ++route) {
        const std::string bytes = textEncodingBytes(route, metadata, source);
        dxfRW reader(path.string().c_str());
        reader.setDxfTextEncoding(TextEncoding::LegacyLibreCadUtf8);
        TextEncodingCapture first;
        first.switchAtHeader = true;
        first.nested = true;
        t.expect(readTextEncodingBytes(reader, first, route, bytes, path)
                     && first.texts == std::vector<std::string>(2, source)
                     && first.nestedCalls == 2 && !first.nestedResult && first.nestedPreserved,
                 "callback mode change and rejected nested read cannot reconfigure active reader");
        TextEncodingCapture next;
        t.expect(readTextEncodingBytes(reader, next, route, bytes, path)
                     && next.texts == std::vector<std::string>(2, declared),
                 "callback setter persists for next admitted read");
        TextEncodingCapture throwing;
        throwing.throwAtText = true;
        bool caught = false;
        try {
            readTextEncodingBytes(reader, throwing, route, bytes, path);
        } catch (const std::runtime_error& error) {
            caught = std::string(error.what()) == "text-mode callback sentinel";
        }
        t.expect(caught && throwing.texts == std::vector<std::string>{declared},
                 "callback exception propagates while preserving requested next-read mode");
        std::string invalid("invalid DXF");
        t.expect(!reader.readAscii(nullptr, false, invalid),
                 "early failed read after exception cannot erase requested input mode");
        TextEncodingCapture failed;
        t.expect(!readTextEncodingBytes(reader, failed, route, "invalid DXF", path),
                 "ordinary failed read cannot erase requested input mode");
        TextEncodingCapture recovered;
        t.expect(readTextEncodingBytes(reader, recovered, route, bytes, path)
                     && recovered.texts == std::vector<std::string>(2, source),
                 "same facade recovers after throwing and failed reads with persistent selected mode");
    }
    std::error_code error;
    std::filesystem::remove(path, error);
}

void testDxfTextEncodingExportIsolation(TestContext& t) {
    class EncodingWriter final : public dx_iface {
    public:
        dxfRW* facade {nullptr};
        DRW_Text text;
        bool emitted {false};
        bool configureDuringWrite {false};
        bool finalizeDxfWrite() override {
            if (configureDuringWrite)
                facade->setDxfTextEncoding(TextEncoding::LegacyLibreCadUtf8);
            return true;
        }
        void writeHeader(DRW_Header& header) override {
            header.addStr("$DWGCODEPAGE", "ANSI_1252", 3);
            if (configureDuringWrite)
                facade->setDxfTextEncoding(TextEncoding::Declared);
        }
        void writeEntities() override { emitted = facade->writeText(&text); }
    } adapter;
    adapter.text.text = u8"café";
    adapter.text.height = 1;
    dx_data data;
    adapter.cData = &data;
    adapter.currentBlock = data.mBlock;
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-text-mode-export-" + std::to_string(nonce) + ".dxf");
    dxfRW facade(path.string().c_str());
    adapter.facade = &facade;
    for (const bool binary : {false, true}) {
      for (const bool configure : {false, true}) {
        adapter.configureDuringWrite = configure;
        for (const auto mode : {TextEncoding::Declared, TextEncoding::LegacyLibreCadUtf8}) {
            facade.setDxfTextEncoding(mode);
            t.expect(facade.write(&adapter, DRW::AC1018, binary) && adapter.emitted,
                     "input mode and callback/finalizer setters never alter export encoding");
            std::ifstream stream(path, std::ios::binary);
            if (binary) stream.seekg(22);
            std::unique_ptr<dxfReader> tags = binary
                ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&stream))
                : std::unique_ptr<dxfReader>(new dxfReaderAscii(&stream));
            int code = 0;
            bool text = false;
            while (tags->readRec(&code)) {
                if (code == 1 && tags->getRawValue() == std::string("caf\xE9"))
                    text = true;
            }
            t.expect(text && adapter.text.text == u8"café",
                     "export emits declared CP1252 bytes without mutating caller semantic text");
            tags.reset();
            stream.close();
            TextEncodingCapture next;
            const std::string expected = configure
                || mode == TextEncoding::LegacyLibreCadUtf8 ? u8"café" : u8"cafÃ©";
            t.expect(readTextEncodingBytes(facade, next, 0,
                         textEncodingBytes(0, {{9, "$ACADVER"}, {1, "AC1018"}}, u8"café"), path)
                         && next.texts == std::vector<std::string>(2, expected),
                     "write finalizer configures only a subsequent read");
        }
      }
    }
    std::error_code error;
    std::filesystem::remove(path, error);
}

void testXdataTextWritePolicy(TestContext& t) {
    struct TextPolicyCase {
        int code;
        std::string value;
        bool legacy;
        bool modern;
    };
    std::string e127, e128, cjk36, cjk37;
    for (int i = 0; i < 128; ++i) e128 += u8"é";
    e127 = e128.substr(0, 254) + "A";
    for (int i = 0; i < 37; ++i) cjk37 += u8"一";
    cjk36 = cjk37.substr(0, 108);
    const std::vector<TextPolicyCase> cases{
        {1000, std::string(255, 'x'), true, true},
        {1000, std::string(256, 'x'), false, false},
        {1000, e127, true, true}, {1000, e128, true, false},
        {1000, cjk36, true, true}, {1000, cjk37, false, true},
        {1001, std::string(31, 'A'), true, true},
        {1001, std::string(32, 'A'), false, false},
        {1000, u8"😀", false, true}, {1003, u8"😀", false, true},
        {1000, "ordinary backslash \\path", true, true},
        // Literal CIF-looking semantic text cannot round-trip under the
        // current escape contract. Reject that typed route observably rather
        // than silently reinterpret it; PAR-85/7AR owns the eventual policy.
        {1000, "literal \\U+0041", false, false},
        {1000, std::string("\xE9"), false, false},
        {1000, std::string("\xED\xA0\x80"), false, false},
        {1000, std::string("\xC0\xAF"), false, false},
        {1000, std::string("\xE2\x82"), false, false}};
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    unsigned index = 0;
    for (const auto& item : cases) {
        for (const bool modern : {false, true}) {
            for (const bool binary : {false, true}) {
                for (const bool table : {false, true}) {
                    dx_data data;
                    DRW_AppId app;
                    app.name = item.code == 1001 ? item.value : "STRINGS";
                    data.appIds.push_back(app);
                    const DRW_Variant application(1001, app.name);
                    const DRW_Variant value(item.code, item.value);
                    if (table) {
                        DRW_Layer layer;
                        layer.name = "XDATA_TEXT";
                        t.expect(layer.addExtData(std::make_unique<DRW_Variant>(application)),
                                 "table text policy installs application");
                        if (item.code != 1001)
                            t.expect(layer.addExtData(std::make_unique<DRW_Variant>(value)),
                                     "table text policy installs text");
                        data.layers.push_back(layer);
                    } else {
                        auto line = std::make_unique<DRW_Line>();
                        line->secPoint.x = 1;
                        line->extData.push_back(std::make_shared<DRW_Variant>(application));
                        if (item.code != 1001)
                            line->extData.push_back(std::make_shared<DRW_Variant>(value));
                        data.mBlock->ent.push_back(line.release());
                    }
                    const auto path = std::filesystem::temp_directory_path()
                        / ("libdxfrw-xdata-text-policy-" + std::to_string(nonce)
                           + "-" + std::to_string(index++) + ".dxf");
                    const std::string sentinel("original destination\n");
                    { std::ofstream stream(path, std::ios::binary); stream << sentinel; }
                    FixtureInterface interface_;
                    const bool written = interface_.fileExport(path.string(),
                        modern ? DRW::AC1021 : DRW::AC1018, binary, &data, false);
                    const bool wanted = modern ? item.modern : item.legacy;
                    t.expect(written == wanted,
                             "XDATA text policy uses scalar validity/loss and encoded-byte limits");
                    const DRW_Variant* retained = table ? data.layers.front().extData.back()
                        : data.mBlock->ent.front()->extData.back().get();
                    t.expect(retained != nullptr && std::string(retained->c_str()) == item.value,
                             "text validation does not mutate caller XDATA on success/rejection");
                    if (!wanted) {
                        std::ifstream stream(path, std::ios::binary);
                        const std::string actual((std::istreambuf_iterator<char>(stream)), {});
                        t.expect(actual == sentinel,
                                 "rejected XDATA text preserves existing destination transactionally");
                    }
                    std::error_code error;
                    std::filesystem::remove(path, error);
                }
            }
        }
    }
}

void testRawClassEntity(TestContext& t) {
    FixtureInterface interface_;
    dx_data data;
    t.expect(importFixture("classes_raw_entity_r2007.dxf", interface_, data),
             "raw custom-class fixture imports");
    t.expect(interface_.classes.size() == 1,
             "raw custom-class fixture publishes one class");
    t.expect(interface_.rawEntities.size() == 1
                 && interface_.rawEntities.front().name == "WEIRDENT"
                 && interface_.rawEntities.front().handle == 0x7Au,
             "raw custom-class fixture preserves entity identity");
}

void testEed(TestContext& t) {
    FixtureInterface interface_;
    dx_data data;
    t.expect(importFixture("eed_binary_r2007.dxf", interface_, data),
             "EED fixture imports");
    const DRW_Point* point = nullptr;
    if (data.mBlock != nullptr) {
        for (const DRW_Entity* entity : data.mBlock->ent) {
            if (entity != nullptr && entity->eType == DRW::POINT)
                point = static_cast<const DRW_Point*>(entity);
        }
    }
    bool sawApplication = false;
    bool sawBinary = false;
    if (point != nullptr) {
        for (const auto& value : point->extData) {
            if (value == nullptr)
                continue;
            sawApplication = sawApplication || value->code() == 1001;
            sawBinary = sawBinary || value->code() == 1004;
        }
    }
    t.expect(point != nullptr && sawApplication && sawBinary,
             "EED fixture preserves application and binary items");
}

void testRawControls(TestContext& t) {
    FixtureInterface interface_;
    dx_data data;
    t.expect(importFixture("raw_control_groups_r2007.dxf", interface_, data),
             "raw control-group fixture imports");
    t.expect(interface_.dictionaries.size() == 2,
             "raw control-group fixture publishes dictionary records");
    t.expect(interface_.materials.size() == 2,
             "raw control-group fixture publishes material records");
    t.expect(interface_.xrecords.size() == 1,
             "raw control-group fixture publishes xrecord record");
}

void testDictionaryWithoutOwner(TestContext& t) {
    FixtureInterface interface_;
    dx_data data;
    const std::string contents = R"DXF(0
SECTION
2
HEADER
9
$ACADVER
1
AC1015
0
ENDSEC
0
SECTION
2
OBJECTS
0
DICTIONARY
5
C
100
AcDbDictionary
281
1
3
ACAD_MYAPP
350
1A
0
DICTIONARY
5
1A
100
AcDbDictionary
281
1
3
LC_ENTRY
350
1B
0
ACDBPLACEHOLDER
5
1B
330
1A
0
ENDSEC
0
EOF
)DXF";
    t.expect(importTemporaryDxf("dictionary-without-owner-r2000", contents,
                                interface_, data),
             "no-owner dictionary fixture imports");

    // Code 330 is optional on a DICTIONARY and R2000-era writers routinely omit
    // it. The root is told apart by its handle and its position, not by the
    // absence of an owner group -- so the named dictionary at 1A has to reach
    // the raw net, and the root at C must not, because the writer regenerates
    // that one and a second copy would be a second NamedObjectsDictionary.
    std::size_t rawDictionaries = 0;
    bool rootRouted = false;
    bool namedRouted = false;
    for (const DRW_RawDxfObject& object : interface_.rawObjects) {
        if (object.name != "DICTIONARY")
            continue;
        ++rawDictionaries;
        if (object.handle == 0xCu)
            rootRouted = true;
        if (object.handle == 0x1Au)
            namedRouted = true;
    }

    t.expect(interface_.dictionaries.size() == 2,
             "both dictionaries reach the typed callback");
    t.expect(namedRouted,
             "a named dictionary with no owner group is preserved");
    t.expect(!rootRouted,
             "the root dictionary is not preserved, the writer regenerates it");
    t.expect(rawDictionaries == 1,
             "exactly one dictionary is routed to the raw net");
}

void testMTextBackgroundFillAndDefinedHeight(TestContext& t) {
    FixtureInterface interface_;
    dx_data data;
    // An R2007 MTEXT with a defined column height and a filled background,
    // with the colour given three ways at once -- which is what the reference
    // asks for: 63 is required even when 421 or 431 carries the answer.
    const std::string contents = R"DXF(0
SECTION
2
HEADER
9
$ACADVER
1
AC1021
0
ENDSEC
0
SECTION
2
ENTITIES
0
MTEXT
5
30
8
0
10
1.0
20
2.0
30
0.0
40
2.5
41
100.0
46
12.5
71
1
72
5
1
filled
73
1
44
1.0
45
1.5
90
1
63
5
421
3368601
431
my colour
441
25
0
ENDSEC
0
EOF
)DXF";
    t.expect(importTemporaryDxf("mtext-background-fill-r2007", contents,
                                interface_, data),
             "MTEXT background fill fixture imports");

    const DRW_MText* mtext = findMText(data);
    t.expect(mtext != nullptr, "the MTEXT reaches the interface");
    if (mtext == nullptr)
        return;

    // Read. Code 46 had no case anywhere in the MTEXT -> TEXT -> LINE ->
    // POINT -> ENTITY chain and fell to the default.
    t.expect(mtext->m_definedHeight == 12.5,
             "code 46, the defined column height, is read");
    t.expect(mtext->m_backgroundFlags == 1, "code 90 is read");
    t.expect(mtext->m_backgroundScale == 1.5, "code 45 is read");
    // 63 and 421 shared one member, so whichever arrived last won.
    t.expect(mtext->m_backgroundColor == 5,
             "code 63 keeps the ACI index once 421 has its own field");
    t.expect(mtext->m_backgroundColorTrue == 3368601,
             "code 421 is read as a true colour of its own");
    t.expect(mtext->m_backgroundColorName == "my colour",
             "code 431 is read");
    t.expect(mtext->m_backgroundTransparency == 25, "code 441 is read");

    // Write. Every one of these was read and none of them was written, so a
    // DXF-to-DXF pass produced an MTEXT with no fill and no defined height.
    interface_.attach(&data);
    const std::filesystem::path output =
        std::filesystem::temp_directory_path() / "libdxfrw-mtext-background.dxf";
    std::error_code error;
    std::filesystem::remove(output, error);
    const bool written = interface_.fileExport(output.string(), DRW::AC1021,
                                               false, &data, false);
    t.expect(written, "the MTEXT is written back out");
    if (!written) {
        std::filesystem::remove(output, error);
        return;
    }

    const auto groups = groupsOfRecord(output, "MTEXT");
    t.expect(!groups.empty(), "the written file holds an MTEXT");
    t.expect(hasGroup(groups, 46), "code 46 is written back");
    t.expect(groupValue(groups, 46) == "12.5", "code 46 keeps its value");
    t.expect(hasGroup(groups, 45), "code 45 is written back");
    t.expect(hasGroup(groups, 90), "code 90 is written back");
    t.expect(hasGroup(groups, 63), "code 63 is written back");
    t.expect(groupValue(groups, 63) == "5", "code 63 keeps the ACI index");
    t.expect(hasGroup(groups, 421), "code 421 is written back");
    t.expect(groupValue(groups, 421) == "3368601", "code 421 keeps its value");
    t.expect(hasGroup(groups, 431), "code 431 is written back");
    t.expect(groupValue(groups, 431) == "my colour", "code 431 keeps its value");
    t.expect(hasGroup(groups, 441), "code 441 is written back");

    std::filesystem::remove(output, error);
}

void testMTextDefinedHeightPresence(TestContext& t) {
    // Authored here from scratch: an explicit zero is distinct from an absent
    // height, and a redundant R2018 context height is not the main group 46.
    const std::string prefix =
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1032\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n0\nMTEXT\n10\n11\n20\n22\n"
        "40\n2.5\n41\n35\n1\nheight control\n";
    for (const bool explicitZero : {false, true}) {
        FixtureInterface interface_;
        dx_data data;
        t.expect(importTemporaryDxf("mtext-defined-height-presence",
                     prefix + (explicitZero ? "46\n0\n" : "")
                         + "0\nENDSEC\n0\nEOF\n", interface_, data),
                 "MTEXT height presence control imports");
        const DRW_MText* parsed = findMText(data);
        if (parsed == nullptr) {
            t.expect(false, "MTEXT height presence control is published");
            continue;
        }
        auto* text = const_cast<DRW_MText*>(parsed);
        text->m_r2018RectHeight = 99.0;
        t.expect(text->m_definedHeightPresent == explicitZero,
                 "MTEXT records the source's defined-height presence");
        const DRW_MText copied(*text);
        t.expect(copied.m_definedHeightPresent == explicitZero,
                 "MTEXT copy retains defined-height presence");
        interface_.attach(&data);
        const auto output = std::filesystem::temp_directory_path()
            / "libdxfrw-mtext-height-presence-output.dxf";
        for (const bool binary : {false, true}) {
            t.expect(interface_.fileExport(output.string(), DRW::AC1032,
                                           binary, &data, false),
                     "MTEXT height presence control exports");
            if (!binary) {
                const auto groups = groupsOfRecord(output, "MTEXT");
                t.expect(hasGroup(groups, 46) == explicitZero,
                         "MTEXT distinguishes explicit zero height from absence");
                if (explicitZero && hasGroup(groups, 46))
                    t.expect(std::stod(groupValue(groups, 46)) == 0.0,
                             "MTEXT zero is not the redundant context height");
            }
            FixtureInterface readback;
            dx_data reread;
            t.expect(readback.fileImport(output.string(), &reread, false),
                     "MTEXT height presence output reopens");
            const DRW_MText* reopened = findMText(reread);
            t.expect(reopened != nullptr
                         && reopened->m_definedHeightPresent == explicitZero
                         && reopened->m_definedHeight == 0.0,
                     "ASCII/binary MTEXT preserves zero versus absent height");
        }
        std::error_code error;
        std::filesystem::remove(output, error);
    }
    class ResetProbe final : public DRW_MText {
    public:
        using DRW_MText::resetDwgState;
    };
    ResetProbe reset;
    reset.m_definedHeight = 9.0;
    reset.m_definedHeightPresent = true;
    reset.resetDwgState();
    t.expect(reset.m_definedHeight == 0.0 && !reset.m_definedHeightPresent,
             "MTEXT reset clears defined-height source state");
}

void testMTextWithoutBackgroundFillIsUnchanged(TestContext& t) {
    FixtureInterface interface_;
    dx_data data;
    // The other half of the promise: an MTEXT that asks for none of this must
    // not acquire any of it. The optional groups are written only when set.
    const std::string contents = R"DXF(0
SECTION
2
HEADER
9
$ACADVER
1
AC1021
0
ENDSEC
0
SECTION
2
ENTITIES
0
MTEXT
5
30
8
0
10
1.0
20
2.0
30
0.0
40
2.5
41
0.0
71
1
72
5
1
plain
73
1
44
1.0
0
ENDSEC
0
EOF
)DXF";
    t.expect(importTemporaryDxf("mtext-plain-r2007", contents, interface_, data),
             "plain MTEXT fixture imports");

    const DRW_MText* mtext = findMText(data);
    t.expect(mtext != nullptr, "the MTEXT reaches the interface");
    if (mtext == nullptr)
        return;

    interface_.attach(&data);
    const std::filesystem::path output =
        std::filesystem::temp_directory_path() / "libdxfrw-mtext-plain.dxf";
    std::error_code error;
    std::filesystem::remove(output, error);
    t.expect(interface_.fileExport(output.string(), DRW::AC1021, false, &data, false),
             "the plain MTEXT is written back out");

    const auto groups = groupsOfRecord(output, "MTEXT");
    t.expect(!groups.empty(), "the written file holds an MTEXT");
    t.expect(!hasGroup(groups, 46), "no defined height is invented");
    t.expect(!hasGroup(groups, 45), "no background scale is invented");
    t.expect(!hasGroup(groups, 90), "no background flags are invented");
    t.expect(!hasGroup(groups, 63), "no background colour is invented");
    t.expect(!hasGroup(groups, 421), "no true colour is invented");
    t.expect(!hasGroup(groups, 431), "no colour name is invented");
    t.expect(!hasGroup(groups, 441), "no transparency is invented");

    std::filesystem::remove(output, error);
}

void testMTextBackgroundModesWriteOnlyTheirOwnGroups(TestContext& t) {
    // Group 90 says which background mode is in use. 45 (the fill box scale)
    // and 63 (the fill colour) describe a COLOUR fill and belong only to the
    // mode that has one. Writing them whenever 90 is non-zero turns "absent"
    // into "present and zero": a text-frame-only MTEXT came out carrying a
    // zero box scale, where the convention is that the tag is absent and the
    // scale is 1.5.
    struct {
        const char* label;
        const char* groups;
        bool expectColourGroups;
    } cases[] = {
        {"colour fill (90 = 1)", "45\n1.5\n90\n1\n63\n5\n", true},
        {"text frame only (90 = 16)", "90\n16\n", false},
        {"drawing window colour (90 = 2)", "90\n2\n", false},
        {"frame and fill (90 = 17)", "45\n1.5\n90\n17\n63\n5\n", true},
    };

    for (const auto& item : cases) {
        FixtureInterface interface_;
        dx_data data;
        const std::string contents =
            std::string("0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n")
            + "0\nSECTION\n2\nENTITIES\n"
            + "0\nMTEXT\n5\n30\n8\n0\n10\n1.0\n20\n2.0\n30\n0.0\n"
            + "40\n2.5\n41\n0.0\n1\nfilled\n71\n1\n72\n5\n73\n1\n44\n1.0\n"
            + item.groups + "0\nENDSEC\n0\nEOF\n";
        t.expect(importTemporaryDxf("mtext-background-mode", contents, interface_, data),
                 "background mode fixture imports");
        if (findMText(data) == nullptr) {
            t.expect(false, "the MTEXT reaches the interface");
            continue;
        }

        interface_.attach(&data);
        const std::filesystem::path output =
            std::filesystem::temp_directory_path() / "libdxfrw-mtext-background-mode.dxf";
        std::error_code error;
        std::filesystem::remove(output, error);
        if (!interface_.fileExport(output.string(), DRW::AC1021, false, &data, false)) {
            t.expect(false, "the MTEXT is written back out");
            continue;
        }

        const auto groups = groupsOfRecord(output, "MTEXT");
        t.expect(hasGroup(groups, 90), "the background mode itself is always written");
        t.expect(hasGroup(groups, 45) == item.expectColourGroups,
                 item.expectColourGroups ? "a colour fill writes its box scale"
                                         : "a mode with no colour fill invents no box scale");
        t.expect(hasGroup(groups, 63) == item.expectColourGroups,
                 item.expectColourGroups ? "a colour fill writes its colour"
                                         : "a mode with no colour fill invents no colour");
        std::filesystem::remove(output, error);
    }
}

void testMTextBlackBackgroundColourSurvives(TestContext& t) {
    // 421 is a 24-bit RGB in which 0 is a legal value: pure black. A zero
    // sentinel for "absent" silently dropped it -- the same class of loss this
    // change exists to fix for 63. -1 is the sentinel, matching
    // DRW_Entity::color24.
    FixtureInterface interface_;
    dx_data data;
    const std::string contents = R"DXF(0
SECTION
2
HEADER
9
$ACADVER
1
AC1021
0
ENDSEC
0
SECTION
2
ENTITIES
0
MTEXT
5
30
8
0
10
1.0
20
2.0
30
0.0
40
2.5
41
0.0
1
black
71
1
72
5
73
1
44
1.0
45
1.5
90
1
63
5
421
0
0
ENDSEC
0
EOF
)DXF";
    t.expect(importTemporaryDxf("mtext-black-background", contents, interface_, data),
             "black background fixture imports");
    const DRW_MText* mtext = findMText(data);
    t.expect(mtext != nullptr, "the MTEXT reaches the interface");
    if (mtext == nullptr)
        return;
    t.expect(mtext->m_backgroundColorTrue == 0,
             "a true colour of 0 is read as black, not as absent");

    interface_.attach(&data);
    const std::filesystem::path output =
        std::filesystem::temp_directory_path() / "libdxfrw-mtext-black-background.dxf";
    std::error_code error;
    std::filesystem::remove(output, error);
    t.expect(interface_.fileExport(output.string(), DRW::AC1021, false, &data, false),
             "the MTEXT is written back out");

    const auto groups = groupsOfRecord(output, "MTEXT");
    t.expect(hasGroup(groups, 421), "a black background colour is written back");
    t.expect(groupValue(groups, 421) == "0", "and it is still black");

    std::filesystem::remove(output, error);
}

void testMTextRejectsAnUnsafeBackgroundColourName(TestContext& t) {
    // Group 431 is a string, and it reaches writeUtf8String, which refuses a
    // string carrying a control character. That refusal is a STICKY write
    // error: without screening the field in the write preflight, the record
    // was dropped from the output while fileExport still reported success --
    // a silently missing MTEXT in a file the caller was told was fine.
    FixtureInterface interface_;
    dx_data data;
    std::string contents =
        std::string("0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n0\nENDSEC\n")
        + "0\nSECTION\n2\nENTITIES\n"
        + "0\nMTEXT\n5\n30\n8\n0\n10\n1.0\n20\n2.0\n30\n0.0\n"
        + "40\n2.5\n41\n0.0\n1\nhi\n71\n1\n72\n5\n73\n1\n44\n1.0\n"
        + "45\n1.5\n90\n1\n63\n5\n431\nmy";
    contents += '\r';                 // an embedded CR, which readString keeps
    contents += "colour\n0\nENDSEC\n0\nEOF\n";

    t.expect(importTemporaryDxf("mtext-unsafe-colour-name", contents, interface_, data),
             "the fixture still imports; the value is only unwritable, not unreadable");
    const DRW_MText* mtext = findMText(data);
    t.expect(mtext != nullptr, "the MTEXT reaches the interface");
    if (mtext == nullptr)
        return;

    interface_.attach(&data);
    const std::filesystem::path output =
        std::filesystem::temp_directory_path() / "libdxfrw-mtext-unsafe-colour-name.dxf";
    std::error_code error;
    std::filesystem::remove(output, error);
    const bool written =
        interface_.fileExport(output.string(), DRW::AC1021, false, &data, false);

    t.expect(!written,
             "an unwritable colour name fails the export instead of deleting the record");

    std::filesystem::remove(output, error);
}


void testXRecordDataOrder(TestContext& t) {
    FixtureInterface interface_;
    dx_data data;
    const std::string contents = R"DXF(0
SECTION
2
HEADER
9
$ACADVER
1
AC1014
0
ENDSEC
0
SECTION
2
OBJECTS
0
XRECORD
5
210
330
200
100
AcDbXrecord
102
EXTNAMES
1
BYBLOCK
2
ByBlock
0
ENDSEC
0
EOF
)DXF";
    t.expect(importTemporaryDxf("xrecord-ordered-data-r14", contents,
                                interface_, data),
             "ordered XRECORD body fixture imports");
    t.expect(interface_.xrecords.size() == 1,
             "ordered XRECORD body reaches the typed callback");
    if (interface_.xrecords.size() == 1) {
        const DRW_XRecord& record = interface_.xrecords.front();
        t.expect(record.m_dataEntries.size() == 3,
                 "XRECORD callback retains its ordered body view");
        t.expect(record.m_dataEntries.size() == 3
                     && record.m_dataEntries[0].code() == 102
                     && record.m_dataEntries[1].code() == 1
                     && record.m_dataEntries[2].code() == 2,
                 "XRECORD body pair order is retained across group 102");
    }
}

std::string xRecordCoordinateSource(const std::vector<DRW_Variant>& payload,
                                    bool binary, int cloning = 2) {
    std::ostringstream bytes;
    if (binary)
        bytes.write("AutoCAD Binary DXF\r\n\x1A\0", 22);
    std::unique_ptr<dxfWriter> writer;
    if (binary)
        writer = std::make_unique<dxfWriterBinary>(&bytes);
    else
        writer = std::make_unique<dxfWriterAscii>(&bytes);
    for (const auto& field : std::vector<std::pair<int, std::string>>{
             {0, "SECTION"}, {2, "HEADER"}, {9, "$ACADVER"}, {1, "AC1015"},
             {0, "ENDSEC"}, {0, "SECTION"}, {2, "OBJECTS"}, {0, "XRECORD"},
             {5, "210"}, {330, "200"}, {100, "AcDbXrecord"}})
        writer->writeString(field.first, field.second);
    writer->writeInt16(280, cloning);
    for (const auto& tag : payload) {
        if (tag.type() == DRW_Variant::DOUBLE)
            writer->writeDouble(tag.code(), tag.d_val());
        else if (tag.type() == DRW_Variant::INTEGER)
            writer->writeInt16(tag.code(), tag.i_val());
        else
            writer->writeString(tag.code(), tag.c_str());
    }
    writer->writeString(0, "ENDSEC");
    writer->writeString(0, "EOF");
    return writer->hasWriteError() ? std::string{} : bytes.str();
}

bool xRecordPointsEqual(const std::vector<DRW_Variant>& values) {
    const std::array<DRW_Coord, 3> expected{{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}}};
    if (values.size() != 5 || values[2].code() != 1 || values[4].code() != 280
        || values[4].i_val() != 9)
        return false;
    const std::array<std::size_t, 3> indices{{0, 1, 3}};
    for (std::size_t index = 0; index < indices.size(); ++index) {
        const auto& value = values[indices[index]];
        const auto& point = expected[index];
        if (value.type() != DRW_Variant::COORD || value.code() != 10
            || value.coord()->x != point.x || value.coord()->y != point.y
            || value.coord()->z != point.z)
            return false;
    }
    return std::string(values[2].c_str()) == "boundary";
}

class XRecordCoordinateWriter final : public dx_iface {
public:
    DRW_XRecord value;
    bool wrote = false;
    void writeObjects() override { wrote = dxfW->writeXRecord(&value); }
};

class XRecordCoordinateProbe : public DRW_XRecord {
public:
    XRecordCoordinateProbe() = default;
    explicit XRecordCoordinateProbe(const DRW_XRecord& value) : DRW_XRecord(value) {}
    using DRW_XRecord::encodeDwg;
};

bool generateXRecordCoordinateDxf(const std::filesystem::path& path,
                                   bool sourceBinary, bool outputBinary) {
    const std::vector<DRW_Variant> payload{
        {10, 1.0}, {20, 2.0}, {30, 3.0}, {10, 4.0}, {20, 5.0}, {30, 6.0},
        {1, std::string("boundary")}, {10, 7.0}, {20, 8.0}, {30, 9.0}, {280, 9}};
    FixtureInterface interface_;
    dx_data imported;
    if (!importTemporaryDxf("xrecord-coordinate-generator",
            xRecordCoordinateSource(payload, sourceBinary), interface_, imported)
        || interface_.xrecords.size() != 1)
        return false;
    XRecordCoordinateWriter writer;
    writer.value = interface_.xrecords.front();
    writer.value.parentHandle = 0;
    dx_data output;
    return writer.fileExport(path.string(), DRW::AC1015, outputBinary, &output, false)
        && writer.wrote;
}

void testXRecordCoordinateTuples(TestContext& t) {
    const std::vector<DRW_Variant> payload{
        {10, 1.0}, {20, 2.0}, {30, 3.0}, {10, 4.0}, {20, 5.0}, {30, 6.0},
        {1, std::string("boundary")}, {10, 7.0}, {20, 8.0}, {30, 9.0}, {280, 9}};
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    for (const bool binary : {false, true}) {
        FixtureInterface interface_;
        dx_data data;
        t.expect(importTemporaryDxf(binary ? "xrecord-coordinate-binary" : "xrecord-coordinate-ascii",
                     xRecordCoordinateSource(payload, binary), interface_, data)
                     && interface_.xrecords.size() == 1,
                 "literal ASCII/binary repeated XRECORD coordinates import");
        if (interface_.xrecords.size() != 1)
            continue;
        const auto& record = interface_.xrecords.front();
        t.expect(record.hasExactDxfCoordinateTuples()
                     && record.m_cloning == 2 && xRecordPointsEqual(record.m_values)
                     && xRecordPointsEqual(record.m_dataEntries),
                 "both typed XRECORD views retain three distinct same-code tuples");
        bool rawComplete = false;
        for (const auto& raw : interface_.rawObjects) {
            if (raw.name != "XRECORD")
                continue;
            std::vector<double> coordinates;
            for (const auto& tag : raw.groups)
                if (tag.code() == 10 || tag.code() == 20 || tag.code() == 30)
                    coordinates.push_back(tag.d_val());
            rawComplete = coordinates == std::vector<double>{1, 2, 3, 4, 5, 6, 7, 8, 9};
        }
        t.expect(rawComplete, "raw companion retains all nine physical coordinate tags");
        DRW_XRecord copied(record);
        DRW_XRecord assigned;
        assigned = record;
        t.expect(xRecordPointsEqual(copied.m_values) && xRecordPointsEqual(copied.m_dataEntries)
                     && xRecordPointsEqual(assigned.m_values) && xRecordPointsEqual(assigned.m_dataEntries),
                 "XRECORD tuple identity survives both deep-copy routes");
        for (const bool outputBinary : {false, true}) {
            const auto path = std::filesystem::temp_directory_path()
                / ("libdxfrw-xrecord-coordinate-" + std::to_string(nonce)
                   + (binary ? "-binary" : "-ascii")
                   + (outputBinary ? "-to-binary.dxf" : "-to-ascii.dxf"));
            XRecordCoordinateWriter writer;
            writer.value = copied;
            writer.value.parentHandle = 0; // isolated API record, not a graph qualification
            dx_data output;
            const bool written = writer.fileExport(path.string(), DRW::AC1015,
                outputBinary, &output, false);
            FixtureInterface rereader;
            dx_data reread;
            t.expect(written && writer.wrote
                         && rereader.fileImport(path.string(), &reread, false)
                         && rereader.xrecords.size() == 1
                         && xRecordPointsEqual(rereader.xrecords.front().m_values)
                         && xRecordPointsEqual(rereader.xrecords.front().m_dataEntries),
                     "typed XRECORD emits all tuples in ASCII/binary without whole-CLI graph masking");
            std::error_code error;
            std::filesystem::remove(path, error);
        }
    }
}

void testXRecordCoordinateBoundaries(TestContext& t) {
    const std::vector<std::vector<DRW_Variant>> negatives{
        {{10, 1.0}}, // end-of-record partial
        {{10, 1.0}, {20, 2.0}, {1, std::string("interrupt")}, {30, 3.0}},
        {{10, 1.0}, {10, 4.0}, {20, 5.0}, {30, 6.0}},
        {{10, 1.0}, {20, 2.0}, {20, 8.0}, {30, 3.0}},
        {{10, 1.0}, {30, 3.0}, {20, 2.0}},
        {{20, 2.0}, {30, 3.0}, {10, 1.0}},
        {{10, 1.0}, {21, 2.0}, {31, 3.0}},
        {{20, 2.0}, {30, 3.0}}, // completed preceding tuple cannot be reused
        {{10, 1.0}, {20, 2.0}, {330, std::string("101")}, {30, 3.0}}};
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    unsigned number = 0;
    for (const bool binary : {false, true}) {
        for (const auto& negative : negatives) {
            std::vector<DRW_Variant> payload{{10, 91.0}, {20, 92.0}, {30, 93.0}};
            payload.insert(payload.end(), negative.begin(), negative.end());
            FixtureInterface interface_;
            dx_data data;
            t.expect(importTemporaryDxf("xrecord-coordinate-boundary",
                         xRecordCoordinateSource(payload, binary), interface_, data)
                         && interface_.xrecords.size() == 1,
                     "partial/repeated/unordered XRECORD remains tolerant-readable");
            if (interface_.xrecords.size() != 1)
                continue;
            const auto& record = interface_.xrecords.front();
            const auto stableFirst = [](const std::vector<DRW_Variant>& values) {
                return !values.empty() && values.front().type() == DRW_Variant::COORD
                    && values.front().coord()->x == 91 && values.front().coord()->y == 92
                    && values.front().coord()->z == 93;
            };
            t.expect(!record.hasExactDxfCoordinateTuples()
                         && stableFirst(record.m_values) && stableFirst(record.m_dataEntries),
                     "inexact tuples never mutate an earlier complete coordinate");
            DRW_XRecord copied(record);
            DRW_XRecord assigned;
            assigned = copied;
            DRW_XRecord moved(std::move(assigned));
            DRW_XRecord moveAssigned;
            moveAssigned = std::move(moved);
            t.expect(!copied.hasExactDxfCoordinateTuples()
                         && !moveAssigned.hasExactDxfCoordinateTuples(),
                     "copy/move retains inexact disposition without retaining parser cursors");
            bool rawComplete = false;
            for (const auto& raw : interface_.rawObjects) {
                if (raw.name != "XRECORD")
                    continue;
                std::vector<std::pair<int, double>> rawPoints, expected;
                for (const auto& tag : raw.groups) {
                    if (tag.type() == DRW_Variant::DOUBLE)
                        rawPoints.emplace_back(tag.code(), tag.d_val());
                }
                for (const auto& tag : payload) {
                    if (tag.type() == DRW_Variant::DOUBLE)
                        expected.emplace_back(tag.code(), tag.d_val());
                }
                rawComplete = rawPoints == expected;
            }
            t.expect(rawComplete, "raw companion retains inexact physical order and absent components");

            XRecordCoordinateProbe probe(copied);
            dwgBufferW body, strings, handles;
            t.expect(!probe.encodeDwg(DRW::AC1015, &body, &strings, &handles)
                         && body.data().empty() && strings.data().empty() && handles.data().empty(),
                     "typed DWG loss guard rejects partial coordinates before bytes are emitted");
            for (const bool outputBinary : {false, true}) {
                const auto path = std::filesystem::temp_directory_path()
                    / ("libdxfrw-xrecord-inexact-" + std::to_string(nonce)
                       + "-" + std::to_string(number++) + ".dxf");
                const std::string sentinel("original destination\n");
                { std::ofstream stream(path, std::ios::binary); stream << sentinel; }
                XRecordCoordinateWriter writer;
                writer.value = copied;
                writer.value.parentHandle = 0;
                dx_data output;
                const bool written = writer.fileExport(path.string(), DRW::AC1015,
                    outputBinary, &output, false);
                std::ifstream stream(path, std::ios::binary);
                const std::string actual((std::istreambuf_iterator<char>(stream)), {});
                t.expect(!written && !writer.wrote && actual == sentinel,
                         "typed DXF rejects inexact coordinates transactionally instead of inventing components");
                writer.value.reset();
                writer.value.handle = 0x210;
                writer.value.m_values.emplace_back(10, DRW_Coord(1, 2, 3));
                writer.value.m_dataEntries = writer.value.m_values;
                t.expect(writer.fileExport(path.string(), DRW::AC1015, outputBinary, &output, false)
                             && writer.wrote && writer.value.hasExactDxfCoordinateTuples(),
                         "failed-then-reset/from-API DXF write clears prior rejection state");
                std::error_code error;
                std::filesystem::remove(path, error);
            }
            // Explicit reconstruction is intentional, not a silent repair of
            // source presence/order. Clearing the public views alone retains
            // the original inexact disposition until reset().
            copied.m_values.clear();
            copied.m_dataEntries.clear();
            t.expect(!copied.hasExactDxfCoordinateTuples(),
                     "vector replacement alone cannot hide inexact source provenance");
            copied.reset();
            copied.handle = 0x210;
            copied.m_values.emplace_back(10, DRW_Coord(1, 2, 3));
            copied.m_dataEntries = copied.m_values;
            t.expect(copied.hasExactDxfCoordinateTuples(),
                     "reset and explicit from-API reconstruction clears prior disposition");
            XRecordCoordinateProbe restored(copied);
            dwgBufferW restoredBody;
            t.expect(restored.encodeDwg(DRW::AC1015, &restoredBody)
                         && !restoredBody.data().empty(),
                     "reset reconstruction restores typed write eligibility");
        }
        FixtureInterface reuse;
        dx_data reused;
        t.expect(!importTemporaryDxf("xrecord-coordinate-invalid-header",
                     xRecordCoordinateSource({{10, 1.0}}, binary, 6), reuse, reused)
                     && reuse.xrecords.empty(),
                 "invalid cloning header publishes no partial XRECORD");
        t.expect(importTemporaryDxf("xrecord-coordinate-valid-reuse",
                     xRecordCoordinateSource({{10, 1.0}, {20, 2.0}, {30, 3.0}, {280, 9}}, binary),
                     reuse, reused) && reuse.xrecords.size() == 1
                     && reuse.xrecords.front().hasExactDxfCoordinateTuples()
                     && reuse.xrecords.front().m_cloning == 2,
                 "failed-then-valid import does not retain tuple/header failure state");
    }
}

void testXRecordVersionedHeaderAndPayload(TestContext& t) {
    const std::array<std::pair<DRW::Version, const char*>, 4> versions = {{
        {DRW::AC1014, "AC1014"}, {DRW::AC1015, "AC1015"},
        {DRW::AC1021, "AC1021"}, {DRW::AC1032, "AC1032"}}};
    for (const auto& version : versions) {
        const bool hasCloningHeader = version.first >= DRW::AC1015;
        const std::string contents =
            std::string("0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\n")
            + version.second + "\n0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n"
            + "0\nXRECORD\n5\n210\n330\n200\n100\nAcDbXrecord\n"
            + (hasCloningHeader ? "280\n0\n" : "")
            + "280\n7\n1\nfirst\n280\n9\n330\n101\n1\nlast\n"
            + "0\nENDSEC\n0\nEOF\n";
        FixtureInterface capture;
        dx_data imported;
        t.expect(importTemporaryDxf("xrecord-header-payload", contents,
                                    capture, imported),
                 "versioned XRECORD header/payload fixture imports");
        if (capture.xrecords.size() != 1u) {
            t.expect(false, "versioned XRECORD reaches its typed callback");
            continue;
        }
        const DRW_XRecord& record = capture.xrecords.front();
        const auto hasExpectedPayload = [](const DRW_XRecord& value) {
            return value.m_cloning == 0 && value.m_values.size() == 4u
                && value.m_handleValues.size() == 1u
                && value.m_dataEntries.size() == 5u
                && value.m_dataEntries[0].code() == 280
                && value.m_dataEntries[0].i_val() == 7
                && value.m_dataEntries[1].code() == 1
                && std::string(value.m_dataEntries[1].c_str()) == "first"
                && value.m_dataEntries[2].code() == 280
                && value.m_dataEntries[2].i_val() == 9
                && value.m_dataEntries[3].code() == 330
                && value.m_dataEntries[4].code() == 1
                && std::string(value.m_dataEntries[4].c_str()) == "last";
        };
        t.expect(hasExpectedPayload(record),
                 "XRECORD keeps application 280 fields and ordered compatibility views");

        dx_data source;
        DRW_LType lineType;
        lineType.handle = 0x101u;
        lineType.name = "ByLayer";
        lineType.xDictHandle = 0x200u;
        source.lineTypes.push_back(lineType);
        DRW_Dictionary dictionary;
        dictionary.handle = 0x200u;
        dictionary.parentHandle = lineType.handle;
        dictionary.m_entries.push_back({"LOCAL_XRECORD", 0x210u});
        source.dictionaries.push_back(dictionary);
        source.xRecords.push_back(record);
        for (const bool binary : {false, true}) {
            const auto output = std::filesystem::temp_directory_path()
                / (std::string("libdxfrw-xrecord-versioned-") + version.second
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            std::error_code error;
            std::filesystem::remove(output, error);
            dx_iface exporter;
            const bool written = exporter.fileExport(
                output.string(), version.first, binary, &source, false);
            t.expect(written, "versioned XRECORD payload exports");
            if (written) {
                FixtureInterface reread;
                dx_data roundTrip;
                t.expect(reread.fileImport(output.string(), &roundTrip, false)
                             && reread.xrecords.size() == 1u
                             && hasExpectedPayload(reread.xrecords.front()),
                         "ASCII/binary XRECORD readback retains header and application order");
                if (!binary) {
                    const auto groups = groupsOfRecord(output, "XRECORD");
                    const auto marker = std::find(
                        groups.begin(), groups.end(),
                        std::make_pair(100, std::string("AcDbXrecord")));
                    const std::vector<std::pair<int, std::string>> expected =
                        hasCloningHeader
                            ? std::vector<std::pair<int, std::string>>{
                                  {280, "0"}, {280, "7"}, {1, "first"},
                                  {280, "9"}, {330, "15"}, {1, "last"}}
                            : std::vector<std::pair<int, std::string>>{
                                  {280, "7"}, {1, "first"}, {280, "9"},
                                  {330, "15"}, {1, "last"}};
                    t.expect(marker != groups.end()
                                 && std::vector<std::pair<int, std::string>>(
                                        marker + 1, groups.end()) == expected,
                             "literal XRECORD wire tags distinguish explicit zero header from payload");
                }
            }
            std::filesystem::remove(output, error);
        }
    }

    const std::string prefix =
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n0\nENDSEC\n"
        "0\nSECTION\n2\nOBJECTS\n0\nXRECORD\n5\n210\n330\n200\n"
        "100\nAcDbXrecord\n";
    const std::string suffix = "0\nENDSEC\n0\nEOF\n";
    FixtureInterface malformed;
    dx_data malformedData;
    t.expect(!importTemporaryDxf("xrecord-invalid-cloning",
                                 prefix + "280\n6\n" + suffix,
                                 malformed, malformedData)
                 && malformed.xrecords.empty(),
             "invalid XRECORD cloning header is rejected before publication");
    FixtureInterface omitted;
    dx_data omittedData;
    t.expect(importTemporaryDxf("xrecord-omitted-cloning",
                                prefix + "1\nlegacy\n280\n7\n" + suffix,
                                omitted, omittedData)
                 && omitted.xrecords.size() == 1u
                 && omitted.xrecords.front().m_cloning == 0
                 && omitted.xrecords.front().m_dataEntries.size() == 2u
                 && omitted.xrecords.front().m_dataEntries[1].code() == 280,
             "legacy omitted header does not consume a later application 280");
    FixtureInterface repeatedMarker;
    dx_data repeatedMarkerData;
    t.expect(importTemporaryDxf(
                 "xrecord-payload-subclass-marker",
                 prefix + "280\n0\n100\nAcDbXrecord\n280\n7\n" + suffix,
                 repeatedMarker, repeatedMarkerData)
                 && repeatedMarker.xrecords.size() == 1u
                 && repeatedMarker.xrecords.front().m_dataEntries.size() == 2u
                 && repeatedMarker.xrecords.front().m_dataEntries[0].code() == 100
                 && repeatedMarker.xrecords.front().m_dataEntries[1].code() == 280,
             "application subclass-marker spelling cannot reopen the XRECORD header");
    FixtureInterface unknownVersion;
    dx_data unknownVersionData;
    t.expect(importTemporaryDxf(
                 "xrecord-unknown-version",
                 "0\nSECTION\n2\nOBJECTS\n0\nXRECORD\n5\n210\n"
                 "100\nAcDbXrecord\n280\n2\n1\napplication\n"
                 "0\nENDSEC\n0\nEOF\n",
                 unknownVersion, unknownVersionData)
                 && unknownVersion.xrecords.size() == 1u
                 && unknownVersion.xrecords.front().m_cloning == 0
                 && unknownVersion.xrecords.front().m_dataEntries.size() == 2u
                 && unknownVersion.xrecords.front().m_dataEntries[0].code() == 280
                 && unknownVersion.xrecords.front().m_dataEntries[0].i_val() == 2,
             "unknown source version preserves ambiguous leading 280 as application data");
}

void testLinetypeExtensionGraphRoundTrip(
    TestContext& t, const std::filesystem::path* generatedOutput = nullptr,
    DRW::Version version = DRW::AC1014, bool binary = false) {
    dx_data source;
    DRW_AppId application;
    application.name = "LOCAL_HANDLE_APP";
    source.appIds.push_back(application);
    const std::array<std::string, 3> names = {
        "ByBlock", "ByLayer", "Continuous"};
    const std::array<std::uint32_t, 3> lineHandles = {
        0x101u, 0x102u, 0x103u};
    const std::array<std::uint32_t, 3> dictionaryHandles = {
        0x14u, 0x202u, 0x203u};
    const std::array<std::uint32_t, 3> recordHandles = {
        0x15u, 0x302u, 0x303u};
    for (std::size_t i = 0; i < names.size(); ++i) {
        DRW_LType lineType;
        lineType.handle = lineHandles[i];
        lineType.name = names[i];
        lineType.xDictHandle = dictionaryHandles[i];
        lineType.appData.push_back({{102, std::string("{SCOPE_TEST")},
                                   {1, names[i]}, {330, std::string("0")},
                                   {102, std::string("}")}});
        t.expect(lineType.addExtData(std::make_unique<DRW_Variant>(
                     1001, std::string("LOCAL_HANDLE_APP")))
                     && lineType.addExtData(std::make_unique<DRW_Variant>(
                         1000, std::string("trailing"))),
                 "canonical linetype retains registered trailing XDATA");
        source.lineTypes.push_back(lineType);

        DRW_Dictionary dictionary;
        dictionary.handle = dictionaryHandles[i];
        dictionary.parentHandle = lineHandles[i];
        dictionary.hardOwner = i == 0 ? 1 : 0;
        dictionary.cloning = 1;
        DRW_Dictionary::Entry entry;
        entry.m_name = "ACAD_XREC_ROUNDTRIP";
        entry.m_handle = recordHandles[i];
        dictionary.m_entries.push_back(entry);
        if (i == 0) {
            t.expect(dictionary.addExtData(std::make_unique<DRW_Variant>(
                         1001, std::string("LOCAL_HANDLE_APP"))),
                     "extension dictionary begins registered XDATA application");
            auto xdataReference = std::make_unique<DRW_Variant>(
                1005, std::string("15"));
            t.expect(dictionary.addExtData(std::move(xdataReference)),
                     "extension dictionary accepts a typed XDATA handle");
        }
        source.dictionaries.push_back(dictionary);

        DRW_XRecord record;
        record.handle = recordHandles[i];
        record.parentHandle = dictionaryHandles[i];
        record.reactorHandles.push_back(dictionaryHandles[i]);
        record.m_dataEntries.emplace_back(102, std::string("EXTNAMES"));
        record.m_dataEntries.emplace_back(1, names[i]);
        record.m_dataEntries.emplace_back(2, names[i]);
        record.m_dataEntries.emplace_back(320, std::string("DEAD"));
        record.m_dataEntries.emplace_back(329, std::string("14"));
        source.xRecords.push_back(record);
    }
    auto line = std::make_unique<DRW_Line>();
    line->basePoint = DRW_Coord(1, 2, 3);
    line->secPoint = DRW_Coord(4, 6, 8);
    line->appData.push_back({{102, std::string("{SCOPE_TEST")},
                             {1, std::string("alpha")},
                             {330, std::string("0")},
                             {102, std::string("}")}});
    line->extData.push_back(std::make_shared<DRW_Variant>(
        1001, std::string("LOCAL_HANDLE_APP")));
    line->extData.push_back(std::make_shared<DRW_Variant>(
        1000, std::string("trailing")));
    source.mBlock->ent.push_back(line.release());

    const auto nonce = std::chrono::steady_clock::now().time_since_epoch()
                           .count();
    const std::filesystem::path output = generatedOutput != nullptr
        ? *generatedOutput : std::filesystem::temp_directory_path()
            / ("libdxfrw-ltype-extension-" + std::to_string(nonce) + ".dxf");
    struct OutputCleanup {
        std::filesystem::path path;
        bool remove;
        ~OutputCleanup() {
            std::error_code error;
            if (remove)
                std::filesystem::remove(path, error);
        }
    } cleanup{output, generatedOutput == nullptr};

    dx_iface writerInterface;
    t.expect(writerInterface.fileExport(output.string(), version, binary,
                                        &source, false),
             "writer exports the canonical linetype extension graph");
    if (generatedOutput != nullptr || !std::filesystem::exists(output))
        return;

    FixtureInterface readerInterface;
    dx_data roundTrip;
    t.expect(readerInterface.fileImport(output.string(), &roundTrip, false),
             "R14 extension graph output reads through the public DXF path");
    t.expect(roundTrip.sourceVersion == DRW::AC1014,
             "R14 extension graph output keeps AC1014 version");
    for (std::size_t i = 0; i < names.size(); ++i) {
        const DRW_LType* lineType = nullptr;
        for (const DRW_LType& candidate : roundTrip.lineTypes) {
            if (candidate.name == names[i])
                lineType = &candidate;
        }
        t.expect(lineType != nullptr,
                 "canonical mandatory linetype survives R14 readback");
        if (lineType == nullptr)
            continue;
        const std::uint32_t outputDictionary = lineType->xDictHandle;
        t.expect(outputDictionary != 0
                     && (i == 0 ? outputDictionary != dictionaryHandles[i]
                                : outputDictionary == dictionaryHandles[i]),
                 "extension dictionary link applies only necessary remapping");

        const DRW_Dictionary* dictionary = nullptr;
        // FixtureInterface intercepts typed object callbacks for focused
        // inspection instead of forwarding them into dx_data.
        for (const DRW_Dictionary& candidate : readerInterface.dictionaries) {
            if (candidate.handle == outputDictionary)
                dictionary = &candidate;
        }
        t.expect(dictionary != nullptr,
                 "linetype extension dictionary object is emitted");
        if (dictionary == nullptr)
            continue;
        t.expect(dictionary->parentHandle == lineType->handle
                     && dictionary->m_entries.size() == 1
                     && dictionary->hardOwner == (i == 0 ? 1 : 0),
                 "extension dictionary owner, membership, and hard-owner flag survive");

        const DRW_XRecord* record = nullptr;
        for (const DRW_XRecord& candidate : readerInterface.xrecords) {
            if (!dictionary->m_entries.empty()
                && candidate.handle == dictionary->m_entries.front().m_handle)
                record = &candidate;
        }
        t.expect(record != nullptr,
                 "dictionary entry resolves to an emitted XRECORD");
        if (record == nullptr)
            continue;
        t.expect(record->parentHandle == dictionary->handle
                     && record->reactorHandles.size() == 1
                     && record->reactorHandles.front() == dictionary->handle,
                 "XRECORD owner and reactor references survive");
        t.expect(record->m_dataEntries.size() == 5
                     && record->m_dataEntries[0].code() == 102
                     && record->m_dataEntries[0].c_str() != nullptr
                     && std::string(record->m_dataEntries[0].c_str()) == "EXTNAMES"
                     && record->m_dataEntries[1].c_str() != nullptr
                     && std::string(record->m_dataEntries[1].c_str()) == names[i]
                     && record->m_dataEntries[2].c_str() != nullptr
                     && std::string(record->m_dataEntries[2].c_str()) == names[i],
                 "ordered XRECORD extension names survive R14 readback");
        if (record->m_dataEntries.size() == 5) {
            t.expect(record->m_dataEntries[3].code() == 320
                         && record->m_dataEntries[3].i64_val() == 0xDEAD
                         && record->m_dataEntries[4].code() == 329
                         && record->m_dataEntries[4].i64_val() == 0x14,
                     "arbitrary XRECORD handles need no closure target and ignore collisions");
        }
        if (i == 0) {
            std::uint32_t xdataHandle = 0;
            bool xdataHandleMatchesRecord = false;
            if (dictionary->extData.size() == 2
                && dictionary->extData[0] != nullptr
                && dictionary->extData[0]->code() == 1001
                && dictionary->extData[1] != nullptr
                && dictionary->extData[1]->c_str() != nullptr) {
                const char* value = dictionary->extData[1]->c_str();
                const char* end = value
                    + std::char_traits<char>::length(value);
                const auto parsed = std::from_chars(value, end, xdataHandle, 16);
                xdataHandleMatchesRecord = parsed.ec == std::errc{}
                    && parsed.ptr == end && xdataHandle == record->handle;
            }
            t.expect(xdataHandleMatchesRecord,
                     "XDATA handle follows the remapped XRECORD handle");
        }
    }
}

void testCanonicalLinetypeCommonScope(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    for (const DRW::Version version : {DRW::AC1014, DRW::AC1015,
                                       DRW::AC1021, DRW::AC1032}) {
        for (const bool binary : {false, true}) {
            const auto path = std::filesystem::temp_directory_path()
                / ("libdxfrw-common-scope-" + std::to_string(nonce) + "-"
                   + std::to_string(static_cast<int>(version))
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            struct Cleanup {
                std::filesystem::path path;
                ~Cleanup() {
                    std::error_code error;
                    std::filesystem::remove(path, error);
                }
            } cleanup{path};
            testLinetypeExtensionGraphRoundTrip(t, &path, version, binary);
            std::vector<TableOwnerRecord> records;
            t.expect(readTableOwnerRecords(path, binary, records),
                     "canonical common-scope raw framing reaches EOF");
            int selected = 0;
            for (const auto& record : records) {
                if (record.kind != "LTYPE" && record.kind != "LINE")
                    continue;
                ++selected;
                const auto firstSubclass = std::find_if(
                    record.groups.begin(), record.groups.end(),
                    [](const auto& tag) { return tag.first == 100; });
                const auto app = std::find(record.groups.begin(), record.groups.end(),
                    std::make_pair(102, std::string("{SCOPE_TEST")));
                t.expect(app != record.groups.end() && app < firstSubclass,
                         "canonical linetype/LINE application data is in base scope");
                const auto xdata = std::find(record.groups.begin(), record.groups.end(),
                    std::make_pair(1001, std::string("LOCAL_HANDLE_APP")));
                t.expect(xdata != record.groups.end() && xdata > firstSubclass
                             && record.groups.end() - xdata == 2
                             && (xdata + 1)->first == 1000
                             && (xdata + 1)->second == "trailing",
                         "canonical linetype/LINE XDATA remains trailing");
                if (record.kind == "LTYPE") {
                    const auto xdict = std::find(record.groups.begin(), record.groups.end(),
                        std::make_pair(102, std::string("{ACAD_XDICTIONARY")));
                    t.expect(xdict != record.groups.end() && xdict < firstSubclass,
                             "mandatory linetype extension dictionary precedes subclasses");
                }
            }
            t.expect(selected == 4,
                     "common-scope graph covers all three mandatory linetypes and LINE");
        }
    }
}

bool generateTypedObjectCommonGraphs(const std::filesystem::path& output,
                                     DRW::Version version, bool binary) {
    class CommonObjectInterface final : public dx_iface {
    public:
        bool emitted{true};
        void writeHeader(DRW_Header& header) override {
            dx_iface::writeHeader(header);
            dxfW->setRootDictEntries({{"COMMON_SCALE", "800"},
                                      {"COMMON_VARIABLE", "810"}});
            // Explicit caller-provided translations, not automatic source
            // identity planning or qualification of the converter's closure.
            dxfW->setHandleRemap({{0x1110u, 0x800u}, {0x2110u, 0x802u},
                                  {0x1111u, 0x810u}, {0x2111u, 0x812u}});
        }
        void writeObjects() override {
            for (int branch = 0; branch < 2; ++branch) {
                const std::uint32_t root = branch == 0 ? 0x800u : 0x810u;
                DRW_Dictionary dictionary;
                dictionary.handle = root;
                dictionary.parentHandle = 0xCu;
                dictionary.hardOwner = 1;
                dictionary.m_entries.push_back({"LEAF", root + 1u});
                emitted = dxfW->writeDictionary(&dictionary) && emitted;

                const auto common = [branch, root](auto& leaf) {
                    leaf.handle = root + 1u;
                    leaf.parentHandle = root;
                    leaf.name = "COMMON_LEAF";
                    leaf.reactorHandles = {0x1110u + static_cast<std::uint32_t>(branch)};
                    leaf.xDictHandle = 0x2110u + static_cast<std::uint32_t>(branch);
                    leaf.appData.push_back({{102, std::string("{OBJECT_META")},
                        {1, std::string("alpha")}, {70, 7},
                        {330, branch == 0 ? std::string("1110") : std::string("1111")},
                        {320, std::string("DEAD")}, {102, std::string("}")}});
                    if (branch == 1) {
                        leaf.appData.push_back({{102, std::string("ACAD_REACTORS")},
                            {330, std::string("1111")}, {102, std::string("}")}});
                        leaf.appData.push_back({{102, std::string("ACAD_XDICTIONARY")},
                            {360, std::string("2111")}, {102, std::string("}")}});
                    }
                    return leaf.addExtData(std::make_unique<DRW_Variant>(
                               1001, std::string("OBJECT_META_APP")))
                        && leaf.addExtData(std::make_unique<DRW_Variant>(
                               1000, std::string("trailing")));
                };
                if (branch == 0) {
                    DRW_Scale leaf;
                    leaf.paperUnits = 2.0;
                    leaf.drawingUnits = 5.0;
                    emitted = common(leaf) && dxfW->writeScale(&leaf) && emitted;
                } else {
                    DRW_DictionaryVar leaf;
                    leaf.m_value = "COMMON_VALUE";
                    emitted = common(leaf) && dxfW->writeDictionaryVar(&leaf) && emitted;
                }
                DRW_Dictionary extension;
                extension.handle = root + 2u;
                extension.parentHandle = root + 1u;
                extension.hardOwner = 1;
                extension.m_entries.push_back({"SENTINEL", root + 3u});
                emitted = dxfW->writeDictionary(&extension) && emitted;
                DRW_XRecord record;
                record.handle = root + 3u;
                record.parentHandle = root + 2u;
                record.m_cloning = 1;
                record.m_dataEntries = {{102, std::string("EXTNAMES")},
                    {1, std::string("ordered-body")}, {320, std::string("DEAD")}};
                emitted = dxfW->writeXRecord(&record) && emitted;
            }
        }
    } interface_;
    dx_data source;
    DRW_AppId application;
    application.name = "OBJECT_META_APP";
    source.appIds.push_back(application);
    for (const char* kind : {"SCALE", "DICTIONARYVAR"}) {
        DRW_Class definition;
        if (!dxfRW::dxfClassForRecordName(kind, definition))
            return false;
        definition.instanceCount = 1;
        source.dxfClasses.push_back(definition);
    }
    return interface_.fileExport(output.string(), version, binary, &source, false)
        && interface_.emitted;
}

void testTypedObjectCommonGraphs(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    for (const DRW::Version version : {DRW::AC1021, DRW::AC1032}) {
        for (const bool binary : {false, true}) {
            const auto path = std::filesystem::temp_directory_path()
                / ("libdxfrw-object-common-" + std::to_string(nonce) + "-"
                   + std::to_string(static_cast<int>(version))
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            struct Cleanup {
                std::filesystem::path path;
                ~Cleanup() {
                    std::error_code error;
                    std::filesystem::remove(path, error);
                }
            } cleanup{path};
            t.expect(generateTypedObjectCommonGraphs(path, version, binary),
                     "two explicitly mapped common-object graphs export");
            FixtureInterface reader;
            dx_data readback;
            t.expect(reader.fileImport(path.string(), &readback, false),
                     "closed common-object graphs read through public callbacks");
            t.expect(reader.scales.size() == 1 && reader.dictionaryVars.size() == 1,
                     "both graph leaves reach their typed callbacks exactly once");
            const auto checkLeaf = [&t, &reader](const auto& leaf, std::uint32_t root) {
                t.expect(leaf.handle == root + 1u && leaf.parentHandle == root
                             && leaf.reactorHandles == std::vector<std::uint32_t>{root}
                             && leaf.xDictHandle == root + 2u && leaf.appData.size() == 3,
                         "common metadata carries translated existing identities once");
                t.expect(leaf.extData.size() == 2 && leaf.extData[0]->code() == 1001
                             && leaf.extData[1]->code() == 1000
                             && std::string(leaf.extData[1]->c_str()) == "trailing",
                         "closed graph leaf preserves trailing registered XDATA");
                const auto extension = std::find_if(reader.dictionaries.begin(),
                    reader.dictionaries.end(), [root](const auto& d) { return d.handle == root + 2u; });
                t.expect(extension != reader.dictionaries.end()
                             && extension->parentHandle == leaf.handle
                             && extension->m_entries.size() == 1
                             && extension->m_entries.front().m_handle == root + 3u,
                         "leaf extension resolves to its owned dictionary and child");
                const auto record = std::find_if(reader.xrecords.begin(), reader.xrecords.end(),
                    [root](const auto& r) { return r.handle == root + 3u; });
                t.expect(record != reader.xrecords.end() && record->parentHandle == root + 2u
                             && record->m_dataEntries.size() == 3
                             && record->m_dataEntries[0].code() == 102
                             && record->m_dataEntries[1].code() == 1
                             && record->m_dataEntries[2].code() == 320,
                         "closed graph preserves XRECORD body ordering and arbitrary handle");
            };
            if (reader.scales.size() == 1)
                checkLeaf(reader.scales.front(), 0x800u);
            if (reader.dictionaryVars.size() == 1)
                checkLeaf(reader.dictionaryVars.front(), 0x810u);
        }
    }
}

bool generateGeneratedObjectCommonCarriers(const std::filesystem::path& output,
                                           DRW::Version version, bool binary) {
    class GeneratedCommonInterface final : public dx_iface {
    public:
        bool emitted{true};
        void writeHeader(DRW_Header& header) override {
            dx_iface::writeHeader(header);
            dxfW->setRootDictEntries({{"COMMON_NAMED", "800"}});
            // The caller explicitly supplies these translations. GROUP's
            // generated identity is not assumed by the extension owner graph.
            dxfW->setHandleRemap({{0x1110u, 0x800u}, {0x1111u, 0xDu},
                                  {0x2110u, 0x802u}});
            const auto common = [](auto& entry, const std::string& reactor) {
                entry.reactorHandles = {static_cast<std::uint32_t>(
                    std::stoul(reactor, nullptr, 16))};
                entry.appData.push_back({{102, std::string("{GENERATED_META")},
                    {1, std::string("alpha")}, {70, 7},
                    {330, reactor}, {320, std::string("DEAD")},
                    {102, std::string("}")}});
                entry.appData.push_back({{102, std::string("ACAD_REACTORS")},
                    {330, reactor}, {102, std::string("}")}});
                return entry.addExtData(std::make_unique<DRW_Variant>(
                           1001, std::string("GENERATED_META_APP")))
                    && entry.addExtData(std::make_unique<DRW_Variant>(
                           1000, std::string("trailing")));
            };
            DRW_Dictionary dictionary;
            dictionary.handle = 0x800u;
            dictionary.parentHandle = 0xCu;
            dictionary.cloning = 2;
            dictionary.m_entries.push_back({"CHILD", 0x804u});
            dictionary.xDictHandle = 0x2110u;
            emitted = common(dictionary, "1111") && emitted;
            dictionary.appData.push_back({{102, std::string("ACAD_XDICTIONARY")},
                {360, std::string("2110")}, {102, std::string("}")}});
            dxfW->setNamedDictObjects({dictionary});
            DRW_Group group;
            group.handle = 0x600u;
            group.name = "GENERATED_GROUP";
            group.m_description = "retained generated group";
            // Carrier-only control: the generated GROUP identity is allocated
            // after ENTITIES. Reciprocal member reactors need the separate
            // source-identity planning contract, not a guessed future handle.
            emitted = common(group, "1110") && emitted;
            dxfW->setGroups({group});
        }
        void writeEntities() override {
            DRW_Line line;
            line.handle = 0x900u;
            line.basePoint = DRW_Coord(1.0, 2.0, 3.0);
            line.secPoint = DRW_Coord(4.0, 6.0, 8.0);
            emitted = dxfW->writeLine(&line) && emitted;
        }
        void writeObjects() override {
            DRW_Dictionary extension;
            extension.handle = 0x802u;
            extension.parentHandle = 0x800u;
            extension.hardOwner = 1;
            extension.m_entries.push_back({"SENTINEL", 0x803u});
            emitted = dxfW->writeDictionary(&extension) && emitted;
            DRW_XRecord record;
            record.handle = 0x803u;
            record.parentHandle = 0x802u;
            record.m_dataEntries = {{102, std::string("EXTNAMES")},
                {1, std::string("ordered-body")}, {320, std::string("DEAD")}};
            emitted = dxfW->writeXRecord(&record) && emitted;
            record.handle = 0x804u;
            record.parentHandle = 0x800u;
            record.m_dataEntries = {{1, std::string("named-child")}};
            emitted = dxfW->writeXRecord(&record) && emitted;
        }
    } interface_;
    dx_data source;
    DRW_AppId application;
    application.name = "GENERATED_META_APP";
    source.appIds.push_back(application);
    return interface_.fileExport(output.string(), version, binary, &source, false)
        && interface_.emitted;
}

void testGeneratedObjectCommonCarriers(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    for (const DRW::Version version : {DRW::AC1021, DRW::AC1032}) {
        for (const bool binary : {false, true}) {
            const auto path = std::filesystem::temp_directory_path()
                / ("libdxfrw-generated-common-" + std::to_string(nonce) + "-"
                   + std::to_string(static_cast<int>(version))
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            struct Cleanup {
                std::filesystem::path path;
                ~Cleanup() {
                    std::error_code error;
                    std::filesystem::remove(path, error);
                }
            } cleanup{path};
            t.expect(generateGeneratedObjectCommonCarriers(path, version, binary),
                     "generated named-dictionary and GROUP carriers export");
            FixtureInterface reader;
            dx_data readback;
            t.expect(reader.fileImport(path.string(), &readback, false),
                     "generated common carriers import through public callbacks");
            const auto named = std::find_if(reader.dictionaries.begin(),
                reader.dictionaries.end(), [](const auto& d) { return d.handle == 0x800u; });
            const auto extension = std::find_if(reader.dictionaries.begin(),
                reader.dictionaries.end(), [](const auto& d) { return d.handle == 0x802u; });
            t.expect(named != reader.dictionaries.end() && named->parentHandle == 0xCu
                         && named->cloning == 2 && named->m_entries.size() == 1
                         && named->m_entries.front().m_handle == 0x804u,
                     "generated named dictionary keeps ownership cloning and child");
            t.expect(extension != reader.dictionaries.end()
                         && extension->parentHandle == 0x800u
                         && extension->m_entries.size() == 1
                         && extension->m_entries.front().m_handle == 0x803u,
                     "named dictionary extension resolves to owned sentinel");
            const auto checkCommon = [&t](const auto& entry, std::uint32_t reactor,
                                          std::uint32_t xdict, std::size_t groupCount) {
                t.expect(entry.reactorHandles == std::vector<std::uint32_t>{reactor}
                             && entry.xDictHandle == xdict && entry.appData.size() == groupCount,
                         "generated route emits translated common carriers exactly once");
                t.expect(entry.extData.size() == 2 && entry.extData[0]->code() == 1001
                             && std::string(entry.extData[0]->c_str()) == "GENERATED_META_APP"
                             && entry.extData[1]->code() == 1000
                             && std::string(entry.extData[1]->c_str()) == "trailing",
                         "generated route preserves trailing registered XDATA");
            };
            if (named != reader.dictionaries.end())
                checkCommon(*named, 0xDu, 0x802u, 3);
            t.expect(reader.groups.size() == 1,
                     "generated GROUP reaches typed callback exactly once");
            if (reader.groups.size() == 1) {
                const auto& group = reader.groups.front();
                checkCommon(group, 0x800u, 0u, 2);
                t.expect(group.parentHandle == 0xDu && group.m_entityHandles.empty()
                             && readback.mBlock != nullptr && readback.mBlock->ent.size() == 1
                             && readback.mBlock->ent.front()->eType == DRW::LINE,
                         "carrier-only GROUP is empty and leaves unrelated LINE intact");
            }
            const auto sentinel = std::find_if(reader.xrecords.begin(), reader.xrecords.end(),
                [](const auto& r) { return r.handle == 0x803u; });
            const auto child = std::find_if(reader.xrecords.begin(), reader.xrecords.end(),
                [](const auto& r) { return r.handle == 0x804u; });
            t.expect(sentinel != reader.xrecords.end() && sentinel->parentHandle == 0x802u
                         && sentinel->m_dataEntries.size() == 3
                         && sentinel->m_dataEntries[0].code() == 102
                         && sentinel->m_dataEntries[1].code() == 1
                         && sentinel->m_dataEntries[2].code() == 320,
                     "generated common graph leaves XRECORD body102 ordering untouched");
            t.expect(child != reader.xrecords.end() && child->parentHandle == 0x800u
                         && child->m_dataEntries.size() == 1
                         && std::string(child->m_dataEntries.front().c_str()) == "named-child",
                     "generated named dictionary body child resolves");
        }
    }
}

void testIncompleteLinetypeExtensionGraphRejected(TestContext& t) {
    dx_data source;
    DRW_LType lineType;
    lineType.handle = 0x101u;
    lineType.name = "ByLayer";
    lineType.xDictHandle = 0x201u;
    source.lineTypes.push_back(lineType);
    DRW_Dictionary dictionary;
    dictionary.handle = 0x201u;
    dictionary.parentHandle = lineType.handle;
    DRW_Dictionary::Entry entry;
    entry.m_name = "MISSING";
    entry.m_handle = 0x202u;
    dictionary.m_entries.push_back(entry);
    source.dictionaries.push_back(dictionary);

    const auto nonce = std::chrono::steady_clock::now().time_since_epoch()
                           .count();
    const std::filesystem::path output =
        std::filesystem::temp_directory_path()
        / ("libdxfrw-incomplete-extension-" + std::to_string(nonce) + ".dxf");
    std::error_code error;
    std::filesystem::remove(output, error);
    dx_iface writerInterface;
    t.expect(!writerInterface.fileExport(output.string(), DRW::AC1014, false,
                                         &source, false),
             "incomplete linetype extension graph is rejected");
    t.expect(!std::filesystem::exists(output),
             "rejected extension graph publishes no partial DXF");
    std::filesystem::remove(output, error);
}

void expectExtensionGraphRejected(TestContext& t, dx_data& source,
                                  const char* caseName,
                                  const char* rejectedLabel,
                                  const char* noOutputLabel) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch()
                           .count();
    const std::filesystem::path output =
        std::filesystem::temp_directory_path()
        / (std::string("libdxfrw-invalid-extension-") + caseName + "-"
           + std::to_string(nonce) + ".dxf");
    struct OutputCleanup {
        std::filesystem::path path;
        ~OutputCleanup() {
            std::error_code error;
            std::filesystem::remove(path, error);
        }
    } cleanup{output};

    dx_iface writerInterface;
    t.expect(!writerInterface.fileExport(output.string(), DRW::AC1014, false,
                                         &source, false),
             rejectedLabel);
    t.expect(!std::filesystem::exists(output),
             noOutputLabel);
}

void testUnsafeLinetypeExtensionGraphsRejected(TestContext& t) {
    dx_data duplicateHandle;
    DRW_LType lineType;
    lineType.handle = 0x101u;
    lineType.name = "ByLayer";
    lineType.xDictHandle = 0x201u;
    duplicateHandle.lineTypes.push_back(lineType);
    DRW_Dictionary firstDictionary;
    firstDictionary.handle = 0x201u;
    firstDictionary.parentHandle = lineType.handle;
    DRW_Dictionary::Entry entry;
    entry.m_name = "CHILD";
    entry.m_handle = 0x301u;
    firstDictionary.m_entries.push_back(entry);
    duplicateHandle.dictionaries.push_back(firstDictionary);
    DRW_Dictionary duplicateDictionary;
    duplicateDictionary.handle = firstDictionary.handle;
    duplicateDictionary.parentHandle = lineType.handle;
    duplicateHandle.dictionaries.push_back(duplicateDictionary);
    DRW_XRecord child;
    child.handle = entry.m_handle;
    child.parentHandle = firstDictionary.handle;
    duplicateHandle.xRecords.push_back(child);
    expectExtensionGraphRejected(
        t, duplicateHandle, "duplicate-handle",
        "duplicate source handles are rejected",
        "duplicate source handles publish no partial DXF");

    dx_data danglingReference;
    lineType.handle = 0x102u;
    lineType.xDictHandle = 0x202u;
    danglingReference.lineTypes.push_back(lineType);
    DRW_Dictionary dictionary;
    dictionary.handle = lineType.xDictHandle;
    dictionary.parentHandle = lineType.handle;
    entry.m_handle = 0x302u;
    dictionary.m_entries.push_back(entry);
    danglingReference.dictionaries.push_back(dictionary);
    DRW_XRecord danglingRecord;
    danglingRecord.handle = entry.m_handle;
    danglingRecord.parentHandle = dictionary.handle;
    danglingRecord.m_dataEntries.emplace_back(330, std::string("DEAD"));
    danglingReference.xRecords.push_back(danglingRecord);
    expectExtensionGraphRejected(t, danglingReference,
                                 "unresolved-object-reference",
                                 "unresolved object references are rejected",
                                 "unresolved references publish no partial DXF");

    dx_data danglingFallbackReference;
    lineType.handle = 0x104u;
    lineType.xDictHandle = 0x204u;
    danglingFallbackReference.lineTypes.push_back(lineType);
    dictionary.handle = lineType.xDictHandle;
    dictionary.parentHandle = lineType.handle;
    entry.m_handle = 0x304u;
    dictionary.m_entries.clear();
    dictionary.m_entries.push_back(entry);
    danglingFallbackReference.dictionaries.push_back(dictionary);
    DRW_XRecord fallbackRecord;
    fallbackRecord.handle = entry.m_handle;
    fallbackRecord.parentHandle = dictionary.handle;
    fallbackRecord.m_handleValues.emplace_back(330, 0xDEADu);
    danglingFallbackReference.xRecords.push_back(fallbackRecord);
    expectExtensionGraphRejected(
        t, danglingFallbackReference, "dangling-fallback-reference",
        "unresolved fallback XRECORD handles are rejected",
        "unresolved fallback handles publish no partial DXF");

    dx_data nullChild;
    lineType.handle = 0x105u;
    lineType.xDictHandle = 0x205u;
    nullChild.lineTypes.push_back(lineType);
    DRW_Dictionary nullChildDictionary;
    nullChildDictionary.handle = lineType.xDictHandle;
    nullChildDictionary.parentHandle = lineType.handle;
    entry.m_handle = DRW::NoHandle;
    nullChildDictionary.m_entries.push_back(entry);
    nullChild.dictionaries.push_back(nullChildDictionary);
    expectExtensionGraphRejected(
        t, nullChild, "null-dictionary-child",
        "null dictionary children are rejected",
        "null children publish no partial DXF");
}
void testClassicVertexXdata(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto output = std::filesystem::temp_directory_path()
        / ("libdxfrw-vertex-xdata-" + std::to_string(nonce) + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{output};
    const auto xdata = [](DRW_Entity& entity, const std::string& marker) {
        entity.extData.push_back(std::make_shared<DRW_Variant>(1001, "VERTEX_APP"));
        entity.extData.push_back(std::make_shared<DRW_Variant>(1000, marker));
        entity.extData.push_back(std::make_shared<DRW_Variant>(1070, -7));
    };
    const auto populate = [&xdata](dx_data& data, int flags, bool modern) {
        DRW_AppId app;
        app.name = "VERTEX_APP";
        data.appIds.push_back(app);
        auto* poly = new DRW_Polyline;
        poly->flags = flags;
        poly->handle = 0x900u;
        poly->layer = "0";
        xdata(*poly, "parent-xdata");
        const int count = flags == 64 ? 4 : 2;
        for (int index = 0; index < count; ++index) {
            DRW_Vertex vertex;
            vertex.handle = static_cast<std::uint32_t>(0x901u + index);
            vertex.basePoint = DRW_Coord(index + 1.25, index - 2.5, flags == 0 ? 0.0 : index + 3.75);
            vertex.flags = flags == 8 ? 32 : (flags == 64 ? (index == 0 ? 128 : 192) : 0);
            if (flags == 64 && index == 0) {
                vertex.vindex1 = -1;
                vertex.vindex2 = 2;
                vertex.vindex3 = 3;
                vertex.basePoint = DRW_Coord(0, 0, 0);
            }
            const std::string marker = "vertex-xdata-" + std::to_string(index);
            xdata(vertex, marker);
            if (modern) {
                vertex.appData.push_back({{102, std::string("{VERTEX_META")},
                    {1, marker}, {320, std::string("DEADBEEF")}, {102, std::string("}")}});
            }
            poly->addVertex(vertex);
        }
        if (flags == 64) {
            poly->vertexcount = 3;
            poly->facecount = 1;
        }
        data.mBlock->ent.push_back(poly);
        auto* next = new DRW_Line;
        next->basePoint = DRW_Coord(100, 200, 300);
        next->secPoint = DRW_Coord(101, 202, 303);
        xdata(*next, "next-record-xdata");
        data.mBlock->ent.push_back(next);
        return poly;
    };
    const auto marker = [](const DRW_Entity& entity) {
        return entity.extData.size() == 3u && entity.extData[0]
            && entity.extData[0]->code() == 1001
            && std::string(entity.extData[0]->c_str()) == "VERTEX_APP"
            && entity.extData[1] && entity.extData[1]->code() == 1000
            && entity.extData[2] && entity.extData[2]->code() == 1070
            && entity.extData[2]->i_val() == -7
                ? std::string(entity.extData[1]->c_str()) : std::string{};
    };
    for (const DRW::Version version : {DRW::AC1009, DRW::AC1021}) {
        for (const bool binary : {false, true}) {
            for (const int flags : {0, 8, 64}) {
                dx_data source;
                auto* poly = populate(source, flags, version > DRW::AC1009);
                const auto vertices = poly->vertlist;
                const DRW_Polyline before(*poly);
                dx_iface writer;
                t.expect(writer.fileExport(output.string(), version, binary, &source, false),
                         "classic VERTEX XDATA matrix exports");
                t.expect(poly->vertlist == vertices && marker(*poly) == "parent-xdata",
                         "VERTEX emission leaves caller vector and parent metadata intact");
                for (std::size_t index = 0; index < vertices.size(); ++index) {
                    t.expect(vertices[index]->handle != 0
                                 && vertices[index]->handle != before.vertlist[index]->handle,
                             "successful VERTEX emission retains the existing minted-handle assignment");
                    t.expect(marker(*vertices[index]) == "vertex-xdata-" + std::to_string(index)
                                 && vertices[index]->basePoint.x == before.vertlist[index]->basePoint.x
                                 && vertices[index]->vindex1 == before.vertlist[index]->vindex1,
                             "VERTEX caller geometry, signed visibility and metadata are not reordered");
                }
                dx_data decoded;
                dx_iface reader;
                t.expect(reader.fileImport(output.string(), &decoded, false),
                         "classic VERTEX ordered XDATA reads back");
                t.expect(decoded.mBlock->ent.size() == 2u,
                         "VERTEX parent and following entity remain separate");
                if (decoded.mBlock->ent.size() != 2u) continue;
                const auto* decodedPoly = dynamic_cast<const DRW_Polyline*>(decoded.mBlock->ent.front());
                t.expect(decodedPoly && marker(*decodedPoly) == "parent-xdata"
                             && marker(*decoded.mBlock->ent.back()) == "next-record-xdata",
                         "parent and next-record XDATA do not absorb child data");
                if (!decodedPoly) continue;
                t.expect(decodedPoly->vertlist.size() == vertices.size(),
                         "VERTEX XDATA does not change child count");
                for (std::size_t wireIndex = 0; wireIndex < decodedPoly->vertlist.size(); ++wireIndex) {
                    const auto& vertex = decodedPoly->vertlist[wireIndex];
                    // Interleaved caller-side polyface order is a robustness
                    // control; canonical output keeps coordinates before faces.
                    const std::size_t sourceIndex = flags == 64
                        ? (wireIndex + 1u) % vertices.size() : wireIndex;
                    t.expect(marker(*vertex) == "vertex-xdata-" + std::to_string(sourceIndex),
                             "each classic VERTEX retains distinct ordered XDATA");
                    if (version > DRW::AC1009) {
                        t.expect(vertex->parentHandle == decodedPoly->handle && vertex->appData.size() == 1u,
                                 "VERTEX common application group and authoritative parent owner survive");
                    }
                    if (flags == 64 && sourceIndex == 0)
                        t.expect(vertex->flags == 128 && vertex->vindex1 == -1
                                     && vertex->vindex2 == 2 && vertex->vindex3 == 3,
                                 "polyface XDATA preserves signed invisible edges and face role");
                }
                if (version > DRW::AC1009) {
                    std::vector<TableOwnerRecord> raw;
                    t.expect(readTableOwnerRecords(output, binary, raw),
                             "VERTEX XDATA raw framing reaches the next record and EOF");
                    for (const TableOwnerRecord& record : raw) {
                        if (record.kind != "VERTEX") continue;
                        const auto xdataStart = std::find_if(record.groups.begin(), record.groups.end(),
                            [](const auto& tag) { return tag.first == 1001; });
                        t.expect(xdataStart != record.groups.end()
                                     && std::none_of(xdataStart, record.groups.end(), [](const auto& tag) {
                                         return tag.first == 100 || tag.first == 102 || tag.first == 91;
                                     }), "VERTEX XDATA follows subtype and common application groups");
                    }
                }
            }
            dx_data bad;
            auto* poly = populate(bad, 8, version > DRW::AC1009);
            poly->vertlist.back()->extData.back() = std::make_shared<DRW_Variant>(1070, 70001);
            const auto vertices = poly->vertlist;
            const auto originalParent = poly->handle;
            const std::array<std::uint32_t, 2> originalHandles{{vertices[0]->handle, vertices[1]->handle}};
            { std::ofstream original(output); original << "existing vertex destination\n"; }
            dx_iface rejecting;
            rejecting.cData = &bad;
            rejecting.currentBlock = bad.mBlock;
            const std::string fileName = output.string();
            dxfRW codec(fileName.c_str());
            rejecting.dxfW = &codec;
            t.expect(!codec.write(&rejecting, version, binary),
                     "invalid child XDATA write failure propagates to the parent transaction");
            std::ifstream bytes(output, std::ios::binary);
            t.expect(std::string(std::istreambuf_iterator<char>(bytes), {}) == "existing vertex destination\n",
                     "rejected child XDATA preserves the existing destination");
            t.expect(poly->handle == originalParent && poly->vertlist == vertices
                         && vertices[0]->handle == originalHandles[0] && vertices[1]->handle == originalHandles[1]
                         && vertices[1]->extData.back()->i_val() == 70001,
                     "failed parent transaction restores child identities without editing caller metadata");
            t.expect(codec.remapEntityHandle(originalParent) == originalParent
                         && codec.remapEntityHandle(originalHandles[0]) == originalHandles[0]
                         && codec.remapEntityHandle(originalHandles[1]) == originalHandles[1],
                     "failed child XDATA rolls back source-to-emitted handle bindings");
            bytes.close();
            vertices[1]->extData.back() = std::make_shared<DRW_Variant>(1070, -7);
            t.expect(codec.write(&rejecting, version, binary),
                     "same codec succeeds after rejected child XDATA is corrected");
            dx_data reused;
            dx_iface reusedReader;
            t.expect(reusedReader.fileImport(fileName, &reused, false),
                     "failed-then-valid compound output reads back");
            const auto* reusedPoly = reused.mBlock->ent.empty() ? nullptr
                : dynamic_cast<const DRW_Polyline*>(reused.mBlock->ent.front());
            t.expect(reusedPoly && reusedPoly->vertlist.size() == 2u
                         && marker(*reusedPoly->vertlist[0]) == "vertex-xdata-0"
                         && marker(*reusedPoly->vertlist[1]) == "vertex-xdata-1",
                     "failed-then-valid reuse retains both child XDATA scopes");
            rejecting.dxfW = nullptr;
        }
    }
}

void testInsertChildHandleRollback(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto output = std::filesystem::temp_directory_path()
        / ("libdxfrw-insert-rollback-" + std::to_string(nonce) + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{output};
    const auto addXdata = [](DRW_Entity& entity, const std::string& text) {
        entity.extData.push_back(std::make_shared<DRW_Variant>(1001, "INSERT_APP"));
        entity.extData.push_back(std::make_shared<DRW_Variant>(1000, text));
        entity.extData.push_back(std::make_shared<DRW_Variant>(1070, -7));
    };
    const auto marker = [](const DRW_Entity& entity) {
        return entity.extData.size() == 3u && entity.extData[0]
            && entity.extData[0]->code() == 1001
            && std::string(entity.extData[0]->c_str()) == "INSERT_APP"
            && entity.extData[1] && entity.extData[1]->code() == 1000
                ? std::string(entity.extData[1]->c_str()) : std::string{};
    };
    for (const DRW::Version version : {DRW::AC1015, DRW::AC1032}) {
        for (const bool binary : {false, true}) {
            for (const bool alias : {false, true}) {
                for (const bool badXdata : {false, true}) {
                    dx_data data;
                    DRW_AppId app;
                    app.name = "INSERT_APP";
                    data.appIds.push_back(app);
                    auto* block = new dx_ifaceBlock;
                    block->name = "LOCAL_INSERT_ROLLBACK";
                    data.blocks.push_back(block);
                    auto* insert = new DRW_Insert;
                    insert->name = block->name;
                    insert->handle = 0xA100u;
                    insert->basePoint = DRW_Coord(12, 23, 34);
                    insert->xscale = -2;
                    insert->angle = 0.5;
                    insert->color = 3;
                    insert->appData.push_back({{102, std::string("{INSERT_META")},
                        {1, std::string("parent-app")}, {102, std::string("}")}});
                    addXdata(*insert, "parent-xdata");
                    auto first = std::make_shared<DRW_Attrib>();
                    first->handle = 0xA101u;
                    first->tag = "FIRST";
                    first->text = "first attribute";
                    first->height = 2;
                    first->basePoint = DRW_Coord(45, 56, 67);
                    first->color = 4;
                    first->lockPosition = true;
                    first->appData.push_back({{102, std::string("{ATTRIB_META")},
                        {1, std::string("first-app")}, {102, std::string("}")}});
                    addXdata(*first, "first-xdata");
                    auto second = std::make_shared<DRW_Attrib>(*first);
                    second->handle = 0xA102u;
                    second->tag = "SECOND";
                    second->text = "second attribute";
                    second->basePoint = DRW_Coord(78, 89, 90);
                    second->extData.clear();
                    addXdata(*second, "second-xdata");
                    if (badXdata)
                        second->extData.back() = std::make_shared<DRW_Variant>(1070, 70001);
                    else
                        second->appData.front().pop_back(); // late unclosed child group
                    insert->attlist = {first, second};
                    if (alias)
                        insert->attlist.insert(insert->attlist.begin() + 1, first);
                    data.mBlock->ent.push_back(insert);
                    auto* next = new DRW_Line;
                    next->handle = 0xA103u;
                    next->secPoint = DRW_Coord(1, 2, 3);
                    addXdata(*next, "next-xdata");
                    data.mBlock->ent.push_back(next);
                    const auto children = insert->attlist;
                    const auto firstXdata = first->extData;
                    const auto secondXdata = second->extData;
                    const auto secondGroups = second->appData.size();
                    const auto secondGroupTags = second->appData.front().size();
                    { std::ofstream original(output); original << "existing insert destination\n"; }
                    dx_iface adapter;
                    adapter.cData = &data;
                    adapter.currentBlock = data.mBlock;
                    const std::string fileName = output.string();
                    dxfRW codec(fileName.c_str());
                    adapter.dxfW = &codec;
                    t.expect(!codec.write(&adapter, version, binary),
                             "late child appData/XDATA failure rejects the enclosing INSERT");
                    std::ifstream bytes(output, std::ios::binary);
                    t.expect(std::string(std::istreambuf_iterator<char>(bytes), {})
                                 == "existing insert destination\n",
                             "rejected INSERT preserves the destination");
                    bytes.close();
                    t.expect(insert->handle == 0xA100u && first->handle == 0xA101u
                                 && second->handle == 0xA102u && next->handle == 0xA103u,
                             "INSERT rollback restores all caller child identities, including aliases");
                    t.expect(insert->attlist == children && first->extData == firstXdata
                                 && second->extData == secondXdata
                                 && second->appData.size() == secondGroups
                                 && second->appData.front().size() == secondGroupTags
                                 && insert->name == block->name && insert->color == 3
                                 && insert->basePoint.x == 12 && insert->xscale == -2
                                 && insert->angle == 0.5 && insert->appData.size() == 1u
                                 && marker(*insert) == "parent-xdata"
                                 && first->tag == "FIRST" && first->text == "first attribute"
                                 && first->basePoint.z == 67 && first->color == 4
                                 && first->lockPosition && first->appData.size() == 1u
                                 && second->tag == "SECOND" && second->basePoint.z == 90
                                 && marker(*first) == "first-xdata" && marker(*second) == "second-xdata"
                                 && second->extData.back()->i_val() == (badXdata ? 70001 : -7),
                             "failed INSERT leaves caller vectors, geometry and metadata unchanged");
                    t.expect(codec.remapEntityHandle(0xA100u) == 0xA100u
                                 && codec.remapEntityHandle(0xA101u) == 0xA101u
                                 && codec.remapEntityHandle(0xA102u) == 0xA102u
                                 && codec.remapEntityHandle(0xA103u) == 0xA103u,
                             "failed INSERT restores parent and child source-handle journal");
                    if (badXdata)
                        second->extData.back() = std::make_shared<DRW_Variant>(1070, -7);
                    else
                        second->appData.front().push_back(DRW_Variant(102, std::string("}")));
                    t.expect(codec.write(&adapter, version, binary),
                             "same INSERT writer succeeds after correcting only the late child error");
                    t.expect(insert->attlist == children && insert->handle != 0xA100u
                                 && first->handle != 0xA101u && second->handle != 0xA102u
                                 && marker(*first) == "first-xdata" && marker(*second) == "second-xdata",
                             "successful INSERT retains minted-child assignment and caller metadata");
                    adapter.dxfW = nullptr;
                    dx_data decoded;
                    dx_iface reader;
                    t.expect(reader.fileImport(fileName, &decoded, false),
                             "failed-then-valid INSERT output reads back");
                    const auto* readInsert = decoded.mBlock->ent.empty() ? nullptr
                        : dynamic_cast<const DRW_Insert*>(decoded.mBlock->ent.front());
                    t.expect(decoded.mBlock->ent.size() == 2u && readInsert
                                 && readInsert->attlist.size() == children.size()
                                 && marker(*readInsert) == "parent-xdata"
                                 && marker(*decoded.mBlock->ent.back()) == "next-xdata",
                             "INSERT retry retains parent, child count and following record boundaries");
                    if (!readInsert) continue;
                    for (std::size_t index = 0; index < readInsert->attlist.size(); ++index) {
                        const auto& child = readInsert->attlist[index];
                        const bool last = index + 1u == readInsert->attlist.size();
                        t.expect(child && child->parentHandle == readInsert->handle
                                     && child->tag == (last ? "SECOND" : "FIRST")
                                     && marker(*child) == (last ? "second-xdata" : "first-xdata")
                                     && child->extData.back()->i_val() == -7
                                     && child->appData.size() == 1u,
                                 "INSERT retry preserves ordered child owners and distinct metadata");
                    }
                    // Repeated pointers are rollback robustness controls,
                    // not a valid-source graph qualification denominator.
                }
            }
        }
    }
}

void populateLegacyEllipseCarriers(dx_data& data, bool includeControls = true) {
    DRW_AppId app;
    app.name = "ELLIPSE_APP";
    data.appIds.push_back(app);
    DRW_Layer layer;
    layer.name = "ELLIPSE_META";
    data.layers.push_back(layer);
    const auto metadata = [](DRW_Entity& entity, const std::string& label,
                             const char* reference) {
        entity.layer = "ELLIPSE_META";
        entity.lineType = "CONTINUOUS";
        entity.extData.push_back(std::make_shared<DRW_Variant>(1001, "ELLIPSE_APP"));
        entity.extData.push_back(std::make_shared<DRW_Variant>(1000, label));
        entity.extData.push_back(std::make_shared<DRW_Variant>(1011, DRW_Coord(7, 8, 9)));
        entity.extData.push_back(std::make_shared<DRW_Variant>(1070, -7));
        entity.extData.push_back(std::make_shared<DRW_Variant>(1000, "ordered-tail"));
        entity.extData.push_back(std::make_shared<DRW_Variant>(1005, reference));
    };
    for (const bool paper : {false, true}) {
        auto* ellipse = new DRW_Ellipse;
        ellipse->handle = paper ? 0xA300u : 0xA200u;
        ellipse->space = paper ? DRW::PaperSpace : DRW::ModelSpace;
        ellipse->basePoint = paper ? DRW_Coord(110, 120, 130) : DRW_Coord(10, 20, 30);
        ellipse->secPoint = DRW_Coord(4, 0, 0);
        ellipse->extPoint = DRW_Coord(0, 0.6, 0.8);
        ellipse->ratio = 0.5;
        ellipse->staparam = paper ? 0.25 : 0;
        ellipse->endparam = paper ? 1.75 : 2 * std::acos(-1.0);
        ellipse->color = paper ? 4 : 3;
        metadata(*ellipse, paper ? "ellipse-paper" : "ellipse-model", paper ? "A300" : "A200");
        data.mBlock->ent.push_back(ellipse);
        if (includeControls) {
            auto* control = new DRW_Polyline;
            ellipse->toPolyline(control, 128);
            control->space = ellipse->space;
            control->handle = paper ? 0xA301u : 0xA201u;
            metadata(*control, paper ? "control-paper" : "control-model", paper ? "A301" : "A201");
            data.mBlock->ent.push_back(control);
        }
    }
    auto* next = new DRW_Line;
    next->handle = 0xA400u;
    next->secPoint = DRW_Coord(1, 2, 3);
    metadata(*next, "next-record", "A200");
    data.mBlock->ent.push_back(next);
}

bool verifyLegacyEllipseCarriers(const std::filesystem::path& path, bool includeControls = true) {
    dx_data decoded;
    dx_iface adapter;
    adapter.cData = &decoded;
    adapter.currentBlock = decoded.mBlock;
    const std::string fileName = path.string();
    dxfRW codec(fileName.c_str());
    const std::vector<const char*> labels = includeControls
        ? std::vector<const char*>{"ellipse-model", "control-model", "ellipse-paper", "control-paper", "next-record"}
        : std::vector<const char*>{"ellipse-model", "ellipse-paper", "next-record"};
    if (!codec.read(&adapter, false) || codec.getVersion() != DRW::AC1009
        || decoded.mBlock->ent.size() != labels.size())
        return false;
    std::uint32_t modelEllipseHandle = 0;
    auto current = decoded.mBlock->ent.cbegin();
    for (std::size_t index = 0; index < labels.size(); ++index) {
        const DRW_Entity* entity = *current++;
        if (!entity || entity->layer != "ELLIPSE_META" || entity->lineType != "CONTINUOUS"
            || entity->extData.size() != 6u)
            return false;
        const auto& xdata = entity->extData;
        const std::array<int, 6> codes{{1001, 1000, 1011, 1070, 1000, 1005}};
        for (std::size_t field = 0; field < codes.size(); ++field) {
            if (!xdata[field] || xdata[field]->code() != codes[field]) return false;
        }
        if (std::string(xdata[0]->c_str()) != "ELLIPSE_APP"
            || std::string(xdata[1]->c_str()) != labels[index]
            || xdata[2]->type() != DRW_Variant::COORD
            || xdata[2]->content.v->x != 7 || xdata[2]->content.v->y != 8
            || xdata[2]->content.v->z != 9 || xdata[3]->i_val() != -7
            || std::string(xdata[4]->c_str()) != "ordered-tail")
            return false;
        std::uint32_t reference = 0;
        const std::string handle = xdata[5]->c_str();
        const auto parsed = std::from_chars(handle.data(), handle.data() + handle.size(), reference, 16);
        if (parsed.ec != std::errc{} || parsed.ptr != handle.data() + handle.size()) return false;
        if (index == 0) modelEllipseHandle = entity->handle;
        const bool next = index + 1u == labels.size();
        if (reference != (next ? modelEllipseHandle : entity->handle)) return false;
        if (next) {
            if (entity->eType != DRW::LINE) return false;
            continue;
        }
        const bool paper = std::string(labels[index]).find("-paper") != std::string::npos;
        const auto* poly = dynamic_cast<const DRW_Polyline*>(entity);
        if (!poly || poly->space != (paper ? DRW::PaperSpace : DRW::ModelSpace)
            || poly->color != (paper ? 4 : 3)
            || poly->flags != (paper ? 8 : 9)
            || poly->vertlist.size() != (paper ? 32u : 128u)
            || poly->vertlist.front()->basePoint.z == 0)
            return false;
    }
    // Read literal R12 framing separately, without document re-parenting.
    std::ifstream stream(path, std::ios::binary);
    char prefix[22]{};
    stream.read(prefix, sizeof(prefix));
    const bool binary = std::string(prefix, sizeof(prefix)) == std::string("AutoCAD Binary DXF\r\n\032\0", 22);
    stream.clear();
    stream.seekg(binary ? 22 : 0);
    std::unique_ptr<dxfReader> tags;
    if (binary) tags = std::make_unique<dxfReaderBinaryR12>(&stream);
    else tags = std::make_unique<dxfReaderAscii>(&stream);
    bool poly = false;
    std::size_t parent = 0, paperFlags = 0, xdataParents = 0;
    int code = 0;
    while (tags->readRec(&code)) {
        if (code == 0) {
            const std::string kind = tags->getString();
            if (kind == "EOF") return parent == labels.size() - 1u
                && paperFlags == (includeControls ? 2u : 1u) && xdataParents == parent;
            if (kind == "ELLIPSE") return false;
            poly = kind == "POLYLINE";
            if (poly) ++parent;
        } else if (poly && code == 67) {
            if (tags->getInt32() != 1 || parent < (includeControls ? 3u : 2u)) return false;
            ++paperFlags;
        } else if (poly && code == 1001) {
            if (tags->getString() != "ELLIPSE_APP") return false;
            ++xdataParents;
        }
    }
    return false;
}

bool generateLegacyEllipseCarriers(const std::filesystem::path& path, bool binary) {
    dx_data source;
    populateLegacyEllipseCarriers(source);
    dx_iface adapter;
    return adapter.fileExport(path.string(), DRW::AC1018, binary, &source, false);
}

void testLegacyEllipseCarrierTransfer(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto output = std::filesystem::temp_directory_path()
        / ("libdxfrw-r12-ellipse-carriers-" + std::to_string(nonce) + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{output};
    for (const bool binary : {false, true}) {
        dx_data source;
        populateLegacyEllipseCarriers(source);
        auto* ellipse = static_cast<DRW_Ellipse*>(source.mBlock->ent.front());
        const DRW_Ellipse before(*ellipse);
        dx_iface adapter;
        adapter.cData = &source;
        adapter.currentBlock = source.mBlock;
        const std::string filename = output.string();
        dxfRW codec(filename.c_str());
        adapter.dxfW = &codec;
        t.expect(codec.write(&adapter, DRW::AC1009, binary), "R12 derived ellipse carrier export succeeds");
        t.expect(verifyLegacyEllipseCarriers(output), "R12 ellipse and ordinary POLYLINE retain space, ordered XDATA and identities");
        t.expect(ellipse->handle == before.handle && ellipse->eType == DRW::ELLIPSE
                     && ellipse->basePoint.z == before.basePoint.z && ellipse->secPoint.x == before.secPoint.x
                     && ellipse->extPoint.y == before.extPoint.y && ellipse->ratio == before.ratio
                     && ellipse->extData.size() == before.extData.size()
                     && std::string(ellipse->extData.back()->c_str()) == "A200",
                 "successful legacy derivation keeps the original ellipse caller state");
        t.expect(codec.remapEntityHandle(0xA200u) != 0xA200u,
                 "R12 derived parent captures the original ellipse source identity");
        adapter.dxfW = nullptr;

        for (int unsupported = 0; unsupported < 15; ++unsupported) {
            dx_data bad;
            // Keep rollback/reuse scoped to derived parents. Unrelated ordinary
            // POLYLINE caller-state rollback is a separate file-level issue.
            populateLegacyEllipseCarriers(bad, false);
            auto* invalid = static_cast<DRW_Ellipse*>(bad.mBlock->ent.front());
            const DRW_Ellipse clean(*invalid);
            switch (unsupported) {
            case 0: invalid->appData.push_back({{102, std::string("{META")}, {102, std::string("}")}}); break;
            case 1: invalid->reactorHandles = {0xDEADu}; break;
            case 2: invalid->xDictHandle = 0xBEEFu; break;
            case 3: invalid->numProxyGraph = 5; invalid->proxyGraphics = "stale"; break;
            case 4: invalid->material = 0x123u; break;
            case 5: invalid->plotStyle = 0x124; break;
            case 6: invalid->fullVisualStyleHandle = 0x125u; break;
            case 7: invalid->faceVisualStyleHandle = 0x126u; break;
            case 8: invalid->edgeVisualStyleHandle = 0x127u; break;
            case 9: invalid->shadowHandle = 0x128u; break;
            case 10: invalid->hasDataStorageRecord = true; invalid->dataStorageData = {1, 2}; break;
            case 11: invalid->setHasDataStorageBinaryData(true); break;
            case 12: invalid->proxyGraphics = "stale"; break;
            case 13: invalid->setDwgAcDbColorHandle(true, 0x129u); break;
            case 14: invalid->dataStorageHandleKey = "orphan"; invalid->dataStorageData = {3, 4}; break;
            }
            { std::ofstream previous(output); previous << "ellipse destination\n"; }
            dx_iface rejecting;
            rejecting.cData = &bad;
            rejecting.currentBlock = bad.mBlock;
            dxfRW retry(filename.c_str());
            rejecting.dxfW = &retry;
            t.expect(!retry.write(&rejecting, DRW::AC1009, binary), "R12 ellipse rejects unsupported carriers/resources explicitly");
            t.expect(retry.getLastDiagnostic().code == "ellipse-r12-carrier",
                     "unsupported ellipse carrier has a bounded observable diagnostic");
            std::ifstream bytes(output, std::ios::binary);
            t.expect(std::string(std::istreambuf_iterator<char>(bytes), {}) == "ellipse destination\n"
                         && invalid->handle == clean.handle && invalid->ratio == clean.ratio
                         && retry.remapEntityHandle(clean.handle) == clean.handle,
                     "rejected R12 derivation preserves destination, caller identity and source map");
            bytes.close();
            *invalid = clean;
            t.expect(retry.write(&rejecting, DRW::AC1009, binary) && verifyLegacyEllipseCarriers(output, false),
                     "same writer succeeds after removing only the unsupported carrier");
            rejecting.dxfW = nullptr;
        }
        dx_data bad;
        populateLegacyEllipseCarriers(bad, false);
        auto* invalid = static_cast<DRW_Ellipse*>(bad.mBlock->ent.front());
        invalid->extData[3] = std::make_shared<DRW_Variant>(1070, 70001);
        { std::ofstream previous(output); previous << "ellipse metadata destination\n"; }
        dx_iface rejecting;
        rejecting.cData = &bad;
        rejecting.currentBlock = bad.mBlock;
        dxfRW retry(filename.c_str());
        rejecting.dxfW = &retry;
        t.expect(!retry.write(&rejecting, DRW::AC1009, binary), "late derived ellipse XDATA error rejects atomically");
        std::ifstream bytes(output, std::ios::binary);
        t.expect(std::string(std::istreambuf_iterator<char>(bytes), {}) == "ellipse metadata destination\n"
                     && invalid->handle == 0xA200u && invalid->extData[3]->i_val() == 70001
                     && retry.remapEntityHandle(0xA200u) == 0xA200u,
                 "derived parent metadata failure rolls back identity and preserves caller/destination");
        bytes.close();
        invalid->extData[3] = std::make_shared<DRW_Variant>(1070, -7);
        t.expect(retry.write(&rejecting, DRW::AC1009, binary) && verifyLegacyEllipseCarriers(output, false),
                 "R12 derivation supports failed-then-valid metadata reuse");
        rejecting.dxfW = nullptr;
    }
}

// RAY/XLINE directions are WCS vectors, not second points. Their DXF fields
// must be unit vectors even when callers supply finite non-unit magnitudes.
void testInfiniteLineDirections(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto output = std::filesystem::temp_directory_path()
        / ("libdxfrw-directions-" + std::to_string(nonce) + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{output};
    const std::string filename = output.string();
    const auto contents = [&output]() {
        std::ifstream stream(output, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(stream), {});
    };
    class DirectionInterface final : public dx_iface {
    public:
        explicit DirectionInterface(dx_data& data, bool xline) : isXline(xline) {
            cData = &data;
            currentBlock = data.mBlock;
            ray.basePoint = xLine.basePoint = {11, 12, 13};
            ray.layer = xLine.layer = "0";
            suffix.basePoint = {21, 22, 23};
            suffix.secPoint = {31, 32, 33};
            suffix.handle = 0xA200u;
        }
        DRW_Line& entity() { return isXline ? static_cast<DRW_Line&>(xLine) : ray; }
        void writeEntities() override {
            callbackVisited = true;
            before = dxfW->highWaterHandle();
            recordWritten = isXline ? dxfW->writeXline(&xLine) : dxfW->writeRay(&ray);
            after = dxfW->highWaterHandle();
            suffixWritten = dxfW->writeLine(&suffix);
            afterSuffix = dxfW->highWaterHandle();
        }
        DRW_Ray ray;
        DRW_Xline xLine;
        DRW_Line suffix;
        bool isXline, callbackVisited {false}, recordWritten {false}, suffixWritten {false};
        std::uint32_t before {0}, after {0}, afterSuffix {0};
    };
    const double maximum = (std::numeric_limits<double>::max)();
    const double denormal = std::numeric_limits<double>::denorm_min();
    const double tiny = (std::numeric_limits<double>::min)();
    const double inv3 = 1.0 / std::sqrt(3.0);
    const double inv21 = 1.0 / std::sqrt(21.0);
    const double inv5 = 1.0 / std::sqrt(5.0);
    struct DirectionCase { DRW_Coord input, expected; };
    // Analytic expected vectors avoid overflow-prone peer factory normalization.
    const DirectionCase positives[] = {
        {{3, 4, 0}, {.6, .8, 0}}, {{0, .6, .8}, {0, .6, .8}},
        {{maximum, maximum, maximum}, {inv3, inv3, inv3}},
        {{denormal, 0, 0}, {1, 0, 0}},
        {{denormal, -denormal, denormal}, {inv3, -inv3, inv3}},
        {{maximum, -maximum / 2, maximum / 4}, {4 * inv21, -2 * inv21, inv21}},
        {{-2, 0, 0}, {-1, 0, 0}}, {{-0.0, 0, -1}, {-0.0, 0, -1}},
        {{maximum, 0, 0}, {1, 0, 0}}, {{0, 0, denormal}, {0, 0, 1}},
        {{1, 0, 0}, {1, 0, 0}}, {{tiny, tiny / 2, 0}, {2 * inv5, inv5, 0}}
    };
    const auto equalComponent = [](double actual, double expected) {
        return (std::isnan(actual) && std::isnan(expected))
            || (actual == expected && (actual != 0 || std::signbit(actual) == std::signbit(expected)));
    };
    const auto unchangedDirection = [&equalComponent](const DRW_Coord& actual, const DRW_Coord& input) {
        return equalComponent(actual.x, input.x) && equalComponent(actual.y, input.y)
            && equalComponent(actual.z, input.z);
    };
    const auto verifyOutput = [&t, &output](bool xline, const DRW_Coord& expected,
                                          double anchorZ, bool binary) {
        dx_data data;
        FixtureInterface reader;
        t.expect(reader.fileImport(output.string(), &data, false), "direction output imports normally");
        t.expect(data.mBlock->ent.size() == 2u, "direction record and following LINE are both retained");
        if (data.mBlock->ent.size() != 2u) return;
        const auto* direction = dynamic_cast<const DRW_Line*>(data.mBlock->ent.front());
        const auto* suffix = dynamic_cast<const DRW_Line*>(data.mBlock->ent.back());
        t.expect(direction && direction->eType == (xline ? DRW::XLINE : DRW::RAY)
                     && direction->basePoint.x == 11 && direction->basePoint.y == 12
                     && direction->basePoint.z == anchorZ,
                 "direction record preserves its WCS anchor and entity type");
        if (direction) {
            const DRW_Coord& actual = direction->secPoint;
            t.expect(std::isfinite(actual.x) && std::isfinite(actual.y) && std::isfinite(actual.z)
                         && std::abs(actual.x - expected.x) < 2e-14
                         && std::abs(actual.y - expected.y) < 2e-14
                         && std::abs(actual.z - expected.z) < 2e-14
                         && std::abs(actual.x * actual.x + actual.y * actual.y
                                     + actual.z * actual.z - 1) < 4e-14,
                     "finite accepted direction serializes to its analytic signed unit vector");
        }
        t.expect(suffix && suffix->eType == DRW::LINE && suffix->basePoint.x == 21
                     && suffix->secPoint.z == 33, "following LINE coordinates are not normalized");
        std::vector<TableOwnerRecord> records;
        t.expect(readTableOwnerRecords(output, binary, records, true),
                 "direction output frames through EOF with literal coordinate presence retained");
        const auto record = std::find_if(records.begin(), records.end(), [xline](const auto& item) {
            return item.kind == (xline ? "XLINE" : "RAY");
        });
        const bool writesZ = anchorZ != 0 || expected.z != 0;
        t.expect(record != records.end() && hasGroup(record->groups, 30) == writesZ
                     && hasGroup(record->groups, 31) == writesZ,
                 "planar zero-Z omission and nonplanar explicit-Z positive use actual raw tags");
    };
    for (const DRW::Version version : {DRW::AC1015, DRW::AC1024, DRW::AC1032}) {
        for (const bool binary : {false, true}) {
            for (const bool xline : {false, true}) {
                for (const auto& value : positives) {
                    dx_data data;
                    DirectionInterface adapter(data, xline);
                    adapter.entity().secPoint = value.input;
                    const double anchorZ = value.input.z == 0 ? 0 : 13;
                    adapter.entity().basePoint.z = anchorZ;
                    adapter.entity().handle = 0xA100u;
                    dxfRW writer(filename.c_str());
                    adapter.dxfW = &writer;
                    t.expect(writer.write(&adapter, version, binary) && adapter.callbackVisited && adapter.recordWritten
                                 && adapter.suffixWritten,
                             "finite nonzero API direction writes across DXF revisions and encodings");
                    t.expect(unchangedDirection(adapter.entity().secPoint, value.input)
                                 && adapter.entity().basePoint.x == 11 && adapter.entity().basePoint.z == anchorZ
                                 && adapter.entity().layer == "0",
                             "normalization never edits caller geometry or metadata");
                    adapter.dxfW = nullptr;
                    verifyOutput(xline, value.expected, anchorZ, binary);
                }
            }
        }
    }
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    const DRW_Coord negatives[] = {
        {0, 0, 0}, {-0.0, 0, -0.0}, {nan, 1, 2}, {1, nan, 2}, {1, 2, nan},
        {inf, 1, 2}, {1, -inf, 2}, {1, 2, inf}
    };
    for (const bool binary : {false, true}) {
        for (const bool xline : {false, true}) {
            // Keep tolerant reading separate from the stricter writer contract.
            // Author tags directly so the writer under test cannot sanitize them.
            {
                std::ofstream stream(output, std::ios::binary | std::ios::trunc);
                if (binary) stream.write("AutoCAD Binary DXF\r\n\032\0", 22);
                std::unique_ptr<dxfWriter> tags;
                if (binary) tags = std::make_unique<dxfWriterBinary>(&stream);
                else tags = std::make_unique<dxfWriterAscii>(&stream);
                tags->setVersion("AC1024", true);
                tags->writeString(0, "SECTION"); tags->writeString(2, "HEADER");
                tags->writeString(9, "$ACADVER"); tags->writeString(1, "AC1024");
                tags->writeString(0, "ENDSEC"); tags->writeString(0, "SECTION");
                tags->writeString(2, "ENTITIES"); tags->writeString(0, xline ? "XLINE" : "RAY");
                tags->writeString(8, "0"); tags->writeDouble(10, 11); tags->writeDouble(20, 12);
                tags->writeDouble(30, 13); tags->writeDouble(11, 0); tags->writeDouble(21, 0);
                tags->writeDouble(31, 0); tags->writeString(0, "ENDSEC"); tags->writeString(0, "EOF");
                t.expect(!tags->hasWriteError(), "literal zero direction control writes its raw tags");
            }
            dx_data tolerantData;
            FixtureInterface tolerantReader;
            t.expect(tolerantReader.fileImport(output.string(), &tolerantData, false)
                         && tolerantData.mBlock->ent.size() == 1u,
                     "existing tolerant reader still accepts a literal zero direction");
            if (!tolerantData.mBlock->ent.empty()) {
                const auto* direction = dynamic_cast<const DRW_Line*>(tolerantData.mBlock->ent.front());
                t.expect(direction && direction->eType == (xline ? DRW::XLINE : DRW::RAY)
                             && direction->secPoint.x == 0 && direction->secPoint.y == 0 && direction->secPoint.z == 0,
                         "tolerant reading preserves the literal zero rather than inventing a direction");
            }
            for (const std::uint32_t handle : {0u, 0xA100u}) {
                for (const DRW_Coord& input : negatives) {
                    dx_data data;
                    DirectionInterface adapter(data, xline);
                    adapter.entity().secPoint = input;
                    adapter.entity().handle = handle;
                    dxfRW writer(filename.c_str());
                    adapter.dxfW = &writer;
                    t.expect(writer.reserveHandle(0xD000u), "direction test reserves an allocator control");
                    writer.setHandleRemap({{0xE100u, 0xE101u}});
                    { std::ofstream original(output, std::ios::binary); original << "direction destination\n"; }
                    t.expect(!writer.write(&adapter, DRW::AC1024, binary) && adapter.callbackVisited && !adapter.recordWritten
                                 && !adapter.suffixWritten,
                             "zero and nonfinite directions reject and latch suffix writes");
                    t.expect(adapter.before > 0 && adapter.before == adapter.after && adapter.after == adapter.afterSuffix
                                 && adapter.entity().handle == handle && adapter.suffix.handle == 0xA200u
                                 && unchangedDirection(adapter.entity().secPoint, input),
                             "direction rejection precedes identity allocation and caller mutation");
                    t.expect(contents() == "direction destination\n"
                                 && writer.remapHandle(0xE100u) == 0xE101u
                                 && writer.getWritingContext()->sourceHandleToMintedMap.empty()
                                 && writer.getWritingContext()->writtenSourceEntityHandles.empty(),
                             "direction rejection preserves destination and persistent remaps without derived maps");
                    adapter.entity().secPoint = {3, 4, 0};
                    t.expect(writer.write(&adapter, DRW::AC1024, binary) && adapter.recordWritten
                                 && adapter.suffixWritten,
                             "correcting only direction permits same-writer same-model retry");
                    t.expect(unchangedDirection(adapter.entity().secPoint, {3, 4, 0}),
                             "retry normalization retains caller nonunit direction");
                    adapter.dxfW = nullptr;
                    verifyOutput(xline, {.6, .8, 0}, 13, binary);
                }
            }
        }
    }
}

void testAsciiOnlySymbolCasing(TestContext& t) {
    // Two-, three- and four-byte UTF-8 sequences must reach the codec intact.
    const std::string suffix = "_\xC2\xA2_\xC3\xA9_\xCE\xA9_\xE2\x82\xAC_\xE4\xB8\xAD_\xF0\x9F\x98\x80";
    const std::string input = "abcdefghijklmnopqrstuvwxyz_09_.Z" + suffix;
    const std::string expected = "ABCDEFGHIJKLMNOPQRSTUVWXYZ_09_.Z" + suffix;
    for (const bool binary : {false, true}) {
        for (const char* version : {"AC1009", "AC1021"}) {
            std::ostringstream actualBytes, expectedBytes;
            std::unique_ptr<dxfWriter> actual, reference;
            if (binary) {
                actual = std::make_unique<dxfWriterBinary>(&actualBytes);
                reference = std::make_unique<dxfWriterBinary>(&expectedBytes);
            } else {
                actual = std::make_unique<dxfWriterAscii>(&actualBytes);
                reference = std::make_unique<dxfWriterAscii>(&expectedBytes);
            }
            actual->setVersion(version, true);
            reference->setVersion(version, true);
            t.expect(actual->writeUtf8Caps(2, input)
                         && reference->writeUtf8String(2, expected)
                         && actualBytes.str() == expectedBytes.str(),
                     "legacy caps changes only ASCII before either target codec");
            t.expect(input == "abcdefghijklmnopqrstuvwxyz_09_.Z" + suffix,
                     "UTF-8 capitalization never edits the caller's string");
            if (std::string(version) == "AC1021")
                t.expect(actualBytes.str().find(expected) != std::string::npos,
                         "modern output contains the exact valid UTF-8 byte sequence");
        }
    }
    std::ostringstream hexBytes;
    dxfWriterBinary hex(&hexBytes);
    const std::string hexExpected("\x36\x01\x08\xAB\xCD\xEF\x01\x23\x45\x67\x89", 11);
    t.expect(hex.writeString(310, "abcdef0123456789")
                 && hexBytes.str() == hexExpected,
             "binary hex digits retain deterministic ASCII case acceptance");
    std::string lowerKeywords =
        "0\nsection\n2\nheader\n9\n$ACADVER\n1\nAC1021\n0\nendsec\n"
        "0\nsection\n2\nentities\n0\nline\n8\n0\n10\n1\n20\n2\n"
        "11\n3\n21\n4\n0\nendsec\n0\neof\n";
    const std::string originalKeywords(lowerKeywords);
    FixtureInterface keywordReader;
    dx_data keywordData;
    keywordReader.attach(&keywordData);
    dxfRW keywordCodec("unused-ascii-casing.dxf");
    t.expect(keywordCodec.readAscii(&keywordReader, false, lowerKeywords)
                 && keywordData.mBlock->ent.size() == 1
                 && keywordData.mBlock->ent.front()->eType == DRW::LINE
                 && lowerKeywords == originalKeywords,
             "ASCII keyword comparison and entity dispatch ignore process CTYPE");

    class CasingInterface final : public dx_iface {
    public:
        void writeEntities() override {
            const std::string lower = "\xC3\xA9", upper = "\xC3\x89";
            const auto firstStyle = dxfW->getTextStyleHandle("MIX_" + lower);
            const auto secondStyle = dxfW->getTextStyleHandle("MIX_" + upper);
            const auto firstBlock = dxfW->getBlockRecordHandleToWrite("MIX_" + lower);
            const auto secondBlock = dxfW->getBlockRecordHandleToWrite("MIX_" + upper);
            lookups = firstStyle != 0 && secondStyle != 0 && firstStyle != secondStyle
                && firstBlock != 0 && secondBlock != 0 && firstBlock != secondBlock;
            DRW_Text text;
            text.text = "case sentinel";
            text.style = "MIX_" + lower;
            text.height = 1.0;
            DRW_Insert insert;
            insert.name = "MIX_" + upper;
            writes = dxfW->writeText(&text) && dxfW->writeInsert(&insert);
            sourceStrings = text.style == "MIX_" + lower && insert.name == "MIX_" + upper;
        }
        bool lookups {false};
        bool writes {false};
        bool sourceStrings {false};
    };
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    for (const bool binary : {false, true}) {
        const auto path = std::filesystem::temp_directory_path()
            / ("libdxfrw-ascii-casing-" + std::to_string(nonce)
               + (binary ? "-binary.dxf" : "-ascii.dxf"));
        dx_data data;
        DRW_LType canonicalLineType;
        canonicalLineType.name = "continuous";
        data.lineTypes.push_back(canonicalLineType);
        for (const char* ending : {"\xC3\xA9", "\xC3\x89"}) {
            DRW_Textstyle style;
            style.name = std::string("mix_") + ending;
            style.font = "CustomFont.ttf";
            data.textStyles.push_back(style);
            auto block = std::make_unique<dx_ifaceBlock>();
            block->name = style.name;
            data.blocks.push_back(block.release());
        }
        CasingInterface adapter;
        t.expect(adapter.fileExport(path.string(), DRW::AC1021, binary, &data, false)
                     && adapter.lookups && adapter.writes && adapter.sourceStrings,
                 "typed STYLE/BLOCK lookups fold ASCII without merging non-ASCII names");
        auto block = data.blocks.begin();
        for (const DRW_Textstyle& style : data.textStyles) {
            t.expect(style.name == (*block)->name && style.name.substr(0, 4) == "mix_"
                         && style.font == "CustomFont.ttf",
                     "typed writer retains source symbol and modern font spelling");
            ++block;
        }
        FixtureInterface reader;
        dx_data imported;
        t.expect(reader.fileImport(path.string(), &imported, false),
                 "mixed-case UTF-8 typed output reads without recovery");
        std::size_t found = 0;
        for (const DRW_Textstyle& style : imported.textStyles) {
            if (style.name == "mix_\xC3\xA9" || style.name == "mix_\xC3\x89") {
                ++found;
                t.expect(style.font == "CustomFont.ttf",
                         "modern STYLE filename keeps its original case");
            }
        }
        t.expect(found == 2, "distinct non-ASCII source names survive typed round trip");
        std::size_t continuous = 0;
        for (const DRW_LType& item : imported.lineTypes) {
            if (item.name == "Continuous" || item.name == "CONTINUOUS" || item.name == "continuous")
                ++continuous;
        }
        t.expect(continuous == 1 && data.lineTypes.front().name == "continuous",
                 "default LTYPE comparison does not duplicate or rename the source record");
        std::error_code error;
        std::filesystem::remove(path, error);

        dx_data legacy;
        DRW_Textstyle style;
        style.name = "MixStyle";
        style.font = "CustomFont.ttf";
        style.bigFont = "BigFont.shx";
        legacy.textStyles.push_back(style);
        FixtureInterface legacyWriter;
        t.expect(legacyWriter.fileExport(path.string(), DRW::AC1009, binary, &legacy, false),
                 "R12 STYLE capitalization compatibility control writes");
        FixtureInterface legacyReader;
        dx_data legacyRead;
        t.expect(legacyReader.fileImport(path.string(), &legacyRead, false),
                 "R12 STYLE capitalization compatibility control reads");
        bool legacyFound = false;
        for (const DRW_Textstyle& item : legacyRead.textStyles) {
            if (item.name == "MIXSTYLE")
                legacyFound = item.font == "CUSTOMFONT.TTF" && item.bigFont == "BIGFONT.SHX";
        }
        t.expect(legacyFound && legacy.textStyles.front().name == "MixStyle"
                     && legacy.textStyles.front().font == "CustomFont.ttf"
                     && legacy.textStyles.front().bigFont == "BigFont.shx",
                 "R12 paths keep legacy ASCII caps without mutating caller fields");
        std::filesystem::remove(path, error);
    }
}

void testDxfConsumerFinalization(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-finalize-" + std::to_string(nonce));
    std::filesystem::create_directory(directory);
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove_all(path, error); }
    } cleanup{directory};
    const auto contents = [](const std::filesystem::path& path) {
        std::ifstream stream(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(stream), {});
    };
    const auto temporaryCount = [](const std::filesystem::path& path) {
        std::size_t count = 0;
        for (const auto& entry : std::filesystem::directory_iterator(path))
            if (entry.path().filename().string().find(".libdxfrw-") != std::string::npos)
                ++count;
        return count;
    };
    class FinalizeInterface final : public dx_iface {
    public:
        enum class Mode {
            Accept, Reject, StandardThrow, UnknownThrow, LateRecord, Allocate,
            Reserve, Ambiguous, Classes, RawSections, RootEntries, NamedDicts,
            Groups, Remap, SourceBlocks, CanonicalLTypes, Bind, Nested, NestedLateSetup
        };
        FinalizeInterface(dx_data& data, TestContext& context)
            : test(context) {
            cData = &data;
            currentBlock = data.mBlock;
        }
        void writeEntities() override {
            // The finalizer runs after this borrowed record has expired. It
            // must never restore it during rejection/exception cleanup.
            {
                DRW_Line prefix;
                prefix.handle = 0xB100u;
                prefix.parentHandle = unresolvedOwner ? 0xDEADu : 0;
                prefix.secPoint = {1, 2, 3};
                prefixResult = dxfW->writeLine(&prefix);
                prefixHandle = prefix.handle;
            }
            prefixExpired = true;
            if (earlyFailure)
                test.expect(!dxfW->writeLine(nullptr), "early invalid record latches write failure");
        }
        bool finalizeDxfWrite() override {
            ++calls;
            test.expect(prefixExpired && prefixResult
                            && dxfW->remapEntityHandle(0xB100u) == prefixHandle,
                        "finalizer inspects completed results after stack-local prefix expiry");
            if (!finalPath.empty()) {
                std::ifstream destination(finalPath, std::ios::binary);
                const std::string bytes(std::istreambuf_iterator<char>(destination), {});
                test.expect(std::filesystem::exists(finalPath) == destinationExisted
                                && (!destinationExisted || bytes == "finalize sentinel\n"),
                            "consumer finalization precedes destination publication");
                const std::string prefix = finalPath.filename().string() + ".libdxfrw-";
                std::size_t pending = 0;
                for (const auto& entry : std::filesystem::directory_iterator(finalPath.parent_path())) {
                    if (entry.path().filename().string().find(prefix) != 0) continue;
                    ++pending;
                    FixtureInterface reader;
                    dx_data decoded;
                    reader.attach(&decoded);
                    const std::string pendingName = entry.path().string();
                    dxfRW pendingReader(pendingName.c_str());
                    test.expect(pendingReader.read(&reader, false)
                                    && decoded.mBlock->ent.size() == 1u,
                                "consumer sees a flushed complete readable temporary before commit");
                }
                test.expect(pending == 1, "finalization owns exactly one unpublished temporary");
            }
            const auto before = dxfW->highWaterHandle();
            switch (mode) {
            case Mode::Reject: return false;
            case Mode::StandardThrow: throw std::runtime_error("finalize standard exception");
            case Mode::UnknownThrow: throw 7;
            case Mode::LateRecord: {
                DRW_Line late;
                late.handle = 0xB200u;
                test.expect(!dxfW->writeLine(&late) && late.handle == 0xB200u,
                            "finalizer rejects record emission without caller mutation");
                break;
            }
            case Mode::Allocate:
                test.expect(dxfW->allocHandle() == 0, "finalizer cannot advance HANDSEED by allocating");
                break;
            case Mode::Reserve:
                test.expect(!dxfW->reserveHandle((std::numeric_limits<std::uint32_t>::max)()),
                            "finalizer rejects even an overflowing reservation before generation mutation");
                break;
            case Mode::Ambiguous: {
                const auto count = dxfW->getWritingContext()->ambiguousSourceHandles.size();
                dxfW->markSourceHandleAmbiguous(0xB100u);
                test.expect(dxfW->getWritingContext()->ambiguousSourceHandles.size() == count
                                && dxfW->remapEntityHandle(0xB100u) == prefixHandle,
                            "finalizer cannot mark an emitted source identity ambiguous");
                break;
            }
            case Mode::Classes: {
                DRW_Class value;
                test.expect(dxfRW::dxfClassForRecordName("SUN", value), "known CLASS control exists");
                dxfW->setDxfClasses({value});
                break;
            }
            case Mode::RawSections: {
                DRW_RawDxfSection value;
                value.m_name = "FINALIZE_SECTION";
                value.m_version = DRW::AC1027;
                value.m_groups = {{0, std::string("FINALIZE_RECORD")}};
                dxfW->setRawDxfSections({value});
                break;
            }
            case Mode::RootEntries: dxfW->setRootDictEntries({{"FINALIZE_DICT", "F100"}}); break;
            case Mode::NamedDicts: {
                DRW_Dictionary value;
                value.handle = 0xF100u;
                value.parentHandle = 0xCu;
                dxfW->setNamedDictObjects({value});
                break;
            }
            case Mode::Groups: {
                DRW_Group value;
                value.name = "FINALIZE_GROUP";
                value.m_entityHandles = {0xB100u};
                dxfW->setGroups({value});
                break;
            }
            case Mode::Remap:
                dxfW->setHandleRemap({{0xE100u, 0xE102u}});
                test.expect(dxfW->remapHandle(0xE100u) == 0xE101u,
                            "finalizer leaves caller's explicit remaps unchanged");
                break;
            case Mode::SourceBlocks: {
                DRW_Block_Record value;
                value.handle = 0xF200u;
                value.name = "FINALIZE_BLOCK";
                dxfW->setSourceBlockRecords({value}, true);
                break;
            }
            case Mode::CanonicalLTypes: {
                DRW_LType value;
                value.name = "CONTINUOUS";
                value.desc = "late metadata";
                dxfW->setCanonicalLineTypeMetadata({value});
                break;
            }
            case Mode::Bind:
                test.expect(dxfW->preallocateEntityHandle(0xB200u) == 0
                                && !dxfW->bindSourceEntityHandle(0xB200u, 0xF300u),
                            "finalizer cannot preallocate or bind a source entity");
                break;
            case Mode::Nested:
            case Mode::NestedLateSetup: {
                const auto diagnostic = dxfW->getLastDiagnostic();
                std::string content = "0\nEOF\n";
                test.expect(!dxfW->write(this, DRW::AC1027, false)
                                && !dxfW->read(this, false)
                                && !dxfW->readAscii(this, false, content)
                                && dxfW->getError() == DRW::BAD_NONE
                                && dxfW->getLastDiagnostic().code == diagnostic.code,
                            "same-facade nested whole operations cannot disturb finalization");
                dx_data otherData;
                FinalizeInterface other(otherData, test);
                dxfRW otherWriter(otherFile.c_str());
                other.dxfW = &otherWriter;
                test.expect(otherWriter.write(&other, DRW::AC1027, false) && other.calls == 1,
                            "a different facade may complete inside finalization");
                other.dxfW = nullptr;
                if (mode == Mode::NestedLateSetup)
                    test.expect(dxfW->allocHandle() == 0,
                                "rejected nested operations cannot reopen the finalization phase");
                break;
            }
            case Mode::Accept: break;
            }
            test.expect(dxfW->highWaterHandle() == before,
                        "finalizer never advances completed output's handle high-water mark");
            return true; // A forbidden mutation must still veto publication.
        }
        TestContext& test;
        Mode mode {Mode::Accept};
        int calls {0};
        bool earlyFailure {false}, unresolvedOwner {false};
        bool prefixResult {false}, prefixExpired {false};
        std::uint32_t prefixHandle {0};
        std::string otherFile;
        std::filesystem::path finalPath;
        bool destinationExisted {false};
    };
    using Mode = FinalizeInterface::Mode;
    const auto prepare = [&t](dxfRW& writer) {
        t.expect(writer.reserveHandle(0xE200u), "between-operation reservation is still accepted");
        writer.setHandleRemap({{0xE100u, 0xE101u}});
    };
    const auto emptyResults = [](dxfRW& writer) {
        const auto& state = *writer.getWritingContext();
        return state.sourceHandleToMintedMap.empty() && state.writtenSourceEntityHandles.empty()
            && state.ambiguousSourceHandles.empty() && state.lineTypesMap.empty();
    };
    const auto target = directory / "finalize.dxf";
    const std::string filename = target.string();
    for (const auto version : {DRW::AC1009, DRW::AC1027}) {
        for (const bool binary : {false, true}) {
            for (const bool existing : {false, true}) {
                for (const auto mode : {Mode::Accept, Mode::Reject, Mode::StandardThrow,
                                        Mode::UnknownThrow, Mode::LateRecord, Mode::Nested,
                                        Mode::NestedLateSetup}) {
                    std::error_code error;
                    std::filesystem::remove(target, error);
                    if (existing) { std::ofstream seed(target, std::ios::binary); seed << "finalize sentinel\n"; }
                    dx_data data;
                    FinalizeInterface adapter(data, t);
                    adapter.mode = mode;
                    adapter.finalPath = target;
                    adapter.destinationExisted = existing;
                    adapter.otherFile = (directory / "nested.dxf").string();
                    dxfRW writer(filename.c_str());
                    adapter.dxfW = &writer;
                    prepare(writer);
                    const bool accepted = mode == Mode::Accept || mode == Mode::Nested;
                    t.expect(writer.write(&adapter, version, binary) == accepted && adapter.calls == 1,
                             "DXF finalizer runs exactly once across revisions and encodings");
                    if (!accepted) {
                        const auto diagnostic = writer.getLastDiagnostic();
                        const bool throwing = mode == Mode::StandardThrow || mode == Mode::UnknownThrow;
                        t.expect(writer.getError() == DRW::BAD_OPEN && emptyResults(writer)
                                     && diagnostic.phase == DRW::OperationPhase::Callback
                                     && diagnostic.cause == (throwing ? DRW::OperationCause::CallbackException
                                                                      : DRW::OperationCause::WriteFailure)
                                     && diagnostic.code == (throwing ? "callback-exception" : "dxf-finalize-rejected"),
                                 "veto/exception keeps the first structured cause and clears derived identities");
                        t.expect(existing ? contents(target) == "finalize sentinel\n"
                                          : !std::filesystem::exists(target),
                                 "consumer rejection preserves every destination byte or creates no destination");
                        adapter.mode = Mode::Accept;
                        adapter.finalPath.clear(); // Retry has its own already-checked pre-commit boundary.
                        prepare(writer);
                        t.expect(writer.write(&adapter, version, binary) && adapter.calls == 2
                                     && writer.getError() == DRW::BAD_NONE,
                                 "same writer retries from pristine stack-local source identities after finalization failure");
                    }
                    t.expect(temporaryCount(directory) == 0, "finalization leaves no transaction temporary");
                    adapter.dxfW = nullptr;
                }
            }
            // Existing adapters inherit the no-op acceptance callback.
            dx_data legacyData;
            FixtureInterface legacy;
            legacy.attach(&legacyData);
            dxfRW writer(filename.c_str());
            legacy.dxfW = &writer;
            t.expect(writer.write(&legacy, version, binary), "adapter without finalizer override retains old behavior");
            legacy.dxfW = nullptr;
            dx_data earlyData;
            FinalizeInterface early(earlyData, t);
            early.earlyFailure = true;
            dxfRW earlyWriter(filename.c_str());
            early.dxfW = &earlyWriter;
            t.expect(!earlyWriter.write(&early, version, binary) && early.calls == 0,
                     "R12 and modern earlier record failures invoke no finalizer");
            early.dxfW = nullptr;
        }
    }
    for (const bool binary : {false, true}) {
        for (const auto mode : {Mode::Allocate, Mode::Reserve, Mode::Ambiguous, Mode::Classes,
                                Mode::RawSections, Mode::RootEntries, Mode::NamedDicts, Mode::Groups,
                                Mode::Remap, Mode::SourceBlocks, Mode::CanonicalLTypes, Mode::Bind}) {
            dx_data data;
            FinalizeInterface adapter(data, t);
            adapter.mode = mode;
            dxfRW writer(filename.c_str());
            adapter.dxfW = &writer;
            prepare(writer);
            { std::ofstream seed(target, std::ios::binary); seed << "setup sentinel\n"; }
            t.expect(!writer.write(&adapter, DRW::AC1027, binary) && adapter.calls == 1
                         && writer.getLastDiagnostic().code == "dxf-finalize-rejected"
                         && contents(target) == "setup sentinel\n" && emptyResults(writer),
                     "each finalizer-only setup mutation latches rejection even when callback returns true");
            adapter.mode = Mode::Accept;
            prepare(writer);
            t.expect(writer.write(&adapter, DRW::AC1027, binary) && adapter.calls == 2,
                     "forbidden finalizer setup does not poison retry or reservation generations");
            const std::string retried = contents(target);
            dx_data freshData;
            FinalizeInterface fresh(freshData, t);
            dxfRW freshWriter(filename.c_str());
            fresh.dxfW = &freshWriter;
            prepare(freshWriter);
            t.expect(freshWriter.write(&fresh, DRW::AC1027, binary) && contents(target) == retried,
                     "rejected finalizer registrations leave pristine retry byte-identical to fresh writer");
            fresh.dxfW = nullptr;
            adapter.dxfW = nullptr;
            t.expect(temporaryCount(directory) == 0, "late setup rejection/retry cleans its temporary");
        }
        for (const bool structured : {false, true}) {
            dx_data data;
            FinalizeInterface adapter(data, t);
            adapter.earlyFailure = !structured;
            adapter.unresolvedOwner = structured;
            dxfRW writer(filename.c_str());
            adapter.dxfW = &writer;
            if (structured) writer.setSourceBlockRecords({}, true);
            t.expect(!writer.write(&adapter, DRW::AC1027, binary) && adapter.calls == 0,
                     "earlier emission failure never invokes consumer finalization");
            if (structured)
                t.expect(writer.getLastDiagnostic().code == "unresolved-block-owner",
                         "skipping finalization preserves the first earlier diagnostic");
            adapter.dxfW = nullptr;
        }
        const auto commitTarget = directory / "destination-directory";
        std::filesystem::create_directory(commitTarget);
        dx_data data;
        FinalizeInterface adapter(data, t);
        const std::string commitName = commitTarget.string();
        dxfRW writer(commitName.c_str());
        adapter.dxfW = &writer;
        t.expect(!writer.write(&adapter, DRW::AC1027, binary) && adapter.calls == 1
                     && writer.getLastDiagnostic().phase == DRW::OperationPhase::Commit
                     && writer.getLastDiagnostic().code == "commit-failure" && emptyResults(writer)
                     && std::filesystem::is_directory(commitTarget) && temporaryCount(directory) == 0,
                 "consumer acceptance does not override a later publication failure");
        std::error_code error;
        t.expect(std::filesystem::remove(commitTarget, error), "commit-failure test removes only its empty blocker");
        prepare(writer);
        t.expect(writer.write(&adapter, DRW::AC1027, binary) && adapter.calls == 2
                     && writer.getError() == DRW::BAD_NONE && temporaryCount(directory) == 0,
                 "same writer recovers after accepted finalization and failed publication");
        std::filesystem::remove(commitTarget, error);
        adapter.dxfW = nullptr;
    }
}

// The operation owns emitted identities, not the lifetime of borrowed callback
// records. A failed operation must not publish suffix mutations or its maps.
void testDxfWriteAttemptFailure(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-write-attempt-" + std::to_string(nonce));
    std::filesystem::create_directory(directory);
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove_all(path, error); }
    } cleanup{directory};
    const auto output = directory / "attempt.dxf";
    const std::string fileName = output.string();
    const auto contents = [](const std::filesystem::path& path) {
        std::ifstream stream(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(stream), {});
    };
    const auto emptyResults = [](dxfRW& codec) {
        const auto& context = *codec.getWritingContext();
        return context.lineTypesMap.empty()
            && context.sourceHandleToMintedMap.empty()
            && context.modelerSourceHandleToMintedMap.empty()
            && context.writtenSourceEntityHandles.empty()
            && context.ambiguousSourceHandles.empty()
            && codec.getTextStyleHandle("STANDARD") == DRW::NoHandle
            && codec.getBlockRecordHandleToWrite("ATTEMPT_BLOCK") == DRW::NoHandle;
    };
    class AttemptInterface final : public dx_iface {
    public:
        explicit AttemptInterface(dx_data& data) {
            cData = &data;
            currentBlock = data.mBlock;
            bad.text = "prefix";
            bad.m_backgroundColorName = "unsafe\nname";
            line.handle = 0xB200u;
            line.secPoint = DRW_Coord(1, 2, 3);
            poly.handle = 0xB100u;
            poly.addVertex(DRW_Vertex(1, 2, 0, 0));
            poly.addVertex(DRW_Vertex(3, 4, 0, 0));
            poly.vertlist[0]->handle = 0xB101u;
            poly.vertlist[1]->handle = 0xB102u;
            poly.extData.push_back(std::make_shared<DRW_Variant>(1001, "ATTEMPT_APP"));
            poly.extData.push_back(std::make_shared<DRW_Variant>(1005, "B100"));
            insert.handle = 0xB300u;
            insert.name = "ATTEMPT_BLOCK";
            auto attribute = std::make_shared<DRW_Attrib>();
            attribute->handle = 0xB301u;
            attribute->tag = "CHILD";
            attribute->text = "child";
            attribute->height = 1;
            insert.attlist.push_back(attribute);
            layer.name = "ATTEMPT_LAYER";
            layer.handle = 0xB400u;
            sun.handle = 0xB500u;
            raw.name = "ATTEMPT_RAW";
            raw.groups.emplace_back(5, std::string("B600"));
            raw.groups.emplace_back(1, std::string("raw payload"));
        }

        void ephemeralPrefix() {
            DRW_Line temporary;
            temporary.handle = 0xC100u;
            temporary.secPoint = DRW_Coord(4, 5, 6);
            prefixResult = dxfW->writeLine(&temporary);
            DRW_Image image;
            image.handle = 0xC200u;
            image.secPoint = DRW_Coord(1, 0, 0);
            image.vVector = DRW_Coord(0, 1, 0);
            image.sizeu = image.sizev = 1;
            prefixResult = (dxfW->writeImage(&image, "attempt-image.png") != nullptr)
                && prefixResult;
        }

        void writeEntities() override {
            if (ephemeral) {
                ephemeralPrefix(); // both temporary records are now dead
                if (structuredFailure) {
                    DRW_Line unresolved;
                    unresolved.parentHandle = 0xFFFFu;
                    unresolved.secPoint = DRW_Coord(9, 8, 7);
                    badResult = dxfW->writeLine(&unresolved);
                    throw std::runtime_error("failure after a structured first cause");
                }
                if (throwAfterPrefix)
                    throw std::runtime_error("intentional callback failure");
            } else if (successfulPrefix) {
                prefixResult = dxfW->writePolyline(&poly);
            }
            if (!successfulOnly)
                badResult = dxfW->writeMText(&bad);
            if (ephemeral || successfulPrefix)
                return;
            beforeSuffix = dxfW->highWaterHandle();
            switch (suffix) {
            case 0: suffixResult = dxfW->writePolyline(&poly); break;
            case 1: suffixResult = dxfW->writeLine(&line); break;
            case 2: suffixResult = dxfW->writeInsert(&insert); break;
            case 3: suffixResult = dxfW->writeLayer(&layer); break;
            case 4: suffixResult = dxfW->writeRawDxfObject(&raw); break;
            case 5: suffixResult = dxfW->writeSun(&sun); break;
            case 6: suffixResult = dxfW->writeBlockRecord("LATE_BLOCK"); break;
            }
            afterSuffix = dxfW->highWaterHandle();
            if (!badResult && !successfulOnly) {
                plannedAfterFailure = dxfW->preallocateEntityHandle(0xD100u);
                boundAfterFailure = dxfW->bindSourceEntityHandle(0xD200u, 0xD201u);
            }
        }

        void writeBlockRecords() override {
            dx_iface::writeBlockRecords();
            if (badBlockPlanning)
                dxfW->writeBlockRecord("ATTEMPT_BLOCK"); // duplicate while outer state scope is live
        }

        DRW_MText bad;
        DRW_Line line;
        DRW_Polyline poly;
        DRW_Insert insert;
        DRW_Layer layer;
        DRW_Sun sun;
        DRW_RawDxfObject raw;
        int suffix {0};
        bool successfulPrefix {false}, successfulOnly {false};
        bool ephemeral {false}, throwAfterPrefix {false}, badBlockPlanning {false};
        bool structuredFailure {false};
        bool badResult {false}, suffixResult {false}, prefixResult {false};
        bool boundAfterFailure {false};
        std::uint32_t plannedAfterFailure {0}, beforeSuffix {0}, afterSuffix {0};
    };
    const auto populate = [](dx_data& data) {
        DRW_AppId app;
        app.name = "ATTEMPT_APP";
        data.appIds.push_back(app);
        auto* block = new dx_ifaceBlock;
        block->name = "ATTEMPT_BLOCK";
        data.blocks.push_back(block);
    };
    const auto rawSelfReference = [&t, &output](bool binary) {
        std::vector<TableOwnerRecord> records;
        t.expect(readTableOwnerRecords(output, binary, records),
                 "retry raw framing reaches EOF");
        bool found = false;
        for (const auto& record : records) {
            if (record.kind != "POLYLINE") continue;
            found = true;
            t.expect(groupValue(record.groups, 5) == groupValue(record.groups, 1005),
                     "retry raw self-XDATA equals the actual emitted POLYLINE identity");
        }
        t.expect(found, "retry emits the POLYLINE control");
    };
    for (const bool binary : {false, true}) {
        for (int suffix = 0; suffix != 7; ++suffix) {
            dx_data data;
            populate(data);
            AttemptInterface adapter(data);
            adapter.suffix = suffix;
            dxfRW codec(fileName.c_str());
            adapter.dxfW = &codec;
            codec.setHandleRemap({{0xE100u, 0xE101u}});
            t.expect(codec.reserveHandle(0xE200u), "caller reservation is accepted before emission");
            if (suffix == 0) {
                DRW_Class cls;
                t.expect(dxfRW::dxfClassForRecordName("SUN", cls), "persistent class control is available");
                codec.setDxfClasses({cls});
                DRW_RawDxfSection section;
                section.m_name = "ATTEMPT_SECTION";
                section.m_version = DRW::AC1027;
                section.m_groups = {{0, std::string("ATTEMPT_RECORD")}, {1, std::string("persistent payload")}};
                codec.setRawDxfSections({section});
                DRW_Dictionary dictionary;
                dictionary.handle = 0xE300u;
                dictionary.parentHandle = 0xCu;
                codec.setNamedDictObjects({dictionary});
                codec.setRootDictEntries({{"ATTEMPT_DICTIONARY", "E300"}});
                t.expect(codec.reserveHandle(dictionary.handle), "persistent dictionary identity is reserved");
                DRW_Group group;
                group.name = "ATTEMPT_GROUP";
                group.m_entityHandles = {0xB100u};
                codec.setGroups({group});
                DRW_Block_Record model;
                model.name = "*Model_Space";
                model.handle = 0xE400u;
                codec.setSourceBlockRecords({model}, true);
                adapter.poly.parentHandle = model.handle;
            }
            { std::ofstream seed(output); seed << "attempt sentinel\n"; }
            t.expect(!codec.write(&adapter, DRW::AC1027, binary) && !adapter.badResult,
                     "rejected prefix fails the complete DXF attempt");
            t.expect(!adapter.suffixResult && adapter.beforeSuffix == adapter.afterSuffix
                         && adapter.plannedAfterFailure == 0 && !adapter.boundAfterFailure,
                     "latched facade failure stops suffix writes and identity planning before mutation");
            t.expect(adapter.poly.handle == 0xB100u
                         && adapter.poly.vertlist[0]->handle == 0xB101u
                         && adapter.poly.vertlist[1]->handle == 0xB102u
                         && adapter.line.handle == 0xB200u && adapter.insert.handle == 0xB300u
                         && adapter.insert.attlist[0]->handle == 0xB301u
                         && adapter.layer.handle == 0xB400u && adapter.layer.name == "ATTEMPT_LAYER",
                     "rejected-prefix suffix preserves caller and child identities");
            t.expect(emptyResults(codec) && codec.remapEntityHandle(0xB100u) == 0xB100u,
                     "failed attempts expose no derived identity results");
            t.expect(codec.remapHandle(0xE100u) == 0xE101u
                         && contents(output) == "attempt sentinel\n",
                     "failure preserves explicit remaps and the existing destination");
            if (suffix == 0) {
                adapter.bad.m_backgroundColorName.clear();
                t.expect(codec.write(&adapter, DRW::AC1027, binary),
                         "correcting only the rejected prefix permits same-model same-writer retry");
                t.expect(adapter.poly.handle > 0xE200u
                             && codec.remapEntityHandle(0xB100u) == adapter.poly.handle,
                         "retry preserves caller reservations and successful handle/map behavior");
                rawSelfReference(binary);
                std::vector<TableOwnerRecord> persistent;
                t.expect(readTableOwnerRecords(output, binary, persistent), "retry persistent registrations frame correctly");
                bool cls = false, section = false, dictionary = false, group = false, owner = false;
                for (const auto& record : persistent) {
                    cls = cls || (record.kind == "CLASS" && groupValue(record.groups, 1) == "SUN");
                    section = section || (record.kind == "SECTION" && groupValue(record.groups, 2) == "ATTEMPT_SECTION");
                    dictionary = dictionary || (record.kind == "DICTIONARY" && groupValue(record.groups, 5) == "E300");
                    group = group || (record.kind == "GROUP" && groupValue(record.groups, 340)
                        == codec.toHexStrHandle(adapter.poly.handle));
                    owner = owner || (record.kind == "POLYLINE" && groupValue(record.groups, 330)
                        == codec.toHexStrHandle(DRW::DxfModelSpaceBlockRecordHandle));
                }
                t.expect(cls && section && dictionary && group && owner,
                         "failed attempt retains classes/raw sections/named dictionaries/groups/source-owner registrations");
                const auto expectEarlyRejection = [&](DRW::Version version, DRW_Interface* iface,
                                                       const char* diagnostic) {
                    const std::string successfulFile = contents(output);
                    t.expect(!codec.write(iface, version, binary) && emptyResults(codec)
                                 && codec.getLastDiagnostic().code == diagnostic
                                 && contents(output) == successfulFile,
                             "pre-session rejection discards previous results but preserves first cause and output");
                };
                expectEarlyRejection(DRW::UNKNOWNV, &adapter, "unsupported-version");
                // Restore the pristine source identity before another complete
                // export; successful writes intentionally mutate caller handles.
                adapter.poly.handle = 0xB100u;
                t.expect(codec.write(&adapter, DRW::AC1027, binary), "writer can recover after version rejection");
                expectEarlyRejection(DRW::AC1027, nullptr, "invalid-argument");
                adapter.poly.handle = 0xB100u;
                t.expect(codec.write(&adapter, DRW::AC1027, binary), "writer can recover after null-interface rejection");
                t.expect(!codec.reserveHandle((std::numeric_limits<std::uint32_t>::max)()),
                         "overflowing reservation supplies an early-admission negative");
                expectEarlyRejection(DRW::AC1027, &adapter, "handle-reservation");
            }
            adapter.dxfW = nullptr;
        }
        {
            dx_data data;
            populate(data);
            AttemptInterface adapter(data);
            adapter.badBlockPlanning = true;
            dxfRW codec(fileName.c_str());
            adapter.dxfW = &codec;
            { std::ofstream seed(output); seed << "planning sentinel\n"; }
            t.expect(!codec.write(&adapter, DRW::AC1027, binary)
                         && emptyResults(codec) && contents(output) == "planning sentinel\n",
                     "failed pre-collection unwinds its state scope before session-result cleanup");
            adapter.badBlockPlanning = false;
            adapter.bad.m_backgroundColorName.clear();
            t.expect(codec.write(&adapter, DRW::AC1027, binary),
                     "same writer recovers after failed block pre-collection");
            rawSelfReference(binary);
            adapter.dxfW = nullptr;
        }
        // A successful prefix remains a committed call; automatic whole-file
        // restoration of borrowed records is deliberately not promised.
        for (int mode = 0; mode != 5; ++mode) {
            dx_data data;
            populate(data);
            AttemptInterface adapter(data);
            adapter.successfulPrefix = mode == 0;
            adapter.ephemeral = mode != 0;
            adapter.throwAfterPrefix = mode == 2;
            adapter.successfulOnly = mode == 3;
            adapter.structuredFailure = mode == 4;
            const auto commitTarget = directory / "destination-directory";
            if (mode == 3) std::filesystem::create_directory(commitTarget);
            const std::string target = mode == 3 ? commitTarget.string() : fileName;
            dxfRW codec(target.c_str());
            if (mode == 4)
                codec.setSourceBlockRecords({}, true);
            adapter.dxfW = &codec;
            t.expect(!codec.write(&adapter, DRW::AC1027, binary) && adapter.prefixResult,
                     "later validation/exception/publication failure follows a successful prefix");
            t.expect(emptyResults(codec), "later failure clears all derived results after callback records expire");
            if (mode == 0)
                t.expect(adapter.poly.handle != 0xB100u,
                         "whole-file failure does not claim rollback of an earlier successful caller record");
            if (mode >= 2)
                t.expect(codec.getLastDiagnostic().code == (mode == 2 ? "callback-exception"
                    : mode == 3 ? "commit-failure" : "unresolved-block-owner"),
                         "cleanup preserves the exception/publication first diagnostic");
            adapter.dxfW = nullptr;
        }
        dx_data pristine;
        populate(pristine);
        AttemptInterface fresh(pristine);
        fresh.bad.m_backgroundColorName.clear();
        dxfRW freshCodec(fileName.c_str());
        fresh.dxfW = &freshCodec;
        t.expect(freshCodec.write(&fresh, DRW::AC1027, binary), "pristine model fresh-writer control succeeds");
        rawSelfReference(binary);
        fresh.dxfW = nullptr;
    }
}
} // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--legacy-dimension-point-order") {
        TestContext context;
        testLegacyDimensionPointOrder(context);
        std::cout << "AC1009 DIMENSION point order: "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--wipeout-subclass-payload") {
        TestContext context; testWipeoutSubclassPayload(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 3 && (std::string(argv[1]) == "--generate-wipeout-payload"
                     || std::string(argv[1]) == "--generate-wipeout-payload-binary"))
        return generateWipeoutPayload(argv[2], std::string(argv[1]) == "--generate-wipeout-payload-binary") ? 0 : 1;
    if (argc == 3 && (std::string(argv[1]) == "--generate-wipeout-block-payload"
                     || std::string(argv[1]) == "--generate-wipeout-block-payload-binary"))
        return generateWipeoutBlockPayload(argv[2], std::string(argv[1]) == "--generate-wipeout-block-payload-binary") ? 0 : 1;
    if (argc == 3 && (std::string(argv[1]) == "--verify-wipeout-payload"
                     || std::string(argv[1]) == "--verify-wipeout-block-payload")) {
        std::ifstream stream(argv[2], std::ios::binary); char prefix[22]{}; stream.read(prefix, sizeof(prefix));
        const bool binary = std::string(prefix, sizeof(prefix)) == std::string("AutoCAD Binary DXF\r\n\x1a\0", 22);
        return (std::string(argv[1]) == "--verify-wipeout-block-payload"
            ? verifyWipeoutBlockPayload(argv[2], binary) : verifyWipeoutPayload(argv[2], binary)) ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--dictionary-key-admission") {
        TestContext context;
        testDictionaryKeyAdmission(context);
        std::cout << "Dictionary key admission: " << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--image-adjustment-admission") {
        TestContext context;
        testImageAdjustmentAdmission(context);
        std::cout << "IMAGE/WIPEOUT adjustment admission: " << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 3 && (std::string(argv[1]) == "--generate-image-adjustments"
                     || std::string(argv[1]) == "--generate-image-adjustments-binary")) {
        TestContext context;
        const std::filesystem::path retained(argv[2]);
        testImageAdjustmentAdmission(context, &retained,
            std::string(argv[1]) == "--generate-image-adjustments-binary");
        return context.failures == 0 ? 0 : 1;
    }
    // Invoked only as an owned bounded child by the focused locale check.
    // Never change CTYPE during the ordinary registered test invocation.
    if (argc == 3 && std::string(argv[1]) == "--ascii-folding-locale") {
        const char* selected = std::setlocale(LC_CTYPE, argv[2]);
        if (selected == nullptr) {
            std::cout << "CTYPE unavailable: " << argv[2] << '\n';
            return 77;
        }
        const std::string initial(selected);
        TestContext context;
        testAsciiOnlySymbolCasing(context);
        context.expect(initial == std::setlocale(LC_CTYPE, nullptr),
                       "drawing operations leave the selected process CTYPE unchanged");
        std::cout << "CTYPE " << initial << ": "
                  << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 3 && (std::string(argv[1]) == "--generate-leader-advisory-counts"
                     || std::string(argv[1]) == "--generate-leader-advisory-counts-binary"))
        return generateLeaderAdvisoryCounts(argv[2], std::string(argv[1]) == "--generate-leader-advisory-counts-binary") ? 0 : 1;
    if (argc == 3 && std::string(argv[1]) == "--verify-leader-advisory-counts")
        return verifyLeaderAdvisoryCounts(argv[2]) ? 0 : 1;
    if (argc == 5 && std::string(argv[1]) == "--generate-xrecord-coordinates") {
        const std::string inputMode(argv[3]), outputMode(argv[4]);
        if ((inputMode != "ascii" && inputMode != "binary")
            || (outputMode != "ascii" && outputMode != "binary"))
            return 2;
        return generateXRecordCoordinateDxf(argv[2], inputMode == "binary",
                                             outputMode == "binary") ? 0 : 1;
    }
    if (argc == 3 && (std::string(argv[1]) == "--generate-vertex-presence"
                     || std::string(argv[1]) == "--generate-vertex-presence-binary"
                     || std::string(argv[1]) == "--generate-vertex-presence-negative"
                     || std::string(argv[1]) == "--generate-vertex-presence-negative-binary")) {
        const std::string mode(argv[1]);
        return generateClassicVertexPresence(argv[2], mode.find("binary") != std::string::npos, 8,
                   mode.find("negative") != std::string::npos ? 1 : -1, 20) ? 0 : 1;
    }
    if (argc == 3 && std::string(argv[1]) == "--verify-vertex-presence")
        return verifyClassicVertexPresence(argv[2]) ? 0 : 1;
    if (argc == 3 && (std::string(argv[1]) == "--generate-classic-vertex-ids"
                     || std::string(argv[1]) == "--generate-classic-vertex-ids-binary"))
        return generateClassicVertexIdentifiers(argv[2], std::string(argv[1]) == "--generate-classic-vertex-ids-binary") ? 0 : 1;
    if (argc == 3 && std::string(argv[1]) == "--verify-classic-vertex-ids")
        return verifyClassicVertexIdentifiers(argv[2]) ? 0 : 1;
    if (argc == 3 && (std::string(argv[1]) == "--generate-mtext-orientation"
                     || std::string(argv[1]) == "--generate-mtext-orientation-binary"))
        return generateMTextOrientationDxf(argv[2], std::string(argv[1]) == "--generate-mtext-orientation-binary") ? 0 : 1;
    if (argc == 3 && std::string(argv[1]) == "--verify-mtext-orientation")
        return verifyMTextOrientationDxf(argv[2]) ? 0 : 1;
    if (argc == 3) {
        const std::string mode(argv[1]);
        if (mode == "--generate-ellipse-carriers" || mode == "--generate-ellipse-carriers-binary")
            return generateLegacyEllipseCarriers(argv[2], mode == "--generate-ellipse-carriers-binary") ? 0 : 1;
        if (mode == "--verify-ellipse-carriers")
            return verifyLegacyEllipseCarriers(argv[2]) ? 0 : 1;
    }
    if (argc == 4 && std::string(argv[1]) == "--generate-xdata-points") {
        const std::string encoding(argv[3]);
        if (encoding != "ascii" && encoding != "binary")
            return 2;
        return generatePointXdataDxf(argv[2], encoding == "binary") ? 0 : 1;
    }
    if (argc == 5 && (std::string(argv[1]) == "--generate-table-owners"
                     || std::string(argv[1]) == "--generate-common-scopes"
                     || std::string(argv[1]) == "--generate-object-common-graphs"
                     || std::string(argv[1]) == "--generate-generated-common-carriers")) {
        const std::map<std::string, DRW::Version> versions{
            {"R14", DRW::AC1014}, {"R2000", DRW::AC1015},
            {"R2007", DRW::AC1021}, {"R2018", DRW::AC1032}};
        const auto version = versions.find(argv[3]);
        const std::string encoding = argv[4];
        if (version == versions.end()
            || (encoding != "ascii" && encoding != "binary"))
            return 2;
        if (std::string(argv[1]) == "--generate-table-owners")
            return generateTableOwnerDxf(argv[2], version->second,
                                         encoding == "binary") ? 0 : 1;
        if (std::string(argv[1]) == "--generate-object-common-graphs")
            return generateTypedObjectCommonGraphs(argv[2], version->second,
                                                     encoding == "binary") ? 0 : 1;
        if (std::string(argv[1]) == "--generate-generated-common-carriers")
            return generateGeneratedObjectCommonCarriers(argv[2], version->second,
                                                        encoding == "binary") ? 0 : 1;
        TestContext context;
        const std::filesystem::path output(argv[2]);
        testLinetypeExtensionGraphRoundTrip(context, &output, version->second,
                                           encoding == "binary");
        return context.failures == 0 ? 0 : 1;
    }
    if (argc != 1)
        return 2;
    TestContext context;
    testDictionaryKeyAdmission(context);
    testImageAdjustmentAdmission(context);
    testWipeoutSubclassPayload(context);
    testAsciiOnlySymbolCasing(context);
    testDxfConsumerFinalization(context);
    testInfiniteLineDirections(context);
    testSortEntsKeyRoles(context);
    testSortEntsCarrierAgreement(context);
    testPlotCustomDpiAdmission(context);
    testLeaderAdvisoryCounts(context);
    testDxfWriteAttemptFailure(context);
    testClassicVertexXdata(context);
    testClassicVertexPresenceBoundaries(context);
    testClassicVertexIdentifierRetention(context);
    testMTextOrientationPrecedence(context);
    testInsertChildHandleRollback(context);
    testLegacyEllipseCarrierTransfer(context);
    testTableRecordOwners(context);
    testBig5(context);
    testBig5Hkscs(context);
    testUhc(context);
    testBlockPreview(context);
    testBlockRecordXdata(context);
    testOrderedPointXdata(context);
    testXdataTextCodecs(context);
    testNamedColorCodecs(context);
    testRawXdataTextBytes(context);
    testLegacyDimensionPointOrder(context);
    testHiddenZeroHeightAttrib(context);
    testMeshRawFieldTags(context);
    testDynamicTableCapacityBasics(context);
    testHeaderMetadataOrder(context);
    testHeaderMetadataBoundaries(context);
    testHeaderAliasProvenance(context);
    testAdapterAcceptedDxfVersion(context);
    testConsumerSourceVersionSnapshot(context);
    testExplicitReservationQuery(context);
    testDxfReferenceResolver(context);
    testDxfWriteBlockRemapPhase(context);
    testDxfWriteLeftOutReporting(context);
    testDxfTextEncodingCodecs(context);
    testDxfTextEncodingReads(context);
    testDxfTextEncodingLifecycle(context);
    testDxfTextEncodingExportIsolation(context);
    testXdataTextWritePolicy(context);
    testRawClassEntity(context);
    testEed(context);
    testRawControls(context);
    testDictionaryWithoutOwner(context);
    testXRecordDataOrder(context);
    testXRecordCoordinateTuples(context);
    testXRecordCoordinateBoundaries(context);
    testXRecordVersionedHeaderAndPayload(context);
    testLinetypeExtensionGraphRoundTrip(context);
    testCanonicalLinetypeCommonScope(context);
    testTypedObjectCommonGraphs(context);
    testGeneratedObjectCommonCarriers(context);
    testIncompleteLinetypeExtensionGraphRejected(context);
    testUnsafeLinetypeExtensionGraphsRejected(context);
    testMTextBackgroundFillAndDefinedHeight(context);
    testMTextDefinedHeightPresence(context);
    testMTextWithoutBackgroundFillIsUnchanged(context);
    testMTextBackgroundModesWriteOnlyTheirOwnGroups(context);
    testMTextBlackBackgroundColourSurvives(context);
    testMTextRejectsAnUnsafeBackgroundColourName(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " DXF fixture assertion(s) failed\n";
        return 1;
    }
    std::cout << "DXF target fixtures: PASS\n";
    return 0;
}
