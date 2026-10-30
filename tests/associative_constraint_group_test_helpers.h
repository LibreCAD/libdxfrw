#ifndef LIBDXFRW_TESTS_ASSOCIATIVE_CONSTRAINT_GROUP_TEST_HELPERS_H
#define LIBDXFRW_TESTS_ASSOCIATIVE_CONSTRAINT_GROUP_TEST_HELPERS_H

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

#include "drw_base.h"
#include "intern/dwgbufferw.h"

namespace associative_constraint_group_test {

struct Node {
    std::int32_t id = 0;
    std::uint8_t status = 0u;
    std::int32_t declaredConnections = -1;
    std::vector<std::int32_t> connections;
    std::string className;
    std::int32_t ownerId = 0;
    bool isImplied = false;
    bool hasGeometricalBody = false;
    bool hasImplicitPointBody = false;
    bool hasHorizontalBody = false;
    bool hasDatumLineIndex = true;
    std::int32_t datumLineIndex = 0;
    bool hasDatumLineBody = false;
    bool hasDatumLineDirection = true;
    bool hasRadiusDiameterBody = false;
    bool hasRadiusDiameterMode = true;
    std::uint8_t radiusDiameterMode = 0u;
    std::uint8_t geometryDependencyCode = DRW::DwgSoftPointer;
    std::uint32_t geometryDependencyHandle = 0u;
    std::int32_t geometryNodeId = 0;
    bool hasPoint = false;
    DRW_Coord point;
    std::uint8_t pointType = 0u;
    std::int32_t pointIndex = 0;
    std::int32_t curveNodeId = 0;
    bool hasBoundedLineBody = false;
    DRW_Coord direction;
    bool isRay = false;
    DRW_Coord startPoint;
    DRW_Coord endPoint;
    bool hasCircleBody = false;
    bool hasArcBody = false;
    DRW_Coord center;
    DRW_Coord normal;
    double radius = 0.0;
    double startParameter = 0.0;
    double endParameter = 0.0;
    bool hasDistanceBody = false;
    std::uint8_t valueDependencyCode = DRW::DwgHardPointer;
    std::uint32_t valueDependencyHandle = 0u;
    std::uint8_t dimensionDependencyCode = DRW::DwgHardPointer;
    std::uint32_t dimensionDependencyHandle = 0u;
    std::uint8_t directionType = 0u;
    DRW_Coord distance;
};

struct Options {
    std::uint16_t classNumber = 602u;
    std::uint32_t handle = 0xA100u;
    std::int32_t classVersion = 1;
    std::int32_t geometryStatus = 0;
    std::int32_t actionIndex = 2;
    std::int32_t maxDependencyIndex = 143;
    std::uint32_t owningNetwork = 0xA010u;
    std::int32_t groupVersion = 2;
    bool groupFlag = false;
    DRW_Coord workplane[3] = {
        DRW_Coord(0.0, 0.0, 0.0),
        DRW_Coord(1.0, 0.0, 0.0),
        DRW_Coord(0.0, 1.0, 0.0)
    };
    std::uint32_t groupReference = 0u;
    std::int32_t declaredActionCount = -1;
    std::vector<std::uint32_t> actions{0xA200u, 0xA201u};
    std::int32_t nextNodeId = 0;
    std::int32_t declaredNodeCount = -1;
    std::vector<Node> nodes;
};

struct Frame {
    std::vector<std::uint8_t> payload;
    std::uint32_t handleBits = 0u;
    std::uint64_t objectEndBit = 0u;
};

inline bool appendStringStream(dwgBufferW& object,
                               const dwgBufferW& strings) {
    if (strings.data().empty()) {
        object.putBit(0u); // no R2007+ detached string stream
        return object.isGood();
    }
    const std::uint64_t stringBits = strings.bitCount();
    const std::uint64_t paddingBits =
        (7u - ((object.bitCount() + stringBits) & 7u)) & 7u;
    const std::uint64_t stringBitSize = stringBits + paddingBits;
    if (stringBitSize > 0x7FFFFFFFULL)
        return false;

    for (std::uint64_t bit = 0; bit < stringBits; ++bit) {
        const std::uint8_t byte = strings.data()[bit >> 3];
        object.putBit(static_cast<std::uint8_t>(
            (byte >> (7u - (bit & 7u))) & 1u));
    }
    for (std::uint64_t i = 0; i < paddingBits; ++i)
        object.putBit(0u);
    if (stringBitSize <= 0x7FFFu) {
        object.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    } else {
        object.putRawShort16(static_cast<std::uint16_t>(stringBitSize >> 15));
        object.putRawShort16(static_cast<std::uint16_t>(
            0x8000u | (stringBitSize & 0x7FFFu)));
    }
    object.putBit(1u);
    return object.isGood();
}

inline dwgHandle reference(std::uint8_t code, std::uint32_t ref) {
    dwgHandle result;
    result.code = code;
    result.size = ref <= 0xFFu ? 1u : 2u;
    result.ref = ref;
    result.ref64 = ref;
    return result;
}

inline bool makeFrame(const Options& options, Frame& frame) {
    dwgBufferW object;
    dwgBufferW strings;
    object.putObjType(DRW::AC1024, options.classNumber);
    object.putHandle(reference(DRW::DwgNullReference, options.handle));
    object.putBitShort(0); // no EED
    object.putBitLong(1); // one reactor
    object.putBit(1u); // no extension dictionary

    object.putBitShort(static_cast<std::int16_t>(options.classVersion));
    object.putBitLong(options.geometryStatus);
    object.putBitLong(options.actionIndex);
    object.putBitLong(options.maxDependencyIndex);
    object.putBitLong(0); // no base action dependencies
    object.putBitLong(options.groupVersion);
    object.putBit(options.groupFlag ? 1u : 0u);
    for (const auto& point : options.workplane)
        object.put3BitDouble(point);
    const std::int32_t actionCount = options.declaredActionCount >= 0
        ? options.declaredActionCount
        : static_cast<std::int32_t>(options.actions.size());
    object.putBitLong(actionCount);
    const std::int32_t nodeCount = options.declaredNodeCount >= 0
        ? options.declaredNodeCount
        : static_cast<std::int32_t>(options.nodes.size());
    object.putBitLong(options.nextNodeId);
    object.putBitLong(nodeCount);
    for (const auto& node : options.nodes) {
        strings.putVariableText(DRW::AC1024, node.className);
        object.putBitLong(node.id);
        const std::int32_t connectionCount = node.declaredConnections >= 0
            ? node.declaredConnections
            : static_cast<std::int32_t>(node.connections.size());
        object.putBitLong(connectionCount);
        for (const auto connection : node.connections)
            object.putBitLong(connection);
        if (node.hasDistanceBody) {
            object.putBitLong(node.ownerId);
            object.putBit(node.isImplied ? 1u : 0u);
            object.putRawChar8(node.directionType);
            if (node.directionType != 0u)
                object.put3BitDouble(node.distance);
        } else if (node.hasRadiusDiameterBody) {
            object.putBitLong(node.ownerId);
            object.putBit(node.isImplied ? 1u : 0u);
            if (node.hasRadiusDiameterMode)
                object.putRawChar8(node.radiusDiameterMode);
        } else if (node.hasHorizontalBody) {
            object.putBitLong(node.ownerId);
            object.putBit(node.isImplied ? 1u : 0u);
            if (node.hasDatumLineIndex)
                object.putBitLong(node.datumLineIndex);
        } else if (node.hasDatumLineBody) {
            object.putBitLong(node.geometryNodeId);
            object.put3BitDouble(node.point);
            if (node.hasDatumLineDirection)
                object.put3BitDouble(node.direction);
        } else if (node.hasGeometricalBody) {
            object.putBitLong(node.ownerId);
            object.putBit(node.isImplied ? 1u : 0u);
        } else if (node.hasImplicitPointBody) {
            object.putBitLong(node.geometryNodeId);
            if (node.geometryDependencyHandle != 0u && node.hasPoint)
                object.put3BitDouble(node.point);
            object.putRawChar8(node.pointType);
            object.putBitLong(node.pointIndex);
            object.putBitLong(node.curveNodeId);
        } else if (node.hasBoundedLineBody) {
            object.putBitLong(node.geometryNodeId);
            object.put3BitDouble(node.point);
            object.put3BitDouble(node.direction);
            object.putBit(node.isRay ? 1u : 0u);
            object.put3BitDouble(node.startPoint);
            object.put3BitDouble(node.endPoint);
        } else if (node.hasCircleBody) {
            object.putBitLong(node.geometryNodeId);
            object.put3BitDouble(node.center);
            object.put3BitDouble(node.normal);
            object.put3BitDouble(node.direction);
            object.putBitDouble(node.radius);
            object.putBitDouble(node.startParameter);
            object.putBitDouble(node.endParameter);
        } else if (node.hasArcBody) {
            object.putBitLong(node.geometryNodeId);
            object.put3BitDouble(node.center);
            object.put3BitDouble(node.normal);
            object.put3BitDouble(node.direction);
            object.putBitDouble(node.radius);
            object.putBitDouble(node.startParameter);
            object.putBitDouble(node.endParameter);
            object.put3BitDouble(node.startPoint);
            object.put3BitDouble(node.endPoint);
        }
    }
    if (!appendStringStream(object, strings))
        return false;
    frame.objectEndBit = object.bitCount();

    object.putHandle(reference(DRW::DwgSoftPointer, 0xA001u)); // owner
    object.putHandle(reference(DRW::DwgSoftPointer, 0xA001u)); // reactor
    object.putHandle(reference(DRW::DwgSoftPointer, options.owningNetwork));
    object.putHandle(reference(DRW::DwgHardOwnership, 0u)); // no action body
    object.putHandle(reference(
        options.groupReference == 0u ? DRW::DwgNullReference
                                     : DRW::DwgSoftPointer,
        options.groupReference));
    for (const auto action : options.actions)
        object.putHandle(reference(DRW::DwgHardOwnership, action));
    for (const auto& node : options.nodes) {
        if (node.hasDistanceBody || node.hasRadiusDiameterBody) {
            object.putHandle(reference(node.valueDependencyCode,
                                       node.valueDependencyHandle));
            object.putHandle(reference(node.dimensionDependencyCode,
                                       node.dimensionDependencyHandle));
        } else if (node.hasDatumLineBody) {
            object.putHandle(reference(node.geometryDependencyCode,
                                       node.geometryDependencyHandle));
        } else if (node.hasImplicitPointBody || node.hasBoundedLineBody
            || node.hasCircleBody || node.hasArcBody)
            object.putHandle(reference(node.geometryDependencyCode,
                                       node.geometryDependencyHandle));
    }
    object.alignToByte();
    if (!object.isGood() || object.bitCount() < frame.objectEndBit
        || object.bitCount() - frame.objectEndBit
               > std::numeric_limits<std::uint32_t>::max())
        return false;
    frame.handleBits = static_cast<std::uint32_t>(
        object.bitCount() - frame.objectEndBit);
    frame.payload = object.data();
    return true;
}

inline bool wrapFrame(const Frame& object, std::vector<std::uint8_t>& bytes) {
    dwgBufferW frame;
    frame.putModularShort(static_cast<std::int32_t>(object.payload.size()));
    frame.putUModularChar(object.handleBits);
    frame.putBytes(object.payload.data(), object.payload.size());
    frame.putRawShort16(frame.crc16(0xC0C1u, 0u, frame.size()));
    if (!frame.isGood())
        return false;
    bytes = frame.data();
    return true;
}

} // namespace associative_constraint_group_test

#endif
