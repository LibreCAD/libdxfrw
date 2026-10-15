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

bool isSafeAsciiDxfString(const std::string& value) {
    return value.find('\r') == std::string::npos
        && value.find('\n') == std::string::npos
        && value.find('\0') == std::string::npos;
}

bool isSafeBinaryDxfString(const std::string& value) {
    return value.find('\0') == std::string::npos;
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

bool dxfWriter::encodeStrictText(std::string_view text, std::size_t maxBytes,
                                 std::vector<std::string>& chunks) {
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
    if (maxBytes == 0u) return failText(TextFailure::TargetLoss);
    std::string chunk;
    for (std::size_t offset = 0; offset < text.size();) {
        const auto lead = static_cast<unsigned char>(text[offset]);
        const std::size_t length = lead < 0x80u ? 1u : lead < 0xE0u ? 2u : lead < 0xF0u ? 3u : 4u;
        const auto scalar = text.substr(offset, length); // complete scalar, validated above
        // Quote only a semantic backslash which would start CIF/MIF decoding.
        // One-pass decoding of \\U+005C then literal U+/M+ preserves meaning;
        // ordinary paths, MTEXT \\P/font/stack commands and raw bytes are untouched.
        const bool quote = lead == '\\' && offset + 2u < text.size()
            && (text[offset + 1u] == 'U' || text[offset + 1u] == 'M')
            && text[offset + 2u] == '+';
        const std::string encoded = quote ? "\\U+005C" : encoder.fromUtf8(scalar);
        // This is a local loss guard, not an independent charset mapping oracle.
        // Literal scalar/wire tests separately constrain the supported transport.
        if (encoder.toUtf8(encoded) != scalar || encoded.size() > maxBytes)
            return failText(TextFailure::TargetLoss);
        if (!chunk.empty() && encoded.size() > maxBytes - chunk.size()) {
            chunks.push_back(std::move(chunk));
            chunk.clear();
        }
        chunk += encoded;
        offset += length;
    }
    chunks.push_back(std::move(chunk)); // empty final field remains explicit
    return true;
}

std::string dxfWriter::fromUtf8String(std::string text) {
    if (!requiresLosslessText()) return encoder.fromUtf8(text);
    std::vector<std::string> chunks;
    if (!encodeStrictText(text, (std::numeric_limits<std::size_t>::max)(), chunks)) return {};
    return std::move(chunks.front());
}

bool dxfWriter::writeUtf8String(int code, std::string text) {
    std::string encoded = fromUtf8String(std::move(text));
    return m_textFailure == TextFailure::NoFailure && writeString(code, std::move(encoded));
}

bool dxfWriter::writeUtf8Chunks(std::string_view text, int continuationCode,
                               int finalCode, std::size_t maxBytes) {
    std::vector<std::string> chunks;
    if (!encodeStrictText(text, maxBytes, chunks)) return false;
    for (std::size_t index = 0; index < chunks.size(); ++index) {
        if (!writeString(index + 1u == chunks.size() ? finalCode : continuationCode,
                         std::move(chunks[index]))) return false;
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

bool dxfWriter::writeUtf8Caps(int code, std::string text) {
    std::string strname = text;
    std::transform(strname.begin(), strname.end(), strname.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(dxfAsciiUpper(ch));
                   });
    return writeUtf8String(code, std::move(strname));
}

bool dxfWriterBinary::writeString(int code, std::string text) {
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
    *filestr << text << '\0';
    return recordWriteResult(filestr->good());
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
    if (!isDxfInt16Value(data))
        return recordWriteResult(false);
    const bool written = writeCode(code)
        && writeLittleEndian(filestr,
                             static_cast<std::uint16_t>(data), sizeof(std::uint16_t));
    return recordWriteResult(written);
}

bool dxfWriterBinary::writeInt32(int code, int data) {
    const bool written = writeCode(code)
        && writeLittleEndian(filestr,
                             static_cast<std::uint32_t>(data), sizeof(std::uint32_t));
    return recordWriteResult(written);
}

bool dxfWriterBinary::writeInt64(int code, std::int64_t data) {
    const std::uint64_t bits = static_cast<std::uint64_t>(data);
    const bool written = writeCode(code)
        && writeLittleEndian(filestr, bits, sizeof(bits));
    return recordWriteResult(written);
}

bool dxfWriterBinary::writeDouble(int code, double data) {
    if (!std::isfinite(data))
        return recordWriteResult(false);
    std::uint64_t bits = 0;
    static_assert(sizeof(bits) == sizeof(data), "DXF doubles require 64 bits");
    std::memcpy(&bits, &data, sizeof(bits));
    const bool written = writeCode(code)
        && writeLittleEndian(filestr, bits, sizeof(bits));
    return recordWriteResult(written);
}

//saved as int or add a bool member??
bool dxfWriterBinary::writeBool(int code, bool data) {
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

bool dxfWriterAscii::writeString(int code, std::string text) {
    if (filestr == nullptr || !isAsciiDxfCode(code) || !isSafeAsciiDxfString(text))
        return recordWriteResult(false);
    if (!writeAsciiDxfIntegerValue(filestr, code, 3))
        return recordWriteResult(false);
    filestr->write(text.data(), static_cast<std::streamsize>(text.size()));
    filestr->put('\n');
    return recordWriteResult(filestr->good());
}

bool dxfWriterAscii::writeInt16(int code, int data) {
    if (!isDxfInt16Value(data))
        return recordWriteResult(false);
    return recordWriteResult(writeAsciiDxfInteger(filestr, code, data));
}

bool dxfWriterAscii::writeInt32(int code, int data) {
    return recordWriteResult(writeAsciiDxfInteger(filestr, code, data));
}

bool dxfWriterAscii::writeInt64(int code, std::int64_t data) {
    return recordWriteResult(writeAsciiDxfInteger(filestr, code, data));
}

bool dxfWriterAscii::writeDouble(int code, double data) {
    if (filestr == nullptr || !isAsciiDxfCode(code) || !std::isfinite(data))
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
    if (filestr == nullptr || !isAsciiDxfCode(code))
        return recordWriteResult(false);
    return recordWriteResult(writeAsciiDxfIntegerValue(filestr, code, 0)
        && writeAsciiDxfIntegerValue(filestr, data ? 1 : 0, 0));
}
