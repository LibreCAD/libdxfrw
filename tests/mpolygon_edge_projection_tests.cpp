// SPDX-License-Identifier: GPL-2.0-or-later
// Generated MPOLYGON line/arc projection and fallback controls.
#include "lc3_compat_check.cpp"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

namespace {
const char* peerOutput = nullptr;
struct ArcProfile {
    double start, end, expectedBulge;
};
class Emitter final : public LC3CompatCheck {
public:
    Emitter(bool ccw, bool gap, const ArcProfile& profile) {
        polygon.dxfVersion = 1;
        polygon.solid = 1;
        polygon.name = "SOLID";
        auto loop = std::make_shared<DRW_HatchLoop>(0);
        loop->m_boundaryHandles.push_back(0x50u);
        auto line = std::make_shared<DRW_Line>();
        const double start = ccw ? profile.start : -profile.start;
        const double end = ccw ? profile.end : -profile.end;
        line->basePoint = {5.0 + 5.0 * std::cos(end), 5.0 * std::sin(end), 0.0};
        line->secPoint = {5.0 + 5.0 * std::cos(start) - (gap ? 1.0 : 0.0),
                          5.0 * std::sin(start), 0.0};
        auto arc = std::make_shared<DRW_Arc>();
        arc->basePoint = {5.0, 0.0, 0.0};
        arc->radious = 5.0;
        arc->staangle = profile.start;
        arc->endangle = profile.end;
        arc->isccw = ccw ? 1 : 0;
        loop->objlist.push_back(line);
        loop->objlist.push_back(arc);
        polygon.looplist.push_back(loop);
    }
    void writeEntities() override {
        if (policyDuringCallback >= 0)
            writer->setMPolygonPolylinePathsOnly(policyDuringCallback != 0);
        accepted = writer->writeMPolygon(&polygon);
    }
    dxfRW* writer = nullptr;
    DRW_MPolygon polygon;
    bool accepted = false;
    int policyDuringCallback = -1;
};

class Capture final : public LC3CompatCheck {
public:
    void addMPolygon(const DRW_MPolygon* value) override {
        if (value) { polygon = *value; ++count; }
    }
    DRW_MPolygon polygon;
    int count = 0;
};

bool run(bool binary, bool ccw, bool gap, DRW::Version version,
         const ArcProfile& profile) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-mpolygon-edges-" + std::to_string(nonce) + ".dxf");
    Emitter emitter(ccw, gap, profile);
    dxfRW output(path.string().c_str());
    emitter.writer = &output;
    auto* arc = static_cast<DRW_Arc*>(emitter.polygon.looplist[0]->objlist[1].get());
    {
        std::ofstream sentinel(path, std::ios::binary);
        sentinel << "prior output";
    }
    arc->radious = -1.0;
    bool ok = !output.write(&emitter, version, binary) && !emitter.accepted;
    {
        std::ifstream preserved(path, std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(preserved)),
                                std::istreambuf_iterator<char>());
        ok = ok && bytes == "prior output" && arc->radious == -1.0;
    }
    arc->radious = 5.0;
    ok = output.write(&emitter, version, binary) && emitter.accepted && ok;
    ok = ok && emitter.polygon.looplist.size() == 1
        && emitter.polygon.looplist[0]->type == 0
        && emitter.polygon.looplist[0]->objlist.size() == 2
        && emitter.polygon.looplist[0]->objlist[0]->eType == DRW::LINE
        && emitter.polygon.looplist[0]->objlist[1]->eType == DRW::ARC;
    Capture capture;
    dxfRW input(path.string().c_str());
    ok = input.read(&capture, false) && capture.count == 1 && ok;
    if (capture.polygon.looplist.size() != 1 || !capture.polygon.looplist[0])
        ok = false;
    else {
        const auto& loop = *capture.polygon.looplist[0];
        ok = ok && loop.m_boundaryHandles == std::vector<std::uint32_t>({0x50u});
        if (gap) {
            ok = ok && (loop.type & 2) == 0 && loop.objlist.size() == 2
                && loop.objlist[0]->eType == DRW::LINE
                && loop.objlist[1]->eType == DRW::ARC;
        } else if (loop.objlist.size() != 1) {
            ok = false;
        } else {
            const auto* polyline = dynamic_cast<const DRW_LWPolyline*>(loop.objlist[0].get());
            ok = ok && (loop.type & 2) != 0 && polyline && (polyline->flags & 1) != 0
                && polyline->vertlist.size() == 2 && polyline->vertlist[0]
                && polyline->vertlist[1]
                && std::abs(polyline->vertlist[0]->x - (5.0 + 5.0 * std::cos(profile.end))) < 1e-12
                && std::abs(polyline->vertlist[0]->y - 5.0 * std::sin(ccw ? profile.end : -profile.end)) < 1e-12
                && polyline->vertlist[0]->bulge == 0.0
                && std::abs(polyline->vertlist[1]->x - (5.0 + 5.0 * std::cos(profile.start))) < 1e-12
                && std::abs(polyline->vertlist[1]->y - 5.0 * std::sin(ccw ? profile.start : -profile.start)) < 1e-12
                && std::abs(polyline->vertlist[1]->bulge
                    - (ccw ? profile.expectedBulge : -profile.expectedBulge)) < 1e-12;
        }
    }
    std::error_code ignored;
    if (peerOutput && version == DRW::AC1021 && !gap) {
        const auto name = std::string(binary ? "binary-" : "ascii-")
            + (ccw ? "ccw-" : "cw-")
            + std::to_string(static_cast<int>(profile.start * 1000)) + "-"
            + std::to_string(static_cast<int>(profile.end * 1000)) + ".dxf";
        const auto target = std::filesystem::path(peerOutput) / name;
        std::filesystem::copy_file(path, target,
            std::filesystem::copy_options::overwrite_existing, ignored);
        ok = !ignored && ok;
        ignored.clear();
    }
    std::filesystem::remove(path, ignored);
    if (!ok) std::cerr << "MPOLYGON edge projection failed: binary=" << binary
        << " ccw=" << ccw << " gap=" << gap << " version=" << static_cast<int>(version) << '\n';
    return ok;
}

// mode 0: a real gap at a large coordinate; mode 1: a small arc at an
// unrepresentably large coordinate; mode 2: an ordinarily translated arc.
bool runPrecisionProfile(bool binary, int mode) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-mpolygon-precision-" + std::to_string(nonce) + ".dxf");
    const ArcProfile profile{0.0, M_PI / 2.0, std::sqrt(2.0) - 1.0};
    Emitter emitter(true, false, profile);
    auto* line = static_cast<DRW_Line*>(emitter.polygon.looplist[0]->objlist[0].get());
    auto* arc = static_cast<DRW_Arc*>(emitter.polygon.looplist[0]->objlist[1].get());
    const double shift = mode == 1 ? 1e16 : mode == 2 ? 1e6 : 1e12;
    line->basePoint.x += shift;
    line->secPoint.x += shift;
    arc->basePoint.x += shift;
    if (mode == 0)
        line->secPoint.x += 0.01;
    const double expectedEnd = line->secPoint.x;
    const double expectedCenter = arc->basePoint.x;
    dxfRW output(path.string().c_str());
    emitter.writer = &output;
    bool ok = output.write(&emitter, DRW::AC1021, binary) && emitter.accepted;
    Capture capture;
    dxfRW input(path.string().c_str());
    ok = input.read(&capture, false) && capture.count == 1 && ok;
    if (capture.polygon.looplist.size() != 1 || !capture.polygon.looplist[0]) {
        ok = false;
    } else if (mode == 2) {
        const auto& loop = *capture.polygon.looplist[0];
        const auto* polyline = loop.objlist.size() == 1
            ? dynamic_cast<const DRW_LWPolyline*>(loop.objlist[0].get()) : nullptr;
        ok = ok && (loop.type & 2) != 0 && polyline
            && polyline->vertlist.size() == 2 && polyline->vertlist[1]
            && std::abs(polyline->vertlist[1]->bulge - profile.expectedBulge) < 1e-12;
    } else if (capture.polygon.looplist[0]->objlist.size() != 2) {
        ok = false;
    } else {
        const auto& loop = *capture.polygon.looplist[0];
        const auto* parsedLine = dynamic_cast<const DRW_Line*>(loop.objlist[0].get());
        const auto* parsedArc = dynamic_cast<const DRW_Arc*>(loop.objlist[1].get());
        ok = ok && (loop.type & 2) == 0 && parsedLine && parsedArc
            && parsedLine->secPoint.x == expectedEnd
            && parsedArc->basePoint.x == expectedCenter && parsedArc->radious == 5.0;
    }
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    if (!ok) std::cerr << "MPOLYGON precision fallback failed: binary=" << binary
        << " mode=" << mode << '\n';
    return ok;
}

bool runPolicy(bool binary) {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-mpolygon-policy-" + std::to_string(nonce) + ".dxf");
    const ArcProfile profile{0.0, M_PI, 1.0};
    Emitter emitter(true, true, profile);
    dxfRW writer(path.string().c_str());
    emitter.writer = &writer;
    bool ok = !writer.mPolygonPolylinePathsOnly();
    writer.setMPolygonPolylinePathsOnly(true);
    emitter.policyDuringCallback = 0;
    {
        std::ofstream previous(path, std::ios::binary);
        previous << "prior output";
    }
    ok = !writer.write(&emitter, DRW::AC1021, binary) && !emitter.accepted
        && !writer.mPolygonPolylinePathsOnly() && ok;
    {
        std::ifstream previous(path, std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(previous)),
                                std::istreambuf_iterator<char>());
        ok = ok && bytes == "prior output";
    }
    emitter.policyDuringCallback = 1;
    ok = writer.write(&emitter, DRW::AC1021, binary) && emitter.accepted
        && writer.mPolygonPolylinePathsOnly() && ok;
    Capture fallback;
    dxfRW fallbackReader(path.string().c_str());
    ok = fallbackReader.read(&fallback, false) && fallback.count == 1
        && fallback.polygon.looplist.size() == 1
        && fallback.polygon.looplist[0]
        && (fallback.polygon.looplist[0]->type & 2) == 0 && ok;
    emitter.policyDuringCallback = -1;
    ok = !writer.write(&emitter, DRW::AC1021, binary) && !emitter.accepted && ok;
    auto* line = static_cast<DRW_Line*>(emitter.polygon.looplist[0]->objlist[0].get());
    line->secPoint.x = 10.0;
    ok = writer.write(&emitter, DRW::AC1021, binary) && emitter.accepted && ok;
    Capture projected;
    dxfRW projectedReader(path.string().c_str());
    ok = projectedReader.read(&projected, false) && projected.count == 1
        && projected.polygon.looplist.size() == 1
        && projected.polygon.looplist[0]
        && (projected.polygon.looplist[0]->type & 2) != 0 && ok;
    auto polyline = std::make_shared<DRW_LWPolyline>();
    polyline->flags = 1;
    polyline->addVertex(DRW_Vertex2D(0.0, 0.0, 0.0));
    polyline->addVertex(DRW_Vertex2D(1.0, 0.0, 0.0));
    polyline->addVertex(DRW_Vertex2D(0.0, 1.0, 0.0));
    emitter.polygon.looplist[0]->type = 3;
    emitter.polygon.looplist[0]->objlist.clear();
    emitter.polygon.looplist[0]->objlist.push_back(polyline);
    ok = writer.write(&emitter, DRW::AC1021, binary) && emitter.accepted && ok;
    std::string validOutput;
    {
        std::ifstream file(path, std::ios::binary);
        validOutput.assign(std::istreambuf_iterator<char>(file),
                           std::istreambuf_iterator<char>());
    }
    polyline->vertlist.clear();
    ok = !writer.write(&emitter, DRW::AC1021, binary) && !emitter.accepted && ok;
    {
        std::ifstream file(path, std::ios::binary);
        const std::string unchanged((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
        ok = ok && unchanged == validOutput;
    }
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    if (!ok) std::cerr << "MPOLYGON polyline-only policy failed: binary=" << binary << '\n';
    return ok;
}
} // namespace

int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--peer-output") {
        peerOutput = argv[2];
        std::error_code error;
        std::filesystem::create_directories(peerOutput, error);
        if (error) return 1;
    } else if (argc != 1) {
        std::cerr << "usage: mpolygon_edge_projection_tests [--peer-output DIR]\n";
        return 1;
    }
    bool ok = true;
    const ArcProfile profiles[] = {
        {0.0, M_PI, 1.0},
        {0.0, M_PI / 2.0, std::sqrt(2.0) - 1.0},
        {0.0, 3.0 * M_PI / 2.0, std::sqrt(2.0) + 1.0},
        {3.0 * M_PI / 2.0, M_PI / 2.0, 1.0},
    };
    for (DRW::Version version : {DRW::AC1021, DRW::AC1032})
        for (bool binary : {false, true})
            for (bool ccw : {false, true})
                for (bool gap : {false, true})
                    for (const ArcProfile& profile : profiles)
                        ok = run(binary, ccw, gap, version, profile) && ok;
    for (bool binary : {false, true})
        for (int mode : {0, 1, 2})
            ok = runPrecisionProfile(binary, mode) && ok;
    for (bool binary : {false, true})
        ok = runPolicy(binary) && ok;
    return ok ? 0 : 1;
}
