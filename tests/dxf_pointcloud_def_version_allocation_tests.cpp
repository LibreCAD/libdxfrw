// SPDX-License-Identifier: GPL-2.0-or-later
// Generated POINTCLOUD definition/reactor version-matrix transaction control.
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

DRW_PointCloudDef makeDefinition(DRW_PointCloudDef::Kind kind) {
    DRW_PointCloudDef definition;
    definition.handle = 0xD970u + static_cast<std::uint32_t>(kind);
    definition.parentHandle = 0xCu;
    definition.m_kind = kind;
    definition.m_classVersion = 2;
    if (kind == DRW_PointCloudDef::Definition
        || kind == DRW_PointCloudDef::DefinitionEx) {
        definition.m_sourceFilename = "pointcloud/" + std::string(257u, 'p')
            + (kind == DRW_PointCloudDef::Definition ? ".rcs" : ".rcp");
        definition.m_isLoaded = true;
        definition.m_pointCount = 0x100000007ULL;
        definition.m_extentsMin = DRW_Coord{-10.0, -20.0, -30.0};
        definition.m_extentsMax = DRW_Coord{40.0, 50.0, 60.0};
    }
    return definition;
}

bool sameDefinition(const DRW_PointCloudDef& a,
                    const DRW_PointCloudDef& b) {
    return a.handle == b.handle && a.parentHandle == b.parentHandle
        && a.m_kind == b.m_kind && a.m_classVersion == b.m_classVersion
        && a.m_sourceFilename == b.m_sourceFilename
        && a.m_isLoaded == b.m_isLoaded && a.m_pointCount == b.m_pointCount
        && a.m_extentsMin.x == b.m_extentsMin.x
        && a.m_extentsMin.y == b.m_extentsMin.y
        && a.m_extentsMin.z == b.m_extentsMin.z
        && a.m_extentsMax.x == b.m_extentsMax.x
        && a.m_extentsMax.y == b.m_extentsMax.y
        && a.m_extentsMax.z == b.m_extentsMax.z;
}

const char* kindName(DRW_PointCloudDef::Kind kind) {
    switch (kind) {
    case DRW_PointCloudDef::Definition: return "DEFINITION";
    case DRW_PointCloudDef::DefinitionEx: return "DEFINITIONEX";
    case DRW_PointCloudDef::Reactor: return "REACTOR";
    case DRW_PointCloudDef::ReactorEx: return "REACTOREX";
    }
    return "UNKNOWN";
}

struct Capture final : LC3CompatCheck {
    void addPointCloudDef(const DRW_PointCloudDef& value) override {
        ++count;
        definition = value;
    }
    DRW_PointCloudDef definition;
    unsigned count = 0;
};

struct Export final : LC3CompatCheck {
    explicit Export(dxfRW& codec, DRW_PointCloudDef::Kind kind)
        : codec(codec), definition(makeDefinition(kind)) {}

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
            accepted = codec.writePointCloudDef(&definition);
        } catch (const std::bad_alloc&) {
            accepted = false;
        }
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    DRW_PointCloudDef definition;
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
                     DRW::Version expectedVersion,
                     DRW_PointCloudDef::Kind kind) {
    dxfRW codec(path.string().c_str());
    Capture capture;
    return codec.read(&capture, false)
        && codec.getSourceVersion() == expectedVersion
        && capture.count == 1u
        && sameDefinition(capture.definition, makeDefinition(kind));
}

bool run(const std::filesystem::path& directory) {
    static constexpr std::array<std::pair<DRW::Version, const char*>, 7>
        versions = {{{DRW::AC1014, "AC1014"}, {DRW::AC1015, "AC1015"},
                     {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
                     {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
                     {DRW::AC1032, "AC1032"}}};
    static constexpr std::array<DRW_PointCloudDef::Kind, 4> kinds = {{
        DRW_PointCloudDef::Definition, DRW_PointCloudDef::DefinitionEx,
        DRW_PointCloudDef::Reactor, DRW_PointCloudDef::ReactorEx}};
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error)
        return false;

    for (const auto kind : kinds) {
        for (const auto& target : versions) {
            for (const bool binary : {false, true}) {
                const std::string stem = std::string("pointcloud-")
                    + kindName(kind) + "-" + target.second
                    + (binary ? "-binary.dxf" : "-ascii.dxf");
                const auto output = directory / stem;
                long allocationCount = 0;
                std::string referenceBytes;
                {
                    dxfRW codec(output.string().c_str());
                    Export exporter(codec, kind);
                    if (!codec.write(&exporter, target.first, binary)
                        || !exporter.accepted || exporter.reached <= 0
                        || !readbackMatches(output, target.first, kind)) {
                        std::cerr << "POINTCLOUD " << kindName(kind)
                                  << " clean write failed at " << target.second
                                  << '\n';
                        return false;
                    }
                    allocationCount = exporter.reached;
                    referenceBytes = fileBytes(output);
                }
                if (referenceBytes.empty())
                    return false;

                const std::string sentinel = "prior POINTCLOUD definition bytes\n";
                for (long offset = 0; offset < allocationCount; ++offset) {
                    {
                        std::ofstream previous(output,
                            std::ios::binary | std::ios::trunc);
                        previous << sentinel;
                        if (!previous)
                            return false;
                    }
                    dxfRW codec(output.string().c_str());
                    Export exporter(codec, kind);
                    exporter.failAt = offset;
                    const DRW_PointCloudDef original = exporter.definition;
                    const bool written = codec.write(&exporter, target.first, binary);
                    if (written || exporter.accepted
                        || exporter.reached != offset + 1
                        || !sameDefinition(exporter.definition, original)
                        || fileBytes(output) != sentinel) {
                        std::cerr << "POINTCLOUD " << kindName(kind)
                                  << " rollback failed at " << target.second
                                  << " offset " << offset << '\n';
                        return false;
                    }
                    exporter.failAt = 1000000;
                    if (!codec.write(&exporter, target.first, binary)
                        || !exporter.accepted
                        || fileBytes(output) != referenceBytes
                        || !readbackMatches(output, target.first, kind)) {
                        std::cerr << "POINTCLOUD " << kindName(kind)
                                  << " retry failed at " << target.second
                                  << " offset " << offset << '\n';
                        return false;
                    }
                }
                std::cout << "POINTCLOUD " << kindName(kind) << ' '
                          << target.second << ' '
                          << (binary ? "Binary" : "ASCII")
                          << ": offsets=" << allocationCount
                          << ", rollback/retry/readback=PASS\n";
                std::filesystem::remove(output, error);
                error.clear();
            }
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
        std::cerr << "usage: dxf_pointcloud_def_version_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    const std::filesystem::path directory(argv[1]);
    if (!run(directory))
        return 1;
    std::error_code error;
    std::filesystem::remove_all(directory, error);
    return error ? 1 : 0;
}
