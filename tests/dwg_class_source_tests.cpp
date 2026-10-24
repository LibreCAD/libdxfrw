// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_iface.h"
#include "dx_dwg_class_section_wire.h"
#include "intern/dwgreader21.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <new>
#include <type_traits>

// Deterministic allocation failures, scoped strictly around the operation under
// test. No drawing, stream, reader or assertion is constructed under this seam.
namespace {
long allocationsBeforeFailure = -1;
struct AllocationFailure {
    explicit AllocationFailure(long count) { allocationsBeforeFailure = count; }
    ~AllocationFailure() { allocationsBeforeFailure = -1; }
};
}
void* operator new(std::size_t size) {
    if (allocationsBeforeFailure == 0) throw std::bad_alloc();
    if (allocationsBeforeFailure > 0) --allocationsBeforeFailure;
    if (void* value = std::malloc(size == 0u ? 1u : size)) return value;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete(void* value, std::size_t) noexcept { std::free(value); }
void operator delete[](void* value, std::size_t) noexcept { std::free(value); }

class DxIfaceDwgClassSourceTestAccess {
public:
    // Contract seam only. Native tests below must enter through fileImport.
    static void open(dx_iface& adapter, dx_data& data, std::uint64_t generation) {
        adapter.cData = &data;
        data.importGeneration = generation; data.sourceFormat = dx_source_format::DWG;
        data.dwgReadOutcome.attempted = true;
        ++data.m_classEvidenceAttempt; data.m_classEvidenceGeneration = generation;
        data.m_classEvidenceHighWater = generation; data.m_classEvidenceFallback = true;
        data.m_classEvidenceFailure = {};
        data.m_classEvidenceFailure.attempt = data.m_classEvidenceAttempt;
        data.m_classEvidenceFailure.generation = generation;
        data.m_classEvidenceFailure.state = dx_class_evidence::State::NotDwg;
        adapter.beginDwgClassEvidenceRead(data);
    }
    static void rows(dx_iface& adapter, const DRW_DwgClassCoverageReport& report) {
        adapter.retainDwgClassEvidenceRows(report);
    }
    static void source(dx_iface& adapter, const DRW_DwgClassSourceReport& report) {
        adapter.retainDwgClassEvidenceSource(report);
    }
    static void finish(dx_iface& adapter, const DRW_DwgClassSourceReport& source, bool success = true) {
        adapter.finishDwgClassEvidenceRead(success, source.m_version, source.m_codePageId,
            source.m_codePagePresent, source.m_codePageRecognized);
    }
    static void evidenceLimits(dx_data& data, std::size_t bytes, std::size_t count = 1024u) {
        data.m_classEvidenceByteLimit = bytes; data.m_classEvidenceCountLimit = count;
    }
    static std::size_t evidenceBytes(const dx_data& data) { return data.m_classEvidenceBytes; }
    static std::size_t evidenceCount(const dx_data& data) { return data.m_classEvidenceImports.size(); }
    static dx_class_evidence::State historyState(const dx_data& data, std::uint64_t generation) {
        const auto found = data.m_classEvidenceImports.find(generation);
        return found == data.m_classEvidenceImports.end() ? dx_class_evidence::State::NoAttempt : found->second.result.state;
    }
    static std::weak_ptr<const dx_class_evidence::Rows> owner(const dx_data& data) {
        const auto found = data.m_classEvidenceImports.find(data.m_classEvidenceGeneration);
        return found == data.m_classEvidenceImports.end() ? std::weak_ptr<const dx_class_evidence::Rows>{} : found->second.rows;
    }
    static void limits(dx_data& data, std::size_t bytes, std::size_t reports = 1024u) {
        data.m_classSourceByteLimit = bytes;
        data.m_classSourceReportLimit = reports;
    }
    static std::size_t retained(const dx_data& data) { return data.m_classSourceRetainedBytes; }
    static std::size_t reports(const dx_data& data) { return data.m_classSourceReports.size(); }
    static void nameLimits(dx_data& data, std::size_t bytes, std::size_t count = 1024u) {
        data.m_rasterDwgNameByteLimit = bytes; data.m_rasterDwgNameImportLimit = count;
    }
    static std::size_t nameBytes(const dx_data& data) { return data.m_rasterDwgNameBytes; }
    static std::size_t nameCount(const dx_data& data) { return data.m_rasterDwgNameImports.size(); }
    static void name(dx_iface& adapter, const DRW_AppId& row, const std::string& value) {
        adapter.retainRasterDwgName(row.handle, value, row.dwgNameSource(), false);
    }
};

namespace {
using Status = DRW_DwgClassSourceStatus;
using Report = DRW_DwgClassSourceReport;
using Space = DRW_DwgFrameOffsetSpace;
std::size_t checks = 0u;
bool expect(bool value, int line) {
    ++checks;
    if (!value) std::cerr << "class source check failed at line " << line << '\n';
    return value;
}
#define CHECK(value) do { if (!expect((value), __LINE__)) return false; } while (false)

class Sink : public DRW_DwgClassSourceHandler {
public:
    bool request = true, throwRequest = false, throwDelivery = false;
    mutable unsigned requests = 0u;
    unsigned calls = 0u;
    Report report;
    bool wantsDwgClassSource() const override {
        ++requests;
        if (throwRequest) throw 1;
        return request;
    }
    void addDwgClassSourceReport(const Report& value) override {
        ++calls;
        if (throwDelivery) throw 1;
        report = value;
    }
};
class Probe : public dwgReader21 {
public:
    Probe() : dwgReader21(std::make_unique<dwgBuffer>(nullptr, 0u), nullptr) {
        version = DRW::AC1027;
        appMaintenanceVersion = 7u;
        m_sourceCodePage.present = true;
        m_sourceCodePage.rawId = 30u;
        m_sourceCodePage.recognized = true;
    }
    void start(DRW_DwgClassSourceHandler* sink) {
        prepareDwgClassSource(sink);
        beginDwgClassCoverage();
    }
    using dwgReader::captureDwgClassSource;
    using dwgReader::capturePhysicalDwgClassSource;
    using dwgReader::finalizeDwgClassSourceNoThrow;
    using dwgReader::readDwgClassesTail;
    void cap(std::size_t value) { m_dwgClassSourceByteLimit = value; }
};

bool captureContracts() {
    std::vector<std::uint8_t> bytes(16u, 0xA5u);
    Probe reader;
    Sink sink;
    DRW_DwgClassSourceHandler defaultSink;
    reader.start(nullptr);
    { AllocationFailure failure(0); reader.captureDwgClassSource(bytes.data(), bytes.size(), 3, 16u); }
    CHECK(reader.m_dwgClassSourceReport.m_status == Status::NotRequested);
    reader.start(&defaultSink);
    { AllocationFailure failure(0); reader.captureDwgClassSource(bytes.data(), bytes.size(), 3, 16u); }
    CHECK(reader.m_dwgClassSourceReport.m_status == Status::NotRequested);
    CHECK(!reader.m_dwgClassSourceReport.m_bytes);
    sink.throwRequest = true;
    reader.start(&sink);
    reader.captureDwgClassSource(bytes.data(), bytes.size(), 3, 16u);
    reader.finalizeDwgClassSourceNoThrow(false);
    CHECK(sink.report.m_status == Status::RequestFailed && !sink.report.m_bytes);
    sink.throwRequest = false;
    reader.start(&sink);
    reader.finalizeDwgClassSourceNoThrow(false);
    CHECK(sink.report.m_status == Status::NotCaptured && !sink.report.m_classReadCompleted);

    reader.cap(16u);
    reader.start(&sink);
    reader.captureDwgClassSource(bytes.data(), bytes.size(), 3, 15u);
    reader.finalizeDwgClassSourceNoThrow(false);
    CHECK(sink.report.m_status == Status::Captured && *sink.report.m_bytes == bytes);
    CHECK(sink.report.m_declaredSectionSize == 15u && sink.report.m_byteCount == 16u);
    CHECK(sink.report.m_version == DRW::AC1027 && sink.report.m_appMaintenanceVersion == 7u
        && sink.report.m_codePageId == 30u && sink.report.m_codePagePresent
        && sink.report.m_codePageRecognized && !sink.report.m_classReadCompleted
        && !sink.report.m_parserCoverageComplete && sink.report.m_baseByteOffset == 0u);
    const auto frozen = sink.report.m_bytes;
    bytes[0] = 1u;
    reader.captureDwgClassSource(bytes.data(), bytes.size(), 4, 16u);
    const auto calls = sink.calls;
    reader.finalizeDwgClassSourceNoThrow(true);
    CHECK(sink.calls == calls && (*frozen)[0] == 0xA5u);
    for (const auto size : {17u, 0u}) {
        reader.start(&sink);
        { AllocationFailure failure(0); reader.captureDwgClassSource(bytes.data(), size, 3, 16u); }
        reader.finalizeDwgClassSourceNoThrow(false);
        CHECK(sink.report.m_status == (size == 17u ? Status::TooLarge : Status::InvalidRange));
        CHECK(!sink.report.m_bytes);
    }
    reader.start(&sink);
    { AllocationFailure failure(0); reader.captureDwgClassSource(nullptr, 16u, 3, 16u); }
    reader.finalizeDwgClassSourceNoThrow(false);
    CHECK(sink.report.m_status == Status::InvalidRange);
    reader.cap(0u);
    reader.start(&sink);
    reader.captureDwgClassSource(bytes.data(), 1u, 3, 1u);
    CHECK(reader.m_dwgClassSourceReport.m_status == Status::TooLarge);
    reader.cap(16u);

    unsigned allocationFailures = 0u, successes = 0u;
    for (long count = 0; count < 7; ++count) {
        reader.start(&sink);
        { AllocationFailure failure(count); reader.captureDwgClassSource(bytes.data(), 16u, 3, 16u); }
        reader.finalizeDwgClassSourceNoThrow(true);
        if (sink.report.m_status == Status::CaptureFailed) {
            ++allocationFailures;
            CHECK(!sink.report.m_bytes);
        } else {
            ++successes;
            CHECK(sink.report.m_status == Status::Captured && *sink.report.m_bytes == bytes);
        }
    }
    CHECK(allocationFailures >= 2u && successes != 0u);
    sink.throwDelivery = true;
    reader.start(&sink);
    reader.captureDwgClassSource(bytes.data(), 16u, 3, 16u);
    reader.finalizeDwgClassSourceNoThrow(true);
    CHECK(reader.m_dwgClassSourceReport.m_status == Status::DeliveryFailed
        && !reader.m_dwgClassSourceReport.m_bytes);
    sink.throwDelivery = false;
    reader.start(&sink);
    reader.captureDwgClassSource(bytes.data(), 16u, 3, 16u);
    reader.finalizeDwgClassSourceNoThrow(true);
    CHECK(sink.report.m_status == Status::Captured);
    return true;
}
bool readerTails() {
    const std::vector<std::uint8_t> sentinel{
        0x72,0x5E,0x3B,0x47,0x3B,0x56,0x07,0x3A,0x3F,0x23,0x0B,0xA0,0x18,0x30,0x49,0x75};
    Probe reader;
    for (auto size : {0u,1u,2u,8u}) {
        std::vector<std::uint8_t> bytes(size, 0u);
        bytes.insert(bytes.end(), sentinel.begin(), sentinel.end());
        dwgBuffer buffer(bytes.data(), bytes.size());
        CHECK(reader.readDwgClassesTail(buffer) && buffer.isGood() && buffer.getPosition() == size);
    }
    for (unsigned size = 0u; size < 16u; ++size) {
        dwgBuffer buffer(sentinel.data(), size);
        CHECK(!reader.readDwgClassesTail(buffer) && buffer.isGood() && buffer.getPosition() == 0u);
    }
    const std::vector<std::uint8_t> malformed(24u, 0u);
    dwgBuffer buffer(malformed.data(), malformed.size());
    CHECK(reader.readDwgClassesTail(buffer) && buffer.isGood() && buffer.getPosition() == 0u);
    return true;
}

bool physicalCursor(dwgBuffer& buffer, dwgBuffer& control, const std::vector<std::uint8_t>& bytes) {
    CHECK(buffer.setPosition(2u) && control.setPosition(2u));
    CHECK(buffer.get3Bits() == control.get3Bits());
    const auto position = buffer.getPosition();
    const auto bit = buffer.getBitPos();
    Probe reader;
    Sink sink;
    reader.start(&sink);
    reader.capturePhysicalDwgClassSource(buffer, 5u, 8u, 1);
    reader.finalizeDwgClassSourceNoThrow(true);
    CHECK(sink.report.m_status == Status::Captured && sink.report.m_baseByteOffset == 5u
        && sink.report.m_offsetSpace == Space::PhysicalFile && sink.report.m_byteCount == 8u);
    CHECK(*sink.report.m_bytes == std::vector<std::uint8_t>(bytes.begin() + 5, bytes.begin() + 13));
    CHECK(buffer.isGood() && buffer.getPosition() == position && buffer.getBitPos() == bit);
    CHECK(buffer.getRawLong32() == control.getRawLong32() && buffer.isGood());
    reader.start(&sink);
    { AllocationFailure failure(0); reader.capturePhysicalDwgClassSource(buffer, bytes.size(), 1u, 1); }
    CHECK(reader.m_dwgClassSourceReport.m_status == Status::InvalidRange && buffer.isGood());
    unsigned failures = 0u, successes = 0u;
    for (long count = 0; count < 9; ++count) {
        reader.start(&sink);
        { AllocationFailure failure(count); reader.capturePhysicalDwgClassSource(buffer, 5u, 8u, 1); }
        reader.finalizeDwgClassSourceNoThrow(true);
        if (sink.report.m_status == Status::CaptureFailed) ++failures;
        else { ++successes; CHECK(sink.report.m_status == Status::Captured); }
        CHECK(buffer.isGood());
    }
    CHECK(failures >= 4u && successes != 0u);
    return true;
}

Report sample() {
    Report report;
    report.m_status = Status::Captured;
    report.m_version = DRW::AC1027;
    report.m_sectionDescriptorId = 3;
    report.m_offsetSpace = Space::DecodedBuffer;
    report.m_byteCount = report.m_declaredSectionSize = 4u;
    report.m_bytes = std::make_shared<const std::vector<std::uint8_t>>(4u, 42u);
    return report;
}
void context(dx_iface& adapter, dx_data& data, std::uint64_t generation = 1u) {
    adapter.cData = &data;
    data.sourceFormat = dx_source_format::DWG;
    data.dwgReadOutcome.attempted = true;
    data.importGeneration = generation;
}
bool adapterContracts() {
    dx_iface adapter;
    dx_data data;
    context(adapter, data);
    const auto source = sample();
    adapter.addDwgClassSourceReport(source);
    CHECK(data.dwgClassSource() && data.dwgClassSource()->m_status == Status::Captured);
    CHECK(data.dwgClassSource()->m_bytes == source.m_bytes);
    data.dwgClassCoverage.m_entries.clear();
    CHECK(data.dwgClassSource()->m_bytes == source.m_bytes);
    const auto cost = DxIfaceDwgClassSourceTestAccess::retained(data);
    CHECK(cost > source.m_byteCount);
    adapter.addDwgClassSourceReport(source);
    CHECK(data.dwgClassSource()->m_status == Status::CaptureFailed);
    CHECK(DxIfaceDwgClassSourceTestAccess::reports(data) == 1u
        && DxIfaceDwgClassSourceTestAccess::retained(data) == cost);
    context(adapter, data, 2u);
    CHECK(data.dwgClassSource() == nullptr);
    adapter.addDwgClassSourceReport(source);
    CHECK(data.dwgClassSource()->m_status == Status::Captured);
    for (const auto budget : {cost - 1u, cost}) {
        dx_data bounded;
        context(adapter, bounded);
        DxIfaceDwgClassSourceTestAccess::limits(bounded, budget);
        adapter.addDwgClassSourceReport(source);
        CHECK(bounded.dwgClassSource()->m_status == (budget == cost ? Status::Captured : Status::TooLarge));
        CHECK(DxIfaceDwgClassSourceTestAccess::retained(bounded) == (budget == cost ? cost : 0u));
    }
    dx_data capped;
    context(adapter, capped);
    DxIfaceDwgClassSourceTestAccess::limits(capped, 4096u, 0u);
    adapter.addDwgClassSourceReport(source);
    CHECK(capped.dwgClassSource()->m_status == Status::TooLarge);
    dx_data failed;
    context(adapter, failed);
    { AllocationFailure failure(0); adapter.addDwgClassSourceReport(source); }
    CHECK(failed.dwgClassSource()->m_status == Status::CaptureFailed
        && DxIfaceDwgClassSourceTestAccess::retained(failed) == 0u);
    context(adapter, failed, 2u);
    adapter.addDwgClassSourceReport(source);
    CHECK(failed.dwgClassSource()->m_status == Status::Captured);
    for (unsigned mutation = 0; mutation < 7u; ++mutation) {
        dx_data invalid;
        context(adapter, invalid);
        auto report = source;
        switch (mutation) {
        case 0: report.m_status = static_cast<Status>(255u); break;
        case 1: report.m_bytes.reset(); break;
        case 2: ++report.m_byteCount; break;
        case 3: report.m_status = Status::NotCaptured; break;
        case 4: report.m_baseByteOffset = 1u; break;
        case 5: report.m_offsetSpace = Space::Unknown; break;
        case 6: report.m_offsetSpace = Space::PhysicalFile;
                report.m_baseByteOffset = (std::numeric_limits<std::uint64_t>::max)(); break;
        }
        adapter.addDwgClassSourceReport(report);
        CHECK(invalid.dwgClassSource()->m_status == Status::CaptureFailed
            && DxIfaceDwgClassSourceTestAccess::retained(invalid) == 0u);
    }
    return true;
}

class NoCaptureAdapter : public dx_iface {
    bool wantsDwgClassSource() const override { return false; }
};
// Diagnostic/test correlation only: the public class rows below are not a
// frozen import baseline and cannot authorize production support admission.
bool nativeFraming(const Report& source, const DRW_DwgClassCoverageReport& rows, const char* name) {
    namespace wire = dx_class_wire;
    wire::SectionProfile profile;
    switch (source.m_version) {
    case DRW::AC1015: profile.revision = 2000u; break;
    case DRW::AC1018: profile.revision = 2004u; break;
    case DRW::AC1021: profile.revision = 2007u; break;
    case DRW::AC1027: profile.revision = 2013u; break;
    default: return false;
    }
    profile.maintenance = source.m_appMaintenanceVersion;
    profile.requireTrueHeaderFlag = false; // Explicit false-bit source profile, not ODA10.2 qualification.
    profile.scalarByteAlignment = true;
    profile.zeroEnvelopePadding = source.m_offsetSpace == Space::DecodedBuffer;
    const bool high = profile.revision >= 2010u && profile.maintenance > 3u;
    unsigned observed = 0u;
    for (auto trailer : {wire::Trailer::OdaBitShort, wire::Trailer::ProducerBitLong})
    for (unsigned size = 0; size < (high ? 2u : 1u); ++size)
    for (auto tail : {wire::ClassTail::SentinelOnly, wire::ClassTail::BeforeOne, wire::ClassTail::BeforeTwo,
                     wire::ClassTail::BeforeEight, wire::ClassTail::AfterEight}) {
        if (profile.revision < 2004u && trailer == wire::Trailer::ProducerBitLong) continue;
        if (profile.revision < 2004u && tail != wire::ClassTail::SentinelOnly) continue;
        profile.trailer = trailer; profile.tail = tail;
        profile.sizeLayout = size ? wire::ClassSizeLayout::ExcludesHighWord : wire::ClassSizeLayout::IncludesHighWord;
        wire::SectionProof proof;
        { AllocationFailure failure(0);
          proof = wire::observeSection(source.m_bytes->data(), source.m_bytes->size(), profile); }
        std::cout << name << " independent candidate: trailer=" << static_cast<unsigned>(trailer)
                  << " size=" << size << " tail=" << static_cast<unsigned>(tail)
                  << " status=" << static_cast<unsigned>(proof.status)
                  << " reason=" << static_cast<unsigned>(proof.reason) << " records=" << proof.records
                  << " headerFlag=" << proof.headerFlag
                  << " cursors=" << proof.dataCursor << '/' << proof.data.end << ','
                  << proof.stringCursor << '/' << proof.strings.end
                  << " crc=" << proof.storedCrc << '/' << proof.calculatedCrc << '\n';
        if (proof.status != wire::Status::Observed) continue;
        ++observed;
        CHECK(proof.crcChecked && proof.crcMatches && proof.records == rows.m_entries.size());
        const auto expectedTail = source.m_version == DRW::AC1027
            ? wire::ClassTail::BeforeOne : wire::ClassTail::SentinelOnly;
        CHECK(tail == expectedTail && (source.m_version == DRW::AC1015 || !proof.headerFlag));
        CHECK(!proof.split || proof.scalarPadding.end - proof.scalarPadding.begin == 5u);
        auto data = proof.data, strings = proof.strings;
        for (std::size_t ordinal = 0; ordinal < rows.m_entries.size(); ++ordinal) {
            const auto& row = rows.m_entries[ordinal];
            const auto record = wire::scan(data, strings, profile.revision, profile.trailer);
            CHECK(record.status == wire::Status::Observed && row.m_streamOrdinal == ordinal
                && row.m_sectionDescriptorId == source.m_sectionDescriptorId
                && row.m_state == DRW_DwgClassCoverageState::Published
                && row.m_reason == DRW_DwgClassCoverageReason::NoReason
                && record.number == row.m_classNumber && record.proxy == row.m_proxyFlag
                && record.zombie == (row.m_wasAProxyFlag != 0) && record.itemClass == row.m_entityFlagRaw
                && row.m_entityFlag == (record.itemClass == 0x1F2u ? 1 : 0)
                && record.instances == row.m_instanceCount && record.dwgVersion == row.m_dwgVersion
                && record.maintenance == row.m_maintenanceVersion && record.unknown1 == row.m_unknown1
                && record.unknown2 == row.m_unknown2);
            const auto names = record.split ? strings : data;
            const auto matches = [&](const wire::Text& text, const std::string& expected) {
                // These tracked native fixtures encode exactly one trailing
                // NUL in each name. This explicit convention is not a codec,
                // arbitrary trim or universal DWG text-profile selection.
                return wire::equalsTerminatedUtf8(names, text, record.split, expected.data(), expected.size());
            };
            const bool namesMatch = matches(record.text[0], row.m_appName)
                && matches(record.text[1], row.m_className) && matches(record.text[2], row.m_recordName);
            CHECK(namesMatch);
            const auto base = source.m_baseByteOffset * 8u;
            CHECK(row.m_dataRange.m_present && row.m_stringRange.m_present
                && row.m_dataRange.m_offsetSpace == source.m_offsetSpace
                && row.m_stringRange.m_offsetSpace == source.m_offsetSpace
                && row.m_dataRange.m_sectionRelative == (source.m_offsetSpace == Space::DecodedBuffer)
                && row.m_stringRange.m_sectionRelative == (source.m_offsetSpace == Space::DecodedBuffer)
                && row.m_dataRange.m_startBit == base + data.begin
                && row.m_dataRange.m_endBit == base + record.dataEnd
                && row.m_stringRange.m_startBit == base + (record.split ? strings.begin : data.begin)
                && row.m_stringRange.m_endBit == base + record.stringEnd);
            data.begin = record.dataEnd;
            if (record.split) strings.begin = record.stringEnd;
        }
        CHECK(data.begin == data.end && (!proof.split || strings.begin == strings.end));
    }
    CHECK(observed == (profile.revision < 2004u ? 1u : 2u));
    std::cout << name << ": " << observed << " explicit framing candidates; schema not selected\n";
    return true;
}
bool native(const std::filesystem::path& root) {
    for (const char* name : {"ordinary_enc_AC1015.dwg", "ordinary_enc_AC1018.dwg",
                            "ordinary_enc_AC1021.dwg", "ordinary_enc_AC1027.dwg"}) {
        const auto path = root / name;
        dx_data captured, ordinary;
        dx_iface adapter;
        NoCaptureAdapter baseline;
        const bool result = adapter.fileImport(path.string(), &captured, false);
        const bool other = baseline.fileImport(path.string(), &ordinary, false);
        CHECK(result && result == other && captured.dwgReadOutcome.error == ordinary.dwgReadOutcome.error);
        const auto* report = captured.dwgClassSource();
        const auto* ignored = ordinary.dwgClassSource();
        CHECK(report && report->m_status == Status::Captured && report->m_bytes
            && report->m_byteCount == report->m_bytes->size() && report->m_byteCount != 0u
            && report->m_classReadCompleted && report->m_parserCoverageComplete
            && report->m_version == captured.dwgReadOutcome.sourceVersion
            && report->m_codePageId == captured.dwgReadOutcome.sourceCodePageId
            && report->m_codePagePresent == captured.dwgReadOutcome.sourceCodePageFieldPresent
            && report->m_codePageRecognized == captured.dwgReadOutcome.sourceCodePageRecognized);
        CHECK(ignored && ignored->m_status == Status::NotRequested && !ignored->m_bytes);
        CHECK(captured.dwgClassEvidence().state == dx_class_evidence::State::Joined
            && captured.dwgClassEvidence().rows == captured.dwgClassCoverage.m_entries.size()
            && captured.dwgClassEvidence().candidateMask == captured.dwgClassEvidence().matchedMask
            && captured.hasUnchangedDwgClassEvidence());
        CHECK(ordinary.dwgClassEvidence().state == dx_class_evidence::State::SourceUnavailable
            && !ordinary.hasUnchangedDwgClassEvidence());
        CHECK(captured.dwgClassCoverage.m_entries.size() == ordinary.dwgClassCoverage.m_entries.size());
        for (std::size_t i = 0; i < captured.dwgClassCoverage.m_entries.size(); ++i) {
            const auto& left = captured.dwgClassCoverage.m_entries[i];
            const auto& right = ordinary.dwgClassCoverage.m_entries[i];
            CHECK(left.m_classNumber == right.m_classNumber && left.m_recordName == right.m_recordName
                && left.m_className == right.m_className && left.m_appName == right.m_appName);
        }
        std::ifstream input(path, std::ios::binary);
        const std::vector<std::uint8_t> file{std::istreambuf_iterator<char>(input), {}};
        CHECK(file.size() > 0x12u && report->m_appMaintenanceVersion == file[0x12u]);
        if (report->m_version == DRW::AC1015) {
            CHECK(report->m_offsetSpace == Space::PhysicalFile
                && report->m_baseByteOffset < file.size()
                && report->m_byteCount <= file.size() - report->m_baseByteOffset);
            CHECK(std::equal(report->m_bytes->begin(), report->m_bytes->end(),
                file.begin() + static_cast<std::ptrdiff_t>(report->m_baseByteOffset)));
        } else CHECK(report->m_offsetSpace == Space::DecodedBuffer && report->m_baseByteOffset == 0u);
        CHECK(nativeFraming(*report, captured.dwgClassCoverage, name));
        const auto lease = report->m_bytes;
        captured.dwgClassCoverage = {};
        CHECK(captured.dwgClassSource()->m_bytes == lease && !lease->empty());
        CHECK(!captured.hasUnchangedDwgClassEvidence());
        const auto generation = captured.importGeneration;
        CHECK(adapter.fileImport(path.string(), &captured, false));
        CHECK(captured.importGeneration == generation + 1u && captured.dwgClassSource()
            && captured.dwgClassSource()->m_status == Status::Captured
            && captured.dwgClassSource()->m_bytes != lease
            && *captured.dwgClassSource()->m_bytes == *lease);
        CHECK(captured.hasUnchangedDwgClassEvidence() && captured.dwgClassEvidence().attempt == 2u);
        std::cout << name << ": source capture/no-copy parity PASS\n";
    }
    return true;
}

using EvidenceAccess = DxIfaceDwgClassSourceTestAccess;
using EvidenceState = dx_class_evidence::State;
void evidencePublic(dx_data& data, const Report& source, const DRW_DwgClassCoverageReport& rows) {
    data.sourceVersion = source.m_version;
    data.dwgReadOutcome.attempted = data.dwgReadOutcome.operationSucceeded = true;
    data.dwgReadOutcome.error = DRW::BAD_NONE;
    data.dwgReadOutcome.sourceVersion = source.m_version;
    data.dwgReadOutcome.sourceCodePageId = source.m_codePageId;
    data.dwgReadOutcome.sourceCodePageFieldPresent = source.m_codePagePresent;
    data.dwgReadOutcome.sourceCodePageRecognized = source.m_codePageRecognized;
    data.dwgClassCoverage = rows;
}
void evidenceCapture(dx_iface& adapter, dx_data& data, const Report& source,
                     const DRW_DwgClassCoverageReport& rows, std::uint64_t generation = 1u) {
    EvidenceAccess::open(adapter, data, generation);
    evidencePublic(data, source, rows);
    EvidenceAccess::rows(adapter, rows); EvidenceAccess::source(adapter, source);
    EvidenceAccess::finish(adapter, source);
}
void mutateRow(DRW_DwgClassCoverageEntry& row, unsigned change) {
    switch (change) {
    case 0: ++row.m_streamOrdinal; break; case 1: ++row.m_classNumber; break;
    case 2: row.m_appName += '!'; break; case 3: row.m_className += '!'; break;
    case 4: row.m_recordName += '!'; break; case 5: row.m_proxyFlag ^= 1; break;
    case 6: row.m_wasAProxyFlag ^= 1; break; case 7: row.m_entityFlagRaw ^= 1; break;
    case 8: row.m_entityFlag ^= 1; break; case 9: ++row.m_instanceCount; break;
    case 10: ++row.m_dwgVersion; break; case 11: ++row.m_maintenanceVersion; break;
    case 12: ++row.m_unknown1; break; case 13: ++row.m_unknown2; break;
    case 14: ++row.m_sectionDescriptorId; break; case 15: ++row.m_dataRange.m_startBit; break;
    case 16: ++row.m_dataRange.m_endBit; break; case 17: ++row.m_stringRange.m_startBit; break;
    case 18: ++row.m_stringRange.m_endBit; break; case 19: row.m_dataRange.m_present = false; break;
    case 20: row.m_stringRange.m_present = false; break;
    case 21: row.m_dataRange.m_sectionRelative = !row.m_dataRange.m_sectionRelative; break;
    case 22: row.m_stringRange.m_sectionRelative = !row.m_stringRange.m_sectionRelative; break;
    case 23: row.m_dataRange.m_offsetSpace = Space::Unknown; break;
    case 24: row.m_stringRange.m_offsetSpace = Space::Unknown; break;
    case 25: row.m_state = DRW_DwgClassCoverageState::Failed; break;
    case 26: row.m_reason = DRW_DwgClassCoverageReason::Parse; break;
    }
}
class EvidenceCallbackAdapter : public dx_iface {
public:
    unsigned mode{0u};
    void addDwgClassCoverageReport(const DRW_DwgClassCoverageReport& data) override {
        if (mode == 1u) { cData->dwgClassCoverage = data; return; } // Public copy is not authority.
        dx_iface::addDwgClassCoverageReport(data);
        if (mode == 2u) dx_iface::addDwgClassCoverageReport(data);
        if (mode == 3u) throw 1;
    }
    void addDwgClassSourceReport(const Report& source) noexcept override {
        if (mode != 4u) dx_iface::addDwgClassSourceReport(source);
        if (mode == 6u) dx_iface::addDwgClassSourceReport(source);
    }
    bool wantsDwgClassSource() const override {
        if (mode == 5u) throw 1;
        return true;
    }
};
bool nameEvidenceContracts(const std::filesystem::path& root) {
    // Contract fault seam, not native qualification. Obtain a real immutable
    // field owner first; all allocation faults are scoped to capture itself.
    dx_data seed; dx_iface reader;
    const auto path = (root / "ordinary_enc_AC1027.dwg").string();
    CHECK(reader.fileImport(path, &seed, false) && !seed.appIds.empty());
    const auto& row = seed.appIds.front();
    CHECK(row.dwgNameSource() && row.dwgNameSource()->matches(row.handle, row.name));
    const auto report = *seed.dwgClassSource();
    const std::string longName(1024u, 'X');
    for (long count = 0; count < 2; ++count) {
        dx_data data; dx_iface adapter;
        { AllocationFailure fault(count); EvidenceAccess::open(adapter, data, 1u); }
        EvidenceAccess::finish(adapter, report);
        CHECK(EvidenceAccess::nameBytes(data) == 0u && EvidenceAccess::nameCount(data) == 0u);
    }
    for (unsigned phase = 0u; phase < 4u; ++phase) {
        dx_data data; dx_iface adapter;
        EvidenceAccess::open(adapter, data, 1u);
        if (phase == 2u) EvidenceAccess::nameLimits(data, EvidenceAccess::nameBytes(data));
        if (phase < 3u) {
            AllocationFailure fault(phase == 1u ? 1 : 0);
            EvidenceAccess::name(adapter, row, longName);
        } else EvidenceAccess::name(adapter, row, row.name);
        { AllocationFailure fault(0); EvidenceAccess::finish(adapter, report, phase != 3u); }
        CHECK(EvidenceAccess::nameBytes(data) == 0u && EvidenceAccess::nameCount(data) == 0u);
        EvidenceAccess::nameLimits(data, DRW::kMaxDwgClassSourceBytes);
        CHECK(adapter.fileImport(path, &data, false) && EvidenceAccess::nameBytes(data) != 0u
            && EvidenceAccess::nameCount(data) == 1u);
    }
    return true;
}

bool evidenceContracts(const std::filesystem::path& root) {
    const auto path = (root / "ordinary_enc_AC1015.dwg").string();
    dx_iface importer;
    dx_data seed;
    CHECK(importer.fileImport(path, &seed, false) && seed.hasUnchangedDwgClassEvidence());
    const Report source = *seed.dwgClassSource();
    const DRW_DwgClassCoverageReport rows = seed.dwgClassCoverage;
    CHECK(!rows.m_entries.empty());
    const auto identity = EvidenceAccess::owner(seed).lock();
    CHECK(identity && identity->entries.size() == rows.m_entries.size());
    for (unsigned change = 0; change < 27u; ++change) {
        mutateRow(seed.dwgClassCoverage.m_entries[0], change);
        CHECK(!seed.hasUnchangedDwgClassEvidence());
        CHECK(EvidenceAccess::owner(seed).lock() == identity
            && dx_class_evidence::matches(*identity, rows));
        seed.dwgClassCoverage = rows;
        CHECK(seed.hasUnchangedDwgClassEvidence());
        dx_iface adapter; dx_data changed;
        auto report = rows; mutateRow(report.m_entries[0], change);
        evidenceCapture(adapter, changed, source, report);
        CHECK(changed.dwgClassEvidence().state == EvidenceState::RowMismatch
            && !changed.hasUnchangedDwgClassEvidence() && EvidenceAccess::evidenceBytes(changed) == 0u);
    }
    for (unsigned change = 0; change < 7u; ++change) {
        switch (change) {
        case 0: ++seed.importGeneration; break;
        case 1: seed.sourceFormat = dx_source_format::DXF; break;
        case 2: seed.sourceVersion = DRW::UNKNOWNV; break;
        case 3: seed.dwgReadOutcome.operationSucceeded = false; break;
        case 4: seed.dwgReadOutcome.sourceVersion = DRW::UNKNOWNV; break;
        case 5: ++seed.dwgReadOutcome.sourceCodePageId; break;
        case 6: seed.dwgReadOutcome.sourceCodePageRecognized = !source.m_codePageRecognized; break;
        }
        CHECK(!seed.hasUnchangedDwgClassEvidence());
        seed.importGeneration = 1u; seed.sourceFormat = dx_source_format::DWG;
        evidencePublic(seed, source, rows);
        CHECK(seed.hasUnchangedDwgClassEvidence());
    }
    dx_iface costAdapter; dx_data costData;
    evidenceCapture(costAdapter, costData, source, rows);
    const auto cost = EvidenceAccess::evidenceBytes(costData);
    CHECK(costData.hasUnchangedDwgClassEvidence() && cost > source.m_byteCount);
    for (const auto cap : {cost - 1u, cost}) {
        dx_iface adapter; dx_data data;
        EvidenceAccess::evidenceLimits(data, cap);
        evidenceCapture(adapter, data, source, rows);
        CHECK(data.dwgClassEvidence().state == (cap == cost ? EvidenceState::Joined : EvidenceState::TooLarge));
        CHECK(EvidenceAccess::evidenceBytes(data) == (cap == cost ? cost : 0u));
    }
    {
        dx_iface adapter; dx_data data;
        EvidenceAccess::open(adapter, data, 1u);
        EvidenceAccess::evidenceLimits(data, EvidenceAccess::evidenceBytes(data));
        { AllocationFailure fault(0); EvidenceAccess::rows(adapter, rows); }
        EvidenceAccess::finish(adapter, source);
        CHECK(data.dwgClassEvidence().state == EvidenceState::TooLarge && EvidenceAccess::evidenceBytes(data) == 0u);
    }
    unsigned failures = 0u, successes = 0u;
    for (long count = 0; count < 6; ++count) {
        dx_iface adapter; dx_data data;
        EvidenceAccess::open(adapter, data, 1u); evidencePublic(data, source, rows);
        { AllocationFailure fault(count); EvidenceAccess::rows(adapter, rows); }
        { AllocationFailure fault(0); EvidenceAccess::source(adapter, source); EvidenceAccess::finish(adapter, source); }
        if (data.dwgClassEvidence().state == EvidenceState::CaptureFailed) {
            ++failures;
            CHECK(EvidenceAccess::evidenceBytes(data) == 0u && EvidenceAccess::evidenceCount(data) == 0u);
            evidenceCapture(adapter, data, source, rows, 2u);
            CHECK(data.hasUnchangedDwgClassEvidence());
        } else { ++successes; CHECK(data.hasUnchangedDwgClassEvidence()); }
    }
    CHECK(failures >= 3u && successes != 0u);
    for (unsigned mode = 0; mode < 4u; ++mode) {
        dx_iface adapter; dx_data data;
        if (mode == 0u) EvidenceAccess::evidenceLimits(data, DRW::kMaxDwgClassSourceBytes, 0u);
        if (mode == 1u) { AllocationFailure fault(0); EvidenceAccess::open(adapter, data, 1u); }
        else EvidenceAccess::open(adapter, data, 1u);
        evidencePublic(data, source, rows);
        if (mode >= 2u) EvidenceAccess::rows(adapter, rows);
        if (mode == 2u) EvidenceAccess::rows(adapter, rows);
        EvidenceAccess::source(adapter, source);
        if (mode == 3u) EvidenceAccess::source(adapter, source);
        EvidenceAccess::finish(adapter, source);
        CHECK(data.dwgClassEvidence().state == (mode == 0u ? EvidenceState::TooLarge : EvidenceState::CaptureFailed)
            && EvidenceAccess::evidenceBytes(data) == 0u && !data.hasUnchangedDwgClassEvidence());
    }
    for (unsigned mode = 0; mode < 5u; ++mode) {
        dx_iface adapter; dx_data data;
        EvidenceAccess::open(adapter, data, 1u); evidencePublic(data, source, rows);
        auto partial = rows;
        if (mode == 3u) partial.m_complete = false;
        if (mode != 0u) EvidenceAccess::rows(adapter, partial);
        if (mode != 1u) EvidenceAccess::source(adapter, source);
        EvidenceAccess::finish(adapter, source, mode != 2u);
        const auto expected = mode == 0u ? EvidenceState::MissingRows : mode == 1u ? EvidenceState::MissingSource
            : mode == 2u ? EvidenceState::ReadFailed : mode == 3u ? EvidenceState::IncompleteRows : EvidenceState::Joined;
        CHECK(data.dwgClassEvidence().state == expected);
    }
    for (unsigned mode = 0; mode < 11u; ++mode) {
        dx_iface adapter; dx_data data;
        EvidenceAccess::open(adapter, data, 1u); evidencePublic(data, source, rows);
        EvidenceAccess::rows(adapter, rows);
        auto changed = source;
        auto expected = EvidenceState::ContextMismatch;
        switch (mode) {
        case 0: changed.m_version = DRW::AC1032; break;
        case 1: ++changed.m_codePageId; break;
        case 2: changed.m_codePagePresent = !changed.m_codePagePresent; break;
        case 3: changed.m_offsetSpace = Space::Unknown; expected = EvidenceState::CaptureFailed; break;
        case 4: ++changed.m_sectionDescriptorId; expected = EvidenceState::RowMismatch; break;
        case 5: ++changed.m_baseByteOffset; expected = EvidenceState::RowMismatch; break;
        case 6: --changed.m_byteCount; expected = EvidenceState::CaptureFailed; break;
        case 7: ++changed.m_declaredSectionSize; expected = EvidenceState::CaptureFailed; break;
        case 8: changed.m_bytes.reset(); expected = EvidenceState::CaptureFailed; break;
        case 9: changed.m_classReadCompleted = false; expected = EvidenceState::SourceUnavailable; break;
        case 10: {
            auto bytes = std::make_shared<std::vector<std::uint8_t>>(*source.m_bytes);
            (*bytes)[0] ^= 1u; changed.m_bytes = std::move(bytes); expected = EvidenceState::FramingRefused; break;
        }
        }
        EvidenceAccess::source(adapter, changed); EvidenceAccess::finish(adapter, source);
        CHECK(data.dwgClassEvidence().state == expected && !data.hasUnchangedDwgClassEvidence());
    }
    // Actual read boundary, not the friend seam: fake success/public generation
    // cannot revive an old private report after a failed or repeated attempt.
    importer.addDwgClassCoverageReport(rows);
    CHECK(seed.dwgClassEvidence().state == EvidenceState::LateCallback && !seed.hasUnchangedDwgClassEvidence());
    CHECK(importer.fileImport(path, &seed, false) && seed.hasUnchangedDwgClassEvidence());
    CHECK(!importer.fileImport((root / "missing-class-evidence.dwg").string(), &seed, false));
    CHECK(seed.dwgClassEvidence().state == EvidenceState::ReadFailed);
    seed.importGeneration = 1u; evidencePublic(seed, source, rows);
    CHECK(!seed.hasUnchangedDwgClassEvidence() && seed.dwgClassEvidence().state == EvidenceState::ReadFailed);
    CHECK(importer.fileImport(path, &seed, false));
    CHECK(seed.dwgClassEvidence().state == EvidenceState::ContextMismatch && !seed.hasUnchangedDwgClassEvidence());
    CHECK(EvidenceAccess::historyState(seed, 2u) == EvidenceState::ContextMismatch);
    seed.importGeneration = 3u;
    CHECK(importer.fileImport(path, &seed, false) && seed.hasUnchangedDwgClassEvidence());
    CHECK(!importer.fileImport("not-a-drawing.bin", &seed, false)
        && seed.dwgClassEvidence().state == EvidenceState::NotDwg && !seed.hasUnchangedDwgClassEvidence());
    for (unsigned mode = 1u; mode <= 6u; ++mode) {
        EvidenceCallbackAdapter adapter; adapter.mode = mode; dx_data data;
        CHECK(adapter.fileImport(path, &data, false));
        const auto expected = mode == 1u ? EvidenceState::MissingRows : (mode == 2u || mode == 6u) ? EvidenceState::CaptureFailed
            : mode == 4u ? EvidenceState::MissingSource : EvidenceState::SourceUnavailable;
        CHECK(data.dwgClassEvidence().state == expected && !data.hasUnchangedDwgClassEvidence()
            && EvidenceAccess::evidenceBytes(data) == 0u);
    }
    for (unsigned mode = 0; mode < 3u; ++mode) {
        dx_iface adapter; dx_data data, redirected;
        EvidenceAccess::open(adapter, data, 1u); evidencePublic(data, source, rows);
        EvidenceAccess::rows(adapter, rows);
        if (mode == 0u) adapter.cData = &redirected;
        if (mode == 1u) ++data.importGeneration;
        if (mode == 2u) CHECK(!adapter.fileImport(path, &redirected, false));
        EvidenceAccess::source(adapter, source); EvidenceAccess::finish(adapter, source);
        CHECK(data.dwgClassEvidence().state == EvidenceState::ContextMismatch
            && !data.hasUnchangedDwgClassEvidence() && !redirected.hasUnchangedDwgClassEvidence()
            && EvidenceAccess::evidenceBytes(data) == 0u);
    }
    dx_iface limited; dx_data capped;
    EvidenceAccess::evidenceLimits(capped, DRW::kMaxDwgClassSourceBytes, 1u);
    CHECK(limited.fileImport(path, &capped, false) && capped.hasUnchangedDwgClassEvidence());
    CHECK(limited.fileImport(path, &capped, false) && capped.dwgClassEvidence().state == EvidenceState::TooLarge
        && !capped.hasUnchangedDwgClassEvidence());
    capped.importGeneration = (std::numeric_limits<std::uint64_t>::max)();
    CHECK(!limited.fileImport(path, &capped, false) && capped.dwgClassEvidence().state == EvidenceState::ContextMismatch);
    {
        const std::vector<std::uint8_t> bytes{
            0x8D,0xA1,0xC4,0xB8,0xC4,0xA9,0xF8,0xC5,0xC0,0xDC,0xF4,0x5F,0xE7,0xCF,0xB6,0x8A,
            0,0,0,0,0x3D,0xC0,
            0x72,0x5E,0x3B,0x47,0x3B,0x56,0x07,0x3A,0x3F,0x23,0x0B,0xA0,0x18,0x30,0x49,0x75};
        auto empty = source;
        empty.m_byteCount = empty.m_declaredSectionSize = bytes.size();
        empty.m_bytes = std::make_shared<const std::vector<std::uint8_t>>(bytes);
        auto noRows = rows; noRows.m_entries.clear();
        dx_iface adapter; dx_data data;
        evidenceCapture(adapter, data, empty, noRows);
        CHECK(data.dwgClassEvidence().state == EvidenceState::Joined && data.dwgClassEvidence().rows == 0u
            && data.hasUnchangedDwgClassEvidence());
        data.dwgClassCoverage.m_entries.push_back(rows.m_entries[0]);
        CHECK(!data.hasUnchangedDwgClassEvidence());
    }
    return true;
}
}

int main(int argc, char** argv) {
    static_assert(std::is_same<decltype(Report{}.m_bytes),
        std::shared_ptr<const std::vector<std::uint8_t>>>::value, "source bytes must be immutable");
    static_assert(std::is_same<decltype(std::declval<const dx_data&>().dwgClassSource()),
        const Report*>::value, "source report must be read-only");
    static_assert(std::is_same<decltype(std::declval<const DRW_AppId&>().dwgNameSource()),
        std::shared_ptr<const DRW_DwgTableNameSource>>::value, "APPID source owner must be a read-only value, not an installable reference");
    static_assert(std::is_same<decltype(std::declval<const DRW_Layer&>().dwgNameSource()),
        std::shared_ptr<const DRW_DwgTableNameSource>>::value, "LAYER source owner must be a read-only value");
    if (argc != 3) return 2;
    std::filesystem::create_directories(argv[2]);
    std::vector<std::uint8_t> bytes(32u);
    for (std::size_t i = 0; i < bytes.size(); ++i) bytes[i] = static_cast<std::uint8_t>(i * 7u);
    dwgBuffer memory(bytes.data(), bytes.size()), control(bytes.data(), bytes.size());
    const auto path = std::filesystem::path(argv[2]) / "cursor.bin";
    { std::ofstream out(path, std::ios::binary); out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size()); }
    std::ifstream input(path, std::ios::binary), reference(path, std::ios::binary);
    dwgBuffer file(&input), fileControl(&reference);
    if (!captureContracts() || !readerTails() || !physicalCursor(memory, control, bytes)
        || !physicalCursor(file, fileControl, bytes) || !adapterContracts() || !native(argv[1])
        || !evidenceContracts(argv[1]) || !nameEvidenceContracts(argv[1])) return 1;
    std::cout << "DWG class source: " << checks << " checks PASS; no framing/semantic qualification\n";
    return 0;
}
