// Generated AC1024 associative constraint-group writer allocation rollback.
#include "drw_objects.h"
#include "libdxfrw.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <new>
#include <ostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

class DxfAssociativeConstraintGroupAllocationTestAccess {
public:
    static void attachWriter(dxfRW& codec, bool binary,
                             std::ostream& output) {
        codec.version = DRW::AC1024;
        codec.binFile = binary;
        codec.writer = binary
            ? std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterBinary>(&output))
            : std::unique_ptr<dxfWriter>(
                  std::make_unique<dxfWriterAscii>(&output));
    }

    static bool appendSentinel(dxfRW& codec) {
        return codec.writer != nullptr
            && codec.writer->writeString(0, "WIRE_END");
    }

    static bool hasEmptyHandleMap(const dxfRW& codec) {
        return codec.m_writingContext.sourceHandleToMintedMap.empty();
    }

    static void resetForRetry(dxfRW& codec) {
        codec.resetDxfWriteSession();
        if (codec.writer != nullptr)
            codec.writer->restoreWriteError(false);
    }
};

namespace {
long allocationsBeforeFailure = -1;
long allocationAttempts = 0;
bool allocationFailed = false;

void armAllocationFailure(long offset) noexcept {
    allocationsBeforeFailure = offset;
    allocationAttempts = 0;
    allocationFailed = false;
}

void disarmAllocationFailure() noexcept {
    allocationsBeforeFailure = -1;
}

DRW_AssociativeObject makeGroup() {
    using Node = DRW_AssociativeConstraintNode;
    DRW_AssociativeObject group("ACDBASSOC2DCONSTRAINTGROUP");
    group.handle = 0x500u;
    group.parentHandle = 0x501u;
    group.m_classVersion = 1u;
    group.m_owningNetworkHandle = 0x502u;
    group.m_actionIndex = 4;
    group.m_maxDependencyIndex = 1;
    group.m_constraintGroupBodyParsed = true;
    group.m_constraintGroupVersion = 0;
    group.m_constraintActionCount = 0;
    group.m_constraintActionsParsed = true;
    group.m_constraintNextNodeIdRead = true;
    group.m_constraintNextNodeId = 2;
    group.m_constraintNodeCount = 1;
    group.m_constraintNodesParsed = true;

    Node node;
    node.m_className = "AcFixedConstraint";
    node.m_dataKind = Node::DataKind::Geometrical;
    node.m_nodeId = 1;
    node.m_ownerId = 1;
    node.m_isImplied = true;
    node.m_classDataParsed = true;
    group.m_constraintNodes.push_back(node);

    DRW_AssociativePrefixStatus action;
    action.m_kind = DRW_AssociativePrefixStatus::Kind::AcDbAssocAction;
    action.m_status = DRW_AssociativePrefixStatus::ParseStatus::Complete;
    action.m_classVersion = 1u;
    action.m_decodedHandleCount = 2u;

    DRW_AssociativePrefixStatus constraintGroup;
    constraintGroup.m_kind =
        DRW_AssociativePrefixStatus::Kind::AcDbAssoc2dConstraintGroup;
    constraintGroup.m_status = DRW_AssociativePrefixStatus::ParseStatus::Complete;
    constraintGroup.m_classVersion = 1u;
    constraintGroup.m_decodedValueCount = 1u;
    constraintGroup.m_decodedCountValue = 1;
    group.m_prefixStatuses = {action, constraintGroup};
    return group;
}

DRW_AssociativeObject makeVariable() {
    DRW_AssociativeObject variable("ACDBASSOCVARIABLE");
    variable.handle = 0x600u;
    variable.parentHandle = 0x601u;
    variable.m_classVersion = 1u;
    variable.m_owningNetworkHandle = 0x602u;
    variable.m_actionIndex = 4;
    variable.m_associativeVariableFieldsParsed = true;
    variable.m_associativeVariableWireComplete = true;
    variable.m_variableOwnedDependenciesParsed = true;
    variable.m_variableClassVersion = 2;
    variable.m_variableName = "BOLTHOLE";
    variable.m_variableExpression = "1";
    variable.m_variableEvaluator = "AcDbCalc:1.0";
    variable.m_variableDescription = "allocation-retry";
    variable.m_variableValueParsed = true;
    variable.m_variableValueCode = 40;
    variable.m_variableValueKind =
        DRW_AssociativeObject::EvalVariantKind::Real;
    variable.m_variableValueReal = 3.5;

    DRW_AssociativePrefixStatus action;
    action.m_kind = DRW_AssociativePrefixStatus::Kind::AcDbAssocAction;
    action.m_status = DRW_AssociativePrefixStatus::ParseStatus::Complete;
    action.m_classVersion = 1u;
    action.m_decodedHandleCount = 2u;
    DRW_AssociativePrefixStatus body;
    body.m_kind = DRW_AssociativePrefixStatus::Kind::AcDbAssocVariable;
    body.m_status = DRW_AssociativePrefixStatus::ParseStatus::Complete;
    body.m_classVersion = 2u;
    body.m_decodedValueCount = 10u;
    variable.m_prefixStatuses = {action, body};
    return variable;
}

bool sameCallerProfile(const DRW_AssociativeObject& actual,
                       const DRW_AssociativeObject& expected) {
    if (actual.handle != expected.handle
        || actual.parentHandle != expected.parentHandle
        || actual.m_recordName != expected.m_recordName
        || actual.m_classVersion != expected.m_classVersion
        || actual.m_owningNetworkHandle != expected.m_owningNetworkHandle
        || actual.m_actionIndex != expected.m_actionIndex
        || actual.m_maxDependencyIndex != expected.m_maxDependencyIndex
        || actual.m_constraintGroupBodyParsed != expected.m_constraintGroupBodyParsed
        || actual.m_constraintGroupVersion != expected.m_constraintGroupVersion
        || actual.m_constraintActionCount != expected.m_constraintActionCount
        || actual.m_constraintActionsParsed != expected.m_constraintActionsParsed
        || actual.m_constraintNextNodeIdRead != expected.m_constraintNextNodeIdRead
        || actual.m_constraintNextNodeId != expected.m_constraintNextNodeId
        || actual.m_constraintNodeCount != expected.m_constraintNodeCount
        || actual.m_constraintNodes.size() != expected.m_constraintNodes.size()
        || actual.m_constraintNodesParsed != expected.m_constraintNodesParsed
        || actual.m_prefixStatuses.size() != expected.m_prefixStatuses.size())
        return false;
    for (std::size_t i = 0; i < actual.m_prefixStatuses.size(); ++i) {
        const auto& leftStatus = actual.m_prefixStatuses[i];
        const auto& rightStatus = expected.m_prefixStatuses[i];
        if (leftStatus.m_kind != rightStatus.m_kind
            || leftStatus.m_status != rightStatus.m_status
            || leftStatus.m_classVersion != rightStatus.m_classVersion
            || leftStatus.m_decodedHandleCount != rightStatus.m_decodedHandleCount
            || leftStatus.m_decodedValueCount != rightStatus.m_decodedValueCount
            || leftStatus.m_decodedCountValue != rightStatus.m_decodedCountValue)
            return false;
    }
    const auto& left = actual.m_constraintNodes.front();
    const auto& right = expected.m_constraintNodes.front();
    return left.m_className == right.m_className
        && left.m_dataKind == right.m_dataKind
        && left.m_nodeId == right.m_nodeId
        && left.m_ownerId == right.m_ownerId
        && left.m_isImplied == right.m_isImplied
        && left.m_classDataParsed == right.m_classDataParsed
        && left.m_connections == right.m_connections;
}

bool sameVariableProfile(const DRW_AssociativeObject& actual,
                         const DRW_AssociativeObject& expected) {
    if (actual.handle != expected.handle
        || actual.parentHandle != expected.parentHandle
        || actual.m_recordName != expected.m_recordName
        || actual.reactorHandles != expected.reactorHandles
        || actual.m_classVersion != expected.m_classVersion
        || actual.m_owningNetworkHandle != expected.m_owningNetworkHandle
        || actual.m_actionIndex != expected.m_actionIndex
        || actual.m_maxDependencyIndex != expected.m_maxDependencyIndex
        || actual.m_associativeVariableFieldsParsed
            != expected.m_associativeVariableFieldsParsed
        || actual.m_associativeVariableWireComplete
            != expected.m_associativeVariableWireComplete
        || actual.m_variableOwnedDependenciesParsed
            != expected.m_variableOwnedDependenciesParsed
        || actual.m_variableOwnedDependencies.size()
            != expected.m_variableOwnedDependencies.size()
        || actual.m_variableSuffixMarkerPresent
            != expected.m_variableSuffixMarkerPresent
        || actual.m_variableSuffixMarker != expected.m_variableSuffixMarker
        || actual.m_variableClassVersion != expected.m_variableClassVersion
        || actual.m_variableName != expected.m_variableName
        || actual.m_variableExpression != expected.m_variableExpression
        || actual.m_variableEvaluator != expected.m_variableEvaluator
        || actual.m_variableDescription != expected.m_variableDescription
        || actual.m_variableValueParsed != expected.m_variableValueParsed
        || actual.m_variableValueCode != expected.m_variableValueCode
        || actual.m_variableValueKind != expected.m_variableValueKind
        || actual.m_variableValueReal != expected.m_variableValueReal
        || actual.m_variableValueInt32 != expected.m_variableValueInt32
        || actual.m_variableHasT78 != expected.m_variableHasT78
        || actual.m_variableT78 != expected.m_variableT78
        || actual.m_variableB290 != expected.m_variableB290
        || actual.m_prefixStatuses.size() != expected.m_prefixStatuses.size())
        return false;
    for (std::size_t i = 0; i < actual.m_prefixStatuses.size(); ++i) {
        const auto& left = actual.m_prefixStatuses[i];
        const auto& right = expected.m_prefixStatuses[i];
        if (left.m_kind != right.m_kind || left.m_status != right.m_status
            || left.m_classVersion != right.m_classVersion
            || left.m_decodedHandleCount != right.m_decodedHandleCount
            || left.m_decodedValueCount != right.m_decodedValueCount
            || left.m_decodedCountValue != right.m_decodedCountValue)
            return false;
    }
    return true;
}

struct Codec {
    explicit Codec(bool binary)
        : binary(binary), output(&outputBuffer), writer(""), group(makeGroup()) {
        writer.setHandleRemap({{0x500u, 0x3E8u}, {0x501u, 0x3E9u},
                               {0x502u, 0x3EAu}, {0x600u, 0x3EBu},
                               {0x601u, 0x3ECu}, {0x602u, 0x3EDu}});
        DxfAssociativeConstraintGroupAllocationTestAccess::attachWriter(
            writer, binary, output);
    }

    class FixedBuffer final : public std::streambuf {
    public:
        std::string str() const { return std::string(storage.data(), size); }
        bool empty() const noexcept { return size == 0u; }

    protected:
        std::streamsize xsputn(const char* source,
                               std::streamsize count) override {
            if (count < 0
                || static_cast<std::size_t>(count) > storage.size() - size)
                return 0;
            std::copy_n(source, static_cast<std::size_t>(count),
                        storage.data() + size);
            size += static_cast<std::size_t>(count);
            return count;
        }

        int_type overflow(int_type character) override {
            if (traits_type::eq_int_type(character, traits_type::eof()))
                return traits_type::not_eof(character);
            if (size == storage.size())
                return traits_type::eof();
            storage[size++] = traits_type::to_char_type(character);
            return character;
        }

    private:
        std::array<char, 65536u> storage{};
        std::size_t size {0u};
    } outputBuffer;

    bool binary;
    std::ostream output;
    dxfRW writer;
    DRW_AssociativeObject group;
};

bool appendSentinel(Codec& codec) {
    return DxfAssociativeConstraintGroupAllocationTestAccess::appendSentinel(
        codec.writer);
}

bool hasTypedRouteTags(const std::string& bytes, bool binary) {
    std::istringstream input(bytes, std::ios::in | std::ios::binary);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
        : std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
    bool inGroup = false;
    bool nodeSeen = false;
    bool endSeen = false;
    int code = 0;
    while (reader->readRec(&code)) {
        if (code == 0) {
            const std::string value = reader->getString();
            if (value == "ACDBASSOC2DCONSTRAINTGROUP")
                inGroup = true;
            else if (value == "WIRE_END") {
                endSeen = true;
                break;
            }
        } else if (inGroup && code == 1
                   && reader->getString() == "AcFixedConstraint") {
            nodeSeen = true;
        } else {
            (void)reader->getString();
        }
    }
    return inGroup && nodeSeen && endSeen;
}

bool hasVariableRouteTags(const std::string& bytes, bool binary) {
    std::istringstream input(bytes, std::ios::in | std::ios::binary);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
        : std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
    bool inVariable = false;
    bool nameSeen = false;
    bool valueSeen = false;
    bool handleSeen = false;
    bool ownerSeen = false;
    bool networkSeen = false;
    bool endSeen = false;
    int code = 0;
    while (reader->readRec(&code)) {
        const std::string value = reader->getString();
        if (code == 0 && value == "ACDBASSOCVARIABLE")
            inVariable = true;
        else if (code == 0 && value == "WIRE_END") {
            endSeen = true;
            break;
        } else if (inVariable && code == 1) {
            nameSeen = nameSeen || value == "BOLTHOLE";
            valueSeen = valueSeen || value == "1";
        } else if (inVariable && code == 5) {
            handleSeen = value == "3EB";
        } else if (inVariable && code == 330) {
            ownerSeen = ownerSeen || value == "3EC";
            networkSeen = networkSeen || value == "3ED";
        }
    }
    return inVariable && nameSeen && valueSeen && handleSeen && ownerSeen
        && networkSeen && endSeen;
}

bool runRoute(bool binary) {
    Codec calibration(binary);
    armAllocationFailure(0x7fffffffL);
    bool calibrationWrote = false;
    try {
        calibrationWrote = calibration.writer.writeAssociativeConstraintGroup(
            &calibration.group);
    } catch (const std::bad_alloc&) {
        disarmAllocationFailure();
        std::cerr << "calibration allocation measurement unexpectedly failed\n";
        return false;
    }
    const long allocationCount = allocationAttempts;
    disarmAllocationFailure();
    if (!calibrationWrote || !appendSentinel(calibration)) {
        std::cerr << "calibration write failed\n";
        return false;
    }
    const std::string expectedBytes = calibration.outputBuffer.str();
    if (!hasTypedRouteTags(expectedBytes, binary)) {
        std::cerr << "calibration output did not read back\n";
        return false;
    }
    if (allocationCount <= 0) {
        std::cerr << "calibration observed no writer allocations\n";
        return false;
    }

    for (long offset = 0; offset < allocationCount; ++offset) {
        Codec retry(binary);
        const DRW_AssociativeObject baseline = retry.group;
        armAllocationFailure(offset);
        bool wrote = false;
        try {
            wrote = retry.writer.writeAssociativeConstraintGroup(&retry.group);
        } catch (const std::bad_alloc&) {
            // A failure before the writer's local try block is still required
            // to leave record bytes and the source identity untouched.
        }
        disarmAllocationFailure();
        if (wrote || !allocationFailed || !retry.outputBuffer.empty()
            || !sameCallerProfile(retry.group, baseline)
            || !DxfAssociativeConstraintGroupAllocationTestAccess::hasEmptyHandleMap(
                retry.writer)) {
            std::cerr << "allocation rollback failed at offset " << offset
                      << " wrote=" << wrote << " fault=" << allocationFailed
                      << " output-empty=" << retry.outputBuffer.empty()
                      << " source-same=" << sameCallerProfile(retry.group, baseline)
                      << " handles-empty="
                      << DxfAssociativeConstraintGroupAllocationTestAccess::hasEmptyHandleMap(
                             retry.writer)
                      << "\n";
            return false;
        }
        DxfAssociativeConstraintGroupAllocationTestAccess::resetForRetry(
            retry.writer);
        const bool retryWrote = retry.writer.writeAssociativeConstraintGroup(
            &retry.group);
        const bool sentinelWrote = retryWrote && appendSentinel(retry);
        const std::string retryBytes = retry.outputBuffer.str();
        const bool tagsRead = hasTypedRouteTags(retryBytes, binary);
        if (!retryWrote || !sentinelWrote || retryBytes != expectedBytes
            || !tagsRead) {
            std::cerr << "same-writer allocation retry/readback failed at offset "
                      << offset << " write=" << retryWrote
                      << " sentinel=" << sentinelWrote
                      << " same-bytes=" << (retryBytes == expectedBytes)
                      << " tags=" << tagsRead
                      << " bytes=" << retryBytes.size()
                      << " expected=" << expectedBytes.size() << "\n";
            return false;
        }
    }
    std::cout << "AC1024 associative constraint group "
              << (binary ? "Binary" : "ASCII")
              << ": allocation offsets=" << allocationCount
              << ", rollback/retry/readback=PASS\n";
    return true;
}

bool runVariableRoute(bool binary) {
    Codec calibration(binary);
    DRW_AssociativeObject variable = makeVariable();
    armAllocationFailure(0x7fffffffL);
    bool calibrationWrote = false;
    try {
        calibrationWrote = calibration.writer.writeAssociativeVariable(&variable);
    } catch (const std::bad_alloc&) {
        disarmAllocationFailure();
        std::cerr << "variable calibration allocation measurement failed\n";
        return false;
    }
    const long allocationCount = allocationAttempts;
    disarmAllocationFailure();
    if (!calibrationWrote || !appendSentinel(calibration)) {
        std::cerr << "variable calibration write failed\n";
        return false;
    }
    const std::string expectedBytes = calibration.outputBuffer.str();
    if (!hasVariableRouteTags(expectedBytes, binary) || allocationCount <= 0) {
        std::cerr << "variable calibration readback/allocation count failed\n";
        return false;
    }

    for (long offset = 0; offset < allocationCount; ++offset) {
        Codec retry(binary);
        DRW_AssociativeObject source = makeVariable();
        const DRW_AssociativeObject baseline = source;
        armAllocationFailure(offset);
        bool wrote = false;
        try {
            wrote = retry.writer.writeAssociativeVariable(&source);
        } catch (const std::bad_alloc&) {
            // The allocation point may precede the writer's local try block.
        }
        disarmAllocationFailure();
        if (wrote || !allocationFailed || !retry.outputBuffer.empty()
            || !sameVariableProfile(source, baseline)
            || !DxfAssociativeConstraintGroupAllocationTestAccess::hasEmptyHandleMap(
                retry.writer)) {
            std::cerr << "variable allocation rollback failed at offset "
                      << offset << '\n';
            return false;
        }
        DxfAssociativeConstraintGroupAllocationTestAccess::resetForRetry(
            retry.writer);
        const bool retryWrote = retry.writer.writeAssociativeVariable(&source);
        const bool sentinelWrote = retryWrote && appendSentinel(retry);
        const std::string retryBytes = retry.outputBuffer.str();
        if (!retryWrote || !sentinelWrote || retryBytes != expectedBytes
            || !hasVariableRouteTags(retryBytes, binary)) {
            std::cerr << "variable same-writer retry/readback failed at offset "
                      << offset << '\n';
            return false;
        }
    }
    std::cout << "AC1024 associative variable "
              << (binary ? "Binary" : "ASCII")
              << ": allocation offsets=" << allocationCount
              << ", rollback/retry/readback=PASS\n";
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (allocationsBeforeFailure >= 0) {
        ++allocationAttempts;
        if (allocationsBeforeFailure == 0) {
            allocationsBeforeFailure = -1;
            allocationFailed = true;
            throw std::bad_alloc();
        }
        --allocationsBeforeFailure;
    }
    if (void* memory = std::malloc(size == 0u ? 1u : size))
        return memory;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

int main() {
    if (!runRoute(false) || !runRoute(true)
        || !runVariableRoute(false) || !runVariableRoute(true))
        return 1;
    return 0;
}
