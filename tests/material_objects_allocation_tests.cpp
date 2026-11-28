// SPDX-License-Identifier: GPL-2.0-or-later
// Generated MATERIAL OBJECTS allocation-failure rollback and retry control.
#include "dx_iface.h"
#include "intern/dwgbuffer.h"
#include "intern/dwg_fixed_handles.h"
#include "material_dwg_test_helpers.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace {
long allocationsBeforeFailure = -1;
std::size_t failedAllocations = 0;

struct AllocationFailure {
    explicit AllocationFailure(long count) { allocationsBeforeFailure = count; }
    ~AllocationFailure() { allocationsBeforeFailure = -1; }
};

class MaterialExport final : public dx_iface {
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
    material_dwg_test::Frame frame;
    material_dwg_test::CodecProbe parsed;
    if (!material_dwg_test::makeFrame({}, frame)
        || !material_dwg_test::parse(frame, parsed))
        return false;

    data.sourceFormat = dx_source_format::DWG;
    data.sourceVersion = DRW::AC1024;
    data.importGeneration = 1u;
    data.dwgReadOutcome.attempted = true;
    data.dwgReadOutcome.operationSucceeded = true;
    data.dwgReadOutcome.sourceVersion = DRW::AC1024;
    data.dwgReadOutcome.observations.frames.status =
        DRW_DwgFrameCoverageStatus::FinalizedComplete;
    data.dwgReadOutcome.sourceCodePageFieldPresent = true;
    data.dwgReadOutcome.sourceCodePageRecognized = true;
    data.hasNamedObjectDictionary = true;
    data.namedObjectDictionary.handle = DRW::DwgNamedObjectsDictionaryHandle;
    data.namedObjectDictionary.m_entries = {{"ACAD_MATERIAL", 0x72u}};

    DRW_Dictionary materialDictionary;
    materialDictionary.handle = 0x72u;
    materialDictionary.parentHandle = data.namedObjectDictionary.handle;
    materialDictionary.cloning = 2;
    materialDictionary.hardOwner = 1;
    materialDictionary.m_entries = {{"ByBlock", 0x97u}};
    data.dictionaries.push_back(std::move(materialDictionary));

    DRW_Dictionary extensionDictionary;
    extensionDictionary.handle = 0x110u;
    extensionDictionary.parentHandle = 0x97u;
    extensionDictionary.cloning = 2;
    extensionDictionary.hardOwner = 1;
    data.dictionaries.push_back(std::move(extensionDictionary));

    dx_iface sourceCapture;
    sourceCapture.cData = &data;
    DRW_DwgFramePublication ltypeControl;
    ltypeControl.m_version = DRW::AC1024;
    ltypeControl.m_handle = 0x401u;
    ltypeControl.m_sourceOffset = 0x120u;
    ltypeControl.m_sourceMapOrdinal = 1u;
    ltypeControl.m_sourceOffsetSpace =
        DRW_DwgFrameOffsetSpace::PhysicalFile;
    ltypeControl.m_hasSourceLocation = true;
    ltypeControl.m_encodedType = DRW::DwgLTypeControlObjectType;
    ltypeControl.m_resolvedType = DRW::DwgLTypeControlObjectType;
    ltypeControl.m_recordName = "LTYPE_CONTROL";
    ltypeControl.m_commonLinkEvidence =
        DRW_DwgCommonLinkEvidence::NotApplicable;
    ltypeControl.m_carrier = DRW_DwgFramePublication::Carrier::Control;
    ltypeControl.m_controlHandles = {
        DRW::DwgLtypeContinuousHandle,
        0x402u,
        DRW::DwgLtypeByBlockHandle,
        DRW::DwgLtypeByLayerHandle};
    sourceCapture.addDwgFramePublication(ltypeControl);
    for (const auto& row : {
             std::pair<std::uint32_t, const char*>{
                 DRW::DwgLtypeContinuousHandle, "Continuous"},
             {DRW::DwgLtypeByBlockHandle, "ByBlock"},
             {DRW::DwgLtypeByLayerHandle, "ByLayer"},
             {0x402u, "MATERIAL_TEST"}}) {
        DRW_LType lineType;
        lineType.handle = row.first;
        lineType.parentHandle = ltypeControl.m_handle;
        lineType.name = row.second;
        sourceCapture.addLType(lineType);
    }

    parsed.handle = 0x97u;
    parsed.parentHandle = 0x72u;
    parsed.reactorHandles = {0x72u};
    parsed.xDictHandle = 0x110u;
    data.materials.push_back(static_cast<const DRW_Material&>(parsed));
    return true;
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
        || data.materials.size() != 1u || !data.hasNamedObjectDictionary)
        return false;
    const DRW_Material& material = data.materials.front();
    const auto root = std::find_if(data.namedObjectDictionary.m_entries.begin(),
        data.namedObjectDictionary.m_entries.end(),
        [](const DRW_Dictionary::Entry& entry) {
            return entry.m_name == "ACAD_MATERIAL" && entry.m_handle == 0x72u;
        });
    const auto dictionary = std::find_if(data.dictionaries.begin(),
        data.dictionaries.end(), [](const DRW_Dictionary& value) {
            return value.handle == 0x72u && value.parentHandle == 0xCu;
        });
    return root != data.namedObjectDictionary.m_entries.end()
        && dictionary != data.dictionaries.end()
        && dictionary->m_entries.size() == 1u
        && dictionary->m_entries.front().m_name == "ByBlock"
        && dictionary->m_entries.front().m_handle == material.handle
        && material.handle == 0x97u
        && material.parentHandle == 0x72u
        && material.reactorHandles == std::vector<std::uint32_t>{0x72u}
        && material.xDictHandle != 0u
        && material.m_name == "ByBlock"
        && material.m_description == "generated AC1024 MATERIAL"
        && material.m_diffuseColorValue == 0x89ABCDEFu
        && material.m_diffuseMapFileName == "diffuse-map.png"
        && material.m_refractionMapFileName == "refraction-map.png";
}

bool sourceUnchanged(const dx_data& data, const DRW_Material& expected) {
    if (data.materials.size() != 1u)
        return false;
    const DRW_Material& actual = data.materials.front();
    return actual.handle == expected.handle
        && actual.parentHandle == expected.parentHandle
        && actual.reactorHandles == expected.reactorHandles
        && actual.xDictHandle == expected.xDictHandle
        && actual.m_name == expected.m_name
        && actual.m_description == expected.m_description
        && actual.m_ambientColorMethod == expected.m_ambientColorMethod
        && actual.m_ambientColorFactor == expected.m_ambientColorFactor
        && actual.m_ambientColorValue == expected.m_ambientColorValue
        && actual.m_diffuseColorValue == expected.m_diffuseColorValue
        && actual.m_diffuseColorFactor == expected.m_diffuseColorFactor
        && actual.m_specularColorMethod == expected.m_specularColorMethod
        && actual.m_specularColorFactor == expected.m_specularColorFactor
        && actual.m_specularColorValue == expected.m_specularColorValue
        && actual.m_diffuseMapBlendFactor == expected.m_diffuseMapBlendFactor
        && actual.m_diffuseMapSource == expected.m_diffuseMapSource
        && actual.m_diffuseMapFileName == expected.m_diffuseMapFileName
        && actual.m_diffuseMapProjection == expected.m_diffuseMapProjection
        && actual.m_diffuseMapTiling == expected.m_diffuseMapTiling
        && actual.m_diffuseMapAutoTransform == expected.m_diffuseMapAutoTransform
        && actual.m_diffuseMapTransform == expected.m_diffuseMapTransform
        && actual.m_specularGlossFactor == expected.m_specularGlossFactor
        && actual.m_specularMapBlendFactor == expected.m_specularMapBlendFactor
        && actual.m_specularMapSource == expected.m_specularMapSource
        && actual.m_specularMapFileName == expected.m_specularMapFileName
        && actual.m_specularMapProjection == expected.m_specularMapProjection
        && actual.m_specularMapTiling == expected.m_specularMapTiling
        && actual.m_specularMapAutoTransform == expected.m_specularMapAutoTransform
        && actual.m_specularMapTransform == expected.m_specularMapTransform
        && actual.m_reflectionMapBlendFactor == expected.m_reflectionMapBlendFactor
        && actual.m_reflectionMapSource == expected.m_reflectionMapSource
        && actual.m_reflectionMapFileName == expected.m_reflectionMapFileName
        && actual.m_reflectionMapProjection == expected.m_reflectionMapProjection
        && actual.m_reflectionMapTiling == expected.m_reflectionMapTiling
        && actual.m_reflectionMapAutoTransform == expected.m_reflectionMapAutoTransform
        && actual.m_reflectionMapTransform == expected.m_reflectionMapTransform
        && actual.m_opacityPercent == expected.m_opacityPercent
        && actual.m_opacityMapBlendFactor == expected.m_opacityMapBlendFactor
        && actual.m_opacityMapSource == expected.m_opacityMapSource
        && actual.m_opacityMapFileName == expected.m_opacityMapFileName
        && actual.m_opacityMapProjection == expected.m_opacityMapProjection
        && actual.m_opacityMapTiling == expected.m_opacityMapTiling
        && actual.m_opacityMapAutoTransform == expected.m_opacityMapAutoTransform
        && actual.m_opacityMapTransform == expected.m_opacityMapTransform
        && actual.m_bumpMapBlendFactor == expected.m_bumpMapBlendFactor
        && actual.m_bumpMapSource == expected.m_bumpMapSource
        && actual.m_bumpMapFileName == expected.m_bumpMapFileName
        && actual.m_bumpMapProjection == expected.m_bumpMapProjection
        && actual.m_bumpMapTiling == expected.m_bumpMapTiling
        && actual.m_bumpMapAutoTransform == expected.m_bumpMapAutoTransform
        && actual.m_bumpMapTransform == expected.m_bumpMapTransform
        && actual.m_refractionIndex == expected.m_refractionIndex
        && actual.m_refractionMapBlendFactor == expected.m_refractionMapBlendFactor
        && actual.m_refractionMapSource == expected.m_refractionMapSource
        && actual.m_refractionMapFileName == expected.m_refractionMapFileName
        && actual.m_refractionMapProjection == expected.m_refractionMapProjection
        && actual.m_refractionMapTiling == expected.m_refractionMapTiling
        && actual.m_refractionMapAutoTransform == expected.m_refractionMapAutoTransform
        && actual.m_refractionMapTransform == expected.m_refractionMapTransform
        && actual.m_translucence == expected.m_translucence
        && actual.m_reflectivity == expected.m_reflectivity
        && actual.m_illuminationModel == expected.m_illuminationModel
        && actual.m_channelFlags == expected.m_channelFlags;
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "MATERIAL OBJECTS allocation retry: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    dx_data data;
    if (!makeData(data))
        return fail("generated AC1024 MATERIAL graph did not initialize");
    const DRW_Material source = data.materials.front();
    MaterialExport exporter;
    const auto output = directory / "material-objects.dxf";
    const std::string sentinel = "prior MATERIAL destination bytes\n";
    auto write = [&](DRW::Version version, bool binary) {
        return exporter.fileExport(output.string(), version, binary,
            &data, false, true, dx_dxfSourceTextPolicy::Compatible,
            DRW::DxfTextWritePolicy::Compatible,
            dx_dxfProxyGraphicsPolicy::Compatible,
            dx_dwgReadCompletenessPolicy::AllowPartial);
    };

    {
        std::ofstream prior(output, std::ios::binary | std::ios::trunc);
        prior << sentinel;
        if (!prior)
            return fail("could not initialize AC1014 refusal sentinel");
    }
    exporter.failAt = -1;
    std::string refusalBytes;
    if (write(DRW::AC1014, false) || !readAll(output, refusalBytes)
        || refusalBytes != sentinel || !sourceUnchanged(data, source))
        return fail("AC1014 down-save refusal changed output or source MATERIAL");

    for (const DRW::Version version :
         {DRW::AC1015, DRW::AC1018, DRW::AC1021, DRW::AC1024,
          DRW::AC1027, DRW::AC1032}) {
        for (const bool binary : {false, true}) {
            exporter.failAt = 1000000;
            if (!write(version, binary) || !verifyGraph(output))
                return fail("allocation calibration did not read back MATERIAL graph");
            const long allocationCount = exporter.reached;
            if (allocationCount <= 0)
                return fail("calibration observed no OBJECTS callback allocations");

            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior)
                        return fail("could not initialize destination sentinel");
                }
                exporter.failAt = offset;
                exporter.reached = 0;
                const bool wrote = write(version, binary);
                exporter.failAt = -1;
                std::string current;
                if (wrote || exporter.reached < 1
                    || !readAll(output, current) || current != sentinel
                    || !sourceUnchanged(data, source))
                    return fail("injected failure changed destination or source MATERIAL");
                if (!write(version, binary) || !verifyGraph(output)
                    || !sourceUnchanged(data, source))
                    return fail("same-adapter retry failed MATERIAL readback");
            }
            std::cout << "MATERIAL OBJECTS target="
                      << static_cast<int>(version) << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
        }
    }
    std::error_code ignored;
    std::filesystem::remove(output, ignored);
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
        std::cerr << "usage: material_objects_allocation_tests SCRATCH_DIR\n";
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
