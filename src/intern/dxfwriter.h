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

#ifndef DXFWRITER_H
#define DXFWRITER_H

#include <cstdint>
#include <ostream>
#include <sstream>
#include <vector>

#include "drw_textcodec.h"

class dxfWriter {
public:
    enum class TextFailure { NoFailure, InvalidUtf8, UnsafeRecordText, UnsupportedProfile, TargetLoss, OpaqueText };
    explicit dxfWriter(std::ostream *stream, bool binary = false):filestr(stream), m_binaryStrings(binary) {}
    virtual ~dxfWriter() = default;
    virtual bool writeString(int code, std::string text) = 0;
    bool writeUtf8String(int code, std::string text);
    bool writeUtf8Caps(int code, std::string text);
    std::string fromUtf8String(std::string t);
    bool writeUtf8Chunks(std::string_view text, int continuationCode,
                         int finalCode, std::size_t maxBytes);
    void setTextWritePolicy(DRW::DxfTextWritePolicy policy) noexcept { m_textPolicy = policy; }
    bool requiresLosslessText() const noexcept { return m_textPolicy == DRW::DxfTextWritePolicy::RequireLossless; }
    TextFailure textFailure() const noexcept { return m_textFailure; }
    // Raw text is never decoded/quoted here. Modern source-bound UTF8 may be
    // validated; other high-byte opaque strings need missing source provenance.
    bool preflightOpaqueText(std::string_view text, bool knownUtf8 = false);
    virtual bool writeInt16(int code, int data) = 0;
    virtual bool writeInt32(int code, int data) = 0;
    virtual bool writeInt64(int code, std::int64_t data) = 0;
    virtual bool writeDouble(int code, double data) = 0;
    virtual bool writeBool(int code, bool data) = 0;
    void setVersion(const std::string &v, bool dxfFormat){encoder.setVersion(v, dxfFormat);}
    void setCodePage(const std::string &c){encoder.setCodePage(c, true);}
    std::string getCodePage(){return encoder.getCodePage();}
    [[nodiscard]] bool hasWriteError() const { return writeError || m_textFailure != TextFailure::NoFailure; }
    /// Underlying output stream, used by dxfRW to record the $HANDSEED value
    /// offset and back-patch it with the final handle high-water mark.
    std::ostream *stream() { return filestr; }
    void setStream(std::ostream *stream) { filestr = stream; }
    void restoreWriteError(bool value) { writeError = value; }
    void markWriteError() { writeError = true; }
protected:
    bool recordWriteResult(bool result) {
        writeError = writeError || !result;
        return result;
    }
    std::ostream *filestr;
private:
    bool encodeStrictText(std::string_view text, std::size_t maxBytes,
                          std::vector<std::string>& chunks);
    bool failText(TextFailure failure) noexcept {
        if (m_textFailure == TextFailure::NoFailure) m_textFailure = failure;
        writeError = true;
        return false;
    }
    DRW_TextCodec encoder;
    bool writeError = false;
    bool m_binaryStrings;
    DRW::DxfTextWritePolicy m_textPolicy{DRW::DxfTextWritePolicy::Compatible};
    TextFailure m_textFailure{TextFailure::NoFailure};
};

/** Stages a DXF byte range and publishes it only after the caller succeeds. */
class DxfWriterRecordScope final {
public:
    explicit DxfWriterRecordScope(dxfWriter& writer)
        : m_writer(writer), m_parent(writer.stream()),
          m_previousError(writer.hasWriteError()) {
        if (m_parent != nullptr) {
            m_buffer.copyfmt(*m_parent);
            m_buffer.clear();
        }
        m_writer.setStream(&m_buffer);
        m_writer.restoreWriteError(false);
    }

    DxfWriterRecordScope(const DxfWriterRecordScope&) = delete;
    DxfWriterRecordScope& operator=(const DxfWriterRecordScope&) = delete;

    ~DxfWriterRecordScope() {
        if (m_state != State::Committed)
            abort();
    }

    bool commit() {
        if (m_state != State::Open || m_parent == nullptr
            || m_writer.hasWriteError() || !m_buffer.good()) {
            abort();
            return false;
        }
        const std::string bytes = m_buffer.str();
        m_writer.setStream(m_parent);
        if (!bytes.empty())
            m_parent->write(bytes.data(),
                            static_cast<std::streamsize>(bytes.size()));
        if (!m_parent->good()) {
            // A generic ostream cannot retract bytes accepted before a short
            // write. The outer scope must discard the staged phase.
            m_writer.markWriteError();
            m_state = State::AppendFailed;
            return false;
        }
        m_writer.restoreWriteError(m_previousError);
        m_state = State::Committed;
        return true;
    }

    void abort() {
        if (m_state == State::Committed || m_state == State::Aborted)
            return;
        m_writer.setStream(m_parent);
        if (m_state == State::AppendFailed)
            m_writer.markWriteError();
        else
            m_writer.restoreWriteError(m_previousError);
        m_state = State::Aborted;
    }

private:
    enum class State {
        Open,
        Committed,
        Aborted,
        AppendFailed
    };

    dxfWriter& m_writer;
    std::ostream *m_parent;
    std::ostringstream m_buffer;
    bool m_previousError;
    State m_state {State::Open};
};

class dxfWriterBinary : public dxfWriter {
public:
    explicit dxfWriterBinary(std::ostream *stream):dxfWriter(stream, true){}
    virtual ~dxfWriterBinary() = default;
    virtual bool writeString(int code, std::string text);
    virtual bool writeInt16(int code, int data);
    virtual bool writeInt32(int code, int data);
    virtual bool writeInt64(int code, std::int64_t data);
    virtual bool writeDouble(int code, double data);
    virtual bool writeBool(int code, bool data);
protected:
    virtual bool writeCode(int code);
};

class dxfWriterBinaryR12 final : public dxfWriterBinary {
public:
    explicit dxfWriterBinaryR12(std::ostream *stream):dxfWriterBinary(stream){}
private:
    bool writeCode(int code) override;
};

class dxfWriterAscii : public dxfWriter {
public:
    explicit dxfWriterAscii(std::ostream *stream);
    virtual ~dxfWriterAscii() = default;
    virtual bool writeString(int code, std::string text);
    virtual bool writeInt16(int code, int data);
    virtual bool writeInt32(int code, int data);
    virtual bool writeInt64(int code, std::int64_t data);
    virtual bool writeDouble(int code, double data);
    virtual bool writeBool(int code, bool data);
};

#endif // DXFWRITER_H
