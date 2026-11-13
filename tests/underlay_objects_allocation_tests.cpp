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

class UnderlayExport final : public dx_iface {
public:
    long failAt = -1;
    long reached = 0;

    void writeObjects() override {
        if (failAt < 0) {
            dx_iface::writeObjects();
            return;
        }
        armAllocationFailure(failAt);
        try {
            dx_iface::writeObjects();
        } catch (const std::bad_alloc&) {
            reached = static_cast<long>(allocationAttempts);
            disarmAllocationFailure();
            throw;
        }
        reached = static_cast<long>(allocationAttempts);
        disarmAllocationFailure();
    }
};

bool readAll(const std::filesystem::path& path, std::string& value) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    value.assign(std::istreambuf_iterator<char>(input),
                 std::istreambuf_iterator<char>());
    return !input.bad();
}

bool verifyGraph(const std::filesystem::path& path) {
    dx_data data;
    dx_iface reader;
    if (!reader.fileImport(path.string(), &data, false)
        || data.underlayDefinitions.size() != 3u
        || data.underlays.size() != 3u)
        return false;
    for (const DRW_Underlay* underlay : data.underlays) {
        if (underlay == nullptr)
            return false;
        const auto definition = std::find_if(data.underlayDefinitions.begin(),
            data.underlayDefinitions.end(), [&](const auto& entry) {
                return entry.second.handle == underlay->definitionHandle;
            });
        if (definition == data.underlayDefinitions.end()
            || static_cast<int>(definition->second.kind)
                != static_cast<int>(underlay->kind))
            return false;
    }
    return true;
}

bool run(const std::filesystem::path& directory, bool binary) {
    const auto fail = [](const char* message) {
        std::cerr << "underlay OBJECTS allocation retry: " << message << '\n';
        return false;
    };
    dx_data data;
    UnderlayExport exporter;
    exporter.cData = &data;
    exporter.currentBlock = data.mBlock;
    data.hasNamedObjectDictionary = true;
    data.namedObjectDictionary.handle = 0xC700u;
    constexpr std::array<DRW_Underlay::Kind, 3> kinds{{
        DRW_Underlay::PDF, DRW_Underlay::DGN, DRW_Underlay::DWF}};
    constexpr std::array<const char*, 3> rootNames{{
        "ACAD_PDFDEFINITIONS", "ACAD_DGNDEFINITIONS",
        "ACAD_DWFDEFINITIONS"}};
    for (std::size_t index = 0; index < kinds.size(); ++index) {
        const auto dictionaryHandle = static_cast<std::uint32_t>(0xC701u + index);
        const auto definitionHandle = static_cast<std::uint32_t>(0xC710u + index);
        data.namedObjectDictionary.m_entries.push_back(
            {rootNames[index], dictionaryHandle});

        DRW_Dictionary dictionary;
        dictionary.handle = dictionaryHandle;
        dictionary.parentHandle = data.namedObjectDictionary.handle;
        dictionary.hardOwner = 1;
        dictionary.cloning = 2;
        dictionary.m_entries = {{"SOURCE_UNDERLAY", definitionHandle}};
        data.dictionaries.push_back(std::move(dictionary));

        DRW_UnderlayDefinition definition;
        definition.handle = definitionHandle;
        definition.parentHandle = dictionaryHandle;
        definition.kind = static_cast<DRW_UnderlayDefinition::Kind>(kinds[index]);
        definition.filename = std::string("underlay-")
            + std::to_string(index) + ".source";
        definition.sheetName = std::string("Sheet") + std::to_string(index + 1u);
        exporter.linkUnderlay(&definition);

        DRW_Underlay underlay;
        underlay.handle = static_cast<std::uint32_t>(0xC720u + index);
        underlay.kind = kinds[index];
        underlay.definitionHandle = definition.handle;
        underlay.position = DRW_Coord(10.0 + index, 20.0 + index, 0.0);
        underlay.scale = DRW_Coord(1.0, 1.0, 1.0);
        underlay.extPoint = DRW_Coord(0.0, 0.0, 1.0);
        underlay.clipBoundary = {DRW_Coord(0.0, 0.0), DRW_Coord(100.0, 80.0)};
        exporter.addUnderlay(&underlay);
    }

    const auto destination = directory
        / (binary ? "underlay-objects-binary.dxf" : "underlay-objects-ascii.dxf");
    const std::string sentinel = "preexisting destination bytes\n";
    {
        std::ofstream output(destination, std::ios::binary);
        output.write(sentinel.data(), static_cast<std::streamsize>(sentinel.size()));
        if (!output)
            return fail("could not initialize destination sentinel");
    }

    const auto write = [&]() {
        return exporter.fileExport(destination.string(), DRW::AC1027, binary,
                                   &data, false, true);
    };
    exporter.failAt = 1000000;
    if (!write())
        return fail("allocation calibration export failed");
    const long allocationCount = exporter.reached;
    if (allocationCount <= 0 || !verifyGraph(destination))
        return fail("calibration did not produce the expected graph");

    unsigned failures = 0u;
    for (long offset = 0; offset < allocationCount; ++offset) {
        {
            std::ofstream output(destination, std::ios::binary | std::ios::trunc);
            output.write(sentinel.data(), static_cast<std::streamsize>(sentinel.size()));
            if (!output)
                return fail("could not reset destination sentinel");
        }
        exporter.failAt = offset;
        exporter.reached = 0;
        const bool wrote = write();
        exporter.failAt = -1;
        std::string current;
        if (wrote || exporter.reached != offset + 1 || !allocationFailed
            || !readAll(destination, current) || current != sentinel) {
            std::cerr << "offset=" << offset << " wrote=" << wrote
                      << " reached=" << exporter.reached
                      << " current-size=" << current.size()
                      << " sentinel-size=" << sentinel.size()
                      << " injected=" << allocationFailed
                      << " attempts=" << allocationAttempts << '\n';
            return fail("injected failure did not preserve the prior destination");
        }
        if (!write() || !verifyGraph(destination))
            return fail("same-adapter retry failed typed graph readback");
        ++failures;
    }
    if (failures != static_cast<unsigned>(allocationCount))
        return fail("not every calibrated callback allocation was injected");
    std::cout << "underlay OBJECTS " << (binary ? "binary" : "ASCII")
              << ": allocations=" << allocationCount
              << ", rollback/retry/readback=" << failures << '\n';
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
        std::cerr << "usage: underlay_objects_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    const std::filesystem::path directory(argv[1]);
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        std::cerr << "could not create scratch directory: " << error.message() << '\n';
        return 2;
    }
    return run(directory, false) && run(directory, true) ? 0 : 1;
}
