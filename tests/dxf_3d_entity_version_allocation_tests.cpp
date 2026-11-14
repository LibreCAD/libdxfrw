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

enum class Profile {
    Face, Line, Solid, Trace, Point, Circle, Arc, BasicLine, Ellipse, Text,
    RText, ArcAlignedText, Shape, SectionObject, PointCloud
};

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
    case Profile::RText: return "RTEXT";
    case Profile::ArcAlignedText: return "ARCALIGNEDTEXT";
    case Profile::Shape: return "SHAPE";
    case Profile::SectionObject: return "SECTIONOBJECT";
    case Profile::PointCloud: return "POINTCLOUD";
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

DRW_RText makeRText() {
    DRW_RText text;
    text.handle = 0xD943u;
    text.basePoint = DRW_Coord{12.0, -3.0, 1.0};
    text.secPoint = DRW_Coord{15.0, 6.0, 2.0};
    text.height = 2.0;
    text.text = "revision matrix RTEXT";
    text.angle = 15.0;
    text.widthscale = 1.2;
    text.oblique = 3.5;
    text.style = "MatrixStyle";
    text.textgen = 2;
    text.alignH = DRW_Text::HCenter;
    text.alignV = DRW_Text::VTop;
    text.extPoint = DRW_Coord{0.0, 0.0, 1.0};
    text.m_rTextFlags = 3;
    return text;
}

DRW_ArcAlignedText makeArcAlignedText() {
    DRW_ArcAlignedText text;
    text.handle = 0xD944u;
    text.text = "revision matrix arc text";
    text.style = "MatrixStyle";
    text.m_center = DRW_Coord{2.0, -1.0, 0.5};
    text.m_radius = 8.0;
    text.m_startAngle = 0.25;
    text.m_endAngle = 1.75;
    text.m_textSize = "2.5";
    text.m_xScale = "1.2";
    text.m_charSpacing = "1.1";
    text.m_offsetFromArc = "0.2";
    text.m_rightOffset = "0.3";
    text.m_leftOffset = "0.4";
    text.m_fontName = "Simplex";
    text.m_bigFontName = "Bigfont";
    text.m_rawColor = 0x123456;
    text.m_characterSet = 1;
    text.m_pitchAndFamily = 2;
    text.m_isShx = 1;
    text.m_isBold = 1;
    text.m_isItalic = 0;
    text.m_isUnderlined = 1;
    text.m_alignment = 2;
    text.m_isReverse = 1;
    text.m_wizardFlag = 1;
    text.m_textPosition = 2;
    text.m_textDirection = 1;
    text.m_arcHandle = 0xD945u;
    return text;
}

DRW_Shape makeShape() {
    DRW_Shape shape;
    shape.handle = 0xD942u;
    shape.m_insertionPoint = DRW_Coord{5.0, -2.0, 1.5};
    shape.m_scale = 2.25;
    shape.m_rotation = 0.5;
    shape.m_widthFactor = 1.3;
    shape.m_oblique = 0.2;
    shape.m_thickness = 0.75;
    shape.m_styleName = "SIMPLEX";
    shape.m_extrusion = DRW_Coord{0.0, 1.0, 0.0};
    return shape;
}

DRW_SectionObject makeSectionObject() {
    DRW_SectionObject section;
    section.handle = 0xD946u;
    section.m_state = 3u;
    section.m_flags = 5u;
    section.m_name = "VERSION_MATRIX_SECTION";
    section.m_vertDir = DRW_Coord{0.0, 0.0, 1.0};
    section.m_topHeight = 12.5;
    section.m_bottomHeight = -2.5;
    section.m_indicatorAlpha = 125u;
    section.m_indicatorColor = 6u;
    section.m_verts = {DRW_Coord{1.0, 2.0, 3.0},
                       DRW_Coord{4.0, 5.0, 6.0}};
    section.m_blVerts = {DRW_Coord{-1.0, -2.0, -3.0},
                         DRW_Coord{-4.0, -5.0, -6.0}};
    return section;
}

bool sameSectionObject(const DRW_SectionObject& a,
                       const DRW_SectionObject& b,
                       bool compareHandle = true) {
    const auto samePoint = [](const DRW_Coord& x, const DRW_Coord& y) {
        return x.x == y.x && x.y == y.y && x.z == y.z;
    };
    const auto samePoints = [&samePoint](const std::vector<DRW_Coord>& x,
                                         const std::vector<DRW_Coord>& y) {
        return x.size() == y.size()
            && std::equal(x.begin(), x.end(), y.begin(), samePoint);
    };
    return (!compareHandle || a.handle == b.handle)
        && a.m_state == b.m_state && a.m_flags == b.m_flags
        && a.m_name == b.m_name && samePoint(a.m_vertDir, b.m_vertDir)
        && a.m_topHeight == b.m_topHeight
        && a.m_bottomHeight == b.m_bottomHeight
        && a.m_indicatorAlpha == b.m_indicatorAlpha
        && a.m_indicatorColor == b.m_indicatorColor
        && samePoints(a.m_verts, b.m_verts)
        && samePoints(a.m_blVerts, b.m_blVerts)
        && a.m_sectionSettingsHandle == b.m_sectionSettingsHandle
        && a.m_indicatorColorRgb == b.m_indicatorColorRgb
        && a.m_indicatorColorName == b.m_indicatorColorName
        && a.m_indicatorColorBookName == b.m_indicatorColorBookName
        && a.m_indicatorColorDwgMethod == b.m_indicatorColorDwgMethod
        && a.m_indicatorColorDwgFlags == b.m_indicatorColorDwgFlags
        && a.m_indicatorColorDwgIndex == b.m_indicatorColorDwgIndex
        && a.m_indicatorColorDwgMetadataPresent
               == b.m_indicatorColorDwgMetadataPresent;
}

DRW_PointCloud makePointCloud(bool withLateFields = false) {
    DRW_PointCloud pointCloud;
    pointCloud.handle = 0xD947u;
    pointCloud.classVersion = 2u;
    pointCloud.origin = DRW_Coord{11.0, 12.0, 13.0};
    pointCloud.savedFilename = "clouds/version-matrix.rcs";
    pointCloud.sourceFileCount = 2;
    pointCloud.sourceFiles = {"clouds/part-a.rcs", "clouds/part-b.rcs"};
    pointCloud.extentsMin = DRW_Coord{-10.0, -20.0, -30.0};
    pointCloud.extentsMax = DRW_Coord{40.0, 50.0, 60.0};
    pointCloud.pointCount = 987654u;
    pointCloud.ucsName = "VERSION_MATRIX_POINTCLOUD_UCS";
    pointCloud.ucsOrigin = DRW_Coord{1.0, 2.0, 3.0};
    pointCloud.ucsXDirection = DRW_Coord{0.0, 1.0, 0.0};
    pointCloud.ucsYDirection = DRW_Coord{-1.0, 0.0, 0.0};
    pointCloud.ucsZDirection = DRW_Coord{0.0, 0.0, 1.0};
    if (withLateFields) {
        pointCloud.intensityScheme = 2;
        pointCloud.intensityStyle = {1.5, 9.5, 2.5, 8.5};
    }
    return pointCloud;
}

bool samePointCloud(const DRW_PointCloud& a, const DRW_PointCloud& b,
                    bool compareHandle = true) {
    const auto samePoint = [](const DRW_Coord& x, const DRW_Coord& y) {
        return x.x == y.x && x.y == y.y && x.z == y.z;
    };
    return (!compareHandle || a.handle == b.handle)
        && a.classVersion == b.classVersion && samePoint(a.origin, b.origin)
        && a.savedFilename == b.savedFilename
        && a.sourceFileCount == b.sourceFileCount
        && a.sourceFiles == b.sourceFiles
        && samePoint(a.extentsMin, b.extentsMin)
        && samePoint(a.extentsMax, b.extentsMax)
        && a.pointCount == b.pointCount && a.ucsName == b.ucsName
        && samePoint(a.ucsOrigin, b.ucsOrigin)
        && samePoint(a.ucsXDirection, b.ucsXDirection)
        && samePoint(a.ucsYDirection, b.ucsYDirection)
        && samePoint(a.ucsZDirection, b.ucsZDirection)
        && a.definitionHandle == b.definitionHandle
        && a.reactorHandle == b.reactorHandle
        && a.showIntensity == b.showIntensity
        && a.intensityScheme == b.intensityScheme
        && a.intensityStyle.minIntensity == b.intensityStyle.minIntensity
        && a.intensityStyle.maxIntensity == b.intensityStyle.maxIntensity
        && a.intensityStyle.lowThreshold == b.intensityStyle.lowThreshold
        && a.intensityStyle.highThreshold == b.intensityStyle.highThreshold
        && a.showClipping == b.showClipping
        && a.clippingCount == b.clippingCount
        && a.clippings.size() == b.clippings.size();
}

bool sameShape(const DRW_Shape& a, const DRW_Shape& b,
               bool compareHandle = true) {
    return (!compareHandle || a.handle == b.handle)
        && a.m_insertionPoint.x == b.m_insertionPoint.x
        && a.m_insertionPoint.y == b.m_insertionPoint.y
        && a.m_insertionPoint.z == b.m_insertionPoint.z
        && a.m_scale == b.m_scale && a.m_rotation == b.m_rotation
        && a.m_widthFactor == b.m_widthFactor && a.m_oblique == b.m_oblique
        && a.m_thickness == b.m_thickness && a.m_styleName == b.m_styleName
        && a.m_extrusion.x == b.m_extrusion.x
        && a.m_extrusion.y == b.m_extrusion.y
        && a.m_extrusion.z == b.m_extrusion.z;
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

bool sameFiniteTextNumber(const std::string& a, const std::string& b) {
    char* aEnd = nullptr;
    char* bEnd = nullptr;
    const double aValue = std::strtod(a.c_str(), &aEnd);
    const double bValue = std::strtod(b.c_str(), &bEnd);
    return aEnd != a.c_str() && *aEnd == '\0'
        && bEnd != b.c_str() && *bEnd == '\0'
        && std::isfinite(aValue) && std::isfinite(bValue)
        && std::abs(aValue - bValue) < 1e-12;
}

bool sameRText(const DRW_RText& a, const DRW_RText& b,
               bool compareHandle = true) {
    return sameText(a, b, compareHandle)
        && a.extPoint.x == b.extPoint.x && a.extPoint.y == b.extPoint.y
        && a.extPoint.z == b.extPoint.z && a.m_rTextFlags == b.m_rTextFlags;
}

bool sameArcAlignedText(const DRW_ArcAlignedText& a,
                        const DRW_ArcAlignedText& b,
                        bool compareHandle = true) {
    return sameText(a, b, compareHandle)
        && a.m_center.x == b.m_center.x && a.m_center.y == b.m_center.y
        && a.m_center.z == b.m_center.z && a.m_radius == b.m_radius
        && a.m_startAngle == b.m_startAngle && a.m_endAngle == b.m_endAngle
        && a.m_textSize == b.m_textSize && a.m_xScale == b.m_xScale
        && a.m_charSpacing == b.m_charSpacing
        && a.m_offsetFromArc == b.m_offsetFromArc
        && a.m_rightOffset == b.m_rightOffset
        && a.m_leftOffset == b.m_leftOffset && a.m_fontName == b.m_fontName
        && a.m_bigFontName == b.m_bigFontName && a.m_rawColor == b.m_rawColor
        && a.m_characterSet == b.m_characterSet
        && a.m_pitchAndFamily == b.m_pitchAndFamily && a.m_isShx == b.m_isShx
        && a.m_isBold == b.m_isBold && a.m_isItalic == b.m_isItalic
        && a.m_isUnderlined == b.m_isUnderlined
        && a.m_alignment == b.m_alignment && a.m_isReverse == b.m_isReverse
        && a.m_wizardFlag == b.m_wizardFlag
        && a.m_textPosition == b.m_textPosition
        && a.m_textDirection == b.m_textDirection
        && a.m_arcHandle == b.m_arcHandle;
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
    Export(dxfRW& codec, Profile profile, bool pointCloudLateFields = false)
        : codec(codec), profile(profile) {
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
        rtext = makeRText();
        arcAlignedText = makeArcAlignedText();
        shape = makeShape();
        sectionObject = makeSectionObject();
        pointCloud = makePointCloud(pointCloudLateFields);
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
        case Profile::RText: accepted = codec.writeRText(&rtext); break;
        case Profile::ArcAlignedText:
            accepted = codec.writeArcAlignedText(&arcAlignedText); break;
        case Profile::Shape: accepted = codec.writeShape(&shape); break;
        case Profile::SectionObject:
            accepted = codec.writeSectionObject(&sectionObject); break;
        case Profile::PointCloud:
            accepted = codec.writePointCloud(&pointCloud); break;
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
    DRW_RText rtext;
    DRW_ArcAlignedText arcAlignedText;
    DRW_Shape shape;
    DRW_SectionObject sectionObject;
    DRW_PointCloud pointCloud;
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
                     DRW::Version expectedVersion,
                     bool pointCloudLateFields = false) {
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
    if (profile == Profile::RText) {
        const auto* text = dynamic_cast<const DRW_RText*>(entity);
        const DRW_RText expected = makeRText();
        return text != nullptr && text->eType == DRW::TEXT
            && text->basePoint.x == expected.basePoint.x
            && text->basePoint.y == expected.basePoint.y
            && text->basePoint.z == expected.basePoint.z
            && text->height == expected.height && text->text == expected.text
            && text->angle == expected.angle && text->style == expected.style
            && text->extPoint.x == expected.extPoint.x
            && text->extPoint.y == expected.extPoint.y
            && text->extPoint.z == expected.extPoint.z
            && text->m_rTextFlags == expected.m_rTextFlags;
    }
    if (profile == Profile::ArcAlignedText) {
        const auto* text = dynamic_cast<const DRW_ArcAlignedText*>(entity);
        DRW_ArcAlignedText expected = makeArcAlignedText();
        expected.applyArcApproximation();
        if (text == nullptr) {
            std::cerr << "ARCALIGNEDTEXT readback subtype missing\n";
            return false;
        }
        const bool matches = text->eType == DRW::TEXT
            && text->text == expected.text && text->style == expected.style
            && text->basePoint.x == expected.basePoint.x
            && text->basePoint.y == expected.basePoint.y
            && text->basePoint.z == expected.basePoint.z
            && text->secPoint.x == expected.secPoint.x
            && text->secPoint.y == expected.secPoint.y
            && text->secPoint.z == expected.secPoint.z
            && text->height == expected.height
            && std::abs(text->angle - expected.angle) < 1e-12
            && text->m_center.x == expected.m_center.x
            && text->m_center.y == expected.m_center.y
            && text->m_center.z == expected.m_center.z
            && text->m_radius == expected.m_radius
            && text->m_startAngle == expected.m_startAngle
            && text->m_endAngle == expected.m_endAngle
            && sameFiniteTextNumber(text->m_textSize, expected.m_textSize)
            && sameFiniteTextNumber(text->m_xScale, expected.m_xScale)
            && sameFiniteTextNumber(text->m_charSpacing, expected.m_charSpacing)
            && sameFiniteTextNumber(text->m_offsetFromArc, expected.m_offsetFromArc)
            && sameFiniteTextNumber(text->m_rightOffset, expected.m_rightOffset)
            && sameFiniteTextNumber(text->m_leftOffset, expected.m_leftOffset)
            && text->m_fontName == expected.m_fontName
            && text->m_bigFontName == expected.m_bigFontName
            && text->m_rawColor == expected.m_rawColor
            && text->m_characterSet == expected.m_characterSet
            && text->m_pitchAndFamily == expected.m_pitchAndFamily
            && text->m_isShx == expected.m_isShx
            && text->m_isBold == expected.m_isBold
            && text->m_isItalic == expected.m_isItalic
            && text->m_isUnderlined == expected.m_isUnderlined
            && text->m_alignment == expected.m_alignment
            && text->m_isReverse == expected.m_isReverse
            && text->m_wizardFlag == expected.m_wizardFlag
            && text->m_textPosition == expected.m_textPosition
            && text->m_textDirection == expected.m_textDirection
            && text->extPoint.x == expected.extPoint.x
            && text->extPoint.y == expected.extPoint.y
            && text->extPoint.z == expected.extPoint.z;
        return matches;
    }
    if (profile == Profile::Shape) {
        const auto* shape = dynamic_cast<const DRW_Shape*>(entity);
        const DRW_Shape expected = makeShape();
        return shape != nullptr && shape->eType == DRW::SHAPE
            && shape->m_insertionPoint.x == expected.m_insertionPoint.x
            && shape->m_insertionPoint.y == expected.m_insertionPoint.y
            && shape->m_insertionPoint.z == expected.m_insertionPoint.z
            && shape->m_scale == expected.m_scale
            && std::abs(shape->m_rotation - expected.m_rotation) < 1e-12
            && shape->m_widthFactor == expected.m_widthFactor
            && std::abs(shape->m_oblique - expected.m_oblique) < 1e-12
            && shape->m_thickness == expected.m_thickness
            && shape->m_styleName == expected.m_styleName
            && shape->m_extrusion.x == expected.m_extrusion.x
            && shape->m_extrusion.y == expected.m_extrusion.y
            && shape->m_extrusion.z == expected.m_extrusion.z;
    }
    if (profile == Profile::SectionObject) {
        const auto* section = dynamic_cast<const DRW_SectionObject*>(entity);
        return section != nullptr && section->eType == DRW::SECTION
            && sameSectionObject(*section, makeSectionObject(), false);
    }
    if (profile == Profile::PointCloud) {
        const auto* pointCloud = dynamic_cast<const DRW_PointCloud*>(entity);
        return pointCloud != nullptr && pointCloud->eType == DRW::POINTCLOUD
            && samePointCloud(*pointCloud,
                              makePointCloud(pointCloudLateFields), false);
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
    const std::array<std::pair<DRW::Version, const char*>, 7> modernEntityVersions = {{
        {DRW::AC1014, "AC1014"}, {DRW::AC1015, "AC1015"},
        {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
        {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
        {DRW::AC1032, "AC1032"}}};
    const std::array<std::pair<DRW::Version, const char*>, 4>
        sectionObjectVersions = {{
        {DRW::AC1021, "AC1021"}, {DRW::AC1024, "AC1024"},
        {DRW::AC1027, "AC1027"}, {DRW::AC1032, "AC1032"}}};
    const std::array<std::pair<DRW::Version, const char*>, 4>
        pointCloudVersions = {{
        {DRW::AC1021, "AC1021"}, {DRW::AC1024, "AC1024"},
        {DRW::AC1027, "AC1027"}, {DRW::AC1032, "AC1032"}}};
    const std::array<std::pair<DRW::Version, const char*>, 8> shapeVersions = {{
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
                const DRW_RText originalRText = exporter.rtext;
                const DRW_ArcAlignedText originalArcAlignedText = exporter.arcAlignedText;
                const DRW_Shape originalShape = exporter.shape;
                const DRW_SectionObject originalSectionObject =
                    exporter.sectionObject;
                const DRW_PointCloud originalPointCloud = exporter.pointCloud;
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
                case Profile::RText:
                    callerPreserved = sameRText(exporter.rtext, originalRText);
                    break;
                case Profile::ArcAlignedText:
                    callerPreserved = sameArcAlignedText(exporter.arcAlignedText,
                                                         originalArcAlignedText);
                    break;
                case Profile::Shape:
                    callerPreserved = sameShape(exporter.shape, originalShape);
                    break;
                case Profile::SectionObject:
                    callerPreserved = sameSectionObject(
                        exporter.sectionObject, originalSectionObject);
                    break;
                case Profile::PointCloud:
                    callerPreserved = samePointCloud(
                        exporter.pointCloud, originalPointCloud);
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
                                                : profile == Profile::Text
                                                    ? sameText(exporter.text,
                                                        originalText)
                                                : profile == Profile::RText
                                                    ? sameRText(exporter.rtext,
                                                        originalRText)
                                                    : profile == Profile::ArcAlignedText
                                                        ? sameArcAlignedText(
                                                            exporter.arcAlignedText,
                                                            originalArcAlignedText)
                                                        : profile == Profile::Shape
                                                            ? sameShape(exporter.shape,
                                                                originalShape)
                                                            : profile == Profile::SectionObject
                                                                ? sameSectionObject(
                                                                    exporter.sectionObject,
                                                                    originalSectionObject)
                                                                : samePointCloud(
                                                                    exporter.pointCloud,
                                                                    originalPointCloud);
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
    } else if (profile == Profile::RText || profile == Profile::ArcAlignedText) {
        for (const auto& version : modernEntityVersions)
            if (!runVersion(version.first, version.second))
                return false;
    } else if (profile == Profile::Shape) {
        for (const auto& version : shapeVersions)
            if (!runVersion(version.first, version.second))
                return false;
    } else if (profile == Profile::SectionObject) {
        for (const auto& version : sectionObjectVersions)
            if (!runVersion(version.first, version.second))
                return false;
    } else if (profile == Profile::PointCloud) {
        for (const auto& version : pointCloudVersions)
            if (!runVersion(version.first, version.second))
                return false;
    } else {
        for (const auto& version : legacyVersions)
            if (!runVersion(version.first, version.second))
                return false;
    }
    return true;
}

bool runUnsupportedTextExtensionBoundaries(
        const std::filesystem::path& directory);

bool runSectionObjectVersionBoundary(
        const std::filesystem::path& directory) {
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return false;
    for (const bool binary : {false, true}) {
        const auto output = directory
            / (binary ? "SECTIONOBJECT-boundary-binary.dxf"
                      : "SECTIONOBJECT-boundary-ascii.dxf");
        std::string expectedBytes;
        {
            dxfRW cleanCodec(output.string().c_str());
            Export cleanExporter(cleanCodec, Profile::SectionObject);
            if (!cleanCodec.write(&cleanExporter, DRW::AC1021, binary)
                || !cleanExporter.accepted
                || !readBytes(output, expectedBytes)
                || !readbackMatches(output, Profile::SectionObject,
                                    DRW::AC1021))
                return false;
        }
        const std::string sentinel = "prior unsupported SECTIONOBJECT target\n";
        {
            std::ofstream prior(output, std::ios::binary | std::ios::trunc);
            prior << sentinel;
            if (!prior)
                return false;
        }
        dxfRW codec(output.string().c_str());
        Export exporter(codec, Profile::SectionObject);
        const DRW_SectionObject original = exporter.sectionObject;
        const bool refused = !codec.write(&exporter, DRW::AC1018, binary)
            && !exporter.accepted
            && sameSectionObject(exporter.sectionObject, original);
        std::string afterRefusal;
        if (!refused || !readBytes(output, afterRefusal)
            || afterRefusal != sentinel) {
            std::cerr << "SECTIONOBJECT AC1018 refusal changed caller or "
                         "destination\n";
            return false;
        }
        const bool retried = codec.write(&exporter, DRW::AC1021, binary)
            && exporter.accepted;
        std::string retryBytes;
        if (!retried || !readBytes(output, retryBytes)
            || retryBytes != expectedBytes
            || !readbackMatches(output, Profile::SectionObject,
                                DRW::AC1021)) {
            std::cerr << "SECTIONOBJECT AC1018 refusal then AC1021 retry "
                         "failed\n";
            return false;
        }
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
    }
    return true;
}

bool runPointCloudVersionLossBoundary(
        const std::filesystem::path& directory) {
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return false;
    for (const bool binary : {false, true}) {
        const auto output = directory
            / (binary ? "POINTCLOUD-boundary-binary.dxf"
                      : "POINTCLOUD-boundary-ascii.dxf");
        std::string expectedBytes;
        {
            dxfRW cleanCodec(output.string().c_str());
            Export cleanExporter(cleanCodec, Profile::PointCloud, true);
            if (!cleanCodec.write(&cleanExporter, DRW::AC1027, binary)
                || !cleanExporter.accepted
                || !readBytes(output, expectedBytes)
                || !readbackMatches(output, Profile::PointCloud,
                                    DRW::AC1027, true))
                return false;
        }
        const std::string sentinel = "prior POINTCLOUD version target\n";
        {
            std::ofstream prior(output, std::ios::binary | std::ios::trunc);
            prior << sentinel;
            if (!prior)
                return false;
        }
        dxfRW codec(output.string().c_str());
        Export exporter(codec, Profile::PointCloud, true);
        const DRW_PointCloud original = exporter.pointCloud;
        const bool refused = !codec.write(&exporter, DRW::AC1024, binary)
            && !exporter.accepted
            && samePointCloud(exporter.pointCloud, original);
        std::string afterRefusal;
        if (!refused || !readBytes(output, afterRefusal)
            || afterRefusal != sentinel) {
            std::cerr << "POINTCLOUD AC1024 refusal changed caller or "
                         "destination\n";
            return false;
        }
        const bool retried = codec.write(&exporter, DRW::AC1027, binary)
            && exporter.accepted;
        std::string retryBytes;
        if (!retried || !readBytes(output, retryBytes)
            || retryBytes != expectedBytes
            || !readbackMatches(output, Profile::PointCloud,
                                DRW::AC1027, true)) {
            std::cerr << "POINTCLOUD AC1024 refusal then AC1027 retry failed\n";
            return false;
        }
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
    }
    return true;
}

bool runPointCloudMinimumVersionBoundary(
        const std::filesystem::path& directory) {
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return false;
    for (const bool binary : {false, true}) {
        const auto output = directory
            / (binary ? "POINTCLOUD-minimum-binary.dxf"
                      : "POINTCLOUD-minimum-ascii.dxf");
        std::string expectedBytes;
        {
            dxfRW cleanCodec(output.string().c_str());
            Export cleanExporter(cleanCodec, Profile::PointCloud);
            if (!cleanCodec.write(&cleanExporter, DRW::AC1021, binary)
                || !cleanExporter.accepted
                || !readBytes(output, expectedBytes)
                || !readbackMatches(output, Profile::PointCloud,
                                    DRW::AC1021))
                return false;
        }
        const std::string sentinel = "prior POINTCLOUD minimum target\n";
        {
            std::ofstream prior(output, std::ios::binary | std::ios::trunc);
            prior << sentinel;
            if (!prior)
                return false;
        }
        dxfRW codec(output.string().c_str());
        Export exporter(codec, Profile::PointCloud);
        const DRW_PointCloud original = exporter.pointCloud;
        const bool refused = !codec.write(&exporter, DRW::AC1018, binary)
            && !exporter.accepted
            && samePointCloud(exporter.pointCloud, original);
        std::string afterRefusal;
        if (!refused || !readBytes(output, afterRefusal)
            || afterRefusal != sentinel) {
            std::cerr << "POINTCLOUD AC1018 refusal changed caller or "
                         "destination\n";
            return false;
        }
        const bool retried = codec.write(&exporter, DRW::AC1021, binary)
            && exporter.accepted;
        std::string retryBytes;
        if (!retried || !readBytes(output, retryBytes)
            || retryBytes != expectedBytes
            || !readbackMatches(output, Profile::PointCloud, DRW::AC1021)) {
            std::cerr << "POINTCLOUD AC1018 refusal then AC1021 retry failed\n";
            return false;
        }
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
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
        && runProfile(directory, Profile::Text)
        && runProfile(directory, Profile::RText)
        && runProfile(directory, Profile::ArcAlignedText)
        && runUnsupportedTextExtensionBoundaries(directory)
        && runProfile(directory, Profile::Shape)
        && runProfile(directory, Profile::SectionObject)
        && runSectionObjectVersionBoundary(directory)
        && runProfile(directory, Profile::PointCloud)
        && runPointCloudVersionLossBoundary(directory)
        && runPointCloudMinimumVersionBoundary(directory);
}

bool runUnsupportedTextExtensionBoundaries(
        const std::filesystem::path& directory) {
    for (const Profile profile : {Profile::RText, Profile::ArcAlignedText}) {
        for (const bool binary : {false, true}) {
            const std::string stem = std::string(profileName(profile))
                + "-AC1009-refusal-retry";
            const auto output = directory
                / (stem + (binary ? "-binary.dxf" : "-ascii.dxf"));
            std::string expectedBytes;
            {
                dxfRW cleanCodec(output.string().c_str());
                Export cleanExporter(cleanCodec, profile);
                if (!cleanCodec.write(&cleanExporter, DRW::AC1014, binary)
                    || !cleanExporter.accepted
                    || !readBytes(output, expectedBytes)
                    || !readbackMatches(output, profile, DRW::AC1014)) {
                    std::cerr << profileName(profile)
                              << " AC1014 boundary reference failed\n";
                    return false;
                }
            }
            const std::string sentinel = "prior unsupported target output\n";
            {
                std::ofstream prior(output,
                                    std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior)
                    return false;
            }
            dxfRW codec(output.string().c_str());
            Export exporter(codec, profile);
            const DRW_RText originalRText = exporter.rtext;
            const DRW_ArcAlignedText originalArcAlignedText =
                exporter.arcAlignedText;
            const bool rejected = !codec.write(&exporter, DRW::AC1009, binary)
                && !exporter.accepted
                && (profile == Profile::RText
                    ? sameRText(exporter.rtext, originalRText)
                    : sameArcAlignedText(exporter.arcAlignedText,
                                         originalArcAlignedText));
            std::string afterRefusal;
            if (!rejected || !readBytes(output, afterRefusal)
                || afterRefusal != sentinel) {
                std::cerr << profileName(profile)
                          << " AC1009 refusal changed caller or destination\n";
                return false;
            }
            const bool retried = codec.write(&exporter, DRW::AC1014, binary)
                && exporter.accepted;
            std::string retryBytes;
            if (!retried || !readBytes(output, retryBytes)
                || retryBytes != expectedBytes
                || !readbackMatches(output, profile, DRW::AC1014)) {
                std::cerr << profileName(profile)
                          << " AC1009 refusal then AC1014 retry failed\n";
                return false;
            }
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
    }
    return true;
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
    if (argc == 3 && std::string(argv[2]) == "--sectionobject-versions") {
        const std::filesystem::path directory(argv[1]);
        const bool passed = runProfile(directory, Profile::SectionObject)
            && runSectionObjectVersionBoundary(directory);
        std::cout << "SECTIONOBJECT version transaction matrix: "
                  << (passed ? "PASS" : "FAIL") << '\n';
        return passed ? 0 : 1;
    }
    if (argc == 3 && std::string(argv[2]) == "--pointcloud-versions") {
        const std::filesystem::path directory(argv[1]);
        const bool passed = runProfile(directory, Profile::PointCloud)
            && runPointCloudVersionLossBoundary(directory)
            && runPointCloudMinimumVersionBoundary(directory);
        std::cout << "POINTCLOUD version transaction matrix: "
                  << (passed ? "PASS" : "FAIL") << '\n';
        return passed ? 0 : 1;
    }
    if (argc != 2) {
        std::cerr << "usage: dxf_3d_entity_version_allocation_tests SCRATCH_DIR"
                     " [--sectionobject-versions|--pointcloud-versions]\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
