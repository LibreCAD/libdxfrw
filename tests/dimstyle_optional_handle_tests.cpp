#include "dx_iface.h"
#include "intern/dxfreader.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>

namespace {

bool fail(const std::string& message) {
    std::cerr << message << '\n';
    return false;
}

bool generate(const std::string& path, bool binary, bool directVars) {
    dx_data data;
    auto* line = new DRW_Line;
    line->basePoint = DRW_Coord(1.25, -2.5, 3.75);
    line->secPoint = DRW_Coord(-4.5, 5.25, -6.75);
    data.mBlock->ent.push_back(line);
    if (directVars) {
        for (const std::string& name : {std::string("Empty"),
                                       std::string("Zero"),
                                       std::string("Reference")}) {
            DRW_Dimstyle style;
            style.name = name;
            const std::string value = name == "Empty" ? ""
                : name == "Zero" ? "0" : "A9";
            for (int code = 341; code <= 347; ++code)
                style.add("OPTIONAL_" + std::to_string(code), code, value);
            data.dimStyles.push_back(style);
        }
    }
    dx_iface iface;
    return iface.fileExport(path, DRW::AC1027, binary, &data, false)
        ? true : fail("could not export DIMSTYLE optional-handle control");
}

bool verify(const std::string& path, bool directVars) {
    std::ifstream input(path, std::ios::binary);
    char signature[22] {};
    input.read(signature, sizeof signature);
    const bool binary = std::string(signature, sizeof signature)
        == std::string("AutoCAD Binary DXF\r\n\x1a\0", 22);
    input.clear();
    input.seekg(binary ? 22 : 0);
    std::unique_ptr<dxfReader> reader;
    if (binary) reader = std::make_unique<dxfReaderBinary>(&input);
    else reader = std::make_unique<dxfReaderAscii>(&input);
    // Match the documented legacy DIMSTYLE string-name reader policy, but
    // still check the literal emitted optional handle tags independently.
    reader->setAllowDimstyleNames(true);
    std::map<std::string, std::map<int, std::string>> references;
    std::string type, name;
    bool eof = false, line = false;
    int code = 0;
    while (reader->readRec(&code)) {
        if (code == 0) {
            type = reader->getString();
            name.clear();
            eof = type == "EOF";
            line = line || type == "LINE";
        } else if (type == "DIMSTYLE" && code == 2) {
            name = reader->getString();
            references.emplace(name, std::map<int, std::string>{});
        } else if (type == "DIMSTYLE" && code >= 341 && code <= 347) {
            const std::string value = reader->getString();
            if (value.empty())
                return fail("writer emitted an empty optional DIMSTYLE handle");
            if (!references[name].emplace(code, value).second)
                return fail("writer duplicated an optional DIMSTYLE handle");
        }
    }
    if (!eof || !line)
        return fail("raw DIMSTYLE control scan did not reach LINE and EOF");
    if (!directVars)
        return true;
    if (!references.count("Empty") || !references["Empty"].empty())
        return fail("unset optional DIMSTYLE handles were not omitted");
    for (const std::string& style : {std::string("Zero"), std::string("Reference")}) {
        if (references[style].size() != 7)
            return fail("explicit optional DIMSTYLE handle count changed");
        for (int code = 341; code <= 347; ++code) {
            if (references[style][code] != (style == "Zero" ? "0" : "A9"))
                return fail("explicit null/nonempty DIMSTYLE handle changed");
        }
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 3) {
        const std::string mode = argv[1];
        if (mode == "--generate") return generate(argv[2], false, false) ? 0 : 1;
        if (mode == "--generate-binary") return generate(argv[2], true, false) ? 0 : 1;
        if (mode == "--verify") return verify(argv[2], false) ? 0 : 1;
        return 2;
    }
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-dimstyle-handles-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    for (bool binary : {false, true}) {
        const auto path = directory / (binary ? "vars binary.dxf" : "vars ascii.dxf");
        if (!generate(path.string(), binary, true) || !verify(path.string(), true)) {
            std::cerr << "Evidence: " << directory << '\n';
            return 1;
        }
    }
    std::filesystem::remove_all(directory);
    return 0;
}
