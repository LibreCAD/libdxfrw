#include <array>
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <limits>
#include <memory>
#include <set>
#include <stdexcept>
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

std::unique_ptr<dx_data> drawing(bool hatchFirst, bool reactors,
                                 bool detached = false, bool mpolygon = false) {
    auto data = std::make_unique<dx_data>();
    DRW_Hatch* hatch = mpolygon
        ? static_cast<DRW_Hatch*>(new DRW_MPolygon()) : new DRW_Hatch();
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
        else if (code == 100) {
            const std::string subclass = reader->getString();
            if (subclass == "AcDbHatch" || subclass == "AcDbMPolygon")
                body = true;
        }
    }
    return false;
}

bool verify(const std::filesystem::path& path, bool reactors, bool detached,
            bool reservedHandle = true, std::size_t modelEntities = 5) {
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
            || (reservedHandle && entry.second.handle == "30") || entry.second.owner != "1F"
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
    add(97, 0); add(75, 0); add(76, 1); add(78, 0);
    add(47, 0); // the first boundary path is derived, so emit pixel size
    add(98, 0);
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
    if (!reader.fileImport(path.string(), &imported, false) || imported.mBlock->ent.size() != modelEntities)
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

bool verifyAliasedHatchPublication(const std::filesystem::path& path) {
    std::map<std::string, Record> records;
    if (!readRecords(path, records) || records.size() != 3u
        || records.count("HATCH_CONTROL") != 1u
        || records.count("BOUNDARY_1") != 1u
        || records.count("FOLLOWING_CONTROL") != 1u)
        return false;
    const Record& hatchRecord = records.at("HATCH_CONTROL");
    const Record& boundaryRecord = records.at("BOUNDARY_1");
    const Record& followingRecord = records.at("FOLLOWING_CONTROL");
    std::vector<std::string> boundaryRefs;
    for (const Tag& tag : hatchRecord.body)
        if (tag.first == 330) boundaryRefs.push_back(tag.second);
    if (hatchRecord.type != "HATCH" || hatchRecord.handle != "30"
        || boundaryRecord.type != "LWPOLYLINE" || boundaryRecord.handle != "31"
        || followingRecord.type != "LINE" || followingRecord.handle.empty()
        || followingRecord.handle == hatchRecord.handle
        || followingRecord.handle == boundaryRecord.handle
        || hatchRecord.owner.empty() || hatchRecord.owner != boundaryRecord.owner
        || hatchRecord.owner != followingRecord.owner
        || boundaryRefs != std::vector<std::string>{"31"}
        || boundaryRecord.reactors != std::vector<std::string>{"30"})
        return false;

    dx_data imported;
    dx_iface reader;
    if (!reader.fileImport(path.string(), &imported, false)
        || imported.mBlock->ent.size() != 3u)
        return false;
    const DRW_Hatch* hatch = nullptr;
    const DRW_LWPolyline* boundary = nullptr;
    const DRW_Line* following = nullptr;
    for (const DRW_Entity* entity : imported.mBlock->ent) {
        if (entity->layer == "HATCH_CONTROL")
            hatch = dynamic_cast<const DRW_Hatch*>(entity);
        else if (entity->layer == "BOUNDARY_1")
            boundary = dynamic_cast<const DRW_LWPolyline*>(entity);
        else if (entity->layer == "FOLLOWING_CONTROL")
            following = dynamic_cast<const DRW_Line*>(entity);
    }
    return hatch != nullptr && boundary != nullptr && following != nullptr
        && hatch->handle == std::stoul(hatchRecord.handle, nullptr, 16)
        && hatch->associative == 1
        && hatch->looplist.size() == 1u && hatch->looplist.front() != nullptr
        && hatch->looplist.front()->m_boundaryHandles
            == std::vector<std::uint32_t>{0x31u}
        && boundary->handle == std::stoul(boundaryRecord.handle, nullptr, 16)
        && boundary->reactorHandles == std::vector<std::uint32_t>{0x30u}
        && following->handle == std::stoul(followingRecord.handle, nullptr, 16)
        && following->basePoint.x == 20 && following->basePoint.y == 21
        && following->basePoint.z == 22 && following->secPoint.x == 23
        && following->secPoint.y == 24 && following->secPoint.z == 25;
}

bool write(const std::filesystem::path& path, dx_data& data, bool binary,
           bool preplan, bool resolver);

bool rewriteHatchPixelSizeToZero(const std::filesystem::path& path,
                                 bool binary) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    std::string bytes((std::istreambuf_iterator<char>(input)), {});
    input.close();
    std::size_t replacements = 0;
    if (binary) {
        const std::string signature("AutoCAD Binary DXF\r\n\x1a\0", 22);
        if (bytes.size() < signature.size()
            || bytes.compare(0, signature.size(), signature) != 0)
            return false;
        std::istringstream stream(bytes, std::ios::binary);
        stream.seekg(static_cast<std::streamoff>(signature.size()));
        dxfReaderBinary reader(&stream);
        bool hatch = false;
        int code = 0;
        while (true) {
            const std::streampos recordStart = stream.tellg();
            if (!reader.readRec(&code)) break;
            if (code == 0) {
                hatch = reader.getString() == "HATCH";
            } else if (code == 47 && hatch) {
                const std::streamoff valueStart =
                    static_cast<std::streamoff>(recordStart)
                    + static_cast<std::streamoff>(2);
                if (valueStart < 0
                    || static_cast<std::size_t>(valueStart) > bytes.size()
                    || bytes.size() - static_cast<std::size_t>(valueStart) < 8u)
                    return false;
                bytes.replace(static_cast<std::size_t>(valueStart), 8u, 8u, '\0');
                ++replacements;
            }
        }
    } else {
        std::istringstream lines(bytes);
        std::ostringstream rewritten;
        std::string codeLine, valueLine;
        bool hatch = false;
        while (std::getline(lines, codeLine)) {
            if (!std::getline(lines, valueLine)) return false;
            int code = -1;
            try {
                code = std::stoi(codeLine);
            } catch (...) {
                return false;
            }
            std::string valueWithoutCr = valueLine;
            if (!valueWithoutCr.empty() && valueWithoutCr.back() == '\r')
                valueWithoutCr.pop_back();
            if (code == 0)
                hatch = valueWithoutCr == "HATCH";
            if (code == 47 && hatch) {
                const bool crlf = !valueLine.empty() && valueLine.back() == '\r';
                valueLine = crlf ? "0\r" : "0";
                ++replacements;
            }
            rewritten << codeLine << '\n' << valueLine << '\n';
        }
        if (lines.bad()) return false;
        bytes = rewritten.str();
    }
    if (replacements != 1u) return false;
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) return false;
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    output.close();
    return output.good();
}

bool verifyHatchPixelSize(const std::filesystem::path& path,
                          bool expectedPresent, double expectedValue,
                          int expectedLoopType, bool patterned) {
    std::map<std::string, Record> records;
    if (!readRecords(path, records) || records.size() != 5u
        || records.count("HATCH_CONTROL") != 1u
        || records.count("FOLLOWING_CONTROL") != 1u)
        return false;
    const Record& hatchRecord = records.at("HATCH_CONTROL");
    std::size_t pixelTags = 0;
    std::size_t pixelPosition = 0;
    std::size_t seedsPosition = hatchRecord.body.size();
    bool firstLoopSeen = false;
    bool patternLineSeen = false;
    std::size_t lastPatternLinePosition = 0;
    double observedPixelSize = 0.0;
    for (std::size_t i = 0; i < hatchRecord.body.size(); ++i) {
        const Tag& tag = hatchRecord.body[i];
        if (!firstLoopSeen && tag.first == 92) {
            firstLoopSeen = true;
            if (tag.second != std::to_string(expectedLoopType)) return false;
        }
        if (tag.first == 47) {
            ++pixelTags;
            pixelPosition = i;
            try {
                observedPixelSize = std::stod(tag.second);
            } catch (...) {
                return false;
            }
        }
        if (tag.first == 49) {
            patternLineSeen = true;
            lastPatternLinePosition = i;
        }
        if (tag.first == 98) seedsPosition = i;
    }
    const bool derivedBoundary = (expectedLoopType & 4) != 0;
    if (!firstLoopSeen || seedsPosition == hatchRecord.body.size()
        || pixelTags != ((expectedPresent || derivedBoundary) ? 1u : 0u))
        return false;
    if (expectedPresent
        && (pixelPosition >= seedsPosition || observedPixelSize != expectedValue))
        return false;
    if (patterned && (!patternLineSeen
        || (expectedPresent && pixelPosition <= lastPatternLinePosition)))
        return false;

    dx_data imported;
    dx_iface reader;
    if (!reader.fileImport(path.string(), &imported, false)
        || imported.mBlock->ent.size() != 5u)
        return false;
    const DRW_Hatch* hatch = nullptr;
    const DRW_Line* following = nullptr;
    for (const DRW_Entity* entity : imported.mBlock->ent) {
        if (entity->layer == "HATCH_CONTROL")
            hatch = dynamic_cast<const DRW_Hatch*>(entity);
        else if (entity->layer == "FOLLOWING_CONTROL")
            following = dynamic_cast<const DRW_Line*>(entity);
    }
    return hatch != nullptr && following != nullptr
        && hatch->pixelSize == expectedValue
        && hatch->solid == (patterned ? 0 : 1)
        && (!patterned || (hatch->name == "LOCAL_PHASE"
            && hatch->patternLines.size() == 1u
            && hatch->patternLines.front().angle == 30
            && hatch->patternLines.front().baseX == 2
            && hatch->patternLines.front().baseY == 3
            && hatch->patternLines.front().offsetX == 4
            && hatch->patternLines.front().offsetY == 7
            && hatch->patternLines.front().dashList
                == std::vector<double>{1, -2, 0}))
        && following->basePoint.x == 20 && following->basePoint.y == 21
        && following->basePoint.z == 22 && following->secPoint.x == 23
        && following->secPoint.y == 24 && following->secPoint.z == 25;
}

bool runHatchPixelPresenceTestsImpl(const std::filesystem::path& parent) {
    const auto directory = parent / ("libdxfrw-hatch-pixel-presence-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    std::size_t controls = 0;
    bool passed = true;
    for (bool binary : {false, true}) {
        for (int loopType : {3, 7}) {
            for (bool patterned : {false, true}) {
                const std::string stem = std::to_string(binary) + '-'
                    + std::to_string(loopType) + '-'
                    + std::to_string(patterned);
                const auto configure = [loopType, patterned](dx_data& data,
                                                             double pixelSize) {
                    auto* entity = static_cast<DRW_Hatch*>(data.mBlock->ent.front());
                    entity->looplist.front()->type = loopType;
                    entity->pixelSize = pixelSize;
                    if (patterned) {
                        entity->name = "LOCAL_PHASE";
                        entity->solid = 0;
                        entity->hpattern = 2;
                        entity->angle = 17;
                        entity->scale = 2.5;
                        entity->patternLines = {{30, 2, 3, 4, 7, {1, -2, 0}}};
                    }
                };
                auto source = drawing(true, true);
                configure(*source, 0.0);
                const auto absent = directory / (stem + "-absent.dxf");
                bool valid = write(absent, *source, binary, true, true)
                    && verifyHatchPixelSize(
                        absent, (loopType & 4) != 0, 0.0, loopType,
                        patterned);
                ++controls;

                source = drawing(true, true);
                configure(*source, 0.125);
                const auto nonzero = directory / (stem + "-nonzero.dxf");
                valid = write(nonzero, *source, binary, true, true)
                    && verifyHatchPixelSize(nonzero, true, 0.125, loopType, patterned)
                    && valid;
                ++controls;

                const auto explicitZero = directory / (stem + "-explicit-zero.dxf");
                std::error_code copyError;
                std::filesystem::copy_file(nonzero, explicitZero,
                    std::filesystem::copy_options::overwrite_existing, copyError);
                valid = !copyError
                    && rewriteHatchPixelSizeToZero(explicitZero, binary)
                    && verifyHatchPixelSize(explicitZero, true, 0.0, loopType, patterned)
                    && valid;
                ++controls;
                if (!valid)
                    std::cerr << "HATCH group-47 presence control failed: binary="
                              << binary << " loop-type=" << loopType
                              << " patterned=" << patterned << '\n';
                passed = valid && passed;
            }
        }
    }
    std::cout << "HATCH group-47 presence characterization: "
              << (passed ? "PASS" : "FAIL") << " (controls=" << controls
              << ", generated ASCII/Binary; no external fixtures)\n";
    if (passed) {
        std::error_code cleanupError;
        std::filesystem::remove_all(directory, cleanupError);
        return !cleanupError;
    }
    std::cout << "HATCH group-47 evidence: " << directory << '\n';
    return false;
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

bool verifyPatternedAssociativeHatch(const std::filesystem::path& path) {
    const std::vector<DRW_Hatch::PatternLine> expectedPatternLines{
        {30.0, 2.0, 3.0, 4.0, 7.0, {1.0, -2.0, 0.0}},
        {-45.0, -1.5, 5.25, 6.5, -7.75, {0.5, -0.25}}
    };
    std::map<std::string, Record> records;
    if (!readRecords(path, records) || records.size() != 5u
        || records.count("HATCH_CONTROL") != 1u
        || records.count("BOUNDARY_1") != 1u
        || records.count("BOUNDARY_2") != 1u
        || records.count("BOUNDARY_3") != 1u
        || records.count("FOLLOWING_CONTROL") != 1u)
        return false;

    const Record& hatchRecord = records.at("HATCH_CONTROL");
    if (hatchRecord.type != "HATCH" || hatchRecord.handle.empty()
        || hatchRecord.handle == "0" || hatchRecord.owner.empty())
        return false;
    std::set<std::string> recordHandles;
    for (const auto& entry : records) {
        if (entry.second.handle.empty() || entry.second.handle == "0"
            || entry.second.owner != hatchRecord.owner
            || !recordHandles.insert(entry.second.handle).second)
            return false;
    }

    std::vector<std::vector<std::string>> pathReferences;
    for (const Tag& tag : hatchRecord.body) {
        if (tag.first == 92)
            pathReferences.emplace_back();
        else if (tag.first == 330) {
            if (pathReferences.empty())
                return false;
            pathReferences.back().push_back(tag.second);
        }
    }
    const std::vector<std::vector<std::string>> expectedReferences{
        {records.at("BOUNDARY_1").handle},
        {records.at("BOUNDARY_2").handle, records.at("BOUNDARY_3").handle},
        {}
    };
    if (pathReferences != expectedReferences)
        return false;

    std::vector<Tag> expectedPatternTags{
        {75, "0"}, {76, "2"}, {52, number(22.5)}, {41, number(1.25)},
        {77, "1"}, {78, "2"}
    };
    for (const DRW_Hatch::PatternLine& line : expectedPatternLines) {
        expectedPatternTags.emplace_back(53, number(line.angle));
        expectedPatternTags.emplace_back(43, number(line.baseX));
        expectedPatternTags.emplace_back(44, number(line.baseY));
        expectedPatternTags.emplace_back(45, number(line.offsetX));
        expectedPatternTags.emplace_back(46, number(line.offsetY));
        expectedPatternTags.emplace_back(
            79, std::to_string(line.dashList.size()));
        for (double dash : line.dashList)
            expectedPatternTags.emplace_back(49, number(dash));
    }
    // The first boundary path has the derived bit, so DXF group 47 is
    // required even though this generated drawing uses pixelSize == 0.
    expectedPatternTags.emplace_back(47, number(0.0));
    expectedPatternTags.emplace_back(98, "0");
    const auto patternStart = std::find_if(
        hatchRecord.body.cbegin(), hatchRecord.body.cend(),
        [](const Tag& tag) { return tag.first == 75; });
    if (patternStart == hatchRecord.body.cend()
        || static_cast<std::size_t>(hatchRecord.body.cend() - patternStart)
            != expectedPatternTags.size()
        || !std::equal(expectedPatternTags.cbegin(), expectedPatternTags.cend(),
                       patternStart))
        return false;

    dx_data imported;
    dx_iface reader;
    if (!reader.fileImport(path.string(), &imported, false)
        || imported.mBlock->ent.size() != 5u)
        return false;
    const DRW_Hatch* hatch = nullptr;
    const DRW_Line* following = nullptr;
    std::map<std::string, std::uint32_t> typedHandles;
    for (const DRW_Entity* entity : imported.mBlock->ent) {
        typedHandles.emplace(entity->layer, entity->handle);
        if (entity->layer == "HATCH_CONTROL")
            hatch = dynamic_cast<const DRW_Hatch*>(entity);
        else if (entity->layer == "FOLLOWING_CONTROL")
            following = dynamic_cast<const DRW_Line*>(entity);
    }
    if (hatch == nullptr || following == nullptr
        || hatch->handle != std::stoul(hatchRecord.handle, nullptr, 16)
        || hatch->name != "LOCAL_PHASE" || hatch->solid != 0
        || hatch->associative != 1 || hatch->hpattern != 2
        || hatch->angle != 22.5 || hatch->scale != 1.25
        || hatch->doubleflag != 1 || hatch->deflines != 2
        || hatch->patternLines.size() != expectedPatternLines.size()
        || hatch->looplist.size() != 3u)
        return false;
    for (std::size_t index = 0u; index < expectedPatternLines.size(); ++index) {
        const DRW_Hatch::PatternLine& actual = hatch->patternLines[index];
        const DRW_Hatch::PatternLine& expected = expectedPatternLines[index];
        if (actual.angle != expected.angle || actual.baseX != expected.baseX
            || actual.baseY != expected.baseY
            || actual.offsetX != expected.offsetX
            || actual.offsetY != expected.offsetY
            || actual.dashList != expected.dashList)
            return false;
    }
    const std::array<std::vector<std::string>, 3> expectedBoundaryLayers{{
        {"BOUNDARY_1"}, {"BOUNDARY_2", "BOUNDARY_3"}, {}
    }};
    for (std::size_t index = 0u; index < hatch->looplist.size(); ++index) {
        if (!hatch->looplist[index]
            || hatch->looplist[index]->m_boundaryHandles.size()
                != expectedBoundaryLayers[index].size())
            return false;
        std::vector<std::uint32_t> expectedHandles;
        for (const std::string& layer : expectedBoundaryLayers[index]) {
            const auto found = typedHandles.find(layer);
            if (found == typedHandles.end())
                return false;
            expectedHandles.push_back(found->second);
        }
        if (hatch->looplist[index]->m_boundaryHandles != expectedHandles)
            return false;
    }
    const auto* spline = hatch->looplist[1]->objlist.empty()
        ? nullptr
        : dynamic_cast<const DRW_Spline*>(
              hatch->looplist[1]->objlist.front().get());
    return spline != nullptr && spline->degree == 2
        && spline->knotslist == knots && spline->weightlist == weights
        && following->basePoint.x == 20 && following->basePoint.y == 21
        && following->basePoint.z == 22 && following->secPoint.x == 23
        && following->secPoint.y == 24 && following->secPoint.z == 25;
}

bool runPatternedAssociativeHatchRoundTrips(
    const std::filesystem::path& directory) {
    bool passed = true;
    for (bool binary : {false, true}) {
        auto source = drawing(true, true);
        auto* hatch = static_cast<DRW_Hatch*>(source->mBlock->ent.front());
        hatch->name = "LOCAL_PHASE";
        hatch->solid = 0;
        hatch->hpattern = 2;
        hatch->angle = 22.5;
        hatch->scale = 1.25;
        hatch->doubleflag = 1;
        hatch->patternLines = {
            {30.0, 2.0, 3.0, 4.0, 7.0, {1.0, -2.0, 0.0}},
            {-45.0, -1.5, 5.25, 6.5, -7.75, {0.5, -0.25}}
        };
        const auto path = directory / (binary
            ? "patterned-associative-binary.dxf"
            : "patterned-associative-ascii.dxf");
        const bool exported = write(path, *source, binary, true, true);
        const bool verified = exported && verifyPatternedAssociativeHatch(path);
        if (!verified)
            std::cerr << "patterned associative multi-path HATCH failed: binary="
                      << binary << " exported=" << exported << '\n';
        passed = verified && passed;
    }
    return passed;
}

} // namespace

bool runHatchPixelPresenceTests(const std::filesystem::path& parent) {
    return runHatchPixelPresenceTestsImpl(parent);
}

bool writeHatchReferenceFile(const std::filesystem::path& path, bool binary) {
    auto data = drawing(true, true);
    return write(path, *data, binary, true, true) && verify(path, true, false);
}

bool verifyHatchReferenceFile(const std::filesystem::path& path) {
    return verify(path, true, false, false);
}

bool runHatchSameDataTests(const std::filesystem::path& parent) {
    const auto directory = parent / ("libdxfrw-hatch-same-data-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    const auto drawingWithChildren = []() {
        auto data = drawing(true, true);
        auto* block = new dx_ifaceBlock();
        block->name = "RetryBlock";
        block->handle = 0x70;
        block->parentHandle = 0x71;
        auto* primitive = new DRW_Line();
        primitive->handle = 0x72;
        primitive->secPoint = {1, 0, 0};
        block->ent.push_back(primitive);
        data->blocks.push_back(block);
        auto* poly = new DRW_Polyline();
        poly->handle = 0x80;
        poly->extPoint = {0, 0, 1};
        poly->vertexcount = 2;
        for (std::uint32_t handle : {0x81u, 0x82u}) {
            auto vertex = std::make_shared<DRW_Vertex>(handle == 0x81 ? 20 : 22, 2, 0, 0);
            vertex->handle = handle;
            poly->vertlist.push_back(vertex);
        }
        poly->setDwgSeqEndHandle(0x83);
        data->mBlock->ent.push_back(poly);
        auto* insert = new DRW_Insert();
        insert->handle = 0x90;
        insert->name = block->name;
        insert->seqendH.ref = 0x92;
        auto attribute = std::make_shared<DRW_Attrib>();
        attribute->handle = 0x91;
        attribute->height = 1;
        attribute->tag = "TAG";
        attribute->text = "VALUE";
        insert->attlist.push_back(attribute);
        data->mBlock->ent.push_back(insert);
        return data;
    };
    using Identities = std::vector<std::pair<const DRW_Entity*, std::uint32_t>>;
    const auto capture = [](const dx_data& data) {
        Identities identities;
        const auto remember = [&identities](const dx_ifaceBlock& block) {
            identities.emplace_back(&block, block.handle);
            for (const DRW_Entity* entity : block.ent) {
                identities.emplace_back(entity, entity->handle);
                if (const auto* poly = dynamic_cast<const DRW_Polyline*>(entity))
                    for (const auto& vertex : poly->vertlist)
                        identities.emplace_back(vertex.get(), vertex->handle);
                if (const auto* insert = dynamic_cast<const DRW_Insert*>(entity))
                    for (const auto& attribute : insert->attlist)
                        identities.emplace_back(attribute.get(), attribute->handle);
            }
        };
        remember(*data.mBlock);
        for (const auto* block : data.blocks) remember(*block);
        return identities;
    };
    const auto unchanged = [](const Identities& identities, const dx_data& data) {
        for (const auto& identity : identities)
            if (identity.first->handle != identity.second) return false;
        const auto* hatch = static_cast<const DRW_Hatch*>(data.mBlock->ent.front());
        const auto* poly = dynamic_cast<const DRW_Polyline*>(*std::next(data.mBlock->ent.begin(), 5));
        const auto* insert = dynamic_cast<const DRW_Insert*>(data.mBlock->ent.back());
        return hatch->associative == 1
            && hatch->looplist[0]->m_boundaryHandles == std::vector<std::uint32_t>{0x31}
            && hatch->looplist[1]->m_boundaryHandles == std::vector<std::uint32_t>({0x32, 0x33})
            && hatch->looplist[2]->m_boundaryHandles.empty()
            && poly != nullptr && poly->dwgSeqEndHandle() == 0x83
            && insert != nullptr && insert->seqendH.ref == 0x92
            && insert->attlist.front()->text == "VALUE";
    };
    class LateFailure final : public dx_iface {
    public:
        bool rejectFinal{false}, throwAfterEntities{false}, growCompound{false};
        void writeEntities() override {
            if (growCompound)
                for (DRW_Entity* entity : cData->mBlock->ent)
                    if (auto* poly = dynamic_cast<DRW_Polyline*>(entity))
                        poly->vertlist.resize(100, poly->vertlist.front());
            dx_iface::writeEntities();
            if (throwAfterEntities) throw std::runtime_error("local late HATCH callback failure");
        }
        bool finalizeDxfWrite() override {
            return dx_iface::finalizeDxfWrite() && !rejectFinal;
        }
    } adapter;
    bool passed = true;
    std::size_t successful = 0, rejected = 0;
    for (bool binary : {false, true}) {
        auto data = drawingWithChildren();
        const Identities identities = capture(*data);
        const auto first = directory / (binary ? "first-binary.dxf" : "first-ascii.dxf");
        const auto second = directory / (binary ? "second-binary.dxf" : "second-ascii.dxf");
        const auto accept = [&](const std::filesystem::path& path) {
            const bool exported = adapter.fileExport(path.string(), DRW::AC1027, binary, data.get(), false);
            const bool verified = exported && verify(path, true, false, false, 7);
            const bool sourceUnchanged = unchanged(identities, *data);
            const bool success = exported && verified && sourceUnchanged;
            if (!success) std::cerr << "HATCH same-data acceptance failed: exported=" << exported
                                    << " verified=" << verified << " identities-unchanged=" << sourceUnchanged << '\n';
            if (success) ++successful;
            passed = success && passed;
            return success;
        };
        accept(first);
        accept(second); // The same drawing, not a readback or fresh clone.
        std::map<std::string, Record> a, b;
        bool identical = readRecords(first, a) && readRecords(second, b) && a.size() == b.size();
        for (const auto& entry : a) {
            const auto found = b.find(entry.first);
            identical = found != b.end() && found->second.handle == entry.second.handle
                && found->second.owner == entry.second.owner
                && found->second.reactors == entry.second.reactors
                && found->second.body == entry.second.body && identical;
        }
        passed = identical && passed;
        for (int failure = 0; failure < 3; ++failure) {
            const auto path = directory / (std::to_string(binary) + '-' + std::to_string(failure) + ".dxf");
            const std::string sentinel = "same-data HATCH destination\n";
            { std::ofstream stream(path, std::ios::binary); stream << sentinel; }
            adapter.rejectFinal = failure == 0;
            adapter.throwAfterEntities = failure == 1;
            auto* boundary = static_cast<DRW_LWPolyline*>(*std::next(data->mBlock->ent.begin()));
            const double x = boundary->vertlist.front()->x;
            if (failure == 2) boundary->vertlist.front()->x = std::numeric_limits<double>::quiet_NaN();
            const bool failed = !adapter.fileExport(path.string(), DRW::AC1027, binary, data.get(), false);
            std::ifstream input(path, std::ios::binary);
            const std::string bytes((std::istreambuf_iterator<char>(input)), {});
            input.close();
            const bool valid = failed && bytes == sentinel && unchanged(identities, *data);
            if (valid) ++rejected;
            passed = valid && passed;
            adapter.rejectFinal = adapter.throwAfterEntities = false;
            boundary->vertlist.front()->x = x;
            accept(path); // Correct and retry the original data on the same adapter.
        }
        dx_iface::ExtensionGraphLimits limits;
        limits.maxSourceRecords = 100;
        adapter.setHatchGraphLimits(limits);
        adapter.growCompound = true;
        const std::string sentinel = "bounded identity snapshot\n";
        { std::ofstream stream(second, std::ios::binary); stream << sentinel; }
        const bool bounded = !adapter.fileExport(second.string(), DRW::AC1027, binary, data.get(), false)
            && adapter.hatchGraphFailure() == dx_iface::ExtensionGraphFailure::SourceRecordLimit
            && unchanged(identities, *data);
        std::ifstream input(second, std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(input)), {});
        input.close();
        if (bounded && bytes == sentinel) ++rejected;
        passed = bounded && bytes == sentinel && passed;
        adapter.growCompound = false;
        static_cast<DRW_Polyline*>(*std::next(data->mBlock->ent.begin(), 5))->vertlist.resize(2);
        adapter.setHatchGraphLimits({});
        accept(second);
        auto detached = drawing(true, false, true);
        detached->mBlock->ent.front()->handle = 0;
        const Identities detachedIds = capture(*detached);
        for (int pass = 0; pass < 2; ++pass) {
            bool success = adapter.fileExport(second.string(), DRW::AC1027, binary,
                                               detached.get(), false)
                && verify(second, false, true, false);
            for (const auto& identity : detachedIds)
                success = identity.first->handle == identity.second && success;
            if (success) ++successful;
            passed = success && passed;
        }
    }
    std::cout << "HATCH same-data identity/retry controls: " << (passed ? "PASS" : "FAIL")
              << " (successful=" << successful << ", rejected=" << rejected << ")\n";
    if (passed) std::filesystem::remove_all(directory);
    else std::cout << "HATCH same-data evidence: " << directory << '\n';
    return passed;
}

bool runHatchPublicationTests(const std::filesystem::path& parent) {
    const auto directory = parent / ("libdxfrw-hatch-publication-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    class FaultAdapter final : public dx_iface {
    public:
        int mode{0};
        void writeBlocks() override {
            if (mode != 9) { dx_iface::writeBlocks(); return; }
            // Emit the entities with their explicit named-block owners but
            // omit the owning BLOCK definition itself.
            for (const auto* block : cData->blocks)
                for (DRW_Entity* entity : block->ent) writeEntity(entity);
        }
        void writeEntities() override {
            if (mode == 9) return;
            DRW_Entity* boundary = nullptr;
            for (DRW_Entity* entity : cData->mBlock->ent)
                if (entity->layer == "BOUNDARY_1") boundary = entity;
            const std::uint32_t source = boundary != nullptr ? boundary->handle : 0;
            if (boundary != nullptr) {
                if (mode == 3) boundary->eType = DRW::UNKNOWN;
                if (mode == 4) boundary->space = DRW::PaperSpace;
                if (mode == 5) boundary->parentHandle = 0x777;
                if (mode == 6) boundary->handle = 0x777;
            }
            for (DRW_Entity* entity : cData->mBlock->ent) {
                if (mode == 1 && entity == boundary) continue;
                if (mode != 8) { writeEntity(entity); continue; }
                // Even a complete codec write is not adapter-dispatch proof.
                switch (entity->eType) {
                case DRW::HATCH: dxfW->writeHatch(static_cast<DRW_Hatch*>(entity)); break;
                case DRW::LWPOLYLINE: dxfW->writeLWPolyline(static_cast<DRW_LWPolyline*>(entity)); break;
                case DRW::SPLINE: dxfW->writeSpline(static_cast<DRW_Spline*>(entity)); break;
                case DRW::LINE: dxfW->writeLine(static_cast<DRW_Line*>(entity)); break;
                default: break;
                }
            }
            if (mode == 7 && boundary != nullptr) {
                boundary->handle = source;
                writeEntity(boundary);
            }
        }
    } adapter;
    bool passed = true;
    std::size_t rejected = 0, retries = 0, aliasedPublications = 0;
    for (bool binary : {false, true}) {
        for (int mode = 1; mode <= 11; ++mode) {
            auto data = drawing(true, true);
            DRW_Entity* boundary = nullptr;
            for (DRW_Entity* entity : data->mBlock->ent)
                if (entity->layer == "BOUNDARY_1") boundary = entity;
            if (mode == 2) boundary->eType = DRW::UNKNOWN;
            if (mode == 10)
                static_cast<DRW_LWPolyline*>(boundary)->vertlist.front()->x
                    = std::numeric_limits<double>::quiet_NaN();
            if (mode == 9) {
                auto* block = new dx_ifaceBlock();
                block->name = "PublicationBlock";
                block->handle = 0x80;
                block->parentHandle = 0x81;
                for (DRW_Entity* entity : data->mBlock->ent) entity->parentHandle = 0x81;
                block->ent.splice(block->ent.end(), data->mBlock->ent);
                data->blocks.push_back(block);
            }
            if (mode == 11) {
                data = std::make_unique<dx_data>();
                auto* hatch = new DRW_Hatch();
                hatch->handle = 0x30; // Same number preallocation will return.
                hatch->layer = "HATCH_CONTROL";
                hatch->name = "SOLID";
                hatch->associative = 1;
                auto loop = std::make_shared<DRW_HatchLoop>(3);
                loop->objlist.push_back(polyline(outer));
                loop->m_boundaryHandles = {0x31};
                loop->update();
                hatch->appendLoop(loop);
                hatch->loopsnum = 1;
                auto* contour = new DRW_LWPolyline(*polyline(outer));
                contour->handle = 0x31;
                contour->layer = "BOUNDARY_1";
                contour->reactorHandles = {0x30};
                auto* following = new DRW_Line();
                following->handle = 0x32;
                following->layer = "FOLLOWING_CONTROL";
                following->basePoint = {20, 21, 22};
                following->secPoint = {23, 24, 25};
                data->mBlock->ent = {hatch, contour, following};
                for (const char* name : {"HATCH_CONTROL", "BOUNDARY_1",
                                         "FOLLOWING_CONTROL"}) {
                    DRW_Layer layer;
                    layer.name = name;
                    data->layers.push_back(layer);
                }
            }
            const auto path = directory / (std::to_string(binary) + '-' + std::to_string(mode) + ".dxf");
            const std::string sentinel = "HATCH publication destination\n";
            { std::ofstream stream(path, std::ios::binary); stream << sentinel; }
            adapter.mode = mode;
            if (mode == 11) {
                std::vector<std::pair<const DRW_Entity*, std::uint32_t>> identities;
                for (const DRW_Entity* entity : data->mBlock->ent)
                    identities.emplace_back(entity, entity->handle);
                const bool success = adapter.fileExport(path.string(), DRW::AC1027,
                                                         binary, data.get(), false);
                bool identitiesRestored = true;
                for (const auto& identity : identities)
                    identitiesRestored = identitiesRestored
                        && identity.first->handle == identity.second;
                const bool valid = success && verifyAliasedHatchPublication(path)
                    && identitiesRestored;
                if (valid) ++aliasedPublications;
                else std::cerr << "HATCH publication alias mode failed: success="
                               << success << " graph-failure="
                               << static_cast<int>(adapter.hatchGraphFailure()) << '\n';
                passed = valid && passed;
                adapter.mode = 0;
                continue;
            }
            const bool success = adapter.fileExport(path.string(), DRW::AC1027,
                                                     binary, data.get(), false);
            std::ifstream input(path, std::ios::binary);
            const std::string bytes((std::istreambuf_iterator<char>(input)), {});
            input.close();
            const bool rejection = !success && bytes == sentinel
                && adapter.hatchGraphFailure() == dx_iface::ExtensionGraphFailure::InvalidGraph;
            if (rejection) ++rejected;
            else std::cerr << "HATCH publication mode " << mode
                           << " failed rejection gate: success=" << success
                           << " destination-preserved=" << (bytes == sentinel)
                           << " graph-failure=" << static_cast<int>(adapter.hatchGraphFailure()) << '\n';
            passed = rejection && passed;
            adapter.mode = 0;
            data = drawing(true, true);
            const bool retry = adapter.fileExport(path.string(), DRW::AC1027, binary, data.get(), false)
                && verify(path, true, false, false);
            if (retry) ++retries;
            passed = retry && passed;
        }
    }
    std::cout << "HATCH publication controls: " << (passed ? "PASS" : "FAIL")
              << " (rejected=" << rejected << ", fresh-data retries=" << retries
              << ", alias publications=" << aliasedPublications << ")\n";
    if (passed) std::filesystem::remove_all(directory);
    else std::cout << "HATCH publication evidence: " << directory << '\n';
    return passed;
}

bool runHatchAdapterTests(const std::filesystem::path& parent) {
    const auto directory = parent / ("libdxfrw-hatch-adapter-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    bool passed = true;
    std::size_t successful = 0, rejected = 0;
    const auto verifyTableReferences = [](const std::filesystem::path& path) {
        dx_data data;
        dx_iface reader;
        if (!reader.fileImport(path.string(), &data, false)) return false;
        std::uint32_t hatchHandle = 0;
        for (const DRW_Entity* entity : data.mBlock->ent)
            if (entity->layer == "HATCH_CONTROL") hatchHandle = entity->handle;
        bool layerFound = false, ltypeFound = false;
        for (const DRW_Layer& layer : data.layers)
            if (layer.name == "HATCH_CONTROL") {
                layerFound = layer.reactorHandles == std::vector<std::uint32_t>{hatchHandle};
            }
        for (const DRW_LType& ltype : data.lineTypes)
            if (ltype.name == "AuxPattern") {
                for (const DRW_Dictionary& dictionary : data.dictionaries)
                    if (dictionary.handle == ltype.xDictHandle
                        && dictionary.parentHandle == ltype.handle)
                        ltypeFound = true;
            }
        return hatchHandle != 0 && layerFound && ltypeFound;
    };
    for (bool binary : {false, true}) {
        for (bool first : {false, true}) {
            for (bool appData : {false, true}) {
                auto data = drawing(first, true);
                for (DRW_Layer& layer : data->layers)
                    if (layer.name == "HATCH_CONTROL") layer.reactorHandles = {0x40};
                DRW_LType ltype;
                ltype.handle = 0xB0;
                ltype.name = "AuxPattern";
                ltype.xDictHandle = 0xA1;
                data->lineTypes.push_back(ltype);
                DRW_Dictionary dictionary;
                dictionary.handle = 0xA1;
                dictionary.parentHandle = ltype.handle;
                data->dictionaries.push_back(dictionary);
                if (appData) {
                    for (DRW_Entity* entity : data->mBlock->ent)
                        if (entity->layer.rfind("BOUNDARY_", 0) == 0) {
                            entity->appData.push_back({
                                DRW_Variant(102, std::string("{ACAD_REACTORS")),
                                DRW_Variant(330, std::string("40")),
                                DRW_Variant(102, std::string("}"))});
                        }
                }
                const std::string key = std::to_string(binary) + '-' + std::to_string(first)
                    + '-' + std::to_string(appData);
                const auto path = directory / (key + ".dxf");
                dx_iface adapter;
                bool success = adapter.fileExport(path.string(), DRW::AC1027, binary,
                                                  data.get(), false)
                    && verify(path, true, false, false) && verifyTableReferences(path);
                if (success) ++successful;
                passed = success && passed;
                dx_data imported;
                const auto second = directory / (key + "-second.dxf");
                success = adapter.fileImport(path.string(), &imported, false)
                    && adapter.fileExport(second.string(), DRW::AC1027, binary, &imported, false)
                    && verify(second, true, false, false) && verifyTableReferences(second);
                if (success) ++successful;
                passed = success && passed;
            }
        }
        class RejectPlanning final : public dx_iface {
        public:
            bool rejectPlan{true};
            void writeHeader(DRW_Header& header) override {
                if (rejectPlan) dxfW->markSourceHandleAmbiguous(0x40);
                dx_iface::writeHeader(header);
            }
        } adapter;
        const auto path = directory / (binary ? "planning-binary.dxf" : "planning-ascii.dxf");
        const std::string sentinel = "planned HATCH destination\n";
        { std::ofstream stream(path, std::ios::binary); stream << sentinel; }
        auto data = drawing(true, true);
        const bool failed = !adapter.fileExport(path.string(), DRW::AC1027, binary, data.get(), false);
        std::ifstream input(path, std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(input)), {});
        input.close();
        if (failed && bytes == sentinel) ++rejected;
        passed = failed && bytes == sentinel && passed;
        adapter.rejectPlan = false;
        data = drawing(true, true);
        const bool retry = adapter.fileExport(path.string(), DRW::AC1027, binary, data.get(), false)
            && verify(path, true, false, false);
        if (retry) ++successful;
        passed = retry && passed;
    }
    std::cout << "HATCH adapter planning/reference controls: " << (passed ? "PASS" : "FAIL")
              << " (successful=" << successful << ", rejected=" << rejected << ")\n";
    if (passed) std::filesystem::remove_all(directory);
    else std::cout << "HATCH adapter evidence: " << directory << '\n';
    return passed;
}

bool runHatchAdmissionTests(const std::filesystem::path& parent) {
    const auto directory = parent / ("libdxfrw-hatch-admission-" + std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    const auto find = [](dx_data& data, const char* layer) -> DRW_Entity* {
        for (DRW_Entity* entity : data.mBlock->ent)
            if (entity->layer == layer)
                return entity;
        return nullptr;
    };
    const auto hatchOf = [&find](dx_data& data) {
        return static_cast<DRW_Hatch*>(find(data, "HATCH_CONTROL"));
    };
    bool passed = true;
    std::size_t accepted = 0, rejected = 0;
    for (bool binary : {false, true}) {
        const auto path = directory / (binary ? "binary.dxf" : "ascii.dxf");
        dx_iface iface;
        const auto accept = [&](dx_data& data, bool expectMpolygon = false) {
            const bool success = iface.fileExport(path.string(), DRW::AC1027, binary, &data, false);
            if (success) ++accepted;
            std::map<std::string, Record> records;
            bool ownersMatch = success && readRecords(path, records) && records.size() == 5;
            if (ownersMatch) {
                const std::string owner = records.at("HATCH_CONTROL").owner;
                ownersMatch = !owner.empty() && owner != "0";
                for (const char* layer : {"BOUNDARY_1", "BOUNDARY_2", "BOUNDARY_3"})
                    ownersMatch = records.at(layer).owner == owner && ownersMatch;
            }
            const bool typeMatches = ownersMatch
                && records.at("HATCH_CONTROL").type
                    == (expectMpolygon ? "MPOLYGON" : "HATCH");
            if (!success || !ownersMatch || !typeMatches ||
                iface.hatchGraphFailure() !=
                    dx_iface::ExtensionGraphFailure::None) {
                std::cerr << "HATCH admission accepted-case mismatch: binary="
                          << binary << " mpolygon=" << expectMpolygon
                          << " success=" << success
                          << " owners=" << ownersMatch
                          << " type=" << typeMatches
                          << " failure="
                          << static_cast<int>(iface.hatchGraphFailure())
                          << " records=" << records.size() << '\n';
                for (const auto &entry : records)
                    std::cerr << "  " << entry.first << " type="
                              << entry.second.type << " owner="
                              << entry.second.owner << " layer="
                              << entry.second.layer << '\n';
            }
            passed = success && ownersMatch
                && typeMatches
                && iface.hatchGraphFailure() == dx_iface::ExtensionGraphFailure::None && passed;
            return success;
        };
        auto baseline = drawing(false, false);
        accept(*baseline);
        const auto stats = iface.hatchGraphStats();
        if (!(stats.visitedNodes == 4 && stats.stagedHandles == 4
              && stats.examinedEdges == 3 && stats.sourceRecords > 0)) {
            std::cerr << "HATCH admission baseline stats mismatch: binary="
                      << binary << " nodes=" << stats.visitedNodes
                      << " handles=" << stats.stagedHandles
                      << " edges=" << stats.examinedEdges
                      << " sources=" << stats.sourceRecords << '\n';
        }
        passed = stats.visitedNodes == 4 && stats.stagedHandles == 4
            && stats.examinedEdges == 3 && stats.sourceRecords > 0 && passed;

        // MPOLYGON shares the boundary-reference graph, despite group 71
        // carrying its solid-fill state rather than HATCH associativity.
        auto polygon = drawing(false, false, false, true);
        passed = accept(*polygon, true) && passed;
        const auto polygonStats = iface.hatchGraphStats();
        if (!(polygonStats.visitedNodes == 4
              && polygonStats.stagedHandles == 4
              && polygonStats.examinedEdges == 3)) {
            std::cerr << "HATCH admission MPOLYGON stats mismatch: binary="
                      << binary << " nodes=" << polygonStats.visitedNodes
                      << " handles=" << polygonStats.stagedHandles
                      << " edges=" << polygonStats.examinedEdges
                      << " sources=" << polygonStats.sourceRecords << '\n';
        }
        passed = polygonStats.visitedNodes == 4
            && polygonStats.stagedHandles == 4
            && polygonStats.examinedEdges == 3 && passed;

        // Explicit owners are source identities, not output fixed literals.
        for (bool paper : {false, true}) {
            auto data = drawing(false, false);
            DRW_Block_Record record;
            record.handle = 0xA0;
            record.name = paper ? "*Paper_Space" : "*Model_Space";
            data->blockRecords.push_back(record);
            for (DRW_Entity* entity : data->mBlock->ent) {
                entity->parentHandle = record.handle;
                entity->space = paper ? DRW::PaperSpace : DRW::ModelSpace;
            }
            accept(*data);
        }
        for (const char* name : {"BoundaryBlock", "*Paper_Space0", "*Model_Space"}) {
            auto data = drawing(false, false);
            auto* block = new dx_ifaceBlock();
            block->name = name;
            block->handle = 0x80;
            block->parentHandle = 0x81;
            block->ent.splice(block->ent.end(), data->mBlock->ent);
            data->blocks.push_back(block);
            accept(*data);
        }
        const auto reject = [&](dx_data& data, dx_iface::ExtensionGraphFailure reason) {
            const std::string sentinel = "HATCH admission destination\n";
            { std::ofstream stream(path, std::ios::binary); stream << sentinel; }
            const auto* hatch = hatchOf(data);
            const std::uint32_t source = hatch->handle;
            const auto references = hatch->looplist.front()->m_boundaryHandles;
            std::vector<std::pair<const DRW_Entity*, std::uint32_t>> identities;
            const auto remember = [&identities](const dx_ifaceBlock& block) {
                for (const DRW_Entity* entity : block.ent) {
                    identities.emplace_back(entity, entity->handle);
                    if (const auto* poly = dynamic_cast<const DRW_Polyline*>(entity))
                        for (const auto& vertex : poly->vertlist)
                            identities.emplace_back(vertex.get(), vertex->handle);
                    if (const auto* insert = dynamic_cast<const DRW_Insert*>(entity))
                        for (const auto& attribute : insert->attlist)
                            identities.emplace_back(attribute.get(), attribute->handle);
                }
            };
            remember(*data.mBlock);
            for (const auto* block : data.blocks) remember(*block);
            const bool success = iface.fileExport(path.string(), DRW::AC1027, binary, &data, false);
            std::ifstream input(path, std::ios::binary);
            const std::string bytes((std::istreambuf_iterator<char>(input)), {});
            bool identitiesMatch = true;
            for (const auto& identity : identities)
                identitiesMatch = identity.first->handle == identity.second && identitiesMatch;
            const bool valid = !success && bytes == sentinel && identitiesMatch
                && iface.hatchGraphFailure() == reason && hatch->handle == source
                && hatch->looplist.front()->m_boundaryHandles == references;
            if (valid) ++rejected;
            passed = valid && passed;
            return valid;
        };
        for (bool asMpolygon : {false, true}) {
          for (int mutation = 0; mutation < 16; ++mutation) {
            auto data = drawing(false, false, false, asMpolygon);
            DRW_Hatch* hatch = hatchOf(*data);
            switch (mutation) {
            case 0: hatch->looplist[0]->m_boundaryHandles = {0x777}; break;
            case 1: hatch->looplist[0]->m_boundaryHandles = {0}; break;
            case 2: hatch->looplist[0]->m_boundaryHandles = {hatch->handle}; break;
            case 3: find(*data, "BOUNDARY_1")->space = DRW::PaperSpace; break;
            case 4: find(*data, "BOUNDARY_1")->parentHandle = 0x777; break;
            case 5: {
                auto* duplicate = new DRW_Line();
                duplicate->handle = 0x31;
                data->mBlock->ent.push_back(duplicate);
                break;
            }
            case 6: data->layers.front().handle = 0x31; break;
            case 7: {
                auto* poly = new DRW_Polyline();
                poly->handle = 0x90;
                auto vertex = std::make_shared<DRW_Vertex>();
                vertex->handle = 0x31;
                poly->vertlist.push_back(vertex);
                data->mBlock->ent.push_back(poly);
                break;
            }
            case 8: {
                auto* insert = new DRW_Insert();
                insert->handle = 0x90;
                auto attribute = std::make_shared<DRW_Attrib>();
                attribute->handle = 0x31;
                insert->attlist.push_back(attribute);
                data->mBlock->ent.push_back(insert);
                break;
            }
            case 9:
            case 10: {
                auto* block = new dx_ifaceBlock();
                block->name = mutation == 9 ? "OtherBlock" : "*Paper_Space1";
                block->handle = 0x80;
                block->parentHandle = 0x81;
                if (mutation == 10)
                    for (DRW_Entity* entity : data->mBlock->ent)
                        entity->space = DRW::PaperSpace;
                DRW_Entity* boundary = find(*data, "BOUNDARY_1");
                data->mBlock->ent.remove(boundary);
                block->ent.push_back(boundary);
                data->blocks.push_back(block);
                break;
            }
            case 11: {
                hatch->looplist[0]->m_boundaryHandles = {0x92};
                auto* poly = new DRW_Polyline();
                poly->handle = 0x90;
                auto vertex = std::make_shared<DRW_Vertex>();
                vertex->handle = 0x92;
                poly->vertlist.push_back(vertex);
                data->mBlock->ent.push_back(poly);
                break;
            }
            case 12: hatch->handle = 0; break;
            case 13: {
                DRW_Block_Record record;
                record.handle = 0x31;
                record.name = "*Model_Space";
                data->blockRecords.push_back(record);
                break;
            }
            case 14: {
                auto* poly = new DRW_Polyline();
                poly->handle = 0x90;
                poly->setDwgSeqEndHandle(0x31);
                data->mBlock->ent.push_back(poly);
                break;
            }
            case 15: {
                auto* insert = new DRW_Insert();
                insert->handle = 0x90;
                insert->seqendH.ref = 0x31;
                data->mBlock->ent.push_back(insert);
                break;
            }
            }
            reject(*data, dx_iface::ExtensionGraphFailure::InvalidGraph);
          }
        }
        auto absent = drawing(false, false);
        hatchOf(*absent)->looplist[0]->m_boundaryHandles = {0x777};
        const auto absentPath = directory / (binary ? "absent-binary.dxf" : "absent-ascii.dxf");
        const bool noPublication = !iface.fileExport(absentPath.string(), DRW::AC1027,
                                                     binary, absent.get(), false)
            && !std::filesystem::exists(absentPath)
            && iface.hatchGraphFailure() == dx_iface::ExtensionGraphFailure::InvalidGraph;
        if (noPublication) ++rejected;
        passed = noPublication && passed;
        auto absentPolygon = drawing(false, false, false, true);
        hatchOf(*absentPolygon)->looplist[0]->m_boundaryHandles = {0x777};
        const auto absentPolygonPath = directory
            / (binary ? "absent-mpolygon-binary.dxf"
                      : "absent-mpolygon-ascii.dxf");
        const std::string polygonSentinel = "MPOLYGON admission destination\n";
        { std::ofstream stream(absentPolygonPath, std::ios::binary);
          stream << polygonSentinel; }
        const bool polygonNoPublication = !iface.fileExport(
                absentPolygonPath.string(), DRW::AC1027, binary,
                absentPolygon.get(), false)
            && iface.hatchGraphFailure()
                == dx_iface::ExtensionGraphFailure::InvalidGraph;
        std::ifstream polygonInput(absentPolygonPath, std::ios::binary);
        const std::string polygonBytes(
            (std::istreambuf_iterator<char>(polygonInput)), {});
        passed = polygonNoPublication && polygonBytes == polygonSentinel
            && passed;
        if (polygonNoPublication && polygonBytes == polygonSentinel)
            ++rejected;
        // The same caller data survives an early rejection and is usable after
        // correcting the reference. Successful export handle restoration is a
        // separate later adapter slice, not asserted by this admission test.
        auto retry = drawing(false, false);
        hatchOf(*retry)->looplist[0]->m_boundaryHandles = {0x777};
        reject(*retry, dx_iface::ExtensionGraphFailure::InvalidGraph);
        hatchOf(*retry)->looplist[0]->m_boundaryHandles = {0x31};
        accept(*retry);

        dx_iface::ExtensionGraphLimits exact;
        exact.maxNodes = exact.maxStagedHandles = 4;
        exact.maxEdges = 3;
        exact.maxSourceRecords = stats.sourceRecords;
        iface.setHatchGraphLimits(exact);
        auto data = drawing(false, false);
        accept(*data);
        for (int budget = 0; budget < 4; ++budget) {
            auto limits = exact;
            auto reason = dx_iface::ExtensionGraphFailure::NodeLimit;
            if (budget == 0) --limits.maxNodes;
            if (budget == 1) { --limits.maxEdges; reason = dx_iface::ExtensionGraphFailure::EdgeLimit; }
            if (budget == 2) { --limits.maxStagedHandles; reason = dx_iface::ExtensionGraphFailure::StagedHandleLimit; }
            if (budget == 3) { --limits.maxSourceRecords; reason = dx_iface::ExtensionGraphFailure::SourceRecordLimit; }
            iface.setHatchGraphLimits(limits);
            data = drawing(false, false);
            reject(*data, reason);
        }
        iface.setHatchGraphLimits({});
        data = drawing(false, false, true);
        accept(*data);
        if (iface.hatchGraphStats().visitedNodes != 0)
            std::cerr << "HATCH admission detached graph stats mismatch: binary="
                      << binary << " nodes="
                      << iface.hatchGraphStats().visitedNodes << '\n';
        passed = iface.hatchGraphStats().visitedNodes == 0 && passed;
    }
    std::cout << "HATCH graph admission controls: " << (passed ? "PASS" : "FAIL")
              << " (accepted=" << accepted << ", rejected=" << rejected << ")\n";
    if (passed) std::filesystem::remove_all(directory);
    else std::cout << "HATCH admission evidence: " << directory << '\n';
    return passed;
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
                auto source = drawing(first, true);
                passed = write(path, *source, binary, true, resolver)
                    && verify(path, true, false) && passed;
                dx_data imported;
                dx_iface reader;
                const auto second = directory / (path.stem().string() + "-second.dxf");
                passed = reader.fileImport(path.string(), &imported, false)
                    && write(second, imported, binary, true, resolver)
                    && verify(second, true, false) && passed;
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
    const bool patternedAssociative =
        runPatternedAssociativeHatchRoundTrips(directory);
    if (!patternedAssociative)
        std::cerr << "patterned associative multi-path HATCH ASCII/Binary round-trip failed\n";
    passed = patternedAssociative && passed;
    std::cout << "HATCH reference/path writer controls: " << (passed ? "PASS" : "FAIL") << '\n';
    if (passed && !keepOutputs) std::filesystem::remove_all(directory);
    else std::cout << "HATCH reference evidence: " << directory << '\n';
    return passed;
}
