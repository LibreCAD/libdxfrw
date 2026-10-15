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

#ifndef DXFREADER_H
#define DXFREADER_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <string>
#include <unordered_set>
#include <utility>

#include "dxfcode.h"
#include "dxfparserlimits.h"
#include "drw_textcodec.h"

// Shared by one public DXF read operation so independently retained buffers
// can use one aggregate logical-byte ceiling. This deliberately measures
// logical lengths, not std::string/vector capacity or process RSS.
class DxfReadRetainedBudget final {
public:
    explicit DxfReadRetainedBudget(std::size_t limit) noexcept
        : m_limit(limit) {}

    bool reserve(std::size_t bytes) noexcept {
        if (m_exceeded)
            return false;
        if (m_used > m_limit || bytes > m_limit - m_used) {
            m_exceeded = true;
            return false;
        }
        m_used += bytes;
        return true;
    }

    void release(std::size_t bytes) noexcept {
        m_used = bytes <= m_used ? m_used - bytes : 0u;
    }

    void reject() noexcept { m_exceeded = true; }

    std::size_t limit() const noexcept { return m_limit; }
    std::size_t used() const noexcept { return m_used; }
    bool exceeded() const noexcept { return m_exceeded; }

private:
    std::size_t m_limit;
    std::size_t m_used {0};
    bool m_exceeded {false};
};

// Reservation lifetime is lexical so exceptions during string/stream
// construction release the logical charge automatically.
class DxfReadRetainedReservation final {
public:
    DxfReadRetainedReservation(
        std::shared_ptr<DxfReadRetainedBudget> budget,
        std::size_t bytes) noexcept
        : m_budget(std::move(budget)), m_bytes(bytes),
          m_reserved(m_budget != nullptr && m_budget->reserve(bytes)) {}

    ~DxfReadRetainedReservation() noexcept {
        if (m_reserved)
            m_budget->release(m_bytes);
    }

    DxfReadRetainedReservation(const DxfReadRetainedReservation&) = delete;
    DxfReadRetainedReservation& operator=(
        const DxfReadRetainedReservation&) = delete;
    DxfReadRetainedReservation(DxfReadRetainedReservation&&) = delete;
    DxfReadRetainedReservation& operator=(
        DxfReadRetainedReservation&&) = delete;

    bool reserved() const noexcept { return m_reserved; }

    // Adjust or transfer a live charge when an owned representation changes
    // form without changing its lifetime owner.
    bool resize(std::size_t bytes) noexcept {
        if (m_budget == nullptr) {
            m_bytes = bytes;
            return true;
        }
        if (!m_reserved) {
            if (!m_budget->reserve(bytes))
                return false;
            m_bytes = bytes;
            m_reserved = true;
            return true;
        }
        if (bytes > m_bytes) {
            if (!m_budget->reserve(bytes - m_bytes))
                return false;
        } else if (bytes < m_bytes) {
            m_budget->release(m_bytes - bytes);
        }
        m_bytes = bytes;
        return true;
    }

    std::size_t transfer() noexcept {
        m_reserved = false;
        return m_bytes;
    }

    const std::shared_ptr<DxfReadRetainedBudget>& budget() const noexcept {
        return m_budget;
    }

private:
    std::shared_ptr<DxfReadRetainedBudget> m_budget;
    std::size_t m_bytes;
    bool m_reserved;
};

// Logical byte owner for strings retained by one parsed value/carrier. The
// owner is separate from reader-record charges so moves/callback publication
// can keep the charge alive beyond the source token's invalidation.
class DxfReadRetainedLease final {
public:
    explicit DxfReadRetainedLease(
        std::shared_ptr<DxfReadRetainedBudget> retainedBudget) noexcept
        : m_budget(std::move(retainedBudget)) {}

    ~DxfReadRetainedLease() noexcept {
        if (m_budget != nullptr && m_bytes != 0u)
            m_budget->release(m_bytes);
    }

    bool bind(std::shared_ptr<DxfReadRetainedBudget> nextBudget,
              std::size_t currentSize) noexcept {
        if (nextBudget == m_budget)
            return resizeTo(currentSize);
        if (nextBudget != nullptr && !nextBudget->reserve(currentSize))
            return false;
        if (m_budget != nullptr && m_bytes != 0u)
            m_budget->release(m_bytes);
        m_budget = std::move(nextBudget);
        m_bytes = m_budget != nullptr ? currentSize : 0u;
        return true;
    }

    bool resizeTo(std::size_t nextSize) noexcept {
        if (m_budget == nullptr) {
            m_bytes = 0u;
            return true;
        }
        if (nextSize > m_bytes) {
            if (!m_budget->reserve(nextSize - m_bytes))
                return false;
        } else if (nextSize < m_bytes) {
            m_budget->release(m_bytes - nextSize);
        }
        m_bytes = nextSize;
        return true;
    }

    bool add(std::size_t bytes) noexcept {
        const std::size_t maxSize =
            (std::numeric_limits<std::size_t>::max)();
        if (bytes > maxSize - m_bytes) {
            if (m_budget != nullptr)
                m_budget->reject();
            return false;
        }
        return resizeTo(m_bytes + bytes);
    }

    bool adopt(DxfReadRetainedReservation& reservation) noexcept {
        if (m_budget == nullptr || reservation.budget() != m_budget
            || !reservation.reserved())
            return false;
        const std::size_t nextSize = reservation.transfer();
        if (m_bytes != 0u)
            m_budget->release(m_bytes);
        m_bytes = nextSize;
        return true;
    }

    std::size_t bytes() const noexcept { return m_bytes; }
    const std::shared_ptr<DxfReadRetainedBudget>& budget() const noexcept {
        return m_budget;
    }

private:
    std::shared_ptr<DxfReadRetainedBudget> m_budget;
    std::size_t m_bytes {0u};
};

inline bool retainDxfReadBytes(
    std::shared_ptr<DxfReadRetainedLease>& lease,
    const std::shared_ptr<DxfReadRetainedBudget>& budget,
    std::size_t bytes) {
    if (budget == nullptr || bytes == 0u)
        return true;
    if (lease == nullptr) {
        lease = std::make_shared<DxfReadRetainedLease>(budget);
    } else if (lease->budget() != budget
               && !lease->bind(budget, lease->bytes())) {
        return false;
    }
    return lease->add(bytes);
}

// A parse helper may build a temporary list of retained strings before it
// publishes the complete application group. Roll the owner back if parsing or
// allocation fails so a caller that reuses the model does not inherit a
// partial charge.
class DxfReadRetainedLeaseCheckpoint final {
public:
    explicit DxfReadRetainedLeaseCheckpoint(
        std::shared_ptr<DxfReadRetainedLease>& lease) noexcept
        : m_target(lease), m_original(lease),
          m_originalBudget(lease != nullptr ? lease->budget()
                                            : std::shared_ptr<DxfReadRetainedBudget>{}),
          m_originalBytes(lease != nullptr ? lease->bytes() : 0u) {}

    ~DxfReadRetainedLeaseCheckpoint() noexcept {
        if (m_committed)
            return;
        if (m_original == nullptr) {
            m_target.reset();
            return;
        }
        m_target = m_original;
        if (m_original->budget() == m_originalBudget)
            (void)m_original->resizeTo(m_originalBytes);
        else
            (void)m_original->bind(m_originalBudget, m_originalBytes);
    }

    DxfReadRetainedLeaseCheckpoint(const DxfReadRetainedLeaseCheckpoint&) = delete;
    DxfReadRetainedLeaseCheckpoint& operator=(
        const DxfReadRetainedLeaseCheckpoint&) = delete;

    void commit() noexcept { m_committed = true; }

private:
    std::shared_ptr<DxfReadRetainedLease>& m_target;
    std::shared_ptr<DxfReadRetainedLease> m_original;
    std::shared_ptr<DxfReadRetainedBudget> m_originalBudget;
    std::size_t m_originalBytes;
    bool m_committed {false};
};

class dxfReader {
public:
    enum TYPE {
        STRING,
        INT32,
        INT64,
        DOUBLE,
        BOOL,
        BINARY,
        INVALID
    };
    enum TYPE type;
public:
    dxfReader(std::istream *stream){
        filestr = stream;
        type = INVALID;
    }
    virtual ~dxfReader() { releaseCurrentRecordRetainedBytes(); }
    bool readRec(int *code);

    /// Bound the aggregate number of physical records consumed by this
    /// reader. The budget is reset by constructing a reader for each public
    /// operation; zero rejects the first record. This is a resource ceiling,
    /// not a DXF semantic limit.
    void setRecordBudget(std::size_t budget) noexcept {
        m_recordBudget = budget;
    }
    std::size_t recordCount() const noexcept { return m_recordCount; }
    bool recordBudgetExceeded() const noexcept {
        return m_recordBudgetExceeded;
    }
    void setTokenByteBudget(std::size_t budget) noexcept {
        m_tokenByteBudget = budget;
    }
    bool tokenByteBudgetExceeded() const noexcept {
        return m_tokenByteBudgetExceeded;
    }
    /// Set a cumulative logical-input ceiling for this read session. The
    /// initial count accounts bytes consumed by bounded format detection (the
    /// binary DXF sentinel); re-reading those bytes through a reopened stream
    /// is not charged a second time.
    void setInputByteBudget(std::size_t budget,
                            std::size_t initiallyConsumed = 0) noexcept {
        m_inputByteBudget = budget;
        m_inputByteCount = initiallyConsumed;
        m_inputByteBudgetExceeded = initiallyConsumed > budget;
    }
    std::size_t inputByteCount() const noexcept { return m_inputByteCount; }
    std::size_t inputByteBudget() const noexcept { return m_inputByteBudget; }
    std::size_t inputByteBudgetRemaining() const noexcept {
        return m_inputByteBudgetExceeded || m_inputByteCount >= m_inputByteBudget
            ? 0 : m_inputByteBudget - m_inputByteCount;
    }
    bool inputByteBudgetExceeded() const noexcept {
        return m_inputByteBudgetExceeded;
    }
    void setDecodeWorkBudget(std::size_t budget) noexcept {
        m_decodeWorkBudget = budget;
    }
    // Charge deterministic source-byte visits made by instrumented decoders.
    // Unlike input bytes this counter is cumulative: revisiting a prefix is a
    // new unit of work even though no source bytes are acquired again.
    bool chargeDecodeWork(std::size_t bytes) noexcept {
        if (m_decodeWorkBudgetExceeded)
            return false;
        if (m_decodeWorkCount > m_decodeWorkBudget
            || bytes > m_decodeWorkBudget - m_decodeWorkCount) {
            m_decodeWorkBudgetExceeded = true;
            return false;
        }
        m_decodeWorkCount += bytes;
        return true;
    }
    std::size_t decodeWorkCount() const noexcept { return m_decodeWorkCount; }
    bool decodeWorkBudgetExceeded() const noexcept {
        return m_decodeWorkBudgetExceeded;
    }
    void setRetainedByteBudget(
        std::shared_ptr<DxfReadRetainedBudget> budget) noexcept {
        releaseCurrentRecordRetainedBytes();
        m_retainedByteBudget = std::move(budget);
    }
    const std::shared_ptr<DxfReadRetainedBudget>& retainedByteBudget() const noexcept {
        return m_retainedByteBudget;
    }
    // Retain the logical length of a small value that must survive alongside
    // this record's reader buffers. The charge ends when readRec invalidates
    // this record or the reader is destroyed.
    bool retainCurrentRecordBytes(std::size_t bytes) noexcept {
        if (bytes == 0 || m_retainedByteBudget == nullptr)
            return true;
        if (!m_retainedByteBudget->reserve(bytes))
            return false;
        m_currentRecordRetainedBytes += bytes;
        return true;
    }
    void releaseCurrentRecordBytes(std::size_t bytes) noexcept {
        const std::size_t released =
            bytes < m_currentRecordRetainedBytes
                ? bytes : m_currentRecordRetainedBytes;
        if (released != 0 && m_retainedByteBudget != nullptr)
            m_retainedByteBudget->release(released);
        m_currentRecordRetainedBytes -= released;
    }
    // Reserve one worst-case returned string representation before parsers
    // can call getString()/getUtf8String(). A single-byte table emits at most
    // one four-byte UTF-8 scalar per input byte; DBCS emits at most two such
    // scalars per two input bytes (including its two-scalar Big5-HKSCS cells),
    // while UTF-16 and identity paths have lower ratios. Binary getString()
    // can duplicate the canonical hex spelling once. This is a logical length
    // bound; transient codec scratch/capacity and caller-owned callback copies
    // are not counted.
    bool reserveCurrentRecordTextOutput() noexcept {
        if (m_retainedByteBudget == nullptr
            || (type != STRING && type != BINARY))
            return true;
        const std::size_t maxSize =
            (std::numeric_limits<std::size_t>::max)();
        const std::size_t multiplier = type == STRING ? 4u : 1u;
        if (strData.size() > maxSize / multiplier) {
            m_retainedByteBudget->reject();
            return false;
        }
        return retainCurrentRecordBytes(strData.size() * multiplier);
    }
    // A derived parser may consume a non-XDATA field without calling its
    // base. Coordinate assembly must still stop at that physical boundary.
    bool followsXdataPointComponent() const noexcept {
        return (m_previousCode >= 1010 && m_previousCode <= 1013)
            || (m_previousCode >= 1020 && m_previousCode <= 1023)
            || (m_previousCode >= 1030 && m_previousCode <= 1033);
    }

    // Semantic EED/XDATA text values (1000/1001/1003) use the drawing's
    // version-aware source codec just like ordinary text strings. Keep code
    // 1002 control strings, 1004 binary chunks
    // and code 1005 handles in their canonical raw spelling. The current
    // group code is latched by readRec(), so every typed parseCode path gets
    // the same conversion without duplicating it in each entity/table.
    std::string getString() {
        // Group 430 is the common-entity color-book name. It is a semantic
        // model string, unlike the raw handles/application markers handled by
        // the other getString() callers. Decode it only while a typed entity
        // parser has enabled semantic mode; raw DXF carriers must retain the
        // source spelling for lossless replay.
        if ((m_decodeSemanticStrings && m_currentCode == 430)
            || m_currentCode == 1000 || m_currentCode == 1001
            || m_currentCode == 1003)
            return decoder.toUtf8(strData);
        return strData;
    }
    const std::string& getRawValue() const { return rawData; }
    // Convert a validated hexadecimal handle string representable by the
    // legacy 32-bit object model. Typed records must use this form.
    std::uint32_t getHandleString();
    bool isValidHandleString() const;
    // Raw DXF carriers may retain a syntactically valid DWG-width handle
    // lexeme (one to sixteen hexadecimal digits) without narrowing it.
    bool isValidHandleLexeme() const;
    // Admit one code-5 self handle for this read session. References use
    // other group codes and are intentionally not registered here.
    bool registerSelfHandle();
    // When readRec() rejects an otherwise decoded handle/reference lexeme,
    // expose the group code so a façade can retain the legacy stage result
    // while adding structured context to its operation diagnostic.
    int lastInvalidHandleCode() const { return m_lastInvalidHandleCode; }
    void setAllowWideHandleLexemes(bool allow) {
        m_allowWideHandleLexemes = allow;
    }
    bool allowsWideHandleLexemes() const {
        return m_allowWideHandleLexemes;
    }
    // Some legacy DIMSTYLE records use 340 for a text-style name and leave
    // subsequent handle slots empty; accept that scoped spelling.
    void setAllowDimstyleNames(bool allow) { m_allowDimstyleNames = allow; }
    std::string toUtf8String(std::string t) {return decoder.toUtf8(t);}
    std::string getUtf8String() {return decoder.toUtf8(strData);}
    double getDouble() {return doubleData;}
    int getInt32() {return intData;}
    std::int64_t getInt64() {return int64;}
    bool getBool() { return (intData==0) ? false : true;}
    int getVersion(){return decoder.getVersion();}
    void setVersion(const std::string &v, bool dxfFormat){decoder.setVersion(v, dxfFormat);}
    void setCodePage(const std::string &c){decoder.setCodePage(c, true);}
    void setDxfTextEncoding(DRW::DxfTextEncoding encoding) noexcept {
        decoder.setDxfTextEncoding(encoding);
    }
    std::string getCodePage(){ return decoder.getCodePage();}
    DRW::Version getSourceVersion() const { return decoder.getSourceVersion(); }
    bool hasSourceVersion() const { return decoder.hasSourceVersion(); }
    void setSemanticStringMode(bool enabled) noexcept {
        m_decodeSemanticStrings = enabled;
    }
    void setIgnoreComments(const bool bValue) {m_bIgnoreComments = bValue;}
    /// Select an explicit classifier profile for compatibility probes. The
    /// standalone-safe profile is the default; production callers must not
    /// silently opt into the pinned LibreCAD legacy widths.
    void setClassifierProfile(DxfClassifierProfile profile) {
        m_classifierProfile = profile;
    }
    DxfClassifierProfile classifierProfile() const {
        return m_classifierProfile;
    }

protected:
    // Only accepted tokens retain text. Failure/throw clears it, rather than
    // exposing getline's partial failed prefix. Zero-byte returned I/O/EOF
    // failure adds failbit; an original badbit-mask exception is rethrown first.
    bool readDelimitedToken(std::string& text, char delimiter,
                            bool allowFinalAsciiValue,
                            bool retainText = true);
    bool canConsumeInputBytes(std::size_t count) noexcept;
    void accountInputBytes(std::size_t count) noexcept;
    bool readInputBytes(char* destination, std::size_t count);
    bool copyRawValue(const std::string& value) {
        if (!retainCurrentRecordBytes(value.size()))
            return false;
        try {
            rawData = value;
        } catch (...) {
            releaseCurrentRecordBytes(value.size());
            throw;
        }
        return true;
    }
    virtual bool readCode(int *code) = 0; //return true if successful (not EOF)
    virtual bool readString(std::string *text) = 0;
    virtual bool readString() = 0;
    virtual bool readBinary() = 0;
    virtual bool readInt16() = 0;
    virtual bool readInt32() = 0;
    virtual bool readInt64() = 0;
    virtual bool readDouble() = 0;
    virtual bool readBool() = 0;

protected:
    std::istream *filestr;
    std::string strData;
    // Source value spelling for ASCII records; binary readers leave this as
    // the decoded representation and callers must use typed values instead.
    std::string rawData;
    double doubleData = 0.0;
    signed int intData = 0; //32 bits integer
    std::int64_t int64 = 0; // signed 64-bit integer (DXF codes 160-169)
    bool skip = false; //set to true for ascii dxf, false for binary
private:
    DRW_TextCodec decoder;
    bool m_bIgnoreComments {false};
    DxfClassifierProfile m_classifierProfile {
        DxfClassifierProfile::StandaloneSafe};
    bool m_allowWideHandleLexemes {false};
    bool m_allowDimstyleNames {false};
    bool m_decodeSemanticStrings {false};
    std::unordered_set<std::uint64_t> m_selfHandles;
    std::uint64_t m_currentSelfHandle {0};
    bool m_currentSelfHandleRegistered {false};
    int m_lastInvalidHandleCode {0};
    std::size_t m_recordBudget {DRW::kDefaultDxfReadRecordBudget};
    std::size_t m_recordCount {0};
    bool m_recordBudgetExceeded {false};
    std::size_t m_tokenByteBudget {DRW::kDefaultDxfReadTokenByteBudget};
    bool m_tokenByteBudgetExceeded {false};
    std::size_t m_inputByteBudget {DRW::kDefaultDxfReadInputByteBudget};
    std::size_t m_inputByteCount {0};
    bool m_inputByteBudgetExceeded {false};
    std::size_t m_decodeWorkBudget {
        DRW::kDefaultDxfReadDecodeWorkBudget};
    std::size_t m_decodeWorkCount {0};
    bool m_decodeWorkBudgetExceeded {false};
    std::shared_ptr<DxfReadRetainedBudget> m_retainedByteBudget;
    std::size_t m_currentRecordRetainedBytes {0};
    int m_currentCode {0};
    int m_previousCode {0};

    void releaseCurrentRecordRetainedBytes() noexcept {
        releaseCurrentRecordBytes(m_currentRecordRetainedBytes);
    }
};

// Replace one parsed string field while charging its current logical value.
// The record's getter-output reservation covers the by-value decode result;
// this function separately admits the destination before assignment, then
// releases the old field charge only after replacement succeeds.
inline bool replaceDxfReadRetainedString(
    const std::unique_ptr<dxfReader>& reader,
    std::shared_ptr<DxfReadRetainedLease>& lease,
    std::size_t& ownerBytes, std::string& destination, std::string value) {
    if (reader == nullptr)
        return false;
    DxfReadRetainedLeaseCheckpoint checkpoint(lease);
    const auto budget = reader->retainedByteBudget();
    if (!retainDxfReadBytes(lease, budget, value.size()))
        return false;
    destination = std::move(value);
    if (ownerBytes != 0u && lease != nullptr) {
        if (ownerBytes > lease->bytes()) {
            if (budget != nullptr)
                budget->reject();
            return false;
        }
        if (!lease->resizeTo(lease->bytes() - ownerBytes))
            return false;
    }
    ownerBytes = budget != nullptr ? destination.size() : 0u;
    checkpoint.commit();
    return true;
}

class dxfReaderBinary : public dxfReader {
public:
    dxfReaderBinary(std::istream *stream):dxfReader(stream){skip = false; }
    virtual ~dxfReaderBinary() = default;
    virtual bool readCode(int *code);
    virtual bool readString(std::string *text);
    virtual bool readString();
    virtual bool readBinary();
    virtual bool readInt16();
    virtual bool readInt32();
    virtual bool readInt64();
    virtual bool readDouble();
    virtual bool readBool();
};

// Pre-R13 (R12/AC1009) binary DXF uses 1-byte group codes instead of the
// 2-byte little-endian codes of R13+. Only readCode differs; every value
// reader (string/double/int/...) is identical, so inherit them all.
class dxfReaderBinaryR12 : public dxfReaderBinary {
public:
    dxfReaderBinaryR12(std::istream *stream):dxfReaderBinary(stream){}
    virtual ~dxfReaderBinaryR12() = default;
    virtual bool readCode(int *code) override;
};

class dxfReaderAscii : public dxfReader {
public:
    dxfReaderAscii(std::istream *stream):dxfReader(stream){skip = true; }
    virtual ~dxfReaderAscii() = default;
    virtual bool readCode(int *code);
    virtual bool readString(std::string *text);
    virtual bool readString();
    virtual bool readBinary();
    virtual bool readInt16();
    virtual bool readDouble();
    virtual bool readInt32();
    virtual bool readInt64();
    virtual bool readBool();
};


/// Read one group code of a coordinate triplet - an origin plus an X and a Y
/// axis - into the three coordinates that carry it.  DXF spells this the same
/// way wherever a record stores such a frame: VPORT, VIEW and VIEWPORT use it
/// for their UCS, and MLEADER's context data for its content base point and
/// base direction/vertical.  The nine cases live here once instead of being
/// repeated per record.
///
/// Returns true when the code belonged to the triplet and was consumed.
inline bool readCoordTripletCode(int code, const std::unique_ptr<dxfReader>& reader,
                                 DRW_Coord& origin, DRW_Coord& xAxis,
                                 DRW_Coord& yAxis) {
    switch (code) {
    case 110: origin.x = reader->getDouble(); return true;
    case 120: origin.y = reader->getDouble(); return true;
    case 130: origin.z = reader->getDouble(); return true;
    case 111: xAxis.x = reader->getDouble(); return true;
    case 121: xAxis.y = reader->getDouble(); return true;
    case 131: xAxis.z = reader->getDouble(); return true;
    case 112: yAxis.x = reader->getDouble(); return true;
    case 122: yAxis.y = reader->getDouble(); return true;
    case 132: yAxis.z = reader->getDouble(); return true;
    default: return false;
    }
}

#endif // DXFREADER_H
