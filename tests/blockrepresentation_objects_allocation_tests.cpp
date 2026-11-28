// SPDX-License-Identifier: GPL-2.0-or-later
// Generated BLOCKREPRESENTATION_DATA OBJECTS allocation rollback and retry.
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

class Export final : public dx_iface {
public:
    explicit Export(dxfRW& codec) : codec(codec) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &codec;
        object.handle = 0xE803u;
        object.parentHandle = 0xE802u;
        object.m_flag = 1u;
        object.m_blockHandle = 0xE804u;
        object.m_bodyFullyDecoded = true;
        object.reactorHandles = {0xE802u};
    }

    void writeObjects() override {
        if (failAt >= 0)
            armAllocationFailure(failAt);
        try {
            accepted = codec.writeBlockRepresentationData(&object);
        } catch (const std::bad_alloc&) {
            accepted = false;
        }
        if (failAt >= 0) {
            reached = static_cast<long>(allocationAttempts);
            disarmAllocationFailure();
        }
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    dx_data storage;
    DRW_BlockRepresentationData object;
    long failAt = -1;
    long reached = 0;
    bool accepted = false;
};

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

bool readbackMatches(const std::filesystem::path& path,
                     DRW::Version version) {
    dx_data data;
    dx_iface reader;
    return reader.fileImport(path.string(), &data, false)
        && data.sourceFormat == dx_source_format::DXF
        && data.sourceVersion == version
        && data.blockRepresentationData.size() == 1u
        && data.blockRepresentationData.front().handle == 0xE803u
        && data.blockRepresentationData.front().parentHandle == 0xE802u
        && data.blockRepresentationData.front().reactorHandles
            == std::vector<std::uint32_t>{0xE802u}
        && data.blockRepresentationData.front().m_flag == 1u
        && data.blockRepresentationData.front().m_blockHandle == 0xE804u
        && data.blockRepresentationData.front().bodyFullyDecoded();
}

bool sourceMatches(const DRW_BlockRepresentationData& actual,
                   const DRW_BlockRepresentationData& expected) {
    return actual.handle == expected.handle
        && actual.parentHandle == expected.parentHandle
        && actual.reactorHandles == expected.reactorHandles
        && actual.m_flag == expected.m_flag
        && actual.m_blockHandle == expected.m_blockHandle
        && actual.m_bodyFullyDecoded == expected.m_bodyFullyDecoded;
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "BLOCKREPRESENTATION_DATA OBJECTS allocation retry: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    const auto output = directory / "blockrepresentation-objects.dxf";
    const std::string sentinel = "prior BLOCKREPRESENTATION_DATA bytes\n";
    for (const bool binary : {false, true}) {
        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW codec(output.string().c_str());
            Export exporter(codec);
            exporter.failAt = 1000000;
            const bool written = codec.write(&exporter, DRW::AC1024, binary);
            if (!written || !exporter.accepted || exporter.reached <= 0
                || !readbackMatches(output, DRW::AC1024))
                return fail("calibration did not write and read back");
            allocationCount = exporter.reached;
            if (!readBytes(output, expectedBytes))
                return fail("could not retain calibration bytes");
        }
        if (allocationCount <= 0)
            return fail("calibration observed no callback allocations");

        const std::vector<DRW::Version> refusedVersions = {
            DRW::AC1009, DRW::AC1014, DRW::AC1015, DRW::AC1018,
            DRW::AC1021, DRW::AC1027, DRW::AC1032};
        for (const DRW::Version refusedVersion : refusedVersions) {
            {
                std::ofstream prior(output,
                    std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior)
                    return fail("could not initialize version-gate sentinel");
            }
            dxfRW codec(output.string().c_str());
            Export exporter(codec);
            const DRW_BlockRepresentationData source = exporter.object;
            const bool written = codec.write(&exporter, refusedVersion,
                                              binary);
            std::string afterRefusal;
            if (written || exporter.accepted
                || !readBytes(output, afterRefusal) || afterRefusal != sentinel
                || !sourceMatches(exporter.object, source))
                return fail("unsupported target changed destination or caller state");

            const bool retry = codec.write(&exporter, DRW::AC1024, binary);
            std::string retryBytes;
            if (!retry || !exporter.accepted
                || !readBytes(output, retryBytes)
                || retryBytes != expectedBytes
                || !readbackMatches(output, DRW::AC1024)
                || !sourceMatches(exporter.object, source))
                return fail("supported-target retry failed after version refusal");
        }

        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior)
                    return fail("could not initialize destination sentinel");
            }
            dxfRW codec(output.string().c_str());
            Export exporter(codec);
            const DRW_BlockRepresentationData source = exporter.object;
            exporter.failAt = offset;
            const bool written = codec.write(&exporter, DRW::AC1024, binary);
            exporter.failAt = -1;
            std::string current;
            if (written || exporter.accepted || exporter.reached != offset + 1
                || !allocationFailed
                || !readBytes(output, current) || current != sentinel)
                return fail("injected allocation changed destination or was missed");
            if (!sourceMatches(exporter.object, source))
                return fail("injected allocation changed caller object state");

            const bool retry = codec.write(&exporter, DRW::AC1024, binary);
            std::string retryBytes;
            if (!retry || !exporter.accepted
                || !readBytes(output, retryBytes)
                || retryBytes != expectedBytes
                || !readbackMatches(output, DRW::AC1024)
                || !sourceMatches(exporter.object, source))
                return fail("same-codec retry failed byte or typed readback");
        }
        std::cout << "BLOCKREPRESENTATION_DATA OBJECTS "
                  << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", rollback/retry/readback=PASS\n";
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
    }
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (failAllocation())
        throw std::bad_alloc();
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
        std::cerr << "usage: blockrepresentation_objects_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
