#include "dx_iface.h"
#include "intern/dxfwriter.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <iterator>
#include <vector>

namespace {

constexpr const char* kImagePath = "assets/adapter-image-control.png";

bool fail(const std::string& message) {
    std::cerr << message << '\n';
    return false;
}

bool generate(const std::string& path) {
    dx_data data;
    dx_iface iface;
    iface.cData = &data;
    iface.currentBlock = data.mBlock;

    DRW_Image image;
    image.ref = 0xD700u;
    image.basePoint = DRW_Coord(1.0, 2.0, 3.0);
    image.secPoint = DRW_Coord(5.0, 2.0, 3.0);
    image.vVector = DRW_Coord(0.0, 3.0, 0.0);
    image.sizeu = 640.0;
    image.sizev = 480.0;
    image.clip = 1;
    image.brightness = 60;
    image.contrast = 70;
    image.fade = 10;
    image.m_classVersion = 2;
    iface.addImage(&image);

    DRW_ImageDef definition;
    definition.handle = image.ref;
    definition.name = kImagePath;
    iface.linkImage(&definition);

    if (data.mBlock->ent.size() != 1)
        return fail("generated image entity count is not one");
    auto* stored = dynamic_cast<dx_ifaceImg*>(data.mBlock->ent.front());
    if (stored == nullptr || stored->path != kImagePath)
        return fail("image-definition callback did not update stored entity");
    if (!iface.fileExport(path, DRW::AC1027, false, &data, false))
        return fail("could not export generated image-path DXF control");
    return true;
}

bool verify(const std::string& path) {
    dx_data data;
    dx_iface iface;
    if (!iface.fileImport(path, &data, false))
        return fail("could not read image-path DXF control");
    if (data.mBlock->ent.size() != 1)
        return fail("read-back image entity count is not one");
    auto* stored = dynamic_cast<dx_ifaceImg*>(data.mBlock->ent.front());
    if (stored == nullptr)
        return fail("read-back image entity lost its adapter type");
    if (stored->path != kImagePath) {
        std::cerr << "image path mismatch: expected '" << kImagePath
                  << "', got '" << stored->path << "'\n";
        return false;
    }
    return true;
}

bool generate3DLine(const std::string& path, bool binary) {
    dx_data data;
    dx_iface iface;
    iface.cData = &data;
    iface.currentBlock = data.mBlock;

    DRW_3DLine line;
    line.basePoint = DRW_Coord(1.25, -2.5, 3.75);
    line.secPoint = DRW_Coord(-4.5, 5.25, -6.75);
    line.extPoint = DRW_Coord(0.0, 1.0, 0.0);
    line.thickness = 2.5;
    iface.add3DLine(line);

    if (data.mBlock->ent.size() != 1
        || dynamic_cast<DRW_3DLine*>(data.mBlock->ent.front()) == nullptr)
        return fail("generated 3DLINE lost its adapter subtype");
    if (!iface.fileExport(path, DRW::AC1027, binary, &data, false))
        return fail("could not export generated 3DLINE control");
    return true;
}

bool verify3DLine(const std::string& path) {
    dx_data data;
    dx_iface iface;
    if (!iface.fileImport(path, &data, false))
        return fail("could not read 3DLINE control");
    if (data.mBlock->ent.size() != 1)
        return fail("read-back 3DLINE entity count is not one");
    auto* line = dynamic_cast<DRW_3DLine*>(data.mBlock->ent.front());
    if (line == nullptr || line->eType != DRW::THREEDLINE)
        return fail("read-back entity did not retain its 3DLINE subtype");
    if (line->basePoint.x != 1.25 || line->basePoint.y != -2.5
        || line->basePoint.z != 3.75 || line->secPoint.x != -4.5
        || line->secPoint.y != 5.25 || line->secPoint.z != -6.75
        || line->thickness != 2.5 || line->extPoint.x != 0.0
        || line->extPoint.y != 1.0 || line->extPoint.z != 0.0)
        return fail("read-back 3DLINE fields changed");
    return true;
}

bool generateMaintenanceHeader(const std::string& path, bool binary) {
    // From-scratch wire control: bypass the HEADER parser/writer so a paired
    // bug cannot turn the large source value into a self-consistent zero.
    std::ofstream output(path, std::ios::binary);
    if (!output)
        return fail("could not create maintenance-header control");
    std::unique_ptr<dxfWriter> writer;
    if (binary) {
        output.write("AutoCAD Binary DXF\r\n\x1A\0", 22);
        writer = std::make_unique<dxfWriterBinary>(&output);
    } else {
        writer = std::make_unique<dxfWriterAscii>(&output);
    }
    writer->setVersion("AC1032", true);
    writer->writeString(0, "SECTION");
    writer->writeString(2, "HEADER");
    writer->writeString(9, "$ACADVER");
    writer->writeString(1, "AC1032");
    writer->writeString(9, "$ACADMAINTVER");
    writer->writeInt32(90, 70001);
    writer->writeString(0, "ENDSEC");
    writer->writeString(0, "SECTION");
    writer->writeString(2, "ENTITIES");
    writer->writeString(0, "LINE");
    writer->writeString(5, "A0");
    writer->writeString(100, "AcDbEntity");
    writer->writeString(8, "0");
    writer->writeString(100, "AcDbLine");
    writer->writeDouble(10, 1.25);
    writer->writeDouble(20, -2.5);
    writer->writeDouble(30, 3.75);
    writer->writeDouble(11, -4.5);
    writer->writeDouble(21, 5.25);
    writer->writeDouble(31, -6.75);
    writer->writeString(0, "ENDSEC");
    writer->writeString(0, "EOF");
    return !writer->hasWriteError() && output.good();
}

bool verifyMaintenanceHeader(const std::string& path) {
    dx_data data;
    dx_iface iface;
    if (!iface.fileImport(path, &data, false))
        return fail("could not read maintenance-header control");
    const auto found = data.headerC.vars.find("$ACADMAINTVER");
    if (found == data.headerC.vars.end() || found->second == nullptr
        || found->second->type() != DRW_Variant::INTEGER
        || found->second->code() != 90 || found->second->i_val() != 70001
        || data.sourceVersion != DRW::AC1032)
        return fail("AC1032 maintenance integer or declared version changed");
    if (data.mBlock->ent.size() != 1)
        return fail("maintenance-header LINE count changed");
    const auto* line = dynamic_cast<const DRW_Line*>(data.mBlock->ent.front());
    if (line == nullptr || line->basePoint.x != 1.25
        || line->basePoint.y != -2.5 || line->basePoint.z != 3.75
        || line->secPoint.x != -4.5 || line->secPoint.y != 5.25
        || line->secPoint.z != -6.75)
        return fail("maintenance-header LINE coordinates changed");
    return true;
}

bool rejectMaintenanceDowngrade(const std::string& path) {
    dx_data data;
    dx_iface iface;
    data.headerC.addInt("$ACADMAINTVER", 70001, 90);
    if (iface.fileExport(path, DRW::AC1027, false, &data, false)
        || std::filesystem::exists(path))
        return fail("unrepresentable maintenance downgrade published output");
    return true;
}

bool reject3DLineBeforeAC1015(const std::string& path) {
    dx_data data;
    dx_iface iface;
    iface.cData = &data;
    iface.currentBlock = data.mBlock;

    DRW_3DLine line;
    line.basePoint = DRW_Coord(1.0, 2.0, 3.0);
    line.secPoint = DRW_Coord(4.0, 5.0, 6.0);
    iface.add3DLine(line);

    if (iface.fileExport(path, DRW::AC1014, false, &data, false))
        return fail("3DLINE export unexpectedly accepted target AC1014");
    std::ifstream output(path, std::ios::binary);
    if (output.good())
        return fail("unsupported AC1014 3DLINE export published a file");
    return true;
}

bool rejectDynamicBlockWithoutSerializer() {
    dx_data data;
    dx_iface iface;
    iface.cData = &data;

    DRW_DynamicBlockObject object("BLOCKVISIBILITYPARAMETER");
    object.handle = 0xD701u;
    iface.addDynamicBlockObject(object);
    if (data.dynamicBlockObjects.size() != 1u
        || data.dynamicBlockObjects.front().handle != object.handle
        || data.dynamicBlockObjects.front().m_recordName != object.m_recordName) {
        return fail("dynamic-block callback was not retained by the adapter");
    }

    const auto nonce = std::chrono::steady_clock::now()
                           .time_since_epoch()
                           .count();
    const std::filesystem::path output =
        std::filesystem::temp_directory_path()
        / ("libdxfrw-dynamic-block-guard-" + std::to_string(nonce)
           + ".dxf");
    if (std::filesystem::exists(output))
        return fail("dynamic-block guard test output path unexpectedly exists");
    if (iface.fileExport(output.string(), DRW::AC1027, false, &data, true))
        return fail("dynamic-block export unexpectedly passed without a serializer");
    if (std::filesystem::exists(output)) {
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
        return fail("rejected dynamic-block export published a DXF");
    }
    return true;
}

void populateExtensionChain(dx_data& data, std::size_t count, bool wide = false) {
    DRW_LType lineType;
    lineType.name = "ByLayer";
    lineType.handle = 0x101u;
    lineType.xDictHandle = 0x201u;
    data.lineTypes.push_back(lineType);
    for (std::size_t i = 0; i < count; ++i) {
        DRW_Dictionary dictionary;
        dictionary.handle = static_cast<std::uint32_t>(0x201u + i);
        dictionary.parentHandle = i == 0 ? 0x101u
            : (wide ? 0x201u : dictionary.handle - 1u);
        if (wide && i == 0) {
            for (std::size_t j = 1; j < count; ++j)
                dictionary.m_entries.push_back({"LEAF_" + std::to_string(j),
                    static_cast<std::uint32_t>(0x201u + j)});
        } else if (!wide && i + 1u < count) {
            dictionary.m_entries.push_back({"NEXT", dictionary.handle + 1u});
        }
        data.dictionaries.push_back(dictionary);
    }
}

bool extensionGraphLimits() {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-extension-limits-" + std::to_string(nonce));
    if (!std::filesystem::create_directory(directory))
        return fail("could not create isolated extension-limit directory");
    struct Cleanup {
        std::filesystem::path directory;
        ~Cleanup() {
            std::error_code error;
            std::filesystem::remove_all(directory, error);
        }
    } cleanup{directory};
    bool passed = true;
    const auto expect = [&passed](bool condition, const std::string& message) {
        if (!condition) {
            fail(message);
            passed = false;
        }
    };
    const auto contents = [](const std::filesystem::path& path) {
        std::ifstream input(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input), {});
    };
    using Failure = dx_iface::ExtensionGraphFailure;
    for (const std::size_t limit : {2u, 3u, 4u}) {
        dx_data data;
        populateExtensionChain(data, 2u);
        dx_iface interface_;
        interface_.setExtensionGraphLimits({100u, 100u, 100u, limit});
        const bool accepted = interface_.fileExport((directory / "inventory.dxf").string(),
                                                   DRW::AC1021, false, &data, false);
        expect(accepted == (limit >= 3u)
                   && interface_.extensionGraphStats().sourceRecords == std::min(limit, std::size_t{3u})
                   && interface_.extensionGraphFailure() == (accepted ? Failure::None : Failure::SourceRecordLimit),
               "source inventory has its own small deterministic record limit");
    }
    {
        dx_data data;
        populateExtensionChain(data, 2u);
        data.dictionaries.emplace_back(); // Unreachable/null slots still cost inventory work.
        dx_iface interface_;
        interface_.setExtensionGraphLimits({100u, 100u, 100u, 3u});
        expect(!interface_.fileExport((directory / "unreachable.dxf").string(),
                                      DRW::AC1021, false, &data, false)
                   && interface_.extensionGraphFailure() == Failure::SourceRecordLimit
                   && interface_.extensionGraphStats().sourceRecords == 3u
                   && interface_.extensionGraphStats().visitedNodes == 0u,
               "unreachable/null inventory records cannot bypass the indexing budget");
    }
    {
        dx_data empty;
        dx_iface interface_;
        interface_.setExtensionGraphLimits({0u, 0u, 0u});
        expect(interface_.fileExport((directory / "empty.dxf").string(),
                                     DRW::AC1021, false, &empty, false)
                   && interface_.extensionGraphStats().visitedNodes == 0u
                   && interface_.extensionGraphStats().examinedEdges == 0u
                   && interface_.extensionGraphStats().stagedHandles == 0u,
               "zero graph budgets admit an empty extension graph");
    }
    for (const bool binary : {false, true}) {
        for (const bool wide : {false, true}) {
            for (const int resource : {0, 1, 2}) {
                for (std::size_t count : {2u, 3u, 4u}) {
                    dx_data data;
                    populateExtensionChain(data, count, wide);
                    dx_iface interface_;
                    dx_iface::ExtensionGraphLimits limits{100u, 100u, 100u};
                    if (resource == 0)
                        limits.maxNodes = 3u;
                    else if (resource == 1)
                        limits.maxEdges = 6u;
                    else
                        limits.maxStagedHandles = 3u;
                    interface_.setExtensionGraphLimits(limits);
                    const auto path = directory / "boundary.dxf";
                    { std::ofstream original(path); original << "original destination\n"; }
                    const bool accepted = interface_.fileExport(
                        path.string(), DRW::AC1021, binary, &data, false);
                    expect(accepted == (count <= 3u), "extension graph limit boundary disposition");
                    const auto stats = interface_.extensionGraphStats();
                    expect(stats.visitedNodes <= limits.maxNodes
                               && stats.examinedEdges <= limits.maxEdges
                               && stats.stagedHandles <= limits.maxStagedHandles,
                           "extension graph never consumes beyond configured budgets");
                    if (accepted) {
                        expect(stats.visitedNodes == count && stats.examinedEdges == count * 2u
                                   && stats.stagedHandles == count
                                   && interface_.extensionGraphFailure() == Failure::None,
                               "accepted chain/fanout reports exact deterministic work");
                        class OrderedInterface final : public dx_iface {
                        public:
                            std::vector<std::uint32_t> handles;
                            void addDictionary(const DRW_Dictionary& d) override {
                                if (d.handle >= 0x201u && d.handle < 0x300u)
                                    handles.push_back(d.handle);
                            }
                        } reader;
                        dx_data readback;
                        expect(reader.fileImport(path.string(), &readback, false),
                               "bounded graph output is independently framed for the next read");
                        std::vector<std::uint32_t> ordered;
                        for (std::size_t i = 0; i < count; ++i)
                            ordered.push_back(static_cast<std::uint32_t>(0x201u + i));
                        expect(reader.handles == ordered, "iterative graph preserves dictionary emission order");
                    } else {
                        const auto expected = resource == 0 ? Failure::NodeLimit
                            : (resource == 1 ? Failure::EdgeLimit : Failure::StagedHandleLimit);
                        expect(interface_.extensionGraphFailure() == expected,
                               "extension graph reports the exhausted resource");
                        expect(contents(path) == "original destination\n",
                               "budget rejection preserves existing destination bytes");
                        dx_data retry;
                        populateExtensionChain(retry, 2u);
                        expect(interface_.fileExport(path.string(), DRW::AC1021, binary, &retry, false)
                                   && interface_.extensionGraphFailure() == Failure::None
                                   && interface_.extensionGraphStats().visitedNodes == 2u,
                               "failed then valid reuse resets staged graph and diagnostics");
                    }
                }
            }
        }
        dx_data shared;
        populateExtensionChain(shared, 2u, true);
        shared.dictionaries.front().m_entries.push_back({"SHARED", 0x202u});
        dx_iface interface_;
        interface_.setExtensionGraphLimits({2u, 5u, 2u});
        const auto path = directory / "shared.dxf";
        expect(interface_.fileExport(path.string(), DRW::AC1021, binary, &shared, false)
                   && interface_.extensionGraphStats().visitedNodes == 2u
                   && interface_.extensionGraphStats().examinedEdges == 5u,
               "shared dictionary target consumes one node but each reference slot");

        dx_data reactors;
        populateExtensionChain(reactors, 1u);
        reactors.dictionaries.front().m_entries.push_back({"RECORD", 0x202u});
        reactors.dictionaries.front().reactorHandles = {0x202u};
        DRW_XRecord record;
        record.handle = 0x202u;
        record.parentHandle = 0x201u;
        record.reactorHandles = {0x201u};
        record.m_dataEntries = {{330, std::string("201")}};
        reactors.xRecords.push_back(record);
        interface_.setExtensionGraphLimits({2u, 7u, 2u});
        expect(interface_.fileExport(path.string(), DRW::AC1021, binary, &reactors, false)
                   && interface_.extensionGraphStats().visitedNodes == 2u
                   && interface_.extensionGraphStats().examinedEdges == 7u,
               "valid non-owning reactor/body pointer cycle remains accepted");
        interface_.setExtensionGraphLimits({2u, 6u, 2u});
        expect(!interface_.fileExport(path.string(), DRW::AC1021, binary, &reactors, false)
                   && interface_.extensionGraphFailure() == Failure::EdgeLimit,
               "XRECORD body reference validation also consumes the edge budget");
        reactors.xRecords.front().m_dataEntries.clear();
        reactors.xRecords.front().m_handleValues.emplace_back(330, 0x201u);
        interface_.setExtensionGraphLimits({2u, 7u, 2u});
        expect(interface_.fileExport(path.string(), DRW::AC1021, binary, &reactors, false)
                   && interface_.extensionGraphStats().examinedEdges == 7u,
               "fallback XRECORD handle vector is counted once and preserves valid pointers");

        dx_data cycle;
        populateExtensionChain(cycle, 2u);
        cycle.dictionaries.back().m_entries.push_back({"CYCLE", 0x201u});
        interface_.setExtensionGraphLimits({100u, 100u, 100u});
        expect(!interface_.fileExport(path.string(), DRW::AC1021, binary, &cycle, false)
                   && interface_.extensionGraphFailure() == Failure::InvalidGraph,
               "owning dictionary cycle remains rejected");
        dx_data duplicate;
        populateExtensionChain(duplicate, 1u);
        duplicate.dictionaries.push_back(duplicate.dictionaries.front());
        expect(!interface_.fileExport(path.string(), DRW::AC1021, binary, &duplicate, false)
                   && interface_.extensionGraphFailure() == Failure::InvalidGraph,
               "duplicate source identity remains rejected");

        dx_data mixed;
        populateExtensionChain(mixed, 1u);
        mixed.dictionaries.front().xDictHandle = 0x203u;
        mixed.dictionaries.front().m_entries.push_back({"CHILD", 0x202u});
        DRW_Dictionary extension;
        extension.handle = 0x203u;
        extension.parentHandle = 0x201u;
        extension.m_entries.push_back({"EXT_LEAF", 0x204u});
        mixed.dictionaries.push_back(extension);
        for (const bool extended : {false, true}) {
            DRW_XRecord leaf;
            leaf.handle = extended ? 0x204u : 0x202u;
            leaf.parentHandle = extended ? 0x203u : 0x201u;
            leaf.m_dataEntries = {{1, std::string(extended ? "EXT_LEAF" : "CHILD")}};
            mixed.xRecords.push_back(leaf);
        }
        interface_.setExtensionGraphLimits({4u, 8u, 4u});
        expect(interface_.fileExport(path.string(), DRW::AC1021, binary, &mixed, false)
                   && interface_.extensionGraphStats().examinedEdges == 8u,
               "mixed dictionary/XRECORD graph fits its exact edge boundary");
        class OrderedMixedInterface final : public dx_iface {
        public:
            std::vector<std::uint32_t> dictionaries;
            std::vector<std::uint32_t> records;
            void addDictionary(const DRW_Dictionary& d) override {
                if (d.handle >= 0x201u && d.handle <= 0x204u)
                    dictionaries.push_back(d.handle);
            }
            void addXRecord(const DRW_XRecord& r) override {
                records.push_back(r.handle);
            }
        } reader;
        dx_data readback;
        expect(reader.fileImport(path.string(), &readback, false)
                   && reader.dictionaries == std::vector<std::uint32_t>{0x201u, 0x203u}
                   && reader.records == std::vector<std::uint32_t>{0x204u, 0x202u},
               "extension descendants retain precedence over ordinary dictionary entries");
        mixed.xRecords.front().xDictHandle = 0x201u;
        interface_.setExtensionGraphLimits({100u, 100u, 100u});
        expect(!interface_.fileExport(path.string(), DRW::AC1021, binary, &mixed, false)
                   && interface_.extensionGraphFailure() == Failure::InvalidGraph,
               "owned extension-dictionary cycle through an XRECORD remains rejected");
    }
    return passed;
}

bool generateExtensionCycle(const std::string& path, bool binary) {
    // From-scratch invalid ownership control for the real CLI's publication
    // boundary. No large/deep graph or stack-exhaustion experiment is needed.
    std::ofstream output(path, std::ios::binary);
    if (!output)
        return false;
    std::unique_ptr<dxfWriter> writer;
    if (binary) {
        output.write("AutoCAD Binary DXF\r\n\x1A\0", 22);
        writer = std::make_unique<dxfWriterBinary>(&output);
    } else {
        writer = std::make_unique<dxfWriterAscii>(&output);
    }
    writer->setVersion("AC1021", true);
    writer->writeString(0, "SECTION");
    writer->writeString(2, "HEADER");
    writer->writeString(9, "$ACADVER");
    writer->writeString(1, "AC1021");
    writer->writeString(0, "ENDSEC");
    writer->writeString(0, "SECTION");
    writer->writeString(2, "TABLES");
    writer->writeString(0, "TABLE");
    writer->writeString(2, "LTYPE");
    writer->writeString(5, "2");
    writer->writeString(330, "0");
    writer->writeString(100, "AcDbSymbolTable");
    writer->writeInt16(70, 1);
    writer->writeString(0, "LTYPE");
    writer->writeString(5, "101");
    writer->writeString(102, "{ACAD_XDICTIONARY");
    writer->writeString(360, "201");
    writer->writeString(102, "}");
    writer->writeString(330, "2");
    writer->writeString(100, "AcDbSymbolTableRecord");
    writer->writeString(100, "AcDbLinetypeTableRecord");
    writer->writeString(2, "ByLayer");
    writer->writeInt16(70, 0);
    writer->writeString(3, "");
    writer->writeInt16(72, 65);
    writer->writeInt16(73, 0);
    writer->writeDouble(40, 0.0);
    writer->writeString(0, "ENDTAB");
    writer->writeString(0, "ENDSEC");
    writer->writeString(0, "SECTION");
    writer->writeString(2, "OBJECTS");
    for (const bool root : {true, false}) {
        writer->writeString(0, "DICTIONARY");
        writer->writeString(5, root ? "201" : "202");
        writer->writeString(330, root ? "101" : "201");
        writer->writeString(100, "AcDbDictionary");
        writer->writeInt16(280, 1);
        writer->writeInt16(281, 1);
        writer->writeString(3, "CYCLE");
        writer->writeString(360, root ? "202" : "201");
    }
    writer->writeString(0, "ENDSEC");
    writer->writeString(0, "EOF");
    return !writer->hasWriteError() && output.good();
}

} // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--extension-graph-limits")
        return extensionGraphLimits() ? 0 : 1;
    if (argc == 2 && std::string(argv[1]) == "--reject-dynamic-block")
        return rejectDynamicBlockWithoutSerializer() ? 0 : 1;
    if (argc != 3) {
        std::cerr << "usage: dwg2dxf_adapter_tests --generate|--verify <file>\n";
        return 2;
    }

    const std::string mode = argv[1];
    const std::string path = argv[2];
    if (mode == "--generate")
        return generate(path) ? 0 : 1;
    if (mode == "--generate-extension-cycle")
        return generateExtensionCycle(path, false) ? 0 : 1;
    if (mode == "--generate-extension-cycle-binary")
        return generateExtensionCycle(path, true) ? 0 : 1;
    if (mode == "--verify")
        return verify(path) ? 0 : 1;
    if (mode == "--generate-3dline")
        return generate3DLine(path, false) ? 0 : 1;
    if (mode == "--generate-3dline-binary")
        return generate3DLine(path, true) ? 0 : 1;
    if (mode == "--verify-3dline")
        return verify3DLine(path) ? 0 : 1;
    if (mode == "--generate-maintenance-header")
        return generateMaintenanceHeader(path, false) ? 0 : 1;
    if (mode == "--generate-maintenance-header-binary")
        return generateMaintenanceHeader(path, true) ? 0 : 1;
    if (mode == "--verify-maintenance-header")
        return verifyMaintenanceHeader(path) ? 0 : 1;
    if (mode == "--reject-maintenance-downgrade")
        return rejectMaintenanceDowngrade(path) ? 0 : 1;
    if (mode == "--reject-3dline-ac1014")
        return reject3DLineBeforeAC1015(path) ? 0 : 1;
    std::cerr << "unknown mode: " << mode << '\n';
    return 2;
}
