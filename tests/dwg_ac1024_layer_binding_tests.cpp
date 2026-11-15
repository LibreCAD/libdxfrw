// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_iface.h"
#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string>

class DxIfaceStandardDwgCaptureTestAccess {
public:
    static bool complete(const dx_iface& iface, const dx_data& data) {
        return iface.hasCompleteAc1024LayerSourceEvidence(data);
    }
    static bool materialComplete(const dx_iface& iface, const dx_data& data) {
        return iface.hasCompleteAc1024DefaultMaterialSourceEvidence(data);
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

class BindingAuthor final : public LC3CompatCheck, public DRW_DwgDefaultMaterialProvider {
public:
    explicit BindingAuthor(dwgRW& output, bool material = false)
        : output_(output), material_(material) {}
    bool wantsDwgDefaultMaterialGraph() const override { return material_; }
    bool dwgDefaultMaterialOpaqueReferencesAbsent(std::uint32_t dictionary,
        std::uint32_t byBlock, std::uint32_t byLayer, std::uint32_t global) const override {
        return dictionary && byBlock && byLayer && global;
    }
    void writeLayers() override {
        DRW_Layer layer; layer.name = "Defpoints"; added = output_.addLayer(&layer);
    }
    bool added = false;
private:
    dwgRW& output_;
    bool material_;
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
int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--physical")
        return checkFile(argv[2], true) ? 0 : 1;
    if (argc != 2) return 2;
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path directory = argv[1];
    std::filesystem::create_directories(directory);
    const auto scratch = directory / ("layer-binding-" + std::to_string(nonce));
    std::error_code error;
    if (!std::filesystem::create_directory(scratch, error)) return 1;
    const auto path = scratch / "generated.dwg";
    const auto materialPath = scratch / "material.dwg";
    struct Cleanup { std::filesystem::path file,materialFile,dir; ~Cleanup() {
        std::error_code e; std::filesystem::remove(file,e);
        std::filesystem::remove(materialFile,e); std::filesystem::remove(dir,e);
    }} cleanup{path,materialPath,scratch};
    dwgRW writer(path.string().c_str());
    BindingAuthor author(writer);
    if (!writer.write(&author, DRW::AC1024, false) || !author.added
        || !checkFile(path, false)) return 1;
    dwgRW materialWriter(materialPath.string().c_str());
    BindingAuthor materialAuthor(materialWriter, true);
    if (!materialWriter.write(&materialAuthor, DRW::AC1024, false)
        || !materialAuthor.added || !checkFile(materialPath, true)) return 1;
    return 0;
}
