#include <array>
#include <algorithm>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <limits>
#include <new>
#include <string>
#include <type_traits>
#include <utility>

#include "drw_base.h"
#include "libdxfrw.h"
#include "libdwgr.h"

namespace {
bool countObservationAllocations = false;
std::size_t observationAllocations = 0;
}

// Count only allocations made while extracting already-seeded observations.
// This replacement belongs to this test executable, not to the library.
void* operator new(std::size_t size) {
    if (countObservationAllocations)
        ++observationAllocations;
    if (void* memory = std::malloc(size == 0 ? 1 : size))
        return memory;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

class DwgReadObservationTestAccess {
public:
    static std::unordered_map<std::string, std::size_t>& custom(dwgRW& reader) {
        return reader.m_skippedCustomClasses;
    }
    static std::unordered_map<std::string, std::size_t>& objects(dwgRW& reader) {
        return reader.m_skippedUnsupportedObjects;
    }
    static std::vector<DwgIntegrityDiagnostic>& integrity(dwgRW& reader) {
        return reader.m_integrityDiagnostics;
    }
    static void upstreamDropped(dwgRW& reader, std::size_t count) {
        reader.m_integrityDiagnosticsDropped = count;
    }
    static void scalarsAndFrames(dwgRW& reader) {
        reader.m_entityParseFailures = 1;
        reader.m_objectParseFailures = 2;
        reader.m_classesCrcMismatch = 3;
        reader.m_r2007CrcMismatch = 4;
        reader.m_r2004CrcMismatch = 5;
        reader.m_dataStorageLinkFailures = 6;
        reader.m_dataStorageOrphanRecords = 7;
        reader.m_decodedProxyPrimitives = 8;
        reader.m_readFrameObservations.status =
            DRW_DwgFrameCoverageStatus::FinalizedPartial;
        reader.m_readFrameObservations.sourceFrames = 10;
        reader.m_readFrameObservations.dispositions = {{0, 0, 0, 6, 1, 1, 1}};
        reader.m_readFrameObservations.unrecognizedDispositions = 1;
    }
};

static_assert(std::is_trivially_copyable<DwgReadObservations>::value,
              "DWG observations must remain an owning, allocation-free value");
static_assert(std::is_nothrow_copy_constructible<DwgReadObservations>::value
                  && std::is_nothrow_copy_assignable<DwgReadObservations>::value,
              "DWG observations must be copied without throwing");
static_assert(noexcept(std::declval<const dwgRW&>().getReadObservations()),
              "observation extraction must not throw");
static_assert(DwgReadObservationMaxRows == 16
                  && DwgReadObservationMaxNameBytes == 64,
              "observation reporting limits are part of this contract");

namespace {

struct TestContext {
    int failures {0};

    void expect(bool condition, const char* label) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << label << '\n';
        }
    }
};

bool prefixMatches(const DwgReadBytePrefix& prefix, const std::string& source) {
    const std::size_t length = std::min(source.size(), DwgReadObservationMaxNameBytes);
    if (prefix.length != length || prefix.sourceBytes != source.size()
        || prefix.truncated() != (source.size() > length))
        return false;
    for (std::size_t index = 0; index < length; ++index) {
        if (prefix.bytes[index] != static_cast<std::uint8_t>(source[index]))
            return false;
    }
    return true;
}

bool sameNamedObservations(const DwgReadNamedObservations& left,
                           const DwgReadNamedObservations& right) {
    if (left.retained > left.rows.size() || right.retained > right.rows.size()
        || left.retained != right.retained || left.sourceNames != right.sourceNames
        || left.instances != right.instances || left.omittedNames != right.omittedNames
        || left.omittedInstances != right.omittedInstances
        || left.truncatedRetainedNames != right.truncatedRetainedNames
        || left.omittedNameBytes != right.omittedNameBytes
        || left.aggregateSaturated != right.aggregateSaturated)
        return false;
    for (std::size_t index = 0; index < left.retained; ++index) {
        const auto& a = left.rows[index];
        const auto& b = right.rows[index];
        if (a.instances != b.instances || a.name.length != b.name.length
            || a.name.sourceBytes != b.name.sourceBytes || a.name.bytes != b.name.bytes)
            return false;
    }
    return true;
}

bool emptyNamedObservations(const DwgReadNamedObservations& value) {
    return value.retained == 0 && value.sourceNames == 0 && value.instances == 0
        && value.omittedNames == 0 && value.omittedInstances == 0
        && value.truncatedRetainedNames == 0 && value.omittedNameBytes == 0
        && !value.aggregateSaturated;
}

bool emptyObservations(const DwgReadObservations& value) {
    return value.entityParseFailures == 0 && value.objectParseFailures == 0
        && value.classesCrcMismatch == 0 && value.reader21CrcMismatch == 0
        && value.reader18HeaderCrcMismatch == 0 && value.dataStorageLinkFailures == 0
        && value.dataStorageOrphanRecords == 0 && value.decodedProxyPrimitives == 0
        && emptyNamedObservations(value.skippedCustomClasses)
        && emptyNamedObservations(value.skippedUnsupportedObjects)
        && value.integrity.retained == 0 && value.integrity.sourceRecorded == 0
        && value.integrity.extractionOmitted == 0
        && value.integrity.reportedUpstreamDropped == 0
        && value.integrity.truncatedRetainedNames == 0
        && value.integrity.omittedNameBytes == 0 && !value.integrity.aggregateSaturated
        && value.frames.status == DRW_DwgFrameCoverageStatus::NotAvailable
        && value.frames.sourceFrames == 0 && value.frames.unrecognizedDispositions == 0
        && std::all_of(value.frames.dispositions.begin(), value.frames.dispositions.end(),
                       [](std::size_t count) { return count == 0; });
}

std::string numberedName(std::size_t index) {
    std::string name("key-00");
    name[4] = static_cast<char>('0' + index / 10);
    name[5] = static_cast<char>('0' + index % 10);
    return name;
}

void testNamedObservationBounds(TestContext& t) {
    dwgRW reader(nullptr);
    t.expect(emptyObservations(reader.getReadObservations()),
             "DWG observations are empty before a full read");
    auto& custom = DwgReadObservationTestAccess::custom(reader);
    auto& objects = DwgReadObservationTestAccess::objects(reader);
    for (std::size_t index = 0; index < 16; ++index) {
        custom.emplace(numberedName(index), index + 1);
        objects.emplace(numberedName(index), index + 1);
    }
    const auto exact = reader.getReadObservations();
    t.expect(exact.skippedCustomClasses.retained == 16
                 && exact.skippedCustomClasses.sourceNames == 16
                 && exact.skippedCustomClasses.instances == 136
                 && exact.skippedCustomClasses.omittedNames == 0
                 && exact.skippedCustomClasses.omittedInstances == 0
                 && exact.skippedCustomClasses.omittedNameBytes == 0
                 && !exact.skippedCustomClasses.aggregateSaturated,
             "sixteen skipped names fit without omission");
    t.expect(sameNamedObservations(exact.skippedCustomClasses,
                                  exact.skippedUnsupportedObjects),
             "overlapping skipped maps remain independent observations");

    custom.emplace(numberedName(16), 17);
    objects.emplace(numberedName(16), 17);
    const auto overflow = reader.getReadObservations();
    const auto& names = overflow.skippedCustomClasses;
    t.expect(names.retained == 16 && names.sourceNames == 17 && names.instances == 153
                 && names.omittedNames == 1 && names.omittedInstances == 17
                 && names.truncatedRetainedNames == 0 && names.omittedNameBytes == 6
                 && !names.aggregateSaturated,
             "seventeenth skipped name has explicit name, instance and byte omissions");
    bool lexical = true;
    for (std::size_t index = 0; index < 16; ++index) {
        lexical = lexical && prefixMatches(names.rows[index].name, numberedName(index))
            && names.rows[index].instances == index + 1;
    }
    t.expect(lexical, "skipped names retain the lexical first sixteen full keys");

    dwgRW reverse(nullptr);
    for (std::size_t index = 17; index-- > 0;) {
        DwgReadObservationTestAccess::custom(reverse).emplace(numberedName(index), index + 1);
        DwgReadObservationTestAccess::objects(reverse).emplace(numberedName(index), index + 1);
    }
    const auto reversed = reverse.getReadObservations();
    t.expect(sameNamedObservations(names, reversed.skippedCustomClasses)
                 && sameNamedObservations(overflow.skippedUnsupportedObjects,
                                          reversed.skippedUnsupportedObjects),
             "bounded skipped summaries do not depend on insertion order");
}

void testNamedObservationPrefixes(TestContext& t) {
    dwgRW reader(nullptr);
    auto& names = DwgReadObservationTestAccess::custom(reader);
    const std::string shared(64, 'x');
    for (std::size_t index = 17; index-- > 0;)
        names.emplace(shared + static_cast<char>('a' + index), index + 1);
    const auto tied = reader.getReadObservations().skippedCustomClasses;
    bool fullKeyOrder = true;
    for (std::size_t index = 0; index < 16; ++index) {
        fullKeyOrder = fullKeyOrder
            && prefixMatches(tied.rows[index].name, shared + static_cast<char>('a' + index))
            && tied.rows[index].instances == index + 1;
    }
    t.expect(fullKeyOrder && tied.retained == 16 && tied.sourceNames == 17
                 && tied.truncatedRetainedNames == 16 && tied.omittedNames == 1
                 && tied.omittedInstances == 17 && tied.omittedNameBytes == 81,
             "equal sixty-four-byte prefixes are selected and ordered by full keys");

    names.clear();
    const char byteName[] = {'a', '\0', static_cast<char>(0x80), static_cast<char>(0xff), 'z'};
    const std::array<std::string, 4> keys{{std::string{},
        std::string(byteName, sizeof(byteName)), std::string(64, 'x'), std::string(65, 'y')}};
    for (std::size_t index = 0; index < keys.size(); ++index)
        names.emplace(keys[index], index + 1);
    const auto boundary = reader.getReadObservations().skippedCustomClasses;
    bool preserved = boundary.retained == keys.size();
    for (std::size_t index = 0; index < keys.size(); ++index)
        preserved = preserved && prefixMatches(boundary.rows[index].name, keys[index]);
    t.expect(preserved && boundary.truncatedRetainedNames == 1
                 && boundary.omittedNameBytes == 1 && boundary.omittedNames == 0,
             "empty, sixty-four, sixty-five and embedded binary name bytes are preserved");
}

void testNamedObservationSaturation(TestContext& t) {
    dwgRW reader(nullptr);
    const auto maximum = std::numeric_limits<std::size_t>::max();
    auto& custom = DwgReadObservationTestAccess::custom(reader);
    custom.emplace("a", maximum);
    const auto exact = reader.getReadObservations().skippedCustomClasses;
    t.expect(exact.instances == maximum && !exact.aggregateSaturated,
             "an exact maximum instance count is representable without overflow");
    custom.emplace("b", 1);
    auto& objects = DwgReadObservationTestAccess::objects(reader);
    for (std::size_t index = 0; index < 18; ++index)
        objects.emplace(numberedName(index), index == 16 ? maximum : index == 17 ? 1 : 0);
    const auto saturated = reader.getReadObservations();
    t.expect(saturated.skippedCustomClasses.instances == maximum
                 && saturated.skippedCustomClasses.aggregateSaturated
                 && saturated.skippedCustomClasses.omittedInstances == 0,
             "total instance addition saturates without wrapping");
    t.expect(saturated.skippedUnsupportedObjects.instances == maximum
                 && saturated.skippedUnsupportedObjects.omittedNames == 2
                 && saturated.skippedUnsupportedObjects.omittedInstances == maximum
                 && saturated.skippedUnsupportedObjects.aggregateSaturated,
             "excluded instance addition saturates independently of retained rows");
}

bool integrityMatches(const DwgReadIntegrityObservation& row,
                      const DwgIntegrityDiagnostic& source) {
    return row.version == source.version && row.severity == source.severity
        && row.offsetSpace == source.offsetSpace && row.phase == source.phase
        && row.kind == source.kind && row.logicalSectionId == source.logicalSectionId
        && row.sectionDescriptorId == source.sectionDescriptorId
        && prefixMatches(row.sectionName, source.sectionName)
        && row.pageId == source.pageId && row.fileOffset == source.fileOffset
        && row.logicalHandle == source.logicalHandle && row.expected == source.expected
        && row.observed == source.observed && row.hasPageId == source.hasPageId
        && row.hasFileOffset == source.hasFileOffset
        && row.hasLogicalHandle == source.hasLogicalHandle
        && row.hasExpected == source.hasExpected && row.hasObserved == source.hasObserved;
}

void seedIntegrity(dwgRW& reader) {
    auto& diagnostics = DwgReadObservationTestAccess::integrity(reader);
    for (std::size_t index = 0; index < 17; ++index) {
        DwgIntegrityDiagnostic row;
        row.version = index % 2 == 0 ? DRW::AC1021 : DRW::AC1024;
        row.severity = index % 2 == 0 ? DwgIntegritySeverity::Warning : DwgIntegritySeverity::Error;
        row.offsetSpace = index % 2 == 0 ? DwgIntegrityAddressSpace::DecodedBuffer
                                       : DwgIntegrityAddressSpace::PhysicalFile;
        row.phase = index % 2 == 0 ? DwgIntegrityPhase::DataPage : DwgIntegrityPhase::ObjectFrame;
        row.kind = index % 2 == 0 ? DwgIntegrityCheckKind::DataPageCrc
                                 : DwgIntegrityCheckKind::ObjectFrameBounds;
        row.logicalSectionId = static_cast<std::int32_t>(index) - 1;
        row.sectionDescriptorId = static_cast<std::int32_t>(100 + index);
        row.sectionName.assign(65, static_cast<char>('A' + index));
        row.pageId = index;
        row.fileOffset = index * 100;
        row.logicalHandle = static_cast<std::uint32_t>(index);
        row.expected = index;
        row.observed = index * 2;
        row.hasPageId = index % 2 == 0;
        row.hasFileOffset = index % 3 == 0;
        row.hasLogicalHandle = index % 4 == 0;
        row.hasExpected = index % 5 == 0;
        row.hasObserved = index % 6 == 0;
        diagnostics.push_back(std::move(row));
    }
    DwgReadObservationTestAccess::upstreamDropped(reader, 5);
}

void testIntegrityObservationBounds(TestContext& t) {
    dwgRW reader(nullptr);
    seedIntegrity(reader);
    const auto overflow = reader.getReadObservations().integrity;
    const auto& source = DwgReadObservationTestAccess::integrity(reader);
    bool projection = overflow.retained == 16;
    for (std::size_t index = 0; index < 16; ++index)
        projection = projection && integrityMatches(overflow.rows[index], source[index]);
    t.expect(projection && overflow.sourceRecorded == 17 && overflow.extractionOmitted == 1
                 && overflow.reportedUpstreamDropped == 5
                 && overflow.truncatedRetainedNames == 16 && overflow.omittedNameBytes == 81
                 && !overflow.aggregateSaturated,
             "integrity preserves encounter order, fields and independent dropped counts");
    t.expect(overflow.rows[0].pageId == 0 && overflow.rows[0].hasPageId
                 && overflow.rows[0].fileOffset == 0 && overflow.rows[0].hasFileOffset
                 && overflow.rows[0].logicalHandle == 0 && overflow.rows[0].hasLogicalHandle
                 && overflow.rows[0].expected == 0 && overflow.rows[0].hasExpected
                 && overflow.rows[0].observed == 0 && overflow.rows[0].hasObserved,
             "zero-valued integrity locations and values retain presence flags");
    DwgReadObservationTestAccess::integrity(reader).pop_back();
    const auto exact = reader.getReadObservations().integrity;
    t.expect(exact.retained == 16 && exact.sourceRecorded == 16 && exact.extractionOmitted == 0
                 && exact.reportedUpstreamDropped == 5 && exact.omittedNameBytes == 16,
             "sixteen integrity rows fit while retained suffix bytes remain explicit");
    DwgReadObservationTestAccess::upstreamDropped(reader,
                                                 std::numeric_limits<std::size_t>::max());
    t.expect(reader.getReadObservations().integrity.reportedUpstreamDropped
                 == std::numeric_limits<std::size_t>::max(),
             "maximum upstream dropped count is copied without combining populations");

    auto& diagnostics = DwgReadObservationTestAccess::integrity(reader);
    diagnostics.resize(1);
    const char bytes[] = {'\0', static_cast<char>(0x80), static_cast<char>(0xff)};
    const std::array<std::string, 4> names{{std::string{}, std::string(64, 's'),
        std::string(65, 's'), std::string(bytes, sizeof(bytes))}};
    for (const auto& name : names) {
        diagnostics[0].sectionName = name;
        const auto value = reader.getReadObservations().integrity;
        t.expect(value.retained == 1 && prefixMatches(value.rows[0].sectionName, name)
                     && value.omittedNameBytes == (name.size() > 64 ? 1u : 0u),
                 "integrity section prefixes preserve byte boundaries and binary bytes");
    }
}

void testObservationAllocationAndReset(TestContext& t) {
    dwgRW reader(nullptr);
    for (std::size_t index = 0; index < 17; ++index) {
        const std::string name = std::string(65, 'n') + numberedName(index);
        DwgReadObservationTestAccess::custom(reader).emplace(name, index + 1);
        DwgReadObservationTestAccess::objects(reader).emplace(name, index + 1);
    }
    seedIntegrity(reader);
    DwgReadObservationTestAccess::scalarsAndFrames(reader);
    observationAllocations = 0;
    countObservationAllocations = true;
    const DwgReadObservations snapshot = reader.getReadObservations();
    countObservationAllocations = false;
    t.expect(observationAllocations == 0,
             "bounded observation extraction allocates no memory after source seeding");
    t.expect(snapshot.entityParseFailures == 1 && snapshot.objectParseFailures == 2
                 && snapshot.classesCrcMismatch == 3 && snapshot.reader21CrcMismatch == 4
                 && snapshot.reader18HeaderCrcMismatch == 5
                 && snapshot.dataStorageLinkFailures == 6 && snapshot.dataStorageOrphanRecords == 7
                 && snapshot.decodedProxyPrimitives == 8,
             "all read scalar observations retain their own values");
    t.expect(snapshot.frames.status == DRW_DwgFrameCoverageStatus::FinalizedPartial
                 && snapshot.frames.sourceFrames == 10
                 && snapshot.frames.dispositions == std::array<std::size_t, 7>{{0, 0, 0, 6, 1, 1, 1}}
                 && snapshot.frames.unrecognizedDispositions == 1,
             "captured frame aggregates preserve availability and every disposition");
    t.expect(DwgReadObservationTestAccess::custom(reader).size() == 17
                 && DwgReadObservationTestAccess::objects(reader).size() == 17
                 && DwgReadObservationTestAccess::integrity(reader).size() == 17
                 && DwgReadObservationTestAccess::integrity(reader)[0].sectionName.size() == 65,
             "bounded extraction leaves full source observations intact");
    t.expect(!reader.read(nullptr, false) && emptyObservations(reader.getReadObservations()),
             "invalid read resets scalar, map, integrity and frame observations");
    t.expect(snapshot.entityParseFailures == 1 && snapshot.skippedCustomClasses.sourceNames == 17
                 && snapshot.integrity.sourceRecorded == 17,
             "a copied observation snapshot survives the next read reset");
    DwgReadObservationTestAccess::scalarsAndFrames(reader);
    t.expect(!reader.readBuffer(nullptr, 0, nullptr, false)
                 && emptyObservations(reader.getReadObservations()),
             "invalid buffer read also starts a fresh observation snapshot");
}

void expectDiagnostic(TestContext& t, const DRW_OperationDiagnostic& d,
                      DRW::OperationKind operation,
                      DRW::OperationPhase phase,
                      DRW::OperationCause cause, const char* code,
                      const char* label) {
    t.expect(d.operation == operation && d.phase == phase
                 && d.cause == cause && d.code == code && d.failed(),
             label);
    t.expect(d.secondary.size() <= DRW_OperationDiagnostic::MaxSecondaryEntries,
             "diagnostic secondary list is bounded");
}

void testDxfDiagnostics(TestContext& t) {
    dxfRW writer(nullptr);
    const DRW_OperationDiagnostic initial = writer.getLastDiagnostic();
    t.expect(initial.operation == DRW::OperationKind::None
                 && !initial.failed(),
             "DXF diagnostic is empty before an operation");
    t.expect(!writer.write(nullptr, DRW::UNKNOWNV, false)
                 && writer.getError() == DRW::BAD_VERSION,
             "DXF unsupported version keeps legacy error");
    expectDiagnostic(t, writer.getLastDiagnostic(), DRW::OperationKind::Write,
                     DRW::OperationPhase::Validation,
                     DRW::OperationCause::UnsupportedVersion,
                     "unsupported-version",
                     "DXF unsupported version has structured cause");

    t.expect(!writer.write(nullptr, DRW::AC1015, false)
                 && writer.getError() == DRW::BAD_UNKNOWN,
             "DXF null interface keeps legacy invalid-argument error");
    expectDiagnostic(t, writer.getLastDiagnostic(), DRW::OperationKind::Write,
                     DRW::OperationPhase::Argument,
                     DRW::OperationCause::InvalidArgument,
                     "invalid-argument",
                     "DXF null interface has structured cause");

    std::string empty;
    t.expect(!writer.readAscii(nullptr, false, empty)
                 && writer.getError() == DRW::BAD_UNKNOWN,
             "DXF null read interface keeps legacy error");
    expectDiagnostic(t, writer.getLastDiagnostic(), DRW::OperationKind::Read,
                     DRW::OperationPhase::Argument,
                     DRW::OperationCause::InvalidArgument,
                     "invalid-argument",
                     "DXF null read interface has structured cause");
}

void testDwgDiagnostics(TestContext& t) {
    dwgRW writer(nullptr);
    const DRW_OperationDiagnostic initial = writer.getLastDiagnostic();
    t.expect(initial.operation == DRW::OperationKind::None
                 && !initial.failed(),
             "DWG diagnostic is empty before an operation");
    t.expect(!writer.write(nullptr, DRW::UNKNOWNV, false)
                 && writer.getError() == DRW::BAD_VERSION,
             "DWG unsupported version keeps legacy error");
    expectDiagnostic(t, writer.getLastDiagnostic(), DRW::OperationKind::Write,
                     DRW::OperationPhase::Validation,
                     DRW::OperationCause::UnsupportedVersion,
                     "unsupported-version",
                     "DWG unsupported version has structured cause");

    t.expect(!writer.write(nullptr, DRW::AC1015, false)
                 && writer.getError() == DRW::BAD_UNKNOWN,
             "DWG null write interface keeps legacy error");
    expectDiagnostic(t, writer.getLastDiagnostic(), DRW::OperationKind::Write,
                     DRW::OperationPhase::Argument,
                     DRW::OperationCause::InvalidArgument,
                     "invalid-argument",
                     "DWG null write interface has structured cause");

    t.expect(!writer.read(nullptr, false)
                 && writer.getError() == DRW::BAD_UNKNOWN,
             "DWG null read interface keeps legacy error");
    expectDiagnostic(t, writer.getLastDiagnostic(), DRW::OperationKind::Read,
                     DRW::OperationPhase::Argument,
                     DRW::OperationCause::InvalidArgument,
                     "invalid-argument",
                     "DWG null read interface has structured cause");

    const std::array<std::uint8_t, 6> badMagic {{'B', 'A', 'D', '0', '0', '0'}};
    t.expect(!writer.readBuffer(badMagic.data(), badMagic.size(), nullptr, false)
                 && writer.getError() == DRW::BAD_UNKNOWN,
             "DWG null buffer consumer keeps legacy error");
    expectDiagnostic(t, writer.getLastDiagnostic(), DRW::OperationKind::Read,
                     DRW::OperationPhase::Argument,
                     DRW::OperationCause::InvalidArgument,
                     "invalid-argument",
                     "DWG null buffer consumer has structured cause");

    dwgRW missingFile("");
    t.expect(!missingFile.testReader()
                 && missingFile.getError() == DRW::BAD_OPEN,
             "DWG missing file keeps legacy open error");
    expectDiagnostic(t, missingFile.getLastDiagnostic(),
                     DRW::OperationKind::Read, DRW::OperationPhase::Open,
                     DRW::OperationCause::OpenFailure, "open-failure",
                     "DWG missing file has stage-aware open cause");
}

} // namespace

int main() {
    TestContext context;
    testDxfDiagnostics(context);
    testDwgDiagnostics(context);
    testNamedObservationBounds(context);
    testNamedObservationPrefixes(context);
    testNamedObservationSaturation(context);
    testIntegrityObservationBounds(context);
    testObservationAllocationAndReset(context);
    if (context.failures != 0) {
        std::cerr << context.failures << " diagnostic assertion(s) failed\n";
        return 1;
    }
    std::cout << "Operation diagnostics: PASS\n";
    return 0;
}
