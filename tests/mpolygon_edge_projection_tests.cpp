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
    void writeEntities() override { accepted = writer->writeMPolygon(&polygon); }
    dxfRW* writer = nullptr;
    DRW_MPolygon polygon;
    bool accepted = false;
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
    return ok ? 0 : 1;
}
