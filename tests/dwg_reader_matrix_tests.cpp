#include <algorithm>
#include <array>
#include <chrono>
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
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "libdwgr.h"
#include "dx_iface.h"

#include "intern/dwgreader15.h"
#include "intern/dwgreader18.h"
#include "intern/dwgreader21.h"
#include "intern/dwgreader24.h"
#include "intern/dwgreader27.h"
#include "intern/dwgreader32.h"
#include "intern/dwgreaderR11.h"
#include "intern/dwgreaderR1_40.h"
#include "intern/dwgreader.h"
#include "intern/dwgutil.h"
#include "intern/drw_textcodec.h"
#include "intern/dwgtemplate.h"
#include "intern/dwgheadersection.h"
#include "intern/dwgwriter15.h"
#include "intern/dwgwriter18.h"
#include "intern/dwgwriter21.h"
#include "intern/dwgwriter24.h"
#include "intern/dwgwriter27.h"
#include "intern/dwgwriter32.h"

namespace {

static_assert(static_cast<unsigned>(DwgIntegrityCheckKind::TemplateDescriptionUnsupported) == 20u,
              "existing Template integrity categories must not be renumbered");
static_assert(static_cast<unsigned>(DwgIntegrityCheckKind::HeaderSectionBounds) == 21u
                  && static_cast<unsigned>(DwgIntegrityCheckKind::HeaderSectionSentinel) == 22u
                  && static_cast<unsigned>(DwgIntegrityCheckKind::HeaderSectionCrc) == 23u,
              "HEADER integrity categories append to the stable diagnostic contract");

struct TestContext {
    int failures {0};

    void expect(bool condition, const std::string& label) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << label << '\n';
        }
    }
};

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
                                 std::uint32_t address = 0)
        : dwgReader15(std::make_unique<dwgBuffer>(bytes.data(), bytes.size()), nullptr) {
        version = DRW::AC1015;
        dwgSectionInfo section;
        section.Id = 0;
        section.size = bytes.size() - address;
        section.address = address;
        sections.emplace(secEnum::HEADER, section);
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

    // Fixed finite USERR1 bits give a genuinely zero checksum for this
    // generated full section; no search loop or opaque payload is needed.
    DRW_Header zeroSource;
    zeroSource.addInt("LUNITS", 4, 70);
    constexpr std::uint64_t zeroBits = UINT64_C(0x400d8939b2fec9f5);
    double zeroValue = 0;
    std::memcpy(&zeroValue, &zeroBits, sizeof zeroValue);
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

class FullHeaderWriterProbe final : public dwgWriter15 {
public:
    FullHeaderWriterProbe(std::ofstream* stream, DRW_Header* header)
        : dwgWriter15(stream, header) {}
    std::size_t headerAddress() const { return m_sectionOffsets.at(0); }
    std::size_t classesAddress() const { return m_sectionOffsets.at(1); }
};

class HeaderPublicationSink final : public dx_iface {
public:
    HeaderPublicationSink() {
        cData = &data;
        currentBlock = data.mBlock;
    }
    void addHeader(const DRW_Header*) override { ++headers; }
    std::size_t headers{0};
    dx_data data;
};

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
    // finalizeHeaderHandseed() edits the already-sealed HEADER. Normalize
    // this generated control's checksum explicitly; writer resealing is a
    // separate concern from the reader's warning/structural policy here.
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

    std::vector<std::uint8_t> malformed(5200, 0);
    malformed[footerEndBit / 8 - 4] = 0xff;
    malformed[footerEndBit / 8 - 3] = 0xff;
    malformed[footerEndBit / 8 - 2] = 0xff;
    malformed[footerEndBit / 8 - 1] = 0xff;
    dwgBuffer bad(malformed.data(), malformed.size());
    t.expect(!readDwgClassStringFooter(bad, footerEndBit, start, size),
             "AC1024 class footer rejects an overlong string-size extension");
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

}  // namespace

int main() {
    TestContext context;
    testBoundedFork(context);
    testHeaderSectionBounds(context);
    testHeaderPublicRead(context);
    testDwgOperationAdmission(context);
    testVersionDispatch(context);
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
