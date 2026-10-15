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
    swap(m_legacyDxfInputEligible, other.m_legacyDxfInputEligible);
    swap(m_legacyDxfCodePageAllowed, other.m_legacyDxfCodePageAllowed);
    swap(m_declaredCodePageSelection, other.m_declaredCodePageSelection);
    swap(m_byteCodePageSelection, other.m_byteCodePageSelection);
    swap(cp, other.cp);
    swap(conv, other.conv);
    swap(cp8Conv, other.cp8Conv);
}

void DRW_TextCodec::setVersion(DRW::Version v, bool dxfFormat){
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
    const DRW_CodePageSelection selection = classifyCodePage(c);
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
    if (explicitDeclaration)
        m_declaredCodePageSelection = selection;
}

std::string DRW_TextCodec::toUtf8(std::string_view s) {
    if (m_dxfTextEncoding == DRW::DxfTextEncoding::LegacyLibreCadUtf8
        && m_legacyDxfInputEligible) {
        // Preserve UTF-8 input bytes while retaining Unicode/MIF escapes.
        // Do not replace conv: fromUtf8 still uses the declared output codec.
        DRW_Converter utf8(nullptr, 0);
        return utf8.toUtf8(s);
    }
    return conv->toUtf8(s);
}

std::string DRW_TextCodec::fromUtf8(std::string_view s) {
    return conv->fromUtf8(s);
}

std::string DRW_TextCodec::fromUtf8CP8(std::string_view s) {
    return cp8Conv ? cp8Conv->fromUtf8(s) : std::string{s};
}

std::string DRW_TextCodec::toUtf8CP8(std::string_view s) {
    return cp8Conv ? cp8Conv->toUtf8(s) : std::string{s};
}

void DRW_TextCodec::setByteCodePage(const std::string &c) {
    DRW_TextCodec byteCodec;
    byteCodec.setCodePage(c, false);
    cp8Conv = std::move(byteCodec.conv);
    m_byteCodePageSelection = byteCodec.m_declaredCodePageSelection;
}

std::string DRW_Converter::toUtf8(std::string_view s) {
    std::string result;
    int j = 0;
    unsigned int i= 0;
    for (i=0; i < s.length(); i++) {
        unsigned char c = s.at(i);
        if (c < 0x80) { //ascii check for \U+???? or \M+cXXXX
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
    std::string res;
    for ( auto it=s.begin() ; it < s.end(); ++it ) {
        unsigned char c = *it;
        // \U+ and \M+ escapes are 7-bit ASCII anchors; valid even when an
        // adjacent table byte is high. Test the backslash branch regardless
        // of `c < 0x80` so an escape immediately following a table-mapped
        // byte is still detected.
        if (c == '\\') {
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
            res += c; // ascii char write
        } else {
            res += encodeNum(table[c-0x80]); // translate from table
        }
    } //end for

    return res;
}

std::string DRW_Converter::encodeText(const std::string &stmp){
    int code;
#if defined(__APPLE__)
    // %x writes an unsigned int; scanning straight into `code` is undefined and
    // GCC 16 rejects it under -Wformat.
    unsigned int scanned = 0;
    int Succeeded = sscanf (&( stmp.substr(3,4)[0]), "%x", &scanned );
    code = static_cast<int>(scanned);
    if ( !Succeeded || Succeeded == EOF )
        code = 0;
#else
    std::istringstream sd(stmp.substr(3,4));
    sd >> std::hex >> code;
#endif
    return encodeNum(code);
}

std::string DRW_Converter::encodeMifText(const std::string &tok){
    // tok layout: "\M+cXXXX" — selector at index 3, 4 hex digits at [4..8).
    if (tok.size() != 8 || tok[0] != '\\' || tok[1] != 'M' || tok[2] != '+')
        return std::string{};
    // Numeric stream/scanf conversion accepts signs, whitespace and partial
    // hex. Validate the complete ASCII payload first on every platform, so a
    // malformed escape stays literal in the caller instead of creating a
    // partially decoded character (including TAB/CR).
    for (std::size_t i = 4; i < 8; ++i) {
        const unsigned char ch = static_cast<unsigned char>(tok[i]);
        if (!((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F')
              || (ch >= 'a' && ch <= 'f')))
            return std::string{};
    }
    const char* cpName = nullptr;
    switch (tok[3]) {
        case '1': cpName = "ANSI_932"; break; // Shift-JIS
        case '2': cpName = "ANSI_950"; break; // Big5
        case '3': cpName = "ANSI_949"; break; // EUC-KR
        // Selector4 names Johab1361, not CP1252. There is no Johab decoder
        // here; let the existing callers preserve the original MIF spelling.
        case '4': return std::string{};
        case '5': cpName = "ANSI_936"; break; // GB18030/GBK
        default:  return std::string{};
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
    if (code < 0x100) return std::string{};
    DRW_TextCodec codec;
    codec.setVersion(DRW::AC1015, /*dxfFormat=*/false);
    codec.setCodePage(cpName, /*dxfFormat=*/false);
    std::string raw;
    raw.push_back(static_cast<char>((code >> 8) & 0xFF));
    raw.push_back(static_cast<char>(code & 0xFF));
    return codec.toUtf8(raw);
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
    if (b == nullptr)
        return -1;
    *b = 1;
    if (s.empty())
        return -1;

    const auto continuation = [](unsigned char value) {
        return (value & 0xC0U) == 0x80U;
    };
    const unsigned char c = static_cast<unsigned char>(s[0]);
    std::size_t length = 0;
    std::uint32_t code = 0;
    std::uint32_t minimum = 0;

    if (c >= 0xC2U && c <= 0xDFU) {
        length = 2;
        code = c & 0x1FU;
        minimum = 0x80U;
    } else if (c >= 0xE0U && c <= 0xEFU) {
        length = 3;
        code = c & 0x0FU;
        minimum = 0x800U;
    } else if (c >= 0xF0U && c <= 0xF4U) {
        length = 4;
        code = c & 0x07U;
        minimum = 0x10000U;
    } else {
        return -1;
    }

    if (s.size() < length)
        return -1;
    for (std::size_t index = 1; index < length; ++index) {
        const unsigned char value = static_cast<unsigned char>(s[index]);
        if (!continuation(value))
            return -1;
        code = (code << 6) | (value & 0x3FU);
    }
    if (code < minimum || code > 0x10FFFFU
        || (code >= 0xD800U && code <= 0xDFFFU))
        return -1;

    *b = static_cast<int>(length);
    return static_cast<int>(code);
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
    std::string res;
    for (auto it=s.begin() ; it < s.end(); ++it ) {
        bool notFound = true;
        unsigned char c = *it;
        if (c < 0x80) {
            notFound = false;
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
        if (notFound) res += encodeNum(NOTFOUND936);
    } //end for

    return res;
}

DRW_Conv932Table::DRW_Conv932Table()
    :DRW_Converter(DRW_Table932, CPLENGTH932) {
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
    std::string res;
    for (auto it=s.begin() ; it < s.end(); ++it ) {
        bool notFound = true;
        unsigned char c = *it;
        if (c < 0x80) {
            notFound = false;
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
        if (notFound) res += encodeNum(NOTFOUND932);
    } //end for

    return res;
}

std::string DRW_ConvUTF16::fromUtf8(std::string_view s){
    DRW_UNUSED(s);
    //RLZ: to be written (only needed for write dwg 2007+)
    return std::string();
}

std::string DRW_ConvUTF16::toUtf8(std::string_view s){
    // Decode UTF-16LE bytes to UTF-8. Combines surrogate pairs
    // 0xD800..0xDBFF + 0xDC00..0xDFFF into astral codepoints; an unpaired
    // half (high without matching low, or stray low) emits U+FFFD.
    std::string res;
    const size_t n = s.size() & ~static_cast<size_t>(1);
    for (size_t i = 0; i < n; i += 2) {
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
                }
            } else {
                ch = 0xFFFD; // truncated high at end
            }
        } else if (ch >= 0xDC00 && ch <= 0xDFFF) { // stray low surrogate
            ch = 0xFFFD;
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
    // This preserves the old selection for every accepted spelling. The kind
    // describes naming policy, not exhaustive byte-mapping qualification.
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
        {"APPLE ROMAN", "ANSI_1252", DRW_CodePageKind::CompatibilityAlias},
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
        {"MACINTOSH", "ANSI_1252", DRW_CodePageKind::Unavailable},
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
    const auto equalsAscii = [](std::string_view source, std::string_view key) {
        if (source.size() != key.size())
            return false;
        for (std::size_t i = 0; i < source.size(); ++i) {
            const unsigned char byte = static_cast<unsigned char>(source[i]);
            const unsigned char folded = byte >= 'a' && byte <= 'z'
                ? static_cast<unsigned char>(byte - 'a' + 'A') : byte;
            if (folded != static_cast<unsigned char>(key[i]))
                return false;
        }
        return true;
    };
    for (const Entry& entry : entries) {
        if (equalsAscii(name, entry.name))
            return {entry.kind, entry.selected};
    }
    // An explicit empty declaration is unknown, not an absent declaration.
    return {DRW_CodePageKind::Unknown, "ANSI_1252"};
}
