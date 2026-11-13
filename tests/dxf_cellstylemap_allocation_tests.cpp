// SPDX-License-Identifier: GPL-2.0-or-later
// Generated CELLSTYLEMAP DXF writer allocation rollback/retry control.
#include "dx_iface.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>
#include <utility>

namespace {
constexpr std::uint32_t kTextStyleHandle = 0xE511u;
long allocationsBeforeFailure = -1;
std::size_t allocationAttempts = 0u;
bool allocationFailed = false;
bool allocationTracking = false;

bool failAllocation() noexcept {
    if (!allocationTracking || allocationsBeforeFailure < 0)
        return false;
    ++allocationAttempts;
    if (allocationsBeforeFailure == 0) {
        allocationsBeforeFailure = -1;
        allocationFailed = true;
        return true;
    }
    --allocationsBeforeFailure;
    return false;
}

void armAllocationFailure(long offset) noexcept {
    allocationsBeforeFailure = offset;
    allocationAttempts = 0u;
    allocationFailed = false;
    allocationTracking = true;
}

void disarmAllocationFailure() noexcept {
    allocationTracking = false;
    allocationsBeforeFailure = -1;
}

DRW_CellStyleMap makeCellStyleMap(int styleId) {
    DRW_CellStyleMap map;
    map.handle = 0xE510u;
    map.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
    DRW_TableStyleCellStyle style;
    style.m_type = 5;
    style.m_hasData = true;
    style.m_id = styleId;
    style.m_styleClass = 2;
    style.m_name = styleId == 4 ? "GENERATED_CELLSTYLEMAP_TABLE"
                                : "GENERATED_CELLSTYLEMAP_DATA";
    style.m_contentFormat.m_cellAlignment = 1;
    style.m_contentFormat.m_textHeight = 1.75;
    style.m_contentFormat.m_textStyleHandle = kTextStyleHandle;
    DRW_TableStyleBorder border;
    border.m_edgeFlags = 1;
    border.m_lineWeight = 25;
    border.m_visible = 1;
    border.m_color = 3;
    style.m_borders.push_back(border);
    map.m_cellStyles.push_back(std::move(style));
    return map;
}

class Export final : public dx_iface {
public:
    Export(dxfRW& codec, long failAt, int styleId)
        : codec(codec), map(makeCellStyleMap(styleId)), failAt(failAt) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &codec;
        DRW_Textstyle textStyle;
        textStyle.handle = kTextStyleHandle;
        textStyle.name = "GENERATED_CELLSTYLEMAP_TEXT";
        storage.textStyles.push_back(textStyle);
    }

    void writeObjects() override {
        struct TrackingScope {
            ~TrackingScope() { disarmAllocationFailure(); }
        } trackingScope;
        armAllocationFailure(failAt);
        accepted = codec.writeCellStyleMap(&map);
        reached = static_cast<long>(allocationAttempts);
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    dx_data storage;
    DRW_CellStyleMap map;
    long failAt;
    long reached{0};
    bool accepted{false};
};

bool sameMap(const DRW_CellStyleMap& actual,
             const DRW_CellStyleMap& expected) {
    if (actual.handle != expected.handle
        || actual.parentHandle != expected.parentHandle
        || actual.m_cellStyles.size() != expected.m_cellStyles.size())
        return false;
    for (std::size_t i = 0; i < actual.m_cellStyles.size(); ++i) {
        const DRW_TableStyleCellStyle& a = actual.m_cellStyles[i];
        const DRW_TableStyleCellStyle& e = expected.m_cellStyles[i];
        if (a.m_type != e.m_type || a.m_hasData != e.m_hasData
            || a.m_id != e.m_id || a.m_styleClass != e.m_styleClass
            || a.m_name != e.m_name
        || a.m_backgroundColor != e.m_backgroundColor
            || a.m_contentFormat.m_cellAlignment
                != e.m_contentFormat.m_cellAlignment
            || a.m_contentFormat.m_textHeight
                != e.m_contentFormat.m_textHeight
            || a.m_contentFormat.m_textStyleHandle
                != e.m_contentFormat.m_textStyleHandle
            || a.m_contentFormat.m_textStyleHandle
                != e.m_contentFormat.m_textStyleHandle
            || a.m_contentFormat.m_contentColor
                != e.m_contentFormat.m_contentColor
            || a.m_marginOverrideFlags != e.m_marginOverrideFlags
            || a.m_verticalMargin != e.m_verticalMargin
            || a.m_horizontalMargin != e.m_horizontalMargin
            || a.m_borders.size() != e.m_borders.size())
            return false;
        for (std::size_t j = 0; j < a.m_borders.size(); ++j) {
            const DRW_TableStyleBorder& ab = a.m_borders[j];
            const DRW_TableStyleBorder& eb = e.m_borders[j];
            if (ab.m_edgeFlags != eb.m_edgeFlags
                || ab.m_lineWeight != eb.m_lineWeight
                || ab.m_visible != eb.m_visible
                || ab.m_color != eb.m_color)
                return false;
        }
    }
    return true;
}

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

bool readbackMatches(const std::filesystem::path& path, int expectedStyleId) {
    dx_data data;
    dx_iface reader;
    const bool imported = reader.fileImport(path.string(), &data, false);
    if (!imported || data.cellStyleMaps.size() != 1u) {
        std::cerr << "  CELLSTYLEMAP readback imported=" << imported
                  << " typed=" << data.cellStyleMaps.size()
                  << " raw=" << data.rawCellStyleMaps.size() << '\n';
        return false;
    }
    const DRW_CellStyleMap& map = data.cellStyleMaps.front();
    if (map.handle != 0xE510u
        || map.parentHandle != DRW::DwgNamedObjectsDictionaryHandle
        || map.m_cellStyles.size() != 1u)
        return false;
    const DRW_TableStyleCellStyle& style = map.m_cellStyles.front();
    const bool textStyleResolved = std::any_of(
        data.textStyles.begin(), data.textStyles.end(),
        [&style](const DRW_Textstyle& textStyle) {
            return textStyle.handle
                    == style.m_contentFormat.m_textStyleHandle
                && textStyle.name == "GENERATED_CELLSTYLEMAP_TEXT";
        });
    const bool matches = style.m_type == 5 && style.m_hasData
        && style.m_id == expectedStyleId
        && style.m_styleClass == 2
        && style.m_name == (expectedStyleId == 4
            ? "GENERATED_CELLSTYLEMAP_TABLE"
            : "GENERATED_CELLSTYLEMAP_DATA")
        && style.m_contentFormat.m_cellAlignment == 1
        && style.m_contentFormat.m_textHeight == 1.75
        && textStyleResolved
        && style.m_contentFormat.m_contentColor == 0
        && style.m_borders.size() == 1u
        && style.m_borders.front().m_edgeFlags == 1
        && style.m_borders.front().m_lineWeight == 25
        && style.m_borders.front().m_visible == 1
        && style.m_borders.front().m_color == 3;
    if (!matches)
        std::cerr << "  CELLSTYLEMAP typed mismatch type=" << style.m_type
                  << " data=" << style.m_hasData << " id=" << style.m_id
                  << " class=" << style.m_styleClass
                  << " name=" << style.m_name
                  << " align=" << style.m_contentFormat.m_cellAlignment
                  << " text-style=" << style.m_contentFormat.m_textStyleHandle
                  << " borders=" << style.m_borders.size() << '\n';
    return matches;
}

bool run(const std::filesystem::path& directory) {
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory)) {
        std::cerr << "CELLSTYLEMAP allocation: scratch directory failed\n";
        return false;
    }
    const std::string sentinel = "prior CELLSTYLEMAP destination\n";
    const std::array<DRW::Version, 4> versions{{
        DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}};
    for (const int styleId : {101, 4}) {
      for (const DRW::Version version : versions) {
        if (styleId == 4 && version < DRW::AC1024)
            continue;
        for (const bool binary : {false, true}) {
            const auto output = directory
                / (std::string("cellstylemap-")
                   + std::to_string(static_cast<int>(version))
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                Export exporter(codec, 1000000, styleId);
                const bool wrote = codec.write(&exporter, version, binary);
                const bool readback = wrote && exporter.accepted
                    && readbackMatches(output, styleId);
                if (!readback) {
                    std::cerr << "CELLSTYLEMAP calibration version="
                              << static_cast<int>(version)
                              << (binary ? " Binary" : " ASCII")
                              << " wrote=" << wrote
                              << " callback=" << exporter.accepted
                              << " error=" << codec.getError()
                              << " diagnostic="
                              << codec.getLastDiagnostic().code << ": "
                              << codec.getLastDiagnostic().message << '\n';
                    return false;
                }
                allocationCount = exporter.reached;
                if (allocationCount <= 0
                    || !readBytes(output, expectedBytes))
                    return false;
            }
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                                       std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior)
                        return false;
                }
                dxfRW codec(output.string().c_str());
                Export exporter(codec, offset, styleId);
                const DRW_CellStyleMap source = exporter.map;
                const bool wrote = codec.write(&exporter, version, binary);
                if (!allocationFailed || wrote || exporter.accepted
                    || allocationAttempts
                        != static_cast<std::size_t>(offset + 1)
                    || !sameMap(exporter.map, source)) {
                    std::cerr << "CELLSTYLEMAP failure offset=" << offset
                              << " version=" << static_cast<int>(version)
                              << " binary=" << binary << '\n';
                    return false;
                }
                std::string afterFailure;
                if (!readBytes(output, afterFailure)
                    || afterFailure != sentinel)
                    return false;
                exporter.failAt = -1;
                const bool retried = codec.write(&exporter, version, binary);
                std::string retryBytes;
                if (!retried || !exporter.accepted
                    || !readBytes(output, retryBytes)
                    || retryBytes != expectedBytes
                    || !sameMap(exporter.map, source)
                    || !readbackMatches(output, styleId))
                    return false;
            }
            std::cout << "CELLSTYLEMAP version="
                      << static_cast<int>(version)
                      << (binary ? " Binary" : " ASCII")
                      << " allocation offsets=" << allocationCount
                      << " rollback/retry/readback=PASS\n";
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
      }
    }

    const auto boundaryOutput = directory / "cellstylemap-id4-ac1021.dxf";
    const std::string boundarySentinel = "prior ID-4 boundary destination\n";
    {
        std::ofstream prior(boundaryOutput,
                            std::ios::binary | std::ios::trunc);
        prior << boundarySentinel;
        if (!prior)
            return false;
    }
    dxfRW boundaryCodec(boundaryOutput.string().c_str());
    Export boundaryExporter(boundaryCodec, -1, 4);
    const DRW_CellStyleMap originalId4 = boundaryExporter.map;
    if (boundaryCodec.write(&boundaryExporter, DRW::AC1021, false)
        || boundaryExporter.accepted
        || !sameMap(boundaryExporter.map, originalId4))
        return false;
    std::string boundaryBytes;
    if (!readBytes(boundaryOutput, boundaryBytes)
        || boundaryBytes != boundarySentinel)
        return false;
    boundaryExporter.map.m_cellStyles.front().m_id = 101;
    boundaryExporter.map.m_cellStyles.front().m_name =
        "GENERATED_CELLSTYLEMAP_DATA";
    const DRW_CellStyleMap correctedId101 = boundaryExporter.map;
    if (!boundaryCodec.write(&boundaryExporter, DRW::AC1021, false)
        || !boundaryExporter.accepted
        || !sameMap(boundaryExporter.map, correctedId101)
        || !readbackMatches(boundaryOutput, 101))
        return false;
    std::cout << "CELLSTYLEMAP ID-4 AC1021 refusal/correction/retry=PASS\n";
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (failAllocation())
        throw std::bad_alloc();
    if (void* value = std::malloc(size == 0u ? 1u : size))
        return value;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete(void* value, std::size_t) noexcept { std::free(value); }
void operator delete[](void* value, std::size_t) noexcept { std::free(value); }

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: dxf_cellstylemap_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
