// SPDX-License-Identifier: GPL-2.0-or-later
// Generated MLEADERSTYLE DXF OBJECTS allocation rollback/retry control.
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

DRW_MLeaderStyle makeStyle(DRW::Version version) {
    DRW_MLeaderStyle style;
    style.handle = 0xD710u;
    style.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
    style.styleVersion = 2;
    style.description = "allocation sweep style description";
    style.textDefault = "allocation sweep default text";
    style.textHeight = 2.75;
    if (version >= DRW::AC1024) {
        style.attachmentDirection = 1;
        style.topAttachment = 10;
        style.bottomAttachment = 9;
    }
    style.textExtended = version >= DRW::AC1027;
    style.extData.push_back(new DRW_Variant(1001, std::string("ACAD")));
    style.extData.push_back(new DRW_Variant(
        1000, std::string("mleaderstyle-eed")));
    return style;
}

class Export final : public LC3CompatCheck {
public:
    Export(dxfRW& codec, DRW::Version version)
        : codec(codec), version(version), style(makeStyle(version)) {}

    void writeObjects() override {
        if (failAt < 0) {
            accepted = codec.writeMLeaderStyle(&style);
        } else {
            armAllocationFailure(failAt);
            try {
                accepted = codec.writeMLeaderStyle(&style);
            } catch (const std::bad_alloc&) {
                accepted = false;
            }
            reached = static_cast<long>(allocationAttempts);
            disarmAllocationFailure();
        }
        if (!accepted)
            (void)codec.writeRawDxfObject(nullptr);
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    DRW::Version version;
    DRW_MLeaderStyle style;
    long failAt = -1;
    long reached = 0;
    bool accepted = false;
};

class Reader final : public LC3CompatCheck {
public:
    void addMLeaderStyle(const DRW_MLeaderStyle* value) override {
        if (value)
            styles.push_back(*value);
    }
    std::vector<DRW_MLeaderStyle> styles;
};

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

bool callerMatches(const DRW_MLeaderStyle& style, DRW::Version version) {
    const DRW_MLeaderStyle expected = makeStyle(version);
    return style.tType == DRW::MLEADERSTYLE
        && style.handle == 0xD710u
        && style.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
        && style.styleVersion == 2
        && style.description == "allocation sweep style description"
        && style.textDefault == "allocation sweep default text"
        && style.textHeight == 2.75
        && style.attachmentDirection == expected.attachmentDirection
        && style.topAttachment == expected.topAttachment
        && style.bottomAttachment == expected.bottomAttachment
        && style.textExtended == expected.textExtended
        && style.extData.size() == 2u
        && style.extData[0]
        && style.extData[0]->code() == 1001
        && style.extData[0]->content.s
        && *style.extData[0]->content.s == "ACAD"
        && style.extData[1]
        && style.extData[1]->code() == 1000
        && style.extData[1]->content.s
        && *style.extData[1]->content.s == "mleaderstyle-eed";
}

bool readbackMatches(const std::filesystem::path& path,
                     DRW::Version version) {
    dxfRW codec(path.string().c_str());
    Reader reader;
    if (!codec.read(&reader, false) || reader.styles.size() != 1u)
        return false;
    const DRW_MLeaderStyle& style = reader.styles.front();
    return callerMatches(style, version);
}

const char* versionCode(DRW::Version version) {
    switch (version) {
    case DRW::AC1021: return "AC1021";
    case DRW::AC1024: return "AC1024";
    case DRW::AC1027: return "AC1027";
    case DRW::AC1032: return "AC1032";
    default: return "unknown";
    }
}

bool allocationSweep(const std::filesystem::path& directory) {
    const auto path = directory / "mleaderstyle-allocation.dxf";
    const std::string prior = "prior MLEADERSTYLE bytes\n";
    constexpr std::array<DRW::Version, 4> versions{{
        DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}};
    for (const DRW::Version version : versions)
    for (const bool binary : {false, true}) {
        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW codec(path.string().c_str());
            Export exporter(codec, version);
            exporter.failAt = 1000000;
            const bool written = codec.write(&exporter, version, binary);
            if (!written || !exporter.accepted
                || !readbackMatches(path, version)
                || !callerMatches(exporter.style, version)
                || !readBytes(path, expectedBytes))
                return false;
            allocationCount = exporter.reached;
        }
        if (allocationCount <= 0)
            return false;

        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream output(path, std::ios::binary | std::ios::trunc);
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
            if (written || exporter.accepted || exporter.reached != offset + 1
                || !allocationFailed
                || !readBytes(path, current) || current != prior
                || !callerMatches(exporter.style, version))
                return false;

            const bool retried = codec.write(&exporter, version, binary);
            std::string retryBytes;
            if (!retried || !exporter.accepted
                || !callerMatches(exporter.style, version)
                || !readBytes(path, retryBytes)
                || retryBytes != expectedBytes
                || !readbackMatches(path, version))
                return false;
        }
        std::cout << "MLEADERSTYLE " << versionCode(version)
                  << (binary ? " Binary" : " ASCII")
                  << " writer allocation offsets="
                  << allocationCount << '\n';
    }
    const auto refusalPath = directory / "mleaderstyle-pre-ac1021-refusal.dxf";
    const std::string priorUnsupported =
        "prior unsupported MLEADERSTYLE destination\n";
    {
        std::ofstream output(refusalPath, std::ios::binary | std::ios::trunc);
        output << priorUnsupported;
        if (!output)
            return false;
    }
    dxfRW codec(refusalPath.string().c_str());
    Export exporter(codec, DRW::AC1021);
    const bool written = codec.write(&exporter, DRW::AC1018, false);
    std::string current;
    if (written || exporter.accepted
        || !callerMatches(exporter.style, DRW::AC1021)
        || !readBytes(refusalPath, current) || current != priorUnsupported)
        return false;
    std::cout << "MLEADERSTYLE AC1018 refusal preserves caller and destination\n";
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

int main() {
    const auto directory = std::filesystem::temp_directory_path()
        / ("mleaderstyle-objects-allocation-"
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
