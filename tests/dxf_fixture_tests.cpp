#include <cstddef>
#include <array>
#include <charconv>
#include <chrono>
#include <memory>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <string>
#include <utility>
#include <vector>

#include "drw_entities.h"
#include "drw_objects.h"
#include "dwg2dxf/dx_data.h"
#include "dwg2dxf/dx_iface.h"

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

bool hasGroup(const std::vector<std::pair<int, std::string>>& groups, int code) {
    for (const auto& group : groups) {
        if (group.first == code)
            return true;
    }
    return false;
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

void testLinetypeExtensionGraphRoundTrip(TestContext& t) {
    dx_data source;
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
        source.xRecords.push_back(record);
    }

    const auto nonce = std::chrono::steady_clock::now().time_since_epoch()
                           .count();
    const std::filesystem::path output =
        std::filesystem::temp_directory_path()
        / ("libdxfrw-ltype-extension-" + std::to_string(nonce) + ".dxf");
    struct OutputCleanup {
        std::filesystem::path path;
        ~OutputCleanup() {
            std::error_code error;
            std::filesystem::remove(path, error);
        }
    } cleanup{output};

    dx_iface writerInterface;
    t.expect(writerInterface.fileExport(output.string(), DRW::AC1014, false,
                                        &source, false),
             "R14 writer exports a closed linetype extension graph");
    if (!std::filesystem::exists(output))
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
        t.expect(record->m_dataEntries.size() == 3
                     && record->m_dataEntries[0].code() == 102
                     && record->m_dataEntries[0].c_str() != nullptr
                     && std::string(record->m_dataEntries[0].c_str()) == "EXTNAMES"
                     && record->m_dataEntries[1].c_str() != nullptr
                     && std::string(record->m_dataEntries[1].c_str()) == names[i]
                     && record->m_dataEntries[2].c_str() != nullptr
                     && std::string(record->m_dataEntries[2].c_str()) == names[i],
                 "ordered XRECORD extension names survive R14 readback");
        if (i == 0) {
            std::uint32_t xdataHandle = 0;
            bool xdataHandleMatchesRecord = false;
            if (dictionary->extData.size() == 1
                && dictionary->extData.front() != nullptr
                && dictionary->extData.front()->c_str() != nullptr) {
                const char* value = dictionary->extData.front()->c_str();
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

int main() {
    TestContext context;
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
    testLinetypeExtensionGraphRoundTrip(context);
    testIncompleteLinetypeExtensionGraphRejected(context);
    testUnsafeLinetypeExtensionGraphsRejected(context);
    testMTextBackgroundFillAndDefinedHeight(context);
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
