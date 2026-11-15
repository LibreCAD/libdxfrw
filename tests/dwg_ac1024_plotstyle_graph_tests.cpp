// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_iface.h"
#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

class DxIfaceStandardDwgCaptureTestAccess {
public:
    static bool select(const dx_data& source, dx_data& out) {
        if (source.sourceFormat != dx_source_format::DWG
            || source.sourceVersion != DRW::AC1024
            || !source.hasSingleSuccessfulDictionaryDefaultImport()
            || !source.hasNamedObjectDictionary
            || source.dictionaryWithDefaults.size() != 1u || source.placeholders.size() != 1u)
            return false;
        const auto& dictionary = source.dictionaryWithDefaults.front();
        const auto& placeholder = source.placeholders.front();
        if (!dictionary.reactorHandles.empty() || !placeholder.reactorHandles.empty()
            || dictionary.m_entries.size() != 1u || dictionary.m_entries.front().m_name != "Normal"
            || dictionary.m_entries.front().m_handle != placeholder.handle
            || dictionary.m_defaultEntryHandle != placeholder.handle) return false;
        out.sourceFormat = source.sourceFormat; out.sourceVersion = source.sourceVersion;
        out.importGeneration = source.importGeneration; out.dwgReadOutcome = source.dwgReadOutcome;
        out.headerC = source.headerC; out.lineTypes = source.lineTypes;
        out.hasNamedObjectDictionary = true; out.namedObjectDictionary = source.namedObjectDictionary;
        out.namedObjectDictionary.m_entries.erase(std::remove_if(
            out.namedObjectDictionary.m_entries.begin(), out.namedObjectDictionary.m_entries.end(),
            [&dictionary](const DRW_Dictionary::Entry& entry) {
                return entry.m_name != "ACAD_PLOTSTYLENAME" || entry.m_handle != dictionary.handle;
            }), out.namedObjectDictionary.m_entries.end());
        if (out.namedObjectDictionary.m_entries.size() != 1u) return false;
        out.dictionaryWithDefaults.push_back(dictionary); out.placeholders.push_back(placeholder);
        out.dictionaryDefaultImport = source.dictionaryDefaultImport;
        out.dictionaryDefaultTypedRetainedRecords = source.dictionaryDefaultTypedRetainedRecords;
        out.dictionaryDefaultTypedRetainedItems = source.dictionaryDefaultTypedRetainedItems;
        out.dictionaryDefaultTypedRetainedBytes = source.dictionaryDefaultTypedRetainedBytes;
        out.basicTableCaptureFailed = source.basicTableCaptureFailed;
        out.dwgClassCoverage = source.dwgClassCoverage;
        out.m_opaqueDwgSourceFrames = source.m_opaqueDwgSourceFrames;
        out.m_opaqueDwgSourceFrameIdentities = source.m_opaqueDwgSourceFrameIdentities;
        out.m_opaqueDwgSourceFrameRetainedBytes = source.m_opaqueDwgSourceFrameRetainedBytes;
        out.m_classEvidenceImports = source.m_classEvidenceImports;
        out.m_classEvidenceFailure = source.m_classEvidenceFailure;
        out.m_classEvidenceAttempt = source.m_classEvidenceAttempt;
        out.m_classEvidenceGeneration = source.m_classEvidenceGeneration;
        out.m_classEvidenceHighWater = source.m_classEvidenceHighWater;
        out.m_classEvidenceOwner = source.m_classEvidenceOwner;
        out.m_classEvidenceBytes = source.m_classEvidenceBytes;
        out.m_classEvidenceByteLimit = source.m_classEvidenceByteLimit;
        out.m_classEvidenceCountLimit = source.m_classEvidenceCountLimit;
        out.m_classEvidenceFallback = source.m_classEvidenceFallback;
        out.m_classSourceReports = source.m_classSourceReports;
        out.m_classSourceFailure = source.m_classSourceFailure;
        out.m_classSourceFailureGeneration = source.m_classSourceFailureGeneration;
        return out.hasSingleSuccessfulDictionaryDefaultImport();
    }
    static DRW_UnsupportedObject& frame(dx_data& data, std::uint32_t handle) {
        return data.m_opaqueDwgSourceFrames.at({data.importGeneration, handle});
    }
};

class PlotStyleAuthor final : public LC3CompatCheck {
public:
    explicit PlotStyleAuthor(dwgRW& writer) : output(writer) {
        dictionary.handle = 0xA10u; dictionary.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        dictionary.cloning = 1; dictionary.hardOwner = 0;
        dictionary.m_entries.push_back({"Normal", 0xA11u}); dictionary.m_defaultEntryHandle = 0xA11u;
        dictionary.setDwgCommonObjectState(0, 1u, false);
        placeholder.handle = 0xA11u; placeholder.parentHandle = dictionary.handle;
        placeholder.setDwgCommonObjectState(0, 1u, false);
    }
    void writeDwgClasses() override {
        registered = output.registerDictionaryWithDefaultObjectClass(&dictionary)
            && output.registerDwgNamedObjectDictionaryEntry("ACAD_PLOTSTYLENAME", dictionary.handle);
    }
    void writeObjects() override {
        written = output.writeDictionaryWithDefault(&dictionary)
            && output.writeAcDbPlaceholder(&placeholder);
    }
    dwgRW& output; DRW_DictionaryWithDefault dictionary; DRW_AcDbPlaceholder placeholder;
    bool registered = false, written = false;
};
static bool verify(const std::filesystem::path& path) {
    dx_data readback; dx_iface reader;
    if (!reader.fileImport(path.string(), &readback, false)
        || !readback.hasNamedObjectDictionary || readback.dictionaryWithDefaults.size() != 1u
        || readback.rawPlaceholders.size() != 1u) return false;
    const auto& d = readback.dictionaryWithDefaults.front();
    const auto& p = readback.rawPlaceholders.front();
    return std::count_if(readback.namedObjectDictionary.m_entries.begin(),
        readback.namedObjectDictionary.m_entries.end(), [&d](const auto& entry) {
            return entry.m_name == "ACAD_PLOTSTYLENAME" && entry.m_handle == d.handle;
        }) == 1 && d.m_entries.size() == 1u && d.m_entries.front().m_name == "Normal"
        && d.m_entries.front().m_handle == p.handle && d.m_defaultEntryHandle == p.handle
        && d.parentHandle == readback.namedObjectDictionary.handle && p.parentHandle == d.handle
        && d.reactorHandles.empty()
        && std::none_of(p.groups.begin(), p.groups.end(), [](const DRW_Variant& group) {
            return group.code() == 102;
        });
}
static bool check(const std::filesystem::path& input, const std::filesystem::path& directory) {
    dx_iface sourceReader; dx_data source;
    if (!sourceReader.fileImport(input.string(), &source, false)) return false;
    dx_data selected;
    if (!DxIfaceStandardDwgCaptureTestAccess::select(source, selected)) {
        std::cerr << "source plot-style projection failed\n"; return false;
    }
    std::filesystem::create_directories(directory);
    for (const bool binary : {false, true}) {
        const auto output = directory / (binary ? "selected-binary.dxf" : "selected-ascii.dxf");
        class LateMutationWriter final : public dx_iface {
        public:
            std::uint32_t handle{0u};
            bool mutate{false};
            void writeObjects() override {
                dx_iface::writeObjects();
                if (mutate && cData != nullptr)
                    DxIfaceStandardDwgCaptureTestAccess::frame(*cData, handle)
                        .m_rawBytes.front() ^= 0x80u;
            }
        } writer;
        writer.handle = selected.dictionaryWithDefaults.front().handle;
        const auto run = [&]() { return writer.fileExport(output.string(), DRW::AC1024, binary,
            &selected, true, false, dx_dxfSourceTextPolicy::Compatible,
            DRW::DxfTextWritePolicy::Compatible, dx_dxfProxyGraphicsPolicy::Compatible,
            dx_dwgReadCompletenessPolicy::Legacy); };
        if (!run()) {
            std::cerr << "selected plot-style export failed: " << writer.extensionGraphFailureDetail() << '\n';
            return false;
        }
        if (!verify(output)) {
            std::cerr << "selected plot-style readback failed\n";
            return false;
        }
        const std::string sentinel = "keep plot-style output\n";
        auto refuse = [&]() {
            { std::ofstream f(output, std::ios::binary | std::ios::trunc); f << sentinel; }
            if (run()) return false;
            std::ifstream f(output, std::ios::binary);
            return std::string(std::istreambuf_iterator<char>(f), {}) == sentinel;
        };
        auto& dict = selected.dictionaryWithDefaults.front();
        const auto savedDefault = dict.m_defaultEntryHandle;
        dict.m_defaultEntryHandle = 0u;
        if (!refuse()) { std::cerr << "changed default was not refused atomically\n"; return false; }
        dict.m_defaultEntryHandle = savedDefault;
        dict.setDwgCommonObjectState(1, 1u, false);
        if (!refuse()) { std::cerr << "changed reactor count was not refused atomically\n"; return false; }
        dict.setDwgCommonObjectState(0, 1u, false);
        auto& placeholder = selected.placeholders.front();
        placeholder.setDwgCommonObjectState(0, 0u, false);
        if (!refuse()) { std::cerr << "changed extension flag was not refused atomically\n"; return false; }
        placeholder.setDwgCommonObjectState(0, 1u, false);
        auto& raw = DxIfaceStandardDwgCaptureTestAccess::frame(selected, dict.handle);
        const auto savedByte = raw.m_rawBytes.front(); raw.m_rawBytes.front() ^= 0x80u;
        if (!refuse()) { std::cerr << "changed source frame was not refused atomically\n"; return false; }
        raw.m_rawBytes.front() = savedByte;
        writer.mutate = true;
        if (!refuse()) { std::cerr << "late source mutation was not refused atomically\n"; return false; }
        raw.m_rawBytes.front() = savedByte;
        writer.mutate = false;
        if (!run() || !verify(output)) { std::cerr << "restored plot-style retry failed\n"; return false; }
    }
    return true;
}
int main(int argc, char** argv) {
    if (argc == 4 && std::string(argv[1]) == "--physical") return check(argv[2], argv[3]) ? 0 : 1;
    if (argc != 2) return 2;
    const std::filesystem::path root = argv[1]; std::filesystem::create_directories(root);
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto scratch = root / ("plot-style-" + std::to_string(nonce));
    std::error_code error; if (!std::filesystem::create_directory(scratch, error)) return 1;
    const auto source = scratch / "generated.dwg";
    struct Cleanup { std::filesystem::path root; ~Cleanup() { std::error_code e; std::filesystem::remove_all(root, e); } } cleanup{scratch};
    dwgRW writer(source.string().c_str()); PlotStyleAuthor author(writer);
    if (!writer.reserveHandle(author.dictionary.handle) || !writer.reserveHandle(author.placeholder.handle)
        || !writer.write(&author, DRW::AC1024, false) || !author.registered || !author.written
        || !check(source, scratch)) return 1;
    return 0;
}
