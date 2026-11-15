// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_iface.h"
#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

class DxIfaceStandardDwgCaptureTestAccess {
public:
    static bool complete(const dx_iface& iface, const dx_data& data) {
        return iface.hasCompleteAc1024LayerSourceEvidence(data);
    }
    static bool materialComplete(const dx_iface& iface, const dx_data& data) {
        return iface.hasCompleteAc1024DefaultMaterialSourceEvidence(data);
    }
    static bool outputInspected(const dx_iface& iface) {
        return iface.m_ac1024DefaultLayersInspected;
    }
    static DRW_UnsupportedObject& materialFrame(dx_data& data) {
        return data.m_opaqueDwgSourceFrames.at({data.importGeneration, data.materials.front().handle});
    }
    static DRW_UnsupportedObject& plotFrame(dx_data& data) {
        return data.m_opaqueDwgSourceFrames.at(
            {data.importGeneration, data.dictionaryWithDefaults.front().handle});
    }
    static auto& layerPublication(dx_data& data) {
        return data.m_ac1024LayerPublications.begin()->second;
    }
    static bool checkMaterialMutations(dx_iface& iface, dx_data& data) {
        if (!materialComplete(iface, data) || data.materials.size() != 3u) {
            std::cerr << "AC1024 material source evidence missing; count=" << data.materials.size() << '\n';
            return false;
        }
        auto& material = data.materials.front();
        auto source = data.m_opaqueDwgSourceFrames.find({data.importGeneration, material.handle});
        if (source == data.m_opaqueDwgSourceFrames.end() || source->second.m_rawBytes.empty()) return false;
        auto& raw = source->second.m_rawBytes.front();
        const auto savedRaw = raw;
        raw ^= 0x80u;
        if (materialComplete(iface, data)) return false;
        raw = savedRaw;
        const auto savedName = material.m_name;
        material.m_name += "-edited";
        if (materialComplete(iface, data)) return false;
        material.m_name = savedName;
        auto dictionary = std::find_if(data.dictionaries.begin(), data.dictionaries.end(),
            [&material](const DRW_Dictionary& item) { return item.handle == material.parentHandle; });
        if (dictionary == data.dictionaries.end() || dictionary->m_entries.empty()) return false;
        const auto savedHandle = dictionary->m_entries.front().m_handle;
        dictionary->m_entries.front().m_handle ^= 1u;
        if (materialComplete(iface, data)) return false;
        dictionary->m_entries.front().m_handle = savedHandle;
        auto root = std::find_if(data.namedObjectDictionary.m_entries.begin(),
            data.namedObjectDictionary.m_entries.end(), [](const DRW_Dictionary::Entry& entry) {
                return entry.m_name == "ACAD_MATERIAL";
            });
        if (root == data.namedObjectDictionary.m_entries.end()) return false;
        const auto savedRoot = root->m_handle;
        root->m_handle ^= 1u;
        if (materialComplete(iface, data)) return false;
        root->m_handle = savedRoot;
        auto frame = data.m_standardDwgObjectFrames.find({data.importGeneration, dictionary->handle});
        if (frame == data.m_standardDwgObjectFrames.end() || frame->second.raw.m_rawBytes.empty()) return false;
        auto& dictionaryByte = frame->second.raw.m_rawBytes.front();
        const auto savedDictionaryByte = dictionaryByte;
        dictionaryByte ^= 0x80u;
        if (materialComplete(iface, data)) return false;
        dictionaryByte = savedDictionaryByte;
        frame->second.payloadMatched = false;
        if (materialComplete(iface, data)) return false;
        frame->second.payloadMatched = true;
        auto classEntry = std::find_if(data.dwgClassCoverage.m_entries.begin(),
            data.dwgClassCoverage.m_entries.end(), [](const DRW_DwgClassCoverageEntry& entry) {
                return entry.m_recordName == "MATERIAL";
            });
        if (classEntry == data.dwgClassCoverage.m_entries.end()) return false;
        const auto savedCount = classEntry->m_instanceCount;
        ++classEntry->m_instanceCount;
        if (materialComplete(iface, data)) return false;
        classEntry->m_instanceCount = savedCount;
        return materialComplete(iface, data);
    }
    static bool checkMutations(dx_iface& iface, dx_data& data) {
        if (!complete(iface, data) || data.m_ac1024LayerSources.empty()
            || data.m_ac1024LayerPublications.empty() || data.layers.empty()) return false;
        auto& source = data.m_ac1024LayerSources.begin()->second;
        auto& publication = data.m_ac1024LayerPublications.at(source.m_raw.m_handle);
        const auto oldOffset = publication.offset;
        ++publication.offset;
        if (complete(iface, data)) return false;
        publication.offset = oldOffset;
        const auto oldByte = source.m_raw.m_rawBytes.front();
        source.m_raw.m_rawBytes.front() ^= 0x80u;
        if (complete(iface, data)) return false;
        source.m_raw.m_rawBytes.front() = oldByte;
        auto layer = std::find_if(data.layers.begin(), data.layers.end(),
            [&source](const DRW_Layer& item) { return item.handle == source.m_raw.m_handle; });
        if (layer == data.layers.end()) return false;
        const auto oldName = layer->name;
        layer->name += "-edited";
        if (complete(iface, data)) return false;
        layer->name = oldName;
        data.m_ac1024LayerEvidenceFailed = true;
        if (complete(iface, data)) return false;
        data.m_ac1024LayerEvidenceFailed = false;
        return complete(iface, data);
    }
};

class BindingAuthor final : public LC3CompatCheck, public DRW_DwgDefaultMaterialProvider,
                            public DRW_DwgCanonicalLayerProvider {
public:
    explicit BindingAuthor(dwgRW& output, bool material = false, int layerShape = 3)
        : output_(output), material_(material), layerShape_(layerShape) {}
    bool wantsDwgDefaultMaterialGraph() const override { return material_; }
    bool wantsDwgCanonicalLayerNulls(const DRW_Layer&) const override {
        ++canonicalCalls;
        return material_ && layerShape_ == 3;
    }
    bool dwgDefaultMaterialOpaqueReferencesAbsent(std::uint32_t dictionary,
        std::uint32_t byBlock, std::uint32_t byLayer, std::uint32_t global) const override {
        return dictionary && byBlock && byLayer && global;
    }
    void dwgDefaultMaterialGraphPlanned(std::uint32_t, std::uint32_t,
        std::uint32_t, std::uint32_t global) override { global_ = global; }
    void writeDwgClasses() override {
        if (!material_) return;
        plotDictionary_ = DRW_DictionaryWithDefault{};
        plotPlaceholder_ = DRW_AcDbPlaceholder{};
        plotDictionary_.handle = 0xA10u;
        plotDictionary_.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        plotDictionary_.cloning = 1; plotDictionary_.hardOwner = 0;
        plotDictionary_.m_entries.push_back({"Normal", 0xA11u});
        plotDictionary_.m_defaultEntryHandle = 0xA11u;
        plotDictionary_.setDwgCommonObjectState(0, 1u, false);
        plotPlaceholder_.handle = 0xA11u;
        plotPlaceholder_.parentHandle = plotDictionary_.handle;
        plotPlaceholder_.setDwgCommonObjectState(0, 1u, false);
        registered_ = output_.registerDictionaryWithDefaultObjectClass(&plotDictionary_)
            && output_.registerDwgNamedObjectDictionaryEntry(
                graphFailure == 4 ? "ALT_PLOT" : "ACAD_PLOTSTYLENAME", plotDictionary_.handle);
    }
    void writeLayers() override {
        DRW_Layer layer;
        if (material_) {
            layer.name = "0"; layer.materialHandle.ref = global_;
            layer.plotStyleHandle.ref = plotPlaceholder_.handle;
            if (layerShape_ != 3 && (layerShape_ & 1) != 0)
                layer.setDwgCommonObjectState(0, 1u, false);
            if (layerShape_ != 3 && (layerShape_ & 2) != 0)
                layer.xrefBlockHandle.code = DRW::DwgHardPointer;
            if (incompatible == 1) layer.xrefBlockHandle.ref = 0xB200u;
            if (incompatible == 2) layer.setDwgCommonLinkTailValidated(true);
            if (incompatible == 3) layer.handlePlotS = "BAD";
            if (incompatible == 4) layer.name = "Other";
            if (incompatible == 5) layer.materialHandle.ref ^= 1u;
            const bool layerZeroAdded = output_.addLayer(&layer);
            if (layerShape_ == 3)
                callerUnchanged = layer.extensionDictionaryFlag() == 0u
                    && layer.xrefBlockHandle.code == 0u
                    && layer.xrefBlockHandle.ref == (incompatible == 1 ? 0xB200u : 0u)
                    && layer.hasDwgCommonLinkTail() == (incompatible == 2)
                    && layer.handlePlotS == (incompatible == 3 ? "BAD" : "")
                    && layer.name == (incompatible == 4 ? "Other" : "0")
                    && layer.materialHandle.ref == (incompatible == 5 ? (global_ ^ 1u) : global_);
            if (!layerZeroAdded) return;
            layer.handle = 0u;
        }
        layer.name = "Defpoints"; added = output_.addLayer(&layer);
        if (material_ && layerShape_ == 3)
            callerUnchanged = callerUnchanged && layer.extensionDictionaryFlag() == 0u
                && layer.xrefBlockHandle.code == 0u && layer.xrefBlockHandle.ref == 0u;
    }
    void writeObjects() override {
        ++objectCalls;
        if (material_) {
            auto dictionary = plotDictionary_;
            auto placeholder = plotPlaceholder_;
            if (graphFailure == 2) placeholder.parentHandle ^= 1u;
            if (graphFailure == 3) dictionary.m_defaultEntryHandle = 0u;
            if (graphFailure == 5) dictionary.m_entries.front().m_name = "Other";
            if (graphFailure == 6) dictionary.m_entries.front().m_handle ^= 1u;
            if (graphFailure == 7) placeholder.setDwgCommonObjectState(0, 0u, false);
            if (graphFailure == 8) dictionary.parentHandle ^= 1u;
            if (graphFailure == 11) placeholder.setDwgCommonObjectState(1, 1u, false);
            if (graphFailure == 12) dictionary.setDwgCommonObjectState(1, 1u, false);
            if (graphFailure == 9) {
                auto bad = dictionary;
                bad.m_entries.front().m_name = "Other";
                if (output_.writeDictionaryWithDefault(&bad)) { objects_ = false; return; }
            }
            if (graphFailure == 10) {
                if (!output_.writeDictionaryWithDefault(&dictionary)) { objects_ = false; return; }
                auto bad = placeholder;
                bad.setDwgCommonObjectState(0, 0u, false);
                if (output_.writeAcDbPlaceholder(&bad)) { objects_ = false; return; }
                objects_ = output_.writeAcDbPlaceholder(&placeholder);
                return;
            }
            objects_ = output_.writeDictionaryWithDefault(&dictionary)
                && (graphFailure == 1 || output_.writeAcDbPlaceholder(&placeholder));
        }
    }
    bool added = false;
    bool registered_ = false, objects_ = false;
    int incompatible = 0;
    int graphFailure = 0;
    bool callerUnchanged = false;
    mutable unsigned canonicalCalls = 0u;
    unsigned objectCalls = 0u;
private:
    dwgRW& output_;
    bool material_;
    int layerShape_;
    std::uint32_t global_ = 0u;
    DRW_DictionaryWithDefault plotDictionary_;
    DRW_AcDbPlaceholder plotPlaceholder_;
};
static bool checkFile(const std::filesystem::path& path, bool expectMaterial) {
    dx_data data;
    dx_iface reader;
    if (!reader.fileImport(path.string(), &data, false)
        || !DxIfaceStandardDwgCaptureTestAccess::checkMutations(reader, data)
        || (expectMaterial
            ? !DxIfaceStandardDwgCaptureTestAccess::checkMaterialMutations(reader, data)
            : DxIfaceStandardDwgCaptureTestAccess::materialComplete(reader, data))) {
        std::cerr << "AC1024 layer source/publication binding failed\n";
        return false;
    }
    return true;
}

static bool checkOutput(const std::filesystem::path& input,
                        const std::filesystem::path& output, bool binary) {
    dx_data source; dx_iface reader;
    if (!reader.fileImport(input.string(), &source, false)) return false;
    class Writer final : public dx_iface {
    public:
        bool mutate = false, mutated = false;
        void writeObjects() override {
            dx_iface::writeObjects();
            if (mutate && cData != nullptr) {
                DxIfaceStandardDwgCaptureTestAccess::materialFrame(*cData).m_rawBytes.front() ^= 0x80u;
                mutated = true;
            }
        }
    } writer;
    const auto write = [&]() {
        return writer.fileExport(output.string(), DRW::AC1024, binary, &source,
            true, true, dx_dxfSourceTextPolicy::Compatible,
            DRW::DxfTextWritePolicy::Compatible, dx_dxfProxyGraphicsPolicy::Compatible,
            dx_dwgReadCompletenessPolicy::Legacy);
    };
    const auto verify = [&]() {
        dx_data actual; dx_iface imported;
        if (!imported.fileImport(output.string(), &actual, false)
            || actual.layers.size() != 2u || actual.dictionaryWithDefaults.size() != 1u
            || actual.rawPlaceholders.size() != 1u || actual.materials.size() != 3u) return false;
        const auto& plot = actual.rawPlaceholders.front();
        const auto& dict = actual.dictionaryWithDefaults.front();
        const auto global = std::find_if(actual.materials.begin(), actual.materials.end(),
            [](const DRW_Material& item) { return item.m_name == "Global"; });
        if (global == actual.materials.end() || dict.m_defaultEntryHandle != plot.handle
            || plot.parentHandle != dict.handle) return false;
        for (const auto& layer : actual.layers) {
            std::uint32_t material = 0u, plotStyle = 0u; bool hasMaterial = false, hasPlot = false;
            if (!layer.getDxfMaterialHandle(material, hasMaterial) || !hasMaterial
                || !layer.getDxfPlotStyleHandle(plotStyle, hasPlot) || !hasPlot
                || material != global->handle || plotStyle != plot.handle) return false;
        }
        return true;
    };
    if (!write() || !DxIfaceStandardDwgCaptureTestAccess::outputInspected(writer) || !verify()) {
        std::cerr << "AC1024 selected layer output failed\n"; return false;
    }
    const std::string sentinel = "keep AC1024 layer destination\n";
    const auto refuse = [&]() {
        { std::ofstream f(output, std::ios::binary | std::ios::trunc); f << sentinel; }
        if (write()) return false;
        std::ifstream f(output, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(f), {}) == sentinel;
    };
    auto& layer = source.layers.front();
    const auto savedPlot = layer.plotStyleHandle.ref;
    layer.plotStyleHandle.ref ^= 1u;
    if (!refuse()) return false;
    layer.plotStyleHandle.ref = savedPlot;
    const auto savedMaterial = layer.materialHandle.ref;
    layer.materialHandle.ref ^= 1u;
    if (!refuse()) return false;
    layer.materialHandle.ref = savedMaterial;
    auto& publication = DxIfaceStandardDwgCaptureTestAccess::layerPublication(source);
    const auto savedOffset = publication.offset;
    ++publication.offset;
    if (!refuse()) return false;
    publication.offset = savedOffset;
    auto& plotRaw = DxIfaceStandardDwgCaptureTestAccess::plotFrame(source).m_rawBytes.front();
    const auto savedPlotRaw = plotRaw;
    plotRaw ^= 0x80u;
    if (!refuse()) return false;
    plotRaw = savedPlotRaw;
    auto& raw = DxIfaceStandardDwgCaptureTestAccess::materialFrame(source).m_rawBytes.front();
    const auto savedRaw = raw;
    writer.mutate = true;
    if (!refuse() || !writer.mutated) return false;
    raw = savedRaw; writer.mutate = false;
    return write() && DxIfaceStandardDwgCaptureTestAccess::outputInspected(writer) && verify();
}
int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--physical")
        return checkFile(argv[2], true) ? 0 : 1;
    if (argc == 4 && std::string(argv[1]) == "--physical-output") {
        const std::filesystem::path directory = argv[3];
        std::filesystem::create_directories(directory);
        return checkOutput(argv[2], directory / "layers-ascii.dxf", false)
            && checkOutput(argv[2], directory / "layers-binary.dxf", true) ? 0 : 1;
    }
    const bool keep = (argc == 3 || argc == 4) && std::string(argv[1]) == "--keep";
    if (argc != 2 && !keep) return 2;
    const std::string mode = argc == 4 ? argv[3] : "3";
    if (mode.size() != 1u || mode[0] < '0' || mode[0] > '3') return 2;
    const int layerShape = mode[0] - '0';
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path directory = argv[keep ? 2 : 1];
    std::filesystem::create_directories(directory);
    const auto scratch = directory / ("layer-binding-" + std::to_string(nonce));
    std::error_code error;
    if (!std::filesystem::create_directory(scratch, error)) return 1;
    const auto path = scratch / "generated.dwg";
    const auto materialPath = scratch / "material.dwg";
    const auto asciiPath = scratch / "layers-ascii.dxf";
    const auto binaryPath = scratch / "layers-binary.dxf";
    struct Cleanup { std::filesystem::path file,materialFile,ascii,binary,dir; bool keep; ~Cleanup() {
        if (keep) return;
        std::error_code e; std::filesystem::remove(file,e);
        std::filesystem::remove(materialFile,e); std::filesystem::remove(ascii,e);
        std::filesystem::remove(binary,e); std::filesystem::remove(dir,e);
    }} cleanup{path,materialPath,asciiPath,binaryPath,scratch,keep};
    dwgRW writer(path.string().c_str());
    BindingAuthor author(writer);
    if (!writer.write(&author, DRW::AC1024, false) || !author.added
        || !checkFile(path, false)) return 1;
    dwgRW materialWriter(materialPath.string().c_str());
    BindingAuthor materialAuthor(materialWriter, true, layerShape);
    if (!materialWriter.reserveHandle(0xA11u)) return 1;
    if (layerShape == 3) {
        const std::string sentinel = "keep canonical layer destination\n";
        for (int failure = 1; failure <= 5; ++failure) {
            { std::ofstream f(materialPath, std::ios::binary | std::ios::trunc); f << sentinel; }
            materialAuthor.incompatible = failure;
            const auto calls = materialAuthor.canonicalCalls;
            if (materialWriter.write(&materialAuthor, DRW::AC1024, false)
                || materialAuthor.canonicalCalls == calls || !materialAuthor.callerUnchanged) return 1;
            { std::ifstream f(materialPath, std::ios::binary);
              if (std::string(std::istreambuf_iterator<char>(f), {}) != sentinel) return 1; }
        }
        materialAuthor.incompatible = 0;
        { std::ofstream f(materialPath, std::ios::binary | std::ios::trunc); f << sentinel; }
        const auto calls = materialAuthor.canonicalCalls;
        if (materialWriter.write(&materialAuthor, DRW::AC1021, false)
            || materialAuthor.canonicalCalls == calls || !materialAuthor.callerUnchanged) return 1;
        { std::ifstream f(materialPath, std::ios::binary);
          if (std::string(std::istreambuf_iterator<char>(f), {}) != sentinel) return 1; }
        for (int failure : {1, 2, 3, 4, 5, 6, 7, 8, 11, 12}) {
            { std::ofstream f(materialPath, std::ios::binary | std::ios::trunc); f << sentinel; }
            materialAuthor.graphFailure = failure;
            const auto calls = materialAuthor.objectCalls;
            if (materialWriter.write(&materialAuthor, DRW::AC1024, false)
                || materialAuthor.objectCalls == calls) return 1;
            { std::ifstream f(materialPath, std::ios::binary);
              if (std::string(std::istreambuf_iterator<char>(f), {}) != sentinel) return 1; }
        }
        for (int retry = 9; retry <= 10; ++retry) {
            materialAuthor.graphFailure = retry;
            if (!materialWriter.write(&materialAuthor, DRW::AC1024, false)
                || !materialAuthor.objects_) return 1;
        }
        materialAuthor.graphFailure = 0;
    }
    const bool materialWritten = materialWriter.write(&materialAuthor, DRW::AC1024, false);
    if (!materialWritten || !materialAuthor.added || !materialAuthor.registered_ || !materialAuthor.objects_
        || (layerShape == 3 && !materialAuthor.callerUnchanged)) {
        std::cerr << "material author failed: write=" << materialWritten << " layers=" << materialAuthor.added
            << " class=" << materialAuthor.registered_ << " objects=" << materialAuthor.objects_
            << " diagnostic=" << materialWriter.getLastDiagnostic().code << '\n';
        return 1;
    }
    if (!checkFile(materialPath, true) || !checkOutput(materialPath, asciiPath, false)
        || !checkOutput(materialPath, binaryPath, true)) return 1;
    if (keep) std::cout << "AC1024_OUTPUT " << scratch.string() << '\n';
    return 0;
}
