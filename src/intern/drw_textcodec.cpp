#include "drw_textcodec.h"
#include <cstdio>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <utility>
#include "../drw_base.h"
#include "drw_cptables.h"
#include "drw_cptable932.h"
#include "drw_cptable936.h"
#include "drw_cptable949.h"
#include "drw_cptable950.h"
#include "drw_cptable_iso8859_4.h"

namespace {
bool codePageNameEquals(std::string_view source, std::string_view key) noexcept {
    if (source.size() != key.size()) return false;
    for (std::size_t i = 0; i < source.size(); ++i) {
        const auto byte = static_cast<unsigned char>(source[i]);
        const auto folded = byte >= 'a' && byte <= 'z'
            ? static_cast<unsigned char>(byte - 'a' + 'A') : byte;
        if (folded != static_cast<unsigned char>(key[i])) return false;
    }
    return true;
}

// Share the existing UTF8 scalar rules with input observations, without a
// substring allocation. Malformed input consumes one byte, as before.
int decodeUtf8Scalar(std::string_view text, int* consumed) noexcept {
    if (consumed == nullptr) return -1;
    *consumed = 1;
    if (text.empty()) return -1;
    const auto lead = static_cast<unsigned char>(text.front());
    std::size_t length = 0;
    std::uint32_t value = 0, minimum = 0;
    if (lead >= 0xC2u && lead <= 0xDFu) {
        length = 2; value = lead & 0x1Fu; minimum = 0x80u;
    } else if (lead >= 0xE0u && lead <= 0xEFu) {
        length = 3; value = lead & 0x0Fu; minimum = 0x800u;
    } else if (lead >= 0xF0u && lead <= 0xF4u) {
        length = 4; value = lead & 0x07u; minimum = 0x10000u;
    } else return -1;
    if (text.size() < length) return -1;
    for (std::size_t i = 1; i < length; ++i) {
        const auto next = static_cast<unsigned char>(text[i]);
        if ((next & 0xC0u) != 0x80u) return -1;
        value = (value << 6u) | (next & 0x3Fu);
    }
    if (value < minimum || value > 0x10FFFFu
        || (value >= 0xD800u && value <= 0xDFFFu)) return -1;
    *consumed = static_cast<int>(length);
    return static_cast<int>(value);
}

bool asciiHex(unsigned char value) noexcept {
    return (value >= '0' && value <= '9') || (value >= 'A' && value <= 'F')
        || (value >= 'a' && value <= 'f');
}

// Every primary/exact/recovery/byte-channel return contributes without
// retaining source text or changing the existing last-value observation.
struct DecodeIssueCollector {
    const DRW_TextDecodeObservation& last;
    std::uint32_t& accumulated;
    ~DecodeIssueCollector() noexcept { accumulated |= last.issues; }
};
} // namespace

bool DRW_TextCodec::isUtf8ScalarText(std::string_view text) noexcept {
    for (std::size_t i = 0; i < text.size();) {
        if (static_cast<unsigned char>(text[i]) < 0x80u) { ++i; continue; }
        int length = 1;
        if (decodeUtf8Scalar(text.substr(i), &length) < 0) return false;
        i += static_cast<std::size_t>(length);
    }
    return true;
}

DRW_TextCodec::DRW_TextCodec()
    : version{DRW::AC1021}
    , conv( new DRW_Converter(nullptr, 0) )
{
}

DRW_TextCodec::~DRW_TextCodec() = default;

void DRW_TextCodec::swap(DRW_TextCodec& other) noexcept {
    using std::swap;
    swap(version, other.version);
    swap(sourceVersion, other.sourceVersion);
    swap(m_sourceVersionSet, other.m_sourceVersionSet);
    swap(m_dxfTextEncoding, other.m_dxfTextEncoding);
    swap(m_dxfCodePagePolicy, other.m_dxfCodePagePolicy);
    swap(m_dxfFormat, other.m_dxfFormat);
    swap(m_dxfInputResolution, other.m_dxfInputResolution);
    swap(m_exactDxfInputConverter, other.m_exactDxfInputConverter);
    swap(m_legacyDxfInputEligible, other.m_legacyDxfInputEligible);
    swap(m_legacyDxfCodePageAllowed, other.m_legacyDxfCodePageAllowed);
    swap(m_declaredCodePageSelection, other.m_declaredCodePageSelection);
    swap(m_byteCodePageSelection, other.m_byteCodePageSelection);
    swap(cp, other.cp);
    swap(conv, other.conv);
    swap(cp8Conv, other.cp8Conv);
    swap(m_lastDecodeObservation, other.m_lastDecodeObservation);
    swap(m_accumulatedDecodeIssues, other.m_accumulatedDecodeIssues);
}

void DRW_TextCodec::setVersion(DRW::Version v, bool dxfFormat){
    m_dxfFormat = dxfFormat;
    sourceVersion = v;
    m_sourceVersionSet = v != DRW::UNKNOWNV;
    m_legacyDxfInputEligible = false;
    switch (v)
    {
        case DRW::UNKNOWNV:
        case DRW::MC00:
        case DRW::AC12:
        case DRW::AC14:
        case DRW::AC150:
        case DRW::AC210:
        case DRW::AC1002:
        case DRW::AC1003:
        case DRW::AC1004:
            // unhandled?
            break;

        case DRW::AC1006:
        case DRW::AC1009:
        {
            version = DRW::AC1009;
            cp = "ANSI_1252";
            configureCodePage( cp, dxfFormat);
            break;
        }

        case DRW::AC1012:
        case DRW::AC1014:
        case DRW::AC1015:
        case DRW::AC1018:
        {
            version = DRW::AC1015;
//            if (cp.empty()) { //codepage not set, initialize ??
                cp = "ANSI_1252";
                configureCodePage( cp, dxfFormat);
//          }
            break;
        }

        case DRW::AC1021:
        case DRW::AC1024:
        case DRW::AC1027:
        case DRW::AC1032:
        {
            version = DRW::AC1021;
            if (dxfFormat) {
                cp = "UTF-8";
                configureCodePage(cp, dxfFormat);
            } else {
                if (cp.empty() || cp == "ANSI_1252") {
                    cp = "UTF-16";
                    configureCodePage(cp, dxfFormat);
                }
            }
            break;
        }
    }
}

void DRW_TextCodec::setVersion(const std::string &v, bool dxfFormat){
    m_dxfFormat = dxfFormat;
    version = DRW::UNKNOWNV;
    sourceVersion = DRW::UNKNOWNV;
    m_sourceVersionSet = false;
    m_legacyDxfInputEligible = false;
    for (const auto& [verName, verId] : DRW::dwgVersionStrings)
    {
        if ( v == verName ) {
            version = verId;
            setVersion( verId, dxfFormat);
            break;
        }
    }
}

void DRW_TextCodec::setCodePage(const std::string &c, bool dxfFormat){
    configureCodePage(c, dxfFormat, true);
}

void DRW_TextCodec::configureCodePage(const std::string& c, bool dxfFormat,
                                    bool explicitDeclaration) {
    m_dxfFormat = dxfFormat;
    m_exactDxfInputConverter.reset();
    const auto selection = resolveCodePage(c, DRW_CodePageMappingPolicy::Compatible).declaration;
    cp = selection.selectedName;
    // Internal version defaults do not erase an earlier explicit non-1252
    // declaration. HEADER finalization reapplies the final declared page.
    if (dxfFormat && explicitDeclaration)
        m_legacyDxfCodePageAllowed = cp == "ANSI_1252";
    m_legacyDxfInputEligible = dxfFormat && m_sourceVersionSet
        && sourceVersion >= DRW::AC1006 && sourceVersion <= DRW::AC1018
        && cp == "ANSI_1252" && m_legacyDxfCodePageAllowed;
    conv.reset();
    // R2007+ DXF string bytes are UTF-8 independently of the retained
    // $DWGCODEPAGE header. That legacy metadata must not switch either the
    // reader or writer back to a byte-codepage converter. Keep DWG TV/TU
    // selection (dxfFormat=false) entirely separate.
    if (dxfFormat && m_sourceVersionSet && sourceVersion >= DRW::AC1021) {
        if (cp == "UTF-8")
            cp = "ANSI_1252"; // preserve the established header spelling
        conv.reset(new DRW_Converter(nullptr, 0));
    } else if (cp == "ANSI_1252")
        conv.reset(new DRW_ConvTable(DRW_Table1252, CPLENGTHCOMMON));
    else if (cp == "ANSI_874")
        conv.reset( new DRW_ConvTable(DRW_Table874, CPLENGTHCOMMON) );
    else if (cp == "ANSI_932")
        conv.reset( new DRW_Conv932Table() );
    else if (cp == "ANSI_936")
        conv.reset( new DRW_ConvDBCSTable(DRW_Table936, DRW_LeadTable936,
                                     DRW_DoubleTable936, CPLENGTH936) );
    else if (cp == "ANSI_949")
        conv.reset( new DRW_ConvDBCSTable(DRW_Table949, DRW_LeadTable949,
                                     DRW_DoubleTable949, CPLENGTH949) );
    else if (cp == "ANSI_950")
        conv.reset( new DRW_ConvDBCSTable(DRW_Table950, DRW_LeadTable950,
                                     DRW_DoubleTable950, CPLENGTH950) );
    else if (cp == "ANSI_1250")
        conv.reset( new DRW_ConvTable(DRW_Table1250, CPLENGTHCOMMON) );
    else if (cp == "ANSI_1251")
        conv.reset( new DRW_ConvTable(DRW_Table1251, CPLENGTHCOMMON) );
    else if (cp == "ANSI_1253")
        conv.reset( new DRW_ConvTable(DRW_Table1253, CPLENGTHCOMMON) );
    else if (cp == "ANSI_1254")
        conv.reset( new DRW_ConvTable(DRW_Table1254, CPLENGTHCOMMON) );
    else if (cp == "ANSI_1255")
        conv.reset( new DRW_ConvTable(DRW_Table1255, CPLENGTHCOMMON) );
    else if (cp == "ANSI_1256")
        conv.reset( new DRW_ConvTable(DRW_Table1256, CPLENGTHCOMMON) );
    else if (cp == "ANSI_1257")
        conv.reset( new DRW_ConvTable(DRW_Table1257, CPLENGTHCOMMON) );
    else if (cp == "ANSI_1258")
        conv.reset( new DRW_ConvTable(DRW_Table1258, CPLENGTHCOMMON) );
    else if (cp == "MACINTOSH")
        conv.reset( new DRW_ConvTable(DRW_TableMacRoman, CPLENGTHCOMMON) );
    else if (cp == "UTF-8") {
        cp = "ANSI_1252";
        conv.reset( new DRW_Converter(nullptr, 0) );
    } else if (cp == "UTF-16") {
        conv.reset( new DRW_ConvUTF16() );
    } else if (dxfFormat) {
        conv.reset( new DRW_Converter(nullptr, 0) );
    } else {
        conv.reset( new DRW_ConvTable(DRW_Table1252, CPLENGTHCOMMON) );
    }
    if (explicitDeclaration) {
        m_declaredCodePageSelection = selection;
        m_dxfInputResolution = resolveCodePage(c, DRW_CodePageMappingPolicy::ExactNamed);
    }
}

std::string DRW_TextCodec::toUtf8(
        std::string_view s, bool decodeDxfAsciiControlEscapes) {
    m_lastDecodeObservation = {};
    const DecodeIssueCollector collect{m_lastDecodeObservation, m_accumulatedDecodeIssues};
    const bool decodeDxfAsciiCaret = decodeDxfAsciiControlEscapes && m_dxfFormat;
    const bool exactPolicy = m_dxfFormat
        && m_dxfCodePagePolicy != DRW::DxfCodePagePolicy::Compatible
        && (!m_sourceVersionSet || sourceVersion < DRW::AC1021);
    if (exactPolicy && (m_declaredCodePageSelection.kind != DRW_CodePageKind::Unspecified
                       || m_dxfCodePagePolicy != DRW::DxfCodePagePolicy::ExactNamed)) {
        const bool available = m_dxfCodePagePolicy == DRW::DxfCodePagePolicy::ExactNamed
            && m_dxfInputResolution.status == DRW_CodePageMappingStatus::ExactImplementationAvailable;
        const bool recovering1252 = available && m_dxfInputResolution.decoderName != nullptr
            && std::string_view(m_dxfInputResolution.decoderName) == "ANSI_1252"
            && m_dxfTextEncoding == DRW::DxfTextEncoding::LegacyLibreCadUtf8 && m_legacyDxfInputEligible;
        if (available && !recovering1252) {
            if (!m_exactDxfInputConverter)
                m_exactDxfInputConverter = createExactCodePageConverter(m_dxfInputResolution.decoderName);
            m_exactDxfInputConverter->setDecodeDxfAsciiControlEscapes(decodeDxfAsciiCaret);
            auto result = m_exactDxfInputConverter->toUtf8(s);
            m_lastDecodeObservation = m_exactDxfInputConverter->decodeObservation();
            m_lastDecodeObservation.exactNamedMapping = true;
            return result;
        }
        if (!available) {
            const bool high = std::any_of(s.begin(), s.end(), [](unsigned char byte) { return byte >= 0x80u; });
            if (high || m_dxfCodePagePolicy != DRW::DxfCodePagePolicy::ExactNamed) {
                // No guessed decoder: retain ASCII lexemes and replace each
                // uninterpretable high byte with U+FFFD. Do not parse escapes
                // across bytes of an unknown multibyte grammar. This tolerant
                // result is explicitly ineligible for complete conversion.
                std::string result;
                for (unsigned char byte : s) {
                    if (byte >= 0x80u) result += "\xEF\xBF\xBD";
                    else result.push_back(static_cast<char>(byte));
                }
                m_lastDecodeObservation.nonAsciiBytes = high;
                m_lastDecodeObservation.legacyBytes = true;
                m_lastDecodeObservation.unavailableNamedMapping = true;
                m_lastDecodeObservation.issues = DRW::dxfTextIssueMask(DRW::DxfTextIssue::UnavailableCodePage);
                if (high) m_lastDecodeObservation.issues |= DRW::dxfTextIssueMask(DRW::DxfTextIssue::ReplacedSource);
                return result;
            }
            DRW_Converter independent(nullptr, 0);
            independent.setDecodeDxfAsciiControlEscapes(decodeDxfAsciiCaret);
            auto result = independent.toUtf8(s); // ASCII/represented CIF/MIF only
            m_lastDecodeObservation = independent.decodeObservation();
            return result;
        }
    }
    if (m_dxfTextEncoding == DRW::DxfTextEncoding::LegacyLibreCadUtf8
        && m_legacyDxfInputEligible) {
        // Preserve UTF-8 input bytes while retaining Unicode/MIF escapes.
        // Do not replace conv: fromUtf8 still uses the declared output codec.
        DRW_Converter utf8(nullptr, 0);
        utf8.setDecodeDxfAsciiControlEscapes(decodeDxfAsciiCaret);
        auto result = utf8.toUtf8(s);
        finishDecodeObservation(utf8, true);
        return result;
    }
    conv->setDecodeDxfAsciiControlEscapes(decodeDxfAsciiCaret);
    auto result = conv->toUtf8(s);
    finishDecodeObservation(*conv);
    return result;
}

void DRW_TextCodec::finishDecodeObservation(const DRW_Converter& converter,
                                           bool recovery, bool byteChannel) noexcept {
    m_lastDecodeObservation = converter.decodeObservation();
    m_lastDecodeObservation.recovery = recovery;
    if (!m_lastDecodeObservation.nonAsciiBytes || !m_lastDecodeObservation.legacyBytes)
        return;
    const auto selection = byteChannel ? m_byteCodePageSelection : m_declaredCodePageSelection;
    using Issue = DRW::DxfTextIssue;
    switch (selection.kind) {
    case DRW_CodePageKind::Unspecified:
        m_lastDecodeObservation.issues |= DRW::dxfTextIssueMask(Issue::UnspecifiedCodePage); break;
    case DRW_CodePageKind::CompatibilityAlias:
        m_lastDecodeObservation.issues |= DRW::dxfTextIssueMask(Issue::CompatibilityCodePage); break;
    case DRW_CodePageKind::Unavailable:
        m_lastDecodeObservation.issues |= DRW::dxfTextIssueMask(Issue::UnavailableCodePage); break;
    case DRW_CodePageKind::Unknown:
        m_lastDecodeObservation.issues |= DRW::dxfTextIssueMask(Issue::UnknownCodePage); break;
    default: break;
    }
    if (!byteChannel && selection.selectedName != nullptr && selection.selectedName != cp)
        m_lastDecodeObservation.issues |= DRW::dxfTextIssueMask(Issue::ReappliedCodePage);
}

DRW::DxfTextSourceProfile DRW_TextCodec::dxfSourceProfile(std::string_view input) const noexcept {
    using Profile = DRW::DxfTextSourceProfile;
    if (input.empty()) return Profile::AsciiOrEscapes;
    if (m_sourceVersionSet && sourceVersion >= DRW::AC1021)
        return Profile::ModernUtf8;
    if (m_lastDecodeObservation.unavailableNamedMapping)
        return Profile::UnavailableNamed;
    const std::string_view name = m_lastDecodeObservation.exactNamedMapping
        ? std::string_view(m_dxfInputResolution.decoderName == nullptr ? "" : m_dxfInputResolution.decoderName)
        : std::string_view(cp);
    const bool utf16 = m_lastDecodeObservation.exactNamedMapping
        ? name == "UTF-16" : conv->sourceKind() == DRW_TextSourceKind::Utf16;
    // Nonempty TU-style bytes are not decoder-independent ASCII DXF text.
    if (utf16) return Profile::LegacyUtf16;
    if (!m_lastDecodeObservation.nonAsciiBytes) return Profile::AsciiOrEscapes;
    if (m_lastDecodeObservation.recovery) return Profile::RecoveredLegacyUtf8;
    const bool utf8 = m_lastDecodeObservation.exactNamedMapping
        ? name == "UTF-8" : conv->sourceKind() == DRW_TextSourceKind::Utf8;
    if (utf8) return Profile::NonModernUtf8;
    struct Mapping { std::string_view name; Profile profile; };
    static constexpr Mapping mappings[] = {
        {"ANSI_874", Profile::Windows874}, {"ANSI_932", Profile::Windows932},
        {"ANSI_936", Profile::Windows936}, {"ANSI_949", Profile::Windows949},
        {"ANSI_950", Profile::Windows950}, {"ANSI_1250", Profile::Windows1250},
        {"ANSI_1251", Profile::Windows1251}, {"ANSI_1252", Profile::Windows1252},
        {"ANSI_1253", Profile::Windows1253}, {"ANSI_1254", Profile::Windows1254},
        {"ANSI_1255", Profile::Windows1255}, {"ANSI_1256", Profile::Windows1256},
        {"ANSI_1257", Profile::Windows1257}, {"ANSI_1258", Profile::Windows1258},
        {"ISO8859-4", Profile::Iso8859_4}, {"ISO8859-1", Profile::Iso8859_1},
        {"MACINTOSH", Profile::MacRoman}, {"ISO8859-2", Profile::Iso8859_2},
        {"ISO8859-3", Profile::Iso8859_3}, {"ISO8859-5", Profile::Iso8859_5},
        {"ISO8859-6", Profile::Iso8859_6}, {"ISO8859-7", Profile::Iso8859_7},
        {"ISO8859-8", Profile::Iso8859_8},
        {"ISO8859-9", Profile::Iso8859_9},
        {"ISO8859-10", Profile::Iso8859_10},
        {"ISO8859-11", Profile::Iso8859_11},
        {"ISO8859-13", Profile::Iso8859_13},
        {"ISO8859-14", Profile::Iso8859_14},
        {"ISO8859-15", Profile::Iso8859_15}
    };
    for (const auto& mapping : mappings)
        if (name == mapping.name) return mapping.profile;
    return Profile::Unidentified;
}

std::string DRW_TextCodec::fromUtf8(std::string_view s) {
    return conv->fromUtf8(s);
}

std::string DRW_TextCodec::fromUtf8CP8(std::string_view s) {
    return cp8Conv ? cp8Conv->fromUtf8(s) : std::string{s};
}

std::string DRW_TextCodec::toUtf8CP8(std::string_view s) {
    m_lastDecodeObservation = {};
    const DecodeIssueCollector collect{m_lastDecodeObservation, m_accumulatedDecodeIssues};
    if (!cp8Conv) {
        // Preserve the historical pass-through result/last-value snapshot,
        // but do not certify unprofiled high bytes during a bounded DWG audit.
        if (std::any_of(s.begin(), s.end(), [](unsigned char byte) { return byte >= 0x80u; }))
            m_accumulatedDecodeIssues |= DRW::dxfTextIssueMask(DRW::DxfTextIssue::UnsupportedEncodingProfile);
        return std::string{s};
    }
    auto result = cp8Conv->toUtf8(s);
    finishDecodeObservation(*cp8Conv, false, true);
    return result;
}

void DRW_TextCodec::setByteCodePage(const std::string &c) {
    DRW_TextCodec byteCodec;
    byteCodec.setCodePage(c, false);
    cp8Conv = std::move(byteCodec.conv);
    m_byteCodePageSelection = byteCodec.m_declaredCodePageSelection;
}

std::string DRW_Converter::toUtf8(std::string_view s) {
    beginDecode();
    std::string result;
    int j = 0;
    unsigned int i= 0;
    for (i=0; i < s.length(); i++) {
        unsigned char c = s.at(i);
        observeSourceByte(c);
        if (c >= 0x80u) {
            int length = 1;
            if (decodeUtf8Scalar(s.substr(i), &length) < 0) {
                markDecodeIssue(DRW::DxfTextIssue::InvalidUtf8);
                markDecodeIssue(DRW::DxfTextIssue::InvalidUnicodeOutput);
            }
        }
        if (c == '^' && m_decodeDxfAsciiControlEscapes) {
            std::string decoded;
            const std::size_t consumed = decodeDxfAsciiControlEscape(s, i, decoded);
            if (consumed != 0u) {
                result += s.substr(j, i-j);
                result += decoded;
                i += static_cast<unsigned int>(consumed - 1u);
                j = i + 1u;
            }
        }
        if (c < 0x80) { //ascii check for \\U+???? or \\M+cXXXX
            if (c == '\\') observeEscapePrefix(s.substr(i));
            if (c == '\\' && i+6 < s.length() && s.at(i+1) == 'U' && s.at(i+2) == '+') {
                result += s.substr(j,i-j);
                result += encodeText(std::string{s.substr(i,7)});
                i +=6;
                j = i+1;
            } else if (c == '\\' && i+7 < s.length() && s.at(i+1) == 'M' && s.at(i+2) == '+') {
                std::string mif = encodeMifText(std::string{s.substr(i, 8)});
                if (!mif.empty()) {
                    result += s.substr(j, i-j);
                    result += mif;
                    i += 7;
                    j = i+1;
                }
            }
        } else if (c < 0xE0 ) {//2 bits
            i++;
        } else if (c < 0xF0 ) {//3 bits
            i +=2;
        } else if (c < 0xF8 ) {//4 bits
            i +=3;
        }
    }
    result += s.substr(j);

    return result;
}

std::size_t DRW_Converter::decodeDxfAsciiControlEscape(
        std::string_view source, std::size_t offset, std::string& output) {
    if (!m_decodeDxfAsciiControlEscapes || offset >= source.size()
        || source[offset] != '^' || source.size() - offset < 2u)
        return 0u;

    const unsigned char escaped = static_cast<unsigned char>(source[offset + 1u]);
    if (escaped == ' ') {
        output.push_back('^');
        return 2u;
    }

    unsigned char control = 0u;
    if (escaped >= '@' && escaped <= '_')
        control = static_cast<unsigned char>(escaped & 0x1Fu);
    else if (escaped == '?')
        control = 0x7Fu;
    else
        return 0u;

    if (control == 0u) {
        // NUL remains outside the supported semantic string domain. Preserve
        // its visible spelling rather than introducing an embedded NUL into
        // downstream model strings.
        markDecodeIssue(DRW::DxfTextIssue::UnrepresentedEscape);
        output.append(source.data() + offset, 2u);
        return 2u;
    }
    output.push_back(static_cast<char>(control));
    return 2u;
}

std::string DRW_ConvTable::fromUtf8(std::string_view s) {
    std::string result;
    bool notFound;
    int code;

    int j = 0;
    for (unsigned int i=0; i < s.length(); i++) {
        unsigned char c = s.at(i);
        if (c > 0x7F) { //need to decode
            result += s.substr(j,i-j);
            std::string part1{s.substr(i,4)};
            int l;
            code = decodeNum(part1, &l);
            j = i+l;
            i = j - 1;
            notFound = true;
            for (int k=0; k<cpLength; k++){
                if(table[k] == code) {
                    result += CPOFFSET + k; //translate from table
                    notFound = false;
                    break;
                }
            }
            if (notFound)
                result += decodeText(code);
        }
    }
    result += s.substr(j);

    return result;
}

std::string DRW_ConvTable::toUtf8(std::string_view s) {
    beginDecode();
    std::string res;
    for ( auto it=s.begin() ; it < s.end(); ++it ) {
        unsigned char c = *it;
        observeSourceByte(c);
        // \U+ and \M+ escapes are 7-bit ASCII anchors; valid even when an
        // adjacent table byte is high. Test the backslash branch regardless
        // of `c < 0x80` so an escape immediately following a table-mapped
        // byte is still detected.
        if (c == '\\') {
            observeEscapePrefix(s.substr(static_cast<std::size_t>(it - s.begin())));
            if (s.end()-it > 6 && *(it+1) == 'U' && *(it+2) == '+') {
                res += encodeText(std::string(it, it+7));
                it += 6;
                continue;
            }
            if (s.end()-it > 7 && *(it+1) == 'M' && *(it+2) == '+') {
                std::string mif = encodeMifText(std::string(it, it+8));
                if (!mif.empty()) {
                    res += mif;
                    it += 7;
                    continue;
                }
            }
            res += c; // literal backslash
        } else if (c < 0x80) {
            if (c == '^' && m_decodeDxfAsciiControlEscapes) {
                std::string decoded;
                const auto offset = static_cast<std::size_t>(it - s.begin());
                const std::size_t consumed = decodeDxfAsciiControlEscape(s, offset, decoded);
                if (consumed != 0u) {
                    res += decoded;
                    it += static_cast<std::ptrdiff_t>(consumed - 1u);
                    continue;
                }
            }
            res += c; // ascii char write
        } else {
            if (table[c-0x80] == 0) {
                markDecodeIssue(DRW::DxfTextIssue::UndefinedByteSequence);
                markDecodeIssue(DRW::DxfTextIssue::DiscardedSource);
            }
            res += encodeNum(table[c-0x80]); // translate from table
        }
    } //end for

    return res;
}

std::string DRW_Converter::encodeText(const std::string &stmp){
    observeEscapePrefix(stmp);
    // CIF is a fixed-width token: exactly four ASCII hex digits follow `\\U+`.
    // scanf/iostream extraction accepts a valid prefix (e.g. `12` from `12G4`)
    // and can turn malformed input into NUL or partially decoded text. Keep
    // malformed spelling intact; its observation still makes strict source
    // admission reject the ambiguous value.
    if (stmp.size() != 7 || stmp[0] != '\\' || stmp[1] != 'U' || stmp[2] != '+') {
        markDecodeIssue(DRW::DxfTextIssue::MalformedEscape);
        markDecodeIssue(DRW::DxfTextIssue::UnrepresentedEscape);
        return stmp;
    }

    unsigned int code = 0;
    for (std::size_t i = 3; i < 7; ++i) {
        const unsigned char c = static_cast<unsigned char>(stmp[i]);
        unsigned int digit = 0;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
        else {
            markDecodeIssue(DRW::DxfTextIssue::MalformedEscape);
            markDecodeIssue(DRW::DxfTextIssue::UnrepresentedEscape);
            return stmp;
        }
        code = (code << 4) | digit;
    }

    // A NUL would be truncated by C-string consumers; UTF-16 surrogate code
    // units are not Unicode scalar values and must not be emitted as UTF-8.
    // Keep either token literal and surface the unrepresented/invalid status.
    if (code == 0) {
        markDecodeIssue(DRW::DxfTextIssue::UnrepresentedEscape);
        return stmp;
    }
    if (code >= 0xD800 && code <= 0xDFFF) {
        markDecodeIssue(DRW::DxfTextIssue::InvalidUnicodeOutput);
        markDecodeIssue(DRW::DxfTextIssue::UnrepresentedEscape);
        return stmp;
    }
    return encodeNum(static_cast<int>(code));
}

std::string DRW_Converter::encodeMifText(const std::string &tok){
    // tok layout: "\M+cXXXX" — selector at index 3, 4 hex digits at [4..8).
    if (tok.size() != 8 || tok[0] != '\\' || tok[1] != 'M' || tok[2] != '+') {
        markDecodeIssue(DRW::DxfTextIssue::MalformedEscape);
        markDecodeIssue(DRW::DxfTextIssue::UnrepresentedEscape);
        return std::string{};
    }
    observeEscapePrefix(tok);
    // Numeric stream/scanf conversion accepts signs, whitespace and partial
    // hex. Validate the complete ASCII payload first on every platform, so a
    // malformed escape stays literal in the caller instead of creating a
    // partially decoded character (including TAB/CR).
    for (std::size_t i = 4; i < 8; ++i) {
        const unsigned char ch = static_cast<unsigned char>(tok[i]);
        if (!((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F')
              || (ch >= 'a' && ch <= 'f'))) {
            markDecodeIssue(DRW::DxfTextIssue::UnrepresentedEscape);
            return std::string{};
        }
    }
    const char* cpName = nullptr;
    switch (tok[3]) {
        case '1': cpName = "ANSI_932"; break; // Shift-JIS
        case '2': cpName = "ANSI_950"; break; // Big5
        case '3': cpName = "ANSI_949"; break; // EUC-KR
        // Selector4 names Johab1361, not CP1252. There is no Johab decoder
        // here; let the existing callers preserve the original MIF spelling.
        case '4':
            markDecodeIssue(DRW::DxfTextIssue::UnrepresentedEscape);
            return std::string{};
        case '5': cpName = "ANSI_936"; break; // GB18030/GBK
        default:
            markDecodeIssue(DRW::DxfTextIssue::UnrepresentedEscape);
            return std::string{};
    }
    int code = 0;
#if defined(__APPLE__)
    // As in encodeText: %x needs an unsigned int.
    unsigned int scanned = 0;
    int succeeded = sscanf(&(tok.substr(4,4)[0]), "%x", &scanned);
    if (!succeeded || succeeded == EOF)
        return std::string{};
    code = static_cast<int>(scanned);
#else
    std::istringstream sd(tok.substr(4, 4));
    sd >> std::hex >> code;
    if (!sd) return std::string{};
#endif
    // A MIF escape names a double-byte character, so the lead byte is
    // code >> 8. Below 0x100 that lead is zero, which is not a DBCS lead at
    // all: the pair {0x00, low} decoded straight through as a NUL, and a NUL
    // in a layer name makes the DXF writers refuse the whole string. Reject
    // it so the caller keeps the escape as literal text instead.
    if (code < 0x100) {
        markDecodeIssue(DRW::DxfTextIssue::MalformedEscape);
        markDecodeIssue(DRW::DxfTextIssue::UnrepresentedEscape);
        return std::string{};
    }
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1015, /*dxfFormat=*/false);
    codec.setCodePage(cpName, /*dxfFormat=*/false);
    std::string raw;
    raw.push_back(static_cast<char>((code >> 8) & 0xFF));
    raw.push_back(static_cast<char>(code & 0xFF));
    auto result = codec.toUtf8(raw);
    m_decodeObservation.issues |= codec.lastDecodeObservation().issues;
    return result;
}

void DRW_Converter::observeEscapePrefix(std::string_view suffix) noexcept {
    if (suffix.size() < 3 || suffix[0] != '\\' || suffix[2] != '+'
        || (suffix[1] != 'U' && suffix[1] != 'M')) return;
    const bool mif = suffix[1] == 'M';
    const std::size_t needed = mif ? 8u : 7u;
    const std::size_t available = (std::min)(needed, suffix.size());
    bool valid = suffix.size() >= needed;
    for (std::size_t i = 3; i < available; ++i) {
        const auto value = static_cast<unsigned char>(suffix[i]);
        observeSourceByte(value);
        if (mif && i == 3) valid &= value >= '1' && value <= '5';
        else valid &= asciiHex(value);
    }
    if (!valid) markDecodeIssue(DRW::DxfTextIssue::MalformedEscape);
    if (suffix.size() < needed) markDecodeIssue(DRW::DxfTextIssue::UnrepresentedEscape);
}

std::string DRW_Converter::decodeText(int c){
    // \U+ carries exactly four hex digits, so it cannot express a code point
    // above the BMP. %04X is a minimum width, not a maximum, and the reader
    // side reads a fixed four: emitting five digits made U+20021 come back as
    // U+2002 followed by '1'. Fall back to the same '?' an unmappable
    // double-byte sequence already produces.
    if (c < 0 || c > 0xFFFF)
        return "?";
    // Format into a bounded buffer, not a sized std::string: such a string
    // keeps its padding NULs, which the DXF writers reject and putCP8Text
    // would embed in the file.
    char digits[8];
    std::snprintf(digits, sizeof digits, "%04X", c);
    return std::string("\\U+") + digits;
}

std::string DRW_Converter::decodeTableValue(int v){
    // Four big5-hkscs sequences stand for two code points rather than one
    // (U+00CA/U+00EA followed by U+0304/U+030C). A table cell holds one int,
    // so those cells carry the pair packed as (first << 16) | second. Every
    // real code point is at most U+10FFFF, and the packed values start above
    // 0x00CA0000, so the two cannot be confused.
    if (v > 0x10FFFF)
        return encodeNum(v >> 16) + encodeNum(v & 0xFFFF);
    return encodeNum(v);
}

std::string DRW_Converter::encodeNum(int c){
    if (c == 0) markDecodeIssue(DRW::DxfTextIssue::DiscardedSource);
    if (c < 0 || c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF))
        markDecodeIssue(DRW::DxfTextIssue::InvalidUnicodeOutput);
    unsigned char ret[5];
    if (c < 128) { // 0-7F US-ASCII 7 bits
        ret[0] = c;
        ret[1] = 0;
    } else if (c < 0x800) { //80-07FF 2 bytes
        ret[0] = 0xC0 | (c >> 6);
        ret[1] = 0x80 | (c & 0x3f);
        ret[2] = 0;
    } else if (c< 0x10000) { //800-FFFF 3 bytes
        ret[0] = 0xe0 | (c >> 12);
        ret[1] = 0x80 | ((c >> 6) & 0x3f);
        ret[2] = 0x80 | (c & 0x3f);
        ret[3] = 0;
    } else { //10000-10FFFF 4 bytes
        ret[0] = 0xf0 | (c >> 18);
        ret[1] = 0x80 | ((c >> 12) & 0x3f);
        ret[2] = 0x80 | ((c >> 6) & 0x3f);
        ret[3] = 0x80 | (c & 0x3f);
        ret[4] = 0;
    }
    return std::string(reinterpret_cast<char*>(ret));
}

/**
 * Decode one UTF-8 code point.
 *
 * The caller passes a short view (at most four bytes) and uses @p b as the
 * number of source bytes consumed.  Invalid or truncated input deliberately
 * consumes one byte and returns -1.  That lets the code-page converters emit
 * their existing '?' fallback without throwing or swallowing a following
 * valid character.
 */
int DRW_Converter::decodeNum(const std::string &s, int *b){
    return decodeUtf8Scalar(s, b);
}


const std::unordered_map<int, int>& DRW_Converter::reverseIndex(
        const int (*doubles)[2]) {
    if (m_reverse.empty() && cpLength > 0 && doubles != nullptr) {
        m_reverse.reserve(static_cast<std::size_t>(cpLength));
        for (int k = 0; k < cpLength; ++k) {
            m_reverse.emplace(doubles[k][1], doubles[k][0]);
        }
    }
    return m_reverse;
}

std::string DRW_ConvDBCSTable::fromUtf8(std::string_view s) {
    const auto& index = reverseIndex(doubleTable);
    std::string result;
    int code;

    int j = 0;
    for (unsigned int i=0; i < s.length(); i++) {
        unsigned char c = s.at(i);
        if (c > 0x7F) { //need to decode
            result += s.substr(j,i-j);
            std::string part1{s.substr(i,4)};
            int l;
            code = decodeNum(part1, &l);
            j = i+l;
            i = j - 1;
            const auto it = index.find(code);
            if (it != index.end()) {
                result += static_cast<char>(it->second >> 8);
                result += static_cast<char>(it->second & 0xFF);
            } else {
                result += decodeText(code);
            }
        } //direct conversion
    }
    result += s.substr(j);

    return result;
}

std::string DRW_ConvDBCSTable::toUtf8(std::string_view s) {
    beginDecode();
    std::string res;
    for (auto it=s.begin() ; it < s.end(); ++it ) {
        bool notFound = true;
        unsigned char c = *it;
        observeSourceByte(c);
        if (c < 0x80) {
            notFound = false;
            if (c == '^' && m_decodeDxfAsciiControlEscapes) {
                std::string decoded;
                const auto offset = static_cast<std::size_t>(it - s.begin());
                const std::size_t consumed = decodeDxfAsciiControlEscape(s, offset, decoded);
                if (consumed != 0u) {
                    res += decoded;
                    it += static_cast<std::ptrdiff_t>(consumed - 1u);
                    continue;
                }
            }
            if (c == '\\') observeEscapePrefix(s.substr(static_cast<std::size_t>(it - s.begin())));
            // check for \U+ or \M+ encoded text (both are ASCII anchors;
            // backslash 0x5C is never a DBCS lead byte)
            if (c == '\\') {
                if (s.end()-it > 6 && *(it+1) == 'U' && *(it+2) == '+')  {
                    res += encodeText(std::string(it, it+7));
                    it +=6;
                } else if (s.end()-it > 7 && *(it+1) == 'M' && *(it+2) == '+') {
                    std::string mif = encodeMifText(std::string(it, it+8));
                    if (!mif.empty()) {
                        res += mif;
                        it += 7;
                    } else {
                        res += c;
                    }
                } else {
                    res +=c; //no \U+/\M+ encoded text write
                }
            } else
                res +=c; //c!='\' ascii char write
        } else if(c == 0x80 ){//1 byte table
            notFound = false;
            res += encodeNum(0x20AC);//euro sign
        } else if (it + 1 == s.end()) {
            // Trailing lead byte with no trail byte. Fixed-width DWG fields are
            // NUL-padded and can cut a double-byte character in half, so this is
            // reachable; fall through to the not-found replacement rather than
            // stepping past the end.
        } else {//2 bytes
            ++it;
            int code = (c << 8) | static_cast<unsigned char >(*it);
            int sta = leadTable[c-0x81];
            int end = leadTable[c-0x80];
            for (int k=sta; k<end; k++){
                if(doubleTable[k][0] == code) {
                    res += decodeTableValue(doubleTable[k][1]); //translate from table
                    notFound = false;
                    break;
                }
            }
        }
        //not found
        if (notFound) {
            markDecodeIssue(DRW::DxfTextIssue::UndefinedByteSequence);
            markDecodeIssue(DRW::DxfTextIssue::ReplacedSource);
            res += encodeNum(NOTFOUND936);
        }
    } //end for

    return res;
}

DRW_Conv932Table::DRW_Conv932Table()
    :DRW_Converter(DRW_Table932, CPLENGTH932, DRW_TextSourceKind::LegacyBytes) {
}

std::string DRW_Conv932Table::fromUtf8(std::string_view s) {
    const auto& index = reverseIndex(DRW_DoubleTable932);
    std::string result;
    bool notFound;
    int code;

    int j = 0;
    for (unsigned int i=0; i < s.length(); i++) {
        unsigned char c = s.at(i);
        if (c > 0x7F) { //need to decode
            result += s.substr(j,i-j);
            std::string part1{s.substr(i,4)};
            int l;
            code = decodeNum(part1, &l);
            j = i+l;
            i = j - 1;
            notFound = true;
            // 1 byte table
            if (code > 0xff60 && code < 0xFFA0) {
                result += code - CPOFFSET932; //translate from table
                notFound = false;
            }
            if (notFound && ( code<0xF8 || (code>0x390 && code<0x542) ||
                    (code>0x200F && code<0x9FA1) || code>0xF928 )) {
                const auto it = index.find(code);
                if (it != index.end()) {
                    result += static_cast<char>(it->second >> 8);
                    result += static_cast<char>(it->second & 0xFF);
                    notFound = false;
                }
            }
            if (notFound)
                result += decodeText(code);
        } //direct conversion
    }
    result += s.substr(j);

    return result;
}

std::string DRW_Conv932Table::toUtf8(std::string_view s) {
    beginDecode();
    std::string res;
    for (auto it=s.begin() ; it < s.end(); ++it ) {
        bool notFound = true;
        unsigned char c = *it;
        observeSourceByte(c);
        if (c < 0x80) {
            notFound = false;
            if (c == '^' && m_decodeDxfAsciiControlEscapes) {
                std::string decoded;
                const auto offset = static_cast<std::size_t>(it - s.begin());
                const std::size_t consumed = decodeDxfAsciiControlEscape(s, offset, decoded);
                if (consumed != 0u) {
                    res += decoded;
                    it += static_cast<std::ptrdiff_t>(consumed - 1u);
                    continue;
                }
            }
            if (c == '\\') observeEscapePrefix(s.substr(static_cast<std::size_t>(it - s.begin())));
            // check for \U+ or \M+ encoded text (SJIS lead bytes are
            // 0x81-0x9F and 0xE0-0xFC; backslash 0x5C is always ASCII)
            if (c == '\\') {
                if (s.end()-it > 6 && *(it+1) == 'U' && *(it+2) == '+')  {
                    res += encodeText(std::string(it, it+7));
                    it +=6;
                } else if (s.end()-it > 7 && *(it+1) == 'M' && *(it+2) == '+') {
                    std::string mif = encodeMifText(std::string(it, it+8));
                    if (!mif.empty()) {
                        res += mif;
                        it += 7;
                    } else {
                        res += c;
                    }
                } else {
                    res +=c; //no \U+/\M+ encoded text write
                }
            } else
                res +=c; //c!='\' ascii char write
        } else if(c > 0xA0 && c < 0xE0 ){//1 byte table
            notFound = false;
            res += encodeNum(c + CPOFFSET932); //translate from table
        } else if (it + 1 == s.end()) {
            // Trailing lead byte with no trail byte; see DRW_ConvDBCSTable.
        } else {//2 bytes
            ++it;
            int code = (c << 8) | static_cast<unsigned char>(*it);
            int sta=0;
            int end=0;
            if (c > 0x80 && c < 0xA0) {
                sta = DRW_LeadTable932[c-0x81];
                end = DRW_LeadTable932[c-0x80];
            } else if (c > 0xDF && c < 0xFD){
                sta = DRW_LeadTable932[c-0xC1];
                end = DRW_LeadTable932[c-0xC0];
            }
            if (end > 0) {
                for (int k=sta; k<end; k++){
                    if(DRW_DoubleTable932[k][0] == code) {
                        res += encodeNum(DRW_DoubleTable932[k][1]); //translate from table
                        notFound = false;
                        break;
                    }
                }
            }
        }
        //not found
        if (notFound) {
            markDecodeIssue(DRW::DxfTextIssue::UndefinedByteSequence);
            markDecodeIssue(DRW::DxfTextIssue::ReplacedSource);
            res += encodeNum(NOTFOUND932);
        }
    } //end for

    return res;
}

std::string DRW_ConvUTF16::fromUtf8(std::string_view s){
    DRW_UNUSED(s);
    //RLZ: to be written (only needed for write dwg 2007+)
    return std::string();
}

std::string DRW_ConvUTF16::toUtf8(std::string_view s){
    beginDecode();
    if ((s.size() & 1u) != 0u) {
        observeSourceByte(static_cast<unsigned char>(s.back()));
        markDecodeIssue(DRW::DxfTextIssue::MalformedUtf16);
        markDecodeIssue(DRW::DxfTextIssue::DiscardedSource);
    }
    // Decode UTF-16LE bytes to UTF-8. Combines surrogate pairs
    // 0xD800..0xDBFF + 0xDC00..0xDFFF into astral codepoints; an unpaired
    // half (high without matching low, or stray low) emits U+FFFD.
    std::string res;
    const size_t n = s.size() & ~static_cast<size_t>(1);
    for (size_t i = 0; i < n; i += 2) {
        observeSourceByte(static_cast<unsigned char>(s[i]));
        observeSourceByte(static_cast<unsigned char>(s[i + 1]));
        std::uint32_t ch =
            static_cast<std::uint8_t>(s[i]) |
            (static_cast<std::uint8_t>(s[i + 1]) << 8);
        if (ch >= 0xD800 && ch <= 0xDBFF) { // high surrogate
            if (i + 3 < n) {
                std::uint16_t lo =
                    static_cast<std::uint8_t>(s[i + 2]) |
                    (static_cast<std::uint8_t>(s[i + 3]) << 8);
                if (lo >= 0xDC00 && lo <= 0xDFFF) {
                    ch = 0x10000u + ((ch - 0xD800u) << 10) + (lo - 0xDC00u);
                    i += 2; // consume the low surrogate
                } else {
                    ch = 0xFFFD; // unpaired high
                    markDecodeIssue(DRW::DxfTextIssue::MalformedUtf16);
                    markDecodeIssue(DRW::DxfTextIssue::ReplacedSource);
                }
            } else {
                ch = 0xFFFD; // truncated high at end
                markDecodeIssue(DRW::DxfTextIssue::MalformedUtf16);
                markDecodeIssue(DRW::DxfTextIssue::ReplacedSource);
            }
        } else if (ch >= 0xDC00 && ch <= 0xDFFF) { // stray low surrogate
            ch = 0xFFFD;
            markDecodeIssue(DRW::DxfTextIssue::MalformedUtf16);
            markDecodeIssue(DRW::DxfTextIssue::ReplacedSource);
        }
        res += encodeNum(static_cast<int>(ch));
    }
    return res;
}

DRW_CodePageSelection DRW_TextCodec::classifyCodePage(std::string_view name) noexcept {
    struct Entry {
        std::string_view name;
        const char* selected;
        DRW_CodePageKind kind;
    };
    // Preserve established selections except for corrected exact mappings
    // such as APPLE ROMAN; kind describes naming policy, not full format
    // qualification.
    static constexpr Entry entries[] = {
        {"ANSI_874", "ANSI_874", DRW_CodePageKind::Canonical},
        {"CP874", "ANSI_874", DRW_CodePageKind::ExactAlias},
        {"ISO8859-11", "ANSI_874", DRW_CodePageKind::CompatibilityAlias},
        {"TIS-620", "ANSI_874", DRW_CodePageKind::CompatibilityAlias},
        {"ANSI_1250", "ANSI_1250", DRW_CodePageKind::Canonical},
        {"CP1250", "ANSI_1250", DRW_CodePageKind::ExactAlias},
        {"ISO8859-2", "ANSI_1250", DRW_CodePageKind::CompatibilityAlias},
        {"ANSI_1251", "ANSI_1251", DRW_CodePageKind::Canonical},
        {"CP1251", "ANSI_1251", DRW_CodePageKind::ExactAlias},
        {"ISO8859-5", "ANSI_1251", DRW_CodePageKind::CompatibilityAlias},
        {"KOI8-R", "ANSI_1251", DRW_CodePageKind::CompatibilityAlias},
        {"KOI8-U", "ANSI_1251", DRW_CodePageKind::CompatibilityAlias},
        {"IBM 866", "ANSI_1251", DRW_CodePageKind::CompatibilityAlias},
        {"ANSI_1252", "ANSI_1252", DRW_CodePageKind::Canonical},
        {"CP1252", "ANSI_1252", DRW_CodePageKind::ExactAlias},
        {"LATIN1", "ANSI_1252", DRW_CodePageKind::CompatibilityAlias},
        {"ISO-8859-1", "ANSI_1252", DRW_CodePageKind::CompatibilityAlias},
        {"CP819", "ANSI_1252", DRW_CodePageKind::CompatibilityAlias},
        {"CSISO", "ANSI_1252", DRW_CodePageKind::CompatibilityAlias},
        {"IBM819", "ANSI_1252", DRW_CodePageKind::CompatibilityAlias},
        {"ISO_8859-1", "ANSI_1252", DRW_CodePageKind::CompatibilityAlias},
        {"MACINTOSH", "MACINTOSH", DRW_CodePageKind::Canonical},
        {"APPLE ROMAN", "MACINTOSH", DRW_CodePageKind::ExactAlias},
        {"ISO8859-1", "ANSI_1252", DRW_CodePageKind::CompatibilityAlias},
        {"ISO8859-15", "ANSI_1252", DRW_CodePageKind::CompatibilityAlias},
        {"ISO-IR-100", "ANSI_1252", DRW_CodePageKind::CompatibilityAlias},
        {"L1", "ANSI_1252", DRW_CodePageKind::CompatibilityAlias},
        {"IBM 850", "ANSI_1252", DRW_CodePageKind::CompatibilityAlias},
        {"ANSI_1253", "ANSI_1253", DRW_CodePageKind::Canonical},
        {"CP1253", "ANSI_1253", DRW_CodePageKind::ExactAlias},
        {"ISO8859-7", "ANSI_1253", DRW_CodePageKind::CompatibilityAlias},
        {"ANSI_1254", "ANSI_1254", DRW_CodePageKind::Canonical},
        {"CP1254", "ANSI_1254", DRW_CodePageKind::ExactAlias},
        {"ISO8859-9", "ANSI_1254", DRW_CodePageKind::CompatibilityAlias},
        {"ISO8859-3", "ANSI_1254", DRW_CodePageKind::CompatibilityAlias},
        {"ANSI_1255", "ANSI_1255", DRW_CodePageKind::Canonical},
        {"CP1255", "ANSI_1255", DRW_CodePageKind::ExactAlias},
        {"ISO8859-8", "ANSI_1255", DRW_CodePageKind::CompatibilityAlias},
        {"ANSI_1256", "ANSI_1256", DRW_CodePageKind::Canonical},
        {"CP1256", "ANSI_1256", DRW_CodePageKind::ExactAlias},
        {"ISO8859-6", "ANSI_1256", DRW_CodePageKind::CompatibilityAlias},
        {"ANSI_1257", "ANSI_1257", DRW_CodePageKind::Canonical},
        {"CP1257", "ANSI_1257", DRW_CodePageKind::ExactAlias},
        {"ISO8859-4", "ANSI_1257", DRW_CodePageKind::CompatibilityAlias},
        {"ISO8859-10", "ANSI_1257", DRW_CodePageKind::CompatibilityAlias},
        {"ISO8859-13", "ANSI_1257", DRW_CodePageKind::CompatibilityAlias},
        {"ANSI_1258", "ANSI_1258", DRW_CodePageKind::Canonical},
        {"CP1258", "ANSI_1258", DRW_CodePageKind::ExactAlias},
        {"ANSI_932", "ANSI_932", DRW_CodePageKind::Canonical},
        {"CSWINDOWS31J", "ANSI_932", DRW_CodePageKind::ExactAlias},
        {"X-MS-CP932", "ANSI_932", DRW_CodePageKind::ExactAlias},
        {"SHIFT-JIS", "ANSI_932", DRW_CodePageKind::CompatibilityAlias},
        {"SHIFT_JIS", "ANSI_932", DRW_CodePageKind::CompatibilityAlias},
        {"CSSHIFTJIS", "ANSI_932", DRW_CodePageKind::CompatibilityAlias},
        {"MS_KANJI", "ANSI_932", DRW_CodePageKind::CompatibilityAlias},
        {"X-SJIS", "ANSI_932", DRW_CodePageKind::CompatibilityAlias},
        {"EUCJP", "ANSI_932", DRW_CodePageKind::CompatibilityAlias},
        {"EUC-JP", "ANSI_932", DRW_CodePageKind::CompatibilityAlias},
        {"CSEUCPKDFMTJAPANESE", "ANSI_932", DRW_CodePageKind::CompatibilityAlias},
        {"X-EUC", "ANSI_932", DRW_CodePageKind::CompatibilityAlias},
        {"X-EUC-JP", "ANSI_932", DRW_CodePageKind::CompatibilityAlias},
        {"JIS7", "ANSI_932", DRW_CodePageKind::CompatibilityAlias},
        {"ANSI_936", "ANSI_936", DRW_CodePageKind::Canonical},
        {"CP936", "ANSI_936", DRW_CodePageKind::ExactAlias},
        {"GBK", "ANSI_936", DRW_CodePageKind::CompatibilityAlias},
        {"GB2312", "ANSI_936", DRW_CodePageKind::CompatibilityAlias},
        {"CHINESE", "ANSI_936", DRW_CodePageKind::CompatibilityAlias},
        {"CN-GB", "ANSI_936", DRW_CodePageKind::CompatibilityAlias},
        {"CSGB2312", "ANSI_936", DRW_CodePageKind::CompatibilityAlias},
        {"CSGB231280", "ANSI_936", DRW_CodePageKind::CompatibilityAlias},
        {"CSISO58BG231280", "ANSI_936", DRW_CodePageKind::CompatibilityAlias},
        {"GB_2312-80", "ANSI_936", DRW_CodePageKind::CompatibilityAlias},
        {"GB231280", "ANSI_936", DRW_CodePageKind::CompatibilityAlias},
        {"GB2312-80", "ANSI_936", DRW_CodePageKind::CompatibilityAlias},
        {"ISO-IR-58", "ANSI_936", DRW_CodePageKind::CompatibilityAlias},
        {"GB18030", "ANSI_936", DRW_CodePageKind::CompatibilityAlias},
        {"ANSI_949", "ANSI_949", DRW_CodePageKind::Canonical},
        {"EUCKR", "ANSI_949", DRW_CodePageKind::CompatibilityAlias},
        {"ANSI_950", "ANSI_950", DRW_CodePageKind::Canonical},
        {"BIG5", "ANSI_950", DRW_CodePageKind::CompatibilityAlias},
        {"CN-BIG5", "ANSI_950", DRW_CodePageKind::CompatibilityAlias},
        {"CSBIG5", "ANSI_950", DRW_CodePageKind::CompatibilityAlias},
        {"X-X-BIG5", "ANSI_950", DRW_CodePageKind::CompatibilityAlias},
        {"BIG5-HKSCS", "ANSI_950", DRW_CodePageKind::CompatibilityAlias},
        {"UTF-8", "UTF-8", DRW_CodePageKind::Canonical},
        {"UTF8", "UTF-8", DRW_CodePageKind::ExactAlias},
        {"UTF8-BIT", "UTF-8", DRW_CodePageKind::ExactAlias},
        {"UTF-16", "UTF-16", DRW_CodePageKind::Canonical},
        {"UTF16", "UTF-16", DRW_CodePageKind::ExactAlias},
        {"UTF16-BIT", "UTF-16", DRW_CodePageKind::ExactAlias},
        {"CP437", "ANSI_1252", DRW_CodePageKind::Unavailable},
        {"CP850", "ANSI_1252", DRW_CodePageKind::Unavailable},
        {"CP866", "ANSI_1252", DRW_CodePageKind::Unavailable},
        {"JOHAB", "ANSI_1252", DRW_CodePageKind::Unavailable},
        {"ANSI_1361", "ANSI_1252", DRW_CodePageKind::Unavailable},
        {"ISO-8859-2", "ANSI_1252", DRW_CodePageKind::Unavailable},
        {"ISO-8859-3", "ANSI_1252", DRW_CodePageKind::Unavailable},
        {"ISO-8859-4", "ANSI_1252", DRW_CodePageKind::Unavailable},
        {"ISO-8859-5", "ANSI_1252", DRW_CodePageKind::Unavailable},
        {"ISO-8859-6", "ANSI_1252", DRW_CodePageKind::Unavailable},
        {"ISO-8859-7", "ANSI_1252", DRW_CodePageKind::Unavailable},
        {"ISO-8859-8", "ANSI_1252", DRW_CodePageKind::Unavailable},
        {"ISO-8859-9", "ANSI_1252", DRW_CodePageKind::Unavailable},
        {"ISO-8859-10", "ANSI_1252", DRW_CodePageKind::Unavailable},
        {"ISO-8859-13", "ANSI_1252", DRW_CodePageKind::Unavailable},
        {"ISO-8859-15", "ANSI_1252", DRW_CodePageKind::Unavailable},
        {"ISO8859-14", "ANSI_1252", DRW_CodePageKind::Unavailable},
        {"TSCII", "ANSI_1252", DRW_CodePageKind::Unavailable},
    };
    for (const Entry& entry : entries) {
        if (codePageNameEquals(name, entry.name))
            return {entry.kind, entry.selected};
    }
    // An explicit empty declaration is unknown, not an absent declaration.
    return {DRW_CodePageKind::Unknown, "ANSI_1252"};
}

DRW_CodePageResolution DRW_TextCodec::resolveCodePage(
    std::string_view name, DRW_CodePageMappingPolicy policy) noexcept {
    const auto declaration = classifyCodePage(name);
    const bool implementedName = declaration.kind == DRW_CodePageKind::Canonical
        || declaration.kind == DRW_CodePageKind::ExactAlias;
    const char* requested = implementedName ? declaration.selectedName : nullptr;
    // Family identity is independent of implementation availability below;
    // spelling or historical approximation alone does not supply a decoder.
    struct Family { std::string_view spelling; const char* name; };
    static constexpr Family families[] = {
        {"ISO8859-1", "ISO8859-1"}, {"ISO-8859-1", "ISO8859-1"},
        {"ISO_8859-1", "ISO8859-1"}, {"LATIN1", "ISO8859-1"},
        {"CP819", "ISO8859-1"}, {"IBM819", "ISO8859-1"},
        {"ISO-IR-100", "ISO8859-1"}, {"L1", "ISO8859-1"},
        {"ISO8859-2", "ISO8859-2"}, {"ISO-8859-2", "ISO8859-2"},
        {"ISO8859-3", "ISO8859-3"}, {"ISO-8859-3", "ISO8859-3"},
        {"ISO8859-4", "ISO8859-4"}, {"ISO-8859-4", "ISO8859-4"},
        {"ISO8859-5", "ISO8859-5"}, {"ISO-8859-5", "ISO8859-5"},
        {"ISO8859-6", "ISO8859-6"}, {"ISO-8859-6", "ISO8859-6"},
        {"ISO8859-7", "ISO8859-7"}, {"ISO-8859-7", "ISO8859-7"},
        {"ISO8859-8", "ISO8859-8"}, {"ISO-8859-8", "ISO8859-8"},
        {"ISO8859-9", "ISO8859-9"}, {"ISO-8859-9", "ISO8859-9"},
        {"ISO8859-10", "ISO8859-10"}, {"ISO-8859-10", "ISO8859-10"},
        {"ISO8859-11", "ISO8859-11"},
        {"ISO8859-13", "ISO8859-13"}, {"ISO-8859-13", "ISO8859-13"},
        {"ISO8859-14", "ISO8859-14"},
        {"ISO8859-15", "ISO8859-15"}, {"ISO-8859-15", "ISO8859-15"},
        {"KOI8-R", "KOI8-R"}, {"KOI8-U", "KOI8-U"},
        {"CP437", "IBM437"},
        {"CP850", "IBM850"}, {"IBM 850", "IBM850"},
        {"CP866", "IBM866"}, {"IBM 866", "IBM866"},
        {"APPLE ROMAN", "MACINTOSH"}, {"MACINTOSH", "MACINTOSH"},
        {"TIS-620", "TIS-620"}, {"TSCII", "TSCII"},
        {"EUCJP", "EUC-JP"}, {"EUC-JP", "EUC-JP"},
        {"CSEUCPKDFMTJAPANESE", "EUC-JP"},
        {"SHIFT-JIS", "SHIFT_JIS"}, {"SHIFT_JIS", "SHIFT_JIS"},
        {"CSSHIFTJIS", "SHIFT_JIS"}, {"MS_KANJI", "SHIFT_JIS"},
        {"GBK", "GBK"}, {"GB2312", "GB2312"}, {"CN-GB", "GB2312"},
        {"CSGB2312", "GB2312"}, {"CSGB231280", "GB2312"},
        {"CSISO58BG231280", "GB2312"}, {"GB_2312-80", "GB2312"},
        {"GB231280", "GB2312"}, {"GB2312-80", "GB2312"},
        {"ISO-IR-58", "GB2312"}, {"GB18030", "GB18030"},
        {"EUCKR", "EUC-KR"},
        {"BIG5", "BIG5"}, {"CN-BIG5", "BIG5"}, {"CSBIG5", "BIG5"},
        {"BIG5-HKSCS", "BIG5-HKSCS"},
        {"JOHAB", "JOHAB"}, {"ANSI_1361", "JOHAB"},
    };
    if (!implementedName) {
        for (const auto& family : families) {
            if (codePageNameEquals(name, family.spelling)) {
                requested = family.name;
                break;
            }
        }
    }
    if (policy == DRW_CodePageMappingPolicy::Compatible)
        return {declaration, requested, declaration.selectedName,
                DRW_CodePageMappingStatus::CompatibleSelection};
    if (policy != DRW_CodePageMappingPolicy::ExactNamed)
        return {declaration, requested, nullptr, DRW_CodePageMappingStatus::Unknown};
    if (implementedName)
        return {declaration, requested, declaration.selectedName,
                DRW_CodePageMappingStatus::ExactImplementationAvailable};
    if (requested != nullptr && std::string_view(requested) == "ISO8859-1")
        return {declaration, requested, "ISO8859-1",
                DRW_CodePageMappingStatus::ExactImplementationAvailable};
    if (requested != nullptr && std::string_view(requested) == "ISO8859-2")
        return {declaration, requested, "ISO8859-2",
                DRW_CodePageMappingStatus::ExactImplementationAvailable};
    if (requested != nullptr && std::string_view(requested) == "ISO8859-3")
        return {declaration, requested, "ISO8859-3",
                DRW_CodePageMappingStatus::ExactImplementationAvailable};
    if (requested != nullptr && std::string_view(requested) == "ISO8859-5")
        return {declaration, requested, "ISO8859-5",
                DRW_CodePageMappingStatus::ExactImplementationAvailable};
    if (requested != nullptr && std::string_view(requested) == "ISO8859-6")
        return {declaration, requested, "ISO8859-6",
                DRW_CodePageMappingStatus::ExactImplementationAvailable};
    if (requested != nullptr && std::string_view(requested) == "ISO8859-7")
        return {declaration, requested, "ISO8859-7",
                DRW_CodePageMappingStatus::ExactImplementationAvailable};
    if (requested != nullptr && std::string_view(requested) == "ISO8859-8")
        return {declaration, requested, "ISO8859-8",
                DRW_CodePageMappingStatus::ExactImplementationAvailable};
    if (requested != nullptr && std::string_view(requested) == "ISO8859-9")
        return {declaration, requested, "ISO8859-9",
                DRW_CodePageMappingStatus::ExactImplementationAvailable};
    if (requested != nullptr && std::string_view(requested) == "ISO8859-10")
        return {declaration, requested, "ISO8859-10",
                DRW_CodePageMappingStatus::ExactImplementationAvailable};
    if (requested != nullptr && std::string_view(requested) == "ISO8859-11")
        return {declaration, requested, "ISO8859-11",
                DRW_CodePageMappingStatus::ExactImplementationAvailable};
    if (requested != nullptr && std::string_view(requested) == "ISO8859-13")
        return {declaration, requested, "ISO8859-13",
                DRW_CodePageMappingStatus::ExactImplementationAvailable};
    if (requested != nullptr && std::string_view(requested) == "ISO8859-14")
        return {declaration, requested, "ISO8859-14",
                DRW_CodePageMappingStatus::ExactImplementationAvailable};
    if (requested != nullptr && std::string_view(requested) == "ISO8859-15")
        return {declaration, requested, "ISO8859-15",
                DRW_CodePageMappingStatus::ExactImplementationAvailable};
    if (requested != nullptr && std::string_view(requested) == "ISO8859-4")
        return {declaration, requested, "ISO8859-4",
                DRW_CodePageMappingStatus::ExactImplementationAvailable};
    return {declaration, requested, nullptr,
            declaration.kind == DRW_CodePageKind::Unknown && requested == nullptr
                ? DRW_CodePageMappingStatus::Unknown : DRW_CodePageMappingStatus::Unavailable};
}

std::unique_ptr<DRW_Converter> DRW_TextCodec::createExactCodePageConverter(std::string_view name) {
    const auto resolved = resolveCodePage(name, DRW_CodePageMappingPolicy::ExactNamed);
    if (resolved.status != DRW_CodePageMappingStatus::ExactImplementationAvailable
        || resolved.decoderName == nullptr) return nullptr;
    if (std::string_view(resolved.decoderName) == "ISO8859-1")
        return std::make_unique<DRW_ConvTable>(DRW_TableISO8859_1, CPLENGTHCOMMON);
    if (std::string_view(resolved.decoderName) == "ISO8859-2")
        return std::make_unique<DRW_ConvTable>(DRW_TableISO8859_2, CPLENGTHCOMMON);
    if (std::string_view(resolved.decoderName) == "ISO8859-3")
        return std::make_unique<DRW_ConvTable>(DRW_TableISO8859_3, CPLENGTHCOMMON);
    if (std::string_view(resolved.decoderName) == "ISO8859-5")
        return std::make_unique<DRW_ConvTable>(DRW_TableISO8859_5, CPLENGTHCOMMON);
    if (std::string_view(resolved.decoderName) == "ISO8859-6")
        return std::make_unique<DRW_ConvTable>(DRW_TableISO8859_6, CPLENGTHCOMMON);
    if (std::string_view(resolved.decoderName) == "ISO8859-7")
        return std::make_unique<DRW_ConvTable>(DRW_TableISO8859_7, CPLENGTHCOMMON);
    if (std::string_view(resolved.decoderName) == "ISO8859-8")
        return std::make_unique<DRW_ConvTable>(DRW_TableISO8859_8, CPLENGTHCOMMON);
    if (std::string_view(resolved.decoderName) == "ISO8859-9")
        return std::make_unique<DRW_ConvTable>(DRW_TableISO8859_9, CPLENGTHCOMMON);
    if (std::string_view(resolved.decoderName) == "ISO8859-10")
        return std::make_unique<DRW_ConvTable>(DRW_TableISO8859_10, CPLENGTHCOMMON);
    if (std::string_view(resolved.decoderName) == "ISO8859-11")
        return std::make_unique<DRW_ConvTable>(DRW_TableISO8859_11, CPLENGTHCOMMON);
    if (std::string_view(resolved.decoderName) == "ISO8859-13")
        return std::make_unique<DRW_ConvTable>(DRW_TableISO8859_13, CPLENGTHCOMMON);
    if (std::string_view(resolved.decoderName) == "ISO8859-14")
        return std::make_unique<DRW_ConvTable>(DRW_TableISO8859_14, CPLENGTHCOMMON);
    if (std::string_view(resolved.decoderName) == "ISO8859-15")
        return std::make_unique<DRW_ConvTable>(DRW_TableISO8859_15, CPLENGTHCOMMON);
    if (std::string_view(resolved.decoderName) == "ISO8859-4")
        return std::make_unique<DRW_ConvTable>(DRW_TableISO8859_4, CPLENGTHCOMMON);
    // Reuse the existing canonical converter construction, without changing
    // any caller's codec, DXF metadata, version or future input policy.
    DRW_TextCodec canonical;
    canonical.setCodePage(resolved.decoderName, false);
    return std::move(canonical.conv);
}
