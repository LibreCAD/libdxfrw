// SPDX-License-Identifier: GPL-2.0-or-later
// Generated raw DXF object string-override allocation rollback/retry control.
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

DRW_RawDxfObject makeRawObject() {
    DRW_RawDxfObject object;
    object.name = "XRECORD";
    object.handle = 0xD8A0u;
    object.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
    object.m_version = DRW::AC1027;
    object.groups = {
        DRW_Variant(5, std::string("D8A0")),
        DRW_Variant(330, std::string("C")),
        DRW_Variant(100, std::string("AcDbXrecord")),
        DRW_Variant(1, std::string("original first value")),
        DRW_Variant(3, std::string("original second value"))};
    return object;
}

const std::vector<std::pair<std::size_t, std::string>>& stringOverrides() {
    static const std::vector<std::pair<std::size_t, std::string>> overrides{
        {3u, "replacement first value"},
        {4u, "replacement second value"}};
    return overrides;
}

bool rawObjectMatches(const DRW_RawDxfObject& actual,
                      const DRW_RawDxfObject& expected,
                      bool expectOverrides) {
    if (actual.name != expected.name || actual.groups.size() != 5u)
        return false;
    const std::string first = expectOverrides
        ? "replacement first value" : "original first value";
    const std::string second = expectOverrides
        ? "replacement second value" : "original second value";
    return actual.groups[1].code() == 330
        && actual.groups[2].code() == 100
        && actual.groups[3].code() == 1
        && actual.groups[3].type() == DRW_Variant::STRING
        && actual.groups[3].c_str() != nullptr
        && actual.groups[3].c_str() == first
        && actual.groups[4].code() == 3
        && actual.groups[4].type() == DRW_Variant::STRING
        && actual.groups[4].c_str() != nullptr
        && actual.groups[4].c_str() == second;
}

class Export final : public dx_iface {
public:
    explicit Export(dxfRW& codec) : codec(codec), object(makeRawObject()) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &codec;
    }

    void writeObjects() override {
        struct TrackingScope {
            ~TrackingScope() { disarmAllocationFailure(); }
        } trackingScope;
        armAllocationFailure(failAt);
        accepted = codec.writeRawDxfObjectWithStringOverrides(
            &object, stringOverrides());
        reached = static_cast<long>(allocationAttempts);
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    dx_data storage;
    DRW_RawDxfObject object;
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

    void addRawDxfObject(const DRW_RawDxfObject& value) override {
        objects.push_back(value);
    }

    dx_data storage;
    std::vector<DRW_RawDxfObject> objects;
};

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

bool readbackMatches(const std::filesystem::path& path) {
    dxfRW codec(path.string().c_str());
    Readback reader;
    const bool read = codec.read(&reader, false);
    return read && reader.objects.size() == 1u
        && rawObjectMatches(reader.objects.front(), makeRawObject(), true);
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "raw DXF object string-override allocation retry: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    // Initialize the static override vector before measuring callback-local
    // allocations; its one-time construction is setup, not writer work.
    (void)stringOverrides();

    const auto output = directory / "raw-xrecord-string-overrides.dxf";
    const std::string sentinel = "prior raw XRECORD destination bytes\n";
    for (const bool binary : {false, true}) {
        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW codec(output.string().c_str());
            Export exporter(codec);
            exporter.failAt = 1000000;
            const bool wrote = codec.write(&exporter, DRW::AC1027, binary);
            if (!wrote || !exporter.accepted || !readbackMatches(output))
                return fail("calibration output did not read back");
            allocationCount = exporter.reached;
            if (!readBytes(output, expectedBytes))
                return fail("could not retain calibration bytes");
        }
        if (allocationCount <= 0)
            return fail("calibration observed no callback allocations");

        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream prior(output,
                                   std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior)
                    return fail("could not initialize destination sentinel");
            }
            dxfRW codec(output.string().c_str());
            Export exporter(codec);
            const DRW_RawDxfObject source = exporter.object;
            exporter.failAt = offset;
            const bool wrote = codec.write(&exporter, DRW::AC1027, binary);
            if (!allocationFailed || wrote || exporter.accepted
                || allocationAttempts != static_cast<std::size_t>(offset + 1)
                || !rawObjectMatches(exporter.object, source, false)) {
                std::cerr << "raw XRECORD override offset=" << offset
                          << " attempts=" << allocationAttempts
                          << " fired=" << allocationFailed
                          << " wrote=" << wrote
                          << " callback=" << exporter.accepted << '\n';
                return fail("injected failure changed caller state or succeeded");
            }
            std::string afterFailure;
            if (!readBytes(output, afterFailure) || afterFailure != sentinel)
                return fail("injected failure replaced destination");

            exporter.failAt = -1;
            const bool retried = codec.write(&exporter, DRW::AC1027, binary);
            std::string retryBytes;
            if (!retried || !exporter.accepted
                || !readBytes(output, retryBytes)
                || retryBytes != expectedBytes || !readbackMatches(output)
                || !rawObjectMatches(exporter.object, source, false))
                return fail("same-codec retry failed exact bytes/readback");
        }
        std::cout << "raw XRECORD string overrides "
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
        std::cerr << "usage: raw_object_string_override_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
