#include "dx_iface.h"
#include "libdwgr.h"

#include <algorithm>
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
std::size_t failedAllocations = 0;

struct AllocationFailure {
    explicit AllocationFailure(long count) { allocationsBeforeFailure = count; }
    ~AllocationFailure() { allocationsBeforeFailure = -1; }
};

class RasterExport final : public dx_iface {
public:
    long failAt = -1;
    long reached = 0;

    void writeObjects() override {
        if (failAt < 0) {
            dx_iface::writeObjects();
            return;
        }
        const auto failuresBefore = failedAllocations;
        try {
            AllocationFailure failure(failAt);
            dx_iface::writeObjects();
            reached = failedAllocations != failuresBefore
                ? static_cast<long>(failedAllocations - failuresBefore)
                : failAt - allocationsBeforeFailure;
        } catch (const std::bad_alloc&) {
            reached = failedAllocations != failuresBefore
                ? static_cast<long>(failedAllocations - failuresBefore)
                : failAt - allocationsBeforeFailure;
            throw;
        }
    }
};

class RasterAuthor final : public dx_iface {
public:
    RasterAuthor(dwgRW& output, DRW::Version version) : output(output) {
        cData = &storage;
        currentBlock = storage.mBlock;
        variables.handle = 0xB500u;
        variables.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        variables.m_classVersion = 0;
        variables.m_imageFrame = 0;
        variables.m_imageQuality = 1;
        variables.m_units = version == DRW::AC1018 || version == DRW::AC1024
                || version == DRW::AC1032 ? 3 : 1;
        variables.reactorHandles = {DRW::DwgNamedObjectsDictionaryHandle};
        variables.setDwgCommonObjectState(1, 1, false);
    }

    void writeHeader(DRW_Header& header) override { header.vars.clear(); }
    void writeDwgClasses() override {
        DRW_RasterVariables registration;
        registration.handle = variables.handle;
        rootRegistered = output.registerDwgNamedObjectDictionaryEntry(
            "ACAD_IMAGE_VARS", variables.handle);
        classRegistered = output.registerRasterVariablesObjectClass(
            &registration);
    }
    void writeObjects() override {
        variablesWritten = output.writeRasterVariables(&variables);
    }
    bool finalizeDwgWrite() override {
        return rootRegistered && classRegistered && variablesWritten;
    }
    bool finalizeDwgWriteStructure() override { return true; }

    dwgRW& output;
    dx_data storage;
    DRW_RasterVariables variables;
    bool rootRegistered = false;
    bool classRegistered = false;
    bool variablesWritten = false;
};

class RasterReadback final : public dx_iface {
public:
    void addRasterVariables(const DRW_RasterVariables& value) override {
        records.push_back(value);
    }
    std::vector<DRW_RasterVariables> records;
};

bool readAll(const std::filesystem::path& path, std::string& value) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    value.assign(std::istreambuf_iterator<char>(input),
                 std::istreambuf_iterator<char>());
    return !input.bad();
}

bool verifyGraph(const std::filesystem::path& path, std::int16_t expectedUnits) {
    dx_data data;
    RasterReadback reader;
    if (!reader.fileImport(path.string(), &data, false)
        || reader.records.size() != 1u || !data.hasNamedObjectDictionary)
        return false;
    const auto& variables = reader.records.front();
    const auto rootEntry = std::find_if(data.namedObjectDictionary.m_entries.begin(),
        data.namedObjectDictionary.m_entries.end(), [&](const DRW_Dictionary::Entry& entry) {
            return entry.m_name == "ACAD_IMAGE_VARS"
                && entry.m_handle == variables.handle;
        });
    return rootEntry != data.namedObjectDictionary.m_entries.end()
        && variables.handle == 0xB500u
        && variables.parentHandle == data.namedObjectDictionary.handle
        && variables.m_classVersion == 0
        && variables.m_imageFrame == 0
        && variables.m_imageQuality == 1
        && variables.m_units == expectedUnits
        && variables.reactorHandles
            == std::vector<std::uint32_t>{data.namedObjectDictionary.handle};
}

bool sourceRecordUnchanged(const dx_data& data,
                           const DRW_RasterVariables& expected) {
    if (data.dwgRasterVariables.size() != 1u)
        return false;
    const auto& actual = data.dwgRasterVariables.begin()->second;
    return actual.handle == expected.handle
        && actual.parentHandle == expected.parentHandle
        && actual.reactorHandles == expected.reactorHandles
        && actual.xDictHandle == expected.xDictHandle
        && actual.m_classVersion == expected.m_classVersion
        && actual.m_imageFrame == expected.m_imageFrame
        && actual.m_imageQuality == expected.m_imageQuality
        && actual.m_units == expected.m_units;
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "RASTERVARIABLES OBJECTS allocation retry: "
                  << message << '\n';
        return false;
    };
    const std::string sentinel = "prior destination bytes\n";
    std::vector<std::filesystem::path> scratchFiles;
    struct Cleanup {
        std::vector<std::filesystem::path>& files;
        ~Cleanup() {
            std::error_code ignored;
            for (const auto& path : files)
                std::filesystem::remove(path, ignored);
        }
    } cleanup{scratchFiles};

    for (const DRW::Version version : {
             DRW::AC1018, DRW::AC1021, DRW::AC1024, DRW::AC1027,
             DRW::AC1032}) {
        const auto source = directory
            / ("rastervariables-source-"
               + std::to_string(static_cast<int>(version)) + ".dwg");
        const std::array<std::filesystem::path, 2> versionDestinations{{
            directory / ("rastervariables-"
                + std::to_string(static_cast<int>(version)) + "-ascii.dxf"),
            directory / ("rastervariables-"
                + std::to_string(static_cast<int>(version)) + "-binary.dxf")}};
        if (std::filesystem::exists(source))
            return fail("scratch input path already exists");
        scratchFiles.push_back(source);
        for (const auto& destination : versionDestinations) {
            if (std::filesystem::exists(destination))
                return fail("scratch output path already exists");
            scratchFiles.push_back(destination);
        }
        {
            dwgRW sourceWriter(source.string().c_str());
            RasterAuthor author(sourceWriter, version);
            if (!sourceWriter.write(&author, version, false)) {
                std::cerr << "from-scratch source generation refused version="
                          << static_cast<int>(version) << '\n';
                return fail("from-scratch DWG source generation failed");
            }
        }
        dx_data data;
        dx_iface importer;
        if (!importer.fileImport(source.string(), &data, false)
            || data.sourceFormat != dx_source_format::DWG
            || data.sourceVersion != version
            || data.rasterVariablesCaptureFailed
            || data.dwgRasterVariables.size() != 1u
            || data.rasterVariablesRetainedRecords != 1u)
            return fail("generated source did not import one typed raster graph");
        const DRW_RasterVariables sourceRecord =
            data.dwgRasterVariables.begin()->second;

        RasterExport exporter;
        exporter.cData = &data;
        const auto write = [&](const std::filesystem::path& destination,
                               bool binary) {
            return exporter.fileExport(destination.string(), version,
                binary, &data, false, false, dx_dxfSourceTextPolicy::Compatible,
                DRW::DxfTextWritePolicy::Compatible,
                dx_dxfProxyGraphicsPolicy::Compatible,
                dx_dwgReadCompletenessPolicy::Legacy);
        };
        for (std::size_t index = 0; index < versionDestinations.size(); ++index) {
            const auto& destination = versionDestinations[index];
            const bool binary = index == 1u;
            exporter.failAt = 1000000;
            if (!write(destination, binary)) {
                std::cerr << "calibration refused version="
                          << static_cast<int>(version)
                          << " binary=" << binary << '\n';
                return fail("allocation calibration export failed");
            }
            const long allocationCount = exporter.reached;
            const std::int16_t expectedUnits =
                version == DRW::AC1018 || version == DRW::AC1024
                    || version == DRW::AC1032 ? 3 : 1;
            if (allocationCount <= 0
                || !verifyGraph(destination, expectedUnits))
                return fail("calibration did not emit the typed raster graph");
            std::string cleanBytes;
            if (!readAll(destination, cleanBytes))
                return fail("could not retain calibration output bytes");

            unsigned offsets = 0u;
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream output(destination,
                        std::ios::binary | std::ios::trunc);
                    output.write(sentinel.data(),
                                 static_cast<std::streamsize>(sentinel.size()));
                    if (!output)
                        return fail("could not reset destination sentinel");
                }
                exporter.failAt = offset;
                exporter.reached = 0;
                const bool wrote = write(destination, binary);
                exporter.failAt = -1;
                std::string current;
                if (wrote || exporter.reached < 1
                    || !readAll(destination, current)
                    || current != sentinel
                    || !sourceRecordUnchanged(data, sourceRecord))
                    return fail("injected failure changed destination or caller record");
                if (!write(destination, binary)
                    || !verifyGraph(destination, expectedUnits)
                    || !sourceRecordUnchanged(data, sourceRecord))
                    return fail("same-adapter retry failed typed graph readback");
                std::string retryBytes;
                if (!readAll(destination, retryBytes) || retryBytes != cleanBytes)
                    return fail("same-adapter retry differed from clean bytes");
                ++offsets;
            }
            if (offsets != static_cast<unsigned>(allocationCount))
                return fail("not every calibrated callback allocation was injected");
            std::cout << "RASTERVARIABLES OBJECTS version="
                      << static_cast<int>(version) << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=" << offsets << '\n';
        }
    }
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (allocationsBeforeFailure == 0) {
        ++failedAllocations;
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
        std::cerr << "usage: rastervariables_objects_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    const std::filesystem::path directory(argv[1]);
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        std::cerr << "could not create scratch directory: " << error.message() << '\n';
        return 2;
    }
    const auto before = failedAllocations;
    if (!run(directory))
        return 1;
    if (failedAllocations == before) {
        std::cerr << "allocation fault injector did not reach operator new\n";
        return 1;
    }
    return 0;
}
