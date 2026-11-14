// SPDX-License-Identifier: GPL-2.0-or-later
// Generated DXF entity writer version transaction controls.
#include "dx_iface.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace {
long allocationsBeforeFailure = -1;
std::size_t allocationAttempts = 0u;
bool allocationFailed = false;
bool allocationTracking = false;

bool failAllocation() noexcept {
    if (!allocationTracking || allocationsBeforeFailure < 0)
        return false;
    ++allocationAttempts;
    if (allocationsBeforeFailure == 0) {
        allocationsBeforeFailure = -1;
        allocationFailed = true;
        return true;
    }
    --allocationsBeforeFailure;
    return false;
}

void armAllocationFailure(long offset) noexcept {
    allocationsBeforeFailure = offset;
    allocationAttempts = 0u;
    allocationFailed = false;
    allocationTracking = true;
}

void disarmAllocationFailure() noexcept {
    allocationTracking = false;
    allocationsBeforeFailure = -1;
}

enum class Profile { Face, Line, Solid, Trace, Point, Circle, Arc, BasicLine, Ellipse, Text };

const char* profileName(Profile profile) {
    switch (profile) {
    case Profile::Face: return "3DFACE";
    case Profile::Line: return "3DLINE";
    case Profile::Solid: return "SOLID";
    case Profile::Trace: return "TRACE";
    case Profile::Point: return "POINT";
    case Profile::Circle: return "CIRCLE";
    case Profile::Arc: return "ARC";
    case Profile::BasicLine: return "LINE";
    case Profile::Ellipse: return "ELLIPSE";
    case Profile::Text: return "TEXT";
    }
    return "unknown";
}

DRW_3Dface makeFace() {
    DRW_3Dface face;
    face.handle = 0xD926u;
    face.basePoint = DRW_Coord{1.0, 2.0, 3.0};
    face.secPoint = DRW_Coord{4.0, 5.0, 6.0};
    face.thirdPoint = DRW_Coord{7.0, 8.0, 9.0};
    face.fourPoint = DRW_Coord{10.0, 11.0, 12.0};
    face.invisibleflag = DRW_3Dface::SecodEdge | DRW_3Dface::FourthEdge;
    return face;
}

DRW_3DLine makeLine() {
    DRW_3DLine line;
    line.handle = 0xD92Au;
    line.basePoint = DRW_Coord{1.25, -2.5, 3.75};
    line.secPoint = DRW_Coord{-4.5, 5.25, -6.75};
    line.extPoint = DRW_Coord{0.0, 1.0, 0.0};
    line.thickness = 2.5;
    return line;
}

DRW_Solid makeSolid() {
    DRW_Solid solid;
    solid.handle = 0xD92Bu;
    solid.basePoint = DRW_Coord{0.0, 0.0, 1.5};
    solid.secPoint = DRW_Coord{3.0, 0.0, 1.5};
    solid.thirdPoint = DRW_Coord{3.0, 2.0, 1.5};
    solid.fourPoint = DRW_Coord{0.0, 2.0, 1.5};
    solid.thickness = 0.5;
    solid.extPoint = DRW_Coord{0.0, 1.0, 0.0};
    return solid;
}

DRW_Trace makeTrace() {
    DRW_Trace trace;
    trace.handle = 0xD92Cu;
    trace.basePoint = DRW_Coord{0.0, 0.0, 2.0};
    trace.secPoint = DRW_Coord{3.0, 0.0, 2.0};
    trace.thirdPoint = DRW_Coord{3.0, 2.0, 2.0};
    trace.fourPoint = DRW_Coord{0.0, 2.0, 2.0};
    trace.thickness = 0.75;
    trace.extPoint = DRW_Coord{0.0, 1.0, 0.0};
    return trace;
}

DRW_Point makePoint() {
    DRW_Point point;
    point.handle = 0xD929u;
    point.basePoint = DRW_Coord{-2.0, 3.0, 4.0};
    point.thickness = 1.25;
    point.extPoint = DRW_Coord{0.0, 1.0, 0.0};
    point.xAxisAngle = 0.625;
    return point;
}

DRW_Circle makeCircle() {
    DRW_Circle circle;
    circle.handle = 0xD927u;
    circle.basePoint = DRW_Coord{7.0, -4.0, 2.0};
    circle.radious = 6.5;
    circle.thickness = 1.25;
    return circle;
}

DRW_Arc makeArc() {
    DRW_Arc arc;
    arc.handle = 0xD926u;
    arc.basePoint = DRW_Coord{-3.0, 4.0, 2.5};
    arc.radious = 4.75;
    arc.staangle = 0.25;
    arc.endangle = 2.75;
    arc.thickness = 0.5;
    return arc;
}

DRW_Line makeBasicLine() {
    DRW_Line line;
    line.handle = 0xD920u;
    line.basePoint = DRW_Coord{1.0, 2.0, 3.0};
    line.secPoint = DRW_Coord{-2.0, 4.0, 5.0};
    line.thickness = 0.75;
    return line;
}

DRW_Ellipse makeEllipse() {
    DRW_Ellipse ellipse;
    ellipse.handle = 0xD928u;
    ellipse.basePoint = DRW_Coord{2.0, -3.0, 4.0};
    ellipse.secPoint = DRW_Coord{4.0, 0.0, 0.0};
    ellipse.ratio = 0.5;
    ellipse.staparam = 0.25;
    ellipse.endparam = 2.5;
    return ellipse;
}

DRW_Text makeText() {
    DRW_Text text;
    text.handle = 0xD92Du;
    text.basePoint = DRW_Coord{12.0, -3.0, 1.0};
    text.secPoint = DRW_Coord{15.0, 6.0, 2.0};
    text.height = 2.0;
    text.text = "revision matrix text";
    text.angle = 15.0;
    text.widthscale = 1.2;
    text.oblique = 3.5;
    text.style = "MatrixStyle";
    text.textgen = 2;
    text.alignH = DRW_Text::HCenter;
    text.alignV = DRW_Text::VTop;
    return text;
}

bool sameText(const DRW_Text& a, const DRW_Text& b,
              bool compareHandle = true) {
    return (!compareHandle || a.handle == b.handle)
        && a.basePoint.x == b.basePoint.x && a.basePoint.y == b.basePoint.y
        && a.basePoint.z == b.basePoint.z && a.secPoint.x == b.secPoint.x
        && a.secPoint.y == b.secPoint.y && a.secPoint.z == b.secPoint.z
        && a.height == b.height && a.text == b.text && a.angle == b.angle
        && a.widthscale == b.widthscale && a.oblique == b.oblique
        && a.style == b.style && a.textgen == b.textgen
        && a.alignH == b.alignH && a.alignV == b.alignV;
}

bool sameEllipse(const DRW_Ellipse& a, const DRW_Ellipse& b,
                 bool compareHandle = true) {
    return (!compareHandle || a.handle == b.handle)
        && a.basePoint.x == b.basePoint.x && a.basePoint.y == b.basePoint.y
        && a.basePoint.z == b.basePoint.z && a.secPoint.x == b.secPoint.x
        && a.secPoint.y == b.secPoint.y && a.secPoint.z == b.secPoint.z
        && a.ratio == b.ratio && a.staparam == b.staparam
        && a.endparam == b.endparam && a.extPoint.x == b.extPoint.x
        && a.extPoint.y == b.extPoint.y && a.extPoint.z == b.extPoint.z
        && a.isccw == b.isccw;
}

bool sameBasicLine(const DRW_Line& a, const DRW_Line& b,
                   bool compareHandle = true) {
    return (!compareHandle || a.handle == b.handle)
        && a.basePoint.x == b.basePoint.x && a.basePoint.y == b.basePoint.y
        && a.basePoint.z == b.basePoint.z
        && a.secPoint.x == b.secPoint.x && a.secPoint.y == b.secPoint.y
        && a.secPoint.z == b.secPoint.z && a.thickness == b.thickness
        && a.extPoint.x == b.extPoint.x && a.extPoint.y == b.extPoint.y
        && a.extPoint.z == b.extPoint.z;
}

bool sameCircle(const DRW_Circle& a, const DRW_Circle& b,
                bool compareHandle = true) {
    return (!compareHandle || a.handle == b.handle)
        && a.basePoint.x == b.basePoint.x && a.basePoint.y == b.basePoint.y
        && a.basePoint.z == b.basePoint.z && a.radious == b.radious
        && a.thickness == b.thickness && a.extPoint.x == b.extPoint.x
        && a.extPoint.y == b.extPoint.y && a.extPoint.z == b.extPoint.z;
}

bool sameArc(const DRW_Arc& a, const DRW_Arc& b,
             bool compareHandle = true) {
    return sameCircle(a, b, compareHandle)
        && std::abs(a.staangle - b.staangle) < 1e-12
        && std::abs(a.endangle - b.endangle) < 1e-12;
}

bool samePoint(const DRW_Point& a, const DRW_Point& b,
               bool compareHandle = true) {
    return (!compareHandle || a.handle == b.handle)
        && a.basePoint.x == b.basePoint.x && a.basePoint.y == b.basePoint.y
        && a.basePoint.z == b.basePoint.z && a.thickness == b.thickness
        && a.extPoint.x == b.extPoint.x && a.extPoint.y == b.extPoint.y
        && a.extPoint.z == b.extPoint.z && a.xAxisAngle == b.xAxisAngle;
}

bool sameTrace(const DRW_Trace& a, const DRW_Trace& b,
               bool compareHandle = true) {
    return (!compareHandle || a.handle == b.handle)
        && a.basePoint.x == b.basePoint.x
        && a.basePoint.y == b.basePoint.y && a.basePoint.z == b.basePoint.z
        && a.secPoint.x == b.secPoint.x && a.secPoint.y == b.secPoint.y
        && a.secPoint.z == b.secPoint.z
        && a.thirdPoint.x == b.thirdPoint.x
        && a.thirdPoint.y == b.thirdPoint.y
        && a.thirdPoint.z == b.thirdPoint.z
        && a.fourPoint.x == b.fourPoint.x && a.fourPoint.y == b.fourPoint.y
        && a.fourPoint.z == b.fourPoint.z
        && a.extPoint.x == b.extPoint.x && a.extPoint.y == b.extPoint.y
        && a.extPoint.z == b.extPoint.z && a.thickness == b.thickness;
}

bool sameFace(const DRW_3Dface& a, const DRW_3Dface& b) {
    return a.handle == b.handle && a.basePoint.x == b.basePoint.x
        && a.basePoint.y == b.basePoint.y && a.basePoint.z == b.basePoint.z
        && a.secPoint.x == b.secPoint.x && a.secPoint.y == b.secPoint.y
        && a.secPoint.z == b.secPoint.z
        && a.thirdPoint.x == b.thirdPoint.x
        && a.thirdPoint.y == b.thirdPoint.y
        && a.thirdPoint.z == b.thirdPoint.z
        && a.fourPoint.x == b.fourPoint.x && a.fourPoint.y == b.fourPoint.y
        && a.fourPoint.z == b.fourPoint.z
        && a.invisibleflag == b.invisibleflag;
}

bool sameLine(const DRW_3DLine& a, const DRW_3DLine& b) {
    return a.handle == b.handle && a.basePoint.x == b.basePoint.x
        && a.basePoint.y == b.basePoint.y && a.basePoint.z == b.basePoint.z
        && a.secPoint.x == b.secPoint.x && a.secPoint.y == b.secPoint.y
        && a.secPoint.z == b.secPoint.z
        && a.extPoint.x == b.extPoint.x && a.extPoint.y == b.extPoint.y
        && a.extPoint.z == b.extPoint.z && a.thickness == b.thickness;
}

class Export final : public dx_iface {
public:
    Export(dxfRW& codec, Profile profile) : codec(codec), profile(profile) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &codec;
        face = makeFace();
        line = makeLine();
        solid = makeSolid();
        trace = makeTrace();
        point = makePoint();
        circle = makeCircle();
        arc = makeArc();
        basicLine = makeBasicLine();
        ellipse = makeEllipse();
        text = makeText();
        codec.setEllipseParts(8);
    }

    void writeEntities() override {
        struct TrackingScope {
            long& reached;
            ~TrackingScope() {
                reached = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
            }
        } trackingScope{reached};
        armAllocationFailure(failAt);
        switch (profile) {
        case Profile::Face: accepted = codec.write3dface(&face); break;
        case Profile::Line: accepted = codec.write3DLine(&line); break;
        case Profile::Solid: accepted = codec.writeSolid(&solid); break;
        case Profile::Trace: accepted = codec.writeTrace(&trace); break;
        case Profile::Point: accepted = codec.writePoint(&point); break;
        case Profile::Circle: accepted = codec.writeCircle(&circle); break;
        case Profile::Arc: accepted = codec.writeArc(&arc); break;
        case Profile::BasicLine: accepted = codec.writeLine(&basicLine); break;
        case Profile::Ellipse: accepted = codec.writeEllipse(&ellipse); break;
        case Profile::Text: accepted = codec.writeText(&text); break;
        }
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    Profile profile;
    dx_data storage;
    DRW_3Dface face;
    DRW_3DLine line;
    DRW_Solid solid;
    DRW_Trace trace;
    DRW_Point point;
    DRW_Circle circle;
    DRW_Arc arc;
    DRW_Line basicLine;
    DRW_Ellipse ellipse;
    DRW_Text text;
    long failAt = -1;
    long reached = 0;
    bool accepted = false;
};

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

bool readbackMatches(const std::filesystem::path& path, Profile profile,
                     DRW::Version expectedVersion) {
    dx_data data;
    dx_iface reader;
    if (!reader.fileImport(path.string(), &data, false)
        || data.sourceVersion != expectedVersion || data.mBlock == nullptr
        || data.mBlock->ent.size() != 1u) {
        std::cerr << "3D entity typed readback failed, profile="
                  << profileName(profile)
                  << "\n";
        return false;
    }
    const DRW_Entity* entity = data.mBlock->ent.front();
    if (profile == Profile::Face) {
        const auto* face = dynamic_cast<const DRW_3Dface*>(entity);
        const DRW_3Dface expected = makeFace();
        return face != nullptr && face->eType == DRW::E3DFACE
            && face->basePoint.x == expected.basePoint.x
            && face->basePoint.y == expected.basePoint.y
            && face->basePoint.z == expected.basePoint.z
            && face->secPoint.x == expected.secPoint.x
            && face->secPoint.y == expected.secPoint.y
            && face->secPoint.z == expected.secPoint.z
            && face->thirdPoint.x == expected.thirdPoint.x
            && face->thirdPoint.y == expected.thirdPoint.y
            && face->thirdPoint.z == expected.thirdPoint.z
            && face->fourPoint.x == expected.fourPoint.x
            && face->fourPoint.y == expected.fourPoint.y
            && face->fourPoint.z == expected.fourPoint.z
            && face->invisibleflag == expected.invisibleflag;
    }
    if (profile == Profile::Line) {
        const auto* line = dynamic_cast<const DRW_3DLine*>(entity);
        const DRW_3DLine expected = makeLine();
        return line != nullptr && line->eType == DRW::THREEDLINE
            && line->basePoint.x == expected.basePoint.x
            && line->basePoint.y == expected.basePoint.y
            && line->basePoint.z == expected.basePoint.z
            && line->secPoint.x == expected.secPoint.x
            && line->secPoint.y == expected.secPoint.y
            && line->secPoint.z == expected.secPoint.z
            && line->extPoint.x == expected.extPoint.x
            && line->extPoint.y == expected.extPoint.y
            && line->extPoint.z == expected.extPoint.z
            && line->thickness == expected.thickness;
    }
    if (profile == Profile::Point) {
        const auto* point = dynamic_cast<const DRW_Point*>(entity);
        const DRW_Point expected = makePoint();
        return point != nullptr && point->eType == DRW::POINT
            && point->basePoint.x == expected.basePoint.x
            && point->basePoint.y == expected.basePoint.y
            && point->basePoint.z == expected.basePoint.z
            && point->thickness == expected.thickness
            && point->extPoint.x == expected.extPoint.x
            && point->extPoint.y == expected.extPoint.y
            && point->extPoint.z == expected.extPoint.z
            && std::abs(point->xAxisAngle - expected.xAxisAngle) < 1e-12;
    }
    if (profile == Profile::Circle) {
        const auto* circle = dynamic_cast<const DRW_Circle*>(entity);
        const DRW_Circle expected = makeCircle();
        return circle != nullptr && circle->eType == DRW::CIRCLE
            && sameCircle(*circle, expected, false);
    }
    if (profile == Profile::Arc) {
        const auto* arc = dynamic_cast<const DRW_Arc*>(entity);
        const DRW_Arc expected = makeArc();
        return arc != nullptr && arc->eType == DRW::ARC
            && sameArc(*arc, expected, false);
    }
    if (profile == Profile::BasicLine) {
        const auto* line = dynamic_cast<const DRW_Line*>(entity);
        const DRW_Line expected = makeBasicLine();
        return line != nullptr && line->eType == DRW::LINE
            && sameBasicLine(*line, expected, false);
    }
    if (profile == Profile::Ellipse) {
        const DRW_Ellipse expected = makeEllipse();
        if (expectedVersion == DRW::AC1009) {
            const auto* polyline = dynamic_cast<const DRW_Polyline*>(entity);
            const bool matches = polyline != nullptr
                && polyline->eType == DRW::POLYLINE
                && polyline->vertlist.size() >= 2u
                && std::all_of(polyline->vertlist.begin(), polyline->vertlist.end(),
                    [](const std::shared_ptr<DRW_Vertex>& vertex) {
                        return vertex != nullptr && std::isfinite(vertex->basePoint.x)
                            && std::isfinite(vertex->basePoint.y)
                            && std::isfinite(vertex->basePoint.z);
                    });
            if (!matches)
                std::cerr << "R12 ELLIPSE downgrade typed polyline="
                          << (polyline != nullptr)
                          << " entity-type=" << static_cast<int>(entity->eType)
                          << " vertices=" << (polyline != nullptr
                                ? polyline->vertlist.size() : 0u)
                          << " vertex-count=" << (polyline != nullptr
                                ? polyline->vertexcount : -1) << '\n';
            return matches;
        }
        const auto* ellipse = dynamic_cast<const DRW_Ellipse*>(entity);
        return ellipse != nullptr && ellipse->eType == DRW::ELLIPSE
            && sameEllipse(*ellipse, expected, false);
    }
    if (profile == Profile::Text) {
        const auto* text = dynamic_cast<const DRW_Text*>(entity);
        DRW_Text expected = makeText();
        if (expectedVersion == DRW::AC1009)
            expected.style = "MATRIXSTYLE";
        return text != nullptr && text->eType == DRW::TEXT
            && sameText(*text, expected, false);
    }
    const auto* trace = dynamic_cast<const DRW_Trace*>(entity);
    const DRW_Trace expected = profile == Profile::Solid
        ? static_cast<const DRW_Trace&>(makeSolid()) : makeTrace();
    return trace != nullptr
        && trace->eType == (profile == Profile::Solid ? DRW::SOLID : DRW::DXF_TRACE)
        && sameTrace(*trace, expected, false);
}

bool runProfile(const std::filesystem::path& directory, Profile profile) {
    const auto fail = [profile](const char* message) {
        std::cerr << profileName(profile) << " transaction matrix: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    const std::array<std::pair<DRW::Version, const char*>, 7> faceVersions = {{
        {DRW::AC1014, "AC1014"}, {DRW::AC1015, "AC1015"},
        {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
        {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
        {DRW::AC1032, "AC1032"}}};
    const std::array<std::pair<DRW::Version, const char*>, 6> lineVersions = {{
        {DRW::AC1015, "AC1015"}, {DRW::AC1018, "AC1018"},
        {DRW::AC1021, "AC1021"}, {DRW::AC1024, "AC1024"},
        {DRW::AC1027, "AC1027"}, {DRW::AC1032, "AC1032"}}};
    const std::array<std::pair<DRW::Version, const char*>, 7> legacyVersions = {{
        {DRW::AC1014, "AC1014"}, {DRW::AC1015, "AC1015"},
        {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
        {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
        {DRW::AC1032, "AC1032"}}};
    const std::array<std::pair<DRW::Version, const char*>, 8> ellipseVersions = {{
        {DRW::AC1009, "AC1009"}, {DRW::AC1014, "AC1014"},
        {DRW::AC1015, "AC1015"}, {DRW::AC1018, "AC1018"},
        {DRW::AC1021, "AC1021"}, {DRW::AC1024, "AC1024"},
        {DRW::AC1027, "AC1027"}, {DRW::AC1032, "AC1032"}}};
    const std::array<std::pair<DRW::Version, const char*>, 8> textVersions = {{
        {DRW::AC1009, "AC1009"}, {DRW::AC1014, "AC1014"},
        {DRW::AC1015, "AC1015"}, {DRW::AC1018, "AC1018"},
        {DRW::AC1021, "AC1021"}, {DRW::AC1024, "AC1024"},
        {DRW::AC1027, "AC1027"}, {DRW::AC1032, "AC1032"}}};
    const auto runVersion = [&](DRW::Version target, const char* versionName) {
        for (const bool binary : {false, true}) {
            const std::string stem = std::string(profileName(profile)) + "-";
            const auto output = directory
                / (stem + versionName + (binary ? "-binary.dxf" : "-ascii.dxf"));
            std::string expectedBytes;
            long allocationCount = 0;
            {
                dxfRW codec(output.string().c_str());
                Export exporter(codec, profile);
                exporter.failAt = 1000000;
                const bool written = codec.write(&exporter, target, binary);
                if (!written || !exporter.accepted
                    || !readBytes(output, expectedBytes))
                    return fail("clean reference write failed");
                allocationCount = exporter.reached;
            }
            if (allocationCount <= 0)
                return fail("writer reported no allocation callbacks");

            const std::string sentinel = "prior 3D entity destination\n";
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior)
                        return fail("could not write destination sentinel");
                }
                dxfRW codec(output.string().c_str());
                Export exporter(codec, profile);
                const DRW_3Dface originalFace = exporter.face;
                const DRW_3DLine originalLine = exporter.line;
                const DRW_Solid originalSolid = exporter.solid;
                const DRW_Trace originalTrace = exporter.trace;
                const DRW_Point originalPoint = exporter.point;
                const DRW_Circle originalCircle = exporter.circle;
                const DRW_Arc originalArc = exporter.arc;
                const DRW_Line originalBasicLine = exporter.basicLine;
                const DRW_Ellipse originalEllipse = exporter.ellipse;
                const DRW_Text originalText = exporter.text;
                exporter.failAt = offset;
                const bool written = codec.write(&exporter, target, binary);
                bool callerPreserved = false;
                switch (profile) {
                case Profile::Face:
                    callerPreserved = sameFace(exporter.face, originalFace);
                    break;
                case Profile::Line:
                    callerPreserved = sameLine(exporter.line, originalLine);
                    break;
                case Profile::Solid:
                    callerPreserved = sameTrace(exporter.solid, originalSolid);
                    break;
                case Profile::Trace:
                    callerPreserved = sameTrace(exporter.trace, originalTrace);
                    break;
                case Profile::Point:
                    callerPreserved = samePoint(exporter.point, originalPoint);
                    break;
                case Profile::Circle:
                    callerPreserved = sameCircle(exporter.circle, originalCircle);
                    break;
                case Profile::Arc:
                    callerPreserved = sameArc(exporter.arc, originalArc);
                    break;
                case Profile::BasicLine:
                    callerPreserved = sameBasicLine(exporter.basicLine,
                                                    originalBasicLine);
                    break;
                case Profile::Ellipse:
                    callerPreserved = sameEllipse(exporter.ellipse,
                                                  originalEllipse);
                    break;
                case Profile::Text:
                    callerPreserved = sameText(exporter.text, originalText);
                    break;
                }
                if (!allocationFailed || written
                    || exporter.reached != offset + 1 || !callerPreserved) {
                    std::cerr << "offset=" << offset
                              << " attempts=" << exporter.reached
                              << " injected=" << allocationFailed
                              << " written=" << written
                              << " accepted=" << exporter.accepted
                              << " callerPreserved=" << callerPreserved
                              << '\n';
                    return fail("allocation failure did not roll back caller state");
                }
                std::string afterFailure;
                if (!readBytes(output, afterFailure) || afterFailure != sentinel)
                    return fail("allocation failure replaced prior destination");

                exporter.failAt = -1;
                const bool retried = codec.write(&exporter, target, binary);
                std::string retryBytes;
                const bool retryCallerPreserved = profile == Profile::Face
                    ? sameFace(exporter.face, originalFace)
                    : profile == Profile::Line
                        ? sameLine(exporter.line, originalLine)
                        : profile == Profile::Solid
                            ? sameTrace(exporter.solid, originalSolid)
                            : profile == Profile::Trace
                                ? sameTrace(exporter.trace, originalTrace)
                                : profile == Profile::Point
                                    ? samePoint(exporter.point, originalPoint)
                                    : profile == Profile::Circle
                                        ? sameCircle(exporter.circle, originalCircle)
                                        : profile == Profile::Arc
                                            ? sameArc(exporter.arc, originalArc)
                                            : profile == Profile::BasicLine
                                                ? sameBasicLine(exporter.basicLine,
                                                    originalBasicLine)
                                            : profile == Profile::Ellipse
                                                ? sameEllipse(exporter.ellipse,
                                                    originalEllipse)
                                                : sameText(exporter.text,
                                                    originalText);
                const bool retryReadback = readbackMatches(output, profile,
                                                            target);
                if (!retried || !exporter.accepted
                    || !readBytes(output, retryBytes)
                    || retryBytes != expectedBytes
                    || !retryReadback
                    || !retryCallerPreserved)
                {
                    std::cerr << profileName(profile) << ' ' << versionName
                              << ' ' << (binary ? "Binary" : "ASCII")
                              << " retry=" << retried
                              << " callback=" << exporter.accepted
                              << " readback=" << retryReadback
                              << " bytes=" << (retryBytes == expectedBytes)
                              << " caller=" << retryCallerPreserved << '\n';
                    return fail("same-codec retry or typed readback failed");
                }
            }
            std::cout << profileName(profile) << ' '
                      << versionName << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
        return true;
    };

    if (profile == Profile::Face) {
        for (const auto& version : faceVersions)
            if (!runVersion(version.first, version.second))
                return false;
    } else if (profile == Profile::Line) {
        for (const auto& version : lineVersions)
            if (!runVersion(version.first, version.second))
                return false;
    } else if (profile == Profile::Ellipse) {
        for (const auto& version : ellipseVersions)
            if (!runVersion(version.first, version.second))
                return false;
    } else if (profile == Profile::Text) {
        for (const auto& version : textVersions)
            if (!runVersion(version.first, version.second))
                return false;
    } else {
        for (const auto& version : legacyVersions)
            if (!runVersion(version.first, version.second))
                return false;
    }
    return true;
}

bool run(const std::filesystem::path& directory) {
    return runProfile(directory, Profile::Face)
        && runProfile(directory, Profile::Line)
        && runProfile(directory, Profile::Solid)
        && runProfile(directory, Profile::Trace)
        && runProfile(directory, Profile::Point)
        && runProfile(directory, Profile::Circle)
        && runProfile(directory, Profile::Arc)
        && runProfile(directory, Profile::BasicLine)
        && runProfile(directory, Profile::Ellipse)
        && runProfile(directory, Profile::Text);
}
} // namespace

void* operator new(std::size_t size) {
    if (failAllocation())
        throw std::bad_alloc();
    if (void* value = std::malloc(size == 0u ? 1u : size))
        return value;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete(void* value, std::size_t) noexcept { std::free(value); }
void operator delete[](void* value, std::size_t) noexcept { std::free(value); }

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: dxf_3d_entity_version_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
