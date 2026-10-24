#include <chrono>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "drw_entities.h"
#include "dwg2dxf/dx_iface.h"
#include "intern/dxfreader.h"

namespace {

using Tag = std::pair<int, std::string>;

struct Record {
    std::string name;
    std::string handle;
    std::vector<Tag> tags;
};

struct Case {
    DRW::ETYPE type;
    std::string payload;
    bool explicitChunks = false;
    bool shellFields = false;
    bool history = false;
};

bool parseHexHandle(const std::string& value, std::uint32_t& handle) {
    const char* const end = value.data() + value.size();
    const auto parsed = std::from_chars(value.data(), end, handle, 16);
    return parsed.ec == std::errc{} && parsed.ptr == end && handle != 0;
}

std::vector<Tag> satTextGroups(const std::string& payload) {
    std::vector<Tag> tags;
    int groupCode = 1;
    std::string chunk;
    const auto flush = [&]() {
        if (chunk.empty())
            return;
        tags.emplace_back(groupCode, std::move(chunk));
        groupCode = 3;
        chunk.clear();
    };
    for (const char value : payload) {
        const std::string_view atom = value == '^'
            ? std::string_view("^ ") : std::string_view(&value, 1u);
        if (chunk.size() + atom.size() > 255u)
            flush();
        chunk.append(atom.data(), atom.size());
    }
    flush();
    return tags;
}

const char* recordName(DRW::ETYPE type) {
    if (type == DRW::E3DSOLID) return "3DSOLID";
    if (type == DRW::REGION) return "REGION";
    return "BODY";
}

bool readRecords(const std::filesystem::path& path, std::vector<Record>& records,
                 bool& sawLine, bool& lastEntityWasLine) {
    std::ifstream input(path, std::ios::binary);
    char signature[22] = {};
    input.read(signature, sizeof(signature));
    const bool binary = std::string(signature, sizeof(signature))
        == std::string("AutoCAD Binary DXF\r\n\x1a\0", 22);
    input.clear();
    input.seekg(binary ? 22 : 0);
    std::unique_ptr<dxfReader> reader;
    if (binary) reader.reset(new dxfReaderBinary(&input));
    else reader.reset(new dxfReaderAscii(&input));
    Record* active = nullptr;
    int code = 0;
    while (reader->readRec(&code)) {
        if (code == 0) {
            const std::string name = reader->getString();
            active = nullptr;
            if (name == "EOF") return true;
            if (name == "LINE" || name == "3DSOLID" || name == "REGION"
                || name == "BODY")
                lastEntityWasLine = name == "LINE";
            if (name == "LINE")
                sawLine = true;
            if (name == "3DSOLID" || name == "REGION" || name == "BODY") {
                records.push_back({name, {}, {}});
                active = &records.back();
            }
        } else if (active != nullptr) {
            if (code == 5) active->handle = reader->getString();
            else if (code == 70 || code == 290)
                active->tags.emplace_back(code, std::to_string(reader->getInt32()));
            else if (code == 100 || code == 1 || code == 3 || code == 2 || code == 350)
                active->tags.emplace_back(code, reader->getString());
        }
    }
    return false;
}

bool verify(const std::filesystem::path& path, DRW::Version version,
            const std::vector<Case>& cases, bool expectFollowingLine = false,
            bool requireSourceHandleRemap = false) {
    std::vector<Record> records;
    bool sawLine = false;
    bool lastEntityWasLine = false;
    if (!readRecords(path, records, sawLine, lastEntityWasLine)
        || records.size() != cases.size()
        || (expectFollowingLine && (!sawLine || !lastEntityWasLine)))
        return false;
    for (std::size_t i = 0; i < cases.size(); ++i) {
        const Case& test = cases[i];
        std::uint32_t emittedHandle = 0;
        if (!parseHexHandle(records[i].handle, emittedHandle)
            || (requireSourceHandleRemap
                && emittedHandle == 0xFC00u + static_cast<std::uint32_t>(i))) {
            std::cerr << "Modeler source-handle remap mismatch: " << path
                      << " record " << i << " emitted=" << records[i].handle
                      << '\n';
            return false;
        }
        std::vector<Tag> expected {{100, "AcDbEntity"}, {100, "AcDbModelerGeometry"}};
        if (version < DRW::AC1027) {
            expected.emplace_back(70, "1");
            const auto payloadTags = satTextGroups(test.payload);
            expected.insert(expected.end(), payloadTags.begin(), payloadTags.end());
        } else if (test.shellFields) {
            expected.emplace_back(290, "1");
            expected.emplace_back(2, "{00000000-0000-0000-0000-000000000001}");
        }
        if (test.type == DRW::E3DSOLID && version >= DRW::AC1021) {
            expected.emplace_back(100, "AcDb3dSolid");
            expected.emplace_back(350, test.history ? records.front().handle : "0");
        }
        if (records[i].name != recordName(test.type) || records[i].handle.empty()
            || records[i].handle == "0" || records[i].tags != expected) {
            std::cerr << "Modeler ordered-tag mismatch: " << path << " record " << i
                      << ' ' << recordName(test.type) << " payload=" << test.payload.size() << '\n';
            return false;
        }
    }
    return true;
}

bool hasExpectedFollowingLine(const dx_data& data) {
    if (data.mBlock == nullptr)
        return false;
    std::size_t lineCount = 0u;
    for (const DRW_Entity* entity : data.mBlock->ent) {
        if (entity == nullptr || entity->eType != DRW::LINE)
            continue;
        const auto* line = dynamic_cast<const DRW_Line*>(entity);
        if (line == nullptr
            || line->basePoint.x != 12.0 || line->basePoint.y != 13.0
            || line->basePoint.z != 14.0 || line->secPoint.x != 15.0
            || line->secPoint.y != 16.0 || line->secPoint.z != 17.0)
            return false;
        ++lineCount;
    }
    return lineCount == 1u;
}

bool runNonCanonicalModelerFieldOrder(const std::filesystem::path& directory) {
    const auto path = directory / "noncanonical-modeler-field-order.dxf";
    const std::string source =
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1027\n"
        "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
        "0\n3DSOLID\n5\n20\n160\n4\n310\n11223344\n"
        "100\nAcDbEntity\n8\n0\n100\nAcDbModelerGeometry\n"
        // Payload remains ordered, but its modeler version follows it.
        "1\nM\n3\nN\n70\n1\n"
        "100\nAcDb3dSolid\n350\n2A\n"
        "0\nLINE\n5\n21\n100\nAcDbEntity\n8\n0\n"
        "100\nAcDbLine\n10\n12\n20\n13\n30\n14\n"
        "11\n15\n21\n16\n31\n17\n"
        "0\nENDSEC\n0\nEOF\n";
    {
        std::ofstream output(path, std::ios::binary);
        output.write(source.data(), static_cast<std::streamsize>(source.size()));
        if (!output.good())
            return false;
    }

    dx_data imported;
    dx_iface importer;
    const bool readSucceeded = importer.fileImport(path.string(), &imported, false);
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    if (!readSucceeded || imported.mBlock == nullptr
        || imported.mBlock->ent.size() != 2u)
        return false;

    const DRW_ModelerGeometry* modeler = nullptr;
    const DRW_Line* line = nullptr;
    for (const DRW_Entity* entity : imported.mBlock->ent) {
        if (entity == nullptr)
            return false;
        if (entity->eType == DRW::E3DSOLID)
            modeler = dynamic_cast<const DRW_ModelerGeometry*>(entity);
        else if (entity->eType == DRW::LINE)
            line = dynamic_cast<const DRW_Line*>(entity);
    }
    if (modeler == nullptr || line == nullptr
        || modeler->m_modelerVersion != 1u
        || modeler->m_rawBytes != std::vector<std::uint8_t>{'M', 'N', '\n'}
        || modeler->m_dxfPayloadChunks.size() != 2u
        || modeler->m_dxfPayloadChunks[0].m_groupCode != 1
        || modeler->m_dxfPayloadChunks[0].m_offset != 0u
        || modeler->m_dxfPayloadChunks[0].m_length != 1u
        || modeler->m_dxfPayloadChunks[1].m_groupCode != 3
        || modeler->m_dxfPayloadChunks[1].m_offset != 1u
        || modeler->m_dxfPayloadChunks[1].m_length != 1u
        || modeler->numProxyGraph != 4
        || modeler->proxyGraphics != std::string("\x11\x22\x33\x44", 4u)
        || modeler->m_historyHandle != 0x2Au
        || line->basePoint.x != 12.0 || line->basePoint.y != 13.0
        || line->basePoint.z != 14.0 || line->secPoint.x != 15.0
        || line->secPoint.y != 16.0 || line->secPoint.z != 17.0) {
        std::cerr << "Noncanonical modeler field-order readback mismatch\n";
        return false;
    }

    const auto malformedPath = directory / "malformed-modeler-chunk.dxf";
    const std::string malformed =
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1027\n"
        "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
        "0\n3DSOLID\n5\n22\n100\nAcDbEntity\n"
        "100\nAcDbModelerGeometry\n70\n1\n310\n0\n"
        "0\nLINE\n5\n23\n100\nAcDbEntity\n8\n0\n"
        "100\nAcDbLine\n10\n1\n20\n2\n30\n3\n"
        "11\n4\n21\n5\n31\n6\n"
        "0\nENDSEC\n0\nEOF\n";
    {
        std::ofstream output(malformedPath, std::ios::binary);
        output.write(malformed.data(),
                     static_cast<std::streamsize>(malformed.size()));
        if (!output.good())
            return false;
    }
    dx_data rejected;
    dx_iface rejectingImporter;
    const bool malformedAccepted = rejectingImporter.fileImport(
        malformedPath.string(), &rejected, false);
    std::filesystem::remove(malformedPath, ignored);
    if (malformedAccepted || rejected.mBlock == nullptr
        || !rejected.mBlock->ent.empty()) {
        std::cerr << "Malformed modeler chunk was not rejected transactionally\n";
        return false;
    }
    return true;
}

bool rejectUnrepresentableSatText(const std::filesystem::path& directory,
                                  DRW::Version version);

bool runVersion(const std::filesystem::path& directory, DRW::Version version, bool binary) {
    const auto path = directory / (std::to_string(static_cast<int>(version))
        + (binary ? "-binary.dxf" : "-ascii.dxf"));
    std::vector<Case> cases;
    const DRW::ETYPE types[] = {DRW::E3DSOLID, DRW::REGION, DRW::BODY};
    for (DRW::ETYPE type : types) {
        if (version >= DRW::AC1027) {
            cases.push_back({type, {}, false, false, false});
            cases.push_back({type, {}, false, true, false});
        } else {
            for (std::size_t length : {254u, 255u, 256u, 509u, 510u, 511u}) {
                for (std::size_t cut : {0u, 253u, 254u, 255u}) {
                    std::string payload(length, 'A');
                    if (cut != 0 && cut < length) payload[cut] = ' ';
                    for (bool chunks : {false, true})
                        cases.push_back({type, payload, chunks, false, false});
                }
            }
        }
    }
    if (version < DRW::AC1027) {
        std::string caretAtBoundary(253u, 'A');
        caretAtBoundary += "^ BC";
        std::string caretAtSplit(254u, 'A');
        caretAtSplit += "^ B";
        cases.push_back({DRW::E3DSOLID, caretAtBoundary, false, false, false});
        cases.push_back({DRW::REGION, caretAtBoundary, true, false, false});
        cases.push_back({DRW::BODY, caretAtSplit, false, false, false});
        if (binary)
            cases.push_back({DRW::REGION, "SAT line one\nSAT ^ line two\r\n",
                             false, false, false});
    }
    if (version >= DRW::AC1021)
        cases.push_back({DRW::E3DSOLID, {}, false, false, true});

    dx_data source;
    for (std::size_t i = 0; i < cases.size(); ++i) {
        const Case& test = cases[i];
        auto* entity = new DRW_ModelerGeometry(test.type);
        entity->handle = 0xFC00u + static_cast<std::uint32_t>(i);
        entity->m_modelerVersion = 1;
        entity->m_rawBytes.assign(test.payload.begin(), test.payload.end());
        if (test.explicitChunks)
            entity->m_dxfPayloadChunks.push_back({1, 0, test.payload.size()});
        if (test.shellFields) {
            entity->m_hasDxfModelerFlag = true;
            entity->m_dxfModelerFlag = true;
            entity->m_hasDxfModelerUid = true;
            entity->m_dxfModelerUid = "{00000000-0000-0000-0000-000000000001}";
        }
        if (test.history) entity->m_historyHandle = 0xFC00u;
        source.mBlock->ent.push_back(entity);
    }
    auto* followingLine = new DRW_Line();
    followingLine->handle = 0xFD00u;
    followingLine->basePoint = DRW_Coord(12.0, 13.0, 14.0);
    followingLine->secPoint = DRW_Coord(15.0, 16.0, 17.0);
    source.mBlock->ent.push_back(followingLine);
    dx_iface exporter;
    if (!exporter.fileExport(path.string(), version, binary, &source, false)) {
        std::cerr << "Modeler first export failed: version="
                  << static_cast<int>(version) << " binary=" << binary << '\n';
        return false;
    }
    if (!verify(path, version, cases, true, true)) {
        std::cerr << "Modeler first verification failed: version="
                  << static_cast<int>(version) << " binary=" << binary << '\n';
        return false;
    }
    std::vector<Record> outputRecords;
    bool outputSawLine = false;
    bool outputLastEntityWasLine = false;
    if (!readRecords(path, outputRecords, outputSawLine,
                     outputLastEntityWasLine)
        || outputRecords.size() != cases.size())
        return false;
    dx_data imported;
    dx_iface importer;
    const auto second = directory / (path.stem().string() + "-second.dxf");
    if (!importer.fileImport(path.string(), &imported, false)
        || imported.mBlock == nullptr
        || !hasExpectedFollowingLine(imported)) {
        std::cerr << "Modeler import failed: version="
                  << static_cast<int>(version) << " binary=" << binary << '\n';
        return false;
    }
    std::size_t modelerIndex = 0;
    std::uint32_t importedHistoryTargetHandle = 0;
    for (const DRW_Entity* entity : imported.mBlock->ent) {
        if (entity == nullptr
            || (entity->eType != DRW::E3DSOLID
                && entity->eType != DRW::REGION
                && entity->eType != DRW::BODY))
            continue;
        if (modelerIndex >= cases.size()
            || entity->eType != cases[modelerIndex].type)
            return false;
        const auto* modeler = dynamic_cast<const DRW_ModelerGeometry*>(entity);
        std::uint32_t emittedHandle = 0;
        if (!parseHexHandle(outputRecords[modelerIndex].handle, emittedHandle))
            return false;
        if (modelerIndex == 0 && modeler != nullptr)
            importedHistoryTargetHandle = modeler->handle;
        const std::uint32_t expectedHistoryHandle = cases[modelerIndex].history
            ? importedHistoryTargetHandle : 0u;
        const std::vector<std::uint8_t> expectedPayload(
            cases[modelerIndex].payload.begin(), cases[modelerIndex].payload.end());
        std::vector<std::uint8_t> expectedRawBytes = expectedPayload;
        if (!expectedPayload.empty())
            expectedRawBytes.push_back('\n');
        std::vector<std::uint8_t> chunkPayload;
        bool validChunks = modeler != nullptr;
        if (modeler != nullptr) {
            for (const DRW_ModelerPayloadChunk& chunk : modeler->m_dxfPayloadChunks) {
                if ((chunk.m_groupCode != 1 && chunk.m_groupCode != 3)
                    || chunk.m_length > 255u
                    || chunk.m_offset > modeler->m_rawBytes.size()
                    || chunk.m_length > modeler->m_rawBytes.size() - chunk.m_offset) {
                    validChunks = false;
                    break;
                }
                chunkPayload.insert(
                    chunkPayload.end(),
                    modeler->m_rawBytes.begin()
                        + static_cast<std::ptrdiff_t>(chunk.m_offset),
                    modeler->m_rawBytes.begin()
                        + static_cast<std::ptrdiff_t>(chunk.m_offset + chunk.m_length));
            }
        }
        if (modeler == nullptr || entity->handle != emittedHandle
            || modeler->m_historyHandle != expectedHistoryHandle
            || modeler->m_rawBytes != expectedRawBytes
            || !validChunks || chunkPayload != expectedPayload) {
            std::cerr << "Modeler payload readback mismatch: version="
                      << static_cast<int>(version) << " binary=" << binary
                      << " index=" << modelerIndex << " expected="
                      << expectedRawBytes.size() << " actual="
                      << (modeler == nullptr ? 0u : modeler->m_rawBytes.size())
                      << " expected-history=" << expectedHistoryHandle
                      << " actual-history="
                      << (modeler == nullptr ? 0u : modeler->m_historyHandle)
                      << '\n';
            return false;
        }
        ++modelerIndex;
    }
    if (modelerIndex != cases.size()) {
        std::cerr << "Modeler count mismatch: version="
                  << static_cast<int>(version) << " binary=" << binary
                  << " expected=" << cases.size() << " actual="
                  << modelerIndex << '\n';
        return false;
    }
    if (!importer.fileExport(second.string(), version, binary, &imported, false)
        || !verify(second, version, cases, true)) {
        std::cerr << "Modeler re-export verification failed: version="
                  << static_cast<int>(version) << " binary=" << binary << '\n';
        return false;
    }
    dx_data secondReadback;
    dx_iface secondImporter;
    if (!secondImporter.fileImport(second.string(), &secondReadback, false)
        || !hasExpectedFollowingLine(secondReadback)) {
        std::cerr << "Modeler following LINE semantic readback failed: version="
                  << static_cast<int>(version) << " binary=" << binary << '\n';
        return false;
    }
    if (!binary && version < DRW::AC1027
        && !rejectUnrepresentableSatText(directory, version)) {
        std::cerr << "Modeler ASCII line-break refusal failed: version="
                  << static_cast<int>(version) << '\n';
        return false;
    }
    return true;
}

bool rejectUnrepresentableSatText(const std::filesystem::path& directory,
                                  DRW::Version version) {
    const auto path = directory / ("rejected-sat-newline-"
        + std::to_string(static_cast<int>(version)) + ".dxf");
    const std::string sentinel = "existing destination\n";
    {
        std::ofstream output(path, std::ios::binary);
        output << sentinel;
    }
    dx_data source;
    auto* entity = new DRW_ModelerGeometry(DRW::REGION);
    entity->handle = 0xFC00u;
    entity->m_modelerVersion = 1;
    const std::string payload = "SAT line one\nSAT line two";
    entity->m_rawBytes.assign(payload.begin(), payload.end());
    source.mBlock->ent.push_back(entity);
    dx_iface exporter;
    const bool rejected = !exporter.fileExport(path.string(), version, false,
                                                &source, false);
    std::ifstream input(path, std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(input)), {});
    if (!rejected || bytes != sentinel) {
        std::cerr << "Modeler ASCII line-break refusal details: rejected="
                  << rejected << " sentinel-preserved=" << (bytes == sentinel)
                  << '\n';
        return false;
    }
    return true;
}

bool rejectUnsupportedHistory(const std::filesystem::path& directory,
                              DRW::Version version, DRW::ETYPE type, bool binary) {
    if (type == DRW::E3DSOLID && version >= DRW::AC1021) return true;
    const auto path = directory / ("rejected-history-" + std::to_string(static_cast<int>(version))
        + '-' + recordName(type) + (binary ? "-binary.dxf" : "-ascii.dxf"));
    const std::string sentinel = "existing destination\n";
    { std::ofstream output(path, std::ios::binary); output << sentinel; }
    dx_data source;
    auto* target = new DRW_ModelerGeometry(DRW::E3DSOLID);
    target->handle = 0xFC00u;
    target->m_modelerVersion = 1;
    source.mBlock->ent.push_back(target);
    auto* entity = new DRW_ModelerGeometry(type);
    entity->handle = 0xFC01u;
    entity->m_modelerVersion = 1;
    entity->m_historyHandle = target->handle;
    source.mBlock->ent.push_back(entity);
    dx_iface exporter;
    const bool rejected = !exporter.fileExport(path.string(), version, binary, &source, false);
    std::ifstream input(path, std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(input)), {});
    input.close();
    if (!rejected || bytes != sentinel) return false;
    entity->m_historyHandle = 0;
    return exporter.fileExport(path.string(), version, binary, &source, false)
        && verify(path, version, {{DRW::E3DSOLID, {}, false, false, false},
                                  {type, {}, false, false, false}});
}

} // namespace

bool runModelerDxfWriterMatrix(const std::filesystem::path& parent, bool keepOutputs) {
    const auto directory = parent / ("libdxfrw-modeler-matrix-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    bool passed = true;
    for (DRW::Version version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                                DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        for (bool binary : {false, true}) {
            passed = runVersion(directory, version, binary) && passed;
            for (DRW::ETYPE type : {DRW::E3DSOLID, DRW::REGION, DRW::BODY})
                passed = rejectUnsupportedHistory(directory, version, type, binary) && passed;
        }
    }
    passed = runNonCanonicalModelerFieldOrder(directory) && passed;
    std::cout << "Modeler DXF writer matrix: " << (passed ? "PASS" : "FAIL") << '\n';
    if (passed && !keepOutputs) std::filesystem::remove_all(directory);
    else std::cout << "Modeler matrix evidence: " << directory << '\n';
    return passed;
}
