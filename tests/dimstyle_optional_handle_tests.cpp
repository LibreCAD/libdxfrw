#include "dx_iface.h"
#include "intern/dxfreader.h"

#include <chrono>
#include <array>
#include <cstring>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

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

bool resourceRemap(const std::filesystem::path& directory) {
    struct Tag { int code; std::string value; };
    const auto sourceTags = [](const char* revision, bool modern) {
        std::vector<Tag> tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,revision},{0,"ENDSEC"},
            {0,"SECTION"},{2,"TABLES"},{0,"TABLE"},{2,"STYLE"},{70,"2"},
            {0,"STYLE"},{5,"A339"},{100,"AcDbSymbolTableRecord"},{100,"AcDbTextStyleTableRecord"},{2,"Standard"},{70,"0"},{3,"txt"},
            {0,"STYLE"},{5,"A340"},{100,"AcDbSymbolTableRecord"},{100,"AcDbTextStyleTableRecord"},{2,"RESOURCE_TEXT"},{70,"0"},{3,"txt"},
            {0,"ENDTAB"},{0,"TABLE"},{2,"LTYPE"},{70,"3"}};
        for (int code = 345; code <= 347; ++code) {
            const std::string name = "LTYPE_" + std::to_string(code);
            tags.insert(tags.end(), {{0,"LTYPE"},{5,"A"+std::to_string(code)},{100,"AcDbSymbolTableRecord"},
                {100,"AcDbLinetypeTableRecord"},{2,name},{70,"0"},{3,name},{72,"65"},{73,"0"},{40,"0"}});
        }
        tags.insert(tags.end(), {{0,"ENDTAB"},{0,"TABLE"},{2,"BLOCK_RECORD"},{70,"4"}});
        for (int code = 341; code <= 344; ++code)
            tags.insert(tags.end(), {{0,"BLOCK_RECORD"},{5,"A"+std::to_string(code)},{100,"AcDbSymbolTableRecord"},
                {100,"AcDbBlockTableRecord"},{2,"ARROW_"+std::to_string(code)},{70,"0"}});
        tags.insert(tags.end(), {{0,"ENDTAB"},{0,"TABLE"},{2,"DIMSTYLE"},{70,"1"},
            {0,"DIMSTYLE"},{105,"A350"},{100,"AcDbSymbolTableRecord"},{100,"AcDbDimStyleTableRecord"},
            {2,"RESOURCE_DIMS"},{70,"0"}});
        for (int code = 340; code <= (modern ? 347 : 344); ++code)
            tags.push_back({code,"A"+std::to_string(code)});
        tags.insert(tags.end(), {{0,"ENDTAB"},{0,"ENDSEC"},{0,"SECTION"},{2,"BLOCKS"}});
        for (int code = 341; code <= 344; ++code)
            tags.insert(tags.end(), {{0,"BLOCK"},{5,"B"+std::to_string(code)},{330,"A"+std::to_string(code)},
                {100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockBegin"},{2,"ARROW_"+std::to_string(code)},
                {70,"0"},{10,"0"},{20,"0"},{30,"0"},{0,"ENDBLK"},{5,"C"+std::to_string(code)},
                {330,"A"+std::to_string(code)},{100,"AcDbEntity"},{8,"0"},{100,"AcDbBlockEnd"}});
        tags.insert(tags.end(), {{0,"ENDSEC"},{0,"SECTION"},{2,"ENTITIES"},{0,"LINE"},{5,"D000"},{8,"0"},
            {10,"1"},{20,"2"},{30,"3"},{11,"4"},{21,"5"},{31,"6"},{0,"ENDSEC"},{0,"EOF"}});
        return tags;
    };
    const auto write = [](const auto& path, const auto& tags, bool binary) {
        std::ofstream stream(path, std::ios::binary);
        if (!stream) return false;
        const auto word = [&](std::uint64_t value, unsigned width) {
            for (unsigned i = 0u; i != width; ++i) stream.put(static_cast<char>(value >> (8u*i)));
        };
        if (binary) stream.write("AutoCAD Binary DXF\r\n\x1a\0", 22);
        for (const auto& tag : tags) {
            if (!binary) stream << tag.code << '\n' << tag.value << '\n';
            else {
                word(static_cast<unsigned>(tag.code), 2u);
                if (tag.code >= 10 && tag.code <= 59) {
                    const double value = std::stod(tag.value); std::uint64_t bits = 0u;
                    std::memcpy(&bits, &value, 8u); word(bits, 8u);
                } else if (tag.code >= 60 && tag.code <= 79) word(std::stoul(tag.value), 2u);
                else { stream << tag.value; stream.put('\0'); }
            }
        }
        return stream.good();
    };
    const auto check = [](const auto& path, bool modern, bool moved,
                          bool nulls = false, int badCode = 0, std::uint32_t badRef = 0u) {
        std::ifstream stream(path, std::ios::binary); char magic[22]{}; stream.read(magic, 22);
        const bool binary = std::string(magic,22) == std::string("AutoCAD Binary DXF\r\n\x1a\0",22);
        stream.clear(); stream.seekg(binary ? 22 : 0);
        std::unique_ptr<dxfReader> reader = binary ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&stream))
            : std::unique_ptr<dxfReader>(new dxfReaderAscii(&stream));
        std::map<std::string,std::uint32_t> targets, bodies;
        std::map<int,std::uint32_t> references;
        std::string type, name; std::uint32_t handle = 0u, owner = 0u;
        int code = 0; bool eof = false;
        while (reader->readRec(&code)) {
            if (code == 0) { type = reader->getString(); name.clear(); handle = owner = 0u; eof = type == "EOF"; }
            else if (code == 5 && reader->isValidHandleString()) handle = reader->getHandleString();
            else if (code == 330 && reader->isValidHandleString()) owner = reader->getHandleString();
            else if (code == 2) {
                name = reader->getString();
                if (type == "STYLE" || type == "LTYPE" || type == "BLOCK_RECORD") targets[type+":"+name] = handle;
                if (type == "BLOCK") bodies[name] = owner;
            } else if (type == "DIMSTYLE" && name == "RESOURCE_DIMS" && code >= 340 && code <= 347) {
                if (!reader->isValidHandleString() || !references.emplace(code,reader->getHandleString()).second) return false;
            }
        }
        if (!eof || references.size() != (modern ? 8u : 5u)) return false;
        for (int c = 340; c <= (modern ? 347 : 344); ++c) {
            if (nulls && c >= 341) { if (references[c] != 0u) return false; continue; }
            if (c == badCode) { if (references[c] != badRef) return false; continue; }
            const auto kind = c == 340 ? "STYLE:RESOURCE_TEXT" : c <= 344 ? "BLOCK_RECORD:ARROW_"+std::to_string(c)
                : "LTYPE:LTYPE_"+std::to_string(c);
            if (!targets.count(kind) || !targets.at(kind) || references[c] != targets.at(kind)
                || (moved && references[c] == std::stoul("A"+std::to_string(c),nullptr,16))) return false;
            if (c >= 341 && c <= 344 && bodies["ARROW_"+std::to_string(c)] != references[c]) return false;
        }
        return true;
    };
    const auto referenceState = [](const dx_data& data) {
        std::vector<std::string> result;
        for (const auto& style : data.dimStyles) {
            result.insert(result.end(), {style.dimtxsty,style.dimldrblk,style.dimblk,style.dimblk1,style.dimblk2});
            for (const auto* h : {&style.dimtxstyH,&style.dimldrblkH,&style.dimblkH,&style.dimblk1H,&style.dimblk2H,
                                  &style.dimltypeH,&style.dimltex1H,&style.dimltex2H})
                result.push_back(std::to_string(h->code)+":"+std::to_string(h->ref)+":"+std::to_string(h->ref64));
            for (const auto& item : style.vars) if (item.second && item.second->type() == DRW_Variant::STRING)
                result.push_back(item.first+":"+std::to_string(item.second->code())+":"+*item.second->content.s);
        }
        return result;
    };
    unsigned routes = 0u;
    const std::array<std::pair<DRW::Version,const char*>,6> versions{{
        {DRW::AC1015,"AC1015"},{DRW::AC1018,"AC1018"},{DRW::AC1021,"AC1021"},
        {DRW::AC1024,"AC1024"},{DRW::AC1027,"AC1027"},{DRW::AC1032,"AC1032"}}};
    for (const auto& version : versions) for (bool inputBinary : {false,true}) {
        const auto source = directory / "resource-source.dxf";
        const bool modern = version.first >= DRW::AC1021;
        if (!write(source,sourceTags(version.second,modern),inputBinary) || !check(source,modern,false)) return fail("literal resource source is invalid");
        dx_data original; dx_iface importer;
        if (!importer.fileImport(source.string(),&original,false)) return fail("literal resource source import failed");
        const auto before = referenceState(original);
        for (bool binary : {false,true}) {
            const auto first = directory / "resource-first.dxf", second = directory / "resource-second.dxf", strict = directory / "resource-strict.dxf";
            dx_iface exporter, reader, replay, secondReader, strictExporter;
            dx_data actual, again;
            if (!exporter.fileExport(first.string(),version.first,binary,&original,false)
                || !check(first,modern,true)) return fail("compatible DIMSTYLE resource map differs from actual emitted rows");
            if (!reader.fileImport(first.string(),&actual,false)
                || !replay.fileExport(second.string(),version.first,!binary,&actual,false)
                || !check(second,modern,true)) return fail("compatible repeated DIMSTYLE remap/readback failed");
            if (!secondReader.fileImport(second.string(),&again,false)
                || !strictExporter.fileExport(strict.string(),version.first,binary,&again,false,true,
                    dx_dxfSourceTextPolicy::RequireValidSource,DRW::DxfTextWritePolicy::RequireLossless,
                    dx_dxfProxyGraphicsPolicy::FailIfNotPreserved,dx_dwgReadCompletenessPolicy::RequireComplete)
                || !check(strict,modern,true)) return fail("strict pass after compatible reexports found dangling resource");
            if (referenceState(original) != before) return fail("compatible export edited source DIMSTYLE references");
            ++routes;
        }
    }
    unsigned refusals = 0u, retries = 0u, compatibleUnknowns = 0u, nullRoutes = 0u;
    const auto source = directory / "resource-source.dxf", output = directory / "resource-fault.dxf";
    dx_data data; dx_iface importer, exporter;
    if (!importer.fileImport(source.string(),&data,false) || data.dimStyles.size() != 1u) return false;
    auto& style = data.dimStyles.front(); const DRW_Dimstyle saved(style);
    const auto bytes = [](const auto& path) { std::ifstream in(path,std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(in),{}); };
    const std::string sentinel = "PRESERVE_DIMSTYLE_RESOURCE_FAILURE\n";
    const auto refuseRetry = [&](bool binary, bool strict) {
        { std::ofstream out(output,std::ios::binary); out << sentinel; }
        const bool refused = !exporter.fileExport(output.string(),DRW::AC1032,binary,&data,false,strict)
            && bytes(output) == sentinel;
        style = saved;
        if (!refused || !exporter.fileExport(output.string(),DRW::AC1032,binary,&data,false,strict)
            || !check(output,true,true)) return false;
        ++refusals; ++retries; return true;
    };
    for (int c = 340; c <= 347; ++c) for (bool wrongRole : {false,true}) for (bool binary : {false,true}) {
        const std::string token = !wrongRole ? "EFFF" : c == 340 ? "A345" : "A340";
        const auto bad = static_cast<std::uint32_t>(std::stoul(token,nullptr,16));
        std::array<dwgHandle*,8> handles{{&style.dimtxstyH,&style.dimldrblkH,&style.dimblkH,&style.dimblk1H,
            &style.dimblk2H,&style.dimltypeH,&style.dimltex1H,&style.dimltex2H}};
        handles[c-340]->code = 5; handles[c-340]->ref = bad; handles[c-340]->ref64 = bad;
        std::array<std::string*,5> names{{&style.dimtxsty,&style.dimldrblk,&style.dimblk,&style.dimblk1,&style.dimblk2}};
        if (c <= 344) *names[c-340] = token;
        for (auto& item : style.vars)
            if (item.second && (item.second->code() == c || (c >= 342 && c <= 344 && item.second->code() == c-337)))
                *item.second = DRW_Variant(item.second->code(),token);
        if (!exporter.fileExport(output.string(),DRW::AC1032,binary,&data,false)
            || !check(output,true,true,false,c,bad)) return fail("compatible unresolved reference was dropped or rebound to wrong resource kind");
        ++compatibleUnknowns;
        if (!refuseRetry(binary,true)) return fail("strict dangling/wrong-role refusal or same-facade recovery failed");
    }
    for (int c = 340; c <= 347; ++c) for (bool binary : {false,true}) {
        style.add("EXTRA_RESOURCE_ONE",c,std::string("A")+std::to_string(c));
        style.add("EXTRA_RESOURCE_TWO",c,std::string("A")+std::to_string(c));
        if (!refuseRetry(binary,false)) return fail("duplicate resource refusal or compatible recovery failed");
    }
    for (bool inputBinary : {false,true}) {
        auto tags = sourceTags("AC1032",true);
        for (auto& tag : tags) if (tag.code >= 341 && tag.code <= 347) tag.value = "0";
        const auto nullSource = directory / "null-source.dxf";
        dx_data nullData; dx_iface nullImporter;
        if (!write(nullSource,tags,inputBinary) || !nullImporter.fileImport(nullSource.string(),&nullData,false)) return false;
        for (bool binary : {false,true}) {
            dx_iface nullExporter;
            if (!nullExporter.fileExport(output.string(),DRW::AC1032,binary,&nullData,false)
                || !check(output,true,true,true)) return fail("explicit null resource presence changed");
            ++nullRoutes;
        }
    }
    std::cout << "DIMSTYLE resources: " << routes << " two-pass compatible routes and strict readbacks; "
              << compatibleUnknowns << " compatible unresolved controls; " << refusals << " atomic refusals/"
              << retries << " corrected readbacks; " << nullRoutes << " explicit-null routes\n";
    return routes == 24u && compatibleUnknowns == 32u && refusals == 48u && retries == 48u && nullRoutes == 4u;
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--resource-remap") {
        const auto directory = std::filesystem::temp_directory_path()
            / ("libdxfrw-dimstyle-resources-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::error_code error;
        if (!std::filesystem::create_directory(directory,error) || error) return 1;
        const bool ok = resourceRemap(directory);
        if (ok) std::filesystem::remove_all(directory,error);
        else std::cerr << "Evidence: " << directory << '\n';
        return ok ? 0 : 1;
    }
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
