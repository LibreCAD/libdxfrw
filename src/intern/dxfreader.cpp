/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include <cerrno>
#include <array>
#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <limits>
#include <string>
#include <sstream>
#include <locale>
#include "dxfreader.h"
#include "dxfnumber.h"
#include "dxfcode.h"
#include "drw_textcodec.h"
#include "drw_dbg.h"
#include "drw_reserve.h"

namespace {

bool parseDxfHandle(const std::string &text, std::uint32_t &value) {
    if (text.empty())
        return false;
    const char *begin = text.data();
    const char *end = begin + text.size();
    const auto result = std::from_chars(begin, end, value, 16);
    return result.ec == std::errc{} && result.ptr == end;
}

bool parseDxfHandleLexeme(const std::string &text, std::uint64_t &value) {
    // A DWG handle is at most eight bytes. Preserve that width for raw DXF
    // replay, but do not let a value outside the typed 32-bit model leak into
    // ordinary object/entity readers.
    if (text.empty() || text.size() > 16)
        return false;
    const char *begin = text.data();
    const char *end = begin + text.size();
    const auto result = std::from_chars(begin, end, value, 16);
    return result.ec == std::errc{} && result.ptr == end;
}

bool isDxfHexString(const std::string &text) {
    if ((text.size() & 1u) != 0u || text.size() / 2u > 127u)
        return false;
    for (char ch : text) {
        if (!std::isxdigit(static_cast<unsigned char>(ch)))
            return false;
    }
    return true;
}

bool isUnambiguousDxfHandleCode(int code) {
    return code == 105 || code == 1005 ||
           (code >= 320 && code <= 369) ||
           (code >= 390 && code <= 399) ||
           (code >= 480 && code <= 481);
}


}  // namespace

void dxfReader::appendTextReadObservation(DRW_DxfTextReadObservations& target,
    const DRW_TextDecodeObservation& value, int groupCode,
    std::uint64_t record, bool hasRecord,
    std::uint64_t valueEndOffset, bool hasValueEndOffset) noexcept {
    const auto increment = [&target](std::uint64_t& count) {
        if (count == (std::numeric_limits<std::uint64_t>::max)())
            target.counterSaturated = true;
        else ++count;
    };
    increment(target.semanticDecodeCalls);
    if (value.nonAsciiBytes) increment(target.nonAsciiDecodeCalls);
    if (value.legacyBytes) increment(target.legacyByteDecodeCalls);
    if (value.recovery) increment(target.recoveryDecodeCalls);
    if (value.exactNamedMapping) increment(target.exactNamedDecodeCalls);
    if (value.unavailableNamedMapping) increment(target.unavailableNamedDecodeCalls);
    const auto profile = value.sourceProfile < DRW::DxfTextSourceProfile::Count
        ? value.sourceProfile : DRW::DxfTextSourceProfile::NoProfile;
    increment(target.sourceProfileCalls[static_cast<std::size_t>(profile)]);
    target.sourceProfileMask |= DRW::dxfTextSourceProfileMask(profile);
    if (value.issues == 0u) return;
    if (target.issueMask == 0u) {
        target.firstIssueMask = value.issues;
        target.firstGroupCode = groupCode;
        target.firstRecord = hasRecord ? record : 0u;
        target.hasFirstRecord = hasRecord;
        target.firstValueEndOffset = hasValueEndOffset ? valueEndOffset : 0u;
        target.hasFirstValueEndOffset = hasValueEndOffset;
    }
    target.issueMask |= value.issues;
    increment(target.issueDecodeCalls);
    for (std::size_t i = 0; i < target.issueCalls.size(); ++i) {
        if ((value.issues & DRW::dxfTextIssueMask(static_cast<DRW::DxfTextIssue>(i))) != 0u)
            increment(target.issueCalls[i]);
    }
}

std::string dxfReader::decodeSemanticText(std::string_view text, int sourceCode,
                                         bool physicalRecord) {
    // A semantic getter may also be used for a subclass/structural marker.
    // Do not turn raw controls, binary data or handles into text-loss events.
    const bool semantic = sourceCode == 0 ? !physicalRecord
        : dxfValueKindForCode(sourceCode, m_classifierProfile) == DxfValueKind::Str
            && sourceCode != 9 && sourceCode != 100 && sourceCode != 102
            && sourceCode != 999 && sourceCode != 1002 && !isDxfHandleCode(sourceCode);
    // Caret escapes are an ASCII-DXF semantic-string convention. Apply them
    // inside the selected converter so DBCS trail bytes that resemble caret
    // syntax remain part of their original multibyte character.
    auto result = decoder.toUtf8(text, semantic && !isBinary());
    if (semantic) {
        auto observation = decoder.lastDecodeObservation();
        observation.sourceProfile = decoder.dxfSourceProfile(text);
        if (!decoder.hasSourceVersion() && observation.nonAsciiBytes)
            observation.issues |= DRW::dxfTextIssueMask(DRW::DxfTextIssue::UnspecifiedSourceVersion);
        const auto selection = decoder.declaredCodePageSelection();
        // DXF does not use DWG's TU/UTF16 wire profile. Observe interpretation
        // at each semantic decode, not just the last delivered HEADER. Modern
        // DXF ignores this legacy metadata and still decodes UTF8 normally.
        if (!text.empty() && (!decoder.hasSourceVersion() || decoder.getSourceVersion() < DRW::AC1021)
            && selection.selectedName != nullptr
            && (std::string_view(selection.selectedName) == "UTF-16"
                || (decoder.dxfCodePagePolicy() == DRW::DxfCodePagePolicy::ExactNamed
                    && std::string_view(selection.selectedName) == "UTF-8" && observation.nonAsciiBytes)))
            observation.issues |= DRW::dxfTextIssueMask(DRW::DxfTextIssue::UnsupportedEncodingProfile);
        appendTextReadObservation(m_textReadObservations, observation,
                                 sourceCode, m_recordCount, physicalRecord,
                                 m_inputByteCount, physicalRecord);
    }
    return result;
}

bool dxfReader::canConsumeInputBytes(std::size_t count) noexcept {
    if (m_inputByteBudgetExceeded)
        return false;
    if (m_inputByteCount > m_inputByteBudget
        || count > m_inputByteBudget - m_inputByteCount) {
        m_inputByteBudgetExceeded = true;
        return false;
    }
    return true;
}

void dxfReader::accountInputBytes(std::size_t count) noexcept {
    if (count > (std::numeric_limits<std::size_t>::max)() - m_inputByteCount) {
        m_inputByteCount = (std::numeric_limits<std::size_t>::max)();
        m_inputByteBudgetExceeded = true;
        return;
    }
    m_inputByteCount += count;
    if (m_inputByteCount > m_inputByteBudget)
        m_inputByteBudgetExceeded = true;
}

bool dxfReader::readInputBytes(char* destination, std::size_t count) {
    if (filestr == nullptr || (count != 0u && destination == nullptr)
        || count > static_cast<std::size_t>(
               (std::numeric_limits<std::streamsize>::max)())
        || !canConsumeInputBytes(count))
        return false;
    if (count == 0u)
        return true;

    try {
        filestr->read(destination, static_cast<std::streamsize>(count));
    } catch (...) {
        const auto extracted = filestr->gcount();
        if (extracted > 0)
            accountInputBytes(static_cast<std::size_t>(extracted));
        throw;
    }
    const auto extracted = filestr->gcount();
    if (extracted > 0)
        accountInputBytes(static_cast<std::size_t>(extracted));
    return filestr->good();
}

bool dxfReader::readDelimitedToken(std::string& text, char delimiter,
                                    bool allowFinalAsciiValue,
                                    bool retainText) {
    text.clear();
    // Failed tokens are unusable, including when an exception mask rethrows.
    // Deliberately differ from getline's retained partial failed prefix. This
    // rollback allocates nothing and preserves the original throw.
    struct TokenTextScope {
        dxfReader& reader;
        std::string& text;
        bool retainsText {false};
        std::size_t retainedBytes {0};
        bool accepted {false};
        ~TokenTextScope() noexcept {
            if (!accepted) {
                text.clear();
                if (retainsText)
                    reader.releaseCurrentRecordBytes(retainedBytes);
            }
        }
    } token {*this, text, retainText || &text == &strData};
    if (filestr == nullptr || m_tokenByteBudgetExceeded
        || m_inputByteBudgetExceeded)
        return false;
    // Unformatted get performs one sentry per chunk, not per source byte.
    // Its gcount preserves embedded NUL bytes in ASCII tokens. The extra
    // scratch byte is get's terminator, not input beyond the remaining cap.
    std::array<char, 4097> scratch;
    const auto finishAtEof = [&]() {
        if (allowFinalAsciiValue && !text.empty()) {
            token.accepted = true;
            return true;
        }
        if (text.empty())
            filestr->setstate(std::ios::failbit);
        return false;
    };
    while (true) {
        std::char_traits<char>::int_type next;
        try {
            next = filestr->peek();
        } catch (...) {
            // peek may throw on eofbit before the empty-extraction rule runs.
            // Add its failbit without replacing the original mask exception.
            if (text.empty() && filestr->eof()) {
                try { filestr->setstate(std::ios::failbit); } catch (...) {}
            }
            throw;
        }
        if (filestr->bad() || filestr->fail()) {
            // Normative zero-extraction failure: add failbit if peek returned
            // after an I/O failure. A badbit-mask throw has already escaped
            // with its original exception. This matches GNU getline, while
            // intentionally making libc++'s zero-byte failure equally strict.
            if (text.empty())
                filestr->setstate(std::ios::failbit);
            return false;
        }
        if (next == std::char_traits<char>::eof())
            return finishAtEof();
        if (next == std::char_traits<char>::to_int_type(delimiter)) {
            char consumedDelimiter = 0;
            token.accepted = readInputBytes(&consumedDelimiter, 1u);
            return token.accepted;
        }
        // Delimiter and EOF take precedence at the exact cap. An overflow
        // peeks at, but neither consumes nor appends, the first excess byte.
        if (text.size() == m_tokenByteBudget) {
            m_tokenByteBudgetExceeded = true;
            return false;
        }
        const std::size_t remaining = m_tokenByteBudget - text.size();
        const std::size_t chunk = std::min(
            {remaining, inputByteBudgetRemaining(), scratch.size() - 1u});
        if (chunk == 0u) {
            (void)canConsumeInputBytes(1u);
            return false;
        }
        std::streamsize extracted = 0;
        try {
            filestr->get(scratch.data(), static_cast<std::streamsize>(chunk + 1u), delimiter);
            extracted = filestr->gcount();
        } catch (...) {
            extracted = filestr->gcount();
            if (extracted > 0)
                accountInputBytes(static_cast<std::size_t>(extracted));
            throw;
        }
        if (extracted > 0) {
            accountInputBytes(static_cast<std::size_t>(extracted));
            if (token.retainsText) {
                const std::size_t retained = static_cast<std::size_t>(extracted);
                if (!retainCurrentRecordBytes(retained))
                    return false;
                token.retainedBytes += retained;
            }
            try {
                text.append(scratch.data(), static_cast<std::size_t>(extracted));
            } catch (...) {
                // Match getline's acquisition exception policy without
                // swallowing exceptions raised by get/peek's EOF/I/O masks.
                const bool rethrow = (filestr->exceptions() & std::ios::badbit) != 0;
                try { filestr->setstate(std::ios::badbit); } catch (...) {}
                if (rethrow)
                    throw;
                return false;
            }
        }
        if (filestr->bad() || filestr->fail())
            return false;
        // Do not peek again after nonempty EOF: peek's sentry would add
        // failbit and reject a valid final ASCII value without a newline.
        if (filestr->eof())
            return finishAtEof();
    }
}

bool dxfReader::readRec(int *codeData) {
//    std::string text;
    int code;

    if (codeData == nullptr || filestr == nullptr)
        return false;

    auto invalidateRecord = [this]() {
        releaseCurrentRecordRetainedBytes();
        type = INVALID;
        strData.clear();
        rawData.clear();
        doubleData = 0.0;
        intData = 0;
        uint32Data = 0u;
        int64 = 0;
        m_currentCode = 0;
        m_currentSelfHandle = 0;
        m_currentSelfHandleRegistered = false;
    };
    m_lastInvalidHandleCode = 0;
    m_previousCode = m_currentCode;
    invalidateRecord();
    if (m_tokenByteBudgetExceeded || m_inputByteBudgetExceeded)
        return false;

    // Comments are ignored only after the first SECTION marker. Skip a run
    // iteratively so hostile input cannot grow the call stack. Count each
    // physical record before decoding its value so ignored comments cannot
    // evade the aggregate work ceiling.
    const auto consumeRecordBudget = [this]() {
        if (m_recordCount >= m_recordBudget) {
            m_recordBudgetExceeded = true;
            return false;
        }
        ++m_recordCount;
        return true;
    };
    do {
        if (!consumeRecordBudget())
            return false;
        if (!readCode(&code))
            return false;
        if (code != 999 || !m_bIgnoreComments)
            break;
        if (!readString()) {
            invalidateRecord();
            return false;
        }
    } while (true);

    *codeData = code;
    m_currentCode = code;

    bool valueOk = true;
    switch (dxfValueKindForCode(code, m_classifierProfile)) {
    case DxfValueKind::Str:    valueOk = readString(); break;
    case DxfValueKind::Dbl:    valueOk = readDouble(); break;
    case DxfValueKind::I16:    valueOk = readInt16();  break;
    case DxfValueKind::I32:    valueOk = readInt32();  break;
    case DxfValueKind::I64:    valueOk = readInt64();  break;
    case DxfValueKind::Bln:    valueOk = readBool();   break;
    case DxfValueKind::Bin:    valueOk = readBinary(); break;
    case DxfValueKind::Unknown:
        if (skip) {
            //skip safely this dxf entry ( ok for ascii dxf)
            valueOk = readString();
            break;
        }
        //break in binary files because the conduct is unpredictable
        invalidateRecord();
        return false;
    }

    // Use !fail() not good(): a final ASCII record WITHOUT a trailing newline
    // sets eofbit (good()==false) on an otherwise SUCCESSFUL extraction
    // (fail()==false). good() would wrongly reject that last record;
    // a genuine failed read sets failbit, which !fail() still catches. (Binary
    // readers gate on their own good() check, so this is a no-op for them.)
    if (!valueOk || filestr->fail()) {
        invalidateRecord();
        return false;
    }
    // Code 5 is intentionally excluded: DIMSTYLE uses it for a block-name
    // string in valid files. All other pointer/handle ranges are unambiguous.
    const bool validHandle = m_allowWideHandleLexemes
        ? isValidHandleLexeme()
        : isValidHandleString();
    const bool dimstyleName = m_allowDimstyleNames
        && (code == 340 || (code > 340 && code <= 344 && strData.empty()));
    if (isUnambiguousDxfHandleCode(code) && !validHandle && !dimstyleName) {
        m_lastInvalidHandleCode = code;
        invalidateRecord();
        return false;
    }
    if (!reserveCurrentRecordTextOutput()) {
        invalidateRecord();
        return false;
    }
    return true;
}
std::uint32_t dxfReader::getHandleString(){
    std::uint32_t value = 0;
    if (!parseDxfHandle(strData, value))
        return 0;
    return value;
}

bool dxfReader::isValidHandleString() const {
    std::uint32_t value = 0;
    return parseDxfHandle(strData, value);
}

bool dxfReader::isValidHandleLexeme() const {
    std::uint64_t value = 0;
    return parseDxfHandleLexeme(strData, value);
}

bool dxfReader::registerSelfHandle() {
    std::uint64_t value = 0;
    if (!parseDxfHandleLexeme(strData, value))
        return false;
    // Zero is the null handle, not a drawing object identity.
    if (value == 0)
        return true;
    // Raw proxy entities admit code 5 once through their proxy host and once
    // through the lossless carrier. Treat those two admissions of the same
    // record as one registration, while still rejecting a later record.
    if (m_currentSelfHandleRegistered && m_currentSelfHandle == value)
        return true;
    if (!m_selfHandles.insert(value).second)
        return false;
    m_currentSelfHandle = value;
    m_currentSelfHandleRegistered = true;
    return true;
}

bool dxfReaderBinary::readCode(int *code) {
    unsigned char buffer[2] = {};
    if (!readInputBytes(reinterpret_cast<char*>(buffer), sizeof(buffer)))
        return false;
    *code = static_cast<int>(buffer[0])
        | (static_cast<int>(buffer[1]) << 8);
    DRW_DBG(*code); DRW_DBG("\n");

    return true;
}

bool dxfReaderBinaryR12::readCode(int *code) {
    unsigned char b = 0;
    if (!readInputBytes(reinterpret_cast<char*>(&b), 1u))
        return false;
    if (b == 255) {
        // 0xFF is the extended-data escape: the real 16-bit LE group code
        // follows in the next two bytes (R12 carries xdata codes >= 255 this
        // way). Matches ezdxf binary_tags_loader.
        unsigned char buffer[2] = {};
        if (!readInputBytes(reinterpret_cast<char*>(buffer), sizeof(buffer)))
            return false;
        *code = static_cast<int>(buffer[0])
            | (static_cast<int>(buffer[1]) << 8);
    } else {
        *code = static_cast<int>(b);
    }
    DRW_DBG(*code); DRW_DBG("\n");

    return true;
}

bool dxfReaderBinary::readString() {
    type = STRING;
    if (!readDelimitedToken(strData, '\0', false))
        return false;
    if (!copyRawValue(strData))
        return false;
    DRW_DBG(strData); DRW_DBG("\n");
    return (filestr->good());
}

bool dxfReaderBinary::readString(std::string *text) {
    type = STRING;
    if (text == nullptr || !readDelimitedToken(*text, '\0', false))
        return false;
    if (!copyRawValue(*text))
        return false;
    DRW_DBG(*text); DRW_DBG("\n");
    return (filestr->good());
}

bool dxfReaderBinary::readBinary() {
    type = BINARY;
    unsigned char chunklen {0};

    if (!readInputBytes(reinterpret_cast<char*>(&chunklen), 1u))
        return false;
    // Binary DXF stores binary chunks as a one-byte length, but the DXF
    // format limits each 310-319/1004 chunk to 127 bytes.
    if (chunklen > 127)
        return false;
    // Capture the chunk bytes as an upper-hex string — the canonical ASCII form
    // of binary codes (310-319/1004) — so getString() returns the real data.
    // Previously this seeked past the chunk and never wrote strData, so any
    // binary group on a binary read (typed entity OR raw-net object) re-emitted
    // a STALE strData (the previous record's value). Same net stream advance.
    if (!canConsumeInputBytes(chunklen))
        return false;
    strData.clear();
    const std::size_t hexSize = static_cast<std::size_t>(chunklen) * 2u;
    if (!retainCurrentRecordBytes(hexSize))
        return false;
    if (!DRW::reserve(strData, static_cast<int>(chunklen) * 2))
        return false;
    std::array<unsigned char, 127> chunk{};
    if (!readInputBytes(reinterpret_cast<char*>(chunk.data()), chunklen))
        return false;
    static const char hex[] = "0123456789ABCDEF";
    for (unsigned i = 0; i < chunklen; ++i) {
        const unsigned char u = chunk[i];
        strData.push_back(hex[(u >> 4) & 0xF]);
        strData.push_back(hex[u & 0xF]);
    }
    if (!copyRawValue(strData))
        return false;
    DRW_DBG( chunklen); DRW_DBG( " byte(s) binary data read\n");

    return (filestr->good());
}

bool dxfReaderBinary::readInt16() {
    type = INT32;
    unsigned char buffer[2] = {};
    if (!readInputBytes(reinterpret_cast<char*>(buffer), sizeof(buffer)))
        return false;
    intData = static_cast<std::int16_t>((static_cast<unsigned char>(buffer[1]) << 8)
                                  | static_cast<unsigned char>(buffer[0]));
    DRW_DBG(intData); DRW_DBG("\n");
    return true;
}

bool dxfReaderBinary::readInt32() {
    type = INT32;
    unsigned char buffer[4] = {};
    if (!readInputBytes(reinterpret_cast<char*>(buffer), sizeof(buffer)))
        return false;
    std::uint32_t value = static_cast<std::uint32_t>(buffer[0])
        | (static_cast<std::uint32_t>(buffer[1]) << 8)
        | (static_cast<std::uint32_t>(buffer[2]) << 16)
        | (static_cast<std::uint32_t>(buffer[3]) << 24);
    uint32Data = value;
    intData = static_cast<std::int32_t>(value);
    DRW_DBG(intData); DRW_DBG("\n");
    return true;
}

bool dxfReaderBinary::readInt64() {
    type = INT64;
    unsigned char buffer[8] = {};
    if (!readInputBytes(reinterpret_cast<char*>(buffer), sizeof(buffer)))
        return false;
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i)
        value |= static_cast<std::uint64_t>(buffer[i]) << (8 * i);
    std::memcpy(&int64, &value, sizeof(int64));
    DRW_DBG(int64); DRW_DBG(" int64\n");
    return true;
}

bool dxfReaderBinary::readDouble() {
    type = DOUBLE;
    unsigned char buffer[8] = {};
    if (!readInputBytes(reinterpret_cast<char*>(buffer), sizeof(buffer)))
        return false;
    std::uint64_t value = 0;
    for (int i = 0; i < 8; ++i)
        value |= static_cast<std::uint64_t>(buffer[i]) << (8 * i);
    std::memcpy(&doubleData, &value, sizeof(doubleData));
    DRW_DBG(doubleData); DRW_DBG("\n");
    return std::isfinite(doubleData);
}

//saved as int or add a bool member??
bool dxfReaderBinary::readBool() {
    type = BOOL;
    unsigned char buffer[1] = {};
    if (!readInputBytes(reinterpret_cast<char*>(buffer), sizeof(buffer)))
        return false;
    intData = static_cast<int>(buffer[0]);
    DRW_DBG(intData); DRW_DBG("\n");
    return true;
}

bool dxfReaderAscii::readCode(int *code) {
    std::string text;
    if (!readDelimitedToken(text, '\n', true, false))
        return false;
    if (!text.empty() && text.at(text.size()-1) == '\r')
        text.erase(text.size()-1);
    char *end = nullptr;
    errno = 0;
    long parsed = DxfNumeric::strtolC(text.c_str(), &end);
    while (end != nullptr && *end != '\0'
           && DxfNumeric::isSpace(*end)) {
        ++end;
    }
    if (end == text.c_str() || end == nullptr || end != text.c_str() + text.size()
        || errno == ERANGE || parsed < 0 || parsed > 1071) {
        return false;
    }
    *code = static_cast<int>(parsed);
    DRW_DBG(*code); DRW_DBG("\n");
    return true;
}
bool dxfReaderAscii::readString(std::string *text) {
    type = STRING;
    if (text == nullptr || !readDelimitedToken(*text, '\n', true))
        return false;
    if (!text->empty() && text->at(text->size()-1) == '\r')
        text->erase(text->size()-1);
    if (!copyRawValue(*text))
        return false;
    return (!filestr->fail());
}

bool dxfReaderAscii::readString() {
    type = STRING;
    if (!readDelimitedToken(strData, '\n', true))
        return false;
    if (!strData.empty() && strData.at(strData.size()-1) == '\r') {
        strData.erase(strData.size()-1);
        releaseCurrentRecordBytes(1u);
    }
    if (!copyRawValue(strData))
        return false;
    DRW_DBG(strData); DRW_DBG("\n");
    return (!filestr->fail());
}

bool dxfReaderAscii::readBinary() {
    if (!readString())
        return false;
    // ASCII DXF binary groups are complete hexadecimal byte pairs. Rejecting
    // malformed data here keeps typed and raw record paths transactional.
    if (!isDxfHexString(strData))
        return false;
    type = BINARY;
    return true;
}

bool dxfReaderAscii::readInt16() {
    std::string text;
    if (readString(&text)){
        char *end = nullptr;
        errno = 0;
        const long long parsed = DxfNumeric::strtollC(text.c_str(), &end);
        while (end != nullptr && *end != '\0'
               && DxfNumeric::isSpace(*end)) {
            ++end;
        }
        if (end == text.c_str() || end == nullptr || end != text.c_str() + text.size()
            || errno == ERANGE
            // DXF 16-bit flag fields may be written as an unsigned bit
            // pattern (for example 0x8001). Keep both signed values and all
            // values representable by the two-byte field.
            || parsed < std::numeric_limits<std::int16_t>::min()
            || parsed > std::numeric_limits<std::uint16_t>::max()) {
            return false;
        }
        type = INT32;
        intData = static_cast<int>(parsed);
        DRW_DBG(intData); DRW_DBG("\n");
        return true;
    }
    return false;
}

bool dxfReaderAscii::readInt32() {
    std::string text;
    if (readString(&text)){
        char *end = nullptr;
        errno = 0;
        const long long parsed = DxfNumeric::strtollC(text.c_str(), &end);
        while (end != nullptr && *end != '\0'
               && DxfNumeric::isSpace(*end)) {
            ++end;
        }
        const bool materialColorValue =
            materialUnsignedColorValuesEnabled()
            && (currentCode() == 90 || currentCode() == 91
                || currentCode() == 92);
        if (end == text.c_str() || end == nullptr || end != text.c_str() + text.size()
            || errno == ERANGE
            || (materialColorValue
                ? (parsed < std::numeric_limits<std::int32_t>::min()
                   || parsed > static_cast<long long>(
                       std::numeric_limits<std::uint32_t>::max()))
                : (parsed < std::numeric_limits<std::int32_t>::min()
                   || parsed > std::numeric_limits<std::int32_t>::max()))) {
            return false;
        }
        type = INT32;
        if (materialColorValue) {
            // MATERIAL colors are uint32 bit patterns, but real producers
            // also spell their high-bit-set values as signed decimal int32.
            // The conversion to uint32 is modulo 2^32; keep ordinary groups
            // signed-only and avoid implementation-defined uint32->int casts.
            uint32Data = static_cast<std::uint32_t>(parsed);
            intData = parsed <= std::numeric_limits<std::int32_t>::max()
                ? static_cast<int>(parsed)
                : -1 - static_cast<int>(
                    std::numeric_limits<std::uint32_t>::max() - uint32Data);
        } else {
            intData = static_cast<int>(parsed);
            uint32Data = static_cast<std::uint32_t>(intData);
        }
        DRW_DBG(intData); DRW_DBG("\n");
        return true;
    }
    return false;
}

bool dxfReaderAscii::readInt64() {
    std::string text;
    if (readString(&text)){
        char *end = nullptr;
        errno = 0;
        const long long parsed = DxfNumeric::strtollC(text.c_str(), &end);
        while (end != nullptr && *end != '\0'
               && DxfNumeric::isSpace(*end)) {
            ++end;
        }
        if (end == text.c_str() || end == nullptr || end != text.c_str() + text.size()
            || errno == ERANGE
            || parsed < std::numeric_limits<std::int64_t>::min()
            || parsed > std::numeric_limits<std::int64_t>::max()) {
            return false;
        }
        type = INT64;
        int64 = static_cast<std::int64_t>(parsed);
        DRW_DBG(int64); DRW_DBG(" int64\n");
        return true;
    }
    return false;
}

bool dxfReaderAscii::readDouble() {
    std::string text;
    if (readString(&text)){
        char *end = nullptr;
        errno = 0;
        double parsed = DxfNumeric::strtodC(text.c_str(), &end);
        while (end != nullptr && *end != '\0'
               && DxfNumeric::isSpace(*end)) {
            ++end;
        }
        // Underflow sets ERANGE but produces a valid tiny value; overflow is
        // rejected by the finite check.
        if (end == text.c_str() || end == nullptr || end != text.c_str() + text.size()
            || !std::isfinite(parsed)) {
            DRW_DBG("dxfReaderAscii::readDouble(): reading double error: ");
            DRW_DBG(text);
            DRW_DBG('\n');
            return false;
        }
        type = DOUBLE;
        doubleData = parsed;
        DRW_DBG(doubleData); DRW_DBG('\n');
        return true;
    }
    return false;
}

//saved as int or add a bool member??
bool dxfReaderAscii::readBool() {
    std::string text;
    if (readString(&text)){
        char *end = nullptr;
        errno = 0;
        long parsed = DxfNumeric::strtolC(text.c_str(), &end);
        while (end != nullptr && *end != '\0'
               && DxfNumeric::isSpace(*end)) {
            ++end;
        }
        if (end == text.c_str() || end == nullptr || end != text.c_str() + text.size()
            || errno == ERANGE
            || parsed < std::numeric_limits<std::int32_t>::min()
            || parsed > std::numeric_limits<std::int32_t>::max()) {
            return false;
        }
        type = BOOL;
        intData = static_cast<int>(parsed);
        DRW_DBG(intData); DRW_DBG("\n");
        return true;
    }
    return false;
}
