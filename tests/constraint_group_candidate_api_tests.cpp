// SPDX-License-Identifier: GPL-2.0-or-later
#include "associative_constraint_group_test_helpers.h"
#include "drw_objects.h"
#include "intern/dwgutil.h"
#include "libdwgr.h"
#include "lc3_compat_check.cpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <string>

using namespace associative_constraint_group_test;

namespace {
long failAt = -1;
long allocationCount = -1;
bool faultReached = false;
struct ScopedFault {
    explicit ScopedFault(long at) { failAt = at; allocationCount = 0; faultReached = false; }
    ~ScopedFault() { failAt = -1; allocationCount = -1; }
};
DRW_UnsupportedObject makeRaw(int variant = 0) {
    dwgBufferW object, strings, handles;
    object.putObjType(DRW::AC1032, 0x301u);
    object.putHandle(reference(DRW::DwgNullReference, 0xA600u));
    object.putBitShort(0); object.putBitLong(0); // EED, reactors
    object.putBit(1u); object.putBit(0u); // no extension dictionary/storage
    object.putBitShort(2); object.putBitLong(0); object.putBitLong(1);
    object.putBitLong(2); object.putBitLong(0);
    object.putBitShort(0); object.putBitLong(0);
    object.putBitShort(0); object.putBitLong(0);
    object.putBitLong(2); object.putBit(0u);
    object.put3BitDouble({0, 0, 0}); object.put3BitDouble({1, 0, 0});
    object.put3BitDouble({0, 1, 0}); object.putBitLong(2);
    object.putBitLong(4); object.putBitLong(0); object.putBitLong(0);
    object.putBit(0u); object.putBitLong(variant == 11 ? 5 : 4); // root flag, class count
    object.putBitLong(4); // registered count, excludes root
    for (int i = 1; i <= 4; ++i) {
        object.putBit(variant == 6 && i == 2 ? 1u : 0u);
        object.putBitLong(variant == 1 && i == 2 ? 9 : i);
        object.putBitLong(variant == 4 && i == 2 ? 1 : i);
    }
    if (variant == 10) {
        strings.putBitShort(250); // length extends beyond the detached stream
        strings.putRawChar8(0x41);
    } else {
        strings.putVariableText(DRW::AC1032, "AcConstrainedBoundedLine");
    }
    strings.putVariableText(DRW::AC1032, "AcConstrainedArc");
    strings.putVariableText(DRW::AC1032, "AcConstrainedImplicitPoint");
    strings.putVariableText(DRW::AC1032,
                            variant == 2 ? "UnknownCandidate" : "AcPointCurveConstraint");
    if (variant == 11)
        strings.putVariableText(DRW::AC1032, "UnusedUnknownCandidate");
    const auto common = [&](int id) {
        object.putBitLong(variant == 5 && id == 2 ? 3 : id);
        object.putBitLong(variant == 8 && id == 1 ? 257 : 1); object.putBitLong(0);
        object.putRawChar8(static_cast<std::uint8_t>(id));
    };
    common(1); object.putBitLong(0);
    object.put3BitDouble({variant == 7 ? std::numeric_limits<double>::quiet_NaN() : 1., 2, 3});
    object.put3BitDouble({1, 0, 0}); object.putBit(0u);
    object.put3BitDouble({1, 2, 3}); object.put3BitDouble({4, 2, 3});
    common(2); object.putBitLong(0); object.put3BitDouble({7, 8, 0});
    object.put3BitDouble({0, 0, 1}); object.put3BitDouble({1, 0, 0});
    object.putBitDouble(5); object.putBitDouble(0); object.putBitDouble(6);
    object.putBitDouble(42); // curve domain and separate reserved value
    object.put3BitDouble({10, 8, 0}); object.put3BitDouble({7, 13, 0});
    common(3); object.putBitLong(0); object.putRawChar8(2);
    object.putBitLong(1); object.putBitLong(1);
    common(4);
    if (variant != 9) {
        object.putBitLong(1); object.putBit(1u); object.putBit(0u);
    }
    if (!appendStringStream(object, strings)) return {};
    object.alignToByte();
    handles.putHandle(reference(DRW::DwgSoftPointer, 0xA601u));
    handles.putHandle(reference(DRW::DwgSoftPointer, 0xA602u));
    handles.putHandle(reference(DRW::DwgHardOwnership, 0u));
    handles.putHandle(reference(DRW::DwgHardOwnership, 0u));
    handles.putHandle(reference(DRW::DwgHardOwnership, 0xA603u));
    handles.putHandle(reference(DRW::DwgHardOwnership, 0xA604u));
    handles.putHandle(reference(variant == 3 ? DRW::DwgHardPointer : DRW::DwgSoftPointer, 0xA701u));
    handles.putHandle(reference(DRW::DwgSoftPointer, 0xA702u));
    handles.putHandle(reference(DRW::DwgSoftPointer, 0u));
    object.putBytes(handles.data().data(), handles.data().size());
    DRW_UnsupportedObject raw;
    raw.m_version = DRW::AC1032; raw.m_handle = 0xA600u;
    raw.m_recordName = "ACDBASSOC2DCONSTRAINTGROUP";
    raw.m_rawBytes = object.data();
    raw.m_objectSize = static_cast<std::uint32_t>(raw.m_rawBytes.size());
    raw.m_bodyBitSize = static_cast<std::uint32_t>(handles.bitCount());
    return raw;
}

bool generated() {
    auto raw = makeRaw();
    DRW_ConstraintGroupCandidate candidate;
    if (!drwDecodeConstraintGroupCandidate(raw, candidate)
        || candidate.m_sourceHandle != raw.m_handle
        || candidate.m_rootId != 0 || candidate.m_rootFlag
        || candidate.m_nextNodeIdCandidate != 4
        || candidate.m_classNames.size() != 4u
        || candidate.m_registeredNodes.size() != 4u)
        return false;
    const auto& line = candidate.m_registeredNodes[0];
    const auto& arc = candidate.m_registeredNodes[1];
    const auto& point = candidate.m_registeredNodes[2];
    const auto& constraint = candidate.m_registeredNodes[3];
    if (line.m_registryFlag || line.m_classIndex != 1 || line.m_statusRaw != 1
        || line.m_fields.m_geometryDependencyHandle != 0xA701u
        || line.m_fields.m_startPoint.x != 1 || line.m_fields.m_endPoint.x != 4
        || arc.m_fields.m_geometryDependencyHandle != 0xA702u
        || arc.m_fields.m_startParameter != 0 || arc.m_fields.m_endParameter != 6
        || arc.m_fields.m_startPoint.x != 10 || arc.m_fields.m_endPoint.y != 13
        || arc.m_fields.m_reserved != 42 || point.m_fields.m_pointType != 2
        || point.m_fields.m_curveNodeId != 1 || constraint.m_fields.m_ownerId != 1
        || !constraint.m_fields.m_isImplied || constraint.m_fields.m_isActive)
        return false;
    for (const auto& node : candidate.m_registeredNodes) {
        if (node.m_fields.m_classDataParsed || node.m_bodyStartBit >= node.m_bodyEndBit
            || node.m_handleStartBit > node.m_handleEndBit)
            return false;
    }
    const auto originalBytes = raw.m_rawBytes;
    for (int variant : {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}) {
        auto bad = makeRaw(variant);
        candidate.m_sourceHandle = 0xDEADBEEFu;
        if (drwDecodeConstraintGroupCandidate(bad, candidate)
            || candidate.m_sourceHandle != 0xDEADBEEFu
            || candidate.m_registeredNodes.size() != 4u
            || candidate.m_registeredNodes.front().m_fields.m_nodeId != 1
            || raw.m_rawBytes != originalBytes
            || !drwDecodeConstraintGroupCandidate(raw, candidate))
            return false;
    }
    auto bad = raw;
    bad.m_rawBytes.pop_back(); --bad.m_objectSize;
    candidate.m_sourceHandle = 0xDEADBEEFu;
    if (drwDecodeConstraintGroupCandidate(bad, candidate)
        || candidate.m_sourceHandle != 0xDEADBEEFu
        || !drwDecodeConstraintGroupCandidate(raw, candidate))
        return false;
    bad = raw; bad.m_handle++;
    candidate.m_sourceHandle = 0xDEADBEEFu;
    return !drwDecodeConstraintGroupCandidate(bad, candidate)
        && candidate.m_sourceHandle == 0xDEADBEEFu
        && drwDecodeConstraintGroupCandidate(raw, candidate);
}

bool allocation() {
    const auto raw = makeRaw();
    DRW_ConstraintGroupCandidate calibrated;
    bool success = false;
    long reached = 0;
    {
        ScopedFault measure(-1);
        success = drwDecodeConstraintGroupCandidate(raw, calibrated);
        reached = allocationCount;
    }
    if (!success || reached <= 0) return false;
    for (long index = 0; index < reached; ++index) {
        DRW_ConstraintGroupCandidate result;
        result.m_sourceHandle = 0xDEADBEEFu;
        result.m_classNames = {"sentinel"};
        DRW_ConstraintNodeCandidate prior;
        prior.m_statusRaw = 233;
        prior.m_fields.m_nodeId = 99;
        result.m_registeredNodes.push_back(prior);
        bool returned = false, threw = false, triggered = false;
        {
            ScopedFault inject(index);
            try { returned = drwDecodeConstraintGroupCandidate(raw, result); }
            catch (...) { threw = true; }
            triggered = faultReached;
        }
        if (!triggered || returned || threw || result.m_sourceHandle != 0xDEADBEEFu
            || result.m_classNames != std::vector<UTF8STRING>{"sentinel"}
            || result.m_registeredNodes.size() != 1u
            || result.m_registeredNodes.front().m_statusRaw != 233
            || result.m_registeredNodes.front().m_fields.m_nodeId != 99
            || !drwDecodeConstraintGroupCandidate(raw, result)
            || result.m_registeredNodes.size() != 4u) {
            std::cerr << "candidate allocation failure/retry failed at " << index << '\n';
            return false;
        }
    }
    std::cout << "candidate allocation failures=" << reached << '\n';
    return true;
}

class PhysicalCapture final : public LC3CompatCheck {
public:
    void addUnsupportedObject(const DRW_UnsupportedObject& raw) override {
        if (raw.m_handle == 0x2A8u) {
            ++count;
            ok = !raw.m_typedPayloadValidated
                && drwDecodeConstraintGroupCandidate(raw, result);
        }
    }
    DRW_ConstraintGroupCandidate result;
    int count = 0;
    bool ok = false;
};
bool physical(const char* path) {
    PhysicalCapture sink;
    dwgR input(path);
    const bool readOk = input.read(&sink, false);
    if (!readOk || sink.count != 1 || !sink.ok
        || sink.result.m_sourceHandle != 0x2A8u
        || sink.result.m_nextNodeIdCandidate != 13
        || sink.result.m_rootId != 0
        || sink.result.m_rootConnectionCount != 0
        || sink.result.m_rootFlag
        || sink.result.m_classNames.size() != 6u
        || sink.result.m_registeredNodes.size() != 13u) {
        std::cerr << "physical header read=" << readOk << " count=" << sink.count
                  << " ok=" << sink.ok << " handle=" << sink.result.m_sourceHandle
                  << " next=" << sink.result.m_nextNodeIdCandidate
                  << " root=" << sink.result.m_rootId
                  << " rootConnections=" << sink.result.m_rootConnectionCount
                  << " rootFlag=" << sink.result.m_rootFlag
                  << " classes=" << sink.result.m_classNames.size()
                  << " nodes=" << sink.result.m_registeredNodes.size() << '\n';
        return false;
    }
    const auto& line = sink.result.m_registeredNodes[0];
    const auto& arc = sink.result.m_registeredNodes[5];
    const std::array<int, 13> classes{{1, 2, 3, 2, 3, 4, 2, 3, 2, 3, 2, 5, 6}};
    const std::array<int, 13> statuses{{1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 0}};
    const std::array<std::vector<std::int32_t>, 13> connections{{
        {3, 5, 13}, {3}, {2, 1}, {5}, {4, 1}, {8, 10, 12, 13}, {8},
        {7, 6}, {10}, {9, 6}, {12}, {11, 6}, {1, 6}}};
    const std::array<const char*, 6> classNames{{
        "AcConstrainedBoundedLine", "AcConstrainedImplicitPoint",
        "AcPointCurveConstraint", "AcConstrainedArc",
        "AcCenterPointConstraint", "AcTangentConstraint"}};
    for (std::size_t i = 0; i < classNames.size(); ++i) {
        if (sink.result.m_classNames[i] != classNames[i])
            return false;
    }
    const std::array<std::uint64_t, 13> starts{{429, 1026, 1118, 1170, 1262,
        1314, 1934, 2026, 2078, 2170, 2222, 2314, 2366}};
    const std::array<std::uint64_t, 13> ends{{1026, 1118, 1170, 1262, 1314,
        1934, 2026, 2078, 2170, 2222, 2314, 2366, 2418}};
    for (std::size_t i = 0; i < classes.size(); ++i) {
        const auto& node = sink.result.m_registeredNodes[i];
        if (node.m_fields.m_nodeId != static_cast<std::int32_t>(i + 1)
            || node.m_classIndex != classes[i] || node.m_statusRaw != statuses[i]
            || node.m_fields.m_connections != connections[i]
            || node.m_registryFlag || node.m_bodyStartBit != starts[i]
            || node.m_bodyEndBit != ends[i] || node.m_fields.m_classDataParsed)
            return false;
    }
    const auto& firstImplicit = sink.result.m_registeredNodes[1].m_fields;
    const auto& firstPointCurve = sink.result.m_registeredNodes[2].m_fields;
    const auto& secondImplicit = sink.result.m_registeredNodes[3].m_fields;
    const auto& secondPointCurve = sink.result.m_registeredNodes[4].m_fields;
    const auto& arcImplicit = sink.result.m_registeredNodes[6].m_fields;
    const auto& arcPointCurve = sink.result.m_registeredNodes[7].m_fields;
    const auto& thirdArcImplicit = sink.result.m_registeredNodes[8].m_fields;
    const auto& thirdPointCurve = sink.result.m_registeredNodes[9].m_fields;
    const auto& fourthArcImplicit = sink.result.m_registeredNodes[10].m_fields;
    const auto& centerConstraint = sink.result.m_registeredNodes[11].m_fields;
    const auto& tangentConstraint = sink.result.m_registeredNodes[12].m_fields;
    const auto close = [](double actual, double expected, double tolerance) {
        return std::fabs(actual - expected) <= tolerance;
    };
    const auto closeCoord = [&close](const DRW_Coord& actual,
                                     double x, double y, double z) {
        return close(actual.x, x, 1e-9) && close(actual.y, y, 1e-9)
            && close(actual.z, z, 1e-9);
    };
    return line.m_fields.m_nodeId == 1 && line.m_statusRaw == 1
        && line.m_fields.m_className == classNames[0]
        && line.m_bodyStartBit == 429 && line.m_bodyEndBit == 1026
        && !line.m_fields.m_hasStatus
        && line.m_fields.m_dataKind
            == DRW_AssociativeConstraintNode::DataKind::BoundedLine
        && line.m_fields.m_hasPoint
        && line.m_fields.m_geometryDependencyHandle == 0x2A9u
        && line.m_fields.m_geometryDependencyCode == DRW::DwgSoftPointer
        && line.m_fields.m_geometryNodeId == 0
        && line.m_fields.m_isRay == false
        && closeCoord(line.m_fields.m_point,
                      1223.6079238755863, 1574.7905199791435, 0.0)
        && closeCoord(line.m_fields.m_direction,
                      0.9211473736090738, 0.38921397211947806, 0.0)
        && closeCoord(line.m_fields.m_startPoint,
                      1223.6079238755592, 1574.7905199791321, 0.0)
        && closeCoord(line.m_fields.m_endPoint,
                      2717.1980837635074, 2205.879719713629, 0.0)
        && firstImplicit.m_hasGeometryDependency
        && firstImplicit.m_dataKind
            == DRW_AssociativeConstraintNode::DataKind::ImplicitPoint
        && firstImplicit.m_geometryDependencyCode == DRW::DwgSoftPointer
        && firstImplicit.m_geometryDependencyHandle == 0u
        && firstImplicit.m_geometryNodeId == 0
        && !firstImplicit.m_hasPoint
        && firstImplicit.m_pointType == 0u
        && firstImplicit.m_pointIndex == -1
        && firstImplicit.m_curveNodeId == 1
        && secondImplicit.m_hasGeometryDependency
        && secondImplicit.m_geometryDependencyCode == DRW::DwgSoftPointer
        && secondImplicit.m_geometryDependencyHandle == 0u
        && secondImplicit.m_geometryNodeId == 0
        && !secondImplicit.m_hasPoint
        && secondImplicit.m_pointType == 1u
        && secondImplicit.m_pointIndex == -1
        && secondImplicit.m_curveNodeId == 1
        && firstPointCurve.m_ownerId == 0
        && firstPointCurve.m_dataKind
            == DRW_AssociativeConstraintNode::DataKind::Geometrical
        && firstPointCurve.m_hasActiveFlag
        && firstPointCurve.m_isImplied && firstPointCurve.m_isActive
        && secondPointCurve.m_ownerId == 0
        && secondPointCurve.m_dataKind
            == DRW_AssociativeConstraintNode::DataKind::Geometrical
        && secondPointCurve.m_hasActiveFlag
        && secondPointCurve.m_isImplied && secondPointCurve.m_isActive
        && arc.m_fields.m_nodeId == 6 && arc.m_bodyStartBit == 1314
        && arc.m_bodyEndBit == 1934 && arc.m_fields.m_geometryDependencyHandle == 0x2AAu
        && arc.m_fields.m_className == classNames[3]
        && !arc.m_fields.m_hasPoint
        && arc.m_fields.m_dataKind
            == DRW_AssociativeConstraintNode::DataKind::Arc
        && arc.m_fields.m_hasReserved
        && arc.m_fields.m_geometryDependencyCode == DRW::DwgSoftPointer
        && arc.m_fields.m_geometryNodeId == 0
        && closeCoord(arc.m_fields.m_center,
                      2054.8988985670894, 1081.7260399142574, 0.0)
        && closeCoord(arc.m_fields.m_normal, 0.0, 0.0, 1.0)
        && closeCoord(arc.m_fields.m_direction, 1.0, 0.0, 0.0)
        && close(arc.m_fields.m_startParameter, 0.0, 1e-12)
        && close(arc.m_fields.m_endParameter,
                 2 * 3.14159265358979323846, 1e-12)
        && close(arc.m_fields.m_reserved, 0.0, 1e-12)
        && close(arc.m_fields.m_radius, 777.7351130784532, 1e-8)
        && closeCoord(arc.m_fields.m_startPoint,
                      2611.1592886840135, 1625.2757473947436, 0.0)
        && closeCoord(arc.m_fields.m_endPoint,
                      1368.438090213127, 1447.298829805927, 0.0)
        && arcImplicit.m_hasGeometryDependency
        && arcImplicit.m_geometryDependencyCode == DRW::DwgSoftPointer
        && arcImplicit.m_geometryDependencyHandle == 0u
        && arcImplicit.m_geometryNodeId == 0
        && !arcImplicit.m_hasPoint
        && arcImplicit.m_pointType == 0u
        && arcImplicit.m_pointIndex == -1
        && arcImplicit.m_curveNodeId == 6
        && arcPointCurve.m_ownerId == 0
        && arcPointCurve.m_dataKind
            == DRW_AssociativeConstraintNode::DataKind::Geometrical
        && arcPointCurve.m_hasActiveFlag
        && arcPointCurve.m_isImplied && arcPointCurve.m_isActive
        && thirdArcImplicit.m_hasGeometryDependency
        && thirdArcImplicit.m_geometryDependencyCode == DRW::DwgSoftPointer
        && thirdArcImplicit.m_geometryDependencyHandle == 0u
        && thirdArcImplicit.m_geometryNodeId == 0
        && !thirdArcImplicit.m_hasPoint
        && thirdArcImplicit.m_pointIndex == -1
        && thirdArcImplicit.m_pointType == 1u
        && thirdArcImplicit.m_curveNodeId == 6
        && thirdPointCurve.m_ownerId == 0
        && thirdPointCurve.m_dataKind
            == DRW_AssociativeConstraintNode::DataKind::Geometrical
        && thirdPointCurve.m_hasActiveFlag
        && thirdPointCurve.m_isImplied && thirdPointCurve.m_isActive
        && fourthArcImplicit.m_hasGeometryDependency
        && fourthArcImplicit.m_geometryDependencyCode == DRW::DwgSoftPointer
        && fourthArcImplicit.m_geometryDependencyHandle == 0u
        && fourthArcImplicit.m_geometryNodeId == 0
        && !fourthArcImplicit.m_hasPoint
        && fourthArcImplicit.m_pointIndex == -1
        && fourthArcImplicit.m_pointType == 3u
        && fourthArcImplicit.m_curveNodeId == 6
        && centerConstraint.m_ownerId == 0
        && centerConstraint.m_dataKind
            == DRW_AssociativeConstraintNode::DataKind::Geometrical
        && centerConstraint.m_hasActiveFlag
        && centerConstraint.m_isImplied && centerConstraint.m_isActive
        && tangentConstraint.m_ownerId == 0
        && tangentConstraint.m_dataKind
            == DRW_AssociativeConstraintNode::DataKind::Geometrical
        && tangentConstraint.m_hasActiveFlag
        && !tangentConstraint.m_isImplied && tangentConstraint.m_isActive;
}
} // namespace

void* operator new(std::size_t size) {
    if (allocationCount >= 0 && allocationCount++ == failAt) {
        faultReached = true; throw std::bad_alloc();
    }
    if (void* result = std::malloc(size == 0 ? 1 : size)) return result;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* ptr) noexcept { std::free(ptr); }
void operator delete[](void* ptr) noexcept { std::free(ptr); }
void operator delete(void* ptr, std::size_t) noexcept { std::free(ptr); }
void operator delete[](void* ptr, std::size_t) noexcept { std::free(ptr); }

int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--physical")
        return physical(argv[2]) ? 0 : 1;
    if (argc != 1) return 2;
    if (!generated() || !allocation()) { std::cerr << "constraint candidate generated control failed\n"; return 1; }
    return 0;
}
