// SPDX-License-Identifier: GPL-2.0-or-later
// Generated SCALE OBJECTS allocation-failure rollback and retry control.
#include "lc3_compat_check.cpp"
#include "dx_iface.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>

namespace {
long allocationsBeforeFailure = -1;
std::size_t failedAllocations = 0;

struct AllocationFailure {
    explicit AllocationFailure(long count) { allocationsBeforeFailure = count; }
    ~AllocationFailure() { allocationsBeforeFailure = -1; }
};

class ScaleExport final : public LC3CompatCheck {
public:
    explicit ScaleExport(dxfRW& output) : output(output) {
        dictionary.handle = 0xE000u;
        dictionary.parentHandle = 0u; // Public writer supplies the NOD owner.
        DRW_Dictionary::Entry entry;
        entry.m_name = "1:1";
        entry.m_handle = 0xE001u;
        dictionary.m_entries.push_back(std::move(entry));
        scale.handle = 0xE001u;
        scale.parentHandle = dictionary.handle;
        scale.name = "1:1";
        scale.paperUnits = 1.0;
        scale.drawingUnits = 1.0;
        scale.isUnitScale = true;
    }

    long failAt = -1;
    long reached = 0;
    DRW_Dictionary dictionary;
    DRW_Scale scale;
    bool dictionaryWritten = false;
    bool scaleWritten = false;
    dxfRW& output;

    void writeObjects() override {
        const auto emit = [this]() {
            dictionaryWritten = output.writeDictionary(&dictionary);
            scaleWritten = dictionaryWritten && output.writeScale(&scale);
        };
        if (failAt < 0) {
            emit();
            return;
        }
        const auto before = failedAllocations;
        try {
            AllocationFailure failure(failAt);
            emit();
            reached = failedAllocations != before
                ? static_cast<long>(failedAllocations - before)
                : failAt - allocationsBeforeFailure;
        } catch (const std::bad_alloc&) {
            reached = failedAllocations != before
                ? static_cast<long>(failedAllocations - before)
                : failAt - allocationsBeforeFailure;
            throw;
        }
    }
    bool finalizeDxfWrite() override {
        return dictionaryWritten && scaleWritten;
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

bool write(ScaleExport& exporter, dxfRW& codec,
           DRW::Version version, bool binary) {
    codec.setRootDictEntries({{"ACAD_SCALELIST", "E000"}});
    return codec.write(&exporter, version, binary);
}

bool verifyGraph(const std::filesystem::path& path) {
    dx_data data;
    dx_iface reader;
    if (!reader.fileImport(path.string(), &data, false)
        || !data.hasNamedObjectDictionary || data.scales.size() != 1u)
        return false;
    bool rootFound = false;
    for (const auto& entry : data.namedObjectDictionary.m_entries)
        rootFound = rootFound || (entry.m_name == "ACAD_SCALELIST"
                                  && entry.m_handle == 0xE000u);
    const auto dictionary = std::find_if(data.dictionaries.begin(),
        data.dictionaries.end(), [](const DRW_Dictionary& value) {
            return value.handle == 0xE000u;
        });
    if (!rootFound || dictionary == data.dictionaries.end()
        || dictionary->parentHandle != data.namedObjectDictionary.handle
        || dictionary->m_entries.size() != 1u
        || dictionary->m_entries.front().m_name != "1:1"
        || dictionary->m_entries.front().m_handle != 0xE001u)
        return false;
    const DRW_Scale& scale = data.scales.front();
    return scale.handle == 0xE001u && scale.parentHandle == 0xE000u
        && scale.name == "1:1" && scale.flag == 0
        && scale.paperUnits == 1.0 && scale.drawingUnits == 1.0
        && scale.isUnitScale;
}

bool sourceUnchanged(const ScaleExport& exporter,
                     const DRW_Dictionary& dictionary,
                     const DRW_Scale& scale) {
    return exporter.dictionary.handle == dictionary.handle
        && exporter.dictionary.parentHandle == dictionary.parentHandle
        && exporter.dictionary.m_entries.size() == dictionary.m_entries.size()
        && exporter.dictionary.m_entries.front().m_name
            == dictionary.m_entries.front().m_name
        && exporter.dictionary.m_entries.front().m_handle
            == dictionary.m_entries.front().m_handle
        && exporter.scale.handle == scale.handle
        && exporter.scale.parentHandle == scale.parentHandle
        && exporter.scale.name == scale.name
        && exporter.scale.flag == scale.flag
        && exporter.scale.paperUnits == scale.paperUnits
        && exporter.scale.drawingUnits == scale.drawingUnits
        && exporter.scale.isUnitScale == scale.isUnitScale;
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "SCALE OBJECTS allocation retry: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const std::string sentinel = "prior SCALE destination bytes\n";
    for (const DRW::Version version : {
             DRW::AC1014, DRW::AC1015, DRW::AC1018, DRW::AC1021,
             DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        const auto output = directory
            / (std::string("scale-objects-")
               + std::to_string(static_cast<int>(version)) + ".dxf");
        dxfRW codec(output.string().c_str());
        ScaleExport exporter(codec);
        const DRW_Dictionary sourceDictionary = exporter.dictionary;
        const DRW_Scale sourceScale = exporter.scale;

        for (const bool binary : {false, true}) {
            // Warm persistent stream/handle containers before calibration so
            // the measured count matches later same-writer attempts.
            if (!write(exporter, codec, version, binary))
                return fail("warm-up export was refused");
            exporter.failAt = 1000000;
            if (!write(exporter, codec, version, binary)) {
                std::cerr << "dictionaryWritten=" << exporter.dictionaryWritten
                          << " scaleWritten=" << exporter.scaleWritten << '\n';
                return fail("allocation calibration export was refused");
            }
            if (!verifyGraph(output))
                return fail("allocation calibration output did not read back graph");
            std::string cleanBytes;
            if (!readAll(output, cleanBytes))
                return fail("could not retain clean calibration bytes");
            const long allocationCount = exporter.reached;
            if (allocationCount <= 0)
                return fail("calibration observed no callback allocations");
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior)
                        return fail("could not initialize destination sentinel");
                }
                exporter.failAt = offset;
                exporter.reached = 0;
                const bool wrote = write(exporter, codec, version, binary);
                exporter.failAt = -1;
                std::string current;
                if (wrote || exporter.reached < 1
                    || !readAll(output, current) || current != sentinel) {
                    std::cerr << "version=" << static_cast<int>(version)
                              << " binary=" << binary << " offset=" << offset
                              << " wrote=" << wrote
                              << " reached=" << exporter.reached
                              << " outputMatchesSentinel=" << (current == sentinel)
                              << '\n';
                    return fail("injected failure changed destination or was missed");
                }
                if (!sourceUnchanged(exporter, sourceDictionary, sourceScale))
                    return fail("injected failure changed the caller graph");
                if (!write(exporter, codec, version, binary)
                    || !verifyGraph(output))
                    return fail("same-writer retry failed typed graph readback");
                std::string retryBytes;
                if (!readAll(output, retryBytes) || retryBytes != cleanBytes)
                    return fail("same-writer retry differed from clean bytes");
                if (!sourceUnchanged(exporter, sourceDictionary, sourceScale))
                    return fail("same-writer retry changed the caller graph");
            }
            std::cout << "SCALE OBJECTS version="
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
        std::cerr << "usage: scale_objects_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    const auto before = failedAllocations;
    if (!run(argv[1]))
        return 1;
    if (failedAllocations == before) {
        std::cerr << "allocation fault injector did not reach operator new\n";
        return 1;
    }
    return 0;
}
