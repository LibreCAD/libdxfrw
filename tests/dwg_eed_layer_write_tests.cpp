// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>

namespace {
using Order = DRW::DwgEedLayerByteOrder;
constexpr std::uint32_t lineHandle = 0x130u;

class Writer final : public LC3CompatCheck, public DRW_DwgLegacyModelChainProvider,
                     public DRW_DwgDefaultPlotStyleProvider {
public:
    explicit Writer(dwgRW& output) : output_(output) {
        line.handle = lineHandle;
        line.basePoint = {1.0, 2.0, 3.0};
        line.secPoint = {4.0, 5.0, 6.0};
        line.extData.push_back(std::make_shared<DRW_Variant>(1001, std::string("ACAD")));
        line.extData.push_back(std::make_shared<DRW_Variant>(1003, std::string("0")));
        table.name = "EED_TABLE";
        table.extData.push_back(new DRW_Variant(1001, std::string("ACAD")));
        table.extData.push_back(new DRW_Variant(1003, std::string("0")));
    }
    std::vector<DRW_DwgLegacyModelChainEntry> dwgLegacyModelChain() const override {
        return {{lineHandle, DRW::LINE}};
    }
    void dwgDefaultPlotStyleGraphPlanned(std::uint32_t, std::uint32_t placeholder) override {
        table.plotStyleHandle.ref = placeholder;
    }
    void writeHeader(DRW_Header&) override {
        ++headerCalls;
        if (flipAtHeader) output_.setDwgWriteEedLayerByteOrder(Order::LegacyLittleEndian);
    }
    void writeLayers() override {
        tableAdded = output_.addLayer(&table);
        if (!tableAdded) output_.markDwgWriteFailure();
    }
    void writeEntities() override {
        emitted = output_.writeLine(&line);
        if (!emitted) output_.markDwgWriteFailure();
    }
    dwgRW& output_;
    DRW_Line line;
    DRW_Layer table;
    bool emitted{false};
    bool tableAdded{false};
    bool flipAtHeader{false};
    unsigned headerCalls{0};
};

class Reader final : public LC3CompatCheck {
public:
    void addLayer(const DRW_Layer& value) override {
        if (value.name != "EED_TABLE") return;
        ++tableCount;
        tableXrefRole = value.xrefBlockHandle.rawCode;
        for (const auto* item : value.extData) {
            if (item && item->code() == 1003 && item->content.s) {
                tableText = *item->content.s;
                tableRaw = item->rawDwgLayerReference();
                tableUnresolved = item->activeDwgLayerReference().has_value();
            }
        }
    }
    void addLine(const DRW_Line& value) override {
        if (value.handle != lineHandle) return;
        ++count;
        for (const auto& item : value.extData) {
            if (item && item->code() == 1003 && item->content.s) {
                text = *item->content.s;
                raw = item->rawDwgLayerReference();
                unresolved = item->activeDwgLayerReference().has_value();
            }
        }
    }
    std::string text, tableText;
    std::uint64_t raw{0}, tableRaw{0};
    unsigned count{0}, tableCount{0};
    unsigned tableXrefRole{255};
    bool unresolved{false}, tableUnresolved{false};
};

std::string bytes(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

bool emit(const std::filesystem::path& path, Order order, bool flip = false,
          bool withEed = true, bool rawEed = false, bool named = false,
          bool retainCommon = false) {
    dwgRW output(path.string().c_str());
    // Force the named target past one-byte handles while leaving the fixed
    // default plot-style graph at 0x30/0x31.
    if (named) for (std::uint32_t h = 0x32u; h < 0x123u; ++h)
        if (!output.reserveHandle(h)) return false;
    if (!output.setDwgWriteEedLayerByteOrder(order)) return false;
    Writer writer(output);
    writer.flipAtHeader = flip;
    if (retainCommon) writer.table.setDwgCommonLinkTailValidated(true);
    if (named) {
        writer.line.extData[1]->addString(1003, "EED_TABLE");
        writer.table.extData[1]->addString(1003, "EED_TABLE");
    }
    if (!withEed) {
        writer.line.extData.clear();
        for (auto* value : writer.table.extData) delete value;
        writer.table.extData.clear();
    }
    if (rawEed) {
        writer.line.extData[1]->addString(1003, "102030405060708");
        writer.line.extData[1]->setDwgRawLayerReference(UINT64_C(0x0102030405060708), DRW::AC1015);
    }
    return output.write(&writer, DRW::AC1015, true) && writer.emitted && writer.tableAdded;
}

bool read(const std::filesystem::path& path, Order order, std::uint64_t raw,
          bool resolved, const std::string& name = "0", unsigned xrefRole = 255u) {
    dwgR input(path.string().c_str());
    if (!input.setDwgEedLayerByteOrder(order)) return false;
    Reader reader;
    if (!input.read(&reader, false) || reader.count != 1u || reader.raw != raw
        || reader.unresolved == resolved || (resolved && reader.text != name)
        || reader.tableCount != 1u
        || (xrefRole != 255u && reader.tableXrefRole != xrefRole)) return false;
    const bool rawReplay = raw == UINT64_C(0x0102030405060708);
    const auto expectedTable = rawReplay ? UINT64_C(0x1200000000000000) : raw;
    const bool tableResolved = rawReplay ? order == Order::BigEndian : resolved;
    return reader.tableRaw == expectedTable && reader.tableUnresolved != tableResolved
        && (!tableResolved || reader.tableText == name);
}

bool controls(const std::filesystem::path& path) {
    const std::string sentinel = "DWG EED layer writer destination preserved\n";
    dwgRW output(path.string().c_str());
    Writer writer(output);
    if (output.dwgWriteEedLayerByteOrder() != Order::LegacyLittleEndian
        || output.setDwgWriteEedLayerByteOrder(static_cast<Order>(99))
        || !output.setDwgWriteEedLayerByteOrder(Order::BigEndian)) return false;
    { std::ofstream out(path, std::ios::binary | std::ios::trunc); out << sentinel; }
    if (output.write(&writer, DRW::AC1018, true) || bytes(path) != sentinel
        || writer.headerCalls != 0u) return false;
    writer.line.extData[1]->addString(1003, "MISSING_LAYER");
    if (output.write(&writer, DRW::AC1015, true) || bytes(path) != sentinel
        || writer.emitted || !writer.line.extData[1]->stringEquals("MISSING_LAYER")) return false;
    writer.line.extData[1]->addString(1003, "0");
    writer.flipAtHeader = true;
    if (!output.write(&writer, DRW::AC1015, true) || !writer.emitted
        || output.dwgWriteEedLayerByteOrder() != Order::LegacyLittleEndian
        || !read(path, Order::BigEndian, UINT64_C(0x1200000000000000), true)
        || !read(path, Order::LegacyLittleEndian, UINT64_C(0x1200000000000000), false)) return false;
    if (!emit(path, Order::LegacyLittleEndian)
        || !read(path, Order::LegacyLittleEndian, 0x12u, true)
        || !read(path, Order::BigEndian, 0x12u, false)) return false;
    if (!emit(path, Order::BigEndian, false, true, false, true)
        || !read(path, Order::BigEndian, UINT64_C(0x2501000000000000), true, "EED_TABLE")
        || !read(path, Order::LegacyLittleEndian, UINT64_C(0x2501000000000000), false)) return false;
    if (!emit(path, Order::BigEndian)
        || !read(path, Order::BigEndian, UINT64_C(0x1200000000000000), true, "0", 5u)
        || !emit(path, Order::BigEndian, false, true, false, false, true)
        || !read(path, Order::BigEndian, UINT64_C(0x1200000000000000), true, "0", 0u)) return false;
    // Raw replay carries physical bytes in little-endian integer order and
    // must not be reversed merely because new references select big-endian.
    if (!emit(path, Order::BigEndian, false, true, true)
        || !read(path, Order::LegacyLittleEndian, UINT64_C(0x0102030405060708), false)
        || !read(path, Order::BigEndian, UINT64_C(0x0102030405060708), false)) return false;
    return true;
}
} // namespace

int main(int argc, char** argv) {
    if (argc == 4 && std::string(argv[1]) == "--emit") {
        const std::string mode = argv[3];
        if (mode != "big" && mode != "little" && mode != "none" && mode != "raw" && mode != "named" && mode != "retained") return 2;
        return emit(argv[2], (mode == "big" || mode == "raw" || mode == "named" || mode == "retained") ? Order::BigEndian : Order::LegacyLittleEndian,
                    false, mode != "none", mode == "raw", mode == "named", mode == "retained") ? 0 : 1;
    }
    if (argc != 1) return 2;
    const auto path = std::filesystem::temp_directory_path() /
        ("libdxfrw-eed-layer-writer-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dwg");
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code error; std::filesystem::remove(path, error);
    }} cleanup{path};
    if (!controls(path)) { std::cerr << "DWG EED layer writer controls failed\n"; return 1; }
    return 0;
}
