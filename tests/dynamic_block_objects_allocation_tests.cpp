// SPDX-License-Identifier: GPL-2.0-or-later
// Generated dynamic-block OBJECTS record allocation rollback control.
#include "libdxfrw.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"

#include <cstdlib>
#include <array>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <sstream>
#include <string>
#include <vector>

class DxfDynamicBlockTestAccess {
public:
    static void attachWriter(dxfRW& codec, DRW::Version version,
                             bool binary, std::ostream& output) {
        codec.version = version;
        codec.binFile = binary;
        codec.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&output))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&output));
    }

    static bool writeEnd(dxfRW& codec) {
        return codec.writer != nullptr
            && codec.writer->writeString(0, "WIRE_END");
    }
};

namespace {
long allocationsBeforeFailure = -1;
std::size_t failedAllocations = 0u;

struct AllocationFailure {
    explicit AllocationFailure(long count) {
        allocationsBeforeFailure = count;
    }
    ~AllocationFailure() { allocationsBeforeFailure = -1; }
};

DRW_DynamicBlockObject makePurgePreventer() {
    DRW_DynamicBlockObject object(
        "ACDB_DYNAMICBLOCKPURGEPREVENTER_VERSION");
    object.handle = 0xE702u;
    object.parentHandle = 0xE701u;
    object.reactorHandles.push_back(0xE701u);
    object.m_purgeFlag = 1u;
    object.m_purgeBlockHandle = 0xE700u;
    object.m_bodyFullyDecoded = true;
    return object;
}

DRW_DynamicBlockObject makeVisibilityParameter() {
    DRW_DynamicBlockObject object("BLOCKVISIBILITYPARAMETER");
    object.handle = 0xE818u;
    object.parentHandle = 0xE817u;
    object.m_kind = DRW_DynamicBlockObject::Kind::VisibilityParameter;
    object.m_bodyFullyDecoded = true;
    object.m_evalExprParsed = true;
    object.m_parentId = std::numeric_limits<std::uint32_t>::max();
    object.m_major = 29u;
    object.m_minor = 2u;
    object.m_valueCode = -9999;
    object.m_nodeId = 16u;
    object.m_elementParsed = true;
    object.m_elementName = "Visibility State";
    object.m_elementMajor = 29u;
    object.m_elementMinor = 2u;
    object.m_propertyInfoCount = 17;
    object.m_isInitialized = true;
    object.m_visibilityName = "ARCH Title Block";
    object.m_blockCount = 117;
    for (std::uint32_t index = 0u; index < 117u; ++index)
        object.m_blockHandles.push_back(0x1001u + index);
    object.m_stateCount = 4;
    static constexpr const char* stateNames[] = {
        "ARCH D", "ARCH C", "ARCH B", "ARCH A"};
    for (std::size_t state = 0u; state < 4u; ++state) {
        object.m_stateNames.emplace_back(stateNames[state]);
        std::vector<std::uint32_t> stateBlocks;
        for (std::uint32_t member = 0u; member < 29u; ++member)
            stateBlocks.push_back(0x1001u
                + static_cast<std::uint32_t>(state) * 29u + member);
        object.m_stateBlockHandles.push_back(std::move(stateBlocks));
        object.m_stateParameterHandles.emplace_back();
    }
    return object;
}

std::map<std::uint32_t, std::uint32_t> visibilityRemap() {
    std::map<std::uint32_t, std::uint32_t> remap;
    for (std::uint32_t index = 0u; index < 117u; ++index) {
        const std::uint32_t source = 0x1001u + index;
        remap.emplace(source, source + 0x5000u);
    }
    return remap;
}

bool visibilitySourceUnchanged(const DRW_DynamicBlockObject& object) {
    return object.handle == 0xE818u
        && static_cast<std::uint32_t>(object.parentHandle) == 0xE817u
        && object.m_kind
            == DRW_DynamicBlockObject::Kind::VisibilityParameter
        && object.m_bodyFullyDecoded && object.m_evalExprParsed
        && object.m_elementName == "Visibility State"
        && object.m_visibilityName == "ARCH Title Block"
        && object.m_blockCount == 117
        && object.m_blockHandles.size() == 117u
        && object.m_blockHandles.front() == 0x1001u
        && object.m_blockHandles.back() == 0x1075u
        && object.m_stateCount == 4
        && object.m_stateNames.size() == 4u
        && object.m_stateNames.front() == "ARCH D"
        && object.m_stateNames.back() == "ARCH A"
        && object.m_stateBlockHandles.size() == 4u
        && std::all_of(object.m_stateBlockHandles.begin(),
            object.m_stateBlockHandles.end(),
            [](const auto& handles) { return handles.size() == 29u; })
        && object.m_stateBlockHandles.front().front() == 0x1001u
        && object.m_stateBlockHandles.back().back() == 0x1074u
        && object.m_stateParameterHandles.size() == 4u
        && std::all_of(object.m_stateParameterHandles.begin(),
            object.m_stateParameterHandles.end(),
            [](const auto& handles) { return handles.empty(); });
}

bool visibilityWireReadback(const std::string& bytes, bool binary) {
    std::istringstream input(bytes);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(std::make_unique<dxfReaderBinary>(&input))
        : std::unique_ptr<dxfReader>(std::make_unique<dxfReaderAscii>(&input));
    int code = 0;
    if (!reader->readRec(&code) || code != 0
        || reader->getString() != "BLOCKVISIBILITYPARAMETER")
        return false;
    bool hasHandle = false;
    bool hasOwner = false;
    std::size_t flatReferenceCount = 0u;
    std::size_t stateReferenceCount = 0u;
    bool referencesInRemapRange = true;
    bool ended = false;
    while (reader->readRec(&code)) {
        if (code == 0) {
            ended = reader->getString() == "WIRE_END";
            break;
        }
        switch (code) {
        case 5:
            hasHandle = reader->getHandleString() == 0xE818u;
            break;
        case 330:
            hasOwner = reader->getHandleString() == 0xE817u;
            break;
        case 331:
            {
                const std::uint32_t handle = reader->getHandleString();
                referencesInRemapRange = referencesInRemapRange
                    && handle >= 0x6001u && handle <= 0x6075u;
                ++flatReferenceCount;
            }
            break;
        case 332:
            {
                const std::uint32_t handle = reader->getHandleString();
                referencesInRemapRange = referencesInRemapRange
                    && handle >= 0x6001u && handle <= 0x6075u;
                ++stateReferenceCount;
            }
            break;
        default:
            break;
        }
    }
    return ended && hasHandle && hasOwner && referencesInRemapRange
        && flatReferenceCount == 117u && stateReferenceCount == 116u;
}

template <typename Factory, typename SourceCheck, typename WireCheck>
bool dynamicBlockAllocationSweep(
        const char* profile, bool binary, const Factory& makeObject,
        const SourceCheck& sourceUnchanged, const WireCheck& wireReadback,
        const std::map<std::uint32_t, std::uint32_t>& remap) {
    constexpr long calibrationLimit = 1000000;
    std::ostringstream cleanBytes;
    dxfRW cleanCodec("");
    DxfDynamicBlockTestAccess::attachWriter(
        cleanCodec, DRW::AC1024, binary, cleanBytes);
    cleanCodec.setHandleRemap(remap);
    DRW_DynamicBlockObject cleanObject = makeObject();
    long allocationCount = 0;
    bool cleanWrite = false;
    {
        AllocationFailure failure(calibrationLimit);
        cleanWrite = cleanCodec.writeDynamicBlockObject(&cleanObject);
        allocationCount = calibrationLimit - allocationsBeforeFailure;
    }
    const DRW_WritingContext* cleanContext =
        cleanCodec.getWritingContext();
    if (!cleanWrite || !sourceUnchanged(cleanObject)
        || cleanContext == nullptr
        || cleanContext->sourceHandleToWrittenObjectMap.count(
               cleanObject.handle) != 1u
        || cleanContext->sourceHandleToWrittenObjectMap.at(cleanObject.handle)
            != cleanCodec.remapHandle(cleanObject.handle)
        || !DxfDynamicBlockTestAccess::writeEnd(cleanCodec)
        || !wireReadback(cleanBytes.str(), binary) || allocationCount <= 0) {
        std::cerr << "clean " << profile
                  << " write/readback failed (binary=" << binary
                  << ") allocations=" << allocationCount << '\n';
        return false;
    }

    for (long offset = 0; offset < allocationCount; ++offset) {
        std::ostringstream failedBytes;
        dxfRW failedCodec("");
        DxfDynamicBlockTestAccess::attachWriter(
            failedCodec, DRW::AC1024, binary, failedBytes);
        failedCodec.setHandleRemap(remap);
        DRW_DynamicBlockObject object = makeObject();
        const std::size_t failuresBefore = failedAllocations;
        bool wrote = false;
        try {
            AllocationFailure failure(offset);
            wrote = failedCodec.writeDynamicBlockObject(&object);
        } catch (const std::bad_alloc&) {
            allocationsBeforeFailure = -1;
        }
        allocationsBeforeFailure = -1;
        const DRW_WritingContext* context = failedCodec.getWritingContext();
        if (wrote || failedAllocations == failuresBefore
            || !sourceUnchanged(object) || context == nullptr
            || context->sourceHandleToWrittenObjectMap.count(object.handle)
                != 0u) {
            std::cerr << profile << " allocation rollback failed at "
                      << (binary ? "Binary" : "ASCII")
                      << " offset=" << offset << " wrote=" << wrote
                      << " identityAbsent="
                      << (context != nullptr
                          && context->sourceHandleToWrittenObjectMap.count(
                                 object.handle) == 0u)
                      << '\n';
            return false;
        }

        // The failed low-level writer can retain partial stream bytes and a
        // sticky write error. Prove the source is retryable in a fresh writer.
        std::ostringstream retryBytes;
        dxfRW retryCodec("");
        DxfDynamicBlockTestAccess::attachWriter(
            retryCodec, DRW::AC1024, binary, retryBytes);
        retryCodec.setHandleRemap(remap);
        const bool retryWrote = retryCodec.writeDynamicBlockObject(&object);
        const DRW_WritingContext* retryContext =
            retryCodec.getWritingContext();
        if (!retryWrote || !sourceUnchanged(object) || retryContext == nullptr
            || retryContext->sourceHandleToWrittenObjectMap.count(
                   object.handle) != 1u
            || !DxfDynamicBlockTestAccess::writeEnd(retryCodec)
            || !wireReadback(retryBytes.str(), binary)) {
            std::cerr << profile << " fresh-writer retry failed at "
                      << (binary ? "Binary" : "ASCII")
                      << " offset=" << offset << '\n';
            return false;
        }
    }
    std::cout << (binary ? "Binary" : "ASCII") << ' ' << profile
              << " allocation offsets=" << allocationCount
              << "; source rollback and fresh-writer retries pass\n";
    return true;
}

bool visibilityAllocationSweep(bool binary) {
    return dynamicBlockAllocationSweep(
        "VisibilityParameter", binary, makeVisibilityParameter,
        visibilitySourceUnchanged, visibilityWireReadback,
        visibilityRemap());
}

DRW_DynamicBlockObject makeBasePointParameter() {
    DRW_DynamicBlockObject object("BLOCKBASEPOINTPARAMETER");
    object.handle = 0x1135u;
    object.parentHandle = 0x110Fu;
    object.m_kind = DRW_DynamicBlockObject::Kind::BasePointParameter;
    object.m_bodyFullyDecoded = true;
    object.m_evalExprParsed = true;
    object.m_parentId = std::numeric_limits<std::uint32_t>::max();
    object.m_major = 29u;
    object.m_minor = 2u;
    object.m_valueCode = -9999;
    object.m_nodeId = 79u;
    object.m_elementParsed = true;
    object.m_elementName = "Base Point";
    object.m_elementMajor = 29u;
    object.m_elementMinor = 2u;
    object.m_showProperties = true;
    object.m_defPoint = DRW_Coord(11.25, -12.5, 13.75);
    object.m_parameterValue93 = -4;
    object.m_parameterValue170 = 2;
    object.m_parameterValue171 = 3;
    object.m_point = DRW_Coord(21.25, -22.5, 23.75);
    object.m_basePoint = DRW_Coord(-31.25, 32.5, -33.75);
    return object;
}

bool basePointSourceUnchanged(const DRW_DynamicBlockObject& object) {
    return object.handle == 0x1135u
        && static_cast<std::uint32_t>(object.parentHandle) == 0x110Fu
        && object.m_kind
            == DRW_DynamicBlockObject::Kind::BasePointParameter
        && object.m_bodyFullyDecoded && object.m_evalExprParsed
        && object.m_parentId == std::numeric_limits<std::uint32_t>::max()
        && object.m_nodeId == 79u && object.m_elementParsed
        && object.m_elementName == "Base Point"
        && object.m_showProperties
        && object.m_defPoint.x == 11.25 && object.m_defPoint.y == -12.5
        && object.m_defPoint.z == 13.75
        && object.m_parameterValue93 == -4
        && object.m_parameterValue170 == 2
        && object.m_parameterValue171 == 3
        && object.m_point.x == 21.25 && object.m_point.y == -22.5
        && object.m_point.z == 23.75
        && object.m_basePoint.x == -31.25
        && object.m_basePoint.y == 32.5
        && object.m_basePoint.z == -33.75;
}

bool basePointWireReadback(const std::string& bytes, bool binary) {
    std::istringstream input(bytes);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(std::make_unique<dxfReaderBinary>(&input))
        : std::unique_ptr<dxfReader>(std::make_unique<dxfReaderAscii>(&input));
    int code = 0;
    if (!reader->readRec(&code) || code != 0
        || reader->getString() != "BLOCKBASEPOINTPARAMETER")
        return false;
    bool hasHandle = false;
    bool hasOwner = false;
    bool hasNode = false;
    bool hasDefPointX = false;
    bool hasParamValue = false;
    bool hasPointX = false;
    bool hasBasePointZ = false;
    bool hasSubclass = false;
    bool ended = false;
    while (reader->readRec(&code)) {
        if (code == 0) {
            ended = reader->getString() == "WIRE_END";
            break;
        }
        switch (code) {
        case 5:
            hasHandle = reader->getHandleString() == 0x1135u;
            break;
        case 330:
            hasOwner = reader->getHandleString() == 0x110Fu;
            break;
        case 90:
            hasNode = reader->getInt32() == 79;
            break;
        case 1010:
            hasDefPointX = reader->getDouble() == 11.25;
            break;
        case 93:
            hasParamValue = reader->getInt32() == -4;
            break;
        case 1011:
            hasPointX = reader->getDouble() == 21.25;
            break;
        case 1032:
            hasBasePointZ = reader->getDouble() == -33.75;
            break;
        case 100:
            hasSubclass = hasSubclass
                || reader->getString() == "AcDbBlockBasepointParameter";
            break;
        default:
            break;
        }
    }
    return ended && hasHandle && hasOwner && hasNode && hasDefPointX
        && hasParamValue && hasPointX && hasBasePointZ && hasSubclass;
}

bool basePointAllocationSweep(bool binary) {
    return dynamicBlockAllocationSweep(
        "BasePointParameter", binary, makeBasePointParameter,
        basePointSourceUnchanged, basePointWireReadback, {});
}

DRW_DynamicBlockObject makeAlignmentParameter() {
    DRW_DynamicBlockObject object("BLOCKALIGNMENTPARAMETER");
    object.handle = 0x1126u;
    object.parentHandle = 0x110Fu;
    object.m_kind = DRW_DynamicBlockObject::Kind::AlignmentParameter;
    object.m_bodyFullyDecoded = true;
    object.m_evalExprParsed = true;
    object.m_parentId = std::numeric_limits<std::uint32_t>::max();
    object.m_major = 29u;
    object.m_minor = 2u;
    object.m_valueCode = -9999;
    object.m_nodeId = 54u;
    object.m_elementParsed = true;
    object.m_elementName = "Alignment";
    object.m_elementMajor = 29u;
    object.m_elementMinor = 2u;
    object.m_showProperties = true;
    object.m_firstPoint = DRW_Coord(1.25, -2.5, 3.75);
    object.m_secondPoint = DRW_Coord(15.0, 4.5, -6.75);
    object.m_twoPointPropertyCodes[0] = {55, -4};
    object.m_twoPointPropertyNames[0] = {"Distance", "Width"};
    object.m_twoPointPropertyCodes[1] = {92};
    object.m_twoPointPropertyNames[1] = {"Angle"};
    object.m_twoPointPropertyCodes[3] = {301, 0};
    object.m_twoPointPropertyNames[3] = {"Position", "Enabled"};
    object.m_propertyStates = {55, 0, 7, -1};
    object.m_parameterBaseLocation = 2;
    object.m_alignPerpendicular = true;
    return object;
}

bool alignmentSourceUnchanged(const DRW_DynamicBlockObject& object) {
    return object.handle == 0x1126u
        && static_cast<std::uint32_t>(object.parentHandle) == 0x110Fu
        && object.m_kind
            == DRW_DynamicBlockObject::Kind::AlignmentParameter
        && object.m_bodyFullyDecoded && object.m_evalExprParsed
        && object.m_parentId == std::numeric_limits<std::uint32_t>::max()
        && object.m_nodeId == 54u && object.m_elementParsed
        && object.m_elementName == "Alignment"
        && object.m_firstPoint.x == 1.25
        && object.m_firstPoint.y == -2.5
        && object.m_firstPoint.z == 3.75
        && object.m_secondPoint.x == 15.0
        && object.m_secondPoint.y == 4.5
        && object.m_secondPoint.z == -6.75
        && object.m_twoPointPropertyCodes.size() == 4u
        && object.m_twoPointPropertyCodes[0]
            == std::vector<std::int32_t>{55, -4}
        && object.m_twoPointPropertyCodes[1]
            == std::vector<std::int32_t>{92}
        && object.m_twoPointPropertyCodes[2].empty()
        && object.m_twoPointPropertyCodes[3]
            == std::vector<std::int32_t>{301, 0}
        && object.m_twoPointPropertyNames.size() == 4u
        && object.m_twoPointPropertyNames[0]
            == std::vector<UTF8STRING>{"Distance", "Width"}
        && object.m_twoPointPropertyNames[1]
            == std::vector<UTF8STRING>{"Angle"}
        && object.m_twoPointPropertyNames[2].empty()
        && object.m_twoPointPropertyNames[3]
            == std::vector<UTF8STRING>{"Position", "Enabled"}
        && object.m_propertyStates
            == std::array<std::int32_t, 4>{{55, 0, 7, -1}}
        && object.m_parameterBaseLocation == 2
        && object.m_alignPerpendicular;
}

bool alignmentWireReadback(const std::string& bytes, bool binary) {
    std::istringstream input(bytes);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(std::make_unique<dxfReaderBinary>(&input))
        : std::unique_ptr<dxfReader>(std::make_unique<dxfReaderAscii>(&input));
    int code = 0;
    if (!reader->readRec(&code) || code != 0
        || reader->getString() != "BLOCKALIGNMENTPARAMETER")
        return false;
    bool hasHandle = false;
    bool hasOwner = false;
    bool hasNode = false;
    bool hasPropertyStates = false;
    bool hasConnections = false;
    bool hasBaseLocation = false;
    bool hasAlignmentSubclass = false;
    bool hasSubclass = false;
    bool ended = false;
    std::vector<std::int32_t> propertyStates;
    std::vector<std::int32_t> propertyCounts;
    std::vector<std::pair<std::int32_t, std::string>> connections;
    bool awaitingConnectionName = false;
    std::int32_t pendingConnectionCode = 0;
    while (reader->readRec(&code)) {
        if (code == 0) {
            ended = reader->getString() == "WIRE_END";
            break;
        }
        switch (code) {
        case 5:
            hasHandle = reader->getHandleString() == 0x1126u;
            break;
        case 330:
            hasOwner = reader->getHandleString() == 0x110Fu;
            break;
        case 90:
            hasNode = reader->getInt32() == 54;
            break;
        case 91:
            propertyStates.push_back(reader->getInt32());
            break;
        case 171:
        case 172:
        case 173:
        case 174:
            propertyCounts.push_back(reader->getInt32());
            break;
        case 92:
        case 93:
        case 94:
        case 95:
            pendingConnectionCode = reader->getInt32();
            awaitingConnectionName = true;
            break;
        case 301:
        case 302:
        case 303:
        case 304:
            if (awaitingConnectionName) {
                connections.emplace_back(
                    pendingConnectionCode, reader->getString());
                awaitingConnectionName = false;
            }
            break;
        case 177:
            hasBaseLocation = reader->getInt32() == 2;
            break;
        case 100:
            {
                const std::string marker = reader->getString();
                hasSubclass = hasSubclass || marker == "AcDbBlock2PtParameter";
                hasAlignmentSubclass = hasAlignmentSubclass
                    || marker == "AcDbBlockAlignmentParameter";
            }
            break;
        case 280:
            hasAlignmentSubclass = hasAlignmentSubclass || hasSubclass;
            break;
        default:
            break;
        }
    }
    hasPropertyStates = propertyStates
        == std::vector<std::int32_t>{55, 0, 7, -1};
    hasConnections = connections
        == std::vector<std::pair<std::int32_t, std::string>>{
            {55, "Distance"}, {-4, "Width"}, {92, "Angle"},
            {301, "Position"}, {0, "Enabled"}};
    return ended && hasHandle && hasOwner && hasNode && hasPropertyStates
        && propertyCounts == std::vector<std::int32_t>{2, 1, 0, 2}
        && hasConnections && hasBaseLocation && hasSubclass
        && hasAlignmentSubclass;
}

DRW_DynamicBlockObject makeStretchAction() {
    DRW_DynamicBlockObject object("BLOCKSTRETCHACTION");
    object.handle = 0x1136u;
    object.parentHandle = 0x110Fu;
    object.m_kind = DRW_DynamicBlockObject::Kind::StretchAction;
    object.m_bodyFullyDecoded = true;
    object.m_evalExprParsed = true;
    object.m_parentId = std::numeric_limits<std::uint32_t>::max();
    object.m_major = 29u;
    object.m_minor = 2u;
    object.m_valueCode = -9999;
    object.m_nodeId = 90u;
    object.m_elementParsed = true;
    object.m_elementName = "Stretch0";
    object.m_elementMajor = 29u;
    object.m_elementMinor = 2u;
    object.m_actionCount = 2;
    object.m_actionIndexes = {17, 24};
    object.m_dependencyCount = 1;
    object.m_dependencyHandles = {0x1127u};
    object.m_displayLocation = DRW_Coord(45.0, -15.0, 0.0);
    object.m_connectionCodes = {1, 8};
    object.m_connectionNames = {"EndXDelta", "EndYDelta"};
    object.m_stretchPointCount = 2;
    object.m_stretchPoints = {
        DRW_Coord(34.5, 4.5, 0.0), DRW_Coord(26.0, -6.5, 0.0)};
    object.m_handleReferenceCount = 2;
    object.m_handleReferenceHandles = {0x1143u, 0x1144u};
    object.m_handleReferenceIndexCounts = {2, 1};
    object.m_handleReferenceIndexes = {{1, 2}, {0}};
    object.m_codeReferenceCount = 1;
    object.m_codeReferenceCodes = {31};
    object.m_codeReferenceIndexCounts = {2};
    object.m_codeReferenceIndexes = {{1, 2}};
    object.m_actionOffsetX = 1.0;
    object.m_actionOffsetY = 0.0;
    return object;
}

DRW_DynamicBlockObject makeMoveAction() {
    DRW_DynamicBlockObject object("BLOCKMOVEACTION");
    object.handle = 0x1134u;
    object.parentHandle = 0x110Fu;
    object.m_kind = DRW_DynamicBlockObject::Kind::MoveAction;
    object.m_bodyFullyDecoded = true;
    object.m_evalExprParsed = true;
    object.m_parentId = std::numeric_limits<std::uint32_t>::max();
    object.m_major = 29u;
    object.m_minor = 2u;
    object.m_valueCode = -9999;
    object.m_nodeId = 81u
        + static_cast<std::uint32_t>(object.m_kind);
    object.m_elementParsed = true;
    object.m_elementName = "MoveHinge";
    object.m_elementMajor = 29u;
    object.m_elementMinor = 2u;
    object.m_actionCount = 3;
    object.m_actionIndexes = {17, 24, 31};
    object.m_dependencyCount = 2;
    object.m_dependencyHandles = {0x1127u, 0x1128u};
    object.m_displayLocation = DRW_Coord(45.0, -15.0, 0.0);
    object.m_connectionCodes = {1, 8};
    object.m_connectionNames = {"EndXDelta", "EndYDelta"};
    object.m_actionOffsetX = 0.5;
    object.m_actionOffsetY = 0.0;
    return object;
}

std::map<std::uint32_t, std::uint32_t> moveActionRemap() {
    return {{0x1127u, 0x6127u}, {0x1128u, 0x6128u}};
}

bool moveActionSourceUnchanged(const DRW_DynamicBlockObject& object) {
    return object.handle == 0x1134u
        && static_cast<std::uint32_t>(object.parentHandle) == 0x110Fu
        && object.m_kind == DRW_DynamicBlockObject::Kind::MoveAction
        && object.m_bodyFullyDecoded && object.m_evalExprParsed
        && object.m_parentId == std::numeric_limits<std::uint32_t>::max()
        && object.m_nodeId == 81u
            + static_cast<std::uint32_t>(object.m_kind)
        && object.m_elementParsed && object.m_elementName == "MoveHinge"
        && object.m_actionCount == 3
        && object.m_actionIndexes == std::vector<std::int32_t>{17, 24, 31}
        && object.m_dependencyCount == 2
        && object.m_dependencyHandles
            == std::vector<std::uint32_t>{0x1127u, 0x1128u}
        && object.m_connectionCodes == std::vector<std::int32_t>{1, 8}
        && object.m_connectionNames
            == std::vector<UTF8STRING>{"EndXDelta", "EndYDelta"}
        && object.m_actionOffsetX == 0.5 && object.m_actionOffsetY == 0.0;
}

bool moveActionWireReadback(const std::string& bytes, bool binary) {
    std::istringstream input(bytes);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(std::make_unique<dxfReaderBinary>(&input))
        : std::unique_ptr<dxfReader>(std::make_unique<dxfReaderAscii>(&input));
    int code = 0;
    if (!reader->readRec(&code) || code != 0
        || reader->getString() != "BLOCKMOVEACTION")
        return false;
    bool ended = false;
    bool hasHandle = false;
    bool hasNode = false;
    bool hasActionCount = false;
    bool hasDependencyCount = false;
    bool hasOffsetX = false;
    bool hasOffsetY = false;
    bool hasSubclass = false;
    std::vector<std::uint32_t> ownerAndDependencies;
    std::vector<std::int32_t> actionIndexes;
    std::vector<std::int32_t> connectionCodes;
    std::vector<std::string> connectionNames;
    while (reader->readRec(&code)) {
        if (code == 0) {
            ended = reader->getString() == "WIRE_END";
            break;
        }
        switch (code) {
        case 5:
            hasHandle = reader->getHandleString() == 0x1134u;
            break;
        case 330:
            ownerAndDependencies.push_back(reader->getHandleString());
            break;
        case 90:
            hasNode = reader->getInt32() == 81
                + static_cast<std::int32_t>(
                    DRW_DynamicBlockObject::Kind::MoveAction);
            break;
        case 70:
            hasActionCount = reader->getInt32() == 3;
            break;
        case 71:
            hasDependencyCount = reader->getInt32() == 2;
            break;
        case 91:
            actionIndexes.push_back(reader->getInt32());
            break;
        case 92:
        case 93:
            connectionCodes.push_back(reader->getInt32());
            break;
        case 140:
            hasOffsetX = reader->getDouble() == 0.5;
            break;
        case 141:
            hasOffsetY = reader->getDouble() == 0.0;
            break;
        case 100:
            hasSubclass = hasSubclass
                || reader->getString() == "AcDbBlockMoveAction";
            break;
        case 301:
        case 302:
            connectionNames.push_back(reader->getString());
            break;
        default:
            break;
        }
    }
    return ended && hasHandle && hasNode && hasActionCount
        && hasDependencyCount && hasOffsetX && hasOffsetY && hasSubclass
        && ownerAndDependencies
            == std::vector<std::uint32_t>{0x110Fu, 0x6127u, 0x6128u}
        && actionIndexes == std::vector<std::int32_t>{17, 24, 31}
        && connectionCodes == std::vector<std::int32_t>{1, 8}
        && connectionNames
            == std::vector<std::string>{"EndXDelta", "EndYDelta"};
}

bool moveActionAllocationSweep(bool binary) {
    return dynamicBlockAllocationSweep(
        "MoveAction", binary, makeMoveAction,
        moveActionSourceUnchanged, moveActionWireReadback,
        moveActionRemap());
}

DRW_DynamicBlockObject makeScaleAction() {
    DRW_DynamicBlockObject object("BLOCKSCALEACTION");
    object.handle = 0x1135u;
    object.parentHandle = 0x110Fu;
    object.m_kind = DRW_DynamicBlockObject::Kind::ScaleAction;
    object.m_bodyFullyDecoded = true;
    object.m_evalExprParsed = true;
    object.m_parentId = std::numeric_limits<std::uint32_t>::max();
    object.m_major = 29u;
    object.m_minor = 2u;
    object.m_valueCode = -9999;
    object.m_nodeId = 81u
        + static_cast<std::uint32_t>(object.m_kind);
    object.m_elementParsed = true;
    object.m_elementName = "ScaleArc";
    object.m_elementMajor = 29u;
    object.m_elementMinor = 2u;
    object.m_actionCount = 0;
    object.m_dependencyCount = 4;
    object.m_dependencyHandles = {0x1127u, 0x1128u, 0x1129u, 0x112Au};
    object.m_displayLocation = DRW_Coord(45.0, -15.0, 0.0);
    object.m_baseConnectionCodes = {1, 1};
    object.m_baseConnectionNames = {"Scale", "XScale"};
    object.m_scaleOffset = DRW_Coord(0.0, 0.0, 0.0);
    object.m_scaleDependent = true;
    object.m_scaleBasePoint = DRW_Coord(1.0, 2.0, 0.0);
    object.m_scaleConnectionCodes = {1, 1, 1};
    object.m_scaleConnectionNames = {"Scale", "XScale", "YScale"};
    return object;
}

std::map<std::uint32_t, std::uint32_t> scaleActionRemap() {
    return {{0x1127u, 0x6127u}, {0x1128u, 0x6128u},
            {0x1129u, 0x6129u}, {0x112Au, 0x612Au}};
}

bool scaleActionSourceUnchanged(const DRW_DynamicBlockObject& object) {
    return object.handle == 0x1135u
        && static_cast<std::uint32_t>(object.parentHandle) == 0x110Fu
        && object.m_kind == DRW_DynamicBlockObject::Kind::ScaleAction
        && object.m_bodyFullyDecoded && object.m_evalExprParsed
        && object.m_parentId == std::numeric_limits<std::uint32_t>::max()
        && object.m_nodeId == 81u
            + static_cast<std::uint32_t>(object.m_kind)
        && object.m_elementParsed && object.m_elementName == "ScaleArc"
        && object.m_actionCount == 0 && object.m_actionIndexes.empty()
        && object.m_dependencyCount == 4
        && object.m_dependencyHandles
            == std::vector<std::uint32_t>{0x1127u, 0x1128u, 0x1129u, 0x112Au}
        && object.m_baseConnectionCodes
            == std::vector<std::int32_t>{1, 1}
        && object.m_baseConnectionNames
            == std::vector<UTF8STRING>{"Scale", "XScale"}
        && object.m_scaleOffset.x == 0.0 && object.m_scaleOffset.y == 0.0
        && object.m_scaleOffset.z == 0.0 && object.m_scaleDependent
        && object.m_scaleBasePoint.x == 1.0
        && object.m_scaleBasePoint.y == 2.0
        && object.m_scaleBasePoint.z == 0.0
        && object.m_scaleConnectionCodes
            == std::vector<std::int32_t>{1, 1, 1}
        && object.m_scaleConnectionNames
            == std::vector<UTF8STRING>{"Scale", "XScale", "YScale"};
}

bool scaleActionWireReadback(const std::string& bytes, bool binary) {
    std::istringstream input(bytes);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(std::make_unique<dxfReaderBinary>(&input))
        : std::unique_ptr<dxfReader>(std::make_unique<dxfReaderAscii>(&input));
    int code = 0;
    if (!reader->readRec(&code) || code != 0
        || reader->getString() != "BLOCKSCALEACTION")
        return false;
    bool ended = false;
    bool hasHandle = false;
    bool hasNode = false;
    bool hasActionCount = false;
    bool hasDependencyCount = false;
    bool hasDependent = false;
    bool hasScaleSubclass = false;
    std::vector<std::uint32_t> ownerAndDependencies;
    std::vector<std::int32_t> baseCodes;
    std::vector<std::string> baseNames;
    std::vector<std::int32_t> scaleCodes;
    std::vector<std::string> scaleNames;
    std::vector<double> scaleGeometry;
    while (reader->readRec(&code)) {
        if (code == 0) {
            ended = reader->getString() == "WIRE_END";
            break;
        }
        switch (code) {
        case 5:
            hasHandle = reader->getHandleString() == 0x1135u;
            break;
        case 330:
            ownerAndDependencies.push_back(reader->getHandleString());
            break;
        case 90:
            hasNode = reader->getInt32() == 81
                + static_cast<std::int32_t>(
                    DRW_DynamicBlockObject::Kind::ScaleAction);
            break;
        case 70:
            hasActionCount = reader->getInt32() == 0;
            break;
        case 71:
            hasDependencyCount = reader->getInt32() == 4;
            break;
        case 92:
        case 93:
            baseCodes.push_back(reader->getInt32());
            break;
        case 94:
        case 95:
        case 96:
            scaleCodes.push_back(reader->getInt32());
            break;
        case 1011:
        case 1021:
        case 1031:
        case 1012:
        case 1022:
        case 1032:
            scaleGeometry.push_back(reader->getDouble());
            break;
        case 280:
            hasDependent = reader->getInt32() == 1;
            break;
        case 100:
            hasScaleSubclass = hasScaleSubclass
                || reader->getString() == "AcDbBlockScaleAction";
            break;
        case 301:
        case 302:
            baseNames.push_back(reader->getString());
            break;
        case 303:
        case 304:
        case 305:
            scaleNames.push_back(reader->getString());
            break;
        default:
            break;
        }
    }
    return ended && hasHandle && hasNode && hasActionCount
        && hasDependencyCount && hasDependent && hasScaleSubclass
        && ownerAndDependencies == std::vector<std::uint32_t>{
            0x110Fu, 0x6127u, 0x6128u, 0x6129u, 0x612Au}
        && baseCodes == std::vector<std::int32_t>{1, 1}
        && baseNames == std::vector<std::string>{"Scale", "XScale"}
        && scaleCodes == std::vector<std::int32_t>{1, 1, 1}
        && scaleNames
            == std::vector<std::string>{"Scale", "XScale", "YScale"}
        && scaleGeometry == std::vector<double>{0.0, 0.0, 0.0, 1.0, 2.0, 0.0};
}

bool scaleActionAllocationSweep(bool binary) {
    return dynamicBlockAllocationSweep(
        "ScaleAction", binary, makeScaleAction,
        scaleActionSourceUnchanged, scaleActionWireReadback,
        scaleActionRemap());
}

DRW_DynamicBlockObject makeFlipAction() {
    DRW_DynamicBlockObject object("BLOCKFLIPACTION");
    object.handle = 0x1133u;
    object.parentHandle = 0x110Fu;
    object.m_kind = DRW_DynamicBlockObject::Kind::FlipAction;
    object.m_bodyFullyDecoded = true;
    object.m_evalExprParsed = true;
    object.m_parentId = std::numeric_limits<std::uint32_t>::max();
    object.m_major = 29u;
    object.m_minor = 2u;
    object.m_valueCode = -9999;
    object.m_nodeId = 76u;
    object.m_elementParsed = true;
    object.m_elementName = "FlipHinge";
    object.m_elementMajor = 29u;
    object.m_elementMinor = 2u;
    object.m_eed1071 = 0;
    object.m_actionCount = 11;
    object.m_actionIndexes = {79, 65, 66, 67, 68, 69, 1, 8, 31, 46, 54};
    object.m_dependencyCount = 27;
    for (std::uint32_t index = 0u; index < 27u; ++index)
        object.m_dependencyHandles.push_back(0x1127u + index);
    object.m_displayLocation = DRW_Coord(35.0, -15.0, 0.0);
    object.m_connectionCodes = {26, 26, 26, 26};
    object.m_connectionNames = {
        "Flip", "UpdatedFlip", "UpdatedBase", "UpdatedEnd"};
    return object;
}

std::map<std::uint32_t, std::uint32_t> flipActionRemap() {
    std::map<std::uint32_t, std::uint32_t> remap;
    for (std::uint32_t index = 0u; index < 27u; ++index)
        remap.emplace(0x1127u + index, 0x6127u + index);
    return remap;
}

bool flipActionSourceUnchanged(const DRW_DynamicBlockObject& object) {
    std::vector<std::uint32_t> dependencies;
    for (std::uint32_t index = 0u; index < 27u; ++index)
        dependencies.push_back(0x1127u + index);
    return object.handle == 0x1133u
        && static_cast<std::uint32_t>(object.parentHandle) == 0x110Fu
        && object.m_kind == DRW_DynamicBlockObject::Kind::FlipAction
        && object.m_bodyFullyDecoded && object.m_evalExprParsed
        && object.m_parentId == std::numeric_limits<std::uint32_t>::max()
        && object.m_nodeId == 76u && object.m_elementParsed
        && object.m_elementName == "FlipHinge"
        && object.m_actionCount == 11
        && object.m_actionIndexes == std::vector<std::int32_t>{
            79, 65, 66, 67, 68, 69, 1, 8, 31, 46, 54}
        && object.m_dependencyCount == 27
        && object.m_dependencyHandles == dependencies
        && object.m_displayLocation.x == 35.0
        && object.m_displayLocation.y == -15.0
        && object.m_displayLocation.z == 0.0
        && object.m_connectionCodes
            == std::vector<std::int32_t>{26, 26, 26, 26}
        && object.m_connectionNames == std::vector<UTF8STRING>{
            "Flip", "UpdatedFlip", "UpdatedBase", "UpdatedEnd"};
}

bool flipActionWireReadback(const std::string& bytes, bool binary) {
    std::istringstream input(bytes);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(std::make_unique<dxfReaderBinary>(&input))
        : std::unique_ptr<dxfReader>(std::make_unique<dxfReaderAscii>(&input));
    int code = 0;
    if (!reader->readRec(&code) || code != 0
        || reader->getString() != "BLOCKFLIPACTION")
        return false;
    bool ended = false;
    bool hasHandle = false;
    bool hasNode = false;
    bool hasActionCount = false;
    bool hasDependencyCount = false;
    bool hasSubclass = false;
    std::uint32_t outputHandle = 0u;
    std::vector<std::uint32_t> ownerAndDependencies;
    std::vector<std::int32_t> actionIndexes;
    std::vector<std::int32_t> connectionCodes;
    std::vector<std::string> connectionNames;
    std::vector<double> location;
    while (reader->readRec(&code)) {
        if (code == 0) {
            ended = reader->getString() == "WIRE_END";
            break;
        }
        switch (code) {
        case 5:
            outputHandle = reader->getHandleString();
            hasHandle = outputHandle == 0x6133u;
            break;
        case 330:
            ownerAndDependencies.push_back(reader->getHandleString());
            break;
        case 70:
            hasActionCount = reader->getInt32() == 11;
            break;
        case 71:
            hasDependencyCount = reader->getInt32() == 27;
            break;
        case 90:
            hasNode = reader->getInt32() == 76;
            break;
        case 91:
            actionIndexes.push_back(reader->getInt32());
            break;
        case 92:
        case 93:
        case 94:
        case 95:
            connectionCodes.push_back(reader->getInt32());
            break;
        case 1010:
        case 1020:
        case 1030:
            location.push_back(reader->getDouble());
            break;
        case 100:
            hasSubclass = hasSubclass
                || reader->getString() == "AcDbBlockFlipAction";
            break;
        case 301:
        case 302:
        case 303:
        case 304:
            connectionNames.push_back(reader->getString());
            break;
        default:
            break;
        }
    }
    std::vector<std::uint32_t> expectedHandles{0x110Fu};
    for (std::uint32_t index = 0u; index < 27u; ++index)
        expectedHandles.push_back(0x6127u + index);
    const bool matches = ended && hasHandle && hasNode && hasActionCount
        && hasDependencyCount && hasSubclass
        && ownerAndDependencies == expectedHandles
        && actionIndexes == std::vector<std::int32_t>{
            79, 65, 66, 67, 68, 69, 1, 8, 31, 46, 54}
        && connectionCodes == std::vector<std::int32_t>{26, 26, 26, 26}
        && connectionNames == std::vector<std::string>{
            "Flip", "UpdatedFlip", "UpdatedBase", "UpdatedEnd"}
        && location == std::vector<double>{35.0, -15.0, 0.0};
    if (!matches)
        std::cerr << "FlipAction readback mismatch: ended=" << ended
                  << " handle=" << hasHandle << " node=" << hasNode
                  << " outputHandle=" << outputHandle
                  << " actionCount=" << hasActionCount
                  << " dependencyCount=" << hasDependencyCount
                  << " subclass=" << hasSubclass
                  << " owner/dependencies=" << ownerAndDependencies.size()
                  << " actionIndexes=" << actionIndexes.size()
                  << " connectionCodes=" << connectionCodes.size()
                  << " connectionNames=" << connectionNames.size()
                  << " location=" << location.size() << '\n';
    return matches;
}

bool flipActionAllocationSweep(bool binary) {
    return dynamicBlockAllocationSweep(
        "FlipAction", binary, makeFlipAction,
        flipActionSourceUnchanged, flipActionWireReadback,
        flipActionRemap());
}

DRW_DynamicBlockObject makeLookupAction() {
    DRW_DynamicBlockObject object("BLOCKLOOKUPACTION");
    object.handle = 0xE351u;
    object.parentHandle = 0xE352u;
    object.m_kind = DRW_DynamicBlockObject::Kind::LookupAction;
    object.m_bodyFullyDecoded = true;
    object.m_evalExprParsed = true;
    object.m_parentId = std::numeric_limits<std::uint32_t>::max();
    object.m_major = 29u;
    object.m_minor = 2u;
    object.m_valueCode = -9999;
    object.m_nodeId = 0xE353u;
    object.m_elementParsed = true;
    object.m_elementName = "LookupTable";
    object.m_elementMajor = 29u;
    object.m_elementMinor = 2u;
    object.m_eed1071 = 7;
    object.m_actionCount = 2;
    object.m_actionIndexes = {4, 9};
    object.m_dependencyCount = 2;
    object.m_dependencyHandles = {0xE360u, 0xE361u};
    object.m_displayLocation = DRW_Coord(8.5, -3.25, 0.0);
    object.m_lookupRowCount = 2;
    object.m_lookupColumnCount = 2;
    object.m_lookupExpressions = {"A=1", "B=2", "A=3", "B=4"};
    object.m_lookupConnectionCodes = {{1, 2, 3}, {4, 5, 6}};
    object.m_lookupConnectionNames = {
        {"UnmatchedA", "ConnectionA"},
        {"UnmatchedB", "ConnectionB"}};
    object.m_lookupEntryFlags282 = {true, false};
    object.m_lookupEntryFlags281 = {false, true};
    object.m_lookupFlag280 = true;
    return object;
}

std::map<std::uint32_t, std::uint32_t> lookupActionRemap() {
    return {{0xE360u, 0x6E60u}, {0xE361u, 0x6E61u}};
}

bool lookupActionSourceUnchanged(const DRW_DynamicBlockObject& object) {
    return object.handle == 0xE351u
        && static_cast<std::uint32_t>(object.parentHandle) == 0xE352u
        && object.m_kind == DRW_DynamicBlockObject::Kind::LookupAction
        && object.m_bodyFullyDecoded && object.m_evalExprParsed
        && object.m_parentId == std::numeric_limits<std::uint32_t>::max()
        && object.m_nodeId == 0xE353u && object.m_elementParsed
        && object.m_elementName == "LookupTable"
        && object.m_actionCount == 2
        && object.m_actionIndexes == std::vector<std::int32_t>{4, 9}
        && object.m_dependencyCount == 2
        && object.m_dependencyHandles
            == std::vector<std::uint32_t>{0xE360u, 0xE361u}
        && object.m_lookupRowCount == 2 && object.m_lookupColumnCount == 2
        && object.m_lookupExpressions
            == std::vector<UTF8STRING>{"A=1", "B=2", "A=3", "B=4"}
        && object.m_lookupConnectionCodes
            == std::vector<std::vector<std::int32_t>>{{1, 2, 3}, {4, 5, 6}}
        && object.m_lookupConnectionNames
            == std::vector<std::vector<UTF8STRING>>{
                {"UnmatchedA", "ConnectionA"},
                {"UnmatchedB", "ConnectionB"}}
        && object.m_lookupEntryFlags282 == std::vector<bool>{true, false}
        && object.m_lookupEntryFlags281 == std::vector<bool>{false, true}
        && object.m_lookupFlag280;
}

bool lookupActionWireReadback(const std::string& bytes, bool binary) {
    std::istringstream input(bytes);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(std::make_unique<dxfReaderBinary>(&input))
        : std::unique_ptr<dxfReader>(std::make_unique<dxfReaderAscii>(&input));
    int code = 0;
    if (!reader->readRec(&code) || code != 0
        || reader->getString() != "BLOCKLOOKUPACTION")
        return false;
    bool ended = false;
    bool hasHandle = false;
    bool hasRowCount = false;
    bool hasColumnCount = false;
    bool hasActionCount = false;
    bool hasDependencyCount = false;
    bool hasLookupFlag = false;
    bool hasSubclass = false;
    std::vector<std::uint32_t> ownerAndDependencies;
    std::vector<std::int32_t> actionIndexes;
    std::vector<std::string> expressions;
    std::vector<std::int32_t> connectionCodes;
    std::vector<std::string> connectionNames;
    std::vector<std::int32_t> flags282;
    std::vector<std::int32_t> flags281;
    while (reader->readRec(&code)) {
        if (code == 0) {
            ended = reader->getString() == "WIRE_END";
            break;
        }
        switch (code) {
        case 5:
            hasHandle = reader->getHandleString() == 0xE351u;
            break;
        case 330:
            ownerAndDependencies.push_back(reader->getHandleString());
            break;
        case 70:
            hasActionCount = reader->getInt32() == 2;
            break;
        case 71:
            hasDependencyCount = reader->getInt32() == 2;
            break;
        case 90:
            break;
        case 91:
            actionIndexes.push_back(reader->getInt32());
            break;
        case 92:
            hasRowCount = reader->getInt32() == 2;
            break;
        case 93:
            hasColumnCount = reader->getInt32() == 2;
            break;
        case 94:
        case 95:
        case 96:
            connectionCodes.push_back(reader->getInt32());
            break;
        case 280:
            hasLookupFlag = reader->getInt32() == 1;
            break;
        case 281:
            flags281.push_back(reader->getInt32());
            break;
        case 282:
            flags282.push_back(reader->getInt32());
            break;
        case 100:
            hasSubclass = hasSubclass
                || reader->getString() == "AcDbBlockLookupAction";
            break;
        case 302:
            expressions.push_back(reader->getString());
            break;
        case 304:
        case 305:
            connectionNames.push_back(reader->getString());
            break;
        default:
            break;
        }
    }
    return ended && hasHandle && hasRowCount && hasColumnCount
        && hasActionCount && hasDependencyCount && hasLookupFlag && hasSubclass
        && ownerAndDependencies
            == std::vector<std::uint32_t>{0xE352u, 0x6E60u, 0x6E61u}
        && actionIndexes == std::vector<std::int32_t>{4, 9}
        && expressions == std::vector<std::string>{"A=1", "B=2", "A=3", "B=4"}
        && connectionCodes == std::vector<std::int32_t>{1, 2, 3, 4, 5, 6}
        && connectionNames == std::vector<std::string>{
            "UnmatchedA", "ConnectionA", "UnmatchedB", "ConnectionB"}
        && flags282 == std::vector<std::int32_t>{1, 0}
        && flags281 == std::vector<std::int32_t>{0, 1};
}

bool lookupActionAllocationSweep(bool binary) {
    return dynamicBlockAllocationSweep(
        "LookupAction", binary, makeLookupAction,
        lookupActionSourceUnchanged, lookupActionWireReadback,
        lookupActionRemap());
}

std::map<std::uint32_t, std::uint32_t> stretchActionRemap() {
    return {{0x1127u, 0x6127u}, {0x1143u, 0x6143u},
            {0x1144u, 0x6144u}};
}

bool stretchActionSourceUnchanged(const DRW_DynamicBlockObject& object) {
    return object.handle == 0x1136u
        && static_cast<std::uint32_t>(object.parentHandle) == 0x110Fu
        && object.m_kind == DRW_DynamicBlockObject::Kind::StretchAction
        && object.m_bodyFullyDecoded && object.m_evalExprParsed
        && object.m_parentId == std::numeric_limits<std::uint32_t>::max()
        && object.m_nodeId == 90u && object.m_elementParsed
        && object.m_elementName == "Stretch0"
        && object.m_actionCount == 2
        && object.m_actionIndexes == std::vector<std::int32_t>{17, 24}
        && object.m_dependencyCount == 1
        && object.m_dependencyHandles == std::vector<std::uint32_t>{0x1127u}
        && object.m_connectionCodes == std::vector<std::int32_t>{1, 8}
        && object.m_connectionNames
            == std::vector<UTF8STRING>{"EndXDelta", "EndYDelta"}
        && object.m_stretchPointCount == 2
        && object.m_stretchPoints.size() == 2u
        && object.m_stretchPoints[0].x == 34.5
        && object.m_stretchPoints[0].y == 4.5
        && object.m_stretchPoints[1].x == 26.0
        && object.m_stretchPoints[1].y == -6.5
        && object.m_handleReferenceCount == 2
        && object.m_handleReferenceHandles
            == std::vector<std::uint32_t>{0x1143u, 0x1144u}
        && object.m_handleReferenceIndexCounts
            == std::vector<std::int32_t>{2, 1}
        && object.m_handleReferenceIndexes
            == std::vector<std::vector<std::int32_t>>{{1, 2}, {0}}
        && object.m_codeReferenceCount == 1
        && object.m_codeReferenceCodes == std::vector<std::int32_t>{31}
        && object.m_codeReferenceIndexCounts == std::vector<std::int32_t>{2}
        && object.m_codeReferenceIndexes
            == std::vector<std::vector<std::int32_t>>{{1, 2}}
        && object.m_actionOffsetX == 1.0 && object.m_actionOffsetY == 0.0;
}

bool stretchActionWireReadback(const std::string& bytes, bool binary) {
    std::istringstream input(bytes);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(std::make_unique<dxfReaderBinary>(&input))
        : std::unique_ptr<dxfReader>(std::make_unique<dxfReaderAscii>(&input));
    int code = 0;
    if (!reader->readRec(&code) || code != 0
        || reader->getString() != "BLOCKSTRETCHACTION")
        return false;
    bool hasHandle = false;
    bool hasNode = false;
    bool hasActionCount = false;
    bool hasDependencyCount = false;
    bool hasStretchSubclass = false;
    bool ended = false;
    std::vector<std::uint32_t> ownerAndDependencies;
    std::vector<std::uint32_t> handleReferences;
    std::vector<std::int32_t> actionIndexes;
    std::vector<std::int32_t> connections;
    std::vector<std::string> connectionNames;
    std::vector<std::int32_t> referenceCounts;
    std::vector<std::int32_t> referenceIndexes;
    std::vector<std::int32_t> codeReferences;
    std::vector<double> stretchPointXY;
    std::int32_t stretchPointCount = -1;
    std::int32_t handleReferenceCount = -1;
    std::int32_t codeReferenceCount = -1;
    bool hasOffsets = false;
    while (reader->readRec(&code)) {
        if (code == 0) {
            ended = reader->getString() == "WIRE_END";
            break;
        }
        switch (code) {
        case 5:
            hasHandle = reader->getHandleString() == 0x1136u;
            break;
        case 330:
            ownerAndDependencies.push_back(reader->getHandleString());
            break;
        case 331:
            handleReferences.push_back(reader->getHandleString());
            break;
        case 90:
            hasNode = reader->getInt32() == 90;
            break;
        case 70:
            hasActionCount = reader->getInt32() == 2;
            break;
        case 71:
            hasDependencyCount = reader->getInt32() == 1;
            break;
        case 72:
            stretchPointCount = reader->getInt32();
            break;
        case 73:
            handleReferenceCount = reader->getInt32();
            break;
        case 74:
        case 76:
            referenceCounts.push_back(reader->getInt32());
            break;
        case 91:
            actionIndexes.push_back(reader->getInt32());
            break;
        case 92:
        case 93:
            connections.push_back(reader->getInt32());
            break;
        case 94:
            referenceIndexes.push_back(reader->getInt32());
            break;
        case 95:
            codeReferences.push_back(reader->getInt32());
            break;
        case 75:
            codeReferenceCount = reader->getInt32();
            break;
        case 1011:
        case 1021:
            stretchPointXY.push_back(reader->getDouble());
            break;
        case 140:
            hasOffsets = reader->getDouble() == 1.0;
            break;
        case 141:
            hasOffsets = hasOffsets && reader->getDouble() == 0.0;
            break;
        case 280:
            if (reader->getInt32() != 1)
                return false;
            break;
        case 100:
            hasStretchSubclass = hasStretchSubclass
                || reader->getString() == "AcDbBlockStretchAction";
            break;
        case 301:
        case 302:
            connectionNames.push_back(reader->getString());
            break;
        default:
            break;
        }
    }
    const bool ownerRoles = ownerAndDependencies
        == std::vector<std::uint32_t>{0x110Fu, 0x6127u};
    const bool remappedHandleRoles = handleReferences
        == std::vector<std::uint32_t>{0x6143u, 0x6144u};
    const bool references = referenceCounts
        == std::vector<std::int32_t>{2, 1, 2}
        && referenceIndexes
            == std::vector<std::int32_t>{1, 2, 0, 1, 2}
        && codeReferences == std::vector<std::int32_t>{31};
    return ended && hasHandle && hasNode && hasActionCount
        && hasDependencyCount && hasStretchSubclass && ownerRoles
        && remappedHandleRoles
        && actionIndexes == std::vector<std::int32_t>{17, 24}
        && connections == std::vector<std::int32_t>{1, 8}
        && connectionNames
            == std::vector<std::string>{"EndXDelta", "EndYDelta"}
        && stretchPointCount == 2 && handleReferenceCount == 2
        && codeReferenceCount == 1 && references
        && stretchPointXY == std::vector<double>{34.5, 4.5, 26.0, -6.5}
        && hasOffsets;
}

bool stretchActionAllocationSweep(bool binary) {
    return dynamicBlockAllocationSweep(
        "StretchAction", binary, makeStretchAction,
        stretchActionSourceUnchanged, stretchActionWireReadback,
        stretchActionRemap());
}

DRW_DynamicBlockObject makeFlipParameter() {
    DRW_DynamicBlockObject object("BLOCKFLIPPARAMETER");
    object.handle = 0x1135u;
    object.parentHandle = 0x110Fu;
    object.m_kind = DRW_DynamicBlockObject::Kind::FlipParameter;
    object.m_bodyFullyDecoded = true;
    object.m_evalExprParsed = true;
    object.m_parentId = std::numeric_limits<std::uint32_t>::max();
    object.m_major = object.m_elementMajor = 29u;
    object.m_minor = object.m_elementMinor = 2u;
    object.m_valueCode = -9999;
    object.m_nodeId = 54u;
    object.m_elementParsed = true;
    object.m_elementName = "Door Size";
    object.m_showProperties = true;
    object.m_firstPoint = DRW_Coord(1.25, -2.5, 3.75);
    object.m_secondPoint = DRW_Coord(15.0, 4.5, -6.75);
    object.m_propertyStates = {55, 0, 7, -1};
    object.m_twoPointPropertyCodes[0] = {55, -4};
    object.m_twoPointPropertyNames[0] = {"Distance", "Width"};
    object.m_twoPointPropertyCodes[1] = {92};
    object.m_twoPointPropertyNames[1] = {"Angle"};
    object.m_twoPointPropertyCodes[3] = {301, 0};
    object.m_twoPointPropertyNames[3] = {"Position", "Enabled"};
    object.m_parameterBaseLocation = 2;
    object.m_flipCaption = "Flip";
    object.m_flipDescription = "Flip the door";
    object.m_flipBaseStateName = "Base";
    object.m_flipFlippedStateName = "Flipped";
    object.m_flipCaptionLocation = DRW_Coord(2.0, 3.0, 4.0);
    object.m_flipValue96 = 1;
    object.m_flipTooltip = "Toggle door handing";
    return object;
}

bool flipParameterSourceUnchanged(const DRW_DynamicBlockObject& object) {
    return object.handle == 0x1135u
        && static_cast<std::uint32_t>(object.parentHandle) == 0x110Fu
        && object.m_kind == DRW_DynamicBlockObject::Kind::FlipParameter
        && object.m_bodyFullyDecoded && object.m_evalExprParsed
        && object.m_parentId == std::numeric_limits<std::uint32_t>::max()
        && object.m_major == 29u && object.m_minor == 2u
        && object.m_valueCode == -9999 && object.m_nodeId == 54u
        && object.m_elementParsed && object.m_elementName == "Door Size"
        && object.m_showProperties && !object.m_chainActions
        && object.m_firstPoint.x == 1.25 && object.m_firstPoint.y == -2.5
        && object.m_firstPoint.z == 3.75
        && object.m_secondPoint.x == 15.0 && object.m_secondPoint.y == 4.5
        && object.m_secondPoint.z == -6.75
        && object.m_propertyStates == std::array<std::int32_t, 4>{55, 0, 7, -1}
        && object.m_twoPointPropertyCodes[0]
            == std::vector<std::int32_t>{55, -4}
        && object.m_twoPointPropertyNames[0]
            == std::vector<UTF8STRING>{"Distance", "Width"}
        && object.m_twoPointPropertyCodes[1]
            == std::vector<std::int32_t>{92}
        && object.m_twoPointPropertyNames[1]
            == std::vector<UTF8STRING>{"Angle"}
        && object.m_twoPointPropertyCodes[3]
            == std::vector<std::int32_t>{301, 0}
        && object.m_twoPointPropertyNames[3]
            == std::vector<UTF8STRING>{"Position", "Enabled"}
        && object.m_parameterBaseLocation == 2
        && object.m_flipCaption == "Flip"
        && object.m_flipDescription == "Flip the door"
        && object.m_flipBaseStateName == "Base"
        && object.m_flipFlippedStateName == "Flipped"
        && object.m_flipCaptionLocation.x == 2.0
        && object.m_flipCaptionLocation.y == 3.0
        && object.m_flipCaptionLocation.z == 4.0
        && object.m_flipValue96 == 1
        && object.m_flipTooltip == "Toggle door handing";
}

bool flipParameterWireReadback(const std::string& bytes, bool binary) {
    std::istringstream input(bytes);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(std::make_unique<dxfReaderBinary>(&input))
        : std::unique_ptr<dxfReader>(std::make_unique<dxfReaderAscii>(&input));
    int code = 0;
    if (!reader->readRec(&code) || code != 0
        || reader->getString() != "BLOCKFLIPPARAMETER")
        return false;
    bool ended = false;
    bool hasHandle = false;
    bool hasNode = false;
    bool hasElement = false;
    bool hasShow = false;
    bool hasChain = false;
    bool hasPropertyCount = false;
    bool hasCaption = false;
    bool hasDescription = false;
    bool hasBaseState = false;
    bool hasFlippedState = false;
    bool hasCaptionValue = false;
    bool hasTooltip = false;
    bool hasSubclass = false;
    std::uint32_t owner = 0u;
    std::vector<std::int32_t> propertyStates;
    std::vector<std::int32_t> propertyCounts;
    std::vector<std::int32_t> propertyCodes;
    std::vector<std::string> propertyNames;
    std::vector<double> points;
    std::vector<double> captionLocation;
    while (reader->readRec(&code)) {
        if (code == 0) {
            ended = reader->getString() == "WIRE_END";
            break;
        }
        switch (code) {
        case 5: hasHandle = reader->getHandleString() == 0x1135u; break;
        case 330: owner = reader->getHandleString(); break;
        case 90: hasNode = reader->getInt32() == 54; break;
        case 300: hasElement = reader->getString() == "Door Size"; break;
        case 170: hasPropertyCount = reader->getInt32() == 4; break;
        case 171: case 172: case 173: case 174:
            propertyCounts.push_back(reader->getInt32()); break;
        case 91: propertyStates.push_back(reader->getInt32()); break;
        case 92: case 93: case 94: case 95:
            propertyCodes.push_back(reader->getInt32()); break;
        case 1010: case 1020: case 1030: case 1011: case 1021: case 1031:
            points.push_back(reader->getDouble()); break;
        case 1012: case 1022: case 1032:
            captionLocation.push_back(reader->getDouble()); break;
        case 280: hasShow = reader->getInt32() == 1; break;
        case 281: hasChain = reader->getInt32() == 0; break;
        case 305: hasCaption = reader->getString() == "Flip"; break;
        case 306: hasDescription = reader->getString() == "Flip the door"; break;
        case 307: hasBaseState = reader->getString() == "Base"; break;
        case 308: hasFlippedState = reader->getString() == "Flipped"; break;
        case 309: hasTooltip = reader->getString() == "Toggle door handing"; break;
        case 96: hasCaptionValue = reader->getInt32() == 1; break;
        case 100:
            hasSubclass = hasSubclass
                || reader->getString() == "AcDbBlockFlipParameter";
            break;
        case 301: case 302: case 303: case 304:
            propertyNames.push_back(reader->getString()); break;
        default: break;
        }
    }
    return ended && hasHandle && owner == 0x110Fu && hasNode && hasElement
        && hasShow && hasChain && hasPropertyCount && hasCaption
        && hasDescription && hasBaseState && hasFlippedState
        && hasCaptionValue && hasTooltip && hasSubclass
        && propertyStates == std::vector<std::int32_t>{55, 0, 7, -1}
        && propertyCounts == std::vector<std::int32_t>{2, 1, 0, 2}
        && propertyCodes == std::vector<std::int32_t>{55, -4, 92, 301, 0}
        && propertyNames == std::vector<std::string>{
            "Distance", "Width", "Angle", "Position", "Enabled"}
        && points == std::vector<double>{1.25, -2.5, 3.75, 15.0, 4.5, -6.75}
        && captionLocation == std::vector<double>{2.0, 3.0, 4.0};
}

bool flipParameterAllocationSweep(bool binary) {
    return dynamicBlockAllocationSweep(
        "FlipParameter", binary, makeFlipParameter,
        flipParameterSourceUnchanged, flipParameterWireReadback, {});
}

DRW_DynamicBlockObject makeLinearParameter() {
    DRW_DynamicBlockObject object("BLOCKLINEARPARAMETER");
    object.handle = 0x1135u;
    object.parentHandle = 0x110Fu;
    object.m_kind = DRW_DynamicBlockObject::Kind::LinearParameter;
    object.m_bodyFullyDecoded = true;
    object.m_evalExprParsed = true;
    object.m_parentId = std::numeric_limits<std::uint32_t>::max();
    object.m_major = object.m_elementMajor = 29u;
    object.m_minor = object.m_elementMinor = 2u;
    object.m_valueCode = -9999;
    object.m_nodeId = 54u;
    object.m_elementParsed = true;
    object.m_elementName = "Door Size";
    object.m_showProperties = true;
    object.m_firstPoint = DRW_Coord(1.25, -2.5, 3.75);
    object.m_secondPoint = DRW_Coord(15.0, 4.5, -6.75);
    object.m_propertyStates = {55, 0, 7, -1};
    object.m_twoPointPropertyCodes[0] = {55, -4};
    object.m_twoPointPropertyNames[0] = {"Distance", "Width"};
    object.m_twoPointPropertyCodes[1] = {92};
    object.m_twoPointPropertyNames[1] = {"Angle"};
    object.m_twoPointPropertyCodes[3] = {301, 0};
    object.m_twoPointPropertyNames[3] = {"Position", "Enabled"};
    object.m_parameterBaseLocation = 2;
    object.m_linearDistanceName = "Door Width";
    object.m_linearDistanceDescription = "Opening width";
    object.m_linearDistance = 36.0;
    object.m_parameterValueDescription = "Distance value";
    object.m_linearValue96 = 8;
    object.m_linearValue141 = 24.0;
    object.m_linearValue142 = 48.0;
    object.m_linearValue143 = 2.0;
    object.m_linearValues = {24.0, 36.0, 48.0};
    object.m_linearDeclaredValueCount = 3;
    return object;
}

bool linearParameterSourceUnchanged(const DRW_DynamicBlockObject& object) {
    return object.handle == 0x1135u
        && static_cast<std::uint32_t>(object.parentHandle) == 0x110Fu
        && object.m_kind == DRW_DynamicBlockObject::Kind::LinearParameter
        && object.m_bodyFullyDecoded && object.m_evalExprParsed
        && object.m_parentId == std::numeric_limits<std::uint32_t>::max()
        && object.m_major == 29u && object.m_minor == 2u
        && object.m_valueCode == -9999 && object.m_nodeId == 54u
        && object.m_elementParsed && object.m_elementName == "Door Size"
        && object.m_showProperties && !object.m_chainActions
        && object.m_firstPoint.x == 1.25 && object.m_firstPoint.y == -2.5
        && object.m_firstPoint.z == 3.75
        && object.m_secondPoint.x == 15.0 && object.m_secondPoint.y == 4.5
        && object.m_secondPoint.z == -6.75
        && object.m_propertyStates == std::array<std::int32_t, 4>{55, 0, 7, -1}
        && object.m_twoPointPropertyCodes[0]
            == std::vector<std::int32_t>{55, -4}
        && object.m_twoPointPropertyNames[0]
            == std::vector<UTF8STRING>{"Distance", "Width"}
        && object.m_twoPointPropertyCodes[1]
            == std::vector<std::int32_t>{92}
        && object.m_twoPointPropertyNames[1]
            == std::vector<UTF8STRING>{"Angle"}
        && object.m_twoPointPropertyCodes[3]
            == std::vector<std::int32_t>{301, 0}
        && object.m_twoPointPropertyNames[3]
            == std::vector<UTF8STRING>{"Position", "Enabled"}
        && object.m_parameterBaseLocation == 2
        && object.m_linearDistanceName == "Door Width"
        && object.m_linearDistanceDescription == "Opening width"
        && object.m_linearDistance == 36.0
        && object.m_parameterValueDescription == "Distance value"
        && object.m_linearValue96 == 8 && object.m_linearValue141 == 24.0
        && object.m_linearValue142 == 48.0 && object.m_linearValue143 == 2.0
        && object.m_linearValues == std::vector<double>{24.0, 36.0, 48.0}
        && object.m_linearDeclaredValueCount == 3;
}

bool linearParameterWireReadback(const std::string& bytes, bool binary) {
    std::istringstream input(bytes);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(std::make_unique<dxfReaderBinary>(&input))
        : std::unique_ptr<dxfReader>(std::make_unique<dxfReaderAscii>(&input));
    int code = 0;
    if (!reader->readRec(&code) || code != 0
        || reader->getString() != "BLOCKLINEARPARAMETER")
        return false;
    bool ended = false;
    bool hasHandle = false;
    bool hasNode = false;
    bool hasElement = false;
    bool hasShow = false;
    bool hasChain = false;
    bool hasPropertyCount = false;
    bool hasSubclass = false;
    bool hasDistanceName = false;
    bool hasDistanceDescription = false;
    bool hasDistance = false;
    bool hasParameterValueDescription = false;
    bool hasValue96 = false;
    bool hasCount = false;
    std::uint32_t owner = 0u;
    std::vector<std::int32_t> propertyStates;
    std::vector<std::int32_t> propertyCounts;
    std::vector<std::int32_t> propertyCodes;
    std::vector<std::string> propertyNames;
    std::vector<double> points;
    std::vector<double> values;
    while (reader->readRec(&code)) {
        if (code == 0) {
            ended = reader->getString() == "WIRE_END";
            break;
        }
        switch (code) {
        case 5: hasHandle = reader->getHandleString() == 0x1135u; break;
        case 330: owner = reader->getHandleString(); break;
        case 90: hasNode = reader->getInt32() == 54; break;
        case 300: hasElement = reader->getString() == "Door Size"; break;
        case 170: hasPropertyCount = reader->getInt32() == 4; break;
        case 171: case 172: case 173: case 174:
            propertyCounts.push_back(reader->getInt32()); break;
        case 91: propertyStates.push_back(reader->getInt32()); break;
        case 92: case 93: case 94: case 95:
            propertyCodes.push_back(reader->getInt32()); break;
        case 1010: case 1020: case 1030: case 1011: case 1021: case 1031:
            points.push_back(reader->getDouble()); break;
        case 140: hasDistance = reader->getDouble() == 36.0; break;
        case 141: values.push_back(reader->getDouble()); break;
        case 142: values.push_back(reader->getDouble()); break;
        case 143: values.push_back(reader->getDouble()); break;
        case 144: values.push_back(reader->getDouble()); break;
        case 175: hasCount = reader->getInt32() == 3; break;
        case 96: hasValue96 = reader->getInt32() == 8; break;
        case 280: hasShow = reader->getInt32() == 1; break;
        case 281: hasChain = reader->getInt32() == 0; break;
        case 100:
            hasSubclass = hasSubclass
                || reader->getString() == "AcDbBlockLinearParameter";
            break;
        case 301: case 302: case 303: case 304:
            propertyNames.push_back(reader->getString()); break;
        case 305: hasDistanceName = reader->getString() == "Door Width"; break;
        case 306: hasDistanceDescription = reader->getString() == "Opening width"; break;
        case 307: hasParameterValueDescription = reader->getString() == "Distance value"; break;
        default: break;
        }
    }
    return ended && hasHandle && owner == 0x110Fu && hasNode && hasElement
        && hasShow && hasChain && hasPropertyCount && hasSubclass
        && hasDistanceName && hasDistanceDescription && hasDistance
        && hasParameterValueDescription && hasValue96 && hasCount
        && propertyStates == std::vector<std::int32_t>{55, 0, 7, -1}
        && propertyCounts == std::vector<std::int32_t>{2, 1, 0, 2}
        && propertyCodes == std::vector<std::int32_t>{55, -4, 92, 301, 0}
        && propertyNames == std::vector<std::string>{
            "Distance", "Width", "Angle", "Position", "Enabled"}
        && points == std::vector<double>{1.25, -2.5, 3.75, 15.0, 4.5, -6.75}
        && values == std::vector<double>{24.0, 48.0, 2.0, 24.0, 36.0, 48.0};
}

bool linearParameterAllocationSweep(bool binary) {
    return dynamicBlockAllocationSweep(
        "LinearParameter", binary, makeLinearParameter,
        linearParameterSourceUnchanged, linearParameterWireReadback, {});
}

DRW_DynamicBlockObject makeConstraintParameter(
        DRW_DynamicBlockObject::Kind kind) {
    const bool horizontal = kind
        == DRW_DynamicBlockObject::Kind::HorizontalConstraintParameter;
    DRW_DynamicBlockObject object(horizontal
        ? "BLOCKHORIZONTALCONSTRAINTPARAMETER"
        : "BLOCKVERTICALCONSTRAINTPARAMETER");
    object.handle = 0x1135u;
    object.parentHandle = 0x110Fu;
    object.m_kind = kind;
    object.m_bodyFullyDecoded = true;
    object.m_evalExprParsed = true;
    object.m_parentId = std::numeric_limits<std::uint32_t>::max();
    object.m_major = object.m_elementMajor = 29u;
    object.m_minor = object.m_elementMinor = 2u;
    object.m_valueCode = -9999;
    object.m_nodeId = 54u;
    object.m_elementParsed = true;
    object.m_elementName = "Door Size";
    object.m_showProperties = true;
    object.m_firstPoint = DRW_Coord(1.25, -2.5, 3.75);
    object.m_secondPoint = DRW_Coord(15.0, 4.5, -6.75);
    object.m_propertyStates = {55, 0, 7, -1};
    object.m_twoPointPropertyCodes[0] = {55, -4};
    object.m_twoPointPropertyNames[0] = {"Distance", "Width"};
    object.m_twoPointPropertyCodes[1] = {92};
    object.m_twoPointPropertyNames[1] = {"Angle"};
    object.m_twoPointPropertyCodes[3] = {301, 0};
    object.m_twoPointPropertyNames[3] = {"Position", "Enabled"};
    object.m_parameterBaseLocation = 2;
    object.m_constraintDependencyHandle = 0x114Bu;
    object.m_constraintExpressionName = horizontal ? "W" : "H";
    object.m_constraintExpressionDescription = "constraint value";
    object.m_constraintValue = -15.25;
    object.m_parameterValueDescription = "constraint description";
    object.m_constraintValueSetFlags = 8;
    object.m_constraintMinimum = 4.0;
    object.m_constraintMaximum = 6.0;
    object.m_constraintIncrement = 1.0;
    object.m_constraintValues = {4.0, 5.0, 6.0};
    object.m_constraintDeclaredValueCount = 3;
    return object;
}

bool constraintParameterSourceUnchanged(
        const DRW_DynamicBlockObject& object,
        DRW_DynamicBlockObject::Kind kind) {
    const bool horizontal = kind
        == DRW_DynamicBlockObject::Kind::HorizontalConstraintParameter;
    return object.handle == 0x1135u
        && static_cast<std::uint32_t>(object.parentHandle) == 0x110Fu
        && object.m_kind == kind && object.m_bodyFullyDecoded
        && object.m_evalExprParsed
        && object.m_parentId == std::numeric_limits<std::uint32_t>::max()
        && object.m_major == 29u && object.m_minor == 2u
        && object.m_valueCode == -9999 && object.m_nodeId == 54u
        && object.m_elementParsed && object.m_elementName == "Door Size"
        && object.m_showProperties && !object.m_chainActions
        && object.m_firstPoint.x == 1.25 && object.m_firstPoint.y == -2.5
        && object.m_firstPoint.z == 3.75
        && object.m_secondPoint.x == 15.0 && object.m_secondPoint.y == 4.5
        && object.m_secondPoint.z == -6.75
        && object.m_propertyStates == std::array<std::int32_t, 4>{55, 0, 7, -1}
        && object.m_twoPointPropertyCodes[0]
            == std::vector<std::int32_t>{55, -4}
        && object.m_twoPointPropertyNames[0]
            == std::vector<UTF8STRING>{"Distance", "Width"}
        && object.m_twoPointPropertyCodes[1]
            == std::vector<std::int32_t>{92}
        && object.m_twoPointPropertyNames[1]
            == std::vector<UTF8STRING>{"Angle"}
        && object.m_twoPointPropertyCodes[3]
            == std::vector<std::int32_t>{301, 0}
        && object.m_twoPointPropertyNames[3]
            == std::vector<UTF8STRING>{"Position", "Enabled"}
        && object.m_parameterBaseLocation == 2
        && object.m_constraintDependencyHandle == 0x114Bu
        && object.m_constraintExpressionName == (horizontal ? "W" : "H")
        && object.m_constraintExpressionDescription == "constraint value"
        && object.m_constraintValue == -15.25
        && object.m_parameterValueDescription == "constraint description"
        && object.m_constraintValueSetFlags == 8
        && object.m_constraintMinimum == 4.0
        && object.m_constraintMaximum == 6.0
        && object.m_constraintIncrement == 1.0
        && object.m_constraintValues == std::vector<double>{4.0, 5.0, 6.0}
        && object.m_constraintDeclaredValueCount == 3;
}

bool constraintParameterWireReadback(const std::string& bytes, bool binary,
                                     DRW_DynamicBlockObject::Kind kind) {
    const bool horizontal = kind
        == DRW_DynamicBlockObject::Kind::HorizontalConstraintParameter;
    const std::string recordName = horizontal
        ? "BLOCKHORIZONTALCONSTRAINTPARAMETER"
        : "BLOCKVERTICALCONSTRAINTPARAMETER";
    const std::string finalSubclass = horizontal
        ? "AcDbBlockHorizontalConstraintParameter"
        : "AcDbBlockVerticalConstraintParameter";
    std::istringstream input(bytes);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(std::make_unique<dxfReaderBinary>(&input))
        : std::unique_ptr<dxfReader>(std::make_unique<dxfReaderAscii>(&input));
    int code = 0;
    if (!reader->readRec(&code) || code != 0
        || reader->getString() != recordName)
        return false;
    bool ended = false;
    bool hasHandle = false;
    bool hasNode = false;
    bool hasElement = false;
    bool hasShow = false;
    bool hasChain = false;
    bool hasPropertyCount = false;
    bool hasExpression = false;
    bool hasExpressionDescription = false;
    bool hasValue = false;
    bool hasParameterDescription = false;
    bool hasFlags = false;
    bool hasCount = false;
    bool hasFinalSubclass = false;
    std::vector<std::uint32_t> handles;
    std::vector<std::int32_t> propertyStates;
    std::vector<std::int32_t> propertyCounts;
    std::vector<std::int32_t> propertyCodes;
    std::vector<std::string> propertyNames;
    std::vector<double> points;
    std::vector<double> constraintValues;
    while (reader->readRec(&code)) {
        if (code == 0) {
            ended = reader->getString() == "WIRE_END";
            break;
        }
        switch (code) {
        case 5: hasHandle = reader->getHandleString() == 0x1135u; break;
        case 330: handles.push_back(reader->getHandleString()); break;
        case 90: hasNode = reader->getInt32() == 54; break;
        case 300: hasElement = reader->getString() == "Door Size"; break;
        case 170: hasPropertyCount = reader->getInt32() == 4; break;
        case 171: case 172: case 173: case 174:
            propertyCounts.push_back(reader->getInt32()); break;
        case 91: propertyStates.push_back(reader->getInt32()); break;
        case 92: case 93: case 94: case 95:
            propertyCodes.push_back(reader->getInt32()); break;
        case 1010: case 1020: case 1030: case 1011: case 1021: case 1031:
            points.push_back(reader->getDouble()); break;
        case 140: hasValue = reader->getDouble() == -15.25; break;
        case 96: hasFlags = reader->getInt32() == 8; break;
        case 128: constraintValues.push_back(reader->getDouble()); break;
        case 129: constraintValues.push_back(reader->getDouble()); break;
        case 130: constraintValues.push_back(reader->getDouble()); break;
        case 131: constraintValues.push_back(reader->getDouble()); break;
        case 175: hasCount = reader->getInt32() == 3; break;
        case 100:
            {
                const std::string subclass = reader->getString();
                hasFinalSubclass = hasFinalSubclass
                    || subclass == finalSubclass;
            }
            break;
        case 280: hasShow = reader->getInt32() == 1; break;
        case 281: hasChain = reader->getInt32() == 0; break;
        case 301: case 302: case 303: case 304:
            propertyNames.push_back(reader->getString()); break;
        case 305: hasExpression = reader->getString()
            == (horizontal ? "W" : "H"); break;
        case 306: hasExpressionDescription = reader->getString()
            == "constraint value"; break;
        case 307: hasParameterDescription = reader->getString()
            == "constraint description"; break;
        default: break;
        }
    }
    return ended && hasHandle && handles
            == std::vector<std::uint32_t>{0x110Fu, 0x614Bu}
        && hasNode && hasElement && hasShow && hasChain && hasPropertyCount
        && hasExpression && hasExpressionDescription && hasValue
        && hasParameterDescription && hasFlags && hasCount && hasFinalSubclass
        && propertyStates == std::vector<std::int32_t>{55, 0, 7, -1}
        && propertyCounts == std::vector<std::int32_t>{2, 1, 0, 2}
        && propertyCodes == std::vector<std::int32_t>{55, -4, 92, 301, 0}
        && propertyNames == std::vector<std::string>{
            "Distance", "Width", "Angle", "Position", "Enabled"}
        && points == std::vector<double>{1.25, -2.5, 3.75, 15.0, 4.5, -6.75}
        && constraintValues == std::vector<double>{4.0, 6.0, 1.0, 4.0, 5.0, 6.0};
}

bool constraintParameterAllocationSweep(
        DRW_DynamicBlockObject::Kind kind, bool binary) {
    const bool horizontal = kind
        == DRW_DynamicBlockObject::Kind::HorizontalConstraintParameter;
    const auto make = [kind]() { return makeConstraintParameter(kind); };
    const auto sourceCheck = [kind](const DRW_DynamicBlockObject& object) {
        return constraintParameterSourceUnchanged(object, kind);
    };
    const auto wireCheck = [kind](const std::string& bytes, bool isBinary) {
        return constraintParameterWireReadback(bytes, isBinary, kind);
    };
    return dynamicBlockAllocationSweep(
        horizontal ? "HorizontalConstraintParameter"
                   : "VerticalConstraintParameter",
        binary, make, sourceCheck, wireCheck, {{0x114Bu, 0x614Bu}});
}

bool alignmentAllocationSweep(bool binary) {
    return dynamicBlockAllocationSweep(
        "AlignmentParameter", binary, makeAlignmentParameter,
        alignmentSourceUnchanged, alignmentWireReadback, {});
}

bool wireReadback(const std::string& bytes, bool binary) {
    std::istringstream input(bytes);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(std::make_unique<dxfReaderBinary>(&input))
        : std::unique_ptr<dxfReader>(std::make_unique<dxfReaderAscii>(&input));
    int code = 0;
    if (!reader->readRec(&code) || code != 0
        || reader->getString() != "ACDB_DYNAMICBLOCKPURGEPREVENTER_VERSION")
        return false;
    bool ended = false;
    bool hasHandle = false;
    bool hasSubclass = false;
    bool hasFlag = false;
    std::vector<std::uint32_t> owners;
    while (reader->readRec(&code)) {
        if (code == 0) {
            ended = reader->getString() == "WIRE_END";
            break;
        }
        switch (code) {
        case 5:
            hasHandle = reader->getHandleString() == 0xE702u;
            break;
        case 102:
            {
                const std::string marker = reader->getString();
                if (marker != "{ACAD_REACTORS" && marker != "}")
                    return false;
            }
            break;
        case 330:
            owners.push_back(reader->getHandleString());
            break;
        case 100:
            hasSubclass = reader->getString()
                == "AcDbDynamicBlockPurgePreventer";
            break;
        case 70:
            hasFlag = reader->getInt32() == 1;
            break;
        default:
            return false;
        }
    }
    return ended && hasHandle && hasSubclass && hasFlag
        && owners == std::vector<std::uint32_t>{0xE701u, 0xE701u};
}

bool sourceUnchanged(const DRW_DynamicBlockObject& object) {
    return object.handle == 0xE702u
        && static_cast<std::uint32_t>(object.parentHandle) == 0xE701u
        && object.reactorHandles == std::vector<std::uint32_t>{0xE701u}
        && object.m_purgeFlag == 1u
        && object.m_purgeBlockHandle == 0xE700u
        && object.m_bodyFullyDecoded;
}

bool allocationSweep(bool binary) {
    constexpr long calibrationLimit = 1000000;
    const DRW::Version versions[] = {
        DRW::AC1024, DRW::AC1027, DRW::AC1032};
    const auto versionName = [](DRW::Version version) {
        switch (version) {
        case DRW::AC1024: return "AC1024";
        case DRW::AC1027: return "AC1027";
        case DRW::AC1032: return "AC1032";
        default: return "UNKNOWN";
        }
    };
    const auto writeRecord = [](dxfRW& codec,
                                DRW_DynamicBlockObject& object) {
        return codec.writeDynamicBlockObject(&object);
    };

    for (const DRW::Version version : versions) {
        std::ostringstream cleanBytes;
        dxfRW cleanCodec("");
        DxfDynamicBlockTestAccess::attachWriter(
            cleanCodec, version, binary, cleanBytes);
        DRW_DynamicBlockObject cleanObject = makePurgePreventer();
        long allocationCount = 0;
        bool cleanWrite = false;
        {
            AllocationFailure failure(calibrationLimit);
            cleanWrite = writeRecord(cleanCodec, cleanObject);
            allocationCount = calibrationLimit - allocationsBeforeFailure;
        }
        const DRW_WritingContext* cleanContext =
            cleanCodec.getWritingContext();
        if (!cleanWrite || !sourceUnchanged(cleanObject)
            || cleanContext == nullptr
            || cleanContext->sourceHandleToWrittenObjectMap.count(
                   cleanObject.handle) != 1u
            || cleanContext->sourceHandleToWrittenObjectMap.at(
                   cleanObject.handle) != cleanCodec.remapHandle(cleanObject.handle)
            || !DxfDynamicBlockTestAccess::writeEnd(cleanCodec)) {
            std::cerr << "clean PurgePreventer record failed (version="
                      << versionName(version) << ", binary=" << binary
                      << ") write=" << cleanWrite
                      << " source=" << sourceUnchanged(cleanObject) << '\n';
            return false;
        }
        const std::string expectedBytes = cleanBytes.str();
        const bool cleanReadback = wireReadback(expectedBytes, binary);
        if (allocationCount <= 0 || !cleanReadback) {
            std::cerr << "PurgePreventer clean readback/calibration failed version="
                      << versionName(version) << " binary=" << binary
                      << " allocationCount=" << allocationCount
                      << " readback=" << cleanReadback << '\n';
            return false;
        }

        for (long offset = 0; offset < allocationCount; ++offset) {
            std::ostringstream failedBytes;
            dxfRW failedCodec("");
            DxfDynamicBlockTestAccess::attachWriter(
                failedCodec, version, binary, failedBytes);
            DRW_DynamicBlockObject object = makePurgePreventer();
            const std::size_t failuresBefore = failedAllocations;
            bool wrote = false;
            try {
                AllocationFailure failure(offset);
                wrote = writeRecord(failedCodec, object);
            } catch (const std::bad_alloc&) {
                allocationsBeforeFailure = -1;
            }
            allocationsBeforeFailure = -1;
            const DRW_WritingContext* context = failedCodec.getWritingContext();
            if (wrote || failedAllocations == failuresBefore
                || !sourceUnchanged(object) || context == nullptr
                || context->sourceHandleToWrittenObjectMap.count(object.handle)
                    != 0u) {
                std::cerr << "PurgePreventer allocation rollback failed at "
                          << versionName(version) << ' '
                          << (binary ? "Binary" : "ASCII")
                          << " offset=" << offset << " wrote=" << wrote << '\n';
                return false;
            }

            std::ostringstream retryBytes;
            dxfRW retryCodec("");
            DxfDynamicBlockTestAccess::attachWriter(
                retryCodec, version, binary, retryBytes);
            const bool retryWrote = writeRecord(retryCodec, object);
            const DRW_WritingContext* retryContext =
                retryCodec.getWritingContext();
            if (!retryWrote || !sourceUnchanged(object) || retryContext == nullptr
                || retryContext->sourceHandleToWrittenObjectMap.count(
                       object.handle) != 1u
                || !DxfDynamicBlockTestAccess::writeEnd(retryCodec)
                || retryBytes.str() != expectedBytes
                || !wireReadback(retryBytes.str(), binary)) {
                std::cerr << "PurgePreventer fresh-writer retry failed at "
                          << versionName(version) << ' '
                          << (binary ? "Binary" : "ASCII")
                          << " offset=" << offset << '\n';
                return false;
            }
        }
        std::cout << versionName(version) << ' '
                  << (binary ? "Binary" : "ASCII")
                  << " PurgePreventer allocation offsets=" << allocationCount
                  << "; rollback, fresh-writer retry, and readback pass\n";
    }
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (allocationsBeforeFailure == 0) {
        ++failedAllocations;
        allocationsBeforeFailure = -1;
        throw std::bad_alloc();
    }
    if (allocationsBeforeFailure > 0)
        --allocationsBeforeFailure;
    if (void* value = std::malloc(size == 0u ? 1u : size))
        return value;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete(void* value, std::size_t) noexcept { std::free(value); }
void operator delete[](void* value, std::size_t) noexcept { std::free(value); }

int main() {
    return allocationSweep(false) && allocationSweep(true)
        && visibilityAllocationSweep(false)
        && visibilityAllocationSweep(true)
        && basePointAllocationSweep(false)
        && basePointAllocationSweep(true)
        && alignmentAllocationSweep(false)
        && alignmentAllocationSweep(true)
        && moveActionAllocationSweep(false)
        && moveActionAllocationSweep(true)
        && scaleActionAllocationSweep(false)
        && scaleActionAllocationSweep(true)
        && flipActionAllocationSweep(false)
        && flipActionAllocationSweep(true)
        && lookupActionAllocationSweep(false)
        && lookupActionAllocationSweep(true)
        && stretchActionAllocationSweep(false)
        && stretchActionAllocationSweep(true)
        && flipParameterAllocationSweep(false)
        && flipParameterAllocationSweep(true)
        && linearParameterAllocationSweep(false)
        && linearParameterAllocationSweep(true)
        && constraintParameterAllocationSweep(
            DRW_DynamicBlockObject::Kind::HorizontalConstraintParameter, false)
        && constraintParameterAllocationSweep(
            DRW_DynamicBlockObject::Kind::HorizontalConstraintParameter, true)
        && constraintParameterAllocationSweep(
            DRW_DynamicBlockObject::Kind::VerticalConstraintParameter, false)
        && constraintParameterAllocationSweep(
            DRW_DynamicBlockObject::Kind::VerticalConstraintParameter, true)
            ? 0 : 1;
}
