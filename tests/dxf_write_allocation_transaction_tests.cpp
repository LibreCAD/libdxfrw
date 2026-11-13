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
        {{kCustomDictionaryName, "D914"}});
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
            && check("ARC", output.writeArc(&arc))
            && check("ELLIPSE", output.writeEllipse(&ellipse))
            && check("POINT", output.writePoint(&point))
            && check("3DLINE", output.writeLine(&line3d))
            && check("SOLID", output.writeLine(&solid))
            && check("TRACE", output.writeLine(&trace))
            && check("TEXT", output.writeText(&text))
            && check("MTEXT", output.writeMText(&mtext))
            && check("RAY", output.writeLine(&ray))
            && check("XLINE", output.writeLine(&xline));
    }

    void writeObjects() override {
        dx_iface::writeObjects();
        objectsWritten = output.writeMLineStyle(&mlineStyle)
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
        ++textCount;
        text = value;
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
    DRW_Mesh mesh;
    DRW_Spline spline;
    DRW_Hatch hatch;
    DRW_Leader leader;
    DRW_MLine mline;
    DRW_MLineStyle mlineStyle;
    DRW_Polyline polyline3;
    DRW_Insert insert;
    DRW_3Dface face;
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
    DRW_Attdef attdef;
    std::vector<DRW_RawDxfObject> objects;
    std::vector<DRW_DataLink> dataLinks;
    std::vector<DRW_Dictionary> dictionaries;
    std::vector<DRW_Group> groups;
    unsigned lineCount = 0u;
    unsigned circleCount = 0u;
    unsigned polylineCount = 0u;
    unsigned imageCount = 0u;
    unsigned meshCount = 0u;
    unsigned splineCount = 0u;
    unsigned hatchCount = 0u;
    unsigned leaderCount = 0u;
    unsigned mlineCount = 0u;
    unsigned mlineStyleCount = 0u;
    unsigned polyline3Count = 0u;
    unsigned insertCount = 0u;
    unsigned faceCount = 0u;
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

bool callerRecordsUnchanged(const Export& adapter) {
    return adapter.line.handle == 0xD900u
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
        [](const DRW_RawDxfObject& value) { return value.name == "XRECORD"; });
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
    const auto insertBlock = std::find_if(
        adapter.storage.blocks.begin(), adapter.storage.blocks.end(),
        [](const dx_ifaceBlock* value) {
            return value != nullptr && value->name == kInsertBlockName;
        });
    const DRW_ImageDef* imageDefinition = adapter.storage.findImageDefinition(
        adapter.storage.importGeneration, adapter.image.ref);
    if (!read || adapter.lineCount != 1u || adapter.circleCount != 1u
        || adapter.polylineCount != 1u
        || adapter.imageCount != 1u
        || adapter.meshCount != 1u
        || adapter.splineCount != 1u
        || adapter.hatchCount != 1u
        || adapter.leaderCount != 1u
        || adapter.mlineCount != 1u
        || adapter.mlineStyleCount != 1u
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
        || rawXrecord->groups[3].c_str() == nullptr
        || std::string(rawXrecord->groups[3].c_str())
            != "whole operation payload"
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
                  << adapter.objects.size();
        if (rawXrecord != adapter.objects.end()
            && rawXrecord->groups.size() > 3u
            && rawXrecord->groups[3].c_str() != nullptr)
            std::cerr << " rawPayload=" << rawXrecord->groups[3].c_str();
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
    const bool rootMatches = adapter.storage.hasNamedObjectDictionary
        && root.handle == 0xCu && root.m_entries.size() == 3u
        && groupRootEntry != root.m_entries.end()
        && imageRootEntry != root.m_entries.end()
        && customRootEntry != root.m_entries.end();
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
                      && value.m_entries.size() == 4u
                      && value.m_entries.front().m_name == kNestedXrecordName
                      && value.m_entries.front().m_handle == 0xD901u
                      && value.m_entries[1].m_name == "MAIN_DATALINK"
                      && value.m_entries[1].m_handle == kMainDataLinkHandle
                      && value.m_entries[2].m_name == "TARGET_ALPHA"
                      && value.m_entries[2].m_handle == kTargetAlphaHandle
                      && value.m_entries[3].m_name == "TARGET_BETA"
                      && value.m_entries[3].m_handle == kTargetBetaHandle;
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
            if (!injected || wrote || reached != static_cast<std::size_t>(offset + 1)) {
                std::cerr << "offset=" << offset << " attempts=" << reached
                          << " fired=" << injected << " wrote=" << wrote
                          << '\n';
                return fail("allocation refusal was not atomic or missed its calibrated offset");
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
                || !retryReadback) {
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
    const auto publicationTarget = directory / "publication-race-target.dxf";
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
