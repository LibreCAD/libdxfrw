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

DRW_RawDxfObject makeRawObject(DRW::Version version) {
    DRW_RawDxfObject object;
    object.name = "XRECORD";
    object.handle = 0xD8A0u;
    object.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
    object.m_version = version;
    object.groups = {
        DRW_Variant(5, std::string("D8A0")),
        DRW_Variant(330, std::string("C")),
        DRW_Variant(100, std::string("AcDbXrecord")),
        DRW_Variant(1, std::string("original first value")),
        DRW_Variant(3, std::string("original second value")),
        DRW_Variant(2, std::string("original symbol name")),
        DRW_Variant(6, std::string("original linetype")),
        DRW_Variant(300, std::string("original extended string")),
        DRW_Variant(90, std::int32_t{314}),
        DRW_Variant(91, std::int32_t{17})};
    return object;
}

const std::vector<std::pair<std::size_t, std::string>>& stringOverrides() {
    static const std::vector<std::pair<std::size_t, std::string>> overrides{
        {3u, "replacement first value"},
        {4u, "replacement second value"},
        {5u, "replacement symbol name"},
        {6u, "replacement linetype"},
        {7u, "replacement extended string"}};
    return overrides;
}

const std::vector<std::pair<std::size_t, std::int32_t>>& int32Overrides() {
    static const std::vector<std::pair<std::size_t, std::int32_t>> overrides{
        {8u, 2718},
        {9u, -91}};
    return overrides;
}

bool rawObjectMatches(const DRW_RawDxfObject& actual,
                      const DRW_RawDxfObject& expected) {
    if (actual.name != expected.name || actual.handle != expected.handle
        || actual.parentHandle != expected.parentHandle
        || actual.groups.size() != expected.groups.size()
        || actual.rawValues != expected.rawValues
        || actual.hasRawValues != expected.hasRawValues
        || actual.m_version != expected.m_version
        || actual.m_retainedTextLease != expected.m_retainedTextLease)
        return false;
    for (std::size_t index = 0; index < actual.groups.size(); ++index) {
        const auto& left = actual.groups[index];
        const auto& right = expected.groups[index];
        if (left.code() != right.code() || left.type() != right.type())
            return false;
        if (left.type() == DRW_Variant::STRING) {
            if (left.c_str() == nullptr || right.c_str() == nullptr
                || std::string(left.c_str()) != right.c_str())
                return false;
        } else if (left.type() == DRW_Variant::INTEGER) {
            if (left.i_val() != right.i_val())
                return false;
        } else {
            return false;
        }
    }
    return true;
}

class Export final : public dx_iface {
public:
    explicit Export(dxfRW& codec, DRW::Version version,
                    bool combinedOverrides)
        : codec(codec), object(makeRawObject(version)),
          combinedOverrides(combinedOverrides) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &codec;
    }

    void writeObjects() override {
        struct TrackingScope {
            ~TrackingScope() { disarmAllocationFailure(); }
        } trackingScope;
        armAllocationFailure(failAt);
        accepted = combinedOverrides
            ? codec.writeRawDxfObjectWithStringAndInt32Overrides(
                  &object, stringOverrides(), int32Overrides())
            : codec.writeRawDxfObjectWithStringOverrides(
                  &object, stringOverrides());
        reached = static_cast<long>(allocationAttempts);
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    dx_data storage;
    DRW_RawDxfObject object;
    bool combinedOverrides;
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

bool rawObjectReadbackMatches(const DRW_RawDxfObject& object,
                              bool expectInt32Override) {
    return object.name == "XRECORD" && object.handle == 0xD8A0u
        && object.parentHandle == 0xCu && object.groups.size() == 10u
        && object.groups[0].code() == 5
        && object.groups[1].code() == 330
        && object.groups[2].code() == 100
        && object.groups[3].code() == 1
        && object.groups[3].type() == DRW_Variant::STRING
        && object.groups[3].c_str() != nullptr
        && std::string(object.groups[3].c_str()) == "replacement first value"
        && object.groups[4].code() == 3
        && object.groups[4].type() == DRW_Variant::STRING
        && object.groups[4].c_str() != nullptr
        && std::string(object.groups[4].c_str()) == "replacement second value"
        && object.groups[5].code() == 2
        && object.groups[5].type() == DRW_Variant::STRING
        && object.groups[5].c_str() != nullptr
        && std::string(object.groups[5].c_str()) == "replacement symbol name"
        && object.groups[6].code() == 6
        && object.groups[6].type() == DRW_Variant::STRING
        && object.groups[6].c_str() != nullptr
        && std::string(object.groups[6].c_str()) == "replacement linetype"
        && object.groups[7].code() == 300
        && object.groups[7].type() == DRW_Variant::STRING
        && object.groups[7].c_str() != nullptr
        && std::string(object.groups[7].c_str())
            == "replacement extended string"
        && object.groups[8].code() == 90
        && object.groups[8].type() == DRW_Variant::INTEGER
        && object.groups[8].i_val() == (expectInt32Override ? 2718 : 314)
        && object.groups[9].code() == 91
        && object.groups[9].type() == DRW_Variant::INTEGER
        && object.groups[9].i_val() == (expectInt32Override ? -91 : 17);
}

bool readbackMatches(const std::filesystem::path& path,
                     bool combinedOverrides) {
    dxfRW codec(path.string().c_str());
    Readback reader;
    const bool read = codec.read(&reader, false);
    const bool matched = read && reader.objects.size() == 1u
        && rawObjectReadbackMatches(reader.objects.front(), combinedOverrides);
    if (!matched) {
        std::cerr << "raw XRECORD readback diagnostics: read=" << read
                  << " objects=" << reader.objects.size()
                  << " combined=" << combinedOverrides << '\n';
        for (const auto& object : reader.objects) {
            std::cerr << "  identity=" << object.name << '/' << object.handle
                      << '/' << object.parentHandle << " groups="
                      << object.groups.size() << '\n';
            for (const auto& group : object.groups) {
                std::cerr << "  code=" << group.code() << " type="
                          << group.type();
                if (group.type() == DRW_Variant::STRING)
                    std::cerr << " text=" << group.c_str();
                else if (group.type() == DRW_Variant::INTEGER)
                    std::cerr << " int=" << group.i_val();
                std::cerr << '\n';
            }
        }
    }
    return matched;
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

    // Initialize the static override vectors before measuring callback-local
    // allocations; their one-time construction is setup, not writer work.
    (void)stringOverrides();
    (void)int32Overrides();

    const std::string sentinel = "prior raw XRECORD destination bytes\n";
    for (const DRW::Version version : {
             DRW::AC1021, DRW::AC1027, DRW::AC1032}) {
      for (const bool binary : {false, true}) {
        for (const bool combinedOverrides : {false, true}) {
            const std::string outputName = (combinedOverrides
                ? "raw-xrecord-string-int32-overrides-"
                : "raw-xrecord-string-overrides-")
                + std::to_string(static_cast<int>(version))
                + (binary ? "-binary.dxf" : "-ascii.dxf");
            const auto output = directory / outputName;
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                Export exporter(codec, version, combinedOverrides);
                exporter.failAt = 1000000;
                const bool wrote = codec.write(&exporter, version, binary);
                if (!wrote || !exporter.accepted
                    || !readbackMatches(output, combinedOverrides))
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
                Export exporter(codec, version, combinedOverrides);
                const DRW_RawDxfObject source = exporter.object;
                exporter.failAt = offset;
                const bool wrote = codec.write(&exporter, version, binary);
                const bool sourcePreserved = rawObjectMatches(
                    exporter.object, source);
                if (!allocationFailed || wrote || exporter.accepted
                    || allocationAttempts != static_cast<std::size_t>(offset + 1)
                    || !sourcePreserved) {
                    std::cerr << "raw XRECORD override offset=" << offset
                              << " attempts=" << allocationAttempts
                              << " fired=" << allocationFailed
                              << " wrote=" << wrote
                              << " callback=" << exporter.accepted
                              << " sourcePreserved=" << sourcePreserved
                              << " name=" << exporter.object.name
                              << " handle=" << exporter.object.handle
                              << " owner=" << exporter.object.parentHandle
                              << " version=" << exporter.object.m_version
                              << " lease="
                              << (exporter.object.m_retainedTextLease
                                      == source.m_retainedTextLease)
                              << '\n';
                    for (std::size_t index = 0;
                         index < exporter.object.groups.size(); ++index) {
                        const auto& group = exporter.object.groups[index];
                        std::cerr << "  group " << index << " code="
                                  << group.code() << " type=" << group.type();
                        if (group.type() == DRW_Variant::STRING)
                            std::cerr << " text=" << group.c_str();
                        else if (group.type() == DRW_Variant::INTEGER)
                            std::cerr << " int=" << group.i_val();
                        std::cerr << '\n';
                    }
                    return fail("injected failure changed caller state or succeeded");
                }
                std::string afterFailure;
                if (!readBytes(output, afterFailure) || afterFailure != sentinel)
                    return fail("injected failure replaced destination");

                exporter.failAt = -1;
                const bool retried = codec.write(&exporter, version, binary);
                std::string retryBytes;
                if (!retried || !exporter.accepted
                    || !readBytes(output, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(output, combinedOverrides)
                    || !rawObjectMatches(exporter.object, source))
                    return fail("same-codec retry failed exact bytes/readback");
            }
            std::cout << "raw XRECORD version="
                      << static_cast<int>(version) << ' '
                      << (combinedOverrides ? "string+Int32" : "string")
                      << " overrides "
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
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
        std::cerr << "usage: raw_object_string_override_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
