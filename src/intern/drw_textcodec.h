#ifndef DRW_TEXTCODEC_H
#define DRW_TEXTCODEC_H

#include <unordered_map>
#include <string>
#include <string_view>
#include <memory>
#include "../drw_base.h"

class DRW_Converter;

// Naming/selection provenance only, not proof that every byte in a mapping
// is qualified. Unavailable means this selection route has no implementation;
// compatibility aliases and unknown names retain their historical decoder.
enum class DRW_CodePageKind : std::uint8_t {
    Unspecified,
    Canonical,
    ExactAlias,
    CompatibilityAlias,
    Unavailable,
    Unknown
};

struct DRW_CodePageSelection {
    DRW_CodePageKind kind {DRW_CodePageKind::Unspecified};
    // Static classifier spelling, not a borrowed caller string. This is the
    // compatibility selection; modern DXF still uses UTF8 independently.
    const char* selectedName {nullptr};
};

// Internal input-selection contract. Neither policy is format admission or
// exhaustive table qualification. Existing readers/writers use Compatible;
// ExactNamed must never substitute a merely similar Windows mapping.
enum class DRW_CodePageMappingPolicy { Compatible, ExactNamed };
enum class DRW_CodePageMappingStatus {
    CompatibleSelection,
    ExactImplementationAvailable,
    Unavailable,
    Unknown
};

struct DRW_CodePageResolution {
    // Preserve the historical declaration classification/selection separately
    // from requested mapping identity and a future explicitly chosen decoder.
    DRW_CodePageSelection declaration;
    // Static family spelling, or null when this resolver cannot identify it.
    // The full original declaration remains the separate HEADER provenance.
    const char* requestedName {nullptr};
    // Static implementation name; null means no eligible exact decoder.
    const char* decoderName {nullptr};
    DRW_CodePageMappingStatus status {DRW_CodePageMappingStatus::Unknown};
};

struct DRW_TextDecodeObservation {
    std::uint32_t issues {0};
    bool nonAsciiBytes {false};
    bool legacyBytes {false};
    bool recovery {false};
    bool exactNamedMapping {false};
    bool unavailableNamedMapping {false};
    DRW::DxfTextSourceProfile sourceProfile {DRW::DxfTextSourceProfile::NoProfile};
};

enum class DRW_TextSourceKind : std::uint8_t { Utf8, LegacyBytes, Utf16 };

class DRW_TextCodec
{
public:
    DRW_TextCodec();
    ~DRW_TextCodec();
    DRW_TextCodec(const DRW_TextCodec&) = delete;
    DRW_TextCodec& operator=(const DRW_TextCodec&) = delete;
    DRW_TextCodec(DRW_TextCodec&&) noexcept = default;
    DRW_TextCodec& operator=(DRW_TextCodec&&) noexcept = default;
    std::string fromUtf8(std::string_view s);
    std::string toUtf8(std::string_view s,
                       bool decodeDxfAsciiControlEscapes = false);
    // Last completed conversion only; the reader owns cumulative observations.
    DRW_TextDecodeObservation lastDecodeObservation() const noexcept {
        return m_lastDecodeObservation;
    }
    // Issues across semantic decode calls since the explicit reset. In
    // particular, a later clean value cannot hide earlier replacement/loss.
    // This is an observation, not an exhaustive mapping/schema certificate.
    std::uint32_t accumulatedDecodeIssues() const noexcept {
        return m_accumulatedDecodeIssues;
    }
    void resetAccumulatedDecodeIssues() noexcept {
        m_accumulatedDecodeIssues = 0u;
    }
    // Bounded subordinate decoders (e.g. per-value DWG EED codecs) can
    // contribute source issues without changing this codec's selection or
    // its last primary-conversion snapshot. This never grants admission.
    void mergeDecodeIssues(std::uint32_t issues) noexcept {
        m_accumulatedDecodeIssues |= issues;
    }
    // DXF reader only, immediately after primary toUtf8 with the same input.
    // Reports the effective decoder, not a normalized/requested source label.
    DRW::DxfTextSourceProfile dxfSourceProfile(std::string_view input) const noexcept;
    static bool isUtf8ScalarText(std::string_view text) noexcept;
    /// Convert byte-oriented DWG TV fields using the file codepage while the
    /// primary codec may remain UTF-16 for R2007+ fields.
    std::string fromUtf8CP8(std::string_view s);
    std::string toUtf8CP8(std::string_view s);
    int getVersion(){return version;}
    DRW::Version getSourceVersion() const { return sourceVersion; }
    bool isDxfFormat() const noexcept { return m_dxfFormat; }
    bool hasSourceVersion() const { return m_sourceVersionSet; }
    void setVersion(const std::string &v, bool dxfFormat);
    void setVersion(DRW::Version v, bool dxfFormat);
    void setCodePage(const std::string &c, bool dxfFormat);
    static DRW_CodePageSelection classifyCodePage(std::string_view name) noexcept;
    static DRW_CodePageResolution resolveCodePage(
        std::string_view name, DRW_CodePageMappingPolicy policy) noexcept;
    // Internal input-side converter, independent of version/wire profile.
    // Null for unavailable/unknown exact mappings; never changes Compatible
    // state. Encode availability is separate (existing UTF16 is read-only).
    static std::unique_ptr<DRW_Converter> createExactCodePageConverter(std::string_view name);
    DRW_CodePageSelection declaredCodePageSelection() const noexcept {
        return m_declaredCodePageSelection;
    }
    DRW_CodePageSelection byteCodePageSelection() const noexcept {
        return m_byteCodePageSelection;
    }
    // HEADER finalization reapplies normalized metadata after version
    // discovery without relabeling the original selection as canonical.
    void reapplyCodePage(const std::string& c, bool dxfFormat) {
        const auto declaration = m_declaredCodePageSelection;
        const auto inputResolution = m_dxfInputResolution;
        // Preserve the historical reapplication's recovery-eligibility
        // updates as well as its converter. Only provenance is restored.
        configureCodePage(c, dxfFormat, true);
        m_declaredCodePageSelection = declaration;
        m_dxfInputResolution = inputResolution;
    }
    void setDxfTextEncoding(DRW::DxfTextEncoding encoding) noexcept {
        m_dxfTextEncoding = encoding;
    }
    void setDxfCodePagePolicy(DRW::DxfCodePagePolicy policy) noexcept {
        m_dxfCodePagePolicy = policy;
    }
    DRW::DxfCodePagePolicy dxfCodePagePolicy() const noexcept { return m_dxfCodePagePolicy; }
    DRW::DxfTextEncoding dxfTextEncoding() const noexcept {return m_dxfTextEncoding;}
    void setByteCodePage(const std::string &c);
    std::string getCodePage(){return cp;}
    std::string_view codePageView() const noexcept {return cp;}
    void swap(DRW_TextCodec& other) noexcept;

private:
    void configureCodePage(const std::string& c, bool dxfFormat,
                           bool explicitDeclaration = false);
    void finishDecodeObservation(const DRW_Converter& converter,
                                 bool recovery = false,
                                 bool byteChannel = false) noexcept;

private:
    DRW::Version version{DRW::UNKNOWNV};
    // Until $ACADVER is read the source version is genuinely unknown.  Every
    // consumer of getSourceVersion() already treats UNKNOWNV as "do not apply
    // post-R12 strictness" (see requiresDxfSelfHandle() and
    // dxfTableEntryComplete() in libdxfrw.cpp), so defaulting to a concrete
    // modern version made those guards unreachable and rejected any DXF that
    // omits $ACADVER - including hand-written files and R12 output, whose
    // table records legitimately carry no handle.
    DRW::Version sourceVersion{DRW::UNKNOWNV};
    bool m_sourceVersionSet { false };
    DRW::DxfTextEncoding m_dxfTextEncoding {DRW::DxfTextEncoding::Declared};
    DRW::DxfCodePagePolicy m_dxfCodePagePolicy {DRW::DxfCodePagePolicy::Compatible};
    bool m_dxfFormat {false};
    DRW_CodePageResolution m_dxfInputResolution;
    std::unique_ptr<DRW_Converter> m_exactDxfInputConverter;
    bool m_legacyDxfInputEligible {false};
    bool m_legacyDxfCodePageAllowed {true};
    DRW_CodePageSelection m_declaredCodePageSelection;
    DRW_CodePageSelection m_byteCodePageSelection;
    std::string cp;
    std::unique_ptr< DRW_Converter> conv;
    std::unique_ptr< DRW_Converter> cp8Conv;
    DRW_TextDecodeObservation m_lastDecodeObservation;
    std::uint32_t m_accumulatedDecodeIssues{0u};
};

class DRW_Converter
{
public:
    DRW_Converter(const int *t, int l,
                  DRW_TextSourceKind kind = DRW_TextSourceKind::Utf8)
        :table{t }
        ,cpLength{l}
        ,m_sourceKind(kind)
    {}
    virtual ~DRW_Converter()=default;
    virtual std::string fromUtf8(std::string_view s) {return std::string{s};}
    virtual std::string toUtf8(std::string_view s);
    void setDecodeDxfAsciiControlEscapes(bool enabled) noexcept {
        m_decodeDxfAsciiControlEscapes = enabled;
    }
    DRW_TextDecodeObservation decodeObservation() const noexcept {
        return m_decodeObservation;
    }
    DRW_TextSourceKind sourceKind() const noexcept { return m_sourceKind; }
    std::string encodeText(const std::string& stmp);
    std::string decodeText(int c);
    std::string encodeNum(int c);
    /// Turn one double-table cell into UTF-8, expanding the packed pair the
    /// four big5-hkscs two-code-point sequences use.
    std::string decodeTableValue(int v);
    int decodeNum(const std::string& s, int *b);
    /// Decode a `\M+cXXXX` MIF escape (8 chars; c=selector 1..5, XXXX=hex
    /// of a 2-byte sequence in the selector's codepage) to UTF-8.
    /// Selector mapping: 1→ANSI_932, 2→ANSI_950, 3→ANSI_949,
    /// 4→Johab(unavailable), 5→ANSI_936. Returns empty on invalid or
    /// unavailable conversion so the caller retains the original token.
    std::string encodeMifText(const std::string& tok);
    const int *table{nullptr};
    int cpLength;

protected:
    void beginDecode() noexcept {
        m_decodeObservation = {};
        m_decodeObservation.legacyBytes = m_sourceKind == DRW_TextSourceKind::LegacyBytes;
    }
    void observeSourceByte(unsigned char value) noexcept {
        m_decodeObservation.nonAsciiBytes |= value >= 0x80u;
    }
    void markDecodeIssue(DRW::DxfTextIssue issue) noexcept {
        m_decodeObservation.issues |= DRW::dxfTextIssueMask(issue);
    }
    bool m_decodeDxfAsciiControlEscapes {false};
    // Decode one Autodesk ASCII-DXF caret pair at a byte position already
    // identified as ASCII by this converter. Returns the number of consumed
    // source bytes (zero when the bytes are not a recognized caret spelling).
    std::size_t decodeDxfAsciiControlEscape(std::string_view source,
                                            std::size_t offset,
                                            std::string& output);
    void observeEscapePrefix(std::string_view suffix) noexcept;
    /// Unicode -> DBCS, over the caller's cpLength-entry double table, built
    /// once per converter. Filled in table order, so a repeated code point
    /// keeps its first mapping - the one the linear scan this replaces
    /// returned.
    const std::unordered_map<int, int>& reverseIndex(const int (*doubles)[2]);

private:
    std::unordered_map<int, int> m_reverse;
    DRW_TextSourceKind m_sourceKind;
    DRW_TextDecodeObservation m_decodeObservation;
};

class DRW_ConvUTF16 : public DRW_Converter {
public:
    DRW_ConvUTF16():DRW_Converter(nullptr, 0, DRW_TextSourceKind::Utf16) {}
    std::string fromUtf8(std::string_view s) override;
    std::string toUtf8(std::string_view s) override;
};

class DRW_ConvTable : public DRW_Converter {
public:
    DRW_ConvTable(const int *t, int l):DRW_Converter(t, l, DRW_TextSourceKind::LegacyBytes) {}
    std::string fromUtf8(std::string_view s) override;
    std::string toUtf8(std::string_view s) override;
};

class DRW_ConvDBCSTable : public DRW_Converter {
public:
    DRW_ConvDBCSTable(const int *t,  const int *lt, const int dt[][2], int l)
        :DRW_Converter(t, l, DRW_TextSourceKind::LegacyBytes)
        ,leadTable{lt}
        ,doubleTable{dt}
    {}

    std::string fromUtf8(std::string_view s) override;
    std::string toUtf8(std::string_view s) override;
private:
    const int *leadTable{nullptr};
    const int (*doubleTable)[2];
};

class DRW_Conv932Table : public DRW_Converter {
public:
    DRW_Conv932Table();
    std::string fromUtf8(std::string_view s) override;
    std::string toUtf8(std::string_view s) override;

};

#endif // DRW_TEXTCODEC_H
