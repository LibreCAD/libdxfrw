#include <cstdint>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#define private public
#include "drw_header.h"
#undef private
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

class FixtureInterface final : public dx_iface {
public:
    std::size_t headerCount {0};
    std::string sourceCodePage;
    bool sawRText {false};
    bool sawArcAlignedText {false};
    bool sawMPolygon {false};
    bool sawLargeRadial {false};
    std::string rtext;
    std::string arcAlignedText;
    double arcRadius {0.0};
    int mpolygonSolid {0};
    int mpolygonFillAci {0};
    DRW_Coord largeRadialJog;
    DRW_Coord largeRadialCenter;
    DRW_Coord largeRadialChord;

    void addHeader(const DRW_Header* data) override {
        ++headerCount;
        sourceCodePage.clear();
        if (data != nullptr) {
            DRW_Header copy(*data);
            copy.getStr("$DWGCODEPAGE", &sourceCodePage);
        }
        dx_iface::addHeader(data);
    }

    void addText(const DRW_Text& data) override {
        if (const auto* value = dynamic_cast<const DRW_RText*>(&data)) {
            sawRText = true;
            rtext = value->text;
        }
        if (const auto* value = dynamic_cast<const DRW_ArcAlignedText*>(&data)) {
            sawArcAlignedText = true;
            arcAlignedText = value->text;
            arcRadius = value->m_radius;
        }
        dx_iface::addText(data);
    }

    void addHatch(const DRW_Hatch* data) override {
        if (const auto* value = dynamic_cast<const DRW_MPolygon*>(data)) {
            sawMPolygon = true;
            mpolygonSolid = value->solid;
            mpolygonFillAci = value->fillColorAci;
        }
        dx_iface::addHatch(data);
    }

    void addDimRadial(const DRW_DimRadial* data) override {
        if (const auto* value = dynamic_cast<const DRW_DimLargeRadial*>(data)) {
            sawLargeRadial = true;
            largeRadialJog = value->jogPoint;
            largeRadialCenter = value->getCenterPoint();
            largeRadialChord = value->getChordPoint();
        }
        dx_iface::addDimRadial(data);
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
                          bool extended, const char* expectedBookName,
                          const char* expectedCodePage) {
    FixtureInterface interface_;
    dx_data data;
    const std::filesystem::path path = fixturePath(name);
    t.expect(std::filesystem::is_regular_file(path), "DWG fixture exists");
    if (!std::filesystem::is_regular_file(path))
        return;
    t.expect(interface_.fileImport(path.string(), &data, false),
             "ordinary ENC DWG imports through dx_iface");
    t.expect(interface_.headerCount == 1u,
             "ordinary ENC DWG publishes exactly one header callback");
    t.expect(interface_.sourceCodePage == expectedCodePage,
             "ordinary ENC DWG propagates its source code page");
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

void testAdvancedTargetFixtures(TestContext& t) {
    {
        FixtureInterface interface_;
        dx_data data;
        t.expect(interface_.fileImport(
                      fixturePath("rtext_arctext.dwg").string(), &data, false),
                 "RTEXT/ARCALIGNEDTEXT fixture imports through dx_iface");
        t.expect(interface_.sawRText && interface_.rtext == "RTEXT-DIESEL-TEST",
                 "RTEXT callback preserves the DIESEL text payload");
        t.expect(interface_.sawArcAlignedText
                     && interface_.arcAlignedText == "ARC-TEXT-TEST"
                     && std::fabs(interface_.arcRadius - 25.0) < 1e-12,
                 "ARCALIGNEDTEXT callback preserves text and radius");
    }

    {
        FixtureInterface interface_;
        dx_data data;
        t.expect(interface_.fileImport(
                      fixturePath("mpolygon_solid.dwg").string(), &data, false),
                 "MPOLYGON fixture imports through dx_iface");
        t.expect(interface_.sawMPolygon && interface_.mpolygonSolid == 1
                     && interface_.mpolygonFillAci == 256,
                 "MPOLYGON callback preserves solid/fill metadata");
    }

    {
        FixtureInterface interface_;
        dx_data data;
        t.expect(interface_.fileImport(
                      fixturePath("large_radial.dwg").string(), &data, false),
                 "large radial dimension fixture imports through dx_iface");
        t.expect(interface_.sawLargeRadial
                     && std::fabs(interface_.largeRadialJog.x - 8.0) < 1e-12
                     && std::fabs(interface_.largeRadialJog.y - 2.0) < 1e-12
                     && std::fabs(interface_.largeRadialCenter.x - 5.0) < 1e-12
                     && std::fabs(interface_.largeRadialCenter.y - 6.0) < 1e-12
                     && std::fabs(interface_.largeRadialChord.x - 10.0) < 1e-12
                     && std::fabs(interface_.largeRadialChord.y) < 1e-12,
                 "large radial callback preserves jog/center/chord points");
    }
}

void testTruncatedTargetFixtures(TestContext& t) {
    const char* names[] = {
        "ordinary_enc_AC1021.dwg",
        "ordinary_enc_AC1027.dwg",
        "rtext_arctext.dwg",
        "mpolygon_solid.dwg",
        "large_radial.dwg"
    };
    for (const char* name : names) {
        const std::filesystem::path source = fixturePath(name);
        std::ifstream input(source, std::ios::binary);
        const std::vector<char> sourceBytes(
            (std::istreambuf_iterator<char>(input)),
            std::istreambuf_iterator<char>());
        t.expect(sourceBytes.size() > 128u,
                 "target DWG fixture is large enough for truncation");
        if (sourceBytes.size() <= 128u)
            continue;

        const std::filesystem::path truncated =
            std::filesystem::temp_directory_path()
            / (std::string("libdxfrw-s247-truncated-") + name);
        std::error_code ec;
        std::filesystem::remove(truncated, ec);
        {
            std::ofstream output(truncated, std::ios::binary);
            output.write(sourceBytes.data(),
                         static_cast<std::streamsize>(sourceBytes.size() / 2u));
        }
        FixtureInterface interface_;
        dx_data data;
        t.expect(!interface_.fileImport(truncated.string(), &data, false),
                 "truncated target DWG is rejected");
        t.expect(data.mBlock == nullptr || data.mBlock->ent.empty(),
                 "truncated target DWG publishes no partial entities");
        std::filesystem::remove(truncated, ec);
    }
}

} // namespace

int main() {
    TestContext context;
    testOrdinaryEncoding(context, "ordinary_enc_AC1015.dwg", false, "",
                         "ANSI_1252");
    testOrdinaryEncoding(context, "ordinary_enc_AC1018.dwg", true,
                         "Book$Entry", "ANSI_1252");
    testOrdinaryEncoding(context, "ordinary_enc_AC1021.dwg", true,
                         "Book$Entry", "ANSI_1252");
    testOrdinaryEncoding(context, "ordinary_enc_AC1027.dwg", true,
                         "Book$Entry", "ANSI_1252");
    testOrdinaryEncoding(context, "ordinary_enc_ac1027_ansi932.dwg", true,
                         "Book$Ａ", "ANSI_932");
    testAdvancedTargetFixtures(context);
    testTruncatedTargetFixtures(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " DWG fixture assertion(s) failed\n";
        return 1;
    }
    std::cout << "DWG target ENC fixtures: PASS\n";
    return 0;
}
