// SPDX-License-Identifier: GPL-2.0-or-later
// Generated POINTCLOUDCOLORMAP version-matrix rollback/retry control.
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include "intern/dxfwriter.h"

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
std::size_t allocationAttempts = 0;

void armAllocationFailure(long count) noexcept {
    allocationsBeforeFailure = count;
    allocationAttempts = 0;
}

void disarmAllocationFailure() noexcept {
    allocationsBeforeFailure = -1;
}

struct AllocationScope {
    explicit AllocationScope(long offset) noexcept {
        armAllocationFailure(offset);
    }
    ~AllocationScope() { disarmAllocationFailure(); }
};

DRW_PointCloudColorMap makeColorMap() {
    DRW_PointCloudColorMap colorMap;
    colorMap.handle = 0xD980u;
    colorMap.parentHandle = 0xCu;
    colorMap.m_classVersion = 4;
    colorMap.m_defaultIntensityColorScheme = "intensity/" + std::string(96u, 'i');
    colorMap.m_defaultElevationColorScheme = "elevation/" + std::string(96u, 'e');
    colorMap.m_defaultClassificationColorScheme = "classification/"
        + std::string(96u, 'c');
    colorMap.m_colorRamps = {
        {2, 3u, {"ground", "canopy", "roof"}},
        {3, 2u, {"low", "high"}}};
    colorMap.m_colorRampCount = 2u;
    colorMap.m_classificationColorRamps = {
        {4, 2u, {"road", "rail"}}};
    colorMap.m_classificationColorRampCount = 1u;
    return colorMap;
}

bool sameRamps(const std::vector<DRW_PointCloudColorMapRamp>& a,
               const std::vector<DRW_PointCloudColorMapRamp>& b) {
    if (a.size() != b.size())
        return false;
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (a[i].m_classVersion != b[i].m_classVersion
            || a[i].m_rampCount != b[i].m_rampCount
            || a[i].m_colorSchemes != b[i].m_colorSchemes)
            return false;
    }
    return true;
}

bool sameColorMap(const DRW_PointCloudColorMap& a,
                  const DRW_PointCloudColorMap& b) {
    return a.handle == b.handle && a.parentHandle == b.parentHandle
        && a.m_classVersion == b.m_classVersion
        && a.m_defaultIntensityColorScheme == b.m_defaultIntensityColorScheme
        && a.m_defaultElevationColorScheme == b.m_defaultElevationColorScheme
        && a.m_defaultClassificationColorScheme
            == b.m_defaultClassificationColorScheme
        && a.m_colorRampCount == b.m_colorRampCount
        && sameRamps(a.m_colorRamps, b.m_colorRamps)
        && a.m_classificationColorRampCount
            == b.m_classificationColorRampCount
        && sameRamps(a.m_classificationColorRamps,
                    b.m_classificationColorRamps);
}

struct Capture final : LC3CompatCheck {
    void addPointCloudColorMap(const DRW_PointCloudColorMap& value) override {
        ++count;
        colorMap = value;
    }
    DRW_PointCloudColorMap colorMap;
    unsigned count = 0;
};

struct Export final : LC3CompatCheck {
    explicit Export(dxfRW& codec) : codec(codec), colorMap(makeColorMap()) {}

    void writeObjects() override {
        struct TrackingScope {
            long& reached;
            ~TrackingScope() {
                reached = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
            }
        } trackingScope{reached};
        AllocationScope allocationScope(failAt);
        try {
            accepted = codec.writePointCloudColorMap(&colorMap);
        } catch (const std::bad_alloc&) {
            accepted = false;
        }
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    DRW_PointCloudColorMap colorMap;
    long failAt = 1000000;
    long reached = 0;
    bool accepted = false;
};

std::string fileBytes(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>{input},
            std::istreambuf_iterator<char>{}};
}

bool readbackMatches(const std::filesystem::path& path,
                     DRW::Version expectedVersion) {
    dxfRW codec(path.string().c_str());
    Capture capture;
    return codec.read(&capture, false)
        && codec.getSourceVersion() == expectedVersion
        && capture.count == 1u
        && sameColorMap(capture.colorMap, makeColorMap());
}

bool run(const std::filesystem::path& directory) {
    static constexpr std::array<std::pair<DRW::Version, const char*>, 7>
        versions = {{{DRW::AC1014, "AC1014"}, {DRW::AC1015, "AC1015"},
                     {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
                     {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
                     {DRW::AC1032, "AC1032"}}};
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error)
        return false;

    for (const auto& target : versions) {
        for (const bool binary : {false, true}) {
            const std::string stem = std::string("pointcloud-colormap-")
                + target.second + (binary ? "-binary.dxf" : "-ascii.dxf");
            const auto output = directory / stem;
            long allocationCount = 0;
            std::string referenceBytes;
            {
                dxfRW codec(output.string().c_str());
                Export exporter(codec);
                if (!codec.write(&exporter, target.first, binary)
                    || !exporter.accepted || exporter.reached <= 0
                    || !readbackMatches(output, target.first)) {
                    std::cerr << "POINTCLOUDCOLORMAP clean write failed at "
                              << target.second << '\n';
                    return false;
                }
                allocationCount = exporter.reached;
                referenceBytes = fileBytes(output);
            }
            if (referenceBytes.empty())
                return false;

            const std::string sentinel = "prior POINTCLOUDCOLORMAP bytes\n";
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream previous(output,
                        std::ios::binary | std::ios::trunc);
                    previous << sentinel;
                    if (!previous)
                        return false;
                }
                dxfRW codec(output.string().c_str());
                Export exporter(codec);
                exporter.failAt = offset;
                const DRW_PointCloudColorMap original = exporter.colorMap;
                const bool written = codec.write(&exporter, target.first, binary);
                if (written || exporter.accepted
                    || exporter.reached != offset + 1
                    || !sameColorMap(exporter.colorMap, original)
                    || fileBytes(output) != sentinel) {
                    std::cerr << "POINTCLOUDCOLORMAP rollback failed at "
                              << target.second << " offset " << offset << '\n';
                    return false;
                }
                exporter.failAt = 1000000;
                if (!codec.write(&exporter, target.first, binary)
                    || !exporter.accepted
                    || fileBytes(output) != referenceBytes
                    || !readbackMatches(output, target.first)) {
                    std::cerr << "POINTCLOUDCOLORMAP retry failed at "
                              << target.second << " offset " << offset << '\n';
                    return false;
                }
            }
            std::cout << "POINTCLOUDCOLORMAP " << target.second << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
            std::filesystem::remove(output, error);
            error.clear();
        }
    }
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (allocationsBeforeFailure >= 0)
        ++allocationAttempts;
    if (allocationsBeforeFailure == 0) {
        allocationsBeforeFailure = -1;
        throw std::bad_alloc();
    }
    if (allocationsBeforeFailure > 0)
        --allocationsBeforeFailure;
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
        std::cerr << "usage: dxf_pointcloud_colormap_version_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    const std::filesystem::path directory(argv[1]);
    if (!run(directory))
        return 1;
    std::error_code error;
    std::filesystem::remove_all(directory, error);
    return error ? 1 : 0;
}
