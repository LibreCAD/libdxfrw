// SPDX-License-Identifier: GPL-2.0-or-later
// Selected, explicitly authorized preview transport; not whole-document support.
#include "dx_iface.h"
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

class DrwEntityEncodeTestAccess {
public:
    static void setParsedDwg(DRW_Entity& e, DRW::Version v) {
        e.m_proxyGraphicsProvenance = {DRW_ProxyGraphicsOrigin::ParsedDwg,
            DRW_ProxyGraphicsEncoding::DwgBitStream, v, {}, 0u,
            static_cast<std::uint64_t>(e.proxyGraphics.size()), false};
    }
};

static std::string preview() {
    const std::string hex =
        "98000000040000000c0000000e000000120000000c00000016000000000000c2"
        "0c00000014000000010000006c00000007000000040000000000000000000000"
        "0000000000000000000000000000000000000000000010400000000000000000"
        "0000000000000000000000000000104000000000000010400000000000000000"
        "000000000000000000000000000010400000000000000000";
    const auto nibble = [](char c) { return c <= '9' ? c - '0' : c - 'a' + 10; };
    std::string result;
    for (std::size_t i = 0; i < hex.size(); i += 2u)
        result.push_back(static_cast<char>((nibble(hex[i]) << 4) | nibble(hex[i + 1u])));
    return result;
}

static DRW_Wipeout authored(DRW::Version version) {
    DRW_Wipeout value;
    value.basePoint = DRW_Coord(0, 0, 0);
    value.secPoint = DRW_Coord(4, 0, 0);
    value.vVector = DRW_Coord(0, 4, 0);
    value.sizeu = value.sizev = 1;
    value.clip = 1;
    value.m_clipBoundaryType = 2;
    value.clipPath = {DRW_Coord(-.5, .5), DRW_Coord(.5, .5),
                      DRW_Coord(.5, -.5), DRW_Coord(-.5, -.5)};
    value.proxyGraphics = preview();
    value.numProxyGraph = static_cast<int>(value.proxyGraphics.size());
    DrwEntityEncodeTestAccess::setParsedDwg(value, version);
    return value;
}

class Readback final : public dx_iface {
public:
    void addWipeout(const DRW_Wipeout* data) override { if (data) records.push_back(*data); }
    std::vector<DRW_Wipeout> records;
};

class DirectWriterBase : public dx_iface {
public:
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {}
    void writeAppId() override {}
    void writeViews() override {}
    void writeUCSs() override {}
    bool finalizeDxfWrite() override { return true; }
};

class DirectWriter final : public DirectWriterBase {
public:
    enum class Route { Wipeout, Line, Image };
    DirectWriter(dxfRW& output, DRW_Wipeout& item, Route route)
        : output(output), item(item), route(route) {}
    void writeHeader(DRW_Header& header) override { header.vars.clear(); }
    void writeEntities() override {
        if (route == Route::Wipeout) accepted = output.writeWipeout(&item);
        else if (route == Route::Line) accepted = output.writeLine(&item);
        else accepted = output.writeImage(&item, "unused") != nullptr;
    }
    dxfRW& output;
    DRW_Wipeout& item;
    Route route;
    bool accepted{false};
};

class ForgedLineWriter final : public DirectWriterBase {
public:
    ForgedLineWriter(dxfRW& output, DRW_Line& item)
        : output(output), item(item) {}
    void writeHeader(DRW_Header& header) override { header.vars.clear(); }
    void writeEntities() override { accepted = output.writeLine(&item); }
    dxfRW& output;
    DRW_Line& item;
    bool accepted{false};
};

static bool dispatchControls(DRW_Wipeout value, DRW::Version version, bool binary,
                             const std::filesystem::path& dir) {
    const auto path = dir / "dispatch.dxf";
    for (auto route : {DirectWriter::Route::Line, DirectWriter::Route::Image}) {
        { std::ofstream out(path, std::ios::binary | std::ios::trunc); out << "SENTINEL"; }
        auto writer = std::make_unique<dxfRW>(path.string().c_str());
        auto adapter = std::make_unique<DirectWriter>(*writer, value, route);
        if (writer->write(adapter.get(), version, binary) || adapter->accepted) return false;
        std::ifstream in(path, std::ios::binary);
        if (std::string(std::istreambuf_iterator<char>(in), {}) != "SENTINEL") return false;
    }
    for (auto type : {DRW::LINE, DRW::PROXYENTITY}) {
        auto wrongType = value;
        wrongType.eType = type;
        { std::ofstream out(path, std::ios::binary | std::ios::trunc); out << "SENTINEL"; }
        auto writer = std::make_unique<dxfRW>(path.string().c_str());
        auto adapter = std::make_unique<DirectWriter>(*writer, wrongType, DirectWriter::Route::Wipeout);
        if (writer->write(adapter.get(), version, binary) || adapter->accepted) return false;
        std::ifstream in(path, std::ios::binary);
        if (std::string(std::istreambuf_iterator<char>(in), {}) != "SENTINEL") return false;
    }
    DRW_Line forged;
    forged.eType = DRW::WIPEOUT;
    forged.proxyGraphics = value.proxyGraphics;
    forged.numProxyGraph = static_cast<int>(forged.proxyGraphics.size());
    DrwEntityEncodeTestAccess::setParsedDwg(forged, version);
    if (!forged.authorizeParsedDwgProxyGraphicsReplay()) return false;
    { std::ofstream out(path, std::ios::binary | std::ios::trunc); out << "SENTINEL"; }
    auto forgedWriter = std::make_unique<dxfRW>(path.string().c_str());
    auto forgedAdapter = std::make_unique<ForgedLineWriter>(*forgedWriter, forged);
    if (forgedWriter->write(forgedAdapter.get(), version, binary) || forgedAdapter->accepted) return false;
    std::ifstream forgedOutput(path, std::ios::binary);
    if (std::string(std::istreambuf_iterator<char>(forgedOutput), {}) != "SENTINEL") return false;
    forgedOutput.close();

    auto outer = value;
    outer.reactorHandles = {123u}; // Invoke the resolver after WIPEOUT preflight.
    auto outerWriter = std::make_unique<dxfRW>((dir / "outer.dxf").string().c_str());
    auto outerAdapter = std::make_unique<DirectWriter>(*outerWriter, outer, DirectWriter::Route::Wipeout);
    bool called = false, nestedOkay = false, wrongWriterRefused = false;
    outerWriter->setReferenceResolver([&](std::uint32_t source) {
        if (source != 123u) return source;
        if (!called) {
            called = true;
            auto wrongWriter = std::make_unique<dxfRW>((dir / "nested-wrong.dxf").string().c_str());
            auto wrongAdapter = std::make_unique<DirectWriter>(*wrongWriter, outer, DirectWriter::Route::Line);
            wrongWriterRefused = !wrongWriter->write(wrongAdapter.get(), version, binary) && !wrongAdapter->accepted;
            auto inner = value;
            auto innerWriter = std::make_unique<dxfRW>((dir / "nested-valid.dxf").string().c_str());
            auto innerAdapter = std::make_unique<DirectWriter>(*innerWriter, inner, DirectWriter::Route::Wipeout);
            nestedOkay = innerWriter->write(innerAdapter.get(), version, binary) && innerAdapter->accepted;
        }
        return 1u;
    });
    if (!outerWriter->write(outerAdapter.get(), version, binary) || !outerAdapter->accepted
        || !called || !nestedOkay || !wrongWriterRefused) return false;

    // Same writer and same object must not reuse the already-consumed permit
    // from a resolver callback. The failed nested action must abort the outer
    // write atomically and leave the writer reusable after correction.
    const auto samePath = dir / "nested-same-writer.dxf";
    { std::ofstream out(samePath, std::ios::binary | std::ios::trunc); out << "SENTINEL"; }
    auto sameWriter = std::make_unique<dxfRW>(samePath.string().c_str());
    auto sameAdapter = std::make_unique<DirectWriter>(*sameWriter, outer, DirectWriter::Route::Wipeout);
    bool sameCalled = false, sameRefused = false;
    sameWriter->setReferenceResolver([&](std::uint32_t source) {
        if (source == 123u && !sameCalled) {
            sameCalled = true;
            sameRefused = !sameWriter->writeLine(&outer);
        }
        return source;
    });
    if (sameWriter->write(sameAdapter.get(), version, binary) || !sameCalled || !sameRefused) return false;
    std::ifstream sameOutput(samePath, std::ios::binary);
    if (std::string(std::istreambuf_iterator<char>(sameOutput), {}) != "SENTINEL") return false;
    sameOutput.close();
    sameWriter->setReferenceResolver([](std::uint32_t source) { return source; });
    if (!sameWriter->write(sameAdapter.get(), version, binary) || !sameAdapter->accepted) return false;

    // A throwing resolver must unwind the outer scope and leave later writes
    // on the same thread and writer usable.
    auto retryWriter = std::make_unique<dxfRW>((dir / "exception.dxf").string().c_str());
    auto retryAdapter = std::make_unique<DirectWriter>(*retryWriter, outer, DirectWriter::Route::Wipeout);
    retryWriter->setReferenceResolver([](std::uint32_t source) -> std::uint32_t {
        if (source == 123u) throw std::runtime_error("test");
        return source;
    });
    if (retryWriter->write(retryAdapter.get(), version, binary)) return false;
    retryWriter->setReferenceResolver([](std::uint32_t) { return 1u; });
    if (!retryWriter->write(retryAdapter.get(), version, binary) || !retryAdapter->accepted) return false;
    return true;
}

static bool exercise(DRW_Wipeout value, DRW::Version version,
                     const std::filesystem::path& preserve = {}) {
    if (value.proxyGraphics != preview() || value.proxyGraphics.size() != 152u) return false;
    value.revokeProxyGraphicsReplayAuthorization();
    const auto original = value.proxyGraphics;
    const auto temp = std::filesystem::temp_directory_path()
        / ("wipeout-preview-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(temp);
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code e; std::filesystem::remove_all(path, e); } } cleanup{temp};
    for (bool binary : {false, true}) {
        auto data = std::make_unique<dx_data>();
        auto* item = new DRW_Wipeout(value);
        data->mBlock->ent.push_back(item);
        dx_iface exporter;
        const auto path = temp / (binary ? "binary.dxf" : "ascii.dxf");
        // The existing compatible path does not emit an unacknowledged cache.
        if (!exporter.fileExport(path.string(), version, binary, data.get(), false, false)) return false;
        auto unacknowledged = std::make_unique<dx_data>();
        Readback initial;
        if (!initial.fileImport(path.string(), unacknowledged.get(), false)
            || initial.records.size() != 1u || !initial.records.front().proxyGraphics.empty()) return false;
        if (!item->authorizeParsedDwgProxyGraphicsReplay()) return false;
        // This low-level admission does not change the strict adapter gate.
        { std::ofstream out(path, std::ios::binary | std::ios::trunc); out << "SENTINEL"; }
        if (exporter.fileExport(path.string(), version, binary, data.get(), false, false,
                dx_dxfSourceTextPolicy::Compatible, DRW::DxfTextWritePolicy::Compatible,
                dx_dxfProxyGraphicsPolicy::FailIfNotPreserved)) return false;
        std::ifstream strict(path, std::ios::binary);
        if (std::string(std::istreambuf_iterator<char>(strict), {}) != "SENTINEL") return false;
        strict.close();
        const auto invalid = [&](std::size_t byte, unsigned char replacement) {
            item->revokeProxyGraphicsReplayAuthorization();
            item->proxyGraphics = original;
            item->proxyGraphics[byte] = static_cast<char>(replacement);
            if (byte == 63u) item->proxyGraphics[62u] = static_cast<char>(0xF0u);
            if (!item->authorizeParsedDwgProxyGraphicsReplay()) return false;
            { std::ofstream out(path, std::ios::binary | std::ios::trunc); out << "SENTINEL"; }
            if (exporter.fileExport(path.string(), version, binary, data.get(), false, false)) return false;
            std::ifstream in(path, std::ios::binary);
            return std::string(std::istreambuf_iterator<char>(in), {}) == "SENTINEL";
        };
        // Length, item count, unknown/resource opcode, invalid fill, and
        // non-finite polygon coordinates cannot pass the bounded wire profile.
        if (!invalid(0u, 151u) || !invalid(4u, 5u) || !invalid(12u, 16u)
            || !invalid(36u, 18u) || !invalid(40u, 2u)
            || !invalid(63u, 0x7fu)) return false;
        item->revokeProxyGraphicsReplayAuthorization();
        item->proxyGraphics = original;
        if (!item->authorizeParsedDwgProxyGraphicsReplay()) return false;
        if (!dispatchControls(*item, version, binary, temp)) return false;
        const DRW::Version wrong = version == DRW::AC1024 ? DRW::AC1027 : DRW::AC1024;
        { std::ofstream out(path, std::ios::binary | std::ios::trunc); out << "SENTINEL"; }
        if (exporter.fileExport(path.string(), wrong, binary, data.get(), false, false)) return false;
        std::ifstream rejected(path, std::ios::binary);
        if (std::string(std::istreambuf_iterator<char>(rejected), {}) != "SENTINEL") return false;
        rejected.close();
        if (!exporter.fileExport(path.string(), version, binary, data.get(), false, false)) return false;
        auto observed = std::make_unique<dx_data>();
        Readback reader;
        if (!reader.fileImport(path.string(), observed.get(), false) || reader.records.size() != 1u
            || reader.records.front().proxyGraphics != original
            || reader.records.front().numProxyGraph != static_cast<int>(original.size())) return false;
        if (!preserve.empty()) {
            std::filesystem::create_directories(preserve);
            if (!std::filesystem::copy_file(path, preserve / (binary ? "binary.dxf" : "ascii.dxf"))) return false;
        }
    }
    return true;
}

int main(int argc, char** argv) {
    if (argc == 2 || argc == 3) {
        auto data = std::make_unique<dx_data>();
        auto reader = std::make_unique<dx_iface>();
        if (!reader->fileImport(argv[1], data.get(), false)) return 1;
        for (const auto* base : data->mBlock->ent) {
            const auto* wipeout = dynamic_cast<const DRW_Wipeout*>(base);
            if (wipeout) return exercise(*wipeout, data->sourceVersion, argc == 3 ? argv[2] : "") ? 0 : 1;
        }
        return 1;
    }
    return exercise(authored(DRW::AC1024), DRW::AC1024)
        && exercise(authored(DRW::AC1027), DRW::AC1027)
        && exercise(authored(DRW::AC1032), DRW::AC1032) ? 0 : 1;
}
