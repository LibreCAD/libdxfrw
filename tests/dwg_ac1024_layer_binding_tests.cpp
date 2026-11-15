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

class BindingAuthor final : public LC3CompatCheck {
public:
    explicit BindingAuthor(dwgRW& output) : output_(output) {}
    void writeLayers() override {
        DRW_Layer layer; layer.name = "Defpoints"; added = output_.addLayer(&layer);
    }
    bool added = false;
private:
    dwgRW& output_;
};
static bool checkFile(const std::filesystem::path& path) {
    dx_data data;
    dx_iface reader;
    if (!reader.fileImport(path.string(), &data, false)
        || !DxIfaceStandardDwgCaptureTestAccess::checkMutations(reader, data)) {
        std::cerr << "AC1024 layer source/publication binding failed\n";
        return false;
    }
    return true;
}
int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--physical")
        return checkFile(argv[2]) ? 0 : 1;
    if (argc != 2) return 2;
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path directory = argv[1];
    std::filesystem::create_directories(directory);
    const auto scratch = directory / ("layer-binding-" + std::to_string(nonce));
    std::error_code error;
    if (!std::filesystem::create_directory(scratch, error)) return 1;
    const auto path = scratch / "generated.dwg";
    struct Cleanup { std::filesystem::path file,dir; ~Cleanup() {
        std::error_code e; std::filesystem::remove(file,e); std::filesystem::remove(dir,e);
    }} cleanup{path,scratch};
    dwgRW writer(path.string().c_str());
    BindingAuthor author(writer);
    if (!writer.write(&author, DRW::AC1024, false) || !author.added
        || !checkFile(path)) return 1;
    return 0;
}
