// SPDX-License-Identifier: GPL-2.0-or-later
// Generated SPATIAL_FILTER OBJECTS allocation rollback and retry control.
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
std::size_t failedAllocations = 0;

struct AllocationFailure {
    explicit AllocationFailure(long count) { allocationsBeforeFailure = count; }
    ~AllocationFailure() { allocationsBeforeFailure = -1; }
};

class SpatialFilterExport final : public dx_iface {
public:
    explicit SpatialFilterExport(dxfRW& codec) : codec(codec) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &codec;
        filter.handle = 0xD7B2u;
        filter.parentHandle = 0xD7B1u;
        filter.m_boundaryPoints = {{1.25, -2.5, 0.0}, {4.75, 6.5, 0.0}};
        filter.m_normal = {0.0, 0.0, 1.0};
        filter.m_origin = {12.0, 13.0, 14.0};
        filter.m_displayBoundary = true;
        filter.m_clipFrontPlane = true;
        filter.m_clipBackPlane = true;
        filter.m_frontDistance = -7.25;
        filter.m_backDistance = 8.5;
        for (std::size_t index = 0; index < 12u; ++index) {
            filter.m_inverseInsertTransform.push_back(100.0 + index);
            filter.m_insertTransform.push_back(200.0 + index);
        }
    }

    void writeObjects() override {
        if (failAt < 0) {
            wrote = codec.writeSpatialFilter(&filter);
            if (!wrote)
                (void)codec.writeRawDxfObject(nullptr);
            return;
        }
        const std::size_t before = failedAllocations;
        AllocationFailure failure(failAt);
        try {
            wrote = codec.writeSpatialFilter(&filter);
        } catch (const std::bad_alloc&) {
            reached = failedAllocations != before
                ? static_cast<long>(failedAllocations - before)
                : failAt - allocationsBeforeFailure;
            throw;
        }
        reached = failedAllocations != before
            ? static_cast<long>(failedAllocations - before)
            : failAt - allocationsBeforeFailure;
        if (!wrote)
            (void)codec.writeRawDxfObject(nullptr);
    }

    bool finalizeDxfWrite() override { return wrote; }

    dxfRW& codec;
    dx_data storage;
    DRW_SpatialFilter filter;
    long failAt = -1;
    long reached = 0;
    bool wrote = false;
};

bool readAll(const std::filesystem::path& path, std::string& value) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    value.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

bool matches(const DRW_SpatialFilter& actual,
             const DRW_SpatialFilter& expected) {
    if (actual.handle != expected.handle
        || actual.parentHandle != expected.parentHandle
        || actual.m_boundaryPoints.size() != expected.m_boundaryPoints.size()
        || actual.m_normal.x != expected.m_normal.x
        || actual.m_normal.y != expected.m_normal.y
        || actual.m_normal.z != expected.m_normal.z
        || actual.m_origin.x != expected.m_origin.x
        || actual.m_origin.y != expected.m_origin.y
        || actual.m_origin.z != expected.m_origin.z
        || actual.m_displayBoundary != expected.m_displayBoundary
        || actual.m_clipFrontPlane != expected.m_clipFrontPlane
        || actual.m_clipBackPlane != expected.m_clipBackPlane
        || actual.m_frontDistance != expected.m_frontDistance
        || actual.m_backDistance != expected.m_backDistance
        || actual.m_inverseInsertTransform != expected.m_inverseInsertTransform
        || actual.m_insertTransform != expected.m_insertTransform)
        return false;
    for (std::size_t index = 0; index < expected.m_boundaryPoints.size(); ++index) {
        const auto& left = actual.m_boundaryPoints[index];
        const auto& right = expected.m_boundaryPoints[index];
        if (left.x != right.x || left.y != right.y || left.z != right.z)
            return false;
    }
    return true;
}

bool readbackMatches(const std::filesystem::path& path,
                     const DRW_SpatialFilter& expected) {
    dx_data data;
    class SpatialFilterReadback final : public dx_iface {
    public:
        void addSpatialFilter(const DRW_SpatialFilter& value) override {
            filters.push_back(value);
        }
        std::vector<DRW_SpatialFilter> filters;
    } reader;
    return reader.fileImport(path.string(), &data, false)
        && data.sourceFormat == dx_source_format::DXF
        && reader.filters.size() == 1u
        && matches(reader.filters.front(), expected);
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "SPATIAL_FILTER OBJECTS allocation retry: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    const std::string sentinel = "prior SPATIAL_FILTER destination bytes\n";
    for (const DRW::Version version : {
             DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        const auto output = directory
            / (std::string("spatial-filter-objects-")
               + std::to_string(static_cast<int>(version)) + ".dxf");
        for (const bool binary : {false, true}) {
            long allocationCount = 0;
            std::string expectedBytes;
            {
                const std::string outputName = output.string();
                dxfRW codec(outputName.c_str());
                SpatialFilterExport exporter(codec);
                codec.setDxfTextWritePolicy(DRW::DxfTextWritePolicy::RequireLossless);
                exporter.failAt = 1000000;
                if (!codec.write(&exporter, version, binary)
                    || !exporter.wrote
                    || !readbackMatches(output, exporter.filter))
                    return fail("allocation calibration output did not read back");
                allocationCount = exporter.reached;
                if (!readAll(output, expectedBytes))
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
                const std::string outputName = output.string();
                dxfRW codec(outputName.c_str());
                SpatialFilterExport exporter(codec);
                codec.setDxfTextWritePolicy(DRW::DxfTextWritePolicy::RequireLossless);
                const DRW_SpatialFilter source = exporter.filter;
                exporter.failAt = offset;
                exporter.reached = 0;
                exporter.wrote = false;
                const bool wrote = codec.write(&exporter, version, binary);
                exporter.failAt = -1;
                std::string current;
                if (wrote || exporter.reached != 1
                    || !readAll(output, current) || current != sentinel)
                    return fail("injected failure changed destination or missed allocation");
                if (!matches(exporter.filter, source))
                    return fail("injected failure changed caller SPATIAL_FILTER state");
                const bool retry = codec.write(&exporter, version, binary);
                std::string retryBytes;
                if (!retry || !exporter.wrote || !readAll(output, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(output, source)
                    || !matches(exporter.filter, source))
                    return fail("same-codec retry failed exact bytes or typed readback");
            }
            std::cout << "SPATIAL_FILTER OBJECTS version="
                      << static_cast<int>(version) << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
        }
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
    }
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (allocationsBeforeFailure == 0) {
        ++failedAllocations;
        allocationsBeforeFailure = -1;
        throw std::bad_alloc();
    }
    if (allocationsBeforeFailure > 0)
        --allocationsBeforeFailure;
    if (void* value = std::malloc(size == 0 ? 1 : size))
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
        std::cerr << "usage: spatial_filter_objects_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    const std::size_t before = failedAllocations;
    if (!run(argv[1]))
        return 1;
    if (failedAllocations == before) {
        std::cerr << "allocation injector did not reach operator new\n";
        return 1;
    }
    return 0;
}
