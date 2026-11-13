// SPDX-License-Identifier: GPL-2.0-or-later
// Generated UCS table-record allocation rollback/retry control.
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"

#include <array>
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

void armAllocationFailure(long offset) noexcept {
    allocationsBeforeFailure = offset;
    allocationAttempts = 0u;
    allocationFailed = false;
}

void disarmAllocationFailure() noexcept {
    allocationsBeforeFailure = -1;
}

DRW_UCS makeUcs(DRW::Version version) {
    DRW_UCS ucs;
    ucs.handle = 0xD730u;
    ucs.parentHandle = 7u;
    ucs.name = "Generated allocation retry UCS";
    ucs.flags = 0;
    ucs.origin = DRW_Coord(1.25, -2.5, 3.75);
    ucs.xAxisDirection = DRW_Coord(1.0, 0.0, 0.0);
    ucs.yAxisDirection = DRW_Coord(0.0, 1.0, 0.0);
    ucs.extData.push_back(new DRW_Variant(1001, std::string("ACAD")));
    ucs.extData.push_back(new DRW_Variant(
        1000, std::string("ucs-allocation-retry")));
    if (version > DRW::AC1014) {
        ucs.elevation = 4.5;
        const DRW_Coord front(-1.0, -2.0, -3.0);
        const DRW_Coord back(7.0, 8.0, 9.0);
        ucs.orthoOrigin = front;
        ucs.orthographicOptions.push_back({1, front});
        ucs.orthographicOptions.push_back({6, back});
    }
    return ucs;
}

bool sameUcs(const DRW_UCS& actual, DRW::Version version) {
    const DRW_UCS expected = makeUcs(version);
    if (actual.tType != DRW::UCS || actual.handle != expected.handle
        || actual.parentHandle != expected.parentHandle
        || actual.name != expected.name || actual.flags != expected.flags
        || actual.origin.x != expected.origin.x
        || actual.origin.y != expected.origin.y
        || actual.origin.z != expected.origin.z
        || actual.xAxisDirection.x != expected.xAxisDirection.x
        || actual.xAxisDirection.y != expected.xAxisDirection.y
        || actual.xAxisDirection.z != expected.xAxisDirection.z
        || actual.yAxisDirection.x != expected.yAxisDirection.x
        || actual.yAxisDirection.y != expected.yAxisDirection.y
        || actual.yAxisDirection.z != expected.yAxisDirection.z
        || actual.elevation != expected.elevation
        || actual.orthoType != expected.orthoType
        || actual.orthoOrigin.x != expected.orthoOrigin.x
        || actual.orthoOrigin.y != expected.orthoOrigin.y
        || actual.orthoOrigin.z != expected.orthoOrigin.z
        || actual.orthographicOptions.size()
            != expected.orthographicOptions.size()
        || actual.extData.size() != expected.extData.size())
        return false;
    for (std::size_t i = 0; i < expected.orthographicOptions.size(); ++i) {
        const auto& left = actual.orthographicOptions[i];
        const auto& right = expected.orthographicOptions[i];
        if (left.type != right.type || left.point.x != right.point.x
            || left.point.y != right.point.y || left.point.z != right.point.z)
            return false;
    }
    for (std::size_t i = 0; i < expected.extData.size(); ++i) {
        if (!actual.extData[i] || !expected.extData[i]
            || actual.extData[i]->code() != expected.extData[i]->code()
            || !actual.extData[i]->content.s
            || !expected.extData[i]->content.s
            || *actual.extData[i]->content.s
                != *expected.extData[i]->content.s)
            return false;
    }
    return true;
}

class Export final : public LC3CompatCheck {
public:
    Export(dxfRW& codec, DRW::Version version)
        : codec(codec), version(version), ucs(makeUcs(version)) {}

    void writeUCSs() override {
        if (failAt < 0) {
            accepted = codec.writeUCS(&ucs);
            return;
        }
        armAllocationFailure(failAt);
        try {
            accepted = codec.writeUCS(&ucs);
        } catch (const std::bad_alloc&) {
            accepted = false;
        }
        reached = static_cast<long>(allocationAttempts);
        disarmAllocationFailure();
        if (!accepted)
            (void)codec.writeRawDxfObject(nullptr);
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    DRW::Version version;
    DRW_UCS ucs;
    long failAt = -1;
    long reached = 0;
    bool accepted = false;
};

class Reader final : public LC3CompatCheck {
public:
    void addUCS(const DRW_UCS& value) override { ucs.push_back(value); }
    std::vector<DRW_UCS> ucs;
};

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

const char* versionCode(DRW::Version version) {
    switch (version) {
    case DRW::AC1014: return "AC1014";
    case DRW::AC1015: return "AC1015";
    case DRW::AC1021: return "AC1021";
    case DRW::AC1032: return "AC1032";
    default: return "unknown";
    }
}

bool readbackMatches(const std::filesystem::path& path,
                     DRW::Version version) {
    dxfRW codec(path.string().c_str());
    Reader reader;
    return codec.read(&reader, false) && reader.ucs.size() == 1u
        && sameUcs(reader.ucs.front(), version);
}

bool allocationSweep(const std::filesystem::path& directory) {
    const auto path = directory / "ucs-table-allocation.dxf";
    const std::string prior = "prior UCS destination bytes\n";
    constexpr std::array<DRW::Version, 4> versions{{
        DRW::AC1014, DRW::AC1015, DRW::AC1021, DRW::AC1032}};
    for (const DRW::Version version : versions) {
        for (const bool binary : {false, true}) {
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(path.string().c_str());
                Export exporter(codec, version);
                exporter.failAt = 1000000;
                const bool written = codec.write(&exporter, version, binary);
                const bool sourceMatches = sameUcs(exporter.ucs, version);
                const bool readback = readbackMatches(path, version);
                if (!written || !exporter.accepted || !sourceMatches
                    || !readback || !readBytes(path, expectedBytes)) {
                    std::cerr << "clean UCS write failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " written=" << written
                              << " accepted=" << exporter.accepted
                              << " source=" << sourceMatches
                              << " readback=" << readback << '\n';
                    return false;
                }
                allocationCount = exporter.reached;
            }
            if (allocationCount <= 0)
                return false;

            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream output(path,
                                         std::ios::binary | std::ios::trunc);
                    output << prior;
                    if (!output)
                        return false;
                }
                dxfRW codec(path.string().c_str());
                Export exporter(codec, version);
                exporter.failAt = offset;
                const bool written = codec.write(&exporter, version, binary);
                exporter.failAt = -1;
                std::string current;
                if (written || exporter.accepted
                    || exporter.reached != offset + 1
                    || !allocationFailed
                    || !readBytes(path, current) || current != prior
                    || !sameUcs(exporter.ucs, version)) {
                    std::cerr << "UCS allocation rollback failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " offset=" << offset << " written=" << written
                              << " accepted=" << exporter.accepted
                              << " reached=" << exporter.reached
                              << " failed=" << allocationFailed << '\n';
                    return false;
                }

                const bool retried = codec.write(&exporter, version, binary);
                std::string retryBytes;
                if (!retried || !exporter.accepted
                    || !sameUcs(exporter.ucs, version)
                    || !readBytes(path, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(path, version)) {
                    std::cerr << "UCS retry failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " offset=" << offset << '\n';
                    return false;
                }
            }
            std::cout << "UCS " << versionCode(version)
                      << (binary ? " Binary" : " ASCII")
                      << " table-writer allocation offsets="
                      << allocationCount << '\n';
        }
    }
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (allocationsBeforeFailure == 0) {
        ++allocationAttempts;
        allocationsBeforeFailure = -1;
        allocationFailed = true;
        throw std::bad_alloc();
    }
    if (allocationsBeforeFailure > 0) {
        --allocationsBeforeFailure;
        ++allocationAttempts;
    }
    if (void* value = std::malloc(size == 0u ? 1u : size))
        return value;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete(void* value, std::size_t) noexcept { std::free(value); }
void operator delete[](void* value, std::size_t) noexcept { std::free(value); }

int main() {
    const auto directory = std::filesystem::temp_directory_path()
        / ("ucs-table-allocation-"
           + std::to_string(static_cast<long long>(
                 std::filesystem::file_time_type::clock::now()
                     .time_since_epoch().count())));
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return 1;
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
    } cleanup{directory};
    return allocationSweep(directory) ? 0 : 2;
}
