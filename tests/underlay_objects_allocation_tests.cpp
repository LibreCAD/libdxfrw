#include "dx_iface.h"

#include <array>
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

bool verifyGraph(const std::filesystem::path& path, std::size_t expectedCount) {
    dx_data data;
    dx_iface reader;
    if (!reader.fileImport(path.string(), &data, false)
        || data.underlayDefinitions.size() != expectedCount
        || data.underlays.size() != expectedCount)
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

bool callerGraphMatches(const dx_data& data, std::size_t expectedCount,
                        bool dwfOnly) {
    if (!data.hasNamedObjectDictionary
        || data.namedObjectDictionary.handle != 0xC700u
        || data.namedObjectDictionary.m_entries.size() != expectedCount
        || data.dictionaries.size() != expectedCount
        || data.underlayDefinitions.size() != expectedCount
        || data.underlays.size() != expectedCount
        || data.underlayDefinitionCaptureFailed)
        return false;
    constexpr std::array<DRW_Underlay::Kind, 3> kinds{{
        DRW_Underlay::PDF, DRW_Underlay::DGN, DRW_Underlay::DWF}};
    constexpr std::array<const char*, 3> rootNames{{
        "ACAD_PDFDEFINITIONS", "ACAD_DGNDEFINITIONS",
        "ACAD_DWFDEFINITIONS"}};
    auto underlayIt = data.underlays.begin();
    for (std::size_t index = 0; index < expectedCount; ++index) {
        const std::size_t kindIndex = dwfOnly ? 2u : index;
        const auto dictionaryHandle = static_cast<std::uint32_t>(0xC701u + index);
        const auto definitionHandle = static_cast<std::uint32_t>(0xC710u + index);
        const auto underlayHandle = static_cast<std::uint32_t>(0xC720u + index);
        const auto root = std::find_if(data.namedObjectDictionary.m_entries.begin(),
            data.namedObjectDictionary.m_entries.end(), [&](const auto& entry) {
                return entry.m_name == rootNames[kindIndex]
                    && entry.m_handle == dictionaryHandle;
            });
        const auto dictionary = std::find_if(data.dictionaries.begin(),
            data.dictionaries.end(), [&](const DRW_Dictionary& value) {
                return value.handle == dictionaryHandle;
            });
        const auto definition = data.underlayDefinitions.find(
            {data.importGeneration, definitionHandle});
        if (root == data.namedObjectDictionary.m_entries.end()
            || dictionary == data.dictionaries.end()
            || dictionary->parentHandle != data.namedObjectDictionary.handle
            || dictionary->hardOwner != 1 || dictionary->cloning != 2
            || dictionary->m_entries.size() != 1u
            || dictionary->m_entries.front().m_name != "SOURCE_UNDERLAY"
            || dictionary->m_entries.front().m_handle != definitionHandle
            || definition == data.underlayDefinitions.end()
            || definition->second.handle != definitionHandle
            || definition->second.parentHandle != dictionaryHandle
            || static_cast<int>(definition->second.kind)
                != static_cast<int>(kinds[kindIndex])
            || definition->second.filename
                != "underlay-" + std::to_string(index) + ".source"
            || definition->second.sheetName
                != "Sheet" + std::to_string(index + 1u))
            return false;
        const dx_ifaceUnderlay* underlay = *underlayIt++;
        if (underlay == nullptr || underlay->handle != underlayHandle
            || underlay->sourceImportGeneration != data.importGeneration
            || static_cast<int>(underlay->kind)
                != static_cast<int>(kinds[kindIndex])
            || underlay->definitionHandle != definitionHandle
            || underlay->position.x != 10.0 + index
            || underlay->position.y != 20.0 + index
            || underlay->position.z != 0.0
            || underlay->scale.x != 1.0 || underlay->scale.y != 1.0
            || underlay->scale.z != 1.0
            || underlay->extPoint.x != 0.0 || underlay->extPoint.y != 0.0
            || underlay->extPoint.z != 1.0
            || underlay->clipBoundary.size() != 2u
            || underlay->clipBoundary[0].x != 0.0
            || underlay->clipBoundary[0].y != 0.0
            || underlay->clipBoundary[1].x != 100.0
            || underlay->clipBoundary[1].y != 80.0)
            return false;
    }
    return true;
}

bool run(const std::filesystem::path& directory, DRW::Version version,
         const char* versionName, bool binary) {
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
    const bool dwfOnly = version == DRW::AC1021;
    const std::size_t profileCount = dwfOnly ? 1u : kinds.size();
    for (std::size_t index = 0; index < profileCount; ++index) {
        const std::size_t kindIndex = dwfOnly ? 2u : index;
        const auto dictionaryHandle = static_cast<std::uint32_t>(0xC701u + index);
        const auto definitionHandle = static_cast<std::uint32_t>(0xC710u + index);
        data.namedObjectDictionary.m_entries.push_back(
            {rootNames[kindIndex], dictionaryHandle});

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
        definition.kind = static_cast<DRW_UnderlayDefinition::Kind>(
            kinds[kindIndex]);
        definition.filename = std::string("underlay-")
            + std::to_string(index) + ".source";
        definition.sheetName = std::string("Sheet") + std::to_string(index + 1u);
        exporter.linkUnderlay(&definition);

        DRW_Underlay underlay;
        underlay.handle = static_cast<std::uint32_t>(0xC720u + index);
        underlay.kind = kinds[kindIndex];
        underlay.definitionHandle = definition.handle;
        underlay.position = DRW_Coord(10.0 + index, 20.0 + index, 0.0);
        underlay.scale = DRW_Coord(1.0, 1.0, 1.0);
        underlay.extPoint = DRW_Coord(0.0, 0.0, 1.0);
        underlay.clipBoundary = {DRW_Coord(0.0, 0.0), DRW_Coord(100.0, 80.0)};
        exporter.addUnderlay(&underlay);
    }

    const auto destination = directory
        / (std::string("underlay-objects-") + versionName
            + (binary ? "-binary.dxf" : "-ascii.dxf"));
    const std::string sentinel = "preexisting destination bytes\n";
    {
        std::ofstream output(destination, std::ios::binary);
        output.write(sentinel.data(), static_cast<std::streamsize>(sentinel.size()));
        if (!output)
            return fail("could not initialize destination sentinel");
    }

    const auto write = [&]() {
        return exporter.fileExport(destination.string(), version, binary,
                                   &data, false, true);
    };
    exporter.failAt = 1000000;
    if (!write() || !callerGraphMatches(data, profileCount, dwfOnly))
        return fail("allocation calibration export failed");
    const long allocationCount = exporter.reached;
    if (allocationCount <= 0 || !verifyGraph(destination, profileCount))
        return fail("calibration did not produce the expected graph");
    std::string expectedBytes;
    if (!readAll(destination, expectedBytes))
        return fail("cannot read clean calibration bytes");

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
            || !callerGraphMatches(data, profileCount, dwfOnly)
            || !readAll(destination, current) || current != sentinel) {
            std::cerr << "offset=" << offset << " wrote=" << wrote
                      << " reached=" << exporter.reached
                      << " current-size=" << current.size()
                      << " sentinel-size=" << sentinel.size()
                      << " injected=" << allocationFailed
                      << " attempts=" << allocationAttempts << '\n';
            return fail("injected failure did not preserve the prior destination");
        }
        if (!write() || !callerGraphMatches(data, profileCount, dwfOnly)
            || !verifyGraph(destination, profileCount))
            return fail("same-adapter retry failed typed graph readback");
        std::string retryBytes;
        if (!readAll(destination, retryBytes) || retryBytes != expectedBytes)
            return fail("same-adapter retry differs from clean output");
        ++failures;
    }
    if (failures != static_cast<unsigned>(allocationCount))
        return fail("not every calibrated callback allocation was injected");
    std::cout << "underlay OBJECTS " << versionName << ' '
              << (binary ? "binary" : "ASCII")
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
    constexpr std::array<std::pair<DRW::Version, const char*>, 4> versions{{
        {DRW::AC1021, "AC1021"}, {DRW::AC1024, "AC1024"},
        {DRW::AC1027, "AC1027"},
        {DRW::AC1032, "AC1032"}}};
    for (const auto& version : versions) {
        if (!run(directory, version.first, version.second, false)
            || !run(directory, version.first, version.second, true))
            return 1;
    }
    return 0;
}
