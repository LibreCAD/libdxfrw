// SPDX-License-Identifier: GPL-2.0-or-later
// Generated whole-operation DXF write allocation rollback/retry control.
#include "dx_iface.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>

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
    explicit Export(dxfRW& output) : output(output) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &output;
        line.handle = 0xD900u;
        line.basePoint = DRW_Coord(1.25, -2.5, 3.75);
        line.secPoint = DRW_Coord(4.5, 5.25, -6.0);
        object.name = "XRECORD";
        object.handle = 0xD901u;
        object.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        object.m_version = DRW::AC1027;
        object.groups.emplace_back(5, std::string("D901"));
        object.groups.emplace_back(330, std::string("C"));
        object.groups.emplace_back(100, std::string("AcDbXrecord"));
        object.groups.emplace_back(1, std::string("whole operation payload"));
    }

    void writeEntities() override {
        entitiesWritten = output.writeLine(&line);
    }

    void writeObjects() override {
        objectsWritten = output.writeRawDxfObject(&object);
    }

    bool finalizeDxfWrite() override {
        return entitiesWritten && objectsWritten;
    }

    dxfRW& output;
    dx_data storage;
    DRW_Line line;
    DRW_RawDxfObject object;
    bool entitiesWritten = false;
    bool objectsWritten = false;
};

class Readback final : public dx_iface {
public:
    Readback() {
        cData = &storage;
        currentBlock = storage.mBlock;
    }
    void addLine(const DRW_Line& value) override { ++lineCount; line = value; }
    void addRawDxfObject(const DRW_RawDxfObject& value) override {
        ++objectCount;
        object = value;
    }
    dx_data storage;
    DRW_Line line;
    DRW_RawDxfObject object;
    unsigned lineCount = 0u;
    unsigned objectCount = 0u;
};

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

bool callerRecordsUnchanged(const Export& adapter) {
    return adapter.line.handle == 0xD900u
        && adapter.line.basePoint.x == 1.25
        && adapter.line.basePoint.y == -2.5
        && adapter.line.basePoint.z == 3.75
        && adapter.line.secPoint.x == 4.5
        && adapter.line.secPoint.y == 5.25
        && adapter.line.secPoint.z == -6.0
        && adapter.object.name == "XRECORD"
        && adapter.object.handle == 0xD901u
        && adapter.object.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
        && adapter.object.groups.size() == 4u
        && adapter.object.groups[0].code() == 5
        && adapter.object.groups[0].c_str() != nullptr
        && std::string(adapter.object.groups[0].c_str()) == "D901"
        && adapter.object.groups[3].code() == 1
        && adapter.object.groups[3].c_str() != nullptr
        && std::string(adapter.object.groups[3].c_str())
            == "whole operation payload";
}

bool matchesReadback(const std::filesystem::path& path) {
    dxfRW reader(path.string().c_str());
    Readback adapter;
    const bool read = reader.read(&adapter, false);
    if (!read || adapter.lineCount != 1u
        || adapter.objectCount != 1u
        || adapter.line.basePoint.x != 1.25
        || adapter.line.secPoint.z != -6.0
        || adapter.object.name != "XRECORD"
        || adapter.object.groups.size() != 4u
        || adapter.object.groups[3].c_str() == nullptr
        || std::string(adapter.object.groups[3].c_str())
            != "whole operation payload") {
        std::cerr << "readback read=" << read << " version=" << reader.getVersion()
                  << " lines=" << adapter.lineCount << " objects="
                  << adapter.objectCount << " lineHandle=" << adapter.line.handle
                  << " firstX=" << adapter.line.basePoint.x << " secondZ="
                  << adapter.line.secPoint.z << " rawGroups="
                  << adapter.object.groups.size();
        if (adapter.object.groups.size() > 3u
            && adapter.object.groups[3].c_str() != nullptr)
            std::cerr << " rawPayload=" << adapter.object.groups[3].c_str();
        std::cerr << '\n';
        return false;
    }
    return true;
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "whole-operation DXF allocation transaction: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    const auto output = directory / "whole-operation.dxf";
    const std::string sentinel = "previous complete drawing\n";
    for (const bool binary : {false, true}) {
        // Flush any one-time writer/codec setup allocations before counting
        // allocations belonging to a normal operation.
        {
            dxfRW codec(output.string().c_str());
            Export adapter(codec);
            if (!codec.write(&adapter, DRW::AC1027, binary))
                return fail("warm-up write failed");
        }
        std::error_code warmupError;
        std::filesystem::remove(output, warmupError);

        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW codec(output.string().c_str());
            Export adapter(codec);
            armAllocationFailure(1000000000L);
            const bool wrote = codec.write(&adapter, DRW::AC1027, binary);
            allocationCount = static_cast<long>(allocationAttempts);
            disarmAllocationFailure();
            const bool readback = matchesReadback(output);
            const bool bytesRead = readBytes(output, expectedBytes);
            if (!wrote || !adapter.entitiesWritten || !adapter.objectsWritten
                || !readback || !bytesRead) {
                std::cerr << "calibration wrote=" << wrote
                          << " entities=" << adapter.entitiesWritten
                          << " objects=" << adapter.objectsWritten
                          << " readback=" << readback
                          << " bytes=" << bytesRead
                          << " error=" << codec.getError() << '\n';
                return fail("calibration write or readback failed");
            }
        }
        if (allocationCount <= 0)
            return fail("calibration observed no operation allocations");

        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream prior(output,
                                   std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior)
                    return fail("could not initialize destination sentinel");
            }

            dxfRW codec(output.string().c_str());
            Export adapter(codec);
            armAllocationFailure(offset);
            const bool wrote = codec.write(&adapter, DRW::AC1027, binary);
            const std::size_t reached = allocationAttempts;
            const bool injected = allocationFailed;
            disarmAllocationFailure();
            if (!injected || wrote || reached != static_cast<std::size_t>(offset + 1)) {
                std::cerr << "offset=" << offset << " attempts=" << reached
                          << " fired=" << injected << " wrote=" << wrote
                          << '\n';
                return fail("allocation refusal was not atomic");
            }

            std::string afterFailure;
            if (!readBytes(output, afterFailure) || afterFailure != sentinel
                || !callerRecordsUnchanged(adapter))
                return fail("failure changed destination or caller records");

            const bool retried = codec.write(&adapter, DRW::AC1027, binary);
            std::string retryBytes;
            if (!retried || !adapter.entitiesWritten || !adapter.objectsWritten
                || !readBytes(output, retryBytes) || retryBytes != expectedBytes
                || !matchesReadback(output))
                return fail("same-codec retry did not reproduce clean output");
        }
        std::cout << "whole DXF write " << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", destination/caller rollback, retry/readback=PASS\n";
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
        std::cerr << "usage: dxf_write_allocation_transaction_tests SCRATCH_DIR\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
