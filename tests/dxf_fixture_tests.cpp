#include <cstddef>
#include <filesystem>
#include <iostream>
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

} // namespace

int main() {
    TestContext context;
    testBig5(context);
    testBig5Hkscs(context);
    testUhc(context);
    testBlockPreview(context);
    testRawClassEntity(context);
    testEed(context);
    testRawControls(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " DXF fixture assertion(s) failed\n";
        return 1;
    }
    std::cout << "DXF target fixtures: PASS\n";
    return 0;
}
