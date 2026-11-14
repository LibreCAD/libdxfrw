// SPDX-License-Identifier: GPL-2.0-or-later
// Generated TABLEGEOMETRY OBJECTS allocation rollback and retry control.
#include "dx_iface.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>
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

DRW_TableGeometry makeGeometry() {
    DRW_TableGeometry geometry;
    geometry.handle = 0xD7C1u;
    geometry.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
    geometry.m_rowCount = 2;
    geometry.m_columnCount = 2;
    geometry.m_cellCount = 4;

    const auto makeContent = [](double offset) {
        DRW_TableGeometryContent content;
        content.m_topLeft = {1.0 + offset, 2.0 + offset, 0.0};
        content.m_center = {7.0 + offset, 5.0 + offset, 0.0};
        content.m_contentWidth = 10.0 + offset;
        content.m_contentHeight = 4.0 + offset;
        content.m_width = 11.0 + offset;
        content.m_height = 5.0 + offset;
        return content;
    };
    const auto addCell = [&geometry](std::int32_t flags, double width,
                                     double height,
                                     std::vector<DRW_TableGeometryContent> contents) {
        DRW_TableGeometryCell cell;
        cell.m_flags = flags;
        cell.m_widthWithGap = width;
        cell.m_heightWithGap = height;
        cell.m_contents = std::move(contents);
        geometry.m_cells.push_back(std::move(cell));
    };
    addCell(3, 12.5, 6.25, {});
    addCell(4, 13.5, 7.25, {makeContent(0.0)});
    addCell(7, 14.5, 8.25, {makeContent(1.0), makeContent(2.0)});
    addCell(3, 15.5, 9.25, {makeContent(3.0)});
    return geometry;
}

class Export final : public dx_iface {
public:
    explicit Export(dxfRW& codec) : codec(codec), geometry(makeGeometry()) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &codec;
    }

    void writeObjects() override {
        if (failAt < 0) {
            accepted = codec.writeTableGeometry(&geometry);
            return;
        }
        armAllocationFailure(failAt);
        try {
            accepted = codec.writeTableGeometry(&geometry);
        } catch (const std::bad_alloc&) {
            accepted = false;
        }
        reached = static_cast<long>(allocationAttempts);
        disarmAllocationFailure();
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    dx_data storage;
    DRW_TableGeometry geometry;
    long failAt = -1;
    long reached = 0;
    bool accepted = false;
};

class Readback final : public dx_iface {
public:
    Readback() {
        cData = &storage;
        currentBlock = storage.mBlock;
    }

    void addTableGeometry(const DRW_TableGeometry& value) override {
        records.push_back(value);
    }
    dx_data storage;
    std::vector<DRW_TableGeometry> records;
};

bool sourceMatches(const DRW_TableGeometry& actual,
                   const DRW_TableGeometry& expected);

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

bool readbackMatches(const std::filesystem::path& path, DRW::Version version) {
    dxfRW codec(path.string().c_str());
    Readback reader;
    if (!codec.read(&reader, false) || codec.getSourceVersion() != version
        || reader.records.size() != 1u)
        return false;
    return sourceMatches(reader.records.front(), makeGeometry());
}

bool sourceMatches(const DRW_TableGeometry& actual,
                   const DRW_TableGeometry& expected) {
    if (actual.handle != expected.handle
        || actual.parentHandle != expected.parentHandle
        || actual.m_rowCount != expected.m_rowCount
        || actual.m_columnCount != expected.m_columnCount
        || actual.m_cellCount != expected.m_cellCount
        || actual.m_cells.size() != expected.m_cells.size())
        return false;
    for (std::size_t i = 0; i < actual.m_cells.size(); ++i) {
        const auto& a = actual.m_cells[i];
        const auto& e = expected.m_cells[i];
        if (a.m_flags != e.m_flags || a.m_widthWithGap != e.m_widthWithGap
            || a.m_heightWithGap != e.m_heightWithGap
            || a.m_unknownHandle != e.m_unknownHandle
            || a.m_contents.size() != e.m_contents.size())
            return false;
        for (std::size_t j = 0; j < a.m_contents.size(); ++j) {
            const auto& x = a.m_contents[j];
            const auto& y = e.m_contents[j];
            if (x.m_topLeft.x != y.m_topLeft.x
                || x.m_topLeft.y != y.m_topLeft.y
                || x.m_topLeft.z != y.m_topLeft.z
                || x.m_center.x != y.m_center.x
                || x.m_center.y != y.m_center.y
                || x.m_center.z != y.m_center.z
                || x.m_contentWidth != y.m_contentWidth
                || x.m_contentHeight != y.m_contentHeight
                || x.m_width != y.m_width || x.m_height != y.m_height
                || x.m_unknown != y.m_unknown
                || x.m_dwgUnknownValue != y.m_dwgUnknownValue
                || x.m_hasDwgUnknownValue != y.m_hasDwgUnknownValue)
                return false;
        }
    }
    return true;
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "TABLEGEOMETRY OBJECTS allocation retry: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    const auto output = directory / "tablegeometry-objects.dxf";
    const std::string sentinel = "prior TABLEGEOMETRY destination bytes\n";
    const DRW::Version versions[] = {
        DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032};
    const auto versionName = [](DRW::Version version) {
        switch (version) {
        case DRW::AC1021: return "AC1021";
        case DRW::AC1024: return "AC1024";
        case DRW::AC1027: return "AC1027";
        case DRW::AC1032: return "AC1032";
        default: return "UNKNOWN";
        }
    };
    for (const DRW::Version version : versions) {
      for (const bool binary : {false, true}) {
        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW codec(output.string().c_str());
            Export exporter(codec);
            exporter.failAt = 1000000;
            if (!codec.write(&exporter, version, binary)
                || !exporter.accepted || !readbackMatches(output, version))
                return fail("allocation calibration output did not read back");
            allocationCount = exporter.reached;
            if (!readBytes(output, expectedBytes))
                return fail("could not retain calibration output bytes");
        }
        if (allocationCount <= 0)
            return fail("calibration observed no callback allocations");

        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior)
                    return fail("could not initialize destination sentinel");
            }
            dxfRW codec(output.string().c_str());
            Export exporter(codec);
            const DRW_TableGeometry source = exporter.geometry;
            exporter.failAt = offset;
            exporter.reached = 0;
            exporter.accepted = false;
            const bool written = codec.write(&exporter, version, binary);
            exporter.failAt = -1;
            std::string current;
            if (written || exporter.accepted || exporter.reached != offset + 1
                || !allocationFailed
                || !readBytes(output, current) || current != sentinel)
                return fail("injected failure changed destination or missed allocation");
            if (!sourceMatches(exporter.geometry, source))
                return fail("injected failure changed caller TABLEGEOMETRY state");
            const bool retry = codec.write(&exporter, version, binary);
            std::string retryBytes;
            if (!retry || !exporter.accepted || !readBytes(output, retryBytes)
                || retryBytes != expectedBytes || !readbackMatches(output, version)
                || !sourceMatches(exporter.geometry, source))
                return fail("same-codec retry failed exact bytes or typed readback");
        }
        std::cout << "TABLEGEOMETRY OBJECTS " << versionName(version) << ' '
                  << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", rollback/retry/readback=PASS\n";
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
    if (void* value = std::malloc(size == 0 ? 1u : size))
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
        std::cerr << "usage: tablegeometry_objects_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
