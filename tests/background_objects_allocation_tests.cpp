// SPDX-License-Identifier: GPL-2.0-or-later
// Generated IMAGEBACKGROUND OBJECTS allocation rollback and retry control.
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
std::size_t failedAllocations = 0;

struct AllocationFailure {
    explicit AllocationFailure(long count) { allocationsBeforeFailure = count; }
    ~AllocationFailure() { allocationsBeforeFailure = -1; }
};

class BackgroundExport final : public dx_iface {
public:
    explicit BackgroundExport(dxfRW& codec) : codec(codec) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &codec;
        background.handle = 0xD7B1u;
        background.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        background.m_kind = DRW_Background::Image;
        background.m_classVersion = 1;
        background.m_fileName = "generated/background-image.png";
        background.m_fitToScreen = true;
        background.m_maintainAspect = true;
        background.m_useTiling = false;
        background.m_offset = {0.125, 0.25};
        background.m_scale = {1.5, 2.0};
    }

    void writeObjects() override {
        if (failAt < 0) {
            wrote = codec.writeBackground(&background, "IMAGEBACKGROUND");
            if (!wrote)
                (void)codec.writeRawDxfObject(nullptr);
            return;
        }
        const std::size_t before = failedAllocations;
        AllocationFailure failure(failAt);
        try {
            wrote = codec.writeBackground(&background, "IMAGEBACKGROUND");
        } catch (const std::bad_alloc&) {
            reached = failedAllocations != before
                ? static_cast<long>(failedAllocations - before)
                : failAt - allocationsBeforeFailure;
            throw;
        }
        reached = failedAllocations != before
            ? static_cast<long>(failedAllocations - before)
            : failAt - allocationsBeforeFailure;
        if (!wrote)
            (void)codec.writeRawDxfObject(nullptr);
    }

    bool finalizeDxfWrite() override { return wrote; }

    dxfRW& codec;
    dx_data storage;
    DRW_Background background;
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
    class BackgroundReadback final : public dx_iface {
    public:
        void addBackground(const DRW_Background& value) override {
            records.push_back(value);
        }
        std::vector<DRW_Background> records;
    } reader;
    if (!reader.fileImport(path.string(), &data, false)
        || data.sourceFormat != dx_source_format::DXF
        || reader.records.size() != 1u)
        return false;
    const DRW_Background& value = reader.records.front();
    return value.handle == 0xD7B1u
        && value.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
        && value.m_kind == DRW_Background::Image
        && value.m_classVersion == 1
        && value.m_fileName == "generated/background-image.png"
        && value.m_fitToScreen && value.m_maintainAspect
        && !value.m_useTiling
        && value.m_offset.x == 0.125 && value.m_offset.y == 0.25
        && value.m_scale.x == 1.5 && value.m_scale.y == 2.0;
}

bool sourceMatches(const DRW_Background& value,
                   const DRW_Background& expected) {
    return value.handle == expected.handle
        && value.parentHandle == expected.parentHandle
        && value.m_kind == expected.m_kind
        && value.m_classVersion == expected.m_classVersion
        && value.m_fileName == expected.m_fileName
        && value.m_fitToScreen == expected.m_fitToScreen
        && value.m_maintainAspect == expected.m_maintainAspect
        && value.m_useTiling == expected.m_useTiling
        && value.m_offset.x == expected.m_offset.x
        && value.m_offset.y == expected.m_offset.y
        && value.m_scale.x == expected.m_scale.x
        && value.m_scale.y == expected.m_scale.y
        && value.reactorHandles == expected.reactorHandles
        && value.extData.size() == expected.extData.size();
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "IMAGEBACKGROUND OBJECTS allocation retry: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    const std::string sentinel = "prior BACKGROUND destination bytes\n";
    for (const DRW::Version version : {
             DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        const auto output = directory
            / (std::string("background-objects-")
               + std::to_string(static_cast<int>(version)) + ".dxf");
        for (const bool binary : {false, true}) {
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                BackgroundExport exporter(codec);
                codec.setDxfTextWritePolicy(DRW::DxfTextWritePolicy::RequireLossless);
                exporter.failAt = 1000000;
                if (!codec.write(&exporter, version, binary)
                    || !exporter.wrote || !readbackMatches(output))
                    return fail("allocation calibration output did not read back");
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
                BackgroundExport exporter(codec);
                codec.setDxfTextWritePolicy(DRW::DxfTextWritePolicy::RequireLossless);
                const DRW_Background source = exporter.background;
                exporter.failAt = offset;
                exporter.reached = 0;
                exporter.wrote = false;
                const bool wrote = codec.write(&exporter, version, binary);
                exporter.failAt = -1;
                std::string current;
                if (wrote || exporter.reached != 1
                    || !readAll(output, current) || current != sentinel)
                    return fail("injected failure changed destination or missed allocation");
                if (!sourceMatches(exporter.background, source))
                    return fail("injected failure changed caller BACKGROUND state");
                const bool retry = codec.write(&exporter, version, binary);
                std::string retryBytes;
                if (!retry || !exporter.wrote || !readAll(output, retryBytes)
                    || retryBytes != expectedBytes || !readbackMatches(output)
                    || !sourceMatches(exporter.background, source))
                    return fail("same-codec retry failed exact bytes or typed readback");
            }
            std::cout << "IMAGEBACKGROUND OBJECTS version="
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
    if (allocationsBeforeFailure == 0) {
        ++failedAllocations;
        allocationsBeforeFailure = -1;
        throw std::bad_alloc();
    }
    if (allocationsBeforeFailure > 0)
        --allocationsBeforeFailure;
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
        std::cerr << "usage: background_objects_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    const std::size_t before = failedAllocations;
    if (!run(argv[1]))
        return 1;
    if (failedAllocations == before) {
        std::cerr << "allocation injector did not reach operator new\n";
        return 1;
    }
    return 0;
}
