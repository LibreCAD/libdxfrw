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

#include <array>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <algorithm>
#include <charconv>
#include <utility>
#include "dxfwriter.h"
#include "dxfnumber.h"
#include "drw_dbg.h"

namespace {

// Uppercase ASCII only, before text encoding. Bytes outside the ASCII lower-
// case range belong to the source spelling, not the process CTYPE locale.
constexpr unsigned char dxfAsciiUpper(unsigned char value) {
    return value >= 'a' && value <= 'z'
        ? static_cast<unsigned char>(value - 'a' + 'A') : value;
}

bool isAsciiDxfCode(int code) {
    return code >= 0 && code <= 1071;
}

bool isBinaryDxfCode(int code) {
    return isAsciiDxfCode(code) && code != 999;
}

bool isSafeAsciiDxfString(std::string_view value) {
    return value.find('\r') == std::string_view::npos
        && value.find('\n') == std::string_view::npos
        && value.find('\0') == std::string_view::npos;
}

bool isSafeBinaryDxfString(std::string_view value) {
    return value.find('\0') == std::string_view::npos;
}

std::size_t encodeDxfAsciiControl(unsigned char value, char* encoded) {
    if (value == '^') {
        encoded[0] = '^';
        encoded[1] = ' ';
        return 2u;
    }
    // NUL is still refused by the existing text admission boundary, and CR/LF
    // retain their field-specific refusal/MTEXT normalization behavior.
    if ((value >= 1u && value <= 31u && value != '\r' && value != '\n')
        || value == 127u) {
        encoded[0] = '^';
        encoded[1] = value == 127u ? '?' : static_cast<char>(value | 0x40u);
        return 2u;
    }
    return 0u;
}

bool hasDxfAsciiControlToEscape(std::string_view text) {
    for (const unsigned char value : text) {
        char encoded[2] = {};
        if (encodeDxfAsciiControl(value, encoded) != 0u)
            return true;
    }
    return false;
}

bool dxfAsciiEqualIgnoreCase(std::string_view left,
                             const char* right) {
    std::size_t index = 0;
    for (; index < left.size() && right[index] != '\0'; ++index) {
        if (dxfAsciiUpper(static_cast<unsigned char>(left[index]))
            != dxfAsciiUpper(static_cast<unsigned char>(right[index])))
            return false;
    }
    return index == left.size() && right[index] == '\0';
}

std::string normalizeDxfRecordName(std::string_view text) {
    std::string normalized;
    normalized.reserve(text.size());
    for (unsigned char character : text)
        normalized.push_back(static_cast<char>(dxfAsciiUpper(character)));
    return normalized;
}

bool parseDxfRecordHandle(std::string_view text, std::uint32_t& handle) {
    if (text.empty())
        return false;
    handle = 0;
    for (unsigned char character : text) {
        const unsigned char upper = dxfAsciiUpper(character);
        unsigned int digit = 0;
        if (upper >= '0' && upper <= '9')
            digit = upper - '0';
        else if (upper >= 'A' && upper <= 'F')
            digit = upper - 'A' + 10u;
        else
            return false;
        if (handle > (std::numeric_limits<std::uint32_t>::max() - digit) / 16u)
            return false;
        handle = handle * 16u + digit;
    }
    return true;
}

bool isDxfInt16Value(int value) {
    return value >= static_cast<int>(std::numeric_limits<std::int16_t>::min())
        && value <= static_cast<int>(std::numeric_limits<std::uint16_t>::max());
}

bool writeAsciiDxfIntegerValue(std::ostream* stream, std::int64_t value,
                               std::size_t width) {
    char buffer[32];
    const auto result = std::to_chars(buffer, buffer + sizeof buffer, value);
    if (result.ec != std::errc{})
        return false;
    const auto length = static_cast<std::size_t>(result.ptr - buffer);
    if (length < width)
        stream->write("     ", static_cast<std::streamsize>(width - length));
    stream->write(buffer, static_cast<std::streamsize>(length));
    stream->put('\n');
    return stream->good();
}

bool writeAsciiDxfInteger(std::ostream *stream, int code, std::int64_t value) {
    if (stream == nullptr || !isAsciiDxfCode(code))
        return false;
    return writeAsciiDxfIntegerValue(stream, code, 3)
        && writeAsciiDxfIntegerValue(stream, value, 5);
}

bool writeBinaryDxfCode(std::ostream *stream, int code) {
    if (stream == nullptr || !isBinaryDxfCode(code))
        return false;
    const std::uint16_t value = static_cast<std::uint16_t>(code);
    const char bytes[] = {
        static_cast<char>(value & 0xFFU),
        static_cast<char>((value >> 8U) & 0xFFU)
    };
    stream->write(bytes, sizeof(bytes));
    return stream->good();
}

bool writeBinaryDxfR12Code(std::ostream *stream, int code) {
    if (stream == nullptr || code < 0 || code > 1071)
        return false;
    if (code <= 254) {
        const char byte = static_cast<char>(code);
        stream->write(&byte, 1);
        return stream->good();
    }
    if (code < 1000)
        return false;
    const char escape = static_cast<char>(0xFF);
    stream->write(&escape, 1);
    return writeBinaryDxfCode(stream, code);
}

bool writeLittleEndian(std::ostream *stream, std::uint64_t value,
                       std::size_t byteCount) {
    if (stream == nullptr || byteCount > sizeof(value))
        return false;
    char bytes[sizeof(value)] = {};
    for (std::size_t i = 0; i < byteCount; ++i) {
        bytes[i] = static_cast<char>((value >> (i * 8U)) & 0xFFU);
    }
    stream->write(bytes, static_cast<std::streamsize>(byteCount));
    return stream->good();
}

} // namespace

bool dxfWriter::observeRecordString(int code, std::string_view text) {
    if (!m_recordInventoryEnabled)
        return true;
    if (code == 0) {
        m_sectionNamePending = false;
        m_recordOwnerPending = false;
        m_recordApplicationDepth = 0;
        if (dxfAsciiEqualIgnoreCase(text, "SECTION")) {
            m_recordHandlePending = false;
            m_currentRecordName = 0;
            m_sectionNamePending = true;
        } else if (dxfAsciiEqualIgnoreCase(text, "ENDSEC")) {
            m_recordHandlePending = false;
            m_currentRecordName = 0;
            m_currentSectionName = 0;
        } else if (dxfAsciiEqualIgnoreCase(text, "EOF")
                   || dxfAsciiEqualIgnoreCase(text, "ENDTAB")) {
            m_recordHandlePending = false;
            m_currentRecordName = 0;
        } else {
            if (!internRecordName(text, m_currentRecordName)) {
                m_recordInventoryFailure = true;
                return false;
            }
            m_recordHandlePending = true;
        }
        return true;
    }
    if (m_sectionNamePending && code == 2) {
        if (!internRecordName(text, m_currentSectionName)) {
            m_recordInventoryFailure = true;
            return false;
        }
        m_sectionNamePending = false;
        return true;
    }
    if (m_recordHandlePending && (code == 5 || code == 105)) {
        std::uint32_t handle = 0;
        if (!parseDxfRecordHandle(text, handle)
            || m_emittedRecords.size() >= m_recordInventoryLimit) {
            m_recordInventoryFailure = true;
            return false;
        }
        m_emittedRecords.push_back(
            {handle, m_currentRecordName, m_currentSectionName});
        m_currentRecordIndex = m_emittedRecords.size() - 1u;
        m_recordInventoryFinalized = false;
        m_recordHandlePending = false;
        m_recordOwnerPending = true;
        return true;
    }
    if (m_recordOwnerPending && code == 102) {
        if (!text.empty() && text.front() == '{') {
            if (m_recordApplicationDepth == (std::numeric_limits<std::uint32_t>::max)()) {
                m_recordInventoryFailure = true;
                return false;
            }
            ++m_recordApplicationDepth;
        } else if (text == "}" && m_recordApplicationDepth != 0) {
            --m_recordApplicationDepth;
        }
        return true;
    }
    // Reactor/application handles are not the record's owner. The first
    // subclass marker closes the common header; e.g. an XRECORD body 330
    // value must never become owner evidence for an ownerless record.
    if (m_recordOwnerPending && m_recordApplicationDepth == 0 && code == 100) {
        m_recordOwnerPending = false;
        return true;
    }
    if (m_recordOwnerPending && m_recordApplicationDepth == 0 && code == 330) {
        std::uint32_t ownerHandle = 0;
        if (!parseDxfRecordHandle(text, ownerHandle)
            || m_currentRecordIndex >= m_emittedRecords.size()) {
            m_recordInventoryFailure = true;
            return false;
        }
        EmittedRecordIdentity& record = m_emittedRecords[m_currentRecordIndex];
        record.ownerHandle = ownerHandle;
        record.hasOwnerHandle = true;
        m_recordOwnerPending = false;
    }
    return true;
}

bool dxfWriter::internRecordName(std::string_view text,
                                std::uint32_t& id) {
    constexpr std::size_t kMaxRecordNameBytes = 255u;
    constexpr std::size_t kMaxUniqueRecordNames = 65536u;
    if (text.empty() || text.size() > kMaxRecordNameBytes)
        return false;
    std::string normalized = normalizeDxfRecordName(text);
    const auto existing = m_recordNameIds.find(normalized);
    if (existing != m_recordNameIds.end()) {
        id = existing->second;
        return true;
    }
    if (m_recordNames.size() >= kMaxUniqueRecordNames)
        return false;
    const std::uint32_t nextId =
        static_cast<std::uint32_t>(m_recordNames.size());
    m_recordNames.push_back(normalized);
    try {
        const auto inserted = m_recordNameIds.emplace(
            std::move(normalized), nextId);
        if (!inserted.second) {
            m_recordNames.pop_back();
            id = inserted.first->second;
            return true;
        }
    } catch (...) {
        m_recordNames.pop_back();
        throw;
    }
    id = nextId;
    return true;
}

bool dxfWriter::finalizeRecordInventory() noexcept {
    if (!m_recordInventoryEnabled)
        return true;
    if (m_recordInventoryFailure)
        return false;
    std::sort(m_emittedRecords.begin(), m_emittedRecords.end(),
        [](const EmittedRecordIdentity& left,
           const EmittedRecordIdentity& right) {
            if (left.handle != right.handle)
                return left.handle < right.handle;
            if (left.recordName != right.recordName)
                return left.recordName < right.recordName;
            return left.sectionName < right.sectionName;
        });
    for (std::size_t index = 1; index < m_emittedRecords.size(); ++index) {
        if (m_emittedRecords[index - 1u].handle
            == m_emittedRecords[index].handle) {
            m_recordInventoryFailure = true;
            return false;
        }
    }
    m_recordOwnerPending = false;
    m_recordInventoryFinalized = true;
    return true;
}

std::size_t dxfWriter::recordHandleCount(std::uint32_t handle) const noexcept {
    if (m_recordInventoryFinalized) {
        const auto first = std::lower_bound(
            m_emittedRecords.begin(), m_emittedRecords.end(), handle,
            [](const EmittedRecordIdentity& record, std::uint32_t value) {
                return record.handle < value;
            });
        const auto last = std::upper_bound(
            first, m_emittedRecords.end(), handle,
            [](std::uint32_t value, const EmittedRecordIdentity& record) {
                return value < record.handle;
            });
        return static_cast<std::size_t>(last - first);
    }
    return static_cast<std::size_t>(std::count_if(
        m_emittedRecords.begin(), m_emittedRecords.end(),
        [handle](const EmittedRecordIdentity& record) {
            return record.handle == handle;
        }));
}

bool dxfWriter::recordHandleHasIdentity(
        std::uint32_t handle, const std::string& recordName,
        const std::string& sectionName) const {
    const std::string normalizedRecordName =
        normalizeDxfRecordName(recordName);
    const std::string normalizedSectionName =
        normalizeDxfRecordName(sectionName);
    const auto recordId = m_recordNameIds.find(normalizedRecordName);
    const auto sectionId = m_recordNameIds.find(normalizedSectionName);
    if (recordId == m_recordNameIds.end() || sectionId == m_recordNameIds.end())
        return false;
    const auto matches = [&recordId, &sectionId, handle](
                             const EmittedRecordIdentity& record) {
        return record.handle == handle
            && record.recordName == recordId->second
            && record.sectionName == sectionId->second;
    };
    if (m_recordInventoryFinalized) {
        const auto first = std::lower_bound(
            m_emittedRecords.begin(), m_emittedRecords.end(), handle,
            [](const EmittedRecordIdentity& record, std::uint32_t value) {
                return record.handle < value;
            });
        return first != m_emittedRecords.end() && matches(*first);
    }
    return std::any_of(m_emittedRecords.begin(), m_emittedRecords.end(), matches);
}

bool dxfWriter::recordHandleHasOwner(
        std::uint32_t handle, std::uint32_t ownerHandle) const noexcept {
    const auto matches = [handle, ownerHandle](
                             const EmittedRecordIdentity& record) {
        return record.handle == handle && record.hasOwnerHandle
            && record.ownerHandle == ownerHandle;
    };
    if (m_recordInventoryFinalized) {
        const auto first = std::lower_bound(
            m_emittedRecords.begin(), m_emittedRecords.end(), handle,
            [](const EmittedRecordIdentity& record, std::uint32_t value) {
                return record.handle < value;
            });
        return first != m_emittedRecords.end() && matches(*first);
    }
    return std::any_of(m_emittedRecords.begin(), m_emittedRecords.end(), matches);
}

std::vector<std::uint32_t> dxfWriter::emittedRecordHandles() const {
    std::vector<std::uint32_t> handles;
    handles.reserve(m_emittedRecords.size());
    for (const EmittedRecordIdentity& record : m_emittedRecords)
        handles.push_back(record.handle);
    return handles;
}

//RLZ TODO change std::endl to x0D x0A (13 10)
/*bool dxfWriter::readRec(int *codeData, bool skip) {
//    std::string text;
    int code;

#ifdef DRW_DBG
    count = count+2; //DBG
#endif

    if (!readCode(&code))
        return false;
    *codeData = code;

    if (code < 10)
        readString();
    else if (code < 60)
        readDouble();
    else if (code < 80)
        readInt();
    else if (code > 89 && code < 100) //TODO this is an int 32b
        readInt32();
    else if (code == 100 || code == 102 || code == 105)
        readString();
    else if (code > 109 && code < 150) //skip not used at the v2012
        readDouble();
    else if (code > 159 && code < 170) //skip not used at the v2012
        readInt64();
    else if (code < 180)
        readInt();
    else if (code > 209 && code < 240) //skip not used at the v2012
        readDouble();
    else if (code > 269 && code < 290) //skip not used at the v2012
        readInt();
    else if (code < 300) //TODO this is a boolean indicator, int in Binary?
        readBool();
    else if (code < 370)
        readString();
    else if (code < 390)
        readInt();
    else if (code < 400)
        readString();
    else if (code < 410)
        readInt();
    else if (code < 420)
        readString();
    else if (code < 430) //TODO this is an int 32b
        readInt32();
    else if (code < 440)
        readString();
    else if (code < 450) //TODO this is an int 32b
        readInt32();
    else if (code < 460) //TODO this is long??
        readInt();
    else if (code < 470) //TODO this is a floating point double precision??
        readDouble();
    else if (code < 481)
        readString();
    else if (code > 998 && code < 1009) //skip not used at the v2012
        readString();
    else if (code < 1060) //TODO this is a floating point double precision??
        readDouble();
    else if (code < 1071)
        readInt();
    else if (code == 1071) //TODO this is an int 32b
        readInt32();
    else if (skip)
        //skip safely this dxf entry ( ok for ascii dxf)
        readString();
    else
        //break in binary files because the conduct is unpredictable
        return false;

    return (filestr->good());
}*/

bool dxfWriter::validateStrictText(std::string_view text) {
    if (m_textFailure != TextFailure::NoFailure) return false;
    if (!DRW_TextCodec::isUtf8ScalarText(text)) return failText(TextFailure::InvalidUtf8);
    if (text.find('\0') != std::string_view::npos
        || (!m_binaryStrings && (text.find('\r') != std::string_view::npos
                                 || text.find('\n') != std::string_view::npos)))
        return failText(TextFailure::UnsafeRecordText);
    const auto target = encoder.getSourceVersion();
    // Normalized metadata can say ANSI_1252 while a legacy UTF8 declaration
    // selected the pass-through converter. Guard the original selection.
    const auto declaration = encoder.declaredCodePageSelection();
    const auto page = declaration.selectedName == nullptr
        ? std::string_view{} : std::string_view(declaration.selectedName);
    if (target < DRW::AC1009 || target > DRW::AC1032
        || (target < DRW::AC1021 && (page == "UTF-8" || page == "UTF-16")))
        return failText(TextFailure::UnsupportedProfile);
    return true;
}

bool dxfWriter::encodeStrictTextScalar(std::string_view text,
                                      std::size_t offset,
                                      std::size_t& sourceBytes,
                                      std::string& encoded) {
    if (offset >= text.size()) return failText(TextFailure::InvalidUtf8);
    const auto lead = static_cast<unsigned char>(text[offset]);
    sourceBytes = lead < 0x80u ? 1u : lead < 0xE0u ? 2u : lead < 0xF0u ? 3u : 4u;
    if (sourceBytes > text.size() - offset)
        return failText(TextFailure::InvalidUtf8);
    const auto scalar = text.substr(offset, sourceBytes); // complete scalar, validated by caller
    if (!m_binaryStrings && sourceBytes == 1u) {
        char caret[2] = {};
        const std::size_t caretBytes = encodeDxfAsciiControl(lead, caret);
        if (caretBytes != 0u) {
            encoded.assign(caret, caretBytes);
            return true;
        }
    }
    // Quote only a semantic backslash which would start CIF/MIF decoding.
    // One-pass decoding of \\U+005C then literal U+/M+ preserves meaning;
    // ordinary paths, MTEXT \\P/font/stack commands and raw bytes are untouched.
    const bool quote = lead == '\\' && offset + 2u < text.size()
        && (text[offset + 1u] == 'U' || text[offset + 1u] == 'M')
        && text[offset + 2u] == '+';
    encoded = quote ? "\\U+005C" : encoder.fromUtf8(scalar);
    // This is a local loss guard, not an independent charset mapping oracle.
    // Literal scalar/wire tests separately constrain the supported transport.
    if (encoder.toUtf8(encoded) != scalar)
        return failText(TextFailure::TargetLoss);
    return true;
}

bool dxfWriter::encodeStrictTextString(std::string_view text,
                                       std::string& encoded) {
    if (!validateStrictText(text)) return false;
    std::string value;
    if (text.size() > value.max_size())
        return failText(TextFailure::TargetLoss);
    value.reserve(text.size());
    for (std::size_t offset = 0; offset < text.size();) {
        std::size_t sourceBytes = 0u;
        std::string scalar;
        if (!encodeStrictTextScalar(text, offset, sourceBytes, scalar))
            return false;
        if (scalar.size() > value.max_size() - value.size())
            return failText(TextFailure::TargetLoss);
        value.append(scalar);
        offset += sourceBytes;
    }
    encoded.swap(value);
    return true;
}

bool dxfWriter::encodeStrictText(std::string_view text, std::size_t maxBytes,
                                 std::string& encoded,
                                 std::vector<EncodedTextChunk>& chunks,
                                 std::size_t maxCharacters,
                                 bool shortFinalChunk) {
    if (!validateStrictText(text)) return false;
    if (maxBytes == 0u) return failText(TextFailure::TargetLoss);
    encoded.clear();
    chunks.clear();
    std::size_t chunkStart = 0u;
    std::size_t chunkCharacters = 0u;
    std::size_t lastScalarBytes = 0u;
    for (std::size_t offset = 0; offset < text.size();) {
        std::size_t sourceBytes = 0u;
        std::string scalar;
        if (!encodeStrictTextScalar(text, offset, sourceBytes, scalar)
            || scalar.size() > maxBytes)
            return failText(TextFailure::TargetLoss);
        const std::size_t chunkBytes = encoded.size() - chunkStart;
        if (chunkBytes != 0u
            && (scalar.size() > maxBytes - chunkBytes
                || (maxCharacters != 0u
                    && chunkCharacters >= maxCharacters))) {
            chunks.push_back({chunkStart, chunkBytes});
            chunkStart = encoded.size();
            chunkCharacters = 0u;
            lastScalarBytes = 0u;
        }
        if (scalar.size() > encoded.max_size() - encoded.size())
            return failText(TextFailure::TargetLoss);
        encoded.append(scalar);
        ++chunkCharacters;
        lastScalarBytes = scalar.size();
        offset += sourceBytes;
    }
    const std::size_t finalChunkSize = encoded.size() - chunkStart;
    if (shortFinalChunk && !chunks.empty()
        && maxCharacters != 0u && chunkCharacters == maxCharacters
        && lastScalarBytes != 0u && lastScalarBytes <= finalChunkSize) {
        // Autodesk's MTEXT profile requires a short group-1 remainder whenever
        // continuation groups are present. If the last chunk exactly filled
        // the character budget, make its last complete encoded scalar the
        // final group-1 value instead of emitting a full-size tail.
        const std::size_t finalScalarOffset = encoded.size() - lastScalarBytes;
        chunks.push_back({chunkStart, finalChunkSize - lastScalarBytes});
        chunks.push_back({finalScalarOffset, lastScalarBytes});
    } else {
        chunks.push_back({chunkStart, finalChunkSize}); // empty field remains explicit
    }
    return true;
}

std::string dxfWriter::fromUtf8String(std::string_view text) {
    if (!requiresLosslessText()) {
        std::string encoded;
        if (!encodeCompatibleAsciiText(text, encoded))
            return {};
        return encoded;
    }
    std::string encoded;
    if (!encodeStrictTextString(text, encoded)) return {};
    return encoded;
}

bool dxfWriter::encodeCompatibleAsciiText(std::string_view text,
                                          std::string& encoded) {
    if (m_binaryStrings) {
        encoded = encoder.fromUtf8(text);
        return true;
    }

    std::string value;
    if (text.size() > value.max_size())
        return recordWriteResult(false);
    value.reserve(text.size());
    for (std::size_t offset = 0; offset < text.size();) {
        const unsigned char byte = static_cast<unsigned char>(text[offset]);
        if (byte < 0x80u) {
            char caret[2] = {};
            const std::size_t caretBytes = encodeDxfAsciiControl(byte, caret);
            const std::size_t appendBytes = caretBytes == 0u ? 1u : caretBytes;
            if (appendBytes > value.max_size() - value.size())
                return recordWriteResult(false);
            if (caretBytes == 0u)
                value.push_back(static_cast<char>(byte));
            else
                value.append(caret, caretBytes);
            ++offset;
            continue;
        }

        // Convert non-ASCII runs as runs, preserving DBCS/codepage behavior;
        // bytewise caret scanning here could mistake a DBCS trail byte for a
        // literal ASCII caret.
        const std::size_t runStart = offset;
        do {
            ++offset;
        } while (offset < text.size()
                 && static_cast<unsigned char>(text[offset]) >= 0x80u);
        const std::string converted = encoder.fromUtf8(
            text.substr(runStart, offset - runStart));
        if (converted.size() > value.max_size() - value.size())
            return recordWriteResult(false);
        value.append(converted);
    }
    encoded.swap(value);
    return true;
}

bool dxfWriter::writeUtf8String(int code, std::string_view text) {
    // R2007+ DXF semantic strings are UTF-8 regardless of the legacy
    // $DWGCODEPAGE spelling. In compatible mode the configured DXF converter
    // is an identity converter, so pass the caller's storage directly to the
    // synchronous sink instead of materializing an identical full-size copy.
    // Keep strict output on the validation/quoting path below.
    if (usesCompatibleModernDxfUtf8()
        && (m_binaryStrings || !hasDxfAsciiControlToEscape(text)))
        return writeString(code, text);
    std::string encoded = fromUtf8String(text);
    return m_textFailure == TextFailure::NoFailure && writeString(code, encoded);
}

bool dxfWriter::writeUtf8Chunks(std::string_view text, int continuationCode,
                               int finalCode, std::size_t maxBytes) {
    std::string encoded;
    std::vector<EncodedTextChunk> chunks;
    if (!encodeStrictText(text, maxBytes, encoded, chunks)) return false;
    for (std::size_t index = 0; index < chunks.size(); ++index) {
        const EncodedTextChunk& chunk = chunks[index];
        if (!writeString(index + 1u == chunks.size() ? finalCode : continuationCode,
                         std::string_view(encoded.data() + chunk.offset,
                                          chunk.size))) return false;
    }
    return true;
}

bool dxfWriter::encodeTextPreservingCaretJ(
        std::string_view text, std::size_t maxBytes, std::string& encoded,
        std::vector<EncodedTextChunk>& chunks) {
    if (maxBytes == 0u)
        return recordWriteResult(false);
    if (requiresLosslessText() && !validateStrictText(text))
        return false;

    encoded.clear();
    chunks.clear();
    std::size_t chunkStart = 0u;
    for (std::size_t offset = 0u; offset < text.size();) {
        std::size_t sourceBytes = 0u;
        std::string encodedScalar;
        if (text.substr(offset, 2u) == "^J") {
            // GEODATA's writer has already mapped a source LF to this DXF
            // control spelling. Do not quote the caret a second time.
            sourceBytes = 2u;
            encodedScalar = "^J";
        } else if (requiresLosslessText()) {
            if (!encodeStrictTextScalar(text, offset, sourceBytes,
                                        encodedScalar))
                return false;
        } else {
            const auto lead = static_cast<unsigned char>(text[offset]);
            sourceBytes = lead < 0x80u ? 1u
                : lead < 0xE0u ? 2u : lead < 0xF0u ? 3u : 4u;
            if (sourceBytes > text.size() - offset)
                sourceBytes = 1u;
            for (std::size_t index = 1u; index < sourceBytes; ++index) {
                const auto continuation =
                    static_cast<unsigned char>(text[offset + index]);
                if ((continuation & 0xC0u) != 0x80u) {
                    sourceBytes = 1u;
                    break;
                }
            }
            encodedScalar = fromUtf8String(text.substr(offset, sourceBytes));
            if (hasWriteError())
                return false;
        }

        if (sourceBytes == 0u || encodedScalar.size() > maxBytes
            || encodedScalar.size() > encoded.max_size() - encoded.size())
            return recordWriteResult(false);
        const std::size_t chunkBytes = encoded.size() - chunkStart;
        if (chunkBytes != 0u
            && encodedScalar.size() > maxBytes - chunkBytes) {
            chunks.push_back({chunkStart, chunkBytes});
            chunkStart = encoded.size();
        }
        encoded.append(encodedScalar);
        offset += sourceBytes;
    }
    chunks.push_back({chunkStart, encoded.size() - chunkStart});
    return true;
}

bool dxfWriter::writeUtf8StringPreservingCaretJ(int code,
                                                std::string_view text) {
    std::string encoded;
    std::vector<EncodedTextChunk> chunks;
    if (!encodeTextPreservingCaretJ(
            text, std::numeric_limits<std::size_t>::max(), encoded, chunks)
        || chunks.size() != 1u)
        return false;
    return writeString(code, encoded);
}

bool dxfWriter::writeUtf8ChunksPreservingCaretJ(
        std::string_view text, int continuationCode, int finalCode,
        std::size_t maxBytes) {
    std::string encoded;
    std::vector<EncodedTextChunk> chunks;
    if (!encodeTextPreservingCaretJ(text, maxBytes, encoded, chunks))
        return false;
    for (std::size_t index = 0; index < chunks.size(); ++index) {
        const EncodedTextChunk& chunk = chunks[index];
        if (!writeString(index + 1u == chunks.size() ? finalCode
                                                     : continuationCode,
                         std::string_view(encoded.data() + chunk.offset,
                                          chunk.size)))
            return false;
    }
    return true;
}

bool dxfWriter::writeUtf8MTextChunks(std::string_view text,
                                    int continuationCode, int finalCode,
                                    std::size_t maxBytes,
                                    std::size_t maxCharacters) {
    if (maxBytes == 0u || maxCharacters == 0u)
        return recordWriteResult(false);

    std::string normalizedText;
    if (text.find('\0') != std::string_view::npos)
        return failText(TextFailure::UnsafeRecordText);
    const bool hasLineBreak = text.find('\r') != std::string_view::npos
        || text.find('\n') != std::string_view::npos;
    if (hasLineBreak && !requiresLosslessText() && encoder.isDxfFormat()
        && encoder.getSourceVersion() >= DRW::AC1021) {
        // Modern compatible DXF uses an identity UTF-8 mapping. Normalize
        // CR/LF lazily into the one encoded backing store rather than keeping
        // both a full normalized string and its chunked output alive.
        std::string encodedChunks;
        std::vector<EncodedTextChunk> chunks;
        std::size_t chunkStart = 0u;
        std::size_t chunkCharacters = 0u;
        std::size_t lastScalarBytes = 0u;
        const auto appendEncodedScalar = [&](std::string_view encoded) {
            if (encoded.size() > maxBytes)
                return recordWriteResult(false);
            const std::size_t chunkBytes = encodedChunks.size() - chunkStart;
            if (chunkBytes != 0u
                && (encoded.size() > maxBytes - chunkBytes
                    || chunkCharacters >= maxCharacters)) {
                chunks.push_back({chunkStart, chunkBytes});
                chunkStart = encodedChunks.size();
                chunkCharacters = 0u;
                lastScalarBytes = 0u;
            }
            if (encoded.size() > encodedChunks.max_size() - encodedChunks.size())
                return recordWriteResult(false);
            encodedChunks.append(encoded);
            ++chunkCharacters;
            lastScalarBytes = encoded.size();
            return true;
        };

        std::size_t sourceOffset = 0u;
        bool pendingParagraphMarker = false;
        const auto nextNormalizedByte = [&](char& value) {
            if (pendingParagraphMarker) {
                pendingParagraphMarker = false;
                value = 'P';
                return 1;
            }
            if (sourceOffset == text.size())
                return 0;
            const char current = text[sourceOffset++];
            if (current == '\r') {
                if (sourceOffset == text.size() || text[sourceOffset] != '\n')
                    return -1;
                ++sourceOffset;
                pendingParagraphMarker = true;
                value = '\\';
                return 1;
            }
            if (current == '\n') {
                pendingParagraphMarker = true;
                value = '\\';
                return 1;
            }
            value = current;
            return 1;
        };

        for (;;) {
            std::array<char, 4> scalar{};
            const int first = nextNormalizedByte(scalar[0]);
            if (first == 0)
                break;
            if (first < 0)
                return failText(TextFailure::UnsafeRecordText);
            const auto lead = static_cast<unsigned char>(scalar[0]);
            const std::size_t expected = lead < 0x80u ? 1u
                : lead < 0xE0u ? 2u : lead < 0xF0u ? 3u : 4u;
            std::size_t scalarSize = 1u;
            while (scalarSize < expected) {
                const int next = nextNormalizedByte(scalar[scalarSize]);
                if (next < 0)
                    return failText(TextFailure::UnsafeRecordText);
                if (next == 0)
                    break;
                ++scalarSize;
            }
            std::string_view encodedScalar(scalar.data(), scalarSize);
            char caret[2] = {};
            const std::size_t caretBytes = !m_binaryStrings && scalarSize == 1u
                ? encodeDxfAsciiControl(
                    static_cast<unsigned char>(scalar[0]), caret) : 0u;
            if (caretBytes != 0u)
                encodedScalar = std::string_view(caret, caretBytes);
            if (!appendEncodedScalar(encodedScalar))
                return false;
        }

        const std::size_t finalChunkSize = encodedChunks.size() - chunkStart;
        if (!chunks.empty() && chunkCharacters == maxCharacters
            && lastScalarBytes != 0u && lastScalarBytes <= finalChunkSize) {
            const std::size_t finalScalarOffset =
                encodedChunks.size() - lastScalarBytes;
            chunks.push_back({chunkStart, finalChunkSize - lastScalarBytes});
            chunks.push_back({finalScalarOffset, lastScalarBytes});
        } else {
            chunks.push_back({chunkStart, finalChunkSize});
        }
        for (std::size_t index = 0; index < chunks.size(); ++index) {
            const EncodedTextChunk& chunk = chunks[index];
            if (!writeString(index + 1u == chunks.size() ? finalCode
                                                         : continuationCode,
                             std::string_view(encodedChunks.data() + chunk.offset,
                                              chunk.size)))
                return false;
        }
        return true;
    }
    if (text.find('\r') != std::string_view::npos
        || text.find('\n') != std::string_view::npos) {
        normalizedText.reserve(text.size());
        for (std::size_t offset = 0; offset < text.size();) {
            if (text[offset] == '\r') {
                if (offset + 1u >= text.size() || text[offset + 1u] != '\n')
                    return failText(TextFailure::UnsafeRecordText);
                normalizedText += "\\P";
                offset += 2u;
            } else if (text[offset] == '\n') {
                normalizedText += "\\P";
                ++offset;
            } else {
                normalizedText.push_back(text[offset]);
                ++offset;
            }
        }
        text = normalizedText;
    }

    std::string encodedChunks;
    std::vector<EncodedTextChunk> chunks;
    if (requiresLosslessText()) {
        if (!encodeStrictText(text, maxBytes, encodedChunks, chunks,
                              maxCharacters, true))
            return false;
    } else {
        std::size_t chunkStart = 0u;
        std::size_t chunkCharacters = 0u;
        std::size_t lastScalarBytes = 0u;
        const bool asciiPassThrough = encoder.getCodePage() != "UTF-16";
        const auto appendEncodedScalar = [&](std::string_view encoded) {
            if (encoded.size() > maxBytes)
                return recordWriteResult(false);
            const std::size_t chunkBytes = encodedChunks.size() - chunkStart;
            if (chunkBytes != 0u
                && (encoded.size() > maxBytes - chunkBytes
                    || chunkCharacters >= maxCharacters)) {
                chunks.push_back({chunkStart, chunkBytes});
                chunkStart = encodedChunks.size();
                chunkCharacters = 0u;
                lastScalarBytes = 0u;
            }
            if (encoded.size() > encodedChunks.max_size() - encodedChunks.size())
                return recordWriteResult(false);
            encodedChunks.append(encoded);
            ++chunkCharacters;
            lastScalarBytes = encoded.size();
            return true;
        };
        for (std::size_t offset = 0; offset < text.size();) {
            const auto lead = static_cast<unsigned char>(text[offset]);
            std::size_t length = lead < 0x80u ? 1u
                : lead < 0xE0u ? 2u : lead < 0xF0u ? 3u : 4u;
            if (length > text.size() - offset)
                length = text.size() - offset;
            if (lead < 0x80u) {
                char caret[2] = {};
                const std::size_t caretBytes = !m_binaryStrings
                    ? encodeDxfAsciiControl(lead, caret) : 0u;
                const std::string_view scalar = caretBytes != 0u
                    ? std::string_view(caret, caretBytes)
                    : text.substr(offset, length);
                if (asciiPassThrough || usesCompatibleModernDxfUtf8()
                    || caretBytes != 0u) {
                    if (!appendEncodedScalar(scalar))
                        return false;
                } else {
                    const std::string encoded = fromUtf8String(
                        text.substr(offset, length));
                    if (!appendEncodedScalar(encoded))
                        return false;
                }
            } else if (usesCompatibleModernDxfUtf8()) {
                // Modern compatible DXF is an identity UTF-8 mapping for
                // every scalar, not only ASCII. Keep the caller's scalar as
                // a view instead of allocating a per-scalar encoded string.
                if (!appendEncodedScalar(text.substr(offset, length)))
                    return false;
            } else {
                const std::string encoded =
                    fromUtf8String(text.substr(offset, length));
                if (!appendEncodedScalar(encoded))
                    return false;
            }
            offset += length;
        }
        const std::size_t finalChunkSize = encodedChunks.size() - chunkStart;
        if (!chunks.empty() && chunkCharacters == maxCharacters
            && lastScalarBytes != 0u && lastScalarBytes <= finalChunkSize) {
            const std::size_t finalScalarOffset =
                encodedChunks.size() - lastScalarBytes;
            chunks.push_back({chunkStart, finalChunkSize - lastScalarBytes});
            chunks.push_back({finalScalarOffset, lastScalarBytes});
        } else {
            chunks.push_back({chunkStart, finalChunkSize});
        }
    }

    for (std::size_t index = 0; index < chunks.size(); ++index) {
        const EncodedTextChunk& chunk = chunks[index];
        if (!writeString(index + 1u == chunks.size() ? finalCode
                                                     : continuationCode,
                         std::string_view(encodedChunks.data() + chunk.offset,
                                          chunk.size)))
            return false;
    }
    return true;
}

bool dxfWriter::preflightOpaqueText(std::string_view text, bool knownUtf8) {
    if (!requiresLosslessText()) return true;
    const bool highBytes = std::any_of(text.begin(), text.end(), [](unsigned char byte) { return byte >= 0x80u; });
    if (!highBytes) return true;
    if (!knownUtf8) return failText(TextFailure::OpaqueText);
    return DRW_TextCodec::isUtf8ScalarText(text) || failText(TextFailure::InvalidUtf8);
}

bool dxfWriter::writeUtf8Caps(int code, const std::string& text) {
    // Symbol fields are commonly already uppercase. For compatible DXF output,
    // plain ASCII without lowercase or caret/control syntax is byte-identical
    // in all supported single-byte and UTF-8 DXF encodings, so let the synchronous
    // raw sink consume the caller's view instead of creating both a casing
    // copy and an encoded copy. Keep strict-profile validation/quoting and the
    // read-only UTF-16 converter on the existing path.
    if (!requiresLosslessText() && encoder.getCodePage() != "UTF-16"
        && text.find('\\') == std::string::npos) {
        const bool alreadyUpperAscii = std::all_of(
            text.begin(), text.end(), [this](unsigned char value) {
                char caret[2] = {};
                return value < 0x80u && !(value >= 'a' && value <= 'z')
                    && (m_binaryStrings
                        || encodeDxfAsciiControl(value, caret) == 0u);
            });
        if (alreadyUpperAscii)
            return writeString(code, text);
    }
    std::string strname = text;
    std::transform(strname.begin(), strname.end(), strname.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(dxfAsciiUpper(ch));
                   });
    return writeUtf8String(code, strname);
}

bool dxfWriterBinary::writeString(int code, std::string_view text) {
    return writeStringImpl(code, text);
}

bool dxfWriterBinary::writeRawString(int code, std::string_view text) {
    // A binary DXF value's width is determined by its group code. Raw text
    // replay cannot safely supply a value for a code whose type is unknown.
    return writeStringImpl(code, text);
}

bool dxfWriterBinary::writeStringImpl(
        int code, std::string_view text) {
    const DxfValueKind kind = dxfValueKindForCode(code, classifierProfile());
    const bool binaryChunk = isDxfBinaryChunkCode(code);
    if ((binaryChunk && kind != DxfValueKind::Bin)
        || (!binaryChunk && kind != DxfValueKind::Str)) {
        DRW_DBG("dxfWriterBinary rejected string value, code ");
        DRW_DBG(code);
        DRW_DBG("\n");
        return recordWriteResult(false);
    }
    if ((code >= 310 && code <= 319) || code == 1004) {
        if ((text.size() % 2) != 0 || text.size() / 2 > 127)
            return recordWriteResult(false);
        auto hexValue = [](char ch) -> int {
            unsigned char c = static_cast<unsigned char>(ch);
            if (c >= '0' && c <= '9')
                return c - '0';
            c = dxfAsciiUpper(c);
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            return -1;
        };
        for (char ch : text) {
            if (hexValue(ch) < 0)
                return recordWriteResult(false);
        }

        if (!writeCode(code))
            return recordWriteResult(false);
        const unsigned char chunkLen = static_cast<unsigned char>(text.size() / 2);
        filestr->write(reinterpret_cast<const char*>(&chunkLen), 1);
        for (std::size_t i = 0; i < text.size(); i += 2) {
            int hi = hexValue(text[i]);
            int lo = hexValue(text[i + 1]);
            unsigned char value = static_cast<unsigned char>((hi << 4) | lo);
            filestr->write(reinterpret_cast<const char*>(&value), 1);
        }
        return recordWriteResult(filestr->good());
    }
    if (!isSafeBinaryDxfString(text) || !writeCode(code))
        return recordWriteResult(false);
    if (!text.empty())
        filestr->write(text.data(), static_cast<std::streamsize>(text.size()));
    filestr->put('\0');
    if (!recordWriteResult(filestr->good()))
        return false;
    return recordWriteResult(observeRecordString(code, text));
}

/*bool dxfWriterBinary::readCode(int *code) {
    unsigned short *int16p;
    char buffer[2];
    filestr->read(buffer,2);
    int16p = (unsigned short *) buffer;
//exist a 32bits int (code 90) with 2 bytes???
    if ((*code == 90) && (*int16p>2000)){
        DBG(*code); DBG(" de 16bits\n");
        filestr->seekg(-4, std::ios_base::cur);
        filestr->read(buffer,2);
        int16p = (unsigned short *) buffer;
    }
    *code = *int16p;
    DBG(*code); DBG("\n");

    return (filestr->good());
}*/

/*bool dxfWriterBinary::readString() {
    std::getline(*filestr, strData, '\0');
    DBG(strData); DBG("\n");
    return (filestr->good());
}*/

/*bool dxfWriterBinary::readString(std::string *text) {
    std::getline(*filestr, *text, '\0');
    DBG(*text); DBG("\n");
    return (filestr->good());
}*/

bool dxfWriterBinary::writeInt16(int code, int data) {
    if (!valueKindMatches(code, DxfValueKind::I16)
        || !isDxfInt16Value(data)) {
        DRW_DBG("dxfWriterBinary rejected int16 value, code ");
        DRW_DBG(code);
        DRW_DBG("\n");
        return recordWriteResult(false);
    }
    const bool written = writeCode(code)
        && writeLittleEndian(filestr,
                             static_cast<std::uint16_t>(data), sizeof(std::uint16_t));
    return recordWriteResult(written);
}

bool dxfWriterBinary::writeInt32(int code, int data) {
    if (!valueKindMatches(code, DxfValueKind::I32)) {
        DRW_DBG("dxfWriterBinary rejected int32 value, code ");
        DRW_DBG(code);
        DRW_DBG("\n");
        return recordWriteResult(false);
    }
    const bool written = writeCode(code)
        && writeLittleEndian(filestr,
                             static_cast<std::uint32_t>(data), sizeof(std::uint32_t));
    return recordWriteResult(written);
}

bool dxfWriterBinary::writeInt64(int code, std::int64_t data) {
    if (!valueKindMatches(code, DxfValueKind::I64)) {
        DRW_DBG("dxfWriterBinary rejected int64 value, code ");
        DRW_DBG(code);
        DRW_DBG("\n");
        return recordWriteResult(false);
    }
    const std::uint64_t bits = static_cast<std::uint64_t>(data);
    const bool written = writeCode(code)
        && writeLittleEndian(filestr, bits, sizeof(bits));
    return recordWriteResult(written);
}

bool dxfWriterBinary::writeDouble(int code, double data) {
    if (!valueKindMatches(code, DxfValueKind::Dbl)
        || !std::isfinite(data)) {
        DRW_DBG("dxfWriterBinary rejected double value, code ");
        DRW_DBG(code);
        DRW_DBG("\n");
        return recordWriteResult(false);
    }
    std::uint64_t bits = 0;
    static_assert(sizeof(bits) == sizeof(data), "DXF doubles require 64 bits");
    std::memcpy(&bits, &data, sizeof(bits));
    const bool written = writeCode(code)
        && writeLittleEndian(filestr, bits, sizeof(bits));
    return recordWriteResult(written);
}

//saved as int or add a bool member??
bool dxfWriterBinary::writeBool(int code, bool data) {
    if (!valueKindMatches(code, DxfValueKind::Bln)) {
        DRW_DBG("dxfWriterBinary rejected Boolean value, code ");
        DRW_DBG(code);
        DRW_DBG("\n");
        return recordWriteResult(false);
    }
    const bool written = writeCode(code)
        && writeLittleEndian(filestr, data ? 1U : 0U, 1);
    return recordWriteResult(written);
}

bool dxfWriterBinary::writeCode(int code) {
    return writeBinaryDxfCode(filestr, code);
}

bool dxfWriterBinaryR12::writeCode(int code) {
    return writeBinaryDxfR12Code(filestr, code);
}

dxfWriterAscii::dxfWriterAscii(std::ostream *stream):dxfWriter(stream){}

bool dxfWriterAscii::writeString(int code, std::string_view text) {
    return writeStringImpl(code, text, false);
}

bool dxfWriterAscii::writeRawString(int code, std::string_view text) {
    return writeStringImpl(code, text, true);
}

bool dxfWriterAscii::writeStringImpl(
        int code, std::string_view text, bool allowUnknownRawString) {
    const DxfValueKind kind = dxfValueKindForCode(code, classifierProfile());
    const bool binaryChunk = isDxfBinaryChunkCode(code);
    if ((binaryChunk && kind != DxfValueKind::Bin)
        || (!allowUnknownRawString && !binaryChunk
            && kind != DxfValueKind::Str))
        return recordWriteResult(false);
    if (binaryChunk) {
        if ((text.size() % 2u) != 0u || text.size() / 2u > 127u)
            return recordWriteResult(false);
        const auto hexValue = [](char ch) {
            const unsigned char c = static_cast<unsigned char>(ch);
            if (c >= '0' && c <= '9')
                return static_cast<int>(c - '0');
            if (c >= 'A' && c <= 'F')
                return static_cast<int>(c - 'A') + 10;
            if (c >= 'a' && c <= 'f')
                return static_cast<int>(c - 'a') + 10;
            return -1;
        };
        for (const char ch : text) {
            if (hexValue(ch) < 0)
                return recordWriteResult(false);
        }
    }
    if (filestr == nullptr || !isAsciiDxfCode(code)
        || !isSafeAsciiDxfString(text))
        return recordWriteResult(false);
    if (!writeAsciiDxfIntegerValue(filestr, code, 3))
        return recordWriteResult(false);
    if (!text.empty())
        filestr->write(text.data(), static_cast<std::streamsize>(text.size()));
    filestr->put('\n');
    if (!recordWriteResult(filestr->good()))
        return false;
    return recordWriteResult(observeRecordString(code, text));
}

bool dxfWriterAscii::writeInt16(int code, int data) {
    if (!valueKindMatches(code, DxfValueKind::I16)
        || !isDxfInt16Value(data))
        return recordWriteResult(false);
    return recordWriteResult(writeAsciiDxfInteger(filestr, code, data));
}

bool dxfWriterAscii::writeInt32(int code, int data) {
    if (!valueKindMatches(code, DxfValueKind::I32))
        return recordWriteResult(false);
    return recordWriteResult(writeAsciiDxfInteger(filestr, code, data));
}

bool dxfWriterAscii::writeInt64(int code, std::int64_t data) {
    if (!valueKindMatches(code, DxfValueKind::I64))
        return recordWriteResult(false);
    return recordWriteResult(writeAsciiDxfInteger(filestr, code, data));
}

bool dxfWriterAscii::writeDouble(int code, double data) {
    if (!valueKindMatches(code, DxfValueKind::Dbl)
        || filestr == nullptr || !isAsciiDxfCode(code) || !std::isfinite(data))
        return recordWriteResult(false);
    if (!writeAsciiDxfIntegerValue(filestr, code, 3))
        return recordWriteResult(false);
    const std::string text = DxfNumeric::formatDouble(data);
    filestr->write(text.data(), static_cast<std::streamsize>(text.size()));
    filestr->put('\n');
    return recordWriteResult(filestr->good());
}

//saved as int or add a bool member??
bool dxfWriterAscii::writeBool(int code, bool data) {
    if (!valueKindMatches(code, DxfValueKind::Bln)
        || filestr == nullptr || !isAsciiDxfCode(code))
        return recordWriteResult(false);
    return recordWriteResult(writeAsciiDxfIntegerValue(filestr, code, 0)
        && writeAsciiDxfIntegerValue(filestr, data ? 1 : 0, 0));
}
