#include <array>
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include "drw_acis.h"
#include "drw_base.h"
#include "drw_datastorage.h"
#include "intern/dwgsafety.h"
#include "intern/dwgreader15.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"
#include "intern/proxygraphicdecoder.h"
#define private public
#include "libdxfrw.h"
#undef private
#include "libdwgr.h"

namespace {

struct TestContext {
    int failures {0};

    void expect(bool condition, const char* label) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << label << '\n';
        }
    }
};

// Expose the protected DXF record hooks for parser-state copy regressions.
// These wrappers stay local to the hardening executable and do not alter the
// installed API surface.
class ExposedAttrib : public DRW_Attrib {
public:
    using DRW_Attrib::parseCode;
};

class ExposedGeoPositionMarker : public DRW_GeoPositionMarker {
public:
    using DRW_GeoPositionMarker::parseCode;
};

class ExposedLWPolyline : public DRW_LWPolyline {
public:
    using DRW_LWPolyline::parseCode;
};

class ExposedLeader : public DRW_Leader {
public:
    using DRW_Leader::parseCode;
    using DRW_Leader::validateDxf;
};

class ExposedMLine : public DRW_MLine {
public:
    using DRW_MLine::parseCode;
};

class ExposedTable : public DRW_Table {
public:
    using DRW_Table::parseCode;
};

class ExposedMesh : public DRW_Mesh {
public:
    using DRW_Mesh::parseCode;
};

class ExposedNavisworksModel : public DRW_NavisworksModel {
public:
    using DRW_NavisworksModel::parseCode;
};

class ExposedPointCloud : public DRW_PointCloud {
public:
    using DRW_PointCloud::parseCode;
    using DRW_PointCloud::finalizeDxf;
};

class ExposedPointCloudEx : public DRW_PointCloudEx {
public:
    using DRW_PointCloudEx::parseCode;
    using DRW_PointCloudEx::finalizeDxf;
};

class ExposedNurbsSurface : public DRW_NurbsSurface {
public:
    using DRW_NurbsSurface::parseCode;
    using DRW_NurbsSurface::finalizeDxf;
};

class ExposedRevolvedSurface : public DRW_RevolvedSurface {
public:
    using DRW_RevolvedSurface::parseCode;
};

class ExposedExtrudedSurface : public DRW_ExtrudedSurface {
public:
    using DRW_ExtrudedSurface::parseCode;
};

class ExposedSweptSurface : public DRW_SweptSurface {
public:
    using DRW_SweptSurface::parseCode;
};

class ExposedLoftedSurface : public DRW_LoftedSurface {
public:
    using DRW_LoftedSurface::parseCode;
    using DRW_LoftedSurface::finalizeDxf;
};

class ExposedImage : public DRW_Image {
public:
    using DRW_Image::parseCode;
};

class ExposedSectionObject : public DRW_SectionObject {
public:
    using DRW_SectionObject::parseCode;
};

class ExposedOle2Frame : public DRW_Ole2Frame {
public:
    using DRW_Ole2Frame::parseCode;
};

class ExposedOleFrame : public DRW_OleFrame {
public:
    using DRW_OleFrame::parseCode;
};

class ExposedBreakData : public DRW_BreakData {
public:
    using DRW_BreakData::parseCode;
};

class ExposedDictionary : public DRW_Dictionary {
public:
    using DRW_Dictionary::parseCode;
};

class ExposedDictionaryWithDefault : public DRW_DictionaryWithDefault {
public:
    using DRW_DictionaryWithDefault::parseCode;
};

class ExposedXRecord : public DRW_XRecord {
public:
    using DRW_XRecord::parseCode;
};

class ExposedSortEntsTable : public DRW_SortEntsTable {
public:
    using DRW_SortEntsTable::parseCode;
};

class ExposedIndex : public DRW_Index {
public:
    using DRW_Index::parseCode;
};

class ExposedIdBuffer : public DRW_IDBuffer {
public:
    using DRW_IDBuffer::parseCode;
};

class ExposedLayerIndex : public DRW_LayerIndex {
public:
    using DRW_LayerIndex::parseCode;
    using DRW_LayerIndex::finalizeDxf;
};

class ExposedSpatialIndex : public DRW_SpatialIndex {
public:
    using DRW_SpatialIndex::parseCode;
};

class ExposedLightList : public DRW_LightList {
public:
    using DRW_LightList::parseCode;
};

class ExposedGeoMapImage : public DRW_GeoMapImage {
public:
    using DRW_GeoMapImage::parseCode;
};

class ExposedDataLink : public DRW_DataLink {
public:
    using DRW_DataLink::parseCode;
};

class ExposedGeoData : public DRW_GeoData {
public:
    using DRW_GeoData::parseCode;
};

class ExposedSpatialFilter : public DRW_SpatialFilter {
public:
    using DRW_SpatialFilter::parseCode;
};

class ExposedEvaluationGraph : public DRW_EvaluationGraph {
public:
    using DRW_EvaluationGraph::parseCode;
    using DRW_EvaluationGraph::finalizeDxf;
};

class ExposedDimensionAssociation : public DRW_DimensionAssociation {
public:
    using DRW_DimensionAssociation::parseCode;
};

class ExposedPointCloudColorMap : public DRW_PointCloudColorMap {
public:
    using DRW_PointCloudColorMap::parseCode;
};

class ExposedSunStudy : public DRW_SunStudy {
public:
    using DRW_SunStudy::parseCode;
};

class ExposedMotionPath : public DRW_MotionPath {
public:
    using DRW_MotionPath::parseCode;
};

class ExposedCurvePath : public DRW_CurvePath {
public:
    using DRW_CurvePath::parseCode;
};

class ExposedPointPath : public DRW_PointPath {
public:
    using DRW_PointPath::parseCode;
};

class ExposedPartialViewingIndex : public DRW_PartialViewingIndex {
public:
    using DRW_PartialViewingIndex::parseCode;
    using DRW_PartialViewingIndex::finalizeDxf;
};

class ExposedSection : public DRW_Section {
public:
    using DRW_Section::parseCode;
};

class ExposedBackground : public DRW_Background {
public:
    using DRW_Background::parseCode;
};

class ExposedNavisworksModelDef : public DRW_NavisworksModelDef {
public:
    using DRW_NavisworksModelDef::parseCode;
};

class ExposedLayout : public DRW_Layout {
public:
    using DRW_Layout::parseCode;
};

class ExposedDetailViewStyle : public DRW_DetailViewStyle {
public:
    using DRW_DetailViewStyle::parseCode;
};

class ExposedSectionViewStyle : public DRW_SectionViewStyle {
public:
    using DRW_SectionViewStyle::parseCode;
};

class ExposedAssociativeObject : public DRW_AssociativeObject {
public:
    using DRW_AssociativeObject::DRW_AssociativeObject;
    using DRW_AssociativeObject::parseCode;
};

template <typename Entity>
bool parseDxfRecords(Entity& entity, const std::string& source) {
    std::stringstream records(source);
    std::unique_ptr<dxfReader> reader =
        std::make_unique<dxfReaderAscii>(&records);
    int code = 0;
    bool ok = true;
    while (reader->readRec(&code))
        ok = entity.parseCode(code, reader) && ok;
    return ok;
}

void testPublicOwnershipContracts(TestContext& t) {
    static_assert(!std::is_copy_constructible<dxfRW>::value,
                  "dxfRW must remain non-copyable");
    static_assert(!std::is_move_constructible<dxfRW>::value,
                  "dxfRW must remain non-movable");
    static_assert(std::is_copy_constructible<DRW_Variant>::value,
                  "DRW_Variant copy contract");
    static_assert(std::is_copy_constructible<DRW_Header>::value,
                  "DRW_Header copy contract");
    static_assert(std::is_copy_constructible<DRW_Layer>::value,
                  "DRW_TableEntry-derived copy contract");
    static_assert(std::is_copy_constructible<DRW_LWPolyline>::value,
                  "DRW_LWPolyline copy contract");
    static_assert(std::is_copy_constructible<DRW_Polyline>::value,
                  "DRW_Polyline copy contract");
    static_assert(std::is_copy_constructible<DRW_Spline>::value,
                  "DRW_Spline copy contract");
    static_assert(std::is_copy_constructible<DRW_Insert>::value,
                  "DRW_Insert copy contract");
    static_assert(std::is_copy_constructible<DRW_Leader>::value,
                  "DRW_Leader copy contract");
    static_assert(std::is_copy_constructible<DRW_MLine>::value,
                  "DRW_MLine copy contract");
    static_assert(std::is_copy_constructible<DRW_Table>::value,
                  "DRW_Table copy contract");
    static_assert(std::is_copy_constructible<DRW_Mesh>::value,
                  "DRW_Mesh copy contract");
    static_assert(std::is_copy_constructible<DRW_Underlay>::value,
                  "DRW_Underlay copy contract");
    static_assert(std::is_copy_constructible<DRW_NavisworksModel>::value,
                  "DRW_NavisworksModel copy contract");
    static_assert(std::is_copy_constructible<DRW_PointCloud>::value,
                  "DRW_PointCloud copy contract");
    static_assert(std::is_copy_constructible<DRW_PointCloudEx>::value,
                  "DRW_PointCloudEx copy contract");
    static_assert(std::is_copy_constructible<DRW_NurbsSurface>::value,
                  "DRW_NurbsSurface copy contract");
    static_assert(std::is_copy_constructible<DRW_RevolvedSurface>::value,
                  "DRW_RevolvedSurface copy contract");
    static_assert(std::is_copy_constructible<DRW_ExtrudedSurface>::value,
                  "DRW_ExtrudedSurface copy contract");
    static_assert(std::is_copy_constructible<DRW_SweptSurface>::value,
                  "DRW_SweptSurface copy contract");
    static_assert(std::is_copy_constructible<DRW_LoftedSurface>::value,
                  "DRW_LoftedSurface copy contract");
    static_assert(std::is_copy_constructible<DRW_Image>::value,
                  "DRW_Image copy contract");
    static_assert(std::is_copy_constructible<DRW_Wipeout>::value,
                  "DRW_Wipeout copy contract");
    static_assert(std::is_copy_constructible<DRW_SectionObject>::value,
                  "DRW_SectionObject copy contract");
    static_assert(std::is_copy_constructible<DRW_Ole2Frame>::value,
                  "DRW_Ole2Frame copy contract");
    static_assert(std::is_copy_constructible<DRW_OleFrame>::value,
                  "DRW_OleFrame copy contract");
    static_assert(std::is_copy_constructible<DRW_BreakData>::value,
                  "DRW_BreakData copy contract");
    static_assert(std::is_copy_constructible<DRW_Dictionary>::value,
                  "DRW_Dictionary copy contract");
    static_assert(std::is_copy_constructible<DRW_DictionaryWithDefault>::value,
                  "DRW_DictionaryWithDefault copy contract");
    static_assert(std::is_copy_constructible<DRW_XRecord>::value,
                  "DRW_XRecord copy contract");
    static_assert(std::is_copy_constructible<DRW_SortEntsTable>::value,
                  "DRW_SortEntsTable copy contract");
    static_assert(std::is_copy_constructible<DRW_Index>::value,
                  "DRW_Index copy contract");
    static_assert(std::is_copy_constructible<DRW_IDBuffer>::value,
                  "DRW_IDBuffer copy contract");
    static_assert(std::is_copy_constructible<DRW_LayerIndex>::value,
                  "DRW_LayerIndex copy contract");
    static_assert(std::is_copy_constructible<DRW_SpatialIndex>::value,
                  "DRW_SpatialIndex copy contract");
    static_assert(std::is_copy_constructible<DRW_LightList>::value,
                  "DRW_LightList copy contract");
    static_assert(std::is_copy_constructible<DRW_GeoMapImage>::value,
                  "DRW_GeoMapImage copy contract");
    static_assert(std::is_copy_constructible<DRW_DataLink>::value,
                  "DRW_DataLink copy contract");
    static_assert(std::is_copy_constructible<DRW_GeoData>::value,
                  "DRW_GeoData copy contract");
    static_assert(std::is_copy_constructible<DRW_SpatialFilter>::value,
                  "DRW_SpatialFilter copy contract");
    static_assert(std::is_copy_constructible<DRW_EvaluationGraph>::value,
                  "DRW_EvaluationGraph copy contract");
    static_assert(std::is_copy_constructible<DRW_DimensionAssociation>::value,
                  "DRW_DimensionAssociation copy contract");
    static_assert(std::is_copy_constructible<DRW_PointCloudColorMap>::value,
                  "DRW_PointCloudColorMap copy contract");
    static_assert(std::is_copy_constructible<DRW_SunStudy>::value,
                  "DRW_SunStudy copy contract");
    static_assert(std::is_copy_constructible<DRW_MotionPath>::value,
                  "DRW_MotionPath copy contract");
    static_assert(std::is_copy_constructible<DRW_CurvePath>::value,
                  "DRW_CurvePath copy contract");
    static_assert(std::is_copy_constructible<DRW_PointPath>::value,
                  "DRW_PointPath copy contract");
    static_assert(std::is_copy_constructible<DRW_PartialViewingIndex>::value,
                  "DRW_PartialViewingIndex copy contract");
    static_assert(std::is_copy_constructible<DRW_Section>::value,
                  "DRW_Section copy contract");
    static_assert(std::is_copy_constructible<DRW_Background>::value,
                  "DRW_Background copy contract");
    static_assert(std::is_copy_constructible<DRW_NavisworksModelDef>::value,
                  "DRW_NavisworksModelDef copy contract");
    static_assert(std::is_copy_constructible<DRW_Layout>::value,
                  "DRW_Layout copy contract");
    static_assert(std::is_copy_constructible<DRW_DetailViewStyle>::value,
                  "DRW_DetailViewStyle copy contract");
    static_assert(std::is_copy_constructible<DRW_SectionViewStyle>::value,
                  "DRW_SectionViewStyle copy contract");
    static_assert(std::is_copy_constructible<DRW_AssociativeObject>::value,
                  "DRW_AssociativeObject copy contract");
    static_assert(std::is_copy_constructible<DRW_Attrib>::value,
                  "DRW_Attrib copy contract");
    static_assert(std::is_copy_constructible<DRW_GeoPositionMarker>::value,
                  "DRW_GeoPositionMarker copy contract");

    DRW_Variant sourceVariant(1, UTF8STRING("source"));
    DRW_Variant copiedVariant(sourceVariant);
    copiedVariant.addString(1, "copy");
    t.expect(std::string(sourceVariant.c_str()) == "source"
                 && std::string(copiedVariant.c_str()) == "copy",
             "DRW_Variant copy owns string storage");

    DRW_Line sourceLine;
    sourceLine.extData.push_back(
        std::make_shared<DRW_Variant>(1000, "line-xdata"));
    DRW_Line copiedLine(sourceLine);
    copiedLine.extData.front()->addString(1000, "copy-line-xdata");
    t.expect(copiedLine.extData.front() != sourceLine.extData.front()
                 && std::string(sourceLine.extData.front()->c_str())
                        == "line-xdata",
             "implicit entity copy deep-copies XDATA through DRW_Entity");
    DRW_Line assignedLine;
    assignedLine = sourceLine;
    assignedLine.extData.front()->addString(1000, "assigned-line-xdata");
    t.expect(assignedLine.extData.front() != sourceLine.extData.front()
                 && std::string(sourceLine.extData.front()->c_str())
                        == "line-xdata",
             "implicit entity assignment isolates XDATA through DRW_Entity");

    DRW_Polyline sourceLegacyPolyline;
    auto polylineVertex = std::make_shared<DRW_Vertex>();
    polylineVertex->basePoint.x = 3.0;
    polylineVertex->extData.push_back(
        std::make_shared<DRW_Variant>(1000, "polyline-xdata"));
    sourceLegacyPolyline.vertlist.push_back(polylineVertex);
    DRW_Polyline copiedLegacyPolyline(sourceLegacyPolyline);
    copiedLegacyPolyline.vertlist.front()->basePoint.x = 4.0;
    copiedLegacyPolyline.vertlist.front()->extData.front()->addString(
        1000, "copy-polyline-xdata");
    t.expect(copiedLegacyPolyline.vertlist.front() != polylineVertex
                 && polylineVertex->basePoint.x == 3.0
                 && std::string(polylineVertex->extData.front()->c_str())
                        == "polyline-xdata",
             "DRW_Polyline copy isolates vertex graph and XDATA");

    DRW_Spline sourceSpline;
    auto controlPoint = std::make_shared<DRW_Coord>(1.0, 2.0, 3.0);
    auto fitPoint = std::make_shared<DRW_Coord>(4.0, 5.0, 6.0);
    sourceSpline.controllist.push_back(controlPoint);
    sourceSpline.fitlist.push_back(fitPoint);
    sourceSpline.extData.push_back(
        std::make_shared<DRW_Variant>(1000, "spline-xdata"));
    DRW_Spline copiedSpline(sourceSpline);
    copiedSpline.controllist.front()->x = 7.0;
    copiedSpline.fitlist.front()->y = 8.0;
    copiedSpline.extData.front()->addString(1000, "copy-spline-xdata");
    t.expect(copiedSpline.controllist.front() != controlPoint
                 && copiedSpline.fitlist.front() != fitPoint
                 && controlPoint->x == 1.0 && fitPoint->y == 5.0
                 && std::string(sourceSpline.extData.front()->c_str())
                        == "spline-xdata",
             "DRW_Spline copy isolates point graphs and XDATA");

    DRW_Insert sourceInsert;
    auto insertAttribute = std::make_shared<DRW_Attrib>();
    insertAttribute->text = "attribute";
    insertAttribute->extData.push_back(
        std::make_shared<DRW_Variant>(1000, "insert-xdata"));
    sourceInsert.attlist.push_back(insertAttribute);
    DRW_Insert copiedInsert(sourceInsert);
    copiedInsert.attlist.front()->text = "copy-attribute";
    copiedInsert.attlist.front()->extData.front()->addString(
        1000, "copy-insert-xdata");
    t.expect(copiedInsert.attlist.front() != insertAttribute
                 && insertAttribute->text == "attribute"
                 && std::string(insertAttribute->extData.front()->c_str())
                        == "insert-xdata",
             "DRW_Insert copy isolates attribute graph and XDATA");

    DRW_Hatch sourceHatch;
    sourceHatch.name = "ANSI31";
    sourceHatch.solid = 0;
    sourceHatch.angle = 0.25;
    sourceHatch.scale = 2.0;
    sourceHatch.isGradient = 1;
    sourceHatch.gradName = "LINEAR";
    sourceHatch.gradColors.push_back({0.25, 0x102030, 7, 1, "", ""});
    sourceHatch.seedPoints.emplace_back(9.0, 10.0, 0.0);
    sourceHatch.extData.push_back(
        std::make_shared<DRW_Variant>(1000, "hatch-xdata"));
    auto sourceHatchLoop = std::make_shared<DRW_HatchLoop>(0);
    sourceHatchLoop->m_boundaryHandles.push_back(0x1234u);
    auto hatchLine = std::make_shared<DRW_Line>();
    hatchLine->secPoint.x = 1.0;
    hatchLine->extData.push_back(
        std::make_shared<DRW_Variant>(1000, "hatch-line-xdata"));
    auto hatchArc = std::make_shared<DRW_Arc>();
    hatchArc->radious = 2.0;
    auto hatchEllipse = std::make_shared<DRW_Ellipse>();
    hatchEllipse->ratio = 0.5;
    auto hatchSpline = std::make_shared<DRW_Spline>();
    hatchSpline->controllist.push_back(
        std::make_shared<DRW_Coord>(3.0, 4.0, 5.0));
    auto hatchPolyline = std::make_shared<DRW_LWPolyline>();
    auto hatchVertex = hatchPolyline->addVertex();
    hatchVertex->x = 6.0;
    sourceHatchLoop->objlist.push_back(hatchLine);
    sourceHatchLoop->objlist.push_back(hatchArc);
    sourceHatchLoop->objlist.push_back(hatchEllipse);
    sourceHatchLoop->objlist.push_back(hatchSpline);
    sourceHatchLoop->objlist.push_back(hatchPolyline);
    sourceHatchLoop->update();
    sourceHatch.looplist.push_back(sourceHatchLoop);

    static_assert(std::is_copy_constructible<DRW_HatchLoop>::value,
                  "DRW_HatchLoop copy contract");
    static_assert(std::is_copy_constructible<DRW_Hatch>::value,
                  "DRW_Hatch copy contract");
    DRW_Hatch copiedHatch(sourceHatch);
    t.expect(copiedHatch.looplist.size() == 1u
                 && copiedHatch.looplist.front() != sourceHatchLoop
                 && copiedHatch.looplist.front()->objlist.size() == 5u
                 && copiedHatch.looplist.front()->m_boundaryHandles.front()
                        == 0x1234u
                 && copiedHatch.name == "ANSI31"
                 && copiedHatch.gradColors.size() == 1u
                 && copiedHatch.seedPoints.front().x == 9.0
                 && copiedHatch.extData.front() != sourceHatch.extData.front(),
             "DRW_Hatch copy clones boundary graph and fill state");
    auto copiedHatchLoop = copiedHatch.looplist.front();
    auto hatchCopiedLine = std::dynamic_pointer_cast<DRW_Line>(
        copiedHatchLoop->objlist.at(0));
    auto hatchCopiedArc = std::dynamic_pointer_cast<DRW_Arc>(
        copiedHatchLoop->objlist.at(1));
    auto hatchCopiedEllipse = std::dynamic_pointer_cast<DRW_Ellipse>(
        copiedHatchLoop->objlist.at(2));
    auto hatchCopiedSpline = std::dynamic_pointer_cast<DRW_Spline>(
        copiedHatchLoop->objlist.at(3));
    auto hatchCopiedPolyline = std::dynamic_pointer_cast<DRW_LWPolyline>(
        copiedHatchLoop->objlist.at(4));
    hatchCopiedLine->secPoint.x = 11.0;
    hatchCopiedLine->extData.front()->addString(1000, "copy-hatch-line-xdata");
    hatchCopiedArc->radious = 12.0;
    hatchCopiedEllipse->ratio = 0.25;
    hatchCopiedSpline->controllist.front()->x = 13.0;
    hatchCopiedPolyline->vertlist.front()->x = 14.0;
    copiedHatch.extData.front()->addString(1000, "copy-hatch-xdata");
    t.expect(hatchCopiedLine && hatchCopiedArc && hatchCopiedEllipse
                 && hatchCopiedSpline && hatchCopiedPolyline
                 && hatchLine->secPoint.x == 1.0
                 && std::string(hatchLine->extData.front()->c_str())
                        == "hatch-line-xdata"
                 && hatchArc->radious == 2.0 && hatchEllipse->ratio == 0.5
                 && hatchSpline->controllist.front()->x == 3.0
                 && hatchVertex->x == 6.0
                 && std::string(sourceHatch.extData.front()->c_str())
                        == "hatch-xdata",
             "DRW_Hatch copy isolates every boundary edge and XDATA");
    DRW_Hatch assignedHatch;
    assignedHatch = sourceHatch;
    assignedHatch.looplist.front()->objlist.front()->eType = DRW::RAY;
    t.expect(assignedHatch.looplist.front() != sourceHatchLoop
                 && sourceHatchLoop->objlist.front()->eType == DRW::LINE
                 && assignedHatch.gradName == "LINEAR",
             "DRW_Hatch assignment isolates boundary graph");

    DRW_Leader sourceLeader;
    sourceLeader.style = "STANDARD";
    sourceLeader.arrow = 0;
    sourceLeader.leadertype = 1;
    sourceLeader.vertnum = 2;
    sourceLeader.extData.push_back(
        std::make_shared<DRW_Variant>(1000, "leader-xdata"));
    auto leaderVertex = std::make_shared<DRW_Coord>(2.0, 3.0, 4.0);
    sourceLeader.vertexlist.push_back(leaderVertex);
    sourceLeader.vertexlist.push_back(nullptr);
    DRW_Leader copiedLeader(sourceLeader);
    t.expect(copiedLeader.vertexlist.size() == 2u
                 && copiedLeader.vertexlist.front() != leaderVertex
                 && copiedLeader.vertexlist.back() == nullptr
                 && copiedLeader.style == "STANDARD"
                 && copiedLeader.arrow == 0
                 && copiedLeader.extData.front() != sourceLeader.extData.front(),
             "DRW_Leader copy clones vertex graph and persisted state");
    copiedLeader.vertexlist.front()->x = 8.0;
    copiedLeader.extData.front()->addString(1000, "copy-leader-xdata");
    t.expect(leaderVertex->x == 2.0
                 && std::string(sourceLeader.extData.front()->c_str())
                        == "leader-xdata",
             "DRW_Leader copy isolates vertex and XDATA mutation");
    DRW_Leader assignedLeader;
    assignedLeader = sourceLeader;
    assignedLeader.vertexlist.front()->y = 9.0;
    t.expect(assignedLeader.vertexlist.front() != leaderVertex
                 && leaderVertex->y == 3.0,
             "DRW_Leader assignment isolates vertex graph");
    DRW_Leader movedLeader(std::move(copiedLeader));
    t.expect(movedLeader.vertexlist.size() == 2u
                 && copiedLeader.vertexlist.empty(),
             "DRW_Leader move transfers vertex ownership");

    DRW_HatchLoop unsupported(0);
    unsupported.objlist.push_back(std::make_shared<DRW_Point>());
    bool rejectedUnsupportedEdge = false;
    try {
        DRW_HatchLoop rejected(unsupported);
        (void)rejected;
    } catch (const std::invalid_argument&) {
        rejectedUnsupportedEdge = true;
    }
    t.expect(rejectedUnsupportedEdge,
             "DRW_HatchLoop rejects unsupported polymorphic edge copies");

    DRW_Layer sourceLayer;
    t.expect(sourceLayer.addExtData(
                  std::make_unique<DRW_Variant>(1000, "layer")),
             "DRW_TableEntry accepts owned extended data");
    DRW_Layer copiedLayer(sourceLayer);
    t.expect(copiedLayer.extData.size() == 1u
                 && copiedLayer.extData.front() != sourceLayer.extData.front()
                 && std::string(copiedLayer.extData.front()->c_str()) == "layer",
             "DRW_TableEntry copy deep-copies extended data");
    copiedLayer.extData.front()->addString(1000, "copy-layer");
    t.expect(std::string(sourceLayer.extData.front()->c_str()) == "layer",
             "DRW_TableEntry copy isolates extended-data mutation");
    DRW_Layer movedLayer(std::move(copiedLayer));
    t.expect(movedLayer.extData.size() == 1u && copiedLayer.extData.empty(),
             "DRW_TableEntry move transfers extended-data ownership");

    DRW_LWPolyline sourcePolyline;
    const std::shared_ptr<DRW_Vertex2D> sourceVertex =
        sourcePolyline.addVertex();
    sourceVertex->x = 1.0;
    sourcePolyline.extData.push_back(
        std::make_shared<DRW_Variant>(1000, "polyline"));
    DRW_LWPolyline copiedPolyline(sourcePolyline);
    t.expect(copiedPolyline.vertlist.size() == 1u
                 && copiedPolyline.vertlist.front() != sourceVertex
                 && copiedPolyline.extData.front() != sourcePolyline.extData.front(),
             "DRW_LWPolyline copy deep-copies owned graphs");
    copiedPolyline.vertlist.front()->x = 2.0;
    copiedPolyline.extData.front()->addString(1000, "copy-polyline");
    t.expect(sourceVertex->x == 1.0
                 && std::string(sourcePolyline.extData.front()->c_str())
                        == "polyline",
             "DRW_LWPolyline copy isolates owned-graph mutation");

    DRW_Attrib sourceAttrib;
    sourceAttrib.extData.push_back(
        std::make_shared<DRW_Variant>(1000, "attribute-xdata"));
    sourceAttrib.mtext = std::make_unique<DRW_MText>();
    sourceAttrib.mtext->text = "attribute";
    DRW_Attrib copiedAttrib(sourceAttrib);
    t.expect(copiedAttrib.mtext != nullptr
                 && copiedAttrib.mtext.get() != sourceAttrib.mtext.get(),
             "DRW_Attrib copy deep-copies embedded MText");
    copiedAttrib.mtext->text = "copy-attribute";
    t.expect(sourceAttrib.mtext->text == "attribute",
             "DRW_Attrib copy isolates embedded MText mutation");
    copiedAttrib.extData.front()->addString(1000, "copy-attribute-xdata");
    t.expect(std::string(sourceAttrib.extData.front()->c_str())
                 == "attribute-xdata",
             "DRW_Attrib copy isolates extended-data mutation");
    DRW_Attrib assignedAttrib;
    assignedAttrib = sourceAttrib;
    t.expect(assignedAttrib.mtext != nullptr
                 && assignedAttrib.mtext.get() != sourceAttrib.mtext.get()
                 && assignedAttrib.extData.front() != sourceAttrib.extData.front(),
             "DRW_Attrib assignment deep-copies owned state");

    DRW_GeoPositionMarker sourceMarker;
    sourceMarker.extData.push_back(
        std::make_shared<DRW_Variant>(1000, "marker-xdata"));
    sourceMarker.mtext = std::make_unique<DRW_MText>();
    sourceMarker.mtext->text = "marker";
    DRW_GeoPositionMarker copiedMarker(sourceMarker);
    t.expect(copiedMarker.mtext != nullptr
                 && copiedMarker.mtext.get() != sourceMarker.mtext.get(),
             "DRW_GeoPositionMarker copy deep-copies embedded MText");
    copiedMarker.mtext->text = "copy-marker";
    t.expect(sourceMarker.mtext->text == "marker",
             "DRW_GeoPositionMarker copy isolates embedded MText mutation");
    copiedMarker.extData.front()->addString(1000, "copy-marker-xdata");
    t.expect(std::string(sourceMarker.extData.front()->c_str())
                 == "marker-xdata",
             "DRW_GeoPositionMarker copy isolates extended-data mutation");
    DRW_GeoPositionMarker assignedMarker;
    assignedMarker = sourceMarker;
    t.expect(assignedMarker.mtext != nullptr
                 && assignedMarker.mtext.get() != sourceMarker.mtext.get()
                 && assignedMarker.extData.front() != sourceMarker.extData.front(),
             "DRW_GeoPositionMarker assignment deep-copies owned state");

    // Copying a model must start a fresh DXF parser walk.  In particular,
    // repeated group-code counters and subclass routing are transient and
    // must not leak through assignment from or into a partially parsed model.
    ExposedLWPolyline parsedPolyline;
    ExposedLWPolyline destinationPolyline;
    t.expect(parseDxfRecords(destinationPolyline, "90\n1\n"),
             "LWPOLYLINE parser accepts initial vertex count");
    DRW_LWPolyline plainPolyline;
    plainPolyline.vertexnum = 2;
    destinationPolyline.DRW_LWPolyline::operator=(plainPolyline);
    t.expect(parseDxfRecords(destinationPolyline, "90\n1\n"),
             "LWPOLYLINE assignment resets vertex-count parser state");
    t.expect(parseDxfRecords(parsedPolyline, "90\n1\n"),
             "LWPOLYLINE parser-state source setup");
    ExposedLWPolyline copiedPolylineState(parsedPolyline);
    t.expect(parseDxfRecords(copiedPolylineState, "90\n1\n"),
             "LWPOLYLINE copy starts a fresh vertex-count walk");

    ExposedAttrib parsedAttrib;
    ExposedAttrib assignedAttribState;
    t.expect(parseDxfRecords(assignedAttribState,
                             "100\nAcDbAttribute\n"),
             "ATTRIB parser subclass source setup");
    DRW_Attrib plainAttrib;
    assignedAttribState.DRW_Attrib::operator=(plainAttrib);
    t.expect(parseDxfRecords(assignedAttribState, "71\n2\n")
                 && assignedAttribState.textgen == 2
                 && assignedAttribState.m_attributeType == 1,
             "ATTRIB assignment resets subclass routing state");
    t.expect(parseDxfRecords(parsedAttrib,
                             "100\nAcDbAttribute\n71\n2\n"),
             "ATTRIB parser subclass state accepts attribute type");
    ExposedAttrib copiedAttribState(parsedAttrib);
    copiedAttribState.textgen = 0;
    copiedAttribState.m_attributeType = 1;
    t.expect(parseDxfRecords(copiedAttribState, "71\n2\n")
                 && copiedAttribState.textgen == 2
                 && copiedAttribState.m_attributeType == 1,
             "ATTRIB copy starts outside subclass routing");

    ExposedGeoPositionMarker parsedMarker;
    t.expect(parseDxfRecords(parsedMarker, "40\n1.5\n"),
             "GEOPOSITIONMARKER parser counter source setup");
    ExposedGeoPositionMarker copiedMarkerState(parsedMarker);
    t.expect(parseDxfRecords(copiedMarkerState, "40\n2.5\n")
                 && copiedMarkerState.m_radius == 2.5
                 && copiedMarkerState.m_landingGap == 0.0,
             "GEOPOSITIONMARKER copy resets repeated-double counter");
    ExposedGeoPositionMarker assignedMarkerState;
    assignedMarkerState = parsedMarker;
    t.expect(parseDxfRecords(assignedMarkerState, "40\n2.5\n")
                 && assignedMarkerState.m_radius == 2.5
                 && assignedMarkerState.m_landingGap == 0.0,
             "GEOPOSITIONMARKER assignment resets repeated-double counter");

    ExposedLeader partialLeader;
    partialLeader.vertnum = 2;
    t.expect(parseDxfRecords(partialLeader, "76\n2\n")
                 && !partialLeader.validateDxf(),
             "LEADER parser count marker source setup");
    ExposedLeader copiedLeaderState(partialLeader);
    t.expect(copiedLeaderState.validateDxf(),
             "LEADER copy resets vertex-count parser state");
    ExposedLeader assignedLeaderState;
    assignedLeaderState = partialLeader;
    t.expect(assignedLeaderState.validateDxf(),
             "LEADER assignment resets vertex-count parser state");

    ExposedMLine partialMLine;
    partialMLine.numVerts = 1;
    partialMLine.numLines = 1;
    t.expect(parseDxfRecords(partialMLine,
                             "11\n0\n21\n0\n31\n0\n74\n1\n"),
             "MLINE parser segment state source setup");
    ExposedMLine copiedMLineState(partialMLine);
    copiedMLineState.vertlist.clear();
    copiedMLineState.numVerts = 1;
    copiedMLineState.numLines = 1;
    t.expect(parseDxfRecords(copiedMLineState,
                             "11\n0\n21\n0\n31\n0\n74\n0\n75\n0\n"),
             "MLINE copy starts a fresh segment parser walk");
    ExposedMLine assignedMLineState;
    assignedMLineState = partialMLine;
    assignedMLineState.vertlist.clear();
    assignedMLineState.numVerts = 1;
    assignedMLineState.numLines = 1;
    t.expect(parseDxfRecords(assignedMLineState,
                             "11\n0\n21\n0\n31\n0\n74\n0\n75\n0\n"),
             "MLINE assignment starts a fresh segment parser walk");

    ExposedTable partialTable;
    t.expect(parseDxfRecords(partialTable,
                             "100\nAcDbTable\n91\n1\n92\n1\n"),
             "TABLE parser grid state source setup");
    ExposedTable copiedTableState(partialTable);
    copiedTableState.m_content.m_rows.clear();
    copiedTableState.m_content.m_columns.clear();
    t.expect(parseDxfRecords(copiedTableState,
                             "100\nAcDbTable\n91\n2\n92\n2\n")
                 && copiedTableState.m_content.m_rows.size() == 2u
                 && copiedTableState.m_content.m_columns.size() == 2u,
             "TABLE copy starts a fresh grid parser walk");
    ExposedTable assignedTableState;
    assignedTableState = partialTable;
    assignedTableState.m_content.m_rows.clear();
    assignedTableState.m_content.m_columns.clear();
    t.expect(parseDxfRecords(assignedTableState,
                             "100\nAcDbTable\n91\n2\n92\n2\n")
                 && assignedTableState.m_content.m_rows.size() == 2u
                 && assignedTableState.m_content.m_columns.size() == 2u,
             "TABLE assignment starts a fresh grid parser walk");

    ExposedMesh partialMesh;
    t.expect(parseDxfRecords(partialMesh,
                             "100\nAcDbSubDMesh\n92\n1\n10\n0\n20\n0\n30\n0\n93\n4\n90\n3\n"),
             "MESH parser face state source setup");
    ExposedMesh copiedMeshState(partialMesh);
    copiedMeshState.vertices.clear();
    copiedMeshState.faces.clear();
    copiedMeshState.edges.clear();
    copiedMeshState.creases.clear();
    copiedMeshState.propertyOverrides.clear();
    t.expect(parseDxfRecords(copiedMeshState,
                             "100\nAcDbSubDMesh\n92\n1\n10\n0\n20\n0\n30\n0\n93\n0\n94\n0\n95\n0\n90\n0\n"),
             "MESH copy starts a fresh topology parser walk");
    ExposedMesh assignedMeshState;
    assignedMeshState = partialMesh;
    assignedMeshState.vertices.clear();
    assignedMeshState.faces.clear();
    assignedMeshState.edges.clear();
    assignedMeshState.creases.clear();
    assignedMeshState.propertyOverrides.clear();
    t.expect(parseDxfRecords(assignedMeshState,
                             "100\nAcDbSubDMesh\n92\n1\n10\n0\n20\n0\n30\n0\n93\n0\n94\n0\n95\n0\n90\n0\n"),
             "MESH assignment starts a fresh topology parser walk");

    DRW_Underlay partialUnderlay;
    t.expect(parseDxfRecords(partialUnderlay, "11\n0\n"),
             "UNDERLAY parser clip state source setup");
    DRW_Underlay copiedUnderlayState(partialUnderlay);
    copiedUnderlayState.clipBoundary.clear();
    t.expect(parseDxfRecords(copiedUnderlayState, "11\n1\n")
                 && copiedUnderlayState.clipBoundary.size() == 1u
                 && copiedUnderlayState.clipBoundary.front().x == 1.0,
             "UNDERLAY copy starts a fresh clip parser walk");
    DRW_Underlay assignedUnderlayState;
    assignedUnderlayState = partialUnderlay;
    assignedUnderlayState.clipBoundary.clear();
    t.expect(parseDxfRecords(assignedUnderlayState, "11\n1\n")
                 && assignedUnderlayState.clipBoundary.size() == 1u
                 && assignedUnderlayState.clipBoundary.front().x == 1.0,
             "UNDERLAY assignment starts a fresh clip parser walk");

    const std::string navisworksComplete =
        "100\nAcDbNavisworksModel\n"
        "40\n1\n40\n2\n40\n3\n40\n4\n"
        "40\n5\n40\n6\n40\n7\n40\n8\n"
        "40\n9\n40\n10\n40\n11\n40\n12\n"
        "40\n13\n40\n14\n40\n15\n40\n16\n"
        "40\n2\n";
    ExposedNavisworksModel partialNavisworks;
    t.expect(parseDxfRecords(partialNavisworks,
                              "100\nAcDbNavisworksModel\n40\n99\n"),
             "NAVISWORKSMODEL parser state source setup");
    ExposedNavisworksModel copiedNavisworks(partialNavisworks);
    copiedNavisworks.transform.fill(0.0);
    t.expect(parseDxfRecords(copiedNavisworks, navisworksComplete)
                 && copiedNavisworks.finalizeDxf()
                 && copiedNavisworks.transform.front() == 1.0
                 && copiedNavisworks.transform.back() == 16.0
                 && copiedNavisworks.unitFactor == 2.0,
             "NAVISWORKSMODEL copy starts a fresh transform parser walk");
    ExposedNavisworksModel assignedNavisworks;
    assignedNavisworks = partialNavisworks;
    assignedNavisworks.transform.fill(0.0);
    t.expect(parseDxfRecords(assignedNavisworks, navisworksComplete)
                 && assignedNavisworks.finalizeDxf()
                 && assignedNavisworks.transform.front() == 1.0
                 && assignedNavisworks.transform.back() == 16.0
                 && assignedNavisworks.unitFactor == 2.0,
             "NAVISWORKSMODEL assignment starts a fresh transform parser walk");

    const std::string pointCloudComplete =
        "100\nAcDbPointCloud\n70\n7\n90\n0\n92\n42\n";
    ExposedPointCloud partialPointCloud;
    t.expect(parseDxfRecords(partialPointCloud,
                              "100\nAcDbPointCloud\n90\n0\n"),
             "POINTCLOUD parser state source setup");
    ExposedPointCloud copiedPointCloud(partialPointCloud);
    copiedPointCloud.classVersion = 0;
    t.expect(parseDxfRecords(copiedPointCloud, pointCloudComplete)
                 && copiedPointCloud.finalizeDxf()
                 && copiedPointCloud.classVersion == 7
                 && copiedPointCloud.pointCount == 42u,
             "POINTCLOUD copy starts a fresh body parser walk");
    ExposedPointCloud assignedPointCloud;
    assignedPointCloud = partialPointCloud;
    assignedPointCloud.classVersion = 0;
    t.expect(parseDxfRecords(assignedPointCloud, pointCloudComplete)
                 && assignedPointCloud.finalizeDxf()
                 && assignedPointCloud.classVersion == 7
                 && assignedPointCloud.pointCount == 42u,
             "POINTCLOUD assignment starts a fresh body parser walk");

    const std::string pointCloudExComplete =
        "100\nAcDbPointCloud\n70\n7\n92\n0\n93\n11\n93\n22\n";
    ExposedPointCloudEx partialPointCloudEx;
    t.expect(parseDxfRecords(partialPointCloudEx,
                              "100\nAcDbPointCloud\n92\n0\n"),
             "POINTCLOUDEX parser state source setup");
    ExposedPointCloudEx copiedPointCloudEx(partialPointCloudEx);
    copiedPointCloudEx.classVersion = 0;
    t.expect(parseDxfRecords(copiedPointCloudEx, pointCloudExComplete)
                 && copiedPointCloudEx.finalizeDxf()
                 && copiedPointCloudEx.classVersion == 7
                 && copiedPointCloudEx.unknownInt0 == 11
                 && copiedPointCloudEx.unknownInt1 == 22,
             "POINTCLOUDEX copy starts a fresh body parser walk");
    ExposedPointCloudEx assignedPointCloudEx;
    assignedPointCloudEx = partialPointCloudEx;
    assignedPointCloudEx.classVersion = 0;
    t.expect(parseDxfRecords(assignedPointCloudEx, pointCloudExComplete)
                 && assignedPointCloudEx.finalizeDxf()
                 && assignedPointCloudEx.classVersion == 7
                 && assignedPointCloudEx.unknownInt0 == 11
                 && assignedPointCloudEx.unknownInt1 == 22,
             "POINTCLOUDEX assignment starts a fresh body parser walk");

    ExposedNurbsSurface partialNurbsSurface;
    t.expect(parseDxfRecords(partialNurbsSurface,
                              "100\nAcDbNurbSurface\n10\n9\n"),
             "NURBSURFACE parser state source setup");
    ExposedNurbsSurface copiedNurbsSurface(partialNurbsSurface);
    copiedNurbsSurface.uvec1 = DRW_Coord{};
    t.expect(parseDxfRecords(copiedNurbsSurface,
                             "100\nAcDbNurbSurface\n170\n2\n")
                 && copiedNurbsSurface.finalizeDxf()
                 && copiedNurbsSurface.short170 == 2u,
             "NURBSURFACE copy starts a fresh coordinate parser walk");
    ExposedNurbsSurface assignedNurbsSurface;
    assignedNurbsSurface = partialNurbsSurface;
    assignedNurbsSurface.uvec1 = DRW_Coord{};
    t.expect(parseDxfRecords(assignedNurbsSurface,
                             "100\nAcDbNurbSurface\n170\n2\n")
                 && assignedNurbsSurface.finalizeDxf()
                 && assignedNurbsSurface.short170 == 2u,
             "NURBSURFACE assignment starts a fresh coordinate parser walk");

    ExposedRevolvedSurface partialRevolvedSurface;
    t.expect(parseDxfRecords(partialRevolvedSurface, "90\n1\n"),
             "REVOLVEDSURFACE parser state source setup");
    ExposedRevolvedSurface copiedRevolvedSurface(partialRevolvedSurface);
    copiedRevolvedSurface.classId = 0;
    copiedRevolvedSurface.id = 0;
    t.expect(parseDxfRecords(copiedRevolvedSurface, "90\n2\n")
                 && copiedRevolvedSurface.classId == 2u
                 && copiedRevolvedSurface.id == 0u,
             "REVOLVEDSURFACE copy starts a fresh class-id parser walk");
    ExposedRevolvedSurface assignedRevolvedSurface;
    assignedRevolvedSurface = partialRevolvedSurface;
    assignedRevolvedSurface.classId = 0;
    assignedRevolvedSurface.id = 0;
    t.expect(parseDxfRecords(assignedRevolvedSurface, "90\n2\n")
                 && assignedRevolvedSurface.classId == 2u
                 && assignedRevolvedSurface.id == 0u,
             "REVOLVEDSURFACE assignment starts a fresh class-id parser walk");

    ExposedExtrudedSurface partialExtrudedSurface;
    t.expect(parseDxfRecords(partialExtrudedSurface,
                             "100\nAcDbExtrudedSurface\n90\n1\n"),
             "EXTRUDEDSURFACE parser state source setup");
    ExposedExtrudedSurface copiedExtrudedSurface(partialExtrudedSurface);
    copiedExtrudedSurface.classId = 0;
    t.expect(parseDxfRecords(copiedExtrudedSurface,
                             "100\nAcDbExtrudedSurface\n90\n2\n")
                 && copiedExtrudedSurface.classId == 2u,
             "EXTRUDEDSURFACE copy starts a fresh class-id parser walk");
    ExposedExtrudedSurface assignedExtrudedSurface;
    assignedExtrudedSurface = partialExtrudedSurface;
    assignedExtrudedSurface.classId = 0;
    t.expect(parseDxfRecords(assignedExtrudedSurface,
                             "100\nAcDbExtrudedSurface\n90\n2\n")
                 && assignedExtrudedSurface.classId == 2u,
             "EXTRUDEDSURFACE assignment starts a fresh class-id parser walk");

    ExposedSweptSurface partialSweptSurface;
    t.expect(parseDxfRecords(partialSweptSurface,
                             "100\nAcDbSweptSurface\n90\n1\n"),
             "SWEPTSURFACE parser state source setup");
    ExposedSweptSurface copiedSweptSurface(partialSweptSurface);
    copiedSweptSurface.sweepEntityId = 0;
    t.expect(parseDxfRecords(copiedSweptSurface,
                             "100\nAcDbSweptSurface\n90\n2\n")
                 && copiedSweptSurface.sweepEntityId == 2u,
             "SWEPTSURFACE copy starts a fresh entity-id parser walk");
    ExposedSweptSurface assignedSweptSurface;
    assignedSweptSurface = partialSweptSurface;
    assignedSweptSurface.sweepEntityId = 0;
    t.expect(parseDxfRecords(assignedSweptSurface,
                             "100\nAcDbSweptSurface\n90\n2\n")
                 && assignedSweptSurface.sweepEntityId == 2u,
             "SWEPTSURFACE assignment starts a fresh entity-id parser walk");

    const std::string loftedTransform =
        "100\nAcDbLoftedSurface\n"
        "40\n1\n40\n2\n40\n3\n40\n4\n"
        "40\n5\n40\n6\n40\n7\n40\n8\n"
        "40\n9\n40\n10\n40\n11\n40\n12\n"
        "40\n13\n40\n14\n40\n15\n40\n16\n";
    ExposedLoftedSurface partialLoftedSurface;
    t.expect(parseDxfRecords(partialLoftedSurface,
                             "100\nAcDbLoftedSurface\n40\n99\n"),
             "LOFTEDSURFACE parser state source setup");
    ExposedLoftedSurface copiedLoftedSurface(partialLoftedSurface);
    copiedLoftedSurface.loftEntityTransform.fill(0.0);
    t.expect(parseDxfRecords(copiedLoftedSurface, loftedTransform)
                 && copiedLoftedSurface.finalizeDxf()
                 && copiedLoftedSurface.loftEntityTransform.front() == 1.0
                 && copiedLoftedSurface.loftEntityTransform.back() == 16.0,
             "LOFTEDSURFACE copy starts a fresh transform parser walk");
    ExposedLoftedSurface assignedLoftedSurface;
    assignedLoftedSurface = partialLoftedSurface;
    assignedLoftedSurface.loftEntityTransform.fill(0.0);
    t.expect(parseDxfRecords(assignedLoftedSurface, loftedTransform)
                 && assignedLoftedSurface.finalizeDxf()
                 && assignedLoftedSurface.loftEntityTransform.front() == 1.0
                 && assignedLoftedSurface.loftEntityTransform.back() == 16.0,
             "LOFTEDSURFACE assignment starts a fresh transform parser walk");

    ExposedImage partialImage;
    t.expect(parseDxfRecords(partialImage, "91\n1\n14\n1\n"),
             "IMAGE parser state source setup");
    ExposedImage copiedImage(partialImage);
    copiedImage.clipPath.clear();
    t.expect(parseDxfRecords(copiedImage, "14\n2\n24\n3\n")
                 && copiedImage.clipPath.size() == 1u
                 && copiedImage.clipPath.front().x == 2.0
                 && copiedImage.clipPath.front().y == 3.0,
             "IMAGE copy starts a fresh clip-vertex parser walk");
    ExposedImage assignedImage;
    assignedImage = partialImage;
    assignedImage.clipPath.clear();
    t.expect(parseDxfRecords(assignedImage, "14\n2\n24\n3\n")
                 && assignedImage.clipPath.size() == 1u
                 && assignedImage.clipPath.front().x == 2.0
                 && assignedImage.clipPath.front().y == 3.0,
             "IMAGE assignment starts a fresh clip-vertex parser walk");

    ExposedSectionObject partialSection;
    t.expect(parseDxfRecords(partialSection,
                             "100\nAcDbSection\n90\n1\n"),
             "SECTIONOBJECT parser state source setup");
    ExposedSectionObject copiedSection(partialSection);
    t.expect(parseDxfRecords(copiedSection, "90\n2\n")
                 && copiedSection.m_state == 1u
                 && parseDxfRecords(copiedSection,
                                    "100\nAcDbSection\n90\n2\n")
                 && copiedSection.m_state == 2u,
             "SECTIONOBJECT copy starts a fresh subclass parser walk");
    ExposedSectionObject assignedSection;
    assignedSection = partialSection;
    t.expect(parseDxfRecords(assignedSection, "90\n2\n")
                 && assignedSection.m_state == 1u
                 && parseDxfRecords(assignedSection,
                                    "100\nAcDbSection\n90\n2\n")
                 && assignedSection.m_state == 2u,
             "SECTIONOBJECT assignment starts a fresh subclass parser walk");

    ExposedOle2Frame partialOle2;
    t.expect(parseDxfRecords(partialOle2, "90\n4\n310\n4142\n")
                 && partialOle2.m_dxfPayloadLengthSpecified,
             "OLE2FRAME parser state source setup");
    ExposedOle2Frame copiedOle2(partialOle2);
    t.expect(!copiedOle2.m_dxfPayloadLengthSpecified
                 && copiedOle2.m_payloadBytes.size() == 2u
                 && copiedOle2.m_payloadBytes[0] == 0x41u
                 && copiedOle2.m_payloadBytes[1] == 0x42u,
             "OLE2FRAME copy resets payload-length parser marker");
    ExposedOle2Frame assignedOle2;
    t.expect(parseDxfRecords(assignedOle2, "90\n9\n")
                 && assignedOle2.m_dxfPayloadLengthSpecified,
             "OLE2FRAME assignment destination setup");
    assignedOle2 = partialOle2;
    t.expect(!assignedOle2.m_dxfPayloadLengthSpecified
                 && assignedOle2.m_payloadBytes.size() == 2u,
             "OLE2FRAME assignment resets payload-length parser marker");

    ExposedOleFrame partialOle;
    t.expect(parseDxfRecords(partialOle, "90\n4\n310\n4344\n")
                 && partialOle.m_dxfPayloadLengthSpecified,
             "OLEFRAME parser state source setup");
    ExposedOleFrame copiedOle(partialOle);
    t.expect(!copiedOle.m_dxfPayloadLengthSpecified
                 && copiedOle.m_payloadBytes.size() == 2u
                 && copiedOle.m_payloadBytes[0] == 0x43u
                 && copiedOle.m_payloadBytes[1] == 0x44u,
             "OLEFRAME copy resets payload-length parser marker");
    ExposedOleFrame assignedOle;
    t.expect(parseDxfRecords(assignedOle, "90\n9\n")
                 && assignedOle.m_dxfPayloadLengthSpecified,
             "OLEFRAME assignment destination setup");
    assignedOle = partialOle;
    t.expect(!assignedOle.m_dxfPayloadLengthSpecified
                 && assignedOle.m_payloadBytes.size() == 2u,
             "OLEFRAME assignment resets payload-length parser marker");

    ExposedBreakData partialBreakData;
    t.expect(parseDxfRecords(partialBreakData,
                             "100\nAcDbBreakData\n90\n0\n")
                 && partialBreakData.m_dimensionHandle == 0u,
             "BREAKDATA parser state source setup");
    ExposedBreakData copiedBreakData(partialBreakData);
    t.expect(parseDxfRecords(copiedBreakData, "331\nB2\n")
                 && copiedBreakData.m_dimensionHandle == 0u
                 && parseDxfRecords(copiedBreakData,
                                    "100\nAcDbBreakData\n331\nB2\n")
                 && copiedBreakData.m_dimensionHandle == 0xB2u,
             "BREAKDATA copy starts a fresh subclass parser walk");
    ExposedBreakData assignedBreakData;
    assignedBreakData = partialBreakData;
    t.expect(parseDxfRecords(assignedBreakData, "331\nB3\n")
                 && assignedBreakData.m_dimensionHandle == 0u
                 && parseDxfRecords(assignedBreakData,
                                    "100\nAcDbBreakData\n331\nB3\n")
                 && assignedBreakData.m_dimensionHandle == 0xB3u,
             "BREAKDATA assignment starts a fresh subclass parser walk");

    ExposedDictionary partialDictionary;
    t.expect(parseDxfRecords(partialDictionary, "3\nfirst\n"),
             "DICTIONARY parser state source setup");
    ExposedDictionary copiedDictionary(partialDictionary);
    t.expect(!parseDxfRecords(copiedDictionary, "350\nA1\n")
                 && parseDxfRecords(copiedDictionary,
                                    "3\nfresh\n350\nA2\n")
                 && copiedDictionary.m_entries.size() == 1u
                 && copiedDictionary.m_entries.front().m_name == "fresh"
                 && copiedDictionary.m_entries.front().m_handle == 0xA2u,
             "DICTIONARY copy clears pending-entry parser state");
    ExposedDictionary assignedDictionary;
    assignedDictionary = partialDictionary;
    t.expect(!parseDxfRecords(assignedDictionary, "350\nA1\n")
                 && parseDxfRecords(assignedDictionary,
                                    "3\nfresh\n350\nA2\n")
                 && assignedDictionary.m_entries.size() == 1u
                 && assignedDictionary.m_entries.front().m_handle == 0xA2u,
             "DICTIONARY assignment clears pending-entry parser state");

    ExposedDictionaryWithDefault partialDictionaryDefault;
    t.expect(parseDxfRecords(partialDictionaryDefault, "340\nA1\n")
                 && partialDictionaryDefault.m_defaultEntryHandle == 0xA1u,
             "DICTIONARYWDFLT parser state source setup");
    ExposedDictionaryWithDefault copiedDictionaryDefault(
        partialDictionaryDefault);
    t.expect(parseDxfRecords(copiedDictionaryDefault, "340\nA2\n")
                 && copiedDictionaryDefault.m_defaultEntryHandle == 0xA2u,
             "DICTIONARYWDFLT copy clears default-entry parser state");
    ExposedDictionaryWithDefault assignedDictionaryDefault;
    t.expect(parseDxfRecords(assignedDictionaryDefault, "340\nA3\n"),
             "DICTIONARYWDFLT assignment destination setup");
    assignedDictionaryDefault = partialDictionaryDefault;
    t.expect(parseDxfRecords(assignedDictionaryDefault, "340\nA4\n")
                 && assignedDictionaryDefault.m_defaultEntryHandle == 0xA4u,
             "DICTIONARYWDFLT assignment clears default-entry parser state");

    ExposedXRecord partialXRecord;
    t.expect(parseDxfRecords(partialXRecord,
                             "100\nAcDbXrecord\n1\nold-value\n")
                 && partialXRecord.m_values.size() == 1u,
             "XRECORD parser state source setup");
    ExposedXRecord copiedXRecord(partialXRecord);
    const bool copiedStale = parseDxfRecords(copiedXRecord, "1\nstale-value\n");
    const std::size_t copiedAfterStale = copiedXRecord.m_values.size();
    const bool copiedFresh = parseDxfRecords(copiedXRecord,
                                    "100\nAcDbXrecord\n1\nfresh-value\n");
    t.expect(copiedStale && copiedAfterStale == 1u
                 && copiedFresh && copiedXRecord.m_values.size() == 2u,
             "XRECORD copy clears subclass-body parser state");
    ExposedXRecord assignedXRecord;
    assignedXRecord = partialXRecord;
    const bool assignedStale = parseDxfRecords(assignedXRecord, "1\nstale-value\n");
    const std::size_t assignedAfterStale = assignedXRecord.m_values.size();
    const bool assignedFresh = parseDxfRecords(assignedXRecord,
                                    "100\nAcDbXrecord\n1\nfresh-value\n");
    t.expect(assignedStale && assignedAfterStale == 1u
                 && assignedFresh && assignedXRecord.m_values.size() == 2u,
             "XRECORD assignment clears subclass-body parser state");

    const std::string sortEntsBody =
        "100\nAcDbSortentsTable\n330\nA1\n331\nB1\n5\nC1\n";
    ExposedSortEntsTable partialSortEnts;
    t.expect(parseDxfRecords(partialSortEnts, sortEntsBody)
                 && partialSortEnts.m_entityHandles.size() == 1u
                 && partialSortEnts.m_sortHandles.size() == 1u,
             "SORTENTSTABLE parser state source setup");
    ExposedSortEntsTable copiedSortEnts(partialSortEnts);
    const bool copiedSortStale = parseDxfRecords(copiedSortEnts, "5\nC2\n");
    const std::size_t copiedSortAfterStale =
        copiedSortEnts.m_entityHandles.size();
    copiedSortEnts.m_entityHandles.clear();
    copiedSortEnts.m_sortHandles.clear();
    copiedSortEnts.m_blockOwnerHandle = DRW::NoHandle;
    const bool copiedSortFresh = parseDxfRecords(
        copiedSortEnts,
        "100\nAcDbSortentsTable\n330\nA2\n331\nB2\n5\nC2\n");
    t.expect(copiedSortStale && copiedSortAfterStale == 1u
                 && copiedSortFresh
                 && copiedSortEnts.m_entityHandles.size() == 1u
                 && copiedSortEnts.m_sortHandles.size() == 1u,
             "SORTENTSTABLE copy clears body parser state");
    ExposedSortEntsTable assignedSortEnts;
    assignedSortEnts = partialSortEnts;
    const bool assignedSortStale = parseDxfRecords(assignedSortEnts, "5\nC3\n");
    const std::size_t assignedSortAfterStale =
        assignedSortEnts.m_entityHandles.size();
    assignedSortEnts.m_entityHandles.clear();
    assignedSortEnts.m_sortHandles.clear();
    assignedSortEnts.m_blockOwnerHandle = DRW::NoHandle;
    const bool assignedSortFresh = parseDxfRecords(
        assignedSortEnts,
        "100\nAcDbSortentsTable\n330\nA3\n331\nB3\n5\nC3\n");
    t.expect(assignedSortStale && assignedSortAfterStale == 1u
                 && assignedSortFresh
                 && assignedSortEnts.m_entityHandles.size() == 1u
                 && assignedSortEnts.m_sortHandles.size() == 1u,
             "SORTENTSTABLE assignment clears body parser state");

    ExposedIndex partialIndex;
    t.expect(parseDxfRecords(partialIndex, "40\n1.5\n")
                 && partialIndex.timestamp1 == 1u,
             "INDEX parser state source setup");
    ExposedIndex copiedIndex(partialIndex);
    t.expect(parseDxfRecords(copiedIndex, "40\n2.5\n")
                 && copiedIndex.timestamp1 == 2u
                 && copiedIndex.timestamp2 == 43200000u,
             "INDEX copy clears timestamp parser state");
    ExposedIndex assignedIndex;
    assignedIndex = partialIndex;
    t.expect(parseDxfRecords(assignedIndex, "40\n3.5\n")
                 && assignedIndex.timestamp1 == 3u
                 && assignedIndex.timestamp2 == 43200000u,
             "INDEX assignment clears timestamp parser state");

    ExposedIdBuffer partialIdBuffer;
    t.expect(parseDxfRecords(partialIdBuffer,
                             "100\nAcDbIdBuffer\n70\n1\n330\nA1\n")
                 && partialIdBuffer.objIds.size() == 1u,
             "IDBUFFER parser state source setup");
    ExposedIdBuffer copiedIdBuffer(partialIdBuffer);
    const bool copiedIdStale = parseDxfRecords(copiedIdBuffer, "330\nA2\n");
    const std::size_t copiedIdAfterStale = copiedIdBuffer.objIds.size();
    const bool copiedIdFresh = parseDxfRecords(
        copiedIdBuffer, "100\nAcDbIdBuffer\n330\nA2\n");
    t.expect(copiedIdStale && copiedIdAfterStale == 1u
                 && copiedIdFresh && copiedIdBuffer.objIds.size() == 2u,
             "IDBUFFER copy clears body parser state");
    ExposedIdBuffer assignedIdBuffer;
    assignedIdBuffer = partialIdBuffer;
    const bool assignedIdStale = parseDxfRecords(assignedIdBuffer, "330\nA3\n");
    const std::size_t assignedIdAfterStale = assignedIdBuffer.objIds.size();
    const bool assignedIdFresh = parseDxfRecords(
        assignedIdBuffer, "100\nAcDbIdBuffer\n330\nA3\n");
    t.expect(assignedIdStale && assignedIdAfterStale == 1u
                 && assignedIdFresh && assignedIdBuffer.objIds.size() == 2u,
             "IDBUFFER assignment clears body parser state");

    const std::string layerIndexBody =
        "40\n1.5\n90\n1\n8\nlayer-one\n360\nA1\n";
    ExposedLayerIndex partialLayerIndex;
    t.expect(parseDxfRecords(partialLayerIndex, layerIndexBody)
                 && partialLayerIndex.finalizeDxf()
                 && partialLayerIndex.entries.size() == 1u,
             "LAYER_INDEX parser state source setup");
    ExposedLayerIndex copiedLayerIndex(partialLayerIndex);
    t.expect(parseDxfRecords(copiedLayerIndex,
                             "40\n2.5\n90\n2\n8\nlayer-two\n360\nA2\n")
                 && copiedLayerIndex.entries.size() == 2u
                 && copiedLayerIndex.entries.back().name == "layer-two",
             "LAYER_INDEX copy clears timestamp and entry parser state");
    ExposedLayerIndex assignedLayerIndex;
    assignedLayerIndex = partialLayerIndex;
    t.expect(parseDxfRecords(assignedLayerIndex,
                             "40\n3.5\n90\n3\n8\nlayer-three\n360\nA3\n")
                 && assignedLayerIndex.entries.size() == 2u
                 && assignedLayerIndex.entries.back().name == "layer-three",
             "LAYER_INDEX assignment clears timestamp and entry parser state");

    ExposedSpatialIndex partialSpatialIndex;
    t.expect(parseDxfRecords(partialSpatialIndex, "40\n1.5\n")
                 && partialSpatialIndex.timestamp1 == 1u,
             "SPATIAL_INDEX parser state source setup");
    ExposedSpatialIndex copiedSpatialIndex(partialSpatialIndex);
    t.expect(parseDxfRecords(copiedSpatialIndex, "40\n2.5\n")
                 && copiedSpatialIndex.timestamp1 == 2u
                 && copiedSpatialIndex.timestamp2 == 43200000u,
             "SPATIAL_INDEX copy clears timestamp parser state");
    ExposedSpatialIndex assignedSpatialIndex;
    assignedSpatialIndex = partialSpatialIndex;
    t.expect(parseDxfRecords(assignedSpatialIndex, "40\n3.5\n")
                 && assignedSpatialIndex.timestamp1 == 3u
                 && assignedSpatialIndex.timestamp2 == 43200000u,
             "SPATIAL_INDEX assignment clears timestamp parser state");

    const std::string lightListBody =
        "100\nAcDbLightList\n90\n1\n90\n1\n5\nA1\n1\nlight-one\n";
    ExposedLightList partialLightList;
    t.expect(parseDxfRecords(partialLightList, lightListBody)
                 && partialLightList.m_lights.size() == 1u
                 && partialLightList.m_lights.front().m_name == "light-one",
             "LIGHTLIST parser state source setup");
    ExposedLightList copiedLightList(partialLightList);
    const bool copiedLightStale = parseDxfRecords(copiedLightList,
                                                   "1\nstale-light\n");
    const std::size_t copiedLightAfterStale = copiedLightList.m_lights.size();
    const bool copiedLightFresh = parseDxfRecords(
        copiedLightList,
        "100\nAcDbLightList\n90\n2\n90\n2\n5\nA2\n1\nlight-two\n");
    t.expect(copiedLightStale && copiedLightAfterStale == 1u
                 && copiedLightFresh && copiedLightList.m_lights.size() == 2u
                 && copiedLightList.m_lights.back().m_name == "light-two",
             "LIGHTLIST copy clears subclass parser state");
    ExposedLightList assignedLightList;
    assignedLightList = partialLightList;
    const bool assignedLightStale = parseDxfRecords(assignedLightList,
                                                     "1\nstale-light\n");
    const std::size_t assignedLightAfterStale = assignedLightList.m_lights.size();
    const bool assignedLightFresh = parseDxfRecords(
        assignedLightList,
        "100\nAcDbLightList\n90\n3\n90\n3\n5\nA3\n1\nlight-three\n");
    t.expect(assignedLightStale && assignedLightAfterStale == 1u
                 && assignedLightFresh && assignedLightList.m_lights.size() == 2u
                 && assignedLightList.m_lights.back().m_name == "light-three",
             "LIGHTLIST assignment clears subclass parser state");

    ExposedGeoMapImage partialGeoMapImage;
    t.expect(parseDxfRecords(partialGeoMapImage,
                             "100\nAcDbGeomapImage\n90\n1\n")
                 && partialGeoMapImage.m_classVersion == 1,
             "GEOMAPIMAGE parser state source setup");
    ExposedGeoMapImage copiedGeoMapImage(partialGeoMapImage);
    const bool copiedGeoStale = parseDxfRecords(copiedGeoMapImage, "90\n2\n");
    const std::int32_t copiedGeoAfterStale = copiedGeoMapImage.m_classVersion;
    const bool copiedGeoFresh = parseDxfRecords(copiedGeoMapImage,
                                                "100\nAcDbGeomapImage\n90\n2\n10\n4\n");
    t.expect(copiedGeoStale && copiedGeoAfterStale == 1
                 && copiedGeoFresh && copiedGeoMapImage.m_classVersion == 2
                 && copiedGeoMapImage.m_insertionPoint.x == 4.0,
             "GEOMAPIMAGE copy clears subclass parser state");
    ExposedGeoMapImage assignedGeoMapImage;
    assignedGeoMapImage = partialGeoMapImage;
    const bool assignedGeoStale = parseDxfRecords(assignedGeoMapImage, "90\n3\n");
    const std::int32_t assignedGeoAfterStale = assignedGeoMapImage.m_classVersion;
    const bool assignedGeoFresh = parseDxfRecords(assignedGeoMapImage,
                                                  "100\nAcDbGeomapImage\n90\n3\n10\n5\n");
    t.expect(assignedGeoStale && assignedGeoAfterStale == 1
                 && assignedGeoFresh && assignedGeoMapImage.m_classVersion == 3
                 && assignedGeoMapImage.m_insertionPoint.x == 5.0,
             "GEOMAPIMAGE assignment clears subclass parser state");

    const std::string dataLinkBody =
        "100\nAcDbDataLink\n1\nSQLite\n300\ndescription\n"
        "1\nCUSTOMDATA\n330\nA1\n304\nfirst-row\n309\n";
    ExposedDataLink partialDataLink;
    t.expect(parseDxfRecords(partialDataLink, dataLinkBody)
                 && partialDataLink.m_dataAdapter == "SQLite"
                 && partialDataLink.m_customData.size() == 1u
                 && partialDataLink.m_customData.front().m_text == "first-row",
             "DATALINK parser state source setup");
    ExposedDataLink copiedDataLink(partialDataLink);
    const bool copiedDataStale = parseDxfRecords(copiedDataLink,
                                                  "304\nstale-row\n");
    const std::size_t copiedDataAfterStale = copiedDataLink.m_customData.size();
    const bool copiedDataFresh = parseDxfRecords(
        copiedDataLink,
        "100\nAcDbDataLink\n1\nCUSTOMDATA\n330\nA2\n304\nsecond-row\n309\n");
    t.expect(copiedDataStale && copiedDataAfterStale == 1u
                 && copiedDataFresh && copiedDataLink.m_customData.size() == 2u
                 && copiedDataLink.m_customData.back().m_text == "second-row",
             "DATALINK copy clears body/custom-data parser state");
    ExposedDataLink assignedDataLink;
    assignedDataLink = partialDataLink;
    const bool assignedDataStale = parseDxfRecords(assignedDataLink,
                                                    "304\nstale-row\n");
    const std::size_t assignedDataAfterStale = assignedDataLink.m_customData.size();
    const bool assignedDataFresh = parseDxfRecords(
        assignedDataLink,
        "100\nAcDbDataLink\n1\nCUSTOMDATA\n330\nA3\n304\nthird-row\n309\n");
    t.expect(assignedDataStale && assignedDataAfterStale == 1u
                 && assignedDataFresh && assignedDataLink.m_customData.size() == 2u
                 && assignedDataLink.m_customData.back().m_text == "third-row",
             "DATALINK assignment clears body/custom-data parser state");

    const std::string geoDataBody =
        "100\nAcDbGeoData\n330\nA1\n90\n1\n93\n1\n"
        "13\n1\n23\n2\n14\n3\n24\n4\n";
    ExposedGeoData partialGeoData;
    t.expect(parseDxfRecords(partialGeoData, geoDataBody)
                 && partialGeoData.m_hostBlockHandle == 0xA1u
                 && partialGeoData.m_points.size() == 1u
                 && partialGeoData.m_points.front().m_source.y == 2.0,
             "GEODATA parser state source setup");
    ExposedGeoData copiedGeoData(partialGeoData);
    const bool copiedGeoDataStale = parseDxfRecords(copiedGeoData, "330\nA2\n");
    const std::uint32_t copiedGeoDataHostAfterStale =
        copiedGeoData.m_hostBlockHandle;
    const bool copiedGeoDataFresh = parseDxfRecords(
        copiedGeoData, "100\nAcDbGeoData\n330\nA3\n90\n2\n");
    t.expect(copiedGeoDataStale && copiedGeoDataHostAfterStale == 0xA1u
                 && copiedGeoDataFresh && copiedGeoData.m_hostBlockHandle == 0xA3u
                 && copiedGeoData.m_version == 2
                 && copiedGeoData.m_points.size() == 1u,
             "GEODATA copy clears subclass and mesh parser state");
    ExposedGeoData assignedGeoData;
    assignedGeoData = partialGeoData;
    const bool assignedGeoDataStale = parseDxfRecords(assignedGeoData, "330\nA4\n");
    const std::uint32_t assignedGeoDataHostAfterStale =
        assignedGeoData.m_hostBlockHandle;
    const bool assignedGeoDataFresh = parseDxfRecords(
        assignedGeoData, "100\nAcDbGeoData\n330\nA5\n90\n3\n");
    t.expect(assignedGeoDataStale && assignedGeoDataHostAfterStale == 0xA1u
                 && assignedGeoDataFresh && assignedGeoData.m_hostBlockHandle == 0xA5u
                 && assignedGeoData.m_version == 3
                 && assignedGeoData.m_points.size() == 1u,
             "GEODATA assignment clears subclass and mesh parser state");

    std::ostringstream spatialFilterRecords;
    for (int i = 0; i < 25; ++i)
        spatialFilterRecords << "40\n" << (i + 1) << "\n";
    ExposedSpatialFilter partialSpatialFilter;
    t.expect(parseDxfRecords(partialSpatialFilter, spatialFilterRecords.str())
                 && partialSpatialFilter.m_inverseInsertTransform.size() == 12u
                 && partialSpatialFilter.m_insertTransform.size() == 12u
                 && partialSpatialFilter.m_frontDistance == 1.0,
             "SPATIAL_FILTER parser state source setup");
    ExposedSpatialFilter copiedSpatialFilter(partialSpatialFilter);
    t.expect(parseDxfRecords(copiedSpatialFilter, "40\n99\n")
                 && copiedSpatialFilter.m_inverseInsertTransform.size() == 12u
                 && copiedSpatialFilter.m_insertTransform.size() == 12u
                 && copiedSpatialFilter.m_frontDistance == 1.0,
             "SPATIAL_FILTER copy clears matrix parser state");
    ExposedSpatialFilter assignedSpatialFilter;
    assignedSpatialFilter = partialSpatialFilter;
    t.expect(parseDxfRecords(assignedSpatialFilter, "40\n98\n")
                 && assignedSpatialFilter.m_inverseInsertTransform.size() == 12u
                 && assignedSpatialFilter.m_insertTransform.size() == 12u
                 && assignedSpatialFilter.m_frontDistance == 1.0,
             "SPATIAL_FILTER assignment clears matrix parser state");

    const std::string evaluationGraphHeader =
        "100\nAcDbEvalGraph\n96\n1\n97\n2\n";
    ExposedEvaluationGraph partialEvaluationGraph;
    t.expect(parseDxfRecords(partialEvaluationGraph, evaluationGraphHeader)
                 && partialEvaluationGraph.m_value96 == 1
                 && partialEvaluationGraph.m_value97 == 2,
             "EVALUATION_GRAPH parser state source setup");
    ExposedEvaluationGraph copiedEvaluationGraph(partialEvaluationGraph);
    const bool copiedEvaluationStale = parseDxfRecords(copiedEvaluationGraph,
                                                        "96\n9\n");
    const bool copiedEvaluationFresh = parseDxfRecords(
        copiedEvaluationGraph,
        "100\nAcDbEvalGraph\n96\n3\n97\n4\n");
    t.expect(copiedEvaluationStale && copiedEvaluationFresh
                 && copiedEvaluationGraph.finalizeDxf()
                 && copiedEvaluationGraph.m_value96 == 3
                 && copiedEvaluationGraph.m_value97 == 4,
             "EVALUATION_GRAPH copy clears malformed parser state");
    ExposedEvaluationGraph assignedEvaluationGraph;
    assignedEvaluationGraph = partialEvaluationGraph;
    const bool assignedEvaluationStale = parseDxfRecords(assignedEvaluationGraph,
                                                          "96\n9\n");
    const bool assignedEvaluationFresh = parseDxfRecords(
        assignedEvaluationGraph,
        "100\nAcDbEvalGraph\n96\n5\n97\n6\n");
    t.expect(assignedEvaluationStale && assignedEvaluationFresh
                 && assignedEvaluationGraph.finalizeDxf()
                 && assignedEvaluationGraph.m_value96 == 5
                 && assignedEvaluationGraph.m_value97 == 6,
             "EVALUATION_GRAPH assignment clears malformed parser state");

    const std::string dimAssocBody =
        "100\nAcDbDimAssoc\n330\nA1\n90\n3\n1\nPOINT\n72\n1\n331\nB1\n";
    ExposedDimensionAssociation partialDimAssoc;
    t.expect(parseDxfRecords(partialDimAssoc, dimAssocBody)
                 && partialDimAssoc.m_dimensionHandle == 0xA1u
                 && partialDimAssoc.m_osnapRefs.size() == 1u
                 && partialDimAssoc.m_hasUnrepresentableDetail,
             "DIMASSOC parser state source setup");
    ExposedDimensionAssociation copiedDimAssoc(partialDimAssoc);
    const bool copiedDimAssocStale = parseDxfRecords(copiedDimAssoc, "330\nA2\n");
    const std::uint32_t copiedDimAssocAfterStale =
        copiedDimAssoc.m_dimensionHandle;
    const bool copiedDimAssocFresh = parseDxfRecords(
        copiedDimAssoc, "100\nAcDbDimAssoc\n330\nA3\n");
    t.expect(copiedDimAssocStale && copiedDimAssocAfterStale == 0xA1u
                 && copiedDimAssocFresh && copiedDimAssoc.m_dimensionHandle == 0xA3u
                 && copiedDimAssoc.m_osnapRefs.size() == 1u,
             "DIMASSOC copy clears body parser state");
    ExposedDimensionAssociation assignedDimAssoc;
    assignedDimAssoc = partialDimAssoc;
    const bool assignedDimAssocStale = parseDxfRecords(assignedDimAssoc, "330\nA4\n");
    const std::uint32_t assignedDimAssocAfterStale =
        assignedDimAssoc.m_dimensionHandle;
    const bool assignedDimAssocFresh = parseDxfRecords(
        assignedDimAssoc, "100\nAcDbDimAssoc\n330\nA5\n");
    t.expect(assignedDimAssocStale && assignedDimAssocAfterStale == 0xA1u
                 && assignedDimAssocFresh && assignedDimAssoc.m_dimensionHandle == 0xA5u
                 && assignedDimAssoc.m_osnapRefs.size() == 1u,
             "DIMASSOC assignment clears body parser state");

    const std::string pointCloudColorMapBody =
        "100\nAcDbPointCloudColorMap\n90\n7\n"
        "1\nintensity\n1\nelevation\n1\nclassification\n"
        "91\n2\n90\n3\n1\nramp-one\n";
    ExposedPointCloudColorMap partialPointCloudColorMap;
    t.expect(parseDxfRecords(partialPointCloudColorMap,
                             pointCloudColorMapBody)
                 && partialPointCloudColorMap.m_classVersion == 7
                 && partialPointCloudColorMap.m_defaultIntensityColorScheme
                        == "intensity"
                 && partialPointCloudColorMap.m_colorRamps.size() == 1u
                 && partialPointCloudColorMap.m_colorRamps.front()
                            .m_classVersion
                        == 3
                 && partialPointCloudColorMap.m_colorRamps.front()
                            .m_colorSchemes.front()
                        == "ramp-one",
             "POINTCLOUDCOLORMAP parser state source setup");
    ExposedPointCloudColorMap copiedPointCloudColorMap(
        partialPointCloudColorMap);
    const bool copiedPointCloudColorMapStale = parseDxfRecords(
        copiedPointCloudColorMap, "1\nstale-default\n");
    const bool copiedPointCloudColorMapFresh = parseDxfRecords(
        copiedPointCloudColorMap,
        "100\nAcDbPointCloudColorMap\n70\n1\n91\n1\n90\n4\n"
        "1\nclass-ramp\n");
    t.expect(copiedPointCloudColorMapStale
                 && copiedPointCloudColorMap.m_defaultIntensityColorScheme
                        == "stale-default"
                 && copiedPointCloudColorMap.m_colorRamps.size() == 1u
                 && copiedPointCloudColorMap.m_colorRamps.front()
                            .m_colorSchemes.front()
                        == "ramp-one"
                 && copiedPointCloudColorMapFresh
                 && copiedPointCloudColorMap.m_classificationColorRamps.size()
                        == 1u
                 && copiedPointCloudColorMap.m_classificationColorRamps.front()
                            .m_colorSchemes.front()
                        == "class-ramp",
             "POINTCLOUDCOLORMAP copy clears ramp/default parser state");
    ExposedPointCloudColorMap assignedPointCloudColorMap;
    assignedPointCloudColorMap = partialPointCloudColorMap;
    const bool assignedPointCloudColorMapStale = parseDxfRecords(
        assignedPointCloudColorMap, "1\nassigned-default\n");
    const bool assignedPointCloudColorMapFresh = parseDxfRecords(
        assignedPointCloudColorMap,
        "100\nAcDbPointCloudColorMap\n70\n1\n91\n1\n90\n5\n"
        "1\nassigned-ramp\n");
    t.expect(assignedPointCloudColorMapStale
                 && assignedPointCloudColorMap.m_defaultIntensityColorScheme
                        == "assigned-default"
                 && assignedPointCloudColorMap.m_colorRamps.size() == 1u
                 && assignedPointCloudColorMap.m_colorRamps.front()
                            .m_colorSchemes.front()
                        == "ramp-one"
                 && assignedPointCloudColorMapFresh
                 && assignedPointCloudColorMap.m_classificationColorRamps.size()
                        == 1u
                 && assignedPointCloudColorMap.m_classificationColorRamps.front()
                            .m_colorSchemes.front()
                        == "assigned-ramp",
             "POINTCLOUDCOLORMAP assignment clears ramp/default parser state");

    const std::string sunStudyBody =
        "90\n7\n1\nsetup\n2\ndescription\n3\nsheet\n4\nsubset\n"
        "70\n0\n290\n1\n91\n1\n90\n10\n90\n20\n91\n2\n"
        "290\n1\n290\n0\n40\n2.5\n340\nA1\n341\nA2\n"
        "342\nA3\n343\nA4\n";
    ExposedSunStudy partialSunStudy;
    t.expect(parseDxfRecords(partialSunStudy, sunStudyBody)
                 && partialSunStudy.m_classVersion == 7
                 && partialSunStudy.m_dates.size() == 1u
                 && partialSunStudy.m_dates.front().m_julianDay == 10
                 && partialSunStudy.m_dates.front().m_milliseconds == 20
                 && partialSunStudy.m_hours.size() == 2u
                 && partialSunStudy.m_hours.front()
                 && !partialSunStudy.m_hours.back()
                 && partialSunStudy.m_pageSetupWizardHandle == 0xA1u,
             "SUNSTUDY parser state source setup");
    ExposedSunStudy copiedSunStudy(partialSunStudy);
    const bool copiedSunStudyFresh = parseDxfRecords(
        copiedSunStudy, "90\n8\n91\n1\n90\n30\n90\n40\n");
    t.expect(copiedSunStudyFresh && copiedSunStudy.m_dates.size() == 2u
                 && copiedSunStudy.m_dates.back().m_julianDay == 30
                 && copiedSunStudy.m_dates.back().m_milliseconds == 40
                 && copiedSunStudy.m_hours.size() == 2u,
             "SUNSTUDY copy clears date/hour parser state");
    ExposedSunStudy assignedSunStudy;
    assignedSunStudy = partialSunStudy;
    const bool assignedSunStudyFresh = parseDxfRecords(
        assignedSunStudy, "90\n9\n91\n1\n90\n50\n90\n60\n");
    t.expect(assignedSunStudyFresh && assignedSunStudy.m_dates.size() == 2u
                 && assignedSunStudy.m_dates.back().m_julianDay == 50
                 && assignedSunStudy.m_dates.back().m_milliseconds == 60
                 && assignedSunStudy.m_hours.size() == 2u,
             "SUNSTUDY assignment clears date/hour parser state");

    const std::string motionPathBody =
        "90\n1\n90\n2\n90\n3\n340\nA1\n340\nA2\n340\nA3\n";
    ExposedMotionPath partialMotionPath;
    t.expect(parseDxfRecords(partialMotionPath, motionPathBody)
                 && partialMotionPath.m_classVersion == 1
                 && partialMotionPath.m_frames == 2
                 && partialMotionPath.m_frameRate == 3
                 && partialMotionPath.m_viewTableHandle == 0xA3u,
             "MOTIONPATH parser state source setup");
    ExposedMotionPath copiedMotionPath(partialMotionPath);
    t.expect(parseDxfRecords(copiedMotionPath, "90\n9\n")
                 && copiedMotionPath.m_classVersion == 9,
             "MOTIONPATH copy clears positional parser state");
    ExposedMotionPath assignedMotionPath;
    assignedMotionPath = partialMotionPath;
    t.expect(parseDxfRecords(assignedMotionPath, "90\n10\n")
                 && assignedMotionPath.m_classVersion == 10,
             "MOTIONPATH assignment clears positional parser state");

    ExposedCurvePath partialCurvePath;
    t.expect(parseDxfRecords(partialCurvePath, "90\n1\n340\nA1\n")
                 && partialCurvePath.m_classVersion == 1
                 && partialCurvePath.m_entityHandle == 0xA1u,
             "CURVEPATH parser state source setup");
    ExposedCurvePath copiedCurvePath(partialCurvePath);
    t.expect(parseDxfRecords(copiedCurvePath, "90\n2\n")
                 && copiedCurvePath.m_classVersion == 2,
             "CURVEPATH copy clears positional parser state");
    ExposedCurvePath assignedCurvePath;
    assignedCurvePath = partialCurvePath;
    t.expect(parseDxfRecords(assignedCurvePath, "340\nA2\n")
                 && assignedCurvePath.m_entityHandle == 0xA2u,
             "CURVEPATH assignment clears positional parser state");

    ExposedPointPath partialPointPath;
    t.expect(parseDxfRecords(partialPointPath,
                             "90\n1\n10\n1\n20\n2\n30\n3\n")
                 && partialPointPath.m_classVersion == 1
                 && partialPointPath.m_point.x == 1.0
                 && partialPointPath.m_point.z == 3.0,
             "POINTPATH parser state source setup");
    ExposedPointPath copiedPointPath(partialPointPath);
    t.expect(parseDxfRecords(copiedPointPath, "10\n9\n")
                 && copiedPointPath.m_point.x == 9.0,
             "POINTPATH copy clears coordinate parser state");
    ExposedPointPath assignedPointPath;
    assignedPointPath = partialPointPath;
    t.expect(parseDxfRecords(assignedPointPath, "10\n8\n")
                 && assignedPointPath.m_point.x == 8.0,
             "POINTPATH assignment clears coordinate parser state");

    const std::string partialViewingIndexBody =
        "10\n1\n20\n2\n30\n3\n11\n4\n21\n5\n31\n6\n340\nA1\n";
    ExposedPartialViewingIndex partialViewingIndex;
    t.expect(parseDxfRecords(partialViewingIndex, partialViewingIndexBody)
                 && partialViewingIndex.finalizeDxf()
                 && partialViewingIndex.m_entries.size() == 1u
                 && partialViewingIndex.m_entries.front().objectHandle == 0xA1u,
             "PARTIAL_VIEWING_INDEX parser state source setup");
    ExposedPartialViewingIndex copiedPartialViewingIndex(partialViewingIndex);
    const bool copiedPartialViewingIndexFresh = parseDxfRecords(
        copiedPartialViewingIndex,
        "10\n7\n20\n8\n30\n9\n11\n10\n21\n11\n31\n12\n340\nA2\n");
    t.expect(copiedPartialViewingIndexFresh
                 && copiedPartialViewingIndex.finalizeDxf()
                 && copiedPartialViewingIndex.m_entries.size() == 2u
                 && copiedPartialViewingIndex.m_entries.back().objectHandle
                        == 0xA2u,
             "PARTIAL_VIEWING_INDEX copy clears pending-entry parser state");
    ExposedPartialViewingIndex assignedPartialViewingIndex;
    assignedPartialViewingIndex = partialViewingIndex;
    const bool assignedPartialViewingIndexFresh = parseDxfRecords(
        assignedPartialViewingIndex,
        "10\n13\n20\n14\n30\n15\n11\n16\n21\n17\n31\n18\n340\nA3\n");
    t.expect(assignedPartialViewingIndexFresh
                 && assignedPartialViewingIndex.finalizeDxf()
                 && assignedPartialViewingIndex.m_entries.size() == 2u
                 && assignedPartialViewingIndex.m_entries.back().objectHandle
                        == 0xA3u,
             "PARTIAL_VIEWING_INDEX assignment clears pending-entry parser state");

    const std::string sectionSettingsBody =
        "100\nAcDbSectionSettings\n90\n1\n91\n2\n92\n3\n"
        "1\nSectionTypeSettings\n90\n10\n91\n11\n92\n1\n93\n0\n"
        "2\nSectionGeometrySettings\n90\n4\n91\n5\n92\n6\n8\nLayer\n"
        "6\nContinuous\n";
    ExposedSection partialSectionSettings;
    partialSectionSettings.m_kind = DRW_Section::Settings;
    t.expect(parseDxfRecords(partialSectionSettings, sectionSettingsBody)
                 && partialSectionSettings.m_classVersion == 1
                 && partialSectionSettings.m_typeCount == 2
                 && partialSectionSettings.m_types.size() == 1u
                 && partialSectionSettings.m_types.front().m_type == 10
                 && partialSectionSettings.m_types.front().m_geometry.size() == 1u
                 && partialSectionSettings.m_types.front().m_geometry.front().m_layer
                        == "Layer",
             "SECTION settings parser state source setup");
    ExposedSection copiedSectionSettings(partialSectionSettings);
    const bool copiedSectionFresh = parseDxfRecords(
        copiedSectionSettings,
        "100\nAcDbSectionSettings\n90\n99\n1\nSectionTypeSettings\n"
        "90\n20\n91\n21\n92\n2\n93\n0\n");
    t.expect(copiedSectionFresh && copiedSectionSettings.m_classVersion == 99
                 && copiedSectionSettings.m_types.size() == 2u
                 && copiedSectionSettings.m_types.back().m_type == 20,
             "SECTION copy clears body/type/geometry parser state");
    ExposedSection assignedSectionSettings;
    assignedSectionSettings = partialSectionSettings;
    const bool assignedSectionFresh = parseDxfRecords(
        assignedSectionSettings,
        "100\nAcDbSectionSettings\n90\n98\n1\nSectionTypeSettings\n"
        "90\n30\n91\n31\n92\n3\n93\n0\n");
    t.expect(assignedSectionFresh && assignedSectionSettings.m_classVersion == 98
                 && assignedSectionSettings.m_types.size() == 2u
                 && assignedSectionSettings.m_types.back().m_type == 30,
             "SECTION assignment clears body/type/geometry parser state");

    ExposedBackground partialGradient;
    partialGradient.m_kind = DRW_Background::Gradient;
    t.expect(parseDxfRecords(partialGradient, "90\n1\n90\n2\n")
                 && partialGradient.m_classVersion == 1
                 && partialGradient.m_colorTop == 2,
             "BACKGROUND gradient parser state source setup");
    ExposedBackground copiedGradient(partialGradient);
    t.expect(parseDxfRecords(copiedGradient, "90\n9\n")
                 && copiedGradient.m_classVersion == 9
                 && copiedGradient.m_colorTop == 2,
             "BACKGROUND copy clears overloaded 90 parser state");
    ExposedBackground assignedGradient;
    assignedGradient = partialGradient;
    t.expect(parseDxfRecords(assignedGradient, "90\n8\n")
                 && assignedGradient.m_classVersion == 8
                 && assignedGradient.m_colorTop == 2,
             "BACKGROUND assignment clears overloaded 90 parser state");

    ExposedBackground partialIbl;
    partialIbl.m_kind = DRW_Background::Ibl;
    t.expect(parseDxfRecords(partialIbl, "290\n0\n")
                 && !partialIbl.m_enabled && !partialIbl.m_displayImage,
             "BACKGROUND IBL parser state source setup");
    ExposedBackground copiedIbl(partialIbl);
    t.expect(parseDxfRecords(copiedIbl, "290\n1\n")
                 && copiedIbl.m_enabled && !copiedIbl.m_displayImage,
             "BACKGROUND copy clears repeated 290 parser state");
    ExposedBackground assignedIbl;
    assignedIbl = partialIbl;
    t.expect(parseDxfRecords(assignedIbl, "290\n1\n")
                 && assignedIbl.m_enabled && !assignedIbl.m_displayImage,
             "BACKGROUND assignment clears repeated 290 parser state");

    ExposedNavisworksModelDef partialNavisworksModelDef;
    t.expect(parseDxfRecords(partialNavisworksModelDef,
                             "290\n1\n290\n0\n")
                 && partialNavisworksModelDef.m_status
                 && !partialNavisworksModelDef.m_hostDrawingVisibility,
             "NAVISWORKSMODELDEF parser state source setup");
    ExposedNavisworksModelDef copiedNavisworksModelDef(
        partialNavisworksModelDef);
    t.expect(parseDxfRecords(copiedNavisworksModelDef, "290\n1\n")
                 && copiedNavisworksModelDef.m_status
                 && !copiedNavisworksModelDef.m_hostDrawingVisibility,
             "NAVISWORKSMODELDEF copy clears repeated 290 parser state");
    ExposedNavisworksModelDef assignedNavisworksModelDef;
    assignedNavisworksModelDef = partialNavisworksModelDef;
    t.expect(parseDxfRecords(assignedNavisworksModelDef, "290\n1\n")
                 && assignedNavisworksModelDef.m_status
                 && !assignedNavisworksModelDef.m_hostDrawingVisibility,
             "NAVISWORKSMODELDEF assignment clears repeated 290 parser state");

    const std::string layoutBody =
        "100\nAcDbPlotSettings\n1\nplot-page\n2\nprinter\n70\n4\n"
        "100\nAcDbLayout\n1\nlayout-name\n70\n2\n71\n3\n330\nA1\n";
    ExposedLayout partialLayout;
    t.expect(parseDxfRecords(partialLayout, layoutBody)
                 && partialLayout.pageSetupName == "plot-page"
                 && partialLayout.printerConfig == "printer"
                 && partialLayout.name == "layout-name"
                 && partialLayout.layoutFlags == 2
                 && partialLayout.tabOrder == 3
                 && partialLayout.paperSpaceBlockRecordHandle.ref == 0xA1u,
             "LAYOUT parser state source setup");
    ExposedLayout copiedLayout(partialLayout);
    t.expect(parseDxfRecords(copiedLayout, "1\ncopied-plot-page\n")
                 && copiedLayout.pageSetupName == "copied-plot-page"
                 && copiedLayout.name == "layout-name"
                 && copiedLayout.layoutFlags == 2,
             "LAYOUT copy clears subclass parser state");
    ExposedLayout assignedLayout;
    assignedLayout = partialLayout;
    t.expect(parseDxfRecords(assignedLayout, "1\nassigned-plot-page\n")
                 && assignedLayout.pageSetupName == "assigned-plot-page"
                 && assignedLayout.name == "layout-name"
                 && assignedLayout.layoutFlags == 2,
             "LAYOUT assignment clears subclass parser state");

    ExposedDetailViewStyle partialDetail;
    t.expect(parseDxfRecords(
                 partialDetail,
                 "100\nAcDbDetailViewStyle\n71\n1\n340\nA1\n")
                 && partialDetail.m_identifierStyleHandle == 0xA1u,
             "DETAILVIEWSTYLE parser state source setup");
    ExposedDetailViewStyle copiedDetail(partialDetail);
    t.expect(parseDxfRecords(copiedDetail, "340\nA2\n")
                 && copiedDetail.m_identifierStyleHandle == 0xA1u
                 && copiedDetail.m_arrowSymbolHandle == 0u,
             "DETAILVIEWSTYLE copy clears group parser state");
    ExposedDetailViewStyle assignedDetail;
    assignedDetail = partialDetail;
    t.expect(parseDxfRecords(assignedDetail, "340\nA3\n")
                 && assignedDetail.m_identifierStyleHandle == 0xA1u
                 && assignedDetail.m_arrowSymbolHandle == 0u,
             "DETAILVIEWSTYLE assignment clears group parser state");

    ExposedSectionViewStyle partialSectionView;
    t.expect(parseDxfRecords(
                 partialSectionView,
                 "100\nAcDbSectionViewStyle\n71\n1\n340\nB1\n")
                 && partialSectionView.m_identifierStyleHandle == 0xB1u,
             "SECTIONVIEWSTYLE parser state source setup");
    ExposedSectionViewStyle copiedSectionView(partialSectionView);
    t.expect(parseDxfRecords(copiedSectionView, "340\nB2\n")
                 && copiedSectionView.m_identifierStyleHandle == 0xB1u
                 && copiedSectionView.m_arrowStartSymbolHandle == 0u,
             "SECTIONVIEWSTYLE copy clears group parser state");
    ExposedSectionViewStyle assignedSectionView;
    assignedSectionView = partialSectionView;
    t.expect(parseDxfRecords(assignedSectionView, "340\nB3\n")
                 && assignedSectionView.m_identifierStyleHandle == 0xB1u
                 && assignedSectionView.m_arrowStartSymbolHandle == 0u,
             "SECTIONVIEWSTYLE assignment clears group parser state");

    ExposedAssociativeObject partialAssociative("ACDBASSOCNETWORK");
    t.expect(parseDxfRecords(partialAssociative,
                             "100\nAcDbPersSubentManager\n90\n7\n330\nA1\n")
                 && partialAssociative.m_classVersion == 7
                 && partialAssociative.m_persistentSubentityHandles.size() == 1u
                 && partialAssociative.m_persistentSubentityHandles.front()
                        == 0xA1u,
             "ASSOCIATIVEOBJECT parser state source setup");
    ExposedAssociativeObject copiedAssociative(partialAssociative);
    t.expect(parseDxfRecords(copiedAssociative, "330\nA2\n")
                 && copiedAssociative.m_persistentSubentityHandles.size() == 1u
                 && copiedAssociative.m_persistentSubentityHandles.front()
                        == 0xA1u,
             "ASSOCIATIVEOBJECT copy clears persistent-body parser state");
    ExposedAssociativeObject assignedAssociative;
    assignedAssociative = partialAssociative;
    t.expect(parseDxfRecords(assignedAssociative, "330\nA3\n")
                 && assignedAssociative.m_persistentSubentityHandles.size()
                        == 1u
                 && assignedAssociative.m_persistentSubentityHandles.front()
                        == 0xA1u,
             "ASSOCIATIVEOBJECT assignment clears persistent-body parser state");

    DRW_Dimension sourceDimension;
    sourceDimension.extData.push_back(
        std::make_shared<DRW_Variant>(1000, "dimension-xdata"));
    DRW_DimAligned copiedDimension(sourceDimension);
    t.expect(copiedDimension.extData.size() == 1u
                 && copiedDimension.extData.front()
                        != sourceDimension.extData.front(),
             "DRW_Dimension copy isolates extended data");
}

// A callback sink for parser-fuzz inputs.  Keeping the sink dependency-free
// makes this lane exercise the public dxfRW::readAscii path without coupling
// hardening coverage to the dwg2dxf adapter's storage policy.
class FuzzInterface final : public DRW_Interface {
public:
    void addHeader(const DRW_Header* data) override {
        ++headerCount;
        headerComments = data != nullptr ? data->getComments() : std::string{};
    }
    void addLType(const DRW_LType&) override {}
    void addLayer(const DRW_Layer&) override {}
    void addDimStyle(const DRW_Dimstyle&) override {}
    void addVport(const DRW_Vport&) override {}
    void addTextStyle(const DRW_Textstyle&) override {}
    void addAppId(const DRW_AppId&) override {}
    void addBlock(const DRW_Block&) override {}
    void setBlock(int) override {}
    void endBlock() override {}
    void addPoint(const DRW_Point&) override {}
    void addLine(const DRW_Line&) override {}
    void addRay(const DRW_Ray&) override {}
    void addXline(const DRW_Xline&) override {}
    void addArc(const DRW_Arc&) override {}
    void addCircle(const DRW_Circle&) override {}
    void addEllipse(const DRW_Ellipse&) override {}
    void addLWPolyline(const DRW_LWPolyline&) override {}
    void addPolyline(const DRW_Polyline&) override {}
    void addSpline(const DRW_Spline*) override {}
    void addKnot(const DRW_Entity&) override {}
    void addInsert(const DRW_Insert&) override {}
    void addTrace(const DRW_Trace&) override {}
    void add3dFace(const DRW_3Dface&) override {}
    void addSolid(const DRW_Solid&) override {}
    void addMText(const DRW_MText&) override {}
    void addText(const DRW_Text&) override {}
    void addDimAlign(const DRW_DimAligned*) override {}
    void addDimLinear(const DRW_DimLinear*) override {}
    void addDimRadial(const DRW_DimRadial*) override {}
    void addDimDiametric(const DRW_DimDiametric*) override {}
    void addDimAngular(const DRW_DimAngular*) override {}
    void addDimAngular3P(const DRW_DimAngular3p*) override {}
    void addDimOrdinate(const DRW_DimOrdinate*) override {}
    void addLeader(const DRW_Leader*) override {}
    void addHatch(const DRW_Hatch*) override {}
    void addViewport(const DRW_Viewport&) override {}
    void addImage(const DRW_Image*) override {}
    void linkImage(const DRW_ImageDef*) override {}
    void addRawDxfSection(const DRW_RawDxfSection& data) override {
        ++rawSectionCount;
        rawSectionHasValues = rawSectionHasValues || data.m_hasRawValues;
    }
    void addComment(const char*) override {}
    void addPlotSettings(const DRW_PlotSettings*) override {}
    void writeHeader(DRW_Header&) override {}
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {}
    void writeAppId() override {}

    std::size_t rawSectionCount {0};
    bool rawSectionHasValues {false};
    std::size_t headerCount {0};
    std::string headerComments;
};

void testCheckedArithmetic(TestContext& t) {
    std::uint64_t result = 0;
    t.expect(dwgSafety::add(1, 2, result) && result == 3,
             "checked add accepts ordinary values");
    t.expect(!dwgSafety::add((std::numeric_limits<std::uint64_t>::max)(), 1,
                             result),
             "checked add rejects overflow");
    t.expect(dwgSafety::multiply(3, 7, result) && result == 21,
             "checked multiply accepts ordinary values");
    t.expect(!dwgSafety::multiply((std::numeric_limits<std::uint64_t>::max)(),
                                  2, result),
             "checked multiply rejects overflow");
    t.expect(dwgSafety::range(4, 6, 10) && !dwgSafety::range(5, 6, 10),
             "checked range enforces end bound");
    t.expect(dwgSafety::alignUp8(9, result) && result == 16,
             "checked alignment rounds up");
    t.expect(!dwgSafety::alignUp8((std::numeric_limits<std::uint64_t>::max)(),
                                  result),
             "checked alignment rejects overflow");

    t.expect(dwgSafety::validReactorCount(0)
                 && dwgSafety::validReactorCount(
                        static_cast<std::int32_t>(dwgSafety::MaxReactorCount))
                 && !dwgSafety::validReactorCount(-1)
                 && !dwgSafety::validReactorCount(
                        static_cast<std::int32_t>(dwgSafety::MaxReactorCount + 1u)),
             "reactor budget accepts only bounded nonnegative counts");
    t.expect(dwgSafety::validOwnedObjectCount(1, 2)
                 && !dwgSafety::validOwnedObjectCount(2, 2)
                 && !dwgSafety::validOwnedObjectCount(1, 0),
             "owned-object budget accounts for remaining bytes");
    t.expect(dwgSafety::sectionBufferCapacity(65, 2, 64, result)
                 && result == 128,
             "section capacity combines logical and written pages");
    t.expect(!dwgSafety::sectionBufferCapacity(
                  (std::numeric_limits<std::uint64_t>::max)(), 1, 64, result),
             "section capacity rejects size overflow");
}

class CountingPrinter final : public DRW::DebugPrinter {
public:
    explicit CountingPrinter(int& destructions) : m_destructions(destructions) {}
    ~CountingPrinter() override { ++m_destructions; }

private:
    int& m_destructions;
};

void testNullAndOwnershipContracts(TestContext& t) {
    bool constructed = true;
    try {
        dxfRW dxf(nullptr);
        dwgRW dwg(nullptr);
        t.expect(!dxf.write(nullptr, DRW::AC1015, false)
                     && dxf.getError() == DRW::BAD_UNKNOWN,
                 "DXF null interface is invalid argument");
        t.expect(!dwg.write(nullptr, DRW::AC1015, false)
                     && dwg.getError() == DRW::BAD_UNKNOWN,
                 "DWG null interface is invalid argument");
        t.expect(!dwg.read(nullptr, false) && dwg.getError() == DRW::BAD_UNKNOWN,
                 "DWG null read interface is invalid argument");
        t.expect(!dwg.readBuffer(nullptr, 0, nullptr, false)
                     && dwg.getError() == DRW::BAD_UNKNOWN,
                 "DWG null buffer/interface is invalid argument");
        t.expect(!dxf.write(nullptr, DRW::UNKNOWNV, false)
                     && dxf.getError() == DRW::BAD_VERSION,
                 "DXF unsupported version precedes null interface");
        t.expect(!dwg.write(nullptr, DRW::UNKNOWNV, false)
                     && dwg.getError() == DRW::BAD_VERSION,
                 "DWG unsupported version precedes null interface");
    } catch (...) {
        constructed = false;
    }
    t.expect(constructed, "null filename constructors do not throw");

    int destructions = 0;
    DRW::setCustomDebugPrinter(new CountingPrinter(destructions));
    DRW::setCustomDebugPrinter(nullptr);
    t.expect(destructions == 1,
             "replacing a custom debug printer destroys the owned instance");
    DRW::setCustomDebugPrinter(new CountingPrinter(destructions));
    DRW::setCustomDebugPrinter(nullptr);
    t.expect(destructions == 2,
             "null debug printer restores a usable default");
}

void testMalformedInMemoryInputs(TestContext& t) {
    std::uint32_t state = 0x13579BDFu;
#ifdef LIBDXFRW_LONG_FUZZ
    constexpr std::size_t iterations = 8192;
#else
    constexpr std::size_t iterations = 256;
#endif
    for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
        const std::size_t length = iteration % 97;
        std::vector<std::uint8_t> bytes(length);
        for (std::uint8_t& value : bytes) {
            state = state * 1664525u + 1013904223u;
            value = static_cast<std::uint8_t>(state >> 24);
        }

        try {
            const std::string proxy(reinterpret_cast<const char*>(bytes.data()),
                                    bytes.size());
            const DRW_ProxyGraphicDecodeResult inspected =
                DRW_ProxyGraphicDecoder::inspect(proxy);
            t.expect(inspected.consumedByteCount <= bytes.size(),
                     "proxy inspection stays within input");

            DRW_SabData sab;
            t.expect(!drw_parseSab(bytes.data(), bytes.size(), sab),
                     "random SAB vector fails closed");

            const DRW_DataStorageSection storage =
                DRW_parseDataStorage(bytes.data(), bytes.size(), DRW::AC1027);
            t.expect(storage.sectionByteLength == bytes.size(),
                     "DataStorage reports bounded input length");
        } catch (...) {
            t.expect(false, "malformed in-memory vector does not throw");
        }
    }

    DRW_SabData nullSab;
    t.expect(!drw_parseSab(nullptr, 0, nullSab),
             "null SAB input fails closed");
    const DRW_DataStorageSection nullStorage =
        DRW_parseDataStorage(nullptr, 0, DRW::UNKNOWNV);
    t.expect(nullStorage.parseFailed && !nullStorage.diagnostics.empty(),
             "null DataStorage input yields a bounded diagnostic");
}

void testDxfReadFuzzSmoke(TestContext& t) {
#ifdef LIBDXFRW_LONG_FUZZ
    // The opt-in target expands the same deterministic generators for a
    // longer local campaign. It remains bounded and payload-free; the default
    // hardening target below stays small enough for the fast inner loop.
    constexpr std::size_t iterations = 65536;
    constexpr std::size_t maxLength = 1024;
#else
    // Keep this deterministic and bounded: it is an inner-loop safety lane,
    // not a substitute for the scheduled long external fuzz campaign.
    constexpr std::size_t iterations = 2048;
    constexpr std::size_t maxLength = 384;
#endif
    const std::string skeleton =
        "0\nSECTION\n2\nHEADER\n0\nENDSEC\n0\nSECTION\n"
        "2\nENTITIES\n0\nENDSEC\n0\nEOF\n";
    std::uint32_t state = 0xC001D00Du;
    for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
        std::string content;
        if (iteration % 4 == 0) {
            content = skeleton;
            const std::size_t extra = iteration % 97;
            content.reserve(content.size() + extra);
            for (std::size_t index = 0; index < extra; ++index) {
                state = state * 1664525u + 1013904223u;
                content.push_back(static_cast<char>(' ' + (state % 95u)));
                if ((index % 7u) == 6u)
                    content.push_back('\n');
            }
        } else {
            const std::size_t length = (iteration * 37u) % maxLength;
            content.reserve(length);
            for (std::size_t index = 0; index < length; ++index) {
                state = state * 1664525u + 1013904223u;
                const std::uint8_t byte = static_cast<std::uint8_t>(state >> 24);
                // Mix arbitrary bytes with line-oriented delimiters so both
                // lexical and group-code recovery paths are exercised.
                content.push_back((index % 11u == 0u)
                                      ? '\n'
                                      : static_cast<char>(byte));
            }
        }

        FuzzInterface interface_;
        dxfRW reader(nullptr);
        try {
            (void)reader.readAscii(&interface_, false, content);
        } catch (...) {
            t.expect(false, "DXF parser fuzz input does not throw");
        }
    }
}

void testDwgReadFuzzSmoke(TestContext& t) {
    // Exercise the public in-memory DWG entry point as well.  Each vector is
    // discarded immediately; the lane is deliberately bounded so it remains
    // suitable for the fast inner loop and sanitizer jobs.
#ifdef LIBDXFRW_LONG_FUZZ
    constexpr std::size_t iterations = 16384;
    constexpr std::size_t maxLength = 512;
#else
    constexpr std::size_t iterations = 512;
    constexpr std::size_t maxLength = 256;
#endif
    constexpr std::array<std::array<std::uint8_t, 6>, 6> magics {{
        {{'A', 'C', '1', '0', '1', '5'}},
        {{'A', 'C', '1', '0', '1', '8'}},
        {{'A', 'C', '1', '0', '2', '1'}},
        {{'A', 'C', '1', '0', '2', '4'}},
        {{'A', 'C', '1', '0', '2', '7'}},
        {{'A', 'C', '1', '0', '3', '2'}},
    }};
    std::uint32_t state = 0xD06F00D5u;
    for (std::size_t iteration = 0; iteration < iterations; ++iteration) {
        const std::size_t length = 6u + ((iteration * 29u) % (maxLength - 5u));
        std::vector<std::uint8_t> bytes(length);
        const auto& magic = magics[iteration % magics.size()];
        std::copy(magic.begin(), magic.end(), bytes.begin());
        for (std::size_t index = 6; index < bytes.size(); ++index) {
            state = state * 1664525u + 1013904223u;
            bytes[index] = static_cast<std::uint8_t>(state >> 24);
        }
        // A few vectors retain a valid header but contain an all-zero tail;
        // this reaches different short-page and offset checks than arbitrary
        // bytes while remaining independent of any real drawing.
        if ((iteration % 8u) == 0u)
            std::fill(bytes.begin() + 6, bytes.end(), 0u);

        FuzzInterface interface_;
        dwgRW reader(nullptr);
        try {
            (void)reader.readBuffer(bytes.data(), bytes.size(), &interface_,
                                    false);
        } catch (...) {
            t.expect(false, "DWG parser fuzz input does not throw");
        }
    }
}

void testDxfReadAsciiResetsFormatState(TestContext& t) {
    std::string content =
        "0\nSECTION\n2\nLOCAL_REUSE\n260\n2147483647\n"
        "0\nENDSEC\n0\nEOF\n";
    FuzzInterface interface_;
    dxfRW reader(nullptr);
    reader.setBinary(true);
    t.expect(reader.readAscii(&interface_, false, content),
             "ASCII read remains usable after binary mode state");
    t.expect(interface_.rawSectionCount == 1u && interface_.rawSectionHasValues,
             "ASCII raw capture retains source values after binary reuse");
}

void testDxfReadResetsHeaderState(TestContext& t) {
    std::string first =
        "999\nfirst-read-comment\n0\nSECTION\n2\nHEADER\n"
        "9\n$HANDSEED\n1\nAB\n0\nENDSEC\n0\nEOF\n";
    std::string second =
        "0\nSECTION\n2\nHEADER\n0\nENDSEC\n0\nEOF\n";
    dxfRW reader(nullptr);
    FuzzInterface firstInterface;
    FuzzInterface secondInterface;
    t.expect(reader.readAscii(&firstInterface, false, first),
             "first ASCII read establishes header state");
    t.expect(reader.readAscii(&secondInterface, false, second),
             "second ASCII read remains usable after header state");
    t.expect(firstInterface.headerCount == 1u
                 && firstInterface.headerComments == "first-read-comment",
             "first read publishes its own header comment");
    t.expect(secondInterface.headerCount == 1u
                 && secondInterface.headerComments.empty(),
             "second read does not inherit prior header comments");
}

void testDwgReadResetsVersionState(TestContext& t) {
    const std::array<std::uint8_t, 6> recognizedMagic {{
        'A', 'C', '1', '0', '2', '7'}};
    const std::array<std::uint8_t, 5> tooShort {{
        'A', 'C', '1', '0', '2'}};
    FuzzInterface interface_;
    dwgRW reader(nullptr);
    t.expect(!reader.readBuffer(recognizedMagic.data(), recognizedMagic.size(),
                                &interface_, false)
                 && reader.getVersion() == DRW::AC1027,
             "recognized DWG magic is retained for the failed parse");
    t.expect(!reader.readBuffer(tooShort.data(), tooShort.size(), &interface_,
                                false)
                 && reader.getVersion() == DRW::UNKNOWNV
                 && reader.getCodePage().empty(),
             "invalid DWG read does not expose stale version or code page");
}

void testDxfAggregateRecordBudget(TestContext& t) {
    std::string content =
        "0\nSECTION\n2\nLOCAL_BUDGET\n0\nENDSEC\n0\nEOF\n";
    FuzzInterface interface_;
    dxfRW reader(nullptr);
    reader.setDxfReadRecordBudget(3u);
    t.expect(!reader.readAscii(&interface_, false, content)
                 && reader.getError() == DRW::BAD_CODE_PARSED,
             "DXF aggregate record budget rejects an exhausted stream");
    const DRW_OperationDiagnostic diagnostic = reader.getLastDiagnostic();
    t.expect(diagnostic.cause == DRW::OperationCause::ResourceLimit
                 && diagnostic.code == "dxf-record-budget",
             "DXF budget exhaustion exposes a resource diagnostic");

    std::string commentContent =
        "0\nSECTION\n2\nLOCAL_BUDGET\n0\nENDSEC\n"
        "999\nignored-after-section\n0\nEOF\n";
    dxfRW commentReader(nullptr);
    commentReader.setDxfReadRecordBudget(4u);
    FuzzInterface commentInterface;
    t.expect(!commentReader.readAscii(&commentInterface, false, commentContent)
                 && commentReader.getLastDiagnostic().cause
                        == DRW::OperationCause::ResourceLimit,
             "ignored DXF comments consume the aggregate record budget");

    std::string retry = content;
    reader.setDxfReadRecordBudget(16u);
    FuzzInterface retryInterface;
    t.expect(reader.readAscii(&retryInterface, false, retry)
                 && retryInterface.rawSectionCount == 1u,
             "raising the DXF budget permits a fresh read session");
}

class ExposedDwgReader15 final : public dwgReader15 {
public:
    ExposedDwgReader15(std::unique_ptr<dwgBuffer> buffer, dwgRW* parent)
        : dwgReader15(std::move(buffer), parent) {}

    using dwgReader::readDwgHandles;

    void setObjectBudget(std::size_t budget) noexcept {
        m_readObjectBudget = budget;
    }

    bool readHandleMap(std::uint64_t size) {
        return readDwgHandles(
            fileBuf.get(), 0, size,
            (std::numeric_limits<std::uint64_t>::max)(),
            DwgIntegrityAddressSpace::None, -1);
    }
};

std::vector<std::uint8_t> makeDwgHandleMapVector() {
    // One data group (size=4, handle/location deltas 1/1) followed by the
    // empty terminator group. CRCs cover each size/data span and are computed
    // from the same in-memory bytes the reader receives.
    std::vector<std::uint8_t> bytes {
        0, 4, 1, 1, 0, 0,
        0, 2, 0, 0};
    dwgBuffer buffer(bytes.data(), bytes.size());
    const std::uint16_t dataCrc = buffer.crc8(0xc0c1, 0, 4);
    const std::uint16_t terminatorCrc = buffer.crc8(0xc0c1, 6, 8);
    bytes[4] = static_cast<std::uint8_t>(dataCrc >> 8);
    bytes[5] = static_cast<std::uint8_t>(dataCrc);
    bytes[8] = static_cast<std::uint8_t>(terminatorCrc >> 8);
    bytes[9] = static_cast<std::uint8_t>(terminatorCrc);
    return bytes;
}

void testDwgAggregateObjectBudget(TestContext& t) {
    std::vector<std::uint8_t> bytes = makeDwgHandleMapVector();
    dwgRW owner(nullptr);
    owner.setDwgReadObjectBudget(0u);
    ExposedDwgReader15 limited(
        std::make_unique<dwgBuffer>(bytes.data(), bytes.size()),
        &owner);
    t.expect(!limited.readHandleMap(bytes.size())
                 && limited.readObjectBudgetExceeded(),
             "DWG aggregate object budget rejects an exhausted handle map");

    dwgRW retryOwner(nullptr);
    retryOwner.setDwgReadObjectBudget(1u);
    ExposedDwgReader15 retry(
        std::make_unique<dwgBuffer>(bytes.data(), bytes.size()),
        &retryOwner);
    t.expect(retry.readHandleMap(bytes.size())
                 && retry.ObjectMap.size() == 1u
                 && !retry.readObjectBudgetExceeded(),
             "DWG object budget permits a bounded valid handle map");
}

class ExposedMLeader final : public DRW_MLeader {
public:
    using DRW_MLeader::parseCode;
};

void testMLeaderDxfContextRoundTrip(TestContext& t) {
    DRW_MLeader source;
    source.handle = 0xA100u;
    source.layer = "0";
    source.styleHandle.ref = 0xA101u;
    source.leaderLineTypeHandle.ref = 0xA102u;
    source.arrowHeadHandle.ref = 0xA103u;
    source.styleTextStyleHandle.ref = 0xA104u;
    source.styleBlockHandle.ref = 0xA105u;
    source.styleContentType = 1;
    source.styleBlockScale = DRW_Coord{2.0, 3.0, 4.0};
    source.classVersion = 7;
    source.context.hasTextContents = true;
    source.context.textLabel = "context text";
    source.context.textStyleHandle.ref = 0xA106u;
    source.context.hasContentsBlock = true;
    source.context.blockTableRecordHandle.ref = 0xA107u;
    for (std::size_t i = 0; i < source.context.blockTransform.size(); ++i)
        source.context.blockTransform[i] = static_cast<double>(i + 1u);

    DRW_MLeaderRoot root;
    root.connectionPoint = DRW_Coord{1.0, 2.0, 3.0};
    root.direction = DRW_Coord{0.0, 1.0, 0.0};
    root.breaks.emplace_back(DRW_Coord{4.0, 5.0, 6.0},
                             DRW_Coord{7.0, 8.0, 9.0});
    DRW_MLeaderLeaderLine line;
    line.points.emplace_back(DRW_Coord{10.0, 11.0, 12.0});
    line.breaks.emplace_back(DRW_Coord{13.0, 14.0, 15.0},
                             DRW_Coord{16.0, 17.0, 18.0});
    line.lineTypeHandle.ref = 0xA108u;
    line.arrowHandle.ref = 0xA109u;
    root.leaderLines.push_back(line);
    source.context.roots.push_back(root);

    std::ostringstream output;
    dxfRW writerOwner("");
    writerOwner.version = DRW::AC1027;
    writerOwner.binFile = false;
    writerOwner.writer = std::make_unique<dxfWriterAscii>(&output);
    t.expect(writerOwner.writeMultiLeader(&source),
             "MULTILEADER DXF writer accepts nested context payload");
    const std::string encoded = output.str();
    t.expect(encoded.find("302\nLEADER{\n") != std::string::npos
                 && encoded.find("304\nLEADER_LINE{\n") != std::string::npos
                 && encoded.find(" 12\n4\n") != std::string::npos
                 && encoded.find("340\nA106\n") != std::string::npos
                 && encoded.find(" 47\n1\n") != std::string::npos
                 && encoded.find(" 10\n2\n") != std::string::npos
                 && encoded.find("270\n") != std::string::npos,
             "MULTILEADER DXF writer emits nested breaks handles and transform");

    std::stringstream records(encoded);
    std::unique_ptr<dxfReader> reader =
        std::make_unique<dxfReaderAscii>(&records);
    ExposedMLeader parsed;
    int code = 0;
    bool parseOk = true;
    while (reader->readRec(&code)) {
        if (code == 0)
            continue;
        parseOk = parsed.parseCode(code, reader) && parseOk;
    }
    t.expect(parseOk && parsed.isDxfContextClosed()
                 && parsed.context.roots.size() == 1u
                 && parsed.context.roots.front().breaks.size() == 1u
                 && parsed.context.roots.front().leaderLines.size() == 1u,
             "MULTILEADER DXF parser closes and retains nested context");
    if (parseOk && parsed.context.roots.size() == 1u
        && parsed.context.roots.front().leaderLines.size() == 1u) {
        const DRW_MLeaderRoot& parsedRoot = parsed.context.roots.front();
        const DRW_MLeaderLeaderLine& parsedLine =
            parsedRoot.leaderLines.front();
        t.expect(parsedRoot.breaks.front().first.x == 4.0
                     && parsedRoot.breaks.front().second.z == 9.0
                     && parsedLine.breaks.front().first.y == 14.0
                     && parsedLine.breaks.front().second.z == 18.0
                     && parsedLine.lineTypeHandle.ref == 0xA108u
                     && parsedLine.arrowHandle.ref == 0xA109u
                     && parsed.context.textStyleHandle.ref == 0xA106u
                     && parsed.context.blockTableRecordHandle.ref == 0xA107u
                     && parsed.context.blockTransform[15] == 16.0
                     && parsed.styleHandle.ref == 0xA101u
                     && parsed.styleBlockScale.x == 2.0
                     && parsed.styleBlockScale.z == 4.0
                     && parsed.classVersion == 7,
                     "MULTILEADER DXF round-trip preserves geometry handles and matrix");
    }

    DRW_MLeader invalid = source;
    invalid.context.roots.resize(DRW_MLeader::kMaxRoots + 1u);
    std::ostringstream rejectedOutput;
    dxfRW rejectingOwner("");
    rejectingOwner.version = DRW::AC1027;
    rejectingOwner.binFile = false;
    rejectingOwner.writer = std::make_unique<dxfWriterAscii>(&rejectedOutput);
    t.expect(!rejectingOwner.writeMultiLeader(&invalid)
                 && rejectedOutput.str().empty(),
             "MULTILEADER DXF writer rejects oversized context transactionally");

    DRW_MLeader legacy = source;
    legacy.styleContentType = 2;
    DRW_MLeader::ArrowHeadEntry arrow;
    arrow.isDefault = false;
    arrow.handle.ref = 0xA10Au;
    legacy.arrowHeads.push_back(arrow);
    DRW_MLeader::BlockLabelEntry label;
    label.attDefHandle.ref = 0xA10Bu;
    label.labelText = "legacy label";
    label.uiIndex = 3;
    label.width = 12.5;
    legacy.blockLabels.push_back(label);
    std::ostringstream legacyOutput;
    dxfRW legacyOwner("");
    legacyOwner.version = DRW::AC1021;
    legacyOwner.binFile = false;
    legacyOwner.writer = std::make_unique<dxfWriterAscii>(&legacyOutput);
    t.expect(legacyOwner.writeMultiLeader(&legacy),
             "AC1021 MULTILEADER writer accepts legacy arrays");
    std::stringstream legacyRecords(legacyOutput.str());
    std::unique_ptr<dxfReader> legacyReader =
        std::make_unique<dxfReaderAscii>(&legacyRecords);
    ExposedMLeader parsedLegacy;
    parseOk = true;
    while (legacyReader->readRec(&code)) {
        if (code == 0)
            continue;
        parseOk = parsedLegacy.parseCode(code, legacyReader) && parseOk;
    }
    t.expect(parseOk && parsedLegacy.arrowHeads.size() == 1u
                 && !parsedLegacy.arrowHeads.front().isDefault
                 && parsedLegacy.arrowHeads.front().handle.ref == 0xA10Au
                 && parsedLegacy.blockLabels.size() == 1u
                 && parsedLegacy.blockLabels.front().attDefHandle.ref == 0xA10Bu
                 && parsedLegacy.blockLabels.front().labelText == "legacy label"
                 && parsedLegacy.blockLabels.front().uiIndex == 3
                 && parsedLegacy.blockLabels.front().width == 12.5,
             "AC1021 MULTILEADER round-trip preserves legacy arrays");

    DRW_MLeader oversizedLegacy = legacy;
    oversizedLegacy.arrowHeads.resize(DRW_MLeader::kMaxLeaderLines + 1u);
    std::ostringstream oversizedLegacyOutput;
    dxfRW oversizedLegacyOwner("");
    oversizedLegacyOwner.version = DRW::AC1021;
    oversizedLegacyOwner.binFile = false;
    oversizedLegacyOwner.writer =
        std::make_unique<dxfWriterAscii>(&oversizedLegacyOutput);
    t.expect(!oversizedLegacyOwner.writeMultiLeader(&oversizedLegacy)
                 && oversizedLegacyOutput.str().empty(),
             "AC1021 MULTILEADER writer rejects oversized legacy arrays");

    constexpr std::size_t maxMLeaderStringBytes = 16u * 1024u * 1024u;
    DRW_MLeader oversizedContextText = source;
    oversizedContextText.context.textLabel.assign(maxMLeaderStringBytes + 1u,
                                                   'x');
    std::ostringstream oversizedContextTextOutput;
    dxfRW oversizedContextTextOwner("");
    oversizedContextTextOwner.version = DRW::AC1027;
    oversizedContextTextOwner.binFile = false;
    oversizedContextTextOwner.writer =
        std::make_unique<dxfWriterAscii>(&oversizedContextTextOutput);
    t.expect(!oversizedContextTextOwner.writeMultiLeader(
                  &oversizedContextText)
                 && oversizedContextTextOutput.str().empty(),
             "MULTILEADER writer rejects oversized context text transactionally");

    DRW_MLeader oversizedBlockLabel = legacy;
    oversizedBlockLabel.blockLabels.front().labelText.assign(
        maxMLeaderStringBytes + 1u, 'y');
    std::ostringstream oversizedBlockLabelOutput;
    dxfRW oversizedBlockLabelOwner("");
    oversizedBlockLabelOwner.version = DRW::AC1021;
    oversizedBlockLabelOwner.binFile = false;
    oversizedBlockLabelOwner.writer =
        std::make_unique<dxfWriterAscii>(&oversizedBlockLabelOutput);
    t.expect(!oversizedBlockLabelOwner.writeMultiLeader(
                  &oversizedBlockLabel)
                 && oversizedBlockLabelOutput.str().empty(),
             "AC1021 MULTILEADER writer rejects oversized block-label text");
}

} // namespace

int main() {
    TestContext context;
    testPublicOwnershipContracts(context);
    testCheckedArithmetic(context);
    testNullAndOwnershipContracts(context);
    testMalformedInMemoryInputs(context);
    testDxfReadFuzzSmoke(context);
    testDwgReadFuzzSmoke(context);
    testDxfReadAsciiResetsFormatState(context);
    testDxfReadResetsHeaderState(context);
    testDwgReadResetsVersionState(context);
    testDxfAggregateRecordBudget(context);
    testDwgAggregateObjectBudget(context);
    testMLeaderDxfContextRoundTrip(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " hardening assertion(s) failed\n";
        return 1;
    }
    std::cout << "Hardening vectors: PASS\n";
    return 0;
}
