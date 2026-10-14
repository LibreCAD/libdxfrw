#include <cstddef>
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <cmath>
#include <memory>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <limits>
#include <map>
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
                           std::vector<TableOwnerRecord>& records) {
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
    int code = 0;
    while (reader->readRec(&code)) {
        if (code == 0) {
            const std::string kind = reader->getString();
            if (kind == "EOF")
                return true;
            records.push_back({kind, {}});
        } else if (!records.empty()) {
            // Only these string tags are relevant to table graph identity.
            if (code == 1 || code == 2 || code == 5 || code == 105
                || code == 100 || code == 102 || code == 330 || code == 340 || code == 360
                || code == 1000 || code == 1001 || code == 1005)
                records.back().groups.emplace_back(code, reader->getString());
        }
    }
    return false;
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
                    bool binary) {
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
