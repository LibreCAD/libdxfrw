// SPDX-License-Identifier: GPL-2.0-or-later
// Generated rooted AC1024 geometry-dependency graph allocation rollback/retry.
#include "dx_iface.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <new>
#include <string>
#include <vector>

namespace {
long allocationsBeforeFailure = -1;
std::size_t failedAllocations = 0;

struct AllocationFailure {
    explicit AllocationFailure(long count) { allocationsBeforeFailure = count; }
    ~AllocationFailure() { allocationsBeforeFailure = -1; }
};

dx_data makeSource() {
    dx_data source;
    source.sourceFormat = dx_source_format::DWG;
    source.sourceVersion = DRW::AC1024;
    source.importGeneration = 1u;
    source.hasNamedObjectDictionary = true;
    source.namedObjectDictionary.handle = 0x900u;
    source.namedObjectDictionary.name = "NOD";
    source.namedObjectDictionary.m_entries.push_back(
        {"ACAD_ASSOCNETWORK", 0x901u});

    DRW_Dictionary associationDictionary;
    associationDictionary.handle = 0x901u;
    associationDictionary.parentHandle = 0x900u;
    associationDictionary.m_entries.push_back(
        {"ACAD_ASSOCNETWORK", 0x902u});
    source.dictionaries.push_back(std::move(associationDictionary));

    auto* line = new DRW_Line();
    line->handle = 0x905u;
    line->layer = "0";
    line->basePoint = DRW_Coord(1.0, 2.0, 0.0);
    line->secPoint = DRW_Coord(3.0, 4.0, 0.0);
    line->reactorHandles = {0x904u, 0x906u, 0x908u, 0x909u};
    source.mBlock->ent.push_back(line);

    DRW_AssociativeObject network("ACDBASSOCNETWORK");
    network.handle = 0x902u;
    network.parentHandle = 0x901u;
    network.reactorHandles.push_back(0x901u);
    network.m_classVersion = 1u;
    network.m_networkFieldsParsed = true;
    network.m_networkVersion = 0;
    network.m_networkActionIndex = 1;
    network.m_actions.push_back({true, 0x903u});
    network.m_prefixStatuses.push_back({
        DRW_AssociativePrefixStatus::Kind::AcDbAssocAction,
        0u, 0u, 1u, DRW_AssociativePrefixStatus::ParseStatus::Complete,
        2u, 0u, 0, "authored root-network action profile"});
    network.m_prefixStatuses.push_back({
        DRW_AssociativePrefixStatus::Kind::AcDbAssocNetwork,
        0u, 0u, 1u, DRW_AssociativePrefixStatus::ParseStatus::Complete,
        1u, 0u, 1, "authored root-network group membership"});

    DRW_AssociativeObject group("ACDBASSOC2DCONSTRAINTGROUP");
    group.handle = 0x903u;
    group.parentHandle = network.handle;
    group.m_classVersion = 1u;
    group.m_maxDependencyIndex = 143;
    group.m_owningNetworkHandle = network.handle;
    group.m_constraintGroupBodyParsed = true;
    group.m_constraintGroupVersion = 0;
    group.m_constraintActionsParsed = true;
    group.m_constraintActions = {0x904u, 0x906u, 0x908u, 0x909u};
    group.m_constraintActionCount = 4;
    group.m_constraintNextNodeIdRead = true;
    group.m_constraintNextNodeId = 9;
    group.m_constraintNodesParsed = true;
    group.m_constraintNodeCount = 8;
    DRW_AssociativeConstraintNode node;
    node.m_className = "AcConstrainedImplicitPoint";
    node.m_nodeId = 1;
    node.m_dataKind =
        DRW_AssociativeConstraintNode::DataKind::ImplicitPoint;
    node.m_hasGeometryDependency = true;
    node.m_geometryDependencyCode = DRW::DwgSoftPointer;
    node.m_geometryDependencyHandle = 0x904u;
    node.m_geometryNodeId = 7;
    node.m_classDataParsed = true;
    group.m_constraintNodes.push_back(node);
    node.m_nodeId = 2;
    node.m_className = "AcFixedConstraint";
    node.m_dataKind =
        DRW_AssociativeConstraintNode::DataKind::Geometrical;
    node.m_hasGeometryDependency = false;
    node.m_geometryDependencyHandle = 0u;
    node.m_geometryNodeId = 0;
    group.m_constraintNodes.push_back(node);
    node = DRW_AssociativeConstraintNode();
    node.m_className = "AcHorizontalConstraint";
    node.m_nodeId = 3;
    node.m_dataKind =
        DRW_AssociativeConstraintNode::DataKind::Horizontal;
    node.m_hasDatumLineIndex = true;
    node.m_datumLineIndex = 12;
    node.m_classDataParsed = true;
    group.m_constraintNodes.push_back(node);
    node = DRW_AssociativeConstraintNode();
    node.m_className = "AcConstrainedDatumLine";
    node.m_nodeId = 5;
    node.m_dataKind = DRW_AssociativeConstraintNode::DataKind::Line;
    node.m_hasGeometryDependency = true;
    node.m_geometryDependencyCode = DRW::DwgSoftPointer;
    node.m_geometryDependencyHandle = 0u;
    node.m_hasPoint = true;
    node.m_point = DRW_Coord(1.0, 2.0, 3.0);
    node.m_direction = DRW_Coord(0.0, 1.0, 0.0);
    node.m_classDataParsed = true;
    group.m_constraintNodes.push_back(node);
    node = DRW_AssociativeConstraintNode();
    node.m_className = "AcConstrainedBoundedLine";
    node.m_nodeId = 6;
    node.m_dataKind = DRW_AssociativeConstraintNode::DataKind::BoundedLine;
    node.m_hasGeometryDependency = true;
    node.m_geometryDependencyCode = DRW::DwgSoftPointer;
    node.m_geometryDependencyHandle = 0x908u;
    node.m_geometryNodeId = 10;
    node.m_hasPoint = true;
    node.m_point = DRW_Coord(1.0, 1.0, 1.0);
    node.m_direction = DRW_Coord(0.0, 1.0, 0.0);
    node.m_isRay = true;
    node.m_startPoint = DRW_Coord(2.0, 2.0, 2.0);
    node.m_endPoint = DRW_Coord(3.0, 3.0, 3.0);
    node.m_classDataParsed = true;
    group.m_constraintNodes.push_back(node);
    node = DRW_AssociativeConstraintNode();
    node.m_className = "AcConstrainedArc";
    node.m_nodeId = 7;
    node.m_dataKind = DRW_AssociativeConstraintNode::DataKind::Arc;
    node.m_hasGeometryDependency = true;
    node.m_geometryDependencyCode = DRW::DwgSoftPointer;
    node.m_geometryDependencyHandle = 0x909u;
    node.m_geometryNodeId = 11;
    node.m_center = DRW_Coord(1.0, 2.0, 3.0);
    node.m_normal = DRW_Coord(0.0, 0.0, 1.0);
    node.m_direction = DRW_Coord(1.0, 0.0, 0.0);
    node.m_radius = 5.0;
    node.m_startParameter = 0.5;
    node.m_endParameter = 1.0;
    node.m_startPoint = DRW_Coord(2.0, 3.0, 4.0);
    node.m_endPoint = DRW_Coord(5.0, 6.0, 7.0);
    node.m_classDataParsed = true;
    group.m_constraintNodes.push_back(node);
    node = DRW_AssociativeConstraintNode();
    node.m_className = "AcConstrainedCircle";
    node.m_nodeId = 4;
    node.m_dataKind = DRW_AssociativeConstraintNode::DataKind::Circle;
    node.m_hasGeometryDependency = true;
    node.m_geometryDependencyCode = DRW::DwgSoftPointer;
    node.m_geometryDependencyHandle = 0x906u;
    node.m_geometryNodeId = 8;
    node.m_center = DRW_Coord(1.0, 2.0, 3.0);
    node.m_normal = DRW_Coord(0.0, 0.0, 1.0);
    node.m_direction = DRW_Coord(1.0, 0.0, 0.0);
    node.m_radius = 4.0;
    node.m_startParameter = 0.25;
    node.m_endParameter = 0.75;
    node.m_classDataParsed = true;
    group.m_constraintNodes.push_back(node);
    node = DRW_AssociativeConstraintNode();
    node.m_className = "AcConstrainedRigidSet";
    node.m_nodeId = 8;
    node.m_dataKind = DRW_AssociativeConstraintNode::DataKind::RigidSet;
    node.m_hasGeometryDependency = true;
    node.m_geometryDependencyCode = DRW::DwgSoftPointer;
    node.m_geometryDependencyHandle = 0u;
    node.m_hasRigidSetReservedFlag = true;
    node.m_rigidSetReservedFlag = true;
    node.m_hasRigidSetTransform = true;
    node.m_rigidSetTransform = {{1.0, 0.0, 0.0, 0.0,
                                 0.0, 1.0, 0.0, 0.0,
                                 0.0, 0.0, 1.0, 0.0,
                                 0.0, 0.0, 0.0, 1.0}};
    node.m_rigidSetGeometryIds = {2, 3};
    node.m_classDataParsed = true;
    group.m_constraintNodes.push_back(node);
    std::sort(group.m_constraintNodes.begin(), group.m_constraintNodes.end(),
        [](const DRW_AssociativeConstraintNode& left,
           const DRW_AssociativeConstraintNode& right) {
            return left.m_nodeId < right.m_nodeId;
        });
    group.m_prefixStatuses.push_back({
        DRW_AssociativePrefixStatus::Kind::AcDbAssocAction,
        0u, 0u, 1u, DRW_AssociativePrefixStatus::ParseStatus::Complete,
        2u, 0u, 0, "authored constraint-group action profile"});
    group.m_prefixStatuses.push_back({
        DRW_AssociativePrefixStatus::Kind::AcDbAssoc2dConstraintGroup,
        0u, 0u, 1u, DRW_AssociativePrefixStatus::ParseStatus::Complete,
        10u, 8u, 8, "authored eight-node inline constraint profile"});

    DRW_AssociativeObject dependency("ACDBASSOCGEOMDEPENDENCY");
    dependency.handle = 0x904u;
    dependency.parentHandle = group.handle;
    dependency.m_classVersion = 1u;
    dependency.m_dependencyFieldsParsed = true;
    dependency.m_dependencyWireComplete = true;
    dependency.m_dependencyIsRead = true;
    dependency.m_dependencyIsAttachedToObject = true;
    dependency.m_dependencyIsDelegatingToOwningAction = true;
    dependency.m_dependencyOrder = -10000;
    dependency.m_dependencyHandle = line->handle;
    dependency.m_geomDependencyFieldsParsed = true;
    dependency.m_geomClassVersion = 0;
    dependency.m_geomEnabled = true;
    dependency.m_geomPersistentSubentityClassName =
        "AcDbAssocEdgePersSubentId";
    dependency.m_geomPersistentSubentityFieldsParsed = true;
    dependency.m_geomPersistentSubentityClassVersion = 0;
    dependency.m_geomPersistentSubentityIndex1 = 201;
    dependency.m_geomPersistentSubentityIndex2 = 202;
    dependency.m_prefixStatuses.push_back({
        DRW_AssociativePrefixStatus::Kind::AcDbAssocDependency,
        0u, 0u, 1u, DRW_AssociativePrefixStatus::ParseStatus::Complete,
        4u, 0u, 0, "authored geometry-dependency prefix"});
    dependency.m_prefixStatuses.push_back({
        DRW_AssociativePrefixStatus::Kind::AcDbAssocGeomDependency,
        0u, 0u, 0u, DRW_AssociativePrefixStatus::ParseStatus::Complete,
        0u, 4u, 0, "authored Edge subentity profile"});

    DRW_AssociativeObject circleDependency = dependency;
    circleDependency.handle = 0x906u;
    circleDependency.parentHandle = group.handle;
    circleDependency.m_dependencyHandle = line->handle;
    DRW_AssociativeObject boundedLineDependency = dependency;
    boundedLineDependency.handle = 0x908u;
    DRW_AssociativeObject arcDependency = dependency;
    arcDependency.handle = 0x909u;
    source.dwgAssociativeObjects = {network, group, dependency,
                                    circleDependency, boundedLineDependency,
                                    arcDependency};
    dx_associativeDwgImportReceipt& receipt = source.associativeDwgImport;
    receipt.generation = source.importGeneration;
    receipt.sourceVersion = source.sourceVersion;
    receipt.typedRange.begin = 0u;
    receipt.typedRange.end = source.dwgAssociativeObjects.size();
    receipt.typedCallbacks = source.dwgAssociativeObjects.size();
    receipt.successful = true;
    receipt.captureComplete = true;
    return source;
}

class GeometryGraphExport final : public dx_iface {
public:
    long failAt = -1;
    long reached = 0;

    void writeObjects() override {
        if (failAt < 0) {
            dx_iface::writeObjects();
            return;
        }
        const auto before = failedAllocations;
        try {
            AllocationFailure failure(failAt);
            dx_iface::writeObjects();
            reached = failedAllocations != before
                ? static_cast<long>(failedAllocations - before)
                : failAt - allocationsBeforeFailure;
        } catch (const std::bad_alloc&) {
            reached = failedAllocations != before
                ? static_cast<long>(failedAllocations - before)
                : failAt - allocationsBeforeFailure;
            throw;
        }
    }
};

bool readAll(const std::filesystem::path& path, std::string& value) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    value.assign(std::istreambuf_iterator<char>(input),
                 std::istreambuf_iterator<char>());
    return !input.bad();
}

bool sourceGraphUnchanged(const dx_data& source) {
    constexpr std::array<std::uint32_t, 4> dependencyHandles{
        0x904u, 0x906u, 0x908u, 0x909u};
    if (source.dwgAssociativeObjects.size() != 6u
        || source.dwgAssociativeObjects[0].handle != 0x902u
        || source.dwgAssociativeObjects[1].handle != 0x903u
        || source.dwgAssociativeObjects[1].m_constraintNodeCount != 8
        || source.dwgAssociativeObjects[1].m_constraintNodes.size() != 8u
        || source.dwgAssociativeObjects[1].m_constraintNextNodeId != 9
        || source.dwgAssociativeObjects[1].m_constraintNodes[0].m_nodeId != 1
        || source.dwgAssociativeObjects[1].m_constraintNodes[1].m_nodeId != 2
        || source.dwgAssociativeObjects[1].m_constraintNodes[1].m_className
            != "AcFixedConstraint"
        || source.dwgAssociativeObjects[1].m_constraintNodes[1]
                .m_hasGeometryDependency
        || source.dwgAssociativeObjects[1].m_constraintNodes[2].m_className
            != "AcHorizontalConstraint"
        || source.dwgAssociativeObjects[1].m_constraintNodes[2]
                .m_datumLineIndex != 12
        || source.dwgAssociativeObjects[1].m_constraintNodes[3].m_className
            != "AcConstrainedCircle"
        || source.dwgAssociativeObjects[1].m_constraintNodes[3].m_radius
            != 4.0
        || source.dwgAssociativeObjects[1].m_constraintNodes[4].m_className
            != "AcConstrainedDatumLine"
        || source.dwgAssociativeObjects[1].m_constraintNodes[4]
                .m_geometryDependencyHandle != 0u
        || source.dwgAssociativeObjects[1].m_constraintNodes[4].m_point.x
            != 1.0
        || source.dwgAssociativeObjects[1].m_constraintNodes[5].m_className
            != "AcConstrainedBoundedLine"
        || !source.dwgAssociativeObjects[1].m_constraintNodes[5].m_isRay
        || source.dwgAssociativeObjects[1].m_constraintNodes[6].m_className
            != "AcConstrainedArc"
        || source.dwgAssociativeObjects[1].m_constraintNodes[6].m_radius
            != 5.0
        || source.dwgAssociativeObjects[1].m_constraintNodes[7].m_className
            != "AcConstrainedRigidSet"
        || source.dwgAssociativeObjects[1].m_constraintNodes[7]
                .m_rigidSetGeometryIds != std::vector<std::int32_t>({2, 3})
        || source.dwgAssociativeObjects[2].handle != 0x904u
        || source.dwgAssociativeObjects[2].parentHandle != 0x903u
        || source.dwgAssociativeObjects[2].m_dependencyHandle != 0x905u
        || source.dwgAssociativeObjects[2].m_geomPersistentSubentityIndex1
            != 201
        || source.dwgAssociativeObjects[2].m_geomPersistentSubentityIndex2
            != 202
        || source.mBlock == nullptr || source.mBlock->ent.size() != 1u)
        return false;
    for (std::size_t index = 0; index < dependencyHandles.size(); ++index) {
        const DRW_AssociativeObject& dependency =
            source.dwgAssociativeObjects[index + 2u];
        if (dependency.handle != dependencyHandles[index]
            || dependency.parentHandle != 0x903u
            || dependency.m_dependencyHandle != 0x905u)
            return false;
    }
    const auto* line = dynamic_cast<const DRW_Line*>(
        source.mBlock->ent.front());
    return line != nullptr && line->handle == 0x905u
        && line->reactorHandles == std::vector<std::uint32_t>(
            dependencyHandles.begin(), dependencyHandles.end());
}

bool verify(const std::filesystem::path& path) {
    dx_data data;
    dx_iface reader;
    if (!reader.fileImport(path.string(), &data, false)
        || !data.hasNamedObjectDictionary || data.mBlock == nullptr
        || data.associativeObjects.size() != 6u)
        return false;
    const auto root = std::find_if(
        data.namedObjectDictionary.m_entries.begin(),
        data.namedObjectDictionary.m_entries.end(),
        [](const DRW_Dictionary::Entry& entry) {
            return entry.m_name == "ACAD_ASSOCNETWORK";
        });
    if (root == data.namedObjectDictionary.m_entries.end())
        return false;
    const std::uint32_t dictionaryHandle = root->m_handle;
    const auto dictionary = std::find_if(
        data.dictionaries.begin(), data.dictionaries.end(),
        [dictionaryHandle](const DRW_Dictionary& entry) {
            return entry.handle == dictionaryHandle;
        });
    const auto network = std::find_if(
        data.associativeObjects.begin(), data.associativeObjects.end(),
        [](const DRW_AssociativeObject& object) {
            return object.m_recordName == "ACDBASSOCNETWORK";
        });
    const auto group = std::find_if(
        data.associativeObjects.begin(), data.associativeObjects.end(),
        [](const DRW_AssociativeObject& object) {
            return object.m_recordName == "ACDBASSOC2DCONSTRAINTGROUP";
        });
    const auto dependency = std::find_if(
        data.associativeObjects.begin(), data.associativeObjects.end(),
        [](const DRW_AssociativeObject& object) {
            return object.m_recordName == "ACDBASSOCGEOMDEPENDENCY";
        });
    const std::uint32_t circleDependencyHandle =
        group != data.associativeObjects.end()
            && group->m_constraintNodes.size() > 3u
        ? group->m_constraintNodes[3].m_geometryDependencyHandle : 0u;
    const auto circleDependency = std::find_if(
        data.associativeObjects.begin(), data.associativeObjects.end(),
        [circleDependencyHandle](const DRW_AssociativeObject& object) {
            return object.handle == circleDependencyHandle;
        });
    const DRW_Line* line = nullptr;
    for (const DRW_Entity* entity : data.mBlock->ent) {
        if (entity != nullptr && entity->eType == DRW::LINE) {
            line = dynamic_cast<const DRW_Line*>(entity);
            break;
        }
    }
    std::array<std::uint32_t, 4> dependencyHandles{};
    const bool hasDependencyRefs = group != data.associativeObjects.end()
        && group->m_constraintNodes.size() == 8u
        && group->m_constraintNodes[0].m_hasGeometryDependency
        && group->m_constraintNodes[3].m_hasGeometryDependency
        && group->m_constraintNodes[5].m_hasGeometryDependency
        && group->m_constraintNodes[6].m_hasGeometryDependency
        && group->m_constraintNodes[0].m_geometryDependencyHandle
            <= std::numeric_limits<std::uint32_t>::max()
        && group->m_constraintNodes[3].m_geometryDependencyHandle
            <= std::numeric_limits<std::uint32_t>::max()
        && group->m_constraintNodes[5].m_geometryDependencyHandle
            <= std::numeric_limits<std::uint32_t>::max()
        && group->m_constraintNodes[6].m_geometryDependencyHandle
            <= std::numeric_limits<std::uint32_t>::max();
    if (hasDependencyRefs) {
        dependencyHandles = {{
            static_cast<std::uint32_t>(
                group->m_constraintNodes[0].m_geometryDependencyHandle),
            static_cast<std::uint32_t>(
                group->m_constraintNodes[3].m_geometryDependencyHandle),
            static_cast<std::uint32_t>(
                group->m_constraintNodes[5].m_geometryDependencyHandle),
            static_cast<std::uint32_t>(
                group->m_constraintNodes[6].m_geometryDependencyHandle)}};
    }
    const bool dependenciesClosed = hasDependencyRefs && line != nullptr
        && std::all_of(dependencyHandles.begin(), dependencyHandles.end(),
        [&data, &group, line](std::uint32_t handle) {
            const auto dependency = std::find_if(
                data.associativeObjects.begin(), data.associativeObjects.end(),
                [handle](const DRW_AssociativeObject& object) {
                    return object.handle == handle;
                });
            if (dependency == data.associativeObjects.end()) {
                std::cerr << "readback dependency missing handle=" << handle
                          << '\n';
                return false;
            }
            if (dependency->m_recordName != "ACDBASSOCGEOMDEPENDENCY"
                || dependency->parentHandle != group->handle
                || dependency->m_dependencyHandle != line->handle
                || !dependency->m_geomDependencyFieldsParsed
                || dependency->m_geomPersistentSubentityIndex1 != 201
                || dependency->m_geomPersistentSubentityIndex2 != 202) {
                std::cerr << "readback dependency mismatch handle="
                          << dependency->handle << " class="
                          << dependency->m_recordName << " owner="
                          << dependency->parentHandle << " target="
                          << dependency->m_dependencyHandle << "/"
                          << line->handle << " geom="
                          << dependency->m_geomDependencyFieldsParsed << " idx="
                          << dependency->m_geomPersistentSubentityIndex1
                          << ','
                          << dependency->m_geomPersistentSubentityIndex2
                          << '\n';
                return false;
            }
            return dependency != data.associativeObjects.end()
                && dependency->m_recordName == "ACDBASSOCGEOMDEPENDENCY"
                && dependency->parentHandle == group->handle
                && dependency->m_dependencyHandle == line->handle
                && dependency->m_geomDependencyFieldsParsed
                && dependency->m_geomPersistentSubentityIndex1 == 201
                && dependency->m_geomPersistentSubentityIndex2 == 202;
        });
    const bool matches = root != data.namedObjectDictionary.m_entries.end()
        && dictionary != data.dictionaries.end()
        && root->m_handle == dictionary->handle
        && dictionary->parentHandle == data.namedObjectDictionary.handle
        && network != data.associativeObjects.end()
        && group != data.associativeObjects.end()
        && dependency != data.associativeObjects.end()
        && dependenciesClosed
        && circleDependency != data.associativeObjects.end()
        && line != nullptr
        && circleDependency->m_recordName == "ACDBASSOCGEOMDEPENDENCY"
        && circleDependency->parentHandle == group->handle
        && circleDependency->m_dependencyHandle == line->handle
        && circleDependency->m_geomDependencyFieldsParsed
        && circleDependency->m_geomPersistentSubentityIndex1 == 201
        && circleDependency->m_geomPersistentSubentityIndex2 == 202
        && network->parentHandle == dictionary->handle
        && network->m_actions.size() == 1u
        && network->m_actions.front().m_handle == group->handle
        && group->parentHandle == network->handle
        && group->m_constraintActions
            == std::vector<std::uint32_t>(dependencyHandles.begin(),
                                          dependencyHandles.end())
        && group->m_constraintActionCount == 4
        && group->m_constraintNodes.size() == 8u
        && group->m_constraintNodeCount == 8
        && group->m_constraintNodes[0].m_nodeId == 1
        && group->m_constraintNodes.front().m_hasGeometryDependency
        && group->m_constraintNodes.front().m_geometryDependencyHandle
            == dependency->handle
        && group->m_constraintNodes[1].m_nodeId == 2
        && group->m_constraintNodes[1].m_className == "AcFixedConstraint"
        && group->m_constraintNodes[1].m_dataKind
            == DRW_AssociativeConstraintNode::DataKind::Geometrical
        && !group->m_constraintNodes[1].m_hasGeometryDependency
        && group->m_constraintNodes[1].m_geometryDependencyHandle == 0u
        && group->m_constraintNodes[2].m_className
            == "AcHorizontalConstraint"
        && group->m_constraintNodes[2].m_hasDatumLineIndex
        && group->m_constraintNodes[2].m_datumLineIndex == 12
        && group->m_constraintNodes[3].m_className
            == "AcConstrainedCircle"
        && group->m_constraintNodes[3].m_hasGeometryDependency
        && group->m_constraintNodes[3].m_geometryDependencyHandle
            == circleDependency->handle
        && group->m_constraintNodes[3].m_geometryNodeId == 8
        && group->m_constraintNodes[3].m_center.x == 1.0
        && group->m_constraintNodes[3].m_center.y == 2.0
        && group->m_constraintNodes[3].m_center.z == 3.0
        && group->m_constraintNodes[3].m_radius == 4.0
        && group->m_constraintNodes[3].m_startParameter == 0.25
        && group->m_constraintNodes[3].m_endParameter == 0.75
        && group->m_constraintNodes[4].m_className
            == "AcConstrainedDatumLine"
        && group->m_constraintNodes[4].m_geometryDependencyHandle == 0u
        && group->m_constraintNodes[4].m_point.x == 1.0
        && group->m_constraintNodes[4].m_point.y == 2.0
        && group->m_constraintNodes[4].m_point.z == 3.0
        && group->m_constraintNodes[5].m_className
            == "AcConstrainedBoundedLine"
        && group->m_constraintNodes[5].m_geometryDependencyHandle
            == dependencyHandles[2]
        && group->m_constraintNodes[5].m_geometryNodeId == 10
        && group->m_constraintNodes[5].m_isRay
        && group->m_constraintNodes[5].m_startPoint.x == 2.0
        && group->m_constraintNodes[5].m_endPoint.x == 3.0
        && group->m_constraintNodes[6].m_className == "AcConstrainedArc"
        && group->m_constraintNodes[6].m_geometryDependencyHandle
            == dependencyHandles[3]
        && group->m_constraintNodes[6].m_geometryNodeId == 11
        && group->m_constraintNodes[6].m_center.z == 3.0
        && group->m_constraintNodes[6].m_radius == 5.0
        && group->m_constraintNodes[6].m_startParameter == 0.5
        && group->m_constraintNodes[6].m_endParameter == 1.0
        && group->m_constraintNodes[6].m_startPoint.y == 3.0
        && group->m_constraintNodes[6].m_endPoint.z == 7.0
        && group->m_constraintNodes[7].m_nodeId == 8
        && group->m_constraintNodes[7].m_className
            == "AcConstrainedRigidSet"
        && group->m_constraintNodes[7].m_hasGeometryDependency
        && group->m_constraintNodes[7].m_geometryDependencyHandle == 0u
        && group->m_constraintNodes[7].m_hasRigidSetReservedFlag
        && group->m_constraintNodes[7].m_rigidSetReservedFlag
        && group->m_constraintNodes[7].m_hasRigidSetTransform
        && group->m_constraintNodes[7].m_rigidSetTransform[0] == 1.0
        && group->m_constraintNodes[7].m_rigidSetTransform[5] == 1.0
        && group->m_constraintNodes[7].m_rigidSetTransform[10] == 1.0
        && group->m_constraintNodes[7].m_rigidSetTransform[15] == 1.0
        && group->m_constraintNodes[7].m_rigidSetGeometryIds
            == std::vector<std::int32_t>({2, 3})
        && dependency->parentHandle == group->handle
        && dependency->m_dependencyWireComplete
        && dependency->m_dependencyHandle == line->handle
        && dependency->m_geomDependencyFieldsParsed
        && dependency->m_geomPersistentSubentityClassName
            == "AcDbAssocEdgePersSubentId"
        && dependency->m_geomPersistentSubentityFieldsParsed
        && dependency->m_geomPersistentSubentityIndex1 == 201
        && dependency->m_geomPersistentSubentityIndex2 == 202
        && line->reactorHandles == std::vector<std::uint32_t>(
            dependencyHandles.begin(), dependencyHandles.end());
    if (!matches) {
        std::cerr << "associative graph readback: records="
                  << data.associativeObjects.size() << " root="
                  << (root != data.namedObjectDictionary.m_entries.end())
                  << " dictionary=" << (dictionary != data.dictionaries.end())
                  << " network=" << (network != data.associativeObjects.end())
                  << " group=" << (group != data.associativeObjects.end())
                  << " dependencies=" << dependenciesClosed << " line="
                  << (line != nullptr) << " refs=" << hasDependencyRefs;
        if (group != data.associativeObjects.end()
            && group->m_constraintNodes.size() == 8u) {
            for (const std::size_t index : {0u, 3u, 5u, 6u, 7u})
                std::cerr << " "
                          << group->m_constraintNodes[index]
                                 .m_hasGeometryDependency
                          << ":" << group->m_constraintNodes[index]
                                           .m_geometryDependencyHandle;
        }
        std::cerr << " max=" << DRW::NoHandle << '\n';
        if (group != data.associativeObjects.end()) {
            std::cerr << "group handle=" << group->handle << " parent="
                      << group->parentHandle << " actions="
                      << group->m_constraintActions.size() << " nodes="
                      << group->m_constraintNodes.size() << '\n';
            for (const DRW_AssociativeConstraintNode& node :
                 group->m_constraintNodes) {
                std::cerr << node.m_nodeId << ':' << node.m_className
                          << " kind=" << static_cast<int>(node.m_dataKind)
                          << " geom=" << node.m_hasGeometryDependency << '/'
                          << node.m_geometryDependencyHandle << " point="
                          << node.m_hasPoint << " ray=" << node.m_isRay
                          << " radius=" << node.m_radius << '\n';
            }
        }
    }
    return matches;
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "associative geometry graph allocation retry: "
                  << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    const auto output = directory / "associative-geometry-graph.dxf";
    const std::string sentinel = "prior associative geometry graph\n";
    dx_data source = makeSource();
    GeometryGraphExport exporter;

    for (const bool binary : {false, true}) {
        const auto write = [&]() {
            return exporter.fileExport(
                output.string(), DRW::AC1024, binary, &source, false, true,
                dx_dxfSourceTextPolicy::Compatible,
                DRW::DxfTextWritePolicy::RequireLossless,
                dx_dxfProxyGraphicsPolicy::FailIfNotPreserved,
                dx_dwgReadCompletenessPolicy::RequireComplete);
        };
        if (!write() || !verify(output))
            return fail("generated rooted geometry graph warm-up failed");
        exporter.failAt = 1000000;
        if (!write() || !verify(output))
            return fail("allocation calibration export/readback failed");
        const long allocationCount = exporter.reached;
        if (allocationCount <= 0)
            return fail("calibration observed no OBJECTS callback allocations");
        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior)
                    return fail("could not initialize destination sentinel");
            }
            exporter.failAt = offset;
            exporter.reached = 0;
            const bool wrote = write();
            exporter.failAt = -1;
            std::string current;
            if (wrote || exporter.reached < 1
                || !readAll(output, current) || current != sentinel)
                return fail("injected failure changed destination or was missed");
            if (!sourceGraphUnchanged(source))
                return fail("injected failure changed source graph");
            if (!write() || !verify(output))
                return fail("same-adapter retry failed rooted graph readback");
            if (!sourceGraphUnchanged(source))
                return fail("retry changed source dependency target");
        }
        std::cout << "AC1024 rooted geometry dependency graph "
                  << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", rollback/retry/readback=PASS\n";
    }
    std::error_code ignored;
    std::filesystem::remove(output, ignored);
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (allocationsBeforeFailure == 0) {
        ++failedAllocations;
        throw std::bad_alloc();
    }
    if (allocationsBeforeFailure > 0)
        --allocationsBeforeFailure;
    if (void* value = std::malloc(size == 0 ? 1 : size))
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
        std::cerr << "usage: associative_geometry_graph_allocation_tests <scratch-dir>\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
