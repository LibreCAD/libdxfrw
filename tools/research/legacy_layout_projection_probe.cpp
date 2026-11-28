// SPDX-License-Identifier: GPL-2.0-or-later
// Optional scratch-only source/target transaction probe. The diagnostic mode
// requires a separately built, explicitly guard-bypassed adapter; it does not
// authorize or change the production export policy. Never commit its output.
#include "dx_iface.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

static std::string bytes(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
static bool write(dx_iface& adapter, dx_data& data, const std::string& output, bool binary) {
    return adapter.fileExport(output, DRW::AC1015, binary, &data, false, false,
        dx_dxfSourceTextPolicy::Compatible, DRW::DxfTextWritePolicy::Compatible,
        dx_dxfProxyGraphicsPolicy::Compatible, dx_dwgReadCompletenessPolicy::AllowPartial);
}
int main(int argc, char** argv) {
    if (argc != 4 || (std::string(argv[3]) != "production-refusal"
                      && std::string(argv[3]) != "diagnostic-only")) {
        std::cerr << "usage: probe INPUT NEW_SCRATCH_DIRECTORY production-refusal|diagnostic-only\n";
        return 2;
    }
    const bool diagnostic = std::string(argv[3]) == "diagnostic-only";
    const auto outputDir = std::filesystem::absolute(argv[2]);
    if (std::filesystem::exists(outputDir) || !std::filesystem::create_directories(outputDir)) return 2;
    dx_iface adapter;
    dx_data source;
    if (!adapter.fileImport(argv[1], &source, false) || source.layouts.empty()
        || (source.sourceVersion != DRW::AC1012 && source.sourceVersion != DRW::AC1014)) return 3;
    const auto originalName = source.layouts.front().name;
    for (const bool binary : {false, true}) {
        const auto out = outputDir / (binary ? "binary.dxf" : "ascii.dxf");
        const std::string sentinel = "diagnostic destination sentinel\n";
        { std::ofstream file(out, std::ios::binary | std::ios::trunc); file << sentinel; }
        if (!diagnostic) {
            if (write(adapter, source, out.string(), binary) || bytes(out) != sentinel) return 4;
            continue;
        }
        source.layouts.front().name += "_changed";
        const bool refused = !write(adapter, source, out.string(), binary) && bytes(out) == sentinel;
        source.layouts.front().name = originalName;
        if (!refused || !write(adapter, source, out.string(), binary)) return 4;
        dx_iface reader;
        dx_data output;
        if (!reader.fileImport(out.string(), &output, false) || output.layouts.size() != source.layouts.size()) return 5;
        for (const DRW_Layout& layout : output.layouts) {
            const auto record = std::find_if(output.blockRecords.begin(), output.blockRecords.end(),
                [&layout](const DRW_Block_Record& value) { return value.handle == layout.paperSpaceBlockRecordHandle.ref; });
            if (record == output.blockRecords.end() || record->layoutHandle != layout.handle) return 6;
        }
        if (std::any_of(source.blockRecords.begin(), source.blockRecords.end(),
                [](const DRW_Block_Record& record) { return record.layoutHandle != 0u; })) return 7;
        const auto count = std::count_if(output.dxfClasses.begin(), output.dxfClasses.end(),
            [](const DRW_Class& value) { return value.recName == "LAYOUT" && value.className == "AcDbLayout"; });
        if (count != 1) return 8;
    }
    std::cout << (diagnostic ? "diagnostic output and retry" : "production refusal")
              << ": ASCII and binary PASS\n";
}
