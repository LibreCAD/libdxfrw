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
#include "intern/dwgwriter15.h"
#include "intern/dwgwriter18.h"
#include "intern/dwgwriter21.h"
#include "intern/dwgwriter24.h"
#include "intern/dwgwriter27.h"
#include "intern/dwgwriter32.h"

namespace {

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
