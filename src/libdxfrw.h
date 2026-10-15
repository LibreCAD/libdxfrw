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

#include <functional>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
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
    /** Bound typed DXF entity and `DRW_TableEntry::parseCode` XDATA
     * `DRW_Variant` nodes admitted by each subsequent read. This also covers
     * modeled object records that delegate XDATA to that base parser; it is
     * not a guarantee over every OBJECTS parser/raw carrier. The default is
     * 1,048,576 nodes; it is caller-raiseable and not a DXF maximum.
     * Application data, raw carriers and other typed containers remain
     * outside this counter. Callback changes apply to the next read. Rebuild
     * consumers using this API. */
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
    bool writeXRecord(DRW_XRecord *ent);
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
    bool writeBlock(DRW_Block *ent);
    bool writeInsert(DRW_Insert *ent);
    bool writeTable(DRW_Table *ent);
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
    bool writeEvaluationGraph(DRW_EvaluationGraph *ent,
                              const char *recordName = "EVALUATION_GRAPH");
    bool writeRawDxfObject(DRW_RawDxfObject *obj);
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
    /*!< Register unmodeled DXF sections for same-version raw replay. */
    void setRawDxfSections(const std::vector<DRW_RawDxfSection> &sections) {
        if (!preflightDxfSetup())
            return;
        m_rawDxfSections = sections;
    }
    /*!< Canonical DXF CLASS metadata (recName/className/appName/proxyFlag/
     * wasaProxyFlag/entityFlag, per ezdxf REQUIRED_CLASSES) for the known
     * custom-class OBJECTS that the raw net round-trips. Returns false for
     * fixed/built-in or unknown record names. instanceCount is left 0 for the
     * caller to fill. */
    static bool dxfClassForRecordName(const std::string &recName, DRW_Class &out);
    /*!< Register extra (name, hex-handle) entries to splice into the regenerated
     * root NamedObjectsDictionary (handle C) so raw-net-routed named dictionaries
     * are reachable from the root and not pruned as orphans. The filter builds
     * this from the source root dict before write(); empty by default. */
    void setRootDictEntries(const std::vector<std::pair<std::string, std::string>> &entries) {
        if (!preflightDxfSetup())
            return;
        m_rootDictEntries = entries;
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
        m_namedDictObjects = dicts;
    }
    /*!< Register GROUP objects to typed-emit in the regenerated OBJECTS section
     * (DXF write path only). Each carries its name/description/flags and the
     * member entity SOURCE handles (DRW_Group::m_entityHandles). The codec mints
     * a fresh group handle, injects the (name, minted-handle) entry into the
     * ACAD_GROUP D dict, and emits the GROUP with 340 references resolved through
     * the writeEntity source->minted map (members absent from the map — consumed
     * or filtered entities — are skipped, never emitted as a dangling 340).
     * Empty by default. */
    void setGroups(const std::vector<DRW_Group> &groups) {
        if (!preflightDxfSetup())
            return;
        m_groups = groups;
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
     * Empty by default (source handles are preserved verbatim). */
    void setHandleRemap(const std::map<std::uint32_t, std::uint32_t> &remap) {
        if (!preflightDxfSetup())
            return;
        m_handleRemap = remap;
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
    //! Strict mode rejects an explicit non-null entity owner without a binding.
    //! Legacy writers without this metadata retain their existing fallback.
    void setSourceBlockRecords(const std::vector<DRW_Block_Record>& records,
                               bool requireResolvedOwners = false) {
        if (!preflightDxfSetup())
            return;
        m_sourceBlockRecords = records;
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
    //! Bind a preserved raw record's source handle to the handle emitted for it.
    //! The binding is rejected for null or ambiguous source identities.
    bool bindSourceEntityHandle(std::uint32_t sourceHandle,
                                std::uint32_t emittedHandle);
    //! Mark a source handle as ambiguous before entity emission. Deferred
    //! references must not guess which of multiple source entities it names.
    void markSourceHandleAmbiguous(std::uint32_t sourceHandle);

    DRW::Version getVersion() const;
    //! Accepted declared DXF source revision, not getVersion()'s codec version.
    //! UNKNOWNV until parsing returns, also after a callback throws. A returned
    //! parse failure retains an accepted declaration; missing/unknown metadata
    //! remains UNKNOWNV. A new file read clears it even on argument/open failure;
    //! readAscii(nullptr) and rejected nesting preserve the previous snapshot.
    //! Writes leave this last-read snapshot unchanged. Consumer rebuild required.
    DRW::Version getSourceVersion() const;
    DRW::error getError() const;
    /// Snapshot the first failure and bounded cleanup evidence from the most
    /// recent read/write operation.  The legacy getError() value remains the
    /// compatibility channel.
    DRW_OperationDiagnostic getLastDiagnostic() const;

    std::uint32_t getBlockRecordHandleToWrite(const std::string& blockName) const;
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
    bool processRawDxfSection(const std::string& sectionName);
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
    bool processAcShHistoryObject();

    DxfEntityBoundary readEntityBoundary();
    DxfEntityBoundary setEntityBoundary(int code);
    DxfEntityBoundary classifyEntityBoundary() const;
    bool acceptEntityCallbackBoundary() const;
    bool acceptObjectBoundary(int code);
    DxfEntityBoundary consumeEntityFooter();

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
    bool processRawObject();
    bool processRawEntity();
    /*!< Append the current DXF record (already read by reader->readRec) to a
     * raw-passthrough carrier as a correctly-TYPED DRW_Variant, classifying the
     * value by DXF code range (numeric codes leave reader->strData stale AND
     * clobber reader->type to STRING, so neither getString() nor type can be
     * trusted for them — that was the A1/A4 capture bug). ASCII-DXF only. Also
     * latches code 5 -> handle and code 330 -> parentHandle. */
    LIBDXFRW_TEST_SEAM_NOINLINE bool captureRawGroup(DRW_RawDxfObject &obj, int code,
                         bool validateHandles = false,
                         bool selfHandle = true);
    bool captureRawDxfApplicationGroup(DRW_RawDxfObject &obj,
                                       std::list<std::list<DRW_Variant>> &appData,
                                       std::vector<std::uint32_t> &reactorHandles,
                                       std::uint32_t &xDictHandle);
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
    };
    enum class DxfWriteMutationKind : std::uint8_t {
        BlockMapInsert,
        TextStyleMapSet,
        SourceHandleInsert,
        SourceHandleErase,
        AmbiguousSourceHandleInsert,
        WrittenSourceEntityHandleInsert,
        ModelerSourceHandleInsert,
        ImageReactorInsert
    };
    struct DxfWriteMutation {
        DxfWriteMutationKind kind {DxfWriteMutationKind::BlockMapInsert};
        std::string key;
        std::uint32_t handle {0};
        std::uint32_t previousHandle {0};
        bool hadPrevious {false};
        DRW_ImageDef *imageDef {nullptr};
    };
    bool emitBlockRecord(const PendingDxfBlockRecord& record);
    bool planSourceBlockRecordHandles();
    // captureSourceHandle=false on the VERTEX/SEQEND parent re-entries
    // (writePolyline/writeInsert) so they do not pollute the source->minted map.
    // recordName is supplied only for a known target-version limitation; the
    // attempted omission is counted, but the standalone writer remains strict.
    bool rejectUnsupportedDxfWrite(const char* recordName = nullptr) noexcept;
    bool failDxfWrite() noexcept;
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
    bool preflightTableEntry(const DRW_TableEntry *ent);
    bool resolveSortEntsEntityHandle(std::uint32_t source,
                                     std::uint32_t& resolved) const;
    bool preflightDxfClasses();
    LIBDXFRW_TEST_SEAM_NOINLINE bool validateHatchPayload(const DRW_Hatch *ent) const;
    bool writeEntity(DRW_Entity *ent, bool captureSourceHandle = true,
                     std::uint32_t ownerOverride = DRW::NoHandle);
    bool writeSequenceEnd(std::uint32_t ownerHandle);
    bool writeArcDimension(DRW_DimArc *d);
    bool writeLargeRadialDimension(DRW_DimLargeRadial *d);
    bool writeTables();
    bool writeBlocks();
    bool writeObjects();
    bool writeExtData(const std::vector<DRW_Variant*> &ed);
    /* Entity-flavoured overload: entities own extData via shared_ptr, table
     * records own raw pointers. Same DXF codes, different storage. */
    bool writeExtData(const std::vector<std::shared_ptr<DRW_Variant>> &ed);
    bool writeMTextBackground(DRW_MText *ent);
    bool writeEmbeddedMText(DRW_MText *ent);
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
                           bool sortEntsTableRecord = false);
    LIBDXFRW_TEST_SEAM_NOINLINE bool writeRawDxfSection(const DRW_RawDxfSection &section);
    bool remapAcdsAsmDataOwnerHandles(DRW_RawDxfSection &section);
    void writePlotSettingsFields(const DRW_PlotSettings *ent);
    /*use version from dwgutil.h*/
    std::string toHexStr(std::uint32_t n);
    std::string toHexStr(int n);// compatibility overload for legacy callers
    bool writeAppData(const std::list<std::list<DRW_Variant>> &appData);
    bool writeTableEntryAppData(const DRW_TableEntry& entry);
    std::uint32_t resolveReference(std::uint32_t handle) const;
    std::list<std::list<DRW_Variant>> resolveAppDataReferences(
        const std::list<std::list<DRW_Variant>> &appData);
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
    void beginDxfReadRetainedBudget();
    bool appendDxfReadHeaderComment(const std::string& comment);
    DRW::Version version { DRW::UNKNOWNV };
    DRW::Version m_sourceVersion { DRW::UNKNOWNV };
    DRW::error error {DRW::BAD_NONE};
    bool m_writeError {false};
    // Internal compatibility-probe switch. The standalone-safe classifier
    // remains the default for all public read/write calls.
    bool m_useTargetLegacyClassifier {false};
    DRW::DxfTextEncoding m_dxfTextEncoding {DRW::DxfTextEncoding::Declared};
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
    bool m_wholeOperationActive {false};
    bool m_dxfWriteAttemptActive {false};
    // Closed after emission, including the consumer veto and publication.
    // WholeOperationScope releases this phase on every admitted exit.
    bool m_dxfWriteFinalizing {false};
    std::vector<DRW_Class> m_dxfClasses;
    bool m_dxfClassesFrozen {false};
    std::vector<DRW_RawDxfSection> m_rawDxfSections;
    std::vector<std::pair<std::string, std::string>> m_rootDictEntries;
    /// Named-dictionary OBJECTS to emit verbatim in writeObjects (DXF path).
    /// Populated via setNamedDictObjects; empty by default so a fresh write is
    /// byte-identical.
    std::vector<DRW_Dictionary> m_namedDictObjects;
    /// Source linetypes used to attach metadata to canonical mandatory rows.
    std::vector<DRW_LType> m_canonicalLineTypeMetadata;
    /// GROUP objects to typed-emit in writeObjects (DXF path). Populated via
    /// setGroups; empty by default so a fresh write is byte-identical.
    std::vector<DRW_Group> m_groups;
    /// Numeric handle -> replacement handle, applied by writeRawDxfObject to the
    /// few raw objects whose original handle collides with a fixed structural
    /// literal. Empty by default (raw handles emitted verbatim).
    std::map<std::uint32_t, std::uint32_t> m_handleRemap;
    // Consumer-derived BLOCK_RECORD mappings live for one write attempt and
    // never overwrite the caller-owned setHandleRemap map above.
    std::map<std::uint32_t, std::uint32_t> m_dxfWriteHandleRemap;
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
    std::vector<DRW_Block_Record> m_sourceBlockRecords;
    std::map<std::uint32_t, std::uint32_t> m_sourceBlockRecordHandles;
    bool m_requireResolvedBlockOwners {false};
    std::unordered_map<std::string,std::uint32_t> textStyleMap;
    std::vector<DRW_ImageDef*> imageDef;  /*!< imageDef list */

    // Mutations made while a RecordStateScope is active. Entries are appended
    // before a map/set mutation and removed in reverse order on rollback.
    std::vector<DxfWriteMutation> m_dxfWriteMutations;
    std::size_t m_recordStateScopeDepth {0};

    std::uint32_t currHandle {DRW::NoHandle};

    DRW_ParsingContext m_readingContext;
    /// Whether the OBJECTS section being read has already yielded a
    /// DICTIONARY. The DXF specification puts the named object dictionary
    /// first, which is one of the two tests that tell the root apart from a
    /// named dictionary that simply omits its owner group.
    bool m_objectsDictionarySeen {false};
    DRW_WritingContext m_writingContext;
    // Raw carriers retain source code-5 lexemes up to DWG's 64-bit handle
    // width; keep exact values for duplicate detection during one read.
    std::set<std::uint64_t> m_readRawHandles;
};

#undef LIBDXFRW_TEST_SEAM_NOINLINE

#endif // LIBDXFRW_H
