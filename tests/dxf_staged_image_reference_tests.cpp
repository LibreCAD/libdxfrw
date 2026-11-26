// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "intern/dxf_raster_output.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
using References = dxfRW::DxfImageOutputReferences;
struct Exporter : LC3CompatCheck {
    Exporter(dxfRW& output, DRW::Version version) : output(output), version(version) {}
    dxfRW& output;
    DRW::Version version;
    std::vector<References> references;
    bool ok = false;
    void writeEntities() override {
        references.clear();
        ok = true;
        for (unsigned i = 0; i != 2; ++i) {
            DRW_Image image;
            image.sizeu = 16;
            image.sizev = 12;
            image.secPoint = {1, 0, 0};
            image.vVector = {0, 1, 0};
            image.m_clipBoundaryType = 1;
            image.clipPath = {{-0.5, -0.5, 0}, {15.5, 11.5, 0}};
            auto* definition = output.writeImage(&image, i ? "second.png" : "first.png");
            if (!definition || definition->reactors.empty()) { ok = false; return; }
            const auto& pair = *definition->reactors.begin();
            references.push_back({static_cast<std::uint32_t>(std::stoul(pair.second, nullptr, 16)),
                                  version == DRW::AC1014 ? 0u : DRW::DxfModelSpaceBlockRecordHandle, definition->handle,
                                  static_cast<std::uint32_t>(std::stoul(pair.first, nullptr, 16))});
        }
    }
    bool finalizeDxfWrite() override { return ok; }
};
struct Reader : LC3CompatCheck {
    std::vector<DRW_Image> images;
    void addImage(const DRW_Image* image) override { if (image) images.push_back(*image); }
};
std::string read(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
}
bool run(const std::filesystem::path& path, DRW::Version version, bool binary,
         dxfRW::DxfCompatibilityProfile profile) {
    dxfRW writer(path.string().c_str());
    writer.setDxfCompatibilityProfile(profile);
    Exporter adapter(writer, version);
    unsigned mode = 0;
    if (!writer.setDxfEntitySectionsWriteInspector([&](const auto& view, std::string_view section) {
        auto expected = section == "ENTITIES" ? adapter.references : std::vector<References>{};
        if (section == "ENTITIES" && mode == 1) ++expected.front().owner;
        if (section == "ENTITIES" && mode == 2) ++expected.front().definition;
        if (section == "ENTITIES" && mode == 3) ++expected.front().reactor;
        if (section == "ENTITIES" && mode == 4) expected.pop_back();
        if (section == "ENTITIES" && mode == 5) expected.push_back(expected.front());
        if (section == "ENTITIES" && mode == 7) {
            auto extra = expected.front(); extra.image = 0xFEEDu;
            expected.push_back(extra);
        }
        if (section == "ENTITIES" && (mode == 6 || mode == 8)) {
            std::string changed(view.bytes);
            const auto profile = view.compatibilityProfile == dxfRW::DxfCompatibilityProfile::StandaloneSafe
                ? dx_raster_output::Profile::StandaloneSafe : dx_raster_output::Profile::LibreCadMasterLegacy;
            dx_raster_output::Cursor cursor(changed, binary, profile);
            dx_raster_output::Token token;
            bool mutated = false;
            std::string firstHandle;
            while (!cursor.done()) {
                if (!cursor.next(token)) return false;
                if (token.text.empty()) continue;
                if (mode == 8 && token.code == 5 && firstHandle.empty()) {
                    firstHandle = token.text;
                    continue;
                }
                if ((mode == 6 && token.code != 340) ||
                    (mode == 8 && (token.code != 5 || token.text.size() != firstHandle.size()))) continue;
                const auto offset = static_cast<std::size_t>(token.text.data() - changed.data());
                if (mode == 6) changed[offset] = changed[offset] == 'A' ? 'B' : 'A';
                else changed.replace(offset, token.text.size(), firstHandle);
                mutated = true;
                break;
            }
            if (!mutated) return false;
            auto altered = view;
            altered.bytes = changed;
            return writer.inspectDxfImageOutputReferences(altered, expected);
        }
        const bool valid = writer.inspectDxfImageOutputReferences(view, expected);
        if (!valid && mode == 0) {
            std::cerr << "inspection failed in " << section << " count=" << expected.size() << '\n';
        }
        return valid;
    })) return false;
    if (!writer.setDxfObjectsWriteInspector([&](const auto& view) {
        const auto dictionary = writer.dxfWriteImageDictionaryHandle();
        std::vector<dxfRW::DxfImageDefinitionOutputLinks> definitions;
        std::vector<dxfRW::DxfImageReactorOutputLinks> reactors;
        for (const auto& reference : adapter.references) {
            definitions.push_back({reference.definition, version == DRW::AC1014 ? 0u : dictionary,
                                   {reference.reactor}});
            reactors.push_back({reference.reactor, reference.image});
        }
        if (mode == 9) ++definitions.front().owner;
        if (mode == 10) ++reactors.front().image;
        if (mode == 11) definitions.pop_back();
        if (mode == 12) definitions.front().reactors.clear();
        if (mode == 13) reactors.push_back(reactors.front());
        if (mode == 14 || mode == 15) {
            std::string changed(view.bytes);
            const auto profile = view.compatibilityProfile == dxfRW::DxfCompatibilityProfile::StandaloneSafe
                ? dx_raster_output::Profile::StandaloneSafe : dx_raster_output::Profile::LibreCadMasterLegacy;
            dx_raster_output::Cursor cursor(changed, binary, profile);
            dx_raster_output::Token token;
            std::string name;
            bool inReactors = false, mutated = false;
            while (!cursor.done()) {
                const auto start = cursor.position();
                if (!cursor.next(token)) return false;
                if (token.code == 0) { name = token.text; inReactors = false; }
                else if (token.code == 102 && name == "IMAGEDEF") {
                    if (token.text == "{ACAD_REACTORS") inReactors = true;
                    else if (token.text == "}") inReactors = false;
                } else if (mode == 14 && name == "IMAGEDEF_REACTOR" && token.code == 90) {
                    if (binary) changed[start + 2] = 3;
                    else {
                        auto pos = changed.find('\n', start);
                        if (pos == std::string::npos) return false;
                        ++pos;
                        while (pos < changed.size() && changed[pos] == ' ') ++pos;
                        if (pos == changed.size()) return false;
                        changed[pos] = '3';
                    }
                    mutated = true;
                    break;
                } else if (mode == 15 && name == "IMAGEDEF" && inReactors && token.code == 330 && !token.text.empty()) {
                    const auto offset = static_cast<std::size_t>(token.text.data() - changed.data());
                    changed[offset] = changed[offset] == 'A' ? 'B' : 'A';
                    mutated = true;
                    break;
                }
            }
            if (!mutated) return false;
            auto altered = view;
            altered.bytes = changed;
            return writer.inspectDxfImageOutputObjects(altered, definitions, reactors);
        }
        const bool valid = writer.inspectDxfImageOutputObjects(view, definitions, reactors);
        if (!valid && mode == 0) std::cerr << "IMAGE object inspection failed\n";
        return valid;
    })) return false;
    if (!writer.write(&adapter, version, binary) || !adapter.ok || adapter.references.size() != 2u) {
        std::cerr << "baseline write failed, ok=" << adapter.ok << " references=" << adapter.references.size() << '\n';
        return false;
    }
    for (mode = 1; mode <= 15; ++mode) {
        { std::ofstream destination(path, std::ios::binary | std::ios::trunc); destination << "sentinel"; }
        if (writer.write(&adapter, version, binary) || read(path) != "sentinel") {
            std::cerr << "refusal failed, mode=" << mode << '\n'; return false;
        }
    }
    mode = 0;
    if (!writer.write(&adapter, version, binary) || !adapter.ok) { std::cerr << "retry failed\n"; return false; }
    Reader reader;
    dxfRW input(path.string().c_str());
    input.setDxfCompatibilityProfile(profile);
    if (!input.read(&reader, false) || reader.images.size() != 2u) { std::cerr << "readback failed: " << reader.images.size() << '\n'; return false; }
    for (std::size_t i = 0; i < reader.images.size(); ++i)
        if (reader.images[i].handle != adapter.references[i].image ||
            reader.images[i].ref != adapter.references[i].definition ||
            reader.images[i].m_imageDefReactorHandle != adapter.references[i].reactor) return false;
    return true;
}
}
int main() {
    const auto directory = std::filesystem::temp_directory_path() /
        ("staged-image-refs-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code error; std::filesystem::remove_all(path, error); } } cleanup{directory};
    for (const auto version : {DRW::AC1014, DRW::AC1015, DRW::AC1018, DRW::AC1021,
                               DRW::AC1024, DRW::AC1027, DRW::AC1032})
        for (const bool binary : {false, true})
          for (const auto profile : {dxfRW::DxfCompatibilityProfile::StandaloneSafe,
                                     dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy})
            if (!run(directory / "output.dxf", version, binary, profile)) {
                std::cerr << "staged IMAGE references failed for version " << static_cast<int>(version)
                          << " binary=" << binary << " profile=" << static_cast<int>(profile) << '\n';
                return 1;
            }
}
