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
    static void limits(dx_data& data, std::size_t bytes, std::size_t reports = 1024u) {
        data.m_classSourceByteLimit = bytes;
        data.m_classSourceReportLimit = reports;
    }
    static std::size_t retained(const dx_data& data) { return data.m_classSourceRetainedBytes; }
    static std::size_t reports(const dx_data& data) { return data.m_classSourceReports.size(); }
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
        const auto generation = captured.importGeneration;
        CHECK(adapter.fileImport(path.string(), &captured, false));
        CHECK(captured.importGeneration == generation + 1u && captured.dwgClassSource()
            && captured.dwgClassSource()->m_status == Status::Captured
            && captured.dwgClassSource()->m_bytes != lease
            && *captured.dwgClassSource()->m_bytes == *lease);
        std::cout << name << ": source capture/no-copy parity PASS\n";
    }
    return true;
}
}

int main(int argc, char** argv) {
    static_assert(std::is_same<decltype(Report{}.m_bytes),
        std::shared_ptr<const std::vector<std::uint8_t>>>::value, "source bytes must be immutable");
    static_assert(std::is_same<decltype(std::declval<const dx_data&>().dwgClassSource()),
        const Report*>::value, "source report must be read-only");
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
        || !physicalCursor(file, fileControl, bytes) || !adapterContracts() || !native(argv[1])) return 1;
    std::cout << "DWG class source: " << checks << " checks PASS; no framing/semantic qualification\n";
    return 0;
}
