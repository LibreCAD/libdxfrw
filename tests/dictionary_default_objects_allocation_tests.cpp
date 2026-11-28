// SPDX-License-Identifier: GPL-2.0-or-later
// Generated DICTIONARYWDFLT/PLACEHOLDER OBJECTS rollback and retry control.
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

class DictionaryDefaultExport final : public dx_iface {
public:
    long failAt = -1;
    long reached = 0;
    bool directTyped = false;
    DRW::Version targetVersion = DRW::AC1027;

    void writeObjects() override {
        if (directTyped) {
            bool emitted = false;
            const auto emit = [this]() {
                if (cData == nullptr || dxfW == nullptr
                    || cData->dictionaryWithDefaults.size() != 1u
                    || cData->rawPlaceholders.size() != 1u)
                    return false;
                const DRW_RawDxfObject& source =
                    cData->rawPlaceholders.front();
                DRW_AcDbPlaceholder placeholder;
                placeholder.handle = source.handle;
                placeholder.parentHandle = source.parentHandle;
                DRW_DictionaryWithDefault dictionary =
                    cData->dictionaryWithDefaults.front();
                // AC1014 predates the DXF group-281 cloning field, so its
                // only lossless/default profile is cloning value zero.
                if (targetVersion == DRW::AC1014)
                    dictionary.cloning = 0;
                const bool dictionaryWritten =
                    dxfW->writeDictionaryWithDefault(&dictionary);
                const bool placeholderWritten = dictionaryWritten
                    && dxfW->writeAcDbPlaceholder(&placeholder);
                return dictionaryWritten && placeholderWritten;
            };
            if (failAt < 0) {
                emitted = emit();
            } else {
                const auto before = failedAllocations;
                try {
                    AllocationFailure failure(failAt);
                    emitted = emit();
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
            if (!emitted)
                (void)dxfW->writeRawDxfObject(nullptr);
            return;
        }
        if (failAt < 0) {
            dx_iface::writeObjects();
            return;
        }
        const auto before = failedAllocations;
        try {
            AllocationFailure failure(failAt);
            dx_iface::writeObjects();
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
};

const char* sourceText =
    "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1027\n"
    "9\n$CEPSNTYPE\n380\n3\n9\n$CEPSNID\n390\n11\n"
    "0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n"
    "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n"
    "280\n1\n281\n1\n3\nACAD_PLOTSTYLENAME\n350\n10\n"
    "0\nACDBPLACEHOLDER\n5\n11\n330\n10\n"
    "0\nDICTIONARYWDFLT\n5\n10\n330\nC\n"
    "100\nAcDbDictionary\n280\n0\n281\n1\n100\nAcDbDictionaryWithDefault\n340\n11\n"
    "0\nENDSEC\n0\nEOF\n";

// AC1014 predates both the DICTIONARYWDFLT cloning field and the HEADER
// plot-style ID. Keep a matching generated source profile for that target.
const char* ac1014SourceText =
    "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1014\n"
    "0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n"
    "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n"
    "280\n1\n281\n1\n3\nACAD_PLOTSTYLENAME\n350\n10\n"
    "0\nACDBPLACEHOLDER\n5\n11\n330\n10\n"
    "0\nDICTIONARYWDFLT\n5\n10\n330\nC\n"
    "100\nAcDbDictionary\n3\nACAD_PLOTSTYLENAME\n350\n11\n"
    "100\nAcDbDictionaryWithDefault\n340\n11\n"
    "0\nENDSEC\n0\nEOF\n";

bool readAll(const std::filesystem::path& path, std::string& value) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    value.assign(std::istreambuf_iterator<char>(input),
                 std::istreambuf_iterator<char>());
    return !input.bad();
}

bool import(const std::filesystem::path& path, dx_data& data) {
    dx_iface reader;
    const bool read = reader.fileImport(path.string(), &data, false);
    if (!read || !data.hasSingleSuccessfulDictionaryDefaultImport()
        || !data.hasNamedObjectDictionary
        || data.dictionaryWithDefaults.size() != 1u
        || data.rawDictionaryWithDefaults.size() != 1u
        || data.rawPlaceholders.size() != 1u) {
        std::cerr << "dictionary-default import detail: read=" << read
                  << " receipt="
                  << data.hasSingleSuccessfulDictionaryDefaultImport()
                  << " nod=" << data.hasNamedObjectDictionary
                  << " typed=" << data.dictionaryWithDefaults.size()
                  << " raw-default=" << data.rawDictionaryWithDefaults.size()
                  << " raw-placeholder=" << data.rawPlaceholders.size()
                  << '\n';
        return false;
    }
    return true;
}

bool verifyGraph(const std::filesystem::path& path, int expectedCloning,
                 bool expectHeaderPlotStyle) {
    dx_data data;
    if (!import(path, data))
        return false;
    const DRW_DictionaryWithDefault& dictionary =
        data.dictionaryWithDefaults.front();
    const DRW_RawDxfObject& placeholder = data.rawPlaceholders.front();
    const auto root = std::find_if(data.namedObjectDictionary.m_entries.begin(),
        data.namedObjectDictionary.m_entries.end(),
        [&dictionary](const DRW_Dictionary::Entry& entry) {
            return entry.m_name == "ACAD_PLOTSTYLENAME"
                && entry.m_handle == dictionary.handle;
        });
    std::uint64_t plotStyleHandle = 0u;
    bool hasPlotStyleHandle = false;
    const bool headerPlotStyleValid =
        data.headerC.getDxfPlotStyleIdHandle(
            plotStyleHandle, hasPlotStyleHandle)
        && (expectHeaderPlotStyle
            ? hasPlotStyleHandle && plotStyleHandle == placeholder.handle
            : !hasPlotStyleHandle);
    return root != data.namedObjectDictionary.m_entries.end()
        && data.namedObjectDictionary.handle == 0xCu
        && dictionary.parentHandle == data.namedObjectDictionary.handle
        && dictionary.cloning == expectedCloning
        && dictionary.handle != placeholder.handle
        && dictionary.m_defaultEntryHandle == placeholder.handle
        && placeholder.parentHandle == dictionary.handle
        && placeholder.name == "ACDBPLACEHOLDER"
        && headerPlotStyleValid;
}

bool sourceUnchanged(const dx_data& data,
                     const DRW_DictionaryWithDefault& dictionary,
                     const DRW_RawDxfObject& placeholder,
                     const DRW_RawDxfObject& rawDictionaryBaseline,
                     bool expectHeaderPlotStyle) {
    if (!data.hasSingleSuccessfulDictionaryDefaultImport()
        || !data.hasNamedObjectDictionary
        || data.namedObjectDictionary.handle != 0xCu
        || data.namedObjectDictionary.m_entries.size() != 1u
        || data.namedObjectDictionary.m_entries.front().m_name
            != "ACAD_PLOTSTYLENAME"
        || data.namedObjectDictionary.m_entries.front().m_handle != 0x10u
        || data.dictionaryWithDefaults.size() != 1u
        || data.rawDictionaryWithDefaults.size() != 1u
        || data.rawPlaceholders.size() != 1u)
        return false;
    const DRW_DictionaryWithDefault& actual =
        data.dictionaryWithDefaults.front();
    const DRW_RawDxfObject& rawPlaceholder = data.rawPlaceholders.front();
    const DRW_RawDxfObject& rawDictionary =
        data.rawDictionaryWithDefaults.front();
    const auto sameEntries = [](const DRW_Dictionary& left,
                                const DRW_Dictionary& right) {
        if (left.m_entries.size() != right.m_entries.size())
            return false;
        for (std::size_t index = 0u; index < left.m_entries.size(); ++index) {
            const auto& a = left.m_entries[index];
            const auto& b = right.m_entries[index];
            if (a.m_name != b.m_name || a.m_handle != b.m_handle
                || a.m_dxfHandleCode != b.m_dxfHandleCode)
                return false;
        }
        return true;
    };
    std::uint64_t plotStyleHandle = 0u;
    bool hasPlotStyleHandle = false;
    return actual.handle == dictionary.handle
        && actual.parentHandle == dictionary.parentHandle
        && actual.name == dictionary.name
        && actual.hardOwner == dictionary.hardOwner
        && actual.cloning == dictionary.cloning
        && actual.xDictHandle == dictionary.xDictHandle
        && actual.reactorHandles == dictionary.reactorHandles
        && actual.m_defaultEntryHandle == dictionary.m_defaultEntryHandle
        && sameEntries(actual, dictionary)
        && rawPlaceholder.name == placeholder.name
        && rawPlaceholder.handle == placeholder.handle
        && rawPlaceholder.parentHandle == placeholder.parentHandle
        && rawPlaceholder.groups.size() == placeholder.groups.size()
        && rawPlaceholder.hasRawValues == placeholder.hasRawValues
        && rawPlaceholder.rawValues == placeholder.rawValues
        && rawDictionary.name == rawDictionaryBaseline.name
        && rawDictionary.handle == dictionary.handle
        && rawDictionary.parentHandle == dictionary.parentHandle
        && rawDictionary.groups.size()
            == rawDictionaryBaseline.groups.size()
        && rawDictionary.hasRawValues == rawDictionaryBaseline.hasRawValues
        && rawDictionary.rawValues == rawDictionaryBaseline.rawValues
        && data.headerC.getDxfPlotStyleIdHandle(
               plotStyleHandle, hasPlotStyleHandle)
        && (expectHeaderPlotStyle
            ? hasPlotStyleHandle && plotStyleHandle == 0x11u
            : !hasPlotStyleHandle);
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "DICTIONARYWDFLT/PLACEHOLDER OBJECTS allocation retry: "
                  << message << '\n';
        return false;
    };
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error)
        return fail("could not create scratch directory");

    const auto sourcePath = directory / "dictionary-default-source.dxf";
    const auto ac1014SourcePath = directory / "dictionary-default-ac1014-source.dxf";
    const auto outputPath = directory / "dictionary-default-output.dxf";
    {
        std::ofstream source(sourcePath, std::ios::binary | std::ios::trunc);
        source << sourceText;
        if (!source)
            return fail("could not create generated source DXF");
    }
    {
        std::ofstream source(ac1014SourcePath,
                             std::ios::binary | std::ios::trunc);
        source << ac1014SourceText;
        if (!source)
            return fail("could not create generated AC1014 source DXF");
    }
    dx_data data;
    dx_data ac1014Data;
    if (!import(sourcePath, data)
        || !import(ac1014SourcePath, ac1014Data)
        || data.namedObjectDictionary.m_entries.size() != 1u
        || data.namedObjectDictionary.m_entries.front().m_handle != 0x10u
        || data.dictionaryWithDefaults.front().handle != 0x10u
        || data.dictionaryWithDefaults.front().m_defaultEntryHandle != 0x11u
        || data.rawPlaceholders.front().handle != 0x11u)
        return fail("generated DICTIONARYWDFLT graph did not import");
    const DRW_RawDxfObject sourceRawDictionary =
        data.rawDictionaryWithDefaults.front();
    const DRW_RawDxfObject ac1014RawDictionary =
        ac1014Data.rawDictionaryWithDefaults.front();
    DictionaryDefaultExport exporter;
    dx_data* activeData = &data;
    const std::string sentinel = "prior dictionary-default bytes\n";
    const auto write = [&exporter, &activeData, &outputPath](
            DRW::Version version, bool binary) {
        exporter.targetVersion = version;
        return exporter.fileExport(outputPath.string(), version, binary,
            activeData, false, true);
    };

    for (const DRW::Version version :
         {DRW::AC1014, DRW::AC1015, DRW::AC1018, DRW::AC1021,
          DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
      activeData = version == DRW::AC1014 ? &ac1014Data : &data;
      const DRW_DictionaryWithDefault sourceDictionary =
          activeData->dictionaryWithDefaults.front();
      const DRW_RawDxfObject sourcePlaceholder =
          activeData->rawPlaceholders.front();
      const DRW_RawDxfObject& rawDictionaryBaseline =
          version == DRW::AC1014 ? ac1014RawDictionary : sourceRawDictionary;
      for (const bool directTyped : {false, true}) {
        if (!directTyped && version != DRW::AC1027)
            continue;
        exporter.directTyped = directTyped;
        for (const bool binary : {false, true}) {
            exporter.failAt = 1000000;
            const bool wroteCalibration = write(version, binary);
            const int expectedCloning = version == DRW::AC1014
                ? 0 : sourceDictionary.cloning;
            const bool expectHeaderPlotStyle = version != DRW::AC1014;
            const bool graphReadback = wroteCalibration
                && verifyGraph(outputPath, expectedCloning,
                               expectHeaderPlotStyle);
            if (!wroteCalibration || !graphReadback) {
                std::cerr << "directTyped=" << directTyped
                          << " target=" << static_cast<int>(version)
                          << " binary=" << binary
                          << " fileExport=" << wroteCalibration
                          << " callback=" << exporter.reached
                          << " graphReadback=" << graphReadback << '\n';
                return fail("allocation calibration did not read back graph");
            }
            const long allocationCount = exporter.reached;
            if (allocationCount <= 0)
                return fail("calibration observed no callback allocations");
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream previous(
                        outputPath, std::ios::binary | std::ios::trunc);
                    previous << sentinel;
                    if (!previous)
                        return fail("could not initialize destination sentinel");
                }
                exporter.failAt = offset;
                exporter.reached = 0;
                const bool wrote = write(version, binary);
                exporter.failAt = -1;
                std::string current;
                if (wrote || exporter.reached < 1
                    || !readAll(outputPath, current) || current != sentinel
                    || !sourceUnchanged(*activeData, sourceDictionary,
                                        sourcePlaceholder,
                                        rawDictionaryBaseline,
                                        expectHeaderPlotStyle))
                    return fail(
                        "injected failure changed destination or source graph");
                if (!write(version, binary)
                    || !verifyGraph(outputPath, expectedCloning,
                                    expectHeaderPlotStyle)
                    || !sourceUnchanged(*activeData, sourceDictionary,
                                        sourcePlaceholder,
                                        rawDictionaryBaseline,
                                        expectHeaderPlotStyle))
                    return fail(
                        "same-adapter retry failed typed graph readback");
            }
            std::cout << "DICTIONARYWDFLT/PLACEHOLDER "
                      << (directTyped ? "typed direct " : "raw adapter ")
                      << static_cast<int>(version) << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
        }
      }
    }
    std::filesystem::remove(sourcePath, error);
    std::filesystem::remove(ac1014SourcePath, error);
    std::filesystem::remove(outputPath, error);
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
        std::cerr << "usage: dictionary_default_objects_allocation_tests SCRATCH_DIR\n";
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
