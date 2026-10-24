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
#include <cstddef>
#include <ostream>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "dxfcode.h"
#include "drw_textcodec.h"

class dxfWriter {
public:
    enum class TextFailure { NoFailure, InvalidUtf8, UnsafeRecordText, UnsupportedProfile, TargetLoss, OpaqueText };
    struct RecordInventoryCheckpoint {
        std::size_t recordCount {0};
        bool recordHandlePending {false};
        bool recordOwnerPending {false};
        std::uint32_t recordApplicationDepth {0};
        std::size_t currentRecordIndex {0};
        std::uint32_t currentRecordOwnerHandle {0};
        bool currentRecordHasOwnerHandle {false};
        bool sectionNamePending {false};
        std::uint32_t currentRecordName {0};
        std::uint32_t currentSectionName {0};
        bool active {false};
    };
    struct EmittedRecordIdentity {
        std::uint32_t handle {0};
        std::uint32_t recordName {0};
        std::uint32_t sectionName {0};
        std::uint32_t ownerHandle {0};
        bool hasOwnerHandle {false};
    };
    explicit dxfWriter(std::ostream *stream, bool binary = false):filestr(stream), m_binaryStrings(binary) {}
    virtual ~dxfWriter() = default;
    void setClassifierProfile(DxfClassifierProfile profile) noexcept {
        m_classifierProfile = profile;
    }
    DxfClassifierProfile classifierProfile() const noexcept {
        return m_classifierProfile;
    }
    // Strings are consumed synchronously and are never retained by a writer.
    virtual bool writeString(int code, std::string_view text) = 0;
    // Opaque string replay is separate so callers must first validate raw
    // group provenance/policy. Textual writers may use the ordinary path.
    virtual bool writeRawString(int code, std::string_view text) {
        return writeString(code, text);
    }
    bool writeUtf8String(int code, std::string_view text);
    bool writeUtf8Caps(int code, const std::string& text);
    std::string fromUtf8String(std::string_view text);
    bool writeUtf8Chunks(std::string_view text, int continuationCode,
                         int finalCode, std::size_t maxBytes);
    // GEODATA definitions use the DXF caret-J spelling for line breaks. This
    // field-specific route preserves those already-normalized escape pairs
    // while encoding all other UTF-8 text normally.
    bool writeUtf8StringPreservingCaretJ(int code, std::string_view text);
    bool writeUtf8ChunksPreservingCaretJ(std::string_view text,
                                         int continuationCode,
                                         int finalCode,
                                         std::size_t maxBytes);
    bool writeUtf8MTextChunks(std::string_view text, int continuationCode,
                              int finalCode, std::size_t maxBytes,
                              std::size_t maxCharacters);
    void setTextWritePolicy(DRW::DxfTextWritePolicy policy) noexcept { m_textPolicy = policy; }
    bool requiresLosslessText() const noexcept { return m_textPolicy == DRW::DxfTextWritePolicy::RequireLossless; }
    bool usesCompatibleModernDxfUtf8() const noexcept {
        return !requiresLosslessText() && encoder.isDxfFormat()
            && encoder.getSourceVersion() >= DRW::AC1021;
    }
    TextFailure textFailure() const noexcept { return m_textFailure; }
    // Raw text is never decoded/quoted here. Modern source-bound UTF8 may be
    // validated; other high-byte opaque strings need missing source provenance.
    bool preflightOpaqueText(std::string_view text, bool knownUtf8 = false);
    virtual bool writeInt16(int code, int data) = 0;
    virtual bool writeInt32(int code, int data) = 0;
    virtual bool writeInt64(int code, std::int64_t data) = 0;
    virtual bool writeDouble(int code, double data) = 0;
    virtual bool writeBool(int code, bool data) = 0;
    void setRecordInventoryEnabled(bool enabled,
                                   std::size_t maxHandles) {
        m_recordInventoryEnabled = enabled;
        m_recordInventoryLimit = maxHandles;
        m_emittedRecords.clear();
        m_recordNames.clear();
        m_recordNameIds.clear();
        m_recordHandlePending = false;
        m_recordOwnerPending = false;
        m_recordApplicationDepth = 0;
        m_currentRecordIndex = 0;
        m_sectionNamePending = false;
        m_currentRecordName = 0;
        m_currentSectionName = 0;
        m_recordInventoryFailure = false;
        m_recordInventoryFinalized = false;
        if (enabled) {
            m_recordNames.push_back(std::string{});
            m_recordNameIds.emplace(std::string{}, 0u);
        }
    }
    const std::vector<EmittedRecordIdentity>& emittedRecords() const noexcept {
        return m_emittedRecords;
    }
    /** Resolve an interned name ID carried by emittedRecords(). The returned
     * pointer is writer-owned and remains valid until inventory reset/clear. */
    const std::string* recordInventoryName(std::uint32_t nameId) const noexcept {
        return nameId < m_recordNames.size() ? &m_recordNames[nameId] : nullptr;
    }
    bool finalizeRecordInventory() noexcept;
    std::size_t recordHandleCount(std::uint32_t handle) const noexcept;
    bool recordHandleHasIdentity(std::uint32_t handle,
                                 const std::string& recordName,
                                 const std::string& sectionName) const;
    bool recordHandleHasOwner(std::uint32_t handle,
                              std::uint32_t ownerHandle) const noexcept;
    std::vector<std::uint32_t> emittedRecordHandles() const;
    void clearRecordInventory() noexcept {
        m_emittedRecords.clear();
        m_recordNames.clear();
        m_recordNameIds.clear();
        m_recordHandlePending = false;
        m_recordOwnerPending = false;
        m_recordApplicationDepth = 0;
        m_currentRecordIndex = 0;
        m_sectionNamePending = false;
        m_currentRecordName = 0;
        m_currentSectionName = 0;
        m_recordInventoryFailure = false;
        m_recordInventoryFinalized = false;
    }
    RecordInventoryCheckpoint recordInventoryCheckpoint() const noexcept {
        const bool currentRecordExists =
            m_currentRecordIndex < m_emittedRecords.size();
        const EmittedRecordIdentity* currentRecord = currentRecordExists
            ? &m_emittedRecords[m_currentRecordIndex] : nullptr;
        return {m_emittedRecords.size(), m_recordHandlePending,
                m_recordOwnerPending, m_recordApplicationDepth, m_currentRecordIndex,
                currentRecord != nullptr ? currentRecord->ownerHandle : 0u,
                currentRecord != nullptr && currentRecord->hasOwnerHandle,
                m_sectionNamePending, m_currentRecordName,
                m_currentSectionName, m_recordInventoryEnabled};
    }
    void rollbackRecordInventory(
            const RecordInventoryCheckpoint& checkpoint) noexcept {
        if (!checkpoint.active || !m_recordInventoryEnabled)
            return;
        if (m_emittedRecords.size() > checkpoint.recordCount)
            m_emittedRecords.resize(checkpoint.recordCount);
        if (checkpoint.currentRecordIndex < m_emittedRecords.size()) {
            EmittedRecordIdentity& record =
                m_emittedRecords[checkpoint.currentRecordIndex];
            record.ownerHandle = checkpoint.currentRecordOwnerHandle;
            record.hasOwnerHandle = checkpoint.currentRecordHasOwnerHandle;
        }
        m_recordHandlePending = checkpoint.recordHandlePending;
        m_recordOwnerPending = checkpoint.recordOwnerPending;
        m_recordApplicationDepth = checkpoint.recordApplicationDepth;
        m_currentRecordIndex = checkpoint.currentRecordIndex;
        m_sectionNamePending = checkpoint.sectionNamePending;
        m_currentRecordName = checkpoint.currentRecordName;
        m_currentSectionName = checkpoint.currentSectionName;
    }
    void setVersion(const std::string &v, bool dxfFormat){encoder.setVersion(v, dxfFormat);}
    void setCodePage(const std::string &c){encoder.setCodePage(c, true);}
    std::string getCodePage(){return encoder.getCodePage();}
    [[nodiscard]] bool hasWriteError() const {
        return writeError || m_textFailure != TextFailure::NoFailure
            || m_recordInventoryFailure;
    }
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
    bool valueKindMatches(int code, DxfValueKind kind) const noexcept {
        return dxfValueKindForCode(code, m_classifierProfile) == kind;
    }
    bool observeRecordString(int code, std::string_view text);
    std::ostream *filestr;
private:
    struct EncodedTextChunk {
        std::size_t offset;
        std::size_t size;
    };
    bool internRecordName(std::string_view text, std::uint32_t& id);
    bool encodeStrictText(std::string_view text, std::size_t maxBytes,
                          std::string& encoded,
                          std::vector<EncodedTextChunk>& chunks,
                          std::size_t maxCharacters = 0u,
                          bool shortFinalChunk = false);
    bool encodeTextPreservingCaretJ(
        std::string_view text, std::size_t maxBytes, std::string& encoded,
        std::vector<EncodedTextChunk>& chunks);
    bool validateStrictText(std::string_view text);
    bool encodeStrictTextScalar(std::string_view text, std::size_t offset,
                                std::size_t& sourceBytes,
                                std::string& encoded);
    bool encodeStrictTextString(std::string_view text,
                                std::string& encoded);
    bool encodeCompatibleAsciiText(std::string_view text,
                                   std::string& encoded);
    bool failText(TextFailure failure) noexcept {
        if (m_textFailure == TextFailure::NoFailure) m_textFailure = failure;
        writeError = true;
        return false;
    }
    DRW_TextCodec encoder;
    bool writeError = false;
    bool m_binaryStrings;
    DxfClassifierProfile m_classifierProfile {
        DxfClassifierProfile::StandaloneSafe};
    DRW::DxfTextWritePolicy m_textPolicy{DRW::DxfTextWritePolicy::Compatible};
    TextFailure m_textFailure{TextFailure::NoFailure};
    bool m_recordInventoryEnabled {false};
    std::size_t m_recordInventoryLimit {4u * 1024u * 1024u};
    bool m_recordInventoryFailure {false};
    bool m_recordInventoryFinalized {false};
    bool m_recordHandlePending {false};
    bool m_recordOwnerPending {false};
    std::uint32_t m_recordApplicationDepth {0};
    std::size_t m_currentRecordIndex {0};
    bool m_sectionNamePending {false};
    std::uint32_t m_currentRecordName {0};
    std::uint32_t m_currentSectionName {0};
    std::vector<EmittedRecordIdentity> m_emittedRecords;
    std::vector<std::string> m_recordNames;
    std::unordered_map<std::string, std::uint32_t> m_recordNameIds;
};

/** Stages a DXF byte range and publishes it only after the caller succeeds. */
class DxfWriterRecordScope final {
public:
    explicit DxfWriterRecordScope(dxfWriter& writer)
        : m_writer(writer), m_parent(writer.stream()),
          m_previousError(writer.hasWriteError()),
          m_inventoryCheckpoint(writer.recordInventoryCheckpoint()) {
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
        return commit([](std::string_view) { return true; });
    }

    // Inspect the exact materialized bytes before appending them. The view
    // expires on return; inspection must not mutate the writer or its stream.
    template <class Inspector> bool commit(Inspector&& inspect) {
        if (m_state != State::Open || m_parent == nullptr
            || m_writer.hasWriteError() || !m_buffer.good()) {
            abort();
            return false;
        }
        const std::string bytes = m_buffer.str();
        if (!inspect(std::string_view(bytes)) || m_writer.hasWriteError()) {
            abort();
            return false;
        }
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
        m_writer.rollbackRecordInventory(m_inventoryCheckpoint);
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
    dxfWriter::RecordInventoryCheckpoint m_inventoryCheckpoint;
    State m_state {State::Open};
};

class dxfWriterBinary : public dxfWriter {
public:
    explicit dxfWriterBinary(std::ostream *stream):dxfWriter(stream, true){}
    virtual ~dxfWriterBinary() = default;
    bool writeString(int code, std::string_view text) override;
    bool writeRawString(int code, std::string_view text) override;
    bool writeInt16(int code, int data) override;
    bool writeInt32(int code, int data) override;
    bool writeInt64(int code, std::int64_t data) override;
    bool writeDouble(int code, double data) override;
    bool writeBool(int code, bool data) override;
protected:
    virtual bool writeCode(int code);
private:
    bool writeStringImpl(int code, std::string_view text);
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
    bool writeString(int code, std::string_view text) override;
    bool writeRawString(int code, std::string_view text) override;
    bool writeInt16(int code, int data) override;
    bool writeInt32(int code, int data) override;
    bool writeInt64(int code, std::int64_t data) override;
    bool writeDouble(int code, double data) override;
    bool writeBool(int code, bool data) override;
private:
    bool writeStringImpl(int code, std::string_view text,
                         bool allowUnknownRawString);
};

#endif // DXFWRITER_H
