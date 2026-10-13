#include <cstddef>
#include <algorithm>
#include <array>
#include <charconv>
#include <chrono>
#include <memory>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "drw_entities.h"
#include "drw_objects.h"
#include "dwg2dxf/dx_data.h"
#include "dwg2dxf/dx_iface.h"
#include "intern/dxfreader.h"

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
                || code == 100 || code == 102 || code == 330 || code == 360
                || code == 1000 || code == 1001)
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
} // namespace

int main(int argc, char** argv) {
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
    testTableRecordOwners(context);
    testBig5(context);
    testBig5Hkscs(context);
    testUhc(context);
    testBlockPreview(context);
    testBlockRecordXdata(context);
    testRawClassEntity(context);
    testEed(context);
    testRawControls(context);
    testDictionaryWithoutOwner(context);
    testXRecordDataOrder(context);
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
