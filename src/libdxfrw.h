/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2016-2021 A. Stebich (librecad@mail.lordofbikes.de)        **
**  Copyright (C) 2011-2015 José F. Soriano, rallazz@gmail.com               **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef LIBDXFRW_H
#define LIBDXFRW_H

// Preserve a caller's X11 None macro while declaring all header enums.
#if defined(None)
# if defined(_MSC_VER) || defined(__GNUC__) || defined(__clang__)
#  pragma push_macro("None")
#  undef None
#  define LIBDXFRW_LIBDXFRW_RESTORE_NONE
# else
#  error "libdxfrw requires push_macro/pop_macro support with a caller None macro"
# endif
#endif

#include <functional>
#include <ios>
#include <list>
#include <map>
#include <array>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "drw_entities.h"
#include "drw_objects.h"
#include "drw_classes.h"
#include "drw_header.h"
#include "drw_interface.h"


class dxfReader;
class DxfReadRetainedBudget;
class dxfWriter;
class DxfWriterRecordScope;
class HandleAllocator;

#if defined(_MSC_VER) && defined(LIBDXFRW_FORCE_TEST_SEAMS)
#define LIBDXFRW_TEST_SEAM_NOINLINE __declspec(dllexport) __declspec(noinline)
#else
#define LIBDXFRW_TEST_SEAM_NOINLINE
#endif

/** Holds per-read-session name-resolution tables populated during DXF/DWG parsing. */
class DRW_ParsingContext {
public:
    struct BlockRecordInfo {
        std::string name;
        int insUnits = 0;
        bool canExplode = true;
        std::uint8_t blockScaling = 0;
        std::uint32_t layoutHandle = DRW::NoHandle;
        std::vector<std::uint8_t> previewData;
        std::vector<std::uint32_t> insertHandles;
    };

    DRW_ParsingContext() = default;
    /** Returns line-type name for a given DXF handle, or empty string if not found. */
    std::string resolveLineTypeName(int handle) const {
        auto it = lineTypeNameMap.find(static_cast<std::uint32_t>(handle));
        return (it != lineTypeNameMap.end()) ? it->second : std::string();
    }
    /** Returns block-record name for a given DXF handle, or empty string if not found. */
    std::string resolveBlockRecordName(std::uint32_t handle) const {
        auto it = blockRecordMap.find(handle);
        return (it != blockRecordMap.end()) ? it->second.name : std::string();
    }
    /** Returns block-record insertion units for a given DXF handle, or 0 if unknown. */
    int resolveBlockRecordInsUnits(std::uint32_t handle) const {
        auto it = blockRecordMap.find(handle);
        return (it != blockRecordMap.end()) ? it->second.insUnits : 0;
    }
    /** Returns a copy of the BLOCK_RECORD preview image bytes, if present. */
    std::vector<std::uint8_t> resolveBlockRecordPreview(
        std::uint32_t handle) const {
        auto it = blockRecordMap.find(handle);
        return (it != blockRecordMap.end()) ? it->second.previewData
                                            : std::vector<std::uint8_t>{};
    }
    /** Returns the INSERT handles from a BLOCK_RECORD BLKREFS group. */
    std::vector<std::uint32_t> resolveBlockRecordInsertHandles(
        std::uint32_t handle) const {
        auto it = blockRecordMap.find(handle);
        return (it != blockRecordMap.end()) ? it->second.insertHandles
                                            : std::vector<std::uint32_t>{};
    }
    /** Returns the BLOCK_RECORD layout handle, or NoHandle if unknown. */
    std::uint32_t resolveBlockRecordLayoutHandle(std::uint32_t handle) const {
        auto it = blockRecordMap.find(handle);
        return (it != blockRecordMap.end()) ? it->second.layoutHandle
                                            : static_cast<std::uint32_t>(DRW::NoHandle);
    }
    std::unordered_map<std::uint32_t, std::string> lineTypeNameMap;
    std::unordered_map<std::uint32_t, BlockRecordInfo> blockRecordMap;
};

class dxfRW {
public:
    using ImageDefinitionKey = std::pair<std::uint64_t, std::uint32_t>;

    // One source IMAGEDEF identity and its full typed payload. The adapter
    // supplies these before write() so IMAGE entities can bind to definitions
    // independently of filenames and callback order.
    struct ImageDefinitionPlanEntry {
        std::uint64_t importGeneration {0};
        std::uint32_t sourceHandle {0};
        DRW_ImageDef definition;
        std::vector<DRW_ImageDefinitionReactor> selectedReactors{};
    };

    // The standalone-safe classifier is the default. LibreCAD's pinned
    // bundled fork uses legacy widths for the disputed 260-269 and 482-998
    // ranges; downstream adapters migrating from that fork may opt in
    // explicitly without changing existing callers' behavior.
    enum class DxfCompatibilityProfile : std::uint8_t {
        StandaloneSafe,
        LibreCadMasterLegacy
    };

    dxfRW(const char* name);
    dxfRW(const dxfRW&) = delete;
    dxfRW& operator=(const dxfRW&) = delete;
    dxfRW(dxfRW&&) = delete;
    dxfRW& operator=(dxfRW&&) = delete;
    ~dxfRW();
    /** Set process-global debugging. Constructing any DXF/DWG codec resets it
     * to None for compatibility; this is not a per-instance setting. */
    void setDebug(DRW::DebugLevel lvl);
    /**
     * Select the DXF classifier profile used by subsequent read, write, and
     * raw-preservation operations. StandaloneSafe is the default and preserves
     * the fail-closed handling of disputed binary widths. LibreCadMasterLegacy
     * is an explicit opt-in for adapters migrating from LibreCAD's bundled
     * fork; it is never selected implicitly from a version or file format.
     * Each admitted read/write keeps one profile for the complete operation;
     * callback changes select subsequent operations, not new widths within
     * the active file. Raw writes use the active writer's admitted profile.
     */
    void setDxfCompatibilityProfile(DxfCompatibilityProfile profile) noexcept;
    /** Return the currently selected DXF compatibility profile. */
    DxfCompatibilityProfile dxfCompatibilityProfile() const noexcept;
    /** Select input text interpretation for subsequent read/readAscii calls.
     * Declared is the default. Recovery applies only to known AC1006..AC1018
     * sources using the default/ANSI_1252 page, never an active non-1252 page.
     * Each admitted read snapshots this persistent setting into its reader;
     * calls during callbacks affect the next read, not the current operation.
     * Source metadata, raw carriers and export encoding are not changed. */
    void setDxfTextEncoding(DRW::DxfTextEncoding encoding) noexcept {
        m_dxfTextEncoding = encoding;
    }
    /** Select input declaration mapping for subsequent DXF reads. Compatible
     * preserves historical aliases. ExactNamed uses an available exact mapping;
     * unknown/unimplemented high-byte text is replaced and observed, never
     * guessed through another page. Modern UTF8, output and raw carriers are
     * unchanged. Admitted reads snapshot this setting before callbacks.
     * Inspect getTextReadObservations(), including the snapped policy.
     * This is not format admission or complete-conversion qualification.
     * Consumer rebuild required; no new pure callback. */
    void setDxfCodePagePolicy(DRW::DxfCodePagePolicy policy) noexcept { m_dxfCodePagePolicy = policy; }
    DRW::DxfCodePagePolicy dxfCodePagePolicy() const noexcept { return m_dxfCodePagePolicy; }
    /** Select semantic-text output policy for subsequent writes. The admitted
     * write snapshots it; callback changes affect only the next write. Ordinary
     * Compatible output/raw carriers remain unchanged. RequireLossless rejects
     * invalid/unrepresentable text and quotes escape-looking semantic slashes,
     * not formatting commands or raw bytes. Consumer rebuild required. */
    void setDxfTextWritePolicy(DRW::DxfTextWritePolicy policy) noexcept {
        m_dxfTextWritePolicy = policy;
    }
    DRW::DxfTextWritePolicy dxfTextWritePolicy() const noexcept {
        return m_dxfTextWritePolicy;
    }
    /** Opt in to a bounded ledger of primary code-5/code-105 record identities.
     * Record scopes roll entries back when aborted; duplicate handles reject
     * the staged write. The completed inventory is queryable only during
     * finalizeDxfWrite(), before atomic publication. It observes the first
     * post-identity group-330 value when present; entity callers may use it
     * to check an expected owner, but it does not prove other edge semantics. */
    void setDxfWriteRecordInventoryEnabled(
            bool enabled,
            std::size_t maxHandles = 4u * 1024u * 1024u);
    struct DxfObjectsWriteView {
        std::string_view bytes; //!< Staged section body, no SECTION/ENDSEC framing.
        DRW::Version version;
        bool binary;
        std::string_view codePage;
        DxfCompatibilityProfile compatibilityProfile;
    };
    using DxfObjectsWriteInspector = std::function<bool(const DxfObjectsWriteView&)>;
    /** Install outside a write. Invoked once for a successfully staged OBJECTS
     * body, before append/publication. Views expire when the callback returns.
     * Returning false, throwing, or attempting a nested read/write or guarded setup mutation
     * refuses the attempt. Future-operation policy setters retain their usual
     * semantics. True is tentative, not a publication/completeness receipt.
     * Default empty: no inspection/copy/decoding overhead beyond normal staging.
     * The callback must not retain views or mutate external stream state. */
    bool setDxfObjectsWriteInspector(DxfObjectsWriteInspector inspector);
    using DxfTablesWriteView = DxfObjectsWriteView;
    using DxfTablesWriteInspector = DxfObjectsWriteInspector;
    /** Same lifetime, mutation guard and tentative/publication contract as
     * setDxfObjectsWriteInspector(), for the complete staged TABLES body after
     * capacity backpatching. Called before BLOCKS/ENTITIES/OBJECTS; table names
     * alone do not qualify complete table semantics or future publication. */
    bool setDxfTablesWriteInspector(DxfTablesWriteInspector inspector);
    using DxfHeaderWriteView = DxfObjectsWriteView;
    using DxfHeaderWriteInspector = DxfObjectsWriteInspector;
    /** Optional HEADER inspection with the same guard/lifetime/tentative
     * contract as OBJECTS. Bytes include the opening group2 HEADER, exclude
     * SECTION/ENDSEC, and contain the pre-backpatch HANDSEED placeholder.
     * Empty by default: retain normal HEADER streaming. Consumer rebuild
     * required for the added private inspector state; no new virtual. */
    bool setDxfHeaderWriteInspector(DxfHeaderWriteInspector inspector);
    // Opt-in staged BLOCKS/ENTITIES inspection. Same guard/lifetime/tentative
    // contract as OBJECTS; section is exactly "BLOCKS" or "ENTITIES". With no
    // inspector, ENTITIES keeps its ordinary streaming path. No new callback.
    using DxfEntitySectionsWriteInspector = std::function<bool(const DxfObjectsWriteView&, std::string_view)>;
    bool setDxfEntitySectionsWriteInspector(DxfEntitySectionsWriteInspector inspector);
    /**
     * Opt in to post-append verification of every generated DXF section body
     * against the transaction's staged file, including CLASSES. The final
     * HANDSEED backpatch is also read back before finalization. This is an
     * internal integrity gate, not a document-completeness or consumer claim.
     */
    bool setDxfStagedOutputAuthentication(bool enabled);
    std::size_t dxfWriteRecordHandleCount(std::uint32_t handle) const noexcept;
    bool dxfWriteRecordHasIdentity(std::uint32_t handle,
                                   const std::string& recordName,
                                   const std::string& sectionName) const;
    bool dxfWriteRecordHasOwner(std::uint32_t handle,
                                std::uint32_t ownerHandle) const noexcept;
    /** During an admitted write, return a source table's successfully staged
     * identity (currently LAYER, APPID, STYLE and VPORT), or zero. Unlike
     * preallocateTableRecordHandle(), this requires a completed row encoder.
     * Tentative until final validation/publication; no semantic name proof. */
    std::uint32_t dxfWriteEmittedSourceTableHandle(std::uint32_t sourceHandle) const noexcept;
    /** During an admitted write, return the existing source LTYPE writer's
     * registered target, or zero for an absent/ambiguous/conflicting mapping.
     * This registry can be populated before its row encoder commits; it is
     * only a tentative target, never completed-row, name or semantic proof.
     * Canonical targets are BYBLOCK 0x14, BYLAYER 0x15 and CONTINUOUS 0x16.
     * Verify actual TABLES bytes separately, then record type/count/owner at
     * finalization. Outside the write attempt (including after failure or
     * publication), return zero. No new virtual or model layout change. */
    std::uint32_t dxfWriteSourceLinetypeTargetHandle(std::uint32_t sourceHandle) const noexcept;
    /** During finalizeDxfWrite(), additionally require one actual TABLES
     * record of the requested type/optional owner. Preallocation, a raw row
     * without a registered source writer, or a conflicting identity cannot
     * supply this receipt. Consumer rebuild required; no new virtual. */
    bool dxfWriteSourceTableHasIdentity(
            std::uint32_t sourceHandle, std::uint32_t emittedHandle,
            const std::string& recordName,
            std::uint32_t expectedOwnerHandle = DRW::NoHandle) const;
    /** During finalizeDxfWrite(), verify one admitted source entity's exact
     * source-to-output binding against the completed record inventory. The
     * source must be marked written and unambiguous; the emitted handle must
     * match its binding and occur once with the requested type/section. This
     * is a per-identity query, not proof that every source or graph edge was
     * inventoried. Consumer rebuild required. */
    bool dxfWriteSourceEntityHasIdentity(
            std::uint32_t sourceHandle, std::uint32_t emittedHandle,
            const std::string& recordName,
            const std::string& sectionName) const;
    /** Owner-aware form for entity identities whose section/type and expected
     * BLOCK_RECORD owner are already known by a closed source graph. */
    bool dxfWriteSourceEntityHasIdentity(
            std::uint32_t sourceHandle, std::uint32_t emittedHandle,
            const std::string& recordName,
            const std::string& sectionName,
            std::uint32_t expectedOwnerHandle) const;
    /** During finalizeDxfWrite(), verify a typed source object identity that
     * its writer registered after successful serialization. The emitted
     * record must be unique and match its requested type/section, and the
     * optional owner must match. This is per-record evidence only; it does not
     * establish closure for object families that do not register identities
     * or validate their outgoing edges. Consumer rebuild required. */
    bool dxfWriteSourceObjectHasIdentity(
            std::uint32_t sourceHandle, std::uint32_t emittedHandle,
            const std::string& recordName,
            const std::string& sectionName) const;
    bool dxfWriteSourceObjectHasIdentity(
            std::uint32_t sourceHandle, std::uint32_t emittedHandle,
            const std::string& recordName,
            const std::string& sectionName,
            std::uint32_t expectedOwnerHandle) const;
    /**
     * Set the aggregate record ceiling for each subsequent DXF read.
     *
     * The default is a resource limit of four million physical records, not a
     * DXF semantic maximum. Set zero to reject the first record; callers that
     * intentionally process larger inputs may raise the limit. The setting is
     * applied independently to each file-backed or in-memory read session.
     */
    void setDxfReadRecordBudget(std::size_t maxRecords) noexcept {
        m_dxfReadRecordBudget = maxRecords;
    }
    /** Return the aggregate record ceiling used by subsequent DXF reads. */
    std::size_t dxfReadRecordBudget() const noexcept {
        return m_dxfReadRecordBudget;
    }
    /** Bound bytes before each ASCII LF or binary string NUL on subsequent
     * reads. ASCII's terminal CR counts before normalization. Zero permits
     * only empty delimited tokens. The 1 MiB resource default is raiseable;
     * this does not bound retained copies, string capacity or OS read-ahead. */
    void setDxfReadTokenByteBudget(std::size_t maxBytes) noexcept {
        m_dxfReadTokenByteBudget = maxBytes;
    }
    std::size_t dxfReadTokenByteBudget() const noexcept {
        return m_dxfReadTokenByteBudget;
    }
    /** Set a cumulative logical-input byte ceiling for each subsequent DXF
     * read. This counts DXF delimiters/group codes, fixed-width values,
     * binary chunks and the binary sentinel, but not file sniff read-ahead,
     * retained copies or stream-buffer capacity. The 1 GiB default is a
     * caller-raiseable resource limit, not a file-format maximum; zero rejects
     * the first consumed byte. Rebuild consumers using this additive API. */
    void setDxfReadInputByteBudget(std::size_t maxBytes) noexcept {
        m_dxfReadInputByteBudget = maxBytes;
    }
    std::size_t dxfReadInputByteBudget() const noexcept {
        return m_dxfReadInputByteBudget;
    }
    /** Set the cumulative deterministic decode-work ceiling for each
     * subsequent DXF read. This counts instrumented source-byte visits, so a
     * decoder that revisits an assembled prefix charges it again; it is not
     * elapsed time, acquired input, retained memory, or a format maximum.
     * The 1 GiB default is caller-raiseable. Changes from callbacks apply to
     * the next read. Current coverage includes repeated MTEXT group-1 prefix
     * decoding and is not a claim that every parser loop is instrumented.
     * Rebuild consumers using this additive API. */
    void setDxfReadDecodeWorkBudget(std::size_t maxUnits) noexcept {
        m_dxfReadDecodeWorkBudget = maxUnits;
    }
    std::size_t dxfReadDecodeWorkBudget() const noexcept {
        return m_dxfReadDecodeWorkBudget;
    }
    /** Bound selected materialized typed DXF `DRW_Variant` nodes admitted by
     * each subsequent read: entity/table-entry XDATA and appData, modeled-
     * object XDATA delegated to `DRW_TableEntry::parseCode`, parsed HEADER
     * variables, synthesized DIMSTYLE `vars`, typed XRECORD payload values,
     * typed LOFTEDSURFACE reference-data entries, DXF ACAD_TABLE cell-value
     * variants, and captured raw-carrier groups plus their distinct
     * raw-backed appData copies. Mirrored XRECORD
     * values in the compatibility and ordered views and each raw/appData
     * materialized copy count as separate nodes; handle payloads count the
     * ordered variant node only. This is not a guarantee over every OBJECTS
     * parser or allocation.
     * The default is 1,048,576 nodes; it is caller-raiseable and not a DXF
     * maximum. Other typed containers and container/user copies remain outside
     * this counter. Callback changes apply to the next read. Rebuild consumers
     * using this API. */
    void setDxfReadTypedVariantNodeBudget(std::size_t maxNodes) noexcept {
        m_dxfReadTypedVariantNodeBudget = maxNodes;
    }
    std::size_t dxfReadTypedVariantNodeBudget() const noexcept {
        return m_dxfReadTypedVariantNodeBudget;
    }
    /** Set the aggregate logical-byte ceiling for library-owned DXF read
     * snapshots/copies. Caller-owned input and allocated capacity are not
     * charged. The 1 GiB default is caller-raiseable and is not a format
     * maximum. The counter covers instrumented owners as implementation
     * coverage is expanded. This applies to subsequent reads; a change from a
     * callback takes effect on the next read. Rebuild consumers using this
     * additive setting. */
    void setDxfReadRetainedByteBudget(std::size_t maxBytes) noexcept {
        m_dxfReadRetainedByteBudget = maxBytes;
    }
    std::size_t dxfReadRetainedByteBudget() const noexcept {
        return m_dxfReadRetainedByteBudget;
    }
    /// Whole-document read/readAscii/write operations are not reentrant on one
    /// instance. A nested call returns false without changing the active
    /// operation, including its error/diagnostic snapshot or destination. Use
    /// another instance for nested whole-document work. Getters and ordinary
    /// write-record/registration callbacks remain available; this is not a
    /// thread-safety guarantee or a change to callback-exception behavior.
    /// reads the file specified in constructor
    /*!
     * An interface must be provided. It is used by the class to signal various
     * components being added.
     * @param interface_ the interface to use
     * @param ext should the extrusion be applied to convert in 2D?
     * @return true for success
     */
    [[nodiscard]] bool read(DRW_Interface *interface_, bool ext);
    [[nodiscard]] bool readAscii(DRW_Interface *interface_, bool ext, std::string& content);
    void setBinary(bool b) {binFile = b;}

    /// Publish a complete DXF, preserving the destination on failure. Successful
    /// record calls may assign caller handles/names; a later file failure does
    /// not roll those calls back. Retry with pristine source identities after a
    /// successful prefix. No borrowed callback records are retained for undo.
    /// On any failed attempt, derived output maps are empty; persistent caller
    /// registrations/remaps/reservations remain available for the next attempt.
    /// finalizeDxfWrite runs after emission, before publication; facade record
    /// writes and output setup mutations are rejected in that final phase.
    [[nodiscard]] bool write(DRW_Interface *interface_, DRW::Version ver, bool bin);
    bool writeLineType(DRW_LType *ent);
    bool writeDictionary(DRW_Dictionary *ent);
    bool writeDictionaryWithDefault(DRW_DictionaryWithDefault *ent);
    bool writeAcDbPlaceholder(DRW_AcDbPlaceholder *ent);
    bool writeXRecord(DRW_XRecord *ent);
    /// Explicit producer-compatibility path for observed AC1021+ DWG-origin
    /// XRECORD Int32 group 1071 values. Ordinary DXF-origin writes must use
    /// writeXRecord(), which retains the standard XRECORD code-domain check.
    bool writeXRecordDwgCompatibility(DRW_XRecord *ent);
    /// Emits the typed AcDbDataLink object profile. Referenced custom-data
    /// targets must already have a valid source-to-output handle mapping.
    bool writeDataLink(DRW_DataLink *ent);
    /// Emits a standalone TABLECONTENT object containing the selected
    /// single-cell linked/reference profiles or a bounded rectangular grid of
    /// unlinked scalar cells. Referenced DATALINK/FIELD/block targets must be
    /// emitted by the caller.
    bool writeTableContent(DRW_TableContentObject *ent);
    bool writeLayer(DRW_Layer *ent);
    bool writeDimstyle(DRW_Dimstyle *ent);
    bool writeTextstyle(DRW_Textstyle *ent);
    bool writeVport(DRW_Vport *ent);
    bool writeView(DRW_View *ent);
    bool writeUCS(DRW_UCS *ent);
    bool writeAppId(DRW_AppId *ent);
    bool writePoint(DRW_Point *ent);
    bool writeLine(DRW_Line *ent);
    bool write3DLine(DRW_3DLine *ent);
    bool writeRay(DRW_Ray *ent);
    bool writeXline(DRW_Xline *ent);
    bool writeCircle(DRW_Circle *ent);
    bool writeArc(DRW_Arc *ent);
    bool writeEllipse(DRW_Ellipse *ent);
    bool writeTrace(DRW_Trace *ent);
    bool writeSolid(DRW_Solid *ent);
    bool write3dface(DRW_3Dface *ent);
    bool writeLWPolyline(DRW_LWPolyline *ent);
    bool writePolyline(DRW_Polyline *ent);
    bool writeSpline(DRW_Spline *ent);
    bool writeHelix(DRW_Helix *ent);
    bool writeBlockRecord(std::string name, int insUnits = 0);
    bool writeBlockRecord(std::string name, int insUnits,
                          const std::vector<std::uint8_t>& previewData);
    bool writeBlockRecord(
        std::string name, int insUnits,
        const std::vector<std::uint8_t>& previewData,
        const std::vector<std::uint32_t>& insertHandles);
    /// Additive BLOCK_RECORD XDATA path. Values are validated and deep-copied
    /// before deferred emission; existing overloads remain unchanged.
    bool writeBlockRecord(
        std::string name, int insUnits,
        const std::vector<std::uint8_t>& previewData,
        const std::vector<std::uint32_t>& insertHandles,
        const std::vector<DRW_Variant*>& extData);
    bool writeBlock(DRW_Block *ent);
    /** Emit the ENDBLK carrier for the currently open BLOCK definition. */
    bool writeBlockEnd(const DRW_Block *footer = nullptr);
    /** Configure retained BLOCK headers for canonical Model_Space/Paper_Space. */
    bool setFixedSpaceBlockBeginCarriers(const DRW_Block *modelSpaceBlock,
                                         const DRW_Block *paperSpaceBlock);
    /** Configure retained footers for the canonical Model_Space/Paper_Space blocks. */
    bool setFixedSpaceBlockEndCarriers(const DRW_Block *modelSpaceFooter,
                                       const DRW_Block *paperSpaceFooter);
    bool writeInsert(DRW_Insert *ent);
    bool writeTable(DRW_Table *ent);
    // Optional AC1018 source-presence spelling; one entry per cell, in row order.
    // The ordinary overload remains source-agnostic.
    bool writeTableWithLegacyZeroOverrides(DRW_Table *ent, const std::vector<std::uint8_t>& legacyZeroOverrides);
    // Selected AC1018 presence mask: bit 0 retains 177=0/178=0;
    // bit 1 retains block-cell 179=0. Zero means neither source spelling.
    bool writeTableWithLegacySourcePresence(DRW_Table *ent, const std::vector<std::uint8_t>& legacyPresence);
    /// Emits the bounded AC1021+ typed TABLESTYLE profile. DXF-origin
    /// TABLESTYLE records continue to use exact raw replay through the
    /// adapter; this typed path is for supported DWG-origin style snapshots.
    bool writeTableStyle(DRW_TableStyle *ent,
                         const std::list<DRW_Textstyle>& textStyles);
    /// Emits a spec-backed CELLSTYLEMAP body. The owning TABLESTYLE's
    /// extension-dictionary edge must still be validated by the adapter.
    bool writeCellStyleMap(DRW_CellStyleMap *ent);
    bool writeAttrib(DRW_Attrib *ent,
                     std::uint32_t ownerOverride = DRW::NoHandle);
    bool writeAttdef(DRW_Attdef *ent,
                     std::uint32_t ownerOverride = DRW::NoHandle);
    bool writeMText(DRW_MText *ent);
    bool writeMLine(DRW_MLine *ent);
    bool writeUnderlay(DRW_Underlay *ent);
    bool writeText(DRW_Text *ent);
    bool writeRText(DRW_RText *ent);
    bool writeArcAlignedText(DRW_ArcAlignedText *ent);
    bool writeTolerance(DRW_Tolerance *ent);
    bool writeHatch(DRW_Hatch *ent);
    bool writeMPolygon(DRW_MPolygon *ent);
    bool writeViewport(DRW_Viewport *ent);
    bool writeLight(DRW_Light *ent);
    bool writeCamera(DRW_Camera *ent);
    bool writeGeoPositionMarker(DRW_GeoPositionMarker *ent);
    bool writeSectionObject(DRW_SectionObject *ent);
    bool writeMesh(DRW_Mesh *ent);
    bool writeShape(DRW_Shape *ent);
    bool writeOle2Frame(DRW_Ole2Frame *ent);
    bool writeOleFrame(DRW_OleFrame *ent);
    /// The returned definition is writer-owned. It is invalidated by failure of
    /// the enclosing write, the next write attempt, or destruction of this codec.
    DRW_ImageDef *writeImage(DRW_Image *ent, std::string name);
    /// Write an IMAGE against the preplanned source definition identity.
    /// Returns nullptr and fails the active write if the identity was not in
    /// the plan; filename fallback is deliberately not used for this overload.
    DRW_ImageDef *writeImage(DRW_Image *ent, std::string name,
                             std::uint64_t importGeneration,
                             std::uint32_t sourceDefinitionHandle);
    bool writeWipeout(DRW_Wipeout *ent);
    bool writePointCloud(DRW_PointCloud *ent);
    bool writePointCloudEx(DRW_PointCloudEx *ent);
    bool writeNavisworksModel(DRW_NavisworksModel *ent);
    bool writePointCloudDef(DRW_PointCloudDef *ent);
    bool writeNavisworksModelDef(DRW_NavisworksModelDef *ent);
    bool writePointCloudColorMap(DRW_PointCloudColorMap *ent);
    bool writeSurface(DRW_Surface *ent);
    bool writeModelerGeometry(DRW_ModelerGeometry *ent);
    bool writeMultiLeader(DRW_MLeader *ent);
    bool writeLeader(DRW_Leader *ent);
    bool writeDimension(DRW_Dimension *ent);
    void setEllipseParts(int parts){elParts = parts;} /*!< set parts number when convert ellipse to polyline */
    bool writePlotSettings(DRW_PlotSettings *ent);
    bool writeLayout(DRW_Layout *ent);
    /*!< F4 — typed DXF emitters for the routed data-only OBJECTS the DWG reader
     * populates only into typed metadata (SUN/SCALE/DICTIONARYVAR/
     * RASTERVARIABLES). The DXF group-code shape is the inverse of each type's
     * parseCode, cross-checked against ezdxf 1.4.4. The filter pulls these from
     * dwgAdvancedMetadata() on the DWG->DXF path (DXF->DXF preserves them via the
     * raw net; the filter dedups by handle to avoid a double-emit). Each emits the
     * verbatim code-5 handle and a 330 owner; a matching CLASS record must be
     * registered (dxfClassForRecordName has SUN/SCALE/DICTIONARYVAR/
     * RASTERVARIABLES). */
    bool writeSun(DRW_Sun *ent);
    bool writeScale(DRW_Scale *ent);
    bool writeDictionaryVar(DRW_DictionaryVar *ent);
    bool writeRasterVariables(DRW_RasterVariables *ent);
    bool writeUnderlayDefinition(DRW_UnderlayDefinition *ent);
    bool writeMLeaderStyle(DRW_MLeaderStyle *ent);
    bool writeGeoData(DRW_GeoData *ent);
    bool writeSpatialFilter(DRW_SpatialFilter *ent);
    bool writeSortEntsTable(DRW_SortEntsTable *ent);
    bool writeField(DRW_Field *ent);
    bool writeFieldList(DRW_FieldList *ent);
    /*!< MLINESTYLE is a FIXED built-in (no CLASS record); DWG read populates
     * only typed metadata, so the filter emits it typed on DWG->DXF, deduped vs
     * the raw net by handle. */
    bool writeMLineStyle(DRW_MLineStyle *ent);
    /*!< WIPEOUTVARIABLES (custom class — CLASS registered) DWG->DXF typed emit;
     * deduped vs the raw net by handle like the other data-only OBJECTS. */
    bool writeWipeoutVariables(DRW_WipeoutVariables *ent);
    bool writeMaterial(DRW_Material *ent);
    bool writeBackground(DRW_Background *ent, const char *recordName);
    bool writeSunStudy(DRW_SunStudy *ent);
    bool writeMotionPath(DRW_MotionPath *ent);
    bool writeCurvePath(DRW_CurvePath *ent);
    bool writePointPath(DRW_PointPath *ent);
    bool writeObjectPtr(DRW_ObjectPtr *ent);
    bool writePartialViewingIndex(DRW_PartialViewingIndex *ent);
    bool writeRenderSettings(DRW_RenderSettings *ent, const char *recordName);
    bool writeSection(DRW_Section *ent, const char *recordName);
    bool writeDbColor(DRW_DbColor *ent, const char *recordName = "DBCOLOR");
    bool writeDimensionAssociation(DRW_DimensionAssociation *ent);
    //! Emit the AC1024 AcDbAssocDependency shared prefix for the exact
    //! ACDBASSOCDEPENDENCY, ACDBASSOCVALUEDEPENDENCY and admitted
    //! ACDBASSOCGEOMDEPENDENCY profiles.
    bool writeAssociativeDependency(DRW_AssociativeObject *ent);
    //! Emit the source-backed AC1024 dimension dependency-body profile.
    bool writeAssociativeDependencyBody(DRW_AssociativeObject *ent);
    //! Emit the source-bound AC1024 ACDBASSOCNETWORK profile.
    bool writeAssociativeNetwork(DRW_AssociativeObject *ent);
    //! Emit the fully consumed AC1024 ACDBASSOCVARIABLE real/int32 profile.
    bool writeAssociativeVariable(DRW_AssociativeObject *ent);
    //! Emit the admitted AC1024 version-0 inline constraint-group profile.
    bool writeAssociativeConstraintGroup(DRW_AssociativeObject *ent);
    bool writeEvaluationGraph(DRW_EvaluationGraph *ent,
                              const char *recordName = "EVALUATION_GRAPH");
    //! Emit only source-backed dynamic-block record-body profiles. Their DWG
    //! owner links must close through the owning dictionary and graph routes
    //! before a higher-level document writer admits them.
    bool writeDynamicBlockObject(DRW_DynamicBlockObject *ent);
    bool writeBlockRepresentationData(
        DRW_BlockRepresentationData *ent);
    bool writeRawDxfObject(DRW_RawDxfObject *obj);
    bool writeRawDxfObject(DRW_RawDxfObject *obj,
                           std::uint32_t ownerOverride);
    //! Replay a raw object while semantically re-encoding selected group-1,
    //! group-2, group-3, group-6 or group-300 UTF-8 strings. Other values retain
    //! the raw path; callers bind each indexed edit to its owning field role.
    bool writeRawDxfObjectWithStringOverrides(
        DRW_RawDxfObject *obj,
        const std::vector<std::pair<std::size_t, std::string>>& overrides);
    //! Replay a raw object with selected UTF-8 string and signed Int32 edits.
    //! All groups outside those indexed edits retain the raw path.
    bool writeRawDxfObjectWithStringAndInt32Overrides(
        DRW_RawDxfObject *obj,
        const std::vector<std::pair<std::size_t, std::string>>& stringOverrides,
        const std::vector<std::pair<std::size_t, std::int32_t>>& int32Overrides);
    //! Replay one captured opaque ENTITIES/BLOCKS record without losing its
    //! source identity or owner. ownerOverride is already an emitted
    //! BLOCK_RECORD handle; NoHandle preserves pre-R2000 ownerless records.
    bool writeRawDxfEntity(DRW_RawDxfObject *entity,
                           std::uint32_t ownerOverride = DRW::NoHandle);
    /*!< Mark a specific code-5 handle as in-use so the minted-handle stream
     * (m_handleAllocator.next()) never re-issues it. Mirrors
     * dwgWriter::reserveHandle. The filter calls this for every verbatim handle
     * preserved in the raw-passthrough net (rawDxfObjects/rawDxfEntities) before
     * write(), so a re-emitted raw OBJECT/ENTITY cannot collide with either a
     * freshly-minted handle or a fixed-low structural handle. Returns false and
     * latches the writer when the handle space is exhausted. */
    bool reserveHandle(std::uint32_t h);
    /*!< High-water mark of the handle allocator (one past the largest handle
     * reserved or minted so far). Used to populate $HANDSEED. Mirrors
     * dwgWriter::highWaterHandle. */
    std::uint32_t highWaterHandle() const;
    //! Explicit allocator reservations, including codec-fixed/BLOCK ranges;
    //! handles merely minted during a write are excluded.
    bool isReservedHandle(std::uint32_t h) const;
    /*!< Register the CLASS records to emit in the DXF CLASSES section. The
     * filter supplies source definitions and recomputes instance counts from
     * raw and typed records selected for output. */
    void setDxfClasses(const std::vector<DRW_Class> &classes) {
        if (!preflightDxfSetup())
            return;
        // Class metadata is immutable once a write has reached its CLASSES
        // section. The writer is reset after each attempt, so callers can
        // replace the persistent input before the next write.
        if (writer != nullptr && m_dxfClassesFrozen) {
            m_writeError = true;
            return;
        }
        m_dxfClasses = classes;
        m_dxfClassesFrozen = false;
    }
    void setDxfClasses(std::vector<DRW_Class>&& classes) {
        if (!preflightDxfSetup())
            return;
        if (writer != nullptr && m_dxfClassesFrozen) {
            m_writeError = true;
            return;
        }
        m_dxfClasses = std::move(classes);
        m_dxfClassesFrozen = false;
    }
    /** Default-off per-write admission. When enabled before a write, the
     * codec refuses if any raw section is present at its replay boundary or
     * after the finalization callback. This concerns only raw-section replay,
     * not HEADER/CLASSES/TABLES/ENTITIES/OBJECTS content or native semantics.
     * A policy change attempted during an operation fails the active write.
     * Rebuild consumers: this adds private state to dxfRW. */
    bool setDxfNoRawSectionsWriteGuard(bool enabled) noexcept;
    /** A transient codec receipt, available only during the consumer's
     * finalization callback after the replay boundary. A true result is
     * tentative until publication and is not a document dependency proof. */
    bool dxfWriteNoRawSectionsObserved() const noexcept;
    /*!< Register unmodeled DXF sections for same-version raw replay. */
    void setRawDxfSections(const std::vector<DRW_RawDxfSection> &sections) {
        if (!preflightDxfSetup())
            return;
        m_rawDxfSections = sections;
    }
    void setRawDxfSections(std::vector<DRW_RawDxfSection>&& sections) {
        if (!preflightDxfSetup())
            return;
        m_rawDxfSections = std::move(sections);
    }
    //! Transfer adapter-owned raw sections into this codec for one synchronous
    //! write, avoiding a second full copy. Restore them with
    //! returnAdoptedRawDxfSections() before the source container is reused.
    bool adoptRawDxfSectionsForWrite(
            std::vector<DRW_RawDxfSection>&& sections) {
        if (!preflightDxfSetup())
            return false;
        m_rawDxfSections = std::move(sections);
        return true;
    }
    //! Return sections previously adopted for a synchronous write without
    //! copying their nested group/value storage.
    void returnAdoptedRawDxfSections(
            std::vector<DRW_RawDxfSection>& sections) noexcept {
        sections.swap(m_rawDxfSections);
    }
    /*!< Canonical DXF CLASS metadata (recName/className/appName/proxyFlag/
     * wasaProxyFlag/entityFlag, per ezdxf REQUIRED_CLASSES) for the known
     * custom-class OBJECTS that the raw net round-trips. Returns false for
     * fixed/built-in or unknown record names. instanceCount is left 0 for the
     * caller to fill. */
    static bool dxfClassForRecordName(const std::string &recName, DRW_Class &out);
    //! Version-specific DXF record spelling for the AcDbSubDMesh entity.
    //! Autodesk documents SUBDIVISION at AC1024 and MESH at AC1027; pre-AC1024
    //! behavior is retained pending an exact sample/provenance audit.
    static const char* dxfMeshRecordNameForVersion(DRW::Version version) noexcept;
    /*!< Register extra (name, hex-handle) entries to splice into the regenerated
     * root NamedObjectsDictionary (handle C) so raw-net-routed named dictionaries
     * are reachable from the root and not pruned as orphans. The filter builds
     * this from the source root dict before write(); empty by default. */
    void setRootDictEntries(const std::vector<std::pair<std::string, std::string>> &entries) {
        if (!preflightDxfSetup())
            return;
        if (m_dxfWriteAttemptActive && m_dxfGroupPlanFrozen && m_generatedNodSource) {
            m_writeError = true;
            return;
        }
        m_rootDictEntries = entries;
    }
    void setRootDictEntries(
            std::vector<std::pair<std::string, std::string>>&& entries) {
        if (!preflightDxfSetup())
            return;
        if (m_dxfWriteAttemptActive && m_dxfGroupPlanFrozen && m_generatedNodSource) {
            m_writeError = true;
            return;
        }
        m_rootDictEntries = std::move(entries);
    }
    /*!< Register named-dictionary OBJECTS to emit verbatim in the regenerated
     * OBJECTS section (DXF write path only). Each carries its source code-5
     * handle, parent (330) owner, and entry (name -> child handle) list. The
     * filter builds these from dwgAdvancedMetadata().dictionaries() on the
     * DWG->DXF path so the named dictionaries that were previously only
     * referenced from the root C dict (via setRootDictEntries) actually exist
     * as reachable objects with a valid owner — clearing the INVALID_OWNER_HANDLE
     * fixes ezdxf applies for the dangling 350 references. Empty by default. */
    void setNamedDictObjects(const std::vector<DRW_Dictionary> &dicts) {
        if (!preflightDxfSetup())
            return;
        if (m_dxfWriteAttemptActive && m_dxfGroupPlanFrozen && !m_groups.empty()) {
            m_writeError = true;
            return;
        }
        m_namedDictObjects = dicts;
    }
    void setNamedDictObjects(std::vector<DRW_Dictionary>&& dicts) {
        if (!preflightDxfSetup())
            return;
        if (m_dxfWriteAttemptActive && m_dxfGroupPlanFrozen && !m_groups.empty()) {
            m_writeError = true;
            return;
        }
        m_namedDictObjects = std::move(dicts);
    }
    /// Declare source identities for the generated NOD (C) and ACAD_GROUP
    /// dictionary (D). Zero leaves an identity unbound; a group dictionary
    /// requires a NOD and cannot share its source handle. Caller remaps must
    /// resolve the declarations to C/D. Registration alone emits nothing and
    /// does not qualify preservation of the source dictionary fields/edges.
    /// Consumers must rebuild against this class-layout addition.
    bool setGeneratedDictionarySourceHandles(std::uint32_t nod,
                                             std::uint32_t groups) {
        if (!preflightDxfSetup() || (groups != 0u && (nod == 0u || groups == nod))
            || (m_generatedNodSource && m_generatedNodSource->handle != nod)
            || (m_generatedGroupDictionarySource && m_generatedGroupDictionarySource->handle != groups))
            return false;
        if (m_dxfWriteAttemptActive && m_dxfGroupPlanFrozen) {
            m_writeError = true;
            return false;
        }
        m_generatedNodSourceHandle = nod;
        m_generatedGroupDictionarySourceHandle = groups;
        return true;
    }
    /// Transfer owned source metadata for the generated C/D dictionaries.
    /// Common metadata and selected source edges retain their values/roles
    /// and order; setRootDictEntries still selects the exported root graph.
    /// This does not certify that omitted source entries were preserved.
    /// Setup/emission is not complete DWG source-frame qualification; any
    /// unmodeled common state still needs separate field/output evidence.
    /// GROUP-dictionary membership must match the emitted GROUPs exactly.
    /// Null/null clears metadata and aliases. Ownership is consumed even on
    /// refusal, while a previous accepted setup is left unchanged. The visible
    /// retained payload is bounded by maxRetainedBytes (at most 64 MiB) and
    /// one million common items. Consumers must rebuild for the owned fields.
    bool setGeneratedDictionarySourceRecords(
        std::unique_ptr<DRW_Dictionary> nod,
        std::unique_ptr<DRW_Dictionary> groups,
        std::size_t maxRetainedBytes = 64u * 1024u * 1024u);
    /// Also transfer the selected source image dictionary, scoped to one
    /// IMAGEDEF-plan generation. All three records share the same byte/item
    /// allowance. Original selected image keys/roles/order and metadata are
    /// preserved; unused source entries remain an explicit projection, not
    /// preservation proof. Image aliases are planned before callbacks but
    /// become emitted identities only after their records serialize.
    bool setGeneratedDictionarySourceRecords(
        std::unique_ptr<DRW_Dictionary> nod,
        std::unique_ptr<DRW_Dictionary> groups,
        std::unique_ptr<DRW_Dictionary> images,
        std::uint64_t imageImportGeneration,
        std::size_t maxRetainedBytes = 64u * 1024u * 1024u);
    /// Install the source IMAGEDEFs referenced by this export. Definitions
    /// are copied, keyed by import generation and source handle, then assigned
    /// fresh output handles before entity emission. An empty plan preserves
    /// the legacy filename-only writeImage API.
    bool setImageDefinitionPlan(
        const std::vector<ImageDefinitionPlanEntry>& definitions);
    bool setImageDefinitionPlan(
        std::vector<ImageDefinitionPlanEntry>&& definitions);
    // Attempt-local generated identities, not publication receipts. Available
    // to synchronous inspectors/finalizers; zero outside a live write.
    std::uint32_t dxfWriteImageDefinitionHandle(ImageDefinitionKey source) const noexcept;
    std::uint32_t dxfWriteImageReactorHandle(ImageDefinitionKey definition,
                                           std::uint32_t sourceReactor) const noexcept;
    // Borrowed attempt-local views, invalid when write returns. The generated
    // view supplies minted IDs only; it is not independent field evidence.
    const ImageDefinitionPlanEntry* dxfWriteImageSource(ImageDefinitionKey source) const noexcept;
    const DRW_ImageDef* dxfWriteGeneratedImageDefinition(ImageDefinitionKey source) const noexcept;
    // Attempt-local frozen source metadata and its minted dictionary identity;
    // neither is proof that the corresponding OBJECTS record was emitted.
    const DRW_Dictionary* dxfWriteImageDictionarySource(std::uint32_t& emitted) const noexcept;
    // Frozen source/plan views, never output evidence. Borrowed only after
    // root planning freezes during an active write; null/zero before that
    // phase or after write returns. Boolean selects ACAD_GROUP.
    const DRW_Dictionary* dxfWriteGeneratedRootSource(bool groups) const noexcept;
    const std::vector<std::pair<std::string, std::string>>* dxfWriteRootEntries() const noexcept;
    std::uint32_t dxfWriteImageDictionaryHandle() const noexcept;
    /*!< Register source handles for complete TABLESTYLE raw records that the
     * same writer callback will emit. A TABLE's 342 reference is accepted only
     * when its style handle is present here; registration alone does not emit
     * or validate the object graph. */
    void setResolvedTableStyleHandles(
            const std::vector<std::uint32_t> &handles) {
        if (!preflightDxfSetup())
            return;
        m_resolvedTableStyleHandles = handles;
    }
    void setResolvedTableStyleHandles(std::vector<std::uint32_t>&& handles) {
        if (!preflightDxfSetup())
            return;
        m_resolvedTableStyleHandles = std::move(handles);
    }
    /*!< Register base-style IDs that were uniquely resolved in the rooted
     * CELLSTYLEMAP of each emitted TABLESTYLE. Standalone TABLECONTENT IDs
     * 4 and 101+ require this proof in addition to the TABLESTYLE handle. */
    void setResolvedTableStyleBaseIds(
            const std::vector<std::pair<std::uint32_t, std::int32_t>>& ids) {
        if (!preflightDxfSetup())
            return;
        m_resolvedTableStyleBaseIds = ids;
    }
    void setResolvedTableStyleBaseIds(
            std::vector<std::pair<std::uint32_t, std::int32_t>>&& ids) {
        if (!preflightDxfSetup())
            return;
        m_resolvedTableStyleBaseIds = std::move(ids);
    }
    /*!< Register rooted Background handles whose typed group-332 consumer or
     * IBL group-340 references will be emitted by this same adapter write.
     * Direct codec callers have no resolved targets by default and remain
     * fail-closed. */
    void setResolvedBackgroundHandles(
            const std::vector<std::uint32_t> &handles) {
        if (!preflightDxfSetup())
            return;
        m_resolvedBackgroundHandles = handles;
    }
    void setResolvedBackgroundHandles(std::vector<std::uint32_t>&& handles) {
        if (!preflightDxfSetup())
            return;
        m_resolvedBackgroundHandles = std::move(handles);
    }
    const std::vector<std::uint32_t>& resolvedBackgroundHandles() const noexcept {
        return m_resolvedBackgroundHandles;
    }
    /*!< Register SUN objects whose one-to-one VPORT/VIEWPORT hard-owner
     * graph was validated by the adapter for this same DXF write. Direct codec
     * callers have no resolved SUN targets by default and remain fail-closed. */
    void setResolvedSunHandles(
            const std::vector<std::uint32_t> &handles) {
        if (!preflightDxfSetup())
            return;
        m_resolvedSunHandles = handles;
    }
    void setResolvedSunHandles(std::vector<std::uint32_t>&& handles) {
        if (!preflightDxfSetup())
            return;
        m_resolvedSunHandles = std::move(handles);
    }
    const std::vector<std::uint32_t>& resolvedSunHandles() const noexcept {
        return m_resolvedSunHandles;
    }
    /*!< Register GROUP objects to typed-emit in the regenerated OBJECTS section
     * (DXF write path only). Each carries its name/description/flags and the
     * member entity SOURCE handles (DRW_Group::m_entityHandles). The codec mints
     * a fresh group handle, injects the (name, minted-handle) entry into the
     * ACAD_GROUP D dict, and emits the GROUP with 340 references resolved through
     * the writeEntity source->minted map. A missing member refuses output
     * rather than silently dropping an edge. A nonzero source GROUP handle
     * registers a transaction-bound emitted object identity; a zero handle
     * preserves the authored/no-source route.
     * Final GROUP identities are planned after writeHeader() returns, before
     * dependent records serialize. Changes during later callbacks refuse the
     * write. Planning is not evidence that a GROUP was emitted.
     * Empty by default. */
    void setGroups(const std::vector<DRW_Group> &groups) {
        if (!preflightDxfSetup())
            return;
        if (m_dxfWriteAttemptActive && m_dxfGroupPlanFrozen) {
            m_writeError = true;
            return;
        }
        m_groups = groups;
        clearDxfGroupPlan();
    }
    //! Move adapter-owned list values into the codec for one synchronous
    //! write. Restore them with returnAdoptedGroupsForWrite() before the
    //! source list is used again.
    bool adoptGroupsForWrite(std::list<DRW_Group>& groups) {
        if (!preflightDxfSetup())
            return false;
        if (m_dxfWriteAttemptActive && m_dxfGroupPlanFrozen) {
            m_writeError = true;
            return false;
        }
        static_assert(noexcept(DRW_Group(std::declval<DRW_Group&&>())),
                      "GROUP write transfer requires nothrow move construction");
        std::vector<DRW_Group> adopted;
        try {
            adopted.reserve(groups.size());
        } catch (...) {
            return false;
        }
        for (DRW_Group& group : groups)
            adopted.emplace_back(std::move(group));
        m_groups = std::move(adopted);
        clearDxfGroupPlan();
        return true;
    }
    //! Borrow the codec-owned GROUP input during a synchronous write callback.
    //! This is input state, not an emitted-record or preservation receipt.
    //! Invalidated by set/adopt/return operations and codec destruction.
    const std::vector<DRW_Group>& groupsForWrite() const noexcept {
        return m_groups;
    }
    //! Restore values into the original list nodes without copying nested
    //! strings or handle vectors. The list's size must remain unchanged until
    //! the synchronous write has returned.
    void returnAdoptedGroupsForWrite(
            std::list<DRW_Group>& groups) noexcept {
        static_assert(noexcept(std::declval<DRW_Group&>() =
                               std::declval<DRW_Group&&>()),
                      "GROUP write transfer requires nothrow move assignment");
        if (groups.size() != m_groups.size()) {
            m_writeError = true;
            return;
        }
        auto source = groups.begin();
        for (DRW_Group& group : m_groups) {
            *source = std::move(group);
            ++source;
        }
        m_groups.clear();
    }
    /*!< Allocate a fresh, collision-free code-5 handle from the codec's
     * allocator. The filter uses this AFTER reserving every raw + fixed handle
     * to remap the small minority of raw objects whose ORIGINAL handle coincides
     * with one of the codec's fixed structural literals (LAYER 0x10, LTYPE
     * 0x14-0x16, BLOCK/ENDBLK/BLOCK_RECORD 0x1C-0x21, ...). Those handles cannot
     * be preserved verbatim because the codec emits its own table/block record at
     * the same literal; remapping the raw object (and rewriting every reference
     * to it via setHandleRemap) is the only collision-free resolution. */
    std::uint32_t allocHandle();
    /*!< Format a handle as the codec's canonical code-5 hex string (uppercase,
     * no leading zeros), so a caller can build a 350/330 reference string that
     * byte-matches the re-emitted handle (e.g. a remapped root-dict entry). */
    std::string toHexStrHandle(std::uint32_t h) { return toHexStr(h); }
    /*!< Register source-handle remaps applied to raw and typed DXF output.
     * Empty by default (source handles are preserved verbatim). With GROUPs,
     * remaps and named dictionaries freeze after writeHeader() returns so
     * later callbacks cannot invalidate their planned destinations. */
    void setHandleRemap(const std::map<std::uint32_t, std::uint32_t> &remap) {
        if (!preflightDxfSetup())
            return;
        if (m_dxfWriteAttemptActive && m_dxfGroupPlanFrozen && !m_groups.empty()) {
            m_writeError = true;
            return;
        }
        m_handleRemap = remap;
    }
    void setHandleRemap(std::map<std::uint32_t, std::uint32_t>&& remap) {
        if (!preflightDxfSetup())
            return;
        if (m_dxfWriteAttemptActive && m_dxfGroupPlanFrozen && !m_groups.empty()) {
            m_writeError = true;
            return;
        }
        m_handleRemap = std::move(remap);
    }
    /*!< Install an owned reference resolver for selected DXF references.
     * Its result is the final emitted identity (it is not remapped again).
     * Zero omits optional reactor/extension-dictionary references; XDATA
     * handle zero is retained as a literal. The callback and its closure are
     * owned/copied by the codec, but objects referenced by closure captures
     * remain the caller's lifetime responsibility. Setup, replacement, and
     * clearing are rejected during a write or finalization. */
    void setReferenceResolver(
        std::function<std::uint32_t(std::uint32_t)> resolver);
    //! Add one derived source-handle mapping while the consumer's writeBlocks
    //! callback is active. Caller mappings installed by setHandleRemap win;
    //! repeated identical derived mappings are harmless, conflicts fail write.
    void addDxfWriteHandleRemap(std::uint32_t sourceHandle,
                                std::uint32_t emittedHandle);
    //! Plan source BLOCK_RECORD identities independently of minted entities.
    //! The records are copied; this does not emit their layout/resource edges.
    //! Strict mode rejects an explicit non-null entity owner without a binding
    //! and modern INSERT names without a planned BLOCK_RECORD target.
    //! Legacy writers without this metadata retain their existing fallback.
    void setSourceBlockRecords(const std::vector<DRW_Block_Record>& records,
                               bool requireResolvedOwners = false) {
        if (!preflightDxfSetup())
            return;
        m_sourceBlockRecords = records;
        m_requireResolvedBlockOwners = requireResolvedOwners;
    }
    void setSourceBlockRecords(std::vector<DRW_Block_Record>&& records,
                               bool requireResolvedOwners = false) {
        if (!preflightDxfSetup())
            return;
        m_sourceBlockRecords = std::move(records);
        m_requireResolvedBlockOwners = requireResolvedOwners;
    }
    /*!< Register source linetypes so metadata attached to the three mandatory
     * canonical rows can be emitted before the LTYPE table is closed. */
    void setCanonicalLineTypeMetadata(
        const std::vector<DRW_LType> &lineTypes) {
        if (!preflightDxfSetup())
            return;
        m_canonicalLineTypeMetadata = lineTypes;
    }
    void setCanonicalLineTypeMetadata(std::vector<DRW_LType>&& lineTypes) {
        if (!preflightDxfSetup())
            return;
        m_canonicalLineTypeMetadata = std::move(lineTypes);
    }
    std::uint32_t remapHandle(std::uint32_t handle) const {
        auto it = m_handleRemap.find(handle);
        if (it != m_handleRemap.end())
            return it->second;
        const auto writeIt = m_dxfWriteHandleRemap.find(handle);
        return writeIt == m_dxfWriteHandleRemap.end()
            ? handle : writeIt->second;
    }
    //! Resolve a source entity during or after a successful write. A failed
    //! attempt clears derived bindings, so an unbound source is returned as-is;
    //! that fallback is not evidence that an object was published.
    std::uint32_t remapEntityHandle(std::uint32_t sourceHandle) const;
    //! Reserve the future code-5 handle for a source entity before table
    //! records are emitted. Returns zero for a zero source handle.
    std::uint32_t preallocateEntityHandle(std::uint32_t sourceHandle);
    //! Reserve the output identity for a source table record before an earlier
    //! table emits a forward reference to it (for example LTYPE -> STYLE).
    //! Returns zero and fails the write for invalid or conflicting identities.
    std::uint32_t preallocateTableRecordHandle(std::uint32_t sourceHandle);
    //! Bind a preserved source-entity identity to its planned output handle.
    //! Used for raw records and structural entities such as ENDBLK whose
    //! output handles are reserved before the record is emitted.
    //! The binding is rejected for null or ambiguous source identities.
    bool bindSourceEntityHandle(std::uint32_t sourceHandle,
                                std::uint32_t emittedHandle);
    //! Mark a source handle as ambiguous before entity emission. Deferred
    //! references must not guess which of multiple source entities it names.
    void markSourceHandleAmbiguous(std::uint32_t sourceHandle);

    // Opt-in independent freeze of the final DXF identity census before the
    // consumer callback. The census stays private; after a successful read a
    // per-reader generation can authenticate the delivered copy exactly.
    // This is delivery integrity, not a namespace/ownership/support claim.
    class DxfIdentityDeliveryTicket {
        friend class dxfRW;
        explicit DxfIdentityDeliveryTicket(std::uint64_t generation) noexcept: m_generation(generation) {}
        std::uint64_t m_generation;
    };
    using DxfIdentityDeliveryTicketPtr=std::shared_ptr<const DxfIdentityDeliveryTicket>;
    struct DxfIdentityDeliveryState {
        bool enabled{false},failed{false},complete{false},captured{false};
        std::uint64_t generation{0};
        std::size_t sections{0},records{0},candidates{0},bytes{0},work{0};
    };
    bool enableDxfIdentityDeliveryLedger(std::size_t sections=1024u,
        std::size_t records=100000u,std::size_t candidates=400000u,
        std::size_t bytes=64u*1024u*1024u,std::size_t work=128u*1024u*1024u) noexcept;
    DxfIdentityDeliveryState dxfIdentityDeliveryState() const noexcept;
    // The ticket is available only inside the exact source publication. It
    // cannot authenticate another read even when that read has identical bytes.
    DxfIdentityDeliveryTicketPtr dxfIdentityDeliveryTicket(const DRW_DxfIdentitySource&) const noexcept;
    bool dxfIdentityDeliveryMatches(const DRW_DxfIdentitySource&,const DxfIdentityDeliveryTicketPtr&) const noexcept;

    // Default-off, per-section authentication of original ENTITIES physical
    // and delivery publications. No geometry, namespace or support claim.
    class DxfEntitiesDeliveryTicket {
        friend class dxfRW;
        DxfEntitiesDeliveryTicket(std::uint64_t generation,std::size_t index) noexcept:m_generation(generation),m_index(index) {}
        std::uint64_t m_generation;std::size_t m_index;
    };
    using DxfEntitiesDeliveryTicketPtr=std::shared_ptr<const DxfEntitiesDeliveryTicket>;
    struct DxfEntitiesDeliveryState {
        bool enabled{false},failed{false},complete{false};std::uint64_t generation{0};
        std::size_t scopes{0},deliveryScopes{0},records{0},groups{0},dispatches{0},events{0},bytes{0},work{0};
    };
    bool enableDxfEntitiesDeliveryLedger(std::size_t scopes=1024u,std::size_t records=100000u,
        std::size_t groups=4000000u,std::size_t dispatches=100000u,std::size_t events=4000000u,
        std::size_t bytes=64u*1024u*1024u,std::size_t work=128u*1024u*1024u) noexcept;
    DxfEntitiesDeliveryState dxfEntitiesDeliveryState() const noexcept;
    DxfEntitiesDeliveryTicketPtr dxfEntitiesSourceTicket(const DRW_DxfEntitiesSource&) const noexcept;
    DxfEntitiesDeliveryTicketPtr dxfEntitiesManifestTicket(const DRW_DxfEntitiesDeliverySource&) const noexcept;
    bool dxfEntitiesSourceMatches(const DRW_DxfEntitiesSource&,const DxfEntitiesDeliveryTicketPtr&) const noexcept;
    bool dxfEntitiesManifestMatches(const DRW_DxfEntitiesDeliverySource&,const DxfEntitiesDeliveryTicketPtr&) const noexcept;

    // Opt-in, bounded original ACDSDATA section capture for import delivery
    // reconciliation. Default reads do not allocate this ledger. The active
    // source pointer is borrowed only during the raw-section callback.
    struct DxfAcdsSourceLedger {
        bool enabled{false}, failed{false}, complete{false};
        std::size_t recordsLimit{100000u}, groupsLimit{4000000u};
        std::size_t bytesLimit{64u * 1024u * 1024u}, workLimit{128u * 1024u * 1024u};
        std::size_t groups{0u}, bytes{0u}, work{0u};
        std::vector<DRW_RawDxfSection> sections;
    };
    bool enableDxfAcdsSourceLedger(std::size_t records=100000u,
        std::size_t groups=4000000u, std::size_t bytes=64u*1024u*1024u,
        std::size_t work=128u*1024u*1024u) noexcept;
    const DxfAcdsSourceLedger& dxfAcdsSourceLedger() const noexcept { return m_dxfAcdsSourceLedger; }
    const DRW_RawDxfSection* dxfActiveAcdsSource() const noexcept { return m_dxfAcdsActiveSource; }

    DRW::Version getVersion() const;
    //! Accepted declared DXF source revision, not getVersion()'s codec version.
    //! UNKNOWNV until parsing returns, also after a callback throws. A returned
    //! parse failure retains an accepted declaration; missing/unknown metadata
    //! remains UNKNOWNV. A new file read clears it even on argument/open failure;
    //! readAscii(nullptr) and rejected nesting preserve the previous snapshot.
    //! Writes leave this last-read snapshot unchanged. Consumer rebuild required.
    DRW::Version getSourceVersion() const;
    struct DxfReadContext {
        DRW::Version sourceVersion{DRW::UNKNOWNV},codecVersion{DRW::UNKNOWNV};
        std::string_view codecCodePage,declaration;
        DxfCompatibilityProfile compatibilityProfile{DxfCompatibilityProfile::StandaloneSafe};
        DRW::DxfTextEncoding textEncoding{DRW::DxfTextEncoding::Declared};
        DRW::DxfCodePagePolicy codePagePolicy{DRW::DxfCodePagePolicy::Compatible};
        bool hasDeclaration{false},applyExtrusion{false},binary{false};
    };
    /** Live read-only interpretation context, available only during an admitted
     * read/readAscii. Returns false and clears the view outside that phase.
     * String views expire on HEADER/codec reconfiguration, read completion/reuse
     * or destruction. It is codec metadata: modern DXF still decodes UTF8
     * independently. Callers retaining it must make their own bounded copy.
     * No interface/model layout or established callback vtable change. */
    bool getDxfReadContext(DxfReadContext&) const noexcept;
    struct DxfBlockSourceView {
        enum class Phase : std::uint8_t {None,BeginSource,BeginTyped,EndSource,EndTyped,Children,Manifest,CallbackBegin,CallbackValue,CallbackEnd,Finish,Close};
        Phase phase{Phase::None};std::uint32_t blockHandle{0},blockRecord{0};std::uint64_t scope{0};std::size_t index{0};
        const DRW_Block* block{nullptr};const DRW_RawDxfObject* source{nullptr};
        const DRW_DxfBlockChildrenSource* physical{nullptr};const DRW_DxfBlockDeliverySource* delivery{nullptr};
        const DRW_DxfBlockDeliveryEvent* event{nullptr};const DRW_Entity* entity{nullptr};const DRW_RawDxfObject* raw{nullptr};
        bool originalPendingXdata{false};std::uint8_t originalCoordinateMask{0};
    };
    /** Borrowed actual opted-in BLOCKS producer publication. Original carriers
     * are moved into read-only storage before exposure; phases/pointers expire
     * at callback/pair return or throw. False clears the view elsewhere. Keep
     * declared source, reconciled typed block, physical inventory, manifest and
     * staged callback identities distinct. Not target/output/native proof.
     * State lives in the internal reader; no facade/model/vtable layout change. */
    bool getDxfBlockSourceView(DxfBlockSourceView&) const noexcept;
    struct DxfLayoutSourceView {
        enum class Phase : std::uint8_t {None,Source,Typed};
        Phase phase{Phase::None};
        const DRW_Layout* model{nullptr};
        const DRW_RawDxfObject* source{nullptr};
    };
    /** Borrowed identity of the actual opted-in LAYOUT publication. Available
     * only inside its source/typed callbacks; false clears the view elsewhere.
     * All pointers expire when that publication returns/throws. Not writable
     * historical evidence, target qualification or an additional callback.
     * Publication state lives in the internal reader, not the public facade. */
    bool getDxfLayoutSourceView(DxfLayoutSourceView&) const noexcept;
    struct DxfTableSourceView {
        enum class Phase : std::uint8_t {None,Source,Typed};
        Phase phase{Phase::None};const DRW_Table* model{nullptr};const DRW_RawDxfObject* source{nullptr};bool inBlock{false};
    };
    /** Exact actual opted-in TABLE source/typed publication in ENTITIES/BLOCKS.
     * Borrowed model/const-raw pointers expire at pair return/throw; false clears
     * the view elsewhere. No new callback, writable history or output proof.
     * State lives in the internal reader, not the public facade/model layout. */
    bool getDxfTableSourceView(DxfTableSourceView&) const noexcept;
    struct DxfViewportSourceView {
        const DRW_Viewport* model{nullptr};bool inBlock{false},originalPendingXdata{false};std::uint8_t originalCoordinateMask{0};
    };
    /** Borrowed actual VIEWPORT typed publication and original pre-staging
     * common state in ENTITIES/BLOCKS. Available only during its callback;
     * false clears the view and pointers expire on return/throw. Uses internal
     * reader state; adds no facade field, callback, raw copy or support claim. */
    bool getDxfViewportSourceView(DxfViewportSourceView&) const noexcept;
    struct DxfStructuralSourceView {
        enum class Kind : std::uint8_t {None,BlockRecord,RootDictionary,Dictionary};
        enum class Phase : std::uint8_t {None,Source,Typed,Raw};
        Kind kind{Kind::None};Phase phase{Phase::None};
        const DRW_TableEntry* model{nullptr};const DRW_RawDxfObject* source{nullptr};
        bool originalPendingXdata{false};std::uint8_t originalCoordinateMask{0};
    };
    /** Exact opted-in BLOCK_RECORD/DICTIONARY publication.
     * Source-before-typed for BLOCK_RECORD, typed-before-source for dictionary;
     * non-root dictionary typed/[existing raw]/source. Established callbacks
     * and raw suppression unchanged. Parser state is observed before
     * staging/copy/callbacks. Borrowed pointers expire at pair return/throw;
     * false clears the view elsewhere. Not target/graph/output qualification.
     * All publication state lives in the internal reader, not this facade. */
    bool getDxfStructuralSourceView(DxfStructuralSourceView&) const noexcept;
    struct DxfResourceSourceView {
        enum class Kind : std::uint8_t {None,TableStyle,Field,XRecord,DataLink,TextStyle,LineType,Layer,AppId};
        enum class Phase : std::uint8_t {None,Typed,Raw,Source};
        Kind kind{Kind::None};Phase phase{Phase::None};
        const DRW_TableEntry* model{nullptr};const DRW_RawDxfObject* source{nullptr};
        bool originalPendingXdata{false};std::uint8_t originalCoordinateMask{0};
        bool typedAvailable{false},originalPayloadComplete{false};
    };
    /** Borrowed opted-in resource publication. Rich objects publish typed/raw;
     * symbols publish source/typed when their source facet is selected. Actual
     * pointers, immutable raw carrier and original pre-copy cursor/decoder
     * closure verdict (not complete field semantics). Symbol state is observed
     * before staging, including copies that sanitize it. Raw-only TABLESTYLE
     * fallback has typedAvailable=false.
     * False clears the view outside the pair, including return/throw. No
     * facade/model/vtable state, extra callback or target/output certificate. */
    bool getDxfResourceSourceView(DxfResourceSourceView&) const noexcept;
    struct DxfPlotStyleSourceView {
        enum class Kind : std::uint8_t {None,DictionaryWithDefault,Placeholder};
        enum class Phase : std::uint8_t {None,Typed,Raw};
        Kind kind{Kind::None};Phase phase{Phase::None};
        const DRW_TableEntry* model{nullptr};const DRW_RawDxfObject* source{nullptr};
        bool originalPendingXdata{false};std::uint8_t originalCoordinateMask{0};
        bool typedAvailable{false},originalPayloadComplete{false};
    };
    /** Borrowed actual opted-in DICTIONARYWDFLT typed/raw publication or
     * ACDBPLACEHOLDER existing raw-only callback. Placeholder has model=null;
     * no typed model/callback is synthesized. Fixed-handle raw suppression
     * is preserved. Pointers expire at return/throw and false clears the view
     * elsewhere. Uses the existing resource-source opt-in; adds no facade
     * state or vtable slot. Existing resource-view kinds/contract are unchanged.
     * This is original publication transport, not graph/output qualification. */
    bool getDxfPlotStyleSourceView(DxfPlotStyleSourceView&) const noexcept;
    struct DxfRenderingSourceView {
        enum class Kind : std::uint8_t {None,VisualStyle,RenderSettings};
        enum class Phase : std::uint8_t {None,Typed,Raw};
        Kind kind{Kind::None};Phase phase{Phase::None};
        const DRW_TableEntry* model{nullptr};
        const DRW_RawDxfObject* source{nullptr};
        bool originalPendingXdata{false};std::uint8_t originalCoordinateMask{0};
        bool originalVisualStyleScope{false},originalVisualStylePropertyTail{false};
        bool originalRenderBaseScope{false};
        // Counts: base strings,90,290,280,40. Fixed base values retain their
        // original positional state; zero/absent distinctions use the counts.
        std::array<std::size_t,5> originalRenderCounts{};
        std::array<std::int32_t,3> originalRenderLongs{},originalRenderBytes{};
        std::array<bool,5> originalRenderBools{};
        std::array<double,4> originalRenderDoubles{};
    };
    /** Borrowed actual VISUALSTYLE/RenderSettings typed/raw publication through
     * the existing resource-source opt-in. Original common/private/positional
     * state is captured before callbacks; it is not reconstructed from copies.
     * Pointers expire at return/throw; false clears the view elsewhere. Adds no
     * facade/model data member or existing interface/facet virtual. Existing
     * callback order and resource-view kinds stay intact. Transport and parser
     * state alone do not qualify full properties, targets, semantics or output. */
    bool getDxfRenderingSourceView(DxfRenderingSourceView&) const noexcept;
    struct DxfVisualStylePropertiesView {
        DxfRenderingSourceView::Phase phase{DxfRenderingSourceView::Phase::None};
        const DRW_VisualStyle* model{nullptr};const DRW_RawDxfObject* source{nullptr};
        const DRW_DxfVisualStyleProperties* properties{nullptr};
    };
    /** Original scoped primary DXF properties/presence for the actual opted-in
     * VISUALSTYLE typed/raw callbacks. The separate immutable properties include
     * distinct colors62/63/421. All pointers expire at callback return/throw;
     * false clears the view elsewhere. Existing model/facade/view layouts and
     * virtuals are unchanged. This is not full extension/source qualification. */
    bool getDxfVisualStylePropertiesView(DxfVisualStylePropertiesView&) const noexcept;
    struct DxfObjectPhysicalSourceView {
        enum class Kind : std::uint8_t {None,VisualStyle,RenderSettings,RootDictionary,Dictionary};
        enum class Phase : std::uint8_t {None,Typed,Raw,Source};
        Kind kind{Kind::None};Phase phase{Phase::None};
        const DRW_TableEntry* model{nullptr};const DRW_RawDxfObject* source{nullptr};
        DRW_DxfObjectPhysicalSource::Status status{DRW_DxfObjectPhysicalSource::Status::NotCaptured};
        std::shared_ptr<const DRW_DxfObjectPhysicalSource> physical;
    };
    /** Actual reader spans for opted-in rendering and OBJECTS dictionary
     * publications, including ignored comments. Borrowed model/source pointers
     * expire on return/throw; the immutable physical owner may be retained.
     * Allocation/limits make evidence unavailable without changing callbacks.
     * False clears the view outside the actual publication. No existing
     * facade/model/view member, interface slot or callback is added. */
    bool getDxfObjectPhysicalSourceView(DxfObjectPhysicalSourceView&) const noexcept;
    struct DxfViewUcsSourceView {
        enum class Kind : std::uint8_t {None,View,Ucs};
        enum class Phase : std::uint8_t {None=0,Typed=1,Source=3};
        Kind kind{Kind::None};Phase phase{Phase::None};
        const DRW_TableEntry* model{nullptr};const DRW_RawDxfObject* source{nullptr};
        bool originalPendingXdata{false};std::uint8_t originalCoordinateMask{0};
        bool originalPayloadComplete{false};
    };
    /** Borrowed actual opted-in VIEW/UCS source/typed pair, including original
     * pre-staging common state. Pointers expire on return/throw; false clears
     * the view elsewhere. No facade state/vtable changes or legacy resource
     * kind extension. Publication alone is not target/semantic/output proof. */
    bool getDxfViewUcsSourceView(DxfViewUcsSourceView&) const noexcept;
    /** Actual legacy raw-callback delivery, only inside a non-root dictionary
     * source callback. False clears delivered outside that exact publication.
     * This distinguishes fixed-handle suppression from an adapter dropping a
     * delivered raw callback; source evidence must not silently repair the latter. */
    bool getDxfDictionaryRawDelivery(bool& delivered) const noexcept;
    /** Exact borrowed identity-source publication, only inside its final
     * callback. Null elsewhere, including after completion or exception. The
     * reader owns publication state; no public facade layout fields added. */
    const DRW_DxfIdentitySource* getDxfIdentitySourceView() const noexcept;
    DRW::error getError() const;
    /// Snapshot the first failure and bounded cleanup evidence from the most
    /// recent read/write operation.  The legacy getError() value remains the
    /// compatibility channel. If diagnostic text allocation fails, recording
    /// retains the scalar phase/cause/location; optional text can be empty.
    DRW_OperationDiagnostic getLastDiagnostic() const;
    /** Fixed-size observations from completed semantic DXF decode calls.
     * Counters overlap and are not unique-record/character coverage. Metadata,
     * raw controls/binary/handles are excluded. No text, selection or failure
     * policy changes. Copies survive reuse/destruction. An admitted file read
     * resets even on early failure; readAscii(nullptr), rejected nested calls
     * and writes preserve the last read snapshot. Consumer rebuild required. */
    DRW_DxfTextReadObservations getTextReadObservations() const noexcept;

    std::uint32_t getBlockRecordHandleToWrite(const std::string& blockName) const;
    //! Return the emitted ENDBLK identity reserved for a named BLOCK body.
    //! Used to bind a retained source footer handle before earlier records
    //! serialize references to it. DRW::NoHandle means no such body is planned.
    std::uint32_t getBlockEndHandleToWrite(const std::string& blockName) const;
    std::uint32_t getTextStyleHandle(const std::string& styleName) const;
    DRW_ParsingContext* getReadingContext() { return &m_readingContext; }
    //! DXF-derived fields are cleared by a failed write attempt. Direct context
    //! access does not itself certify that any referenced object was published.
    //! Do not mutate this context, including a previously retained alias, from
    //! finalizeDxfWrite; the facade cannot guard writes through caller aliases.
    DRW_WritingContext* getWritingContext() { return &m_writingContext; }
    /** Return target-version omissions observed during the most recent
     * admitted write attempt. Unsupported target records still make write()
     * fail and are never silently published as a lossy file; their counts are
     * retained for diagnostics after failure. The map object is owned by this
     * codec and lives until destruction; its contents reset at the beginning
     * of the next admitted write attempt. This is not thread-safe. */
    const std::map<std::string, std::size_t>& leftOut() const {
        return m_leftOut;
    }

private:
    // Executable-local allocation instrumentation; no runtime/public test API.
    friend class DxIfaceStandardDwgCaptureTestAccess;
    enum class DxfEntityBoundary {
        NextEntity,
        EndSection,
        EndBlock,
        Error
    };

    /// used by read() to parse the content of the file
#if defined(_MSC_VER) && defined(LIBDXFRW_FORCE_TEST_SEAMS)
public:
#endif
    LIBDXFRW_TEST_SEAM_NOINLINE bool processDxf();
    bool processRawDxfSection(std::string_view sectionName);
    bool processHeader();
    bool processClasses();
    bool processTables();
    bool processBlocks();
    bool processBlock();
    bool processEntities(bool isblock);
    bool processObjects();
    bool processUnderlayDefinition();
    bool processDetailViewStyle();
    bool processSectionViewStyle();
    bool publishOrQueueDxfViewStyleName(
        bool sectionStyle, std::uint32_t handle, std::uint32_t parentHandle,
        std::function<void(const std::string&)> publish);
    bool resolveDxfViewStyleName(bool sectionStyle, std::uint32_t handle,
                                 std::uint32_t parentHandle,
                                 std::string& name) const;
    bool flushPendingDxfViewStyleNames(bool force);
    void resetDxfViewStyleNameResolution();
    void observeDxfViewStyleRoot(const DRW_Dictionary& dictionary);
    bool observeDxfViewStyleDictionary(DRW_Dictionary&& dictionary);
    bool processBreakData();
    bool processBreakPointRef();
    bool processMaterial();
    /// Shared body of the raw-capture object readers: capture every group
    /// into a DRW_RawDxfObject for lossless re-emit, parse it into the typed
    /// payload, and hand both to the interface at the object boundary.  Only
    /// the payload type, the debug label and the interface callback differ
    /// between the readers that use it.  Defined in libdxfrw.cpp and
    /// instantiated only there.
    template <typename T, typename AddFn>
    bool processRawCapturedObject(const char* debugName, AddFn addTyped);

    bool processDbColor();
    bool processEvaluationGraph();
    bool processGeoData();
    bool processVisualStyle();
    bool processImageDefReactor();
    bool processSpatialFilter();
    bool processTableStyle();
    bool processCellStyleMap();
    bool processMLeaderStyle();
    bool processSortEntsTable();
    bool processDimAssoc();
    bool processBackground();
    bool processPointCloudDef();
    bool processNavisworksModelDef();
    bool processPointCloudColorMap();
    bool processSunStudy();
    bool processIndex();
    bool processLayerIndex();
    bool processSpatialIndex();
    bool processIDBuffer();
    bool processMotionPath();
    bool processCurvePath();
    bool processPointPath();
    bool processTvDeviceProperties();
    bool processCsacDocumentOptions();
    bool processObjectPtr();
    bool processPartialViewingIndex();
    bool processRenderSettings();
    bool processSection();
    bool processAssociativeObject();
    bool processDynamicBlockObject();
    bool processBlockRepresentationData();
    bool processAcShHistoryObject();

    DxfEntityBoundary readEntityBoundary();
    DxfEntityBoundary setEntityBoundary(int code);
    DxfEntityBoundary classifyEntityBoundary() const;
    bool acceptEntityCallbackBoundary() const;
    bool acceptObjectBoundary(int code);
    DxfEntityBoundary consumeEntityFooter(DRW_SeqEnd *footer = nullptr);

    bool processLType();
    bool processLayer();
    bool processDimStyle();
    bool processTextStyle();
    bool processVports();
    bool processAppId();
    bool processView();
    bool processUCS();
    bool processBlockRecord();

    bool processPoint();
    bool processLine();
    bool process3DLine();
    bool processRay();
    bool processXline();
    bool processCircle();
    bool processArc();
    bool processEllipse();
    bool processTrace();
    bool processSolid();
    bool processShape();
    bool processOle2Frame();
    bool processOleFrame();
    bool processInsert();
    bool processTable();
    bool processAttrib(DRW_Insert* insert);
    bool processAttdef();
    bool processLWPolyline();
    bool processPolyline();
    bool processVertex(DRW_Polyline* pl);
    bool processText();
    bool processTolerance();
    bool processCamera();
    bool processGeoPositionMarker();
    bool processSectionObject();
    bool processMText();
    bool processRText();
    bool processArcAlignedText();
    bool processMLine();
    bool processUnderlay(const std::string& kind);
    bool processHatch();
    bool processMPolygon();
    bool processSpline();
    bool processHelix();
    bool process3dface();
    bool processMesh();
    bool processViewport();
    bool processImage();
    bool processImageDef();
    bool processWipeout();
    bool processPointCloud();
    bool processPointCloudEx();
    bool processNavisworksModel();
    bool processSurface();
    bool processModelerGeometry();
    bool processMultiLeader();
    bool processDimension();
    bool processArcDimension();
    bool processLargeRadialDimension();
    bool processLeader();
    bool processPlotSettings();
    bool processGroup();
    bool processLightList();
    bool processDataLink();
    bool processGeoMapImage();
    bool processLayerFilter();
    bool processDictionary();
    bool processScale();
    bool processMLineStyle();
    bool processDictionaryVar();
    bool processXRecord();
    bool processDictionaryWithDefault();
    bool processRasterVariables();
    bool processField();
    bool processFieldList();
    bool processSun();
    bool processLayout();
    bool processWipeoutVariables();
    bool processProxyObject();
    bool processProxyEntity();
    bool processRawObject(bool reportUnmodeled = true);
    bool processRawEntity();
    /*!< Append the current DXF record (already read by reader->readRec) to a
     * raw-passthrough carrier as a correctly-TYPED DRW_Variant, classifying the
     * value by DXF code range (numeric codes leave reader->strData stale AND
     * clobber reader->type to STRING, so neither getString() nor type can be
     * trusted for them — that was the A1/A4 capture bug). ASCII-DXF only. Also
     * latches code 5 -> handle and code 330 -> parentHandle. */
    LIBDXFRW_TEST_SEAM_NOINLINE bool captureRawGroup(DRW_RawDxfObject &obj, int code,
                         bool validateHandles = false,
                         bool selfHandle = true,
                         bool commonIdentity = true);
    bool captureRawDxfApplicationGroup(DRW_RawDxfObject &obj,
                                       std::list<std::list<DRW_Variant>> &appData,
                                       std::vector<std::uint32_t> &reactorHandles,
                                       std::uint32_t &xDictHandle,
                                       std::shared_ptr<DxfReadRetainedLease> &typedLease);
    bool captureRawDxfApplicationGroup(DRW_RawDxfObject &obj,
                                       DRW_TableEntry &entry);
    bool captureRawDxfApplicationGroup(DRW_RawDxfObject &obj,
                                       DRW_Entity &entity);
    bool copyRawDxfCarrierName(DRW_RawDxfObject &obj,
                               const std::string& name);
    bool captureAndParseRawDxfGroup(DRW_RawDxfObject &obj, int code,
                                    DRW_TableEntry &entry);
    bool captureAndParseRawDxfGroup(DRW_RawDxfObject &obj, int code,
                                    DRW_Entity &entity);

//    bool writeHeader();
    /// Reserve the DXF codec's fixed structural code-5 literals (table heads,
    /// mandatory table records, BLOCK_RECORDs, the *Model/*Paper BLOCK+ENDBLK,
    /// and the root dict "C" / ACAD_GROUP "D") in m_handleAllocator before the
    /// body is streamed. These DIFFER from the DWG seedReserved() set. After
    /// this, the first next() yields FIRSTHANDLE (0x30) exactly as the legacy
    /// ++entCount did, so a fresh write (empty raw net) is byte-identical.
    void seedReservedDxf();
    struct PendingDxfBlockRecord {
        std::uint32_t handle {0};
        std::string name;
        int insUnits {0};
        std::vector<std::uint8_t> previewData;
        std::vector<std::uint32_t> insertHandles;
        std::vector<std::shared_ptr<DRW_Variant>> extData;
    };
    enum class DxfViewStyleNameState : std::uint8_t {
        NotSeen,
        Missing,
        AwaitingDictionary,
        Ready,
        Invalid
    };
    struct DxfViewStyleNameRoute {
        DxfViewStyleNameState state {DxfViewStyleNameState::NotSeen};
        std::uint32_t rootHandle {DRW::NoHandle};
        std::uint32_t childHandle {DRW::NoHandle};
        std::vector<DRW_Dictionary::Entry> entries;
        std::unordered_map<std::uint32_t, std::size_t> entryIndexByHandle;
        std::unordered_set<std::uint32_t> ambiguousHandles;
    };
    struct PendingDxfViewStyleName {
        bool sectionStyle {false};
        std::uint32_t handle {DRW::NoHandle};
        std::uint32_t parentHandle {DRW::NoHandle};
        std::function<void(const std::string&)> publish;
    };
    enum class DxfWriteMutationKind : std::uint8_t {
        BlockMapInsert,
        TextStyleMapSet,
        SourceTextStyleNameInsert,
        SourceLinetypeTargetInsert,
        SourceHandleInsert,
        SourceTableRecordInsert,
        SourceTableIdentityInsert,
        SourceHandleErase,
        AmbiguousSourceHandleInsert,
        WrittenSourceEntityHandleInsert,
        SourceObjectIdentityInsert,
        ModelerSourceHandleInsert,
        ImageReactorInsert,
        ImageSourceReactorInsert,
        EmittedDxfBlockBodyInsert,
        EmittedDxfDimStyleNameInsert,
        PendingDxfDimensionPictureBlockInsert,
        PendingDxfInsertBodyTargetInsert,
        PendingDxfDimstyleBlockResourceInsert
    };
    struct DxfWriteMutation {
        DxfWriteMutationKind kind {DxfWriteMutationKind::BlockMapInsert};
        std::string key;
        std::uint32_t handle {0};
        std::uint32_t previousHandle {0};
        bool hadPrevious {false};
        DRW_ImageDef *imageDef {nullptr};
    };
    struct SourceLinetypeTarget {
        std::string name;
        std::uint32_t emittedHandle {0};
    };
    bool emitBlockRecord(const PendingDxfBlockRecord& record);
    bool preflightDxfXdataAppIds(
        const std::vector<DRW_Variant*>& extData);
    bool validateDxfXdataAppIdRegistrations();
    bool planSourceBlockRecordHandles();
    bool planDxfGroupHandles();
    void clearDxfGroupPlan() noexcept {
        m_plannedDxfGroupHandles.clear();
        m_plannedDxfGroupSourceHandles.clear();
        m_dxfGroupPlanFrozen = false;
    }
    bool sourceBlockRecordLayoutHandle(
        std::uint32_t emittedBlockRecord,
        std::uint32_t& sourceLayoutHandle) const noexcept;
    bool writeSourceBlockRecordLayoutLink(
        std::uint32_t emittedBlockRecord);
    bool writeSourceBlockRecordExtensionDictionary(
        std::uint32_t emittedBlockRecord);
    // captureSourceHandle=false on the VERTEX/SEQEND parent re-entries
    // (writePolyline/writeInsert) so they do not pollute the source->minted map.
    // recordName is supplied only for a known target-version limitation; the
    // attempted omission is counted, but the standalone writer remains strict.
    bool rejectUnsupportedDxfWrite(const char* recordName = nullptr) noexcept;
    bool rejectDxfWriteWithReason(const char* recordName,
                                  const char* reason) noexcept;
    bool failDxfWrite() noexcept;
    bool bindDxfWriteSourceObjectHandle(std::uint32_t sourceHandle,
                                       std::uint32_t emittedHandle);
    bool bindDxfWriteSourceTableHandle(std::uint32_t sourceHandle,
                                      std::uint32_t emittedHandle,
                                      bool allowLayerZeroEntityAlias = false);
    bool writeRequiredString(int code, const std::string& value);
    bool allocateDxfHandle(std::uint32_t& handle) noexcept;
    bool preflightDxfWrite() noexcept;
    bool preflightDxfSetup() noexcept;
    void discardDxfWriteResults() noexcept;
    void resetDxfWriteSession();
    class WholeOperationScope;
    class WriteAttemptScope;
    class RecordStateScope;
    class EntityRecordScope;
    bool preflightEntity(const DRW_Entity *ent);
    bool writeEntityReferenceGroups(const DRW_Entity *ent);
    bool preflightTableEntry(const DRW_TableEntry *ent);
    bool resolveSortEntsEntityHandle(std::uint32_t source,
                                     std::uint32_t& resolved) const;
    bool preflightDxfClasses();
    LIBDXFRW_TEST_SEAM_NOINLINE bool validateHatchPayload(const DRW_Hatch *ent) const;
    bool writeEntity(DRW_Entity *ent, bool captureSourceHandle = true,
                     std::uint32_t ownerOverride = DRW::NoHandle);
    bool writeBlockEndWithHandle(const DRW_Block *footer,
                                 std::uint32_t emittedHandle);
    bool writeBlockEndCommonFields(const DRW_Block& footer);
    bool bindSourceBlockBeginHandle(const DRW_Block *block,
                                    std::uint32_t emittedHandle);
    bool bindSourceBlockEndHandle(const DRW_Block *footer,
                                  std::uint32_t emittedHandle);
    bool validateSourceBlockEndOwner(const DRW_Block *footer,
                                     std::uint32_t emittedOwnerHandle);
    bool writeSequenceEnd(const DRW_SeqEnd *footer,
                          std::uint32_t ownerHandle);
    bool materializeImageDefinitionPlan() noexcept;
    DRW_ImageDef *writeImageRecord(DRW_Image *ent, std::string name,
                                   DRW_ImageDef *plannedDefinition);
    bool writeArcDimension(DRW_DimArc *d);
    bool writeLargeRadialDimension(DRW_DimLargeRadial *d);
    bool preflightDimensionStyle(const DRW_Dimension *dimension);
    bool preflightDimensionPictureBlock(const DRW_Dimension *dimension);
    bool preflightInsertTargetBody(const DRW_Insert *insert);
    bool resolveInsertBlockRecordName(
        const DRW_Insert& insert, std::string& emittedName) const noexcept;
    bool recordEmittedDxfBlockBody(const std::string& name);
    bool recordEmittedDxfDimStyleName(const std::string& name);
    bool recordSourceLinetypeTarget(std::uint32_t sourceHandle,
                                    const std::string& name,
                                    std::uint32_t emittedHandle);
    bool recordPendingDxfDimstyleBlockResource(
        const std::string& name, std::uint32_t sourceDimstyleHandle);
    bool completeCurrentDxfBlockBody();
    bool buildDimensionDstyleXdata(
        const DRW_Dimension& dimension,
        std::vector<std::shared_ptr<DRW_Variant>>& output);
    bool writeTables();
    bool writeBlocks();
    bool writeObjects();
    struct DxfDictionaryOutputEntry {
        std::string_view name;
        std::uint32_t target{0};
        int code{350};
    };
    enum class DxfDictionaryRecordPhase { Validate, Emit, ValidateAndEmit };
    bool writeDictionaryRecord(
        const DRW_Dictionary& dictionary, std::uint32_t emittedHandle,
        std::uint32_t emittedOwner, bool emitCloning,
        const std::vector<DxfDictionaryOutputEntry>* projectedEntries = nullptr,
        DxfDictionaryRecordPhase phase = DxfDictionaryRecordPhase::ValidateAndEmit);
    bool writeExtData(const std::vector<DRW_Variant*> &ed);
    /* Entity-flavoured overload: entities own extData via shared_ptr, table
     * records own raw pointers. Same DXF codes, different storage. */
    bool writeExtData(const std::vector<std::shared_ptr<DRW_Variant>> &ed);
    bool writeMTextBackground(DRW_MText *ent);
    bool writeEmbeddedMText(DRW_MText *ent);
    bool writeAttributeXrecord(const DRW_Attrib *ent);
    bool writeAttributeR2018Features(DRW_Attrib *ent);
    /*!< F4 — emit a 330 owner handle for a typed data-only OBJECT (the record's
     * parentHandle when nonzero, else root dict "C" so it is reachable and not
     * pruned as an orphan); no-op pre-R2000 (DXF has no OBJECTS 330 then). */
    void writeObjectOwner(std::uint32_t parentHandle);
    bool writeRawDxfGroups(const std::vector<DRW_Variant> &groups,
                           const std::vector<UTF8STRING> &rawValues,
                           bool hasRawValues,
                           DRW::Version sourceVersion,
                           bool remapSourceHandles = true,
                           bool useLegacyClassifier = false,
                           bool sortEntsTableRecord = false,
                           const std::vector<std::pair<std::size_t, std::string>>*
                               stringOverrides = nullptr,
                           bool allowMaterialUnsignedColorValues = false,
                           const std::vector<std::pair<std::size_t, std::int32_t>>*
                               int32Overrides = nullptr);
    LIBDXFRW_TEST_SEAM_NOINLINE bool writeRawDxfSection(const DRW_RawDxfSection &section);
    bool collectAcdsAsmDataOwnerHandleOverrides(
        const DRW_RawDxfSection& section,
        std::vector<std::pair<std::size_t, std::string>>& overrides);
    void writePlotSettingsFields(const DRW_PlotSettings *ent);
    /*use version from dwgutil.h*/
    std::string toHexStr(std::uint32_t n);
    std::string toHexStr(int n);// compatibility overload for legacy callers
    bool writeAppData(const std::list<std::list<DRW_Variant>> &appData);
    bool writeTableEntryAppData(const DRW_TableEntry& entry, bool emitReactors = true);
    template <typename Range, typename GetPointer>
    bool writeExtDataRange(const Range& values, GetPointer getPointer);
    std::uint32_t resolveReference(std::uint32_t handle) const;
    std::uint32_t remapObjectHandle(std::uint32_t handle) const noexcept;

    bool setError(const DRW::error lastError);
    bool failDxfReadBudget();
    bool failDxfReadRetainedBudget();

    LIBDXFRW_TEST_SEAM_NOINLINE void beginOperationDiagnostic(DRW::OperationKind kind);
#if defined(_MSC_VER) && defined(LIBDXFRW_FORCE_TEST_SEAMS)
private:
#endif
    void recordOperationDiagnostic(DRW::OperationPhase phase,
                                   DRW::OperationCause cause,
                                   const char* code,
                                   const char* message,
                                   std::uint64_t offset = 0,
                                   bool hasOffset = false,
                                   std::uint32_t handle = 0,
                                   bool hasHandle = false,
                                   bool secondary = false);
    void recordOperationDiagnosticForError(DRW::error value);
    void recordInvalidRawHandleDiagnostic();

private:
    bool writeRawDxfGroupsInternal(
        const std::vector<DRW_Variant>& groups,
        const std::vector<UTF8STRING>& rawValues,
        bool hasRawValues, DRW::Version sourceVersion,
        bool remapSourceHandles, bool useLegacyClassifier,
        bool sortEntsTableRecord,
        const std::vector<std::pair<std::size_t, std::string>>*
            stringOverrides,
        bool allowMaterialUnsignedColorValues,
        bool allowVisualStyleUnsignedColorValues,
        const std::vector<std::pair<std::size_t, std::int32_t>>*
            int32Overrides);
    bool writeRawDxfEntityGroups(
        const std::vector<DRW_Variant>& groups,
        const std::vector<UTF8STRING>& rawValues, bool hasRawValues,
        DRW::Version sourceVersion, bool remapSourceHandles,
        bool useLegacyClassifier, bool sortEntsTableRecord,
        int ownerOverrideGroupIndex, std::uint32_t emittedOwnerOverride,
        bool remapEntityXdataHandleReferences,
        const std::vector<std::pair<std::size_t, std::string>>*
            stringOverrides,
        bool allowMaterialUnsignedColorValues,
        const std::vector<std::pair<std::size_t, std::int32_t>>*
            int32Overrides = nullptr,
        bool allowVisualStyleUnsignedColorValues = false);
    bool writeRawDxfObjectInternal(
        DRW_RawDxfObject *obj, std::uint32_t ownerOverride,
        const std::vector<std::pair<std::size_t, std::string>>*
            stringOverrides,
        const std::vector<std::pair<std::size_t, std::int32_t>>*
            int32Overrides = nullptr);
    bool writeXRecordInternal(DRW_XRecord *ent,
                              bool allowDwgGroup1071Compatibility);
    void beginDxfReadRetainedBudget();
    bool appendDxfReadHeaderComment(const std::string& comment);
    bool recordDxfTableRow() noexcept;
    DRW::Version version { DRW::UNKNOWNV };
    DRW::Version m_sourceVersion { DRW::UNKNOWNV };
    DRW::error error {DRW::BAD_NONE};
    bool m_writeError {false};
    std::size_t m_dxfTableRowCount {0};
    bool m_dxfTableRowCountActive {false};
    // Internal compatibility-probe switch. The standalone-safe classifier
    // remains the default for all public read/write calls.
    bool m_useTargetLegacyClassifier {false};
    DRW::DxfTextEncoding m_dxfTextEncoding {DRW::DxfTextEncoding::Declared};
    DRW::DxfCodePagePolicy m_dxfCodePagePolicy {DRW::DxfCodePagePolicy::Compatible};
    std::string fileName;
    std::string codePage;
    bool binFile {false};
    std::size_t m_dxfReadRecordBudget {
        4u * 1024u * 1024u};
    std::size_t m_dxfReadTokenByteBudget {1024u * 1024u};
    std::size_t m_dxfReadInputByteBudget {1024u * 1024u * 1024u};
    std::size_t m_dxfReadDecodeWorkBudget {
        1024u * 1024u * 1024u};
    std::size_t m_dxfReadTypedVariantNodeBudget {1024u * 1024u};
    std::size_t m_dxfReadRetainedByteBudget {1024u * 1024u * 1024u};
    std::shared_ptr<DxfReadRetainedBudget> m_activeDxfReadRetainedBudget;
    std::size_t m_dxfHeaderCommentRetainedBytes {0};
    // Armed only after this operation constructs its reader. Early argument
    // failure must not report a token flag left by a previous interrupted read.
    bool m_readTokenBudgetActive {false};
    bool m_readInputByteBudgetActive {false};
    bool m_readDecodeWorkBudgetActive {false};
    std::unique_ptr<dxfReader> reader;
    std::unique_ptr<dxfWriter> writer;
    DRW_Interface *iface {nullptr};
    DRW_Header header;
//    int section;
    std::string nextentity;
    // A code-0 token may legitimately have an empty value. Keep cursor
    // presence separate from the token text so an empty boundary is an error,
    // not an implicit "no pending record" state.
    bool m_hasPendingEntityBoundary {false};
    bool m_readingBlockEntities {false};
    /// Mints monotonic, collision-free code-5 handles for the DXF write path.
    /// Seeded with the codec's fixed structural literals (seedReservedDxf) plus
    /// every raw-net handle the filter reserves before write(); next() then
    /// skips that whole set, so a minted handle can never duplicate a fixed-low
    /// or preserved-raw handle. Replaces the old `int entCount` + handle floor.
    std::unique_ptr<HandleAllocator> m_handleAllocator;
    bool m_handleReservationFailed {false};
    // A reservation may be requested before write().  Generations distinguish
    // a new failure from the failure already consumed by an earlier attempt,
    // so a codec can be retried without silently hiding a fresh overflow.
    std::uint64_t m_reservationFailureGeneration {0};
    std::uint64_t m_consumedReservationFailureGeneration {0};
    DRW_OperationDiagnostic m_lastDiagnostic;
    struct DxfIdentityDeliveryLedger {
        DxfIdentityDeliveryState state;
        std::size_t maxSections{1024u},maxRecords{100000u},maxCandidates{400000u};
        std::size_t maxBytes{64u*1024u*1024u},maxWork{128u*1024u*1024u};
        std::uint64_t sequence{0};
        DRW_DxfIdentitySource source;
        DxfIdentityDeliveryTicketPtr ticket;
    };
    DxfIdentityDeliveryLedger m_dxfIdentityDelivery;
    void beginDxfIdentityDelivery() noexcept;
    void captureDxfIdentityDelivery(const DRW_DxfIdentitySource&) noexcept;
    struct DxfEntitiesDeliveryLedger {
        struct Scope {DRW_DxfEntitiesSource source;DRW_DxfEntitiesDeliverySource delivery;
            DxfEntitiesDeliveryTicketPtr ticket;bool delivered{false};};
        DxfEntitiesDeliveryState state;
        std::size_t maxScopes{1024u},maxRecords{100000u},maxGroups{4000000u},maxDispatches{100000u},maxEvents{4000000u};
        std::size_t maxBytes{64u*1024u*1024u},maxWork{128u*1024u*1024u};
        std::uint64_t sequence{0};std::vector<Scope> scopes;
        const DRW_DxfEntitiesSource* sourcePublication{nullptr};
        const DRW_DxfEntitiesDeliverySource* manifestPublication{nullptr};
        DxfEntitiesDeliveryTicketPtr activeTicket;
    };
    DxfEntitiesDeliveryLedger m_dxfEntitiesDelivery;
    void beginDxfEntitiesDelivery() noexcept;
    std::size_t captureDxfEntitiesSource(const DRW_DxfEntitiesSource&) noexcept;
    void captureDxfEntitiesManifest(std::size_t,const DRW_DxfEntitiesDeliverySource&) noexcept;
    DxfAcdsSourceLedger m_dxfAcdsSourceLedger;
    const DRW_RawDxfSection* m_dxfAcdsActiveSource{nullptr};
    bool captureDxfAcdsSection(const DRW_RawDxfSection&) noexcept;
    void resetDxfAcdsSourceLedger() noexcept;
    DRW_DxfTextReadObservations m_textReadObservations;
    DRW::DxfTextWritePolicy m_dxfTextWritePolicy{DRW::DxfTextWritePolicy::Compatible};
    bool m_readTextObservationsActive {false};
    bool m_wholeOperationActive {false};
    bool m_dxfWriteAttemptActive {false};
    // XDATA APPID names are gathered during BLOCK_RECORD precollection when
    // available and during emission for streamed callbacks. Missing registrations
    // are rejected as soon as the APPID table closes and again before commit.
    std::unordered_set<std::string> m_registeredDxfXdataAppIds;
    std::unordered_set<std::string> m_requiredDxfXdataAppIds;
    // Closed after emission, including the consumer veto and publication.
    // WholeOperationScope releases this phase on every admitted exit.
    bool m_dxfWriteFinalizing {false};
    bool m_dxfWriteInspecting {false};
    bool m_dxfNoRawSectionsPolicy {false};
    bool m_dxfNoRawSectionsSelected {false};
    bool m_dxfNoRawSectionsObserved {false};

    DxfObjectsWriteInspector m_dxfObjectsWriteInspector;
    DxfEntitySectionsWriteInspector m_dxfEntitySectionsWriteInspector;
    DxfHeaderWriteInspector m_dxfHeaderWriteInspector;
    DxfTablesWriteInspector m_dxfTablesWriteInspector;
    bool m_dxfStagedOutputAuthentication {false};
    using DxfStagedRangeVerifier =
        std::function<bool(std::streamoff, std::string_view)>;
    DxfStagedRangeVerifier m_dxfStagedRangeVerifier;
    bool verifyDxfStagedOutputRange(std::streamoff offset,
                                    std::string_view bytes);
    template <class Inspector>
    bool commitDxfWriteSection(DxfWriterRecordScope& record,
                               Inspector&& inspector);
    bool inspectDxfWriteSection(const DxfObjectsWriteInspector& inspector,
                                std::string_view bytes);
    bool m_dxfWriteRecordInventoryEnabled {false};
    std::size_t m_dxfWriteRecordInventoryLimit {4u * 1024u * 1024u};
    std::vector<DRW_Class> m_dxfClasses;
    bool m_dxfClassesFrozen {false};
    std::vector<DRW_RawDxfSection> m_rawDxfSections;
    std::vector<std::pair<std::string, std::string>> m_rootDictEntries;
    /// Named-dictionary OBJECTS to emit verbatim in writeObjects (DXF path).
    /// Populated via setNamedDictObjects; empty by default so a fresh write is
    /// byte-identical.
    std::vector<DRW_Dictionary> m_namedDictObjects;
    /// TABLESTYLE source handles whose complete raw records are emitted by the
    /// owning adapter during this write.
    std::vector<std::uint32_t> m_resolvedTableStyleHandles;
    /// TABLESTYLE/base-style pairs proven unique in the rooted CELLSTYLEMAP.
    std::vector<std::pair<std::uint32_t, std::int32_t>>
        m_resolvedTableStyleBaseIds;
    /// Background targets validated in the adapter's rooted dictionary graph.
    std::vector<std::uint32_t> m_resolvedBackgroundHandles;
    /// SUN targets whose VPORT/VIEWPORT hard-owner graph was validated by the adapter.
    std::vector<std::uint32_t> m_resolvedSunHandles;
    /// Source linetypes used to attach metadata to canonical mandatory rows.
    std::vector<DRW_LType> m_canonicalLineTypeMetadata;
    /// GROUP objects to typed-emit in writeObjects (DXF path). Populated via
    /// setGroups; empty by default so a fresh write is byte-identical.
    std::vector<DRW_Group> m_groups;
    // Per-attempt destinations, separate from committed object identities.
    // The vector also represents authored GROUPs with no source handle.
    std::vector<std::uint32_t> m_plannedDxfGroupHandles;
    std::map<std::uint32_t, std::uint32_t> m_plannedDxfGroupSourceHandles;
    bool m_dxfGroupPlanFrozen{false};
    std::uint32_t m_generatedNodSourceHandle{0};
    std::uint32_t m_generatedGroupDictionarySourceHandle{0};
    std::unique_ptr<const DRW_Dictionary> m_generatedNodSource;
    std::unique_ptr<const DRW_Dictionary> m_generatedGroupDictionarySource;
    std::unique_ptr<const DRW_Dictionary> m_generatedImageDictionarySource;
    std::uint64_t m_generatedImageDictionaryGeneration{0};
    std::uint32_t m_plannedImageDictionaryHandle{0};
    std::map<std::uint32_t, std::uint32_t> m_plannedImageSourceHandles;
    /// Numeric handle -> replacement handle, applied by writeRawDxfObject to the
    /// few raw objects whose original handle collides with a fixed structural
    /// literal. Empty by default (raw handles emitted verbatim).
    std::map<std::uint32_t, std::uint32_t> m_handleRemap;
    // Consumer-derived BLOCK_RECORD mappings live for one write attempt and
    // never overwrite the caller-owned setHandleRemap map above.
    std::map<std::uint32_t, std::uint32_t> m_dxfWriteHandleRemap;
    // Successful row encoders, separate from forward-reference preallocation.
    // The reverse index prevents two source identities borrowing one built-in.
    std::map<std::uint32_t, std::uint32_t> m_writtenDxfSourceTableHandles;
    std::map<std::uint32_t, std::uint32_t> m_writtenDxfTableSourceHandles;
    std::function<std::uint32_t(std::uint32_t)> m_referenceResolver;
    // Attempted target-version omissions from the most recent admitted write.
    std::map<std::string, std::size_t> m_leftOut;
    bool wlayer0 {false};
    bool dimstyleStd {false};
    bool applyExt {false};
    bool writingBlock {false};
    bool m_collectingBlockRecords {false};
    bool m_dxfWriteBlockRemapPhase {false};
    int elParts {128};  /*!< parts number when convert ellipse to polyline */
    std::unordered_map<std::string,std::uint32_t> blockMap;
    std::vector<PendingDxfBlockRecord> m_pendingBlockRecords;
    std::unordered_set<std::string> m_emittedDxfBlockBodyNames;
    std::unordered_set<std::string> m_emittedDxfDimStyleNames;
    std::unordered_map<std::string, std::uint32_t>
        m_pendingDxfDimensionPictureBlocks;
    std::unordered_map<std::string, std::uint32_t>
        m_pendingDxfInsertBodyTargets;
    std::string m_currentDxfBlockName;
    bool m_dxfBlockSectionActive {false};
    bool m_dxfBlockSectionComplete {false};
    std::vector<DRW_Block_Record> m_sourceBlockRecords;
    std::unordered_set<std::uint32_t> m_writtenDxfFieldSourceHandles;
    std::unique_ptr<DRW_Block> m_fixedModelSpaceBlockBegin;
    std::unique_ptr<DRW_Block> m_fixedPaperSpaceBlockBegin;
    std::unique_ptr<DRW_Block> m_fixedModelSpaceBlockEnd;
    std::unique_ptr<DRW_Block> m_fixedPaperSpaceBlockEnd;
    std::map<std::uint32_t, std::uint32_t> m_sourceBlockRecordHandles;
    bool m_requireResolvedBlockOwners {false};
    std::unordered_map<std::string,std::uint32_t> textStyleMap;
    std::unordered_map<std::uint32_t, std::string>
        m_sourceTextStyleNamesByHandle;
    std::unordered_map<std::uint32_t, std::string>
        m_sourceBlockRecordNamesByHandle;
    std::unordered_map<std::uint32_t, SourceLinetypeTarget>
        m_sourceLinetypeTargetsByHandle;
    std::unordered_map<std::string, std::uint32_t>
        m_pendingDxfDimstyleBlockResources;
    std::vector<DRW_ImageDef*> imageDef;  /*!< imageDef list */
    // Caller-owned configuration survives write-session resets; the runtime
    // pointer map and imageDef vector are rebuilt for every admitted write.
    std::map<ImageDefinitionKey, ImageDefinitionPlanEntry> m_sourceImageDefinitionPlan;
    std::map<const DRW_ImageDef*, const ImageDefinitionPlanEntry*> m_imagePlanSources;
    std::map<const DRW_ImageDef*, std::map<std::uint32_t, const DRW_ImageDefinitionReactor*>> m_imagePlanReactorSources;
    std::map<DRW_ImageDef*, std::map<std::uint32_t, std::uint32_t>> m_regeneratedImageSourceReactors;
    std::map<ImageDefinitionKey, DRW_ImageDef*> m_plannedImageDefinitions;

    // Mutations made while a RecordStateScope is active. Entries are appended
    // before a map/set mutation and removed in reverse order on rollback.
    std::vector<DxfWriteMutation> m_dxfWriteMutations;
    std::size_t m_recordStateScopeDepth {0};

    std::uint32_t currHandle {DRW::NoHandle};

    DRW_ParsingContext m_readingContext;
    /// Whether the first-record position is no longer available to identify
    /// the DXF Named Object Dictionary. A later ownerless DICTIONARY must not
    /// be promoted to root merely because it is the first dictionary parsed.
    bool m_objectsRootPositionPassed {false};
    bool m_dxfViewStyleRootObserved {false};
    DxfViewStyleNameRoute m_detailViewStyleNameRoute;
    DxfViewStyleNameRoute m_sectionViewStyleNameRoute;
    std::list<PendingDxfViewStyleName> m_pendingDxfViewStyleNames;
    DRW_WritingContext m_writingContext;
    // Raw carriers retain source code-5 lexemes up to DWG's 64-bit handle
    // width; keep exact values for duplicate detection during one read.
    std::set<std::uint64_t> m_readRawHandles;
};

#undef LIBDXFRW_TEST_SEAM_NOINLINE

#ifdef LIBDXFRW_LIBDXFRW_RESTORE_NONE
# pragma pop_macro("None")
# undef LIBDXFRW_LIBDXFRW_RESTORE_NONE
#endif

#endif // LIBDXFRW_H
