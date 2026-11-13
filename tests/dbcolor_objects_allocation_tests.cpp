// SPDX-License-Identifier: GPL-2.0-or-later
// Generated DBCOLOR OBJECTS allocation rollback and retry control.
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

class DbColorExport final : public dx_iface {
public:
    explicit DbColorExport(dxfRW& codec) : codec(codec) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &codec;
        dictionary.handle = 0xD7AFu;
        dictionary.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        dictionary.hardOwner = 1u;
        dictionary.cloning = 2u;
        color.handle = 0xD7B0u;
        color.parentHandle = dictionary.handle;
        color.rgb = 0x345678;
        color.colorIndex = 0u;
        color.colorMethod = 0xC2u;
        color.name = "LOCAL_COLOR_ENTRY";
        color.bookName = "LOCAL_COLOR_BOOK";
        dictionary.m_entries = {{"LOCAL_COLOR_BOOK$LOCAL_COLOR_ENTRY", color.handle}};
        codec.setRootDictEntries({{"ACAD_COLOR", "D7AF"}});
        codec.setNamedDictObjects(std::vector<DRW_Dictionary>{dictionary});
    }

    void writeObjects() override {
        if (failAt < 0) {
            wrote = codec.writeDbColor(&color, "DBCOLOR");
            if (!wrote)
                (void)codec.writeRawDxfObject(nullptr);
            return;
        }
        armAllocationFailure(failAt);
        try {
            wrote = codec.writeDbColor(&color, "DBCOLOR");
        } catch (const std::bad_alloc&) {
            reached = static_cast<long>(allocationAttempts);
            disarmAllocationFailure();
            throw;
        }
        reached = static_cast<long>(allocationAttempts);
        disarmAllocationFailure();
        if (!wrote)
            (void)codec.writeRawDxfObject(nullptr);
    }
    bool finalizeDxfWrite() override { return wrote; }

    dxfRW& codec;
    dx_data storage;
    DRW_Dictionary dictionary;
    DRW_DbColor color;
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
    class DbColorReadback final : public dx_iface {
    public:
        void addDbColor(const DRW_DbColor& value) override {
            dx_iface::addDbColor(value);
            records.push_back(value);
        }
        std::vector<DRW_DbColor> records;
    } reader;
    const bool imported = reader.fileImport(path.string(), &data, false);
    if (!imported || data.sourceFormat != dx_source_format::DXF
        || reader.records.size() != 1u) {
        std::cerr << "readback imported=" << imported
                  << " sourceFormat=" << static_cast<int>(data.sourceFormat)
                  << " records=" << reader.records.size() << '\n';
        return false;
    }
    const DRW_DbColor& color = reader.records.front();
    return color.handle == 0xD7B0u && color.parentHandle == 0xD7AFu
        && color.rgb == 0x345678
        && color.colorMethod == 0xC2u
        && color.name == "LOCAL_COLOR_ENTRY"
        && color.bookName == "LOCAL_COLOR_BOOK";
}

bool sourceMatches(const DRW_DbColor& actual, const DRW_DbColor& expected) {
    return actual.handle == expected.handle
        && actual.parentHandle == expected.parentHandle
        && actual.rgb == expected.rgb
        && actual.colorIndex == expected.colorIndex
        && actual.colorMethod == expected.colorMethod
        && actual.name == expected.name
        && actual.bookName == expected.bookName
        && actual.reactorHandles == expected.reactorHandles
        && actual.extData.size() == expected.extData.size();
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "DBCOLOR OBJECTS allocation retry: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    const std::string sentinel = "prior DBCOLOR destination bytes\n";
    for (const DRW::Version version : {
             DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        const auto output = directory
            / (std::string("dbcolor-objects-")
               + std::to_string(static_cast<int>(version)) + ".dxf");
        for (const bool binary : {false, true}) {
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                DbColorExport exporter(codec);
                codec.setDxfTextWritePolicy(DRW::DxfTextWritePolicy::RequireLossless);
                exporter.failAt = 1000000;
                const bool exported = codec.write(&exporter, version, binary);
                const bool matches = exported && exporter.wrote
                    && readbackMatches(output);
                if (!matches) {
                    std::cerr << "calibration codecWrite=" << exported
                              << " callbackWrite=" << exporter.wrote
                              << " version=" << static_cast<int>(version)
                              << " binary=" << binary << '\n';
                    return fail("allocation calibration output did not read back");
                }
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
                dxfRW codec(output.string().c_str());
                DbColorExport exporter(codec);
                codec.setDxfTextWritePolicy(DRW::DxfTextWritePolicy::RequireLossless);
                const DRW_DbColor source = exporter.color;
                exporter.failAt = offset;
                exporter.reached = 0;
                exporter.wrote = false;
                const bool wrote = codec.write(&exporter, version, binary);
                exporter.failAt = -1;
                std::string current;
                if (wrote || exporter.wrote || exporter.reached != offset + 1
                    || !allocationFailed
                    || !readAll(output, current) || current != sentinel)
                {
                    std::cerr << "allocation offset=" << offset
                              << " codecWrite=" << wrote
                              << " callbackWrite=" << exporter.wrote
                              << " reached=" << exporter.reached
                              << " version=" << static_cast<int>(version)
                              << " sentinel=" << (current == sentinel) << '\n';
                    return fail("injected failure changed destination or missed allocation");
                }
                if (!sourceMatches(exporter.color, source))
                    return fail("injected failure changed caller DBCOLOR state");
                const bool retry = codec.write(&exporter, version, binary);
                std::string retryBytes;
                const bool byteEqual = retry && readAll(output, retryBytes)
                    && retryBytes == expectedBytes;
                const bool readback = retry && readbackMatches(output);
                if (!retry || !exporter.wrote || !byteEqual || !readback
                    || !sourceMatches(exporter.color, source)) {
                    std::cerr << "retry codecWrite=" << retry
                              << " callbackWrite=" << exporter.wrote
                              << " readback=" << readback
                              << " bytesEqual=" << byteEqual
                              << " binary=" << binary
                              << " version=" << static_cast<int>(version)
                              << " offset=" << offset << '\n';
                    if (retry && !byteEqual) {
                        std::size_t diff = 0u;
                        while (diff < expectedBytes.size()
                               && diff < retryBytes.size()
                               && expectedBytes[diff] == retryBytes[diff])
                            ++diff;
                        std::cerr << "first byte difference=" << diff
                                  << " expected-size=" << expectedBytes.size()
                                  << " actual-size=" << retryBytes.size()
                                  << " expected-context="
                                  << expectedBytes.substr(diff > 40u ? diff - 40u : 0u, 100u)
                                  << " actual-context="
                                  << retryBytes.substr(diff > 40u ? diff - 40u : 0u, 100u)
                                  << '\n';
                    }
                    return fail("same-codec retry failed typed DBCOLOR readback");
                }
            }
            std::cout << "DBCOLOR OBJECTS version="
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
        std::cerr << "usage: dbcolor_objects_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
