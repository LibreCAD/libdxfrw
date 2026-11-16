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
enum class RawWriteMode {
    Plain,
    StringOverrides,
    StringAndInt32Overrides
};

enum class InvalidOverride {
    None,
    StringOnInteger,
    Int32OnString
};

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

const std::vector<std::pair<std::size_t, std::string>>& invalidStringOverrides() {
    // Index 8 is group 90, an integer field, so it is not a valid string edit.
    static const std::vector<std::pair<std::size_t, std::string>> overrides{
        {3u, "valid first edit"}, {8u, "invalid integer edit"}};
    return overrides;
}

const std::vector<std::pair<std::size_t, std::int32_t>>& invalidInt32Overrides() {
    // Index 3 is group 1, a string field, so it is not a valid Int32 edit.
    static const std::vector<std::pair<std::size_t, std::int32_t>> overrides{
        {3u, 2718}};
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
                    RawWriteMode mode)
        : codec(codec), object(makeRawObject(version)),
          mode(mode) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &codec;
    }

    void writeObjects() override {
        struct TrackingScope {
            ~TrackingScope() { disarmAllocationFailure(); }
        } trackingScope;
        armAllocationFailure(failAt);
        if (invalidOverride == InvalidOverride::StringOnInteger)
            accepted = codec.writeRawDxfObjectWithStringOverrides(
                &object, invalidStringOverrides());
        else if (invalidOverride == InvalidOverride::Int32OnString)
            accepted = codec.writeRawDxfObjectWithStringAndInt32Overrides(
                &object, stringOverrides(), invalidInt32Overrides());
        else if (mode == RawWriteMode::Plain)
            accepted = codec.writeRawDxfObject(&object);
        else if (mode == RawWriteMode::StringAndInt32Overrides)
            accepted = codec.writeRawDxfObjectWithStringAndInt32Overrides(
                &object, stringOverrides(), int32Overrides());
        else
            accepted = codec.writeRawDxfObjectWithStringOverrides(
                &object, stringOverrides());
        reached = static_cast<long>(allocationAttempts);
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    dx_data storage;
    DRW_RawDxfObject object;
    RawWriteMode mode;
    InvalidOverride invalidOverride = InvalidOverride::None;
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
        if (value.name == "XRECORD")
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
                              RawWriteMode mode) {
    const bool expectStringOverrides = mode != RawWriteMode::Plain;
    const bool expectInt32Overrides =
        mode == RawWriteMode::StringAndInt32Overrides;
    return object.name == "XRECORD" && object.handle == 0xD8A0u
        && object.parentHandle == 0xCu && object.groups.size() == 10u
        && object.groups[0].code() == 5
        && object.groups[1].code() == 330
        && object.groups[2].code() == 100
        && object.groups[3].code() == 1
        && object.groups[3].type() == DRW_Variant::STRING
        && object.groups[3].c_str() != nullptr
        && std::string(object.groups[3].c_str())
            == (expectStringOverrides ? "replacement first value"
                                      : "original first value")
        && object.groups[4].code() == 3
        && object.groups[4].type() == DRW_Variant::STRING
        && object.groups[4].c_str() != nullptr
        && std::string(object.groups[4].c_str())
            == (expectStringOverrides ? "replacement second value"
                                      : "original second value")
        && object.groups[5].code() == 2
        && object.groups[5].type() == DRW_Variant::STRING
        && object.groups[5].c_str() != nullptr
        && std::string(object.groups[5].c_str())
            == (expectStringOverrides ? "replacement symbol name"
                                      : "original symbol name")
        && object.groups[6].code() == 6
        && object.groups[6].type() == DRW_Variant::STRING
        && object.groups[6].c_str() != nullptr
        && std::string(object.groups[6].c_str())
            == (expectStringOverrides ? "replacement linetype"
                                      : "original linetype")
        && object.groups[7].code() == 300
        && object.groups[7].type() == DRW_Variant::STRING
        && object.groups[7].c_str() != nullptr
        && std::string(object.groups[7].c_str())
            == (expectStringOverrides ? "replacement extended string"
                                      : "original extended string")
        && object.groups[8].code() == 90
        && object.groups[8].type() == DRW_Variant::INTEGER
        && object.groups[8].i_val() == (expectInt32Overrides ? 2718 : 314)
        && object.groups[9].code() == 91
        && object.groups[9].type() == DRW_Variant::INTEGER
        && object.groups[9].i_val() == (expectInt32Overrides ? -91 : 17);
}

bool readbackMatches(const std::filesystem::path& path,
                     RawWriteMode mode) {
    dxfRW codec(path.string().c_str());
    Readback reader;
    const bool read = codec.read(&reader, false);
    const bool matched = read && reader.objects.size() == 1u
        && rawObjectReadbackMatches(reader.objects.front(), mode);
    if (!matched) {
        std::cerr << "raw XRECORD readback diagnostics: read=" << read
                  << " objects=" << reader.objects.size()
                  << " mode=" << static_cast<unsigned>(mode) << '\n';
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

bool runInvalidOverrideRetries(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "raw DXF object invalid override retry: " << message
                  << '\n';
        return false;
    };
    constexpr DRW::Version version = DRW::AC1024;
    const std::string sentinel = "prior invalid-override destination bytes\n";
    const std::pair<InvalidOverride, RawWriteMode> cases[] = {
        {InvalidOverride::StringOnInteger, RawWriteMode::StringOverrides},
        {InvalidOverride::Int32OnString,
         RawWriteMode::StringAndInt32Overrides}};
    for (const bool binary : {false, true}) {
        for (const auto& test : cases) {
            const auto output = directory
                / (std::string("raw-xrecord-invalid-override-")
                   + (test.first == InvalidOverride::StringOnInteger
                          ? "string-on-integer-" : "int32-on-string-")
                   + (binary ? "binary.dxf" : "ascii.dxf"));
            std::string expectedBytes;
            {
                dxfRW calibration(output.string().c_str());
                Export exporter(calibration, version, test.second);
                if (!calibration.write(&exporter, version, binary)
                    || !exporter.accepted
                    || !readbackMatches(output, test.second)
                    || !readBytes(output, expectedBytes))
                    return fail("valid calibration output did not read back");
            }
            {
                std::ofstream prior(output,
                                   std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior)
                    return fail("could not initialize destination sentinel");
            }
            dxfRW codec(output.string().c_str());
            Export exporter(codec, version, test.second);
            const DRW_RawDxfObject source = exporter.object;
            exporter.invalidOverride = test.first;
            const bool rejected = !codec.write(&exporter, version, binary)
                && !exporter.accepted;
            std::string afterRefusal;
            if (!rejected || !readBytes(output, afterRefusal)
                || afterRefusal != sentinel
                || !rawObjectMatches(exporter.object, source))
                return fail("invalid override changed output or caller state");

            exporter.invalidOverride = InvalidOverride::None;
            exporter.accepted = false;
            const bool retried = codec.write(&exporter, version, binary);
            std::string retryBytes;
            if (!retried || !exporter.accepted
                || !readBytes(output, retryBytes)
                || retryBytes != expectedBytes
                || !readbackMatches(output, test.second)
                || !rawObjectMatches(exporter.object, source))
                return fail("corrected same-codec retry did not match output");
            std::cout << "raw XRECORD invalid override "
                      << (test.first == InvalidOverride::StringOnInteger
                              ? "string-on-integer" : "int32-on-string")
                      << ' ' << (binary ? "Binary" : "ASCII")
                      << ": refusal rollback/retry/readback=PASS\n";
        }
    }
    return true;
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
             DRW::AC1014, DRW::AC1015, DRW::AC1018, DRW::AC1021,
             DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
      for (const bool binary : {false, true}) {
        for (const RawWriteMode mode : {RawWriteMode::Plain,
                                        RawWriteMode::StringOverrides,
                                        RawWriteMode::StringAndInt32Overrides}) {
            const char* const profile = mode == RawWriteMode::Plain
                ? "raw-xrecord-plain-"
                : (mode == RawWriteMode::StringAndInt32Overrides
                    ? "raw-xrecord-string-int32-overrides-"
                    : "raw-xrecord-string-overrides-");
            const std::string outputName = std::string(profile)
                + std::to_string(static_cast<int>(version))
                + (binary ? "-binary.dxf" : "-ascii.dxf");
            const auto output = directory / outputName;
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                Export exporter(codec, version, mode);
                exporter.failAt = 1000000;
                const bool wrote = codec.write(&exporter, version, binary);
                if (!wrote || !exporter.accepted
                    || !readbackMatches(output, mode))
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
                Export exporter(codec, version, mode);
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
                    || !readbackMatches(output, mode)
                    || !rawObjectMatches(exporter.object, source))
                    return fail("same-codec retry failed exact bytes/readback");
            }
            std::cout << "raw XRECORD version="
                      << static_cast<int>(version) << ' '
                      << (mode == RawWriteMode::Plain ? "plain"
                          : (mode == RawWriteMode::StringAndInt32Overrides
                              ? "string+Int32" : "string"))
                      << (mode == RawWriteMode::Plain ? " " : " overrides ")
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
      }
    }
    return runInvalidOverrideRetries(directory);
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
