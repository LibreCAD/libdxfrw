// SPDX-License-Identifier: GPL-2.0-or-later
// Generated DXF default plot-style graph, output identity and retry controls.
#include "lc3_compat_check.cpp"
#include "drw_objects.h"
#include "drw_classes.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
class Capture final : public LC3CompatCheck {
public:
    std::vector<DRW_Dictionary> roots;
    std::vector<DRW_DictionaryWithDefault> dictionaries;
    std::vector<DRW_RawDxfObject> objects;
    std::vector<DRW_Layer> layers;
    std::vector<DRW_Class> classes;
    void addDxfClass(const DRW_Class& value) override { classes.push_back(value); }
    void addNamedObjectDictionary(const DRW_Dictionary& value) override { roots.push_back(value); }
    void addDictionaryWithDefault(const DRW_DictionaryWithDefault& value) override { dictionaries.push_back(value); }
    void addRawDxfObject(const DRW_RawDxfObject& value) override { objects.push_back(value); }
    void addLayer(const DRW_Layer& value) override { layers.push_back(value); }
};

class AdversarialExport final : public LC3CompatCheck {
public:
    enum class Mode { Clean, LateRoot, LatePlaceholder };
    explicit AdversarialExport(dxfRW& output) : output(output) {}
    dxfRW& output;
    Mode mode = Mode::Clean;
    void writeObjects() override {
        if (mode == Mode::LateRoot) {
            output.setRootDictEntries({{"aCaD_PlotStyleName", "99"}});
        } else if (mode == Mode::LatePlaceholder) {
            DRW_AcDbPlaceholder placeholder;
            placeholder.handle = 0xFu;
            placeholder.parentHandle = 0xEu;
            (void)output.writeAcDbPlaceholder(&placeholder);
        }
    }
};

class CallerGraphExport final : public LC3CompatCheck {
public:
    enum class Mode { Valid, MissingDictionary, MissingPlaceholder, WrongDefault,
                      WrongNormal, WrongNormalTarget, WrongNormalRole,
                      WrongOwner, LateRoot, Alias };
    CallerGraphExport(dxfRW& output, const DRW_DictionaryWithDefault& source)
        : output(output), dictionary(source) {
        layer.name = "Caller default";
        explicitLayer.name = "Caller explicit";
        explicitLayer.handlePlotS = "A1";
        placeholder.handle = 0xA1u;
        placeholder.parentHandle = 0xA0u;
    }
    dxfRW& output;
    DRW_DictionaryWithDefault dictionary;
    DRW_AcDbPlaceholder placeholder;
    DRW_Layer layer, explicitLayer;
    Mode mode = Mode::Valid;
    void writeLayers() override {
        if (mode == Mode::Alias) explicitLayer.handlePlotS = "F";
        (void)output.writeLayer(&layer);
        (void)output.writeLayer(&explicitLayer);
    }
    void writeObjects() override {
        if (mode == Mode::LateRoot)
            output.setRootDictEntries({{"ACAD_PLOTSTYLENAME", "EF"}});
        DRW_DictionaryWithDefault dict(dictionary);
        DRW_AcDbPlaceholder child(placeholder);
        if (mode == Mode::WrongDefault) dict.m_defaultEntryHandle = 0xA2u;
        if (mode == Mode::WrongNormal) dict.m_entries.front().m_name = "Other";
        if (mode == Mode::WrongNormalTarget) dict.m_entries.front().m_handle = 0xA2u;
        if (mode == Mode::WrongNormalRole)
            dict.m_entries.front().m_dxfHandleCode = static_cast<DRW_Dictionary::Entry::DxfHandleCode>(360);
        if (mode == Mode::WrongOwner) child.parentHandle = 0xC;
        if (mode != Mode::MissingDictionary) (void)output.writeDictionaryWithDefault(&dict);
        if (mode != Mode::MissingPlaceholder) (void)output.writeAcDbPlaceholder(&child);
    }
};

class ExplicitDefaultExport final : public LC3CompatCheck {
public:
    explicit ExplicitDefaultExport(dxfRW& output) : output(output) {
        layer.name = "Explicit default plot style";
        layer.handlePlotS = "F";
    }
    dxfRW& output;
    DRW_Layer layer;
    void writeLayers() override { (void)output.writeLayer(&layer); }
};

class AliasExport final : public LC3CompatCheck {
public:
    explicit AliasExport(dxfRW& output) : output(output) {
        layer.name = "Source plot-style alias";
        layer.handlePlotS = "123";
    }
    dxfRW& output;
    DRW_Layer layer;
    void writeLayers() override { (void)output.writeLayer(&layer); }
};

std::string bytes(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

bool checkGraph(const std::filesystem::path& path, std::uint32_t forbiddenA,
                std::uint32_t forbiddenB) {
    dxfRW input(path.string().c_str());
    Capture capture;
    if (!input.read(&capture, false) || capture.roots.size() != 1u
        || capture.dictionaries.size() != 1u)
        return false;
    const auto& root = capture.roots.front();
    const auto& dictionary = capture.dictionaries.front();
    std::uint32_t rootTarget = 0;
    for (const auto& entry : root.m_entries)
        if (entry.m_name == "ACAD_PLOTSTYLENAME") rootTarget = entry.m_handle;
    if (rootTarget != dictionary.handle || dictionary.parentHandle != root.handle
        || dictionary.m_entries.size() != 1u
        || dictionary.m_entries.front().m_name != "Normal"
        || dictionary.m_entries.front().m_handle != dictionary.m_defaultEntryHandle
        || dictionary.handle == 0 || dictionary.m_defaultEntryHandle == 0
        || dictionary.handle == dictionary.m_defaultEntryHandle
        || dictionary.handle == forbiddenA || dictionary.handle == forbiddenB
        || dictionary.m_defaultEntryHandle == forbiddenA
        || dictionary.m_defaultEntryHandle == forbiddenB)
        return false;
    std::size_t found = 0;
    for (const auto& object : capture.objects)
        if (object.name == "ACDBPLACEHOLDER"
            && object.handle == dictionary.m_defaultEntryHandle
            && object.parentHandle == dictionary.handle) ++found;
    if (found != 1u) return false;
    for (const auto& layer : capture.layers) {
        std::uint32_t plotStyle = 0;
        bool present = false;
        if (!layer.getDxfPlotStyleHandle(plotStyle, present)
            || !present || plotStyle != dictionary.m_defaultEntryHandle)
            return false;
    }
    return !capture.layers.empty();
}
}

int main(int argc, char** argv) {
    if (argc != 2 && argc != 3) return 2;
    const auto path = std::filesystem::path(argv[1]);
    for (const auto version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                               DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        for (bool binary : {false, true}) {
            for (unsigned reserveMask : {0u, 1u, 2u, 3u}) {
                dxfRW output(path.string().c_str());
                LC3CompatCheck adapter;
                if (((reserveMask & 1u) && !output.reserveHandle(0xEu))
                    || ((reserveMask & 2u) && !output.reserveHandle(0xFu))) return 1;
                if (!output.write(&adapter, version, binary)
                    || !checkGraph(path, (reserveMask & 1u) ? 0xEu : 0u,
                                   (reserveMask & 2u) ? 0xFu : 0u)) {
                    std::cerr << "default plot-style graph failed: version=" << version
                              << " binary=" << binary << " reserveMask=" << reserveMask << '\n';
                    return 1;
                }
                // The same facade must plan a fresh equivalent graph on retry.
                if (!output.write(&adapter, version, binary)
                    || !checkGraph(path, (reserveMask & 1u) ? 0xEu : 0u,
                                   (reserveMask & 2u) ? 0xFu : 0u)) return 1;
                if (argc == 3) {
                    const auto evidence = std::filesystem::path(argv[2])
                        / (std::to_string(static_cast<int>(version))
                           + (binary ? "-binary-" : "-ascii-")
                           + std::to_string(reserveMask) + ".dxf");
                    std::filesystem::copy_file(path, evidence,
                        std::filesystem::copy_options::overwrite_existing);
                }
            }
        }
    }
    // Explicit source F cannot silently name an unrelated or absent object
    // if reservations force the generated Normal placeholder elsewhere.
    for (const auto version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                               DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        for (bool binary : {false, true}) {
            for (unsigned reserveMask : {0u, 1u, 2u, 3u}) {
                dxfRW output(path.string().c_str());
                ExplicitDefaultExport adapter(output);
                if (((reserveMask & 1u) && !output.reserveHandle(0xEu))
                    || ((reserveMask & 2u) && !output.reserveHandle(0xFu))) return 1;
                { std::ofstream sentinel(path, std::ios::binary); sentinel << "previous file"; }
                const bool written = output.write(&adapter, version, binary);
                if (reserveMask == 0u) {
                    if (!written || !checkGraph(path, 0u, 0u)) return 1;
                } else {
                    if (written || bytes(path) != "previous file") {
                        std::cerr << "explicit plot-style F did not refuse relocated target: version="
                                  << version << " binary=" << binary
                                  << " reserveMask=" << reserveMask << '\n';
                        return 1;
                    }
                    // The same writer remains reusable after correction.
                    adapter.layer.handlePlotS.clear();
                    if (!output.write(&adapter, version, binary)
                        || !checkGraph(path, (reserveMask & 1u) ? 0xEu : 0u,
                                       (reserveMask & 2u) ? 0xFu : 0u)) return 1;
                }
                if (argc == 3) {
                    const auto evidence = std::filesystem::path(argv[2])
                        / ("explicit-" + std::to_string(static_cast<int>(version))
                           + (binary ? "-binary-" : "-ascii-")
                           + std::to_string(reserveMask) + ".dxf");
                    std::filesystem::copy_file(path, evidence,
                        std::filesystem::copy_options::overwrite_existing);
                }
            }
        }
    }
    // A caller-authored graph must be declared before LAYER and physically
    // emitted with the same default edge and reciprocal owner roles.
    {
        const std::string source =
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1027\n0\nENDSEC\n"
            "0\nSECTION\n2\nOBJECTS\n0\nDICTIONARY\n5\nC\n330\n0\n"
            "100\nAcDbDictionary\n3\nACAD_PLOTSTYLENAME\n350\nA0\n"
            "0\nDICTIONARYWDFLT\n5\nA0\n330\nC\n100\nAcDbDictionary\n"
            "281\n1\n3\nNormal\n350\nA1\n100\nAcDbDictionaryWithDefault\n"
            "340\nA1\n0\nACDBPLACEHOLDER\n5\nA1\n330\nA0\n0\nENDSEC\n0\nEOF\n";
        { std::ofstream input(path, std::ios::binary); input << source; }
        Capture parsed;
        dxfRW input(path.string().c_str());
        if (!input.read(&parsed, false) || parsed.dictionaries.size() != 1u) return 1;
        for (const auto version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                                   DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
            for (bool binary : {false, true}) {
                for (bool remapped : {false, true}) {
                    dxfRW output(path.string().c_str());
                    CallerGraphExport adapter(output, parsed.dictionaries.front());
                    DRW_Class dictionaryClass, placeholderClass;
                    if (!dxfRW::dxfClassForRecordName("ACDBDICTIONARYWDFLT", dictionaryClass)
                        || !dxfRW::dxfClassForRecordName("ACDBPLACEHOLDER", placeholderClass)) return 1;
                    dictionaryClass.instanceCount = placeholderClass.instanceCount = 1;
                    output.setDxfClasses(std::vector<DRW_Class>{dictionaryClass, placeholderClass});
                    output.setRootDictEntries({{"ACAD_PLOTSTYLENAME", remapped ? "B0" : "A0"}});
                    if (remapped) output.setHandleRemap({{0xA0u, 0xB0u}, {0xA1u, 0xB1u}});
                    if (!output.setDxfDefaultPlotStyleSourceHandles(0xA0u, 0xA1u)
                        || output.setDxfDefaultPlotStyleSourceHandles(0u, 0xA1u)
                        || output.setDxfDefaultPlotStyleSourceHandles(0xA0u, 0u)
                        || output.setDxfDefaultPlotStyleSourceHandles(0xA0u, 0xA0u)
                        || !output.write(&adapter, version, binary)
                        || !checkGraph(path, 0u, 0u)) {
                        std::cerr << "caller plot-style graph failed: version=" << version
                                  << " binary=" << binary << " remapped=" << remapped << '\n';
                        return 1;
                    }
                    for (const auto mode : {CallerGraphExport::Mode::MissingDictionary,
                                             CallerGraphExport::Mode::MissingPlaceholder,
                                             CallerGraphExport::Mode::WrongDefault,
                                             CallerGraphExport::Mode::WrongNormal,
                                             CallerGraphExport::Mode::WrongNormalTarget,
                                             CallerGraphExport::Mode::WrongNormalRole,
                                             CallerGraphExport::Mode::WrongOwner,
                                             CallerGraphExport::Mode::LateRoot}) {
                        adapter.mode = mode;
                        { std::ofstream sentinel(path, std::ios::binary); sentinel << "previous file"; }
                        if (output.write(&adapter, version, binary) || bytes(path) != "previous file") {
                            std::cerr << "caller plot-style graph accepted invalid mode "
                                      << static_cast<int>(mode) << '\n';
                            return 1;
                        }
                        adapter.mode = CallerGraphExport::Mode::Valid;
                        if (!output.write(&adapter, version, binary) || !checkGraph(path, 0u, 0u)) return 1;
                    }
                    if (argc == 3) {
                        const auto evidence = std::filesystem::path(argv[2])
                            / ("caller-" + std::to_string(static_cast<int>(version))
                               + (binary ? "-binary-" : "-ascii-")
                               + (remapped ? "remapped.dxf" : "identity.dxf"));
                        std::filesystem::copy_file(path, evidence,
                            std::filesystem::copy_options::overwrite_existing);
                    }
                    if (!output.setDxfDefaultPlotStyleSourceHandles(0u, 0u)) return 1;
                    output.setRootDictEntries({});
                    LC3CompatCheck ordinary;
                    if (!output.write(&ordinary, version, binary)
                        || !checkGraph(path, 0u, 0u)) return 1;
                }
            }
        }
        // Setup failures leave the destination intact and do not poison retry.
        {
            dxfRW output(path.string().c_str());
            CallerGraphExport adapter(output, parsed.dictionaries.front());
            if (!output.setDxfDefaultPlotStyleSourceHandles(0xA0u, 0xA1u)) return 1;
            { std::ofstream sentinel(path, std::ios::binary); sentinel << "previous file"; }
            if (output.write(&adapter, DRW::AC1024, false)
                || bytes(path) != "previous file") return 1; // no root
            output.setRootDictEntries({{"ACAD_PLOTSTYLENAME", "A0"}});
            if (output.write(&adapter, DRW::AC1024, false)
                || bytes(path) != "previous file") return 1; // no classes
            DRW_Class dictionaryClass, placeholderClass;
            if (!dxfRW::dxfClassForRecordName("ACDBDICTIONARYWDFLT", dictionaryClass)
                || !dxfRW::dxfClassForRecordName("ACDBPLACEHOLDER", placeholderClass)) return 1;
            output.setDxfClasses(std::vector<DRW_Class>{dictionaryClass, placeholderClass});
            if (output.write(&adapter, DRW::AC1014, false)
                || bytes(path) != "previous file") return 1; // no R14 layer-390 profile
            if (!output.write(&adapter, DRW::AC1024, false) || !checkGraph(path, 0u, 0u)) return 1;
        }
        // A different source identity cannot borrow the declared target F.
        for (bool binary : {false, true}) {
            dxfRW output(path.string().c_str());
            CallerGraphExport adapter(output, parsed.dictionaries.front());
            DRW_Class dictionaryClass, placeholderClass;
            if (!dxfRW::dxfClassForRecordName("ACDBDICTIONARYWDFLT", dictionaryClass)
                || !dxfRW::dxfClassForRecordName("ACDBPLACEHOLDER", placeholderClass)) return 1;
            output.setDxfClasses(std::vector<DRW_Class>{dictionaryClass, placeholderClass});
            output.setRootDictEntries({{"ACAD_PLOTSTYLENAME", "B0"}});
            output.setHandleRemap({{0xA0u, 0xB0u}, {0xA1u, 0xFu}});
            if (!output.setDxfDefaultPlotStyleSourceHandles(0xA0u, 0xA1u)) return 1;
            adapter.mode = CallerGraphExport::Mode::Alias;
            { std::ofstream sentinel(path, std::ios::binary); sentinel << "previous file"; }
            if (output.write(&adapter, DRW::AC1024, binary)
                || bytes(path) != "previous file") return 1;
            adapter.mode = CallerGraphExport::Mode::Valid;
            adapter.explicitLayer.handlePlotS = "A1";
            if (!output.write(&adapter, DRW::AC1024, binary)
                || !checkGraph(path, 0u, 0u)) return 1;
        }
    }
    // A source identity resolving to the generated placeholder must not
    // silently become an alias for the synthetic Normal object.
    {
        dxfRW output(path.string().c_str());
        AliasExport adapter(output);
        output.setReferenceResolver([](std::uint32_t source) {
            return source == 0x123u ? 0xFu : source;
        });
        { std::ofstream sentinel(path, std::ios::binary); sentinel << "previous file"; }
        if (output.write(&adapter, DRW::AC1024, false)
            || bytes(path) != "previous file") return 1;
        adapter.layer.handlePlotS.clear();
        output.setReferenceResolver({});
        if (!output.write(&adapter, DRW::AC1024, false)
            || !checkGraph(path, 0u, 0u)) return 1;
    }
    for (const auto mode : {AdversarialExport::Mode::LateRoot,
                            AdversarialExport::Mode::LatePlaceholder}) {
        dxfRW output(path.string().c_str());
        AdversarialExport adapter(output);
        adapter.mode = mode;
        { std::ofstream sentinel(path, std::ios::binary); sentinel << "previous file"; }
        if (output.write(&adapter, DRW::AC1024, false)
            || bytes(path) != "previous file") return 1;
        adapter.mode = AdversarialExport::Mode::Clean;
        if (!output.write(&adapter, DRW::AC1024, false)
            || !checkGraph(path, 0u, 0u)) return 1;
    }
    {
        dxfRW output(path.string().c_str());
        LC3CompatCheck adapter;
        DRW_Class conflict;
        conflict.recName = "ACDBPLACEHOLDER";
        conflict.className = "ConflictingClass";
        conflict.appName = "ObjectDBX Classes";
        output.setDxfClasses(std::vector<DRW_Class>{conflict});
        { std::ofstream sentinel(path, std::ios::binary); sentinel << "previous file"; }
        if (output.write(&adapter, DRW::AC1024, false)
            || bytes(path) != "previous file") return 1;
        output.setDxfClasses(std::vector<DRW_Class>{});
        if (!output.write(&adapter, DRW::AC1024, false)
            || !checkGraph(path, 0u, 0u)) return 1;
    }
    // Existing compatible class registrations are retained and have one
    // additional emitted instance, without mutating the caller's classes.
    {
        dxfRW output(path.string().c_str());
        LC3CompatCheck adapter;
        DRW_Class dictionaryClass;
        DRW_Class placeholderClass;
        if (!dxfRW::dxfClassForRecordName("DICTIONARYWDFLT", dictionaryClass)
            || !dxfRW::dxfClassForRecordName("ACDBPLACEHOLDER", placeholderClass)) return 1;
        dictionaryClass.instanceCount = 3;
        placeholderClass.instanceCount = 7;
        const std::vector<DRW_Class> supplied{dictionaryClass, placeholderClass};
        output.setDxfClasses(supplied);
        if (!output.write(&adapter, DRW::AC1024, false)
            || supplied[0].instanceCount != 3 || supplied[1].instanceCount != 7
            || !checkGraph(path, 0u, 0u)) return 1;
        dxfRW input(path.string().c_str());
        Capture capture;
        if (!input.read(&capture, false)) return 1;
        bool dictionaryFound = false, placeholderFound = false;
        for (const auto& cls : capture.classes) {
            if (cls.recName == "DICTIONARYWDFLT" && cls.instanceCount == 4) dictionaryFound = true;
            if (cls.recName == "ACDBPLACEHOLDER" && cls.instanceCount == 8) placeholderFound = true;
        }
        if (!dictionaryFound || !placeholderFound) return 1;
    }
    // Disabling an optional caller inventory with a zero limit must not
    // accidentally disable the writer's required graph collision check.
    {
        dxfRW output(path.string().c_str());
        LC3CompatCheck adapter;
        output.setDxfWriteRecordInventoryEnabled(false, 0u);
        if (!output.write(&adapter, DRW::AC1024, false)
            || !checkGraph(path, 0u, 0u)) return 1;
        output.setDxfWriteRecordInventoryEnabled(true, 0u);
        { std::ofstream sentinel(path, std::ios::binary); sentinel << "previous file"; }
        if (output.write(&adapter, DRW::AC1024, false)
            || bytes(path) != "previous file") return 1;
        output.setDxfWriteRecordInventoryEnabled(false, 0u);
        if (!output.write(&adapter, DRW::AC1024, false)
            || !checkGraph(path, 0u, 0u)) return 1;
    }
    for (const auto version : {DRW::AC1009, DRW::AC1014}) {
        dxfRW output(path.string().c_str());
        LC3CompatCheck adapter;
        if (!output.write(&adapter, version, false)) return 1;
        dxfRW input(path.string().c_str());
        Capture capture;
        if (!input.read(&capture, false) || !capture.dictionaries.empty()) return 1;
    }
    std::filesystem::remove(path);
    return 0;
}
