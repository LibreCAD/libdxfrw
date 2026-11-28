// SPDX-License-Identifier: GPL-2.0-or-later
// Narrow ACAD_PLOTSTYLENAME admission controls. No drawing fixtures are kept.
#include "lc3_compat_check.cpp"
#include "intern/dwgwriter15.h"
#include "intern/dwgwriter18.h"
#include "intern/dwgwriter21.h"
#include "intern/dwgwriter24.h"
#include "intern/dwgwriter27.h"
#include "intern/dwgwriter32.h"
#include "libdwgr.h"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

template<class Writer> bool admissionControls() {
    DRW_Header header;
    Writer writer(nullptr, &header);
    if (!writer.registerDwgNamedObjectDictionaryEntry("OTHER", 0xA100u)) return false;
    const auto checkpoint = writer.checkpointPublicTransaction();
    if (!writer.registerDwgNamedObjectDictionaryEntry("aCaD_PloTstyLeNaMe", 0xA101u)
        || writer.registerDwgNamedObjectDictionaryEntry("ACAD_PLOTSTYLENAME", 0xA102u)
        || writer.registerDwgNamedObjectDictionaryEntry("ALIAS", 0xA101u)
        || !writer.registerDwgNamedObjectDictionaryEntry("OTHER_AFTER", 0xA102u))
        return false;
    writer.rollbackCompoundWrite(checkpoint);
    if (writer.registerDwgNamedObjectDictionaryEntry("acad_plotstylename", 0xA100u)
        || writer.registerDwgNamedObjectDictionaryEntry("ACAD_PLOTSTYLENAME", DRW::NoHandle)
        || !writer.registerDwgNamedObjectDictionaryEntry("ACAD_plotSTYLENAME", 0xA103u)
        || writer.registerDwgNamedObjectDictionaryEntry("aCaD_PLOTSTYLENAME", 0xA104u)
        || writer.registerDwgNamedObjectDictionaryEntry("OTHER_ALIAS", 0xA103u))
        return false;
    // This narrowly targeted rule does not change unrelated NOD collation.
    if (!writer.registerDwgNamedObjectDictionaryEntry("CUSTOM", 0xA105u)
        || !writer.registerDwgNamedObjectDictionaryEntry("custom", 0xA106u))
        return false;
    DRW_Header beforeHeader, afterHeader;
    Writer groupBefore(nullptr, &beforeHeader), groupAfter(nullptr, &afterHeader);
    return groupBefore.registerDwgGroupDictionaryEntry("GROUP", 0xA107u)
        && !groupBefore.registerDwgNamedObjectDictionaryEntry("ACAD_PLOTSTYLENAME", 0xA107u)
        && groupBefore.registerDwgNamedObjectDictionaryEntry("ACAD_PLOTSTYLENAME", 0xA108u)
        && groupAfter.registerDwgNamedObjectDictionaryEntry("ACAD_PLOTSTYLENAME", 0xA109u)
        && !groupAfter.registerDwgGroupDictionaryEntry("GROUP", 0xA109u)
        && groupAfter.registerDwgGroupDictionaryEntry("GROUP", 0xA10Au);
}

struct GraphAdapter : LC3CompatCheck {
    explicit GraphAdapter(dwgRW& output, DRW::Version version)
        : output(output) {
        dictionary.handle = 0xB100u;
        dictionary.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        dictionary.cloning = 1;
        dictionary.setDwgCommonObjectState(0, version >= DRW::AC1018 ? 1 : 0, false);
        dictionary.m_entries = {{"Normal", 0xB101u}};
        dictionary.m_defaultEntryHandle = 0xB101u;
        placeholder.handle = 0xB101u;
        placeholder.parentHandle = dictionary.handle;
        placeholder.setDwgCommonObjectState(0, version >= DRW::AC1018 ? 1 : 0, false);
    }
    void writeDwgClasses() override {
        registered = lateRoot || output.registerDwgNamedObjectDictionaryEntry(
            "aCaD_PlotStyleName", dictionary.handle);
        if (tryDuplicate) {
            duplicateRejected = !output.registerDwgNamedObjectDictionaryEntry(
                "ACAD_PLOTSTYLENAME", 0xB102u);
        }
        classRegistered = output.registerDictionaryWithDefaultObjectClass(&dictionary);
    }
    void writeLTypes() override {
        if (lateRoot)
            registered = output.registerDwgNamedObjectDictionaryEntry(
                "aCaD_PlotStyleName", dictionary.handle);
    }
    void writeObjects() override {
        written = output.writeDictionaryWithDefault(&dictionary)
            && output.writeAcDbPlaceholder(&placeholder);
    }
    bool finalizeDwgWrite() override { return registered && classRegistered && written; }
    dwgRW& output;
    DRW_DictionaryWithDefault dictionary;
    DRW_AcDbPlaceholder placeholder;
    bool tryDuplicate = true;
    bool lateRoot = false;
    bool registered = false, duplicateRejected = false, classRegistered = false, written = false;
};

struct Capture : LC3CompatCheck {
    void addNamedObjectDictionary(const DRW_Dictionary& value) override {
        for (const auto& e : value.m_entries) {
            if (e.m_name == "aCaD_PlotStyleName" && e.m_handle == 0xB100u)
                sawRoot = true;
        }
    }
    void addDictionaryWithDefault(const DRW_DictionaryWithDefault& value) override {
        sawDictionary = value.handle == 0xB100u
            && value.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
            && value.m_defaultEntryHandle == 0xB101u
            && value.m_entries.size() == 1u && value.m_entries[0].m_name == "Normal"
            && value.m_entries[0].m_handle == 0xB101u;
    }
    void addAcDbPlaceholder(const DRW_AcDbPlaceholder& value) override {
        sawPlaceholder = value.handle == 0xB101u && value.parentHandle == 0xB100u;
    }
    bool sawRoot = false, sawDictionary = false, sawPlaceholder = false;
};

bool facadeControls(const std::filesystem::path& path, DRW::Version version) {
    { std::ofstream out(path, std::ios::binary); out << "prior destination"; }
    const std::string name = path.string();
    dwgRW output(name.c_str());
    if (!output.reserveHandle(0xB100u) || !output.reserveHandle(0xB101u)) return false;
    GraphAdapter graph(output, version);
    if (output.write(&graph, version, true) || !graph.duplicateRejected) return false;
    { std::ifstream in(path, std::ios::binary);
      const std::string bytes((std::istreambuf_iterator<char>(in)), {});
      if (bytes != "prior destination") return false; }
    graph.tryDuplicate = false;
    if (!output.write(&graph, version, true)) return false;
    dwgRW reader(name.c_str());
    Capture capture;
    if (!reader.read(&capture, false) || !capture.sawRoot
        || !capture.sawDictionary || !capture.sawPlaceholder) {
        std::cerr << "plotstyle graph readback failed for " << version << '\n';
        return false;
    }
    // Late NOD admission remains supported until deferred OBJECTS starts;
    // the class is still registered in the established pre-CLASSES callback.
    graph.lateRoot = true;
    graph.registered = false;
    graph.classRegistered = false;
    graph.written = false;
    if (!output.write(&graph, version, true)) return false;
    dwgRW lateReader(name.c_str());
    Capture late;
    return lateReader.read(&late, false) && late.sawRoot
        && late.sawDictionary && late.sawPlaceholder;
}

int main(int argc, char** argv) {
    const bool keep = argc == 2 && std::string(argv[1]) == "--keep";
    if (argc > 2 || (argc == 2 && !keep)) return 2;
    if (!admissionControls<dwgWriter15>() || !admissionControls<dwgWriter18>()
        || !admissionControls<dwgWriter21>() || !admissionControls<dwgWriter24>()
        || !admissionControls<dwgWriter27>() || !admissionControls<dwgWriter32>())
        return 1;
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto dir = std::filesystem::temp_directory_path()
        / ("libdxfrw-plotstyle-root-" + std::to_string(nonce));
    if (!std::filesystem::create_directory(dir)) return 2;
    struct Cleanup { std::filesystem::path path; bool keep; ~Cleanup() {
        if (!keep) { std::error_code error; std::filesystem::remove_all(path,error); }
    }} cleanup{dir, keep};
    for (const DRW::Version version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                                       DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        const auto filename = "probe-" + std::to_string(version) + ".dwg";
        if (!facadeControls(dir / filename, version)) return 1;
    }
    if (keep) std::cout << "OUTPUT " << dir.string() << '\n';
    std::cout << "DWG plotstyle NOD admission PASS\n";
    return 0;
}
