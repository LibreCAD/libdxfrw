#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "drw_header.h"
#include "drw_entities.h"
#include "intern/dwg_dxf_output_transaction.h"
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
            const auto it = data->vars.find("$DWGCODEPAGE");
            if (it != data->vars.end() && it->second != nullptr
                    && it->second->type() == DRW_Variant::STRING) {
                sourceCodePage = it->second->c_str();
            }
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
        dx_iface::addHatch(data);
    }

    void addMPolygon(const DRW_MPolygon* data) override {
        if (data != nullptr) {
            sawMPolygon = true;
            mpolygonSolid = data->solid;
            mpolygonFillAci = data->fillColorAci;
        }
        dx_iface::addMPolygon(data);
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

std::filesystem::path temporaryDxfPath(const char* label) {
    const auto nonce = std::chrono::steady_clock::now()
                           .time_since_epoch().count();
    return std::filesystem::temp_directory_path()
           / (std::string("libdxfrw-freecad-") + label + "-"
              + std::to_string(nonce) + ".dxf");
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

bool hasControlRoleDiagnostic(const dx_data& data, const char* controlName) {
    if (controlName == nullptr)
        return false;
    const std::string expected(controlName);
    const DwgReadIntegrityObservations& integrity =
        data.dwgReadOutcome.observations.integrity;
    for (std::size_t index = 0;
         index < std::min(integrity.retained, integrity.rows.size()); ++index) {
        const DwgReadIntegrityObservation& row = integrity.rows[index];
        if (row.kind != DwgIntegrityCheckKind::TableControlHandleRole
            || row.sectionName.length != expected.size())
            continue;
        const std::string observed(
            reinterpret_cast<const char*>(row.sectionName.bytes.data()),
            row.sectionName.length);
        if (observed == expected)
            return true;
    }
    return false;
}

bool hasRoleMismatch(const dx_data& data, DwgIntegrityCheckKind kind,
                     std::uint64_t expected, std::uint64_t observed) {
    const auto& integrity = data.dwgReadOutcome.observations.integrity;
    for (std::size_t index = 0;
         index < std::min(integrity.retained, integrity.rows.size()); ++index) {
        const auto& row = integrity.rows[index];
        if (row.kind == kind && row.severity == DwgIntegritySeverity::Error
            && row.hasExpected && row.hasObserved
            && row.expected == expected && row.observed == observed)
            return true;
    }
    return false;
}

bool hasHistoricalAc1021BlockBodyRoles(const dx_data& data) {
    const DRW_Block_Record* source = nullptr;
    for (const DRW_Block_Record& record : data.blockRecords) {
        if (record.handle != 0x17u)
            continue;
        if (source != nullptr)
            return false;
        source = &record;
    }
    if (source == nullptr)
        return false;
    std::size_t blockSoftPointers = 0u;
    std::size_t ownedEntitySoftPointers = 0u;
    bool legacyNullLayout = false;
    for (const DRW_DwgSubrecordRange& span : source->dwgHandleFieldSpans()) {
        if (span.m_name == "layout"
            && span.m_rawHandleCode == DRW::DwgHardOwnership
            && source->layoutHandle == 0u)
            legacyNullLayout = true;
        if (span.m_rawHandleCode == DRW::DwgSoftPointer
            && span.m_name == "block")
            ++blockSoftPointers;
        else if (span.m_rawHandleCode == DRW::DwgSoftPointer
                 && span.m_name == "ownedEntity")
            ++ownedEntitySoftPointers;
    }
    return blockSoftPointers == 1u && ownedEntitySoftPointers == 3u
        && legacyNullLayout;
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
    t.expect(hasRoleMismatch(data, DwgIntegrityCheckKind::LayerLineTypeHandleRole,
                             DRW::DwgHardPointer, DRW::DwgSoftPointer),
             "historical LAYER soft pointer remains an integrity error");
    t.expect(hasRoleMismatch(data, DwgIntegrityCheckKind::BlockRecordBodyHandleRole,
                             DRW::DwgHardOwnership, DRW::DwgSoftPointer),
             "historical BLOCK_HEADER soft pointer remains an integrity error");
    for (bool binary : {false, true}) {
        const auto output = temporaryDxfPath(binary ? "enc-strict-binary" : "enc-strict-ascii");
        { std::ofstream seed(output, std::ios::binary); seed << "sentinel"; }
        dx_iface writer;
        const bool accepted = writer.fileExport(output.string(), data.sourceVersion,
            binary, &data, false, true, dx_dxfSourceTextPolicy::Compatible,
            DRW::DxfTextWritePolicy::Compatible, dx_dxfProxyGraphicsPolicy::Compatible,
            dx_dwgReadCompletenessPolicy::RequireComplete);
        std::ifstream saved(output, std::ios::binary);
        const std::string contents((std::istreambuf_iterator<char>(saved)), {});
        t.expect(!accepted && contents == "sentinel",
                 "historical integrity errors refuse strict output atomically");
        saved.close();
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
    }
    if (data.sourceVersion == DRW::AC1021 || data.sourceVersion == DRW::AC1027) {
        t.expect(hasControlRoleDiagnostic(data, "LTYPE_CONTROL"),
                 "historical ENC LTYPE member roles remain diagnostic");
        t.expect(hasControlRoleDiagnostic(data, "BLOCK_CONTROL"),
                 "historical ENC BLOCK member roles remain diagnostic");
    }
    if (data.sourceVersion == DRW::AC1021) {
        t.expect(hasHistoricalAc1021BlockBodyRoles(data),
                 "historical AC1021 BLOCK_HEADER spans retain soft pointers and a null hard-owned LAYOUT handle");
    }
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

        DRW_DimLargeRadial* sourceDimension = nullptr;
        std::size_t sourceDimensionCount = 0u;
        for (DRW_Entity* entity : data.mBlock->ent) {
            if (auto* candidate = dynamic_cast<DRW_DimLargeRadial*>(entity)) {
                sourceDimension = candidate;
                ++sourceDimensionCount;
            }
        }
        t.expect(sourceDimensionCount == 1u,
                 "large radial fixture has one typed source record");
        if (sourceDimensionCount == 1u) {
            const auto sameCoord = [](const DRW_Coord& left,
                                      const DRW_Coord& right) {
                return std::fabs(left.x - right.x) < 1e-12
                    && std::fabs(left.y - right.y) < 1e-12
                    && std::fabs(left.z - right.z) < 1e-12;
            };
            const auto entityTypes = [](const dx_ifaceBlock* block) {
                std::vector<int> result;
                if (block != nullptr) {
                    result.reserve(block->ent.size());
                    for (const DRW_Entity* entity : block->ent)
                        result.push_back(entity == nullptr
                            ? -1 : static_cast<int>(entity->eType));
                }
                return result;
            };
            dx_ifaceBlock* sourcePictureBlock = nullptr;
            std::size_t sourcePictureBlockCount = 0u;
            for (dx_ifaceBlock* block : data.blocks) {
                if (block != nullptr
                    && block->name == sourceDimension->getName()) {
                    sourcePictureBlock = block;
                    ++sourcePictureBlockCount;
                }
            }
            const DRW_Block_Record* sourcePictureBlockRecord = nullptr;
            std::size_t sourcePictureBlockRecordCount = 0u;
            for (const DRW_Block_Record& record : data.blockRecords) {
                if (record.name == sourceDimension->getName()) {
                    sourcePictureBlockRecord = &record;
                    ++sourcePictureBlockRecordCount;
                }
            }
            t.expect(!sourceDimension->getName().empty()
                         && sourcePictureBlockCount == 1u
                         && sourcePictureBlockRecordCount == 1u
                         && sourcePictureBlock != nullptr
                         && !sourcePictureBlock->ent.empty(),
                     "large radial source picture-block handle resolves to one typed block graph");
            for (const bool binary : {false, true}) {
                dx_data isolated;
                isolated.headerC = data.headerC;
                isolated.sourceVersion = data.sourceVersion;
                isolated.lineTypes = data.lineTypes;
                isolated.layers = data.layers;
                isolated.dimStyles = data.dimStyles;
                isolated.textStyles = data.textStyles;
                isolated.appIds = data.appIds;
                if (sourcePictureBlock != nullptr
                    && sourcePictureBlockRecord != nullptr) {
                    // Borrow the fixture-owned block for this synchronous
                    // projection. The list node is removed before isolated's
                    // destructor; data remains the sole block owner.
                    isolated.blocks.push_back(sourcePictureBlock);
                    isolated.blockRecords.push_back(*sourcePictureBlockRecord);
                }
                auto* isolatedDimension =
                    new DRW_DimLargeRadial(*sourceDimension);
                isolated.mBlock->ent.push_back(isolatedDimension);

                const std::filesystem::path output = temporaryDxfPath(
                    binary ? "large-radial-binary"
                           : "large-radial-ascii");
                dx_iface exporter;
                // This projection omits the original plot-style and material
                // object graphs. Prove that the unmodified references are
                // refused atomically before testing the isolated geometry.
                bool hasDetachedReference = false;
                for (const DRW_Layer& layer : isolated.layers) {
                    std::uint32_t plot = 0u, material = 0u;
                    bool plotPresent = false, materialPresent = false;
                    if (layer.getDxfPlotStyleHandle(plot, plotPresent)
                        && layer.getDxfMaterialHandle(material, materialPresent)
                        && ((plotPresent && plot != 0u)
                            || (materialPresent && material != 0u)))
                        hasDetachedReference = true;
                }
                { std::ofstream saved(output, std::ios::binary); saved << "sentinel"; }
                const bool rejected = !exporter.fileExport(
                    output.string(), isolated.sourceVersion, binary,
                    &isolated, false);
                std::ifstream saved(output, std::ios::binary);
                const std::string contents((std::istreambuf_iterator<char>(saved)), {});
                saved.close();
                t.expect(hasDetachedReference && rejected && contents == "sentinel",
                         "large radial detached layer references refuse atomically");
                // The following test covers the dimension and picture-block
                // geometry only, with fresh default layer dependencies.
                for (DRW_Layer& layer : isolated.layers) {
                    layer.handlePlotS.clear();
                    layer.handleMaterialS.clear();
                    layer.plotStyleHandle = dwgHandle{};
                    layer.materialHandle = dwgHandle{};
                }
                const bool exported = exporter.fileExport(
                    output.string(), isolated.sourceVersion, binary,
                    &isolated, false);
                t.expect(exported,
                         binary
                             ? "large radial binary DXF record exports"
                             : "large radial ASCII DXF record exports");

                if (exported) {
                    FixtureInterface reader;
                    dx_data reopened;
                    const bool readBack = reader.fileImport(
                        output.string(), &reopened, false);
                    DRW_DimLargeRadial* actual = nullptr;
                    std::size_t actualCount = 0u;
                    const dx_ifaceBlock* actualPictureBlock = nullptr;
                    std::size_t actualPictureBlockCount = 0u;
                    if (readBack && reopened.mBlock != nullptr) {
                        for (DRW_Entity* entity : reopened.mBlock->ent) {
                            if (auto* candidate =
                                    dynamic_cast<DRW_DimLargeRadial*>(entity)) {
                                actual = candidate;
                                ++actualCount;
                            }
                        }
                    }
                    if (readBack) {
                        for (const dx_ifaceBlock* block : reopened.blocks) {
                            if (block != nullptr && actual != nullptr
                                && block->name == actual->getName()) {
                                actualPictureBlock = block;
                                ++actualPictureBlockCount;
                            }
                        }
                    }
                    const bool matches = actualCount == 1u
                        && actual != nullptr
                        && actual->getName() == sourceDimension->getName()
                        && actual->getText() == sourceDimension->getText()
                        && actual->getStyle() == sourceDimension->getStyle()
                        && actual->type == sourceDimension->type
                        && actual->getAlign() == sourceDimension->getAlign()
                        && actual->getTextLineStyle()
                            == sourceDimension->getTextLineStyle()
                        && actual->getTextLineFactor()
                            == sourceDimension->getTextLineFactor()
                        && actual->getDir() == sourceDimension->getDir()
                        && actual->getHDir() == sourceDimension->getHDir()
                        && actual->getFlipArrow1()
                            == sourceDimension->getFlipArrow1()
                        && actual->getFlipArrow2()
                            == sourceDimension->getFlipArrow2()
                        && actual->getMeasureValue()
                            == sourceDimension->getMeasureValue()
                        && sameCoord(actual->getExtrusion(),
                                     sourceDimension->getExtrusion())
                        && sameCoord(actual->getCenterPoint(),
                                     sourceDimension->getCenterPoint())
                        && sameCoord(actual->getTextPoint(),
                                     sourceDimension->getTextPoint())
                        && sameCoord(actual->getChordPoint(),
                                     sourceDimension->getChordPoint())
                        && sameCoord(actual->overrideCenterPoint,
                                     sourceDimension->overrideCenterPoint)
                        && sameCoord(actual->jogPoint,
                                     sourceDimension->jogPoint)
                        && actual->jogAngle == sourceDimension->jogAngle
                        && actualPictureBlockCount == 1u
                        && actualPictureBlock != nullptr
                        && entityTypes(actualPictureBlock)
                            == entityTypes(sourcePictureBlock);
                    t.expect(readBack && matches,
                             binary
                                 ? "large radial binary DXF readback preserves fields and picture-block graph"
                                 : "large radial ASCII DXF readback preserves fields and picture-block graph");
                }
                std::error_code ec;
                std::filesystem::remove(output, ec);
                isolated.blocks.clear();
            }
        }
    }
}

void testTypedEntitiesRemainAvailableWhenDxfReplayIsBlocked(TestContext& t) {
    {
        FixtureInterface sourceInterface;
        dx_data sourceData;
        t.expect(sourceInterface.fileImport(
                      fixturePath("rtext_arctext.dwg").string(), &sourceData,
                      false),
                 "RTEXT/ARCALIGNEDTEXT fixture imports");
        t.expect(sourceInterface.sawRText
                     && sourceInterface.rtext == "RTEXT-DIESEL-TEST"
                     && sourceInterface.sawArcAlignedText
                     && sourceInterface.arcAlignedText == "ARC-TEXT-TEST"
                     && std::fabs(sourceInterface.arcRadius - 25.0) < 1e-12,
                 "RTEXT/ARCALIGNEDTEXT typed values are retained on import");

        const std::filesystem::path output = temporaryDxfPath("rtext");
        const bool exported = sourceInterface.fileExport(
            output.string(), sourceData.sourceVersion, false, &sourceData,
            false);
        t.expect(!sourceData.visualStyles.empty()
                     && sourceData.rawVisualStyleObjects.empty()
                     && !exported && !std::filesystem::exists(output),
                 "RTEXT/ARCALIGNEDTEXT conversion refuses typed-only VISUALSTYLE output atomically");
        std::error_code ec;
        std::filesystem::remove(output, ec);
    }

    {
        FixtureInterface sourceInterface;
        dx_data sourceData;
        t.expect(sourceInterface.fileImport(
                      fixturePath("mpolygon_solid.dwg").string(), &sourceData,
                      false),
                 "MPOLYGON fixture imports");
        t.expect(sourceInterface.sawMPolygon
                     && sourceInterface.mpolygonSolid == 1
                     && sourceInterface.mpolygonFillAci == 256,
                 "MPOLYGON typed values are retained on import");

        const std::filesystem::path output = temporaryDxfPath("mpolygon");
        const bool exported = sourceInterface.fileExport(
            output.string(), sourceData.sourceVersion, false, &sourceData,
            false);
        t.expect(!sourceData.visualStyles.empty()
                     && sourceData.rawVisualStyleObjects.empty()
                     && !exported && !std::filesystem::exists(output),
                 "MPOLYGON conversion refuses typed-only VISUALSTYLE output atomically");
        std::error_code ec;
        std::filesystem::remove(output, ec);
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

void testUtf8FilesystemPaths(TestContext& t) {
    namespace fs = std::filesystem;
    const int before = t.failures;
    const auto root = fs::temp_directory_path() / fs::u8path(
        std::string(u8"libdxfrw paths \u03a9 \u4e2d \U0001f4d0 ")
        + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::error_code error;
    const bool created = fs::create_directory(root, error);
    t.expect(created && !error, "create a fresh native Unicode parent directory");
    if (!created || error)
        return;
    struct Cleanup {
        fs::path root;
        ~Cleanup() { std::error_code ignored; fs::remove_all(root, ignored); }
    } cleanup {root};

    const auto dwg = root / fs::u8path(u8"drawing \u03a9 \u4e2d \U0001f4d0.dwg");
    fs::copy_file(fixturePath("ordinary_enc_AC1015.dwg"), dwg, error);
    t.expect(!error, "copy the admitted DWG to its exact native Unicode path");
    if (error)
        return;
    FixtureInterface source;
    dx_data data;
    const bool imported = source.fileImport(dwg.u8string(), &data, false);
    t.expect(imported && linesIn(data).size() == 3u,
             "UTF-8 DWG input reads all three lines from the native filename");
    if (imported) {
        // Export through a fresh, empty drawing. The imported AC1015 sample
        // carries an extension dictionary the writer correctly refuses as
        // unclosed; this case isolates native UTF-8 pathname handling from
        // extension-object graph qualification.
        FixtureInterface writer;
        dx_data outputData;
        for (bool binary : {false, true}) {
            const auto dxf = root / fs::u8path(binary
                ? u8"binary \u03a9 \u4e2d \U0001f4d0.dxf"
                : u8"ascii \u03a9 \u4e2d \U0001f4d0.dxf");
            const bool written = writer.fileExport(
                dxf.u8string(), DRW::AC1015, binary, &outputData, false);
            t.expect(written && fs::is_regular_file(dxf),
                     "UTF-8 DXF output preserves the exact native filename");
            if (written) {
                FixtureInterface reader;
                dx_data restored;
                t.expect(reader.fileImport(dxf.u8string(), &restored, false)
                             && linesIn(restored).empty(),
                         "empty ASCII and binary DXF reopen through UTF-8 paths");
            }
        }
    }

    const auto target = root / fs::u8path(u8"atomic \u03a9 \u4e2d \U0001f4d0.bin");
    { std::ofstream seed(target, std::ios::binary); seed << "original"; }
    const auto readTarget = [&] {
        std::ifstream input(target, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input), {});
    };
    const auto temporaryCount = [&] {
        int count = 0;
        const auto prefix = target.filename().u8string() + ".libdxfrw-";
        for (const auto& entry : fs::directory_iterator(root))
            if (entry.path().filename().u8string().rfind(prefix, 0) == 0)
                ++count;
        return count;
    };
    {
        DwgDxfOutputTransaction transaction(target.u8string(), std::ios::binary);
        t.expect(transaction.open(), "Unicode transaction opens an exclusive temporary");
        t.expect(temporaryCount() == 1, "temporary keeps the exact native Unicode basename");
        transaction.stream() << "replacement";
        t.expect(readTarget() == "original", "Unicode output remains atomic before commit");
        t.expect(transaction.commit(), "Unicode transaction publishes successfully");
    }
    t.expect(readTarget() == "replacement" && temporaryCount() == 0,
             "Unicode commit preserves content and removes its temporary");
    {
        DwgDxfOutputTransaction transaction(target.u8string(), std::ios::binary);
        t.expect(transaction.open(), "Unicode rollback opens an exclusive temporary");
        transaction.stream() << "discarded";
    }
    t.expect(readTarget() == "replacement" && temporaryCount() == 0,
             "Unicode destructor rollback preserves destination and cleans up");
    for (const auto& entry : fs::directory_iterator(root))
        t.expect(entry.path().filename().u8string().find(".libdxfrw-") == std::string::npos,
                 "no wrongly encoded transaction temporary is left behind");
    if (t.failures == before)
        std::cout << "UTF-8 filesystem paths: PASS\n";
}

void testInactiveHatchMPolygonGradientStopsFailClosed(TestContext& t) {
    for (const bool mpolygon : {false, true}) {
        for (const bool binary : {false, true}) {
            dx_data data;
            auto* hatch = mpolygon
                ? static_cast<DRW_Hatch*>(new DRW_MPolygon())
                : new DRW_Hatch();
            hatch->solid = 1;
            hatch->isGradient = 0;
            hatch->name = "SOLID";
            auto loop = std::make_shared<DRW_HatchLoop>(2);
            auto polyline = std::make_shared<DRW_LWPolyline>();
            polyline->flags = 1;
            polyline->addVertex(DRW_Vertex2D(0.0, 0.0, 0.0));
            polyline->addVertex(DRW_Vertex2D(2.0, 0.0, 0.0));
            polyline->addVertex(DRW_Vertex2D(1.0, 2.0, 0.0));
            loop->objlist.push_back(polyline);
            loop->update();
            hatch->appendLoop(loop);
            data.mBlock->ent.push_back(hatch);

            const std::filesystem::path baseline = temporaryDxfPath(
                mpolygon
                    ? (binary ? "mpolygon-inactive-gradient-baseline-binary"
                              : "mpolygon-inactive-gradient-baseline-ascii")
                    : (binary ? "hatch-inactive-gradient-baseline-binary"
                              : "hatch-inactive-gradient-baseline-ascii"));
            FixtureInterface writer;
            const bool baselineExported = writer.fileExport(
                baseline.string(), DRW::AC1027, binary, &data, false);
            t.expect(baselineExported,
                     mpolygon
                         ? (binary
                                ? "ordinary binary MPOLYGON exports before the loss control"
                                : "ordinary ASCII MPOLYGON exports before the loss control")
                         : (binary
                                ? "ordinary binary HATCH exports before the loss control"
                                : "ordinary ASCII HATCH exports before the loss control"));
            if (!baselineExported)
                continue;

            FixtureInterface baselineReader;
            dx_data baselineData;
            const bool baselineRead = baselineReader.fileImport(
                baseline.string(), &baselineData, false);
            std::size_t baselineEntityCount = 0u;
            if (baselineRead && baselineData.mBlock != nullptr) {
                for (const DRW_Entity* entity : baselineData.mBlock->ent) {
                    if (mpolygon) {
                        if (dynamic_cast<const DRW_MPolygon*>(entity) != nullptr)
                            ++baselineEntityCount;
                    } else if (dynamic_cast<const DRW_Hatch*>(entity) != nullptr
                               && dynamic_cast<const DRW_MPolygon*>(entity) == nullptr) {
                        ++baselineEntityCount;
                    }
                }
            }
            t.expect(baselineRead && baselineEntityCount == 1u,
                     mpolygon
                         ? (binary
                                ? "ordinary binary MPOLYGON baseline has typed readback"
                                : "ordinary ASCII MPOLYGON baseline has typed readback")
                         : (binary
                                ? "ordinary binary HATCH baseline has typed readback"
                                : "ordinary ASCII HATCH baseline has typed readback"));

            hatch->gradAngle = 0.0;
            hatch->gradShift = 0.0;
            hatch->singleColor = 1;
            hatch->gradTint = 1.0;
            hatch->gradName = "LINEAR";
            hatch->gradColors.emplace_back(0.0, 0x0000FF, 0, 0xC2u);
            hatch->gradColors.emplace_back(1.0, 0xFFFFFF, 0, 0xC2u);
            const std::filesystem::path rejected = temporaryDxfPath(
                mpolygon
                    ? (binary ? "mpolygon-inactive-gradient-rejected-binary"
                              : "mpolygon-inactive-gradient-rejected-ascii")
                    : (binary ? "hatch-inactive-gradient-rejected-binary"
                              : "hatch-inactive-gradient-rejected-ascii"));
            const std::string sentinel = "inactive gradient stops must be retained\n";
            {
                std::ofstream output(rejected, std::ios::binary);
                t.expect(static_cast<bool>(output),
                         "create HATCH loss-guard destination sentinel");
                output << sentinel;
            }
            const bool rejectedLossyProjection = !writer.fileExport(
                rejected.string(), DRW::AC1027, binary, &data, false);
            std::ifstream rejectedInput(rejected, std::ios::binary);
            const std::string rejectedContents{
                std::istreambuf_iterator<char>{rejectedInput},
                std::istreambuf_iterator<char>{}};
            t.expect(rejectedLossyProjection && rejectedContents == sentinel,
                     mpolygon
                         ? (binary
                                ? "binary MPOLYGON refuses inactive stops atomically"
                                : "ASCII MPOLYGON refuses inactive stops atomically")
                         : (binary
                                ? "binary HATCH refuses inactive stops atomically"
                                : "ASCII HATCH refuses inactive stops atomically"));

            // A caller that explicitly resolves the retained, unrepresentable
            // source values can reuse the same adapter after the failed export.
            hatch->gradColors.clear();
            const std::filesystem::path retry = temporaryDxfPath(
                mpolygon
                    ? (binary ? "mpolygon-inactive-gradient-retry-binary"
                              : "mpolygon-inactive-gradient-retry-ascii")
                    : (binary ? "hatch-inactive-gradient-retry-binary"
                              : "hatch-inactive-gradient-retry-ascii"));
            const bool retried = writer.fileExport(
                retry.string(), DRW::AC1027, binary, &data, false);
            FixtureInterface retryReader;
            dx_data retryData;
            const bool retryRead = retried && retryReader.fileImport(
                retry.string(), &retryData, false);
            const DRW_Hatch* retryHatch = nullptr;
            std::size_t retryEntityCount = 0u;
            if (retryRead && retryData.mBlock != nullptr) {
                for (const DRW_Entity* entity : retryData.mBlock->ent) {
                    const auto* candidate =
                        dynamic_cast<const DRW_Hatch*>(entity);
                    const bool candidateKind = mpolygon
                        ? dynamic_cast<const DRW_MPolygon*>(entity) != nullptr
                        : candidate != nullptr
                            && dynamic_cast<const DRW_MPolygon*>(entity) == nullptr;
                    if (candidate != nullptr && candidateKind) {
                        retryHatch = candidate;
                        ++retryEntityCount;
                    }
                }
            }
            const bool retryMetadataMatches = retryHatch != nullptr
                && retryHatch->isGradient == 0
                && retryHatch->gradReserved == 0
                && retryHatch->gradAngle == 0.0
                && retryHatch->gradShift == 0.0
                && retryHatch->singleColor == 1
                && retryHatch->gradTint == 1.0
                && retryHatch->gradName == "LINEAR"
                && retryHatch->gradColors.empty();
            t.expect(retried && retryRead && retryEntityCount == 1u
                         && retryMetadataMatches,
                     mpolygon
                         ? (binary
                                ? "binary MPOLYGON retry preserves inactive gradient companions"
                                : "ASCII MPOLYGON retry preserves inactive gradient companions")
                         : (binary
                                ? "binary HATCH retry preserves inactive gradient companions"
                                : "ASCII HATCH retry preserves inactive gradient companions"));

            std::error_code ec;
            std::filesystem::remove(baseline, ec);
            std::filesystem::remove(rejected, ec);
            std::filesystem::remove(retry, ec);
        }
    }
}

int testPhysicalInactiveGradientHatch(const char* sourcePath) {
    constexpr std::uint32_t sourceHandle = 0x173u;
    FixtureInterface sourceReader;
    dx_data sourceData;
    if (!sourceReader.fileImport(sourcePath, &sourceData, false)
        || sourceData.mBlock == nullptr) {
        std::cerr << "FAIL: physical HATCH source imports\n";
        return 1;
    }

    const DRW_Hatch* sourceHatch = nullptr;
    for (const DRW_Entity* entity : sourceData.mBlock->ent) {
        const auto* candidate = dynamic_cast<const DRW_Hatch*>(entity);
        if (candidate != nullptr && candidate->handle == sourceHandle) {
            sourceHatch = candidate;
            break;
        }
    }
    const bool sourceProfileMatches = sourceHatch != nullptr
        && sourceHatch->isGradient == 0
        && sourceHatch->gradColors.size() == 2u
        && sourceHatch->gradColors[0].value == 0.0
        && sourceHatch->gradColors[0].colorMethod == 0xC2u
        && sourceHatch->gradColors[0].rgb == 0x0000FF
        && sourceHatch->gradColors[1].value == 1.0
        && sourceHatch->gradColors[1].colorMethod == 0xC2u
        && sourceHatch->gradColors[1].rgb == 0xFFFFFF
        && sourceHatch->gradReserved == 0
        && sourceHatch->gradAngle == 0.0
        && sourceHatch->gradShift == 0.0
        && sourceHatch->singleColor == 1
        && sourceHatch->gradTint == 1.0
        && sourceHatch->gradName == "LINEAR"
        && sourceHatch->looplist.size() == 2u;
    if (!sourceProfileMatches) {
        std::cerr << "FAIL: physical HATCH 0x173 inactive-gradient fields\n";
        return 1;
    }

    for (std::size_t loopIndex = 0; loopIndex < sourceHatch->looplist.size();
         ++loopIndex) {
        const auto& loop = sourceHatch->looplist[loopIndex];
        const auto* polyline = loop != nullptr && loop->objlist.size() == 1u
            ? dynamic_cast<const DRW_LWPolyline*>(loop->objlist.front().get())
            : nullptr;
        if (loop == nullptr || loop->type != 7 || polyline == nullptr
            || polyline->vertlist.size() != 2u
            || polyline->vertlist[0] == nullptr
            || polyline->vertlist[1] == nullptr
            || polyline->vertlist[0]->bulge != 1.0
            || polyline->vertlist[1]->bulge != 1.0) {
            std::cerr << "FAIL: physical HATCH 0x173 boundary fields\n";
            return 1;
        }
    }

    // The physical source contains stop records even though its gradient flag
    // is clear. Exercise the selected record as a standalone source-to-DXF
    // projection after the caller explicitly removes those unrepresentable
    // stops; never convert or claim completeness for the rest of this file.
    DRW_Hatch resolvedHatch(*sourceHatch);
    resolvedHatch.gradColors.clear();
    const auto outputMatchesSelectedFields = [&](const DRW_Hatch* output) {
        if (output == nullptr || output->isGradient != 0
            || output->gradReserved != sourceHatch->gradReserved
            || output->gradAngle != sourceHatch->gradAngle
            || output->gradShift != sourceHatch->gradShift
            || output->singleColor != sourceHatch->singleColor
            || output->gradTint != sourceHatch->gradTint
            || output->gradName != sourceHatch->gradName
            || !output->gradColors.empty()
            || output->looplist.size() != sourceHatch->looplist.size())
            return false;

        for (std::size_t loopIndex = 0;
             loopIndex < sourceHatch->looplist.size(); ++loopIndex) {
            const auto& sourceLoop = sourceHatch->looplist[loopIndex];
            const auto& outputLoop = output->looplist[loopIndex];
            if (!sourceLoop || !outputLoop
                || outputLoop->type != sourceLoop->type
                || outputLoop->objlist.size() != sourceLoop->objlist.size())
                return false;

            const auto* sourcePolyline = sourceLoop->objlist.size() == 1u
                ? dynamic_cast<const DRW_LWPolyline*>(
                      sourceLoop->objlist.front().get())
                : nullptr;
            const auto* outputPolyline = outputLoop->objlist.size() == 1u
                ? dynamic_cast<const DRW_LWPolyline*>(
                      outputLoop->objlist.front().get())
                : nullptr;
            if (sourcePolyline == nullptr || outputPolyline == nullptr
                || outputPolyline->flags != sourcePolyline->flags
                || outputPolyline->vertlist.size()
                    != sourcePolyline->vertlist.size())
                return false;
            for (std::size_t vertexIndex = 0;
                 vertexIndex < sourcePolyline->vertlist.size(); ++vertexIndex) {
                const auto& sourceVertex = sourcePolyline->vertlist[vertexIndex];
                const auto& outputVertex = outputPolyline->vertlist[vertexIndex];
                if (!sourceVertex || !outputVertex
                    || outputVertex->x != sourceVertex->x
                    || outputVertex->y != sourceVertex->y
                    || outputVertex->bulge != sourceVertex->bulge)
                    return false;
            }
        }
        return true;
    };

    for (const bool binary : {false, true}) {
        dx_data selectedData;
        selectedData.mBlock->ent.push_back(new DRW_Hatch(resolvedHatch));
        const std::filesystem::path outputPath = temporaryDxfPath(
            binary ? "physical-hatch-inactive-gradient-binary"
                   : "physical-hatch-inactive-gradient-ascii");
        FixtureInterface writer;
        const bool exported = writer.fileExport(
            outputPath.string(), DRW::AC1027, binary, &selectedData, false);
        FixtureInterface reader;
        dx_data outputData;
        const bool read = exported && reader.fileImport(
            outputPath.string(), &outputData, false);
        const DRW_Hatch* outputHatch = nullptr;
        std::size_t outputHatchCount = 0u;
        if (read && outputData.mBlock != nullptr) {
            for (const DRW_Entity* entity : outputData.mBlock->ent) {
                const auto* candidate = dynamic_cast<const DRW_Hatch*>(entity);
                if (candidate != nullptr
                    && dynamic_cast<const DRW_MPolygon*>(entity) == nullptr) {
                    outputHatch = candidate;
                    ++outputHatchCount;
                }
            }
        }
        const bool outputMatches = exported && read && outputHatchCount == 1u
            && outputMatchesSelectedFields(outputHatch);
        std::error_code ec;
        std::filesystem::remove(outputPath, ec);
        if (!outputMatches) {
            std::cerr << "FAIL: physical HATCH 0x173 selected-record "
                      << (binary ? "Binary" : "ASCII")
                      << " output/readback\n";
            return 1;
        }
    }
    std::cout << "Physical AC1018 HATCH 0x173 inactive-gradient profile: PASS\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--ordinary-enc-historical") {
        TestContext context;
        testOrdinaryEncoding(context, "ordinary_enc_AC1015.dwg", false, "", "ANSI_1252");
        testOrdinaryEncoding(context, "ordinary_enc_AC1018.dwg", true, "Book$Entry", "ANSI_1252");
        testOrdinaryEncoding(context, "ordinary_enc_AC1021.dwg", true, "Book$Entry", "ANSI_1252");
        testOrdinaryEncoding(context, "ordinary_enc_AC1027.dwg", true, "Book$Entry", "ANSI_1252");
        testOrdinaryEncoding(context, "ordinary_enc_ac1027_ansi932.dwg", true, "Book$Ａ", "ANSI_932");
        if (context.failures != 0) return 1;
        std::cout << "Historical ENC fixtures: PASS\n";
        return 0;
    }
    if (argc == 2
        && std::string(argv[1]) == "--ordinary-enc-ac1021-only") {
        TestContext context;
        testOrdinaryEncoding(context, "ordinary_enc_AC1021.dwg", true,
                             "Book$Entry", "ANSI_1252");
        if (context.failures != 0)
            return 1;
        std::cout << "Historical AC1021 ENC role profile: PASS\n";
        return 0;
    }
    if (argc == 2
        && std::string(argv[1]) == "--hatch-mpolygon-inactive-gradient-guard") {
        TestContext context;
        testInactiveHatchMPolygonGradientStopsFailClosed(context);
        if (context.failures != 0)
            return 1;
        std::cout << "Inactive HATCH/MPOLYGON gradient-stop controls: PASS\n";
        return 0;
    }
    if (argc == 3
        && std::string(argv[1]) == "--hatch-source-profile")
        return testPhysicalInactiveGradientHatch(argv[2]);
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
    testTypedEntitiesRemainAvailableWhenDxfReplayIsBlocked(context);
    testTruncatedTargetFixtures(context);
    testUtf8FilesystemPaths(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " DWG fixture assertion(s) failed\n";
        return 1;
    }
    std::cout << "DWG target ENC fixtures: PASS\n";
    return 0;
}
