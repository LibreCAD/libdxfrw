#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "drw_entities.h"
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

std::filesystem::path fixturePath(const char* name) {
#ifdef LIBDXFRW_DWG_FIXTURE_DIR
    return std::filesystem::path(LIBDXFRW_DWG_FIXTURE_DIR) / name;
#else
    return std::filesystem::path("tests/fixtures/dwg") / name;
#endif
}

std::vector<const DRW_Line*> linesIn(const dx_data& data) {
    std::vector<const DRW_Line*> lines;
    if (data.mBlock == nullptr)
        return lines;
    for (const DRW_Entity* entity : data.mBlock->ent) {
        if (entity != nullptr && entity->eType == DRW::LINE)
            lines.push_back(static_cast<const DRW_Line*>(entity));
    }
    return lines;
}

void testOrdinaryEncoding(TestContext& t, const char* name,
                          bool extended, const char* expectedBookName) {
    dx_iface interface_;
    dx_data data;
    const std::filesystem::path path = fixturePath(name);
    t.expect(std::filesystem::is_regular_file(path), "DWG fixture exists");
    if (!std::filesystem::is_regular_file(path))
        return;
    t.expect(interface_.fileImport(path.string(), &data, false),
             "ordinary ENC DWG imports through dx_iface");
    const std::vector<const DRW_Line*> lines = linesIn(data);
    t.expect(lines.size() == 3u,
             "ordinary ENC DWG publishes the three LINE records");
    if (lines.size() != 3u)
        return;

    if (!extended) {
        t.expect(lines[0]->color24 == -1,
                 "AC1015 ordinary ENC keeps legacy color24 sentinel");
        t.expect(lines[1]->colorName.empty()
                     && !lines[1]->hasDwgAcDbColorHandle(),
                 "AC1015 ordinary ENC omits modern color side channel");
        t.expect(lines[2]->colorName.empty(),
                 "AC1015 ordinary ENC omits linked color name");
        return;
    }

    t.expect(lines[0]->color24 == 0x112233
                 && lines[0]->transparency == 0x03000080,
             "modern ordinary ENC preserves true color and transparency");
    t.expect(lines[1]->colorName == expectedBookName
                 && lines[1]->hasDwgAcDbColorHandle()
                 && lines[1]->dwgAcDbColorHandle() == 0x35u,
             "modern ordinary ENC preserves color-book handle");
    t.expect(lines[2]->colorName == "Book$Linked"
                 && lines[2]->reactorHandles
                        == std::vector<std::uint32_t>{0x36u, 0x37u}
                 && lines[2]->xDictHandle == 0x38u,
             "modern ordinary ENC preserves linked color graph");
}

} // namespace

int main() {
    TestContext context;
    testOrdinaryEncoding(context, "ordinary_enc_AC1015.dwg", false, "");
    testOrdinaryEncoding(context, "ordinary_enc_AC1018.dwg", true,
                         "Book$Entry");
    testOrdinaryEncoding(context, "ordinary_enc_AC1021.dwg", true,
                         "Book$Entry");
    testOrdinaryEncoding(context, "ordinary_enc_AC1027.dwg", true,
                         "Book$Entry");
    testOrdinaryEncoding(context, "ordinary_enc_ac1027_ansi932.dwg", true,
                         "Book$Ａ");
    if (context.failures != 0) {
        std::cerr << context.failures << " DWG fixture assertion(s) failed\n";
        return 1;
    }
    std::cout << "DWG target ENC fixtures: PASS\n";
    return 0;
}
