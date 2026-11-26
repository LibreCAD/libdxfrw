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
    std::vector<DRW_Image> imageFields;
    std::vector<DRW_ImageDef> definitionFields;
    std::vector<DRW_RawDxfObject> rawPlot;
    bool ok = false;
    void writeEntities() override {
        references.clear();
        imageFields.clear();
        definitionFields.clear();
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
            definitionFields.push_back(*definition);
            imageFields.push_back(image);
            references.push_back({static_cast<std::uint32_t>(std::stoul(pair.second, nullptr, 16)),
                                  version == DRW::AC1014 ? 0u : DRW::DxfModelSpaceBlockRecordHandle, definition->handle,
                                  static_cast<std::uint32_t>(std::stoul(pair.first, nullptr, 16))});
        }
    }
    void writeObjects() override {
        for (auto& object : rawPlot) ok = output.writeRawDxfObject(&object) && ok;
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
    if (version >= DRW::AC1015) {
        DRW_RawDxfObject dictionary, placeholder;
        dictionary.name = "ACDBDICTIONARYWDFLT"; dictionary.handle = 0xA0u;
        dictionary.parentHandle = 0xCu; dictionary.m_version = version;
        dictionary.groups.emplace_back(5, std::string("A0"));
        dictionary.groups.emplace_back(330, std::string("C"));
        dictionary.groups.emplace_back(100, std::string("AcDbDictionary"));
        dictionary.groups.emplace_back(281, 1);
        dictionary.groups.emplace_back(3, std::string("Normal"));
        dictionary.groups.emplace_back(350, std::string("A1"));
        dictionary.groups.emplace_back(100, std::string("AcDbDictionaryWithDefault"));
        dictionary.groups.emplace_back(340, std::string("A1"));
        placeholder.name = "ACDBPLACEHOLDER"; placeholder.handle = 0xA1u;
        placeholder.parentHandle = 0xA0u; placeholder.m_version = version;
        placeholder.groups.emplace_back(5, std::string("A1"));
        placeholder.groups.emplace_back(330, std::string("A0"));
        adapter.rawPlot = {dictionary, placeholder};
        writer.setRootDictEntries({{"ACAD_PLOTSTYLENAME", "A0"}});
    }
    unsigned mode = 0;
    if (!writer.setDxfEntitySectionsWriteInspector([&](const auto& view, std::string_view section) {
        auto expected = section == "ENTITIES" ? adapter.references : std::vector<References>{};
        if (section == "ENTITIES")
            for (std::size_t i = 0; i < expected.size(); ++i) {
                expected[i].scalarFields = &adapter.imageFields[i];
                expected[i].canonicalCommon = version >= DRW::AC1015;
            }
        auto changedImage = section == "ENTITIES" ? adapter.imageFields.front() : DRW_Image{};
        if (section == "ENTITIES" && mode >= 26 && mode <= 34) {
            if (mode == 26) ++changedImage.m_classVersion;
            if (mode == 27) ++changedImage.basePoint.z;
            if (mode == 28) ++changedImage.secPoint.x;
            if (mode == 29) ++changedImage.vVector.y;
            if (mode == 30) ++changedImage.sizeu;
            if (mode == 31) ++changedImage.brightness;
            if (mode == 32) ++changedImage.clipPath.front().x;
            if (mode == 33) changedImage.clipMode = !changedImage.clipMode;
            if (mode == 34) changedImage.clipPath.pop_back();
            expected.front().scalarFields = &changedImage;
        }
        if (section == "ENTITIES" && mode == 1) ++expected.front().owner;
        if (section == "ENTITIES" && mode == 2) ++expected.front().definition;
        if (section == "ENTITIES" && mode == 3) ++expected.front().reactor;
        if (section == "ENTITIES" && mode == 4) expected.pop_back();
        if (section == "ENTITIES" && mode == 5) expected.push_back(expected.front());
        if (section == "ENTITIES" && mode == 7) {
            auto extra = expected.front(); extra.image = 0xFEEDu;
            expected.push_back(extra);
        }
        if (section == "ENTITIES" && (mode == 6 || mode == 8 || (mode >= 46 && mode <= 49))) {
            std::string changed(view.bytes);
            const auto profile = view.compatibilityProfile == dxfRW::DxfCompatibilityProfile::StandaloneSafe
                ? dx_raster_output::Profile::StandaloneSafe : dx_raster_output::Profile::LibreCadMasterLegacy;
            dx_raster_output::Cursor cursor(changed, binary, profile);
            dx_raster_output::Token token;
            bool mutated = false;
            std::string firstHandle;
            while (!cursor.done()) {
                if (!cursor.next(token)) return false;
                if (mode == 8 && token.code == 5 && firstHandle.empty()) {
                    firstHandle = token.text;
                    continue;
                }
                if (mode >= 46) {
                    const int target = mode == 46 ? 8 : mode == 47 ? 6 : mode == 48 ? 62 : 370;
                    if (token.code != target) continue;
                    if (token.type == dx_raster_output::Kind::Text && !token.text.empty()) {
                        const auto offset = static_cast<std::size_t>(token.text.data() - changed.data());
                        changed[offset] = 'X';
                    } else if (binary) {
                        const auto offset = cursor.position() - 2u;
                        changed[offset] ^= 1;
                    } else {
                        const auto end = cursor.position();
                        auto pos = changed.rfind('\n', end - 2u);
                        if (pos == std::string::npos) return false;
                        while (++pos < end && (changed[pos] == ' ' || changed[pos] == '-')) {}
                        if (pos >= end) return false;
                        changed[pos] = changed[pos] == '1' ? '2' : '1';
                    }
                    mutated = true;
                    break;
                }
                if (token.text.empty()) continue;
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
        for (std::size_t i = 0; i < adapter.references.size(); ++i) {
            const auto& reference = adapter.references[i];
            definitions.push_back({reference.definition, version == DRW::AC1014 ? 0u : dictionary,
                                   {reference.reactor}, &adapter.definitionFields[i]});
            reactors.push_back({reference.reactor, reference.image});
        }
        auto changedFields = adapter.definitionFields.front();
        if (mode == 16) { changedFields.u += 1; definitions.front().scalarFields = &changedFields; }
        if (mode == 18) { changedFields.name = "unsafe\\path"; definitions.front().scalarFields = &changedFields; }
        if (mode >= 19 && mode <= 24) {
            if (mode == 19) ++changedFields.imgVersion;
            if (mode == 20) ++changedFields.v;
            if (mode == 21) ++changedFields.up;
            if (mode == 22) ++changedFields.vp;
            if (mode == 23) changedFields.loaded = 1 - changedFields.loaded;
            if (mode == 24) ++changedFields.resolution;
            definitions.front().scalarFields = &changedFields;
        }
        if (mode == 9) ++definitions.front().owner;
        if (mode == 10) ++reactors.front().image;
        if (mode == 11) definitions.pop_back();
        if (mode == 12) definitions.front().reactors.clear();
        if (mode == 13) reactors.push_back(reactors.front());
        if (mode == 14 || mode == 15 || mode == 17 || mode == 25) {
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
                } else if (mode == 17 && name == "IMAGEDEF" && token.code == 1 && !token.text.empty()) {
                    const auto offset = static_cast<std::size_t>(token.text.data() - changed.data());
                    changed[offset] = changed[offset] == 'x' ? 'y' : 'x';
                    mutated = true;
                    break;
                } else if (mode == 25 && name == "IMAGEDEF" && token.code == 20) {
                    if (binary) changed[start] = 10;
                    else {
                        const auto end = changed.find('\n', start);
                        const auto digit = changed.find('2', start);
                        if (end == std::string::npos || digit == std::string::npos || digit >= end) return false;
                        changed[digit] = '1';
                    }
                    mutated = true;
                    break;
                }
            }
            if (!mutated) return false;
            auto altered = view;
            altered.bytes = changed;
            return writer.inspectDxfImageOutputObjects(altered, definitions, reactors);
        }
        bool valid = writer.inspectDxfImageOutputObjects(view, definitions, reactors);
        if (valid && version >= DRW::AC1015) {
            std::vector<dxfRW::DxfDictionaryOutputExpectation> root{{"ACAD_GROUP", 0xDu, 350}};
            const auto plot = writer.dxfWriteDefaultPlotStyleDictionaryHandle();
            if (plot) root.push_back({"ACAD_PLOTSTYLENAME", plot, 350});
            root.push_back({"ACAD_IMAGE_DICT", dictionary, 350});
            if (!adapter.rawPlot.empty()) root.push_back({"ACAD_PLOTSTYLENAME", 0xA0u, 350});
            std::vector<dxfRW::DxfDictionaryOutputExpectation> images{
                {"first", adapter.references[0].definition, 350},
                {"second", adapter.references[1].definition, 350}};
            if (mode == 35) ++root.back().target;
            if (mode == 36) ++images.front().target;
            if (mode == 37) images.pop_back();
            if (mode == 38) images.front().role = 360;
            if (mode == 41) images.front().name.assign(1025u, 'x');
            if (mode == 42) images.back().name = "FIRST";
            const int cloning = mode == 39 ? 2 : 1;
            if (mode == 40) {
                std::string changed(view.bytes);
                const auto position = changed.find("first");
                if (position == std::string::npos) return false;
                changed[position] = 'x';
                auto altered = view;
                altered.bytes = changed;
                return writer.inspectDxfDictionaryOutput(altered, dictionary, 0xCu, 0, cloning, images);
            }
            valid = writer.inspectDxfDictionaryOutput(view, 0xCu, 0u, 0, 1, root) &&
                    writer.inspectDxfDictionaryOutput(view, dictionary, 0xCu, 0, cloning, images);
            if (valid && !adapter.rawPlot.empty()) {
                auto changedPlot = adapter.rawPlot;
                if (mode == 43) changedPlot.front().groups[3] = DRW_Variant(281, 2);
                std::vector<const DRW_RawDxfObject*> expectedPlot{&changedPlot[0], &changedPlot[1]};
                if (mode == 44 || mode == 45) {
                    std::string changed(view.bytes);
                    const auto position = changed.find(mode == 44 ? "Normal" : "ACDBDICTIONARYWDFLT");
                    if (position == std::string::npos) return false;
                    changed[position] = 'X';
                    auto altered = view;
                    altered.bytes = changed;
                    return writer.inspectDxfRawObjectRecords(altered, expectedPlot);
                }
                valid = writer.inspectDxfRawObjectRecords(view, expectedPlot);
            }
        }
        if (!valid && mode == 0) std::cerr << "IMAGE object inspection failed\n";
        return valid;
    })) return false;
    if (!writer.write(&adapter, version, binary) || !adapter.ok || adapter.references.size() != 2u) {
        std::cerr << "baseline write failed, ok=" << adapter.ok << " references=" << adapter.references.size() << '\n';
        return false;
    }
    for (mode = 1; mode <= 49; ++mode) {
        if (mode == 33 && version < DRW::AC1024) continue;
        if (mode >= 35 && version < DRW::AC1015) continue;
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
