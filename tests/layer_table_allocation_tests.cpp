// SPDX-License-Identifier: GPL-2.0-or-later
// Generated LAYER table-record allocation rollback/retry control.
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include "../src/intern/dxf_ac1027_bound_layer.h"

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
std::size_t allocationAttempts = 0u;
bool allocationFailed = false;

void armAllocationFailure(long offset) noexcept {
    allocationsBeforeFailure = offset;
    allocationAttempts = 0u;
    allocationFailed = false;
}
void disarmAllocationFailure() noexcept { allocationsBeforeFailure = -1; }

DRW_Layer makeLayer(DRW::Version version) {
    DRW_Layer layer;
    layer.handle = 0xD720u;
    layer.name = "Generated allocation retry LAYER";
    layer.flags = 4;
    layer.color = 3;
    layer.color24 = version <= DRW::AC1015 ? -1 : 0x336699;
    layer.lineType = "CONTINUOUS";
    layer.plotF = version <= DRW::AC1014;
    layer.lWeight = version <= DRW::AC1014
        ? DRW_LW_Conv::widthDefault : DRW_LW_Conv::width11;
    if (version >= DRW::AC1021)
        layer.colorName = "generated-layer-color";
    return layer;
}

bool sameLayer(const DRW_Layer& actual, DRW::Version version,
               bool readback = false) {
    const DRW_Layer expected = makeLayer(version);
    return actual.tType == DRW::LAYER
        && (readback ? actual.handle != 0u : actual.handle == expected.handle)
        && actual.name == expected.name
        && actual.flags == expected.flags
        && actual.color == expected.color
        && actual.lineType == expected.lineType
        && actual.color24 == expected.color24
        && actual.plotF == expected.plotF
        && actual.lWeight == expected.lWeight
        && actual.colorName == expected.colorName;
}

class Export final : public LC3CompatCheck {
public:
    Export(dxfRW& codec, DRW::Version version,
           const std::string& plotStyle = {})
        : codec(codec), version(version), layer(makeLayer(version)) {
        layer.handlePlotS = plotStyle;
    }
    void writeHeader(DRW_Header&) override {}
    void writeLayers() override {
        if (failAt < 0) {
            accepted = codec.writeLayer(&layer);
            return;
        }
        armAllocationFailure(failAt);
        try {
            accepted = codec.writeLayer(&layer);
        } catch (const std::bad_alloc&) {
            accepted = false;
        }
        reached = static_cast<long>(allocationAttempts);
        disarmAllocationFailure();
        if (!accepted)
            (void)codec.writeRawDxfObject(nullptr);
    }
    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    DRW::Version version;
    DRW_Layer layer;
    long failAt = -1;
    long reached = 0;
    bool accepted = false;
};

class Reader final : public LC3CompatCheck {
public:
    void addLayer(const DRW_Layer& value) override {
        if (value.name == "Generated allocation retry LAYER"
            || value.name == "XREF|External"
            || value.name == "XREF|EXTERNAL"
            || value.name == "Bound plot style")
            layers.push_back(value);
    }
    std::vector<DRW_Layer> layers;
};

const char* versionCode(DRW::Version version) {
    switch (version) {
    case DRW::AC1014: return "AC1014";
    case DRW::AC1015: return "AC1015";
    case DRW::AC1018: return "AC1018";
    case DRW::AC1021: return "AC1021";
    case DRW::AC1024: return "AC1024";
    case DRW::AC1027: return "AC1027";
    case DRW::AC1032: return "AC1032";
    default: return "unknown";
    }
}

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

bool readbackMatches(const std::filesystem::path& path,
                     DRW::Version version) {
    dxfRW codec(path.string().c_str());
    Reader reader;
    const bool read = codec.read(&reader, false);
    if (!read || reader.layers.size() != 1u) {
        std::cerr << "LAYER readback count=" << reader.layers.size()
                  << " read=" << read << " error=" << codec.getError();
        for (const auto& layer : reader.layers)
            std::cerr << " [" << layer.name << ']';
        std::cerr << '\n';
        return false;
    }
    if (!sameLayer(reader.layers.front(), version, true)) {
        const auto& layer = reader.layers.front();
        std::cerr << "LAYER mismatch name=" << layer.name
                  << " flags=" << layer.flags << " color=" << layer.color
                  << " truecolor=" << layer.color24
                  << " ltype=" << layer.lineType << " plot=" << layer.plotF
                  << " weight=" << static_cast<int>(layer.lWeight)
                  << " colorName=" << layer.colorName << '\n';
        return false;
    }
    return true;
}

bool readbackPlotAndLineweightMatch(
    const std::filesystem::path& path, bool plot,
    DRW_LW_Conv::lineWidth lineweight) {
    dxfRW codec(path.string().c_str());
    Reader reader;
    return codec.read(&reader, false) && reader.layers.size() == 1u
        && reader.layers.front().plotF == plot
        && reader.layers.front().lWeight == lineweight;
}

bool readbackPlotStyleMatches(const std::filesystem::path& path,
                              const std::string& handle) {
    dxfRW codec(path.string().c_str());
    Reader reader;
    return codec.read(&reader, false) && reader.layers.size() == 1u
        && reader.layers.front().handlePlotS == handle;
}

bool plotStyleReferenceMapping(const std::filesystem::path& directory) {
    constexpr std::uint32_t sourceHandle = 0x123u;
    constexpr std::uint32_t targetHandle = 0x456u;
    const auto path = directory / "layer-plotstyle-closure.dxf";
    const std::string prior = "prior LAYER destination bytes\n";
    {
        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        output << prior;
        if (!output)
            return false;
    }
    {
        dxfRW codec(path.string().c_str());
        Export exporter(codec, DRW::AC1027, "123");
        const bool written = codec.write(&exporter, DRW::AC1027, false);
        std::string current;
        if (written || exporter.accepted || !readBytes(path, current)
            || current != prior) {
            std::cerr << "unresolved LAYER PlotStyleName was not refused\n";
            return false;
        }
        exporter.accepted = false;
        codec.setReferenceResolver([](std::uint32_t handle) {
            return handle == sourceHandle ? targetHandle : 0u;
        });
        const bool retried = codec.write(&exporter, DRW::AC1027, false);
        if (!retried || !exporter.accepted
            || !readbackPlotStyleMatches(path, "456")) {
            std::cerr << "LAYER PlotStyleName same-writer retry failed\n";
            return false;
        }
    }
    const auto nullPath = directory / "layer-null-plotstyle.dxf";
    {
        dxfRW codec(nullPath.string().c_str());
        Export exporter(codec, DRW::AC1027, "0");
        if (!codec.write(&exporter, DRW::AC1027, false) || !exporter.accepted
            || !readbackPlotStyleMatches(nullPath, "0")) {
            std::cerr << "explicit null LAYER PlotStyleName was not preserved\n";
            return false;
        }
    }
    std::cout << "LAYER PlotStyleName resolver mapping and null passed\n";
    return true;
}

bool xrefLayerLinetypeNormalization(
    const std::filesystem::path& directory) {
    constexpr std::array<DRW::Version, 8> versions{{
        DRW::AC1009, DRW::AC1014, DRW::AC1015, DRW::AC1018,
        DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}};
    const auto path = directory / "xref-layer-linetype.dxf";
    for (const DRW::Version version : versions) {
        for (const bool binary : {false, true}) {
            dxfRW codec(path.string().c_str());
            Export exporter(codec, version);
            exporter.layer.name = "XREF|External";
            exporter.layer.flags = 16u;
            exporter.layer.lineType = "DASHED";
            if (!codec.write(&exporter, version, binary)
                || !exporter.accepted) {
                std::cerr << "xref-dependent LAYER write failed: version="
                          << versionCode(version) << " binary=" << binary
                          << '\n';
                return false;
            }
            dxfRW readerCodec(path.string().c_str());
            Reader reader;
            if (!readerCodec.read(&reader, false) || reader.layers.size() != 1u
                || reader.layers.front().flags != 16u
                || reader.layers.front().lineType != "CONTINUOUS") {
                std::cerr << "xref-dependent LAYER linetype normalization "
                             "readback failed: version="
                          << versionCode(version) << " binary=" << binary
                          << '\n';
                return false;
            }
        }
    }
    std::cout << "xref-dependent LAYER linetype normalization passed\n";
    return true;
}

template <class BoundLayer>
class BoundPlotStyleExport final : public LC3CompatCheck {
public:
    explicit BoundPlotStyleExport(dxfRW& codec)
        : codec(codec), layer(DRW_Layer{}, 0x123u) {
        layer.name = "Bound plot style";
        layer.handlePlotS = "456";
        layer.handleMaterialS = "ABC";
    }
    void writeHeader(DRW_Header&) override {}
    void writeLayers() override { accepted = codec.writeLayer(&layer); }
    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    BoundLayer layer;
    bool accepted = false;
};

template <class BoundLayer>
bool boundPlotStyleAliasConsistency(
    const std::filesystem::path& directory, DRW::Version version,
    DRW::Version wrongVersion) {
    const auto path = directory / "bound-layer-plotstyle-alias.dxf";
    const std::string prior = "prior LAYER destination bytes\n";
    for (const bool binary : {false, true}) {
        {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output << prior;
            if (!output)
                return false;
        }
        dxfRW codec(path.string().c_str());
        codec.setSourceBlockRecords(std::vector<DRW_Block_Record>{}, true);
        BoundPlotStyleExport<BoundLayer> exporter(codec);
        const bool written = codec.write(&exporter, version, binary);
        std::string current;
        if (written || exporter.accepted || !readBytes(path, current)
            || current != prior || exporter.layer.handlePlotS != "456") {
            std::cerr << "conflicting bound LAYER plot-style alias was not "
                         "refused\n";
            return false;
        }
        exporter.layer.handlePlotS.clear();
        codec.setReferenceResolver([](std::uint32_t handle) {
            return handle == 0x123u ? 0x789u : handle == 0xABCu ? 0xDEFu : handle;
        });
        exporter.accepted = false;
        if (codec.write(&exporter, wrongVersion, binary) || exporter.accepted
            || !readBytes(path, current) || current != prior) {
            std::cerr << "wrong-version bound LAYER was not refused\n";
            return false;
        }
        exporter.accepted = false;
        const bool retried = codec.write(&exporter, version, binary);
        dxfRW readerCodec(path.string().c_str());
        Reader reader;
        if (!retried || !exporter.accepted
            || !readerCodec.read(&reader, false) || reader.layers.size() != 1u
            || reader.layers.front().handlePlotS != "789"
            || reader.layers.front().handleMaterialS != "DEF") {
            std::cerr << "corrected bound LAYER plot-style retry failed\n";
            return false;
        }
    }
    std::cout << "bound LAYER PlotStyleName alias refusal and retry passed\n";
    return true;
}

bool allocationSweep(const std::filesystem::path& directory) {
    const auto path = directory / "layer-table-allocation.dxf";
    const std::string prior = "prior LAYER destination bytes\n";
    constexpr std::array<DRW::Version, 7> versions{{
        DRW::AC1014, DRW::AC1015, DRW::AC1018, DRW::AC1021,
        DRW::AC1024, DRW::AC1027, DRW::AC1032}};
    for (const DRW::Version version : versions) {
        for (const bool binary : {false, true}) {
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(path.string().c_str());
                Export exporter(codec, version);
                exporter.failAt = 1000000;
                const bool written = codec.write(&exporter, version, binary);
                const bool sourceMatches = sameLayer(exporter.layer, version);
                const bool readback = readbackMatches(path, version);
                if (!written || !exporter.accepted || !sourceMatches
                    || !readback || !readBytes(path, expectedBytes)) {
                    std::cerr << "clean LAYER write failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " written=" << written
                              << " accepted=" << exporter.accepted
                              << " source=" << sourceMatches
                              << " readback=" << readback << '\n';
                    return false;
                }
                allocationCount = exporter.reached;
            }
            if (allocationCount <= 0)
                return false;
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream output(path,
                                         std::ios::binary | std::ios::trunc);
                    output << prior;
                    if (!output)
                        return false;
                }
                dxfRW codec(path.string().c_str());
                Export exporter(codec, version);
                exporter.failAt = offset;
                const bool written = codec.write(&exporter, version, binary);
                exporter.failAt = -1;
                std::string current;
                if (written || exporter.accepted
                    || exporter.reached != offset + 1
                    || !allocationFailed || !readBytes(path, current)
                    || current != prior || !sameLayer(exporter.layer, version)) {
                    std::cerr << "LAYER rollback failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " offset=" << offset << " written=" << written
                              << " accepted=" << exporter.accepted
                              << " reached=" << exporter.reached
                              << " failed=" << allocationFailed << '\n';
                    return false;
                }
                const bool retried = codec.write(&exporter, version, binary);
                std::string retryBytes;
                if (!retried || !exporter.accepted
                    || !sameLayer(exporter.layer, version)
                    || !readBytes(path, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(path, version)) {
                    std::cerr << "LAYER retry failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " offset=" << offset << '\n';
                    return false;
                }
            }
            std::cout << "LAYER " << versionCode(version)
                      << (binary ? " Binary" : " ASCII")
                      << " table-writer allocation offsets="
                      << allocationCount << '\n';
        }
    }
    return true;
}

bool preR2000LayerLossRefusalAndRetry(
    const std::filesystem::path& directory) {
    const auto path = directory / "layer-r13-loss-refusal.dxf";
    const std::string prior = "prior LAYER destination bytes\n";
    for (const bool binary : {false, true}) {
        for (const bool nondefaultPlot : {false, true}) {
            {
                std::ofstream output(path, std::ios::binary | std::ios::trunc);
                output << prior;
                if (!output)
                    return false;
            }

            dxfRW codec(path.string().c_str());
            Export exporter(codec, DRW::AC1014);
            if (nondefaultPlot) {
                exporter.layer.plotF = false;
                exporter.layer.lWeight = DRW_LW_Conv::widthDefault;
            } else {
                exporter.layer.plotF = true;
                exporter.layer.lWeight = DRW_LW_Conv::width11;
            }
            const bool inputPlot = exporter.layer.plotF;
            const auto inputLineweight = exporter.layer.lWeight;
            const bool wroteLegacy = codec.write(
                &exporter, DRW::AC1014, binary);
            std::string afterRefusal;
            if (wroteLegacy || exporter.accepted
                || exporter.layer.plotF != inputPlot
                || exporter.layer.lWeight != inputLineweight
                || !readBytes(path, afterRefusal) || afterRefusal != prior) {
                std::cerr << "pre-R2000 LAYER loss refusal failed: binary="
                          << binary << " plot=" << nondefaultPlot
                          << " written=" << wroteLegacy
                          << " accepted=" << exporter.accepted << '\n';
                return false;
            }

            exporter.version = DRW::AC1015;
            const bool retried = codec.write(
                &exporter, DRW::AC1015, binary);
            if (!retried || !exporter.accepted
                || !readbackPlotAndLineweightMatch(
                    path, inputPlot, inputLineweight)) {
                std::cerr << "R2000 LAYER corrected retry failed: binary="
                          << binary << " plot=" << nondefaultPlot << '\n';
                return false;
            }
        }
    }
    return true;
}

bool preR2000PlotStyleRefusalAndRetry(
    const std::filesystem::path& directory) {
    constexpr std::uint32_t sourceHandle = 0x123u;
    constexpr std::uint32_t targetHandle = 0x456u;
    const auto path = directory / "layer-r13-plotstyle-refusal.dxf";
    const std::string prior = "prior LAYER destination bytes\n";
    for (const bool binary : {false, true}) {
        {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output << prior;
            if (!output)
                return false;
        }
        dxfRW codec(path.string().c_str());
        Export exporter(codec, DRW::AC1014, "123");
        const bool written = codec.write(&exporter, DRW::AC1014, binary);
        std::string current;
        if (written || exporter.accepted || !readBytes(path, current)
            || current != prior || exporter.layer.handlePlotS != "123") {
            std::cerr << "pre-R2000 LAYER plot-style loss was not refused\n";
            return false;
        }
        codec.setReferenceResolver([](std::uint32_t handle) {
            return handle == sourceHandle ? targetHandle : 0u;
        });
        const bool retried = codec.write(&exporter, DRW::AC1015, binary);
        if (!retried || !exporter.accepted
            || !readbackPlotStyleMatches(path, "456")) {
            std::cerr << "R2000 LAYER plot-style retry failed: binary="
                      << binary << '\n';
            return false;
        }
    }
    std::cout << "pre-R2000 LAYER PlotStyleName refusal and retry passed\n";
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (allocationsBeforeFailure == 0) {
        ++allocationAttempts;
        allocationsBeforeFailure = -1;
        allocationFailed = true;
        throw std::bad_alloc();
    }
    if (allocationsBeforeFailure > 0) {
        --allocationsBeforeFailure;
        ++allocationAttempts;
    }
    if (void* value = std::malloc(size == 0u ? 1u : size))
        return value;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete(void* value, std::size_t) noexcept { std::free(value); }
void operator delete[](void* value, std::size_t) noexcept { std::free(value); }

int main() {
    const auto directory = std::filesystem::temp_directory_path()
        / ("layer-table-allocation-"
           + std::to_string(static_cast<long long>(
                 std::filesystem::file_time_type::clock::now()
                     .time_since_epoch().count())));
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return 1;
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
    } cleanup{directory};
    return allocationSweep(directory)
            && preR2000LayerLossRefusalAndRetry(directory)
            && preR2000PlotStyleRefusalAndRetry(directory)
            && plotStyleReferenceMapping(directory)
            && xrefLayerLinetypeNormalization(directory)
            && boundPlotStyleAliasConsistency<DRW_Ac1027BoundLayer>(directory, DRW::AC1027, DRW::AC1024)
            && boundPlotStyleAliasConsistency<DRW_Ac1024BoundLayer>(directory, DRW::AC1024, DRW::AC1027)
        ? 0 : 2;
}
