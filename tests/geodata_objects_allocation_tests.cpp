// SPDX-License-Identifier: GPL-2.0-or-later
// Generated GEODATA OBJECTS allocation rollback and retry control.
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

class GeoDataExport final : public dx_iface {
public:
    explicit GeoDataExport(dxfRW& codec) : codec(codec) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &codec;
        geo.handle = 0xD7A0u;
        geo.parentHandle = 0x2Fu;
        geo.m_version = 2;
        geo.m_hostBlockHandle = 0x17u;
        geo.m_designPoint = {100.0, 200.0, 0.0};
        geo.m_referencePoint = {10.0, 20.0, 0.0};
        geo.m_coordinateSystemDefinition = "LOCAL_GRID_PROFILE";
        geo.m_geoRssTag = "LOCAL_GEODATA_TAG";
    }

    void writeObjects() override {
        if (failAt < 0) {
            wrote = codec.writeGeoData(&geo);
            if (!wrote)
                (void)codec.writeRawDxfObject(nullptr);
            return;
        }
        const std::size_t before = failedAllocations;
        AllocationFailure failure(failAt);
        try {
            wrote = codec.writeGeoData(&geo);
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

    dxfRW& codec;
    dx_data storage;
    DRW_GeoData geo;
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

bool readbackMatches(const std::filesystem::path& path) {
    dx_data data;
    class GeoDataReadback final : public dx_iface {
    public:
        void addGeoData(const DRW_GeoData& value) override {
            records.push_back(value);
        }
        std::vector<DRW_GeoData> records;
    } reader;
    if (!reader.fileImport(path.string(), &data, false)
        || data.sourceFormat != dx_source_format::DXF
        || reader.records.size() != 1u)
        return false;
    const DRW_GeoData& geo = reader.records.front();
    return geo.handle == 0xD7A0u && geo.m_version == 2
        && geo.m_hostBlockHandle == 0x17u
        && geo.m_designPoint.x == 100.0
        && geo.m_designPoint.y == 200.0
        && geo.m_referencePoint.x == 10.0
        && geo.m_referencePoint.y == 20.0
        && geo.m_coordinateSystemDefinition == "LOCAL_GRID_PROFILE"
        && geo.m_geoRssTag == "LOCAL_GEODATA_TAG";
}

bool sourceMatches(const DRW_GeoData& actual, const DRW_GeoData& expected) {
    return actual.handle == expected.handle
        && actual.parentHandle == expected.parentHandle
        && actual.m_version == expected.m_version
        && actual.m_hostBlockHandle == expected.m_hostBlockHandle
        && actual.m_designPoint.x == expected.m_designPoint.x
        && actual.m_designPoint.y == expected.m_designPoint.y
        && actual.m_designPoint.z == expected.m_designPoint.z
        && actual.m_referencePoint.x == expected.m_referencePoint.x
        && actual.m_referencePoint.y == expected.m_referencePoint.y
        && actual.m_referencePoint.z == expected.m_referencePoint.z
        && actual.m_coordinateSystemDefinition
            == expected.m_coordinateSystemDefinition
        && actual.m_geoRssTag == expected.m_geoRssTag;
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "GEODATA OBJECTS allocation retry: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    const std::string sentinel = "prior GEODATA destination bytes\n";
    for (const DRW::Version version : {
             DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        const auto output = directory
            / (std::string("geodata-objects-")
               + std::to_string(static_cast<int>(version)) + ".dxf");
        for (const bool binary : {false, true}) {
            long allocationCount = 0;
            std::string cleanBytes;
            {
                dxfRW codec(output.string().c_str());
                GeoDataExport exporter(codec);
                codec.setDxfTextWritePolicy(DRW::DxfTextWritePolicy::RequireLossless);
                exporter.failAt = 1000000;
                if (!codec.write(&exporter, version, binary)
                    || !exporter.wrote || !readbackMatches(output))
                    return fail("allocation calibration output did not read back");
                allocationCount = exporter.reached;
                if (!readAll(output, cleanBytes))
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
                GeoDataExport exporter(codec);
                codec.setDxfTextWritePolicy(DRW::DxfTextWritePolicy::RequireLossless);
                const DRW_GeoData source = exporter.geo;
                exporter.failAt = offset;
                exporter.reached = 0;
                exporter.wrote = false;
                const bool wrote = codec.write(&exporter, version, binary);
                exporter.failAt = -1;
                std::string current;
                if (wrote || exporter.reached != 1
                    || !readAll(output, current) || current != sentinel)
                {
                    std::cerr << "version=" << static_cast<int>(version)
                              << " binary=" << binary
                              << " allocation offset=" << offset
                              << " codecWrite=" << wrote
                              << " callbackWrite=" << exporter.wrote
                              << " reached=" << exporter.reached
                              << " sentinel=" << (current == sentinel) << '\n';
                    return fail("injected failure changed destination or missed allocation");
                }
                if (!sourceMatches(exporter.geo, source))
                    return fail("injected failure changed caller GEODATA state");

                if (!codec.write(&exporter, version, binary)
                    || !exporter.wrote || !readbackMatches(output)
                    || !sourceMatches(exporter.geo, source))
                    return fail("same-codec retry failed typed GEODATA readback");
                std::string retryBytes;
                if (!readAll(output, retryBytes) || retryBytes != cleanBytes)
                    return fail("same-codec retry differs from clean output");
            }
            std::cout << "GEODATA OBJECTS version="
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
        // One-shot injection lets cleanup and the next fallback allocation
        // proceed if the production layer converts bad_alloc to a refusal.
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
        std::cerr << "usage: geodata_objects_allocation_tests SCRATCH_DIR\n";
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
