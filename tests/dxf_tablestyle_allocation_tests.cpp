// SPDX-License-Identifier: GPL-2.0-or-later
// Generated legacy TABLESTYLE DXF writer allocation rollback/retry control.
#include "dx_iface.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace {
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

DRW_TableStyle makeTableStyle(bool rich) {
    DRW_TableStyle style;
    style.handle = 0xD958u;
    style.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
    style.m_name = "GENERATED_LEGACY_TRANSACTION_STYLE";
    if (rich) {
        style.m_name = "GENERATED_R24_TRANSACTION_STYLE";
        style.m_r24PrefixPresent = true;
        style.m_r24UnknownValue2 = 101;
        const auto noneColor = [] {
            DRW_TableStyleCmcColor color;
            color.m_method = 0xC1u;
            return color;
        };
        const auto makeCellStyle = [&noneColor](int id, int styleClass,
                                                 const char* name,
                                                 int alignment,
                                                 double textHeight,
                                                 std::uint32_t textStyleHandle) {
            DRW_TableStyleCellStyle cell;
            cell.m_type = 5;
            cell.m_hasData = true;
            cell.m_backgroundColor = 256;
            cell.m_backgroundColorCmc.m_method = 0xC8u;
            cell.m_contentLayout = 1u;
            cell.m_marginOverrideFlags = 1;
            cell.m_verticalMargin = 0.06;
            cell.m_horizontalMargin = 0.06;
            cell.m_bottomMargin = 0.06;
            cell.m_rightMargin = 0.06;
            cell.m_marginHorizontalSpacing = 0.18;
            cell.m_marginVerticalSpacing = 0.18;
            cell.m_id = id;
            cell.m_styleClass = styleClass;
            cell.m_name = name;
            cell.m_contentFormat.m_cellAlignment = alignment;
            cell.m_contentFormat.m_contentColor = 0;
            cell.m_contentFormat.m_contentColorCmc = noneColor();
            cell.m_contentFormat.m_textStyleHandle = textStyleHandle;
            cell.m_contentFormat.m_textHeight = textHeight;
            for (int borderIndex = 0; borderIndex < 6; ++borderIndex) {
                DRW_TableStyleBorder border;
                border.m_edgeFlags = 1 << borderIndex;
                border.m_borderType = 1;
                border.m_color = 0;
                border.m_colorCmc = noneColor();
                border.m_lineWeight = -2;
                border.m_visible = 0;
                cell.m_borders.push_back(border);
            }
            return cell;
        };
        style.m_tableCellStyle = makeCellStyle(4, 2, "Table", 1, 0.0, 0u);
        style.m_tableCellStyle.m_borders.clear();
        style.m_cellStyles.push_back(makeCellStyle(
            1, 1, "_TITLE", 5, 0.25, 0xD957u));
        style.m_cellStyles.push_back(makeCellStyle(
            2, 1, "_HEADER", 5, 0.18, 0xD957u));
        style.m_cellStyles.push_back(makeCellStyle(
            3, 2, "_DATA", 2, 0.18, 0xD957u));
        style.m_cellStyles[0].m_mergeFlags = 0x8000u;
        return style;
    }
    for (int rowIndex = 0; rowIndex < 3; ++rowIndex) {
        DRW_TableStyleRowStyle row;
        row.m_textStyleHandle = 0xD957u;
        row.m_textHeight = 1.0 + rowIndex;
        row.m_textAlignment = 5;
        row.m_textColor = rowIndex + 1;
        row.m_fillColor = rowIndex + 4;
        row.m_hasBackgroundColor = rowIndex != 1;
        for (int borderIndex = 0; borderIndex < 6; ++borderIndex) {
            DRW_TableStyleBorder border;
            border.m_edgeFlags = 1 << borderIndex;
            border.m_lineWeight = borderIndex;
            border.m_visible = borderIndex % 2;
            border.m_color = borderIndex + 1;
            row.m_borders.push_back(border);
        }
        style.m_rowStyles.push_back(std::move(row));
    }
    return style;
}

class Export final : public dx_iface {
public:
    Export(dxfRW& codec, long failAt, bool rich)
        : codec(codec), style(makeTableStyle(rich)), failAt(failAt) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &codec;
        storage.importGeneration = 1u;
        DRW_Textstyle textStyle;
        textStyle.handle = 0xD957u;
        textStyle.name = "GENERATED_LEGACY_TRANSACTION_TEXT";
        storage.textStyles.push_back(textStyle);
    }

    void writeObjects() override {
        struct TrackingScope {
            ~TrackingScope() { disarmAllocationFailure(); }
        } trackingScope;
        armAllocationFailure(failAt);
        accepted = codec.writeTableStyle(&style, storage.textStyles);
        reached = static_cast<long>(allocationAttempts);
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    dx_data storage;
    DRW_TableStyle style;
    long failAt;
    long reached{0};
    bool accepted{false};
};

bool sameStyle(const DRW_TableStyle& actual,
               const DRW_TableStyle& expected) {
    const auto sameColor = [](const DRW_TableStyleCmcColor& a,
                              const DRW_TableStyleCmcColor& e) {
        return a.m_rgb == e.m_rgb && a.m_name == e.m_name
            && a.m_bookName == e.m_bookName && a.m_method == e.m_method
            && a.m_flags == e.m_flags
            && a.m_reservedIndex == e.m_reservedIndex;
    };
    const auto sameBorder = [&sameColor](const DRW_TableStyleBorder& a,
                                         const DRW_TableStyleBorder& e) {
        return a.m_edgeFlags == e.m_edgeFlags
            && a.m_propertyOverrideFlags == e.m_propertyOverrideFlags
            && a.m_borderType == e.m_borderType
            && a.m_color == e.m_color
            && sameColor(a.m_colorCmc, e.m_colorCmc)
            && a.m_lineWeight == e.m_lineWeight
            && a.m_lineTypeHandle == e.m_lineTypeHandle
            && a.m_visible == e.m_visible
            && a.m_doubleLineSpacing == e.m_doubleLineSpacing;
    };
    const auto sameFormat = [&sameColor](
            const DRW_TableStyleContentFormat& a,
            const DRW_TableStyleContentFormat& e) {
        return a.m_propertyOverrideFlags == e.m_propertyOverrideFlags
            && a.m_propertyFlags == e.m_propertyFlags
            && a.m_valueDataType == e.m_valueDataType
            && a.m_valueUnitType == e.m_valueUnitType
            && a.m_valueFormatString == e.m_valueFormatString
            && a.m_rotation == e.m_rotation
            && a.m_blockScale == e.m_blockScale
            && a.m_cellAlignment == e.m_cellAlignment
            && a.m_contentColor == e.m_contentColor
            && sameColor(a.m_contentColorCmc, e.m_contentColorCmc)
            && a.m_textStyleHandle == e.m_textStyleHandle
            && a.m_textHeight == e.m_textHeight;
    };
    const auto sameCell = [&sameColor, &sameBorder, &sameFormat](
            const DRW_TableStyleCellStyle& a,
            const DRW_TableStyleCellStyle& e) {
        if (a.m_type != e.m_type || a.m_hasData != e.m_hasData
            || a.m_propertyOverrideFlags != e.m_propertyOverrideFlags
            || a.m_mergeFlags != e.m_mergeFlags
            || a.m_backgroundColor != e.m_backgroundColor
            || !sameColor(a.m_backgroundColorCmc, e.m_backgroundColorCmc)
            || a.m_contentLayout != e.m_contentLayout
            || !sameFormat(a.m_contentFormat, e.m_contentFormat)
            || a.m_marginOverrideFlags != e.m_marginOverrideFlags
            || a.m_verticalMargin != e.m_verticalMargin
            || a.m_horizontalMargin != e.m_horizontalMargin
            || a.m_bottomMargin != e.m_bottomMargin
            || a.m_rightMargin != e.m_rightMargin
            || a.m_marginHorizontalSpacing != e.m_marginHorizontalSpacing
            || a.m_marginVerticalSpacing != e.m_marginVerticalSpacing
            || a.m_id != e.m_id || a.m_styleClass != e.m_styleClass
            || a.m_name != e.m_name || a.m_borders.size() != e.m_borders.size())
            return false;
        for (std::size_t index = 0; index < a.m_borders.size(); ++index) {
            if (!sameBorder(a.m_borders[index], e.m_borders[index]))
                return false;
        }
        return true;
    };
    if (actual.handle != expected.handle
        || actual.parentHandle != expected.parentHandle
        || actual.m_name != expected.m_name
        || actual.m_r24PrefixPresent != expected.m_r24PrefixPresent
        || actual.m_r24ClassVersion != expected.m_r24ClassVersion
        || actual.m_r24UnknownValue1 != expected.m_r24UnknownValue1
        || actual.m_r24UnknownValue2 != expected.m_r24UnknownValue2
        || actual.m_rowStyles.size() != expected.m_rowStyles.size()
        || actual.m_cellStyles.size() != expected.m_cellStyles.size()
        || !sameCell(actual.m_tableCellStyle, expected.m_tableCellStyle))
        return false;
    for (std::size_t rowIndex = 0;
         rowIndex < actual.m_rowStyles.size(); ++rowIndex) {
        const DRW_TableStyleRowStyle& a = actual.m_rowStyles[rowIndex];
        const DRW_TableStyleRowStyle& e = expected.m_rowStyles[rowIndex];
        if (a.m_textStyleHandle != e.m_textStyleHandle
            || a.m_textHeight != e.m_textHeight
            || a.m_textAlignment != e.m_textAlignment
            || a.m_textColor != e.m_textColor
            || a.m_fillColor != e.m_fillColor
            || !sameColor(a.m_textColorCmc, e.m_textColorCmc)
            || !sameColor(a.m_fillColorCmc, e.m_fillColorCmc)
            || a.m_hasBackgroundColor != e.m_hasBackgroundColor
            || a.m_valueDataType != e.m_valueDataType
            || a.m_valueUnitType != e.m_valueUnitType
            || a.m_valueFormatString != e.m_valueFormatString
            || a.m_borders.size() != e.m_borders.size())
            return false;
        for (std::size_t borderIndex = 0;
             borderIndex < a.m_borders.size(); ++borderIndex) {
            const DRW_TableStyleBorder& ab = a.m_borders[borderIndex];
            const DRW_TableStyleBorder& eb = e.m_borders[borderIndex];
            if (!sameBorder(ab, eb))
                return false;
        }
    }
    for (std::size_t index = 0; index < actual.m_cellStyles.size(); ++index) {
        if (!sameCell(actual.m_cellStyles[index], expected.m_cellStyles[index]))
            return false;
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

bool readbackMatches(const std::filesystem::path& path, bool rich) {
    dx_data data;
    dx_iface reader;
    const bool imported = reader.fileImport(path.string(), &data, false);
    if (!imported || data.tableStyles.size() != 1u) {
        std::cerr << "  readback imported=" << imported << " styles="
                  << data.tableStyles.size() << '\n';
        return false;
    }
    const DRW_TableStyle& style = data.tableStyles.front();
    const bool common = style.handle == 0xD958u
        && style.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
        && style.m_name == (rich ? "GENERATED_R24_TRANSACTION_STYLE"
                                 : "GENERATED_LEGACY_TRANSACTION_STYLE");
    if (!common)
        return false;
    if (rich) {
        const bool matches = style.m_cellStyles.size() == 3u
            && style.m_cellStyles[0].m_id == 1
            && style.m_cellStyles[0].m_name == "_TITLE"
            && style.m_cellStyles[1].m_id == 2
            && style.m_cellStyles[1].m_name == "_HEADER"
            && style.m_cellStyles[2].m_id == 3
            && style.m_cellStyles[2].m_name == "_DATA"
            && style.m_cellStyles[2].m_borders.size() == 6u
            && style.m_cellStyles[2].m_borders[5].m_edgeFlags == 32;
        if (!matches)
            std::cerr << "  rich fields prefix=" << style.m_r24PrefixPresent
                      << " cells=" << style.m_cellStyles.size()
                      << " rows=" << style.m_rowStyles.size()
                      << " name=" << style.m_name << '\n';
        return matches;
    }
    return !style.m_r24PrefixPresent && style.m_rowStyles.size() == 3u
        && style.m_rowStyles[0].m_textHeight == 1.0
        && style.m_rowStyles[1].m_textHeight == 2.0
        && style.m_rowStyles[2].m_textHeight == 3.0
        && style.m_rowStyles[0].m_borders.size() == 6u
        && style.m_rowStyles[2].m_borders[5].m_edgeFlags == 32
        && style.m_rowStyles[2].m_borders[5].m_lineWeight == 5
        && style.m_rowStyles[2].m_borders[5].m_visible == 1
        && style.m_rowStyles[2].m_borders[5].m_color == 6;
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](DRW::Version version, bool binary,
                         const char* message) {
        std::cerr << "TABLESTYLE allocation version="
                  << static_cast<int>(version)
                  << (binary ? " Binary: " : " ASCII: ") << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory)) {
        std::cerr << "TABLESTYLE allocation: scratch directory failed\n";
        return false;
    }

    const std::string sentinel = "previous TABLESTYLE destination\n";
    struct Profile {
        DRW::Version version;
        bool rich;
    };
    for (const Profile profile : std::array<Profile, 5>{{
             {DRW::AC1021, false}, {DRW::AC1032, false},
             {DRW::AC1024, true}, {DRW::AC1027, true},
             {DRW::AC1032, true}}}) {
        const DRW::Version version = profile.version;
        const bool rich = profile.rich;
        for (const bool binary : {false, true}) {
            const auto output = directory
                / (std::string(rich ? "tablestyle-r24-" : "tablestyle-legacy-")
                   + std::to_string(static_cast<int>(version))
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                Export exporter(codec, 1000000, rich);
                const bool wrote = codec.write(&exporter, version, binary);
                const bool readback = wrote && exporter.accepted
                    && readbackMatches(output, rich);
                if (!readback) {
                    std::cerr << "  calibration wrote=" << wrote
                              << " callback=" << exporter.accepted
                              << " error=" << codec.getError()
                              << " diagnostic="
                              << codec.getLastDiagnostic().code << '\n';
                    return fail(version, binary,
                                "calibration output/readback failed");
                }
                allocationCount = exporter.reached;
                if (allocationCount <= 0
                    || !readBytes(output, expectedBytes))
                    return fail(version, binary,
                                "could not retain calibration bytes/count");
            }
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                                       std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior)
                        return fail(version, binary,
                                    "could not seed destination sentinel");
                }
                dxfRW codec(output.string().c_str());
                Export exporter(codec, offset, rich);
                const DRW_TableStyle source = exporter.style;
                const bool wrote = codec.write(&exporter, version, binary);
                if (!allocationFailed || wrote || exporter.accepted
                    || allocationAttempts
                        != static_cast<std::size_t>(offset + 1)
                    || !sameStyle(exporter.style, source))
                    return fail(version, binary,
                                "allocation refusal changed caller or succeeded");
                std::string afterFailure;
                if (!readBytes(output, afterFailure) || afterFailure != sentinel)
                    return fail(version, binary,
                                "allocation refusal replaced destination");

                exporter.failAt = -1;
                const bool retried = codec.write(&exporter, version, binary);
                std::string retryBytes;
                if (!retried || !exporter.accepted
                    || !readBytes(output, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(output, rich))
                    return fail(version, binary,
                                "same-codec retry/readback failed");
            }
            std::cout << (rich ? "R24 TABLESTYLE version="
                               : "legacy TABLESTYLE version=")
                      << static_cast<int>(version)
                      << (binary ? " Binary" : " ASCII")
                      << " allocation offsets=" << allocationCount
                      << " rollback/retry/readback=PASS\n";
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
    }
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
        std::cerr << "usage: dxf_tablestyle_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
