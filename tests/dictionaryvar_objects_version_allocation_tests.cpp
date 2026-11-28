// SPDX-License-Identifier: GPL-2.0-or-later
// Generated DICTIONARYVAR version-matrix allocation rollback and retry control.
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"

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
std::size_t failedAllocations = 0u;

struct AllocationFailure {
    explicit AllocationFailure(long count) { allocationsBeforeFailure = count; }
    ~AllocationFailure() { allocationsBeforeFailure = -1; }
};

class DictionaryVarExport final : public LC3CompatCheck {
public:
    explicit DictionaryVarExport(dxfRW& codec) : codec(codec) {
        dictionary.handle = 0xD000u;
        dictionary.parentHandle = 0u;
        DRW_Dictionary::Entry entry;
        entry.m_name = "ACAD_TEST_VAR";
        entry.m_handle = 0xD001u;
        dictionary.m_entries.push_back(std::move(entry));
        variable.handle = 0xD001u;
        variable.parentHandle = dictionary.handle;
        variable.m_schema = 0;
        variable.m_value = "generated-version-matrix-value";
    }

    void writeObjects() override {
        const auto emit = [this]() {
            dictionaryWritten = codec.writeDictionary(&dictionary);
            variableWritten = dictionaryWritten
                && codec.writeDictionaryVar(&variable);
        };
        if (failAt < 0) {
            emit();
            return;
        }
        const std::size_t before = failedAllocations;
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
        return dictionaryWritten && variableWritten;
    }

    dxfRW& codec;
    DRW_Dictionary dictionary;
    DRW_DictionaryVar variable;
    long failAt = -1;
    long reached = 0;
    bool dictionaryWritten = false;
    bool variableWritten = false;
};

class DictionaryVarRead final : public LC3CompatCheck {
public:
    void addDictionary(const DRW_Dictionary& value) override {
        dictionaries.push_back(value);
    }
    void addDictionaryVar(const DRW_DictionaryVar& value) override {
        variables.push_back(value);
    }
    std::vector<DRW_Dictionary> dictionaries;
    std::vector<DRW_DictionaryVar> variables;
};

bool readAll(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

bool unchanged(const DictionaryVarExport& writer,
               const DRW_Dictionary& dictionary,
               const DRW_DictionaryVar& variable) {
    return writer.dictionary.handle == dictionary.handle
        && writer.dictionary.parentHandle == dictionary.parentHandle
        && writer.dictionary.m_entries.size() == dictionary.m_entries.size()
        && writer.dictionary.m_entries.front().m_name
            == dictionary.m_entries.front().m_name
        && writer.dictionary.m_entries.front().m_handle
            == dictionary.m_entries.front().m_handle
        && writer.variable.handle == variable.handle
        && writer.variable.parentHandle == variable.parentHandle
        && writer.variable.m_schema == variable.m_schema
        && writer.variable.m_value == variable.m_value;
}

bool verifyGraph(const std::filesystem::path& path) {
    dxfRW codec(path.string().c_str());
    DictionaryVarRead reader;
    if (!codec.read(&reader, false))
        return false;
    const DRW_Dictionary* dictionary = nullptr;
    const DRW_Dictionary* namedObjects = nullptr;
    const DRW_DictionaryVar* variable = nullptr;
    for (const DRW_Dictionary& value : reader.dictionaries) {
        if (value.handle == 0xD000u) {
            if (dictionary != nullptr)
                return false;
            dictionary = &value;
        }
        if (value.handle == DRW::DwgNamedObjectsDictionaryHandle)
            namedObjects = &value;
    }
    for (const DRW_DictionaryVar& value : reader.variables) {
        if (value.handle == 0xD001u) {
            if (variable != nullptr)
                return false;
            variable = &value;
        }
    }
    if (dictionary == nullptr || namedObjects == nullptr || variable == nullptr)
        return false;
    const auto namedEntry = std::find_if(
        namedObjects->m_entries.begin(), namedObjects->m_entries.end(),
        [](const DRW_Dictionary::Entry& entry) {
            return entry.m_name == "ACAD_DICTIONARYVAR_TEST";
        });
    return dictionary->parentHandle == DRW::DwgNamedObjectsDictionaryHandle
        && dictionary->m_entries.size() == 1u
        && dictionary->m_entries.front().m_name == "ACAD_TEST_VAR"
        && dictionary->m_entries.front().m_handle == variable->handle
        && namedEntry != namedObjects->m_entries.end()
        && namedEntry->m_handle == dictionary->handle
        && variable->parentHandle == dictionary->handle
        && variable->m_schema == 0
        && variable->m_value == "generated-version-matrix-value";
}

bool write(DictionaryVarExport& writer, DRW::Version version, bool binary) {
    writer.codec.setRootDictEntries({{"ACAD_DICTIONARYVAR_TEST", "D000"}});
    return writer.codec.write(&writer, version, binary);
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "DICTIONARYVAR version allocation retry: " << message
                  << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    const std::string sentinel = "prior DICTIONARYVAR destination bytes\n";
    for (const DRW::Version version : {
             DRW::AC1014, DRW::AC1015, DRW::AC1018, DRW::AC1021,
             DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        const auto output = directory
            / ("dictionaryvar-"
               + std::to_string(static_cast<int>(version)) + ".dxf");
        dxfRW codec(output.string().c_str());
        DictionaryVarExport writer(codec);
        const DRW_Dictionary sourceDictionary = writer.dictionary;
        const DRW_DictionaryVar sourceVariable = writer.variable;
        for (const bool binary : {false, true}) {
            if (!write(writer, version, binary))
                return fail("warm-up export was refused");
            writer.failAt = 1000000;
            if (!write(writer, version, binary) || !verifyGraph(output))
                return fail("calibration output did not read back the typed graph");
            std::string cleanBytes;
            if (!readAll(output, cleanBytes))
                return fail("could not retain clean calibration bytes");
            const long allocationCount = writer.reached;
            if (allocationCount <= 0)
                return fail("calibration observed no callback allocations");

            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior)
                        return fail("could not initialize destination sentinel");
                }
                writer.failAt = offset;
                writer.reached = 0;
                writer.dictionaryWritten = false;
                writer.variableWritten = false;
                const bool wrote = write(writer, version, binary);
                writer.failAt = -1;
                std::string actual;
                const bool outputPreserved = readAll(output, actual)
                    && actual == sentinel;
                if (wrote || writer.reached < 1 || !outputPreserved
                    || !unchanged(writer, sourceDictionary, sourceVariable)) {
                    std::cerr << "version=" << static_cast<int>(version)
                              << " binary=" << binary << " offset=" << offset
                              << " wrote=" << wrote
                              << " reached=" << writer.reached
                              << " outputMatchesSentinel=" << outputPreserved
                              << " callerUnchanged="
                              << unchanged(writer, sourceDictionary, sourceVariable)
                              << '\n';
                    return fail("injected failure changed output or caller state");
                }
                if (!write(writer, version, binary) || !verifyGraph(output))
                    return fail("same-writer retry failed typed graph readback");
                std::string retryBytes;
                if (!readAll(output, retryBytes) || retryBytes != cleanBytes
                    || !unchanged(writer, sourceDictionary, sourceVariable))
                    return fail("retry changed bytes or caller graph");
            }
            std::cout << "DICTIONARYVAR " << static_cast<int>(version) << ' '
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
        std::cerr << "usage: dictionaryvar_objects_version_allocation_tests "
                     "SCRATCH_DIR\n";
        return 2;
    }
    const std::size_t before = failedAllocations;
    if (!run(argv[1]))
        return 1;
    if (failedAllocations == before) {
        std::cerr << "allocation fault injector did not reach operator new\n";
        return 1;
    }
    return 0;
}
