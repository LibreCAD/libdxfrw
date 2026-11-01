#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <memory>
#include <new>
#include <sstream>
#include <set>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "libdwgr.h"
#include "dx_iface.h"
#include "drw_classes.h"

#include "intern/dwgreader15.h"
#include "intern/dwgreader18.h"
#include "intern/dwgreader21.h"
#include "intern/dwgreader24.h"
#include "intern/dwgreader27.h"
#include "intern/dwgreader32.h"
#include "intern/dwgreaderR11.h"
#include "intern/dwgreaderR1_40.h"
#include "intern/dwgreader.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgutil.h"
#include "intern/drw_textcodec.h"
#include "intern/dwgtemplate.h"
#include "intern/dwgheadersection.h"
#include "intern/dwg_header_time.h"
#include "intern/dwgobjectframe.h"
#include "persistent_manager_test_helpers.h"
#include "associative_constraint_group_test_helpers.h"
#include "intern/dwgwriter15.h"
#include "intern/dwgwriter18.h"
#include "intern/dwgwriter21.h"
#include "intern/dwgwriter24.h"
#include "intern/dwgwriter27.h"
#include "intern/dwgwriter32.h"
#include "intern/dwgsafety.h"

class DwgBlockOwnershipTestAccess {
public:
    static bool parse(DRW_Block_Record& record, DRW::Version version,
                      dwgBuffer& buffer) {
        return record.parseDwg(version, &buffer);
    }

    static void setEntityHandles(DRW_Block_Record& record,
                                 std::vector<std::uint32_t> handles) {
        record.entMap = std::move(handles);
    }

    static void setLegacyEntityChain(DRW_Block_Record& record,
                                     std::uint32_t first,
                                     std::uint32_t last) {
        record.firstEH = first;
        record.lastEH = last;
    }

    static void setDelimiters(DRW_Block_Record& record,
                              std::uint32_t block,
                              std::uint32_t endBlock) {
        record.block = block;
        record.endBlock = endBlock;
    }
};

std::vector<std::uint8_t> makeGeneratedR2000XrefBlockRecord(
    DRW::Version version, std::uint16_t xrefIndexPlusOne, bool overlaid,
    bool xrefUnloaded = false) {
    dwgBufferW output;
    output.putObjType(version,
                      static_cast<std::uint16_t>(DRW::BLOCK_RECORD));
    output.putRawLong32(0u); // Legacy object-size sentinel: inline handles.

    const auto makeHandle = [](std::uint8_t code, std::uint32_t reference) {
        dwgHandle value;
        value.code = code;
        value.ref = reference;
        return value;
    };
    output.putHandle(makeHandle(0u, 0x200u)); // Object's own handle.
    output.putBitShort(0u); // EED terminator.
    output.putBitLong(0); // Reactor count.
    if (version > DRW::AC1015)
        output.putBoolBit(false); // No extension dictionary; pointer follows.

    output.putVariableText(version, "XREF_TEST");
    output.putBoolBit(true); // is_xref_ref
    output.putBitShort(xrefIndexPlusOne);
    output.putBoolBit(true); // xref-dependent
    output.putBoolBit(false); // anonymous
    output.putBoolBit(false); // has attributes
    output.putBoolBit(true); // BLOCK is an XREF
    output.putBoolBit(overlaid);
    output.putBoolBit(xrefUnloaded); // Wire 0 means loaded (ODA §20.4.52).
    output.put3BitDouble(DRW_Coord(0.0, 0.0, 0.0));
    output.putVariableText(version, "refs/linked.dwg");
    output.putRawChar8(0u); // No INSERT references; terminates the list.
    output.putVariableText(version, "generated XREF test");
    output.putBitLong(0); // Preview size.

    // Common table links followed by BLOCK_HEADER links. XREF records do not
    // carry first/last entity handles, so the next reference is ENDBLK.
    output.putHandle(makeHandle(DRW::DwgSoftPointer, 0x10u)); // BLOCK_CONTROL
    output.putHandle(makeHandle(DRW::DwgHardOwnership, 0u)); // XDictionary
    output.putHandle(makeHandle(DRW::DwgHardPointer, 0u)); // XREF pointer
    output.putHandle(makeHandle(DRW::DwgHardOwnership, 0x201u)); // BLOCK
    output.putHandle(makeHandle(DRW::DwgHardOwnership, 0x202u)); // ENDBLK
    output.putHandle(makeHandle(DRW::DwgHardPointer, 0u)); // LAYOUT
    return output.data();
}

class PolylineStageTestReader final : public dwgReader21 {
public:
    using Outcome = DwgMappedEntityOutcome;
    using StageFailure = DwgPolylineStageFailurePoint;
    using PendingStateForAccounting = PendingPolylineState;
    using OrphanStateForAccounting = OrphanPolylineVertexState;

    PolylineStageTestReader()
        : dwgReader21(std::make_unique<dwgBuffer>(nullptr, 0u), nullptr) {
        version = DRW::AC1021;
    }

    bool addFrame(std::uint32_t handle) {
        return ObjectMap.emplace(handle, objHandle(0u, handle, handle)).second;
    }

    void setReaderVersionForTest(DRW::Version value) {
        version = value;
        decoder.setVersion(value, false);
    }

    bool setSourceOrdinal(std::uint32_t handle, std::uint64_t ordinal) {
        const auto found = ObjectMap.find(handle);
        if (found == ObjectMap.end())
            return false;
        found->second.sourceOrdinal = ordinal;
        return true;
    }

    bool seedPendingCoverageForTest(std::uint32_t handle) {
        const auto found = ObjectMap.find(handle);
        if (found == ObjectMap.end() || !m_dwgSourceFrameLedger.empty() ||
            !m_dwgSourceFrameIndexes.empty())
            return false;
        DRW_DwgFrameCoverageEntry entry;
        entry.m_handle = handle;
        entry.m_sourceOffset = found->second.loc;
        entry.m_sourceMapOrdinal = found->second.sourceOrdinal;
        entry.m_sourceOffsetSpace = found->second.sourceOffsetSpace;
        try {
            m_dwgSourceFrameLedger.push_back(entry);
            if (!m_dwgSourceFrameIndexes.emplace(handle, 0u).second) {
                m_dwgSourceFrameLedger.clear();
                return false;
            }
        } catch (...) {
            m_dwgSourceFrameLedger.clear();
            m_dwgSourceFrameIndexes.clear();
            return false;
        }
        m_dwgFrameCoverageStatus = DRW_DwgFrameCoverageStatus::InProgress;
        return true;
    }

    void configureResidualRestoreForTest(std::uint32_t handle,
                                         std::size_t restoreAttempts) {
        m_residualRestoreHandle = handle;
        m_residualRestoreAttempts = restoreAttempts;
        m_residualReadAttempts = 0u;
    }

    std::size_t residualReadAttemptsForTest() const noexcept {
        return m_residualReadAttempts;
    }

    bool readDwgEntity(dwgBuffer *dbuf, objHandle &obj, DRW_Interface &intfa,
                       bool *frameFailure,
                       DwgIntegrityAddressSpace offsetSpace) override {
        if (obj.handle != m_residualRestoreHandle) {
            return dwgReader21::readDwgEntity(dbuf, obj, intfa, frameFailure,
                                               offsetSpace);
        }
        ++m_residualReadAttempts;
        if (m_residualRestoreAttempts != 0u) {
            --m_residualRestoreAttempts;
            if (!ObjectMap.emplace(obj.handle, obj).second) {
                if (frameFailure != nullptr)
                    *frameFailure = true;
                return false;
            }
            return false;
        }

        DRW_DwgFramePublication publication;
        publication.m_handle = obj.handle;
        publication.m_version = version;
        publication.m_sourceOffset = obj.loc;
        publication.m_sourceMapOrdinal = obj.sourceOrdinal;
        publication.m_sourceOffsetSpace = obj.sourceOffsetSpace;
        publication.m_hasSourceLocation = true;
        publication.m_encodedType = static_cast<int>(obj.type);
        publication.m_resolvedType = static_cast<int>(obj.type);
        publication.m_isEntity = true;
        publication.m_carrier = DRW_DwgFramePublication::Carrier::Typed;
        if (!publishDwgFramePublication(intfa, std::move(publication))) {
            if (frameFailure != nullptr)
                *frameFailure = true;
            return false;
        }
        return true;
    }

    bool runEntitySweep(std::vector<std::uint8_t>& source,
                        DRW_Interface& sink) {
        dwgBuffer input(source.data(), source.size(), &decoder);
        return dwgReader::readDwgEntities(
            sink, &input, DwgIntegrityAddressSpace::PhysicalFile);
    }

    bool addEncodedEntityFrame(
        std::vector<std::uint8_t>& source, std::uint32_t type,
        std::uint32_t handle,
        const std::function<bool(dwgBufferW*, dwgBufferW*)>& encode) {
        dwgBufferW body;
        dwgBufferW handles;
        if (!encode(&body, &handles) || !body.isGood() || !handles.isGood())
            return false;
        body.alignToByte();
        handles.alignToByte();
        if (!body.isGood() || !handles.isGood() || body.size() == 0u ||
            body.size() > static_cast<std::size_t>(
                              (std::numeric_limits<std::uint32_t>::max)() / 8u) ||
            handles.size() > (std::numeric_limits<std::size_t>::max)() -
                                 body.size())
            return false;

        const std::uint8_t bsCode =
            static_cast<std::uint8_t>((body.data()[0] >> 6u) & 0x03u);
        const std::size_t objectSizeBitOffset =
            (bsCode == 0x01u) ? 10u : (bsCode == 0x00u) ? 18u : 2u;
        body.patchRawLong32AtBit(
            objectSizeBitOffset,
            static_cast<std::uint32_t>(body.size() * 8u));

        dwgBufferW frame;
        const std::size_t sourceOffset = source.size();
        const std::size_t frameStart = frame.size();
        const std::size_t totalBodyBytes = body.size() + handles.size();
        if (totalBodyBytes > static_cast<std::size_t>(
                                 (std::numeric_limits<std::int32_t>::max)()))
            return false;
        frame.putModularShort(static_cast<std::int32_t>(totalBodyBytes));
        frame.putBytes(body.data().data(), body.data().size());
        if (!handles.data().empty())
            frame.putBytes(handles.data().data(), handles.data().size());
        const std::uint16_t crc =
            frame.crc16(0xC0C1u, frameStart, frame.size());
        frame.putRawShort16(crc);
        if (!frame.isGood() || sourceOffset > static_cast<std::size_t>(
                                                (std::numeric_limits<std::int32_t>::max)()) ||
            !ObjectMap.emplace(handle,
                               objHandle(type, handle,
                                         static_cast<std::uint32_t>(sourceOffset)))
                 .second)
            return false;
        source.insert(source.end(), frame.data().begin(), frame.data().end());
        return true;
    }

    bool preflightPolylineOwnership(
        const std::vector<const DRW_Block_Record*>& records,
        std::vector<std::uint8_t>& source) {
        decoder.setVersion(version, false);
        dwgBuffer input(source.data(), source.size(), &decoder);
        return preflightMappedPolylineOwnership(records, &input);
    }

    bool hasInvalidPolylineOwner(std::uint32_t handle) const {
        return m_invalidPolylineOwners.find(handle) !=
               m_invalidPolylineOwners.end();
    }
    std::size_t entityParseFailures() const { return m_entityParseFailures; }
    const std::vector<DwgEntityFailureDiagnostic>& entityFailures() const {
        return m_entityFailureDiagnostics;
    }
    bool readHandleMapSection(std::vector<std::uint8_t>& bytes) {
        dwgBuffer mapBuffer(bytes.data(), bytes.size());
        return dwgReader::readDwgHandles(
            &mapBuffer, 0u, bytes.size(),
            (std::numeric_limits<std::uint64_t>::max)(),
            DwgIntegrityAddressSpace::DecodedBuffer, 77);
    }

    Outcome stageParent(DRW_Polyline parent, DRW_Interface &interface,
                        bool &frameRemainsInMap) {
        return stageWithFrame(
            parent.handle, interface, frameRemainsInMap,
            [this, &parent, &interface](
                const DRW_DwgFramePublication &publication) {
                return stagePendingPolyline(std::move(parent), publication,
                                            interface);
            });
    }

    Outcome stageInsert(DRW_Insert insert, DRW_Interface &interface,
                        bool &frameRemainsInMap) {
        return stageWithFrame(
            insert.handle, interface, frameRemainsInMap,
            [this, &insert, &interface](
                const DRW_DwgFramePublication &publication) {
                return stagePendingInsert(std::move(insert), publication,
                                          interface);
            });
    }

    Outcome stageAttribute(std::uint32_t sourceFrameHandle,
                           std::uint32_t ownerHandle,
                           std::uint32_t attributeHandle,
                           DRW_Interface &interface,
                           bool &frameRemainsInMap) {
        DRW_Attrib attribute;
        attribute.handle = attributeHandle;
        attribute.parentHandle = ownerHandle;
        auto staged = std::make_shared<DRW_Attrib>(std::move(attribute));
        return stageWithFrame(
            sourceFrameHandle, interface, frameRemainsInMap,
            [this, &staged, &interface](
                const DRW_DwgFramePublication &publication) {
                return stagePendingAttribute(std::move(staged), publication,
                                             interface);
            });
    }

    Outcome stageVertex(DRW_Vertex vertex, DRW_Interface &interface,
                        bool &frameRemainsInMap) {
        return stageWithFrame(
            vertex.handle, interface, frameRemainsInMap,
            [this, &vertex, &interface](
                const DRW_DwgFramePublication &publication) {
                return stagePendingPolylineVertex(std::move(vertex),
                                                  publication, interface);
            });
    }

    Outcome stageSequenceEnd(std::uint32_t handle, std::uint32_t owner,
                             DRW_Interface &interface,
                             bool &frameRemainsInMap) {
        return stageWithFrame(
            handle, interface, frameRemainsInMap,
            [this, handle, owner, &interface](
                const DRW_DwgFramePublication &publication) {
                return stagePendingSeqEnd(handle, owner, publication,
                                          interface);
            });
    }

    bool validateState() { return validateStagedCompoundState(); }
    bool hasOrphan(std::uint32_t owner) const {
        return m_orphanPolylineVertexStates.find(owner) !=
               m_orphanPolylineVertexStates.end();
    }
    bool hasPending(std::uint32_t owner) const {
        return m_pendingPolylineStates.find(owner) !=
               m_pendingPolylineStates.end();
    }
    bool hasPendingInsert(std::uint32_t owner) const {
      return m_pendingInsertStates.find(owner) != m_pendingInsertStates.end();
    }
    bool hasOrphanAttributeOwner(std::uint32_t owner) const {
      return m_orphanAttribStates.find(owner) != m_orphanAttribStates.end();
    }
    std::size_t pendingAttributeCount(std::uint32_t owner) const {
      const auto found = m_pendingInsertStates.find(owner);
      return found == m_pendingInsertStates.end()
                 ? 0u
                 : found->second.attributes.size();
    }
    std::size_t orphanAttributeCount(std::uint32_t owner) const {
      const auto found = m_orphanAttribStates.find(owner);
      return found == m_orphanAttribStates.end()
                 ? 0u
                 : found->second.attributes.size();
    }
    std::size_t stagedAttributeIndexSize() const {
      return m_stagedAttributeIndex.size();
    }
    void setExpectedAttributeBudgetForTest(std::size_t count) {
      m_stagedInsertExpectedAttributeCount = count;
    }
    void terminalizeInsert(std::uint32_t owner) {
      terminalizeInsertGroup(owner, DwgInsertTerminalReason::MalformedGroup);
    }
    void terminalizeOrphanAttributes(std::uint32_t owner) {
      terminalizeOrphanAttribOwner(owner);
    }
    std::size_t pendingVertexCount(std::uint32_t owner) const {
        const auto found = m_pendingPolylineStates.find(owner);
        return found == m_pendingPolylineStates.end()
                   ? 0u
                   : found->second.vertices.size();
    }
    bool hasOrphanClaim(std::uint32_t owner, std::uint32_t child) const {
        return m_polylineOrphanClaims.containsOwner(owner, child);
    }
    std::size_t stagingBytes() const { return m_polylineStagingBytes; }
    std::size_t ownerMapBucketCount() const {
        return m_pendingPolylineStates.bucket_count() +
               m_orphanPolylineVertexStates.bucket_count();
    }
    std::size_t ownerMapBucketBytes() const {
        return ownerMapBucketCount() * sizeof(void *);
    }
    bool stagedStateIsEmpty() const {
        return m_pendingPolylineStates.empty() &&
               m_orphanPolylineVertexStates.empty() &&
               m_polylineDeclaredClaims.empty() &&
               m_polylineOrphanClaims.empty() &&
               m_polylineStagingBytes == ownerMapBucketBytes();
    }
    bool abandonState() { return abandonStagedCompoundState(); }
    void failStageAt(StageFailure point) {
        setPolylineStageFailurePointForTest(point);
    }
    void terminalize(std::uint32_t owner) {
        terminalizePendingPolylineState(owner,
                                        DwgInsertTerminalReason::MalformedGroup);
    }
    void finalizeCoverage(DRW_Interface &interface, bool readCompleted) {
        finalizeDwgFrameCoverage(interface, readCompleted);
    }
    const std::vector<DRW_DwgFrameCoverageEntry>& coverageEntries() const {
        return m_dwgSourceFrameLedger;
    }
    bool coverageIntegrityViolation() const {
        return m_dwgFrameCoverageIntegrityViolation;
    }

private:
    std::uint32_t m_residualRestoreHandle{DRW::NoHandle};
    std::size_t m_residualRestoreAttempts{0u};
    std::size_t m_residualReadAttempts{0u};

    template <typename Operation>
    Outcome stageWithFrame(std::uint32_t handle, DRW_Interface &interface,
                           bool &frameRemainsInMap, Operation operation) {
        (void)interface;
        const auto found = ObjectMap.find(handle);
        if (found == ObjectMap.end()) {
            frameRemainsInMap = false;
            return Outcome::Rejected;
        }
        DwgFrameMapLease activeLease;
        if (!detachDwgSourceFrame(ObjectMap, found, activeLease)) {
            frameRemainsInMap = ObjectMap.find(handle) != ObjectMap.end();
            return Outcome::Rejected;
        }
        DRW_DwgFramePublication publication;
        publication.m_handle = handle;
        publication.m_version = version;
        publication.m_sourceOffset = activeLease.source.offset;
        publication.m_sourceMapOrdinal = activeLease.source.ordinal;
        publication.m_sourceOffsetSpace = activeLease.source.offsetSpace;
        publication.m_hasSourceLocation = activeLease.hasCoverage;
        publication.m_encodedType = static_cast<int>(activeLease.object.type);
        publication.m_resolvedType = publication.m_encodedType;
        publication.m_isEntity = true;
        publication.m_carrier = DRW_DwgFramePublication::Carrier::Typed;
        DwgFrameMapLease *const previousLease = m_activeEntityFrameLease;
        m_activeEntityFrameLease = &activeLease;
        Outcome outcome = Outcome::Rejected;
        try {
            outcome = operation(publication);
        } catch (...) {
            outcome = Outcome::Rejected;
        }
        m_activeEntityFrameLease = previousLease;
        if (activeLease.isDetached())
            (void)restoreDwgSourceFrame(activeLease);
        frameRemainsInMap = ObjectMap.find(handle) != ObjectMap.end();
        return outcome;
    }
};

class PolylinePublicationSink final : public dx_iface {
public:
    void addInsert(const DRW_Insert& value) override {
        inserts.push_back(value);
    }
    void addPolyline(const DRW_Polyline& value) override {
        polylines.push_back(value);
    }
    void addDwgFramePublication(
        const DRW_DwgFramePublication& value) override {
        framePublications.push_back(value);
    }
    void addDwgFrameCoverageReport(
        const DRW_DwgFrameCoverageReport& value) override {
        coverageReports.push_back(value);
    }

    std::vector<DRW_Insert> inserts;
    std::vector<DRW_Polyline> polylines;
    std::vector<DRW_DwgFramePublication> framePublications;
    std::vector<DRW_DwgFrameCoverageReport> coverageReports;
};

class DrwEntityEncodeTestAccess {
public:
    static void addPolylineHandle(DRW_Polyline& polyline,
                                  std::uint32_t handle) {
        polyline.hadlesList.push_back(handle);
    }

    static bool encodeBlock(DRW_Block& block, DRW::Version version,
                            dwgBufferW* body, dwgBufferW* strings,
                            dwgBufferW* handles) {
        return block.encodeDwg(version, body, 0u, strings, handles);
    }

    static bool encodeLine(DRW_Line& line, DRW::Version version,
                           dwgBufferW* body, dwgBufferW* strings,
                           dwgBufferW* handles) {
        line.dwgEedAppIdWriteRefs = {{0u, 0x123u}, {3u, 0x456u}};
        line.dwgEedLayerWriteRefs.clear();
        return line.encodeDwg(version, body, 0u, strings, handles);
    }

    static bool encodePolyline(DRW_Polyline& polyline, DRW::Version version,
                               dwgBufferW* body, dwgBufferW* handles) {
        return polyline.encodeDwg(version, body, 0u, nullptr, handles);
    }

    static bool encodeVertex(DRW_Vertex& vertex, DRW::Version version,
                             dwgBufferW* body, dwgBufferW* handles) {
        return vertex.encodeDwg(version, body, 0u, nullptr, handles);
    }

    static bool encodeSequenceEnd(DRW_SeqEnd& sequenceEnd,
                                  DRW::Version version, dwgBufferW* body,
                                  dwgBufferW* handles) {
        return sequenceEnd.encodeDwg(version, body, 0u, nullptr, handles);
    }

    static bool encodeViewport(DRW_Viewport& viewport, DRW::Version version,
                               dwgBufferW* body, dwgBufferW* strings,
                               dwgBufferW* handles) {
        return viewport.encodeDwg(version, body, 0u, strings, handles);
    }

    static bool corruptSecondEedBinaryLength(dwgBufferW& body,
                                              std::uint8_t replacement) {
        std::vector<std::uint8_t>& bytes = body.data();
        if (bytes.empty())
            return false;
        dwgBuffer cursor(bytes.data(), bytes.size());
        if (cursor.getObjType(DRW::AC1027) != 19
            || !cursor.getHandle().ref)
            return false;
        for (unsigned int chunk = 0; chunk < 2u; ++chunk) {
            const std::uint16_t dataSize = cursor.getBitShort();
            const dwgHandle app = cursor.getHandle();
            if (!cursor.isGood() || dataSize == 0u || app.ref == 0u)
                return false;
            if (chunk == 0u) {
                std::vector<std::uint8_t> skipped(dataSize);
                if (!cursor.getBytes(skipped.data(), skipped.size())
                    || !cursor.isGood())
                    return false;
                continue;
            }
            if (cursor.getRawChar8() != 4u || !cursor.isGood())
                return false;
            const std::uint64_t lengthStart =
                cursor.getPosition() * 8u + cursor.getBitPos();
            if (cursor.getRawChar8() != 2u || !cursor.isGood())
                return false;
            for (unsigned int bit = 0; bit < 8u; ++bit) {
                const std::uint64_t target = lengthStart + bit;
                const std::size_t byteIndex =
                    static_cast<std::size_t>(target / 8u);
                const unsigned int bitIndex =
                    static_cast<unsigned int>(target & 7u);
                if (byteIndex >= bytes.size())
                    return false;
                const std::uint8_t mask = static_cast<std::uint8_t>(
                    1u << (7u - bitIndex));
                const bool set =
                    (replacement & (1u << (7u - bit))) != 0u;
                bytes[byteIndex] = static_cast<std::uint8_t>(
                    set ? (bytes[byteIndex] | mask)
                        : (bytes[byteIndex] & static_cast<std::uint8_t>(~mask)));
            }
            return true;
        }
        return false;
    }
};

namespace {

static_assert(static_cast<unsigned>(DwgIntegrityCheckKind::TemplateDescriptionUnsupported) == 20u,
              "existing Template integrity categories must not be renumbered");
static_assert(static_cast<unsigned>(DwgIntegrityCheckKind::HeaderSectionBounds) == 21u
                  && static_cast<unsigned>(DwgIntegrityCheckKind::HeaderSectionSentinel) == 22u
                  && static_cast<unsigned>(DwgIntegrityCheckKind::HeaderSectionCrc) == 23u,
              "HEADER integrity categories append to the stable diagnostic contract");
static_assert(static_cast<unsigned>(DwgIntegrityCheckKind::HeaderSectionCompatibility) == 24u
                  && static_cast<unsigned>(DwgIntegrityCheckKind::HeaderSectionUnclaimedData) == 25u,
              "compatible HEADER observations append without renumbering prior categories");
static_assert(static_cast<unsigned>(DwgIntegrityCheckKind::NamedObjectDictionaryReference) == 26u,
              "NOD reference observations append without renumbering prior categories");

struct TestContext {
    int failures {0};

    void expect(bool condition, const std::string& label) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << label << '\n';
        }
    }
};

std::uint64_t r2007CrcBitwiseReference(std::uint64_t seed,
                                       const std::uint8_t* data,
                                       std::size_t size, bool mirrored) {
    const std::uint64_t polynomial = mirrored
        ? 0xc96c5795d7870f42ULL : 0x42f0e1eba9ea3693ULL;
    const auto updateByte = [&](std::uint8_t byte) {
        if (mirrored) {
            seed ^= byte;
            for (int bit = 0; bit < 8; ++bit)
                seed = (seed & 1u) != 0u
                    ? (seed >> 1u) ^ polynomial : seed >> 1u;
        } else {
            seed ^= static_cast<std::uint64_t>(byte) << 56u;
            for (int bit = 0; bit < 8; ++bit)
                seed = (seed & (std::uint64_t{1} << 63u)) != 0u
                    ? (seed << 1u) ^ polynomial : seed << 1u;
        }
    };
    std::size_t offset = 0u;
    while (size - offset >= 8u) {
        for (const std::size_t index : {6u, 7u, 4u, 5u, 2u, 3u, 0u, 1u})
            updateByte(data[offset + index]);
        offset += 8u;
    }
    const std::size_t remainder = size - offset;
    if (remainder == 4u) {
        for (const std::size_t index : {2u, 3u, 0u, 1u})
            updateByte(data[offset + index]);
    } else {
        for (std::size_t index = 0u; index < remainder; ++index)
            updateByte(data[offset + index]);
    }
    return mirrored ? seed : ~seed;
}

// Source-derived diagnostic only: mirrors LibreDWG's current decode_rs()
// gather of the systematic bytes from interleaved AC1021 codewords. It
// intentionally does not check or correct parity bytes.
std::vector<std::uint8_t> r2007DeinterleaveReference(
    const std::uint8_t* encoded, std::size_t encodedSize,
    std::uint32_t blockCount, std::size_t messageSize) {
    constexpr std::size_t codewordSize = 255u;
    const auto maxSize = std::numeric_limits<std::size_t>::max();
    if (encoded == nullptr || blockCount == 0u || messageSize == 0u
        || messageSize >= codewordSize
        || static_cast<std::uint64_t>(blockCount)
            > static_cast<std::uint64_t>(maxSize / codewordSize))
        return {};
    const std::size_t blockCountSize = static_cast<std::size_t>(blockCount);
    const std::size_t requiredSize = blockCountSize * codewordSize;
    if (encodedSize < requiredSize
        || blockCountSize > maxSize / messageSize)
        return {};

    std::vector<std::uint8_t> payload(blockCountSize * messageSize);
    for (std::uint32_t block = 0u; block < blockCount; ++block) {
        for (std::size_t byte = 0u; byte < messageSize; ++byte) {
            payload[static_cast<std::size_t>(block) * messageSize + byte] =
                encoded[byte * blockCountSize + block];
        }
    }
    return payload;
}

// ODA v5.4.1 §5.11's ten-bit CRC-random-encoding decode map, kept separate
// from the production helper so the manual page probe can cross-check it.
std::uint64_t r2007DecodeCrcSeedReference(std::uint64_t encoded) {
    const auto high = static_cast<std::uint32_t>(encoded >> 32u);
    const auto low = static_cast<std::uint32_t>(encoded);
    std::uint64_t seed = 0u;
    if ((high & 0x08000000u) != 0u) seed |= 0x001u;
    if ((high & 0x00200000u) != 0u) seed |= 0x002u;
    if ((high & 0x00008000u) != 0u) seed |= 0x004u;
    if ((high & 0x00000200u) != 0u) seed |= 0x008u;
    if ((high & 0x00000008u) != 0u) seed |= 0x010u;
    if ((low & 0x20000000u) != 0u) seed |= 0x020u;
    if ((low & 0x00800000u) != 0u) seed |= 0x040u;
    if ((low & 0x00020000u) != 0u) seed |= 0x080u;
    if ((low & 0x00000800u) != 0u) seed |= 0x100u;
    if ((low & 0x00000020u) != 0u) seed |= 0x200u;
    return seed;
}

std::uint64_t r2007UpdateSeed1Reference(std::uint64_t seed,
                                        std::uint64_t dataLength) {
    seed = (seed + dataLength) * 0x343fdULL + 0x269ec3ULL;
    seed |= seed * (0x343fdULL << 32u) + (0x269ec3ULL << 32u);
    return ~seed;
}

struct RangeComparisonCounter {
    explicit RangeComparisonCounter(std::size_t* count = nullptr) : count(count) {}
    bool operator()(std::uint64_t lhs, std::uint64_t rhs) const noexcept {
        if (count != nullptr)
            ++*count;
        return lhs < rhs;
    }
    std::size_t* count;
};

struct NameComparisonCounter {
    explicit NameComparisonCounter(std::size_t* count = nullptr) : count(count) {}
    bool operator()(const std::string& lhs, const std::string& rhs) const noexcept {
        if (count != nullptr)
            ++*count;
        return lhs < rhs;
    }
    std::size_t* count;
};

struct RangeAllocationFailure {
    static bool failNext;
};

bool RangeAllocationFailure::failNext = false;

template <typename T>
struct ThrowOnceAllocator {
    using value_type = T;
    template <typename U> struct rebind { using other = ThrowOnceAllocator<U>; };
    ThrowOnceAllocator() = default;
    template <typename U>
    ThrowOnceAllocator(const ThrowOnceAllocator<U>&) noexcept {}

    T* allocate(std::size_t count) {
        if (RangeAllocationFailure::failNext) {
            RangeAllocationFailure::failNext = false;
            throw std::bad_alloc();
        }
        return std::allocator<T>{}.allocate(count);
    }
    void deallocate(T* value, std::size_t count) noexcept {
        std::allocator<T>{}.deallocate(value, count);
    }
    template <typename U> bool operator==(const ThrowOnceAllocator<U>&) const noexcept {
        return true;
    }
    template <typename U> bool operator!=(const ThrowOnceAllocator<U>&) const noexcept {
        return false;
    }
};

struct AllocationAccounting {
    std::size_t liveBytes{0u};
    std::size_t peakBytes{0u};
    std::size_t allocationCalls{0u};
    bool deallocationUnderflow{false};
};

template <typename T>
struct AllocationAccountingAllocator {
    using value_type = T;
    template <typename U>
    struct rebind { using other = AllocationAccountingAllocator<U>; };

    explicit AllocationAccountingAllocator(
        AllocationAccounting* accounting = nullptr) noexcept
        : m_accounting(accounting) {}
    template <typename U>
    AllocationAccountingAllocator(
        const AllocationAccountingAllocator<U>& other) noexcept
        : m_accounting(other.accounting()) {}

    T* allocate(std::size_t count) {
        if (count > std::numeric_limits<std::size_t>::max() / sizeof(T))
            throw std::bad_alloc();
        const std::size_t bytes = count * sizeof(T);
        T* result = std::allocator<T>{}.allocate(count);
        if (m_accounting != nullptr) {
            if (bytes > std::numeric_limits<std::size_t>::max()
                            - m_accounting->liveBytes) {
                std::allocator<T>{}.deallocate(result, count);
                throw std::bad_alloc();
            }
            m_accounting->liveBytes += bytes;
            m_accounting->peakBytes = std::max(
                m_accounting->peakBytes, m_accounting->liveBytes);
            ++m_accounting->allocationCalls;
        }
        return result;
    }

    void deallocate(T* value, std::size_t count) noexcept {
        if (m_accounting != nullptr) {
            const std::size_t bytes = count * sizeof(T);
            if (bytes > m_accounting->liveBytes)
                m_accounting->deallocationUnderflow = true;
            else
                m_accounting->liveBytes -= bytes;
        }
        std::allocator<T>{}.deallocate(value, count);
    }

    AllocationAccounting* accounting() const noexcept { return m_accounting; }

    template <typename U>
    bool operator==(
        const AllocationAccountingAllocator<U>& other) const noexcept {
        return m_accounting == other.accounting();
    }
    template <typename U>
    bool operator!=(
        const AllocationAccountingAllocator<U>& other) const noexcept {
        return !(*this == other);
    }

private:
    template <typename>
    friend struct AllocationAccountingAllocator;
    AllocationAccounting* m_accounting;
};

void testPolylineOwnershipPreflightIndex(TestContext& t) {
    using Index = dwgReaderDetail::PolylineOwnershipPreflightIndex;
    constexpr std::uint32_t firstOwner = 41001u;
    constexpr std::uint32_t secondOwner = 41002u;
    constexpr std::uint32_t thirdOwner = 41003u;
    constexpr std::uint32_t sharedChild = 42001u;
    Index ownership;
    const bool firstClaimsAdded =
        ownership.registerClaim(firstOwner, sharedChild, 0u,
                                dwgSafety::MaxPolylineStagingBytes) &&
        ownership.registerClaim(firstOwner, sharedChild, 0u,
                                dwgSafety::MaxPolylineStagingBytes) &&
        ownership.markInvalid(firstOwner, 0u,
                              dwgSafety::MaxPolylineStagingBytes);
    const bool competingClaimsAdded =
        ownership.registerClaim(secondOwner, sharedChild, 0u,
                                dwgSafety::MaxPolylineStagingBytes) &&
        ownership.registerClaim(thirdOwner, sharedChild, 0u,
                                dwgSafety::MaxPolylineStagingBytes);
    t.expect(firstClaimsAdded && competingClaimsAdded &&
                 ownership.isInvalid(firstOwner) &&
                 ownership.isInvalid(secondOwner) &&
                 ownership.isInvalid(thirdOwner) &&
                 ownership.invalidOwnerCount() == 3u &&
                 ownership.referenceCount() == 4u &&
                 ownership.chargedBytes() >=
                     4u * dwgReaderDetail::PolylineOwnershipReferenceChargeBytes,
             "POLYLINE ownership preflight retains every competing parent while charging repeated references");

    Index bounded;
    const std::size_t firstClaimBytes =
        dwgReaderDetail::PolylineOwnershipReferenceChargeBytes +
        dwgReaderDetail::PolylineOwnershipClaimNodeChargeBytes;
    const bool boundedFirstClaim = bounded.registerClaim(
        firstOwner, sharedChild, 0u, firstClaimBytes);
    const std::size_t beforeConflict = bounded.chargedBytes();
    const std::size_t conflictBytes =
        dwgReaderDetail::PolylineOwnershipReferenceChargeBytes +
        2u * dwgReaderDetail::PolylineClaimOwnerNodeChargeBytes;
    const bool conflictBudgetRefused = !bounded.registerClaim(
        secondOwner, sharedChild, 0u, beforeConflict + conflictBytes - 1u);
    const bool conflictRetryPassed = bounded.registerClaim(
        secondOwner, sharedChild, 0u, beforeConflict + conflictBytes);
    t.expect(boundedFirstClaim && conflictBudgetRefused &&
                 bounded.chargedBytes() == beforeConflict + conflictBytes &&
                 bounded.isInvalid(firstOwner) &&
                 bounded.isInvalid(secondOwner) && conflictRetryPassed &&
                 !bounded.registerClaim(DRW::NoHandle, sharedChild, 0u,
                                        dwgSafety::MaxPolylineStagingBytes),
             "POLYLINE preflight budget refusal is atomic and the exact retry admits conflict quarantine");
}

void testPolylineStagingAllocationRequestEstimates(TestContext& t) {
    using Handle = std::uint32_t;
    using TrackedOwnerSet = std::set<
        Handle, std::less<Handle>, AllocationAccountingAllocator<Handle>>;
    using TrackedClaimValue = std::pair<const Handle, TrackedOwnerSet>;
    using TrackedClaimMap = std::map<
        Handle, TrackedOwnerSet, std::less<Handle>,
        AllocationAccountingAllocator<TrackedClaimValue>>;

    AllocationAccounting treeAccounting;
    std::size_t ownerCount = 0u;
    std::size_t claimMapBaselineBytes = 0u;
    bool treePopulationSucceeded = true;
    {
        TrackedClaimMap claims(
            std::less<Handle>{},
            AllocationAccountingAllocator<TrackedClaimValue>(&treeAccounting));
        claimMapBaselineBytes = treeAccounting.liveBytes;
        for (Handle handle = 1u; handle <= 3u; ++handle) {
            TrackedOwnerSet owners(
                std::less<Handle>{},
                AllocationAccountingAllocator<Handle>(&treeAccounting));
            const auto inserted = claims.emplace(handle, std::move(owners));
            treePopulationSucceeded = treePopulationSucceeded
                && inserted.second;
            const Handle lastOwner = handle == 1u ? 4u : 2u;
            for (Handle owner = 1u; owner <= lastOwner; ++owner) {
                treePopulationSucceeded =
                    inserted.first->second.insert(owner).second
                    && treePopulationSucceeded;
                ++ownerCount;
            }
        }
        const std::size_t modeledTreeLiveBytes = claimMapBaselineBytes
            + claims.size()
                * dwgReaderDetail::PolylineClaimHandleNodeChargeBytes
            + ownerCount
                * dwgReaderDetail::PolylineClaimOwnerNodeChargeBytes;
        const std::size_t modeledTreePeakBytes = modeledTreeLiveBytes
            + claims.size()
                * dwgReaderDetail::PolylineClaimOwnerNodeChargeBytes;
        t.expect(treePopulationSucceeded && claims.size() == 3u
                     && ownerCount == 8u
                     && treeAccounting.peakBytes <= modeledTreePeakBytes
                     && treeAccounting.liveBytes <= modeledTreeLiveBytes
                     && treeAccounting.allocationCalls != 0u
                     && !treeAccounting.deallocationUnderflow,
                 "POLYLINE tree-node estimate covers requested map/set node bytes on this STL");
        claims.clear();
        t.expect(treeAccounting.liveBytes <= claimMapBaselineBytes
                     && !treeAccounting.deallocationUnderflow,
                 "POLYLINE tracked tree nodes release requested bytes while retaining only map control storage on clear");
    }
    t.expect(treeAccounting.liveBytes == 0u
                 && !treeAccounting.deallocationUnderflow,
             "POLYLINE map control storage releases when the map is destroyed");

    using PendingState = PolylineStageTestReader::PendingStateForAccounting;
    using OrphanState = PolylineStageTestReader::OrphanStateForAccounting;
    using PendingPair = std::pair<const Handle, PendingState>;
    using OrphanPair = std::pair<const Handle, OrphanState>;
    using TrackedPendingMap = std::unordered_map<
        Handle, PendingState, std::hash<Handle>, std::equal_to<Handle>,
        AllocationAccountingAllocator<PendingPair>>;
    using TrackedOrphanMap = std::unordered_map<
        Handle, OrphanState, std::hash<Handle>, std::equal_to<Handle>,
        AllocationAccountingAllocator<OrphanPair>>;
    AllocationAccounting ownerMapAccounting;
    bool ownerMapPopulationSucceeded = true;
    std::size_t retainedBucketBytes = 0u;
    std::size_t initialOwnerMapBytes = 0u;
    {
        TrackedPendingMap pending(
            0u, std::hash<Handle>{}, std::equal_to<Handle>{},
            AllocationAccountingAllocator<PendingPair>(&ownerMapAccounting));
        TrackedOrphanMap orphans(
            0u, std::hash<Handle>{}, std::equal_to<Handle>{},
            AllocationAccountingAllocator<OrphanPair>(&ownerMapAccounting));
        const std::size_t initialBucketsBytes =
            (pending.bucket_count() + orphans.bucket_count()) * sizeof(void *);
        // Capture actual constructor allocations separately. Some libraries
        // report an implicit bucket before allocating its storage.
        initialOwnerMapBytes = ownerMapAccounting.liveBytes;
        pending.reserve(8u);
        orphans.reserve(8u);
        const std::size_t pendingBuckets = pending.bucket_count();
        const std::size_t orphanBuckets = orphans.bucket_count();
        for (Handle handle = 1u; handle <= 8u; ++handle)
            ownerMapPopulationSucceeded = pending.emplace(handle, PendingState{}).second
                && ownerMapPopulationSucceeded;
        for (Handle handle = 101u; handle <= 108u; ++handle)
            ownerMapPopulationSucceeded = orphans.emplace(handle, OrphanState{}).second
                && ownerMapPopulationSucceeded;
        const std::size_t retainedBucketBytesAfterReserve =
            (pendingBuckets + orphanBuckets) * sizeof(void *);
        const std::size_t ownerMapNodeBytes =
            pending.size() * (sizeof(PendingPair) + 5u * sizeof(void *))
            + orphans.size() * (sizeof(OrphanPair) + 5u * sizeof(void *));
        // Include the constructor baseline and both original/replacement
        // bucket arrays in the transient rehash ceiling. The baseline captures
        // real control allocations without inferring them from bucket_count().
        const std::size_t modeledOwnerMapLive = initialOwnerMapBytes
            + retainedBucketBytesAfterReserve + ownerMapNodeBytes;
        const std::size_t modeledOwnerMapPeak = initialOwnerMapBytes
            + initialBucketsBytes + retainedBucketBytesAfterReserve
            + ownerMapNodeBytes;
        t.expect(ownerMapPopulationSucceeded && pending.size() == 8u
                     && orphans.size() == 8u
                     && pending.bucket_count() == pendingBuckets
                     && orphans.bucket_count() == orphanBuckets
                     && ownerMapAccounting.peakBytes <= modeledOwnerMapPeak
                     && ownerMapAccounting.liveBytes <= modeledOwnerMapLive
                     && ownerMapAccounting.allocationCalls != 0u
                     && !ownerMapAccounting.deallocationUnderflow,
                 "POLYLINE pending/orphan node and bucket estimates cover allocator requests without rehash");
        pending.clear();
        orphans.clear();
        retainedBucketBytes =
            (pending.bucket_count() + orphans.bucket_count()) * sizeof(void *);
        t.expect(ownerMapAccounting.liveBytes
                     <= initialOwnerMapBytes + retainedBucketBytes
                     && !ownerMapAccounting.deallocationUnderflow,
                 "POLYLINE pending/orphan clear retains no tracked node bytes beyond bucket estimates");
    }
    t.expect(ownerMapAccounting.liveBytes == 0u && retainedBucketBytes != 0u
                 && !ownerMapAccounting.deallocationUnderflow,
             "POLYLINE tracked owner-map allocations release when the map is destroyed");
}

void testDisjointRangeIndex(TestContext& t) {
    using Range = std::pair<std::uint64_t, std::uint64_t>;
    std::vector<Range> candidates;
    for (std::uint64_t start = 0; start < 6; ++start) {
        for (std::uint64_t end = start + 1; end <= 6; ++end)
            candidates.emplace_back(start, end);
    }

    bool differentialMatch = true;
    for (const auto& first : candidates) {
        for (const auto& second : candidates) {
            for (const auto& third : candidates) {
                dwgSafety::DisjointRangeIndex<> index;
                std::vector<Range> reference;
                for (const auto& range : {first, second, third}) {
                    const bool expected = std::none_of(
                        reference.cbegin(), reference.cend(),
                        [&range](const Range& existing) {
                            return range.first < existing.second
                                && existing.first < range.second;
                        });
                    const bool observed = dwgSafety::tryInsertSectionPageRange(
                        index, range.first, range.second - range.first)
                        == dwgSafety::SectionPageRangeResult::Inserted;
                    differentialMatch = differentialMatch
                        && (observed == expected);
                    if (expected)
                        reference.push_back(range);
                }
            }
        }
    }
    t.expect(differentialMatch,
             "ordered interval admission matches first-arrival vector semantics for 9,261 triples");

    dwgSafety::DisjointRangeIndex<> boundaries;
    t.expect(dwgSafety::tryInsertSectionPageRange(boundaries, 0, 4)
                     == dwgSafety::SectionPageRangeResult::Inserted
                 && dwgSafety::tryInsertSectionPageRange(boundaries, 4, 5)
                     == dwgSafety::SectionPageRangeResult::Inserted
                 && dwgSafety::tryInsertSectionPageRange(boundaries, 3, 2)
                     == dwgSafety::SectionPageRangeResult::Overlap
                 && dwgSafety::tryInsertSectionPageRange(boundaries, 0, 1)
                     == dwgSafety::SectionPageRangeResult::Overlap
                 && dwgSafety::tryInsertSectionPageRange(boundaries, 8, 2)
                     == dwgSafety::SectionPageRangeResult::Overlap
                 && dwgSafety::tryInsertSectionPageRange(boundaries, 7, 0)
                     == dwgSafety::SectionPageRangeResult::Empty,
             "interval index accepts adjacency and rejects predecessor/successor overlap and empty ranges");
    t.expect(dwgSafety::tryInsertSectionPageRange(
                 boundaries, (std::numeric_limits<std::uint64_t>::max)(), 1)
                 == dwgSafety::SectionPageRangeResult::Overflow,
             "checked section range admission rejects overflowing interval ends");

    const auto compareWork = [](std::size_t count, int order) {
        std::size_t comparisons = 0;
        dwgSafety::DisjointRangeIndex<RangeComparisonCounter> index(
            RangeComparisonCounter{&comparisons});
        for (std::size_t i = 0; i < count; ++i) {
            std::size_t slot = i;
            if (order == 1)
                slot = count - i - 1;
            else if (order == 2)
                slot = (i * 197u) % count;
            if (!index.insert(slot * 4u, slot * 4u + 2u))
                return std::numeric_limits<std::size_t>::max();
        }
        return comparisons;
    };
    for (int order = 0; order < 3; ++order) {
        const std::size_t small = compareWork(128, order);
        const std::size_t large = compareWork(512, order);
        t.expect(small < 128u * 40u && large < 512u * 40u
                     && large < small * 8u,
                 "128/512 interval comparison envelopes remain ordered-logarithmic across insertion orders");
    }

    using FailingRangeIndex = dwgSafety::DisjointRangeIndex<
        std::less<std::uint64_t>,
        ThrowOnceAllocator<std::pair<const std::uint64_t, std::uint64_t>>>;
    FailingRangeIndex recoverable;
    const bool firstInserted = dwgSafety::tryInsertSectionPageRange(
        recoverable, 0, 2)
        == dwgSafety::SectionPageRangeResult::Inserted;
    RangeAllocationFailure::failNext = true;
    const bool reportedResourceFailure = dwgSafety::tryInsertSectionPageRange(
        recoverable, 4, 6)
        == dwgSafety::SectionPageRangeResult::ResourceFailure;
    const bool retainedAfterFailure = recoverable.size() == 1;
    const bool recovered = dwgSafety::tryInsertSectionPageRange(
        recoverable, 4, 6)
        == dwgSafety::SectionPageRangeResult::Inserted;
    const bool insertedAfterRecovery = recoverable.size() == 2;
    recoverable.clear();
    t.expect(firstInserted && reportedResourceFailure && retainedAfterFailure && recovered
                 && insertedAfterRecovery && recoverable.empty(),
             "allocation failure leaves the staged range index reusable and clearable");
}

void testMappedPolylineCrossBlockOwnership(TestContext& t) {
    constexpr DRW::Version version = DRW::AC1021;
    constexpr std::uint32_t vertexType = 0x0Bu;
    constexpr std::uint32_t sequenceEndType = 0x06u;
    constexpr std::uint32_t polylineType =
        static_cast<std::uint32_t>(dwgType::POLYLINE_3D);

    const auto appendPolyline = [polylineType, version](
        PolylineStageTestReader& reader, std::vector<std::uint8_t>& source,
        std::uint32_t blockHandle, std::uint32_t parentHandle,
        std::uint32_t childHandle, std::uint32_t sequenceEndHandle,
        std::size_t childOccurrences = 1u) {
        DRW_Vertex vertex;
        vertex.handle = childHandle;
        vertex.parentHandle = blockHandle;
        vertex.setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex3D);

        DRW_Polyline polyline;
        polyline.handle = parentHandle;
        polyline.parentHandle = blockHandle;
        polyline.flags = 8;
        for (std::size_t i = 0; i < childOccurrences; ++i)
            polyline.addVertex(vertex);
        polyline.setDwgSeqEndHandle(sequenceEndHandle);
        return reader.addEncodedEntityFrame(
            source, polylineType, parentHandle,
            [&polyline, version](dwgBufferW* body, dwgBufferW* handles) {
                return DrwEntityEncodeTestAccess::encodePolyline(
                    polyline, version, body, handles);
            });
    };
    const auto appendVertex = [vertexType, version](
        PolylineStageTestReader& reader, std::vector<std::uint8_t>& source,
        std::uint32_t blockHandle, std::uint32_t vertexHandle) {
        DRW_Vertex vertex;
        vertex.handle = vertexHandle;
        vertex.parentHandle = blockHandle;
        vertex.setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex3D);
        return reader.addEncodedEntityFrame(
            source, vertexType, vertexHandle,
            [&vertex, version](dwgBufferW* body, dwgBufferW* handles) {
                return DrwEntityEncodeTestAccess::encodeVertex(
                    vertex, version, body, handles);
            });
    };
    const auto appendSequenceEnd = [sequenceEndType, version](
        PolylineStageTestReader& reader, std::vector<std::uint8_t>& source,
        std::uint32_t blockHandle, std::uint32_t sequenceEndHandle) {
        DRW_SeqEnd sequenceEnd;
        sequenceEnd.handle = sequenceEndHandle;
        sequenceEnd.parentHandle = blockHandle;
        return reader.addEncodedEntityFrame(
            source, sequenceEndType, sequenceEndHandle,
            [&sequenceEnd, version](dwgBufferW* body, dwgBufferW* handles) {
                return DrwEntityEncodeTestAccess::encodeSequenceEnd(
                    sequenceEnd, version, body, handles);
            });
    };

    PolylineStageTestReader uniqueReader;
    std::vector<std::uint8_t> uniqueSource;
    DRW_Block_Record uniqueBlock;
    uniqueBlock.handle = 0x900u;
    constexpr std::uint32_t uniqueParent = 0x100u;
    constexpr std::uint32_t uniqueChild = 0x101u;
    constexpr std::uint32_t uniqueSequenceEnd = 0x102u;
    DwgBlockOwnershipTestAccess::setEntityHandles(uniqueBlock,
                                                  {uniqueParent});
    const bool uniqueFramesEncoded =
        appendPolyline(uniqueReader, uniqueSource, uniqueBlock.handle,
                       uniqueParent, uniqueChild, uniqueSequenceEnd) &&
        appendVertex(uniqueReader, uniqueSource, uniqueBlock.handle,
                     uniqueChild) &&
        appendSequenceEnd(uniqueReader, uniqueSource, uniqueBlock.handle,
                          uniqueSequenceEnd);
    const std::vector<const DRW_Block_Record*> uniqueRecords{&uniqueBlock};
    const bool uniqueOwnerAccepted = uniqueFramesEncoded &&
        uniqueReader.preflightPolylineOwnership(uniqueRecords, uniqueSource);
    t.expect(uniqueFramesEncoded,
             "unique-owner POLYLINE, VERTEX, and SEQEND source frames encode");
    t.expect(uniqueOwnerAccepted,
             "unique-owner source frames pass mapped ownership preflight");
    t.expect(uniqueOwnerAccepted && uniqueReader.ObjectMap.size() == 3u &&
                 uniqueReader.entityParseFailures() == 0u &&
                 !uniqueReader.hasInvalidPolylineOwner(uniqueParent),
             "source-framed 3D POLYLINE with a unique block owner passes ownership preflight");

    PolylineStageTestReader sharedReader;
    std::vector<std::uint8_t> sharedSource;
    DRW_Block_Record firstBlock;
    DRW_Block_Record secondBlock;
    firstBlock.handle = 0x910u;
    secondBlock.handle = 0x911u;
    constexpr std::uint32_t firstParent = 0x110u;
    constexpr std::uint32_t secondParent = 0x111u;
    constexpr std::uint32_t sharedChild = 0x120u;
    constexpr std::uint32_t firstSequenceEnd = 0x121u;
    constexpr std::uint32_t secondSequenceEnd = 0x122u;
    DwgBlockOwnershipTestAccess::setEntityHandles(firstBlock, {firstParent});
    DwgBlockOwnershipTestAccess::setEntityHandles(secondBlock, {secondParent});
    const bool sharedFramesEncoded =
        appendPolyline(sharedReader, sharedSource, firstBlock.handle,
                       firstParent, sharedChild, firstSequenceEnd) &&
        appendPolyline(sharedReader, sharedSource, secondBlock.handle,
                       secondParent, sharedChild, secondSequenceEnd) &&
        appendVertex(sharedReader, sharedSource, firstBlock.handle,
                     sharedChild) &&
        appendSequenceEnd(sharedReader, sharedSource, firstBlock.handle,
                          firstSequenceEnd) &&
        appendSequenceEnd(sharedReader, sharedSource, secondBlock.handle,
                          secondSequenceEnd);
    const std::vector<const DRW_Block_Record*> sharedRecords{
        &firstBlock, &secondBlock};
    const bool sharedOwnerRejected = sharedFramesEncoded &&
        !sharedReader.preflightPolylineOwnership(sharedRecords, sharedSource);
    t.expect(sharedFramesEncoded,
             "cross-block POLYLINE, shared VERTEX, and SEQEND source frames encode");
    t.expect(sharedOwnerRejected,
             "cross-block shared child is rejected by mapped ownership preflight");
    bool diagnosticsMatch = sharedReader.entityFailures().size() == 2u;
    for (const auto& diagnostic : sharedReader.entityFailures()) {
        const bool firstMatches =
            diagnostic.handle == firstParent &&
            diagnostic.blockRecordHandle == firstBlock.handle;
        const bool secondMatches =
            diagnostic.handle == secondParent &&
            diagnostic.blockRecordHandle == secondBlock.handle;
        diagnosticsMatch = diagnosticsMatch &&
            diagnostic.type == dwgType::POLYLINE_3D &&
            diagnostic.phase ==
                dwgReader::DwgEntityFailurePhase::Aggregate &&
            (firstMatches || secondMatches);
    }
    t.expect(sharedOwnerRejected && sharedReader.ObjectMap.empty() &&
                 sharedReader.entityParseFailures() == 2u &&
                 sharedReader.hasInvalidPolylineOwner(firstParent) &&
                 sharedReader.hasInvalidPolylineOwner(secondParent) &&
                 diagnosticsMatch,
             "cross-block shared child quarantines both actual POLYLINE frames, their child and SEQEND frames, and attributes both failures");

    PolylineStageTestReader duplicateReader;
    std::vector<std::uint8_t> duplicateSource;
    DRW_Block_Record duplicateBlock;
    duplicateBlock.handle = 0x920u;
    constexpr std::uint32_t duplicateParent = 0x130u;
    constexpr std::uint32_t duplicateChild = 0x131u;
    constexpr std::uint32_t duplicateSequenceEnd = 0x132u;
    DwgBlockOwnershipTestAccess::setEntityHandles(duplicateBlock,
                                                  {duplicateParent});
    const bool duplicateFramesEncoded =
        appendPolyline(duplicateReader, duplicateSource,
                       duplicateBlock.handle, duplicateParent,
                       duplicateChild, duplicateSequenceEnd, 2u) &&
        appendVertex(duplicateReader, duplicateSource,
                     duplicateBlock.handle, duplicateChild) &&
        appendSequenceEnd(duplicateReader, duplicateSource,
                          duplicateBlock.handle, duplicateSequenceEnd);
    const std::vector<const DRW_Block_Record*> duplicateRecords{
        &duplicateBlock};
    const bool duplicateChildRejected = duplicateFramesEncoded &&
        !duplicateReader.preflightPolylineOwnership(duplicateRecords,
                                                   duplicateSource);
    const auto duplicateFailure = duplicateReader.entityFailures().begin();
    const bool duplicateDiagnosticMatches =
        duplicateReader.entityFailures().size() == 1u &&
        duplicateFailure != duplicateReader.entityFailures().end() &&
        duplicateFailure->handle == duplicateParent &&
        duplicateFailure->blockRecordHandle == duplicateBlock.handle &&
        duplicateFailure->type == dwgType::POLYLINE_3D &&
        duplicateFailure->phase ==
            dwgReader::DwgEntityFailurePhase::Aggregate;
    t.expect(duplicateFramesEncoded,
             "same-owner duplicate-child POLYLINE source frames encode");
    t.expect(duplicateChildRejected,
             "same-owner duplicate VERTEX references are rejected by mapped ownership preflight");
    t.expect(duplicateChildRejected && duplicateReader.ObjectMap.empty() &&
                 duplicateReader.entityParseFailures() == 1u &&
                 duplicateReader.hasInvalidPolylineOwner(duplicateParent) &&
                 duplicateDiagnosticMatches,
             "same-owner duplicate child quarantines the parent, child, and SEQEND and records one owner-scoped aggregate failure");

    PolylineStageTestReader sameBlockReader;
    std::vector<std::uint8_t> sameBlockSource;
    DRW_Block_Record sameBlock;
    sameBlock.handle = 0x930u;
    constexpr std::uint32_t sameBlockFirstParent = 0x140u;
    constexpr std::uint32_t sameBlockSecondParent = 0x141u;
    constexpr std::uint32_t sameBlockSharedChild = 0x142u;
    constexpr std::uint32_t sameBlockFirstSequenceEnd = 0x143u;
    constexpr std::uint32_t sameBlockSecondSequenceEnd = 0x144u;
    DwgBlockOwnershipTestAccess::setEntityHandles(
        sameBlock, {sameBlockFirstParent, sameBlockSecondParent});
    const bool sameBlockFramesEncoded =
        appendPolyline(sameBlockReader, sameBlockSource, sameBlock.handle,
                       sameBlockFirstParent, sameBlockSharedChild,
                       sameBlockFirstSequenceEnd) &&
        appendPolyline(sameBlockReader, sameBlockSource, sameBlock.handle,
                       sameBlockSecondParent, sameBlockSharedChild,
                       sameBlockSecondSequenceEnd) &&
        appendVertex(sameBlockReader, sameBlockSource, sameBlock.handle,
                     sameBlockSharedChild) &&
        appendSequenceEnd(sameBlockReader, sameBlockSource, sameBlock.handle,
                          sameBlockFirstSequenceEnd) &&
        appendSequenceEnd(sameBlockReader, sameBlockSource, sameBlock.handle,
                          sameBlockSecondSequenceEnd);
    const std::vector<const DRW_Block_Record*> sameBlockRecords{&sameBlock};
    const bool sameBlockConflictRejected = sameBlockFramesEncoded &&
        !sameBlockReader.preflightPolylineOwnership(sameBlockRecords,
                                                    sameBlockSource);
    bool sameBlockDiagnosticsMatch =
        sameBlockReader.entityFailures().size() == 2u;
    for (const auto& diagnostic : sameBlockReader.entityFailures()) {
        const bool firstMatches =
            diagnostic.handle == sameBlockFirstParent;
        const bool secondMatches =
            diagnostic.handle == sameBlockSecondParent;
        sameBlockDiagnosticsMatch = sameBlockDiagnosticsMatch &&
            diagnostic.blockRecordHandle == sameBlock.handle &&
            diagnostic.type == dwgType::POLYLINE_3D &&
            diagnostic.phase ==
                dwgReader::DwgEntityFailurePhase::Aggregate &&
            (firstMatches || secondMatches);
    }
    t.expect(sameBlockFramesEncoded,
             "same-block competing POLYLINE parents and shared-child source frames encode");
    t.expect(sameBlockConflictRejected,
             "same-block distinct POLYLINE parents cannot claim the same VERTEX");
    t.expect(sameBlockConflictRejected && sameBlockReader.ObjectMap.empty() &&
                 sameBlockReader.entityParseFailures() == 2u &&
                 sameBlockReader.hasInvalidPolylineOwner(sameBlockFirstParent) &&
                 sameBlockReader.hasInvalidPolylineOwner(sameBlockSecondParent) &&
                 sameBlockDiagnosticsMatch,
             "same-block child conflict quarantines both parent frames, the shared VERTEX, and both SEQENDs while preserving per-parent diagnostics");
}

void testBoundedPolylineStagingGrowth(TestContext& t) {
    testMappedPolylineCrossBlockOwnership(t);
    testPolylineOwnershipPreflightIndex(t);
    testPolylineStagingAllocationRequestEstimates(t);
    constexpr std::size_t childLimit = 512u;
    PolylineStageTestReader baselineReader;
    const std::size_t initialBucketBytes =
        baselineReader.ownerMapBucketCount() * sizeof(void *);
    t.expect(baselineReader.stagingBytes() == initialBucketBytes &&
                 baselineReader.ownerMapBucketBytes() == initialBucketBytes &&
                 baselineReader.validateState(),
             "POLYLINE byte ledger includes both owner maps' initial bucket arrays");

    std::vector<std::uint32_t> vertices;
    std::size_t vectorGrowths = 0u;
    bool vectorReservationSucceeded = true;
    for (std::size_t index = 0; index < childLimit; ++index) {
        const std::size_t oldCapacity = vertices.capacity();
        if (!dwgReaderDetail::reserveVectorForOneMore(vertices, childLimit)) {
            vectorReservationSucceeded = false;
            break;
        }
        vectorGrowths += vertices.capacity() > oldCapacity ? 1u : 0u;
        vertices.push_back(static_cast<std::uint32_t>(index));
    }
    const std::size_t fullVectorCapacity = vertices.capacity();
    const bool vectorLimitRefused =
        !dwgReaderDetail::reserveVectorForOneMore(vertices, childLimit);
    t.expect(vectorReservationSucceeded && vertices.size() == childLimit
                 && vectorGrowths <= 10u && vectorLimitRefused
                 && vertices.capacity() == fullVectorCapacity,
             "POLYLINE orphan vector uses bounded geometric growth through 512 children");

    using TestMap = std::unordered_map<std::uint32_t, std::uint32_t>;
    TestMap owners;
    std::size_t mapGrowths = 0u;
    bool mapReservationSucceeded = true;
    for (std::size_t index = 0; index < childLimit; ++index) {
        const std::size_t oldBuckets = owners.bucket_count();
        if (!dwgReaderDetail::reserveUnorderedMapForOneMore(owners, childLimit)) {
            mapReservationSucceeded = false;
            break;
        }
        mapGrowths += owners.bucket_count() > oldBuckets ? 1u : 0u;
        const auto inserted = owners.emplace(static_cast<std::uint32_t>(index),
                                             static_cast<std::uint32_t>(index));
        mapReservationSucceeded = mapReservationSucceeded && inserted.second;
    }
    const std::size_t fullMapBuckets = owners.bucket_count();
    const bool mapLimitRefused =
        !dwgReaderDetail::reserveUnorderedMapForOneMore(owners, childLimit);
    t.expect(mapReservationSucceeded && owners.size() == childLimit
                 && mapGrowths <= 12u && mapLimitRefused
                 && owners.bucket_count() == fullMapBuckets,
             "POLYLINE pending-owner map grows geometrically and respects its count cap");

    TestMap budgetedOwners;
    budgetedOwners.reserve(1u);
    while (budgetedOwners.size() < childLimit) {
        const long double currentElementCapacity =
            static_cast<long double>(budgetedOwners.bucket_count()) *
            static_cast<long double>(budgetedOwners.max_load_factor());
        const std::size_t elementsToFill =
            currentElementCapacity >= static_cast<long double>(childLimit)
                ? childLimit
                : static_cast<std::size_t>(currentElementCapacity);
        if (budgetedOwners.size() >= elementsToFill)
            break;
        const auto next = static_cast<std::uint32_t>(budgetedOwners.size());
        budgetedOwners.emplace(next, next);
    }
    const std::size_t bucketsBeforeBudgetRefusal =
        budgetedOwners.bucket_count();
    std::size_t plannedTarget = 0u;
    std::size_t upperRetainedBucketBytes = 0u;
    std::size_t upperTransientBucketBytes = 0u;
    const bool growthEnvelopeKnown =
        dwgReaderDetail::planUnorderedMapForOneMore(
            budgetedOwners, childLimit, plannedTarget) &&
        plannedTarget != 0u &&
        dwgReaderDetail::unorderedMapBucketGrowthUpperBound(
            budgetedOwners, plannedTarget, upperRetainedBucketBytes,
            upperTransientBucketBytes);
    std::size_t refusedBucketBytes = 99u;
    const bool bucketBudgetRefused =
        !dwgReaderDetail::reserveUnorderedMapForOneMoreWithinBudget(
            budgetedOwners, childLimit, 0u, 0u, refusedBucketBytes);
    std::size_t refusedPeakBucketBytes = 99u;
    const bool transientPeakBudgetRefused = growthEnvelopeKnown &&
        upperTransientBucketBytes > 0u &&
        !dwgReaderDetail::reserveUnorderedMapForOneMoreWithinBudget(
            budgetedOwners, childLimit,
            dwgSafety::MaxPolylineStagingBytes,
            upperTransientBucketBytes - 1u, refusedPeakBucketBytes);
    const bool refusalPreservedBuckets =
        budgetedOwners.bucket_count() == bucketsBeforeBudgetRefusal;
    std::size_t addedBucketBytes = 0u;
    const bool budgetedReservationSucceeded =
        dwgReaderDetail::reserveUnorderedMapForOneMoreWithinBudget(
            budgetedOwners, childLimit,
            dwgSafety::MaxPolylineStagingBytes,
            upperTransientBucketBytes, addedBucketBytes);
    const std::size_t bucketsAfterBudgetedReservation =
        budgetedOwners.bucket_count();
    const bool bucketBytesMatchGrowth =
        bucketsAfterBudgetedReservation >= bucketsBeforeBudgetRefusal &&
        addedBucketBytes ==
            (bucketsAfterBudgetedReservation - bucketsBeforeBudgetRefusal) *
                sizeof(void *);
    const bool transientPeakCoversReplacementArray =
        bucketsAfterBudgetedReservation <=
            upperTransientBucketBytes / sizeof(void *) &&
        upperRetainedBucketBytes >= addedBucketBytes;
    const auto nextBudgetedOwner =
        static_cast<std::uint32_t>(budgetedOwners.size());
    const auto budgetedInsert =
        budgetedOwners.emplace(nextBudgetedOwner, nextBudgetedOwner);
    t.expect(growthEnvelopeKnown && bucketBudgetRefused &&
                 refusedBucketBytes == 0u && transientPeakBudgetRefused &&
                 refusedPeakBucketBytes == 0u && refusalPreservedBuckets &&
                 budgetedReservationSucceeded && addedBucketBytes != 0u &&
                 bucketBytesMatchGrowth && transientPeakCoversReplacementArray &&
                 addedBucketBytes <= dwgSafety::MaxPolylineStagingBytes &&
                 budgetedInsert.second,
             "POLYLINE owner-map admission budgets retained bucket delta and conservative old-plus-replacement rehash peak before reserve");

    std::vector<std::uint32_t, ThrowOnceAllocator<std::uint32_t>> failingVector;
    RangeAllocationFailure::failNext = true;
    const bool allocationFailureRejected =
        !dwgReaderDetail::reserveVectorForOneMore(failingVector, childLimit);
    RangeAllocationFailure::failNext = false;
    t.expect(allocationFailureRejected && failingVector.empty()
                 && failingVector.capacity() == 0u,
             "POLYLINE staging reservation failure leaves vector state unchanged");

    using FailingMap = std::unordered_map<
        std::uint32_t, std::uint32_t, std::hash<std::uint32_t>,
        std::equal_to<std::uint32_t>,
        ThrowOnceAllocator<std::pair<const std::uint32_t, std::uint32_t>>>;
    FailingMap failingOwners;
    std::uint32_t nextFailingOwner = 1u;
    std::size_t failingMapGrowthTarget = 0u;
    bool failingMapPrepared = true;
    while (failingMapPrepared) {
        if (!dwgReaderDetail::planUnorderedMapForOneMore(
                failingOwners, childLimit, failingMapGrowthTarget)) {
            failingMapPrepared = false;
            break;
        }
        if (failingMapGrowthTarget != 0u && !failingOwners.empty())
            break;
        failingMapPrepared =
            dwgReaderDetail::reserveUnorderedMapForOneMore(
                failingOwners, childLimit) &&
            failingOwners.emplace(nextFailingOwner, nextFailingOwner).second;
        ++nextFailingOwner;
    }
    const std::size_t failingMapSizeBefore = failingOwners.size();
    const std::size_t failingMapBucketsBefore = failingOwners.bucket_count();
    const bool growthWasRequired = failingMapPrepared
        && !failingOwners.empty() && failingMapGrowthTarget != 0u;
    RangeAllocationFailure::failNext = true;
    const bool mapAllocationFailureRejected = growthWasRequired
        && !dwgReaderDetail::reserveUnorderedMapForOneMore(failingOwners,
                                                           childLimit);
    bool mapStatePreserved = failingOwners.size() == failingMapSizeBefore
        && failingOwners.bucket_count() == failingMapBucketsBefore
        && failingOwners.find(nextFailingOwner) == failingOwners.end();
    for (std::uint32_t owner = 1u;
         owner < nextFailingOwner && mapStatePreserved; ++owner) {
        const auto found = failingOwners.find(owner);
        mapStatePreserved = found != failingOwners.end()
            && found->second == owner;
    }
    RangeAllocationFailure::failNext = false;
    const bool mapReservationRecovered = growthWasRequired
        && dwgReaderDetail::reserveUnorderedMapForOneMore(failingOwners,
                                                          childLimit)
        && failingOwners.emplace(nextFailingOwner, nextFailingOwner).second;
    t.expect(mapAllocationFailureRejected && mapStatePreserved
                 && failingOwners.size() == failingMapSizeBefore + 1u
                 && mapReservationRecovered,
             "POLYLINE owner-map reservation failure is rejected and same map can retry");

    struct StagedVertexProbe {
        std::uint32_t handle;
    };
    struct CountingHandleLess {
        std::size_t* comparisons;
        bool operator()(std::uint32_t lhs, std::uint32_t rhs) const noexcept {
            ++*comparisons;
            return lhs < rhs;
        }
    };
    std::vector<std::uint32_t> declaredHandles;
    std::vector<StagedVertexProbe> stagedVertices;
    declaredHandles.reserve(childLimit);
    stagedVertices.reserve(childLimit);
    for (std::size_t index = 0; index < childLimit; ++index) {
        declaredHandles.push_back(static_cast<std::uint32_t>(
            (index * 197u) % childLimit + 1u));
        stagedVertices.push_back(
            {static_cast<std::uint32_t>(index + 1u)});
    }
    std::vector<std::size_t> orderedIndices;
    std::size_t handleComparisons = 0u;
    const bool orderBuilt = dwgReaderDetail::buildPolylineVertexOrder(
        declaredHandles, stagedVertices,
        [](const StagedVertexProbe& vertex) { return vertex.handle; },
        CountingHandleLess{&handleComparisons}, orderedIndices, 0u,
        dwgSafety::MaxPolylinePreparationScratchBytes);
    bool sourceOrderPreserved = orderBuilt &&
        orderedIndices.size() == declaredHandles.size();
    for (std::size_t index = 0;
         sourceOrderPreserved && index < declaredHandles.size(); ++index) {
        sourceOrderPreserved =
            stagedVertices[orderedIndices[index]].handle == declaredHandles[index];
    }
    t.expect(sourceOrderPreserved &&
                 handleComparisons < childLimit * 32u,
             "POLYLINE sorted child index preserves declared order within a bounded comparison envelope");

    std::vector<std::uint32_t> duplicateDeclarations = declaredHandles;
    duplicateDeclarations[1] = duplicateDeclarations[0];
    std::vector<std::size_t> retainedOrder{17u};
    const bool duplicateRefused = !dwgReaderDetail::buildPolylineVertexOrder(
        duplicateDeclarations, stagedVertices,
        [](const StagedVertexProbe& vertex) { return vertex.handle; },
        CountingHandleLess{&handleComparisons}, retainedOrder, 0u,
        dwgSafety::MaxPolylinePreparationScratchBytes);
    t.expect(duplicateRefused && retainedOrder == std::vector<std::size_t>{17u},
             "POLYLINE sorted child index rejects duplicate declarations without replacing prior output");

    std::vector<std::uint32_t> missingDeclarations = declaredHandles;
    missingDeclarations[0] = static_cast<std::uint32_t>(childLimit + 1u);
    const bool missingChildRefused = !dwgReaderDetail::buildPolylineVertexOrder(
        missingDeclarations, stagedVertices,
        [](const StagedVertexProbe& vertex) { return vertex.handle; },
        CountingHandleLess{&handleComparisons}, retainedOrder, 0u,
        dwgSafety::MaxPolylinePreparationScratchBytes);
    t.expect(missingChildRefused &&
                 retainedOrder == std::vector<std::size_t>{17u},
             "POLYLINE sorted child index rejects a missing handle without replacing prior output");

    const bool scratchLimitRefused =
        !dwgReaderDetail::buildPolylineVertexOrder(
            declaredHandles, stagedVertices,
            [](const StagedVertexProbe& vertex) { return vertex.handle; },
            CountingHandleLess{&handleComparisons}, retainedOrder, 0u, 1u);
    t.expect(scratchLimitRefused &&
                 retainedOrder == std::vector<std::size_t>{17u},
             "POLYLINE child index refuses an undersized scratch budget atomically");

    std::vector<std::pair<std::uint32_t, std::size_t>> declaredIndex;
    std::vector<std::uint8_t> stagedSlots;
    const bool membershipIndexBuilt = dwgReaderDetail::buildPolylineDeclaredIndex(
        declaredHandles, declaredIndex, stagedSlots,
        dwgSafety::MaxPolylineStagingBytes);
    std::size_t membershipComparisons = 0u;
    bool slotsMatchSource = membershipIndexBuilt &&
        declaredIndex.size() == declaredHandles.size() &&
        stagedSlots.size() == declaredHandles.size();
    for (const std::uint32_t declared : declaredHandles) {
        std::size_t slot = 0u;
        const bool found = dwgReaderDetail::findPolylineDeclaredChildSlot(
            declaredIndex, declared, slot, true,
            CountingHandleLess{&membershipComparisons});
        slotsMatchSource = slotsMatchSource && found &&
            slot < declaredHandles.size() && declaredHandles[slot] == declared &&
            stagedSlots[slot] == 0u;
        if (found && slot < stagedSlots.size())
            stagedSlots[slot] = 1u;
    }
    t.expect(slotsMatchSource &&
                 membershipComparisons < childLimit * 16u &&
                 std::all_of(stagedSlots.cbegin(), stagedSlots.cend(),
                             [](std::uint8_t value) { return value == 1u; }),
             "POLYLINE declared-child membership lookup is logarithmic and tracks each source slot once");

    std::vector<std::pair<std::uint32_t, std::size_t>> duplicateIndex;
    std::vector<std::uint8_t> duplicateSlots;
    const bool duplicateIndexBuilt = dwgReaderDetail::buildPolylineDeclaredIndex(
        duplicateDeclarations, duplicateIndex, duplicateSlots,
        dwgSafety::MaxPolylineStagingBytes);
    std::size_t duplicateSlot = 99u;
    const bool duplicateIndexRejectsExactOnce =
        duplicateIndexBuilt && !dwgReaderDetail::findPolylineDeclaredChildSlot(
            duplicateIndex, duplicateDeclarations[0], duplicateSlot, true,
            CountingHandleLess{&membershipComparisons});
    t.expect(duplicateIndexRejectsExactOnce && duplicateSlot == 99u,
             "POLYLINE declared index preserves duplicate declarations for exact-once rejection");

    const bool declaredIndexBudgetRefused =
        !dwgReaderDetail::buildPolylineDeclaredIndex(
            declaredHandles, declaredIndex, stagedSlots, 1u);
    t.expect(declaredIndexBudgetRefused && declaredIndex.size() == childLimit &&
                 stagedSlots.size() == childLimit &&
                 dwgReaderDetail::fitsByteBudget(100u, 20u, 120u) &&
                 !dwgReaderDetail::fitsByteBudget(100u, 21u, 120u) &&
                 !dwgReaderDetail::fitsByteBudget(121u, 0u, 120u),
             "POLYLINE membership index enforces a byte ceiling without replacing retained output");

    std::size_t claimComparisons = 0u;
    dwgReaderDetail::PolylineDeclaredClaimIndex<CountingHandleLess> claims(
        CountingHandleLess{&claimComparisons});
    constexpr std::size_t competingParentCount = 4u;
    std::array<std::uint32_t, competingParentCount> parentHandles{
        10001u, 10002u, 10003u, 10004u};
    bool allClaimsAdded = true;
    for (const std::uint32_t parent : parentHandles) {
        std::size_t added = 0u;
        allClaimsAdded = claims.addParent(
                             parent, declaredIndex, 0u,
                             dwgSafety::MaxPolylineStagingBytes, added) &&
                         allClaimsAdded;
    }
    bool everyClaimHasAllOwners = allClaimsAdded && claims.bytes() != 0u;
    for (const std::uint32_t child : declaredHandles) {
        const auto* owners = claims.ownersFor(child);
        everyClaimHasAllOwners = everyClaimHasAllOwners && owners != nullptr &&
            owners->size() == competingParentCount;
        for (const std::uint32_t parent : parentHandles) {
            everyClaimHasAllOwners = everyClaimHasAllOwners &&
                claims.containsParent(parent, child);
        }
    }
    std::size_t releasedClaimBytes = 0u;
    const bool firstClaimRemoved = claims.removeParent(
        parentHandles.front(), declaredIndex, releasedClaimBytes);
    bool remainingClaimsRetained = firstClaimRemoved &&
        releasedClaimBytes != 0u && claims.ownersFor(declaredHandles[0]) != nullptr &&
        claims.ownersFor(declaredHandles[0])->size() == competingParentCount - 1u;
    for (std::size_t index = 1u; index < parentHandles.size(); ++index) {
        std::size_t released = 0u;
        remainingClaimsRetained = claims.removeParent(
                                      parentHandles[index], declaredIndex,
                                      released) && remainingClaimsRetained;
    }
    t.expect(everyClaimHasAllOwners && remainingClaimsRetained &&
                 claims.empty() && claims.bytes() == 0u &&
                 claims.validateAccounting() &&
                 claimComparisons < competingParentCount * childLimit * 80u,
             "POLYLINE reverse claims retain every competing parent with bounded ordered-index work and exact release");

    dwgReaderDetail::PolylineDeclaredClaimIndex<CountingHandleLess>
        underBudgetClaims(CountingHandleLess{&claimComparisons});
    std::size_t underBudgetAdded = 77u;
    const bool claimBudgetRefused = !underBudgetClaims.addParent(
        10001u, declaredIndex, 0u, 1u, underBudgetAdded);
    t.expect(claimBudgetRefused && underBudgetClaims.empty() &&
                 underBudgetClaims.bytes() == 0u && underBudgetAdded == 77u,
             "POLYLINE reverse-claim budget refusal leaves no partial owner index");

    dwgReaderDetail::PolylineDeclaredClaimIndex<> duplicateClaims;
    std::size_t duplicateClaimBytes = 0u;
    const bool duplicateClaimAdded = duplicateIndexBuilt &&
        duplicateClaims.addParent(10001u, duplicateIndex, 0u,
                                  dwgSafety::MaxPolylineStagingBytes,
                                  duplicateClaimBytes);
    const auto* duplicateOwners =
        duplicateClaims.ownersFor(duplicateDeclarations[0]);
    t.expect(duplicateClaimAdded && duplicateOwners != nullptr &&
                 duplicateOwners->size() == 1u &&
                 duplicateClaims.validateAccounting(),
             "POLYLINE reverse claims count one parent once despite duplicate declaration slots");

    std::size_t orphanComparisons = 0u;
    dwgReaderDetail::PolylineDeclaredClaimIndex<CountingHandleLess>
        orphanClaims(CountingHandleLess{&orphanComparisons});
    constexpr std::uint32_t orphanOwner = 20001u;
    constexpr std::uint32_t lateForeignParent = 20002u;
    std::size_t orphanCharge = 0u;
    const bool orphanClaimAdded = orphanClaims.addOwner(
        orphanOwner, declaredHandles.front(), 0u,
        dwgSafety::MaxPolylineStagingBytes, orphanCharge);
    const std::size_t beforeLateParentLookup = orphanComparisons;
    const bool lateForeignParentRejected =
        dwgReaderDetail::hasForeignPolylineOrphanOwner(
            lateForeignParent, declaredIndex, orphanClaims);
    const bool matchingLateParentAccepted =
        !dwgReaderDetail::hasForeignPolylineOrphanOwner(
            orphanOwner, declaredIndex, orphanClaims);
    const std::size_t lateParentLookupWork =
        orphanComparisons - beforeLateParentLookup;
    const std::size_t retainedOrphanClaimBytes = orphanClaims.bytes();
    std::size_t duplicateOrphanCharge = 91u;
    const bool duplicateOrphanOwnerRefused = !orphanClaims.addOwner(
        orphanOwner, declaredHandles.front(), 0u,
        dwgSafety::MaxPolylineStagingBytes, duplicateOrphanCharge);
    const bool duplicateOrphanPreserved =
        orphanClaims.bytes() == retainedOrphanClaimBytes;
    std::size_t releasedOrphanBytes = 0u;
    const bool orphanClaimReleased = orphanClaims.removeOwner(
        orphanOwner, declaredHandles.front(), releasedOrphanBytes);
    t.expect(orphanClaimAdded && orphanCharge != 0u &&
                 lateForeignParentRejected && matchingLateParentAccepted &&
                 lateParentLookupWork < childLimit * 16u &&
                 duplicateOrphanOwnerRefused &&
                 duplicateOrphanCharge == 91u &&
                 duplicateOrphanPreserved &&
                 orphanClaimReleased &&
                 releasedOrphanBytes == retainedOrphanClaimBytes &&
                 orphanClaims.empty() && orphanClaims.bytes() == 0u &&
                 orphanClaims.validateAccounting(),
             "late POLYLINE parents reject foreign staged orphans while preserving the matching owner and exact claim accounting");

    dwgReaderDetail::PolylineDeclaredClaimIndex<> underBudgetOrphans;
    std::size_t underBudgetOrphanCharge = 113u;
    const bool orphanClaimBudgetRefused = !underBudgetOrphans.addOwner(
        orphanOwner, declaredHandles.front(), 0u, 1u,
        underBudgetOrphanCharge);
    t.expect(orphanClaimBudgetRefused && underBudgetOrphans.empty() &&
                 underBudgetOrphans.bytes() == 0u &&
                 underBudgetOrphanCharge == 113u &&
                 underBudgetOrphans.validateAccounting(),
             "orphan ownership index budget refusal is atomic and retry-safe");
}

void testResidualDwgEntitySweep(TestContext& t) {
    using Key = dwgReaderDetail::DwgResidualEntityKey;
    using SourceMap = std::unordered_map<std::uint32_t, objHandle>;
    constexpr std::size_t count = 512u;
    SourceMap objects;
    objects.reserve(count);
    for (std::size_t index = 0u; index < count; ++index) {
        const std::uint32_t handle =
            0x50000u + static_cast<std::uint32_t>((index * 197u) % count);
        const std::uint64_t ordinal =
            static_cast<std::uint64_t>((index * 73u) % count);
        const std::uint32_t location =
            static_cast<std::uint32_t>((index * 29u) % count);
        objects.emplace(handle, objHandle(0u, handle, location, ordinal));
    }

    std::vector<Key> ordered;
    std::size_t sortComparisons = 0u;
    const bool built = dwgReaderDetail::buildDwgResidualEntityOrder(
        objects, ordered, count, count * sizeof(Key), &sortComparisons);
    const bool sorted = built && ordered.size() == count &&
        std::is_sorted(ordered.cbegin(), ordered.cend(),
                       dwgReaderDetail::residualEntityKeyLess);
    t.expect(sorted && sortComparisons < count * 64u,
             "512 residual frame keys sort once within a bounded comparison envelope");

    SourceMap tiedObjects;
    tiedObjects.emplace(9u, objHandle(0u, 9u, 44u, 7u));
    tiedObjects.emplace(3u, objHandle(0u, 3u, 44u, 7u));
    tiedObjects.emplace(6u, objHandle(0u, 6u, 44u, 7u));
    std::vector<Key> tiedOrder;
    const bool tiedBuilt = dwgReaderDetail::buildDwgResidualEntityOrder(
        tiedObjects, tiedOrder, 3u, 3u * sizeof(Key));
    t.expect(tiedBuilt && tiedOrder.size() == 3u &&
                 tiedOrder[0].handle == 3u && tiedOrder[1].handle == 6u &&
                 tiedOrder[2].handle == 9u,
             "residual ordering breaks equal ordinal/location ties by handle");

    std::vector<Key> preserved{{99u, 88u, 77u}};
    const bool overCountRejected =
        !dwgReaderDetail::buildDwgResidualEntityOrder(
            objects, preserved, count - 1u, count * sizeof(Key)) &&
        preserved.size() == 1u && preserved[0].handle == 77u;
    const bool overBytesRejected =
        !dwgReaderDetail::buildDwgResidualEntityOrder(
            objects, preserved, count, count * sizeof(Key) - 1u) &&
        preserved.size() == 1u && preserved[0].handle == 77u;
    std::vector<Key> retainedCapacity;
    retainedCapacity.reserve(count);
    retainedCapacity.push_back({99u, 88u, 77u});
    const bool retainedCapacityRejected =
        !dwgReaderDetail::buildDwgResidualEntityOrder(
            objects, retainedCapacity, count, count * sizeof(Key)) &&
        retainedCapacity.size() == 1u &&
        retainedCapacity[0].handle == 77u;
    t.expect(overCountRejected && overBytesRejected &&
                 retainedCapacityRejected,
             "residual key admission rejects count, byte and simultaneous-capacity overflow atomically");

    using FailingKeyVector = std::vector<Key, ThrowOnceAllocator<Key>>;
    FailingKeyVector allocationPreserved;
    allocationPreserved.push_back({99u, 88u, 77u});
    RangeAllocationFailure::failNext = true;
    const bool allocationRejected =
        !dwgReaderDetail::buildDwgResidualEntityOrder(
            objects, allocationPreserved, count, 2u * count * sizeof(Key)) &&
        allocationPreserved.size() == 1u &&
        allocationPreserved[0].handle == 77u &&
        !RangeAllocationFailure::failNext;
    t.expect(allocationRejected,
             "residual key allocation failure preserves prior scratch and is retry-safe");

    const bool validRetry = dwgReaderDetail::canRetryResidualEntity(
        true, true, false, false, true, DRW_DwgFrameDisposition::Pending, 0u);
    const bool invalidRetry =
        !dwgReaderDetail::canRetryResidualEntity(
            false, true, false, false, true,
            DRW_DwgFrameDisposition::Pending, 0u) &&
        !dwgReaderDetail::canRetryResidualEntity(
            true, true, true, false, true,
            DRW_DwgFrameDisposition::Pending, 0u) &&
        !dwgReaderDetail::canRetryResidualEntity(
            true, true, false, true, true,
            DRW_DwgFrameDisposition::Pending, 0u) &&
        !dwgReaderDetail::canRetryResidualEntity(
            true, true, false, false, true,
            DRW_DwgFrameDisposition::Published, 1u);
    t.expect(validRetry && invalidRetry,
             "residual retry is limited to one soft unpublished frame restoration");

    class VertexSink final : public dx_iface {
    public:
        void addDwgFramePublication(
            const DRW_DwgFramePublication& value) override {
            handles.push_back(value.m_handle);
        }
        std::vector<std::uint32_t> handles;
    };
    constexpr std::uint32_t retryHandle = 0x62001u;
    PolylineStageTestReader retryReader;
    retryReader.setReaderVersionForTest(DRW::AC1015);
    std::vector<std::uint8_t> retrySource;
    DRW_Vertex retryVertex;
    retryVertex.handle = retryHandle;
    retryVertex.setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex2D);
    const bool retryFrameEncoded = retryReader.addEncodedEntityFrame(
        retrySource, dwgType::VERTEX_2D, retryHandle,
        [&retryVertex](dwgBufferW* body, dwgBufferW* handles) {
            return DrwEntityEncodeTestAccess::encodeVertex(
                retryVertex, DRW::AC1015, body, handles);
        });
    const bool retryReady = retryFrameEncoded &&
        retryReader.setSourceOrdinal(retryHandle, 17u) &&
        retryReader.seedPendingCoverageForTest(retryHandle);
    retryReader.configureResidualRestoreForTest(retryHandle, 1u);
    VertexSink retrySink;
    const bool restoredRetrySucceeded = retryReady &&
        retryReader.runEntitySweep(retrySource, retrySink);
    const auto& retryCoverage = retryReader.coverageEntries();
    const bool retryPublishedOnce = retryCoverage.size() == 1u &&
        retryCoverage[0].m_disposition == DRW_DwgFrameDisposition::Published &&
        retryCoverage[0].m_publicationCount == 1u;
    t.expect(restoredRetrySucceeded &&
                 retryReader.residualReadAttemptsForTest() == 2u &&
                 retrySink.handles == std::vector<std::uint32_t>{retryHandle} &&
                 retryReader.ObjectMap.empty() && retryPublishedOnce,
             "AC1015 legacy VERTEX restoration receives exactly one same-key retry and one publication");

    PolylineStageTestReader repeatedRestoreReader;
    repeatedRestoreReader.setReaderVersionForTest(DRW::AC1015);
    std::vector<std::uint8_t> repeatedRestoreSource;
    DRW_Vertex repeatedRestoreVertex;
    repeatedRestoreVertex.handle = retryHandle;
    repeatedRestoreVertex.setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex2D);
    const bool repeatedFrameEncoded =
        repeatedRestoreReader.addEncodedEntityFrame(
            repeatedRestoreSource, dwgType::VERTEX_2D, retryHandle,
            [&repeatedRestoreVertex](dwgBufferW* body,
                                     dwgBufferW* handles) {
                return DrwEntityEncodeTestAccess::encodeVertex(
                    repeatedRestoreVertex, DRW::AC1015, body, handles);
            });
    const bool repeatedRestoreReady = repeatedFrameEncoded &&
        repeatedRestoreReader.setSourceOrdinal(retryHandle, 17u) &&
        repeatedRestoreReader.seedPendingCoverageForTest(retryHandle);
    repeatedRestoreReader.configureResidualRestoreForTest(retryHandle, 2u);
    VertexSink repeatedRestoreSink;
    const bool repeatedRestoreAccepted = repeatedRestoreReady &&
        repeatedRestoreReader.runEntitySweep(repeatedRestoreSource,
                                             repeatedRestoreSink);
    const auto& repeatedCoverage = repeatedRestoreReader.coverageEntries();
    t.expect(!repeatedRestoreAccepted &&
                 repeatedRestoreReader.residualReadAttemptsForTest() == 2u &&
                 repeatedRestoreSink.handles.empty() &&
                 repeatedRestoreReader.ObjectMap.size() == 1u &&
                 repeatedCoverage.size() == 1u &&
                 repeatedCoverage[0].m_disposition ==
                     DRW_DwgFrameDisposition::Failed &&
                 repeatedCoverage[0].m_publicationCount == 0u,
             "a second restored legacy frame is rejected without publication or an unbounded retry");

    class LineSink final : public dx_iface {
    public:
        void addLine(const DRW_Line& value) override {
            handles.push_back(value.handle);
        }
        std::vector<std::uint32_t> handles;
    };
    constexpr std::size_t lineCount = 64u;
    PolylineStageTestReader reader;
    reader.setReaderVersionForTest(DRW::AC1021);
    std::vector<std::uint8_t> source;
    std::vector<Key> expected;
    bool framesEncoded = true;
    for (std::size_t index = 0u; index < lineCount; ++index) {
        const std::uint32_t handle =
            0x70000u + static_cast<std::uint32_t>((index * 29u) % lineCount);
        const std::uint64_t ordinal =
            static_cast<std::uint64_t>((index * 17u) % 16u);
        DRW_Line line;
        line.handle = handle;
        line.parentHandle = DRW::NoHandle;
        line.basePoint = DRW_Coord{static_cast<double>(index), 1.0, 0.0};
        line.secPoint = DRW_Coord{static_cast<double>(index + 1u), 2.0, 0.0};
        framesEncoded = framesEncoded && reader.addEncodedEntityFrame(
            source, dwgType::LINE, handle,
            [&line](dwgBufferW* body, dwgBufferW* handles) {
                return DrwEntityEncodeTestAccess::encodeLine(
                    line, DRW::AC1021, body, nullptr, handles);
            }) && reader.setSourceOrdinal(handle, ordinal);
        expected.push_back(
            {ordinal, static_cast<std::uint32_t>(index), handle});
    }
    std::sort(expected.begin(), expected.end(),
              dwgReaderDetail::residualEntityKeyLess);
    LineSink sink;
    const bool swept = framesEncoded && reader.runEntitySweep(source, sink);
    std::vector<std::uint32_t> expectedHandles;
    expectedHandles.reserve(expected.size());
    for (const auto& item : expected)
        expectedHandles.push_back(item.handle);
    if (!swept || sink.handles != expectedHandles || !reader.ObjectMap.empty()) {
        std::cerr << "residual sweep generated control: encoded=" << framesEncoded
                  << " swept=" << swept << " expected="
                  << expectedHandles.size() << " observed=" << sink.handles.size()
                  << " remaining=" << reader.ObjectMap.size()
                  << " parseFailures=" << reader.entityParseFailures() << '\n';
        for (std::size_t index = 0;
             index < std::min<std::size_t>(reader.entityFailures().size(), 4u);
             ++index) {
            const auto& failure = reader.entityFailures()[index];
            std::cerr << "failure handle=" << failure.handle
                      << " type=" << failure.type
                      << " phase=" << static_cast<unsigned>(failure.phase)
                      << '\n';
        }
    }
    t.expect(swept && sink.handles == expectedHandles &&
                 reader.ObjectMap.empty(),
             "generated AC1021 LINE sweep preserves exact sourceOrdinal/location transcript");
}

void testPolylineStageRollbackTranscript(TestContext& t) {
    constexpr std::uint32_t orphanOwner = 31001u;
    constexpr std::uint32_t foreignOwner = 31002u;
    constexpr std::uint32_t childHandle = 32001u;
    dx_iface interface;

    const auto makeParent = [](std::uint32_t owner, std::uint32_t child) {
        DRW_Polyline parent;
        parent.handle = owner;
        DrwEntityEncodeTestAccess::addPolylineHandle(parent, child);
        return parent;
    };
    const auto makeVertex = [](std::uint32_t owner, std::uint32_t child) {
        DRW_Vertex vertex;
        vertex.handle = child;
        vertex.parentHandle = owner;
        vertex.setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex2D);
        return vertex;
    };

    PolylineStageTestReader reader;
    bool vertexFrameRemains = true;
    const bool orphanFrameInserted = reader.addFrame(childHandle);
    const auto orphanOutcome = reader.stageVertex(
        makeVertex(orphanOwner, childHandle), interface, vertexFrameRemains);
    const bool orphanStageValid = orphanFrameInserted &&
        orphanOutcome == PolylineStageTestReader::Outcome::StagedCompound &&
        !vertexFrameRemains && reader.hasOrphan(orphanOwner) &&
        reader.hasOrphanClaim(orphanOwner, childHandle) &&
        reader.stagingBytes() != 0u && reader.validateState();

    bool foreignParentFrameRemains = false;
    const bool foreignParentInserted = reader.addFrame(foreignOwner);
    const auto foreignOutcome = reader.stageParent(
        makeParent(foreignOwner, childHandle), interface,
        foreignParentFrameRemains);
    const bool foreignConflictRolledBack = foreignParentInserted &&
        foreignOutcome == PolylineStageTestReader::Outcome::Rejected &&
        foreignParentFrameRemains && !reader.hasPending(foreignOwner) &&
        reader.hasOrphan(orphanOwner) &&
        reader.hasOrphanClaim(orphanOwner, childHandle) &&
        reader.validateState();
    reader.ObjectMap.erase(foreignOwner);

    bool matchingParentFrameRemains = true;
    const bool matchingParentInserted = reader.addFrame(orphanOwner);
    const auto matchingOutcome = reader.stageParent(
        makeParent(orphanOwner, childHandle), interface,
        matchingParentFrameRemains);
    const bool matchingOrphanAdopted = matchingParentInserted &&
        matchingOutcome == PolylineStageTestReader::Outcome::StagedCompound &&
        !matchingParentFrameRemains && !reader.hasOrphan(orphanOwner) &&
        !reader.hasOrphanClaim(orphanOwner, childHandle) &&
        reader.hasPending(orphanOwner) &&
        reader.pendingVertexCount(orphanOwner) == 1u &&
        reader.validateState();
    reader.terminalize(orphanOwner);
    const bool terminalizedCleanly = reader.stagedStateIsEmpty() &&
        reader.validateState();
    t.expect(orphanStageValid && foreignConflictRolledBack &&
                 matchingOrphanAdopted && terminalizedCleanly,
             "POLYLINE orphan/late-parent/adoption/terminalization releases staged charges and retains only owner-map bucket estimates");

    const std::array<PolylineStageTestReader::StageFailure, 4> failurePoints{
        PolylineStageTestReader::StageFailure::BeforeParentStateReserve,
        PolylineStageTestReader::StageFailure::BeforeParentStateInsert,
        PolylineStageTestReader::StageFailure::BeforeFrameStaging,
        PolylineStageTestReader::StageFailure::BeforeOrphanAdoption};
    const std::array<std::uint32_t, 4> failureOwners{
        33001u, 33002u, 33003u, 33004u};
    const std::array<std::uint32_t, 4> failureChildren{
        34001u, 34002u, 34003u, 34004u};
    bool failuresRolledBack = true;
    bool retriesSucceeded = true;
    for (std::size_t index = 0u; index < failurePoints.size(); ++index) {
        PolylineStageTestReader failingReader;
        const std::uint32_t owner = failureOwners[index];
        const std::uint32_t child = failureChildren[index];
        bool childRemains = true;
        bool parentRemains = false;
        const bool childAdded = failingReader.addFrame(child);
        const auto childOutcome = failingReader.stageVertex(
            makeVertex(owner, child), interface, childRemains);
        failingReader.failStageAt(failurePoints[index]);
        const bool parentAdded = failingReader.addFrame(owner);
        const auto parentOutcome = failingReader.stageParent(
            makeParent(owner, child), interface, parentRemains);
        const bool shouldRestoreParent =
            failurePoints[index] !=
            PolylineStageTestReader::StageFailure::BeforeOrphanAdoption;
        const bool rolledBack = childAdded &&
            childOutcome == PolylineStageTestReader::Outcome::StagedCompound &&
            !childRemains && parentAdded &&
            parentOutcome == PolylineStageTestReader::Outcome::Rejected &&
            parentRemains == shouldRestoreParent &&
            failingReader.stagedStateIsEmpty() &&
            !failingReader.hasOrphanClaim(owner, child) &&
            failingReader.validateState();
        failuresRolledBack = failuresRolledBack && rolledBack;

        const std::uint32_t retryOwner = owner + 100u;
        const std::uint32_t retryChild = child + 100u;
        bool retryChildRemains = true;
        bool retryParentRemains = true;
        const bool retryChildAdded = failingReader.addFrame(retryChild);
        const auto retryChildOutcome = failingReader.stageVertex(
            makeVertex(retryOwner, retryChild), interface, retryChildRemains);
        const bool retryParentAdded = failingReader.addFrame(retryOwner);
        const auto retryParentOutcome = failingReader.stageParent(
            makeParent(retryOwner, retryChild), interface,
            retryParentRemains);
        const bool retryStaged = retryChildAdded &&
            retryChildOutcome == PolylineStageTestReader::Outcome::StagedCompound &&
            !retryChildRemains && retryParentAdded &&
            retryParentOutcome == PolylineStageTestReader::Outcome::StagedCompound &&
            !retryParentRemains && failingReader.hasPending(retryOwner) &&
            failingReader.pendingVertexCount(retryOwner) == 1u &&
            failingReader.validateState();
        failingReader.terminalize(retryOwner);
        retriesSucceeded = retriesSucceeded && retryStaged &&
            failingReader.stagedStateIsEmpty() && failingReader.validateState();
    }
    t.expect(failuresRolledBack && retriesSucceeded,
             "POLYLINE stage failure injection releases every claim/lease and the same reader retries after all four adoption boundaries");

    constexpr std::size_t ownerCount = 512u;
    PolylineStageTestReader pendingGrowthReader;
    bool pendingOwnersStaged = true;
    for (std::size_t index = 0u; index < ownerCount; ++index) {
        const std::uint32_t owner =
            40000u + static_cast<std::uint32_t>(index);
        const std::uint32_t child =
            50000u + static_cast<std::uint32_t>(index);
        bool frameRemains = true;
        const bool frameAdded = pendingGrowthReader.addFrame(owner);
        const auto outcome = pendingGrowthReader.stageParent(
            makeParent(owner, child), interface, frameRemains);
        pendingOwnersStaged = pendingOwnersStaged && frameAdded &&
            outcome == PolylineStageTestReader::Outcome::StagedCompound &&
            !frameRemains && pendingGrowthReader.hasPending(owner);
    }
    const std::size_t pendingBucketCharge =
        pendingGrowthReader.ownerMapBucketBytes();
    const bool pendingBucketsCharged = pendingOwnersStaged &&
        pendingBucketCharge != 0u &&
        pendingGrowthReader.stagingBytes() >= pendingBucketCharge &&
        pendingGrowthReader.validateState();
    bool pendingOwnersTerminalized = true;
    for (std::size_t index = 0u; index < ownerCount; ++index) {
        pendingGrowthReader.terminalize(
            40000u + static_cast<std::uint32_t>(index));
        pendingOwnersTerminalized = pendingOwnersTerminalized &&
            !pendingGrowthReader.hasPending(
                40000u + static_cast<std::uint32_t>(index));
    }
    const bool pendingBucketsRetainedAndCharged =
        pendingOwnersTerminalized && pendingGrowthReader.stagedStateIsEmpty() &&
        pendingGrowthReader.stagingBytes() == pendingBucketCharge &&
        pendingGrowthReader.validateState();
    const std::uint32_t reusedPendingOwner = 59001u;
    const std::uint32_t reusedPendingChild = 69001u;
    bool reusedPendingFrameRemains = true;
    const bool reusedPendingFrameAdded =
        pendingGrowthReader.addFrame(reusedPendingOwner);
    const auto reusedPendingOutcome = pendingGrowthReader.stageParent(
        makeParent(reusedPendingOwner, reusedPendingChild), interface,
        reusedPendingFrameRemains);
    const bool pendingReaderReused = reusedPendingFrameAdded &&
        reusedPendingOutcome ==
            PolylineStageTestReader::Outcome::StagedCompound &&
        !reusedPendingFrameRemains && pendingGrowthReader.validateState();
    pendingGrowthReader.terminalize(reusedPendingOwner);
    const bool pendingReaderReuseReleased = pendingReaderReused &&
        pendingGrowthReader.stagedStateIsEmpty() &&
        pendingGrowthReader.stagingBytes() == pendingBucketCharge &&
        pendingGrowthReader.validateState();

    PolylineStageTestReader orphanGrowthReader;
    bool orphanOwnersStaged = true;
    for (std::size_t index = 0u; index < ownerCount; ++index) {
        const std::uint32_t owner =
            70000u + static_cast<std::uint32_t>(index);
        const std::uint32_t child =
            90000u + static_cast<std::uint32_t>(index);
        bool frameRemains = true;
        const bool frameAdded = orphanGrowthReader.addFrame(child);
        const auto outcome = orphanGrowthReader.stageVertex(
            makeVertex(owner, child), interface, frameRemains);
        orphanOwnersStaged = orphanOwnersStaged && frameAdded &&
            outcome == PolylineStageTestReader::Outcome::StagedCompound &&
            !frameRemains && orphanGrowthReader.hasOrphan(owner);
    }
    const std::size_t orphanBucketCharge =
        orphanGrowthReader.ownerMapBucketBytes();
    const bool orphanBucketsCharged = orphanOwnersStaged &&
        orphanBucketCharge != 0u &&
        orphanGrowthReader.stagingBytes() >= orphanBucketCharge &&
        orphanGrowthReader.validateState();
    const bool orphanOwnersAbandoned = orphanGrowthReader.abandonState();
    const bool orphanBucketsRetainedAndCharged = orphanOwnersAbandoned &&
        orphanGrowthReader.stagedStateIsEmpty() &&
        orphanGrowthReader.stagingBytes() == orphanBucketCharge &&
        orphanGrowthReader.validateState();
    const std::uint32_t reusedOrphanOwner = 81001u;
    const std::uint32_t reusedOrphanChild = 91001u;
    bool reusedOrphanFrameRemains = true;
    const bool reusedOrphanFrameAdded =
        orphanGrowthReader.addFrame(reusedOrphanChild);
    const auto reusedOrphanOutcome = orphanGrowthReader.stageVertex(
        makeVertex(reusedOrphanOwner, reusedOrphanChild), interface,
        reusedOrphanFrameRemains);
    const bool orphanReaderReused = reusedOrphanFrameAdded &&
        reusedOrphanOutcome ==
            PolylineStageTestReader::Outcome::StagedCompound &&
        !reusedOrphanFrameRemains && orphanGrowthReader.validateState();
    const bool orphanReaderReabandoned = orphanGrowthReader.abandonState();
    const bool orphanReaderReuseReleased = orphanReaderReused &&
        orphanReaderReabandoned && orphanGrowthReader.stagedStateIsEmpty() &&
        orphanGrowthReader.stagingBytes() == orphanBucketCharge &&
        orphanGrowthReader.validateState();
    t.expect(pendingBucketsCharged,
             "POLYLINE pending owner-map buckets are charged while staged");
    t.expect(pendingBucketsRetainedAndCharged,
             "POLYLINE pending owner-map buckets stay charged after terminalization");
    t.expect(pendingReaderReuseReleased,
             "POLYLINE pending map reuse retains only its charged buckets");
    t.expect(orphanBucketsCharged,
             "POLYLINE orphan owner-map buckets are charged while staged");
    t.expect(orphanBucketsRetainedAndCharged,
             "POLYLINE orphan owner-map buckets stay charged after abandonment");
    t.expect(orphanReaderReuseReleased,
             "POLYLINE orphan map reuse retains only its charged buckets");
}

std::uint16_t polylineHandleMapTestCrc(const std::vector<std::uint8_t>& bytes,
                                      std::size_t start, std::size_t end) {
    // Independent bitwise projection of the ODA reflected CRC16 algorithm.
    std::uint16_t crc = 0xC0C1u;
    for (std::size_t index = start; index < end; ++index) {
        crc ^= bytes[index];
        for (unsigned int bit = 0; bit < 8u; ++bit) {
            crc = static_cast<std::uint16_t>(
                (crc >> 1u) ^ ((crc & 1u) != 0u ? 0xA001u : 0u));
        }
    }
    return crc;
}

void appendBigEndianShort(std::vector<std::uint8_t>& bytes,
                          std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value >> 8u));
    bytes.push_back(static_cast<std::uint8_t>(value));
}

std::vector<std::uint8_t> makePolylineHandleMapVector(
    const std::array<std::uint32_t, 4>& handles,
    const std::array<std::uint32_t, 4>& offsets) {
    dwgBufferW entries;
    std::uint32_t previousHandle = 0u;
    std::uint32_t previousOffset = 0u;
    for (std::size_t index = 0u; index < handles.size(); ++index) {
        const bool wroteHandle =
            entries.putUModularChar(handles[index] - previousHandle);
        const bool wroteOffset = entries.putModularChar(
            static_cast<std::int64_t>(offsets[index]) - previousOffset);
        if (!wroteHandle || !wroteOffset || !entries.isGood())
            return {};
        previousHandle = handles[index];
        previousOffset = offsets[index];
    }

    const auto& entryBytes = entries.data();
    const std::size_t groupSize = entryBytes.size() + 2u;
    if (groupSize > (std::numeric_limits<std::uint16_t>::max)())
        return {};

    std::vector<std::uint8_t> bytes;
    bytes.reserve(groupSize + 6u);
    appendBigEndianShort(bytes, static_cast<std::uint16_t>(groupSize));
    bytes.insert(bytes.end(), entryBytes.begin(), entryBytes.end());
    appendBigEndianShort(bytes, polylineHandleMapTestCrc(bytes, 0u,
                                                          bytes.size()));

    const std::size_t terminatorStart = bytes.size();
    appendBigEndianShort(bytes, 2u);
    appendBigEndianShort(
        bytes, polylineHandleMapTestCrc(bytes, terminatorStart, bytes.size()));
    return bytes;
}

void testLocatedPolylineCallbackPublication(TestContext& t) {
    constexpr std::uint32_t parentHandle = 10101u;
    constexpr std::uint32_t firstChildHandle = 10201u;
    constexpr std::uint32_t secondChildHandle = 10202u;
    constexpr std::uint32_t sequenceHandle = 10301u;
    constexpr std::array<std::uint32_t, 4> handles{
        parentHandle, firstChildHandle, secondChildHandle, sequenceHandle};
    constexpr std::array<std::uint32_t, 4> offsets{
        0x100u, 0x180u, 0x1C0u, 0x240u};
    constexpr std::array<std::uint64_t, 4> ordinals{0u, 1u, 2u, 3u};

    PolylineStageTestReader reader;
    PolylinePublicationSink sink;
    std::vector<std::uint8_t> handleMap =
        makePolylineHandleMapVector(handles, offsets);
    const bool handleMapRead = !handleMap.empty()
        && reader.readHandleMapSection(handleMap);
    bool sourceMapExact = handleMapRead
        && reader.ObjectMap.size() == handles.size()
        && reader.coverageEntries().size() == handles.size();
    for (std::size_t index = 0u; index < handles.size(); ++index) {
        const auto mapped = reader.ObjectMap.find(handles[index]);
        sourceMapExact = sourceMapExact
            && mapped != reader.ObjectMap.end()
            && mapped->second.handle == handles[index]
            && mapped->second.loc == offsets[index]
            && mapped->second.sourceOrdinal == ordinals[index]
            && mapped->second.sourceOffsetSpace
                == DRW_DwgFrameOffsetSpace::DecodedBuffer;
        if (index < reader.coverageEntries().size()) {
            const auto& ledger = reader.coverageEntries()[index];
            sourceMapExact = sourceMapExact
                && ledger.m_handle == handles[index]
                && ledger.m_sourceOffset == offsets[index]
                && ledger.m_sourceMapOrdinal == ordinals[index]
                && ledger.m_sourceOffsetSpace
                    == DRW_DwgFrameOffsetSpace::DecodedBuffer;
        }
    }

    DRW_Vertex firstChild;
    firstChild.handle = firstChildHandle;
    firstChild.parentHandle = parentHandle;
    firstChild.basePoint.x = 1.0;
    firstChild.setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex2D);
    DRW_Vertex secondChild;
    secondChild.handle = secondChildHandle;
    secondChild.parentHandle = parentHandle;
    secondChild.basePoint.x = 2.0;
    secondChild.setDwgSubtype(DRW_Vertex::DwgSubtype::Vertex2D);
    DRW_Polyline parent;
    parent.handle = parentHandle;
    parent.setDwgSeqEndHandle(sequenceHandle);
    DrwEntityEncodeTestAccess::addPolylineHandle(parent, firstChildHandle);
    DrwEntityEncodeTestAccess::addPolylineHandle(parent, secondChildHandle);

    bool secondChildRemains = true;
    const auto secondChildOutcome = reader.stageVertex(
        secondChild, sink, secondChildRemains);
    const bool noEarlyCallbackAfterSecondChild =
        secondChildOutcome == PolylineStageTestReader::Outcome::StagedCompound
        && !secondChildRemains && sink.polylines.empty()
        && sink.framePublications.empty();

    bool firstChildRemains = true;
    const auto firstChildOutcome = reader.stageVertex(
        firstChild, sink, firstChildRemains);
    const bool noEarlyCallbackAfterFirstChild =
        firstChildOutcome == PolylineStageTestReader::Outcome::StagedCompound
        && !firstChildRemains && sink.polylines.empty()
        && sink.framePublications.empty();

    bool parentRemains = true;
    const auto parentOutcome = reader.stageParent(parent, sink, parentRemains);
    const bool parentWaitsForSeqEnd =
        parentOutcome == PolylineStageTestReader::Outcome::StagedCompound
        && !parentRemains && reader.hasPending(parentHandle)
        && sink.polylines.empty() && sink.framePublications.empty();

    bool sequenceRemains = true;
    const auto sequenceOutcome = reader.stageSequenceEnd(
        sequenceHandle, parentHandle, sink, sequenceRemains);
    const bool committed =
        sequenceOutcome == PolylineStageTestReader::Outcome::CommittedCompound
        && !sequenceRemains && !reader.hasPending(parentHandle)
        && reader.stagedStateIsEmpty() && reader.validateState()
        && !reader.coverageIntegrityViolation()
        && sink.polylines.size() == 1u
        && sink.framePublications.size() == handles.size();

    bool orderedVertices = committed
        && sink.polylines.front().vertlist.size() == 2u;
    if (orderedVertices) {
        orderedVertices = sink.polylines.front().vertlist[0] != nullptr
            && sink.polylines.front().vertlist[1] != nullptr
            && sink.polylines.front().vertlist[0]->handle == firstChildHandle
            && sink.polylines.front().vertlist[1]->handle == secondChildHandle;
    }

    bool sourceReceiptsExact = committed;
    bool frameLedgerPublishedExactlyOnce =
        reader.coverageEntries().size() == handles.size();
    for (std::size_t index = 0u; index < handles.size(); ++index) {
        const auto receipt = std::find_if(
            sink.framePublications.begin(), sink.framePublications.end(),
            [handle = handles[index]](const DRW_DwgFramePublication& value) {
                return value.m_handle == handle;
            });
        sourceReceiptsExact = sourceReceiptsExact
            && receipt != sink.framePublications.end()
            && receipt->m_version == DRW::AC1021
            && receipt->m_sourceOffset == offsets[index]
            && receipt->m_sourceMapOrdinal == ordinals[index]
            && receipt->m_sourceOffsetSpace
                == DRW_DwgFrameOffsetSpace::DecodedBuffer
            && receipt->m_hasSourceLocation
            && receipt->m_isEntity;

        const auto ledgerEntry = std::find_if(
            reader.coverageEntries().begin(), reader.coverageEntries().end(),
            [handle = handles[index]](
                const DRW_DwgFrameCoverageEntry& value) {
                return value.m_handle == handle;
            });
        frameLedgerPublishedExactlyOnce = frameLedgerPublishedExactlyOnce
            && ledgerEntry != reader.coverageEntries().end()
            && ledgerEntry->m_sourceOffset == offsets[index]
            && ledgerEntry->m_sourceMapOrdinal == ordinals[index]
            && ledgerEntry->m_sourceOffsetSpace
                == DRW_DwgFrameOffsetSpace::DecodedBuffer
            && ledgerEntry->m_disposition ==
                DRW_DwgFrameDisposition::Published
            && ledgerEntry->m_publicationCount == 1u;
    }

    reader.finalizeCoverage(sink, true);
    const bool completeReport = sink.coverageReports.size() == 1u
        && sink.coverageReports.front().m_status
            == DRW_DwgFrameCoverageStatus::FinalizedComplete
        && sink.coverageReports.front().m_complete
        && sink.coverageReports.front().m_entries.size() == handles.size();
    t.expect(sourceMapExact && noEarlyCallbackAfterSecondChild
                 && noEarlyCallbackAfterFirstChild && parentWaitsForSeqEnd
                 && committed && orderedVertices && sourceReceiptsExact
                 && frameLedgerPublishedExactlyOnce && completeReport,
             "production HANDLES parsing seeds source-located POLYLINE/child/SEQEND leases which publish one ordered callback and exact frame-coverage receipts");
}

void testInsertAttributeStagingIndex(TestContext& t) {
    constexpr std::size_t attributeCount = 257u;
    constexpr std::uint32_t parentFirstHandle = 0x21000u;
    constexpr std::uint32_t sequenceHandle = 0x22000u;
    constexpr std::uint32_t lineSentinelHandle = 0x23000u;
    const auto makeHandles = [attributeCount] {
        std::vector<std::uint32_t> handles;
        handles.reserve(attributeCount);
        for (std::size_t index = 0u; index < attributeCount; ++index) {
            // A coprime stride produces a unique, deliberately nonmonotonic
            // declaration sequence while keeping handles deterministic.
            const std::size_t slot = (index * 73u) % attributeCount;
            handles.push_back(0x30000u +
                              static_cast<std::uint32_t>(slot * 3u + 1u));
        }
        return handles;
    };
    const std::vector<std::uint32_t> declaredHandles = makeHandles();
    const auto addDeclaredHandles = [](DRW_Insert &insert,
                                       const std::vector<std::uint32_t> &handles) {
        for (const std::uint32_t handle : handles) {
            dwgHandle reference;
            reference.ref = handle;
            insert.attribHandles.push_back(reference);
        }
    };
    const auto hasDeclaredOrder = [](const DRW_Insert &insert,
                                     const std::vector<std::uint32_t> &handles) {
        if (insert.attlist.size() != handles.size())
            return false;
        for (std::size_t index = 0u; index < handles.size(); ++index) {
            if (!insert.attlist[index] ||
                insert.attlist[index]->handle != handles[index]) {
                return false;
            }
        }
        return true;
    };

    PolylineStageTestReader parentFirstReader;
    PolylinePublicationSink parentFirstSink;
    DRW_Insert parentFirst;
    parentFirst.handle = parentFirstHandle;
    parentFirst.seqendH.ref = sequenceHandle;
    addDeclaredHandles(parentFirst, declaredHandles);
    bool parentFrameRemains = true;
    bool parentFirstValid = parentFirstReader.addFrame(parentFirstHandle) &&
        parentFirstReader.addFrame(sequenceHandle) &&
        parentFirstReader.addFrame(lineSentinelHandle) &&
        parentFirstReader.stageInsert(parentFirst, parentFirstSink,
                                      parentFrameRemains) ==
            PolylineStageTestReader::Outcome::StagedCompound &&
        !parentFrameRemains &&
        parentFirstReader.pendingAttributeCount(parentFirstHandle) == 0u;
    for (std::size_t offset = attributeCount; offset > 0u; --offset) {
        const std::uint32_t handle = declaredHandles[offset - 1u];
        bool frameRemains = true;
        parentFirstValid = parentFirstValid &&
            parentFirstReader.addFrame(handle) &&
            parentFirstReader.stageAttribute(handle, parentFirstHandle, handle,
                                             parentFirstSink, frameRemains) ==
                PolylineStageTestReader::Outcome::StagedCompound &&
            !frameRemains && parentFirstSink.inserts.empty();
    }
    parentFirstValid = parentFirstValid &&
        parentFirstReader.stagedAttributeIndexSize() == attributeCount &&
        parentFirstReader.pendingAttributeCount(parentFirstHandle) ==
            attributeCount &&
        parentFirstReader.validateState();
    bool sequenceFrameRemains = true;
    parentFirstValid = parentFirstValid &&
        parentFirstReader.stageSequenceEnd(sequenceHandle, parentFirstHandle,
                                           parentFirstSink,
                                           sequenceFrameRemains) ==
            PolylineStageTestReader::Outcome::CommittedCompound &&
        !sequenceFrameRemains && parentFirstSink.inserts.size() == 1u &&
        hasDeclaredOrder(parentFirstSink.inserts.front(), declaredHandles) &&
        parentFirstReader.stagedAttributeIndexSize() == 0u &&
        parentFirstReader.ObjectMap.find(lineSentinelHandle) !=
            parentFirstReader.ObjectMap.end() &&
        parentFirstReader.validateState();

    constexpr std::uint32_t childFirstHandle = 0x24000u;
    PolylineStageTestReader childFirstReader;
    PolylinePublicationSink childFirstSink;
    bool childFirstValid = true;
    for (std::size_t offset = 0u; offset < attributeCount; ++offset) {
        const std::size_t index = (offset * 97u) % attributeCount;
        const std::uint32_t handle = declaredHandles[index];
        bool frameRemains = true;
        childFirstValid = childFirstValid && childFirstReader.addFrame(handle) &&
            childFirstReader.stageAttribute(handle, childFirstHandle, handle,
                                            childFirstSink, frameRemains) ==
                PolylineStageTestReader::Outcome::StagedCompound &&
            !frameRemains;
    }
    childFirstValid = childFirstValid &&
        childFirstReader.hasOrphanAttributeOwner(childFirstHandle) &&
        childFirstReader.orphanAttributeCount(childFirstHandle) ==
            attributeCount &&
        childFirstReader.stagedAttributeIndexSize() == attributeCount &&
        childFirstReader.validateState();
    DRW_Insert childFirst;
    childFirst.handle = childFirstHandle;
    addDeclaredHandles(childFirst, declaredHandles);
    bool lateParentFrameRemains = true;
    childFirstValid = childFirstValid &&
        childFirstReader.addFrame(childFirstHandle) &&
        childFirstReader.stageInsert(childFirst, childFirstSink,
                                     lateParentFrameRemains) ==
            PolylineStageTestReader::Outcome::CommittedCompound &&
        !lateParentFrameRemains && childFirstSink.inserts.size() == 1u &&
        hasDeclaredOrder(childFirstSink.inserts.front(), declaredHandles) &&
        !childFirstReader.hasOrphanAttributeOwner(childFirstHandle) &&
        childFirstReader.stagedAttributeIndexSize() == 0u &&
        childFirstReader.validateState();

    // A following ATTRIB with a different owner and an untouched LINE frame
    // must remain isolated from the just-committed INSERT group.
    constexpr std::uint32_t followingOwner = 0x25000u;
    constexpr std::uint32_t followingAttribute = 0x25001u;
    constexpr std::uint32_t followingLine = 0x25002u;
    bool followingAttributeRemains = true;
    const bool followingSentinelsIsolated =
        childFirstReader.addFrame(followingAttribute) &&
        childFirstReader.addFrame(followingLine) &&
        childFirstReader.stageAttribute(followingAttribute, followingOwner,
                                        followingAttribute, childFirstSink,
                                        followingAttributeRemains) ==
            PolylineStageTestReader::Outcome::StagedCompound &&
        !followingAttributeRemains &&
        childFirstReader.hasOrphanAttributeOwner(followingOwner) &&
        childFirstReader.stagedAttributeIndexSize() == 1u &&
        childFirstSink.inserts.size() == 1u &&
        childFirstReader.ObjectMap.find(followingLine) !=
            childFirstReader.ObjectMap.end() &&
        childFirstReader.validateState();
    childFirstReader.terminalizeOrphanAttributes(followingOwner);
    const bool followingCleanup =
        followingSentinelsIsolated &&
        childFirstReader.stagedAttributeIndexSize() == 0u &&
        childFirstReader.validateState();

    PolylineStageTestReader malformedReader;
    PolylinePublicationSink malformedSink;
    constexpr std::uint32_t malformedOwner = 0x26000u;
    constexpr std::uint32_t duplicateHandle = 0x26001u;
    DRW_Insert duplicateDeclaration;
    duplicateDeclaration.handle = malformedOwner;
    addDeclaredHandles(duplicateDeclaration,
                       {duplicateHandle, duplicateHandle});
    bool malformedParentRemains = false;
    const bool duplicateDeclarationRejected =
        malformedReader.addFrame(malformedOwner) &&
        malformedReader.stageInsert(duplicateDeclaration, malformedSink,
                                    malformedParentRemains) ==
            PolylineStageTestReader::Outcome::Rejected &&
        malformedParentRemains &&
        !malformedReader.hasPendingInsert(malformedOwner) &&
        malformedReader.stagedAttributeIndexSize() == 0u &&
        malformedReader.validateState();

    constexpr std::uint32_t missingOwner = 0x27000u;
    constexpr std::uint32_t missingFirst = 0x27001u;
    constexpr std::uint32_t missingSecond = 0x27002u;
    DRW_Insert missingChild;
    missingChild.handle = missingOwner;
    addDeclaredHandles(missingChild, {missingFirst, missingSecond});
    bool missingParentRemains = true;
    bool missingFirstRemains = true;
    const bool missingChildWaits =
        malformedReader.addFrame(missingOwner) &&
        malformedReader.stageInsert(missingChild, malformedSink,
                                    missingParentRemains) ==
            PolylineStageTestReader::Outcome::StagedCompound &&
        !missingParentRemains && malformedReader.addFrame(missingFirst) &&
        malformedReader.stageAttribute(missingFirst, missingOwner, missingFirst,
                                       malformedSink, missingFirstRemains) ==
            PolylineStageTestReader::Outcome::StagedCompound &&
        !missingFirstRemains && malformedSink.inserts.empty() &&
        malformedReader.stagedAttributeIndexSize() == 1u &&
        malformedReader.validateState();
    malformedReader.terminalizeInsert(missingOwner);
    const bool missingChildReleasesIndex =
        missingChildWaits &&
        !malformedReader.hasPendingInsert(missingOwner) &&
        malformedReader.stagedAttributeIndexSize() == 0u &&
        malformedReader.validateState();

    constexpr std::uint32_t duplicateOwner = 0x28000u;
    constexpr std::uint32_t duplicateChild = 0x28001u;
    constexpr std::uint32_t duplicateFrame = 0x28002u;
    constexpr std::uint32_t duplicateMissing = 0x28003u;
    DRW_Insert duplicateChildParent;
    duplicateChildParent.handle = duplicateOwner;
    addDeclaredHandles(duplicateChildParent,
                       {duplicateChild, duplicateMissing});
    bool duplicateOwnerRemains = true;
    bool firstDuplicateRemains = true;
    bool secondDuplicateRemains = false;
    const bool duplicateOwnerSetup =
        malformedReader.addFrame(duplicateOwner) &&
        malformedReader.stageInsert(duplicateChildParent, malformedSink,
                                    duplicateOwnerRemains) ==
            PolylineStageTestReader::Outcome::StagedCompound &&
        !duplicateOwnerRemains;
    const bool firstDuplicateStaged = duplicateOwnerSetup &&
        malformedReader.addFrame(duplicateChild) &&
        malformedReader.stageAttribute(duplicateChild, duplicateOwner,
                                       duplicateChild, malformedSink,
                                       firstDuplicateRemains) ==
            PolylineStageTestReader::Outcome::StagedCompound &&
        !firstDuplicateRemains;
    const auto duplicateChildOutcome = malformedReader.addFrame(duplicateFrame)
        ? malformedReader.stageAttribute(duplicateFrame, duplicateOwner,
                                         duplicateChild, malformedSink,
                                         secondDuplicateRemains)
        : PolylineStageTestReader::Outcome::StagedCompound;
    const bool duplicateChildRejected = firstDuplicateStaged &&
        duplicateChildOutcome == PolylineStageTestReader::Outcome::Rejected &&
        secondDuplicateRemains &&
        !malformedReader.hasPendingInsert(duplicateOwner) &&
        malformedReader.stagedAttributeIndexSize() == 0u &&
        malformedReader.validateState();

    constexpr std::uint32_t wrongOwner = 0x29000u;
    constexpr std::uint32_t expectedOwner = 0x29100u;
    constexpr std::uint32_t wrongOwnerAttribute = 0x29001u;
    DRW_Insert wrongOwnerParent;
    wrongOwnerParent.handle = expectedOwner;
    addDeclaredHandles(wrongOwnerParent, {wrongOwnerAttribute});
    bool wrongAttributeRemains = true;
    bool expectedParentRemains = true;
    const bool wrongAttributeStaged =
        malformedReader.addFrame(wrongOwnerAttribute) &&
        malformedReader.stageAttribute(wrongOwnerAttribute, wrongOwner,
                                       wrongOwnerAttribute, malformedSink,
                                       wrongAttributeRemains) ==
            PolylineStageTestReader::Outcome::StagedCompound &&
        !wrongAttributeRemains;
    const bool expectedOwnerPending = wrongAttributeStaged &&
        malformedReader.addFrame(expectedOwner) &&
        malformedReader.stageInsert(wrongOwnerParent, malformedSink,
                                    expectedParentRemains) ==
            PolylineStageTestReader::Outcome::StagedCompound &&
        !expectedParentRemains;
    const bool wrongOwnerDoesNotAdopt = expectedOwnerPending &&
        malformedSink.inserts.empty() &&
        malformedReader.hasOrphanAttributeOwner(wrongOwner) &&
        malformedReader.pendingAttributeCount(expectedOwner) == 0u &&
        malformedReader.validateState();
    malformedReader.terminalizeInsert(expectedOwner);
    malformedReader.terminalizeOrphanAttributes(wrongOwner);
    const bool wrongOwnerCleaned =
        wrongOwnerDoesNotAdopt &&
        malformedReader.stagedAttributeIndexSize() == 0u &&
        malformedReader.validateState();

    t.expect(parentFirstValid,
             "generated parent-first INSERT/ATTRIB chain commits declared order after SEQEND and preserves the LINE sentinel");
    t.expect(childFirstValid,
             "generated child-first INSERT/ATTRIB chain adopts orphans and commits declared order");
    t.expect(followingCleanup,
             "following ATTRIB and LINE sentinels remain isolated after INSERT commit");
    t.expect(duplicateDeclarationRejected,
             "duplicate declared ATTRIB handles are rejected without staging");
    t.expect(missingChildReleasesIndex,
             "missing declared ATTRIB keeps the INSERT pending and terminalization releases its index entry");
    t.expect(duplicateChildRejected,
             "duplicate staged ATTRIB handles terminalize the owner without leaking index entries");
    t.expect(duplicateOwnerSetup && firstDuplicateStaged &&
                 duplicateChildOutcome ==
                     PolylineStageTestReader::Outcome::Rejected &&
                 secondDuplicateRemains,
             "duplicate-child negative reaches the duplicate rejection branch and restores the duplicate source frame");
    t.expect(duplicateOwnerSetup, "duplicate-child parent stages");
    t.expect(firstDuplicateStaged, "first duplicate-child frame stages");
    t.expect(duplicateChildOutcome ==
                 PolylineStageTestReader::Outcome::Rejected,
             "second duplicate-child frame is rejected");
    t.expect(secondDuplicateRemains,
             "rejected duplicate-child source frame is restored");
    t.expect(!malformedReader.hasPendingInsert(duplicateOwner) &&
                 malformedReader.stagedAttributeIndexSize() == 0u &&
                 malformedReader.validateState(),
             "duplicate-child owner/index state terminalizes cleanly");

    PolylineStageTestReader orphanDuplicateReader;
    PolylinePublicationSink orphanDuplicateSink;
    constexpr std::uint32_t orphanDuplicateOwner = 0x2B000u;
    constexpr std::uint32_t orphanDuplicateHandle = 0x2B001u;
    constexpr std::uint32_t orphanDuplicateFrame = 0x2B002u;
    bool orphanDuplicateFirstRemains = true;
    bool orphanDuplicateSecondRemains = false;
    const bool orphanDuplicateFirstStaged =
        orphanDuplicateReader.addFrame(orphanDuplicateHandle) &&
        orphanDuplicateReader.stageAttribute(
            orphanDuplicateHandle, orphanDuplicateOwner,
            orphanDuplicateHandle, orphanDuplicateSink,
            orphanDuplicateFirstRemains) ==
            PolylineStageTestReader::Outcome::StagedCompound &&
        !orphanDuplicateFirstRemains;
    const bool orphanDuplicateRejected = orphanDuplicateFirstStaged &&
        orphanDuplicateReader.addFrame(orphanDuplicateFrame) &&
        orphanDuplicateReader.stageAttribute(
            orphanDuplicateFrame, orphanDuplicateOwner,
            orphanDuplicateHandle, orphanDuplicateSink,
            orphanDuplicateSecondRemains) ==
            PolylineStageTestReader::Outcome::Rejected &&
        orphanDuplicateSecondRemains &&
        !orphanDuplicateReader.hasOrphanAttributeOwner(
            orphanDuplicateOwner) &&
        orphanDuplicateReader.stagedAttributeIndexSize() == 0u &&
        orphanDuplicateReader.validateState();
    t.expect(orphanDuplicateRejected,
             "duplicate child-first ATTRIB identity terminalizes and clears its orphan index");

    PolylineStageTestReader budgetReader;
    PolylinePublicationSink budgetSink;
    constexpr std::uint32_t budgetOwner = 0x2C000u;
    constexpr std::uint32_t budgetChild = 0x2C001u;
    DRW_Insert budgetInsert;
    budgetInsert.handle = budgetOwner;
    addDeclaredHandles(budgetInsert, {budgetChild});
    budgetReader.setExpectedAttributeBudgetForTest(
        dwgSafety::MaxDwgReadObjectCount);
    bool budgetParentRemains = false;
    const bool expectedIndexBudgetRefused =
        budgetReader.addFrame(budgetOwner) &&
        budgetReader.stageInsert(budgetInsert, budgetSink,
                                 budgetParentRemains) ==
            PolylineStageTestReader::Outcome::Rejected &&
        budgetParentRemains && !budgetReader.hasPendingInsert(budgetOwner) &&
        budgetReader.stagedAttributeIndexSize() == 0u;
    budgetReader.setExpectedAttributeBudgetForTest(0u);
    t.expect(expectedIndexBudgetRefused && budgetReader.validateState(),
             "aggregate expected-handle index budget refuses an over-limit owner before mutation and permits subsequent validation");
    t.expect(wrongOwnerCleaned,
             "wrong-owner ATTRIB is not adopted by the declared INSERT and cleanup releases indexes");
    t.expect(wrongAttributeStaged && expectedOwnerPending,
             "wrong-owner negative stages the independent orphan and leaves the declared parent pending");
    t.expect(wrongAttributeStaged, "wrong-owner ATTRIB stages as an orphan");
    t.expect(expectedOwnerPending,
             "declared owner remains pending without adopting wrong-owner ATTRIB");
    t.expect(wrongOwnerDoesNotAdopt,
             "wrong-owner state validates before explicit cleanup");
    t.expect(wrongOwnerCleaned,
             "wrong-owner and parent cleanup leave no staged index entries");

    constexpr std::uint32_t legacyOwner = 0x2A000u;
    constexpr std::uint32_t legacyMiddle = 0x2A001u;
    constexpr std::uint32_t legacyLast = 0x2A002u;
    constexpr std::uint32_t legacyFirst = 0x2A003u;
    PolylineStageTestReader legacyReader;
    legacyReader.setReaderVersionForTest(DRW::AC1015);
    PolylinePublicationSink legacySink;
    DRW_Insert legacyInsert;
    legacyInsert.handle = legacyOwner;
    addDeclaredHandles(legacyInsert, {legacyFirst, legacyLast});
    bool legacyParentRemains = true;
    bool legacyMiddleRemains = true;
    bool legacyLastRemains = true;
    bool legacyFirstRemains = true;
    const bool legacyParentStaged = legacyReader.addFrame(legacyOwner) &&
        legacyReader.stageInsert(legacyInsert, legacySink,
                                 legacyParentRemains) ==
            PolylineStageTestReader::Outcome::StagedCompound &&
        !legacyParentRemains;
    const bool legacyMiddleStaged = legacyParentStaged &&
        legacyReader.addFrame(legacyMiddle) &&
        legacyReader.stageAttribute(legacyMiddle, legacyOwner, legacyMiddle,
                                    legacySink, legacyMiddleRemains) ==
            PolylineStageTestReader::Outcome::StagedCompound &&
        !legacyMiddleRemains;
    const bool legacyLastStaged = legacyMiddleStaged &&
        legacyReader.addFrame(legacyLast) &&
        legacyReader.stageAttribute(legacyLast, legacyOwner, legacyLast,
                                    legacySink, legacyLastRemains) ==
            PolylineStageTestReader::Outcome::StagedCompound &&
        !legacyLastRemains && legacySink.inserts.empty();
    const bool legacyEndpointsCommit = legacyLastStaged &&
        legacyReader.addFrame(legacyFirst) &&
        legacyReader.stageAttribute(legacyFirst, legacyOwner, legacyFirst,
                                    legacySink, legacyFirstRemains) ==
            PolylineStageTestReader::Outcome::CommittedCompound &&
        !legacyFirstRemains && legacySink.inserts.size() == 1u &&
        legacySink.inserts.front().attlist.size() == 3u &&
        legacySink.inserts.front().attlist[0]->handle == legacyMiddle &&
        legacySink.inserts.front().attlist[1]->handle == legacyLast &&
        legacySink.inserts.front().attlist[2]->handle == legacyFirst &&
        legacyReader.stagedAttributeIndexSize() == 0u &&
        legacyReader.validateState();
    t.expect(legacyEndpointsCommit,
             "R13-R2000 INSERT first/last endpoint compatibility retains encounter order for intervening ATTRIB records");
}

void testExactUnknownSectionNames(TestContext& t) {
    const auto makeName = [](std::size_t index) {
        return std::string("Vendor.CommonPrefix.UnknownSection.")
            + std::to_string(index);
    };
    const auto compareWork = [&makeName](std::size_t count, int order) {
        std::size_t comparisons = 0;
        dwgSafety::ExactNameIndex<NameComparisonCounter> index(
            NameComparisonCounter{&comparisons});
        bool admitted = true;
        for (std::size_t i = 0; i < count; ++i) {
            std::size_t slot = i;
            if (order == 1)
                slot = count - i - 1;
            else if (order == 2)
                slot = (i * 197u) % count;
            admitted = admitted
                && dwgSafety::tryInsertExactName(index, makeName(slot))
                    == dwgSafety::ExactNameInsertResult::Inserted;
        }
        return std::make_pair(comparisons, admitted && index.size() == count);
    };
    for (int order = 0; order < 3; ++order) {
        const auto small = compareWork(128, order);
        const auto large = compareWork(512, order);
        t.expect(small.second && large.second
                     && small.first < 128u * 48u
                     && large.first < 512u * 48u
                     && large.first < small.first * 8u,
                 "128/512 exact unknown-name admission stays ordered-logarithmic with common prefixes");
    }

    dwgSafety::ExactNameIndex<> names;
    const std::string early = makeName(0);
    const std::string late = makeName(511);
    bool uniqueRows = true;
    for (std::size_t i = 0; i < 512; ++i) {
        uniqueRows = uniqueRows
            && dwgSafety::tryInsertExactName(names, makeName(i))
                == dwgSafety::ExactNameInsertResult::Inserted;
    }
    const auto duplicateEarly = dwgSafety::tryInsertExactName(names, early);
    const auto duplicateLate = dwgSafety::tryInsertExactName(names, late);
    dwgSafety::ExactNameIndex<> caseDistinct;
    const bool casesRemainDistinct = dwgSafety::tryInsertExactName(
            caseDistinct, "Vendor:Context")
            == dwgSafety::ExactNameInsertResult::Inserted
        && dwgSafety::tryInsertExactName(caseDistinct, "vendor:Context")
            == dwgSafety::ExactNameInsertResult::Inserted;
    t.expect(uniqueRows && duplicateEarly
                     == dwgSafety::ExactNameInsertResult::Duplicate
                 && duplicateLate == dwgSafety::ExactNameInsertResult::Duplicate
                 && casesRemainDistinct,
             "unknown-name admission rejects exact early/late duplicates but preserves case-distinct names");

    using FailingNameIndex = dwgSafety::ExactNameIndex<
        std::less<std::string>, ThrowOnceAllocator<std::string>>;
    FailingNameIndex recoverable;
    const bool firstInserted = dwgSafety::tryInsertExactName(
        recoverable, "Vendor:Stable")
        == dwgSafety::ExactNameInsertResult::Inserted;
    RangeAllocationFailure::failNext = true;
    const bool resourceReported = dwgSafety::tryInsertExactName(
        recoverable, "Vendor:Retry")
        == dwgSafety::ExactNameInsertResult::ResourceFailure;
    const bool retainedAfterFailure = recoverable.size() == 1;
    const bool retried = dwgSafety::tryInsertExactName(
        recoverable, "Vendor:Retry")
        == dwgSafety::ExactNameInsertResult::Inserted;
    t.expect(firstInserted && resourceReported && retainedAfterFailure
                 && retried && recoverable.size() == 2,
             "unknown-name index handles node-allocation failure without losing prior staged names");
}

bool readLe32(const std::vector<std::uint8_t>& bytes, std::size_t offset,
              std::uint32_t& value) {
    if (offset > bytes.size() || bytes.size() - offset < 4u)
        return false;
    value = static_cast<std::uint32_t>(bytes[offset])
        | (static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u)
        | (static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u)
        | (static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u);
    return true;
}

bool writeLe32(std::vector<std::uint8_t>& bytes, std::size_t offset,
               std::uint32_t value) {
    if (offset > bytes.size() || bytes.size() - offset < 4u)
        return false;
    for (unsigned int i = 0; i < 4u; ++i)
        bytes[offset + i] = static_cast<std::uint8_t>(value >> (i * 8u));
    return true;
}

std::uint32_t r2004PageChecksumReference(std::uint32_t seed,
                                        const std::uint8_t* data,
                                        std::uint64_t size) {
    std::uint32_t sum1 = seed & 0xffffu;
    std::uint32_t sum2 = seed >> 16u;
    while (size != 0u) {
        const std::uint64_t chunk = std::min<std::uint64_t>(size, 0x15b0u);
        size -= chunk;
        for (std::uint64_t index = 0; index < chunk; ++index) {
            sum1 += data[index];
            sum2 += sum1;
        }
        data += chunk;
        sum1 %= 0xfff1u;
        sum2 %= 0xfff1u;
    }
    return (sum2 << 16u) | (sum1 & 0xffffu);
}

std::vector<std::uint8_t> makeR2004DataPageVector(
        std::uint64_t address, std::uint32_t sectionId,
        std::uint32_t startOffset,
        const std::array<std::uint8_t, 7>& literals) {
    constexpr std::uint32_t physicalPageSize = 64u;
    // Four initial literal bytes, a 57-byte distance-two back-reference,
    // and three literal bytes produce exactly one 64-byte output page.
    const std::array<std::uint8_t, 12> payload{{
        0x01u, literals[0], literals[1], literals[2], literals[3],
        0x20u, 0x18u, 0x07u, 0x00u,
        literals[4], literals[5], literals[6]}};
    std::array<std::uint8_t, 32> header{};
    const auto putHeaderLong = [&header](std::size_t offset,
                                         std::uint32_t value) {
        for (unsigned int index = 0; index < 4u; ++index)
            header[offset + index] = static_cast<std::uint8_t>(
                value >> (index * 8u));
    };
    putHeaderLong(0u, 0x4163043bu);
    putHeaderLong(4u, sectionId);
    putHeaderLong(8u, static_cast<std::uint32_t>(payload.size()));
    // Match the captured AC1024 profile: this word records physical extent.
    putHeaderLong(12u, physicalPageSize);
    putHeaderLong(16u, startOffset);
    const std::uint32_t dataChecksum = r2004PageChecksumReference(
        0u, payload.data(), payload.size());
    putHeaderLong(28u, dataChecksum);
    putHeaderLong(24u, r2004PageChecksumReference(
        dataChecksum, header.data(), header.size()));

    dwgCompressor::decrypt18Hdr(header.data(), header.size(), address);
    std::vector<std::uint8_t> page(physicalPageSize, 0xa5u);
    std::copy(header.begin(), header.end(), page.begin());
    std::copy(payload.begin(), payload.end(), page.begin() + 32u);
    return page;
}

std::array<std::uint8_t, 64> expectedR2004PageOutput(
        const std::array<std::uint8_t, 7>& literals) {
    std::array<std::uint8_t, 64> expected{};
    std::copy_n(literals.begin(), 4u, expected.begin());
    for (std::size_t index = 4u; index < 61u; ++index)
        expected[index] = expected[index - 2u];
    std::copy_n(literals.begin() + 4, 3u, expected.begin() + 61u);
    return expected;
}

std::vector<std::uint8_t> encodeR2004LiteralRun(
        const std::vector<std::uint8_t>& source) {
    if (source.empty() || source.size() > std::numeric_limits<std::uint32_t>::max())
        return {};
    const auto length = static_cast<std::uint32_t>(source.size());
    std::vector<std::uint8_t> compressed;
    if (length <= 18u) {
        compressed.push_back(static_cast<std::uint8_t>(length - 3u));
    } else {
        compressed.push_back(0);
        std::uint32_t remaining = length - 18u;
        while (remaining > 0xFFu) {
            compressed.push_back(0);
            remaining -= 0xFFu;
        }
        compressed.push_back(static_cast<std::uint8_t>(remaining));
    }
    compressed.insert(compressed.end(), source.begin(), source.end());
    return compressed;
}

std::vector<std::uint8_t> encodeR2004SystemPage(
        std::uint32_t pageType, const std::vector<std::uint8_t>& content) {
    const auto compressed = encodeR2004LiteralRun(content);
    if (compressed.empty()
        || content.size() > std::numeric_limits<std::uint32_t>::max()
        || compressed.size() > std::numeric_limits<std::uint32_t>::max())
        return {};
    std::vector<std::uint8_t> page(20u, 0);
    (void)writeLe32(page, 0, pageType);
    (void)writeLe32(page, 4, static_cast<std::uint32_t>(content.size()));
    (void)writeLe32(page, 8, static_cast<std::uint32_t>(compressed.size()));
    (void)writeLe32(page, 12, 2u);
    std::uint32_t checksum = dwgUtil::checksum18(0, page.data(), page.size());
    checksum = dwgUtil::checksum18(checksum, compressed.data(), compressed.size());
    (void)writeLe32(page, 16, checksum);
    page.insert(page.end(), compressed.begin(), compressed.end());
    return page;
}

bool decodeR2004SystemPage(const std::vector<std::uint8_t>& bytes,
                           std::uint64_t address, std::uint32_t expectedType,
                           std::vector<std::uint8_t>& content) {
    if (address > bytes.size() || bytes.size() - static_cast<std::size_t>(address) < 20u)
        return false;
    const auto offset = static_cast<std::size_t>(address);
    std::uint32_t pageType = 0, decompressedSize = 0, compressedSize = 0;
    if (!readLe32(bytes, offset, pageType)
        || !readLe32(bytes, offset + 4u, decompressedSize)
        || !readLe32(bytes, offset + 8u, compressedSize)
        || pageType != expectedType || decompressedSize == 0
        || decompressedSize > dwgSafety::MaxBufferSize
        || compressedSize == 0
        || compressedSize > bytes.size() - offset - 20u)
        return false;

    content.resize(decompressedSize);
    dwgCompressor decompressor;
    return decompressor.decompress18(bytes.data() + offset + 20u,
                                     content.data(), compressedSize,
                                     decompressedSize)
        && decompressor.decompressedBytes() == decompressedSize;
}

bool makeOverflowingR2004SectionPageMapOffset(
        std::vector<std::uint8_t>& bytes) {
    if (bytes.size() < 0x100u)
        return false;

    std::array<std::uint8_t, 0x6c> variableHeader{};
    for (std::size_t index = 0; index < variableHeader.size(); ++index) {
        variableHeader[index] = static_cast<std::uint8_t>(
            bytes[0x80u + index]
            ^ static_cast<std::uint8_t>(DRW_magicNum18[index]));
    }
    const auto putLong = [&variableHeader](std::size_t offset,
                                           std::uint32_t value) {
        for (unsigned int byte = 0; byte < 4u; ++byte) {
            variableHeader[offset + byte] = static_cast<std::uint8_t>(
                value >> (byte * 8u));
        }
    };
    constexpr std::uint64_t overflowingOffset =
        std::numeric_limits<std::uint64_t>::max() - 0xFFu;
    for (unsigned int byte = 0; byte < 8u; ++byte) {
        variableHeader[0x54u + byte] = static_cast<std::uint8_t>(
            overflowingOffset >> (byte * 8u));
    }

    // The file-header CRC slot is zeroed while computing the checksum.
    std::fill(variableHeader.begin() + 0x68u, variableHeader.end(), 0u);
    const std::uint32_t headerCrc = dwgUtil::crc32(
        0u, variableHeader.data(), variableHeader.size());
    putLong(0x68u, headerCrc);
    for (std::size_t index = 0; index < variableHeader.size(); ++index) {
        bytes[0x80u + index] = static_cast<std::uint8_t>(
            variableHeader[index]
            ^ static_cast<std::uint8_t>(DRW_magicNum18[index]));
    }
    return true;
}

bool makeDuplicateUnknownR2004SectionMap(std::vector<std::uint8_t>& bytes) {
    if (bytes.size() < 0x100u)
        return false;

    std::array<std::uint8_t, 0x6c> variableHeader{};
    for (std::size_t i = 0; i < variableHeader.size(); ++i)
        variableHeader[i] = bytes[0x80u + i]
            ^ static_cast<std::uint8_t>(DRW_magicNum18[i]);
    const auto headerValue32 = [&variableHeader](std::size_t offset) {
        return static_cast<std::uint32_t>(variableHeader[offset])
            | (static_cast<std::uint32_t>(variableHeader[offset + 1u]) << 8u)
            | (static_cast<std::uint32_t>(variableHeader[offset + 2u]) << 16u)
            | (static_cast<std::uint32_t>(variableHeader[offset + 3u]) << 24u);
    };
    const std::uint32_t sectionPageMapId = headerValue32(0x50u);
    const std::uint32_t dataSectionMapId = headerValue32(0x5cu);
    const std::uint64_t sectionPageMapOffset = headerValue32(0x54u)
        | (static_cast<std::uint64_t>(headerValue32(0x58u)) << 32u);
    if (sectionPageMapOffset > std::numeric_limits<std::uint64_t>::max() - 0x100u)
        return false;
    const std::uint64_t sectionPageMapAddress = sectionPageMapOffset + 0x100u;

    std::vector<std::uint8_t> sectionPageMapContent;
    if (!decodeR2004SystemPage(bytes, sectionPageMapAddress, 0x41630e3bu,
                               sectionPageMapContent)
        || sectionPageMapContent.size() % 8u != 0u)
        return false;

    std::uint64_t physicalAddress = 0x100u;
    std::uint64_t dataSectionMapAddress = 0;
    std::uint32_t dataSectionMapPageSize = 0;
    bool foundSectionPageMap = false;
    bool foundDataSectionMap = false;
    for (std::size_t offset = 0; offset < sectionPageMapContent.size(); offset += 8u) {
        std::uint32_t pageId = 0, pageSize = 0;
        if (!readLe32(sectionPageMapContent, offset, pageId)
            || !readLe32(sectionPageMapContent, offset + 4u, pageSize)
            || pageSize == 0
            || physicalAddress > std::numeric_limits<std::uint64_t>::max() - pageSize)
            return false;
        if (pageId == sectionPageMapId) {
            if (physicalAddress != sectionPageMapAddress)
                return false;
            foundSectionPageMap = true;
        }
        if (pageId == dataSectionMapId) {
            dataSectionMapAddress = physicalAddress;
            dataSectionMapPageSize = pageSize;
            foundDataSectionMap = true;
        }
        physicalAddress += pageSize;
    }
    if (!foundSectionPageMap || !foundDataSectionMap
        || dataSectionMapAddress > bytes.size()
        || dataSectionMapPageSize > bytes.size() - dataSectionMapAddress)
        return false;

    std::vector<std::uint8_t> dataSectionMapContent;
    if (!decodeR2004SystemPage(bytes, dataSectionMapAddress, 0x4163003bu,
                               dataSectionMapContent)
        || dataSectionMapContent.size() < 20u)
        return false;
    std::uint32_t descriptionCount = 0;
    if (!readLe32(dataSectionMapContent, 0, descriptionCount)
        || descriptionCount < 2u)
        return false;

    std::vector<std::size_t> nameOffsets;
    std::size_t offset = 20u;
    for (std::uint32_t i = 0; i < descriptionCount; ++i) {
        std::uint32_t pageCount = 0;
        if (offset > dataSectionMapContent.size()
            || dataSectionMapContent.size() - offset < 96u
            || !readLe32(dataSectionMapContent, offset + 8u, pageCount))
            return false;
        nameOffsets.push_back(offset + 32u);
        const std::uint64_t next = static_cast<std::uint64_t>(offset) + 96u
            + static_cast<std::uint64_t>(pageCount) * 16u;
        if (next > dataSectionMapContent.size())
            return false;
        offset = static_cast<std::size_t>(next);
    }
    if (nameOffsets.size() < 2u)
        return false;

    constexpr char duplicateName[] = "AcDb:CodexDuplicate";
    for (std::size_t i = 0; i < 2u; ++i) {
        const auto nameOffset = nameOffsets[i];
        std::fill(dataSectionMapContent.begin() + static_cast<std::ptrdiff_t>(nameOffset),
                  dataSectionMapContent.begin() + static_cast<std::ptrdiff_t>(nameOffset + 64u),
                  std::uint8_t{0});
        std::copy(duplicateName, duplicateName + sizeof(duplicateName) - 1u,
                  dataSectionMapContent.begin() + static_cast<std::ptrdiff_t>(nameOffset));
    }

    auto replacement = encodeR2004SystemPage(0x4163003bu, dataSectionMapContent);
    if (replacement.empty() || replacement.size() > dataSectionMapPageSize)
        return false;
    replacement.resize(dataSectionMapPageSize, 0);
    std::copy(replacement.begin(), replacement.end(),
              bytes.begin() + static_cast<std::ptrdiff_t>(dataSectionMapAddress));
    return true;
}

class SectionMapPublicationProbe final : public dwgReader18 {
public:
    SectionMapPublicationProbe(std::vector<std::uint8_t>& bytes, dwgRW* owner)
        : dwgReader18(std::make_unique<dwgBuffer>(bytes.data(), bytes.size()), owner) {
        version = DRW::AC1018;
        decoder.setVersion(version, false);
    }
    bool parse() { return readFileHeader(); }
    std::size_t knownSections() const { return sections.size(); }
    std::size_t unknownSections() const { return m_unknownSections.size(); }
    const std::vector<DwgIntegrityDiagnostic>& diagnostics() const {
        return m_integrityDiagnostics;
    }
};

template<class Reader>
class AuxHeaderSectionProbe final : public Reader {
public:
    AuxHeaderSectionProbe(std::vector<std::uint8_t>& bytes,
                          DRW::Version version)
        : Reader(std::make_unique<dwgBuffer>(bytes.data(), bytes.size()),
                 nullptr) {
        this->version = version;
        this->decoder.setVersion(version, false);
    }

    bool decode(std::vector<std::uint8_t>& decoded) {
        if (!this->readFileHeader())
            return false;
        const auto section = this->sections.find(secEnum::AUXHEADER);
        if (section == this->sections.end())
            return false;
        std::unique_ptr<std::uint8_t[]> data;
        std::uint64_t size = 0;
        if (!this->parseDataPage(section->second, data, size)
            || data == nullptr
            || size > std::numeric_limits<std::size_t>::max())
            return false;
        decoded.assign(data.get(), data.get() + static_cast<std::size_t>(size));
        return true;
    }
};

class DataPageDecodeProbe final : public dwgReader18 {
public:
    explicit DataPageDecodeProbe(std::vector<std::uint8_t>& bytes)
        : dwgReader18(std::make_unique<dwgBuffer>(bytes.data(), bytes.size()),
                      nullptr) {
        version = DRW::AC1024;
        decoder.setVersion(version, false);
    }
    bool decode(const dwgSectionInfo& section,
                std::unique_ptr<std::uint8_t[]>& decoded,
                std::uint64_t& logicalSize) {
        return parseDataPage(section, decoded, logicalSize);
    }
    const std::vector<DwgIntegrityDiagnostic>& diagnostics() const {
        return m_integrityDiagnostics;
    }
};

class R21DataPageDecodeProbe final : public dwgReader21 {
public:
    explicit R21DataPageDecodeProbe(std::vector<std::uint8_t>& bytes)
        : dwgReader21(std::make_unique<dwgBuffer>(bytes.data(), bytes.size()),
                      nullptr) {
        version = DRW::AC1021;
        decoder.setVersion(version, false);
    }
    bool readSections() { return readFileHeader(); }
    std::size_t knownSections() const { return sections.size(); }
    std::size_t unknownSections() const { return m_unknownSections.size(); }
    const dwgSectionInfo* findSection(const std::string& name) const {
        for (const auto& entry : sections) {
            if (entry.second.name == name)
                return &entry.second;
        }
        return nullptr;
    }
    bool decode(const dwgSectionInfo& section, std::uint8_t* decoded) {
        return parseDataPage(section, decoded);
    }
    bool decodePageMapSystemPage(
        std::uint64_t compressedSize, std::uint64_t uncompressedSize,
        std::uint64_t correctionFactor, std::uint64_t offset,
        std::uint64_t crcSeed, std::uint64_t compressedCrc,
        std::uint64_t uncompressedCrc, std::vector<std::uint8_t>& decoded,
        DwgIntegrityPhase phase = DwgIntegrityPhase::PageMap) {
        if (uncompressedSize == 0u
            || uncompressedSize > std::numeric_limits<std::size_t>::max())
            return false;
        decoded.assign(static_cast<std::size_t>(uncompressedSize), 0u);
        return parseSysPage(compressedSize, uncompressedSize,
                            correctionFactor, offset, crcSeed, compressedCrc,
                            uncompressedCrc, decoded.data(),
                            phase);
    }
    bool parsePageMapEntries(
        std::uint8_t* data, std::uint64_t size,
        std::uint64_t expectedRecordCount, std::uint64_t maxPageId,
        std::uint64_t fileSize,
        std::unordered_map<std::uint64_t, dwgPageInfo>& pages) {
        return parseSectionPageMap(data, size, expectedRecordCount, maxPageId,
                                   fileSize, pages);
    }
    const std::vector<DwgIntegrityDiagnostic>& diagnostics() const {
        return m_integrityDiagnostics;
    }
};

class SectionMapSink final : public dx_iface {
public:
    SectionMapSink() {
        cData = &data;
        currentBlock = data.mBlock;
    }
    void addHeader(const DRW_Header*) override { ++headers; }
    void addLine(const DRW_Line& value) override {
        ++lines;
        end = value.secPoint;
    }
    dx_data data;
    std::size_t headers{0};
    std::size_t lines{0};
    DRW_Coord end;
};

class ModelerDispatchSource final : public DRW_ModelerGeometry {
public:
    explicit ModelerDispatchSource(DRW::ETYPE type) : DRW_ModelerGeometry(type) {}

    void setObjectType(std::uint16_t value) { oType = value; }
    bool encodeCommon(DRW::Version version, dwgBufferW& body,
                      dwgBufferW& strings) {
        return encodeDwgCommon(version, &body, &strings);
    }
    bool encodeHandles(DRW::Version version, dwgBufferW& body,
                       dwgBufferW& handles) {
        return encodeDwgEntHandle(version, &body, &handles);
    }
};

class InsertDispatchSource final : public DRW_Insert {
public:
    bool encode(DRW::Version version, dwgBufferW& body,
                dwgBufferW& strings, dwgBufferW& handles) {
        return encodeDwg(version, &body, 0u, &strings, &handles);
    }
};

class ModelerDispatchFrameWriter final : public dwgWriter27 {
public:
    ModelerDispatchFrameWriter(std::ofstream* stream, DRW_Header* header)
        : dwgWriter27(stream, header) {}

    bool encodeEmptySolid(std::uint32_t handle, std::uint32_t ownerHandle) {
        ModelerDispatchSource source(DRW::E3DSOLID);
        source.setObjectType(38u); // ODA v5.4.1 §20.4.41: 3DSOLID
        source.handle = handle;
        source.parentHandle = ownerHandle;

        dwgBufferW& body = beginObject(handle);
        if (!source.encodeCommon(m_version, body, m_objectStrings))
            return false;
        body.putBit(1u); // empty modeler data: no ACIS body follows
        body.putBit(0u); // modeler-data unknown bit
        if (!body.isGood()
            || !source.encodeHandles(m_version, body, m_objectHandles))
            return false;

        finishObject();
        return !objectWriteFailed() && m_buf.isGood() && !m_buf.data().empty();
    }

    bool encodeBlockDelimiter(std::uint32_t handle,
                              std::uint32_t ownerHandle,
                              const std::string& name, bool isEnd) {
        DRW_Block source;
        source.handle = handle;
        source.parentHandle = ownerHandle;
        source.name = name;
        source.setIsEnd(isEnd);

        dwgBufferW& body = beginObject(handle);
        if (!DrwEntityEncodeTestAccess::encodeBlock(
                source, m_version, &body, &m_objectStrings, &m_objectHandles))
            return false;
        finishObject();
        return !objectWriteFailed() && m_buf.isGood() && !m_buf.data().empty();
    }

    bool encodeLineWithEed(std::uint32_t handle, std::uint32_t ownerHandle,
                           bool malformedBinaryItem) {
        DRW_Line source;
        source.handle = handle;
        source.parentHandle = ownerHandle;
        source.basePoint = DRW_Coord{1.0, 2.0, 0.0};
        source.secPoint = DRW_Coord{3.0, 4.0, 0.0};
        source.extData.push_back(
            std::make_shared<DRW_Variant>(1001, std::string("EED_APP_A")));
        source.extData.push_back(
            std::make_shared<DRW_Variant>(1000, std::string("EED_TEXT")));
        source.extData.push_back(
            std::make_shared<DRW_Variant>(1070, 0x1234));
        source.extData.push_back(
            std::make_shared<DRW_Variant>(1001, std::string("EED_APP_B")));
        source.extData.push_back(std::make_shared<DRW_Variant>(
            1004, std::vector<std::uint8_t>{0xAAu, 0xBBu}));

        dwgBufferW& body = beginObject(handle);
        if (!DrwEntityEncodeTestAccess::encodeLine(
                source, m_version, &body, &m_objectStrings, &m_objectHandles))
            return false;
        if (malformedBinaryItem
            && !DrwEntityEncodeTestAccess::corruptSecondEedBinaryLength(
                body, 4u))
            return false;
        finishObject();
        return !objectWriteFailed() && m_buf.isGood() && !m_buf.data().empty();
    }

    bool encodeLine(std::uint32_t handle, std::uint32_t ownerHandle) {
        DRW_Line source;
        source.handle = handle;
        source.parentHandle = ownerHandle;
        source.basePoint = DRW_Coord{71.0, 83.0, 0.0};
        source.secPoint = DRW_Coord{97.0, 101.0, 0.0};

        dwgBufferW& body = beginObject(handle);
        m_objectStrings.reset();
        m_objectHandles.reset();
        if (!DrwEntityEncodeTestAccess::encodeLine(
                source, m_version, &body, &m_objectStrings, &m_objectHandles))
            return false;
        finishObject();
        return !objectWriteFailed() && m_buf.isGood() && !m_buf.data().empty();
    }

    const std::vector<std::pair<std::uint32_t, std::uint32_t>>&
    objectOffsets() const {
        return m_objectMap;
    }

    bool encodeAcDsModeler(DRW::ETYPE entityType, std::uint16_t objectType,
                           std::uint32_t handle,
                           std::uint32_t ownerHandle,
                           std::vector<std::uint32_t> reactors = {}) {
        ModelerDispatchSource source(entityType);
        source.setObjectType(objectType);
        source.handle = handle;
        source.parentHandle = ownerHandle;
        source.reactorHandles = std::move(reactors);
        source.setHasDataStorageBinaryData(true);

        m_modelerObjectOffsets[handle] =
            static_cast<std::uint32_t>(m_buf.size());
        dwgBufferW& body = beginObject(handle);
        if (!source.encodeCommon(m_version, body, m_objectStrings))
            return false;
        body.putBit(0u); // nonempty modeler data; SAB is in AcDs
        body.putBit(0u); // modeler-data unknown bit
        body.putBitShort(168u); // opaque AC1027-local field, not SAB version
        body.putBit(0u); // empty R2007+ string stream
        body.alignToByte();
        if (!body.isGood()
            || !source.encodeHandles(m_version, body, m_objectHandles))
            return false;

        finishObject();
        return !objectWriteFailed() && m_buf.isGood() && !m_buf.data().empty();
    }

    bool encodeAcDsSolid(std::uint32_t handle, std::uint32_t ownerHandle,
                         std::vector<std::uint32_t> reactors = {}) {
        return encodeAcDsModeler(DRW::E3DSOLID, 38u, handle, ownerHandle,
                                 std::move(reactors));
    }

    bool encodeAcDsRegion(std::uint32_t handle, std::uint32_t ownerHandle) {
        return encodeAcDsModeler(DRW::REGION, 37u, handle, ownerHandle);
    }

    bool encodeInsert(std::uint32_t handle, std::uint32_t ownerHandle,
                      std::uint32_t targetBlockHandle) {
        InsertDispatchSource source;
        source.handle = handle;
        source.parentHandle = ownerHandle;
        source.blockRecH.ref = targetBlockHandle;

        dwgBufferW& body = beginObject(handle);
        if (!source.encode(m_version, body, m_objectStrings, m_objectHandles))
            return false;
        finishObject();
        return !objectWriteFailed() && m_buf.isGood() && !m_buf.data().empty();
    }

    std::uint32_t modelerObjectOffset(std::uint32_t handle) const {
        const auto found = m_modelerObjectOffsets.find(handle);
        return found == m_modelerObjectOffsets.end() ? 0u : found->second;
    }

private:
    std::map<std::uint32_t, std::uint32_t> m_modelerObjectOffsets;
};

class LegacyLayoutFrameWriter final : public dwgWriter15 {
public:
    using dwgWriter15::beginObject;
    using dwgWriter15::finishObject;
    using dwgWriter15::m_buf;
    using dwgWriter15::m_objectHandles;
    using dwgWriter15::m_objectMap;
    using dwgWriter15::m_objectStrings;
    using dwgWriter15::m_version;
    using dwgWriter15::objectWriteFailed;

    LegacyLayoutFrameWriter(std::ofstream* stream, DRW_Header* header)
        : dwgWriter15(stream, header) {}

    bool encodeOwnerlessLine(std::uint32_t handle, DRW::Space space) {
        return encodeOwnerlessLine(handle, space, 0u, 0u, false);
    }

    bool encodeOwnerlessLineWithExplicitLinks(std::uint32_t handle,
                                               DRW::Space space,
                                               std::uint32_t previous,
                                               std::uint32_t next) {
        return encodeOwnerlessLine(handle, space, previous, next, true);
    }

    bool encodeBlockDelimiter(std::uint32_t handle, const std::string& name,
                              bool isEnd) {
        DRW_Block block;
        block.handle = handle;
        block.parentHandle = DRW::NoHandle;
        block.name = name;
        block.setIsEnd(isEnd);
        dwgBufferW& body = beginObject(handle);
        m_objectStrings.reset();
        m_objectHandles.reset();
        if (!DrwEntityEncodeTestAccess::encodeBlock(
                block, m_version, &body, nullptr, &m_objectHandles))
            return false;
        finishObject();
        return !objectWriteFailed() && m_buf.isGood();
    }

    bool encodeOwnerlessLine(std::uint32_t handle, DRW::Space space,
                             std::uint32_t previous, std::uint32_t next,
                             bool explicitLinks) {
        DRW_Line line;
        line.handle = handle;
        line.parentHandle = DRW::NoHandle;
        line.space = space;
        line.basePoint = DRW_Coord{static_cast<double>(handle), 23.0, 0.0};
        line.secPoint = DRW_Coord{static_cast<double>(handle) + 1.0,
                                  59.0, 0.0};

        dwgBufferW& body = beginObject(handle);
        m_objectStrings.reset();
        m_objectHandles.reset();
        if (!DrwEntityEncodeTestAccess::encodeLine(
                line, m_version, &body, &m_objectStrings, &m_objectHandles))
            return false;
        if (explicitLinks && !writeExplicitLinks(body, handle, previous, next))
            return false;
        finishObject();
        return !objectWriteFailed() && m_buf.isGood();
    }

    const std::vector<std::pair<std::uint32_t, std::uint32_t>>&
    objectOffsets() const {
        return m_objectMap;
    }

private:
    bool writeExplicitLinks(dwgBufferW& body, std::uint32_t handle,
                            std::uint32_t previous,
                            std::uint32_t next) {
        // Locate the R13-R2000 `nolinks` bit from the generated common prefix.
        // This vector is intentionally EED-free and has no proxy graphics, so
        // the bit immediately follows the common handle, EED size, graphics
        // flag, entmode, and reactor count.
        std::vector<std::uint8_t>& bodyBytes = body.data();
        if (bodyBytes.empty())
            return false;
        dwgBuffer prefix(bodyBytes.data(), bodyBytes.size());
        if (prefix.getObjType(DRW::AC1015) != 19u)
            return false;
        (void)prefix.getRawLong32(); // RL object-size placeholder, patched later
        if (prefix.getHandle().ref != handle
            || prefix.getBitShort() != 0u
            || prefix.getBit() != 0u) {
            return false;
        }
        (void)prefix.get2Bits();
        if (prefix.getBitLong() != 0 || !prefix.isGood())
            return false;
        const std::uint64_t noLinksBit = prefix.getPosition() * 8u
            + prefix.getBitPos();
        if (noLinksBit >= static_cast<std::uint64_t>(bodyBytes.size()) * 8u)
            return false;
        const std::size_t byteIndex = static_cast<std::size_t>(noLinksBit / 8u);
        const unsigned int bitIndex = static_cast<unsigned int>(noLinksBit & 7u);
        const std::uint8_t mask = static_cast<std::uint8_t>(1u << (7u - bitIndex));
        if ((bodyBytes[byteIndex] & mask) == 0u)
            return false; // The current encoder must have emitted `nolinks=1`.
        bodyBytes[byteIndex] = static_cast<std::uint8_t>(bodyBytes[byteIndex] & ~mask);

        m_objectHandles.reset();
        const auto putNullHandle = [this]() {
            dwgHandle nullHandle;
            nullHandle.code = 0u;
            m_objectHandles.putHandle(nullHandle);
        };
        const auto putRelativeHandle = [this, handle, &putNullHandle](
                std::uint32_t target) {
            if (target == 0u) {
                putNullHandle();
                return;
            }
            dwgHandle link;
            if (target > handle) {
                link.code = 0x0Au; // relative soft pointer forward
                link.ref = target - handle;
            } else if (target < handle) {
                link.code = 0x0Cu; // relative soft pointer backward
                link.ref = handle - target;
            } else {
                link.code = 0x0Au;
                link.ref = 0u;
            }
            m_objectHandles.putHandle(link);
        };

        // AC1015 handle-stream order: xdictionary, previous entity, next
        // entity, then the layer reference. Layer is null in this vector.
        putNullHandle();
        putRelativeHandle(previous);
        putRelativeHandle(next);
        putNullHandle();
        return m_objectHandles.isGood();
    }
};

class ProxyEntityDispatchSource final : public DRW_ProxyEntity {
public:
    void setObjectType(std::uint16_t value) { oType = value; }
    bool encodeCommon(DRW::Version version, dwgBufferW& body,
                      dwgBufferW& strings) {
        return encodeDwgCommon(version, &body, &strings);
    }
    bool encodeHandles(DRW::Version version, dwgBufferW& body,
                       dwgBufferW& handles) {
        return encodeDwgEntHandle(version, &body, &handles);
    }
};

template <typename WriterBase>
class ProxyEntityDispatchFrameWriter final : public WriterBase {
public:
    using WriterBase::appendR2007StringStream;
    using WriterBase::beginObject;
    using WriterBase::finishObject;
    using WriterBase::m_buf;
    using WriterBase::m_currentHandle;
    using WriterBase::m_objectHandles;
    using WriterBase::m_objectMap;
    using WriterBase::m_objectStrings;
    using WriterBase::m_version;
    using WriterBase::objectWriteFailed;

    ProxyEntityDispatchFrameWriter(std::ofstream* stream, DRW_Header* header)
        : WriterBase(stream, header) {}

    bool encodeProxyEntity(
        std::uint32_t handle, std::uint32_t ownerHandle,
        const std::vector<std::uint8_t>& payload,
        std::uint8_t payloadBitCount,
        const std::vector<std::uint8_t>& graphics,
        const std::vector<std::pair<std::uint8_t, std::uint32_t>>& objectIds,
        bool malformedObjectId = false) {
        if (payloadBitCount > 8u
            || payloadBitCount > payload.size() * 8u)
            return false;

        ProxyEntityDispatchSource source;
        source.setObjectType(DRW_ProxyEntity::kDwgType);
        source.handle = handle;
        source.parentHandle = ownerHandle;
        if (!graphics.empty()) {
            source.proxyGraphics.assign(
                reinterpret_cast<const char*>(graphics.data()),
                graphics.size());
        }

        dwgBufferW& body = beginObject(handle);
        m_objectStrings.reset();
        m_objectHandles.reset();
        if (!source.encodeCommon(m_version, body, m_objectStrings))
            return false;

        // ODA v5.4.1 §20.4.90: class ID, version-dependent drawing-format
        // fields, original-data format, then opaque bits ending at the
        // detached string stream.
        body.putBitLong(0x1234);
        m_objectStrings.putVariableText(m_version, "AcDbProxyEntity");
        if (m_version >= DRW::AC1032) {
            body.putBitLong(0x1122); // R2018+ object DWG version.
            body.putBitLong(0x3344); // R2018+ maintenance version.
        } else {
            body.putBitLong(0x00102401); // Pre-R2018 packed drawing format.
        }
        body.putBit(0u); // original data format: DWG
        for (std::uint8_t bit = 0; bit < payloadBitCount; ++bit) {
            const std::uint8_t value = static_cast<std::uint8_t>(
                (payload[bit / 8u] >> (7u - (bit % 8u))) & 1u);
            body.putBit(value);
        }

        if (!source.encodeHandles(m_version, body, m_objectHandles))
            return false;
        if (malformedObjectId) {
            // A hard-pointer handle claiming 15 payload bytes with none
            // remaining; the outer object framing and CRC remain valid.
            m_objectHandles.putRawChar8(0x5Fu);
        } else {
            for (const auto& entry : objectIds) {
                dwgHandle ref;
                ref.code = entry.first;
                ref.ref = entry.second;
                ref.ref64 = entry.second;
                std::uint32_t remaining = entry.second;
                while (remaining != 0u) {
                    ++ref.size;
                    remaining >>= 8u;
                }
                m_objectHandles.putHandle(ref);
            }
        }
        if (!body.isGood() || !m_objectHandles.isGood()
            || !appendR2007StringStream(body, m_objectStrings, true))
            return false;

        const std::uint64_t dataBits = body.bitCount();
        const std::uint64_t handleBits = m_objectHandles.bitCount();
        if (dataBits > std::numeric_limits<std::uint32_t>::max()
            || handleBits > std::numeric_limits<std::uint32_t>::max())
            return false;
        const std::uint64_t tailBits = dataBits + handleBits;
        const std::uint8_t alignmentBits = static_cast<std::uint8_t>(
            (8u - (tailBits & 7u)) & 7u);

        dwgBufferW encodedBody;
        if (!appendBits(encodedBody, body)
            || !appendBits(encodedBody, m_objectHandles))
            return false;
        for (std::uint8_t bit = 0; bit < alignmentBits; ++bit)
            encodedBody.putBit(0u);
        const std::uint64_t handleStreamBits = handleBits + alignmentBits;
        if (!encodedBody.isGood() || (encodedBody.bitCount() & 7u) != 0u
            || handleStreamBits > std::numeric_limits<std::uint32_t>::max()
            || encodedBody.size()
                > static_cast<std::size_t>(
                    std::numeric_limits<std::int32_t>::max()))
            return false;

        dwgBufferW frame;
        const std::size_t frameStart = m_buf.size();
        frame.putModularShort(static_cast<std::int32_t>(encodedBody.size()));
        frame.putUModularChar(static_cast<std::uint32_t>(handleStreamBits));
        frame.putBytes(encodedBody.data().data(), encodedBody.data().size());
        const std::uint16_t crc = frame.crc16(0xC0C1u, 0u, frame.size());
        frame.putRawShort16(crc);
        if (!frame.isGood() || !m_buf.isGood())
            return false;
        m_buf.putBytes(frame.data().data(), frame.data().size());
        if (!m_buf.isGood()
            || !this->publishDwgObjectMapEntry(
                handle, static_cast<std::uint32_t>(frameStart)))
            return false;
        m_currentHandle = 0u;
        return true;
    }

    bool encodeLine(std::uint32_t handle, std::uint32_t ownerHandle) {
        DRW_Line line;
        line.handle = handle;
        line.parentHandle = ownerHandle;
        line.basePoint = DRW_Coord{17.0, 23.0, 0.0};
        line.secPoint = DRW_Coord{41.0, 59.0, 0.0};

        dwgBufferW& body = beginObject(handle);
        m_objectStrings.reset();
        m_objectHandles.reset();
        if (!DrwEntityEncodeTestAccess::encodeLine(
                line, m_version, &body, &m_objectStrings, &m_objectHandles))
            return false;
        finishObject();
        return !objectWriteFailed() && m_buf.isGood();
    }

    const std::vector<std::pair<std::uint32_t, std::uint32_t>>&
    objectOffsets() const {
        return m_objectMap;
    }

private:
    static bool appendBits(dwgBufferW& destination,
                           const dwgBufferW& source) {
        dwgBuffer reader(source.data().data(), source.data().size());
        for (std::uint64_t bit = 0; bit < source.bitCount(); ++bit) {
            const std::uint8_t value = reader.getBit();
            if (!reader.isGood())
                return false;
            destination.putBit(value);
        }
        return destination.isGood();
    }
};

template <typename WriterBase>
class ViewportObjectFrameWriter final : public WriterBase {
public:
    using WriterBase::beginObject;
    using WriterBase::finishObject;
    using WriterBase::m_buf;
    using WriterBase::m_objectHandles;
    using WriterBase::m_objectMap;
    using WriterBase::m_objectStrings;
    using WriterBase::m_version;
    using WriterBase::objectWriteFailed;

    ViewportObjectFrameWriter(std::ofstream* stream, DRW_Header* header)
        : WriterBase(stream, header) {}

    bool encodeViewport(std::uint32_t handle, std::uint32_t ownerHandle) {
        DRW_Viewport viewport;
        viewport.handle = handle;
        viewport.parentHandle = ownerHandle;

        dwgBufferW& body = beginObject(handle);
        m_objectStrings.reset();
        m_objectHandles.reset();
        if (!DrwEntityEncodeTestAccess::encodeViewport(
                viewport, m_version, &body, &m_objectStrings,
                &m_objectHandles))
            return false;
        finishObject();
        return !objectWriteFailed() && m_buf.isGood();
    }

    const std::vector<std::pair<std::uint32_t, std::uint32_t>>&
    objectOffsets() const {
        return m_objectMap;
    }

    bool objectBaseOffsetForTest(std::uint32_t& offset) const {
        return this->objectBaseOffset(offset);
    }
};

class R2004SectionMapDecodeProbe final : public dwgReader18 {
public:
    explicit R2004SectionMapDecodeProbe(
        std::vector<std::uint8_t>& bytes,
        DRW::Version formatVersion = DRW::AC1024)
        : dwgReader18(std::make_unique<dwgBuffer>(bytes.data(), bytes.size()),
                      nullptr) {
        version = formatVersion;
        decoder.setVersion(version, false);
    }

    bool readSections() { return readFileHeader(); }

    const dwgSectionInfo* findSection(const std::string& name) const {
        for (const auto& entry : sections) {
            if (entry.second.name == name)
                return &entry.second;
        }
        return nullptr;
    }

    bool decode(const dwgSectionInfo& section,
                std::unique_ptr<std::uint8_t[]>& decoded,
                std::uint64_t& logicalSize) {
        return parseDataPage(section, decoded, logicalSize);
    }
};

using AcDsTestRecord =
    std::pair<std::uint32_t, std::vector<std::uint8_t>>;

std::vector<std::uint8_t> makeInlineAcDsSection(
    const std::vector<AcDsTestRecord>& records,
    const std::vector<std::size_t>& dataIndexOrder = {}) {
    constexpr std::size_t sectionSize = 576u;
    constexpr std::size_t segmentIndexOffset = 64u;
    constexpr std::size_t dataIndexOffset = 192u;
    constexpr std::size_t dataOffset = 320u;
    constexpr std::size_t segmentDataOffset = dataOffset + 48u;
    if (records.size() > 4u
        || records.size()
            > (sectionSize - segmentDataOffset) /
                   DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE)
        return {};

    std::vector<std::size_t> rowOrder = dataIndexOrder;
    if (rowOrder.empty()) {
        rowOrder.resize(records.size());
        for (std::size_t index = 0; index < rowOrder.size(); ++index)
            rowOrder[index] = index;
    }
    if (rowOrder.size() != records.size())
        return {};
    std::vector<bool> recordSeen(records.size(), false);
    for (std::size_t recordIndex : rowOrder) {
        if (recordIndex >= records.size() || recordSeen[recordIndex])
            return {};
        recordSeen[recordIndex] = true;
    }

    const std::size_t headerBytes =
        records.size() * DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE;
    const std::size_t recordDataOffset =
        (segmentDataOffset + headerBytes + 15u) & ~std::size_t{15u};
    std::size_t payloadByteCount = 0;
    for (const AcDsTestRecord& record : records) {
        if (record.second.size() > std::numeric_limits<std::uint32_t>::max()
            || record.second.size()
                > sectionSize - recordDataOffset - 4u - payloadByteCount)
            return {};
        payloadByteCount += 4u + record.second.size();
    }

    std::vector<std::uint8_t> bytes(sectionSize, 0u);
    const auto putU16 = [&bytes](std::size_t offset, std::uint16_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value);
        bytes[offset + 1u] = static_cast<std::uint8_t>(value >> 8);
    };
    const auto putU32 = [&bytes](std::size_t offset, std::uint32_t value) {
        for (unsigned index = 0; index < 4u; ++index)
            bytes[offset + index] =
                static_cast<std::uint8_t>(value >> (index * 8u));
    };
    const auto putU64 = [&putU32](std::size_t offset, std::uint64_t value) {
        putU32(offset, static_cast<std::uint32_t>(value));
        putU32(offset + 4u, static_cast<std::uint32_t>(value >> 32));
    };
    const auto putSegment = [&putU16, &putU32, &bytes](
                                std::size_t offset, const char* name,
                                std::uint32_t index, std::uint32_t size,
                                std::uint32_t objectDataAlignmentOffset) {
        putU16(offset, DRW_DataStorageConst::SEGMENT_SIGNATURE);
        for (std::size_t byte = 0; byte < 6u; ++byte)
            bytes[offset + 2u + byte] =
                static_cast<std::uint8_t>(name[byte]);
        putU32(offset + 8u, index);
        putU32(offset + 16u, size);
        putU32(offset + 24u, 1u);
        putU32(offset + 36u, objectDataAlignmentOffset);
    };

    putU32(4u, DRW_DataStorageConst::HEADER_SIZE);
    putU32(12u, 1u);
    putU32(20u, 1u);
    putU32(24u, static_cast<std::uint32_t>(segmentIndexOffset));
    putU32(32u, 3u); // Segment-index entry count.
    putU32(36u, 0xffffffffu); // No schema-index segment.
    putU32(40u, 1u); // Data-index segment.
    putU32(44u, 0xffffffffu); // No search segment.
    putU32(48u, 0xffffffffu); // No previous-save segment.
    putU32(52u, static_cast<std::uint32_t>(sectionSize));

    putSegment(segmentIndexOffset, "segidx", 0u, 128u, 0u);
    putSegment(dataIndexOffset, "datidx", 1u, 128u, 0u);
    putSegment(dataOffset, "_data_", 2u, 256u,
               static_cast<std::uint32_t>((recordDataOffset - dataOffset) / 16u));
    const std::array<std::pair<std::size_t, std::uint32_t>, 3> segments{{
        {segmentIndexOffset, 128u}, {dataIndexOffset, 128u},
        {dataOffset, 256u}}};
    for (std::size_t index = 0; index < segments.size(); ++index) {
        const std::size_t entry = segmentIndexOffset + 48u + index * 12u;
        putU64(entry, segments[index].first);
        putU32(entry + 8u, segments[index].second);
    }

    putU32(dataIndexOffset + 48u,
           static_cast<std::uint32_t>(records.size()));
    for (std::size_t index = 0; index < records.size(); ++index) {
        const std::size_t recordIndex = rowOrder[index];
        const std::size_t dataIndexEntry = dataIndexOffset + 56u + index * 12u;
        putU32(dataIndexEntry, 2u);
        putU32(dataIndexEntry + 4u, static_cast<std::uint32_t>(
            recordIndex * DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE));
        putU32(dataIndexEntry + 8u, 0u);
    }

    std::size_t payloadLocalOffset = 0u;
    for (std::size_t index = 0; index < records.size(); ++index) {
        const std::size_t header = segmentDataOffset
            + index * DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE;
        putU32(header, DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE);
        putU64(header + 8u, records[index].first);
        putU32(header + 16u,
               static_cast<std::uint32_t>(payloadLocalOffset));

        const std::size_t data = recordDataOffset + payloadLocalOffset;
        putU32(data, static_cast<std::uint32_t>(records[index].second.size()));
        std::copy(records[index].second.begin(), records[index].second.end(),
                  bytes.begin() + static_cast<std::ptrdiff_t>(data + 4u));
        payloadLocalOffset += 4u + records[index].second.size();
    }
    return bytes;
}

std::vector<std::uint8_t> addStaleSecondaryAcDsSegmentIndexWithOrphanData(
    const std::vector<std::uint8_t>& source) {
    if (source.size() < DRW_DataStorageConst::HEADER_SIZE)
        return {};
    const auto readU32 = [&source](std::size_t offset) {
        return static_cast<std::uint32_t>(source[offset])
            | (static_cast<std::uint32_t>(source[offset + 1u]) << 8u)
            | (static_cast<std::uint32_t>(source[offset + 2u]) << 16u)
            | (static_cast<std::uint32_t>(source[offset + 3u]) << 24u);
    };
    const std::size_t segmentIndexOffset = readU32(24u);
    if (readU32(32u) != 3u
        || segmentIndexOffset > source.size()
        || source.size() - segmentIndexOffset
               < DRW_DataStorageConst::SEGMENT_HEADER_SIZE
                     + DRW_DataStorageConst::SEGMENT_INDEX_ENTRY_SIZE) {
        return {};
    }

    const std::size_t firstEntryOffset = segmentIndexOffset
        + DRW_DataStorageConst::SEGMENT_HEADER_SIZE;
    const std::uint32_t segmentIndexSize = readU32(firstEntryOffset + 8u);
    constexpr std::size_t orphanSegmentSize = 128u;
    if (segmentIndexSize < DRW_DataStorageConst::SEGMENT_HEADER_SIZE
        || segmentIndexSize > source.size() - segmentIndexOffset
        || segmentIndexSize
               < DRW_DataStorageConst::SEGMENT_HEADER_SIZE
                     + 4u * DRW_DataStorageConst::SEGMENT_INDEX_ENTRY_SIZE
        || source.size()
               > std::numeric_limits<std::uint32_t>::max()
                     - segmentIndexSize - orphanSegmentSize) {
        return {};
    }

    const std::size_t staleOffset = source.size();
    const std::vector<std::uint8_t> previousIndex(
        source.begin() + static_cast<std::ptrdiff_t>(segmentIndexOffset),
        source.begin() + static_cast<std::ptrdiff_t>(
                             segmentIndexOffset + segmentIndexSize));
    std::vector<std::uint8_t> bytes = source;
    const std::size_t orphanOffset = staleOffset + segmentIndexSize;
    bytes.resize(orphanOffset + orphanSegmentSize, 0u);
    std::copy(previousIndex.begin(), previousIndex.end(),
              bytes.begin() + static_cast<std::ptrdiff_t>(staleOffset));

    const auto putU16 = [&bytes](std::size_t offset, std::uint16_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value);
        bytes[offset + 1u] = static_cast<std::uint8_t>(value >> 8u);
    };
    const auto putU32 = [&bytes](std::size_t offset, std::uint32_t value) {
        for (unsigned index = 0; index < 4u; ++index)
            bytes[offset + index] = static_cast<std::uint8_t>(
                value >> (index * 8u));
    };
    const auto putU64 = [&putU32](std::size_t offset, std::uint64_t value) {
        putU32(offset, static_cast<std::uint32_t>(value));
        putU32(offset + 4u, static_cast<std::uint32_t>(value >> 32u));
    };

    // This well-formed data segment is deliberately absent from the live
    // index. Its record must not appear in the typed data-record projection.
    putU16(orphanOffset, DRW_DataStorageConst::SEGMENT_SIGNATURE);
    const std::string orphanName = "_data_";
    std::copy(orphanName.begin(), orphanName.end(),
              bytes.begin() + static_cast<std::ptrdiff_t>(orphanOffset + 2u));
    putU32(orphanOffset + 8u, 4u);
    putU32(orphanOffset + 16u,
           static_cast<std::uint32_t>(orphanSegmentSize));
    putU32(orphanOffset + 24u, 1u);
    putU32(orphanOffset + 36u, 5u);
    const std::size_t orphanRecordHeader =
        orphanOffset + DRW_DataStorageConst::SEGMENT_HEADER_SIZE;
    putU32(orphanRecordHeader,
           DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE);
    putU64(orphanRecordHeader + 8u, 0xA50u);
    putU32(orphanRecordHeader + 16u, 0u);
    const std::size_t orphanPayload = orphanOffset + 80u;
    constexpr char orphanPayloadText[] = "ACIS BinaryFile UNREFERENCED";
    constexpr std::size_t orphanPayloadLength = sizeof(orphanPayloadText) - 1u;
    putU32(orphanPayload,
           static_cast<std::uint32_t>(orphanPayloadLength));
    std::copy(orphanPayloadText,
              orphanPayloadText + orphanPayloadLength,
              bytes.begin() + static_cast<std::ptrdiff_t>(orphanPayload + 4u));

    // Keep a prior self-contained index snapshot in slot zero, and place the
    // live segment index in slot three. The live index is still found by the
    // file-header offset; its row-zero entry now names the stale snapshot.
    const std::size_t staleSelfEntry = staleOffset
        + DRW_DataStorageConst::SEGMENT_HEADER_SIZE;
    putU32(staleOffset + 8u, 0u);
    putU64(staleSelfEntry, staleOffset);
    putU32(staleSelfEntry + 8u, segmentIndexSize);

    putU32(32u, 4u);
    putU32(52u, static_cast<std::uint32_t>(bytes.size()));
    putU32(segmentIndexOffset + 8u, 3u);
    putU64(firstEntryOffset, staleOffset);
    putU32(firstEntryOffset + 8u, segmentIndexSize);
    const std::size_t liveSelfEntry = firstEntryOffset
        + 3u * DRW_DataStorageConst::SEGMENT_INDEX_ENTRY_SIZE;
    putU64(liveSelfEntry, segmentIndexOffset);
    putU32(liveSelfEntry + 8u, segmentIndexSize);
    return bytes;
}

struct AcDsBlobTestSection {
    std::vector<std::uint8_t> bytes;
    std::size_t secondPageHeaderOffset {0};
    std::size_t secondPageReferenceOffset {0};
    std::size_t firstPageByteLength {0};
    std::size_t secondPageByteLength {0};
    std::vector<std::uint8_t> payload;
};

AcDsBlobTestSection makeTwoPageAcDsSection(
    std::uint32_t handle,
    std::size_t firstPageBytes =
        DRW_DataStorageConst::DATA_BLOB_PAGE_MAX_SIZE,
    std::size_t secondPageBytes = 48u) {
    if (firstPageBytes == 0u
        || firstPageBytes > DRW_DataStorageConst::DATA_BLOB_PAGE_MAX_SIZE
        || secondPageBytes > DRW_DataStorageConst::DATA_BLOB_PAGE_MAX_SIZE)
        return {};
    const std::uint32_t pageCount = secondPageBytes == 0u ? 1u : 2u;
    constexpr std::size_t segmentIndexOffset = 64u;
    constexpr std::size_t segmentIndexSize = 128u;
    constexpr std::size_t dataIndexOffset = segmentIndexOffset
        + segmentIndexSize;
    constexpr std::size_t dataIndexSize = 128u;
    constexpr std::size_t dataOffset = dataIndexOffset + dataIndexSize;
    constexpr std::size_t dataSegmentSize = 192u;
    constexpr std::size_t secondPageOffset = dataOffset + dataSegmentSize;
    const std::size_t secondPageSize =
        (DRW_DataStorageConst::SEGMENT_HEADER_SIZE
         + DRW_DataStorageConst::DATA_BLOB_PAGE_HEADER_SIZE
         + secondPageBytes + 63u)
        & ~std::size_t{63u};
    const std::size_t firstPageOffset = secondPageOffset + secondPageSize;
    const std::size_t firstPageSize =
        (DRW_DataStorageConst::SEGMENT_HEADER_SIZE
         + DRW_DataStorageConst::DATA_BLOB_PAGE_HEADER_SIZE
         + firstPageBytes + 63u)
        & ~std::size_t{63u};
    const std::size_t sectionSize = firstPageOffset + firstPageSize;
    constexpr std::size_t segmentCount = 5u;
    constexpr std::size_t dataRecordHeaderOffset = dataOffset
        + DRW_DataStorageConst::SEGMENT_HEADER_SIZE;
    constexpr std::size_t dataRegionOffset =
        (dataRecordHeaderOffset
         + DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE + 15u)
        & ~std::size_t{15u};
    constexpr std::size_t descriptorOffset = dataRegionOffset + 4u;
    constexpr std::size_t secondPageHeaderOffset = secondPageOffset
        + DRW_DataStorageConst::SEGMENT_HEADER_SIZE;
    constexpr std::size_t secondPageReferenceOffset = descriptorOffset
        + DRW_DataStorageConst::DATA_BLOB_REFERENCE_FIXED_SIZE
        + DRW_DataStorageConst::DATA_BLOB_PAGE_ENTRY_SIZE;

    AcDsBlobTestSection result;
    result.bytes.resize(sectionSize, 0u);
    result.payload.resize(firstPageBytes + secondPageBytes);
    static constexpr std::uint8_t signature[] = {
        'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F', 'i', 'l', 'e'};
    std::copy(std::begin(signature), std::end(signature),
              result.payload.begin());
    std::fill(result.payload.begin() + static_cast<std::ptrdiff_t>(
                  sizeof(signature)),
              result.payload.begin() + static_cast<std::ptrdiff_t>(firstPageBytes),
              0xA5u);
    std::fill(result.payload.begin() + static_cast<std::ptrdiff_t>(firstPageBytes),
              result.payload.end(), 0x5Au);
    result.secondPageHeaderOffset = secondPageHeaderOffset;
    result.secondPageReferenceOffset = secondPageReferenceOffset;
    result.firstPageByteLength = firstPageBytes;
    result.secondPageByteLength = secondPageBytes;

    const auto putU16 = [&result](std::size_t offset, std::uint16_t value) {
        result.bytes[offset] = static_cast<std::uint8_t>(value);
        result.bytes[offset + 1u] = static_cast<std::uint8_t>(value >> 8u);
    };
    const auto putU32 = [&result](std::size_t offset, std::uint32_t value) {
        for (unsigned index = 0; index < 4u; ++index)
            result.bytes[offset + index] = static_cast<std::uint8_t>(
                value >> (index * 8u));
    };
    const auto putU64 = [&putU32](std::size_t offset, std::uint64_t value) {
        putU32(offset, static_cast<std::uint32_t>(value));
        putU32(offset + 4u, static_cast<std::uint32_t>(value >> 32u));
    };
    const auto putSegment = [&putU16, &putU32, &result](
                                std::size_t offset, const char* name,
                                std::uint32_t index, std::uint32_t size,
                                std::uint32_t objectDataAlignmentOffset) {
        putU16(offset, DRW_DataStorageConst::SEGMENT_SIGNATURE);
        for (std::size_t byte = 0; byte < 6u; ++byte)
            result.bytes[offset + 2u + byte] =
                static_cast<std::uint8_t>(name[byte]);
        putU32(offset + 8u, index);
        putU32(offset + 16u, size);
        putU32(offset + 24u, 1u);
        putU32(offset + 36u, objectDataAlignmentOffset);
    };
    const auto putBlobPage = [&putU64, &putU32, &result, pageCount](
                                 std::size_t segmentOffset,
                                 std::uint64_t startOffset,
                                 std::uint32_t pageIndex,
                                 std::size_t payloadOffset,
                                 std::size_t payloadSize) {
        const std::size_t header = segmentOffset
            + DRW_DataStorageConst::SEGMENT_HEADER_SIZE;
        putU64(header, result.payload.size());
        putU64(header + 8u, startOffset);
        putU32(header + 16u, pageIndex);
        putU32(header + 20u, pageCount);
        putU64(header + 24u, payloadSize);
        std::copy_n(result.payload.begin()
                        + static_cast<std::ptrdiff_t>(payloadOffset),
                    payloadSize,
                    result.bytes.begin() + static_cast<std::ptrdiff_t>(
                        header + DRW_DataStorageConst::DATA_BLOB_PAGE_HEADER_SIZE));
    };

    putU32(4u, DRW_DataStorageConst::HEADER_SIZE);
    putU32(12u, 1u);
    putU32(20u, 1u);
    putU32(24u, static_cast<std::uint32_t>(segmentIndexOffset));
    putU32(32u, static_cast<std::uint32_t>(segmentCount));
    putU32(36u, 0xffffffffu); // No schema-index segment.
    putU32(40u, 1u);
    putU32(44u, 0xffffffffu); // No search segment.
    putU32(48u, 0xffffffffu); // No previous-save segment.
    putU32(52u, static_cast<std::uint32_t>(sectionSize));

    putSegment(segmentIndexOffset, "segidx", 0u,
               static_cast<std::uint32_t>(segmentIndexSize), 0u);
    putSegment(dataIndexOffset, "datidx", 1u,
               static_cast<std::uint32_t>(dataIndexSize), 0u);
    putSegment(dataOffset, "_data_", 2u,
               static_cast<std::uint32_t>(dataSegmentSize),
               static_cast<std::uint32_t>((dataRegionOffset - dataOffset) / 16u));
    putSegment(secondPageOffset, "blob01", 3u,
               static_cast<std::uint32_t>(secondPageSize), 0u);
    putSegment(firstPageOffset, "blob01", 4u,
               static_cast<std::uint32_t>(firstPageSize), 0u);

    const std::array<std::pair<std::size_t, std::uint32_t>, segmentCount>
        segments {{{segmentIndexOffset,
                    static_cast<std::uint32_t>(segmentIndexSize)},
                   {dataIndexOffset,
                    static_cast<std::uint32_t>(dataIndexSize)},
                   {dataOffset,
                    static_cast<std::uint32_t>(dataSegmentSize)},
                   {secondPageOffset,
                    static_cast<std::uint32_t>(secondPageSize)},
                   {firstPageOffset,
                    static_cast<std::uint32_t>(firstPageSize)}}};
    for (std::size_t index = 0; index < segments.size(); ++index) {
        const std::size_t entry = segmentIndexOffset
            + DRW_DataStorageConst::SEGMENT_HEADER_SIZE
            + index * DRW_DataStorageConst::SEGMENT_INDEX_ENTRY_SIZE;
        putU64(entry, segments[index].first);
        putU32(entry + 8u, segments[index].second);
    }

    const std::size_t dataIndexEntry = dataIndexOffset
        + DRW_DataStorageConst::SEGMENT_HEADER_SIZE + 8u;
    putU32(dataIndexOffset
               + DRW_DataStorageConst::SEGMENT_HEADER_SIZE,
           1u);
    putU32(dataIndexEntry, 2u);
    putU32(dataIndexEntry + 4u, 0u);

    putU32(dataRecordHeaderOffset,
           DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE);
    putU64(dataRecordHeaderOffset + 8u, handle);
    putU32(dataRecordHeaderOffset + 16u, 0u);
    putU32(dataRegionOffset,
           DRW_DataStorageConst::DATA_BLOB_REFERENCE_MARKER);
    putU64(descriptorOffset, result.payload.size());
    putU32(descriptorOffset + 8u, pageCount);
    putU32(descriptorOffset + 12u,
           DRW_DataStorageConst::DATA_BLOB_REFERENCE_FIXED_SIZE
               + pageCount
                   * DRW_DataStorageConst::DATA_BLOB_PAGE_ENTRY_SIZE);
    putU32(descriptorOffset + 16u, static_cast<std::uint32_t>(firstPageBytes));
    putU32(descriptorOffset + 20u,
           static_cast<std::uint32_t>(pageCount == 1u
                                          ? firstPageBytes
                                          : secondPageBytes));
    putU32(descriptorOffset + 32u, 4u); // Page 0 is physically second.
    putU32(descriptorOffset + 36u,
           static_cast<std::uint32_t>(firstPageBytes));
    if (pageCount > 1u) {
        putU32(secondPageReferenceOffset, 3u);
        putU32(secondPageReferenceOffset + 4u,
               static_cast<std::uint32_t>(secondPageBytes));
    }

    // Page 1 precedes page 0 in the physical section. Descriptor order and
    // page metadata, rather than segment order, define the assembled payload.
    if (pageCount > 1u) {
        putBlobPage(secondPageOffset, firstPageBytes, 1u, firstPageBytes,
                    secondPageBytes);
    }
    putBlobPage(firstPageOffset, 0u, 0u, 0u, firstPageBytes);
    return result;
}

class ModelerDispatchReader final : public dwgReader27 {
public:
    using dwgReader::readDwgBlocks;
    using dwgReader::walkBlockRecordEntities;
    using dwgReader18::captureRawDwgDataSections;

    ModelerDispatchReader(std::unique_ptr<dwgBuffer> buffer, dwgRW* parent)
        : dwgReader27(std::move(buffer), parent) {}

    void setFormatVersion(DRW::Version value) {
        version = value;
        decoder.setVersion(value, false);
    }

    void setPrototypeSectionData(std::vector<std::uint8_t> data) {
        m_prototypeSectionData = std::move(data);
        dwgSectionInfo section;
        section.Id = 1;
        section.size = m_prototypeSectionData.size();
        section.compressed = 1u;
        sections[secEnum::PROTOTYPE] = std::move(section);
    }

    bool replayOwnedUnsupportedObjectTwice(DRW_UnsupportedObject &&value,
                                           DRW_Interface &target,
                                           bool& firstDelivered) {
        DwgBlockJournalOutput output(*this);
        output.appendUnsupportedObject(std::move(value));
        firstDelivered = output.replay(target);
        return output.replay(target);
    }

    bool captureDataStorageSection() {
        return captureRawDwgDataSections();
    }

    std::size_t rawDataStorageSections() const {
        return m_rawDwgSections.size();
    }

    std::size_t parsedDataStorageRecords() const {
        return m_dataStorageSections.empty()
            ? 0u : m_dataStorageSections.front().records.size();
    }

    const DRW_DataStorageRecord* parsedDataStorageRecord(
        std::uint64_t handle) const {
        if (m_dataStorageSections.empty())
            return nullptr;
        const auto& records = m_dataStorageSections.front().records;
        const auto found = std::find_if(
            records.begin(), records.end(), [handle](const auto& record) {
                return record.handle == handle;
            });
        return found == records.end() ? nullptr : &*found;
    }

    bool rawDataStorageSectionEquals(
        const std::vector<std::uint8_t>& expected) const {
        return m_rawDwgSections.size() == 1u
            && m_rawDwgSections.front().m_data == expected;
    }

    void finalizeDataStorage() { finalizeDataStorageLinks(); }
    bool readBlocks(std::vector<std::uint8_t>& bytes, DRW_Interface& sink) {
        dwgBuffer input(bytes.data(), bytes.size(), &decoder);
        return dwgReader::readDwgBlocks(sink, &input);
    }
    const std::vector<DwgEntityFailureDiagnostic>& entityFailures() const {
        return m_entityFailureDiagnostics;
    }
    std::size_t dataStorageLinkFailures() const {
        return m_dataStorageLinkFailures;
    }
    std::size_t dataStorageOrphans() const {
        return m_dataStorageOrphanRecords;
    }

protected:
    bool parseDataPage(const dwgSectionInfo& section,
                       std::unique_ptr<std::uint8_t[]>& sectionData,
                       std::uint64_t& sectionSize) override {
        if (section.size != m_prototypeSectionData.size())
            return false;
        sectionSize = section.size;
        sectionData.reset(new std::uint8_t[
            static_cast<std::size_t>(sectionSize)]);
        std::copy(m_prototypeSectionData.begin(),
                  m_prototypeSectionData.end(), sectionData.get());
        return true;
    }

private:
    std::vector<std::uint8_t> m_prototypeSectionData;
};

template <typename ReaderBase>
class ProxyEntityDispatchReader final : public ReaderBase {
public:
    ProxyEntityDispatchReader(std::vector<std::uint8_t>& bytes, dwgRW* parent,
                              DRW::Version sourceVersion)
        : ReaderBase(std::make_unique<dwgBuffer>(bytes.data(), bytes.size()),
                     parent) {
        this->version = sourceVersion;
        this->decoder.setVersion(sourceVersion, false);
    }

    bool readFrame(std::vector<std::uint8_t>& bytes, std::uint32_t type,
                   std::uint32_t handle, std::uint32_t offset,
                   DRW_Interface& sink, bool* frameFailure = nullptr) {
        dwgBuffer input(bytes.data(), bytes.size(), &this->decoder);
        objHandle object(type, handle, offset);
        return this->readDwgEntity(&input, object, sink, frameFailure);
    }
};

class ModelerDispatchSink : public dx_iface {
public:
    struct OwnedSolid {
        std::uint32_t walkOwner;
        DRW_ModelerGeometry entity;
    };
    struct OwnedInsert {
        std::uint32_t walkOwner;
        DRW_Insert entity;
    };

    ModelerDispatchSink() {
        cData = &data;
        currentBlock = data.mBlock;
    }

    void addModelerGeometry(const DRW_ModelerGeometry& value) override {
        modeler.push_back({activeWalkOwner, value});
    }

    void addModelerGeometry(DRW_ModelerGeometry&& value) override {
        ++modelerMoveCallbacks;
        modeler.push_back({activeWalkOwner, std::move(value)});
    }

    void addInsert(const DRW_Insert& value) override {
        inserts.push_back({activeWalkOwner, value});
    }

    void addUnsupportedObject(const DRW_UnsupportedObject& value) override {
        if (value.m_isEntity) {
            rawEntityFrameMatchesExpected = expectedRawEntityFrame != nullptr
                && value.m_rawBytes.data() == expectedRawEntityFrame
                && value.m_rawBytes.size() == expectedRawEntityFrameSize;
            rawEntityCallbacks.push_back(value);
            return;
        }
        dx_iface::addUnsupportedObject(value);
    }

    dx_data data;
    std::uint32_t activeWalkOwner{DRW::NoHandle};
    std::vector<OwnedSolid> modeler;
    std::vector<OwnedInsert> inserts;
    std::vector<DRW_UnsupportedObject> rawEntityCallbacks;
    std::size_t modelerMoveCallbacks{0};
    const std::uint8_t* expectedRawEntityFrame{nullptr};
    std::size_t expectedRawEntityFrameSize{0};
    bool rawEntityFrameMatchesExpected{false};
};

class AcDsOrderDispatchSink final : public ModelerDispatchSink {
public:
    void addLine(const DRW_Line& value) override { lines.push_back(value); }

    std::vector<DRW_Line> lines;
};

class RawObjectRecordingSink : public dx_iface {
public:
    RawObjectRecordingSink() {
        cData = &data;
        currentBlock = data.mBlock;
    }

    void addUnsupportedObject(const DRW_UnsupportedObject& value) override {
        rawObjects.push_back(value);
        dx_iface::addUnsupportedObject(value);
    }

    dx_data data;
    std::vector<DRW_UnsupportedObject> rawObjects;
};

class AssociativeObjectRecordingSink final : public RawObjectRecordingSink {
public:
    void addAssociativeObject(const DRW_AssociativeObject& value) override {
        associativeObjects.push_back(value);
        dx_iface::addAssociativeObject(value);
    }

    std::vector<DRW_AssociativeObject> associativeObjects;
};

class LayoutChainDispatchSink final : public ModelerDispatchSink {
public:
    struct LineMembership {
        std::string layoutName;
        std::uint32_t layoutHandle;
        std::uint32_t blockRecordHandle;
        std::uint32_t entityHandle;
        std::uint32_t entityOwnerHandle;
        DRW::Space space;
    };

    void beginLayout(const std::string& name, std::uint32_t layout,
                     std::uint32_t blockRecord) {
        activeLayoutName = name;
        activeLayoutHandle = layout;
        activeBlockRecordHandle = blockRecord;
        activeWalkOwner = blockRecord;
    }

    void addBlockRecord(const DRW_Block_Record& value) override {
        activeLayoutName = value.name;
        activeLayoutHandle = value.layoutHandle;
        activeBlockRecordHandle = value.handle;
        activeWalkOwner = value.handle;
        blockRecords.push_back(value.handle);
        ModelerDispatchSink::addBlockRecord(value);
    }

    void addBlock(const DRW_Block& value) override {
        blocks.push_back(value.name);
        ModelerDispatchSink::addBlock(value);
    }

    void endBlock() override {
        ++endBlocks;
        ModelerDispatchSink::endBlock();
    }

    void addLine(const DRW_Line& value) override {
        lines.push_back({activeLayoutName, activeLayoutHandle,
                         activeBlockRecordHandle, value.handle,
                         value.parentHandle, value.space});
        dx_iface::addLine(value);
    }

    std::string activeLayoutName;
    std::uint32_t activeLayoutHandle{DRW::NoHandle};
    std::uint32_t activeBlockRecordHandle{DRW::NoHandle};
    std::vector<LineMembership> lines;
    std::vector<std::uint32_t> blockRecords;
    std::vector<std::string> blocks;
    std::size_t endBlocks{0};
};

class MoveAwareUnsupportedObjectSink final : public ModelerDispatchSink {
public:
    void addUnsupportedObject(DRW_UnsupportedObject&& value) override {
        rawEntityFrameMatchesExpected = expectedRawEntityFrame != nullptr
            && value.m_rawBytes.data() == expectedRawEntityFrame
            && value.m_rawBytes.size() == expectedRawEntityFrameSize;
        rawEntityMoveCallbacks.push_back(std::move(value));
    }

    std::vector<DRW_UnsupportedObject> rawEntityMoveCallbacks;
};

class BlockPreflightSink final : public ModelerDispatchSink {
public:
    void addBlockRecord(const DRW_Block_Record& value) override {
        blockRecords.push_back(value.handle);
        dx_iface::addBlockRecord(value);
    }
    void addBlock(const DRW_Block& value) override {
        blocks.push_back(value.name);
        dx_iface::addBlock(value);
    }
    void endBlock() override {
        ++endBlocks;
        dx_iface::endBlock();
    }
    void addLine(const DRW_Line& value) override {
        lines.push_back(value.handle);
        dx_iface::addLine(value);
    }

    std::vector<std::uint32_t> blockRecords;
    std::vector<std::string> blocks;
    std::vector<std::uint32_t> lines;
    std::size_t endBlocks{0};
};

class ProxyEntityDispatchSink final : public dx_iface {
public:
    void addProxyEntity(const DRW_ProxyEntity& value) override {
        proxies.push_back(value);
    }
    void addUnsupportedObject(const DRW_UnsupportedObject& value) override {
        if (value.m_isEntity)
            rawEntities.push_back(value);
    }
    void addLine(const DRW_Line& value) override { lines.push_back(value); }

    std::vector<DRW_ProxyEntity> proxies;
    std::vector<DRW_UnsupportedObject> rawEntities;
    std::vector<DRW_Line> lines;
};

void testProxyEntityDispatch(TestContext& t) {
    constexpr std::uint32_t ownerHandle = 0x400u;
    constexpr std::uint32_t absentGraphicsHandle = 0x900u;
    constexpr std::uint32_t presentGraphicsHandle = 0x901u;
    constexpr std::uint32_t sentinelHandle = 0x910u;
    constexpr std::uint32_t firstObjectId = 0xA00u;
    constexpr std::uint32_t secondObjectId = 0xA01u;
    const std::vector<std::pair<std::uint8_t, std::uint32_t>> objectIds{
        {5u, firstObjectId}, {2u, secondObjectId}};
    const std::vector<std::uint8_t> noGraphics;
    const std::vector<std::uint8_t> graphics{0x11u, 0x22u, 0x33u};
    const std::vector<std::uint8_t> firstPayload{0xB0u};
    const std::vector<std::uint8_t> secondPayload{0xC0u};

    std::ofstream output;
    DRW_Header header;
    ProxyEntityDispatchFrameWriter<dwgWriter24> writer(&output, &header);
    const bool generated = writer.encodeProxyEntity(
            absentGraphicsHandle, ownerHandle, firstPayload, 5u, noGraphics,
            objectIds)
        && writer.encodeProxyEntity(presentGraphicsHandle, ownerHandle,
                                    secondPayload, 3u, graphics, objectIds)
        && writer.encodeLine(sentinelHandle, ownerHandle);
    t.expect(generated,
             "in-memory AC1024 writer frames proxy payloads and a following LINE");
    if (!generated)
        return;

    std::vector<std::uint8_t> bytes = writer.buffer();
    const auto offsetFor = [&writer](std::uint32_t handle) {
        const auto found = std::find_if(
            writer.objectOffsets().begin(), writer.objectOffsets().end(),
            [handle](const auto& entry) { return entry.first == handle; });
        return found == writer.objectOffsets().end() ? 0u : found->second;
    };
    const std::uint32_t absentOffset = offsetFor(absentGraphicsHandle);
    const std::uint32_t presentOffset = offsetFor(presentGraphicsHandle);
    const std::uint32_t sentinelOffset = offsetFor(sentinelHandle);
    t.expect(!bytes.empty() && absentOffset == 0u && presentOffset > absentOffset
                 && sentinelOffset > presentOffset,
             "generated AC1024 proxy frames precede the LINE sentinel");

    dwgRW parent("");
    ProxyEntityDispatchReader<dwgReader24> reader(
        bytes, &parent, DRW::AC1024);
    ProxyEntityDispatchSink sink;
    const bool absentRead = reader.readFrame(
        bytes, DRW_ProxyEntity::kDwgType, absentGraphicsHandle, absentOffset,
        sink);
    const bool presentRead = reader.readFrame(
        bytes, DRW_ProxyEntity::kDwgType, presentGraphicsHandle, presentOffset,
        sink);
    const bool sentinelRead = reader.readFrame(
        bytes, 19u, sentinelHandle, sentinelOffset, sink);
    t.expect(absentRead && presentRead && sentinelRead,
             "AC1024 proxy entity dispatch preserves following LINE alignment");

    const bool callbacksMatch = sink.proxies.size() == 2u
        && sink.rawEntities.size() == 2u && sink.lines.size() == 1u;
    t.expect(callbacksMatch,
             "AC1024 dispatch emits two typed/raw proxies and one sentinel LINE");
    if (callbacksMatch) {
        const DRW_ProxyEntity& absent = sink.proxies[0];
        const DRW_ProxyEntity& present = sink.proxies[1];
        const bool metadataMatches = absent.handle == absentGraphicsHandle
            && absent.m_hasProxyCarrierId
            && absent.m_proxyCarrierId == DRW_ProxyEntity::kDwgType
            && absent.m_hasProxyClassId && absent.m_proxyClassId == 0x1234
            && absent.m_proxySubclass == "AcDbProxyEntity"
            && absent.m_hasProxyDrawingFormat
            && absent.m_proxyDrawingFormat == 0x00102401u
            && absent.m_hasFromDxf && !absent.m_fromDxf;
        const bool payloadSizeFlagsMatch = absent.m_hasEntityDataBitSize
            && present.m_hasEntityDataBitSize;
        const bool payloadBitCountsMatch = absent.m_entityDataBitSize == 5u
            && present.m_entityDataBitSize == 3u;
        const bool payloadBytesMatch = absent.m_entityData.size() == 1u
            && absent.m_entityData[0] == 0xB0u
            && present.m_entityData.size() == 1u
            && present.m_entityData[0] == 0xC0u;
        bool referencesMatch = true;
        for (const DRW_ProxyEntity* proxy : {&absent, &present}) {
            referencesMatch = referencesMatch
                && proxy->m_objectIdRefs.size() == objectIds.size()
                && proxy->m_objectIdRefs[0].m_handleCode == 5u
                && proxy->m_objectIdRefs[0].m_dxfCode == 340
                && proxy->m_objectIdRefs[0].m_handle == firstObjectId
                && proxy->m_objectIdRefs[0].m_rawHandle == firstObjectId
                && proxy->m_objectIdRefs[1].m_handleCode == 2u
                && proxy->m_objectIdRefs[1].m_dxfCode == 350
                && proxy->m_objectIdRefs[1].m_handle == secondObjectId
                && proxy->m_objectIdRefs[1].m_rawHandle == secondObjectId;
        }
        const bool graphicsMatch = !absent.m_hasProxyGraphicsByteSize
            && absent.proxyGraphics.empty()
            && present.handle == presentGraphicsHandle
            && present.m_hasProxyGraphicsByteSize
            && present.m_proxyGraphicsByteSize == graphics.size()
            && present.proxyGraphics.size() == graphics.size()
            && std::equal(present.proxyGraphics.begin(),
                          present.proxyGraphics.end(), graphics.begin(),
                          [](char actual, std::uint8_t expected) {
                              return static_cast<std::uint8_t>(actual)
                                  == expected;
                          });
        const bool rawAndSentinelMatch =
            sink.rawEntities[0].m_handle == absentGraphicsHandle
            && sink.rawEntities[0].m_recordName == "ACAD_PROXY_ENTITY"
            && sink.rawEntities[0].m_className == "AcDbProxyEntity"
            && sink.rawEntities[0].m_rawBytes.size() > 0u
            && sink.rawEntities[1].m_handle == presentGraphicsHandle
            && sink.rawEntities[1].m_recordName == "ACAD_PROXY_ENTITY"
            && sink.lines[0].handle == sentinelHandle
            && sink.lines[0].basePoint.x == 17.0
            && sink.lines[0].secPoint.y == 59.0;
        t.expect(metadataMatches,
                 "AC1024 proxy metadata maps the DWG class ID and carrier correctly");
        t.expect(payloadSizeFlagsMatch,
                 "AC1024 proxy marks entity-data bit-size presence");
        t.expect(payloadBitCountsMatch,
                 "AC1024 proxy preserves non-byte-aligned entity-data bit counts");
        t.expect(payloadBytesMatch,
                 "AC1024 proxy preserves entity-data bits packed in wire order");
        t.expect(referencesMatch,
                 "AC1024 proxy preserves typed object IDs in wire order");
        t.expect(graphicsMatch,
                 "AC1024 proxy graphics byte presence, count, and bytes are independent");
        t.expect(rawAndSentinelMatch,
                 "AC1024 proxy publishes raw fallback and leaves following LINE intact");
    }

    std::ofstream r2018Output;
    ProxyEntityDispatchFrameWriter<dwgWriter32> r2018Writer(
        &r2018Output, &header);
    const bool r2018Generated = r2018Writer.encodeProxyEntity(
            0x930u, ownerHandle, std::vector<std::uint8_t>{0xA0u}, 7u,
            noGraphics, {})
        && r2018Writer.encodeLine(0x931u, ownerHandle);
    t.expect(r2018Generated,
             "in-memory AC1032 writer frames an R2018 proxy and LINE sentinel");
    if (r2018Generated) {
        std::vector<std::uint8_t> r2018Bytes = r2018Writer.buffer();
        const auto r2018OffsetFor = [&r2018Writer](std::uint32_t handle) {
            const auto found = std::find_if(
                r2018Writer.objectOffsets().begin(),
                r2018Writer.objectOffsets().end(),
                [handle](const auto& entry) { return entry.first == handle; });
            return found == r2018Writer.objectOffsets().end()
                ? 0u : found->second;
        };
        const std::uint32_t proxyOffset = r2018OffsetFor(0x930u);
        const std::uint32_t lineOffset = r2018OffsetFor(0x931u);
        dwgRW r2018Parent("");
        ProxyEntityDispatchReader<dwgReader32> r2018Reader(
            r2018Bytes, &r2018Parent, DRW::AC1032);
        ProxyEntityDispatchSink r2018Sink;
        const bool proxyRead = r2018Reader.readFrame(
            r2018Bytes, DRW_ProxyEntity::kDwgType, 0x930u, proxyOffset,
            r2018Sink);
        const bool lineRead = r2018Reader.readFrame(
            r2018Bytes, 19u, 0x931u, lineOffset, r2018Sink);
        const bool r2018CallbacksMatch = proxyRead && lineRead
            && r2018Sink.proxies.size() == 1u
            && r2018Sink.rawEntities.size() == 1u
            && r2018Sink.lines.size() == 1u;
        t.expect(r2018CallbacksMatch,
                 "AC1032 proxy dispatch preserves following LINE alignment");
        if (r2018CallbacksMatch) {
            const DRW_ProxyEntity& proxy = r2018Sink.proxies.front();
            const bool r2018MetadataMatches = proxy.m_hasProxyClassId
                && proxy.m_proxyClassId == 0x1234
                && proxy.m_proxySubclass == "AcDbProxyEntity"
                && proxy.m_hasProxyDrawingFormat
                && proxy.m_proxyDwgVersion == 0x1122u
                && proxy.m_proxyMaintenanceVersion == 0x3344u
                && proxy.m_proxyDrawingFormat == 0x33441122u
                && proxy.m_hasFromDxf && !proxy.m_fromDxf;
            const bool r2018PayloadMatches = proxy.m_hasEntityDataBitSize
                && proxy.m_entityDataBitSize == 7u
                && proxy.m_entityData.size() == 1u
                && proxy.m_entityData.front() == 0xA0u;
            const bool r2018SentinelMatches =
                r2018Sink.lines.front().handle == 0x931u
                && r2018Sink.lines.front().secPoint.y == 59.0;
            t.expect(r2018MetadataMatches,
                     "AC1032 proxy maps separate DWG and maintenance versions");
            t.expect(r2018PayloadMatches,
                     "AC1032 proxy preserves non-byte-aligned entity data");
            t.expect(r2018SentinelMatches,
                     "AC1032 proxy leaves the following LINE intact");
        }
    }

    std::ofstream malformedOutput;
    ProxyEntityDispatchFrameWriter<dwgWriter24> malformedWriter(
        &malformedOutput, &header);
    const bool malformedGenerated = malformedWriter.encodeProxyEntity(
        0x920u, ownerHandle, firstPayload, 5u, noGraphics, {}, true);
    t.expect(malformedGenerated,
             "in-memory AC1024 writer frames a bounded malformed proxy-ID stream");
    if (!malformedGenerated)
        return;
    std::vector<std::uint8_t> malformedBytes = malformedWriter.buffer();
    const std::uint32_t malformedOffset = malformedWriter.objectOffsets()[0].second;
    dwgRW malformedParent("");
    ProxyEntityDispatchReader<dwgReader24> malformedReader(
        malformedBytes, &malformedParent, DRW::AC1024);
    ProxyEntityDispatchSink malformedSink;
    t.expect(!malformedReader.readFrame(
                 malformedBytes, DRW_ProxyEntity::kDwgType, 0x920u,
                 malformedOffset, malformedSink)
                 && malformedSink.proxies.empty()
                 && malformedSink.rawEntities.empty(),
             "truncated typed object-ID payload fails before typed or raw callback");

    std::vector<std::uint8_t> truncatedBytes = malformedWriter.buffer();
    truncatedBytes.pop_back(); // remove one byte from the bounded frame/CRC
    dwgRW truncatedParent("");
    ProxyEntityDispatchReader<dwgReader24> truncatedReader(
        truncatedBytes, &truncatedParent, DRW::AC1024);
    ProxyEntityDispatchSink truncatedSink;
    t.expect(!truncatedReader.readFrame(
                 truncatedBytes, DRW_ProxyEntity::kDwgType, 0x920u, 0u,
                 truncatedSink)
                 && truncatedSink.proxies.empty()
                 && truncatedSink.rawEntities.empty(),
             "truncated proxy object frame fails without partial callbacks");
}

template <typename WriterBase>
void verifySerializedViewportObjectFrame(
    TestContext& t, const ViewportObjectFrameWriter<WriterBase>& writer,
    DRW::Version version, std::uint32_t handle, std::uint32_t ownerHandle,
    const std::string& label) {
    const std::vector<std::uint8_t>& bytes = writer.buffer();
    DRW::DwgObjectFrameReceipt receipt;
    const bool hasReceipt = writer.getLastDwgObjectFrame(receipt);
    const auto& objectMap = writer.objectOffsets();
    const bool mapMatches = objectMap.size() == 1u
        && objectMap.front().first == handle
        && objectMap.front().second == 0u;
    const bool receiptMatches = hasReceipt && receipt.valid
        && receipt.generation != 0u && receipt.objectHandle == handle
        && receipt.version == version && receipt.classNumber == 34u
        && !receipt.occurrences.empty();
    t.expect(mapMatches && receiptMatches && !bytes.empty(),
             label + ": writer commits one type-34 frame and its object-map offset");
    if (!mapMatches || !receiptMatches || bytes.empty())
        return;

    const std::uint64_t frameStart = objectMap.front().second;
    dwgBuffer framePrefix(bytes.data() + frameStart, bytes.size() - frameStart);
    const std::int32_t bodyLengthSigned = framePrefix.getModularShort();
    const std::uint64_t prefixAfterSize = framePrefix.getPosition();
    const bool modern = version >= DRW::AC1024;
    const std::uint64_t handleBitCount = modern
        ? framePrefix.getUModularChar() : 0u;
    const std::uint64_t prefixBytes = framePrefix.getPosition();
    const bool prefixGood = framePrefix.isGood() && bodyLengthSigned > 0
        && prefixAfterSize > 0u && prefixBytes >= prefixAfterSize;
    if (!prefixGood) {
        t.expect(false, label + ": raw frame has a readable size prefix");
        return;
    }

    const std::uint64_t bodyLength =
        static_cast<std::uint64_t>(bodyLengthSigned);
    const std::uint64_t handleBytes = modern ? handleBitCount / 8u : 0u;
    const bool handleSizeValid = !modern
        || (handleBitCount != 0u && (handleBitCount & 7u) == 0u
            && handleBytes < bodyLength);
    const std::uint64_t dataBytes = modern
        ? bodyLength - handleBytes : bodyLength;
    const std::uint64_t bodyStart = frameStart + prefixBytes;
    const std::uint64_t crcOffset = bodyStart + bodyLength;
    const std::uint64_t frameEnd = crcOffset + 2u;
    const bool frameBoundsValid = handleSizeValid
        && frameEnd == bytes.size() && bodyStart <= crcOffset
        && crcOffset <= bytes.size();
    t.expect(frameBoundsValid,
             label + ": modular prefix and R2010+ handle-bit count bound the physical frame");
    if (!frameBoundsValid)
        return;

    dwgBuffer objectBody(bytes.data() + bodyStart, dataBytes);
    const std::uint16_t objectType = objectBody.getObjType(version);
    std::uint32_t legacyDataBitCount = 0u;
    bool legacySizeGood = true;
    if (!modern) {
        legacyDataBitCount = objectBody.getRawLong32();
        legacySizeGood = objectBody.isGood()
            && legacyDataBitCount <= bodyLength * 8u;
    }
    const bool typeAndReceiptLayout = objectBody.isGood()
        && objectType == 34u && legacySizeGood
        && (modern
                ? receipt.dataBitSize == dataBytes * 8u
                    && receipt.handleBitSize == handleBitCount
                : receipt.handleBitSize == 0u
                    && receipt.dataBitSize == bodyLength * 8u);
    t.expect(typeAndReceiptLayout,
             label + ": raw object type and receipt agree with the versioned segment boundary");
    if (!typeAndReceiptLayout)
        return;

    dwgBuffer storedCrcReader(bytes.data() + crcOffset, 2u);
    const std::uint16_t storedCrc = storedCrcReader.getRawShort16();
    dwgBufferW crcInput;
    crcInput.putBytes(bytes.data() + frameStart,
                      static_cast<std::size_t>(crcOffset - frameStart));
    const std::uint16_t computedCrc =
        crcInput.crc16(0xC0C1u, 0u, crcInput.size());
    const bool crcMatches = storedCrcReader.isGood() && crcInput.isGood()
        && storedCrc == computedCrc;
    t.expect(crcMatches,
             label + ": stored CRC covers the raw size prefix and complete object body");

    bool allTokensMatchBytes = true;
    bool segmentLayoutMatches = true;
    bool legacyHandleTailStartsAtDataBoundary = true;
    std::size_t ownerTokenCount = 0u;
    std::size_t handleStreamTokenCount = 0u;
    for (const DRW::DwgObjectHandleOccurrence& occurrence
         : receipt.occurrences) {
        const bool isHandleStream =
            occurrence.stream == DRW::DwgObjectHandleStream::Handles;
        const bool shouldBeHandleSegment = modern && isHandleStream;
        segmentLayoutMatches = segmentLayoutMatches
            && (occurrence.serializedSegment
                == (shouldBeHandleSegment
                        ? DRW::DwgObjectSerializedSegment::Handles
                        : DRW::DwgObjectSerializedSegment::Data));
        if (isHandleStream) {
            ++handleStreamTokenCount;
            if (!modern) {
                legacyHandleTailStartsAtDataBoundary =
                    legacyHandleTailStartsAtDataBoundary
                    && occurrence.token.startBit >= legacyDataBitCount;
            }
        }

        if (isHandleStream && occurrence.token.code == 4u
            && occurrence.token.reference == ownerHandle)
            ++ownerTokenCount;

        const std::uint64_t segmentSizeBytes =
            occurrence.serializedSegment
                    == DRW::DwgObjectSerializedSegment::Handles
                ? handleBytes : dataBytes;
        const std::uint8_t* segmentData = bytes.data()
            + (occurrence.serializedSegment
                       == DRW::DwgObjectSerializedSegment::Handles
                   ? bodyStart + dataBytes : bodyStart);
        const bool tokenBoundsValid = occurrence.objectHandle == handle
            && occurrence.token.endBit >= occurrence.token.startBit
            && occurrence.token.endBit <= segmentSizeBytes * 8u
            && occurrence.token.startBit < segmentSizeBytes * 8u;
        if (!tokenBoundsValid) {
            allTokensMatchBytes = false;
            continue;
        }

        dwgBuffer tokenReader(segmentData, segmentSizeBytes);
        const bool cursorSet = tokenReader.setPosition(
            occurrence.token.startBit / 8u);
        tokenReader.setBitPos(static_cast<std::uint8_t>(
            occurrence.token.startBit & 7u));
        const dwgHandle decoded = cursorSet ? tokenReader.getHandle()
                                            : dwgHandle{};
        const std::uint64_t decodedEndBit = tokenReader.getPosition() * 8u
            + tokenReader.getBitPos();
        allTokensMatchBytes = allTokensMatchBytes && cursorSet
            && tokenReader.isGood()
            && decoded.code == occurrence.token.code
            && decoded.ref64 == occurrence.token.reference
            && decodedEndBit == occurrence.token.endBit;
    }

    const bool ownerAndSegmentMatch = segmentLayoutMatches
        && handleStreamTokenCount > 0u && ownerTokenCount == 1u
        && legacyHandleTailStartsAtDataBoundary;
    t.expect(ownerAndSegmentMatch,
             label + ": owner H-token is in the physical handle segment at the versioned boundary");
    t.expect(allTokensMatchBytes,
             label + ": every captured handle occurrence decodes from its serialized byte span");
}

void testViewportObjectFrameSerialization(TestContext& t) {
    constexpr std::uint32_t viewportHandle = 0x501u;
    constexpr std::uint32_t ownerHandle = 0x400u;
    DRW_Header header;

    std::ofstream legacyOutput;
    ViewportObjectFrameWriter<dwgWriter15> legacyWriter(
        &legacyOutput, &header);
    const bool legacyGenerated =
        legacyWriter.encodeViewport(viewportHandle, ownerHandle);
    t.expect(legacyGenerated,
             "generated AC1015 writer commits a type-34 VIEWPORT frame");
    if (legacyGenerated) {
        verifySerializedViewportObjectFrame(
            t, legacyWriter, DRW::AC1015, viewportHandle, ownerHandle,
            "AC1015 VIEWPORT");
    }

    std::ofstream modernOutput;
    ViewportObjectFrameWriter<dwgWriter24> modernWriter(
        &modernOutput, &header);
    const bool modernGenerated =
        modernWriter.encodeViewport(viewportHandle, ownerHandle);
    t.expect(modernGenerated,
             "generated AC1024 writer commits a type-34 VIEWPORT frame");
    if (modernGenerated) {
        verifySerializedViewportObjectFrame(
            t, modernWriter, DRW::AC1024, viewportHandle, ownerHandle,
            "AC1024 VIEWPORT");
    }
}

void testFileLevelViewportHandlesSection(TestContext& t) {
    constexpr std::size_t generatedViewportCount = 1200u;
    constexpr std::uint32_t firstViewportHandle = 0x1000u;
    constexpr std::uint32_t ownerHandle = DRW_Vport::kDwgControlHandle;

    std::stringbuf streamBuffer(std::ios::out | std::ios::binary);
    std::ofstream output;
    static_cast<std::ostream&>(output).rdbuf(&streamBuffer);
    DRW_Header header;
    header.addInt("$MEASUREMENT", 1, 70);
    ViewportObjectFrameWriter<dwgWriter15> writer(&output, &header);

    bool generated = writer.prepareDwgClassManifest()
        && writer.writeFileHeaderStub() && writer.writeDwgHeader()
        && writer.writeDwgClasses() && writer.writeDwgObjects();
    std::vector<std::pair<std::uint32_t, std::uint32_t>> viewportFrames;
    viewportFrames.reserve(generatedViewportCount);
    std::vector<DRW::DwgObjectFrameReceipt> frameReceipts;
    frameReceipts.reserve(generatedViewportCount);

    for (std::size_t index = 0; generated
         && index < generatedViewportCount; ++index) {
        const auto handle = firstViewportHandle
            + static_cast<std::uint32_t>(index);
        generated = writer.encodeViewport(handle, ownerHandle);
        DRW::DwgObjectFrameReceipt receipt;
        const auto& objectMap = writer.objectOffsets();
        generated = generated && writer.getLastDwgObjectFrame(receipt)
            && receipt.valid && receipt.objectHandle == handle
            && receipt.version == DRW::AC1015 && receipt.classNumber == 34u
            && !objectMap.empty() && objectMap.back().first == handle;
        if (generated) {
            frameReceipts.push_back(receipt);
            viewportFrames.push_back(objectMap.back());
        }
    }

    generated = generated && writer.emitDeferredBlockControl();
    std::vector<std::pair<std::uint32_t, std::uint32_t>> expectedMap =
        writer.objectOffsets();
    std::sort(expectedMap.begin(), expectedMap.end());

    generated = generated && writer.finalizeHeaderHandseed()
        && writer.writeDwgHandles()
        && writer.writeSecondHeader() && writer.finalize();
    t.expect(generated,
             "generated AC1015 file envelope finalizes many VIEWPORT frames and HANDLES pages");
    if (!generated)
        return;

    const std::string encoded = streamBuffer.str();
    const std::vector<std::uint8_t> file(encoded.begin(), encoded.end());
    const auto readRawLongAt = [&file](std::size_t offset,
                                       std::uint32_t& value) {
        if (offset > file.size() || file.size() - offset < 4u)
            return false;
        value = static_cast<std::uint32_t>(file[offset])
            | (static_cast<std::uint32_t>(file[offset + 1u]) << 8u)
            | (static_cast<std::uint32_t>(file[offset + 2u]) << 16u)
            | (static_cast<std::uint32_t>(file[offset + 3u]) << 24u);
        return true;
    };

    std::uint32_t sectionCount = 0;
    const bool headerHasSectionCount = file.size() >= 0x19u
        && readRawLongAt(0x15u, sectionCount)
        && sectionCount <= (file.size() - 0x19u) / 9u;
    std::uint32_t handlesOffset = 0;
    std::uint32_t handlesSize = 0;
    bool foundHandlesSection = false;
    for (std::uint32_t index = 0;
         headerHasSectionCount && index < sectionCount; ++index) {
        const std::size_t locator = 0x19u
            + static_cast<std::size_t>(index) * 9u;
        if (file[locator] != 2u)
            continue;
        foundHandlesSection = readRawLongAt(locator + 1u, handlesOffset)
            && readRawLongAt(locator + 5u, handlesSize);
        break;
    }
    const bool handlesSectionBounds = headerHasSectionCount
        && foundHandlesSection && handlesSize >= 4u
        && handlesOffset <= file.size()
        && handlesSize <= file.size() - handlesOffset;
    t.expect(handlesSectionBounds,
             "finalized AC1015 section locator bounds the HANDLES byte range");
    if (!handlesSectionBounds)
        return;

    std::vector<std::pair<std::uint32_t, std::uint32_t>> wireMap;
    std::unordered_set<std::uint32_t> wireHandles;
    std::unordered_set<std::uint32_t> wireOffsets;
    const std::size_t handlesEnd = static_cast<std::size_t>(handlesOffset)
        + handlesSize;
    std::size_t cursor = handlesOffset;
    std::size_t dataPageCount = 0u;
    bool sawTerminator = false;
    bool allPageCrcsValid = true;
    bool allPageEntriesValid = true;
    while (cursor < handlesEnd && !sawTerminator) {
        const std::size_t remaining = handlesEnd - cursor;
        if (remaining < 4u) {
            allPageCrcsValid = false;
            break;
        }

        dwgBuffer pageHeader(
            const_cast<std::uint8_t*>(file.data() + cursor), remaining);
        const std::uint16_t pageSize = pageHeader.getBERawShort16();
        if (!pageHeader.isGood() || pageSize < 2u
            || static_cast<std::size_t>(pageSize) + 2u > remaining) {
            allPageCrcsValid = false;
            break;
        }

        dwgBuffer storedCrcReader(
            const_cast<std::uint8_t*>(file.data() + cursor + pageSize), 2u);
        const std::uint16_t storedCrc = storedCrcReader.getBERawShort16();
        dwgBuffer crcInput(
            const_cast<std::uint8_t*>(file.data() + cursor), pageSize);
        const std::uint16_t computedCrc =
            crcInput.crc8(0xC0C1u, 0, static_cast<std::int32_t>(pageSize));
        allPageCrcsValid = allPageCrcsValid && storedCrcReader.isGood()
            && crcInput.isGood() && storedCrc == computedCrc;

        if (pageSize == 2u) {
            sawTerminator = true;
            cursor += 4u;
            break;
        }

        ++dataPageCount;
        dwgBuffer entries(
            const_cast<std::uint8_t*>(file.data() + cursor), pageSize);
        const std::uint16_t parsedPageSize = entries.getBERawShort16();
        std::uint32_t lastHandle = 0u;
        std::int64_t lastLocation = 0;
        while (entries.isGood() && entries.getPosition() < pageSize) {
            const std::uint64_t before = entries.getPosition();
            const std::uint64_t handleDelta = entries.getUModularChar();
            const std::int64_t locationDelta = entries.getModularChar();
            const bool handleFits = handleDelta != 0u
                && handleDelta <= std::numeric_limits<std::uint32_t>::max()
                && lastHandle <= std::numeric_limits<std::uint32_t>::max()
                    - handleDelta;
            const bool locationFits =
                (locationDelta >= 0
                 && lastLocation <= std::numeric_limits<std::int64_t>::max()
                    - locationDelta)
                || (locationDelta < 0
                    && lastLocation >= std::numeric_limits<std::int64_t>::min()
                        - locationDelta);
            if (!entries.isGood() || entries.getPosition() <= before
                || !handleFits || !locationFits) {
                allPageEntriesValid = false;
                break;
            }

            lastHandle += static_cast<std::uint32_t>(handleDelta);
            lastLocation += locationDelta;
            const bool locationInFile = lastLocation >= 0
                && static_cast<std::uint64_t>(lastLocation) < handlesOffset;
            if (lastHandle == 0u || !locationInFile
                || !wireHandles.insert(lastHandle).second
                || !wireOffsets.insert(static_cast<std::uint32_t>(lastLocation)).second) {
                allPageEntriesValid = false;
                break;
            }
            wireMap.emplace_back(
                lastHandle, static_cast<std::uint32_t>(lastLocation));
        }
        allPageEntriesValid = allPageEntriesValid && entries.isGood()
            && parsedPageSize == pageSize
            && entries.getPosition() == pageSize;
        cursor += static_cast<std::size_t>(pageSize) + 2u;
    }

    t.expect(sawTerminator && cursor == handlesEnd && dataPageCount >= 2u,
             "final HANDLES section terminates exactly after multiple data pages (pages="
                 + std::to_string(dataPageCount) + ", terminator="
                 + std::to_string(sawTerminator) + ", remaining="
                 + std::to_string(handlesEnd - std::min(cursor, handlesEnd))
                 + ")");
    t.expect(allPageCrcsValid,
             "every finalized HANDLES data/terminator page has a valid big-endian CRC");
    t.expect(allPageEntriesValid && !wireMap.empty(),
             "final HANDLES pages decode unique nonzero handles and in-file offsets");

    const bool mapMatchesEmittedFrames = allPageEntriesValid
        && wireMap == expectedMap
        && wireHandles.size() == expectedMap.size()
        && wireOffsets.size() == expectedMap.size();
    t.expect(mapMatchesEmittedFrames,
             "decoded HANDLES pairs exactly match every emitted object-map frame receipt");

    std::unordered_map<std::uint32_t, std::uint32_t> expectedOffsets;
    expectedOffsets.reserve(expectedMap.size());
    bool emittedFrameReceiptsMatch = frameReceipts.size() == generatedViewportCount
        && viewportFrames.size() == generatedViewportCount;
    for (std::size_t index = 0; emittedFrameReceiptsMatch
         && index < frameReceipts.size(); ++index) {
        const DRW::DwgObjectFrameReceipt& receipt = frameReceipts[index];
        const auto& emitted = viewportFrames[index];
        emittedFrameReceiptsMatch = receipt.valid && receipt.generation != 0u
            && receipt.objectHandle == emitted.first
            && emitted.first == firstViewportHandle + index
            && expectedOffsets.emplace(emitted.first, emitted.second).second;
    }
    const bool receiptsCloseToMap = emittedFrameReceiptsMatch
        && std::all_of(viewportFrames.cbegin(), viewportFrames.cend(),
                       [&expectedOffsets](const auto& frame) {
                           const auto found = expectedOffsets.find(frame.first);
                           return found != expectedOffsets.end()
                               && found->second == frame.second;
                       });
    t.expect(receiptsCloseToMap,
             "each generated VIEWPORT receipt maps to its exact pre-finalization frame offset");

    std::vector<std::pair<std::uint32_t, std::uint32_t>> framesByOffset =
        expectedMap;
    std::sort(framesByOffset.begin(), framesByOffset.end(),
              [](const auto& left, const auto& right) {
                  return left.second < right.second;
              });
    bool allFramesBounded = true;
    std::uint32_t priorFrameEnd = 0u;
    std::unordered_map<std::uint32_t,
                       std::pair<std::size_t, std::size_t>> frameSegments;
    frameSegments.reserve(framesByOffset.size());
    for (const auto& entry : framesByOffset) {
        if (entry.second >= handlesOffset || entry.second < priorFrameEnd) {
            allFramesBounded = false;
            break;
        }
        dwgBuffer framePrefix(
            const_cast<std::uint8_t*>(file.data() + entry.second),
            handlesOffset - entry.second);
        const std::int32_t bodySizeSigned = framePrefix.getModularShort();
        const std::uint64_t prefixSize = framePrefix.getPosition();
        const std::uint64_t bytesBeforeHandles =
            static_cast<std::uint64_t>(handlesOffset - entry.second);
        if (!framePrefix.isGood() || bodySizeSigned <= 0
            || prefixSize == 0u
            || prefixSize > bytesBeforeHandles
            || bytesBeforeHandles - prefixSize < 2u
            || static_cast<std::uint64_t>(bodySizeSigned)
                > bytesBeforeHandles - prefixSize - 2u) {
            allFramesBounded = false;
            break;
        }
        const std::size_t bodyStart = static_cast<std::size_t>(entry.second)
            + static_cast<std::size_t>(prefixSize);
        const std::size_t checksumStart = static_cast<std::size_t>(entry.second)
            + static_cast<std::size_t>(prefixSize)
            + static_cast<std::size_t>(bodySizeSigned);
        dwgBuffer storedFrameCrcReader(
            const_cast<std::uint8_t*>(file.data() + checksumStart), 2u);
        const std::uint16_t storedFrameCrc =
            storedFrameCrcReader.getRawShort16();
        const std::size_t framePayloadSize = checksumStart - entry.second;
        dwgBuffer framePayload(
            const_cast<std::uint8_t*>(file.data() + entry.second),
            framePayloadSize);
        const std::uint16_t computedFrameCrc = framePayload.crc8(
            0xC0C1u, 0, static_cast<std::int32_t>(framePayloadSize));
        if (!storedFrameCrcReader.isGood() || !framePayload.isGood()
            || storedFrameCrc != computedFrameCrc) {
            allFramesBounded = false;
            break;
        }
        priorFrameEnd = static_cast<std::uint32_t>(checksumStart + 2u);
        frameSegments.emplace(
            entry.second,
            std::make_pair(bodyStart,
                           static_cast<std::size_t>(bodySizeSigned)));
    }
    t.expect(allFramesBounded,
             "every HANDLES offset lands on a nonoverlapping, CRC-valid object frame before the section");

    bool allReceiptTokensMatch = emittedFrameReceiptsMatch
        && receiptsCloseToMap;
    for (std::size_t index = 0; allReceiptTokensMatch
         && index < frameReceipts.size(); ++index) {
        const DRW::DwgObjectFrameReceipt& receipt = frameReceipts[index];
        const auto segment = frameSegments.find(viewportFrames[index].second);
        if (segment == frameSegments.end()
            || receipt.dataBitSize
                != static_cast<std::uint64_t>(segment->second.second) * 8u
            || receipt.handleBitSize != 0u
            || receipt.occurrences.empty()) {
            allReceiptTokensMatch = false;
            break;
        }

        std::size_t ownerTokenCount = 0u;
        for (const DRW::DwgObjectHandleOccurrence& occurrence
             : receipt.occurrences) {
            const bool bounded = occurrence.objectHandle
                    == receipt.objectHandle
                && occurrence.serializedSegment
                    == DRW::DwgObjectSerializedSegment::Data
                && occurrence.token.endBit >= occurrence.token.startBit
                && occurrence.token.endBit
                    <= static_cast<std::uint64_t>(segment->second.second) * 8u
                && occurrence.token.startBit
                    < static_cast<std::uint64_t>(segment->second.second) * 8u;
            if (!bounded) {
                allReceiptTokensMatch = false;
                break;
            }

            dwgBuffer tokenReader(
                const_cast<std::uint8_t*>(
                    file.data() + segment->second.first),
                segment->second.second);
            const bool positioned = tokenReader.setPosition(
                occurrence.token.startBit / 8u);
            tokenReader.setBitPos(static_cast<std::uint8_t>(
                occurrence.token.startBit & 7u));
            const dwgHandle decoded = positioned
                ? tokenReader.getHandle() : dwgHandle{};
            const std::uint64_t decodedEndBit =
                tokenReader.getPosition() * 8u + tokenReader.getBitPos();
            allReceiptTokensMatch = positioned && tokenReader.isGood()
                && decoded.code == occurrence.token.code
                && decoded.ref64 == occurrence.token.reference
                && decodedEndBit == occurrence.token.endBit;
            if (occurrence.token.code == 4u
                && occurrence.token.reference == ownerHandle)
                ++ownerTokenCount;
            if (!allReceiptTokensMatch)
                break;
        }
        allReceiptTokensMatch = allReceiptTokensMatch
            && ownerTokenCount == 1u;
    }
    t.expect(allReceiptTokensMatch,
             "each emitted VIEWPORT receipt's own/owner H tokens decode at its final frame offset");
}

void testFileLevelViewportHandlesSectionModern(TestContext& t) {
    constexpr std::size_t generatedViewportCount = 1200u;
    constexpr std::uint32_t firstViewportHandle = 0x1000u;
    constexpr std::uint32_t ownerHandle = DRW_Vport::kDwgControlHandle;

    std::stringbuf streamBuffer(std::ios::out | std::ios::binary);
    std::ofstream output;
    static_cast<std::ostream&>(output).rdbuf(&streamBuffer);
    DRW_Header header;
    header.addInt("$MEASUREMENT", 1, 70);
    ViewportObjectFrameWriter<dwgWriter24> writer(&output, &header);

    bool generated = writer.prepareDwgClassManifest()
        && writer.writeFileHeaderStub() && writer.writeDwgHeader()
        && writer.writeDwgClasses() && writer.writeDwgObjects();
    std::vector<std::pair<std::uint32_t, std::uint32_t>> viewportFrames;
    viewportFrames.reserve(generatedViewportCount);
    std::vector<DRW::DwgObjectFrameReceipt> frameReceipts;
    frameReceipts.reserve(generatedViewportCount);
    for (std::size_t index = 0; generated
         && index < generatedViewportCount; ++index) {
        const auto handle = firstViewportHandle
            + static_cast<std::uint32_t>(index);
        generated = writer.encodeViewport(handle, ownerHandle);
        DRW::DwgObjectFrameReceipt receipt;
        const auto& objectMap = writer.objectOffsets();
        generated = generated && writer.getLastDwgObjectFrame(receipt)
            && receipt.valid && receipt.objectHandle == handle
            && receipt.version == DRW::AC1024 && receipt.classNumber == 34u
            && !objectMap.empty() && objectMap.back().first == handle;
        if (generated) {
            frameReceipts.push_back(receipt);
            viewportFrames.push_back(objectMap.back());
        }
    }

    generated = generated && writer.emitDeferredBlockControl();
    std::uint32_t objectBaseOffset = 0u;
    generated = generated
        && writer.objectBaseOffsetForTest(objectBaseOffset);
    std::vector<std::pair<std::uint32_t, std::uint32_t>> expectedMap =
        writer.objectOffsets();
    bool offsetsAreSectionRelative = generated;
    for (auto& entry : expectedMap) {
        if (entry.second < objectBaseOffset) {
            offsetsAreSectionRelative = false;
            break;
        }
        entry.second -= objectBaseOffset;
    }
    std::sort(expectedMap.begin(), expectedMap.end());
    t.expect(offsetsAreSectionRelative,
             "generated AC1024 object map converts to bounded section-relative offsets");
    generated = generated && offsetsAreSectionRelative
        && writer.finalizeHeaderHandseed() && writer.writeDwgHandles()
        && writer.writeSecondHeader() && writer.finalize();
    t.expect(generated,
             "generated AC1024 file envelope finalizes many VIEWPORT frames and HANDLES pages");
    if (!generated)
        return;

    const std::string encoded = streamBuffer.str();
    std::vector<std::uint8_t> file(encoded.begin(), encoded.end());
    R2004SectionMapDecodeProbe reader(file);
    const bool sectionMapParsed = reader.readSections();
    t.expect(sectionMapParsed,
             "finalized AC1024 section map publishes data-section descriptors");
    if (!sectionMapParsed)
        return;
    const bool productionHandleMapParsed = reader.readDwgHandles();
    t.expect(productionHandleMapParsed,
             "AC1024 production reader accepts the finalized HANDLES section");

    const dwgSectionInfo* handlesSection =
        reader.findSection("AcDb:Handles");
    const dwgSectionInfo* objectsSection =
        reader.findSection("AcDb:AcDbObjects");
    t.expect(handlesSection != nullptr && objectsSection != nullptr,
             "finalized AC1024 map contains HANDLES and OBJECTS sections");
    if (handlesSection == nullptr || objectsSection == nullptr)
        return;

    std::unique_ptr<std::uint8_t[]> handlesData;
    std::unique_ptr<std::uint8_t[]> objectsData;
    std::uint64_t handlesSize = 0u;
    std::uint64_t objectsSize = 0u;
    const bool handlesDecoded =
        reader.decode(*handlesSection, handlesData, handlesSize);
    const bool objectsDecoded =
        reader.decode(*objectsSection, objectsData, objectsSize);
    t.expect(handlesDecoded && handlesData != nullptr && handlesSize > 0u,
             "AC1024 HANDLES logical bytes decode through the section-page path");
    t.expect(objectsDecoded && objectsData != nullptr && objectsSize > 0u,
             "AC1024 OBJECTS logical bytes decode through the section-page path");
    if (!handlesDecoded || handlesData == nullptr || handlesSize == 0u
        || !objectsDecoded || objectsData == nullptr || objectsSize == 0u
        || handlesSize > std::numeric_limits<std::size_t>::max()
        || objectsSize > std::numeric_limits<std::size_t>::max())
        return;

    std::vector<std::pair<std::uint32_t, std::uint32_t>> wireMap;
    std::unordered_set<std::uint32_t> wireHandles;
    std::unordered_set<std::uint32_t> wireOffsets;
    std::size_t cursor = 0u;
    const std::size_t handlesEnd = static_cast<std::size_t>(handlesSize);
    std::size_t dataPageCount = 0u;
    bool sawTerminator = false;
    bool allPageCrcsValid = true;
    bool allPageEntriesValid = true;
    while (cursor < handlesEnd && !sawTerminator) {
        const std::size_t remaining = handlesEnd - cursor;
        if (remaining < 4u) {
            allPageCrcsValid = false;
            break;
        }
        dwgBuffer pageHeader(handlesData.get() + cursor, remaining);
        const std::uint16_t pageSize = pageHeader.getBERawShort16();
        if (!pageHeader.isGood() || pageSize < 2u
            || static_cast<std::size_t>(pageSize) + 2u > remaining) {
            allPageCrcsValid = false;
            break;
        }
        dwgBuffer storedCrcReader(
            handlesData.get() + cursor + pageSize, 2u);
        dwgBuffer crcInput(handlesData.get() + cursor, pageSize);
        const std::uint16_t storedCrc = storedCrcReader.getBERawShort16();
        const std::uint16_t computedCrc =
            crcInput.crc8(0xC0C1u, 0, static_cast<std::int32_t>(pageSize));
        allPageCrcsValid = allPageCrcsValid && storedCrcReader.isGood()
            && crcInput.isGood() && storedCrc == computedCrc;
        if (pageSize == 2u) {
            sawTerminator = true;
            cursor += 4u;
            break;
        }

        ++dataPageCount;
        dwgBuffer entries(handlesData.get() + cursor, pageSize);
        const std::uint16_t parsedPageSize = entries.getBERawShort16();
        std::uint32_t lastHandle = 0u;
        std::int64_t lastLocation = 0;
        while (entries.isGood() && entries.getPosition() < pageSize) {
            const std::uint64_t before = entries.getPosition();
            const std::uint64_t handleDelta = entries.getUModularChar();
            const std::int64_t locationDelta = entries.getModularChar();
            const bool handleFits = handleDelta != 0u
                && handleDelta <= std::numeric_limits<std::uint32_t>::max()
                && lastHandle <= std::numeric_limits<std::uint32_t>::max()
                    - handleDelta;
            const bool locationFits =
                (locationDelta >= 0
                 && lastLocation <= std::numeric_limits<std::int64_t>::max()
                    - locationDelta)
                || (locationDelta < 0
                    && lastLocation >= std::numeric_limits<std::int64_t>::min()
                        - locationDelta);
            if (!entries.isGood() || entries.getPosition() <= before
                || !handleFits || !locationFits) {
                allPageEntriesValid = false;
                break;
            }
            lastHandle += static_cast<std::uint32_t>(handleDelta);
            lastLocation += locationDelta;
            if (lastHandle == 0u || lastLocation < 0
                || static_cast<std::uint64_t>(lastLocation) >= objectsSize
                || !wireHandles.insert(lastHandle).second
                || !wireOffsets.insert(
                    static_cast<std::uint32_t>(lastLocation)).second) {
                allPageEntriesValid = false;
                break;
            }
            wireMap.emplace_back(
                lastHandle, static_cast<std::uint32_t>(lastLocation));
        }
        allPageEntriesValid = allPageEntriesValid && entries.isGood()
            && parsedPageSize == pageSize
            && entries.getPosition() == pageSize;
        cursor += static_cast<std::size_t>(pageSize) + 2u;
    }
    t.expect(sawTerminator && cursor == handlesEnd && dataPageCount >= 2u,
             "AC1024 HANDLES logical section ends exactly after multiple CRC pages and a terminator");
    t.expect(allPageCrcsValid,
             "each decoded AC1024 HANDLES data/terminator page has a valid CRC");
    t.expect(allPageEntriesValid && wireMap == expectedMap
                 && wireHandles.size() == expectedMap.size()
                 && wireOffsets.size() == expectedMap.size(),
             "decoded AC1024 HANDLES pairs exactly match emitted section-relative object frames");
    std::vector<std::pair<std::uint32_t, std::uint32_t>> productionMap;
    productionMap.reserve(reader.ObjectMap.size());
    for (const auto& entry : reader.ObjectMap)
        productionMap.emplace_back(entry.first, entry.second.loc);
    std::sort(productionMap.begin(), productionMap.end());
    t.expect(productionHandleMapParsed && productionMap == wireMap,
             "AC1024 production HANDLES parser agrees with independent logical-section decoding");

    std::vector<std::pair<std::uint32_t, std::uint32_t>> framesByOffset =
        expectedMap;
    std::sort(framesByOffset.begin(), framesByOffset.end(),
              [](const auto& left, const auto& right) {
                  return left.second < right.second;
              });
    bool allFramesBounded = !framesByOffset.empty();
    std::string frameFailureReason = allFramesBounded
        ? std::string() : "empty emitted map";
    std::uint64_t priorFrameEnd = 0u;
    std::unordered_map<std::uint32_t, std::size_t> frameBodySizes;
    frameBodySizes.reserve(framesByOffset.size());
    for (const auto& entry : framesByOffset) {
        if (entry.second >= objectsSize || entry.second < priorFrameEnd) {
            allFramesBounded = false;
            frameFailureReason = "offset=" + std::to_string(entry.second)
                + " objectsSize=" + std::to_string(objectsSize)
                + " priorEnd=" + std::to_string(priorFrameEnd);
            break;
        }
        const std::uint64_t bytesRemaining = objectsSize - entry.second;
        dwgBuffer framePrefix(objectsData.get() + entry.second,
                              bytesRemaining);
        const std::int32_t bodySizeSigned = framePrefix.getModularShort();
        const std::uint64_t sizePrefix = framePrefix.getPosition();
        const std::uint64_t handleBits = framePrefix.getUModularChar();
        const std::uint64_t prefixSize = framePrefix.getPosition();
        const std::uint64_t handleBytes = (handleBits + 7u) / 8u;
        if (!framePrefix.isGood() || bodySizeSigned <= 0 || prefixSize == 0u
            || prefixSize > bytesRemaining
            || handleBits % 8u != 0u
            || handleBytes > static_cast<std::uint64_t>(bodySizeSigned)
            || bytesRemaining - prefixSize < 2u
            || static_cast<std::uint64_t>(bodySizeSigned)
                > bytesRemaining - prefixSize - 2u) {
            allFramesBounded = false;
            frameFailureReason = "invalid frame geometry at offset="
                + std::to_string(entry.second) + " body="
                + std::to_string(bodySizeSigned) + " prefix="
                + std::to_string(prefixSize) + " sizePrefix="
                + std::to_string(sizePrefix) + " handleBits="
                + std::to_string(handleBits) + " remaining="
                + std::to_string(bytesRemaining);
            break;
        }
        const std::uint64_t checksumOffset = entry.second + prefixSize
            + static_cast<std::uint64_t>(bodySizeSigned);
        dwgBuffer storedFrameCrcReader(
            objectsData.get() + checksumOffset, 2u);
        dwgBuffer framePayload(
            objectsData.get() + entry.second,
            prefixSize + static_cast<std::uint64_t>(bodySizeSigned));
        const std::uint16_t storedFrameCrc =
            storedFrameCrcReader.getRawShort16();
        const std::uint16_t computedFrameCrc = framePayload.crc8(
            0xC0C1u, 0,
            static_cast<std::int32_t>(prefixSize + bodySizeSigned));
        if (!storedFrameCrcReader.isGood() || !framePayload.isGood()
            || storedFrameCrc != computedFrameCrc) {
            allFramesBounded = false;
            frameFailureReason = "frame CRC mismatch at offset="
                + std::to_string(entry.second) + " stored="
                + std::to_string(storedFrameCrc) + " computed="
                + std::to_string(computedFrameCrc);
            break;
        }
        priorFrameEnd = checksumOffset + 2u;
        frameBodySizes.emplace(
            entry.second, static_cast<std::size_t>(bodySizeSigned - handleBytes));
    }
    t.expect(allFramesBounded,
             "every AC1024 HANDLES offset lands on a bounded, nonoverlapping, CRC-valid OBJECTS frame ("
                 + frameFailureReason + ")");

    bool receiptsCloseToFrames = frameReceipts.size() == generatedViewportCount
        && viewportFrames.size() == generatedViewportCount;
    std::string receiptFailureReason;
    for (std::size_t index = 0; receiptsCloseToFrames
         && index < frameReceipts.size(); ++index) {
        const DRW::DwgObjectFrameReceipt& receipt = frameReceipts[index];
        const auto& emitted = viewportFrames[index];
        const auto relativeOffset = emitted.second >= objectBaseOffset
            ? emitted.second - objectBaseOffset
            : std::numeric_limits<std::uint32_t>::max();
        const auto body = frameBodySizes.find(relativeOffset);
        receiptsCloseToFrames = receipt.valid && receipt.generation != 0u
            && receipt.objectHandle == emitted.first
            && emitted.first == firstViewportHandle + index
            && body != frameBodySizes.end()
            && receipt.dataBitSize
                == static_cast<std::uint64_t>(body->second) * 8u;
        if (!receiptsCloseToFrames) {
            receiptFailureReason = "receipt index=" + std::to_string(index)
                + " handle=" + std::to_string(emitted.first)
                + " offset=" + std::to_string(relativeOffset)
                + " dataBits=" + std::to_string(receipt.dataBitSize)
                + " frameBodyBytes="
                + (body == frameBodySizes.end()
                    ? std::string("missing")
                    : std::to_string(body->second));
            break;
        }
        const auto mapEntry = std::lower_bound(
            expectedMap.cbegin(), expectedMap.cend(),
            std::make_pair(emitted.first, relativeOffset));
        receiptsCloseToFrames = receiptsCloseToFrames
            && mapEntry != expectedMap.cend()
            && *mapEntry == std::make_pair(emitted.first, relativeOffset);
        if (!receiptsCloseToFrames)
            receiptFailureReason = "receipt map mismatch index="
                + std::to_string(index) + " handle="
                + std::to_string(emitted.first) + " offset="
                + std::to_string(relativeOffset);
    }
    t.expect(receiptsCloseToFrames,
             "each AC1024 VIEWPORT receipt closes to its finalized section-relative HANDLE entry and frame ("
                 + receiptFailureReason + ")");
}

template <typename WriterBase>
void testFileLevelVportHandlesSectionR2004(
    TestContext& t, DRW::Version version, const std::string& versionName) {
    constexpr std::size_t generatedVportCount = 1200u;
    std::stringbuf streamBuffer(std::ios::out | std::ios::binary);
    std::ofstream output;
    static_cast<std::ostream&>(output).rdbuf(&streamBuffer);
    DRW_Header header;
    header.addInt("$MEASUREMENT", 1, 70);
    ViewportObjectFrameWriter<WriterBase> writer(&output, &header);
    bool generated = writer.prepareDwgClassManifest()
        && writer.writeFileHeaderStub() && writer.writeDwgHeader()
        && writer.writeDwgClasses();
    std::vector<std::uint32_t> generatedVportHandles;
    generatedVportHandles.reserve(generatedVportCount);
    for (std::size_t index = 0; generated && index < generatedVportCount;
         ++index) {
        DRW_Vport vport;
        vport.name = "PAR144_" + versionName + "_VPORT_"
            + std::to_string(index);
        generated = writer.addVport(vport) && vport.handle != 0u;
        if (generated)
            generatedVportHandles.push_back(vport.handle);
    }
    generated = generated && writer.writeDwgObjects()
        && writer.emitDeferredBlockControl();
    std::vector<std::uint32_t> emittedTableRecordHandles;
    std::vector<std::uint32_t> emittedTableControlHandles;
    generated = generated
        && writer.getEmittedDwgTableRecordHandles(emittedTableRecordHandles)
        && writer.getEmittedDwgTableControlHandles(emittedTableControlHandles);
    std::uint32_t objectBaseOffset = 0u;
    generated = generated
        && writer.objectBaseOffsetForTest(objectBaseOffset);
    std::vector<std::pair<std::uint32_t, std::uint32_t>> expectedMap =
        writer.objectOffsets();
    bool offsetsAreSectionRelative = generated;
    for (auto& entry : expectedMap) {
        if (entry.second < objectBaseOffset) {
            offsetsAreSectionRelative = false;
            break;
        }
        entry.second -= objectBaseOffset;
    }
    std::sort(expectedMap.begin(), expectedMap.end());
    generated = generated && offsetsAreSectionRelative
        && writer.finalizeHeaderHandseed() && writer.writeDwgHandles()
        && writer.writeSecondHeader() && writer.finalize();
    t.expect(generated,
             "generated " + versionName
                 + " file envelope finalizes many VPORT records and HANDLES pages");
    if (!generated)
        return;

    const std::string encoded = streamBuffer.str();
    std::vector<std::uint8_t> file(encoded.begin(), encoded.end());
    R2004SectionMapDecodeProbe reader(file, version);
    const bool sectionMapParsed = reader.readSections();
    t.expect(sectionMapParsed,
             "finalized " + versionName
                 + " section map publishes data-section descriptors");
    if (!sectionMapParsed)
        return;
    const dwgSectionInfo* handlesSection =
        reader.findSection("AcDb:Handles");
    const dwgSectionInfo* objectsSection =
        reader.findSection("AcDb:AcDbObjects");
    t.expect(handlesSection != nullptr && objectsSection != nullptr,
             "finalized " + versionName
                 + " map contains HANDLES and OBJECTS sections");
    if (handlesSection == nullptr || objectsSection == nullptr)
        return;

    const bool productionHandleMapParsed = reader.readDwgHandles();
    t.expect(productionHandleMapParsed,
             versionName + " production reader accepts the finalized HANDLES section");
    std::vector<std::pair<std::uint32_t, std::uint32_t>> productionMap;
    productionMap.reserve(reader.ObjectMap.size());
    for (const auto& entry : reader.ObjectMap)
        productionMap.emplace_back(entry.first, entry.second.loc);
    std::sort(productionMap.begin(), productionMap.end());
    t.expect(productionHandleMapParsed && productionMap == expectedMap,
             versionName + " production HANDLES parser exactly matches emitted section-relative object-map entries");

    std::unique_ptr<std::uint8_t[]> objectsData;
    std::uint64_t objectsSize = 0u;
    const bool objectsDecoded =
        reader.decode(*objectsSection, objectsData, objectsSize);
    t.expect(objectsDecoded && objectsData != nullptr && objectsSize > 0u,
             versionName + " OBJECTS logical bytes decode through the production R2004 data-page path");
    if (!objectsDecoded || objectsData == nullptr || objectsSize == 0u
        || objectsSize > std::numeric_limits<std::size_t>::max())
        return;

    std::vector<std::pair<std::uint32_t, std::uint32_t>> framesByOffset =
        expectedMap;
    std::sort(framesByOffset.begin(), framesByOffset.end(),
              [](const auto& left, const auto& right) {
                  return left.second < right.second;
              });
    bool allFramesBounded = !framesByOffset.empty();
    std::uint64_t priorFrameEnd = 0u;
    std::unordered_map<std::uint32_t, std::size_t> frameBodySizes;
    frameBodySizes.reserve(framesByOffset.size());
    for (const auto& entry : framesByOffset) {
        if (entry.second >= objectsSize || entry.second < priorFrameEnd) {
            allFramesBounded = false;
            break;
        }
        const std::uint64_t bytesRemaining = objectsSize - entry.second;
        dwgBuffer framePrefix(objectsData.get() + entry.second,
                              bytesRemaining);
        const std::int32_t bodySize = framePrefix.getModularShort();
        const std::uint64_t handleBits = version >= DRW::AC1024
            ? framePrefix.getUModularChar() : 0u;
        const std::uint64_t prefixSize = framePrefix.getPosition();
        if (!framePrefix.isGood() || bodySize <= 0 || prefixSize == 0u
            || handleBits % 8u != 0u
            || handleBits / 8u > static_cast<std::uint64_t>(bodySize)
            || prefixSize > bytesRemaining
            || bytesRemaining - prefixSize < 2u
            || static_cast<std::uint64_t>(bodySize)
                > bytesRemaining - prefixSize - 2u) {
            allFramesBounded = false;
            break;
        }
        const std::uint64_t checksumOffset = entry.second + prefixSize
            + static_cast<std::uint64_t>(bodySize);
        dwgBuffer storedFrameCrcReader(
            objectsData.get() + checksumOffset, 2u);
        dwgBuffer framePayload(
            objectsData.get() + entry.second,
            prefixSize + static_cast<std::uint64_t>(bodySize));
        const std::uint16_t storedFrameCrc =
            storedFrameCrcReader.getRawShort16();
        const std::uint16_t computedFrameCrc = framePayload.crc8(
            0xC0C1u, 0,
            static_cast<std::int32_t>(prefixSize + bodySize));
        if (!storedFrameCrcReader.isGood() || !framePayload.isGood()
            || storedFrameCrc != computedFrameCrc) {
            allFramesBounded = false;
            break;
        }
        priorFrameEnd = checksumOffset + 2u;
        frameBodySizes.emplace(entry.second,
                               static_cast<std::size_t>(bodySize));
    }
    t.expect(allFramesBounded,
             "every " + versionName
                 + " HANDLES offset lands on a bounded, nonoverlapping, CRC-valid OBJECTS frame");

    const auto ledgerHandlesMapped = [&reader](
            const std::vector<std::uint32_t>& handles) {
        return std::all_of(handles.cbegin(), handles.cend(),
                           [&reader](std::uint32_t handle) {
                               return reader.ObjectMap.find(handle)
                                   != reader.ObjectMap.end();
                           });
    };
    const bool generatedRecordsClose =
        generatedVportHandles.size() == generatedVportCount
        && ledgerHandlesMapped(generatedVportHandles)
        && ledgerHandlesMapped(emittedTableRecordHandles)
        && ledgerHandlesMapped(emittedTableControlHandles)
        && std::all_of(generatedVportHandles.cbegin(),
                       generatedVportHandles.cend(),
                       [&reader, &frameBodySizes](std::uint32_t handle) {
                           const auto mapped = reader.ObjectMap.find(handle);
                           return mapped != reader.ObjectMap.end()
                               && frameBodySizes.find(mapped->second.loc)
                                   != frameBodySizes.end();
                       });
    t.expect(generatedRecordsClose,
             "every generated " + versionName
                 + " VPORT record and public table/control ledger handle closes to a CRC-valid frame");
}

void testFileLevelViewportHandlesSectionR21(TestContext& t) {
    constexpr std::size_t generatedViewportCount = 1200u;

    std::stringbuf streamBuffer(std::ios::out | std::ios::binary);
    std::ofstream output;
    static_cast<std::ostream&>(output).rdbuf(&streamBuffer);
    DRW_Header header;
    header.addInt("$MEASUREMENT", 1, 70);
    dwgWriter21 writer(&output, &header);
    bool generated = writer.prepareDwgClassManifest()
        && writer.writeFileHeaderStub() && writer.writeDwgHeader()
        && writer.writeDwgClasses();
    std::vector<std::uint32_t> generatedVportHandles;
    generatedVportHandles.reserve(generatedViewportCount);
    for (std::size_t index = 0; generated
         && index < generatedViewportCount; ++index) {
        DRW_Vport vport;
        vport.name = "PAR144_VPORT_" + std::to_string(index);
        generated = writer.addVport(vport) && vport.handle != 0u;
        if (generated)
            generatedVportHandles.push_back(vport.handle);
    }
    generated = generated && writer.writeDwgObjects()
        && writer.emitDeferredBlockControl();
    std::vector<std::uint32_t> emittedTableRecordHandles;
    std::vector<std::uint32_t> emittedTableControlHandles;
    generated = generated
        && writer.getEmittedDwgTableRecordHandles(emittedTableRecordHandles)
        && writer.getEmittedDwgTableControlHandles(emittedTableControlHandles)
        && writer.finalizeHeaderHandseed() && writer.writeDwgHandles()
        && writer.writeSecondHeader() && writer.finalize();
    t.expect(generated,
             "generated AC1021 file envelope finalizes many VPORT records and HANDLES pages");
    if (!generated)
        return;

    const std::string encoded = streamBuffer.str();
    std::vector<std::uint8_t> file(encoded.begin(), encoded.end());
    R21DataPageDecodeProbe reader(file);
    const bool sectionMapParsed = reader.readSections();
    t.expect(sectionMapParsed,
             "finalized AC1021 page maps publish data-section descriptors");
    if (!sectionMapParsed)
        return;
    const bool productionHandleMapParsed = reader.readDwgHandles();
    t.expect(productionHandleMapParsed,
             "AC1021 production reader accepts the finalized HANDLES section");
    const dwgSectionInfo* handlesSection =
        reader.findSection("AcDb:Handles");
    const dwgSectionInfo* objectsSection =
        reader.findSection("AcDb:AcDbObjects");
    t.expect(handlesSection != nullptr && objectsSection != nullptr,
             "finalized AC1021 map contains HANDLES and OBJECTS sections");
    if (handlesSection == nullptr || objectsSection == nullptr
        || handlesSection->size > std::numeric_limits<std::size_t>::max()
        || objectsSection->size > std::numeric_limits<std::size_t>::max())
        return;

    std::vector<std::uint8_t> handlesData(
        static_cast<std::size_t>(handlesSection->size), 0u);
    std::vector<std::uint8_t> objectsData(
        static_cast<std::size_t>(objectsSection->size), 0u);
    const bool handlesDecoded =
        reader.decode(*handlesSection, handlesData.data());
    const bool objectsDecoded =
        reader.decode(*objectsSection, objectsData.data());
    t.expect(handlesDecoded && objectsDecoded,
             "AC1021 HANDLES and OBJECTS pages decode through the production R2007 page path");
    if (!handlesDecoded || !objectsDecoded)
        return;

    std::vector<std::pair<std::uint32_t, std::uint32_t>> wireMap;
    std::unordered_set<std::uint32_t> wireHandles;
    std::unordered_set<std::uint32_t> wireOffsets;
    std::size_t cursor = 0u;
    std::size_t dataPageCount = 0u;
    bool sawTerminator = false;
    bool allPageCrcsValid = true;
    bool allPageEntriesValid = true;
    while (cursor < handlesData.size() && !sawTerminator) {
        const std::size_t remaining = handlesData.size() - cursor;
        if (remaining < 4u) {
            allPageCrcsValid = false;
            break;
        }
        dwgBuffer pageHeader(handlesData.data() + cursor, remaining);
        const std::uint16_t pageSize = pageHeader.getBERawShort16();
        if (!pageHeader.isGood() || pageSize < 2u
            || static_cast<std::size_t>(pageSize) + 2u > remaining) {
            allPageCrcsValid = false;
            break;
        }
        dwgBuffer storedCrcReader(handlesData.data() + cursor + pageSize, 2u);
        dwgBuffer crcInput(handlesData.data() + cursor, pageSize);
        const std::uint16_t storedCrc = storedCrcReader.getBERawShort16();
        const std::uint16_t computedCrc =
            crcInput.crc8(0xC0C1u, 0, static_cast<std::int32_t>(pageSize));
        allPageCrcsValid = allPageCrcsValid && storedCrcReader.isGood()
            && crcInput.isGood() && storedCrc == computedCrc;
        if (pageSize == 2u) {
            sawTerminator = true;
            cursor += 4u;
            break;
        }
        ++dataPageCount;
        dwgBuffer entries(handlesData.data() + cursor, pageSize);
        const std::uint16_t parsedPageSize = entries.getBERawShort16();
        std::uint32_t lastHandle = 0u;
        std::int64_t lastLocation = 0;
        while (entries.isGood() && entries.getPosition() < pageSize) {
            const std::uint64_t before = entries.getPosition();
            const std::uint64_t handleDelta = entries.getUModularChar();
            const std::int64_t locationDelta = entries.getModularChar();
            const bool handleFits = handleDelta != 0u
                && handleDelta <= std::numeric_limits<std::uint32_t>::max()
                && lastHandle <= std::numeric_limits<std::uint32_t>::max()
                    - handleDelta;
            const bool locationFits =
                (locationDelta >= 0
                 && lastLocation <= std::numeric_limits<std::int64_t>::max()
                    - locationDelta)
                || (locationDelta < 0
                    && lastLocation >= std::numeric_limits<std::int64_t>::min()
                        - locationDelta);
            if (!entries.isGood() || entries.getPosition() <= before
                || !handleFits || !locationFits) {
                allPageEntriesValid = false;
                break;
            }
            lastHandle += static_cast<std::uint32_t>(handleDelta);
            lastLocation += locationDelta;
            if (lastHandle == 0u || lastLocation < 0
                || static_cast<std::uint64_t>(lastLocation) >= objectsData.size()
                || !wireHandles.insert(lastHandle).second
                || !wireOffsets.insert(
                    static_cast<std::uint32_t>(lastLocation)).second) {
                allPageEntriesValid = false;
                break;
            }
            wireMap.emplace_back(
                lastHandle, static_cast<std::uint32_t>(lastLocation));
        }
        allPageEntriesValid = allPageEntriesValid && entries.isGood()
            && parsedPageSize == pageSize
            && entries.getPosition() == pageSize;
        cursor += static_cast<std::size_t>(pageSize) + 2u;
    }
    t.expect(sawTerminator && cursor == handlesData.size()
                 && dataPageCount >= 2u,
             "AC1021 HANDLES section ends exactly after multiple CRC pages and a terminator");
    t.expect(allPageCrcsValid,
             "each decoded AC1021 HANDLES data/terminator page has a valid CRC");
    const auto allLedgerHandlesMapped = [&wireHandles](
            const std::vector<std::uint32_t>& handles) {
        return std::all_of(handles.cbegin(), handles.cend(),
                           [&wireHandles](std::uint32_t handle) {
                               return wireHandles.find(handle)
                                   != wireHandles.end();
                           });
    };
    t.expect(allPageEntriesValid && !wireMap.empty()
                 && wireHandles.size() == wireMap.size()
                 && wireOffsets.size() == wireMap.size()
                 && allLedgerHandlesMapped(emittedTableRecordHandles)
                 && allLedgerHandlesMapped(emittedTableControlHandles)
                 && generatedVportHandles.size() == generatedViewportCount
                 && allLedgerHandlesMapped(generatedVportHandles),
             "decoded AC1021 HANDLES map uniquely resolves every emitted table/control and generated VPORT handle");
    std::vector<std::pair<std::uint32_t, std::uint32_t>> productionMap;
    productionMap.reserve(reader.ObjectMap.size());
    for (const auto& entry : reader.ObjectMap)
        productionMap.emplace_back(entry.first, entry.second.loc);
    std::sort(productionMap.begin(), productionMap.end());
    t.expect(productionHandleMapParsed && productionMap == wireMap,
             "AC1021 production HANDLES parser agrees with independent logical-section decoding");

    std::vector<std::pair<std::uint32_t, std::uint32_t>> framesByOffset =
        wireMap;
    std::sort(framesByOffset.begin(), framesByOffset.end(),
              [](const auto& left, const auto& right) {
                  return left.second < right.second;
              });
    bool allFramesBounded = !framesByOffset.empty();
    std::uint64_t priorFrameEnd = 0u;
    std::unordered_map<std::uint32_t, std::size_t> frameBodySizes;
    frameBodySizes.reserve(framesByOffset.size());
    for (const auto& entry : framesByOffset) {
        if (entry.second >= objectsData.size() || entry.second < priorFrameEnd) {
            allFramesBounded = false;
            break;
        }
        const std::uint64_t bytesRemaining = objectsData.size() - entry.second;
        dwgBuffer framePrefix(objectsData.data() + entry.second,
                              bytesRemaining);
        const std::int32_t bodySize = framePrefix.getModularShort();
        const std::uint64_t prefixSize = framePrefix.getPosition();
        if (!framePrefix.isGood() || bodySize <= 0 || prefixSize == 0u
            || prefixSize > bytesRemaining
            || bytesRemaining - prefixSize < 2u
            || static_cast<std::uint64_t>(bodySize)
                > bytesRemaining - prefixSize - 2u) {
            allFramesBounded = false;
            break;
        }
        const std::uint64_t checksumOffset = entry.second + prefixSize
            + static_cast<std::uint64_t>(bodySize);
        dwgBuffer storedFrameCrcReader(
            objectsData.data() + checksumOffset, 2u);
        dwgBuffer framePayload(
            objectsData.data() + entry.second,
            prefixSize + static_cast<std::uint64_t>(bodySize));
        const std::uint16_t storedFrameCrc =
            storedFrameCrcReader.getRawShort16();
        const std::uint16_t computedFrameCrc = framePayload.crc8(
            0xC0C1u, 0,
            static_cast<std::int32_t>(prefixSize + bodySize));
        if (!storedFrameCrcReader.isGood() || !framePayload.isGood()
            || storedFrameCrc != computedFrameCrc) {
            allFramesBounded = false;
            break;
        }
        priorFrameEnd = checksumOffset + 2u;
        frameBodySizes.emplace(entry.second,
                               static_cast<std::size_t>(bodySize));
    }
    t.expect(allFramesBounded,
             "every AC1021 HANDLES offset lands on a bounded, nonoverlapping, CRC-valid OBJECTS frame");

    const bool generatedVportsCloseToFrames =
        generatedVportHandles.size() == generatedViewportCount
        && std::all_of(generatedVportHandles.cbegin(),
                       generatedVportHandles.cend(),
                       [&wireMap, &frameBodySizes](std::uint32_t handle) {
                           const auto entry = std::find_if(
                               wireMap.cbegin(), wireMap.cend(),
                               [handle](const auto& candidate) {
                                   return candidate.first == handle;
                               });
                           return entry != wireMap.cend()
                               && frameBodySizes.find(entry->second)
                                   != frameBodySizes.end();
                       });
    t.expect(generatedVportsCloseToFrames,
             "each generated AC1021 VPORT table-record handle resolves to a finalized CRC-valid object frame");
}

void testModelerBlockRecordDispatch(TestContext& t) {
    constexpr std::uint32_t modelSpaceHandle = 0x400u;
    constexpr std::uint32_t liveBlockHandle = 0x500u;
    constexpr std::uint32_t unusedBlockHandle = 0x510u;
    constexpr std::uint32_t liveSolidHandle = 0x900u;
    constexpr std::uint32_t unusedSolidHandle = 0x901u;
    constexpr std::uint32_t insertHandle = 0x902u;
    std::ofstream output;
    DRW_Header header;
    ModelerDispatchFrameWriter liveWriter(&output, &header);
    ModelerDispatchFrameWriter unusedWriter(&output, &header);
    ModelerDispatchFrameWriter insertWriter(&output, &header);
    const bool generated =
        liveWriter.encodeEmptySolid(liveSolidHandle, liveBlockHandle)
        && unusedWriter.encodeEmptySolid(unusedSolidHandle, unusedBlockHandle)
        && insertWriter.encodeInsert(insertHandle, modelSpaceHandle,
                                     liveBlockHandle);
    t.expect(generated,
             "in-memory AC1027 writer frames two block-owned solids and one INSERT");
    if (!generated)
        return;

    std::vector<std::uint8_t> liveBytes = liveWriter.buffer();
    std::vector<std::uint8_t> unusedBytes = unusedWriter.buffer();
    std::vector<std::uint8_t> insertBytes = insertWriter.buffer();
    dwgRW parent("");
    auto makeBlock = [](std::uint32_t handle, const std::string& name,
                        std::uint32_t entityHandle) {
        auto block = std::make_unique<DRW_Block_Record>();
        block->handle = handle;
        block->name = name;
        DwgBlockOwnershipTestAccess::setEntityHandles(*block, {entityHandle});
        return block;
    };
    ModelerDispatchReader reader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(liveBytes.data(), liveBytes.size())),
        &parent);
    reader.setFormatVersion(DRW::AC1027);
    const auto registerBlock = [&reader](std::unique_ptr<DRW_Block_Record> value) {
        DRW_Block_Record* raw = value.get();
        reader.blockRecordmap.emplace(raw->handle, raw);
        (void)value.release(); // dwgReader owns and deletes blockRecordmap values.
    };
    registerBlock(makeBlock(modelSpaceHandle, "*Model_Space", insertHandle));
    registerBlock(makeBlock(liveBlockHandle, "LIVE_SOLID", liveSolidHandle));
    registerBlock(makeBlock(unusedBlockHandle, "UNUSED_SOLID", unusedSolidHandle));
    reader.ObjectMap.emplace(liveSolidHandle,
                             objHandle(38u, liveSolidHandle, 0u));
    reader.ObjectMap.emplace(unusedSolidHandle,
                             objHandle(38u, unusedSolidHandle, 0u));
    reader.ObjectMap.emplace(insertHandle, objHandle(7u, insertHandle, 0u));

    dwgBuffer liveStream(liveBytes.data(), liveBytes.size());
    dwgBuffer unusedStream(unusedBytes.data(), unusedBytes.size());
    dwgBuffer insertStream(insertBytes.data(), insertBytes.size());
    ModelerDispatchSink sink;
    sink.activeWalkOwner = modelSpaceHandle;
    const bool modelWalk = reader.walkBlockRecordEntities(
        reader.blockRecordmap.at(modelSpaceHandle), &insertStream, sink,
        modelSpaceHandle, modelSpaceHandle);
    sink.activeWalkOwner = liveBlockHandle;
    const bool liveWalk = reader.walkBlockRecordEntities(
        reader.blockRecordmap.at(liveBlockHandle), &liveStream, sink,
        liveBlockHandle, liveBlockHandle);
    sink.activeWalkOwner = unusedBlockHandle;
    const bool unusedWalk = reader.walkBlockRecordEntities(
        reader.blockRecordmap.at(unusedBlockHandle), &unusedStream, sink,
        unusedBlockHandle, unusedBlockHandle);
    t.expect(modelWalk && liveWalk && unusedWalk && sink.inserts.size() == 1u
                 && sink.modeler.size() == 2u
                 && sink.modelerMoveCallbacks == 2u
                 && sink.rawEntityCallbacks.size() == 2u,
             "AC1027 BLOCK_RECORD walks move-publish one INSERT and two stored solids");

    bool graphMatches = sink.inserts.size() == 1u && sink.modeler.size() == 2u;
    if (graphMatches) {
        const ModelerDispatchSink::OwnedInsert& insert = sink.inserts.front();
        graphMatches = insert.walkOwner == modelSpaceHandle
            && insert.entity.handle == insertHandle
            && insert.entity.parentHandle == modelSpaceHandle
            && insert.entity.blockRecH.ref == liveBlockHandle
            && insert.entity.name == "LIVE_SOLID";
        std::size_t reachableSolids = 0;
        bool liveSolidFound = false;
        bool unusedSolidStored = false;
        for (const ModelerDispatchSink::OwnedSolid& solid : sink.modeler) {
            const bool isLive = solid.entity.handle == liveSolidHandle
                && solid.walkOwner == liveBlockHandle
                && solid.entity.parentHandle == liveBlockHandle
                && solid.entity.eType == DRW::E3DSOLID
                && solid.entity.m_isEmpty
                && solid.entity.m_dwgAcisPayload.empty();
            const bool isUnused = solid.entity.handle == unusedSolidHandle
                && solid.walkOwner == unusedBlockHandle
                && solid.entity.parentHandle == unusedBlockHandle
                && solid.entity.eType == DRW::E3DSOLID
                && solid.entity.m_isEmpty
                && solid.entity.m_dwgAcisPayload.empty();
            graphMatches = graphMatches && (isLive || isUnused);
            liveSolidFound = liveSolidFound || isLive;
            unusedSolidStored = unusedSolidStored || isUnused;
            if (insert.entity.blockRecH.ref == solid.walkOwner)
                ++reachableSolids;
        }
        graphMatches = graphMatches && liveSolidFound && unusedSolidStored
            && reachableSolids == 1u;
    }
    t.expect(graphMatches,
             "INSERT reaches its named block's solid; a second block's solid stays stored but outside this reference");
}

void testBlockRecordXrefIndexRetention(TestContext& t) {
    struct Case {
        DRW::Version version;
        std::uint16_t encodedIndex;
        std::int32_t expectedIndex;
        bool overlaid;
        bool xrefUnloaded;
    };
    const std::array<Case, 3> cases{{
        {DRW::AC1015, 0u, -1, false, false},
        {DRW::AC1018, 42u, 41, true, false},
        {DRW::AC1018, 32769u, 32768, false, true},
    }};

    for (const Case& testCase : cases) {
        std::vector<std::uint8_t> bytes = makeGeneratedR2000XrefBlockRecord(
            testCase.version, testCase.encodedIndex, testCase.overlaid,
            testCase.xrefUnloaded);
        dwgBuffer buffer(bytes.data(), bytes.size());
        DRW_Block_Record record;
        const bool parsed = DwgBlockOwnershipTestAccess::parse(
            record, testCase.version, buffer);
        t.expect(parsed,
                 "generated R2000/R2004 XREF BLOCK_RECORD parses completely");
        if (!parsed)
            continue;

        const int expectedFlags = 64 | 16 | 4
            | (testCase.overlaid ? 8 : 0);
        t.expect(record.xrefIndex == testCase.expectedIndex,
                 "BLOCK_RECORD retains zero-based unsigned XREF index");
        t.expect((record.flags & (64 | 16 | 4 | 8 | 32)) == expectedFlags
                     && record.xrefUnloaded == testCase.xrefUnloaded,
                 "BLOCK_RECORD keeps DWG unload state separate from DXF resolved flags");
        t.expect(record.name == "XREF_TEST"
                     && record.xrefPath == "refs/linked.dwg"
                     && record.parentHandle == 0x10u
                     && record.xrefBlockHandle.ref == 0u
                     && record.layoutHandle == 0u,
                 "XREF parse keeps path and typed references without resolving external files");
    }
}

void testLegacyLayoutBlockChains(TestContext& t) {
    // Exercise the legacy firstEH..lastEH walker directly. BLOCK_RECORD and
    // layout objects are intentionally supplied as generated in-memory data;
    // this is a walker regression, not a whole-file table/object decoder test.
    constexpr std::uint32_t defaultRecordHandle = 0x500u;
    constexpr std::uint32_t sheetARecordHandle = 0x510u;
    constexpr std::uint32_t sheetBRecordHandle = 0x520u;
    constexpr std::uint32_t defaultLayoutHandle = 0x700u;
    constexpr std::uint32_t sheetALayoutHandle = 0x701u;
    constexpr std::uint32_t sheetBLayoutHandle = 0x702u;
    constexpr std::uint32_t sheetAFirst = 0xA00u;
    constexpr std::uint32_t sheetASecond = 0xA01u;
    constexpr std::uint32_t sheetBFirst = 0xB00u;
    constexpr std::uint32_t sentinelHandle = 0xB01u;

    std::ofstream output;
    DRW_Header header;
    LegacyLayoutFrameWriter legacyWriter(&output, &header);
    const bool legacyGenerated =
        legacyWriter.encodeOwnerlessLine(sheetAFirst, DRW::PaperSpace)
        && legacyWriter.encodeOwnerlessLine(sheetASecond, DRW::PaperSpace)
        && legacyWriter.encodeOwnerlessLine(sheetBFirst, DRW::PaperSpace)
        && legacyWriter.encodeOwnerlessLine(sentinelHandle,
                                            DRW::PaperSpace);
    t.expect(legacyGenerated,
             "from-scratch AC1015 paper-space entity frames are generated");
    if (!legacyGenerated)
        return;

    const std::vector<std::uint8_t> legacyBytes =
        legacyWriter.m_buf.data();
    dwgRW legacyParent("");
    ModelerDispatchReader legacyReader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(legacyBytes.data(),
                                                 legacyBytes.size())),
        &legacyParent);
    legacyReader.setFormatVersion(DRW::AC1015);
    const auto mapFrames = [](ModelerDispatchReader& reader,
                              const std::vector<std::pair<std::uint32_t,
                                                          std::uint32_t>>& frames) {
        for (const auto& frame : frames) {
            if (!reader.ObjectMap.emplace(
                    frame.first,
                    objHandle(0u, frame.first, frame.second)).second)
                return false;
        }
        return true;
    };
    t.expect(mapFrames(legacyReader, legacyWriter.objectOffsets()),
             "AC1015 reader receives the exact generated frame offsets");

    DRW_Block_Record defaultRecord;
    defaultRecord.handle = defaultRecordHandle;
    defaultRecord.name = "*PAPER_SPACE";
    defaultRecord.layoutHandle = defaultLayoutHandle;
    DwgBlockOwnershipTestAccess::setLegacyEntityChain(defaultRecord, 0u, 0u);
    DRW_Block_Record sheetARecord;
    sheetARecord.handle = sheetARecordHandle;
    sheetARecord.name = "*PAPER_SPACE0";
    sheetARecord.layoutHandle = sheetALayoutHandle;
    DwgBlockOwnershipTestAccess::setLegacyEntityChain(
        sheetARecord, sheetAFirst, sheetASecond);
    DRW_Block_Record sheetBRecord;
    sheetBRecord.handle = sheetBRecordHandle;
    sheetBRecord.name = "*PAPER_SPACE1";
    sheetBRecord.layoutHandle = sheetBLayoutHandle;
    DwgBlockOwnershipTestAccess::setLegacyEntityChain(
        sheetBRecord, sheetBFirst, sheetBFirst);

    LayoutChainDispatchSink legacySink;
    dwgBuffer legacyStream(legacyBytes.data(), legacyBytes.size());
    legacySink.beginLayout("Default", defaultLayoutHandle,
                           defaultRecordHandle);
    const bool defaultWalked = legacyReader.walkBlockRecordEntities(
        &defaultRecord, &legacyStream, legacySink, DRW::NoHandle,
        defaultRecordHandle);
    legacySink.beginLayout("Sheet A", sheetALayoutHandle,
                           sheetARecordHandle);
    const bool sheetAWalked = legacyReader.walkBlockRecordEntities(
        &sheetARecord, &legacyStream, legacySink, DRW::NoHandle,
        sheetARecordHandle);
    legacySink.beginLayout("Sheet B", sheetBLayoutHandle,
                           sheetBRecordHandle);
    const bool sheetBWalked = legacyReader.walkBlockRecordEntities(
        &sheetBRecord, &legacyStream, legacySink, DRW::NoHandle,
        sheetBRecordHandle);

    bool legacyMembershipMatches = legacySink.lines.size() == 3u;
    const std::array<std::string, 3> expectedLegacyLayouts{{
        "Sheet A", "Sheet A", "Sheet B"}};
    const std::array<std::uint32_t, 3> expectedLegacyLayoutHandles{{
        sheetALayoutHandle, sheetALayoutHandle, sheetBLayoutHandle}};
    const std::array<std::uint32_t, 3> expectedLegacyRecords{{
        sheetARecordHandle, sheetARecordHandle, sheetBRecordHandle}};
    const std::array<std::uint32_t, 3> expectedLegacyEntities{{
        sheetAFirst, sheetASecond, sheetBFirst}};
    for (std::size_t index = 0;
         legacyMembershipMatches && index < legacySink.lines.size();
         ++index) {
        const LayoutChainDispatchSink::LineMembership& line =
            legacySink.lines[index];
        legacyMembershipMatches =
            line.layoutName == expectedLegacyLayouts[index]
            && line.layoutHandle == expectedLegacyLayoutHandles[index]
            && line.blockRecordHandle == expectedLegacyRecords[index]
            && line.entityHandle == expectedLegacyEntities[index]
            && line.entityOwnerHandle == DRW::NoHandle
            && line.space == DRW::PaperSpace;
    }
    t.expect(defaultWalked && sheetAWalked && sheetBWalked
                 && legacyMembershipMatches,
             "AC1015 first/last chains preserve exact identities under each supplied paper-space record");
    t.expect(legacyReader.ObjectMap.find(sentinelHandle)
                 != legacyReader.ObjectMap.end(),
             "AC1015 singleton chain stops before its following paper-space sentinel");

    constexpr std::uint32_t cycleFirst = 0xC10u;
    constexpr std::uint32_t cycleSecond = 0xC11u;
    constexpr std::uint32_t cycleSentinel = 0xC12u;
    LegacyLayoutFrameWriter cycleWriter(&output, &header);
    const bool cycleGenerated =
        cycleWriter.encodeOwnerlessLineWithExplicitLinks(
            cycleFirst, DRW::PaperSpace, cycleSecond, cycleSecond)
        && cycleWriter.encodeOwnerlessLineWithExplicitLinks(
            cycleSecond, DRW::PaperSpace, cycleFirst, cycleFirst)
        && cycleWriter.encodeOwnerlessLine(cycleSentinel, DRW::PaperSpace);
    t.expect(cycleGenerated,
             "from-scratch AC1015 frames encode explicit relative links that form a two-entity cycle");
    if (cycleGenerated) {
        const std::vector<std::uint8_t> cycleBytes = cycleWriter.m_buf.data();
        dwgRW cycleParent("");
        ModelerDispatchReader cycleReader(
            std::unique_ptr<dwgBuffer>(new dwgBuffer(cycleBytes.data(),
                                                     cycleBytes.size())),
            &cycleParent);
        cycleReader.setFormatVersion(DRW::AC1015);
        t.expect(mapFrames(cycleReader, cycleWriter.objectOffsets()),
                 "AC1015 cycle reader receives generated entity offsets");

        DRW_Block_Record cycleRecord;
        cycleRecord.handle = 0x531u;
        cycleRecord.name = "*PAPER_SPACE3";
        DwgBlockOwnershipTestAccess::setLegacyEntityChain(
            cycleRecord, cycleFirst, cycleSentinel);
        LayoutChainDispatchSink cycleSink;
        cycleSink.beginLayout("Cyclic sheet", 0x704u, cycleRecord.handle);
        dwgBuffer cycleStream(cycleBytes.data(), cycleBytes.size());
        const bool cycleWalked = cycleReader.walkBlockRecordEntities(
            &cycleRecord, &cycleStream, cycleSink, DRW::NoHandle,
            cycleRecord.handle);
        const bool exactCyclePrefix = cycleSink.lines.size() == 2u
            && cycleSink.lines[0].entityHandle == cycleFirst
            && cycleSink.lines[1].entityHandle == cycleSecond
            && cycleSink.lines[0].layoutName == "Cyclic sheet"
            && cycleSink.lines[1].layoutName == "Cyclic sheet"
            && cycleReader.m_entityParseFailures == 1u;
        t.expect(!cycleWalked && exactCyclePrefix,
                 "AC1015 explicit-link cycle terminates after each reachable entity and reports one bounded chain failure");
        t.expect(cycleReader.ObjectMap.find(cycleSentinel)
                     != cycleReader.ObjectMap.end(),
                 "AC1015 explicit-link cycle leaves the following paper-space sentinel untouched");
    }

    constexpr std::uint32_t integratedDefaultRecord = 0x540u;
    constexpr std::uint32_t integratedSheetARecord = 0x541u;
    constexpr std::uint32_t integratedSheetBRecord = 0x542u;
    constexpr std::uint32_t integratedDefaultLayout = 0x710u;
    constexpr std::uint32_t integratedSheetALayout = 0x711u;
    constexpr std::uint32_t integratedSheetBLayout = 0x712u;
    constexpr std::uint32_t integratedDefaultBlock = 0x550u;
    constexpr std::uint32_t integratedSheetABlock = 0x551u;
    constexpr std::uint32_t integratedSheetBBlock = 0x552u;
    constexpr std::uint32_t integratedDefaultEnd = 0x560u;
    constexpr std::uint32_t integratedSheetAEnd = 0x561u;
    constexpr std::uint32_t integratedSheetBEnd = 0x562u;
    constexpr std::uint32_t integratedSheetAFirst = 0xD00u;
    constexpr std::uint32_t integratedSheetASecond = 0xD01u;
    constexpr std::uint32_t integratedSheetBFirst = 0xE00u;
    constexpr std::uint32_t integratedSentinel = 0xE01u;
    LegacyLayoutFrameWriter integratedWriter(&output, &header);
    const bool integratedGenerated =
        integratedWriter.encodeBlockDelimiter(integratedDefaultBlock,
                                              "*PAPER_SPACE", false)
        && integratedWriter.encodeBlockDelimiter(integratedDefaultEnd, "", true)
        && integratedWriter.encodeBlockDelimiter(integratedSheetABlock,
                                                 "*PAPER_SPACE0", false)
        && integratedWriter.encodeBlockDelimiter(integratedSheetAEnd, "", true)
        && integratedWriter.encodeBlockDelimiter(integratedSheetBBlock,
                                                 "*PAPER_SPACE1", false)
        && integratedWriter.encodeBlockDelimiter(integratedSheetBEnd, "", true)
        && integratedWriter.encodeOwnerlessLine(integratedSheetAFirst,
                                                DRW::PaperSpace)
        && integratedWriter.encodeOwnerlessLine(integratedSheetASecond,
                                                DRW::PaperSpace)
        && integratedWriter.encodeOwnerlessLine(integratedSheetBFirst,
                                                DRW::PaperSpace)
        && integratedWriter.encodeOwnerlessLine(integratedSentinel,
                                                DRW::PaperSpace);
    t.expect(integratedGenerated,
             "from-scratch AC1015 BLOCK/ENDBLK and paper-space frames generate for readDwgBlocks integration");
    if (integratedGenerated) {
        std::vector<std::uint8_t> integratedBytes =
            integratedWriter.m_buf.data();
        dwgRW integratedParent("");
        ModelerDispatchReader integratedReader(
            std::unique_ptr<dwgBuffer>(new dwgBuffer(
                integratedBytes.data(), integratedBytes.size())),
            &integratedParent);
        integratedReader.setFormatVersion(DRW::AC1015);
        t.expect(mapFrames(integratedReader,
                           integratedWriter.objectOffsets()),
                 "AC1015 BLOCK-scope reader receives each generated frame offset");

        const auto addIntegratedRecord = [&integratedReader](
                std::uint32_t handle, const std::string& name,
                std::uint32_t layoutHandle, std::uint32_t blockHandle,
                std::uint32_t endHandle, std::uint32_t firstHandle,
                std::uint32_t lastHandle) {
            auto record = std::make_unique<DRW_Block_Record>();
            record->handle = handle;
            record->name = name;
            record->layoutHandle = layoutHandle;
            DwgBlockOwnershipTestAccess::setDelimiters(
                *record, blockHandle, endHandle);
            DwgBlockOwnershipTestAccess::setLegacyEntityChain(
                *record, firstHandle, lastHandle);
            DRW_Block_Record* raw = record.get();
            if (!integratedReader.blockRecordmap.emplace(handle, raw).second)
                return false;
            (void)record.release();
            return true;
        };
        const bool recordsAdded =
            addIntegratedRecord(integratedDefaultRecord, "*PAPER_SPACE",
                                integratedDefaultLayout,
                                integratedDefaultBlock,
                                integratedDefaultEnd, 0u, 0u)
            && addIntegratedRecord(integratedSheetARecord, "*PAPER_SPACE0",
                                   integratedSheetALayout,
                                   integratedSheetABlock,
                                   integratedSheetAEnd,
                                   integratedSheetAFirst,
                                   integratedSheetASecond)
            && addIntegratedRecord(integratedSheetBRecord, "*PAPER_SPACE1",
                                   integratedSheetBLayout,
                                   integratedSheetBBlock,
                                   integratedSheetBEnd,
                                   integratedSheetBFirst,
                                   integratedSheetBFirst);
        t.expect(recordsAdded,
                 "AC1015 BLOCK_RECORD/layout metadata maps each generated delimiter pair and first/last chain");
        if (recordsAdded) {
            LayoutChainDispatchSink integratedSink;
            const bool integratedWalked = integratedReader.readBlocks(
                integratedBytes, integratedSink);
            std::sort(integratedSink.blockRecords.begin(),
                      integratedSink.blockRecords.end());
            std::sort(integratedSink.blocks.begin(), integratedSink.blocks.end());
            std::sort(integratedSink.lines.begin(), integratedSink.lines.end(),
                      [](const auto& lhs, const auto& rhs) {
                          return std::tie(lhs.blockRecordHandle,
                                          lhs.entityHandle)
                              < std::tie(rhs.blockRecordHandle,
                                          rhs.entityHandle);
                      });
            const std::vector<std::uint32_t> expectedRecords{
                integratedDefaultRecord, integratedSheetARecord,
                integratedSheetBRecord};
            const std::vector<std::string> expectedBlocks{
                "*PAPER_SPACE", "*PAPER_SPACE0", "*PAPER_SPACE1"};
            const std::array<std::uint32_t, 3> expectedIntegratedRecords{{
                integratedSheetARecord, integratedSheetARecord,
                integratedSheetBRecord}};
            const std::array<std::uint32_t, 3> expectedIntegratedLayouts{{
                integratedSheetALayout, integratedSheetALayout,
                integratedSheetBLayout}};
            const std::array<std::uint32_t, 3> expectedIntegratedEntities{{
                integratedSheetAFirst, integratedSheetASecond,
                integratedSheetBFirst}};
            bool integratedManifestMatches =
                integratedSink.lines.size() == 3u;
            for (std::size_t index = 0;
                 integratedManifestMatches && index < 3u; ++index) {
                const auto& line = integratedSink.lines[index];
                integratedManifestMatches =
                    line.blockRecordHandle == expectedIntegratedRecords[index]
                    && line.layoutHandle == expectedIntegratedLayouts[index]
                    && line.layoutName ==
                           (index < 2u ? "*PAPER_SPACE0" : "*PAPER_SPACE1")
                    && line.entityHandle == expectedIntegratedEntities[index]
                    && line.entityOwnerHandle == DRW::NoHandle
                    && line.space == DRW::PaperSpace;
            }
            t.expect(integratedWalked,
                     "readDwgBlocks accepts generated AC1015 paper-space scopes");
            t.expect(integratedSink.blockRecords == expectedRecords
                         && integratedSink.blocks == expectedBlocks
                         && integratedSink.endBlocks == 3u,
                     "readDwgBlocks emits each generated BLOCK_RECORD/BLOCK/ENDBLK scope");
            t.expect(integratedManifestMatches,
                     "readDwgBlocks preserves exact AC1015 line/layout identity within each paper-space scope");
            t.expect(integratedReader.ObjectMap.find(integratedSentinel)
                         != integratedReader.ObjectMap.end(),
                     "AC1015 readDwgBlocks integration leaves the unowned following sentinel untouched");
        }
    }

    ProxyEntityDispatchFrameWriter<dwgWriter27> modernWriter(&output,
                                                              &header);
    const bool modernGenerated =
        modernWriter.encodeLine(sheetAFirst, sheetARecordHandle)
        && modernWriter.encodeLine(sheetASecond, sheetARecordHandle)
        && modernWriter.encodeLine(sheetBFirst, sheetBRecordHandle)
        && modernWriter.encodeLine(sentinelHandle, DRW::NoHandle);
    t.expect(modernGenerated,
             "from-scratch AC1027 explicit-owner control frames are generated");
    if (!modernGenerated)
        return;

    const std::vector<std::uint8_t> modernBytes = modernWriter.m_buf.data();
    dwgRW modernParent("");
    ModelerDispatchReader modernReader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(modernBytes.data(),
                                                 modernBytes.size())),
        &modernParent);
    modernReader.setFormatVersion(DRW::AC1027);
    t.expect(mapFrames(modernReader, modernWriter.objectOffsets()),
             "AC1027 control reader receives the exact generated frame offsets");

    DRW_Block_Record modernDefaultRecord;
    modernDefaultRecord.handle = defaultRecordHandle;
    modernDefaultRecord.name = "*PAPER_SPACE";
    modernDefaultRecord.layoutHandle = defaultLayoutHandle;
    DRW_Block_Record modernSheetARecord;
    modernSheetARecord.handle = sheetARecordHandle;
    modernSheetARecord.name = "*PAPER_SPACE0";
    modernSheetARecord.layoutHandle = sheetALayoutHandle;
    DwgBlockOwnershipTestAccess::setEntityHandles(
        modernSheetARecord, {sheetAFirst, sheetASecond});
    DRW_Block_Record modernSheetBRecord;
    modernSheetBRecord.handle = sheetBRecordHandle;
    modernSheetBRecord.name = "*PAPER_SPACE1";
    modernSheetBRecord.layoutHandle = sheetBLayoutHandle;
    DwgBlockOwnershipTestAccess::setEntityHandles(
        modernSheetBRecord, {sheetBFirst});

    LayoutChainDispatchSink modernSink;
    dwgBuffer modernStream(modernBytes.data(), modernBytes.size());
    modernSink.beginLayout("Default", defaultLayoutHandle,
                           defaultRecordHandle);
    const bool modernDefaultWalked = modernReader.walkBlockRecordEntities(
        &modernDefaultRecord, &modernStream, modernSink, DRW::NoHandle,
        defaultRecordHandle);
    modernSink.beginLayout("Sheet A", sheetALayoutHandle,
                           sheetARecordHandle);
    const bool modernSheetAWalked = modernReader.walkBlockRecordEntities(
        &modernSheetARecord, &modernStream, modernSink,
        sheetARecordHandle, sheetARecordHandle);
    modernSink.beginLayout("Sheet B", sheetBLayoutHandle,
                           sheetBRecordHandle);
    const bool modernSheetBWalked = modernReader.walkBlockRecordEntities(
        &modernSheetBRecord, &modernStream, modernSink,
        sheetBRecordHandle, sheetBRecordHandle);

    bool modernMembershipMatches = modernSink.lines.size() == 3u;
    for (std::size_t index = 0;
         modernMembershipMatches && index < modernSink.lines.size();
         ++index) {
        const LayoutChainDispatchSink::LineMembership& line =
            modernSink.lines[index];
        modernMembershipMatches =
            line.layoutName == expectedLegacyLayouts[index]
            && line.layoutHandle == expectedLegacyLayoutHandles[index]
            && line.blockRecordHandle == expectedLegacyRecords[index]
            && line.entityHandle == expectedLegacyEntities[index]
            && line.entityOwnerHandle == expectedLegacyRecords[index];
    }
    t.expect(modernDefaultWalked && modernSheetAWalked && modernSheetBWalked
                 && modernMembershipMatches,
             "AC1027 explicit-owner control normalizes to the same layout/entity manifest");
    t.expect(modernReader.ObjectMap.find(sentinelHandle)
                 != modernReader.ObjectMap.end(),
             "AC1027 explicit-owner walk leaves the following unowned sentinel untouched");

    LegacyLayoutFrameWriter missingWriter(&output, &header);
    constexpr std::uint32_t missingFirst = 0xC00u;
    constexpr std::uint32_t missingLast = 0xC02u;
    const bool missingGenerated =
        missingWriter.encodeOwnerlessLine(missingFirst, DRW::PaperSpace)
        && missingWriter.encodeOwnerlessLine(missingLast, DRW::PaperSpace);
    t.expect(missingGenerated,
             "from-scratch AC1015 frames generate a bounded missing-middle chain");
    if (!missingGenerated)
        return;

    const std::vector<std::uint8_t> missingBytes = missingWriter.m_buf.data();
    dwgRW missingParent("");
    ModelerDispatchReader missingReader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(missingBytes.data(),
                                                 missingBytes.size())),
        &missingParent);
    missingReader.setFormatVersion(DRW::AC1015);
    for (const auto& frame : missingWriter.objectOffsets()) {
        if (frame.first == missingFirst || frame.first == missingLast) {
            missingReader.ObjectMap.emplace(
                frame.first, objHandle(0u, frame.first, frame.second));
        }
    }
    DRW_Block_Record missingRecord;
    missingRecord.handle = 0x530u;
    missingRecord.name = "*PAPER_SPACE2";
    DwgBlockOwnershipTestAccess::setLegacyEntityChain(
        missingRecord, missingFirst, missingLast);
    LayoutChainDispatchSink missingSink;
    missingSink.beginLayout("Sheet with gap", 0x703u,
                            missingRecord.handle);
    dwgBuffer missingStream(missingBytes.data(), missingBytes.size());
    const bool missingWalked = missingReader.walkBlockRecordEntities(
        &missingRecord, &missingStream, missingSink, DRW::NoHandle,
        missingRecord.handle);
    t.expect(missingWalked && missingSink.lines.size() == 1u
                 && missingSink.lines.front().entityHandle == missingFirst
                 && missingReader.ObjectMap.find(missingLast)
                        != missingReader.ObjectMap.end(),
             "AC1015 missing-middle chain stops at the gap without rehoming its later sibling");
}

void testModelerBlockWalkRejectsBadOwnership(TestContext& t) {
    constexpr std::uint32_t blockHandle = 0x600u;
    constexpr std::uint32_t wrongOwnerHandle = 0x601u;
    constexpr std::uint32_t solidHandle = 0xA00u;
    constexpr std::uint32_t wrongOwnerSolidHandle = 0xA01u;
    constexpr std::uint32_t missingSolidHandle = 0xA02u;
    std::ofstream output;
    DRW_Header header;
    ModelerDispatchFrameWriter validWriter(&output, &header);
    ModelerDispatchFrameWriter wrongOwnerWriter(&output, &header);
    const bool generated = validWriter.encodeEmptySolid(solidHandle, blockHandle)
        && wrongOwnerWriter.encodeEmptySolid(wrongOwnerSolidHandle,
                                             wrongOwnerHandle);
    t.expect(generated, "in-memory AC1027 ownership-failure frames are generated");
    if (!generated)
        return;

    struct WalkResult {
        bool walked;
        std::size_t callbacks;
    };
    const auto runWalk = [](const std::vector<std::uint8_t>& bytes,
                            std::uint32_t mappedHandle,
                            std::uint32_t blockHandle,
                            std::vector<std::uint32_t> ownership,
                            bool seedObject) {
        std::vector<std::uint8_t> sourceBytes = bytes;
        dwgRW parent("");
        auto blockOwner = std::make_unique<DRW_Block_Record>();
        DRW_Block_Record* block = blockOwner.get();
        block->handle = blockHandle;
        block->name = "BAD_OWNERSHIP_TEST";
        DwgBlockOwnershipTestAccess::setEntityHandles(*block,
                                                     std::move(ownership));
        ModelerDispatchReader reader(
            std::unique_ptr<dwgBuffer>(new dwgBuffer(
                sourceBytes.data(), sourceBytes.size())),
            &parent);
        reader.setFormatVersion(DRW::AC1027);
        reader.blockRecordmap.emplace(blockHandle, block);
        (void)blockOwner.release(); // dwgReader owns blockRecordmap values.
        if (seedObject) {
            reader.ObjectMap.emplace(mappedHandle,
                                     objHandle(38u, mappedHandle, 0u));
        }
        dwgBuffer objectStream(sourceBytes.data(), sourceBytes.size());
        ModelerDispatchSink sink;
        const bool walked = reader.walkBlockRecordEntities(
            block, &objectStream, sink, blockHandle, blockHandle);
        return WalkResult{walked, sink.modeler.size()};
    };

    const WalkResult duplicate = runWalk(
        validWriter.buffer(), solidHandle, blockHandle,
        {solidHandle, solidHandle}, true);
    const WalkResult missing = runWalk(
        validWriter.buffer(), missingSolidHandle, blockHandle,
        {missingSolidHandle}, false);
    const WalkResult wrongOwner = runWalk(
        wrongOwnerWriter.buffer(), wrongOwnerSolidHandle, blockHandle,
        {wrongOwnerSolidHandle}, true);
    t.expect(!duplicate.walked && duplicate.callbacks == 0u,
             "duplicate BLOCK_RECORD entity handle is rejected before callback");
    t.expect(missing.walked && missing.callbacks == 0u,
             "missing BLOCK_RECORD entity frame is skipped without failing the block walk");
    t.expect(!wrongOwner.walked && wrongOwner.callbacks == 0u,
             "entity whose encoded owner differs from BLOCK_RECORD is quarantined");
}

void testSpaceBlockDelimiterOwner(TestContext& t) {
    constexpr std::uint32_t blockRecordHandle = 0x680u;
    constexpr std::uint32_t blockHandle = 0x681u;
    constexpr std::uint32_t endBlockHandle = 0x682u;
    constexpr const char* blockName = "*PAPER_SPACE0";
    std::ofstream output;
    DRW_Header header;
    ModelerDispatchFrameWriter writer(&output, &header);
    const bool generated = writer.encodeBlockDelimiter(
            blockHandle, blockRecordHandle, blockName, false)
        && writer.encodeBlockDelimiter(
            endBlockHandle, blockRecordHandle, blockName, true);
    t.expect(generated,
             "generated AC1027 paper-space delimiters encode their BLOCK_RECORD owner");
    if (!generated)
        return;

    std::vector<std::uint8_t> bytes = writer.buffer();
    dwgRW parent("");
    ModelerDispatchReader reader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(bytes.data(), bytes.size())),
        &parent);
    reader.setFormatVersion(DRW::AC1027);
    auto record = std::make_unique<DRW_Block_Record>();
    record->handle = blockRecordHandle;
    record->name = "*PAPER_SPACE";
    DwgBlockOwnershipTestAccess::setDelimiters(
        *record, blockHandle, endBlockHandle);
    DRW_Block_Record* const recordPointer = record.get();
    const bool recordAdded = reader.blockRecordmap.emplace(
        blockRecordHandle, recordPointer).second;
    t.expect(recordAdded,
             "generated paper-space BLOCK_RECORD metadata is installed once");
    if (!recordAdded)
        return;
    (void)record.release(); // dwgReader owns blockRecordmap values.

    for (const auto& frame : writer.objectOffsets()) {
        const std::uint32_t type = frame.first == blockHandle ? 4u : 5u;
        reader.ObjectMap.emplace(frame.first,
                                 objHandle(type, frame.first, frame.second));
    }
    BlockPreflightSink sink;
    const bool complete = reader.readBlocks(bytes, sink);
    t.expect(complete,
             "readDwgBlocks accepts a paper-space delimiter owned by its BLOCK_RECORD");
    t.expect(sink.blockRecords == std::vector<std::uint32_t>{blockRecordHandle}
                 && sink.blocks == std::vector<std::string>{blockName}
                 && sink.endBlocks == 1u,
             "owner-bearing paper-space delimiters publish one complete scope");
}

void testMixedMissingBlockOwnership(TestContext& t) {
    std::ofstream output;
    DRW_Header header;
    for (std::uint32_t missingPosition = 0u; missingPosition < 3u;
         ++missingPosition) {
        const std::uint32_t lowerOwner = 0x630u + missingPosition;
        const std::uint32_t lowerBase = 0xA30u + missingPosition * 0x10u;
        std::vector<std::uint32_t> lowerHandles;
        std::vector<std::uint32_t> lowerValidHandles;
        ModelerDispatchFrameWriter lowerWriter(&output, &header);
        bool lowerGenerated = true;
        for (std::uint32_t position = 0u; position < 3u; ++position) {
            const std::uint32_t handle = lowerBase + position;
            lowerHandles.push_back(handle);
            if (position == missingPosition)
                continue;
            lowerValidHandles.push_back(handle);
            lowerGenerated = lowerWriter.encodeEmptySolid(handle, lowerOwner)
                && lowerGenerated;
        }
        t.expect(lowerGenerated,
                 "generated lower walker frames cover missing position "
                     + std::to_string(missingPosition));
        if (!lowerGenerated)
            continue;

        auto lowerBytes = lowerWriter.buffer();
        dwgRW lowerParent("");
        auto lowerBlockOwner = std::make_unique<DRW_Block_Record>();
        DRW_Block_Record* lowerBlock = lowerBlockOwner.get();
        lowerBlock->handle = lowerOwner;
        lowerBlock->name = "LOWER_MIXED_OWNER";
        DwgBlockOwnershipTestAccess::setEntityHandles(*lowerBlock,
                                                      lowerHandles);
        ModelerDispatchReader lowerReader(
            std::unique_ptr<dwgBuffer>(new dwgBuffer(lowerBytes.data(),
                                                     lowerBytes.size())),
            &lowerParent);
        lowerReader.setFormatVersion(DRW::AC1027);
        lowerReader.blockRecordmap.emplace(lowerOwner, lowerBlock);
        (void)lowerBlockOwner.release(); // reader owns block-record values.
        bool lowerFramesMapped = true;
        for (const std::uint32_t handle : lowerValidHandles) {
            const auto found = std::find_if(
                lowerWriter.objectOffsets().cbegin(),
                lowerWriter.objectOffsets().cend(),
                [handle](const auto& entry) { return entry.first == handle; });
            if (found == lowerWriter.objectOffsets().cend()) {
                lowerFramesMapped = false;
                continue;
            }
            lowerReader.ObjectMap.emplace(
                handle, objHandle(38u, handle, found->second));
        }
        t.expect(lowerFramesMapped,
                 "lower walker receives exact offsets for the valid siblings");
        if (!lowerFramesMapped)
            continue;
        dwgBuffer lowerStream(lowerBytes.data(), lowerBytes.size());
        ModelerDispatchSink lowerSink;
        lowerSink.activeWalkOwner = lowerOwner;
        const bool lowerWalked = lowerReader.walkBlockRecordEntities(
            lowerBlock, &lowerStream, lowerSink, lowerOwner, lowerOwner);
        const std::uint32_t lowerMissingHandle =
            lowerHandles[missingPosition];
        const auto lowerMissingDiagnostic = std::find_if(
            lowerReader.entityFailures().cbegin(),
            lowerReader.entityFailures().cend(),
            [lowerOwner, lowerMissingHandle](const auto& diagnostic) {
                return diagnostic.handle == lowerMissingHandle
                    && diagnostic.blockRecordHandle == lowerOwner
                    && diagnostic.phase
                        == dwgReader::DwgEntityFailurePhase::BlockFinalize;
            });
        bool lowerOrderCorrect = lowerWalked
            && lowerSink.modeler.size() == lowerValidHandles.size();
        for (std::size_t index = 0u;
             lowerOrderCorrect && index < lowerValidHandles.size(); ++index) {
            lowerOrderCorrect = lowerSink.modeler[index].entity.handle
                    == lowerValidHandles[index]
                && lowerSink.modeler[index].walkOwner == lowerOwner;
        }
        t.expect(lowerOrderCorrect,
                 "lower walker skips the missing position "
                     + std::to_string(missingPosition)
                     + " and reports valid siblings in source order");
        t.expect(lowerMissingDiagnostic != lowerReader.entityFailures().cend(),
                 "lower walker records the exact missing handle and owner at position "
                     + std::to_string(missingPosition));

        const std::uint32_t badRecordHandle = 0x640u + missingPosition * 0x10u;
        const std::uint32_t badBlockHandle = badRecordHandle + 1u;
        const std::uint32_t badEndHandle = badRecordHandle + 2u;
        const std::uint32_t badBase = 0xA80u + missingPosition * 0x10u;
        const std::uint32_t badMissingSolid = badBase + missingPosition;
        const std::uint32_t goodRecordHandle = badRecordHandle + 3u;
        const std::uint32_t goodBlockHandle = badRecordHandle + 4u;
        const std::uint32_t goodEndHandle = badRecordHandle + 5u;
        const std::uint32_t goodSolid = 0xB80u + missingPosition;
        const std::string badName = "BAD_MIXED_OWNER_"
            + std::to_string(missingPosition);
        const std::string goodName = "GOOD_AFTER_BAD_"
            + std::to_string(missingPosition);
        ModelerDispatchFrameWriter blockWriter(&output, &header);
        bool blockFramesGenerated = blockWriter.encodeBlockDelimiter(
            badBlockHandle, badRecordHandle, badName, false);
        std::vector<std::uint32_t> badHandles;
        for (std::uint32_t position = 0u; position < 3u; ++position) {
            const std::uint32_t handle = badBase + position;
            badHandles.push_back(handle);
            if (position != missingPosition) {
                blockFramesGenerated = blockWriter.encodeEmptySolid(
                    handle, badRecordHandle) && blockFramesGenerated;
            }
        }
        blockFramesGenerated = blockWriter.encodeBlockDelimiter(
            badEndHandle, badRecordHandle, badName, true)
            && blockFramesGenerated;
        blockFramesGenerated = blockWriter.encodeBlockDelimiter(
            goodBlockHandle, goodRecordHandle, goodName, false)
            && blockFramesGenerated;
        blockFramesGenerated = blockWriter.encodeEmptySolid(
            goodSolid, goodRecordHandle) && blockFramesGenerated;
        blockFramesGenerated = blockWriter.encodeBlockDelimiter(
            goodEndHandle, goodRecordHandle, goodName, true)
            && blockFramesGenerated;
        t.expect(blockFramesGenerated,
                 "generated strict preflight frames cover missing position "
                     + std::to_string(missingPosition));
        if (!blockFramesGenerated)
            continue;

        auto blockBytes = blockWriter.buffer();
        dwgRW blockParent("");
        ModelerDispatchReader blockReader(
            std::unique_ptr<dwgBuffer>(new dwgBuffer(blockBytes.data(),
                                                     blockBytes.size())),
            &blockParent);
        blockReader.setFormatVersion(DRW::AC1027);
        const auto addRecord = [&blockReader](
                                  std::uint32_t handle,
                                  const std::string& name,
                                  std::uint32_t block,
                                  std::uint32_t endBlock,
                                  std::vector<std::uint32_t> entities) {
            auto record = std::make_unique<DRW_Block_Record>();
            record->handle = handle;
            record->name = name;
            DwgBlockOwnershipTestAccess::setDelimiters(*record, block,
                                                       endBlock);
            DwgBlockOwnershipTestAccess::setEntityHandles(*record,
                                                          std::move(entities));
            DRW_Block_Record* raw = record.get();
            blockReader.blockRecordmap.emplace(handle, raw);
            (void)record.release();
        };
        addRecord(badRecordHandle, badName, badBlockHandle, badEndHandle,
                  std::move(badHandles));
        addRecord(goodRecordHandle, goodName, goodBlockHandle, goodEndHandle,
                  {goodSolid});
        for (const auto& frame : blockWriter.objectOffsets()) {
            const bool isBlockDelimiter = frame.first == badBlockHandle
                || frame.first == goodBlockHandle;
            const bool isEndDelimiter = frame.first == badEndHandle
                || frame.first == goodEndHandle;
            const std::uint32_t type = isBlockDelimiter ? 4u
                : isEndDelimiter ? 5u : 38u;
            blockReader.ObjectMap.emplace(
                frame.first, objHandle(type, frame.first, frame.second));
        }
        BlockPreflightSink blockSink;
        const bool complete = blockReader.readBlocks(blockBytes, blockSink);
        const auto missingDiagnostic = std::find_if(
            blockReader.entityFailures().cbegin(),
            blockReader.entityFailures().cend(),
            [badRecordHandle, badMissingSolid](const auto& diagnostic) {
                return diagnostic.handle == badMissingSolid
                    && diagnostic.blockRecordHandle == badRecordHandle
                    && diagnostic.phase
                        == dwgReader::DwgEntityFailurePhase::BlockFinalize;
            });
        t.expect(!complete
                     && missingDiagnostic != blockReader.entityFailures().cend(),
                 "strict preflight diagnoses the exact missing handle at position "
                     + std::to_string(missingPosition));
        t.expect(blockSink.blockRecords
                         == std::vector<std::uint32_t>{goodRecordHandle}
                     && blockSink.blocks == std::vector<std::string>{goodName}
                     && blockSink.endBlocks == 1u
                     && blockSink.modeler.size() == 1u
                     && blockSink.modeler.front().entity.handle == goodSolid,
                 "strict preflight quarantines the malformed owner and publishes the next valid owner at position "
                     + std::to_string(missingPosition));
    }
}

void testBoundedEedApplicationChunks(TestContext& t) {
    const auto appendChunk = [](dwgBufferW& output, std::uint32_t appHandle,
                                const std::vector<std::uint8_t>& data) {
        if (data.empty() || data.size() >
                static_cast<std::size_t>(
                    (std::numeric_limits<std::uint16_t>::max)()))
            return false;
        output.putBitShort(static_cast<std::uint16_t>(data.size()));
        dwgHandle app;
        app.code = 5u;
        app.size = 2u;
        app.ref = appHandle;
        app.ref64 = appHandle;
        output.putHandle(app);
        output.putBytes(data.data(), data.size());
        return output.isGood();
    };
    const auto bitPosition = [](const dwgBuffer& input) {
        return input.getPosition() * 8u + input.getBitPos();
    };
    const std::vector<std::uint8_t> firstData{2u, 0u, 70u, 0x34u, 0x12u};
    const std::vector<std::uint8_t> secondData{4u, 3u, 0xAAu, 0xBBu, 0xCCu};
    constexpr std::uint32_t objectSentinel = 0x5A17C0DEu;

    dwgBufferW validWriter;
    const bool validGenerated = appendChunk(validWriter, 0x123u, firstData)
        && appendChunk(validWriter, 0x456u, secondData);
    validWriter.putBitShort(0u); // terminates the application-chunk list
    const std::uint64_t validSentinelBit = validWriter.bitCount();
    validWriter.putRawLong32(objectSentinel);
    t.expect(validGenerated && validWriter.isGood(),
             "source-authored EED vector writes two bounded application chunks");
    if (!validGenerated || !validWriter.isGood())
        return;
    auto validBytes = validWriter.data();
    dwgBuffer validInput(validBytes.data(), validBytes.size());
    std::vector<DwgEedChunk> chunks;
    const bool parsed = readDwgEed(DRW::AC1027, validInput, chunks,
                                   validWriter.bitCount());
    bool validValues = parsed && chunks.size() == 2u
        && chunks[0].appHandle == 0x123u && chunks[0].items.size() == 2u
        && chunks[0].items[0].code() == 1002
        && chunks[0].items[0].content.s != nullptr
        && *chunks[0].items[0].content.s == "{"
        && chunks[0].items[1].code() == 1070
        && chunks[0].items[1].i_val() == 0x1234
        && chunks[1].appHandle == 0x456u && chunks[1].items.size() == 1u
        && chunks[1].items[0].code() == 1004
        && chunks[1].items[0].binary() != nullptr
        && *chunks[1].items[0].binary()
            == std::vector<std::uint8_t>({0xAAu, 0xBBu, 0xCCu});
    t.expect(validValues,
             "bounded EED parser preserves exact application order and typed item values");
    t.expect(parsed && bitPosition(validInput) == validSentinelBit
                 && validInput.getRawLong32()
                        == static_cast<std::int32_t>(objectSentinel),
             "valid EED termination leaves the following object sentinel untouched");

    const std::vector<std::uint8_t> malformedData{4u, 4u, 0x7Fu};
    const std::vector<std::uint8_t> followingData{70u, 0x78u, 0x56u};
    dwgBufferW malformedWriter;
    const bool malformedGenerated =
        appendChunk(malformedWriter, 0x123u, firstData)
        && appendChunk(malformedWriter, 0x456u, malformedData);
    const std::uint64_t followingChunkBit = malformedWriter.bitCount();
    const bool followingGenerated = appendChunk(
        malformedWriter, 0x789u, followingData);
    malformedWriter.putBitShort(0u);
    const std::uint64_t malformedSentinelBit = malformedWriter.bitCount();
    malformedWriter.putRawLong32(objectSentinel);
    t.expect(malformedGenerated && followingGenerated
                 && malformedWriter.isGood(),
             "malformed EED vector retains a valid following application and object sentinel");
    if (!malformedGenerated || !followingGenerated || !malformedWriter.isGood())
        return;

    auto malformedBytes = malformedWriter.data();
    dwgBuffer malformedInput(malformedBytes.data(), malformedBytes.size());
    DwgEedChunk prior;
    prior.appHandle = 0x999u;
    prior.items.emplace_back(1070, 77);
    std::vector<DwgEedChunk> unchanged{prior};
    const bool rejected = !readDwgEed(
        DRW::AC1027, malformedInput, unchanged, malformedWriter.bitCount());
    t.expect(rejected && unchanged.size() == 1u
                 && unchanged[0].appHandle == 0x999u
                 && unchanged[0].items.size() == 1u
                 && unchanged[0].items[0].i_val() == 77
                 && bitPosition(malformedInput) == followingChunkBit,
             "overlong variable item is refused within its app span without partial EED publication or consuming the next chunk");

    dwgBuffer followingInput(malformedBytes.data(), malformedBytes.size());
    const bool bytePositioned =
        followingInput.setPosition(followingChunkBit / 8u);
    if (bytePositioned) {
        followingInput.setBitPos(
            static_cast<std::uint8_t>(followingChunkBit & 7u));
    }
    const bool positioned = bytePositioned && followingInput.isGood()
        && bitPosition(followingInput) == followingChunkBit;
    std::vector<DwgEedChunk> followingChunks;
    const bool followingParsed = positioned && readDwgEed(
        DRW::AC1027, followingInput, followingChunks,
        malformedWriter.bitCount());
    const bool followingValue = followingParsed
        && followingChunks.size() == 1u
        && followingChunks[0].appHandle == 0x789u
        && followingChunks[0].items.size() == 1u
        && followingChunks[0].items[0].code() == 1070
        && followingChunks[0].items[0].i_val() == 0x5678;
    t.expect(followingValue
                 && bitPosition(followingInput) == malformedSentinelBit
                 && followingInput.getRawLong32()
                        == static_cast<std::int32_t>(objectSentinel),
             "the untouched next EED chunk and following object sentinel remain independently readable after refusal");
}

void testMalformedEedEntityDoesNotBlockFollowingEntity(TestContext& t) {
    constexpr std::uint32_t badRecordHandle = 0x660u;
    constexpr std::uint32_t badBlockHandle = 0x661u;
    constexpr std::uint32_t badEndBlockHandle = 0x662u;
    constexpr std::uint32_t malformedLineHandle = 0xA60u;
    constexpr std::uint32_t goodRecordHandle = 0x670u;
    constexpr std::uint32_t goodBlockHandle = 0x671u;
    constexpr std::uint32_t goodEndBlockHandle = 0x672u;
    constexpr std::uint32_t followingLineHandle = 0xA61u;
    std::ofstream output;
    DRW_Header header;
    ModelerDispatchFrameWriter writer(&output, &header);
    const bool generated =
        writer.encodeBlockDelimiter(badBlockHandle, badRecordHandle,
                                    "EED_MALFORMED", false)
        && writer.encodeLineWithEed(malformedLineHandle, badRecordHandle,
                                    true)
        && writer.encodeBlockDelimiter(badEndBlockHandle, badRecordHandle,
                                       "EED_MALFORMED", true)
        && writer.encodeBlockDelimiter(goodBlockHandle, goodRecordHandle,
                                       "EED_RECOVERY", false)
        && writer.encodeLineWithEed(followingLineHandle, goodRecordHandle,
                                    false)
        && writer.encodeBlockDelimiter(goodEndBlockHandle, goodRecordHandle,
                                       "EED_RECOVERY", true);
    t.expect(generated,
             "in-memory AC1027 writer frames a malformed-EED block and a separate following valid block");
    if (!generated)
        return;

    auto bytes = writer.buffer();
    dwgRW parent("");
    ModelerDispatchReader reader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(bytes.data(), bytes.size())),
        &parent);
    reader.setFormatVersion(DRW::AC1027);
    const auto addRecord = [&reader](std::uint32_t handle,
                                     const std::string& name,
                                     std::uint32_t block,
                                     std::uint32_t endBlock,
                                     std::uint32_t entity) {
        auto record = std::make_unique<DRW_Block_Record>();
        record->handle = handle;
        record->name = name;
        DwgBlockOwnershipTestAccess::setDelimiters(
            *record, block, endBlock);
        DwgBlockOwnershipTestAccess::setEntityHandles(*record, {entity});
        reader.blockRecordmap.emplace(handle, record.get());
        (void)record.release();
    };
    addRecord(badRecordHandle, "EED_MALFORMED", badBlockHandle,
              badEndBlockHandle, malformedLineHandle);
    addRecord(goodRecordHandle, "EED_RECOVERY", goodBlockHandle,
              goodEndBlockHandle, followingLineHandle);
    for (const auto& frame : writer.objectOffsets()) {
        const std::uint32_t type =
            frame.first == badBlockHandle || frame.first == goodBlockHandle
                ? 4u
            : frame.first == badEndBlockHandle ||
                      frame.first == goodEndBlockHandle
                ? 5u
                : 19u;
        reader.ObjectMap.emplace(
            frame.first, objHandle(type, frame.first, frame.second));
    }

    BlockPreflightSink sink;
    const bool read = reader.readBlocks(bytes, sink);
    const auto malformedDiagnostic = std::find_if(
        reader.entityFailures().cbegin(), reader.entityFailures().cend(),
        [malformedLineHandle, badRecordHandle](const auto& diagnostic) {
            return diagnostic.handle == malformedLineHandle
                && diagnostic.blockRecordHandle == badRecordHandle
                && diagnostic.phase == dwgReader::DwgEntityFailurePhase::TypedBody;
        });
    t.expect(!read && malformedDiagnostic != reader.entityFailures().cend(),
             "bounded EED failure is attributed to the malformed block without confusing it with the separate valid block");
    t.expect(sink.blockRecords == std::vector<std::uint32_t>{goodRecordHandle}
                 && sink.blocks == std::vector<std::string>{"EED_RECOVERY"}
                 && sink.endBlocks == 1u
                 && sink.lines == std::vector<std::uint32_t>{followingLineHandle},
             "malformed entity publishes no partial LINE callback and the separate following block is still processed");
}

void testModelerInsertTargetResolution(TestContext& t) {
    constexpr std::uint32_t modelSpaceHandle = 0x700u;
    constexpr std::uint32_t insertHandle = 0xB00u;
    constexpr std::uint32_t resolvedTargetHandle = 0x710u;
    constexpr std::uint32_t missingTargetHandle = 0x7FFu;

    struct InsertWalkResult {
        bool walked;
        std::size_t callbacks;
        std::uint32_t targetHandle;
        std::string targetName;
    };
    const auto runWalk = [=](std::uint32_t targetHandle, bool registerTarget) {
        std::ofstream output;
        DRW_Header header;
        ModelerDispatchFrameWriter writer(&output, &header);
        const bool generated = writer.encodeInsert(insertHandle,
                                                   modelSpaceHandle,
                                                   targetHandle);
        if (!generated)
            return InsertWalkResult{false, 0u, DRW::NoHandle, std::string()};

        std::vector<std::uint8_t> sourceBytes = writer.buffer();
        dwgRW parent("");
        auto modelSpaceOwner = std::make_unique<DRW_Block_Record>();
        DRW_Block_Record* modelSpace = modelSpaceOwner.get();
        modelSpace->handle = modelSpaceHandle;
        modelSpace->name = "*Model_Space";
        DwgBlockOwnershipTestAccess::setEntityHandles(*modelSpace,
                                                     {insertHandle});
        ModelerDispatchReader reader(
            std::unique_ptr<dwgBuffer>(new dwgBuffer(
                sourceBytes.data(), sourceBytes.size())),
            &parent);
        reader.setFormatVersion(DRW::AC1027);
        reader.blockRecordmap.emplace(modelSpaceHandle, modelSpace);
        (void)modelSpaceOwner.release(); // reader owns block-record values.
        if (registerTarget) {
            auto targetOwner = std::make_unique<DRW_Block_Record>();
            targetOwner->handle = resolvedTargetHandle;
            targetOwner->name = "RESOLVED_TARGET";
            reader.blockRecordmap.emplace(resolvedTargetHandle,
                                           targetOwner.get());
            (void)targetOwner.release();
        }
        reader.ObjectMap.emplace(insertHandle,
                                 objHandle(7u, insertHandle, 0u));

        dwgBuffer objectStream(sourceBytes.data(), sourceBytes.size());
        ModelerDispatchSink sink;
        sink.activeWalkOwner = modelSpaceHandle;
        const bool walked = reader.walkBlockRecordEntities(
            modelSpace, &objectStream, sink, modelSpaceHandle,
            modelSpaceHandle);
        if (sink.inserts.empty())
            return InsertWalkResult{walked, 0u, DRW::NoHandle, std::string()};
        const DRW_Insert& insert = sink.inserts.front().entity;
        return InsertWalkResult{walked, sink.inserts.size(),
                                insert.blockRecH.ref, insert.name};
    };

    const InsertWalkResult resolved = runWalk(resolvedTargetHandle, true);
    const InsertWalkResult missing = runWalk(missingTargetHandle, false);
    t.expect(resolved.walked && resolved.callbacks == 1u
                 && resolved.targetHandle == resolvedTargetHandle
                 && resolved.targetName == "RESOLVED_TARGET",
             "INSERT target handle resolves to the matching registered block name");
    t.expect(missing.walked && missing.callbacks == 1u
                 && missing.targetHandle == missingTargetHandle
                 && missing.targetName.empty(),
             "unresolved INSERT target preserves its handle and leaves the name empty");
}

void testModelerAcDsBodyDispatch(TestContext& t) {
    constexpr std::uint32_t blockHandle = 0x720u;
    constexpr std::uint32_t solidHandle = 0xA20u;
    static constexpr std::uint8_t signature[] = {
        'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F', 'i', 'l', 'e'};
    // Marker-only association bytes, deliberately not a valid SAB body.
    std::vector<std::uint8_t> payload(std::begin(signature),
                                      std::end(signature));
    payload.insert(payload.end(), {0xA5u, 0x5Au});

    std::ofstream output;
    DRW_Header header;
    ModelerDispatchFrameWriter writer(&output, &header);
    const bool generated = writer.encodeAcDsSolid(solidHandle, blockHandle);
    t.expect(generated,
             "in-memory AC1027 nonempty modeler shell and AcDs payload are generated");
    if (!generated)
        return;

    const std::vector<std::uint8_t> rawSection = makeInlineAcDsSection(
        {{solidHandle, payload}});
    t.expect(!rawSection.empty(),
             "source-authored one-record AcDs section is generated in memory");
    if (rawSection.empty())
        return;

    std::vector<std::uint8_t> sourceBytes = writer.buffer();
    dwgRW parent("");
    auto blockOwner = std::make_unique<DRW_Block_Record>();
    DRW_Block_Record* block = blockOwner.get();
    block->handle = blockHandle;
    block->name = "ACDS_SOLID_TEST";
    DwgBlockOwnershipTestAccess::setEntityHandles(*block, {solidHandle});
    ModelerDispatchReader reader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(
            sourceBytes.data(), sourceBytes.size())),
        &parent);
    reader.setFormatVersion(DRW::AC1027);
    reader.blockRecordmap.emplace(blockHandle, block);
    (void)blockOwner.release(); // reader owns block-record values.
    reader.ObjectMap.emplace(solidHandle, objHandle(38u, solidHandle, 0u));
    reader.setPrototypeSectionData(rawSection);
    const bool captured = reader.captureDataStorageSection();
    t.expect(captured && reader.rawDataStorageSections() == 1u
                 && reader.parsedDataStorageRecords() == 1u
                 && reader.rawDataStorageSectionEquals(rawSection),
             "generated AcDs bytes pass through raw page capture and typed indexing");
    if (!captured)
        return;

    dwgBuffer objectStream(sourceBytes.data(), sourceBytes.size());
    ModelerDispatchSink sink;
    sink.activeWalkOwner = blockHandle;
    const bool walked = reader.walkBlockRecordEntities(
        block, &objectStream, sink, blockHandle, blockHandle);
    reader.finalizeDataStorage();

    bool carrierMatches = walked && sink.modeler.size() == 1u;
    if (carrierMatches) {
        const ModelerDispatchSink::OwnedSolid& solid = sink.modeler.front();
        carrierMatches = solid.walkOwner == blockHandle
            && solid.entity.handle == solidHandle
            && solid.entity.parentHandle == blockHandle
            && solid.entity.eType == DRW::E3DSOLID
            && solid.entity.hasDataStorageBinaryData()
            && solid.entity.m_hasModelerData
            && !solid.entity.m_isEmpty
            && solid.entity.m_modelerVersion == 2u
            && solid.entity.hasDataStorageRecord
            && solid.entity.dataStorageHandle == solidHandle
            && solid.entity.dataStorageHandleKey == "A20"
            && solid.entity.dataStorageData == payload
            && solid.entity.m_dwgAcisPayload.empty()
            && !solid.entity.m_rawBytes.empty()
            && sink.rawEntityCallbacks.size() == 1u
            && sink.rawEntityCallbacks.front().m_rawBytes ==
                   solid.entity.m_rawBytes;
    }
    t.expect(carrierMatches,
             "3DSOLID callback retains owner, full frame, and exact linked AcDs bytes separately");
    t.expect(reader.dataStorageLinkFailures() == 0u
                 && reader.dataStorageOrphans() == 0u,
             "dispatched solid consumes exactly one generated AcDs record");
}

void testModelerAcDsRegionDispatch(TestContext& t) {
    constexpr std::uint32_t blockHandle = 0x730u;
    constexpr std::uint32_t regionHandle = 0xA30u;
    static constexpr std::uint8_t signature[] = {
        'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F', 'i', 'l', 'e'};
    // Marker-only association bytes, deliberately not a valid SAB body.
    std::vector<std::uint8_t> payload(std::begin(signature),
                                      std::end(signature));
    payload.insert(payload.end(), {0x31u, 0x73u});

    std::ofstream output;
    DRW_Header header;
    ModelerDispatchFrameWriter writer(&output, &header);
    const bool generated = writer.encodeAcDsRegion(regionHandle, blockHandle);
    t.expect(generated,
             "in-memory AC1027 REGION shell and AcDs record are generated");
    if (!generated)
        return;
    const std::vector<std::uint8_t> rawSection = makeInlineAcDsSection(
        {{regionHandle, payload}});
    t.expect(!rawSection.empty(),
             "source-authored REGION AcDs section is generated in memory");
    if (rawSection.empty())
        return;

    std::vector<std::uint8_t> sourceBytes = writer.buffer();
    dwgRW parent("");
    auto blockOwner = std::make_unique<DRW_Block_Record>();
    DRW_Block_Record* block = blockOwner.get();
    block->handle = blockHandle;
    block->name = "ACDS_REGION_TEST";
    DwgBlockOwnershipTestAccess::setEntityHandles(*block, {regionHandle});
    ModelerDispatchReader reader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(
            sourceBytes.data(), sourceBytes.size())),
        &parent);
    reader.setFormatVersion(DRW::AC1027);
    reader.blockRecordmap.emplace(blockHandle, block);
    (void)blockOwner.release(); // reader owns block-record values.
    reader.ObjectMap.emplace(regionHandle, objHandle(37u, regionHandle, 0u));
    reader.setPrototypeSectionData(rawSection);
    const bool captured = reader.captureDataStorageSection();
    t.expect(captured && reader.rawDataStorageSections() == 1u
                 && reader.parsedDataStorageRecords() == 1u
                 && reader.rawDataStorageSectionEquals(rawSection),
             "REGION AcDs bytes pass through raw page capture and typed indexing");
    if (!captured)
        return;

    dwgBuffer objectStream(sourceBytes.data(), sourceBytes.size());
    ModelerDispatchSink sink;
    sink.activeWalkOwner = blockHandle;
    const bool walked = reader.walkBlockRecordEntities(
        block, &objectStream, sink, blockHandle, blockHandle);
    reader.finalizeDataStorage();

    bool carrierMatches = walked && sink.modeler.size() == 1u;
    if (carrierMatches) {
        const ModelerDispatchSink::OwnedSolid& region = sink.modeler.front();
        carrierMatches = region.walkOwner == blockHandle
            && region.entity.handle == regionHandle
            && region.entity.parentHandle == blockHandle
            && region.entity.eType == DRW::REGION
            && region.entity.hasDataStorageBinaryData()
            && region.entity.m_hasModelerData
            && !region.entity.m_isEmpty
            && region.entity.m_modelerVersion == 2u
            && region.entity.hasDataStorageRecord
            && region.entity.dataStorageHandle == regionHandle
            && region.entity.dataStorageHandleKey == "A30"
            && region.entity.dataStorageData == payload
            && region.entity.m_dwgAcisPayload.empty()
            && !region.entity.m_rawBytes.empty()
            && sink.rawEntityCallbacks.size() == 1u
            && sink.rawEntityCallbacks.front().m_rawBytes ==
                   region.entity.m_rawBytes;
    }
    t.expect(carrierMatches,
             "REGION callback retains owner, full frame, and exact linked AcDs bytes separately");
    t.expect(reader.dataStorageLinkFailures() == 0u
                 && reader.dataStorageOrphans() == 0u,
             "dispatched REGION consumes exactly one generated AcDs record");
}

void testModelerAcDsRecordDispositions(TestContext& t) {
    constexpr std::uint32_t blockHandle = 0x740u;
    constexpr std::uint32_t missingRecordSolid = 0xA40u;
    constexpr std::uint32_t wrongHandleSolid = 0xA41u;
    constexpr std::uint32_t duplicateRecordSolid = 0xA42u;
    static constexpr std::uint8_t signature[] = {
        'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F', 'i', 'l', 'e'};
    std::vector<std::uint8_t> firstPayload(std::begin(signature),
                                          std::end(signature));
    firstPayload.insert(firstPayload.end(), {0x21u, 0x43u});
    std::vector<std::uint8_t> secondPayload(std::begin(signature),
                                           std::end(signature));
    secondPayload.insert(secondPayload.end(), {0x54u, 0x76u});
    std::vector<std::uint8_t> thirdPayload(std::begin(signature),
                                          std::end(signature));
    thirdPayload.insert(thirdPayload.end(), {0x87u, 0x65u});

    struct WalkResult {
        bool captured;
        bool walked;
        std::size_t callbacks;
        std::uint32_t callbackHandle;
        std::uint32_t callbackOwner;
        bool hasDataStorage;
        bool hasLinkedRecord;
        std::uint16_t modelerVersion;
        std::vector<std::uint8_t> linkedPayload;
        std::size_t parsedRecords;
        std::size_t linkFailures;
        std::size_t orphans;
    };
    const auto runWalk = [=](std::uint32_t solidHandle,
                             const std::vector<AcDsTestRecord>& records) {
        std::ofstream output;
        DRW_Header header;
        ModelerDispatchFrameWriter writer(&output, &header);
        if (!writer.encodeAcDsSolid(solidHandle, blockHandle))
            return WalkResult{false, false, 0u, DRW::NoHandle,
                              DRW::NoHandle, false, false, 0u, {}, 0u, 0u, 0u};

        std::vector<std::uint8_t> sourceBytes = writer.buffer();
        const std::vector<std::uint8_t> rawSection =
            makeInlineAcDsSection(records);
        if (rawSection.empty())
            return WalkResult{false, false, 0u, DRW::NoHandle,
                              DRW::NoHandle, false, false, 0u, {}, 0u, 0u, 0u};

        dwgRW parent("");
        auto blockOwner = std::make_unique<DRW_Block_Record>();
        DRW_Block_Record* block = blockOwner.get();
        block->handle = blockHandle;
        block->name = "ACDS_DISPOSITION_TEST";
        DwgBlockOwnershipTestAccess::setEntityHandles(*block, {solidHandle});
        ModelerDispatchReader reader(
            std::unique_ptr<dwgBuffer>(new dwgBuffer(
                sourceBytes.data(), sourceBytes.size())),
            &parent);
        reader.setFormatVersion(DRW::AC1027);
        reader.blockRecordmap.emplace(blockHandle, block);
        (void)blockOwner.release(); // reader owns block-record values.
        reader.ObjectMap.emplace(solidHandle,
                                 objHandle(38u, solidHandle, 0u));
        reader.setPrototypeSectionData(rawSection);
        const bool captured = reader.captureDataStorageSection();
        if (!captured) {
            reader.finalizeDataStorage();
            return WalkResult{false, false, 0u, DRW::NoHandle,
                              DRW::NoHandle, false, false, 0u, {},
                              reader.parsedDataStorageRecords(),
                              reader.dataStorageLinkFailures(),
                              reader.dataStorageOrphans()};
        }

        dwgBuffer objectStream(sourceBytes.data(), sourceBytes.size());
        ModelerDispatchSink sink;
        sink.activeWalkOwner = blockHandle;
        const bool walked = reader.walkBlockRecordEntities(
            block, &objectStream, sink, blockHandle, blockHandle);
        reader.finalizeDataStorage();
        WalkResult result{captured, walked, sink.modeler.size(),
                          DRW::NoHandle, DRW::NoHandle, false, false, 0u,
                          {}, reader.parsedDataStorageRecords(),
                          reader.dataStorageLinkFailures(),
                          reader.dataStorageOrphans()};
        if (!sink.modeler.empty()) {
            const ModelerDispatchSink::OwnedSolid& solid =
                sink.modeler.front();
            result.callbackHandle = solid.entity.handle;
            result.callbackOwner = solid.walkOwner;
            result.hasDataStorage = solid.entity.hasDataStorageBinaryData();
            result.hasLinkedRecord = solid.entity.hasDataStorageRecord;
            result.modelerVersion = solid.entity.m_modelerVersion;
            result.linkedPayload = solid.entity.dataStorageData;
        }
        return result;
    };

    const WalkResult missing = runWalk(missingRecordSolid, {});
    const WalkResult wrongHandle = runWalk(
        wrongHandleSolid, {{wrongHandleSolid + 1u, firstPayload}});
    const WalkResult duplicate = runWalk(
        duplicateRecordSolid, {{duplicateRecordSolid, firstPayload},
                               {duplicateRecordSolid, secondPayload},
                               {duplicateRecordSolid, thirdPayload}});
    t.expect(missing.captured && missing.walked && missing.callbacks == 1u
                 && missing.callbackHandle == missingRecordSolid
                 && missing.callbackOwner == blockHandle
                 && missing.hasDataStorage && !missing.hasLinkedRecord
                 && missing.modelerVersion == 168u
                 && missing.linkedPayload.empty()
                 && missing.parsedRecords == 0u
                 && missing.linkFailures == 1u && missing.orphans == 0u,
             "missing AcDs record stays explicit while the typed solid callback survives");
    t.expect(wrongHandle.captured && wrongHandle.walked
                 && wrongHandle.callbacks == 1u
                 && wrongHandle.callbackHandle == wrongHandleSolid
                 && wrongHandle.hasDataStorage && !wrongHandle.hasLinkedRecord
                 && wrongHandle.modelerVersion == 168u
                 && wrongHandle.linkedPayload.empty()
                 && wrongHandle.parsedRecords == 1u
                 && wrongHandle.linkFailures == 1u && wrongHandle.orphans == 1u,
             "wrong-handle AcDs record is not attached and is reported orphaned");
    t.expect(duplicate.captured && duplicate.walked
                 && duplicate.callbacks == 1u
                 && duplicate.callbackHandle == duplicateRecordSolid
                 && duplicate.hasDataStorage && duplicate.hasLinkedRecord
                 && duplicate.modelerVersion == 2u
                 && duplicate.linkedPayload == thirdPayload
                 && duplicate.parsedRecords == 3u
                 && duplicate.linkFailures == 0u && duplicate.orphans == 2u,
             "three duplicate AcDs records keep the deterministic preferred payload and expose the others as orphans");

    const std::vector<std::uint8_t> duplicateSectionBytes =
        makeInlineAcDsSection({{duplicateRecordSolid, firstPayload},
                               {duplicateRecordSolid, secondPayload},
                               {duplicateRecordSolid, thirdPayload}});
    const DRW_DataStorageSection duplicateSection = DRW_parseDataStorage(
        duplicateSectionBytes, DRW::AC1027);
    const auto duplicateDiagnostic = std::find_if(
        duplicateSection.diagnostics.cbegin(),
        duplicateSection.diagnostics.cend(),
        [](const DRW_DataStorageDiagnostic& diagnostic) {
            return diagnostic.code == "datastorage-duplicate-record-handle";
        });
    const auto duplicateIndexes = duplicateSection.recordIndexesByHandle.find(
        duplicateRecordSolid);
    const auto duplicateKeyIndexes =
        duplicateSection.recordIndexesByHandleKey.find("A42");
    const DRW_DataStorageRecord* preferredDuplicate =
        duplicateSection.findRecordByHandle(duplicateRecordSolid);
    t.expect(duplicateSection.records.size() == 3u
                 && duplicateSection.duplicateRecordHandleKeys
                        == std::vector<UTF8STRING>{"A42"}
                 && duplicateIndexes
                        != duplicateSection.recordIndexesByHandle.end()
                 && duplicateIndexes->second
                        == std::vector<std::size_t>{0u, 1u, 2u}
                 && duplicateKeyIndexes
                        != duplicateSection.recordIndexesByHandleKey.end()
                 && duplicateKeyIndexes->second
                        == std::vector<std::size_t>{0u, 1u, 2u}
                 && preferredDuplicate == &duplicateSection.records[2]
                 && duplicateDiagnostic != duplicateSection.diagnostics.cend()
                 && duplicateDiagnostic->message.find("selected record 2")
                        != std::string::npos
                 && duplicateDiagnostic->handle == duplicateRecordSolid
                 && duplicateDiagnostic->hasHandle
                 && duplicateDiagnostic->offset
                        == duplicateSection.records[2].recordOffset
                 && duplicateDiagnostic->hasOffset,
             "three-record duplicate index diagnostic reports the final preferred record and preserves every source index");
}

void testModelerAcDsOverlappingRecordHeaders(TestContext& t) {
    constexpr std::uint32_t firstHandle = 0xA60u;
    constexpr std::uint32_t secondHandle = 0xA61u;
    constexpr std::uint32_t followingHandle = 0xA62u;
    const auto payload = [](std::uint8_t marker) {
        static constexpr std::uint8_t signature[] = {
            'A', 'C', 'I', 'S', ' ', 'B', 'i', 'n', 'a', 'r', 'y', 'F', 'i',
            'l', 'e'};
        std::vector<std::uint8_t> bytes(std::begin(signature),
                                        std::end(signature));
        bytes.push_back(marker);
        return bytes;
    };
    const std::vector<std::uint8_t> firstPayload = payload(0x10u);
    const std::vector<std::uint8_t> secondPayload = payload(0x20u);
    const std::vector<std::uint8_t> followingPayload = payload(0x30u);
    std::vector<std::uint8_t> sectionBytes = makeInlineAcDsSection({
        {firstHandle, firstPayload},
        {secondHandle, secondPayload},
        {followingHandle, followingPayload}});
    t.expect(!sectionBytes.empty(),
             "three independent AC1027 data records are generated for the overlapping-header control");
    if (sectionBytes.empty())
        return;

    // The second DataIndex row now points one byte into the first fixed-size
    // 20-byte record header. The third row and header remain independently
    // bounded and valid.
    constexpr std::size_t dataIndexOffset = 192u;
    constexpr std::size_t dataIndexEntriesOffset = dataIndexOffset + 56u;
    const auto putU32 = [&sectionBytes](std::size_t offset,
                                        std::uint32_t value) {
        for (unsigned byte = 0; byte < 4u; ++byte) {
            sectionBytes[offset + byte] =
                static_cast<std::uint8_t>(value >> (byte * 8u));
        }
    };
    putU32(dataIndexEntriesOffset + 12u + 4u, 1u);

    const DRW_DataStorageSection parsed = DRW_parseDataStorage(
        sectionBytes, DRW::AC1027);
    const std::size_t overlapDiagnostics = static_cast<std::size_t>(
        std::count_if(parsed.diagnostics.cbegin(), parsed.diagnostics.cend(),
            [](const DRW_DataStorageDiagnostic& diagnostic) {
                return diagnostic.code
                    == "datastorage-record-header-overlap";
            }));
    const DRW_DataStorageRecord* following =
        parsed.findRecordByHandle(followingHandle);
    t.expect(!parsed.structurallyValid && !parsed.replayAllowed
                 && overlapDiagnostics >= 2u
                 && parsed.findRecordByHandle(firstHandle) == nullptr
                 && parsed.findRecordByHandle(secondHandle) == nullptr
                 && following != nullptr
                 && following->payload == followingPayload,
             "partially overlapping 20-byte DataIndex record headers are refused while an independent following record remains readable");
}

void testModelerAcDsDataIndexOrder(TestContext& t) {
    constexpr std::uint32_t blockHandle = 0x750u;
    constexpr std::uint32_t lineSentinelHandle = 0xA55u;
    const std::vector<std::uint32_t> callbackOrder {
        0xA54u, 0xA53u, 0xA52u, 0xA51u};
    const auto makePayload = [](const char* signature, std::uint8_t marker) {
        const std::string prefix(signature);
        std::vector<std::uint8_t> payload(prefix.begin(), prefix.end());
        payload.push_back(marker);
        return payload;
    };
    const std::map<std::uint32_t, std::vector<std::uint8_t>> payloadByHandle {
        {0xA51u, makePayload("ACIS BinaryFile", 0x51u)},
        {0xA52u, makePayload("ASM BinaryFile4", 0x52u)},
        {0xA53u, makePayload("ACIS BinaryFile", 0x53u)},
        {0xA54u, makePayload("ASM BinaryFile4", 0x54u)}};
    // The directory rows, physical record stream, numeric handle order, and
    // entity callback order are four distinct permutations.
    const std::vector<AcDsTestRecord> physicalRecords {
        {0xA52u, payloadByHandle.at(0xA52u)},
        {0xA54u, payloadByHandle.at(0xA54u)},
        {0xA51u, payloadByHandle.at(0xA51u)},
        {0xA53u, payloadByHandle.at(0xA53u)}};
    const std::vector<std::size_t> dataIndexOrder {3u, 2u, 0u, 1u};

    std::ofstream output;
    DRW_Header header;
    ModelerDispatchFrameWriter writer(&output, &header);
    bool generated = true;
    for (std::uint32_t handle : callbackOrder)
        generated = generated && writer.encodeAcDsSolid(handle, blockHandle);
    generated = generated
        && writer.encodeLine(lineSentinelHandle, blockHandle);
    t.expect(generated,
             "four source-authored AC1027 modeler frames and a following LINE are generated");
    if (!generated)
        return;

    const std::vector<std::uint8_t> baseSection =
        makeInlineAcDsSection(physicalRecords, dataIndexOrder);
    const std::vector<std::uint8_t> rawSection =
        addStaleSecondaryAcDsSegmentIndexWithOrphanData(baseSection);
    t.expect(!rawSection.empty(),
             "four-record AcDs section with a stale secondary segidx is generated in memory");
    if (rawSection.empty())
        return;

    const DRW_DataStorageSection staleIndexParsed = DRW_parseDataStorage(
        rawSection, DRW::AC1027);
    t.expect(staleIndexParsed.structurallyValid
                 && staleIndexParsed.replayAllowed
                 && staleIndexParsed.segments.size() == 4u
                 && staleIndexParsed.records.size() == physicalRecords.size()
                 && staleIndexParsed.findRecordByHandle(0xA50u) == nullptr,
             "live segidx remains fully readable when its first row names a stale secondary index");

    std::vector<std::uint8_t> oversizedIndex = rawSection;
    const auto putU32 = [](std::vector<std::uint8_t>& bytes,
                           std::size_t offset, std::uint32_t value) {
        for (unsigned index = 0; index < 4u; ++index)
            bytes[offset + index] = static_cast<std::uint8_t>(value >> (index * 8u));
    };
    const auto readU32 = [](const std::vector<std::uint8_t>& bytes,
                            std::size_t offset) {
        return static_cast<std::uint32_t>(bytes[offset])
            | (static_cast<std::uint32_t>(bytes[offset + 1u]) << 8u)
            | (static_cast<std::uint32_t>(bytes[offset + 2u]) << 16u)
            | (static_cast<std::uint32_t>(bytes[offset + 3u]) << 24u);
    };

    std::vector<std::uint8_t> duplicateDataIndexRow = rawSection;
    const auto dataIndexSegment = std::find_if(
        staleIndexParsed.segments.cbegin(), staleIndexParsed.segments.cend(),
        [](const DRW_DataStorageSegment& segment) {
            return segment.kind == DRW_DataStorageSegmentKind::DataIndex;
        });
    bool duplicateRowControlBuilt =
        dataIndexSegment != staleIndexParsed.segments.cend();
    if (duplicateRowControlBuilt) {
        const std::size_t firstRow = static_cast<std::size_t>(
            dataIndexSegment->offset
            + DRW_DataStorageConst::SEGMENT_HEADER_SIZE + 8u);
        const std::size_t secondRow =
            firstRow + DRW_DataStorageConst::DATA_INDEX_ENTRY_SIZE;
        if (firstRow > duplicateDataIndexRow.size()
            || duplicateDataIndexRow.size() - firstRow
                   < 2u * DRW_DataStorageConst::DATA_INDEX_ENTRY_SIZE) {
            duplicateRowControlBuilt = false;
        } else {
            std::copy_n(duplicateDataIndexRow.cbegin()
                            + static_cast<std::ptrdiff_t>(firstRow),
                        DRW_DataStorageConst::DATA_INDEX_ENTRY_SIZE,
                        duplicateDataIndexRow.begin()
                            + static_cast<std::ptrdiff_t>(secondRow));
        }
    }
    t.expect(duplicateRowControlBuilt,
             "a generated duplicate DataIndex target control is in bounds");
    if (duplicateRowControlBuilt) {
        const DRW_DataStorageSection duplicateIndexParsed =
            DRW_parseDataStorage(duplicateDataIndexRow, DRW::AC1027);
        const auto duplicateIndexDiagnostic = std::find_if(
            duplicateIndexParsed.diagnostics.cbegin(),
            duplicateIndexParsed.diagnostics.cend(),
            [](const DRW_DataStorageDiagnostic& diagnostic) {
                return diagnostic.code
                    == "datastorage-duplicate-data-index-entry";
            });
        t.expect(!duplicateIndexParsed.structurallyValid
                     && !duplicateIndexParsed.replayAllowed
                     && duplicateIndexDiagnostic
                            != duplicateIndexParsed.diagnostics.cend()
                     && duplicateIndexParsed.dataIndexEntries.size()
                            + 1u == staleIndexParsed.dataIndexEntries.size()
                     && duplicateIndexParsed.records.size() + 1u
                            == staleIndexParsed.records.size(),
                 "duplicate DataIndex record locations are diagnosed and cannot be replayed");
    }

    if (duplicateRowControlBuilt) {
        const std::size_t firstRow = static_cast<std::size_t>(
            dataIndexSegment->offset
            + DRW_DataStorageConst::SEGMENT_HEADER_SIZE + 8u);
        const std::size_t secondRow =
            firstRow + DRW_DataStorageConst::DATA_INDEX_ENTRY_SIZE;
        std::vector<std::uint8_t> duplicateTargetDifferentSchema = rawSection;
        std::copy_n(duplicateTargetDifferentSchema.cbegin()
                        + static_cast<std::ptrdiff_t>(firstRow),
                    DRW_DataStorageConst::DATA_INDEX_ENTRY_SIZE,
                    duplicateTargetDifferentSchema.begin()
                        + static_cast<std::ptrdiff_t>(secondRow));
        putU32(duplicateTargetDifferentSchema, secondRow + 8u,
               readU32(duplicateTargetDifferentSchema, firstRow + 8u) + 1u);
        const DRW_DataStorageSection differentSchemaIndexParsed =
            DRW_parseDataStorage(duplicateTargetDifferentSchema, DRW::AC1027);
        const bool differentSchemaDuplicateDiagnosed = std::any_of(
            differentSchemaIndexParsed.diagnostics.cbegin(),
            differentSchemaIndexParsed.diagnostics.cend(),
            [](const DRW_DataStorageDiagnostic& diagnostic) {
                return diagnostic.code
                    == "datastorage-duplicate-data-index-entry";
            });
        t.expect(differentSchemaDuplicateDiagnosed
                     && !differentSchemaIndexParsed.structurallyValid
                     && !differentSchemaIndexParsed.replayAllowed
                     && differentSchemaIndexParsed.dataIndexEntries.size()
                            + 1u == staleIndexParsed.dataIndexEntries.size()
                     && differentSchemaIndexParsed.records.size() + 1u
                            == staleIndexParsed.records.size(),
                 "a repeated physical DataIndex target remains ambiguous even when its schema index differs");
    }

    const auto recordSegment = std::find_if(
        staleIndexParsed.segments.cbegin(), staleIndexParsed.segments.cend(),
        [](const DRW_DataStorageSegment& segment) {
            return segment.kind == DRW_DataStorageSegmentKind::Data;
        });
    std::vector<const DRW_DataStorageRecord*> recordsByPayloadOffset;
    if (recordSegment != staleIndexParsed.segments.cend()) {
        for (const DRW_DataStorageRecord& record : staleIndexParsed.records) {
            if (record.segmentIndex == recordSegment->index)
                recordsByPayloadOffset.push_back(&record);
        }
        std::sort(recordsByPayloadOffset.begin(), recordsByPayloadOffset.end(),
                  [](const DRW_DataStorageRecord* lhs,
                     const DRW_DataStorageRecord* rhs) {
                      return lhs->recordLocalOffset < rhs->recordLocalOffset;
                  });
    }
    const bool malformedBoundaryControlBuilt =
        recordsByPayloadOffset.size() >= 3u
        && recordsByPayloadOffset[1]->recordLocalOffset
               > recordsByPayloadOffset[0]->recordLocalOffset + 3u;
    t.expect(malformedBoundaryControlBuilt,
             "a generated malformed-entry boundary control is in bounds");
    if (malformedBoundaryControlBuilt) {
        const DRW_DataStorageRecord& predecessor =
            *recordsByPayloadOffset[0];
        const DRW_DataStorageRecord& malformed =
            *recordsByPayloadOffset[1];
        const DRW_DataStorageRecord& suffix = *recordsByPayloadOffset[2];
        const std::uint32_t nextRecordDistance =
            malformed.recordLocalOffset - predecessor.recordLocalOffset;
        const std::size_t malformedHeaderOffset = static_cast<std::size_t>(
            recordSegment->offset
            + DRW_DataStorageConst::SEGMENT_HEADER_SIZE
            + malformed.localOffset);
        std::vector<std::uint8_t> malformedHeaderBoundary = rawSection;
        putU32(malformedHeaderBoundary,
               static_cast<std::size_t>(predecessor.recordOffset),
               nextRecordDistance - 3u);
        putU32(malformedHeaderBoundary, malformedHeaderOffset,
               DRW_DataStorageConst::DATA_RECORD_HEADER_SIZE - 1u);

        const DRW_DataStorageSection rejectedMalformedBoundary =
            DRW_parseDataStorage(malformedHeaderBoundary, DRW::AC1027);
        bool predecessorBoundaryDiagnostic = false;
        bool malformedHeaderDiagnostic = false;
        for (const DRW_DataStorageDiagnostic& diagnostic :
             rejectedMalformedBoundary.diagnostics) {
            predecessorBoundaryDiagnostic = predecessorBoundaryDiagnostic
                || (diagnostic.code
                        == "datastorage-record-payload-out-of-record"
                    && diagnostic.handle == predecessor.handle);
            malformedHeaderDiagnostic = malformedHeaderDiagnostic
                || (diagnostic.code == "datastorage-record-entry-invalid"
                    && diagnostic.handle == malformed.handle);
        }
        const DRW_DataStorageRecord* retainedSuffix =
            rejectedMalformedBoundary.findRecordByHandle(suffix.handle);
        t.expect(!rejectedMalformedBoundary.structurallyValid
                     && !rejectedMalformedBoundary.replayAllowed
                     && predecessorBoundaryDiagnostic
                     && malformedHeaderDiagnostic
                     && rejectedMalformedBoundary.findRecordByHandle(
                            predecessor.handle) == nullptr
                     && rejectedMalformedBoundary.findRecordByHandle(
                            malformed.handle) == nullptr
                     && retainedSuffix != nullptr
                     && retainedSuffix->payload == suffix.payload,
                 "an undersized indexed record header still bounds its predecessor and leaves a following valid record readable");
    }

    putU32(oversizedIndex, 32u, 8u);
    const DRW_DataStorageSection boundedIndex = DRW_parseDataStorage(
        oversizedIndex, DRW::AC1027);
    const bool countWasCapped = std::any_of(
        boundedIndex.diagnostics.begin(), boundedIndex.diagnostics.end(),
        [](const DRW_DataStorageDiagnostic& diagnostic) {
            return diagnostic.code == "datastorage-count-capped";
        });
    t.expect(countWasCapped && !boundedIndex.structurallyValid
                 && !boundedIndex.replayAllowed
                 && boundedIndex.segments.size() == 6u,
             "oversized live segidx count is capped by its own extent despite a stale row-zero reference");

    std::vector<std::uint8_t> invalidExtent = rawSection;
    const std::size_t segmentIndexOffset = readU32(rawSection, 24u);
    putU32(invalidExtent, segmentIndexOffset + 16u,
           std::numeric_limits<std::uint32_t>::max());
    const DRW_DataStorageSection invalidExtentParsed = DRW_parseDataStorage(
        invalidExtent, DRW::AC1027);
    const bool invalidExtentDiagnosed = std::any_of(
        invalidExtentParsed.diagnostics.begin(),
        invalidExtentParsed.diagnostics.end(),
        [](const DRW_DataStorageDiagnostic& diagnostic) {
            return diagnostic.code == "datastorage-segment-index-size-invalid";
        });
    t.expect(invalidExtentDiagnosed
                 && !invalidExtentParsed.structurallyValid
                 && !invalidExtentParsed.replayAllowed
                 && invalidExtentParsed.segments.empty(),
             "out-of-bounds live segidx extent fails closed before reading directory rows");

    std::vector<std::uint8_t> sourceBytes = writer.buffer();
    dwgRW parent("");
    auto blockOwner = std::make_unique<DRW_Block_Record>();
    DRW_Block_Record* block = blockOwner.get();
    block->handle = blockHandle;
    block->name = "ACDS_INDEX_ORDER_TEST";
    std::vector<std::uint32_t> entityOrder = callbackOrder;
    entityOrder.push_back(lineSentinelHandle);
    DwgBlockOwnershipTestAccess::setEntityHandles(*block, entityOrder);
    ModelerDispatchReader reader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(
            sourceBytes.data(), sourceBytes.size())),
        &parent);
    reader.setFormatVersion(DRW::AC1027);
    reader.blockRecordmap.emplace(blockHandle, block);
    (void)blockOwner.release(); // reader owns block-record values.
    for (const auto& object : writer.objectOffsets()) {
        const std::uint32_t objectType = object.first == lineSentinelHandle
            ? 19u : 38u;
        reader.ObjectMap.emplace(
            object.first, objHandle(objectType, object.first, object.second));
    }
    reader.setPrototypeSectionData(rawSection);
    const bool captured = reader.captureDataStorageSection();
    t.expect(captured && reader.rawDataStorageSections() == 1u
                 && reader.parsedDataStorageRecords() == callbackOrder.size()
                 && reader.rawDataStorageSectionEquals(rawSection),
             "reordered DataIndex rows retain exact raw section and all typed records");
    if (!captured)
        return;

    dwgBuffer objectStream(sourceBytes.data(), sourceBytes.size());
    AcDsOrderDispatchSink sink;
    sink.activeWalkOwner = blockHandle;
    const bool walked = reader.walkBlockRecordEntities(
        block, &objectStream, sink, blockHandle, blockHandle);
    reader.finalizeDataStorage();

    bool linksMatch = walked && sink.modeler.size() == callbackOrder.size();
    const auto handleKey = [](std::uint32_t handle) {
        std::ostringstream text;
        text << std::uppercase << std::hex << handle;
        return text.str();
    };
    if (linksMatch) {
        for (std::size_t index = 0; index < callbackOrder.size(); ++index) {
            const auto& modeler = sink.modeler[index];
            const std::uint32_t expectedHandle = callbackOrder[index];
            const auto expectedPayload = payloadByHandle.find(expectedHandle);
            if (expectedPayload == payloadByHandle.end()
                || modeler.walkOwner != blockHandle
                || modeler.entity.handle != expectedHandle
                || !modeler.entity.hasDataStorageRecord
                || modeler.entity.dataStorageHandle != expectedHandle
                || modeler.entity.dataStorageHandleKey
                       != handleKey(expectedHandle)
                || modeler.entity.dataStorageData != expectedPayload->second) {
                linksMatch = false;
                break;
            }
        }
    }
    if (!linksMatch) {
        std::cerr << "AcDs DataIndex order diagnostic: walked=" << walked
                  << " callbacks=" << sink.modeler.size()
                  << " linkFailures=" << reader.dataStorageLinkFailures()
                  << " orphans=" << reader.dataStorageOrphans() << '\n';
        for (const auto& modeler : sink.modeler) {
            std::cerr << "  handle=" << std::hex << modeler.entity.handle
                      << " linked=" << modeler.entity.hasDataStorageRecord
                      << " linkedHandle=" << modeler.entity.dataStorageHandle
                      << " payloadBytes=" << std::dec
                      << modeler.entity.dataStorageData.size() << '\n';
        }
    }
    t.expect(linksMatch,
             "DataIndex handle/offset links beat physical, numeric, and callback ordering");
    t.expect(reader.dataStorageLinkFailures() == 0u
                 && reader.dataStorageOrphans() == 0u,
             "all reordered AcDs records resolve exactly once without orphans");
    const bool lineSentinelMatch = walked && sink.lines.size() == 1u
        && sink.lines.front().handle == lineSentinelHandle
        && sink.lines.front().basePoint.x == 71.0
        && sink.lines.front().basePoint.y == 83.0
        && sink.lines.front().secPoint.x == 97.0
        && sink.lines.front().secPoint.y == 101.0;
    t.expect(lineSentinelMatch,
             "alternate ASM/ACIS DataStorage records preserve the following LINE");

    dwgRW ambiguousParent("");
    auto ambiguousBlockOwner = std::make_unique<DRW_Block_Record>();
    DRW_Block_Record* ambiguousBlock = ambiguousBlockOwner.get();
    ambiguousBlock->handle = blockHandle;
    ambiguousBlock->name = "ACDS_AMBIGUOUS_INDEX_TEST";
    DwgBlockOwnershipTestAccess::setEntityHandles(*ambiguousBlock, entityOrder);
    ModelerDispatchReader ambiguousReader(
        std::unique_ptr<dwgBuffer>(new dwgBuffer(
            sourceBytes.data(), sourceBytes.size())),
        &ambiguousParent);
    ambiguousReader.setFormatVersion(DRW::AC1027);
    ambiguousReader.blockRecordmap.emplace(blockHandle, ambiguousBlock);
    (void)ambiguousBlockOwner.release();
    for (const auto& object : writer.objectOffsets()) {
        const std::uint32_t objectType = object.first == lineSentinelHandle
            ? 19u : 38u;
        ambiguousReader.ObjectMap.emplace(
            object.first, objHandle(objectType, object.first, object.second));
    }
    ambiguousReader.setPrototypeSectionData(duplicateDataIndexRow);
    const bool ambiguousCaptured =
        ambiguousReader.captureDataStorageSection();
    const bool noAmbiguousPublication = !ambiguousCaptured
        && ambiguousReader.rawDataStorageSections() == 0u
        && ambiguousReader.parsedDataStorageRecords() == 0u
        && !ambiguousReader.rawDataStorageSectionEquals(duplicateDataIndexRow);
    t.expect(noAmbiguousPublication,
             "structurally ambiguous DataIndex refuses atomic raw/typed section publication before the entity walk");
    ambiguousReader.setPrototypeSectionData(rawSection);
    const bool repairedCapture = ambiguousReader.captureDataStorageSection();
    t.expect(repairedCapture
                 && ambiguousReader.rawDataStorageSectionEquals(rawSection)
                 && ambiguousReader.parsedDataStorageRecords()
                        == callbackOrder.size(),
             "valid DataIndex section captures cleanly after the ambiguous capture refusal on the same reader");
}

void testModelerAcDsBlobPageChain(TestContext& t) {
    constexpr std::uint32_t blockHandle = 0x760u;
    constexpr std::uint32_t solidHandle = 0xA80u;
    const AcDsBlobTestSection generated = makeTwoPageAcDsSection(solidHandle);
    t.expect(!generated.bytes.empty() && !generated.payload.empty(),
             "source-authored two-page Blob01 AcDs chain is generated in memory");
    if (generated.bytes.empty() || generated.payload.empty())
        return;

    std::ofstream output;
    DRW_Header header;
    ModelerDispatchFrameWriter writer(&output, &header);
    const bool frameGenerated = writer.encodeAcDsSolid(
        solidHandle, blockHandle, {0x5A5u});
    t.expect(frameGenerated,
             "modeler frame for two-page AcDs linkage is generated in memory");
    if (!frameGenerated)
        return;
    std::vector<std::uint8_t> sourceBytes = writer.buffer();

    dwgRW parent("");
    struct ReaderFixture {
        std::unique_ptr<ModelerDispatchReader> reader;
        DRW_Block_Record* block = nullptr;
    };
    const auto makeReader = [&](const std::vector<std::uint8_t>& sectionBytes) {
        ReaderFixture fixture;
        fixture.reader.reset(new ModelerDispatchReader(
            std::unique_ptr<dwgBuffer>(new dwgBuffer(
                sourceBytes.data(), sourceBytes.size())),
            &parent));
        fixture.reader->setFormatVersion(DRW::AC1027);
        auto blockOwner = std::make_unique<DRW_Block_Record>();
        fixture.block = blockOwner.get();
        fixture.block->handle = blockHandle;
        fixture.block->name = "ACDS_BLOB_PAGE_CHAIN_TEST";
        DwgBlockOwnershipTestAccess::setEntityHandles(
            *fixture.block, {solidHandle});
        fixture.reader->blockRecordmap.emplace(blockHandle, fixture.block);
        (void)blockOwner.release(); // Reader owns block-record values.
        fixture.reader->ObjectMap.emplace(
            solidHandle, objHandle(38u, solidHandle,
                                   writer.modelerObjectOffset(solidHandle)));
        fixture.reader->setPrototypeSectionData(sectionBytes);
        return fixture;
    };

    ReaderFixture valid = makeReader(generated.bytes);
    const bool captured = valid.reader->captureDataStorageSection();
    const DRW_DataStorageRecord* record =
        valid.reader->parsedDataStorageRecord(solidHandle);
    const bool pageMetadataMatches = record != nullptr
        && record->isBlobReference
        && record->blobTotalByteLength == generated.payload.size()
        && record->blobPageCount == 2u
        && record->blobPageByteSize
               == generated.firstPageByteLength
        && record->blobLastPageByteSize == generated.secondPageByteLength
        && record->blobPages.size() == 2u
        && record->blobPages[0].segmentIndex == 4u
        && record->blobPages[0].byteLength
               == generated.firstPageByteLength
        && record->blobPages[1].segmentIndex == 3u
        && record->blobPages[1].byteLength == generated.secondPageByteLength
        && record->blobPayloadRetained
        && record->payload == generated.payload;
    if (!captured || !pageMetadataMatches) {
        std::cerr << "AcDs Blob01 chain diagnostic: captured=" << captured
                  << " raw=" << valid.reader->rawDataStorageSections()
                  << " records=" << valid.reader->parsedDataStorageRecords()
                  << " record=" << (record != nullptr);
        if (record != nullptr) {
            std::cerr << " blob=" << record->isBlobReference
                      << " handle=" << record->handle
                      << " segment=" << record->segmentIndex
                      << " local=" << record->localOffset
                      << " recordLocal=" << record->recordLocalOffset
                      << " entrySize=" << record->entrySize
                      << " dataSize=" << record->dataByteLength
                      << " recordOffset=" << record->recordOffset
                      << " dataOffset=" << record->dataOffset
                      << " total=" << record->blobTotalByteLength
                      << " pages=" << record->blobPageCount
                      << " pageSize=" << record->blobPageByteSize
                      << " lastPage=" << record->blobLastPageByteSize
                      << " retained=" << record->blobPayloadRetained
                      << " payload=" << record->payload.size();
        }
        std::cerr << '\n';
    }
    t.expect(captured && valid.reader->rawDataStorageSections() == 1u
                 && valid.reader->parsedDataStorageRecords() == 1u
                 && valid.reader->rawDataStorageSectionEquals(generated.bytes)
                 && pageMetadataMatches,
             "out-of-physical-order Blob01 pages assemble from validated descriptor order");
    if (!captured || !pageMetadataMatches)
        return;

    dwgBuffer objectStream(sourceBytes.data(), sourceBytes.size());
    ModelerDispatchSink sink;
    sink.activeWalkOwner = blockHandle;
    const bool walked = valid.reader->walkBlockRecordEntities(
        valid.block, &objectStream, sink, blockHandle, blockHandle);
    valid.reader->finalizeDataStorage();
    const bool linked = walked && sink.modeler.size() == 1u
        && sink.modelerMoveCallbacks == 1u
        && sink.rawEntityCallbacks.size() == 1u
        && sink.modeler.front().entity.handle == solidHandle
        && sink.modeler.front().entity.hasDataStorageRecord
        && sink.modeler.front().entity.dataStorageData == generated.payload
        && sink.modeler.front().entity.reactorHandles
               == std::vector<std::uint32_t>{0x5A5u}
        && sink.rawEntityCallbacks.front().m_handle == solidHandle
        && sink.rawEntityCallbacks.front().m_hasDataStorage
        && sink.rawEntityCallbacks.front().m_parentHandle == blockHandle
        && sink.rawEntityCallbacks.front().m_reactorHandles
               == std::vector<std::uint32_t>{0x5A5u}
        && !sink.rawEntityCallbacks.front().m_rawBytes.empty();
    t.expect(linked && valid.reader->dataStorageLinkFailures() == 0u
                 && valid.reader->dataStorageOrphans() == 0u,
             "assembled Blob01 bytes link to their owning modeler entity without loss");

    const AcDsBlobTestSection thresholdGenerated = makeTwoPageAcDsSection(
        solidHandle,
        DRW_DataStorageConst::DATA_RECORD_INLINE_MAX_SIZE, 1u);
    const DRW_DataStorageSection thresholdParsed = DRW_parseDataStorage(
        thresholdGenerated.bytes, DRW::AC1027);
    const DRW_DataStorageRecord* thresholdRecord =
        thresholdParsed.findRecordByHandle(solidHandle);
    t.expect(thresholdParsed.structurallyValid && thresholdRecord != nullptr
                 && thresholdRecord->isBlobReference
                 && thresholdRecord->blobTotalByteLength
                        == DRW_DataStorageConst::DATA_RECORD_INLINE_MAX_SIZE + 1u
                 && thresholdRecord->blobPages.size() == 2u
                 && thresholdRecord->blobPages[0].byteLength
                        == DRW_DataStorageConst::DATA_RECORD_INLINE_MAX_SIZE
                 && thresholdRecord->blobPages[1].byteLength == 1u
                 && thresholdRecord->payload == thresholdGenerated.payload,
             "first payload byte above the direct-record limit uses Blob01 pages");

    const AcDsBlobTestSection onePageAtCap = makeTwoPageAcDsSection(
        solidHandle, DRW_DataStorageConst::DATA_BLOB_PAGE_MAX_SIZE, 0u);
    const DRW_DataStorageSection onePageParsed = DRW_parseDataStorage(
        onePageAtCap.bytes, DRW::AC1027);
    const DRW_DataStorageRecord* onePageRecord =
        onePageParsed.findRecordByHandle(solidHandle);
    t.expect(onePageParsed.structurallyValid && onePageRecord != nullptr
                 && onePageRecord->isBlobReference
                 && onePageRecord->blobTotalByteLength
                        == DRW_DataStorageConst::DATA_BLOB_PAGE_MAX_SIZE
                 && onePageRecord->blobPageCount == 1u
                 && onePageRecord->blobPages.size() == 1u
                 && onePageRecord->blobPages.front().byteLength
                        == DRW_DataStorageConst::DATA_BLOB_PAGE_MAX_SIZE
                 && onePageRecord->payload == onePageAtCap.payload,
             "one-page Blob01 accepts the exact page-size cap");

    const AcDsBlobTestSection twoPagesOverCap = makeTwoPageAcDsSection(
        solidHandle, DRW_DataStorageConst::DATA_BLOB_PAGE_MAX_SIZE, 1u);
    const DRW_DataStorageSection twoPagesParsed = DRW_parseDataStorage(
        twoPagesOverCap.bytes, DRW::AC1027);
    const DRW_DataStorageRecord* twoPagesRecord =
        twoPagesParsed.findRecordByHandle(solidHandle);
    t.expect(twoPagesParsed.structurallyValid && twoPagesRecord != nullptr
                 && twoPagesRecord->blobPageCount == 2u
                 && twoPagesRecord->blobTotalByteLength
                        == DRW_DataStorageConst::DATA_BLOB_PAGE_MAX_SIZE + 1u
                 && twoPagesRecord->blobPages.size() == 2u
                 && twoPagesRecord->blobPages[0].byteLength
                        == DRW_DataStorageConst::DATA_BLOB_PAGE_MAX_SIZE
                 && twoPagesRecord->blobPages[1].byteLength == 1u
                 && twoPagesRecord->payload == twoPagesOverCap.payload,
             "Blob01 rolls into a second page immediately above the page-size cap");

    const auto putU32 = [](std::vector<std::uint8_t>& bytes,
                           std::size_t offset, std::uint32_t value) {
        for (unsigned index = 0; index < 4u; ++index)
            bytes[offset + index] = static_cast<std::uint8_t>(
                value >> (index * 8u));
    };
    const auto putU64 = [&putU32](std::vector<std::uint8_t>& bytes,
                                  std::size_t offset, std::uint64_t value) {
        putU32(bytes, offset, static_cast<std::uint32_t>(value));
        putU32(bytes, offset + 4u, static_cast<std::uint32_t>(value >> 32u));
    };
    struct Mutation {
        const char* name;
        std::size_t offset;
        std::uint64_t value;
        bool wide;
    };
    const std::array<Mutation, 4> malformedCases {{
        {"missing page segment", generated.secondPageReferenceOffset, 99u, false},
        {"duplicate page ordinal", generated.secondPageHeaderOffset + 16u, 0u, false},
        {"incorrect page start offset", generated.secondPageHeaderOffset + 8u, 0u, true},
        {"mismatched page reference length", generated.secondPageReferenceOffset + 4u, 47u, false}}};
    for (const Mutation& mutation : malformedCases) {
        std::vector<std::uint8_t> malformed = generated.bytes;
        if (mutation.wide)
            putU64(malformed, mutation.offset, mutation.value);
        else
            putU32(malformed, mutation.offset,
                   static_cast<std::uint32_t>(mutation.value));
        ReaderFixture invalid = makeReader(malformed);
        const bool invalidAccepted =
            invalid.reader->captureDataStorageSection();
        t.expect(!invalidAccepted
                     && invalid.reader->rawDataStorageSections() == 0u
                     && invalid.reader->parsedDataStorageRecords() == 0u
                     && invalid.reader->parsedDataStorageRecord(solidHandle)
                            == nullptr,
                 std::string("malformed Blob01 chain is rejected atomically: ")
                     + mutation.name);
    }
}

void testOwnedUnsupportedObjectJournalEvent(TestContext& t) {
    dwgRW parent("");
    std::vector<std::uint8_t> inputBytes{0u};
    auto readerBuffer = std::unique_ptr<dwgBuffer>(
        new dwgBuffer(inputBytes.data(), inputBytes.size()));
    ModelerDispatchReader reader(std::move(readerBuffer), &parent);

    DRW_UnsupportedObject raw;
    raw.m_isEntity = true;
    raw.m_handle = 0xC01u;
    raw.m_rawBytes.assign(256u * 1024u, 0xA5u);
    const std::uint8_t* expectedFrame = raw.m_rawBytes.data();
    const std::size_t expectedFrameSize = raw.m_rawBytes.size();

    ModelerDispatchSink sink;
    sink.expectedRawEntityFrame = expectedFrame;
    sink.expectedRawEntityFrameSize = expectedFrameSize;
    bool firstDelivered = false;
    const bool secondDelivered = reader.replayOwnedUnsupportedObjectTwice(
        std::move(raw), sink, firstDelivered);
    t.expect(firstDelivered && !secondDelivered && raw.m_rawBytes.empty()
                 && sink.rawEntityFrameMatchesExpected
                 && sink.rawEntityCallbacks.size() == 1u
                 && sink.rawEntityCallbacks.front().m_rawBytes.size()
                        == expectedFrameSize,
             "raw entity journal event forwards to the legacy callback once without copying its frame");

    DRW_UnsupportedObject moveRaw;
    moveRaw.m_isEntity = true;
    moveRaw.m_handle = 0xC02u;
    moveRaw.m_rawBytes.assign(256u * 1024u, 0x5Au);
    const std::uint8_t* expectedMoveFrame = moveRaw.m_rawBytes.data();
    const std::size_t expectedMoveFrameSize = moveRaw.m_rawBytes.size();
    MoveAwareUnsupportedObjectSink moveSink;
    moveSink.expectedRawEntityFrame = expectedMoveFrame;
    moveSink.expectedRawEntityFrameSize = expectedMoveFrameSize;
    bool firstMoveDelivered = false;
    const bool secondMoveDelivered = reader.replayOwnedUnsupportedObjectTwice(
        std::move(moveRaw), moveSink, firstMoveDelivered);
    t.expect(firstMoveDelivered && !secondMoveDelivered
                 && moveRaw.m_rawBytes.empty()
                 && moveSink.rawEntityFrameMatchesExpected
                 && moveSink.rawEntityMoveCallbacks.size() == 1u
                 && moveSink.rawEntityMoveCallbacks.front().m_handle == 0xC02u
                 && moveSink.rawEntityMoveCallbacks.front().m_rawBytes.size()
                        == expectedMoveFrameSize
                 && moveSink.rawEntityMoveCallbacks.front().m_rawBytes.data()
                        == expectedMoveFrame
                 && moveSink.rawEntityMoveCallbacks.front().m_rawBytes.front()
                        == 0x5Au
                 && moveSink.rawEntityMoveCallbacks.front().m_rawBytes.back()
                        == 0x5Au,
             "move-aware raw-object callback retains the event allocation exactly once");
}

void testR2004DataPageLogicalTail(TestContext& t) {
    constexpr std::uint32_t sectionId = 7u;
    constexpr std::uint64_t firstPageAddress = 0x100u;
    constexpr std::uint64_t secondPageAddress = 0x140u;
    constexpr std::uint64_t pageCapacity = 64u;
    const std::array<std::uint8_t, 7> firstLiterals{{
        'A', 'B', 'C', 'D', 'E', 'F', 'G'}};
    const std::array<std::uint8_t, 7> secondLiterals{{
        'W', 'X', 'Y', 'Z', '1', '2', '3'}};
    const auto firstExpected = expectedR2004PageOutput(firstLiterals);
    const auto secondExpected = expectedR2004PageOutput(secondLiterals);
    const auto makeFile = [firstPageAddress, secondPageAddress, sectionId,
                           pageCapacity, firstLiterals, secondLiterals]() {
        std::vector<std::uint8_t> bytes(0x180u, 0xccu);
        const auto first = makeR2004DataPageVector(
            firstPageAddress, sectionId, 0u, firstLiterals);
        const auto second = makeR2004DataPageVector(
            secondPageAddress, sectionId,
            static_cast<std::uint32_t>(pageCapacity), secondLiterals);
        std::copy(first.begin(), first.end(),
                  bytes.begin() + static_cast<std::ptrdiff_t>(firstPageAddress));
        std::copy(second.begin(), second.end(),
                  bytes.begin() + static_cast<std::ptrdiff_t>(secondPageAddress));
        return bytes;
    };
    const auto makeSection = [sectionId, pageCapacity, firstPageAddress,
                              secondPageAddress](std::uint64_t logicalSize) {
        dwgSectionInfo section;
        section.Id = static_cast<std::int32_t>(sectionId);
        section.name = "AcDb:AcDbObjects";
        section.compressed = 2u;
        section.maxSize = pageCapacity;
        section.size = logicalSize;
        dwgPageInfo first(1u, firstPageAddress, pageCapacity);
        first.dataSize = 12u;
        first.startOffset = 0u;
        dwgPageInfo second(2u, secondPageAddress, pageCapacity);
        second.dataSize = 12u;
        second.startOffset = pageCapacity;
        section.pages.emplace(1u, first);
        section.pages.emplace(2u, second);
        section.pageCount = section.pages.size();
        return section;
    };

    std::vector<std::uint8_t> bytes = makeFile();
    const std::vector<std::uint8_t> validBytes = bytes;
    DataPageDecodeProbe reader(bytes);
    dwgSectionInfo shortTailSection = makeSection(65u);
    std::unique_ptr<std::uint8_t[]> decoded(new std::uint8_t[4u]);
    std::fill_n(decoded.get(), 4u, std::uint8_t{0x5au});
    std::uint8_t* const priorDecoded = decoded.get();
    std::uint64_t logicalSize = 777u;

    // Corrupt a stored literal without resealing the checksum. The parser must
    // leave its caller's previously-published result intact on a later-page
    // failure, and the same production reader must accept the repaired bytes.
    bytes[static_cast<std::size_t>(secondPageAddress) + 33u] ^= 0x01u;
    const bool malformedRejected = !reader.decode(
        shortTailSection, decoded, logicalSize);
    const bool outputPreserved = decoded.get() == priorDecoded
        && logicalSize == 777u
        && std::all_of(decoded.get(), decoded.get() + 4u,
                       [](std::uint8_t value) { return value == 0x5au; });
    const bool checksumReported = std::any_of(
        reader.diagnostics().begin(), reader.diagnostics().end(),
        [](const DwgIntegrityDiagnostic& diagnostic) {
            return diagnostic.phase == DwgIntegrityPhase::DataPage
                && diagnostic.kind == DwgIntegrityCheckKind::DataPageChecksum;
        });
    t.expect(malformedRejected && outputPreserved && checksumReported,
             "production R2004 page decode keeps the prior section on checksum failure");

    std::copy(validBytes.begin(), validBytes.end(), bytes.begin());
    const bool shortTailDecoded = reader.decode(
        shortTailSection, decoded, logicalSize);
    bool shortTailMatches = shortTailDecoded && logicalSize == 65u
        && decoded[64u] == secondExpected[0];
    if (shortTailDecoded) {
        for (std::size_t index = 0; index < pageCapacity; ++index)
            shortTailMatches = shortTailMatches
                && decoded[index] == firstExpected[index];
    }
    t.expect(shortTailMatches,
             "production R2004 decode publishes only the one-byte logical prefix of a full final page");

    // Two written pages account for 128 bytes, while the declared section is
    // 129 bytes. The final all-zero page is omitted and must remain zero in the
    // checked rounded section allocation rather than being mistaken for EOF.
    dwgSectionInfo omittedZeroTailSection = makeSection(129u);
    std::unique_ptr<std::uint8_t[]> omittedZeroTail;
    std::uint64_t omittedLogicalSize = 0u;
    std::uint64_t expectedCapacity = 0u;
    const bool expectedCapacityValid = dwgSafety::sectionBufferCapacity(
        omittedZeroTailSection.size, omittedZeroTailSection.pages.size(),
        pageCapacity, expectedCapacity);
    const bool omittedZeroTailDecoded = reader.decode(
        omittedZeroTailSection, omittedZeroTail, omittedLogicalSize);
    bool omittedPagesMatch = omittedZeroTailDecoded
        && omittedLogicalSize == 129u && expectedCapacityValid
        && expectedCapacity == 192u && omittedZeroTail[128u] == 0u;
    if (omittedZeroTailDecoded) {
        for (std::size_t index = 0; index < pageCapacity; ++index) {
            omittedPagesMatch = omittedPagesMatch
                && omittedZeroTail[index] == firstExpected[index]
                && omittedZeroTail[pageCapacity + index] == secondExpected[index];
        }
    }
    t.expect(omittedPagesMatch,
             "production R2004 decode covers declared size beyond written-page estimate and preserves omitted zero tail");
}

void testR2007CrcLiteralVectors(TestContext& t) {
    struct CrcVector {
        std::size_t size;
        std::uint64_t mirrored;
        std::uint64_t normal;
    };
    static constexpr std::array<CrcVector, 10> vectors {{
        {0u, 0x0000000000000000ULL, 0xffffffffffffffffULL},
        {1u, 0x0000000000000000ULL, 0xffffffffffffffffULL},
        {2u, 0xb32e4cbe03a75f6fULL, 0xbd0f1e145615c96cULL},
        {3u, 0xa06d4fc5f4905156ULL, 0xd51b1643ffa54dd0ULL},
        {4u, 0xc9ef7bddf440aa8aULL, 0x50f47eaca94df860ULL},
        {5u, 0xe51201f91a06376eULL, 0xa772a52bd58fe24dULL},
        {6u, 0x2f6f865be4460e56ULL, 0x57381a1b4e42eacdULL},
        {7u, 0x1902e574e114a7e6ULL, 0x5c2e66695f08eaabULL},
        {8u, 0xe7072a532e324a3fULL, 0x5bcf3d8b641c9387ULL},
        {9u, 0xba315d0ff4133996ULL, 0x2ee8359452a5cd8dULL}
    }};
    static constexpr std::array<std::uint8_t, 9> bytes {{
        0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u
    }};

    for (const auto& vector : vectors) {
        const auto expectedMirrored = r2007CrcBitwiseReference(
            0u, bytes.data(), vector.size, true);
        const auto expectedNormal = r2007CrcBitwiseReference(
            0u, bytes.data(), vector.size, false);
        t.expect(expectedMirrored == vector.mirrored,
                 "ODA literal mirrored CRC vector length "
                     + std::to_string(vector.size));
        t.expect(expectedNormal == vector.normal,
                 "ODA literal normal CRC vector length "
                     + std::to_string(vector.size));
        t.expect(dwgUtil::crc64Mirrored(0u, bytes.data(), vector.size)
                     == vector.mirrored,
                 "production mirrored CRC matches literal length "
                     + std::to_string(vector.size));
        t.expect(dwgUtil::crc64Normal(0u, bytes.data(), vector.size)
                     == vector.normal,
                 "production normal CRC matches literal length "
                     + std::to_string(vector.size));
    }

    struct SeedVector {
        std::uint64_t seed;
        std::uint64_t size;
        std::uint64_t expected;
    };
    static constexpr std::array<SeedVector, 5> seedVectors {{
        {0u, 0u, 0xe1d87185ffd9613cULL},
        {0u, 1u, 0x37dc097cffd61d3fULL},
        {0u, 8u, 0x91f5313dffbf4154ULL},
        {0x123456789abcdef0ULL, 0u, 0x40230a1108083e0cULL},
        {0x123456789abcdef0ULL, 300u, 0x0c19200904349190ULL}
    }};
    for (const auto& vector : seedVectors) {
        t.expect(r2007UpdateSeed1Reference(vector.seed, vector.size)
                     == vector.expected,
                 "independent ODA UpdateSeed1 recurrence length "
                     + std::to_string(vector.size));
        t.expect(dwgUtil::updateSeed1(vector.seed, vector.size)
                     == vector.expected,
                 "ODA UpdateSeed1 literal vector length "
                     + std::to_string(vector.size));
    }
}

void testR2007DataPageProfiles(TestContext& t) {
    const auto align32 = [](std::uint64_t value, std::uint64_t& aligned) {
        std::uint64_t adjusted = 0;
        if (!dwgSafety::add(value, 31u, adjusted))
            return false;
        aligned = adjusted & ~std::uint64_t{31u};
        return true;
    };
    const auto makePattern = [](std::size_t size, std::uint8_t seed) {
        std::vector<std::uint8_t> data(size);
        for (std::size_t i = 0; i < size; ++i)
            data[i] = static_cast<std::uint8_t>(seed + i * 37u + (i >> 2u));
        return data;
    };
    const auto makeRawPage = [&](const std::vector<std::uint8_t>& payload) {
        std::uint64_t physicalSize = 0;
        const bool aligned = align32(payload.size(), physicalSize);
        if (!aligned || physicalSize > std::numeric_limits<std::size_t>::max())
            return std::vector<std::uint8_t>{};
        std::vector<std::uint8_t> page(
            static_cast<std::size_t>(physicalSize), 0u);
        std::copy(payload.begin(), payload.end(), page.begin());
        return page;
    };
    const auto makeRsPage = [&](const std::vector<std::uint8_t>& payload,
                                std::uint64_t encoding) {
        std::vector<std::uint8_t> page;
        std::uint64_t alignedSize = 0;
        std::uint64_t numerator = 0;
        std::uint64_t blockCount = 0;
        std::uint64_t paddedSize = 0;
        std::uint64_t encodedSize = 0;
        std::uint64_t physicalSize = 0;
        if (payload.empty()
            || !dwgSafety::alignUp8(payload.size(), alignedSize)
            || !dwgSafety::add(alignedSize, 250u, numerator))
            return page;
        blockCount = numerator / 251u;
        if (blockCount == 0
            || !dwgSafety::multiply(blockCount, 251u, paddedSize)
            || !dwgSafety::multiply(blockCount, 255u, encodedSize)
            || !align32(encodedSize, physicalSize)
            || paddedSize > std::numeric_limits<std::size_t>::max()
            || encodedSize > std::numeric_limits<std::size_t>::max()
            || physicalSize > std::numeric_limits<std::size_t>::max())
            return page;
        std::vector<std::uint8_t> padded(
            static_cast<std::size_t>(paddedSize), 0u);
        std::copy(payload.begin(), payload.end(), padded.begin());
        page.resize(static_cast<std::size_t>(physicalSize), 0u);
        const bool encoded = encoding == 1u
            ? dwgRSCodec::encode251(padded.data(), page.data(),
                                    static_cast<std::uint32_t>(blockCount))
            : encoding == 4u
                ? dwgRSCodec::encode251I(
                      padded.data(), page.data(),
                      static_cast<std::uint32_t>(blockCount))
                : false;
        if (!encoded)
            page.clear();
        return page;
    };
    const auto setChecksums = [](dwgPageInfo& page,
                                 const std::vector<std::uint8_t>& compressed,
                                 const std::vector<std::uint8_t>& logical) {
        page.crc = dwgUtil::crc64Mirrored(
            dwgUtil::updateSeed1(0u, compressed.size()), compressed.data(),
            compressed.size());
        page.checksum = dwgUtil::checksum21(0u, logical.data(),
                                            logical.size());
    };
    const auto copyPage = [](std::vector<std::uint8_t>& file,
                             std::uint64_t address,
                             const std::vector<std::uint8_t>& page) {
        if (address > file.size()
            || page.size() > file.size() - static_cast<std::size_t>(address))
            return false;
        std::copy(page.begin(), page.end(),
                  file.begin() + static_cast<std::ptrdiff_t>(address));
        return true;
    };

    constexpr std::uint64_t firstAddress = 0x100u;
    const std::vector<std::uint8_t> firstLogical = makePattern(300u, 0x19u);
    const std::vector<std::uint8_t> secondLogical = makePattern(300u, 0x83u);
    const std::vector<std::uint8_t> rawPage = makeRawPage(firstLogical);
    t.expect(!rawPage.empty(), "AC1021 verbatim page vector is generated");
    if (rawPage.empty())
        return;

    for (const std::uint64_t encoding : {1u, 4u}) {
        const std::vector<std::uint8_t> encodedSecond =
            makeRsPage(secondLogical, encoding);
        t.expect(!encodedSecond.empty(),
                 "AC1021 RS page vector is generated for section encoding "+
                     std::to_string(encoding));
        if (encodedSecond.empty())
            continue;
        const std::uint64_t secondAddress = firstAddress + rawPage.size();
        std::vector<std::uint8_t> file(
            static_cast<std::size_t>(secondAddress + encodedSecond.size()),
            0xccu);
        const bool pagesCopied = copyPage(file, firstAddress, rawPage)
            && copyPage(file, secondAddress, encodedSecond);
        t.expect(pagesCopied,
                 "AC1021 raw and RS page bytes fit their generated file");
        if (!pagesCopied)
            continue;

        dwgSectionInfo section;
        section.Id = 4;
        section.name = "AcDb:AcDbObjects";
        section.compressed = encoding;
        section.maxSize = firstLogical.size();
        section.size = firstLogical.size() + secondLogical.size();
        dwgPageInfo raw(1u, firstAddress, rawPage.size());
        raw.dataSize = rawPage.size();
        raw.startOffset = 0u;
        raw.cSize = firstLogical.size();
        raw.uSize = firstLogical.size();
        setChecksums(raw, firstLogical, firstLogical);
        dwgPageInfo rs(2u, secondAddress, encodedSecond.size());
        rs.dataSize = encodedSecond.size();
        rs.startOffset = firstLogical.size();
        rs.cSize = secondLogical.size();
        rs.uSize = secondLogical.size();
        setChecksums(rs, secondLogical, secondLogical);
        section.pages.emplace(raw.Id, raw);
        section.pages.emplace(rs.Id, rs);
        section.pageCount = section.pages.size();

        R21DataPageDecodeProbe reader(file);
        std::vector<std::uint8_t> decoded(
            static_cast<std::size_t>(section.size), 0xa5u);
        const bool accepted = reader.decode(section, decoded.data());
        bool exact = accepted;
        if (accepted) {
            exact = std::equal(firstLogical.begin(), firstLogical.end(),
                               decoded.begin())
                && std::equal(secondLogical.begin(), secondLogical.end(),
                              decoded.begin() + static_cast<std::ptrdiff_t>(
                                  firstLogical.size()));
        }
        t.expect(exact,
                 "AC1021 decoder distinguishes verbatim and RS-stored pages, preserves bytes and alignment (encoding "+
                     std::to_string(encoding)+")");
        t.expect(reader.diagnostics().empty(),
                 "valid AC1021 stored-page profiles have no integrity warnings (encoding "+
                     std::to_string(encoding)+")");
    }

    std::vector<std::uint8_t> paddedRawPage = rawPage;
    paddedRawPage.resize(rawPage.size() + 96u, 0xe7u);
    constexpr std::uint64_t paddedRawAddress = 0x100u;
    std::vector<std::uint8_t> paddedRawFile(
        static_cast<std::size_t>(paddedRawAddress + paddedRawPage.size()),
        0xccu);
    const bool paddedRawCopied = copyPage(
        paddedRawFile, paddedRawAddress, paddedRawPage);
    t.expect(paddedRawCopied,
             "AC1021 verbatim page with additional 32-byte padding is generated");
    if (paddedRawCopied) {
        dwgSectionInfo section;
        section.Id = 4;
        section.name = "AcDb:AppInfo";
        section.compressed = 1u;
        section.maxSize = firstLogical.size();
        section.size = firstLogical.size();
        dwgPageInfo page(1u, paddedRawAddress, paddedRawPage.size());
        page.dataSize = paddedRawPage.size();
        page.startOffset = 0u;
        page.cSize = firstLogical.size();
        page.uSize = firstLogical.size();
        setChecksums(page, firstLogical, firstLogical);
        page.crc ^= 1u;
        section.pages.emplace(page.Id, page);
        section.pageCount = section.pages.size();
        R21DataPageDecodeProbe reader(paddedRawFile);
        std::vector<std::uint8_t> decoded(firstLogical.size(), 0xa5u);
        const bool accepted = reader.decode(section, decoded.data());
        const bool crcWarning = reader.diagnostics().size() == 1u
            && reader.diagnostics().front().kind
                == DwgIntegrityCheckKind::DataPageCrc
            && reader.diagnostics().front().severity
                == DwgIntegritySeverity::Warning;
        t.expect(accepted && decoded == firstLogical
                     && crcWarning,
                 "AC1021 verbatim page with physical tail padding follows its page profile and reports a CRC mismatch without corrupting payload bytes");

        dwgSectionInfo noCrcSection = section;
        noCrcSection.pages.at(page.Id).crc = 0u;
        R21DataPageDecodeProbe noCrcReader(paddedRawFile);
        std::vector<std::uint8_t> noCrcOutput(firstLogical.size(), 0xa5u);
        const bool noCrcAccepted = noCrcReader.decode(
            noCrcSection, noCrcOutput.data());
        t.expect(noCrcAccepted && noCrcOutput == firstLogical
                     && noCrcReader.diagnostics().empty(),
                 "AC1021 unambiguous verbatim profile does not require a CRC field for transform classification");
    }

    const std::vector<std::uint8_t> compressedLogical{
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H'};
    const std::vector<std::uint8_t> compressed{
        0x00u, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 0x87u, 0x00u};
    std::array<std::uint8_t, 16> directCompressedOutput{};
    dwgCompressor compressor;
    const bool compressedVectorValid = compressor.decompress21(
        compressed.data(), directCompressedOutput.data(), compressed.size(),
        directCompressedOutput.size())
        && std::equal(compressedLogical.begin(), compressedLogical.end(),
                      directCompressedOutput.begin());
    t.expect(compressedVectorValid,
             "source-authored AC1021 compressed-page stream expands to its fixed logical bytes");
    if (compressedVectorValid) {
        const std::vector<std::uint8_t> encoded = makeRsPage(compressed, 4u);
        constexpr std::uint64_t address = 0x100u;
        std::vector<std::uint8_t> file(
            static_cast<std::size_t>(address + encoded.size()), 0xccu);
        const bool copied = !encoded.empty() && copyPage(file, address, encoded);
        t.expect(copied, "AC1021 compressed RS page vector is generated");
        if (copied) {
            dwgSectionInfo section;
            section.Id = 4;
            section.name = "AcDb:AcDbObjects";
            section.compressed = 4u;
            section.maxSize = compressedLogical.size();
            section.size = compressedLogical.size();
            dwgPageInfo page(1u, address, encoded.size());
            page.dataSize = encoded.size();
            page.startOffset = 0u;
            page.cSize = compressed.size();
            page.uSize = compressedLogical.size();
            setChecksums(page, compressed, compressedLogical);
            section.pages.emplace(page.Id, page);
            section.pageCount = section.pages.size();
            R21DataPageDecodeProbe reader(file);
            std::vector<std::uint8_t> decoded(compressedLogical.size(), 0xa5u);
            const bool decodedPage = reader.decode(section, decoded.data());
            t.expect(decodedPage
                         && std::equal(compressedLogical.begin(),
                                       compressedLogical.end(), decoded.begin())
                         && reader.diagnostics().empty(),
                     "AC1021 decoder RS-decodes before LZ77 decompression and matches fixed logical bytes");
        }

        const std::vector<std::uint8_t> verbatimCompressed =
            makeRawPage(compressed);
        constexpr std::uint64_t verbatimAddress = 0x100u;
        std::vector<std::uint8_t> verbatimFile(
            static_cast<std::size_t>(verbatimAddress + verbatimCompressed.size()),
            0xccu);
        const bool verbatimCopied = !verbatimCompressed.empty()
            && copyPage(verbatimFile, verbatimAddress, verbatimCompressed);
        t.expect(verbatimCopied,
                 "AC1021 unencoded compressed-page negative vector is generated");
        if (verbatimCopied) {
            dwgSectionInfo section;
            section.Id = 4;
            section.name = "AcDb:AcDbObjects";
            section.compressed = 4u;
            section.maxSize = compressedLogical.size();
            section.size = compressedLogical.size();
            dwgPageInfo page(1u, verbatimAddress, verbatimCompressed.size());
            page.dataSize = verbatimCompressed.size();
            page.startOffset = 0u;
            page.cSize = compressed.size();
            page.uSize = compressedLogical.size();
            setChecksums(page, compressed, compressedLogical);
            section.pages.emplace(page.Id, page);
            section.pageCount = section.pages.size();
            R21DataPageDecodeProbe reader(verbatimFile);
            std::vector<std::uint8_t> output(compressedLogical.size(), 0x5au);
            const bool rejected = !reader.decode(section, output.data());
            const bool outputUnchanged = std::all_of(
                output.begin(), output.end(),
                [](std::uint8_t value) { return value == 0x5au; });
            const bool geometryDiagnostic = std::any_of(
                reader.diagnostics().begin(), reader.diagnostics().end(),
                [](const DwgIntegrityDiagnostic& diagnostic) {
                    return diagnostic.phase == DwgIntegrityPhase::DataPage
                        && diagnostic.kind == DwgIntegrityCheckKind::PageGeometry;
                });
            t.expect(rejected && outputUnchanged && geometryDiagnostic,
                     "AC1021 compressed page without its independent RS transform fails closed");
        }
    }

    // cSize=237 collides geometrically: align32(cSize) and align32(RS(cSize))
    // are both 256 bytes.  Compression is nevertheless followed by RS per
    // ODA §5.4, so a mismatching CRC is a warning, not a transform selector.
    std::vector<std::uint8_t> compressedCollision(237u, 0u);
    compressedCollision[0] = 0u;
    for (std::size_t i = 0u; i < 8u; ++i)
        compressedCollision[i + 1u] = static_cast<std::uint8_t>('A' + i);
    for (std::size_t i = 0u; i < 29u; ++i) {
        compressedCollision[9u + i * 2u] = 0x87u;
        compressedCollision[10u + i * 2u] = 0u;
    }
    std::vector<std::uint8_t> compressedCollisionLogical(248u, 0u);
    for (std::size_t i = 0u; i < 240u; ++i)
        compressedCollisionLogical[i] = static_cast<std::uint8_t>('A' + i % 8u);
    std::vector<std::uint8_t> decodedCollisionLogical(248u, 0u);
    dwgCompressor collisionCompressor;
    const bool collisionStreamValid = collisionCompressor.decompress21(
        compressedCollision.data(), decodedCollisionLogical.data(),
        compressedCollision.size(), decodedCollisionLogical.size())
        && decodedCollisionLogical == compressedCollisionLogical;
    t.expect(collisionStreamValid,
             "generated AC1021 compressed collision stream has the declared logical size");
    const std::vector<std::uint8_t> encodedCollision =
        makeRsPage(compressedCollision, 4u);
    constexpr std::uint64_t compressedCollisionAddress = 0x100u;
    std::vector<std::uint8_t> compressedCollisionFile(
        static_cast<std::size_t>(compressedCollisionAddress
                                 + encodedCollision.size()), 0xccu);
    const bool compressedCollisionCopied = collisionStreamValid
        && !encodedCollision.empty()
        && copyPage(compressedCollisionFile, compressedCollisionAddress,
                    encodedCollision);
    t.expect(compressedCollisionCopied,
             "AC1021 compressed page with colliding raw/RS extent is generated");
    if (compressedCollisionCopied) {
        dwgSectionInfo section;
        section.Id = 4u;
        section.name = "AcDb:AcDbObjects";
        section.compressed = 4u;
        section.maxSize = compressedCollisionLogical.size();
        section.size = compressedCollisionLogical.size();
        dwgPageInfo page(1u, compressedCollisionAddress,
                         encodedCollision.size());
        page.dataSize = encodedCollision.size();
        page.startOffset = 0u;
        page.cSize = compressedCollision.size();
        page.uSize = compressedCollisionLogical.size();
        setChecksums(page, compressedCollision, compressedCollisionLogical);
        page.crc ^= 1u;
        section.pages.emplace(page.Id, page);
        section.pageCount = section.pages.size();

        R21DataPageDecodeProbe reader(compressedCollisionFile);
        std::vector<std::uint8_t> decoded(compressedCollisionLogical.size(),
                                          0x5au);
        const bool accepted = reader.decode(section, decoded.data());
        const bool crcWarning = reader.diagnostics().size() == 1u
            && reader.diagnostics().front().kind
                == DwgIntegrityCheckKind::DataPageCrc
            && reader.diagnostics().front().severity
                == DwgIntegritySeverity::Warning;
        t.expect(accepted && decoded == compressedCollisionLogical
                     && crcWarning,
                 "AC1021 compressed extent collision selects mandatory RS before advisory CRC validation");
    }

    constexpr std::uint64_t ambiguousAddress = 0x100u;
    constexpr std::uint64_t retryAddress = 0x300u;
    const std::vector<std::uint8_t> ambiguousLogical = makePattern(232u, 0x37u);
    const std::vector<std::uint8_t> retryLogical = makePattern(300u, 0xd1u);
    const std::vector<std::uint8_t> ambiguousPage = makeRawPage(ambiguousLogical);
    const std::vector<std::uint8_t> retryPage = makeRawPage(retryLogical);
    std::vector<std::uint8_t> retryFile(0x500u, 0xccu);
    const bool retryBytesCopied = !ambiguousPage.empty() && !retryPage.empty()
        && copyPage(retryFile, ambiguousAddress, ambiguousPage)
        && copyPage(retryFile, retryAddress, retryPage);
    t.expect(retryBytesCopied,
             "ambiguous and retry AC1021 pages fit the generated file");
    if (!retryBytesCopied)
        return;

    dwgSectionInfo ambiguousSection;
    ambiguousSection.Id = 4;
    ambiguousSection.name = "AcDb:AcDbObjects";
    ambiguousSection.compressed = 1u;
    ambiguousSection.maxSize = ambiguousLogical.size();
    ambiguousSection.size = ambiguousLogical.size();
    dwgPageInfo ambiguous(1u, ambiguousAddress, ambiguousPage.size());
    ambiguous.dataSize = ambiguousPage.size();
    ambiguous.startOffset = 0u;
    ambiguous.cSize = ambiguousLogical.size();
    ambiguous.uSize = ambiguousLogical.size();
    setChecksums(ambiguous, ambiguousLogical, ambiguousLogical);
    ambiguousSection.pages.emplace(ambiguous.Id, ambiguous);
    ambiguousSection.pageCount = ambiguousSection.pages.size();

    R21DataPageDecodeProbe retryReader(retryFile);
    std::vector<std::uint8_t> collisionOutput(ambiguousLogical.size(), 0x5au);
    const bool collisionAccepted =
        retryReader.decode(ambiguousSection, collisionOutput.data());
    t.expect(collisionAccepted && collisionOutput == ambiguousLogical
                 && retryReader.diagnostics().empty(),
             "AC1021 raw/RS page-size collision is resolved by the matching CRC and identical prefix");

    dwgSectionInfo noCrcSection = ambiguousSection;
    noCrcSection.pages.at(ambiguous.Id).crc = 0u;
    std::vector<std::uint8_t> priorOutput(ambiguousLogical.size(), 0x5au);
    const bool ambiguousRejected = !retryReader.decode(
        noCrcSection, priorOutput.data());
    const bool priorOutputPreserved = std::all_of(
        priorOutput.begin(), priorOutput.end(),
        [](std::uint8_t value) { return value == 0x5au; });
    const bool geometryDiagnostic = std::any_of(
        retryReader.diagnostics().begin(), retryReader.diagnostics().end(),
        [](const DwgIntegrityDiagnostic& diagnostic) {
            return diagnostic.phase == DwgIntegrityPhase::DataPage
                && diagnostic.kind == DwgIntegrityCheckKind::PageGeometry;
        });
    t.expect(ambiguousRejected && priorOutputPreserved && geometryDiagnostic,
             "AC1021 ambiguous page without CRC evidence fails closed without publishing bytes");

    const std::vector<std::uint8_t> ambiguousRsPage =
        makeRsPage(ambiguousLogical, 1u);
    constexpr std::uint64_t ambiguousRsAddress = 0x200u;
    const bool ambiguousRsCopied = !ambiguousRsPage.empty()
        && copyPage(retryFile, ambiguousRsAddress, ambiguousRsPage);
    t.expect(ambiguousRsCopied,
             "AC1021 RS page with colliding raw extent is generated");
    if (ambiguousRsCopied) {
        dwgSectionInfo ambiguousRsSection;
        ambiguousRsSection.Id = 4;
        ambiguousRsSection.name = "AcDb:AcDbObjects";
        ambiguousRsSection.compressed = 1u;
        ambiguousRsSection.maxSize = ambiguousLogical.size();
        ambiguousRsSection.size = ambiguousLogical.size();
        dwgPageInfo ambiguousRs(3u, ambiguousRsAddress,
                                ambiguousRsPage.size());
        ambiguousRs.dataSize = ambiguousRsPage.size();
        ambiguousRs.startOffset = 0u;
        ambiguousRs.cSize = ambiguousLogical.size();
        ambiguousRs.uSize = ambiguousLogical.size();
        setChecksums(ambiguousRs, ambiguousLogical, ambiguousLogical);
        ambiguousRsSection.pages.emplace(ambiguousRs.Id, ambiguousRs);
        ambiguousRsSection.pageCount = ambiguousRsSection.pages.size();
        std::vector<std::uint8_t> ambiguousRsOutput(
            ambiguousLogical.size(), 0u);
        const bool ambiguousRsAccepted = retryReader.decode(
            ambiguousRsSection, ambiguousRsOutput.data());
        t.expect(ambiguousRsAccepted
                     && ambiguousRsOutput == ambiguousLogical,
                 "AC1021 RS/raw page-size collision is resolved by an RS-valid equal payload prefix");
    }

    dwgSectionInfo retrySection;
    retrySection.Id = 4;
    retrySection.name = "AcDb:AcDbObjects";
    retrySection.compressed = 1u;
    retrySection.maxSize = retryLogical.size();
    retrySection.size = retryLogical.size();
    dwgPageInfo retry(2u, retryAddress, retryPage.size());
    retry.dataSize = retryPage.size();
    retry.startOffset = 0u;
    retry.cSize = retryLogical.size();
    retry.uSize = retryLogical.size();
    setChecksums(retry, retryLogical, retryLogical);
    retrySection.pages.emplace(retry.Id, retry);
    retrySection.pageCount = retrySection.pages.size();
    std::vector<std::uint8_t> retryOutput(retryLogical.size(), 0u);
    const bool retryAccepted = retryReader.decode(retrySection, retryOutput.data());
    t.expect(retryAccepted && retryOutput == retryLogical,
             "same AC1021 reader accepts a valid profile after ambiguous-page refusal");
}

bool readR2007TestU64(const std::vector<std::uint8_t>& bytes,
                      std::size_t offset, std::uint64_t& value) {
    if (offset > bytes.size() || bytes.size() - offset < 8u)
        return false;
    value = 0u;
    for (unsigned int index = 0u; index < 8u; ++index)
        value |= static_cast<std::uint64_t>(bytes[offset + index])
            << (index * 8u);
    return true;
}

bool writeR2007TestU64(std::vector<std::uint8_t>& bytes,
                       std::size_t offset, std::uint64_t value) {
    if (offset > bytes.size() || bytes.size() - offset < 8u)
        return false;
    for (unsigned int index = 0u; index < 8u; ++index)
        bytes[offset + index] =
            static_cast<std::uint8_t>(value >> (index * 8u));
    return true;
}

struct R2007TestMapHeader {
    std::vector<std::uint8_t> decodedRs;
    std::vector<std::uint8_t> data;
    std::uint64_t fileSize{0u};
    std::uint64_t pageMapOffset{0u};
    std::uint64_t pageMapId{0u};
    std::uint64_t pageMapCompressedSize{0u};
    std::uint64_t pageMapUncompressedSize{0u};
    std::uint64_t pageMapCount{0u};
    std::uint64_t pageMapMaxId{0u};
    std::uint64_t pageMapCrcSeedEncoded{0u};
    std::uint64_t pageMapCrcCompressed{0u};
    std::uint64_t pageMapCrcUncompressed{0u};
    std::uint64_t pageMapCorrectionFactor{0u};
    std::uint64_t sectionMapId{0u};
    std::uint64_t sectionMapCompressedSize{0u};
    std::uint64_t sectionMapUncompressedSize{0u};
    std::uint64_t sectionMapCrcSeedEncoded{0u};
    std::uint64_t sectionMapCrcCompressed{0u};
    std::uint64_t sectionMapCrcUncompressed{0u};
    std::uint64_t sectionMapCorrectionFactor{0u};
};

bool decodeR2007TestMapHeader(const std::vector<std::uint8_t>& file,
                              R2007TestMapHeader& header) {
    constexpr std::size_t fileHeaderOffset = 0x80u;
    constexpr std::size_t encodedHeaderSize = 0x2fdu;
    constexpr std::size_t decodedHeaderSize = 3u * 239u;
    constexpr std::size_t headerDataSize = 0x110u;
    constexpr std::size_t headerDataOffset = 32u;
    if (file.size() < fileHeaderOffset + encodedHeaderSize)
        return false;

    header.decodedRs.assign(decodedHeaderSize, 0u);
    std::vector<std::uint8_t> encodedHeader(
        file.begin() + static_cast<std::ptrdiff_t>(fileHeaderOffset),
        file.begin() + static_cast<std::ptrdiff_t>(
            fileHeaderOffset + encodedHeaderSize));
    if (!dwgRSCodec::decode239I(
            encodedHeader.data(), header.decodedRs.data(), 3u))
        return false;
    std::uint32_t encodedDataSize = 0u;
    if (!readLe32(header.decodedRs, 24u, encodedDataSize)
        || static_cast<std::int32_t>(encodedDataSize)
            != -static_cast<std::int32_t>(headerDataSize))
        return false;
    header.data.assign(
        header.decodedRs.begin() + static_cast<std::ptrdiff_t>(headerDataOffset),
        header.decodedRs.begin()
            + static_cast<std::ptrdiff_t>(headerDataOffset + headerDataSize));

    return readR2007TestU64(header.data, 8u, header.fileSize)
        && readR2007TestU64(header.data, 24u,
                            header.pageMapCorrectionFactor)
        && readR2007TestU64(header.data, 32u,
                            header.pageMapCrcSeedEncoded)
        && readR2007TestU64(header.data, 56u, header.pageMapOffset)
        && readR2007TestU64(header.data, 64u, header.pageMapId)
        && readR2007TestU64(header.data, 80u,
                            header.pageMapCompressedSize)
        && readR2007TestU64(header.data, 88u,
                            header.pageMapUncompressedSize)
        && readR2007TestU64(header.data, 96u, header.pageMapCount)
        && readR2007TestU64(header.data, 104u, header.pageMapMaxId)
        && readR2007TestU64(header.data, 128u,
                            header.pageMapCrcUncompressed)
        && readR2007TestU64(header.data, 16u,
                            header.pageMapCrcCompressed)
        && readR2007TestU64(header.data, 168u,
                            header.sectionMapCrcUncompressed)
        && readR2007TestU64(header.data, 176u,
                            header.sectionMapCompressedSize)
        && readR2007TestU64(header.data, 192u, header.sectionMapId)
        && readR2007TestU64(header.data, 200u,
                            header.sectionMapUncompressedSize)
        && readR2007TestU64(header.data, 208u,
                            header.sectionMapCrcCompressed)
        && readR2007TestU64(header.data, 216u,
                            header.sectionMapCorrectionFactor)
        && readR2007TestU64(header.data, 224u,
                            header.sectionMapCrcSeedEncoded);
}

bool resealR2007TestMapHeader(std::vector<std::uint8_t>& file,
                              R2007TestMapHeader& header) {
    constexpr std::size_t fileHeaderOffset = 0x80u;
    constexpr std::size_t encodedHeaderSize = 0x2fdu;
    constexpr std::size_t decodedHeaderSize = 3u * 239u;
    constexpr std::size_t headerDataOffset = 32u;
    if (header.data.size() != 0x110u
        || header.decodedRs.size() != decodedHeaderSize
        || file.size() < fileHeaderOffset + encodedHeaderSize)
        return false;

    if (!writeR2007TestU64(header.data, 264u, 0u))
        return false;
    const std::uint64_t headerCrc = dwgUtil::crc64Normal(
        dwgUtil::updateSeed2(0u, header.data.size()), header.data.data(),
        header.data.size());
    if (!writeR2007TestU64(header.data, 264u, headerCrc))
        return false;
    std::copy(header.data.begin(), header.data.end(),
              header.decodedRs.begin()
                  + static_cast<std::ptrdiff_t>(headerDataOffset));
    const std::uint64_t payloadCrc = dwgUtil::crc64Normal(
        dwgUtil::updateSeed1(0u, header.data.size()), header.data.data(),
        header.data.size());
    if (!writeR2007TestU64(header.decodedRs, 16u, payloadCrc))
        return false;

    std::vector<std::uint8_t> encoded(encodedHeaderSize, 0u);
    if (!dwgRSCodec::encode239I(header.decodedRs.data(), encoded.data(), 3u))
        return false;
    std::copy(encoded.begin(), encoded.end(),
              file.begin() + static_cast<std::ptrdiff_t>(fileHeaderOffset));
    return true;
}

bool encodeR2007TestSystemPagePayload(
    std::vector<std::uint8_t>& file, const dwgPageInfo& page,
    std::uint64_t compressedSize, std::uint64_t correctionFactor,
    const std::vector<std::uint8_t>& payload) {
    if (payload.empty() || compressedSize != payload.size()
        || page.address > file.size() || page.size > file.size() - page.address)
        return false;
    std::uint64_t alignedSize = 0u;
    std::uint64_t repeatedDataSize = 0u;
    std::uint64_t scaledSize = 0u;
    std::uint64_t chunkNumerator = 0u;
    if (!dwgSafety::alignUp8(payload.size(), alignedSize)
        || !dwgSafety::multiply(alignedSize, correctionFactor,
                                repeatedDataSize)
        || !dwgSafety::add(repeatedDataSize, 238u, chunkNumerator))
        return false;
    scaledSize = chunkNumerator / 239u;
    if (scaledSize == 0u
        || scaledSize > std::numeric_limits<std::uint32_t>::max())
        return false;
    std::uint64_t encodedSize = 0u;
    std::uint64_t rsInputSize = 0u;
    if (!dwgSafety::multiply(scaledSize, 255u, encodedSize)
        || !dwgSafety::multiply(scaledSize, 239u, rsInputSize)
        || encodedSize > page.size
        || rsInputSize > std::numeric_limits<std::size_t>::max())
        return false;

    std::vector<std::uint8_t> alignedPayload(
        static_cast<std::size_t>(alignedSize), 0u);
    std::copy(payload.begin(), payload.end(), alignedPayload.begin());
    std::vector<std::uint8_t> rsInput(static_cast<std::size_t>(rsInputSize), 0u);
    for (std::uint64_t offset = 0u; offset < repeatedDataSize;
         offset += alignedSize) {
        std::copy(alignedPayload.begin(), alignedPayload.end(),
                  rsInput.begin() + static_cast<std::ptrdiff_t>(offset));
    }
    std::vector<std::uint8_t> encoded(static_cast<std::size_t>(encodedSize), 0u);
    if (!dwgRSCodec::encode239I(
            rsInput.data(), encoded.data(),
            static_cast<std::uint32_t>(scaledSize)))
        return false;
    std::copy(encoded.begin(), encoded.end(),
              file.begin() + static_cast<std::ptrdiff_t>(page.address));
    return true;
}

struct R2007SectionExtentMutation {
    std::uint64_t pageId{0u};
    std::uint64_t sectionSize{0u};
    std::uint64_t pageOffset{0u};
    std::uint64_t invalidUncompressedSize{0u};
};

bool makeOutOfRangeR2007SectionPageExtent(
    std::vector<std::uint8_t>& file, R2007SectionExtentMutation& mutation) {
    constexpr std::uint64_t pageMapBaseOffset = 0x480u;
    R2007TestMapHeader header;
    if (!decodeR2007TestMapHeader(file, header)
        || header.fileSize != file.size())
        return false;
    R21DataPageDecodeProbe decoder(file);
    std::vector<std::uint8_t> pageMapData;
    std::uint64_t pageMapAddress = 0u;
    if (!dwgSafety::add(pageMapBaseOffset, header.pageMapOffset,
                        pageMapAddress)
        || !decoder.decodePageMapSystemPage(
            header.pageMapCompressedSize, header.pageMapUncompressedSize,
            header.pageMapCorrectionFactor, pageMapAddress,
            dwgUtil::decodeCrcSeed(header.pageMapCrcSeedEncoded),
            header.pageMapCrcCompressed, header.pageMapCrcUncompressed,
            pageMapData))
        return false;

    std::unordered_map<std::uint64_t, dwgPageInfo> pageMap;
    if (!decoder.parsePageMapEntries(
            pageMapData.data(), pageMapData.size(), header.pageMapCount,
            header.pageMapMaxId, header.fileSize, pageMap))
        return false;
    const auto sectionMapPage = pageMap.find(header.sectionMapId);
    if (sectionMapPage == pageMap.end())
        return false;

    std::vector<std::uint8_t> sectionMapData;
    if (!decoder.decodePageMapSystemPage(
            header.sectionMapCompressedSize,
            header.sectionMapUncompressedSize,
            header.sectionMapCorrectionFactor, sectionMapPage->second.address,
            dwgUtil::decodeCrcSeed(header.sectionMapCrcSeedEncoded),
            header.sectionMapCrcCompressed,
            header.sectionMapCrcUncompressed, sectionMapData,
            DwgIntegrityPhase::SectionMap))
        return false;

    std::size_t descriptorOffset = 0u;
    bool mutated = false;
    while (descriptorOffset <= sectionMapData.size()
           && sectionMapData.size() - descriptorOffset >= 64u) {
        const auto* record = sectionMapData.data() + descriptorOffset;
        if (std::all_of(record, record + 64u,
                        [](std::uint8_t byte) { return byte == 0u; }))
            break;
        std::uint64_t dataSize = 0u;
        std::uint64_t nameLength = 0u;
        std::uint64_t pageCount = 0u;
        if (!readR2007TestU64(sectionMapData, descriptorOffset, dataSize)
            || !readR2007TestU64(sectionMapData, descriptorOffset + 32u,
                                 nameLength)
            || !readR2007TestU64(sectionMapData, descriptorOffset + 56u,
                                 pageCount)
            || nameLength > sectionMapData.size() - descriptorOffset - 64u)
            return false;
        const std::uint64_t pageRecordsOffset = descriptorOffset + 64u
            + nameLength;
        std::uint64_t pageRecordsSize = 0u;
        if (!dwgSafety::multiply(pageCount, 56u, pageRecordsSize)
            || pageRecordsOffset > sectionMapData.size()
            || pageRecordsSize > sectionMapData.size() - pageRecordsOffset)
            return false;
        for (std::uint64_t pageIndex = 0u;
             pageIndex < pageCount && !mutated; ++pageIndex) {
            const std::uint64_t pageRecordOffset = pageRecordsOffset
                + pageIndex * 56u;
            std::uint64_t pageOffset = 0u;
            std::uint64_t pageId = 0u;
            if (!readR2007TestU64(sectionMapData,
                                 static_cast<std::size_t>(pageRecordOffset),
                                 pageOffset)
                || !readR2007TestU64(
                    sectionMapData,
                    static_cast<std::size_t>(pageRecordOffset + 16u), pageId)
                || pageOffset >= dataSize || dataSize == 0u)
                continue;
            const std::uint64_t invalidSize = dataSize - pageOffset + 1u;
            if (!writeR2007TestU64(
                    sectionMapData,
                    static_cast<std::size_t>(pageRecordOffset + 24u),
                    invalidSize))
                return false;
            mutation = {pageId, dataSize, pageOffset, invalidSize};
            mutated = true;
        }
        descriptorOffset = static_cast<std::size_t>(
            pageRecordsOffset + pageRecordsSize);
    }
    if (!mutated)
        return false;

    const std::uint64_t sectionMapCrc = dwgUtil::crc64Mirrored(
        dwgUtil::updateSeed1(
            dwgUtil::decodeCrcSeed(header.sectionMapCrcSeedEncoded),
            sectionMapData.size()),
        sectionMapData.data(), sectionMapData.size());
    if (!writeR2007TestU64(header.data, 168u, sectionMapCrc)
        || !writeR2007TestU64(header.data, 208u, sectionMapCrc)
        || !encodeR2007TestSystemPagePayload(
            file, sectionMapPage->second,
            header.sectionMapCompressedSize,
            header.sectionMapCorrectionFactor, sectionMapData))
        return false;
    return resealR2007TestMapHeader(file, header);
}

void testR2007SectionMapPageExtent(TestContext& t) {
    std::stringbuf streamBuffer(std::ios::out | std::ios::binary);
    std::ofstream output;
    static_cast<std::ostream&>(output).rdbuf(&streamBuffer);
    DRW_Header header;
    header.addInt("$MEASUREMENT", 1, 70);
    DRW_Line line;
    line.basePoint = {2, 3, 5};
    line.secPoint = {7, 11, 13};
    dwgWriter21 writer(&output, &header);
    const bool generated = writer.prepareDwgClassManifest()
        && writer.writeFileHeaderStub() && writer.writeDwgHeader()
        && writer.writeDwgClasses() && writer.writeDwgObjects()
        && writer.encodeEntity(&line) && writer.emitDeferredBlockControl()
        && writer.finalizeHeaderHandseed() && writer.writeDwgHandles()
        && writer.writeSecondHeader() && writer.finalize();
    t.expect(generated,
             "complete AC1021 section-map witness is generated in memory");
    if (!generated)
        return;

    const std::string encoded = streamBuffer.str();
    std::vector<std::uint8_t> valid(encoded.begin(), encoded.end());
    R21DataPageDecodeProbe validReader(valid);
    t.expect(validReader.readSections(),
             "generated AC1021 witness has a valid file header and section map");

    std::vector<std::uint8_t> invalid = valid;
    R2007SectionExtentMutation mutation;
    const bool changed = makeOutOfRangeR2007SectionPageExtent(invalid, mutation);
    t.expect(changed && mutation.invalidUncompressedSize
                 > mutation.sectionSize - mutation.pageOffset,
             "generated AC1021 section-map page extent crosses its declared section size");
    if (!changed)
        return;
    const std::vector<std::uint8_t> publicInvalid = invalid;

    R21DataPageDecodeProbe reader(invalid);
    const bool rejected = !reader.readSections();
    const bool exactFailure = std::any_of(
        reader.diagnostics().begin(), reader.diagnostics().end(),
        [&mutation](const DwgIntegrityDiagnostic& diagnostic) {
            return diagnostic.phase == DwgIntegrityPhase::SectionMap
                && diagnostic.kind == DwgIntegrityCheckKind::PageRange
                && diagnostic.severity == DwgIntegritySeverity::Error
                && diagnostic.hasPageId
                && diagnostic.pageId == mutation.pageId;
        });
    const bool noChecksumFailure = std::none_of(
        reader.diagnostics().begin(), reader.diagnostics().end(),
        [](const DwgIntegrityDiagnostic& diagnostic) {
            return diagnostic.kind == DwgIntegrityCheckKind::FileHeaderCrc
                || diagnostic.kind == DwgIntegrityCheckKind::SystemPageCrc
                || diagnostic.kind
                    == DwgIntegrityCheckKind::SystemPageUncompressedCrc;
        });
    t.expect(rejected && exactFailure && noChecksumFailure
                 && reader.knownSections() == 0u
                 && reader.unknownSections() == 0u,
             "out-of-range AC1021 section-map page is rejected before publishing descriptors with valid enclosing CRCs");
    std::copy(valid.begin(), valid.end(), invalid.begin());
    t.expect(reader.readSections() && reader.knownSections() > 0u
                 && reader.unknownSections() == 0u,
             "same AC1021 reader recovers after the original section map is restored");

    SectionMapSink sink;
    dwgRW publicReader("");
    const bool publicRejected = !publicReader.readBuffer(
        publicInvalid.data(), publicInvalid.size(), &sink, false);
    t.expect(publicRejected && sink.headers == 0u && sink.lines == 0u,
             "public AC1021 read refuses an out-of-range section page without callbacks");
    t.expect(publicReader.readBuffer(valid.data(), valid.size(), &sink, false)
                 && sink.headers == 1u && sink.lines == 1u
                 && sink.end.x == 7 && sink.end.y == 11 && sink.end.z == 13,
             "same public AC1021 reader accepts the corrected map and publishes the generated LINE");
}

template<class Writer>
std::vector<std::uint8_t> makeGeneratedAuxHeaderFile() {
    std::stringbuf streamBuffer(std::ios::out | std::ios::binary);
    std::ofstream output;
    static_cast<std::ostream&>(output).rdbuf(&streamBuffer);
    DRW_Header header;
    header.addInt("$MEASUREMENT", 1, 70);
    DRW_Line line;
    line.basePoint = {2, 3, 5};
    line.secPoint = {7, 11, 13};
    Writer writer(&output, &header);
    const bool generated = writer.prepareDwgClassManifest()
        && writer.writeFileHeaderStub() && writer.writeDwgHeader()
        && writer.writeDwgClasses() && writer.writeDwgObjects()
        && writer.encodeEntity(&line) && writer.emitDeferredBlockControl()
        && writer.finalizeHeaderHandseed() && writer.writeDwgHandles()
        && writer.writeSecondHeader() && writer.finalize();
    if (!generated)
        return {};
    const std::string encoded = streamBuffer.str();
    return std::vector<std::uint8_t>(encoded.begin(), encoded.end());
}

void testR2018AuxHeaderTrailer(TestContext& t) {
    std::vector<std::uint8_t> ac1027File =
        makeGeneratedAuxHeaderFile<dwgWriter27>();
    std::vector<std::uint8_t> ac1032File =
        makeGeneratedAuxHeaderFile<dwgWriter32>();
    t.expect(!ac1027File.empty() && !ac1032File.empty(),
             "AC1027 and AC1032 AuxHeader witnesses are generated in memory");
    if (ac1027File.empty() || ac1032File.empty())
        return;

    AuxHeaderSectionProbe<dwgReader27> ac1027Reader(ac1027File,
                                                     DRW::AC1027);
    AuxHeaderSectionProbe<dwgReader32> ac1032Reader(ac1032File,
                                                     DRW::AC1032);
    std::vector<std::uint8_t> ac1027AuxHeader;
    std::vector<std::uint8_t> ac1032AuxHeader;
    const bool ac1027Decoded = ac1027Reader.decode(ac1027AuxHeader);
    const bool ac1032Decoded = ac1032Reader.decode(ac1032AuxHeader);
    t.expect(ac1027Decoded && ac1032Decoded,
             "AC1027 and AC1032 generated AuxHeader sections decode through their versioned readers");
    if (!ac1027Decoded || !ac1032Decoded)
        return;

    const bool versionCodesMatch = ac1027AuxHeader.size() >= 5u
        && ac1032AuxHeader.size() >= 5u
        && ac1027AuxHeader[0] == 0xffu && ac1027AuxHeader[1] == 0x77u
        && ac1027AuxHeader[2] == 0x01u
        && ac1027AuxHeader[3] == 31u && ac1027AuxHeader[4] == 0u
        && ac1032AuxHeader[0] == 0xffu && ac1032AuxHeader[1] == 0x77u
        && ac1032AuxHeader[2] == 0x01u
        && ac1032AuxHeader[3] == 33u && ac1032AuxHeader[4] == 0u;
    t.expect(versionCodesMatch,
             "AuxHeader version words match ODA AC1027=31 and AC1032=33");

    const bool trailerMatches = ac1032AuxHeader.size()
            == ac1027AuxHeader.size() + 6u
        && std::all_of(ac1032AuxHeader.end() - 6,
                       ac1032AuxHeader.end(),
                       [](std::uint8_t value) { return value == 0u; });
    t.expect(trailerMatches,
             "R2018 AuxHeader appends exactly three zero int16 values after the AC1027 payload");
}

void testR2004SectionMapPublication(TestContext& t) {
    std::stringbuf streamBuffer(std::ios::out | std::ios::binary);
    std::ofstream output;
    static_cast<std::ostream&>(output).rdbuf(&streamBuffer);
    DRW_Header header;
    header.addInt("$MEASUREMENT", 1, 70);
    DRW_Line line;
    line.basePoint = {2, 3, 5};
    line.secPoint = {7, 11, 13};
    dwgWriter18 writer(&output, &header);
    const bool generated = writer.prepareDwgClassManifest()
        && writer.writeFileHeaderStub() && writer.writeDwgHeader()
        && writer.writeDwgClasses() && writer.writeDwgObjects()
        && writer.encodeEntity(&line) && writer.emitDeferredBlockControl()
        && writer.finalizeHeaderHandseed() && writer.writeDwgHandles()
        && writer.writeSecondHeader() && writer.finalize();
    t.expect(generated, "complete AC1018 section-map witness is generated in memory");
    if (!generated)
        return;

    const auto encoded = streamBuffer.str();
    const std::vector<std::uint8_t> valid(encoded.begin(), encoded.end());

    std::vector<std::uint8_t> overflowingOffset = valid;
    const bool offsetMutated =
        makeOverflowingR2004SectionPageMapOffset(overflowingOffset);
    t.expect(offsetMutated,
             "generated R2004 file header accepts a scratch page-map-offset mutation");
    if (offsetMutated) {
        const std::vector<std::uint8_t> publicOverflowingOffset =
            overflowingOffset;
        dwgRW offsetDiagnosticOwner("");
        SectionMapPublicationProbe offsetProbe(overflowingOffset,
                                                &offsetDiagnosticOwner);
        const bool offsetRejected = !offsetProbe.parse();
        const auto offsetDiagnostics = offsetProbe.diagnostics();
        const bool exactOffsetFailure = std::any_of(
            offsetDiagnostics.begin(), offsetDiagnostics.end(),
            [](const DwgIntegrityDiagnostic& entry) {
                return entry.phase == DwgIntegrityPhase::FileHeader
                    && entry.kind == DwgIntegrityCheckKind::PageRange
                    && entry.severity == DwgIntegritySeverity::Error;
            });
        const bool headerChecksumStayedValid = std::none_of(
            offsetDiagnostics.begin(), offsetDiagnostics.end(),
            [](const DwgIntegrityDiagnostic& entry) {
                return entry.phase == DwgIntegrityPhase::FileHeader
                    && entry.kind == DwgIntegrityCheckKind::FileHeaderCrc;
            });
        t.expect(offsetRejected && exactOffsetFailure
                     && headerChecksumStayedValid
                     && offsetProbe.knownSections() == 0u
                     && offsetProbe.unknownSections() == 0u,
                 "overflowing R2004 section-page-map offset is rejected before descriptor publication with a valid header CRC");
        std::copy(valid.begin(), valid.end(), overflowingOffset.begin());
        t.expect(offsetProbe.parse() && offsetProbe.knownSections() > 0u
                     && offsetProbe.unknownSections() == 0u,
                 "same R2004 reader retries a corrected file after page-map offset overflow refusal");

        SectionMapSink offsetSink;
        dwgRW offsetPublicReader("");
        t.expect(!offsetPublicReader.readBuffer(
                     publicOverflowingOffset.data(),
                     publicOverflowingOffset.size(),
                     &offsetSink, false)
                     && offsetSink.headers == 0u && offsetSink.lines == 0u,
                 "public R2004 read refuses an overflowing page-map offset without publishing callbacks");
        const auto publicOffsetDiagnostics =
            offsetPublicReader.getIntegrityDiagnostics();
        t.expect(std::any_of(
                     publicOffsetDiagnostics.begin(),
                     publicOffsetDiagnostics.end(),
                     [](const DwgIntegrityDiagnostic& entry) {
                         return entry.phase == DwgIntegrityPhase::FileHeader
                             && entry.kind == DwgIntegrityCheckKind::PageRange;
                     }),
                 "public read preserves the file-header page-map overflow diagnostic");
        t.expect(offsetPublicReader.readBuffer(
                     valid.data(), valid.size(), &offsetSink, false)
                     && offsetSink.headers == 1u && offsetSink.lines == 1u,
                 "same public DWG reader accepts a corrected file after page-map offset overflow refusal");
    }

    std::vector<std::uint8_t> invalid = valid;
    t.expect(makeDuplicateUnknownR2004SectionMap(invalid),
             "two generated R2004 descriptors receive an exact duplicate unknown name with a resealed DSM page");
    if (invalid == valid)
        return;
    const std::vector<std::uint8_t> publicInvalid = invalid;

    dwgRW diagnosticOwner("");
    SectionMapPublicationProbe probe(invalid, &diagnosticOwner);
    const bool rejected = !probe.parse();
    const auto diagnostics = probe.diagnostics();
    const bool exactFailure = std::any_of(
        diagnostics.begin(), diagnostics.end(), [](const DwgIntegrityDiagnostic& entry) {
            return entry.phase == DwgIntegrityPhase::SectionMap
                && entry.kind == DwgIntegrityCheckKind::SectionPageReference
                && entry.severity == DwgIntegritySeverity::Error;
        });
    const bool checksumsStayedValid = std::none_of(
        diagnostics.begin(), diagnostics.end(), [](const DwgIntegrityDiagnostic& entry) {
            return (entry.phase == DwgIntegrityPhase::FileHeader
                    && entry.kind == DwgIntegrityCheckKind::FileHeaderCrc)
                || entry.kind == DwgIntegrityCheckKind::SystemPageChecksum;
        });
    t.expect(rejected && exactFailure && checksumsStayedValid,
             "duplicate-name failure reaches the real SectionMap stage after valid envelope and checksums");
    t.expect(probe.knownSections() == 0u && probe.unknownSections() == 0u,
             "failed full-map admission publishes neither standard nor unknown section descriptors");

    std::copy(valid.begin(), valid.end(), invalid.begin());
    t.expect(probe.parse() && probe.knownSections() > 0u
                 && probe.unknownSections() == 0u,
             "the same reader instance retries a corrected full map and publishes the valid descriptors");

    SectionMapSink sink;
    dwgRW publicReader("");
    t.expect(!publicReader.readBuffer(
                 publicInvalid.data(), publicInvalid.size(), &sink, false)
                 && sink.headers == 0u && sink.lines == 0u,
             "public DWG read publishes no callbacks for a duplicate full section map");
    const auto publicDiagnostics = publicReader.getIntegrityDiagnostics();
    t.expect(std::any_of(publicDiagnostics.begin(), publicDiagnostics.end(),
                         [](const DwgIntegrityDiagnostic& entry) {
                             return entry.phase == DwgIntegrityPhase::SectionMap
                                 && entry.kind == DwgIntegrityCheckKind::SectionPageReference;
                         }),
             "public readBuffer preserves the reader-level duplicate-map diagnostic identity");
    t.expect(publicReader.readBuffer(valid.data(), valid.size(), &sink, false)
                 && sink.headers == 1u && sink.lines == 1u
                 && sink.end.x == 7 && sink.end.y == 11
                 && sink.end.z == 13,
             "public readBuffer recovers on the same facade after the corrected full map is supplied");
}

void appendAdmissionDiagnostic(std::ostream& out, const DRW_OperationDiagnostic& value) {
    out << static_cast<int>(value.operation) << ':';
    const auto append = [&out](const auto& entry) {
        out << static_cast<int>(entry.phase) << ':' << static_cast<int>(entry.cause)
            << ':' << std::quoted(entry.code) << ':' << std::quoted(entry.message)
            << ':' << entry.offset << ':' << entry.handle << ':' << entry.hasOffset
            << ':' << entry.hasHandle << ';';
    };
    append(value);
    out << value.secondary.size() << ':';
    for (const auto& entry : value.secondary) append(entry);
}

// Public snapshots plus physical prefix/suffix readback avoid a private-member
// macro or a new production test-access API at this lifecycle boundary.
std::string admissionState(dwgRW& owner) {
    std::ostringstream out;
    appendAdmissionDiagnostic(out, owner.getLastDiagnostic());
    const auto skip = owner.getWriteSkipCounters();
    out << owner.getError() << ':' << owner.getVersion() << ':' << std::quoted(owner.getCodePage())
        << ':' << owner.dwgReadObjectBudget() << ':' << owner.hasDwgWriteFailure()
        << ':' << owner.getSuccessfulEntityWriteCount() << ':' << owner.getLastSuccessfulEntityHandle()
        << ':' << skip.entityWrites << ':' << skip.tableRecordWrites << ':' << skip.objectWrites
        << ':' << skip.classRegistrations << ':' << skip.rawObjectWrites << ':' << skip.rawSectionWrites
        << ':' << skip.blockDefinitions << ':' << owner.getEntityParseFailures()
        << ':' << owner.getObjectParseFailures() << ':' << owner.getClassesCrcMismatch()
        << ':' << owner.getR2007CrcMismatch() << ':' << owner.getR2004CrcMismatch()
        << ':' << owner.getDataStorageLinkFailures() << ':' << owner.getDataStorageOrphanRecords()
        << ':' << owner.getDecodedProxyPrimitives() << ':' << owner.getIntegrityDiagnosticsDropped();
    for (const auto& diagnostic : owner.getIntegrityDiagnostics()) {
        out << '|' << diagnostic.version << ':' << static_cast<int>(diagnostic.severity)
            << ':' << static_cast<int>(diagnostic.offsetSpace) << ':' << static_cast<int>(diagnostic.phase)
            << ':' << static_cast<int>(diagnostic.kind) << ':' << diagnostic.logicalSectionId
            << ':' << diagnostic.sectionDescriptorId << ':' << std::quoted(diagnostic.sectionName)
            << ':' << diagnostic.pageId << ':' << diagnostic.fileOffset << ':' << diagnostic.logicalHandle
            << ':' << diagnostic.expected << ':' << diagnostic.observed << ':' << diagnostic.hasPageId
            << ':' << diagnostic.hasFileOffset << ':' << diagnostic.hasLogicalHandle
            << ':' << diagnostic.hasExpected << ':' << diagnostic.hasObserved;
    }
    const auto appendMap = [&out](const auto& values) {
        out << '|';
        for (const auto& entry : std::map<std::string, std::size_t>(values.begin(), values.end()))
            out << std::quoted(entry.first) << ':' << entry.second << ';';
    };
    appendMap(owner.getSkippedCustomClasses());
    appendMap(owner.getSkippedUnsupportedObjects());
    out << '|';
    for (const auto& name : owner.getLayerNameOrder()) out << std::quoted(name) << ';';
    out << '|';
    for (const auto& name : owner.getLtypeNameOrder()) out << std::quoted(name) << ';';
    std::vector<std::uint32_t> handles;
    out << '|' << owner.getEmittedDwgTableRecordHandles(handles) << ':';
    for (const auto handle : handles) out << handle << ';';
    DRW::DwgObjectFrameReceipt frame;
    out << '|' << owner.getLastDwgObjectFrame(frame) << ':' << frame.valid << ':' << frame.generation
        << ':' << frame.objectHandle << ':' << frame.version << ':' << frame.dataBitSize
        << ':' << frame.handleBitSize << ':' << frame.classNumber << ':' << frame.writerOperation
        << ':' << frame.admissionToken;
    for (const auto& occurrence : frame.occurrences) {
        out << '|' << occurrence.objectHandle << ':' << static_cast<int>(occurrence.stream)
            << ':' << static_cast<int>(occurrence.serializedSegment) << ':' << occurrence.segmentOrdinal
            << ':' << static_cast<int>(occurrence.token.code) << ':' << occurrence.token.reference
            << ':' << occurrence.token.startBit << ':' << occurrence.token.endBit;
    }
    return out.str();
}

class AdmissionInterface final : public dx_iface {
public:
    explicit AdmissionInterface(dwgRW* owner = nullptr) : codec(owner) {
        cData = &data;
        currentBlock = data.mBlock;
        layer.name = "ADMISSION_LAYER";
        app.name = "ADMISSION_APP";
        for (int i = 0; i < 2; ++i) {
            input[i].layer = layer.name;
            input[i].basePoint = {1.0 + i * 6, 2.0 + i * 6, 3.0 + i * 6};
            input[i].secPoint = {4.0 + i * 6, 5.0 + i * 6, 6.0 + i * 6};
        }
    }
    void addLine(const DRW_Line& line) override {
        lines.push_back(line);
        if (lines.size() == 1u && callback) callback();
    }
    void addHeader(const DRW_Header* value) override {
        dx_iface::addHeader(value);
        if (headerCallback) headerCallback();
    }
    void writeHeader(DRW_Header&) override {}
    void writeDwgClasses() override { ++phaseHooks; }
    void collectDwgAppIds() override { ++phaseHooks; }
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeLTypes() override {}
    void writeLayers() override { recordsOkay = codec->addLayer(&layer) && recordsOkay; }
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeDimstyles() override {}
    void writeObjects() override {}
    void writeAppId() override { recordsOkay = codec->addAppId(&app) && recordsOkay; }
    void writeEntities() override {
        ++writeCalls;
        const auto transaction = codec->beginDwgWriteTransaction();
        recordsOkay = transaction != 0 && recordsOkay;
        for (int i = 0; i < 2; ++i) {
            // Exercise provenance hooks without qualifying DataStorage payloads.
            recordsOkay = codec->setDwgObjectFrameProvenance(0, 0, 0) && recordsOkay;
            codec->clearDwgObjectFrameProvenance();
            recordsOkay = codec->writeLine(&input[i]) && recordsOkay;
            if (i == 0 && callback) callback();
        }
        recordsOkay = codec->commitDwgWriteTransaction(transaction) && recordsOkay;
    }
    dwgRW* codec;
    std::function<void()> callback;
    std::function<void()> headerCallback;
    std::vector<DRW_Line> lines;
    bool recordsOkay {true};
    int writeCalls {0}, phaseHooks {0};
private:
    dx_data data;
    DRW_Layer layer;
    DRW_AppId app;
    std::array<DRW_Line, 2> input;
};

bool admissionLinesMatch(const AdmissionInterface& sink) {
    return sink.lines.size() == 2u && sink.lines[0].basePoint.x == 1
        && sink.lines[0].secPoint.z == 6 && sink.lines[1].basePoint.x == 7
        && sink.lines[1].secPoint.z == 12 && sink.lines[0].layer == "ADMISSION_LAYER"
        && sink.lines[1].layer == "ADMISSION_LAYER";
}

void testDwgOperationAdmission(TestContext& t) {
    const auto root = std::filesystem::temp_directory_path()
        / ("libdxfrw-dwg-admission-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::error_code error;
    if (!std::filesystem::create_directory(root, error)) {
        t.expect(false, "DWG admission owns an exclusive scratch directory");
        return;
    }
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
    } cleanup{root};
    const auto bytes = [](const std::filesystem::path& path) {
        std::ifstream stream(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(stream), {});
    };
    for (const auto version : {DRW::AC1015, DRW::AC1027}) {
        const auto path = root / (std::to_string(static_cast<int>(version)) + ".dwg");
        const auto otherPath = root / "other.dwg";
        const std::string filename = path.u8string();
        dwgRW generator(filename.c_str());
        AdmissionInterface source(&generator);
        t.expect(generator.write(&source, version, true) && source.recordsOkay && source.phaseHooks == 2,
                 "DWG lifecycle source uses ordinary table, provenance and record hooks");
        const std::string original = bytes(path);
        const std::vector<std::uint8_t> buffer(original.begin(), original.end());
        const auto restore = [&](const std::filesystem::path& target) {
            std::ofstream stream(target, std::ios::binary | std::ios::trunc);
            stream.write(original.data(), static_cast<std::streamsize>(original.size()));
            return stream.good();
        };
        for (int outer = 0; outer < 3; ++outer) {
            for (int nested = 0; nested < 5; ++nested) {
                t.expect(restore(path), "DWG admission restores only its generated input");
                dwgRW owner(filename.c_str());
                AdmissionInterface sink(&owner);
                int attempts = 0;
                sink.callback = [&]() {
                    ++attempts;
                    const std::string state = admissionState(owner), destination = bytes(path);
                    AdmissionInterface rejected(&owner);
                    const bool accepted = nested == 0 ? owner.read(&rejected, true)
                        : nested == 1 ? owner.readBuffer(buffer.data(), buffer.size(), &rejected, true)
                        : nested == 2 ? owner.write(&rejected, version, false)
                        : nested == 3 ? owner.getPreview() : owner.testReader();
                    t.expect(!accepted && rejected.lines.empty() && rejected.writeCalls == 0,
                             "same DWG facade rejects each whole-operation entry before callbacks");
                    t.expect(!owner.read(nullptr, true)
                                 && !owner.readBuffer(nullptr, 6, nullptr, true)
                                 && !owner.write(nullptr, DRW::UNKNOWNV, false),
                             "DWG admission precedes nested argument and version validation");
                    t.expect(admissionState(owner) == state && bytes(path) == destination,
                             "nested DWG rejection preserves live results, diagnostics and destination");
                    t.expect(restore(otherPath), "different-facade source is independent of outer output");
                    const std::string otherName = otherPath.u8string();
                    dwgRW other(otherName.c_str());
                    AdmissionInterface independent(&other);
                    const bool otherOkay = nested == 0 ? other.read(&independent, false)
                        : nested == 2 ? other.write(&independent, version, true)
                        : nested == 1 ? other.readBuffer(buffer.data(), buffer.size(), &independent, false)
                        : nested == 3 ? other.getPreview() : other.testReader();
                    const bool otherResult = nested < 3
                        ? otherOkay && (nested == 2 ? independent.recordsOkay : admissionLinesMatch(independent))
                        : !otherOkay && other.getLastDiagnostic().operation == DRW::OperationKind::Read
                            && (nested == 3 ? other.getError() == DRW::BAD_READ_METADATA && other.getVersion() == version
                                            : other.getError() == DRW::BAD_NONE && other.getVersion() == DRW::UNKNOWNV);
                    t.expect(otherResult, "another DWG facade retains each entry's normal callback-time contract");
                    t.expect(admissionState(owner) == state && bytes(path) == destination,
                             "independent callback work does not change the outer DWG facade");
                };
                const bool okay = outer == 0 ? owner.read(&sink, false)
                    : outer == 1 ? owner.readBuffer(buffer.data(), buffer.size(), &sink, false)
                                 : owner.write(&sink, version, true);
                t.expect(okay && attempts == 1 && owner.getError() == DRW::BAD_NONE
                             && (outer == 2 ? sink.recordsOkay : admissionLinesMatch(sink)),
                         "DWG outer operation retains both records after rejected nesting");
                AdmissionInterface sequential(&owner);
                t.expect(owner.read(&sequential, false) && admissionLinesMatch(sequential),
                         "DWG operation releases admission for sequential file reuse");
            }
        }
        // Preview has no consumer callback in the current implementation.
        // Preserve its unsupported-preview result, not an invented outer matrix.
        dwgRW preview(filename.c_str());
        t.expect(!preview.getPreview() && preview.getError() == DRW::BAD_READ_METADATA,
                 "generated DWG retains the existing unsupported-preview contract");
        AdmissionInterface afterPreview(&preview);
        t.expect(preview.read(&afterPreview, false) && admissionLinesMatch(afterPreview),
                 "unsupported preview releases admission for a full read");
        dwgRW diagnosticReader(filename.c_str());
        t.expect(!diagnosticReader.testReader() && diagnosticReader.getError() == DRW::BAD_NONE,
                 "legacy diagnostic reader retains its false-without-parse-error result");
        AdmissionInterface afterDiagnostic(&diagnosticReader);
        t.expect(diagnosticReader.read(&afterDiagnostic, false) && admissionLinesMatch(afterDiagnostic),
                 "legacy diagnostic entry releases admission for a full read");

        for (int nested = 0; nested < 3; ++nested) {
            dwgR legacy(filename.c_str());
            AdmissionInterface sink;
            int attempts = 0;
            sink.callback = [&]() {
                ++attempts;
                std::ostringstream before;
                appendAdmissionDiagnostic(before, legacy.getLastDiagnostic());
                const auto errorBefore = legacy.getError();
                const auto versionBefore = legacy.getVersion();
                const auto codePage = legacy.getCodePage(), destination = bytes(path);
                AdmissionInterface rejected;
                const bool accepted = nested == 0 ? legacy.read(&rejected, true)
                    : nested == 1 ? legacy.getPreview() : legacy.testReader();
                std::ostringstream after;
                appendAdmissionDiagnostic(after, legacy.getLastDiagnostic());
                t.expect(!accepted && before.str() == after.str() && legacy.getError() == errorBefore
                             && legacy.getVersion() == versionBefore && legacy.getCodePage() == codePage
                             && bytes(path) == destination,
                         "deprecated DWG forwarding shares implementation admission exactly once");
            };
            t.expect(legacy.read(&sink, false) && attempts == 1 && admissionLinesMatch(sink),
                     "deprecated DWG facade continues its outer callbacks");
            sink.callback = {};
            sink.lines.clear();
            t.expect(legacy.read(&sink, false) && admissionLinesMatch(sink),
                     "deprecated DWG facade releases admission for sequential reuse");
        }
        dwgRW reusable(filename.c_str());
        AdmissionInterface reuse(&reusable);
        t.expect(!reusable.read(nullptr, false), "DWG null-interface read rejects");
        t.expect(reusable.read(&reuse, false), "DWG early read failure releases admission");
        t.expect(!reusable.readBuffer(nullptr, 6, &reuse, false), "DWG invalid-buffer read rejects");
        reuse.lines.clear();
        t.expect(reusable.readBuffer(buffer.data(), buffer.size(), &reuse, false) && admissionLinesMatch(reuse),
                 "DWG early buffer failure releases admission");
        t.expect(!reusable.write(&reuse, DRW::UNKNOWNV, true), "DWG unsupported write rejects");
        t.expect(reusable.write(&reuse, version, true) && reuse.recordsOkay,
                 "DWG early write failure releases admission");
        for (const bool memory : {false, true}) {
            reuse.lines.clear();
            reuse.headerCallback = []() { throw std::runtime_error("DWG header admission callback"); };
            bool caught = false;
            bool readOkay = true;
            try {
                readOkay = memory ? reusable.readBuffer(buffer.data(), buffer.size(), &reuse, false)
                                  : reusable.read(&reuse, false);
            } catch (const std::runtime_error&) { caught = true; }
            t.expect(!caught && !readOkay && reusable.getError() == DRW::BAD_READ_HEADER
                         && reusable.getLastDiagnostic().cause == DRW::OperationCause::CallbackException,
                     "DWG header callback retains existing contained-exception classification");
            reuse.headerCallback = {};
            reuse.lines.clear();
            t.expect(reusable.readBuffer(buffer.data(), buffer.size(), &reuse, false) && admissionLinesMatch(reuse),
                     "DWG read callback exception releases admission");
        }
        const std::string previous = bytes(path);
        reuse.callback = []() { throw std::runtime_error("DWG write admission callback"); };
        t.expect(!reusable.write(&reuse, version, true)
                     && reusable.getLastDiagnostic().cause == DRW::OperationCause::CallbackException
                     && bytes(path) == previous,
                 "DWG write callback failure retains classification and atomic destination");
        reuse.callback = {};
        t.expect(reusable.write(&reuse, version, true) && reuse.recordsOkay,
                 "DWG write catch releases admission for reuse");
        for (int entry = 0; entry < 3; ++entry) {
            const std::string missingName = (root / ("missing-" + std::to_string(entry) + ".dwg")).u8string();
            dwgRW missing(missingName.c_str());
            AdmissionInterface afterMissing(&missing);
            const bool opened = entry == 0 ? missing.read(&afterMissing, false)
                : entry == 1 ? missing.getPreview() : missing.testReader();
            t.expect(!opened && missing.getError() == DRW::BAD_OPEN,
                     "DWG file-backed entry preserves its missing-file failure");
            t.expect(missing.write(&afterMissing, version, true) && afterMissing.recordsOkay,
                     "DWG missing-file failure releases admission for subsequent output");
            std::error_code ignored;
            std::filesystem::remove(std::filesystem::u8path(missingName), ignored);
        }
    }
}

class HeaderSectionReader final : public dwgReader15 {
public:
    explicit HeaderSectionReader(std::vector<std::uint8_t>& bytes,
                                 std::uint32_t address = 0,
                                 DRW::Version headerVersion = DRW::AC1015,
                                 std::uint8_t appMaintenance = 0)
        : dwgReader15(std::make_unique<dwgBuffer>(bytes.data(), bytes.size()), nullptr) {
        version = headerVersion;
        appMaintenanceVersion = appMaintenance;
        decoder.setVersion(headerVersion, false);
        dwgSectionInfo section;
        section.Id = 0;
        section.size = bytes.size() - address;
        section.address = address;
        sections.emplace(secEnum::HEADER, section);
    }
    bool readDwgHeader(DRW_Header& hdr) override {
        const auto sectionIt = sections.find(secEnum::HEADER);
        if (sectionIt == sections.end() || sectionIt->second.Id < 0)
            return false;
        const auto& section = sectionIt->second;
        if (section.size == 0 || section.size > std::numeric_limits<int>::max()
            || !dwgSafety::range(section.address, section.size, fileBuf->size())
            || !fileBuf->setPosition(section.address))
            return false;
        std::vector<std::uint8_t> data(static_cast<std::size_t>(section.size));
        if (!fileBuf->getBytes(data.data(), data.size()))
            return false;
        dwgBuffer dataBuffer(data.data(), data.size(), &decoder);
        (void)checkSentinel(&dataBuffer, secEnum::HEADER, true);
        const auto addressSpace = version == DRW::AC1015
            ? DwgIntegrityAddressSpace::PhysicalFile
            : DwgIntegrityAddressSpace::DecodedBuffer;
        if (version > DRW::AC1018) {
            dwgBuffer handleBuffer(data.data(), data.size(), &decoder);
            return dwgReader::readDwgHeader(
                hdr, &dataBuffer, &handleBuffer, addressSpace, section.Id,
                section.address);
        }
        return dwgReader::readDwgHeader(
            hdr, &dataBuffer, &dataBuffer, addressSpace, section.Id,
            section.address);
    }
    const auto& diagnostics() const { return m_integrityDiagnostics; }
    std::size_t dropped() const { return m_integrityDiagnosticsDropped; }
    void fillDiagnostics() {
        for (std::size_t i = 0; i < dwgSafety::MaxIntegrityDiagnostics; ++i)
            addIntegrityDiagnostic({});
    }
};

class HeaderBodyProbe final : public DRW_Header {
public:
    using DRW_Header::parseDwg;
};

class HeaderWriteProbe final : public DRW_Header {
public:
    using DRW_Header::encodeDwg;
};

std::uint16_t headerTestCrc(const std::vector<std::uint8_t>& bytes,
                           std::size_t start, std::size_t end) {
    // Bit-wise CRC from ODA's reflected CRC16 table, independent of the
    // production writer/buffer checksum helpers (not a drawing oracle).
    std::uint16_t crc = 0xC0C1u;
    for (auto i = start; i < end; ++i) {
        crc ^= bytes[i];
        for (unsigned int bit = 0; bit < 8u; ++bit)
            crc = static_cast<std::uint16_t>((crc >> 1u)
                ^ ((crc & 1u) != 0u ? 0xA001u : 0u));
    }
    return crc;
}

void testBoundedFork(TestContext& t) {
    std::array<std::uint8_t, 4> bytes{{0xB5, 0x6A, 0xCC, 0xDD}};
    DRW_TextCodec codec;
    dwgBuffer parent(bytes.data(), bytes.size(), &codec);
    t.expect(parent.get3Bits() == 5u, "bounded fork source has a cached partial byte");
    auto bounded = parent.forkBounded(1u);
    t.expect(bounded.decoder == &codec && bounded.getBitPos() == 3u
                 && bounded.getPosition() == 0u && bounded.size() == 1u,
             "bounded fork preserves same-origin cursor/cache/decoder");
    unsigned int tail = 0;
    for (unsigned int i = 0; i < 5u; ++i)
        tail = (tail << 1u) | bounded.getBit();
    t.expect(tail == 21u && bounded.isGood() && bounded.getPosition() == 1u,
             "bounded fork reads remaining cached bits through its exact ceiling");
    (void)bounded.getBit();
    t.expect(!bounded.isGood() && parent.isGood() && parent.getBitPos() == 3u,
             "bounded stream exhaustion cannot read a following byte or move its parent");

    auto shared = parent.forkBounded(2u);
    auto copied = shared;
    copied.invalidate();
    t.expect(!parent.isGood() && !shared.isGood(),
             "related bounded/copy cursors intentionally share structural invalidation");
    dwgBuffer independentParent(bytes.data(), bytes.size());
    auto speculative = independentParent.forkIndependent().forkBounded(2u);
    speculative.invalidate();
    t.expect(independentParent.isGood(), "speculative independent fork isolates structural invalidation");

    auto nested = independentParent.forkBounded(2u).forkBounded(99u);
    t.expect(nested.size() == 2u && nested.getRawShort16() == 0x6AB5u,
             "nested ceilings intersect rather than expanding source access");
    t.expect(!nested.setPosition(3u) && !nested.isGood(),
             "bounded seek cannot escape the intersected ceiling");
    auto copySource = independentParent.forkBounded(2u);
    (void)copySource.get3Bits();
    auto cachedCopy = copySource;
    t.expect(cachedCopy.getBitPos() == 3u && cachedCopy.get2Bits() == 2u
                 && copySource.getBitPos() == 3u,
             "copy clones partial-byte state and cursor independently");
    auto excludedCache = copySource.forkBounded(0u);
    t.expect(!excludedCache.isGood() && excludedCache.getBit() == 0u,
             "a ceiling excluding the cached byte cannot expose its bits");
    auto mixed = independentParent.forkBounded(2u);
    (void)mixed.get3Bits();
    std::uint8_t mixedByte = 0;
    t.expect(mixed.getBytes(&mixedByte, 1u) && mixedByte == 0xABu
                 && mixed.getBitPos() == 3u && mixed.getPosition() == 1u,
             "raw byte reads preserve cached partial-byte alignment at a ceiling");
    unsigned int mixedTail = 0;
    for (unsigned int i = 0; i < 5u; ++i)
        mixedTail = (mixedTail << 1u) | mixed.getBit();
    t.expect(mixedTail == 10u && mixed.isGood(),
             "raw byte reads leave the remaining cached bits available");
    (void)mixed.getBit();
    t.expect(!mixed.isGood(), "mixed raw/bit reads cannot borrow a byte beyond the ceiling");

    std::array<std::uint8_t, 3> destination{{0xEE, 0xEE, 0xEE}};
    auto byteRead = independentParent.forkBounded(2u);
    t.expect(!byteRead.getBytes(destination.data(), destination.size())
                 && destination == std::array<std::uint8_t, 3>{{0xEE, 0xEE, 0xEE}},
             "out-of-window raw read does not expose even a partial prefix");
    auto crcRead = independentParent.forkBounded(2u);
    (void)crcRead.crc8(0xC0C1u, 0, 4);
    t.expect(!crcRead.isGood(), "CRC direct-pointer/fallback paths cannot escape a ceiling");
    auto validCrc = independentParent.forkBounded(2u);
    std::vector<std::uint8_t> prefix{bytes[0], bytes[1]};
    t.expect(validCrc.crc8(0xC0C1u, 0, 2) == headerTestCrc(prefix, 0, 2)
                 && validCrc.isGood() && validCrc.getPosition() == 0u,
             "bounded valid CRC preserves its source cursor");
    auto survivor = [&bytes]() {
        dwgBuffer temporary(bytes.data(), bytes.size());
        return temporary.forkBounded(2u);
    }();
    t.expect(survivor.getRawShort16() == 0x6AB5u && survivor.isGood(),
             "bounded fork owns its stream after parent expiry with backing storage alive");
    const std::vector<std::uint8_t> zeroCrc{{0xC1, 0xC0}};
    t.expect(headerTestCrc(zeroCrc, 0, 2) == 0u,
             "a calculated zero CRC is a real checksum value, not absence");
}

void testHeaderSectionBounds(TestContext& t) {
    DRW_Header source;
    source.addInt("LUNITS", 4, 70);
    dwgWriter15 writer(nullptr, &source);
    t.expect(writer.writeDwgHeader(), "full AC1015 HEADER generation succeeds");
    const auto original = writer.buffer();
    const std::uint32_t declared = original[16]
        | (std::uint32_t(original[17]) << 8u)
        | (std::uint32_t(original[18]) << 16u)
        | (std::uint32_t(original[19]) << 24u);
    const std::size_t crcOffset = 20u + declared;
    t.expect(crcOffset + 18u == original.size(),
             "HEADER RL delimits the complete payload before CRC and sentinel");
    t.expect(headerTestCrc(original, 16u, crcOffset)
                 == (original[crcOffset] | (std::uint16_t(original[crcOffset + 1u]) << 8u)),
             "literal bit-wise CRC confirms the generated full HEADER span");
    for (const std::string name : {"valid", "crc-flip", "crc-zero", "end-first",
            "end-zero", "begin-first", "truncated", "declared-zero",
            "declared-short", "declared-max", "declared-large"}) {
        auto bytes = original;
        if (name == "crc-flip") bytes[crcOffset] ^= 1u;
        if (name == "crc-zero") bytes[crcOffset] = bytes[crcOffset + 1u] = 0u;
        if (name == "end-first") bytes[crcOffset + 2u] ^= 1u;
        if (name == "end-zero") std::fill(bytes.begin() + crcOffset + 2u, bytes.end(), 0u);
        if (name == "begin-first") bytes[0] ^= 1u;
        if (name == "truncated") bytes.pop_back();
        std::uint32_t count = declared;
        if (name == "declared-zero") count = 0u;
        if (name == "declared-short") count -= 1u;
        if (name == "declared-max") count = 0xFFFFFFFFu;
        if (name == "declared-large") count += 1u;
        if (name.rfind("declared-", 0u) == 0u)
            for (std::size_t i = 0u; i < 4u; ++i)
                bytes[16u + i] = static_cast<std::uint8_t>(count >> (8u * i));
        HeaderSectionReader reader(bytes);
        DRW_Header header;
        header.addInt("stale", 99, 70);
        const bool accepted = reader.readDwgHeader(header);
        const bool warning = name == "crc-flip" || name == "crc-zero";
        const bool valid = name == "valid" || warning;
        t.expect(accepted == valid, "HEADER outcome: " + name);
        if (valid) {
            const auto entry = header.vars.find("LUNITS");
            t.expect(entry != header.vars.end() && entry->second->content.i == 4,
                     "bounded HEADER retains semantic values: " + name);
            t.expect(reader.diagnostics().size() == (warning ? 1u : 0u),
                     "HEADER checksum warnings remain observable: " + name);
            if (warning && !reader.diagnostics().empty())
                t.expect(reader.diagnostics()[0].severity == DwgIntegritySeverity::Warning,
                         "HEADER checksum corruption is not a structural failure");
        } else {
            t.expect(header.vars.empty(), "failed HEADER publishes no variables: " + name);
        }
    }

    // A canonical envelope is insufficient if the body itself is truncated.
    std::vector<std::uint8_t> shortBody(original.begin(), original.begin() + 36u);
    shortBody[16] = 16u;
    shortBody[17] = shortBody[18] = shortBody[19] = 0u;
    const auto shortCrc = headerTestCrc(shortBody, 16u, shortBody.size());
    shortBody.push_back(static_cast<std::uint8_t>(shortCrc));
    shortBody.push_back(static_cast<std::uint8_t>(shortCrc >> 8u));
    shortBody.insert(shortBody.end(), std::begin(dwgSentinels::HEADER_END),
                     std::end(dwgSentinels::HEADER_END));
    HeaderSectionReader shortReader(shortBody);
    DRW_Header shortHeader;
    t.expect(!shortReader.readDwgHeader(shortHeader) && shortHeader.vars.empty()
                 && shortReader.diagnostics().size() == 1u
                 && shortReader.diagnostics()[0].kind == DwgIntegrityCheckKind::HeaderSectionBounds,
             "valid CRC/sentinel cannot let a truncated body borrow trailer bytes");

    std::vector<std::uint8_t> compact(original.begin() + 16u, original.begin() + crcOffset);
    HeaderBodyProbe body;
    dwgBuffer compactBuffer(compact.data(), compact.size());
    t.expect(body.parseDwg(DRW::AC1015, &compactBuffer, &compactBuffer)
                 && body.vars.count("LUNITS") == 1u,
             "exact compact body-only HEADER remains readable without a trailer");
    compact.pop_back();
    dwgBuffer truncatedCompact(compact.data(), compact.size());
    t.expect(!body.parseDwg(DRW::AC1015, &truncatedCompact, &truncatedCompact)
                 && body.vars.empty(), "truncated compact data cannot bypass full-section admission");

    auto reuseBytes = original;
    HeaderSectionReader reuse(reuseBytes);
    DRW_Header reused;
    t.expect(reuse.readDwgHeader(reused), "same HEADER reader starts with valid data");
    reuseBytes[crcOffset + 2u] ^= 1u;
    t.expect(!reuse.readDwgHeader(reused) && reused.vars.empty(),
             "same HEADER reader clears rejected replacement data");
    std::copy(original.begin(), original.end(), reuseBytes.begin());
    t.expect(reuse.readDwgHeader(reused) && reused.vars.count("LUNITS") == 1u,
             "same HEADER reader can reread corrected data without stale parser state");

    std::vector<std::uint8_t> prefixed(64u, 0u);
    prefixed.insert(prefixed.end(), original.begin(), original.end());
    prefixed[64u + crcOffset] ^= 1u;
    HeaderSectionReader located(prefixed, 64u);
    DRW_Header locatedHeader;
    t.expect(located.readDwgHeader(locatedHeader) && located.diagnostics().size() == 1u,
             "physical HEADER checksum warning survives a nonzero section address");
    if (!located.diagnostics().empty()) {
        const auto& diagnostic = located.diagnostics()[0];
        t.expect(diagnostic.kind == DwgIntegrityCheckKind::HeaderSectionCrc
                     && diagnostic.phase == DwgIntegrityPhase::SectionParser
                     && diagnostic.offsetSpace == DwgIntegrityAddressSpace::PhysicalFile
                     && diagnostic.logicalSectionId == secEnum::HEADER
                     && diagnostic.sectionDescriptorId == 0
                     && diagnostic.hasFileOffset && diagnostic.fileOffset == 64u + crcOffset
                     && diagnostic.hasExpected && diagnostic.hasObserved,
                 "HEADER diagnostics bind exact section/address/value presence");
    }
    HeaderSectionReader saturated(prefixed, 64u);
    saturated.fillDiagnostics();
    DRW_Header saturatedHeader;
    t.expect(saturated.readDwgHeader(saturatedHeader) && saturated.dropped() == 1u
                 && saturated.diagnostics().size() == dwgSafety::MaxIntegrityDiagnostics,
             "dropped checksum diagnostics retain warning-only read behavior");

    // Choose the low 16 mantissa bits so this real generated section's CRC is
    // zero. Solve the CRC's affine 16x16 map from the base value and its bit
    // toggles; this remains fast and avoids depending on a stale magic double
    // when the HEADER handle stream legitimately changes.
    constexpr std::uint64_t baseBits = UINT64_C(0x400d8939b2fec9f5);
    const auto generatedHeaderCrc = [](std::uint64_t bits,
                                       std::vector<std::uint8_t>& result) {
        double value = 0;
        std::memcpy(&value, &bits, sizeof value);
        DRW_Header source;
        source.addInt("LUNITS", 4, 70);
        source.addDouble("USERR1", value, 40);
        dwgWriter15 writer(nullptr, &source);
        if (!writer.writeDwgHeader()) return false;
        result = writer.buffer();
        return true;
    };
    std::vector<std::uint8_t> baseBytes;
    const bool baseGenerated = generatedHeaderCrc(baseBits, baseBytes);
    t.expect(baseGenerated, "zero-CRC base HEADER generation succeeds");
    if (!baseGenerated) return;
    const auto baseCrcOffset = baseBytes.size() - 18u;
    const std::uint16_t baseCrc = headerTestCrc(baseBytes, 16u, baseCrcOffset);
    std::array<std::uint16_t, 32> basis{};
    for (unsigned int bit = 0; bit < basis.size(); ++bit) {
        std::vector<std::uint8_t> toggled;
        const bool generated = generatedHeaderCrc(
            baseBits ^ (UINT64_C(1) << bit), toggled);
        t.expect(generated, "CRC basis HEADER generation succeeds");
        if (!generated) return;
        basis[bit] = static_cast<std::uint16_t>(
            headerTestCrc(toggled, 16u, toggled.size() - 18u) ^ baseCrc);
    }
    std::array<std::uint64_t, 16> equations{};
    for (unsigned int outputBit = 0; outputBit < equations.size(); ++outputBit) {
        for (unsigned int inputBit = 0; inputBit < basis.size(); ++inputBit)
            if ((basis[inputBit] & (1u << outputBit)) != 0u)
                equations[outputBit] |= UINT64_C(1) << inputBit;
        if ((baseCrc & (1u << outputBit)) != 0u)
            equations[outputBit] |= UINT64_C(1) << 32u;
    }
    unsigned int pivotRow = 0;
    std::array<unsigned int, 16> pivotColumns{};
    for (unsigned int column = 0; column < basis.size(); ++column) {
        unsigned int pivot = pivotRow;
        while (pivot < equations.size()
               && (equations[pivot] & (UINT64_C(1) << column)) == 0u)
            ++pivot;
        if (pivot == equations.size()) continue;
        std::swap(equations[pivotRow], equations[pivot]);
        for (unsigned int row = 0; row < equations.size(); ++row)
            if (row != pivotRow
                && (equations[row] & (UINT64_C(1) << column)) != 0u)
                equations[row] ^= equations[pivotRow];
        pivotColumns[pivotRow] = column;
        ++pivotRow;
    }
    std::uint64_t zeroBits = baseBits;
    bool solvable = true;
    for (unsigned int row = pivotRow; row < equations.size(); ++row)
        if ((equations[row] & UINT64_C(0xFFFFFFFF)) == 0u
            && (equations[row] & (UINT64_C(1) << 32u)) != 0u)
            solvable = false;
    std::uint32_t adjustment = 0;
    for (unsigned int row = 0; row < pivotRow; ++row)
        if ((equations[row] & (UINT64_C(1) << 32u)) != 0u)
            adjustment |= UINT32_C(1) << pivotColumns[row];
    zeroBits ^= adjustment;
    t.expect(solvable, "generated HEADER CRC bit map admits a zero-checksum solution");
    double zeroValue = 0;
    std::memcpy(&zeroValue, &zeroBits, sizeof zeroValue);
    DRW_Header zeroSource;
    zeroSource.addInt("LUNITS", 4, 70);
    zeroSource.addDouble("USERR1", zeroValue, 40);
    dwgWriter15 zeroWriter(nullptr, &zeroSource);
    t.expect(zeroWriter.writeDwgHeader(), "zero-CRC full HEADER generation succeeds");
    auto zeroBytes = zeroWriter.buffer();
    const auto zeroOffset = zeroBytes.size() - 18u;
    t.expect(headerTestCrc(zeroBytes, 16u, zeroOffset) == 0u
                 && zeroBytes[zeroOffset] == 0u && zeroBytes[zeroOffset + 1u] == 0u,
             "full HEADER's independently calculated and stored CRC are both zero");
    HeaderSectionReader zeroReader(zeroBytes);
    DRW_Header zeroHeader;
    t.expect(zeroReader.readDwgHeader(zeroHeader) && zeroReader.diagnostics().empty()
                 && zeroHeader.vars.count("USERR1") == 1u
                 && zeroHeader.vars.at("USERR1")->content.d == zeroValue,
             "a matching zero CRC is accepted cleanly with exact semantic data");
}

void setHeaderHighSizeWord(std::vector<std::uint8_t>& bytes, bool present) {
    const std::uint32_t oldCount = std::uint32_t(bytes[16])
        | (std::uint32_t(bytes[17]) << 8u)
        | (std::uint32_t(bytes[18]) << 16u)
        | (std::uint32_t(bytes[19]) << 24u);
    if (present) {
        bytes.insert(bytes.begin() + 20, 4u, 0u);
    } else {
        bytes.erase(bytes.begin() + 20, bytes.begin() + 24);
    }
    const std::uint32_t newCount = present ? oldCount + 4u : oldCount - 4u;
    for (std::size_t i = 0; i < 4u; ++i)
        bytes[16u + i] = static_cast<std::uint8_t>(newCount >> (i * 8u));
    const std::size_t crcOffset = 20u + newCount;
    const std::uint16_t crc = headerTestCrc(bytes, 16u, crcOffset);
    bytes[crcOffset] = static_cast<std::uint8_t>(crc);
    bytes[crcOffset + 1u] = static_cast<std::uint8_t>(crc >> 8u);
}

void excludeHeaderHighSizeWordFromCount(std::vector<std::uint8_t>& bytes) {
    const std::uint32_t oldCount = std::uint32_t(bytes.at(16u))
        | (std::uint32_t(bytes.at(17u)) << 8u)
        | (std::uint32_t(bytes.at(18u)) << 16u)
        | (std::uint32_t(bytes.at(19u)) << 24u);
    const std::uint32_t newCount = oldCount - 4u;
    for (std::size_t i = 0; i < 4u; ++i)
        bytes[16u + i] = static_cast<std::uint8_t>(newCount >> (i * 8u));
    const std::size_t crcOffset = 24u + newCount;
    const std::uint16_t crc = headerTestCrc(bytes, 16u, crcOffset);
    bytes.at(crcOffset) = static_cast<std::uint8_t>(crc);
    bytes.at(crcOffset + 1u) = static_cast<std::uint8_t>(crc >> 8u);
}

template<class Writer>
std::vector<std::uint8_t> makeHeaderSection(TestContext& t,
                                             const std::string& label) {
    DRW_Header source;
    source.addInt("LUNITS", 4, 70);
    Writer writer(nullptr, &source);
    t.expect(writer.writeDwgHeader(), "generated HEADER section: " + label);
    return writer.buffer();
}

std::vector<std::uint8_t> headerWithNodReference(TestContext& t,
                                                std::uint8_t code,
                                                std::uint64_t value) {
    auto bytes = makeHeaderSection<dwgWriter32>(t, "NOD reference vector");
    dwgBuffer input(bytes.data(), bytes.size());
    t.expect(input.setPosition(24u), "NOD vector reaches the modern HEADER bit-size");
    const auto handlesBit = 192u + input.getRawLong32();
    t.expect(input.setPosition(handlesBit / 8u), "NOD vector reaches the handle stream");
    input.setBitPos(handlesBit & 7u);
    // ODA9: CLAYER..CMLSTYLE6, paper/model UCS3+3, DIM8,
    // controls9, ACAD_GROUP/MLINESTYLE2; the next H is NOD.
    for (int i = 0; i < 31; ++i) (void)input.getHandle();
    const auto offset = input.getPosition() * 8u + input.getBitPos();
    const auto old = input.getHandle();
    const bool located = input.isGood()
        && old.code == DRW::DwgHardOwnership
        && old.ref64 == DRW::DwgNamedObjectsDictionaryHandle;
    t.expect(located,
             "independent HEADER H walk locates the writer's emitted NOD root");
    if (!located) return {};
    std::uint8_t size = 0;
    for (auto remaining = value; remaining != 0u; remaining >>= 8u) ++size;
    std::vector<std::uint8_t> encoded{static_cast<std::uint8_t>((code << 4u) | size)};
    for (int i = size; i > 0; --i)
        encoded.push_back(static_cast<std::uint8_t>(value >> (8u * (i - 1))));
    // HEADER handle streams need not be byte aligned. Replace the complete
    // emitted NOD H while preserving every surrounding bit and stream origin.
    const auto original = bytes;
    const std::size_t oldEncodedSize = 1u + old.size;
    const std::int64_t bitDelta =
        (static_cast<std::int64_t>(encoded.size())
         - static_cast<std::int64_t>(oldEncodedSize)) * 8;
    bytes.assign(original.size() + encoded.size() - oldEncodedSize, 0u);
    const auto copyBit = [&bytes](std::size_t destination, unsigned bit) {
        bytes[destination / 8u] |= static_cast<std::uint8_t>(bit << (7u - destination % 8u));
    };
    for (std::size_t i = 0; i < offset; ++i)
        copyBit(i, (original[i / 8u] >> (7u - i % 8u)) & 1u);
    for (std::size_t i = 0; i < encoded.size() * 8u; ++i)
        copyBit(offset + i, (encoded[i / 8u] >> (7u - i % 8u)) & 1u);
    for (std::size_t i = offset + oldEncodedSize * 8u;
         i < original.size() * 8u; ++i) {
        const auto destination = static_cast<std::size_t>(
            static_cast<std::int64_t>(i) + bitDelta);
        copyBit(destination,
                (original[i / 8u] >> (7u - i % 8u)) & 1u);
    }
    const auto declared = static_cast<std::uint32_t>(bytes.size() - 38u);
    for (unsigned i = 0; i < 4; ++i)
        bytes[16u + i] = static_cast<std::uint8_t>(declared >> (8u * i));
    const auto crcOffset = bytes.size() - 18u;
    const auto crc = headerTestCrc(bytes, 16u, crcOffset);
    bytes[crcOffset] = static_cast<std::uint8_t>(crc);
    bytes[crcOffset + 1u] = static_cast<std::uint8_t>(crc >> 8u);
    return bytes;
}

class NodObjectWriter final : public dwgWriter32 {
public:
    using dwgWriter32::dwgWriter32;
    const auto& objects() const { return m_objectMap; }
};

class NodObjectReader final : public dwgReader32 {
public:
    explicit NodObjectReader(std::vector<std::uint8_t>& bytes)
        : dwgReader32(std::make_unique<dwgBuffer>(bytes.data(), bytes.size()), nullptr) {
        version = DRW::AC1032;
        decoder.setVersion(version, false);
    }
    void adoptHeader(const DRW_Header& header) {
        m_dwgNodReference = header.dwgNamedObjectDictionaryReference();
        m_dwgNodPublished = m_dwgNodFailureReported = false;
    }
    bool readObjects(std::vector<std::uint8_t>& bytes,
                     const std::vector<std::pair<std::uint32_t, std::uint32_t>>& frames,
                     DRW_Interface& sink) {
        objObjectMap.clear();
        for (std::size_t i = 0; i < frames.size(); ++i)
            objObjectMap.emplace(frames[i].first,
                objHandle(0u, frames[i].first, frames[i].second, i));
        dwgBuffer input(bytes.data(), bytes.size(), &decoder);
        return dwgReader::readDwgObjects(sink, &input);
    }
    const auto& diagnostics() const { return m_integrityDiagnostics; }
};

struct GeneratedTableStyleObject {
    std::vector<std::uint8_t> payload;
    std::uint32_t handleBits{0};
};

bool appendGeneratedBits(dwgBufferW& destination,
                         const dwgBufferW& source) {
    dwgBuffer reader(source.data().data(), source.data().size());
    for (std::uint64_t bit = 0; bit < source.bitCount(); ++bit) {
        const std::uint8_t value = reader.getBit();
        if (!reader.isGood())
            return false;
        destination.putBit(value);
    }
    return destination.isGood();
}

bool makeGeneratedR24TableStyleObject(std::uint32_t handle,
                                      std::int32_t declaredCellStyleCount,
                                      GeneratedTableStyleObject& result) {
    constexpr DRW::Version version = DRW::AC1032;
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    dwgBufferW object(&codec), body(&codec), strings(&codec), handles(&codec);

    body.putRawChar8(1u); // R24 TABLESTYLE class version.
    strings.putVariableText(version, "Generated table style");
    body.putBitLong(0); // reserved field 1.
    body.putBitLong(0); // reserved field 2.
    body.putBitLong(5); // R24 cell-style type.
    body.putBitShort(0u); // no populated cell-style data.
    body.putBitLong(4); // table-style cell-style ID.
    body.putBitLong(2); // table-style cell-style class.
    strings.putVariableText(version, "Table");
    body.putBitLong(declaredCellStyleCount);
    // Deliberately emit no repeated cell style: count 1 is the malformed
    // control; count 0 is the valid following-object sentinel.

    object.putObjType(version, DRW_TableStyle::kDwgClassNum);
    dwgHandle ownHandle;
    ownHandle.ref = handle;
    object.putHandle(ownHandle);
    object.putBitShort(0u); // no EED.
    object.putBitLong(0); // no reactors.
    object.putBit(1u); // no extension-dictionary handle.
    object.putBit(0u); // no AcDs data.
    if (!appendGeneratedBits(object, body))
        return false;
    object.alignToByte();

    strings.alignToByte();
    if (!strings.data().empty())
        object.putBytes(strings.data().data(), strings.size());
    for (int bit = 0; bit < 7; ++bit)
        object.putBit(0u);
    const std::uint64_t stringBitSize = strings.size() * 8u + 7u;
    if (stringBitSize > std::numeric_limits<std::uint16_t>::max())
        return false;
    object.putRawShort16(static_cast<std::uint16_t>(stringBitSize));
    object.putBit(1u); // bounded R2007+ string stream.
    object.alignToByte();

    dwgHandle owner;
    owner.code = DRW::DwgHardOwnership;
    owner.size = 2u;
    owner.ref = 0xA200u;
    owner.ref64 = owner.ref;
    handles.putHandle(owner);
    dwgHandle unknown;
    unknown.code = DRW::DwgHardPointer;
    unknown.size = 2u;
    unknown.ref = 0xA300u;
    unknown.ref64 = unknown.ref;
    handles.putHandle(unknown);
    handles.alignToByte();
    if (!object.isGood() || !handles.isGood()
        || handles.bitCount() > std::numeric_limits<std::uint32_t>::max())
        return false;
    result.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    if (!handles.data().empty())
        object.putBytes(handles.data().data(), handles.size());
    if (!object.isGood())
        return false;
    result.payload = object.data();
    return true;
}

bool wrapGeneratedR24Object(const GeneratedTableStyleObject& object,
                            std::vector<std::uint8_t>& frameBytes) {
    if (object.payload.empty())
        return false;
    dwgBufferW frame;
    frame.putModularShort(static_cast<std::int32_t>(object.payload.size()));
    frame.putUModularChar(object.handleBits);
    frame.putBytes(object.payload.data(), object.payload.size());
    if (!frame.isGood())
        return false;
    frame.putRawShort16(frame.crc16(0xC0C1u, 0u, frame.size()));
    if (!frame.isGood())
        return false;
    frameBytes = frame.data();
    return true;
}

class TableStyleDispatchReader final : public dwgReader32 {
public:
    explicit TableStyleDispatchReader(std::vector<std::uint8_t>& bytes)
        : dwgReader32(std::make_unique<dwgBuffer>(bytes.data(), bytes.size()),
                      nullptr) {
        version = DRW::AC1032;
        decoder.setVersion(version, false);
        auto tableStyleClass = std::make_unique<DRW_Class>();
        tableStyleClass->classNum = DRW_TableStyle::kDwgClassNum;
        tableStyleClass->recName = "TABLESTYLE";
        tableStyleClass->className = "AcDbTableStyle";
        tableStyleClass->entityFlag = 0;
        if (classesmap.emplace(tableStyleClass->classNum,
                               tableStyleClass.get()).second)
            tableStyleClass.release();
    }

    bool readObjects(
        std::vector<std::uint8_t>& bytes,
        const std::vector<std::tuple<std::uint32_t, std::uint32_t,
                                     std::uint64_t>>& frames,
        DRW_Interface& sink) {
        objObjectMap.clear();
        for (const auto& frame : frames) {
            const auto handle = std::get<0>(frame);
            const auto offset = std::get<1>(frame);
            const auto ordinal = std::get<2>(frame);
            if (!objObjectMap.emplace(
                    handle, objHandle(0u, handle, offset, ordinal)).second)
                return false;
        }
        dwgBuffer input(bytes.data(), bytes.size(), &decoder);
        return dwgReader::readDwgObjects(sink, &input);
    }

    std::size_t objectParseFailures() const { return m_objectParseFailures; }

};

class TableStyleDispatchSink final : public dx_iface {
public:
    TableStyleDispatchSink() {
        cData = &data;
        currentBlock = data.mBlock;
    }
    void addTableStyle(const DRW_TableStyle& object) override {
        handles.push_back(object.handle);
    }

    dx_data data;
    std::vector<std::uint32_t> handles;
};

void testTableStyleMalformedCountDoesNotBlockFollowingObject(TestContext& t) {
    GeneratedTableStyleObject malformedObject, sentinelObject;
    std::vector<std::uint8_t> malformedFrame, sentinelFrame;
    const bool malformedBuilt = makeGeneratedR24TableStyleObject(
        0xA100u, 1, malformedObject)
        && wrapGeneratedR24Object(malformedObject, malformedFrame);
    const bool sentinelBuilt = makeGeneratedR24TableStyleObject(
        0xA101u, 0, sentinelObject)
        && wrapGeneratedR24Object(sentinelObject, sentinelFrame);
    t.expect(malformedBuilt && sentinelBuilt,
             "generated R24 TABLESTYLE frames build without persistent fixtures");
    if (!malformedBuilt || !sentinelBuilt)
        return;

    const std::uint32_t sentinelOffset =
        static_cast<std::uint32_t>(malformedFrame.size());
    std::vector<std::uint8_t> bytes = malformedFrame;
    bytes.insert(bytes.end(), sentinelFrame.begin(), sentinelFrame.end());
    TableStyleDispatchReader reader(bytes);
    TableStyleDispatchSink sink;
    // Both frames resolve to the same custom class. Give the malformed frame
    // the earlier dispatch ordinal so the later byte-range sentinel is read
    // after its record-level parse refusal.
    const bool read = reader.readObjects(bytes,
        {{0xA101u, sentinelOffset, 0u}, {0xA100u, 0u, 1u}}, sink);
    t.expect(read && reader.objectParseFailures() == 1u
                 && sink.handles == std::vector<std::uint32_t>{0xA101u},
             "malformed TABLESTYLE count is refused before callback and the following OBJECT still publishes");
}

class PersistentManagerDispatchReader final : public dwgReader21 {
public:
    PersistentManagerDispatchReader(std::vector<std::uint8_t>& bytes,
                                    DRW::Version fileVersion,
                                    std::uint16_t classNumber)
        : dwgReader21(std::make_unique<dwgBuffer>(bytes.data(), bytes.size()), nullptr) {
        version = fileVersion;
        decoder.setVersion(version, false);
        auto definition = std::make_unique<DRW_Class>();
        definition->classNum = classNumber;
        definition->recName = "ACDBPERSSUBENTMANAGER";
        definition->className = "AcDbPersSubentManager";
        definition->entityFlag = 0;
        if (classesmap.emplace(classNumber, definition.get()).second)
            definition.release();
    }

    bool readObjects(std::vector<std::uint8_t>& bytes,
                     std::uint32_t firstHandle, std::uint32_t nextOffset,
                     std::uint32_t nextHandle, DRW_Interface& sink) {
        objObjectMap.emplace(firstHandle, objHandle(0u, firstHandle, 0u, 1u));
        objObjectMap.emplace(nextHandle, objHandle(0u, nextHandle, nextOffset, 0u));
        // This is an OBJECTS-only harness, so seed the same source identities
        // that the production HANDLE-map reader normally freezes. Without
        // coverage enabled the publication callback is intentionally omitted.
        for (auto& item : objObjectMap) {
            item.second.sourceOffsetSpace = DRW_DwgFrameOffsetSpace::DecodedBuffer;
            DRW_DwgFrameCoverageEntry entry;
            entry.m_handle = item.second.handle;
            entry.m_sourceOffset = item.second.loc;
            entry.m_sourceMapOrdinal = item.second.sourceOrdinal;
            entry.m_sourceOffsetSpace = item.second.sourceOffsetSpace;
            m_dwgSourceFrameIndexes.emplace(entry.m_handle, m_dwgSourceFrameLedger.size());
            m_dwgSourceFrameLedger.push_back(entry);
        }
        m_dwgFrameCoverageStatus = DRW_DwgFrameCoverageStatus::InProgress;
        dwgBuffer input(bytes.data(), bytes.size(), &decoder);
        return dwgReader::readDwgObjects(sink, &input);
    }

    std::size_t objectFailures() const { return m_objectParseFailures; }
};

class PersistentManagerDispatchSink final : public dx_iface {
public:
    explicit PersistentManagerDispatchSink(bool forwardToAdapter = false)
        : forward(forwardToAdapter) { cData = &data; currentBlock = data.mBlock; }
    void addAssociativeObject(const DRW_AssociativeObject& object) override {
        typed.push_back(object);
        if (forward)
            dx_iface::addAssociativeObject(object);
    }
    void addUnsupportedObject(const DRW_UnsupportedObject& object) override {
        raw.push_back(object);
        if (forward)
            dx_iface::addUnsupportedObject(object);
    }
    void addDwgFramePublication(const DRW_DwgFramePublication& publication) override {
        publications.push_back(publication);
        if (forward)
            dx_iface::addDwgFramePublication(publication);
    }
    dx_data data;
    std::vector<DRW_AssociativeObject> typed;
    std::vector<DRW_UnsupportedObject> raw;
    std::vector<DRW_DwgFramePublication> publications;
private:
    bool forward;
};

class AssociativeConstraintGroupDispatchReader final : public dwgReader24 {
public:
    explicit AssociativeConstraintGroupDispatchReader(
        std::vector<std::uint8_t>& bytes, std::uint16_t classNumber,
        const char* recordName = "ACDBASSOC2DCONSTRAINTGROUP",
        const char* className = "AcDbAssoc2dConstraintGroup")
        : dwgReader24(std::make_unique<dwgBuffer>(bytes.data(), bytes.size()),
                      nullptr) {
        version = DRW::AC1024;
        decoder.setVersion(version, false);
        auto definition = std::make_unique<DRW_Class>();
        definition->classNum = classNumber;
        definition->recName = recordName;
        definition->className = className;
        definition->entityFlag = 0;
        if (classesmap.emplace(classNumber, definition.get()).second)
            definition.release();
    }

    bool readObjects(
        std::vector<std::uint8_t>& bytes,
        const std::vector<std::tuple<std::uint32_t, std::uint32_t,
                                     std::uint64_t>>& frames,
        DRW_Interface& sink) {
        objObjectMap.clear();
        m_dwgSourceFrameIndexes.clear();
        m_dwgSourceFrameLedger.clear();
        for (const auto& frame : frames) {
            const auto handle = std::get<0>(frame);
            const auto offset = std::get<1>(frame);
            const auto ordinal = std::get<2>(frame);
            objHandle source(0u, handle, offset, ordinal);
            source.sourceOffsetSpace = DRW_DwgFrameOffsetSpace::DecodedBuffer;
            if (!objObjectMap.emplace(handle, source).second)
                return false;
            DRW_DwgFrameCoverageEntry entry;
            entry.m_handle = handle;
            entry.m_sourceOffset = offset;
            entry.m_sourceMapOrdinal = ordinal;
            entry.m_sourceOffsetSpace = source.sourceOffsetSpace;
            m_dwgSourceFrameIndexes.emplace(handle,
                                             m_dwgSourceFrameLedger.size());
            m_dwgSourceFrameLedger.push_back(entry);
        }
        m_dwgFrameCoverageStatus = DRW_DwgFrameCoverageStatus::InProgress;
        dwgBuffer input(bytes.data(), bytes.size(), &decoder);
        return dwgReader::readDwgObjects(sink, &input);
    }

    std::size_t objectFailures() const { return m_objectParseFailures; }
};

void testAssociativeNetworkActionIndexGenerated(TestContext& t) {
    using namespace associative_constraint_group_test;
    constexpr std::uint16_t classNumber = 603u;
    constexpr std::uint32_t sourceHandle = 0xA300u;
    constexpr std::uint32_t parentHandle = 0xA009u;
    constexpr std::uint32_t owningNetworkHandle = 0xA010u;
    constexpr std::uint32_t softActionHandle = 0xA011u;
    constexpr std::uint32_t ownedActionHandle = 0xA012u;
    constexpr std::int32_t baseActionIndex = 4;
    constexpr std::int32_t networkActionIndex = 3;

    dwgBufferW object;
    dwgBufferW strings;
    object.putObjType(DRW::AC1024, classNumber);
    object.putHandle(reference(DRW::DwgNullReference, sourceHandle));
    object.putBitShort(0); // no EED
    object.putBitLong(0); // no persistent reactors
    object.putBit(1u); // no extension dictionary
    object.putBitShort(1); // AcDbAssocAction class version
    object.putBitLong(0); // geometry status
    object.putBitLong(baseActionIndex);
    object.putBitLong(0); // maximum dependency index
    object.putBitLong(0); // no action dependencies
    object.putBitShort(0); // AcDbAssocNetwork version
    object.putBitLong(networkActionIndex);
    object.putBitLong(2); // two actions
    object.putBit(0u); // soft action reference
    object.putBit(1u); // owned action reference
    object.putBitLong(0); // no additional owned-action list

    Frame frame;
    const bool stringStreamBuilt = appendStringStream(object, strings);
    frame.objectEndBit = object.bitCount();
    if (!stringStreamBuilt) {
        t.expect(false, "generated associative network string stream is valid");
        return;
    }
    object.putHandle(reference(DRW::DwgSoftPointer, parentHandle));
    object.putHandle(reference(DRW::DwgSoftPointer, owningNetworkHandle));
    object.putHandle(reference(DRW::DwgHardOwnership, 0u)); // no action body
    object.putHandle(reference(DRW::DwgSoftPointer, softActionHandle));
    object.putHandle(reference(DRW::DwgHardOwnership, ownedActionHandle));
    object.alignToByte();
    const bool frameBuilt = object.isGood()
        && object.bitCount() >= frame.objectEndBit
        && object.bitCount() - frame.objectEndBit
            <= std::numeric_limits<std::uint32_t>::max();
    if (frameBuilt) {
        frame.handleBits = static_cast<std::uint32_t>(
            object.bitCount() - frame.objectEndBit);
        frame.payload = object.data();
    }
    std::vector<std::uint8_t> bytes;
    const bool wrapped = frameBuilt && wrapFrame(frame, bytes);
    t.expect(wrapped,
             "AC1024 associative network regression frame is generated in memory");
    if (!wrapped)
        return;

    AssociativeConstraintGroupDispatchReader reader(
        bytes, classNumber, "ACDBASSOCNETWORK", "AcDbAssocNetwork");
    PersistentManagerDispatchSink sink;
    const bool read = reader.readObjects(bytes, {{sourceHandle, 0u, 0u}}, sink);
    const bool parsed = sink.typed.size() == 1u
        && sink.raw.size() == 1u
        && sink.typed.front().m_recordName == "ACDBASSOCNETWORK"
        && sink.typed.front().handle == sourceHandle
        && sink.typed.front().parentHandle == parentHandle
        && sink.typed.front().m_classVersion == 1u
        && sink.typed.front().m_actionIndex == baseActionIndex
        && sink.typed.front().m_networkFieldsParsed
        && sink.typed.front().m_networkVersion == 0
        && sink.typed.front().m_networkActionIndex == networkActionIndex
        && sink.typed.front().m_owningNetworkHandle == owningNetworkHandle
        && sink.typed.front().m_actions.size() == 2u
        && !sink.typed.front().m_actions[0].m_isOwned
        && sink.typed.front().m_actions[0].m_handle == softActionHandle
        && sink.typed.front().m_actions[1].m_isOwned
        && sink.typed.front().m_actions[1].m_handle == ownedActionHandle
        && reader.objectFailures() == 0u;
    t.expect(read && parsed,
             "AC1024 network and base action indexes remain distinct with action roles intact");
    if (!parsed) {
        std::cerr << "AC1024 generated associative network state: read="
                  << read << " typed=" << sink.typed.size()
                  << " raw=" << sink.raw.size()
                  << " failures=" << reader.objectFailures();
        if (!sink.typed.empty()) {
            const DRW_AssociativeObject& value = sink.typed.front();
            std::cerr << " record=" << value.m_recordName
                      << " handle=" << value.handle
                      << " owner=" << value.parentHandle
                      << " classVersion=" << value.m_classVersion
                      << " actionIndex=" << value.m_actionIndex
                      << " networkVersion=" << value.m_networkVersion
                      << " networkActionIndex=" << value.m_networkActionIndex
                      << " networkParsed=" << value.m_networkFieldsParsed
                      << " actions=" << value.m_actions.size()
                      << " statuses=" << value.m_prefixStatuses.size();
            if (!value.m_prefixStatuses.empty())
                std::cerr << " lastStatus=" << static_cast<int>(
                    value.m_prefixStatuses.back().m_status);
        }
        std::cerr << '\n';
        return;
    }

    DRW_AssociativeObject copied(sink.typed.front());
    DRW_AssociativeObject assigned;
    assigned = sink.typed.front();
    t.expect(copied.m_actionIndex == baseActionIndex
                 && copied.m_networkActionIndex == networkActionIndex
                 && copied.m_networkVersion == 0
                 && copied.m_networkFieldsParsed
                 && assigned.m_actionIndex == baseActionIndex
                 && assigned.m_networkActionIndex == networkActionIndex
                 && assigned.m_networkVersion == 0
                 && assigned.m_networkFieldsParsed,
             "associative network index fields survive copy construction and assignment");
}

void testAssociativeVariableGenerated(TestContext& t) {
    using namespace associative_constraint_group_test;
    constexpr std::uint16_t classNumber = 757u; // DWG class type 1257
    constexpr std::uint32_t sourceHandle = 0xA400u;
    constexpr std::uint32_t parentHandle = 0xA009u;
    constexpr std::uint32_t owningNetworkHandle = 0xA010u;

    const auto makeVariable = [&](std::int32_t valueCode,
                                  std::int32_t ownedDependencyCount = 0,
                                  std::uint8_t ownedDependencyCode =
                                      DRW::DwgHardOwnership) {
        dwgBufferW object;
        dwgBufferW strings;
        object.putObjType(DRW::AC1024, classNumber);
        object.putHandle(reference(DRW::DwgNullReference, sourceHandle));
        object.putBitShort(0); // no EED
        object.putBitLong(0); // no persistent reactors
        object.putBit(1u); // no extension dictionary
        object.putBitShort(1); // AcDbAssocAction class version
        object.putBitLong(0); // geometry status
        object.putBitLong(7); // action index
        object.putBitLong(0); // maximum dependency index
        object.putBitLong(0); // no action dependencies
        object.putBitLong(2); // AcDbAssocVariable class version
        strings.putVariableText(DRW::AC1024, "BOLTHOLE");
        strings.putVariableText(DRW::AC1024, "1");
        strings.putVariableText(DRW::AC1024, "AcDbCalc:1.0");
        strings.putVariableText(DRW::AC1024, "");
        object.putBitShort(static_cast<std::int16_t>(valueCode));
        if (valueCode == 40)
            object.putBitDouble(3.5);
        else if (valueCode == 90)
            object.putBitLong(1);
        else
            object.putBitLong(0); // unconsumed, unsupported value control
        object.putBit(1u); // has_t78
        strings.putVariableText(DRW::AC1024, "");
        object.putBit(0u); // b290
        object.putBitShort(static_cast<std::int16_t>(
            ownedDependencyCount));
        if (ownedDependencyCount == 1)
            object.putBitShort(0); // observed uninterpreted suffix marker

        Frame frame;
        const bool stringsAdded = appendStringStream(object, strings);
        frame.objectEndBit = object.bitCount();
        object.putHandle(reference(DRW::DwgSoftPointer, parentHandle));
        object.putHandle(reference(
            DRW::DwgSoftPointer, owningNetworkHandle));
        object.putHandle(reference(DRW::DwgHardOwnership, 0u));
        for (std::int32_t index = 0; index < ownedDependencyCount; ++index)
            object.putHandle(reference(ownedDependencyCode,
                                       sourceHandle + 5u));
        object.alignToByte();
        const bool frameBuilt = stringsAdded && object.isGood()
            && object.bitCount() >= frame.objectEndBit
            && object.bitCount() - frame.objectEndBit
                <= std::numeric_limits<std::uint32_t>::max();
        if (frameBuilt) {
            frame.handleBits = static_cast<std::uint32_t>(
                object.bitCount() - frame.objectEndBit);
            frame.payload = object.data();
        }
        std::vector<std::uint8_t> bytes;
        return frameBuilt && wrapFrame(frame, bytes)
            ? bytes : std::vector<std::uint8_t>{};
    };

    const std::array<std::pair<std::int32_t, std::int32_t>, 3>
        variableProfiles{{{40, 0}, {90, 0}, {90, 1}}};
    for (const auto& profile : variableProfiles) {
        const std::int32_t valueCode = profile.first;
        const std::int32_t ownedDependencyCount = profile.second;
        std::vector<std::uint8_t> bytes = makeVariable(
            valueCode, ownedDependencyCount);
        AssociativeConstraintGroupDispatchReader reader(
            bytes, classNumber, "ACDBASSOCVARIABLE", "AcDbAssocVariable");
        PersistentManagerDispatchSink sink;
        const bool read = !bytes.empty()
            && reader.readObjects(bytes, {{sourceHandle, 0u, 0u}}, sink);
        const bool parsed = sink.typed.size() == 1u
            && sink.typed.front().m_recordName == "ACDBASSOCVARIABLE"
            && sink.typed.front().m_classVersion == 1u
            && sink.typed.front().m_actionIndex == 7
            && sink.typed.front().m_owningNetworkHandle
                == owningNetworkHandle
            && sink.typed.front().m_associativeVariableFieldsParsed
            && sink.typed.front().m_associativeVariableWireComplete
            && sink.typed.front().m_variableOwnedDependenciesParsed
            && sink.typed.front().m_variableOwnedDependencies.size()
                == static_cast<std::size_t>(ownedDependencyCount)
            && sink.typed.front().m_variableSuffixMarkerPresent
                == (ownedDependencyCount == 1)
            && (!sink.typed.front().m_variableSuffixMarkerPresent
                || sink.typed.front().m_variableSuffixMarker == 0)
            && sink.typed.front().m_variableClassVersion == 2
            && sink.typed.front().m_variableName == "BOLTHOLE"
            && sink.typed.front().m_variableExpression == "1"
            && sink.typed.front().m_variableEvaluator == "AcDbCalc:1.0"
            && sink.typed.front().m_variableDescription.empty()
            && sink.typed.front().m_variableHasT78
            && sink.typed.front().m_variableT78.empty()
            && !sink.typed.front().m_variableB290
            && sink.typed.front().m_variableValueParsed
            && sink.typed.front().m_variableValueCode == valueCode
            && reader.objectFailures() == 0u
            && !sink.typed.front().m_prefixStatuses.empty()
            && sink.typed.front().m_prefixStatuses.back().m_kind
                == DRW_AssociativePrefixStatus::Kind::AcDbAssocVariable
            && sink.typed.front().m_prefixStatuses.back().m_status
                == DRW_AssociativePrefixStatus::ParseStatus::Complete
            && sink.typed.front().m_prefixStatuses.back()
                   .m_decodedValueCount == 10u;
        const bool ownedDependencyMatches = parsed
            && sink.typed.front().m_prefixStatuses.back()
                   .m_decodedHandleCount
                == static_cast<std::size_t>(ownedDependencyCount)
            && sink.typed.front().m_prefixStatuses.back()
                   .m_decodedCountValue == ownedDependencyCount
            && (ownedDependencyCount == 0
                || (sink.typed.front().m_variableOwnedDependencies.front()
                        .m_isOwned
                    && sink.typed.front().m_variableOwnedDependencies.front()
                        .m_handle == sourceHandle + 5u));
        const bool payloadMatches = parsed
            && (valueCode == 40
                ? sink.typed.front().m_variableValueKind
                        == DRW_AssociativeObject::EvalVariantKind::Real
                    && sink.typed.front().m_variableValueReal == 3.5
                : sink.typed.front().m_variableValueKind
                        == DRW_AssociativeObject::EvalVariantKind::Int32
                    && sink.typed.front().m_variableValueInt32 == 1);
        t.expect(read && parsed && payloadMatches && ownedDependencyMatches,
                 valueCode == 40
                    ? "AC1024 ASSOCVARIABLE real EvalVariant and string fields decode within exact spans"
                    : ownedDependencyCount == 0
                        ? "AC1024 ASSOCVARIABLE int32 EvalVariant and empty dependency suffix decode within exact spans"
                        : "AC1024 ASSOCVARIABLE captures its hard-owned value-dependency handle and zero suffix marker");
        if (parsed) {
            DRW_AssociativeObject copied(sink.typed.front());
            DRW_AssociativeObject assigned;
            assigned = sink.typed.front();
            t.expect(copied.m_associativeVariableFieldsParsed
                         && copied.m_variableValueCode == valueCode
                         && copied.m_variableName == "BOLTHOLE"
                         && copied.m_variableOwnedDependencies.size()
                            == static_cast<std::size_t>(ownedDependencyCount)
                         && assigned.m_associativeVariableWireComplete
                         && assigned.m_variableValueCode == valueCode
                         && assigned.m_variableExpression == "1",
                     "ASSOCVARIABLE typed body survives copy construction and assignment");
        }
    }

    std::vector<std::uint8_t> unsupportedBytes = makeVariable(330);
    AssociativeConstraintGroupDispatchReader unsupportedReader(
        unsupportedBytes, classNumber, "ACDBASSOCVARIABLE",
        "AcDbAssocVariable");
    PersistentManagerDispatchSink unsupportedSink;
    const bool unsupportedRead = !unsupportedBytes.empty()
        && unsupportedReader.readObjects(
            unsupportedBytes, {{sourceHandle, 0u, 0u}}, unsupportedSink);
    t.expect(unsupportedRead && unsupportedSink.typed.size() == 1u
                 && !unsupportedSink.typed.front()
                        .m_associativeVariableWireComplete
                 && unsupportedReader.objectFailures() == 0u
                 && !unsupportedSink.typed.front().m_prefixStatuses.empty()
                 && unsupportedSink.typed.front().m_prefixStatuses.back()
                        .m_status
                    == DRW_AssociativePrefixStatus::ParseStatus::Missing,
             "unsupported ASSOCVARIABLE EvalVariant remains typed-shell-only without corrupting object dispatch");

    const auto malformedOwnedSuffix = [&](std::int32_t count,
                                          std::uint8_t handleCode) {
        std::vector<std::uint8_t> bytes = makeVariable(90, count, handleCode);
        AssociativeConstraintGroupDispatchReader reader(
            bytes, classNumber, "ACDBASSOCVARIABLE", "AcDbAssocVariable");
        PersistentManagerDispatchSink sink;
        const bool read = !bytes.empty()
            && reader.readObjects(bytes, {{sourceHandle, 0u, 0u}}, sink);
        return read && sink.typed.size() == 1u
            && sink.typed.front().m_associativeVariableFieldsParsed
            && !sink.typed.front().m_associativeVariableWireComplete
            && !sink.typed.front().m_variableOwnedDependenciesParsed
            && reader.objectFailures() == 0u;
    };
    t.expect(malformedOwnedSuffix(1, DRW::DwgSoftPointer),
             "wrong-role AC1024 variable child handle remains a partial soft-failed record");
    t.expect(malformedOwnedSuffix(2, DRW::DwgHardOwnership),
             "unwitnessed AC1024 variable child count remains a partial soft-failed record");
}

void testAssociativeConstraintGroupGenerated(TestContext& t) {
    using namespace associative_constraint_group_test;
    constexpr std::uint16_t classNumber = 602u;
    constexpr std::uint32_t sourceHandle = 0xA100u;
    constexpr std::uint32_t sentinelHandle = 0xA101u;
    Options source;
    source.classNumber = classNumber;
    source.handle = sourceHandle;
    Options sentinel;
    sentinel.classNumber = classNumber;
    sentinel.handle = sentinelHandle;
    sentinel.actions.clear();
    sentinel.nodes.clear();

    Frame sourceObject;
    Frame sentinelObject;
    std::vector<std::uint8_t> sourceFrame;
    std::vector<std::uint8_t> sentinelFrame;
    const bool built = makeFrame(source, sourceObject)
        && wrapFrame(sourceObject, sourceFrame)
        && makeFrame(sentinel, sentinelObject)
        && wrapFrame(sentinelObject, sentinelFrame);
    t.expect(built,
             "AC1024 constraint-group input and sentinel are generated from scratch");
    if (!built)
        return;

    const auto sentinelOffset =
        static_cast<std::uint32_t>(sourceFrame.size());
    std::vector<std::uint8_t> bytes = sourceFrame;
    bytes.insert(bytes.end(), sentinelFrame.begin(), sentinelFrame.end());
    AssociativeConstraintGroupDispatchReader reader(bytes, classNumber);
    PersistentManagerDispatchSink sink;
    const bool read = reader.readObjects(bytes,
        {{sourceHandle, 0u, 0u},
         {sentinelHandle, sentinelOffset, 1u}}, sink);
    const auto parsed = std::find_if(
        sink.typed.begin(), sink.typed.end(),
        [sourceHandle](const DRW_AssociativeObject& value) {
            return value.handle == sourceHandle;
        });
    const auto parsedSentinel = std::find_if(
        sink.typed.begin(), sink.typed.end(),
        [sentinelHandle](const DRW_AssociativeObject& value) {
            return value.handle == sentinelHandle;
        });
    const bool groupMatches = parsed != sink.typed.end()
        && parsed->m_recordName == "ACDBASSOC2DCONSTRAINTGROUP"
        && parsed->m_classVersion == 1u
        && parsed->m_constraintGroupBodyParsed
        && parsed->m_constraintGroupVersion == source.groupVersion
        && parsed->m_constraintGroupFlag == source.groupFlag
        && parsed->m_constraintWorkplane[0].x == 0.0
        && parsed->m_constraintWorkplane[1].x == 1.0
        && parsed->m_constraintWorkplane[2].y == 1.0
        && parsed->m_constraintGroupReference == source.groupReference
        && parsed->m_constraintActionsParsed
        && parsed->m_constraintActions == source.actions
        && parsed->m_constraintNextNodeIdRead
        && parsed->m_constraintNextNodeId == source.nextNodeId
        && parsed->m_constraintNodeCount == 0
        && parsed->m_constraintNodesParsed
        && parsed->m_constraintNodes.empty()
        && parsed->m_prefixStatuses.size() == 2u
        && parsed->m_prefixStatuses.back().m_kind
               == DRW_AssociativePrefixStatus::Kind::AcDbAssoc2dConstraintGroup
        && parsed->m_prefixStatuses.back().m_status
               == DRW_AssociativePrefixStatus::ParseStatus::Complete
        && parsed->m_prefixStatuses.back().m_decodedHandleCount == 2u
        && parsed->m_prefixStatuses.back().m_decodedCountValue == 0
        && parsedSentinel != sink.typed.end()
        && parsedSentinel->m_constraintGroupBodyParsed
        && parsedSentinel->m_constraintActions.empty()
        && sink.raw.size() == 2u && reader.objectFailures() == 0u;
    t.expect(read && groupMatches,
             "AC1024 version-2 constraint-group fixed fields, action roles, node-ID seed and empty-node boundary decode exactly");
    if (!groupMatches)
        std::cerr << "AC1024 associative constraint-group generated profile failed: read="
                  << read << " typed=" << sink.typed.size()
                  << " raw=" << sink.raw.size()
                  << " failures=" << reader.objectFailures()
                  << " sourceFound=" << (parsed != sink.typed.end());
    if (!groupMatches && parsed != sink.typed.end()) {
        std::cerr << " bodyParsed=" << parsed->m_constraintGroupBodyParsed
                  << " version=" << parsed->m_constraintGroupVersion
                  << " actions=" << parsed->m_constraintActions.size()
                  << " nodeCount=" << parsed->m_constraintNodeCount
                  << " statuses=" << parsed->m_prefixStatuses.size();
        if (!parsed->m_prefixStatuses.empty())
            std::cerr << " lastStatus=" << static_cast<int>(
                parsed->m_prefixStatuses.back().m_status)
                      << " decodedHandles=" << parsed->m_prefixStatuses.back()
                          .m_decodedHandleCount
                      << " decodedCount=" << parsed->m_prefixStatuses.back()
                          .m_decodedCountValue;
        std::cerr << '\n';
    } else if (!groupMatches) {
        std::cerr << '\n';
    }

    for (int profileMode = 0; profileMode < 8; ++profileMode) {
        Options profile;
        profile.classNumber = classNumber;
        profile.handle = sourceHandle;
        profile.actions.clear();
        profile.nodes.clear();
        if (profileMode == 0)
            profile.declaredActionCount = 100001;
        else if (profileMode == 1) {
            profile.groupVersion = 0;
            profile.nextNodeId = 523;
            profile.nodes = {{456, 0u, -1, {427, 433},
                              "AcEqualLengthConstraint", 0, true, true}};
        } else if (profileMode == 2 || profileMode == 3) {
            profile.groupVersion = 0;
            profile.nextNodeId = 523;
            if (profileMode == 3)
                profile.declaredNodeCount = 100001;
        } else if (profileMode == 4) {
            profile.nextNodeId = 523;
        } else if (profileMode == 5) {
            profile.groupVersion = 0;
            profile.nextNodeId = -1;
        } else if (profileMode == 6) {
            profile.groupVersion = 0;
            profile.nextNodeId = 523;
            profile.nodes = {{457, 0u, -1, {456},
                              "AcUnknownConstraint", 0, false, false}};
        } else {
            profile.groupVersion = 0;
            profile.nextNodeId = 523;
            profile.nodes = {{458, 0u, -1, {456},
                              std::string(257u, 'X'), 0, false, false}};
        }
        Frame profileObject;
        Frame nextObject;
        std::vector<std::uint8_t> profileFrame;
        std::vector<std::uint8_t> nextFrame;
        const bool profileBuilt = makeFrame(profile, profileObject)
            && wrapFrame(profileObject, profileFrame)
            && makeFrame(sentinel, nextObject)
            && wrapFrame(nextObject, nextFrame);
        t.expect(profileBuilt,
                 "AC1024 constraint-group profiles are generated from scratch");
        if (!profileBuilt)
            continue;
        const auto nextOffset = static_cast<std::uint32_t>(profileFrame.size());
        std::vector<std::uint8_t> profileBytes = profileFrame;
        profileBytes.insert(profileBytes.end(), nextFrame.begin(),
                            nextFrame.end());
        AssociativeConstraintGroupDispatchReader profileReader(
            profileBytes, classNumber);
        PersistentManagerDispatchSink profileSink;
        const bool profileRead = profileReader.readObjects(profileBytes,
            {{sourceHandle, 0u, 0u}, {sentinelHandle, nextOffset, 1u}},
            profileSink);
        const auto partialSource = std::find_if(
            profileSink.typed.begin(), profileSink.typed.end(),
            [sourceHandle](const DRW_AssociativeObject& value) {
                return value.handle == sourceHandle;
            });
        const auto partialRaw = std::find_if(
            profileSink.raw.begin(), profileSink.raw.end(),
            [sourceHandle](const DRW_UnsupportedObject& value) {
                return value.m_handle == sourceHandle;
            });
        const bool completeProfile = profileMode == 1 || profileMode == 2;
        const auto expectedStatus = profileMode == 0 || profileMode == 3
            ? DRW_AssociativePrefixStatus::ParseStatus::BoundedCountOverflow
            : completeProfile
                ? DRW_AssociativePrefixStatus::ParseStatus::Complete
                : DRW_AssociativePrefixStatus::ParseStatus::Partial;
        const bool recovered = profileRead
            && profileReader.objectFailures() == 0u
            && profileSink.typed.size() == 2u
            && partialSource != profileSink.typed.end()
            && partialSource->m_constraintGroupBodyParsed == completeProfile
            && partialSource->m_constraintActionsParsed
                   == (profileMode != 0)
            && partialSource->m_constraintNodesParsed == completeProfile
            && partialSource->m_prefixStatuses.size() == 2u
            && partialSource->m_prefixStatuses.back().m_status
                   == expectedStatus
            && (profileMode != 0
                || partialSource->m_constraintActionCount == 100001)
            && ((profileMode != 1 && profileMode != 6)
                || partialSource->m_constraintNodeCount == 1)
            && (profileMode == 0
                || (partialSource->m_constraintNextNodeIdRead
                    && partialSource->m_constraintNextNodeId
                           == profile.nextNodeId))
            && (profileMode != 2
                || (partialSource->m_constraintGroupVersion == 0
                    && partialSource->m_constraintNodeCount == 0
                    && partialSource->m_prefixStatuses.back().m_decodedCountValue
                           == 0))
            && (profileMode != 1
                || (partialSource->m_constraintNodes.size() == 1u
                    && partialSource->m_constraintNodes.front().m_className
                           == "AcEqualLengthConstraint"
                    && partialSource->m_constraintNodes.front().m_nodeId == 456
                    && partialSource->m_constraintNodes.front().m_connections
                           == std::vector<std::int32_t>{427, 433}
                    && partialSource->m_constraintNodes.front().m_dataKind
                           == DRW_AssociativeConstraintNode::DataKind::Geometrical
                    && partialSource->m_constraintNodes.front().m_ownerId == 0
                    && partialSource->m_constraintNodes.front().m_isImplied
                    && !partialSource->m_constraintNodes.front().m_hasActiveFlag
                    && partialSource->m_constraintNodes.front().m_classDataParsed))
            && (profileMode != 3
                || (partialSource->m_constraintNodeCount == 100001
                    && partialSource->m_prefixStatuses.back().m_decodedCountValue
                           == 100001))
            && (profileMode != 4
                || (partialSource->m_constraintGroupVersion == 2
                    && partialSource->m_constraintNextNodeIdRead
                    && partialSource->m_constraintNextNodeId == 523
                    && partialSource->m_prefixStatuses.back().m_decodedCountValue
                           == -1))
            && (profileMode != 5
                || (partialSource->m_constraintGroupVersion == 0
                    && partialSource->m_constraintNextNodeIdRead
                    && partialSource->m_constraintNextNodeId == -1
                    && partialSource->m_prefixStatuses.back().m_decodedCountValue
                           == -1))
            && (profileMode != 6
                || (partialSource->m_constraintNodes.size() == 1u
                    && partialSource->m_constraintNodes.front().m_className
                           == "AcUnknownConstraint"
                    && partialSource->m_constraintNodes.front().m_nodeId == 457
                    && partialSource->m_constraintNodes.front().m_connections
                           == std::vector<std::int32_t>{456}
                    && !partialSource->m_constraintNodes.front().m_classDataParsed
                    && partialSource->m_prefixStatuses.back().m_decodedValueCount
                           == 1u))
            && (profileMode != 7
                || (partialSource->m_constraintNodes.empty()
                    && partialSource->m_prefixStatuses.back().m_decodedValueCount
                           == 0u
                    && partialSource->m_prefixStatuses.back().m_decodedCountValue
                           == 1))
            && partialRaw != profileSink.raw.end()
            && (completeProfile || !partialRaw->m_typedPayloadValidated)
            && std::any_of(profileSink.typed.begin(), profileSink.typed.end(),
                [sentinelHandle](const DRW_AssociativeObject& value) {
                    return value.handle == sentinelHandle
                        && value.m_constraintGroupBodyParsed;
                })
            && profileSink.raw.size() == 2u
            && profileReader.objectFailures() == 0u;
        t.expect(recovered,
                 profileMode == 0
                     ? "over-bound constraint action counts publish a partial raw carrier and preserve the following OBJECT"
                     : profileMode == 1
                         ? "version-0 geometrical nodes decode names, IDs, connections and absent active-flag semantics"
                         : profileMode == 2
                             ? "empty version-0 constraint groups decode their next-node-ID seed and inline zero count exactly"
                             : profileMode == 3
                                 ? "over-bound inline node counts preserve their raw carrier and following OBJECT"
                                 : profileMode == 4
                                     ? "version-2 nonzero seeds preserve possible registry layouts without consuming a count"
                                     : profileMode == 5
                                         ? "negative next-node-ID seeds preserve their raw carrier without consuming a node count"
                                         : profileMode == 6
                                             ? "unknown node class retains its bounded envelope and leaves the remaining group raw"
                                             : "overlong node class names stop before allocating or consuming the node body");
    }

    {
        Options profile;
        profile.classNumber = classNumber;
        profile.handle = sourceHandle;
        profile.groupVersion = 0;
        profile.nextNodeId = 523;
        profile.actions.clear();

        Node parallel;
        parallel.id = 408;
        parallel.connections = {360, 295};
        parallel.className = "AcParallelConstraint";
        parallel.ownerId = 0;
        parallel.isImplied = false;
        parallel.hasGeometricalBody = true;
        profile.nodes.push_back(parallel);

        Node point;
        point.id = 409;
        point.connections = {408};
        point.className = "AcConstrainedImplicitPoint";
        point.hasImplicitPointBody = true;
        point.geometryDependencyCode = DRW::DwgSoftPointer;
        point.geometryDependencyHandle = 0xA007u;
        point.geometryNodeId = 0;
        point.hasPoint = true;
        point.point = DRW_Coord(5.0, 6.0, 7.0);
        point.pointType = 1u;
        point.pointIndex = 2;
        point.curveNodeId = 360;
        profile.nodes.push_back(point);

        Frame profileObject;
        Frame nextObject;
        std::vector<std::uint8_t> profileFrame;
        std::vector<std::uint8_t> nextFrame;
        const bool profileBuilt = makeFrame(profile, profileObject)
            && wrapFrame(profileObject, profileFrame)
            && makeFrame(sentinel, nextObject)
            && wrapFrame(nextObject, nextFrame);
        t.expect(profileBuilt,
                 "AC1024 parallel-node profile is generated from scratch");
        if (profileBuilt) {
            const auto nextOffset =
                static_cast<std::uint32_t>(profileFrame.size());
            std::vector<std::uint8_t> profileBytes = profileFrame;
            profileBytes.insert(profileBytes.end(), nextFrame.begin(),
                                nextFrame.end());
            AssociativeConstraintGroupDispatchReader profileReader(
                profileBytes, classNumber);
            PersistentManagerDispatchSink profileSink;
            const bool profileRead = profileReader.readObjects(profileBytes,
                {{sourceHandle, 0u, 0u}, {sentinelHandle, nextOffset, 1u}},
                profileSink);
            const auto partialSource = std::find_if(
                profileSink.typed.begin(), profileSink.typed.end(),
                [sourceHandle](const DRW_AssociativeObject& value) {
                    return value.handle == sourceHandle;
                });
            bool nodesMatch = partialSource != profileSink.typed.end()
                && partialSource->m_constraintNodes.size() == 2u;
            if (nodesMatch) {
                const auto& parsedParallel =
                    partialSource->m_constraintNodes[0];
                const auto& parsedPoint = partialSource->m_constraintNodes[1];
                nodesMatch = parsedParallel.m_className
                        == "AcParallelConstraint"
                    && parsedParallel.m_nodeId == 408
                    && parsedParallel.m_connections
                           == std::vector<std::int32_t>{360, 295}
                    && parsedParallel.m_dataKind
                           == DRW_AssociativeConstraintNode::DataKind::Geometrical
                    && parsedParallel.m_ownerId == 0
                    && !parsedParallel.m_isImplied
                    && !parsedParallel.m_hasActiveFlag
                    && parsedParallel.m_classDataParsed
                    && parsedPoint.m_className
                           == "AcConstrainedImplicitPoint"
                    && parsedPoint.m_nodeId == 409
                    && parsedPoint.m_dataKind
                           == DRW_AssociativeConstraintNode::DataKind::ImplicitPoint
                    && parsedPoint.m_hasGeometryDependency
                    && parsedPoint.m_geometryDependencyCode
                           == DRW::DwgSoftPointer
                    && parsedPoint.m_geometryDependencyHandle == 0xA007u
                    && parsedPoint.m_hasPoint
                    && parsedPoint.m_point.x == 5.0
                    && parsedPoint.m_point.y == 6.0
                    && parsedPoint.m_point.z == 7.0
                    && parsedPoint.m_pointType == 1u
                    && parsedPoint.m_pointIndex == 2
                    && parsedPoint.m_curveNodeId == 360
                    && parsedPoint.m_classDataParsed;
            }
            const bool profileMatches = profileRead
                && profileReader.objectFailures() == 0u
                && partialSource != profileSink.typed.end()
                && partialSource->m_constraintGroupBodyParsed
                && partialSource->m_constraintNodesParsed
                && partialSource->m_constraintNodeCount == 2
                && nodesMatch
                && partialSource->m_prefixStatuses.back().m_status
                       == DRW_AssociativePrefixStatus::ParseStatus::Complete
                && partialSource->m_prefixStatuses.back().m_decodedHandleCount
                       == 1u
                && std::any_of(profileSink.typed.begin(), profileSink.typed.end(),
                    [sentinelHandle](const DRW_AssociativeObject& value) {
                        return value.handle == sentinelHandle
                            && value.m_constraintGroupBodyParsed;
                    });
            t.expect(profileMatches,
                     "parallel node decodes owner/implied with active absent and consumes no handle before the following dependency node");
        }
    }

    for (int distanceMode = 0; distanceMode < 4; ++distanceMode) {
        Options profile;
        profile.classNumber = classNumber;
        profile.handle = sourceHandle;
        profile.groupVersion = 0;
        profile.nextNodeId = 523;
        profile.actions.clear();
        Node distance;
        distance.id = 77;
        distance.connections = {70, 75};
        distance.className = "AcDistanceConstraint";
        distance.ownerId = 0;
        distance.isImplied = false;
        distance.hasDistanceBody = true;
        distance.valueDependencyCode = distanceMode == 1
            ? DRW::DwgSoftPointer : DRW::DwgHardPointer;
        distance.valueDependencyHandle = 1210u;
        distance.dimensionDependencyCode = distanceMode == 2
            ? DRW::DwgSoftPointer : DRW::DwgHardPointer;
        distance.dimensionDependencyHandle = 1211u;
        distance.directionType = distanceMode == 3 ? 0u : 1u;
        distance.distance = DRW_Coord(1.0, 0.0, 0.0);
        profile.nodes.push_back(distance);

        Frame profileObject;
        Frame nextObject;
        std::vector<std::uint8_t> profileFrame;
        std::vector<std::uint8_t> nextFrame;
        const bool profileBuilt = makeFrame(profile, profileObject)
            && wrapFrame(profileObject, profileFrame)
            && makeFrame(sentinel, nextObject)
            && wrapFrame(nextObject, nextFrame);
        t.expect(profileBuilt,
                 "AC1024 distance-constraint profiles are generated from scratch");
        if (!profileBuilt)
            continue;

        const auto nextOffset = static_cast<std::uint32_t>(profileFrame.size());
        std::vector<std::uint8_t> profileBytes = profileFrame;
        profileBytes.insert(profileBytes.end(), nextFrame.begin(),
                            nextFrame.end());
        AssociativeConstraintGroupDispatchReader profileReader(
            profileBytes, classNumber);
        PersistentManagerDispatchSink profileSink;
        const bool profileRead = profileReader.readObjects(profileBytes,
            {{sourceHandle, 0u, 0u}, {sentinelHandle, nextOffset, 1u}},
            profileSink);
        const auto partialSource = std::find_if(
            profileSink.typed.begin(), profileSink.typed.end(),
            [sourceHandle](const DRW_AssociativeObject& value) {
                return value.handle == sourceHandle;
            });
        const auto partialRaw = std::find_if(
            profileSink.raw.begin(), profileSink.raw.end(),
            [sourceHandle](const DRW_UnsupportedObject& value) {
                return value.m_handle == sourceHandle;
            });
        const bool expectedComplete = distanceMode == 0 || distanceMode == 3;
        bool nodeMatches = partialSource != profileSink.typed.end()
            && partialSource->m_constraintNodes.size() == 1u;
        if (nodeMatches) {
            const auto& node = partialSource->m_constraintNodes.front();
            nodeMatches = node.m_className == "AcDistanceConstraint"
                && node.m_nodeId == 77
                && node.m_connections
                       == std::vector<std::int32_t>{70, 75}
                && node.m_classDataParsed == expectedComplete;
            if (expectedComplete) {
                nodeMatches = nodeMatches
                    && node.m_dataKind
                           == DRW_AssociativeConstraintNode::DataKind::Distance
                    && node.m_ownerId == 0
                    && !node.m_isImplied
                    && !node.m_hasActiveFlag
                    && node.m_hasValueDependencyHandle
                    && node.m_valueDependencyCode == DRW::DwgHardPointer
                    && node.m_valueDependencyHandle == 1210u
                    && node.m_hasDimensionDependencyHandle
                    && node.m_dimensionDependencyCode == DRW::DwgHardPointer
                    && node.m_dimensionDependencyHandle == 1211u
                    && node.m_directionType
                           == (distanceMode == 3 ? 0u : 1u)
                    && node.m_hasDistance == (distanceMode != 3)
                    && (distanceMode == 3
                        || (node.m_distance.x == 1.0
                            && node.m_distance.y == 0.0
                            && node.m_distance.z == 0.0));
            }
        }
        const bool profileMatches = profileRead
            && profileReader.objectFailures() == 0u
            && partialSource != profileSink.typed.end()
            && partialSource->m_constraintGroupBodyParsed == expectedComplete
            && partialSource->m_constraintNodeCount == 1
            && partialSource->m_constraintNodesParsed == expectedComplete
            && !partialSource->m_prefixStatuses.empty()
            && partialSource->m_prefixStatuses.back().m_decodedHandleCount
                   == (expectedComplete ? 2u : 0u)
            && nodeMatches
            && partialRaw != profileSink.raw.end()
            && (expectedComplete || !partialRaw->m_typedPayloadValidated)
            && std::any_of(profileSink.typed.begin(), profileSink.typed.end(),
                [sentinelHandle](const DRW_AssociativeObject& value) {
                    return value.handle == sentinelHandle
                        && value.m_constraintGroupBodyParsed;
                });
        t.expect(profileMatches,
                 distanceMode == 0
                     ? "distance node matches both code-5 dependencies and its conditional 3BD payload"
                     : distanceMode == 1
                         ? "distance value-dependency role mismatch preserves raw data and following-object recovery"
                         : distanceMode == 2
                             ? "distance dimension-dependency role mismatch preserves raw data and following-object recovery"
                             : "zero direction type preserves the absent distance separately from zero-valued geometry");
    }

    for (int implicitPointMode = 0; implicitPointMode < 3;
         ++implicitPointMode) {
        Options profile;
        profile.classNumber = classNumber;
        profile.handle = sourceHandle;
        profile.groupVersion = 0;
        profile.nextNodeId = 523;
        profile.actions.clear();
        Node implicitPoint;
        implicitPoint.id = 387;
        implicitPoint.connections = {388};
        implicitPoint.className = "AcConstrainedImplicitPoint";
        implicitPoint.hasImplicitPointBody = true;
        implicitPoint.geometryDependencyCode = implicitPointMode == 2
            ? DRW::DwgHardPointer : DRW::DwgSoftPointer;
        implicitPoint.geometryDependencyHandle = implicitPointMode == 1
            ? 0xA005u : implicitPointMode == 2 ? 0xA006u : 0u;
        implicitPoint.geometryNodeId = 0;
        implicitPoint.hasPoint = implicitPointMode == 1;
        implicitPoint.point = DRW_Coord(0.5, 1.5, 2.5);
        implicitPoint.pointType = 1u;
        implicitPoint.pointIndex = -1;
        implicitPoint.curveNodeId = 384;
        profile.nodes.push_back(implicitPoint);

        Frame profileObject;
        Frame nextObject;
        std::vector<std::uint8_t> profileFrame;
        std::vector<std::uint8_t> nextFrame;
        const bool profileBuilt = makeFrame(profile, profileObject)
            && wrapFrame(profileObject, profileFrame)
            && makeFrame(sentinel, nextObject)
            && wrapFrame(nextObject, nextFrame);
        t.expect(profileBuilt,
                 "AC1024 implicit-point node profiles are generated from scratch");
        if (!profileBuilt)
            continue;

        const auto nextOffset = static_cast<std::uint32_t>(profileFrame.size());
        std::vector<std::uint8_t> profileBytes = profileFrame;
        profileBytes.insert(profileBytes.end(), nextFrame.begin(),
                            nextFrame.end());
        AssociativeConstraintGroupDispatchReader profileReader(
            profileBytes, classNumber);
        PersistentManagerDispatchSink profileSink;
        const bool profileRead = profileReader.readObjects(profileBytes,
            {{sourceHandle, 0u, 0u}, {sentinelHandle, nextOffset, 1u}},
            profileSink);
        const auto partialSource = std::find_if(
            profileSink.typed.begin(), profileSink.typed.end(),
            [sourceHandle](const DRW_AssociativeObject& value) {
                return value.handle == sourceHandle;
            });
        const auto partialRaw = std::find_if(
            profileSink.raw.begin(), profileSink.raw.end(),
            [sourceHandle](const DRW_UnsupportedObject& value) {
                return value.m_handle == sourceHandle;
            });
        const bool expectedComplete = implicitPointMode != 2;
        bool nodeMatches = partialSource != profileSink.typed.end()
            && partialSource->m_constraintNodes.size() == 1u;
        if (nodeMatches) {
            const auto& node = partialSource->m_constraintNodes.front();
            nodeMatches = node.m_className == "AcConstrainedImplicitPoint"
                && node.m_nodeId == 387
                && node.m_connections == std::vector<std::int32_t>{388}
                && node.m_classDataParsed == expectedComplete;
            if (expectedComplete) {
                nodeMatches = nodeMatches
                    && node.m_dataKind
                           == DRW_AssociativeConstraintNode::DataKind::ImplicitPoint
                    && node.m_hasGeometryDependency
                    && node.m_geometryDependencyCode == DRW::DwgSoftPointer
                    && node.m_geometryDependencyHandle
                           == (implicitPointMode == 1 ? 0xA005u : 0u)
                    && node.m_geometryNodeId == 0
                    && node.m_hasPoint == (implicitPointMode == 1)
                    && (!node.m_hasPoint
                        || (node.m_point.x == 0.5
                            && node.m_point.y == 1.5
                            && node.m_point.z == 2.5))
                    && node.m_pointType == 1u
                    && node.m_pointIndex == -1
                    && node.m_curveNodeId == 384;
            }
        }
        const bool profileMatches = profileRead
            && profileReader.objectFailures() == 0u
            && partialSource != profileSink.typed.end()
            && partialSource->m_constraintGroupBodyParsed == expectedComplete
            && partialSource->m_constraintNodeCount == 1
            && partialSource->m_constraintNodesParsed == expectedComplete
            && nodeMatches
            && partialRaw != profileSink.raw.end()
            && !partialRaw->m_typedPayloadValidated
            && std::any_of(profileSink.typed.begin(), profileSink.typed.end(),
                [sentinelHandle](const DRW_AssociativeObject& value) {
                    return value.handle == sentinelHandle
                        && value.m_constraintGroupBodyParsed;
                });
        t.expect(profileMatches,
                 implicitPointMode == 0
                     ? "implicit-point null soft pointer is retained with absent coordinates and exact scalar fields"
                     : implicitPointMode == 1
                         ? "implicit-point non-null soft pointer decodes its optional 3BD and exact scalar fields"
                         : "implicit-point handle-role mismatch preserves raw data and following-object recovery");
        if (!profileMatches) {
            std::cerr << "AC1024 implicit-point profile mode="
                      << implicitPointMode << " read=" << profileRead
                      << " failures=" << profileReader.objectFailures()
                      << " typed=" << profileSink.typed.size()
                      << " raw=" << profileSink.raw.size()
                      << " source="
                      << (partialSource != profileSink.typed.end())
                      << " nodes="
                      << (partialSource != profileSink.typed.end()
                              ? partialSource->m_constraintNodes.size() : 0u)
                      << " parsed="
                      << (partialSource != profileSink.typed.end()
                              ? partialSource->m_constraintNodesParsed : false)
                      << " complete="
                      << (partialSource != profileSink.typed.end()
                              ? partialSource->m_constraintGroupBodyParsed : false);
            if (partialSource != profileSink.typed.end()
                && !partialSource->m_constraintNodes.empty()) {
                const auto& node = partialSource->m_constraintNodes.front();
                std::cerr << " class=" << node.m_className
                          << " data=" << node.m_classDataParsed
                          << " kind=" << static_cast<int>(node.m_dataKind)
                          << " depPresent=" << node.m_hasGeometryDependency
                          << " depCode="
                          << static_cast<unsigned>(node.m_geometryDependencyCode)
                          << " dep=" << node.m_geometryDependencyHandle
                          << " geomNode=" << node.m_geometryNodeId
                          << " hasPoint=" << node.m_hasPoint
                          << " pointType="
                          << static_cast<unsigned>(node.m_pointType)
                          << " pointIndex=" << node.m_pointIndex
                          << " curve=" << node.m_curveNodeId;
            }
            std::cerr << '\n';
        }
    }

    for (int boundedLineMode = 0; boundedLineMode < 2; ++boundedLineMode) {
        Options profile;
        profile.classNumber = classNumber;
        profile.handle = sourceHandle;
        profile.groupVersion = 0;
        profile.nextNodeId = 523;
        profile.actions.clear();
        Node boundedLine;
        boundedLine.id = 295;
        boundedLine.connections = {297, 299, 408};
        boundedLine.className = "AcConstrainedBoundedLine";
        boundedLine.hasBoundedLineBody = true;
        boundedLine.geometryDependencyCode = boundedLineMode == 0
            ? DRW::DwgSoftPointer : DRW::DwgHardPointer;
        boundedLine.geometryDependencyHandle = 1518u;
        boundedLine.geometryNodeId = 0;
        boundedLine.point = DRW_Coord(89.85816226224345,
                                      26.072948833487143, 0.0);
        boundedLine.direction = DRW_Coord(0.0, 1.0, 0.0);
        boundedLine.isRay = false;
        boundedLine.startPoint = boundedLine.point;
        boundedLine.endPoint = DRW_Coord(89.85816226224344,
                                         49.072948833487146, 0.0);
        profile.nodes.push_back(boundedLine);

        Frame profileObject;
        Frame nextObject;
        std::vector<std::uint8_t> profileFrame;
        std::vector<std::uint8_t> nextFrame;
        const bool profileBuilt = makeFrame(profile, profileObject)
            && wrapFrame(profileObject, profileFrame)
            && makeFrame(sentinel, nextObject)
            && wrapFrame(nextObject, nextFrame);
        t.expect(profileBuilt,
                 "AC1024 bounded-line profiles are generated from scratch");
        if (!profileBuilt)
            continue;

        const auto nextOffset = static_cast<std::uint32_t>(profileFrame.size());
        std::vector<std::uint8_t> profileBytes = profileFrame;
        profileBytes.insert(profileBytes.end(), nextFrame.begin(),
                            nextFrame.end());
        AssociativeConstraintGroupDispatchReader profileReader(
            profileBytes, classNumber);
        PersistentManagerDispatchSink profileSink;
        const bool profileRead = profileReader.readObjects(profileBytes,
            {{sourceHandle, 0u, 0u}, {sentinelHandle, nextOffset, 1u}},
            profileSink);
        const auto partialSource = std::find_if(
            profileSink.typed.begin(), profileSink.typed.end(),
            [sourceHandle](const DRW_AssociativeObject& value) {
                return value.handle == sourceHandle;
            });
        const auto partialRaw = std::find_if(
            profileSink.raw.begin(), profileSink.raw.end(),
            [sourceHandle](const DRW_UnsupportedObject& value) {
                return value.m_handle == sourceHandle;
            });
        const bool expectedComplete = boundedLineMode == 0;
        bool nodeMatches = partialSource != profileSink.typed.end()
            && partialSource->m_constraintNodes.size() == 1u;
        if (nodeMatches) {
            const auto& node = partialSource->m_constraintNodes.front();
            nodeMatches = node.m_className == "AcConstrainedBoundedLine"
                && node.m_nodeId == 295
                && node.m_connections
                       == std::vector<std::int32_t>{297, 299, 408}
                && node.m_classDataParsed == expectedComplete;
            if (expectedComplete) {
                nodeMatches = nodeMatches
                    && node.m_dataKind
                           == DRW_AssociativeConstraintNode::DataKind::BoundedLine
                    && node.m_hasGeometryDependency
                    && node.m_geometryDependencyCode == DRW::DwgSoftPointer
                    && node.m_geometryDependencyHandle == 1518u
                    && node.m_geometryNodeId == 0
                    && node.m_hasPoint
                    && node.m_point.x == boundedLine.point.x
                    && node.m_point.y == boundedLine.point.y
                    && node.m_point.z == boundedLine.point.z
                    && node.m_direction.x == 0.0
                    && node.m_direction.y == 1.0
                    && node.m_direction.z == 0.0
                    && !node.m_isRay
                    && node.m_startPoint.x == boundedLine.startPoint.x
                    && node.m_startPoint.y == boundedLine.startPoint.y
                    && node.m_startPoint.z == boundedLine.startPoint.z
                    && node.m_endPoint.x == boundedLine.endPoint.x
                    && node.m_endPoint.y == boundedLine.endPoint.y
                    && node.m_endPoint.z == boundedLine.endPoint.z;
            }
        }
        const bool profileMatches = profileRead
            && profileReader.objectFailures() == 0u
            && partialSource != profileSink.typed.end()
            && partialSource->m_constraintGroupBodyParsed == expectedComplete
            && partialSource->m_constraintNodeCount == 1
            && partialSource->m_constraintNodesParsed == expectedComplete
            && nodeMatches
            && partialRaw != profileSink.raw.end()
            && !partialRaw->m_typedPayloadValidated
            && std::any_of(profileSink.typed.begin(), profileSink.typed.end(),
                [sentinelHandle](const DRW_AssociativeObject& value) {
                    return value.handle == sentinelHandle
                        && value.m_constraintGroupBodyParsed;
                });
        t.expect(profileMatches,
                 boundedLineMode == 0
                     ? "bounded-line code-4 dependency and all witnessed vectors and flags decode exactly"
                     : "bounded-line wrong handle role preserves raw data and following-object recovery");
    }

    for (int arcMode = 0; arcMode < 2; ++arcMode) {
        Options profile;
        profile.classNumber = classNumber;
        profile.handle = sourceHandle;
        profile.groupVersion = 0;
        profile.nextNodeId = 523;
        profile.actions.clear();
        Node arc;
        arc.id = 163;
        arc.connections = {165, 167, 169, 170};
        arc.className = "AcConstrainedArc";
        arc.hasArcBody = true;
        arc.geometryDependencyCode = arcMode == 0
            ? DRW::DwgSoftPointer : DRW::DwgHardPointer;
        arc.geometryDependencyHandle = 1404u;
        arc.geometryNodeId = 0;
        arc.center = DRW_Coord(63.11370687434459,
                               37.572948833487146, 0.0);
        arc.normal = DRW_Coord(0.0, 0.0, 1.0);
        arc.direction = DRW_Coord(1.0, 0.0, 0.0);
        arc.radius = 11.5;
        arc.startParameter = 0.0;
        arc.endParameter = 6.283185307179586;
        arc.startPoint = DRW_Coord(64.11370687434459,
                                   26.116509596097544, 0.0);
        arc.endPoint = DRW_Coord(64.11370687434459,
                                 49.02938807087675, 0.0);
        profile.nodes.push_back(arc);

        Frame profileObject;
        Frame nextObject;
        std::vector<std::uint8_t> profileFrame;
        std::vector<std::uint8_t> nextFrame;
        const bool profileBuilt = makeFrame(profile, profileObject)
            && wrapFrame(profileObject, profileFrame)
            && makeFrame(sentinel, nextObject)
            && wrapFrame(nextObject, nextFrame);
        t.expect(profileBuilt,
                 "AC1024 arc profiles are generated from scratch");
        if (!profileBuilt)
            continue;

        const auto nextOffset = static_cast<std::uint32_t>(profileFrame.size());
        std::vector<std::uint8_t> profileBytes = profileFrame;
        profileBytes.insert(profileBytes.end(), nextFrame.begin(),
                            nextFrame.end());
        AssociativeConstraintGroupDispatchReader profileReader(
            profileBytes, classNumber);
        PersistentManagerDispatchSink profileSink;
        const bool profileRead = profileReader.readObjects(profileBytes,
            {{sourceHandle, 0u, 0u}, {sentinelHandle, nextOffset, 1u}},
            profileSink);
        const auto partialSource = std::find_if(
            profileSink.typed.begin(), profileSink.typed.end(),
            [sourceHandle](const DRW_AssociativeObject& value) {
                return value.handle == sourceHandle;
            });
        const auto partialRaw = std::find_if(
            profileSink.raw.begin(), profileSink.raw.end(),
            [sourceHandle](const DRW_UnsupportedObject& value) {
                return value.m_handle == sourceHandle;
            });
        const bool expectedComplete = arcMode == 0;
        bool nodeMatches = partialSource != profileSink.typed.end()
            && partialSource->m_constraintNodes.size() == 1u;
        if (nodeMatches) {
            const auto& node = partialSource->m_constraintNodes.front();
            nodeMatches = node.m_className == "AcConstrainedArc"
                && node.m_nodeId == 163
                && node.m_connections
                       == std::vector<std::int32_t>{165, 167, 169, 170}
                && node.m_classDataParsed == expectedComplete;
            if (expectedComplete) {
                nodeMatches = nodeMatches
                    && node.m_dataKind
                           == DRW_AssociativeConstraintNode::DataKind::Arc
                    && node.m_hasGeometryDependency
                    && node.m_geometryDependencyCode == DRW::DwgSoftPointer
                    && node.m_geometryDependencyHandle == 1404u
                    && node.m_geometryNodeId == 0
                    && node.m_center.x == arc.center.x
                    && node.m_center.y == arc.center.y
                    && node.m_center.z == arc.center.z
                    && node.m_normal.x == 0.0
                    && node.m_normal.y == 0.0
                    && node.m_normal.z == 1.0
                    && node.m_direction.x == 1.0
                    && node.m_direction.y == 0.0
                    && node.m_direction.z == 0.0
                    && node.m_radius == arc.radius
                    && node.m_startParameter == arc.startParameter
                    && node.m_endParameter == arc.endParameter
                    && !node.m_hasReserved
                    && node.m_reserved == 0.0
                    && node.m_startPoint.x == arc.startPoint.x
                    && node.m_startPoint.y == arc.startPoint.y
                    && node.m_startPoint.z == arc.startPoint.z
                    && node.m_endPoint.x == arc.endPoint.x
                    && node.m_endPoint.y == arc.endPoint.y
                    && node.m_endPoint.z == arc.endPoint.z;
            }
        }
        const bool profileMatches = profileRead
            && profileReader.objectFailures() == 0u
            && partialSource != profileSink.typed.end()
            && partialSource->m_constraintGroupBodyParsed == expectedComplete
            && partialSource->m_constraintNodeCount == 1
            && partialSource->m_constraintNodesParsed == expectedComplete
            && nodeMatches
            && partialRaw != profileSink.raw.end()
            && !partialRaw->m_typedPayloadValidated
            && std::any_of(profileSink.typed.begin(), profileSink.typed.end(),
                [sentinelHandle](const DRW_AssociativeObject& value) {
                    return value.handle == sentinelHandle
                        && value.m_constraintGroupBodyParsed;
                });
        t.expect(profileMatches,
                 arcMode == 0
                     ? "R2010 arc body decodes all observed fields with the reserved double absent"
                     : "arc wrong dependency handle role preserves raw data and following-object recovery");
    }

    for (int circleMode = 0; circleMode < 2; ++circleMode) {
        Options profile;
        profile.classNumber = classNumber;
        profile.handle = sourceHandle;
        profile.groupVersion = 0;
        profile.nextNodeId = 523;
        profile.actions.clear();
        Node circle;
        circle.id = 17;
        circle.connections = {19, 158, 162, 170, 178, 186, 194};
        circle.className = "AcConstrainedCircle";
        circle.hasCircleBody = true;
        circle.geometryDependencyCode = circleMode == 0
            ? DRW::DwgSoftPointer : DRW::DwgHardPointer;
        circle.geometryDependencyHandle = 1165u;
        circle.geometryNodeId = 0;
        circle.center = DRW_Coord(63.11370687434459,
                                  37.572948833487146, 0.0);
        circle.normal = DRW_Coord(0.0, 0.0, 1.0);
        circle.direction = DRW_Coord(1.0, 0.0, 0.0);
        circle.radius = 3.0;
        circle.startParameter = 0.0;
        circle.endParameter = 6.283185307179586;
        profile.nodes.push_back(circle);

        Frame profileObject;
        Frame nextObject;
        std::vector<std::uint8_t> profileFrame;
        std::vector<std::uint8_t> nextFrame;
        const bool profileBuilt = makeFrame(profile, profileObject)
            && wrapFrame(profileObject, profileFrame)
            && makeFrame(sentinel, nextObject)
            && wrapFrame(nextObject, nextFrame);
        t.expect(profileBuilt,
                 "AC1024 circle profiles are generated from scratch");
        if (!profileBuilt)
            continue;

        const auto nextOffset = static_cast<std::uint32_t>(profileFrame.size());
        std::vector<std::uint8_t> profileBytes = profileFrame;
        profileBytes.insert(profileBytes.end(), nextFrame.begin(),
                            nextFrame.end());
        AssociativeConstraintGroupDispatchReader profileReader(
            profileBytes, classNumber);
        PersistentManagerDispatchSink profileSink;
        const bool profileRead = profileReader.readObjects(profileBytes,
            {{sourceHandle, 0u, 0u}, {sentinelHandle, nextOffset, 1u}},
            profileSink);
        const auto partialSource = std::find_if(
            profileSink.typed.begin(), profileSink.typed.end(),
            [sourceHandle](const DRW_AssociativeObject& value) {
                return value.handle == sourceHandle;
            });
        const auto partialRaw = std::find_if(
            profileSink.raw.begin(), profileSink.raw.end(),
            [sourceHandle](const DRW_UnsupportedObject& value) {
                return value.m_handle == sourceHandle;
            });
        const bool expectedComplete = circleMode == 0;
        bool nodeMatches = partialSource != profileSink.typed.end()
            && partialSource->m_constraintNodes.size() == 1u;
        if (nodeMatches) {
            const auto& node = partialSource->m_constraintNodes.front();
            nodeMatches = node.m_className == "AcConstrainedCircle"
                && node.m_nodeId == 17
                && node.m_connections
                       == std::vector<std::int32_t>{19, 158, 162, 170,
                                                    178, 186, 194}
                && node.m_classDataParsed == expectedComplete;
            if (expectedComplete) {
                nodeMatches = nodeMatches
                    && node.m_dataKind
                           == DRW_AssociativeConstraintNode::DataKind::Circle
                    && node.m_hasGeometryDependency
                    && node.m_geometryDependencyCode == DRW::DwgSoftPointer
                    && node.m_geometryDependencyHandle == 1165u
                    && node.m_geometryNodeId == 0
                    && node.m_center.x == circle.center.x
                    && node.m_center.y == circle.center.y
                    && node.m_center.z == circle.center.z
                    && node.m_normal.x == 0.0
                    && node.m_normal.y == 0.0
                    && node.m_normal.z == 1.0
                    && node.m_direction.x == 1.0
                    && node.m_direction.y == 0.0
                    && node.m_direction.z == 0.0
                    && node.m_radius == circle.radius
                    && node.m_startParameter == circle.startParameter
                    && node.m_endParameter == circle.endParameter
                    && !node.m_hasReserved
                    && node.m_reserved == 0.0;
            }
        }
        const bool profileMatches = profileRead
            && profileReader.objectFailures() == 0u
            && partialSource != profileSink.typed.end()
            && partialSource->m_constraintGroupBodyParsed == expectedComplete
            && partialSource->m_constraintNodeCount == 1
            && partialSource->m_constraintNodesParsed == expectedComplete
            && nodeMatches
            && partialRaw != profileSink.raw.end()
            && !partialRaw->m_typedPayloadValidated
            && std::any_of(profileSink.typed.begin(), profileSink.typed.end(),
                [sentinelHandle](const DRW_AssociativeObject& value) {
                    return value.handle == sentinelHandle
                        && value.m_constraintGroupBodyParsed;
                });
        t.expect(profileMatches,
                 circleMode == 0
                     ? "R2010 circle body decodes all observed fields with the reserved double absent"
                     : "circle wrong dependency handle role preserves raw data and following-object recovery");
    }

    for (int horizontalMode = 0; horizontalMode < 2; ++horizontalMode) {
        Options profile;
        profile.classNumber = classNumber;
        profile.handle = sourceHandle;
        profile.groupVersion = 0;
        profile.nextNodeId = 523;
        profile.actions.clear();

        Node horizontal;
        horizontal.id = 414;
        horizontal.connections = {379};
        horizontal.className = "AcHorizontalConstraint";
        horizontal.hasHorizontalBody = true;
        horizontal.ownerId = 0;
        horizontal.isImplied = false;
        horizontal.datumLineIndex = 412;
        horizontal.hasDatumLineIndex = horizontalMode == 0;
        profile.nodes.push_back(horizontal);

        Node implicitPoint;
        if (horizontalMode == 0) {
            implicitPoint.id = 415;
            implicitPoint.connections = {414};
            implicitPoint.className = "AcConstrainedImplicitPoint";
            implicitPoint.hasImplicitPointBody = true;
            implicitPoint.geometryDependencyCode = DRW::DwgSoftPointer;
            implicitPoint.geometryDependencyHandle = 0xA007u;
            implicitPoint.geometryNodeId = 0;
            implicitPoint.hasPoint = true;
            implicitPoint.point = DRW_Coord(5.0, 6.0, 7.0);
            implicitPoint.pointType = 1u;
            implicitPoint.pointIndex = 2;
            implicitPoint.curveNodeId = 379;
            profile.nodes.push_back(implicitPoint);
        }

        Frame profileObject;
        Frame nextObject;
        std::vector<std::uint8_t> profileFrame;
        std::vector<std::uint8_t> nextFrame;
        const bool profileBuilt = makeFrame(profile, profileObject)
            && wrapFrame(profileObject, profileFrame)
            && makeFrame(sentinel, nextObject)
            && wrapFrame(nextObject, nextFrame);
        t.expect(profileBuilt,
                 "AC1024 Horizontal profiles are generated from scratch");
        if (!profileBuilt)
            continue;

        const auto nextOffset = static_cast<std::uint32_t>(profileFrame.size());
        std::vector<std::uint8_t> profileBytes = profileFrame;
        profileBytes.insert(profileBytes.end(), nextFrame.begin(),
                            nextFrame.end());
        AssociativeConstraintGroupDispatchReader profileReader(
            profileBytes, classNumber);
        PersistentManagerDispatchSink profileSink;
        const bool profileRead = profileReader.readObjects(profileBytes,
            {{sourceHandle, 0u, 0u}, {sentinelHandle, nextOffset, 1u}},
            profileSink);
        const auto partialSource = std::find_if(
            profileSink.typed.begin(), profileSink.typed.end(),
            [sourceHandle](const DRW_AssociativeObject& value) {
                return value.handle == sourceHandle;
            });
        const auto partialRaw = std::find_if(
            profileSink.raw.begin(), profileSink.raw.end(),
            [sourceHandle](const DRW_UnsupportedObject& value) {
                return value.m_handle == sourceHandle;
            });
        const bool expectedComplete = horizontalMode == 0;
        bool nodesMatch = partialSource != profileSink.typed.end()
            && partialSource->m_constraintNodes.size()
                   == (expectedComplete ? 2u : 1u);
        if (nodesMatch) {
            const auto& parsedHorizontal =
                partialSource->m_constraintNodes.front();
            nodesMatch = parsedHorizontal.m_className
                    == "AcHorizontalConstraint"
                && parsedHorizontal.m_nodeId == 414
                && parsedHorizontal.m_connections
                       == std::vector<std::int32_t>{379}
                && parsedHorizontal.m_ownerId == 0
                && !parsedHorizontal.m_isImplied
                && !parsedHorizontal.m_hasActiveFlag
                && parsedHorizontal.m_classDataParsed == expectedComplete;
            if (expectedComplete) {
                nodesMatch = nodesMatch
                    && parsedHorizontal.m_dataKind
                           == DRW_AssociativeConstraintNode::DataKind::Horizontal
                    && parsedHorizontal.m_hasDatumLineIndex
                    && parsedHorizontal.m_datumLineIndex == 412;
                if (nodesMatch) {
                    const auto& parsedPoint =
                        partialSource->m_constraintNodes[1];
                    nodesMatch = parsedPoint.m_className
                            == "AcConstrainedImplicitPoint"
                        && parsedPoint.m_nodeId == 415
                        && parsedPoint.m_dataKind
                               == DRW_AssociativeConstraintNode::DataKind::ImplicitPoint
                        && parsedPoint.m_hasGeometryDependency
                        && parsedPoint.m_geometryDependencyCode
                               == DRW::DwgSoftPointer
                        && parsedPoint.m_geometryDependencyHandle == 0xA007u
                        && parsedPoint.m_hasPoint
                        && parsedPoint.m_point.x == 5.0
                        && parsedPoint.m_point.y == 6.0
                        && parsedPoint.m_point.z == 7.0
                        && parsedPoint.m_pointType == 1u
                        && parsedPoint.m_pointIndex == 2
                        && parsedPoint.m_curveNodeId == 379;
                }
            } else {
                nodesMatch = nodesMatch
                    && parsedHorizontal.m_dataKind
                           == DRW_AssociativeConstraintNode::DataKind::Unsupported
                    && !parsedHorizontal.m_hasDatumLineIndex;
            }
        }
        const bool profileMatches = profileRead
            && profileReader.objectFailures() == 0u
            && partialSource != profileSink.typed.end()
            && partialSource->m_constraintGroupBodyParsed == expectedComplete
            && partialSource->m_constraintNodeCount
                   == (expectedComplete ? 2 : 1)
            && partialSource->m_constraintNodesParsed == expectedComplete
            && partialSource->m_prefixStatuses.size() == 2u
            && partialSource->m_prefixStatuses.back().m_status
                   == (expectedComplete
                       ? DRW_AssociativePrefixStatus::ParseStatus::Complete
                       : DRW_AssociativePrefixStatus::ParseStatus::Partial)
            && nodesMatch
            && partialRaw != profileSink.raw.end()
            && !partialRaw->m_typedPayloadValidated
            && std::any_of(profileSink.typed.begin(), profileSink.typed.end(),
                [sentinelHandle](const DRW_AssociativeObject& value) {
                    return value.handle == sentinelHandle
                        && value.m_constraintGroupBodyParsed;
                });
        t.expect(profileMatches,
                 horizontalMode == 0
                     ? "Horizontal datum index decodes without a handle and preserves the following implicit-point fields and handle"
                     : "missing Horizontal datum index stays raw and preserves following-OBJECT recovery");
    }

    for (int verticalMode = 0; verticalMode < 2; ++verticalMode) {
        Options profile;
        profile.classNumber = classNumber;
        profile.handle = sourceHandle;
        profile.groupVersion = 0;
        profile.nextNodeId = 523;
        profile.actions.clear();

        Node datumLine;
        datumLine.id = 69;
        datumLine.className = "AcConstrainedDatumLine";
        datumLine.hasDatumLineBody = true;
        datumLine.geometryDependencyCode = DRW::DwgSoftPointer;
        datumLine.geometryDependencyHandle = 0u;
        datumLine.direction = DRW_Coord(1.0, 0.0, 0.0);
        profile.nodes.push_back(datumLine);

        Node vertical;
        vertical.id = 122;
        vertical.connections = {69};
        vertical.className = "AcVerticalConstraint";
        vertical.hasVerticalBody = true;
        vertical.ownerId = 0;
        vertical.isImplied = false;
        vertical.datumLineIndex = 69;
        vertical.hasDatumLineIndex = verticalMode == 0;
        profile.nodes.push_back(vertical);

        Node implicitPoint;
        if (verticalMode == 0) {
            implicitPoint.id = 123;
            implicitPoint.connections = {122};
            implicitPoint.className = "AcConstrainedImplicitPoint";
            implicitPoint.hasImplicitPointBody = true;
            implicitPoint.geometryDependencyCode = DRW::DwgSoftPointer;
            implicitPoint.geometryDependencyHandle = 0xA007u;
            implicitPoint.geometryNodeId = 0;
            implicitPoint.hasPoint = true;
            implicitPoint.point = DRW_Coord(5.0, 6.0, 7.0);
            implicitPoint.pointType = 1u;
            implicitPoint.pointIndex = 2;
            implicitPoint.curveNodeId = 69;
            profile.nodes.push_back(implicitPoint);
        }

        Frame profileObject;
        Frame nextObject;
        std::vector<std::uint8_t> profileFrame;
        std::vector<std::uint8_t> nextFrame;
        const bool profileBuilt = makeFrame(profile, profileObject)
            && wrapFrame(profileObject, profileFrame)
            && makeFrame(sentinel, nextObject)
            && wrapFrame(nextObject, nextFrame);
        t.expect(profileBuilt,
                 "AC1024 Vertical profiles are generated from scratch");
        if (!profileBuilt)
            continue;

        const auto nextOffset = static_cast<std::uint32_t>(profileFrame.size());
        std::vector<std::uint8_t> profileBytes = profileFrame;
        profileBytes.insert(profileBytes.end(), nextFrame.begin(),
                            nextFrame.end());
        AssociativeConstraintGroupDispatchReader profileReader(
            profileBytes, classNumber);
        PersistentManagerDispatchSink profileSink;
        const bool profileRead = profileReader.readObjects(profileBytes,
            {{sourceHandle, 0u, 0u}, {sentinelHandle, nextOffset, 1u}},
            profileSink);
        const auto parsedSource = std::find_if(
            profileSink.typed.begin(), profileSink.typed.end(),
            [sourceHandle](const DRW_AssociativeObject& value) {
                return value.handle == sourceHandle;
            });
        const auto rawSource = std::find_if(
            profileSink.raw.begin(), profileSink.raw.end(),
            [sourceHandle](const DRW_UnsupportedObject& value) {
                return value.m_handle == sourceHandle;
            });
        const bool expectedComplete = verticalMode == 0;
        const std::size_t expectedNodeCount = expectedComplete ? 3u : 2u;
        bool nodesMatch = parsedSource != profileSink.typed.end()
            && parsedSource->m_constraintNodes.size() == expectedNodeCount;
        if (nodesMatch) {
            const auto& parsedDatumLine =
                parsedSource->m_constraintNodes[0];
            const auto& parsedVertical =
                parsedSource->m_constraintNodes[1];
            nodesMatch = parsedDatumLine.m_className
                    == "AcConstrainedDatumLine"
                && parsedDatumLine.m_nodeId == 69
                && parsedDatumLine.m_classDataParsed
                && parsedDatumLine.m_dataKind
                       == DRW_AssociativeConstraintNode::DataKind::Line
                && parsedVertical.m_className == "AcVerticalConstraint"
                && parsedVertical.m_nodeId == 122
                && parsedVertical.m_connections
                       == std::vector<std::int32_t>{69}
                && parsedVertical.m_ownerId == 0
                && !parsedVertical.m_isImplied
                && !parsedVertical.m_hasActiveFlag
                && parsedVertical.m_classDataParsed == expectedComplete;
            if (expectedComplete) {
                nodesMatch = nodesMatch
                    && parsedVertical.m_dataKind
                           == DRW_AssociativeConstraintNode::DataKind::Vertical
                    && parsedVertical.m_hasDatumLineIndex
                    && parsedVertical.m_datumLineIndex == 69;
                if (nodesMatch) {
                    const auto& parsedPoint =
                        parsedSource->m_constraintNodes[2];
                    nodesMatch = parsedPoint.m_className
                            == "AcConstrainedImplicitPoint"
                        && parsedPoint.m_nodeId == 123
                        && parsedPoint.m_dataKind
                               == DRW_AssociativeConstraintNode::DataKind::ImplicitPoint
                        && parsedPoint.m_hasGeometryDependency
                        && parsedPoint.m_geometryDependencyCode
                               == DRW::DwgSoftPointer
                        && parsedPoint.m_geometryDependencyHandle == 0xA007u
                        && parsedPoint.m_hasPoint
                        && parsedPoint.m_point.x == 5.0
                        && parsedPoint.m_point.y == 6.0
                        && parsedPoint.m_point.z == 7.0
                        && parsedPoint.m_pointType == 1u
                        && parsedPoint.m_pointIndex == 2
                        && parsedPoint.m_curveNodeId == 69;
                }
            } else {
                nodesMatch = nodesMatch
                    && parsedVertical.m_dataKind
                           == DRW_AssociativeConstraintNode::DataKind::Unsupported
                    && !parsedVertical.m_hasDatumLineIndex;
            }
        }
        const bool profileMatches = profileRead
            && profileReader.objectFailures() == 0u
            && parsedSource != profileSink.typed.end()
            && parsedSource->m_constraintGroupBodyParsed == expectedComplete
            && parsedSource->m_constraintNodeCount
                   == static_cast<std::int32_t>(expectedNodeCount)
            && parsedSource->m_constraintNodesParsed == expectedComplete
            && nodesMatch
            && rawSource != profileSink.raw.end()
            && !rawSource->m_typedPayloadValidated
            && std::any_of(profileSink.typed.begin(), profileSink.typed.end(),
                [sentinelHandle](const DRW_AssociativeObject& value) {
                    return value.handle == sentinelHandle
                        && value.m_constraintGroupBodyParsed;
                });
        t.expect(profileMatches,
                 expectedComplete
                     ? "Vertical datum-line index decodes with no node handle and preserves following geometry fields and handle alignment"
                     : "missing Vertical datum-line index stays raw and preserves following-OBJECT recovery");
    }

    for (int rigidSetMode = 0; rigidSetMode < 5; ++rigidSetMode) {
        Options profile;
        profile.classNumber = classNumber;
        profile.handle = sourceHandle;
        profile.groupVersion = 0;
        profile.nextNodeId = 800;
        profile.actions.clear();

        Node rigidSet;
        rigidSet.id = 768;
        rigidSet.className = "AcConstrainedRigidSet";
        rigidSet.hasRigidSetBody = true;
        rigidSet.geometryDependencyCode = rigidSetMode == 1
            ? DRW::DwgHardPointer : DRW::DwgSoftPointer;
        rigidSet.geometryDependencyHandle = 0u;
        rigidSet.geometryNodeId = 0;
        rigidSet.rigidSetReservedFlag = false;
        rigidSet.rigidSetTransform = {{
            1.0, 0.0, 0.0, 384.02944432664754,
            0.0, 1.0, 0.0, 167.53646572387899,
            0.0, 0.0, 1.0, 0.0,
            0.0, 0.0, 0.0, 1.0
        }};
        rigidSet.rigidSetGeometryIds = {767, 769, 771};
        if (rigidSetMode == 2) {
            rigidSet.hasRigidSetTransform = false;
            rigidSet.hasRigidSetGeometryCount = false;
            rigidSet.hasRigidSetGeometryIds = false;
        } else if (rigidSetMode == 3) {
            rigidSet.declaredRigidSetGeometryCount = 3;
            rigidSet.rigidSetGeometryIds = {767, 769};
        } else if (rigidSetMode == 4) {
            rigidSet.declaredRigidSetGeometryCount = -1;
            rigidSet.rigidSetGeometryIds.clear();
        }
        profile.nodes.push_back(rigidSet);

        Node implicitPoint;
        if (rigidSetMode == 0) {
            implicitPoint.id = 772;
            implicitPoint.connections = {768};
            implicitPoint.className = "AcConstrainedImplicitPoint";
            implicitPoint.hasImplicitPointBody = true;
            implicitPoint.geometryDependencyCode = DRW::DwgSoftPointer;
            implicitPoint.geometryDependencyHandle = 0xA007u;
            implicitPoint.geometryNodeId = 0;
            implicitPoint.hasPoint = true;
            implicitPoint.point = DRW_Coord(5.0, 6.0, 7.0);
            implicitPoint.pointType = 1u;
            implicitPoint.pointIndex = 2;
            implicitPoint.curveNodeId = 767;
            profile.nodes.push_back(implicitPoint);
        }

        Frame profileObject;
        Frame nextObject;
        std::vector<std::uint8_t> profileFrame;
        std::vector<std::uint8_t> nextFrame;
        const bool profileBuilt = makeFrame(profile, profileObject)
            && wrapFrame(profileObject, profileFrame)
            && makeFrame(sentinel, nextObject)
            && wrapFrame(nextObject, nextFrame);
        t.expect(profileBuilt,
                 "AC1024 rigid-set profiles are generated from scratch");
        if (!profileBuilt)
            continue;

        const auto nextOffset = static_cast<std::uint32_t>(profileFrame.size());
        std::vector<std::uint8_t> profileBytes = profileFrame;
        profileBytes.insert(profileBytes.end(), nextFrame.begin(),
                            nextFrame.end());
        AssociativeConstraintGroupDispatchReader profileReader(
            profileBytes, classNumber);
        PersistentManagerDispatchSink profileSink;
        const bool profileRead = profileReader.readObjects(profileBytes,
            {{sourceHandle, 0u, 0u}, {sentinelHandle, nextOffset, 1u}},
            profileSink);
        const auto parsedSource = std::find_if(
            profileSink.typed.begin(), profileSink.typed.end(),
            [sourceHandle](const DRW_AssociativeObject& value) {
                return value.handle == sourceHandle;
            });
        const auto rawSource = std::find_if(
            profileSink.raw.begin(), profileSink.raw.end(),
            [sourceHandle](const DRW_UnsupportedObject& value) {
                return value.m_handle == sourceHandle;
            });
        const bool expectedComplete = rigidSetMode == 0;
        const std::size_t expectedNodeCount = expectedComplete ? 2u : 1u;
        bool nodesMatch = parsedSource != profileSink.typed.end()
            && parsedSource->m_constraintNodes.size() == expectedNodeCount;
        if (nodesMatch) {
            const auto& parsedRigidSet =
                parsedSource->m_constraintNodes.front();
            nodesMatch = parsedRigidSet.m_className
                    == "AcConstrainedRigidSet"
                && parsedRigidSet.m_nodeId == 768
                && parsedRigidSet.m_connections.empty()
                && parsedRigidSet.m_classDataParsed == expectedComplete;
            if (expectedComplete) {
                nodesMatch = nodesMatch
                    && parsedRigidSet.m_dataKind
                           == DRW_AssociativeConstraintNode::DataKind::RigidSet
                    && parsedRigidSet.m_hasGeometryDependency
                    && parsedRigidSet.m_geometryDependencyCode
                           == DRW::DwgSoftPointer
                    && parsedRigidSet.m_geometryDependencyHandle == 0u
                    && parsedRigidSet.m_geometryNodeId == 0
                    && parsedRigidSet.m_hasRigidSetReservedFlag
                    && !parsedRigidSet.m_rigidSetReservedFlag
                    && parsedRigidSet.m_hasRigidSetTransform
                    && parsedRigidSet.m_rigidSetTransform
                           == rigidSet.rigidSetTransform
                    && parsedRigidSet.m_rigidSetGeometryIds
                           == std::vector<std::int32_t>{767, 769, 771};
                if (nodesMatch) {
                    const auto& parsedPoint =
                        parsedSource->m_constraintNodes[1];
                    nodesMatch = parsedPoint.m_className
                            == "AcConstrainedImplicitPoint"
                        && parsedPoint.m_nodeId == 772
                        && parsedPoint.m_dataKind
                               == DRW_AssociativeConstraintNode::DataKind::ImplicitPoint
                        && parsedPoint.m_hasGeometryDependency
                        && parsedPoint.m_geometryDependencyCode
                               == DRW::DwgSoftPointer
                        && parsedPoint.m_geometryDependencyHandle == 0xA007u
                        && parsedPoint.m_hasPoint
                        && parsedPoint.m_point.x == 5.0
                        && parsedPoint.m_point.y == 6.0
                        && parsedPoint.m_point.z == 7.0
                        && parsedPoint.m_pointType == 1u
                        && parsedPoint.m_pointIndex == 2
                        && parsedPoint.m_curveNodeId == 767;
                }
            } else {
                nodesMatch = nodesMatch
                    && parsedRigidSet.m_dataKind
                           == DRW_AssociativeConstraintNode::DataKind::Unsupported
                    && !parsedRigidSet.m_hasGeometryDependency
                    && !parsedRigidSet.m_hasRigidSetTransform
                    && parsedRigidSet.m_rigidSetGeometryIds.empty();
            }
        }
        const bool profileMatches = profileRead
            && profileReader.objectFailures() == 0u
            && parsedSource != profileSink.typed.end()
            && parsedSource->m_constraintGroupBodyParsed == expectedComplete
            && parsedSource->m_constraintNodeCount
                   == static_cast<std::int32_t>(expectedNodeCount)
            && parsedSource->m_constraintNodesParsed == expectedComplete
            && nodesMatch
            && rawSource != profileSink.raw.end()
            && !rawSource->m_typedPayloadValidated
            && std::any_of(profileSink.typed.begin(), profileSink.typed.end(),
                [sentinelHandle](const DRW_AssociativeObject& value) {
                    return value.handle == sentinelHandle
                        && value.m_constraintGroupBodyParsed;
                });
        t.expect(profileMatches,
                 expectedComplete
                     ? "RigidSet null soft pointer, ordered transform, geometry IDs and following handle decode exactly"
                     : rigidSetMode == 1
                         ? "RigidSet wrong handle role stays raw and recovers the following object"
                         : rigidSetMode == 2
                             ? "truncated RigidSet transform stays raw and recovers the following object"
                         : rigidSetMode == 3
                             ? "truncated RigidSet geometry-ID list stays raw and recovers the following object"
                             : "negative RigidSet geometry count stays raw and recovers the following object");
    }

    for (std::uint8_t mode = 0u; mode <= 3u; ++mode) {
        Options profile;
        profile.classNumber = classNumber;
        profile.handle = sourceHandle;
        profile.groupVersion = 0;
        profile.nextNodeId = 523;
        profile.actions.clear();

        Node radiusDiameter;
        radiusDiameter.id = 135;
        radiusDiameter.connections = {17, 19};
        radiusDiameter.className = "AcRadiusDiameterConstraint";
        radiusDiameter.hasRadiusDiameterBody = true;
        radiusDiameter.ownerId = 0;
        radiusDiameter.isImplied = false;
        radiusDiameter.valueDependencyCode = DRW::DwgHardPointer;
        radiusDiameter.valueDependencyHandle = 0x553u;
        radiusDiameter.dimensionDependencyCode = DRW::DwgHardPointer;
        radiusDiameter.dimensionDependencyHandle = 0x554u;
        radiusDiameter.radiusDiameterMode = mode;
        profile.nodes.push_back(radiusDiameter);

        Node implicitPoint;
        implicitPoint.id = 136;
        implicitPoint.connections = {135};
        implicitPoint.className = "AcConstrainedImplicitPoint";
        implicitPoint.hasImplicitPointBody = true;
        implicitPoint.geometryDependencyCode = DRW::DwgSoftPointer;
        implicitPoint.geometryDependencyHandle = 0xA007u;
        implicitPoint.geometryNodeId = 0;
        implicitPoint.hasPoint = true;
        implicitPoint.point = DRW_Coord(5.0, 6.0, 7.0);
        implicitPoint.pointType = 1u;
        implicitPoint.pointIndex = 2;
        implicitPoint.curveNodeId = 19;
        profile.nodes.push_back(implicitPoint);

        Frame profileObject;
        Frame nextObject;
        std::vector<std::uint8_t> profileFrame;
        std::vector<std::uint8_t> nextFrame;
        const bool profileBuilt = makeFrame(profile, profileObject)
            && wrapFrame(profileObject, profileFrame)
            && makeFrame(sentinel, nextObject)
            && wrapFrame(nextObject, nextFrame);
        t.expect(profileBuilt,
                 "AC1024 Radius/Diameter enum profiles are generated from scratch");
        if (!profileBuilt)
            continue;

        const auto nextOffset = static_cast<std::uint32_t>(profileFrame.size());
        std::vector<std::uint8_t> profileBytes = profileFrame;
        profileBytes.insert(profileBytes.end(), nextFrame.begin(),
                            nextFrame.end());
        AssociativeConstraintGroupDispatchReader profileReader(
            profileBytes, classNumber);
        PersistentManagerDispatchSink profileSink;
        const bool profileRead = profileReader.readObjects(profileBytes,
            {{sourceHandle, 0u, 0u}, {sentinelHandle, nextOffset, 1u}},
            profileSink);
        const auto parsedSource = std::find_if(
            profileSink.typed.begin(), profileSink.typed.end(),
            [sourceHandle](const DRW_AssociativeObject& value) {
                return value.handle == sourceHandle;
            });
        const auto rawSource = std::find_if(
            profileSink.raw.begin(), profileSink.raw.end(),
            [sourceHandle](const DRW_UnsupportedObject& value) {
                return value.m_handle == sourceHandle;
            });
        bool nodesMatch = parsedSource != profileSink.typed.end()
            && parsedSource->m_constraintNodes.size() == 2u;
        if (nodesMatch) {
            const auto& parsedRadiusDiameter =
                parsedSource->m_constraintNodes[0];
            const auto& parsedPoint = parsedSource->m_constraintNodes[1];
            nodesMatch = parsedRadiusDiameter.m_className
                    == "AcRadiusDiameterConstraint"
                && parsedRadiusDiameter.m_nodeId == 135
                && parsedRadiusDiameter.m_connections
                       == std::vector<std::int32_t>{17, 19}
                && parsedRadiusDiameter.m_dataKind
                       == DRW_AssociativeConstraintNode::DataKind::RadiusDiameter
                && parsedRadiusDiameter.m_ownerId == 0
                && !parsedRadiusDiameter.m_isImplied
                && !parsedRadiusDiameter.m_hasActiveFlag
                && parsedRadiusDiameter.m_classDataParsed
                && parsedRadiusDiameter.m_hasValueDependencyHandle
                && parsedRadiusDiameter.m_valueDependencyCode
                       == DRW::DwgHardPointer
                && parsedRadiusDiameter.m_valueDependencyHandle == 0x553u
                && parsedRadiusDiameter.m_hasDimensionDependencyHandle
                && parsedRadiusDiameter.m_dimensionDependencyCode
                       == DRW::DwgHardPointer
                && parsedRadiusDiameter.m_dimensionDependencyHandle == 0x554u
                && parsedRadiusDiameter.m_radiusDiameterMode == mode
                && parsedPoint.m_className == "AcConstrainedImplicitPoint"
                && parsedPoint.m_nodeId == 136
                && parsedPoint.m_dataKind
                       == DRW_AssociativeConstraintNode::DataKind::ImplicitPoint
                && parsedPoint.m_hasGeometryDependency
                && parsedPoint.m_geometryDependencyCode == DRW::DwgSoftPointer
                && parsedPoint.m_geometryDependencyHandle == 0xA007u
                && parsedPoint.m_hasPoint
                && parsedPoint.m_point.x == 5.0
                && parsedPoint.m_point.y == 6.0
                && parsedPoint.m_point.z == 7.0
                && parsedPoint.m_pointType == 1u
                && parsedPoint.m_pointIndex == 2
                && parsedPoint.m_curveNodeId == 19;
        }
        const bool profileMatches = profileRead
            && profileReader.objectFailures() == 0u
            && parsedSource != profileSink.typed.end()
            && parsedSource->m_constraintGroupBodyParsed
            && parsedSource->m_constraintNodeCount == 2
            && parsedSource->m_constraintNodesParsed
            && nodesMatch
            && rawSource != profileSink.raw.end()
            && !rawSource->m_typedPayloadValidated
            && std::any_of(profileSink.typed.begin(), profileSink.typed.end(),
                [sentinelHandle](const DRW_AssociativeObject& value) {
                    return value.handle == sentinelHandle
                        && value.m_constraintGroupBodyParsed;
                });
        t.expect(profileMatches,
                 "Radius/Diameter mode, dependency roles and values preserve following implicit-point handle alignment");
    }

    for (int invalidMode = 0; invalidMode < 4; ++invalidMode) {
        Options profile;
        profile.classNumber = classNumber;
        profile.handle = sourceHandle;
        profile.groupVersion = 0;
        profile.nextNodeId = 523;
        profile.actions.clear();

        Node radiusDiameter;
        radiusDiameter.id = 135;
        radiusDiameter.connections = {17, 19};
        radiusDiameter.className = "AcRadiusDiameterConstraint";
        radiusDiameter.hasRadiusDiameterBody = true;
        radiusDiameter.ownerId = 0;
        radiusDiameter.isImplied = false;
        radiusDiameter.valueDependencyCode = invalidMode == 0
            ? DRW::DwgSoftPointer : DRW::DwgHardPointer;
        radiusDiameter.valueDependencyHandle = 0x553u;
        radiusDiameter.dimensionDependencyCode = invalidMode == 1
            ? DRW::DwgSoftPointer : DRW::DwgHardPointer;
        radiusDiameter.dimensionDependencyHandle = 0x554u;
        radiusDiameter.radiusDiameterMode = invalidMode == 2 ? 255u : 1u;
        radiusDiameter.hasRadiusDiameterMode = invalidMode != 3;
        profile.nodes.push_back(radiusDiameter);

        Frame profileObject;
        Frame nextObject;
        std::vector<std::uint8_t> profileFrame;
        std::vector<std::uint8_t> nextFrame;
        const bool profileBuilt = makeFrame(profile, profileObject)
            && wrapFrame(profileObject, profileFrame)
            && makeFrame(sentinel, nextObject)
            && wrapFrame(nextObject, nextFrame);
        t.expect(profileBuilt,
                 "AC1024 Radius/Diameter failure profiles are generated from scratch");
        if (!profileBuilt)
            continue;

        const auto nextOffset = static_cast<std::uint32_t>(profileFrame.size());
        std::vector<std::uint8_t> profileBytes = profileFrame;
        profileBytes.insert(profileBytes.end(), nextFrame.begin(),
                            nextFrame.end());
        AssociativeConstraintGroupDispatchReader profileReader(
            profileBytes, classNumber);
        PersistentManagerDispatchSink profileSink;
        const bool profileRead = profileReader.readObjects(profileBytes,
            {{sourceHandle, 0u, 0u}, {sentinelHandle, nextOffset, 1u}},
            profileSink);
        const auto parsedSource = std::find_if(
            profileSink.typed.begin(), profileSink.typed.end(),
            [sourceHandle](const DRW_AssociativeObject& value) {
                return value.handle == sourceHandle;
            });
        const auto rawSource = std::find_if(
            profileSink.raw.begin(), profileSink.raw.end(),
            [sourceHandle](const DRW_UnsupportedObject& value) {
                return value.m_handle == sourceHandle;
            });
        const bool untypedRadiusDiameter =
            parsedSource != profileSink.typed.end()
            && parsedSource->m_constraintNodes.size() == 1u
            && parsedSource->m_constraintNodes.front().m_className
                   == "AcRadiusDiameterConstraint"
            && !parsedSource->m_constraintNodes.front().m_classDataParsed
            && parsedSource->m_constraintNodes.front().m_dataKind
                   == DRW_AssociativeConstraintNode::DataKind::Unsupported
            && !parsedSource->m_constraintNodes.front()
                    .m_hasValueDependencyHandle
            && !parsedSource->m_constraintNodes.front()
                    .m_hasDimensionDependencyHandle;
        const bool profileMatches = profileRead
            && profileReader.objectFailures() == 0u
            && parsedSource != profileSink.typed.end()
            && !parsedSource->m_constraintGroupBodyParsed
            && parsedSource->m_constraintNodeCount == 1
            && !parsedSource->m_constraintNodesParsed
            && parsedSource->m_prefixStatuses.size() == 2u
            && parsedSource->m_prefixStatuses.back().m_status
                   == DRW_AssociativePrefixStatus::ParseStatus::Partial
            && untypedRadiusDiameter
            && rawSource != profileSink.raw.end()
            && !rawSource->m_typedPayloadValidated
            && std::any_of(profileSink.typed.begin(), profileSink.typed.end(),
                [sentinelHandle](const DRW_AssociativeObject& value) {
                    return value.handle == sentinelHandle
                        && value.m_constraintGroupBodyParsed;
                });
        t.expect(profileMatches,
                 invalidMode == 0
                     ? "wrong value-dependency handle role preserves Radius/Diameter raw data and following-OBJECT recovery"
                     : invalidMode == 1
                         ? "wrong dimension-dependency handle role preserves raw data and following-OBJECT recovery"
                         : invalidMode == 2
                             ? "undocumented Radius/Diameter mode remains raw with following-OBJECT recovery"
                             : "missing Radius/Diameter mode byte remains raw with following-OBJECT recovery");
    }

    {
        Options profile;
        profile.classNumber = classNumber;
        profile.handle = sourceHandle;
        profile.groupVersion = 0;
        profile.nextNodeId = 523;
        profile.actions.clear();

        Node datumLine;
        datumLine.id = 412;
        datumLine.connections = {406, 398};
        datumLine.className = "AcConstrainedDatumLine";
        datumLine.hasDatumLineBody = true;
        datumLine.geometryDependencyCode = DRW::DwgSoftPointer;
        datumLine.geometryDependencyHandle = 0u;
        datumLine.geometryNodeId = 0;
        datumLine.point = DRW_Coord(0.0, 0.0, 0.0);
        datumLine.direction = DRW_Coord(1.0, 0.0, 0.0);
        profile.nodes.push_back(datumLine);

        Node implicitPoint;
        implicitPoint.id = 413;
        implicitPoint.connections = {412};
        implicitPoint.className = "AcConstrainedImplicitPoint";
        implicitPoint.hasImplicitPointBody = true;
        implicitPoint.geometryDependencyCode = DRW::DwgSoftPointer;
        implicitPoint.geometryDependencyHandle = 0xA007u;
        implicitPoint.geometryNodeId = 0;
        implicitPoint.hasPoint = true;
        implicitPoint.point = DRW_Coord(5.0, 6.0, 7.0);
        implicitPoint.pointType = 1u;
        implicitPoint.pointIndex = 2;
        implicitPoint.curveNodeId = 19;
        profile.nodes.push_back(implicitPoint);

        Frame profileObject;
        Frame nextObject;
        std::vector<std::uint8_t> profileFrame;
        std::vector<std::uint8_t> nextFrame;
        const bool profileBuilt = makeFrame(profile, profileObject)
            && wrapFrame(profileObject, profileFrame)
            && makeFrame(sentinel, nextObject)
            && wrapFrame(nextObject, nextFrame);
        t.expect(profileBuilt,
                 "AC1024 datum-line profile is generated from scratch");
        if (profileBuilt) {
            const auto nextOffset =
                static_cast<std::uint32_t>(profileFrame.size());
            std::vector<std::uint8_t> profileBytes = profileFrame;
            profileBytes.insert(profileBytes.end(), nextFrame.begin(),
                                nextFrame.end());
            AssociativeConstraintGroupDispatchReader profileReader(
                profileBytes, classNumber);
            PersistentManagerDispatchSink profileSink;
            const bool profileRead = profileReader.readObjects(profileBytes,
                {{sourceHandle, 0u, 0u}, {sentinelHandle, nextOffset, 1u}},
                profileSink);
            const auto parsedSource = std::find_if(
                profileSink.typed.begin(), profileSink.typed.end(),
                [sourceHandle](const DRW_AssociativeObject& value) {
                    return value.handle == sourceHandle;
                });
            const auto rawSource = std::find_if(
                profileSink.raw.begin(), profileSink.raw.end(),
                [sourceHandle](const DRW_UnsupportedObject& value) {
                    return value.m_handle == sourceHandle;
                });
            bool nodesMatch = parsedSource != profileSink.typed.end()
                && parsedSource->m_constraintNodes.size() == 2u;
            if (nodesMatch) {
                const auto& parsedDatumLine =
                    parsedSource->m_constraintNodes[0];
                const auto& parsedPoint = parsedSource->m_constraintNodes[1];
                nodesMatch = parsedDatumLine.m_className
                        == "AcConstrainedDatumLine"
                    && parsedDatumLine.m_nodeId == 412
                    && parsedDatumLine.m_connections
                           == std::vector<std::int32_t>{406, 398}
                    && parsedDatumLine.m_dataKind
                           == DRW_AssociativeConstraintNode::DataKind::Line
                    && parsedDatumLine.m_classDataParsed
                    && parsedDatumLine.m_hasGeometryDependency
                    && parsedDatumLine.m_geometryDependencyCode
                           == DRW::DwgSoftPointer
                    && parsedDatumLine.m_geometryDependencyHandle == 0u
                    && parsedDatumLine.m_geometryNodeId == 0
                    && parsedDatumLine.m_hasPoint
                    && parsedDatumLine.m_point.x == 0.0
                    && parsedDatumLine.m_point.y == 0.0
                    && parsedDatumLine.m_point.z == 0.0
                    && parsedDatumLine.m_direction.x == 1.0
                    && parsedDatumLine.m_direction.y == 0.0
                    && parsedDatumLine.m_direction.z == 0.0
                    && !parsedDatumLine.m_isRay
                    && parsedPoint.m_className
                           == "AcConstrainedImplicitPoint"
                    && parsedPoint.m_nodeId == 413
                    && parsedPoint.m_dataKind
                           == DRW_AssociativeConstraintNode::DataKind::ImplicitPoint
                    && parsedPoint.m_hasGeometryDependency
                    && parsedPoint.m_geometryDependencyCode
                           == DRW::DwgSoftPointer
                    && parsedPoint.m_geometryDependencyHandle == 0xA007u
                    && parsedPoint.m_hasPoint
                    && parsedPoint.m_point.x == 5.0
                    && parsedPoint.m_point.y == 6.0
                    && parsedPoint.m_point.z == 7.0;
            }
            const bool profileMatches = profileRead
                && profileReader.objectFailures() == 0u
                && parsedSource != profileSink.typed.end()
                && parsedSource->m_constraintGroupBodyParsed
                && parsedSource->m_constraintNodeCount == 2
                && parsedSource->m_constraintNodesParsed
                && nodesMatch
                && rawSource != profileSink.raw.end()
                && !rawSource->m_typedPayloadValidated
                && std::any_of(profileSink.typed.begin(), profileSink.typed.end(),
                    [sentinelHandle](const DRW_AssociativeObject& value) {
                        return value.handle == sentinelHandle
                            && value.m_constraintGroupBodyParsed;
                    });
            t.expect(profileMatches,
                     "datum-line null soft pointer and point/direction decode while preserving the following implicit-point handle");
        }
    }

    for (int invalidMode = 0; invalidMode < 4; ++invalidMode) {
        Options profile;
        profile.classNumber = classNumber;
        profile.handle = sourceHandle;
        profile.groupVersion = 0;
        profile.nextNodeId = 523;
        profile.actions.clear();

        Node datumLine;
        datumLine.id = 412;
        datumLine.connections = {406, 398};
        datumLine.className = "AcConstrainedDatumLine";
        datumLine.hasDatumLineBody = true;
        datumLine.geometryDependencyCode = invalidMode == 0
            ? DRW::DwgHardPointer : DRW::DwgSoftPointer;
        datumLine.geometryDependencyHandle = invalidMode == 1
            ? 0xA008u : 0u;
        datumLine.geometryNodeId = 0;
        datumLine.point = DRW_Coord(0.0, 0.0, 0.0);
        datumLine.direction = invalidMode == 3
            ? DRW_Coord(0.0, 0.0, 0.0)
            : DRW_Coord(1.0, 0.0, 0.0);
        datumLine.hasDatumLineDirection = invalidMode != 2;
        profile.nodes.push_back(datumLine);

        Frame profileObject;
        Frame nextObject;
        std::vector<std::uint8_t> profileFrame;
        std::vector<std::uint8_t> nextFrame;
        const bool profileBuilt = makeFrame(profile, profileObject)
            && wrapFrame(profileObject, profileFrame)
            && makeFrame(sentinel, nextObject)
            && wrapFrame(nextObject, nextFrame);
        t.expect(profileBuilt,
                 "AC1024 datum-line failure profiles are generated from scratch");
        if (!profileBuilt)
            continue;

        const auto nextOffset = static_cast<std::uint32_t>(profileFrame.size());
        std::vector<std::uint8_t> profileBytes = profileFrame;
        profileBytes.insert(profileBytes.end(), nextFrame.begin(),
                            nextFrame.end());
        AssociativeConstraintGroupDispatchReader profileReader(
            profileBytes, classNumber);
        PersistentManagerDispatchSink profileSink;
        const bool profileRead = profileReader.readObjects(profileBytes,
            {{sourceHandle, 0u, 0u}, {sentinelHandle, nextOffset, 1u}},
            profileSink);
        const auto parsedSource = std::find_if(
            profileSink.typed.begin(), profileSink.typed.end(),
            [sourceHandle](const DRW_AssociativeObject& value) {
                return value.handle == sourceHandle;
            });
        const auto rawSource = std::find_if(
            profileSink.raw.begin(), profileSink.raw.end(),
            [sourceHandle](const DRW_UnsupportedObject& value) {
                return value.m_handle == sourceHandle;
            });
        const bool untypedDatumLine = parsedSource != profileSink.typed.end()
            && parsedSource->m_constraintNodes.size() == 1u
            && parsedSource->m_constraintNodes.front().m_className
                   == "AcConstrainedDatumLine"
            && !parsedSource->m_constraintNodes.front().m_classDataParsed
            && parsedSource->m_constraintNodes.front().m_dataKind
                   == DRW_AssociativeConstraintNode::DataKind::Unsupported
            && !parsedSource->m_constraintNodes.front()
                    .m_hasGeometryDependency;
        const bool profileMatches = profileRead
            && profileReader.objectFailures() == 0u
            && parsedSource != profileSink.typed.end()
            && !parsedSource->m_constraintGroupBodyParsed
            && parsedSource->m_constraintNodeCount == 1
            && !parsedSource->m_constraintNodesParsed
            && parsedSource->m_prefixStatuses.size() == 2u
            && parsedSource->m_prefixStatuses.back().m_status
                   == DRW_AssociativePrefixStatus::ParseStatus::Partial
            && untypedDatumLine
            && rawSource != profileSink.raw.end()
            && !rawSource->m_typedPayloadValidated
            && std::any_of(profileSink.typed.begin(), profileSink.typed.end(),
                [sentinelHandle](const DRW_AssociativeObject& value) {
                    return value.handle == sentinelHandle
                        && value.m_constraintGroupBodyParsed;
                });
        t.expect(profileMatches,
                 invalidMode == 0
                     ? "wrong datum-line dependency role preserves raw data and following-OBJECT recovery"
                     : invalidMode == 1
                         ? "non-null datum-line dependency remains raw and following-OBJECT recovers"
                         : invalidMode == 2
                             ? "truncated datum-line direction remains raw and following-OBJECT recovers"
                             : "zero datum-line direction remains raw and following-OBJECT recovers");
    }
}

void testPersistentManagerPrefixDispatch(TestContext& t) {
    using namespace persistent_manager_test;
    for (const std::uint16_t ordinal : {601u, 667u}) {
        for (int mode = 0; mode < 7; ++mode) {
            Options source;
            source.classNumber = ordinal;
            if (mode == 1)
                source.hasSubentityCount = true;
            else if (mode == 2)
                source.singleCountBit = true;
            else if (mode == 3)
                source.steps.pop_back();
            else if (mode == 4)
                source.associationSubentities = 1;
            else if (mode == 5) {
                source.version = DRW::AC1024;
                source.associationStepCount = 2;
                source.steps = {1, 2};
            } else if (mode == 6) {
                source.version = DRW::AC1024;
                source.associationStepCount = 2;
            }
            Options sentinel;
            sentinel.classNumber = ordinal;
            sentinel.version = source.version;
            sentinel.handle = 0xD64u;
            sentinel.hasSubentityCount = true;
            Frame sourceObject, sentinelObject;
            std::vector<std::uint8_t> sourceFrame, sentinelFrame;
            const bool built = makeFrame(source, sourceObject)
                && wrapFrame(source, sourceObject, sourceFrame)
                && makeFrame(sentinel, sentinelObject)
                && wrapFrame(sentinel, sentinelObject, sentinelFrame);
            t.expect(built, "Persistent-manager dispatcher controls are generated from scratch");
            if (!built)
                continue;
            const auto nextOffset = static_cast<std::uint32_t>(sourceFrame.size());
            std::vector<std::uint8_t> bytes = sourceFrame;
            bytes.insert(bytes.end(), sentinelFrame.begin(), sentinelFrame.end());
            DRW_TextCodec codec;
            codec.setVersion(source.version, false);
            dwgBuffer outer(bytes.data(), bytes.size(), &codec);
            DwgObjectFrame checked;
            t.expect(checked.readAt(outer, source.version, 0u)
                         && checked.readAt(outer, source.version, nextOffset),
                     "Partial/malformed payloads retain independently CRC-valid object frames");
            PersistentManagerDispatchReader reader(bytes, source.version, ordinal);
            PersistentManagerDispatchSink sink;
            const bool read = reader.readObjects(bytes, source.handle, nextOffset,
                                                 sentinel.handle, sink);
            const auto rawSource = std::find_if(sink.raw.begin(), sink.raw.end(),
                [&source](const DRW_UnsupportedObject& object) {
                    return object.m_handle == source.handle;
                });
            const auto rawSentinel = std::find_if(sink.raw.begin(), sink.raw.end(),
                [&sentinel](const DRW_UnsupportedObject& object) {
                    return object.m_handle == sentinel.handle;
                });
            const auto typedSource = std::find_if(sink.typed.begin(), sink.typed.end(),
                [&source](const DRW_AssociativeObject& object) {
                    return object.handle == source.handle;
                });
            const auto sourcePublication = std::find_if(
                sink.publications.begin(), sink.publications.end(),
                [&source](const DRW_DwgFramePublication& publication) {
                    return publication.m_handle == source.handle;
                });
            const auto sentinelPublication = std::find_if(
                sink.publications.begin(), sink.publications.end(),
                [&sentinel](const DRW_DwgFramePublication& publication) {
                    return publication.m_handle == sentinel.handle;
                });
            const bool validPrefix = mode <= 1 || mode == 5;
            const bool expectedPartialPrefix = mode == 0 || mode == 5;
            const bool sourcePublicationMatches = validPrefix
                ? sourcePublication != sink.publications.end()
                    && sourcePublication->m_carrier == (expectedPartialPrefix
                        ? DRW_DwgFramePublication::Carrier::Raw
                        : DRW_DwgFramePublication::Carrier::TypedAndRaw)
                : sourcePublication == sink.publications.end();
            bool partialPrefixMatches = !expectedPartialPrefix;
            if (expectedPartialPrefix && typedSource != sink.typed.end()) {
                const UTF8STRING expectedVersion = mode == 5
                    ? "AC1024" : "AC1021";
                partialPrefixMatches = typedSource->m_classVersion == 2u
                    && typedSource->m_persistentUnknown0 == 0u
                    && typedSource->m_persistentUnknown2 == 2u
                    && typedSource->m_persistentAssocStepCount
                        == static_cast<std::uint32_t>(source.associationStepCount)
                    && typedSource->m_persistentAssocSubentityCount == 0u
                    && typedSource->m_persistentSteps.size() == 2u
                    && typedSource->m_persistentSteps[0]
                        == static_cast<std::uint32_t>(source.steps[0])
                    && typedSource->m_persistentSteps[1]
                        == static_cast<std::uint32_t>(source.steps[1])
                    && typedSource->m_persistentSubentityIds.empty()
                    && typedSource->m_prefixStatuses.size() == 1u
                    && typedSource->m_prefixStatuses[0].m_status
                        == DRW_AssociativePrefixStatus::ParseStatus::Partial
                    && typedSource->m_prefixStatuses[0].m_decodedCountValue == -1
                    && typedSource->m_prefixStatuses[0].m_sourceAssumption.find(
                        expectedVersion) != UTF8STRING::npos;
            }
            const bool matches = read && reader.objectFailures() == (validPrefix ? 0u : 1u)
                         && sink.typed.size() == (validPrefix ? 2u : 1u)
                         && sink.raw.size() == 2u
                         && rawSource != sink.raw.end() && rawSentinel != sink.raw.end()
                         && rawSource->m_rawBytes == sourceObject.payload
                         && rawSource->m_objectType == ordinal
                         && rawSource->m_hasClassDefinition
                         && rawSource->m_className == "AcDbPersSubentManager"
                         && !rawSource->m_typedPayloadValidated
                         && !rawSentinel->m_typedPayloadValidated
                         && sourcePublicationMatches
                         && sentinelPublication != sink.publications.end()
                         && sentinelPublication->m_carrier
                              == DRW_DwgFramePublication::Carrier::TypedAndRaw
                         && sink.publications.size() == (validPrefix ? 2u : 1u)
                         && (typedSource != sink.typed.end()) == validPrefix
                         && partialPrefixMatches;
            t.expect(matches,
                     "Only exact source profiles publish partial/raw records; bad frames stay failed and the following object publishes");
            if (!matches) {
                std::cerr << "Persistent manager dispatch mode=" << mode
                          << " ordinal=" << ordinal << " read=" << read
                          << " failures=" << reader.objectFailures()
                          << " typed=" << sink.typed.size() << " raw=" << sink.raw.size();
                if (rawSource != sink.raw.end())
                    std::cerr << " sourceRawValidated=" << rawSource->m_typedPayloadValidated
                              << " sourceRawType=" << rawSource->m_objectType
                              << " sourceRawClass=" << rawSource->m_className
                              << " sourceRawBytesEqual=" << (rawSource->m_rawBytes == sourceObject.payload);
                if (rawSentinel != sink.raw.end())
                    std::cerr << " sentinelRawValidated=" << rawSentinel->m_typedPayloadValidated;
                std::cerr << '\n';
            }
        }
    }
}

int runPersistentManagerSourcePrefix(const std::string& path) {
    PersistentManagerDispatchSink sink(true);
    const bool read = sink.fileImport(path, &sink.data, false);
    const auto typed = std::find_if(sink.typed.begin(), sink.typed.end(),
        [](const DRW_AssociativeObject& object) { return object.handle == 0xD63u; });
    const auto raw = std::find_if(sink.raw.begin(), sink.raw.end(),
        [](const DRW_UnsupportedObject& object) { return object.m_handle == 0xD63u; });
    const auto publication = std::find_if(sink.publications.begin(), sink.publications.end(),
        [](const DRW_DwgFramePublication& value) { return value.m_handle == 0xD63u; });
    const auto& outcome = sink.data.dwgReadOutcome;
    const bool matches = read && outcome.attempted && outcome.operationSucceeded
        && outcome.sourceVersion == DRW::AC1021
        && outcome.observations.entityParseFailures == 0u
        && outcome.observations.objectParseFailures == 0u
        && typed != sink.typed.end() && raw != sink.raw.end()
        && publication != sink.publications.end()
        && typed->m_classVersion == 2u && typed->m_persistentUnknown0 == 0u
        && typed->m_persistentUnknown2 == 2u
        && typed->m_persistentAssocStepCount == 4u
        && typed->m_persistentAssocSubentityCount == 0u
        && typed->m_persistentSteps == std::vector<std::uint32_t>{3u, 4u}
        && typed->parentHandle == 0xCu
        && typed->reactorHandles == std::vector<std::uint32_t>{0xCu}
        && typed->m_prefixStatuses.size() == 1u
        && typed->m_prefixStatuses[0].m_status
             == DRW_AssociativePrefixStatus::ParseStatus::Partial
        && typed->m_prefixStatuses[0].m_decodedCountValue == -1
        && !raw->m_typedPayloadValidated && !raw->m_rawBytes.empty()
        && raw->m_recordName == "ACDBPERSSUBENTMANAGER"
        && publication->m_carrier == DRW_DwgFramePublication::Carrier::Raw;
    if (!matches) {
        std::cerr << "Pinned AC1021 persistent-manager prefix disposition mismatch\n";
        dx_iface::reportDwgReadOutcome(std::cerr, outcome);
        return 1;
    }
    std::cout << "Pinned AC1021 D63: partial eight-scalar prefix + exact raw carrier; "
                 "missing final count not inferred: PASS\n";
    dx_iface::reportDwgReadOutcome(std::cout, outcome);
    return 0;
}

class NodSink : public dx_iface {
public:
    NodSink() { cData = &data; currentBlock = data.mBlock; }
    void addDictionary(const DRW_Dictionary& object) override {
        ++dictionaries;
        dx_iface::addDictionary(object);
    }
    void addNamedObjectDictionary(const DRW_Dictionary& object) override {
        ++roots;
        dx_iface::addNamedObjectDictionary(object);
    }
    void addXRecord(const DRW_XRecord& object) override {
        ++records;
        dx_iface::addXRecord(object);
    }
    dx_data data;
    std::size_t roots{0}, dictionaries{0}, records{0};
};

class NodCommonProbe final : public DRW_Dictionary {
public:
    bool readCommon(dwgBuffer* body, dwgBuffer* strings, std::uint32_t handleBits) {
        return DRW_TableEntry::parseDwg(DRW::AC1032, body, strings, handleBits);
    }
};

bool malformNodCount(TestContext& t, std::vector<std::uint8_t>& bytes,
                     std::uint32_t offset) {
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1032, false);
    dwgBuffer input(bytes.data(), bytes.size(), &codec);
    DwgObjectFrame frame;
    if (!frame.readAt(input, DRW::AC1032, offset)) return false;
    dwgBuffer body(frame.body().data(), frame.body().size(), &codec);
    dwgBuffer strings = body;
    NodCommonProbe common;
    if (!common.readCommon(&body, &strings, frame.bodyBitSize())) return false;
    const auto countBit = body.getPosition() * 8u + body.getBitPos();
    if (body.getBitLong() != 2) return false;
    (void)input.setPosition(offset);
    (void)input.getModularShort();
    (void)input.getUModularChar();
    const auto bodyOffset = input.getPosition();
    const auto bit = bodyOffset * 8u + countBit + 1u;
    // BL2's01 prefix becomes00 (a raw32-bit count) inside the same frame.
    // Body bounds/CRC stay valid, but the declared dictionary body does not.
    bytes[bit / 8u] &= static_cast<std::uint8_t>(~(0x80u >> (bit & 7u)));
    const auto crcOffset = bodyOffset + frame.body().size();
    const auto crc = headerTestCrc(bytes, offset, crcOffset);
    bytes[crcOffset] = static_cast<std::uint8_t>(crc);
    bytes[crcOffset + 1u] = static_cast<std::uint8_t>(crc >> 8u);
    dwgBuffer verify(bytes.data(), bytes.size(), &codec);
    t.expect(frame.readAt(verify, DRW::AC1032, offset),
             "malformed NOD count is still an exact CRC-valid bounded object frame");
    return true;
}

void testDwgNodReference(TestContext& t) {
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-nod-" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!std::filesystem::create_directory(directory)) {
        t.expect(false, "NOD test owns a fresh scratch directory");
        return;
    }
    struct Cleanup {
        std::filesystem::path directory;
        ~Cleanup() { std::error_code error; std::filesystem::remove_all(directory, error); }
    } cleanup{directory};
    DRW_Header fresh;
    t.expect(!fresh.dwgNamedObjectDictionaryReference(), "fresh HEADER has no DWG root provenance");
    for (const auto& row : {
            std::pair<std::uint8_t, std::uint64_t>{DRW::DwgHardOwnership, 0xA000u},
            {DRW::DwgSoftOwnership, 0u}, {DRW::DwgHardPointer, 0xA000u},
            {6u, 0u}, {DRW::DwgHardOwnership, 0x10000A000ULL}}) {
        auto bytes = headerWithNodReference(t, row.first, row.second);
        if (bytes.empty()) continue;
        HeaderSectionReader reader(bytes, 0u, DRW::AC1032, 6u);
        DRW_Header parsed;
        const bool accepted = reader.readDwgHeader(parsed);
        const auto reference = parsed.dwgNamedObjectDictionaryReference();
        t.expect(accepted && reference && reference->code == row.first
                     && reference->ref64 == row.second
                     && reference->ref == static_cast<std::uint32_t>(row.second),
                 "HEADER retains complete role/null/wide NOD H without truncation or guessing");
        if (!reference) continue;
        const bool usable = row.first == DRW::DwgSoftOwnership
            || (row.first == DRW::DwgHardOwnership && row.second <= 0xFFFFFFFFu);
        t.expect(DRW_Header::isUsableDwgNamedObjectDictionaryReference(*reference) == usable
                     && reader.diagnostics().size() == (usable ? 0u : 1u),
                 "invalid NOD role/width is observable while ordinary HEADER reading stays tolerant");
        if (!usable && !reader.diagnostics().empty())
            t.expect(reader.diagnostics().back().kind
                         == DwgIntegrityCheckKind::NamedObjectDictionaryReference
                         && reader.diagnostics().back().severity == DwgIntegritySeverity::Warning
                         && !reader.diagnostics().back().hasFileOffset
                         && reader.diagnostics().back().hasObserved
                         && reader.diagnostics().back().observed
                              == (row.second > 0xFFFFFFFFu ? row.second : row.first),
                     "NOD warning uses its appended optional diagnostic category");
        if (!usable) {
            dx_data incomplete;
            incomplete.headerC = parsed;
            dx_iface converter;
            const auto destination = directory / "bad-reference.dxf";
            { std::ofstream existing(destination); existing << "NOD_SENTINEL"; }
            t.expect(!converter.fileExport(destination.string(), DRW::AC1032, false,
                                           &incomplete, false, true),
                     "strict converter refuses invalid/wide NOD source before output publication");
            std::ifstream unchanged(destination);
            const std::string content((std::istreambuf_iterator<char>(unchanged)),
                                       std::istreambuf_iterator<char>());
            t.expect(content == "NOD_SENTINEL", "invalid NOD reference preserves destination");
        }
        bytes.back() ^= 1u;
        t.expect(!reader.readDwgHeader(parsed) && !parsed.dwgNamedObjectDictionaryReference(),
                 "failed HEADER reuse clears prior NOD provenance");
        bytes.back() ^= 1u;
        t.expect(reader.readDwgHeader(parsed) && parsed.dwgNamedObjectDictionaryReference(),
                 "corrected same-reader HEADER restores NOD provenance");
        HeaderBodyProbe direct;
        static_cast<DRW_Header&>(direct) = parsed;
        t.expect(!direct.parseDwg(DRW::AC1032, nullptr, nullptr)
                     && !direct.dwgNamedObjectDictionaryReference(),
                 "null direct HEADER parse cannot retain an old NOD reference");
    }

    auto headerBytes = headerWithNodReference(t, DRW::DwgHardOwnership, 0xA000u);
    if (headerBytes.empty()) return;
    HeaderSectionReader headerReader(headerBytes, 0u, DRW::AC1032, 6u);
    DRW_Header header;
    t.expect(headerReader.readDwgHeader(header), "object-root controls have a decoded source HEADER");
    for (const std::string variant : {"valid", "missing", "wrong-type", "wrong-owner", "malformed"}) {
        NodObjectWriter writer(nullptr, &fresh);
        DRW_Dictionary child;
        child.handle = 0x100u;
        child.parentHandle = 0xA000u;
        t.expect(writer.writeDictionary(child), "lower-handle dictionary emits before the root");
        if (variant != "missing") {
            if (variant == "wrong-type") {
                DRW_XRecord wrong;
                wrong.handle = 0xA000u;
                t.expect(writer.writeXRecord(wrong), "wrong root type stays a valid object record");
            } else {
                DRW_Dictionary root;
                root.handle = 0xA000u;
                root.parentHandle = variant == "wrong-owner" ? 0xA100u : 0u;
                root.hardOwner = 1;
                root.m_entries = {{"LOCAL_CHILD", 0x100u}, {"LOCAL_RECORD", 0xA001u}};
                t.expect(writer.writeDictionary(root), "root dictionary emits complete counted entries");
            }
        }
        DRW_XRecord suffix;
        suffix.handle = 0xA001u;
        suffix.parentHandle = 0xA000u;
        suffix.m_values.emplace_back(1, UTF8STRING("NOD_SUFFIX"));
        t.expect(writer.writeXRecord(suffix), "following XRECORD is an independent suffix control");
        auto bytes = writer.buffer();
        const auto original = bytes;
        if (variant == "malformed") {
            const auto frame = std::find_if(writer.objects().begin(), writer.objects().end(),
                [](const auto& object) { return object.first == 0xA000u; });
            const bool changed = frame != writer.objects().end()
                && malformNodCount(t, bytes, frame->second);
            t.expect(changed, "NOD negative mutates only its declared body count");
            if (!changed) continue;
        }
        NodObjectReader reader(bytes);
        reader.adoptHeader(header);
        NodSink sink;
        sink.data.headerC = header;
        const bool valid = variant == "valid";
        t.expect(reader.readObjects(bytes, writer.objects(), sink)
                     && sink.roots == (valid ? 1u : 0u)
                     && sink.records == (variant == "wrong-type" ? 2u : 1u)
                     && sink.dictionaries == (variant == "wrong-owner" ? 2u : 1u),
                 "NOD selection is typed/complete/owner-checked without dropping suffixes: " + variant);
        t.expect(reader.diagnostics().size() == (valid ? 0u : 1u),
                 "missing or inconsistent NOD remains a bounded warning: " + variant);
        if (valid) {
            for (const auto& absent : {
                    std::pair<std::uint8_t, std::uint64_t>{DRW::DwgSoftOwnership, 0u},
                    {DRW::DwgHardPointer, 0xA000u},
                    {DRW::DwgHardOwnership, 0x10000A000ULL}, {6u, 0u}}) {
                auto noRootBytes = headerWithNodReference(t, absent.first, absent.second);
                if (noRootBytes.empty()) continue;
                HeaderSectionReader noRootReader(noRootBytes, 0u, DRW::AC1032, 6u);
                DRW_Header noRootHeader;
                t.expect(noRootReader.readDwgHeader(noRootHeader),
                         "NOD selector negative has a complete source HEADER");
                reader.adoptHeader(noRootHeader);
                NodSink unrelated;
                t.expect(reader.readObjects(bytes, writer.objects(), unrelated)
                             && unrelated.roots == 0u && unrelated.dictionaries == 2u
                             && unrelated.records == 1u,
                         "null/wrong-role/wide reference never aliases the matching low32 dictionary");
            }
            reader.adoptHeader(fresh);
            NodSink noSource;
            t.expect(reader.readObjects(bytes, writer.objects(), noSource)
                         && noSource.roots == 0u && noSource.dictionaries == 2u,
                     "absent source reference never promotes an ownerless dictionary");
            class LegacyDefault final : public NodSink {
            public:
                void addNamedObjectDictionary(const DRW_Dictionary& object) override {
                    DRW_Interface::addNamedObjectDictionary(object);
                }
            } legacy;
            reader.adoptHeader(header);
            t.expect(reader.readObjects(bytes, writer.objects(), legacy)
                         && legacy.roots == 0u && legacy.dictionaries == 2u,
                     "existing optional root default calls historical addDictionary exactly once");
        } else {
            if (variant == "malformed") {
                bytes = original;
                reader.adoptHeader(header);
                NodSink retry;
                t.expect(reader.readObjects(bytes, writer.objects(), retry)
                             && retry.roots == 1u && retry.records == 1u,
                         "corrected dictionary body succeeds on the same reader");
            }
            dx_iface converter;
            const auto destination = directory / (variant + ".dxf");
            t.expect(!converter.fileExport(destination.string(), DRW::AC1032, false,
                                           &sink.data, false, true)
                         && !std::filesystem::exists(destination),
                     "missing/wrong NOD cannot publish an absent destination");
            { std::ofstream existing(destination); existing << "NOD_SENTINEL"; }
            t.expect(!converter.fileExport(destination.string(), DRW::AC1032, true,
                                           &sink.data, false, true),
                     "missing/wrong NOD refuses binary publication too");
            std::ifstream unchanged(destination);
            const std::string content((std::istreambuf_iterator<char>(unchanged)),
                                       std::istreambuf_iterator<char>());
            t.expect(content == "NOD_SENTINEL", "root refusal preserves an existing destination");
            unchanged.close();
            sink.data.dictionaries.erase(std::remove_if(sink.data.dictionaries.begin(),
                sink.data.dictionaries.end(), [](const DRW_Dictionary& object) {
                    return object.handle == 0xA000u;
                }), sink.data.dictionaries.end());
            sink.data.xRecords.erase(std::remove_if(sink.data.xRecords.begin(),
                sink.data.xRecords.end(), [](const DRW_XRecord& object) {
                    return object.handle == 0xA000u;
                }), sink.data.xRecords.end());
            auto& corrected = sink.data.namedObjectDictionary;
            corrected.handle = 0xA000u;
            corrected.parentHandle = 0u;
            corrected.hardOwner = 1;
            corrected.m_entries = {{"LOCAL_CHILD", 0x100u}, {"LOCAL_RECORD", 0xA001u}};
            sink.data.hasNamedObjectDictionary = true;
            for (const bool binary : {false, true}) {
                t.expect(converter.fileExport(destination.string(), DRW::AC1032, binary,
                                              &sink.data, false, true),
                         "explicit corrected root permits same-model/converter retry");
                dx_data reopened;
                dx_iface input;
                t.expect(input.fileImport(destination.string(), &reopened, false)
                             && reopened.hasNamedObjectDictionary
                             && !reopened.headerC.dwgNamedObjectDictionaryReference()
                             && reopened.xRecords.size() == 1u
                             && sink.data.headerC.dwgNamedObjectDictionaryReference()->ref64 == 0xA000u
                             && sink.data.namedObjectDictionary.handle == 0xA000u,
                         "corrected ASCII/binary DXF preserves root/suffix and source identities");
            }
        }
    }
}

void checkHeaderVersionProfile(TestContext& t,
                               std::vector<std::uint8_t> bytes,
                               DRW::Version version, std::uint8_t maintenance,
                               bool highSizePresent, const std::string& label) {
    dwgBuffer view(bytes.data(), bytes.size());
    const bool positioned = view.setPosition(16u);
    const auto envelope = positioned
        ? inspectDwgHeaderEnvelope(view, version, maintenance)
        : DwgHeaderEnvelope{};
    t.expect(envelope.error == DwgHeaderEnvelopeError::None
                 && envelope.hasHighSizeWord == highSizePresent
                 && (!highSizePresent || envelope.highSizeWord == 0u)
                 && envelope.payloadEnd + 18u == bytes.size(),
             "versioned logical HEADER boundary/profile: " + label);

    HeaderSectionReader reader(bytes, 0u, version, maintenance);
    DRW_Header parsed;
    parsed.addInt("stale", 99, 70);
    t.expect(reader.readDwgHeader(parsed) && parsed.vars.count("LUNITS") == 1u
                 && reader.diagnostics().empty(),
             "versioned HEADER variables and CRC are accepted: " + label);

    auto trailing = bytes;
    trailing.push_back(0u);
    HeaderSectionReader trailingReader(trailing, 0u, version, maintenance);
    DRW_Header trailingHeader;
    trailingHeader.addInt("stale", 99, 70);
    t.expect(!trailingReader.readDwgHeader(trailingHeader)
                 && trailingHeader.vars.empty()
                 && trailingReader.diagnostics().size() == 1u
                 && trailingReader.diagnostics()[0].kind
                        == DwgIntegrityCheckKind::HeaderSectionBounds,
             "canonical HEADER profile cannot claim trailing bytes: " + label);

    auto crcMutation = bytes;
    const auto crcOffset = static_cast<std::size_t>(envelope.payloadEnd);
    crcMutation[crcOffset] ^= 1u;
    HeaderSectionReader crcReader(crcMutation, 0u, version, maintenance);
    DRW_Header crcHeader;
    t.expect(crcReader.readDwgHeader(crcHeader)
                 && crcReader.diagnostics().size() == 1u
                 && crcReader.diagnostics()[0].kind
                        == DwgIntegrityCheckKind::HeaderSectionCrc
                 && crcReader.diagnostics()[0].severity
                        == DwgIntegritySeverity::Warning,
             "versioned HEADER CRC mismatch remains warning-only: " + label);

    auto endMutation = bytes;
    endMutation[static_cast<std::size_t>(envelope.payloadEnd) + 2u] ^= 1u;
    HeaderSectionReader endReader(endMutation, 0u, version, maintenance);
    DRW_Header endHeader;
    endHeader.addInt("stale", 99, 70);
    t.expect(!endReader.readDwgHeader(endHeader) && endHeader.vars.empty()
                 && endReader.diagnostics().size() == 1u
                 && endReader.diagnostics()[0].kind
                        == DwgIntegrityCheckKind::HeaderSectionSentinel,
             "versioned HEADER end sentinel is checked before publication: " + label);
}

template<class Writer>
void checkHeaderDateTimeVersion(TestContext& t, DRW::Version version,
                                std::uint8_t maintenance,
                                const std::string& label) {
    constexpr std::array<const char*, 4> keys{{
        "TDCREATE", "TDUPDATE", "TDINDWG", "TDUSRTIMER"
    }};
    constexpr std::array<double, 4> values{{
        2451545.25, 2451545.5, 2.5, 0.125
    }};

    DRW_Header source;
    for (std::size_t index = 0; index < keys.size(); ++index)
        source.addDouble(keys[index], values[index], 40);

    Writer writer(nullptr, &source);
    if (!writer.writeDwgHeader()) {
        t.expect(false, "date/time HEADER writer succeeds: " + label);
        return;
    }
    std::vector<std::uint8_t> bytes = writer.buffer();
    HeaderSectionReader reader(bytes, 0u, version, maintenance);
    DRW_Header parsed;
    const bool read = reader.readDwgHeader(parsed);
    t.expect(read && reader.diagnostics().empty(),
             "date/time HEADER reads cleanly: " + label);
    if (!read)
        return;

    for (std::size_t index = 0; index < keys.size(); ++index) {
        std::int32_t day = 0;
        std::int32_t milliseconds = 0;
        const bool representable =
            dwgHeaderTime::split(values[index], day, milliseconds);
        const auto found = parsed.vars.find(keys[index]);
        const double expected = static_cast<double>(day)
            + static_cast<double>(milliseconds)
                / dwgHeaderTime::kMillisecondsPerDay;
        t.expect(representable && found != parsed.vars.end()
                     && found->second != nullptr
                     && found->second->type() == DRW_Variant::DOUBLE
                     && found->second->d_val() == expected,
                 "date/time value round-trips to exact millisecond pair: "
                     + label + " " + keys[index]);
    }
}

void testHeaderDateTimeEncoding(TestContext& t) {
    std::int32_t day = 77;
    std::int32_t milliseconds = 88;
    t.expect(dwgHeaderTime::split(0.0, day, milliseconds)
                 && day == 0 && milliseconds == 0,
             "zero date/time splits into the zero pair");
    t.expect(dwgHeaderTime::split(-0.25, day, milliseconds)
                 && day == -1 && milliseconds == 64800000,
             "negative representable date/time keeps floor-based day semantics");

    const double beforeMidnight = std::nextafter(
        2451546.0, -std::numeric_limits<double>::infinity());
    t.expect(dwgHeaderTime::split(beforeMidnight, day, milliseconds)
                 && day == 2451546 && milliseconds == 0,
             "nearest-millisecond rounding carries across midnight");

    const double minDay = static_cast<double>(
        (std::numeric_limits<std::int32_t>::min)());
    const double maxDay = static_cast<double>(
        (std::numeric_limits<std::int32_t>::max)());
    t.expect(dwgHeaderTime::split(minDay + 0.25, day, milliseconds)
                 && day == (std::numeric_limits<std::int32_t>::min)()
                 && milliseconds == 21600000,
             "minimum signed day is accepted when its pair is representable");
    t.expect(dwgHeaderTime::split(maxDay + 0.25, day, milliseconds)
                 && day == (std::numeric_limits<std::int32_t>::max)()
                 && milliseconds == 21600000,
             "maximum signed day is accepted when its pair is representable");

    const std::array<double, 7> invalidValues{{
        std::numeric_limits<double>::quiet_NaN(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        minDay - 0.25,
        maxDay + 1.0,
        std::numeric_limits<double>::max(),
        -std::numeric_limits<double>::max()
    }};
    for (double invalid : invalidValues) {
        day = 77;
        milliseconds = 88;
        const bool accepted = dwgHeaderTime::split(invalid, day, milliseconds);
        t.expect(!accepted && day == 77 && milliseconds == 88,
                 "invalid/unrepresentable time is refused without changing outputs");
    }

    constexpr std::array<const char*, 4> dateKeys{{
        "TDCREATE", "TDUPDATE", "TDINDWG", "TDUSRTIMER"
    }};
    for (const char* key : dateKeys) {
        for (double invalid : invalidValues) {
            HeaderWriteProbe source;
            source.addDouble(key, 0.25, 40);

            dwgBufferW data;
            dwgBufferW handles;
            dwgBufferW strings;
            data.putRawChar8(0xA5u);
            handles.putRawChar8(0x5Au);
            strings.putRawChar8(0xC3u);
            const bool primed = source.encodeDwg(
                DRW::AC1024, &data, &handles, &strings);
            t.expect(primed
                         && source.dwgHandseedBitOffset()
                                != DRW_Header::kInvalidDwgHandseedBitOffset,
                     "valid HEADER encode establishes a HANDSEED locator");
            const auto dataBefore = data.data();
            const auto handlesBefore = handles.data();
            const auto stringsBefore = strings.data();
            const auto handseedOffsetBefore = source.dwgHandseedBitOffset();
            source.addDouble(key, invalid, 40);
            const bool encoded = source.encodeDwg(
                DRW::AC1024, &data, &handles, &strings);
            t.expect(!encoded && data.data() == dataBefore
                         && handles.data() == handlesBefore
                         && strings.data() == stringsBefore
                         && source.dwgHandseedBitOffset() == handseedOffsetBefore,
                     "invalid " + std::string(key)
                         + " leaves encode streams and HANDSEED locator unchanged");
        }
    }

    checkHeaderDateTimeVersion<dwgWriter15>(t, DRW::AC1015, 0u, "AC1015");
    checkHeaderDateTimeVersion<dwgWriter18>(t, DRW::AC1018, 0u, "AC1018");
    checkHeaderDateTimeVersion<dwgWriter21>(t, DRW::AC1021, 0u, "AC1021");
    checkHeaderDateTimeVersion<dwgWriter24>(t, DRW::AC1024, 3u, "AC1024");
    checkHeaderDateTimeVersion<dwgWriter27>(t, DRW::AC1027, 6u, "AC1027");
    checkHeaderDateTimeVersion<dwgWriter32>(t, DRW::AC1032, 0u, "AC1032");
}

void testHeaderVersionEnvelopes(TestContext& t) {
    checkHeaderVersionProfile(t,
        makeHeaderSection<dwgWriter15>(t, "AC1015"), DRW::AC1015, 0u, false,
        "AC1015");
    checkHeaderVersionProfile(t,
        makeHeaderSection<dwgWriter18>(t, "AC1018"), DRW::AC1018, 0u, false,
        "AC1018");
    checkHeaderVersionProfile(t,
        makeHeaderSection<dwgWriter21>(t, "AC1021"), DRW::AC1021, 0u, false,
        "AC1021");

    auto ac1021 = makeHeaderSection<dwgWriter21>(t, "AC1021");
    checkHeaderVersionProfile(t, ac1021, DRW::AC1021, 6u, false,
                              "AC1021 app maintenance does not add a high-size word");

    auto ac1024Low = makeHeaderSection<dwgWriter24>(t, "AC1024 MV<=3");
    checkHeaderVersionProfile(t, ac1024Low, DRW::AC1024, 3u,
                                           false, "AC1024 MV<=3");
    auto ac1024High = ac1024Low;
    setHeaderHighSizeWord(ac1024High, true);
    checkHeaderVersionProfile(t, ac1024High, DRW::AC1024, 4u,
                                           true, "AC1024 MV>3");

    auto ac1024Excluded = ac1024High;
    excludeHeaderHighSizeWordFromCount(ac1024Excluded);
    dwgBuffer ac1024ExcludedView(ac1024Excluded.data(), ac1024Excluded.size());
    const bool ac1024ExcludedPositioned = ac1024ExcludedView.setPosition(16u);
    const auto ac1024ExcludedEnvelope = ac1024ExcludedPositioned
        ? inspectDwgHeaderEnvelope(ac1024ExcludedView, DRW::AC1024, 4u)
        : DwgHeaderEnvelope{};
    t.expect(ac1024ExcludedEnvelope.error == DwgHeaderEnvelopeError::None
                 && ac1024ExcludedEnvelope.excludesHighSizeWord
                 && ac1024ExcludedEnvelope.highSizeWord == 0u
                 && ac1024ExcludedEnvelope.unclaimedBytes == 0u
                 && ac1024ExcludedEnvelope.payloadEnd + 18u
                        == ac1024Excluded.size(),
             "AC1024 MV>3 admits the exact zero-word excluded-size profile");
    HeaderSectionReader ac1024ExcludedReader(
        ac1024Excluded, 0u, DRW::AC1024, 4u);
    DRW_Header ac1024ExcludedHeader;
    t.expect(ac1024ExcludedReader.readDwgHeader(ac1024ExcludedHeader)
                 && ac1024ExcludedHeader.vars.count("LUNITS") == 1u
                 && ac1024ExcludedReader.diagnostics().size() == 1u
                 && ac1024ExcludedReader.diagnostics()[0].kind
                        == DwgIntegrityCheckKind::HeaderSectionCompatibility,
             "AC1024 excluded-size profile parses fields and records compatibility");

    auto ac1024ExcludedSuffix = ac1024Excluded;
    ac1024ExcludedSuffix.push_back(0xA5u);
    HeaderSectionReader ac1024SuffixReader(
        ac1024ExcludedSuffix, 0u, DRW::AC1024, 4u);
    DRW_Header ac1024SuffixHeader;
    ac1024SuffixHeader.addInt("stale", 99, 70);
    t.expect(!ac1024SuffixReader.readDwgHeader(ac1024SuffixHeader)
                 && ac1024SuffixHeader.vars.empty()
                 && ac1024SuffixReader.diagnostics().size() == 1u
                 && ac1024SuffixReader.diagnostics()[0].kind
                        == DwgIntegrityCheckKind::HeaderSectionBounds,
             "AC1024 excluded-size profile rejects any logical suffix");

    auto ac1024ExcludedNonzeroWord = ac1024Excluded;
    ac1024ExcludedNonzeroWord[20u] = 1u;
    const auto ac1024NonzeroCrc = headerTestCrc(
        ac1024ExcludedNonzeroWord, 16u,
        static_cast<std::size_t>(ac1024ExcludedEnvelope.payloadEnd));
    ac1024ExcludedNonzeroWord[static_cast<std::size_t>(
        ac1024ExcludedEnvelope.payloadEnd)] = static_cast<std::uint8_t>(
            ac1024NonzeroCrc);
    ac1024ExcludedNonzeroWord[static_cast<std::size_t>(
        ac1024ExcludedEnvelope.payloadEnd) + 1u] = static_cast<std::uint8_t>(
            ac1024NonzeroCrc >> 8u);
    HeaderSectionReader ac1024NonzeroReader(
        ac1024ExcludedNonzeroWord, 0u, DRW::AC1024, 4u);
    DRW_Header ac1024NonzeroHeader;
    t.expect(!ac1024NonzeroReader.readDwgHeader(ac1024NonzeroHeader)
                 && ac1024NonzeroHeader.vars.empty(),
             "AC1024 excluded-size profile rejects nonzero high word");

    auto ac1024ExcludedBadCrc = ac1024Excluded;
    ac1024ExcludedBadCrc[static_cast<std::size_t>(
        ac1024ExcludedEnvelope.payloadEnd)] ^= 1u;
    HeaderSectionReader ac1024BadCrcReader(
        ac1024ExcludedBadCrc, 0u, DRW::AC1024, 4u);
    DRW_Header ac1024BadCrcHeader;
    t.expect(!ac1024BadCrcReader.readDwgHeader(ac1024BadCrcHeader)
                 && ac1024BadCrcHeader.vars.empty(),
             "AC1024 excluded-size profile never falls back around a bad CRC");

    HeaderSectionReader ac1024OldMaintenanceReader(
        ac1024Excluded, 0u, DRW::AC1024, 3u);
    DRW_Header ac1024OldMaintenanceHeader;
    t.expect(!ac1024OldMaintenanceReader.readDwgHeader(
                 ac1024OldMaintenanceHeader)
                 && ac1024OldMaintenanceHeader.vars.empty(),
             "AC1024 excluded-size profile is gated to app maintenance >3");

    auto ac1027Excluded = makeHeaderSection<dwgWriter27>(
        t, "AC1027 excluded-word negative");
    excludeHeaderHighSizeWordFromCount(ac1027Excluded);
    HeaderSectionReader ac1027ExcludedReader(
        ac1027Excluded, 0u, DRW::AC1027, 6u);
    DRW_Header ac1027ExcludedHeader;
    t.expect(!ac1027ExcludedReader.readDwgHeader(ac1027ExcludedHeader)
                 && ac1027ExcludedHeader.vars.empty(),
             "AC1027 excluded-size profile stays closed without a real witness");

    for (const auto& mismatch : {
             std::pair<const std::vector<std::uint8_t>*, std::uint8_t>{&ac1024High, 3u},
             {&ac1024Low, 4u}}) {
        auto mismatchBytes = *mismatch.first;
        HeaderSectionReader reader(mismatchBytes, 0u, DRW::AC1024, mismatch.second);
        DRW_Header header;
        header.addInt("stale", 99, 70);
        t.expect(!reader.readDwgHeader(header) && header.vars.empty(),
                 "AC1024 maintenance gate must match actual optional word presence");
    }

    auto ac1027High = makeHeaderSection<dwgWriter27>(t, "AC1027 MV>3");
    checkHeaderVersionProfile(t, ac1027High, DRW::AC1027, 6u,
                                           true, "AC1027 MV>3");
    auto ac1027WithUnclaimedSuffix = ac1027High;
    excludeHeaderHighSizeWordFromCount(ac1027WithUnclaimedSuffix);
    std::vector<std::uint8_t> ac1027Suffix(735u);
    for (std::size_t index = 0; index < ac1027Suffix.size(); ++index)
        ac1027Suffix[index] = static_cast<std::uint8_t>(index * 37u + 0x31u);
    ac1027WithUnclaimedSuffix.insert(ac1027WithUnclaimedSuffix.end(),
                                     ac1027Suffix.begin(),
                                     ac1027Suffix.end());
    const auto ac1027WithUnclaimedSuffixOriginal =
        ac1027WithUnclaimedSuffix;
    HeaderSectionReader ac1027SuffixReader(
        ac1027WithUnclaimedSuffix, 0u, DRW::AC1027, 6u);
    DRW_Header ac1027SuffixHeader;
    t.expect(ac1027SuffixReader.readDwgHeader(ac1027SuffixHeader)
                 && ac1027SuffixHeader.vars.count("LUNITS") == 1u
                 && ac1027SuffixHeader.vars.at("LUNITS")->content.i == 4
                 && ac1027SuffixReader.diagnostics().size() == 2u
                 && ac1027SuffixReader.diagnostics()[0].kind
                        == DwgIntegrityCheckKind::HeaderSectionCompatibility
                 && ac1027SuffixReader.diagnostics()[0].severity
                        == DwgIntegritySeverity::Warning
                 && ac1027SuffixReader.diagnostics()[1].kind
                        == DwgIntegrityCheckKind::HeaderSectionUnclaimedData
                 && ac1027SuffixReader.diagnostics()[1].severity
                        == DwgIntegritySeverity::Warning
                 && ac1027SuffixReader.diagnostics()[1].hasExpected
                 && ac1027SuffixReader.diagnostics()[1].expected
                        == ac1027High.size()
                 && ac1027SuffixReader.diagnostics()[1].hasObserved
                 && ac1027SuffixReader.diagnostics()[1].observed
                        == ac1027WithUnclaimedSuffix.size()
                 && ac1027WithUnclaimedSuffix
                        == ac1027WithUnclaimedSuffixOriginal,
             "AC1027 app-maintenance-4 admits parsed HEADER plus explicit suffix warning without mutating source bytes");
    auto ac1027Low = ac1027High;
    setHeaderHighSizeWord(ac1027Low, false);
    checkHeaderVersionProfile(t, ac1027Low, DRW::AC1027, 3u,
                                           false, "AC1027 MV<=3");
    auto ac1027LowWithSuffix = ac1027Low;
    ac1027LowWithSuffix.insert(ac1027LowWithSuffix.end(),
                               ac1027Suffix.begin(), ac1027Suffix.end());
    HeaderSectionReader ac1027LowSuffixReader(
        ac1027LowWithSuffix, 0u, DRW::AC1027, 3u);
    DRW_Header ac1027LowSuffixHeader;
    ac1027LowSuffixHeader.addInt("stale", 99, 70);
    t.expect(!ac1027LowSuffixReader.readDwgHeader(ac1027LowSuffixHeader)
                 && ac1027LowSuffixHeader.vars.empty(),
             "AC1027 app-maintenance<=3 does not inherit the suffix profile");
    for (const auto& mismatch : {
             std::pair<const std::vector<std::uint8_t>*, std::uint8_t>{&ac1027High, 3u},
             {&ac1027Low, 4u}}) {
        auto mismatchBytes = *mismatch.first;
        HeaderSectionReader reader(mismatchBytes, 0u, DRW::AC1027, mismatch.second);
        DRW_Header header;
        header.addInt("stale", 99, 70);
        t.expect(!reader.readDwgHeader(header) && header.vars.empty(),
                 "AC1027 maintenance gate must match actual optional word presence");
    }

    const auto ac1032 = makeHeaderSection<dwgWriter32>(t, "AC1032");
    checkHeaderVersionProfile(t, ac1032, DRW::AC1032, 0u,
                                           true, "AC1032 always");

    // R13/R14 are separate reader15 admission profiles. This envelope-only
    // partner checks their shared on-disk sentinels/RL/CRC layout without
    // treating AC1015 body semantics as R13/R14 qualification.
    auto ac1015 = makeHeaderSection<dwgWriter15>(t, "R13/R14 envelope basis");
    for (const DRW::Version version : {DRW::AC1012, DRW::AC1014}) {
        dwgBuffer view(ac1015.data(), ac1015.size());
        const bool positioned = view.setPosition(16u);
        const auto envelope = positioned
            ? inspectDwgHeaderEnvelope(view, version, 0u)
            : DwgHeaderEnvelope{};
        t.expect(envelope.error == DwgHeaderEnvelopeError::None
                     && !envelope.hasHighSizeWord
                     && envelope.payloadEnd + 18u == ac1015.size(),
                 "R13/R14 read-only HEADER section envelope: "
                     + std::string(version == DRW::AC1012 ? "AC1012" : "AC1014"));
        auto malformed = ac1015;
        malformed.front() ^= 1u;
        HeaderSectionReader reader(malformed, 0u, version, 0u);
        DRW_Header header;
        header.addInt("stale", 99, 70);
        t.expect(!reader.readDwgHeader(header) && header.vars.empty()
                     && reader.diagnostics().size() == 1u
                     && reader.diagnostics()[0].kind
                            == DwgIntegrityCheckKind::HeaderSectionSentinel,
                 "R13/R14 malformed HEADER sentinel rejects before parse: "
                     + std::string(version == DRW::AC1012 ? "AC1012" : "AC1014"));
    }
}

void patchHeaderTestLong(std::vector<std::uint8_t>& bytes, std::size_t offset,
                         std::uint32_t value) {
    for (std::size_t i = 0; i < 4u; ++i)
        bytes.at(offset + i) = static_cast<std::uint8_t>(value >> (8u * i));
}

void sealHeaderTest(std::vector<std::uint8_t>& bytes, std::size_t crcOffset) {
    const auto crc = headerTestCrc(bytes, 16u, crcOffset);
    bytes.at(crcOffset) = static_cast<std::uint8_t>(crc);
    bytes.at(crcOffset + 1u) = static_cast<std::uint8_t>(crc >> 8u);
}

void testHeaderCompatibilityProfile(TestContext& t) {
    DRW_Header source;
    source.addInt("LUNITS", 4, 70);
    source.addStr("MENU", "header-boundary-menu", 1);
    dwgWriter32 writer(nullptr, &source);
    t.expect(writer.writeDwgHeader(), "compatible profile source generation");
    const auto canonical = writer.buffer();
    const auto crcOffset = canonical.size() - 18u;
    auto compatible = canonical;
    patchHeaderTestLong(compatible, 16u,
                        static_cast<std::uint32_t>(crcOffset - 24u));
    sealHeaderTest(compatible, crcOffset);
    for (const std::size_t suffix : {0u, 1u, 739u}) {
        auto bytes = compatible;
        bytes.insert(bytes.end(), suffix, 0xA5u); // Authored opaque bytes, not zero padding.
        const auto original = bytes;
        dwgBuffer view(bytes.data(), bytes.size());
        (void)view.setPosition(16u);
        const auto envelope = inspectDwgHeaderEnvelope(view, DRW::AC1032, 55u);
        t.expect(envelope.error == DwgHeaderEnvelopeError::None
                     && envelope.excludesHighSizeWord
                     && envelope.payloadEnd == crcOffset
                     && envelope.trailerEnd == compatible.size()
                     && envelope.unclaimedBytes == suffix
                     && view.getPosition() == 16u && view.isGood()
                     && bytes == original,
                 "CRC-bound compatible envelope is nonmutating, suffix=" + std::to_string(suffix));
        HeaderSectionReader reader(bytes, 0u, DRW::AC1032, 55u);
        DRW_Header header;
        t.expect(reader.readDwgHeader(header) && header.vars.count("LUNITS") == 1u
                     && header.vars.at("LUNITS")->content.i == 4
                     && header.vars.count("MENU") == 1u
                     && header.vars.at("MENU")->type() == DRW_Variant::STRING
                     && header.vars.at("MENU")->c_str() != nullptr
                     && std::string(header.vars.at("MENU")->c_str()) == "header-boundary-menu"
                     && reader.diagnostics().size() == (suffix == 0u ? 1u : 2u),
                 "compatible HEADER retains fields and explicit observations");
        if (!reader.diagnostics().empty()) {
            const auto& layout = reader.diagnostics().front();
            t.expect(layout.kind == DwgIntegrityCheckKind::HeaderSectionCompatibility
                         && layout.severity == DwgIntegritySeverity::Warning
                         && layout.offsetSpace == DwgIntegrityAddressSpace::DecodedBuffer
                         && layout.hasFileOffset && layout.fileOffset == 16u
                         && layout.hasExpected && layout.expected == crcOffset - 4u
                         && layout.hasObserved && layout.observed == crcOffset,
                     "compatible layout diagnostic binds actual CRC offsets");
        }
        if (suffix != 0u && reader.diagnostics().size() == 2u) {
            const auto& tail = reader.diagnostics().back();
            t.expect(tail.kind == DwgIntegrityCheckKind::HeaderSectionUnclaimedData
                         && tail.severity == DwgIntegritySeverity::Warning
                         && tail.hasFileOffset && tail.fileOffset == compatible.size()
                         && tail.hasExpected && tail.expected == compatible.size()
                         && tail.hasObserved && tail.observed == bytes.size(),
                     "unclaimed logical suffix is observable, not declared padding");
        }
    }
    dwgBuffer fieldProbe(compatible.data(), compatible.size());
    (void)fieldProbe.setPosition(24u);
    const auto bits = fieldProbe.getRawLong32();
    const auto endBit = 192u + std::uint64_t(bits);
    std::uint64_t stringStart = 0, stringEnd = 0;
    const bool stringsLocated = fieldProbe.getR2007StringStreamBounds(
        endBit, stringStart, stringEnd);
    t.expect(stringsLocated && stringStart < stringEnd,
             "generated nonempty string stream has independent literal bounds");
    if (stringsLocated) {
        const auto patchBit = [](std::vector<std::uint8_t>& bytes,
                                  std::uint64_t offset, bool value) {
            const auto mask = static_cast<std::uint8_t>(1u << (7u - (offset & 7u)));
            auto& byte = bytes.at(static_cast<std::size_t>(offset >> 3u));
            byte = value ? static_cast<std::uint8_t>(byte | mask)
                         : static_cast<std::uint8_t>(byte & ~mask);
        };
        for (const std::string name : {"string-length", "string-absence", "footer-length"}) {
            auto bytes = compatible;
            bytes.insert(bytes.end(), 739u, 0xA5u);
            if (name == "string-length") {
                patchBit(bytes, stringStart, false);
                patchBit(bytes, stringStart + 1u, false); // BS00 + raw 0xffff.
                for (unsigned i = 0; i < 16u; ++i)
                    patchBit(bytes, stringStart + 2u + i, true);
            } else if (name == "string-absence") {
                patchBit(bytes, endBit - 1u, false);
            } else {
                for (unsigned i = 0; i < 16u; ++i)
                    patchBit(bytes, endBit - 17u + i, true);
            }
            sealHeaderTest(bytes, crcOffset);
            HeaderSectionReader reader(bytes, 0u, DRW::AC1032, 55u);
            DRW_Header header;
            t.expect(!reader.readDwgHeader(header) && header.vars.empty(),
                     "CRC-valid string mutation cannot borrow handle/suffix bytes: " + name);
        }
    }

    for (const std::string name : {"crc", "begin", "end", "word", "short",
                                   "max", "bits-zero", "bits-max"}) {
        auto bytes = compatible;
        bytes.insert(bytes.end(), 739u, 0xA5u);
        if (name == "crc") bytes[crcOffset] ^= 1u;
        if (name == "begin") bytes[0] ^= 1u;
        if (name == "end") bytes[crcOffset + 2u] ^= 1u;
        if (name == "word") patchHeaderTestLong(bytes, 20u, 1u);
        if (name == "short") patchHeaderTestLong(bytes, 16u, 1u);
        if (name == "max") patchHeaderTestLong(bytes, 16u, 0xFFFFFFFFu);
        if (name == "bits-zero") patchHeaderTestLong(bytes, 24u, 0u);
        if (name == "bits-max") patchHeaderTestLong(bytes, 24u, 0xFFFFFFFFu);
        if (name != "crc") sealHeaderTest(bytes, crcOffset);
        HeaderSectionReader reader(bytes, 0u, DRW::AC1032, 55u);
        DRW_Header header;
        header.addInt("stale", 1, 70);
        t.expect(!reader.readDwgHeader(header) && header.vars.empty()
                     && reader.diagnostics().size() == 1u
                     && reader.diagnostics().front().severity == DwgIntegritySeverity::Error,
                 "opaque suffix cannot rescue malformed compatible body: " + name);
    }
    for (const DRW::Version version : {DRW::AC1024, DRW::AC1027}) {
        auto bytes = compatible;
        HeaderSectionReader reader(bytes, 0u, version, 55u);
        DRW_Header header;
        t.expect(!reader.readDwgHeader(header) && header.vars.empty(),
                 "AC1032 compatibility is not extrapolated to unwitnessed versions");
    }
    auto fakeTail = canonical;
    fakeTail.insert(fakeTail.end(), 739u, 0xA5u);
    fakeTail.insert(fakeTail.end(), compatible.end() - 18, compatible.end());
    HeaderSectionReader fakeReader(fakeTail, 0u, DRW::AC1032, 55u);
    DRW_Header fakeHeader;
    t.expect(!fakeReader.readDwgHeader(fakeHeader) && fakeHeader.vars.empty(),
             "canonical surplus and distant matching sentinel do not trigger a scan");

    // A correct alternate CRC/sentinel with abundant suffix bytes still cannot
    // supply a missing variable body, string footer or handle stream.
    std::vector<std::uint8_t> shortBody(compatible.begin(), compatible.begin() + 32u);
    patchHeaderTestLong(shortBody, 16u, 8u);
    shortBody.resize(34u);
    sealHeaderTest(shortBody, 32u);
    shortBody.insert(shortBody.end(), std::begin(dwgSentinels::HEADER_END),
                     std::end(dwgSentinels::HEADER_END));
    shortBody.insert(shortBody.end(), 739u, 0xA5u);
    HeaderSectionReader shortReader(shortBody, 0u, DRW::AC1032, 55u);
    DRW_Header shortHeader;
    t.expect(!shortReader.readDwgHeader(shortHeader) && shortHeader.vars.empty(),
             "CRC-valid truncated alternate body cannot borrow unclaimed suffix");

    auto reuseBytes = compatible;
    reuseBytes.insert(reuseBytes.end(), 739u, 0xA5u);
    HeaderSectionReader reuse(reuseBytes, 0u, DRW::AC1032, 55u);
    DRW_Header reused;
    t.expect(reuse.readDwgHeader(reused), "compatible reader first valid operation");
    reuseBytes[crcOffset] ^= 1u;
    t.expect(!reuse.readDwgHeader(reused) && reused.vars.empty(),
             "compatible reader clears state on rejected replacement");
    reuseBytes[crcOffset] ^= 1u;
    t.expect(reuse.readDwgHeader(reused) && reused.vars.count("LUNITS") == 1u,
             "compatible reader accepts corrected retry");
    HeaderSectionReader saturated(reuseBytes, 0u, DRW::AC1032, 55u);
    saturated.fillDiagnostics();
    DRW_Header saturatedHeader;
    t.expect(saturated.readDwgHeader(saturatedHeader) && saturated.dropped() == 2u,
             "compatible observations preserve bounded dropped-diagnostic accounting");
}

class FullHeaderWriterProbe final : public dwgWriter15 {
public:
    FullHeaderWriterProbe(std::ofstream* stream, DRW_Header* header)
        : dwgWriter15(stream, header) {}
    std::size_t headerAddress() const { return m_sectionOffsets.at(0); }
    std::size_t classesAddress() const { return m_sectionOffsets.at(1); }
    struct SealingState {
        std::vector<std::uint8_t> bytes;
        std::uint32_t offset, size, patch, dataStart;
    };
    SealingState sealingState() const {
        return {buffer(), m_sectionOffsets.at(0), m_sectionSizes.at(0),
                m_headerHandseedBitOffset, m_headerDataStartBit};
    }
    void corruptSealing(const std::string& name) {
        const auto start = m_sectionOffsets.at(0);
        const auto crc = start + m_sectionSizes.at(0) - 18u;
        if (name == "missing-range") m_sectionOffsets.erase(0);
        if (name == "outside-range") m_sectionOffsets[0] = 0xFFFFFFFFu;
        if (name == "short-range") m_sectionSizes[0] = 37u;
        if (name == "long-range") ++m_sectionSizes[0];
        if (name == "declared-max") m_buf.patchRawLong32(start + 16u, 0xFFFFFFFFu);
        if (name == "declared-short") m_buf.patchRawLong32(start + 16u, 1u);
        if (name == "begin-sentinel") {
            const std::uint8_t value = std::uint8_t(buffer()[start] ^ 1u);
            (void)m_buf.patchRawBytesAtBit(start * 8u, &value, 1u);
        }
        if (name == "end-sentinel") {
            const std::uint8_t value = std::uint8_t(buffer()[crc + 2u] ^ 1u);
            (void)m_buf.patchRawBytesAtBit((crc + 2u) * 8u, &value, 1u);
        }
        if (name == "patch-prefix") m_headerHandseedBitOffset = m_headerDataStartBit - 1u;
        if (name == "patch-crc-bit") m_headerHandseedBitOffset = crc * 8u - 31u;
        if (name == "invalid-data-origin") m_headerDataStartBit = crc * 8u + 1u;
    }
    bool restoreSealing(const SealingState& state) {
        m_sectionOffsets[0] = state.offset;
        m_sectionSizes[0] = state.size;
        m_headerHandseedBitOffset = state.patch;
        m_headerDataStartBit = state.dataStart;
        return m_buf.patchRawBytesAtBit(0u, state.bytes.data(), state.bytes.size());
    }
};

class HeaderPublicationSink final : public dx_iface {
public:
    HeaderPublicationSink() {
        cData = &data;
        currentBlock = data.mBlock;
    }
    void addHeader(const DRW_Header* header) override {
        ++headers;
        handseed = header->getHandSeed();
    }
    void addLine(const DRW_Line& line) override {
        ++lines;
        lineEnd = line.secPoint;
    }
    std::size_t headers{0};
    std::size_t lines{0};
    std::uint32_t handseed{0};
    DRW_Coord lineEnd;
    dx_data data;
};

class Ac1027CommonEntityHandleProbe final : public DRW_Line {
public:
    void configure(std::uint32_t entityHandle, bool hasOwner,
                   std::int32_t reactors, bool hasColorBookHandle,
                   bool hasLinetypeHandle) {
        DRW_Entity::reset();
        handle = entityHandle;
        objSize = 0; // The generated handle stream starts at bit zero.
        ownerHandle = hasOwner;
        numReactors = reactors;
        xDictFlag = 0; // XDICTIONARY handle is present.
        ltFlags = hasLinetypeHandle ? 3u : 0u;
        setDwgAcDbColorHandle(hasColorBookHandle, 0u);
    }

    bool parseHandles(dwgBuffer& buffer, std::uint64_t handleEndBit) {
        return parseDwgEntHandle(DRW::AC1027, &buffer, true, handleEndBit);
    }
};

void testAc1027CommonEntityHandleOrder(TestContext& t) {
    struct GeneratedStream {
        std::vector<std::uint8_t> bytes;
        std::uint64_t fieldEndBit{0};
        std::uint64_t layerEndBit{0};
    };
    const auto makeStream = [](bool hasColorBookHandle,
                               bool hasLinetypeHandle) {
        GeneratedStream result;
        dwgBufferW writer;
        const auto appendHandle = [&writer](std::uint8_t code,
                                            std::uint32_t reference) {
            dwgHandle value;
            value.code = code;
            value.ref = reference;
            value.ref64 = reference;
            writer.putHandle(value);
        };

        // ODA v5.4.1 §20.4.2: owner, reactors, XDICTIONARY, optional
        // R2004+ color-book handle, layer, then optional linetype.
        appendHandle(DRW::DwgSoftPointer, 0x49u); // owner
        appendHandle(DRW::DwgSoftPointer, 0x47u); // reactor 1
        appendHandle(DRW::DwgSoftPointer, 0x48u); // reactor 2
        appendHandle(DRW::DwgHardOwnership, 0x45u); // XDICTIONARY
        if (hasColorBookHandle)
            appendHandle(DRW::DwgHardPointer, 0x46u); // ENC AcDbColor
        appendHandle(DRW::DwgSoftPointer, 0x42u); // layer
        result.layerEndBit = writer.bitCount();
        if (hasLinetypeHandle)
            appendHandle(DRW::DwgSoftPointer, 0x41u); // linetype
        result.fieldEndBit = writer.bitCount();
        appendHandle(DRW::DwgHardPointer, 0xCAFEu); // following-field sentinel
        result.bytes = writer.data();
        return result;
    };

    const auto valid = makeStream(true, true);
    Ac1027CommonEntityHandleProbe parsed;
    parsed.configure(0x40u, true, 2, true, true);
    dwgBuffer validBuffer(valid.bytes.data(), valid.bytes.size());
    const bool parsedValid = parsed.parseHandles(validBuffer, valid.fieldEndBit);
    const dwgHandle sentinel = validBuffer.getHandle();
    t.expect(parsedValid && parsed.parentHandle == 0x49u
                 && parsed.reactorHandles == std::vector<std::uint32_t>{0x47u, 0x48u}
                 && parsed.xDictHandle == 0x45u
                 && parsed.hasDwgAcDbColorHandle()
                 && parsed.dwgAcDbColorHandle() == 0x46u
                 && parsed.dwgLayerHandle() == 0x42u
                 && parsed.dwgLinetypeHandle() == 0x41u
                 && sentinel.code == DRW::DwgHardPointer
                 && sentinel.ref == 0xCAFEu
                 && validBuffer.numRemainingBytes() == 0,
             "AC1027 ENC/common handles follow ODA field order and preserve next-handle alignment");

    const auto noColor = makeStream(false, false);
    Ac1027CommonEntityHandleProbe parsedWithoutColor;
    parsedWithoutColor.configure(0x40u, true, 2, false, false);
    dwgBuffer noColorBuffer(noColor.bytes.data(), noColor.bytes.size());
    const bool parsedWithoutColorOk = parsedWithoutColor.parseHandles(
        noColorBuffer, noColor.fieldEndBit);
    const dwgHandle noColorSentinel = noColorBuffer.getHandle();
    t.expect(parsedWithoutColorOk
                 && !parsedWithoutColor.hasDwgAcDbColorHandle()
                 && parsedWithoutColor.xDictHandle == 0x45u
                 && parsedWithoutColor.dwgLayerHandle() == 0x42u
                 && noColorSentinel.code == DRW::DwgHardPointer
                 && noColorSentinel.ref == 0xCAFEu
                 && noColorBuffer.numRemainingBytes() == 0,
             "AC1027 absent ENC color reference does not shift common handles");

    Ac1027CommonEntityHandleProbe rejected;
    rejected.configure(0x40u, true, 2, true, true);
    rejected.parentHandle = 0xEEEEu;
    rejected.reactorHandles = {0xDDDDu};
    rejected.xDictHandle = 0xCCCCu;
    rejected.setDwgAcDbColorHandle(true, 0xBBBBu);
    rejected.setDwgLayerHandle(0xAAAAu);
    rejected.setDwgLinetypeHandle(0x9999u);
    dwgBuffer truncatedBuffer(valid.bytes.data(), valid.bytes.size());
    const bool parsedTruncated = rejected.parseHandles(
        truncatedBuffer, valid.layerEndBit);
    t.expect(!parsedTruncated && rejected.parentHandle == 0xEEEEu
                 && rejected.reactorHandles == std::vector<std::uint32_t>{0xDDDDu}
                 && rejected.xDictHandle == 0xCCCCu
                 && rejected.dwgAcDbColorHandle() == 0xBBBBu
                 && rejected.dwgLayerHandle() == 0xAAAAu
                 && rejected.dwgLinetypeHandle() == 0x9999u,
             "AC1027 truncated common handle tail fails without publishing partial roles");
}

void testTrackedHeaderRead(TestContext& t) {
    class Sink final : public dx_iface {
    public:
        Sink() {
            cData = &data;
            currentBlock = data.mBlock;
        }
        void addHeader(const DRW_Header* header) override {
            ++headers;
            dx_iface::addHeader(header);
            const std::array<const char*, 4> keys{{"LUNITS", "INSUNITS", "LUPREC", "DIMDEC"}};
            for (std::size_t i = 0; i < keys.size(); ++i) {
                const auto found = header->vars.find(keys[i]);
                if (found != header->vars.end() && found->second != nullptr
                    && found->second->type() == DRW_Variant::INTEGER)
                    values[i] = found->second->content.i;
            }
        }
        void addLine(const DRW_Line& line) override {
            if (line.handle == 0x32u) {
                ordinaryLineSeen = true;
                ordinaryLineReactors = line.reactorHandles;
                ordinaryLineXDictionary = line.xDictHandle;
                ordinaryLineHasColor = line.hasDwgAcDbColorHandle();
                ordinaryLineColor = line.dwgAcDbColorHandle();
                ordinaryLineLayer = line.dwgLayerHandle();
                ordinaryLineLinetype = line.dwgLinetypeHandle();
            }
            dx_iface::addLine(line);
        }
        void addText(const DRW_Text& text) override {
            rtext += dynamic_cast<const DRW_RText*>(&text) != nullptr;
            arcText += dynamic_cast<const DRW_ArcAlignedText*>(&text) != nullptr;
        }
        void addMPolygon(const DRW_MPolygon*) override { ++mpolygon; }
        void addNamedObjectDictionary(const DRW_Dictionary& dictionary) override {
            ++roots;
            dx_iface::addNamedObjectDictionary(dictionary);
        }
        std::array<int, 4> values{{-1, -1, -1, -1}};
        std::size_t headers{0};
        std::size_t roots{0};
        std::size_t rtext{0}, arcText{0}, mpolygon{0};
        bool ordinaryLineSeen{false};
        std::vector<std::uint32_t> ordinaryLineReactors;
        std::uint32_t ordinaryLineXDictionary{0};
        bool ordinaryLineHasColor{false};
        std::uint32_t ordinaryLineColor{0};
        std::uint32_t ordinaryLineLayer{0};
        std::uint32_t ordinaryLineLinetype{0};
        dx_data data;
    };
    struct Row {
        const char* name;
        std::size_t frames;
        std::array<int, 4> values;
        bool compatible;
    };
    const std::array<Row, 4> rows{{
        {"rtext_arctext.dwg", 267u, {{2, 4, 4, 2}}, true},
        {"mpolygon_solid.dwg", 95u, {{2, 0, 4, 4}}, true},
        {"large_radial.dwg", 110u, {{2, 0, 4, 4}}, true},
        {"ordinary_enc_AC1027.dwg", 26u, {{2, 0, 4, 4}}, false}
    }};
    for (const auto& row : rows) {
        const auto path = (std::filesystem::path(LIBDXFRW_READER_FIXTURE_DIR) / row.name).string();
        dwgRW reader(path.c_str());
        Sink sink;
        const bool read = reader.read(&sink, false);
        const auto observations = reader.getReadObservations();
        t.expect(read && reader.getError() == DRW::BAD_NONE && sink.headers == 1u
                     && sink.values == row.values,
                 "tracked HEADER exact public fields: " + std::string(row.name));
        t.expect(observations.frames.status == DRW_DwgFrameCoverageStatus::FinalizedComplete
                     && observations.frames.sourceFrames == row.frames,
                 "tracked physical frame accounting (not semantic qualification): " + std::string(row.name));
        const auto root = sink.data.headerC.dwgNamedObjectDictionaryReference();
        t.expect(root.has_value() && root->ref64 == (row.compatible ? 12u : 0u)
                     && sink.roots == (row.compatible ? 1u : 0u)
                     && sink.data.hasNamedObjectDictionary == row.compatible,
                 "DWG root identity follows the source HEADER, not handle/order guessing");
        if (std::string(row.name) == "ordinary_enc_AC1027.dwg") {
            t.expect(sink.ordinaryLineSeen
                         && sink.ordinaryLineReactors
                             == std::vector<std::uint32_t>{0x36u, 0x37u}
                         && sink.ordinaryLineXDictionary == 0x38u
                         && sink.ordinaryLineHasColor
                         && sink.ordinaryLineColor == 0x35u
                         && sink.ordinaryLineLayer == 0x12u
                         && sink.ordinaryLineLinetype == 0x10u,
                     "tracked AC1027 ENC handle slots map in ODA common-handle order");
        }
        if (row.compatible && root) {
            t.expect(root->code == DRW::DwgHardOwnership
                         && sink.data.namedObjectDictionary.handle == root->ref64
                         && sink.data.namedObjectDictionary.hasCompleteDwgEntries()
                         && std::none_of(sink.data.dictionaries.begin(), sink.data.dictionaries.end(),
                                [&root](const DRW_Dictionary& dictionary) {
                                    return dictionary.handle == root->ref64;
                                }),
                     "validated DWG NOD is published once on its explicit root path");
            DRW_Header copy(sink.data.headerC);
            DRW_Header moved(std::move(copy));
            DRW_Header assigned;
            assigned = moved;
            const auto movedRoot = moved.dwgNamedObjectDictionaryReference();
            const auto assignedRoot = assigned.dwgNamedObjectDictionaryReference();
            t.expect(!copy.dwgNamedObjectDictionaryReference()
                         && movedRoot && movedRoot->ref64 == root->ref64
                         && assignedRoot && assignedRoot->ref64 == root->ref64,
                     "DWG root-reference copies/moves own the captured value after HEADER delivery");
        }
        std::size_t layouts = 0, tails = 0;
        for (std::size_t i = 0; i < observations.integrity.retained; ++i) {
            const auto& diagnostic = observations.integrity.rows[i];
            layouts += diagnostic.kind == DwgIntegrityCheckKind::HeaderSectionCompatibility;
            tails += diagnostic.kind == DwgIntegrityCheckKind::HeaderSectionUnclaimedData;
        }
        t.expect(layouts == (row.compatible ? 1u : 0u)
                     && tails == (row.compatible ? 1u : 0u),
                 "tracked HEADER compatibility evidence survives facade snapshot");
        t.expect(sink.data.visualStyles.size() == (row.compatible ? 24u : 0u),
                 "tracked VISUALSTYLE source cardinality survives bounded decoding");
        for (const auto& style : sink.data.visualStyles) {
            const auto& metadata = style.m_body.m_encoding;
            t.expect(style.m_bodyDecoded && metadata.companionCount == 58u
                         && metadata.colorCount == 8u && metadata.hasSourceIsolines,
                     "tracked modern VISUALSTYLE publishes complete source metadata");
            if (style.desc == "Flat") {
                t.expect(metadata.colors[0].rawIndex == 0u
                             && metadata.colors[0].rawRgb == 0xC2FFFFFFu
                             && metadata.colors[2].rawRgb == 0xC8000000u
                             && metadata.colors[5].rawRgb == 0xC2000000u
                             && metadata.companions[51] == 0u
                             && metadata.companions[52] == 0u
                             && style.m_body.faceMonoColor == 256
                             && style.m_body.edgeObscuredColor == 256,
                         "tracked Flat style retains RGB white/black/C8 and source operation zeros");
            }
        }
        if (std::string(row.name) == "rtext_arctext.dwg")
            t.expect(sink.rtext == 1u && sink.arcText == 1u,
                     "tracked RTEXT/ARCTEXT concrete callbacks survive HEADER admission");
        if (std::string(row.name) == "mpolygon_solid.dwg")
            t.expect(sink.mpolygon == 1u, "tracked MPOLYGON callback survives HEADER admission");
    }
}

template<class Writer>
void testHeaderWriterSealing(TestContext& t, const std::string& version) {
    for (const std::uint32_t suppliedSeed : {0u, 0x12345678u}) {
        std::stringbuf output(std::ios::out | std::ios::binary);
        std::ofstream stream;
        static_cast<std::ostream&>(stream).rdbuf(&output);
        DRW_Header header;
        header.setHandSeed(suppliedSeed);
        Writer writer(&stream, &header);
        writer.reserveHandle(0x7000u);
        const auto label = version + " seed=" + std::to_string(suppliedSeed);
        const bool prepared = writer.prepareDwgClassManifest()
            && writer.writeFileHeaderStub();
        t.expect(prepared, "HEADER writer preparation: " + label);
        if (!prepared) continue;
        const auto start = writer.buffer().size();
        if (!writer.writeDwgHeader()) {
            t.expect(false, "HEADER writer emission: " + label);
            continue;
        }
        const auto sectionSize = writer.buffer().size() - start;
        const auto crcOffset = start + sectionSize - 18u;
        const auto storedCrc = [&writer, crcOffset]() {
            return std::uint16_t(writer.buffer()[crcOffset])
                | (std::uint16_t(writer.buffer()[crcOffset + 1u]) << 8u);
        };
        t.expect(storedCrc() == headerTestCrc(writer.buffer(), start + 16u, crcOffset),
                 "initial HEADER CRC matches independent bitwise projection: " + label);
        DRW_Line line;
        line.basePoint = DRW_Coord(1, 2, 3);
        line.secPoint = DRW_Coord(4, 5, 6);
        const bool objects = writer.writeDwgClasses() && writer.writeDwgObjects()
            && writer.encodeEntity(&line) && writer.emitDeferredBlockControl();
        t.expect(objects, "HEADER writer following-record preparation: " + label);
        if (!objects) continue;
        // An unemitted reservation must also contribute to the high-water
        // seed; maxEmittedHandle+1 alone is not the complete existing policy.
        writer.reserveHandle(0x9000u);
        std::uint32_t maxHandle = 0;
        t.expect(writer.validateEmittedHandleMap(maxHandle), "HEADER writer handle map: " + label);
        const auto before = writer.buffer();
        t.expect(writer.finalizeHeaderHandseed(), "final HANDSEED patch: " + label);
        const auto finalSeed = header.getHandSeed();
        t.expect(finalSeed > maxHandle && finalSeed >= writer.highWaterHandle()
                     && (suppliedSeed == 0u || finalSeed == suppliedSeed),
                 "HANDSEED retains supplied/high-water bounds: " + label);
        t.expect(storedCrc() == headerTestCrc(writer.buffer(), start + 16u, crcOffset),
                 "final HEADER CRC matches independent bitwise projection: " + label);
        const std::size_t dataPrefix = version == "AC1015" || version == "AC1018"
            ? 20u : version == "AC1027" || version == "AC1032" ? 28u : 24u;
        const auto patchBit = (start + dataPrefix) * 8u + header.dwgHandseedBitOffset();
        bool otherBitsExact = before.size() == writer.buffer().size();
        for (std::size_t byte = 0; otherBitsExact && byte < before.size(); ++byte) {
            const auto changed = std::uint8_t(before[byte] ^ writer.buffer()[byte]);
            for (unsigned int bit = 0; bit < 8u; ++bit) {
                const auto at = byte * 8u + bit;
                if (at >= patchBit && at < patchBit + 32u) continue;
                if (byte >= crcOffset && byte < crcOffset + 2u) continue;
                if ((changed & (1u << (7u - bit))) != 0u) otherBitsExact = false;
            }
        }
        t.expect(otherBitsExact,
                 "only HANDSEED bits/CRC change, including unaligned neighbors: " + label);
        if (suppliedSeed != 0u)
            t.expect(writer.buffer() == before, "unchanged high HANDSEED leaves all bytes exact: " + label);
        const auto once = writer.buffer();
        t.expect(writer.finalizeHeaderHandseed() && writer.buffer() == once,
                 "HANDSEED patch/reseal is idempotent, not container finalization: " + label);
        t.expect(output.str().empty(), "HANDSEED finalization publishes no file prefix: " + label);
        const bool published = writer.writeDwgHandles() && writer.writeSecondHeader()
            && writer.finalize();
        t.expect(published, "sealed HEADER container publication: " + label);
        if (!published) continue;
        const auto file = output.str();
        std::vector<std::uint8_t> bytes(file.begin(), file.end());
        HeaderPublicationSink sink;
        dwgRW reader("");
        t.expect(reader.readBuffer(bytes.data(), bytes.size(), &sink, false)
                     && sink.headers == 1u && sink.handseed == finalSeed
                     && sink.lines == 1u && sink.lineEnd.x == 4
                     && sink.lineEnd.y == 5 && sink.lineEnd.z == 6,
                 "sealed complete file retains HEADER and following LINE: " + label);
        const auto diagnostics = reader.getIntegrityDiagnostics();
        t.expect(std::none_of(diagnostics.begin(), diagnostics.end(), [](const auto& entry) {
                     return entry.kind == DwgIntegrityCheckKind::HeaderSectionCrc;
                 }), "clean writer needs no HEADER CRC warning: " + label);
    }
}

void testHeaderWriters(TestContext& t) {
    testHeaderWriterSealing<dwgWriter15>(t, "AC1015");
    testHeaderWriterSealing<dwgWriter18>(t, "AC1018");
    testHeaderWriterSealing<dwgWriter21>(t, "AC1021");
    testHeaderWriterSealing<dwgWriter24>(t, "AC1024");
    testHeaderWriterSealing<dwgWriter27>(t, "AC1027");
    testHeaderWriterSealing<dwgWriter32>(t, "AC1032");
}

void testHeaderWriterSealingFailures(TestContext& t) {
    std::stringbuf output(std::ios::out | std::ios::binary);
    std::ofstream stream;
    static_cast<std::ostream&>(stream).rdbuf(&output);
    DRW_Header header;
    FullHeaderWriterProbe writer(&stream, &header);
    const bool prepared = writer.prepareDwgClassManifest() && writer.writeFileHeaderStub()
        && writer.writeDwgHeader() && writer.writeDwgClasses()
        && writer.writeDwgObjects() && writer.emitDeferredBlockControl();
    t.expect(prepared, "writer sealing negative controls have valid preceding records");
    if (!prepared) return;
    const auto valid = writer.sealingState();
    t.expect(valid.patch % 8u != 0u, "actual HEADER HANDSEED patch is unaligned");
    for (const std::string name : {"missing-range", "outside-range", "short-range",
            "long-range", "declared-max", "declared-short", "begin-sentinel",
            "end-sentinel", "patch-prefix", "patch-crc-bit", "invalid-data-origin"}) {
        writer.corruptSealing(name);
        const auto corrupted = writer.buffer();
        const auto priorSeed = header.getHandSeed();
        t.expect(!writer.finalizeHeaderHandseed() && writer.buffer() == corrupted
                     && header.getHandSeed() == priorSeed && output.str().empty(),
                 "invalid HEADER sealing rejects before any mutation/publication: " + name);
        t.expect(writer.restoreSealing(valid) && writer.finalizeHeaderHandseed(),
                 "corrected internal HEADER range can be retried: " + name);
        const auto crc = valid.offset + valid.size - 18u;
        const auto stored = std::uint16_t(writer.buffer()[crc])
            | (std::uint16_t(writer.buffer()[crc + 1u]) << 8u);
        t.expect(stored == headerTestCrc(writer.buffer(), valid.offset + 16u, crc),
                 "successful retry reseals the independently checked HEADER: " + name);
    }
}

void testHeaderPublicRead(TestContext& t) {
    // The legacy writer takes an ofstream; its ostream buffer can be owned
    // memory, so complete-file controls need no fixture file or filesystem.
    std::stringbuf output(std::ios::out | std::ios::binary);
    std::ofstream stream;
    static_cast<std::ostream&>(stream).rdbuf(&output);
    DRW_Header source;
    FullHeaderWriterProbe writer(&stream, &source);
    auto phase = [&t](bool result, const std::string& name) {
        t.expect(result, "complete AC1015 memory writer phase: " + name);
        return result;
    };
    const bool generated = phase(writer.prepareDwgClassManifest(), "class manifest")
        && phase(writer.writeFileHeaderStub(), "file header")
        && phase(writer.writeDwgHeader(), "HEADER")
        && phase(writer.writeDwgClasses(), "CLASSES")
        && phase(writer.writeDwgObjects(), "objects")
        && phase(writer.emitDeferredBlockControl(), "block control")
        && phase(writer.finalizeHeaderHandseed(), "header HANDSEED")
        && phase(writer.writeDwgHandles(), "handles")
        && phase(writer.writeSecondHeader(), "second header")
        && phase(writer.finalize(), "finalize");
    t.expect(generated, "complete from-scratch AC1015 source writes to an owned memory stream");
    if (!generated) return;
    const auto data = output.str();
    std::vector<std::uint8_t> original(data.begin(), data.end());
    const auto headerStart = writer.headerAddress();
    const auto classesStart = writer.classesAddress();
    const auto count = std::uint32_t(original[headerStart + 16u])
        | (std::uint32_t(original[headerStart + 17u]) << 8u)
        | (std::uint32_t(original[headerStart + 18u]) << 16u)
        | (std::uint32_t(original[headerStart + 19u]) << 24u);
    const auto crc = headerStart + 20u + count;
    const auto writerStoredCrc = std::uint16_t(original[crc])
        | (std::uint16_t(original[crc + 1u]) << 8u);
    const auto correctedCrc = headerTestCrc(original, headerStart + 16u, crc);
    HeaderPublicationSink unmodifiedSink;
    dwgRW unmodifiedReader("");
    const bool unmodifiedAccepted = unmodifiedReader.readBuffer(
        original.data(), original.size(), &unmodifiedSink, false);
    const auto unmodifiedDiagnostics = unmodifiedReader.getIntegrityDiagnostics();
    const auto unmodifiedWarnings = std::count_if(
        unmodifiedDiagnostics.begin(), unmodifiedDiagnostics.end(), [](const auto& entry) {
            return entry.kind == DwgIntegrityCheckKind::HeaderSectionCrc;
        });
    t.expect(unmodifiedAccepted && unmodifiedSink.headers == 1u
                 && unmodifiedWarnings == (writerStoredCrc == correctedCrc ? 0 : 1),
             "unmodified writer output retains checksum-warning compatibility");
    // Keep the reader mutation controls independent of writer sealing. This
    // test-only normalization cannot qualify production writer integrity;
    // testHeaderWriters checks the unmodified production CRC independently.
    original[crc] = static_cast<std::uint8_t>(correctedCrc);
    original[crc + 1u] = static_cast<std::uint8_t>(correctedCrc >> 8u);
    dwgRW reader("");
    for (const std::string name : {"valid", "crc", "end-and-classes", "valid-again"}) {
        auto bytes = original;
        if (name == "crc") bytes[crc] ^= 1u;
        if (name == "end-and-classes") {
            bytes[crc + 2u] ^= 1u;
            std::fill(bytes.begin() + classesStart + 16u,
                      bytes.begin() + classesStart + 20u, 0xFFu);
        }
        HeaderPublicationSink sink;
        const bool accepted = reader.readBuffer(bytes.data(), bytes.size(), &sink, false);
        const bool structural = name == "end-and-classes";
        t.expect(accepted == !structural && sink.headers == (structural ? 0u : 1u),
                 "public HEADER success/publication boundary: " + name
                     + " (error=" + std::to_string(reader.getError())
                     + ", callbacks=" + std::to_string(sink.headers) + ")");
        t.expect(reader.getError() == (structural ? DRW::BAD_READ_HEADER : DRW::BAD_NONE),
                 "public first HEADER error remains sticky and resets on reuse: " + name);
        const auto diagnostics = reader.getIntegrityDiagnostics();
        const auto warnings = std::count_if(diagnostics.begin(), diagnostics.end(), [](const auto& entry) {
            return entry.kind == DwgIntegrityCheckKind::HeaderSectionCrc;
        });
        t.expect(warnings == (name == "crc" ? 1 : 0),
                 "public HEADER diagnostics reset between read operations: " + name);
    }
}

void testVersionDispatch(TestContext& t) {
    struct ReaderCase {
        const char* magic;
        DRW::Version version;
    };
    const std::vector<ReaderCase> cases {
        {"AC1.40", DRW::AC14},
        {"AC2.10", DRW::AC210},
        {"AC1003", DRW::AC1003},
        {"AC1004", DRW::AC1004},
        {"AC1006", DRW::AC1006},
        {"AC1009", DRW::AC1009},
        {"AC1012", DRW::AC1012},
        {"AC1014", DRW::AC1014},
        {"AC1015", DRW::AC1015},
        {"AC1018", DRW::AC1018},
        {"AC1021", DRW::AC1021},
        {"AC1024", DRW::AC1024},
        {"AC1027", DRW::AC1027},
        {"AC1032", DRW::AC1032},
    };

    dwgRW owner("");
    dx_iface interface_;
    dx_data data;
    interface_.cData = &data;
    interface_.currentBlock = data.mBlock;
    for (const ReaderCase& test : cases) {
        std::array<std::uint8_t, 6> bytes {};
        std::memcpy(bytes.data(), test.magic, bytes.size());
        const bool opened = owner.readBuffer(bytes.data(), bytes.size(),
                                             &interface_, false);
        const std::string label = std::string("DWG dispatch ") + test.magic;
        t.expect(owner.getVersion() == test.version,
                 label + " sniffs expected version before body rejection");
        t.expect(!opened, label + " rejects synthetic body after dispatch");
    }

    std::array<std::uint8_t, 6> unsupported {
        {'B', 'A', 'D', '0', '0', '0'}
    };
    t.expect(!owner.readBuffer(unsupported.data(), unsupported.size(),
                               &interface_, false)
                 && owner.getError() == DRW::BAD_VERSION,
             "DWG dispatch rejects unknown AC magic with BAD_VERSION");

    std::array<std::uint8_t, 5> truncated {};
    t.expect(!owner.readBuffer(truncated.data(), truncated.size(),
                               &interface_, false)
                 && owner.getError() == DRW::BAD_UNKNOWN,
             "DWG dispatch rejects truncated version header");
}

void testR140RepeatFailsClosed(TestContext& t) {
    class PointSink final : public dx_iface {
    public:
        PointSink() {
            cData = &data;
            currentBlock = data.mBlock;
        }
        void addPoint(const DRW_Point& value) override {
            points.push_back(value);
        }

        dx_data data;
        std::vector<DRW_Point> points;
    } sink;

    const auto makeDrawing = [](bool includeRepeat) {
        constexpr std::size_t entityStart = 0x202u;
        std::vector<std::uint8_t> bytes(entityStart, 0u);
        std::memcpy(bytes.data(), "AC1.40", 6u);
        const auto appendU16 = [&bytes](std::uint16_t value) {
            bytes.push_back(static_cast<std::uint8_t>(value));
            bytes.push_back(static_cast<std::uint8_t>(value >> 8u));
        };
        const auto appendU32At = [&bytes](std::size_t offset,
                                          std::uint32_t value) {
            for (unsigned byte = 0; byte < 4u; ++byte)
                bytes[offset + byte] =
                    static_cast<std::uint8_t>(value >> (byte * 8u));
        };
        const auto appendDouble = [&bytes](double value) {
            static_assert(sizeof(double) == sizeof(std::uint64_t),
                          "R1.40 test vectors require 64-bit doubles");
            std::uint64_t bits = 0;
            std::memcpy(&bits, &value, sizeof(bits));
            for (unsigned byte = 0; byte < 8u; ++byte)
                bytes.push_back(
                    static_cast<std::uint8_t>(bits >> (byte * 8u)));
        };
        const auto appendPoint = [&](double x, double y) {
            appendU16(2u); // T_POINT
            appendU16(0u); // palette layer index
            appendDouble(x);
            appendDouble(y);
        };

        appendPoint(1.0, 2.0);
        if (includeRepeat) {
            appendU16(5u); // T_REPEAT, empty structural marker
            appendU16(0u);
            appendPoint(3.0, 4.0); // seed member; must not complete silently
            appendU16(6u); // T_ENDREP
            appendU16(0u);
            appendU16(3u); // columns
            appendU16(2u); // rows
            appendDouble(7.0);
            appendDouble(11.0);
            appendPoint(13.0, 17.0); // following-record sentinel
        } else {
            appendPoint(13.0, 17.0);
        }
        appendU32At(0x24u, static_cast<std::uint32_t>(bytes.size()));
        return bytes;
    };

    const std::vector<std::uint8_t> repeated = makeDrawing(true);
    dwgRW reader("");
    const bool repeatAccepted = reader.readBuffer(
        repeated.data(), repeated.size(), &sink, false);
    const DRW_OperationDiagnostic diagnostic = reader.getLastDiagnostic();
    t.expect(!repeatAccepted && reader.getError() == DRW::BAD_READ_ENTITIES,
             "AC1.40 REPEAT refuses a seed-only successful read");
    t.expect(diagnostic.failed()
                 && diagnostic.phase == DRW::OperationPhase::Entities
                 && diagnostic.cause == DRW::OperationCause::ValidationFailure
                 && diagnostic.code == "unsupported-legacy-repeat"
                 && diagnostic.hasOffset && diagnostic.offset == 0x216u,
             "AC1.40 REPEAT reports a bounded semantic-loss diagnostic");
    t.expect(sink.points.size() == 1u
                 && sink.points.front().basePoint.x == 1.0
                 && sink.points.front().basePoint.y == 2.0,
             "AC1.40 REPEAT does not publish its seed as a complete array");

    const auto inputPath = std::filesystem::temp_directory_path()
        / ("libdxfrw-ac1_40-repeat-" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count())
           + ".dwg");
    {
        std::ofstream output(inputPath, std::ios::binary);
        output.write(reinterpret_cast<const char*>(repeated.data()),
                     static_cast<std::streamsize>(repeated.size()));
        t.expect(output.good(), "generated AC1.40 repeat control writes to scratch");
    }
    dx_iface importer;
    dx_data imported;
    std::ostringstream readDiagnostics;
    bool importAccepted = true;
    {
        class CerrCapture {
        public:
            explicit CerrCapture(std::ostream& stream)
                : previous(std::cerr.rdbuf(stream.rdbuf())) {}
            ~CerrCapture() { std::cerr.rdbuf(previous); }
        private:
            std::streambuf* previous;
        } capture(readDiagnostics);
        importAccepted = importer.fileImport(inputPath.u8string(), &imported,
                                             false);
    }
    std::error_code removeError;
    std::filesystem::remove(inputPath, removeError);
    t.expect(!importAccepted
                 && imported.dwgReadOutcome.error == DRW::BAD_READ_ENTITIES
                 && readDiagnostics.str().find("DWG conversion incomplete: ")
                        != std::string::npos
                 && readDiagnostics.str().find("unsupported-legacy-repeat")
                        != std::string::npos,
             "adapter exposes the human-readable incomplete-conversion reason");

    sink.points.clear();
    const std::vector<std::uint8_t> ordinary = makeDrawing(false);
    const bool ordinaryAccepted = reader.readBuffer(
        ordinary.data(), ordinary.size(), &sink, false);
    t.expect(ordinaryAccepted && reader.getError() == DRW::BAD_NONE
                 && sink.points.size() == 2u
                 && sink.points[0].basePoint.x == 1.0
                 && sink.points[1].basePoint.x == 13.0
                 && !reader.getLastDiagnostic().failed(),
             "AC1.40 non-repeat conversion succeeds after the fail-closed case");
}

void testR11UnsupportedMarkersFailClosed(TestContext& t) {
    class PointSink final : public dx_iface {
    public:
        PointSink() {
            cData = &data;
            currentBlock = data.mBlock;
        }
        void addPoint(const DRW_Point& value) override {
            points.push_back(value);
        }
        void addBlock(const DRW_Block& value) override {
            blocks.push_back(value);
            dx_iface::addBlock(value);
        }

        dx_data data;
        std::vector<DRW_Point> points;
        std::vector<DRW_Block> blocks;
    };

    const auto makeDrawing = [](const char *magic, std::uint16_t headerVars,
                                std::uint8_t markerType) {
        constexpr std::size_t entityStart = 0x600u;
        std::vector<std::uint8_t> bytes(entityStart, 0u);
        std::memcpy(bytes.data(), magic, 6u);
        const auto writeU16At = [&bytes](std::size_t offset,
                                         std::uint16_t value) {
            bytes[offset] = static_cast<std::uint8_t>(value);
            bytes[offset + 1u] = static_cast<std::uint8_t>(value >> 8u);
        };
        const auto writeU32At = [&bytes](std::size_t offset,
                                         std::uint32_t value) {
            for (unsigned byte = 0; byte < 4u; ++byte)
                bytes[offset + byte] = static_cast<std::uint8_t>(
                    value >> (byte * 8u));
        };
        const auto appendU16 = [&bytes](std::uint16_t value) {
            bytes.push_back(static_cast<std::uint8_t>(value));
            bytes.push_back(static_cast<std::uint8_t>(value >> 8u));
        };
        const auto appendDouble = [&bytes](double value) {
            static_assert(sizeof(double) == sizeof(std::uint64_t),
                          "pre-R13 test vectors require 64-bit doubles");
            std::uint64_t bits = 0;
            std::memcpy(&bits, &value, sizeof(bits));
            for (unsigned byte = 0; byte < 8u; ++byte)
                bytes.push_back(static_cast<std::uint8_t>(
                    bits >> (byte * 8u)));
        };
        const auto appendMarker = [&](std::uint8_t type) {
            bytes.push_back(type);
            bytes.push_back(0u); // flags
            appendU16(8u);       // common entity header only
            appendU16(0u);       // layer index
            appendU16(0u);       // optional-field flags
        };
        const auto appendBlockSentinel = [&]() {
            bytes.push_back(12u); // R11 T_BLOCK
            bytes.push_back(0u);  // flags
            appendU16(24u);       // header + common fields + base point
            appendU16(0u);        // layer index
            appendU16(0u);        // no optional block strings
            appendDouble(4.0);    // block base point (2RD)
            appendDouble(9.0);
            bytes.push_back(13u); // R11 T_ENDBLK
            bytes.push_back(0u);
            appendU16(8u);
            appendU16(0u);
            appendU16(0u);
        };

        writeU16At(0x11u, headerVars);
        writeU32At(0x14u, static_cast<std::uint32_t>(entityStart));
        if (markerType != 0u)
            appendMarker(markerType);
        appendBlockSentinel(); // must not be reached after a refused marker
        writeU32At(0x18u, static_cast<std::uint32_t>(bytes.size()));
        return bytes;
    };

    const std::array<std::uint8_t, 3> markerTypes {{5u, 6u, 10u}};
    const std::array<const char *, 3> markerCodes {{
        "unsupported-r11-repeat", "unsupported-r11-endrep",
        "unsupported-r11-load"}};
    const std::array<const char *, 2> magics {{"AC1006", "AC1009"}};
    const std::array<std::uint16_t, 2> headerVars {{158u, 205u}};

    for (std::size_t version = 0; version < magics.size(); ++version) {
        for (std::size_t marker = 0; marker < markerTypes.size(); ++marker) {
            PointSink sink;
            dwgRW reader("");
            const std::vector<std::uint8_t> marked = makeDrawing(
                magics[version], headerVars[version], markerTypes[marker]);
            const bool accepted = reader.readBuffer(
                marked.data(), marked.size(), &sink, false);
            const DRW_OperationDiagnostic diagnostic =
                reader.getLastDiagnostic();
            t.expect(!accepted && reader.getError() == DRW::BAD_READ_ENTITIES,
                     std::string(magics[version]) + " marker "
                         + std::to_string(markerTypes[marker])
                         + " refuses a potentially lossy conversion");
            t.expect(diagnostic.failed()
                         && diagnostic.phase == DRW::OperationPhase::Entities
                         && diagnostic.cause
                                == DRW::OperationCause::ValidationFailure
                         && diagnostic.code == markerCodes[marker]
                         && diagnostic.hasOffset
                         && diagnostic.offset == 0x600u,
                     std::string(magics[version]) + " marker diagnostic identifies "
                         + markerCodes[marker] + " and its record offset");
            t.expect(sink.points.empty() && sink.blocks.empty(),
                     std::string(magics[version]) + " refuses before publishing "
                         "the following block sentinel");

            sink.points.clear();
            const std::vector<std::uint8_t> ordinary = makeDrawing(
                magics[version], headerVars[version], 0u);
            const bool retryAccepted = reader.readBuffer(
                ordinary.data(), ordinary.size(), &sink, false);
            t.expect(retryAccepted && reader.getError() == DRW::BAD_NONE
                         && sink.points.empty() && sink.blocks.size() == 1u
                         && sink.blocks.front().basePoint.x == 4.0
                         && sink.blocks.front().basePoint.y == 9.0
                         && !reader.getLastDiagnostic().failed(),
                     std::string(magics[version]) + " marker-free sentinel retry succeeds "
                         "on the same reader");
        }
    }
}

void testR11PaperSpaceProjection(TestContext& t,
                                 const std::string& generatedInputPath = {}) {
    class PointSink final : public dx_iface {
    public:
        PointSink() {
            cData = &data;
            currentBlock = data.mBlock;
        }
        void addPoint(const DRW_Point& value) override {
            points.push_back(value);
            dx_iface::addPoint(value);
        }

        dx_data data;
        std::vector<DRW_Point> points;
    };

    const auto makeDrawing = []() {
        constexpr std::size_t entityStart = 0x600u;
        std::vector<std::uint8_t> bytes(entityStart, 0u);
        std::memcpy(bytes.data(), "AC1009", 6u);
        const auto writeU16At = [&bytes](std::size_t offset,
                                         std::uint16_t value) {
            bytes[offset] = static_cast<std::uint8_t>(value);
            bytes[offset + 1u] = static_cast<std::uint8_t>(value >> 8u);
        };
        const auto writeU32At = [&bytes](std::size_t offset,
                                         std::uint32_t value) {
            for (unsigned byte = 0; byte < 4u; ++byte)
                bytes[offset + byte] = static_cast<std::uint8_t>(
                    value >> (byte * 8u));
        };
        const auto appendU16 = [&bytes](std::uint16_t value) {
            bytes.push_back(static_cast<std::uint8_t>(value));
            bytes.push_back(static_cast<std::uint8_t>(value >> 8u));
        };
        const auto appendDouble = [&bytes](double value) {
            static_assert(sizeof(double) == sizeof(std::uint64_t),
                          "pre-R13 test vectors require 64-bit doubles");
            std::uint64_t bits = 0;
            std::memcpy(&bits, &value, sizeof(bits));
            for (unsigned byte = 0; byte < 8u; ++byte)
                bytes.push_back(static_cast<std::uint8_t>(
                    bits >> (byte * 8u)));
        };
        const auto appendPoint = [&](bool paperSpace, double x) {
            const std::uint8_t flags = paperSpace ? 0x40u : 0u;
            const std::uint16_t size = static_cast<std::uint16_t>(
                4u + 2u + 2u + (paperSpace ? 1u : 0u) + 3u * 8u);
            bytes.push_back(2u); // R11 T_POINT
            bytes.push_back(flags);
            appendU16(size);
            appendU16(0u); // layer index
            appendU16(0u); // optional-field flags
            if (paperSpace)
                bytes.push_back(0u); // extra_r11: no EED or viewport
            appendDouble(x);
            appendDouble(x + 10.0);
            appendDouble(x + 20.0);
        };

        writeU16At(0x11u, 205u); // AC1009 header variable size
        writeU32At(0x14u, static_cast<std::uint32_t>(entityStart));
        appendPoint(true, 1.0);
        appendPoint(false, 2.0);
        appendPoint(false, 3.0); // alignment sentinel after extended record
        writeU32At(0x18u, static_cast<std::uint32_t>(bytes.size()));
        return bytes;
    };

    const std::vector<std::uint8_t> drawing = makeDrawing();
    if (!generatedInputPath.empty()) {
        std::ofstream output(generatedInputPath, std::ios::binary);
        if (output.good()) {
            output.write(reinterpret_cast<const char*>(drawing.data()),
                         static_cast<std::streamsize>(drawing.size()));
        }
        t.expect(output.good(),
                 "generated AC1009 paper-space DWG control writes to scratch");
    }
    PointSink sink;
    dwgRW reader("");
    const bool accepted = reader.readBuffer(
        drawing.data(), drawing.size(), &sink, false);
    t.expect(accepted && reader.getError() == DRW::BAD_NONE,
             "generated AC1009 paper-space control reads successfully");
    t.expect(sink.points.size() == 3u,
             "AC1009 extended paper-space header preserves following points");
    if (sink.points.size() == 3u) {
        t.expect(sink.points[0].basePoint.x == 1.0
                     && sink.points[0].basePoint.y == 11.0
                     && sink.points[0].basePoint.z == 21.0
                     && sink.points[0].space == DRW::PaperSpace,
                 "AC1009 HAS_PSPACE projects the point as paper space");
        t.expect(sink.points[1].basePoint.x == 2.0
                     && sink.points[1].space == DRW::ModelSpace,
                 "AC1009 point without HAS_PSPACE remains model space");
        t.expect(sink.points[2].basePoint.x == 3.0
                     && sink.points[2].basePoint.y == 13.0
                     && sink.points[2].basePoint.z == 23.0
                     && sink.points[2].space == DRW::ModelSpace,
                 "AC1009 point after extension-byte record remains aligned");
    }
    t.expect(sink.data.mBlock != nullptr
                 && sink.data.mBlock->ent.size() == 3u
                 && sink.data.mBlock->ent.front()->space == DRW::PaperSpace,
             "2D adapter retains the projected space classification");

    const auto outputPath = std::filesystem::temp_directory_path()
        / ("libdxfrw-r11-paper-space-" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count())
           + ".dxf");
    const bool exported = sink.fileExport(outputPath.string(), DRW::AC1009,
                                           false, &sink.data, false);
    t.expect(exported,
             "generated AC1009 paper-space control exports through the 2D adapter");
    if (exported) {
        std::ifstream input(outputPath, std::ios::binary);
        const bool opened = input.good();
        std::size_t pointRecords = 0u;
        std::size_t paperSpaceMarkers = 0u;
        std::string previousLine;
        std::string line;
        while (std::getline(input, line)) {
            const std::size_t first = line.find_first_not_of(" \t\r");
            if (first == std::string::npos) {
                line.clear();
            } else {
                line = line.substr(first,
                    line.find_last_not_of(" \t\r") - first + 1u);
            }
            if (previousLine == "0" && line == "POINT")
                ++pointRecords;
            if (previousLine == "67" && line == "1")
                ++paperSpaceMarkers;
            previousLine = line;
        }
        t.expect(opened && pointRecords == 3u,
                 "AC1009 DXF output retains all generated point records");
        t.expect(opened && paperSpaceMarkers == 1u,
                 "AC1009 DXF output emits group 67 for the paper-space point");

        PointSink roundTrip;
        const std::string outputName = outputPath.string();
        dxfRW outputReader(outputName.c_str());
        const bool reopened = outputReader.read(&roundTrip, false);
        t.expect(reopened && roundTrip.points.size() == 3u
                     && roundTrip.points[0].basePoint.x == 1.0
                     && roundTrip.points[0].space == DRW::PaperSpace
                     && roundTrip.points[1].basePoint.x == 2.0
                     && roundTrip.points[1].space == DRW::ModelSpace
                     && roundTrip.points[2].basePoint.x == 3.0
                     && roundTrip.points[2].space == DRW::ModelSpace,
                 "AC1009 DXF reopen preserves exact model/paper point membership");
    }
    std::error_code removeError;
    std::filesystem::remove(outputPath, removeError);
}

void testR140HeaderBounds(TestContext& t) {
    class PointSink final : public dx_iface {
    public:
        PointSink() {
            cData = &data;
            currentBlock = data.mBlock;
        }
        void addPoint(const DRW_Point& value) override {
            points.push_back(value);
        }

        dx_data data;
        std::vector<DRW_Point> points;
    } sink;

    constexpr std::size_t entityStart = 0x202u;
    const auto makeHeader = [](std::size_t physicalSize,
                               std::uint32_t declaredEnd) {
        std::vector<std::uint8_t> bytes(physicalSize, 0u);
        if (bytes.size() >= 6u)
            std::memcpy(bytes.data(), "AC1.40", 6u);
        if (bytes.size() >= 0x28u) {
            for (unsigned byte = 0; byte < 4u; ++byte)
                bytes[0x24u + byte] = static_cast<std::uint8_t>(
                    declaredEnd >> (byte * 8u));
        }
        return bytes;
    };
    const auto writeU16 = [](std::vector<std::uint8_t>& bytes,
                             std::size_t offset, std::uint16_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value);
        bytes[offset + 1u] = static_cast<std::uint8_t>(value >> 8u);
    };
    const auto writeDouble = [](std::vector<std::uint8_t>& bytes,
                                std::size_t offset, double value) {
        static_assert(sizeof(double) == sizeof(std::uint64_t),
                      "R1.40 test vectors require 64-bit doubles");
        std::uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        for (unsigned byte = 0; byte < 8u; ++byte)
            bytes[offset + byte] = static_cast<std::uint8_t>(
                bits >> (byte * 8u));
    };

    dwgRW reader("");
    const auto expectHeaderFailure = [&](const std::vector<std::uint8_t>& bytes,
                                         const std::string& label) {
        sink.points.clear();
        const bool accepted = reader.readBuffer(
            bytes.data(), bytes.size(), &sink, false);
        t.expect(!accepted && reader.getError() == DRW::BAD_READ_FILE_HEADER,
                 label + " rejects at the file-header stage");
        t.expect(sink.points.empty(), label + " invokes no entity callbacks");
    };

    const std::vector<std::uint8_t> belowStart = makeHeader(entityStart,
        static_cast<std::uint32_t>(entityStart - 1u));
    expectHeaderFailure(belowStart, "AC1.40 declared end below entity start");

    const std::vector<std::uint8_t> beyondPhysical = makeHeader(entityStart,
        static_cast<std::uint32_t>(entityStart + 1u));
    expectHeaderFailure(beyondPhysical,
                        "AC1.40 declared end beyond physical input");

    // Same short-file boundary class as the published upstream lead, but
    // generated independently: no external bytes or payload are reused.
    const std::vector<std::uint8_t> shortPhysical = makeHeader(
        496u, static_cast<std::uint32_t>(entityStart + 1u));
    expectHeaderFailure(shortPhysical,
                        "AC1.40 physical input shorter than entity start");

    std::vector<std::uint8_t> truncatedRecordHeader = makeHeader(
        entityStart + 1u, static_cast<std::uint32_t>(entityStart + 1u));
    truncatedRecordHeader[entityStart] = 2u; // partial T_POINT RS
    sink.points.clear();
    t.expect(!reader.readBuffer(truncatedRecordHeader.data(),
                                truncatedRecordHeader.size(), &sink, false)
                 && reader.getError() == DRW::BAD_READ_ENTITIES
                 && sink.points.empty(),
             "AC1.40 partial entity header fails within its declared span");

    std::vector<std::uint8_t> truncatedPointBody = makeHeader(
        entityStart + 4u, static_cast<std::uint32_t>(entityStart + 4u));
    writeU16(truncatedPointBody, entityStart, 2u); // T_POINT
    writeU16(truncatedPointBody, entityStart + 2u, 0u); // layer
    sink.points.clear();
    t.expect(!reader.readBuffer(truncatedPointBody.data(),
                                truncatedPointBody.size(), &sink, false)
                 && reader.getError() == DRW::BAD_READ_ENTITIES
                 && sink.points.empty(),
             "AC1.40 truncated point body is rejected before callback");

    std::vector<std::uint8_t> exactEmpty = makeHeader(
        entityStart, static_cast<std::uint32_t>(entityStart));
    sink.points.clear();
    t.expect(reader.readBuffer(exactEmpty.data(), exactEmpty.size(), &sink, false)
                 && reader.getError() == DRW::BAD_NONE && sink.points.empty(),
             "AC1.40 exact lower-bound empty stream succeeds after failures");

    constexpr std::size_t pointRecordSize = 20u;
    const std::size_t twoPointEnd = entityStart + 2u * pointRecordSize;
    std::vector<std::uint8_t> twoPoints = makeHeader(
        twoPointEnd, static_cast<std::uint32_t>(twoPointEnd));
    const auto writePoint = [&](std::size_t offset, double x, double y) {
        writeU16(twoPoints, offset, 2u); // T_POINT
        writeU16(twoPoints, offset + 2u, 0u); // layer
        writeDouble(twoPoints, offset + 4u, x);
        writeDouble(twoPoints, offset + 12u, y);
    };
    writePoint(entityStart, 1.0, 2.0);
    writePoint(entityStart + pointRecordSize, 13.0, 17.0);
    sink.points.clear();
    t.expect(reader.readBuffer(twoPoints.data(), twoPoints.size(), &sink, false)
                 && reader.getError() == DRW::BAD_NONE
                 && sink.points.size() == 2u
                 && sink.points[0].basePoint.x == 1.0
                 && sink.points[0].basePoint.y == 2.0
                 && sink.points[1].basePoint.x == 13.0
                 && sink.points[1].basePoint.y == 17.0,
             "AC1.40 exact-end records advance and preserve following sentinel");
}

void testR140LoadFailsClosed(TestContext& t) {
    class PointSink final : public dx_iface {
    public:
        PointSink() {
            cData = &data;
            currentBlock = data.mBlock;
        }
        void addPoint(const DRW_Point& value) override {
            points.push_back(value);
        }

        dx_data data;
        std::vector<DRW_Point> points;
    } sink;

    constexpr std::size_t entityStart = 0x202u;
    const auto makeDrawing = [=](bool includeLoad) {
        std::vector<std::uint8_t> bytes(entityStart, 0u);
        std::memcpy(bytes.data(), "AC1.40", 6u);
        const auto appendU16 = [&bytes](std::uint16_t value) {
            bytes.push_back(static_cast<std::uint8_t>(value));
            bytes.push_back(static_cast<std::uint8_t>(value >> 8u));
        };
        const auto appendU32At = [&bytes](std::size_t offset,
                                          std::uint32_t value) {
            for (unsigned byte = 0; byte < 4u; ++byte)
                bytes[offset + byte] = static_cast<std::uint8_t>(
                    value >> (byte * 8u));
        };
        const auto appendDouble = [&bytes](double value) {
            static_assert(sizeof(double) == sizeof(std::uint64_t),
                          "R1.40 test vectors require 64-bit doubles");
            std::uint64_t bits = 0;
            std::memcpy(&bits, &value, sizeof(bits));
            for (unsigned byte = 0; byte < 8u; ++byte)
                bytes.push_back(static_cast<std::uint8_t>(
                    bits >> (byte * 8u)));
        };
        const auto appendPoint = [&](double x, double y) {
            appendU16(2u); // T_POINT
            appendU16(0u); // palette layer index
            appendDouble(x);
            appendDouble(y);
        };

        if (includeLoad) {
            appendU16(10u); // T_LOAD; conversion stops at this marker
            appendU16(0u);  // palette layer index
            appendU16(4u);  // bounded source-authored TV payload
            bytes.insert(bytes.end(), {'S', 'H', 'X', '1'});
            appendPoint(5.0, 7.0); // must not be reported after LOAD
        } else {
            appendPoint(5.0, 7.0);
        }
        appendU32At(0x24u, static_cast<std::uint32_t>(bytes.size()));
        return bytes;
    };

    const std::vector<std::uint8_t> withLoad = makeDrawing(true);
    dwgRW reader("");
    const bool accepted = reader.readBuffer(
        withLoad.data(), withLoad.size(), &sink, false);
    const DRW_OperationDiagnostic diagnostic = reader.getLastDiagnostic();
    t.expect(!accepted && reader.getError() == DRW::BAD_READ_ENTITIES,
             "AC1.40 LOAD does not report an unresolved resource as success");
    t.expect(diagnostic.failed()
                 && diagnostic.phase == DRW::OperationPhase::Entities
                 && diagnostic.cause == DRW::OperationCause::ValidationFailure
                 && diagnostic.code == "unsupported-legacy-load"
                 && diagnostic.hasOffset && diagnostic.offset == entityStart,
             "AC1.40 LOAD reports its source offset and unsupported reason");
    t.expect(sink.points.empty(),
             "AC1.40 LOAD refuses before publishing following geometry");

    const std::vector<std::uint8_t> ordinary = makeDrawing(false);
    const bool retryAccepted = reader.readBuffer(
        ordinary.data(), ordinary.size(), &sink, false);
    t.expect(retryAccepted && reader.getError() == DRW::BAD_NONE
                 && sink.points.size() == 1u
                 && sink.points.front().basePoint.x == 5.0
                 && sink.points.front().basePoint.y == 7.0
                 && !reader.getLastDiagnostic().failed(),
             "AC1.40 ordinary stream succeeds after LOAD refusal");
}

void testR140ShapeFailsClosed(TestContext& t) {
    class ShapeSink final : public dx_iface {
    public:
        ShapeSink() {
            cData = &data;
            currentBlock = data.mBlock;
        }
        void addPoint(const DRW_Point& value) override {
            points.push_back(value);
        }
        void addShape(const DRW_Shape& value) override {
            shapes.push_back(value);
        }

        dx_data data;
        std::vector<DRW_Point> points;
        std::vector<DRW_Shape> shapes;
    } sink;

    constexpr std::size_t entityStart = 0x202u;
    std::vector<std::uint8_t> withShape(entityStart, 0u);
    std::memcpy(withShape.data(), "AC1.40", 6u);
    const auto appendU16 = [&withShape](std::uint16_t value) {
        withShape.push_back(static_cast<std::uint8_t>(value));
        withShape.push_back(static_cast<std::uint8_t>(value >> 8u));
    };
    const auto appendDouble = [&withShape](double value) {
        static_assert(sizeof(double) == sizeof(std::uint64_t),
                      "R1.40 test vectors require 64-bit doubles");
        std::uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        for (unsigned byte = 0; byte < 8u; ++byte)
            withShape.push_back(
                static_cast<std::uint8_t>(bits >> (byte * 8u)));
    };
    const auto appendPoint = [&appendU16, &appendDouble](double x, double y) {
        appendU16(2u); // T_POINT
        appendU16(0u); // palette layer index
        appendDouble(x);
        appendDouble(y);
    };

    appendU16(4u); // T_SHAPE
    appendU16(0u); // palette layer index
    appendDouble(1.0); // insertion point x
    appendDouble(2.0); // insertion point y
    appendDouble(3.0); // scale
    appendDouble(0.25); // rotation
    appendU16(7u); // glyph index; no external identity is available here
    appendPoint(13.0, 17.0); // following geometry must not be reported
    const std::uint32_t declaredEnd =
        static_cast<std::uint32_t>(withShape.size());
    for (unsigned byte = 0; byte < 4u; ++byte)
        withShape[0x24u + byte] = static_cast<std::uint8_t>(
            declaredEnd >> (byte * 8u));

    dwgRW reader("");
    const bool accepted = reader.readBuffer(
        withShape.data(), withShape.size(), &sink, false);
    const DRW_OperationDiagnostic diagnostic = reader.getLastDiagnostic();
    t.expect(!accepted && reader.getError() == DRW::BAD_READ_ENTITIES,
             "AC1.40 SHAPE does not publish an unresolved glyph index");
    t.expect(diagnostic.failed()
                 && diagnostic.phase == DRW::OperationPhase::Entities
                 && diagnostic.cause == DRW::OperationCause::ValidationFailure
                 && diagnostic.code == "unsupported-legacy-shape"
                 && diagnostic.hasOffset && diagnostic.offset == entityStart,
             "AC1.40 SHAPE reports its source offset and unresolved identity");
    t.expect(sink.shapes.empty() && sink.points.empty(),
             "AC1.40 SHAPE refuses before publishing shape or later geometry");

    std::vector<std::uint8_t> ordinary(entityStart, 0u);
    std::memcpy(ordinary.data(), "AC1.40", 6u);
    const auto appendOrdinaryU16 = [&ordinary](std::uint16_t value) {
        ordinary.push_back(static_cast<std::uint8_t>(value));
        ordinary.push_back(static_cast<std::uint8_t>(value >> 8u));
    };
    const auto appendOrdinaryDouble = [&ordinary](double value) {
        std::uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        for (unsigned byte = 0; byte < 8u; ++byte)
            ordinary.push_back(
                static_cast<std::uint8_t>(bits >> (byte * 8u)));
    };
    appendOrdinaryU16(2u); // T_POINT
    appendOrdinaryU16(0u); // palette layer index
    appendOrdinaryDouble(5.0);
    appendOrdinaryDouble(7.0);
    const auto ordinaryEnd = static_cast<std::uint32_t>(ordinary.size());
    for (unsigned byte = 0; byte < 4u; ++byte)
        ordinary[0x24u + byte] = static_cast<std::uint8_t>(
            ordinaryEnd >> (byte * 8u));
    sink.points.clear();
    const bool retryAccepted = reader.readBuffer(
        ordinary.data(), ordinary.size(), &sink, false);
    t.expect(retryAccepted && reader.getError() == DRW::BAD_NONE
                 && sink.shapes.empty() && sink.points.size() == 1u
                 && sink.points.front().basePoint.x == 5.0
                 && sink.points.front().basePoint.y == 7.0
                 && !reader.getLastDiagnostic().failed(),
             "AC1.40 ordinary stream succeeds after SHAPE refusal");
}

void testR140EndrepFailsClosed(TestContext& t) {
    class PointSink final : public dx_iface {
    public:
        PointSink() {
            cData = &data;
            currentBlock = data.mBlock;
        }
        void addPoint(const DRW_Point& value) override {
            points.push_back(value);
        }

        dx_data data;
        std::vector<DRW_Point> points;
    } sink;

    constexpr std::size_t entityStart = 0x202u;
    std::vector<std::uint8_t> withEndrep(entityStart, 0u);
    std::memcpy(withEndrep.data(), "AC1.40", 6u);
    const auto appendU16 = [&withEndrep](std::uint16_t value) {
        withEndrep.push_back(static_cast<std::uint8_t>(value));
        withEndrep.push_back(static_cast<std::uint8_t>(value >> 8u));
    };
    const auto appendDouble = [&withEndrep](double value) {
        std::uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        for (unsigned byte = 0; byte < 8u; ++byte)
            withEndrep.push_back(
                static_cast<std::uint8_t>(bits >> (byte * 8u)));
    };

    appendU16(6u); // T_ENDREP without a preceding T_REPEAT
    appendU16(0u); // palette layer index
    appendU16(3u); // unresolved column count
    appendU16(2u); // unresolved row count
    appendDouble(7.0); // unresolved column spacing
    appendDouble(11.0); // unresolved row spacing
    appendU16(2u); // following T_POINT sentinel
    appendU16(0u); // palette layer index
    appendDouble(13.0);
    appendDouble(17.0);
    const auto declaredEnd = static_cast<std::uint32_t>(withEndrep.size());
    for (unsigned byte = 0; byte < 4u; ++byte)
        withEndrep[0x24u + byte] = static_cast<std::uint8_t>(
            declaredEnd >> (byte * 8u));

    dwgRW reader("");
    const bool accepted = reader.readBuffer(
        withEndrep.data(), withEndrep.size(), &sink, false);
    const DRW_OperationDiagnostic diagnostic = reader.getLastDiagnostic();
    t.expect(!accepted && reader.getError() == DRW::BAD_READ_ENTITIES,
             "AC1.40 ENDREP does not silently discard unresolved array metadata");
    t.expect(diagnostic.failed()
                 && diagnostic.phase == DRW::OperationPhase::Entities
                 && diagnostic.cause == DRW::OperationCause::ValidationFailure
                 && diagnostic.code == "unsupported-legacy-endrep"
                 && diagnostic.hasOffset && diagnostic.offset == entityStart,
             "AC1.40 ENDREP reports its source offset and unsupported semantics");
    t.expect(sink.points.empty(),
             "AC1.40 ENDREP refusal prevents following sentinel geometry");

    std::vector<std::uint8_t> ordinary(entityStart, 0u);
    std::memcpy(ordinary.data(), "AC1.40", 6u);
    const auto appendOrdinaryU16 = [&ordinary](std::uint16_t value) {
        ordinary.push_back(static_cast<std::uint8_t>(value));
        ordinary.push_back(static_cast<std::uint8_t>(value >> 8u));
    };
    const auto appendOrdinaryDouble = [&ordinary](double value) {
        std::uint64_t bits = 0;
        std::memcpy(&bits, &value, sizeof(bits));
        for (unsigned byte = 0; byte < 8u; ++byte)
            ordinary.push_back(
                static_cast<std::uint8_t>(bits >> (byte * 8u)));
    };
    appendOrdinaryU16(2u); // T_POINT
    appendOrdinaryU16(0u); // palette layer index
    appendOrdinaryDouble(5.0);
    appendOrdinaryDouble(7.0);
    const auto ordinaryEnd = static_cast<std::uint32_t>(ordinary.size());
    for (unsigned byte = 0; byte < 4u; ++byte)
        ordinary[0x24u + byte] = static_cast<std::uint8_t>(
            ordinaryEnd >> (byte * 8u));
    const bool retryAccepted = reader.readBuffer(
        ordinary.data(), ordinary.size(), &sink, false);
    t.expect(retryAccepted && reader.getError() == DRW::BAD_NONE
                 && sink.points.size() == 1u
                 && sink.points.front().basePoint.x == 5.0
                 && sink.points.front().basePoint.y == 7.0
                 && !reader.getLastDiagnostic().failed(),
             "AC1.40 ordinary stream succeeds after ENDREP refusal");
}

void testReadBufferRejection(TestContext& t) {
    dwgRW owner("");
    std::array<std::uint8_t, 6> unknown {
        {'B', 'A', 'D', '0', '0', '0'}
    };
    t.expect(!owner.readBuffer(unknown.data(), unknown.size(), nullptr, false)
                 && owner.getError() == DRW::BAD_UNKNOWN,
             "public readBuffer rejects null interface before format parsing");

    std::array<std::uint8_t, 5> shortHeader {};
    t.expect(!owner.readBuffer(shortHeader.data(), shortHeader.size(), nullptr, false)
                 && owner.getError() == DRW::BAD_UNKNOWN,
             "public readBuffer rejects null interface for short input");
    t.expect(!owner.readBuffer(nullptr, 6, nullptr, false)
                 && owner.getError() == DRW::BAD_UNKNOWN,
             "public readBuffer rejects null input without crashing");
}

void testSectionNameMatrix(TestContext& t) {
    struct SectionCase {
        const char* name;
        secEnum::DWGSection section;
    };
    const std::vector<SectionCase> cases {
        {"AcDb:Header", secEnum::HEADER},
        {"AcDb:Classes", secEnum::CLASSES},
        {"AcDb:Handles", secEnum::HANDLES},
        {"AcDb:AcDbObjects", secEnum::OBJECTS},
        {"AcDb:Preview", secEnum::PREVIEW},
        {"AcDb:SummaryInfo", secEnum::SUMARYINFO},
        {"AcDb:RevHistory", secEnum::REVHISTORY},
        {"AcDb:AppInfo", secEnum::APPINFO},
        {"AcDb:AppInfoHistory", secEnum::APPINFOHISTORY},
        {"AcDb:ObjFreeSpace", secEnum::OBJFREESPACE},
        {"AcDb:Template", secEnum::TEMPLATE},
        {"AcDb:FileDepList", secEnum::FILEDEP},
        {"AcDb:Security", secEnum::SECURITY},
        {"AcDb:AuxHeader", secEnum::AUXHEADER},
        {"AcDb:Signature", secEnum::SIGNATURE},
        {"AcDb:VBAProject", secEnum::VBAPROY},
        {"AcDb:AcDsPrototype_1b", secEnum::PROTOTYPE},
    };
    for (const SectionCase& test : cases) {
        t.expect(secEnum::getEnum(test.name) == test.section,
                 std::string("section matrix maps ") + test.name);
    }
    t.expect(secEnum::getEnum("AcDb:Unknown") == secEnum::UNKNOWNS,
             "section matrix keeps unknown names explicit");
    t.expect(secEnum::getEnum("") == secEnum::UNKNOWNS,
             "section matrix keeps empty names unsupported");
}

void testSectionNameUtf16Framing(TestContext& t) {
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1021, false);

    // SectionNameLength is a byte count.  A one-code-unit name must be
    // accepted, and the following byte must remain untouched for the next
    // section-map field.
    std::vector<std::uint8_t> oneUnit {'A', 0, 0x7f};
    dwgBuffer oneUnitBuffer(oneUnit.data(), oneUnit.size(), &codec);
    t.expect(oneUnitBuffer.getUCSStr(2) == "A",
             "section-name framing accepts one UTF-16 code unit");
    t.expect(oneUnitBuffer.getPosition() == 2
                 && oneUnitBuffer.getRawChar8() == 0x7f,
             "section-name framing preserves the following byte");

    // Some section-map producers include a declared UTF-16 NUL code unit in
    // the byte count.  Normalize that declared unit without consuming the
    // next field.
    std::vector<std::uint8_t> declaredNull {'A', 0, 0, 0, 0x6b};
    dwgBuffer declaredNullBuffer(declaredNull.data(), declaredNull.size(),
                                  &codec);
    t.expect(declaredNullBuffer.getUCSStr(4) == "A",
             "section-name framing strips a declared UTF-16 NUL");
    t.expect(declaredNullBuffer.getPosition() == 4
                 && declaredNullBuffer.getRawChar8() == 0x6b,
             "declared UTF-16 NUL normalization preserves alignment");

    std::vector<std::uint8_t> malformed {'A', 0, 0x7f};
    dwgBuffer malformedBuffer(malformed.data(), malformed.size(), &codec);
    t.expect(malformedBuffer.getUCSStr(3).empty()
                 && malformedBuffer.getPosition() == 0,
             "section-name framing rejects an odd byte length");
}

void testR2007ClassStringFooter(TestContext& t) {
    // The AC1024 RTM class footer uses the high-bit extension when the UTF-16
    // string stream exceeds 0x7fff bits. Keep this vector local and synthetic:
    // it exercises the wire arithmetic without retaining a drawing fixture.
    constexpr std::uint64_t footerEndBit = 40800;
    constexpr std::uint64_t highSize = 1;
    constexpr std::uint64_t lowSize = 0x8001;
    constexpr std::uint64_t expectedSize = (highSize << 15) | 1;
    constexpr std::uint64_t expectedStart = footerEndBit - 32 - expectedSize;
    std::vector<std::uint8_t> bytes(5200, 0);
    bytes[footerEndBit / 8 - 4] = static_cast<std::uint8_t>(highSize);
    bytes[footerEndBit / 8 - 3] = 0;
    bytes[footerEndBit / 8 - 2] = static_cast<std::uint8_t>(lowSize & 0xff);
    bytes[footerEndBit / 8 - 1] = static_cast<std::uint8_t>(lowSize >> 8);
    dwgBuffer buffer(bytes.data(), bytes.size());
    std::uint64_t start = 0;
    std::uint64_t size = 0;
    t.expect(readDwgClassStringFooter(buffer, footerEndBit, start, size),
             "AC1024 class footer accepts high-bit string-size extension");
    t.expect(start == expectedStart && size == expectedSize,
             "AC1024 class footer reconstructs extended string bounds");
    t.expect(buffer.getPosition() == expectedStart / 8
                 && buffer.getBitPos() == expectedStart % 8,
             "AC1024 class footer leaves cursor at string start");

    // The non-extended form is the same bit-relative calculation with only
    // the low size word. Keep this separate from the extension case so a
    // false high-bit read cannot pass by matching the extended vector.
    constexpr std::uint64_t shortFooterEndBit = 192;
    constexpr std::uint64_t shortStringSize = 37;
    constexpr std::uint64_t expectedShortStart =
        shortFooterEndBit - 16 - shortStringSize;
    dwgBufferW shortEncoded;
    for (std::uint64_t bit = 0; bit < shortFooterEndBit - 16; ++bit)
        shortEncoded.putBit(0);
    shortEncoded.putRawShort16(static_cast<std::uint16_t>(shortStringSize));
    shortEncoded.putBit(0);
    const std::vector<std::uint8_t> shortBytes = shortEncoded.data();
    dwgBuffer shortBuffer(shortBytes.data(), shortBytes.size());
    std::uint64_t shortStart = 0;
    std::uint64_t shortSize = 0;
    t.expect(readDwgClassStringFooter(shortBuffer, shortFooterEndBit,
                                      shortStart, shortSize)
                 && shortStart == expectedShortStart
                 && shortSize == shortStringSize
                 && shortBuffer.getPosition() == expectedShortStart / 8
                 && shortBuffer.getBitPos() == expectedShortStart % 8,
             "R2007 class footer decodes the unextended bit-sized string stream");

    std::vector<std::uint8_t> malformed(5200, 0);
    malformed[footerEndBit / 8 - 4] = 0xff;
    malformed[footerEndBit / 8 - 3] = 0xff;
    malformed[footerEndBit / 8 - 2] = 0xff;
    malformed[footerEndBit / 8 - 1] = 0xff;
    dwgBuffer bad(malformed.data(), malformed.size());
    bad.setPosition(10);
    bad.setBitPos(3);
    constexpr std::uint64_t unchangedStart = 0x1234;
    constexpr std::uint64_t unchangedSize = 0x5678;
    start = unchangedStart;
    size = unchangedSize;
    t.expect(!readDwgClassStringFooter(bad, footerEndBit, start, size),
             "AC1024 class footer rejects an overlong string-size extension");
    t.expect(bad.getPosition() == 10 && bad.getBitPos() == 3
                 && start == unchangedStart && size == unchangedSize,
             "malformed class footer preserves the caller cursor and outputs");

    // A flagged low word at the end of the available buffer has no room for
    // its high-size word. Refusal must remain atomic for both cursor and
    // output values.
    dwgBufferW truncatedEncoded;
    for (std::uint64_t bit = 0; bit < 15; ++bit)
        truncatedEncoded.putBit(0);
    truncatedEncoded.putRawShort16(0x8001);
    truncatedEncoded.putBit(0);
    const std::vector<std::uint8_t> truncatedBytes = truncatedEncoded.data();
    dwgBuffer truncated(truncatedBytes.data(), truncatedBytes.size());
    truncated.setPosition(1);
    truncated.setBitPos(2);
    start = unchangedStart;
    size = unchangedSize;
    t.expect(!readDwgClassStringFooter(truncated, 31, start, size)
                 && truncated.getPosition() == 1
                 && truncated.getBitPos() == 2
                 && start == unchangedStart && size == unchangedSize,
             "truncated extended class footer refuses atomically");

    std::uint64_t outOfRangeStart = unchangedStart;
    std::uint64_t outOfRangeSize = unchangedSize;
    dwgBuffer outOfRange(shortBytes.data(), shortBytes.size());
    outOfRange.setPosition(2);
    outOfRange.setBitPos(5);
    const bool rejectedOutOfRange = !readDwgClassStringFooter(
        outOfRange, static_cast<std::uint64_t>(shortBytes.size()) * 8,
        outOfRangeStart, outOfRangeSize);
    t.expect(rejectedOutOfRange && outOfRange.getPosition() == 2
                 && outOfRange.getBitPos() == 5
                 && outOfRangeStart == unchangedStart
                 && outOfRangeSize == unchangedSize,
             "out-of-range class footer preserves the caller cursor and outputs");

    // The footer offsets are bit positions, not byte indexes. Exercise every
    // possible trailing bit offset so the high-size word, low-size word, and
    // terminal bit remain correctly framed when the class-data end is not
    // byte-aligned.
    for (std::uint64_t bitOffset = 1; bitOffset < 8; ++bitOffset) {
        const std::uint64_t unalignedFooterEndBit = footerEndBit + bitOffset;
        dwgBufferW encoded;
        for (std::uint64_t bit = 0; bit < unalignedFooterEndBit - 32; ++bit)
            encoded.putBit(0);
        encoded.putRawShort16(static_cast<std::uint16_t>(highSize));
        encoded.putRawShort16(static_cast<std::uint16_t>(lowSize));
        encoded.putBit(0);
        const std::vector<std::uint8_t> unalignedBytes = encoded.data();

        dwgBuffer unaligned(unalignedBytes.data(), unalignedBytes.size());
        std::uint64_t unalignedStart = 0;
        std::uint64_t unalignedSize = 0;
        const std::uint64_t expectedUnalignedStart =
            unalignedFooterEndBit - 32 - expectedSize;
        const bool parsed = readDwgClassStringFooter(
            unaligned, unalignedFooterEndBit, unalignedStart, unalignedSize);
        t.expect(parsed && unalignedStart == expectedUnalignedStart
                     && unalignedSize == expectedSize
                     && unaligned.getPosition() == expectedUnalignedStart / 8
                     && unaligned.getBitPos() == expectedUnalignedStart % 8,
                 "AC1024 class footer preserves extended bounds at bit offset "
                     + std::to_string(bitOffset));
    }
}

void testAcDbPlaceholderClassDispatch(TestContext& t) {
    const auto parseClass = [](const std::string& recordName,
                               const std::string& className,
                               DRW_Class& result) {
        constexpr DRW::Version version = DRW::AC1015;
        dwgBufferW data;
        dwgBufferW strings;
        data.putBitShort(501u); // File-local class ordinal from the DWG.
        data.putBitShort(0u);   // Proxy capabilities.
        strings.putVariableText(version, "test-app");
        strings.putVariableText(version, className);
        strings.putVariableText(version, recordName);
        data.putBit(false); // was-a-proxy flag.
        data.putBitShort(0x1F3u); // OBJECT class.
        const auto dataBytes = data.data();
        const auto stringBytes = strings.data();
        dwgBuffer dataInput(dataBytes.data(), dataBytes.size());
        dwgBuffer stringInput(stringBytes.data(), stringBytes.size());
        return result.parseDwg(version, &dataInput, &stringInput)
            && dataInput.isGood() && stringInput.isGood();
    };

    DRW_Class placeholder;
    const bool parsedPlaceholder = parseClass(
        "ACDBPLACEHOLDER", "AcDbPlaceHolder", placeholder);
    t.expect(parsedPlaceholder && placeholder.classNum == 501u
                 && placeholder.dwgType == dwgObjType::ACDBPLACEHOLDER,
             "the built-in AcDbPlaceHolder class resolves to fixed DWG type 80 without using its file-local ordinal");

    DRW_Class unrelated;
    const bool parsedUnrelated = parseClass(
        "ACDBPLACEHOLDER", "AcDbUnrelated", unrelated);
    t.expect(parsedUnrelated && unrelated.dwgType == 0,
             "a mismatched C++ class name does not receive the placeholder parser");
}

class TemplateProbe final : public dwgReader15 {
public:
    explicit TemplateProbe(DRW::Version target)
        : dwgReader15(nullptr, nullptr) { version = target; }
    using dwgReader::decodeDwgTemplate;
    using dwgReader::missingDwgTemplate;
    const std::vector<DwgIntegrityDiagnostic>& diagnostics() const {
        return m_integrityDiagnostics;
    }
};

void testTemplateFields(TestContext& t) {
    for (auto version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                          DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        for (std::uint8_t value : {0, 1}) {
            const std::array<std::uint8_t, 6> padded{{0, 0, value, 0, 0x7f, 0x6a}};
            std::uint16_t measurement = 99;
            t.expect(dwgTemplate::decode(padded.data(), 4, version, measurement)
                         == dwgTemplate::DecodeResult::Valid
                         && measurement == value,
                     "Template consumes logical bytes, not page padding");
            t.expect(dwgTemplate::decode(padded.data(), padded.size(), version,
                                         measurement)
                         == dwgTemplate::DecodeResult::Malformed,
                     "Template does not scan trailing page bytes for a value");
            if (version >= DRW::AC1021) {
                const std::array<std::uint8_t, 5> canonicalNul{{1, 0, 0, value, 0}};
                t.expect(dwgTemplate::decode(canonicalNul.data(), canonicalNul.size(),
                                             version, measurement)
                             == dwgTemplate::DecodeResult::Valid
                             && measurement == value,
                         "modern canonical one-byte NUL description uses ODA byte count");
                const std::array<std::uint8_t, 6> oldEmpty{{1, 0, 0, 0, value, 0}};
                t.expect(dwgTemplate::decode(oldEmpty.data(), oldEmpty.size(),
                                             version, measurement)
                             == dwgTemplate::DecodeResult::Valid
                             && measurement == value,
                         "modern one-unit UTF16 NUL Template remains compatible");
            }
        }
        TemplateProbe missing(version);
        t.expect(missing.missingDwgTemplate(), "missing Template remains compatible");
        t.expect(version < DRW::AC1018 ? missing.diagnostics().empty()
                     : missing.diagnostics().size() == 1
                         && missing.diagnostics()[0].kind
                             == DwgIntegrityCheckKind::TemplateMissing
                         && missing.diagnostics()[0].severity
                             == DwgIntegritySeverity::Warning,
                 "mandatory modern Template absence has a bounded named warning");
        std::vector<std::uint8_t> maximumDescription(0x7fffu + 4u, 'A');
        maximumDescription[0] = 0xff;
        maximumDescription[1] = 0x7f;
        maximumDescription[maximumDescription.size() - 2] = 1;
        maximumDescription.back() = 0;
        std::uint16_t maximumMeasurement = 99;
        t.expect(dwgTemplate::decode(maximumDescription.data(),
                                     maximumDescription.size(), version,
                                     maximumMeasurement)
                     == dwgTemplate::DecodeResult::Valid
                     && maximumMeasurement == 1,
                 "Template accepts the largest signed byte-count description");
    }
    for (const std::vector<std::uint8_t>& bytes : {
             std::vector<std::uint8_t>{}, {0, 0}, {0, 0, 1}, {0, 0, 2, 0},
             {0xff, 0xff, 0, 0}, {0, 0x80, 0, 0},
             {4, 0, 'A', 0, 1, 0}, {1, 0, 0, 0, 1}, {1, 0, 0, 0, 2, 0}}) {
        TemplateProbe reader(DRW::AC1024);
        DRW_Header header;
        t.expect(!reader.decodeDwgTemplate(header, bytes.data(), bytes.size(), 5)
                     && header.vars.empty()
                     && reader.diagnostics().back().kind
                         == DwgIntegrityCheckKind::TemplateMalformed,
                 "malformed Template publishes no partial header variable");
    }
    const std::array<std::uint8_t, 7> legacyDescription{{3, 0, 'A', 'B', 'C', 1, 0}};
    std::uint16_t measurement = 99;
    for (auto version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                          DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        measurement = 99;
        t.expect(dwgTemplate::decode(legacyDescription.data(), legacyDescription.size(),
                                     version, measurement)
                     == dwgTemplate::DecodeResult::Valid && measurement == 1,
                 "all versions use the specified Template description byte count");
    }
    TemplateProbe modern(DRW::AC1024);
    DRW_Header header;
    const std::array<std::uint8_t, 8> noncanonicalModern{{2, 0, 'A', 0, 'B', 0, 1, 0}};
    t.expect(modern.decodeDwgTemplate(header, noncanonicalModern.data(),
                                      noncanonicalModern.size(), 5)
                 && header.vars.empty()
                 && modern.diagnostics().back().kind
                     == DwgIntegrityCheckKind::TemplateDescriptionUnsupported,
             "noncanonical modern description remains explicit, not guessed");
}

class TemplateSink final : public dx_iface {
public:
    TemplateSink() { cData = &data; currentBlock = data.mBlock; }
    void addHeader(const DRW_Header* header) override {
        ++headers;
        for (const auto& field : header->vars) {
            if (field.first == "MEASUREMENT") measurement = field.second->i_val();
            if (field.first == "INSUNITS") units = field.second->i_val();
        }
    }
    void addLine(const DRW_Line& line) override {
        if (line.basePoint.x == 11 && line.secPoint.z == 29) ++lines;
    }
    dx_data data;
    int headers = 0, measurement = -9, units = -9, lines = 0;
};

template<class Writer>
void testTemplatePublication(TestContext& t, dwgRW& reader, int measurement) {
    std::stringbuf bytes(std::ios::out | std::ios::binary);
    std::ofstream output;
    static_cast<std::ostream&>(output).rdbuf(&bytes);
    DRW_Header header;
    header.addInt("$MEASUREMENT", measurement, 70);
    const int units = measurement == 0 ? 4 : 1;
    header.addInt("$INSUNITS", units, 70);
    Writer writer(&output, &header);
    DRW_Line line;
    line.basePoint = {11, 13, 17}; line.secPoint = {19, 23, 29};
    const bool written = writer.prepareDwgClassManifest()
        && writer.writeFileHeaderStub() && writer.writeDwgHeader()
        && writer.writeDwgClasses() && writer.writeDwgObjects()
        && writer.encodeEntity(&line) && writer.emitDeferredBlockControl()
        && writer.finalizeHeaderHandseed() && writer.writeDwgHandles()
        && writer.writeSecondHeader() && writer.finalize();
    t.expect(written, "Template six-target memory writer completes");
    if (!written) return;
    const auto encoded = bytes.str();
    std::vector<std::uint8_t> source(encoded.begin(), encoded.end());
    const auto before = source;
    TemplateSink sink;
    t.expect(reader.readBuffer(source.data(), source.size(), &sink, false)
                 && sink.headers == 1 && sink.measurement == measurement
                 && sink.units == units && sink.lines == 1 && source == before,
             "one public header publishes Template measurement independent of units");
    if (std::is_same<Writer, dwgWriter15>::value) {
        // The Template locator is record4 in this locally generated file.
        std::size_t at = 0;
        for (unsigned i = 0; i < source[21]; ++i) {
            const std::size_t record = 25 + 9u * i;
            if (source[record] != 4) continue;
            at = static_cast<std::size_t>(source[record + 1])
                | static_cast<std::size_t>(source[record + 2]) << 8
                | static_cast<std::size_t>(source[record + 3]) << 16
                | static_cast<std::size_t>(source[record + 4]) << 24;
        }
        t.expect(at != 0 && at + 4 <= source.size(), "legacy Template locator is present");
        if (at != 0 && at + 4 <= source.size()) {
            source[at + 2] = 2;
            TemplateSink invalid;
            t.expect(!reader.readBuffer(source.data(), source.size(), &invalid, false)
                         && reader.getError() == DRW::BAD_READ_HEADER
                         && invalid.headers == 0 && invalid.lines == 0,
                     "invalid Template fails Header before public callback");
            TemplateSink retry;
            t.expect(reader.readBuffer(before.data(), before.size(), &retry, false)
                         && retry.measurement == measurement && retry.headers == 1,
                     "same public reader recovers without stale Template state");
            source = before;
            const unsigned count = source[21];
            for (unsigned i = 0; i < count; ++i) {
                const std::size_t record = 25 + 9u * i;
                if (source[record] == 4)
                    std::fill(source.begin() + record + 1,
                              source.begin() + record + 9, 0);
            }
            const std::size_t crcOffset = 25 + 9u * count;
            dwgBuffer crcBuffer(source.data(), source.size());
            const std::uint16_t seeds[] = {0, 0, 0, 0xa598, 0x8101, 0x3cc4, 0x8461};
            const auto crc = static_cast<std::uint16_t>(
                crcBuffer.crc8(0, 0, static_cast<std::int32_t>(crcOffset))
                ^ seeds[count]);
            source[crcOffset] = static_cast<std::uint8_t>(crc);
            source[crcOffset + 1] = static_cast<std::uint8_t>(crc >> 8);
            TemplateSink absent;
            t.expect(reader.readBuffer(source.data(), source.size(), &absent, false)
                         && absent.headers == 1 && absent.measurement == -9,
                     "absent optional legacy Template cannot reuse prior measurement");
        }
    }
}

void testTemplateFilePublication(TestContext& t) {
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-template-" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
    if (!std::filesystem::create_directory(directory)) {
        t.expect(false, "Template test scratch directory creation");
        return;
    }
    const auto path = directory / "destination.dwg";
    for (auto version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                          DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        for (int value : {-1, 2, (std::numeric_limits<int>::max)()}) {
            { std::ofstream existing(path, std::ios::binary); existing << "sentinel"; }
            TemplateSink source;
            source.data.headerC.addInt("$MEASUREMENT", value, 70);
            dwgRW writer(path.string().c_str());
            t.expect(!writer.write(&source, version, true)
                         && source.data.headerC.vars.at("$MEASUREMENT")->i_val() == value,
                     "invalid explicit measurement does not mutate caller header");
            std::ifstream unchanged(path, std::ios::binary);
            const std::string preserved((std::istreambuf_iterator<char>(unchanged)),
                                         std::istreambuf_iterator<char>());
            t.expect(preserved == "sentinel", "invalid Template preserves destination file");
            unchanged.close();
            source.data.headerC.addInt("$MEASUREMENT", 1, 70);
            t.expect(writer.write(&source, version, true),
                     "corrected public writer retries and publishes");
            TemplateSink result;
            dwgRW reader(path.string().c_str());
            t.expect(reader.read(&result, false) && result.headers == 1
                         && result.measurement == 1,
                     "file reader publishes corrected Template measurement");
        }
    }
    std::filesystem::remove(path);
    std::filesystem::remove(directory);
}

void testTemplatePublicMatrix(TestContext& t) {
    dwgRW reader("");
    for (int measurement : {0, 1}) {
        testTemplatePublication<dwgWriter15>(t, reader, measurement);
        testTemplatePublication<dwgWriter18>(t, reader, measurement);
        testTemplatePublication<dwgWriter21>(t, reader, measurement);
        testTemplatePublication<dwgWriter24>(t, reader, measurement);
        testTemplatePublication<dwgWriter27>(t, reader, measurement);
        testTemplatePublication<dwgWriter32>(t, reader, measurement);
    }
}

int runR2007PageCrcDiagnostic(const std::string& path,
                              const std::string& sectionName,
                              std::uint64_t pageId) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::cerr << "cannot open AC1021 sample: " << path << '\n';
        return 2;
    }
    std::vector<std::uint8_t> fileBytes(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>());
    if (fileBytes.empty()) {
        std::cerr << "empty AC1021 sample: " << path << '\n';
        return 2;
    }

    R21DataPageDecodeProbe reader(fileBytes);
    if (!reader.readSections()) {
        std::cerr << "failed to parse AC1021 file/section maps\n";
        return 2;
    }
    const dwgSectionInfo* section = reader.findSection(sectionName);
    if (section == nullptr) {
        std::cerr << "section not found: " << sectionName << '\n';
        return 2;
    }
    const auto pageIt = section->pages.find(static_cast<std::uint32_t>(pageId));
    if (pageIt == section->pages.end()) {
        std::cerr << "page not found: " << pageId << '\n';
        return 2;
    }
    const dwgPageInfo& page = pageIt->second;
    if (page.address > fileBytes.size()
        || page.size > fileBytes.size() - static_cast<std::size_t>(page.address)
        || page.cSize > page.size) {
        std::cerr << "invalid selected page bounds\n";
        return 2;
    }

    std::uint64_t alignedSize = 0u;
    if (!dwgSafety::alignUp8(page.cSize, alignedSize)) {
        std::cerr << "invalid page compressed-size alignment\n";
        return 2;
    }
    std::uint64_t chunksNumerator = 0u;
    if (!dwgSafety::add(alignedSize, 250u, chunksNumerator)) {
        std::cerr << "invalid page RS block count\n";
        return 2;
    }
    const std::uint64_t chunks64 = chunksNumerator / 251u;
    if (chunks64 == 0u
        || chunks64 > std::numeric_limits<std::uint32_t>::max()) {
        std::cerr << "invalid page RS block count\n";
        return 2;
    }

    const auto rawBegin = fileBytes.begin()
        + static_cast<std::ptrdiff_t>(page.address);
    std::vector<std::uint8_t> rawPage(
        rawBegin, rawBegin + static_cast<std::ptrdiff_t>(page.size));
    std::vector<std::uint8_t> rsDecoded(page.size, 0u);
    const bool rsDecodedOk = section->compressed == 1u
        ? dwgRSCodec::decode251(rawPage.data(), rsDecoded.data(),
                                static_cast<std::uint32_t>(chunks64))
        : section->compressed == 4u
            ? dwgRSCodec::decode251I(rawPage.data(), rsDecoded.data(),
                                     static_cast<std::uint32_t>(chunks64))
            : false;
    const std::vector<std::uint8_t> sourceRulePayload = section->compressed == 4u
        ? r2007DeinterleaveReference(
              rawPage.data(), rawPage.size(),
              static_cast<std::uint32_t>(chunks64), 251u)
        : std::vector<std::uint8_t>();
    std::size_t differingPayloadBytes = 0u;
    std::size_t firstDifferingPayloadByte = 0u;
    if (sourceRulePayload.size() >= page.cSize && rsDecodedOk) {
        for (std::size_t index = 0u;
             index < static_cast<std::size_t>(page.cSize); ++index) {
            if (sourceRulePayload[index] != rsDecoded[index]) {
                if (differingPayloadBytes == 0u)
                    firstDifferingPayloadByte = index;
                ++differingPayloadBytes;
            }
        }
    }

    // The tablet sample's decoded R2007 CrcSeed is zero. Recompute the two
    // candidate payload CRCs with the test-local bitwise implementation,
    // rather than relying on dwgUtil's table-based helper.
    const std::uint64_t crcSeed = r2007UpdateSeed1Reference(0u, page.cSize);
    const std::uint64_t rawCrc = r2007CrcBitwiseReference(
        crcSeed, rawPage.data(), static_cast<std::size_t>(page.cSize), true);
    const std::uint64_t rsCrc = rsDecodedOk
        ? r2007CrcBitwiseReference(
              crcSeed, rsDecoded.data(), static_cast<std::size_t>(page.cSize),
              true)
        : 0u;
    const std::uint64_t rsNormalCrc = rsDecodedOk
        ? r2007CrcBitwiseReference(
              crcSeed, rsDecoded.data(), static_cast<std::size_t>(page.cSize),
              false)
        : 0u;
    const std::uint64_t sourceRuleCrc = sourceRulePayload.size() >= page.cSize
        ? r2007CrcBitwiseReference(
              crcSeed, sourceRulePayload.data(),
              static_cast<std::size_t>(page.cSize), true)
        : 0u;

    std::cout << std::hex << std::setfill('0');
    std::cout << "SECTION name=" << section->name << " id=0x"
              << section->Id << " encoding=0x" << section->compressed
              << " encrypted=0x" << section->encrypted << " size=0x"
              << section->size << " maxSize=0x" << section->maxSize << '\n';
    std::cout << "PAGE id=0x" << page.Id << " address=0x" << page.address
              << " extent=0x" << page.size << " dataSize=0x"
              << page.dataSize << " startOffset=0x" << page.startOffset
              << " cSize=0x" << page.cSize << " uSize=0x" << page.uSize
              << " checksum=0x" << page.checksum << " crc=0x" << page.crc
              << '\n';
    std::cout << "CRC seedInput=0x0 seedAfterUpdate=0x" << crcSeed
              << " rawCandidate=0x" << rawCrc
              << " rsDecode=" << (rsDecodedOk ? "ok" : "failed")
              << " rsMirroredCandidate=0x" << rsCrc
              << " rsNormalCandidate=0x" << rsNormalCrc << '\n';
    std::cout << "SOURCE_RULE deinterleave="
              << (sourceRulePayload.size() >= page.cSize ? "ok" : "failed")
              << " matchesRsDecoder="
              << (rsDecodedOk && sourceRulePayload.size() >= page.cSize
                      && differingPayloadBytes == 0u ? "true" : "false")
              << " differingBytes=" << std::dec << differingPayloadBytes
              << " firstDifferingByte=";
    if (differingPayloadBytes == 0u) {
        std::cout << "none";
    } else {
        std::cout << "0x" << std::hex << firstDifferingPayloadByte;
    }
    std::cout << " mirroredCandidate=0x" << std::hex << sourceRuleCrc << '\n';

    if (section->size == 0u
        || section->size > std::numeric_limits<std::size_t>::max()) {
        std::cerr << "invalid selected section size\n";
        return 2;
    }
    std::vector<std::uint8_t> decoded(
        static_cast<std::size_t>(section->size), 0u);
    const bool parseSucceeded = reader.decode(*section, decoded.data());
    std::cout << "parseDataPage=" << (parseSucceeded ? "ok" : "failed")
              << '\n';
    if (parseSucceeded && page.uSize <= decoded.size()) {
        const auto logicalChecksum = dwgUtil::checksum21(
            0u, decoded.data(), page.uSize);
        std::cout << "logicalChecksum=0x" << logicalChecksum
                  << " storedChecksum=0x" << page.checksum << '\n';
    }
    for (const auto& diagnostic : reader.diagnostics()) {
        if (diagnostic.phase != DwgIntegrityPhase::DataPage
            || (diagnostic.kind != DwgIntegrityCheckKind::DataPageCrc
                && diagnostic.kind
                    != DwgIntegrityCheckKind::DataPageChecksum))
            continue;
        std::cout << "DIAGNOSTIC kind="
                  << static_cast<unsigned>(diagnostic.kind)
                  << " page=0x" << diagnostic.pageId << " offset=0x"
                  << diagnostic.fileOffset << " expected=0x"
                  << diagnostic.expected << " observed=0x"
                  << diagnostic.observed << '\n';
    }
    return 0;
}

struct R2007PageMapSystemDescriptor {
    std::uint64_t fileSize{0u};
    std::uint64_t pageId{0u};
    std::uint64_t pageOffset{0u};
    std::uint64_t compressedSize{0u};
    std::uint64_t uncompressedSize{0u};
    std::uint64_t correctionFactor{0u};
    std::uint64_t seedEncoded{0u};
    std::uint64_t seed{0u};
    std::uint64_t compressedCrc{0u};
    std::uint64_t uncompressedCrc{0u};
};

bool decodeR2007PageMapSystemDescriptor(
    const std::vector<std::uint8_t>& fileBytes,
    R2007PageMapSystemDescriptor& descriptor) {
    constexpr std::size_t fileHeaderOffset = 0x80u;
    constexpr std::size_t rawFileHeaderSize = 0x2fdu;
    constexpr std::size_t decodedFileHeaderSize = 0x2cdu;
    constexpr std::size_t fileHeaderDataSize = 0x110u;
    constexpr std::size_t fileHeaderPrefixSize = 32u;
    if (fileBytes.size() < fileHeaderOffset + rawFileHeaderSize)
        return false;

    const auto rawHeaderBegin =
        fileBytes.begin() + static_cast<std::ptrdiff_t>(fileHeaderOffset);
    std::vector<std::uint8_t> rawHeader(
        rawHeaderBegin,
        rawHeaderBegin + static_cast<std::ptrdiff_t>(rawFileHeaderSize));
    std::vector<std::uint8_t> decodedHeader(decodedFileHeaderSize, 0u);
    if (!dwgRSCodec::decode239I(rawHeader.data(), decodedHeader.data(), 3u))
        return false;
    dwgBuffer headerBuffer(decodedHeader.data(), decodedHeader.size());
    (void)headerBuffer.getRawLong64();
    (void)headerBuffer.getRawLong64();
    (void)headerBuffer.getRawLong64();
    const std::int32_t compressedHeaderSize = headerBuffer.getRawLong32();
    (void)headerBuffer.getRawLong32();
    if (!headerBuffer.isGood()
        || compressedHeaderSize == (std::numeric_limits<std::int32_t>::min)())
        return false;

    std::vector<std::uint8_t> fileHeaderData(fileHeaderDataSize, 0u);
    if (compressedHeaderSize > 0) {
        const auto compressedSize =
            static_cast<std::uint64_t>(compressedHeaderSize);
        if (compressedSize > decodedHeader.size() - fileHeaderPrefixSize)
            return false;
        dwgCompressor compressor;
        if (!compressor.decompress21(
                decodedHeader.data() + fileHeaderPrefixSize,
                fileHeaderData.data(), compressedSize, fileHeaderData.size()))
            return false;
    } else {
        const auto uncompressedSize = static_cast<std::uint64_t>(
            -static_cast<std::int64_t>(compressedHeaderSize));
        if (uncompressedSize != fileHeaderDataSize
            || uncompressedSize > decodedHeader.size() - fileHeaderPrefixSize)
            return false;
        std::copy_n(decodedHeader.data() + fileHeaderPrefixSize,
                    fileHeaderData.size(), fileHeaderData.begin());
    }

    dwgBuffer dataBuffer(fileHeaderData.data(), fileHeaderData.size());
    (void)dataBuffer.getRawLong64();
    descriptor.fileSize = dataBuffer.getRawLong64();
    descriptor.compressedCrc = dataBuffer.getRawLong64();
    descriptor.correctionFactor = dataBuffer.getRawLong64();
    descriptor.seedEncoded = dataBuffer.getRawLong64();
    (void)dataBuffer.getRawLong64();
    (void)dataBuffer.getRawLong64();
    descriptor.pageOffset = dataBuffer.getRawLong64();
    descriptor.pageId = dataBuffer.getRawLong64();
    (void)dataBuffer.getRawLong64();
    descriptor.compressedSize = dataBuffer.getRawLong64();
    descriptor.uncompressedSize = dataBuffer.getRawLong64();
    (void)dataBuffer.getRawLong64();
    (void)dataBuffer.getRawLong64();
    (void)dataBuffer.getRawLong64();
    (void)dataBuffer.getRawLong64();
    descriptor.uncompressedCrc = dataBuffer.getRawLong64();
    if (!dataBuffer.isGood())
        return false;
    descriptor.seed = r2007DecodeCrcSeedReference(descriptor.seedEncoded);
    return descriptor.fileSize == fileBytes.size()
        && descriptor.pageId != 0u && descriptor.compressedSize != 0u
        && descriptor.uncompressedSize != 0u
        && descriptor.correctionFactor != 0u;
}

int runR2007SystemPageCrcDiagnostic(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        std::cerr << "cannot open AC1021 sample: " << path << '\n';
        return 2;
    }
    std::vector<std::uint8_t> fileBytes(
        (std::istreambuf_iterator<char>(input)),
        std::istreambuf_iterator<char>());
    if (fileBytes.empty()) {
        std::cerr << "empty AC1021 sample: " << path << '\n';
        return 2;
    }

    R2007PageMapSystemDescriptor descriptor;
    if (!decodeR2007PageMapSystemDescriptor(fileBytes, descriptor)) {
        std::cerr << "failed to independently extract R2007 page-map header fields\n";
        return 2;
    }
    if (descriptor.seed != dwgUtil::decodeCrcSeed(descriptor.seedEncoded)) {
        std::cerr << "test-local and production CRC seed decoders disagree\n";
        return 2;
    }
    if (descriptor.uncompressedSize > 128ULL * 1024ULL * 1024ULL
        || descriptor.uncompressedSize
            > std::numeric_limits<std::size_t>::max()) {
        std::cerr << "R2007 system page exceeds diagnostic output limit\n";
        return 2;
    }

    std::uint64_t alignedSize = 0u;
    std::uint64_t scaledSize = 0u;
    std::uint64_t chunkNumerator = 0u;
    if (!dwgSafety::alignUp8(descriptor.compressedSize, alignedSize)
        || !dwgSafety::multiply(alignedSize, descriptor.correctionFactor,
                                scaledSize)
        || !dwgSafety::add(scaledSize, 238u, chunkNumerator)) {
        std::cerr << "invalid R2007 system page block geometry\n";
        return 2;
    }
    const std::uint64_t chunks64 = chunkNumerator / 239u;
    std::uint64_t encodedSize = 0u;
    std::uint64_t offset = 0u;
    if (chunks64 == 0u
        || chunks64 > std::numeric_limits<std::uint32_t>::max()
        || !dwgSafety::multiply(chunks64, 255u, encodedSize)
        || !dwgSafety::add(0x480u, descriptor.pageOffset, offset)
        || !dwgSafety::range(offset, encodedSize, fileBytes.size())
        || descriptor.compressedSize > chunks64 * 239u) {
        std::cerr << "R2007 system page bounds or size declarations are invalid\n";
        return 2;
    }

    const auto rawBegin = fileBytes.begin() + static_cast<std::ptrdiff_t>(offset);
    std::vector<std::uint8_t> rawPage(
        rawBegin, rawBegin + static_cast<std::ptrdiff_t>(encodedSize));
    std::vector<std::uint8_t> rsDecoded(
        static_cast<std::size_t>(chunks64) * 239u, 0u);
    const bool rsDecodedOk = dwgRSCodec::decode239I(
        rawPage.data(), rsDecoded.data(), static_cast<std::uint32_t>(chunks64));
    const std::vector<std::uint8_t> sourceRulePayload =
        r2007DeinterleaveReference(
            rawPage.data(), rawPage.size(),
            static_cast<std::uint32_t>(chunks64), 239u);
    std::size_t differingPayloadBytes = 0u;
    std::size_t firstDifferingPayloadByte = 0u;
    if (sourceRulePayload.size() >= descriptor.compressedSize && rsDecodedOk) {
        for (std::size_t index = 0u;
             index < static_cast<std::size_t>(descriptor.compressedSize);
             ++index) {
            if (sourceRulePayload[index] != rsDecoded[index]) {
                if (differingPayloadBytes == 0u)
                    firstDifferingPayloadByte = index;
                ++differingPayloadBytes;
            }
        }
    }

    R21DataPageDecodeProbe reader(fileBytes);
    if (!reader.readSections()) {
        std::cerr << "failed to parse AC1021 file/section maps\n";
        return 2;
    }
    const std::size_t firstDiagnostic = reader.diagnostics().size();
    std::vector<std::uint8_t> uncompressed(
        static_cast<std::size_t>(descriptor.uncompressedSize), 0u);
    const bool parseSucceeded = reader.decodePageMapSystemPage(
        descriptor.compressedSize, descriptor.uncompressedSize,
        descriptor.correctionFactor, offset, descriptor.seed,
        descriptor.compressedCrc, descriptor.uncompressedCrc, uncompressed);

    const auto compressedSeed = r2007UpdateSeed1Reference(
        descriptor.seed, descriptor.compressedSize);
    const auto uncompressedSeed = r2007UpdateSeed1Reference(
        descriptor.seed, descriptor.uncompressedSize);
    const std::uint64_t compressedCrc = sourceRulePayload.size()
            >= descriptor.compressedSize
        ? r2007CrcBitwiseReference(
              compressedSeed, sourceRulePayload.data(),
              static_cast<std::size_t>(descriptor.compressedSize), true)
        : 0u;
    const std::uint64_t uncompressedCrc = parseSucceeded
        ? r2007CrcBitwiseReference(
              uncompressedSeed, uncompressed.data(), uncompressed.size(), true)
        : 0u;

    std::cout << std::hex << std::setfill('0');
    std::cout << "SYSTEM_PAGE kind=page-map id=0x" << descriptor.pageId
              << " address=0x" << offset << " encodedSize=0x" << encodedSize
              << " compressedSize=0x" << descriptor.compressedSize
              << " uncompressedSize=0x" << descriptor.uncompressedSize
              << " correctionFactor=0x" << descriptor.correctionFactor
              << " seedEncoded=0x" << descriptor.seedEncoded
              << " decodedSeed=0x" << descriptor.seed
              << " seedDecoderMatch=true\n";
    std::cout << "SYSTEM_CRC compressedSeed=0x" << compressedSeed
              << " computedCompressed=0x" << compressedCrc
              << " storedCompressed=0x" << descriptor.compressedCrc
              << " uncompressedSeed=0x" << uncompressedSeed
              << " computedUncompressed=0x" << uncompressedCrc
              << " storedUncompressed=0x" << descriptor.uncompressedCrc
              << " rsDecode=" << (rsDecodedOk ? "ok" : "failed")
              << " sourceRule="
              << (sourceRulePayload.size() >= descriptor.compressedSize
                      ? "ok" : "failed")
              << " matchesRsDecoder="
              << (rsDecodedOk && sourceRulePayload.size()
                      >= descriptor.compressedSize
                      && differingPayloadBytes == 0u ? "true" : "false")
              << " differingBytes=" << std::dec << differingPayloadBytes
              << " firstDifferingByte=";
    if (differingPayloadBytes == 0u)
        std::cout << "none";
    else
        std::cout << "0x" << std::hex << firstDifferingPayloadByte;
    std::cout << std::hex;
    std::cout << " parseSysPage=" << (parseSucceeded ? "ok" : "failed")
              << '\n';
    for (std::size_t index = firstDiagnostic;
         index < reader.diagnostics().size(); ++index) {
        const auto& diagnostic = reader.diagnostics()[index];
        if (diagnostic.phase != DwgIntegrityPhase::PageMap
            || (diagnostic.kind != DwgIntegrityCheckKind::SystemPageCrc
                && diagnostic.kind
                    != DwgIntegrityCheckKind::SystemPageUncompressedCrc))
            continue;
        std::cout << "DIAGNOSTIC kind="
                  << static_cast<unsigned>(diagnostic.kind)
                  << " expected=0x" << diagnostic.expected
                  << " observed=0x" << diagnostic.observed << '\n';
    }
    return 0;
}

int runDwgReadSample(const std::string& path) {
    dx_data data;
    dx_iface interface;
    const bool imported = interface.fileImport(path, &data, false);
    dx_iface::reportDwgReadOutcome(std::cerr, data.dwgReadOutcome);
    const dx_dwgReadOutcome& outcome = data.dwgReadOutcome;
    const DwgReadObservations& observations = outcome.observations;
    if (!imported || data.sourceFormat != dx_source_format::DWG
        || !outcome.attempted || !outcome.operationSucceeded
        || outcome.error != DRW::BAD_NONE
        || observations.entityParseFailures != 0u
        || observations.objectParseFailures != 0u) {
        std::cerr << "DWG sample read probe failed: import=" << imported
                  << " format=" << static_cast<unsigned>(data.sourceFormat)
                  << " attempted=" << outcome.attempted
                  << " operationSucceeded=" << outcome.operationSucceeded
                  << " error=" << static_cast<int>(outcome.error)
                  << " entityParseFailures="
                  << observations.entityParseFailures
                  << " objectParseFailures="
                  << observations.objectParseFailures << '\n';
        return 1;
    }
    std::cout << "DWG sample read probe: PASS\n";
    return 0;
}

int runCustomObjectsAc1024Sample(const std::string& path) {
    std::ifstream sample(path, std::ios::binary);
    char version[6] = {};
    if (!sample.read(version, sizeof(version))
        || std::string(version, sizeof(version)) != "AC1024") {
        std::cerr << "BLOCKLOOKUPACTION sample is not AC1024\n";
        return 1;
    }
    AssociativeObjectRecordingSink sink;
    dx_data& data = sink.data;
    const bool imported = sink.fileImport(path, &data, false);
    const auto object = std::find_if(
        data.dynamicBlockObjects.cbegin(), data.dynamicBlockObjects.cend(),
        [](const DRW_DynamicBlockObject& item) {
            return item.handle == 559u
                && item.m_recordName == "BLOCKLOOKUPACTION";
        });
    const bool columnsMatch = object != data.dynamicBlockObjects.cend()
        && object->m_lookupConnectionCodes.size() == 7u
        && object->m_lookupConnectionNames.size() == 7u
        && object->m_lookupEntryFlags282.size() == 7u
        && object->m_lookupEntryFlags281.size() == 7u
        && std::all_of(
            object->m_lookupConnectionCodes.cbegin(),
            object->m_lookupConnectionCodes.cend(),
            [](const std::vector<std::int32_t>& codes) {
                return codes.size() == 3u;
            })
        && std::all_of(
            object->m_lookupConnectionNames.cbegin(),
            object->m_lookupConnectionNames.cend(),
            [](const std::vector<UTF8STRING>& names) {
                return names.size() == 2u;
            });
    const auto manager = std::find_if(
        sink.associativeObjects.cbegin(), sink.associativeObjects.cend(),
        [](const DRW_AssociativeObject& item) {
            return item.handle == 363u
                && item.m_recordName == "ACDBPERSSUBENTMANAGER";
        });
    const auto managerRaw = std::find_if(
        sink.rawObjects.cbegin(), sink.rawObjects.cend(),
        [](const DRW_UnsupportedObject& item) {
            return item.m_handle == 363u
                && item.m_recordName == "ACDBPERSSUBENTMANAGER";
        });
    const bool managerPrefixMatches =
        manager != sink.associativeObjects.cend()
        && manager->m_classVersion == 2u
        && manager->m_persistentUnknown0 == 0u
        && manager->m_persistentUnknown2 == 2u
        && manager->m_persistentAssocStepCount == 2u
        && manager->m_persistentAssocSubentityCount == 0u
        && manager->m_persistentSteps.size() == 2u
        && manager->m_persistentSteps[0] == 1u
        && manager->m_persistentSteps[1] == 2u
        && manager->m_persistentSubentityIds.empty()
        && manager->parentHandle == 0xCu
        && manager->reactorHandles == std::vector<std::uint32_t>{0xCu}
        && manager->m_prefixStatuses.size() == 1u
        && manager->m_prefixStatuses.front().m_kind
            == DRW_AssociativePrefixStatus::Kind::AcDbAssocActionBody
        && manager->m_prefixStatuses.front().m_startBit == 47u
        && manager->m_prefixStatuses.front().m_bitSize == 64u
        && manager->m_prefixStatuses.front().m_status
            == DRW_AssociativePrefixStatus::ParseStatus::Partial
        && manager->m_prefixStatuses.front().m_decodedValueCount == 2u
        && manager->m_prefixStatuses.front().m_decodedCountValue == -1
        && manager->m_prefixStatuses.front().m_sourceAssumption.find(
            "AC1024") != UTF8STRING::npos
        && managerRaw != sink.rawObjects.cend()
        && managerRaw->m_version == DRW::AC1024
        && managerRaw->m_objectType == 515
        && managerRaw->m_recordName == "ACDBPERSSUBENTMANAGER"
        && managerRaw->m_className == "AcDbPersSubentManager"
        && managerRaw->m_isCustomClass
        && managerRaw->m_hasClassDefinition
        && !managerRaw->m_typedPayloadValidated
        && managerRaw->m_rawBytes.size() == 18u
        && managerRaw->m_objectSize == managerRaw->m_rawBytes.size()
        && managerRaw->m_bodyBitSize > 0u
        && managerRaw->m_bodyBitSize <= managerRaw->m_objectSize * 8u;
    if (!imported || data.sourceFormat != dx_source_format::DWG
        || data.dwgReadOutcome.observations.objectParseFailures != 0u
        || object == data.dynamicBlockObjects.cend()
        || object->m_kind != DRW_DynamicBlockObject::Kind::LookupAction
        || !object->m_bodyFullyDecoded
        || object->m_lookupRowCount != 41
        || object->m_lookupColumnCount != 7
        || object->m_lookupExpressions.size() != 287u
        || !columnsMatch || !managerPrefixMatches) {
        std::cerr << "AC1024 custom-object profiles failed: import="
                  << imported << " dynamicObjects="
                  << data.dynamicBlockObjects.size()
                  << " associativeObjects=" << sink.associativeObjects.size()
                  << " objectParseFailures="
                  << data.dwgReadOutcome.observations.objectParseFailures;
        if (object != data.dynamicBlockObjects.cend()) {
            std::cerr << " bodyFullyDecoded=" << object->m_bodyFullyDecoded
                      << " rows=" << object->m_lookupRowCount
                      << " columns=" << object->m_lookupColumnCount
                      << " expressions=" << object->m_lookupExpressions.size()
                      << " columnRecords="
                      << object->m_lookupConnectionCodes.size();
        }
        if (manager != sink.associativeObjects.cend()) {
            std::cerr << " managerSteps="
                      << manager->m_persistentSteps.size()
                      << " unknown=" << manager->m_persistentUnknown0
                      << '/' << manager->m_persistentUnknown2
                      << " assocCounts=" << manager->m_persistentAssocStepCount
                      << '/' << manager->m_persistentAssocSubentityCount
                      << " managerPrefixStatuses="
                      << manager->m_prefixStatuses.size();
            if (!manager->m_prefixStatuses.empty()) {
                const DRW_AssociativePrefixStatus& status =
                    manager->m_prefixStatuses.front();
                std::cerr << " span=" << status.m_startBit << '+' << status.m_bitSize
                          << " values=" << status.m_decodedValueCount
                          << " count=" << status.m_decodedCountValue
                          << " assumption=" << status.m_sourceAssumption;
            }
            std::cerr << " owner=" << manager->parentHandle
                      << " reactors=" << manager->reactorHandles.size();
            for (const std::uint32_t reactor : manager->reactorHandles)
                std::cerr << ',' << reactor;
        }
        if (managerRaw != sink.rawObjects.cend())
            std::cerr << " managerRawValidated="
                      << managerRaw->m_typedPayloadValidated
                      << " managerRawBytes=" << managerRaw->m_rawBytes.size()
                      << " managerRawObjectSize=" << managerRaw->m_objectSize
                      << " managerRawBodyBits=" << managerRaw->m_bodyBitSize
                      << " managerRawType=" << managerRaw->m_objectType
                      << " managerRawClass=" << managerRaw->m_className;
        std::cerr << '\n';
        return 1;
    }
    std::cout << "AC1024 custom-object profiles: PASS BLOCKLOOKUPACTION="
                 "41x7/287 expressions; PERSUBENTMGR=partial steps [1,2]"
                 " raw-only objectParseFailures=0\n";
    return 0;
}

int runAcShOpaqueFrameCapture(const std::string& path) {
    dx_data data;
    dx_iface interface;
    const bool imported = interface.fileImport(path, &data, false);
    dx_iface::reportDwgReadOutcome(std::cerr, data.dwgReadOutcome);
    const dx_dwgReadOutcome& outcome = data.dwgReadOutcome;
    const DwgReadObservations& observations = outcome.observations;
    const std::array<std::uint32_t, 3> expectedHandles{{1706u, 1747u, 1788u}};
    std::size_t expectedParseFailures = 0u;
    for (const std::uint32_t handle : expectedHandles) {
        const auto raw = std::find_if(
            data.acdsHistoryDwgFrames.cbegin(),
            data.acdsHistoryDwgFrames.cend(),
            [handle](const DRW_UnsupportedObject& frame) {
                return frame.m_handle == handle;
            });
        if (raw == data.acdsHistoryDwgFrames.cend()) {
            std::cerr << "ACSH revolve raw frame was not retained for handle "
                      << handle << '\n';
            return 1;
        }
        if (raw->m_version != DRW::AC1024
            || raw->m_recordName != "ACSH_REVOLVE_CLASS"
            || raw->m_className != "AcDbShRevolve"
            || raw->m_rawBytes.empty()
            || raw->m_objectSize != raw->m_rawBytes.size()
            || raw->m_bodyBitSize != 34u) {
            std::cerr << "ACSH revolve raw frame metadata disagreed for handle "
                      << handle << " (bytes=" << raw->m_rawBytes.size()
                      << ", objectSize=" << raw->m_objectSize
                      << ", handleBits=" << raw->m_bodyBitSize << ")\n";
            return 1;
        }
        const auto typed = std::find_if(
            data.acshHistoryObjects.cbegin(), data.acshHistoryObjects.cend(),
            [handle](const DRW_AcShHistoryObject& object) {
                return object.handle == handle;
            });
        if (typed == data.acshHistoryObjects.cend())
            ++expectedParseFailures;
    }
    if (!imported || data.sourceFormat != dx_source_format::DWG
        || !outcome.attempted || !outcome.operationSucceeded
        || outcome.error != DRW::BAD_NONE
        || observations.entityParseFailures != 0u
        || observations.objectParseFailures != expectedParseFailures
        || data.acdsHistoryDwgFrameCaptureFailed) {
        std::cerr << "ACSH raw-frame probe disagreed with typed disposition: "
                  << "import=" << imported
                  << " objectParseFailures=" << observations.objectParseFailures
                  << " expected=" << expectedParseFailures << '\n';
        return 1;
    }
    std::cout << "ACSH revolve opaque-frame retention: PASS (typed failures="
              << expectedParseFailures << ")\n";
    return 0;
}

int runAcShFailedFrameRetention(const std::string& path) {
    dx_data data;
    dx_iface interface;
    const bool imported = interface.fileImport(path, &data, false);
    const dx_dwgReadOutcome& outcome = data.dwgReadOutcome;
    const DwgReadObservations& observations = outcome.observations;
    const auto startsWith = [](const std::string& value,
                               const char* prefix) {
        return value.rfind(prefix, 0u) == 0u;
    };
    std::size_t untypedAcShFrames = 0u;
    for (const DRW_UnsupportedObject& raw : data.acdsHistoryDwgFrames) {
        if (!startsWith(raw.m_recordName, "ACSH_")
            && !startsWith(raw.m_className, "AcDbSh"))
            continue;
        const auto typed = std::find_if(
            data.acshHistoryObjects.cbegin(), data.acshHistoryObjects.cend(),
            [&raw](const DRW_AcShHistoryObject& object) {
                return object.handle == raw.m_handle;
            });
        if (typed != data.acshHistoryObjects.cend())
            continue;
        ++untypedAcShFrames;
        if (raw.m_isEntity || !raw.m_isCustomClass
            || !raw.m_hasClassDefinition || raw.m_version != DRW::AC1024
            || raw.m_recordName.empty() || raw.m_className.empty()
            || raw.m_rawBytes.empty()
            || raw.m_objectSize != raw.m_rawBytes.size()
            || raw.m_bodyBitSize > raw.m_objectSize * 8u) {
            std::cerr << "Malformed retained ACSH frame for handle "
                      << raw.m_handle << " record=" << raw.m_recordName
                      << " class=" << raw.m_className << '\n';
            return 1;
        }
    }
    if (!imported || data.sourceFormat != dx_source_format::DWG
        || !outcome.attempted || !outcome.operationSucceeded
        || outcome.error != DRW::BAD_NONE
        || observations.entityParseFailures != 0u
        || observations.objectParseFailures != 3u
        || untypedAcShFrames != observations.objectParseFailures
        || data.acdsHistoryDwgFrameCaptureFailed) {
        std::cerr << "ACSH failed-frame retention did not match the partial "
                  << "sample disposition: import=" << imported
                  << " objectParseFailures="
                  << observations.objectParseFailures
                  << " untypedAcShFrames=" << untypedAcShFrames << '\n';
        dx_iface::reportDwgReadOutcome(std::cerr, outcome);
        return 1;
    }
    std::cout << "ACSH failed-frame retention: PASS (opaque frames="
              << untypedAcShFrames << ", typed failures remain visible)\n";
    return 0;
}

int runAssociativeRawCarrierRetention(const std::string& path) {
    AssociativeObjectRecordingSink sink;
    const bool imported = sink.fileImport(path, &sink.data, false);
    const dx_dwgReadOutcome& outcome = sink.data.dwgReadOutcome;
    const DwgReadObservations& observations = outcome.observations;
    struct ExpectedCarrier {
        std::uint32_t handle;
        std::uint32_t type;
        bool typedPayloadValidated;
    };
    const std::array<ExpectedCarrier, 2> expected{{
        {559u, 528u, false}, // decoded BLOCKLOOKUPACTION; raw remains unvalidated
        {363u, 515u, false} // partial PERSUBENTMGR prefix
    }};
    for (const auto& item : expected) {
        const auto raw = std::find_if(
            sink.rawObjects.cbegin(), sink.rawObjects.cend(),
            [&item](const DRW_UnsupportedObject& object) {
                return object.m_handle == item.handle
                    && object.m_objectType == static_cast<int>(item.type);
            });
        if (raw == sink.rawObjects.cend()) {
            std::cerr << "Associative-family raw frame was not retained for "
                      << "handle " << item.handle << " type " << item.type
                      << '\n';
            return 1;
        }
        if (raw->m_version != DRW::AC1024 || raw->m_isEntity
            || !raw->m_isCustomClass || !raw->m_hasClassDefinition
            || raw->m_typedPayloadValidated != item.typedPayloadValidated
            || raw->m_recordName.empty()
            || raw->m_className.empty() || raw->m_rawBytes.empty()
            || raw->m_objectSize != raw->m_rawBytes.size()
            || raw->m_bodyBitSize > raw->m_objectSize * 8u) {
            std::cerr << "Malformed associative-family raw frame for handle "
                      << item.handle << " type " << item.type
                      << " record=" << raw->m_recordName
                      << " class=" << raw->m_className << '\n';
            return 1;
        }
    }

    if (!imported || sink.data.sourceFormat != dx_source_format::DWG
        || !outcome.attempted || !outcome.operationSucceeded
        || outcome.error != DRW::BAD_NONE
        || observations.entityParseFailures != 0u
        || observations.objectParseFailures != 0u) {
        std::cerr << "Associative raw-carrier retention changed the sample "
                  << "read disposition: import=" << imported
                  << " objectParseFailures="
                  << observations.objectParseFailures
                  << " expected=0\n";
        dx_iface::reportDwgReadOutcome(std::cerr, outcome);
        return 1;
    }
    std::cout << "Associative custom-object carriers: PASS (complete lookup-action "
                 "typed; manager prefix partial/raw-only; parse failures=0)\n";
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 3 && std::string(argv[1]) == "--persistent-manager-source-prefix")
        return runPersistentManagerSourcePrefix(argv[2]);
    if (argc == 2 && std::string(argv[1]) == "--persistent-manager-prefix-dispatch") {
        TestContext focused;
        testPersistentManagerPrefixDispatch(focused);
        std::cout << "Persistent manager bounded-prefix dispatch: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--associative-constraint-group-generated") {
        TestContext focused;
        testAssociativeConstraintGroupGenerated(focused);
        std::cout << "Generated AC1024 associative constraint-group profile: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--associative-network-action-index-generated") {
        TestContext focused;
        testAssociativeNetworkActionIndexGenerated(focused);
        std::cout << "Generated AC1024 associative network action indexes: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--associative-variable-generated") {
        TestContext focused;
        testAssociativeVariableGenerated(focused);
        std::cout << "Generated AC1024 associative variable body: "
                  << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--tablestyle-malformed-count-dispatch") {
        TestContext focused;
        testTableStyleMalformedCountDoesNotBlockFollowingObject(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--r2018-auxheader-trailer") {
        TestContext focused;
        testR2018AuxHeaderTrailer(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 3
        && std::string(argv[1]) == "--acsh-opaque-frame-capture")
        return runAcShOpaqueFrameCapture(argv[2]);
    if (argc == 3
        && std::string(argv[1]) == "--acsh-failed-frame-retention")
        return runAcShFailedFrameRetention(argv[2]);
    if (argc == 3
        && std::string(argv[1]) == "--associative-custom-object-raw-retention")
        return runAssociativeRawCarrierRetention(argv[2]);
    if (argc == 3 && std::string(argv[1]) == "--read-dwg-sample")
        return runDwgReadSample(argv[2]);
    if (argc == 3
        && std::string(argv[1]) == "--custom-objects-ac1024-sample")
        return runCustomObjectsAc1024Sample(argv[2]);
    if (argc == 3
        && std::string(argv[1]) == "--r2007-system-page-crc-diagnostic") {
        return runR2007SystemPageCrcDiagnostic(argv[2]);
    }
    if (argc == 5
        && std::string(argv[1]) == "--r2007-page-crc-diagnostic") {
        try {
            const auto pageId = std::stoull(argv[4], nullptr, 0);
            return runR2007PageCrcDiagnostic(argv[2], argv[3], pageId);
        } catch (const std::exception& error) {
            std::cerr << "invalid page id: " << error.what() << '\n';
            return 2;
        }
    }
    if (argc == 2 && std::string(argv[1]) == "--r2007-crc-literal-vectors") {
        TestContext focused;
        testR2007CrcLiteralVectors(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--r2007-class-string-footer") {
        TestContext focused;
        testR2007ClassStringFooter(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--acdb-placeholder-class-dispatch") {
        TestContext focused;
        testAcDbPlaceholderClassDispatch(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--r2007-data-page-profiles") {
        TestContext focused;
        testR2007DataPageProfiles(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--r2007-section-map-page-extent") {
        TestContext focused;
        testR2007SectionMapPageExtent(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--r2004-data-page-logical-tail") {
        TestContext focused;
        testR2004DataPageLogicalTail(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--header-date-time-validation") {
        TestContext focused;
        testHeaderDateTimeEncoding(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--polyline-staging-growth") {
        TestContext focused;
        testBoundedPolylineStagingGrowth(focused);
        testPolylineStageRollbackTranscript(focused);
        testLocatedPolylineCallbackPublication(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--insert-staging-index") {
        TestContext focused;
        testInsertAttributeStagingIndex(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--block-record-xref-index") {
        TestContext focused;
        testBlockRecordXrefIndexRetention(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--residual-entity-sweep") {
        TestContext focused;
        testResidualDwgEntitySweep(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--mixed-missing-block-ownership") {
        TestContext focused;
        testMixedMissingBlockOwnership(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--space-block-delimiter-owner") {
        TestContext focused;
        testSpaceBlockDelimiterOwner(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--acds-modeler-move-callback") {
        TestContext focused;
        testOwnedUnsupportedObjectJournalEvent(focused);
        testModelerBlockRecordDispatch(focused);
        testModelerAcDsBlobPageChain(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--acds-dataindex-order") {
        TestContext focused;
        testModelerAcDsDataIndexOrder(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--acds-record-dispositions") {
        TestContext focused;
        testModelerAcDsRecordDispositions(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--acds-overlapping-record-headers") {
        TestContext focused;
        testModelerAcDsOverlappingRecordHeaders(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--legacy-layout-block-chains") {
        TestContext focused;
        testLegacyLayoutBlockChains(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--proxy-entity-dispatch") {
        TestContext focused;
        testProxyEntityDispatch(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--viewport-object-frames") {
        TestContext focused;
        testViewportObjectFrameSerialization(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--viewport-handles-section") {
        TestContext focused;
        testFileLevelViewportHandlesSection(focused);
        testFileLevelVportHandlesSectionR2004<dwgWriter18>(
            focused, DRW::AC1018, "AC1018");
        testFileLevelVportHandlesSectionR2004<dwgWriter27>(
            focused, DRW::AC1027, "AC1027");
        testFileLevelVportHandlesSectionR2004<dwgWriter32>(
            focused, DRW::AC1032, "AC1032");
        testFileLevelViewportHandlesSectionModern(focused);
        testFileLevelViewportHandlesSectionR21(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--r11-unsupported-markers") {
        TestContext focused;
        testR11UnsupportedMarkersFailClosed(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 2
        && std::string(argv[1]) == "--r11-paper-space-projection") {
        TestContext focused;
        testR11PaperSpaceProjection(focused);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }
    if (argc == 3
        && std::string(argv[1]) == "--write-r11-paper-space-control") {
        TestContext focused;
        testR11PaperSpaceProjection(focused, argv[2]);
        std::cout << (focused.failures == 0 ? "PASS" : "FAIL") << '\n';
        return focused.failures == 0 ? 0 : 1;
    }

    TestContext context;
    testR2007CrcLiteralVectors(context);
    testDisjointRangeIndex(context);
    testBoundedPolylineStagingGrowth(context);
    testResidualDwgEntitySweep(context);
    testPolylineStageRollbackTranscript(context);
    testLocatedPolylineCallbackPublication(context);
    testInsertAttributeStagingIndex(context);
    testProxyEntityDispatch(context);
    testViewportObjectFrameSerialization(context);
    testFileLevelViewportHandlesSection(context);
    testFileLevelVportHandlesSectionR2004<dwgWriter18>(
        context, DRW::AC1018, "AC1018");
    testFileLevelVportHandlesSectionR2004<dwgWriter27>(
        context, DRW::AC1027, "AC1027");
    testFileLevelVportHandlesSectionR2004<dwgWriter32>(
        context, DRW::AC1032, "AC1032");
    testFileLevelViewportHandlesSectionModern(context);
    testFileLevelViewportHandlesSectionR21(context);
    testExactUnknownSectionNames(context);
    testR2004DataPageLogicalTail(context);
    testAcDbPlaceholderClassDispatch(context);
    testR2007DataPageProfiles(context);
    testR2007SectionMapPageExtent(context);
    testR2004SectionMapPublication(context);
    testModelerBlockRecordDispatch(context);
    testBlockRecordXrefIndexRetention(context);
    testLegacyLayoutBlockChains(context);
    testModelerBlockWalkRejectsBadOwnership(context);
    testSpaceBlockDelimiterOwner(context);
    testMixedMissingBlockOwnership(context);
    testBoundedEedApplicationChunks(context);
    testMalformedEedEntityDoesNotBlockFollowingEntity(context);
    testModelerInsertTargetResolution(context);
    testModelerAcDsBodyDispatch(context);
    testModelerAcDsRegionDispatch(context);
    testModelerAcDsRecordDispositions(context);
    testModelerAcDsOverlappingRecordHeaders(context);
    testModelerAcDsDataIndexOrder(context);
    testModelerAcDsBlobPageChain(context);
    testBoundedFork(context);
    testHeaderSectionBounds(context);
    testHeaderDateTimeEncoding(context);
    testHeaderVersionEnvelopes(context);
    testDwgNodReference(context);
    testHeaderCompatibilityProfile(context);
    testAc1027CommonEntityHandleOrder(context);
    testTrackedHeaderRead(context);
    testHeaderPublicRead(context);
    testHeaderWriters(context);
    testHeaderWriterSealingFailures(context);
    testDwgOperationAdmission(context);
    testVersionDispatch(context);
    testR140RepeatFailsClosed(context);
    testR11UnsupportedMarkersFailClosed(context);
    testR140HeaderBounds(context);
    testR140LoadFailsClosed(context);
    testR140ShapeFailsClosed(context);
    testR140EndrepFailsClosed(context);
    testReadBufferRejection(context);
    testSectionNameMatrix(context);
    testSectionNameUtf16Framing(context);
    testR2007ClassStringFooter(context);
    testTemplateFields(context);
    testTemplatePublicMatrix(context);
    testTemplateFilePublication(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " DWG reader matrix assertion(s) failed\n";
        return 1;
    }
    std::cout << "DWG reader matrix tests: PASS\n";
    return 0;
}
