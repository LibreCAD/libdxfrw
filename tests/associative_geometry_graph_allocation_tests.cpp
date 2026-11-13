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
    group.m_constraintActions = {
        0x904u, 0x906u, 0x908u, 0x909u, 0x90Au, 0x90Bu};
    group.m_constraintActionCount = 6;
    group.m_constraintNextNodeIdRead = true;
    group.m_constraintNextNodeId = 10;
    group.m_constraintNodesParsed = true;
    group.m_constraintNodeCount = 9;
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
    node.m_geometryDependencyCode = 0u;
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
    node = DRW_AssociativeConstraintNode();
    node.m_className = "AcDistanceConstraint";
    node.m_nodeId = 9;
    node.m_ownerId = 6;
    node.m_dataKind = DRW_AssociativeConstraintNode::DataKind::Distance;
    node.m_hasValueDependencyHandle = true;
    node.m_valueDependencyCode = DRW::DwgHardPointer;
    node.m_valueDependencyHandle = 0x90Au;
    node.m_hasDimensionDependencyHandle = true;
    node.m_dimensionDependencyCode = DRW::DwgHardPointer;
    node.m_dimensionDependencyHandle = 0x90Bu;
    node.m_directionType = 1u;
    node.m_hasDistance = true;
    node.m_distance = DRW_Coord(6.0, 7.0, 8.0);
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
        14u, 9u, 9, "authored nine-node inline constraint profile"});

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
    DRW_AssociativeObject valueDependency("ACDBASSOCVALUEDEPENDENCY");
    valueDependency.handle = 0x90Au;
    valueDependency.parentHandle = group.handle;
    valueDependency.m_classVersion = 1u;
    valueDependency.m_actionPrefixParsed = true;
    valueDependency.m_dependencyFieldsParsed = true;
    valueDependency.m_dependencyWireComplete = true;
    valueDependency.m_dependencyIsRead = true;
    valueDependency.m_dependencyIsAttachedToObject = true;
    valueDependency.m_dependencyIsDelegatingToOwningAction = true;
    valueDependency.m_dependencyHandle = 0x90Cu;
    valueDependency.m_valueDependencyFieldsParsed = true;
    valueDependency.m_valueDependencyClassVersion = 0;
    valueDependency.m_valueDependencyValueCode = 40;
    valueDependency.m_valueDependencyValueKind =
        DRW_AssociativeObject::EvalVariantKind::Real;
    valueDependency.m_valueDependencyValueReal = 8.0;
    valueDependency.m_prefixStatuses.push_back({
        DRW_AssociativePrefixStatus::Kind::AcDbAssocDependency,
        0u, 0u, 1u, DRW_AssociativePrefixStatus::ParseStatus::Complete,
        4u, 0u, 0, "authored value-dependency prefix"});
    valueDependency.m_prefixStatuses.push_back({
        DRW_AssociativePrefixStatus::Kind::AcDbAssocValueDependency,
        0u, 0u, 0u, DRW_AssociativePrefixStatus::ParseStatus::Complete,
        0u, 3u, 40, "authored value-dependency value profile"});

    DRW_AssociativeObject dimensionDependency("ACDBASSOCDEPENDENCY");
    dimensionDependency.handle = 0x90Bu;
    dimensionDependency.parentHandle = group.handle;
    dimensionDependency.m_classVersion = 1u;
    dimensionDependency.m_dependencyFieldsParsed = true;
    dimensionDependency.m_dependencyWireComplete = true;
    dimensionDependency.m_dependencyIsWrite = true;
    dimensionDependency.m_dependencyIsAttachedToObject = true;
    dimensionDependency.m_dependencyIsDelegatingToOwningAction = true;
    dimensionDependency.m_dependencyHandle = 0x90Du;
    dimensionDependency.m_dependencyBodyHandle = 0x90Eu;
    dimensionDependency.m_prefixStatuses.push_back({
        DRW_AssociativePrefixStatus::Kind::AcDbAssocDependency,
        0u, 0u, 1u, DRW_AssociativePrefixStatus::ParseStatus::Complete,
        4u, 0u, 0, "authored dimension-dependency prefix"});
    DRW_AssociativeObject dimensionBody("ACDBASSOCDIMDEPENDENCYBODY");
    dimensionBody.handle = 0x90Eu;
    dimensionBody.parentHandle = dimensionDependency.handle;
    dimensionBody.m_dependencyBodyVersion = 1u;
    dimensionBody.m_dimBaseVersion = 1u;
    dimensionBody.m_classVersion = 1u;
    dimensionBody.m_dimensionDependencyBodyParsed = true;
    dimensionBody.m_prefixStatuses.push_back({
        DRW_AssociativePrefixStatus::Kind::AcDbAssocDependencyBody,
        0u, 30u, 1u, DRW_AssociativePrefixStatus::ParseStatus::Complete,
        0u, 4u, 0, "authored dimension-dependency body profile"});

    DRW_AssociativeObject variable("ACDBASSOCVARIABLE");
    variable.handle = 0x90Cu;
    variable.parentHandle = network.handle;
    variable.reactorHandles.push_back(valueDependency.handle);
    variable.m_classVersion = 1u;
    variable.m_owningNetworkHandle = network.handle;
    variable.m_associativeVariableFieldsParsed = true;
    variable.m_associativeVariableWireComplete = true;
    variable.m_variableOwnedDependenciesParsed = true;
    variable.m_variableClassVersion = 2;
    variable.m_variableName = "BOLTHOLE";
    variable.m_variableExpression = "1";
    variable.m_variableEvaluator = "AcDbCalc:1.0";
    variable.m_variableValueParsed = true;
    variable.m_variableValueCode = 40;
    variable.m_variableValueKind =
        DRW_AssociativeObject::EvalVariantKind::Real;
    variable.m_variableValueReal = 8.0;
    variable.m_prefixStatuses.push_back({
        DRW_AssociativePrefixStatus::Kind::AcDbAssocAction,
        0u, 0u, 1u, DRW_AssociativePrefixStatus::ParseStatus::Complete,
        2u, 0u, 0, "authored variable action profile"});
    variable.m_prefixStatuses.push_back({
        DRW_AssociativePrefixStatus::Kind::AcDbAssocVariable,
        0u, 0u, 2u, DRW_AssociativePrefixStatus::ParseStatus::Complete,
        0u, 10u, 0, "authored variable value profile"});
    network.m_actions.push_back({true, variable.handle});
    network.m_prefixStatuses[0].m_decodedHandleCount = 2u;
    network.m_prefixStatuses[1].m_decodedHandleCount = 2u;
    network.m_prefixStatuses[1].m_decodedCountValue = 2;
    DRW_DimLinear* dimension = new DRW_DimLinear();
    dimension->handle = dimensionDependency.m_dependencyHandle;
    dimension->layer = "0";
    dimension->reactorHandles.push_back(dimensionDependency.handle);
    source.mBlock->ent.push_back(dimension);
    source.dwgAssociativeObjects = {
        network, group, dependency, circleDependency, boundedLineDependency,
        arcDependency, valueDependency, dimensionDependency, dimensionBody,
        variable};
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
    if (source.dwgAssociativeObjects.size() != 10u
        || source.dwgAssociativeObjects[0].handle != 0x902u
        || source.dwgAssociativeObjects[0].m_actions.size() != 2u
        || source.dwgAssociativeObjects[0].m_actions[0].m_handle != 0x903u
        || source.dwgAssociativeObjects[0].m_actions[1].m_handle != 0x90Cu
        || !source.dwgAssociativeObjects[0].m_actions[1].m_isOwned
        || source.dwgAssociativeObjects[1].handle != 0x903u
        || source.dwgAssociativeObjects[1].m_constraintActions
            != std::vector<std::uint32_t>(
                {0x904u, 0x906u, 0x908u, 0x909u, 0x90Au, 0x90Bu})
        || source.dwgAssociativeObjects[1].m_constraintNodeCount != 9
        || source.dwgAssociativeObjects[1].m_constraintNodes.size() != 9u
        || source.dwgAssociativeObjects[1].m_constraintNextNodeId != 10
        || source.dwgAssociativeObjects[1].m_constraintNodes[0].m_nodeId != 1
        || source.dwgAssociativeObjects[1].m_constraintNodes[1].m_nodeId != 2
        || source.dwgAssociativeObjects[1].m_constraintNodes[1].m_className
            != "AcFixedConstraint"
        || source.dwgAssociativeObjects[1].m_constraintNodes[1]
                .m_hasGeometryDependency
        || source.dwgAssociativeObjects[1].m_constraintNodes[1]
                .m_geometryDependencyCode != 0u
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
        || source.dwgAssociativeObjects[1].m_constraintNodes[8].m_className
            != "AcDistanceConstraint"
        || source.dwgAssociativeObjects[1].m_constraintNodes[8]
                .m_valueDependencyHandle != 0x90Au
        || source.dwgAssociativeObjects[1].m_constraintNodes[8]
                .m_dimensionDependencyHandle != 0x90Bu
        || source.dwgAssociativeObjects[1].m_constraintNodes[8]
                .m_distance.z != 8.0
        || source.dwgAssociativeObjects[2].handle != 0x904u
        || source.dwgAssociativeObjects[2].parentHandle != 0x903u
        || source.dwgAssociativeObjects[2].m_dependencyHandle != 0x905u
        || source.dwgAssociativeObjects[2].m_geomPersistentSubentityIndex1
            != 201
        || source.dwgAssociativeObjects[2].m_geomPersistentSubentityIndex2
            != 202
        || source.dwgAssociativeObjects[6].m_dependencyHandle != 0x90Cu
        || source.dwgAssociativeObjects[6].m_recordName
            != "ACDBASSOCVALUEDEPENDENCY"
        || source.dwgAssociativeObjects[6].parentHandle != 0x903u
        || !source.dwgAssociativeObjects[6].m_actionPrefixParsed
        || !source.dwgAssociativeObjects[6].m_dependencyWireComplete
        || source.dwgAssociativeObjects[6].m_valueDependencyValueReal != 8.0
        || source.dwgAssociativeObjects[6].m_valueDependencyValueCode != 40
        || source.dwgAssociativeObjects[6].m_prefixStatuses.size() != 2u
        || source.dwgAssociativeObjects[7].m_dependencyHandle != 0x90Du
        || source.dwgAssociativeObjects[7].parentHandle != 0x903u
        || !source.dwgAssociativeObjects[7].m_dependencyIsWrite
        || !source.dwgAssociativeObjects[7].m_dependencyIsAttachedToObject
        || source.dwgAssociativeObjects[7].m_dependencyBodyHandle != 0x90Eu
        || source.dwgAssociativeObjects[8].m_recordName
            != "ACDBASSOCDIMDEPENDENCYBODY"
        || source.dwgAssociativeObjects[8].parentHandle != 0x90Bu
        || !source.dwgAssociativeObjects[8]
                .m_dimensionDependencyBodyParsed
        || source.dwgAssociativeObjects[8].m_prefixStatuses.size() != 1u
        || source.dwgAssociativeObjects[8].m_prefixStatuses[0].m_bitSize
            != 30u
        || source.dwgAssociativeObjects[9].handle != 0x90Cu
        || source.dwgAssociativeObjects[9].parentHandle != 0x902u
        || source.dwgAssociativeObjects[9].m_owningNetworkHandle != 0x902u
        || source.dwgAssociativeObjects[9].m_variableName != "BOLTHOLE"
        || source.dwgAssociativeObjects[9].m_variableValueReal != 8.0
        || source.mBlock == nullptr || source.mBlock->ent.size() != 2u)
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
    const DRW_DimLinear* dimension = nullptr;
    for (const DRW_Entity* entity : source.mBlock->ent)
        if (entity != nullptr && entity->eType == DRW::DIMLINEAR)
            dimension = dynamic_cast<const DRW_DimLinear*>(entity);
    return line != nullptr && line->handle == 0x905u
        && line->reactorHandles == std::vector<std::uint32_t>(
            dependencyHandles.begin(), dependencyHandles.end())
        && dimension != nullptr && dimension->handle == 0x90Du
        && dimension->reactorHandles == std::vector<std::uint32_t>({0x90Bu});
}

bool verify(const std::filesystem::path& path) {
    dx_data data;
    dx_iface reader;
    if (!reader.fileImport(path.string(), &data, false)
        || !data.hasNamedObjectDictionary || data.mBlock == nullptr
        || data.associativeObjects.size() != 10u)
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
    const auto valueDependency = std::find_if(
        data.associativeObjects.begin(), data.associativeObjects.end(),
        [](const DRW_AssociativeObject& object) {
            return object.m_recordName == "ACDBASSOCVALUEDEPENDENCY";
        });
    const auto variable = std::find_if(
        data.associativeObjects.begin(), data.associativeObjects.end(),
        [](const DRW_AssociativeObject& object) {
            return object.m_recordName == "ACDBASSOCVARIABLE";
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
    const std::uint32_t valueDependencyHandle =
        group != data.associativeObjects.end()
            && group->m_constraintNodes.size() > 8u
        ? group->m_constraintNodes[8].m_valueDependencyHandle : 0u;
    const std::uint32_t dimensionDependencyHandle =
        group != data.associativeObjects.end()
            && group->m_constraintNodes.size() > 8u
        ? group->m_constraintNodes[8].m_dimensionDependencyHandle : 0u;
    const auto dimensionDependency = std::find_if(
        data.associativeObjects.begin(), data.associativeObjects.end(),
        [dimensionDependencyHandle](const DRW_AssociativeObject& object) {
            return object.handle == dimensionDependencyHandle;
        });
    const auto dimensionBody = dimensionDependency
            != data.associativeObjects.end()
        ? std::find_if(data.associativeObjects.begin(),
                       data.associativeObjects.end(),
            [&dimensionDependency](const DRW_AssociativeObject& object) {
                return object.handle
                    == dimensionDependency->m_dependencyBodyHandle;
            })
        : data.associativeObjects.end();
    const auto valueTarget = valueDependency != data.associativeObjects.end()
        ? std::find_if(data.associativeObjects.begin(),
                       data.associativeObjects.end(),
            [&valueDependency](const DRW_AssociativeObject& object) {
                return object.handle == valueDependency->m_dependencyHandle;
            })
        : data.associativeObjects.end();
    const DRW_Line* line = nullptr;
    const DRW_DimLinear* dimension = nullptr;
    for (const DRW_Entity* entity : data.mBlock->ent) {
        if (entity != nullptr && entity->eType == DRW::LINE) {
            line = dynamic_cast<const DRW_Line*>(entity);
            break;
        }
    }
    for (const DRW_Entity* entity : data.mBlock->ent) {
        if (entity != nullptr && entity->eType == DRW::DIMLINEAR) {
            dimension = dynamic_cast<const DRW_DimLinear*>(entity);
            break;
        }
    }
    std::array<std::uint32_t, 4> dependencyHandles{};
    const bool hasDependencyRefs = group != data.associativeObjects.end()
        && group->m_constraintNodes.size() == 9u
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
    const bool dimensionDependenciesClosed =
        group != data.associativeObjects.end()
        && valueDependency != data.associativeObjects.end()
        && variable != data.associativeObjects.end()
        && dimensionDependency != data.associativeObjects.end()
        && dimensionBody != data.associativeObjects.end()
        && valueTarget != data.associativeObjects.end()
        && dimension != nullptr
        && valueDependency->handle == valueDependencyHandle
        && valueDependency->m_classVersion == 1u
        && valueDependency->parentHandle == group->handle
        && valueDependency->m_dependencyWireComplete
        && valueDependency->m_dependencyIsRead
        && valueDependency->m_dependencyIsAttachedToObject
        && valueDependency->m_dependencyIsDelegatingToOwningAction
        && valueDependency->m_dependencyHandle == variable->handle
        && valueDependency->m_valueDependencyFieldsParsed
        && valueDependency->m_valueDependencyClassVersion == 0
        && valueDependency->m_valueDependencyValueCode == 40
        && valueDependency->m_valueDependencyValueKind
            == DRW_AssociativeObject::EvalVariantKind::Real
        && valueDependency->m_valueDependencyName.empty()
        && valueDependency->m_valueDependencyValueReal == 8.0
        && variable->m_variableValueParsed
        && variable->m_variableClassVersion == 2
        && variable->m_owningNetworkHandle == network->handle
        && variable->parentHandle == network->handle
        && variable->m_variableValueReal == 8.0
        && std::count(variable->reactorHandles.begin(),
                      variable->reactorHandles.end(),
                      valueDependency->handle) == 1
        && valueDependency->handle
            == group->m_constraintNodes[8].m_valueDependencyHandle
        && dimensionDependency->handle == dimensionDependencyHandle
        && dimensionDependency->m_recordName == "ACDBASSOCDEPENDENCY"
        && dimensionDependency->parentHandle == group->handle
        && dimensionDependency->m_dependencyHandle == dimension->handle
        && dimensionDependency->m_dependencyBodyHandle
            == dimensionBody->handle
        && dimensionDependency->m_dependencyIsDelegatingToOwningAction
        && !dimensionDependency->m_dependencyIsRead
        && dimensionDependency->m_dependencyIsWrite
        && dimensionDependency->m_dependencyIsAttachedToObject
        && dimensionBody->m_dimensionDependencyBodyParsed
        && dimensionBody->parentHandle == dimensionDependency->handle
        && dimensionBody->m_dependencyBodyVersion == 1u
        && dimensionBody->m_dimBaseVersion == 1u
        && dimensionBody->m_classVersion == 1u
        && dimension->handle == dimensionDependency->m_dependencyHandle
        && std::count(dimension->reactorHandles.begin(),
                      dimension->reactorHandles.end(),
                      dimensionDependency->handle) == 1
        && group->m_constraintNodes[8].m_dataKind
            == DRW_AssociativeConstraintNode::DataKind::Distance
        && group->m_constraintNodes[8].m_directionType == 1u
        && group->m_constraintNodes[8].m_hasDistance
        && group->m_constraintNodes[8].m_distance.x == 6.0
        && group->m_constraintNodes[8].m_distance.y == 7.0
        && group->m_constraintNodes[8].m_distance.z == 8.0;
    const std::vector<std::uint32_t> expectedGroupActions{
        dependencyHandles[0], dependencyHandles[1], dependencyHandles[2],
        dependencyHandles[3], valueDependencyHandle,
        dimensionDependencyHandle};
    const bool matches = root != data.namedObjectDictionary.m_entries.end()
        && dictionary != data.dictionaries.end()
        && root->m_handle == dictionary->handle
        && dictionary->parentHandle == data.namedObjectDictionary.handle
        && network != data.associativeObjects.end()
        && group != data.associativeObjects.end()
        && dependency != data.associativeObjects.end()
        && dependenciesClosed
        && dimensionDependenciesClosed
        && circleDependency != data.associativeObjects.end()
        && line != nullptr
        && circleDependency->m_recordName == "ACDBASSOCGEOMDEPENDENCY"
        && circleDependency->parentHandle == group->handle
        && circleDependency->m_dependencyHandle == line->handle
        && circleDependency->m_geomDependencyFieldsParsed
        && circleDependency->m_geomPersistentSubentityIndex1 == 201
        && circleDependency->m_geomPersistentSubentityIndex2 == 202
        && network->parentHandle == dictionary->handle
        && network->m_actions.size() == 2u
        && network->m_actions.front().m_handle == group->handle
        && network->m_actions.back().m_handle == variable->handle
        && group->parentHandle == network->handle
        && group->m_constraintActions == expectedGroupActions
        && group->m_constraintActionCount == 6
        && group->m_constraintNodes.size() == 9u
        && group->m_constraintNodeCount == 9
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
        && group->m_constraintNodes[8].m_nodeId == 9
        && group->m_constraintNodes[8].m_className
            == "AcDistanceConstraint"
        && group->m_constraintNodes[8].m_hasValueDependencyHandle
        && group->m_constraintNodes[8].m_valueDependencyHandle
            == valueDependency->handle
        && group->m_constraintNodes[8].m_hasDimensionDependencyHandle
        && group->m_constraintNodes[8].m_dimensionDependencyHandle
            == dimensionDependency->handle
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
                  << " dependencies=" << dependenciesClosed
                  << " dimensionDependencies="
                  << dimensionDependenciesClosed << " line="
                  << (line != nullptr) << " refs=" << hasDependencyRefs;
        if (group != data.associativeObjects.end()
            && group->m_constraintNodes.size() == 9u) {
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
