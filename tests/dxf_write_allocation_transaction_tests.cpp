// SPDX-License-Identifier: GPL-2.0-or-later
// Generated whole-operation DXF write allocation rollback/retry control.
#include "dx_iface.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <memory>
#include <new>
#include <string>

namespace {
constexpr std::uint32_t kNodHandle = 0xD910u;
constexpr std::uint32_t kGroupDictionaryHandle = 0xD911u;
constexpr std::uint32_t kGroupHandle = 0xD912u;
constexpr std::uint32_t kImageDefHandle = 0xD913u;
constexpr std::uint32_t kCustomDictionaryHandle = 0xD914u;
constexpr std::uint32_t kMainDataLinkHandle = 0xD915u;
constexpr std::uint32_t kTargetAlphaHandle = 0xD916u;
constexpr std::uint32_t kTargetBetaHandle = 0xD917u;
constexpr std::uint32_t kPolylineHandle = 0xD918u;
constexpr std::uint32_t kMeshHandle = 0xD919u;
constexpr std::uint32_t kSplineHandle = 0xD91Au;
constexpr std::uint32_t kHatchHandle = 0xD91Bu;
constexpr std::uint32_t kLeaderHandle = 0xD91Cu;
constexpr std::uint32_t kMLineHandle = 0xD91Du;
constexpr std::uint32_t kMLineStyleHandle = 0xD91Eu;
constexpr std::uint32_t kPolyline3Handle = 0xD91Fu;
constexpr std::uint32_t kInsertHandle = 0xD922u;
constexpr std::uint32_t kInsertBlockPointHandle = 0xD923u;
constexpr std::uint32_t kInsertAttribHandle = 0xD924u;
constexpr std::uint32_t kInsertBlockHandle = 0xD925u;
constexpr std::uint32_t kFaceHandle = 0xD926u;
constexpr std::uint32_t kArcHandle = 0xD927u;
constexpr std::uint32_t kEllipseHandle = 0xD928u;
constexpr std::uint32_t kPointHandle = 0xD929u;
constexpr std::uint32_t k3DLineHandle = 0xD92Au;
constexpr std::uint32_t kSolidHandle = 0xD92Bu;
constexpr std::uint32_t kTraceHandle = 0xD92Cu;
constexpr std::uint32_t kTextHandle = 0xD92Du;
constexpr std::uint32_t kMTextHandle = 0xD92Eu;
constexpr std::uint32_t kRayHandle = 0xD92Fu;
constexpr std::uint32_t kXlineHandle = 0xD930u;
constexpr std::uint32_t kAttdefHandle = 0xD931u;
constexpr std::uint32_t kHelixHandle = 0xD933u;
constexpr std::uint32_t kMaterialHandle = 0xD934u;
constexpr std::uint32_t kLoftedSurfaceHandle = 0xD935u;
constexpr std::uint32_t kSweptSurfaceHandle = 0xD936u;
constexpr std::uint32_t kPlaneSurfaceHandle = 0xD937u;
constexpr std::uint32_t kExtrudedSurfaceHandle = 0xD938u;
constexpr std::uint32_t kRevolvedSurfaceHandle = 0xD939u;
constexpr std::uint32_t kNurbsSurfaceHandle = 0xD93Au;
constexpr std::uint32_t kScaleHandle = 0xD93Bu;
constexpr std::uint32_t kBackgroundHandle = 0xD93Cu;
constexpr std::uint32_t kPlotSettingsHandle = 0xD93Du;
constexpr std::uint32_t kWipeoutVariablesHandle = 0xD93Eu;
constexpr std::uint32_t kSunHandle = 0xD93Fu;
constexpr std::uint32_t kOle2FrameHandle = 0xD940u;
constexpr std::uint32_t kOleFrameHandle = 0xD941u;
constexpr std::uint32_t kShapeHandle = 0xD942u;
constexpr std::uint32_t kRasterVariablesHandle = 0xD943u;
constexpr std::uint32_t kDictionaryVarHandle = 0xD944u;
constexpr std::uint32_t kDbColorHandle = 0xD945u;
constexpr std::uint32_t kUnderlayDefinitionHandle = 0xD946u;
constexpr std::uint32_t kUnderlayHandle = 0xD947u;
constexpr std::uint32_t kLineTypeHandle = 0xD948u;
constexpr std::uint32_t kTypedXRecordHandle = 0xD949u;
constexpr std::uint32_t kMultiLeaderHandle = 0xD94Au;
constexpr std::uint32_t kDynamicConstraintDimensionHandle = 0xD94Bu;
const std::string kGroupName = "WHOLE_OPERATION_ALLOCATION_GROUP";
const std::string kCustomDictionaryName = "WHOLE_OPERATION_CUSTOM";
const std::string kInsertBlockName = "WHOLE_OPERATION_INSERT_BLOCK";
const std::string kNestedXrecordName = "NESTED_XRECORD";
using DictionaryEdge = DRW_Dictionary::Entry::DxfHandleCode;

bool installGeneratedGroupGraph(dxfRW& writer, std::uint32_t lineHandle,
                                std::uint32_t circleHandle) {
    DRW_Group group;
    group.handle = kGroupHandle;
    group.name = kGroupName;
    group.m_entityHandles = {lineHandle, circleHandle};
    writer.setGroups({group});
    writer.setHandleRemap({{kNodHandle, 0xCu},
                           {kGroupDictionaryHandle, 0xDu}});

    auto nod = std::make_unique<DRW_Dictionary>();
    nod->handle = kNodHandle;
    nod->m_entries.push_back(
        {"ACAD_GROUP", kGroupDictionaryHandle, DictionaryEdge::SoftOwner});
    auto groupDictionary = std::make_unique<DRW_Dictionary>();
    groupDictionary->handle = kGroupDictionaryHandle;
    groupDictionary->parentHandle = kNodHandle;
    groupDictionary->m_entries.push_back(
        {kGroupName, kGroupHandle, DictionaryEdge::SoftOwner});
    if (!writer.setGeneratedDictionarySourceRecords(
            std::move(nod), std::move(groupDictionary)))
        return false;

    writer.setRootDictEntries(
        {{kCustomDictionaryName, "D914"},
         {"WHOLE_OPERATION_DICTIONARYVAR", "D944"},
         {"WHOLE_OPERATION_DBCOLOR", "D945"}});
    DRW_Dictionary custom;
    custom.handle = kCustomDictionaryHandle;
    custom.parentHandle = kNodHandle;
    custom.m_entries.push_back(
        {kNestedXrecordName, 0xD901u, DictionaryEdge::SoftOwner});
    custom.m_entries.push_back(
        {"MAIN_DATALINK", kMainDataLinkHandle, DictionaryEdge::SoftOwner});
    custom.m_entries.push_back(
        {"TARGET_ALPHA", kTargetAlphaHandle, DictionaryEdge::SoftOwner});
    custom.m_entries.push_back(
        {"TARGET_BETA", kTargetBetaHandle, DictionaryEdge::SoftOwner});
    custom.m_entries.push_back(
        {"WHOLE_OPERATION_TYPED_XRECORD", kTypedXRecordHandle,
         DictionaryEdge::SoftOwner});
    writer.setNamedDictObjects({custom});
    return true;
}

bool installGeneratedImageDefinition(dxfRW& writer) {
    dxfRW::ImageDefinitionPlanEntry plan;
    plan.importGeneration = 7u;
    plan.sourceHandle = kImageDefHandle;
    plan.definition.handle = kImageDefHandle;
    plan.definition.name = "whole-operation-image.png";
    plan.definition.imgVersion = 2;
    plan.definition.u = 640.0;
    plan.definition.v = 480.0;
    plan.definition.loaded = 1;
    plan.definition.resolution = 5;
    return writer.setImageDefinitionPlan({plan});
}
} // namespace

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

class Export final : public dx_iface {
public:
    explicit Export(dxfRW& output) : output(output) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &output;
        line.handle = 0xD900u;
        line.basePoint = DRW_Coord(1.25, -2.5, 3.75);
        line.secPoint = DRW_Coord(4.5, 5.25, -6.0);
        polyline.handle = kPolylineHandle;
        polyline.flags = 1;
        polyline.addVertex(DRW_Vertex2D(1.0, 1.5, 0.0));
        polyline.addVertex(DRW_Vertex2D(4.0, 5.5, 0.5));
        polyline.addVertex(DRW_Vertex2D(8.0, 9.5, 0.0));
        circle.handle = 0xD902u;
        circle.basePoint = DRW_Coord(-7.0, 8.5, 9.25);
        circle.radious = 2.75;
        image.handle = 0xD903u;
        image.ref = kImageDefHandle;
        image.basePoint = DRW_Coord(2.0, 3.0, 4.0);
        image.secPoint = DRW_Coord(1.0, 0.0, 0.0);
        image.vVector = DRW_Coord(0.0, 1.0, 0.0);
        image.sizeu = 640.0;
        image.sizev = 480.0;
        mesh.handle = kMeshHandle;
        mesh.vertices = {DRW_Coord(0.0, 0.0, 0.0),
                         DRW_Coord(2.0, 0.0, 0.0),
                         DRW_Coord(0.0, 3.0, 0.0)};
        mesh.faces = {{0, 1, 2}};
        mesh.edges = {{0, 1}, {1, 2}, {2, 0}};
        mesh.creases = {0.5};
        DRW_MeshPropertyOverride overrideData;
        overrideData.subEntityMarker = 7;
        overrideData.propertyTypes = {0, 2};
        mesh.propertyOverrides.push_back(overrideData);
        spline.handle = kSplineHandle;
        spline.degree = 1;
        spline.controllist = {
            std::make_shared<DRW_Coord>(DRW_Coord(-1.0, 0.0, 2.0)),
            std::make_shared<DRW_Coord>(DRW_Coord(3.0, 4.0, 5.0))};
        spline.knotslist = {0.0, 0.0, 1.0, 1.0};
        hatch.handle = kHatchHandle;
        hatch.name = "ANSI31";
        hatch.solid = 0;
        hatch.hpattern = 0;
        hatch.angle = 0.25;
        hatch.scale = 1.5;
        DRW_Hatch::PatternLine pattern;
        pattern.angle = 0.5;
        pattern.baseX = 0.25;
        pattern.baseY = -0.5;
        pattern.offsetX = 0.0;
        pattern.offsetY = 1.25;
        pattern.dashList = {0.5, -0.25, 1.0};
        hatch.patternLines.push_back(pattern);
        auto loop = std::make_shared<DRW_HatchLoop>(2);
        auto boundary = std::make_shared<DRW_LWPolyline>();
        boundary->flags = 1;
        boundary->addVertex(DRW_Vertex2D(0.0, 0.0, 0.0));
        boundary->addVertex(DRW_Vertex2D(2.0, 0.0, 0.0));
        boundary->addVertex(DRW_Vertex2D(2.0, 3.0, 0.0));
        boundary->addVertex(DRW_Vertex2D(0.0, 3.0, 0.0));
        loop->objlist.push_back(boundary);
        loop->update();
        hatch.appendLoop(loop);
        hatch.seedPoints.emplace_back(0.5, 0.5, 0.0);
        leader.handle = kLeaderHandle;
        leader.style = "STANDARD";
        leader.vertnum = 3;
        leader.vertexlist = {
            std::make_shared<DRW_Coord>(DRW_Coord(1.0, 2.0, 3.0)),
            std::make_shared<DRW_Coord>(DRW_Coord(4.0, 5.0, 6.0)),
            std::make_shared<DRW_Coord>(DRW_Coord(7.0, 8.0, 9.0))};
        leader.offsetblock = DRW_Coord(0.25, 0.5, 0.75);
        mlineStyle.handle = kMLineStyleHandle;
        mlineStyle.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        mlineStyle.name = "WHOLE_OPERATION_MLINESTYLE";
        DRW_MLineElement mlineElement;
        mlineElement.offset = 0.5;
        mlineElement.color = 256;
        mlineElement.linetypeIndex = -1;
        mlineStyle.elements.push_back(mlineElement);
        mline.handle = kMLineHandle;
        mline.styleName = mlineStyle.name;
        mline.styleHandle = kMLineStyleHandle;
        mline.numLines = 1;
        mline.numVerts = 2;
        mline.basePoint = DRW_Coord(2.0, 3.0, 0.0);
        mline.vertlist.resize(2);
        mline.vertlist[0].position = DRW_Coord(2.0, 3.0, 0.0);
        mline.vertlist[0].vertexDir = DRW_Coord(1.0, 0.0, 0.0);
        mline.vertlist[0].miterDir = DRW_Coord(0.0, 1.0, 0.0);
        mline.vertlist[0].segParms = {{0.25}};
        mline.vertlist[0].areaFillParms = {{0.0}};
        mline.vertlist[1].position = DRW_Coord(7.0, 3.0, 0.0);
        mline.vertlist[1].vertexDir = DRW_Coord(1.0, 0.0, 0.0);
        mline.vertlist[1].miterDir = DRW_Coord(0.0, 1.0, 0.0);
        mline.vertlist[1].segParms = {{-0.5, 0.75}};
        mline.vertlist[1].areaFillParms = {{0.125}};
        polyline3.handle = kPolyline3Handle;
        polyline3.flags = 0;
        polyline3.basePoint = DRW_Coord(0.0, 0.0, 2.5);
        DRW_Vertex polylineVertex0(1.0, 2.0, 0.0, 0.25);
        polylineVertex0.handle = 0xD920u;
        DRW_Vertex polylineVertex1(4.0, 6.0, 0.0, 0.0);
        polylineVertex1.handle = 0xD921u;
        polyline3.addVertex(polylineVertex0);
        polyline3.addVertex(polylineVertex1);
        object.name = "XRECORD";
        object.handle = 0xD901u;
        object.parentHandle = kCustomDictionaryHandle;
        object.m_version = DRW::AC1027;
        object.groups.emplace_back(5, std::string("D901"));
        object.groups.emplace_back(330, std::string("D914"));
        object.groups.emplace_back(100, std::string("AcDbXrecord"));
        object.groups.emplace_back(1, std::string("whole operation payload"));

        mainDataLink.handle = kMainDataLinkHandle;
        mainDataLink.parentHandle = kCustomDictionaryHandle;
        mainDataLink.m_dataAdapter = "EXCEL";
        mainDataLink.m_description = "whole operation data link";
        mainDataLink.m_tooltip = "allocation retry";
        mainDataLink.m_connectionString = "inert://whole-operation.xlsx";
        mainDataLink.m_customDataCount = 2;
        mainDataLink.m_customData.push_back(
            {kTargetAlphaHandle, "sheet alpha"});
        mainDataLink.m_customData.push_back(
            {kTargetBetaHandle, "sheet beta"});

        targetAlpha.handle = kTargetAlphaHandle;
        targetAlpha.parentHandle = kCustomDictionaryHandle;
        targetAlpha.m_dataAdapter = "EXCEL";
        targetAlpha.m_description = "alpha target";
        targetBeta.handle = kTargetBetaHandle;
        targetBeta.parentHandle = kCustomDictionaryHandle;
        targetBeta.m_dataAdapter = "EXCEL";
        targetBeta.m_description = "beta target";

        DRW_Block block;
        block.handle = kInsertBlockHandle;
        block.name = kInsertBlockName;
        addBlock(block);
        DRW_Point blockPoint;
        blockPoint.handle = kInsertBlockPointHandle;
        blockPoint.basePoint = DRW_Coord(6.0, 7.0, 8.0);
        addPoint(blockPoint);
        DRW_Attdef blockAttdef;
        blockAttdef.handle = kAttdefHandle;
        blockAttdef.tag = "BLOCK_TAG";
        blockAttdef.prompt = "Enter block value";
        blockAttdef.text = "Default value";
        blockAttdef.basePoint = DRW_Coord(2.0, 4.0, 0.0);
        blockAttdef.height = 1.75;
        blockAttdef.style = "STANDARD";
        addAttDef(blockAttdef);
        endBlock();

        insert.handle = kInsertHandle;
        insert.name = kInsertBlockName;
        insert.basePoint = DRW_Coord(10.0, 20.0, 0.0);
        auto attribute = std::make_shared<DRW_Attrib>();
        attribute->handle = kInsertAttribHandle;
        attribute->tag = "WHOLE_OPERATION_TAG";
        attribute->text = "whole operation attribute";
        attribute->basePoint = DRW_Coord(11.0, 21.0, 0.0);
        attribute->height = 2.5;
        attribute->style = "STANDARD";
        insert.attlist.push_back(std::move(attribute));
        auto secondAttribute = std::make_shared<DRW_Attrib>();
        secondAttribute->handle = 0xD932u;
        secondAttribute->tag = "WHOLE_OPERATION_TAG2";
        secondAttribute->text = "second whole operation attribute";
        secondAttribute->basePoint = DRW_Coord(12.0, 22.0, 1.0);
        secondAttribute->height = 1.25;
        secondAttribute->style = "STANDARD";
        insert.attlist.push_back(std::move(secondAttribute));

        face.handle = kFaceHandle;
        face.basePoint = DRW_Coord(0.0, 0.0, 1.0);
        face.secPoint = DRW_Coord(3.0, 0.0, 2.0);
        face.thirdPoint = DRW_Coord(3.0, 4.0, 3.0);
        face.fourPoint = DRW_Coord(0.0, 4.0, 4.0);
        face.invisibleflag = DRW_3Dface::FirstEdge | DRW_3Dface::ThirdEdge;
        dimension.handle = 0xD960u;
        dimension.setDefPoint(DRW_Coord(-3.0, 4.0, 5.0));
        dimension.setTextPoint(DRW_Coord(1.5, 2.5, 0.0));
        dimension.setText("whole-operation dimension");
        dimension.setStyle("STANDARD");
        dynamicConstraintDimension.handle =
            kDynamicConstraintDimensionHandle;
        dynamicConstraintDimension.layer = "*ADSK_CONSTRAINTS";
        dynamicConstraintDimension.setDefPoint(DRW_Coord{15.0, 16.0, 17.0});
        dynamicConstraintDimension.setTextPoint(DRW_Coord{15.5, 16.5, 0.0});
        dynamicConstraintDimension.setText(
            "whole-operation dynamic constraint");
        dynamicConstraintDimension.setStyle("STANDARD");
        const auto initDimension = [](DRW_Dimension& value,
                                      std::uint32_t handle,
                                      double coordinate) {
            value.handle = handle;
            value.setDefPoint(DRW_Coord(coordinate, coordinate + 1.0,
                                        coordinate + 2.0));
            value.setTextPoint(DRW_Coord(coordinate + 0.5,
                                         coordinate + 1.5, 0.0));
            value.setText("whole-operation subtype dimension");
            value.setStyle("STANDARD");
        };
        initDimension(linearDimension, 0xD961u, 10.0);
        linearDimension.setDef1Point(DRW_Coord(10.0, 11.0, 12.0));
        linearDimension.setDef2Point(DRW_Coord(13.0, 14.0, 15.0));
        initDimension(radialDimension, 0xD962u, 20.0);
        radialDimension.setDiameterPoint(DRW_Coord(22.0, 20.0, 20.0));
        radialDimension.setLeaderLength(3.0);
        initDimension(diametricDimension, 0xD963u, 30.0);
        diametricDimension.setDiameter1Point(DRW_Coord(32.0, 30.0, 30.0));
        diametricDimension.setLeaderLength(4.0);
        initDimension(angularDimension, 0xD964u, 40.0);
        angularDimension.setFirstLine1(DRW_Coord(40.0, 40.0, 0.0));
        angularDimension.setFirstLine2(DRW_Coord(41.0, 40.0, 0.0));
        angularDimension.setSecondLine1(DRW_Coord(40.0, 41.0, 0.0));
        angularDimension.setSecondLine2(DRW_Coord(41.0, 41.0, 0.0));
        angularDimension.setDimPoint(DRW_Coord(40.5, 40.5, 0.0));
        initDimension(angular3pDimension, 0xD965u, 50.0);
        angular3pDimension.setFirstLine(DRW_Coord(50.0, 50.0, 0.0));
        angular3pDimension.setSecondLine(DRW_Coord(51.0, 50.0, 0.0));
        angular3pDimension.SetVertexPoint(DRW_Coord(50.0, 51.0, 0.0));
        angular3pDimension.setDimPoint(DRW_Coord(50.5, 50.5, 0.0));
        initDimension(ordinateDimension, 0xD966u, 60.0);
        ordinateDimension.setFirstLine(DRW_Coord(61.0, 62.0, 63.0));
        ordinateDimension.setSecondLine(DRW_Coord(64.0, 65.0, 66.0));
        arcDimension.handle = 0xD967u;
        arcDimension.setArcDefPoint(DRW_Coord(70.0, 71.0, 72.0));
        arcDimension.setTextPoint(DRW_Coord(70.5, 71.5, 0.0));
        arcDimension.setText("whole-operation arc dimension");
        arcDimension.setStyle("STANDARD");
        arcDimension.setExtLine1(DRW_Coord(71.0, 70.0, 0.0));
        arcDimension.setExtLine2(DRW_Coord(71.0, 72.0, 0.0));
        arcDimension.setArcCenter(DRW_Coord(70.0, 71.0, 0.0));
        arcDimension.setLeaderPt1(DRW_Coord(72.0, 71.0, 0.0));
        arcDimension.leaderPt2 = DRW_Coord(73.0, 71.0, 0.0);
        arcDimension.arcStartAngle = 0.25;
        arcDimension.arcEndAngle = 1.75;
        arcDimension.isPartial = true;
        arcDimension.hasLeader = true;
        largeRadialDimension.handle = 0xD968u;
        largeRadialDimension.setCenterPoint(DRW_Coord(80.0, 81.0, 82.0));
        largeRadialDimension.setTextPoint(DRW_Coord(80.5, 81.5, 0.0));
        largeRadialDimension.setText("whole-operation large radial");
        largeRadialDimension.setStyle("STANDARD");
        largeRadialDimension.setChordPoint(DRW_Coord(83.0, 80.0, 82.0));
        largeRadialDimension.overrideCenterPoint = DRW_Coord(80.0, 81.0, 82.0);
        largeRadialDimension.jogPoint = DRW_Coord(84.0, 82.0, 0.0);
        largeRadialDimension.jogAngle = 0.375;
        tolerance.handle = 0xD969u;
        tolerance.dimStyleName = "STANDARD";
        tolerance.insertionPoint = DRW_Coord(90.0, 91.0, 92.0);
        tolerance.text = "whole-operation tolerance";
        tolerance.extPoint = DRW_Coord(0.0, 0.0, 1.0);
        tolerance.xAxisDirectionVector = DRW_Coord(1.0, 0.0, 0.0);
        rtext.handle = 0xD96Au;
        rtext.basePoint = DRW_Coord(95.0, 96.0, 0.0);
        rtext.text = "whole-operation RTEXT";
        rtext.height = 2.5;
        rtext.angle = 30.0;
        rtext.style = "STANDARD";
        rtext.m_rTextFlags = 1;
        rtext.extPoint = DRW_Coord(0.0, 0.0, 1.0);
        arcAlignedText.handle = 0xD96Bu;
        arcAlignedText.text = "whole-operation arc-aligned text";
        arcAlignedText.style = "STANDARD";
        arcAlignedText.m_fontName = "SIMPLEX.SHX";
        arcAlignedText.m_bigFontName = "BIGFONT.SHX";
        arcAlignedText.m_center = DRW_Coord(100.0, 101.0, 0.0);
        arcAlignedText.m_radius = 12.0;
        arcAlignedText.m_startAngle = 0.25;
        arcAlignedText.m_endAngle = 1.25;
        arcAlignedText.m_xScale = "1.2";
        arcAlignedText.m_textSize = "2.25";
        arcAlignedText.m_charSpacing = "0.75";
        arcAlignedText.m_offsetFromArc = "0.5";
        arcAlignedText.m_rightOffset = "0.25";
        arcAlignedText.m_leftOffset = "0.125";
        arcAlignedText.m_rawColor = 0x010203;
        arcAlignedText.m_characterSet = 1;
        arcAlignedText.m_pitchAndFamily = 2;
        arcAlignedText.m_isShx = 1;
        arcAlignedText.m_isBold = 1;
        arcAlignedText.m_isItalic = 1;
        arcAlignedText.m_isUnderlined = 1;
        arcAlignedText.m_alignment = 2;
        arcAlignedText.m_isReverse = 1;
        arcAlignedText.m_wizardFlag = 1;
        arcAlignedText.m_textPosition = 1;
        arcAlignedText.m_textDirection = 1;
        light.handle = 0xD96Cu;
        light.m_classVersion = 1;
        light.m_name = "WHOLE_OPERATION_LIGHT";
        light.m_type = 2;
        light.m_status = true;
        light.m_color = 3;
        light.m_plotGlyph = true;
        light.m_intensity = 2.5;
        light.m_position = DRW_Coord(110.0, 111.0, 112.0);
        light.m_target = DRW_Coord(113.0, 114.0, 115.0);
        light.m_attenuationType = 1;
        light.m_useAttenuationLimits = true;
        light.m_attenuationStartLimit = 0.5;
        light.m_attenuationEndLimit = 12.5;
        light.m_hotspotAngle = 0.25;
        light.m_falloffAngle = 0.75;
        light.m_castShadows = true;
        light.m_shadowType = 1;
        light.m_shadowMapSize = 512;
        light.m_shadowMapSoftness = 2;
        DRW_View cameraView;
        cameraView.handle = 0xD96Eu;
        cameraView.name = "WHOLE_OPERATION_CAMERA_VIEW";
        cameraView.size = DRW_Coord(640.0, 480.0, 0.0);
        cameraView.center = DRW_Coord(0.5, 0.5, 0.0);
        cameraView.viewDirectionFromTarget = DRW_Coord(0.0, 0.0, 1.0);
        cameraView.targetPoint = DRW_Coord(10.0, 20.0, 30.0);
        storage.views.push_back(cameraView);
        camera.handle = 0xD96Du;
        camera.m_viewHandle = cameraView.handle;
        geoPositionMarker.handle = 0xD96Fu;
        geoPositionMarker.m_classVersion = 1u;
        geoPositionMarker.m_position = DRW_Coord(120.0, 121.0, 122.0);
        geoPositionMarker.m_radius = 6.5;
        geoPositionMarker.m_notes = "whole-operation geo marker";
        geoPositionMarker.m_landingGap = 0.75;
        geoPositionMarker.m_mtextVisible = true;
        geoPositionMarker.m_textAlignment = 2u;
        geoPositionMarker.m_enableFrameText = false;
        geoPositionMarkerEmbedded.handle = 0xD970u;
        geoPositionMarkerEmbedded.m_classVersion = 1u;
        geoPositionMarkerEmbedded.m_position = DRW_Coord(123.0, 124.0, 125.0);
        geoPositionMarkerEmbedded.m_radius = 8.5;
        geoPositionMarkerEmbedded.m_notes = "whole-operation embedded geo marker";
        geoPositionMarkerEmbedded.m_landingGap = 1.25;
        geoPositionMarkerEmbedded.m_mtextVisible = true;
        geoPositionMarkerEmbedded.m_textAlignment = 1u;
        geoPositionMarkerEmbedded.m_enableFrameText = true;
        geoPositionMarkerEmbedded.mtext = std::make_unique<DRW_MText>();
        geoPositionMarkerEmbedded.mtext->text = "whole-operation embedded frame";
        geoPositionMarkerEmbedded.mtext->basePoint =
            DRW_Coord(123.0, 124.0, 125.0);
        geoPositionMarkerEmbedded.mtext->height = 1.75;
        geoPositionMarkerEmbedded.mtext->widthscale = 5.0;
        geoPositionMarkerEmbedded.mtext->style = "STANDARD";
        sectionObject.handle = 0xD971u;
        sectionObject.m_state = 3u;
        sectionObject.m_flags = 5u;
        sectionObject.m_name = "WHOLE_OPERATION_SECTION";
        sectionObject.m_vertDir = DRW_Coord(0.0, 0.0, 1.0);
        sectionObject.m_topHeight = 12.0;
        sectionObject.m_bottomHeight = -4.0;
        sectionObject.m_indicatorAlpha = 128u;
        sectionObject.m_indicatorColor = 6u;
        sectionObject.m_verts = {DRW_Coord(1.0, 2.0, 3.0),
                                 DRW_Coord(4.0, 5.0, 6.0)};
        sectionObject.m_blVerts = {DRW_Coord(-1.0, -2.0, -3.0)};
        mpolygon.handle = 0xD972u;
        mpolygon.dxfVersion = 1;
        mpolygon.name = "SOLID";
        mpolygon.solid = 1;
        mpolygon.basePoint.z = 2.0;
        mpolygon.fillColorAci = 5;
        mpolygon.annotatedBoundary = 1;
        mpolygon.xDirX = 1.0;
        auto mpolygonLoop = std::make_shared<DRW_HatchLoop>(2);
        auto mpolygonBoundary = std::make_shared<DRW_LWPolyline>();
        mpolygonBoundary->flags = 1;
        mpolygonBoundary->addVertex(DRW_Vertex2D(1.0, 1.0, 0.0));
        mpolygonBoundary->addVertex(DRW_Vertex2D(4.0, 1.0, 0.0));
        mpolygonBoundary->addVertex(DRW_Vertex2D(4.0, 3.0, 0.0));
        mpolygonBoundary->addVertex(DRW_Vertex2D(1.0, 3.0, 0.0));
        mpolygonLoop->objlist.push_back(mpolygonBoundary);
        mpolygonLoop->update();
        mpolygon.appendLoop(mpolygonLoop);
        viewport.handle = 0xD973u;
        viewport.pswidth = 210.0;
        viewport.psheight = 148.0;
        viewport.vpstatus = 1;
        viewport.vpID = 2;
        viewport.centerPX = 105.0;
        viewport.centerPY = 74.0;
        viewport.viewDir = DRW_Coord(0.0, 0.0, 1.0);
        viewport.viewTarget = DRW_Coord(10.0, 20.0, 30.0);
        viewport.viewHeight = 100.0;
        viewport.snapSpPX = 5.0;
        viewport.snapSpPY = 6.0;
        viewport.gridSpX = 10.0;
        viewport.gridSpY = 12.0;
        viewport.styleSheet = "WHOLE_OPERATION_STYLE";
        arc.handle = kArcHandle;
        arc.basePoint = DRW_Coord(-3.0, 2.0, 5.0);
        arc.radious = 6.5;
        arc.staangle = 0.25;
        arc.endangle = 2.75;
        arc.thickness = 1.25;
        ellipse.handle = kEllipseHandle;
        ellipse.basePoint = DRW_Coord(7.0, -4.0, 2.0);
        ellipse.secPoint = DRW_Coord(4.0, 1.0, 0.0);
        ellipse.ratio = 0.5;
        ellipse.staparam = 0.2;
        ellipse.endparam = 2.4;
        ellipse.extPoint = DRW_Coord(0.0, 0.0, 1.0);
        point.handle = kPointHandle;
        point.basePoint = DRW_Coord(-2.0, 3.0, 4.0);
        point.xAxisAngle = 0.75;
        line3d.handle = k3DLineHandle;
        line3d.basePoint = DRW_Coord(1.0, 2.0, 3.0);
        line3d.secPoint = DRW_Coord(4.0, 5.0, 6.0);
        solid.handle = kSolidHandle;
        solid.basePoint = DRW_Coord(0.0, 0.0, 1.5);
        solid.secPoint = DRW_Coord(3.0, 0.0, 1.5);
        solid.thirdPoint = DRW_Coord(3.0, 2.0, 1.5);
        solid.fourPoint = DRW_Coord(0.0, 2.0, 1.5);
        trace.handle = kTraceHandle;
        trace.basePoint = DRW_Coord(0.0, 0.0, 2.0);
        trace.secPoint = DRW_Coord(3.0, 0.0, 2.0);
        trace.thirdPoint = DRW_Coord(3.0, 2.0, 2.0);
        trace.fourPoint = DRW_Coord(0.0, 2.0, 2.0);
        text.handle = kTextHandle;
        text.basePoint = DRW_Coord(12.0, -3.0, 1.0);
        text.text = "whole-operation TEXT";
        text.height = 2.0;
        text.angle = 15.0;
        text.widthscale = 1.2;
        text.style = "STANDARD";
        mtext.handle = kMTextHandle;
        mtext.basePoint = DRW_Coord(4.0, 5.0, 6.0);
        mtext.text = "whole-operation MTEXT";
        mtext.height = 1.5;
        mtext.widthscale = 12.0;
        mtext.style = "STANDARD";
        mtext.m_definedHeight = 2.25;
        mtext.m_definedHeightPresent = true;
        ray.handle = kRayHandle;
        ray.basePoint = DRW_Coord(-5.0, 7.0, 2.0);
        ray.secPoint = DRW_Coord(1.0, 2.0, 3.0);
        xline.handle = kXlineHandle;
        xline.basePoint = DRW_Coord(8.0, -6.0, 4.0);
        xline.secPoint = DRW_Coord(-2.0, 1.0, 0.5);
        helix.handle = kHelixHandle;
        helix.m_majorVersion = 1;
        helix.m_maintVersion = 2;
        helix.degree = 2;
        helix.flags = 8;
        helix.knotslist = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
        helix.controllist = {
            std::make_shared<DRW_Coord>(DRW_Coord(1.0, 0.0, 0.0)),
            std::make_shared<DRW_Coord>(DRW_Coord(0.0, 1.0, 2.0)),
            std::make_shared<DRW_Coord>(DRW_Coord(-1.0, 0.0, 4.0))};
        helix.axisBasePt = DRW_Coord(2.0, 3.0, 4.0);
        helix.startPt = DRW_Coord(3.0, 3.0, 4.0);
        helix.axisVector = DRW_Coord(0.0, 0.0, 1.0);
        helix.radius = 1.0;
        helix.turns = 2.5;
        helix.turnHeight = 3.0;
        helix.handedness = true;
        helix.constraintType = 1;
        material.handle = kMaterialHandle;
        material.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        material.m_name = "WHOLE_OPERATION_MATERIAL";
        material.m_description = "material allocation retry";
        loftedSurface.handle = kLoftedSurfaceHandle;
        loftedSurface.modelerFormatVersion = 1;
        loftedSurface.uIsolines = 10;
        loftedSurface.vIsolines = 11;
        loftedSurface.planeNormalLoftingType = 1;
        loftedSurface.startDraftAngle = 0.2;
        loftedSurface.endDraftAngle = 0.3;
        loftedSurface.startDraftMagnitude = 0.4;
        loftedSurface.endDraftMagnitude = 0.5;
        loftedSurface.arcLengthParameterization = true;
        loftedSurface.closedSurfaces = true;
        loftedSurface.solid = true;
        loftedSurface.ruledSurface = true;
        loftedSurface.virtualGuide = true;
        loftedSurface.loftEntityTransform[3] = 0.25;
        loftedSurface.loftEntityTransform[7] = 0.5;
        loftedSurface.loftEntityTransform[11] = 0.75;
        loftedSurface.dxfReferenceData.emplace_back(90, std::int32_t{2});
        loftedSurface.dxfReferenceData.emplace_back(
            310, std::vector<std::uint8_t>{0x12u, 0x34u});
        sweptSurface.handle = kSweptSurfaceHandle;
        sweptSurface.modelerFormatVersion = 1;
        sweptSurface.uIsolines = 12;
        sweptSurface.vIsolines = 13;
        sweptSurface.sweepEntityId = 101u;
        sweptSurface.sweepData = {0x21u, 0x22u};
        sweptSurface.pathEntityId = 202u;
        sweptSurface.pathData = {0x31u, 0x32u, 0x33u};
        sweptSurface.referenceVector = DRW_Coord(1.0, 2.0, 3.0);
        sweptSurface.sweepAlignmentFlags = 2;
        sweptSurface.pathFlags = 3;
        sweptSurface.draftAngle = 0.125;
        sweptSurface.draftStartDistance = 0.25;
        sweptSurface.draftEndDistance = 0.5;
        sweptSurface.twistAngle = 0.75;
        sweptSurface.scaleFactor = 1.25;
        sweptSurface.alignAngle = 0.875;
        sweptSurface.sweepEntityTransform[3] = 0.5;
        sweptSurface.pathEntityTransform[7] = 0.75;
        sweptSurface.sweepEntityTransformed[11] = 1.0;
        sweptSurface.pathEntityTransformed[15] = 1.25;
        sweptSurface.solid = true;
        sweptSurface.alignStart = true;
        sweptSurface.bank = true;
        sweptSurface.basePointSet = true;
        sweptSurface.sweepEntityTransformComputed = true;
        sweptSurface.pathEntityTransformComputed = true;
        planeSurface.handle = kPlaneSurfaceHandle;
        planeSurface.modelerFormatVersion = 1;
        planeSurface.uIsolines = 2;
        planeSurface.vIsolines = 3;
        extrudedSurface.handle = kExtrudedSurfaceHandle;
        extrudedSurface.modelerFormatVersion = 1;
        extrudedSurface.uIsolines = 4;
        extrudedSurface.vIsolines = 5;
        extrudedSurface.classId = 2;
        extrudedSurface.dxfBinaryData = {0x21u, 0x43u};
        extrudedSurface.sweepVector = DRW_Coord(0.0, 0.0, 1.0);
        extrudedSurface.referenceVector = DRW_Coord(1.0, 2.0, 3.0);
        extrudedSurface.sweepAlignmentFlags = 3;
        extrudedSurface.pathFlags = 7;
        extrudedSurface.draftAngle = 0.1;
        extrudedSurface.draftStartDistance = 0.25;
        extrudedSurface.draftEndDistance = 0.5;
        extrudedSurface.twistAngle = 0.75;
        extrudedSurface.scaleFactor = 1.25;
        extrudedSurface.alignAngle = 0.5;
        extrudedSurface.solid = true;
        extrudedSurface.alignStart = true;
        extrudedSurface.bank = true;
        extrudedSurface.basePointSet = true;
        extrudedSurface.sweepEntityTransformComputed = true;
        extrudedSurface.pathEntityTransformComputed = true;
        extrudedSurface.extrudedTransform[3] = 0.25;
        extrudedSurface.sweepEntityTransform[3] = 1.25;
        extrudedSurface.pathEntityTransform[7] = 1.5;
        revolvedSurface.handle = kRevolvedSurfaceHandle;
        revolvedSurface.modelerFormatVersion = 1;
        revolvedSurface.uIsolines = 6;
        revolvedSurface.vIsolines = 7;
        revolvedSurface.id = 2;
        revolvedSurface.dxfBinaryData = {0x65u, 0x87u};
        revolvedSurface.axisPoint = DRW_Coord(1.0, 2.0, 3.0);
        revolvedSurface.axisVector = DRW_Coord(0.0, 1.0, 2.0);
        revolvedSurface.revolveAngle = 1.5;
        revolvedSurface.startAngle = 0.25;
        revolvedSurface.draftAngle = 0.5;
        revolvedSurface.draftStartDistance = 0.75;
        revolvedSurface.draftEndDistance = 1.0;
        revolvedSurface.twistAngle = 1.25;
        revolvedSurface.solid = true;
        revolvedSurface.closeToAxis = true;
        revolvedSurface.transform[3] = 0.25;
        revolvedSurface.transform[7] = 0.5;
        revolvedSurface.transform[11] = 0.75;
        nurbsSurface.handle = kNurbsSurfaceHandle;
        nurbsSurface.modelerFormatVersion = 1;
        nurbsSurface.uIsolines = 14;
        nurbsSurface.vIsolines = 15;
        nurbsSurface.short170 = 16;
        nurbsSurface.cvHullDisplay = true;
        nurbsSurface.uvec1 = DRW_Coord(1.0, 0.0, 0.0);
        nurbsSurface.vvec1 = DRW_Coord(0.0, 1.0, 0.0);
        nurbsSurface.uvec2 = DRW_Coord(2.0, 0.0, 0.0);
        nurbsSurface.vvec2 = DRW_Coord(0.0, 2.0, 0.0);
        scale.handle = kScaleHandle;
        scale.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        scale.name = "WHOLE_OPERATION_SCALE";
        scale.paperUnits = 2.0;
        scale.drawingUnits = 5.0;
        scale.isUnitScale = false;
        background.handle = kBackgroundHandle;
        background.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        background.m_kind = DRW_Background::Solid;
        background.m_classVersion = 1;
        background.m_solidColor = 0x010203;
        plotSettings.handle = kPlotSettingsHandle;
        plotSettings.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        plotSettings.pageSetupName = "WHOLE_OPERATION_PAGE_SETUP";
        plotSettings.printerConfig = "WHOLE_OPERATION_PRINTER";
        plotSettings.paperSize = "ISO_A4_(210.00_x_297.00_MM)";
        plotSettings.paperWidth = 210.0;
        plotSettings.paperHeight = 297.0;
        plotSettings.marginLeft = 4.0;
        plotSettings.marginBottom = 5.0;
        plotSettings.marginRight = 6.0;
        plotSettings.marginTop = 7.0;
        plotSettings.plotOriginX = 1.0;
        plotSettings.plotOriginY = 2.0;
        plotSettings.paperUnits = 1;
        plotSettings.plotRotation = 2;
        plotSettings.plotType = 1;
        plotSettings.windowMinX = 10.0;
        plotSettings.windowMinY = 11.0;
        plotSettings.windowMaxX = 20.0;
        plotSettings.windowMaxY = 21.0;
        plotSettings.realWorldUnits = 1.0;
        plotSettings.drawingUnits = 2.0;
        plotSettings.currentStyleSheet = "WHOLE_OPERATION_STYLE";
        plotSettings.scaleType = 3;
        plotSettings.scaleFactor = 0.5;
        plotSettings.paperImageOriginX = 0.25;
        plotSettings.paperImageOriginY = 0.75;
        plotSettings.shadePlotMode = 1;
        plotSettings.shadePlotResLevel = 2;
        plotSettings.shadePlotCustomDPI = 150;
        wipeoutVariables.handle = kWipeoutVariablesHandle;
        wipeoutVariables.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        wipeoutVariables.m_classVersion = 0;
        wipeoutVariables.m_displayFrame = 1;
        sun.handle = kSunHandle;
        sun.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        sun.m_classVersion = 0;
        sun.m_isOn = true;
        sun.m_color = 7;
        sun.m_intensity = 0.625;
        sun.m_hasShadow = true;
        sun.m_julianDay = 2460000;
        sun.m_milliseconds = 43210000;
        sun.m_isDaylightSavings = true;
        sun.m_shadowType = 2;
        sun.m_shadowMapSize = 1024;
        sun.m_shadowSoftness = 3;
        ole2Frame.handle = kOle2FrameHandle;
        ole2Frame.m_oleVersion = 2u;
        ole2Frame.m_oleClient = "WHOLE_OPERATION_OLE2";
        ole2Frame.m_pt1 = DRW_Coord(1.0, 2.0, 0.0);
        ole2Frame.m_pt2 = DRW_Coord(7.0, 9.0, 0.0);
        ole2Frame.m_flags = 3u;
        ole2Frame.m_mode = 1u;
        ole2Frame.m_lockAspect = 1u;
        ole2Frame.m_payloadPresent = true;
        ole2Frame.m_payloadBytes = {0x00u, 0x1Fu, 0x80u, 0xFFu, 0x35u};
        ole2Frame.m_payloadByteCount = static_cast<std::uint32_t>(
            ole2Frame.m_payloadBytes.size());
        ole2Frame.m_declaredPayloadLength = ole2Frame.m_payloadByteCount;
        oleFrame.handle = kOleFrameHandle;
        oleFrame.m_flags = 2u;
        oleFrame.m_payloadPresent = true;
        oleFrame.m_payloadBytes = {0x80u, 0x00u, 0xFFu, 0xA5u};
        oleFrame.m_payloadByteCount = static_cast<std::uint32_t>(
            oleFrame.m_payloadBytes.size());
        oleFrame.m_declaredPayloadLength = oleFrame.m_payloadByteCount;
        shape.handle = kShapeHandle;
        shape.m_insertionPoint = DRW_Coord(8.0, 9.0, 10.0);
        shape.m_scale = 1.75;
        shape.m_rotation = 0.5;
        shape.m_widthFactor = 0.8;
        shape.m_oblique = 0.1;
        shape.m_thickness = 0.25;
        shape.m_extrusion = DRW_Coord(0.0, 0.0, 1.0);
        shape.m_styleName = "STANDARD";
        rasterVariables.handle = kRasterVariablesHandle;
        rasterVariables.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        rasterVariables.m_classVersion = 0;
        rasterVariables.m_imageFrame = 1;
        rasterVariables.m_imageQuality = 1;
        rasterVariables.m_units = 4;
        dictionaryVar.handle = kDictionaryVarHandle;
        dictionaryVar.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        dictionaryVar.m_schema = 1;
        dictionaryVar.m_value = "whole-operation dictionary value";
        dbColor.handle = kDbColorHandle;
        dbColor.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        dbColor.rgb = 0x123456;
        dbColor.setDxfColorIndex(7u);
        dbColor.bookName = "WHOLE_OPERATION_BOOK";
        dbColor.name = "ACCENT";
        underlayDefinition.handle = kUnderlayDefinitionHandle;
        underlayDefinition.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        underlayDefinition.kind = DRW_UnderlayDefinition::PDF;
        underlayDefinition.filename = "whole-operation-underlay.pdf";
        underlayDefinition.sheetName = "sheet-1";
        underlay.handle = kUnderlayHandle;
        underlay.kind = DRW_Underlay::PDF;
        underlay.position = DRW_Coord(141.0, 142.0, 143.0);
        underlay.scale = DRW_Coord(2.0, 3.0, 1.0);
        underlay.rotation = 0.25;
        underlay.flags = 2u;
        underlay.contrast = 80u;
        underlay.fade = 5u;
        underlay.definitionHandle = kUnderlayDefinitionHandle;
        underlay.clipBoundary = {DRW_Coord(0.0, 0.0), DRW_Coord(64.0, 0.0),
            DRW_Coord(64.0, 48.0), DRW_Coord(0.0, 48.0)};
        underlay.inverseClipBoundary = {DRW_Coord(8.0, 8.0),
            DRW_Coord(56.0, 8.0), DRW_Coord(56.0, 40.0),
            DRW_Coord(8.0, 40.0)};
        underlay.flags = static_cast<std::uint8_t>(underlay.flags | 0x10u);
        lineType.handle = kLineTypeHandle;
        lineType.name = "WHOLE_OPERATION_LTYPE";
        lineType.desc = "whole-operation line type";
        lineType.path = {10.0, -5.0};
        lineType.size = 2;
        lineType.length = 15.0;
        storage.lineTypes.push_back(lineType);
        typedXRecord.handle = kTypedXRecordHandle;
        typedXRecord.parentHandle = kCustomDictionaryHandle;
        typedXRecord.m_values.emplace_back(
            1, UTF8STRING("whole-operation typed XRECORD"));
        typedXRecord.m_values.emplace_back(90, std::int32_t{42});
        multiLeader.handle = kMultiLeaderHandle;
        multiLeader.styleContentType = 2u;
        multiLeader.context.hasTextContents = true;
        multiLeader.context.textLabel = "whole-operation multi-leader text";
        multiLeader.context.contentBasePoint = DRW_Coord{2.0, 3.0, 4.0};
        multiLeader.context.textLocation = DRW_Coord{5.0, 6.0, 7.0};
        multiLeader.context.textHeight = 1.5;
    }

    void writeEntities() override {
        const auto check = [](const char* name, bool result) {
            if (!result && !allocationTracking)
                std::cerr << "whole-write entity failed: " << name << '\n';
            return result;
        };
        entitiesWritten = check("LINE", output.writeLine(&line))
            && check("CIRCLE", output.writeCircle(&circle))
            && check("LWPOLYLINE", output.writeLWPolyline(&polyline))
            && check("IMAGE", output.writeImage(
                &image, "whole-operation-image.png", 7u, image.ref) != nullptr)
            && check("MESH", output.writeMesh(&mesh))
            && check("SPLINE", output.writeSpline(&spline))
            && check("HATCH", output.writeHatch(&hatch))
            && check("LEADER", output.writeLeader(&leader))
            && check("MLINE", output.writeMLine(&mline))
            && check("POLYLINE", output.writePolyline(&polyline3))
            && check("INSERT", output.writeInsert(&insert))
            && check("3DFACE", output.writeLine(&face))
            && check("DIMENSION", output.writeDimension(&dimension))
            && check("DYNAMIC_CONSTRAINT_DIMENSION",
                     output.writeDimension(&dynamicConstraintDimension))
            && check("DIMLINEAR", output.writeDimension(&linearDimension))
            && check("DIMRADIAL", output.writeDimension(&radialDimension))
            && check("DIMDIAMETRIC", output.writeDimension(&diametricDimension))
            && check("DIMANGULAR", output.writeDimension(&angularDimension))
            && check("DIMANGULAR3P", output.writeDimension(&angular3pDimension))
            && check("DIMORDINATE", output.writeDimension(&ordinateDimension))
            && check("ARC_DIMENSION", output.writeDimension(&arcDimension))
            && check("LARGE_RADIAL_DIMENSION", output.writeDimension(&largeRadialDimension))
            && check("TOLERANCE", output.writeTolerance(&tolerance))
            && check("RTEXT", output.writeRText(&rtext))
            && check("ARCALIGNEDTEXT", output.writeArcAlignedText(&arcAlignedText))
            && check("LIGHT", output.writeLight(&light))
            && check("CAMERA", output.writeCamera(&camera))
            && check("GEOPOSITIONMARKER", output.writeGeoPositionMarker(&geoPositionMarker))
            && check("GEOPOSITIONMARKER_EMBEDDED", output.writeGeoPositionMarker(&geoPositionMarkerEmbedded))
            && check("SECTIONOBJECT", output.writeSectionObject(&sectionObject))
            && check("MPOLYGON", output.writeMPolygon(&mpolygon))
            && check("VIEWPORT", output.writeViewport(&viewport))
            && check("ARC", output.writeArc(&arc))
            && check("ELLIPSE", output.writeEllipse(&ellipse))
            && check("POINT", output.writePoint(&point))
            && check("3DLINE", output.write3DLine(&line3d))
            && check("SOLID", output.writeSolid(&solid))
            && check("TRACE", output.writeTrace(&trace))
            && check("TEXT", output.writeText(&text))
            && check("MTEXT", output.writeMText(&mtext))
            && check("RAY", output.writeRay(&ray))
            && check("XLINE", output.writeXline(&xline))
            && check("HELIX", output.writeHelix(&helix))
            && check("MULTILEADER", output.writeMultiLeader(&multiLeader))
            && check("OLE2FRAME", output.writeOle2Frame(&ole2Frame))
            && check("OLEFRAME", output.writeOleFrame(&oleFrame))
            && check("SHAPE", output.writeShape(&shape))
            && check("UNDERLAY", output.writeUnderlay(&underlay))
            && check("LOFTEDSURFACE", output.writeSurface(&loftedSurface))
            && check("SWEPTSURFACE", output.writeSurface(&sweptSurface))
            && check("PLANESURFACE", output.writeSurface(&planeSurface))
            && check("EXTRUDEDSURFACE", output.writeSurface(&extrudedSurface))
            && check("REVOLVEDSURFACE", output.writeSurface(&revolvedSurface))
            && check("NURBSSURFACE", output.writeSurface(&nurbsSurface));
    }

    void writeObjects() override {
        dx_iface::writeObjects();
        objectsWritten = output.writeMLineStyle(&mlineStyle)
            && output.writeScale(&scale)
            && output.writeBackground(&background, "SOLIDBACKGROUND")
            && output.writePlotSettings(&plotSettings)
            && output.writeRasterVariables(&rasterVariables)
            && output.writeDictionaryVar(&dictionaryVar)
            && output.writeDbColor(&dbColor)
            && output.writeXRecord(&typedXRecord)
            && output.writeUnderlayDefinition(&underlayDefinition)
            && output.writeWipeoutVariables(&wipeoutVariables)
            && output.writeSun(&sun)
            && output.writeMaterial(&material)
            && output.writeRawDxfObject(&object)
            && output.writeDataLink(&mainDataLink)
            && output.writeDataLink(&targetAlpha)
            && output.writeDataLink(&targetBeta);
    }

    bool finalizeDxfWrite() override {
        return entitiesWritten && objectsWritten
            && (!finalizeHook || finalizeHook());
    }

    std::function<bool()> finalizeHook;
    dxfRW& output;
    dx_data storage;
    DRW_Line line;
    DRW_Circle circle;
    DRW_LWPolyline polyline;
    DRW_Image image;
    DRW_Mesh mesh;
    DRW_Spline spline;
    DRW_Hatch hatch;
    DRW_Leader leader;
    DRW_MLine mline;
    DRW_MLineStyle mlineStyle;
    DRW_Polyline polyline3;
    DRW_Insert insert;
    DRW_3Dface face;
    DRW_DimAligned dimension;
    DRW_Dimension dynamicConstraintDimension;
    DRW_DimLinear linearDimension;
    DRW_DimRadial radialDimension;
    DRW_DimDiametric diametricDimension;
    DRW_DimAngular angularDimension;
    DRW_DimAngular3p angular3pDimension;
    DRW_DimOrdinate ordinateDimension;
    DRW_DimArc arcDimension;
    DRW_DimLargeRadial largeRadialDimension;
    DRW_Tolerance tolerance;
    DRW_RText rtext;
    DRW_ArcAlignedText arcAlignedText;
    DRW_Light light;
    DRW_Camera camera;
    DRW_GeoPositionMarker geoPositionMarker;
    DRW_GeoPositionMarker geoPositionMarkerEmbedded;
    DRW_SectionObject sectionObject;
    DRW_MPolygon mpolygon;
    DRW_Viewport viewport;
    DRW_Arc arc;
    DRW_Ellipse ellipse;
    DRW_Point point;
    DRW_3DLine line3d;
    DRW_Solid solid;
    DRW_Trace trace;
    DRW_Text text;
    DRW_MText mtext;
    DRW_Ray ray;
    DRW_Xline xline;
    DRW_Helix helix;
    DRW_Ole2Frame ole2Frame;
    DRW_OleFrame oleFrame;
    DRW_Shape shape;
    DRW_RasterVariables rasterVariables;
    DRW_DictionaryVar dictionaryVar;
    DRW_DbColor dbColor;
    DRW_UnderlayDefinition underlayDefinition;
    DRW_Underlay underlay;
    DRW_LType lineType;
    DRW_XRecord typedXRecord;
    DRW_MLeader multiLeader;
    DRW_Material material;
    DRW_LoftedSurface loftedSurface;
    DRW_SweptSurface sweptSurface;
    DRW_PlaneSurface planeSurface;
    DRW_ExtrudedSurface extrudedSurface;
    DRW_RevolvedSurface revolvedSurface;
    DRW_NurbsSurface nurbsSurface;
    DRW_Scale scale;
    DRW_Background background;
    DRW_PlotSettings plotSettings;
    DRW_WipeoutVariables wipeoutVariables;
    DRW_Sun sun;
    DRW_RawDxfObject object;
    DRW_DataLink mainDataLink;
    DRW_DataLink targetAlpha;
    DRW_DataLink targetBeta;
    bool entitiesWritten = false;
    bool objectsWritten = false;
};

class Readback final : public dx_iface {
public:
    Readback() {
        cData = &storage;
        currentBlock = storage.mBlock;
    }
    void addLine(const DRW_Line& value) override { ++lineCount; line = value; }
    void addCircle(const DRW_Circle& value) override {
        ++circleCount;
        circle = value;
    }
    void addLWPolyline(const DRW_LWPolyline& value) override {
        ++polylineCount;
        polyline = value;
    }
    void addImage(const DRW_Image* value) override {
        if (value != nullptr) {
            ++imageCount;
            image = *value;
        }
    }
    void addOle2Frame(const DRW_Ole2Frame& value) override {
        ++ole2FrameCount;
        ole2Frame = value;
    }
    void addOleFrame(const DRW_OleFrame& value) override {
        ++oleFrameCount;
        oleFrame = value;
    }
    void addShape(const DRW_Shape& value) override {
        ++shapeCount;
        shape = value;
    }
    void addRasterVariables(const DRW_RasterVariables& value) override {
        ++rasterVariablesCount;
        rasterVariables = value;
    }
    void addDictionaryVar(const DRW_DictionaryVar& value) override {
        ++dictionaryVarCount;
        dictionaryVar = value;
    }
    void addDbColor(const DRW_DbColor& value) override {
        ++dbColorCount;
        dbColor = value;
    }
    void addMLeader(const DRW_MLeader* value) override {
        dx_iface::addMLeader(value);
        if (value != nullptr) {
            ++multiLeaderCount;
            multiLeader = *value;
        }
    }
    void addXRecord(const DRW_XRecord& value) override {
        dx_iface::addXRecord(value);
        const bool isGeneratedTypedXRecord = std::any_of(
            value.m_values.begin(), value.m_values.end(),
            [](const DRW_Variant& item) {
                return item.stringEquals("whole-operation typed XRECORD");
            });
        if (isGeneratedTypedXRecord) {
            ++typedXRecordCount;
            typedXRecord = value;
        }
    }
    void addUnderlay(const DRW_Underlay* value) override {
        if (value != nullptr) {
            ++underlayCount;
            underlay = *value;
        }
    }
    void linkUnderlay(const DRW_UnderlayDefinition* value) override {
        if (value != nullptr) {
            ++underlayDefinitionCount;
            underlayDefinition = *value;
        }
    }
    void addMesh(const DRW_Mesh& value) override {
        ++meshCount;
        mesh = value;
    }
    void addSpline(const DRW_Spline* value) override {
        if (value != nullptr) {
            ++splineCount;
            spline = *value;
        }
    }
    void addHatch(const DRW_Hatch* value) override {
        if (value != nullptr) {
            ++hatchCount;
            hatch = *value;
        }
    }
    void addLeader(const DRW_Leader* value) override {
        if (value != nullptr) {
            ++leaderCount;
            leader = *value;
        }
    }
    void addMLine(const DRW_MLine* value) override {
        if (value != nullptr) {
            ++mlineCount;
            mline = *value;
        }
    }
    void addMLineStyle(const DRW_MLineStyle& value) override {
        ++mlineStyleCount;
        mlineStyle = value;
    }
    void addPolyline(const DRW_Polyline& value) override {
        ++polyline3Count;
        polyline3 = value;
    }
    void addInsert(const DRW_Insert& value) override {
        ++insertCount;
        insert = value;
    }
    void add3dFace(const DRW_3Dface& value) override {
        ++faceCount;
        face = value;
    }
    void addDimAlign(const DRW_DimAligned* value) override {
        if (value == nullptr)
            return;
        dx_iface::addDimAlign(value);
        ++dimensionCount;
        dimension = *value;
    }
    void addDimLinear(const DRW_DimLinear* value) override {
        if (value == nullptr)
            return;
        dx_iface::addDimLinear(value);
        if (value->layer == "*ADSK_CONSTRAINTS") {
            ++dynamicConstraintDimensionCount;
            dynamicConstraintDimension = *value;
        } else {
            ++linearDimensionCount;
            linearDimension = *value;
        }
    }
    void addDimRadial(const DRW_DimRadial* value) override {
        if (value == nullptr)
            return;
        dx_iface::addDimRadial(value);
        if (const auto* large =
                dynamic_cast<const DRW_DimLargeRadial*>(value)) {
            ++largeRadialDimensionCount;
            largeRadialDimension = *large;
        } else {
            ++radialDimensionCount;
            radialDimension = *value;
        }
    }
    void addTolerance(const DRW_Tolerance& value) override {
        ++toleranceCount;
        tolerance = value;
    }
    void addDimDiametric(const DRW_DimDiametric* value) override {
        if (value == nullptr)
            return;
        dx_iface::addDimDiametric(value);
        ++diametricDimensionCount;
        diametricDimension = *value;
    }
    void addDimAngular(const DRW_DimAngular* value) override {
        if (value == nullptr)
            return;
        dx_iface::addDimAngular(value);
        ++angularDimensionCount;
        angularDimension = *value;
    }
    void addDimAngular3P(const DRW_DimAngular3p* value) override {
        if (value == nullptr)
            return;
        dx_iface::addDimAngular3P(value);
        ++angular3pDimensionCount;
        angular3pDimension = *value;
    }
    void addDimOrdinate(const DRW_DimOrdinate* value) override {
        if (value == nullptr)
            return;
        dx_iface::addDimOrdinate(value);
        ++ordinateDimensionCount;
        ordinateDimension = *value;
    }
    void addDimArc(const DRW_DimArc* value) override {
        if (value == nullptr)
            return;
        dx_iface::addDimArc(value);
        ++arcDimensionCount;
        arcDimension = *value;
    }
    void addArc(const DRW_Arc& value) override {
        ++arcCount;
        arc = value;
    }
    void addEllipse(const DRW_Ellipse& value) override {
        ++ellipseCount;
        ellipse = value;
    }
    void addPoint(const DRW_Point& value) override {
        dx_iface::addPoint(value);
        ++pointCount;
        point = value;
    }
    void add3DLine(const DRW_3DLine& value) override {
        dx_iface::add3DLine(value);
        ++line3dCount;
        line3d = value;
    }
    void addSolid(const DRW_Solid& value) override {
        dx_iface::addSolid(value);
        ++solidCount;
        solid = value;
    }
    void addTrace(const DRW_Trace& value) override {
        dx_iface::addTrace(value);
        ++traceCount;
        trace = value;
    }
    void addText(const DRW_Text& value) override {
        dx_iface::addText(value);
        if (const auto* arcAligned =
                dynamic_cast<const DRW_ArcAlignedText*>(&value)) {
            ++arcAlignedTextCount;
            arcAlignedText = *arcAligned;
        } else if (const auto* reactive =
                       dynamic_cast<const DRW_RText*>(&value)) {
            ++rtextCount;
            rtext = *reactive;
        } else {
            ++textCount;
            text = value;
        }
    }
    void addLight(const DRW_Light& value) override {
        ++lightCount;
        light = value;
    }
    void addCamera(const DRW_Camera& value) override {
        ++cameraCount;
        camera = value;
    }
    void addView(const DRW_View& value) override {
        ++viewCount;
        view = value;
    }
    void addGeoPositionMarker(const DRW_GeoPositionMarker& value) override {
        ++geoPositionMarkerCount;
        if (value.m_notes == "whole-operation embedded geo marker")
            geoPositionMarkerEmbedded = value;
        else
            geoPositionMarker = value;
    }
    void addSectionObject(const DRW_SectionObject& value) override {
        ++sectionObjectCount;
        sectionObject = value;
    }
    void addMPolygon(const DRW_MPolygon* value) override {
        if (value == nullptr)
            return;
        ++mpolygonCount;
        mpolygon = *value;
    }
    void addViewport(const DRW_Viewport& value) override {
        ++viewportCount;
        viewport = value;
    }
    void addMText(const DRW_MText& value) override {
        dx_iface::addMText(value);
        ++mtextCount;
        mtext = value;
    }
    void addRay(const DRW_Ray& value) override {
        dx_iface::addRay(value);
        ++rayCount;
        ray = value;
    }
    void addXline(const DRW_Xline& value) override {
        dx_iface::addXline(value);
        ++xlineCount;
        xline = value;
    }
    void addHelix(const DRW_Helix* value) override {
        if (value != nullptr) {
            ++helixCount;
            helix = *value;
        }
    }
    void addMaterial(const DRW_Material& value) override {
        ++materialCount;
        material = value;
    }
    void addScale(const DRW_Scale& value) override {
        dx_iface::addScale(value);
        ++scaleCount;
        scale = value;
    }
    void addBackground(const DRW_Background& value) override {
        ++backgroundCount;
        background = value;
    }
    void addPlotSettings(const DRW_PlotSettings* value) override {
        if (value == nullptr)
            return;
        ++plotSettingsCount;
        plotSettings = *value;
    }
    void addWipeoutVariables(const DRW_WipeoutVariables& value) override {
        ++wipeoutVariablesCount;
        wipeoutVariables = value;
    }
    void addSun(const DRW_Sun& value) override {
        ++sunCount;
        sun = value;
    }
    void addSurface(const DRW_Surface* value) override {
        if (value == nullptr)
            return;
        if (value->eType == DRW::LOFTEDSURFACE) {
            ++loftedSurfaceCount;
            loftedSurface = *static_cast<const DRW_LoftedSurface*>(value);
        } else if (value->eType == DRW::SWEPTSURFACE) {
            ++sweptSurfaceCount;
            sweptSurface = *static_cast<const DRW_SweptSurface*>(value);
        } else if (value->eType == DRW::PLANESURFACE) {
            ++planeSurfaceCount;
            planeSurface = *static_cast<const DRW_PlaneSurface*>(value);
        } else if (value->eType == DRW::EXTRUDEDSURFACE) {
            ++extrudedSurfaceCount;
            extrudedSurface = *static_cast<const DRW_ExtrudedSurface*>(value);
        } else if (value->eType == DRW::REVOLVEDSURFACE) {
            ++revolvedSurfaceCount;
            revolvedSurface = *static_cast<const DRW_RevolvedSurface*>(value);
        } else if (value->eType == DRW::NURBSURFACE) {
            ++nurbsSurfaceCount;
            nurbsSurface = *static_cast<const DRW_NurbsSurface*>(value);
        }
    }
    void addAttDef(const DRW_Attdef& value) override {
        dx_iface::addAttDef(value);
        ++attdefCount;
        attdef = value;
    }
    void addDictionary(const DRW_Dictionary& value) override {
        dictionaries.push_back(value);
    }
    void addGroup(const DRW_Group& value) override { groups.push_back(value); }
    void addRawDxfObject(const DRW_RawDxfObject& value) override {
        ++objectCount;
        objects.push_back(value);
    }
    void addDataLink(const DRW_DataLink& value) override {
        dataLinks.push_back(value);
    }
    dx_data storage;
    DRW_Line line;
    DRW_Circle circle;
    DRW_LWPolyline polyline;
    DRW_Image image;
    DRW_Ole2Frame ole2Frame;
    DRW_OleFrame oleFrame;
    DRW_Shape shape;
    DRW_RasterVariables rasterVariables;
    DRW_DictionaryVar dictionaryVar;
    DRW_DbColor dbColor;
    DRW_XRecord typedXRecord;
    DRW_MLeader multiLeader;
    DRW_UnderlayDefinition underlayDefinition;
    DRW_Underlay underlay;
    DRW_Mesh mesh;
    DRW_Spline spline;
    DRW_Hatch hatch;
    DRW_Leader leader;
    DRW_MLine mline;
    DRW_MLineStyle mlineStyle;
    DRW_Polyline polyline3;
    DRW_Insert insert;
    DRW_3Dface face;
    DRW_DimAligned dimension;
    DRW_DimLinear dynamicConstraintDimension;
    DRW_DimLinear linearDimension;
    DRW_DimRadial radialDimension;
    DRW_DimDiametric diametricDimension;
    DRW_DimAngular angularDimension;
    DRW_DimAngular3p angular3pDimension;
    DRW_DimOrdinate ordinateDimension;
    DRW_DimArc arcDimension;
    DRW_DimLargeRadial largeRadialDimension;
    DRW_Tolerance tolerance;
    DRW_RText rtext;
    DRW_ArcAlignedText arcAlignedText;
    DRW_Light light;
    DRW_Camera camera;
    DRW_GeoPositionMarker geoPositionMarker;
    DRW_GeoPositionMarker geoPositionMarkerEmbedded;
    DRW_SectionObject sectionObject;
    DRW_MPolygon mpolygon;
    DRW_Viewport viewport;
    DRW_View view;
    DRW_Arc arc;
    DRW_Ellipse ellipse;
    DRW_Point point;
    DRW_3DLine line3d;
    DRW_Solid solid;
    DRW_Trace trace;
    DRW_Text text;
    DRW_MText mtext;
    DRW_Ray ray;
    DRW_Xline xline;
    DRW_Helix helix;
    DRW_Material material;
    DRW_LoftedSurface loftedSurface;
    DRW_SweptSurface sweptSurface;
    DRW_PlaneSurface planeSurface;
    DRW_ExtrudedSurface extrudedSurface;
    DRW_RevolvedSurface revolvedSurface;
    DRW_NurbsSurface nurbsSurface;
    DRW_Scale scale;
    DRW_Background background;
    DRW_PlotSettings plotSettings;
    DRW_WipeoutVariables wipeoutVariables;
    DRW_Sun sun;
    DRW_Attdef attdef;
    std::vector<DRW_RawDxfObject> objects;
    std::vector<DRW_DataLink> dataLinks;
    std::vector<DRW_Dictionary> dictionaries;
    std::vector<DRW_Group> groups;
    unsigned lineCount = 0u;
    unsigned circleCount = 0u;
    unsigned polylineCount = 0u;
    unsigned imageCount = 0u;
    unsigned ole2FrameCount = 0u;
    unsigned oleFrameCount = 0u;
    unsigned shapeCount = 0u;
    unsigned rasterVariablesCount = 0u;
    unsigned dictionaryVarCount = 0u;
    unsigned dbColorCount = 0u;
    unsigned typedXRecordCount = 0u;
    unsigned multiLeaderCount = 0u;
    unsigned underlayDefinitionCount = 0u;
    unsigned underlayCount = 0u;
    unsigned meshCount = 0u;
    unsigned splineCount = 0u;
    unsigned hatchCount = 0u;
    unsigned leaderCount = 0u;
    unsigned mlineCount = 0u;
    unsigned mlineStyleCount = 0u;
    unsigned polyline3Count = 0u;
    unsigned insertCount = 0u;
    unsigned faceCount = 0u;
    unsigned dimensionCount = 0u;
    unsigned linearDimensionCount = 0u;
    unsigned dynamicConstraintDimensionCount = 0u;
    unsigned radialDimensionCount = 0u;
    unsigned diametricDimensionCount = 0u;
    unsigned angularDimensionCount = 0u;
    unsigned angular3pDimensionCount = 0u;
    unsigned ordinateDimensionCount = 0u;
    unsigned arcDimensionCount = 0u;
    unsigned largeRadialDimensionCount = 0u;
    unsigned toleranceCount = 0u;
    unsigned rtextCount = 0u;
    unsigned arcAlignedTextCount = 0u;
    unsigned lightCount = 0u;
    unsigned cameraCount = 0u;
    unsigned viewCount = 0u;
    unsigned geoPositionMarkerCount = 0u;
    unsigned sectionObjectCount = 0u;
    unsigned mpolygonCount = 0u;
    unsigned viewportCount = 0u;
    unsigned arcCount = 0u;
    unsigned ellipseCount = 0u;
    unsigned pointCount = 0u;
    unsigned line3dCount = 0u;
    unsigned solidCount = 0u;
    unsigned traceCount = 0u;
    unsigned textCount = 0u;
    unsigned mtextCount = 0u;
    unsigned rayCount = 0u;
    unsigned xlineCount = 0u;
    unsigned helixCount = 0u;
    unsigned materialCount = 0u;
    unsigned loftedSurfaceCount = 0u;
    unsigned sweptSurfaceCount = 0u;
    unsigned planeSurfaceCount = 0u;
    unsigned extrudedSurfaceCount = 0u;
    unsigned revolvedSurfaceCount = 0u;
    unsigned nurbsSurfaceCount = 0u;
    unsigned scaleCount = 0u;
    unsigned backgroundCount = 0u;
    unsigned plotSettingsCount = 0u;
    unsigned wipeoutVariablesCount = 0u;
    unsigned sunCount = 0u;
    unsigned attdefCount = 0u;
    unsigned objectCount = 0u;
};

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

bool hasStagedOutput(const std::filesystem::path& target) {
    const std::filesystem::path directory = target.parent_path().empty()
        ? std::filesystem::path(".") : target.parent_path();
    const std::string prefix = target.filename().string() + ".libdxfrw-";
    std::error_code error;
    std::filesystem::directory_iterator entry(directory, error);
    if (error)
        return true;
    const std::filesystem::directory_iterator end;
    while (entry != end) {
        if (entry->path().filename().string().rfind(prefix, 0u) == 0u)
            return true;
        entry.increment(error);
        if (error)
            return true;
    }
    return false;
}

bool clearStagedOutput(const std::filesystem::path& target) {
    const std::filesystem::path directory = target.parent_path().empty()
        ? std::filesystem::path(".") : target.parent_path();
    const std::string prefix = target.filename().string() + ".libdxfrw-";
    std::error_code error;
    std::filesystem::directory_iterator entry(directory, error);
    if (error)
        return false;
    const std::filesystem::directory_iterator end;
    while (entry != end) {
        if (entry->path().filename().string().rfind(prefix, 0u) == 0u) {
            std::filesystem::remove(entry->path(), error);
            if (error)
                return false;
        }
        entry.increment(error);
        if (error)
            return false;
    }
    return true;
}

bool callerRecordsUnchanged(const Export& adapter) {
    const auto unchangedSubtypeDimension = [](const DRW_Dimension& value,
                                              std::uint32_t handle,
                                              double coordinate,
                                              bool checkDefinition = true) {
        const DRW_Coord definition = value.getDefPoint();
        const DRW_Coord text = value.getTextPoint();
        return value.handle == handle
            && (!checkDefinition
                || (definition.x == coordinate
                    && definition.y == coordinate + 1.0
                    && definition.z == coordinate + 2.0))
            && text.x == coordinate + 0.5
            && text.y == coordinate + 1.5
            && value.getText() == "whole-operation subtype dimension"
            && value.getStyle() == "STANDARD";
    };
    const auto lineType = std::find_if(
        adapter.storage.lineTypes.begin(), adapter.storage.lineTypes.end(),
        [](const DRW_LType& value) {
            return value.name == "WHOLE_OPERATION_LTYPE";
        });
    const std::size_t lineTypeCount = static_cast<std::size_t>(std::count_if(
        adapter.storage.lineTypes.begin(), adapter.storage.lineTypes.end(),
        [](const DRW_LType& value) {
            return value.name == "WHOLE_OPERATION_LTYPE";
        }));
    return adapter.line.handle == 0xD900u
        && lineTypeCount == 1u
        && adapter.line.basePoint.x == 1.25
        && adapter.line.basePoint.y == -2.5
        && adapter.line.basePoint.z == 3.75
        && adapter.line.secPoint.x == 4.5
        && adapter.line.secPoint.y == 5.25
        && adapter.line.secPoint.z == -6.0
        && adapter.circle.handle == 0xD902u
        && adapter.circle.basePoint.x == -7.0
        && adapter.circle.basePoint.y == 8.5
        && adapter.circle.basePoint.z == 9.25
        && adapter.circle.radious == 2.75
        && adapter.polyline.handle == kPolylineHandle
        && adapter.polyline.flags == 1
        && adapter.polyline.vertlist.size() == 3u
        && adapter.polyline.vertlist[0]
        && adapter.polyline.vertlist[0]->x == 1.0
        && adapter.polyline.vertlist[0]->y == 1.5
        && adapter.polyline.vertlist[1]
        && adapter.polyline.vertlist[1]->x == 4.0
        && adapter.polyline.vertlist[1]->y == 5.5
        && adapter.polyline.vertlist[1]->bulge == 0.5
        && adapter.polyline.vertlist[2]
        && adapter.polyline.vertlist[2]->x == 8.0
        && adapter.polyline.vertlist[2]->y == 9.5
        && adapter.image.handle == 0xD903u
        && adapter.image.ref == kImageDefHandle
        && adapter.image.basePoint.x == 2.0
        && adapter.image.basePoint.y == 3.0
        && adapter.image.basePoint.z == 4.0
        && adapter.image.sizeu == 640.0
        && adapter.image.sizev == 480.0
        && lineType != adapter.storage.lineTypes.end()
        && lineType->handle == kLineTypeHandle
        && lineType->desc == "whole-operation line type"
        && lineType->path == std::vector<double>{10.0, -5.0}
        && lineType->size == 2
        && lineType->length == 15.0
        && adapter.typedXRecord.handle == kTypedXRecordHandle
        && adapter.typedXRecord.parentHandle == kCustomDictionaryHandle
        && adapter.typedXRecord.m_values.size() == 2u
        && adapter.typedXRecord.m_values[0].code() == 1
        && adapter.typedXRecord.m_values[0].type() == DRW_Variant::STRING
        && adapter.typedXRecord.m_values[0].stringEquals(
            "whole-operation typed XRECORD")
        && adapter.typedXRecord.m_values[1].code() == 90
        && adapter.typedXRecord.m_values[1].type() == DRW_Variant::INTEGER
        && adapter.typedXRecord.m_values[1].i_val() == 42
        && adapter.multiLeader.handle == kMultiLeaderHandle
        && adapter.multiLeader.context.hasTextContents
        && adapter.multiLeader.context.textLabel
            == "whole-operation multi-leader text"
        && adapter.multiLeader.context.contentBasePoint.x == 2.0
        && adapter.multiLeader.context.contentBasePoint.y == 3.0
        && adapter.multiLeader.context.contentBasePoint.z == 4.0
        && adapter.multiLeader.context.textLocation.x == 5.0
        && adapter.multiLeader.context.textLocation.y == 6.0
        && adapter.multiLeader.context.textLocation.z == 7.0
        && adapter.multiLeader.context.textHeight == 1.5
        && adapter.ole2Frame.handle == kOle2FrameHandle
        && adapter.ole2Frame.m_oleVersion == 2u
        && adapter.ole2Frame.m_oleClient == "WHOLE_OPERATION_OLE2"
        && adapter.ole2Frame.m_pt1.x == 1.0
        && adapter.ole2Frame.m_pt1.y == 2.0
        && adapter.ole2Frame.m_pt2.x == 7.0
        && adapter.ole2Frame.m_pt2.y == 9.0
        && adapter.ole2Frame.m_flags == 3u
        && adapter.ole2Frame.m_mode == 1u
        && adapter.ole2Frame.m_lockAspect == 1u
        && adapter.ole2Frame.m_payloadPresent
        && adapter.ole2Frame.m_payloadBytes
            == std::vector<std::uint8_t>{0x00u, 0x1Fu, 0x80u, 0xFFu, 0x35u}
        && adapter.ole2Frame.m_declaredPayloadLength == 5u
        && adapter.oleFrame.handle == kOleFrameHandle
        && adapter.oleFrame.m_flags == 2u
        && adapter.oleFrame.m_payloadPresent
        && adapter.oleFrame.m_payloadBytes
            == std::vector<std::uint8_t>{0x80u, 0x00u, 0xFFu, 0xA5u}
        && adapter.oleFrame.m_declaredPayloadLength == 4u
        && adapter.shape.handle == kShapeHandle
        && adapter.shape.m_styleName == "STANDARD"
        && adapter.shape.m_insertionPoint.x == 8.0
        && adapter.shape.m_insertionPoint.y == 9.0
        && adapter.shape.m_insertionPoint.z == 10.0
        && adapter.shape.m_scale == 1.75
        && adapter.shape.m_rotation == 0.5
        && adapter.shape.m_widthFactor == 0.8
        && adapter.shape.m_oblique == 0.1
        && adapter.shape.m_thickness == 0.25
        && adapter.shape.m_extrusion.z == 1.0
        && adapter.rasterVariables.handle == kRasterVariablesHandle
        && adapter.rasterVariables.parentHandle
            == DRW::DwgNamedObjectsDictionaryHandle
        && adapter.rasterVariables.m_classVersion == 0
        && adapter.rasterVariables.m_imageFrame == 1
        && adapter.rasterVariables.m_imageQuality == 1
        && adapter.rasterVariables.m_units == 4
        && adapter.dictionaryVar.handle == kDictionaryVarHandle
        && adapter.dictionaryVar.parentHandle
            == DRW::DwgNamedObjectsDictionaryHandle
        && adapter.dictionaryVar.m_schema == 1
        && adapter.dictionaryVar.m_value
            == "whole-operation dictionary value"
        && adapter.dbColor.handle == kDbColorHandle
        && adapter.dbColor.parentHandle
            == DRW::DwgNamedObjectsDictionaryHandle
        && adapter.dbColor.rgb == 0x123456
        && adapter.dbColor.hasDxfColorIndex()
        && adapter.dbColor.colorIndex == 7u
        && adapter.dbColor.bookName == "WHOLE_OPERATION_BOOK"
        && adapter.dbColor.name == "ACCENT"
        && adapter.underlayDefinition.handle == kUnderlayDefinitionHandle
        && adapter.underlayDefinition.parentHandle
            == DRW::DwgNamedObjectsDictionaryHandle
        && adapter.underlayDefinition.kind == DRW_UnderlayDefinition::PDF
        && adapter.underlayDefinition.filename
            == "whole-operation-underlay.pdf"
        && adapter.underlayDefinition.sheetName == "sheet-1"
        && adapter.underlay.handle == kUnderlayHandle
        && adapter.underlay.definitionHandle == kUnderlayDefinitionHandle
        && adapter.underlay.clipBoundary.size() == 4u
        && adapter.underlay.inverseClipBoundary.size() == 4u
        && adapter.underlay.flags == 0x12u
        && adapter.mesh.handle == kMeshHandle
        && adapter.mesh.vertices.size() == 3u
        && adapter.mesh.vertices[0].x == 0.0
        && adapter.mesh.vertices[1].x == 2.0
        && adapter.mesh.vertices[2].y == 3.0
        && adapter.mesh.faces == std::vector<std::vector<std::int32_t>>{{0, 1, 2}}
        && adapter.mesh.edges
            == std::vector<std::pair<std::int32_t, std::int32_t>>{{0, 1}, {1, 2}, {2, 0}}
        && adapter.mesh.creases == std::vector<double>{0.5}
        && adapter.mesh.propertyOverrides.size() == 1u
        && adapter.mesh.propertyOverrides.front().subEntityMarker == 7
        && adapter.mesh.propertyOverrides.front().propertyTypes
            == std::vector<std::int32_t>{0, 2}
        && adapter.spline.handle == kSplineHandle
        && adapter.spline.degree == 1
        && adapter.spline.controllist.size() == 2u
        && adapter.spline.controllist[0]
        && adapter.spline.controllist[0]->x == -1.0
        && adapter.spline.controllist[0]->z == 2.0
        && adapter.spline.controllist[1]
        && adapter.spline.controllist[1]->y == 4.0
        && adapter.spline.knotslist == std::vector<double>{0.0, 0.0, 1.0, 1.0}
        && adapter.hatch.handle == kHatchHandle
        && adapter.hatch.name == "ANSI31"
        && adapter.hatch.looplist.size() == 1u
        && adapter.hatch.looplist.front()
        && adapter.hatch.looplist.front()->objlist.size() == 1u
        && dynamic_cast<const DRW_LWPolyline*>(
            adapter.hatch.looplist.front()->objlist.front().get()) != nullptr
        && dynamic_cast<const DRW_LWPolyline*>(
            adapter.hatch.looplist.front()->objlist.front().get())->vertlist.size() == 4u
        && adapter.hatch.patternLines.size() == 1u
        && adapter.hatch.patternLines.front().dashList
            == std::vector<double>{0.5, -0.25, 1.0}
        && adapter.hatch.seedPoints.size() == 1u
        && adapter.hatch.seedPoints.front().x == 0.5
        && adapter.hatch.seedPoints.front().y == 0.5
        && adapter.leader.handle == kLeaderHandle
        && adapter.leader.style == "STANDARD"
        && adapter.leader.vertnum == 3
        && adapter.leader.vertexlist.size() == 3u
        && adapter.leader.vertexlist[0]
        && adapter.leader.vertexlist[0]->x == 1.0
        && adapter.leader.vertexlist[1]
        && adapter.leader.vertexlist[1]->y == 5.0
        && adapter.leader.vertexlist[2]
        && adapter.leader.vertexlist[2]->z == 9.0
        && adapter.leader.offsetblock.x == 0.25
        && adapter.leader.offsetblock.y == 0.5
        && adapter.leader.offsetblock.z == 0.75
        && adapter.mline.handle == kMLineHandle
        && adapter.mline.styleName == "WHOLE_OPERATION_MLINESTYLE"
        && adapter.mline.styleHandle == kMLineStyleHandle
        && adapter.mline.numLines == 1
        && adapter.mline.numVerts == 2
        && adapter.mline.vertlist.size() == 2u
        && adapter.mline.vertlist[0].position.x == 2.0
        && adapter.mline.vertlist[0].vertexDir.x == 1.0
        && adapter.mline.vertlist[0].miterDir.y == 1.0
        && adapter.mline.vertlist[0].segParms
            == std::vector<std::vector<double>>{{0.25}}
        && adapter.mline.vertlist[0].areaFillParms
            == std::vector<std::vector<double>>{{0.0}}
        && adapter.mline.vertlist[1].position.x == 7.0
        && adapter.mline.vertlist[1].segParms
            == std::vector<std::vector<double>>{{-0.5, 0.75}}
        && adapter.mline.vertlist[1].areaFillParms
            == std::vector<std::vector<double>>{{0.125}}
        && adapter.mlineStyle.handle == kMLineStyleHandle
        && adapter.mlineStyle.parentHandle
            == DRW::DwgNamedObjectsDictionaryHandle
        && adapter.mlineStyle.name == "WHOLE_OPERATION_MLINESTYLE"
        && adapter.mlineStyle.elements.size() == 1u
        && adapter.mlineStyle.elements.front().offset == 0.5
        && adapter.mlineStyle.elements.front().linetype.empty()
        && adapter.mlineStyle.elements.front().linetypeIndex == -1
        && adapter.polyline3.handle == kPolyline3Handle
        && adapter.polyline3.flags == 0
        && adapter.polyline3.basePoint.z == 2.5
        && adapter.polyline3.vertlist.size() == 2u
        && adapter.polyline3.vertlist[0]
        && adapter.polyline3.vertlist[0]->handle == 0xD920u
        && adapter.polyline3.vertlist[0]->basePoint.x == 1.0
        && adapter.polyline3.vertlist[0]->basePoint.y == 2.0
        && adapter.polyline3.vertlist[0]->bulge == 0.25
        && adapter.polyline3.vertlist[1]
        && adapter.polyline3.vertlist[1]->handle == 0xD921u
        && adapter.polyline3.vertlist[1]->basePoint.x == 4.0
        && adapter.polyline3.vertlist[1]->basePoint.y == 6.0
        && adapter.insert.handle == kInsertHandle
        && adapter.insert.name == kInsertBlockName
        && adapter.insert.basePoint.x == 10.0
        && adapter.insert.basePoint.y == 20.0
        && adapter.insert.attlist.size() == 2u
        && adapter.insert.attlist.front()
        && adapter.insert.attlist.front()->handle == kInsertAttribHandle
        && adapter.insert.attlist.front()->tag == "WHOLE_OPERATION_TAG"
        && adapter.insert.attlist.front()->text == "whole operation attribute"
        && adapter.insert.attlist.front()->basePoint.x == 11.0
        && adapter.insert.attlist.front()->height == 2.5
        && adapter.insert.attlist.back()
        && adapter.insert.attlist.back()->handle == 0xD932u
        && adapter.insert.attlist.back()->tag == "WHOLE_OPERATION_TAG2"
        && adapter.insert.attlist.back()->text
            == "second whole operation attribute"
        && adapter.insert.attlist.back()->basePoint.x == 12.0
        && adapter.insert.attlist.back()->height == 1.25
        && adapter.face.handle == kFaceHandle
        && adapter.face.basePoint.x == 0.0
        && adapter.face.basePoint.y == 0.0
        && adapter.face.basePoint.z == 1.0
        && adapter.face.secPoint.x == 3.0
        && adapter.face.secPoint.z == 2.0
        && adapter.face.thirdPoint.y == 4.0
        && adapter.face.thirdPoint.z == 3.0
        && adapter.face.fourPoint.y == 4.0
        && adapter.face.fourPoint.z == 4.0
        && adapter.face.invisibleflag
            == (DRW_3Dface::FirstEdge | DRW_3Dface::ThirdEdge)
        && adapter.dimension.handle == 0xD960u
        && adapter.dimension.getDefPoint().x == -3.0
        && adapter.dimension.getDefPoint().y == 4.0
        && adapter.dimension.getDefPoint().z == 5.0
        && adapter.dimension.getTextPoint().x == 1.5
        && adapter.dimension.getTextPoint().y == 2.5
        && adapter.dimension.getText() == "whole-operation dimension"
        && adapter.dimension.getStyle() == "STANDARD"
        && adapter.dynamicConstraintDimension.handle
            == kDynamicConstraintDimensionHandle
        && adapter.dynamicConstraintDimension.layer == "*ADSK_CONSTRAINTS"
        && adapter.dynamicConstraintDimension.getDefPoint().x == 15.0
        && adapter.dynamicConstraintDimension.getDefPoint().y == 16.0
        && adapter.dynamicConstraintDimension.getDefPoint().z == 17.0
        && adapter.dynamicConstraintDimension.getTextPoint().x == 15.5
        && adapter.dynamicConstraintDimension.getTextPoint().y == 16.5
        && adapter.dynamicConstraintDimension.getText()
            == "whole-operation dynamic constraint"
        && unchangedSubtypeDimension(adapter.linearDimension, 0xD961u, 10.0)
        && adapter.linearDimension.getDef1Point().z == 12.0
        && adapter.linearDimension.getDef2Point().x == 13.0
        && unchangedSubtypeDimension(adapter.radialDimension, 0xD962u, 20.0)
        && adapter.radialDimension.getDiameterPoint().x == 22.0
        && adapter.radialDimension.getLeaderLength() == 3.0
        && unchangedSubtypeDimension(adapter.diametricDimension, 0xD963u, 30.0)
        && adapter.diametricDimension.getDiameter1Point().x == 32.0
        && adapter.diametricDimension.getLeaderLength() == 4.0
        && unchangedSubtypeDimension(adapter.angularDimension, 0xD964u, 40.0, false)
        && adapter.angularDimension.getDefPoint().x == 41.0
        && adapter.angularDimension.getDefPoint().y == 41.0
        && adapter.angularDimension.getDefPoint().z == 0.0
        && adapter.angularDimension.getFirstLine1().x == 40.0
        && adapter.angularDimension.getSecondLine2().y == 41.0
        && adapter.angularDimension.getDimPoint().x == 40.5
        && unchangedSubtypeDimension(adapter.angular3pDimension, 0xD965u, 50.0, false)
        && adapter.angular3pDimension.getDefPoint().x == 50.5
        && adapter.angular3pDimension.getDefPoint().y == 50.5
        && adapter.angular3pDimension.getDefPoint().z == 0.0
        && adapter.angular3pDimension.getFirstLine().x == 50.0
        && adapter.angular3pDimension.getVertexPoint().y == 51.0
        && adapter.angular3pDimension.getDimPoint().x == 50.5
        && unchangedSubtypeDimension(adapter.ordinateDimension, 0xD966u, 60.0)
        && adapter.ordinateDimension.getFirstLine().x == 61.0
        && adapter.ordinateDimension.getSecondLine().z == 66.0
        && adapter.arcDimension.handle == 0xD967u
        && adapter.arcDimension.getArcDefPoint().x == 70.0
        && adapter.arcDimension.getArcDefPoint().y == 71.0
        && adapter.arcDimension.getArcDefPoint().z == 72.0
        && adapter.arcDimension.getTextPoint().x == 70.5
        && adapter.arcDimension.getTextPoint().y == 71.5
        && adapter.arcDimension.getText() == "whole-operation arc dimension"
        && adapter.arcDimension.getStyle() == "STANDARD"
        && adapter.arcDimension.getExtLine1().x == 71.0
        && adapter.arcDimension.getExtLine2().y == 72.0
        && adapter.arcDimension.getArcCenter().x == 70.0
        && adapter.arcDimension.getLeaderPt1().x == 72.0
        && adapter.arcDimension.leaderPt2.x == 73.0
        && adapter.arcDimension.arcStartAngle == 0.25
        && adapter.arcDimension.arcEndAngle == 1.75
        && adapter.arcDimension.isPartial
        && adapter.arcDimension.hasLeader
        && adapter.largeRadialDimension.handle == 0xD968u
        && adapter.largeRadialDimension.getCenterPoint().x == 80.0
        && adapter.largeRadialDimension.getCenterPoint().y == 81.0
        && adapter.largeRadialDimension.getCenterPoint().z == 82.0
        && adapter.largeRadialDimension.getTextPoint().x == 80.5
        && adapter.largeRadialDimension.getChordPoint().x == 83.0
        && adapter.largeRadialDimension.overrideCenterPoint.y == 81.0
        && adapter.largeRadialDimension.jogPoint.x == 84.0
        && adapter.largeRadialDimension.jogAngle == 0.375
        && adapter.tolerance.handle == 0xD969u
        && adapter.tolerance.dimStyleName == "STANDARD"
        && adapter.tolerance.insertionPoint.x == 90.0
        && adapter.tolerance.insertionPoint.y == 91.0
        && adapter.tolerance.insertionPoint.z == 92.0
        && adapter.tolerance.text == "whole-operation tolerance"
        && adapter.tolerance.extPoint.z == 1.0
        && adapter.tolerance.xAxisDirectionVector.x == 1.0
        && adapter.rtext.handle == 0xD96Au
        && adapter.rtext.basePoint.x == 95.0
        && adapter.rtext.basePoint.y == 96.0
        && adapter.rtext.text == "whole-operation RTEXT"
        && adapter.rtext.height == 2.5
        && adapter.rtext.angle == 30.0
        && adapter.rtext.style == "STANDARD"
        && adapter.rtext.m_rTextFlags == 1
        && adapter.rtext.extPoint.z == 1.0
        && adapter.arcAlignedText.handle == 0xD96Bu
        && adapter.arcAlignedText.text == "whole-operation arc-aligned text"
        && adapter.arcAlignedText.style == "STANDARD"
        && adapter.arcAlignedText.m_fontName == "SIMPLEX.SHX"
        && adapter.arcAlignedText.m_bigFontName == "BIGFONT.SHX"
        && adapter.arcAlignedText.m_center.x == 100.0
        && adapter.arcAlignedText.m_center.y == 101.0
        && adapter.arcAlignedText.m_radius == 12.0
        && adapter.arcAlignedText.m_startAngle == 0.25
        && adapter.arcAlignedText.m_endAngle == 1.25
        && adapter.arcAlignedText.m_xScale == "1.2"
        && adapter.arcAlignedText.m_textSize == "2.25"
        && adapter.arcAlignedText.m_charSpacing == "0.75"
        && adapter.arcAlignedText.m_offsetFromArc == "0.5"
        && adapter.arcAlignedText.m_rightOffset == "0.25"
        && adapter.arcAlignedText.m_leftOffset == "0.125"
        && adapter.arcAlignedText.m_rawColor == 0x010203
        && adapter.arcAlignedText.m_characterSet == 1
        && adapter.arcAlignedText.m_pitchAndFamily == 2
        && adapter.arcAlignedText.m_isShx == 1
        && adapter.arcAlignedText.m_isBold == 1
        && adapter.arcAlignedText.m_isItalic == 1
        && adapter.arcAlignedText.m_isUnderlined == 1
        && adapter.arcAlignedText.m_alignment == 2
        && adapter.arcAlignedText.m_isReverse == 1
        && adapter.arcAlignedText.m_wizardFlag == 1
        && adapter.arcAlignedText.m_textPosition == 1
        && adapter.arcAlignedText.m_textDirection == 1
        && adapter.light.handle == 0xD96Cu
        && adapter.light.m_classVersion == 1u
        && adapter.light.m_name == "WHOLE_OPERATION_LIGHT"
        && adapter.light.m_type == 2u
        && adapter.light.m_status
        && adapter.light.m_color == 3u
        && adapter.light.m_plotGlyph
        && adapter.light.m_intensity == 2.5
        && adapter.light.m_position.x == 110.0
        && adapter.light.m_position.y == 111.0
        && adapter.light.m_position.z == 112.0
        && adapter.light.m_target.x == 113.0
        && adapter.light.m_target.y == 114.0
        && adapter.light.m_target.z == 115.0
        && adapter.light.m_attenuationType == 1u
        && adapter.light.m_useAttenuationLimits
        && adapter.light.m_attenuationStartLimit == 0.5
        && adapter.light.m_attenuationEndLimit == 12.5
        && adapter.light.m_hotspotAngle == 0.25
        && adapter.light.m_falloffAngle == 0.75
        && adapter.light.m_castShadows
        && adapter.light.m_shadowType == 1u
        && adapter.light.m_shadowMapSize == 512u
        && adapter.light.m_shadowMapSoftness == 2u
        && adapter.camera.handle == 0xD96Du
        && adapter.camera.m_viewHandle == 0xD96Eu
        && adapter.storage.views.size() == 1u
        && adapter.storage.views.front().handle == 0xD96Eu
        && adapter.storage.views.front().name == "WHOLE_OPERATION_CAMERA_VIEW"
        && adapter.storage.views.front().size.x == 640.0
        && adapter.storage.views.front().targetPoint.z == 30.0
        && adapter.geoPositionMarker.handle == 0xD96Fu
        && adapter.geoPositionMarker.m_classVersion == 1u
        && adapter.geoPositionMarker.m_position.x == 120.0
        && adapter.geoPositionMarker.m_position.y == 121.0
        && adapter.geoPositionMarker.m_position.z == 122.0
        && adapter.geoPositionMarker.m_radius == 6.5
        && adapter.geoPositionMarker.m_notes == "whole-operation geo marker"
        && adapter.geoPositionMarker.m_landingGap == 0.75
        && adapter.geoPositionMarker.m_mtextVisible
        && adapter.geoPositionMarker.m_textAlignment == 2u
        && !adapter.geoPositionMarker.m_enableFrameText
        && adapter.geoPositionMarkerEmbedded.handle == 0xD970u
        && adapter.geoPositionMarkerEmbedded.m_classVersion == 1u
        && adapter.geoPositionMarkerEmbedded.m_position.x == 123.0
        && adapter.geoPositionMarkerEmbedded.m_position.y == 124.0
        && adapter.geoPositionMarkerEmbedded.m_position.z == 125.0
        && adapter.geoPositionMarkerEmbedded.m_radius == 8.5
        && adapter.geoPositionMarkerEmbedded.m_notes == "whole-operation embedded geo marker"
        && adapter.geoPositionMarkerEmbedded.m_landingGap == 1.25
        && adapter.geoPositionMarkerEmbedded.m_mtextVisible
        && adapter.geoPositionMarkerEmbedded.m_textAlignment == 1u
        && adapter.geoPositionMarkerEmbedded.m_enableFrameText
        && adapter.geoPositionMarkerEmbedded.mtext
        && adapter.geoPositionMarkerEmbedded.mtext->text
            == "whole-operation embedded frame"
        && adapter.geoPositionMarkerEmbedded.mtext->basePoint.x == 123.0
        && adapter.geoPositionMarkerEmbedded.mtext->basePoint.y == 124.0
        && adapter.geoPositionMarkerEmbedded.mtext->basePoint.z == 125.0
        && adapter.geoPositionMarkerEmbedded.mtext->height == 1.75
        && adapter.geoPositionMarkerEmbedded.mtext->widthscale == 5.0
        && adapter.geoPositionMarkerEmbedded.mtext->style == "STANDARD"
        && adapter.sectionObject.handle == 0xD971u
        && adapter.sectionObject.m_state == 3u
        && adapter.sectionObject.m_flags == 5u
        && adapter.sectionObject.m_name == "WHOLE_OPERATION_SECTION"
        && adapter.sectionObject.m_vertDir.z == 1.0
        && adapter.sectionObject.m_topHeight == 12.0
        && adapter.sectionObject.m_bottomHeight == -4.0
        && adapter.sectionObject.m_indicatorAlpha == 128u
        && adapter.sectionObject.m_indicatorColor == 6u
        && adapter.sectionObject.m_verts.size() == 2u
        && adapter.sectionObject.m_verts[1].z == 6.0
        && adapter.sectionObject.m_blVerts.size() == 1u
        && adapter.sectionObject.m_blVerts[0].x == -1.0
        && adapter.mpolygon.handle == 0xD972u
        && adapter.mpolygon.dxfVersion == 1
        && adapter.mpolygon.name == "SOLID"
        && adapter.mpolygon.solid == 1
        && adapter.mpolygon.basePoint.z == 2.0
        && adapter.mpolygon.fillColorAci == 5
        && adapter.mpolygon.annotatedBoundary == 1
        && adapter.mpolygon.xDirX == 1.0
        && adapter.mpolygon.looplist.size() == 1u
        && adapter.mpolygon.looplist[0]
        && adapter.mpolygon.looplist[0]->objlist.size() == 1u
        && adapter.mpolygon.looplist[0]->objlist[0]
        && dynamic_cast<const DRW_LWPolyline*>(
            adapter.mpolygon.looplist[0]->objlist[0].get()) != nullptr
        && dynamic_cast<const DRW_LWPolyline*>(
            adapter.mpolygon.looplist[0]->objlist[0].get())->vertlist.size() == 4u
        && adapter.viewport.handle == 0xD973u
        && adapter.viewport.pswidth == 210.0
        && adapter.viewport.psheight == 148.0
        && adapter.viewport.vpstatus == 1
        && adapter.viewport.vpID == 2
        && adapter.viewport.centerPX == 105.0
        && adapter.viewport.centerPY == 74.0
        && adapter.viewport.viewTarget.x == 10.0
        && adapter.viewport.viewTarget.y == 20.0
        && adapter.viewport.viewTarget.z == 30.0
        && adapter.viewport.viewHeight == 100.0
        && adapter.viewport.snapSpPX == 5.0
        && adapter.viewport.snapSpPY == 6.0
        && adapter.viewport.gridSpX == 10.0
        && adapter.viewport.gridSpY == 12.0
        && adapter.viewport.styleSheet == "WHOLE_OPERATION_STYLE"
        && adapter.arc.handle == kArcHandle
        && adapter.arc.basePoint.x == -3.0
        && adapter.arc.basePoint.y == 2.0
        && adapter.arc.basePoint.z == 5.0
        && adapter.arc.radious == 6.5
        && adapter.arc.staangle == 0.25
        && adapter.arc.endangle == 2.75
        && adapter.arc.thickness == 1.25
        && adapter.ellipse.handle == kEllipseHandle
        && adapter.ellipse.basePoint.x == 7.0
        && adapter.ellipse.basePoint.y == -4.0
        && adapter.ellipse.basePoint.z == 2.0
        && adapter.ellipse.secPoint.x == 4.0
        && adapter.ellipse.secPoint.y == 1.0
        && adapter.ellipse.secPoint.z == 0.0
        && adapter.ellipse.ratio == 0.5
        && adapter.ellipse.staparam == 0.2
        && adapter.ellipse.endparam == 2.4
        && adapter.point.handle == kPointHandle
        && adapter.point.basePoint.x == -2.0
        && adapter.point.basePoint.y == 3.0
        && adapter.point.basePoint.z == 4.0
        && adapter.point.xAxisAngle == 0.75
        && adapter.line3d.handle == k3DLineHandle
        && adapter.line3d.basePoint.x == 1.0
        && adapter.line3d.basePoint.z == 3.0
        && adapter.line3d.secPoint.x == 4.0
        && adapter.line3d.secPoint.z == 6.0
        && adapter.solid.handle == kSolidHandle
        && adapter.solid.basePoint.z == 1.5
        && adapter.solid.secPoint.x == 3.0
        && adapter.solid.thirdPoint.y == 2.0
        && adapter.solid.fourPoint.y == 2.0
        && adapter.trace.handle == kTraceHandle
        && adapter.trace.basePoint.z == 2.0
        && adapter.trace.secPoint.x == 3.0
        && adapter.trace.thirdPoint.y == 2.0
        && adapter.trace.fourPoint.y == 2.0
        && adapter.text.handle == kTextHandle
        && adapter.text.basePoint.x == 12.0
        && adapter.text.basePoint.y == -3.0
        && adapter.text.basePoint.z == 1.0
        && adapter.text.text == "whole-operation TEXT"
        && adapter.text.height == 2.0
        && adapter.text.angle == 15.0
        && adapter.text.widthscale == 1.2
        && adapter.text.style == "STANDARD"
        && adapter.mtext.handle == kMTextHandle
        && adapter.mtext.basePoint.x == 4.0
        && adapter.mtext.basePoint.y == 5.0
        && adapter.mtext.basePoint.z == 6.0
        && adapter.mtext.text == "whole-operation MTEXT"
        && adapter.mtext.height == 1.5
        && adapter.mtext.widthscale == 12.0
        && adapter.mtext.style == "STANDARD"
        && adapter.mtext.m_definedHeight == 2.25
        && adapter.mtext.m_definedHeightPresent
        && adapter.ray.handle == kRayHandle
        && adapter.ray.basePoint.x == -5.0
        && adapter.ray.basePoint.y == 7.0
        && adapter.ray.basePoint.z == 2.0
        && adapter.ray.secPoint.x == 1.0
        && adapter.ray.secPoint.y == 2.0
        && adapter.ray.secPoint.z == 3.0
        && adapter.xline.handle == kXlineHandle
        && adapter.xline.basePoint.x == 8.0
        && adapter.xline.basePoint.y == -6.0
        && adapter.xline.basePoint.z == 4.0
        && adapter.xline.secPoint.x == -2.0
        && adapter.xline.secPoint.y == 1.0
        && adapter.xline.secPoint.z == 0.5
        && adapter.helix.handle == kHelixHandle
        && adapter.helix.m_majorVersion == 1
        && adapter.helix.m_maintVersion == 2
        && adapter.helix.degree == 2
        && adapter.helix.flags == 8
        && adapter.helix.knotslist == std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}
        && adapter.helix.controllist.size() == 3u
        && adapter.helix.controllist.front()
        && adapter.helix.controllist.front()->x == 1.0
        && adapter.helix.controllist.back()
        && adapter.helix.controllist.back()->z == 4.0
        && adapter.helix.axisBasePt.x == 2.0
        && adapter.helix.axisBasePt.y == 3.0
        && adapter.helix.axisBasePt.z == 4.0
        && adapter.helix.startPt.x == 3.0
        && adapter.helix.startPt.y == 3.0
        && adapter.helix.startPt.z == 4.0
        && adapter.helix.axisVector.z == 1.0
        && adapter.helix.radius == 1.0
        && adapter.helix.turns == 2.5
        && adapter.helix.turnHeight == 3.0
        && adapter.helix.handedness
        && adapter.helix.constraintType == 1
        && adapter.material.handle == kMaterialHandle
        && adapter.material.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
        && adapter.material.m_name == "WHOLE_OPERATION_MATERIAL"
        && adapter.material.m_description == "material allocation retry"
        && adapter.loftedSurface.handle == kLoftedSurfaceHandle
        && adapter.loftedSurface.modelerFormatVersion == 1
        && adapter.loftedSurface.uIsolines == 10
        && adapter.loftedSurface.vIsolines == 11
        && adapter.loftedSurface.planeNormalLoftingType == 1
        && adapter.loftedSurface.startDraftAngle == 0.2
        && adapter.loftedSurface.endDraftAngle == 0.3
        && adapter.loftedSurface.startDraftMagnitude == 0.4
        && adapter.loftedSurface.endDraftMagnitude == 0.5
        && adapter.loftedSurface.arcLengthParameterization
        && adapter.loftedSurface.closedSurfaces
        && adapter.loftedSurface.solid
        && adapter.loftedSurface.ruledSurface
        && adapter.loftedSurface.virtualGuide
        && adapter.loftedSurface.loftEntityTransform[3] == 0.25
        && adapter.loftedSurface.loftEntityTransform[7] == 0.5
        && adapter.loftedSurface.loftEntityTransform[11] == 0.75
        && adapter.loftedSurface.dxfReferenceData.size() == 2u
        && adapter.loftedSurface.dxfReferenceData[0].code() == 90
        && adapter.loftedSurface.dxfReferenceData[0].i_val() == 2
        && adapter.loftedSurface.dxfReferenceData[1].code() == 310
        && adapter.loftedSurface.dxfReferenceData[1].binary()
        && *adapter.loftedSurface.dxfReferenceData[1].binary()
            == std::vector<std::uint8_t>{0x12u, 0x34u}
        && adapter.sweptSurface.handle == kSweptSurfaceHandle
        && adapter.sweptSurface.modelerFormatVersion == 1
        && adapter.sweptSurface.uIsolines == 12
        && adapter.sweptSurface.vIsolines == 13
        && adapter.sweptSurface.sweepEntityId == 101u
        && adapter.sweptSurface.sweepData
            == std::vector<std::uint8_t>{0x21u, 0x22u}
        && adapter.sweptSurface.pathEntityId == 202u
        && adapter.sweptSurface.pathData
            == std::vector<std::uint8_t>{0x31u, 0x32u, 0x33u}
        && adapter.sweptSurface.referenceVector.x == 1.0
        && adapter.sweptSurface.referenceVector.y == 2.0
        && adapter.sweptSurface.referenceVector.z == 3.0
        && adapter.sweptSurface.sweepAlignmentFlags == 2
        && adapter.sweptSurface.pathFlags == 3
        && adapter.sweptSurface.draftAngle == 0.125
        && adapter.sweptSurface.draftStartDistance == 0.25
        && adapter.sweptSurface.draftEndDistance == 0.5
        && adapter.sweptSurface.twistAngle == 0.75
        && adapter.sweptSurface.scaleFactor == 1.25
        && adapter.sweptSurface.alignAngle == 0.875
        && adapter.sweptSurface.sweepEntityTransform[3] == 0.5
        && adapter.sweptSurface.pathEntityTransform[7] == 0.75
        && adapter.sweptSurface.sweepEntityTransformed[11] == 1.0
        && adapter.sweptSurface.pathEntityTransformed[15] == 1.25
        && adapter.sweptSurface.solid
        && adapter.sweptSurface.alignStart
        && adapter.sweptSurface.bank
        && adapter.sweptSurface.basePointSet
        && adapter.sweptSurface.sweepEntityTransformComputed
        && adapter.sweptSurface.pathEntityTransformComputed
        && adapter.planeSurface.handle == kPlaneSurfaceHandle
        && adapter.planeSurface.modelerFormatVersion == 1
        && adapter.planeSurface.uIsolines == 2
        && adapter.planeSurface.vIsolines == 3
        && adapter.extrudedSurface.handle == kExtrudedSurfaceHandle
        && adapter.extrudedSurface.classId == 2u
        && adapter.extrudedSurface.dxfBinaryData
            == std::vector<std::uint8_t>{0x21u, 0x43u}
        && adapter.extrudedSurface.sweepVector.z == 1.0
        && adapter.extrudedSurface.referenceVector.x == 1.0
        && adapter.extrudedSurface.sweepAlignmentFlags == 3
        && adapter.extrudedSurface.pathFlags == 7
        && adapter.extrudedSurface.draftAngle == 0.1
        && adapter.extrudedSurface.draftStartDistance == 0.25
        && adapter.extrudedSurface.draftEndDistance == 0.5
        && adapter.extrudedSurface.twistAngle == 0.75
        && adapter.extrudedSurface.scaleFactor == 1.25
        && adapter.extrudedSurface.alignAngle == 0.5
        && adapter.extrudedSurface.solid
        && adapter.extrudedSurface.alignStart
        && adapter.extrudedSurface.bank
        && adapter.extrudedSurface.basePointSet
        && adapter.extrudedSurface.sweepEntityTransformComputed
        && adapter.extrudedSurface.pathEntityTransformComputed
        && adapter.extrudedSurface.extrudedTransform[3] == 0.25
        && adapter.extrudedSurface.sweepEntityTransform[3] == 1.25
        && adapter.extrudedSurface.pathEntityTransform[7] == 1.5
        && adapter.revolvedSurface.handle == kRevolvedSurfaceHandle
        && adapter.revolvedSurface.id == 2u
        && adapter.revolvedSurface.dxfBinaryData
            == std::vector<std::uint8_t>{0x65u, 0x87u}
        && adapter.revolvedSurface.axisPoint.x == 1.0
        && adapter.revolvedSurface.axisVector.y == 1.0
        && adapter.revolvedSurface.revolveAngle == 1.5
        && adapter.revolvedSurface.startAngle == 0.25
        && adapter.revolvedSurface.draftAngle == 0.5
        && adapter.revolvedSurface.draftStartDistance == 0.75
        && adapter.revolvedSurface.draftEndDistance == 1.0
        && adapter.revolvedSurface.twistAngle == 1.25
        && adapter.revolvedSurface.solid
        && adapter.revolvedSurface.closeToAxis
        && adapter.revolvedSurface.transform[3] == 0.25
        && adapter.revolvedSurface.transform[7] == 0.5
        && adapter.revolvedSurface.transform[11] == 0.75
        && adapter.nurbsSurface.handle == kNurbsSurfaceHandle
        && adapter.nurbsSurface.short170 == 16u
        && adapter.nurbsSurface.cvHullDisplay
        && adapter.nurbsSurface.uvec1.x == 1.0
        && adapter.nurbsSurface.vvec1.y == 1.0
        && adapter.nurbsSurface.uvec2.x == 2.0
        && adapter.nurbsSurface.vvec2.y == 2.0
        && adapter.scale.handle == kScaleHandle
        && adapter.scale.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
        && adapter.scale.name == "WHOLE_OPERATION_SCALE"
        && adapter.scale.flag == 0
        && adapter.scale.paperUnits == 2.0
        && adapter.scale.drawingUnits == 5.0
        && !adapter.scale.isUnitScale
        && adapter.background.handle == kBackgroundHandle
        && adapter.background.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
        && adapter.background.m_kind == DRW_Background::Solid
        && adapter.background.m_classVersion == 1
        && adapter.background.m_solidColor == 0x010203
        && adapter.plotSettings.handle == kPlotSettingsHandle
        && adapter.plotSettings.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
        && adapter.plotSettings.pageSetupName == "WHOLE_OPERATION_PAGE_SETUP"
        && adapter.plotSettings.printerConfig == "WHOLE_OPERATION_PRINTER"
        && adapter.plotSettings.paperSize == "ISO_A4_(210.00_x_297.00_MM)"
        && adapter.plotSettings.marginLeft == 4.0
        && adapter.plotSettings.marginBottom == 5.0
        && adapter.plotSettings.marginRight == 6.0
        && adapter.plotSettings.marginTop == 7.0
        && adapter.plotSettings.paperWidth == 210.0
        && adapter.plotSettings.paperHeight == 297.0
        && adapter.plotSettings.plotOriginX == 1.0
        && adapter.plotSettings.plotOriginY == 2.0
        && adapter.plotSettings.paperUnits == 1
        && adapter.plotSettings.plotRotation == 2
        && adapter.plotSettings.plotType == 1
        && adapter.plotSettings.windowMinX == 10.0
        && adapter.plotSettings.windowMinY == 11.0
        && adapter.plotSettings.windowMaxX == 20.0
        && adapter.plotSettings.windowMaxY == 21.0
        && adapter.plotSettings.realWorldUnits == 1.0
        && adapter.plotSettings.drawingUnits == 2.0
        && adapter.plotSettings.currentStyleSheet == "WHOLE_OPERATION_STYLE"
        && adapter.plotSettings.scaleType == 3
        && adapter.plotSettings.scaleFactor == 0.5
        && adapter.plotSettings.paperImageOriginX == 0.25
        && adapter.plotSettings.paperImageOriginY == 0.75
        && adapter.plotSettings.shadePlotMode == 1
        && adapter.plotSettings.shadePlotResLevel == 2
        && adapter.plotSettings.shadePlotCustomDPI == 150
        && adapter.wipeoutVariables.handle == kWipeoutVariablesHandle
        && adapter.wipeoutVariables.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
        && adapter.wipeoutVariables.m_classVersion == 0
        && adapter.wipeoutVariables.m_displayFrame == 1
        && adapter.sun.handle == kSunHandle
        && adapter.sun.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
        && adapter.sun.m_classVersion == 0
        && adapter.sun.m_isOn
        && adapter.sun.m_color == 7
        && adapter.sun.m_intensity == 0.625
        && adapter.sun.m_hasShadow
        && adapter.sun.m_julianDay == 2460000
        && adapter.sun.m_milliseconds == 43210000
        && adapter.sun.m_isDaylightSavings
        && adapter.sun.m_shadowType == 2
        && adapter.sun.m_shadowMapSize == 1024
        && adapter.sun.m_shadowSoftness == 3
        && adapter.storage.blocks.size() == 1u
        && adapter.storage.blocks.front()->handle == kInsertBlockHandle
        && adapter.storage.blocks.front()->name == kInsertBlockName
        && adapter.storage.blocks.front()->ent.size() == 2u
        && adapter.storage.blocks.front()->ent.front()->handle
            == kInsertBlockPointHandle
        && adapter.storage.blocks.front()->ent.back()->handle == kAttdefHandle
        && dynamic_cast<const DRW_Attdef*>(
            adapter.storage.blocks.front()->ent.back()) != nullptr
        && dynamic_cast<const DRW_Attdef*>(
            adapter.storage.blocks.front()->ent.back())->tag == "BLOCK_TAG"
        && dynamic_cast<const DRW_Attdef*>(
            adapter.storage.blocks.front()->ent.back())->prompt
            == "Enter block value"
        && adapter.object.name == "XRECORD"
        && adapter.object.handle == 0xD901u
        && adapter.object.parentHandle == kCustomDictionaryHandle
        && adapter.object.groups.size() == 4u
        && adapter.object.groups[0].code() == 5
        && adapter.object.groups[0].c_str() != nullptr
        && std::string(adapter.object.groups[0].c_str()) == "D901"
        && adapter.object.groups[1].code() == 330
        && adapter.object.groups[1].c_str() != nullptr
        && std::string(adapter.object.groups[1].c_str()) == "D914"
        && adapter.object.groups[3].code() == 1
        && adapter.object.groups[3].c_str() != nullptr
        && std::string(adapter.object.groups[3].c_str())
            == "whole operation payload"
        && adapter.mainDataLink.handle == kMainDataLinkHandle
        && adapter.mainDataLink.parentHandle == kCustomDictionaryHandle
        && adapter.mainDataLink.m_customDataCount == 2
        && adapter.mainDataLink.m_customData.size() == 2u
        && adapter.mainDataLink.m_customData[0].m_targetHandle
            == kTargetAlphaHandle
        && adapter.mainDataLink.m_customData[0].m_text == "sheet alpha"
        && adapter.mainDataLink.m_customData[1].m_targetHandle
            == kTargetBetaHandle
        && adapter.mainDataLink.m_customData[1].m_text == "sheet beta"
        && adapter.targetAlpha.handle == kTargetAlphaHandle
        && adapter.targetAlpha.parentHandle == kCustomDictionaryHandle
        && adapter.targetBeta.handle == kTargetBetaHandle
        && adapter.targetBeta.parentHandle == kCustomDictionaryHandle;
}

bool matchesReadback(const std::filesystem::path& path) {
    dxfRW reader(path.string().c_str());
    Readback adapter;
    const bool read = reader.read(&adapter, false);
    const auto rawXrecord = std::find_if(
        adapter.objects.begin(), adapter.objects.end(),
        [](const DRW_RawDxfObject& value) {
            return value.name == "XRECORD"
                && std::any_of(
                    value.groups.begin(), value.groups.end(),
                    [](const DRW_Variant& group) {
                        return group.stringEquals("whole operation payload");
                    });
        });
    const auto mainDataLink = std::find_if(
        adapter.dataLinks.begin(), adapter.dataLinks.end(),
        [](const DRW_DataLink& value) {
            return value.handle == kMainDataLinkHandle;
        });
    const auto targetAlpha = std::find_if(
        adapter.dataLinks.begin(), adapter.dataLinks.end(),
        [](const DRW_DataLink& value) {
            return value.handle == kTargetAlphaHandle;
        });
    const auto targetBeta = std::find_if(
        adapter.dataLinks.begin(), adapter.dataLinks.end(),
        [](const DRW_DataLink& value) {
            return value.handle == kTargetBetaHandle;
        });
    const auto lineType = std::find_if(
        adapter.storage.lineTypes.begin(), adapter.storage.lineTypes.end(),
        [](const DRW_LType& value) {
            return value.name == "WHOLE_OPERATION_LTYPE";
        });
    const std::size_t lineTypeCount = static_cast<std::size_t>(std::count_if(
        adapter.storage.lineTypes.begin(), adapter.storage.lineTypes.end(),
        [](const DRW_LType& value) {
            return value.name == "WHOLE_OPERATION_LTYPE";
        }));
    const auto insertBlock = std::find_if(
        adapter.storage.blocks.begin(), adapter.storage.blocks.end(),
        [](const dx_ifaceBlock* value) {
            return value != nullptr && value->name == kInsertBlockName;
        });
    const DRW_ImageDef* imageDefinition = adapter.storage.findImageDefinition(
        adapter.storage.importGeneration, adapter.image.ref);
    const auto commonDimensionMatches = [](const DRW_Dimension& value,
                                           double coordinate,
                                           bool checkDefinition = true) {
        const DRW_Coord definition = value.getDefPoint();
        const DRW_Coord text = value.getTextPoint();
        return value.handle != 0u
            && (!checkDefinition
                || (definition.x == coordinate
                    && definition.y == coordinate + 1.0
                    && definition.z == coordinate + 2.0))
            && text.x == coordinate + 0.5
            && text.y == coordinate + 1.5
            && value.getText() == "whole-operation subtype dimension"
            && value.getStyle() == "STANDARD";
    };
    if (!read || adapter.lineCount != 1u || adapter.circleCount != 1u
        || adapter.polylineCount != 1u
        || adapter.imageCount != 1u
        || adapter.meshCount != 1u
        || adapter.splineCount != 1u
        || adapter.hatchCount != 1u
        || adapter.leaderCount != 1u
        || adapter.mlineCount != 1u
        || adapter.mlineStyleCount != 1u
        || adapter.dimensionCount != 1u
        || adapter.dynamicConstraintDimensionCount != 1u
        || adapter.linearDimensionCount != 1u
        || adapter.radialDimensionCount != 1u
        || adapter.diametricDimensionCount != 1u
        || adapter.angularDimensionCount != 1u
        || adapter.angular3pDimensionCount != 1u
        || adapter.ordinateDimensionCount != 1u
        || adapter.arcDimensionCount != 1u
        || adapter.largeRadialDimensionCount != 1u
        || adapter.toleranceCount != 1u
        || adapter.rtextCount != 1u
        || adapter.arcAlignedTextCount != 1u
        || adapter.lightCount != 1u
        || adapter.cameraCount != 1u
        || adapter.viewCount != 1u
        || adapter.geoPositionMarkerCount != 2u
        || adapter.line.basePoint.x != 1.25
        || adapter.line.secPoint.z != -6.0
        || adapter.circle.basePoint.x != -7.0
        || adapter.circle.basePoint.y != 8.5
        || adapter.circle.basePoint.z != 9.25
        || adapter.circle.radious != 2.75
        || adapter.polyline.handle == 0u
        || adapter.polyline.flags != 1
        || adapter.polyline.vertlist.size() != 3u
        || !adapter.polyline.vertlist[0]
        || adapter.polyline.vertlist[0]->x != 1.0
        || adapter.polyline.vertlist[0]->y != 1.5
        || !adapter.polyline.vertlist[1]
        || adapter.polyline.vertlist[1]->x != 4.0
        || adapter.polyline.vertlist[1]->y != 5.5
        || adapter.polyline.vertlist[1]->bulge != 0.5
        || !adapter.polyline.vertlist[2]
        || adapter.polyline.vertlist[2]->x != 8.0
        || adapter.polyline.vertlist[2]->y != 9.5
        || adapter.image.handle == 0u || adapter.image.ref == 0u
        || adapter.image.basePoint.x != 2.0
        || adapter.image.basePoint.y != 3.0
        || adapter.image.basePoint.z != 4.0
        || adapter.image.sizeu != 640.0
        || adapter.image.sizev != 480.0
        || lineTypeCount != 1u
        || lineType == adapter.storage.lineTypes.end()
        || lineType->handle == 0u
        || lineType->desc != "whole-operation line type"
        || lineType->path != std::vector<double>{10.0, -5.0}
        || lineType->size != 2
        || lineType->length != 15.0
        || adapter.typedXRecordCount != 1u
        || adapter.typedXRecord.handle == 0u
        || adapter.typedXRecord.parentHandle != kCustomDictionaryHandle
        || adapter.typedXRecord.m_values.size() != 2u
        || adapter.typedXRecord.m_values[0].code() != 1
        || adapter.typedXRecord.m_values[0].type() != DRW_Variant::STRING
        || !adapter.typedXRecord.m_values[0].stringEquals(
            "whole-operation typed XRECORD")
        || adapter.typedXRecord.m_values[1].code() != 90
        || adapter.typedXRecord.m_values[1].type() != DRW_Variant::INTEGER
        || adapter.typedXRecord.m_values[1].i_val() != 42
        || adapter.multiLeaderCount != 1u
        || adapter.multiLeader.handle == 0u
        || !adapter.multiLeader.isDxfContextClosed()
        || !adapter.multiLeader.context.hasTextContents
        || adapter.multiLeader.context.textLabel
            != "whole-operation multi-leader text"
        || adapter.multiLeader.context.contentBasePoint.x != 2.0
        || adapter.multiLeader.context.contentBasePoint.y != 3.0
        || adapter.multiLeader.context.contentBasePoint.z != 4.0
        || adapter.multiLeader.context.textLocation.x != 5.0
        || adapter.multiLeader.context.textLocation.y != 6.0
        || adapter.multiLeader.context.textLocation.z != 7.0
        || adapter.multiLeader.context.textHeight != 1.5
        || adapter.ole2FrameCount != 1u
        || adapter.ole2Frame.handle == 0u
        || adapter.ole2Frame.m_oleVersion != 2u
        || adapter.ole2Frame.m_oleClient != "WHOLE_OPERATION_OLE2"
        || adapter.ole2Frame.m_pt1.x != 1.0
        || adapter.ole2Frame.m_pt1.y != 2.0
        || adapter.ole2Frame.m_pt2.x != 7.0
        || adapter.ole2Frame.m_pt2.y != 9.0
        || adapter.ole2Frame.m_flags != 3u
        || adapter.ole2Frame.m_mode != 1u
        || adapter.ole2Frame.m_lockAspect != 1u
        || !adapter.ole2Frame.m_payloadPresent
        || adapter.ole2Frame.m_payloadBytes
            != std::vector<std::uint8_t>{0x00u, 0x1Fu, 0x80u, 0xFFu, 0x35u}
        || adapter.ole2Frame.m_declaredPayloadLength != 5u
        || adapter.oleFrameCount != 1u
        || adapter.oleFrame.handle == 0u
        || adapter.oleFrame.m_flags != 2u
        || !adapter.oleFrame.m_payloadPresent
        || adapter.oleFrame.m_payloadBytes
            != std::vector<std::uint8_t>{0x80u, 0x00u, 0xFFu, 0xA5u}
        || adapter.oleFrame.m_declaredPayloadLength != 4u
        || adapter.shapeCount != 1u
        || adapter.shape.handle == 0u
        || adapter.shape.m_styleName != "STANDARD"
        || adapter.shape.m_insertionPoint.x != 8.0
        || adapter.shape.m_insertionPoint.y != 9.0
        || adapter.shape.m_insertionPoint.z != 10.0
        || adapter.shape.m_scale != 1.75
        || std::abs(adapter.shape.m_rotation - 0.5) > 1.0e-12
        || adapter.shape.m_widthFactor != 0.8
        || std::abs(adapter.shape.m_oblique - 0.1) > 1.0e-12
        || adapter.shape.m_thickness != 0.25
        || adapter.shape.m_extrusion.z != 1.0
        || adapter.rasterVariablesCount != 1u
        || adapter.rasterVariables.handle == 0u
        || adapter.rasterVariables.parentHandle
            != DRW::DwgNamedObjectsDictionaryHandle
        || adapter.rasterVariables.m_classVersion != 0
        || adapter.rasterVariables.m_imageFrame != 1
        || adapter.rasterVariables.m_imageQuality != 1
        || adapter.rasterVariables.m_units != 4
        || adapter.dictionaryVarCount != 1u
        || adapter.dictionaryVar.handle != kDictionaryVarHandle
        || adapter.dictionaryVar.parentHandle
            != DRW::DwgNamedObjectsDictionaryHandle
        || adapter.dictionaryVar.m_schema != 1
        || adapter.dictionaryVar.m_value
            != "whole-operation dictionary value"
        || adapter.dbColorCount != 1u
        || adapter.dbColor.handle != kDbColorHandle
        || adapter.dbColor.parentHandle
            != DRW::DwgNamedObjectsDictionaryHandle
        || adapter.dbColor.rgb != 0x123456
        || !adapter.dbColor.hasDxfColorIndex()
        || adapter.dbColor.colorIndex != 7u
        || adapter.dbColor.bookName != "WHOLE_OPERATION_BOOK"
        || adapter.dbColor.name != "ACCENT"
        || adapter.underlayDefinitionCount != 1u
        || adapter.underlayDefinition.handle != kUnderlayDefinitionHandle
        || adapter.underlayDefinition.parentHandle
            != DRW::DwgNamedObjectsDictionaryHandle
        || adapter.underlayDefinition.kind != DRW_UnderlayDefinition::PDF
        || adapter.underlayDefinition.filename
            != "whole-operation-underlay.pdf"
        || adapter.underlayDefinition.sheetName != "sheet-1"
        || adapter.underlayCount != 1u
        || adapter.underlay.handle == 0u
        || adapter.underlay.kind != DRW_Underlay::PDF
        || adapter.underlay.definitionHandle != kUnderlayDefinitionHandle
        || adapter.underlay.position.x != 141.0
        || adapter.underlay.position.y != 142.0
        || adapter.underlay.position.z != 143.0
        || adapter.underlay.scale.x != 2.0
        || adapter.underlay.scale.y != 3.0
        || adapter.underlay.rotation != 0.25
        || adapter.underlay.contrast != 80u
        || adapter.underlay.fade != 5u
        || adapter.underlay.flags != 0x12u
        || adapter.underlay.clipBoundary.size() != 4u
        || adapter.underlay.clipBoundary[2].x != 64.0
        || adapter.underlay.clipBoundary[2].y != 48.0
        || adapter.underlay.inverseClipBoundary.size() != 4u
        || adapter.underlay.inverseClipBoundary[2].x != 56.0
        || adapter.underlay.inverseClipBoundary[2].y != 40.0
        || adapter.mesh.handle == 0u
        || adapter.mesh.vertices.size() != 3u
        || adapter.mesh.vertices[0].x != 0.0
        || adapter.mesh.vertices[1].x != 2.0
        || adapter.mesh.vertices[2].y != 3.0
        || adapter.mesh.faces != std::vector<std::vector<std::int32_t>>{{0, 1, 2}}
        || adapter.mesh.edges
            != std::vector<std::pair<std::int32_t, std::int32_t>>{{0, 1}, {1, 2}, {2, 0}}
        || adapter.mesh.creases != std::vector<double>{0.5}
        || adapter.mesh.propertyOverrides.size() != 1u
        || adapter.mesh.propertyOverrides.front().subEntityMarker != 7
        || adapter.mesh.propertyOverrides.front().propertyTypes
            != std::vector<std::int32_t>{0, 2}
        || adapter.spline.handle == 0u
        || adapter.spline.degree != 1
        || adapter.spline.controllist.size() != 2u
        || !adapter.spline.controllist[0]
        || adapter.spline.controllist[0]->x != -1.0
        || adapter.spline.controllist[0]->z != 2.0
        || !adapter.spline.controllist[1]
        || adapter.spline.controllist[1]->y != 4.0
        || adapter.spline.knotslist != std::vector<double>{0.0, 0.0, 1.0, 1.0}
        || adapter.hatch.handle == 0u
        || adapter.hatch.name != "ANSI31"
        || adapter.hatch.solid != 0
        || adapter.hatch.hpattern != 0
        || adapter.hatch.looplist.size() != 1u
        || !adapter.hatch.looplist.front()
        || adapter.hatch.looplist.front()->objlist.size() != 1u
        || dynamic_cast<const DRW_LWPolyline*>(
            adapter.hatch.looplist.front()->objlist.front().get()) == nullptr
        || dynamic_cast<const DRW_LWPolyline*>(
            adapter.hatch.looplist.front()->objlist.front().get())->vertlist.size() != 4u
        || adapter.hatch.patternLines.size() != 1u
        || adapter.hatch.patternLines.front().dashList
            != std::vector<double>{0.5, -0.25, 1.0}
        || adapter.hatch.seedPoints.size() != 1u
        || adapter.hatch.seedPoints.front().x != 0.5
        || adapter.hatch.seedPoints.front().y != 0.5
        || adapter.leader.handle == 0u
        || adapter.leader.style != "STANDARD"
        || adapter.leader.vertnum != 3
        || adapter.leader.vertexlist.size() != 3u
        || !adapter.leader.vertexlist[0]
        || adapter.leader.vertexlist[0]->x != 1.0
        || !adapter.leader.vertexlist[1]
        || adapter.leader.vertexlist[1]->y != 5.0
        || !adapter.leader.vertexlist[2]
        || adapter.leader.vertexlist[2]->z != 9.0
        || adapter.leader.offsetblock.x != 0.25
        || adapter.leader.offsetblock.y != 0.5
        || adapter.leader.offsetblock.z != 0.75
        || adapter.mline.handle == 0u
        || adapter.mline.styleName != "WHOLE_OPERATION_MLINESTYLE"
        || adapter.mline.styleHandle == 0u
        || adapter.mline.numLines != 1
        || adapter.mline.numVerts != 2
        || adapter.mline.vertlist.size() != 2u
        || adapter.mline.vertlist[0].position.x != 2.0
        || adapter.mline.vertlist[0].vertexDir.x != 1.0
        || adapter.mline.vertlist[0].miterDir.y != 1.0
        || adapter.mline.vertlist[0].segParms
            != std::vector<std::vector<double>>{{0.25}}
        || adapter.mline.vertlist[0].areaFillParms
            != std::vector<std::vector<double>>{{0.0}}
        || adapter.mline.vertlist[1].position.x != 7.0
        || adapter.mline.vertlist[1].segParms
            != std::vector<std::vector<double>>{{-0.5, 0.75}}
        || adapter.mline.vertlist[1].areaFillParms
            != std::vector<std::vector<double>>{{0.125}}
        || adapter.mlineStyle.handle == 0u
        || adapter.mlineStyle.name != "WHOLE_OPERATION_MLINESTYLE"
        || adapter.mlineStyle.elements.size() != 1u
        || adapter.mlineStyle.elements.front().offset != 0.5
        || adapter.mlineStyle.elements.front().linetype != "BYLAYER"
        || adapter.polyline3Count != 1u
        || adapter.polyline3.handle == 0u
        || adapter.polyline3.flags != 0
        || adapter.polyline3.basePoint.z != 2.5
        || adapter.polyline3.vertlist.size() != 2u
        || !adapter.polyline3.vertlist[0]
        || adapter.polyline3.vertlist[0]->handle == 0u
        || adapter.polyline3.vertlist[0]->basePoint.x != 1.0
        || adapter.polyline3.vertlist[0]->basePoint.y != 2.0
        || adapter.polyline3.vertlist[0]->bulge != 0.25
        || !adapter.polyline3.vertlist[1]
        || adapter.polyline3.vertlist[1]->handle == 0u
        || adapter.polyline3.vertlist[1]->basePoint.x != 4.0
        || adapter.polyline3.vertlist[1]->basePoint.y != 6.0
        || adapter.insertCount != 1u
        || adapter.insert.handle == 0u
        || adapter.insert.name != kInsertBlockName
        || adapter.insert.basePoint.x != 10.0
        || adapter.insert.basePoint.y != 20.0
        || adapter.insert.attlist.size() != 2u
        || !adapter.insert.attlist.front()
        || adapter.insert.attlist.front()->handle == 0u
        || adapter.insert.attlist.front()->tag != "WHOLE_OPERATION_TAG"
        || adapter.insert.attlist.front()->text != "whole operation attribute"
        || adapter.insert.attlist.front()->basePoint.x != 11.0
        || adapter.insert.attlist.front()->height != 2.5
        || !adapter.insert.attlist.back()
        || adapter.insert.attlist.back()->handle == 0u
        || adapter.insert.attlist.back()->tag != "WHOLE_OPERATION_TAG2"
        || adapter.insert.attlist.back()->text
            != "second whole operation attribute"
        || adapter.insert.attlist.back()->basePoint.x != 12.0
        || adapter.insert.attlist.back()->height != 1.25
        || adapter.faceCount != 1u
        || adapter.face.handle == 0u
        || adapter.face.basePoint.z != 1.0
        || adapter.face.secPoint.x != 3.0
        || adapter.face.secPoint.z != 2.0
        || adapter.face.thirdPoint.y != 4.0
        || adapter.face.thirdPoint.z != 3.0
        || adapter.face.fourPoint.y != 4.0
        || adapter.face.fourPoint.z != 4.0
        || adapter.face.invisibleflag
            != (DRW_3Dface::FirstEdge | DRW_3Dface::ThirdEdge)
        || adapter.dimensionCount != 1u
        || adapter.dimension.handle == 0u
        || adapter.dimension.getDefPoint().x != -3.0
        || adapter.dimension.getDefPoint().y != 4.0
        || adapter.dimension.getDefPoint().z != 5.0
        || adapter.dimension.getTextPoint().x != 1.5
        || adapter.dimension.getTextPoint().y != 2.5
        || adapter.dimension.getText() != "whole-operation dimension"
        || adapter.dimension.getStyle() != "STANDARD"
        || adapter.dynamicConstraintDimensionCount != 1u
        || adapter.dynamicConstraintDimension.handle == 0u
        || adapter.dynamicConstraintDimension.layer != "*ADSK_CONSTRAINTS"
        || adapter.dynamicConstraintDimension.getDefPoint().x != 15.0
        || adapter.dynamicConstraintDimension.getDefPoint().y != 16.0
        || adapter.dynamicConstraintDimension.getDefPoint().z != 17.0
        || adapter.dynamicConstraintDimension.getTextPoint().x != 15.5
        || adapter.dynamicConstraintDimension.getTextPoint().y != 16.5
        || adapter.dynamicConstraintDimension.getText()
            != "whole-operation dynamic constraint"
        || adapter.dynamicConstraintDimension.getStyle() != "STANDARD"
        || !commonDimensionMatches(adapter.linearDimension, 10.0)
        || adapter.linearDimension.getDef1Point().z != 12.0
        || adapter.linearDimension.getDef2Point().x != 13.0
        || !commonDimensionMatches(adapter.radialDimension, 20.0)
        || adapter.radialDimension.getDiameterPoint().x != 22.0
        || adapter.radialDimension.getLeaderLength() != 3.0
        || !commonDimensionMatches(adapter.diametricDimension, 30.0)
        || adapter.diametricDimension.getDiameter1Point().x != 32.0
        || adapter.diametricDimension.getLeaderLength() != 4.0
        || !commonDimensionMatches(adapter.angularDimension, 40.0, false)
        || adapter.angularDimension.getDefPoint().x != 41.0
        || adapter.angularDimension.getDefPoint().y != 41.0
        || adapter.angularDimension.getDefPoint().z != 0.0
        || adapter.angularDimension.getFirstLine1().x != 40.0
        || adapter.angularDimension.getSecondLine2().y != 41.0
        || adapter.angularDimension.getDimPoint().x != 40.5
        || !commonDimensionMatches(adapter.angular3pDimension, 50.0, false)
        || adapter.angular3pDimension.getDefPoint().x != 50.5
        || adapter.angular3pDimension.getDefPoint().y != 50.5
        || adapter.angular3pDimension.getDefPoint().z != 0.0
        || adapter.angular3pDimension.getFirstLine().x != 50.0
        || adapter.angular3pDimension.getVertexPoint().y != 51.0
        || adapter.angular3pDimension.getDimPoint().x != 50.5
        || !commonDimensionMatches(adapter.ordinateDimension, 60.0)
        || adapter.ordinateDimension.getFirstLine().x != 61.0
        || adapter.ordinateDimension.getSecondLine().z != 66.0
        || adapter.arcDimension.handle == 0u
        || adapter.arcDimension.getArcDefPoint().x != 70.0
        || adapter.arcDimension.getArcDefPoint().y != 71.0
        || adapter.arcDimension.getArcDefPoint().z != 72.0
        || adapter.arcDimension.getTextPoint().x != 70.5
        || adapter.arcDimension.getTextPoint().y != 71.5
        || adapter.arcDimension.getText() != "whole-operation arc dimension"
        || adapter.arcDimension.getStyle() != "STANDARD"
        || adapter.arcDimension.getExtLine1().x != 71.0
        || adapter.arcDimension.getExtLine2().y != 72.0
        || adapter.arcDimension.getArcCenter().x != 70.0
        || adapter.arcDimension.getLeaderPt1().x != 72.0
        || adapter.arcDimension.leaderPt2.x != 73.0
        || adapter.arcDimension.arcStartAngle != 0.25
        || adapter.arcDimension.arcEndAngle != 1.75
        || !adapter.arcDimension.isPartial
        || !adapter.arcDimension.hasLeader
        || adapter.largeRadialDimension.handle == 0u
        || adapter.largeRadialDimension.getCenterPoint().x != 80.0
        || adapter.largeRadialDimension.getCenterPoint().y != 81.0
        || adapter.largeRadialDimension.getCenterPoint().z != 82.0
        || adapter.largeRadialDimension.getTextPoint().x != 80.5
        || adapter.largeRadialDimension.getChordPoint().x != 83.0
        || adapter.largeRadialDimension.overrideCenterPoint.y != 81.0
        || adapter.largeRadialDimension.jogPoint.x != 84.0
        || adapter.largeRadialDimension.jogAngle != 0.375
        || adapter.tolerance.handle == 0u
        || adapter.tolerance.dimStyleName != "STANDARD"
        || adapter.tolerance.insertionPoint.x != 90.0
        || adapter.tolerance.insertionPoint.y != 91.0
        || adapter.tolerance.insertionPoint.z != 92.0
        || adapter.tolerance.text != "whole-operation tolerance"
        || adapter.tolerance.extPoint.x != 0.0
        || adapter.tolerance.extPoint.y != 0.0
        || adapter.tolerance.extPoint.z != 1.0
        || adapter.tolerance.xAxisDirectionVector.x != 1.0
        || adapter.tolerance.xAxisDirectionVector.y != 0.0
        || adapter.tolerance.xAxisDirectionVector.z != 0.0
        || adapter.rtext.handle == 0u
        || adapter.rtext.basePoint.x != 95.0
        || adapter.rtext.basePoint.y != 96.0
        || adapter.rtext.text != "whole-operation RTEXT"
        || adapter.rtext.height != 2.5
        || adapter.rtext.angle != 30.0
        || adapter.rtext.style != "STANDARD"
        || adapter.rtext.m_rTextFlags != 1
        || adapter.rtext.extPoint.x != 0.0
        || adapter.rtext.extPoint.y != 0.0
        || adapter.rtext.extPoint.z != 1.0
        || adapter.arcAlignedText.handle == 0u
        || adapter.arcAlignedText.text != "whole-operation arc-aligned text"
        || adapter.arcAlignedText.style != "STANDARD"
        || adapter.arcAlignedText.m_fontName != "SIMPLEX.SHX"
        || adapter.arcAlignedText.m_bigFontName != "BIGFONT.SHX"
        || adapter.arcAlignedText.m_center.x != 100.0
        || adapter.arcAlignedText.m_center.y != 101.0
        || adapter.arcAlignedText.m_radius != 12.0
        || adapter.arcAlignedText.m_startAngle != 0.25
        || adapter.arcAlignedText.m_endAngle != 1.25
        || adapter.arcAlignedText.m_xScale != "1.2"
        || adapter.arcAlignedText.m_textSize != "2.25"
        || adapter.arcAlignedText.m_charSpacing != "0.75"
        || adapter.arcAlignedText.m_offsetFromArc != "0.5"
        || adapter.arcAlignedText.m_rightOffset != "0.25"
        || adapter.arcAlignedText.m_leftOffset != "0.125"
        || adapter.arcAlignedText.m_rawColor != 0x010203
        || adapter.arcAlignedText.m_characterSet != 1
        || adapter.arcAlignedText.m_pitchAndFamily != 2
        || adapter.arcAlignedText.m_isShx != 1
        || adapter.arcAlignedText.m_isBold != 1
        || adapter.arcAlignedText.m_isItalic != 1
        || adapter.arcAlignedText.m_isUnderlined != 1
        || adapter.arcAlignedText.m_alignment != 2
        || adapter.arcAlignedText.m_isReverse != 1
        || adapter.arcAlignedText.m_wizardFlag != 1
        || adapter.arcAlignedText.m_textPosition != 1
        || adapter.arcAlignedText.m_textDirection != 1
        || adapter.light.handle == 0u
        || adapter.light.m_classVersion != 1u
        || adapter.light.m_name != "WHOLE_OPERATION_LIGHT"
        || adapter.light.m_type != 2u
        || !adapter.light.m_status
        || adapter.light.m_color != 3u
        || !adapter.light.m_plotGlyph
        || adapter.light.m_intensity != 2.5
        || adapter.light.m_position.x != 110.0
        || adapter.light.m_position.y != 111.0
        || adapter.light.m_position.z != 112.0
        || adapter.light.m_target.x != 113.0
        || adapter.light.m_target.y != 114.0
        || adapter.light.m_target.z != 115.0
        || adapter.light.m_attenuationType != 1u
        || !adapter.light.m_useAttenuationLimits
        || adapter.light.m_attenuationStartLimit != 0.5
        || adapter.light.m_attenuationEndLimit != 12.5
        || adapter.light.m_hotspotAngle != 0.25
        || adapter.light.m_falloffAngle != 0.75
        || !adapter.light.m_castShadows
        || adapter.light.m_shadowType != 1u
        || adapter.light.m_shadowMapSize != 512u
        || adapter.light.m_shadowMapSoftness != 2u
        || adapter.camera.handle == 0u
        || adapter.view.handle == 0u
        || adapter.view.name != "WHOLE_OPERATION_CAMERA_VIEW"
        || adapter.view.size.x != 640.0
        || adapter.view.size.y != 480.0
        || adapter.view.targetPoint.x != 10.0
        || adapter.view.targetPoint.y != 20.0
        || adapter.view.targetPoint.z != 30.0
        || adapter.camera.m_viewHandle != adapter.view.handle
        || adapter.geoPositionMarker.handle == 0u
        || adapter.geoPositionMarker.m_classVersion != 1u
        || adapter.geoPositionMarker.m_position.x != 120.0
        || adapter.geoPositionMarker.m_position.y != 121.0
        || adapter.geoPositionMarker.m_position.z != 122.0
        || adapter.geoPositionMarker.m_radius != 6.5
        || adapter.geoPositionMarker.m_notes != "whole-operation geo marker"
        || adapter.geoPositionMarker.m_landingGap != 0.75
        || !adapter.geoPositionMarker.m_mtextVisible
        || adapter.geoPositionMarker.m_textAlignment != 2u
        || adapter.geoPositionMarker.m_enableFrameText
        || adapter.geoPositionMarkerEmbedded.handle == 0u
        || adapter.geoPositionMarkerEmbedded.m_classVersion != 1u
        || adapter.geoPositionMarkerEmbedded.m_position.x != 123.0
        || adapter.geoPositionMarkerEmbedded.m_position.y != 124.0
        || adapter.geoPositionMarkerEmbedded.m_position.z != 125.0
        || adapter.geoPositionMarkerEmbedded.m_radius != 8.5
        || adapter.geoPositionMarkerEmbedded.m_notes != "whole-operation embedded geo marker"
        || adapter.geoPositionMarkerEmbedded.m_landingGap != 1.25
        || !adapter.geoPositionMarkerEmbedded.m_mtextVisible
        || adapter.geoPositionMarkerEmbedded.m_textAlignment != 1u
        || !adapter.geoPositionMarkerEmbedded.m_enableFrameText
        || !adapter.geoPositionMarkerEmbedded.mtext
        || adapter.geoPositionMarkerEmbedded.mtext->text
            != "whole-operation embedded frame"
        || adapter.geoPositionMarkerEmbedded.mtext->basePoint.x != 123.0
        || adapter.geoPositionMarkerEmbedded.mtext->basePoint.y != 124.0
        || adapter.geoPositionMarkerEmbedded.mtext->basePoint.z != 125.0
        || adapter.geoPositionMarkerEmbedded.mtext->height != 1.75
        || adapter.geoPositionMarkerEmbedded.mtext->widthscale != 5.0
        || adapter.geoPositionMarkerEmbedded.mtext->style != "STANDARD"
        || adapter.sectionObjectCount != 1u
        || adapter.sectionObject.handle == 0u
        || adapter.sectionObject.m_state != 3u
        || adapter.sectionObject.m_flags != 5u
        || adapter.sectionObject.m_name != "WHOLE_OPERATION_SECTION"
        || adapter.sectionObject.m_vertDir.x != 0.0
        || adapter.sectionObject.m_vertDir.y != 0.0
        || adapter.sectionObject.m_vertDir.z != 1.0
        || adapter.sectionObject.m_topHeight != 12.0
        || adapter.sectionObject.m_bottomHeight != -4.0
        || adapter.sectionObject.m_indicatorAlpha != 128u
        || adapter.sectionObject.m_indicatorColor != 6u
        || adapter.sectionObject.m_verts.size() != 2u
        || adapter.sectionObject.m_verts[0].x != 1.0
        || adapter.sectionObject.m_verts[0].y != 2.0
        || adapter.sectionObject.m_verts[0].z != 3.0
        || adapter.sectionObject.m_verts[1].x != 4.0
        || adapter.sectionObject.m_verts[1].y != 5.0
        || adapter.sectionObject.m_verts[1].z != 6.0
        || adapter.sectionObject.m_blVerts.size() != 1u
        || adapter.sectionObject.m_blVerts[0].x != -1.0
        || adapter.sectionObject.m_blVerts[0].y != -2.0
        || adapter.sectionObject.m_blVerts[0].z != -3.0
        || adapter.mpolygonCount != 1u
        || adapter.mpolygon.handle == 0u
        || adapter.mpolygon.dxfVersion != 1
        || adapter.mpolygon.name != "SOLID"
        || adapter.mpolygon.solid != 1
        || adapter.mpolygon.basePoint.z != 2.0
        || adapter.mpolygon.fillColorAci != 5
        || adapter.mpolygon.annotatedBoundary != 1
        || adapter.mpolygon.xDirX != 1.0
        || adapter.mpolygon.looplist.size() != 1u
        || !adapter.mpolygon.looplist[0]
        || adapter.mpolygon.looplist[0]->objlist.size() != 1u
        || !adapter.mpolygon.looplist[0]->objlist[0]
        || dynamic_cast<const DRW_LWPolyline*>(
            adapter.mpolygon.looplist[0]->objlist[0].get()) == nullptr
        || dynamic_cast<const DRW_LWPolyline*>(
            adapter.mpolygon.looplist[0]->objlist[0].get())->vertlist.size() != 4u
        || dynamic_cast<const DRW_LWPolyline*>(
            adapter.mpolygon.looplist[0]->objlist[0].get())->vertlist[2]->x != 4.0
        || dynamic_cast<const DRW_LWPolyline*>(
            adapter.mpolygon.looplist[0]->objlist[0].get())->vertlist[2]->y != 3.0
        || adapter.viewportCount != 1u
        || adapter.viewport.handle == 0u
        || adapter.viewport.pswidth != 210.0
        || adapter.viewport.psheight != 148.0
        || adapter.viewport.vpstatus != 1
        || adapter.viewport.vpID != 2
        || adapter.viewport.centerPX != 105.0
        || adapter.viewport.centerPY != 74.0
        || adapter.viewport.viewDir.x != 0.0
        || adapter.viewport.viewDir.y != 0.0
        || adapter.viewport.viewDir.z != 1.0
        || adapter.viewport.viewTarget.x != 10.0
        || adapter.viewport.viewTarget.y != 20.0
        || adapter.viewport.viewTarget.z != 30.0
        || adapter.viewport.viewHeight != 100.0
        || adapter.viewport.snapSpPX != 5.0
        || adapter.viewport.snapSpPY != 6.0
        || adapter.viewport.gridSpX != 10.0
        || adapter.viewport.gridSpY != 12.0
        || adapter.viewport.styleSheet != "WHOLE_OPERATION_STYLE"
        || adapter.arcCount != 1u
        || adapter.arc.handle == 0u
        || adapter.arc.basePoint.x != -3.0
        || adapter.arc.basePoint.y != 2.0
        || adapter.arc.basePoint.z != 5.0
        || adapter.arc.radious != 6.5
        || std::abs(adapter.arc.staangle - 0.25) > 1.0e-12
        || std::abs(adapter.arc.endangle - 2.75) > 1.0e-12
        || adapter.arc.thickness != 1.25
        || adapter.ellipseCount != 1u
        || adapter.ellipse.handle == 0u
        || adapter.ellipse.basePoint.x != 7.0
        || adapter.ellipse.basePoint.y != -4.0
        || adapter.ellipse.basePoint.z != 2.0
        || adapter.ellipse.secPoint.x != 4.0
        || adapter.ellipse.secPoint.y != 1.0
        || adapter.ellipse.secPoint.z != 0.0
        || adapter.ellipse.ratio != 0.5
        || std::abs(adapter.ellipse.staparam - 0.2) > 1.0e-12
        || std::abs(adapter.ellipse.endparam - 2.4) > 1.0e-12
        || adapter.pointCount != 2u
        || adapter.point.handle == 0u
        || adapter.point.basePoint.x != -2.0
        || adapter.point.basePoint.y != 3.0
        || adapter.point.basePoint.z != 4.0
        || std::abs(adapter.point.xAxisAngle - 0.75) > 1.0e-12
        || adapter.line3dCount != 1u
        || adapter.line3d.handle == 0u
        || adapter.line3d.basePoint.x != 1.0
        || adapter.line3d.basePoint.y != 2.0
        || adapter.line3d.basePoint.z != 3.0
        || adapter.line3d.secPoint.x != 4.0
        || adapter.line3d.secPoint.y != 5.0
        || adapter.line3d.secPoint.z != 6.0
        || adapter.solidCount != 1u
        || adapter.solid.handle == 0u
        || adapter.solid.basePoint.z != 1.5
        || adapter.solid.secPoint.x != 3.0
        || adapter.solid.thirdPoint.y != 2.0
        || adapter.solid.fourPoint.y != 2.0
        || adapter.traceCount != 1u
        || adapter.trace.handle == 0u
        || adapter.trace.basePoint.z != 2.0
        || adapter.trace.secPoint.x != 3.0
        || adapter.trace.thirdPoint.x != 3.0
        || adapter.trace.thirdPoint.y != 2.0
        || adapter.trace.fourPoint.y != 2.0
        || adapter.textCount != 1u
        || adapter.text.handle == 0u
        || adapter.text.basePoint.x != 12.0
        || adapter.text.basePoint.y != -3.0
        || adapter.text.basePoint.z != 1.0
        || adapter.text.text != "whole-operation TEXT"
        || adapter.text.height != 2.0
        || std::abs(adapter.text.angle - 15.0) > 1.0e-12
        || adapter.text.widthscale != 1.2
        || adapter.text.style != "STANDARD"
        || adapter.mtextCount != 1u
        || adapter.mtext.handle == 0u
        || adapter.mtext.basePoint.x != 4.0
        || adapter.mtext.basePoint.y != 5.0
        || adapter.mtext.basePoint.z != 6.0
        || adapter.mtext.text != "whole-operation MTEXT"
        || adapter.mtext.height != 1.5
        || adapter.mtext.widthscale != 12.0
        || adapter.mtext.style != "STANDARD"
        || adapter.mtext.m_definedHeight != 2.25
        || !adapter.mtext.m_definedHeightPresent
        || adapter.rayCount != 1u
        || adapter.ray.handle == 0u
        || adapter.ray.basePoint.x != -5.0
        || adapter.ray.basePoint.y != 7.0
        || adapter.ray.basePoint.z != 2.0
        || std::abs(adapter.ray.secPoint.x - 1.0 / std::sqrt(14.0)) > 1.0e-12
        || std::abs(adapter.ray.secPoint.y - 2.0 / std::sqrt(14.0)) > 1.0e-12
        || std::abs(adapter.ray.secPoint.z - 3.0 / std::sqrt(14.0)) > 1.0e-12
        || adapter.xlineCount != 1u
        || adapter.xline.handle == 0u
        || adapter.xline.basePoint.x != 8.0
        || adapter.xline.basePoint.y != -6.0
        || adapter.xline.basePoint.z != 4.0
        || std::abs(adapter.xline.secPoint.x - -2.0 / std::sqrt(5.25)) > 1.0e-12
        || std::abs(adapter.xline.secPoint.y - 1.0 / std::sqrt(5.25)) > 1.0e-12
        || std::abs(adapter.xline.secPoint.z - 0.5 / std::sqrt(5.25)) > 1.0e-12
        || adapter.helixCount != 1u
        || adapter.helix.handle == 0u
        || adapter.helix.m_majorVersion != 1
        || adapter.helix.m_maintVersion != 2
        || adapter.helix.degree != 2
        || adapter.helix.flags != 8
        || adapter.helix.knotslist != std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}
        || adapter.helix.controllist.size() != 3u
        || !adapter.helix.controllist.front()
        || adapter.helix.controllist.front()->x != 1.0
        || !adapter.helix.controllist.back()
        || adapter.helix.controllist.back()->z != 4.0
        || adapter.helix.axisBasePt.x != 2.0
        || adapter.helix.axisBasePt.y != 3.0
        || adapter.helix.axisBasePt.z != 4.0
        || adapter.helix.startPt.x != 3.0
        || adapter.helix.startPt.y != 3.0
        || adapter.helix.startPt.z != 4.0
        || adapter.helix.axisVector.z != 1.0
        || adapter.helix.radius != 1.0
        || adapter.helix.turns != 2.5
        || adapter.helix.turnHeight != 3.0
        || !adapter.helix.handedness
        || adapter.helix.constraintType != 1
        || adapter.materialCount != 1u
        || adapter.material.handle == 0u
        || adapter.material.m_name != "WHOLE_OPERATION_MATERIAL"
        || adapter.material.m_description != "material allocation retry"
        || adapter.loftedSurfaceCount != 1u
        || adapter.loftedSurface.handle == 0u
        || adapter.loftedSurface.modelerFormatVersion != 1
        || adapter.loftedSurface.uIsolines != 10
        || adapter.loftedSurface.vIsolines != 11
        || adapter.loftedSurface.planeNormalLoftingType != 1
        || adapter.loftedSurface.startDraftAngle != 0.2
        || adapter.loftedSurface.endDraftAngle != 0.3
        || adapter.loftedSurface.startDraftMagnitude != 0.4
        || adapter.loftedSurface.endDraftMagnitude != 0.5
        || !adapter.loftedSurface.arcLengthParameterization
        || !adapter.loftedSurface.closedSurfaces
        || !adapter.loftedSurface.solid
        || !adapter.loftedSurface.ruledSurface
        || !adapter.loftedSurface.virtualGuide
        || adapter.loftedSurface.loftEntityTransform[3] != 0.25
        || adapter.loftedSurface.loftEntityTransform[7] != 0.5
        || adapter.loftedSurface.loftEntityTransform[11] != 0.75
        || adapter.loftedSurface.dxfReferenceData.size() != 2u
        || adapter.loftedSurface.dxfReferenceData[0].code() != 90
        || adapter.loftedSurface.dxfReferenceData[0].i_val() != 2
        || adapter.loftedSurface.dxfReferenceData[1].code() != 310
        || !adapter.loftedSurface.dxfReferenceData[1].binary()
        || *adapter.loftedSurface.dxfReferenceData[1].binary()
            != std::vector<std::uint8_t>{0x12u, 0x34u}
        || adapter.sweptSurfaceCount != 1u
        || adapter.sweptSurface.handle == 0u
        || adapter.sweptSurface.modelerFormatVersion != 1
        || adapter.sweptSurface.uIsolines != 12
        || adapter.sweptSurface.vIsolines != 13
        || adapter.sweptSurface.sweepEntityId != 101u
        || adapter.sweptSurface.sweepData
            != std::vector<std::uint8_t>{0x21u, 0x22u}
        || adapter.sweptSurface.pathEntityId != 202u
        || adapter.sweptSurface.pathData
            != std::vector<std::uint8_t>{0x31u, 0x32u, 0x33u}
        || adapter.sweptSurface.referenceVector.x != 1.0
        || adapter.sweptSurface.referenceVector.y != 2.0
        || adapter.sweptSurface.referenceVector.z != 3.0
        || adapter.sweptSurface.sweepAlignmentFlags != 2
        || adapter.sweptSurface.pathFlags != 3
        || adapter.sweptSurface.draftAngle != 0.125
        || adapter.sweptSurface.draftStartDistance != 0.25
        || adapter.sweptSurface.draftEndDistance != 0.5
        || adapter.sweptSurface.twistAngle != 0.75
        || adapter.sweptSurface.scaleFactor != 1.25
        || adapter.sweptSurface.alignAngle != 0.875
        || adapter.sweptSurface.sweepEntityTransform[3] != 0.5
        || adapter.sweptSurface.pathEntityTransform[7] != 0.75
        || adapter.sweptSurface.sweepEntityTransformed[11] != 1.0
        || adapter.sweptSurface.pathEntityTransformed[15] != 1.25
        || !adapter.sweptSurface.solid
        || !adapter.sweptSurface.alignStart
        || !adapter.sweptSurface.bank
        || !adapter.sweptSurface.basePointSet
        || !adapter.sweptSurface.sweepEntityTransformComputed
        || !adapter.sweptSurface.pathEntityTransformComputed
        || adapter.planeSurfaceCount != 1u
        || adapter.planeSurface.handle == 0u
        || adapter.planeSurface.modelerFormatVersion != 1
        || adapter.planeSurface.uIsolines != 2
        || adapter.planeSurface.vIsolines != 3
        || adapter.extrudedSurfaceCount != 1u
        || adapter.extrudedSurface.handle == 0u
        || adapter.extrudedSurface.classId != 2u
        || adapter.extrudedSurface.dxfBinaryData
            != std::vector<std::uint8_t>{0x21u, 0x43u}
        || adapter.extrudedSurface.sweepVector.z != 1.0
        || adapter.extrudedSurface.referenceVector.x != 1.0
        || adapter.extrudedSurface.sweepAlignmentFlags != 3
        || adapter.extrudedSurface.pathFlags != 7
        || adapter.extrudedSurface.draftAngle != 0.1
        || adapter.extrudedSurface.draftStartDistance != 0.25
        || adapter.extrudedSurface.draftEndDistance != 0.5
        || adapter.extrudedSurface.twistAngle != 0.75
        || adapter.extrudedSurface.scaleFactor != 1.25
        || adapter.extrudedSurface.alignAngle != 0.5
        || !adapter.extrudedSurface.solid
        || !adapter.extrudedSurface.alignStart
        || !adapter.extrudedSurface.bank
        || !adapter.extrudedSurface.basePointSet
        || !adapter.extrudedSurface.sweepEntityTransformComputed
        || !adapter.extrudedSurface.pathEntityTransformComputed
        || adapter.extrudedSurface.extrudedTransform[3] != 0.25
        || adapter.extrudedSurface.sweepEntityTransform[3] != 1.25
        || adapter.extrudedSurface.pathEntityTransform[7] != 1.5
        || adapter.revolvedSurfaceCount != 1u
        || adapter.revolvedSurface.handle == 0u
        || adapter.revolvedSurface.id != 2u
        || adapter.revolvedSurface.dxfBinaryData
            != std::vector<std::uint8_t>{0x65u, 0x87u}
        || adapter.revolvedSurface.axisPoint.x != 1.0
        || adapter.revolvedSurface.axisVector.y != 1.0
        || adapter.revolvedSurface.revolveAngle != 1.5
        || adapter.revolvedSurface.startAngle != 0.25
        || adapter.revolvedSurface.draftAngle != 0.5
        || adapter.revolvedSurface.draftStartDistance != 0.75
        || adapter.revolvedSurface.draftEndDistance != 1.0
        || adapter.revolvedSurface.twistAngle != 1.25
        || !adapter.revolvedSurface.solid
        || !adapter.revolvedSurface.closeToAxis
        || adapter.revolvedSurface.transform[3] != 0.25
        || adapter.revolvedSurface.transform[7] != 0.5
        || adapter.revolvedSurface.transform[11] != 0.75
        || adapter.nurbsSurfaceCount != 1u
        || adapter.nurbsSurface.handle == 0u
        || adapter.nurbsSurface.short170 != 16u
        || !adapter.nurbsSurface.cvHullDisplay
        || adapter.nurbsSurface.uvec1.x != 1.0
        || adapter.nurbsSurface.vvec1.y != 1.0
        || adapter.nurbsSurface.uvec2.x != 2.0
        || adapter.nurbsSurface.vvec2.y != 2.0
        || adapter.scaleCount != 1u
        || adapter.scale.handle == 0u
        || adapter.scale.parentHandle != DRW::DwgNamedObjectsDictionaryHandle
        || adapter.scale.name != "WHOLE_OPERATION_SCALE"
        || adapter.scale.flag != 0
        || adapter.scale.paperUnits != 2.0
        || adapter.scale.drawingUnits != 5.0
        || adapter.scale.isUnitScale
        || adapter.backgroundCount != 1u
        || adapter.background.handle == 0u
        || adapter.background.parentHandle != DRW::DwgNamedObjectsDictionaryHandle
        || adapter.background.m_kind != DRW_Background::Solid
        || adapter.background.m_classVersion != 1
        || adapter.background.m_solidColor != 0x010203
        || adapter.plotSettingsCount != 1u
        || adapter.plotSettings.handle == 0u
        || adapter.plotSettings.parentHandle != DRW::DwgNamedObjectsDictionaryHandle
        || adapter.plotSettings.pageSetupName != "WHOLE_OPERATION_PAGE_SETUP"
        || adapter.plotSettings.printerConfig != "WHOLE_OPERATION_PRINTER"
        || adapter.plotSettings.paperSize != "ISO_A4_(210.00_x_297.00_MM)"
        || adapter.plotSettings.paperWidth != 210.0
        || adapter.plotSettings.paperHeight != 297.0
        || adapter.plotSettings.marginLeft != 4.0
        || adapter.plotSettings.marginBottom != 5.0
        || adapter.plotSettings.marginRight != 6.0
        || adapter.plotSettings.marginTop != 7.0
        || adapter.plotSettings.plotOriginX != 1.0
        || adapter.plotSettings.plotOriginY != 2.0
        || adapter.plotSettings.paperUnits != 1
        || adapter.plotSettings.plotRotation != 2
        || adapter.plotSettings.plotType != 1
        || adapter.plotSettings.windowMinX != 10.0
        || adapter.plotSettings.windowMinY != 11.0
        || adapter.plotSettings.windowMaxX != 20.0
        || adapter.plotSettings.windowMaxY != 21.0
        || adapter.plotSettings.realWorldUnits != 1.0
        || adapter.plotSettings.drawingUnits != 2.0
        || adapter.plotSettings.currentStyleSheet != "WHOLE_OPERATION_STYLE"
        || adapter.plotSettings.scaleType != 3
        || adapter.plotSettings.scaleFactor != 0.5
        || adapter.plotSettings.paperImageOriginX != 0.25
        || adapter.plotSettings.paperImageOriginY != 0.75
        || adapter.plotSettings.shadePlotMode != 1
        || adapter.plotSettings.shadePlotResLevel != 2
        || adapter.plotSettings.shadePlotCustomDPI != 150
        || adapter.wipeoutVariablesCount != 1u
        || adapter.wipeoutVariables.handle == 0u
        || adapter.wipeoutVariables.parentHandle != DRW::DwgNamedObjectsDictionaryHandle
        || adapter.wipeoutVariables.m_classVersion != 0
        || adapter.wipeoutVariables.m_displayFrame != 1u
        || adapter.sunCount != 1u
        || adapter.sun.handle == 0u
        || adapter.sun.parentHandle != DRW::DwgNamedObjectsDictionaryHandle
        || adapter.sun.m_classVersion != 0
        || !adapter.sun.m_isOn
        || adapter.sun.m_color != 7u
        || adapter.sun.m_intensity != 0.625
        || !adapter.sun.m_hasShadow
        || adapter.sun.m_julianDay != 2460000
        || adapter.sun.m_milliseconds != 43210000
        || !adapter.sun.m_isDaylightSavings
        || adapter.sun.m_shadowType != 2u
        || adapter.sun.m_shadowMapSize != 1024u
        || adapter.sun.m_shadowSoftness != 3u
        || insertBlock == adapter.storage.blocks.end()
        || (*insertBlock)->ent.size() != 2u
        || (*insertBlock)->ent.front()->handle == 0u
        || (*insertBlock)->ent.back()->handle == 0u
        || dynamic_cast<const DRW_Attdef*>((*insertBlock)->ent.back()) == nullptr
        || dynamic_cast<const DRW_Attdef*>(
            (*insertBlock)->ent.back())->tag != "BLOCK_TAG"
        || dynamic_cast<const DRW_Attdef*>(
            (*insertBlock)->ent.back())->prompt != "Enter block value"
        || adapter.attdefCount != 1u
        || adapter.attdef.handle == 0u
        || adapter.attdef.tag != "BLOCK_TAG"
        || adapter.attdef.prompt != "Enter block value"
        || adapter.attdef.text != "Default value"
        || adapter.attdef.basePoint.x != 2.0
        || adapter.attdef.basePoint.y != 4.0
        || adapter.attdef.height != 1.75
        || imageDefinition == nullptr
        || imageDefinition->name != "whole-operation-image.png"
        || imageDefinition->u != 640.0 || imageDefinition->v != 480.0
        || imageDefinition->loaded != 1 || imageDefinition->resolution != 5
        || rawXrecord == adapter.objects.end()
        || rawXrecord->groups.size() != 4u
        || !rawXrecord->groups[3].stringEquals("whole operation payload")
        || adapter.dataLinks.size() != 3u
        || mainDataLink == adapter.dataLinks.end()
        || targetAlpha == adapter.dataLinks.end()
        || targetBeta == adapter.dataLinks.end()
        || mainDataLink->parentHandle != 0xD914u
        || mainDataLink->m_customDataCount != 2
        || mainDataLink->m_customData.size() != 2u
        || mainDataLink->m_customData[0].m_targetHandle
            != targetAlpha->handle
        || mainDataLink->m_customData[0].m_text != "sheet alpha"
        || mainDataLink->m_customData[1].m_targetHandle
            != targetBeta->handle
        || mainDataLink->m_customData[1].m_text != "sheet beta"
        || targetAlpha->parentHandle != 0xD914u
        || targetBeta->parentHandle != 0xD914u) {
        std::cerr << "readback read=" << read << " version=" << reader.getVersion()
                  << " lines=" << adapter.lineCount << " circles="
                  << adapter.circleCount << " images=" << adapter.imageCount
                  << " objects=" << adapter.objectCount
                  << " polylines=" << adapter.polylineCount
                  << " polylineHandle=" << adapter.polyline.handle
                  << " polylineFlags=" << adapter.polyline.flags
                  << " polylineVertices=" << adapter.polyline.vertlist.size()
                  << " insertCount=" << adapter.insertCount
                  << " insertName=" << adapter.insert.name
                  << " insertHandle=" << adapter.insert.handle
                  << " attribs=" << adapter.insert.attlist.size()
                  << " arcs=" << adapter.arcCount << " arcHandle="
                  << adapter.arc.handle << " arcRadius=" << adapter.arc.radious
                  << " arcStart=" << adapter.arc.staangle << " arcEnd="
                  << adapter.arc.endangle << " arcThickness="
                  << adapter.arc.thickness << " ellipses="
                  << adapter.ellipseCount << " ellipseHandle="
                  << adapter.ellipse.handle << " ellipseRatio="
                  << adapter.ellipse.ratio << " ellipseStart="
                  << adapter.ellipse.staparam << " ellipseEnd="
                  << adapter.ellipse.endparam << " points="
                  << adapter.pointCount << " pointHandle=" << adapter.point.handle
                  << " point=" << adapter.point.basePoint.x << ','
                  << adapter.point.basePoint.y << ',' << adapter.point.basePoint.z
                  << " angle=" << adapter.point.xAxisAngle
                  << " rays=" << adapter.rayCount << " rayHandle="
                  << adapter.ray.handle << " rayBase="
                  << adapter.ray.basePoint.x << ',' << adapter.ray.basePoint.y
                  << ',' << adapter.ray.basePoint.z << " rayDir="
                  << adapter.ray.secPoint.x << ',' << adapter.ray.secPoint.y
                  << ',' << adapter.ray.secPoint.z << " xlines="
                  << adapter.xlineCount << " xlineHandle="
                  << adapter.xline.handle << " xlineBase="
                  << adapter.xline.basePoint.x << ',' << adapter.xline.basePoint.y
                  << ',' << adapter.xline.basePoint.z << " xlineDir="
                  << adapter.xline.secPoint.x << ',' << adapter.xline.secPoint.y
                  << ',' << adapter.xline.secPoint.z
                  << " blocks=" << adapter.storage.blocks.size()
                  << " lineHandle=" << adapter.line.handle
                  << " firstX=" << adapter.line.basePoint.x << " secondZ="
                  << adapter.line.secPoint.z << " rawGroups="
                  << adapter.objects.size() << " dimensions="
                  << adapter.dimensionCount << '/' << adapter.linearDimensionCount
                  << '/' << adapter.radialDimensionCount << '/'
                  << adapter.diametricDimensionCount << '/'
                  << adapter.angularDimensionCount << '/'
                  << adapter.angular3pDimensionCount << '/'
                  << adapter.ordinateDimensionCount << "/arc="
                  << adapter.arcDimensionCount << "/largeRadial="
                  << adapter.largeRadialDimensionCount;
        const auto printDimension = [](const char* name,
                                       const DRW_Dimension& value) {
            const DRW_Coord definition = value.getDefPoint();
            const DRW_Coord text = value.getTextPoint();
            std::cerr << ' ' << name << "[handle=" << value.handle
                      << ",type=" << value.eType << ",def="
                      << definition.x << ',' << definition.y << ','
                      << definition.z << ",textPoint=" << text.x << ','
                      << text.y << ',' << text.z << ",label="
                      << value.getText() << ",style=" << value.getStyle()
                      << ']';
        };
        printDimension("aligned", adapter.dimension);
        printDimension("linear", adapter.linearDimension);
        printDimension("radial", adapter.radialDimension);
        printDimension("diametric", adapter.diametricDimension);
        printDimension("angular", adapter.angularDimension);
        printDimension("angular3p", adapter.angular3pDimension);
        printDimension("ordinate", adapter.ordinateDimension);
        std::cerr << " large radial fields: handle="
                  << adapter.largeRadialDimension.handle << ", center="
                  << adapter.largeRadialDimension.getCenterPoint().x << ','
                  << adapter.largeRadialDimension.getCenterPoint().y << ','
                  << adapter.largeRadialDimension.getCenterPoint().z
                  << ", chord="
                  << adapter.largeRadialDimension.getChordPoint().x << ','
                  << adapter.largeRadialDimension.getChordPoint().y << ','
                  << adapter.largeRadialDimension.getChordPoint().z
                  << ", override="
                  << adapter.largeRadialDimension.overrideCenterPoint.x << ','
                  << adapter.largeRadialDimension.overrideCenterPoint.y << ','
                  << adapter.largeRadialDimension.overrideCenterPoint.z
                  << ", jog=" << adapter.largeRadialDimension.jogPoint.x << ','
                  << adapter.largeRadialDimension.jogPoint.y << ','
                  << adapter.largeRadialDimension.jogPoint.z << ','
                  << adapter.largeRadialDimension.jogAngle << '\n';
        std::cerr << " geoposition markers: count="
                  << adapter.geoPositionMarkerCount << " plain="
                  << adapter.geoPositionMarker.handle << ','
                  << adapter.geoPositionMarker.m_notes << ','
                  << adapter.geoPositionMarker.m_enableFrameText
                  << " embedded=" << adapter.geoPositionMarkerEmbedded.handle
                  << ',' << adapter.geoPositionMarkerEmbedded.m_notes << ','
                  << adapter.geoPositionMarkerEmbedded.m_enableFrameText;
        if (adapter.geoPositionMarkerEmbedded.mtext != nullptr) {
            std::cerr << " mtext="
                      << adapter.geoPositionMarkerEmbedded.mtext->text << ','
                      << adapter.geoPositionMarkerEmbedded.mtext->basePoint.x
                      << ','
                      << adapter.geoPositionMarkerEmbedded.mtext->basePoint.y
                      << ','
                      << adapter.geoPositionMarkerEmbedded.mtext->basePoint.z
                      << ','
                      << adapter.geoPositionMarkerEmbedded.mtext->height << ','
                      << adapter.geoPositionMarkerEmbedded.mtext->widthscale
                      << ','
                      << adapter.geoPositionMarkerEmbedded.mtext->style;
        }
        std::cerr << '\n';
        if (rawXrecord != adapter.objects.end()
            && rawXrecord->groups.size() > 3u)
            std::cerr << " rawPayloadMatches="
                      << rawXrecord->groups[3].stringEquals(
                             "whole operation payload");
        std::cerr << " typed-xrecord=" << adapter.typedXRecordCount << '/'
                  << adapter.typedXRecord.handle << '/'
                  << adapter.typedXRecord.parentHandle << '/'
                  << adapter.typedXRecord.m_values.size();
        for (const DRW_Variant& value : adapter.typedXRecord.m_values)
            std::cerr << '[' << value.code() << ':' << value.type() << ':'
                      << value.stringEquals("whole-operation typed XRECORD")
                      << ':' << (value.type() == DRW_Variant::INTEGER
                                      ? value.i_val() : -999) << ']';
        std::cerr << " underlay-definitions=" << adapter.underlayDefinitionCount
                  << " underlay-entities=" << adapter.underlayCount
                  << " underlay-def-handle=" << adapter.underlayDefinition.handle
                  << " underlay-def-parent=" << adapter.underlayDefinition.parentHandle
                  << " underlay-def-name=" << adapter.underlayDefinition.filename
                  << '/' << adapter.underlayDefinition.sheetName
                  << " underlay-handle=" << adapter.underlay.handle
                  << " underlay-kind=" << adapter.underlay.kind
                  << " underlay-ref=" << adapter.underlay.definitionHandle
                  << " flags=" << static_cast<unsigned>(adapter.underlay.flags)
                  << " pos=" << adapter.underlay.position.x << ','
                  << adapter.underlay.position.y << ','
                  << adapter.underlay.position.z
                  << " scale=" << adapter.underlay.scale.x << ','
                  << adapter.underlay.scale.y << ','
                  << adapter.underlay.scale.z
                  << " rotation=" << adapter.underlay.rotation
                  << " contrast/fade=" << static_cast<unsigned>(adapter.underlay.contrast)
                  << '/' << static_cast<unsigned>(adapter.underlay.fade)
                  << " clip=" << adapter.underlay.clipBoundary.size()
                  << ':' << (adapter.underlay.clipBoundary.size() > 2u
                          ? adapter.underlay.clipBoundary[2].x : -1.0)
                  << ',' << (adapter.underlay.clipBoundary.size() > 2u
                          ? adapter.underlay.clipBoundary[2].y : -1.0)
                  << " inverse-clip="
                  << adapter.underlay.inverseClipBoundary.size()
                  << ':' << (adapter.underlay.inverseClipBoundary.size() > 2u
                          ? adapter.underlay.inverseClipBoundary[2].x : -1.0)
                  << ',' << (adapter.underlay.inverseClipBoundary.size() > 2u
                          ? adapter.underlay.inverseClipBoundary[2].y : -1.0);
        std::cerr << '\n';
        return false;
    }
    const DRW_Dictionary& root = adapter.storage.namedObjectDictionary;
    const auto groupRootEntry = std::find_if(
        root.m_entries.begin(), root.m_entries.end(),
        [](const DRW_Dictionary::Entry& entry) {
            return entry.m_name == "ACAD_GROUP" && entry.m_handle == 0xDu;
        });
    const auto imageRootEntry = std::find_if(
        root.m_entries.begin(), root.m_entries.end(),
        [](const DRW_Dictionary::Entry& entry) {
            return entry.m_name == "ACAD_IMAGE_DICT";
        });
    const auto customRootEntry = std::find_if(
        root.m_entries.begin(), root.m_entries.end(),
        [](const DRW_Dictionary::Entry& entry) {
            return entry.m_name == kCustomDictionaryName
                && entry.m_handle == kCustomDictionaryHandle;
        });
    const auto dictionaryVarRootEntry = std::find_if(
        root.m_entries.begin(), root.m_entries.end(),
        [](const DRW_Dictionary::Entry& entry) {
            return entry.m_name == "WHOLE_OPERATION_DICTIONARYVAR"
                && entry.m_handle == kDictionaryVarHandle;
        });
    const auto dbColorRootEntry = std::find_if(
        root.m_entries.begin(), root.m_entries.end(),
        [](const DRW_Dictionary::Entry& entry) {
            return entry.m_name == "WHOLE_OPERATION_DBCOLOR"
                && entry.m_handle == kDbColorHandle;
        });
    const bool rootMatches = adapter.storage.hasNamedObjectDictionary
        && root.handle == 0xCu && root.m_entries.size() == 5u
        && groupRootEntry != root.m_entries.end()
        && imageRootEntry != root.m_entries.end()
        && customRootEntry != root.m_entries.end()
        && dictionaryVarRootEntry != root.m_entries.end()
        && dbColorRootEntry != root.m_entries.end();
    const auto child = std::find_if(
        adapter.dictionaries.begin(), adapter.dictionaries.end(),
        [](const DRW_Dictionary& value) {
            return value.handle == 0xDu && value.parentHandle == 0xCu
                && value.m_entries.size() == 1u
                && value.m_entries.front().m_name == kGroupName;
        });
    const auto imageDictionary = imageRootEntry == root.m_entries.end()
        ? adapter.dictionaries.end()
        : std::find_if(
              adapter.dictionaries.begin(), adapter.dictionaries.end(),
              [&root, &imageRootEntry, &imageDefinition](
                  const DRW_Dictionary& value) {
                  return value.handle == imageRootEntry->m_handle
                      && value.parentHandle == root.handle
                      && value.m_entries.size() == 1u
                      && imageDefinition != nullptr
                      && value.m_entries.front().m_handle
                          == imageDefinition->handle;
              });
    const auto customDictionary = customRootEntry == root.m_entries.end()
        ? adapter.dictionaries.end()
        : std::find_if(
              adapter.dictionaries.begin(), adapter.dictionaries.end(),
              [&root, &customRootEntry](const DRW_Dictionary& value) {
                  return value.handle == customRootEntry->m_handle
                      && value.parentHandle == root.handle
                      && value.m_entries.size() == 5u
                      && value.m_entries.front().m_name == kNestedXrecordName
                      && value.m_entries.front().m_handle == 0xD901u
                      && value.m_entries[1].m_name == "MAIN_DATALINK"
                      && value.m_entries[1].m_handle == kMainDataLinkHandle
                      && value.m_entries[2].m_name == "TARGET_ALPHA"
                      && value.m_entries[2].m_handle == kTargetAlphaHandle
                      && value.m_entries[3].m_name == "TARGET_BETA"
                      && value.m_entries[3].m_handle == kTargetBetaHandle
                      && value.m_entries[4].m_name
                          == "WHOLE_OPERATION_TYPED_XRECORD"
                      && value.m_entries[4].m_handle
                          == kTypedXRecordHandle;
              });
    const auto group = std::find_if(
        adapter.groups.begin(), adapter.groups.end(),
        [](const DRW_Group& value) {
            return value.handle != 0u && value.handle != DRW::NoHandle
                && value.m_entityHandles.size() == 2u;
        });
    const bool graphMatches = rootMatches
        && child != adapter.dictionaries.end()
        && imageDictionary != adapter.dictionaries.end()
        && customDictionary != adapter.dictionaries.end()
        && rawXrecord != adapter.objects.end()
        && rawXrecord->parentHandle == customDictionary->handle
        && group != adapter.groups.end()
        && child->m_entries.front().m_handle == group->handle
        && group->m_entityHandles[0] == adapter.line.handle
        && group->m_entityHandles[1] == adapter.circle.handle;
    if (!graphMatches) {
        std::cerr << "graph dictionaries=" << adapter.dictionaries.size()
                  << " groups=" << adapter.groups.size()
                  << " root=" << rootMatches
                  << " child=" << (child != adapter.dictionaries.end())
                  << " group=" << (group != adapter.groups.end());
        std::cerr << " rootHandle=" << root.handle;
        if (child != adapter.dictionaries.end())
            std::cerr << " childHandle=" << child->handle << " groupRef="
                      << child->m_entries.front().m_handle;
        if (group != adapter.groups.end())
            std::cerr << " groupHandle=" << group->handle << " members="
                      << group->m_entityHandles.size();
        std::cerr << " imageRoot="
                  << (imageRootEntry != root.m_entries.end())
                  << " imageDict="
                  << (imageDictionary != adapter.dictionaries.end())
                  << " customRoot="
                  << (customRootEntry != root.m_entries.end())
                  << " customDict="
                  << (customDictionary != adapter.dictionaries.end())
                  << " xrecords=" << adapter.objects.size();
        if (customRootEntry != root.m_entries.end())
            std::cerr << " customRootHandle=" << customRootEntry->m_handle;
        if (customDictionary != adapter.dictionaries.end())
            std::cerr << " customParent=" << customDictionary->parentHandle
                      << " customEntryHandle="
                      << customDictionary->m_entries.front().m_handle;
        if (rawXrecord != adapter.objects.end())
            std::cerr << " xrecordParent=" << rawXrecord->parentHandle;
        std::cerr << " lineHandle=" << adapter.line.handle << '\n';
    }
    return graphMatches;
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "whole-operation DXF allocation transaction: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    const auto output = directory / "whole-operation.dxf";
    const auto publicationTarget = directory / "publication-race-target.dxf";
    if (!clearStagedOutput(output) || !clearStagedOutput(publicationTarget))
        return fail("could not clear stale output staging files in test scratch");
    const std::string sentinel = "previous complete drawing\n";
    for (const bool binary : {false, true}) {
        // Flush any one-time writer/codec setup allocations before counting
        // allocations belonging to a normal operation.
        {
            dxfRW codec(output.string().c_str());
            Export adapter(codec);
            if (!installGeneratedGroupGraph(codec, adapter.line.handle,
                                            adapter.circle.handle)
                || !installGeneratedImageDefinition(codec)
                || !codec.write(&adapter, DRW::AC1027, binary)) {
                std::cerr << "warm-up binary=" << binary
                          << " entities=" << adapter.entitiesWritten
                          << " objects=" << adapter.objectsWritten
                          << " error=" << codec.getError() << '\n';
                return fail("warm-up write failed");
            }
        }
        std::error_code warmupError;
        std::filesystem::remove(output, warmupError);

        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW codec(output.string().c_str());
            Export adapter(codec);
            if (!installGeneratedGroupGraph(codec, adapter.line.handle,
                                            adapter.circle.handle))
                return fail("could not install generated graph before calibration");
            if (!installGeneratedImageDefinition(codec))
                return fail("could not install generated IMAGEDEF before calibration");
            armAllocationFailure(1000000000L);
            const bool wrote = codec.write(&adapter, DRW::AC1027, binary);
            allocationCount = static_cast<long>(allocationAttempts);
            disarmAllocationFailure();
            const bool readback = matchesReadback(output);
            const bool bytesRead = readBytes(output, expectedBytes);
            if (!wrote || !adapter.entitiesWritten || !adapter.objectsWritten
                || !readback || !bytesRead
                || !callerRecordsUnchanged(adapter)) {
                std::cerr << "calibration wrote=" << wrote
                          << " entities=" << adapter.entitiesWritten
                          << " objects=" << adapter.objectsWritten
                          << " readback=" << readback
                          << " bytes=" << bytesRead
                          << " callerState="
                          << callerRecordsUnchanged(adapter)
                          << " error=" << codec.getError() << '\n';
                const auto printDimension = [](const char* name,
                                               const DRW_Dimension& value) {
                    const DRW_Coord definition = value.getDefPoint();
                    const DRW_Coord text = value.getTextPoint();
                    std::cerr << name << " input: handle=" << value.handle
                              << ", type=" << value.eType << ", def="
                              << definition.x << ',' << definition.y << ','
                              << definition.z << ", textPoint=" << text.x
                              << ',' << text.y << ',' << text.z << ", label="
                              << value.getText() << ", style="
                              << value.getStyle() << '\n';
                };
                printDimension("aligned", adapter.dimension);
                printDimension("linear", adapter.linearDimension);
                printDimension("radial", adapter.radialDimension);
                printDimension("diametric", adapter.diametricDimension);
                printDimension("angular", adapter.angularDimension);
                printDimension("angular3p", adapter.angular3pDimension);
                printDimension("ordinate", adapter.ordinateDimension);
                return fail("calibration write or readback failed");
            }
        }
        if (allocationCount <= 0)
            return fail("calibration observed no operation allocations");

        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream prior(output,
                                   std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior)
                    return fail("could not initialize destination sentinel");
            }

            dxfRW codec(output.string().c_str());
            Export adapter(codec);
            if (!installGeneratedGroupGraph(codec, adapter.line.handle,
                                            adapter.circle.handle))
                return fail("could not install generated graph before injection");
            if (!installGeneratedImageDefinition(codec))
                return fail("could not install generated IMAGEDEF before injection");
            armAllocationFailure(offset);
            const bool wrote = codec.write(&adapter, DRW::AC1027, binary);
            const std::size_t reached = allocationAttempts;
            const bool injected = allocationFailed;
            disarmAllocationFailure();
            const auto& context = *codec.getWritingContext();
            const bool identitiesRolledBack =
                context.sourceHandleToMintedMap.empty()
                && context.sourceHandleToWrittenObjectMap.empty()
                && context.writtenSourceEntityHandles.empty()
                && context.ambiguousSourceHandles.empty();
            const bool stagingClean = !hasStagedOutput(output);
            if (!injected || wrote || reached != static_cast<std::size_t>(offset + 1)) {
                std::cerr << "offset=" << offset << " attempts=" << reached
                          << " fired=" << injected << " wrote=" << wrote
                          << '\n';
                return fail("allocation refusal was not atomic or missed its calibrated offset");
            }
            if (!stagingClean) {
                std::cerr << "offset=" << offset << " binary=" << binary
                          << " left an output staging file\n";
                return fail("failed whole-file write leaked its staged output");
            }

            std::string afterFailure;
            if (!identitiesRolledBack || !readBytes(output, afterFailure)
                || afterFailure != sentinel
                || !callerRecordsUnchanged(adapter)) {
                std::cerr << "rollback offset=" << offset
                          << " identities=" << identitiesRolledBack
                          << " destination=" << (afterFailure == sentinel)
                          << " caller=" << callerRecordsUnchanged(adapter)
                          << " insert/attrib/block/point handles="
                          << adapter.insert.handle << '/'
                          << adapter.insert.attlist.front()->handle << '/'
                          << adapter.storage.blocks.front()->handle << '/'
                          << adapter.storage.blocks.front()->ent.front()->handle
                          << '\n';
                return fail("failure published identities or changed destination/caller records");
            }

            const bool retried = codec.write(&adapter, DRW::AC1027, binary);
            std::string retryBytes;
            const bool retryReadBytes = readBytes(output, retryBytes);
            const bool retryReadback = matchesReadback(output);
            if (!retried || !adapter.entitiesWritten || !adapter.objectsWritten
                || !retryReadBytes || retryBytes != expectedBytes
                || !retryReadback || hasStagedOutput(output)) {
                std::cerr << "retry offset=" << offset << " retried=" << retried
                          << " entities=" << adapter.entitiesWritten
                          << " objects=" << adapter.objectsWritten
                          << " bytesRead=" << retryReadBytes
                          << " bytesMatch=" << (retryBytes == expectedBytes)
                          << " readback=" << retryReadback
                          << " error=" << codec.getError() << '\n';
                const std::size_t mismatch = std::mismatch(
                    expectedBytes.begin(), expectedBytes.end(), retryBytes.begin(),
                    retryBytes.end()).first - expectedBytes.begin();
                std::cerr << "first byte mismatch=" << mismatch
                          << " expected size=" << expectedBytes.size()
                          << " actual size=" << retryBytes.size()
                          << " blockHandle=" << adapter.storage.blocks.front()->handle
                          << " pointHandle="
                          << adapter.storage.blocks.front()->ent.front()->handle
                          << " insertHandle=" << adapter.insert.handle
                          << " attribHandle=" << adapter.insert.attlist.front()->handle
                          << '\n';
                const std::size_t left = mismatch > 80u ? mismatch - 80u : 0u;
                std::cerr << "expected slice=["
                          << expectedBytes.substr(left, 240u) << "]\nactual slice=["
                          << retryBytes.substr(left, 240u) << "]\n";
                return fail("same-codec retry did not reproduce clean output");
            }
        }
        std::cout << "whole DXF write " << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", destination/caller rollback, retry/readback=PASS\n";
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
    }
    const auto marker = publicationTarget / "existing-destination";
    std::error_code cleanupError;
    std::filesystem::remove_all(publicationTarget, cleanupError);
    if (cleanupError)
        return fail("could not prepare publication-race destination");
    const std::string markerBytes = "destination created during write\n";
    dxfRW publicationCodec(publicationTarget.string().c_str());
    Export publicationAdapter(publicationCodec);
    if (!installGeneratedGroupGraph(publicationCodec,
                                    publicationAdapter.line.handle,
                                    publicationAdapter.circle.handle)
        || !installGeneratedImageDefinition(publicationCodec))
        return fail("could not install graph before publication-race control");
    publicationAdapter.finalizeHook = [&]() {
        std::error_code error;
        if (!std::filesystem::create_directory(publicationTarget, error)
            || error)
            return false;
        std::ofstream destination(marker, std::ios::binary);
        destination.write(markerBytes.data(),
                          static_cast<std::streamsize>(markerBytes.size()));
        destination.close();
        return destination.good();
    };
    const bool publicationAccepted = publicationCodec.write(
        &publicationAdapter, DRW::AC1027, false);
    std::string markerAfterRace;
    const bool markerPreserved = std::filesystem::is_directory(publicationTarget)
        && readBytes(marker, markerAfterRace) && markerAfterRace == markerBytes;
    if (publicationAccepted || !publicationAdapter.entitiesWritten
        || !publicationAdapter.objectsWritten || !markerPreserved
        || hasStagedOutput(publicationTarget)
        || publicationCodec.getError() != DRW::BAD_OPEN
        || !callerRecordsUnchanged(publicationAdapter))
        return fail("publication race overwrote or damaged the concurrent destination");

    std::filesystem::remove_all(publicationTarget, cleanupError);
    if (cleanupError)
        return fail("could not clear raced destination before same-codec retry");
    publicationAdapter.finalizeHook = {};
    const bool publicationRetry = publicationCodec.write(
        &publicationAdapter, DRW::AC1027, false);
    const bool publicationRetryReadback = matchesReadback(publicationTarget);
    if (!publicationRetry || !publicationAdapter.entitiesWritten
        || !publicationAdapter.objectsWritten || !publicationRetryReadback
        || hasStagedOutput(publicationTarget)
        || !callerRecordsUnchanged(publicationAdapter))
        return fail("whole-file publication failure did not allow corrected same-codec retry");
    std::filesystem::remove(publicationTarget, cleanupError);
    if (cleanupError)
        return fail("could not remove publication-race retry output");
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
    if (argc != 2) {
        std::cerr << "usage: dxf_write_allocation_transaction_tests SCRATCH_DIR\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
