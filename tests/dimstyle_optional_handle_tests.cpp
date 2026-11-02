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
                          bool nulls = false, int badCode = 0, std::uint32_t badRef = 0u,
                          const std::string& textTarget = "RESOURCE_TEXT", bool missingText = false,
                          bool literalTextName = false, bool owners = false) {
        std::ifstream stream(path, std::ios::binary); char magic[22]{}; stream.read(magic, 22);
        const bool binary = std::string(magic,22) == std::string("AutoCAD Binary DXF\r\n\x1a\0",22);
        stream.clear(); stream.seekg(binary ? 22 : 0);
        std::unique_ptr<dxfReader> reader = binary ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&stream))
            : std::unique_ptr<dxfReader>(new dxfReaderAscii(&stream));
        reader->setAllowDimstyleNames(literalTextName);
        std::map<std::string,std::uint32_t> targets, bodies;
        std::map<std::string,std::uint32_t> controls, styleOwners;
        std::map<std::string,unsigned> styleCounts;
        std::map<int,std::uint32_t> references;
        std::string type, name; std::uint32_t handle = 0u, owner = 0u;
        std::uint32_t dimensionHandle = 0u, dimensionOwner = 0u;
        const auto finish = [&] {
            if (type == "TABLE") controls[name] = handle;
            else if (type == "STYLE") { styleOwners[name] = owner; ++styleCounts[name]; }
            else if (type == "DIMSTYLE" && name == "RESOURCE_DIMS") { dimensionHandle = handle; dimensionOwner = owner; }
        };
        int code = 0; bool eof = false;
        while (reader->readRec(&code)) {
            if (code == 0) { finish(); type = reader->getString(); name.clear(); handle = owner = 0u; eof = type == "EOF"; }
            else if ((code == 5 || (type == "DIMSTYLE" && code == 105)) && reader->isValidHandleString()) handle = reader->getHandleString();
            else if (code == 330 && reader->isValidHandleString()) owner = reader->getHandleString();
            else if (code == 2) {
                name = reader->getString();
                if (type == "STYLE" || type == "LTYPE" || type == "BLOCK_RECORD") targets[type+":"+name] = handle;
                if (type == "BLOCK") bodies[name] = owner;
            } else if (type == "DIMSTYLE" && name == "RESOURCE_DIMS" && code >= 340 && code <= 347) {
                if (code == 340 && literalTextName) {
                    if (reader->getString() != textTarget || !references.emplace(code,0u).second) return false;
                } else if (!reader->isValidHandleString() || !references.emplace(code,reader->getHandleString()).second) return false;
            }
        }
        if (!eof || references.size() != (modern ? 8u : 5u) - (missingText ? 1u : 0u)
            || (missingText && references.count(340))) return false;
        if (owners && (!dimensionHandle || !controls["DIMSTYLE"] || dimensionOwner != controls["DIMSTYLE"]
            || !controls["STYLE"] || styleCounts[textTarget] != 1u || styleOwners[textTarget] != controls["STYLE"])) return false;
        for (int c = 340; c <= (modern ? 347 : 344); ++c) {
            if (c == 340 && (missingText || literalTextName)) continue;
            if (nulls && c >= 341) { if (references[c] != 0u) return false; continue; }
            if (c == badCode) { if (references[c] != badRef) return false; continue; }
            const auto kind = c == 340 ? "STYLE:"+textTarget : c <= 344 ? "BLOCK_RECORD:ARROW_"+std::to_string(c)
                : "LTYPE:LTYPE_"+std::to_string(c);
            if (!targets.count(kind) || !targets.at(kind) || references[c] != targets.at(kind)
                || (moved && references[c] == (c == 340 && textTarget == "Standard" ? 0xA339u
                    : std::stoul("A"+std::to_string(c),nullptr,16)))) return false;
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
    // The omitted group-340 source becomes a logical default name in vars.
    // Keep that name distinct from explicit zero/hex reference transport.
    const auto namedSource = [&](const std::string& token, const char* revision, bool modern) {
        auto tags = sourceTags(revision,modern);
        std::vector<Tag> result;
        std::string type, table;
        for (auto tag : tags) {
            if (tag.code == 0) type = tag.value;
            if (type == "TABLE" && tag.code == 2) table = tag.value;
            if (type == "ENDTAB" && tag.code == 0 && table == "STYLE") {
                for (const auto& entry : {std::pair<const char*,const char*>{"A337","A9"},
                                         std::pair<const char*,const char*>{"A338","100000000"},
                                         std::pair<const char*,const char*>{"A336","10000000000000000"}})
                    result.insert(result.end(), {{0,"STYLE"},{5,entry.first},{330,"A301"},
                        {100,"AcDbSymbolTableRecord"},{100,"AcDbTextStyleTableRecord"},
                        {2,entry.second},{70,"0"},{3,"txt"}});
            }
            if (type == "DIMSTYLE" && tag.code == 340) {
                if (token.empty()) continue;
                tag.value = token;
            }
            if (type == "TABLE" && table == "STYLE" && tag.code == 70) tag.value = "5";
            result.push_back(tag);
            if (type == "TABLE" && tag.code == 2 && (table == "STYLE" || table == "DIMSTYLE"))
                result.insert(result.end(), {{5,table == "STYLE" ? "A301" : "A302"},
                    {330,"0"},{100,"AcDbSymbolTable"}});
            else if (type == "STYLE" && tag.code == 5) result.push_back({330,"A301"});
            else if (type == "DIMSTYLE" && tag.code == 105) result.push_back({330,"A302"});
        }
        return result;
    };
    const auto textCheck = [&](const auto& path, const std::string& target,
                               bool moved, bool missing = false, bool literal = false,
                               int rawReference = -1) {
        return check(path,true,moved,false,rawReference < 0 ? 0 : 340,
            rawReference < 0 ? 0u : static_cast<std::uint32_t>(rawReference),
            target,missing,literal,true);
    };
    unsigned namedInputs = 0u, defaultRoutes = 0u, namedRoutes = 0u,
        dynamicRoutes = 0u, literalRoutes = 0u, wideRoutes = 0u, nameRefusals = 0u, nameRetries = 0u;
    for (bool inputBinary : {false,true}) for (unsigned mode = 0u; mode != 4u; ++mode) {
        const std::string token = mode == 0u ? "" : mode == 1u ? "RESOURCE_TEXT" : mode == 2u ? "0" : "A9";
        const std::string target = mode == 1u ? "RESOURCE_TEXT" : "Standard";
        const int rawReference = mode == 2u ? 0 : mode == 3u ? 0xA9 : -1;
        const auto input = directory / "named-source.dxf";
        if (!write(input,namedSource(token,"AC1032",true),inputBinary)
            || !textCheck(input,target,false,mode == 0u,mode == 1u,rawReference))
            return fail("literal DIMSTYLE name/default source differs from authored fields or owners");
        const auto inputBefore = bytes(input);
        dx_data namedData; dx_iface namedImporter;
        if (!namedImporter.fileImport(input.string(),&namedData,false) || namedData.dimStyles.size() != 1u)
            return fail("DIMSTYLE name/default source import failed");
        auto& namedStyle = namedData.dimStyles.front();
        DRW_Variant* textVariable = nullptr;
        for (auto& item : namedStyle.vars) if (item.second && item.second->code() == 340) {
            if (textVariable) return fail("DIMSTYLE synthesized duplicate text-style variable");
            textVariable = item.second;
        }
        if (!textVariable || textVariable->type() != DRW_Variant::STRING
            || !textVariable->c_str() || std::string(textVariable->c_str()) != (mode == 0u ? "Standard" : token)
            || namedStyle.dimtxsty != (mode == 0u ? "Standard" : token))
            return fail("DIMSTYLE source/default name was not retained in POD and dynamic field");
        if (mode <= 1u && (namedStyle.dimtxstyH.code != 0u || namedStyle.dimtxstyH.ref != 0u
            || namedStyle.dimtxstyH.ref64 != 0u)) return fail("logical DIMSTYLE name acquired a fabricated source handle");
        if (mode >= 2u && (namedStyle.dimtxstyH.code != 5u
            || namedStyle.dimtxstyH.ref != static_cast<std::uint32_t>(rawReference)))
            return fail("explicit zero/hex DIMSTYLE source handle lost its role");
        const auto before = referenceState(namedData); ++namedInputs;
        for (bool binary : {false,true}) {
            const auto first = directory / "named-first.dxf", second = directory / "named-second.dxf";
            dx_iface namedExporter, namedReader, namedReplay; dx_data namedActual;
            if (!namedExporter.fileExport(first.string(),DRW::AC1032,binary,&namedData,false)
                || !textCheck(first,target,true,false,false,rawReference)
                || !namedReader.fileImport(first.string(),&namedActual,false)
                || !namedReplay.fileExport(second.string(),DRW::AC1032,!binary,&namedActual,false)
                || !textCheck(second,target,true,false,false,rawReference)
                || referenceState(namedData) != before || bytes(input) != inputBefore)
                return fail("DIMSTYLE name/default output, owner, repeated readback or source immutability differs");
            if (mode == 0u) ++defaultRoutes;
            else if (mode == 1u) ++namedRoutes;
            else ++literalRoutes;
            if (mode != 0u) continue;
            // The dynamic request deliberately differs from the POD default,
            // matching the existing strict writer's dynamic-name precedence.
            *textVariable = DRW_Variant(340,std::string("RESOURCE_TEXT"));
            const auto overridden = referenceState(namedData);
            if (!namedExporter.fileExport(first.string(),DRW::AC1032,binary,&namedData,false)
                || !textCheck(first,"RESOURCE_TEXT",true) || namedStyle.dimtxsty != "Standard"
                || referenceState(namedData) != overridden)
                return fail("compatible DIMSTYLE dynamic name did not override the POD name without mutation");
            ++dynamicRoutes;
            *textVariable = DRW_Variant(340,std::string("MISSING_TEXT_STYLE"));
            const auto badBefore = referenceState(namedData);
            { std::ofstream destination(first,std::ios::binary); destination << sentinel; }
            if (namedExporter.fileExport(first.string(),DRW::AC1032,binary,&namedData,false)
                || bytes(first) != sentinel || referenceState(namedData) != badBefore)
                return fail("missing DIMSTYLE name changed destination/source or succeeded");
            ++nameRefusals;
            *textVariable = DRW_Variant(340,std::string("Standard"));
            if (!namedExporter.fileExport(first.string(),DRW::AC1032,binary,&namedData,false)
                || !textCheck(first,"Standard",true) || referenceState(namedData) != before)
                return fail("same-facade DIMSTYLE name/default corrected retry failed");
            ++nameRetries;
            for (const std::string& hex : {std::string("100000000"),std::string("10000000000000000")}) {
                *textVariable = DRW_Variant(340,hex);
                const auto hexBefore = referenceState(namedData);
                if (!namedExporter.fileExport(first.string(),DRW::AC1032,binary,&namedData,false)
                    || !textCheck(first,hex,true,false,true)
                    || referenceState(namedData) != hexBefore)
                    return fail("over-wide hexadecimal token was not retained as a literal");
                ++wideRoutes;
            }
            *textVariable = DRW_Variant(340,std::string("Standard"));
        }
    }
    unsigned earlierRoutes = 0u;
    for (bool inputBinary : {false,true}) {
        const auto input = directory / "earlier-default-source.dxf";
        if (!write(input,namedSource("","AC1015",false),inputBinary)
            || !check(input,false,false,false,0,0,"Standard",true,false,true))
            return fail("earlier DIMSTYLE source did not omit group 340");
        dx_data earlier; dx_iface reader;
        if (!reader.fileImport(input.string(),&earlier,false))
            return fail("earlier DIMSTYLE default import failed");
        const auto before = referenceState(earlier);
        for (bool binary : {false,true}) {
            const auto first = directory / "earlier-default-first.dxf";
            const auto second = directory / "earlier-default-second.dxf";
            dx_iface writer, readback, replay; dx_data actual;
            if (!writer.fileExport(first.string(),DRW::AC1015,binary,&earlier,false)
                || !check(first,false,true,false,0,0,"Standard",false,false,true)
                || !readback.fileImport(first.string(),&actual,false)
                || !replay.fileExport(second.string(),DRW::AC1015,!binary,&actual,false)
                || !check(second,false,true,false,0,0,"Standard",false,false,true)
                || referenceState(earlier) != before)
                return fail("earlier DIMSTYLE default output or readback differs");
            ++earlierRoutes;
        }
    }
    std::cout << "DIMSTYLE resources: " << routes << " two-pass compatible routes and strict readbacks; "
              << compatibleUnknowns << " compatible unresolved controls; " << refusals << " atomic refusals/"
              << retries << " corrected readbacks; " << nullRoutes << " explicit-null routes; "
              << namedInputs << " named/default/zero/hex sources; " << defaultRoutes << " default routes; "
              << namedRoutes << " explicit-name routes; " << dynamicRoutes << " dynamic-name overrides; "
              << literalRoutes << " zero/hex routes; " << wideRoutes << " over-wide literal routes; "
              << nameRefusals << " named refusals/" << nameRetries << " retries; "
              << earlierRoutes << " earlier-version default routes\n";
    return routes == 24u && compatibleUnknowns == 32u && refusals == 48u && retries == 48u && nullRoutes == 4u
        && namedInputs == 8u && defaultRoutes == 4u && namedRoutes == 4u && dynamicRoutes == 4u
        && literalRoutes == 8u && wideRoutes == 8u && nameRefusals == 4u && nameRetries == 4u
        && earlierRoutes == 4u;
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
