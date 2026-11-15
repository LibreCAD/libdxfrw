// SPDX-License-Identifier: GPL-2.0-or-later
// Generated whole-operation DXF write allocation rollback/retry control.
#include "dx_iface.h"

#include <algorithm>
#include <array>
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
#include <utility>

class DxfDimensionDstyleTestAccess {
public:
    static void setOverrides(DRW_Dimension& dimension) {
        dimension.tolPlus = 0.25;
        dimension.tolMinus = 0.125;
        dimension.genTol = true;
        dimension.limGen = true;
        dimension.tolScale = 1.5;
        dimension.tolDecimals = 2;
        dimension.altTolDecimals = 3;
        dimension.textMove = 1;
        dimension.tolAlign = 2;
        dimension.tolZero = 4;
        dimension.altZero = 5;
        dimension.altTolZero = 6;
    }

    static bool matchesOverrides(const DRW_Dimension& dimension) {
        return dimension.tolPlus == 0.25
            && dimension.tolMinus == 0.125 && dimension.genTol
            && dimension.limGen && dimension.tolScale == 1.5
            && dimension.tolDecimals == 2
            && dimension.altTolDecimals == 3 && dimension.textMove == 1
            && dimension.tolAlign == 2 && dimension.tolZero == 4
            && dimension.altZero == 5 && dimension.altTolZero == 6;
    }

    static void printOverrides(const DRW_Dimension& dimension) {
        std::cerr << " dstyle=" << dimension.tolPlus << ','
                  << dimension.tolMinus << ',' << dimension.genTol << ','
                  << dimension.limGen << ',' << dimension.tolScale << ','
                  << dimension.tolDecimals << ','
                  << dimension.altTolDecimals << ',' << dimension.textMove
                  << ',' << dimension.tolAlign << ',' << dimension.tolZero
                  << ',' << dimension.altZero << ',' << dimension.altTolZero;
    }
};

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
constexpr std::uint32_t kLegacyDynamicConstraintDimensionHandle = 0xD94Cu;
constexpr std::uint32_t kDirectAttdefHandle = 0xD94Du;
constexpr std::uint32_t kPointCloudColorMapHandle = 0xD94Eu;
constexpr std::uint32_t kNavisworksModelDefHandle = 0xD94Fu;
constexpr std::uint32_t kPointCloudDefinitionHandle = 0xD950u;
constexpr std::uint32_t kPointCloudDefinitionExHandle = 0xD951u;
constexpr std::uint32_t kPointCloudReactorHandle = 0xD952u;
constexpr std::uint32_t kPointCloudReactorExHandle = 0xD953u;
constexpr std::uint32_t kPointCloudHandle = 0xD954u;
constexpr std::uint32_t kPointCloudExHandle = 0xD955u;
constexpr std::uint32_t kNavisworksModelHandle = 0xD956u;
constexpr std::uint32_t kTableStyleTextHandle = 0xD957u;
constexpr std::uint32_t kTableStyleHandle = 0xD958u;
constexpr std::uint32_t kCellStyleMapHandle = 0xD959u;
constexpr std::uint32_t kDirectViewHandle = 0xD974u;
constexpr std::uint32_t kDirectUcsHandle = 0xD975u;
constexpr std::uint32_t kDirectAppIdHandle = 0xD976u;
constexpr std::uint32_t kDirectVportHandle = 0xD977u;
constexpr std::uint32_t kDirectLayerHandle = 0xD978u;
constexpr std::uint32_t kDirectTextstyleHandle = 0xD979u;
constexpr std::uint32_t kDirectDimstyleHandle = 0xD97Au;
constexpr std::uint32_t kDirectWipeoutHandle = 0xD97Bu;
constexpr std::uint32_t kDirectPlaceholderHandle = 0xD97Cu;
constexpr std::uint32_t kDirectMLeaderStyleHandle = 0xD97Du;
constexpr std::uint32_t kDirectGeoDataHandle = 0xD97Eu;
constexpr std::uint32_t kDirectLayoutHandle = 0xD97Fu;
constexpr std::uint32_t kDirectRawEntityHandle = 0xD980u;
constexpr std::uint32_t kBlockMLeaderHandle = 0xD981u;
constexpr std::uint32_t kBlockMLeaderStyleHandle = 0xD982u;
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
    custom.m_entries.push_back(
        {"WHOLE_OPERATION_MLEADERSTYLE", kDirectMLeaderStyleHandle,
         DictionaryEdge::SoftOwner});
    custom.m_entries.push_back(
        {"WHOLE_OPERATION_BLOCK_MLEADERSTYLE", kBlockMLeaderStyleHandle,
         DictionaryEdge::SoftOwner});
    custom.m_entries.push_back(
        {"WHOLE_OPERATION_LAYOUT", kDirectLayoutHandle,
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
        line.layer = "WHOLE_OPERATION_LAYER";
        line.basePoint = DRW_Coord(1.25, -2.5, 3.75);
        line.secPoint = DRW_Coord(4.5, 5.25, -6.0);
        DRW_Layer directLayer;
        directLayer.handle = kDirectLayerHandle;
        directLayer.name = "WHOLE_OPERATION_LAYER";
        directLayer.color = 3;
        directLayer.lineType = "CONTINUOUS";
        directLayer.lWeight = DRW_LW_Conv::width07;
        storage.layers.push_back(directLayer);
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
        wipeout.handle = kDirectWipeoutHandle;
        wipeout.basePoint = DRW_Coord(21.0, 22.0, 23.0);
        wipeout.secPoint = DRW_Coord(4.0, 0.0, 0.0);
        wipeout.vVector = DRW_Coord(0.0, 3.0, 0.0);
        wipeout.sizeu = 320.0;
        wipeout.sizev = 240.0;
        wipeout.clip = 1;
        wipeout.brightness = 60;
        wipeout.contrast = 70;
        wipeout.fade = 10;
        wipeout.m_clipBoundaryType = 2;
        wipeout.clipPath = {DRW_Coord(0.0, 0.0), DRW_Coord(32.0, 0.0),
            DRW_Coord(24.0, 18.0), DRW_Coord(0.0, 16.0)};
        wipeout.clipMode = true;
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

        directAttdef.handle = kDirectAttdefHandle;
        directAttdef.tag = "WHOLE_OPERATION_DIRECT_ATTDEF";
        directAttdef.prompt = "Long direct attribute prompt for allocation rollback";
        directAttdef.text = "Long direct attribute default value for retry";
        directAttdef.basePoint = DRW_Coord(3.0, 5.0, 0.0);
        directAttdef.height = 2.25;
        directAttdef.style = "STANDARD";
        directAttdef.attribFlags = 1;
        directAttdef.m_fieldLength = 48;

        DRW_Textstyle tableStyleText;
        tableStyleText.handle = kTableStyleTextHandle;
        tableStyleText.name = "WHOLE_OPERATION_TABLE_TEXT";
        storage.textStyles.push_back(tableStyleText);
        tableStyle.handle = kTableStyleHandle;
        tableStyle.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        tableStyle.m_name = "WHOLE_OPERATION_TABLE_STYLE";
        for (int rowIndex = 0; rowIndex < 3; ++rowIndex) {
            DRW_TableStyleRowStyle row;
            row.m_textStyleHandle = kTableStyleTextHandle;
            row.m_textHeight = 1.25 + rowIndex;
            row.m_textAlignment = 5;
            row.m_textColor = rowIndex + 1;
            row.m_fillColor = rowIndex + 4;
            row.m_hasBackgroundColor = rowIndex != 1;
            for (int borderIndex = 0; borderIndex < 6; ++borderIndex) {
                DRW_TableStyleBorder border;
                border.m_edgeFlags = 1 << borderIndex;
                border.m_lineWeight = borderIndex;
                border.m_visible = borderIndex % 2;
                border.m_color = borderIndex + 1;
                row.m_borders.push_back(border);
            }
            tableStyle.m_rowStyles.push_back(std::move(row));
        }
        cellStyleMap.handle = kCellStyleMapHandle;
        cellStyleMap.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        DRW_TableStyleCellStyle mappedStyle;
        mappedStyle.m_type = 5;
        mappedStyle.m_hasData = true;
        mappedStyle.m_id = 101;
        mappedStyle.m_styleClass = 2;
        mappedStyle.m_name = "WHOLE_OPERATION_CELL_STYLE";
        mappedStyle.m_contentFormat.m_cellAlignment = 1;
        mappedStyle.m_contentFormat.m_textStyleHandle = kTableStyleTextHandle;
        mappedStyle.m_contentFormat.m_textHeight = 1.75;
        DRW_TableStyleBorder mappedBorder;
        mappedBorder.m_edgeFlags = 1;
        mappedBorder.m_visible = 1;
        mappedStyle.m_borders.push_back(mappedBorder);
        cellStyleMap.m_cellStyles.push_back(std::move(mappedStyle));

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
        DxfDimensionDstyleTestAccess::setOverrides(
            dynamicConstraintDimension);
        legacyDynamicConstraintDimension.handle =
            kLegacyDynamicConstraintDimensionHandle;
        legacyDynamicConstraintDimension.layer =
            "*LayerNameForDynamicConstraint";
        legacyDynamicConstraintDimension.setDefPoint(
            DRW_Coord{18.0, 19.0, 20.0});
        legacyDynamicConstraintDimension.setTextPoint(
            DRW_Coord{18.5, 19.5, 0.0});
        legacyDynamicConstraintDimension.setText(
            "whole-operation legacy dynamic constraint");
        legacyDynamicConstraintDimension.setStyle("STANDARD");
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
        DRW_View directView;
        directView.handle = kDirectViewHandle;
        directView.name = "WHOLE_OPERATION_DIRECT_VIEW";
        directView.size = DRW_Coord(320.0, 200.0, 0.0);
        directView.center = DRW_Coord(12.0, 18.0, 0.0);
        directView.viewDirectionFromTarget = DRW_Coord(0.0, 0.0, 1.0);
        directView.targetPoint = DRW_Coord(-4.0, 8.0, 12.0);
        directView.lensLen = 42.0;
        directView.twistAngle = 15.0;
        directView.viewMode = 1;
        directView.renderMode = 2;
        directView.cameraPlottable = true;
        storage.views.push_back(directView);
        DRW_UCS directUcs;
        directUcs.handle = kDirectUcsHandle;
        directUcs.name = "WHOLE_OPERATION_DIRECT_UCS";
        directUcs.origin = DRW_Coord(4.0, 5.0, 6.0);
        directUcs.xAxisDirection = DRW_Coord(1.0, 0.0, 0.0);
        directUcs.yAxisDirection = DRW_Coord(0.0, 1.0, 0.0);
        directUcs.orthoOrigin = DRW_Coord(1.0, 2.0, 3.0);
        directUcs.elevation = 2.5;
        directUcs.orthographicOptions = {
            {2, DRW_Coord(1.0, 2.0, 3.0)},
            {5, DRW_Coord(4.0, 5.0, 6.0)}};
        storage.ucsList.push_back(directUcs);
        DRW_AppId directAppId;
        directAppId.handle = kDirectAppIdHandle;
        directAppId.name = "WHOLE_OPERATION_APPID";
        storage.appIds.push_back(directAppId);
        DRW_Vport activeVport;
        activeVport.name = "*ACTIVE";
        storage.VPorts.push_back(activeVport);
        DRW_Vport directVport;
        directVport.handle = kDirectVportHandle;
        directVport.name = "WHOLE_OPERATION_DIRECT_VPORT";
        directVport.center = DRW_Coord(12.0, 14.0, 0.0);
        directVport.snapBase = DRW_Coord(1.0, 2.0, 0.0);
        directVport.snapSpacing = DRW_Coord(3.0, 4.0, 0.0);
        directVport.gridSpacing = DRW_Coord(5.0, 6.0, 0.0);
        directVport.viewDir = DRW_Coord(0.0, 0.0, 1.0);
        directVport.viewTarget = DRW_Coord(7.0, 8.0, 9.0);
        directVport.height = 75.0;
        directVport.ratio = 1.6;
        directVport.twistAngle = 30.0;
        directVport.snap = 1;
        directVport.grid = 1;
        storage.VPorts.push_back(directVport);
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
        mtext.m_backgroundFlags = 1;
        mtext.m_backgroundScale = 1.35;
        mtext.m_backgroundColor = 3;
        mtext.m_backgroundColorTrue = 0x123456;
        mtext.m_backgroundColorName = "WHOLE_OPERATION_BACKGROUND_COLOR";
        mtext.m_backgroundTransparency = 0x02000000;
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
        pointCloudColorMap.handle = kPointCloudColorMapHandle;
        pointCloudColorMap.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        pointCloudColorMap.m_classVersion = 4;
        pointCloudColorMap.m_defaultIntensityColorScheme = "intensity ramp";
        pointCloudColorMap.m_defaultElevationColorScheme = "elevation ramp";
        pointCloudColorMap.m_defaultClassificationColorScheme = "classification ramp";
        pointCloudColorMap.m_colorRamps = {
            {2, 3u, {"ground", "canopy", "roof"}},
            {3, 2u, {"low", "high"}}};
        pointCloudColorMap.m_colorRampCount =
            static_cast<std::uint32_t>(pointCloudColorMap.m_colorRamps.size());
        pointCloudColorMap.m_classificationColorRamps = {
            {4, 2u, {"road", "rail"}}};
        pointCloudColorMap.m_classificationColorRampCount =
            static_cast<std::uint32_t>(pointCloudColorMap.m_classificationColorRamps.size());
        navisworksModelDef.handle = kNavisworksModelDefHandle;
        navisworksModelDef.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        navisworksModelDef.m_flags = 3;
        navisworksModelDef.m_path = "models/" + std::string(160u, 'n') + ".nwd";
        navisworksModelDef.m_status = true;
        navisworksModelDef.m_minExtent = DRW_Coord(-1.5, -2.5, -3.5);
        navisworksModelDef.m_maxExtent = DRW_Coord(4.5, 5.5, 6.5);
        navisworksModelDef.m_hostDrawingVisibility = true;
        pointCloudDefinition.handle = kPointCloudDefinitionHandle;
        pointCloudDefinition.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        pointCloudDefinition.m_kind = DRW_PointCloudDef::Definition;
        pointCloudDefinition.m_classVersion = 1;
        pointCloudDefinition.m_sourceFilename =
            "pointcloud/" + std::string(96u, 'a') + ".rcs";
        pointCloudDefinition.m_isLoaded = true;
        pointCloudDefinition.m_pointCount = 123456u;
        pointCloudDefinition.m_extentsMin = DRW_Coord(-1.0, -2.0, -3.0);
        pointCloudDefinition.m_extentsMax = DRW_Coord(4.0, 5.0, 6.0);
        pointCloudDefinitionEx.handle = kPointCloudDefinitionExHandle;
        pointCloudDefinitionEx.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        pointCloudDefinitionEx.m_kind = DRW_PointCloudDef::DefinitionEx;
        pointCloudDefinitionEx.m_classVersion = 2;
        pointCloudDefinitionEx.m_sourceFilename =
            "pointcloud-ex/" + std::string(80u, 'b') + ".rcs";
        pointCloudDefinitionEx.m_isLoaded = true;
        pointCloudDefinitionEx.m_pointCount = 654321u;
        pointCloudDefinitionEx.m_extentsMin = DRW_Coord(-7.0, -8.0, -9.0);
        pointCloudDefinitionEx.m_extentsMax = DRW_Coord(10.0, 11.0, 12.0);
        pointCloudReactor.handle = kPointCloudReactorHandle;
        pointCloudReactor.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        pointCloudReactor.m_kind = DRW_PointCloudDef::Reactor;
        pointCloudReactor.m_classVersion = 3;
        pointCloudReactorEx.handle = kPointCloudReactorExHandle;
        pointCloudReactorEx.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        pointCloudReactorEx.m_kind = DRW_PointCloudDef::ReactorEx;
        pointCloudReactorEx.m_classVersion = 4;
        pointCloud.handle = kPointCloudHandle;
        pointCloud.classVersion = 2u;
        pointCloud.origin = DRW_Coord(11.0, 12.0, 13.0);
        pointCloud.savedFilename = "clouds/whole-operation.rcs";
        pointCloud.sourceFileCount = 2;
        pointCloud.sourceFiles = {"clouds/part-a.rcs", "clouds/part-b.rcs"};
        pointCloud.extentsMin = DRW_Coord(-10.0, -20.0, -30.0);
        pointCloud.extentsMax = DRW_Coord(40.0, 50.0, 60.0);
        pointCloud.pointCount = 987654u;
        pointCloud.ucsName = "WHOLE_OPERATION_POINTCLOUD_UCS";
        pointCloud.ucsOrigin = DRW_Coord(1.0, 2.0, 3.0);
        pointCloud.ucsXDirection = DRW_Coord(0.0, 1.0, 0.0);
        pointCloud.ucsYDirection = DRW_Coord(-1.0, 0.0, 0.0);
        pointCloud.ucsZDirection = DRW_Coord(0.0, 0.0, 1.0);
        pointCloud.definitionHandle = kPointCloudDefinitionHandle;
        pointCloud.reactorHandle = kPointCloudReactorHandle;
        pointCloudEx.handle = kPointCloudExHandle;
        pointCloudEx.classVersion = 3u;
        pointCloudEx.extentsMin = DRW_Coord(-100.0, -200.0, -300.0);
        pointCloudEx.extentsMax = DRW_Coord(400.0, 500.0, 600.0);
        pointCloudEx.ucsOrigin = DRW_Coord(7.0, 8.0, 9.0);
        pointCloudEx.ucsXDirection = DRW_Coord(0.0, 1.0, 0.0);
        pointCloudEx.ucsYDirection = DRW_Coord(-1.0, 0.0, 0.0);
        pointCloudEx.ucsZDirection = DRW_Coord(0.0, 0.0, 1.0);
        pointCloudEx.isLocked = true;
        pointCloudEx.definitionHandle = kPointCloudDefinitionExHandle;
        pointCloudEx.reactorHandle = kPointCloudReactorExHandle;
        pointCloudEx.name = "whole-operation pointcloud-ex";
        pointCloudEx.showIntensity = true;
        pointCloudEx.showCropping = true;
        pointCloudEx.croppingCount = 1;
        pointCloudEx.stylizationType = 2;
        pointCloudEx.intensityColorScheme = "intensity";
        pointCloudEx.currentColorScheme = "current";
        pointCloudEx.classificationColorScheme = "classification";
        pointCloudEx.elevationMin = -5.5;
        pointCloudEx.elevationMax = 25.5;
        pointCloudEx.intensityMin = 0.0;
        pointCloudEx.intensityMax = 65535.0;
        pointCloudEx.intensityOutOfRangeBehavior = 1;
        pointCloudEx.elevationOutOfRangeBehavior = 2;
        pointCloudEx.elevationApplyToFixedRange = true;
        pointCloudEx.intensityAsGradient = true;
        pointCloudEx.elevationAsGradient = true;
        DRW_PointCloudExCropping cropping;
        cropping.type = 1;
        cropping.isInside = true;
        cropping.isInverted = false;
        cropping.cropPlane = DRW_Coord(1.0, 2.0, 3.0);
        cropping.cropXDirection = DRW_Coord(1.0, 0.0, 0.0);
        cropping.cropYDirection = DRW_Coord(0.0, 1.0, 0.0);
        cropping.pointCount = 3;
        cropping.points = {DRW_Coord(1.0, 2.0, 3.0),
            DRW_Coord(4.0, 5.0, 6.0), DRW_Coord(7.0, 8.0, 9.0)};
        pointCloudEx.croppings.push_back(cropping);
        navisworksModel.handle = kNavisworksModelHandle;
        navisworksModel.flags = 5u;
        navisworksModel.definitionHandle = kNavisworksModelDefHandle;
        navisworksModel.transform = {1.0, 0.0, 0.0, 2.0,
            0.0, 1.0, 0.0, 3.0, 0.0, 0.0, 1.0, 4.0,
            5.0, 6.0, 7.0, 1.0};
        navisworksModel.unitFactor = 0.25;
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
        geoData.handle = kDirectGeoDataHandle;
        geoData.parentHandle = 0x2Fu;
        geoData.m_version = 2;
        geoData.m_hostBlockHandle = 0x17u;
        geoData.m_designPoint = DRW_Coord(100.0, 200.0, 0.0);
        geoData.m_referencePoint = DRW_Coord(10.0, 20.0, 0.0);
        geoData.m_coordinateSystemDefinition = "LOCAL_GRID_PROFILE";
        geoData.m_geoRssTag = "LOCAL_GEODATA_TAG";
        layout.handle = kDirectLayoutHandle;
        layout.parentHandle = kCustomDictionaryHandle;
        layout.name = "WHOLE_OPERATION_LAYOUT";
        layout.tabOrder = 4;
        layout.layoutFlags = 1;
        layout.paperWidth = 210.0;
        layout.paperHeight = 297.0;
        layout.limMinX = -2.0;
        layout.limMinY = -3.0;
        layout.limMaxX = 20.0;
        layout.limMaxY = 30.0;
        layout.paperSize = "ISO_A4_(210.00_x_297.00_MM)";
        layout.currentStyleSheet = "WHOLE_OPERATION_STYLE";
        rawEntity.name = "WHOLE_OPERATION_RAW_ENTITY";
        rawEntity.handle = kDirectRawEntityHandle;
        rawEntity.parentHandle = DRW::DxfModelSpaceBlockRecordHandle;
        rawEntity.m_version = DRW::AC1027;
        rawEntity.groups.emplace_back(5, UTF8STRING("D980"));
        rawEntity.groups.emplace_back(330, UTF8STRING("1F"));
        rawEntity.groups.emplace_back(100, UTF8STRING("AcDbEntity"));
        rawEntity.groups.emplace_back(8, UTF8STRING("0"));
        rawEntity.groups.emplace_back(100, UTF8STRING("AcDbWholeOperation"));
        rawEntity.groups.emplace_back(90, std::int32_t{77});
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
        placeholder.handle = kDirectPlaceholderHandle;
        placeholder.parentHandle = kCustomDictionaryHandle;
        lineType.handle = kLineTypeHandle;
        lineType.name = "WHOLE_OPERATION_LTYPE";
        lineType.desc = "whole-operation line type";
        lineType.path = {10.0, -5.0};
        lineType.size = 2;
        lineType.length = 15.0;
        storage.lineTypes.push_back(lineType);
        DRW_Textstyle textstyle;
        textstyle.handle = kDirectTextstyleHandle;
        textstyle.name = "WHOLE_OPERATION_TEXT_STYLE";
        textstyle.font = "simplex.shx";
        textstyle.height = 0.0;
        textstyle.width = 0.9;
        textstyle.oblique = 0.1;
        textstyle.genFlag = 2;
        textstyle.lastHeight = 2.0;
        storage.textStyles.push_back(textstyle);
        DRW_Dimstyle dimstyle;
        dimstyle.handle = kDirectDimstyleHandle;
        dimstyle.name = "WHOLE_OPERATION_DIMSTYLE";
        dimstyle.dimtxsty = textstyle.name;
        dimstyle.dimtxstyH.ref = textstyle.handle;
        dimstyle.dimpost = "<> mm";
        dimstyle.dimscale = 2.0;
        dimstyle.dimasz = 2.75;
        dimstyle.dimtxt = 3.5;
        dimstyle.dimdec = 3;
        dimstyle.dimazin = 2;
        storage.dimStyles.push_back(dimstyle);
        multiLeaderStyle.handle = kDirectMLeaderStyleHandle;
        multiLeaderStyle.parentHandle = kCustomDictionaryHandle;
        multiLeaderStyle.contentType = 2;
        multiLeaderStyle.description = "whole-operation MLEADERSTYLE";
        multiLeaderStyle.textDefault = "callout";
        multiLeaderStyle.textStyleHandle.ref = kDirectTextstyleHandle;
        multiLeaderStyle.textHeight = 2.25;
        multiLeaderStyle.arrowHeadSize = 1.75;
        multiLeaderStyle.maxLeaderPoints = 5;
        blockMultiLeaderStyle.handle = kBlockMLeaderStyleHandle;
        blockMultiLeaderStyle.parentHandle = kCustomDictionaryHandle;
        blockMultiLeaderStyle.contentType = 1;
        blockMultiLeaderStyle.description =
            "whole-operation block MLEADERSTYLE";
        blockMultiLeaderStyle.blockHandle.ref = kInsertBlockHandle;
        blockMultiLeaderStyle.blockScale = DRW_Coord{1.25, 1.5, 1.75};
        blockMultiLeaderStyle.blockScaleEnabled = true;
        typedXRecord.handle = kTypedXRecordHandle;
        typedXRecord.parentHandle = kCustomDictionaryHandle;
        typedXRecord.m_values.emplace_back(
            1, UTF8STRING("whole-operation typed XRECORD"));
        typedXRecord.m_values.emplace_back(90, std::int32_t{42});
        multiLeader.handle = kMultiLeaderHandle;
        multiLeader.styleHandle.ref = kDirectMLeaderStyleHandle;
        multiLeader.styleContentType = 2u;
        multiLeader.context.hasTextContents = true;
        multiLeader.context.textLabel = "whole-operation multi-leader text";
        multiLeader.context.contentBasePoint = DRW_Coord{2.0, 3.0, 4.0};
        multiLeader.context.textLocation = DRW_Coord{5.0, 6.0, 7.0};
        multiLeader.context.textHeight = 1.5;
        DRW_MLeaderRoot multiLeaderRoot;
        multiLeaderRoot.connectionPoint = DRW_Coord{1.0, 2.0, 3.0};
        multiLeaderRoot.direction = DRW_Coord{0.0, 1.0, 0.0};
        multiLeaderRoot.leaderIndex = 2;
        multiLeaderRoot.landingDistance = 4.5;
        DRW_MLeaderLeaderLine multiLeaderLine;
        multiLeaderLine.points = {DRW_Coord{1.0, 2.0, 3.0},
            DRW_Coord{4.0, 5.0, 6.0}, DRW_Coord{7.0, 8.0, 9.0}};
        multiLeaderLine.segmentIndex = 1;
        multiLeaderLine.leaderLineIndex = 3;
        multiLeaderLine.leaderType = 1;
        multiLeaderLine.color = 4;
        multiLeaderLine.lineWeight = 25;
        multiLeaderLine.arrowSize = 2.25;
        multiLeaderRoot.leaderLines.push_back(multiLeaderLine);
        multiLeader.context.roots.push_back(multiLeaderRoot);
        blockMultiLeader.handle = kBlockMLeaderHandle;
        blockMultiLeader.styleHandle.ref = kBlockMLeaderStyleHandle;
        blockMultiLeader.styleContentType = 1u;
        blockMultiLeader.context.hasContentsBlock = true;
        blockMultiLeader.context.blockTableRecordHandle.ref =
            kInsertBlockHandle;
        blockMultiLeader.context.blockNormal = DRW_Coord{0.0, 0.0, 1.0};
        blockMultiLeader.context.blockLocation =
            DRW_Coord{21.0, 22.0, 23.0};
        blockMultiLeader.context.blockScale =
            DRW_Coord{1.25, 1.5, 1.75};
        blockMultiLeader.context.blockRotation = 0.625;
        blockMultiLeader.context.blockColor = 4;
        blockMultiLeader.context.blockTransform = {
            1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
            0.0, 0.0, 1.0, 0.0, 21.0, 22.0, 23.0, 1.0};
        DRW_MLeaderRoot blockMultiLeaderRoot;
        blockMultiLeaderRoot.connectionPoint =
            DRW_Coord{24.0, 25.0, 26.0};
        blockMultiLeaderRoot.direction = DRW_Coord{0.0, 0.0, 1.0};
        blockMultiLeaderRoot.leaderIndex = 4;
        DRW_MLeaderLeaderLine blockMultiLeaderLine;
        blockMultiLeaderLine.points = {
            DRW_Coord{24.0, 25.0, 26.0}, DRW_Coord{27.0, 28.0, 29.0}};
        blockMultiLeaderLine.segmentIndex = 2;
        blockMultiLeaderLine.leaderLineIndex = 5;
        blockMultiLeaderLine.leaderType = 1;
        blockMultiLeaderLine.breaks.push_back({
            DRW_Coord{30.0, 31.0, 32.0}, DRW_Coord{33.0, 34.0, 35.0}});
        blockMultiLeaderRoot.leaderLines.push_back(blockMultiLeaderLine);
        blockMultiLeaderRoot.breaks.push_back({
            DRW_Coord{36.0, 37.0, 38.0}, DRW_Coord{39.0, 40.0, 41.0}});
        blockMultiLeader.context.roots.push_back(blockMultiLeaderRoot);
        DRW_MLeaderRoot blockMultiLeaderSecondRoot;
        blockMultiLeaderSecondRoot.connectionPoint =
            DRW_Coord{42.0, 43.0, 44.0};
        blockMultiLeaderSecondRoot.direction = DRW_Coord{1.0, 0.0, 0.0};
        blockMultiLeaderSecondRoot.leaderIndex = 6;
        DRW_MLeaderLeaderLine blockMultiLeaderSecondLine;
        blockMultiLeaderSecondLine.points = {
            DRW_Coord{42.0, 43.0, 44.0}, DRW_Coord{45.0, 46.0, 47.0}};
        blockMultiLeaderSecondLine.segmentIndex = 3;
        blockMultiLeaderSecondLine.leaderLineIndex = 7;
        blockMultiLeaderSecondLine.leaderType = 1;
        blockMultiLeaderSecondRoot.leaderLines.push_back(
            blockMultiLeaderSecondLine);
        blockMultiLeader.context.roots.push_back(blockMultiLeaderSecondRoot);
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
            && check("WIPEOUT", output.writeWipeout(&wipeout))
            && check("MESH", output.writeMesh(&mesh))
            && check("SPLINE", output.writeSpline(&spline))
            && check("HATCH", output.writeHatch(&hatch))
            && check("LEADER", output.writeLeader(&leader))
            && check("MLINE", output.writeMLine(&mline))
            && check("POLYLINE", output.writePolyline(&polyline3))
            && check("INSERT", output.writeInsert(&insert))
            && check("block MULTILEADER",
                     output.writeMultiLeader(&blockMultiLeader))
            && check("POINTCLOUD", output.writePointCloud(&pointCloud))
            && check("POINTCLOUDEX", output.writePointCloudEx(&pointCloudEx))
            && check("NAVISWORKSMODEL", output.writeNavisworksModel(&navisworksModel))
            && check("ATTDEF", output.writeAttdef(
                &directAttdef, DRW::DxfModelSpaceBlockRecordHandle))
            && check("3DFACE", output.write3dface(&face))
            && check("DIMENSION", output.writeDimension(&dimension))
            && check("DYNAMIC_CONSTRAINT_DIMENSION",
                     output.writeDimension(&dynamicConstraintDimension))
            && check("LEGACY_DYNAMIC_CONSTRAINT_DIMENSION",
                     output.writeDimension(&legacyDynamicConstraintDimension))
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
            && check("NURBSSURFACE", output.writeSurface(&nurbsSurface))
            && check("RAW_ENTITY", output.writeRawDxfEntity(
                &rawEntity, DRW::DxfModelSpaceBlockRecordHandle));
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
            && output.writeGeoData(&geoData)
            && output.writeMaterial(&material)
            && output.writePointCloudColorMap(&pointCloudColorMap)
            && output.writeNavisworksModelDef(&navisworksModelDef)
            && output.writePointCloudDef(&pointCloudDefinition)
            && output.writePointCloudDef(&pointCloudDefinitionEx)
            && output.writePointCloudDef(&pointCloudReactor)
            && output.writePointCloudDef(&pointCloudReactorEx)
            && output.writeRawDxfObject(&object)
            && output.writeDataLink(&mainDataLink)
            && output.writeDataLink(&targetAlpha)
            && output.writeDataLink(&targetBeta);
        const bool tableStyleWritten = objectsWritten
            && output.writeTableStyle(&tableStyle, storage.textStyles);
        const bool cellStyleMapWritten = tableStyleWritten
            && output.writeCellStyleMap(&cellStyleMap);
        const bool mleaderStyleWritten = cellStyleMapWritten
            && output.writeMLeaderStyle(&multiLeaderStyle)
            && output.writeMLeaderStyle(&blockMultiLeaderStyle);
        const bool layoutWritten = mleaderStyleWritten
            && output.writeLayout(&layout);
        objectsWritten = layoutWritten
            && output.writeAcDbPlaceholder(&placeholder);
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
    DRW_Wipeout wipeout;
    DRW_AcDbPlaceholder placeholder;
    DRW_MLeaderStyle multiLeaderStyle;
    DRW_MLeaderStyle blockMultiLeaderStyle;
    DRW_GeoData geoData;
    DRW_Layout layout;
    DRW_RawDxfObject rawEntity;
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
    DRW_Attdef directAttdef;
    DRW_TableStyle tableStyle;
    DRW_CellStyleMap cellStyleMap;
    DRW_Dimension dynamicConstraintDimension;
    DRW_Dimension legacyDynamicConstraintDimension;
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
    DRW_MLeader blockMultiLeader;
    DRW_Material material;
    DRW_PointCloudColorMap pointCloudColorMap;
    DRW_NavisworksModelDef navisworksModelDef;
    DRW_PointCloudDef pointCloudDefinition;
    DRW_PointCloudDef pointCloudDefinitionEx;
    DRW_PointCloudDef pointCloudReactor;
    DRW_PointCloudDef pointCloudReactorEx;
    DRW_PointCloud pointCloud;
    DRW_PointCloudEx pointCloudEx;
    DRW_NavisworksModel navisworksModel;
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

class EmbeddedAttributeMTextExport final : public dx_iface {
public:
    explicit EmbeddedAttributeMTextExport(dxfRW& output) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &output;
        insert.handle = 0xDA01u;
        insert.name = "WHOLE_OPERATION_ATTRIBUTE_MTEXT_BLOCK";
        insert.basePoint = DRW_Coord{1.0, 2.0, 0.0};
        auto attribute = std::make_shared<DRW_Attrib>();
        attribute->handle = 0xDA02u;
        attribute->tag = "R2018_MULTILINE_ATTRIBUTE";
        attribute->text = "attribute fallback";
        attribute->basePoint = DRW_Coord{3.0, 4.0, 0.0};
        attribute->secPoint = DRW_Coord{5.0, 6.0, 0.0};
        attribute->height = 2.5;
        attribute->style = "STANDARD";
        attribute->m_attributeType = 2;
        attribute->mtext = std::make_unique<DRW_MText>();
        attribute->mtext->text = "embedded attribute first paragraph\\Psecond paragraph";
        attribute->mtext->basePoint = DRW_Coord{3.0, 4.0, 0.0};
        attribute->mtext->height = 2.5;
        attribute->mtext->style = "STANDARD";
        insert.attlist.push_back(std::move(attribute));

        attdef.handle = 0xDA03u;
        attdef.tag = "WHOLE_OPERATION_R2018_MULTILINE_ATTDEF";
        attdef.prompt = "R2018 multiline definition prompt";
        attdef.text = "definition fallback";
        attdef.basePoint = DRW_Coord{7.0, 8.0, 0.0};
        attdef.secPoint = DRW_Coord{9.0, 10.0, 0.0};
        attdef.height = 3.5;
        attdef.style = "STANDARD";
        attdef.mtext = std::make_unique<DRW_MText>();
        attdef.mtext->text = "definition first paragraph\\Psecond paragraph";
        attdef.mtext->basePoint = DRW_Coord{7.0, 8.0, 0.0};
        attdef.mtext->height = 3.5;
        attdef.mtext->style = "STANDARD";
    }

    void writeEntities() override {
        entitiesWritten = dxfW != nullptr && dxfW->writeInsert(&insert)
            && dxfW->writeAttdef(
                &attdef, DRW::DxfModelSpaceBlockRecordHandle);
    }

    DRW_Insert insert;
    DRW_Attdef attdef;
    bool entitiesWritten = false;
    dx_data storage;
};

class LegacyCompoundEntitiesExport final : public dx_iface {
public:
    explicit LegacyCompoundEntitiesExport(dxfRW& output) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &output;
        polyline.handle = 0xDA30u;
        polyline.flags = 0;
        polyline.basePoint = DRW_Coord{0.0, 0.0, 2.5};
        DRW_Vertex firstVertex(19.0, 20.0, 0.0, 0.375);
        firstVertex.handle = 0xDA31u;
        DRW_Vertex secondVertex(22.0, 24.0, 0.0, 0.0);
        secondVertex.handle = 0xDA32u;
        polyline.addVertex(firstVertex);
        polyline.addVertex(secondVertex);
        insert.handle = 0xDA20u;
        insert.name = "LEGACY_ATTRIBUTED_INSERT_BLOCK";
        insert.basePoint = DRW_Coord{11.0, 12.0, 0.0};
        insert.colcount = 2;
        insert.rowcount = 3;
        insert.colspace = 4.5;
        insert.rowspace = 5.5;
        for (unsigned i = 0; i < 2u; ++i) {
            auto attribute = std::make_shared<DRW_Attrib>();
            attribute->handle = 0xDA21u + i;
            attribute->tag = i == 0u ? "LEGACY_TAG_A" : "LEGACY_TAG_B";
            attribute->text = i == 0u ? "legacy value alpha" : "legacy value beta";
            attribute->basePoint = DRW_Coord{13.0 + i, 14.0 + i, 0.0};
            attribute->secPoint = DRW_Coord{15.0 + i, 16.0 + i, 0.0};
            attribute->height = 2.0 + i;
            attribute->style = "STANDARD";
            insert.attlist.push_back(std::move(attribute));
        }
    }

    void writeEntities() override {
        entitiesWritten = dxfW != nullptr && dxfW->writePolyline(&polyline)
            && dxfW->writeInsert(&insert);
    }

    DRW_Polyline polyline;
    DRW_Insert insert;
    bool entitiesWritten = false;
    dx_data storage;
};

class PolylineTopologyExport final : public dx_iface {
public:
    enum class Topology {
        ThreeDimensional,
        ThreeDimensionalWithIdentifiers,
        PolygonMesh,
        RectangularPolygonMesh,
        Polyface
    };

    PolylineTopologyExport(dxfRW& output, Topology topology)
        : topology(topology) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &output;
        polyline.handle = 0xDB00u;
        polyline.basePoint = DRW_Coord{0.0, 0.0, 2.5};
        if (topology == Topology::ThreeDimensional
            || topology == Topology::ThreeDimensionalWithIdentifiers) {
            const bool identifiers =
                topology == Topology::ThreeDimensionalWithIdentifiers;
            polyline.flags = 8;
            addVertex(0xDB01u, 1.0, 2.0, 3.0,
                      DRW_Vertex::DwgSubtype::Vertex3D, 32,
                      identifiers ? 101 : 0);
            addVertex(0xDB02u, 4.0, 5.0, 6.0,
                      DRW_Vertex::DwgSubtype::Vertex3D, 32,
                      identifiers ? 202 : 0);
        } else if (topology == Topology::PolygonMesh
                   || topology == Topology::RectangularPolygonMesh) {
            polyline.flags = 16 | 32;
            polyline.vertexcount = topology == Topology::PolygonMesh ? 2 : 3;
            polyline.facecount = 2;
            if (topology == Topology::RectangularPolygonMesh)
                polyline.flags |= 1;
            const unsigned vertexCount = topology == Topology::PolygonMesh
                ? 4u : 6u;
            for (unsigned i = 0; i != vertexCount; ++i) {
                const double column = i % static_cast<unsigned>(polyline.vertexcount);
                const double row = i / static_cast<unsigned>(polyline.vertexcount);
                addVertex(0xDB01u + i, 1.0 + column, 2.0 + row,
                          3.0 + 3.0 * i,
                          DRW_Vertex::DwgSubtype::Mesh, 64);
            }
        } else {
            polyline.flags = 64;
            polyline.vertexcount = 5;
            polyline.facecount = 2;
            for (unsigned i = 0; i != 5; ++i)
                addVertex(0xDB01u + i, 1.0 + i, 2.0 + i, 3.0 + i,
                          DRW_Vertex::DwgSubtype::Polyface, 64);
            DRW_Vertex firstFace;
            firstFace.handle = 0xDB06u;
            firstFace.setDwgSubtype(DRW_Vertex::DwgSubtype::PolyfaceFace);
            firstFace.vindex1 = -1;
            firstFace.vindex2 = 2;
            firstFace.vindex3 = -3;
            polyline.addVertex(firstFace);
            DRW_Vertex secondFace;
            secondFace.handle = 0xDB07u;
            secondFace.setDwgSubtype(DRW_Vertex::DwgSubtype::PolyfaceFace);
            secondFace.vindex1 = 2;
            secondFace.vindex2 = -4;
            secondFace.vindex3 = 5;
            polyline.addVertex(secondFace);
        }
    }

    void writeEntities() override {
        written = dxfW != nullptr && dxfW->writePolyline(&polyline);
    }
    bool finalizeDxfWrite() override { return written; }

    DRW_Polyline polyline;
    Topology topology;
    bool written = false;
    dx_data storage;

private:
    void addVertex(std::uint32_t handle, double x, double y, double z,
                   DRW_Vertex::DwgSubtype subtype, int flags,
                   int identifier = 0) {
        DRW_Vertex vertex(x, y, z, 0.0);
        vertex.handle = handle;
        vertex.setDwgSubtype(subtype);
        vertex.flags = flags;
        vertex.identifier = identifier;
        polyline.addVertex(vertex);
    }
};

class PolylineTopologyReadback final : public dx_iface {
public:
    PolylineTopologyReadback() {
        cData = &storage;
        currentBlock = storage.mBlock;
    }
    void addPolyline(const DRW_Polyline& value) override {
        polylines.push_back(value);
    }
    dx_data storage;
    std::vector<DRW_Polyline> polylines;
};

class DimensionVersionExport final : public dx_iface {
public:
    explicit DimensionVersionExport(dxfRW& output) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &output;

        DRW_Textstyle textStyle;
        textStyle.handle = 0xDC01u;
        textStyle.name = "DIM_MATRIX_TEXT";
        textStyle.font = "simplex.shx";
        storage.textStyles.push_back(textStyle);

        DRW_Dimstyle dimStyle;
        dimStyle.handle = 0xDC02u;
        dimStyle.name = "DIM_MATRIX_STYLE";
        dimStyle.dimtxsty = textStyle.name;
        dimStyle.dimtxstyH.ref = textStyle.handle;
        dimStyle.dimscale = 1.25;
        dimStyle.dimasz = 2.5;
        dimStyle.dimtxt = 3.0;
        dimStyle.dimdec = 3;
        storage.dimStyles.push_back(dimStyle);

        aligned.handle = 0xDC10u;
        aligned.setStyle(dimStyle.name);
        aligned.setDefPoint(DRW_Coord{1.25, 2.5, 3.75});
        aligned.setTextPoint(DRW_Coord{4.5, 5.75, 0.0});
        aligned.setText("version matrix aligned");

        linear.handle = 0xDC11u;
        linear.setStyle(dimStyle.name);
        linear.setDefPoint(DRW_Coord{10.0, 11.0, 12.0});
        linear.setDef1Point(DRW_Coord{10.0, 11.0, 12.0});
        linear.setDef2Point(DRW_Coord{14.0, 16.0, 18.0});
        linear.setTextPoint(DRW_Coord{12.0, 13.0, 0.0});
        linear.setText("version matrix linear");

        const auto initDimension = [](DRW_Dimension& value,
                                      std::uint32_t handle,
                                      double coordinate,
                                      const char* text) {
            value.handle = handle;
            value.setStyle("DIM_MATRIX_STYLE");
            value.setDefPoint(DRW_Coord{coordinate, coordinate + 1.0,
                                        coordinate + 2.0});
            value.setTextPoint(DRW_Coord{coordinate + 0.5,
                                         coordinate + 1.5, 0.0});
            value.setText(text);
        };
        initDimension(radial, 0xDC12u, 20.0, "version matrix radial");
        radial.setDiameterPoint(DRW_Coord{22.0, 20.0, 20.0});
        radial.setLeaderLength(3.0);
        initDimension(diametric, 0xDC13u, 30.0,
                      "version matrix diametric");
        diametric.setDiameter1Point(DRW_Coord{32.0, 30.0, 30.0});
        diametric.setLeaderLength(4.0);
        initDimension(angular, 0xDC14u, 40.0, "version matrix angular");
        angular.setFirstLine1(DRW_Coord{40.0, 40.0, 0.0});
        angular.setFirstLine2(DRW_Coord{41.0, 40.0, 0.0});
        angular.setSecondLine1(DRW_Coord{40.0, 41.0, 0.0});
        angular.setSecondLine2(DRW_Coord{41.0, 41.0, 0.0});
        angular.setDimPoint(DRW_Coord{40.5, 40.5, 0.0});
        initDimension(angular3p, 0xDC15u, 50.0,
                      "version matrix angular 3p");
        angular3p.setFirstLine(DRW_Coord{50.0, 50.0, 0.0});
        angular3p.setSecondLine(DRW_Coord{51.0, 50.0, 0.0});
        angular3p.SetVertexPoint(DRW_Coord{50.0, 51.0, 0.0});
        angular3p.setDimPoint(DRW_Coord{50.5, 50.5, 0.0});
        initDimension(ordinate, 0xDC16u, 60.0,
                      "version matrix ordinate");
        ordinate.setFirstLine(DRW_Coord{61.0, 62.0, 63.0});
        ordinate.setSecondLine(DRW_Coord{64.0, 65.0, 66.0});

        dynamicConstraint.handle = 0xDC17u;
        dynamicConstraint.layer = "*ADSK_CONSTRAINTS";
        dynamicConstraint.setStyle("DIM_MATRIX_STYLE");
        dynamicConstraint.setDefPoint(DRW_Coord{70.0, 71.0, 72.0});
        dynamicConstraint.setTextPoint(DRW_Coord{70.5, 71.5, 0.0});
        dynamicConstraint.setText("version matrix dynamic constraint");
        DxfDimensionDstyleTestAccess::setOverrides(dynamicConstraint);

        legacyDynamicConstraint.handle = 0xDC18u;
        legacyDynamicConstraint.layer = "*LayerNameForDynamicConstraint";
        legacyDynamicConstraint.setStyle("DIM_MATRIX_STYLE");
        legacyDynamicConstraint.setDefPoint(DRW_Coord{80.0, 81.0, 82.0});
        legacyDynamicConstraint.setTextPoint(DRW_Coord{80.5, 81.5, 0.0});
        legacyDynamicConstraint.setText("version matrix legacy constraint");

        genericDimension.handle = 0xDC1Bu;
        genericDimension.setStyle("DIM_MATRIX_STYLE");
        genericDimension.setDefPoint(DRW_Coord{85.0, 86.0, 87.0});
        genericDimension.setTextPoint(DRW_Coord{85.5, 86.5, 0.0});
        genericDimension.setText("version matrix generic dimension");

        arcDimension.handle = 0xDC19u;
        arcDimension.setArcDefPoint(DRW_Coord{90.0, 91.0, 92.0});
        arcDimension.setTextPoint(DRW_Coord{90.5, 91.5, 0.0});
        arcDimension.setText("version matrix arc dimension");
        arcDimension.setStyle("DIM_MATRIX_STYLE");
        arcDimension.setExtLine1(DRW_Coord{91.0, 90.0, 0.0});
        arcDimension.setExtLine2(DRW_Coord{91.0, 92.0, 0.0});
        arcDimension.setArcCenter(DRW_Coord{90.0, 91.0, 0.0});
        arcDimension.setLeaderPt1(DRW_Coord{92.0, 91.0, 0.0});
        arcDimension.leaderPt2 = DRW_Coord{93.0, 91.0, 0.0};
        arcDimension.arcStartAngle = 0.25;
        arcDimension.arcEndAngle = 1.75;
        arcDimension.isPartial = true;
        arcDimension.hasLeader = true;

        largeRadialDimension.handle = 0xDC1Au;
        largeRadialDimension.setCenterPoint(DRW_Coord{100.0, 101.0, 102.0});
        largeRadialDimension.setTextPoint(DRW_Coord{100.5, 101.5, 0.0});
        largeRadialDimension.setText("version matrix large radial");
        largeRadialDimension.setStyle("DIM_MATRIX_STYLE");
        largeRadialDimension.setChordPoint(DRW_Coord{103.0, 100.0, 102.0});
        largeRadialDimension.overrideCenterPoint =
            DRW_Coord{100.0, 101.0, 102.0};
        largeRadialDimension.jogPoint = DRW_Coord{104.0, 102.0, 0.0};
        largeRadialDimension.jogAngle = 0.375;
    }

    void writeEntities() override {
        written = dxfW != nullptr && dxfW->writeDimension(&aligned)
            && dxfW->writeDimension(&linear)
            && dxfW->writeDimension(&radial)
            && dxfW->writeDimension(&diametric)
            && dxfW->writeDimension(&angular)
            && dxfW->writeDimension(&angular3p)
            && dxfW->writeDimension(&ordinate)
            && dxfW->writeDimension(&dynamicConstraint)
            && dxfW->writeDimension(&legacyDynamicConstraint)
            && dxfW->writeDimension(&genericDimension)
            && dxfW->writeDimension(&arcDimension)
            && dxfW->writeDimension(&largeRadialDimension);
    }
    bool finalizeDxfWrite() override { return written; }

    bool sourceUnchanged() const {
        return aligned.handle == 0xDC10u
            && aligned.getStyle() == "DIM_MATRIX_STYLE"
            && aligned.getDefPoint().x == 1.25
            && aligned.getDefPoint().y == 2.5
            && aligned.getDefPoint().z == 3.75
            && aligned.getTextPoint().x == 4.5
            && aligned.getTextPoint().y == 5.75
            && aligned.getText() == "version matrix aligned"
            && linear.handle == 0xDC11u
            && linear.getStyle() == "DIM_MATRIX_STYLE"
            && linear.getDefPoint().x == 10.0
            && linear.getDefPoint().y == 11.0
            && linear.getDefPoint().z == 12.0
            && linear.getDef1Point().x == 10.0
            && linear.getDef1Point().y == 11.0
            && linear.getDef1Point().z == 12.0
            && linear.getDef2Point().x == 14.0
            && linear.getDef2Point().y == 16.0
            && linear.getDef2Point().z == 18.0
            && linear.getTextPoint().x == 12.0
            && linear.getTextPoint().y == 13.0
            && linear.getText() == "version matrix linear"
            && radial.handle == 0xDC12u
            && radial.getDefPoint().x == 20.0
            && radial.getDefPoint().y == 21.0
            && radial.getDefPoint().z == 22.0
            && radial.getDiameterPoint().x == 22.0
            && radial.getLeaderLength() == 3.0
            && radial.getText() == "version matrix radial"
            && diametric.handle == 0xDC13u
            && diametric.getDefPoint().x == 30.0
            && diametric.getDiameter1Point().x == 32.0
            && diametric.getLeaderLength() == 4.0
            && diametric.getText() == "version matrix diametric"
            && angular.handle == 0xDC14u
            && angular.getFirstLine1().x == 40.0
            && angular.getFirstLine2().x == 41.0
            && angular.getSecondLine1().y == 41.0
            && angular.getSecondLine2().y == 41.0
            && angular.getDimPoint().x == 40.5
            && angular.getText() == "version matrix angular"
            && angular3p.handle == 0xDC15u
            && angular3p.getFirstLine().x == 50.0
            && angular3p.getSecondLine().x == 51.0
            && angular3p.getVertexPoint().y == 51.0
            && angular3p.getDimPoint().x == 50.5
            && angular3p.getText() == "version matrix angular 3p"
            && ordinate.handle == 0xDC16u
            && ordinate.getFirstLine().x == 61.0
            && ordinate.getSecondLine().x == 64.0
            && ordinate.getText() == "version matrix ordinate"
            && dynamicConstraint.handle == 0xDC17u
            && dynamicConstraint.layer == "*ADSK_CONSTRAINTS"
            && dynamicConstraint.getDefPoint().x == 70.0
            && dynamicConstraint.getDefPoint().y == 71.0
            && dynamicConstraint.getDefPoint().z == 72.0
            && dynamicConstraint.getText() == "version matrix dynamic constraint"
            && DxfDimensionDstyleTestAccess::matchesOverrides(
                dynamicConstraint)
            && legacyDynamicConstraint.handle == 0xDC18u
            && legacyDynamicConstraint.layer
                == "*LayerNameForDynamicConstraint"
            && legacyDynamicConstraint.getDefPoint().x == 80.0
            && legacyDynamicConstraint.getText()
                == "version matrix legacy constraint"
            && genericDimension.handle == 0xDC1Bu
            && genericDimension.getDefPoint().x == 85.0
            && genericDimension.getDefPoint().y == 86.0
            && genericDimension.getDefPoint().z == 87.0
            && genericDimension.getTextPoint().x == 85.5
            && genericDimension.getTextPoint().y == 86.5
            && genericDimension.getText() == "version matrix generic dimension"
            && arcDimension.handle == 0xDC19u
            && arcDimension.getArcDefPoint().x == 90.0
            && arcDimension.getArcDefPoint().y == 91.0
            && arcDimension.getArcDefPoint().z == 92.0
            && arcDimension.getExtLine1().x == 91.0
            && arcDimension.getExtLine2().y == 92.0
            && arcDimension.getArcCenter().x == 90.0
            && arcDimension.getLeaderPt1().x == 92.0
            && arcDimension.leaderPt2.x == 93.0
            && arcDimension.arcStartAngle == 0.25
            && arcDimension.arcEndAngle == 1.75
            && arcDimension.isPartial && arcDimension.hasLeader
            && arcDimension.getText() == "version matrix arc dimension"
            && largeRadialDimension.handle == 0xDC1Au
            && largeRadialDimension.getCenterPoint().x == 100.0
            && largeRadialDimension.getCenterPoint().y == 101.0
            && largeRadialDimension.getCenterPoint().z == 102.0
            && largeRadialDimension.getChordPoint().x == 103.0
            && largeRadialDimension.overrideCenterPoint.x == 100.0
            && largeRadialDimension.jogPoint.x == 104.0
            && largeRadialDimension.jogAngle == 0.375
            && largeRadialDimension.getText() == "version matrix large radial";
    }

    DRW_DimAligned aligned;
    DRW_DimLinear linear;
    DRW_DimRadial radial;
    DRW_DimDiametric diametric;
    DRW_DimAngular angular;
    DRW_DimAngular3p angular3p;
    DRW_DimOrdinate ordinate;
    DRW_DimLinear dynamicConstraint;
    DRW_DimLinear legacyDynamicConstraint;
    DRW_Dimension genericDimension;
    DRW_DimArc arcDimension;
    DRW_DimLargeRadial largeRadialDimension;
    bool written = false;
    dx_data storage;
};

class DimensionVersionReadback final : public dx_iface {
public:
    DimensionVersionReadback() {
        cData = &storage;
        currentBlock = storage.mBlock;
    }
    void addDimAlign(const DRW_DimAligned* value) override {
        if (value != nullptr) {
            ++alignedCount;
            aligned = *value;
        }
    }
    void addDimLinear(const DRW_DimLinear* value) override {
        if (value != nullptr) {
            if (value->getText() == "version matrix dynamic constraint") {
                ++dynamicConstraintCount;
                dynamicConstraint = *value;
            } else if (value->getText()
                       == "version matrix legacy constraint") {
                ++legacyDynamicConstraintCount;
                legacyDynamicConstraint = *value;
            } else if (value->getText()
                       == "version matrix generic dimension") {
                ++genericDimensionCount;
                genericDimension = *value;
            } else {
                ++linearCount;
                linear = *value;
            }
        }
    }
    void addDimRadial(const DRW_DimRadial* value) override {
        if (value != nullptr) {
            if (const auto* large =
                    dynamic_cast<const DRW_DimLargeRadial*>(value)) {
                ++largeRadialCount;
                largeRadial = *large;
            } else {
                ++radialCount;
                radial = *value;
            }
        }
    }
    void addDimDiametric(const DRW_DimDiametric* value) override {
        if (value != nullptr) {
            ++diametricCount;
            diametric = *value;
        }
    }
    void addDimAngular(const DRW_DimAngular* value) override {
        if (value != nullptr) {
            ++angularCount;
            angular = *value;
        }
    }
    void addDimAngular3P(const DRW_DimAngular3p* value) override {
        if (value != nullptr) {
            ++angular3pCount;
            angular3p = *value;
        }
    }
    void addDimOrdinate(const DRW_DimOrdinate* value) override {
        if (value != nullptr) {
            ++ordinateCount;
            ordinate = *value;
        }
    }
    void addDimArc(const DRW_DimArc* value) override {
        if (value != nullptr) {
            ++arcDimensionCount;
            arcDimension = *value;
        }
    }

    unsigned alignedCount{0};
    unsigned linearCount{0};
    unsigned radialCount{0};
    unsigned diametricCount{0};
    unsigned angularCount{0};
    unsigned angular3pCount{0};
    unsigned ordinateCount{0};
    unsigned dynamicConstraintCount{0};
    unsigned legacyDynamicConstraintCount{0};
    unsigned genericDimensionCount{0};
    unsigned arcDimensionCount{0};
    unsigned largeRadialCount{0};
    DRW_DimAligned aligned;
    DRW_DimLinear linear;
    DRW_DimRadial radial;
    DRW_DimDiametric diametric;
    DRW_DimAngular angular;
    DRW_DimAngular3p angular3p;
    DRW_DimOrdinate ordinate;
    DRW_DimLinear dynamicConstraint;
    DRW_DimLinear legacyDynamicConstraint;
    DRW_DimLinear genericDimension;
    DRW_DimArc arcDimension;
    DRW_DimLargeRadial largeRadial;
    dx_data storage;
};

const std::string& longMTextPayload() {
    static const std::string payload = std::string(249u, 'A') + "€"
        + std::string(300u, 'B');
    return payload;
}

class LongMTextExport final : public dx_iface {
public:
    explicit LongMTextExport(dxfRW& output) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &output;
        mtext.handle = 0xDA10u;
        mtext.text = longMTextPayload();
        mtext.basePoint = DRW_Coord{12.0, -4.0, 3.0};
        mtext.height = 2.25;
        mtext.style = "STANDARD";
    }

    void writeEntities() override {
        entityWritten = dxfW != nullptr && dxfW->writeMText(&mtext);
    }

    DRW_MText mtext;
    bool entityWritten = false;
    dx_data storage;
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
    void addWipeout(const DRW_Wipeout* value) override {
        if (value != nullptr) {
            ++wipeoutCount;
            wipeout = *value;
        }
    }
    void addPointCloud(const DRW_PointCloud* value) override {
        if (value != nullptr) {
            ++pointCloudCount;
            pointCloud = *value;
        }
    }
    void addPointCloudEx(const DRW_PointCloudEx* value) override {
        if (value != nullptr) {
            ++pointCloudExCount;
            pointCloudEx = *value;
        }
    }
    void addNavisworksModel(const DRW_NavisworksModel* value) override {
        if (value != nullptr) {
            ++navisworksModelCount;
            navisworksModel = *value;
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
            if (value->context.hasContentsBlock)
                blockMultiLeader = *value;
            else
                multiLeader = *value;
        }
    }
    void addMLeaderStyle(const DRW_MLeaderStyle* value) override {
        dx_iface::addMLeaderStyle(value);
        if (value != nullptr) {
            ++multiLeaderStyleCount;
            if (value->contentType == 1)
                blockMultiLeaderStyle = *value;
            else
                multiLeaderStyle = *value;
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
        if (value->getText()
            == "whole-operation legacy dynamic constraint") {
            ++legacyDynamicConstraintDimensionCount;
            legacyDynamicConstraintDimension = *value;
        } else if (value->layer == "*ADSK_CONSTRAINTS") {
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
        dx_iface::addView(value);
        ++viewCount;
        if (value.name == "WHOLE_OPERATION_CAMERA_VIEW")
            view = value;
        else if (value.name == "WHOLE_OPERATION_DIRECT_VIEW") {
            ++directViewCount;
            directView = value;
        }
    }
    void addUCS(const DRW_UCS& value) override {
        dx_iface::addUCS(value);
        ++ucsCount;
        if (value.name == "WHOLE_OPERATION_DIRECT_UCS")
            directUcs = value;
    }
    void addAppId(const DRW_AppId& value) override {
        dx_iface::addAppId(value);
        if (value.name == "WHOLE_OPERATION_APPID") {
            ++directAppIdCount;
            directAppId = value;
        }
    }
    void addVport(const DRW_Vport& value) override {
        dx_iface::addVport(value);
        if (value.name == "WHOLE_OPERATION_DIRECT_VPORT") {
            ++directVportCount;
            directVport = value;
        }
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
    void addPointCloudColorMap(const DRW_PointCloudColorMap& value) override {
        ++pointCloudColorMapCount;
        pointCloudColorMap = value;
    }
    void addNavisworksModelDef(const DRW_NavisworksModelDef& value) override {
        ++navisworksModelDefCount;
        navisworksModelDef = value;
    }
    void addPointCloudDef(const DRW_PointCloudDef& value) override {
        pointCloudDefs.push_back(value);
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
    void addGeoData(const DRW_GeoData& value) override {
        ++geoDataCount;
        geoData = value;
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
        if (value.tag == "WHOLE_OPERATION_DIRECT_ATTDEF"
            || value.tag == "WHOLE_OPERATION_R2018_MULTILINE_ATTDEF") {
            ++directAttdefCount;
            directAttdef = value;
        } else {
            ++attdefCount;
            attdef = value;
        }
    }
    void addTableStyle(const DRW_TableStyle& value) override {
        ++tableStyleCount;
        tableStyle = value;
    }
    void addCellStyleMap(const DRW_CellStyleMap& value) override {
        ++cellStyleMapCount;
        cellStyleMap = value;
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
    DRW_Wipeout wipeout;
    DRW_PointCloud pointCloud;
    DRW_PointCloudEx pointCloudEx;
    DRW_NavisworksModel navisworksModel;
    DRW_Ole2Frame ole2Frame;
    DRW_OleFrame oleFrame;
    DRW_Shape shape;
    DRW_RasterVariables rasterVariables;
    DRW_DictionaryVar dictionaryVar;
    DRW_DbColor dbColor;
    DRW_XRecord typedXRecord;
    DRW_MLeader multiLeader;
    DRW_MLeader blockMultiLeader;
    DRW_MLeaderStyle multiLeaderStyle;
    DRW_MLeaderStyle blockMultiLeaderStyle;
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
    DRW_DimLinear legacyDynamicConstraintDimension;
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
    DRW_PointCloudColorMap pointCloudColorMap;
    DRW_NavisworksModelDef navisworksModelDef;
    std::vector<DRW_PointCloudDef> pointCloudDefs;
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
    DRW_GeoData geoData;
    DRW_Attdef attdef;
    DRW_Attdef directAttdef;
    DRW_TableStyle tableStyle;
    DRW_CellStyleMap cellStyleMap;
    DRW_View directView;
    DRW_UCS directUcs;
    DRW_AppId directAppId;
    DRW_Vport directVport;
    std::vector<DRW_RawDxfObject> objects;
    std::vector<DRW_DataLink> dataLinks;
    std::vector<DRW_Dictionary> dictionaries;
    std::vector<DRW_Group> groups;
    unsigned lineCount = 0u;
    unsigned circleCount = 0u;
    unsigned polylineCount = 0u;
    unsigned imageCount = 0u;
    unsigned wipeoutCount = 0u;
    unsigned pointCloudCount = 0u;
    unsigned pointCloudExCount = 0u;
    unsigned navisworksModelCount = 0u;
    unsigned ole2FrameCount = 0u;
    unsigned oleFrameCount = 0u;
    unsigned shapeCount = 0u;
    unsigned rasterVariablesCount = 0u;
    unsigned dictionaryVarCount = 0u;
    unsigned dbColorCount = 0u;
    unsigned typedXRecordCount = 0u;
    unsigned multiLeaderCount = 0u;
    unsigned multiLeaderStyleCount = 0u;
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
    unsigned legacyDynamicConstraintDimensionCount = 0u;
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
    unsigned pointCloudColorMapCount = 0u;
    unsigned navisworksModelDefCount = 0u;
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
    unsigned geoDataCount = 0u;
    unsigned attdefCount = 0u;
    unsigned directAttdefCount = 0u;
    unsigned tableStyleCount = 0u;
    unsigned cellStyleMapCount = 0u;
    unsigned directViewCount = 0u;
    unsigned ucsCount = 0u;
    unsigned directAppIdCount = 0u;
    unsigned directVportCount = 0u;
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

bool pointCloudDefinitionMatches(
    const DRW_PointCloudDef& value, std::uint32_t handle,
    DRW_PointCloudDef::Kind kind, std::int32_t classVersion,
    const std::string& filename, bool loaded, std::uint64_t pointCount,
    const DRW_Coord& minimum, const DRW_Coord& maximum) {
    return value.handle == handle
        && value.parentHandle == DRW::DwgNamedObjectsDictionaryHandle
        && value.m_kind == kind && value.m_classVersion == classVersion
        && value.m_sourceFilename == filename && value.m_isLoaded == loaded
        && value.m_pointCount == pointCount
        && value.m_extentsMin.x == minimum.x
        && value.m_extentsMin.y == minimum.y
        && value.m_extentsMin.z == minimum.z
        && value.m_extentsMax.x == maximum.x
        && value.m_extentsMax.y == maximum.y
        && value.m_extentsMax.z == maximum.z;
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
    const auto unchangedTableStyle = [](const DRW_TableStyle& value) {
        if (value.handle != kTableStyleHandle
            || value.parentHandle != DRW::DwgNamedObjectsDictionaryHandle
            || value.m_name != "WHOLE_OPERATION_TABLE_STYLE"
            || value.m_rowStyles.size() != 3u)
            return false;
        for (std::size_t rowIndex = 0; rowIndex < 3u; ++rowIndex) {
            const DRW_TableStyleRowStyle& row = value.m_rowStyles[rowIndex];
            if (row.m_textStyleHandle != kTableStyleTextHandle
                || row.m_textHeight != 1.25 + rowIndex
                || row.m_textAlignment != 5
                || row.m_textColor != static_cast<int>(rowIndex + 1u)
                || row.m_fillColor != static_cast<int>(rowIndex + 4u)
                || row.m_hasBackgroundColor != (rowIndex != 1u)
                || row.m_borders.size() != 6u)
                return false;
            for (std::size_t borderIndex = 0; borderIndex < 6u;
                 ++borderIndex) {
                const DRW_TableStyleBorder& border = row.m_borders[borderIndex];
                if (border.m_edgeFlags != (1 << borderIndex)
                    || border.m_lineWeight != static_cast<int>(borderIndex)
                    || border.m_visible != static_cast<int>(borderIndex % 2u)
                    || border.m_color != static_cast<int>(borderIndex + 1u))
                    return false;
            }
        }
        return true;
    };
    const auto unchangedCellStyleMap = [](const DRW_CellStyleMap& value) {
        if (value.handle != kCellStyleMapHandle
            || value.parentHandle != DRW::DwgNamedObjectsDictionaryHandle
            || value.m_cellStyles.size() != 1u)
            return false;
        const DRW_TableStyleCellStyle& style = value.m_cellStyles.front();
        return style.m_type == 5 && style.m_hasData
            && style.m_id == 101 && style.m_styleClass == 2
            && style.m_name == "WHOLE_OPERATION_CELL_STYLE"
            && style.m_contentFormat.m_cellAlignment == 1
            && style.m_contentFormat.m_textStyleHandle == kTableStyleTextHandle
            && style.m_contentFormat.m_textHeight == 1.75
            && style.m_borders.size() == 1u
            && style.m_borders.front().m_edgeFlags == 1
            && style.m_borders.front().m_visible == 1;
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
    const auto directView = std::find_if(
        adapter.storage.views.begin(), adapter.storage.views.end(),
        [](const DRW_View& value) {
            return value.handle == kDirectViewHandle;
        });
    const auto directUcs = std::find_if(
        adapter.storage.ucsList.begin(), adapter.storage.ucsList.end(),
        [](const DRW_UCS& value) {
            return value.handle == kDirectUcsHandle;
        });
    const auto directAppId = std::find_if(
        adapter.storage.appIds.begin(), adapter.storage.appIds.end(),
        [](const DRW_AppId& value) {
            return value.handle == kDirectAppIdHandle;
        });
    const auto directVport = std::find_if(
        adapter.storage.VPorts.begin(), adapter.storage.VPorts.end(),
        [](const DRW_Vport& value) {
            return value.handle == kDirectVportHandle;
        });
    const auto directLayer = std::find_if(
        adapter.storage.layers.begin(), adapter.storage.layers.end(),
        [](const DRW_Layer& value) {
            return value.handle == kDirectLayerHandle;
        });
    const auto directDimstyle = std::find_if(
        adapter.storage.dimStyles.begin(), adapter.storage.dimStyles.end(),
        [](const DRW_Dimstyle& value) {
            return value.handle == kDirectDimstyleHandle;
        });
    const auto directTextstyle = std::find_if(
        adapter.storage.textStyles.begin(), adapter.storage.textStyles.end(),
        [](const DRW_Textstyle& value) {
            return value.handle == kDirectTextstyleHandle;
        });
    return unchangedTableStyle(adapter.tableStyle)
        && unchangedCellStyleMap(adapter.cellStyleMap)
        && adapter.storage.views.size() == 2u
        && directView != adapter.storage.views.end()
        && directView->name == "WHOLE_OPERATION_DIRECT_VIEW"
        && directView->size.x == 320.0 && directView->size.y == 200.0
        && directView->center.x == 12.0 && directView->center.y == 18.0
        && directView->targetPoint.x == -4.0
        && directView->targetPoint.y == 8.0
        && directView->targetPoint.z == 12.0
        && directView->lensLen == 42.0
        && directView->twistAngle == 15.0
        && directView->viewMode == 1 && directView->renderMode == 2u
        && directView->cameraPlottable
        && adapter.storage.ucsList.size() == 1u
        && directUcs != adapter.storage.ucsList.end()
        && directUcs->name == "WHOLE_OPERATION_DIRECT_UCS"
        && directUcs->origin.x == 4.0 && directUcs->origin.y == 5.0
        && directUcs->origin.z == 6.0
        && directUcs->xAxisDirection.x == 1.0
        && directUcs->xAxisDirection.y == 0.0
        && directUcs->yAxisDirection.x == 0.0
        && directUcs->yAxisDirection.y == 1.0
        && directUcs->orthoOrigin.x == 1.0
        && directUcs->orthoOrigin.y == 2.0
        && directUcs->orthoOrigin.z == 3.0
        && directUcs->elevation == 2.5
        && directUcs->orthographicOptions.size() == 2u
        && directUcs->orthographicOptions[0].type == 2
        && directUcs->orthographicOptions[0].point.x == 1.0
        && directUcs->orthographicOptions[1].type == 5
        && directUcs->orthographicOptions[1].point.z == 6.0
        && directAppId != adapter.storage.appIds.end()
        && directAppId->name == "WHOLE_OPERATION_APPID"
        && directAppId->flags == 0
        && directVport != adapter.storage.VPorts.end()
        && adapter.storage.VPorts.size() == 2u
        && adapter.storage.VPorts.front().name == "*ACTIVE"
        && directVport->name == "WHOLE_OPERATION_DIRECT_VPORT"
        && directVport->center.x == 12.0 && directVport->center.y == 14.0
        && directVport->snapBase.x == 1.0 && directVport->snapBase.y == 2.0
        && directVport->snapSpacing.x == 3.0
        && directVport->snapSpacing.y == 4.0
        && directVport->gridSpacing.x == 5.0
        && directVport->gridSpacing.y == 6.0
        && directVport->viewTarget.x == 7.0
        && directVport->viewTarget.y == 8.0
        && directVport->viewTarget.z == 9.0
        && directVport->height == 75.0 && directVport->ratio == 1.6
        && directVport->twistAngle == 30.0
        && directVport->snap == 1 && directVport->grid == 1
        && adapter.line.layer == "WHOLE_OPERATION_LAYER"
        && directLayer != adapter.storage.layers.end()
        && directLayer->name == "WHOLE_OPERATION_LAYER"
        && directLayer->color == 3
        && directLayer->lineType == "CONTINUOUS"
        && directLayer->lWeight == DRW_LW_Conv::width07
        && directDimstyle != adapter.storage.dimStyles.end()
        && directDimstyle->name == "WHOLE_OPERATION_DIMSTYLE"
        && directDimstyle->dimtxsty == "WHOLE_OPERATION_TEXT_STYLE"
        && directDimstyle->dimtxstyH.ref == kDirectTextstyleHandle
        && directDimstyle->dimpost == "<> mm"
        && directDimstyle->dimscale == 2.0
        && directDimstyle->dimasz == 2.75
        && directDimstyle->dimtxt == 3.5
        && directDimstyle->dimdec == 3
        && directDimstyle->dimazin == 2
        && directTextstyle != adapter.storage.textStyles.end()
        && directTextstyle->font == "simplex.shx"
        && directTextstyle->width == 0.9
        && directTextstyle->oblique == 0.1
        && directTextstyle->genFlag == 2
        && directTextstyle->lastHeight == 2.0
        && adapter.line.handle == 0xD900u
        && lineTypeCount == 1u
        && adapter.line.basePoint.x == 1.25
        && adapter.line.basePoint.y == -2.5
        && adapter.line.basePoint.z == 3.75
        && adapter.line.secPoint.x == 4.5
        && adapter.line.secPoint.y == 5.25
        && adapter.line.secPoint.z == -6.0
        && adapter.directAttdef.handle == kDirectAttdefHandle
        && adapter.directAttdef.tag == "WHOLE_OPERATION_DIRECT_ATTDEF"
        && adapter.directAttdef.prompt
            == "Long direct attribute prompt for allocation rollback"
        && adapter.directAttdef.text
            == "Long direct attribute default value for retry"
        && adapter.directAttdef.basePoint.x == 3.0
        && adapter.directAttdef.basePoint.y == 5.0
        && adapter.directAttdef.height == 2.25
        && adapter.directAttdef.style == "STANDARD"
        && adapter.directAttdef.attribFlags == 1
        && adapter.directAttdef.m_fieldLength == 48
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
        && adapter.wipeout.handle == kDirectWipeoutHandle
        && adapter.wipeout.clip == 1
        && adapter.wipeout.basePoint.x == 21.0
        && adapter.wipeout.basePoint.y == 22.0
        && adapter.wipeout.basePoint.z == 23.0
        && adapter.wipeout.clipPath.size() == 4u
        && adapter.wipeout.clipPath[2].x == 24.0
        && adapter.wipeout.clipPath[2].y == 18.0
        && adapter.wipeout.clipMode
        && adapter.wipeout.brightness == 60
        && adapter.wipeout.contrast == 70
        && adapter.wipeout.fade == 10
        && adapter.pointCloud.handle == kPointCloudHandle
        && adapter.pointCloud.classVersion == 2u
        && adapter.pointCloud.origin.x == 11.0
        && adapter.pointCloud.origin.y == 12.0
        && adapter.pointCloud.origin.z == 13.0
        && adapter.pointCloud.savedFilename == "clouds/whole-operation.rcs"
        && adapter.pointCloud.sourceFileCount == 2
        && adapter.pointCloud.sourceFiles
            == std::vector<UTF8STRING>{"clouds/part-a.rcs", "clouds/part-b.rcs"}
        && adapter.pointCloud.extentsMin.x == -10.0
        && adapter.pointCloud.extentsMin.y == -20.0
        && adapter.pointCloud.extentsMin.z == -30.0
        && adapter.pointCloud.extentsMax.x == 40.0
        && adapter.pointCloud.extentsMax.y == 50.0
        && adapter.pointCloud.extentsMax.z == 60.0
        && adapter.pointCloud.pointCount == 987654u
        && adapter.pointCloud.ucsName == "WHOLE_OPERATION_POINTCLOUD_UCS"
        && adapter.pointCloud.ucsOrigin.x == 1.0
        && adapter.pointCloud.ucsOrigin.y == 2.0
        && adapter.pointCloud.ucsOrigin.z == 3.0
        && adapter.pointCloud.ucsXDirection.y == 1.0
        && adapter.pointCloud.ucsYDirection.x == -1.0
        && adapter.pointCloud.ucsZDirection.z == 1.0
        && adapter.pointCloud.definitionHandle == kPointCloudDefinitionHandle
        && adapter.pointCloud.reactorHandle == kPointCloudReactorHandle
        && adapter.pointCloudEx.handle == kPointCloudExHandle
        && adapter.pointCloudEx.classVersion == 3u
        && adapter.pointCloudEx.extentsMin.x == -100.0
        && adapter.pointCloudEx.extentsMax.z == 600.0
        && adapter.pointCloudEx.ucsOrigin.x == 7.0
        && adapter.pointCloudEx.ucsYDirection.x == -1.0
        && adapter.pointCloudEx.isLocked
        && adapter.pointCloudEx.definitionHandle == kPointCloudDefinitionExHandle
        && adapter.pointCloudEx.reactorHandle == kPointCloudReactorExHandle
        && adapter.pointCloudEx.name == "whole-operation pointcloud-ex"
        && adapter.pointCloudEx.showIntensity
        && adapter.pointCloudEx.showCropping
        && adapter.pointCloudEx.croppingCount == 1
        && adapter.pointCloudEx.stylizationType == 2
        && adapter.pointCloudEx.intensityColorScheme == "intensity"
        && adapter.pointCloudEx.currentColorScheme == "current"
        && adapter.pointCloudEx.classificationColorScheme == "classification"
        && adapter.pointCloudEx.elevationMin == -5.5
        && adapter.pointCloudEx.elevationMax == 25.5
        && adapter.pointCloudEx.intensityMin == 0.0
        && adapter.pointCloudEx.intensityMax == 65535.0
        && adapter.pointCloudEx.intensityOutOfRangeBehavior == 1
        && adapter.pointCloudEx.elevationOutOfRangeBehavior == 2
        && adapter.pointCloudEx.elevationApplyToFixedRange
        && adapter.pointCloudEx.intensityAsGradient
        && adapter.pointCloudEx.elevationAsGradient
        && adapter.pointCloudEx.croppings.size() == 1u
        && adapter.pointCloudEx.croppings[0].pointCount == 3
        && adapter.pointCloudEx.croppings[0].points.size() == 3u
        && adapter.pointCloudEx.croppings[0].points[2].z == 9.0
        && adapter.navisworksModel.handle == kNavisworksModelHandle
        && adapter.navisworksModel.flags == 5u
        && adapter.navisworksModel.definitionHandle == kNavisworksModelDefHandle
        && adapter.navisworksModel.transform
            == std::array<double, 16>{1.0, 0.0, 0.0, 2.0,
                0.0, 1.0, 0.0, 3.0, 0.0, 0.0, 1.0, 4.0,
                5.0, 6.0, 7.0, 1.0}
        && adapter.navisworksModel.unitFactor == 0.25
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
        && adapter.multiLeader.context.roots.size() == 1u
        && adapter.multiLeader.context.roots[0].leaderIndex == 2
        && adapter.multiLeader.context.roots[0].connectionPoint.x == 1.0
        && adapter.multiLeader.context.roots[0].direction.y == 1.0
        && adapter.multiLeader.context.roots[0].landingDistance == 4.5
        && adapter.multiLeader.context.roots[0].leaderLines.size() == 1u
        && adapter.multiLeader.context.roots[0].leaderLines[0].points.size() == 3u
        && adapter.multiLeader.context.roots[0].leaderLines[0].points[2].z == 9.0
        && adapter.multiLeader.context.roots[0].leaderLines[0].segmentIndex == 1
        && adapter.multiLeader.context.roots[0].leaderLines[0].leaderLineIndex == 3
        && adapter.multiLeader.context.roots[0].leaderLines[0].color == 4
        && adapter.multiLeader.context.roots[0].leaderLines[0].lineWeight == 25
        && adapter.multiLeader.context.roots[0].leaderLines[0].arrowSize == 2.25
        && adapter.multiLeader.styleHandle.ref == kDirectMLeaderStyleHandle
        && adapter.multiLeaderStyle.handle == kDirectMLeaderStyleHandle
        && adapter.multiLeaderStyle.parentHandle == kCustomDictionaryHandle
        && adapter.multiLeaderStyle.textStyleHandle.ref
            == kDirectTextstyleHandle
        && adapter.multiLeaderStyle.description
            == "whole-operation MLEADERSTYLE"
        && adapter.multiLeaderStyle.textDefault == "callout"
        && adapter.multiLeaderStyle.textHeight == 2.25
        && adapter.multiLeaderStyle.arrowHeadSize == 1.75
        && adapter.multiLeaderStyle.maxLeaderPoints == 5
        && adapter.blockMultiLeader.handle == kBlockMLeaderHandle
        && adapter.blockMultiLeader.styleHandle.ref
            == kBlockMLeaderStyleHandle
        && adapter.blockMultiLeader.context.hasContentsBlock
        && !adapter.blockMultiLeader.context.hasTextContents
        && adapter.blockMultiLeader.context.blockTableRecordHandle.ref
            == kInsertBlockHandle
        && adapter.blockMultiLeader.context.blockLocation.x == 21.0
        && adapter.blockMultiLeader.context.blockLocation.y == 22.0
        && adapter.blockMultiLeader.context.blockLocation.z == 23.0
        && adapter.blockMultiLeader.context.blockScale.x == 1.25
        && adapter.blockMultiLeader.context.blockScale.y == 1.5
        && adapter.blockMultiLeader.context.blockScale.z == 1.75
        && adapter.blockMultiLeader.context.blockRotation == 0.625
        && adapter.blockMultiLeader.context.blockNormal.z == 1.0
        && adapter.blockMultiLeader.context.blockTransform
            == std::array<double, 16>{
                1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0, 21.0, 22.0, 23.0, 1.0}
        && adapter.blockMultiLeader.context.roots.size() == 2u
        && adapter.blockMultiLeader.context.roots[0].leaderIndex == 4
        && adapter.blockMultiLeader.context.roots[0].connectionPoint.x == 24.0
        && adapter.blockMultiLeader.context.roots[0].breaks.size() == 1u
        && adapter.blockMultiLeader.context.roots[0].leaderLines.size() == 1u
        && adapter.blockMultiLeader.context.roots[0].leaderLines[0].points.size()
            == 2u
        && adapter.blockMultiLeader.context.roots[0].leaderLines[0]
            .points[1].z == 29.0
        && adapter.blockMultiLeader.context.roots[0].leaderLines[0]
            .breaks.size() == 1u
        && adapter.blockMultiLeader.context.roots[1].leaderIndex == 6
        && adapter.blockMultiLeader.context.roots[1].leaderLines.size() == 1u
        && adapter.blockMultiLeader.context.roots[1].leaderLines[0]
            .points[1].z == 47.0
        && adapter.blockMultiLeaderStyle.handle == kBlockMLeaderStyleHandle
        && adapter.blockMultiLeaderStyle.parentHandle
            == kCustomDictionaryHandle
        && adapter.blockMultiLeaderStyle.contentType == 1
        && adapter.blockMultiLeaderStyle.blockHandle.ref == kInsertBlockHandle
        && adapter.blockMultiLeaderStyle.blockScale.x == 1.25
        && adapter.blockMultiLeaderStyle.blockScale.y == 1.5
        && adapter.blockMultiLeaderStyle.blockScale.z == 1.75
        && adapter.blockMultiLeaderStyle.blockScaleEnabled
        && adapter.geoData.handle == kDirectGeoDataHandle
        && adapter.geoData.parentHandle == 0x2Fu
        && adapter.geoData.m_version == 2
        && adapter.geoData.m_hostBlockHandle == 0x17u
        && adapter.geoData.m_designPoint.x == 100.0
        && adapter.geoData.m_designPoint.y == 200.0
        && adapter.geoData.m_referencePoint.x == 10.0
        && adapter.geoData.m_referencePoint.y == 20.0
        && adapter.geoData.m_coordinateSystemDefinition == "LOCAL_GRID_PROFILE"
        && adapter.geoData.m_geoRssTag == "LOCAL_GEODATA_TAG"
        && adapter.layout.handle == kDirectLayoutHandle
        && adapter.layout.parentHandle == kCustomDictionaryHandle
        && adapter.layout.name == "WHOLE_OPERATION_LAYOUT"
        && adapter.layout.tabOrder == 4
        && adapter.layout.layoutFlags == 1
        && adapter.layout.paperWidth == 210.0
        && adapter.layout.paperHeight == 297.0
        && adapter.layout.limMinX == -2.0
        && adapter.layout.limMinY == -3.0
        && adapter.layout.limMaxX == 20.0
        && adapter.layout.limMaxY == 30.0
        && adapter.layout.paperSize == "ISO_A4_(210.00_x_297.00_MM)"
        && adapter.layout.currentStyleSheet == "WHOLE_OPERATION_STYLE"
        && adapter.rawEntity.name == "WHOLE_OPERATION_RAW_ENTITY"
        && adapter.rawEntity.handle == kDirectRawEntityHandle
        && adapter.rawEntity.parentHandle
            == DRW::DxfModelSpaceBlockRecordHandle
        && adapter.rawEntity.m_version == DRW::AC1027
        && adapter.rawEntity.groups.size() == 6u
        && adapter.rawEntity.groups[0].code() == 5
        && adapter.rawEntity.groups[0].stringEquals("D980")
        && adapter.rawEntity.groups[1].code() == 330
        && adapter.rawEntity.groups[1].stringEquals("1F")
        && adapter.rawEntity.groups[5].code() == 90
        && adapter.rawEntity.groups[5].i_val() == 77
        && adapter.placeholder.handle == kDirectPlaceholderHandle
        && adapter.placeholder.parentHandle == kCustomDictionaryHandle
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
        && DxfDimensionDstyleTestAccess::matchesOverrides(
            adapter.dynamicConstraintDimension)
        && adapter.legacyDynamicConstraintDimension.handle
            == kLegacyDynamicConstraintDimensionHandle
        && adapter.legacyDynamicConstraintDimension.layer
            == "*LayerNameForDynamicConstraint"
        && adapter.legacyDynamicConstraintDimension.getDefPoint().x == 18.0
        && adapter.legacyDynamicConstraintDimension.getDefPoint().y == 19.0
        && adapter.legacyDynamicConstraintDimension.getDefPoint().z == 20.0
        && adapter.legacyDynamicConstraintDimension.getTextPoint().x == 18.5
        && adapter.legacyDynamicConstraintDimension.getTextPoint().y == 19.5
        && adapter.legacyDynamicConstraintDimension.getText()
            == "whole-operation legacy dynamic constraint"
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
        && adapter.storage.views.size() == 2u
        && adapter.storage.views.front().handle == 0xD96Eu
        && adapter.storage.views.front().name == "WHOLE_OPERATION_CAMERA_VIEW"
        && adapter.storage.views.front().size.x == 640.0
        && adapter.storage.views.front().targetPoint.z == 30.0
        && directView != adapter.storage.views.end()
        && directView->name == "WHOLE_OPERATION_DIRECT_VIEW"
        && directView->size.x == 320.0 && directView->size.y == 200.0
        && directView->center.x == 12.0 && directView->center.y == 18.0
        && directView->targetPoint.x == -4.0
        && directView->targetPoint.y == 8.0
        && directView->targetPoint.z == 12.0
        && directView->lensLen == 42.0
        && directView->twistAngle == 15.0
        && directView->viewMode == 1 && directView->renderMode == 2u
        && directView->cameraPlottable
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
        && adapter.mtext.m_backgroundFlags == 1
        && adapter.mtext.m_backgroundScale == 1.35
        && adapter.mtext.m_backgroundColor == 3
        && adapter.mtext.m_backgroundColorTrue == 0x123456
        && adapter.mtext.m_backgroundTrueColor == -1
        && adapter.mtext.m_backgroundColorName
            == "WHOLE_OPERATION_BACKGROUND_COLOR"
        && adapter.mtext.m_backgroundTransparency == 0x02000000
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
        && adapter.pointCloudColorMap.handle == kPointCloudColorMapHandle
        && adapter.pointCloudColorMap.parentHandle
            == DRW::DwgNamedObjectsDictionaryHandle
        && adapter.pointCloudColorMap.m_classVersion == 4
        && adapter.pointCloudColorMap.m_defaultIntensityColorScheme
            == "intensity ramp"
        && adapter.pointCloudColorMap.m_defaultElevationColorScheme
            == "elevation ramp"
        && adapter.pointCloudColorMap.m_defaultClassificationColorScheme
            == "classification ramp"
        && adapter.pointCloudColorMap.m_colorRampCount == 2u
        && adapter.pointCloudColorMap.m_colorRamps.size() == 2u
        && adapter.pointCloudColorMap.m_colorRamps[0].m_classVersion == 2
        && adapter.pointCloudColorMap.m_colorRamps[0].m_rampCount == 3u
        && adapter.pointCloudColorMap.m_colorRamps[0].m_colorSchemes
            == std::vector<UTF8STRING>{"ground", "canopy", "roof"}
        && adapter.pointCloudColorMap.m_colorRamps[1].m_classVersion == 3
        && adapter.pointCloudColorMap.m_colorRamps[1].m_rampCount == 2u
        && adapter.pointCloudColorMap.m_colorRamps[1].m_colorSchemes
            == std::vector<UTF8STRING>{"low", "high"}
        && adapter.pointCloudColorMap.m_classificationColorRampCount == 1u
        && adapter.pointCloudColorMap.m_classificationColorRamps.size() == 1u
        && adapter.pointCloudColorMap.m_classificationColorRamps[0].m_classVersion == 4
        && adapter.pointCloudColorMap.m_classificationColorRamps[0].m_colorSchemes
            == std::vector<UTF8STRING>{"road", "rail"}
        && adapter.navisworksModelDef.handle == kNavisworksModelDefHandle
        && adapter.navisworksModelDef.parentHandle
            == DRW::DwgNamedObjectsDictionaryHandle
        && adapter.navisworksModelDef.m_flags == 3
        && adapter.navisworksModelDef.m_path
            == "models/" + std::string(160u, 'n') + ".nwd"
        && adapter.navisworksModelDef.m_status
        && adapter.navisworksModelDef.m_minExtent.x == -1.5
        && adapter.navisworksModelDef.m_minExtent.y == -2.5
        && adapter.navisworksModelDef.m_minExtent.z == -3.5
        && adapter.navisworksModelDef.m_maxExtent.x == 4.5
        && adapter.navisworksModelDef.m_maxExtent.y == 5.5
        && adapter.navisworksModelDef.m_maxExtent.z == 6.5
        && adapter.navisworksModelDef.m_hostDrawingVisibility
        && pointCloudDefinitionMatches(
            adapter.pointCloudDefinition, kPointCloudDefinitionHandle,
            DRW_PointCloudDef::Definition, 1,
            "pointcloud/" + std::string(96u, 'a') + ".rcs", true, 123456u,
            DRW_Coord(-1.0, -2.0, -3.0), DRW_Coord(4.0, 5.0, 6.0))
        && pointCloudDefinitionMatches(
            adapter.pointCloudDefinitionEx, kPointCloudDefinitionExHandle,
            DRW_PointCloudDef::DefinitionEx, 2,
            "pointcloud-ex/" + std::string(80u, 'b') + ".rcs", true, 654321u,
            DRW_Coord(-7.0, -8.0, -9.0), DRW_Coord(10.0, 11.0, 12.0))
        && pointCloudDefinitionMatches(
            adapter.pointCloudReactor, kPointCloudReactorHandle,
            DRW_PointCloudDef::Reactor, 3, "", false, 0u,
            DRW_Coord(), DRW_Coord())
        && pointCloudDefinitionMatches(
            adapter.pointCloudReactorEx, kPointCloudReactorExHandle,
            DRW_PointCloudDef::ReactorEx, 4, "", false, 0u,
            DRW_Coord(), DRW_Coord())
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

bool tableStyleReadbackMatches(const DRW_TableStyle& value) {
    if (value.handle != kTableStyleHandle
        || value.parentHandle != DRW::DwgNamedObjectsDictionaryHandle
        || value.m_name != "WHOLE_OPERATION_TABLE_STYLE"
        || value.m_rowStyles.size() != 3u)
        return false;
    for (std::size_t rowIndex = 0; rowIndex < 3u; ++rowIndex) {
        const DRW_TableStyleRowStyle& row = value.m_rowStyles[rowIndex];
        if (row.m_textStyleName != "WHOLE_OPERATION_TABLE_TEXT"
            || row.m_textHeight != 1.25 + rowIndex
            || row.m_textAlignment != 5
            || row.m_textColor != static_cast<int>(rowIndex + 1u)
            || row.m_fillColor != static_cast<int>(rowIndex + 4u)
            || row.m_hasBackgroundColor != (rowIndex != 1u)
            || row.m_borders.size() != 6u)
            return false;
        for (std::size_t borderIndex = 0; borderIndex < 6u;
             ++borderIndex) {
            const DRW_TableStyleBorder& border = row.m_borders[borderIndex];
            if (border.m_edgeFlags != (1 << borderIndex)
                || border.m_lineWeight != static_cast<int>(borderIndex)
                || border.m_visible != static_cast<int>(borderIndex % 2u)
                || border.m_color != static_cast<int>(borderIndex + 1u))
                return false;
        }
    }
    return true;
}

bool cellStyleMapReadbackMatches(const DRW_CellStyleMap& value,
                                 const dx_data& storage) {
    if (value.handle != kCellStyleMapHandle
        || value.parentHandle != DRW::DwgNamedObjectsDictionaryHandle
        || value.m_cellStyles.size() != 1u)
        return false;
    const DRW_TableStyleCellStyle& style = value.m_cellStyles.front();
    const auto textStyle = std::find_if(
        storage.textStyles.begin(), storage.textStyles.end(),
        [&style](const DRW_Textstyle& candidate) {
            return candidate.handle
                    == style.m_contentFormat.m_textStyleHandle
                && candidate.name == "WHOLE_OPERATION_TABLE_TEXT";
        });
    return style.m_type == 5 && style.m_hasData && style.m_id == 101
        && style.m_styleClass == 2
        && style.m_name == "WHOLE_OPERATION_CELL_STYLE"
        && style.m_contentFormat.m_cellAlignment == 1
        && textStyle != storage.textStyles.end()
        && style.m_contentFormat.m_textHeight == 1.75
        && style.m_borders.size() == 1u
        && style.m_borders.front().m_edgeFlags == 1
        && style.m_borders.front().m_visible == 1;
}

bool matchesReadback(const std::filesystem::path& path) {
    dxfRW reader(path.string().c_str());
    Readback adapter;
    const bool read = reader.read(&adapter, false);
    const auto directView = std::find_if(
        adapter.storage.views.begin(), adapter.storage.views.end(),
        [](const DRW_View& value) {
            return value.name == "WHOLE_OPERATION_DIRECT_VIEW";
        });
    const auto directAppId = std::find_if(
        adapter.storage.appIds.begin(), adapter.storage.appIds.end(),
        [](const DRW_AppId& value) {
            return value.name == "WHOLE_OPERATION_APPID";
        });
    const auto directVport = std::find_if(
        adapter.storage.VPorts.begin(), adapter.storage.VPorts.end(),
        [](const DRW_Vport& value) {
            return value.name == "WHOLE_OPERATION_DIRECT_VPORT";
        });
    const auto directLayer = std::find_if(
        adapter.storage.layers.begin(), adapter.storage.layers.end(),
        [](const DRW_Layer& value) {
            return value.name == "WHOLE_OPERATION_LAYER";
        });
    const auto directDimstyle = std::find_if(
        adapter.storage.dimStyles.begin(), adapter.storage.dimStyles.end(),
        [](const DRW_Dimstyle& value) {
            return value.name == "WHOLE_OPERATION_DIMSTYLE";
        });
    const auto customTextStyle = std::find_if(
        adapter.storage.textStyles.begin(), adapter.storage.textStyles.end(),
        [](const DRW_Textstyle& value) {
            return value.name == "WHOLE_OPERATION_TEXT_STYLE";
        });
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
    const auto rawPlaceholder = std::find_if(
        adapter.objects.begin(), adapter.objects.end(),
        [](const DRW_RawDxfObject& value) {
            return value.name == "ACDBPLACEHOLDER";
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
    const DRW_RawDxfObject* rawEntity = nullptr;
    if (adapter.storage.mBlock != nullptr) {
        for (const dx_ifaceRawEntityEvent& event
             : adapter.storage.mBlock->rawEntities) {
            if (event.record.name == "WHOLE_OPERATION_RAW_ENTITY") {
                if (rawEntity != nullptr)
                    return false;
                rawEntity = &event.record;
            }
        }
    }
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
    const auto pointCloudDefinitionFor = [&adapter](std::uint32_t handle)
        -> const DRW_PointCloudDef* {
        const auto found = std::find_if(
            adapter.pointCloudDefs.begin(), adapter.pointCloudDefs.end(),
            [handle](const DRW_PointCloudDef& value) {
                return value.handle == handle;
            });
        return found == adapter.pointCloudDefs.end() ? nullptr : &*found;
    };
    if (!read || adapter.lineCount != 1u || adapter.circleCount != 1u
        || adapter.polylineCount != 1u
        || adapter.imageCount != 1u
        || adapter.wipeoutCount != 1u
        || adapter.pointCloudCount != 1u
        || adapter.pointCloudExCount != 1u
        || adapter.navisworksModelCount != 1u
        || adapter.meshCount != 1u
        || adapter.splineCount != 1u
        || adapter.hatchCount != 1u
        || adapter.leaderCount != 1u
        || adapter.mlineCount != 1u
        || adapter.mlineStyleCount != 1u
        || adapter.dimensionCount != 1u
        || adapter.dynamicConstraintDimensionCount != 1u
        || adapter.legacyDynamicConstraintDimensionCount != 1u
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
        || adapter.viewCount != 2u
        || adapter.directViewCount != 1u
        || adapter.ucsCount != 1u
        || adapter.directAppIdCount != 1u
        || directAppId == adapter.storage.appIds.end()
        || directAppId->handle == DRW::NoHandle
        || directAppId->flags != 0
        || adapter.directVportCount != 1u
        || adapter.storage.VPorts.size() != 2u
        || directVport == adapter.storage.VPorts.end()
        || directVport->handle == DRW::NoHandle
        || directVport->center.x != 12.0 || directVport->center.y != 14.0
        || directVport->snapBase.x != 1.0 || directVport->snapBase.y != 2.0
        || directVport->snapSpacing.x != 3.0
        || directVport->snapSpacing.y != 4.0
        || directVport->gridSpacing.x != 5.0
        || directVport->gridSpacing.y != 6.0
        || directVport->viewTarget.x != 7.0
        || directVport->viewTarget.y != 8.0
        || directVport->viewTarget.z != 9.0
        || directVport->height != 75.0 || directVport->ratio != 1.6
        || directVport->twistAngle != 30.0
        || directVport->snap != 1 || directVport->grid != 1
        || directLayer == adapter.storage.layers.end()
        || directLayer->handle == DRW::NoHandle
        || directLayer->color != 3
        || directLayer->lineType != "CONTINUOUS"
        || directLayer->lWeight != DRW_LW_Conv::width07
        || directDimstyle == adapter.storage.dimStyles.end()
        || directDimstyle->handle == DRW::NoHandle
        || customTextStyle == adapter.storage.textStyles.end()
        || customTextStyle->handle == DRW::NoHandle
        || directDimstyle->dimtxstyH.ref == 0u
        || directDimstyle->dimtxstyH.ref != customTextStyle->handle
        || customTextStyle->font != "simplex.shx"
        || customTextStyle->width != 0.9
        || customTextStyle->oblique != 0.1
        || customTextStyle->genFlag != 2
        || customTextStyle->lastHeight != 2.0
        || directDimstyle->dimpost != "<> mm"
        || directDimstyle->dimscale != 2.0
        || directDimstyle->dimasz != 2.75
        || directDimstyle->dimtxt != 3.5
        || directDimstyle->dimdec != 3
        || directDimstyle->dimazin != 2
        || adapter.geoPositionMarkerCount != 2u
        || adapter.line.basePoint.x != 1.25
        || adapter.line.layer != "WHOLE_OPERATION_LAYER"
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
        || adapter.wipeout.handle == 0u
        || adapter.wipeout.basePoint.x != 21.0
        || adapter.wipeout.basePoint.y != 22.0
        || adapter.wipeout.basePoint.z != 23.0
        || adapter.wipeout.secPoint.x != 4.0
        || adapter.wipeout.vVector.y != 3.0
        || adapter.wipeout.sizeu != 320.0
        || adapter.wipeout.sizev != 240.0
        || adapter.wipeout.clip != 1
        || adapter.wipeout.brightness != 60
        || adapter.wipeout.contrast != 70
        || adapter.wipeout.fade != 10
        || adapter.wipeout.m_clipBoundaryType != 2
        || adapter.wipeout.clipPath.size() != 4u
        || adapter.wipeout.clipPath[2].x != 24.0
        || adapter.wipeout.clipPath[2].y != 18.0
        || !adapter.wipeout.clipMode
        || adapter.pointCloud.handle == 0u
        || adapter.pointCloud.classVersion != 2u
        || adapter.pointCloud.origin.x != 11.0
        || adapter.pointCloud.origin.y != 12.0
        || adapter.pointCloud.origin.z != 13.0
        || adapter.pointCloud.savedFilename != "clouds/whole-operation.rcs"
        || adapter.pointCloud.sourceFileCount != 2
        || adapter.pointCloud.sourceFiles
            != std::vector<UTF8STRING>{"clouds/part-a.rcs", "clouds/part-b.rcs"}
        || adapter.pointCloud.extentsMin.x != -10.0
        || adapter.pointCloud.extentsMin.y != -20.0
        || adapter.pointCloud.extentsMin.z != -30.0
        || adapter.pointCloud.extentsMax.x != 40.0
        || adapter.pointCloud.extentsMax.y != 50.0
        || adapter.pointCloud.extentsMax.z != 60.0
        || adapter.pointCloud.pointCount != 987654u
        || adapter.pointCloud.ucsName != "WHOLE_OPERATION_POINTCLOUD_UCS"
        || adapter.pointCloud.ucsOrigin.x != 1.0
        || adapter.pointCloud.ucsOrigin.y != 2.0
        || adapter.pointCloud.ucsOrigin.z != 3.0
        || adapter.pointCloud.ucsXDirection.y != 1.0
        || adapter.pointCloud.ucsYDirection.x != -1.0
        || adapter.pointCloud.ucsZDirection.z != 1.0
        || adapter.pointCloud.definitionHandle != kPointCloudDefinitionHandle
        || adapter.pointCloud.reactorHandle != kPointCloudReactorHandle
        || adapter.pointCloudEx.handle == 0u
        || adapter.pointCloudEx.classVersion != 3u
        || adapter.pointCloudEx.extentsMin.x != -100.0
        || adapter.pointCloudEx.extentsMax.z != 600.0
        || adapter.pointCloudEx.ucsOrigin.x != 7.0
        || adapter.pointCloudEx.ucsYDirection.x != -1.0
        || !adapter.pointCloudEx.isLocked
        || adapter.pointCloudEx.definitionHandle != kPointCloudDefinitionExHandle
        || adapter.pointCloudEx.reactorHandle != kPointCloudReactorExHandle
        || adapter.pointCloudEx.name != "whole-operation pointcloud-ex"
        || !adapter.pointCloudEx.showIntensity
        || !adapter.pointCloudEx.showCropping
        || adapter.pointCloudEx.croppingCount != 1
        || adapter.pointCloudEx.stylizationType != 2
        || adapter.pointCloudEx.intensityColorScheme != "intensity"
        || adapter.pointCloudEx.currentColorScheme != "current"
        || adapter.pointCloudEx.classificationColorScheme != "classification"
        || adapter.pointCloudEx.elevationMin != -5.5
        || adapter.pointCloudEx.elevationMax != 25.5
        || adapter.pointCloudEx.intensityMin != 0.0
        || adapter.pointCloudEx.intensityMax != 65535.0
        || adapter.pointCloudEx.intensityOutOfRangeBehavior != 1
        || adapter.pointCloudEx.elevationOutOfRangeBehavior != 2
        || !adapter.pointCloudEx.elevationApplyToFixedRange
        || !adapter.pointCloudEx.intensityAsGradient
        || !adapter.pointCloudEx.elevationAsGradient
        || adapter.pointCloudEx.croppings.size() != 1u
        || adapter.pointCloudEx.croppings[0].type != 1
        || !adapter.pointCloudEx.croppings[0].isInside
        || adapter.pointCloudEx.croppings[0].isInverted
        || adapter.pointCloudEx.croppings[0].cropPlane.x != 1.0
        || adapter.pointCloudEx.croppings[0].cropXDirection.x != 1.0
        || adapter.pointCloudEx.croppings[0].cropYDirection.y != 1.0
        || adapter.pointCloudEx.croppings[0].pointCount != 3
        || adapter.pointCloudEx.croppings[0].points.size() != 3u
        || adapter.pointCloudEx.croppings[0].points[0].x != 1.0
        || adapter.pointCloudEx.croppings[0].points[1].y != 5.0
        || adapter.pointCloudEx.croppings[0].points[2].z != 9.0
        || adapter.navisworksModel.handle == 0u
        || adapter.navisworksModel.flags != 5u
        || adapter.navisworksModel.definitionHandle != kNavisworksModelDefHandle
        || adapter.navisworksModel.transform
            != std::array<double, 16>{1.0, 0.0, 0.0, 2.0,
                0.0, 1.0, 0.0, 3.0, 0.0, 0.0, 1.0, 4.0,
                5.0, 6.0, 7.0, 1.0}
        || adapter.navisworksModel.unitFactor != 0.25
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
        || adapter.multiLeaderCount != 2u
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
        || adapter.multiLeader.context.roots.size() != 1u
        || adapter.multiLeader.context.roots[0].leaderIndex != 2
        || adapter.multiLeader.context.roots[0].connectionPoint.x != 1.0
        || adapter.multiLeader.context.roots[0].connectionPoint.y != 2.0
        || adapter.multiLeader.context.roots[0].connectionPoint.z != 3.0
        || adapter.multiLeader.context.roots[0].direction.y != 1.0
        || adapter.multiLeader.context.roots[0].landingDistance != 4.5
        || adapter.multiLeader.context.roots[0].leaderLines.size() != 1u
        || adapter.multiLeader.context.roots[0].leaderLines[0].points.size() != 3u
        || adapter.multiLeader.context.roots[0].leaderLines[0].points[0].x != 1.0
        || adapter.multiLeader.context.roots[0].leaderLines[0].points[2].z != 9.0
        || adapter.multiLeader.context.roots[0].leaderLines[0].segmentIndex != 1
        || adapter.multiLeader.context.roots[0].leaderLines[0].leaderLineIndex != 3
        || adapter.multiLeader.context.roots[0].leaderLines[0].color != 4
        || adapter.multiLeader.context.roots[0].leaderLines[0].lineWeight != 25
        || adapter.multiLeader.context.roots[0].leaderLines[0].arrowSize != 2.25
        || adapter.multiLeaderStyleCount != 2u
        || adapter.multiLeader.styleHandle.ref
            != adapter.multiLeaderStyle.handle
        || adapter.multiLeaderStyle.handle == 0u
        || adapter.multiLeaderStyle.parentHandle != kCustomDictionaryHandle
        || adapter.multiLeaderStyle.textStyleHandle.ref
            != customTextStyle->handle
        || adapter.multiLeaderStyle.description
            != "whole-operation MLEADERSTYLE"
        || adapter.multiLeaderStyle.textDefault != "callout"
        || adapter.multiLeaderStyle.textHeight != 2.25
        || adapter.multiLeaderStyle.arrowHeadSize != 1.75
        || adapter.multiLeaderStyle.maxLeaderPoints != 5
        || adapter.blockMultiLeader.handle == 0u
        || adapter.blockMultiLeader.parentHandle
            != DRW::DxfModelSpaceBlockRecordHandle
        || adapter.blockMultiLeader.styleHandle.ref
            != adapter.blockMultiLeaderStyle.handle
        || adapter.blockMultiLeader.styleContentType != 1u
        || adapter.blockMultiLeader.context.hasTextContents
        || !adapter.blockMultiLeader.context.hasContentsBlock
        || insertBlock == adapter.storage.blocks.end()
        || adapter.blockMultiLeader.context.blockTableRecordHandle.ref
            != (*insertBlock)->handle
        || adapter.blockMultiLeader.context.blockLocation.x != 21.0
        || adapter.blockMultiLeader.context.blockLocation.y != 22.0
        || adapter.blockMultiLeader.context.blockLocation.z != 23.0
        || adapter.blockMultiLeader.context.blockScale.x != 1.25
        || adapter.blockMultiLeader.context.blockScale.y != 1.5
        || adapter.blockMultiLeader.context.blockScale.z != 1.75
        || adapter.blockMultiLeader.context.blockRotation != 0.625
        || adapter.blockMultiLeader.context.blockNormal.z != 1.0
        || adapter.blockMultiLeader.context.blockTransform.size() != 16u
        || adapter.blockMultiLeader.context.blockTransform
            != std::array<double, 16>{
                1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0,
                0.0, 0.0, 1.0, 0.0, 21.0, 22.0, 23.0, 1.0}
        || adapter.blockMultiLeader.context.roots.size() != 2u
        || adapter.blockMultiLeader.context.roots[0].leaderIndex != 4
        || adapter.blockMultiLeader.context.roots[0].connectionPoint.x != 24.0
        || adapter.blockMultiLeader.context.roots[0].breaks.size() != 1u
        || adapter.blockMultiLeader.context.roots[0].leaderLines.size() != 1u
        || adapter.blockMultiLeader.context.roots[0].leaderLines[0].points.size()
            != 2u
        || adapter.blockMultiLeader.context.roots[0].leaderLines[0]
            .points[1].z != 29.0
        || adapter.blockMultiLeader.context.roots[0].leaderLines[0]
            .breaks.size() != 1u
        || adapter.blockMultiLeader.context.roots[1].leaderIndex != 6
        || adapter.blockMultiLeader.context.roots[1].leaderLines.size() != 1u
        || adapter.blockMultiLeader.context.roots[1].leaderLines[0]
            .points[1].z != 47.0
        || adapter.blockMultiLeader.context.blockColor != 4
        || adapter.multiLeaderStyleCount != 2u
        || adapter.blockMultiLeaderStyle.handle != kBlockMLeaderStyleHandle
        || adapter.blockMultiLeaderStyle.parentHandle
            != kCustomDictionaryHandle
        || adapter.blockMultiLeaderStyle.contentType != 1
        || adapter.blockMultiLeaderStyle.blockHandle.ref != (*insertBlock)->handle
        || adapter.blockMultiLeaderStyle.blockScale.x != 1.25
        || adapter.blockMultiLeaderStyle.blockScale.y != 1.5
        || adapter.blockMultiLeaderStyle.blockScale.z != 1.75
        || !adapter.blockMultiLeaderStyle.blockScaleEnabled
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
        || !DxfDimensionDstyleTestAccess::matchesOverrides(
            adapter.dynamicConstraintDimension)
        || adapter.legacyDynamicConstraintDimension.handle == 0u
        || adapter.legacyDynamicConstraintDimension.layer
            != "*ADSK_CONSTRAINTS"
        || adapter.legacyDynamicConstraintDimension.getDefPoint().x != 18.0
        || adapter.legacyDynamicConstraintDimension.getDefPoint().y != 19.0
        || adapter.legacyDynamicConstraintDimension.getDefPoint().z != 20.0
        || adapter.legacyDynamicConstraintDimension.getTextPoint().x != 18.5
        || adapter.legacyDynamicConstraintDimension.getTextPoint().y != 19.5
        || adapter.legacyDynamicConstraintDimension.getText()
            != "whole-operation legacy dynamic constraint"
        || adapter.legacyDynamicConstraintDimension.getStyle() != "STANDARD"
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
        || adapter.storage.views.size() != 2u
        || directView == adapter.storage.views.end()
        || directView->handle == DRW::NoHandle
        || directView->size.x != 320.0 || directView->size.y != 200.0
        || directView->center.x != 12.0 || directView->center.y != 18.0
        || directView->targetPoint.x != -4.0
        || directView->targetPoint.y != 8.0
        || directView->targetPoint.z != 12.0
        || directView->lensLen != 42.0
        || directView->twistAngle != 15.0
        || directView->viewMode != 1 || directView->renderMode != 2u
        || !directView->cameraPlottable
        || adapter.storage.ucsList.size() != 1u
        || adapter.directUcs.handle == DRW::NoHandle
        || adapter.directUcs.name != "WHOLE_OPERATION_DIRECT_UCS"
        || adapter.directUcs.origin.x != 4.0
        || adapter.directUcs.origin.y != 5.0
        || adapter.directUcs.origin.z != 6.0
        || adapter.directUcs.xAxisDirection.x != 1.0
        || adapter.directUcs.yAxisDirection.y != 1.0
        || adapter.directUcs.orthoOrigin.x != 1.0
        || adapter.directUcs.orthoOrigin.y != 2.0
        || adapter.directUcs.orthoOrigin.z != 3.0
        || adapter.directUcs.elevation != 2.5
        || adapter.directUcs.orthographicOptions.size() != 2u
        || adapter.directUcs.orthographicOptions[0].type != 2
        || adapter.directUcs.orthographicOptions[0].point.x != 1.0
        || adapter.directUcs.orthographicOptions[0].point.y != 2.0
        || adapter.directUcs.orthographicOptions[0].point.z != 3.0
        || adapter.directUcs.orthographicOptions[1].type != 5
        || adapter.directUcs.orthographicOptions[1].point.x != 4.0
        || adapter.directUcs.orthographicOptions[1].point.y != 5.0
        || adapter.directUcs.orthographicOptions[1].point.z != 6.0
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
        || adapter.mtext.m_backgroundFlags != 1
        || adapter.mtext.m_backgroundScale != 1.35
        || adapter.mtext.m_backgroundColor != 3
        || adapter.mtext.m_backgroundColorTrue != 0x123456
        || adapter.mtext.m_backgroundTrueColor != 0x123456
        || adapter.mtext.m_backgroundColorName
            != "WHOLE_OPERATION_BACKGROUND_COLOR"
        || adapter.mtext.m_backgroundTransparency != 0x02000000
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
        || adapter.pointCloudColorMapCount != 1u
        || adapter.pointCloudColorMap.handle == 0u
        || adapter.pointCloudColorMap.parentHandle
            != DRW::DwgNamedObjectsDictionaryHandle
        || adapter.pointCloudColorMap.m_classVersion != 4
        || adapter.pointCloudColorMap.m_defaultIntensityColorScheme
            != "intensity ramp"
        || adapter.pointCloudColorMap.m_defaultElevationColorScheme
            != "elevation ramp"
        || adapter.pointCloudColorMap.m_defaultClassificationColorScheme
            != "classification ramp"
        || adapter.pointCloudColorMap.m_colorRampCount != 2u
        || adapter.pointCloudColorMap.m_colorRamps.size() != 2u
        || adapter.pointCloudColorMap.m_colorRamps[0].m_classVersion != 2
        || adapter.pointCloudColorMap.m_colorRamps[0].m_rampCount != 3u
        || adapter.pointCloudColorMap.m_colorRamps[0].m_colorSchemes
            != std::vector<UTF8STRING>{"ground", "canopy", "roof"}
        || adapter.pointCloudColorMap.m_colorRamps[1].m_classVersion != 3
        || adapter.pointCloudColorMap.m_colorRamps[1].m_rampCount != 2u
        || adapter.pointCloudColorMap.m_colorRamps[1].m_colorSchemes
            != std::vector<UTF8STRING>{"low", "high"}
        || adapter.pointCloudColorMap.m_classificationColorRampCount != 1u
        || adapter.pointCloudColorMap.m_classificationColorRamps.size() != 1u
        || adapter.pointCloudColorMap.m_classificationColorRamps[0].m_classVersion != 4
        || adapter.pointCloudColorMap.m_classificationColorRamps[0].m_rampCount != 2u
        || adapter.pointCloudColorMap.m_classificationColorRamps[0].m_colorSchemes
            != std::vector<UTF8STRING>{"road", "rail"}
        || adapter.pointCloudDefs.size() != 4u
        || pointCloudDefinitionFor(kPointCloudDefinitionHandle) == nullptr
        || !pointCloudDefinitionMatches(
            *pointCloudDefinitionFor(kPointCloudDefinitionHandle),
            kPointCloudDefinitionHandle, DRW_PointCloudDef::Definition, 1,
            "pointcloud/" + std::string(96u, 'a') + ".rcs", true, 123456u,
            DRW_Coord(-1.0, -2.0, -3.0), DRW_Coord(4.0, 5.0, 6.0))
        || pointCloudDefinitionFor(kPointCloudDefinitionExHandle) == nullptr
        || !pointCloudDefinitionMatches(
            *pointCloudDefinitionFor(kPointCloudDefinitionExHandle),
            kPointCloudDefinitionExHandle, DRW_PointCloudDef::DefinitionEx, 2,
            "pointcloud-ex/" + std::string(80u, 'b') + ".rcs", true, 654321u,
            DRW_Coord(-7.0, -8.0, -9.0), DRW_Coord(10.0, 11.0, 12.0))
        || pointCloudDefinitionFor(kPointCloudReactorHandle) == nullptr
        || !pointCloudDefinitionMatches(
            *pointCloudDefinitionFor(kPointCloudReactorHandle),
            kPointCloudReactorHandle, DRW_PointCloudDef::Reactor, 3,
            "", false, 0u, DRW_Coord(), DRW_Coord())
        || pointCloudDefinitionFor(kPointCloudReactorExHandle) == nullptr
        || !pointCloudDefinitionMatches(
            *pointCloudDefinitionFor(kPointCloudReactorExHandle),
            kPointCloudReactorExHandle, DRW_PointCloudDef::ReactorEx, 4,
            "", false, 0u, DRW_Coord(), DRW_Coord())
        || adapter.navisworksModelDefCount != 1u
        || adapter.navisworksModelDef.handle == 0u
        || adapter.navisworksModelDef.parentHandle
            != DRW::DwgNamedObjectsDictionaryHandle
        || adapter.navisworksModelDef.m_flags != 3
        || adapter.navisworksModelDef.m_path
            != "models/" + std::string(160u, 'n') + ".nwd"
        || !adapter.navisworksModelDef.m_status
        || adapter.navisworksModelDef.m_minExtent.x != -1.5
        || adapter.navisworksModelDef.m_minExtent.y != -2.5
        || adapter.navisworksModelDef.m_minExtent.z != -3.5
        || adapter.navisworksModelDef.m_maxExtent.x != 4.5
        || adapter.navisworksModelDef.m_maxExtent.y != 5.5
        || adapter.navisworksModelDef.m_maxExtent.z != 6.5
        || !adapter.navisworksModelDef.m_hostDrawingVisibility
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
        || adapter.geoDataCount != 1u
        || adapter.geoData.handle == 0u
        || adapter.geoData.parentHandle != 0x2Fu
        || adapter.geoData.m_version != 2
        || adapter.geoData.m_hostBlockHandle != 0x17u
        || adapter.geoData.m_designPoint.x != 100.0
        || adapter.geoData.m_designPoint.y != 200.0
        || adapter.geoData.m_referencePoint.x != 10.0
        || adapter.geoData.m_referencePoint.y != 20.0
        || adapter.geoData.m_coordinateSystemDefinition
            != "LOCAL_GRID_PROFILE"
        || adapter.geoData.m_geoRssTag != "LOCAL_GEODATA_TAG"
        || adapter.storage.layouts.size() != 1u
        || adapter.storage.layouts.front().handle == 0u
        || adapter.storage.layouts.front().parentHandle
            != kCustomDictionaryHandle
        || adapter.storage.layouts.front().name != "WHOLE_OPERATION_LAYOUT"
        || adapter.storage.layouts.front().tabOrder != 4
        || adapter.storage.layouts.front().layoutFlags != 1
        || adapter.storage.layouts.front().paperWidth != 210.0
        || adapter.storage.layouts.front().paperHeight != 297.0
        || adapter.storage.layouts.front().limMinX != -2.0
        || adapter.storage.layouts.front().limMinY != -3.0
        || adapter.storage.layouts.front().limMaxX != 20.0
        || adapter.storage.layouts.front().limMaxY != 30.0
        || adapter.storage.layouts.front().paperSize
            != "ISO_A4_(210.00_x_297.00_MM)"
        || adapter.storage.layouts.front().currentStyleSheet
            != "WHOLE_OPERATION_STYLE"
        || rawEntity == nullptr
        || rawEntity->handle != kDirectRawEntityHandle
        || rawEntity->parentHandle
            != DRW::DxfModelSpaceBlockRecordHandle
        || rawEntity->m_version != DRW::AC1027
        || rawEntity->groups.size() != 6u
        || rawEntity->groups[0].code() != 5
        || !rawEntity->groups[0].stringEquals("D980")
        || rawEntity->groups[1].code() != 330
        || !rawEntity->groups[1].stringEquals("1F")
        || rawEntity->groups[5].code() != 90
        || rawEntity->groups[5].i_val() != 77
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
        || adapter.directAttdefCount != 1u
        || adapter.directAttdef.handle == 0u
        || adapter.directAttdef.tag != "WHOLE_OPERATION_DIRECT_ATTDEF"
        || adapter.directAttdef.prompt
            != "Long direct attribute prompt for allocation rollback"
        || adapter.directAttdef.text
            != "Long direct attribute default value for retry"
        || adapter.directAttdef.basePoint.x != 3.0
        || adapter.directAttdef.basePoint.y != 5.0
        || adapter.directAttdef.height != 2.25
        || adapter.directAttdef.style != "STANDARD"
        || adapter.directAttdef.attribFlags != 1
        || adapter.directAttdef.m_fieldLength != 48
        || adapter.tableStyleCount != 1u
        || !tableStyleReadbackMatches(adapter.tableStyle)
        || adapter.cellStyleMapCount != 1u
        || !cellStyleMapReadbackMatches(adapter.cellStyleMap,
                                        adapter.storage)
        || rawPlaceholder == adapter.objects.end()
        || rawPlaceholder->parentHandle != kCustomDictionaryHandle
        || rawPlaceholder->groups.size() != 2u
        || rawPlaceholder->groups[0].code() != 5
        || rawPlaceholder->groups[0].c_str() == nullptr
        || std::string(rawPlaceholder->groups[0].c_str()) != "D97C"
        || rawPlaceholder->groups[1].code() != 330
        || rawPlaceholder->groups[1].c_str() == nullptr
        || std::string(rawPlaceholder->groups[1].c_str()) != "D914"
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
        std::cerr << " wipeouts=" << adapter.wipeoutCount
                  << " wipeoutHandle=" << adapter.wipeout.handle
                  << " wipeoutBoundary=" << adapter.wipeout.clipPath.size()
                  << ':' << adapter.wipeout.m_clipBoundaryType
                  << " wipeoutClip=" << adapter.wipeout.clip
                  << " wipeoutMode=" << adapter.wipeout.clipMode
                  << " wipeoutBrightness=" << adapter.wipeout.brightness
                  << " typed-xrecord=" << adapter.typedXRecordCount << '/'
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
                      && value.m_entries.size() == 8u
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
                          == kTypedXRecordHandle
                      && value.m_entries[5].m_name
                          == "WHOLE_OPERATION_MLEADERSTYLE"
                      && value.m_entries[5].m_handle
                          == kDirectMLeaderStyleHandle
                      && value.m_entries[6].m_name
                          == "WHOLE_OPERATION_BLOCK_MLEADERSTYLE"
                      && value.m_entries[6].m_handle
                          == kBlockMLeaderStyleHandle
                      && value.m_entries[7].m_name
                          == "WHOLE_OPERATION_LAYOUT"
                      && value.m_entries[7].m_handle
                          == kDirectLayoutHandle;
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
            {
                std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior.good())
                    return fail("could not seed calibration destination");
            }
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

class HatchOnlyExport final : public dx_iface {
public:
    explicit HatchOnlyExport(dxfRW& output, bool useMPolygon = false,
                             bool useGradient = false)
        : output(output), useMPolygon(useMPolygon), useGradient(useGradient) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &output;
        hatch.handle = 0xDA50u;
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

        line = std::make_shared<DRW_Line>();
        line->basePoint = DRW_Coord(1.0, 2.0, 0.0);
        line->secPoint = DRW_Coord(4.0, 6.0, 0.0);
        auto lineLoop = std::make_shared<DRW_HatchLoop>(0);
        lineLoop->objlist.push_back(line);
        lineLoop->update();
        hatch.appendLoop(lineLoop);

        arc = std::make_shared<DRW_Arc>();
        arc->basePoint = DRW_Coord(2.0, 3.0, 0.0);
        arc->radious = 4.0;
        arc->staangle = 0.125;
        arc->endangle = 1.25;
        arc->isccw = 1;
        auto arcLoop = std::make_shared<DRW_HatchLoop>(0);
        arcLoop->objlist.push_back(arc);
        arcLoop->update();
        hatch.appendLoop(arcLoop);

        ellipse = std::make_shared<DRW_Ellipse>();
        ellipse->basePoint = DRW_Coord(5.0, 7.0, 0.0);
        ellipse->secPoint = DRW_Coord(3.0, 0.0, 0.0);
        ellipse->ratio = 0.5;
        ellipse->staparam = 0.25;
        ellipse->endparam = 1.5;
        ellipse->isccw = 1;
        auto ellipseLoop = std::make_shared<DRW_HatchLoop>(0);
        ellipseLoop->objlist.push_back(ellipse);
        ellipseLoop->update();
        hatch.appendLoop(ellipseLoop);

        spline = std::make_shared<DRW_Spline>();
        spline->degree = 2;
        spline->flags = 4;
        spline->knotslist = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
        spline->controllist = {
            std::make_shared<DRW_Coord>(0.0, 0.0, 0.0),
            std::make_shared<DRW_Coord>(1.0, 2.0, 0.0),
            std::make_shared<DRW_Coord>(3.0, 0.0, 0.0)};
        spline->weightlist = {1.0, 1.25, 1.0};
        spline->fitlist = {
            std::make_shared<DRW_Coord>(1.5, 1.0, 0.0)};
        auto splineLoop = std::make_shared<DRW_HatchLoop>(0);
        splineLoop->objlist.push_back(spline);
        splineLoop->update();
        hatch.appendLoop(splineLoop);
        hatch.seedPoints.emplace_back(0.5, 0.75, 0.0);
        if (useGradient) {
            hatch.solid = 1;
            hatch.patternLines.clear();
            hatch.isGradient = 1;
            hatch.gradReserved = 0;
            hatch.gradAngle = 0.75;
            hatch.gradShift = 0.25;
            hatch.singleColor = 0;
            hatch.gradName = "LINEAR";
            hatch.gradColors = {
                DRW_Hatch::GradientStop{0.0, 0x112233, 0, 0, "", ""},
                DRW_Hatch::GradientStop{1.0, 0x445566, 0, 0, "", ""}};
        }
        if (useMPolygon) {
            static_cast<DRW_Hatch&>(mpolygon) = hatch;
            mpolygon.eType = DRW::MPOLYGON;
            mpolygon.dxfVersion = 1;
            mpolygon.fillColorAci = 5;
            mpolygon.annotatedBoundary = 1;
            mpolygon.xDirX = 1.5;
            mpolygon.xDirY = -0.75;
        }
    }

    void writeEntities() override {
        entitiesWritten = useMPolygon ? output.writeMPolygon(&mpolygon)
                                      : output.writeHatch(&hatch);
    }

    bool sourceUnchanged() const {
        const DRW_Hatch& entity = useMPolygon
            ? static_cast<const DRW_Hatch&>(mpolygon) : hatch;
        if (entity.handle != 0xDA50u || entity.name != "ANSI31"
            || entity.solid != (useGradient ? 1 : 0) || entity.hpattern != 0
            || entity.angle != 0.25 || entity.scale != 1.5
            || entity.looplist.size() != 5u || !entity.looplist[0]
            || entity.looplist[0]->objlist.size() != 1u
            || !entity.looplist[1] || entity.looplist[1]->objlist.size() != 1u
            || !entity.looplist[2] || entity.looplist[2]->objlist.size() != 1u
            || !entity.looplist[3] || entity.looplist[3]->objlist.size() != 1u
            || !entity.looplist[4] || entity.looplist[4]->objlist.size() != 1u
            || entity.patternLines.size() != (useGradient ? 0u : 1u)
            || (!useGradient && entity.patternLines.front().dashList
                != std::vector<double>{0.5, -0.25, 1.0}
            )
            || entity.seedPoints.size() != 1u
            || entity.seedPoints.front().x != 0.5
            || entity.seedPoints.front().y != 0.75
            || entity.isGradient != (useGradient ? 1 : 0)
            || (useGradient && (entity.gradName != "LINEAR"
                || entity.gradReserved != 0 || entity.gradAngle != 0.75
                || entity.gradShift != 0.25 || entity.singleColor != 0
                || entity.gradTint != 0.0
                || entity.gradColors.size() != 2u
                || entity.gradColors[0].value != 0.0
                || entity.gradColors[0].rgb != 0x112233
                || entity.gradColors[1].value != 1.0
                || entity.gradColors[1].rgb != 0x445566))
            || (useMPolygon && (mpolygon.dxfVersion != 1
                || mpolygon.fillColorAci != 5
                || mpolygon.annotatedBoundary != 1
                || mpolygon.xDirX != 1.5 || mpolygon.xDirY != -0.75)))
            return false;
        const auto* boundary = dynamic_cast<const DRW_LWPolyline*>(
            entity.looplist[0]->objlist.front().get());
        const auto* lineEdge = dynamic_cast<const DRW_Line*>(
            entity.looplist[1]->objlist.front().get());
        const auto* arcEdge = dynamic_cast<const DRW_Arc*>(
            entity.looplist[2]->objlist.front().get());
        const auto* ellipseEdge = dynamic_cast<const DRW_Ellipse*>(
            entity.looplist[3]->objlist.front().get());
        const auto* splineEdge = dynamic_cast<const DRW_Spline*>(
            entity.looplist[4]->objlist.front().get());
        return boundary != nullptr && lineEdge != nullptr && arcEdge != nullptr
            && ellipseEdge != nullptr && splineEdge != nullptr
            && lineEdge->basePoint.x == 1.0 && lineEdge->secPoint.y == 6.0
            && arcEdge->basePoint.x == 2.0 && arcEdge->radious == 4.0
            && arcEdge->staangle == 0.125 && arcEdge->endangle == 1.25
            && ellipseEdge->basePoint.x == 5.0 && ellipseEdge->secPoint.x == 3.0
            && ellipseEdge->ratio == 0.5 && ellipseEdge->staparam == 0.25
            && ellipseEdge->endparam == 1.5
            && splineEdge->degree == 2
            && splineEdge->knotslist
                == std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}
            && splineEdge->weightlist == std::vector<double>{1.0, 1.25, 1.0}
            && boundary->flags == 1
            && boundary->vertlist.size() == 4u
            && boundary->vertlist[0]->x == 0.0
            && boundary->vertlist[1]->x == 2.0
            && boundary->vertlist[2]->y == 3.0
            && boundary->vertlist[3]->x == 0.0;
    }

    dxfRW& output;
    dx_data storage;
    DRW_Hatch hatch;
    DRW_MPolygon mpolygon;
    bool useMPolygon;
    bool useGradient;
    std::shared_ptr<DRW_Line> line;
    std::shared_ptr<DRW_Arc> arc;
    std::shared_ptr<DRW_Ellipse> ellipse;
    std::shared_ptr<DRW_Spline> spline;
    bool entitiesWritten = false;
};

bool runHatchVersionTransactionSweep(const std::filesystem::path& directory) {
    const auto fail = [](DRW::Version version, bool binary,
                         const char* message) {
        std::cerr << "HATCH transaction " << static_cast<int>(version)
                  << (binary ? " Binary: " : " ASCII: ") << message << '\n';
        return false;
    };
    std::error_code directoryError;
    if (!std::filesystem::create_directories(directory, directoryError)
        && !std::filesystem::is_directory(directory)) {
        std::cerr << "HATCH transaction: could not create scratch directory\n";
        return false;
    }
    const auto output = directory / "hatch-version-transaction.dxf";
    const std::string sentinel = "prior HATCH drawing\n";
    const std::vector<DRW::Version> versions = {
        DRW::AC1014, DRW::AC1015, DRW::AC1018,
        DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032};
    for (const bool useMPolygon : {false, true}) {
      for (const DRW::Version version : versions) {
        for (const bool useGradient : {false, true}) {
          if (useGradient && version < DRW::AC1018)
              continue;
        for (const bool binary : {false, true}) {
            {
                dxfRW codec(output.string().c_str());
                HatchOnlyExport adapter(codec, useMPolygon, useGradient);
                const bool wrote = codec.write(&adapter, version, binary);
                if (!wrote || !adapter.entitiesWritten
                    || !adapter.sourceUnchanged()) {
                    std::cerr << "warm-up wrote=" << wrote
                              << " callback=" << adapter.entitiesWritten
                              << " source=" << adapter.sourceUnchanged()
                              << " error=" << codec.getError() << '\n';
                    return fail(version, binary, "warm-up write failed");
                }
            }
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                HatchOnlyExport adapter(codec, useMPolygon, useGradient);
                armAllocationFailure(1000000000L);
                const bool wrote = codec.write(&adapter, version, binary);
                allocationCount = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
                if (!wrote || !adapter.entitiesWritten
                    || !adapter.sourceUnchanged()
                    || !readBytes(output, expectedBytes))
                    return fail(version, binary, "calibration failed");
            }
            if (allocationCount <= 0)
                return fail(version, binary, "calibration observed no allocations");

            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                                       std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior)
                        return fail(version, binary,
                                    "could not seed destination sentinel");
                }
                dxfRW codec(output.string().c_str());
                HatchOnlyExport adapter(codec, useMPolygon, useGradient);
                armAllocationFailure(offset);
                const bool wrote = codec.write(&adapter, version, binary);
                const std::size_t reached = allocationAttempts;
                const bool injected = allocationFailed;
                disarmAllocationFailure();
                std::string afterFailure;
                if (wrote || !injected
                    || reached != static_cast<std::size_t>(offset + 1)
                    || !readBytes(output, afterFailure)
                    || afterFailure != sentinel || !adapter.sourceUnchanged()
                    || hasStagedOutput(output))
                    return fail(version, binary,
                                "failure changed caller or destination state");

                const bool retried = codec.write(&adapter, version, binary);
                std::string retryBytes;
                Readback readback;
                dxfRW reader(output.string().c_str());
                const bool read = reader.read(&readback, false);
                const DRW_Hatch& typedEntity = useMPolygon
                    ? static_cast<const DRW_Hatch&>(readback.mpolygon)
                    : readback.hatch;
                if (!retried || !adapter.entitiesWritten
                    || !adapter.sourceUnchanged()
                    || !readBytes(output, retryBytes)
                    || retryBytes != expectedBytes || !read
                    || (useMPolygon ? readback.mpolygonCount != 1u
                                    : readback.hatchCount != 1u)
                    || typedEntity.name != "ANSI31"
                    || typedEntity.solid != (useGradient ? 1 : 0)
                    || typedEntity.handle == 0u
                    || (!useGradient && (typedEntity.angle != 0.25
                        || typedEntity.scale != 1.5))
                    || typedEntity.looplist.size() != 5u
                    || std::any_of(typedEntity.looplist.begin(),
                        typedEntity.looplist.end(), [](const auto& loop) {
                            return !loop || loop->objlist.size() != 1u;
                        })
                    || dynamic_cast<const DRW_LWPolyline*>(
                        typedEntity.looplist.front()->objlist.front().get())
                        == nullptr
                    || typedEntity.patternLines.size()
                        != (useGradient ? 0u : 1u)
                    || (!useGradient
                        && (typedEntity.patternLines.front().angle != 0.5
                            || typedEntity.patternLines.front().baseX != 0.25
                            || typedEntity.patternLines.front().baseY != -0.5
                            || typedEntity.patternLines.front().offsetY != 1.25
                            || typedEntity.patternLines.front().dashList
                                != std::vector<double>{0.5, -0.25, 1.0}))
                    || typedEntity.seedPoints.size() != 1u
                    || typedEntity.seedPoints.front().x != 0.5
                    || typedEntity.seedPoints.front().y != 0.75
                    || typedEntity.isGradient != (useGradient ? 1 : 0)
                    || (useGradient && (typedEntity.gradName != "LINEAR"
                        || typedEntity.gradReserved != 0
                        || typedEntity.gradAngle != 0.75
                        || typedEntity.gradShift != 0.25
                        || typedEntity.singleColor != 0
                        || typedEntity.gradTint != 0.0
                        || typedEntity.gradColors.size() != 2u
                        || typedEntity.gradColors[0].value != 0.0
                        || typedEntity.gradColors[0].rgb != 0x112233
                        || typedEntity.gradColors[1].value != 1.0
                        || typedEntity.gradColors[1].rgb != 0x445566))
                    || (useMPolygon && (readback.mpolygon.dxfVersion != 1
                        || readback.mpolygon.fillColorAci != 5
                        || readback.mpolygon.annotatedBoundary != 1
                        || readback.mpolygon.xDirX != 1.5
                        || readback.mpolygon.xDirY != -0.75))
                    || hasStagedOutput(output)) {
                    std::cerr << "retry=" << retried << " read=" << read
                              << " readError=" << reader.getError()
                              << " hatchCount=" << readback.hatchCount
                              << " mpolygonCount=" << readback.mpolygonCount
                              << " gradient=" << typedEntity.isGradient
                              << " gradientName=" << typedEntity.gradName
                              << " gradientStops=" << typedEntity.gradColors.size()
                              << " patternLines=" << typedEntity.patternLines.size()
                              << " solid=" << typedEntity.solid
                              << " outputBytesMatch="
                              << (readBytes(output, retryBytes)
                                  && retryBytes == expectedBytes) << '\n';
                    return fail(version, binary,
                                "retry or typed readback failed");
                }
                const auto* boundary = dynamic_cast<const DRW_LWPolyline*>(
                    typedEntity.looplist[0]->objlist.front().get());
                const auto* lineEdge = dynamic_cast<const DRW_Line*>(
                    typedEntity.looplist[1]->objlist.front().get());
                const auto* arcEdge = dynamic_cast<const DRW_Arc*>(
                    typedEntity.looplist[2]->objlist.front().get());
                const auto* ellipseEdge = dynamic_cast<const DRW_Ellipse*>(
                    typedEntity.looplist[3]->objlist.front().get());
                const auto* splineEdge = dynamic_cast<const DRW_Spline*>(
                    typedEntity.looplist[4]->objlist.front().get());
                if (boundary->flags != 1 || boundary->vertlist.size() != 4u
                    || boundary->vertlist[0]->x != 0.0
                    || boundary->vertlist[1]->x != 2.0
                    || boundary->vertlist[2]->y != 3.0
                    || boundary->vertlist[3]->x != 0.0
                    || lineEdge == nullptr || lineEdge->basePoint.x != 1.0
                    || lineEdge->secPoint.y != 6.0 || arcEdge == nullptr
                    || arcEdge->basePoint.x != 2.0 || arcEdge->radious != 4.0
                    || arcEdge->isccw != 1 || ellipseEdge == nullptr
                    || ellipseEdge->basePoint.x != 5.0
                    || ellipseEdge->ratio != 0.5
                    || ellipseEdge->staparam != 0.25
                    || ellipseEdge->endparam != 1.5 || splineEdge == nullptr
                    || splineEdge->degree != 2
                    || splineEdge->knotslist
                        != std::vector<double>{0.0, 0.0, 0.0, 1.0, 1.0, 1.0}
                    || splineEdge->weightlist
                        != std::vector<double>{1.0, 1.25, 1.0}
                    || splineEdge->fitlist.size() != 1u
                    || splineEdge->fitlist.front()->x != 1.5)
                    return fail(version, binary,
                                "typed boundary readback mismatch");
            }
            std::cout << (useMPolygon ? "MPOLYGON " : "HATCH ")
                      << (useGradient ? "gradient " : "pattern ")
                      << static_cast<int>(version)
                      << (binary ? " Binary" : " ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
        }
        }
      }
    }
    std::error_code ignored;
    std::filesystem::remove(output, ignored);
    return true;
}

bool runEmbeddedAttributeMTextSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "R2018 embedded ATTRIB/MTEXT allocation transaction: "
                  << message << '\n';
        return false;
    };
    std::error_code directoryError;
    if (!std::filesystem::create_directories(directory, directoryError)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const auto output = directory / "r2018-embedded-attribute-mtext.dxf";
    const std::string sentinel = "previous complete attribute drawing\n";
    const auto sourceUnchanged = [](const EmbeddedAttributeMTextExport& adapter) {
        if (adapter.insert.handle != 0xDA01u
            || adapter.insert.name != "WHOLE_OPERATION_ATTRIBUTE_MTEXT_BLOCK"
            || adapter.insert.basePoint.x != 1.0
            || adapter.insert.basePoint.y != 2.0
            || adapter.insert.basePoint.z != 0.0
            || adapter.insert.attlist.size() != 1u
            || !adapter.insert.attlist.front()
            || adapter.attdef.handle != 0xDA03u
            || adapter.attdef.tag != "WHOLE_OPERATION_R2018_MULTILINE_ATTDEF"
            || adapter.attdef.prompt != "R2018 multiline definition prompt"
            || adapter.attdef.text != "definition fallback"
            || adapter.attdef.basePoint.x != 7.0
            || adapter.attdef.basePoint.y != 8.0
            || adapter.attdef.secPoint.x != 9.0
            || adapter.attdef.secPoint.y != 10.0
            || adapter.attdef.height != 3.5
            || adapter.attdef.style != "STANDARD"
            || adapter.attdef.mtext == nullptr
            || adapter.attdef.mtext->text
                != "definition first paragraph\\Psecond paragraph"
            || adapter.attdef.mtext->height != 3.5
            || adapter.attdef.mtext->style != "STANDARD")
            return false;
        const DRW_Attrib& attribute = *adapter.insert.attlist.front();
        return attribute.handle == 0xDA02u
            && attribute.tag == "R2018_MULTILINE_ATTRIBUTE"
            && attribute.text == "attribute fallback"
            && attribute.m_attributeType == 2u
            && attribute.basePoint.x == 3.0
            && attribute.basePoint.y == 4.0
            && attribute.basePoint.z == 0.0
            && attribute.secPoint.x == 5.0
            && attribute.secPoint.y == 6.0
            && attribute.height == 2.5
            && attribute.style == "STANDARD"
            && attribute.mtext != nullptr
            && attribute.mtext->text
                == "embedded attribute first paragraph\\Psecond paragraph"
            && attribute.mtext->basePoint.x == 3.0
            && attribute.mtext->basePoint.y == 4.0
            && attribute.mtext->height == 2.5
            && attribute.mtext->style == "STANDARD";
    };
    const auto readbackMatches = [](const std::filesystem::path& path) {
        dxfRW reader(path.string().c_str());
        Readback adapter;
        const bool read = reader.read(&adapter, false);
        if (!read || adapter.insertCount != 1u
            || adapter.insert.name != "WHOLE_OPERATION_ATTRIBUTE_MTEXT_BLOCK"
            || adapter.insert.attlist.size() != 1u
            || !adapter.insert.attlist.front()
            || adapter.directAttdefCount != 1u
            || adapter.directAttdef.tag
                != "WHOLE_OPERATION_R2018_MULTILINE_ATTDEF") {
            std::cerr << "readback read=" << read
                      << " error=" << reader.getError()
                      << " insertCount=" << adapter.insertCount
                      << " name=" << adapter.insert.name
                      << " attributes=" << adapter.insert.attlist.size() << '\n';
            return false;
        }
        const DRW_Attrib& attribute = *adapter.insert.attlist.front();
        const bool matches = attribute.tag == "R2018_MULTILINE_ATTRIBUTE"
            && attribute.text == "attribute fallback"
            && attribute.m_attributeType == 2u
            && attribute.mtext != nullptr
            && attribute.mtext->text
                == "embedded attribute first paragraph\\Psecond paragraph"
            && attribute.mtext->basePoint.x == 3.0
            && attribute.mtext->basePoint.y == 4.0
            && attribute.mtext->height == 2.5
            && attribute.mtext->style == "STANDARD";
        const DRW_Attdef& attdef = adapter.directAttdef;
        const bool attdefMatches =
            attdef.m_attributeType == 4u
            && attdef.text == "definition fallback"
            && attdef.mtext != nullptr
            && attdef.mtext->text
                == "definition first paragraph\\Psecond paragraph"
            && attdef.mtext->basePoint.x == 7.0
            && attdef.mtext->basePoint.y == 8.0
            && attdef.mtext->height == 3.5
            && attdef.mtext->style == "STANDARD";
        if (!matches || !attdefMatches)
            std::cerr << "attribute tag=" << attribute.tag
                      << " fallback=" << attribute.text
                      << " type=" << static_cast<unsigned>(attribute.m_attributeType)
                      << " mtext=" << (attribute.mtext != nullptr)
                      << (attribute.mtext ? " text=" + attribute.mtext->text
                                             : std::string{})
                      << (attribute.mtext ? " height=" + std::to_string(attribute.mtext->height)
                                             : std::string{})
                      << (attribute.mtext ? " style=" + attribute.mtext->style
                                             : std::string{})
                      << " attdef type="
                      << static_cast<unsigned>(attdef.m_attributeType)
                      << " attdef text="
                      << (attdef.mtext ? attdef.mtext->text : std::string{})
                      << '\n';
        return matches && attdefMatches;
    };

    for (const bool binary : {false, true}) {
        if (!clearStagedOutput(output))
            return fail("could not clear output before warm-up");
        {
            dxfRW codec(output.string().c_str());
            EmbeddedAttributeMTextExport adapter(codec);
            const bool wrote = codec.write(&adapter, DRW::AC1032, binary);
            const bool unchanged = sourceUnchanged(adapter);
            const bool readback = wrote && readbackMatches(output);
            if (!wrote || !adapter.entitiesWritten || !unchanged || !readback) {
                std::cerr << "warm-up binary=" << binary << " wrote=" << wrote
                          << " entities=" << adapter.entitiesWritten
                          << " source=" << unchanged
                          << " readback=" << readback
                          << " error=" << codec.getError() << '\n';
                return fail("AC1032 warm-up or typed readback failed");
            }
        }
        std::error_code removeError;
        std::filesystem::remove(output, removeError);

        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW codec(output.string().c_str());
            EmbeddedAttributeMTextExport adapter(codec);
            armAllocationFailure(1000000000L);
            const bool wrote = codec.write(&adapter, DRW::AC1032, binary);
            allocationCount = static_cast<long>(allocationAttempts);
            disarmAllocationFailure();
            if (!wrote || !adapter.entitiesWritten
                || !sourceUnchanged(adapter) || !readbackMatches(output)
                || !readBytes(output, expectedBytes))
                return fail("allocation calibration or readback failed");
        }
        if (allocationCount <= 0)
            return fail("calibration observed no allocations");

        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream prior(output,
                                   std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior)
                    return fail("could not seed destination sentinel");
            }
            dxfRW codec(output.string().c_str());
            EmbeddedAttributeMTextExport adapter(codec);
            armAllocationFailure(offset);
            const bool wrote = codec.write(&adapter, DRW::AC1032, binary);
            const std::size_t reached = allocationAttempts;
            const bool injected = allocationFailed;
            disarmAllocationFailure();
            std::string afterFailure;
            if (!injected || wrote
                || reached != static_cast<std::size_t>(offset + 1)
                || !readBytes(output, afterFailure) || afterFailure != sentinel
                || !sourceUnchanged(adapter) || hasStagedOutput(output))
                return fail("injected failure changed caller or destination state");

            const bool retried = codec.write(&adapter, DRW::AC1032, binary);
            std::string retryBytes;
            if (!retried || !adapter.entitiesWritten
                || !sourceUnchanged(adapter) || !readBytes(output, retryBytes)
                || retryBytes != expectedBytes || !readbackMatches(output)
                || hasStagedOutput(output))
                return fail("same-codec retry did not reproduce typed output");
        }
        std::cout << "R2018 embedded ATTRIB/MTEXT "
                  << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", caller/destination rollback, retry/readback=PASS\n";
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
    }
    return true;
}

bool runPolylineTopologySweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "POLYLINE topology allocation transaction: "
                  << message << '\n';
        return false;
    };
    std::error_code directoryError;
    if (!std::filesystem::create_directories(directory, directoryError)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    struct Profile {
        PolylineTopologyExport::Topology topology;
        const char* name;
        int parentFlags;
        std::size_t vertexCount;
    };
    const std::array<Profile, 5> profiles{{
        {PolylineTopologyExport::Topology::ThreeDimensional,
         "3d-polyline", 8, 2u},
        {PolylineTopologyExport::Topology::ThreeDimensionalWithIdentifiers,
         "3d-polyline-identifiers", 8, 2u},
        {PolylineTopologyExport::Topology::PolygonMesh,
         "polygon-mesh", 16 | 32, 4u},
        {PolylineTopologyExport::Topology::RectangularPolygonMesh,
         "polygon-mesh-3x2-closed-both", 16 | 32 | 1, 6u},
        {PolylineTopologyExport::Topology::Polyface,
         "polyface-mesh", 64, 7u}
    }};
    const std::array<std::pair<DRW::Version, const char*>, 7> versions{{
        {DRW::AC1014, "AC1014"}, {DRW::AC1015, "AC1015"},
        {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
        {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
        {DRW::AC1032, "AC1032"}
    }};
    const std::string sentinel = "prior POLYLINE topology drawing bytes\n";

    const auto sourceUnchanged = [](const PolylineTopologyExport& adapter) {
        const int expectedFlags =
            adapter.topology == PolylineTopologyExport::Topology::ThreeDimensional
                || adapter.topology
                    == PolylineTopologyExport::Topology::ThreeDimensionalWithIdentifiers
                ? 8
                : adapter.topology
                    == PolylineTopologyExport::Topology::PolygonMesh
                    ? 16 | 32
                    : adapter.topology
                        == PolylineTopologyExport::Topology::RectangularPolygonMesh
                        ? 16 | 32 | 1 : 64;
        const std::size_t expectedVertices =
            adapter.topology == PolylineTopologyExport::Topology::ThreeDimensional
                || adapter.topology
                    == PolylineTopologyExport::Topology::ThreeDimensionalWithIdentifiers
                ? 2u
                : adapter.topology
                    == PolylineTopologyExport::Topology::PolygonMesh
                    ? 4u
                    : adapter.topology
                        == PolylineTopologyExport::Topology::RectangularPolygonMesh
                        ? 6u : 7u;
        const int expectedM =
            adapter.topology == PolylineTopologyExport::Topology::PolygonMesh
                ? 2
                : adapter.topology
                    == PolylineTopologyExport::Topology::RectangularPolygonMesh
                    ? 3
                    : adapter.topology == PolylineTopologyExport::Topology::Polyface
                        ? 5 : 0;
        const int expectedN = expectedM == 0 ? 0 : 2;
        if (adapter.polyline.handle != 0xDB00u
            || adapter.polyline.flags != expectedFlags
            || adapter.polyline.vertexcount != expectedM
            || adapter.polyline.facecount != expectedN
            || adapter.polyline.vertlist.size() != expectedVertices
            || adapter.polyline.basePoint.x != 0.0
            || adapter.polyline.basePoint.y != 0.0
            || adapter.polyline.basePoint.z != 2.5)
            return false;
        for (std::size_t i = 0; i < adapter.polyline.vertlist.size(); ++i) {
            const auto& vertex = adapter.polyline.vertlist[i];
            if (!vertex || vertex->handle != 0xDB01u + i)
                return false;
            if (adapter.topology
                    == PolylineTopologyExport::Topology::Polyface
                && i >= 5u) {
                const int expectedIndex = i == 5u ? -1 : 2;
                if (vertex->vindex1 != expectedIndex
                    || vertex->vindex2 != (i == 5u ? 2 : -4)
                    || vertex->vindex3 != (i == 5u ? -3 : 5)
                    || vertex->vindex4 != 0)
                    return false;
                continue;
            }
            double expectedX = 1.0 + 3.0 * i;
            double expectedY = 2.0 + 3.0 * i;
            double expectedZ = 3.0 + 3.0 * i;
            if (adapter.topology
                    == PolylineTopologyExport::Topology::PolygonMesh
                || adapter.topology
                    == PolylineTopologyExport::Topology::RectangularPolygonMesh) {
                const std::size_t columns = adapter.topology
                        == PolylineTopologyExport::Topology::PolygonMesh
                    ? 2u : 3u;
                expectedX = 1.0 + static_cast<double>(i % columns);
                expectedY = 2.0 + static_cast<double>(i / columns);
            } else if (adapter.topology
                           == PolylineTopologyExport::Topology::Polyface) {
                expectedX = 1.0 + static_cast<double>(i);
                expectedY = 2.0 + static_cast<double>(i);
                expectedZ = 3.0 + static_cast<double>(i);
            }
            if (vertex->basePoint.x != expectedX
                || vertex->basePoint.y != expectedY
                || vertex->basePoint.z != expectedZ)
                return false;
            if (adapter.topology
                    == PolylineTopologyExport::Topology::ThreeDimensionalWithIdentifiers
                && vertex->identifier != (i == 0u ? 101 : 202))
                return false;
            if (adapter.topology
                    != PolylineTopologyExport::Topology::ThreeDimensionalWithIdentifiers
                && vertex->identifier != 0)
                return false;
        }
        return true;
    };

    const auto refusalPath = directory / "AC1021-polyline-identifiers-refused.dxf";
    {
        std::ofstream prior(refusalPath, std::ios::binary | std::ios::trunc);
        prior << sentinel;
        if (!prior)
            return fail("could not seed pre-AC1024 refusal sentinel");
    }
    dxfRW refusalCodec(refusalPath.string().c_str());
    PolylineTopologyExport refusalAdapter(refusalCodec,
        PolylineTopologyExport::Topology::ThreeDimensionalWithIdentifiers);
    const bool refusedWrite =
        refusalCodec.write(&refusalAdapter, DRW::AC1021, false);
    if (refusedWrite || !sourceUnchanged(refusalAdapter)) {
        std::cerr << "pre-AC1024 identifier refusal: wrote=" << refusedWrite
                  << " error=" << refusalCodec.getError() << '\n';
        return fail("pre-AC1024 VERTEX identifiers were not refused cleanly");
    }
    std::string refusalBytes;
    if (!readBytes(refusalPath, refusalBytes) || refusalBytes != sentinel)
        return fail("pre-AC1024 identifier refusal changed destination");
    std::error_code refusalCleanup;
    std::filesystem::remove(refusalPath, refusalCleanup);

    for (const Profile& profile : profiles) {
      for (const auto& version : versions) {
        if (profile.topology
                == PolylineTopologyExport::Topology::ThreeDimensionalWithIdentifiers
            && version.first < DRW::AC1024)
            continue;
        const auto output = directory
            / (std::string(version.second) + "-" + profile.name + ".dxf");
        for (const bool binary : {false, true}) {
            if (!clearStagedOutput(output))
                return fail("could not clear output before calibration");
            std::string cleanBytes;
            long allocationCount = 0;
            {
                dxfRW codec(output.string().c_str());
                PolylineTopologyExport adapter(codec, profile.topology);
                if (!codec.write(&adapter, version.first, binary)
                    || !adapter.written || !sourceUnchanged(adapter))
                    return fail("clean whole-write baseline failed");
                PolylineTopologyReadback readback;
                dxfRW reader(output.string().c_str());
                if (!reader.read(&readback, false)
                    || readback.polylines.size() != 1u)
                    return fail("baseline typed readback failed");
                const auto& actual = readback.polylines.front();
                if (actual.flags != profile.parentFlags
                    || actual.vertlist.size() != profile.vertexCount
                    || actual.handle == 0u
                    || actual.vertexcount
                        != (profile.topology
                                == PolylineTopologyExport::Topology::PolygonMesh
                            ? 2
                            : profile.topology
                                == PolylineTopologyExport::Topology::RectangularPolygonMesh
                                ? 3
                                : profile.topology
                                    == PolylineTopologyExport::Topology::Polyface
                                    ? 5 : 0)
                    || actual.facecount
                        != (profile.topology
                                == PolylineTopologyExport::Topology::PolygonMesh
                            || profile.topology
                                == PolylineTopologyExport::Topology::RectangularPolygonMesh
                            || profile.topology
                                == PolylineTopologyExport::Topology::Polyface
                            ? 2 : 0))
                    return fail("baseline topology header mismatch");
                if (profile.topology
                    == PolylineTopologyExport::Topology::Polyface) {
                    for (std::size_t i = 0; i < 5u; ++i) {
                        const auto& coordinate = actual.vertlist[i];
                        if (!coordinate || coordinate->handle == 0u
                            || coordinate->basePoint.x != 1.0 + i
                            || coordinate->basePoint.y != 2.0 + i
                            || coordinate->basePoint.z != 3.0 + i)
                            return fail("polyface coordinate-pool readback mismatch");
                    }
                    const auto& firstFace = actual.vertlist[5];
                    const auto& secondFace = actual.vertlist[6];
                    if (!firstFace || !secondFace
                        || (firstFace->flags & 128) == 0
                        || firstFace->vindex1 != -1
                        || firstFace->vindex2 != 2
                        || firstFace->vindex3 != -3
                        || firstFace->vindex4 != 0
                        || (secondFace->flags & 128) == 0
                        || secondFace->vindex1 != 2
                        || secondFace->vindex2 != -4
                        || secondFace->vindex3 != 5
                        || secondFace->vindex4 != 0)
                        return fail("polyface face indices/edge visibility mismatch");
                } else {
                    for (std::size_t i = 0; i < actual.vertlist.size(); ++i) {
                        const auto& vertex = actual.vertlist[i];
                        double expectedX = 1.0 + 3.0 * i;
                        double expectedY = 2.0 + 3.0 * i;
                        if (profile.topology
                                == PolylineTopologyExport::Topology::PolygonMesh
                            || profile.topology
                                == PolylineTopologyExport::Topology::RectangularPolygonMesh) {
                            const std::size_t columns =
                                profile.topology
                                    == PolylineTopologyExport::Topology::PolygonMesh
                                    ? 2u : 3u;
                            expectedX = 1.0 + static_cast<double>(i % columns);
                            expectedY = 2.0 + static_cast<double>(i / columns);
                        }
                        if (!vertex || vertex->handle == 0u
                            || vertex->basePoint.x != expectedX
                            || vertex->basePoint.y != expectedY
                            || vertex->basePoint.z != 3.0 + 3.0 * i)
                            return fail("coordinate-pool readback mismatch");
                        if (profile.topology
                                == PolylineTopologyExport::Topology::ThreeDimensionalWithIdentifiers
                            && vertex->identifier != (i == 0u ? 101 : 202))
                            return fail("AC1024+ VERTEX identifier readback mismatch");
                    }
                }
                if (!readBytes(output, cleanBytes) || cleanBytes.empty())
                    return fail("could not capture clean baseline bytes");
            }
            std::error_code removeError;
            std::filesystem::remove(output, removeError);

            {
                dxfRW codec(output.string().c_str());
                PolylineTopologyExport adapter(codec, profile.topology);
                armAllocationFailure(1000000000L);
                const bool wrote = codec.write(&adapter, version.first, binary);
                allocationCount = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
                if (!wrote || !adapter.written || !sourceUnchanged(adapter))
                    return fail("allocation calibration write failed");
            }
            if (allocationCount <= 0)
                return fail("calibration observed no allocations");

            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior)
                        return fail("could not seed destination sentinel");
                }
                dxfRW codec(output.string().c_str());
                PolylineTopologyExport adapter(codec, profile.topology);
                armAllocationFailure(offset);
                const bool wrote = codec.write(&adapter, version.first, binary);
                const std::size_t reached = allocationAttempts;
                const bool injected = allocationFailed;
                disarmAllocationFailure();
                std::string afterFailure;
                if (!injected || wrote
                    || reached != static_cast<std::size_t>(offset + 1)
                    || !readBytes(output, afterFailure)
                    || afterFailure != sentinel
                    || !sourceUnchanged(adapter) || hasStagedOutput(output))
                    return fail("injected failure changed caller or destination");

                if (!codec.write(&adapter, version.first, binary)
                    || !adapter.written || !sourceUnchanged(adapter))
                    return fail("same-codec retry failed");
                std::string retryBytes;
                if (!readBytes(output, retryBytes) || retryBytes != cleanBytes)
                    return fail("retry bytes differ from clean baseline");
                PolylineTopologyReadback readback;
                dxfRW reader(output.string().c_str());
                if (!reader.read(&readback, false)
                    || readback.polylines.size() != 1u
                    || readback.polylines.front().flags != profile.parentFlags
                    || readback.polylines.front().vertlist.size()
                        != profile.vertexCount
                    || readback.polylines.front().vertexcount
                        != (profile.topology
                                == PolylineTopologyExport::Topology::PolygonMesh
                            ? 2
                            : profile.topology
                                == PolylineTopologyExport::Topology::RectangularPolygonMesh
                                ? 3
                                : profile.topology
                                    == PolylineTopologyExport::Topology::Polyface
                                    ? 5 : 0)
                    || readback.polylines.front().facecount
                        != (profile.topology
                                == PolylineTopologyExport::Topology::PolygonMesh
                            || profile.topology
                                == PolylineTopologyExport::Topology::RectangularPolygonMesh
                            || profile.topology
                                == PolylineTopologyExport::Topology::Polyface
                            ? 2 : 0))
                    return fail("retry typed readback mismatch");
                if (profile.topology
                        == PolylineTopologyExport::Topology::ThreeDimensionalWithIdentifiers
                    && (readback.polylines.front().vertlist[0]->identifier != 101
                        || readback.polylines.front().vertlist[1]->identifier != 202))
                    return fail("retry VERTEX identifier readback mismatch");
            }
            std::cout << version.second << ' ' << profile.name << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
      }
    }
    return true;
}

bool runLegacyCompoundEntitiesSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "legacy compound entity allocation transaction: "
                  << message << '\n';
        return false;
    };
    std::error_code directoryError;
    if (!std::filesystem::create_directories(directory, directoryError)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const std::string sentinel = "prior legacy attributed drawing\n";
    const auto sourceUnchanged = [](const LegacyCompoundEntitiesExport& adapter) {
        if (adapter.polyline.handle != 0xDA30u
            || adapter.polyline.flags != 0
            || adapter.polyline.basePoint.x != 0.0
            || adapter.polyline.basePoint.y != 0.0
            || adapter.polyline.basePoint.z != 2.5
            || adapter.polyline.vertlist.size() != 2u
            || !adapter.polyline.vertlist[0] || !adapter.polyline.vertlist[1]
            || adapter.polyline.vertlist[0]->handle != 0xDA31u
            || adapter.polyline.vertlist[0]->basePoint.x != 19.0
            || adapter.polyline.vertlist[0]->basePoint.y != 20.0
            || adapter.polyline.vertlist[0]->bulge != 0.375
            || adapter.polyline.vertlist[1]->handle != 0xDA32u
            || adapter.polyline.vertlist[1]->basePoint.x != 22.0
            || adapter.polyline.vertlist[1]->basePoint.y != 24.0
            || adapter.insert.handle != 0xDA20u
            || adapter.insert.name != "LEGACY_ATTRIBUTED_INSERT_BLOCK"
            || !adapter.insert.isMInsert()
            || adapter.insert.colcount != 2 || adapter.insert.rowcount != 3
            || adapter.insert.colspace != 4.5 || adapter.insert.rowspace != 5.5
            || adapter.insert.attlist.size() != 2u)
            return false;
        for (unsigned i = 0; i < 2u; ++i) {
            const auto& ptr = adapter.insert.attlist[i];
            if (!ptr || ptr->handle != 0xDA21u + i
                || ptr->tag != (i == 0u ? "LEGACY_TAG_A" : "LEGACY_TAG_B")
                || ptr->text != (i == 0u ? "legacy value alpha" : "legacy value beta")
                || ptr->basePoint.x != 13.0 + i
                || ptr->basePoint.y != 14.0 + i
                || ptr->height != 2.0 + i || ptr->style != "STANDARD"
                || ptr->mtext)
                return false;
        }
        return true;
    };
    const auto readbackMatches = [](const std::filesystem::path& path) {
        dxfRW reader(path.string().c_str());
        Readback adapter;
        const bool read = reader.read(&adapter, false);
        if (!read || adapter.polyline3Count != 1u
            || adapter.polyline3.handle == 0u
            || adapter.polyline3.flags != 0
            || adapter.polyline3.basePoint.x != 0.0
            || adapter.polyline3.basePoint.y != 0.0
            || adapter.polyline3.basePoint.z != 2.5
            || adapter.polyline3.vertlist.size() != 2u
            || !adapter.polyline3.vertlist[0] || !adapter.polyline3.vertlist[1]
            || adapter.polyline3.vertlist[0]->handle == 0u
            || adapter.polyline3.vertlist[0]->handle == adapter.polyline3.handle
            || adapter.polyline3.vertlist[0]->basePoint.x != 19.0
            || adapter.polyline3.vertlist[0]->basePoint.y != 20.0
            || adapter.polyline3.vertlist[0]->bulge != 0.375
            || adapter.polyline3.vertlist[1]->handle == 0u
            || adapter.polyline3.vertlist[1]->handle == adapter.polyline3.handle
            || adapter.polyline3.vertlist[1]->handle
                == adapter.polyline3.vertlist[0]->handle
            || adapter.polyline3.vertlist[1]->basePoint.x != 22.0
            || adapter.polyline3.vertlist[1]->basePoint.y != 24.0
            || !adapter.polyline3.seqEndEntity
            || adapter.polyline3.seqEndEntity->handle == 0u
            || adapter.insertCount != 1u
            || adapter.insert.name != "LEGACY_ATTRIBUTED_INSERT_BLOCK"
            || !adapter.insert.isMInsert()
            || adapter.insert.colcount != 2 || adapter.insert.rowcount != 3
            || adapter.insert.colspace != 4.5 || adapter.insert.rowspace != 5.5
            || adapter.insert.attlist.size() != 2u) {
            std::cerr << "legacy compound readback read=" << read
                      << " error=" << reader.getError()
                      << " polylineCount=" << adapter.polyline3Count
                      << " polylineHandle=" << adapter.polyline3.handle
                      << " vertices=" << adapter.polyline3.vertlist.size()
                      << " insertCount=" << adapter.insertCount
                      << " attributes=" << adapter.insert.attlist.size()
                      << '\n';
            return false;
        }
        for (unsigned i = 0; i < 2u; ++i) {
            const auto& ptr = adapter.insert.attlist[i];
            if (!ptr || ptr->tag != (i == 0u ? "LEGACY_TAG_A" : "LEGACY_TAG_B")
                || ptr->text != (i == 0u ? "legacy value alpha" : "legacy value beta")
                || ptr->basePoint.x != 13.0 + i
                || ptr->basePoint.y != 14.0 + i
                || ptr->height != 2.0 + i || ptr->style != "STANDARD"
                || ptr->mtext)
                return false;
        }
        return true;
    };

    for (const DRW::Version version : {
             DRW::AC1014, DRW::AC1015, DRW::AC1018,
             DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
      const char* versionName = version == DRW::AC1014 ? "AC1014"
          : version == DRW::AC1015 ? "AC1015"
          : version == DRW::AC1018 ? "AC1018"
          : version == DRW::AC1021 ? "AC1021"
          : version == DRW::AC1024 ? "AC1024"
          : version == DRW::AC1027 ? "AC1027" : "AC1032";
      const auto output = directory
          / (std::string(versionName) + "-attributed-insert.dxf");
      for (const bool binary : {false, true}) {
        if (!clearStagedOutput(output))
            return fail("could not clear output before warm-up");
        {
            dxfRW codec(output.string().c_str());
            LegacyCompoundEntitiesExport adapter(codec);
            const bool wrote = codec.write(&adapter, version, binary);
            const bool unchanged = sourceUnchanged(adapter);
            const bool readback = wrote && readbackMatches(output);
            if (!wrote || !adapter.entitiesWritten || !unchanged || !readback) {
                std::cerr << versionName << " warm-up binary=" << binary
                          << " wrote=" << wrote
                          << " entities=" << adapter.entitiesWritten
                          << " unchanged=" << unchanged
                          << " readback=" << readback
                          << " error=" << codec.getError() << '\n';
                return fail("version warm-up or typed readback failed");
            }
        }
        std::error_code removeError;
        std::filesystem::remove(output, removeError);

        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW codec(output.string().c_str());
            LegacyCompoundEntitiesExport adapter(codec);
            armAllocationFailure(1000000000L);
            const bool wrote = codec.write(&adapter, version, binary);
            allocationCount = static_cast<long>(allocationAttempts);
            disarmAllocationFailure();
            if (!wrote || !adapter.entitiesWritten
                || !sourceUnchanged(adapter) || !readbackMatches(output)
                || !readBytes(output, expectedBytes))
                return fail("allocation calibration or readback failed");
        }
        if (allocationCount <= 0)
            return fail("calibration observed no allocations");

        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior)
                    return fail("could not seed destination sentinel");
            }
            dxfRW codec(output.string().c_str());
            LegacyCompoundEntitiesExport adapter(codec);
            armAllocationFailure(offset);
            const bool wrote = codec.write(&adapter, version, binary);
            const std::size_t reached = allocationAttempts;
            const bool injected = allocationFailed;
            disarmAllocationFailure();
            std::string afterFailure;
            if (!injected || wrote
                || reached != static_cast<std::size_t>(offset + 1)
                || !readBytes(output, afterFailure) || afterFailure != sentinel
                || !sourceUnchanged(adapter) || hasStagedOutput(output))
                return fail("injected failure changed caller or destination state");

            const bool retried = codec.write(&adapter, version, binary);
            std::string retryBytes;
            if (!retried || !adapter.entitiesWritten
                || !sourceUnchanged(adapter) || !readBytes(output, retryBytes)
                || retryBytes != expectedBytes || !readbackMatches(output)
                || hasStagedOutput(output))
                return fail("same-codec retry or typed readback failed");
        }
        std::cout << versionName << " standalone ATTRIB/SEQEND "
                  << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", caller/destination rollback, retry/readback=PASS\n";
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
      }
    }
    return true;
}

bool runDimensionVersionSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "DIMENSION version-matrix allocation transaction: "
                  << message << '\n';
        return false;
    };
    std::error_code directoryError;
    if (!std::filesystem::create_directories(directory, directoryError)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    const auto readbackMatches = [](const std::filesystem::path& path) {
        dxfRW reader(path.string().c_str());
        DimensionVersionReadback adapter;
        const bool read = reader.read(&adapter, false);
        const bool matches = read
            && adapter.alignedCount == 1u && adapter.linearCount == 1u
            && adapter.radialCount == 1u && adapter.diametricCount == 1u
            && adapter.angularCount == 1u && adapter.angular3pCount == 1u
            && adapter.ordinateCount == 1u
            && adapter.dynamicConstraintCount == 1u
            && adapter.legacyDynamicConstraintCount == 1u
            && adapter.genericDimensionCount == 1u
            && adapter.arcDimensionCount == 1u
            && adapter.largeRadialCount == 1u
            && adapter.aligned.handle != 0u
            && adapter.aligned.getStyle() == "DIM_MATRIX_STYLE"
            && adapter.aligned.getDefPoint().x == 1.25
            && adapter.aligned.getDefPoint().y == 2.5
            && adapter.aligned.getDefPoint().z == 3.75
            && adapter.aligned.getTextPoint().x == 4.5
            && adapter.aligned.getTextPoint().y == 5.75
            && adapter.aligned.getText() == "version matrix aligned"
            && adapter.linear.handle != 0u
            && adapter.linear.handle != adapter.aligned.handle
            && adapter.linear.getStyle() == "DIM_MATRIX_STYLE"
            && adapter.linear.getDef1Point().x == 10.0
            && adapter.linear.getDef1Point().y == 11.0
            && adapter.linear.getDef1Point().z == 12.0
            && adapter.linear.getDef2Point().x == 14.0
            && adapter.linear.getDef2Point().y == 16.0
            && adapter.linear.getDef2Point().z == 18.0
            && adapter.linear.getTextPoint().x == 12.0
            && adapter.linear.getTextPoint().y == 13.0
            && adapter.linear.getText() == "version matrix linear"
            && adapter.radial.handle != 0u
            && adapter.radial.getStyle() == "DIM_MATRIX_STYLE"
            && adapter.radial.getDefPoint().x == 20.0
            && adapter.radial.getDefPoint().y == 21.0
            && adapter.radial.getDefPoint().z == 22.0
            && adapter.radial.getDiameterPoint().x == 22.0
            && adapter.radial.getLeaderLength() == 3.0
            && adapter.radial.getText() == "version matrix radial"
            && adapter.diametric.handle != 0u
            && adapter.diametric.getStyle() == "DIM_MATRIX_STYLE"
            && adapter.diametric.getDefPoint().x == 30.0
            && adapter.diametric.getDiameter1Point().x == 32.0
            && adapter.diametric.getLeaderLength() == 4.0
            && adapter.diametric.getText() == "version matrix diametric"
            && adapter.angular.handle != 0u
            && adapter.angular.getStyle() == "DIM_MATRIX_STYLE"
            && adapter.angular.getDefPoint().x == 41.0
            && adapter.angular.getFirstLine1().x == 40.0
            && adapter.angular.getFirstLine2().x == 41.0
            && adapter.angular.getSecondLine1().y == 41.0
            && adapter.angular.getSecondLine2().y == 41.0
            && adapter.angular.getDimPoint().x == 40.5
            && adapter.angular.getText() == "version matrix angular"
            && adapter.angular3p.handle != 0u
            && adapter.angular3p.getStyle() == "DIM_MATRIX_STYLE"
            && adapter.angular3p.getDefPoint().x == 50.5
            && adapter.angular3p.getFirstLine().x == 50.0
            && adapter.angular3p.getSecondLine().x == 51.0
            && adapter.angular3p.getVertexPoint().y == 51.0
            && adapter.angular3p.getText() == "version matrix angular 3p"
            && adapter.ordinate.handle != 0u
            && adapter.ordinate.getStyle() == "DIM_MATRIX_STYLE"
            && adapter.ordinate.getDefPoint().x == 60.0
            && adapter.ordinate.getDefPoint().y == 61.0
            && adapter.ordinate.getDefPoint().z == 62.0
            && adapter.ordinate.getFirstLine().x == 61.0
            && adapter.ordinate.getSecondLine().x == 64.0
            && adapter.ordinate.getText() == "version matrix ordinate"
            && adapter.dynamicConstraint.handle != 0u
            && adapter.dynamicConstraint.layer == "*ADSK_CONSTRAINTS"
            && adapter.dynamicConstraint.getStyle() == "DIM_MATRIX_STYLE"
            && adapter.dynamicConstraint.getDefPoint().x == 70.0
            && adapter.dynamicConstraint.getDefPoint().y == 71.0
            && adapter.dynamicConstraint.getDefPoint().z == 72.0
            && adapter.dynamicConstraint.getText()
                == "version matrix dynamic constraint"
            && DxfDimensionDstyleTestAccess::matchesOverrides(
                adapter.dynamicConstraint)
            && adapter.legacyDynamicConstraint.handle != 0u
            && adapter.legacyDynamicConstraint.layer
                == "*ADSK_CONSTRAINTS"
            && adapter.legacyDynamicConstraint.getStyle() == "DIM_MATRIX_STYLE"
            && adapter.legacyDynamicConstraint.getDefPoint().x == 80.0
            && adapter.legacyDynamicConstraint.getDefPoint().y == 81.0
            && adapter.legacyDynamicConstraint.getDefPoint().z == 82.0
            && adapter.legacyDynamicConstraint.getText()
                == "version matrix legacy constraint"
            && adapter.genericDimension.handle != 0u
            && adapter.genericDimension.getStyle() == "DIM_MATRIX_STYLE"
            && adapter.genericDimension.getDefPoint().x == 85.0
            && adapter.genericDimension.getDefPoint().y == 86.0
            && adapter.genericDimension.getDefPoint().z == 87.0
            && adapter.genericDimension.getTextPoint().x == 85.5
            && adapter.genericDimension.getTextPoint().y == 86.5
            && adapter.genericDimension.getText()
                == "version matrix generic dimension"
            && adapter.arcDimension.handle != 0u
            && adapter.arcDimension.getStyle() == "DIM_MATRIX_STYLE"
            && adapter.arcDimension.getArcDefPoint().x == 90.0
            && adapter.arcDimension.getArcDefPoint().y == 91.0
            && adapter.arcDimension.getArcDefPoint().z == 92.0
            && adapter.arcDimension.getExtLine1().x == 91.0
            && adapter.arcDimension.getExtLine2().y == 92.0
            && adapter.arcDimension.getArcCenter().x == 90.0
            && adapter.arcDimension.getLeaderPt1().x == 92.0
            && adapter.arcDimension.leaderPt2.x == 93.0
            && adapter.arcDimension.arcStartAngle == 0.25
            && adapter.arcDimension.arcEndAngle == 1.75
            && adapter.arcDimension.isPartial
            && adapter.arcDimension.hasLeader
            && adapter.arcDimension.getText()
                == "version matrix arc dimension"
            && adapter.largeRadial.handle != 0u
            && adapter.largeRadial.getStyle() == "DIM_MATRIX_STYLE"
            && adapter.largeRadial.getCenterPoint().x == 100.0
            && adapter.largeRadial.getCenterPoint().y == 101.0
            && adapter.largeRadial.getCenterPoint().z == 102.0
            && adapter.largeRadial.getChordPoint().x == 103.0
            && adapter.largeRadial.overrideCenterPoint.x == 100.0
            && adapter.largeRadial.jogPoint.x == 104.0
            && adapter.largeRadial.jogAngle == 0.375
            && adapter.largeRadial.getText()
                == "version matrix large radial";
        if (!matches) {
            std::cerr << "DIMENSION readback diagnostic read=" << read
                      << " error=" << reader.getError()
                      << " aligned=" << adapter.alignedCount
                      << " linear=" << adapter.linearCount
                      << " radial=" << adapter.radialCount
                      << " diametric=" << adapter.diametricCount
                      << " angular=" << adapter.angularCount
                      << " angular3p=" << adapter.angular3pCount
                      << " ordinate=" << adapter.ordinateCount
                      << " dynamic=" << adapter.dynamicConstraintCount
                      << " legacyDynamic="
                      << adapter.legacyDynamicConstraintCount
                      << " generic=" << adapter.genericDimensionCount
                      << " arcDimension=" << adapter.arcDimensionCount
                      << " largeRadial=" << adapter.largeRadialCount
                      << " alignedHandle=" << adapter.aligned.handle
                      << " alignedStyle=" << adapter.aligned.getStyle()
                      << " alignedDef=" << adapter.aligned.getDefPoint().x
                      << ',' << adapter.aligned.getDefPoint().y << ','
                      << adapter.aligned.getDefPoint().z
                      << " alignedTextPoint=" << adapter.aligned.getTextPoint().x
                      << ',' << adapter.aligned.getTextPoint().y
                      << " alignedText=" << adapter.aligned.getText()
                      << " linearHandle=" << adapter.linear.handle
                      << " linearStyle=" << adapter.linear.getStyle()
                      << " linearDef1=" << adapter.linear.getDef1Point().x
                      << ',' << adapter.linear.getDef1Point().y
                      << " linearDef2=" << adapter.linear.getDef2Point().x
                      << ',' << adapter.linear.getDef2Point().y
                      << " linearTextPoint=" << adapter.linear.getTextPoint().x
                      << ',' << adapter.linear.getTextPoint().y
                      << " linearText=" << adapter.linear.getText()
                      << " dynamicLayer=" << adapter.dynamicConstraint.layer
                      << " dynamicText=" << adapter.dynamicConstraint.getText()
                      << " dynamicOverrides="
                      << DxfDimensionDstyleTestAccess::matchesOverrides(
                             adapter.dynamicConstraint)
                      << " legacyLayer="
                      << adapter.legacyDynamicConstraint.layer
                      << " legacyText="
                      << adapter.legacyDynamicConstraint.getText();
            DxfDimensionDstyleTestAccess::printOverrides(
                adapter.dynamicConstraint);
            std::cerr << '\n';
        }
        return matches;
    };
    const std::array<std::pair<DRW::Version, const char*>, 7> versions{{
        {DRW::AC1014, "AC1014"}, {DRW::AC1015, "AC1015"},
        {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
        {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
        {DRW::AC1032, "AC1032"}}};
    const std::string sentinel = "prior DIMENSION version matrix bytes\n";

    for (const auto& target : versions) {
        for (const bool binary : {false, true}) {
            const auto output = directory / (std::string(target.second)
                + (binary ? "-dimension-matrix-binary.dxf"
                          : "-dimension-matrix-ascii.dxf"));
            if (!clearStagedOutput(output))
                return fail("could not clear output before warm-up");
            {
                // Prime version-specific writer state before calibration;
                // otherwise a lazy first-use allocation is counted once in
                // the baseline but is absent from every injected retry.
                dxfRW codec(output.string().c_str());
                DimensionVersionExport adapter(codec);
                if (!codec.write(&adapter, target.first, binary)
                    || !adapter.written || !adapter.sourceUnchanged()
                    || !readbackMatches(output))
                    return fail("version/encoding warm-up failed");
            }
            if (!clearStagedOutput(output))
                return fail("could not clear warmed output");
            std::string expectedBytes;
            long allocationCount = 0;
            {
                dxfRW codec(output.string().c_str());
                DimensionVersionExport adapter(codec);
                armAllocationFailure(1000000000L);
                const bool wrote = codec.write(&adapter, target.first, binary);
                allocationCount = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
                if (!wrote || !adapter.written || !adapter.sourceUnchanged()
                    || !readbackMatches(output)
                    || !readBytes(output, expectedBytes)) {
                    std::cerr << target.second << " binary=" << binary
                              << " wrote=" << wrote
                              << " entities=" << adapter.written
                              << " unchanged=" << adapter.sourceUnchanged()
                              << " readback=" << readbackMatches(output)
                              << " error=" << codec.getError() << '\n';
                    return fail("warm-up or typed readback failed");
                }
            }
            if (allocationCount <= 0)
                return fail("calibration observed no allocations");

            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior)
                        return fail("could not seed destination sentinel");
                }
                dxfRW codec(output.string().c_str());
                DimensionVersionExport adapter(codec);
                armAllocationFailure(offset);
                const bool wrote = codec.write(&adapter, target.first, binary);
                const std::size_t reached = allocationAttempts;
                const bool injected = allocationFailed;
                disarmAllocationFailure();
                std::string afterFailure;
                const bool destinationReadable = readBytes(output, afterFailure);
                const bool sourceUnchanged = adapter.sourceUnchanged();
                const bool stagedOutput = hasStagedOutput(output);
                if (!injected || wrote
                    || reached != static_cast<std::size_t>(offset + 1)
                    || !destinationReadable || afterFailure != sentinel
                    || !sourceUnchanged || stagedOutput) {
                    std::cerr << target.second << " binary=" << binary
                              << " offset=" << offset
                              << " injected=" << injected
                              << " wrote=" << wrote
                              << " reached=" << reached
                              << " destinationReadable=" << destinationReadable
                              << " destinationMatchesSentinel="
                              << (afterFailure == sentinel)
                              << " sourceUnchanged=" << sourceUnchanged
                              << " stagedOutput=" << stagedOutput << '\n';
                    return fail("injected failure changed caller or destination");
                }

                const bool retried = codec.write(&adapter, target.first, binary);
                std::string retryBytes;
                if (!retried || !adapter.written || !adapter.sourceUnchanged()
                    || !readBytes(output, retryBytes)
                    || retryBytes != expectedBytes || !readbackMatches(output)
                    || hasStagedOutput(output))
                    return fail("same-codec retry or typed readback failed");
            }
            std::cout << target.second << " DIMENSION subtypes/constraints "
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", caller/destination rollback, retry/readback=PASS\n";
            std::error_code removeError;
            std::filesystem::remove(output, removeError);
        }
    }
    return true;
}

bool runLongMTextSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "long UTF-8 MTEXT allocation transaction: "
                  << message << '\n';
        return false;
    };
    std::error_code directoryError;
    if (!std::filesystem::create_directories(directory, directoryError)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const std::string sentinel = "prior long MTEXT drawing bytes\n";
    const std::array<std::pair<DRW::Version, const char*>, 7> versions{{
        {DRW::AC1014, "AC1014"}, {DRW::AC1015, "AC1015"},
        {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
        {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
        {DRW::AC1032, "AC1032"}}};
    const auto sourceUnchanged = [](const LongMTextExport& adapter) {
        return adapter.mtext.handle == 0xDA10u
            && adapter.mtext.text == longMTextPayload()
            && adapter.mtext.basePoint.x == 12.0
            && adapter.mtext.basePoint.y == -4.0
            && adapter.mtext.basePoint.z == 3.0
            && adapter.mtext.height == 2.25
            && adapter.mtext.style == "STANDARD";
    };
    const auto readbackMatches = [](const std::filesystem::path& path) {
        dxfRW reader(path.string().c_str());
        Readback adapter;
        return reader.read(&adapter, false) && adapter.mtextCount == 1u
            && adapter.mtext.handle != 0u
            && adapter.mtext.text == longMTextPayload()
            && adapter.mtext.basePoint.x == 12.0
            && adapter.mtext.basePoint.y == -4.0
            && adapter.mtext.basePoint.z == 3.0
            && adapter.mtext.height == 2.25
            && adapter.mtext.style == "STANDARD";
    };

    for (const auto& versionProfile : versions) {
      const DRW::Version version = versionProfile.first;
      const std::string versionName = versionProfile.second;
      const auto output = directory / ("long-utf8-mtext-" + versionName + ".dxf");
      for (const bool binary : {false, true}) {
        if (!clearStagedOutput(output))
            return fail("could not clear output before warm-up");
        {
            dxfRW codec(output.string().c_str());
            LongMTextExport adapter(codec);
            if (!codec.write(&adapter, version, binary)
                || !adapter.entityWritten || !sourceUnchanged(adapter)
                || !readbackMatches(output))
                return fail((versionName + " warm-up or typed readback failed").c_str());
        }
        std::error_code removeError;
        std::filesystem::remove(output, removeError);

        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW codec(output.string().c_str());
            LongMTextExport adapter(codec);
            armAllocationFailure(1000000000L);
            const bool wrote = codec.write(&adapter, version, binary);
            allocationCount = static_cast<long>(allocationAttempts);
            disarmAllocationFailure();
            if (!wrote || !adapter.entityWritten || !sourceUnchanged(adapter)
                || !readbackMatches(output) || !readBytes(output, expectedBytes))
                return fail((versionName + " allocation calibration or readback failed").c_str());
        }
        if (allocationCount <= 0)
            return fail("calibration observed no allocations");

        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream prior(output,
                                   std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior)
                    return fail("could not seed destination sentinel");
            }
            dxfRW codec(output.string().c_str());
            LongMTextExport adapter(codec);
            armAllocationFailure(offset);
            const bool wrote = codec.write(&adapter, version, binary);
            const std::size_t reached = allocationAttempts;
            const bool injected = allocationFailed;
            disarmAllocationFailure();
            std::string afterFailure;
            if (!injected || wrote
                || reached != static_cast<std::size_t>(offset + 1)
                || !readBytes(output, afterFailure) || afterFailure != sentinel
                || !sourceUnchanged(adapter) || hasStagedOutput(output))
                return fail((versionName + " injected failure changed caller or destination state").c_str());

            const bool retried = codec.write(&adapter, version, binary);
            std::string retryBytes;
            if (!retried || !adapter.entityWritten || !sourceUnchanged(adapter)
                || !readBytes(output, retryBytes) || retryBytes != expectedBytes
                || !readbackMatches(output) || hasStagedOutput(output))
                return fail((versionName + " same-codec retry did not reproduce typed output").c_str());
        }
        std::cout << versionName << " long UTF-8 MTEXT "
                  << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", caller/destination rollback, retry/readback=PASS\n";
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
      }
    }
    return true;
}

bool runMLeaderVersionSweep(const std::filesystem::path& directory) {
    const auto fail = [](DRW::Version version, bool binary,
                         const char* message) {
        std::cerr << "MULTILEADER " << static_cast<int>(version)
                  << (binary ? " Binary: " : " ASCII: ") << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return false;

    const auto makeLeader = [] {
        DRW_MLeader value;
        value.handle = 0xEA10u;
        value.styleContentType = 2u;
        value.landingDistance = 2.5;
        value.defaultArrowHeadSize = 1.25;
        value.context.overallScale = 1.5;
        value.context.hasTextContents = true;
        value.context.textLabel = "version matrix MULTILEADER text";
        value.context.contentBasePoint = DRW_Coord{2.0, 3.0, 4.0};
        value.context.textLocation = DRW_Coord{5.0, 6.0, 7.0};
        value.context.textHeight = 2.0;
        DRW_MLeaderRoot root;
        root.connectionPoint = DRW_Coord{1.0, 2.0, 3.0};
        root.direction = DRW_Coord{0.0, 1.0, 0.0};
        root.leaderIndex = 2;
        root.landingDistance = 3.5;
        DRW_MLeaderLeaderLine line;
        line.points = {DRW_Coord{1.0, 2.0, 3.0},
                       DRW_Coord{4.0, 5.0, 6.0}};
        line.segmentIndex = 1;
        line.leaderLineIndex = 3;
        line.leaderType = 1;
        root.leaderLines.push_back(std::move(line));
        value.context.roots.push_back(std::move(root));
        return value;
    };
    const auto sameLeader = [](const DRW_MLeader& actual,
                               const DRW_MLeader& expected,
                               bool compareHandle) {
        if ((compareHandle && actual.handle != expected.handle)
            || actual.styleContentType != expected.styleContentType
            || actual.landingDistance != expected.landingDistance
            || actual.defaultArrowHeadSize != expected.defaultArrowHeadSize
            || actual.context.overallScale != expected.context.overallScale
            || actual.context.hasTextContents
                != expected.context.hasTextContents
            || actual.context.textLabel != expected.context.textLabel
            || actual.context.contentBasePoint.x
                != expected.context.contentBasePoint.x
            || actual.context.contentBasePoint.y
                != expected.context.contentBasePoint.y
            || actual.context.contentBasePoint.z
                != expected.context.contentBasePoint.z
            || actual.context.textLocation.x != expected.context.textLocation.x
            || actual.context.textLocation.y != expected.context.textLocation.y
            || actual.context.textLocation.z != expected.context.textLocation.z
            || actual.context.textHeight != expected.context.textHeight
            || actual.context.roots.size() != expected.context.roots.size())
            return false;
        for (std::size_t i = 0; i < actual.context.roots.size(); ++i) {
            const auto& a = actual.context.roots[i];
            const auto& e = expected.context.roots[i];
            if (a.isContentValid != e.isContentValid
                || a.unknown291 != e.unknown291
                || a.connectionPoint.x != e.connectionPoint.x
                || a.connectionPoint.y != e.connectionPoint.y
                || a.connectionPoint.z != e.connectionPoint.z
                || a.direction.x != e.direction.x
                || a.direction.y != e.direction.y
                || a.direction.z != e.direction.z
                || a.leaderIndex != e.leaderIndex
                || a.landingDistance != e.landingDistance
                || a.leaderLines.size() != e.leaderLines.size())
                return false;
            for (std::size_t j = 0; j < a.leaderLines.size(); ++j) {
                const auto& al = a.leaderLines[j];
                const auto& el = e.leaderLines[j];
                if (al.points.size() != el.points.size()
                    || al.segmentIndex != el.segmentIndex
                    || al.leaderLineIndex != el.leaderLineIndex
                    || al.leaderType != el.leaderType)
                    return false;
                for (std::size_t k = 0; k < al.points.size(); ++k) {
                    if (al.points[k].x != el.points[k].x
                        || al.points[k].y != el.points[k].y
                        || al.points[k].z != el.points[k].z)
                        return false;
                }
            }
        }
        return true;
    };
    class MLeaderExport final : public dx_iface {
    public:
        MLeaderExport(dxfRW& codec, DRW_MLeader value)
            : output(codec), leader(std::move(value)) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
        }
        void writeEntities() override { emitted = output.writeMultiLeader(&leader); }
        bool finalizeDxfWrite() override { return emitted; }
        dxfRW& output;
        dx_data storage;
        DRW_MLeader leader;
        bool emitted = false;
    };
    class MLeaderReadback final : public dx_iface {
    public:
        MLeaderReadback() {
            cData = &storage;
            currentBlock = storage.mBlock;
        }
        void addMLeader(const DRW_MLeader* value) override {
            if (value != nullptr) {
                leader = *value;
                seen = true;
            }
        }
        dx_data storage;
        DRW_MLeader leader;
        bool seen = false;
    };
    const auto readbackMatches = [&](const std::filesystem::path& path,
                                     DRW::Version version) {
        dxfRW reader(path.string().c_str());
        MLeaderReadback probe;
        return reader.read(&probe, true)
            && reader.getSourceVersion() == version && probe.seen
            && sameLeader(probe.leader, makeLeader(), false);
    };
    const std::array<std::pair<DRW::Version, const char*>, 4> versions = {{
        {DRW::AC1021, "AC1021"}, {DRW::AC1024, "AC1024"},
        {DRW::AC1027, "AC1027"}, {DRW::AC1032, "AC1032"}}};
    const auto output = directory / "mleader-version-transaction.dxf";
    const std::string sentinel = "prior MULTILEADER destination\n";
    for (const auto& version : versions) {
        for (const bool binary : {false, true}) {
            if (!clearStagedOutput(output))
                return fail(version.first, binary,
                            "could not clear stale staging files");
            long allocationCount = 0;
            std::string expectedBytes;
            {
                std::ofstream prior(output,
                    std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior.good())
                    return fail(version.first, binary,
                                "could not seed calibration destination");
            }
            {
                dxfRW codec(output.string().c_str());
                MLeaderExport adapter(codec, makeLeader());
                armAllocationFailure(1000000000L);
                const bool written = codec.write(&adapter, version.first,
                                                  binary);
                allocationCount = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
                const bool bytesRead = readBytes(output, expectedBytes);
                const bool readback = bytesRead
                    && readbackMatches(output, version.first);
                if (!written || !adapter.emitted || allocationCount <= 0
                    || !bytesRead || !readback) {
                    std::cerr << "  write=" << written
                              << " callback=" << adapter.emitted
                              << " allocations=" << allocationCount
                              << " bytes=" << bytesRead
                              << " readback=" << readback
                              << " error=" << codec.getError() << '\n';
                    return fail(version.first, binary,
                                "clean write or typed readback failed");
                }
            }
            long failureCount = 0;
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior.good())
                        return fail(version.first, binary,
                                    "could not seed destination");
                }
                dxfRW codec(output.string().c_str());
                MLeaderExport adapter(codec, makeLeader());
                const DRW_MLeader original = adapter.leader;
                armAllocationFailure(offset);
                const bool written = codec.write(&adapter, version.first,
                                                  binary);
                const long attempts = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
                std::string afterFailure;
                const bool callerPreserved =
                    sameLeader(adapter.leader, original, true);
                const bool destinationPreserved = readBytes(output, afterFailure)
                    && afterFailure == sentinel;
                const bool stagingPreserved = hasStagedOutput(output);
                if (!allocationFailed) {
                    std::string boundaryBytes;
                    if (!written || !adapter.emitted || !callerPreserved
                        || !readBytes(output, boundaryBytes)
                        || boundaryBytes != expectedBytes
                        || !readbackMatches(output, version.first)
                        || stagingPreserved)
                        return fail(version.first, binary,
                                    "unfaulted allocation boundary changed output");
                    break;
                }
                if (!allocationFailed || written
                    || attempts != offset + 1
                    || !callerPreserved || !destinationPreserved
                    || stagingPreserved) {
                    std::cerr << "  offset=" << offset
                              << " failed=" << allocationFailed
                              << " write=" << written
                              << " callback=" << adapter.emitted
                              << " attempts=" << attempts
                              << " caller=" << callerPreserved
                              << " destination=" << destinationPreserved
                              << " staged=" << stagingPreserved << '\n';
                    return fail(version.first, binary,
                                "injected failure did not roll back");
                }
                ++failureCount;
                const bool retried = codec.write(&adapter, version.first,
                                                  binary);
                std::string retryBytes;
                if (!retried || !adapter.emitted
                    || !sameLeader(adapter.leader, original, true)
                    || !readBytes(output, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(output, version.first)
                    || hasStagedOutput(output))
                    return fail(version.first, binary,
                                "same-codec retry/readback failed");
            }
            std::cout << "MULTILEADER " << version.second << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": injected allocation offsets=" << failureCount
                      << ", rollback/retry/readback=PASS\n";
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
    }
    for (const bool binary : {false, true}) {
        {
            std::ofstream prior(output,
                std::ios::binary | std::ios::trunc);
            prior << sentinel;
            if (!prior.good())
                return fail(DRW::AC1018, binary,
                            "could not seed minimum-version destination");
        }
        dxfRW codec(output.string().c_str());
        MLeaderExport adapter(codec, makeLeader());
        const DRW_MLeader original = adapter.leader;
        if (codec.write(&adapter, DRW::AC1018, binary) || adapter.emitted
            || !sameLeader(adapter.leader, original, true))
            return fail(DRW::AC1018, binary,
                        "pre-R2010 target was not atomically refused");
        std::string afterRefusal;
        if (!readBytes(output, afterRefusal) || afterRefusal != sentinel)
            return fail(DRW::AC1018, binary,
                        "minimum-version refusal replaced destination");
        if (!codec.write(&adapter, DRW::AC1021, binary) || !adapter.emitted
            || !readbackMatches(output, DRW::AC1021))
            return fail(DRW::AC1021, binary,
                        "same-codec retry at minimum supported version failed");
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
    }
    return true;
}

bool runLeaderVersionSweep(const std::filesystem::path& directory) {
    const auto fail = [](DRW::Version version, bool binary,
                         const char* message) {
        std::cerr << "LEADER " << static_cast<int>(version)
                  << (binary ? " Binary: " : " ASCII: ") << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return false;
    const auto makeLeader = [] {
        DRW_Leader value;
        value.handle = 0xEA20u;
        value.style = "STANDARD";
        value.arrow = 0;
        value.leadertype = 1;
        value.flag = 2;
        value.textheight = 2.25;
        value.textwidth = 4.5;
        value.coloruse = 5;
        value.offsetblock = DRW_Coord{0.25, 0.5, 0.75};
        value.extrusionPoint = DRW_Coord{0.0, 0.0, 1.0};
        value.vertnum = 3;
        value.vertexlist = {
            std::make_shared<DRW_Coord>(DRW_Coord{1.0, 2.0, 3.0}),
            std::make_shared<DRW_Coord>(DRW_Coord{4.0, 5.0, 6.0}),
            std::make_shared<DRW_Coord>(DRW_Coord{7.0, 8.0, 9.0})};
        return value;
    };
    const auto sameLeader = [](const DRW_Leader& actual,
                               const DRW_Leader& expected,
                               bool compareHandle) {
        if ((compareHandle && actual.handle != expected.handle)
            || actual.style != expected.style || actual.arrow != expected.arrow
            || actual.leadertype != expected.leadertype
            || actual.flag != expected.flag || actual.hookline != expected.hookline
            || actual.hookflag != expected.hookflag
            || actual.textheight != expected.textheight
            || actual.textwidth != expected.textwidth
            || actual.vertnum != expected.vertnum
            || actual.coloruse != expected.coloruse
            || actual.annotHandle != expected.annotHandle
            || actual.offsetblock.x != expected.offsetblock.x
            || actual.offsetblock.y != expected.offsetblock.y
            || actual.offsetblock.z != expected.offsetblock.z
            || actual.offsettext.x != expected.offsettext.x
            || actual.offsettext.y != expected.offsettext.y
            || actual.offsettext.z != expected.offsettext.z
            || actual.extrusionPoint.x != expected.extrusionPoint.x
            || actual.extrusionPoint.y != expected.extrusionPoint.y
            || actual.extrusionPoint.z != expected.extrusionPoint.z
            || actual.horizdir.x != expected.horizdir.x
            || actual.horizdir.y != expected.horizdir.y
            || actual.horizdir.z != expected.horizdir.z
            || actual.vertexlist.size() != expected.vertexlist.size())
            return false;
        for (std::size_t i = 0; i < actual.vertexlist.size(); ++i) {
            if (!actual.vertexlist[i] || !expected.vertexlist[i]
                || actual.vertexlist[i]->x != expected.vertexlist[i]->x
                || actual.vertexlist[i]->y != expected.vertexlist[i]->y
                || actual.vertexlist[i]->z != expected.vertexlist[i]->z)
                return false;
        }
        return true;
    };
    class LeaderExport final : public dx_iface {
    public:
        LeaderExport(dxfRW& codec, DRW_Leader value)
            : output(codec), leader(std::move(value)) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
            DRW_Dimstyle standard;
            standard.handle = 0xEA21u;
            standard.name = "STANDARD";
            storage.dimStyles.push_back(std::move(standard));
        }
        void writeEntities() override { emitted = output.writeLeader(&leader); }
        bool finalizeDxfWrite() override { return emitted; }
        dxfRW& output;
        dx_data storage;
        DRW_Leader leader;
        bool emitted = false;
    };
    class LeaderReadback final : public dx_iface {
    public:
        LeaderReadback() {
            cData = &storage;
            currentBlock = storage.mBlock;
        }
        void addLeader(const DRW_Leader* value) override {
            if (value != nullptr) {
                leader = *value;
                seen = true;
            }
        }
        dx_data storage;
        DRW_Leader leader;
        bool seen = false;
    };
    const auto readbackMatches = [&](const std::filesystem::path& path,
                                     DRW::Version version) {
        dxfRW reader(path.string().c_str());
        LeaderReadback probe;
        return reader.read(&probe, true)
            && reader.getSourceVersion() == version && probe.seen
            && sameLeader(probe.leader, makeLeader(), false);
    };
    const std::array<std::pair<DRW::Version, const char*>, 7> versions = {{
        {DRW::AC1014, "AC1014"}, {DRW::AC1015, "AC1015"},
        {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
        {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
        {DRW::AC1032, "AC1032"}}};
    const auto output = directory / "leader-version-transaction.dxf";
    const std::string sentinel = "prior LEADER destination\n";
    for (const auto& version : versions) {
        for (const bool binary : {false, true}) {
            if (!clearStagedOutput(output))
                return fail(version.first, binary,
                            "could not clear stale staging files");
            long allocationCount = 0;
            long failureCount = 0;
            std::string expectedBytes;
            {
                std::ofstream prior(output,
                    std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior.good())
                    return fail(version.first, binary,
                                "could not seed calibration destination");
            }
            {
                dxfRW codec(output.string().c_str());
                LeaderExport adapter(codec, makeLeader());
                armAllocationFailure(1000000000L);
                const bool written = codec.write(&adapter, version.first,
                                                  binary);
                allocationCount = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
                if (!written || !adapter.emitted || allocationCount <= 0
                    || !readBytes(output, expectedBytes)
                    || !readbackMatches(output, version.first))
                    return fail(version.first, binary,
                                "clean write or typed readback failed");
            }
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior.good())
                        return fail(version.first, binary,
                                    "could not seed destination");
                }
                dxfRW codec(output.string().c_str());
                LeaderExport adapter(codec, makeLeader());
                const DRW_Leader original = adapter.leader;
                armAllocationFailure(offset);
                const bool written = codec.write(&adapter, version.first,
                                                  binary);
                const long attempts = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
                if (!allocationFailed) {
                    std::string boundaryBytes;
                    if (!written || !adapter.emitted
                        || !sameLeader(adapter.leader, original, true)
                        || !readBytes(output, boundaryBytes)
                        || boundaryBytes != expectedBytes
                        || !readbackMatches(output, version.first)
                        || hasStagedOutput(output))
                        return fail(version.first, binary,
                                    "unfaulted allocation boundary changed output");
                    break;
                }
                std::string afterFailure;
                if (written || attempts != offset + 1
                    || !sameLeader(adapter.leader, original, true)
                    || !readBytes(output, afterFailure)
                    || afterFailure != sentinel || hasStagedOutput(output))
                    return fail(version.first, binary,
                                "injected failure did not roll back");
                ++failureCount;
                if (!codec.write(&adapter, version.first, binary)
                    || !adapter.emitted
                    || !sameLeader(adapter.leader, original, true))
                    return fail(version.first, binary,
                                "same-codec retry failed");
                std::string retryBytes;
                if (!readBytes(output, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(output, version.first)
                    || hasStagedOutput(output))
                    return fail(version.first, binary,
                                "retry bytes/readback failed");
            }
            std::cout << "LEADER " << version.second << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": injected allocation offsets=" << failureCount
                      << ", rollback/retry/readback=PASS\n";
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
    }
    for (const bool binary : {false, true}) {
        {
            std::ofstream prior(output,
                std::ios::binary | std::ios::trunc);
            prior << sentinel;
            if (!prior.good())
                return fail(DRW::AC1009, binary,
                            "could not seed unsupported-version destination");
        }
        dxfRW codec(output.string().c_str());
        LeaderExport adapter(codec, makeLeader());
        const DRW_Leader original = adapter.leader;
        if (codec.write(&adapter, DRW::AC1009, binary) || adapter.emitted
            || !sameLeader(adapter.leader, original, true))
            return fail(DRW::AC1009, binary,
                        "R12 target was not atomically refused");
        std::string afterRefusal;
        if (!readBytes(output, afterRefusal) || afterRefusal != sentinel)
            return fail(DRW::AC1009, binary,
                        "unsupported-version refusal replaced destination");
        if (!codec.write(&adapter, DRW::AC1014, binary) || !adapter.emitted
            || !readbackMatches(output, DRW::AC1014))
            return fail(DRW::AC1014, binary,
                        "same-codec retry at minimum supported version failed");
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
    }
    return true;
}

bool runGeoPositionMarkerVersionSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "GEOPOSITIONMARKER version transaction: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const auto makeMarker = [] {
        DRW_GeoPositionMarker value;
        value.handle = 0xDA40u;
        value.m_classVersion = 1u;
        value.m_position = DRW_Coord(123.0, 124.0, 125.0);
        value.m_radius = 8.5;
        value.m_notes = "version matrix embedded frame marker";
        value.m_landingGap = 1.25;
        value.m_mtextVisible = true;
        value.m_textAlignment = 1u;
        value.m_enableFrameText = true;
        value.mtext = std::make_unique<DRW_MText>();
        value.mtext->text = "version matrix embedded frame";
        value.mtext->basePoint = value.m_position;
        value.mtext->height = 1.75;
        value.mtext->widthscale = 5.0;
        value.mtext->style = "STANDARD";
        return value;
    };
    const auto sameMarker = [](const DRW_GeoPositionMarker& actual,
                               const DRW_GeoPositionMarker& expected) {
        return actual.m_classVersion == expected.m_classVersion
            && actual.m_position.x == expected.m_position.x
            && actual.m_position.y == expected.m_position.y
            && actual.m_position.z == expected.m_position.z
            && actual.m_radius == expected.m_radius
            && actual.m_notes == expected.m_notes
            && actual.m_landingGap == expected.m_landingGap
            && actual.m_mtextVisible == expected.m_mtextVisible
            && actual.m_textAlignment == expected.m_textAlignment
            && actual.m_enableFrameText == expected.m_enableFrameText
            && actual.mtext && expected.mtext
            && actual.mtext->text == expected.mtext->text
            && actual.mtext->basePoint.x == expected.mtext->basePoint.x
            && actual.mtext->basePoint.y == expected.mtext->basePoint.y
            && actual.mtext->basePoint.z == expected.mtext->basePoint.z
            && actual.mtext->height == expected.mtext->height
            && actual.mtext->widthscale == expected.mtext->widthscale
            && actual.mtext->style == expected.mtext->style;
    };
    class MarkerExport final : public dx_iface {
    public:
        MarkerExport(dxfRW& codec, DRW_GeoPositionMarker source)
            : output(codec), marker(std::move(source)) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
        }
        void writeEntities() override {
            written = output.writeGeoPositionMarker(&marker);
        }
        bool finalizeDxfWrite() override { return written; }
        dxfRW& output;
        dx_data storage;
        DRW_GeoPositionMarker marker;
        bool written = false;
    };
    class MarkerReadback final : public dx_iface {
    public:
        MarkerReadback() {
            cData = &storage;
            currentBlock = storage.mBlock;
        }
        void addGeoPositionMarker(const DRW_GeoPositionMarker& value) override {
            marker = value;
            ++count;
        }
        dx_data storage;
        DRW_GeoPositionMarker marker;
        unsigned count = 0u;
    };
    const auto readbackMatches = [&](const std::filesystem::path& path,
                                     DRW::Version expectedVersion) {
        dxfRW reader(path.string().c_str());
        MarkerReadback probe;
        DRW_GeoPositionMarker expected = makeMarker();
        const bool read = reader.read(&probe, true);
        if (probe.count == 1u)
            expected.handle = probe.marker.handle;
        return read && reader.getSourceVersion() == expectedVersion
            && probe.count == 1u && probe.marker.handle != 0u
            && sameMarker(probe.marker, expected);
    };
    const std::array<std::pair<DRW::Version, const char*>, 2> versions = {{
        {DRW::AC1027, "AC1027"}, {DRW::AC1032, "AC1032"}}};
    const std::string sentinel = "prior geo marker destination\n";
    for (const bool binary : {false, true}) {
        const auto output = directory
            / (binary ? "geo-marker-refusal-binary.dxf"
                      : "geo-marker-refusal-ascii.dxf");
        const auto expectedPath = directory
            / (binary ? "geo-marker-retry-expected-binary.dxf"
                      : "geo-marker-retry-expected-ascii.dxf");
        if (!clearStagedOutput(output) || !clearStagedOutput(expectedPath))
            return fail("could not clear refusal/retry staging files");
        {
            dxfRW codec(expectedPath.string().c_str());
            MarkerExport adapter(codec, makeMarker());
            if (!codec.write(&adapter, DRW::AC1027, binary)
                || !adapter.written)
                return fail("could not create refusal/retry reference output");
        }
        std::string expectedBytes;
        if (!readBytes(expectedPath, expectedBytes))
            return fail("could not read refusal/retry reference output");
        {
            std::ofstream prior(output, std::ios::binary | std::ios::trunc);
            prior << sentinel;
            if (!prior.good())
                return fail("could not seed refused destination");
        }
        dxfRW codec(output.string().c_str());
        MarkerExport adapter(codec, makeMarker());
        const DRW_GeoPositionMarker original(adapter.marker);
        if (codec.write(&adapter, DRW::AC1024, binary)
            || adapter.written
            || !sameMarker(adapter.marker, original)
            || adapter.marker.handle != original.handle)
            return fail("unsupported AC1024 route was not refused atomically");
        std::string afterRefusal;
        if (!readBytes(output, afterRefusal) || afterRefusal != sentinel
            || hasStagedOutput(output))
            return fail("AC1024 refusal changed destination state");
        if (!codec.write(&adapter, DRW::AC1027, binary) || !adapter.written
            || !sameMarker(adapter.marker, original)
            || adapter.marker.handle != original.handle)
            return fail("same-writer AC1027 retry failed");
        std::string retryBytes;
        if (!readBytes(output, retryBytes) || retryBytes != expectedBytes
            || !readbackMatches(output, DRW::AC1027)
            || hasStagedOutput(output))
            return fail("AC1027 retry differs from clean output");
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
        std::filesystem::remove(expectedPath, ignored);
    }
    for (const auto& version : versions) {
        for (const bool binary : {false, true}) {
            const auto output = directory
                / (std::string("geo-marker-") + version.second
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            if (!clearStagedOutput(output))
                return fail("could not clear stale output staging files");
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                MarkerExport adapter(codec, makeMarker());
                if (!codec.write(&adapter, version.first, binary)
                    || !adapter.written)
                    return fail("calibration write failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            {
                dxfRW codec(output.string().c_str());
                MarkerExport adapter(codec, makeMarker());
                armAllocationFailure(1000000000L);
                const bool wrote = codec.write(&adapter, version.first, binary);
                allocationCount = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
                if (!wrote || !adapter.written || allocationFailed
                    || allocationCount <= 0 || !readBytes(output, expectedBytes)
                    || !readbackMatches(output, version.first))
                    return fail("calibration write/readback failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior.good())
                        return fail("could not seed prior destination");
                }
                dxfRW codec(output.string().c_str());
                MarkerExport adapter(codec, makeMarker());
                const DRW_GeoPositionMarker original(adapter.marker);
                armAllocationFailure(offset);
                const bool wrote = codec.write(&adapter, version.first, binary);
                const bool injected = allocationFailed;
                disarmAllocationFailure();
                std::string afterFailure;
                if (wrote || !injected || !readBytes(output, afterFailure)
                    || afterFailure != sentinel
                    || !sameMarker(adapter.marker, original)
                    || adapter.marker.handle != original.handle
                    || hasStagedOutput(output))
                    return fail("injected failure changed caller or destination");
                const bool retried = codec.write(&adapter, version.first, binary)
                    && adapter.written;
                std::string retryBytes;
                if (!retried || !sameMarker(adapter.marker, original)
                    || adapter.marker.handle != original.handle
                    || !readBytes(output, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(output, version.first)
                    || hasStagedOutput(output))
                    return fail("same-writer retry or typed readback failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            std::cout << "GEOPOSITIONMARKER " << version.second << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
        }
    }
    return true;
}

bool runMeshVersionSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "MESH version allocation transaction: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const auto makeMesh = [] {
        DRW_Mesh value;
        value.handle = 0xDA50u;
        value.version = 2u;
        value.blendCrease = true;
        value.subdivisionLevel = 3;
        value.vertices = {DRW_Coord(0.0, 0.0, 0.0),
                          DRW_Coord(2.0, 0.0, 0.0),
                          DRW_Coord(0.0, 3.0, 0.0),
                          DRW_Coord(0.0, 0.0, 4.0)};
        value.faces = {{0, 1, 2}, {0, 2, 3}};
        value.edges = {{0, 1}, {1, 2}, {2, 0}, {2, 3}};
        value.creases = {0.5, 1.25};
        DRW_MeshPropertyOverride overrideData;
        overrideData.subEntityMarker = 7;
        overrideData.propertyTypes = {0, 2};
        value.propertyOverrides.push_back(overrideData);
        return value;
    };
    const auto sameMesh = [](const DRW_Mesh& actual, const DRW_Mesh& expected) {
        if (actual.version != expected.version
            || actual.blendCrease != expected.blendCrease
            || actual.subdivisionLevel != expected.subdivisionLevel
            || actual.vertices.size() != expected.vertices.size()
            || actual.faces != expected.faces || actual.edges != expected.edges
            || actual.creases != expected.creases
            || actual.propertyOverrides.size() != expected.propertyOverrides.size())
            return false;
        for (std::size_t i = 0u; i < actual.vertices.size(); ++i) {
            if (actual.vertices[i].x != expected.vertices[i].x
                || actual.vertices[i].y != expected.vertices[i].y
                || actual.vertices[i].z != expected.vertices[i].z)
                return false;
        }
        for (std::size_t i = 0u; i < actual.propertyOverrides.size(); ++i) {
            if (actual.propertyOverrides[i].subEntityMarker
                    != expected.propertyOverrides[i].subEntityMarker
                || actual.propertyOverrides[i].propertyTypes
                    != expected.propertyOverrides[i].propertyTypes)
                return false;
        }
        return true;
    };
    class MeshExport final : public dx_iface {
    public:
        MeshExport(dxfRW& codec, DRW_Mesh source) : output(codec), mesh(std::move(source)) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
        }
        void writeEntities() override { written = output.writeMesh(&mesh); }
        bool finalizeDxfWrite() override { return written; }
        dxfRW& output;
        dx_data storage;
        DRW_Mesh mesh;
        bool written = false;
    };
    class MeshReadback final : public dx_iface {
    public:
        MeshReadback() {
            cData = &storage;
            currentBlock = storage.mBlock;
        }
        void addMesh(const DRW_Mesh& value) override {
            mesh = value;
            ++count;
        }
        dx_data storage;
        DRW_Mesh mesh;
        unsigned count = 0u;
    };
    const auto readbackMatches = [&](const std::filesystem::path& path,
                                     DRW::Version expectedVersion) {
        dxfRW reader(path.string().c_str());
        MeshReadback probe;
        const bool read = reader.read(&probe, true);
        DRW_Mesh expected = makeMesh();
        if (probe.count == 1u)
            expected.handle = probe.mesh.handle;
        return read && reader.getSourceVersion() == expectedVersion
            && probe.count == 1u && probe.mesh.handle != 0u
            && sameMesh(probe.mesh, expected);
    };
    const std::array<std::pair<DRW::Version, const char*>, 7> versions = {{
        {DRW::AC1014, "AC1014"}, {DRW::AC1015, "AC1015"},
        {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
        {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
        {DRW::AC1032, "AC1032"}}};
    const std::string sentinel = "prior mesh destination\n";
    for (const bool binary : {false, true}) {
        const auto output = directory
            / (binary ? "mesh-refusal-binary.dxf" : "mesh-refusal-ascii.dxf");
        const auto expectedPath = directory
            / (binary ? "mesh-retry-expected-binary.dxf"
                      : "mesh-retry-expected-ascii.dxf");
        if (!clearStagedOutput(output) || !clearStagedOutput(expectedPath))
            return fail("could not clear refusal/retry staging files");
        {
            dxfRW codec(expectedPath.string().c_str());
            MeshExport adapter(codec, makeMesh());
            if (!codec.write(&adapter, DRW::AC1014, binary) || !adapter.written)
                return fail("could not create refusal/retry reference output");
        }
        std::string expectedBytes;
        if (!readBytes(expectedPath, expectedBytes))
            return fail("could not read refusal/retry reference output");
        {
            std::ofstream prior(output, std::ios::binary | std::ios::trunc);
            prior << sentinel;
            if (!prior.good())
                return fail("could not seed refused destination");
        }
        dxfRW codec(output.string().c_str());
        MeshExport adapter(codec, makeMesh());
        const DRW_Mesh original(adapter.mesh);
        if (codec.write(&adapter, DRW::AC1009, binary) || adapter.written
            || !sameMesh(adapter.mesh, original)
            || adapter.mesh.handle != original.handle)
            return fail("unsupported AC1009 route was not refused atomically");
        std::string afterRefusal;
        if (!readBytes(output, afterRefusal) || afterRefusal != sentinel
            || hasStagedOutput(output))
            return fail("AC1009 refusal changed destination state");
        if (!codec.write(&adapter, DRW::AC1014, binary) || !adapter.written
            || !sameMesh(adapter.mesh, original)
            || adapter.mesh.handle != original.handle)
            return fail("same-writer AC1014 retry failed");
        std::string retryBytes;
        if (!readBytes(output, retryBytes) || retryBytes != expectedBytes
            || !readbackMatches(output, DRW::AC1014)
            || hasStagedOutput(output))
            return fail("AC1014 retry differs from clean output");
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
        std::filesystem::remove(expectedPath, ignored);
    }
    for (const auto& version : versions) {
        for (const bool binary : {false, true}) {
            const auto output = directory
                / (std::string("mesh-") + version.second
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            if (!clearStagedOutput(output))
                return fail("could not clear stale output staging files");
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                MeshExport adapter(codec, makeMesh());
                if (!codec.write(&adapter, version.first, binary)
                    || !adapter.written)
                    return fail("calibration write failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            {
                dxfRW codec(output.string().c_str());
                MeshExport adapter(codec, makeMesh());
                armAllocationFailure(1000000000L);
                const bool wrote = codec.write(&adapter, version.first, binary);
                allocationCount = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
                if (!wrote || !adapter.written || allocationFailed
                    || allocationCount <= 0 || !readBytes(output, expectedBytes)
                    || !readbackMatches(output, version.first))
                    return fail("calibration write/readback failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior.good())
                        return fail("could not seed prior destination");
                }
                dxfRW codec(output.string().c_str());
                MeshExport adapter(codec, makeMesh());
                const DRW_Mesh original(adapter.mesh);
                armAllocationFailure(offset);
                const bool wrote = codec.write(&adapter, version.first, binary);
                const bool injected = allocationFailed;
                disarmAllocationFailure();
                std::string afterFailure;
                if (wrote || !injected || !readBytes(output, afterFailure)
                    || afterFailure != sentinel || !sameMesh(adapter.mesh, original)
                    || adapter.mesh.handle != original.handle
                    || hasStagedOutput(output))
                    return fail("injected failure changed caller or destination");
                const bool retried = codec.write(&adapter, version.first, binary)
                    && adapter.written;
                std::string retryBytes;
                if (!retried || !sameMesh(adapter.mesh, original)
                    || adapter.mesh.handle != original.handle
                    || !readBytes(output, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(output, version.first)
                    || hasStagedOutput(output))
                    return fail("same-writer retry or typed readback failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            std::cout << "MESH " << version.second << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
        }
    }
    return true;
}

bool runViewportVersionSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "VIEWPORT version allocation transaction: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const auto makeViewport = [](DRW::Version targetVersion) {
        DRW_Viewport value;
        value.handle = 0xDA60u;
        value.space = DRW::PaperSpace;
        value.basePoint = DRW_Coord(5.0, 6.0, 7.0);
        value.pswidth = 210.0;
        value.psheight = 148.0;
        value.vpstatus = 1;
        value.vpID = 2;
        value.centerPX = 105.0;
        value.centerPY = 74.0;
        value.snapPX = 1.5;
        value.snapPY = 2.5;
        value.snapSpPX = 3.5;
        value.snapSpPY = 4.5;
        value.gridSpX = 5.5;
        value.gridSpY = 6.5;
        value.viewDir = DRW_Coord(1.0, 2.0, 3.0);
        value.viewTarget = DRW_Coord(10.0, 20.0, 30.0);
        value.viewLength = 55.0;
        value.frontClip = 0.25;
        value.backClip = 500.0;
        value.viewHeight = 90.0;
        value.snapAngle = 30.0;
        value.twistAngle = 45.0;
        value.circleZoom = 137.0;
        value.majorGridLines = targetVersion > DRW::AC1018 ? 5 : 0;
        value.statusFlags = 0xA51;
        value.styleSheet = "viewport-style-sheet";
        value.renderMode = 2;
        value.ucsPerViewport = true;
        value.ucsAtOrigin = true;
        value.ucsOrigin = DRW_Coord(7.0, 8.0, 9.0);
        value.ucsXAxis = DRW_Coord(0.0, 1.0, 0.0);
        value.ucsYAxis = DRW_Coord(-1.0, 0.0, 0.0);
        value.ucsElevation = 11.0;
        value.ucsOrthographicType = 4;
        value.shadePlotMode = targetVersion > DRW::AC1015 ? 3 : 0;
        if (targetVersion > DRW::AC1018) {
            value.useDefaultLighting = false;
            value.defaultLightingType = 0;
            value.brightness = 0.4;
            value.contrast = 0.6;
            value.ambientColor = 42u;
            value.ambientColorRgb = 0x123456;
            value.ambientColorName = "Named Ambient";
        }
        if (targetVersion <= DRW::AC1009) {
            const DRW_Viewport defaults;
            value.snapPX = defaults.snapPX;
            value.snapPY = defaults.snapPY;
            value.snapSpPX = defaults.snapSpPX;
            value.snapSpPY = defaults.snapSpPY;
            value.gridSpX = defaults.gridSpX;
            value.gridSpY = defaults.gridSpY;
            value.viewDir = defaults.viewDir;
            value.viewTarget = defaults.viewTarget;
            value.viewLength = defaults.viewLength;
            value.frontClip = defaults.frontClip;
            value.backClip = defaults.backClip;
            value.viewHeight = defaults.viewHeight;
            value.snapAngle = defaults.snapAngle;
            value.twistAngle = defaults.twistAngle;
            value.circleZoom = defaults.circleZoom;
            value.majorGridLines = defaults.majorGridLines;
            value.statusFlags = defaults.statusFlags;
            value.styleSheet = defaults.styleSheet;
            value.renderMode = defaults.renderMode;
            value.ucsPerViewport = defaults.ucsPerViewport;
            value.ucsAtOrigin = defaults.ucsAtOrigin;
            value.ucsOrigin = defaults.ucsOrigin;
            value.ucsXAxis = defaults.ucsXAxis;
            value.ucsYAxis = defaults.ucsYAxis;
            value.ucsElevation = defaults.ucsElevation;
            value.ucsOrthographicType = defaults.ucsOrthographicType;
            value.shadePlotMode = defaults.shadePlotMode;
            value.useDefaultLighting = defaults.useDefaultLighting;
            value.defaultLightingType = defaults.defaultLightingType;
            value.brightness = defaults.brightness;
            value.contrast = defaults.contrast;
            value.ambientColor = defaults.ambientColor;
            value.ambientColorRgb = defaults.ambientColorRgb;
            value.ambientColorName = defaults.ambientColorName;
        }
        return value;
    };
    const auto sameViewport = [](const DRW_Viewport& a,
                                 const DRW_Viewport& b) {
        return a.basePoint.x == b.basePoint.x
            && a.basePoint.y == b.basePoint.y
            && a.basePoint.z == b.basePoint.z
            && a.pswidth == b.pswidth && a.psheight == b.psheight
            && a.vpstatus == b.vpstatus && a.vpID == b.vpID
            && a.centerPX == b.centerPX && a.centerPY == b.centerPY
            && a.snapPX == b.snapPX && a.snapPY == b.snapPY
            && a.snapSpPX == b.snapSpPX && a.snapSpPY == b.snapSpPY
            && a.gridSpX == b.gridSpX && a.gridSpY == b.gridSpY
            && a.viewDir.x == b.viewDir.x && a.viewDir.y == b.viewDir.y
            && a.viewDir.z == b.viewDir.z
            && a.viewTarget.x == b.viewTarget.x
            && a.viewTarget.y == b.viewTarget.y
            && a.viewTarget.z == b.viewTarget.z
            && a.viewLength == b.viewLength && a.frontClip == b.frontClip
            && a.backClip == b.backClip && a.viewHeight == b.viewHeight
            && a.snapAngle == b.snapAngle && a.twistAngle == b.twistAngle
            && a.circleZoom == b.circleZoom
            && a.majorGridLines == b.majorGridLines
            && a.statusFlags == b.statusFlags && a.styleSheet == b.styleSheet
            && a.renderMode == b.renderMode
            && a.ucsPerViewport == b.ucsPerViewport
            && a.ucsAtOrigin == b.ucsAtOrigin
            && a.ucsOrigin.x == b.ucsOrigin.x
            && a.ucsOrigin.y == b.ucsOrigin.y
            && a.ucsOrigin.z == b.ucsOrigin.z
            && a.ucsXAxis.x == b.ucsXAxis.x
            && a.ucsXAxis.y == b.ucsXAxis.y
            && a.ucsXAxis.z == b.ucsXAxis.z
            && a.ucsYAxis.x == b.ucsYAxis.x
            && a.ucsYAxis.y == b.ucsYAxis.y
            && a.ucsYAxis.z == b.ucsYAxis.z
            && a.ucsElevation == b.ucsElevation
            && a.ucsOrthographicType == b.ucsOrthographicType
            && a.shadePlotMode == b.shadePlotMode
            && a.useDefaultLighting == b.useDefaultLighting
            && a.defaultLightingType == b.defaultLightingType
            && a.brightness == b.brightness && a.contrast == b.contrast
            && a.ambientColor == b.ambientColor
            && a.ambientColorRgb == b.ambientColorRgb
            && a.ambientColorName == b.ambientColorName;
    };
    class ViewportExport final : public dx_iface {
    public:
        ViewportExport(dxfRW& codec, DRW_Viewport source)
            : output(codec), viewport(std::move(source)) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
        }
        void writeEntities() override {
            written = output.writeViewport(&viewport);
        }
        bool finalizeDxfWrite() override { return written; }
        dxfRW& output;
        dx_data storage;
        DRW_Viewport viewport;
        bool written = false;
    };
    class ViewportReadback final : public dx_iface {
    public:
        ViewportReadback() {
            cData = &storage;
            currentBlock = storage.mBlock;
        }
        void addViewport(const DRW_Viewport& value) override {
            viewport = value;
            ++count;
        }
        dx_data storage;
        DRW_Viewport viewport;
        unsigned count = 0u;
    };
    const auto readbackMatchesProfile = [&](
        const std::filesystem::path& path, DRW::Version version,
        const DRW_Viewport& source) {
        dxfRW reader(path.string().c_str());
        ViewportReadback probe;
        DRW_Viewport expected(source);
        const bool read = reader.read(&probe, true);
        if (probe.count == 1u)
            expected.handle = probe.viewport.handle;
        return read && reader.getSourceVersion() == version
            && probe.count == 1u && probe.viewport.handle != 0u
            && sameViewport(probe.viewport, expected);
    };
    const auto readbackMatches = [&](const std::filesystem::path& path,
                                     DRW::Version version) {
        return readbackMatchesProfile(path, version, makeViewport(version));
    };
    const std::array<std::pair<DRW::Version, const char*>, 8> versions = {{
        {DRW::AC1009, "AC1009"}, {DRW::AC1014, "AC1014"},
        {DRW::AC1015, "AC1015"},
        {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
        {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
        {DRW::AC1032, "AC1032"}}};
    const std::string sentinel = "prior viewport destination\n";
    const auto refusalThenUpgrade = [&](const char* label,
                                        DRW::Version refusedVersion,
                                        DRW::Version supportedVersion,
                                        const auto& mutate) {
        for (const bool binary : {false, true}) {
            const std::string stem = std::string("viewport-loss-") + label
                + (binary ? "-binary" : "-ascii");
            const auto output = directory / (stem + ".dxf");
            const auto expectedPath = directory / (stem + "-expected.dxf");
            if (!clearStagedOutput(output) || !clearStagedOutput(expectedPath))
                return false;
            DRW_Viewport source = makeViewport(supportedVersion);
            mutate(source);
            {
                dxfRW expectedCodec(expectedPath.string().c_str());
                ViewportExport expectedAdapter(expectedCodec, source);
                if (!expectedCodec.write(&expectedAdapter, supportedVersion, binary)
                    || !expectedAdapter.written)
                    return false;
            }
            std::string expectedBytes;
            if (!readBytes(expectedPath, expectedBytes))
                return false;
            {
                std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior.good())
                    return false;
            }
            dxfRW codec(output.string().c_str());
            ViewportExport adapter(codec, source);
            const DRW_Viewport original(adapter.viewport);
            if (codec.write(&adapter, refusedVersion, binary)
                || adapter.written || !sameViewport(adapter.viewport, original)
                || adapter.viewport.handle != original.handle)
                return false;
            std::string afterRefusal;
            if (!readBytes(output, afterRefusal) || afterRefusal != sentinel
                || hasStagedOutput(output))
                return false;
            if (!codec.write(&adapter, supportedVersion, binary)
                || !adapter.written || !sameViewport(adapter.viewport, original)
                || adapter.viewport.handle != original.handle)
                return false;
            std::string retryBytes;
            const bool passed = readBytes(output, retryBytes)
                && retryBytes == expectedBytes
                && readbackMatchesProfile(output, supportedVersion, source)
                && !hasStagedOutput(output);
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
            std::filesystem::remove(expectedPath, ignored);
            if (!passed)
                return false;
        }
        return true;
    };
    const auto refuseUnrepresentable = [&](const char* label,
                                           DRW::Version target,
                                           const auto& mutate,
                                           const auto& inspect) {
        for (const bool binary : {false, true}) {
            const auto output = directory
                / (std::string("viewport-unrepresentable-") + label
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            if (!clearStagedOutput(output))
                return false;
            {
                std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior.good())
                    return false;
            }
            dxfRW codec(output.string().c_str());
            ViewportExport adapter(codec, makeViewport(target));
            mutate(adapter.viewport);
            const auto originalValue = inspect(adapter.viewport);
            const DRW_Viewport original(adapter.viewport);
            const bool wrote = codec.write(&adapter, target, binary);
            std::string afterRefusal;
            const bool passed = !wrote && !adapter.written
                && inspect(adapter.viewport) == originalValue
                && sameViewport(adapter.viewport, original)
                && adapter.viewport.handle == original.handle
                && readBytes(output, afterRefusal)
                && afterRefusal == sentinel && !hasStagedOutput(output);
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
            if (!passed)
                return false;
        }
        return true;
    };
    if (!refusalThenUpgrade("r12-fields", DRW::AC1009, DRW::AC1014,
            [](DRW_Viewport&) {}))
        return fail("R12 target silently accepted nondefault post-R12 state");
    if (!refusalThenUpgrade("major-grid", DRW::AC1018, DRW::AC1021,
            [](DRW_Viewport& value) { value.majorGridLines = 7; }))
        return fail("older target silently accepted nondefault major-grid count");
    if (!refusalThenUpgrade("shade-plot", DRW::AC1015, DRW::AC1018,
            [](DRW_Viewport& value) { value.shadePlotMode = 2; }))
        return fail("older target silently accepted nondefault shade-plot mode");
    if (!refusalThenUpgrade("lighting", DRW::AC1018, DRW::AC1021,
            [](DRW_Viewport& value) { value.ambientColorName = "Newer book color"; }))
        return fail("older target silently accepted nondefault ambient-light state");
    if (!refuseUnrepresentable("clip-boundary", DRW::AC1014,
            [](DRW_Viewport& value) { value.clipBoundaryHandle = 0xDA71u; },
            [](const DRW_Viewport& value) { return value.clipBoundaryHandle; })
        || !refuseUnrepresentable("named-ucs", DRW::AC1014,
            [](DRW_Viewport& value) { value.namedUcsHandle = 0xDA72u; },
            [](const DRW_Viewport& value) { return value.namedUcsHandle; })
        || !refuseUnrepresentable("base-ucs", DRW::AC1014,
            [](DRW_Viewport& value) { value.baseUcsHandle = 0xDA73u; },
            [](const DRW_Viewport& value) { return value.baseUcsHandle; })
        || !refuseUnrepresentable("visual-style", DRW::AC1018,
            [](DRW_Viewport& value) { value.visualStyleHandle = 0xDA74u; },
            [](const DRW_Viewport& value) { return value.visualStyleHandle; })
        || !refuseUnrepresentable("frozen-layer", DRW::AC1009,
            [](DRW_Viewport& value) {
                value.frozenLayerHandles = {0xDA75u};
            },
            [](const DRW_Viewport& value) {
                return value.frozenLayerHandles;
            }))
        return fail("older target silently accepted an unrepresentable reference");
    for (const bool binary : {false, true}) {
        const auto output = directory
            / (binary ? "viewport-refusal-binary.dxf"
                      : "viewport-refusal-ascii.dxf");
        const auto expectedPath = directory
            / (binary ? "viewport-retry-expected-binary.dxf"
                      : "viewport-retry-expected-ascii.dxf");
        if (!clearStagedOutput(output) || !clearStagedOutput(expectedPath))
            return fail("could not clear refusal/retry staging files");
        {
            dxfRW codec(expectedPath.string().c_str());
            ViewportExport adapter(codec, makeViewport(DRW::AC1014));
            if (!codec.write(&adapter, DRW::AC1014, binary) || !adapter.written)
                return fail("could not create refusal/retry reference output");
        }
        std::string expectedBytes;
        if (!readBytes(expectedPath, expectedBytes))
            return fail("could not read refusal/retry reference output");
        {
            std::ofstream prior(output, std::ios::binary | std::ios::trunc);
            prior << sentinel;
            if (!prior.good())
                return fail("could not seed refused destination");
        }
        dxfRW codec(output.string().c_str());
        ViewportExport adapter(codec, makeViewport(DRW::AC1014));
        const DRW_Viewport original(adapter.viewport);
        if (codec.write(&adapter, DRW::AC1012, binary) || adapter.written
            || !sameViewport(adapter.viewport, original)
            || adapter.viewport.handle != original.handle)
            return fail("unsupported AC1012 route was not refused atomically");
        std::string afterRefusal;
        if (!readBytes(output, afterRefusal) || afterRefusal != sentinel
            || hasStagedOutput(output))
            return fail("AC1012 refusal changed destination state");
        if (!codec.write(&adapter, DRW::AC1014, binary) || !adapter.written
            || !sameViewport(adapter.viewport, original)
            || adapter.viewport.handle != original.handle)
            return fail("same-writer AC1014 retry failed");
        std::string retryBytes;
        if (!readBytes(output, retryBytes) || retryBytes != expectedBytes
            || !readbackMatches(output, DRW::AC1014)
            || hasStagedOutput(output))
            return fail("AC1014 retry differs from clean output");
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
        std::filesystem::remove(expectedPath, ignored);
    }
    for (const auto& version : versions) {
        for (const bool binary : {false, true}) {
            const auto output = directory
                / (std::string("viewport-") + version.second
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            if (!clearStagedOutput(output))
                return fail("could not clear stale output staging files");
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                ViewportExport adapter(codec, makeViewport(version.first));
                if (!codec.write(&adapter, version.first, binary)
                    || !adapter.written)
                    return fail("calibration write failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            {
                dxfRW codec(output.string().c_str());
                ViewportExport adapter(codec, makeViewport(version.first));
                armAllocationFailure(1000000000L);
                const bool wrote = codec.write(&adapter, version.first, binary);
                allocationCount = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
                if (!wrote || !adapter.written || allocationFailed
                    || allocationCount <= 0 || !readBytes(output, expectedBytes)
                    || !readbackMatches(output, version.first))
                    return fail("calibration write/readback failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior.good())
                        return fail("could not seed prior destination");
                }
                dxfRW codec(output.string().c_str());
                ViewportExport adapter(codec, makeViewport(version.first));
                const DRW_Viewport original(adapter.viewport);
                armAllocationFailure(offset);
                const bool wrote = codec.write(&adapter, version.first, binary);
                const bool injected = allocationFailed;
                disarmAllocationFailure();
                std::string afterFailure;
                if (wrote || !injected || !readBytes(output, afterFailure)
                    || afterFailure != sentinel
                    || !sameViewport(adapter.viewport, original)
                    || adapter.viewport.handle != original.handle
                    || hasStagedOutput(output))
                    return fail("injected failure changed caller or destination");
                const bool retried = codec.write(&adapter, version.first, binary)
                    && adapter.written;
                std::string retryBytes;
                if (!retried || !sameViewport(adapter.viewport, original)
                    || adapter.viewport.handle != original.handle
                    || !readBytes(output, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(output, version.first)
                    || hasStagedOutput(output))
                    return fail("same-writer retry or typed readback failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            std::cout << "VIEWPORT " << version.second << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
        }
    }
    return true;
}

bool runCameraSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "CAMERA allocation transaction: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const DRW_View sourceView = [] {
        DRW_View value;
        value.handle = 0xDA31u;
        value.name = "CAMERA_VIEW_MATRIX";
        value.size = DRW_Coord(640.0, 480.0, 0.0);
        value.center = DRW_Coord(0.5, 0.5, 0.0);
        value.viewDirectionFromTarget = DRW_Coord(0.0, 0.0, 1.0);
        value.targetPoint = DRW_Coord(10.0, 20.0, 30.0);
        return value;
    }();
    const auto makeCamera = [] {
        DRW_Camera value;
        value.handle = 0xDA30u;
        value.m_viewHandle = 0xDA31u;
        return value;
    };
    const auto sameView = [](const DRW_View& a, const DRW_View& b) {
        return a.handle == b.handle && a.name == b.name
            && a.size.x == b.size.x && a.size.y == b.size.y
            && a.size.z == b.size.z && a.center.x == b.center.x
            && a.center.y == b.center.y
            && a.viewDirectionFromTarget.x == b.viewDirectionFromTarget.x
            && a.viewDirectionFromTarget.y == b.viewDirectionFromTarget.y
            && a.viewDirectionFromTarget.z == b.viewDirectionFromTarget.z
            && a.targetPoint.x == b.targetPoint.x
            && a.targetPoint.y == b.targetPoint.y
            && a.targetPoint.z == b.targetPoint.z;
    };
    class CameraExport final : public dx_iface {
    public:
        CameraExport(dxfRW& codec, DRW_View view, DRW_Camera source)
            : output(codec), camera(std::move(source)) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
            storage.views.push_back(std::move(view));
        }
        void writeEntities() override { written = output.writeCamera(&camera); }
        bool finalizeDxfWrite() override { return written; }
        dxfRW& output;
        dx_data storage;
        DRW_Camera camera;
        bool written = false;
    };
    class CameraReadback final : public dx_iface {
    public:
        CameraReadback() {
            cData = &storage;
            currentBlock = storage.mBlock;
        }
        void addView(const DRW_View& value) override {
            view = value;
            hasView = true;
        }
        void addCamera(const DRW_Camera& value) override {
            camera = value;
            hasCamera = true;
        }
        dx_data storage;
        DRW_View view;
        DRW_Camera camera;
        bool hasView = false;
        bool hasCamera = false;
    };
    const auto readbackMatches = [&](const std::filesystem::path& path,
                                     DRW::Version expectedVersion) {
        dxfRW reader(path.string().c_str());
        CameraReadback probe;
        const bool read = reader.read(&probe, true);
        DRW_View expectedView = sourceView;
        if (probe.hasView)
            expectedView.handle = probe.view.handle;
        return read
            && reader.getSourceVersion() == expectedVersion
            && probe.hasView && probe.hasCamera
            && sameView(probe.view, expectedView)
            && probe.view.handle != 0u && probe.camera.handle != 0u
            && probe.camera.m_viewHandle == probe.view.handle;
    };
    const std::array<std::pair<DRW::Version, const char*>, 6> versions = {{
        {DRW::AC1015, "AC1015"}, {DRW::AC1018, "AC1018"},
        {DRW::AC1021, "AC1021"}, {DRW::AC1024, "AC1024"},
        {DRW::AC1027, "AC1027"}, {DRW::AC1032, "AC1032"}}};
    const std::string sentinel = "prior camera document\n";
    for (const auto& version : versions) {
        for (const bool binary : {false, true}) {
            const auto output = directory
                / (std::string("camera-") + version.second
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            if (!clearStagedOutput(output))
                return fail("could not clear stale output staging files");
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                CameraExport adapter(codec, sourceView, makeCamera());
                if (!codec.write(&adapter, version.first, binary))
                    return fail("warm-up write failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            {
                dxfRW codec(output.string().c_str());
                CameraExport adapter(codec, sourceView, makeCamera());
                armAllocationFailure(1000000000L);
                const bool wrote = codec.write(&adapter, version.first, binary);
                allocationCount = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
                if (!wrote || allocationFailed || allocationCount <= 0
                    || !readBytes(output, expectedBytes))
                    return fail("calibration write failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                                       std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior.good())
                        return fail("could not seed prior destination");
                }
                dxfRW codec(output.string().c_str());
                CameraExport adapter(codec, sourceView, makeCamera());
                const std::uint32_t sourceCameraHandle = adapter.camera.handle;
                const std::uint32_t sourceViewHandle =
                    adapter.camera.m_viewHandle;
                const DRW_View originalView = adapter.storage.views.front();
                armAllocationFailure(offset);
                const bool wrote = codec.write(&adapter, version.first, binary);
                const bool injected = allocationFailed;
                disarmAllocationFailure();
                std::string afterFailure;
                const bool bytesRead = readBytes(output, afterFailure);
                const auto& context = *codec.getWritingContext();
                const bool identitiesRolledBack =
                    context.sourceHandleToMintedMap.empty()
                    && context.sourceHandleToWrittenObjectMap.empty()
                    && context.writtenSourceEntityHandles.empty()
                    && context.ambiguousSourceHandles.empty();
                const bool callerSame = adapter.camera.handle == sourceCameraHandle
                    && adapter.camera.m_viewHandle == sourceViewHandle
                    && adapter.storage.views.size() == 1u
                    && sameView(adapter.storage.views.front(), originalView);
                const bool stageRemains = hasStagedOutput(output);
                if (wrote || !injected || !bytesRead || afterFailure != sentinel
                    || !identitiesRolledBack || !callerSame || stageRemains)
                    return fail("injected allocation failure changed state");
                const bool retried = codec.write(&adapter, version.first, binary);
                std::string retryBytes;
                if (!retried || adapter.camera.handle != sourceCameraHandle
                    || adapter.camera.m_viewHandle != sourceViewHandle
                    || adapter.storage.views.size() != 1u
                    || !sameView(adapter.storage.views.front(), originalView)
                    || !readBytes(output, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(output, version.first)
                    || hasStagedOutput(output))
                    return fail("same-codec retry or graph readback failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            std::cout << "CAMERA " << version.second << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", graph rollback/retry/readback=PASS\n";
        }
    }
    return true;
}

bool runLightEntitySweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "LIGHT entity allocation transaction: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const auto makeLight = [] {
        DRW_Light value;
        value.handle = 0xDA30u;
        value.m_classVersion = 1u;
        value.m_name = "ALLOCATION_MATRIX_LIGHT";
        value.m_type = 2u;
        value.m_status = true;
        value.m_color = 3u;
        value.m_plotGlyph = true;
        value.m_intensity = 2.5;
        value.m_position = DRW_Coord(10.0, 11.0, 12.0);
        value.m_target = DRW_Coord(13.0, 14.0, 15.0);
        value.m_attenuationType = 1u;
        value.m_useAttenuationLimits = true;
        value.m_attenuationStartLimit = 0.5;
        value.m_attenuationEndLimit = 12.5;
        value.m_hotspotAngle = 0.25;
        value.m_falloffAngle = 0.75;
        value.m_castShadows = true;
        value.m_shadowType = 1u;
        value.m_shadowMapSize = 512u;
        value.m_shadowMapSoftness = 2u;
        return value;
    };
    const auto sameLight = [](const DRW_Light& a, const DRW_Light& b) {
        return a.handle == b.handle && a.m_classVersion == b.m_classVersion
            && a.m_name == b.m_name && a.m_type == b.m_type
            && a.m_status == b.m_status && a.m_color == b.m_color
            && a.m_colorRgb == b.m_colorRgb
            && a.m_plotGlyph == b.m_plotGlyph
            && a.m_intensity == b.m_intensity
            && a.m_position.x == b.m_position.x
            && a.m_position.y == b.m_position.y
            && a.m_position.z == b.m_position.z
            && a.m_target.x == b.m_target.x
            && a.m_target.y == b.m_target.y
            && a.m_target.z == b.m_target.z
            && a.m_attenuationType == b.m_attenuationType
            && a.m_useAttenuationLimits == b.m_useAttenuationLimits
            && a.m_attenuationStartLimit == b.m_attenuationStartLimit
            && a.m_attenuationEndLimit == b.m_attenuationEndLimit
            && a.m_hotspotAngle == b.m_hotspotAngle
            && a.m_falloffAngle == b.m_falloffAngle
            && a.m_castShadows == b.m_castShadows
            && a.m_shadowType == b.m_shadowType
            && a.m_shadowMapSize == b.m_shadowMapSize
            && a.m_shadowMapSoftness == b.m_shadowMapSoftness;
    };
    class LightExport final : public dx_iface {
    public:
        LightExport(dxfRW& codec, DRW_Light source)
            : output(codec), light(std::move(source)) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
        }
        void writeEntities() override { written = output.writeLight(&light); }
        bool finalizeDxfWrite() override { return written; }
        dxfRW& output;
        dx_data storage;
        DRW_Light light;
        bool written = false;
    };
    class LightReadback final : public dx_iface {
    public:
        LightReadback() {
            cData = &storage;
            currentBlock = storage.mBlock;
        }
        void addLight(const DRW_Light& value) override {
            light = value;
            found = true;
        }
        dx_data storage;
        DRW_Light light;
        bool found = false;
    };
    const auto readbackMatches = [&](const std::filesystem::path& path,
                                     DRW::Version expectedVersion) {
        dxfRW reader(path.string().c_str());
        LightReadback probe;
        const bool read = reader.read(&probe, true);
        DRW_Light expected = makeLight();
        if (probe.found)
            expected.handle = probe.light.handle;
        return read && reader.getSourceVersion() == expectedVersion
            && probe.found && probe.light.handle != 0u
            && sameLight(probe.light, expected);
    };
    const std::array<std::pair<DRW::Version, const char*>, 4> versions = {{
        {DRW::AC1021, "AC1021"}, {DRW::AC1024, "AC1024"},
        {DRW::AC1027, "AC1027"}, {DRW::AC1032, "AC1032"}}};
    const std::string sentinel = "previous complete document\n";
    for (const auto& version : versions) {
        for (const bool binary : {false, true}) {
            const auto output = directory
                / (std::string("light-entity-") + version.second
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            if (!clearStagedOutput(output))
                return fail("could not clear stale output staging files");
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                LightExport adapter(codec, makeLight());
                if (!codec.write(&adapter, version.first, binary))
                    return fail("warm-up write failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            {
                dxfRW codec(output.string().c_str());
                LightExport adapter(codec, makeLight());
                armAllocationFailure(1000000000L);
                const bool wrote = codec.write(&adapter, version.first, binary);
                allocationCount = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
                if (!wrote || allocationFailed || allocationCount <= 0
                    || !readBytes(output, expectedBytes))
                    return fail("calibration write failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                                       std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior.good())
                        return fail("could not seed prior destination");
                }
                dxfRW codec(output.string().c_str());
                LightExport adapter(codec, makeLight());
                const DRW_Light original(adapter.light);
                armAllocationFailure(offset);
                const bool wrote = codec.write(&adapter, version.first, binary);
                const bool injected = allocationFailed;
                disarmAllocationFailure();
                std::string afterFailure;
                const bool bytesRead = readBytes(output, afterFailure);
                const auto& context = *codec.getWritingContext();
                const bool identitiesRolledBack =
                    context.sourceHandleToMintedMap.empty()
                    && context.sourceHandleToWrittenObjectMap.empty()
                    && context.writtenSourceEntityHandles.empty()
                    && context.ambiguousSourceHandles.empty();
                const bool stageRemains = hasStagedOutput(output);
                if (wrote || !injected || !bytesRead || afterFailure != sentinel
                    || !sameLight(adapter.light, original)
                    || !identitiesRolledBack || stageRemains)
                    return fail("injected allocation failure changed state");
                const bool retried = codec.write(&adapter, version.first, binary);
                std::string retryBytes;
                const bool retryRead = readBytes(output, retryBytes);
                const bool readback = retryRead
                    && readbackMatches(output, version.first);
                const bool staged = hasStagedOutput(output);
                if (!retried || !sameLight(adapter.light, original)
                    || !retryRead || retryBytes != expectedBytes
                    || !readback || staged) {
                    std::cerr << "version=" << version.second << " binary="
                              << binary << " retried=" << retried
                              << " callerSame=" << sameLight(adapter.light, original)
                              << " retryRead=" << retryRead
                              << " bytesMatch=" << (retryBytes == expectedBytes)
                              << " readback=" << readback << " staged=" << staged
                              << '\n';
                    return fail("same-codec retry or typed readback failed");
                }
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            std::cout << "LIGHT entity " << version.second << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
        }
    }
    return true;
}

bool runOleFrameVersionSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "OLE frame version allocation transaction: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const auto makePayload = [](std::uint8_t seed) {
        std::vector<std::uint8_t> bytes(130u);
        for (std::size_t i = 0u; i < bytes.size(); ++i)
            bytes[i] = static_cast<std::uint8_t>(seed + i * 17u);
        return bytes;
    };
    const auto makeOle2 = [&] {
        DRW_Ole2Frame value;
        value.m_oleVersion = 2u;
        value.m_oleClient = "LibreCAD-OLE2";
        value.m_pt1 = DRW_Coord(1.0, 2.0, 3.0);
        value.m_pt2 = DRW_Coord(4.0, 5.0, 6.0);
        value.m_flags = 2u;
        value.m_mode = 1u;
        value.m_lockAspect = 1u;
        value.m_payloadBytes = makePayload(0x21u);
        value.m_payloadPresent = true;
        value.m_declaredPayloadLength =
            static_cast<std::uint32_t>(value.m_payloadBytes.size());
        return value;
    };
    const auto makeOle = [&] {
        DRW_OleFrame value;
        value.m_flags = 1u;
        value.m_payloadBytes = makePayload(0x47u);
        value.m_payloadPresent = true;
        value.m_declaredPayloadLength =
            static_cast<std::uint32_t>(value.m_payloadBytes.size());
        return value;
    };
    const auto sameOle2 = [](const DRW_Ole2Frame& actual,
                             const DRW_Ole2Frame& expected) {
        return actual.handle == expected.handle
            && actual.m_oleVersion == expected.m_oleVersion
            && actual.m_oleClient == expected.m_oleClient
            && actual.m_pt1.x == expected.m_pt1.x
            && actual.m_pt1.y == expected.m_pt1.y
            && actual.m_pt1.z == expected.m_pt1.z
            && actual.m_pt2.x == expected.m_pt2.x
            && actual.m_pt2.y == expected.m_pt2.y
            && actual.m_pt2.z == expected.m_pt2.z
            && actual.m_flags == expected.m_flags
            && actual.m_mode == expected.m_mode
            && actual.m_lockAspect == expected.m_lockAspect
            && actual.m_payloadBytes == expected.m_payloadBytes;
    };
    const auto sameOle = [](const DRW_OleFrame& actual,
                            const DRW_OleFrame& expected) {
        return actual.handle == expected.handle
            && actual.m_flags == expected.m_flags
            && actual.m_payloadBytes == expected.m_payloadBytes;
    };
    class OleExport final : public dx_iface {
    public:
        OleExport(dxfRW& codec, DRW_Ole2Frame framed, DRW_OleFrame legacy)
            : output(codec), ole2(std::move(framed)), ole(std::move(legacy)) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
        }
        void writeEntities() override {
            wroteOle2 = output.writeOle2Frame(&ole2);
            wroteOle = wroteOle2 && output.writeOleFrame(&ole);
        }
        bool finalizeDxfWrite() override { return wroteOle; }
        dxfRW& output;
        dx_data storage;
        DRW_Ole2Frame ole2;
        DRW_OleFrame ole;
        bool wroteOle2 = false;
        bool wroteOle = false;
    };
    const auto readbackMatches = [&](const std::filesystem::path& path,
                                     DRW::Version expectedVersion) {
        dx_data decoded;
        dx_iface reader;
        if (!reader.fileImport(path.string(), &decoded, false)
            || decoded.sourceVersion != expectedVersion
            || decoded.mBlock == nullptr || decoded.mBlock->ent.size() != 2u) {
            std::cerr << "  RAY/XLINE inventory/import mismatch\n";
            return false;
        }
        const auto firstEntity = decoded.mBlock->ent.begin();
        const auto* actualOle2 = dynamic_cast<const DRW_Ole2Frame*>(
            *firstEntity);
        const auto* actualOle = dynamic_cast<const DRW_OleFrame*>(
            *std::next(firstEntity));
        DRW_Ole2Frame expectedOle2 = makeOle2();
        DRW_OleFrame expectedOle = makeOle();
        if (actualOle2 != nullptr)
            expectedOle2.handle = actualOle2->handle;
        if (actualOle != nullptr)
            expectedOle.handle = actualOle->handle;
        return actualOle2 != nullptr && actualOle != nullptr
            && actualOle2->handle != 0u && actualOle->handle != 0u
            && sameOle2(*actualOle2, expectedOle2)
            && sameOle(*actualOle, expectedOle)
            && actualOle2->m_payloadPresent && actualOle->m_payloadPresent;
    };
    const std::array<std::pair<DRW::Version, const char*>, 7> versions = {{
        {DRW::AC1014, "AC1014"}, {DRW::AC1015, "AC1015"},
        {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
        {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
        {DRW::AC1032, "AC1032"}}};
    const std::string sentinel = "previous complete OLE destination\n";
    for (const auto& version : versions) {
        for (const bool binary : {false, true}) {
            const auto output = directory
                / (std::string("ole-frames-") + version.second
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            if (!clearStagedOutput(output))
                return fail("could not clear stale output staging files");
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                OleExport adapter(codec, makeOle2(), makeOle());
                if (!codec.write(&adapter, version.first, binary)
                    || !adapter.wroteOle2 || !adapter.wroteOle)
                    return fail("warm-up write failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            {
                dxfRW codec(output.string().c_str());
                OleExport adapter(codec, makeOle2(), makeOle());
                armAllocationFailure(1000000000L);
                const bool wrote = codec.write(&adapter, version.first, binary);
                allocationCount = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
                if (!wrote || !adapter.wroteOle2 || !adapter.wroteOle
                    || allocationFailed || allocationCount <= 0
                    || !readBytes(output, expectedBytes)
                    || !readbackMatches(output, version.first))
                    return fail("calibration write/readback failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior.good())
                        return fail("could not seed prior destination");
                }
                dxfRW codec(output.string().c_str());
                OleExport adapter(codec, makeOle2(), makeOle());
                const DRW_Ole2Frame originalOle2(adapter.ole2);
                const DRW_OleFrame originalOle(adapter.ole);
                armAllocationFailure(offset);
                const bool wrote = codec.write(&adapter, version.first, binary);
                const bool injected = allocationFailed;
                disarmAllocationFailure();
                std::string afterFailure;
                const bool bytesRead = readBytes(output, afterFailure);
                const bool callerSame = sameOle2(adapter.ole2, originalOle2)
                    && sameOle(adapter.ole, originalOle);
                const auto& context = *codec.getWritingContext();
                const bool identitiesRolledBack =
                    context.sourceHandleToMintedMap.empty()
                    && context.sourceHandleToWrittenObjectMap.empty()
                    && context.writtenSourceEntityHandles.empty()
                    && context.ambiguousSourceHandles.empty();
                if (wrote || !injected || !bytesRead || afterFailure != sentinel
                    || !callerSame || !identitiesRolledBack
                    || hasStagedOutput(output)) {
                    std::cerr << "version=" << version.second << " binary="
                              << binary << " offset=" << offset
                              << " wrote=" << wrote << " injected=" << injected
                              << " destination=" << (afterFailure == sentinel)
                              << " caller=" << callerSame << '\n';
                    return fail("injected failure changed caller or destination state");
                }
                const bool retried = codec.write(&adapter, version.first, binary)
                    && adapter.wroteOle2 && adapter.wroteOle;
                std::string retryBytes;
                const bool retryRead = readBytes(output, retryBytes);
                if (!retried || !sameOle2(adapter.ole2, originalOle2)
                    || !sameOle(adapter.ole, originalOle)
                    || !retryRead || retryBytes != expectedBytes
                    || !readbackMatches(output, version.first)
                    || hasStagedOutput(output))
                    return fail("same-writer retry or typed readback failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            std::cout << "OLEFRAME/OLE2FRAME " << version.second << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
        }
    }
    return true;
}

bool runRayXlineVersionSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "RAY/XLINE version allocation transaction: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const auto makeRay = [] {
        DRW_Ray value;
        value.basePoint = DRW_Coord(1.25, -2.5, 3.75);
        value.secPoint = DRW_Coord(0.0, 0.0, 1.0);
        return value;
    };
    const auto makeXline = [] {
        DRW_Xline value;
        value.basePoint = DRW_Coord(-7.0, 8.5, 9.25);
        value.secPoint = DRW_Coord(1.0, 0.0, 0.0);
        return value;
    };
    const auto sameDirection = [](const DRW_Line& actual,
                                  const DRW_Line& expected) {
        return actual.handle == expected.handle
            && actual.basePoint.x == expected.basePoint.x
            && actual.basePoint.y == expected.basePoint.y
            && actual.basePoint.z == expected.basePoint.z
            && actual.secPoint.x == expected.secPoint.x
            && actual.secPoint.y == expected.secPoint.y
            && actual.secPoint.z == expected.secPoint.z;
    };
    class DirectionExport final : public dx_iface {
    public:
        DirectionExport(dxfRW& codec, DRW_Ray sourceRay,
                        DRW_Xline sourceXline)
            : output(codec), ray(std::move(sourceRay)),
              xline(std::move(sourceXline)) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
        }
        void writeEntities() override {
            wroteRay = output.writeRay(&ray);
            wroteXline = wroteRay && output.writeXline(&xline);
        }
        bool finalizeDxfWrite() override { return wroteXline; }
        dxfRW& output;
        dx_data storage;
        DRW_Ray ray;
        DRW_Xline xline;
        bool wroteRay = false;
        bool wroteXline = false;
    };
    const auto readbackMatches = [&](const std::filesystem::path& path,
                                     DRW::Version expectedVersion) {
        dx_data decoded;
        dx_iface reader;
        if (!reader.fileImport(path.string(), &decoded, false)
            || decoded.sourceVersion != expectedVersion
            || decoded.mBlock == nullptr || decoded.mBlock->ent.size() != 2u)
            return false;
        auto entity = decoded.mBlock->ent.begin();
        const auto* ray = dynamic_cast<const DRW_Ray*>(*entity++);
        const auto* xline = dynamic_cast<const DRW_Xline*>(*entity);
        DRW_Ray expectedRay = makeRay();
        DRW_Xline expectedXline = makeXline();
        if (ray != nullptr)
            expectedRay.handle = ray->handle;
        if (xline != nullptr)
            expectedXline.handle = xline->handle;
        const bool matched = ray != nullptr && xline != nullptr
            && ray->handle != 0u && xline->handle != 0u
            && sameDirection(*ray, expectedRay)
            && sameDirection(*xline, expectedXline);
        if (!matched)
            std::cerr << "  RAY/XLINE fields/type mismatch: ray="
                      << (ray != nullptr) << " xline=" << (xline != nullptr)
                      << " handles=" << (ray == nullptr ? 0u : ray->handle)
                      << ',' << (xline == nullptr ? 0u : xline->handle)
                      << " base=" << (ray == nullptr ? 0.0 : ray->basePoint.x)
                      << ',' << (ray == nullptr ? 0.0 : ray->basePoint.y)
                      << ',' << (ray == nullptr ? 0.0 : ray->basePoint.z)
                      << " direction="
                      << (ray == nullptr ? 0.0 : ray->secPoint.x) << ','
                      << (ray == nullptr ? 0.0 : ray->secPoint.y) << ','
                      << (ray == nullptr ? 0.0 : ray->secPoint.z) << '\n';
        return matched;
    };
    const std::array<std::pair<DRW::Version, const char*>, 7> versions = {{
        {DRW::AC1014, "AC1014"}, {DRW::AC1015, "AC1015"},
        {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
        {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
        {DRW::AC1032, "AC1032"}}};
    const std::string sentinel = "previous complete RAY/XLINE destination\n";
    for (const auto& version : versions) {
        for (const bool binary : {false, true}) {
            const auto output = directory
                / (std::string("ray-xline-") + version.second
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            if (!clearStagedOutput(output))
                return fail("could not clear stale output staging files");
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                DirectionExport adapter(codec, makeRay(), makeXline());
                if (!codec.write(&adapter, version.first, binary)
                    || !adapter.wroteRay || !adapter.wroteXline)
                    return fail("warm-up write failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            {
                dxfRW codec(output.string().c_str());
                DirectionExport adapter(codec, makeRay(), makeXline());
                armAllocationFailure(1000000000L);
                const bool wrote = codec.write(&adapter, version.first, binary);
                allocationCount = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
                const bool bytesRead = readBytes(output, expectedBytes);
                const bool readback = bytesRead
                    && readbackMatches(output, version.first);
                if (!wrote || !adapter.wroteRay || !adapter.wroteXline
                    || allocationFailed || allocationCount <= 0
                    || !bytesRead || !readback) {
                    std::cerr << "  calibration: wrote=" << wrote
                              << " ray=" << adapter.wroteRay
                              << " xline=" << adapter.wroteXline
                              << " failed=" << allocationFailed
                              << " allocations=" << allocationCount
                              << " bytes=" << bytesRead
                              << " readback=" << readback << '\n';
                    return fail("calibration write/readback failed");
                }
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior.good())
                        return fail("could not seed prior destination");
                }
                dxfRW codec(output.string().c_str());
                DirectionExport adapter(codec, makeRay(), makeXline());
                const DRW_Ray originalRay(adapter.ray);
                const DRW_Xline originalXline(adapter.xline);
                armAllocationFailure(offset);
                const bool wrote = codec.write(&adapter, version.first, binary);
                const bool injected = allocationFailed;
                disarmAllocationFailure();
                std::string afterFailure;
                const bool bytesRead = readBytes(output, afterFailure);
                const bool callerSame = sameDirection(adapter.ray, originalRay)
                    && sameDirection(adapter.xline, originalXline);
                const auto& context = *codec.getWritingContext();
                const bool identitiesRolledBack =
                    context.sourceHandleToMintedMap.empty()
                    && context.sourceHandleToWrittenObjectMap.empty()
                    && context.writtenSourceEntityHandles.empty()
                    && context.ambiguousSourceHandles.empty();
                if (wrote || !injected || !bytesRead || afterFailure != sentinel
                    || !callerSame || !identitiesRolledBack
                    || hasStagedOutput(output))
                    return fail("injected failure changed caller or destination state");
                const bool retried = codec.write(&adapter, version.first, binary)
                    && adapter.wroteRay && adapter.wroteXline;
                std::string retryBytes;
                const bool retryRead = readBytes(output, retryBytes);
                if (!retried
                    || !sameDirection(adapter.ray, originalRay)
                    || !sameDirection(adapter.xline, originalXline) || !retryRead
                    || retryBytes != expectedBytes
                    || !readbackMatches(output, version.first)
                    || hasStagedOutput(output))
                    return fail("same-writer retry or typed readback failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            std::cout << "RAY/XLINE " << version.second << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
        }
    }
    return true;
}

bool runToleranceVersionSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "TOLERANCE version allocation transaction: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const auto makeTolerance = [] {
        DRW_Tolerance value;
        value.dimStyleName = "STANDARD";
        value.text = "%%v0.25^0.10";
        value.insertionPoint = DRW_Coord(2.5, -3.75, 4.125);
        value.extPoint = DRW_Coord(0.0, 0.0, 1.0);
        value.xAxisDirectionVector = DRW_Coord(0.0, 1.0, 0.0);
        return value;
    };
    const auto sameTolerance = [](const DRW_Tolerance& actual,
                                  const DRW_Tolerance& expected) {
        return actual.handle == expected.handle
            && actual.dimStyleName == expected.dimStyleName
            && actual.text == expected.text
            && actual.insertionPoint.x == expected.insertionPoint.x
            && actual.insertionPoint.y == expected.insertionPoint.y
            && actual.insertionPoint.z == expected.insertionPoint.z
            && actual.extPoint.x == expected.extPoint.x
            && actual.extPoint.y == expected.extPoint.y
            && actual.extPoint.z == expected.extPoint.z
            && actual.xAxisDirectionVector.x
                == expected.xAxisDirectionVector.x
            && actual.xAxisDirectionVector.y
                == expected.xAxisDirectionVector.y
            && actual.xAxisDirectionVector.z
                == expected.xAxisDirectionVector.z;
    };
    class ToleranceExport final : public dx_iface {
    public:
        ToleranceExport(dxfRW& codec, DRW_Tolerance source)
            : output(codec), tolerance(std::move(source)) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
        }
        void writeEntities() override {
            emitted = output.writeTolerance(&tolerance);
        }
        bool finalizeDxfWrite() override { return emitted; }
        dxfRW& output;
        dx_data storage;
        DRW_Tolerance tolerance;
        bool emitted = false;
    };
    const auto readbackMatches = [&](const std::filesystem::path& path,
                                     DRW::Version expectedVersion) {
        dx_data decoded;
        dx_iface reader;
        if (!reader.fileImport(path.string(), &decoded, false)
            || decoded.sourceVersion != expectedVersion
            || decoded.mBlock == nullptr || decoded.mBlock->ent.size() != 1u)
            return false;
        const auto* actual = dynamic_cast<const DRW_Tolerance*>(
            decoded.mBlock->ent.front());
        DRW_Tolerance expected = makeTolerance();
        if (actual != nullptr)
            expected.handle = actual->handle;
        return actual != nullptr && actual->handle != 0u
            && sameTolerance(*actual, expected);
    };
    const std::array<std::pair<DRW::Version, const char*>, 7> versions = {{
        {DRW::AC1014, "AC1014"}, {DRW::AC1015, "AC1015"},
        {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
        {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
        {DRW::AC1032, "AC1032"}}};
    const std::string sentinel = "previous complete TOLERANCE destination\n";
    for (const auto& version : versions) {
        for (const bool binary : {false, true}) {
            const auto output = directory
                / (std::string("tolerance-") + version.second
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            if (!clearStagedOutput(output))
                return fail("could not clear stale output staging files");
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                ToleranceExport adapter(codec, makeTolerance());
                if (!codec.write(&adapter, version.first, binary)
                    || !adapter.emitted)
                    return fail("warm-up write failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            {
                dxfRW codec(output.string().c_str());
                ToleranceExport adapter(codec, makeTolerance());
                armAllocationFailure(1000000000L);
                const bool wrote = codec.write(&adapter, version.first, binary);
                allocationCount = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
                if (!wrote || !adapter.emitted || allocationFailed
                    || allocationCount <= 0
                    || !readBytes(output, expectedBytes)
                    || !readbackMatches(output, version.first))
                    return fail("calibration write/readback failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior.good())
                        return fail("could not seed prior destination");
                }
                dxfRW codec(output.string().c_str());
                ToleranceExport adapter(codec, makeTolerance());
                const DRW_Tolerance original(adapter.tolerance);
                armAllocationFailure(offset);
                const bool wrote = codec.write(&adapter, version.first, binary);
                const bool injected = allocationFailed;
                disarmAllocationFailure();
                std::string afterFailure;
                const bool bytesRead = readBytes(output, afterFailure);
                const auto& context = *codec.getWritingContext();
                const bool identitiesRolledBack =
                    context.sourceHandleToMintedMap.empty()
                    && context.sourceHandleToWrittenObjectMap.empty()
                    && context.writtenSourceEntityHandles.empty()
                    && context.ambiguousSourceHandles.empty();
                if (wrote || !injected || !bytesRead || afterFailure != sentinel
                    || !sameTolerance(adapter.tolerance, original)
                    || !identitiesRolledBack || hasStagedOutput(output))
                    return fail("injected failure changed caller or destination state");
                const bool retried = codec.write(&adapter, version.first, binary)
                    && adapter.emitted;
                std::string retryBytes;
                if (!retried || !sameTolerance(adapter.tolerance, original)
                    || !readBytes(output, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(output, version.first)
                    || hasStagedOutput(output))
                    return fail("same-writer retry or typed readback failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            std::cout << "TOLERANCE " << version.second << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
        }
    }
    return true;
}

bool runHelixVersionSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "HELIX version allocation transaction: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const auto makeHelix = [] {
        DRW_Helix value;
        value.m_majorVersion = 1;
        value.m_maintVersion = 2;
        value.degree = 2;
        value.flags = 8;
        value.knotslist = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
        value.controllist = {
            std::make_shared<DRW_Coord>(1.0, 0.0, 0.0),
            std::make_shared<DRW_Coord>(0.0, 1.0, 2.0),
            std::make_shared<DRW_Coord>(-1.0, 0.0, 4.0)};
        value.axisBasePt = DRW_Coord(2.0, 3.0, 4.0);
        value.startPt = DRW_Coord(3.0, 3.0, 4.0);
        value.axisVector = DRW_Coord(0.0, 0.0, 1.0);
        value.radius = 1.0;
        value.turns = 2.5;
        value.turnHeight = 3.0;
        value.handedness = true;
        value.constraintType = 1;
        return value;
    };
    const auto sameHelix = [](const DRW_Helix& actual,
                              const DRW_Helix& expected) {
        if (actual.handle != expected.handle
            || actual.m_majorVersion != expected.m_majorVersion
            || actual.m_maintVersion != expected.m_maintVersion
            || actual.degree != expected.degree || actual.flags != expected.flags
            || actual.knotslist != expected.knotslist
            || actual.controllist.size() != expected.controllist.size()
            || actual.axisBasePt.x != expected.axisBasePt.x
            || actual.axisBasePt.y != expected.axisBasePt.y
            || actual.axisBasePt.z != expected.axisBasePt.z
            || actual.startPt.x != expected.startPt.x
            || actual.startPt.y != expected.startPt.y
            || actual.startPt.z != expected.startPt.z
            || actual.axisVector.x != expected.axisVector.x
            || actual.axisVector.y != expected.axisVector.y
            || actual.axisVector.z != expected.axisVector.z
            || actual.radius != expected.radius || actual.turns != expected.turns
            || actual.turnHeight != expected.turnHeight
            || actual.handedness != expected.handedness
            || actual.constraintType != expected.constraintType)
            return false;
        for (std::size_t i = 0u; i < actual.controllist.size(); ++i) {
            if (!actual.controllist[i] || !expected.controllist[i]
                || actual.controllist[i]->x != expected.controllist[i]->x
                || actual.controllist[i]->y != expected.controllist[i]->y
                || actual.controllist[i]->z != expected.controllist[i]->z)
                return false;
        }
        return true;
    };
    class HelixExport final : public dx_iface {
    public:
        HelixExport(dxfRW& codec, DRW_Helix source)
            : output(codec), helix(std::move(source)) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
        }
        void writeEntities() override { emitted = output.writeHelix(&helix); }
        bool finalizeDxfWrite() override { return emitted; }
        dxfRW& output;
        dx_data storage;
        DRW_Helix helix;
        bool emitted = false;
    };
    const auto readbackMatches = [&](const std::filesystem::path& path,
                                     DRW::Version expectedVersion) {
        dx_data decoded;
        dx_iface reader;
        if (!reader.fileImport(path.string(), &decoded, false)
            || decoded.sourceVersion != expectedVersion
            || decoded.mBlock == nullptr || decoded.mBlock->ent.size() != 1u)
            return false;
        const auto* actual = dynamic_cast<const DRW_Helix*>(
            decoded.mBlock->ent.front());
        DRW_Helix expected = makeHelix();
        if (actual != nullptr)
            expected.handle = actual->handle;
        return actual != nullptr && actual->handle != 0u
            && sameHelix(*actual, expected);
    };
    const std::array<std::pair<DRW::Version, const char*>, 7> versions = {{
        {DRW::AC1014, "AC1014"}, {DRW::AC1015, "AC1015"},
        {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
        {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
        {DRW::AC1032, "AC1032"}}};
    const std::string sentinel = "previous complete HELIX destination\n";
    for (const auto& version : versions) {
        for (const bool binary : {false, true}) {
            const auto output = directory
                / (std::string("helix-") + version.second
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            if (!clearStagedOutput(output))
                return fail("could not clear stale output staging files");
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                HelixExport adapter(codec, makeHelix());
                if (!codec.write(&adapter, version.first, binary)
                    || !adapter.emitted)
                    return fail("warm-up write failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            {
                dxfRW codec(output.string().c_str());
                HelixExport adapter(codec, makeHelix());
                armAllocationFailure(1000000000L);
                const bool wrote = codec.write(&adapter, version.first, binary);
                allocationCount = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
                if (!wrote || !adapter.emitted || allocationFailed
                    || allocationCount <= 0
                    || !readBytes(output, expectedBytes)
                    || !readbackMatches(output, version.first))
                    return fail("calibration write/readback failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior.good())
                        return fail("could not seed prior destination");
                }
                dxfRW codec(output.string().c_str());
                HelixExport adapter(codec, makeHelix());
                const DRW_Helix original(adapter.helix);
                armAllocationFailure(offset);
                const bool wrote = codec.write(&adapter, version.first, binary);
                const bool injected = allocationFailed;
                disarmAllocationFailure();
                std::string afterFailure;
                const bool bytesRead = readBytes(output, afterFailure);
                const auto& context = *codec.getWritingContext();
                const bool identitiesRolledBack =
                    context.sourceHandleToMintedMap.empty()
                    && context.sourceHandleToWrittenObjectMap.empty()
                    && context.writtenSourceEntityHandles.empty()
                    && context.ambiguousSourceHandles.empty();
                if (wrote || !injected || !bytesRead || afterFailure != sentinel
                    || !sameHelix(adapter.helix, original)
                    || !identitiesRolledBack || hasStagedOutput(output))
                    return fail("injected failure changed caller or destination state");
                const bool retried = codec.write(&adapter, version.first, binary)
                    && adapter.emitted;
                std::string retryBytes;
                if (!retried || !sameHelix(adapter.helix, original)
                    || !readBytes(output, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(output, version.first)
                    || hasStagedOutput(output))
                    return fail("same-writer retry or typed readback failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            std::cout << "HELIX " << version.second << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
        }
    }
    return true;
}

bool runSplineVersionSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "SPLINE version transaction: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    const auto makeSpline = [](bool fitOnly) {
        DRW_Spline value;
        value.normalVec = DRW_Coord(0.0, 0.0, 1.0);
        value.tgStart = DRW_Coord(1.0, 0.0, 0.0);
        value.tgEnd = DRW_Coord(0.0, 1.0, 0.0);
        value.tolknot = 1.0e-6;
        value.tolcontrol = 2.0e-6;
        value.tolfit = 3.0e-6;
        if (fitOnly) {
            value.degree = 3;
            value.fitlist = {
                std::make_shared<DRW_Coord>(DRW_Coord(0.0, 0.0, 0.0)),
                std::make_shared<DRW_Coord>(DRW_Coord(2.0, 3.0, 1.0)),
                std::make_shared<DRW_Coord>(DRW_Coord(4.0, 0.0, 2.0))};
            value.nfit = static_cast<std::int32_t>(value.fitlist.size());
        } else {
            value.degree = 2;
            value.flags = 12; // planar + rational for the non-unit middle weight
            value.knotslist = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
            value.weightlist = {1.0, 1.25, 1.0};
            value.controllist = {
                std::make_shared<DRW_Coord>(DRW_Coord(0.0, 0.0, 0.0)),
                std::make_shared<DRW_Coord>(DRW_Coord(2.0, 3.0, 1.0)),
                std::make_shared<DRW_Coord>(DRW_Coord(4.0, 0.0, 2.0))};
            value.nknots = static_cast<std::int32_t>(value.knotslist.size());
            value.ncontrol = static_cast<std::int32_t>(value.controllist.size());
        }
        return value;
    };
    const auto sameSpline = [](const DRW_Spline& actual,
                               const DRW_Spline& expected) {
        if (actual.handle != expected.handle || actual.degree != expected.degree
            || actual.flags != expected.flags
            || actual.nknots != expected.nknots
            || actual.ncontrol != expected.ncontrol
            || actual.nfit != expected.nfit
            || actual.normalVec.x != expected.normalVec.x
            || actual.normalVec.y != expected.normalVec.y
            || actual.normalVec.z != expected.normalVec.z
            || actual.tgStart.x != expected.tgStart.x
            || actual.tgStart.y != expected.tgStart.y
            || actual.tgStart.z != expected.tgStart.z
            || actual.tgEnd.x != expected.tgEnd.x
            || actual.tgEnd.y != expected.tgEnd.y
            || actual.tgEnd.z != expected.tgEnd.z
            || actual.tolknot != expected.tolknot
            || actual.tolcontrol != expected.tolcontrol
            || actual.tolfit != expected.tolfit
            || actual.knotslist != expected.knotslist
            || actual.weightlist != expected.weightlist
            || actual.controllist.size() != expected.controllist.size()
            || actual.fitlist.size() != expected.fitlist.size())
            return false;
        for (std::size_t i = 0; i < actual.controllist.size(); ++i) {
            if (!actual.controllist[i] || !expected.controllist[i]
                || actual.controllist[i]->x != expected.controllist[i]->x
                || actual.controllist[i]->y != expected.controllist[i]->y
                || actual.controllist[i]->z != expected.controllist[i]->z)
                return false;
        }
        for (std::size_t i = 0; i < actual.fitlist.size(); ++i) {
            if (!actual.fitlist[i] || !expected.fitlist[i]
                || actual.fitlist[i]->x != expected.fitlist[i]->x
                || actual.fitlist[i]->y != expected.fitlist[i]->y
                || actual.fitlist[i]->z != expected.fitlist[i]->z)
                return false;
        }
        return true;
    };
    class SplineExport final : public dx_iface {
    public:
        SplineExport(dxfRW& codec, DRW_Spline source)
            : output(codec), spline(std::move(source)) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
        }
        void writeEntities() override { emitted = output.writeSpline(&spline); }
        bool finalizeDxfWrite() override { return emitted; }
        dxfRW& output;
        dx_data storage;
        DRW_Spline spline;
        bool emitted = false;
    };
    const auto readbackMatches = [&](const std::filesystem::path& path,
                                     DRW::Version expectedVersion,
                                     bool fitOnly) {
        dx_data decoded;
        dx_iface reader;
        if (!reader.fileImport(path.string(), &decoded, false)
            || decoded.sourceVersion != expectedVersion
            || decoded.mBlock == nullptr || decoded.mBlock->ent.size() != 1u)
            return false;
        const auto* actual = dynamic_cast<const DRW_Spline*>(
            decoded.mBlock->ent.front());
        DRW_Spline expected = makeSpline(fitOnly);
        if (actual != nullptr)
            expected.handle = actual->handle;
        const bool matches = actual != nullptr && actual->handle != 0u
            && sameSpline(*actual, expected);
        if (!matches && actual != nullptr) {
            std::cerr << "SPLINE readback mismatch " << path << ": flags "
                      << actual->flags << '/' << expected.flags << ", counts "
                      << actual->knotslist.size() << '/'
                      << expected.knotslist.size() << ' '
                      << actual->controllist.size() << '/'
                      << expected.controllist.size() << ", weights "
                      << actual->weightlist.size() << '/'
                      << expected.weightlist.size() << ", tolerances "
                      << actual->tolknot << '/' << expected.tolknot << ' '
                      << actual->tolcontrol << '/' << expected.tolcontrol << ' '
                      << actual->tolfit << '/' << expected.tolfit << '\n';
        }
        return matches;
    };
    const std::array<std::pair<DRW::Version, const char*>, 7> versions = {{
        {DRW::AC1014, "AC1014"}, {DRW::AC1015, "AC1015"},
        {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
        {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
        {DRW::AC1032, "AC1032"}}};
    const std::string sentinel = "previous complete SPLINE destination\n";
    for (const auto& version : versions) {
      for (const bool fitOnly : {false, true}) {
        for (const bool binary : {false, true}) {
            const auto output = directory
                / (std::string("spline-") + version.second
                   + (fitOnly ? "-fit" : "-rational")
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            if (!clearStagedOutput(output))
                return fail("could not clear stale output staging files");
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                SplineExport adapter(codec, makeSpline(fitOnly));
                if (!codec.write(&adapter, version.first, binary)
                    || !adapter.emitted)
                    return fail("warm-up write failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            {
                dxfRW codec(output.string().c_str());
                SplineExport adapter(codec, makeSpline(fitOnly));
                armAllocationFailure(1000000000L);
                const bool wrote = codec.write(&adapter, version.first, binary);
                allocationCount = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
                const bool bytesRead = readBytes(output, expectedBytes);
                const bool readback = bytesRead
                    && readbackMatches(output, version.first, fitOnly);
                if (!wrote || !adapter.emitted || allocationFailed
                    || allocationCount <= 0 || !bytesRead || !readback) {
                    std::cerr << "SPLINE calibration " << version.second << ' '
                              << (binary ? "Binary" : "ASCII")
                              << ": write=" << wrote
                              << " emitted=" << adapter.emitted
                              << " allocationFailed=" << allocationFailed
                              << " attempts=" << allocationCount
                              << " bytes=" << bytesRead
                              << " readback=" << readback << '\n';
                    return fail("calibration write/readback failed");
                }
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior.good())
                        return fail("could not seed prior destination");
                }
                dxfRW codec(output.string().c_str());
                SplineExport adapter(codec, makeSpline(fitOnly));
                const DRW_Spline original(adapter.spline);
                armAllocationFailure(offset);
                const bool wrote = codec.write(&adapter, version.first, binary);
                const bool injected = allocationFailed;
                disarmAllocationFailure();
                std::string afterFailure;
                const bool bytesRead = readBytes(output, afterFailure);
                const auto& context = *codec.getWritingContext();
                const bool identitiesRolledBack =
                    context.sourceHandleToMintedMap.empty()
                    && context.sourceHandleToWrittenObjectMap.empty()
                    && context.writtenSourceEntityHandles.empty()
                    && context.ambiguousSourceHandles.empty();
                if (wrote || !injected || !bytesRead || afterFailure != sentinel
                    || !sameSpline(adapter.spline, original)
                    || !identitiesRolledBack || hasStagedOutput(output))
                    return fail("injected failure changed caller or destination state");
                const bool retried = codec.write(&adapter, version.first, binary)
                    && adapter.emitted;
                std::string retryBytes;
                if (!retried || !sameSpline(adapter.spline, original)
                    || !readBytes(output, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(output, version.first, fitOnly)
                    || hasStagedOutput(output))
                    return fail("same-writer retry or typed readback failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            std::cout << "SPLINE " << version.second << ' '
                      << (fitOnly ? "fit " : "rational ")
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", rollback/retry/readback=PASS\n";
        }
      }
    }
    return true;
}

bool runLightListSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "LIGHTLIST allocation transaction: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");

    const std::map<std::uint32_t, std::uint32_t> remap = {
        {0xD00u, 0xAD00u}, {0xA00u, 0xAA00u}, {0xA01u, 0xAA01u},
        {0xA02u, 0xAA02u}, {0xB00u, 0xAB00u}, {0xB01u, 0xAB01u}};
    const auto makeLightList = [] {
        DRW_LightList value;
        value.handle = 0xD00u;
        value.parentHandle = 0xA02u;
        value.m_classVersion = 1u;
        value.m_lightCount = 2u;
        value.m_lights = {{0xB00u, "key-light"}, {0xB01u, "fill-light"}};
        value.reactorHandles = {0xA01u};
        std::list<DRW_Variant> reactors;
        reactors.emplace_back(102, std::string("ACAD_REACTORS"));
        reactors.emplace_back(330, std::string("A00"));
        reactors.emplace_back(330, std::string("A01"));
        reactors.emplace_back(102, std::string("}"));
        value.appData.push_back(std::move(reactors));
        return value;
    };
    class LightListExport final : public dx_iface {
    public:
        LightListExport(dxfRW& codec, DRW_LightList source)
            : output(codec), list(std::move(source)) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
        }
        void writeObjects() override { written = output.writeLightList(&list); }
        bool finalizeDxfWrite() override { return written; }
        dxfRW& output;
        dx_data storage;
        DRW_LightList list;
        bool written = false;
    };
    class LightListReadback final : public dx_iface {
    public:
        LightListReadback() {
            cData = &storage;
            currentBlock = storage.mBlock;
        }
        void addLightList(const DRW_LightList& value) override {
            captured = value;
            found = true;
        }
        dx_data storage;
        DRW_LightList captured;
        bool found = false;
    };
    const auto readbackMatches = [](const std::filesystem::path& path,
                                    const char* expectedVersion,
                                    DRW::Version expectedSourceVersion,
                                    bool binary) {
        std::ifstream input(path, std::ios::binary);
        if (!input)
            return false;
        if (binary)
            input.seekg(22, std::ios::beg);
        std::unique_ptr<dxfReader> rawReader = binary
            ? std::unique_ptr<dxfReader>(
                  std::make_unique<dxfReaderBinary>(&input))
            : std::unique_ptr<dxfReader>(
                  std::make_unique<dxfReaderAscii>(&input));
        int code = 0;
        bool versionMatches = false;
        while (rawReader->readRec(&code)) {
            if (code == 9 && rawReader->getStringView() == "$ACADVER") {
                versionMatches = rawReader->readRec(&code) && code == 1
                    && rawReader->getStringView() == expectedVersion;
                break;
            }
        }
        if (!versionMatches)
            return false;

        const std::string pathString = path.string();
        dxfRW reader(pathString.c_str());
        LightListReadback probe;
        return reader.read(&probe, true)
            && reader.getSourceVersion() == expectedSourceVersion
            && probe.found
            && probe.captured.handle == 0xAD00u
            && probe.captured.parentHandle == 0xAA02u
            && probe.captured.m_classVersion == 1u
            && probe.captured.m_lightCount == 2u
            && probe.captured.reactorHandles == std::vector<std::uint32_t>{0xAA01u}
            && probe.captured.m_lights.size() == 2u
            && probe.captured.m_lights[0].m_handle == 0xAB00u
            && probe.captured.m_lights[0].m_name == "key-light"
            && probe.captured.m_lights[1].m_handle == 0xAB01u
            && probe.captured.m_lights[1].m_name == "fill-light";
    };
    const std::string sentinel = "previous complete document\n";
    const std::array<std::pair<DRW::Version, const char*>, 4> versions = {{
        {DRW::AC1021, "AC1021"}, {DRW::AC1024, "AC1024"},
        {DRW::AC1027, "AC1027"}, {DRW::AC1032, "AC1032"}}};
    for (const auto& version : versions) {
        for (const bool binary : {false, true}) {
            const auto output = directory
                / (std::string("light-list-allocation-") + version.second
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            if (!clearStagedOutput(output))
                return fail("could not clear stale output staging files in test scratch");
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                codec.setHandleRemap(remap);
                LightListExport adapter(codec, makeLightList());
                if (!codec.write(&adapter, version.first, binary))
                    return fail("warm-up write failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            {
                dxfRW codec(output.string().c_str());
                codec.setHandleRemap(remap);
                LightListExport adapter(codec, makeLightList());
                armAllocationFailure(1000000000L);
                const bool wrote = codec.write(&adapter, version.first, binary);
                allocationCount = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
                if (!wrote || allocationFailed || allocationCount <= 0
                    || !readBytes(output, expectedBytes))
                    return fail("calibration write failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }

            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior.good())
                        return fail("could not seed prior destination");
                }
                dxfRW codec(output.string().c_str());
                codec.setHandleRemap(remap);
                LightListExport adapter(codec, makeLightList());
                const DRW_LightList original(adapter.list);
                armAllocationFailure(offset);
                const bool wrote = codec.write(&adapter, version.first, binary);
                const bool injected = allocationFailed;
                disarmAllocationFailure();
                std::string afterFailure;
                const bool bytesRead = readBytes(output, afterFailure);
                const auto& context = *codec.getWritingContext();
                const bool identitiesRolledBack =
                    context.sourceHandleToMintedMap.empty()
                    && context.sourceHandleToWrittenObjectMap.empty()
                    && context.writtenSourceEntityHandles.empty()
                    && context.ambiguousSourceHandles.empty();
                const bool callerSame = adapter.list.handle == original.handle
                    && adapter.list.parentHandle == original.parentHandle
                    && adapter.list.m_lights.size() == original.m_lights.size()
                    && adapter.list.m_lights[0].m_handle == original.m_lights[0].m_handle
                    && adapter.list.m_lights[0].m_name == original.m_lights[0].m_name
                    && adapter.list.m_lights[1].m_handle == original.m_lights[1].m_handle
                    && adapter.list.m_lights[1].m_name == original.m_lights[1].m_name
                    && adapter.list.reactorHandles == original.reactorHandles
                    && adapter.list.m_classVersion == original.m_classVersion
                    && adapter.list.m_lightCount == original.m_lightCount;
                const bool stageRemains = hasStagedOutput(output);
                if (wrote || !injected || !bytesRead || afterFailure != sentinel
                    || !identitiesRolledBack || !callerSame || stageRemains) {
                    std::cerr << "offset=" << offset << " wrote=" << wrote
                              << " injected=" << injected << " bytesRead=" << bytesRead
                              << " destinationPreserved=" << (afterFailure == sentinel)
                              << " identitiesRolledBack=" << identitiesRolledBack
                              << " callerSame=" << callerSame
                              << " stageRemains=" << stageRemains << '\n';
                    return fail("injected allocation failure changed bytes or caller fields");
                }

                const bool retried = codec.write(&adapter, version.first, binary);
                std::string retryBytes;
                const bool retryRead = readBytes(output, retryBytes);
                const bool readback = retryRead
                    && readbackMatches(output, version.second, version.first,
                                       binary);
                const bool stagedAfterRetry = hasStagedOutput(output);
                if (!retried || !retryRead || retryBytes != expectedBytes
                    || !readback || stagedAfterRetry) {
                    std::cerr << "version=" << version.second << " binary="
                              << binary << " retried=" << retried
                              << " retryRead=" << retryRead
                              << " bytesMatch=" << (retryBytes == expectedBytes)
                              << " readback=" << readback
                              << " staged=" << stagedAfterRetry << '\n';
                    return fail("same-codec retry or bounded readback failed");
                }
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            std::cout << "LIGHTLIST " << version.second << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", caller/destination rollback, retry/readback=PASS\n";
        }
    }
    return true;
}

bool runSunStudySweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "SUNSTUDY allocation transaction: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const std::string sentinel = "previous complete document\n";
    const auto makeStudy = [] {
        DRW_SunStudy value;
        value.handle = 0xDA20u;
        value.parentHandle = 0xDA21u;
        value.m_classVersion = 1;
        value.m_setupName = "SUMMER_SOLSTICE";
        value.m_description = "Two dates, three hours";
        value.m_sheetSetName = "SOLAR_SET";
        value.m_sheetSubsetName = "NORTH";
        value.m_outputType = 0;
        value.m_useSubset = true;
        value.m_selectDatesFromCalendar = true;
        value.m_selectRangeOfDates = true;
        value.m_startTime = 100;
        value.m_endTime = 900;
        value.m_interval = 60;
        value.m_dates = {{2450000, 12345000}, {2450001, 86399000}};
        value.m_hours = {true, false, true};
        value.m_shadePlotType = 1;
        value.m_viewportCount = 2;
        value.m_rowCount = 1;
        value.m_columnCount = 2;
        value.m_spacing = 0.5;
        value.m_lockViewports = true;
        value.m_labelViewports = true;
        return value;
    };
    const auto sameStudy = [](const DRW_SunStudy& a, const DRW_SunStudy& b) {
        if (a.handle != b.handle || a.parentHandle != b.parentHandle
            || a.m_classVersion != b.m_classVersion
            || a.m_setupName != b.m_setupName
            || a.m_description != b.m_description
            || a.m_sheetSetName != b.m_sheetSetName
            || a.m_sheetSubsetName != b.m_sheetSubsetName
            || a.m_outputType != b.m_outputType || a.m_useSubset != b.m_useSubset
            || a.m_selectDatesFromCalendar != b.m_selectDatesFromCalendar
            || a.m_selectRangeOfDates != b.m_selectRangeOfDates
            || a.m_startTime != b.m_startTime || a.m_endTime != b.m_endTime
            || a.m_interval != b.m_interval || a.m_dates.size() != b.m_dates.size()
            || a.m_hours != b.m_hours || a.m_shadePlotType != b.m_shadePlotType
            || a.m_viewportCount != b.m_viewportCount || a.m_rowCount != b.m_rowCount
            || a.m_columnCount != b.m_columnCount || a.m_spacing != b.m_spacing
            || a.m_lockViewports != b.m_lockViewports
            || a.m_labelViewports != b.m_labelViewports)
            return false;
        for (std::size_t i = 0u; i < a.m_dates.size(); ++i) {
            if (a.m_dates[i].m_julianDay != b.m_dates[i].m_julianDay
                || a.m_dates[i].m_milliseconds != b.m_dates[i].m_milliseconds)
                return false;
        }
        return true;
    };
    class SunStudyExport final : public dx_iface {
    public:
        SunStudyExport(dxfRW& codec, DRW_SunStudy source)
            : output(codec), study(std::move(source)) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
        }
        void writeObjects() override { written = output.writeSunStudy(&study); }
        bool finalizeDxfWrite() override { return written; }
        dxfRW& output;
        dx_data storage;
        DRW_SunStudy study;
        bool written = false;
    };
    class SunStudyReadback final : public dx_iface {
    public:
        SunStudyReadback() {
            cData = &storage;
            currentBlock = storage.mBlock;
        }
        void addSunStudy(const DRW_SunStudy& value) override {
            study = value;
            found = true;
        }
        dx_data storage;
        DRW_SunStudy study;
        bool found = false;
    };
    const auto readbackMatches = [&](const std::filesystem::path& path,
                                     DRW::Version expectedVersion) {
        dxfRW reader(path.string().c_str());
        SunStudyReadback probe;
        if (!reader.read(&probe, true)
            || reader.getSourceVersion() != expectedVersion || !probe.found)
            return false;
        const DRW_SunStudy expected = makeStudy();
        return probe.study.handle != 0u
            && probe.study.parentHandle == expected.parentHandle
            && probe.study.m_classVersion == expected.m_classVersion
            && probe.study.m_setupName == expected.m_setupName
            && probe.study.m_description == expected.m_description
            && probe.study.m_sheetSetName == expected.m_sheetSetName
            && probe.study.m_sheetSubsetName == expected.m_sheetSubsetName
            && probe.study.m_outputType == expected.m_outputType
            && probe.study.m_useSubset == expected.m_useSubset
            && probe.study.m_selectDatesFromCalendar == expected.m_selectDatesFromCalendar
            && probe.study.m_selectRangeOfDates == expected.m_selectRangeOfDates
            && probe.study.m_startTime == expected.m_startTime
            && probe.study.m_endTime == expected.m_endTime
            && probe.study.m_interval == expected.m_interval
            && probe.study.m_dates.size() == expected.m_dates.size()
            && probe.study.m_dates[0].m_julianDay == expected.m_dates[0].m_julianDay
            && probe.study.m_dates[0].m_milliseconds == expected.m_dates[0].m_milliseconds
            && probe.study.m_dates[1].m_julianDay == expected.m_dates[1].m_julianDay
            && probe.study.m_dates[1].m_milliseconds == expected.m_dates[1].m_milliseconds
            && probe.study.m_hours == expected.m_hours
            && probe.study.m_shadePlotType == expected.m_shadePlotType
            && probe.study.m_viewportCount == expected.m_viewportCount
            && probe.study.m_rowCount == expected.m_rowCount
            && probe.study.m_columnCount == expected.m_columnCount
            && probe.study.m_spacing == expected.m_spacing
            && probe.study.m_lockViewports == expected.m_lockViewports
            && probe.study.m_labelViewports == expected.m_labelViewports;
    };

    const std::array<std::pair<DRW::Version, const char*>, 4> versions = {{
        {DRW::AC1021, "AC1021"}, {DRW::AC1024, "AC1024"},
        {DRW::AC1027, "AC1027"}, {DRW::AC1032, "AC1032"}}};
    for (const auto& version : versions) {
        const auto output = directory
            / (std::string("sun-study-allocation-") + version.second
               + ".dxf");
        if (!clearStagedOutput(output))
            return fail("could not clear stale output staging files in test scratch");
        for (const bool binary : {false, true}) {
            {
                dxfRW codec(output.string().c_str());
                SunStudyExport adapter(codec, makeStudy());
                if (!codec.write(&adapter, version.first, binary)) {
                    std::cerr << "warm-up binary=" << binary
                              << " objectWritten=" << adapter.written
                              << " error=" << codec.getError()
                              << " diagnostic=" << codec.getLastDiagnostic().code
                              << " message=" << codec.getLastDiagnostic().message
                              << '\n';
                    return fail("warm-up write failed");
                }
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            {
                std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior.good())
                    return fail("could not seed calibration destination");
            }
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                SunStudyExport adapter(codec, makeStudy());
                armAllocationFailure(1000000000L);
                const bool wrote = codec.write(&adapter, version.first, binary);
                allocationCount = static_cast<long>(allocationAttempts);
                disarmAllocationFailure();
                if (!wrote || allocationFailed || allocationCount <= 0
                    || !readBytes(output, expectedBytes))
                    return fail("calibration write failed");
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior.good())
                        return fail("could not seed prior destination");
                }
                dxfRW codec(output.string().c_str());
                SunStudyExport adapter(codec, makeStudy());
                const DRW_SunStudy original(adapter.study);
                armAllocationFailure(offset);
                const bool wrote = codec.write(&adapter, version.first, binary);
                const bool injected = allocationFailed;
                disarmAllocationFailure();
                std::string afterFailure;
                const bool bytesRead = readBytes(output, afterFailure);
                const auto& context = *codec.getWritingContext();
                const bool identitiesRolledBack =
                    context.sourceHandleToMintedMap.empty()
                    && context.sourceHandleToWrittenObjectMap.empty()
                    && context.writtenSourceEntityHandles.empty()
                    && context.ambiguousSourceHandles.empty();
                const bool stageRemains = hasStagedOutput(output);
                if (wrote || !injected || !bytesRead || afterFailure != sentinel
                    || !sameStudy(adapter.study, original) || !identitiesRolledBack
                    || stageRemains) {
                    std::cerr << "offset=" << offset << " wrote=" << wrote
                              << " injected=" << injected
                              << " destinationPreserved=" << (afterFailure == sentinel)
                              << " callerSame=" << sameStudy(adapter.study, original)
                              << " identitiesRolledBack=" << identitiesRolledBack
                              << " stageRemains=" << stageRemains << '\n';
                    return fail("injected allocation failure changed caller or destination state");
                }
                const bool retried = codec.write(&adapter, version.first, binary);
                std::string retryBytes;
                if (!retried || !sameStudy(adapter.study, original)
                    || !readBytes(output, retryBytes) || retryBytes != expectedBytes
                    || !readbackMatches(output, version.first) || hasStagedOutput(output))
                    return fail("same-codec retry or typed readback failed");
            }
            std::cout << "SUNSTUDY " << version.second << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", caller/destination rollback, retry/readback=PASS\n";
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
    }
    return true;
}

bool runSpatialFilterSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "SPATIAL_FILTER allocation transaction: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const auto output = directory / "spatial-filter-allocation-transaction.dxf";
    if (!clearStagedOutput(output))
        return fail("could not clear stale output staging files in test scratch");
    const std::string sentinel = "previous complete document\n";
    const auto makeFilter = [] {
        DRW_SpatialFilter value;
        value.handle = 0xDB00u;
        value.parentHandle = 0xDB01u;
        value.m_boundaryPoints = {{1.25, -2.5, 0.0}, {4.75, 6.5, 0.0},
                                  {8.0, 3.25, 0.0}};
        value.m_normal = {0.0, 0.0, 1.0};
        value.m_origin = {3.0, -4.0, 5.0};
        value.m_displayBoundary = true;
        value.m_clipFrontPlane = true;
        value.m_clipBackPlane = true;
        value.m_frontDistance = 12.5;
        value.m_backDistance = -7.25;
        for (std::size_t i = 0u; i < 12u; ++i) {
            value.m_inverseInsertTransform.push_back(100.0 + i);
            value.m_insertTransform.push_back(200.0 + i);
        }
        return value;
    };
    const auto sameFilter = [](const DRW_SpatialFilter& a,
                               const DRW_SpatialFilter& b) {
        if (a.handle != b.handle || a.parentHandle != b.parentHandle
            || a.m_boundaryPoints.size() != b.m_boundaryPoints.size()
            || a.m_normal.x != b.m_normal.x || a.m_normal.y != b.m_normal.y
            || a.m_normal.z != b.m_normal.z || a.m_origin.x != b.m_origin.x
            || a.m_origin.y != b.m_origin.y || a.m_origin.z != b.m_origin.z
            || a.m_displayBoundary != b.m_displayBoundary
            || a.m_clipFrontPlane != b.m_clipFrontPlane
            || a.m_clipBackPlane != b.m_clipBackPlane
            || a.m_frontDistance != b.m_frontDistance
            || a.m_backDistance != b.m_backDistance
            || a.m_inverseInsertTransform != b.m_inverseInsertTransform
            || a.m_insertTransform != b.m_insertTransform)
            return false;
        for (std::size_t i = 0u; i < a.m_boundaryPoints.size(); ++i) {
            if (a.m_boundaryPoints[i].x != b.m_boundaryPoints[i].x
                || a.m_boundaryPoints[i].y != b.m_boundaryPoints[i].y
                || a.m_boundaryPoints[i].z != b.m_boundaryPoints[i].z)
                return false;
        }
        return true;
    };
    class SpatialFilterExport final : public dx_iface {
    public:
        SpatialFilterExport(dxfRW& codec, DRW_SpatialFilter source)
            : output(codec), filter(std::move(source)) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
        }
        void writeObjects() override { written = output.writeSpatialFilter(&filter); }
        bool finalizeDxfWrite() override { return written; }
        dxfRW& output;
        dx_data storage;
        DRW_SpatialFilter filter;
        bool written = false;
    };
    class SpatialFilterReadback final : public dx_iface {
    public:
        SpatialFilterReadback() {
            cData = &storage;
            currentBlock = storage.mBlock;
        }
        void addSpatialFilter(const DRW_SpatialFilter& value) override {
            filter = value;
            found = true;
        }
        dx_data storage;
        DRW_SpatialFilter filter;
        bool found = false;
    };
    const auto readbackMatches = [&](const std::filesystem::path& path) {
        dxfRW reader(path.string().c_str());
        SpatialFilterReadback probe;
        return reader.read(&probe, true) && probe.found
            && sameFilter(probe.filter, makeFilter());
    };

    for (const bool binary : {false, true}) {
        {
            dxfRW codec(output.string().c_str());
            SpatialFilterExport adapter(codec, makeFilter());
            if (!codec.write(&adapter, DRW::AC1027, binary))
                return fail("warm-up write failed");
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
        {
            std::ofstream prior(output, std::ios::binary | std::ios::trunc);
            prior << sentinel;
            if (!prior.good())
                return fail("could not seed calibration destination");
        }
        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW codec(output.string().c_str());
            SpatialFilterExport adapter(codec, makeFilter());
            armAllocationFailure(1000000000L);
            const bool wrote = codec.write(&adapter, DRW::AC1027, binary);
            allocationCount = static_cast<long>(allocationAttempts);
            disarmAllocationFailure();
            if (!wrote || allocationFailed || allocationCount <= 0
                || !readBytes(output, expectedBytes))
                return fail("calibration write failed");
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior.good())
                    return fail("could not seed prior destination");
            }
            dxfRW codec(output.string().c_str());
            SpatialFilterExport adapter(codec, makeFilter());
            const DRW_SpatialFilter original(adapter.filter);
            armAllocationFailure(offset);
            const bool wrote = codec.write(&adapter, DRW::AC1027, binary);
            const bool injected = allocationFailed;
            disarmAllocationFailure();
            std::string afterFailure;
            const bool bytesRead = readBytes(output, afterFailure);
            const auto& context = *codec.getWritingContext();
            const bool identitiesRolledBack =
                context.sourceHandleToMintedMap.empty()
                && context.sourceHandleToWrittenObjectMap.empty()
                && context.writtenSourceEntityHandles.empty()
                && context.ambiguousSourceHandles.empty();
            const bool stageRemains = hasStagedOutput(output);
            if (wrote || !injected || !bytesRead || afterFailure != sentinel
                || !sameFilter(adapter.filter, original)
                || !identitiesRolledBack || stageRemains) {
                std::cerr << "offset=" << offset << " wrote=" << wrote
                          << " injected=" << injected
                          << " destinationPreserved=" << (afterFailure == sentinel)
                          << " callerSame=" << sameFilter(adapter.filter, original)
                          << " identitiesRolledBack=" << identitiesRolledBack
                          << " stageRemains=" << stageRemains << '\n';
                return fail("injected allocation failure changed caller or destination state");
            }
            const bool retried = codec.write(&adapter, DRW::AC1027, binary);
            std::string retryBytes;
            if (!retried || !sameFilter(adapter.filter, original)
                || !readBytes(output, retryBytes) || retryBytes != expectedBytes
                || !readbackMatches(output) || hasStagedOutput(output))
                return fail("same-codec retry or typed readback failed");
        }
        std::cout << "SPATIAL_FILTER " << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", caller/destination rollback, retry/readback=PASS\n";
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
    }
    return true;
}

bool runSectionManagerSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "SECTIONMANAGER allocation transaction: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const auto output = directory / "section-manager-allocation-transaction.dxf";
    if (!clearStagedOutput(output))
        return fail("could not clear stale output staging files in test scratch");
    const std::string sentinel = "previous complete document\n";
    const std::map<std::uint32_t, std::uint32_t> remap = {
        {0xDC00u, 0xEDC00u}, {0xDC01u, 0xEDC01u},
        {0xDC02u, 0xEDC02u}, {0xDC03u, 0xEDC03u},
        {0xDC04u, 0xEDC04u}};
    const auto makeManager = [] {
        DRW_Section value;
        value.handle = 0xDC00u;
        value.parentHandle = 0xDC01u;
        value.m_kind = DRW_Section::Manager;
        value.m_isLive = true;
        value.m_sectionCount = 3;
        value.m_sectionHandles = {0xDC02u, 0xDC03u, 0xDC04u};
        return value;
    };
    const auto sameManager = [](const DRW_Section& a, const DRW_Section& b) {
        return a.handle == b.handle && a.parentHandle == b.parentHandle
            && a.m_kind == b.m_kind && a.m_isLive == b.m_isLive
            && a.m_sectionCount == b.m_sectionCount
            && a.m_sectionHandles == b.m_sectionHandles;
    };
    class SectionManagerExport final : public dx_iface {
    public:
        SectionManagerExport(dxfRW& codec, DRW_Section source)
            : output(codec), manager(std::move(source)) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
        }
        void writeObjects() override {
            written = output.writeSection(&manager, "SECTIONMANAGER");
        }
        bool finalizeDxfWrite() override { return written; }
        dxfRW& output;
        dx_data storage;
        DRW_Section manager;
        bool written = false;
    };
    class SectionManagerReadback final : public dx_iface {
    public:
        SectionManagerReadback() {
            cData = &storage;
            currentBlock = storage.mBlock;
        }
        void addSection(const DRW_Section& value) override {
            manager = value;
            found = true;
        }
        dx_data storage;
        DRW_Section manager;
        bool found = false;
    };
    const auto readbackMatches = [&](const std::filesystem::path& path) {
        dxfRW reader(path.string().c_str());
        SectionManagerReadback probe;
        const DRW_Section expected = makeManager();
        return reader.read(&probe, true) && probe.found
            && probe.manager.handle == 0xEDC00u
            && probe.manager.parentHandle == 0xEDC01u
            && probe.manager.m_kind == DRW_Section::Manager
            && probe.manager.m_isLive == expected.m_isLive
            && probe.manager.m_sectionCount == expected.m_sectionCount
            && probe.manager.m_sectionHandles
                == std::vector<std::uint32_t>{0xEDC02u, 0xEDC03u, 0xEDC04u};
    };

    for (const bool binary : {false, true}) {
        {
            dxfRW codec(output.string().c_str());
            codec.setHandleRemap(remap);
            SectionManagerExport adapter(codec, makeManager());
            if (!codec.write(&adapter, DRW::AC1032, binary))
                return fail("warm-up write failed");
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
        {
            std::ofstream prior(output, std::ios::binary | std::ios::trunc);
            prior << sentinel;
            if (!prior.good())
                return fail("could not seed calibration destination");
        }
        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW codec(output.string().c_str());
            codec.setHandleRemap(remap);
            SectionManagerExport adapter(codec, makeManager());
            armAllocationFailure(1000000000L);
            const bool wrote = codec.write(&adapter, DRW::AC1032, binary);
            allocationCount = static_cast<long>(allocationAttempts);
            disarmAllocationFailure();
            if (!wrote || allocationFailed || allocationCount <= 0
                || !readBytes(output, expectedBytes))
                return fail("calibration write failed");
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior.good())
                    return fail("could not seed prior destination");
            }
            dxfRW codec(output.string().c_str());
            codec.setHandleRemap(remap);
            SectionManagerExport adapter(codec, makeManager());
            const DRW_Section original(adapter.manager);
            armAllocationFailure(offset);
            const bool wrote = codec.write(&adapter, DRW::AC1032, binary);
            const bool injected = allocationFailed;
            disarmAllocationFailure();
            std::string afterFailure;
            const bool bytesRead = readBytes(output, afterFailure);
            const auto& context = *codec.getWritingContext();
            const bool identitiesRolledBack =
                context.sourceHandleToMintedMap.empty()
                && context.sourceHandleToWrittenObjectMap.empty()
                && context.writtenSourceEntityHandles.empty()
                && context.ambiguousSourceHandles.empty();
            const bool stageRemains = hasStagedOutput(output);
            if (wrote || !injected || !bytesRead || afterFailure != sentinel
                || !sameManager(adapter.manager, original)
                || !identitiesRolledBack || stageRemains) {
                std::cerr << "offset=" << offset << " wrote=" << wrote
                          << " injected=" << injected
                          << " destinationPreserved=" << (afterFailure == sentinel)
                          << " callerSame=" << sameManager(adapter.manager, original)
                          << " identitiesRolledBack=" << identitiesRolledBack
                          << " stageRemains=" << stageRemains << '\n';
                return fail("injected allocation failure changed caller or destination state");
            }
            const bool retried = codec.write(&adapter, DRW::AC1032, binary);
            std::string retryBytes;
            if (!retried || !sameManager(adapter.manager, original)
                || !readBytes(output, retryBytes) || retryBytes != expectedBytes
                || !readbackMatches(output) || hasStagedOutput(output))
                return fail("same-codec retry or typed readback failed");
        }
        std::cout << "SECTIONMANAGER " << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", caller/destination rollback, retry/readback=PASS\n";
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
    }
    return true;
}

bool runSectionSettingsSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "SECTIONSETTINGS allocation transaction: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const auto output = directory / "section-settings-allocation-transaction.dxf";
    if (!clearStagedOutput(output))
        return fail("could not clear stale output staging files in test scratch");
    const std::string sentinel = "previous complete document\n";
    const std::map<std::uint32_t, std::uint32_t> remap = {
        {0xDD00u, 0xEDD00u}, {0xDD01u, 0xEDD01u},
        {0xDD02u, 0xEDD02u}, {0xDD03u, 0xEDD03u},
        {0xDD04u, 0xEDD04u}};
    const auto makeSettings = [] {
        DRW_Section value;
        value.handle = 0xDD00u;
        value.parentHandle = 0xDD01u;
        value.m_kind = DRW_Section::Settings;
        value.m_currentType = 1;
        value.m_typeCount = 1;
        DRW_SectionTypeSettings type;
        type.m_type = 2;
        type.m_generation = 3;
        type.m_numSources = 2;
        type.m_sourceHandles = {0xDD02u, 0xDD03u};
        type.m_destinationBlockHandle = 0xDD04u;
        type.m_destinationFile = "section-output.dwg";
        type.m_numGeometrySettings = 2;
        DRW_SectionGeometrySettings first;
        first.m_numGeometries = 4;
        first.m_hexIndex = 5;
        first.m_flags = 1;
        first.m_color = 42;
        first.m_layer = "A-WALL";
        first.m_lineType = "DASHED";
        first.m_lineTypeScale = 1.25;
        first.m_plotStyle = "Normal";
        first.m_lineWeight = 25;
        first.m_faceTransparency = 20;
        first.m_edgeTransparency = 30;
        first.m_hatchType = 1;
        first.m_hatchPattern = "ANSI31";
        first.m_hatchAngle = 0.25;
        first.m_hatchSpacing = 2.0;
        first.m_hatchScale = 0.75;
        DRW_SectionGeometrySettings second;
        second.m_numGeometries = 2;
        second.m_hexIndex = 9;
        second.m_flags = 2;
        second.m_color = 7;
        second.m_layer = "A-GLAZ";
        second.m_lineType = "CONTINUOUS";
        second.m_lineTypeScale = 0.5;
        second.m_plotStyle = "Hidden";
        second.m_lineWeight = 13;
        second.m_faceTransparency = 5;
        second.m_edgeTransparency = 10;
        second.m_hatchType = 2;
        second.m_hatchPattern = "ANSI32";
        second.m_hatchAngle = 0.5;
        second.m_hatchSpacing = 1.5;
        second.m_hatchScale = 1.25;
        type.m_geometry = {first, second};
        value.m_types.push_back(std::move(type));
        return value;
    };
    const auto sameSettings = [](const DRW_Section& a, const DRW_Section& b) {
        if (a.handle != b.handle || a.parentHandle != b.parentHandle
            || a.m_kind != b.m_kind || a.m_currentType != b.m_currentType
            || a.m_typeCount != b.m_typeCount || a.m_types.size() != b.m_types.size())
            return false;
        for (std::size_t i = 0; i < a.m_types.size(); ++i) {
            const DRW_SectionTypeSettings& x = a.m_types[i];
            const DRW_SectionTypeSettings& y = b.m_types[i];
            if (x.m_type != y.m_type || x.m_generation != y.m_generation
                || x.m_numSources != y.m_numSources
                || x.m_sourceHandles != y.m_sourceHandles
                || x.m_destinationBlockHandle != y.m_destinationBlockHandle
                || x.m_destinationFile != y.m_destinationFile
                || x.m_geometry.size() != y.m_geometry.size())
                return false;
            for (std::size_t j = 0; j < x.m_geometry.size(); ++j) {
                const DRW_SectionGeometrySettings& p = x.m_geometry[j];
                const DRW_SectionGeometrySettings& q = y.m_geometry[j];
                if (p.m_numGeometries != q.m_numGeometries
                    || p.m_hexIndex != q.m_hexIndex || p.m_flags != q.m_flags
                    || p.m_color != q.m_color || p.m_layer != q.m_layer
                    || p.m_lineType != q.m_lineType
                    || p.m_lineTypeScale != q.m_lineTypeScale
                    || p.m_plotStyle != q.m_plotStyle
                    || p.m_lineWeight != q.m_lineWeight
                    || p.m_faceTransparency != q.m_faceTransparency
                    || p.m_edgeTransparency != q.m_edgeTransparency
                    || p.m_hatchType != q.m_hatchType
                    || p.m_hatchPattern != q.m_hatchPattern
                    || p.m_hatchAngle != q.m_hatchAngle
                    || p.m_hatchSpacing != q.m_hatchSpacing
                    || p.m_hatchScale != q.m_hatchScale)
                    return false;
            }
        }
        return true;
    };
    class SectionSettingsExport final : public dx_iface {
    public:
        SectionSettingsExport(dxfRW& codec, DRW_Section source)
            : output(codec), settings(std::move(source)) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
        }
        void writeObjects() override {
            written = output.writeSection(&settings, "SECTIONSETTINGS");
        }
        bool finalizeDxfWrite() override { return written; }
        dxfRW& output;
        dx_data storage;
        DRW_Section settings;
        bool written = false;
    };
    class SectionSettingsReadback final : public dx_iface {
    public:
        SectionSettingsReadback() {
            cData = &storage;
            currentBlock = storage.mBlock;
        }
        void addSection(const DRW_Section& value) override {
            settings = value;
            found = true;
        }
        dx_data storage;
        DRW_Section settings;
        bool found = false;
    };
    const auto readbackMatches = [&](const std::filesystem::path& path) {
        dxfRW reader(path.string().c_str());
        SectionSettingsReadback probe;
        if (!reader.read(&probe, true) || !probe.found)
            return false;
        DRW_Section expected = makeSettings();
        expected.handle = 0xEDD00u;
        expected.parentHandle = 0xEDD01u;
        expected.m_types[0].m_sourceHandles = {0xEDD02u, 0xEDD03u};
        expected.m_types[0].m_destinationBlockHandle = 0xEDD04u;
        return sameSettings(probe.settings, expected);
    };

    for (const bool binary : {false, true}) {
        {
            dxfRW codec(output.string().c_str());
            codec.setHandleRemap(remap);
            SectionSettingsExport adapter(codec, makeSettings());
            if (!codec.write(&adapter, DRW::AC1032, binary))
                return fail("warm-up write failed");
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
        {
            std::ofstream prior(output, std::ios::binary | std::ios::trunc);
            prior << sentinel;
            if (!prior.good())
                return fail("could not seed calibration destination");
        }
        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW codec(output.string().c_str());
            codec.setHandleRemap(remap);
            SectionSettingsExport adapter(codec, makeSettings());
            armAllocationFailure(1000000000L);
            const bool wrote = codec.write(&adapter, DRW::AC1032, binary);
            allocationCount = static_cast<long>(allocationAttempts);
            disarmAllocationFailure();
            if (!wrote || allocationFailed || allocationCount <= 0
                || !readBytes(output, expectedBytes))
                return fail("calibration write failed");
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior.good())
                    return fail("could not seed prior destination");
            }
            dxfRW codec(output.string().c_str());
            codec.setHandleRemap(remap);
            SectionSettingsExport adapter(codec, makeSettings());
            const DRW_Section original(adapter.settings);
            armAllocationFailure(offset);
            const bool wrote = codec.write(&adapter, DRW::AC1032, binary);
            const bool injected = allocationFailed;
            disarmAllocationFailure();
            std::string afterFailure;
            const bool bytesRead = readBytes(output, afterFailure);
            const auto& context = *codec.getWritingContext();
            const bool identitiesRolledBack =
                context.sourceHandleToMintedMap.empty()
                && context.sourceHandleToWrittenObjectMap.empty()
                && context.writtenSourceEntityHandles.empty()
                && context.ambiguousSourceHandles.empty();
            const bool stageRemains = hasStagedOutput(output);
            if (wrote || !injected || !bytesRead || afterFailure != sentinel
                || !sameSettings(adapter.settings, original)
                || !identitiesRolledBack || stageRemains) {
                std::cerr << "offset=" << offset << " wrote=" << wrote
                          << " injected=" << injected
                          << " destinationPreserved=" << (afterFailure == sentinel)
                          << " callerSame=" << sameSettings(adapter.settings, original)
                          << " identitiesRolledBack=" << identitiesRolledBack
                          << " stageRemains=" << stageRemains << '\n';
                return fail("injected allocation failure changed caller or destination state");
            }
            const bool retried = codec.write(&adapter, DRW::AC1032, binary);
            std::string retryBytes;
            if (!retried || !sameSettings(adapter.settings, original)
                || !readBytes(output, retryBytes) || retryBytes != expectedBytes
                || !readbackMatches(output) || hasStagedOutput(output))
                return fail("same-codec retry or typed readback failed");
        }
        std::cout << "SECTIONSETTINGS " << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", caller/destination rollback, retry/readback=PASS\n";
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
    }
    return true;
}

bool runBlockRecordSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "BLOCK_RECORD allocation transaction: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const auto output = directory / "block-record-allocation-transaction.dxf";
    if (!clearStagedOutput(output))
        return fail("could not clear stale output staging files");
    const std::string sentinel = "previous complete document\n";
    const std::string blockName = "LOCAL_BLOCK_RECORD_PROFILE";
    const std::vector<std::uint8_t> preview = {
        0x89u, 0x50u, 0x4Eu, 0x47u, 0x0Du, 0x0Au, 0x1Au, 0x0Au,
        0x10u, 0x20u, 0x30u, 0x40u};
    const std::vector<std::uint32_t> insertHandles = {0xA100u, 0xA200u};
    class BlockRecordExport final : public dx_iface {
    public:
        BlockRecordExport(dxfRW& codec, const std::string& name,
                          const std::vector<std::uint8_t>& previewBytes,
                          const std::vector<std::uint32_t>& inserts,
                          unsigned writerOverload)
            : output(codec), blockName(name), previewData(previewBytes),
              sourceInsertHandles(inserts), overload(writerOverload),
              appId(1001, std::string("LOCAL_BLOCK_RECORD_APP")),
              xdataText(1000, std::string("block-record metadata")) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
            DRW_AppId appIdRecord;
            appIdRecord.name = "LOCAL_BLOCK_RECORD_APP";
            storage.appIds.push_back(appIdRecord);
            xdata = {&appId, &xdataText};
            for (std::size_t index = 0; index < inserts.size(); ++index) {
                DRW_Insert insert;
                insert.handle = inserts[index];
                insert.parentHandle = DRW::DxfModelSpaceBlockRecordHandle;
                insert.name = blockName;
                insert.basePoint = DRW_Coord{
                    12.0 + static_cast<double>(index) * 10.0,
                    34.0 + static_cast<double>(index) * 20.0, 0.0};
                this->inserts.push_back(std::move(insert));
            }
            block.handle = 0xA300u;
            block.name = blockName;
            block.basePoint = DRW_Coord{3.0, 4.0, 5.0};
            block.description = "generated block envelope";
            block.hasDescription = true;
            blockEnd.setIsEnd(true);
            blockEnd.layer = "LOCAL_BLOCK_FOOTER_LAYER";
            blockEnd.lineType = "DASHED";
            blockEnd.color = 5;
            blockEnd.visible = false;
            blockEnd.ltypeScale = 2.5;
            blockEnd.lWeight = DRW_LW_Conv::width09;
            blockEnd.transparency = static_cast<int>(0x03000040u);
        }
        void writeBlockRecords() override {
            switch (overload) {
            case 0u:
                blockRecordWritten = output.writeBlockRecord(blockName, 4);
                break;
            case 1u:
                blockRecordWritten = output.writeBlockRecord(
                    blockName, 4, previewData);
                break;
            case 2u:
                blockRecordWritten = output.writeBlockRecord(
                    blockName, 4, previewData, sourceInsertHandles);
                break;
            case 3u:
                blockRecordWritten = output.writeBlockRecord(
                    blockName, 4, previewData, sourceInsertHandles, xdata);
                break;
            default:
                blockRecordWritten = false;
                break;
            }
            if (!blockRecordWritten && !allocationTracking)
                std::cerr << "BLOCK_RECORD writer overload " << overload
                          << " rejected staged record\n";
        }
        void writeEntities() override {
            insertWritten = true;
            for (DRW_Insert& insert : inserts)
                insertWritten = output.writeInsert(&insert) && insertWritten;
            if (!insertWritten && !allocationTracking)
                std::cerr << "BLOCK_RECORD profile INSERT write failed\n";
        }
        void writeBlocks() override {
            blockBeginWritten = output.writeBlock(&block);
            blockEndWritten = blockBeginWritten
                && output.writeBlockEnd(&blockEnd);
            if ((!blockBeginWritten || !blockEndWritten)
                && !allocationTracking)
                std::cerr << "BLOCK_RECORD profile BLOCK envelope write failed\n";
        }
        bool finalizeDxfWrite() override {
            return blockRecordWritten && insertWritten
                && blockBeginWritten && blockEndWritten;
        }
        dxfRW& output;
        dx_data storage;
        std::string blockName;
        std::vector<std::uint8_t> previewData;
        std::vector<std::uint32_t> sourceInsertHandles;
        unsigned overload = 0u;
        DRW_Variant appId;
        DRW_Variant xdataText;
        std::vector<DRW_Variant*> xdata;
        std::vector<DRW_Insert> inserts;
        DRW_Block block;
        DRW_Block blockEnd;
        bool blockRecordWritten = false;
        bool insertWritten = false;
        bool blockBeginWritten = false;
        bool blockEndWritten = false;
    };
    class BlockRecordReadback final : public dx_iface {
    public:
        struct Record {
            std::uint32_t handle = DRW::NoHandle;
            std::int32_t insUnits = 0;
            std::string name;
            std::vector<std::uint8_t> previewData;
            std::vector<std::uint32_t> insertHandles;
            std::vector<std::pair<int, std::string>> xdata;
        };
        explicit BlockRecordReadback(std::string target)
            : targetName(std::move(target)) {
            cData = &storage;
            currentBlock = storage.mBlock;
        }
        void addBlockRecord(const DRW_Block_Record& value) override {
            if (value.name != targetName)
                return;
            Record record;
            record.handle = value.handle;
            record.insUnits = value.insUnits;
            record.name = value.name;
            record.previewData = value.previewData;
            record.insertHandles = value.insertHandles;
            for (const DRW_Variant* item : value.extData) {
                if (item == nullptr)
                    continue;
                if (item->type() == DRW_Variant::STRING
                    && item->content.s != nullptr)
                    record.xdata.emplace_back(item->code(), item->c_str());
                else
                    record.xdata.emplace_back(item->code(), std::string());
            }
            records.push_back(std::move(record));
        }
        void addInsert(const DRW_Insert& value) override {
            inserts.push_back(value);
        }
        void addBlock(const DRW_Block& value) override {
            blocks.push_back(value);
        }
        void addBlockEnd(const DRW_Block& value) override {
            blockFooters.push_back(value);
        }
        void endBlock() override { ++blockEnds; }
        dx_data storage;
        std::string targetName;
        std::vector<Record> records;
        std::vector<DRW_Insert> inserts;
        std::vector<DRW_Block> blocks;
        std::vector<DRW_Block> blockFooters;
        unsigned blockEnds = 0u;
    };
    const auto readbackMatches = [&](const std::filesystem::path& path,
                                     unsigned overload,
                                     DRW::Version version) {
        dxfRW reader(path.string().c_str());
        BlockRecordReadback probe(blockName);
        const bool read = reader.read(&probe, true);
        if (!read || probe.records.size() != 1u
            || probe.inserts.size() != insertHandles.size()) {
            std::cerr << "BLOCK_RECORD readback read=" << read
                      << " records=" << probe.records.size()
                      << " inserts=" << probe.inserts.size() << '\n';
            return false;
        }
        const auto block = std::find_if(probe.blocks.begin(), probe.blocks.end(),
            [&](const DRW_Block& value) {
                return value.name == blockName && !value.getIsEnd();
            });
        if (block == probe.blocks.end() || probe.blockEnds == 0u
            || block->handle == 0u || block->handle == DRW::NoHandle
            || block->basePoint.x != 3.0 || block->basePoint.y != 4.0
            || block->basePoint.z != 5.0
            || block->description != "generated block envelope"
            || !block->hasDescription)
            return false;
        const BlockRecordReadback::Record& record = probe.records.front();
        const auto footerIt = std::find_if(
            probe.blockFooters.begin(), probe.blockFooters.end(),
            [](const DRW_Block& value) {
                return value.layer == "LOCAL_BLOCK_FOOTER_LAYER";
            });
        if (footerIt == probe.blockFooters.end())
            return false;
        const DRW_Block& footer = *footerIt;
        const std::uint32_t expectedFooterOwner =
            version > DRW::AC1014 ? record.handle : DRW::NoHandle;
        if (!footer.getIsEnd() || footer.handle != record.handle + 2u
            || footer.handle == DRW::NoHandle
            || footer.parentHandle != expectedFooterOwner
            || footer.layer != "LOCAL_BLOCK_FOOTER_LAYER"
            || footer.lineType != "DASHED" || footer.color != 5
            || footer.visible || footer.ltypeScale != 2.5
            || footer.lWeight != (version > DRW::AC1014
                ? DRW_LW_Conv::width09 : DRW_LW_Conv::widthByLayer)
            || footer.transparency != (version > DRW::AC1015
                ? static_cast<int>(0x03000040u) : DRW::Opaque))
            return false;
        std::vector<std::uint32_t> emittedInsertHandles;
        emittedInsertHandles.reserve(probe.inserts.size());
        bool insertsMatch = true;
        for (std::size_t index = 0; index < probe.inserts.size(); ++index) {
            const DRW_Insert& inserted = probe.inserts[index];
            emittedInsertHandles.push_back(inserted.handle);
            insertsMatch = insertsMatch
                && inserted.handle != 0u && inserted.handle != DRW::NoHandle
                && inserted.name == blockName
                && inserted.basePoint.x == 12.0 + static_cast<double>(index) * 10.0
                && inserted.basePoint.y == 34.0 + static_cast<double>(index) * 20.0;
        }
        const bool matches = record.handle != 0u && record.handle != DRW::NoHandle
            && record.insUnits == (version > DRW::AC1018 ? 4 : 0)
            && record.name == blockName
            && record.previewData == (overload >= 1u
                ? preview : std::vector<std::uint8_t>{})
            && insertsMatch
            && record.insertHandles == (overload >= 2u
                ? emittedInsertHandles : std::vector<std::uint32_t>{})
            && record.xdata == (overload >= 3u
                ? std::vector<std::pair<int, std::string>>{
                    {1001, "LOCAL_BLOCK_RECORD_APP"},
                    {1000, "block-record metadata"}}
                : std::vector<std::pair<int, std::string>>{});
        if (!matches) {
            std::cerr << "BLOCK_RECORD mismatch handle=" << record.handle
                      << " insUnits=" << record.insUnits
                      << " name=" << record.name
                      << " preview=" << record.previewData.size()
                      << " refs=" << record.insertHandles.size();
            for (std::uint32_t handle : record.insertHandles)
                std::cerr << ' ' << handle;
            std::cerr << " xdata=" << record.xdata.size()
                      << " insert-count=" << probe.inserts.size();
            for (const DRW_Insert& inserted : probe.inserts)
                std::cerr << " [" << inserted.handle << ',' << inserted.name
                          << ',' << inserted.basePoint.x << ','
                          << inserted.basePoint.y << ']';
            std::cerr << '\n';
        }
        return matches;
    };
    const auto sameCaller = [&](const BlockRecordExport& adapter,
                                const std::vector<DRW_Insert>& original) {
        if (adapter.blockName != blockName || adapter.overload > 3u
            || adapter.previewData != preview
            || adapter.sourceInsertHandles != insertHandles
            || adapter.xdata.size() != 2u
            || adapter.xdata[0] != &adapter.appId
            || adapter.xdata[1] != &adapter.xdataText
            || adapter.appId.code() != 1001
            || !adapter.appId.stringEquals("LOCAL_BLOCK_RECORD_APP")
            || adapter.xdataText.code() != 1000
            || !adapter.xdataText.stringEquals("block-record metadata")
            || adapter.block.handle != 0xA300u
            || adapter.block.name != blockName
            || adapter.block.basePoint.x != 3.0
            || adapter.block.basePoint.y != 4.0
            || adapter.block.basePoint.z != 5.0
            || adapter.block.description != "generated block envelope"
            || !adapter.block.hasDescription
            || !adapter.blockEnd.getIsEnd()
            || adapter.blockEnd.layer != "LOCAL_BLOCK_FOOTER_LAYER"
            || adapter.blockEnd.lineType != "DASHED"
            || adapter.blockEnd.color != 5 || adapter.blockEnd.visible
            || adapter.blockEnd.ltypeScale != 2.5
            || adapter.blockEnd.lWeight != DRW_LW_Conv::width09
            || adapter.blockEnd.transparency != static_cast<int>(0x03000040u)
            || adapter.inserts.size() != original.size())
            return false;
        for (std::size_t index = 0; index < original.size(); ++index) {
            const DRW_Insert& current = adapter.inserts[index];
            const DRW_Insert& before = original[index];
            if (current.handle != before.handle
                || current.parentHandle != before.parentHandle
                || current.name != before.name
                || current.basePoint.x != before.basePoint.x
                || current.basePoint.y != before.basePoint.y
                || current.basePoint.z != before.basePoint.z)
                return false;
        }
        return true;
    };
    const std::vector<std::pair<DRW::Version, const char *>> versions = {
        {DRW::AC1014, "AC1014"}, {DRW::AC1015, "AC1015"},
        {DRW::AC1018, "AC1018"}, {DRW::AC1021, "AC1021"},
        {DRW::AC1024, "AC1024"}, {DRW::AC1027, "AC1027"},
        {DRW::AC1032, "AC1032"}};
    for (unsigned overload = 0u; overload < 4u; ++overload) {
      for (const auto &target : versions) {
        // Ordered INSERT handle references were introduced after AC1014.
        if (target.first == DRW::AC1014 && overload >= 2u)
          continue;
        for (const bool binary : {false, true}) {
          {
            dxfRW codec(output.string().c_str());
            BlockRecordExport adapter(codec, blockName, preview, insertHandles,
                                      overload);
            if (!codec.write(&adapter, target.first, binary)) {
              const DRW_OperationDiagnostic diagnostic =
                  codec.getLastDiagnostic();
              std::cerr << "BLOCK_RECORD warmup callbacks="
                        << adapter.blockRecordWritten << ','
                        << adapter.insertWritten
                        << " diagnostic=" << diagnostic.code << ": "
                        << diagnostic.message << '\n';
              return fail("warm-up write failed");
            }
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
          }
          {
            std::ofstream prior(output, std::ios::binary | std::ios::trunc);
            prior << sentinel;
            if (!prior.good())
              return fail("could not seed calibration destination");
          }
          long allocationCount = 0;
          std::string expectedBytes;
          {
            dxfRW codec(output.string().c_str());
            BlockRecordExport adapter(codec, blockName, preview, insertHandles,
                                      overload);
            armAllocationFailure(1000000000L);
            const bool wrote = codec.write(&adapter, target.first, binary);
            allocationCount = static_cast<long>(allocationAttempts);
            disarmAllocationFailure();
            if (!wrote || allocationFailed || allocationCount <= 0 ||
                !readBytes(output, expectedBytes))
              return fail("calibration write failed");
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
          }
          for (long offset = 0; offset < allocationCount; ++offset) {
            {
              std::ofstream prior(output, std::ios::binary | std::ios::trunc);
              prior << sentinel;
              if (!prior.good())
                return fail("could not seed prior destination");
            }
            dxfRW codec(output.string().c_str());
            BlockRecordExport adapter(codec, blockName, preview, insertHandles,
                                      overload);
            const std::vector<DRW_Insert> originalInserts = adapter.inserts;
            armAllocationFailure(offset);
            const bool wrote = codec.write(&adapter, target.first, binary);
            const bool injected = allocationFailed;
            disarmAllocationFailure();
            std::string afterFailure;
            const bool bytesRead = readBytes(output, afterFailure);
            const auto &context = *codec.getWritingContext();
            const bool identitiesRolledBack =
                context.sourceHandleToMintedMap.empty() &&
                context.sourceHandleToWrittenObjectMap.empty() &&
                context.writtenSourceEntityHandles.empty() &&
                context.ambiguousSourceHandles.empty();
            const bool stageRemains = hasStagedOutput(output);
            if (wrote || !injected || !bytesRead || afterFailure != sentinel ||
                !sameCaller(adapter, originalInserts) ||
                !identitiesRolledBack || stageRemains)
              return fail("injected failure changed caller/destination state");
            const bool retried = codec.write(&adapter, target.first, binary);
            std::string retryBytes;
            const bool retryRead = readBytes(output, retryBytes);
            const bool sameBytes = retryRead && retryBytes == expectedBytes;
            const bool readback =
                retryRead && readbackMatches(output, overload, target.first);
            const bool stageAfterRetry = hasStagedOutput(output);
            if (!retried || !retryRead || !sameBytes || !readback ||
                stageAfterRetry || !sameCaller(adapter, originalInserts)) {
              std::cerr << "BLOCK_RECORD retry=" << retried
                        << " read=" << retryRead << " sameBytes=" << sameBytes
                        << " readback=" << readback
                        << " stageRemains=" << stageAfterRetry << '\n';
              return fail("same-codec retry or typed readback failed");
            }
          }
          std::cout << "BLOCK_RECORD " << target.second << " overload "
                    << overload << ' ' << (binary ? "Binary" : "ASCII")
                    << ": allocation offsets=" << allocationCount
                    << ", caller/destination rollback, retry/readback=PASS\n";
          std::error_code ignored;
          std::filesystem::remove(output, ignored);
        }
      }
    }
    return true;
}

bool runBackgroundSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "BACKGROUND allocation transaction: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const auto output = directory / "background-allocation-transaction.dxf";
    if (!clearStagedOutput(output))
        return fail("could not clear stale output staging files");
    const std::string sentinel = "previous complete document\n";
    const auto makeBackgrounds = [] {
        std::vector<DRW_Background> result;
        DRW_Background solid;
        solid.parentHandle = 0xCu;
        solid.m_kind = DRW_Background::Solid;
        solid.m_classVersion = 1;
        solid.m_solidColor = 0x010203;
        result.push_back(solid);

        DRW_Background gradient;
        gradient.parentHandle = 0xCu;
        gradient.m_kind = DRW_Background::Gradient;
        gradient.m_classVersion = 1;
        gradient.m_colorTop = 0x101112;
        gradient.m_colorMiddle = 0x202122;
        gradient.m_colorBottom = 0x303132;
        gradient.m_horizon = 0.1;
        gradient.m_height = 0.2;
        gradient.m_rotation = 0.3;
        result.push_back(gradient);

        DRW_Background ground;
        ground.parentHandle = 0xCu;
        ground.m_kind = DRW_Background::GroundPlane;
        ground.m_classVersion = 1;
        ground.m_colorSkyZenith = 0x404142;
        ground.m_colorSkyHorizon = 0x505152;
        ground.m_colorUndergroundHorizon = 0x606162;
        ground.m_colorUndergroundAzimuth = 0x707172;
        ground.m_colorNear = 0x808182;
        ground.m_colorFar = 0x909192;
        result.push_back(ground);

        DRW_Background image;
        image.parentHandle = 0xCu;
        image.handle = 0xE300u;
        image.m_kind = DRW_Background::Image;
        image.m_classVersion = 1;
        image.m_fileName = "LOCAL_BACKGROUND_IMAGE.png";
        image.m_fitToScreen = true;
        image.m_maintainAspect = true;
        image.m_useTiling = false;
        image.m_offset = DRW_Coord{1.0, 2.0, 0.0};
        image.m_scale = DRW_Coord{3.0, 4.0, 0.0};
        result.push_back(image);

        DRW_Background ibl;
        ibl.parentHandle = 0xCu;
        ibl.handle = 0xE400u;
        ibl.m_kind = DRW_Background::Ibl;
        ibl.m_classVersion = 1;
        ibl.m_iblName = "LOCAL_IBL";
        ibl.m_enabled = true;
        ibl.m_displayImage = true;
        ibl.m_rotation = 0.4;
        ibl.m_secondaryBackgroundHandle = image.handle;
        result.push_back(ibl);

        DRW_Background skylight;
        skylight.parentHandle = 0xCu;
        skylight.handle = 0xE500u;
        skylight.m_kind = DRW_Background::Skylight;
        skylight.m_classVersion = 1;
        skylight.m_sunHandle = 0xE600u;
        result.push_back(skylight);
        return result;
    };
    const auto samePayload = [](const DRW_Background& a,
                                const DRW_Background& b) {
        return a.parentHandle == b.parentHandle && a.m_kind == b.m_kind
            && a.m_classVersion == b.m_classVersion
            && a.m_solidColor == b.m_solidColor
            && a.m_colorTop == b.m_colorTop
            && a.m_colorMiddle == b.m_colorMiddle
            && a.m_colorBottom == b.m_colorBottom
            && a.m_horizon == b.m_horizon && a.m_height == b.m_height
            && a.m_rotation == b.m_rotation
            && a.m_colorSkyZenith == b.m_colorSkyZenith
            && a.m_colorSkyHorizon == b.m_colorSkyHorizon
            && a.m_colorUndergroundHorizon == b.m_colorUndergroundHorizon
            && a.m_colorUndergroundAzimuth == b.m_colorUndergroundAzimuth
            && a.m_colorNear == b.m_colorNear && a.m_colorFar == b.m_colorFar
            && a.m_fileName == b.m_fileName
            && a.m_fitToScreen == b.m_fitToScreen
            && a.m_maintainAspect == b.m_maintainAspect
            && a.m_useTiling == b.m_useTiling
            && a.m_offset.x == b.m_offset.x && a.m_offset.y == b.m_offset.y
            && a.m_scale.x == b.m_scale.x && a.m_scale.y == b.m_scale.y
            && a.m_iblName == b.m_iblName && a.m_enabled == b.m_enabled
            && a.m_displayImage == b.m_displayImage
            && a.m_secondaryBackgroundHandle == b.m_secondaryBackgroundHandle
            && a.m_sunHandle == b.m_sunHandle;
    };
    class BackgroundExport final : public dx_iface {
    public:
        BackgroundExport(dxfRW& codec, std::vector<DRW_Background> source)
            : output(codec), backgrounds(std::move(source)) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
            output.setResolvedBackgroundHandles({0xE300u});
            output.setResolvedSunHandles({0xE600u});
            sun.handle = 0xE600u;
            sun.parentHandle = 0xCu;
            sun.m_classVersion = 1;
            sun.m_isOn = true;
            sun.m_color = 3u;
            sun.m_intensity = 0.85;
            sun.m_hasShadow = true;
            sun.m_julianDay = 2460600;
            sun.m_milliseconds = 43210000;
            sun.m_shadowType = 2u;
            sun.m_shadowMapSize = 512u;
            sun.m_shadowSoftness = 25u;
        }
        void writeObjects() override {
            written = output.writeSun(&sun);
            for (DRW_Background& value : backgrounds) {
                if (!written)
                    break;
                const char* name = recordName(value.m_kind);
                if (name == nullptr || !output.writeBackground(&value, name)) {
                    written = false;
                    break;
                }
            }
        }
        bool finalizeDxfWrite() override { return written; }
        static const char* recordName(DRW_Background::Kind kind) {
            switch (kind) {
            case DRW_Background::Solid: return "SOLIDBACKGROUND";
            case DRW_Background::Gradient: return "GRADIENTBACKGROUND";
            case DRW_Background::GroundPlane: return "GROUNDPLANEBACKGROUND";
            case DRW_Background::Image: return "IMAGEBACKGROUND";
            case DRW_Background::Ibl: return "IBLBACKGROUND";
            case DRW_Background::Skylight: return "SKYLIGHTBACKGROUND";
            }
            return nullptr;
        }
        dxfRW& output;
        dx_data storage;
        DRW_Sun sun;
        std::vector<DRW_Background> backgrounds;
        bool written = false;
    };
    class BackgroundReadback final : public dx_iface {
    public:
        BackgroundReadback() {
            cData = &storage;
            currentBlock = storage.mBlock;
        }
        void addBackground(const DRW_Background& value) override {
            backgrounds.push_back(value);
        }
        void addSun(const DRW_Sun& value) override { suns.push_back(value); }
        dx_data storage;
        std::vector<DRW_Background> backgrounds;
        std::vector<DRW_Sun> suns;
    };
    const auto readbackMatches = [&](const std::filesystem::path& path) {
        dxfRW reader(path.string().c_str());
        BackgroundReadback probe;
        const std::vector<DRW_Background> expected = makeBackgrounds();
        if (!reader.read(&probe, true)
            || probe.backgrounds.size() != expected.size()
            || probe.suns.size() != 1u)
            return false;
        for (std::size_t i = 0u; i < expected.size(); ++i) {
            if (!samePayload(probe.backgrounds[i], expected[i])
                || probe.backgrounds[i].handle == 0u
                || probe.backgrounds[i].handle == DRW::NoHandle)
                return false;
        }
        const DRW_Sun& actualSun = probe.suns.front();
        if (actualSun.parentHandle != 0xCu
            || actualSun.handle != 0xE600u
            || actualSun.m_classVersion != 1u || !actualSun.m_isOn
            || actualSun.m_color != 3u || actualSun.m_intensity != 0.85
            || !actualSun.m_hasShadow || actualSun.m_julianDay != 2460600
            || actualSun.m_milliseconds != 43210000
            || actualSun.m_shadowType != 2u
            || actualSun.m_shadowMapSize != 512u
            || actualSun.m_shadowSoftness != 25u)
            return false;
        if (probe.backgrounds[3].m_kind != DRW_Background::Image
            || probe.backgrounds[3].handle != 0xE300u
            || probe.backgrounds[4].m_kind != DRW_Background::Ibl
            || probe.backgrounds[4].m_secondaryBackgroundHandle
                != probe.backgrounds[3].handle
            || probe.backgrounds[5].m_kind != DRW_Background::Skylight
            || probe.backgrounds[5].m_sunHandle != actualSun.handle)
            return false;
        return true;
    };
    for (const bool binary : {false, true}) {
        {
            dxfRW codec(output.string().c_str());
            BackgroundExport adapter(codec, makeBackgrounds());
            if (!codec.write(&adapter, DRW::AC1032, binary))
                return fail("warm-up write failed");
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
        {
            std::ofstream prior(output, std::ios::binary | std::ios::trunc);
            prior << sentinel;
            if (!prior.good())
                return fail("could not seed calibration destination");
        }
        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW codec(output.string().c_str());
            BackgroundExport adapter(codec, makeBackgrounds());
            armAllocationFailure(1000000000L);
            const bool wrote = codec.write(&adapter, DRW::AC1032, binary);
            allocationCount = static_cast<long>(allocationAttempts);
            disarmAllocationFailure();
            if (!wrote || allocationFailed || allocationCount <= 0
                || !readBytes(output, expectedBytes))
                return fail("calibration write failed");
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior.good())
                    return fail("could not seed prior destination");
            }
            dxfRW codec(output.string().c_str());
            BackgroundExport adapter(codec, makeBackgrounds());
            const DRW_Sun originalSun = adapter.sun;
            const std::vector<DRW_Background> original(adapter.backgrounds);
            armAllocationFailure(offset);
            const bool wrote = codec.write(&adapter, DRW::AC1032, binary);
            const bool injected = allocationFailed;
            disarmAllocationFailure();
            std::string afterFailure;
            const bool bytesRead = readBytes(output, afterFailure);
            const auto& context = *codec.getWritingContext();
            const bool identitiesRolledBack =
                context.sourceHandleToMintedMap.empty()
                && context.sourceHandleToWrittenObjectMap.empty()
                && context.writtenSourceEntityHandles.empty()
                && context.ambiguousSourceHandles.empty();
            const bool stageRemains = hasStagedOutput(output);
            bool callerSame = adapter.backgrounds.size() == original.size();
            for (std::size_t i = 0u; callerSame && i < original.size(); ++i)
                callerSame = samePayload(adapter.backgrounds[i], original[i])
                    && adapter.backgrounds[i].handle == original[i].handle;
            callerSame = callerSame
                && adapter.sun.handle == originalSun.handle
                && adapter.sun.parentHandle == originalSun.parentHandle
                && adapter.sun.m_classVersion == originalSun.m_classVersion
                && adapter.sun.m_isOn == originalSun.m_isOn
                && adapter.sun.m_color == originalSun.m_color
                && adapter.sun.m_intensity == originalSun.m_intensity
                && adapter.sun.m_hasShadow == originalSun.m_hasShadow
                && adapter.sun.m_julianDay == originalSun.m_julianDay
                && adapter.sun.m_milliseconds == originalSun.m_milliseconds
                && adapter.sun.m_shadowType == originalSun.m_shadowType
                && adapter.sun.m_shadowMapSize == originalSun.m_shadowMapSize
                && adapter.sun.m_shadowSoftness == originalSun.m_shadowSoftness;
            if (wrote || !injected || !bytesRead || afterFailure != sentinel
                || !callerSame || !identitiesRolledBack || stageRemains)
                return fail("injected failure changed caller/destination state");
            const bool retried = codec.write(&adapter, DRW::AC1032, binary);
            std::string retryBytes;
            if (!retried || !readBytes(output, retryBytes)
                || retryBytes != expectedBytes || !readbackMatches(output)
                || hasStagedOutput(output))
                return fail("same-codec retry or typed readback failed");
        }
        std::cout << "BACKGROUND variants " << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", caller/destination rollback, retry/readback=PASS\n";
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
    }
    return true;
}

bool runRenderSettingsSweep(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "RENDERSETTINGS allocation transaction: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const auto output = directory / "render-settings-allocation-transaction.dxf";
    if (!clearStagedOutput(output))
        return fail("could not clear stale output staging files in test scratch");
    const std::string sentinel = "previous complete document\n";
    const auto makeSettings = [] {
        std::vector<DRW_RenderSettings> result;
        DRW_RenderSettings settings;
        settings.parentHandle = 0xCu;
        settings.m_kind = DRW_RenderSettings::Settings;
        settings.m_classVersion = 1;
        settings.m_name = "LOCAL_RENDERSETTINGS";
        settings.m_strings = {"LOCAL_RENDERSETTINGS", "", "LOCAL_RENDER_DESC"};
        settings.m_longs = {1, 2};
        settings.m_bools = {true, false, true, false};
        settings.m_description = "LOCAL_RENDER_DESC";
        settings.m_displayIndex = 2;
        settings.m_fogEnabled = true;
        settings.m_backfacesEnabled = true;
        result.push_back(settings);

        DRW_RenderSettings environment;
        environment.parentHandle = 0xCu;
        environment.m_kind = DRW_RenderSettings::Environment;
        environment.m_name = "LOCAL_RENDER_ENVIRONMENT";
        environment.m_strings = {"LOCAL_RENDER_ENVIRONMENT"};
        environment.m_environmentImageFilename = "LOCAL_RENDER_ENVIRONMENT";
        environment.m_bools = {true, false, true};
        environment.m_bytes = {10, 20, 30};
        environment.m_doubles = {0.1, 0.9, 2.0, 3.0};
        environment.m_fogEnabled = true;
        environment.m_environmentImageEnabled = true;
        environment.m_fogColorR = 10;
        environment.m_fogColorG = 20;
        environment.m_fogColorB = 30;
        environment.m_fogDensityNear = 0.1;
        environment.m_fogDensityFar = 0.9;
        environment.m_fogDistanceNear = 2.0;
        environment.m_fogDistanceFar = 3.0;
        result.push_back(environment);

        DRW_RenderSettings global;
        global.parentHandle = 0xCu;
        global.m_kind = DRW_RenderSettings::Global;
        global.m_classVersion = 1;
        global.m_name = "LOCAL_RENDER_GLOBAL";
        global.m_strings = {"LOCAL_RENDER_GLOBAL"};
        global.m_longs = {1, 7, 8};
        global.m_procedure = 7;
        global.m_destination = 8;
        result.push_back(global);

        DRW_RenderSettings entry;
        entry.parentHandle = 0xCu;
        entry.m_kind = DRW_RenderSettings::Entry;
        entry.m_classVersion = 1;
        entry.m_name = "LOCAL_RENDER_ENTRY";
        entry.m_strings = {"LOCAL_RENDER_ENTRY", "", ""};
        entry.m_longs = {1, 11, 12, 13, 14, 15, 16, 17};
        entry.m_shorts = {1, 2, 3, 4, 5, 6};
        entry.m_doubles = {1.5};
        result.push_back(entry);

        DRW_RenderSettings rapid;
        rapid.parentHandle = 0xCu;
        rapid.m_kind = DRW_RenderSettings::RapidRT;
        rapid.m_classVersion = 1;
        rapid.m_name = "LOCAL_RENDER_RAPIDRT";
        rapid.m_strings = {"LOCAL_RENDER_RAPIDRT", "", "LOCAL_RAPID_DESC"};
        rapid.m_longs = {1, 9, 2, 3, 4};
        rapid.m_bools = {true, false, true, false, true};
        rapid.m_shorts = {1, 2, 3};
        rapid.m_doubles = {0.25, 0.75};
        rapid.m_hasPredefined = true;
        rapid.m_hasPredefinedPresent = true;
        rapid.m_description = "LOCAL_RAPID_DESC";
        rapid.m_displayIndex = 9;
        rapid.m_fogEnabled = true;
        rapid.m_backfacesEnabled = true;
        result.push_back(rapid);

        DRW_RenderSettings mental;
        mental.parentHandle = 0xCu;
        mental.m_kind = DRW_RenderSettings::MentalRay;
        mental.m_classVersion = 1;
        mental.m_name = "LOCAL_RENDER_MENTALRAY";
        mental.m_strings = {"LOCAL_RENDER_MENTALRAY", "", "LOCAL_MENTAL_DESC",
                            "LOCAL_MENTAL_EXTRA"};
        mental.m_longs = {1, 2, 2, 4, 5, 6, 7, 8,
                          9, 10, 11, 12, 13, 14, 15, 16};
        mental.m_shorts = {1, 2, 3, 4, 5, 6, 7};
        mental.m_bools = {true, false, true, false, true, false, true, false,
                          true, false, true, false, true, false, true};
        mental.m_doubles = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6,
                            0.7, 0.8, 0.9, 1.0, 1.1, 1.2};
        mental.m_hasPredefined = true;
        mental.m_hasPredefinedPresent = true;
        mental.m_description = "LOCAL_MENTAL_DESC";
        mental.m_displayIndex = 2;
        mental.m_fogEnabled = true;
        mental.m_backfacesEnabled = true;
        result.push_back(mental);

        DRW_RenderSettings legacyMental;
        legacyMental.parentHandle = 0xCu;
        legacyMental.m_kind = DRW_RenderSettings::MentalRay;
        legacyMental.m_classVersion = 1;
        legacyMental.m_name = "LOCAL_MENTAL_LEGACY";
        legacyMental.m_strings = {"LOCAL_MENTAL_LEGACY", "",
                                  "LEGACY_UNPROJECTED_DESCRIPTION"};
        legacyMental.m_longs = {1, 0, 1, 4, 5, 6, 7, 8,
                                9, 10, 11, 12, 13, 14, 15, 16};
        legacyMental.m_shorts = {1, 2, 3, 4, 5, 6, 7};
        legacyMental.m_bools = {true, false, true, false, true, false,
                                true, false, true, false, true, false};
        legacyMental.m_doubles = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6,
                                  0.7, 0.8, 0.9, 1.0, 1.1};
        legacyMental.m_fogEnabled = true;
        legacyMental.m_backfacesEnabled = false;
        legacyMental.m_environmentImageEnabled = true;
        result.push_back(legacyMental);

        DRW_RenderSettings modernMental;
        modernMental.parentHandle = 0xCu;
        modernMental.m_kind = DRW_RenderSettings::MentalRay;
        modernMental.m_classVersion = 1;
        modernMental.m_name = "LOCAL_MENTAL_MODERN";
        modernMental.m_strings = {"LOCAL_MENTAL_MODERN", "LOCAL_MAP",
                                  "LOCAL_MODERN_DESC", "LOCAL_MODERN_EXTRA"};
        modernMental.m_environmentImageFilename = "LOCAL_MAP";
        modernMental.m_longs = {1, 3, 1, 4, 5, 6, 7, 8,
                                9, 10, 11, 12, 13, 14, 15, 16};
        modernMental.m_shorts = {1, 2, 3, 4, 5, 6, 7};
        modernMental.m_bools = {true, false, true, false, true, false, true,
                                false, true, false, true, false, true, false};
        modernMental.m_doubles = {0.1, 0.2, 0.3, 0.4, 0.5, 0.6,
                                  0.7, 0.8, 0.9, 1.0, 1.1};
        modernMental.m_hasPredefined = true;
        modernMental.m_hasPredefinedPresent = true;
        modernMental.m_description = "LOCAL_MODERN_DESC";
        modernMental.m_displayIndex = 3;
        modernMental.m_fogEnabled = true;
        modernMental.m_backfacesEnabled = true;
        result.push_back(modernMental);
        return result;
    };
    const auto samePayload = [](const DRW_RenderSettings& a,
                                const DRW_RenderSettings& b) {
        return a.parentHandle == b.parentHandle
            && a.m_kind == b.m_kind && a.m_classVersion == b.m_classVersion
            && a.m_name == b.m_name && a.m_strings == b.m_strings
            && a.m_longs == b.m_longs && a.m_bools == b.m_bools
            && a.m_shorts == b.m_shorts && a.m_bytes == b.m_bytes
            && a.m_doubles == b.m_doubles
            && a.m_hasPredefined == b.m_hasPredefined
            && a.m_hasPredefinedPresent == b.m_hasPredefinedPresent
            && a.m_fogEnabled == b.m_fogEnabled
            && a.m_fogBackgroundEnabled == b.m_fogBackgroundEnabled
            && a.m_backfacesEnabled == b.m_backfacesEnabled
            && a.m_environmentImageEnabled == b.m_environmentImageEnabled
            && a.m_environmentImageFilename == b.m_environmentImageFilename
            && a.m_description == b.m_description
            && a.m_displayIndex == b.m_displayIndex
            && a.m_fogColorR == b.m_fogColorR
            && a.m_fogColorG == b.m_fogColorG
            && a.m_fogColorB == b.m_fogColorB
            && a.m_fogDensityNear == b.m_fogDensityNear
            && a.m_fogDensityFar == b.m_fogDensityFar
            && a.m_fogDistanceNear == b.m_fogDistanceNear
            && a.m_fogDistanceFar == b.m_fogDistanceFar
            && a.m_procedure == b.m_procedure
            && a.m_destination == b.m_destination;
    };
    class RenderSettingsExport final : public dx_iface {
    public:
        RenderSettingsExport(dxfRW& codec, std::vector<DRW_RenderSettings> source)
            : output(codec), settings(std::move(source)) {
            cData = &storage;
            currentBlock = storage.mBlock;
            dxfW = &output;
        }
        void writeObjects() override {
            written = true;
            for (DRW_RenderSettings& value : settings) {
                const char* name = renderRecordName(value.m_kind);
                if (name == nullptr || !output.writeRenderSettings(&value, name)) {
                    if (!allocationTracking)
                        std::cerr << "RENDERSETTINGS write failed kind="
                                  << static_cast<int>(value.m_kind)
                                  << " name=" << value.m_name << '\n';
                    written = false;
                    break;
                }
            }
        }
        bool finalizeDxfWrite() override { return written; }
        static const char* renderRecordName(DRW_RenderSettings::Kind kind) {
            switch (kind) {
            case DRW_RenderSettings::Settings: return "RENDERSETTINGS";
            case DRW_RenderSettings::Global: return "RENDERGLOBAL";
            case DRW_RenderSettings::Environment: return "RENDERENVIRONMENT";
            case DRW_RenderSettings::Entry: return "RENDERENTRY";
            case DRW_RenderSettings::RapidRT: return "RAPIDRTRENDERSETTINGS";
            case DRW_RenderSettings::MentalRay: return "MENTALRAYRENDERSETTINGS";
            }
            return nullptr;
        }
        dxfRW& output;
        dx_data storage;
        std::vector<DRW_RenderSettings> settings;
        bool written = false;
    };
    class RenderSettingsReadback final : public dx_iface {
    public:
        RenderSettingsReadback() {
            cData = &storage;
            currentBlock = storage.mBlock;
        }
        void addRenderSettings(const DRW_RenderSettings& value) override {
            settings.push_back(value);
        }
        dx_data storage;
        std::vector<DRW_RenderSettings> settings;
    };
    const auto readbackMatches = [&](const std::filesystem::path& path) {
        dxfRW reader(path.string().c_str());
        RenderSettingsReadback probe;
        const std::vector<DRW_RenderSettings> expected = makeSettings();
        if (!reader.read(&probe, true) || probe.settings.size() != expected.size()) {
            std::cerr << "render readback count=" << probe.settings.size()
                      << " expected=" << expected.size() << '\n';
            return false;
        }
        for (std::size_t i = 0u; i < expected.size(); ++i) {
            if (!samePayload(probe.settings[i], expected[i])
                || probe.settings[i].handle == 0u
                || probe.settings[i].handle == DRW::NoHandle) {
                std::cerr << "render readback mismatch index=" << i
                          << " actualKind=" << static_cast<int>(probe.settings[i].m_kind)
                          << " expectedKind=" << static_cast<int>(expected[i].m_kind)
                          << " actualName=" << probe.settings[i].m_name
                          << " expectedName=" << expected[i].m_name
                          << " actualCounts=" << probe.settings[i].m_longs.size()
                          << ',' << probe.settings[i].m_strings.size() << ','
                          << probe.settings[i].m_bools.size() << ','
                          << probe.settings[i].m_shorts.size() << ','
                          << probe.settings[i].m_bytes.size() << ','
                          << probe.settings[i].m_doubles.size()
                          << " expectedCounts=" << expected[i].m_longs.size()
                          << ',' << expected[i].m_strings.size() << ','
                          << expected[i].m_bools.size() << ','
                          << expected[i].m_shorts.size() << ','
                          << expected[i].m_bytes.size() << ','
                          << expected[i].m_doubles.size() << '\n';
                std::cerr << "classVersion=" << probe.settings[i].m_classVersion
                          << '/' << expected[i].m_classVersion
                          << " hasPredefined=" << probe.settings[i].m_hasPredefined
                          << '/' << expected[i].m_hasPredefined
                          << " present=" << probe.settings[i].m_hasPredefinedPresent
                          << '/' << expected[i].m_hasPredefinedPresent
                          << " bytesEqual="
                          << (probe.settings[i].m_bytes == expected[i].m_bytes)
                          << " doublesEqual="
                          << (probe.settings[i].m_doubles == expected[i].m_doubles)
                          << " longsEqual="
                          << (probe.settings[i].m_longs == expected[i].m_longs)
                          << " stringsEqual="
                          << (probe.settings[i].m_strings == expected[i].m_strings)
                          << " boolsEqual="
                          << (probe.settings[i].m_bools == expected[i].m_bools)
                          << " shortsEqual="
                          << (probe.settings[i].m_shorts == expected[i].m_shorts)
                          << '\n';
                std::cerr << "named fog=" << probe.settings[i].m_fogEnabled
                          << '/' << expected[i].m_fogEnabled
                          << " bg=" << probe.settings[i].m_fogBackgroundEnabled
                          << '/' << expected[i].m_fogBackgroundEnabled
                          << " backfaces=" << probe.settings[i].m_backfacesEnabled
                          << '/' << expected[i].m_backfacesEnabled
                          << " image=" << probe.settings[i].m_environmentImageEnabled
                          << '/' << expected[i].m_environmentImageEnabled
                          << " filename=" << probe.settings[i].m_environmentImageFilename
                          << '/' << expected[i].m_environmentImageFilename
                          << " desc=" << probe.settings[i].m_description
                          << '/' << expected[i].m_description
                          << " display=" << probe.settings[i].m_displayIndex
                          << '/' << expected[i].m_displayIndex
                          << " rgb=" << probe.settings[i].m_fogColorR << ','
                          << probe.settings[i].m_fogColorG << ','
                          << probe.settings[i].m_fogColorB << '/'
                          << expected[i].m_fogColorR << ','
                          << expected[i].m_fogColorG << ','
                          << expected[i].m_fogColorB << " proc/dest="
                          << probe.settings[i].m_procedure << ','
                          << probe.settings[i].m_destination << '/'
                          << expected[i].m_procedure << ','
                          << expected[i].m_destination << '\n';
                return false;
            }
        }
        return true;
    };

    for (const bool binary : {false, true}) {
        {
            dxfRW codec(output.string().c_str());
            RenderSettingsExport adapter(codec, makeSettings());
            if (!codec.write(&adapter, DRW::AC1032, binary))
                return fail("warm-up write failed");
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
        {
            std::ofstream prior(output, std::ios::binary | std::ios::trunc);
            prior << sentinel;
            if (!prior.good())
                return fail("could not seed calibration destination");
        }
        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW codec(output.string().c_str());
            RenderSettingsExport adapter(codec, makeSettings());
            armAllocationFailure(1000000000L);
            const bool wrote = codec.write(&adapter, DRW::AC1032, binary);
            allocationCount = static_cast<long>(allocationAttempts);
            disarmAllocationFailure();
            if (!wrote || allocationFailed || allocationCount <= 0
                || !readBytes(output, expectedBytes))
                return fail("calibration write failed");
            std::error_code ignored;
            std::filesystem::remove(output, ignored);
        }
        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior.good())
                    return fail("could not seed prior destination");
            }
            dxfRW codec(output.string().c_str());
            RenderSettingsExport adapter(codec, makeSettings());
            const std::vector<DRW_RenderSettings> original(adapter.settings);
            armAllocationFailure(offset);
            const bool wrote = codec.write(&adapter, DRW::AC1032, binary);
            const bool injected = allocationFailed;
            disarmAllocationFailure();
            std::string afterFailure;
            const bool bytesRead = readBytes(output, afterFailure);
            const auto& context = *codec.getWritingContext();
            const bool identitiesRolledBack =
                context.sourceHandleToMintedMap.empty()
                && context.sourceHandleToWrittenObjectMap.empty()
                && context.writtenSourceEntityHandles.empty()
                && context.ambiguousSourceHandles.empty();
            const bool stageRemains = hasStagedOutput(output);
            bool callerSame = adapter.settings.size() == original.size();
            for (std::size_t i = 0u; callerSame && i < original.size(); ++i)
                callerSame = samePayload(adapter.settings[i], original[i])
                    && adapter.settings[i].handle == original[i].handle;
            if (wrote || !injected || !bytesRead || afterFailure != sentinel
                || !callerSame || !identitiesRolledBack || stageRemains) {
                std::cerr << "offset=" << offset << " wrote=" << wrote
                          << " injected=" << injected
                          << " destinationPreserved=" << (afterFailure == sentinel)
                          << " callerSame=" << callerSame
                          << " identitiesRolledBack=" << identitiesRolledBack
                          << " stageRemains=" << stageRemains << '\n';
                return fail("injected allocation failure changed caller or destination state");
            }
            const bool retried = codec.write(&adapter, DRW::AC1032, binary);
            std::string retryBytes;
            const bool retryRead = readBytes(output, retryBytes);
            const bool sameBytes = retryRead && retryBytes == expectedBytes;
            const bool readback = retryRead && readbackMatches(output);
            const bool staged = hasStagedOutput(output);
            if (!retried || !retryRead || !sameBytes || !readback || staged) {
                std::cerr << "retry=" << retried << " read=" << retryRead
                          << " sameBytes=" << sameBytes << " readback="
                          << readback << " stageRemains=" << staged << '\n';
                return fail("same-codec retry or typed readback failed");
            }
        }
        std::cout << "RENDERSETTINGS variants " << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", caller/destination rollback, retry/readback=PASS\n";
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
    }
    return true;
}

class ObjectPtrExport final : public dx_iface {
public:
    explicit ObjectPtrExport(dxfRW& output) : output(output) {
        cData = &storage;
        currentBlock = storage.mBlock;
        dxfW = &output;
        object.handle = 0xDA70u;
        object.parentHandle = 0xCu;
    }

    void writeObjects() override { written = output.writeObjectPtr(&object); }

    dxfRW& output;
    dx_data storage;
    DRW_ObjectPtr object;
    bool written = false;
};

class ObjectPtrCapture final : public dx_iface {
public:
    ObjectPtrCapture() {
        cData = &storage;
        currentBlock = storage.mBlock;
    }

    void addObjectPtr(const DRW_ObjectPtr& value) override {
        ++count;
        object = value;
    }

    dx_data storage;
    DRW_ObjectPtr object;
    unsigned count = 0u;
};

bool runObjectPtrSweep(const std::filesystem::path& directory) {
    const auto fail = [](DRW::Version version, bool binary,
                         const char* message) {
        std::cerr << "OBJECT_PTR " << static_cast<int>(version) << ' '
                  << (binary ? "Binary" : "ASCII") << ": " << message
                  << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory)) {
        std::cerr << "OBJECT_PTR allocation transaction: could not create scratch directory\n";
        return false;
    }

    // AC1018–AC1032 are covered by the multi-object route matrix; this
    // focused profile closes its missing earliest supported target only.
    const std::array<DRW::Version, 1> versions{{DRW::AC1015}};
    const std::string sentinel = "previous complete document\n";
    for (const DRW::Version version : versions) {
        for (const bool binary : {false, true}) {
            const std::string stem = "objectptr-"
                + std::to_string(static_cast<int>(version))
                + (binary ? "-binary.dxf" : "-ascii.dxf");
            const std::filesystem::path output = directory / stem;
            if (!clearStagedOutput(output))
                return fail(version, binary,
                            "could not clear output staging files");

            {
                dxfRW codec(output.string().c_str());
                ObjectPtrExport adapter(codec);
                if (!codec.write(&adapter, version, binary)
                    || !adapter.written) {
                    std::error_code ignored;
                    std::filesystem::remove(output, ignored);
                    return fail(version, binary, "warm-up write failed");
                }
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            {
                std::ofstream prior(output,
                                    std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior.good())
                    return fail(version, binary,
                                "could not seed calibration destination");
            }
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(output.string().c_str());
                ObjectPtrExport adapter(codec);
                armAllocationFailure(1000000000L);
                const bool wrote = codec.write(&adapter, version, binary);
                allocationCount = static_cast<long>(allocationAttempts);
                const bool injected = allocationFailed;
                disarmAllocationFailure();
                if (!wrote || !adapter.written || injected
                    || allocationCount <= 0
                    || !readBytes(output, expectedBytes)) {
                    std::error_code ignored;
                    std::filesystem::remove(output, ignored);
                    return fail(version, binary,
                                "calibration write failed");
                }
                ObjectPtrCapture capture;
                dxfRW reader(output.string().c_str());
                if (!reader.read(&capture, false) || capture.count != 1u
                    || capture.object.handle != adapter.object.handle
                    || capture.object.parentHandle
                        != adapter.object.parentHandle) {
                    std::error_code ignored;
                    std::filesystem::remove(output, ignored);
                    return fail(version, binary,
                                "calibration typed readback failed");
                }
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }

            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream prior(output,
                                        std::ios::binary | std::ios::trunc);
                    prior << sentinel;
                    if (!prior.good())
                        return fail(version, binary,
                                    "could not seed prior destination");
                }
                dxfRW codec(output.string().c_str());
                ObjectPtrExport adapter(codec);
                const DRW_ObjectPtr original = adapter.object;
                armAllocationFailure(offset);
                const bool wrote = codec.write(&adapter, version, binary);
                const bool injected = allocationFailed;
                disarmAllocationFailure();
                std::string afterFailure;
                const bool bytesRead = readBytes(output, afterFailure);
                const auto& context = *codec.getWritingContext();
                const bool identitiesRolledBack =
                    context.sourceHandleToMintedMap.empty()
                    && context.sourceHandleToWrittenObjectMap.empty()
                    && context.writtenSourceEntityHandles.empty()
                    && context.ambiguousSourceHandles.empty();
                const bool callerSame = adapter.object.handle == original.handle
                    && adapter.object.parentHandle == original.parentHandle
                    && adapter.object.reactorHandles
                        == original.reactorHandles
                    && adapter.object.extensionDictionaryFlag()
                        == original.extensionDictionaryFlag()
                    && adapter.object.xDictHandle == original.xDictHandle
                    && adapter.object.extData.empty();
                if (wrote || !injected || !bytesRead
                    || afterFailure != sentinel || !callerSame
                    || !identitiesRolledBack || hasStagedOutput(output)) {
                    std::cerr << "offset=" << offset << " wrote=" << wrote
                              << " callbackCompleted=" << adapter.written
                              << " injected=" << injected
                              << " callerSame=" << callerSame
                              << " identitiesRolledBack="
                              << identitiesRolledBack << '\n';
                    std::error_code ignored;
                    std::filesystem::remove(output, ignored);
                    return fail(version, binary,
                                "injected failure changed caller or destination "
                                "state");
                }

                const bool retried = codec.write(&adapter, version, binary);
                std::string retryBytes;
                const bool retryRead = readBytes(output, retryBytes);
                ObjectPtrCapture capture;
                dxfRW reader(output.string().c_str());
                const bool readback = reader.read(&capture, false)
                    && capture.count == 1u
                    && capture.object.handle == original.handle
                    && capture.object.parentHandle == original.parentHandle;
                if (!retried || !adapter.written || !retryRead
                    || retryBytes != expectedBytes || !readback
                    || hasStagedOutput(output)) {
                    std::cerr << "offset=" << offset << " retry=" << retried
                              << " bytesMatch="
                              << (retryRead && retryBytes == expectedBytes)
                              << " readback=" << readback << '\n';
                    std::error_code ignored;
                    std::filesystem::remove(output, ignored);
                    return fail(version, binary,
                                "same-writer retry or typed readback failed");
                }
                std::error_code ignored;
                std::filesystem::remove(output, ignored);
            }
            std::cout << "OBJECT_PTR " << static_cast<int>(version) << ' '
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << allocationCount
                      << ", caller/destination rollback, retry/readback=PASS\n";
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
    if (argc < 2 || argc > 3) {
        std::cerr << "usage: dxf_write_allocation_transaction_tests SCRATCH_DIR [r2018-embedded-attribute|legacy-compound-entities|polyline-topologies|dimension-versions|hatch-mpolygon-versions|long-mtext|camera|leader-versions|mleader-versions|light-entity|ole-frames|ray-xline|tolerance|helix|spline|light-list|sun-study|spatial-filter|section-manager|section-settings|block-record|background|render-settings|objectptr]\n";
        return 2;
    }
    const std::filesystem::path directory(argv[1]);
    if (argc == 3) {
        const std::string mode(argv[2]);
        if (mode == "r2018-embedded-attribute")
            return runEmbeddedAttributeMTextSweep(directory) ? 0 : 1;
        if (mode == "legacy-compound-entities")
            return runLegacyCompoundEntitiesSweep(directory) ? 0 : 1;
        if (mode == "polyline-topologies")
            return runPolylineTopologySweep(directory) ? 0 : 1;
        if (mode == "dimension-versions")
            return runDimensionVersionSweep(directory) ? 0 : 1;
        if (mode == "hatch-mpolygon-versions")
            return runHatchVersionTransactionSweep(directory) ? 0 : 1;
        if (mode == "long-mtext")
            return runLongMTextSweep(directory) ? 0 : 1;
        if (mode == "camera")
            return runCameraSweep(directory) ? 0 : 1;
        if (mode == "viewport-versions")
            return runViewportVersionSweep(directory) ? 0 : 1;
        if (mode == "mesh-versions")
            return runMeshVersionSweep(directory) ? 0 : 1;
        if (mode == "geo-marker-versions")
            return runGeoPositionMarkerVersionSweep(directory) ? 0 : 1;
        if (mode == "leader-versions")
            return runLeaderVersionSweep(directory) ? 0 : 1;
        if (mode == "mleader-versions")
            return runMLeaderVersionSweep(directory) ? 0 : 1;
        if (mode == "light-entity")
            return runLightEntitySweep(directory) ? 0 : 1;
        if (mode == "ole-frames")
            return runOleFrameVersionSweep(directory) ? 0 : 1;
        if (mode == "ray-xline")
            return runRayXlineVersionSweep(directory) ? 0 : 1;
        if (mode == "tolerance")
            return runToleranceVersionSweep(directory) ? 0 : 1;
        if (mode == "helix")
            return runHelixVersionSweep(directory) ? 0 : 1;
        if (mode == "spline")
            return runSplineVersionSweep(directory) ? 0 : 1;
        if (mode == "light-list")
            return runLightListSweep(directory) ? 0 : 1;
        if (mode == "sun-study")
            return runSunStudySweep(directory) ? 0 : 1;
        if (mode == "spatial-filter")
            return runSpatialFilterSweep(directory) ? 0 : 1;
        if (mode == "section-manager")
            return runSectionManagerSweep(directory) ? 0 : 1;
        if (mode == "section-settings")
            return runSectionSettingsSweep(directory) ? 0 : 1;
        if (mode == "block-record")
            return runBlockRecordSweep(directory) ? 0 : 1;
        if (mode == "background")
            return runBackgroundSweep(directory) ? 0 : 1;
        if (mode == "render-settings")
            return runRenderSettingsSweep(directory) ? 0 : 1;
        if (mode == "objectptr")
            return runObjectPtrSweep(directory) ? 0 : 1;
        return 2;
    }
    return run(directory) ? 0 : 1;
}
