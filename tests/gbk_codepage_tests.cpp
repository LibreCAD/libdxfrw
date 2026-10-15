#include <array>
#include <cstdint>
#include <chrono>
#include <charconv>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
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
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"
#include "libdwgr.h"
#include "dwg2dxf/dx_data.h"
#include "dwg2dxf/dx_iface.h"

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
};

class PreparedDxInterface final : public dx_iface {
public:
    void attach(dx_data* data) {
        cData = data;
        currentBlock = data != nullptr ? data->mBlock : nullptr;
    }

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


struct CodePageAnchor {
    const char* name;
    const char* bytes;
    const char* utf8;
};

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

    // Frozen pre-change selector transcription, not a semantic Unicode
    // oracle. Future mapping/policy changes must intentionally change this
    // compatibility baseline, never interpret it as named-codec support.
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
        {"APPLE ROMAN", "ANSI_1252"},
        {"ISO8859-1", "ANSI_1252"},
        {"ISO8859-15", "ANSI_1252"},
        {"ISO-IR-100", "ANSI_1252"},
        {"L1", "ANSI_1252"},
        {"IBM 850", "ANSI_1252"},
        {"ANSI_1253", "ANSI_1253"},
        {"CP1253", "ANSI_1253"},
        {"ISO8859-7", "ANSI_1253"},
        {"ANSI_1254", "ANSI_1254"},
        {"CP1254", "ANSI_1254"},
        {"ISO8859-9", "ANSI_1254"},
        {"ISO8859-3", "ANSI_1254"},
        {"ANSI_1255", "ANSI_1255"},
        {"CP1255", "ANSI_1255"},
        {"ISO8859-8", "ANSI_1255"},
        {"ANSI_1256", "ANSI_1256"},
        {"CP1256", "ANSI_1256"},
        {"ISO8859-6", "ANSI_1256"},
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
            && imported.mBlock->ent.size() == 2;
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

        dx_data source;
        source.headerC.addStr("$ACADVER", "AC1018", 1);
        source.headerC.addStr("$DWGCODEPAGE", "ANSI_936", 3);
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

        auto* mline = new DRW_MLine();
        mline->layer = layer.name;
        mline->scale = 1.0;
        mline->justification = 0;
        mline->basePoint = DRW_Coord(10.0, 11.0, 0.0);
        mline->extPoint = DRW_Coord(0.0, 0.0, 1.0);
        mline->openClosed = 1;
        mline->numLines = 1;
        mline->numVerts = 1;
        mline->styleName = "中文线型";
        DRW_MLineVertex mlineVertex;
        mlineVertex.position = DRW_Coord(12.0, 13.0, 0.0);
        mlineVertex.vertexDir = DRW_Coord(1.0, 0.0, 0.0);
        mlineVertex.miterDir = DRW_Coord(0.0, 1.0, 0.0);
        mlineVertex.segParms = {{0.5}};
        mlineVertex.areaFillParms = {{0.25}};
        mline->vertlist.push_back(mlineVertex);
        source.mBlock->ent.push_back(mline);

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
    t.expect(reader.getCodePage() == "ANSI_1252",
             "public dwgRW getter exposes the source code page after success");

    const std::array<std::uint8_t, 6> invalidDwg {{
        'A', 'C', '1', '0', '2', '7'}};
    t.expect(!reader.readBuffer(invalidDwg.data(), invalidDwg.size(),
                                &interface_, false)
                 && reader.getCodePage().empty(),
             "failed public DWG read clears the prior source code page");

    dx_data legacyData;
    PreparedDxInterface legacyInterface;
    legacyInterface.attach(&legacyData);
    dwgR legacy(source.string().c_str());
    t.expect(legacy.read(&legacyInterface, false),
             "deprecated dwgR wrapper accepts the eligible fixture");
    t.expect(legacy.getCodePage() == "ANSI_1252",
             "deprecated dwgR wrapper forwards source code page");
}

} // namespace

int main(int argc, char** argv) {
    TestContext context;
    if (argc == 2 && std::string(argv[1]) == "--codepage-selection") {
        testCodePageSelectionMetadata(context);
        testCodePageDiagnosticHandoff(context);
        return context.failures == 0 ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--codepage-policy") {
        testCodePagePolicyAnchors(context);
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
    testCodePagePolicyAnchors(context);
    testCodePageSelectionMetadata(context);
    testCodePageDiagnosticHandoff(context);
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
