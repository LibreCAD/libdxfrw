#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
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

const char* recordName(DRW::ETYPE type) {
    if (type == DRW::E3DSOLID) return "3DSOLID";
    if (type == DRW::REGION) return "REGION";
    return "BODY";
}

bool readRecords(const std::filesystem::path& path, std::vector<Record>& records) {
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
            const std::vector<Case>& cases) {
    std::vector<Record> records;
    if (!readRecords(path, records) || records.size() != cases.size()) return false;
    for (std::size_t i = 0; i < cases.size(); ++i) {
        const Case& test = cases[i];
        std::vector<Tag> expected {{100, "AcDbEntity"}, {100, "AcDbModelerGeometry"}};
        if (version < DRW::AC1027) {
            expected.emplace_back(70, "1");
            for (std::size_t offset = 0; offset < test.payload.size(); offset += 255)
                expected.emplace_back(offset == 0 ? 1 : 3, test.payload.substr(offset, 255));
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
    dx_iface exporter;
    if (!exporter.fileExport(path.string(), version, binary, &source, false)
        || !verify(path, version, cases)) return false;
    dx_data imported;
    dx_iface importer;
    const auto second = directory / (path.stem().string() + "-second.dxf");
    return importer.fileImport(path.string(), &imported, false)
        && importer.fileExport(second.string(), version, binary, &imported, false)
        && verify(second, version, cases);
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
                                DRW::AC1024, DRW::AC1027}) {
        for (bool binary : {false, true}) {
            passed = runVersion(directory, version, binary) && passed;
            for (DRW::ETYPE type : {DRW::E3DSOLID, DRW::REGION, DRW::BODY})
                passed = rejectUnsupportedHistory(directory, version, type, binary) && passed;
        }
    }
    std::cout << "Modeler DXF writer matrix: " << (passed ? "PASS" : "FAIL") << '\n';
    if (passed && !keepOutputs) std::filesystem::remove_all(directory);
    else std::cout << "Modeler matrix evidence: " << directory << '\n';
    return passed;
}
