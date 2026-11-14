#include "dx_iface.h"

#include <algorithm>
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

class SunStudyExport final : public dx_iface {
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
        || data.sunStudies.size() != 1u || !data.hasNamedObjectDictionary)
        return false;
    const auto& study = data.sunStudies.front();
    const auto owner = std::find_if(data.dictionaries.begin(), data.dictionaries.end(),
        [&](const DRW_Dictionary& dictionary) {
            return dictionary.handle == study.parentHandle;
        });
    const auto rootEntry = std::find_if(data.namedObjectDictionary.m_entries.begin(),
        data.namedObjectDictionary.m_entries.end(), [](const DRW_Dictionary::Entry& entry) {
            return entry.m_name == "MY_SUNSTUDIES";
        });
    return owner != data.dictionaries.end()
        && owner->parentHandle == data.namedObjectDictionary.handle
        && rootEntry != data.namedObjectDictionary.m_entries.end()
        && rootEntry->m_handle == owner->handle
        && owner->m_entries.size() == 1u
        && owner->m_entries.front().m_handle == study.handle
        && study.m_setupName == "Morning"
        && study.m_dates.size() == 1u
        && study.m_dates.front().m_julianDay == 2460000
        && study.m_dates.front().m_milliseconds == 3600000
        && study.m_hours == std::vector<bool>({true, false});
}

bool run(const std::filesystem::path& source,
         const std::filesystem::path& destination, DRW::Version version) {
    const auto fail = [](const char* message) {
        std::cerr << "SUNSTUDY OBJECTS allocation retry: " << message << '\n';
        return false;
    };
    dx_data data;
    dx_iface importer;
    if (!importer.enableWholeDxfSourceObservation()
        || !importer.fileImport(source.string(), &data, false)
        || data.sunStudies.size() != 1u || data.rawSunStudies.size() != 1u
        || data.sourceVersion != version)
        return fail("generated source did not produce the qualified revision graph");

    SunStudyExport exporter;
    exporter.cData = &data;
    const char* revision = version == DRW::AC1024 ? "AC1024"
        : version == DRW::AC1027 ? "AC1027" : "AC1032";
    const auto write = [&](bool binary) {
        return exporter.fileExport(destination.string(), version, binary,
            &data, false, true, dx_dxfSourceTextPolicy::Compatible,
            DRW::DxfTextWritePolicy::Compatible,
            dx_dxfProxyGraphicsPolicy::Compatible,
            dx_dwgReadCompletenessPolicy::RequireComplete);
    };
    const std::string sentinel = "prior destination bytes\n";
    unsigned encodings = 0u;
    for (const bool binary : {false, true}) {
        {
            std::ofstream output(destination, std::ios::binary | std::ios::trunc);
            output.write(sentinel.data(), static_cast<std::streamsize>(sentinel.size()));
            if (!output)
                return fail("could not initialize destination sentinel");
        }
        exporter.failAt = 1000000;
        if (!write(binary))
            return fail("allocation calibration export failed");
        const long allocationCount = exporter.reached;
        if (allocationCount <= 0 || !verifyGraph(destination))
            return fail("calibration did not produce the expected SUNSTUDY graph");

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
            const bool wrote = write(binary);
            exporter.failAt = -1;
            std::string current;
            if (wrote || exporter.reached < 1 || !readAll(destination, current)
                || current != sentinel)
                return fail("injected failure did not preserve the prior destination");
            if (!write(binary) || !verifyGraph(destination))
                return fail("same-adapter retry failed typed graph readback");
            ++offsets;
        }
        if (offsets != static_cast<unsigned>(allocationCount))
            return fail("not every calibrated callback allocation offset was tested");
        std::cout << "SUNSTUDY OBJECTS " << revision << ' '
                  << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", rollback/retry/readback=" << offsets << '\n';
        ++encodings;
    }
    return encodings == 2u;
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
    if (argc != 4) {
        std::cerr << "usage: sunstudy_objects_allocation_tests SOURCE_DXF DESTINATION_DXF VERSION\n";
        return 2;
    }
    const std::string revision(argv[3]);
    const DRW::Version version = revision == "AC1024" ? DRW::AC1024
        : revision == "AC1027" ? DRW::AC1027
        : revision == "AC1032" ? DRW::AC1032 : DRW::UNKNOWNV;
    if (version == DRW::UNKNOWNV) {
        std::cerr << "unsupported SUNSTUDY allocation revision: " << revision << '\n';
        return 2;
    }
    const auto before = failedAllocations;
    if (!run(argv[1], argv[2], version))
        return 1;
    if (failedAllocations == before) {
        std::cerr << "allocation fault injector did not reach operator new\n";
        return 1;
    }
    return 0;
}
