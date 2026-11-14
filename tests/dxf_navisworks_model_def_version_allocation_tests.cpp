// SPDX-License-Identifier: GPL-2.0-or-later
// Generated NAVISWORKSMODELDEF version-matrix rollback/retry control.
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include "intern/dxfwriter.h"

#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <new>
#include <string>
#include <utility>

namespace {
long allocationsBeforeFailure = -1;
std::size_t failedAllocations = 0;
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

DRW_NavisworksModelDef makeDefinition() {
    DRW_NavisworksModelDef definition;
    definition.handle = 0xD962u;
    definition.parentHandle = 0xCu;
    definition.m_flags = 3;
    definition.m_path = "models/" + std::string(257u, 'n') + ".nwd";
    definition.m_status = true;
    definition.m_minExtent = DRW_Coord{-1.5, -2.5, -3.5};
    definition.m_maxExtent = DRW_Coord{4.5, 5.5, 6.5};
    definition.m_hostDrawingVisibility = true;
    return definition;
}

bool sameDefinition(const DRW_NavisworksModelDef& a,
                    const DRW_NavisworksModelDef& b) {
    return a.handle == b.handle && a.parentHandle == b.parentHandle
        && a.m_flags == b.m_flags && a.m_path == b.m_path
        && a.m_status == b.m_status
        && a.m_minExtent.x == b.m_minExtent.x
        && a.m_minExtent.y == b.m_minExtent.y
        && a.m_minExtent.z == b.m_minExtent.z
        && a.m_maxExtent.x == b.m_maxExtent.x
        && a.m_maxExtent.y == b.m_maxExtent.y
        && a.m_maxExtent.z == b.m_maxExtent.z
        && a.m_hostDrawingVisibility == b.m_hostDrawingVisibility;
}

struct Capture final : LC3CompatCheck {
    void addNavisworksModelDef(const DRW_NavisworksModelDef& value) override {
        ++count;
        definition = value;
    }
    DRW_NavisworksModelDef definition;
    unsigned count = 0;
};

struct Export final : LC3CompatCheck {
    explicit Export(dxfRW& codec) : codec(codec), definition(makeDefinition()) {
    }

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
            accepted = codec.writeNavisworksModelDef(&definition);
        } catch (const std::bad_alloc&) {
            accepted = false;
        }
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    DRW_NavisworksModelDef definition;
    long failAt = 1000000;
    long reached = 0;
    bool accepted = false;
};

std::string bytesAt(const std::filesystem::path& path) {
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
        && sameDefinition(capture.definition, makeDefinition());
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
            const std::string stem = std::string("navisworks-model-def-")
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
                    std::cerr << "NAVISWORKSMODELDEF clean write failed at "
                              << target.second << '\n';
                    return false;
                }
                allocationCount = exporter.reached;
                referenceBytes = bytesAt(output);
            }
            if (referenceBytes.empty())
                return false;

            const std::string sentinel = "previous NAVISWORKSMODELDEF bytes\n";
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
                const DRW_NavisworksModelDef original = exporter.definition;
                const bool written = codec.write(&exporter, target.first, binary);
                if (written || exporter.accepted || exporter.reached != offset + 1
                    || !sameDefinition(exporter.definition, original)
                    || bytesAt(output) != sentinel) {
                    std::cerr << "NAVISWORKSMODELDEF rollback failed at "
                              << target.second << " offset " << offset << '\n';
                    return false;
                }
                exporter.failAt = 1000000;
                if (!codec.write(&exporter, target.first, binary)
                    || !exporter.accepted
                    || bytesAt(output) != referenceBytes
                    || !readbackMatches(output, target.first)) {
                    std::cerr << "NAVISWORKSMODELDEF retry failed at "
                              << target.second << " offset " << offset << '\n';
                    return false;
                }
            }
            std::cout << "NAVISWORKSMODELDEF " << target.second << ' '
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
        ++failedAllocations;
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
        std::cerr << "usage: dxf_navisworks_model_def_version_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    const std::filesystem::path directory(argv[1]);
    if (!run(directory))
        return 1;
    std::error_code error;
    std::filesystem::remove_all(directory, error);
    return error ? 1 : 0;
}
