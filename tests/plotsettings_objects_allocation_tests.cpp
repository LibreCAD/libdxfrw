// SPDX-License-Identifier: GPL-2.0-or-later
// Generated PLOTSETTINGS OBJECTS allocation-failure rollback and retry control.
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

class PlotSettingsExport final : public dx_iface {
public:
    long failAt = -1;
    long reached = 0;

    void writeObjects() override {
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

bool makeData(dx_data& data) {
    const std::string source =
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1027\n"
        "0\nENDSEC\n0\nSECTION\n2\nOBJECTS\n"
        "0\nDICTIONARY\n5\nC\n330\n0\n100\nAcDbDictionary\n"
        "280\n1\n281\n1\n3\nACAD_PLOTSETTINGS\n350\n1E\n"
        "0\nDICTIONARY\n5\n1E\n330\nC\n100\nAcDbDictionary\n"
        "280\n1\n281\n1\n3\nGENERATED_SETUP\n350\nA720\n"
        "0\nPLOTSETTINGS\n5\nA720\n330\n1E\n"
        "100\nAcDbObject\n100\nAcDbPlotSettings\n"
        "1\nGENERATED_SETUP\n2\nDWG To PDF.pc3\n4\nISO_A4\n"
        "40\n1.25\n41\n2.5\n44\n210.0\n45\n297.0\n"
        "70\n3\n72\n1\n73\n0\n74\n5\n"
        "0\nENDSEC\n0\nEOF\n";
    PlotSettingsExport parser;
    parser.cData = &data;
    std::string mutableSource = source;
    dxfRW reader("generated-plotsettings-allocation-control");
    if (!reader.readAscii(&parser, true, mutableSource))
        return false;
    data.sourceFormat = dx_source_format::DXF;
    return data.plotSettings.size() == 1u
        && data.hasNamedObjectDictionary
        && data.namedObjectDictionary.handle == 0xCu
        && data.namedObjectDictionary.m_entries.size() == 1u
        && data.namedObjectDictionary.m_entries.front().m_name
            == "ACAD_PLOTSETTINGS"
        && data.namedObjectDictionary.m_entries.front().m_handle == 0x1Eu
        && data.plotSettings.front().handle == 0xA720u
        && data.plotSettings.front().parentHandle == 0x1Eu
        && data.plotSettings.front().pageSetupName == "GENERATED_SETUP"
        && data.plotSettings.front().printerConfig == "DWG To PDF.pc3"
        && data.plotSettings.front().paperSize == "ISO_A4"
        && data.plotSettings.front().marginLeft == 1.25
        && data.plotSettings.front().marginBottom == 2.5
        && data.plotSettings.front().paperWidth == 210.0
        && data.plotSettings.front().paperHeight == 297.0;
}

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
        || !data.hasNamedObjectDictionary || data.plotSettings.size() != 1u)
        return false;
    const auto root = std::find_if(data.namedObjectDictionary.m_entries.begin(),
        data.namedObjectDictionary.m_entries.end(),
        [](const DRW_Dictionary::Entry& entry) {
            return entry.m_name == "ACAD_PLOTSETTINGS";
        });
    if (root == data.namedObjectDictionary.m_entries.end())
        return false;
    const auto dictionary = std::find_if(data.dictionaries.begin(),
        data.dictionaries.end(), [&data, &root](const DRW_Dictionary& value) {
            return value.handle == root->m_handle
                && value.parentHandle == data.namedObjectDictionary.handle;
        });
    if (dictionary == data.dictionaries.end()
        || dictionary->m_entries.size() != 1u
        || dictionary->m_entries.front().m_name != "GENERATED_SETUP")
        return false;
    const DRW_PlotSettings& settings = data.plotSettings.front();
    return dictionary->m_entries.front().m_handle == settings.handle
        && settings.parentHandle == dictionary->handle
        && settings.pageSetupName == "GENERATED_SETUP"
        && settings.printerConfig == "DWG To PDF.pc3"
        && settings.paperSize == "ISO_A4"
        && settings.marginLeft == 1.25 && settings.marginBottom == 2.5
        && settings.paperWidth == 210.0 && settings.paperHeight == 297.0
        && settings.plotLayoutFlags == 3 && settings.paperUnits == 1
        && settings.plotRotation == 0 && settings.plotType == 5;
}

bool sourceUnchanged(const dx_data& data, const DRW_PlotSettings& expected) {
    if (!data.hasNamedObjectDictionary || data.plotSettings.size() != 1u
        || data.namedObjectDictionary.handle != 0xCu
        || data.namedObjectDictionary.m_entries.size() != 1u
        || data.namedObjectDictionary.m_entries.front().m_name
            != "ACAD_PLOTSETTINGS"
        || data.namedObjectDictionary.m_entries.front().m_handle != 0x1Eu)
        return false;
    const DRW_PlotSettings& actual = data.plotSettings.front();
    return actual.handle == expected.handle
        && actual.parentHandle == expected.parentHandle
        && actual.name == expected.name
        && actual.flags == expected.flags
        && actual.reactorHandles == expected.reactorHandles
        && actual.xDictHandle == expected.xDictHandle
        && actual.pageSetupName == expected.pageSetupName
        && actual.printerConfig == expected.printerConfig
        && actual.paperSize == expected.paperSize
        && actual.plotLayoutFlags == expected.plotLayoutFlags
        && actual.marginLeft == expected.marginLeft
        && actual.marginBottom == expected.marginBottom
        && actual.marginRight == expected.marginRight
        && actual.marginTop == expected.marginTop
        && actual.paperWidth == expected.paperWidth
        && actual.paperHeight == expected.paperHeight
        && actual.plotOriginX == expected.plotOriginX
        && actual.plotOriginY == expected.plotOriginY
        && actual.paperUnits == expected.paperUnits
        && actual.plotRotation == expected.plotRotation
        && actual.plotType == expected.plotType
        && actual.windowMinX == expected.windowMinX
        && actual.windowMinY == expected.windowMinY
        && actual.windowMaxX == expected.windowMaxX
        && actual.windowMaxY == expected.windowMaxY
        && actual.plotViewName == expected.plotViewName
        && actual.realWorldUnits == expected.realWorldUnits
        && actual.drawingUnits == expected.drawingUnits
        && actual.currentStyleSheet == expected.currentStyleSheet
        && actual.scaleType == expected.scaleType
        && actual.scaleFactor == expected.scaleFactor
        && actual.paperImageOriginX == expected.paperImageOriginX
        && actual.paperImageOriginY == expected.paperImageOriginY
        && actual.shadePlotMode == expected.shadePlotMode
        && actual.shadePlotResLevel == expected.shadePlotResLevel
        && actual.shadePlotCustomDPI == expected.shadePlotCustomDPI
        && actual.plotViewHandle.code == expected.plotViewHandle.code
        && actual.plotViewHandle.ref == expected.plotViewHandle.ref
        && actual.plotViewHandle.ref64 == expected.plotViewHandle.ref64
        && actual.shadePlotHandle.code == expected.shadePlotHandle.code
        && actual.shadePlotHandle.ref == expected.shadePlotHandle.ref
        && actual.shadePlotHandle.ref64 == expected.shadePlotHandle.ref64
        && actual.dwgEedAppIdReferences.size()
            == expected.dwgEedAppIdReferences.size()
        && actual.dwgEedLayerReferences.size()
            == expected.dwgEedLayerReferences.size()
        && data.dictionaries.size() == 1u
        && data.dictionaries.front().handle == 0x1Eu
        && data.dictionaries.front().parentHandle == 0xCu
        && data.dictionaries.front().m_entries.size() == 1u
        && data.dictionaries.front().m_entries.front().m_name
            == "GENERATED_SETUP"
        && data.dictionaries.front().m_entries.front().m_handle == 0xA720u;
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "PLOTSETTINGS OBJECTS allocation retry: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    dx_data data;
    if (!makeData(data))
        return fail("generated PLOTSETTINGS graph did not initialize");
    const DRW_PlotSettings source = data.plotSettings.front();
    const std::string sentinel = "prior PLOTSETTINGS destination bytes\n";
    for (const DRW::Version version : {
             DRW::AC1015, DRW::AC1018, DRW::AC1021, DRW::AC1024,
             DRW::AC1027, DRW::AC1032}) {
      PlotSettingsExport exporter;
      const auto output = directory
          / ("plotsettings-objects-"
             + std::to_string(static_cast<int>(version)) + ".dxf");
      const auto write = [&exporter, &data, &output, version](bool binary) {
          return exporter.fileExport(output.string(), version, binary,
              &data, false);
      };
      for (const bool binary : {false, true}) {
        exporter.failAt = 1000000;
        if (!write(binary) || !verifyGraph(output))
            return fail("allocation calibration did not read back graph");
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
            const bool wrote = write(binary);
            exporter.failAt = -1;
            std::string current;
            if (wrote || exporter.reached < 1
                || !readAll(output, current) || current != sentinel
                || !sourceUnchanged(data, source))
                return fail("injected failure changed destination or source graph");
            if (!write(binary) || !verifyGraph(output)
                || !sourceUnchanged(data, source))
                return fail("same-adapter retry failed typed graph readback");
        }
        std::cout << "PLOTSETTINGS OBJECTS "
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
        std::cerr << "usage: plotsettings_objects_allocation_tests SCRATCH_DIR\n";
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
