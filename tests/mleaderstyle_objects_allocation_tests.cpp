// SPDX-License-Identifier: GPL-2.0-or-later
// Generated MLEADERSTYLE DXF OBJECTS allocation rollback/retry control.
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"

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

DRW_MLeaderStyle makeStyle() {
    DRW_MLeaderStyle style;
    style.handle = 0xD710u;
    style.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
    style.styleVersion = 2;
    style.description = "allocation sweep style description";
    style.textDefault = "allocation sweep default text";
    style.textHeight = 2.75;
    style.extData.push_back(new DRW_Variant(1001, std::string("ACAD")));
    style.extData.push_back(new DRW_Variant(
        1000, std::string("mleaderstyle-eed")));
    return style;
}

class Export final : public LC3CompatCheck {
public:
    explicit Export(dxfRW& codec) : codec(codec), style(makeStyle()) {}

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

bool readbackMatches(const std::filesystem::path& path) {
    dxfRW codec(path.string().c_str());
    Reader reader;
    if (!codec.read(&reader, false) || reader.styles.size() != 1u)
        return false;
    const DRW_MLeaderStyle& style = reader.styles.front();
    return style.handle == 0xD710u
        && style.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
        && style.styleVersion == 2
        && style.description == "allocation sweep style description"
        && style.textDefault == "allocation sweep default text"
        && style.textHeight == 2.75
        && style.extData.size() == 2u
        && style.extData[0]
        && style.extData[0]->content.s
        && *style.extData[0]->content.s == "ACAD"
        && style.extData[1]
        && style.extData[1]->content.s
        && *style.extData[1]->content.s == "mleaderstyle-eed";
}

bool allocationSweep(const std::filesystem::path& directory) {
    const auto path = directory / "mleaderstyle-allocation.dxf";
    const std::string prior = "prior MLEADERSTYLE bytes\n";
    for (const bool binary : {false, true}) {
        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW codec(path.string().c_str());
            Export exporter(codec);
            exporter.failAt = 1000000;
            const bool written = codec.write(&exporter, DRW::AC1027, binary);
            if (!written || !exporter.accepted || !readbackMatches(path)
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
            Export exporter(codec);
            exporter.failAt = offset;
            const bool written = codec.write(&exporter, DRW::AC1027, binary);
            exporter.failAt = -1;
            std::string current;
            if (written || exporter.accepted || exporter.reached != offset + 1
                || !allocationFailed
                || !readBytes(path, current) || current != prior
                || exporter.style.handle != 0xD710u
                || exporter.style.parentHandle
                    != DRW::DwgNamedObjectsDictionaryHandle
                || exporter.style.description
                    != "allocation sweep style description"
                || exporter.style.textDefault
                    != "allocation sweep default text"
                || exporter.style.extData.size() != 2u
                || !exporter.style.extData[0]
                || !exporter.style.extData[0]->content.s
                || *exporter.style.extData[0]->content.s != "ACAD"
                || !exporter.style.extData[1]
                || !exporter.style.extData[1]->content.s
                || *exporter.style.extData[1]->content.s
                    != "mleaderstyle-eed")
                return false;

            const bool retried = codec.write(&exporter, DRW::AC1027, binary);
            std::string retryBytes;
            if (!retried || !exporter.accepted
                || !readBytes(path, retryBytes)
                || retryBytes != expectedBytes || !readbackMatches(path))
                return false;
        }
        std::cout << (binary ? "Binary" : "ASCII")
                  << " MLEADERSTYLE writer allocation offsets="
                  << allocationCount << '\n';
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
