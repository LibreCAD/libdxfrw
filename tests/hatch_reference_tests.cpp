#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "dwg2dxf/dx_iface.h"
#include "intern/dxfreader.h"

namespace {

using Tag = std::pair<int, std::string>;
using Point = std::array<double, 2>;
const std::array<Point, 4> outer {{{0, -1}, {14, -1}, {14, 5}, {0, 5}}};
const std::array<Point, 4> inner {{{1, 1}, {3, 1}, {3, 3}, {1, 3}}};
const std::array<Point, 4> controls {{{6, 0}, {8, 3}, {10, 3}, {12, 0}}};
const std::vector<double> knots {2, 2, 2, 3.5, 7, 7, 7};
const std::vector<double> weights {1.25, 2.5, .75, 1.5};

std::string number(double value) {
    std::ostringstream stream;
    stream << std::setprecision(17) << value;
    return stream.str();
}

std::shared_ptr<DRW_LWPolyline> polyline(const std::array<Point, 4>& points) {
    auto value = std::make_shared<DRW_LWPolyline>();
    value->flags = 1;
    for (const Point& point : points)
        value->addVertex(DRW_Vertex2D(point[0], point[1], 0));
    return value;
}

std::unique_ptr<dx_data> drawing(bool hatchFirst, bool reactors, bool detached = false) {
    auto data = std::make_unique<dx_data>();
    auto* hatch = new DRW_Hatch();
    hatch->handle = 0x40;
    hatch->layer = "HATCH_CONTROL";
    hatch->name = "SOLID";
    hatch->associative = detached ? 0 : 1;
    hatch->extPoint = {0, 0, 1};
    auto first = std::make_shared<DRW_HatchLoop>(7); // external + polyline + derived
    first->objlist.push_back(polyline(outer));
    if (!detached) first->m_boundaryHandles = {0x31};
    first->update();
    hatch->appendLoop(first);
    auto second = std::make_shared<DRW_HatchLoop>(0);
    auto spline = std::make_shared<DRW_Spline>();
    spline->degree = 2;
    spline->flags = 4;
    spline->knotslist = knots;
    spline->weightlist = weights;
    for (const Point& point : controls)
        spline->controllist.push_back(std::make_shared<DRW_Coord>(point[0], point[1], 0));
    auto closing = std::make_shared<DRW_Line>();
    closing->basePoint = {12, 0, 0};
    closing->secPoint = {6, 0, 0};
    second->objlist = {spline, closing};
    if (!detached) second->m_boundaryHandles = {0x32, 0x33};
    second->update();
    hatch->appendLoop(second);
    auto third = std::make_shared<DRW_HatchLoop>(2);
    third->objlist.push_back(polyline(inner));
    third->update();
    hatch->appendLoop(third);
    hatch->loopsnum = 3;
    auto* boundary1 = new DRW_LWPolyline(*polyline(outer));
    auto* boundary2 = new DRW_Spline(*spline);
    auto* boundary3 = new DRW_Line(*closing);
    DRW_Entity* boundaries[] = {boundary1, boundary2, boundary3};
    if (hatchFirst) data->mBlock->ent.push_back(hatch);
    for (std::size_t i = 0; i < 3; ++i) {
        boundaries[i]->handle = 0x31 + static_cast<std::uint32_t>(i);
        boundaries[i]->layer = "BOUNDARY_" + std::to_string(i + 1);
        if (reactors && !detached) boundaries[i]->reactorHandles = {hatch->handle};
        data->mBlock->ent.push_back(boundaries[i]);
    }
    if (!hatchFirst) data->mBlock->ent.push_back(hatch);
    auto* after = new DRW_Line();
    after->handle = 0x99;
    after->layer = "FOLLOWING_CONTROL";
    after->basePoint = {20, 21, 22};
    after->secPoint = {23, 24, 25};
    data->mBlock->ent.push_back(after);
    for (const char* name : {"HATCH_CONTROL", "BOUNDARY_1", "BOUNDARY_2", "BOUNDARY_3", "FOLLOWING_CONTROL"}) {
        DRW_Layer layer;
        layer.name = name;
        data->layers.push_back(layer);
    }
    return data;
}

class Writer final : public dx_iface {
public:
    Writer(dxfRW& output, dx_data& data, bool preplan) : preplan(preplan) {
        dxfW = &output;
        cData = &data;
    }
    void writeHeader(DRW_Header& header) override {
        if (preplan) {
            for (const char* layer : {"HATCH_CONTROL", "BOUNDARY_1", "BOUNDARY_2", "BOUNDARY_3"}) {
                for (const DRW_Entity* entity : cData->mBlock->ent) {
                    if (entity->layer == layer && dxfW->preallocateEntityHandle(entity->handle) == 0)
                        planned = false;
                }
            }
        }
        dx_iface::writeHeader(header);
    }
    bool planned = true;
private:
    bool preplan;
};

struct Record {
    std::string type, handle, owner, layer;
    std::vector<std::string> reactors;
    std::vector<Tag> body;
};

bool readRecords(const std::filesystem::path& path, std::map<std::string, Record>& records) {
    std::ifstream input(path, std::ios::binary);
    char signature[22] = {};
    input.read(signature, sizeof(signature));
    const bool binary = std::string(signature, sizeof(signature))
        == std::string("AutoCAD Binary DXF\r\n\x1a\0", 22);
    input.clear();
    input.seekg(binary ? 22 : 0);
    std::unique_ptr<dxfReader> reader;
    if (binary) reader = std::make_unique<dxfReaderBinary>(&input);
    else reader = std::make_unique<dxfReaderAscii>(&input);
    Record record;
    bool reactor = false, body = false;
    const auto finish = [&]() {
        if (record.layer == "HATCH_CONTROL" || record.layer == "FOLLOWING_CONTROL"
            || record.layer.rfind("BOUNDARY_", 0) == 0)
            return records.emplace(record.layer, record).second;
        return true;
    };
    int code = 0;
    while (reader->readRec(&code)) {
        if (code == 0) {
            if (!finish()) return false;
            record = {};
            reactor = body = false;
            record.type = reader->getString();
            if (record.type == "EOF") return true;
        } else if (body) {
            const bool string = code == 2 || code == 330 || code == 100;
            const bool integer = (code >= 70 && code <= 99);
            record.body.emplace_back(code, string ? reader->getString()
                : integer ? std::to_string(reader->getInt32()) : number(reader->getDouble()));
        } else if (code == 102) {
            reactor = reader->getString() == "{ACAD_REACTORS";
        } else if (code == 330) {
            if (reactor) record.reactors.push_back(reader->getString());
            else record.owner = reader->getString();
        } else if (code == 5) record.handle = reader->getString();
        else if (code == 8) record.layer = reader->getString();
        else if (code == 100 && reader->getString() == "AcDbHatch") body = true;
    }
    return false;
}

bool verify(const std::filesystem::path& path, bool reactors, bool detached) {
    std::map<std::string, Record> records;
    if (!readRecords(path, records) || records.size() != 5) return false;
    for (const char* name : {"HATCH_CONTROL", "BOUNDARY_1", "BOUNDARY_2", "BOUNDARY_3", "FOLLOWING_CONTROL"})
        if (records.count(name) != 1) return false;
    if (records.at("BOUNDARY_1").type != "LWPOLYLINE"
        || records.at("BOUNDARY_2").type != "SPLINE"
        || records.at("BOUNDARY_3").type != "LINE"
        || records.at("FOLLOWING_CONTROL").type != "LINE") return false;
    const Record& hatch = records.at("HATCH_CONTROL");
    std::set<std::string> handles;
    for (const auto& entry : records) {
        if (entry.second.handle.empty() || entry.second.handle == "0"
            || entry.second.handle == "30" || entry.second.owner != "1F"
            || !handles.insert(entry.second.handle).second) return false;
    }
    std::vector<Tag> expected;
    const auto add = [&](int code, double value) { expected.emplace_back(code, number(value)); };
    add(10, 0); add(20, 0); add(30, 0);
    add(210, 0); add(220, 0); add(230, 1);
    expected.emplace_back(2, "SOLID");
    add(70, 1); add(71, detached ? 0 : 1); add(91, 3);
    const auto addPolyline = [&](int flags, const std::array<Point, 4>& points) {
        add(92, flags); add(72, 0); add(73, 1); add(93, 4);
        for (const Point& point : points) { add(10, point[0]); add(20, point[1]); }
    };
    addPolyline(7, outer);
    add(97, detached ? 0 : 1);
    if (!detached) expected.emplace_back(330, records.at("BOUNDARY_1").handle);
    add(92, 0); add(93, 2); add(72, 4); add(94, 2); add(73, 1); add(74, 0);
    add(95, 7); add(96, 4);
    for (double knot : knots) add(40, knot);
    for (std::size_t i = 0; i < controls.size(); ++i) {
        add(10, controls[i][0]); add(20, controls[i][1]); add(42, weights[i]);
    }
    add(97, 0); // spline fit-point count, distinct from the loop source count
    add(72, 1); add(10, 12); add(20, 0); add(11, 6); add(21, 0);
    add(97, detached ? 0 : 2);
    if (!detached) {
        expected.emplace_back(330, records.at("BOUNDARY_2").handle);
        expected.emplace_back(330, records.at("BOUNDARY_3").handle);
    }
    addPolyline(2, inner);
    add(97, 0); add(75, 0); add(76, 1); add(78, 0); add(98, 0);
    if (hatch.type != "HATCH" || hatch.body != expected) {
        std::cerr << "HATCH ordered path/reference tags differ: " << path << '\n';
        for (std::size_t i = 0; i < hatch.body.size() && i < expected.size(); ++i) {
            if (hatch.body[i] != expected[i]) {
                std::cerr << "  tag " << i << ": expected " << expected[i].first
                          << '/' << expected[i].second << ", observed "
                          << hatch.body[i].first << '/' << hatch.body[i].second << '\n';
                break;
            }
        }
        return false;
    }
    for (const char* name : {"BOUNDARY_1", "BOUNDARY_2", "BOUNDARY_3"}) {
        const auto wanted = reactors && !detached ? std::vector<std::string>{hatch.handle}
                                                  : std::vector<std::string>{};
        if (records.at(name).reactors != wanted) return false;
    }
    dx_data imported;
    dx_iface reader;
    if (!reader.fileImport(path.string(), &imported, false) || imported.mBlock->ent.size() != 5)
        return false;
    std::map<std::string, std::uint32_t> typedHandles;
    for (const DRW_Entity* entity : imported.mBlock->ent)
        typedHandles.emplace(entity->layer, entity->handle);
    for (const DRW_Entity* entity : imported.mBlock->ent) {
        if (entity->layer == "HATCH_CONTROL") {
            const auto* typed = dynamic_cast<const DRW_Hatch*>(entity);
            if (!typed || typed->looplist.size() != 3 || typed->associative != (detached ? 0 : 1))
                return false;
            for (std::size_t i = 0; i < 3; ++i) {
                const std::vector<std::uint32_t> references = detached || i == 2
                    ? std::vector<std::uint32_t>{} : i == 0
                    ? std::vector<std::uint32_t>{typedHandles.at("BOUNDARY_1")}
                    : std::vector<std::uint32_t>{typedHandles.at("BOUNDARY_2"), typedHandles.at("BOUNDARY_3")};
                if (!typed->looplist[i] || typed->looplist[i]->type != (i == 0 ? 7 : i == 1 ? 0 : 2)
                    || typed->looplist[i]->objlist.size() != (i == 1 ? 2u : 1u)
                    || typed->looplist[i]->m_boundaryHandles != references) return false;
            }
            const auto* spline = dynamic_cast<const DRW_Spline*>(typed->looplist[1]->objlist[0].get());
            if (!spline || spline->degree != 2 || spline->knotslist != knots
                || spline->weightlist != weights || spline->controllist.size() != 4) return false;
            for (std::size_t i = 0; i < controls.size(); ++i)
                if (spline->controllist[i]->x != controls[i][0]
                    || spline->controllist[i]->y != controls[i][1] || spline->controllist[i]->z != 0)
                    return false;
        } else if (entity->layer == "FOLLOWING_CONTROL") {
            const auto* line = dynamic_cast<const DRW_Line*>(entity);
            if (!line || line->basePoint.x != 20 || line->basePoint.y != 21
                || line->basePoint.z != 22 || line->secPoint.x != 23
                || line->secPoint.y != 24 || line->secPoint.z != 25) return false;
        }
    }
    return true;
}

bool emit(dxfRW& output, dx_data& data, bool binary,
          bool preplan, bool resolver, std::uint32_t rejectedSource = 0) {
    Writer iface(output, data, preplan);
    if (resolver) output.setReferenceResolver([&output, rejectedSource](std::uint32_t source) {
        return source == rejectedSource ? 0u : output.remapEntityHandle(source);
    });
    else output.setReferenceResolver({});
    return output.write(&iface, DRW::AC1027, binary) && iface.planned;
}

bool write(const std::filesystem::path& path, dx_data& data, bool binary,
           bool preplan, bool resolver) {
    dxfRW output(path.string().c_str());
    return output.reserveHandle(0x30) && emit(output, data, binary, preplan, resolver);
}

} // namespace

bool writeHatchReferenceFile(const std::filesystem::path& path, bool binary) {
    auto data = drawing(true, true);
    return write(path, *data, binary, true, true) && verify(path, true, false);
}

bool verifyHatchReferenceFile(const std::filesystem::path& path) {
    return verify(path, true, false);
}

bool runHatchReferenceTests(const std::filesystem::path& parent, bool keepOutputs) {
    const auto directory = parent / ("libdxfrw-hatch-references-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    bool passed = true;
    for (bool binary : {false, true}) {
        for (bool first : {false, true}) {
            for (bool resolver : {false, true}) {
                const auto path = directory / (std::to_string(binary) + '-' + std::to_string(first)
                    + '-' + std::to_string(resolver) + ".dxf");
                auto source = drawing(first, resolver);
                passed = write(path, *source, binary, true, resolver)
                    && verify(path, resolver, false) && passed;
                dx_data imported;
                dx_iface reader;
                const auto second = directory / (path.stem().string() + "-second.dxf");
                passed = reader.fileImport(path.string(), &imported, false)
                    && write(second, imported, binary, true, resolver)
                    && verify(second, resolver, false) && passed;
            }
        }
        for (bool detached : {false, true}) {
            const auto path = directory / ("backward-" + std::to_string(binary)
                + '-' + std::to_string(detached) + ".dxf");
            auto source = drawing(false, false, detached);
            passed = write(path, *source, binary, false, false)
                && verify(path, false, detached) && passed;
        }
        for (bool nullReference : {false, true}) {
            const auto path = directory / ("reject-" + std::to_string(binary)
                + '-' + std::to_string(nullReference) + ".dxf");
            const std::string sentinel = "existing destination\n";
            { std::ofstream stream(path, std::ios::binary); stream << sentinel; }
            auto source = drawing(true, true);
            if (nullReference) static_cast<DRW_Hatch*>(source->mBlock->ent.front())
                ->looplist[1]->m_boundaryHandles[0] = 0;
            dxfRW output(path.string().c_str());
            const bool reserved = output.reserveHandle(0x30);
            const bool rejected = reserved
                && !emit(output, *source, binary, true, true, nullReference ? 0 : 0x32);
            std::ifstream input(path, std::ios::binary);
            const std::string bytes((std::istreambuf_iterator<char>(input)), {});
            input.close();
            passed = rejected && bytes == sentinel && passed;
            source = drawing(true, true);
            passed = emit(output, *source, binary, true, true)
                && verify(path, true, false) && passed;
        }
    }
    std::cout << "HATCH reference/path writer controls: " << (passed ? "PASS" : "FAIL") << '\n';
    if (passed && !keepOutputs) std::filesystem::remove_all(directory);
    else std::cout << "HATCH reference evidence: " << directory << '\n';
    return passed;
}
