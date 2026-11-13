// SPDX-License-Identifier: GPL-2.0-or-later
// Generated legacy TABLESTYLE DXF writer allocation rollback/retry control.
#include "dx_iface.h"

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

DRW_TableStyle makeTableStyle() {
    DRW_TableStyle style;
    style.handle = 0xD958u;
    style.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
    style.m_name = "GENERATED_LEGACY_TRANSACTION_STYLE";
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
    Export(dxfRW& codec, long failAt)
        : codec(codec), style(makeTableStyle()), failAt(failAt) {
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
    if (actual.handle != expected.handle
        || actual.parentHandle != expected.parentHandle
        || actual.m_name != expected.m_name
        || actual.m_rowStyles.size() != expected.m_rowStyles.size())
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
            if (ab.m_edgeFlags != eb.m_edgeFlags
                || ab.m_lineWeight != eb.m_lineWeight
                || ab.m_visible != eb.m_visible
                || ab.m_color != eb.m_color
                || ab.m_doubleLineSpacing != eb.m_doubleLineSpacing)
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

bool readbackMatches(const std::filesystem::path& path) {
    dx_data data;
    dx_iface reader;
    if (!reader.fileImport(path.string(), &data, false)
        || data.tableStyles.size() != 1u)
        return false;
    const DRW_TableStyle& style = data.tableStyles.front();
    return style.handle == 0xD958u
        && style.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
        && style.m_name == "GENERATED_LEGACY_TRANSACTION_STYLE"
        && style.m_rowStyles.size() == 3u
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
        std::cerr << "legacy TABLESTYLE allocation version="
                  << static_cast<int>(version)
                  << (binary ? " Binary: " : " ASCII: ") << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory)) {
        std::cerr << "legacy TABLESTYLE allocation: scratch directory failed\n";
        return false;
    }

    const std::string sentinel = "previous TABLESTYLE destination\n";
    for (const DRW::Version version : {DRW::AC1021, DRW::AC1032}) {
        for (const bool binary : {false, true}) {
            const auto output = directory
                / (std::string("tablestyle-")
                   + std::to_string(static_cast<int>(version))
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                Export exporter(codec, 1000000);
                if (!codec.write(&exporter, version, binary)
                    || !exporter.accepted || !readbackMatches(output))
                    return fail(version, binary,
                                "calibration output/readback failed");
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
                Export exporter(codec, offset);
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
                    || retryBytes != expectedBytes || !readbackMatches(output))
                    return fail(version, binary,
                                "same-codec retry/readback failed");
            }
            std::cout << "legacy TABLESTYLE version="
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
        std::cerr << "usage: dxf_tablestyle_legacy_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
