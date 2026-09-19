#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <string>
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
    if (context.failures != 0) {
        std::cerr << context.failures << " DXF fixture assertion(s) failed\n";
        return 1;
    }
    std::cout << "DXF target fixtures: PASS\n";
    return 0;
}
