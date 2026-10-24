#include <algorithm>
#include <array>
#include <cstdint>
#include <chrono>
#include <charconv>
#include <clocale>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <string>
#include <vector>

#include "drw_base.h"
#include "drw_header.h"
#include "intern/drw_textcodec.h"
#include "intern/dwgreader18.h"
#include "intern/dwgreader21.h"
#include "intern/dwgreader15.h"
#include "intern/dwgreaderR11.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"
#include "libdwgr.h"
#include "dwg2dxf/dx_data.h"
#include "dwg2dxf/dx_iface.h"
#include "xrecord_test_helpers.h"
#include "appid_test_helpers.h"
#include "layer_test_helpers.h"

namespace {
bool failImportObservationAllocations = false;
std::size_t standardPayloadFailAt = 0u;
std::size_t standardPayloadAllocations = 0u;

struct StandardOutputAllocationState {
    enum class Phase { None, Inspection, Finalization, GroupPlan, TableInspection, LayerRows, AppIdRows };
    Phase phase{Phase::None};
    std::size_t failAt{0u}, measured{0u};
    bool entered{false};
};

class StandardOutputAllocationScope {
public:
    explicit StandardOutputAllocationScope(StandardOutputAllocationState& state) noexcept
        : state_(state) {
        state_.entered = true;
        standardPayloadAllocations = 0u;
        standardPayloadFailAt = state_.failAt;
    }
    ~StandardOutputAllocationScope() {
        state_.measured = standardPayloadAllocations;
        standardPayloadFailAt = 0u;
    }
private:
    StandardOutputAllocationState& state_;
};
}

// Only armed after a test callback throws a nonallocating sentinel. Exercise
// diagnostic-copy failure during unwind without changing production allocators.
void* operator new(std::size_t size) {
    if (standardPayloadFailAt != 0u
        && ++standardPayloadAllocations == standardPayloadFailAt)
        throw std::bad_alloc();
    if (!failImportObservationAllocations) {
        if (void* memory = std::malloc(size == 0 ? 1 : size)) return memory;
    }
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

class DxIfaceStandardDwgCaptureTestAccess {
public:
    static void wrapInspector(dxfRW& writer, StandardOutputAllocationState& state) {
        auto inspect = std::move(writer.m_dxfObjectsWriteInspector);
        writer.m_dxfObjectsWriteInspector = [inspect = std::move(inspect), &state](const auto& view) {
            StandardOutputAllocationScope inject(state);
            return inspect && inspect(view);
        };
    }
    static void wrapTableInspector(dxfRW& writer, StandardOutputAllocationState& state) {
        auto inspect = std::move(writer.m_dxfTablesWriteInspector);
        writer.m_dxfTablesWriteInspector = [inspect = std::move(inspect), &state](const auto& view) {
            StandardOutputAllocationScope inject(state);
            return inspect && inspect(view);
        };
    }
    static bool planGroups(dxfRW& writer, StandardOutputAllocationState& state) {
        StandardOutputAllocationScope inject(state);
        return writer.planDxfGroupHandles();
    }
    static bool output(const dx_iface& iface, std::size_t count, bool published) {
        return iface.m_standardDwgOutputProofs.size() == count
            && iface.m_standardDwgOutputPublished == published
            && (!published || (iface.m_standardDwgOutputInspected && iface.m_standardDwgOutputFinalized));
    }
    static std::size_t acdsBaselines(const dx_data& data) {
        return data.m_acdsRawDxfSectionBaselines.size();
    }
    static void finish(dx_iface& iface) {
        iface.finishStandardDwgObjectCapture(true);
    }
    static void validate(dx_iface& iface, std::size_t first = 0u) {
        iface.validateStandardDwgObjectPayloads({0u, 0u, first, 0u, 0u});
    }
    static bool matched(const dx_data& data, std::uint64_t generation,
                        std::uint32_t handle, std::uint16_t codePage) {
        const auto& frame = data.m_standardDwgObjectFrames.at({generation, handle});
        return frame.payloadMatched && frame.sourceCodePageId == codePage;
    }
    static bool unchanged(const dx_iface& iface) {
        return iface.hasUnchangedStandardDwgObjectSources();
    }
};

// Test-only access to the reader's primary codec and version.  The production
// API exposes only the source-code-page snapshot; no test seam is exported.
class DwgCodePageTestAccess {
public:
    static void setVersion(dwgReader& reader, DRW::Version version) {
        reader.version = version;
        reader.decoder.setVersion(version, false);
    }

    static std::string primaryCodePage(dwgReader& reader) {
        return reader.decoder.getCodePage();
    }

    static bool record(dwgReader& reader, std::uint16_t id, bool present,
                       bool applyPrimaryCodec) {
        return reader.recordSourceCodePage(id, present, applyPrimaryCodec);
    }

    static bool hasField(const dwgReader& reader) {
        return reader.hasSourceCodePageField();
    }

    static bool recognized(const dwgReader& reader) {
        return reader.hasSourceCodePage();
    }

    static std::string sourceName(const dwgReader& reader) {
        return reader.getSourceCodePageName();
    }

    static std::uint16_t sourceId(const dwgReader& reader) {
        return reader.getSourceCodePageId();
    }

    static std::string decodeByteText(dwgReader& reader,
                                      const std::string& value) {
        return reader.decoder.toUtf8CP8(value);
    }

    static bool capturePublicSnapshot(dwgRW& facade, std::uint16_t id,
                                      bool present, bool applyPrimaryCodec) {
        facade.version = DRW::AC1018;
        facade.reader = std::make_unique<dwgReader18>(
            std::unique_ptr<dwgBuffer>{}, &facade);
        setVersion(*facade.reader, DRW::AC1018);
        if (!record(*facade.reader, id, present, applyPrimaryCodec))
            return false;
        facade.captureReaderDiagnostics(true);
        facade.reader.reset();
        return true;
    }
};

class PreparedDxInterface final : public dx_iface {
public:
    void attach(dx_data* data) {
        cData = data;
        currentBlock = data != nullptr ? data->mBlock : nullptr;
    }
    void attachWriter(dxfRW* writer) { dxfW = writer; }

    void addMLine(const DRW_MLine* data) override {
        mlineStyleName = data == nullptr ? std::string{} : data->styleName;
    }

    void writeEntities() override {
        for (DRW_Entity* entity : cData->mBlock->ent) {
            if (entity != nullptr && entity->eType == DRW::MLINE)
                dxfW->writeMLine(static_cast<DRW_MLine*>(entity));
            else
                writeEntity(entity);
        }
    }

    std::string mlineStyleName;
};

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

std::string headerString(const DRW_Header& header, const char* key) {
    const auto found = header.vars.find(key);
    return found != header.vars.end() && found->second != nullptr
        && found->second->type() == DRW_Variant::STRING
        ? std::string(found->second->c_str()) : std::string{};
}

std::string makeMalformedBytes(std::initializer_list<unsigned int> values) {
    std::string result;
    result.reserve(values.size());
    for (unsigned int value : values)
        result.push_back(static_cast<char>(value));
    return result;
}

bool validateAsciiDxfStructure(const std::string& bytes) {
    std::istringstream input(bytes);
    std::string codeLine;
    std::string valueLine;
    bool sawEof = false;
    while (std::getline(input, codeLine)) {
        if (!codeLine.empty() && codeLine.back() == '\r')
            codeLine.pop_back();
        if (!std::getline(input, valueLine))
            return false;
        if (!valueLine.empty() && valueLine.back() == '\r')
            valueLine.pop_back();
        if (codeLine.empty())
            return false;
        const auto first = codeLine.find_first_not_of(" \t");
        const auto last = codeLine.find_last_not_of(" \t");
        if (first == std::string::npos)
            return false;
        codeLine = codeLine.substr(first, last - first + 1);
        int code = 0;
        const auto parsed = std::from_chars(codeLine.data(),
                                            codeLine.data() + codeLine.size(),
                                            code);
        if (parsed.ec != std::errc{} || parsed.ptr != codeLine.data()
            + codeLine.size())
            return false;
        if (code == 0 && valueLine == "EOF") {
            sawEof = true;
            break;
        }
    }
    if (!sawEof)
        return false;
    std::string trailing;
    while (std::getline(input, trailing)) {
        if (!trailing.empty() && trailing != "\r")
            return false;
    }
    return true;
}

bool validateBinaryDxfStructure(const std::string& bytes) {
    static constexpr char signature[] = "AutoCAD Binary DXF\r\n\x1a\0";
    return bytes.size() >= sizeof(signature) - 1
        && bytes.compare(0, sizeof(signature) - 1,
                        signature, sizeof(signature) - 1) == 0
        && bytes.find("EOF") != std::string::npos;
}

struct MTextWireChunk {
    int code;
    std::string value;
};

bool readMTextWireChunks(const std::filesystem::path& path, bool binary,
                        std::vector<MTextWireChunk>& chunks) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    std::unique_ptr<dxfReader> reader;
    if (binary) {
        static constexpr char sentinel[] = "AutoCAD Binary DXF\r\n\x1a\0";
        std::string signature(sizeof(sentinel) - 1, '\0');
        input.read(signature.data(), static_cast<std::streamsize>(signature.size()));
        if (!input || signature != std::string(sentinel, sizeof(sentinel) - 1))
            return false;
        reader = std::make_unique<dxfReaderBinary>(&input);
    } else {
        reader = std::make_unique<dxfReaderAscii>(&input);
    }
    bool inMText = false;
    unsigned int mtextCount = 0;
    int code = 0;
    while (reader->readRec(&code)) {
        if (code == 0) {
            const std::string type = reader->getString();
            if (type == "EOF")
                return mtextCount == 1;
            inMText = type == "MTEXT";
            if (inMText)
                ++mtextCount;
        } else if (inMText && (code == 3 || code == 1)) {
            // Read the physical string payload without semantic MIF decoding.
            chunks.push_back({code, reader->getString()});
        }
    }
    return false;
}

void testCodePageAliases(TestContext& t) {
    t.expect(std::string(dwgCodePageName(31)) == "ANSI_936",
             "DWG codepage id 31 maps to ANSI_936");
    t.expect(std::string(dwgCodePageName(39)) == "ANSI_936",
             "DWG codepage id 39 maps to ANSI_936");
    t.expect(dwgCodePageId("ANSI_936") == 39,
             "ANSI_936 uses canonical writer id 39");

    for (const char* alias : {"GBK", "GB2312", "CP936", "ANSI_936", "GB18030"}) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1015, false);
        codec.setCodePage(alias, false);
        const std::string decoded = codec.toUtf8(makeMalformedBytes({0xD6, 0xD0}));
        t.expect(codec.getCodePage() == "ANSI_936",
                 "GBK-family alias normalizes to ANSI_936");
        t.expect(decoded == "中",
                 "CP936 alias decodes representative CJK");
        t.expect(dwgCodePageId(codec.getCodePage().c_str()) == 39,
                 "normalized GBK-family alias uses canonical writer id 39");
    }

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1015, false);
    codec.setCodePage("ANSI_936", false);
    t.expect(codec.fromUtf8("中文A") == makeMalformedBytes({0xD6, 0xD0,
                                                               0xCE, 0xC4,
                                                               0x41}),
             "CP936 encodes mixed CJK and ASCII exactly");
    t.expect(codec.toUtf8(makeMalformedBytes({0xD6, 0xD0, 0xCE, 0xC4}))
                 == "中文",
             "CP936 decodes mixed CJK exactly");
    t.expect(codec.toUtf8("\\U+4E2D") == "中",
             "CP936 decodes a four-digit Unicode escape");
    t.expect(codec.toUtf8("\\M+5D6D0") == "中",
             "CP936 decodes a GBK MIF escape");
    t.expect(codec.toUtf8(makeMalformedBytes({0x81, 0x30, 0x81, 0x30}))
                 == "??",
             "CP936 rejects a four-byte GB18030 sequence as out of scope");
    t.expect(codec.toUtf8(makeMalformedBytes({0xD6})) == "?",
             "truncated CP936 lead byte is bounded");
    t.expect(codec.fromUtf8("\xF0\xA0\x80\xA1") == "?",
             "valid supplementary codepoint keeps documented fallback");
}

void testCifEscapePolicy(TestContext& t) {
    struct Row {
        const char* codePage;
        DRW::Version version;
        bool dxfFormat;
        const char* source;
        const char* expected;
    };
    const Row rows[] = {
        {"ANSI_936", DRW::AC1015, false, "\\U+abcdZ", "\xEA\xAF\x8DZ"},
        {"ANSI_1252", DRW::AC1015, false, "\\U+0041B", "AB"},
        {"ANSI_936", DRW::AC1015, false, "\\U+0041\\U+0042", "AB"},
        {"ANSI_932", DRW::AC1015, false, "ordinary \\path", "ordinary \\path"},
        {"ANSI_1252", DRW::AC1024, true, "\\U+0041B", "AB"},
    };
    for (const auto& row : rows) {
        DRW_TextCodec codec;
        codec.setVersion(row.version, row.dxfFormat);
        codec.setCodePage(row.codePage, row.dxfFormat);
        t.expect(codec.toUtf8(row.source) == row.expected
                     && codec.lastDecodeObservation().issues == 0,
                 "CIF consumes exactly four hex digits across single-byte, DBCS and modern routes");
    }
}

void testTurkishCodePageRoundTrip(TestContext& t) {
    const char* currentLocale = std::setlocale(LC_CTYPE, nullptr);
    const std::string previousLocale = currentLocale != nullptr
        ? currentLocale : "C";
    const char* selectedLocale = nullptr;
    for (const char* candidate : {"tr_TR.UTF-8", "tr_TR.utf8", "tr_TR",
                                   "Turkish_Turkey.1254"}) {
        selectedLocale = std::setlocale(LC_CTYPE, candidate);
        if (selectedLocale != nullptr)
            break;
    }
    if (selectedLocale == nullptr) {
        std::cout << "Turkish codepage round-trip: SKIP (locale unavailable)\n";
        return;
    }
    const int failuresBefore = t.failures;
    struct LocaleRestore {
        std::string name;
        ~LocaleRestore() { (void)std::setlocale(LC_CTYPE, name.c_str()); }
    } restore{previousLocale};

    const auto nonce = std::chrono::steady_clock::now()
                           .time_since_epoch()
                           .count();
    const std::filesystem::path sourcePath =
        std::filesystem::temp_directory_path()
        / ("libdxfrw-turkish-codepage-source-" + std::to_string(nonce)
           + ".dxf");
    const std::filesystem::path outputPath =
        std::filesystem::temp_directory_path()
        / ("libdxfrw-turkish-codepage-output-" + std::to_string(nonce)
           + ".dxf");
    struct FileCleanup {
        std::filesystem::path first;
        std::filesystem::path second;
        ~FileCleanup() {
            std::error_code ignored;
            std::filesystem::remove(first, ignored);
            std::filesystem::remove(second, ignored);
        }
    } cleanup{sourcePath, outputPath};

    {
        std::ofstream output(sourcePath, std::ios::binary);
        if (!output) {
            t.expect(false, "could not create generated Turkish codepage DXF");
            return;
        }
        output << "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1018\n"
               << "9\n$DWGCODEPAGE\n3\nansi_1254\n"
               << "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n"
               << "0\nTEXT\n5\n10\n100\nAcDbEntity\n8\n0\n"
               << "100\nAcDbText\n10\n1\n20\n2\n30\n0\n40\n1\n1\n";
        output.put(static_cast<char>(0xD0)); // CP1254 U+011E LATIN CAPITAL G WITH BREVE
        output << "\n0\nENDSEC\n0\nEOF\n";
    }

    dx_data source;
    dx_iface importer;
    const bool readSource = importer.fileImport(sourcePath.string(), &source,
                                                false);
    const DRW_Text* sourceText = nullptr;
    if (source.mBlock != nullptr) {
        for (const DRW_Entity* entity : source.mBlock->ent) {
            sourceText = dynamic_cast<const DRW_Text*>(entity);
            if (sourceText != nullptr)
                break;
        }
    }
    const std::string expectedUtf8("\xC4\x9E", 2u);
    t.expect(readSource && sourceText != nullptr
                 && sourceText->text == expectedUtf8
                 && source.headerC.dxfCodePageDeclaration() == "ansi_1254",
             "Turkish locale resolves lowercase ANSI_1254 and decodes CP1254 U+011E");
    if (!readSource || sourceText == nullptr || sourceText->text != expectedUtf8)
        return;

    dx_iface writer;
    t.expect(writer.fileExport(outputPath.string(), DRW::AC1018, false,
                               &source, false),
             "Turkish legacy-text drawing exports under Turkish locale");
    if (!std::filesystem::exists(outputPath))
        return;
    std::ifstream rawInput(outputPath, std::ios::binary);
    const std::string raw((std::istreambuf_iterator<char>(rawInput)),
                          std::istreambuf_iterator<char>());
    const std::string expectedGroup = std::string("1\n")
        + static_cast<char>(0xD0) + "\n";
    t.expect(raw.find(expectedGroup) != std::string::npos
                 && raw.find("ANSI_1254") != std::string::npos,
             "legacy output uses deterministic CP1254 bytes and canonical codepage declaration");

    dx_data readback;
    dx_iface readbackImporter;
    const bool readOutput = readbackImporter.fileImport(
        outputPath.string(), &readback, false);
    const DRW_Text* readbackText = nullptr;
    if (readback.mBlock != nullptr) {
        for (const DRW_Entity* entity : readback.mBlock->ent) {
            readbackText = dynamic_cast<const DRW_Text*>(entity);
            if (readbackText != nullptr)
                break;
        }
    }
    t.expect(readOutput && readbackText != nullptr
                 && readbackText->text == expectedUtf8
                 && readback.headerC.dxfCodePageDeclaration() == "ANSI_1254",
             "Turkish-locale CP1254 output re-import preserves the exact Unicode scalar");
    std::cout << "Turkish codepage round-trip: "
              << (t.failures == failuresBefore ? "PASS" : "FAIL")
              << " (" << selectedLocale << ")\n";
}


struct CodePageAnchor {
    const char* name;
    const char* bytes;
    const char* utf8;
};

std::string unicodeScalarExpectedUtf8(int scalar);

void testCodePagePolicyAnchors(TestContext& t) {
    // Literal Unicode anchors, independently checked with the standard
    // codecs and Unicode vendor mapping references. No peer fallback or
    // local table is used to derive the expected text.
    const CodePageAnchor canonical[] = {
        {"ANSI_874", "\xA1", "\xE0\xB8\x81"},
        {"ANSI_932", "\x82\xA0", "\xE3\x81\x82"},
        {"ANSI_936", "\xD6\xD0", "\xE4\xB8\xAD"},
        {"ANSI_949", "\xB0\xA1", "\xEA\xB0\x80"},
        {"ANSI_950", "\xA4\xA4", "\xE4\xB8\xAD"},
        {"ANSI_1250", "\xA5", "\xC4\x84"},
        {"ANSI_1251", "\xC0", "\xD0\x90"},
        {"ANSI_1252", "\xE9", "\xC3\xA9"},
        {"ANSI_1253", "\xC1", "\xCE\x91"},
        {"ANSI_1254", "\xD0", "\xC4\x9E"},
        {"ANSI_1255", "\xE0", "\xD7\x90"},
        {"ANSI_1256", "\xC7", "\xD8\xA7"},
        {"ANSI_1257", "\xC0", "\xC4\x84"},
        {"ANSI_1258", "\xD5", "\xC6\xA0"}
    };
    std::size_t canonicalCases = 0, escapeCases = 0, modernCases = 0;
    for (bool dxf : {false, true}) {
        for (const auto& row : canonical) {
            DRW_TextCodec codec;
            codec.setVersion(DRW::AC1015, dxf);
            codec.setCodePage(row.name, dxf);
            const std::string bytes = std::string("L") + row.bytes + "R";
            const std::string utf8 = std::string("L") + row.utf8 + "R";
            const std::string label = std::string("canonical codec byte/scalar/encode anchor: ") + row.name;
            t.expect(codec.getCodePage() == row.name
                         && codec.toUtf8(bytes) == utf8
                         && codec.fromUtf8(utf8) == bytes, label.c_str());
            ++canonicalCases;
        }
        struct EscapeAnchor {
            const char* bytes;
            const char* utf8;
            const char* emitted;
        };
        const EscapeAnchor hkscs[] = {
            {"\x88\x62", "\xC3\x8A\xCC\x84", "\x88\x66" R"(\U+0304)"},
            {"\x88\x64", "\xC3\x8A\xCC\x8C", "\x88\x66" R"(\U+030C)"},
            {"\x88\xA3", "\xC3\xAA\xCC\x84", "\x88\xA7" R"(\U+0304)"},
            {"\x88\xA5", "\xC3\xAA\xCC\x8C", "\x88\xA7" R"(\U+030C)"}
        };
        for (const auto& row : hkscs) {
            DRW_TextCodec codec;
            codec.setVersion(DRW::AC1015, dxf);
            codec.setCodePage("BIG5-HKSCS", dxf);
            const std::string decoded = codec.toUtf8(row.bytes);
            const std::string emitted = codec.fromUtf8(row.utf8);
            t.expect(decoded == row.utf8 && emitted == row.emitted
                         && emitted != row.bytes && codec.toUtf8(emitted) == row.utf8,
                     "HKSCS two-scalar CIF transport preserves meaning without byte identity");
            ++escapeCases;
        }
    }
    for (DRW::Version version : {DRW::AC1021, DRW::AC1032}) {
        for (const char* declaration : {"ANSI_1252", "ANSI_949", "unknown-local-test-codepage"}) {
            DRW_TextCodec codec;
            codec.setVersion(version, true);
            codec.setCodePage(declaration, true);
            const std::string utf8 = "\xC3\xA9\xE4\xB8\xAD";
            t.expect(codec.toUtf8(utf8) == utf8 && codec.fromUtf8(utf8) == utf8,
                     "modern DXF stays UTF8 independently of legacy codepage metadata");
            ++modernCases;
        }
    }
    std::cout << "Codepage policy anchors: canonical=" << canonicalCases
              << ", two-scalar escapes=" << escapeCases
              << ", modern UTF8=" << modernCases << '\n';
}

void testMacRomanCodePage(TestContext& t) {
    // Apple’s Unicode vendor mapping ROMAN.TXT, bytes 0x80..0xFF. These
    // expectations are independent of DRW_TableMacRoman.
    static constexpr int upper[] = {
        0x00C4, 0x00C5, 0x00C7, 0x00C9, 0x00D1, 0x00D6, 0x00DC, 0x00E1,
        0x00E0, 0x00E2, 0x00E4, 0x00E3, 0x00E5, 0x00E7, 0x00E9, 0x00E8,
        0x00EA, 0x00EB, 0x00ED, 0x00EC, 0x00EE, 0x00EF, 0x00F1, 0x00F3,
        0x00F2, 0x00F4, 0x00F6, 0x00F5, 0x00FA, 0x00F9, 0x00FB, 0x00FC,
        0x2020, 0x00B0, 0x00A2, 0x00A3, 0x00A7, 0x2022, 0x00B6, 0x00DF,
        0x00AE, 0x00A9, 0x2122, 0x00B4, 0x00A8, 0x2260, 0x00C6, 0x00D8,
        0x221E, 0x00B1, 0x2264, 0x2265, 0x00A5, 0x00B5, 0x2202, 0x2211,
        0x220F, 0x03C0, 0x222B, 0x00AA, 0x00BA, 0x03A9, 0x00E6, 0x00F8,
        0x00BF, 0x00A1, 0x00AC, 0x221A, 0x0192, 0x2248, 0x2206, 0x00AB,
        0x00BB, 0x2026, 0x00A0, 0x00C0, 0x00C3, 0x00D5, 0x0152, 0x0153,
        0x2013, 0x2014, 0x201C, 0x201D, 0x2018, 0x2019, 0x00F7, 0x25CA,
        0x00FF, 0x0178, 0x2044, 0x20AC, 0x2039, 0x203A, 0xFB01, 0xFB02,
        0x2021, 0x00B7, 0x201A, 0x201E, 0x2030, 0x00C2, 0x00CA, 0x00C1,
        0x00CB, 0x00C8, 0x00CD, 0x00CE, 0x00CF, 0x00CC, 0x00D3, 0x00D4,
        0xF8FF, 0x00D2, 0x00DA, 0x00DB, 0x00D9, 0x0131, 0x02C6, 0x02DC,
        0x00AF, 0x02D8, 0x02D9, 0x02DA, 0x00B8, 0x02DD, 0x02DB, 0x02C7
    };
    static_assert(sizeof(upper) / sizeof(upper[0]) == 128,
                  "MacRoman expected mapping spans every upper byte");

    std::size_t byteScalarPairs = 0;
    for (bool dxf : {false, true}) {
        for (const char* alias : {"MACINTOSH", "APPLE ROMAN"}) {
            DRW_TextCodec codec;
            codec.setVersion(DRW::AC1015, dxf);
            codec.setCodePage(alias, dxf);
            const auto selection = DRW_TextCodec::classifyCodePage(alias);
            const auto resolution = DRW_TextCodec::resolveCodePage(
                alias, DRW_CodePageMappingPolicy::ExactNamed);
            t.expect(codec.getCodePage() == "MACINTOSH"
                         && selection.selectedName != nullptr
                         && std::string(selection.selectedName) == "MACINTOSH"
                         && resolution.status
                                == DRW_CodePageMappingStatus::ExactImplementationAvailable
                         && resolution.decoderName != nullptr
                         && std::string(resolution.decoderName) == "MACINTOSH",
                     "Macintosh labels resolve to the exact MacRoman codec");
            for (unsigned int byte = 0x80u; byte <= 0xFFu; ++byte) {
                const std::string wire(
                    1u, static_cast<char>(static_cast<unsigned char>(byte)));
                const std::string expected =
                    unicodeScalarExpectedUtf8(upper[byte - 0x80u]);
                t.expect(codec.toUtf8(wire) == expected
                             && codec.lastDecodeObservation().issues == 0
                             && codec.lastDecodeObservation().nonAsciiBytes
                             && codec.fromUtf8(expected) == wire,
                         "MacRoman upper-byte mapping decodes and inverse-encodes exactly");
                ++byteScalarPairs;
            }
        }
    }
    t.expect(byteScalarPairs == 512,
             "both MacRoman labels cover every upper byte on DWG and DXF codec paths");
    DRW_TextCodec modernDwg;
    modernDwg.setVersion(DRW::AC1021, false);
    modernDwg.setByteCodePage("APPLE ROMAN");
    t.expect(modernDwg.getCodePage() == "UTF-16"
                 && modernDwg.toUtf8(std::string("A\0", 2u)) == "A"
                 && modernDwg.toUtf8CP8("\x80") == "\xC3\x84",
             "R2007+ DWG keeps its UTF16 and separate MacRoman byte-string channels");

    DRW_TextCodec modernDxf;
    modernDxf.setVersion(DRW::AC1021, true);
    modernDxf.setCodePage("APPLE ROMAN", true);
    const std::string utf8 = "\xC3\x84\xE2\x82\xAC";
    t.expect(modernDxf.getCodePage() == "MACINTOSH"
                 && modernDxf.toUtf8(utf8) == utf8
                 && modernDxf.fromUtf8(utf8) == utf8,
             "R2007+ DXF stays UTF8 while preserving MacRoman as inert legacy metadata");
}

std::size_t auditNamedCodePageAliases(TestContext& t, const std::string& filter) {
    // Opt-in discriminating audit, not a mandatory test or a claim that
    // every historical/extra alias is a supported format declaration.
    // Expected standard-codec meaning is frozen independently of this codec.
    // Keep this mode red until the corresponding mapping/policy is resolved.
    const CodePageAnchor rows[] = {
        {"ANSI_1252", "\x80", "\xE2\x82\xAC"},
        {"LATIN1", "\x80", "\xC2\x80"},
        {"ISO-8859-1", "\x80", "\xC2\x80"},
        {"ISO8859-2", "\xA1", "\xC4\x84"},
        {"ISO8859-3", "\xA1", "\xC4\xA6"},
        {"ISO8859-4", "\xA1", "\xC4\x84"},
        {"ISO8859-5", "\xB0", "\xD0\x90"},
        {"KOI8-R", "\xE1", "\xD0\x90"},
        {"KOI8-U", "\xE1", "\xD0\x90"},
        {"IBM 866", "\x80", "\xD0\x90"},
        {"CP866", "\x80", "\xD0\x90"},
        {"IBM 850", "\x80", "\xC3\x87"},
        {"CP850", "\x80", "\xC3\x87"},
        {"APPLE ROMAN", "\x80", "\xC3\x84"},
        {"MACINTOSH", "\x80", "\xC3\x84"},
        {"ISO8859-6", "\xAC", "\xD8\x8C"},
        {"ISO8859-7", "\xA1", "\xE2\x80\x98"},
        {"ISO8859-8", "\xE0", "\xD7\x90"},
        {"ISO8859-9", "\xD0", "\xC4\x9E"},
        {"ISO8859-10", "\xA1", "\xC4\x84"},
        {"ISO8859-13", "\xA1", "\xE2\x80\x9D"},
        {"ISO8859-15", "\xA4", "\xE2\x82\xAC"},
        {"ISO8859-8", "\xDF", "\xE2\x80\x97"},
        {"ISO8859-9", "\x80", "\xC2\x80"},
        {"EUC-JP", "\xA4\xA2", "\xE3\x81\x82"},
        {"ANSI_932", "\x82\xA0", "\xE3\x81\x82"},
        {"GB18030", "\x81\x30\x81\x30", "\xC2\x80"},
        {"ANSI_936", "\xD6\xD0", "\xE4\xB8\xAD"},
        {"ANSI_949", "\xB0\xA1", "\xEA\xB0\x80"},
        {"ansi_949", "\xB0\xA1", "\xEA\xB0\x80"},
        {"BIG5-HKSCS", "\x88\x62", "\xC3\x8A\xCC\x84"},
        {"ANSI_950", "\xA4\xA4", "\xE4\xB8\xAD"},
        {"BIG5-HKSCS", "\x88\x64", "\xC3\x8A\xCC\x8C"},
        {"BIG5-HKSCS", "\x88\xA3", "\xC3\xAA\xCC\x84"},
        {"BIG5-HKSCS", "\x88\xA5", "\xC3\xAA\xCC\x8C"},
    };
    std::size_t cases = 0, byteStableWrong = 0, empty = 0;
    for (bool dxf : {false, true}) {
        for (const auto& row : rows) {
            if (!filter.empty() && filter != row.name)
                continue;
            DRW_TextCodec codec;
            codec.setVersion(DRW::AC1015, dxf);
            codec.setCodePage(row.name, dxf);
            const std::string decoded = codec.toUtf8(row.bytes);
            const std::string label = std::string("named-codec semantics: ") + row.name;
            t.expect(decoded == row.utf8, label.c_str());
            if (decoded != row.utf8 && codec.fromUtf8(decoded) == row.bytes)
                ++byteStableWrong;
            if (decoded.empty())
                ++empty;
            ++cases;
        }
    }
    std::cout << "Named-codec alias audit: cases=" << cases
              << ", mismatches=" << t.failures
              << ", byte-stable wrong=" << byteStableWrong
              << ", empty=" << empty << '\n';
    return cases;
}


void testCodePageSelectionMetadata(TestContext& t) {
    using Kind = DRW_CodePageKind;
    struct Row { const char* source; Kind kind; const char* selected; };
    const Row rows[] = {
        {"ANSI_1252", Kind::Canonical, "ANSI_1252"},
        {"ansi_949", Kind::Canonical, "ANSI_949"},
        {"CP1252", Kind::ExactAlias, "ANSI_1252"},
        {"MACINTOSH", Kind::Canonical, "MACINTOSH"},
        {"APPLE ROMAN", Kind::ExactAlias, "MACINTOSH"},
        {"ISO8859-3", Kind::CompatibilityAlias, "ANSI_1254"},
        {"ISO-8859-3", Kind::Unavailable, "ANSI_1252"},
        {"ISO8859-5", Kind::CompatibilityAlias, "ANSI_1251"},
        {"ISO-8859-5", Kind::Unavailable, "ANSI_1252"},
        {"ISO8859-6", Kind::CompatibilityAlias, "ANSI_1256"},
        {"ISO-8859-6", Kind::Unavailable, "ANSI_1252"},
        {"ISO8859-7", Kind::CompatibilityAlias, "ANSI_1253"},
        {"ISO-8859-7", Kind::Unavailable, "ANSI_1252"},
        {"ISO8859-8", Kind::CompatibilityAlias, "ANSI_1255"},
        {"ISO-8859-8", Kind::Unavailable, "ANSI_1252"},
        {"ISO8859-9", Kind::CompatibilityAlias, "ANSI_1254"},
        {"ISO-8859-9", Kind::Unavailable, "ANSI_1252"},
        {"ISO8859-10", Kind::CompatibilityAlias, "ANSI_1257"},
        {"ISO-8859-10", Kind::Unavailable, "ANSI_1252"},
        {"ISO8859-11", Kind::CompatibilityAlias, "ANSI_874"},
        {"ISO-8859-11", Kind::Unknown, "ANSI_1252"},
        {"ISO8859-13", Kind::CompatibilityAlias, "ANSI_1257"},
        {"ISO-8859-13", Kind::Unavailable, "ANSI_1252"},
        {"ISO8859-14", Kind::Unavailable, "ANSI_1252"},
        {"ISO8859-15", Kind::CompatibilityAlias, "ANSI_1252"},
        {"ISO-8859-15", Kind::Unavailable, "ANSI_1252"},
        {"gbk", Kind::CompatibilityAlias, "ANSI_936"},
        {"UTF8", Kind::ExactAlias, "UTF-8"},
        {"UTF-16", Kind::Canonical, "UTF-16"},
        {"ISO8859-4", Kind::CompatibilityAlias, "ANSI_1257"},
        {"KOI8-R", Kind::CompatibilityAlias, "ANSI_1251"},
        {"GB18030", Kind::CompatibilityAlias, "ANSI_936"},
        {"EUC-JP", Kind::CompatibilityAlias, "ANSI_932"},
        {"BIG5-HKSCS", Kind::CompatibilityAlias, "ANSI_950"},
        {"CP866", Kind::Unavailable, "ANSI_1252"},
        {"ISO-8859-4", Kind::Unavailable, "ANSI_1252"},
        {"JOHAB", Kind::Unavailable, "ANSI_1252"},
        {"unknown-local-test-codepage", Kind::Unknown, "ANSI_1252"},
        {"", Kind::Unknown, "ANSI_1252"},
        {"ANSI_1252 ", Kind::Unknown, "ANSI_1252"},
        {"\xE9", Kind::Unknown, "ANSI_1252"}
    };
    for (const auto& row : rows) {
        const auto selected = DRW_TextCodec::classifyCodePage(row.source);
        t.expect(selected.kind == row.kind && selected.selectedName != nullptr
                     && std::string(selected.selectedName) == row.selected,
                 "classifier distinguishes canonical/exact/inexact/unavailable/unknown names");
    }
    const std::string nulName("ANSI_1252\0X", 11);
    t.expect(DRW_TextCodec::classifyCodePage(nulName).kind == Kind::Unknown,
             "embedded NUL cannot truncate a declared page to a valid prefix");

    // Frozen pre-change selector transcription for unchanged aliases, not a
    // semantic Unicode oracle. A mapping correction such as APPLE ROMAN is
    // tested separately; never interpret this baseline as named-codec support.
    struct LegacySelection { const char* source; const char* selected; };
    const LegacySelection legacy[] = {
        {"ANSI_874", "ANSI_874"},
        {"CP874", "ANSI_874"},
        {"ISO8859-11", "ANSI_874"},
        {"TIS-620", "ANSI_874"},
        {"ANSI_1250", "ANSI_1250"},
        {"CP1250", "ANSI_1250"},
        {"ISO8859-2", "ANSI_1250"},
        {"ANSI_1251", "ANSI_1251"},
        {"CP1251", "ANSI_1251"},
        {"ISO8859-5", "ANSI_1251"},
        {"KOI8-R", "ANSI_1251"},
        {"KOI8-U", "ANSI_1251"},
        {"IBM 866", "ANSI_1251"},
        {"ANSI_1252", "ANSI_1252"},
        {"CP1252", "ANSI_1252"},
        {"LATIN1", "ANSI_1252"},
        {"ISO-8859-1", "ANSI_1252"},
        {"CP819", "ANSI_1252"},
        {"CSISO", "ANSI_1252"},
        {"IBM819", "ANSI_1252"},
        {"ISO_8859-1", "ANSI_1252"},
        {"ISO8859-1", "ANSI_1252"},
        {"ISO8859-15", "ANSI_1252"},
        {"ISO-IR-100", "ANSI_1252"},
        {"L1", "ANSI_1252"},
        {"IBM 850", "ANSI_1252"},
        {"ANSI_1253", "ANSI_1253"},
        {"CP1253", "ANSI_1253"},
        {"ISO8859-7", "ANSI_1253"},
        {"ISO-8859-7", "ANSI_1252"},
        {"ANSI_1254", "ANSI_1254"},
        {"CP1254", "ANSI_1254"},
        {"ISO8859-9", "ANSI_1254"},
        {"ISO-8859-9", "ANSI_1252"},
        {"ISO8859-10", "ANSI_1257"},
        {"ISO-8859-10", "ANSI_1252"},
        {"ISO8859-3", "ANSI_1254"},
        {"ANSI_1255", "ANSI_1255"},
        {"CP1255", "ANSI_1255"},
        {"ISO8859-8", "ANSI_1255"},
        {"ISO-8859-8", "ANSI_1252"},
        {"ANSI_1256", "ANSI_1256"},
        {"CP1256", "ANSI_1256"},
        {"ISO8859-6", "ANSI_1256"},
        {"ISO-8859-6", "ANSI_1252"},
        {"ANSI_1257", "ANSI_1257"},
        {"CP1257", "ANSI_1257"},
        {"ISO8859-4", "ANSI_1257"},
        {"ISO8859-10", "ANSI_1257"},
        {"ISO8859-13", "ANSI_1257"},
        {"ANSI_1258", "ANSI_1258"},
        {"CP1258", "ANSI_1258"},
        {"ANSI_932", "ANSI_932"},
        {"SHIFT-JIS", "ANSI_932"},
        {"SHIFT_JIS", "ANSI_932"},
        {"CSSHIFTJIS", "ANSI_932"},
        {"CSWINDOWS31J", "ANSI_932"},
        {"MS_KANJI", "ANSI_932"},
        {"X-MS-CP932", "ANSI_932"},
        {"X-SJIS", "ANSI_932"},
        {"EUCJP", "ANSI_932"},
        {"EUC-JP", "ANSI_932"},
        {"CSEUCPKDFMTJAPANESE", "ANSI_932"},
        {"X-EUC", "ANSI_932"},
        {"X-EUC-JP", "ANSI_932"},
        {"JIS7", "ANSI_932"},
        {"ANSI_936", "ANSI_936"},
        {"CP936", "ANSI_936"},
        {"GBK", "ANSI_936"},
        {"GB2312", "ANSI_936"},
        {"CHINESE", "ANSI_936"},
        {"CN-GB", "ANSI_936"},
        {"CSGB2312", "ANSI_936"},
        {"CSGB231280", "ANSI_936"},
        {"CSISO58BG231280", "ANSI_936"},
        {"GB_2312-80", "ANSI_936"},
        {"GB231280", "ANSI_936"},
        {"GB2312-80", "ANSI_936"},
        {"ISO-IR-58", "ANSI_936"},
        {"GB18030", "ANSI_936"},
        {"ANSI_949", "ANSI_949"},
        {"EUCKR", "ANSI_949"},
        {"ANSI_950", "ANSI_950"},
        {"BIG5", "ANSI_950"},
        {"CN-BIG5", "ANSI_950"},
        {"CSBIG5", "ANSI_950"},
        {"X-X-BIG5", "ANSI_950"},
        {"BIG5-HKSCS", "ANSI_950"},
        {"UTF-8", "UTF-8"},
        {"UTF8", "UTF-8"},
        {"UTF8-BIT", "UTF-8"},
        {"UTF-16", "UTF-16"},
        {"UTF16", "UTF-16"},
        {"UTF16-BIT", "UTF-16"},
    };
    std::size_t compatibilityCases = 0;
    for (const auto& row : legacy) for (bool dxf : {false, true}) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1015, dxf);
        codec.setCodePage(row.source, dxf);
        const char* headerName = std::string(row.selected) == "UTF-8"
            ? "ANSI_1252" : row.selected;
        t.expect(codec.getCodePage() == headerName,
                 "every pre-change accepted alias retains its compatibility selection");
        ++compatibilityCases;
    }

    DRW_TextCodec codec;
    t.expect(codec.declaredCodePageSelection().kind == Kind::Unspecified,
             "unconfigured codec has no explicit declaration");
    codec.setVersion(DRW::AC1015, true);
    t.expect(codec.declaredCodePageSelection().kind == Kind::Unspecified,
             "version-selected default is not a source declaration");
    codec.setCodePage("KOI8-R", true);
    codec.setVersion(DRW::AC1015, true);
    codec.reapplyCodePage("ANSI_1251", true);
    t.expect(codec.declaredCodePageSelection().kind == Kind::CompatibilityAlias
                 && std::string(codec.declaredCodePageSelection().selectedName) == "ANSI_1251"
                 && codec.getCodePage() == "ANSI_1251",
             "version reset and normalized HEADER reapply preserve declaration category");
    codec.setByteCodePage("CP866");
    t.expect(codec.byteCodePageSelection().kind == Kind::Unavailable
                 && codec.declaredCodePageSelection().kind == Kind::CompatibilityAlias,
             "secondary byte selection does not replace primary declaration metadata");
    DRW_TextCodec other;
    other.setVersion(DRW::AC1015, true);
    other.setCodePage("CP936", true);
    other.setByteCodePage("ANSI_936");
    codec.swap(other);
    t.expect(codec.declaredCodePageSelection().kind == Kind::ExactAlias
                 && codec.byteCodePageSelection().kind == Kind::Canonical
                 && other.declaredCodePageSelection().kind == Kind::CompatibilityAlias
                 && other.byteCodePageSelection().kind == Kind::Unavailable
                 && codec.toUtf8CP8("\xD6\xD0") == "\xE4\xB8\xAD",
             "swap transfers primary/byte classification with their converters");
    DRW_TextCodec moved(std::move(other));
    t.expect(moved.declaredCodePageSelection().kind == Kind::CompatibilityAlias
                 && moved.byteCodePageSelection().kind == Kind::Unavailable
                 && moved.getCodePage() == "ANSI_1251",
             "move retains selection metadata without borrowing source strings");
    const auto temporary = DRW_TextCodec::classifyCodePage(std::string("koi8-r"));
    t.expect(std::string(temporary.selectedName) == "ANSI_1251",
             "classification retains only static canonical spelling after source destruction");
    DRW_TextCodec recovery;
    recovery.setDxfTextEncoding(DRW::DxfTextEncoding::LegacyLibreCadUtf8);
    recovery.setCodePage("UTF8", true);
    recovery.setVersion(DRW::AC1015, true);
    recovery.reapplyCodePage("ANSI_1252", true);
    t.expect(recovery.declaredCodePageSelection().kind == Kind::ExactAlias
                 && std::string(recovery.declaredCodePageSelection().selectedName) == "UTF-8"
                 && recovery.getCodePage() == "ANSI_1252"
                 && recovery.toUtf8("\xC3\xA9") == "\xC3\xA9"
                 && recovery.fromUtf8("\xC3\xA9") == "\xE9",
             "normalized UTF8 declaration keeps historical input-only recovery and export encoding");
    std::cout << "Codec selection metadata: legacy selection cases=" << compatibilityCases << '\n';
}

void testExactCodePageResolution(TestContext& t) {
    using Policy = DRW_CodePageMappingPolicy;
    using Status = DRW_CodePageMappingStatus;
    using Kind = DRW_CodePageKind;
    struct Row { const char* spelling; const char* requested; const char* historical; Kind kind; };
    const Row available[] = {
        {"ANSI_1252", "ANSI_1252", "ANSI_1252", Kind::Canonical},
        {"cp1252", "ANSI_1252", "ANSI_1252", Kind::ExactAlias},
        {"ANSI_1257", "ANSI_1257", "ANSI_1257", Kind::Canonical},
        {"CP1257", "ANSI_1257", "ANSI_1257", Kind::ExactAlias},
        {"MACINTOSH", "MACINTOSH", "MACINTOSH", Kind::Canonical},
        {"APPLE ROMAN", "MACINTOSH", "MACINTOSH", Kind::ExactAlias},
        {"ansi_949", "ANSI_949", "ANSI_949", Kind::Canonical},
        {"CSWINDOWS31J", "ANSI_932", "ANSI_932", Kind::ExactAlias},
        {"X-MS-CP932", "ANSI_932", "ANSI_932", Kind::ExactAlias},
        {"UTF8", "UTF-8", "UTF-8", Kind::ExactAlias},
        {"UTF-16", "UTF-16", "UTF-16", Kind::Canonical}
    };
    const Row isoAvailable[] = {
        {"ISO8859-4", "ISO8859-4", "ANSI_1257", Kind::CompatibilityAlias},
        {"iso-8859-4", "ISO8859-4", "ANSI_1252", Kind::Unavailable},
    };
    const Row iso2Available[] = {
        {"ISO8859-2", "ISO8859-2", "ANSI_1250", Kind::CompatibilityAlias},
        {"iso-8859-2", "ISO8859-2", "ANSI_1252", Kind::Unavailable},
    };
    const Row iso3Available[] = {
        {"ISO8859-3", "ISO8859-3", "ANSI_1254", Kind::CompatibilityAlias},
        {"iso-8859-3", "ISO8859-3", "ANSI_1252", Kind::Unavailable},
    };
    const Row iso5Available[] = {
        {"ISO8859-5", "ISO8859-5", "ANSI_1251", Kind::CompatibilityAlias},
        {"iso-8859-5", "ISO8859-5", "ANSI_1252", Kind::Unavailable},
    };
    const Row iso6Available[] = {
        {"ISO8859-6", "ISO8859-6", "ANSI_1256", Kind::CompatibilityAlias},
        {"iso-8859-6", "ISO8859-6", "ANSI_1252", Kind::Unavailable},
    };
    const Row iso7Available[] = {
        {"ISO8859-7", "ISO8859-7", "ANSI_1253", Kind::CompatibilityAlias},
        {"iso-8859-7", "ISO8859-7", "ANSI_1252", Kind::Unavailable},
    };
    const Row iso8Available[] = {
        {"ISO8859-8", "ISO8859-8", "ANSI_1255", Kind::CompatibilityAlias},
        {"iso-8859-8", "ISO8859-8", "ANSI_1252", Kind::Unavailable},
    };
    const Row iso9Available[] = {
        {"ISO8859-9", "ISO8859-9", "ANSI_1254", Kind::CompatibilityAlias},
        {"iso-8859-9", "ISO8859-9", "ANSI_1252", Kind::Unavailable},
    };
    const Row iso10Available[] = {
        {"ISO8859-10", "ISO8859-10", "ANSI_1257", Kind::CompatibilityAlias},
        {"iso-8859-10", "ISO8859-10", "ANSI_1252", Kind::Unavailable},
    };
    const Row iso11Available[] = {
        {"ISO8859-11", "ISO8859-11", "ANSI_874", Kind::CompatibilityAlias},
        {"iso8859-11", "ISO8859-11", "ANSI_874", Kind::CompatibilityAlias},
    };
    const Row iso13Available[] = {
        {"ISO8859-13", "ISO8859-13", "ANSI_1257", Kind::CompatibilityAlias},
        {"iso8859-13", "ISO8859-13", "ANSI_1257", Kind::CompatibilityAlias},
        {"ISO-8859-13", "ISO8859-13", "ANSI_1252", Kind::Unavailable},
        {"iso-8859-13", "ISO8859-13", "ANSI_1252", Kind::Unavailable},
    };
    const Row iso14Available[] = {
        {"ISO8859-14", "ISO8859-14", "ANSI_1252", Kind::Unavailable},
        {"iso8859-14", "ISO8859-14", "ANSI_1252", Kind::Unavailable},
    };
    const Row iso15Available[] = {
        {"ISO8859-15", "ISO8859-15", "ANSI_1252", Kind::CompatibilityAlias},
        {"iso8859-15", "ISO8859-15", "ANSI_1252", Kind::CompatibilityAlias},
        {"ISO-8859-15", "ISO8859-15", "ANSI_1252", Kind::Unavailable},
        {"iso-8859-15", "ISO8859-15", "ANSI_1252", Kind::Unavailable},
    };
    const Row latin1Available[] = {
        {"LATIN1", "ISO8859-1", "ANSI_1252", Kind::CompatibilityAlias},
        {"ISO8859-1", "ISO8859-1", "ANSI_1252", Kind::CompatibilityAlias},
        {"ISO-8859-1", "ISO8859-1", "ANSI_1252", Kind::CompatibilityAlias},
        {"ISO_8859-1", "ISO8859-1", "ANSI_1252", Kind::CompatibilityAlias},
        {"CP819", "ISO8859-1", "ANSI_1252", Kind::CompatibilityAlias},
        {"IBM819", "ISO8859-1", "ANSI_1252", Kind::CompatibilityAlias},
        {"ISO-IR-100", "ISO8859-1", "ANSI_1252", Kind::CompatibilityAlias},
        {"L1", "ISO8859-1", "ANSI_1252", Kind::CompatibilityAlias}
    };
    const Row unavailable[] = {
        {"TIS-620", "TIS-620", "ANSI_874", Kind::CompatibilityAlias},
        {"KOI8-R", "KOI8-R", "ANSI_1251", Kind::CompatibilityAlias},
        {"KOI8-U", "KOI8-U", "ANSI_1251", Kind::CompatibilityAlias},
        {"IBM 866", "IBM866", "ANSI_1251", Kind::CompatibilityAlias},
        {"CP866", "IBM866", "ANSI_1252", Kind::Unavailable},
        {"IBM 850", "IBM850", "ANSI_1252", Kind::CompatibilityAlias},
        {"CP850", "IBM850", "ANSI_1252", Kind::Unavailable},
        {"EUC-JP", "EUC-JP", "ANSI_932", Kind::CompatibilityAlias},
        {"csEUCPkdFmtJapanese", "EUC-JP", "ANSI_932", Kind::CompatibilityAlias},
        {"SHIFT_JIS", "SHIFT_JIS", "ANSI_932", Kind::CompatibilityAlias},
        {"MS_KANJI", "SHIFT_JIS", "ANSI_932", Kind::CompatibilityAlias},
        {"GB18030", "GB18030", "ANSI_936", Kind::CompatibilityAlias},
        {"GBK", "GBK", "ANSI_936", Kind::CompatibilityAlias},
        {"GB2312", "GB2312", "ANSI_936", Kind::CompatibilityAlias},
        {"EUCKR", "EUC-KR", "ANSI_949", Kind::CompatibilityAlias},
        {"BIG5", "BIG5", "ANSI_950", Kind::CompatibilityAlias},
        {"BIG5-HKSCS", "BIG5-HKSCS", "ANSI_950", Kind::CompatibilityAlias},
        {"ANSI_1361", "JOHAB", "ANSI_1252", Kind::Unavailable},
        {"TSCII", "TSCII", "ANSI_1252", Kind::Unavailable},
        // Historical accepted but ambiguous/vendor-only labels retain their
        // selection without inventing a qualified requested mapping family.
        {"CSISO", nullptr, "ANSI_1252", Kind::CompatibilityAlias},
        {"X-EUC", nullptr, "ANSI_932", Kind::CompatibilityAlias},
        {"X-X-BIG5", nullptr, "ANSI_950", Kind::CompatibilityAlias}
    };
    const auto matches = [](const char* value, const char* expected) {
        return value == nullptr || expected == nullptr ? value == expected
            : std::string_view(value) == expected;
    };
    std::size_t selections = 0;
    const auto check = [&](const Row& row, bool exactAvailable, const char* exactName = nullptr) {
        for (auto policy : {Policy::Compatible, Policy::ExactNamed}) {
            const auto result = DRW_TextCodec::resolveCodePage(row.spelling, policy);
            const auto expectedStatus = policy == Policy::Compatible ? Status::CompatibleSelection
                : exactAvailable ? Status::ExactImplementationAvailable : Status::Unavailable;
            t.expect(matches(result.requestedName, row.requested)
                         && matches(result.declaration.selectedName, row.historical)
                         && result.declaration.kind == row.kind && result.status == expectedStatus
                         && matches(result.decoderName,
                             policy == Policy::Compatible ? row.historical
                                 : exactAvailable ? (exactName == nullptr ? row.historical : exactName) : nullptr),
                     "exact resolution preserves provenance and never substitutes a similar Windows mapping");
            ++selections;
        }
    };
    for (const auto& row : available) check(row, true);
    for (const auto& row : isoAvailable) check(row, true, "ISO8859-4");
    for (const auto& row : iso2Available) check(row, true, "ISO8859-2");
    for (const auto& row : iso3Available) check(row, true, "ISO8859-3");
    for (const auto& row : iso5Available) check(row, true, "ISO8859-5");
    for (const auto& row : iso6Available) check(row, true, "ISO8859-6");
    for (const auto& row : iso7Available) check(row, true, "ISO8859-7");
    for (const auto& row : iso8Available) check(row, true, "ISO8859-8");
    for (const auto& row : iso9Available) check(row, true, "ISO8859-9");
    for (const auto& row : iso10Available) check(row, true, "ISO8859-10");
    for (const auto& row : iso11Available) check(row, true, "ISO8859-11");
    for (const auto& row : iso13Available) check(row, true, "ISO8859-13");
    for (const auto& row : iso14Available) check(row, true, "ISO8859-14");
    for (const auto& row : iso15Available) check(row, true, "ISO8859-15");
    for (const auto& row : latin1Available) check(row, true, "ISO8859-1");
    for (const auto& row : unavailable) check(row, false);
    const std::string names[] = {"", "unknown-authored-page", "ANSI_1252 ",
        "ISO8859-4/UTF8", "\xE9", std::string("ISO8859-4\0X", 11)};
    for (const auto& name : names) {
        const auto result = DRW_TextCodec::resolveCodePage(name, Policy::ExactNamed);
        t.expect(result.status == Status::Unknown && result.requestedName == nullptr
                     && result.decoderName == nullptr && result.declaration.kind == Kind::Unknown
                     && matches(result.declaration.selectedName, "ANSI_1252"),
                 "unknown empty contaminated or NUL-containing declarations cannot become exact prefixes");
    }
    const auto invalidPolicy = DRW_TextCodec::resolveCodePage("ANSI_1252", static_cast<Policy>(255));
    t.expect(invalidPolicy.decoderName == nullptr && invalidPolicy.status == Status::Unknown,
             "an unknown policy cannot silently select a decoder");
    const auto owned = DRW_TextCodec::resolveCodePage(std::string("iso-8859-4"), Policy::ExactNamed);
    t.expect(matches(owned.requestedName, "ISO8859-4") && matches(owned.decoderName, "ISO8859-4"),
             "resolved identity survives destruction of the caller spelling without borrowed storage");
    bool allocationFree = true;
    failImportObservationAllocations = true;
    for (const auto& row : available) for (auto policy : {Policy::Compatible, Policy::ExactNamed})
        allocationFree &= DRW_TextCodec::resolveCodePage(row.spelling, policy).decoderName != nullptr;
    for (const auto& row : isoAvailable)
        allocationFree &= DRW_TextCodec::resolveCodePage(row.spelling, Policy::ExactNamed).decoderName != nullptr;
    for (const auto& row : iso2Available)
        allocationFree &= DRW_TextCodec::resolveCodePage(row.spelling, Policy::ExactNamed).decoderName != nullptr;
    for (const auto& row : iso3Available)
        allocationFree &= DRW_TextCodec::resolveCodePage(row.spelling, Policy::ExactNamed).decoderName != nullptr;
    for (const auto& row : iso5Available)
        allocationFree &= DRW_TextCodec::resolveCodePage(row.spelling, Policy::ExactNamed).decoderName != nullptr;
    for (const auto& row : iso6Available)
        allocationFree &= DRW_TextCodec::resolveCodePage(row.spelling, Policy::ExactNamed).decoderName != nullptr;
    for (const auto& row : iso7Available)
        allocationFree &= DRW_TextCodec::resolveCodePage(row.spelling, Policy::ExactNamed).decoderName != nullptr;
    for (const auto& row : iso8Available)
        allocationFree &= DRW_TextCodec::resolveCodePage(row.spelling, Policy::ExactNamed).decoderName != nullptr;
    for (const auto& row : iso11Available)
        allocationFree &= DRW_TextCodec::resolveCodePage(row.spelling, Policy::ExactNamed).decoderName != nullptr;
    for (const auto& row : iso13Available)
        allocationFree &= DRW_TextCodec::resolveCodePage(row.spelling, Policy::ExactNamed).decoderName != nullptr;
    for (const auto& row : iso14Available)
        allocationFree &= DRW_TextCodec::resolveCodePage(row.spelling, Policy::ExactNamed).decoderName != nullptr;
    for (const auto& row : iso15Available)
        allocationFree &= DRW_TextCodec::resolveCodePage(row.spelling, Policy::ExactNamed).decoderName != nullptr;
    for (const auto& row : latin1Available)
        allocationFree &= DRW_TextCodec::resolveCodePage(row.spelling, Policy::ExactNamed).decoderName != nullptr;
    for (const auto& row : unavailable)
        allocationFree &= DRW_TextCodec::resolveCodePage(row.spelling, Policy::ExactNamed).decoderName == nullptr;
    failImportObservationAllocations = false;
    t.expect(allocationFree, "resolution remains allocation-free even under forced allocator failure");
    std::cout << "Exact codepage resolution: selections=" << selections
              << ", unknown controls=6, invalid-policy/ownership/allocation controls=3\n";
}

std::string unicodeScalarExpectedUtf8(int scalar) {
    if (scalar < 0x80) return std::string(1, static_cast<char>(scalar));
    // Every scalar in the independently transcribed ISO8859-4/CP1257
    // expectations fits two/three UTF8 bytes; no production encoder is used.
    if (scalar < 0x800)
        return {static_cast<char>(0xC0 | (scalar >> 6)), static_cast<char>(0x80 | (scalar & 0x3F))};
    return {static_cast<char>(0xE0 | (scalar >> 12)), static_cast<char>(0x80 | ((scalar >> 6) & 0x3F)),
            static_cast<char>(0x80 | (scalar & 0x3F))};
}

void testExactIso8859_4(TestContext& t) {
    // Independent CPython3.9 strict-codec expectations, checked against the
    // pinned Unicode tables by the optional research gate. Do not include or
    // derive expected values from the production generated header.
    static constexpr int graphic[] = {
        0x00A0, 0x0104, 0x0138, 0x0156, 0x00A4, 0x0128, 0x013B, 0x00A7,
        0x00A8, 0x0160, 0x0112, 0x0122, 0x0166, 0x00AD, 0x017D, 0x00AF,
        0x00B0, 0x0105, 0x02DB, 0x0157, 0x00B4, 0x0129, 0x013C, 0x02C7,
        0x00B8, 0x0161, 0x0113, 0x0123, 0x0167, 0x014A, 0x017E, 0x014B,
        0x0100, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x012E,
        0x010C, 0x00C9, 0x0118, 0x00CB, 0x0116, 0x00CD, 0x00CE, 0x012A,
        0x0110, 0x0145, 0x014C, 0x0136, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
        0x00D8, 0x0172, 0x00DA, 0x00DB, 0x00DC, 0x0168, 0x016A, 0x00DF,
        0x0101, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x012F,
        0x010D, 0x00E9, 0x0119, 0x00EB, 0x0117, 0x00ED, 0x00EE, 0x012B,
        0x0111, 0x0146, 0x014D, 0x0137, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
        0x00F8, 0x0173, 0x00FA, 0x00FB, 0x00FC, 0x0169, 0x016B, 0x02D9,
    };
    static constexpr int windows[] = {
        0x20AC, 0x0000, 0x201A, 0x0000, 0x201E, 0x2026, 0x2020, 0x2021,
        0x0000, 0x2030, 0x0000, 0x2039, 0x0000, 0x00A8, 0x02C7, 0x00B8,
        0x0000, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
        0x0000, 0x2122, 0x0000, 0x203A, 0x0000, 0x00AF, 0x02DB, 0x0000,
        0x00A0, 0x0000, 0x00A2, 0x00A3, 0x00A4, 0x0000, 0x00A6, 0x00A7,
        0x00D8, 0x00A9, 0x0156, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00C6,
        0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
        0x00F8, 0x00B9, 0x0157, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00E6,
        0x0104, 0x012E, 0x0100, 0x0106, 0x00C4, 0x00C5, 0x0118, 0x0112,
        0x010C, 0x00C9, 0x0179, 0x0116, 0x0122, 0x0136, 0x012A, 0x013B,
        0x0160, 0x0143, 0x0145, 0x00D3, 0x014C, 0x00D5, 0x00D6, 0x00D7,
        0x0172, 0x0141, 0x015A, 0x016A, 0x00DC, 0x017B, 0x017D, 0x00DF,
        0x0105, 0x012F, 0x0101, 0x0107, 0x00E4, 0x00E5, 0x0119, 0x0113,
        0x010D, 0x00E9, 0x017A, 0x0117, 0x0123, 0x0137, 0x012B, 0x013C,
        0x0161, 0x0144, 0x0146, 0x00F3, 0x014D, 0x00F5, 0x00F6, 0x00F7,
        0x0173, 0x0142, 0x015B, 0x016B, 0x00FC, 0x017C, 0x017E, 0x02D9,
    };
    static_assert(sizeof(graphic) / sizeof(graphic[0]) == 96, "complete ISO graphic expectations");
    static_assert(sizeof(windows) / sizeof(windows[0]) == 128, "complete Windows upper expectations");
    std::size_t pairs = 0;
    for (const char* name : {"ISO8859-4", "ISO-8859-4", "iso8859-4", "iso-8859-4"}) {
        auto converter = DRW_TextCodec::createExactCodePageConverter(name);
        t.expect(converter != nullptr, "both exact ISO spellings and case variants have a real converter");
        if (!converter) continue;
        std::string allBytes, allExpected;
        for (int byte = 0; byte < 256; ++byte) {
            const std::string wire(1, static_cast<char>(byte));
            const int scalar = byte < 160 ? byte : graphic[byte - 160];
            const auto expected = unicodeScalarExpectedUtf8(scalar);
            t.expect(converter->toUtf8(wire) == expected
                         && converter->decodeObservation().issues == 0
                         && converter->decodeObservation().nonAsciiBytes == (byte >= 128)
                         && converter->decodeObservation().legacyBytes,
                     "every defined ISO byte decodes to its independent scalar with truthful observations");
            t.expect(converter->fromUtf8(expected) == wire,
                     "every independent ISO scalar inverse-encodes to the original byte");
            allBytes += wire; allExpected += expected; ++pairs;
        }
        t.expect(converter->toUtf8(allBytes) == allExpected
                     && converter->fromUtf8(allExpected) == allBytes,
                 "mixed full-range ISO input including NUL/C1 preserves all table bytes without record-framing claims");
        t.expect(converter->toUtf8("\xA1\\U+4E2D\xA2Z") == "\xC4\x84\xE4\xB8\xAD\xC4\xB8Z"
                     && converter->fromUtf8("\xE4\xB8\xAD") == "\\U+4E2D",
                 "adjacent ISO byte/CIF and unmapped BMP fallback retain exact scalar/suffix semantics");
        t.expect(converter->fromUtf8("\x80Z") == "?Z",
                 "malformed UTF8 retains existing converter fallback and suffix, not a supported-format pass");
    }
    auto canonical = DRW_TextCodec::createExactCodePageConverter("ANSI_1257");
    t.expect(canonical != nullptr, "existing canonical Windows factory is available independently");
    for (const char* name : {"ANSI_1252", "cp1252", "ANSI_1257", "CP1257", "ansi_949",
             "CSWINDOWS31J", "X-MS-CP932", "UTF8", "UTF-16"}) {
        auto factory = DRW_TextCodec::createExactCodePageConverter(name);
        const bool utf16 = std::string_view(name) == "UTF-16";
        const auto wire = utf16 ? std::string("A\0", 2) : "A";
        t.expect(factory && factory->toUtf8(wire) == "A"
                     && (utf16 ? factory->fromUtf8("A").empty() : factory->fromUtf8("A") == wire),
                 "input-side factory preserves canonical byte grammar and the existing UTF16 read-only boundary");
    }
    std::size_t validWindows = 0, undefinedWindows = 0;
    if (canonical) for (int byte = 0; byte < 256; ++byte) {
        const auto wire = std::string(1, static_cast<char>(byte));
        const int scalar = byte < 128 ? byte : windows[byte - 128];
        const auto decoded = canonical->toUtf8(wire);
        if (byte >= 128 && scalar == 0) {
            const auto expectedIssues = DRW::dxfTextIssueMask(DRW::DxfTextIssue::UndefinedByteSequence)
                | DRW::dxfTextIssueMask(DRW::DxfTextIssue::DiscardedSource);
            t.expect(decoded.empty() && canonical->decodeObservation().issues == expectedIssues,
                     "undefined Windows bytes remain separate from the fully defined ISO table");
            ++undefinedWindows;
        } else {
            const auto expected = unicodeScalarExpectedUtf8(scalar);
            t.expect(decoded == expected && canonical->decodeObservation().issues == 0
                         && canonical->fromUtf8(expected) == wire,
                     "all defined canonical Windows bytes and inverse mappings remain unchanged");
            ++validWindows;
        }
    }
    for (const char* unsupported : {"EUC-JP", "GB18030",
             "BIG5-HKSCS", "CSISO", "unknown-authored-page", ""})
        t.expect(DRW_TextCodec::createExactCodePageConverter(unsupported) == nullptr,
                 "unimplemented exact families never acquire the neighboring Windows converter");
    auto retained = DRW_TextCodec::createExactCodePageConverter("ISO8859-4");
    bool allocationFailed = false;
    failImportObservationAllocations = true;
    try { auto rejected = DRW_TextCodec::createExactCodePageConverter("ISO8859-4"); }
    catch (const std::bad_alloc&) { allocationFailed = true; }
    failImportObservationAllocations = false;
    t.expect(allocationFailed && retained && retained->toUtf8("\xA1") == "\xC4\x84"
                 && DRW_TextCodec::createExactCodePageConverter("ISO8859-4") != nullptr,
             "failed factory allocation preserves an existing converter and a later retry");
    t.expect(DRW_TextCodec::createExactCodePageConverter(std::string("ISO8859-4\0X", 11)) == nullptr,
             "factory does not truncate an exact mapping name at embedded NUL");
    DRW_TextCodec compatible;
    compatible.setVersion(DRW::AC1015, true); compatible.setCodePage("ISO8859-4", true);
    auto exact = DRW_TextCodec::createExactCodePageConverter("ISO8859-4");
    t.expect(exact && exact->toUtf8("\xA1\xA2\xFF") == "\xC4\x84\xC4\xB8\xCB\x99"
                 && compatible.getCodePage() == "ANSI_1257" && compatible.toUtf8("\xA1").empty(),
             "literal independent ISO anchors do not change the caller's historical codec/metadata");
    std::cout << "Exact ISO8859-4: byte/inverse pairs=" << pairs << ", aliases=4, Windows defined="
              << validWindows << ", Windows undefined=" << undefinedWindows
              << "; batch/CIF/malformed/factory lifecycle controls pass\n";
}

void testExactIso8859_1(TestContext& t) {
    const char* aliases[] = {"ISO8859-1", "ISO-8859-1", "iso8859-1", "iso-8859-1",
        "LATIN1", "ISO_8859-1", "CP819", "IBM819", "ISO-IR-100", "L1"};
    std::size_t pairs = 0;
    for (const char* name : aliases) {
        auto converter = DRW_TextCodec::createExactCodePageConverter(name);
        t.expect(converter != nullptr, "exact ISO-8859-1 spelling resolves to its own decoder");
        if (!converter) continue;
        std::string allBytes, allExpected;
        for (int byte = 0; byte < 256; ++byte) {
            const std::string wire(1, static_cast<char>(byte));
            const auto expected = unicodeScalarExpectedUtf8(byte);
            t.expect(converter->toUtf8(wire) == expected
                         && converter->decodeObservation().issues == 0
                         && converter->decodeObservation().nonAsciiBytes == (byte >= 128)
                         && converter->decodeObservation().legacyBytes,
                     "every ISO-8859-1 byte decodes to its same-number Unicode scalar");
            t.expect(converter->fromUtf8(expected) == wire,
                     "every ISO-8859-1 scalar inverse-encodes to its source byte");
            allBytes += wire;
            allExpected += expected;
            ++pairs;
        }
        t.expect(converter->toUtf8(allBytes) == allExpected
                     && converter->fromUtf8(allExpected) == allBytes,
                 "mixed full-range ISO-8859-1 batch preserves ASCII, C1 controls and graphic bytes");
    }

    auto windows = DRW_TextCodec::createExactCodePageConverter("ANSI_1252");
    auto latin1 = DRW_TextCodec::createExactCodePageConverter("LATIN1");
    t.expect(windows && latin1 && windows->toUtf8("\x80") == "\xE2\x82\xAC"
                 && latin1->toUtf8("\x80") == "\xC2\x80",
             "explicit Latin-1 keeps C1 identity distinct from historical Windows-1252 selection");
    DRW_TextCodec compatible;
    compatible.setVersion(DRW::AC1015, true);
    compatible.setCodePage("LATIN1", true);
    t.expect(compatible.getCodePage() == "ANSI_1252"
                 && compatible.toUtf8("\x80") == "\xE2\x82\xAC",
             "ordinary Compatible decoding remains unchanged for Latin-1 aliases");
    std::cout << "Exact ISO8859-1: byte/inverse pairs=" << pairs
              << ", aliases=" << (sizeof(aliases) / sizeof(aliases[0]))
              << "; CP1252 compatibility remains unchanged\n";
}

void testExactIso8859_2(TestContext& t) {
    // Independent transcription of Unicode's ISO8859/8859-2.TXT mapping.
    static constexpr int upper[] = {
        0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
        0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,
        0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
        0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F,
        0x00A0, 0x0104, 0x02D8, 0x0141, 0x00A4, 0x013D, 0x015A, 0x00A7,
        0x00A8, 0x0160, 0x015E, 0x0164, 0x0179, 0x00AD, 0x017D, 0x017B,
        0x00B0, 0x0105, 0x02DB, 0x0142, 0x00B4, 0x013E, 0x015B, 0x02C7,
        0x00B8, 0x0161, 0x015F, 0x0165, 0x017A, 0x02DD, 0x017E, 0x017C,
        0x0154, 0x00C1, 0x00C2, 0x0102, 0x00C4, 0x0139, 0x0106, 0x00C7,
        0x010C, 0x00C9, 0x0118, 0x00CB, 0x011A, 0x00CD, 0x00CE, 0x010E,
        0x0110, 0x0143, 0x0147, 0x00D3, 0x00D4, 0x0150, 0x00D6, 0x00D7,
        0x0158, 0x016E, 0x00DA, 0x0170, 0x00DC, 0x00DD, 0x0162, 0x00DF,
        0x0155, 0x00E1, 0x00E2, 0x0103, 0x00E4, 0x013A, 0x0107, 0x00E7,
        0x010D, 0x00E9, 0x0119, 0x00EB, 0x011B, 0x00ED, 0x00EE, 0x010F,
        0x0111, 0x0144, 0x0148, 0x00F3, 0x00F4, 0x0151, 0x00F6, 0x00F7,
        0x0159, 0x016F, 0x00FA, 0x0171, 0x00FC, 0x00FD, 0x0163, 0x02D9
    };
    static_assert(sizeof(upper) / sizeof(upper[0]) == 128,
                  "ISO-8859-2 mapping spans every upper byte");

    const char* aliases[] = {"ISO8859-2", "ISO-8859-2"};
    std::size_t pairs = 0;
    for (const char* name : aliases) {
        const auto resolution = DRW_TextCodec::resolveCodePage(
            name, DRW_CodePageMappingPolicy::ExactNamed);
        auto converter = DRW_TextCodec::createExactCodePageConverter(name);
        t.expect(resolution.status
                         == DRW_CodePageMappingStatus::ExactImplementationAvailable
                     && resolution.decoderName != nullptr
                     && std::string(resolution.decoderName) == "ISO8859-2"
                     && converter != nullptr,
                 "exact ISO-8859-2 aliases resolve to the distinct Latin-2 decoder");
        if (!converter) continue;
        std::string bytes, expected;
        for (unsigned int byte = 0x80u; byte <= 0xFFu; ++byte) {
            const std::string wire(
                1u, static_cast<char>(static_cast<unsigned char>(byte)));
            const auto utf8 = unicodeScalarExpectedUtf8(upper[byte - 0x80u]);
            t.expect(converter->toUtf8(wire) == utf8
                         && converter->decodeObservation().issues == 0
                         && converter->decodeObservation().nonAsciiBytes
                         && converter->fromUtf8(utf8) == wire,
                     "every ISO-8859-2 upper byte decodes and inverse-encodes exactly");
            bytes += wire;
            expected += utf8;
            ++pairs;
        }
        t.expect(converter->toUtf8(bytes) == expected
                     && converter->fromUtf8(expected) == bytes,
                 "ISO-8859-2 mixed high-byte batch preserves ordering and roundtrip");
    }
    t.expect(pairs == 256,
             "both exact ISO-8859-2 spellings cover the complete high-byte range");

    DRW_TextCodec compatible;
    compatible.setVersion(DRW::AC1015, true);
    compatible.setCodePage("ISO8859-2", true);
    t.expect(compatible.getCodePage() == "ANSI_1250"
                 && compatible.toUtf8("\x80") == "\xE2\x82\xAC",
             "ordinary Compatible decoding retains the historical Windows-1250 fallback");
}

void testExactIso8859_3(TestContext& t) {
    // Independent transcription of Unicode's ISO8859/8859-3.TXT mapping.
    // Zero marks the seven undefined byte positions; valid C1 controls remain
    // identity mappings rather than being treated as missing table entries.
    static constexpr int upper[] = {
        0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
        0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,
        0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
        0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F,
        0x00A0, 0x0126, 0x02D8, 0x00A3, 0x00A4, 0x0000, 0x0124, 0x00A7,
        0x00A8, 0x0130, 0x015E, 0x011E, 0x0134, 0x00AD, 0x0000, 0x017B,
        0x00B0, 0x0127, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x0125, 0x00B7,
        0x00B8, 0x0131, 0x015F, 0x011F, 0x0135, 0x00BD, 0x0000, 0x017C,
        0x00C0, 0x00C1, 0x00C2, 0x0000, 0x00C4, 0x010A, 0x0108, 0x00C7,
        0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
        0x0000, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x0120, 0x00D6, 0x00D7,
        0x011C, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x016C, 0x015C, 0x00DF,
        0x00E0, 0x00E1, 0x00E2, 0x0000, 0x00E4, 0x010B, 0x0109, 0x00E7,
        0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
        0x0000, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x0121, 0x00F6, 0x00F7,
        0x011D, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x016D, 0x015D, 0x02D9
    };
    static_assert(sizeof(upper) / sizeof(upper[0]) == 128,
                  "ISO-8859-3 mapping spans every upper byte");

    constexpr std::uint32_t undefinedIssues =
        DRW::dxfTextIssueMask(DRW::DxfTextIssue::UndefinedByteSequence)
        | DRW::dxfTextIssueMask(DRW::DxfTextIssue::DiscardedSource);
    const char* aliases[] = {"ISO8859-3", "ISO-8859-3", "iso8859-3", "iso-8859-3"};
    std::size_t validPairs = 0, undefinedPairs = 0;
    for (const char* name : aliases) {
        const auto resolution = DRW_TextCodec::resolveCodePage(
            name, DRW_CodePageMappingPolicy::ExactNamed);
        auto converter = DRW_TextCodec::createExactCodePageConverter(name);
        t.expect(resolution.status
                         == DRW_CodePageMappingStatus::ExactImplementationAvailable
                     && resolution.requestedName != nullptr
                     && std::string(resolution.requestedName) == "ISO8859-3"
                     && resolution.decoderName != nullptr
                     && std::string(resolution.decoderName) == "ISO8859-3"
                     && converter != nullptr,
                 "exact ISO-8859-3 aliases resolve to the distinct Latin-3 decoder");
        if (!converter) continue;
        std::string allBytes, allExpected, definedBytes;
        for (unsigned int byte = 0x80u; byte <= 0xFFu; ++byte) {
            const std::string wire(
                1u, static_cast<char>(static_cast<unsigned char>(byte)));
            allBytes += wire;
            const int scalar = upper[byte - 0x80u];
            if (scalar == 0) {
                const auto decoded = converter->toUtf8(wire);
                t.expect(decoded.empty()
                             && converter->decodeObservation().issues == undefinedIssues
                             && converter->decodeObservation().nonAsciiBytes
                             && converter->decodeObservation().legacyBytes,
                         "each undefined ISO-8859-3 byte is discarded and distinctly observed");
                ++undefinedPairs;
                continue;
            }
            const auto utf8 = unicodeScalarExpectedUtf8(scalar);
            t.expect(converter->toUtf8(wire) == utf8
                         && converter->decodeObservation().issues == 0
                         && converter->decodeObservation().nonAsciiBytes
                         && converter->decodeObservation().legacyBytes
                         && converter->fromUtf8(utf8) == wire,
                     "every defined ISO-8859-3 byte decodes and inverse-encodes exactly");
            allExpected += utf8;
            definedBytes += wire;
            ++validPairs;
        }
        t.expect(converter->toUtf8(allBytes) == allExpected
                     && converter->decodeObservation().issues == undefinedIssues
                     && converter->decodeObservation().nonAsciiBytes
                     && converter->fromUtf8(allExpected) == definedBytes,
                 "mixed ISO-8859-3 batch preserves order while discarding only undefined slots");
    }
    t.expect(validPairs == 4u * 121u && undefinedPairs == 4u * 7u,
             "all four exact spellings cover defined and undefined upper-half bytes");

    DRW_TextCodec compact;
    compact.setVersion(DRW::AC1015, true);
    compact.setCodePage("ISO8859-3", true);
    DRW_TextCodec hyphenated;
    hyphenated.setVersion(DRW::AC1015, true);
    hyphenated.setCodePage("ISO-8859-3", true);
    t.expect(compact.getCodePage() == "ANSI_1254"
                 && compact.toUtf8("\xA1") == "\xC2\xA1"
                 && hyphenated.getCodePage() == "ANSI_1252"
                 && hyphenated.toUtf8("\xA1") == "\xC2\xA1",
             "Compatible ISO-8859-3 spellings retain their distinct historical Windows fallbacks");
}

void testExactIso8859_5(TestContext& t) {
    // Independent transcription of Unicode's ISO8859/8859-5.TXT mapping.
    static constexpr int upper[] = {
        0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
        0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,
        0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
        0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F,
        0x00A0, 0x0401, 0x0402, 0x0403, 0x0404, 0x0405, 0x0406, 0x0407,
        0x0408, 0x0409, 0x040A, 0x040B, 0x040C, 0x00AD, 0x040E, 0x040F,
        0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
        0x0418, 0x0419, 0x041A, 0x041B, 0x041C, 0x041D, 0x041E, 0x041F,
        0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
        0x0428, 0x0429, 0x042A, 0x042B, 0x042C, 0x042D, 0x042E, 0x042F,
        0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
        0x0438, 0x0439, 0x043A, 0x043B, 0x043C, 0x043D, 0x043E, 0x043F,
        0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
        0x0448, 0x0449, 0x044A, 0x044B, 0x044C, 0x044D, 0x044E, 0x044F,
        0x2116, 0x0451, 0x0452, 0x0453, 0x0454, 0x0455, 0x0456, 0x0457,
        0x0458, 0x0459, 0x045A, 0x045B, 0x045C, 0x00A7, 0x045E, 0x045F
    };
    static_assert(sizeof(upper) / sizeof(upper[0]) == 128,
                  "ISO-8859-5 mapping spans every upper byte");

    const char* aliases[] = {"ISO8859-5", "ISO-8859-5", "iso8859-5", "iso-8859-5"};
    std::size_t pairs = 0;
    for (const char* name : aliases) {
        const auto resolution = DRW_TextCodec::resolveCodePage(
            name, DRW_CodePageMappingPolicy::ExactNamed);
        auto converter = DRW_TextCodec::createExactCodePageConverter(name);
        t.expect(resolution.status
                         == DRW_CodePageMappingStatus::ExactImplementationAvailable
                     && resolution.requestedName != nullptr
                     && std::string(resolution.requestedName) == "ISO8859-5"
                     && resolution.decoderName != nullptr
                     && std::string(resolution.decoderName) == "ISO8859-5"
                     && converter != nullptr,
                 "exact ISO-8859-5 aliases resolve to the distinct Cyrillic decoder");
        if (!converter) continue;
        std::string bytes, expected;
        for (unsigned int byte = 0x80u; byte <= 0xFFu; ++byte) {
            const std::string wire(
                1u, static_cast<char>(static_cast<unsigned char>(byte)));
            const auto utf8 = unicodeScalarExpectedUtf8(upper[byte - 0x80u]);
            t.expect(converter->toUtf8(wire) == utf8
                         && converter->decodeObservation().issues == 0
                         && converter->decodeObservation().nonAsciiBytes
                         && converter->decodeObservation().legacyBytes
                         && converter->fromUtf8(utf8) == wire,
                     "every ISO-8859-5 upper byte decodes and inverse-encodes exactly");
            bytes += wire;
            expected += utf8;
            ++pairs;
        }
        t.expect(converter->toUtf8(bytes) == expected
                     && converter->decodeObservation().issues == 0
                     && converter->fromUtf8(expected) == bytes,
                 "mixed ISO-8859-5 batch preserves C1 controls and all Cyrillic bytes");
    }
    t.expect(pairs == 512u,
             "all four exact ISO-8859-5 spellings cover the complete upper-byte range");

    DRW_TextCodec compact;
    compact.setVersion(DRW::AC1015, true);
    compact.setCodePage("ISO8859-5", true);
    DRW_TextCodec hyphenated;
    hyphenated.setVersion(DRW::AC1015, true);
    hyphenated.setCodePage("ISO-8859-5", true);
    t.expect(compact.getCodePage() == "ANSI_1251"
                 && compact.toUtf8("\xA1") == "\xD0\x8E"
                 && hyphenated.getCodePage() == "ANSI_1252"
                 && hyphenated.toUtf8("\xA1") == "\xC2\xA1",
             "Compatible ISO-8859-5 spellings retain their Windows-1251/1252 fallbacks");
}

std::string makeCodePageDiagnosticDrawing(const char* declaration, bool modern,
                                          bool pageFirst, bool binary,
                                          int headerRepeats = 1) {
    std::ostringstream output(std::ios::binary);
    if (binary) {
        static constexpr char signature[] = "AutoCAD Binary DXF\r\n\x1a\0";
        output.write(signature, sizeof(signature) - 1);
    }
    std::unique_ptr<dxfWriter> writer;
    if (binary) writer = std::make_unique<dxfWriterBinary>(&output);
    else writer = std::make_unique<dxfWriterAscii>(&output);
    const auto page = [&]() {
        if (declaration != nullptr) {
            writer->writeString(9, "$DWGCODEPAGE");
            writer->writeString(3, declaration);
        }
    };
    for (int i = 0; i < headerRepeats; ++i) {
        writer->writeString(0, "SECTION");
        writer->writeString(2, "HEADER");
        if (pageFirst) page();
        writer->writeString(9, "$ACADVER");
        writer->writeString(1, modern ? "AC1024" : "AC1015");
        if (!pageFirst) page();
        writer->writeString(0, "ENDSEC");
    }
    writer->writeString(0, "SECTION");
    writer->writeString(2, "ENTITIES");
    writer->writeString(0, "TEXT");
    writer->writeString(5, "A");
    writer->writeString(8, "0");
    writer->writeDouble(10, 0);
    writer->writeDouble(20, 0);
    writer->writeDouble(30, 0);
    writer->writeDouble(40, 1);
    writer->writeString(1, modern ? "\xC3\xA9\xE4\xB8\xAD" : "ascii-control");
    writer->writeString(0, "LINE");
    writer->writeString(5, "B");
    writer->writeString(8, "0");
    writer->writeDouble(10, 1);
    writer->writeDouble(20, 2);
    writer->writeDouble(30, 3);
    writer->writeDouble(11, 4);
    writer->writeDouble(21, 5);
    writer->writeDouble(31, 6);
    writer->writeString(0, "ENDSEC");
    writer->writeString(0, "EOF");
    return output.str();
}

void testCodePageDiagnosticHandoff(TestContext& t) {
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-codepage-selection-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    struct Row { const char* source; const char* advisory; };
    const Row rows[] = {
        {nullptr, nullptr}, {"ANSI_1252", nullptr}, {"CP1252", nullptr},
        {"ISO8859-4", "compatibility-codepage-alias"},
        {"CP866", "unavailable-codepage"},
        {"unknown-local-test-codepage", "unknown-codepage"},
        {"UTF8", "reapplied-codepage"}
    };
    std::size_t cases = 0, warnings = 0;
    for (const auto& row : rows) for (bool modern : {false, true})
        for (bool pageFirst : {false, true}) for (int route = 0; route < 3; ++route) {
        const bool binary = route == 2;
        auto bytes = makeCodePageDiagnosticDrawing(row.source, modern, pageFirst, binary);
        const auto path = directory / (std::to_string(cases) + ".dxf");
        if (route != 0) {
            std::ofstream output(path, std::ios::binary);
            output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        }
        dx_data imported;
        PreparedDxInterface capture;
        capture.attach(&imported);
        dxfRW facade(path.string().c_str());
        const bool read = route == 0 ? facade.readAscii(&capture, false, bytes)
                                     : facade.read(&capture, false);
        const auto diagnostic = facade.getLastDiagnostic();
        const char* expected = modern ? nullptr : row.advisory;
        bool correct = read && facade.getError() == DRW::BAD_NONE && !diagnostic.failed()
            && imported.mBlock->ent.size() == 2
            && imported.headerC.hasDxfCodePageDeclaration() == (row.source != nullptr)
            && imported.headerC.dxfCodePageDeclaration() == (row.source != nullptr ? row.source : "");
        if (expected != nullptr) {
            correct = correct && diagnostic.secondary.size() == 1
                && diagnostic.secondary.front().phase == DRW::OperationPhase::Header
                && diagnostic.secondary.front().code == expected;
            ++warnings;
        } else correct = correct && diagnostic.secondary.empty();
        if (correct) {
            const auto* text = dynamic_cast<const DRW_Text*>(imported.mBlock->ent.front());
            const auto* line = dynamic_cast<const DRW_Line*>(imported.mBlock->ent.back());
            correct = text != nullptr && text->text
                == (modern ? "\xC3\xA9\xE4\xB8\xAD" : "ascii-control")
                && line != nullptr && line->basePoint.z == 3 && line->secPoint.z == 6;
        }
        t.expect(correct, "codepage category survives metadata order and ASCII/memory/binary handoff");
        ++cases;
    }
    PreparedDxInterface capture;
    dx_data data;
    capture.attach(&data);
    dxfRW reusable("");
    auto repeated = makeCodePageDiagnosticDrawing("unknown-local-test-codepage", false, false, false, 20);
    t.expect(reusable.readAscii(&capture, false, repeated)
                 && !reusable.getLastDiagnostic().failed()
                 && reusable.getLastDiagnostic().secondary.size()
                        == DRW::OperationDiagnostic::MaxSecondaryEntries,
             "repeated HEADER advisories are bounded without becoming a read failure");
    auto good = makeCodePageDiagnosticDrawing("ANSI_1252", false, false, false);
    const auto retainedSnapshot = reusable.getLastDiagnostic();
    auto malformed = repeated;
    const auto handle = malformed.find("\nA\n");
    if (handle != std::string::npos)
        malformed.replace(handle + 1, 1, "NOT_HEX");
    dx_data failedData;
    capture.attach(&failedData);
    t.expect(handle != std::string::npos
                 && !reusable.readAscii(&capture, false, malformed)
                 && reusable.getLastDiagnostic().failed()
                 && reusable.getLastDiagnostic().code != "unknown-codepage"
                 && reusable.getLastDiagnostic().secondary.size()
                        == DRW::OperationDiagnostic::MaxSecondaryEntries,
             "bounded nonfatal advisories do not hide the later primary handle failure");
    t.expect(!reusable.readAscii(nullptr, false, good)
                 && reusable.getLastDiagnostic().failed()
                 && reusable.getLastDiagnostic().secondary.empty()
                 && retainedSnapshot.secondary.size()
                        == DRW::OperationDiagnostic::MaxSecondaryEntries,
             "null-interface read resets its diagnostic but a copied advisory snapshot survives");
    dx_data retryData;
    capture.attach(&retryData);
    t.expect(reusable.readAscii(&capture, false, good)
                 && reusable.getLastDiagnostic().secondary.empty()
                 && !reusable.getLastDiagnostic().failed(),
             "next admitted read resets advisory metadata and diagnostics");
    std::cout << "Codec HEADER diagnostic handoff: cases=" << cases
              << ", nonfatal advisory cases=" << warnings << '\n';
    if (t.failures == 0) std::filesystem::remove_all(directory);
    else std::cerr << "Codec selection evidence: " << directory << '\n';
}

class HeaderProvenanceAccess final : public DRW_Header {
public:
    bool parse(int code, const std::unique_ptr<dxfReader>& reader) {
        return parseCode(code, reader);
    }
};

bool parseCodePageValue(HeaderProvenanceAccess& header, const std::string& value,
                        const std::shared_ptr<DxfReadRetainedBudget>& budget) {
    std::istringstream input("9\n$DWGCODEPAGE\n3\n" + value + "\n");
    std::unique_ptr<dxfReader> reader = std::make_unique<dxfReaderAscii>(&input);
    reader->setRetainedByteBudget(budget);
    int code = 0;
    return reader->readRec(&code) && header.parse(code, reader)
        && reader->readRec(&code) && header.parse(code, reader);
}

void testCodePageProvenanceBudget(TestContext& t) {
    const std::string first = "local-codepage-" + std::string(32, 'x');
    const std::string second = "LOCAL-codepage-" + std::string(32, 'y');
    const auto size = first.size();
    std::size_t admitted = 0, refused = 0;
    t.expect(size == second.size(), "provenance budget inputs have equal literal lengths");
    for (bool exact : {false, true}) {
        auto budget = std::make_shared<DxfReadRetainedBudget>(7u * size - (exact ? 0u : 1u));
        HeaderProvenanceAccess header;
        const bool parsed = parseCodePageValue(header, first, budget);
        if (parsed) ++admitted; else ++refused;
        t.expect(parsed == exact && header.hasDxfCodePageDeclaration() == exact
                     && budget->exceeded() == !exact
                     && budget->used() == (exact ? size : 0u),
                 "one added provenance value is admitted at exact logical-byte peak and refused one below");
        if (!exact) continue;
        DRW_Header copied(header), assigned;
        assigned = copied;
        DRW_Header moved(std::move(copied)), moveAssigned;
        moveAssigned = std::move(assigned);
        t.expect(&header.dxfCodePageDeclaration() == &moved.dxfCodePageDeclaration()
                     && &header.dxfCodePageDeclaration() == &moveAssigned.dxfCodePageDeclaration()
                     && !copied.hasDxfCodePageDeclaration() && !assigned.hasDxfCodePageDeclaration()
                     && budget->used() == size,
                 "copy/move headers share one immutable charged physical value without borrowing");
        moved.addStr("$DWGCODEPAGE", "ANSI_936", 3);
        t.expect(moved.dxfCodePageDeclaration() == first,
                 "editing the normalized HEADER value does not rewrite historical source provenance");
        header.clearDxfCodePageDeclaration();
        moved = DRW_Header{};
        t.expect(!header.hasDxfCodePageDeclaration()
                     && header.dxfCodePageDeclaration().empty()
                     && moveAssigned.dxfCodePageDeclaration() == first && budget->used() == size,
                 "clear/reset detach a header while other owning copies keep the value charged");
        moveAssigned.clearDxfCodePageDeclaration();
        t.expect(budget->used() == 0u, "last provenance owner releases the logical charge");
    }
    for (bool exact : {false, true}) {
        auto budget = std::make_shared<DxfReadRetainedBudget>(8u * size - (exact ? 0u : 1u));
        HeaderProvenanceAccess header;
        t.expect(parseCodePageValue(header, first, budget), "replacement starts from an admitted declaration");
        DRW_Header retained(header);
        const bool replaced = parseCodePageValue(header, second, budget);
        if (replaced) ++admitted; else ++refused;
        t.expect(replaced == exact && retained.dxfCodePageDeclaration() == first
                     && header.dxfCodePageDeclaration() == (exact ? second : first)
                     && budget->used() == (exact ? 2u * size : size),
                 "replacement reserves overlapping old/new bytes before publication and keeps old snapshot on refusal");
        header.clearDxfCodePageDeclaration();
        retained.clearDxfCodePageDeclaration();
        t.expect(budget->used() == 0u, "replacement/refusal releases every provenance charge at last ownership");
    }
    t.expect(admitted == 2 && refused == 2, "both exact/one-below provenance admission pairs execute");
    std::cout << "Codepage provenance budget: admitted=" << admitted << ", refused=" << refused << '\n';
}

void testCodePageDeclarationProvenance(TestContext& t) {
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-codepage-provenance-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(directory);
    struct Row { std::string raw; const char* normalized; };
    const Row rows[] = {
        {"iso8859-4", "ANSI_1257"}, {"gBk", "ANSI_936"}, {"", "ANSI_1252"},
        {"ANSI_1252 ", "ANSI_1252"}, {"local-\xE9-\\U+0041", "ANSI_1252"},
        {"local-long-" + std::string(4096, 'q'), "ANSI_1252"}
    };
    std::size_t cases = 0;
    for (const auto& row : rows) for (bool modern : {false, true})
        for (bool pageFirst : {false, true}) for (int route = 0; route < 3; ++route) {
        auto bytes = makeCodePageDiagnosticDrawing(row.raw.c_str(), modern, pageFirst, route == 2);
        const auto path = directory / (std::to_string(cases) + ".dxf");
        if (route != 0) {
            std::ofstream output(path, std::ios::binary);
            output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        }
        dx_data imported;
        PreparedDxInterface capture;
        capture.attach(&imported);
        bool read = false;
        {
            dxfRW facade(path.string().c_str());
            read = route == 0 ? facade.readAscii(&capture, false, bytes)
                              : facade.read(&capture, false);
        }
        t.expect(read && imported.headerC.hasDxfCodePageDeclaration()
                     && imported.headerC.dxfCodePageDeclaration() == row.raw
                     && headerString(imported.headerC, "$DWGCODEPAGE") == row.normalized
                     && imported.mBlock->ent.size() == 2,
                 "full physical declaration survives reader/facade destruction without changing normalized HEADER");
        ++cases;
    }

    // ASCII CRLF terminates the value; in binary DXF a CR before the NUL is
    // payload and must not be silently trimmed from the source snapshot.
    std::size_t delimiterCases = 0;
    for (int route = 0; route < 3; ++route) {
        const std::string expected = route == 2 ? "gBk\r" : "gBk";
        auto bytes = makeCodePageDiagnosticDrawing(expected.c_str(), false, false, route == 2);
        if (route != 2) {
            std::string crlf;
            crlf.reserve(2u * bytes.size());
            for (char value : bytes) {
                if (value == '\n') crlf += '\r';
                crlf += value;
            }
            bytes = std::move(crlf);
        }
        const auto path = directory / ("delimiters-" + std::to_string(route) + ".dxf");
        if (route != 0) {
            std::ofstream output(path, std::ios::binary);
            output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        }
        dx_data data;
        PreparedDxInterface sink;
        sink.attach(&data);
        bool read = false;
        {
            dxfRW facade(path.string().c_str());
            read = route == 0 ? facade.readAscii(&sink, false, bytes)
                              : facade.read(&sink, false);
        }
        t.expect(read && data.headerC.hasDxfCodePageDeclaration()
                     && data.headerC.dxfCodePageDeclaration() == expected
                     && data.mBlock->ent.size() == 2,
                 "provenance excludes ASCII CRLF framing but preserves binary payload CR");
        ++delimiterCases;
    }

    class HeaderHistory final : public dx_iface {
    public:
        explicit HeaderHistory(dx_data& data) { cData = &data; currentBlock = data.mBlock; }
        void addHeader(const DRW_Header* value) override {
            headers.push_back(*value);
            dx_iface::addHeader(value);
        }
        std::vector<DRW_Header> headers;
    };
    auto repeated = makeCodePageDiagnosticDrawing("CP866", false, false, false, 2);
    const auto first = repeated.find("CP866\n");
    if (first != std::string::npos) repeated.replace(first, 6, "KoI8-r\n");
    dx_data historyData;
    HeaderHistory history(historyData);
    dxfRW reusable("");
    t.expect(first != std::string::npos && reusable.readAscii(&history, false, repeated)
                 && history.headers.size() == 2
                 && history.headers[0].dxfCodePageDeclaration() == "KoI8-r"
                 && history.headers[1].dxfCodePageDeclaration() == "CP866"
                 && historyData.headerC.dxfCodePageDeclaration() == "CP866",
             "repeated HEADER publication keeps each immutable original declaration independently");
    auto plain = makeCodePageDiagnosticDrawing(nullptr, false, false, false);
    dx_data fresh;
    PreparedDxInterface capture;
    capture.attach(&fresh);
    t.expect(reusable.readAscii(&capture, false, plain)
                 && !fresh.headerC.hasDxfCodePageDeclaration()
                 && history.headers.front().dxfCodePageDeclaration() == "KoI8-r",
             "a new read without a declaration does not inherit old provenance or invalidate retained copies");
    auto value = makeCodePageDiagnosticDrawing("gBk", false, false, false);
    reusable.setDxfReadRetainedByteBudget(value.size() - 1u);
    dx_data rejected;
    capture.attach(&rejected);
    t.expect(!reusable.readAscii(&capture, false, value)
                 && reusable.getLastDiagnostic().code == "dxf-retained-byte-budget"
                 && !rejected.headerC.hasDxfCodePageDeclaration(),
             "failed pre-header admission publishes no provenance");
    reusable.setDxfReadRetainedByteBudget(value.size() + 4096u);
    dx_data retry;
    capture.attach(&retry);
    t.expect(reusable.readAscii(&capture, false, value)
                 && retry.headerC.dxfCodePageDeclaration() == "gBk",
             "same facade accepts fresh provenance after corrected read budget");
    auto duplicate = makeCodePageDiagnosticDrawing("CP866", false, false, false);
    const std::string marker = "\nCP866\n";
    const auto duplicateAt = duplicate.find(marker);
    if (duplicateAt != std::string::npos)
        duplicate.insert(duplicateAt + marker.size(), "9\n$DWGCODEPAGE\n3\nansi_1252\n");
    dx_data duplicateData;
    capture.attach(&duplicateData);
    t.expect(duplicateAt != std::string::npos && reusable.readAscii(&capture, false, duplicate)
                 && duplicateData.headerC.dxfCodePageDeclaration() == "ansi_1252"
                 && headerString(duplicateData.headerC, "$DWGCODEPAGE") == "ANSI_1252",
             "repeated declarations retain the last complete physical spelling, not a normalized alias");
    auto partial = makeCodePageDiagnosticDrawing("KoI8-r", false, false, false);
    const auto invalidHandle = partial.find("\nA\n");
    if (invalidHandle != std::string::npos) partial.replace(invalidHandle + 1, 1, "NOT_HEX");
    dx_data partialData;
    capture.attach(&partialData);
    t.expect(invalidHandle != std::string::npos && !reusable.readAscii(&capture, false, partial)
                 && partialData.headerC.dxfCodePageDeclaration() == "KoI8-r",
             "a later entity failure does not invalidate already published source metadata");

    const std::string budgetSource = "local-budget-" + std::string(64, 'r');
    std::size_t facadeAdmitted = 0, facadeRefused = 0;
    for (int route = 0; route < 3; ++route) {
        auto input = makeCodePageDiagnosticDrawing(budgetSource.c_str(), false, false, route == 2);
        const auto path = directory / ("budget-" + std::to_string(route) + ".dxf");
        if (route != 0) {
            std::ofstream output(path, std::ios::binary);
            output.write(input.data(), static_cast<std::streamsize>(input.size()));
        }
        dxfRW facade(path.string().c_str());
        // 1x source spelling + 1x reader value + 4x getter-output headroom
        // + 1x new immutable provenance; memory route also owns the stream.
        const auto peak = (route == 0 ? input.size() : 0u) + 7u * budgetSource.size();
        for (bool exact : {false, true}) {
            facade.setDxfReadRetainedByteBudget(peak - (exact ? 0u : 1u));
            dx_data data;
            capture.attach(&data);
            const bool accepted = route == 0 ? facade.readAscii(&capture, false, input)
                                             : facade.read(&capture, false);
            bool correct = accepted == exact;
            if (accepted) {
                ++facadeAdmitted;
                correct = correct && data.headerC.dxfCodePageDeclaration() == budgetSource
                    && data.mBlock->ent.size() == 2;
            } else {
                ++facadeRefused;
                correct = correct && facade.getLastDiagnostic().code == "dxf-retained-byte-budget"
                    && facade.getLastDiagnostic().cause == DRW::OperationCause::ResourceLimit
                    && !data.headerC.hasDxfCodePageDeclaration() && data.mBlock->ent.empty();
            }
            t.expect(correct, "facade exact/one-below provenance budget controls reject before callback and retry correctly");
        }
    }
    const auto exported = directory / "normalized-output.dxf";
    dx_iface writer;
    t.expect(writer.fileExport(exported.string(), DRW::AC1015, false, &retry, false)
                 && retry.headerC.dxfCodePageDeclaration() == "gBk",
             "export does not rewrite historical source provenance");
    dx_data reimported;
    dx_iface reader;
    t.expect(reader.fileImport(exported.string(), &reimported, false)
                 && reimported.headerC.dxfCodePageDeclaration() == "ANSI_936",
             "writer emits the established normalized declaration, not hidden source replay");
    std::cout << "Codepage declaration provenance: handoffs=" << cases
              << ", delimiter controls=" << delimiterCases
              << ", facade admitted=" << facadeAdmitted << ", facade refused=" << facadeRefused << '\n';
    if (t.failures == 0) std::filesystem::remove_all(directory);
    else std::cerr << "Codepage provenance evidence: " << directory << '\n';
}

void testMifEscapePolicy(TestContext& t) {
    struct MifCase { const char* source; const char* expected; };
    const std::vector<MifCase> cases{
        {"\\M+193FA", "日"}, {"\\M+2A4A4", "中"},
        {"\\M+3B0A1", "가"}, {"\\M+5D6D0", "中"},
        {"\\M+193fa", "日"}, {"\\M+2a4a4", "中"},
        {"\\M+3b0a1", "가"}, {"\\M+5d6d0", "中"},
        {"\\M+193FAB", "日B"}, {"\\M+193FA\\M+5D6D0", "日中"},
        {"\\M+48861", "\\M+48861"},
        {"X\\M+48861Y", "X\\M+48861Y"},
        {"\\M+48861\\M+48861", "\\M+48861\\M+48861"},
        {"\\M+193FA\\M+48861\\M+5D6D0", "日\\M+48861中"},
        {"\\M+193FG", "\\M+193FG"}, {"\\M+5D6DG", "\\M+5D6DG"},
        {"\\M+193F ", "\\M+193F "}, {"\\M+1 3FA", "\\M+1 3FA"},
        {"\\M+1+3FA", "\\M+1+3FA"}, {"\\M+1-3FA", "\\M+1-3FA"},
        {"\\M+1ZZZZ", "\\M+1ZZZZ"}, {"\\M+5    ", "\\M+5    "},
        {"\\M+4886G", "\\M+4886G"}, {"\\M+10041", "\\M+10041"},
        {"\\M+68861", "\\M+68861"}, {"\\M+08861", "\\M+08861"},
        {"\\M+", "\\M+"}, {"\\M+1", "\\M+1"},
        {"\\M+193F", "\\M+193F"}, {"\\m+193FA", "\\m+193FA"},
        {"ordinary\\path", "ordinary\\path"}, {"", ""}
    };
    // Cover each existing caller family: pass-through, single-byte table,
    // generic DBCS and specialized Shift-JIS. The selector's page is not the
    // outer drawing page. Unavailable Johab remains literal, not CP1252 text.
    for (const char* page : {"ANSI_1252", "ANSI_1251", "ANSI_936", "ANSI_932"}) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1018, true);
        codec.setCodePage(page, true);
        for (const auto& row : cases) {
            const std::string source(row.source);
            t.expect(codec.toUtf8(source) == row.expected,
                     "MIF exact-hex decoding preserves unsupported/malformed spelling in every codec family");
            t.expect(source == row.source, "MIF decoding does not mutate caller text");
        }
    }
    DRW_Converter token(nullptr, 0);
    t.expect(token.encodeMifText("\\M+193FA") == "日"
                 && token.encodeMifText("\\M+193FAB").empty(),
             "the internal MIF helper admits exactly one eight-byte token");
}

void testMifLiteralModelRoundTrip(TestContext& t) {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const std::string literal = "before\\M+48861after";
    // The six-byte "before" prefix starts the token at offset247, splitting
    // "\\M+" from "48861" at the MTEXT250-byte chunk boundary. This checks
    // payload preservation and stored command text, not text rendering.
    const std::string multiline = std::string(241, 'A') + literal + "\\Ptail";
    for (const DRW::Version version : {DRW::AC1018, DRW::AC1021}) {
        for (const bool binary : {false, true}) {
            const auto path = std::filesystem::temp_directory_path()
                / ("libdxfrw-mif-literal-" + std::to_string(stamp) + "-"
                   + std::to_string(static_cast<int>(version))
                   + (binary ? "-binary.dxf" : "-ascii.dxf"));
            struct Cleanup {
                std::filesystem::path path;
                ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
            } cleanup{path};
            auto source = std::make_unique<dx_data>();
            source->headerC.addStr("$DWGCODEPAGE", "ANSI_1252", 3);
            auto* text = new DRW_Text();
            text->height = 1;
            text->text = literal;
            source->mBlock->ent.push_back(text);
            auto* mtext = new DRW_MText();
            mtext->height = 1;
            mtext->text = multiline;
            source->mBlock->ent.push_back(mtext);
            auto* line = new DRW_Line();
            line->basePoint = {34, 35, 36};
            line->secPoint = {37, 38, 39};
            source->mBlock->ent.push_back(line);
            for (int pass = 0; pass != 2; ++pass) {
                PreparedDxInterface exporter;
                exporter.attach(source.get());
                t.expect(exporter.fileExport(path.string(), version, binary, source.get(), false),
                         "literal unavailable MIF exports in legacy/UTF8 ASCII/binary drawings");
                std::vector<MTextWireChunk> chunks;
                t.expect(readMTextWireChunks(path, binary, chunks) && chunks.size() == 2
                             && chunks[0].code == 3 && chunks[0].value.size() == 250
                             && chunks[0].value == multiline.substr(0, 250)
                             && chunks[0].value.substr(247) == "\\M+"
                             && chunks[1].code == 1 && chunks[1].value == multiline.substr(250)
                             && chunks[1].value.compare(0, 5, "48861") == 0,
                         "actual MTEXT group3/group1 payloads straddle the unavailable MIF token before readback");
                auto imported = std::make_unique<dx_data>();
                PreparedDxInterface reader;
                reader.attach(imported.get());
                const bool read = reader.fileImport(path.string(), imported.get(), false);
                t.expect(read && imported->mBlock != nullptr && imported->mBlock->ent.size() == 3,
                         "repeated MIF handoff retains both text records and the following LINE");
                if (!read || imported->mBlock == nullptr || imported->mBlock->ent.size() != 3)
                    break;
                auto entity = imported->mBlock->ent.begin();
                const auto* decodedText = dynamic_cast<const DRW_Text*>(*entity++);
                const auto* decodedMText = dynamic_cast<const DRW_MText*>(*entity++);
                const auto* decodedLine = dynamic_cast<const DRW_Line*>(*entity);
                t.expect(decodedText != nullptr && decodedText->eType == DRW::TEXT
                             && decodedText->text == literal && decodedMText != nullptr
                             && decodedMText->text == multiline && decodedLine != nullptr
                             && decodedLine->eType == DRW::LINE && decodedLine->basePoint.x == 34
                             && decodedLine->secPoint.z == 39,
                         "literal MIF remains exact through TEXT/MTEXT chunk assembly and successive reads");
                source = std::move(imported);
            }
        }
    }
}

std::uint32_t textIssueMask(std::initializer_list<DRW_DxfTextIssue> issues) {
    std::uint32_t mask = 0;
    for (auto issue : issues) mask |= DRW::dxfTextIssueMask(issue);
    return mask;
}

void testTextDecodeObservations(TestContext& t) {
    static_assert(std::is_trivially_copyable<DRW_DxfTextReadObservations>::value,
                  "text observations must remain fixed-size owned values");
    using Issue = DRW_DxfTextIssue;
    struct Row {
        const char* page;
        bool modern;
        std::string bytes, expected;
        std::uint32_t issues;
    };
    const Row rows[] = {
        {"ANSI_1252", false, "L\xE9R", "L\xC3\xA9R", 0},
        {"ANSI_1252", false, "L\x81R", "LR", textIssueMask({Issue::UndefinedByteSequence, Issue::DiscardedSource})},
        {"ISO8859-4", false, "L\xA1R", "LR", textIssueMask({Issue::CompatibilityCodePage, Issue::UndefinedByteSequence, Issue::DiscardedSource})},
        {"ISO8859-5", false, "L\xB0R", "L\xC2\xB0R", textIssueMask({Issue::CompatibilityCodePage})},
        {"CP866", false, "L\x80R", "L\xE2\x82\xACR", textIssueMask({Issue::UnavailableCodePage})},
        {"local-unknown", false, "L\x80R", "L\xE2\x82\xACR", textIssueMask({Issue::UnknownCodePage})},
        {nullptr, false, "L\xE9R", "L\xC3\xA9R", textIssueMask({Issue::UnspecifiedCodePage})},
        {"ISO8859-4", false, "plain", "plain", 0},
        {"local-unknown", false, "\\U+0410", "\xD0\x90", 0},
        {"ANSI_1252", false, "\\U+0000", "\\U+0000", textIssueMask({Issue::UnrepresentedEscape})},
        {"ANSI_1252", false, "\\U+ZZZZ", "\\U+ZZZZ", textIssueMask({Issue::MalformedEscape, Issue::UnrepresentedEscape})},
        {"ANSI_1252", false, "\\U+12G4", "\\U+12G4", textIssueMask({Issue::MalformedEscape, Issue::UnrepresentedEscape})},
        {"ANSI_1252", false, "\\U+    ", "\\U+    ", textIssueMask({Issue::MalformedEscape, Issue::UnrepresentedEscape})},
        {"ANSI_936", false, "\\U+12G4", "\\U+12G4", textIssueMask({Issue::MalformedEscape, Issue::UnrepresentedEscape})},
        {"ANSI_1252", false, "\\U+D800", "\\U+D800", textIssueMask({Issue::InvalidUnicodeOutput, Issue::UnrepresentedEscape})},
        {"ANSI_1252", false, "\\U+DFFF", "\\U+DFFF", textIssueMask({Issue::InvalidUnicodeOutput, Issue::UnrepresentedEscape})},
        {"ANSI_1252", false, "\\U+D83D\\U+DE00", "\\U+D83D\\U+DE00", textIssueMask({Issue::InvalidUnicodeOutput, Issue::UnrepresentedEscape})},
        {"ANSI_1252", false, "\\U+12", "\\U+12", textIssueMask({Issue::MalformedEscape, Issue::UnrepresentedEscape})},
        {"ANSI_1252", false, "\\U+0041B", "AB", 0},
        {"ANSI_1252", false, "\\U+0041\\U+0042", "AB", 0},
        {"ANSI_932", false, "ordinary \\path", "ordinary \\path", 0},
        {"ANSI_1252", false, "\\M+48861", "\\M+48861", textIssueMask({Issue::UnrepresentedEscape})},
        {"ANSI_1252", false, "\\M+193FG", "\\M+193FG", textIssueMask({Issue::MalformedEscape, Issue::UnrepresentedEscape})},
        {"ANSI_1252", false, "\\M+10080", "\\M+10080", textIssueMask({Issue::MalformedEscape, Issue::UnrepresentedEscape})},
        {"local-unknown", false, "\\M+182A0", "\xE3\x81\x82", 0},
        {"ANSI_932", false, "L\x82", "L\xE3\x83\xBB", textIssueMask({Issue::UndefinedByteSequence, Issue::ReplacedSource})},
        {"ANSI_936", false, "L\x81", "L?", textIssueMask({Issue::UndefinedByteSequence, Issue::ReplacedSource})},
        {"ANSI_950", false, "\x88\x62", "\xC3\x8A\xCC\x84", 0},
        {"ANSI_932", false, "?", "?", 0},
        {"ANSI_1252", false, "", "", 0},
        {"ANSI_1252", false, "\\M+1FFFF", "\xE3\x83\xBB", textIssueMask({Issue::UndefinedByteSequence, Issue::ReplacedSource})},
        {"ANSI_1252", true, "\xEF\xBF\xBD", "\xEF\xBF\xBD", 0},
        {"local-unknown", true, "\xC3\xA9", "\xC3\xA9", 0},
        {"CP866", true, "\xF0\x9F\x98\x80", "\xF0\x9F\x98\x80", 0},
        {"ANSI_1252", true, "\xC0\x80", "\xC0\x80", textIssueMask({Issue::InvalidUtf8, Issue::InvalidUnicodeOutput})},
        {"ANSI_1252", true, "\xED\xA0\x80", "\xED\xA0\x80", textIssueMask({Issue::InvalidUtf8, Issue::InvalidUnicodeOutput})},
        {"ANSI_1252", true, "\xF4\x90\x80\x80", "\xF4\x90\x80\x80", textIssueMask({Issue::InvalidUtf8, Issue::InvalidUnicodeOutput})},
        {"ANSI_1252", true, "L\x80R", "L\x80R", textIssueMask({Issue::InvalidUtf8, Issue::InvalidUnicodeOutput})},
        {"ANSI_1252", true, "L\xE2", "L\xE2", textIssueMask({Issue::InvalidUtf8, Issue::InvalidUnicodeOutput})},
        {"ANSI_1252", true, "\\M+48861", "\\M+48861", textIssueMask({Issue::UnrepresentedEscape})},
    };
    std::size_t cases = 0;
    for (const auto& row : rows) {
        DRW_TextCodec codec;
        codec.setVersion(row.modern ? DRW::AC1024 : DRW::AC1015, true);
        if (row.page != nullptr) codec.setCodePage(row.page, true);
        const auto decoded = codec.toUtf8(row.bytes);
        const auto observed = codec.lastDecodeObservation();
        t.expect(decoded == row.expected && observed.issues == row.issues
                     && codec.accumulatedDecodeIssues() == row.issues
                     && observed.legacyBytes == !row.modern,
                 "source observation preserves old decoder output and reports only evidenced events");
        if (decoded != row.expected || observed.issues != row.issues)
            std::cerr << "Observation row " << cases << " issues=" << observed.issues
                      << " expected=" << row.issues << '\n';
        t.expect(codec.toUtf8("clean") == "clean" && codec.lastDecodeObservation().issues == 0
                     && codec.accumulatedDecodeIssues() == row.issues
                     && !codec.lastDecodeObservation().nonAsciiBytes,
                 "next conversion resets per-value events without inventing loss from result characters");
        codec.resetAccumulatedDecodeIssues();
        t.expect(codec.accumulatedDecodeIssues() == 0u
                     && codec.toUtf8("clean") == "clean" && codec.accumulatedDecodeIssues() == 0u,
                 "explicit source-frame reset isolates the next decode envelope");
        ++cases;
    }
    DRW_TextCodec utf16;
    utf16.setVersion(DRW::AC1024, false);
    const std::pair<std::string, std::uint32_t> utf16Rows[] = {
        {std::string("\x3D\xD8\0\xDE", 4), 0},
        {std::string("\0\xD8", 2), textIssueMask({Issue::MalformedUtf16, Issue::ReplacedSource})},
        {std::string("\xFD\xFF", 2), 0},
        {"A", textIssueMask({Issue::MalformedUtf16, Issue::DiscardedSource})}
    };
    std::uint32_t accumulatedUtf16Issues = 0u;
    for (const auto& row : utf16Rows) {
        (void)utf16.toUtf8(row.first);
        accumulatedUtf16Issues |= row.second;
        t.expect(utf16.lastDecodeObservation().issues == row.second
                     && utf16.accumulatedDecodeIssues() == accumulatedUtf16Issues
                     && !utf16.lastDecodeObservation().legacyBytes,
                 "UTF16 malformed units are observed separately from valid replacement scalars");
    }
    DRW_TextCodec recovery;
    recovery.setVersion(DRW::AC1015, true);
    recovery.setCodePage("ANSI_1252", true);
    recovery.setDxfTextEncoding(DRW::DxfTextEncoding::LegacyLibreCadUtf8);
    t.expect(recovery.toUtf8("\xC3\xA9") == "\xC3\xA9"
                 && recovery.lastDecodeObservation().recovery
                 && !recovery.lastDecodeObservation().legacyBytes
                 && recovery.lastDecodeObservation().issues == 0,
             "explicit recovery observes its actual UTF8 path rather than unused legacy mapping");
    DRW_TextCodec other;
    (void)other.toUtf8("\x80");
    const auto prior = other.lastDecodeObservation();
    other.swap(recovery);
    DRW_TextCodec moved(std::move(recovery));
    t.expect(moved.lastDecodeObservation().issues == prior.issues
                 && moved.accumulatedDecodeIssues() == prior.issues
                 && other.accumulatedDecodeIssues() == 0u
                 && other.lastDecodeObservation().recovery,
             "last-value snapshots swap/move with their codec state without borrowing");
    DRW_TextCodec mixed;
    mixed.setVersion(DRW::AC1021, false);
    mixed.setByteCodePage("ANSI_1252");
    (void)mixed.toUtf8(std::string("\0\xD8", 2));
    const auto unicodeIssues = textIssueMask({Issue::MalformedUtf16, Issue::ReplacedSource});
    t.expect(mixed.toUtf8CP8("clean") == "clean" && mixed.lastDecodeObservation().issues == 0u
                 && mixed.accumulatedDecodeIssues() == unicodeIssues,
             "clean byte-channel decode cannot erase a prior UTF16 issue");
    (void)mixed.toUtf8CP8("\x81");
    t.expect(mixed.accumulatedDecodeIssues() == (unicodeIssues
                 | textIssueMask({Issue::UndefinedByteSequence, Issue::DiscardedSource})),
             "primary and byte-channel losses share one bounded issue envelope");
    mixed.resetAccumulatedDecodeIssues();
    t.expect(mixed.accumulatedDecodeIssues() == 0u && mixed.lastDecodeObservation().issues != 0u,
             "aggregate reset does not rewrite the last-value observation");
    DRW_TextCodec unprofiled;
    t.expect(unprofiled.toUtf8CP8("\xE9") == "\xE9"
                 && unprofiled.lastDecodeObservation().issues == 0u
                 && unprofiled.accumulatedDecodeIssues() == textIssueMask({Issue::UnsupportedEncodingProfile}),
             "unprofiled CP8 preserves tolerant output but cannot certify its high bytes");
    DRW_TextCodec exact;
    exact.setVersion(DRW::AC1018, true);
    exact.setCodePage("ANSI_1252", true);
    exact.setDxfCodePagePolicy(DRW::DxfCodePagePolicy::ExactNamed);
    (void)exact.toUtf8("\x81");
    t.expect(exact.accumulatedDecodeIssues() == textIssueMask({Issue::UndefinedByteSequence, Issue::DiscardedSource})
                 && exact.toUtf8("clean") == "clean" && exact.accumulatedDecodeIssues() != 0u,
             "exact named decoder early-return path also contributes to the aggregate");
    std::cout << "Text decoder observations: literal rows=" << cases << ", UTF16 controls=4\n";
}

void testDwgLegacyTextNormalization(TestContext& t) {
    struct Row { std::string bytes, expected; bool collapsed; };
    const Row rows[] = {
        {"", "", false}, {"A", "A", false}, {std::string("A\0", 2), "A", false},
        {std::string("A\0\0", 3), "A", true}, {std::string("A\0B", 3), std::string("A\0B", 3), false},
        {std::string("A\0B\0", 4), std::string("A\0B", 3), false},
        {std::string("A\0B\0\0", 5), std::string("A\0B", 3), true}
    };
    unsigned cells = 0u;
    for (const auto version : {DRW::AC1014, DRW::AC1015, DRW::AC1018}) for (const auto& row : rows) {
      for (unsigned route = 0u; route < 3u; ++route) {
        dwgBufferW encoded;
        encoded.putBitShort(static_cast<std::uint16_t>(row.bytes.size()));
        encoded.putBytes(reinterpret_cast<const std::uint8_t*>(row.bytes.data()), row.bytes.size());
        const auto boundary = encoded.bitCount();
        encoded.putRawShort16(0xA55Au);
        DRW_TextCodec codec;
        codec.setVersion(version, false);
        codec.setCodePage("ANSI_1252", false);
        codec.setByteCodePage("ANSI_936");
        dwgBuffer buffer(encoded.data().data(), encoded.data().size(), &codec);
        const auto value = route == 0u ? buffer.getCP8Text()
            : route == 1u ? buffer.getVariableText(version, false) : buffer.getENCText();
        const auto expectedIssues = row.collapsed
            ? DRW::dxfTextIssueMask(DRW::DxfTextIssue::DiscardedSource) : 0u;
        t.expect(value == row.expected && buffer.getPosition() * 8u + buffer.getBitPos() == boundary
                     && buffer.getRawShort16() == 0xA55Au && buffer.isGood()
                     && codec.accumulatedDecodeIssues() == expectedIssues,
                 "legacy T/TV/CP8 retains its previous string/cursor while observing repeated terminal-NUL collapse");
        t.expect(codec.toUtf8("clean") == "clean" && codec.lastDecodeObservation().issues == 0u
                     && codec.accumulatedDecodeIssues() == expectedIssues,
                 "a clean decode cannot erase a legacy boundary normalization issue");
        ++cells;
      }
    }
    std::cout << "Legacy text normalization cells=" << cells << '\n';
}

void testDwgEedTextProvenance(TestContext& t) {
    using Issue = DRW::DxfTextIssue;
    const auto discarded = textIssueMask({Issue::UndefinedByteSequence, Issue::DiscardedSource});
    const auto replaced = textIssueMask({Issue::UndefinedByteSequence, Issue::ReplacedSource});
    const auto unknown = textIssueMask({Issue::UnknownCodePage});
    const auto alias = textIssueMask({Issue::CompatibilityCodePage});
    struct Row { std::uint16_t page; std::string raw, expected; std::uint32_t issues; };
    const Row rows[] = {
        {30u, "L\x81R", "LR", discarded}, {30u, "LR", "LR", 0u}, {30u, "", "", 0u},
        {30u, "L\xE9R", "L\xC3\xA9R", 0u},
        {39u, "L\x81", "L?", replaced}, {39u, "L?", "L?", 0u},
        {39u, "\xD6\xD0", "\xE4\xB8\xAD", 0u},
        {31u, "\xD6\xD0", "\xE4\xB8\xAD", alias}, {31u, "plain", "plain", alias}, {31u, "", "", alias},
        {999u, "L\xE9R", "L\xC3\xA9R", unknown}, {0u, "plain", "plain", unknown},
        {255u, "", "", unknown}, {1252u, "plain", "plain", unknown},
        {29u, "\xCF", "\xD0\x9F", 0u}, {38u, "L\x82", "L\xE3\x83\xBB", replaced},
        {38u, "?", "?", 0u}, {999u, "", "", unknown}
    };
    for (const auto& row : rows) {
        DRW_TextCodec file;
        file.setVersion(DRW::AC1018, false);
        file.setCodePage("ANSI_1252", false);
        file.setByteCodePage("ANSI_936");
        (void)file.toUtf8("prior");
        const auto byteSelection = file.byteCodePageSelection();
        const auto actual = decodeEedString(row.page, row.raw, &file);
        t.expect(actual == row.expected && file.accumulatedDecodeIssues() == row.issues
                     && file.getCodePage() == "ANSI_1252"
                     && file.byteCodePageSelection().kind == byteSelection.kind
                     && file.byteCodePageSelection().selectedName == byteSelection.selectedName,
                 "per-value EED conversion retains tolerant text and file codecs while contributing source-profile/loss issues");
        t.expect(file.toUtf8("clean") == "clean" && file.lastDecodeObservation().issues == 0u
                     && file.accumulatedDecodeIssues() == row.issues,
                 "later clean file text cannot hide an earlier EED issue");
        file.resetAccumulatedDecodeIssues();
        t.expect(decodeEedString(30u, "next", &file) == "next" && file.accumulatedDecodeIssues() == 0u,
                 "explicit frame reset isolates the following EED text");
    }
    DRW_TextCodec file;
    file.setVersion(DRW::AC1018, false);
    file.setCodePage("ANSI_1252", false);
    (void)file.toUtf8("\x81");
    const auto last = file.lastDecodeObservation();
    file.resetAccumulatedDecodeIssues();
    t.expect(decodeEedString(30u, "clean", &file) == "clean"
                 && file.accumulatedDecodeIssues() == 0u && file.lastDecodeObservation().issues == last.issues,
             "merging subordinate EED events does not reuse a stale primary snapshot");
    (void)decodeEedString(999u, "plain", &file);
    (void)decodeEedString(39u, "\x81", &file);
    t.expect(file.accumulatedDecodeIssues() == (unknown | replaced),
             "independent per-value EED profiles contribute to the same bounded source envelope");
    t.expect(decodeEedString(30u, "L\x81R", nullptr) == "LR"
                 && decodeEedString(999u, "\xE9", nullptr) == "\xE9",
             "legacy EED helper without an observation sink retains its existing result");

    // ODA section28: Unicode EED is counted in complete UTF16 code units.
    // This decoder already refuses malformed units rather than replacing
    // them. Retain that distinction and its all-or-nothing chunk publication.
    struct UnicodeRow { std::vector<std::uint8_t> bytes; std::uint16_t units; bool valid; std::string text; };
    const UnicodeRow unicode[] = {
        {{0xFDu, 0xFFu}, 1u, true, "\xEF\xBF\xBD"},
        {{0x3Du, 0xD8u, 0u, 0xDEu}, 2u, true, "\xF0\x9F\x98\x80"},
        {{0u, 0u}, 1u, true, std::string(1u, '\0')},
        {{0u, 0xD8u}, 1u, false, ""}, {{0u, 0xDCu}, 1u, false, ""},
        {{'A'}, 1u, false, ""}
    };
    for (const auto& row : unicode) {
        dwgBufferW item;
        item.putRawChar8(0u);
        item.putRawShort16(row.units);
        item.putBytes(row.bytes.data(), row.bytes.size());
        dwgBufferW encoded;
        encoded.putBitShort(static_cast<std::uint16_t>(item.data().size()));
        dwgHandle app; app.code = 5u; app.ref = 0x123u;
        encoded.putHandle(app);
        encoded.putBytes(item.data().data(), item.data().size());
        encoded.putBitShort(0u);
        encoded.putRawLong32(0x13579BDFu);
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1021, false);
        dwgBuffer buffer(encoded.data().data(), encoded.data().size(), &codec);
        std::vector<DwgEedChunk> chunks(1u);
        chunks.front().appHandle = 0xDEADu;
        const bool read = readDwgEed(DRW::AC1021, buffer, chunks);
        t.expect(read == row.valid, "Unicode EED still distinguishes valid replacement/astral/NUL scalars from malformed units");
        if (read) {
            t.expect(chunks.size() == 1u && chunks[0].appHandle == 0x123u && chunks[0].items.size() == 1u
                         && chunks[0].items[0].content.s && *chunks[0].items[0].content.s == row.text
                         && buffer.getRawLong32() == 0x13579BDFu && codec.accumulatedDecodeIssues() == 0u,
                     "valid Unicode EED preserves exact text and the following field without inventing replacement loss");
        } else {
            t.expect(chunks.size() == 1u && chunks[0].appHandle == 0xDEADu && chunks[0].items.empty(),
                     "malformed Unicode EED publishes no partial replacement for the caller's prior chunks");
        }
    }
    std::cout << "DWG per-value EED provenance rows=18 Unicode structural controls=6\n";
}

void testAppIdNameSource(TestContext& t) {
    using Issue = DRW::DxfTextIssue;
    const auto lost = textIssueMask({Issue::UndefinedByteSequence, Issue::DiscardedSource});
    const auto unicodeLoss = textIssueMask({Issue::MalformedUtf16, Issue::ReplacedSource});
    unsigned cells = 0u;
    for (const auto version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                              DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
      const bool modern = version > DRW::AC1018;
      for (unsigned row = 0u; row < 4u; ++row) {
        const std::string clean = modern ? appid_test::unicodeBytes("AB") : "AB";
        const std::string raw = row == 0u ? clean : modern
            ? appid_test::unicodeBytes("A") + std::string("\0\xD8", 2) + appid_test::unicodeBytes("B")
            : "A\x81" "B";
        const auto expectedIssues = row == 0u ? 0u : modern ? unicodeLoss : lost;
        const std::string expected = row == 0u || !modern ? "AB" : "A\xEF\xBF\xBD" "B";
        const auto encoded = appid_test::frame(version, raw);
        DRW_TextCodec codec;
        codec.setVersion(version, false);
        codec.setByteCodePage("ANSI_1252");
        if (!modern) codec.setCodePage("ANSI_1252", false);
        if (row >= 2u) codec.mergeDecodeIssues(expectedIssues);
        const auto prior = codec.accumulatedDecodeIssues();
        dwgBuffer body(encoded.bytes.data(), encoded.bytes.size(), row == 3u ? nullptr : &codec);
        appid_test::Probe parsed;
        const bool read = parsed.parseDwg(version, &body, encoded.handleBits);
        t.expect(read && body.isGood() && parsed.hasDwgNameSource()
                     && parsed.hasUnchangedDwgSourceName()
                     && parsed.dwgNameSourceRetainedBytes() > parsed.name.size()
                     && (row == 3u || (parsed.name == expected
                         && parsed.dwgNameDecodeIssues() == expectedIssues
                         && parsed.hasBoundDwgNameProfile(version, modern ? "UTF-16" : "ANSI_1252")
                         && codec.accumulatedDecodeIssues() == (prior | expectedIssues)))
                     && (row != 3u || !parsed.hasBoundDwgNameProfile(version, modern ? "UTF-16" : "ANSI_1252")),
                 "APPID retains immutable name bytes/profile and isolated issues, including a repeated prior issue");
        DRW_AppId copy(parsed), assigned;
        assigned = parsed;
        t.expect(copy.sameDwgNameSource(parsed) && assigned.sameDwgNameSource(parsed),
                 "APPID copies share exact private source evidence without relying on editable fields");
        copy.name += "edited";
        assigned.handle += 1u;
        t.expect(!copy.hasUnchangedDwgSourceName() && !assigned.hasUnchangedDwgSourceName()
                     && parsed.hasUnchangedDwgSourceName(),
                 "public APPID name/handle edits do not refresh immutable source evidence");
        copy.reset();
        t.expect(!copy.hasDwgNameSource() && copy.dwgNameSourceRetainedBytes() == 0u,
                 "APPID reset releases its source owner without changing another copy");
        std::vector<std::uint8_t> truncated(encoded.bytes.begin(), encoded.bytes.begin() + 1u);
        dwgBuffer bad(truncated.data(), truncated.size(), &codec);
        t.expect(!parsed.parseDwg(version, &bad, encoded.handleBits)
                     && !parsed.hasDwgNameSource() && parsed.name.empty(),
                 "failed APPID reuse publishes no stale name/source observation");
        ++cells;
      }
    }
    DRW_AppId a, b;
    t.expect(appid_test::parse(DRW::AC1018, "A\x81" "B", "ANSI_1252", a)
                 && appid_test::parse(DRW::AC1018, "AB", "ANSI_1252", b)
                 && a.name == b.name && !a.sameDwgNameSource(b),
             "equal decoded APPID names do not equate different source bytes/issues");
    for (const auto version : {DRW::AC1018, DRW::AC1021}) {
        const bool modern = version > DRW::AC1018;
        const auto first = modern ? appid_test::unicodeBytes("A") + std::string("\0\xD8", 2)
                                  : std::string("A\x81", 2);
        const auto second = modern ? appid_test::unicodeBytes("A") + std::string("\0\xDC", 2)
                                   : std::string("A\x81\x81", 3);
        t.expect(appid_test::parse(version, first, "ANSI_1252", a)
                     && appid_test::parse(version, second, "ANSI_1252", b)
                     && a.name == b.name && a.dwgNameDecodeIssues() == b.dwgNameDecodeIssues()
                     && a.hasUnchangedDwgSourceName() && b.hasUnchangedDwgSourceName()
                     && !a.sameDwgNameSource(b),
                 "owned APPID source bytes distinguish equal decoded names with identical issue masks after input destruction");
    }
    t.expect(appid_test::parse(DRW::AC1018, "APP", "ANSI_1252", a)
                 && appid_test::parse(DRW::AC1018, std::string("APP\0", 4), "ANSI_1252", b)
                 && a.name == b.name && a.dwgNameDecodeIssues() == 0u && b.dwgNameDecodeIssues() == 0u
                 && !a.sameDwgNameSource(b),
             "ordinary single-terminator normalization does not erase exact APPID source-field identity");
    for (const auto* page : {"NOT_A_DWG_PAGE", "LATIN1"}) for (const auto* text : {"", "APP"}) {
        const auto issue = DRW::dxfTextIssueMask(std::string(page) == "LATIN1"
            ? Issue::CompatibilityCodePage : Issue::UnknownCodePage);
        t.expect(appid_test::parse(DRW::AC1018, text, page, a)
                     && a.name == text && a.dwgNameDecodeIssues() == issue,
                 "empty and ASCII APPID names retain unknown/aliased legacy profile observations");
    }
    for (const auto version : {DRW::AC1018, DRW::AC1021}) {
        const auto frame = appid_test::frame(version, version > DRW::AC1018 ? appid_test::unicodeBytes("APP") : "APP");
        DRW_TextCodec codec; codec.setVersion(version, false);
        if (version <= DRW::AC1018) codec.setCodePage("ANSI_1252", false);
        codec.mergeDecodeIssues(lost);
        dwgBuffer body(frame.bytes.data(), frame.bytes.size(), &codec);
        appid_test::Probe parsed;
        t.expect(parsed.parseDwg(version, &body, frame.handleBits)
                     && parsed.dwgNameDecodeIssues() == 0u
                     && codec.accumulatedDecodeIssues() == lost,
                 "clean APPID name evidence is isolated from prior common-field loss without erasing it globally");
        t.expect(appid_test::parse(version, version > DRW::AC1018 ? appid_test::unicodeBytes("APP") : "APP",
                                  "ANSI_1252", a, 0x123u, 68u)
                     && !a.hasBoundDwgNameProfile(version, version > DRW::AC1018 ? "UTF-16" : "ANSI_1252"),
                 "tolerant name parsing under another fixed table type cannot qualify an APPID source profile");
    }
    for (const auto version : {DRW::AC1021, DRW::AC1032}) {
        t.expect(appid_test::parse(version, appid_test::unicodeBytes("ABCD"), "ANSI_1252", a, 0x123u, 67u, true)
                     && a.name == "AB" && a.dwgNameDecodeIssues() == 0u
                     && !a.hasBoundDwgNameProfile(version, "UTF-16"),
                 "a speculative byte-length mode cannot qualify a partially consumed counted TU name with clean decoded text");
    }
    std::cout << "APPID name-source cells=" << cells << " equal-output controls=4 profile controls=4 prior-issue controls=2 wrong-type controls=2 counted-width controls=2\n";
}

void testLayerNameSource(TestContext& t) {
    using Issue = DRW::DxfTextIssue;
    unsigned cells = 0u;
    for (const auto version : {DRW::AC1015, DRW::AC1018, DRW::AC1021,
                              DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
      const bool modern = version > DRW::AC1018;
      for (unsigned row = 0u; row < 4u; ++row) {
        const auto issues = row == 0u ? 0u : modern
            ? textIssueMask({Issue::MalformedUtf16, Issue::ReplacedSource})
            : textIssueMask({Issue::UndefinedByteSequence, Issue::DiscardedSource});
        const std::string raw = row == 0u ? modern ? appid_test::unicodeBytes("AB") : "AB"
            : modern ? appid_test::unicodeBytes("A") + std::string("\0\xD8", 2) + appid_test::unicodeBytes("B")
            : "A\x81" "B";
        const std::string expected = row == 0u || !modern ? "AB" : "A\xEF\xBF\xBD" "B";
        const auto encoded = layer_test::frame(version, raw);
        DRW_TextCodec codec; codec.setVersion(version, false); codec.setByteCodePage("ANSI_1252");
        if (!modern) codec.setCodePage("ANSI_1252", false);
        if (row >= 2u) codec.mergeDecodeIssues(issues);
        dwgBuffer body(encoded.bytes.data(), encoded.bytes.size(), row == 3u ? nullptr : &codec);
        layer_test::Probe parsed;
        t.expect(parsed.parseDwg(version, &body, encoded.handleBits) && body.isGood()
                     && parsed.hasUnchangedDwgSourceName() && parsed.dwgNameSourceRetainedBytes() > 0u
                     && (row == 3u || (parsed.name == expected && parsed.dwgNameDecodeIssues() == issues
                         && codec.accumulatedDecodeIssues() == issues
                         && parsed.hasBoundDwgNameProfile(version, modern ? "UTF-16" : "ANSI_1252")))
                     && (row != 3u || !parsed.hasBoundDwgNameProfile(version, modern ? "UTF-16" : "ANSI_1252")),
                 "LAYER name source isolates exact raw/profile issues without changing tolerant text or following fields");
        DRW_Layer copy(parsed), assigned; assigned = parsed;
        t.expect(copy.sameDwgNameSource(parsed) && assigned.sameDwgNameSource(parsed), "LAYER copies retain exact immutable name ownership");
        copy.name += "EDIT"; assigned.handle += 1u;
        t.expect(!copy.hasUnchangedDwgSourceName() && !assigned.hasUnchangedDwgSourceName(), "LAYER name/handle edits do not refresh source evidence");
        copy.reset();
        t.expect(!copy.hasDwgNameSource(), "LAYER reset releases private name evidence");
        const std::uint8_t shortFrame[] = {0u};
        dwgBuffer bad(shortFrame, sizeof(shortFrame), &codec);
        t.expect(!parsed.parseDwg(version, &bad, encoded.handleBits) && !parsed.hasDwgNameSource(), "failed LAYER reuse clears its source owner");
        ++cells;
      }
    }
    for (const auto version : {DRW::AC1018, DRW::AC1021}) {
        const bool modern = version > DRW::AC1018;
        const auto clean = modern ? appid_test::unicodeBytes("AB") : "AB";
        const auto badColor = modern ? std::string("\0\xD8", 2) : std::string("C\x81", 2);
        const auto encoded = layer_test::frame(version, clean, 0x234u, 51u, badColor);
        DRW_TextCodec codec; codec.setVersion(version, false); codec.setByteCodePage("ANSI_1252");
        if (!modern) codec.setCodePage("ANSI_1252", false);
        dwgBuffer body(encoded.bytes.data(), encoded.bytes.size(), &codec);
        layer_test::Probe parsed;
        t.expect(parsed.parseDwg(version, &body, encoded.handleBits) && parsed.name == "AB"
                     && parsed.dwgNameDecodeIssues() == 0u && codec.accumulatedDecodeIssues() != 0u,
                 "later CMC name loss remains globally observed without contaminating the independent layer-name field");
    }
    DRW_Layer a, b;
    t.expect(layer_test::parse(DRW::AC1018, "A\x81" "B", "ANSI_1252", a)
                 && layer_test::parse(DRW::AC1018, "A\x81\x81" "B", "ANSI_1252", b)
                 && a.name == b.name && a.dwgNameDecodeIssues() == b.dwgNameDecodeIssues()
                 && !a.sameDwgNameSource(b), "equal LAYER names and issue masks do not equate different source bits");
    for (const auto* page : {"NOT_A_PAGE", "LATIN1"}) for (const auto* text : {"", "AB"}) {
        t.expect(layer_test::parse(DRW::AC1018, text, page, a)
                     && a.dwgNameDecodeIssues() == DRW::dxfTextIssueMask(std::string(page) == "LATIN1"
                         ? Issue::CompatibilityCodePage : Issue::UnknownCodePage),
                 "empty/ASCII LAYER names keep declared legacy profile uncertainty");
    }
    for (const auto version : {DRW::AC1021, DRW::AC1032}) {
        t.expect(layer_test::parse(version, appid_test::unicodeBytes("ABCD"), "ANSI_1252", a, 0x234u, 51u, true)
                     && a.name == "AB" && !a.hasBoundDwgNameProfile(version, "UTF-16"), "partial counted LAYER name does not qualify as a full TU field");
        t.expect(layer_test::parse(version, appid_test::unicodeBytes("AB"), "ANSI_1252", a, 0x234u, 52u)
                     && !a.hasBoundDwgNameProfile(version, "UTF-16"), "another table type cannot qualify a LAYER name");
    }
    std::cout << "LAYER name-source cells=" << cells << " CMC isolation=2 equal-issue raw identity=1 profile=4 counted-width=2 wrong-type=2\n";
}

void testXRecordPerValueCodePage(TestContext& t) {
    using Issue = DRW::DxfTextIssue;
    const auto lost = textIssueMask({Issue::DiscardedSource});
    const auto undefined = textIssueMask({Issue::UndefinedByteSequence, Issue::DiscardedSource});
    const auto replaced = textIssueMask({Issue::UndefinedByteSequence, Issue::ReplacedSource});
    const auto unknown = textIssueMask({Issue::UnknownCodePage});
    const auto alias = textIssueMask({Issue::CompatibilityCodePage});
    struct Row {
        const char* filePage;
        std::uint8_t page;
        std::string raw, expected;
        std::uint32_t issues;
    };
    const Row rows[] = {
        {"ANSI_1252", 39u, "\xD6\xD0", "\xE4\xB8\xAD", 0u},
        {"ANSI_936", 30u, "\xE9", "\xC3\xA9", 0u},
        {"ANSI_1252", 29u, "\xCF", "\xD0\x9F", 0u},
        {"ANSI_1252", 30u, "L\x81R", "LR", undefined},
        {"ANSI_1252", 30u, "LR", "LR", 0u},
        {"ANSI_1252", 39u, "L\x81", "L?", replaced},
        {"ANSI_1252", 39u, "L?", "L?", 0u},
        {"ANSI_1252", 38u, "L\x82", "L\xE3\x83\xBB", replaced},
        {"ANSI_1252", 31u, "\xD6\xD0", "\xE4\xB8\xAD", alias},
        {"ANSI_1252", 31u, "plain", "plain", alias},
        {"ANSI_1252", 31u, "", "", alias},
        {"ANSI_1252", 255u, "\xE9", "\xC3\xA9", unknown},
        {"ANSI_1252", 255u, "plain", "plain", unknown},
        {"ANSI_1252", 255u, "", "", unknown},
        {"ANSI_1252", 0u, "\xE9", "\xC3\xA9", 0u},
        {"ANSI_936", 0u, "\xD6\xD0", "\xE4\xB8\xAD", 0u},
        {"ANSI_1252", 0u, "", "", 0u},
        {"ANSI_1252", 0u, std::string("A\0", 2), "A", lost},
        {"ANSI_1252", 30u, std::string("A\0\0", 3), "A", lost},
        {"ANSI_1252", 39u, std::string("A\0B", 3), std::string("A\0B", 3), 0u},
        {"ANSI_1252", 255u, std::string("A\0", 2), "A", unknown | lost}
    };
    unsigned count = 0u;
    for (const auto& row : rows) {
        // Independently authored application pairs retain the established RS
        // count, not common EED's RC count/two-byte codepage framing. The
        // disputed legacy count layout is deliberately not qualified here.
        dwgBufferW payload;
        payload.putRawShort16(1u);
        payload.putRawShort16(static_cast<std::uint16_t>(row.raw.size()));
        payload.putRawChar8(row.page);
        payload.putBytes(reinterpret_cast<const std::uint8_t*>(row.raw.data()), row.raw.size());
        payload.putRawShort16(2u);
        payload.putRawShort16(5u);
        payload.putRawChar8(0u);
        const std::string clean = "clean";
        payload.putBytes(reinterpret_cast<const std::uint8_t*>(clean.data()), clean.size());
        payload.putRawShort16(70u);
        payload.putRawShort16(0x1357u);
        const auto bytes = xrecord_test::frameWithData(DRW::AC1018, payload.data());
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1018, false);
        codec.setCodePage(row.filePage, false);
        codec.setByteCodePage("ANSI_932");
        const auto bytePage = codec.byteCodePageSelection();
        dwgBuffer body(bytes.data(), bytes.size(), &codec);
        xrecord_test::CodecProbe parsed;
        const bool read = parsed.parseDwg(DRW::AC1018, &body);
        t.expect(read && body.isGood() && parsed.m_dataEntries.size() == 3u
                     && parsed.m_dataEntries[0].content.s && *parsed.m_dataEntries[0].content.s == row.expected
                     && parsed.m_dataEntries[1].content.s && *parsed.m_dataEntries[1].content.s == "clean"
                     && parsed.m_dataEntries[2].content.i == 0x1357 && parsed.parentHandle == 0xCu
                     && codec.accumulatedDecodeIssues() == row.issues
                     && codec.lastDecodeObservation().issues == 0u
                     && codec.getCodePage() == row.filePage
                     && codec.byteCodePageSelection().selectedName == bytePage.selectedName,
                 "XRECORD per-value codepage/text issues survive a clean following field without changing file profiles");
        codec.resetAccumulatedDecodeIssues();
        const auto next = xrecord_test::frame(DRW::AC1018, false);
        dwgBuffer nextBody(next.data(), next.size(), &codec);
        t.expect(parsed.parseDwg(DRW::AC1018, &nextBody) && nextBody.isGood()
                     && codec.accumulatedDecodeIssues() == 0u,
                 "resetting the frame observation isolates the next XRECORD");
        dwgBufferW malformed;
        malformed.putRawShort16(1u);
        malformed.putRawShort16(0xFFFFu);
        malformed.putRawChar8(row.page);
        const auto bad = xrecord_test::frameWithData(DRW::AC1018, malformed.data());
        dwgBuffer badBody(bad.data(), bad.size(), &codec);
        t.expect(!parsed.parseDwg(DRW::AC1018, &badBody) && parsed.m_dataEntries.empty()
                     && parsed.m_values.empty(),
                 "oversized counted text cannot borrow following common handles or retain prior XRECORD values");
        ++count;
    }
    unsigned noSink = 0u;
    for (const auto page : {0u, 30u, 39u, 255u}) {
        const std::string raw = page == 39u ? "\xD6\xD0" : "\xE9";
        const std::string expected = page == 39u ? "\xE4\xB8\xAD" : page == 30u ? "\xC3\xA9" : raw;
        dwgBufferW payload;
        payload.putRawShort16(1u);
        payload.putRawShort16(static_cast<std::uint16_t>(raw.size()));
        payload.putRawChar8(static_cast<std::uint8_t>(page));
        payload.putBytes(reinterpret_cast<const std::uint8_t*>(raw.data()), raw.size());
        const auto bytes = xrecord_test::frameWithData(DRW::AC1018, payload.data());
        dwgBuffer body(bytes.data(), bytes.size(), nullptr);
        xrecord_test::CodecProbe parsed;
        t.expect(parsed.parseDwg(DRW::AC1018, &body) && body.isGood()
                     && parsed.m_dataEntries.size() == 1u && parsed.m_dataEntries[0].content.s
                     && *parsed.m_dataEntries[0].content.s == expected,
                 "known application codepages decode without a sink while default/unknown pages retain raw fallback");
        ++noSink;
    }
    std::cout << "XRECORD application-codepage rows=" << count
              << " malformed/reset controls=" << count << " no-sink controls=" << noSink << '\n';
}

void testDwgTextDecodeBoundaries(TestContext& t) {
    using Issue = DRW::DxfTextIssue;
    const auto lost = textIssueMask({Issue::DiscardedSource});
    const auto replaced = textIssueMask({Issue::MalformedUtf16, Issue::ReplacedSource});
    struct Row {
        std::vector<std::uint16_t> units;
        std::string expected;
        std::uint32_t issues;
    };
    const Row rows[] = {
        {{}, "", 0u}, {{'A'}, "A", 0u}, {{'A', 0u}, "A", 0u},
        {{'A', 0u, 0u}, "A", lost}, {{'A', 0u, 'B'}, "AB", lost},
        {{0xD800u, 0u}, "\xEF\xBF\xBD", replaced},
        {{0xFFFDu, 0u}, "\xEF\xBF\xBD", 0u},
        {{0xD83Du, 0xDE00u, 0u}, "\xF0\x9F\x98\x80", 0u},
        {{0xD800u, 'A', 0u}, "\xEF\xBF\xBD" "A", replaced}
    };
    for (const auto& row : rows) {
        dwgBufferW encoded;
        encoded.putBitShort(static_cast<std::uint16_t>(row.units.size()));
        for (const auto unit : row.units) encoded.putRawShort16(unit);
        const auto boundary = encoded.bitCount();
        encoded.putVariableText(DRW::AC1021, "clean");
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1021, false);
        dwgBuffer buffer(encoded.data().data(), encoded.data().size(), &codec);
        const auto value = buffer.getVariableText(DRW::AC1021, false);
        t.expect(buffer.isGood() && value == row.expected
                     && buffer.getPosition() * 8u + buffer.getBitPos() == boundary
                     && codec.accumulatedDecodeIssues() == row.issues,
                 "framed TU removes one terminator without masking embedded/repeated NUL or surrogate loss");
        t.expect(buffer.getVariableText(DRW::AC1021, false) == "clean"
                     && codec.lastDecodeObservation().issues == 0u
                     && codec.accumulatedDecodeIssues() == row.issues,
                 "next correctly framed TU preserves the prior source issue envelope");
    }
    for (const bool included : {false, true}) {
        dwgBufferW encoded;
        encoded.putBitShort(included ? 2u : 1u);
        encoded.putRawShort16('A');
        encoded.putRawShort16(0u);
        const auto boundary = encoded.bitCount();
        encoded.putVariableText(DRW::AC1021, "next");
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1021, false);
        dwgBuffer buffer(encoded.data().data(), encoded.data().size(), &codec);
        t.expect(buffer.getVariableText(DRW::AC1021, true) == "A"
                     && buffer.getPosition() * 8u + buffer.getBitPos() == boundary
                     && buffer.getVariableText(DRW::AC1021, false) == "next"
                     && codec.accumulatedDecodeIssues() == 0u && buffer.isGood(),
                 "included or separately appended TU terminator keeps its original cursor contract");
    }
    std::cout << "DWG framed text-boundary cells=11\n";
}

void testTextObservationReaderRoles(TestContext& t) {
    using Issue = DRW_DxfTextIssue;
    const int codes[] = {0, 5, 9, 100, 102, 105, 310, 330, 999, 1002, 1005, 1, 1000, 1001, 1003, 430};
    std::size_t cases = 0;
    for (int code : codes) {
        const bool semantic = code == 1 || code == 1000 || code == 1001 || code == 1003 || code == 430;
        const bool handle = isDxfHandleCode(code);
        const std::string value = handle ? "A" : code == 310 ? "81" : "\x81";
        std::istringstream input(std::to_string(code) + "\n" + value + "\n");
        std::unique_ptr<dxfReader> reader = std::make_unique<dxfReaderAscii>(&input);
        reader->setVersion("AC1015", true);
        reader->setCodePage("ANSI_1252");
        reader->setSemanticStringMode(true);
        int actualCode = 0;
        const bool read = reader->readRec(&actualCode);
        if (read) {
            const std::string raw = reader->getRawValue();
            // Exercise semantic-code dispatch and the explicit UTF8 getter;
            // observations count calls, not deduplicated physical records.
            (void)reader->getString();
            (void)reader->getUtf8String();
            const auto observed = reader->textReadObservations();
            t.expect(reader->getRawValue() == raw
                         && (observed.semanticDecodeCalls != 0u) == semantic
                         && (observed.issueCount(Issue::UndefinedByteSequence) != 0u) == semantic,
                     "semantic getters exclude controls/binary/handles and never rewrite raw carrier bytes");
            if (semantic)
                t.expect(observed.hasFirstRecord && observed.firstRecord == 1
                             && observed.hasFirstValueEndOffset
                             && observed.firstValueEndOffset == std::to_string(code).size() + value.size() + 2u,
                         "physical context records the exact value end, not an invented offending byte");
        }
        t.expect(read && actualCode == code, "reader-role control reads its intended physical record");
        ++cases;
    }
    DRW_DxfTextReadObservations saturated;
    const auto maximum = (std::numeric_limits<std::uint64_t>::max)();
    saturated.semanticDecodeCalls = saturated.nonAsciiDecodeCalls = maximum;
    saturated.legacyByteDecodeCalls = saturated.recoveryDecodeCalls = maximum;
    saturated.issueDecodeCalls = maximum;
    saturated.issueCalls[static_cast<std::size_t>(Issue::UnknownCodePage)] = maximum;
    DRW_TextDecodeObservation value;
    value.nonAsciiBytes = value.legacyBytes = value.recovery = true;
    value.issues = textIssueMask({Issue::UnknownCodePage});
    dxfReader::appendTextReadObservation(saturated, value, 1, 17, true);
    dxfReader::appendTextReadObservation(saturated, {}, 2, 99, false);
    t.expect(saturated.counterSaturated && saturated.semanticDecodeCalls == maximum
                 && saturated.issueCount(Issue::UnknownCodePage) == maximum
                 && saturated.firstGroupCode == 1 && saturated.firstRecord == 17
                 && saturated.hasFirstRecord && saturated.firstIssueMask == value.issues
                 && saturated.issueCount(Issue::Count) == 0,
             "fixed counters saturate without wrapping or replacing first issue context");
    std::cout << "Text observation reader roles: cases=" << cases << ", saturation controls=1\n";
}

std::string makeTextObservationDrawing(const char* page, bool modern,
                                        bool pageFirst, bool binary,
                                        const std::string& payload) {
    auto bytes = makeCodePageDiagnosticDrawing(page, modern, pageFirst, binary);
    const std::string original = modern ? "\xC3\xA9\xE4\xB8\xAD" : "ascii-control";
    const auto at = bytes.find(original);
    if (at == std::string::npos) throw std::runtime_error("authored TEXT payload not found");
    bytes.replace(at, original.size(), payload);
    return bytes;
}

void testExactIso8859_6(TestContext& t) {
    // Independent transcription of Unicode's ISO8859/8859-6.TXT mapping.
    // Zero marks one of the 45 undefined upper-half byte positions.
    static constexpr int upper[] = {
        0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
        0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,
        0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
        0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F,
        0x00A0, 0x0000, 0x0000, 0x0000, 0x00A4, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x060C, 0x00AD, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x061B, 0x0000, 0x0000, 0x0000, 0x061F,
        0x0000, 0x0621, 0x0622, 0x0623, 0x0624, 0x0625, 0x0626, 0x0627,
        0x0628, 0x0629, 0x062A, 0x062B, 0x062C, 0x062D, 0x062E, 0x062F,
        0x0630, 0x0631, 0x0632, 0x0633, 0x0634, 0x0635, 0x0636, 0x0637,
        0x0638, 0x0639, 0x063A, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0640, 0x0641, 0x0642, 0x0643, 0x0644, 0x0645, 0x0646, 0x0647,
        0x0648, 0x0649, 0x064A, 0x064B, 0x064C, 0x064D, 0x064E, 0x064F,
        0x0650, 0x0651, 0x0652, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000
    };
    static_assert(sizeof(upper) / sizeof(upper[0]) == 128,
                  "ISO-8859-6 mapping spans every upper byte");

    using Policy = DRW_CodePageMappingPolicy;
    using Status = DRW_CodePageMappingStatus;
    using Issue = DRW_DxfTextIssue;
    const char* aliases[] = {
        "ISO8859-6", "ISO-8859-6", "iso8859-6", "iso-8859-6"
    };
    constexpr std::uint32_t undefinedIssues =
        DRW::dxfTextIssueMask(Issue::UndefinedByteSequence)
        | DRW::dxfTextIssueMask(Issue::DiscardedSource);
    std::size_t definedPairs = 0;
    std::size_t undefinedPairs = 0;
    for (const char* name : aliases) {
        const auto resolution = DRW_TextCodec::resolveCodePage(
            name, Policy::ExactNamed);
        auto converter = DRW_TextCodec::createExactCodePageConverter(name);
        t.expect(resolution.status == Status::ExactImplementationAvailable
                     && resolution.requestedName != nullptr
                     && std::string(resolution.requestedName) == "ISO8859-6"
                     && resolution.decoderName != nullptr
                     && std::string(resolution.decoderName) == "ISO8859-6"
                     && converter != nullptr,
                 "exact ISO-8859-6 spellings resolve to the distinct Arabic decoder");
        if (!converter) continue;

        std::string allBytes;
        std::string expected;
        std::string definedBytes;
        for (unsigned int byte = 0x80u; byte <= 0xFFu; ++byte) {
            const std::string wire(
                1u, static_cast<char>(static_cast<unsigned char>(byte)));
            allBytes += wire;
            const int scalar = upper[byte - 0x80u];
            if (scalar == 0) {
                t.expect(converter->toUtf8(wire).empty()
                             && converter->decodeObservation().issues == undefinedIssues
                             && converter->decodeObservation().nonAsciiBytes
                             && converter->decodeObservation().legacyBytes,
                         "undefined ISO-8859-6 bytes are discarded and reported");
                ++undefinedPairs;
                continue;
            }
            const auto utf8 = unicodeScalarExpectedUtf8(scalar);
            t.expect(converter->toUtf8(wire) == utf8
                         && converter->decodeObservation().issues == 0
                         && converter->decodeObservation().nonAsciiBytes
                         && converter->decodeObservation().legacyBytes
                         && converter->fromUtf8(utf8) == wire,
                     "every defined ISO-8859-6 byte decodes and inverse-encodes exactly");
            expected += utf8;
            definedBytes += wire;
            ++definedPairs;
        }
        t.expect(converter->toUtf8(allBytes) == expected
                     && converter->decodeObservation().issues == undefinedIssues
                     && converter->decodeObservation().nonAsciiBytes
                     && converter->fromUtf8(expected) == definedBytes,
                 "mixed ISO-8859-6 batch preserves order while discarding only undefined bytes");
    }
    t.expect(definedPairs == 4u * 83u && undefinedPairs == 4u * 45u,
             "all exact ISO-8859-6 spellings cover all 128 upper-half byte positions");

    DRW_TextCodec compactCompatible;
    compactCompatible.setVersion(DRW::AC1015, true);
    compactCompatible.setCodePage("ISO8859-6", true);
    DRW_TextCodec hyphenatedCompatible;
    hyphenatedCompatible.setVersion(DRW::AC1015, true);
    hyphenatedCompatible.setCodePage("ISO-8859-6", true);
    t.expect(compactCompatible.getCodePage() == "ANSI_1256"
                 && hyphenatedCompatible.getCodePage() == "ANSI_1252"
                 && compactCompatible.toUtf8("\xA1") == "\xD8\x8C"
                 && hyphenatedCompatible.toUtf8("\xA1") == "\xC2\xA1",
             "Compatible ISO-8859-6 spellings retain their distinct Windows-1256/1252 fallbacks");
}

void testExactIso8859_7(TestContext& t) {
    // Independent transcription of Unicode's ISO8859/8859-7.TXT mapping.
    // The 2003 profile has three undefined upper-half positions.
    static constexpr int upper[] = {
        0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
        0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,
        0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
        0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F,
        0x00A0, 0x2018, 0x2019, 0x00A3, 0x20AC, 0x20AF, 0x00A6, 0x00A7,
        0x00A8, 0x00A9, 0x037A, 0x00AB, 0x00AC, 0x00AD, 0x0000, 0x2015,
        0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x0384, 0x0385, 0x0386, 0x00B7,
        0x0388, 0x0389, 0x038A, 0x00BB, 0x038C, 0x00BD, 0x038E, 0x038F,
        0x0390, 0x0391, 0x0392, 0x0393, 0x0394, 0x0395, 0x0396, 0x0397,
        0x0398, 0x0399, 0x039A, 0x039B, 0x039C, 0x039D, 0x039E, 0x039F,
        0x03A0, 0x03A1, 0x0000, 0x03A3, 0x03A4, 0x03A5, 0x03A6, 0x03A7,
        0x03A8, 0x03A9, 0x03AA, 0x03AB, 0x03AC, 0x03AD, 0x03AE, 0x03AF,
        0x03B0, 0x03B1, 0x03B2, 0x03B3, 0x03B4, 0x03B5, 0x03B6, 0x03B7,
        0x03B8, 0x03B9, 0x03BA, 0x03BB, 0x03BC, 0x03BD, 0x03BE, 0x03BF,
        0x03C0, 0x03C1, 0x03C2, 0x03C3, 0x03C4, 0x03C5, 0x03C6, 0x03C7,
        0x03C8, 0x03C9, 0x03CA, 0x03CB, 0x03CC, 0x03CD, 0x03CE, 0x0000
    };
    static_assert(sizeof(upper) / sizeof(upper[0]) == 128,
                  "ISO-8859-7 mapping spans every upper byte");

    using Policy = DRW_CodePageMappingPolicy;
    using Status = DRW_CodePageMappingStatus;
    using Issue = DRW::DxfTextIssue;
    constexpr std::uint32_t undefinedIssues =
        DRW::dxfTextIssueMask(Issue::UndefinedByteSequence)
        | DRW::dxfTextIssueMask(Issue::DiscardedSource);
    const char* aliases[] = {
        "ISO8859-7", "ISO-8859-7", "iso8859-7", "iso-8859-7"
    };
    std::size_t definedPairs = 0;
    std::size_t undefinedPairs = 0;
    for (const char* name : aliases) {
        const auto resolution = DRW_TextCodec::resolveCodePage(
            name, Policy::ExactNamed);
        auto converter = DRW_TextCodec::createExactCodePageConverter(name);
        t.expect(resolution.status == Status::ExactImplementationAvailable
                     && resolution.requestedName != nullptr
                     && std::string(resolution.requestedName) == "ISO8859-7"
                     && resolution.decoderName != nullptr
                     && std::string(resolution.decoderName) == "ISO8859-7"
                     && converter != nullptr,
                 "exact ISO-8859-7 spellings resolve to the distinct Greek decoder");
        if (!converter) continue;

        std::string allBytes;
        std::string expected;
        std::string definedBytes;
        for (unsigned int byte = 0x80u; byte <= 0xFFu; ++byte) {
            const std::string wire(
                1u, static_cast<char>(static_cast<unsigned char>(byte)));
            allBytes += wire;
            const int scalar = upper[byte - 0x80u];
            if (scalar == 0) {
                t.expect(converter->toUtf8(wire).empty()
                             && converter->decodeObservation().issues == undefinedIssues
                             && converter->decodeObservation().nonAsciiBytes
                             && converter->decodeObservation().legacyBytes,
                         "undefined ISO-8859-7 bytes are discarded and reported");
                ++undefinedPairs;
                continue;
            }
            const auto utf8 = unicodeScalarExpectedUtf8(scalar);
            t.expect(converter->toUtf8(wire) == utf8
                         && converter->decodeObservation().issues == 0
                         && converter->decodeObservation().nonAsciiBytes
                         && converter->decodeObservation().legacyBytes
                         && converter->fromUtf8(utf8) == wire,
                     "every defined ISO-8859-7 byte decodes and inverse-encodes exactly");
            expected += utf8;
            definedBytes += wire;
            ++definedPairs;
        }
        t.expect(converter->toUtf8(allBytes) == expected
                     && converter->decodeObservation().issues == undefinedIssues
                     && converter->decodeObservation().nonAsciiBytes
                     && converter->fromUtf8(expected) == definedBytes,
                 "mixed ISO-8859-7 batch preserves order while discarding only undefined bytes");
    }
    t.expect(definedPairs == 4u * 125u && undefinedPairs == 4u * 3u,
             "all exact ISO-8859-7 spellings cover all 128 upper-half byte positions");

    DRW_TextCodec compactCompatible;
    compactCompatible.setVersion(DRW::AC1015, true);
    compactCompatible.setCodePage("ISO8859-7", true);
    DRW_TextCodec hyphenatedCompatible;
    hyphenatedCompatible.setVersion(DRW::AC1015, true);
    hyphenatedCompatible.setCodePage("ISO-8859-7", true);
    t.expect(compactCompatible.getCodePage() == "ANSI_1253"
                 && hyphenatedCompatible.getCodePage() == "ANSI_1252"
                 && compactCompatible.toUtf8("\xA1") == "\xCE\x85"
                 && hyphenatedCompatible.toUtf8("\xA1") == "\xC2\xA1",
             "Compatible ISO-8859-7 spellings retain their Windows-1253/1252 fallbacks");
}

void testExactIso8859_8(TestContext& t) {
    // Independent transcription of Unicode's ISO8859/8859-8.TXT mapping.
    // Zero marks one of the 36 undefined upper-half byte positions.
    static constexpr int upper[] = {
        0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
        0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,
        0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
        0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F,
        0x00A0, 0x0000, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
        0x00A8, 0x00A9, 0x00D7, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
        0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
        0x00B8, 0x00B9, 0x00F7, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
        0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2017,
        0x05D0, 0x05D1, 0x05D2, 0x05D3, 0x05D4, 0x05D5, 0x05D6, 0x05D7,
        0x05D8, 0x05D9, 0x05DA, 0x05DB, 0x05DC, 0x05DD, 0x05DE, 0x05DF,
        0x05E0, 0x05E1, 0x05E2, 0x05E3, 0x05E4, 0x05E5, 0x05E6, 0x05E7,
        0x05E8, 0x05E9, 0x05EA, 0x0000, 0x0000, 0x200E, 0x200F, 0x0000
    };
    static_assert(sizeof(upper) / sizeof(upper[0]) == 128,
                  "ISO-8859-8 mapping spans every upper byte");

    using Policy = DRW_CodePageMappingPolicy;
    using Status = DRW_CodePageMappingStatus;
    using Issue = DRW::DxfTextIssue;
    constexpr std::uint32_t undefinedIssues =
        DRW::dxfTextIssueMask(Issue::UndefinedByteSequence)
        | DRW::dxfTextIssueMask(Issue::DiscardedSource);
    const char* aliases[] = {
        "ISO8859-8", "ISO-8859-8", "iso8859-8", "iso-8859-8"
    };
    std::size_t definedPairs = 0;
    std::size_t undefinedPairs = 0;
    for (const char* name : aliases) {
        const auto resolution = DRW_TextCodec::resolveCodePage(
            name, Policy::ExactNamed);
        auto converter = DRW_TextCodec::createExactCodePageConverter(name);
        t.expect(resolution.status == Status::ExactImplementationAvailable
                     && resolution.requestedName != nullptr
                     && std::string(resolution.requestedName) == "ISO8859-8"
                     && resolution.decoderName != nullptr
                     && std::string(resolution.decoderName) == "ISO8859-8"
                     && converter != nullptr,
                 "exact ISO-8859-8 spellings resolve to the distinct Hebrew decoder");
        if (!converter) continue;

        std::string allBytes;
        std::string expected;
        std::string definedBytes;
        for (unsigned int byte = 0x80u; byte <= 0xFFu; ++byte) {
            const std::string wire(
                1u, static_cast<char>(static_cast<unsigned char>(byte)));
            allBytes += wire;
            const int scalar = upper[byte - 0x80u];
            if (scalar == 0) {
                t.expect(converter->toUtf8(wire).empty()
                             && converter->decodeObservation().issues == undefinedIssues
                             && converter->decodeObservation().nonAsciiBytes
                             && converter->decodeObservation().legacyBytes,
                         "undefined ISO-8859-8 bytes are discarded and reported");
                ++undefinedPairs;
                continue;
            }
            const auto utf8 = unicodeScalarExpectedUtf8(scalar);
            t.expect(converter->toUtf8(wire) == utf8
                         && converter->decodeObservation().issues == 0
                         && converter->decodeObservation().nonAsciiBytes
                         && converter->decodeObservation().legacyBytes
                         && converter->fromUtf8(utf8) == wire,
                     "every defined ISO-8859-8 byte decodes and inverse-encodes exactly");
            expected += utf8;
            definedBytes += wire;
            ++definedPairs;
        }
        t.expect(converter->toUtf8(allBytes) == expected
                     && converter->decodeObservation().issues == undefinedIssues
                     && converter->decodeObservation().nonAsciiBytes
                     && converter->fromUtf8(expected) == definedBytes,
                 "mixed ISO-8859-8 batch preserves order while discarding only undefined bytes");
    }
    t.expect(definedPairs == 4u * 92u && undefinedPairs == 4u * 36u,
             "all exact ISO-8859-8 spellings cover all 128 upper-half byte positions");

    DRW_TextCodec compactCompatible;
    compactCompatible.setVersion(DRW::AC1015, true);
    compactCompatible.setCodePage("ISO8859-8", true);
    DRW_TextCodec hyphenatedCompatible;
    hyphenatedCompatible.setVersion(DRW::AC1015, true);
    hyphenatedCompatible.setCodePage("ISO-8859-8", true);
    t.expect(compactCompatible.getCodePage() == "ANSI_1255"
                 && hyphenatedCompatible.getCodePage() == "ANSI_1252"
                 && compactCompatible.toUtf8("\xE0") == "\xD7\x90"
                 && hyphenatedCompatible.toUtf8("\xE0") == "\xC3\xA0",
             "Compatible ISO-8859-8 spellings retain their Windows-1255/1252 fallbacks");
}

void testExactIso8859_9(TestContext& t) {
    // Independent transcription of Unicode's ISO8859/8859-9.TXT mapping.
    static constexpr int upper[] = {
        0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
        0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,
        0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
        0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F,
        0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x00A4, 0x00A5, 0x00A6, 0x00A7,
        0x00A8, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
        0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x00B4, 0x00B5, 0x00B6, 0x00B7,
        0x00B8, 0x00B9, 0x00BA, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00BF,
        0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
        0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
        0x011E, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
        0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x0130, 0x015E, 0x00DF,
        0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
        0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
        0x011F, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
        0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x0131, 0x015F, 0x00FF
    };
    static_assert(sizeof(upper) / sizeof(upper[0]) == 128,
                  "ISO-8859-9 mapping spans every upper byte");

    using Policy = DRW_CodePageMappingPolicy;
    using Status = DRW_CodePageMappingStatus;
    const char* aliases[] = {
        "ISO8859-9", "ISO-8859-9", "iso8859-9", "iso-8859-9"
    };
    std::size_t definedPairs = 0;
    for (const char* name : aliases) {
        const auto resolution = DRW_TextCodec::resolveCodePage(
            name, Policy::ExactNamed);
        auto converter = DRW_TextCodec::createExactCodePageConverter(name);
        t.expect(resolution.status == Status::ExactImplementationAvailable
                     && resolution.requestedName != nullptr
                     && std::string(resolution.requestedName) == "ISO8859-9"
                     && resolution.decoderName != nullptr
                     && std::string(resolution.decoderName) == "ISO8859-9"
                     && converter != nullptr,
                 "exact ISO-8859-9 spellings resolve to the distinct Turkish decoder");
        if (!converter) continue;

        std::string allBytes;
        std::string expected;
        std::string definedBytes;
        for (unsigned int byte = 0x80u; byte <= 0xFFu; ++byte) {
            const std::string wire(
                1u, static_cast<char>(static_cast<unsigned char>(byte)));
            allBytes += wire;
            const int scalar = upper[byte - 0x80u];
            const auto utf8 = unicodeScalarExpectedUtf8(scalar);
            t.expect(converter->toUtf8(wire) == utf8
                         && converter->decodeObservation().issues == 0
                         && converter->decodeObservation().nonAsciiBytes
                         && converter->decodeObservation().legacyBytes
                         && converter->fromUtf8(utf8) == wire,
                     "every defined ISO-8859-9 byte decodes and inverse-encodes exactly");
            expected += utf8;
            definedBytes += wire;
            ++definedPairs;
        }
        t.expect(converter->toUtf8(allBytes) == expected
                     && converter->decodeObservation().issues == 0
                     && converter->decodeObservation().nonAsciiBytes
                     && converter->fromUtf8(expected) == definedBytes,
                 "mixed ISO-8859-9 batch preserves every upper-half byte in order");
    }
    t.expect(definedPairs == 4u * 128u,
             "all exact ISO-8859-9 spellings cover all 128 upper-half byte positions");

    DRW_TextCodec compactCompatible;
    compactCompatible.setVersion(DRW::AC1015, true);
    compactCompatible.setCodePage("ISO8859-9", true);
    DRW_TextCodec hyphenatedCompatible;
    hyphenatedCompatible.setVersion(DRW::AC1015, true);
    hyphenatedCompatible.setCodePage("ISO-8859-9", true);
    t.expect(compactCompatible.getCodePage() == "ANSI_1254"
                 && hyphenatedCompatible.getCodePage() == "ANSI_1252"
                 && compactCompatible.toUtf8("\x80") == "\xE2\x82\xAC"
                 && hyphenatedCompatible.toUtf8("\xD0") == "\xC3\x90",
             "Compatible ISO-8859-9 spellings retain their Windows-1254/1252 fallbacks");
}

void testExactIso8859_10(TestContext& t) {
    // Independent transcription of Unicode's ISO8859/8859-10.TXT mapping.
    static constexpr int upper[] = {
        0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
        0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,
        0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
        0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F,
        0x00A0, 0x0104, 0x0112, 0x0122, 0x012A, 0x0128, 0x0136, 0x00A7,
        0x013B, 0x0110, 0x0160, 0x0166, 0x017D, 0x00AD, 0x016A, 0x014A,
        0x00B0, 0x0105, 0x0113, 0x0123, 0x012B, 0x0129, 0x0137, 0x00B7,
        0x013C, 0x0111, 0x0161, 0x0167, 0x017E, 0x2015, 0x016B, 0x014B,
        0x0100, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x012E,
        0x010C, 0x00C9, 0x0118, 0x00CB, 0x0116, 0x00CD, 0x00CE, 0x00CF,
        0x00D0, 0x0145, 0x014C, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x0168,
        0x00D8, 0x0172, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
        0x0101, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x012F,
        0x010D, 0x00E9, 0x0119, 0x00EB, 0x0117, 0x00ED, 0x00EE, 0x00EF,
        0x00F0, 0x0146, 0x014D, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x0169,
        0x00F8, 0x0173, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x0138
    };
    static_assert(sizeof(upper) / sizeof(upper[0]) == 128,
                  "ISO-8859-10 mapping spans every upper byte");

    using Policy = DRW_CodePageMappingPolicy;
    using Status = DRW_CodePageMappingStatus;
    const char* aliases[] = {
        "ISO8859-10", "ISO-8859-10", "iso8859-10", "iso-8859-10"
    };
    std::size_t definedPairs = 0;
    for (const char* name : aliases) {
        const auto resolution = DRW_TextCodec::resolveCodePage(
            name, Policy::ExactNamed);
        auto converter = DRW_TextCodec::createExactCodePageConverter(name);
        t.expect(resolution.status == Status::ExactImplementationAvailable
                     && resolution.requestedName != nullptr
                     && std::string(resolution.requestedName) == "ISO8859-10"
                     && resolution.decoderName != nullptr
                     && std::string(resolution.decoderName) == "ISO8859-10"
                     && converter != nullptr,
                 "exact ISO-8859-10 spellings resolve to the distinct Nordic decoder");
        if (!converter) continue;

        std::string allBytes;
        std::string expected;
        std::string definedBytes;
        for (unsigned int byte = 0x80u; byte <= 0xFFu; ++byte) {
            const std::string wire(
                1u, static_cast<char>(static_cast<unsigned char>(byte)));
            allBytes += wire;
            const auto utf8 = unicodeScalarExpectedUtf8(upper[byte - 0x80u]);
            t.expect(converter->toUtf8(wire) == utf8
                         && converter->decodeObservation().issues == 0
                         && converter->decodeObservation().nonAsciiBytes
                         && converter->decodeObservation().legacyBytes
                         && converter->fromUtf8(utf8) == wire,
                     "every defined ISO-8859-10 byte decodes and inverse-encodes exactly");
            expected += utf8;
            definedBytes += wire;
            ++definedPairs;
        }
        t.expect(converter->toUtf8(allBytes) == expected
                     && converter->decodeObservation().issues == 0
                     && converter->decodeObservation().nonAsciiBytes
                     && converter->fromUtf8(expected) == definedBytes,
                 "mixed ISO-8859-10 batch preserves every upper-half byte in order");
    }
    t.expect(definedPairs == 4u * 128u,
             "all exact ISO-8859-10 spellings cover all 128 upper-half byte positions");

    DRW_TextCodec compactCompatible;
    compactCompatible.setVersion(DRW::AC1015, true);
    compactCompatible.setCodePage("ISO8859-10", true);
    DRW_TextCodec hyphenatedCompatible;
    hyphenatedCompatible.setVersion(DRW::AC1015, true);
    hyphenatedCompatible.setCodePage("ISO-8859-10", true);
    t.expect(compactCompatible.getCodePage() == "ANSI_1257"
                 && hyphenatedCompatible.getCodePage() == "ANSI_1252"
                 && compactCompatible.toUtf8("\x80") == "\xE2\x82\xAC"
                 && hyphenatedCompatible.toUtf8("\xA1") == "\xC2\xA1",
             "Compatible ISO-8859-10 spellings retain their Windows-1257/1252 fallbacks");
}

void testExactIso8859_11(TestContext& t) {
    // Independent transcription of Unicode's ISO8859/8859-11.TXT mapping.
    // Zero marks one of the eight undefined upper-half byte positions.
    static constexpr int upper[] = {
        0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
        0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,
        0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
        0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F,
        0x00A0, 0x0E01, 0x0E02, 0x0E03, 0x0E04, 0x0E05, 0x0E06, 0x0E07,
        0x0E08, 0x0E09, 0x0E0A, 0x0E0B, 0x0E0C, 0x0E0D, 0x0E0E, 0x0E0F,
        0x0E10, 0x0E11, 0x0E12, 0x0E13, 0x0E14, 0x0E15, 0x0E16, 0x0E17,
        0x0E18, 0x0E19, 0x0E1A, 0x0E1B, 0x0E1C, 0x0E1D, 0x0E1E, 0x0E1F,
        0x0E20, 0x0E21, 0x0E22, 0x0E23, 0x0E24, 0x0E25, 0x0E26, 0x0E27,
        0x0E28, 0x0E29, 0x0E2A, 0x0E2B, 0x0E2C, 0x0E2D, 0x0E2E, 0x0E2F,
        0x0E30, 0x0E31, 0x0E32, 0x0E33, 0x0E34, 0x0E35, 0x0E36, 0x0E37,
        0x0E38, 0x0E39, 0x0E3A, 0x0000, 0x0000, 0x0000, 0x0000, 0x0E3F,
        0x0E40, 0x0E41, 0x0E42, 0x0E43, 0x0E44, 0x0E45, 0x0E46, 0x0E47,
        0x0E48, 0x0E49, 0x0E4A, 0x0E4B, 0x0E4C, 0x0E4D, 0x0E4E, 0x0E4F,
        0x0E50, 0x0E51, 0x0E52, 0x0E53, 0x0E54, 0x0E55, 0x0E56, 0x0E57,
        0x0E58, 0x0E59, 0x0E5A, 0x0E5B, 0x0000, 0x0000, 0x0000, 0x0000
    };
    static_assert(sizeof(upper) / sizeof(upper[0]) == 128,
                  "ISO-8859-11 mapping spans every upper byte");

    using Policy = DRW_CodePageMappingPolicy;
    using Status = DRW_CodePageMappingStatus;
    using Issue = DRW::DxfTextIssue;
    constexpr std::uint32_t undefinedIssues =
        DRW::dxfTextIssueMask(Issue::UndefinedByteSequence)
        | DRW::dxfTextIssueMask(Issue::DiscardedSource);
    const char* aliases[] = {"ISO8859-11", "iso8859-11"};
    std::size_t definedPairs = 0;
    std::size_t undefinedPairs = 0;
    for (const char* name : aliases) {
        const auto resolution = DRW_TextCodec::resolveCodePage(
            name, Policy::ExactNamed);
        auto converter = DRW_TextCodec::createExactCodePageConverter(name);
        t.expect(resolution.status == Status::ExactImplementationAvailable
                     && resolution.requestedName != nullptr
                     && std::string(resolution.requestedName) == "ISO8859-11"
                     && resolution.decoderName != nullptr
                     && std::string(resolution.decoderName) == "ISO8859-11"
                     && converter != nullptr,
                 "exact ISO-8859-11 spellings resolve to the distinct Thai decoder");
        if (!converter) continue;

        std::string allBytes;
        std::string expected;
        std::string definedBytes;
        for (unsigned int byte = 0x80u; byte <= 0xFFu; ++byte) {
            const std::string wire(
                1u, static_cast<char>(static_cast<unsigned char>(byte)));
            allBytes += wire;
            const int scalar = upper[byte - 0x80u];
            if (scalar == 0) {
                t.expect(converter->toUtf8(wire).empty()
                             && converter->decodeObservation().issues == undefinedIssues
                             && converter->decodeObservation().nonAsciiBytes
                             && converter->decodeObservation().legacyBytes,
                         "undefined ISO-8859-11 bytes are discarded and reported");
                ++undefinedPairs;
                continue;
            }
            const auto utf8 = unicodeScalarExpectedUtf8(scalar);
            t.expect(converter->toUtf8(wire) == utf8
                         && converter->decodeObservation().issues == 0
                         && converter->decodeObservation().nonAsciiBytes
                         && converter->decodeObservation().legacyBytes
                         && converter->fromUtf8(utf8) == wire,
                     "every defined ISO-8859-11 byte decodes and inverse-encodes exactly");
            expected += utf8;
            definedBytes += wire;
            ++definedPairs;
        }
        t.expect(converter->toUtf8(allBytes) == expected
                     && converter->decodeObservation().issues == undefinedIssues
                     && converter->decodeObservation().nonAsciiBytes
                     && converter->fromUtf8(expected) == definedBytes,
                 "mixed ISO-8859-11 batch preserves Thai text and discards only undefined bytes");
    }
    t.expect(definedPairs == 2u * 120u && undefinedPairs == 2u * 8u,
             "both exact ISO-8859-11 spellings cover all 128 upper-half byte positions");

    DRW_TextCodec compatible;
    compatible.setVersion(DRW::AC1015, true);
    compatible.setCodePage("ISO8859-11", true);
    t.expect(compatible.getCodePage() == "ANSI_874"
                 && compatible.toUtf8("\x80") == "\xE2\x82\xAC",
             "Compatible ISO8859-11 retains its historical Windows-874 fallback");
}

void testExactIso8859_13(TestContext& t) {
    // Independent transcription of Unicode's ISO8859/8859-13.TXT mapping.
    static constexpr int upper[] = {
        0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
        0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,
        0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
        0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F,
        0x00A0, 0x201D, 0x00A2, 0x00A3, 0x00A4, 0x201E, 0x00A6, 0x00A7,
        0x00D8, 0x00A9, 0x0156, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00C6,
        0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x201C, 0x00B5, 0x00B6, 0x00B7,
        0x00F8, 0x00B9, 0x0157, 0x00BB, 0x00BC, 0x00BD, 0x00BE, 0x00E6,
        0x0104, 0x012E, 0x0100, 0x0106, 0x00C4, 0x00C5, 0x0118, 0x0112,
        0x010C, 0x00C9, 0x0179, 0x0116, 0x0122, 0x0136, 0x012A, 0x013B,
        0x0160, 0x0143, 0x0145, 0x00D3, 0x014C, 0x00D5, 0x00D6, 0x00D7,
        0x0172, 0x0141, 0x015A, 0x016A, 0x00DC, 0x017B, 0x017D, 0x00DF,
        0x0105, 0x012F, 0x0101, 0x0107, 0x00E4, 0x00E5, 0x0119, 0x0113,
        0x010D, 0x00E9, 0x017A, 0x0117, 0x0123, 0x0137, 0x012B, 0x013C,
        0x0161, 0x0144, 0x0146, 0x00F3, 0x014D, 0x00F5, 0x00F6, 0x00F7,
        0x0173, 0x0142, 0x015B, 0x016B, 0x00FC, 0x017C, 0x017E, 0x2019
    };
    static_assert(sizeof(upper) / sizeof(upper[0]) == 128,
                  "ISO-8859-13 mapping spans every upper byte");

    using Policy = DRW_CodePageMappingPolicy;
    using Status = DRW_CodePageMappingStatus;
    const char* aliases[] = {
        "ISO8859-13", "iso8859-13", "ISO-8859-13", "iso-8859-13"
    };
    std::size_t definedPairs = 0;
    for (const char* name : aliases) {
        const auto resolution = DRW_TextCodec::resolveCodePage(
            name, Policy::ExactNamed);
        auto converter = DRW_TextCodec::createExactCodePageConverter(name);
        t.expect(resolution.status == Status::ExactImplementationAvailable
                     && resolution.requestedName != nullptr
                     && std::string(resolution.requestedName) == "ISO8859-13"
                     && resolution.decoderName != nullptr
                     && std::string(resolution.decoderName) == "ISO8859-13"
                     && converter != nullptr,
                 "exact ISO-8859-13 spellings resolve to the distinct Baltic decoder");
        if (!converter) continue;

        std::string allBytes;
        std::string expected;
        for (unsigned int byte = 0x80u; byte <= 0xFFu; ++byte) {
            const std::string wire(
                1u, static_cast<char>(static_cast<unsigned char>(byte)));
            allBytes += wire;
            const auto utf8 = unicodeScalarExpectedUtf8(upper[byte - 0x80u]);
            t.expect(converter->toUtf8(wire) == utf8
                         && converter->decodeObservation().issues == 0
                         && converter->decodeObservation().nonAsciiBytes
                         && converter->decodeObservation().legacyBytes
                         && converter->fromUtf8(utf8) == wire,
                     "every defined ISO-8859-13 byte decodes and inverse-encodes exactly");
            expected += utf8;
            ++definedPairs;
        }
        t.expect(converter->toUtf8(allBytes) == expected
                     && converter->decodeObservation().issues == 0
                     && converter->decodeObservation().nonAsciiBytes
                     && converter->fromUtf8(expected) == allBytes,
                 "mixed ISO-8859-13 batch preserves all C1, punctuation and Baltic letters");
    }
    t.expect(definedPairs == 4u * 128u,
             "all exact ISO-8859-13 spellings cover all 128 upper-half byte positions");

    DRW_TextCodec compactCompatible;
    compactCompatible.setVersion(DRW::AC1015, true);
    compactCompatible.setCodePage("ISO8859-13", true);
    DRW_TextCodec hyphenatedCompatible;
    hyphenatedCompatible.setVersion(DRW::AC1015, true);
    hyphenatedCompatible.setCodePage("ISO-8859-13", true);
    t.expect(compactCompatible.getCodePage() == "ANSI_1257"
                 && hyphenatedCompatible.getCodePage() == "ANSI_1252"
                 && compactCompatible.toUtf8("\xA1").empty()
                 && hyphenatedCompatible.toUtf8("\xA1") == "\xC2\xA1",
             "Compatible ISO-8859-13 spellings retain their Windows-1257/1252 fallbacks");
}

void testExactIso8859_14(TestContext& t) {
    // Independent transcription of Unicode's ISO8859/8859-14.TXT mapping.
    static constexpr int upper[] = {
        0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
        0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,
        0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
        0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F,
        0x00A0, 0x1E02, 0x1E03, 0x00A3, 0x010A, 0x010B, 0x1E0A, 0x00A7,
        0x1E80, 0x00A9, 0x1E82, 0x1E0B, 0x1EF2, 0x00AD, 0x00AE, 0x0178,
        0x1E1E, 0x1E1F, 0x0120, 0x0121, 0x1E40, 0x1E41, 0x00B6, 0x1E56,
        0x1E81, 0x1E57, 0x1E83, 0x1E60, 0x1EF3, 0x1E84, 0x1E85, 0x1E61,
        0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
        0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
        0x0174, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x1E6A,
        0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x0176, 0x00DF,
        0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
        0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
        0x0175, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x1E6B,
        0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x0177, 0x00FF
    };
    static_assert(sizeof(upper) / sizeof(upper[0]) == 128,
                  "ISO-8859-14 mapping spans every upper byte");

    using Policy = DRW_CodePageMappingPolicy;
    using Status = DRW_CodePageMappingStatus;
    const char* aliases[] = {"ISO8859-14", "iso8859-14"};
    std::size_t definedPairs = 0;
    for (const char* name : aliases) {
        const auto resolution = DRW_TextCodec::resolveCodePage(
            name, Policy::ExactNamed);
        auto converter = DRW_TextCodec::createExactCodePageConverter(name);
        t.expect(resolution.status == Status::ExactImplementationAvailable
                     && resolution.requestedName != nullptr
                     && std::string(resolution.requestedName) == "ISO8859-14"
                     && resolution.decoderName != nullptr
                     && std::string(resolution.decoderName) == "ISO8859-14"
                     && converter != nullptr,
                 "exact ISO-8859-14 spellings resolve to the distinct Celtic decoder");
        if (!converter) continue;

        std::string allBytes;
        std::string expected;
        for (unsigned int byte = 0x80u; byte <= 0xFFu; ++byte) {
            const std::string wire(
                1u, static_cast<char>(static_cast<unsigned char>(byte)));
            allBytes += wire;
            const auto utf8 = unicodeScalarExpectedUtf8(upper[byte - 0x80u]);
            t.expect(converter->toUtf8(wire) == utf8
                         && converter->decodeObservation().issues == 0
                         && converter->decodeObservation().nonAsciiBytes
                         && converter->decodeObservation().legacyBytes
                         && converter->fromUtf8(utf8) == wire,
                     "every defined ISO-8859-14 byte decodes and inverse-encodes exactly");
            expected += utf8;
            ++definedPairs;
        }
        t.expect(converter->toUtf8(allBytes) == expected
                     && converter->decodeObservation().issues == 0
                     && converter->decodeObservation().nonAsciiBytes
                     && converter->fromUtf8(expected) == allBytes,
                 "mixed ISO-8859-14 batch preserves C1 controls and Celtic letters");
    }
    t.expect(definedPairs == 2u * 128u,
             "both exact ISO-8859-14 spellings cover all 128 upper-half byte positions");

    DRW_TextCodec compatible;
    compatible.setVersion(DRW::AC1015, true);
    compatible.setCodePage("ISO8859-14", true);
    t.expect(compatible.getCodePage() == "ANSI_1252"
                 && compatible.toUtf8("\xA1") == "\xC2\xA1",
             "Compatible ISO8859-14 retains its historical Windows-1252 fallback");
}

void testExactIso8859_15(TestContext& t) {
    // Independent transcription of Unicode's ISO8859/8859-15.TXT mapping.
    static constexpr int upper[] = {
        0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
        0x0088, 0x0089, 0x008A, 0x008B, 0x008C, 0x008D, 0x008E, 0x008F,
        0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
        0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F,
        0x00A0, 0x00A1, 0x00A2, 0x00A3, 0x20AC, 0x00A5, 0x0160, 0x00A7,
        0x0161, 0x00A9, 0x00AA, 0x00AB, 0x00AC, 0x00AD, 0x00AE, 0x00AF,
        0x00B0, 0x00B1, 0x00B2, 0x00B3, 0x017D, 0x00B5, 0x00B6, 0x00B7,
        0x017E, 0x00B9, 0x00BA, 0x00BB, 0x0152, 0x0153, 0x0178, 0x00BF,
        0x00C0, 0x00C1, 0x00C2, 0x00C3, 0x00C4, 0x00C5, 0x00C6, 0x00C7,
        0x00C8, 0x00C9, 0x00CA, 0x00CB, 0x00CC, 0x00CD, 0x00CE, 0x00CF,
        0x00D0, 0x00D1, 0x00D2, 0x00D3, 0x00D4, 0x00D5, 0x00D6, 0x00D7,
        0x00D8, 0x00D9, 0x00DA, 0x00DB, 0x00DC, 0x00DD, 0x00DE, 0x00DF,
        0x00E0, 0x00E1, 0x00E2, 0x00E3, 0x00E4, 0x00E5, 0x00E6, 0x00E7,
        0x00E8, 0x00E9, 0x00EA, 0x00EB, 0x00EC, 0x00ED, 0x00EE, 0x00EF,
        0x00F0, 0x00F1, 0x00F2, 0x00F3, 0x00F4, 0x00F5, 0x00F6, 0x00F7,
        0x00F8, 0x00F9, 0x00FA, 0x00FB, 0x00FC, 0x00FD, 0x00FE, 0x00FF
    };
    static_assert(sizeof(upper) / sizeof(upper[0]) == 128,
                  "ISO-8859-15 mapping spans every upper byte");

    using Policy = DRW_CodePageMappingPolicy;
    using Status = DRW_CodePageMappingStatus;
    const char* aliases[] = {
        "ISO8859-15", "iso8859-15", "ISO-8859-15", "iso-8859-15"
    };
    std::size_t definedPairs = 0;
    for (const char* name : aliases) {
        const auto resolution = DRW_TextCodec::resolveCodePage(
            name, Policy::ExactNamed);
        auto converter = DRW_TextCodec::createExactCodePageConverter(name);
        t.expect(resolution.status == Status::ExactImplementationAvailable
                     && resolution.requestedName != nullptr
                     && std::string(resolution.requestedName) == "ISO8859-15"
                     && resolution.decoderName != nullptr
                     && std::string(resolution.decoderName) == "ISO8859-15"
                     && converter != nullptr,
                 "exact ISO-8859-15 spellings resolve to the distinct Latin-9 decoder");
        if (!converter) continue;

        std::string allBytes;
        std::string expected;
        for (unsigned int byte = 0x80u; byte <= 0xFFu; ++byte) {
            const std::string wire(
                1u, static_cast<char>(static_cast<unsigned char>(byte)));
            allBytes += wire;
            const auto utf8 = unicodeScalarExpectedUtf8(upper[byte - 0x80u]);
            t.expect(converter->toUtf8(wire) == utf8
                         && converter->decodeObservation().issues == 0
                         && converter->decodeObservation().nonAsciiBytes
                         && converter->decodeObservation().legacyBytes
                         && converter->fromUtf8(utf8) == wire,
                     "every defined ISO-8859-15 byte decodes and inverse-encodes exactly");
            expected += utf8;
            ++definedPairs;
        }
        t.expect(converter->toUtf8(allBytes) == expected
                     && converter->decodeObservation().issues == 0
                     && converter->decodeObservation().nonAsciiBytes
                     && converter->fromUtf8(expected) == allBytes,
                 "mixed ISO-8859-15 batch preserves C1 controls and Latin-9 replacements");
    }
    t.expect(definedPairs == 4u * 128u,
             "all exact ISO-8859-15 spellings cover all 128 upper-half byte positions");

    for (const char* name : {"ISO8859-15", "ISO-8859-15"}) {
        DRW_TextCodec compatible;
        compatible.setVersion(DRW::AC1015, true);
        compatible.setCodePage(name, true);
        t.expect(compatible.getCodePage() == "ANSI_1252"
                     && compatible.toUtf8("\xA4") == "\xC2\xA4",
                 "Compatible ISO-8859-15 spellings retain their Windows-1252 fallback");
    }
}

bool sameTextObservations(const DRW_DxfTextReadObservations& a,
                          const DRW_DxfTextReadObservations& b) {
    return a.codePagePolicy == b.codePagePolicy
        && a.sourceProfileMask == b.sourceProfileMask
        && a.sourceProfileCalls == b.sourceProfileCalls
        && a.exactNamedDecodeCalls == b.exactNamedDecodeCalls
        && a.unavailableNamedDecodeCalls == b.unavailableNamedDecodeCalls
        && a.semanticDecodeCalls == b.semanticDecodeCalls
        && a.nonAsciiDecodeCalls == b.nonAsciiDecodeCalls
        && a.legacyByteDecodeCalls == b.legacyByteDecodeCalls
        && a.recoveryDecodeCalls == b.recoveryDecodeCalls
        && a.issueDecodeCalls == b.issueDecodeCalls && a.issueCalls == b.issueCalls
        && a.issueMask == b.issueMask && a.firstIssueMask == b.firstIssueMask
        && a.firstGroupCode == b.firstGroupCode && a.firstRecord == b.firstRecord
        && a.firstValueEndOffset == b.firstValueEndOffset
        && a.hasFirstValueEndOffset == b.hasFirstValueEndOffset
        && a.hasFirstRecord == b.hasFirstRecord && a.counterSaturated == b.counterSaturated;
}

void testExactCodePageReader(TestContext& t) {
    using Policy = DRW::DxfCodePagePolicy;
    using Issue = DRW::DxfTextIssue;
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-exact-reader-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    const std::string expected = "L\xC4\x84\xC4\xB8\xCB\x99Z";
    std::size_t handoffs = 0;
    for (const char* page : {"ISO8859-4", "iso-8859-4"}) for (bool modern : {false, true})
        for (bool pageFirst : {false, true}) for (int route = 0; route < 3; ++route) {
        auto bytes = makeTextObservationDrawing(page, modern, pageFirst, route == 2,
            modern ? expected : "L\xA1\xA2\xFFZ");
        // A raw semantic HEADER field precedes both declarations and must be
        // decoded only after version/page discovery, not through a default.
        std::ostringstream prefix(std::ios::binary), field(std::ios::binary);
        std::unique_ptr<dxfWriter> prefixWriter = route == 2
            ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&prefix))
            : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&prefix));
        std::unique_ptr<dxfWriter> fieldWriter = route == 2
            ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&field))
            : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&field));
        prefixWriter->writeString(0, "SECTION"); prefixWriter->writeString(2, "HEADER");
        fieldWriter->writeString(9, "$DIMPOST");
        fieldWriter->writeString(1, modern ? "HDR\xC4\x84Z" : "HDR\xA1Z");
        const auto headerAt = bytes.find(prefix.str());
        t.expect(headerAt != std::string::npos, "authored HEADER prefix is present before inserting its deferred text");
        if (headerAt != std::string::npos) bytes.insert(headerAt + prefix.str().size(), field.str());
        const auto path = directory / (std::to_string(handoffs) + ".dxf");
        if (route != 0) { std::ofstream file(path, std::ios::binary); file.write(bytes.data(), bytes.size()); }
        dx_data data; PreparedDxInterface sink; sink.attach(&data);
        dxfRW reader(path.string().c_str());
        t.expect(reader.dxfCodePagePolicy() == Policy::Compatible, "ordinary facade input mapping default stays compatible");
        reader.setDxfCodePagePolicy(Policy::ExactNamed);
        const bool read = route == 0 ? reader.readAscii(&sink, false, bytes) : reader.read(&sink, false);
        const auto* text = data.mBlock->ent.empty() ? nullptr : dynamic_cast<const DRW_Text*>(data.mBlock->ent.front());
        const auto* line = data.mBlock->ent.empty() ? nullptr : dynamic_cast<const DRW_Line*>(data.mBlock->ent.back());
        const auto seen = reader.getTextReadObservations();
        const auto normalized = std::string_view(page) == "ISO8859-4" ? "ANSI_1257" : "ANSI_1252";
        t.expect(read && text && text->text == expected && line && line->eType == DRW::LINE
                     && line->basePoint.x == 1 && line->basePoint.y == 2 && line->basePoint.z == 3
                     && line->secPoint.x == 4 && line->secPoint.y == 5 && line->secPoint.z == 6
                     && data.mBlock->ent.size() == 2 && seen.codePagePolicy == Policy::ExactNamed
                     && !seen.hasIssues() && (seen.exactNamedDecodeCalls > 0) == !modern
                     && seen.unavailableNamedDecodeCalls == 0
                     && data.headerC.dxfCodePageDeclaration() == page
                     && headerString(data.headerC, "$DIMPOST") == "HDR\xC4\x84Z"
                     && headerString(data.headerC, "$DWGCODEPAGE") == normalized,
                 "exact input reaches typed callbacks across HEADER order/file/memory/binary while modern UTF8 and source history remain distinct");
        ++handoffs;
    }
    std::size_t latin1Routes = 0;
    for (bool binary : {false, true}) {
        auto bytes = makeTextObservationDrawing("LATIN1", false, false, binary, "L\x80Z");
        const auto path = directory / (binary ? "latin1-binary.dxf" : "latin1-ascii.dxf");
        if (binary) {
            std::ofstream file(path, std::ios::binary);
            file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        }
        dx_data data;
        PreparedDxInterface sink;
        sink.attach(&data);
        dxfRW reader(path.string().c_str());
        reader.setDxfCodePagePolicy(Policy::ExactNamed);
        const bool read = binary ? reader.read(&sink, false) : reader.readAscii(&sink, false, bytes);
        const auto* text = data.mBlock->ent.empty() ? nullptr
            : dynamic_cast<const DRW_Text*>(data.mBlock->ent.front());
        const auto observed = reader.getTextReadObservations();
        t.expect(read && text && text->text == "L\xC2\x80Z"
                     && data.mBlock->ent.size() == 2 && observed.exactNamedDecodeCalls > 0
                     && observed.profileCount(DRW::DxfTextSourceProfile::Iso8859_1) > 0
                     && !observed.hasIssues()
                     && data.headerC.dxfCodePageDeclaration() == "LATIN1"
                     && headerString(data.headerC, "$DWGCODEPAGE") == "ANSI_1252",
                 "exact Latin-1 reaches ASCII/binary DXF callbacks without changing compatible HEADER normalization");
        ++latin1Routes;
    }
    t.expect(latin1Routes == 2u, "exact Latin-1 controls cover both DXF encodings");
    std::size_t mtextRoutes = 0;
    for (const char* page : {"ISO8859-4", "ISO-8859-4"}) for (bool binary : {false,true}) {
        auto bytes = makeTextObservationDrawing(page, false, false, binary, "L\xA1\xA2\xFFZ");
        const auto marker = bytes.find("TEXT");
        t.expect(marker != std::string::npos, "authored entity token is present before making the MTEXT control");
        if (marker != std::string::npos) bytes.replace(marker, 4, "MTEXT");
        const auto path = directory / ("mtext-" + std::to_string(mtextRoutes) + ".dxf");
        { std::ofstream file(path,std::ios::binary); file.write(bytes.data(),bytes.size()); }
        dx_data data; PreparedDxInterface sink; sink.attach(&data); dxfRW reader(path.string().c_str());
        reader.setDxfCodePagePolicy(Policy::ExactNamed);
        const bool read = reader.read(&sink,false);
        const auto* text = data.mBlock->ent.empty() ? nullptr : dynamic_cast<const DRW_MText*>(data.mBlock->ent.front());
        t.expect(read && text && text->text == expected && !reader.getTextReadObservations().hasIssues()
                     && reader.getTextReadObservations().exactNamedDecodeCalls > 0 && data.mBlock->ent.size() == 2,
                 "assembled MTEXT uses the original exact mapping once in both file framings");
        ++mtextRoutes;
    }
    std::size_t unsupported = 0;
    const std::string unknownPage = std::string("ANSI_1252") + '\x81';
    for (const std::string& page : {std::string("EUC-JP"), unknownPage})
        for (bool binary : {false, true}) {
        const auto path = directory / ("unavailable-" + std::to_string(unsupported) + ".dxf");
        const auto bytes = makeTextObservationDrawing(page.c_str(), false, false, binary, "L\xA1\\U+0041Z");
        { std::ofstream file(path, std::ios::binary); file.write(bytes.data(), bytes.size()); }
        dx_data data; PreparedDxInterface sink; sink.attach(&data); dxfRW reader(path.string().c_str());
        reader.setDxfCodePagePolicy(Policy::ExactNamed);
        const bool read = reader.read(&sink, false);
        const auto* text = data.mBlock->ent.empty() ? nullptr : dynamic_cast<const DRW_Text*>(data.mBlock->ent.front());
        const auto seen = reader.getTextReadObservations();
        t.expect(read && text && text->text == "L\xEF\xBF\xBD\\U+0041Z"
                     && seen.issueCount(Issue::UnavailableCodePage) > 0
                     && seen.issueCount(Issue::ReplacedSource) > 0 && seen.unavailableNamedDecodeCalls > 0
                     && data.headerC.dxfCodePageDeclaration() == page,
                 "unavailable exact mappings replace and observe high bytes without guessing or metadata-prefix collapse");
        ++unsupported;
    }
    for (const char* page : {"UTF8", "UTF16"}) for (bool modern : {false,true}) {
        const bool utf16 = std::string_view(page) == "UTF16";
        auto bytes = makeTextObservationDrawing(page, modern, false, false,
            !modern && utf16 ? "AB" : "\xC3\xA9");
        dx_data data; PreparedDxInterface sink; sink.attach(&data);
        dxfRW reader((directory / "unicode-profile.dxf").string().c_str());
        reader.setDxfCodePagePolicy(Policy::ExactNamed);
        t.expect(reader.readAscii(&sink, false, bytes)
                     && (reader.getTextReadObservations().issueCount(Issue::UnsupportedEncodingProfile) > 0) == !modern,
                 "an available Unicode decoder does not admit a legacy Unicode wire profile; modern metadata remains inert");
    }
    auto good = makeTextObservationDrawing("ISO8859-4", false, true, false, "L\xA1Z");
    {
        dx_data data; dxfRW reader((directory / "snapshot.dxf").string().c_str());
        class ChangingSink final : public dx_iface {
        public:
            explicit ChangingSink(dxfRW& reader) : reader(reader) {}
            void attach(dx_data* data) { cData = data; currentBlock = data->mBlock; }
            void attachWriter(dxfRW* writer) { dxfW = writer; }
            void addText(const DRW_Text& value) override {
                reader.setDxfCodePagePolicy(Policy::Compatible);
                dx_iface::addText(value);
            }
            dxfRW& reader;
        } sink(reader);
        sink.attach(&data); reader.setDxfCodePagePolicy(Policy::ExactNamed);
        t.expect(reader.readAscii(&sink, false, good), "exact snapshot callback read succeeds");
        const auto retained = reader.getTextReadObservations();
        const auto* first = dynamic_cast<const DRW_Text*>(data.mBlock->ent.front());
        t.expect(first && first->text == "L\xC4\x84Z" && retained.codePagePolicy == Policy::ExactNamed
                     && reader.dxfCodePagePolicy() == Policy::Compatible && retained.exactNamedDecodeCalls > 0,
                 "callback policy mutation does not alter the admitted reader or its owned policy snapshot");
        t.expect(!reader.readAscii(nullptr, false, good)
                     && sameTextObservations(retained, reader.getTextReadObservations()),
                 "rejected null-interface memory reads preserve the previous policy snapshot");
        sink.attachWriter(&reader);
        t.expect(reader.write(&sink, DRW::AC1018, false)
                     && sameTextObservations(retained, reader.getTextReadObservations()),
                 "input mapping policy does not alter writer policy or the retained read observations");
        sink.attachWriter(nullptr);
        dx_data next; sink.attach(&next);
        t.expect(reader.readAscii(&sink, false, good)
                     && reader.getTextReadObservations().codePagePolicy == Policy::Compatible
                     && reader.getTextReadObservations().exactNamedDecodeCalls == 0
                     && reader.getTextReadObservations().hasIssues() && !retained.hasIssues(),
                 "next admitted read uses the changed policy while copied previous evidence remains exact");
    }
    {
        auto first = makeTextObservationDrawing("EUC-JP", false, false, false, "\xA1");
        auto second = makeTextObservationDrawing("ISO8859-4", false, false, false, "\xA1");
        for (const auto& pair : {std::pair<std::string,std::string>{"\nA\n","\nC\n"}, {"\nB\n","\nD\n"}}) {
            const auto at = second.find(pair.first); if (at != std::string::npos) second.replace(at, pair.first.size(), pair.second);
        }
        first.resize(first.rfind("0\nEOF\n")); first += second;
        dx_data data; PreparedDxInterface sink; sink.attach(&data);
        dxfRW reader((directory / "cumulative.dxf").string().c_str()); reader.setDxfCodePagePolicy(Policy::ExactNamed);
        t.expect(reader.readAscii(&sink, false, first)
                     && reader.getTextReadObservations().unavailableNamedDecodeCalls > 0
                     && reader.getTextReadObservations().exactNamedDecodeCalls > 0
                     && reader.getTextReadObservations().hasIssues()
                     && data.headerC.dxfCodePageDeclaration() == "ISO8859-4" && data.mBlock->ent.size() == 4,
                 "later exact metadata cannot wash earlier unavailable decoding out of cumulative evidence");
    }
    {
        auto invalid = good; const auto at = invalid.find("\nB\n");
        t.expect(at != std::string::npos, "late-failure source has its distinct LINE handle");
        if (at != std::string::npos) invalid.replace(at + 1, 1, "INVALID_HANDLE");
        dxfRW reader((directory / "retry.dxf").string().c_str()); reader.setDxfCodePagePolicy(Policy::ExactNamed);
        dx_data bad, fresh; PreparedDxInterface sink; sink.attach(&bad);
        t.expect(!reader.readAscii(&sink, false, invalid) && reader.getTextReadObservations().exactNamedDecodeCalls > 0,
                 "late record failure retains completed exact-prefix observations");
        sink.attach(&fresh);
        t.expect(reader.readAscii(&sink, false, good) && !reader.getTextReadObservations().hasIssues(),
                 "same facade corrected retry resets observations and still uses the explicit mapping policy");
    }
    {
        DRW_TextCodec codec; codec.setDxfCodePagePolicy(Policy::ExactNamed);
        codec.setDxfTextEncoding(DRW::DxfTextEncoding::LegacyLibreCadUtf8);
        codec.setCodePage("ISO-8859-4", true); codec.setVersion(DRW::AC1015, true); codec.reapplyCodePage("ANSI_1252", true);
        t.expect(codec.toUtf8("\xA1") == "\xC4\x84" && codec.lastDecodeObservation().exactNamedMapping
                     && !codec.lastDecodeObservation().recovery && codec.fromUtf8("\xC4\x84") == "\\U+0104",
                 "original exact ISO mapping survives normalized reapplication and cannot become CP1252 recovery/output encoding");
        codec.setCodePage("CP1252", true);
        t.expect(codec.toUtf8("\xC3\xA9") == "\xC3\xA9" && codec.lastDecodeObservation().recovery,
                 "explicit input-only recovery remains available for an actual CP1252 mapping");
        codec.setCodePage("EUC-JP", true);
        t.expect(codec.toUtf8("\\U+0104Z") == "\xC4\x84Z" && codec.lastDecodeObservation().issues == 0,
                 "decoder-independent ASCII and represented CIF do not require an unavailable byte mapping");
        DRW_TextCodec other; other.setCodePage("ISO8859-4", true); other.setVersion(DRW::AC1015, true);
        codec.swap(other);
        t.expect(other.dxfCodePagePolicy() == Policy::ExactNamed && other.toUtf8("\xA1") == "\xEF\xBF\xBD",
                 "swap transfers exact policy and unavailable resolution with their codec state");
        DRW_TextCodec moved(std::move(other));
        t.expect(moved.toUtf8("\xA1") == "\xEF\xBF\xBD" && moved.lastDecodeObservation().unavailableNamedMapping,
                 "move retains exact policy state without borrowing declaration bytes");
        moved.setVersion(DRW::AC1015, false); moved.setCodePage("ISO8859-4", false);
        t.expect(moved.toUtf8("\xA1").empty() && !moved.lastDecodeObservation().exactNamedMapping,
                 "DXF input mapping policy does not change a DWG byte-codec channel");
    }
    {
        dxfRW missing((directory / "never-created.dxf").string().c_str());
        missing.setDxfCodePagePolicy(Policy::ExactNamed);
        dx_data data; PreparedDxInterface sink; sink.attach(&data);
        t.expect(!missing.read(&sink, false) && missing.getTextReadObservations().codePagePolicy == Policy::ExactNamed
                     && missing.getTextReadObservations().semanticDecodeCalls == 0,
                 "admitted early file failure records its requested policy without inventing decode events");
        DRW_DxfTextReadObservations oldPrefix {1,2,3,4,5};
        t.expect(oldPrefix.semanticDecodeCalls == 1 && oldPrefix.nonAsciiDecodeCalls == 2
                     && oldPrefix.legacyByteDecodeCalls == 3 && oldPrefix.recoveryDecodeCalls == 4
                     && oldPrefix.issueDecodeCalls == 5 && oldPrefix.exactNamedDecodeCalls == 0
                     && oldPrefix.codePagePolicy == Policy::Compatible,
                 "appended observation fields preserve the original aggregate initialization prefix");
        DRW_DxfTextReadObservations saturated;
        saturated.exactNamedDecodeCalls = saturated.unavailableNamedDecodeCalls = (std::numeric_limits<std::uint64_t>::max)();
        DRW_TextDecodeObservation value; value.exactNamedMapping = value.unavailableNamedMapping = true;
        dxfReader::appendTextReadObservation(saturated, value, 1, 0, false);
        t.expect(saturated.counterSaturated && saturated.exactNamedDecodeCalls == (std::numeric_limits<std::uint64_t>::max)()
                     && saturated.unavailableNamedDecodeCalls == (std::numeric_limits<std::uint64_t>::max)(),
                 "new mapping observation counters saturate without rollover or clearing existing evidence");
    }
    std::cout << "Exact reader policy: handoffs=" << handoffs << ", MTEXT routes=" << mtextRoutes << ", unavailable routes=" << unsupported
              << ", Unicode profile controls=4; callback/write/reuse/cumulative/failure/recovery/move/saturation controls pass\n";
    if (t.failures == 0) std::filesystem::remove_all(directory);
    else std::cerr << "Exact reader evidence: " << directory << '\n';
}

void testTextObservationHandoff(TestContext& t) {
    using Issue = DRW_DxfTextIssue;
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-text-observations-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    struct Row {
        const char* page;
        bool modern;
        std::string bytes, expected;
        std::uint32_t issues;
    };
    const Row rows[] = {
        {"ISO8859-4", false, "L\xA1R", "LR", textIssueMask({Issue::CompatibilityCodePage, Issue::UndefinedByteSequence, Issue::DiscardedSource})},
        {"local-unknown", false, "plain", "plain", 0},
        {"local-unknown", false, "\\U+0410", "\xD0\x90", 0},
        {"CP866", false, "\x80", "\xE2\x82\xAC", textIssueMask({Issue::UnavailableCodePage})},
        {"ANSI_1252", false, "L\x81R", "LR", textIssueMask({Issue::UndefinedByteSequence, Issue::DiscardedSource})},
        {"local-unknown", true, "\xC3\xA9", "\xC3\xA9", 0},
        {"ANSI_1252", true, "L\x80R", "L\x80R", textIssueMask({Issue::InvalidUtf8, Issue::InvalidUnicodeOutput})},
        {"ANSI_1252", false, "\\M+48861", "\\M+48861", textIssueMask({Issue::UnrepresentedEscape})},
        {"ANSI_1252", false, "\\U+12G4", "\\U+12G4", textIssueMask({Issue::MalformedEscape, Issue::UnrepresentedEscape})},
        {"ANSI_1252", true, "\\U+D800", "\\U+D800", textIssueMask({Issue::InvalidUnicodeOutput, Issue::UnrepresentedEscape})},
        {"ANSI_932", false, "L\x82", "L\xE3\x83\xBB", textIssueMask({Issue::UndefinedByteSequence, Issue::ReplacedSource})},
        {"ANSI_1252", true, "\\U+D800", "\\U+D800", textIssueMask({Issue::InvalidUnicodeOutput, Issue::UnrepresentedEscape})},
        {"UTF8", false, "\xC3\xA9", "\xC3\x83\xC2\xA9", textIssueMask({Issue::ReappliedCodePage})},
        {nullptr, false, "\xE9", "\xC3\xA9", textIssueMask({Issue::UnspecifiedCodePage})},
        {"ISO8859-4", false, "plain", "plain", 0},
        {"local-\x81", false, "plain", "plain", 0}
    };
    std::size_t cases = 0;
    for (const auto& row : rows) for (bool pageFirst : {false, true})
        for (int route = 0; route < 3; ++route) {
        auto bytes = makeTextObservationDrawing(row.page, row.modern, pageFirst, route == 2, row.bytes);
        const auto path = directory / (std::to_string(cases) + ".dxf");
        if (route != 0) {
            std::ofstream output(path, std::ios::binary);
            output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        }
        dx_data data;
        PreparedDxInterface sink;
        sink.attach(&data);
        DRW_DxfTextReadObservations observed;
        bool read = false;
        {
            dxfRW facade(path.string().c_str());
            read = route == 0 ? facade.readAscii(&sink, false, bytes) : facade.read(&sink, false);
            observed = facade.getTextReadObservations();
            t.expect(facade.getError() == DRW::BAD_NONE && !facade.getLastDiagnostic().failed(),
                     "text observations do not change the tolerant primary read result");
        }
        const auto* text = data.mBlock->ent.empty() ? nullptr
            : dynamic_cast<const DRW_Text*>(data.mBlock->ent.front());
        bool correct = read && text != nullptr && text->text == row.expected
            && data.mBlock->ent.size() == 2 && observed.issueMask == row.issues
            && observed.issueDecodeCalls == (row.issues != 0 ? 1u : 0u)
            && observed.semanticDecodeCalls != 0 && !observed.counterSaturated;
        if (row.issues != 0)
            correct = correct && observed.firstGroupCode == 1 && observed.hasFirstRecord
                && observed.firstRecord != 0 && observed.firstIssueMask == row.issues
                && observed.hasFirstValueEndOffset && observed.firstValueEndOffset != 0
                && observed.firstValueEndOffset <= bytes.size();
        for (std::size_t i = 0; i < observed.issueCalls.size(); ++i)
            correct = correct && observed.issueCalls[i] == ((row.issues & (1u << i)) != 0 ? 1u : 0u);
        t.expect(correct, "owned facade snapshot preserves source issues across metadata order/memory/ASCII/binary");
        if (!correct) std::cerr << "Handoff row " << cases << " issues=" << observed.issueMask
                                << " expected=" << row.issues << '\n';
        ++cases;
    }

    auto bad = makeTextObservationDrawing("ISO8859-4", false, false, false, "L\xA1R");
    auto clean = makeTextObservationDrawing("ANSI_1252", false, false, false, "plain");
    dxfRW reusable((directory / "write-preserves-snapshot.dxf").string().c_str());
    dx_data data;
    PreparedDxInterface sink;
    sink.attach(&data);
    t.expect(reusable.readAscii(&sink, false, bad), "lifecycle starts with observed lossy source");
    const auto retained = reusable.getTextReadObservations();
    t.expect(!reusable.readAscii(nullptr, false, clean)
                 && sameTextObservations(retained, reusable.getTextReadObservations()),
             "readAscii null-interface rejection preserves the last source snapshot");
    sink.attachWriter(&reusable);
    t.expect(reusable.write(&sink, DRW::AC1024, false)
                 && sameTextObservations(retained, reusable.getTextReadObservations()),
             "write preserves last-read observations without adding a text-loss policy");
    sink.attachWriter(nullptr);
    dx_data fresh;
    sink.attach(&fresh);
    t.expect(reusable.readAscii(&sink, false, clean)
                 && !reusable.getTextReadObservations().hasIssues() && retained.hasIssues(),
             "next admitted read resets observations without changing a copied snapshot");
    const auto at = bad.find("\nB\n");
    auto invalid = bad;
    if (at != std::string::npos) invalid.replace(at + 1, 1, "INVALID_HANDLE");
    dx_data partial;
    sink.attach(&partial);
    t.expect(at != std::string::npos && !reusable.readAscii(&sink, false, invalid)
                 && reusable.getTextReadObservations().issueMask == retained.issueMask,
             "later primary record failure keeps earlier source-loss observations");
    t.expect(!reusable.read(nullptr, false)
                 && reusable.getTextReadObservations().semanticDecodeCalls == 0,
             "admitted file read clears source observations even on early argument failure");

    class LiveSink final : public dx_iface {
    public:
        LiveSink(dx_data& data, dxfRW& facade, std::string& nested)
            : codec(facade), content(nested) { cData = &data; currentBlock = data.mBlock; }
        void addText(const DRW_Text& value) override {
            during = codec.getTextReadObservations();
            nestedRejected = !codec.readAscii(this, false, content)
                && !codec.write(this, DRW::AC1024, false)
                && sameTextObservations(during, codec.getTextReadObservations());
            if (throwNow) throw std::runtime_error("authored observation callback failure");
            dx_iface::addText(value);
        }
        dxfRW& codec;
        std::string& content;
        DRW_DxfTextReadObservations during;
        bool nestedRejected {false}, throwNow {false};
    };
    dx_data liveData;
    LiveSink live(liveData, reusable, clean);
    t.expect(reusable.readAscii(&live, false, bad) && live.nestedRejected
                 && live.during.issueMask == retained.issueMask
                 && reusable.getTextReadObservations().issueMask == retained.issueMask,
             "callback snapshots and rejected nested operations preserve active read observations");
    live.throwNow = true;
    bool threw = false;
    try { (void)reusable.readAscii(&live, false, bad); }
    catch (const std::runtime_error&) { threw = true; }
    t.expect(threw && reusable.getTextReadObservations().issueMask == retained.issueMask,
             "scope unwind retains completed decode observations after a throwing callback");
    dx_data afterThrow;
    sink.attach(&afterThrow);
    t.expect(reusable.readAscii(&sink, false, clean) && !reusable.getTextReadObservations().hasIssues(),
             "corrected same-facade read resets after callback exception");

    // Deferred HEADER values are decoded after metadata discovery. Their
    // original physical positions are unavailable; never label ENDSEC as it.
    std::string deferred = "0\nSECTION\n2\nHEADER\n9\n$CLAYER\n8\nL\x81R\n"
        "9\n$ACADVER\n1\nAC1015\n9\n$DWGCODEPAGE\n3\nANSI_1252\n"
        "0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nLINE\n5\nA\n8\n0\n"
        "10\n1\n20\n2\n11\n3\n21\n4\n0\nENDSEC\n0\nEOF\n";
    dx_data headerData;
    sink.attach(&headerData);
    t.expect(reusable.readAscii(&sink, false, deferred)
                 && reusable.getTextReadObservations().firstGroupCode == 8
                 && !reusable.getTextReadObservations().hasFirstRecord
                 && reusable.getTextReadObservations().firstRecord == 0
                 && !reusable.getTextReadObservations().hasFirstValueEndOffset
                 && reusable.getTextReadObservations().firstValueEndOffset == 0,
             "deferred HEADER context retains field code without fabricating a physical record");
    std::string changedMetadata =
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n"
        "9\n$DWGCODEPAGE\n3\nISO8859-4\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n0\nTEXT\n5\nA\n8\n0\n"
        "10\n0\n20\n0\n40\n1\n1\n\xA1\n0\nENDSEC\n"
        "0\nSECTION\n2\nHEADER\n9\n$DWGCODEPAGE\n3\nANSI_1252\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n0\nTEXT\n5\nB\n8\n0\n"
        "10\n1\n20\n1\n40\n1\n1\nplain\n0\nENDSEC\n0\nEOF\n";
    dx_data changedData;
    sink.attach(&changedData);
    t.expect(reusable.readAscii(&sink, false, changedMetadata)
                 && headerString(changedData.headerC, "$DWGCODEPAGE") == "ANSI_1252"
                 && reusable.getTextReadObservations().issueMask == retained.issueMask
                 && reusable.getTextReadObservations().issueDecodeCalls == 1,
             "later canonical metadata and clean text cannot erase an earlier source-loss event");
    auto capped = makeCodePageDiagnosticDrawing("ISO8859-4", false, false, false, 20);
    const std::string oldPayload = "ascii-control";
    const auto cappedAt = capped.find(oldPayload);
    if (cappedAt != std::string::npos) capped.replace(cappedAt, oldPayload.size(), "\xA1");
    dx_data cappedData;
    sink.attach(&cappedData);
    t.expect(cappedAt != std::string::npos && reusable.readAscii(&sink, false, capped)
                 && reusable.getLastDiagnostic().secondary.size() == DRW_OperationDiagnostic::MaxSecondaryEntries
                 && reusable.getTextReadObservations().issueMask == retained.issueMask,
             "source-loss observations survive a full bounded diagnostic advisory list");
    reusable.setDxfTextEncoding(DRW::DxfTextEncoding::LegacyLibreCadUtf8);
    auto recovered = makeTextObservationDrawing("ANSI_1252", false, false, false, "\xC3\xA9");
    dx_data recoveredData;
    sink.attach(&recoveredData);
    t.expect(reusable.readAscii(&sink, false, recovered)
                 && !reusable.getTextReadObservations().hasIssues()
                 && reusable.getTextReadObservations().recoveryDecodeCalls != 0
                 && reusable.getTextReadObservations().legacyByteDecodeCalls == 0,
             "facade recovery observations describe the actual opt-in UTF8 path");
    const auto throwingPath = directory / "throwing-file-read.dxf";
    {
        std::ofstream output(throwingPath, std::ios::binary);
        output.write(bad.data(), static_cast<std::streamsize>(bad.size()));
    }
    DRW_DxfTextReadObservations fileUnwind;
    {
        dxfRW fileCodec(throwingPath.string().c_str());
        dx_data fileData;
        LiveSink fileSink(fileData, fileCodec, clean);
        fileSink.throwNow = true;
        bool fileThrew = false;
        try { (void)fileCodec.read(&fileSink, false); }
        catch (const std::runtime_error&) { fileThrew = true; }
        fileUnwind = fileCodec.getTextReadObservations();
        t.expect(fileThrew && fileUnwind.issueMask == retained.issueMask,
                 "file callback unwind snapshots observations without touching the destroyed stream");
    }
    dxfRW missing((directory / "not-created.dxf").string().c_str());
    dx_data missingData;
    sink.attach(&missingData);
    t.expect(missing.readAscii(&sink, false, bad) && !missing.read(&sink, false)
                 && missing.getTextReadObservations().semanticDecodeCalls == 0
                 && fileUnwind.issueMask == retained.issueMask,
             "file open failure resets new observations while destroyed-facade copies remain valid");
    std::cout << "Text observation facade handoffs=" << cases << ", lifecycle/context controls=15\n";
    if (t.failures == 0) std::filesystem::remove_all(directory);
    else std::cerr << "Text observation evidence: " << directory << '\n';
}

bool sameReadDiagnostic(const DRW_OperationDiagnostic& a,
                        const DRW_OperationDiagnostic& b) {
    if (a.operation != b.operation || a.phase != b.phase || a.cause != b.cause
        || a.code != b.code || a.message != b.message || a.offset != b.offset
        || a.handle != b.handle || a.hasOffset != b.hasOffset || a.hasHandle != b.hasHandle
        || a.secondary.size() != b.secondary.size()) return false;
    for (std::size_t i = 0; i < a.secondary.size(); ++i) {
        const auto& x = a.secondary[i];
        const auto& y = b.secondary[i];
        if (x.phase != y.phase || x.cause != y.cause || x.code != y.code
            || x.message != y.message || x.offset != y.offset || x.handle != y.handle
            || x.hasOffset != y.hasOffset || x.hasHandle != y.hasHandle) return false;
    }
    return true;
}

bool dxfPrefixMatches(const dx_dxfTextPrefix& prefix, std::string_view expected) {
    const auto retained = std::min(expected.size(), prefix.bytes.size());
    return prefix.sourceBytes == expected.size() && prefix.length == retained
        && prefix.truncated == (retained != expected.size())
        && std::equal(expected.begin(), expected.begin() + retained, prefix.bytes.begin());
}

void testConverterDxfReadOutcome(TestContext& t) {
    using Kind = dx_dxfCodePageKind;
    using Issue = DRW_DxfTextIssue;
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-dxf-import-outcome-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    const auto writeInput = [&](const std::string& name, const std::string& bytes) {
        const auto path = directory / name;
        std::ofstream output(path, std::ios::binary);
        output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        if (!output) throw std::runtime_error("authored outcome input could not be written");
        return path;
    };
    struct Row { const char* page; bool modern; std::string bytes; Kind kind; };
    const Row rows[] = {
        {"ANSI_1252", false, "plain", Kind::Canonical},
        {"CP1252", false, "\xE9", Kind::ExactAlias},
        {"ISO8859-4", false, "L\xA1R", Kind::CompatibilityAlias},
        {"CP866", false, "\x80", Kind::Unavailable},
        {"local-unknown", false, "plain", Kind::Unknown},
        {"local-unknown", true, "\xC3\xA9", Kind::Unknown},
        {nullptr, false, "\xE9", Kind::Unspecified},
        {"", false, "\xE9", Kind::Unknown},
        {"ANSI_1252", true, "\x80", Kind::Canonical}
    };
    std::size_t handoffs = 0;
    for (const auto& row : rows) for (bool pageFirst : {false, true})
        for (bool binary : {false, true}) {
        const auto bytes = makeTextObservationDrawing(row.page, row.modern, pageFirst, binary, row.bytes);
        const auto path = writeInput(std::to_string(handoffs) + ".dxf", bytes);
        dx_data reference;
        PreparedDxInterface referenceSink;
        referenceSink.attach(&reference);
        dxfRW reader(path.string().c_str());
        const bool direct = reader.read(&referenceSink, false);
        dx_dxfReadOutcome owned;
        {
            dx_data data;
            dx_iface adapter;
            const bool imported = adapter.fileImport(path.string(), &data, false);
            owned = data.dxfReadOutcome;
            const bool correct = imported == direct && data.sourceFormat == dx_source_format::DXF
                         && !data.dwgReadOutcome.attempted && owned.attempted
                         && owned.readReturned && !owned.readThrew && owned.operationSucceeded
                         && owned.sourceVersion == reader.getSourceVersion()
                         && owned.codecVersion == reader.getVersion() && owned.error == reader.getError()
                         && owned.diagnosticAvailable && !owned.diagnosticCaptureFailed
                         && sameReadDiagnostic(owned.diagnostic, reader.getLastDiagnostic())
                         && sameTextObservations(owned.observations, reader.getTextReadObservations())
                         && owned.headerDelivered && owned.hasCodePageDeclaration == (row.page != nullptr)
                         && owned.finalDeclarationKind == row.kind
                         && dxfPrefixMatches(owned.finalDeclaration, row.page == nullptr ? "" : row.page)
                         && dxfPrefixMatches(owned.finalDeclarationSelection,
                             row.page == nullptr ? "" : row.kind == Kind::CompatibilityAlias ? "ANSI_1257" : "ANSI_1252")
                         && dxfPrefixMatches(owned.finalNormalizedCodePage, headerString(data.headerC, "$DWGCODEPAGE"));
            t.expect(correct,
                     "converter owns exact DXF outcomes without altering ASCII/binary/metadata-order reads");
            if (!correct) std::cerr << "Outcome handoff row=" << handoffs
                << " page=" << (row.page == nullptr ? "<absent>" : row.page)
                << " kind=" << static_cast<int>(owned.finalDeclarationKind)
                << " expectedKind=" << static_cast<int>(row.kind)
                << " declared=" << owned.hasCodePageDeclaration
                << " normalized=" << headerString(data.headerC, "$DWGCODEPAGE") << '\n';
        }
        t.expect(owned.readReturned && sameTextObservations(owned.observations, reader.getTextReadObservations()),
                 "copied converter outcome survives both facade and model destruction");
        ++handoffs;
    }

    std::size_t versionControls = 0;
    for (const char* version : {"", "AC9999", "AC1018", "AC1032"}) {
        std::string bytes = "0\nSECTION\n2\nHEADER\n";
        if (*version != '\0') bytes += std::string("9\n$ACADVER\n1\n") + version + '\n';
        bytes += "0\nENDSEC\n0\nEOF\n";
        const auto path = writeInput("version-" + std::to_string(versionControls) + ".dxf", bytes);
        dx_data data;
        dx_iface adapter;
        const auto accepted = std::string_view(version) == "AC1018" ? DRW::AC1018
            : std::string_view(version) == "AC1032" ? DRW::AC1032 : DRW::UNKNOWNV;
        const auto codecVersion = std::string_view(version) == "AC9999" ? DRW::UNKNOWNV
            : accepted == DRW::AC1018 ? DRW::AC1015 : DRW::AC1021;
        const bool imported = adapter.fileImport(path.string(), &data, false);
        const bool correct = imported
                     && data.dxfReadOutcome.sourceVersion == accepted
                     && data.dxfReadOutcome.codecVersion == codecVersion;
        t.expect(correct,
                 "accepted source revision remains separate from default/folded codec revision");
        if (!correct) std::cerr << "Version control=" << version << " accepted="
            << data.dxfReadOutcome.sourceVersion << " codec=" << data.dxfReadOutcome.codecVersion << '\n';
        ++versionControls;
    }
    {
        const auto path = writeInput("version-repeated.dxf",
            "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n9\n$ACADVER\n1\nAC1009\n0\nENDSEC\n0\nEOF\n");
        dx_data data;
        dx_iface adapter;
        t.expect(adapter.fileImport(path.string(), &data, false)
                     && data.dxfReadOutcome.sourceVersion == DRW::AC1021
                     && headerString(data.headerC, "$ACADVER") == "AC1009",
                 "last raw lower-revision spelling cannot masquerade as accepted source authority");
        ++versionControls;
    }

    const auto lossy = makeTextObservationDrawing("ISO8859-4", false, false, false, "L\xA1R");
    const auto clean = makeTextObservationDrawing("ANSI_1252", false, false, false, "plain");
    const auto lossyPath = writeInput("lossy.dxf", lossy);
    const auto cleanPath = writeInput("clean.dxf", clean);
    dx_data reused;
    dx_iface adapter;
    t.expect(adapter.fileImport(lossyPath.string(), &reused, false), "outcome reuse starts with lossy source");
    const auto saved = reused.dxfReadOutcome;
    auto invalid = lossy;
    const auto at = invalid.find("\nB\n");
    if (at != std::string::npos) invalid.replace(at + 1, 1, "INVALID_HANDLE");
    const auto invalidPath = writeInput("later-invalid.dxf", invalid);
    t.expect(at != std::string::npos && !adapter.fileImport(invalidPath.string(), &reused, false)
                 && reused.dxfReadOutcome.readReturned && !reused.dxfReadOutcome.operationSucceeded
                 && reused.dxfReadOutcome.sourceVersion == DRW::AC1015
                 && reused.dxfReadOutcome.error != DRW::BAD_NONE
                 && reused.dxfReadOutcome.diagnostic.failed()
                 && reused.dxfReadOutcome.observations.issueMask == saved.observations.issueMask,
             "failed read retains accepted revision, primary diagnostic and earlier decode loss separately");
    t.expect(!adapter.fileImport((directory / "missing.dxf").string(), &reused, false)
                 && reused.dxfReadOutcome.attempted && reused.dxfReadOutcome.readReturned
                 && reused.dxfReadOutcome.sourceVersion == DRW::UNKNOWNV
                 && reused.dxfReadOutcome.error == DRW::BAD_OPEN
                 && !reused.dxfReadOutcome.headerDelivered
                 && !reused.dxfReadOutcome.hasCodePageDeclaration
                 && reused.dxfReadOutcome.finalDeclaration.sourceBytes == 0
                 && reused.dxfReadOutcome.observations.semanticDecodeCalls == 0,
             "open failure resets evidence rather than inheriting the reused model's old HEADER");
    const auto earlyPath = writeInput("early-invalid.dxf", "not-a-group\nvalue\n");
    t.expect(!adapter.fileImport(earlyPath.string(), &reused, false)
                 && reused.dxfReadOutcome.readReturned && !reused.dxfReadOutcome.readThrew
                 && reused.dxfReadOutcome.sourceVersion == DRW::UNKNOWNV
                 && !reused.dxfReadOutcome.headerDelivered
                 && reused.dxfReadOutcome.diagnosticAvailable
                 && reused.dxfReadOutcome.diagnostic.failed(),
             "early parser failure retains its diagnostic without stale header/revision evidence");
    t.expect(adapter.fileImport(cleanPath.string(), &reused, false)
                 && !reused.dxfReadOutcome.observations.hasIssues()
                 && saved.observations.hasIssues(),
             "corrected reuse clears the next outcome without changing copied prior evidence");
    t.expect(!adapter.fileImport((directory / "invalid.ext").string(), &reused, false)
                 && !reused.dxfReadOutcome.attempted && !reused.dwgReadOutcome.attempted
                 && reused.sourceFormat == dx_source_format::Unknown,
             "invalid-extension attempts clear both import outcome envelopes");
    const auto dwgPath = writeInput("local-truncated.dwg", "AC1027");
    t.expect(!adapter.fileImport(dwgPath.string(), &reused, false)
                 && !reused.dxfReadOutcome.attempted && reused.dwgReadOutcome.attempted
                 && reused.sourceFormat == dx_source_format::DWG,
             "changed-format read resets DXF evidence without changing DWG outcomes");

    auto capped = makeCodePageDiagnosticDrawing("ISO8859-4", false, false, false, 20);
    const auto cappedAt = capped.find("ascii-control");
    if (cappedAt != std::string::npos) capped.replace(cappedAt, 13, "\xA1");
    const auto cappedPath = writeInput("capped.dxf", capped);
    dx_data cappedData;
    t.expect(adapter.fileImport(cappedPath.string(), &cappedData, false)
                 && cappedData.dxfReadOutcome.diagnostic.secondary.size() == 16
                 && cappedData.dxfReadOutcome.observations.hasIssues(),
             "a full diagnostic advisory cap cannot erase converter source-loss state");
    const auto largePage = std::string(4096, 'x') + "-distinct-suffix";
    const auto longPath = writeInput("long-page.dxf",
        makeTextObservationDrawing(largePage.c_str(), false, false, false, "plain"));
    dx_data longData;
    t.expect(adapter.fileImport(longPath.string(), &longData, false)
                 && dxfPrefixMatches(longData.dxfReadOutcome.finalDeclaration, largePage)
                 && longData.dxfReadOutcome.finalDeclaration.truncated
                 && longData.headerC.dxfCodePageDeclaration() == largePage
                 && longData.dxfReadOutcome.finalDeclarationKind == Kind::Unknown,
             "bounded prefix retains total length/truncation while HEADER owns the full declaration");
    const std::string changed =
        "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1015\n9\n$DWGCODEPAGE\n3\nISO8859-4\n0\nENDSEC\n"
        "0\nSECTION\n2\nENTITIES\n0\nTEXT\n5\nA\n8\n0\n10\n0\n20\n0\n40\n1\n1\n\xA1\n0\nENDSEC\n"
        "0\nSECTION\n2\nHEADER\n9\n$DWGCODEPAGE\n3\nANSI_1252\n0\nENDSEC\n0\nEOF\n";
    const auto changedPath = writeInput("changed-metadata.dxf", changed);
    dx_data changedData;
    t.expect(adapter.fileImport(changedPath.string(), &changedData, false)
                 && changedData.dxfReadOutcome.finalDeclarationKind == Kind::Canonical
                 && dxfPrefixMatches(changedData.dxfReadOutcome.finalDeclaration, "ANSI_1252")
                 && changedData.dxfReadOutcome.observations.issueCount(Issue::DiscardedSource) == 1,
             "last canonical metadata does not relabel or erase earlier semantic loss");

    class ThrowingImport final : public dx_iface {
    public:
        bool headerThrow{false}, allocationFailure{false}, enabled{true};
        void addHeader(const DRW_Header* value) override {
            if (headerThrow) throw 71;
            dx_iface::addHeader(value);
        }
        void addText(const DRW_Text& value) override {
            if (enabled) {
                failImportObservationAllocations = allocationFailure;
                throw 73;
            }
            dx_iface::addText(value);
        }
    };
    for (int mode = 0; mode < 3; ++mode) {
        dx_data data;
        data.sourceVersion = DRW::AC1032;
        ThrowingImport throwing;
        throwing.headerThrow = mode == 0;
        throwing.allocationFailure = mode == 2;
        bool caughtOriginal = false;
        try { (void)throwing.fileImport(lossyPath.string(), &data, false); }
        catch (int sentinel) { failImportObservationAllocations = false; caughtOriginal = sentinel == (mode == 0 ? 71 : 73); }
        catch (...) { failImportObservationAllocations = false; }
        failImportObservationAllocations = false;
        const auto& outcome = data.dxfReadOutcome;
        const bool unwindEvidence = caughtOriginal && outcome.attempted && outcome.readThrew && !outcome.readReturned
            && !outcome.operationSucceeded && outcome.sourceVersion == DRW::UNKNOWNV
            && data.sourceVersion == DRW::AC1032 && outcome.headerDelivered == (mode != 0)
            && outcome.observations.hasIssues() == (mode != 0)
            && outcome.diagnosticAvailable == (mode != 2)
            && outcome.diagnosticCaptureFailed == (mode == 2);
        t.expect(unwindEvidence,
                 "RAII unwind retains prefix evidence without inventing a version or replacing the original exception");
        if (mode != 2)
            t.expect(outcome.diagnostic.secondary.size() == 1,
                     "throwing read preserves already recorded secondary advisories without synthesizing a primary failure");
        throwing.headerThrow = throwing.allocationFailure = throwing.enabled = false;
        dx_data retry;
        t.expect(throwing.fileImport(cleanPath.string(), &retry, false)
                     && retry.dxfReadOutcome.readReturned && !retry.dxfReadOutcome.readThrew
                     && !retry.dxfReadOutcome.observations.hasIssues()
                     && retry.dxfReadOutcome.diagnosticAvailable
                     && !retry.dxfReadOutcome.diagnosticCaptureFailed,
                 "same adapter recovers after callback/diagnostic-copy exceptions with fresh evidence");
    }
    std::cout << "Converter DXF outcome: handoffs=" << handoffs
              << ", version controls=" << versionControls
              << ", reuse/context controls=10, exception/retry pairs=3\n";
    if (t.failures == 0) std::filesystem::remove_all(directory);
    else std::cerr << "Converter DXF outcome evidence: " << directory << '\n';
}

void testConsumedTextProfiles(TestContext& t) {
    using Profile = DRW::DxfTextSourceProfile;
    using Mapping = DRW::DxfCodePagePolicy;
    using Status = dx_dxfSourceTextStatus;
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-consumed-profiles-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    const auto writeBytes = [](const std::filesystem::path& path, const std::string& bytes) {
        std::ofstream file(path, std::ios::binary); file.write(bytes.data(), bytes.size());
        if (!file) throw std::runtime_error("authored profile input could not be written");
    };
    const auto partitioned = [](const DRW_DxfTextReadObservations& value) {
        std::uint64_t total = 0; std::uint64_t mask = 0;
        for (std::size_t i = 0; i < value.sourceProfileCalls.size(); ++i) {
            const auto calls = value.sourceProfileCalls[i];
            if (calls > (std::numeric_limits<std::uint64_t>::max)() - total) return false;
            total += calls;
            if (calls) mask |= DRW::dxfTextSourceProfileMask(static_cast<Profile>(i));
        }
        return total == value.semanticDecodeCalls && mask == value.sourceProfileMask
            && value.profileCount(Profile::NoProfile) == 0;
    };
    struct Row { const char* page; const char* bytes; const char* expected; Profile profile; };
    // Independent literal anchors from the existing canonical codec controls,
    // not expected strings manufactured through a production converter.
    const Row rows[] = {
        {"ANSI_874", "\xA1", "\xE0\xB8\x81", Profile::Windows874},
        {"ANSI_932", "\x82\xA0", "\xE3\x81\x82", Profile::Windows932},
        {"ANSI_936", "\xD6\xD0", "\xE4\xB8\xAD", Profile::Windows936},
        {"ANSI_949", "\xB0\xA1", "\xEA\xB0\x80", Profile::Windows949},
        {"ANSI_950", "\xA4\xA4", "\xE4\xB8\xAD", Profile::Windows950},
        {"ANSI_1250", "\xA5", "\xC4\x84", Profile::Windows1250},
        {"ANSI_1251", "\xC0", "\xD0\x90", Profile::Windows1251},
        {"ANSI_1252", "\xE9", "\xC3\xA9", Profile::Windows1252},
        {"ANSI_1253", "\xC1", "\xCE\x91", Profile::Windows1253},
        {"ANSI_1254", "\xD0", "\xC4\x9E", Profile::Windows1254},
        {"ANSI_1255", "\xE0", "\xD7\x90", Profile::Windows1255},
        {"ANSI_1256", "\xC7", "\xD8\xA7", Profile::Windows1256},
        {"ANSI_1257", "\xC0", "\xC4\x84", Profile::Windows1257},
        {"ANSI_1258", "\xD5", "\xC6\xA0", Profile::Windows1258},
        {"MACINTOSH", "\x80", "\xC3\x84", Profile::MacRoman},
        {"ISO8859-2", "\x80", "\xC2\x80", Profile::Iso8859_2},
        {"ISO8859-3", "\xA1", "\xC4\xA6", Profile::Iso8859_3},
        {"ISO8859-5", "\xA1", "\xD0\x81", Profile::Iso8859_5},
        {"ISO8859-6", "\xAC", "\xD8\x8C", Profile::Iso8859_6},
        {"ISO8859-7", "\xC1", "\xCE\x91", Profile::Iso8859_7},
        {"ISO8859-8", "\xE0", "\xD7\x90", Profile::Iso8859_8},
        {"ISO8859-9", "\xD0", "\xC4\x9E", Profile::Iso8859_9},
        {"ISO8859-10", "\xA1", "\xC4\x84", Profile::Iso8859_10},
        {"ISO8859-11", "\xA1", "\xE0\xB8\x81", Profile::Iso8859_11},
        {"ISO8859-13", "\xA1", "\xE2\x80\x9D", Profile::Iso8859_13},
        {"ISO8859-14", "\xA1", "\xE1\xB8\x82", Profile::Iso8859_14},
        {"ISO8859-15", "\xA4", "\xE2\x82\xAC", Profile::Iso8859_15}
    };
    std::size_t handoffs = 0, exports = 0;
    for (const auto& row : rows) for (bool binary : {false,true}) {
        const auto input = directory / (std::to_string(handoffs) + ".dxf");
        const std::string expected = std::string("L") + row.expected + "Z";
        writeBytes(input, makeTextObservationDrawing(row.page, false, false, binary,
                                                    std::string("L") + row.bytes + "Z"));
        dx_data data; dx_iface adapter;
        const bool read = adapter.fileImport(input.string(), &data, false, Mapping::ExactNamed);
        const auto& observations = data.dxfReadOutcome.observations;
        const auto* text = data.mBlock->ent.empty() ? nullptr : dynamic_cast<const DRW_Text*>(data.mBlock->ent.front());
        const auto admission = dx_iface::admitDxfSourceText(data).status;
        const auto expectedAdmission = row.profile == Profile::MacRoman
                || row.profile == Profile::Iso8859_2
                || row.profile == Profile::Iso8859_3
                || row.profile == Profile::Iso8859_5
                || row.profile == Profile::Iso8859_6
                || row.profile == Profile::Iso8859_7
                || row.profile == Profile::Iso8859_8
                || row.profile == Profile::Iso8859_9
                || row.profile == Profile::Iso8859_10
                || row.profile == Profile::Iso8859_11
                || row.profile == Profile::Iso8859_13
                || row.profile == Profile::Iso8859_14
                || row.profile == Profile::Iso8859_15
            ? Status::UnsupportedProfile : Status::Admitted;
        t.expect(read && text && text->text == expected && partitioned(observations)
                     && observations.profileCount(row.profile) == 1 && !observations.hasIssues()
                     && data.headerC.dxfCodePageDeclaration() == row.page
                     && admission == expectedAdmission,
                 "exact bytes retain actual per-consumption identity and profile-specific support gating");
        if (row.profile == Profile::Windows932
            || row.profile == Profile::Windows949
            || row.profile == Profile::Windows1252)
            for (const auto target : {DRW::AC1018,DRW::AC1021}) {
            const auto output = directory / ("first-" + std::to_string(handoffs) + "-" + std::to_string(target) + ".dxf");
            t.expect(adapter.fileExport(output.string(), target, binary, &data, false, false,
                                         dx_dxfSourceTextPolicy::RequireValidSource, DRW::DxfTextWritePolicy::RequireLossless),
                     "profile-admitted exact input composes with explicit strict source and target output");
            if (row.profile == Profile::Windows949 && target == DRW::AC1018
                && !binary) {
                std::ifstream exportedInput(output, std::ios::binary);
                const std::string exportedBytes(
                    (std::istreambuf_iterator<char>(exportedInput)),
                    std::istreambuf_iterator<char>());
                const std::string expectedLegacyText = std::string("1\nL")
                    + makeMalformedBytes({0xB0, 0xA1}) + "Z\n";
                t.expect(exportedInput.good() || exportedInput.eof(),
                         "CP949 AC1018 output can be reopened as bytes");
                t.expect(exportedBytes.find("1\nAC1018\n") != std::string::npos
                             && exportedBytes.find("3\nANSI_949\n")
                                    != std::string::npos
                             && exportedBytes.find(expectedLegacyText)
                                    != std::string::npos,
                         "CP949 AC1018 output declares ANSI_949 and emits the exact B0 A1 legacy payload");
            }
            dx_data first; dx_iface next;
            t.expect(next.fileImport(output.string(), &first, false), "profile output first default read succeeds");
            const auto secondPath = directory / ("second-" + std::to_string(handoffs) + "-" + std::to_string(target) + ".dxf");
            t.expect(next.fileExport(secondPath.string(), target, binary, &first, false, false,
                                     dx_dxfSourceTextPolicy::RequireValidSource, DRW::DxfTextWritePolicy::RequireLossless),
                     "profile output second strict conversion succeeds");
            dx_data second;
            const bool thirdRead = next.fileImport(secondPath.string(), &second, false);
            const auto* finalText = second.mBlock->ent.empty() ? nullptr : dynamic_cast<const DRW_Text*>(second.mBlock->ent.front());
            const auto* line = second.mBlock->ent.empty() ? nullptr : dynamic_cast<const DRW_Line*>(second.mBlock->ent.back());
            t.expect(thirdRead && finalText && finalText->text == expected && line && line->eType == DRW::LINE
                         && line->basePoint.z == 3 && line->secPoint.z == 6 && second.mBlock->ent.size() == 2,
                     "profile-admitted repeated conversion retains literal Unicode and following nonzero-Z LINE");
            exports += 2;
        }
        ++handoffs;
    }
    std::size_t undefinedIso3Routes = 0;
    for (bool binary : {false, true}) {
        const auto input = directory / (binary ? "iso8859-3-undefined-binary.dxf"
                                               : "iso8859-3-undefined-ascii.dxf");
        std::string sourceText("L");
        sourceText.push_back(static_cast<char>(0xA5)); // undefined in ISO-8859-3
        sourceText += "Z";
        writeBytes(input, makeTextObservationDrawing("ISO8859-3", false, false,
                                                      binary, sourceText));
        dx_data data;
        dx_iface adapter;
        const bool read = adapter.fileImport(input.string(), &data, false,
                                              Mapping::ExactNamed);
        const auto& observations = data.dxfReadOutcome.observations;
        const auto* text = data.mBlock->ent.empty() ? nullptr
            : dynamic_cast<const DRW_Text*>(data.mBlock->ent.front());
        t.expect(read && text && text->text == "LZ"
                     && observations.profileCount(Profile::Iso8859_3) == 1
                     && observations.issueCount(DRW::DxfTextIssue::UndefinedByteSequence) > 0
                     && observations.issueCount(DRW::DxfTextIssue::DiscardedSource) > 0
                     && dx_iface::admitDxfSourceText(data).status == Status::ObservedIssues,
                 "ASCII/Binary DXF import observes and refuses support admission for undefined ISO-8859-3 bytes");
        ++undefinedIso3Routes;
    }
    std::size_t undefinedIso6Routes = 0;
    for (bool binary : {false, true}) {
        const auto input = directory / (binary ? "iso8859-6-undefined-binary.dxf"
                                               : "iso8859-6-undefined-ascii.dxf");
        std::string sourceText("L");
        sourceText.push_back(static_cast<char>(0xA1)); // undefined in ISO-8859-6
        sourceText += "Z";
        writeBytes(input, makeTextObservationDrawing("ISO8859-6", false, false,
                                                      binary, sourceText));
        dx_data data;
        dx_iface adapter;
        const bool read = adapter.fileImport(input.string(), &data, false,
                                              Mapping::ExactNamed);
        const auto& observations = data.dxfReadOutcome.observations;
        const auto* text = data.mBlock->ent.empty() ? nullptr
            : dynamic_cast<const DRW_Text*>(data.mBlock->ent.front());
        t.expect(read && text && text->text == "LZ"
                     && observations.profileCount(Profile::Iso8859_6) == 1
                     && observations.issueCount(DRW::DxfTextIssue::UndefinedByteSequence) > 0
                     && observations.issueCount(DRW::DxfTextIssue::DiscardedSource) > 0
                     && dx_iface::admitDxfSourceText(data).status == Status::ObservedIssues,
                 "ASCII/Binary DXF import observes and refuses support admission for undefined ISO-8859-6 bytes");
        ++undefinedIso6Routes;
    }
    std::size_t undefinedIso7Routes = 0;
    for (bool binary : {false, true}) {
        const auto input = directory / (binary ? "iso8859-7-undefined-binary.dxf"
                                               : "iso8859-7-undefined-ascii.dxf");
        std::string sourceText("L");
        sourceText.push_back(static_cast<char>(0xAE)); // undefined in ISO-8859-7
        sourceText += "Z";
        writeBytes(input, makeTextObservationDrawing("ISO8859-7", false, false,
                                                      binary, sourceText));
        dx_data data;
        dx_iface adapter;
        const bool read = adapter.fileImport(input.string(), &data, false,
                                              Mapping::ExactNamed);
        const auto& observations = data.dxfReadOutcome.observations;
        const auto* text = data.mBlock->ent.empty() ? nullptr
            : dynamic_cast<const DRW_Text*>(data.mBlock->ent.front());
        t.expect(read && text && text->text == "LZ"
                     && observations.profileCount(Profile::Iso8859_7) == 1
                     && observations.issueCount(DRW::DxfTextIssue::UndefinedByteSequence) > 0
                     && observations.issueCount(DRW::DxfTextIssue::DiscardedSource) > 0
                     && dx_iface::admitDxfSourceText(data).status == Status::ObservedIssues,
                 "ASCII/Binary DXF import observes and refuses support admission for undefined ISO-8859-7 bytes");
        ++undefinedIso7Routes;
    }
    std::size_t undefinedIso8Routes = 0;
    for (bool binary : {false, true}) {
        const auto input = directory / (binary ? "iso8859-8-undefined-binary.dxf"
                                               : "iso8859-8-undefined-ascii.dxf");
        std::string sourceText("L");
        sourceText.push_back(static_cast<char>(0xA1)); // undefined in ISO-8859-8
        sourceText += "Z";
        writeBytes(input, makeTextObservationDrawing("ISO8859-8", false, false,
                                                      binary, sourceText));
        dx_data data;
        dx_iface adapter;
        const bool read = adapter.fileImport(input.string(), &data, false,
                                              Mapping::ExactNamed);
        const auto& observations = data.dxfReadOutcome.observations;
        const auto* text = data.mBlock->ent.empty() ? nullptr
            : dynamic_cast<const DRW_Text*>(data.mBlock->ent.front());
        t.expect(read && text && text->text == "LZ"
                     && observations.profileCount(Profile::Iso8859_8) == 1
                     && observations.issueCount(DRW::DxfTextIssue::UndefinedByteSequence) > 0
                     && observations.issueCount(DRW::DxfTextIssue::DiscardedSource) > 0
                     && dx_iface::admitDxfSourceText(data).status == Status::ObservedIssues,
                 "ASCII/Binary DXF import observes and refuses support admission for undefined ISO-8859-8 bytes");
        ++undefinedIso8Routes;
    }
    std::size_t undefinedIso11Routes = 0;
    for (bool binary : {false, true}) {
        const auto input = directory / (binary ? "iso8859-11-undefined-binary.dxf"
                                               : "iso8859-11-undefined-ascii.dxf");
        std::string sourceText("L");
        sourceText.push_back(static_cast<char>(0xDB)); // undefined in ISO-8859-11
        sourceText += "Z";
        writeBytes(input, makeTextObservationDrawing("ISO8859-11", false, false,
                                                      binary, sourceText));
        dx_data data;
        dx_iface adapter;
        const bool read = adapter.fileImport(input.string(), &data, false,
                                              Mapping::ExactNamed);
        const auto& observations = data.dxfReadOutcome.observations;
        const auto* text = data.mBlock->ent.empty() ? nullptr
            : dynamic_cast<const DRW_Text*>(data.mBlock->ent.front());
        t.expect(read && text && text->text == "LZ"
                     && observations.profileCount(Profile::Iso8859_11) == 1
                     && observations.issueCount(DRW::DxfTextIssue::UndefinedByteSequence) > 0
                     && observations.issueCount(DRW::DxfTextIssue::DiscardedSource) > 0
                     && dx_iface::admitDxfSourceText(data).status == Status::ObservedIssues,
                 "ASCII/Binary DXF import observes and refuses source admission for undefined ISO-8859-11 bytes");
        ++undefinedIso11Routes;
    }
    // An ASCII legacy field and later modern non-ASCII text have intersecting
    // old counters, but neither actually consumed a noncanonical byte profile.
    for (bool isoBytes : {false,true}) {
        auto bytes = makeTextObservationDrawing("ISO8859-4", false, false, false, isoBytes ? "L\xA1Z" : "plain");
        bytes.resize(bytes.rfind("0\nEOF\n"));
        bytes += "0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1021\n9\n$DWGCODEPAGE\n3\nANSI_1252\n0\nENDSEC\n"
                 "0\nSECTION\n2\nENTITIES\n0\nTEXT\n5\nC\n8\n0\n10\n0\n20\n0\n40\n1\n1\n\xC3\xA9\n0\nENDSEC\n0\nEOF\n";
        const auto path = directory / (isoBytes ? "mixed-iso.dxf" : "mixed-independent.dxf"); writeBytes(path, bytes);
        dx_data data; dx_iface adapter;
        t.expect(adapter.fileImport(path.string(), &data, false, Mapping::ExactNamed)
                     && data.dxfReadOutcome.observations.exactNamedDecodeCalls > 0
                     && data.dxfReadOutcome.observations.nonAsciiDecodeCalls > 0
                     && partitioned(data.dxfReadOutcome.observations)
                     && data.dxfReadOutcome.observations.profileCount(Profile::ModernUtf8) > 0
                     && (data.dxfReadOutcome.observations.profileCount(Profile::Iso8859_4) > 0) == isoBytes
                     && !data.dxfReadOutcome.observations.hasIssues()
                     && dx_iface::admitDxfSourceText(data).status == (isoBytes ? Status::UnsupportedProfile : Status::Admitted),
                 "actual cumulative profiles distinguish decoder-independent mixed versions from earlier ISO bytes despite identical old counter intersections");
    }
    {
        auto bytes = makeTextObservationDrawing("ANSI_1251", false, false, false, "\xC0");
        bytes.resize(bytes.rfind("0\nEOF\n"));
        bytes += "0\nSECTION\n2\nHEADER\n9\n$DWGCODEPAGE\n3\nANSI_1252\n0\nENDSEC\n"
                 "0\nSECTION\n2\nENTITIES\n0\nTEXT\n5\nC\n8\n0\n10\n0\n20\n0\n40\n1\n1\n\xE9\n0\nENDSEC\n0\nEOF\n";
        const auto path = directory / "two-windows.dxf"; writeBytes(path, bytes);
        dx_data data; dx_iface adapter;
        t.expect(adapter.fileImport(path.string(), &data, false, Mapping::ExactNamed)
                     && data.dxfReadOutcome.observations.profileCount(Profile::Windows1251) == 1
                     && data.dxfReadOutcome.observations.profileCount(Profile::Windows1252) == 1
                     && partitioned(data.dxfReadOutcome.observations)
                     && dx_iface::admitDxfSourceText(data).status == Status::Admitted,
                 "two consumed Windows profiles remain distinct beyond final HEADER normalization");
        const auto saved = data.dxfReadOutcome.observations;
        for (int mode = 0; mode < 6; ++mode) {
            auto& value = data.dxfReadOutcome.observations; value = saved;
            if (mode == 0) value.sourceProfileCalls = {};
            if (mode == 1) value.sourceProfileMask |= std::uint64_t{1} << 63;
            if (mode == 2) {
                ++value.sourceProfileCalls[0]; ++value.semanticDecodeCalls;
                value.sourceProfileMask |= DRW::dxfTextSourceProfileMask(Profile::NoProfile);
            }
            if (mode == 3) {
                value.sourceProfileCalls[0] = value.sourceProfileCalls[1] = (std::numeric_limits<std::uint64_t>::max)();
            }
            if (mode == 4) value.counterSaturated = true;
            if (mode == 5) ++value.semanticDecodeCalls;
            t.expect(dx_iface::admitDxfSourceText(data).status == Status::EvidenceUnavailable,
                     "missing/unknown/inconsistent/overflowed/saturated profile facts cannot authorize exact source conversion");
        }
        data.dxfReadOutcome.observations = saved;
        t.expect(dx_iface::admitDxfSourceText(data).admitted(), "restored source facts admit without changing the original owned snapshot");
    }
    {
        const auto path = directory / "latin1-unqualified-profile.dxf";
        writeBytes(path, makeTextObservationDrawing("LATIN1", false, false, false, "L\x80Z"));
        dx_data data;
        dx_iface adapter;
        const bool imported = adapter.fileImport(path.string(), &data, false, Mapping::ExactNamed);
        const auto* text = data.mBlock->ent.empty() ? nullptr
            : dynamic_cast<const DRW_Text*>(data.mBlock->ent.front());
        t.expect(imported && text && text->text == "L\xC2\x80Z"
                     && data.dxfReadOutcome.observations.profileCount(Profile::Iso8859_1) == 1
                     && dx_iface::admitDxfSourceText(data).status == Status::UnsupportedProfile,
                 "exact Latin-1 is decoded and identified but remains outside strict producer-profile admission");
    }
    for (const auto mapping : {Mapping::Compatible, Mapping::ExactNamed}) {
        auto bytes = makeTextObservationDrawing("ISO8859-4", false, false, false, "\xA2");
        dx_data data; PreparedDxInterface sink; sink.attach(&data); dxfRW reader("profile-memory.dxf");
        reader.setDxfCodePagePolicy(mapping);
        t.expect(reader.readAscii(&sink, false, bytes)
                     && reader.getTextReadObservations().profileCount(mapping == Mapping::ExactNamed ? Profile::Iso8859_4 : Profile::Windows1257) == 1,
                 "effective consumed decoder is distinct from the requested ISO family under Compatible versus ExactNamed");
    }
    for (const char* page : {"UTF8","UTF16"}) for (bool modern : {false,true}) {
        const bool utf16 = std::string_view(page) == "UTF16";
        auto bytes = makeTextObservationDrawing(page, modern, false, false, !modern && utf16 ? "AB" : "\xC3\xA9");
        dx_data data; PreparedDxInterface sink; sink.attach(&data); dxfRW reader("unicode-profile-memory.dxf");
        reader.setDxfCodePagePolicy(Mapping::ExactNamed);
        const bool read = reader.readAscii(&sink, false, bytes);
        const auto observations = reader.getTextReadObservations();
        const auto expected = modern ? Profile::ModernUtf8 : utf16 ? Profile::LegacyUtf16 : Profile::NonModernUtf8;
        t.expect(read && partitioned(observations) && observations.profileCount(expected) > 0
                     && observations.hasIssues() == !modern,
                 "actual reader records distinct legacy Unicode and modern inert-metadata profiles without admitting unsupported interpretation");
    }
    {
        DRW_TextCodec codec; codec.setVersion(DRW::AC1015, true);
        codec.setCodePage("ANSI_1252", true); codec.setDxfTextEncoding(DRW::DxfTextEncoding::LegacyLibreCadUtf8);
        (void)codec.toUtf8("\xC3\xA9");
        t.expect(codec.dxfSourceProfile("\xC3\xA9") == Profile::RecoveredLegacyUtf8, "explicit recovery has its own effective profile");
        codec.setDxfTextEncoding(DRW::DxfTextEncoding::Declared); codec.setCodePage("UTF16", true);
        (void)codec.toUtf8("AB");
        t.expect(codec.dxfSourceProfile("AB") == Profile::LegacyUtf16, "TU-like ASCII bytes are not decoder-independent ASCII DXF");
        codec.setCodePage("UTF8", true); (void)codec.toUtf8("\xC3\xA9");
        t.expect(codec.dxfSourceProfile("\xC3\xA9") == Profile::NonModernUtf8, "legacy UTF8 pass-through is not a modern source profile");
        DRW_DxfTextReadObservations value;
        value.sourceProfileCalls[static_cast<std::size_t>(Profile::Iso8859_4)] = (std::numeric_limits<std::uint64_t>::max)();
        DRW_TextDecodeObservation one; one.sourceProfile = Profile::Iso8859_4;
        dxfReader::appendTextReadObservation(value, one, 1, 0, false);
        t.expect(value.counterSaturated && value.profileCount(Profile::Iso8859_4) == (std::numeric_limits<std::uint64_t>::max)()
                     && value.sourceProfileMask == DRW::dxfTextSourceProfileMask(Profile::Iso8859_4),
                 "profile counters saturate without rollover and keep their observed identity bit");
        DRW_DxfTextReadObservations invalid; one.sourceProfile = static_cast<Profile>(255);
        dxfReader::appendTextReadObservation(invalid, one, 1, 0, false);
        t.expect(invalid.profileCount(Profile::NoProfile) == 1 && invalid.profileCount(static_cast<Profile>(255)) == 0
                     && DRW::dxfTextSourceProfileMask(static_cast<Profile>(255)) == 0,
                 "invalid profile enums record missing evidence without unsafe mask shifts or indexing");
    }
    std::cout << "Consumed DXF profiles: canonical handoffs=" << handoffs
              << ", undefined ISO-8859-3 routes=" << undefinedIso3Routes
              << ", undefined ISO-8859-6 routes=" << undefinedIso6Routes
              << ", undefined ISO-8859-7 routes=" << undefinedIso7Routes
              << ", undefined ISO-8859-8 routes=" << undefinedIso8Routes
              << ", undefined ISO-8859-11 routes=" << undefinedIso11Routes
              << ", strict two-pass exports=" << exports
              << "; mixed/cumulative/effective/recovery/Unicode controls, 6 evidence mutations and saturation/invalid-enum checks pass\n";
    if (t.failures == 0) std::filesystem::remove_all(directory);
    else std::cerr << "Consumed profile evidence: " << directory << '\n';
}

void testExactCodePageImport(TestContext& t) {
    using Mapping = DRW::DxfCodePagePolicy;
    using Source = dx_dxfSourceTextPolicy;
    using Status = dx_dxfSourceTextStatus;
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-exact-import-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    const auto writeBytes = [](const std::filesystem::path& path, const std::string& bytes) {
        std::ofstream file(path, std::ios::binary); file.write(bytes.data(), bytes.size());
        if (!file) throw std::runtime_error("authored exact import could not be written");
    };
    const auto readBytes = [](const std::filesystem::path& path) {
        std::ifstream file(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(file), {});
    };
    const auto hasTextAndLine = [](const dx_data& data, const std::string& expected) {
        if (data.mBlock->ent.size() != 2) return false;
        const auto* text = dynamic_cast<const DRW_Text*>(data.mBlock->ent.front());
        const auto* line = dynamic_cast<const DRW_Line*>(data.mBlock->ent.back());
        return text && text->text == expected && line && line->eType == DRW::LINE
            && line->basePoint.x == 1 && line->basePoint.y == 2 && line->basePoint.z == 3
            && line->secPoint.x == 4 && line->secPoint.y == 5 && line->secPoint.z == 6;
    };
    const std::string expected = "L\xC4\x84Z";
    std::size_t handoffs = 0, publications = 0, refusals = 0;
    for (const char* page : {"ISO8859-4", "ISO-8859-4"})
        for (bool modern : {false,true}) for (bool binary : {false,true}) {
        const auto input = directory / ("source-" + std::to_string(handoffs) + ".dxf");
        writeBytes(input, makeTextObservationDrawing(page, modern, false, binary,
                                                    modern ? expected : "L\xA1Z"));
        dx_data data; dx_iface adapter;
        t.expect(adapter.fileImport(input.string(), &data, false, Mapping::ExactNamed)
                     && hasTextAndLine(data, expected)
                     && data.dxfReadOutcome.requestedCodePagePolicy == Mapping::ExactNamed
                     && data.dxfReadOutcome.observations.codePagePolicy == Mapping::ExactNamed
                     && !data.dxfReadOutcome.observations.hasIssues()
                     && (data.dxfReadOutcome.observations.exactNamedDecodeCalls > 0) == !modern
                     && data.headerC.dxfCodePageDeclaration() == page,
                 "explicit adapter input policy reaches typed ASCII/binary legacy/modern text and owned outcomes");
        const auto retained = data.dxfReadOutcome;
        t.expect(dx_iface::admitDxfSourceText(data).status == (modern ? Status::Admitted : Status::UnsupportedProfile),
                 "exact mapping does not itself admit a new high-byte conversion profile; modern metadata stays inert");
        const auto output = directory / ("first-" + std::to_string(handoffs) + ".dxf");
        const auto target = modern ? DRW::AC1018 : DRW::AC1021;
        if (!modern) {
            for (bool sentinel : {false,true}) {
                if (sentinel) writeBytes(output, "preserved-destination");
                t.expect(!adapter.fileExport(output.string(), target, binary, &data, false, false,
                                             Source::RequireValidSource, DRW::DxfTextWritePolicy::RequireLossless)
                             && adapter.sourceTextAdmission().status == Status::UnsupportedProfile
                             && (sentinel ? readBytes(output) == "preserved-destination" : !std::filesystem::exists(output))
                             && sameTextObservations(retained.observations, data.dxfReadOutcome.observations),
                         "unqualified exact high-byte source refuses before destination creation/replacement or evidence mutation");
                ++refusals;
            }
        }
        // Explicit Compatible source admission is a caller choice, not producer
        // qualification. Target transport still checks every required scalar.
        t.expect(adapter.fileExport(output.string(), target, binary, &data, false, false,
                                   modern ? Source::RequireValidSource : Source::Compatible,
                                   DRW::DxfTextWritePolicy::RequireLossless),
                 "explicit compatible import export retains exact scalars under separate lossless target policy");
        dx_data first; dx_iface next;
        t.expect(next.fileImport(output.string(), &first, false) && hasTextAndLine(first, expected)
                     && first.dxfReadOutcome.requestedCodePagePolicy == Mapping::Compatible
                     && first.dxfReadOutcome.observations.codePagePolicy == Mapping::Compatible,
                 "first actual output reimports under unchanged default input mapping");
        const auto secondOutput = directory / ("second-" + std::to_string(handoffs) + ".dxf");
        t.expect(next.fileExport(secondOutput.string(), target, binary, &first, false, false,
                                Source::RequireValidSource, DRW::DxfTextWritePolicy::RequireLossless),
                 "normalized actual output composes with existing source and target admission");
        dx_data second;
        t.expect(next.fileImport(secondOutput.string(), &second, false) && hasTextAndLine(second, expected)
                     && data.dxfReadOutcome.requestedCodePagePolicy == Mapping::ExactNamed
                     && sameTextObservations(retained.observations, data.dxfReadOutcome.observations),
                 "two exports and third read preserve semantic text/LINE and the original owned evidence");
        publications += 2;
        ++handoffs;
    }
    const auto good = directory / "retry.dxf";
    writeBytes(good, makeTextObservationDrawing("ISO8859-4", false, false, false, "L\xA1Z"));
    dx_data reused; dx_iface adapter;
    t.expect(adapter.fileImport(good.string(), &reused, false, Mapping::ExactNamed), "reuse starts with exact input");
    const auto saved = reused.dxfReadOutcome;
    auto invalid = makeTextObservationDrawing("ISO8859-4", false, false, false, "L\xA1Z");
    const auto handleAt = invalid.find("\nB\n");
    t.expect(handleAt != std::string::npos, "authored late failure has its distinct LINE handle");
    if (handleAt != std::string::npos) invalid.replace(handleAt + 1, 1, "INVALID_HANDLE");
    const auto invalidPath = directory / "late-invalid.dxf"; writeBytes(invalidPath, invalid);
    t.expect(!adapter.fileImport(invalidPath.string(), &reused, false, Mapping::ExactNamed)
                 && reused.dxfReadOutcome.readReturned && !reused.dxfReadOutcome.operationSucceeded
                 && reused.dxfReadOutcome.requestedCodePagePolicy == Mapping::ExactNamed
                 && reused.dxfReadOutcome.observations.codePagePolicy == Mapping::ExactNamed
                 && reused.dxfReadOutcome.observations.exactNamedDecodeCalls > 0
                 && dx_iface::admitDxfSourceText(reused).status == Status::ImportNotSuccessful,
             "late returned failure retains applied exact prefix without authorizing conversion");
    t.expect(!adapter.fileImport((directory / "missing.dxf").string(), &reused, false, Mapping::ExactNamed)
                 && reused.dxfReadOutcome.readReturned && reused.dxfReadOutcome.error == DRW::BAD_OPEN
                 && reused.dxfReadOutcome.requestedCodePagePolicy == Mapping::ExactNamed
                 && reused.dxfReadOutcome.observations.codePagePolicy == Mapping::ExactNamed
                 && reused.dxfReadOutcome.observations.semanticDecodeCalls == 0,
             "early returned failure owns requested/applied policy without stale events");
    t.expect(adapter.fileImport(good.string(), &reused, false)
                 && reused.dxfReadOutcome.requestedCodePagePolicy == Mapping::Compatible
                 && reused.dxfReadOutcome.observations.codePagePolicy == Mapping::Compatible
                 && reused.dxfReadOutcome.observations.hasIssues() && !saved.observations.hasIssues(),
             "defaulted next import resets policies and observations without changing the copied exact outcome");
    t.expect(!adapter.fileImport((directory / "invalid.ext").string(), &reused, false, Mapping::ExactNamed)
                 && !reused.dxfReadOutcome.attempted
                 && reused.dxfReadOutcome.requestedCodePagePolicy == Mapping::Compatible,
             "non-DXF attempt resets the DXF-specific request rather than fabricating an applied policy");
    for (bool asciiOnly : {false,true}) {
        const auto path = directory / (asciiOnly ? "ascii-independent.dxf" : "unavailable.dxf");
        writeBytes(path, makeTextObservationDrawing("EUC-JP", false, false, false,
                                                   asciiOnly ? "L\\U+0104Z" : "L\xA1Z"));
        dx_data data;
        t.expect(adapter.fileImport(path.string(), &data, false, Mapping::ExactNamed)
                     && dx_iface::admitDxfSourceText(data).status == (asciiOnly ? Status::Admitted : Status::ObservedIssues)
                     && data.dxfReadOutcome.observations.profileCount(DRW::DxfTextSourceProfile::UnavailableNamed) == (asciiOnly ? 0u : 1u)
                     && data.dxfReadOutcome.observations.unavailableNamedDecodeCalls == (asciiOnly ? 0u : 1u),
                 "unknown-family ASCII escapes remain independent while unavailable high bytes are observed/refused");
    }
    {
        auto bytes = makeTextObservationDrawing("ISO8859-4", false, false, false, "L\xA1Z");
        bytes.resize(bytes.rfind("0\nEOF\n"));
        bytes += "0\nSECTION\n2\nHEADER\n9\n$DWGCODEPAGE\n3\nANSI_1252\n0\nENDSEC\n0\nEOF\n";
        const auto path = directory / "later-canonical.dxf"; writeBytes(path, bytes);
        dx_data data;
        t.expect(adapter.fileImport(path.string(), &data, false, Mapping::ExactNamed)
                     && data.headerC.dxfCodePageDeclaration() == "ANSI_1252"
                     && !data.dxfReadOutcome.observations.hasIssues()
                     && dx_iface::admitDxfSourceText(data).status == Status::UnsupportedProfile,
                 "later canonical metadata cannot qualify earlier exact high-byte consumption");
        data.dxfReadOutcome.requestedCodePagePolicy = Mapping::Compatible;
        t.expect(dx_iface::admitDxfSourceText(data).status == Status::EvidenceUnavailable,
                 "request/applied policy disagreement is unavailable evidence, not a clean source");
        data.dxfReadOutcome.requestedCodePagePolicy = data.dxfReadOutcome.observations.codePagePolicy = static_cast<Mapping>(255);
        t.expect(dx_iface::admitDxfSourceText(data).status == Status::EvidenceUnavailable,
                 "unknown input-policy enum values cannot authorize complete conversion");
    }
    class ChangingImport final : public dx_iface {
    public:
        explicit ChangingImport(Mapping& request) : request(request) {}
        void addText(const DRW_Text& value) override {
            request = Mapping::Compatible;
            if (throwNow) { failImportObservationAllocations = failCopy; throw 79; }
            dx_iface::addText(value);
        }
        Mapping& request;
        bool throwNow{false}, failCopy{false};
    };
    for (int mode = 0; mode < 3; ++mode) {
        Mapping request = Mapping::ExactNamed; ChangingImport changing(request);
        changing.throwNow = mode != 0; changing.failCopy = mode == 2;
        dx_data data; bool returned = false, caught = false;
        try { returned = changing.fileImport(good.string(), &data, false, request); }
        catch (int sentinel) { failImportObservationAllocations = false; caught = sentinel == 79; }
        catch (...) { failImportObservationAllocations = false; }
        failImportObservationAllocations = false;
        const auto& outcome = data.dxfReadOutcome;
        t.expect((mode == 0 ? returned && !caught : !returned && caught)
                     && request == Mapping::Compatible && outcome.readThrew == (mode != 0)
                     && outcome.requestedCodePagePolicy == Mapping::ExactNamed
                     && outcome.observations.codePagePolicy == Mapping::ExactNamed
                     && outcome.observations.exactNamedDecodeCalls > 0 && !outcome.observations.hasIssues()
                     && outcome.diagnosticAvailable == (mode != 2)
                     && outcome.diagnosticCaptureFailed == (mode == 2),
                 "copied requested policy and applied observations survive callback mutation/throw/diagnostic-copy failure");
        changing.throwNow = changing.failCopy = false;
        dx_data retry;
        t.expect(changing.fileImport(good.string(), &retry, false)
                     && retry.dxfReadOutcome.requestedCodePagePolicy == Mapping::Compatible
                     && retry.dxfReadOutcome.observations.codePagePolicy == Mapping::Compatible,
                 "same adapter corrected retry uses the default rather than a stale explicit policy");
    }
    std::cout << "Exact adapter input: handoffs=" << handoffs << ", two-pass exports=" << publications
              << ", atomic strict-source refusals=" << refusals
              << "; reuse/unavailable/ASCII/cumulative/profile-evidence and 3 callback/retry controls pass\n";
    if (t.failures == 0) std::filesystem::remove_all(directory);
    else std::cerr << "Exact import evidence: " << directory << '\n';
}

void testDxfSourceTextAdmission(TestContext& t) {
    using Status = dx_dxfSourceTextStatus;
    using Policy = dx_dxfSourceTextPolicy;
    using Issue = DRW_DxfTextIssue;
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-source-text-admission-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    const auto writeBytes = [](const std::filesystem::path& path, const std::string& bytes) {
        std::ofstream file(path, std::ios::binary);
        file.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        if (!file) throw std::runtime_error("source-admission scratch write failed");
    };
    struct Row { const char* page; bool modern; std::string bytes; bool admitted; };
    const Row rows[] = {
        {"ANSI_1252", false, "L\xE9R", true},
        {"CP1252", false, "L\xE9R", true},
        {"ISO8859-8", false, "L\xE0R", false},
        {"ISO8859-9", false, "L\xD0R", false},
        {"ISO8859-10", false, "L\xA1R", false},
        {"ISO8859-7", false, "L\xA1R", false},
        {"ISO8859-6", false, "L\xACR", false},
        {"ISO8859-6", false, "L\xA1R", false},
        {"ISO8859-4", false, "L\xA1R", false},
        {"ISO8859-4", false, "plain", true},
        {"local-unknown", false, "\\U+0410", true},
        {"local-unknown", false, "plain", true},
        {"local-unknown", false, "\xE9", false},
        {"CP866", false, "\x80", false},
        {"ANSI_1252", false, "\x81", false},
        {"ANSI_932", false, "\x82", false},
        {"ANSI_1252", false, "\\M+48861", false},
        {"ANSI_1252", false, "\\M+193FA", true},
        {"UTF8", false, "plain", false},
        {"UTF16", false, "AB", false},
        {nullptr, false, "\xE9", false},
        {nullptr, false, "plain", true},
        {"local-unknown", true, "\xC3\xA9", true},
        {"UTF16", true, "\xC3\xA9", true},
        {"ANSI_1252", true, "\x80", false},
        {"ANSI_1252", true, "\\U+D800", false}
    };
    std::size_t cases = 0;
    dx_iface exporter;
    for (const auto& row : rows) for (bool binary : {false, true}) {
        const auto path = directory / (std::to_string(cases) + ".dxf");
        const auto output = directory / ("output-" + std::to_string(cases) + ".dxf");
        writeBytes(path, makeTextObservationDrawing(row.page, row.modern, false, binary, row.bytes));
        dx_data data;
        dx_iface importer;
        t.expect(importer.fileImport(path.string(), &data, false),
                 "source admission does not change tolerant import success");
        const auto result = dx_iface::admitDxfSourceText(data);
        t.expect(result.admitted() == row.admitted,
                 "explicit source policy separates corruption/ambiguous decoding from inert metadata and valid transports");
        const auto observations = data.dxfReadOutcome.observations;
        const auto legacyError = data.dxfReadOutcome.error;
        const std::string sentinel = "source-admission destination sentinel\n";
        if (!row.admitted) writeBytes(output, sentinel);
        const bool written = exporter.fileExport(output.string(), DRW::AC1021, binary,
            &data, false, true, Policy::RequireValidSource);
        t.expect(written == row.admitted
                     && exporter.sourceTextAdmission().status == result.status
                     && sameTextObservations(observations, data.dxfReadOutcome.observations)
                     && data.dxfReadOutcome.error == legacyError,
                 "source gate runs independently of owners and cannot mutate the import evidence");
        if (!row.admitted) {
            std::ifstream existing(output, std::ios::binary);
            const std::string actual((std::istreambuf_iterator<char>(existing)), {});
            t.expect(actual == sentinel, "source refusal leaves an existing destination unchanged");
            const auto absent = directory / ("absent-" + std::to_string(cases) + ".dxf");
            t.expect(!exporter.fileExport(absent.string(), DRW::AC1021, binary,
                         &data, false, false, Policy::RequireValidSource)
                         && !std::filesystem::exists(absent),
                     "source refusal precedes any output creation even with tolerant owner policy");
            t.expect(exporter.fileExport(output.string(), DRW::AC1021, binary, &data, false)
                         && exporter.sourceTextAdmission().status == Status::NotChecked,
                     "ordinary adapter export remains compatible and does not silently opt into source refusal");
        }
        ++cases;
    }
    // The source-profile observation is reader-only; DWG's valid TU codec is
    // not reclassified. This even-length ASCII pair decodes to a valid scalar
    // under UTF16, so malformed-byte checks alone cannot close the profile gate.
    std::istringstream input("1\nAB\n");
    dxfReaderAscii profile(&input);
    profile.setVersion("AC1015", true);
    profile.setCodePage("UTF16");
    int code = 0;
    t.expect(profile.readRec(&code) && profile.getUtf8String() == "\xE4\x89\x81"
                 && profile.textReadObservations().issueMask == DRW::dxfTextIssueMask(Issue::UnsupportedEncodingProfile),
             "valid-scalar UTF16 interpretation still records its unsupported DXF source profile");
    dx_data synthetic;
    synthetic.sourceFormat = dx_source_format::DXF;
    auto& outcome = synthetic.dxfReadOutcome;
    outcome.attempted = outcome.readReturned = outcome.operationSucceeded = true;
    outcome.diagnosticAvailable = true;
    outcome.diagnostic.operation = DRW::OperationKind::Read;
    const auto cleanOutcome = outcome;
    t.expect(dx_iface::admitDxfSourceText(synthetic).admitted(),
             "unknown revision with decoder-independent text does not invent a required charset");
    outcome.diagnosticCaptureFailed = true;
    t.expect(dx_iface::admitDxfSourceText(synthetic).status == Status::EvidenceUnavailable,
             "failed diagnostic capture cannot count as clean evidence");
    outcome.diagnosticCaptureFailed = false;
    outcome.readThrew = true;
    t.expect(dx_iface::admitDxfSourceText(synthetic).status == Status::ImportNotSuccessful,
             "a throwing read is not admitted even if a caller leaves success set");
    outcome.readThrew = false;
    outcome.observations.counterSaturated = true;
    t.expect(dx_iface::admitDxfSourceText(synthetic).status == Status::EvidenceUnavailable,
             "saturated evidence does not masquerade as an empty clean mask");
    outcome.observations.counterSaturated = false;
    outcome.observations.nonAsciiDecodeCalls = 1;
    t.expect(dx_iface::admitDxfSourceText(synthetic).status == Status::UnknownRevision,
             "raw nonASCII requires an accepted revision even without an observed decoder error");
    synthetic.sourceFormat = dx_source_format::DWG;
    t.expect(dx_iface::admitDxfSourceText(synthetic).status == Status::NotApplicable,
             "the DXF source gate does not invent DWG text qualification");
    synthetic.sourceFormat = dx_source_format::Unknown;
    t.expect(dx_iface::admitDxfSourceText(synthetic).status == Status::EvidenceUnavailable,
             "an explicitly strict source export requires an actual import route");
    synthetic.sourceFormat = dx_source_format::DXF;
    for (int fault = 0; fault < 10; ++fault) {
        outcome = cleanOutcome;
        Status expected = Status::ImportNotSuccessful;
        switch (fault) {
        case 0: outcome.attempted = false; break;
        case 1: outcome.readReturned = false; break;
        case 2: outcome.operationSucceeded = false; break;
        case 3: outcome.error = DRW::BAD_READ_ENTITIES; break;
        case 4: outcome.diagnosticAvailable = false; expected = Status::EvidenceUnavailable; break;
        case 5: outcome.diagnostic.operation = DRW::OperationKind::Write; expected = Status::EvidenceUnavailable; break;
        case 6: outcome.diagnostic.cause = DRW::OperationCause::ReadFailure; break;
        case 7:
            outcome.headerDelivered = outcome.hasCodePageDeclaration = true;
            expected = Status::EvidenceUnavailable;
            break;
        case 8:
            outcome.diagnostic.secondary.resize(DRW_OperationDiagnostic::MaxSecondaryEntries + 1);
            expected = Status::EvidenceUnavailable;
            break;
        case 9: outcome.observations.issueMask = 0x80000000u; expected = Status::ObservedIssues; break;
        }
        t.expect(dx_iface::admitDxfSourceText(synthetic).status == expected,
                 "contradictory, unavailable or future unknown evidence cannot be silently admitted");
    }
    std::cout << "DXF source admission: handoffs=" << cases << ", profile controls=1, envelope controls=17\n";
    if (t.failures == 0) std::filesystem::remove_all(directory);
    else std::cerr << "Source admission evidence: " << directory << '\n';
}

void testDxfAsciiCaretEscapes(TestContext& t) {
    struct Row { std::string semantic; std::string wire; };
    const Row rows[] = {
        {std::string(1, '\x07'), "^G"},
        {std::string(1, '\x1B'), "^["},
        {std::string(1, '\x09'), "^I"},
        {std::string(1, '\x1F'), "^_"},
        {std::string(1, '\x7F'), "^?"},
        {"^", "^ "},
        {"^^G", "^ ^ G"}
    };
    std::size_t cases = 0;
    for (const auto& row : rows) {
        for (const auto version : {DRW::AC1018, DRW::AC1021}) {
            for (const int code : {1, 1000}) {
                for (bool strict : {false, true}) {
                    std::ostringstream output;
                    dxfWriterAscii writer(&output);
                    writer.setVersion(version == DRW::AC1021 ? "AC1021" : "AC1018", true);
                    writer.setCodePage("ANSI_1252");
                    if (strict)
                        writer.setTextWritePolicy(DRW::DxfTextWritePolicy::RequireLossless);
                    const bool exported = writer.writeUtf8String(code, row.semantic)
                        && writer.writeString(0, "LINE")
                        && writer.writeUtf8String(1, "following")
                        && writer.writeString(0, "EOF");
                    t.expect(exported && !writer.hasWriteError(),
                             "ASCII caret/control text exports with strict and compatible policies");

                    std::istringstream input(output.str());
                    dxfReaderAscii reader(&input);
                    reader.setVersion(version == DRW::AC1021 ? "AC1021" : "AC1018", true);
                    reader.setCodePage("ANSI_1252");
                    int readCode = 0;
                    t.expect(reader.readRec(&readCode) && readCode == code
                                 && reader.getRawValue() == row.wire
                                 && reader.getUtf8String() == row.semantic,
                             "ASCII caret spelling decodes once while physical spelling remains available");
                    t.expect(reader.readRec(&readCode) && readCode == 0
                                 && reader.getRawValue() == "LINE"
                                 && reader.readRec(&readCode) && readCode == 1
                                 && reader.getUtf8String() == "following"
                                 && reader.readRec(&readCode) && readCode == 0
                                 && reader.getRawValue() == "EOF"
                                 && !reader.readRec(&readCode),
                             "caret expansion preserves following-record framing and EOF");
                    ++cases;
                }
            }
        }
    }

    {
        std::ostringstream output;
        dxfWriterAscii writer(&output);
        writer.setVersion("AC1021", true);
        writer.setCodePage("ANSI_1252");
        t.expect(writer.writeRawString(1, "^G") && output.str().find("\n^G\n") != std::string::npos,
                 "opaque raw-string output preserves its exact caret spelling");
        std::istringstream input(output.str());
        dxfReaderAscii reader(&input);
        reader.setVersion("AC1021", true);
        reader.setCodePage("ANSI_1252");
        int code = 0;
        t.expect(reader.readRec(&code) && code == 1
                     && reader.getString() == "^G"
                     && reader.getRawValue() == "^G",
                 "raw value access remains byte-preserving beside the semantic caret getter");
    }
    {
        std::ostringstream output;
        dxfWriterAscii writer(&output);
        writer.setVersion("AC1021", true);
        const std::string symbol = std::string("sym^") + '\x07';
        t.expect(writer.writeUtf8Caps(2, symbol)
                     && output.str().find("SYM^ ^G") != std::string::npos,
                 "uppercase symbol fast path still caret-encodes semantic controls");
    }
    {
        std::ostringstream output;
        dxfWriterAscii writer(&output);
        writer.setVersion("AC1021", true);
        writer.setCodePage("ANSI_1252");
        t.expect(!writer.writeUtf8String(1, std::string("before\0after", 12u))
                     && output.str().empty(),
                 "NUL remains refused before ASCII group bytes are published");
    }
    {
        std::ostringstream output;
        dxfWriterAscii writer(&output);
        writer.setVersion("AC1018", true);
        writer.setCodePage("ANSI_932");
        const std::string source{"\x81^G", 3u};
        const std::string expected = [&]() {
            DRW_TextCodec codec;
            codec.setVersion("AC1018", true);
            codec.setCodePage("ANSI_932", true);
            return codec.toUtf8(source);
        }();
        t.expect(writer.writeRawString(1, source),
                 "generated DBCS trail-byte caret control is admitted as raw input");
        std::istringstream input(output.str());
        dxfReaderAscii reader(&input);
        reader.setVersion("AC1018", true);
        reader.setCodePage("ANSI_932");
        int code = 0;
        t.expect(reader.readRec(&code) && code == 1
                     && reader.getUtf8String() == expected,
                 "DBCS trail byte equal to caret is not decoded as an ASCII caret escape");
    }
    {
        std::ostringstream output(std::ios::binary);
        dxfWriterBinary writer(&output);
        writer.setVersion("AC1021", true);
        writer.setCodePage("ANSI_1252");
        const std::string semantic = std::string("^") + '\x07' + '\x1B';
        t.expect(writer.writeUtf8String(1, semantic),
                 "Binary-DXF control text writes without ASCII caret transformation");
        std::istringstream input(output.str(), std::ios::binary);
        dxfReaderBinary reader(&input);
        reader.setVersion("AC1021", true);
        reader.setCodePage("ANSI_1252");
        int code = 0;
        t.expect(reader.readRec(&code) && code == 1
                     && reader.getRawValue() == semantic
                     && reader.getUtf8String() == semantic,
                 "Binary-DXF caret and control bytes remain unchanged on readback");
    }
    {
        const std::string semantic = std::string(249u, 'A') + '\x07' + "tail";
        for (bool binary : {false, true}) {
            std::ostringstream output(std::ios::binary);
            std::unique_ptr<dxfWriter> writer = binary
                ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&output))
                : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&output));
            writer->setVersion("AC1021", true);
            writer->setCodePage("ANSI_1252");
            t.expect(writer->writeUtf8MTextChunks(semantic, 3, 1, 250u, 250u),
                     "caret pair participates atomically in the existing MTEXT chunker");
            std::istringstream input(output.str(), std::ios::binary);
            std::unique_ptr<dxfReader> reader = binary
                ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
                : std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
            reader->setVersion("AC1021", true);
            reader->setCodePage("ANSI_1252");
            std::vector<std::pair<int, std::string>> chunks;
            std::string decoded;
            int code = 0;
            while (reader->readRec(&code)) {
                chunks.emplace_back(code, reader->getRawValue());
                decoded += reader->getUtf8String();
            }
            const bool asciiChunks = !binary && chunks.size() == 2u
                && chunks[0].first == 3 && chunks[0].second == std::string(249u, 'A')
                && chunks[1].first == 1 && chunks[1].second == "^Gtail";
            const bool binaryChunks = binary && chunks.size() == 2u
                && chunks[0].first == 3
                && chunks[0].second == std::string(249u, 'A') + '\x07'
                && chunks[1].first == 1 && chunks[1].second == "tail";
            t.expect((asciiChunks || binaryChunks) && decoded == semantic,
                     "MTEXT group-3 boundary never splits ASCII caret syntax and Binary-DXF stays unchanged");
        }
    }
    {
        std::istringstream input("1\n^z^@\n");
        dxfReaderAscii reader(&input);
        reader.setVersion("AC1021", true);
        reader.setCodePage("ANSI_1252");
        int code = 0;
        t.expect(reader.readRec(&code) && code == 1
                     && reader.getUtf8String() == "^z^@"
                     && (reader.textReadObservations().issueMask
                         & DRW::dxfTextIssueMask(DRW::DxfTextIssue::UnrepresentedEscape)) != 0u,
                 "unknown caret text remains literal and the refused NUL escape is observed");
    }
    std::cout << "ASCII DXF caret/control transport cases: " << cases << '\n';
}

void testTargetTextTransport(TestContext& t) {
    using Policy = DRW::DxfTextWritePolicy;
    using Failure = dxfWriter::TextFailure;
    struct WireRow { const char* page; std::string semantic, legacy, modern; };
    const WireRow rows[] = {
        {"ANSI_1252", "plain?", "plain?", "plain?"},
        {"ANSI_1252", "\xC3\xA9", "\xE9", "\xC3\xA9"},
        {"ANSI_1252", "\xD0\x90", "\\U+0410", "\xD0\x90"},
        {"ANSI_932", "\xE6\x97\xA5", "\x93\xFA", "\xE6\x97\xA5"},
        {"ANSI_950", "\xE4\xB8\xAD", "\xA4\xA4", "\xE4\xB8\xAD"},
        {"ANSI_1252", "\xEF\xBF\xBD", "\\U+FFFD", "\xEF\xBF\xBD"},
        {"ANSI_1252", "prefix\\U+0041suffix", "prefix\\U+005CU+0041suffix", "prefix\\U+005CU+0041suffix"},
        {"ANSI_1252", "\\M+193FA", "\\U+005CM+193FA", "\\U+005CM+193FA"},
        {"ANSI_1252", "\\U+\\M+", "\\U+005CU+\\U+005CM+", "\\U+005CU+\\U+005CM+"},
        {"ANSI_1252", "C:\\ordinary\\path\\P{\\fArial;A}\\S1/2;", "C:\\ordinary\\path\\P{\\fArial;A}\\S1/2;", "C:\\ordinary\\path\\P{\\fArial;A}\\S1/2;"}
    };
    std::size_t wireCases = 0;
    for (const auto& row : rows) for (bool modern : {false, true}) for (bool binary : {false, true}) {
        std::ostringstream output(std::ios::binary);
        std::unique_ptr<dxfWriter> writer = binary
            ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&output))
            : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&output));
        writer->setVersion(modern ? "AC1021" : "AC1018", true);
        writer->setCodePage(row.page);
        writer->setTextWritePolicy(Policy::RequireLossless);
        const auto original = row.semantic;
        const auto encoded = writer->fromUtf8String(row.semantic);
        t.expect(encoded == (modern ? row.modern : row.legacy)
                     && writer->writeUtf8String(1, row.semantic) && !writer->hasWriteError()
                     && row.semantic == original,
                 "literal scalar/wire anchors constrain strict transport without changing caller text");
        std::istringstream input(output.str(), std::ios::binary);
        std::unique_ptr<dxfReader> reader = binary
            ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
            : std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
        reader->setVersion(modern ? "AC1021" : "AC1018", true);
        reader->setCodePage(row.page);
        int code = 0;
        t.expect(reader->readRec(&code) && code == 1 && reader->getRawValue() == encoded
                     && reader->getUtf8String() == row.semantic,
                 "strict semantic quoting is one-pass callback-idempotent in both framings");
        ++wireCases;
    }

    // LibreDWG #1412 describes an unterminated C helper after quoting C0 and
    // MIF text. Exercise the analogous transformations with length-bearing
    // C++ strings, then require the following record to remain independently
    // readable with no bytes absorbed into the first value.
    const std::string transformedControl = std::string("prefix") + '\x07'
        + "\\M+193FA" + '\x01' + "suffix";
    for (const char* version : {"AC1018", "AC1032"}) {
        for (const bool binary : {false, true}) {
            std::ostringstream output(std::ios::out | std::ios::binary);
            std::unique_ptr<dxfWriter> writer = binary
                ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&output))
                : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&output));
            writer->setVersion(version, true);
            writer->setCodePage("ANSI_1252");
            writer->setTextWritePolicy(Policy::RequireLossless);
            std::string expectedWire = "prefix";
            if (binary)
                expectedWire.push_back('\x07');
            else
                expectedWire += "^G";
            expectedWire += "\\U+005CM+193FA";
            if (binary)
                expectedWire.push_back('\x01');
            else
                expectedWire += "^A";
            expectedWire += "suffix";

            const std::string encoded = writer->fromUtf8String(
                transformedControl);
            const bool wrote = writer->writeUtf8String(
                    1, transformedControl)
                && writer->writeUtf8String(2, "FOLLOWING_SENTINEL")
                && !writer->hasWriteError();
            std::istringstream input(output.str(), std::ios::in | std::ios::binary);
            std::unique_ptr<dxfReader> reader = binary
                ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
                : std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
            reader->setVersion(version, true);
            reader->setCodePage("ANSI_1252");
            int code = 0;
            const bool firstRead = reader->readRec(&code) && code == 1;
            const std::string firstRaw = firstRead ? reader->getRawValue()
                                                   : std::string{};
            const std::string firstText = firstRead ? reader->getUtf8String()
                                                    : std::string{};
            const bool secondRead = reader->readRec(&code) && code == 2
                && reader->getRawValue() == "FOLLOWING_SENTINEL";
            t.expect(encoded == expectedWire && wrote && firstRead
                         && firstRaw == expectedWire
                         && firstText == transformedControl && secondRead
                         && !reader->readRec(&code),
                     "C0/MIF-transformed text has exact length and preserves the following DXF record");
        }
    }
    std::size_t rejectionCases = 0;
    for (const std::string bad : {"\x80", "\xC0\xAF", "\xED\xA0\x80", "\xF4\x90\x80\x80", "\xE2\x82"})
        for (bool modern : {false, true}) {
        std::ostringstream output;
        dxfWriterAscii writer(&output);
        writer.setVersion(modern ? "AC1021" : "AC1018", true);
        writer.setTextWritePolicy(Policy::RequireLossless);
        {
            DxfWriterRecordScope scope(writer);
            t.expect(!writer.writeUtf8String(1, bad), "invalid UTF8 fails before any semantic field bytes");
        }
        t.expect(output.str().empty() && writer.hasWriteError()
                     && writer.textFailure() == Failure::InvalidUtf8
                     && !writer.writeUtf8String(1, "later valid"),
                 "record rollback cannot clear an admitted operation's sticky text failure");
        ++rejectionCases;
    }
    for (bool binary : {false, true}) {
        for (bool modern : {false, true}) {
            std::ostringstream output(std::ios::binary);
            std::unique_ptr<dxfWriter> writer = binary
                ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&output))
                : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&output));
            writer->setVersion(modern ? "AC1021" : "AC1018", true);
            writer->setTextWritePolicy(Policy::RequireLossless);
            const bool written = writer->writeUtf8String(1, "\xF0\x9F\x98\x80");
            t.expect(written == modern && (modern || (output.str().empty() && writer->textFailure() == Failure::TargetLoss)),
                     "supplementary scalars remain UTF8 in modern targets and cannot become a legacy question mark");
            ++rejectionCases;
        }
    }
    std::size_t chunkCases = 0;
    for (const auto& row : rows) for (bool modern : {false, true}) for (bool binary : {false, true}) {
        const auto source = std::string(248, 'A') + row.semantic + "\\Ptail";
        std::ostringstream output(std::ios::binary);
        std::unique_ptr<dxfWriter> writer = binary
            ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&output))
            : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&output));
        writer->setVersion(modern ? "AC1021" : "AC1018", true);
        writer->setCodePage(row.page);
        writer->setTextWritePolicy(Policy::RequireLossless);
        t.expect(writer->writeUtf8Chunks(source, 3, 1, 250), "whole semantic chunk preflight succeeds");
        std::istringstream input(output.str(), std::ios::binary);
        std::unique_ptr<dxfReader> reader = binary
            ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
            : std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
        reader->setVersion(modern ? "AC1021" : "AC1018", true);
        reader->setCodePage(row.page);
        std::string decoded;
        int code = 0, last = 0;
        while (reader->readRec(&code)) {
            t.expect((code == 3 || code == 1) && last != 1 && reader->getRawValue().size() <= 250,
                     "encoded chunks preserve field order and byte ceiling");
            decoded += reader->getUtf8String();
            last = code;
        }
        t.expect(last == 1 && decoded == source,
                 "CIF/MIF quotes and UTF8/DBCS scalar atoms are never split across physical chunks");
        ++chunkCases;
    }
    std::size_t preflightFailureCases = 0;
    for (bool binary : {false, true}) {
        const std::string semantic = std::string("A") + "\xE6\x97\xA5";
        std::ostringstream strictOutput(std::ios::binary);
        std::unique_ptr<dxfWriter> strictWriter = binary
            ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&strictOutput))
            : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&strictOutput));
        strictWriter->setVersion("AC1018", true);
        strictWriter->setCodePage("ANSI_932");
        strictWriter->setTextWritePolicy(Policy::RequireLossless);
        t.expect(!strictWriter->writeUtf8Chunks(semantic, 3, 1, 1)
                     && strictOutput.str().empty()
                     && strictWriter->textFailure() == Failure::TargetLoss,
                 "strict chunk range preflight emits no earlier fields on a later oversized scalar");

        std::ostringstream compatibleOutput(std::ios::binary);
        std::unique_ptr<dxfWriter> compatibleWriter = binary
            ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&compatibleOutput))
            : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&compatibleOutput));
        compatibleWriter->setVersion("AC1018", true);
        compatibleWriter->setCodePage("ANSI_932");
        t.expect(!compatibleWriter->writeUtf8MTextChunks(semantic, 3, 1, 1, 10)
                     && compatibleOutput.str().empty()
                     && compatibleWriter->hasWriteError(),
                 "compatible MTEXT range preflight emits no earlier fields on an oversized scalar");
        preflightFailureCases += 2u;
    }
    std::size_t emptyChunkCases = 0;
    std::size_t shortFinalChunkCases = 0;
    for (bool binary : {false, true}) {
        std::ostringstream emptyChunkOutput(std::ios::binary);
        std::unique_ptr<dxfWriter> emptyChunkWriter = binary
            ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&emptyChunkOutput))
            : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&emptyChunkOutput));
        emptyChunkWriter->setVersion("AC1021", true);
        emptyChunkWriter->setTextWritePolicy(Policy::RequireLossless);
        t.expect(emptyChunkWriter->writeUtf8Chunks("", 3, 1, 250),
                 "strict empty chunk emits its explicit final field");
        {
            std::istringstream input(emptyChunkOutput.str(), std::ios::binary);
            std::unique_ptr<dxfReader> reader = binary
                ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
                : std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
            int code = 0;
            t.expect(reader->readRec(&code) && code == 1
                         && reader->getRawValue().empty()
                         && !reader->readRec(&code),
                     "strict empty chunk remains one empty group-1 value");
        }
        ++emptyChunkCases;

        for (bool strict : {false, true}) {
            std::ostringstream output(std::ios::binary);
            std::unique_ptr<dxfWriter> writer = binary
                ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&output))
                : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&output));
            writer->setVersion("AC1021", true);
            if (strict) writer->setTextWritePolicy(Policy::RequireLossless);
            t.expect(writer->writeUtf8MTextChunks("", 3, 1, 250, 250),
                     "empty strict/compatible MTEXT emits its explicit final field");
            std::istringstream input(output.str(), std::ios::binary);
            std::unique_ptr<dxfReader> reader = binary
                ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
                : std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
            int code = 0;
            t.expect(reader->readRec(&code) && code == 1
                         && reader->getRawValue().empty()
                         && !reader->readRec(&code),
                     "empty strict/compatible MTEXT remains one empty group-1 value");
            ++emptyChunkCases;
        }

        const std::string fullContinuation(500, 'A');
        for (bool strict : {false, true}) {
            std::ostringstream output(std::ios::binary);
            std::unique_ptr<dxfWriter> writer = binary
                ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&output))
                : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&output));
            writer->setVersion("AC1021", true);
            if (strict) writer->setTextWritePolicy(Policy::RequireLossless);
            t.expect(writer->writeUtf8MTextChunks(fullContinuation, 3, 1,
                                                  1000, 250),
                     "strict/compatible MTEXT accepts exact full continuation boundary");
            std::istringstream input(output.str(), std::ios::binary);
            std::unique_ptr<dxfReader> reader = binary
                ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
                : std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
            std::vector<std::pair<int, std::string>> chunks;
            std::string decoded;
            int code = 0;
            while (reader->readRec(&code)) {
                chunks.emplace_back(code, reader->getRawValue());
                decoded += reader->getUtf8String();
            }
            t.expect(chunks.size() == 3
                         && chunks[0].first == 3 && chunks[0].second.size() == 250
                         && chunks[1].first == 3 && chunks[1].second.size() == 249
                         && chunks[2].first == 1 && chunks[2].second == "A"
                         && decoded == fullContinuation,
                     "strict/compatible MTEXT keeps short final scalar and group order");
            ++shortFinalChunkCases;
        }
    }
    std::size_t framingCases = 0;
    for (bool binary : {false, true}) {
        for (const std::string& text : {std::string("A\0B", 3), std::string("A\nB")}) {
            std::ostringstream output(std::ios::binary);
            std::unique_ptr<dxfWriter> writer = binary
                ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&output))
                : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&output));
            writer->setVersion("AC1021", true); writer->setTextWritePolicy(Policy::RequireLossless);
            const bool eligible = binary && text.find('\0') == std::string::npos;
            t.expect(writer->writeUtf8String(1, text) == eligible
                         && (eligible || (output.str().empty() && writer->textFailure() == Failure::UnsafeRecordText)),
                     "semantic framing preserves binary newline but never publishes NUL/ASCII pair injection");
            ++framingCases;
        }
    }
    for (const char* page : {"UTF8", "UTF16"}) {
        std::ostringstream output;
        dxfWriterAscii writer(&output);
        writer.setVersion("AC1018", true); writer.setCodePage(page);
        writer.setTextWritePolicy(Policy::RequireLossless);
        t.expect(!writer.writeUtf8String(1, "plain") && output.str().empty()
                     && writer.textFailure() == Failure::UnsupportedProfile,
                 "a canonical/exact Unicode name cannot admit an unsupported legacy target profile");
    }
    std::size_t compatibleMTextCases = 0;
    for (bool binary : {false, true}) {
        for (bool modern : {false, true}) {
            const std::string semantic =
                std::string(249, 'A') + "\xE6\x97\xA5" + "Z\\Ptail";
            std::ostringstream output(std::ios::binary);
            std::unique_ptr<dxfWriter> writer = binary
                ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&output))
                : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&output));
            writer->setVersion(modern ? "AC1021" : "AC1018", true);
            writer->setCodePage("ANSI_932");
            t.expect(writer->writeUtf8MTextChunks(semantic, 3, 1, 250, 250)
                         && !writer->hasWriteError(),
                     "compatible MTEXT chunking preserves DBCS/UTF8 boundaries and ASCII controls");
            std::istringstream input(output.str(), std::ios::binary);
            std::unique_ptr<dxfReader> reader = binary
                ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
                : std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
            reader->setVersion(modern ? "AC1021" : "AC1018", true);
            reader->setCodePage("ANSI_932");
            int code = 0;
            std::vector<std::pair<int, std::string>> chunks;
            std::string decoded;
            while (reader->readRec(&code)) {
                chunks.emplace_back(code, reader->getRawValue());
                decoded += reader->getUtf8String();
            }
            const std::string expectedEncodedTail = modern
                ? "\xE6\x97\xA5Z\\Ptail" : "\x93\xFAZ\\Ptail";
            t.expect(chunks.size() == 2 && chunks[0].first == 3
                         && chunks[0].second == std::string(249, 'A')
                         && chunks[1].first == 1
                         && chunks[1].second == expectedEncodedTail
                         && decoded == semantic,
                     "compatible ASCII/Binary MTEXT preserves target bytes, chunk order and semantic text");
            ++compatibleMTextCases;
        }
    }
    std::size_t borrowedViewCases = 0;
    for (bool binary : {false, true}) {
        std::string source = "prefixcapturedsuffix";
        const std::string_view borrowed(source.data() + 6, 8);
        std::ostringstream output(std::ios::binary);
        std::unique_ptr<dxfWriter> writer = binary
            ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&output))
            : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&output));
        const bool written = writer->writeString(1, borrowed);
        source.assign(source.size(), 'x');
        const std::string bytes = output.str();
        const bool emittedBorrowedValue = binary
            ? bytes.find(std::string("captured\0", 9)) != std::string::npos
            : bytes.find("captured\n") != std::string::npos;
        t.expect(written && emittedBorrowedValue
                     && bytes.find("xxxxxxxx") == std::string::npos,
                 "ASCII/Binary raw sinks consume borrowed string views synchronously");
        ++borrowedViewCases;
    }
    std::size_t uppercaseCapsFastPathCases = 0;
    for (bool binary : {false, true}) {
        for (bool modern : {false, true}) {
            const std::string source = "ALREADY_UPPER_09_.Z";
            std::ostringstream fastOutput(std::ios::binary), rawOutput(std::ios::binary);
            std::unique_ptr<dxfWriter> fastWriter = binary
                ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&fastOutput))
                : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&fastOutput));
            std::unique_ptr<dxfWriter> rawWriter = binary
                ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&rawOutput))
                : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&rawOutput));
            fastWriter->setVersion(modern ? "AC1021" : "AC1018", true);
            rawWriter->setVersion(modern ? "AC1021" : "AC1018", true);
            fastWriter->setCodePage("ANSI_932");
            rawWriter->setCodePage("ANSI_932");
            t.expect(fastWriter->writeUtf8Caps(2, source)
                         && rawWriter->writeString(2, source)
                         && fastOutput.str() == rawOutput.str()
                         && source == "ALREADY_UPPER_09_.Z",
                     "already-uppercase ASCII symbols use the raw sink without changing bytes or caller text");
            ++uppercaseCapsFastPathCases;
        }
    }
    {
        // Preserve the old conversion path for a configured UTF-16 codec even
        // when the symbol itself happens to be plain uppercase ASCII.
        const std::string source = "ALREADY_UPPER";
        std::ostringstream capsOutput, utf16Output;
        dxfWriterAscii capsWriter(&capsOutput), utf16Writer(&utf16Output);
        capsWriter.setVersion("AC1018", true);
        utf16Writer.setVersion("AC1018", true);
        capsWriter.setCodePage("UTF-16");
        utf16Writer.setCodePage("UTF-16");
        t.expect(capsWriter.writeUtf8Caps(2, source)
                     && utf16Writer.writeUtf8String(2, source)
                     && capsOutput.str() == utf16Output.str(),
                 "UTF-16 output retains the established codec conversion path");
        ++uppercaseCapsFastPathCases;
    }
    {
        std::ostringstream output;
        dxfWriterAscii writer(&output);
        writer.setVersion("AC1009", true); writer.setTextWritePolicy(Policy::RequireLossless);
        const std::string sourceName = "name\\u+0041";
        t.expect(writer.writeUtf8Caps(2, sourceName)
                     && output.str().find("NAME\\U+005CU+0041") != std::string::npos,
                 "R12 ASCII-capitalized symbols quote escape-looking slashes after case policy");
        t.expect(sourceName == "name\\u+0041",
                 "ASCII symbol capitalization does not mutate caller-owned source text");
        std::ostringstream raw;
        dxfWriterAscii carrier(&raw);
        carrier.setVersion("AC1021", true); carrier.setTextWritePolicy(Policy::RequireLossless);
        t.expect(carrier.writeString(1, "\\U+0041") && raw.str().find("\\U+0041") != std::string::npos
                     && raw.str().find("\\U+005C") == std::string::npos,
                 "explicit raw carrier emission never quotes semantic-looking source bytes");
    }
    std::cout << "Target text units: wire=" << wireCases << ", rejection/modern controls=" << rejectionCases
              << ", chunks=" << chunkCases << ", framing=" << framingCases
              << ", chunk-preflight-failures=" << preflightFailureCases
              << ", empty-chunks=" << emptyChunkCases
              << ", short-final=" << shortFinalChunkCases
              << ", compatible-MTEXT=" << compatibleMTextCases
              << ", borrowed-view=" << borrowedViewCases
              << ", uppercase-caps-fast-path=" << uppercaseCapsFastPathCases
              << ", profile/caps/raw=4\n";
}

void testTargetTextFacade(TestContext& t) {
    using Policy = DRW::DxfTextWritePolicy;
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-target-text-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(directory);
    const auto bytes = [](const std::filesystem::path& path) {
        std::ifstream input(path, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(input), {});
    };
    const std::string sentinel = "target transport destination sentinel\n";
    const auto makeData = [](int field, const std::string& payload) {
        auto data = std::make_unique<dx_data>();
        data->headerC.addStr("$DWGCODEPAGE", "ANSI_1252", 3);
        auto* line = new DRW_Line();
        line->basePoint = {34, 35, 36}; line->secPoint = {37, 38, 39};
        data->mBlock->ent.push_back(line);
        if (field == 0 || field == 1) {
            DRW_Text* text = field == 0 ? new DRW_Text() : new DRW_MText();
            text->height = 1; text->text = payload;
            data->mBlock->ent.push_front(text);
        } else if (field == 2) {
            DRW_Layer layer; layer.name = payload; data->layers.push_back(layer);
        } else if (field == 3) {
            data->headerC.addStr("$DIMPOST", payload, 1);
        } else if (field == 4 || field == 5) {
            auto* block = new dx_ifaceBlock(); block->name = field == 4 ? payload : "TARGET";
            data->blocks.push_back(block);
            auto* insert = new DRW_Insert(); insert->name = block->name;
            if (field == 5) {
                auto attribute = std::make_shared<DRW_Attrib>();
                attribute->tag = "TAG"; attribute->height = 1; attribute->text = payload;
                insert->attlist.push_back(attribute);
            }
            data->mBlock->ent.push_front(insert);
        } else if (field == 6) {
            line->extData.push_back(std::make_shared<DRW_Variant>(1001, std::string("ACAD")));
            line->extData.push_back(std::make_shared<DRW_Variant>(1000, payload));
        } else {
            std::list<DRW_Variant> group;
            group.emplace_back(102, std::string("AUTHORED_APP"));
            group.emplace_back(1, payload); group.emplace_back(102, std::string("}"));
            line->appData.push_back(std::move(group));
        }
        return data;
    };
    std::size_t cases = 0;
    for (int field = 0; field < 8; ++field) for (bool modern : {false, true}) for (bool binary : {false, true}) {
        const auto path = directory / (std::to_string(cases) + ".dxf");
        auto data = makeData(field, field == 7 ? "\xC3\xA9" : "\x80");
        PreparedDxInterface adapter; adapter.attach(data.get());
        dxfRW codec(path.string().c_str()); adapter.attachWriter(&codec);
        t.expect(codec.dxfTextWritePolicy() == Policy::Compatible,
                 "ordinary facade writer default remains compatible");
        codec.setDxfTextWritePolicy(Policy::RequireLossless);
        { std::ofstream output(path, std::ios::binary); output << sentinel; }
        const auto version = modern ? DRW::AC1021 : DRW::AC1018;
        const auto firstHandle = data->mBlock->ent.front()->handle;
        const auto lastHandle = data->mBlock->ent.back()->handle;
        t.expect(!codec.write(&adapter, version, binary) && bytes(path) == sentinel
                     && codec.getError() != DRW::BAD_NONE && codec.getLastDiagnostic().failed()
                     && data->mBlock->ent.front()->handle == firstHandle
                     && data->mBlock->ent.back()->handle == lastHandle,
                 "target refusal preserves destination and publishes a primary failure for every required owner");
        if (field == 0 || field == 1) static_cast<DRW_Text*>(data->mBlock->ent.front())->text = "corrected";
        else if (field == 2) data->layers.front().name = "corrected";
        else if (field == 3) data->headerC.addStr("$DIMPOST", "corrected", 1);
        else if (field == 4) {
            data->blocks.front()->name = "corrected";
            static_cast<DRW_Insert*>(data->mBlock->ent.front())->name = "corrected";
        } else if (field == 5) static_cast<DRW_Insert*>(data->mBlock->ent.front())->attlist.front()->text = "corrected";
        else if (field == 6) data->mBlock->ent.back()->extData.back()->addString(1000, "corrected");
        else {
            auto value = data->mBlock->ent.back()->appData.front().begin();
            (++value)->addString(1, "corrected");
        }
        t.expect(codec.write(&adapter, version, binary) && bytes(path) != sentinel,
                 "same facade and corrected same model clear target failure and publish the retry");
        ++cases;
    }
    std::size_t idempotence = 0;
    for (int field : {0, 1}) for (bool modern : {false, true}) for (bool binary : {false, true}) {
        const auto path = directory / ("semantic-" + std::to_string(idempotence) + ".dxf");
        const auto source = std::string(248, 'A') + "\\U+0041\\M+193FA\\P{\\fArial;" + "\xE4\xB8\xAD" + "}\\S1/2;";
        auto data = makeData(field, source);
        PreparedDxInterface adapter; adapter.attach(data.get());
        dxfRW codec(path.string().c_str()); adapter.attachWriter(&codec);
        codec.setDxfTextWritePolicy(Policy::RequireLossless);
        const auto version = modern ? DRW::AC1021 : DRW::AC1018;
        for (int pass = 0; pass < 2; ++pass) {
            t.expect(codec.write(&adapter, version, binary), "strict valid literal text exports");
            dx_data read; dx_iface importer;
            const bool imported = importer.fileImport(path.string(), &read, false);
            const auto* text = read.mBlock->ent.empty() ? nullptr : dynamic_cast<const DRW_Text*>(read.mBlock->ent.front());
            t.expect(imported && text != nullptr && text->text == source,
                     "valid literal backslashes and MTEXT formatting survive repeated callback export/read");
        }
        ++idempotence;
    }
    struct CifHandoffRow {
        const char* rawSource;
        const char* semantic;
        const char* legacyOutput;
        const char* modernOutput;
    };
    const CifHandoffRow cifRows[] = {
        {"prefix\\U+005CU+0041suffix", "prefix\\U+0041suffix",
         "prefix\\U+005CU+0041suffix", "prefix\\U+005CU+0041suffix"},
        {"prefix\\U+0041suffix", "prefixAsuffix", "prefixAsuffix", "prefixAsuffix"},
        {"x\\U+005CU+0041\\U+4E2DZ", "x\\U+0041\xE4\xB8\xADZ",
         "x\\U+005CU+0041\\U+4E2DZ", "x\\U+005CU+0041\xE4\xB8\xADZ"},
        {"C:\\ordinary\\path\\P{\\fArial;A}\\S1/2;",
         "C:\\ordinary\\path\\P{\\fArial;A}\\S1/2;",
         "C:\\ordinary\\path\\P{\\fArial;A}\\S1/2;",
         "C:\\ordinary\\path\\P{\\fArial;A}\\S1/2;"}
    };
    struct PhysicalTextObservation {
        bool found{false};
        std::string raw;
        std::string semantic;
    };
    const auto observeText = [](const std::filesystem::path& path, bool modern,
                                bool binary) {
        PhysicalTextObservation result;
        std::ifstream input(path, std::ios::binary);
        if (!input) return result;
        if (binary) {
            static constexpr char signature[] = "AutoCAD Binary DXF\r\n\x1a\0";
            std::string actual(sizeof(signature) - 1, '\0');
            input.read(actual.data(), static_cast<std::streamsize>(actual.size()));
            if (!input || actual != std::string(signature, sizeof(signature) - 1))
                return result;
        }
        std::unique_ptr<dxfReader> reader = binary
            ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
            : std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
        reader->setVersion(modern ? "AC1021" : "AC1018", true);
        reader->setCodePage("ANSI_1252");
        bool inText = false;
        int code = 0;
        while (reader->readRec(&code)) {
            // For ordinary string groups, getString() returns the physical
            // carrier value without semantic CIF decoding. getRawValue() is
            // reserved for the opaque/raw-tag companion path.
            const std::string value = reader->getString();
            if (code == 0) {
                if (value == "TEXT" || value == "MTEXT") {
                    inText = true;
                    continue;
                }
                if (inText) break;
            }
            if (inText && code == 1) {
                result.found = true;
                result.raw = value;
                result.semantic = reader->getUtf8String();
                break;
            }
        }
        return result;
    };
    {
        const auto path = directory / "ascii-caret-public-text.dxf";
        const std::string semantic = std::string("caret ^ and BEL ") + '\x07' + " end";
        auto data = makeData(0, semantic);
        PreparedDxInterface adapter;
        adapter.attach(data.get());
        dxfRW codec(path.string().c_str());
        adapter.attachWriter(&codec);
        t.expect(codec.write(&adapter, DRW::AC1021, false),
                 "public ASCII DXF export routes semantic TEXT through caret encoding");
        const std::string wire = bytes(path);
        t.expect(wire.find("caret ^  and BEL ^G end") != std::string::npos
                     && wire.find(std::string(1, '\x07')) == std::string::npos,
                 "public ASCII output contains printable caret wire bytes, not the control scalar");
        dx_data imported;
        dx_iface importer;
        const bool read = importer.fileImport(path.string(), &imported, false);
        const auto* text = imported.mBlock->ent.empty() ? nullptr
            : dynamic_cast<const DRW_Text*>(imported.mBlock->ent.front());
        t.expect(read && text != nullptr && text->text == semantic,
                 "public ASCII TEXT import restores the exact semantic control/caret string");
    }
    {
        const auto path = directory / "ascii-caret-public-xdata.dxf";
        const std::string semantic = std::string("XDATA ") + '\x1B' + " ^ tail";
        auto data = makeData(6, semantic);
        PreparedDxInterface adapter;
        adapter.attach(data.get());
        dxfRW codec(path.string().c_str());
        adapter.attachWriter(&codec);
        const bool exported = codec.write(&adapter, DRW::AC1018, false);
        t.expect(exported,
                 "public legacy ASCII DXF export routes typed XDATA text through caret encoding");
        const std::string wire = bytes(path);
        dx_data imported;
        dx_iface importer;
        const bool read = importer.fileImport(path.string(), &imported, false);
        const auto* line = imported.mBlock->ent.empty() ? nullptr
            : dynamic_cast<const DRW_Line*>(imported.mBlock->ent.back());
        const bool hasXdata = line != nullptr
            && line->extData.size() >= 2u
            && line->extData.back()->code() == 1000
            && line->extData.back()->type() == DRW_Variant::STRING
            && std::string(line->extData.back()->c_str()) == semantic;
        t.expect(read && wire.find("XDATA ^[ ^  tail") != std::string::npos
                     && wire.find(std::string(1, '\x1B')) == std::string::npos
                     && hasXdata,
                 "public ASCII XDATA control escape restores semantic bytes without disturbing the owner");
    }
    {
        const auto path = directory / "ascii-caret-xdata-expanded-limit.dxf";
        const std::string sentinel = "caret-expanded-length sentinel\n";
        { std::ofstream existing(path, std::ios::binary); existing << sentinel; }
        auto data = makeData(6, std::string(254u, 'X') + '^');
        PreparedDxInterface adapter;
        adapter.attach(data.get());
        dxfRW codec(path.string().c_str());
        adapter.attachWriter(&codec);
        const bool exported = codec.write(&adapter, DRW::AC1021, false);
        t.expect(!exported && bytes(path) == sentinel
                     && codec.getLastDiagnostic().code == "xdata-text-length",
                 "caret expansion is included in the XDATA group-byte preflight before publication");
    }
    {
        const auto path = directory / "ascii-caret-public-attrib.dxf";
        const std::string semantic = std::string("ATTRIB ^ ") + '\x1B' + " tail";
        auto data = makeData(5, semantic);
        PreparedDxInterface adapter;
        adapter.attach(data.get());
        dxfRW codec(path.string().c_str());
        adapter.attachWriter(&codec);
        const bool exported = codec.write(&adapter, DRW::AC1021, false);
        dx_data imported;
        dx_iface importer;
        const bool read = exported
            && importer.fileImport(path.string(), &imported, false);
        const auto* insert = imported.mBlock->ent.empty() ? nullptr
            : dynamic_cast<const DRW_Insert*>(imported.mBlock->ent.front());
        const bool hasAttribute = insert != nullptr && !insert->attlist.empty()
            && insert->attlist.front()->text == semantic;
        t.expect(exported && read && hasAttribute,
                 "public nested ATTRIB caret/control text survives ASCII export and import");
    }
    {
        const auto path = directory / "ascii-caret-public-attdef.dxf";
        const std::string semantic = std::string("ATTDEF ^ ") + '\x07' + " tail";
        auto data = std::make_unique<dx_data>();
        auto* attdef = new DRW_Attdef();
        attdef->height = 1.0;
        attdef->tag = "TAG";
        attdef->prompt = "Prompt";
        attdef->style = "Standard";
        attdef->text = semantic;
        data->mBlock->ent.push_back(attdef);
        PreparedDxInterface adapter;
        adapter.attach(data.get());
        dxfRW codec(path.string().c_str());
        adapter.attachWriter(&codec);
        const bool exported = codec.write(&adapter, DRW::AC1021, false);
        dx_data imported;
        dx_iface importer;
        const bool read = exported
            && importer.fileImport(path.string(), &imported, false);
        const auto* importedAttdef = imported.mBlock->ent.empty() ? nullptr
            : dynamic_cast<const DRW_Attdef*>(imported.mBlock->ent.front());
        t.expect(exported && read && importedAttdef != nullptr
                     && importedAttdef->text == semantic,
                 "public ATTDEF caret/control text survives ASCII export and import");
    }
    std::size_t sourceBoundHandoffs = 0;
    for (const auto& row : cifRows) for (bool mtext : {false, true})
        for (bool modern : {false, true}) for (bool binary : {false, true}) {
        const auto stem = "cif-source-" + std::to_string(sourceBoundHandoffs);
        const auto sourcePath = directory / (stem + "-input.dxf");
        {
            std::ofstream bytes(sourcePath, std::ios::binary);
            if (!bytes) {
                t.expect(false, "could not create source-authored CIF handoff control");
                continue;
            }
            if (binary) {
                static constexpr char signature[] = "AutoCAD Binary DXF\r\n\x1a\0";
                bytes.write(signature, sizeof(signature) - 1);
            }
            std::unique_ptr<dxfWriter> sourceWriter = binary
                ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&bytes))
                : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&bytes));
            sourceWriter->setVersion(modern ? "AC1021" : "AC1018", true);
            sourceWriter->setCodePage("ANSI_1252");
            const char* version = modern ? "AC1021" : "AC1018";
            const char* entity = mtext ? "MTEXT" : "TEXT";
            const bool sourceWritten =
                sourceWriter->writeString(0, "SECTION")
                && sourceWriter->writeString(2, "HEADER")
                && sourceWriter->writeString(9, "$ACADVER")
                && sourceWriter->writeString(1, version)
                && sourceWriter->writeString(9, "$DWGCODEPAGE")
                && sourceWriter->writeString(3, "ANSI_1252")
                && sourceWriter->writeString(0, "ENDSEC")
                && sourceWriter->writeString(0, "SECTION")
                && sourceWriter->writeString(2, "ENTITIES")
                && sourceWriter->writeString(0, entity)
                && sourceWriter->writeString(5, "A")
                && sourceWriter->writeString(100, "AcDbEntity")
                && sourceWriter->writeString(8, "0")
                && sourceWriter->writeString(100, mtext ? "AcDbMText" : "AcDbText")
                && sourceWriter->writeDouble(10, 1)
                && sourceWriter->writeDouble(20, 2)
                && sourceWriter->writeDouble(30, 0)
                && sourceWriter->writeDouble(40, 1)
                && sourceWriter->writeString(1, row.rawSource)
                && sourceWriter->writeString(0, "LINE")
                && sourceWriter->writeString(5, "B")
                && sourceWriter->writeString(8, "0")
                && sourceWriter->writeDouble(10, 34)
                && sourceWriter->writeDouble(20, 35)
                && sourceWriter->writeDouble(30, 36)
                && sourceWriter->writeDouble(11, 37)
                && sourceWriter->writeDouble(21, 38)
                && sourceWriter->writeDouble(31, 39)
                && sourceWriter->writeString(0, "ENDSEC")
                && sourceWriter->writeString(0, "EOF");
            t.expect(sourceWritten, "source raw carrier is authored without semantic CIF escaping");
        }
        auto current = std::make_unique<dx_data>();
        dx_iface sourceImporter;
        const bool sourceRead = sourceImporter.fileImport(sourcePath.string(), current.get(), false);
        const DRW_Text* sourceText = nullptr;
        bool sourceLine = false;
        if (current->mBlock != nullptr) for (const auto* candidate : current->mBlock->ent) {
            if (const auto* text = dynamic_cast<const DRW_Text*>(candidate)) sourceText = text;
            if (candidate != nullptr && candidate->eType == DRW::LINE) sourceLine = true;
        }
        const auto sourceObservation = observeText(sourcePath, modern, binary);
        const bool sourceMatches = sourceRead && sourceText != nullptr
            && sourceText->text == row.semantic && sourceLine
            && sourceObservation.found && sourceObservation.raw == row.rawSource
            && sourceObservation.semantic == row.semantic;
        if (!sourceMatches)
            std::cerr << "CIF source mismatch: case=" << sourceBoundHandoffs
                      << " type=" << (mtext ? "MTEXT" : "TEXT")
                      << " modern=" << modern << " binary=" << binary
                      << " raw=" << sourceObservation.raw
                      << " semantic=" << sourceObservation.semantic << '\n';
        t.expect(sourceMatches,
                 "physical CIF carrier and public TEXT/MTEXT callback are distinct, expected layers");
        if (!sourceRead || sourceText == nullptr || sourceText->text != row.semantic) {
            ++sourceBoundHandoffs;
            continue;
        }
        for (int pass = 0; pass < 2; ++pass) {
            const auto outputPath = directory / (stem + "-pass-" + std::to_string(pass) + ".dxf");
            PreparedDxInterface adapter; adapter.attach(current.get());
            dxfRW output(outputPath.string().c_str()); adapter.attachWriter(&output);
            output.setDxfTextWritePolicy(Policy::RequireLossless);
            const auto version = modern ? DRW::AC1021 : DRW::AC1018;
            const bool written = output.write(&adapter, version, binary);
            const auto expectedRaw = modern ? row.modernOutput : row.legacyOutput;
            const auto observation = written ? observeText(outputPath, modern, binary)
                                             : PhysicalTextObservation{};
            auto next = std::make_unique<dx_data>();
            dx_iface readback;
            const bool imported = written && readback.fileImport(outputPath.string(), next.get(), false);
            const DRW_Text* readbackText = nullptr;
            bool readbackLine = false;
            if (next->mBlock != nullptr) for (const auto* candidate : next->mBlock->ent) {
                if (const auto* text = dynamic_cast<const DRW_Text*>(candidate)) readbackText = text;
                if (candidate != nullptr && candidate->eType == DRW::LINE) readbackLine = true;
            }
            const bool handoffMatches = written && observation.found
                && observation.raw == expectedRaw && observation.semantic == row.semantic
                && imported && readbackText != nullptr
                && readbackText->text == row.semantic && readbackLine;
            if (!handoffMatches)
                std::cerr << "CIF handoff mismatch: case=" << sourceBoundHandoffs
                          << " pass=" << pass << " type=" << (mtext ? "MTEXT" : "TEXT")
                          << " modern=" << modern << " binary=" << binary
                          << " written=" << written << " imported=" << imported
                          << " raw=" << observation.raw
                          << " expected-raw=" << expectedRaw
                          << " semantic=" << observation.semantic
                          << " callback=" << (readbackText != nullptr ? readbackText->text : "<missing>")
                          << '\n';
            t.expect(handoffMatches,
                     "successive adapter handoff preserves raw/semantic CIF layers and following LINE");
            if (!written || !imported || readbackText == nullptr) break;
            current = std::move(next);
        }
        ++sourceBoundHandoffs;
    }
    for (bool binary : {false, true}) {
        dx_data data;
        const auto path = directory / (binary ? "geo-binary.dxf" : "geo-ascii.dxf");
        dxfRW codec(path.string().c_str());
        class GeoSink final : public dx_iface {
        public:
            GeoSink(dx_data& data, dxfRW& codec) : codec(codec) {
                cData = &data; currentBlock = data.mBlock; dxfW = &codec;
                geo.m_version = 2;
            }
            void writeObjects() override { wrote = codec.writeGeoData(&geo); }
            void addGeoData(const DRW_GeoData& value) override { observed = value.m_coordinateSystemDefinition; }
            dxfRW& codec; DRW_GeoData geo; bool wrote{false}; std::string observed;
        } sink(data, codec);
        const auto definition = std::string(254, 'A') + "\xE4\xB8\xAD" + "\\U+0041";
        sink.geo.m_coordinateSystemDefinition = definition;
        codec.setDxfTextWritePolicy(Policy::RequireLossless);
        t.expect(codec.write(&sink, DRW::AC1024, binary) && sink.wrote
                     && codec.read(&sink, false) && sink.observed == definition,
                 "GEODATA strict255-byte chunks preserve a boundary Unicode scalar and literal escape");
    }
    {
        auto data = makeData(0, "\x80");
        const auto path = directory / "callback-policy.dxf";
        dxfRW codec(path.string().c_str());
        class PolicySink final : public dx_iface {
        public:
            PolicySink(dx_data& data, dxfRW& codec) : codec(codec) { cData = &data; currentBlock = data.mBlock; dxfW = &codec; }
            void writeEntities() override {
                codec.setDxfTextWritePolicy(Policy::Compatible);
                dx_iface::writeEntities();
            }
            dxfRW& codec;
        } sink(*data, codec);
        codec.setDxfTextWritePolicy(Policy::RequireLossless);
        t.expect(!codec.write(&sink, DRW::AC1021, false)
                     && codec.getLastDiagnostic().code == "dxf-text-invalid-utf8"
                     && codec.dxfTextWritePolicy() == Policy::Compatible
                     && codec.write(&sink, DRW::AC1021, false),
                 "callback policy changes affect the next operation, never the admitted strict writer");
    }
    for (bool modern : {false, true}) for (bool binary : {false, true}) {
        auto data = makeData(7, "ascii raw control");
        auto& group = data->mBlock->ent.back()->appData.front();
        auto close = std::prev(group.end());
        group.emplace(close, 1000, std::string("\xD0\x90"));
        group.emplace(close, 310, std::string("C6D096"));
        group.emplace(close, 320, std::string("ABCDEF"));
        const auto path = directory / (std::string("mixed-roles-") + (modern ? "modern" : "legacy") + (binary ? "binary" : "ascii") + ".dxf");
        PreparedDxInterface sink; sink.attach(data.get());
        dxfRW codec(path.string().c_str()); sink.attachWriter(&codec);
        codec.setDxfTextWritePolicy(Policy::RequireLossless);
        const bool written = codec.write(&sink, modern ? DRW::AC1021 : DRW::AC1018, binary);
        dx_data read; dx_iface importer;
        const bool imported = written && importer.fileImport(path.string(), &read, false);
        bool scalar = false, handle = false, chunk = false;
        if (imported && !read.mBlock->ent.empty()) for (const auto& values : read.mBlock->ent.back()->appData)
            for (const auto& value : values) {
                if (value.type() != DRW_Variant::STRING) continue;
                const auto text = std::string(value.c_str());
                if (value.code() == 1000) scalar = text == "\xD0\x90";
                if (value.code() == 320) handle = text == "ABCDEF";
                if (value.code() == 310) chunk = text == "C6D096";
            }
        t.expect(imported && scalar && handle && chunk,
                 "1000-in102 is encoded once while opaque ASCII, binary hex and arbitrary handles retain their roles");
    }
    std::cout << "Target text facade: owner refusals/same-model retries=" << cases
              << ", repeated literal routes=" << idempotence << ", source-bound CIF handoffs=" << sourceBoundHandoffs
              << ", GEODATA controls=2, snapshot control=1, mixed-role controls=4\n";
    if (t.failures == 0) std::filesystem::remove_all(directory);
    else std::cerr << "Target transport evidence: " << directory << '\n';
}

void writeTargetDwgControl(TestContext& t, const std::filesystem::path& path) {
    // Optional locally authored whole-DWG witness, not an external/native oracle.
    dwgRW writer(path.string().c_str());
    class Source final : public dx_iface {
    public:
        explicit Source(dwgRW& writer) : writer(writer) {
            cData = &data; currentBlock = data.mBlock;
            text.height = 1; text.basePoint = {1, 2, 3};
            text.text = "DWG \xF0\x9F\x98\x80 literal\\U+0041";
            line.basePoint = {34, 35, 36}; line.secPoint = {37, 38, 39};
        }
        void writeHeader(DRW_Header& header) override { header.addStr("$DWGCODEPAGE", "ANSI_1252", 3); }
        void writeEntities() override { wrote = writer.writeText(&text) && writer.writeLine(&line); }
        void writeBlocks() override {} void writeBlockRecords() override {}
        void writeLTypes() override {} void writeLayers() override {} void writeTextstyles() override {}
        void writeVports() override {} void writeDimstyles() override {} void writeAppId() override {} void writeObjects() override {}
        void writeViews() override {} void writeUCSs() override {}
        dx_data data; dwgRW& writer; DRW_Text text; DRW_Line line; bool wrote{false};
    } source(writer);
    t.expect(writer.write(&source, DRW::AC1027, true) && source.wrote,
             "authored DWG source writes required TEXT/LINE without a skip");
    dx_data data; dx_iface reader;
    const bool read = reader.fileImport(path.string(), &data, false);
    bool matched = false;
    for (const auto* entity : data.mBlock->ent) {
        const auto* text = dynamic_cast<const DRW_Text*>(entity);
        if (text != nullptr && text->text == source.text.text) matched = true;
    }
    t.expect(read && matched && data.mBlock->ent.size() == 2,
             "authored whole-DWG witness reaches target admission with the intended semantic text");
    std::cout << "Authored target DWG witness: " << (t.failures == 0 ? "PASS" : "FAIL") << '\n';
}

void testDwgTvTuModernDxfCharacterization(TestContext& t) {
    struct SourceObservation {
        std::vector<std::uint8_t> wire;
        std::string typed;
        std::uint8_t sentinel {0};
        bool valid {false};
    };
    const auto captureSource = [](DRW::Version version,
                                  const std::string& value) {
        SourceObservation result;
        DRW_TextCodec writerCodec;
        writerCodec.setVersion(version, false);
        if (version <= DRW::AC1018)
            writerCodec.setCodePage("ANSI_1252", false);
        dwgBufferW writer(&writerCodec);
        writer.putVariableText(version, value);
        writer.putRawChar8(0xA7u);
        result.wire = writer.data();
        if (!writer.isGood() || result.wire.empty())
            return result;

        DRW_TextCodec readerCodec;
        readerCodec.setVersion(version, false);
        if (version <= DRW::AC1018)
            readerCodec.setCodePage("ANSI_1252", false);
        dwgBuffer reader(result.wire.data(), result.wire.size(), &readerCodec);
        result.typed = reader.getVariableText(version);
        result.sentinel = reader.getRawChar8();
        result.valid = reader.isGood();
        return result;
    };
    const auto hex = [](const std::vector<std::uint8_t>& bytes) {
        static constexpr char digits[] = "0123456789ABCDEF";
        std::string result;
        result.reserve(bytes.size() * 2u);
        for (std::uint8_t byte : bytes) {
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 0x0Fu]);
        }
        return result;
    };
    const auto hexString = [&hex](const std::string& value) {
        std::vector<std::uint8_t> bytes(value.begin(), value.end());
        return hex(bytes);
    };
    struct WireOracle {
        std::vector<std::uint8_t> bytes;
        unsigned bitPosition {0};

        void bit(bool value) {
            if (bitPosition == 0)
                bytes.push_back(0);
            if (value)
                bytes.back() = static_cast<std::uint8_t>(bytes.back()
                    | static_cast<std::uint8_t>(1u << (7u - bitPosition)));
            bitPosition = (bitPosition + 1u) & 7u;
        }

        void rawByte(std::uint8_t value) {
            if (bitPosition == 0) {
                bytes.push_back(value);
                return;
            }
            bytes.back() = static_cast<std::uint8_t>(bytes.back()
                | static_cast<std::uint8_t>(value >> bitPosition));
            bytes.push_back(static_cast<std::uint8_t>(
                value << (8u - bitPosition)));
        }

        void bitShort(std::uint16_t value) {
            if (value < 256u) {
                bit(false);
                bit(true);
                rawByte(static_cast<std::uint8_t>(value));
            } else {
                bit(false);
                bit(false);
                rawByte(static_cast<std::uint8_t>(value & 0xFFu));
                rawByte(static_cast<std::uint8_t>(value >> 8));
            }
        }
    };
    struct Row {
        const char* name;
        std::string source;
        std::string expectedTvPayload;
        std::string expectedTv;
        std::string expectedTu;
        std::string expectedTvDxfRaw;
        std::string expectedTuDxfRaw;
    };
    const Row rows[] = {
        {"bmp-scalar", "prefix \xCE\xA9 suffix",
         "prefix \\U+03A9 suffix", "prefix \xCE\xA9 suffix",
         "prefix \xCE\xA9 suffix", "prefix \xCE\xA9 suffix",
         "prefix \xCE\xA9 suffix"},
        {"literal-cif", "literal \\U+0041 suffix",
         "literal \\U+0041 suffix", "literal A suffix",
         "literal \\U+0041 suffix", "literal A suffix",
         "literal \\U+005CU+0041 suffix"},
        {"lone-surrogate-escape", "literal \\U+D800 suffix",
         "literal \\U+D800 suffix", "literal \\U+D800 suffix",
         "literal \\U+D800 suffix", "literal \\U+005CU+D800 suffix",
         "literal \\U+005CU+D800 suffix"}
    };
    const auto expectedWire = [](DRW::Version version, const Row& row) {
        WireOracle oracle;
        if (version <= DRW::AC1018) {
            std::string payload = row.expectedTvPayload;
            payload.push_back('\0');
            oracle.bitShort(static_cast<std::uint16_t>(payload.size()));
            for (unsigned char value : payload)
                oracle.rawByte(value);
        } else {
            std::vector<std::uint16_t> units;
            for (std::size_t i = 0; i < row.source.size();) {
                const unsigned char first =
                    static_cast<unsigned char>(row.source[i]);
                if (first == 0xCEu && i + 1 < row.source.size()
                    && static_cast<unsigned char>(row.source[i + 1]) == 0xA9u) {
                    units.push_back(0x03A9u);
                    i += 2;
                } else if (first < 0x80u) {
                    units.push_back(first);
                    ++i;
                } else {
                    return std::vector<std::uint8_t>{};
                }
            }
            units.push_back(0);
            oracle.bitShort(static_cast<std::uint16_t>(units.size()));
            for (std::uint16_t unit : units) {
                oracle.rawByte(static_cast<std::uint8_t>(unit & 0xFFu));
                oracle.rawByte(static_cast<std::uint8_t>(unit >> 8));
            }
        }
        oracle.rawByte(0xA7u);
        return oracle.bytes;
    };

    std::size_t cases = 0;
    for (const Row& row : rows) {
        const SourceObservation tv = captureSource(DRW::AC1018, row.source);
        const SourceObservation tu = captureSource(DRW::AC1021, row.source);
        const bool sourcePaths = tv.valid && tu.valid
            && tv.sentinel == 0xA7u && tu.sentinel == 0xA7u
            && tv.typed == row.expectedTv && tu.typed == row.expectedTu
            && tv.wire == expectedWire(DRW::AC1018, row)
            && tu.wire == expectedWire(DRW::AC1021, row);
        t.expect(sourcePaths,
                 "generated DWG TV and TU paths retain typed value and following wire sentinel");

        const auto writeModernTarget = [](const std::string& value,
                                          std::string& bytes,
                                          std::string& firstRaw,
                                          std::string& firstTyped) {
            std::ostringstream output(std::ios::binary);
            dxfWriterAscii writer(&output);
            writer.setVersion("AC1027", true);
            writer.setTextWritePolicy(DRW::DxfTextWritePolicy::RequireLossless);
            const bool encoded = writer.writeUtf8String(1, value)
                && writer.writeUtf8String(2, "AFTER-TEXT");
            bytes = output.str();
            if (!encoded || writer.hasWriteError())
                return false;
            std::istringstream input(bytes, std::ios::binary);
            dxfReaderAscii reader(&input);
            reader.setVersion("AC1027", true);
            int code = 0;
            if (!reader.readRec(&code) || code != 1)
                return false;
            firstRaw = reader.getRawValue();
            firstTyped = reader.getUtf8String();
            return reader.readRec(&code) && code == 2
                && reader.getRawValue() == "AFTER-TEXT"
                && reader.getUtf8String() == "AFTER-TEXT";
        };

        std::string tvBytes, tvRaw, tvTyped;
        std::string tuBytes, tuRaw, tuTyped;
        const bool tvOutput = writeModernTarget(tv.typed, tvBytes, tvRaw, tvTyped);
        const bool tuOutput = writeModernTarget(tu.typed, tuBytes, tuRaw, tuTyped);
        const bool targetMatches = tvOutput && tuOutput
            && tvRaw == row.expectedTvDxfRaw && tvTyped == tv.typed
            && tuRaw == row.expectedTuDxfRaw && tuTyped == tu.typed
            && tvBytes.find(row.expectedTvDxfRaw) != std::string::npos
            && tuBytes.find(row.expectedTuDxfRaw) != std::string::npos;
        t.expect(targetMatches,
                 "modern DXF raw bytes and decoded values remain separately observable, with next group intact");
        if (!targetMatches) {
            std::cerr << "TV/TU target detail " << row.name
                      << ": tvOutput=" << tvOutput << " tuOutput=" << tuOutput
                      << " tvRaw=" << hexString(tvRaw)
                      << " tvTyped=" << hexString(tvTyped)
                      << " tuRaw=" << hexString(tuRaw)
                      << " tuTyped=" << hexString(tuTyped)
                      << " tvExpectedRaw=" << hexString(row.expectedTvDxfRaw)
                      << " tuExpectedRaw=" << hexString(row.expectedTuDxfRaw)
                      << " expectedTv=" << hexString(row.expectedTv)
                      << " expectedTu=" << hexString(row.expectedTu)
                      << '\n';
        }

        std::cout << "DWG TV/TU characterization " << row.name
                  << ": TV-wire=" << hex(tv.wire)
                  << " TV-typed=" << tv.typed
                  << " TU-wire=" << hex(tu.wire)
                  << " TU-typed=" << tu.typed
                  << " target=" << (tvOutput && tuOutput ? "PASS" : "FAIL")
                  << '\n';
        ++cases;
    }
    std::cout << "Generated DWG TV/TU → AC1027 DXF characterization: "
              << cases << " rows; observational only, not an escape-policy ruling\n";
}

void testDwgTvTuEntityIntegration(TestContext& t) {
    const auto nonce = std::chrono::steady_clock::now()
                           .time_since_epoch().count();
    struct Cleanup {
        std::vector<std::filesystem::path> paths;
        ~Cleanup() {
            std::error_code ignored;
            for (const auto& path : paths)
                std::filesystem::remove(path, ignored);
        }
    } cleanup;
    const std::string textSource = "TEXT \\U+0041 \xCE\xA9";
    const std::string mtextSource = "MTEXT \\U+0041 \xCE\xA9";
    struct VersionRow {
        DRW::Version version;
        const char* label;
        std::string expectedText;
        std::string expectedMText;
        std::string expectedTextRaw;
        std::string expectedMTextRaw;
        std::string expectedXRecord;
        std::string expectedXRecordRaw;
    };
    const VersionRow rows[] = {
        {DRW::AC1018, "AC1018-TV", "TEXT A \xCE\xA9", "MTEXT A \xCE\xA9",
         "TEXT A \xCE\xA9", "MTEXT A \xCE\xA9",
         "XRECORD A", "XRECORD A"},
        {DRW::AC1021, "AC1021-TU", textSource, mtextSource,
         "TEXT \\U+005CU+0041 \xCE\xA9",
         "MTEXT \\U+005CU+0041 \xCE\xA9",
         "XRECORD \\U+0041",
         "XRECORD \\U+005CU+0041"}
    };
    constexpr const char* xrecordRootName = "LOCAL_TV_TU_XRECORD";
    constexpr const char* xrecordDictionaryName = "LOCAL_TV_TU_TEXT";
    constexpr const char* xrecordFollowingValue = "AFTER-XRECORD";
    constexpr const char* xrecordSource = "XRECORD \\U+0041";

    for (const VersionRow& row : rows) {
        const std::string stem = "libdxfrw-tv-tu-entity-"
            + std::to_string(nonce) + "-"
            + std::to_string(static_cast<int>(row.version));
        const auto sourcePath = std::filesystem::temp_directory_path()
            / (stem + ".dwg");
        const auto targetPath = std::filesystem::temp_directory_path()
            / (stem + ".dxf");
        cleanup.paths.push_back(sourcePath);
        cleanup.paths.push_back(targetPath);

        dwgRW writer(sourcePath.string().c_str());
        class Source final : public dx_iface {
        public:
            explicit Source(dwgRW& writer) : writer(writer) {
                cData = &data;
                currentBlock = data.mBlock;
                text.height = 1.0;
                text.text = "TEXT \\U+0041 \xCE\xA9";
                mtext.height = 1.0;
                mtext.widthscale = 24.0;
                mtext.basePoint = {4.0, 5.0, 0.0};
                mtext.secPoint = {1.0, 0.0, 0.0};
                mtext.text = "MTEXT \\U+0041 \xCE\xA9";
                line.basePoint = {34.0, 35.0, 36.0};
                line.secPoint = {37.0, 38.0, 39.0};
            }
            void writeDwgClasses() override {
                xrecordDictionaryHandle = writer.allocNextHandle();
                xrecordHandle = writer.allocNextHandle();
                registeredXRecordDictionary = xrecordDictionaryHandle != 0
                    && xrecordHandle != 0
                    && writer.registerDwgNamedObjectDictionaryEntry(
                        "LOCAL_TV_TU_TEXT", xrecordDictionaryHandle);
            }
            void writeHeader(DRW_Header& header) override {
                header.addStr("$DWGCODEPAGE", "ANSI_1252", 3);
            }
            void writeEntities() override {
                wrote = writer.writeText(&text)
                    && writer.writeMText(&mtext)
                    && writer.writeLine(&line);
            }
            void writeBlocks() override {}
            void writeBlockRecords() override {}
            void writeLTypes() override {}
            void writeLayers() override {}
            void writeTextstyles() override {}
            void writeVports() override {}
            void writeDimstyles() override {}
            void writeAppId() override {}
            void writeObjects() override {
                if (!registeredXRecordDictionary)
                    return;
                DRW_Dictionary dictionary;
                dictionary.handle = xrecordDictionaryHandle;
                dictionary.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
                dictionary.cloning = 1;
                dictionary.hardOwner = 1;
                dictionary.m_entries.push_back(
                    {"LOCAL_TV_TU_XRECORD", xrecordHandle});
                wroteXRecordDictionary = writer.writeDictionary(&dictionary);
                if (!wroteXRecordDictionary)
                    return;
                DRW_XRecord record;
                record.handle = xrecordHandle;
                record.parentHandle = dictionary.handle;
                record.m_dataEntries.emplace_back(1, UTF8STRING(xrecordSource));
                record.m_dataEntries.emplace_back(
                    2, UTF8STRING(xrecordFollowingValue));
                xrecordRawReplayAdmitted =
                    record.canReplayDwgRawData(writer.getVersion());
                wroteXRecord = writer.writeXRecord(&record);
            }
            void writeViews() override {}
            void writeUCSs() override {}

            dx_data data;
            dwgRW& writer;
            DRW_Text text;
            DRW_MText mtext;
            DRW_Line line;
            std::uint32_t xrecordDictionaryHandle {0};
            std::uint32_t xrecordHandle {0};
            bool registeredXRecordDictionary {false};
            bool wroteXRecordDictionary {false};
            bool xrecordRawReplayAdmitted {false};
            bool wroteXRecord {false};
            bool wrote {false};
        } source(writer);
        const bool wrote = writer.write(&source, row.version, true)
            && source.wrote && source.wroteXRecordDictionary
            && source.wroteXRecord;
        t.expect(wrote,
                 "locally generated source DWG writes TEXT, MTEXT, LINE, and a rooted XRECORD");
        if (!wrote) {
            const auto skipped = writer.getWriteSkipCounters();
            std::cerr << "TV/TU source writer detail " << row.label
                      << ": registered=" << source.registeredXRecordDictionary
                      << " dictionary=" << source.wroteXRecordDictionary
                      << " rawAdmitted=" << source.xrecordRawReplayAdmitted
                      << " xrecord=" << source.wroteXRecord
                      << " entities=" << source.wrote
                      << " error=" << writer.getError()
                      << " skippedObjects=" << skipped.objectWrites << '\n';
        }
        if (!wrote)
            continue;

        dx_data imported;
        dx_iface importer;
        const bool readSource = importer.fileImport(sourcePath.string(),
                                                     &imported, false);
        std::string readText, readMText;
        std::string readXRecord;
        bool sourceXRecordRooted = false;
        if (imported.hasNamedObjectDictionary) {
            for (const DRW_Dictionary::Entry& entry :
                 imported.namedObjectDictionary.m_entries) {
                sourceXRecordRooted = sourceXRecordRooted
                    || (entry.m_name == xrecordDictionaryName
                        && entry.m_handle == source.xrecordDictionaryHandle);
            }
        }
        bool sourceXRecordOwned = false;
        for (const DRW_Dictionary& dictionary : imported.dictionaries) {
            if (dictionary.handle != source.xrecordDictionaryHandle
                || dictionary.parentHandle
                    != imported.namedObjectDictionary.handle)
                continue;
            for (const DRW_Dictionary::Entry& entry : dictionary.m_entries) {
                sourceXRecordOwned = sourceXRecordOwned
                    || (entry.m_name == xrecordRootName
                        && entry.m_handle == source.xrecordHandle);
            }
        }
        if (imported.xRecords.size() == 1u
            && imported.xRecords.front().handle == source.xrecordHandle
            && imported.xRecords.front().parentHandle
                == source.xrecordDictionaryHandle
            && imported.xRecords.front().m_dataEntries.size() == 2u
            && imported.xRecords.front().m_dataEntries[0].code() == 1
            && imported.xRecords.front().m_dataEntries[0].type()
                == DRW_Variant::STRING
            && imported.xRecords.front().m_dataEntries[0].content.s != nullptr
            && imported.xRecords.front().m_dataEntries[1].code() == 2
            && imported.xRecords.front().m_dataEntries[1].type()
                == DRW_Variant::STRING
            && imported.xRecords.front().m_dataEntries[1].content.s != nullptr
            && *imported.xRecords.front().m_dataEntries[1].content.s
                == xrecordFollowingValue)
            readXRecord = *imported.xRecords.front().m_dataEntries[0].content.s;
        bool readLine = false;
        if (imported.mBlock != nullptr) {
            for (const DRW_Entity* entity : imported.mBlock->ent) {
                if (entity->eType == DRW::MTEXT) {
                    const auto* value = dynamic_cast<const DRW_MText*>(entity);
                    if (value != nullptr)
                        readMText = value->text;
                } else if (entity->eType == DRW::TEXT) {
                    const auto* value = dynamic_cast<const DRW_Text*>(entity);
                    if (value != nullptr)
                        readText = value->text;
                }
                if (const auto* value = dynamic_cast<const DRW_Line*>(entity))
                    readLine = value->basePoint.x == 34.0
                        && value->basePoint.y == 35.0
                        && value->basePoint.z == 36.0
                        && value->secPoint.x == 37.0
                        && value->secPoint.y == 38.0
                        && value->secPoint.z == 39.0;
            }
        }
        const bool readExpected = readSource && imported.mBlock != nullptr
            && imported.mBlock->ent.size() == 3
            && readText == row.expectedText
            && readMText == row.expectedMText && readLine
            && sourceXRecordRooted && sourceXRecordOwned
            && readXRecord == row.expectedXRecord;
        t.expect(readExpected,
                 "whole generated source DWG exposes version-specific TEXT/MTEXT/XRECORD values, rooted ownership, and following sentinels");
        if (!readExpected) {
            std::cerr << "TV/TU entity source detail " << row.label
                      << ": read=" << readSource
                      << " count=" << (imported.mBlock == nullptr
                             ? 0u : imported.mBlock->ent.size())
                      << " text=[" << readText << "] mtext=[" << readMText
                      << "] line=" << readLine
                      << " xrecord=[" << readXRecord << "] root="
                      << sourceXRecordRooted << " owner=" << sourceXRecordOwned
                      << " expectedText=[" << row.expectedText
                      << "] expectedMtext=[" << row.expectedMText
                      << "] expectedXRecord=[" << row.expectedXRecord << "]\n";
            continue;
        }

        // Isolate the already-read XRECORD owner graph for the writer route.
        // The generated whole DWG also contains unrelated opaque view frames,
        // so its original read receipt correctly does not qualify as a strict-
        // complete conversion input. This projection tests only the typed
        // value and its rooted dictionary→XRECORD graph.
        dx_data xrecordProjection;
        xrecordProjection.sourceFormat = dx_source_format::DWG;
        xrecordProjection.sourceVersion = row.version;
        xrecordProjection.hasNamedObjectDictionary = true;
        xrecordProjection.namedObjectDictionary = imported.namedObjectDictionary;
        xrecordProjection.namedObjectDictionary.m_entries.erase(
            std::remove_if(
                xrecordProjection.namedObjectDictionary.m_entries.begin(),
                xrecordProjection.namedObjectDictionary.m_entries.end(),
                [](const DRW_Dictionary::Entry& entry) {
                    return entry.m_name != xrecordDictionaryName;
                }),
            xrecordProjection.namedObjectDictionary.m_entries.end());
        for (const DRW_Dictionary& dictionary : imported.dictionaries) {
            if (dictionary.handle == source.xrecordDictionaryHandle)
                xrecordProjection.dictionaries.push_back(dictionary);
        }
        for (const DRW_XRecord& record : imported.xRecords) {
            if (record.handle == source.xrecordHandle)
                xrecordProjection.xRecords.push_back(record);
        }

        dx_iface exporter;
        const bool exported = exporter.fileExport(
            targetPath.string(), DRW::AC1027, false, &imported, false, false,
            dx_dxfSourceTextPolicy::RequireValidSource,
            DRW::DxfTextWritePolicy::RequireLossless);
        t.expect(exported,
                 "generated DWG text entities export to strict AC1027 ASCII DXF");
        if (!exported)
            continue;

        std::ifstream targetInput(targetPath, std::ios::binary);
        const std::string targetBytes(
            (std::istreambuf_iterator<char>(targetInput)),
            std::istreambuf_iterator<char>());
        std::istringstream physicalInput(targetBytes);
        std::string currentEntity;
        std::string targetTextRaw, targetMTextRaw;
        std::string targetXRecordRaw;
        std::string targetXRecordFollowingRaw;
        std::vector<std::string> entityOrder;
        bool targetEof = false;
        bool sawAcadver = false;
        bool expectAcadverValue = false;
        double lineX = 0.0, lineY = 0.0, lineZ = 0.0;
        double endX = 0.0, endY = 0.0, endZ = 0.0;
        std::string codeLine, valueLine;
        while (std::getline(physicalInput, codeLine)
               && std::getline(physicalInput, valueLine)) {
            if (!codeLine.empty() && codeLine.back() == '\r')
                codeLine.pop_back();
            if (!valueLine.empty() && valueLine.back() == '\r')
                valueLine.pop_back();
            const auto first = codeLine.find_first_not_of(" \t");
            const auto last = codeLine.find_last_not_of(" \t");
            if (first == std::string::npos)
                break;
            const std::string_view codeText(codeLine.data() + first,
                                             last - first + 1u);
            int code = 0;
            const auto parsed = std::from_chars(codeText.data(),
                codeText.data() + codeText.size(), code);
            if (parsed.ec != std::errc{}
                || parsed.ptr != codeText.data() + codeText.size())
                break;

            if (expectAcadverValue) {
                sawAcadver = code == 1 && valueLine == "AC1027";
                expectAcadverValue = false;
            }
            if (code == 9 && valueLine == "$ACADVER")
                expectAcadverValue = true;
            if (code == 0) {
                currentEntity = valueLine;
                if (currentEntity == "EOF") {
                    targetEof = true;
                    break;
                }
                if (currentEntity == "TEXT" || currentEntity == "MTEXT"
                    || currentEntity == "LINE")
                    entityOrder.push_back(currentEntity);
            } else if (currentEntity == "TEXT" && code == 1) {
                targetTextRaw = valueLine;
            } else if (currentEntity == "MTEXT" && (code == 3 || code == 1)) {
                targetMTextRaw += valueLine;
            } else if (currentEntity == "XRECORD" && code == 1) {
                targetXRecordRaw = valueLine;
            } else if (currentEntity == "XRECORD" && code == 2) {
                targetXRecordFollowingRaw = valueLine;
            } else if (currentEntity == "LINE") {
                const auto numeric = [&valueLine]() {
                    try {
                        return std::stod(valueLine);
                    } catch (...) {
                        return std::numeric_limits<double>::quiet_NaN();
                    }
                };
                if (code == 10) lineX = numeric();
                if (code == 20) lineY = numeric();
                if (code == 30) lineZ = numeric();
                if (code == 11) endX = numeric();
                if (code == 21) endY = numeric();
                if (code == 31) endZ = numeric();
            }
        }
        dx_data targetData;
        dx_iface targetImporter;
        const bool targetRead = targetImporter.fileImport(
            targetPath.string(), &targetData, false);
        std::string targetText, targetMText;
        std::string targetXRecord;
        bool targetXRecordGraph = false;
        std::uint32_t targetXRecordDictionaryHandle = 0;
        if (targetData.hasNamedObjectDictionary) {
            for (const DRW_Dictionary::Entry& entry :
                 targetData.namedObjectDictionary.m_entries) {
                if (entry.m_name == xrecordDictionaryName)
                    targetXRecordDictionaryHandle = entry.m_handle;
            }
        }
        std::uint32_t targetXRecordHandle = 0;
        for (const DRW_Dictionary& dictionary : targetData.dictionaries) {
            if (dictionary.handle != targetXRecordDictionaryHandle
                || dictionary.parentHandle
                    != targetData.namedObjectDictionary.handle)
                continue;
            for (const DRW_Dictionary::Entry& entry : dictionary.m_entries) {
                if (entry.m_name == xrecordRootName)
                    targetXRecordHandle = entry.m_handle;
            }
        }
        if (targetData.xRecords.size() == 1u
            && targetData.xRecords.front().handle == targetXRecordHandle
            && targetData.xRecords.front().handle != 0
            && targetData.xRecords.front().parentHandle
                == targetXRecordDictionaryHandle
            && targetData.xRecords.front().m_dataEntries.size() == 2u
            && targetData.xRecords.front().m_dataEntries[0].code() == 1
            && targetData.xRecords.front().m_dataEntries[0].type()
                == DRW_Variant::STRING
            && targetData.xRecords.front().m_dataEntries[0].content.s != nullptr
            && targetData.xRecords.front().m_dataEntries[1].code() == 2
            && targetData.xRecords.front().m_dataEntries[1].type()
                == DRW_Variant::STRING
            && targetData.xRecords.front().m_dataEntries[1].content.s != nullptr
            && *targetData.xRecords.front().m_dataEntries[1].content.s
                == xrecordFollowingValue) {
            targetXRecord = *targetData.xRecords.front().m_dataEntries[0].content.s;
            targetXRecordGraph = true;
        }
        bool targetLine = false;
        if (targetData.mBlock != nullptr) {
            for (const DRW_Entity* entity : targetData.mBlock->ent) {
                if (entity->eType == DRW::MTEXT) {
                    const auto* value = dynamic_cast<const DRW_MText*>(entity);
                    if (value != nullptr)
                        targetMText = value->text;
                } else if (entity->eType == DRW::TEXT) {
                    const auto* value = dynamic_cast<const DRW_Text*>(entity);
                    if (value != nullptr)
                        targetText = value->text;
                }
                if (const auto* value = dynamic_cast<const DRW_Line*>(entity))
                    targetLine = value->basePoint.x == 34.0
                        && value->basePoint.y == 35.0
                        && value->basePoint.z == 36.0
                        && value->secPoint.x == 37.0
                        && value->secPoint.y == 38.0
                        && value->secPoint.z == 39.0;
            }
        }
        const bool expectedOrder = entityOrder.size() >= 3
            && entityOrder[0] == "TEXT" && entityOrder[1] == "MTEXT"
            && entityOrder[2] == "LINE";
        const bool expectedRawAndSemantic = targetTextRaw == row.expectedTextRaw
            && targetMTextRaw == row.expectedMTextRaw
            && targetRead && targetData.mBlock != nullptr
            && targetText == row.expectedText && targetMText == row.expectedMText
            && targetLine && targetData.mBlock->ent.size() == 3;
        const bool targetComplete = targetEof
            && sawAcadver && validateAsciiDxfStructure(targetBytes)
            && expectedOrder && expectedRawAndSemantic
            && lineX == 34.0 && lineY == 35.0 && lineZ == 36.0
            && endX == 37.0 && endY == 38.0 && endZ == 39.0;
        t.expect(targetComplete,
                 "target DXF contains exact physical text, typed values, AC1027 header, EOF, and following LINE geometry");
        if (!targetComplete) {
            std::cerr << "TV/TU entity target detail " << row.label
                      << ": eof=" << targetEof
                      << " version=" << sawAcadver << " targetRead=" << targetRead
                      << " order=" << expectedOrder
                      << " textRaw=[" << targetTextRaw << "] text=[" << targetText
                      << "] mtextRaw=[" << targetMTextRaw << "] mtext=[" << targetMText
                      << "] line=" << lineX << ',' << lineY << ',' << lineZ
                      << " -> " << endX << ',' << endY << ',' << endZ
                      << " lineRead=" << targetLine
                      << " xrecordRaw=[" << targetXRecordRaw << "] xrecord=["
                      << targetXRecord << "] xrecordGraph=" << targetXRecordGraph
                      << " expectedTextRaw=[" << row.expectedTextRaw
                      << "] expectedMtextRaw=[" << row.expectedMTextRaw
                      << "] expectedXRecordRaw=[" << row.expectedXRecordRaw
                      << "] bytes=" << targetBytes.size() << '\n';
        }
        std::cout << "Generated DWG entity TV/TU integration " << row.label
                  << ": read=" << readExpected << " export=" << exported
                  << " output=" << (targetComplete ? "PASS" : "FAIL") << '\n';

        const auto verifyXRecordTarget = [&](bool binary) {
            const std::string suffix = binary ? "-xrecord-binary.dxf"
                                              : "-xrecord-ascii.dxf";
            const auto outputPath = targetPath.parent_path()
                / (targetPath.stem().string() + suffix);
            cleanup.paths.push_back(outputPath);
            dx_iface xrecordExporter;
            const bool written = xrecordExporter.fileExport(
                outputPath.string(), DRW::AC1027, binary, &xrecordProjection,
                false, true, dx_dxfSourceTextPolicy::Compatible,
                DRW::DxfTextWritePolicy::RequireLossless);

            std::string rawValue, rawFollowing;
            bool terminalEof = false;
            bool targetVersion = false;
            bool binaryHeaderValid = !binary;
            if (written) {
                std::ifstream file(outputPath, std::ios::binary);
                const std::string bytes(
                    (std::istreambuf_iterator<char>(file)),
                    std::istreambuf_iterator<char>());
                std::istringstream input(bytes, std::ios::binary);
                if (binary) {
                    const std::string binaryMagic(
                        "AutoCAD Binary DXF\r\n\x1a\0", 22u);
                    binaryHeaderValid = bytes.size() >= binaryMagic.size()
                        && bytes.compare(0, binaryMagic.size(), binaryMagic) == 0;
                    if (binaryHeaderValid)
                        input.seekg(static_cast<std::streamoff>(binaryMagic.size()),
                                    std::ios::beg);
                }
                std::unique_ptr<dxfReader> reader = binary
                    ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
                    : std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
                reader->setVersion("AC1027", true);
                int code = 0;
                std::string objectName;
                bool expectVersion = false;
                while (reader->readRec(&code)) {
                    if (expectVersion) {
                        targetVersion = code == 1
                            && reader->getRawValue() == "AC1027";
                        expectVersion = false;
                    }
                    if (code == 9 && reader->getRawValue() == "$ACADVER")
                        expectVersion = true;
                    if (code == 0) {
                        objectName = reader->getString();
                        if (objectName == "EOF") {
                            const std::streampos eofPosition = input.tellg();
                            terminalEof = eofPosition != std::streampos(-1)
                                && static_cast<std::streamoff>(eofPosition)
                                    == static_cast<std::streamoff>(bytes.size());
                            break;
                        }
                    } else if (objectName == "XRECORD" && code == 1) {
                        rawValue = reader->getRawValue();
                    } else if (objectName == "XRECORD" && code == 2) {
                        rawFollowing = reader->getRawValue();
                    }
                }
                terminalEof = terminalEof && binaryHeaderValid;
                if (!binary)
                    targetVersion = targetVersion
                        && validateAsciiDxfStructure(bytes);
            }

            dx_data reopened;
            dx_iface xrecordImporter;
            const bool readBack = written && xrecordImporter.fileImport(
                outputPath.string(), &reopened, false);
            std::string typedValue;
            std::uint32_t emittedDictionaryHandle = 0;
            if (reopened.hasNamedObjectDictionary) {
                for (const DRW_Dictionary::Entry& entry :
                     reopened.namedObjectDictionary.m_entries) {
                    if (entry.m_name == xrecordDictionaryName)
                        emittedDictionaryHandle = entry.m_handle;
                }
            }
            std::uint32_t emittedRecordHandle = 0;
            for (const DRW_Dictionary& dictionary : reopened.dictionaries) {
                if (dictionary.handle != emittedDictionaryHandle
                    || dictionary.parentHandle
                        != reopened.namedObjectDictionary.handle)
                    continue;
                for (const DRW_Dictionary::Entry& entry : dictionary.m_entries) {
                    if (entry.m_name == xrecordRootName)
                        emittedRecordHandle = entry.m_handle;
                }
            }
            bool graphClosed = false;
            if (reopened.xRecords.size() == 1u
                && reopened.xRecords.front().handle == emittedRecordHandle
                && emittedRecordHandle != 0
                && reopened.xRecords.front().parentHandle
                    == emittedDictionaryHandle
                && reopened.xRecords.front().m_dataEntries.size() == 2u
                && reopened.xRecords.front().m_dataEntries[0].code() == 1
                && reopened.xRecords.front().m_dataEntries[0].type()
                    == DRW_Variant::STRING
                && reopened.xRecords.front().m_dataEntries[0].content.s != nullptr
                && reopened.xRecords.front().m_dataEntries[1].code() == 2
                && reopened.xRecords.front().m_dataEntries[1].type()
                    == DRW_Variant::STRING
                && reopened.xRecords.front().m_dataEntries[1].content.s != nullptr
                && *reopened.xRecords.front().m_dataEntries[1].content.s
                    == xrecordFollowingValue) {
                typedValue =
                    *reopened.xRecords.front().m_dataEntries[0].content.s;
                graphClosed = true;
            }
            const bool complete = readBack && terminalEof && targetVersion
                && rawValue == row.expectedXRecordRaw
                && rawFollowing == xrecordFollowingValue
                && typedValue == row.expectedXRecord && graphClosed;
            t.expect(complete,
                     binary
                         ? "generated TV/TU XRECORD retains exact AC1027 Binary DXF bytes, semantic value, owner graph, and following value"
                         : "generated TV/TU XRECORD retains exact AC1027 ASCII DXF bytes, semantic value, owner graph, and following value");
            if (!complete) {
                std::cerr << "TV/TU XRECORD target detail " << row.label
                          << (binary ? " Binary" : " ASCII")
                          << ": written=" << written << " read=" << readBack
                          << " eof=" << terminalEof << " version=" << targetVersion
                          << " raw=[" << rawValue << "] typed=[" << typedValue
                          << "] graph=" << graphClosed
                          << " following=[" << rawFollowing << "]\n";
            }
            return complete;
        };
        const bool xrecordAsciiComplete = verifyXRecordTarget(false);
        const bool xrecordBinaryComplete = verifyXRecordTarget(true);
        std::cout << "Generated DWG XRECORD TV/TU integration " << row.label
                  << ": ASCII=" << xrecordAsciiComplete
                  << " Binary=" << xrecordBinaryComplete << '\n';
    }
}

void verifyTargetDxfControl(TestContext& t, const std::filesystem::path& path) {
    dx_data data; dx_iface reader;
    const bool read = reader.fileImport(path.string(), &data, false);
    bool text = false, line = false;
    for (const auto* entity : data.mBlock->ent) {
        const auto* value = dynamic_cast<const DRW_Text*>(entity);
        if (value != nullptr) text = value->text == "DWG \xF0\x9F\x98\x80 literal\\U+0041";
        const auto* geometry = dynamic_cast<const DRW_Line*>(entity);
        if (geometry != nullptr && entity->eType == DRW::LINE)
            line = geometry->basePoint.x == 34 && geometry->basePoint.y == 35 && geometry->basePoint.z == 36
                && geometry->secPoint.x == 37 && geometry->secPoint.y == 38 && geometry->secPoint.z == 39;
    }
    t.expect(read && text && line && data.mBlock->ent.size() == 2,
             "DWG-derived DXF retains exact supplementary/literal semantic text and a distinct following LINE");
    std::cout << "Target DXF witness: " << (t.failures == 0 ? "PASS" : "FAIL") << '\n';
}

void testMalformedUtf8(TestContext& t) {
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1015, false);
    codec.setCodePage("ANSI_936", false);

    t.expect(codec.fromUtf8(makeMalformedBytes({0xE4, 0x41})) == "?A",
             "truncated UTF-8 emits one replacement and preserves ASCII");
    t.expect(codec.fromUtf8(makeMalformedBytes({0xE4, 0xB8, 0x41}))
                 == "??A",
             "invalid continuation does not swallow the next character");
    t.expect(codec.fromUtf8(makeMalformedBytes({0xC0, 0xAF, 0x42}))
                 == "??B",
             "overlong UTF-8 is rejected byte-by-byte");
    t.expect(codec.fromUtf8(makeMalformedBytes({0xED, 0xA0, 0x80, 0x43}))
                 == "???C",
             "UTF-8 surrogate encoding is rejected byte-by-byte");
    t.expect(codec.fromUtf8(makeMalformedBytes({0xF4, 0x90, 0x80, 0x80,
                                                 0x44}))
                 == "????D",
             "out-of-range UTF-8 is rejected byte-by-byte");
}

void testReaderSourceState(TestContext& t) {
    std::vector<std::uint8_t> storage(64, 0);
    dwgReader21 reader(std::make_unique<dwgBuffer>(storage.data(), storage.size()),
                       nullptr);
    DwgCodePageTestAccess::setVersion(reader, DRW::AC1021);
    t.expect(DwgCodePageTestAccess::record(reader, 31, true, false),
             "AC1021 source codepage records successfully");
    t.expect(DwgCodePageTestAccess::hasField(reader)
                 && DwgCodePageTestAccess::recognized(reader)
                 && DwgCodePageTestAccess::sourceName(reader) == "ANSI_936"
                 && DwgCodePageTestAccess::sourceId(reader) == 31,
             "AC1021 preserves source id and canonical name");
    t.expect(DwgCodePageTestAccess::primaryCodePage(reader) == "UTF-16",
             "AC1021 keeps UTF-16 as primary codec");
    t.expect(DwgCodePageTestAccess::decodeByteText(
                 reader, makeMalformedBytes({0xD6, 0xD0})) == "中",
             "AC1021 decodes byte fields through secondary codec");

    t.expect(DwgCodePageTestAccess::record(reader, 999, true, false),
             "unknown source codepage is non-fatal");
    t.expect(DwgCodePageTestAccess::hasField(reader)
                 && !DwgCodePageTestAccess::recognized(reader)
                 && DwgCodePageTestAccess::sourceName(reader).empty()
                 && DwgCodePageTestAccess::sourceId(reader) == 999
                 && DwgCodePageTestAccess::primaryCodePage(reader) == "UTF-16",
             "unknown id retains diagnostics without changing UTF-16");
    t.expect(DwgCodePageTestAccess::decodeByteText(
                 reader, makeMalformedBytes({0xD6, 0xD0}))
                 == makeMalformedBytes({0xD6, 0xD0}),
             "unknown id clears the previous byte-codec claim");

    t.expect(DwgCodePageTestAccess::record(reader, 0, false, false),
             "absent source codepage is non-fatal");
    t.expect(!DwgCodePageTestAccess::hasField(reader)
                 && DwgCodePageTestAccess::sourceName(reader).empty(),
             "absent source codepage has no claim");

    dwgReader18 legacy(std::make_unique<dwgBuffer>(storage.data(), storage.size()),
                       nullptr);
    DwgCodePageTestAccess::setVersion(legacy, DRW::AC1018);
    t.expect(DwgCodePageTestAccess::record(legacy, 39, true, true),
             "AC1018 source codepage records successfully");
    t.expect(DwgCodePageTestAccess::primaryCodePage(legacy) == "ANSI_936"
                 && DwgCodePageTestAccess::decodeByteText(
                        legacy, makeMalformedBytes({0xD6, 0xD0})) == "中",
             "AC1018 uses CP936 as primary byte codec");
    t.expect(DwgCodePageTestAccess::record(legacy, 999, true, true),
             "AC1018 unknown source codepage is non-fatal");
    t.expect(DwgCodePageTestAccess::primaryCodePage(legacy) == "ANSI_1252"
                 && DwgCodePageTestAccess::decodeByteText(
                        legacy, makeMalformedBytes({0xD6, 0xD0}))
                        == makeMalformedBytes({0xD6, 0xD0}),
             "AC1018 unknown id restores conservative byte decoding");

    dwgReader15 r15(std::make_unique<dwgBuffer>(storage.data(), storage.size()),
                    nullptr);
    DwgCodePageTestAccess::setVersion(r15, DRW::AC1015);
    t.expect(DwgCodePageTestAccess::record(r15, 31, true, true)
                 && DwgCodePageTestAccess::primaryCodePage(r15) == "ANSI_936"
                 && DwgCodePageTestAccess::sourceName(r15) == "ANSI_936",
             "AC1015 uses the shared CP936 source-state route");
    t.expect(DwgCodePageTestAccess::record(r15, 0, false, true)
                 && !DwgCodePageTestAccess::recognized(r15)
                 && DwgCodePageTestAccess::primaryCodePage(r15) == "ANSI_1252",
             "AC1015 absent codepage resets to the conservative default");

    auto put16 = [](std::vector<std::uint8_t>& bytes, std::size_t offset,
                    std::uint16_t value) {
        bytes[offset] = static_cast<std::uint8_t>(value & 0xffu);
        bytes[offset + 1] = static_cast<std::uint8_t>(value >> 8u);
    };
    auto put32 = [](std::vector<std::uint8_t>& bytes, std::size_t offset,
                    std::uint32_t value) {
        for (unsigned shift = 0; shift < 32; shift += 8)
            bytes[offset + shift / 8] =
                static_cast<std::uint8_t>((value >> shift) & 0xffu);
    };
    auto checkPreR13CodePage = [&](const char* magic,
                                   bool expectSourceClaim) {
        std::vector<std::uint8_t> preR13(0x600, 0);
        for (std::size_t i = 0; i < 6; ++i)
            preR13[i] = static_cast<std::uint8_t>(magic[i]);
        put16(preR13, 0x11, 130);
        put32(preR13, 0x14, 0x500);
        put32(preR13, 0x18, 0x500);
        put16(preR13, 0x3f9, 31);
        dwgReaderR11 preReader(
            std::make_unique<dwgBuffer>(preR13.data(), preR13.size()), nullptr);
        const bool metadataOk = preReader.readMetaData();
        const bool headerOk = metadataOk && preReader.readFileHeader();
        t.expect(headerOk
                     && DwgCodePageTestAccess::recognized(preReader)
                            == expectSourceClaim,
                 expectSourceClaim
                     ? "AC1009 publishes its validated fixed codepage"
                     : "pre-R10 fixed-header codepage remains non-promoting");
        if (headerOk && expectSourceClaim)
            t.expect(DwgCodePageTestAccess::sourceName(preReader) == "ANSI_936",
                     "AC1009 fixed codepage resolves through the shared table");
    };
    checkPreR13CodePage("AC1009", true);
    checkPreR13CodePage("AC1003", false);
}

void testDxfWriters(TestContext& t) {
    std::ostringstream ascii;
    dxfWriterAscii asciiWriter(&ascii);
    asciiWriter.setCodePage("ANSI_936");
    t.expect(asciiWriter.writeUtf8String(1, "中文A"),
             "ASCII writer encodes CP936 text");
    t.expect(ascii.str().find(makeMalformedBytes({0xD6, 0xD0, 0xCE, 0xC4,
                                                   0x41})) != std::string::npos,
             "ASCII writer emits exact CP936 bytes");
    t.expect(!asciiWriter.writeUtf8String(1, std::string("A\0B", 3))
                 && asciiWriter.hasWriteError(),
             "ASCII writer rejects embedded NUL");

    std::ostringstream binary(std::ios::binary);
    dxfWriterBinary binaryWriter(&binary);
    binaryWriter.setCodePage("ANSI_936");
    t.expect(binaryWriter.writeUtf8String(1, "中文A"),
             "binary writer encodes CP936 text");
    t.expect(binary.str().find(makeMalformedBytes({0xD6, 0xD0, 0xCE, 0xC4,
                                                    0x41, 0x00}))
                 != std::string::npos,
             "binary writer emits exact CP936 bytes and terminator");
    t.expect(!binaryWriter.writeUtf8String(1, std::string("A\0B", 3))
                 && binaryWriter.hasWriteError(),
             "binary writer rejects embedded NUL");

    DRW_Header header;
    header.addStr("$DWGCODEPAGE", "ANSI_1252", 3);
    header.addStr("$DWGCODEPAGE", "ANSI_936", 3);
    t.expect(headerString(header, "$DWGCODEPAGE") == "ANSI_936",
             "duplicate DWG codepage header is replaced deterministically");
}

bool writeCodepageMLineSeed(const std::filesystem::path& path) {
    std::ofstream output(path, std::ios::binary);
    if (!output)
        return false;

    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1018, true);
    codec.setCodePage("ANSI_936", false);
    const std::string encodedStyle = codec.fromUtf8("中文线型");
    if (encodedStyle.empty())
        return false;

    dxfWriterAscii writer(&output);
    writer.setVersion("AC1018", true);
    writer.writeString(0, "SECTION");
    writer.writeString(2, "HEADER");
    writer.writeString(9, "$ACADVER");
    writer.writeString(1, "AC1018");
    writer.writeString(9, "$DWGCODEPAGE");
    writer.writeString(3, "ANSI_936");
    writer.writeString(9, "$CMLSTYLE");
    writer.writeString(2, encodedStyle);
    writer.writeString(0, "ENDSEC");
    writer.writeString(0, "SECTION");
    writer.writeString(2, "TABLES");
    writer.writeString(0, "TABLE");
    writer.writeString(2, "BLOCK_RECORD");
    writer.writeString(5, "20");
    writer.writeString(330, "0");
    writer.writeString(100, "AcDbSymbolTable");
    writer.writeInt16(70, 2);
    for (const auto& record : std::array<std::pair<const char*, const char*>, 2>{{
             {"1F", "*Model_Space"}, {"1E", "*Paper_Space"}}}) {
        writer.writeString(0, "BLOCK_RECORD");
        writer.writeString(5, record.first);
        writer.writeString(330, "20");
        writer.writeString(100, "AcDbSymbolTableRecord");
        writer.writeString(100, "AcDbBlockTableRecord");
        writer.writeString(2, record.second);
        writer.writeInt16(70, 0);
    }
    writer.writeString(0, "ENDTAB");
    writer.writeString(0, "ENDSEC");
    writer.writeString(0, "SECTION");
    writer.writeString(2, "ENTITIES");
    writer.writeString(0, "MLINE");
    writer.writeString(5, "40");
    writer.writeString(330, "1F");
    writer.writeString(100, "AcDbEntity");
    writer.writeString(8, "0");
    writer.writeString(100, "AcDbMline");
    writer.writeString(2, encodedStyle);
    writer.writeString(340, "1C");
    writer.writeDouble(40, 1.0);
    writer.writeInt16(70, 1);
    writer.writeInt16(71, 1);
    writer.writeInt16(72, 0);
    writer.writeInt16(73, 1);
    writer.writeDouble(10, 10.0);
    writer.writeDouble(20, 11.0);
    writer.writeDouble(30, 0.0);
    writer.writeString(0, "ENDSEC");
    writer.writeString(0, "SECTION");
    writer.writeString(2, "OBJECTS");
    writer.writeString(0, "DICTIONARY");
    writer.writeString(5, "C");
    writer.writeString(330, "0");
    writer.writeString(100, "AcDbDictionary");
    writer.writeInt16(280, 1);
    writer.writeInt16(281, 1);
    writer.writeString(3, "ACAD_MLINESTYLE");
    writer.writeString(350, "A");
    writer.writeString(0, "DICTIONARY");
    writer.writeString(5, "A");
    writer.writeString(330, "C");
    writer.writeString(100, "AcDbDictionary");
    writer.writeInt16(280, 1);
    writer.writeInt16(281, 1);
    writer.writeString(3, encodedStyle);
    writer.writeString(350, "1C");
    writer.writeString(0, "MLINESTYLE");
    writer.writeString(5, "1C");
    writer.writeString(330, "A");
    writer.writeString(100, "AcDbMlineStyle");
    writer.writeString(2, encodedStyle);
    writer.writeInt16(70, 0);
    writer.writeString(3, "generated codepage MLINESTYLE");
    writer.writeInt16(62, 256);
    writer.writeDouble(51, 0.0);
    writer.writeDouble(52, 0.0);
    writer.writeInt16(71, 1);
    writer.writeDouble(49, 0.5);
    writer.writeInt16(62, 0);
    writer.writeString(6, "CONTINUOUS");
    writer.writeString(0, "ENDSEC");
    writer.writeString(0, "EOF");
    return !writer.hasWriteError() && output.good();
}

void testDxfModelRoundTrip(TestContext& t) {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch()
                       .count();
    for (const bool binary : {false, true}) {
        const std::filesystem::path path =
            std::filesystem::temp_directory_path()
            / (std::string("libdxfrw-gbk-roundtrip-")
               + std::to_string(stamp) + (binary ? "-bin" : "-ascii")
               + ".dxf");
        std::error_code ec;
        std::filesystem::remove(path, ec);

        const std::filesystem::path seedPath =
            std::filesystem::temp_directory_path()
            / ("libdxfrw-gbk-mline-seed-" + std::to_string(stamp) + ".dxf");
        struct SeedCleanup {
            std::filesystem::path path;
            ~SeedCleanup() {
                std::error_code ignored;
                std::filesystem::remove(path, ignored);
            }
        } seedCleanup{seedPath};
        dx_data source;
        dx_iface seedReader;
        if (!writeCodepageMLineSeed(seedPath)
            || !seedReader.fileImport(seedPath.string(), &source, false)) {
            t.expect(false, "generated rooted CP936 MLINESTYLE imports");
            std::filesystem::remove(path, ec);
            continue;
        }
        t.expect(source.hasSingleSuccessfulNamedResourceImport()
                     && source.mLineStyles.size() == 1u
                     && source.mLineStyles.front().name == "中文线型",
                 "generated MLINESTYLE source has a valid rooted CP936 identity");
        DRW_Layer layer;
        layer.name = "中文层";
        source.layers.push_back(layer);
        DRW_AppId appId;
        appId.name = "GBKAPP";
        source.appIds.push_back(appId);
        auto* text = new DRW_Text();
        text->layer = layer.name;
        text->basePoint = DRW_Coord(1.0, 2.0, 0.0);
        text->height = 1.0;
        text->text = "中文A";
        text->style = "中文样式";
        text->colorName = "中文色";
        text->extData.push_back(std::make_shared<DRW_Variant>(
            1001, std::string("GBKAPP")));
        text->extData.push_back(std::make_shared<DRW_Variant>(
            1000, std::string("附加")));
        text->extData.push_back(std::make_shared<DRW_Variant>(
            1004, std::vector<std::uint8_t>{0xDE, 0xAD, 0xBE, 0xEF}));
        source.mBlock->ent.push_back(text);
        auto* dimension = new DRW_DimLinear();
        // Regression for issue #78: callers used the public entity enum in
        // the code-70 field.  The writer must derive the subtype from the
        // concrete DRW_DimLinear object instead of emitting that enum's low
        // nibble as a diametric dimension.
        dimension->type = DRW::DIMENSION;
        dimension->layer = layer.name;
        dimension->setDefPoint(DRW_Coord(2.0, 3.0, 0.0));
        dimension->setTextPoint(DRW_Coord(4.0, 5.0, 0.0));
        dimension->setText("中文尺寸");
        // The writer side is asserted below.  The matching reader assertion
        // remains evidence-gated with the frozen drw_entities.cpp contract.
        const std::string dimensionBlockName = "中文块";
        dimension->setName(dimensionBlockName);
        source.mBlock->ent.push_back(dimension);

        auto* mline = dynamic_cast<DRW_MLine*>(source.mBlock->ent.front());
        t.expect(mline != nullptr,
                 "generated rooted CP936 source retains its MLINE entity");
        if (mline == nullptr) {
            std::filesystem::remove(path, ec);
            continue;
        }
        mline->layer = layer.name;
        mline->scale = 1.0;
        mline->justification = 0;
        mline->basePoint = DRW_Coord(10.0, 11.0, 0.0);
        mline->extPoint = DRW_Coord(0.0, 0.0, 1.0);
        mline->openClosed = 1;
        mline->numLines = 1;
        mline->numVerts = 1;
        mline->styleName = "中文线型";
        mline->vertlist.clear();
        DRW_MLineVertex mlineVertex;
        mlineVertex.position = DRW_Coord(12.0, 13.0, 0.0);
        mlineVertex.vertexDir = DRW_Coord(1.0, 0.0, 0.0);
        mlineVertex.miterDir = DRW_Coord(0.0, 1.0, 0.0);
        mlineVertex.segParms = {{0.5}};
        mlineVertex.areaFillParms = {{0.25}};
        mline->vertlist.push_back(mlineVertex);

        auto* hatch = new DRW_Hatch();
        hatch->layer = layer.name;
        hatch->name = "中文图案";
        hatch->solid = 1;
        auto hatchLoop = std::make_shared<DRW_HatchLoop>(2);
        auto hatchBoundary = std::make_shared<DRW_LWPolyline>();
        hatchBoundary->flags = 1;
        hatchBoundary->addVertex(DRW_Vertex2D(6.0, 7.0, 0.0));
        hatchBoundary->addVertex(DRW_Vertex2D(8.0, 7.0, 0.0));
        hatchBoundary->addVertex(DRW_Vertex2D(8.0, 9.0, 0.0));
        hatchBoundary->addVertex(DRW_Vertex2D(6.0, 9.0, 0.0));
        hatchLoop->objlist.push_back(hatchBoundary);
        hatchLoop->update();
        hatch->appendLoop(hatchLoop);
        source.mBlock->ent.push_back(hatch);

        PreparedDxInterface exporter;
        exporter.attach(&source);
        const bool writeOk = exporter.fileExport(path.string(), DRW::AC1018,
                                                  binary, &source, false);
        t.expect(writeOk, binary ? "binary CP936 model export succeeds"
                                 : "ASCII CP936 model export succeeds");
        if (!writeOk) {
            std::filesystem::remove(path, ec);
            continue;
        }

        std::ifstream raw(path, std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(raw)),
                                std::istreambuf_iterator<char>());
        const std::string cp936 = makeMalformedBytes({0xD6, 0xD0, 0xCE, 0xC4});
        t.expect(bytes.find(cp936) != std::string::npos,
                 binary ? "binary output contains CP936 bytes"
                        : "ASCII output contains CP936 bytes");
        DRW_TextCodec outputCodec;
        outputCodec.setCodePage("ANSI_936", false);
        const std::string encodedDimensionBlockName =
            outputCodec.fromUtf8(dimensionBlockName);
        t.expect(bytes.find(encodedDimensionBlockName) != std::string::npos,
                 binary ? "binary output encodes dimension block name as CP936"
                        : "ASCII output encodes dimension block name as CP936");
        t.expect(bytes.find(dimensionBlockName) == std::string::npos,
                 binary ? "binary output does not leave dimension block name as UTF-8"
                        : "ASCII output does not leave dimension block name as UTF-8");
        t.expect(bytes.find("\xE4\xB8\xAD\xE6\x96\x87") == std::string::npos,
                 binary ? "binary output does not label text as UTF-8"
                        : "ASCII output does not label text as UTF-8");
        const bool validStructure = binary ? validateBinaryDxfStructure(bytes)
                                           : validateAsciiDxfStructure(bytes);
        t.expect(validStructure,
                 binary ? "binary output has valid DXF framing"
                        : "ASCII output has valid group-code framing");

        dx_data imported;
        PreparedDxInterface importer;
        importer.attach(&imported);
        const bool readOk = importer.fileImport(path.string(), &imported, false);
        t.expect(readOk, binary ? "binary CP936 model import succeeds"
                                : "ASCII CP936 model import succeeds");
        const std::string codePage = headerString(imported.headerC, "$DWGCODEPAGE");
        t.expect(codePage == "ANSI_936",
                 binary ? "binary round-trip preserves ANSI_936 header"
                        : "ASCII round-trip preserves ANSI_936 header");
        bool foundLayer = false;
        for (const DRW_Layer& candidate : imported.layers)
            foundLayer = foundLayer || candidate.name == layer.name;
        t.expect(foundLayer,
                 binary ? "binary round-trip preserves CP936 layer"
                        : "ASCII round-trip preserves CP936 layer");
        bool foundText = false;
        bool foundDimension = false;
        bool foundHatch = false;
        bool foundColorName = false;
        bool foundEedText = false;
        bool foundEedBinary = false;
        if (imported.mBlock != nullptr) {
            for (const DRW_Entity* entity : imported.mBlock->ent) {
                if (entity != nullptr && entity->eType == DRW::TEXT) {
                    const auto* value = static_cast<const DRW_Text*>(entity);
                    foundText = value->text == "中文A";
                    foundText = foundText && value->style == "中文样式";
                    foundColorName = value->colorName == "中文色";
                    for (const auto& ext : value->extData) {
                        if (ext == nullptr)
                            continue;
                        if (ext->code() == 1000
                            && ext->type() == DRW_Variant::STRING)
                            foundEedText = std::string(ext->c_str()) == "附加";
                        if (ext->code() == 1004
                            && ext->type() == DRW_Variant::STRING)
                            foundEedBinary = std::string(ext->c_str())
                                             == "DEADBEEF";
                    }
                }
                if (entity != nullptr && entity->eType == DRW::DIMLINEAR) {
                    const auto* value =
                        static_cast<const DRW_DimLinear*>(entity);
                    foundDimension = value->getText() == "中文尺寸";
                    foundDimension = foundDimension
                        && const_cast<DRW_DimLinear*>(value)->getName()
                               == dimensionBlockName;
                }
                if (entity != nullptr && entity->eType == DRW::HATCH) {
                    const auto* value = static_cast<const DRW_Hatch*>(entity);
                    foundHatch = value->name == "中文图案";
                }
            }
        }
        t.expect(foundText,
                 binary ? "binary round-trip preserves CP936 text"
                        : "ASCII round-trip preserves CP936 text");
        t.expect(foundColorName,
                 binary ? "binary round-trip preserves CP936 color name"
                        : "ASCII round-trip preserves CP936 color name");
        t.expect(foundDimension,
                 binary ? "binary round-trip preserves CP936 dimension text"
                        : "ASCII round-trip preserves CP936 dimension text");
        t.expect(foundHatch,
                 binary ? "binary round-trip preserves CP936 hatch name"
                        : "ASCII round-trip preserves CP936 hatch name");
        t.expect(importer.mlineStyleName == "中文线型",
                 binary ? "binary round-trip preserves CP936 MLINE style name"
                        : "ASCII round-trip preserves CP936 MLINE style name");
        t.expect(foundEedText,
                 binary ? "binary round-trip preserves CP936 EED text"
                        : "ASCII round-trip preserves CP936 EED text");
        t.expect(foundEedBinary,
                 binary ? "binary round-trip preserves 1004 bytes"
                        : "ASCII round-trip preserves 1004 bytes");
        std::filesystem::remove(path, ec);
    }
}

void testDimensionAdapterCopyPreservesSubtype(TestContext& t) {
    dx_data data;
    PreparedDxInterface interface_;
    interface_.attach(&data);

    DRW_DimAligned aligned;
    DRW_DimLinear linear;
    DRW_DimRadial radial;
    DRW_DimDiametric diametric;
    DRW_DimAngular angular;
    DRW_DimAngular3p angular3p;
    DRW_DimOrdinate ordinate;
    interface_.addDimAlign(&aligned);
    interface_.addDimLinear(&linear);
    interface_.addDimRadial(&radial);
    interface_.addDimDiametric(&diametric);
    interface_.addDimAngular(&angular);
    interface_.addDimAngular3P(&angular3p);
    interface_.addDimOrdinate(&ordinate);

    const std::array<DRW::ETYPE, 7> expected {{
        DRW::DIMALIGNED, DRW::DIMLINEAR, DRW::DIMRADIAL,
        DRW::DIMDIAMETRIC, DRW::DIMANGULAR, DRW::DIMANGULAR3P,
        DRW::DIMORDINATE}};
    bool preserved = data.mBlock != nullptr
                     && data.mBlock->ent.size() == expected.size();
    if (preserved) {
        std::size_t index = 0;
        for (const DRW_Entity* entity : data.mBlock->ent) {
            preserved = entity != nullptr && entity->eType == expected[index];
            if (!preserved)
                break;
            ++index;
        }
    }
    t.expect(preserved,
             "dx_iface dimension callbacks preserve concrete subtypes");
}

void testPublicDwgCodePageSnapshot(TestContext& t) {
#ifdef LIBDXFRW_DWG_FIXTURE_DIR
    const std::filesystem::path source =
        std::filesystem::path(LIBDXFRW_DWG_FIXTURE_DIR)
        / "ordinary_enc_AC1021.dwg";
#else
    const std::filesystem::path source =
        std::filesystem::path("tests/fixtures/dwg")
        / "ordinary_enc_AC1021.dwg";
#endif
    dx_data data;
    PreparedDxInterface interface_;
    interface_.attach(&data);
    dwgRW reader(source.string().c_str());
    t.expect(reader.read(&interface_, false),
             "public DWG reader accepts the eligible AC1021 fixture");
    t.expect(!data.headerC.hasDxfCodePageDeclaration(),
             "DWG HEADER does not fabricate a physical DXF declaration");
    t.expect(reader.getCodePage() == "ANSI_1252",
             "public dwgRW getter exposes the source code page after success");
    t.expect(reader.hasSourceCodePageField()
                 && reader.hasRecognizedSourceCodePage()
                 && reader.getSourceCodePageId() == 30,
             "public DWG codepage snapshot exposes recognized envelope identity");

    dwgRW unsupported("unused-unknown-codepage.dwg");
    t.expect(DwgCodePageTestAccess::capturePublicSnapshot(
                 unsupported, 999, true, true)
                 && unsupported.hasSourceCodePageField()
                 && !unsupported.hasRecognizedSourceCodePage()
                 && unsupported.getSourceCodePageId() == 999
                 && unsupported.getCodePage().empty(),
             "public DWG codepage snapshot distinguishes unsupported raw id from absence");
    t.expect(DwgCodePageTestAccess::capturePublicSnapshot(
                 unsupported, 0, false, true)
                 && !unsupported.hasSourceCodePageField()
                 && !unsupported.hasRecognizedSourceCodePage()
                 && unsupported.getSourceCodePageId() == 0
                 && unsupported.getCodePage().empty(),
             "public DWG codepage snapshot distinguishes an absent field");

    const std::array<std::uint8_t, 6> invalidDwg {{
        'A', 'C', '1', '0', '2', '7'}};
    t.expect(!reader.readBuffer(invalidDwg.data(), invalidDwg.size(),
                                &interface_, false)
                 && reader.getCodePage().empty()
                 && !reader.hasSourceCodePageField()
                 && !reader.hasRecognizedSourceCodePage()
                 && reader.getSourceCodePageId() == 0,
             "failed public DWG read clears the prior source code page");

    dx_data legacyData;
    PreparedDxInterface legacyInterface;
    legacyInterface.attach(&legacyData);
    dwgR legacy(source.string().c_str());
    t.expect(legacy.read(&legacyInterface, false),
             "deprecated dwgR wrapper accepts the eligible fixture");
    t.expect(legacy.getCodePage() == "ANSI_1252",
             "deprecated dwgR wrapper forwards source code page");
    t.expect(legacy.hasSourceCodePageField()
                 && legacy.hasRecognizedSourceCodePage()
                 && legacy.getSourceCodePageId() == 30,
             "deprecated dwgR wrapper forwards source codepage identity");
}

} // namespace

void testStandardDwgPayloadProfilesAndAllocation(TestContext& t) {
    using Access = DxIfaceStandardDwgCaptureTestAccess;
    dwgBufferW value;
    value.putRawShort16(1);
    value.putRawShort16(2);
    value.putRawChar8(0);
    value.putRawChar8(0xD6u);
    value.putRawChar8(0xD0u);
    const auto bytes = xrecord_test::frameWithData(DRW::AC1018, value.data());
    const auto setup = [](dx_data& data, dx_iface& iface, std::uint64_t generation,
                          std::uint16_t codePage) {
        iface.cData = &data;
        data.sourceFormat = dx_source_format::DWG;
        data.sourceVersion = DRW::AC1018;
        data.importGeneration = generation;
        data.dwgReadOutcome = {};
        data.dwgReadOutcome.attempted = true;
        data.dwgReadOutcome.sourceVersion = DRW::AC1018;
        data.dwgReadOutcome.sourceCodePageId = codePage;
        data.dwgReadOutcome.sourceCodePageFieldPresent = true;
        data.dwgReadOutcome.sourceCodePageRecognized = true;
    };
    const auto rawFrame = [&bytes]() {
        DRW_UnsupportedObject raw;
        raw.m_objectType = 79;
        raw.m_version = DRW::AC1018;
        raw.m_handle = 0xA620u;
        raw.m_objectOffset = 100u;
        raw.m_rawBytes = bytes;
        raw.m_objectSize = static_cast<std::uint32_t>(bytes.size());
        return raw; // legacy common-link projections intentionally Unknown
    };
    const auto retainTyped = [&bytes](dx_iface& iface, std::uint16_t codePage) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1018, false);
        codec.setCodePage(dwgCodePageName(codePage), false);
        codec.setByteCodePage(dwgCodePageName(codePage));
        dwgBuffer body(bytes.data(), bytes.size(), &codec);
        xrecord_test::CodecProbe parsed;
        if (!parsed.parseDwg(DRW::AC1018, &body) || !body.isGood())
            return false;
        iface.addXRecord(parsed);
        return true;
    };
    const auto retainSource = [&rawFrame](dx_iface& iface) {
        iface.addUnsupportedObject(rawFrame());
        DRW_DwgFramePublication receipt;
        receipt.m_version = DRW::AC1018;
        receipt.m_handle = 0xA620u;
        receipt.m_sourceOffset = 100u;
        receipt.m_sourceMapOrdinal = 7u;
        receipt.m_sourceOffsetSpace = DRW_DwgFrameOffsetSpace::DecodedBuffer;
        receipt.m_hasSourceLocation = true;
        receipt.m_encodedType = receipt.m_resolvedType = 79;
        receipt.m_xDictFlag = 1u;
        receipt.m_carrier = DRW_DwgFramePublication::Carrier::TypedAndRaw;
        iface.addDwgFramePublication(receipt);
        Access::finish(iface);
    };
    dx_data reused;
    dx_iface reader;
    setup(reused, reader, 1u, 30u);
    t.expect(retainTyped(reader, 30u), "CP1252 source XRECORD parses");
    retainSource(reader);
    reused.dwgReadOutcome.sourceCodePageId = 31u; // latest envelope is not the saved source
    Access::validate(reader);
    t.expect(!reused.hasStandardDwgObjectPayloadFailure()
                 && Access::matched(reused, 1u, 0xA620u, 30u)
                 && *reused.xRecords.front().m_values.front().content.s == "\xC3\x96\xC3\x90",
             "saved CP1252 profile survives a changed latest envelope");
    std::size_t currentModelFailures = 0u;
    bool currentModelCompleted = false;
    for (std::size_t failAt = 1u; failAt <= 32u; ++failAt) {
        standardPayloadAllocations = 0u;
        standardPayloadFailAt = failAt;
        const bool unchanged = Access::unchanged(reader);
        standardPayloadFailAt = 0u;
        if (standardPayloadAllocations < failAt) {
            currentModelCompleted = unchanged;
            break;
        }
        ++currentModelFailures;
        t.expect(!unchanged && !reused.hasStandardDwgObjectPayloadFailure()
                     && Access::unchanged(reader),
                 "current-model index allocation failure refuses without poisoning same-model retry");
    }
    t.expect(currentModelCompleted && currentModelFailures != 0u,
             "current-model allocation sweep reaches unchanged source admission");
    std::cout << "Standard DWG current-model allocation positions refused=" << currentModelFailures << '\n';
    setup(reused, reader, 2u, 31u);
    t.expect(retainTyped(reader, 31u), "CP936 source XRECORD parses");
    retainSource(reader);
    Access::validate(reader, 1u);
    t.expect(!reused.hasStandardDwgObjectCaptureFailure()
                 && !reused.hasStandardDwgObjectPayloadFailure()
                 && Access::matched(reused, 1u, 0xA620u, 30u)
                 && Access::matched(reused, 2u, 0xA620u, 31u)
                 && *reused.xRecords.back().m_values.front().content.s == "\xE4\xB8\xAD"
                 && reused.dwgReadOutcome.standardObjectTypedPayloadsMatched == 1u
                 && reused.dwgReadOutcome.opaqueFramesRetainedByAdapter == 0u,
             "same bytes and handle use distinct import generations and immutable CP1252/CP936 profiles");
    t.expect(!Access::unchanged(reader),
             "duplicate source handles across imports cannot qualify one output namespace");
    bool completed = false;
    std::size_t injected = 0u;
    for (std::size_t failAt = 1u; failAt <= 512u; ++failAt) {
        dx_data data;
        dx_iface iface;
        setup(data, iface, 1u, 31u);
        if (!retainTyped(iface, 31u)) {
            t.expect(false, "allocation control source parses before injection");
            break;
        }
        retainSource(iface);
        standardPayloadAllocations = 0u;
        standardPayloadFailAt = failAt;
        Access::validate(iface);
        standardPayloadFailAt = 0u;
        if (standardPayloadAllocations < failAt) {
            completed = !data.hasStandardDwgObjectPayloadFailure()
                && data.dwgReadOutcome.standardObjectTypedPayloadsMatched == 1u;
            break;
        }
        ++injected;
        t.expect(data.hasStandardDwgObjectPayloadFailure()
                     && data.dwgReadOutcome.standardObjectPayloadValidationFailed
                     && data.dwgReadOutcome.standardObjectTypedPayloadsMatched == 0u
                     && data.dwgReadOutcome.opaqueFramesRetainedByAdapter == 0u,
                 "each injected validation allocation failure refuses without publishing completeness");
    }
    t.expect(completed && injected != 0u,
             "allocation sweep reaches a complete fresh-data retry after all failing positions");
    dx_data failedCapture;
    dx_iface capture;
    setup(failedCapture, capture, 1u, 30u);
    const auto raw = rawFrame();
    failImportObservationAllocations = true;
    capture.addUnsupportedObject(raw);
    failImportObservationAllocations = false;
    t.expect(failedCapture.hasStandardDwgObjectCaptureFailure(),
             "forced source-copy allocation failure stays in its distinct capture lane");
    std::cout << "Standard DWG payload allocation positions refused=" << injected << '\n';
    DRW_RawDxfSection rawSection;
    rawSection.m_name = "ACDSDATA";
    rawSection.m_version = DRW::AC1027;
    rawSection.m_groups.emplace_back(0, "ACDSRECORD");
    rawSection.m_groups.emplace_back(310, std::string(256u, 'A'));
    rawSection.m_rawValues = {"ACDSRECORD", std::string(256u, 'A')};
    rawSection.m_hasRawValues = true;
    bool sectionCompleted = false;
    std::size_t sectionFailures = 0u;
    for (std::size_t failAt = 1u; failAt <= 256u; ++failAt) {
        dx_data sectionData;
        dx_iface sectionReader;
        sectionReader.cData = &sectionData;
        sectionData.sourceFormat = dx_source_format::DXF;
        sectionData.dxfReadOutcome.attempted = true;
        sectionData.importGeneration = 1u;
        standardPayloadAllocations = 0u;
        standardPayloadFailAt = failAt;
        sectionReader.addRawDxfSection(rawSection);
        standardPayloadFailAt = 0u;
        if (standardPayloadAllocations < failAt) {
            sectionCompleted = !sectionData.rawDxfSectionCaptureFailed
                && sectionData.rawDxfSections.size() == 1u
                && sectionData.hasUnchangedAcdsRawDxfSections();
            break;
        }
        ++sectionFailures;
        t.expect(sectionData.rawDxfSectionCaptureFailed && sectionData.rawDxfSections.empty()
                     && sectionData.rawDxfSectionRetainedRecords == 0u
                     && sectionData.rawDxfSectionRetainedGroups == 0u
                     && sectionData.rawDxfSectionRetainedBytes == 0u
                     && !sectionData.hasUnchangedAcdsRawDxfSections()
                     && Access::acdsBaselines(sectionData) == 0u,
                 "ACDS baseline/public-copy allocation failure leaves neither a partial carrier nor an orphan baseline");
    }
    t.expect(sectionCompleted && sectionFailures != 0u,
             "ACDS baseline allocation sweep reaches complete fresh-data capture");
    std::cout << "ACDS raw baseline allocation positions refused=" << sectionFailures << '\n';
}

// API-authored scratch input only. The structure-finalization override keeps
// this an isolated codec/adapter fault control, not native CAD qualification.
class StandardOutputAllocationAuthor final : public dx_iface {
public:
    explicit StandardOutputAllocationAuthor(dwgRW& writer) : writer_(writer) {
        cData = &data_; currentBlock = data_.mBlock;
    }
    void writeHeader(DRW_Header& header) override { header.vars.clear(); }
    void writeDwgClasses() override {
        registered_ = writer_.registerDwgGroupDictionaryEntry("ALLOC_GROUP", 0xA700u)
            && writer_.registerDwgNamedObjectDictionaryEntry("ALLOC_XRECORD", 0xA701u);
    }
    void writeEntities() override {
        DRW_Line line;
        line.handle = 0xA710u;
        line.secPoint = {1.0, 2.0, 3.0};
        wroteLine_ = writer_.writeLine(&line);
    }
    void writeObjects() override {
        DRW_Group group;
        group.handle = 0xA700u;
        group.parentHandle = DRW::DwgAcadGroupDictionaryHandle;
        group.name = "ALLOC_GROUP";
        group.m_description = "allocation group";
        group.m_entityHandles = {0xA710u};
        DRW_XRecord record;
        record.handle = 0xA701u;
        record.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
        record.m_dataEntries = {{1, std::string("allocation value")},
            {330, std::uint64_t(group.handle)}, {310, std::vector<std::uint8_t>{0x13u, 0xA5u}}};
        (void)record.addExtData(std::make_unique<DRW_Variant>(1001, std::string("ACAD")));
        auto layer = std::make_unique<DRW_Variant>(1003, std::string("0"));
        layer->setLayerRefName("0");
        (void)record.addExtData(std::move(layer));
        wroteObjects_ = writer_.writeGroup(&group) && writer_.writeXRecord(&record);
    }
    bool finalizeDwgWrite() override { return registered_ && wroteLine_ && wroteObjects_; }
    bool finalizeDwgWriteStructure() override { return true; }
private:
    dwgRW& writer_;
    dx_data data_;
    bool registered_{false}, wroteLine_{false}, wroteObjects_{false};
};

class StandardOutputAllocationExporter final : public dx_iface {
public:
    StandardOutputAllocationState state;
    void writeHeader(DRW_Header& header) override {
        dx_iface::writeHeader(header);
        using Phase = StandardOutputAllocationState::Phase;
        if (state.phase == Phase::Inspection)
            DxIfaceStandardDwgCaptureTestAccess::wrapInspector(*dxfW, state);
        else if (state.phase == Phase::TableInspection)
            DxIfaceStandardDwgCaptureTestAccess::wrapTableInspector(*dxfW, state);
        else if (state.phase == Phase::GroupPlan)
            (void)DxIfaceStandardDwgCaptureTestAccess::planGroups(*dxfW, state);
    }
    void writeLayers() override {
        if (state.phase == StandardOutputAllocationState::Phase::LayerRows) {
            StandardOutputAllocationScope inject(state);
            dx_iface::writeLayers();
        } else dx_iface::writeLayers();
    }
    void writeAppId() override {
        if (state.phase == StandardOutputAllocationState::Phase::AppIdRows) {
            StandardOutputAllocationScope inject(state);
            dx_iface::writeAppId();
        } else dx_iface::writeAppId();
    }
    bool finalizeDxfWrite() override {
        if (state.phase == StandardOutputAllocationState::Phase::Finalization) {
            StandardOutputAllocationScope inject(state);
            return dx_iface::finalizeDxfWrite();
        }
        return dx_iface::finalizeDxfWrite();
    }
};

void testStandardDwgOutputAllocation(TestContext& t) {
    using Access = DxIfaceStandardDwgCaptureTestAccess;
    using Phase = StandardOutputAllocationState::Phase;
    const auto scratch = std::filesystem::temp_directory_path()
        / ("libdxfrw-standard-output-allocation-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::error_code error;
    if (!std::filesystem::create_directory(scratch, error) || error) {
        t.expect(false, "allocation control obtains its unique scratch directory");
        return;
    }
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code ignored; std::filesystem::remove_all(path, ignored); } } cleanup{scratch};
    const auto sourcePath = scratch / "source.dwg";
    dwgRW sourceWriter(sourcePath.string().c_str());
    StandardOutputAllocationAuthor author(sourceWriter);
    dx_data source;
    dx_iface importer;
    const bool prepared = sourceWriter.write(&author, DRW::AC1021, true)
        && importer.fileImport(sourcePath.string(), &source, false)
        && !source.hasStandardDwgObjectCaptureFailure() && !source.hasStandardDwgObjectPayloadFailure();
    t.expect(prepared, "rooted allocation source authors/imports before allocator injection");
    if (!prepared) return;
    StandardOutputAllocationExporter exporter;
    const auto output = scratch / "output.dxf";
    const auto write = [&](bool binary) {
        return exporter.fileExport(output.string(), DRW::AC1032, binary, &source, false, true,
            dx_dxfSourceTextPolicy::Compatible, DRW::DxfTextWritePolicy::Compatible,
            dx_dxfProxyGraphicsPolicy::Compatible, dx_dwgReadCompletenessPolicy::AllowPartial);
    };
    for (const bool binary : {false, true}) {
        t.expect(write(binary) && Access::output(exporter, 4u, true),
                 "allocation source has four actual output facts before injection");
        for (const auto phase : {Phase::Inspection, Phase::Finalization, Phase::GroupPlan,
                Phase::TableInspection, Phase::LayerRows, Phase::AppIdRows}) {
            std::size_t refused = 0u;
            bool completed = false;
            for (std::size_t failAt = 1u; failAt <= 1024u; ++failAt) {
                const std::string sentinel = "PRESERVE_STANDARD_OUTPUT_ALLOCATION_DESTINATION";
                { std::ofstream previous(output, std::ios::binary); previous << sentinel; }
                exporter.state = {};
                exporter.state.phase = phase;
                exporter.state.failAt = failAt;
                const bool wrote = write(binary);
                standardPayloadFailAt = 0u;
                const auto measured = exporter.state.measured;
                const auto entered = exporter.state.entered;
                if (!entered) {
                    t.expect(false, "allocation sweep reached its selected production phase");
                    break;
                }
                if (measured < failAt) {
                    completed = wrote && Access::output(exporter, 4u, true);
                    break;
                }
                ++refused;
                std::ifstream previous(output, std::ios::binary);
                const std::string retained((std::istreambuf_iterator<char>(previous)), std::istreambuf_iterator<char>());
                previous.close();
                const bool refusedAtomically = !wrote && retained == sentinel && Access::output(exporter, 0u, false)
                    && Access::unchanged(importer) && !source.hasStandardDwgObjectPayloadFailure()
                    && source.dwgReadOutcome.opaqueFramesRetainedByAdapter == 0u;
                if (!refusedAtomically) {
                    std::cerr << "Standard output allocation phase=" << static_cast<int>(phase)
                              << " binary=" << binary << " failAt=" << failAt << " measured=" << measured << " wrote=" << wrote << '\n';
                    t.expect(false, "injected output-proof/finalization/GROUP-plan allocation must retract facts and preserve destination/source evidence");
                    break;
                }
                exporter.state = {};
                if (!write(binary) || !Access::output(exporter, 4u, true) || !Access::unchanged(importer)) {
                    t.expect(false, "corrected same-adapter retry restores full modeled output after allocation refusal");
                    break;
                }
            }
            exporter.state = {};
            t.expect(completed && refused != 0u,
                     "output inspection/finalization/GROUP-plan sweep reaches every allocation position and a no-fault completion");
            std::cout << "Standard output allocation phase=" << static_cast<int>(phase)
                      << " binary=" << binary << " refused=" << refused << " completed=" << completed << '\n';
        }
    }
}

void testXrecordWriterCodepage(TestContext& t) {
    struct Cell { const char* page; std::string text; std::vector<std::uint8_t> bytes; };
    const std::vector<Cell> cells{
        {"ANSI_1252", "caf\xC3\xA9", {'c', 'a', 'f', 0xE9u}},
        {"ANSI_936", "\xE4\xB8\xAD\xE6\x96\x87", {0xD6u, 0xD0u, 0xCEu, 0xC4u}},
        {"ANSI_1251", "\xD0\x9F\xD1\x80\xD0\xB8\xD0\xB2\xD0\xB5\xD1\x82", {0xCFu, 0xF0u, 0xE8u, 0xE2u, 0xE5u, 0xF2u}},
        {"ANSI_1252", std::string("a\0b", 3), {'a', 0, 'b'}}
    };
    for (const auto version : {DRW::AC1015, DRW::AC1018}) for (const auto& cell : cells) {
        DRW_TextCodec codec;
        codec.setVersion(version, false);
        // Deliberately retain the default primary codec: legacy value text
        // must follow the selected *byte* profile of the containing DWG.
        codec.setByteCodePage(cell.page);
        dwgBufferW body(&codec), handles;
        xrecord_test::CodecProbe source;
        source.handle = 0xA620u; source.parentHandle = 0xCu;
        source.setDwgCommonObjectState(0, 1, false);
        source.m_dataEntries.emplace_back(1, cell.text);
        t.expect(source.encodeDwg(version, &body, nullptr, &handles), "legacy XRECORD encodes through containing codec");
        dwgBuffer readback(body.data().data(), body.data().size());
        bool exact = readback.getBitLong() == static_cast<std::int32_t>(cell.bytes.size() + 5u)
            && readback.getRawShort16() == 1u
            && readback.getRawShort16() == cell.bytes.size()
            && readback.getRawChar8() == 0u;
        for (const auto byte : cell.bytes) exact = readback.getRawChar8() == byte && exact;
        t.expect(exact && readback.isGood(), "literal legacy XRECORD bytes/count use CP1252/CP936/CP1251 and preserve embedded NUL");
    }
    std::cout << "Legacy XRECORD writer literal codepage cells=8\n";
}

void testGeneratedDictionaryMetadataAllocation(TestContext& t) {
    DRW_Dictionary prototype;
    prototype.handle = 0xA620u;
    prototype.m_entries = {{"ACAD_GROUP", 0xA621u}, {"ADDITIONAL_METADATA_SLOT", 0xA622u}};
    (void)prototype.addExtData(std::make_unique<DRW_Variant>(1001, std::string("SETUP_APP")));
    (void)prototype.addExtData(std::make_unique<DRW_Variant>(1000, std::string(128u, 'R')));
    DRW_Dictionary groupPrototype;
    groupPrototype.handle = 0xA621u;
    groupPrototype.parentHandle = prototype.handle;
    groupPrototype.appData.push_back({{102, std::string("{SETUP_METADATA")},
                                      {1, std::string(128u, 'G')}, {102, std::string("}")}});
    bool completed = false;
    std::size_t failures = 0u;
    for (std::size_t failAt = 1u; failAt <= 256u; ++failAt) {
        dxfRW writer("");
        auto previous = std::make_unique<DRW_Dictionary>();
        previous->handle = 0xA600u;
        if (!writer.setGeneratedDictionarySourceRecords(std::move(previous), nullptr)) {
            t.expect(false, "baseline dictionary metadata installs before allocation injection");
            break;
        }
        bool installed = false;
        bool threw = false;
        standardPayloadAllocations = 0u;
        standardPayloadFailAt = failAt;
        try {
            auto root = std::make_unique<DRW_Dictionary>(prototype);
            auto groups = std::make_unique<DRW_Dictionary>(groupPrototype);
            installed = writer.setGeneratedDictionarySourceRecords(std::move(root), std::move(groups));
        } catch (const std::bad_alloc&) {
            threw = true; // Caller-owned snapshot creation, before transfer.
        }
        standardPayloadFailAt = 0u;
        if (standardPayloadAllocations < failAt) {
            completed = installed && !threw
                && writer.setGeneratedDictionarySourceHandles(0xA620u, 0xA621u);
            break;
        }
        ++failures;
        t.expect(!installed && writer.setGeneratedDictionarySourceHandles(0xA600u, 0u),
                 "copy/index allocation failure leaves the prior owned dictionary setup unchanged");
        t.expect(writer.setGeneratedDictionarySourceRecords(
                     std::make_unique<DRW_Dictionary>(prototype),
                     std::make_unique<DRW_Dictionary>(groupPrototype))
                     && writer.setGeneratedDictionarySourceHandles(0xA620u, 0xA621u),
                 "same codec accepts fresh metadata after every failed allocation position");
    }
    t.expect(completed && failures != 0u,
             "dictionary ownership/copy/index allocation sweep reaches complete fresh setup");
    std::cout << "Generated dictionary metadata allocation positions refused=" << failures << '\n';
}

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--xrecord-writer-codepage") {
        TestContext focused;
        testXrecordWriterCodepage(focused);
        return focused.failures == 0 ? 0 : 1;
    }
    TestContext context;
    if (argc == 2 && std::string(argv[1]) == "--generated-dictionary-metadata-allocation") {
        testGeneratedDictionaryMetadataAllocation(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--standard-dwg-payload-profile-and-allocation") {
        testStandardDwgPayloadProfilesAndAllocation(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--dwg-codepage-snapshot") {
        testPublicDwgCodePageSnapshot(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--consumed-text-profiles") {
        testConsumedTextProfiles(context);
        testExactCodePageImport(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--exact-codepage-import") {
        testExactCodePageImport(context);
        testConverterDxfReadOutcome(context);
        testDxfSourceTextAdmission(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--exact-codepage-reader") {
        testExactCodePageReader(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--iso8859-4") {
        testExactIso8859_4(context);
        testExactCodePageResolution(context);
        testCodePageSelectionMetadata(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--iso8859-1") {
        testExactIso8859_1(context);
        testExactCodePageResolution(context);
        testExactCodePageReader(context);
        testConsumedTextProfiles(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--codepage-resolution") {
        testExactCodePageResolution(context);
        testCodePageSelectionMetadata(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 3 && std::string(argv[1]) == "--target-dwg-control") {
        writeTargetDwgControl(context, std::filesystem::u8path(argv[2]));
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 3 && std::string(argv[1]) == "--verify-target-dxf-control") {
        verifyTargetDxfControl(context, std::filesystem::u8path(argv[2]));
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--dwg-tv-tu-target-matrix") {
        testDwgTvTuModernDxfCharacterization(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--dwg-tv-tu-entity-integration") {
        testDwgTvTuEntityIntegration(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--target-text-transport") {
        testDxfAsciiCaretEscapes(context);
        testTargetTextTransport(context);
        testTargetTextFacade(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--source-text-admission") {
        testDxfSourceTextAdmission(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--dxf-import-outcome") {
        testConverterDxfReadOutcome(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--text-read-observations") {
        testTextDecodeObservations(context);
        testTextObservationReaderRoles(context);
        testTextObservationHandoff(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--text-decode-issue-accumulation") {
        testTextDecodeObservations(context);
        testDwgTextDecodeBoundaries(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--dwg-eed-text-provenance") {
        testDwgEedTextProvenance(context);
        testDwgLegacyTextNormalization(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--xrecord-per-value-codepage") {
        testXRecordPerValueCodePage(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--appid-name-source") {
        testAppIdNameSource(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--layer-name-source") {
        testLayerNameSource(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--standard-dwg-output-allocation") {
        testStandardDwgOutputAllocation(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--codepage-provenance") {
        testCodePageProvenanceBudget(context);
        testCodePageDeclarationProvenance(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--codepage-selection") {
        testCodePageSelectionMetadata(context);
        testCodePageDiagnosticHandoff(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--codepage-policy") {
        testCodePagePolicyAnchors(context);
        testMacRomanCodePage(context);
        testExactIso8859_2(context);
        testExactIso8859_3(context);
        testExactIso8859_5(context);
        testExactIso8859_6(context);
        testExactIso8859_7(context);
        testExactIso8859_8(context);
        testExactIso8859_9(context);
        testExactIso8859_10(context);
        testExactIso8859_11(context);
        testExactIso8859_13(context);
        testExactIso8859_14(context);
        testExactIso8859_15(context);
        testExactCodePageResolution(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--iso8859-2-codepage") {
        testExactIso8859_2(context);
        testExactCodePageResolution(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--iso8859-3-codepage") {
        testExactIso8859_3(context);
        testExactCodePageResolution(context);
        testConsumedTextProfiles(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--iso8859-5-codepage") {
        testExactIso8859_5(context);
        testExactCodePageResolution(context);
        testConsumedTextProfiles(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--iso8859-6-codepage") {
        testExactIso8859_6(context);
        testExactCodePageResolution(context);
        testCodePageSelectionMetadata(context);
        testConsumedTextProfiles(context);
        testDxfSourceTextAdmission(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--iso8859-7-codepage") {
        testExactIso8859_7(context);
        testExactCodePageResolution(context);
        testCodePageSelectionMetadata(context);
        testConsumedTextProfiles(context);
        testDxfSourceTextAdmission(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--iso8859-8-codepage") {
        testExactIso8859_8(context);
        testExactCodePageResolution(context);
        testCodePageSelectionMetadata(context);
        testConsumedTextProfiles(context);
        testDxfSourceTextAdmission(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--iso8859-9-codepage") {
        testExactIso8859_9(context);
        testExactCodePageResolution(context);
        testCodePageSelectionMetadata(context);
        testConsumedTextProfiles(context);
        testDxfSourceTextAdmission(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--iso8859-10-codepage") {
        testExactIso8859_10(context);
        testExactCodePageResolution(context);
        testCodePageSelectionMetadata(context);
        testConsumedTextProfiles(context);
        testDxfSourceTextAdmission(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--iso8859-11-codepage") {
        testExactIso8859_11(context);
        testExactCodePageResolution(context);
        testCodePageSelectionMetadata(context);
        testConsumedTextProfiles(context);
        testDxfSourceTextAdmission(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--iso8859-13-codepage") {
        testExactIso8859_13(context);
        testExactCodePageResolution(context);
        testCodePageSelectionMetadata(context);
        testConsumedTextProfiles(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--iso8859-14-codepage") {
        testExactIso8859_14(context);
        testExactCodePageResolution(context);
        testCodePageSelectionMetadata(context);
        testConsumedTextProfiles(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--iso8859-15-codepage") {
        testExactIso8859_15(context);
        testExactCodePageResolution(context);
        testCodePageSelectionMetadata(context);
        testConsumedTextProfiles(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--mac-roman-codepage") {
        testMacRomanCodePage(context);
        testCodePageSelectionMetadata(context);
        testExactCodePageResolution(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--cif-policy") {
        testCifEscapePolicy(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--turkish-codepage") {
        testTurkishCodePageRoundTrip(context);
        return context.failures == 0 ? 0 : 1;
    }
    if ((argc == 2 || argc == 3) && std::string(argv[1]) == "--codepage-alias-audit") {
        const auto cases = auditNamedCodePageAliases(context, argc == 3 ? argv[2] : "");
        return cases == 0 ? 2 : context.failures == 0 ? 0 : 1;
    }
    testMifEscapePolicy(context);
    testMifLiteralModelRoundTrip(context);
    if (argc == 2 && std::string(argv[1]) == "--mif-policy") {
        std::cout << "MIF policy controls: " << (context.failures == 0 ? "PASS" : "FAIL") << '\n';
        return context.failures == 0 ? 0 : 1;
    }
    if (argc != 1)
        return 2;
    testCodePageAliases(context);
    testCifEscapePolicy(context);
    testTurkishCodePageRoundTrip(context);
    testCodePagePolicyAnchors(context);
    testMacRomanCodePage(context);
    testExactIso8859_2(context);
    testExactIso8859_3(context);
    testExactIso8859_5(context);
    testExactIso8859_6(context);
    testExactIso8859_7(context);
    testExactIso8859_8(context);
    testExactIso8859_9(context);
    testExactIso8859_10(context);
    testExactIso8859_11(context);
    testExactIso8859_13(context);
    testExactIso8859_14(context);
    testExactIso8859_15(context);
    testCodePageSelectionMetadata(context);
    testExactCodePageResolution(context);
    testExactIso8859_4(context);
    testCodePageDiagnosticHandoff(context);
    testCodePageProvenanceBudget(context);
    testCodePageDeclarationProvenance(context);
    testTextDecodeObservations(context);
    testDwgTextDecodeBoundaries(context);
    testDwgEedTextProvenance(context);
    testDwgLegacyTextNormalization(context);
    testXRecordPerValueCodePage(context);
    testAppIdNameSource(context);
    testLayerNameSource(context);
    testTextObservationReaderRoles(context);
    testTextObservationHandoff(context);
    testExactCodePageReader(context);
    testConverterDxfReadOutcome(context);
    testExactCodePageImport(context);
    testConsumedTextProfiles(context);
    testDxfSourceTextAdmission(context);
    testDxfAsciiCaretEscapes(context);
    testTargetTextTransport(context);
    testTargetTextFacade(context);
    testMalformedUtf8(context);
    testReaderSourceState(context);
    testDxfWriters(context);
    testDxfModelRoundTrip(context);
    testDimensionAdapterCopyPreservesSubtype(context);
    testPublicDwgCodePageSnapshot(context);
    if (context.failures != 0) {
        std::cerr << context.failures
                  << " GBK/codepage assertion(s) failed\n";
        return 1;
    }
    std::cout << "GBK/codepage tests: PASS\n";
    return 0;
}
