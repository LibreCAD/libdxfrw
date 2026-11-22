/* Internal opt-in observation of an already committed XRECORD frame.
 * This is source evidence only; it grants no output or semantic authority.
 */
#ifndef DWG_XRECORD_VERIFIED_SOURCE_H
#define DWG_XRECORD_VERIFIED_SOURCE_H

#include "drw_objects.h"
#include "dwgobjectframe.h"
#include <cstddef>

class dwgReader;
class DRW_Table;

class DwgVerifiedXRecordTicket final {
    friend class dwgReader;
    DwgVerifiedXRecordTicket(DRW::Version version, std::uint32_t handle,
            std::uint32_t offset, std::uint64_t ordinal,
            DRW_DwgFrameOffsetSpace space, const DwgObjectFrame& frame) noexcept
        : version_(version), handle_(handle), offset_(offset), ordinal_(ordinal),
          space_(space), frame_(frame) {}
public:
    DwgVerifiedXRecordTicket(const DwgVerifiedXRecordTicket&) = delete;
    DwgVerifiedXRecordTicket& operator=(const DwgVerifiedXRecordTicket&) = delete;
    DRW::Version version() const noexcept { return version_; }
    std::uint32_t handle() const noexcept { return handle_; }
    std::uint32_t offset() const noexcept { return offset_; }
    std::uint64_t ordinal() const noexcept { return ordinal_; }
    DRW_DwgFrameOffsetSpace space() const noexcept { return space_; }
    const DwgObjectFrame& frame() const noexcept { return frame_; }
private:
    DRW::Version version_;
    std::uint32_t handle_, offset_;
    std::uint64_t ordinal_;
    DRW_DwgFrameOffsetSpace space_;
    const DwgObjectFrame& frame_;
};

// Each delivery is provisional. A sink must publish an aggregate only after
// all selected tickets and the completed import match; a prefix is not proof.
class DwgVerifiedXRecordSink {
public:
    virtual ~DwgVerifiedXRecordSink() = default;
    virtual bool wantsVerifiedXRecord(std::uint32_t handle) const noexcept = 0;
    virtual void verifiedXRecord(const DwgVerifiedXRecordTicket&) noexcept = 0;
};

enum class DwgVerifiedTableCandidateKind : std::uint8_t { TableContent, DataTable, TableGeometry, ExtensionDictionary, TableEntity, TableStyle, StyleExtensionDictionary, CellStyleMap, LTypeControl, LTypeRecord, StyleControl, StyleRecord };

// As with XRECORD tickets, this is a provisional, reader-created CRC-checked
// frame observation, not an authenticated aggregate or an output permit.
class DwgVerifiedTableCandidateTicket final {
    friend class dwgReader;
    DwgVerifiedTableCandidateTicket(DwgVerifiedTableCandidateKind kind,
            DRW::Version version,std::uint16_t classNumber,std::uint32_t handle,
            std::uint32_t offset,std::uint64_t ordinal,
            DRW_DwgFrameOffsetSpace space,const DwgObjectFrame& frame,const DRW_Table* model,
            const DRW_LType* ltypeModel,const DRW_Textstyle* styleModel) noexcept
        :kind_(kind),version_(version),classNumber_(classNumber),handle_(handle),
         offset_(offset),ordinal_(ordinal),space_(space),frame_(frame),model_(model),ltypeModel_(ltypeModel),styleModel_(styleModel) {}
public:
    DwgVerifiedTableCandidateTicket(const DwgVerifiedTableCandidateTicket&)=delete;
    DwgVerifiedTableCandidateTicket& operator=(const DwgVerifiedTableCandidateTicket&)=delete;
    DwgVerifiedTableCandidateKind kind() const noexcept {return kind_;}
    DRW::Version version() const noexcept {return version_;}
    std::uint16_t classNumber() const noexcept {return classNumber_;}
    std::uint32_t handle() const noexcept {return handle_;}
    std::uint32_t offset() const noexcept {return offset_;}
    std::uint64_t ordinal() const noexcept {return ordinal_;}
    DRW_DwgFrameOffsetSpace space() const noexcept {return space_;}
    const DwgObjectFrame& frame() const noexcept {return frame_;}
    const DRW_Table* tableModel() const noexcept {return model_;}
    const DRW_LType* ltypeModel() const noexcept {return ltypeModel_;}
    const DRW_Textstyle* styleModel() const noexcept {return styleModel_;}
private:
    DwgVerifiedTableCandidateKind kind_;
    DRW::Version version_;
    std::uint16_t classNumber_;
    std::uint32_t handle_,offset_;
    std::uint64_t ordinal_;
    DRW_DwgFrameOffsetSpace space_;
    const DwgObjectFrame& frame_;
    const DRW_Table* model_;
    const DRW_LType* ltypeModel_;
    const DRW_Textstyle* styleModel_;
};

class DwgVerifiedTableCandidateSink {
public:
    virtual ~DwgVerifiedTableCandidateSink()=default;
    // Private opt-in. Zero means no post-BLOCKS entity reread.
    virtual std::uint32_t verifiedTableEntitySelection() const noexcept {return 0u;}
    virtual bool wantsVerifiedTableEntityModel() const noexcept {return false;}
    virtual std::uint32_t verifiedStyleExtensionDictionarySelection() const noexcept {return 0u;}
    virtual bool wantsVerifiedTableCandidate(std::uint32_t handle) const noexcept=0;
    virtual void verifiedTableCandidate(const DwgVerifiedTableCandidateTicket&) noexcept=0;
};

enum class DwgVerifiedTableCacheSourceKind : std::uint8_t {
    BlockRecord,
    BlockEntity
};

class DwgVerifiedTableCacheSourceTicket final {
    friend class dwgReader;
    DwgVerifiedTableCacheSourceTicket(
            DwgVerifiedTableCacheSourceKind kind, DRW::Version version,
            std::uint32_t handle, std::uint32_t offset, std::uint64_t ordinal,
            DRW_DwgFrameOffsetSpace space, const DwgObjectFrame& frame,
            const DRW_Block_Record* blockRecord,
            const DRW_Block* block) noexcept
        : kind_(kind), version_(version), handle_(handle), offset_(offset),
          ordinal_(ordinal), space_(space), frame_(frame),
          blockRecord_(blockRecord), block_(block) {}
public:
    DwgVerifiedTableCacheSourceTicket(
            const DwgVerifiedTableCacheSourceTicket&) = delete;
    DwgVerifiedTableCacheSourceTicket& operator=(
            const DwgVerifiedTableCacheSourceTicket&) = delete;
    DwgVerifiedTableCacheSourceKind kind() const noexcept { return kind_; }
    DRW::Version version() const noexcept { return version_; }
    std::uint32_t handle() const noexcept { return handle_; }
    std::uint32_t offset() const noexcept { return offset_; }
    std::uint64_t ordinal() const noexcept { return ordinal_; }
    DRW_DwgFrameOffsetSpace space() const noexcept { return space_; }
    const DwgObjectFrame& frame() const noexcept { return frame_; }
    const DRW_Block_Record* blockRecordModel() const noexcept {
        return blockRecord_;
    }
    const DRW_Block* blockModel() const noexcept { return block_; }
private:
    DwgVerifiedTableCacheSourceKind kind_;
    DRW::Version version_;
    std::uint32_t handle_, offset_;
    std::uint64_t ordinal_;
    DRW_DwgFrameOffsetSpace space_;
    const DwgObjectFrame& frame_;
    const DRW_Block_Record* blockRecord_;
    const DRW_Block* block_;
};

class DwgVerifiedTableCacheSourceSink {
public:
    virtual ~DwgVerifiedTableCacheSourceSink() = default;
    virtual std::uint32_t verifiedTableCacheBlockRecordSelection()
        const noexcept = 0;
    virtual std::uint32_t verifiedTableCacheBlockSelection()
        const noexcept = 0;
    virtual void verifiedTableCacheSource(
        const DwgVerifiedTableCacheSourceTicket&) noexcept = 0;
};

// Provisional, reader-authenticated R13/R14 BLOCK_HEADER frame. The sink must
// bind this to the completed import, table membership and live model before
// using it; delivery alone does not authorize output.
class DwgLegacyBlockRecordSourceTicket final {
    friend class dwgReader;
    DwgLegacyBlockRecordSourceTicket(DRW::Version version, std::uint32_t handle,
        std::uint32_t offset, std::uint64_t ordinal, DRW_DwgFrameOffsetSpace space,
        const DwgObjectFrame& frame) noexcept
        : version_(version), handle_(handle), offset_(offset), ordinal_(ordinal),
          space_(space), frame_(frame) {}
public:
    DwgLegacyBlockRecordSourceTicket(const DwgLegacyBlockRecordSourceTicket&) = delete;
    DwgLegacyBlockRecordSourceTicket& operator=(const DwgLegacyBlockRecordSourceTicket&) = delete;
    DRW::Version version() const noexcept { return version_; }
    std::uint32_t handle() const noexcept { return handle_; }
    std::uint32_t offset() const noexcept { return offset_; }
    std::uint64_t ordinal() const noexcept { return ordinal_; }
    DRW_DwgFrameOffsetSpace space() const noexcept { return space_; }
    const DwgObjectFrame& frame() const noexcept { return frame_; }
private:
    DRW::Version version_;
    std::uint32_t handle_, offset_;
    std::uint64_t ordinal_;
    DRW_DwgFrameOffsetSpace space_;
    const DwgObjectFrame& frame_;
};

class DwgLegacyBlockRecordSourceSink {
public:
    static constexpr std::size_t MaxFrameBytes = 64u * 1024u;
    virtual ~DwgLegacyBlockRecordSourceSink() = default;
    virtual bool wantsLegacyBlockRecordSource(DRW::Version) const noexcept = 0;
    virtual void legacyBlockRecordSource(const DwgLegacyBlockRecordSourceTicket&) noexcept = 0;
    virtual void legacyBlockRecordSourceFailed() noexcept = 0;
};
#endif
