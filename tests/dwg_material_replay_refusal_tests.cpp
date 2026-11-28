// SPDX-License-Identifier: GPL-2.0-or-later
// MATERIAL raw bodies do not retain their detached DWG filename strings.
#include "intern/dwgbuffer.h"
#include "material_dwg_test_helpers.h"
#include "lc3_compat_check.cpp"
#include "libdwgr.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace {
class MaterialProbe : public DRW_Material {
public:
    using DRW_Material::encodeDwg;
    using DRW_Material::parseDwg;
};

bool refusalIsAtomic(const MaterialProbe& material, DRW::Version version) {
    dwgBufferW body, strings, handles;
    body.putRawChar8(0xA5u);
    strings.putRawChar8(0x5Au);
    handles.putRawChar8(0xC3u);
    const auto b = body.data();
    const auto s = strings.data();
    const auto h = handles.data();
    return !material.encodeDwg(version, &body, &strings, &handles)
        && body.data() == b && strings.data() == s && handles.data() == h
        && body.bitCount() == 8u && strings.bitCount() == 8u
        && handles.bitCount() == 8u;
}

bool codecControls(MaterialProbe& source) {
    material_dwg_test::Frame frame;
    if (!material_dwg_test::makeFrame({}, frame)) return false;
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1024, false);
    dwgBuffer input(frame.bytes.data(), frame.bytes.size(), &codec);
    if (!source.parseDwg(DRW::AC1024, &input, frame.handleBits)
        || !source.hasDwgRawData() || !source.hasCompleteDwgSemanticFields()
        || source.m_diffuseMapFileName != "diffuse-map.png"
        || source.m_specularMapFileName != "specular-map.png"
        || source.m_reflectionMapFileName != "reflection-map.png"
        || source.m_opacityMapFileName != "opacity-map.png"
        || source.m_bumpMapFileName != "bump-map.png"
        || source.m_refractionMapFileName != "refraction-map.png"
        || !refusalIsAtomic(source, DRW::AC1024)) return false;

    // Even decoded empty names do not certify original slots or raw source
    // selectors; callers can mutate these public fields after parsing.
    MaterialProbe edited = source;
    edited.m_diffuseMapFileName.clear();
    edited.m_specularMapFileName.clear();
    edited.m_reflectionMapFileName.clear();
    edited.m_opacityMapFileName.clear();
    edited.m_bumpMapFileName.clear();
    edited.m_refractionMapFileName.clear();
    if (!edited.hasCompleteDwgSemanticFields()
        || !refusalIsAtomic(edited, DRW::AC1024)) return false;

    for (auto version : {DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        MaterialProbe opaque;
        opaque.m_name = "unproven-opaque";
        if (!opaque.setDwgRawData({}, 0u, version)
            || !refusalIsAtomic(opaque, version)) return false;
    }
    // Manually authored legacy MATERIAL stays available; modern versions
    // require an exact retained source profile, including detached strings.
    MaterialProbe manual;
    manual.m_name = "manual";
    for (auto version : {DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032})
        if (!refusalIsAtomic(manual, version)) return false;
    dwgBufferW body, strings, handles;
    return manual.encodeDwg(DRW::AC1018, &body, &strings, &handles)
        && body.bitCount() > 0;
}

struct Adapter : LC3CompatCheck {
    Adapter(dwgRW& output, const DRW_Material& source) : writer(output), material(source) {}
    void writeDwgClasses() override {
        registered = writer.registerMaterialObjectClass(&material);
    }
    void writeObjects() override {
        // Deliberately ignore the return when choosing the whole-file result.
        accepted = writer.writeMaterial(&material);
    }
    dwgRW& writer;
    DRW_Material material;
    bool registered = false;
    bool accepted = true;
};

struct Capture : LC3CompatCheck {
    void addMaterial(const DRW_Material& material) override {
        if (material.m_name == "manual-after-refusal") ++matches;
    }
    unsigned matches = 0;
};

bool wholeFileControls(const MaterialProbe& parsed) {
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-material-refusal-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dwg");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{path};
    const auto filename = path.string();
    dwgRW output(filename.c_str());
    Adapter adapter(output, parsed);
    for (auto target : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                        DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        { std::ofstream file(path, std::ios::binary); file << "preserve-sentinel"; }
        adapter.registered = false;
        adapter.accepted = true;
        if (output.write(&adapter, target, true)
            || !adapter.registered || adapter.accepted) return false;
        std::ifstream input(path, std::ios::binary);
        if (std::string(std::istreambuf_iterator<char>(input), {}) != "preserve-sentinel")
            return false;
    }
    // Newly authored modern bodies are also outside the exact replay profiles.
    // Refusal remains fatal to the whole file even if the caller ignores it.
    adapter.material.reset();
    adapter.material.handle = 0x97u;
    adapter.material.m_name = "manual-after-refusal";
    for (auto target : {DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        { std::ofstream file(path, std::ios::binary); file << "preserve-sentinel"; }
        adapter.registered = false;
        adapter.accepted = true;
        if (output.write(&adapter, target, true)
            || !adapter.registered || adapter.accepted) return false;
        std::ifstream input(path, std::ios::binary);
        if (std::string(std::istreambuf_iterator<char>(input), {}) != "preserve-sentinel")
            return false;
    }
    // Retry the same authored object on a supported legacy profile.
    adapter.registered = false;
    adapter.accepted = false;
    if (!output.write(&adapter, DRW::AC1018, true)
        || !adapter.registered || !adapter.accepted) return false;
    dwgRW reader(filename.c_str());
    Capture capture;
    return reader.read(&capture, false) && capture.matches == 1u;
}
} // namespace

int main() {
    MaterialProbe parsed;
    if (!codecControls(parsed)) {
        std::fprintf(stderr, "MATERIAL codec refusal control failed\n");
        return 1;
    }
    if (!wholeFileControls(parsed)) {
        std::fprintf(stderr, "MATERIAL whole-file refusal/retry control failed\n");
        return 1;
    }
    std::puts("MATERIAL detached-string refusal and retry: PASS");
    return 0;
}
