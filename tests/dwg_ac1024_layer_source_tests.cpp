// SPDX-License-Identifier: GPL-2.0-or-later
// From-scratch DWG controls for the optional AC1024 layer source callback.
#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include "intern/dwgbuffer.h"
#include "intern/drw_textcodec.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <vector>

class LayerReceiptAuthor final : public LC3CompatCheck {
public:
    explicit LayerReceiptAuthor(dwgRW& output) : output_(output) {}
    void writeLayers() override {
        DRW_Layer layer;
        layer.name = "Defpoints";
        added = output_.addLayer(&layer);
    }
    bool added = false;
private:
    dwgRW& output_;
};
class LayerReceiptObserver final : public LC3CompatCheck,
                                   public DRW_DwgAc1024LayerSourceHandler {
public:
    void addLayer(const DRW_Layer& layer) override { layers.push_back(layer); }
    void addDwgAc1024LayerSource(const DRW_DwgAc1024LayerSource& source) noexcept override {
        try { sources.push_back(source); } catch (...) { failed = true; }
    }
    void clear() { layers.clear(); sources.clear(); failed = false; }
    std::vector<DRW_Layer> layers;
    std::vector<DRW_DwgAc1024LayerSource> sources;
    bool failed = false;
};
class LayerReceiptDecoder final : public DRW_Layer {
public:
    using DRW_Layer::parseDwg;
};
static bool generate(const std::filesystem::path& path, DRW::Version version) {
    dwgRW output(path.string().c_str());
    LayerReceiptAuthor author(output);
    return output.write(&author, version, false) && author.added;
}
static bool read(const std::filesystem::path& path, LayerReceiptObserver& observer,
                 DRW::Version version) {
    observer.clear();
    dwgRW input(path.string().c_str());
    if (!input.read(&observer, false) || observer.failed || observer.layers.size() != 2u)
        return false;
    if (version != DRW::AC1024) return observer.sources.empty();
    if (observer.sources.size() != 2u) return false;
    for (const auto& layer : observer.layers) {
        const auto count = std::count_if(observer.sources.begin(), observer.sources.end(),
            [&layer](const auto& source) { return source.m_raw.m_handle == layer.handle; });
        const auto found = std::find_if(observer.sources.begin(), observer.sources.end(),
            [&layer](const auto& source) { return source.m_raw.m_handle == layer.handle; });
        if (count != 1 || found == observer.sources.end()
            || found->m_raw.m_version != DRW::AC1024 || found->m_raw.m_objectType != 51
            || found->m_raw.m_parentHandle != layer.parentHandle
            || found->m_raw.m_objectOffset != found->m_sourceOffset
            || found->m_raw.m_objectSize != found->m_raw.m_rawBytes.size()
            || found->m_raw.m_rawBytes.empty() || found->m_raw.m_rawBytes.size() > 1024u
            || found->m_sourceOffsetSpace == DRW_DwgFrameOffsetSpace::Unknown)
            return false;
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1024, false);
        dwgBuffer body(found->m_raw.m_rawBytes.data(), found->m_raw.m_rawBytes.size(), &codec);
        LayerReceiptDecoder decoded;
        if (!decoded.parseDwg(DRW::AC1024, &body, found->m_raw.m_bodyBitSize)
            || decoded.handle != layer.handle || decoded.name != layer.name
            || decoded.parentHandle != layer.parentHandle
            || decoded.plotStyleHandle.ref != layer.plotStyleHandle.ref
            || decoded.materialHandle.ref != layer.materialHandle.ref)
            return false;
        auto truncated = found->m_raw.m_rawBytes;
        truncated.resize(2u);
        dwgBuffer shortBody(truncated.data(), truncated.size(), &codec);
        if (decoded.parseDwg(DRW::AC1024, &shortBody, found->m_raw.m_bodyBitSize))
            return false;
        dwgBuffer retryBody(found->m_raw.m_rawBytes.data(),
                            found->m_raw.m_rawBytes.size(), &codec);
        if (!decoded.parseDwg(DRW::AC1024, &retryBody, found->m_raw.m_bodyBitSize)
            || decoded.handle != layer.handle || decoded.name != layer.name)
            return false;
    }
    return true;
}
int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--physical") {
        LayerReceiptObserver observer;
        return read(argv[2], observer, DRW::AC1024) ? 0 : 1;
    }
    if (argc != 2) return 2;
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::filesystem::path directory=argv[1];
    std::filesystem::create_directories(directory);
    const auto scratch = directory / ("layer-receipt-" + std::to_string(nonce));
    std::error_code createError;
    if (!std::filesystem::create_directory(scratch, createError)) return 1;
    const auto modern = scratch / "source-1024.dwg";
    const auto legacy = scratch / "source-1021.dwg";
    struct Cleanup { std::filesystem::path a,b,dir; ~Cleanup() {
        std::error_code error; std::filesystem::remove(a,error); std::filesystem::remove(b,error);
        std::filesystem::remove(dir,error);
    }} cleanup{modern,legacy,scratch};
    if (!generate(modern, DRW::AC1024) || !generate(legacy, DRW::AC1021)) return 1;
    LayerReceiptObserver observer;
    if (!read(modern, observer, DRW::AC1024)
        || !read(legacy, observer, DRW::AC1021)
        || !read(modern, observer, DRW::AC1024)) {
        std::cerr << "AC1024 layer source receipt or version/reset control failed\n";
        return 1;
    }
    LC3CompatCheck originalConsumer;
    dwgRW compatible(modern.string().c_str());
    if (!compatible.read(&originalConsumer, false)) return 1;
    return 0;
}
