// SPDX-License-Identifier: GPL-2.0-or-later
// Generated whole-operation DXF write allocation rollback/retry control.
#include "dx_iface.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
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
const std::string kGroupName = "WHOLE_OPERATION_ALLOCATION_GROUP";
const std::string kCustomDictionaryName = "WHOLE_OPERATION_CUSTOM";
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
    }

    void writeEntities() override {
        entitiesWritten = output.writeLine(&line)
            && output.writeCircle(&circle)
            && output.writeLWPolyline(&polyline)
            && output.writeImage(&image, "whole-operation-image.png", 7u,
                                 image.ref) != nullptr
            && output.writeMesh(&mesh)
            && output.writeSpline(&spline)
            && output.writeHatch(&hatch)
            && output.writeLeader(&leader)
            && output.writeMLine(&mline);
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
        return entitiesWritten && objectsWritten;
    }

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
                || !readback || !bytesRead) {
                std::cerr << "calibration wrote=" << wrote
                          << " entities=" << adapter.entitiesWritten
                          << " objects=" << adapter.objectsWritten
                          << " readback=" << readback
                          << " bytes=" << bytesRead
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
                          << '\n';
                return fail("failure published identities or changed destination/caller records");
            }

            const bool retried = codec.write(&adapter, DRW::AC1027, binary);
            std::string retryBytes;
            if (!retried || !adapter.entitiesWritten || !adapter.objectsWritten
                || !readBytes(output, retryBytes) || retryBytes != expectedBytes
                || !matchesReadback(output))
                return fail("same-codec retry did not reproduce clean output");
        }
        std::cout << "whole DXF write " << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", destination/caller rollback, retry/readback=PASS\n";
        std::error_code ignored;
        std::filesystem::remove(output, ignored);
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
    if (argc != 2) {
        std::cerr << "usage: dxf_write_allocation_transaction_tests SCRATCH_DIR\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
