/******************************************************************************
**  dwg2dxf - Program to convert dwg/dxf to dxf(ascii & binary)              **
**                                                                           **
**  Copyright (C) 2015 José F. Soriano, rallazz@gmail.com                    **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DX_DATA_H
#define DX_DATA_H
#include "libdxfrw.h"
#include "libdwgr.h"
#include "dx_raster_dwg_wire.h"
#include "dx_dwg_class_evidence.h"
#include "intern/dxftablereferences.h"
#include "intern/dxfattdef.h"
#include "intern/dxfcompound.h"
#include "intern/dxftable.h"
#include "intern/dxflayout.h"
#include "dx_attdef_source_profile.h"
#include "dx_block_source_profile.h"
#include "dx_block_membership.h"
#include "dx_compound_source_profile.h"
#include "dx_table_source_profile.h"
#include "dx_layout_source_profile.h"
#include "dx_viewucs_source_profile.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <list>
#include <limits>
#include <map>
#include <set>
#include <utility>
#include <vector>

// CLI-owned metadata prefixes. Full physical declaration bytes remain in the
// delivered headerC capsule; a prefix is never a complete declaration identity.
struct dx_dxfTextPrefix {
    std::array<char, 64> bytes{};
    std::size_t length{0};
    std::size_t sourceBytes{0};
    bool truncated{false};
};

enum class dx_dxfCodePageKind : std::uint8_t {
    Unspecified, Canonical, ExactAlias, CompatibilityAlias, Unavailable, Unknown
};

// Source-text admission is independent of owner resolution, target transport
// and whole-drawing completeness. Only the CLI opts in by default.
enum class dx_dxfSourceTextPolicy : std::uint8_t { Compatible, RequireValidSource };
// The generic writer replays only explicitly acknowledged, same-profile,
// complete primitive streams. Unsupported caches keep the compatibility
// drop behavior; strict complete conversions fail before publication.
enum class dx_dxfProxyGraphicsPolicy : std::uint8_t {
    Compatible,
    FailIfNotPreserved
};
// Read-result completeness is independent of owner closure. Legacy preserves
// the existing coupling to requireResolvedOwners; explicit partial mode may
// waive only semantic read completeness after bounded frame finalization.
enum class dx_dwgReadCompletenessPolicy : std::uint8_t {
    Legacy,
    RequireComplete,
    AllowPartial
};
enum class dx_dxfSourceTextStatus : std::uint8_t {
    NotChecked, NotApplicable, Admitted, ImportNotSuccessful,
    EvidenceUnavailable, ObservedIssues, UnsupportedProfile, UnknownRevision
};
struct dx_dxfSourceTextAdmission {
    dx_dxfSourceTextStatus status{dx_dxfSourceTextStatus::NotChecked};
    std::uint32_t issueMask{0};
    bool admitted() const noexcept {
        return status == dx_dxfSourceTextStatus::Admitted
            || status == dx_dxfSourceTextStatus::NotApplicable;
    }
};

// One DXF import attempt, not a charset/support or export-completeness verdict.
// Final HEADER metadata describes only the last delivered header; cumulative
// observations preserve earlier semantic decode events independently.
struct dx_dxfReadOutcome {
    bool attempted{false};
    bool readReturned{false};
    bool readThrew{false};
    bool operationSucceeded{false};
    DRW::Version sourceVersion{DRW::UNKNOWNV}; // accepted, not raw HEADER spelling
    DRW::Version codecVersion{DRW::UNKNOWNV}; // distinct compatibility channel
    DRW::error error{DRW::BAD_NONE};
    DRW_DxfTextReadObservations observations;
    bool headerDelivered{false};
    bool hasCodePageDeclaration{false};
    dx_dxfCodePageKind finalDeclarationKind{dx_dxfCodePageKind::Unspecified};
    dx_dxfTextPrefix finalDeclaration;
    // Classifier's compatibility selection, not an actual per-field decoder.
    // Absent declarations leave this empty; modern DXF can ignore it entirely.
    dx_dxfTextPrefix finalDeclarationSelection;
    dx_dxfTextPrefix finalNormalizedCodePage;
    // Read diagnostics contain only fixed implementation strings and at most
    // 16 secondary rows (no source names/payloads). Copying can still allocate;
    // failure is explicit and never changes the read Boolean/original throw.
    bool diagnosticAvailable{false};
    bool diagnosticCaptureFailed{false};
    DRW_OperationDiagnostic diagnostic;
    // Request exists even if setup fails before read entry. Applied policy is
    // observations.codePagePolicy and is meaningful with the read status.
    DRW::DxfCodePagePolicy requestedCodePagePolicy{DRW::DxfCodePagePolicy::Compatible};
};

// Read result and observations have separate meanings. This envelope belongs
// to one CLI import, not a support claim or an output-completeness verdict.
struct dx_dwgReadOutcome {
    bool attempted{false};
    bool operationSucceeded{false};
    DRW::Version sourceVersion{DRW::UNKNOWNV};
    DRW::error error{DRW::BAD_NONE};
    std::uint16_t sourceCodePageId{0};
    bool sourceCodePageFieldPresent{false};
    bool sourceCodePageRecognized{false};
    DwgReadObservations observations;
    // Receipt callbacks precede some companion callbacks and final ledger
    // marking. These are observed deliveries, not unique terminal counts.
    std::array<std::size_t, 4> observedReceiptCarriers{};
    std::size_t opaqueViewCallbacks{0};
    std::size_t opaqueFramesRetainedByAdapter{0};
    // Standard-object bytes and source receipts are a separate staging lane.
    // They do not contribute to the opaque-retention completeness counter
    // above until their typed/output handoff has been validated.
    std::size_t standardObjectFramesCaptured{0};
    std::size_t standardObjectSourceReceiptsMatched{0};
    bool standardObjectCaptureFailed{false};
    // Reparse/callback equality only, never serializer or semantic evidence.
    std::size_t standardObjectTypedPayloadsMatched{0};
    std::size_t standardObjectOpaquePayloadsMatched{0};
    bool standardObjectPayloadValidationFailed{false};
    bool callbackCountsSaturated{false};

    bool hasUnsupportedSourceCodePage() const noexcept {
        return sourceCodePageFieldPresent && !sourceCodePageRecognized
            && sourceCodePageId != 0 && sourceCodePageId != 0x00ffu;
    }
};

//class to store image data and path from DRW_ImageDef
class dx_ifaceImg : public DRW_Image {
public:
    dx_ifaceImg(){}
    dx_ifaceImg(const DRW_Image& p):DRW_Image(p){}
    ~dx_ifaceImg(){}
    std::string path; //stores the image path
    // Prevent a later read into a reused dx_data from binding this image to
    // an identically numbered definition from an earlier source file.
    std::uint64_t sourceImportGeneration{0};
    // Bytes charged when linkImage copies the delayed IMAGEDEF path into this
    // retained entity; the path is populated after addImage returns.
    std::size_t retainedPathBytes{0};
};

struct dx_ifaceRawEntityEvent {
    std::size_t typedEntityIndex {0};
    DRW_RawDxfObject record;
};

//container class to store entites.
class dx_ifaceBlock : public DRW_Block {
public:
    dx_ifaceBlock(){}
    dx_ifaceBlock(const DRW_Block& p):DRW_Block(p){}
    ~dx_ifaceBlock(){
        for (std::list<DRW_Entity*>::const_iterator it=ent.begin(); it!=ent.end(); ++it)
            delete *it;
    }
    std::list<DRW_Entity*>ent; //stores the entities list
    DRW_Block endBlockData;
    bool hasEndBlockData{false};
    // Keep opaque entity callbacks interleaved with typed callbacks. The index
    // names the number of typed records already delivered when the raw record
    // arrived, preserving physical record order during same-version replay.
    std::vector<dx_ifaceRawEntityEvent> rawEntities;
};

enum class dx_source_format : std::uint8_t {
    Unknown,
    DXF,
    DWG
};

// Converter-private source families. These are callback/model identities,
// never binary DWG type numbers or output/support qualification.
enum class dx_tableResourceFamily : std::uint8_t {
    TextStyle, LineType, TableStyle, CellStyleMap, Field, DataLink,
    BlockRecord, AppId, Layer, Dictionary, XRecord, AttributeDefinition,
    Entity, Table, TableContent, TableGeometry, Layout, View, Ucs
};

enum class dx_tableResourceRole : std::uint8_t {
    Self, Owner, Reactor, ExtensionDictionary, TextStyle, LineType,
    LocalStyleDefinition, XdataObjectId, AppIdName, LayerName, Unknown,
    TextStyleName, LTypeTextStyle, LTypeShapeStyle,
    FieldChild, FieldObject, FieldValueObject, DataLinkCustomTarget, DataLinkHardOwner,
    DictionarySoftOwner, DictionaryHardOwner, XRecordHandle, XRecordSoftPointer,
    XRecordHardPointer, XRecordSoftOwner, XRecordHardOwner, BlockInsert, BlockLayout,
    LineTypeName, LayerMaterial, LayerPlotStyle,
    TableStyle, TableGeometryBlock, TableCellBlock, TableCellField,
    TableCellAttributeDefinition, TableValueObject, BlockName, Material, PlotStyle, VisualStyle, LayoutName, DictionaryDefault,
    LayoutBlockRecord,LayoutViewport,LayoutNamedUcs,LayoutBaseUcs,LayoutShadePlot,PlotViewName,ViewBackground,ViewLiveSection,ViewSun,ViewportFrozenLayer,ViewportClip,ViewportShade
};
struct dx_tableResourceReference {
    dx_tableResourceRole kind;
    std::size_t groupIndex;
    int code;
    std::uint32_t value;
};

// Binds the visual-style records appended during one successful import to
// that import's source profile. This is provenance only: it does not certify
// the decoded payload or make an unsupported profile writable.
struct dx_visualStyleImportReceipt {
    std::uint64_t generation{0};
    dx_source_format sourceFormat{dx_source_format::Unknown};
    DRW::Version sourceVersion{DRW::UNKNOWNV};
    std::size_t visualStyleBegin{0};
    std::size_t visualStyleEnd{0};
    std::size_t rawVisualStyleBegin{0};
    std::size_t rawVisualStyleEnd{0};
    std::size_t rawGroupCount{0};
    std::size_t rawRetainedBytes{0};
    std::size_t typedRetainedRecords{0};
    std::size_t typedRetainedBytes{0};
    bool typedResourceLimitExceeded{false};
    bool typedCaptureFailed{false};
    bool resourceLimitExceeded{false};
    bool captureFailed{false};
    bool successful{false};
};

struct dx_importedRecordRange {
    std::size_t begin{0};
    std::size_t end{0};
};

// Binds FIELD/FIELDLIST typed callbacks to their raw DXF tag carriers from
// one import. This receipt proves only record identity/owner pairing, not
// reference closure, wire semantics, or serializer readiness.
struct dx_fieldCarrierImportReceipt {
    std::uint64_t generation{0};
    dx_source_format sourceFormat{dx_source_format::Unknown};
    DRW::Version sourceVersion{DRW::UNKNOWNV};
    dx_importedRecordRange fieldRange;
    dx_importedRecordRange fieldListRange;
    dx_importedRecordRange rawFieldRange;
    dx_importedRecordRange rawFieldListRange;
    std::size_t typedCallbacks{0};
    std::size_t typedItemCount{0};
    std::size_t typedRetainedBytes{0};
    bool typedResourceLimitExceeded{false};
    bool typedCaptureFailed{false};
    std::size_t rawCallbacks{0};
    std::size_t rawGroupCount{0};
    std::size_t retainedBytes{0};
    bool resourceLimitExceeded{false};
    bool captureFailed{false};
    bool successful{false};
};

// Binds associative DXF callback pairs and retained payloads to one import
// generation. Capture completeness is distinct from serializer qualification.
struct dx_associativeDxfImportReceipt {
    std::uint64_t generation{0};
    dx_source_format sourceFormat{dx_source_format::Unknown};
    DRW::Version sourceVersion{DRW::UNKNOWNV};
    dx_importedRecordRange typedRange;
    dx_importedRecordRange rawRange;
    dx_importedRecordRange dictionaryRange;
    bool namedObjectDictionaryCallbackSeen{false};
    bool namedObjectDictionaryCallbackAmbiguous{false};
    std::size_t typedCallbacks{0};
    std::size_t rawCallbacks{0};
    std::size_t rawGroupCount{0};
    // Count of generic DXF handle-code positions in retained raw groups.
    // Associative class grammar has not established their field meaning, so
    // these remain unclassified and are never treated as graph edges.
    std::size_t unclassifiedHandleCodeGroups{0};
    // Number of associative records whose owner chain was proven to reach the
    // position-identified NOD through retained DICTIONARY/associative nodes.
    // This does not prove class-specific reachability or reference closure.
    std::size_t rootedOwnerChains{0};
    std::size_t retainedBytes{0};
    std::size_t retainedRecords{0};
    std::size_t qualifiedRecords{0};
    bool resourceLimitExceeded{false};
    bool captureFailed{false};
    bool captureComplete{false};
    bool handleCodeScanComplete{false};
    bool ownerChainScanComplete{false};
    bool successful{false};
};

// Import provenance for the four named-resource records whose raw DXF
// companions are retained by this adapter. Payload/profile qualification is a
// separate gate and is intentionally not represented here.
struct dx_namedResourceImportReceipt {
    std::uint64_t generation{0};
    dx_source_format sourceFormat{dx_source_format::Unknown};
    DRW::Version sourceVersion{DRW::UNKNOWNV};
    dx_importedRecordRange scaleRange;
    dx_importedRecordRange dictionaryVarRange;
    dx_importedRecordRange mLineStyleRange;
    dx_importedRecordRange mLeaderStyleRange;
    dx_importedRecordRange rawRange;
    std::size_t typedRetainedRecords{0};
    std::size_t typedRetainedBytes{0};
    bool typedResourceLimitExceeded{false};
    bool typedCaptureFailed{false};
    std::size_t rawCallbacks{0};
    std::size_t rawGroupCount{0};
    std::size_t retainedBytes{0};
    bool resourceLimitExceeded{false};
    bool captureFailed{false};
    bool successful{false};
};

// Binds MATERIAL's typed projection to its complete ordered DXF tag carrier
// for one import generation. This proves capture identity/owner pairing only;
// dictionary closure and payload replay remain separate gates.
struct dx_materialImportReceipt {
    std::uint64_t generation{0};
    dx_source_format sourceFormat{dx_source_format::Unknown};
    DRW::Version sourceVersion{DRW::UNKNOWNV};
    dx_importedRecordRange typedRange;
    dx_importedRecordRange rawRange;
    std::size_t rawCallbacks{0};
    std::size_t rawGroupCount{0};
    std::size_t retainedBytes{0};
    bool resourceLimitExceeded{false};
    bool captureFailed{false};
    bool successful{false};
};

// CLI-only raw ACSH/EvaluationGraph capture; retained bytes are not serializer
// or semantic qualification. One successful source generation is required.
struct dx_acdsHistoryDxfImportReceipt {
    std::uint64_t generation{0};
    DRW::Version version{DRW::UNKNOWNV};
    std::size_t callbacks{0}, historyCallbacks{0}, groups{0}, bytes{0};
    bool failed{false}, successful{false};
};

// Binds the typed model-document view-style callbacks to their complete
// ordered DXF carriers for one import. This proves capture pairing only; the
// styles' resource edges and any writer route remain separate gates.
struct dx_modelDocViewStyleImportReceipt {
    std::uint64_t generation{0};
    dx_source_format sourceFormat{dx_source_format::Unknown};
    DRW::Version sourceVersion{DRW::UNKNOWNV};
    dx_importedRecordRange detailRange;
    dx_importedRecordRange sectionRange;
    dx_importedRecordRange rawDetailRange;
    dx_importedRecordRange rawSectionRange;
    dx_importedRecordRange dictionaryRange;
    std::size_t typedCallbacks{0};
    std::size_t typedItemCount{0};
    std::size_t typedRetainedBytes{0};
    bool typedResourceLimitExceeded{false};
    bool typedCaptureFailed{false};
    // DWG style names are stored on the owning NOD child-dictionary edge,
    // not in the typed style object body. True only after the post-read join
    // completed without ambiguous or wrong-owner edges.
    bool nameAssociationsResolved{false};
    std::size_t rawCallbacks{0};
    std::size_t rawGroupCount{0};
    std::size_t retainedBytes{0};
    bool resourceLimitExceeded{false};
    bool captureFailed{false};
    bool successful{false};
};

// Binds typed DICTIONARYWDFLT/PLACEHOLDER callbacks and retained DXF raw
// companions to one import generation. This proves origin and identity
// pairing only; root reachability and output closure are separate gates.
struct dx_dictionaryDefaultImportReceipt {
    std::uint64_t generation{0};
    dx_source_format sourceFormat{dx_source_format::Unknown};
    DRW::Version sourceVersion{DRW::UNKNOWNV};
    dx_importedRecordRange dictionaryWithDefaultRange;
    dx_importedRecordRange placeholderRange;
    dx_importedRecordRange rawDictionaryWithDefaultRange;
    dx_importedRecordRange rawPlaceholderRange;
    std::size_t rawCallbacks{0};
    std::size_t rawGroupCount{0};
    std::size_t retainedBytes{0};
    bool resourceLimitExceeded{false};
    bool captureFailed{false};
    bool successful{false};
};

// Captures typed SPATIAL_FILTER callbacks and, for DXF input, their complete
// ordered raw companions. This receipt binds provenance and record identity;
// owner-graph closure and export support are separate gates.
struct dx_spatialFilterImportReceipt {
    std::uint64_t generation{0};
    dx_source_format sourceFormat{dx_source_format::Unknown};
    DRW::Version sourceVersion{DRW::UNKNOWNV};
    dx_importedRecordRange typedRange;
    dx_importedRecordRange rawRange;
    dx_importedRecordRange dictionaryRange;
    std::size_t typedCallbacks{0};
    std::size_t rawCallbacks{0};
    std::size_t rawGroupCount{0};
    std::size_t retainedBytes{0};
    bool resourceLimitExceeded{false};
    bool captureFailed{false};
    bool captureComplete{false};
    bool ownerGraphComplete{false};
    std::string ownerGraphFailure;
    bool successful{false};
};

//container class to store full dwg/dxf data.
class dx_data {
public:
    dx_data(){
        sourceVersion = DRW::UNKNOWNV;
        mBlock = new dx_ifaceBlock();
    }
    ~dx_data(){
        //cleanup,
        for (std::list<dx_ifaceBlock*>::const_iterator it=blocks.begin(); it!=blocks.end(); ++it)
            delete *it;
        delete mBlock;
    }

    DRW_Header headerC;                 //stores a copy of the header vars
    DRW::Version sourceVersion;          //detected source DXF/DWG revision
    dx_source_format sourceFormat {dx_source_format::Unknown};
    // Monotonic per-container import attempt identity. Zero means no attempt.
    std::uint64_t importGeneration{0};
    dx_dxfReadOutcome dxfReadOutcome;     //owned most-recent DXF import evidence
    dx_dwgReadOutcome dwgReadOutcome;     //bounded most-recent-import evidence
    // Container-lifetime adapter admission counters. Typed entity and block
    // callback copies use conservative aggregate retained-byte estimates.
    std::size_t typedEntityRetainedRecords{0};
    std::size_t typedEntityRetainedBytes{0};
    std::size_t typedBlockRetainedBytes{0};
    std::size_t typedTableRetainedRecords{0};
    std::size_t typedTableRetainedBytes{0};
    bool typedEntityCaptureFailed{false};
    dx_associativeDxfImportReceipt associativeDxfImport;
    dx_visualStyleImportReceipt visualStyleImport;
    dx_namedResourceImportReceipt namedResourceImport;
    dx_materialImportReceipt materialImport;
    dx_modelDocViewStyleImportReceipt modelDocViewStyleImport;
    dx_dictionaryDefaultImportReceipt dictionaryDefaultImport;
    dx_fieldCarrierImportReceipt fieldCarrierImport;
    dx_spatialFilterImportReceipt spatialFilterImport;
    std::list<DRW_LType>lineTypes;      //stores a copy of all line types
    std::list<DRW_Layer>layers;         //stores a copy of all layers
    std::list<DRW_Dimstyle>dimStyles;   //stores a copy of all dimension styles
    std::list<DRW_Vport>VPorts;         //stores a copy of all vports
    std::list<DRW_View>views;            //stored views and visual-style links
    std::list<DRW_UCS>ucsList;          //named UCS table records
    std::list<DRW_Textstyle>textStyles; //stores a copy of all text styles
    std::list<DRW_AppId>appIds;         //stores a copy of all line types
    std::vector<DRW_Block_Record> blockRecords; //owned source identity/metadata
    std::vector<DRW_Layout> layouts; //typed DXF LAYOUT graph records
    // Standalone OBJECTS PLOTSETTINGS records. Keep distinct from the
    // AcDbPlotSettings prefix embedded in each LAYOUT; export requires the
    // ACAD_PLOTSETTINGS owner graph and a qualified null-shade-reference path.
    std::list<DRW_PlotSettings> plotSettings;
    // Adapter-owned standalone PLOTSETTINGS callback copies are bounded
    // independently of Layout's embedded plot prefix.
    std::size_t plotSettingsRetainedRecords{0};
    std::size_t plotSettingsRetainedBytes{0};
    bool plotSettingsCaptureFailed{false};
    // Render-settings callbacks are paired with raw positional DXF carriers;
    // export only replays complete rooted graphs at the exact source version.
    std::list<DRW_RenderSettings> renderSettings;
    std::list<DRW_RawDxfObject> rawRenderSettingsObjects;
    // Adapter-owned typed RenderSettings copies are budgeted independently
    // from their raw DXF companion carriers.
    std::size_t renderSettingsRetainedRecords{0};
    std::size_t renderSettingsRetainedValues{0};
    std::size_t renderSettingsRetainedBytes{0};
    bool renderSettingsCaptureFailed{false};
    // Background and SUN callback data are retained separately from their raw
    // DXF carriers. Same-version Background records can be replayed only
    // through a validated ACAD_BACKGROUND dictionary; VIEW/VPORT/VIEWPORT
    // references require rooted, remapped targets and viewport non-sharing.
    // IBL group-340 edges admit only uniquely owned rooted Solid/Gradient/Image
    // targets. Skylight-to-SUN and common reactor/extension-dictionary edges
    // remain fail-closed.
    std::list<DRW_Background> backgrounds;
    std::list<DRW_Sun> suns;
    std::list<DRW_RawDxfObject> rawBackgroundObjects;
    std::list<DRW_RawDxfObject> rawSunObjects;
    // Aggregate admission for the raw RenderSettings/Background/SUN DXF
    // companions. These counters and the failure flag persist across imports
    // appended to this container; typed graph validation remains separate.
    std::size_t rawRenderCompanionRetainedRecords{0};
    std::size_t rawRenderCompanionRawGroups{0};
    std::size_t rawRenderCompanionRetainedBytes{0};
    bool rawRenderCompanionCaptureFailed{false};
    // Typed Background/SUN callback copies have a separate aggregate budget
    // from their raw DXF companions and from typed RenderSettings values.
    std::size_t backgroundSunTypedRetainedRecords{0};
    std::size_t backgroundSunTypedRetainedBytes{0};
    bool backgroundSunTypedCaptureFailed{false};
    std::list<DRW_Dictionary>dictionaries; //typed OBJECTS dictionaries
    // Common DICTIONARY and named-object-root callback copies share one
    // aggregate budget because both retain the same entry-name/handle graph.
    std::size_t dictionaryRetainedRecords{0};
    std::size_t dictionaryRetainedEntries{0};
    std::size_t dictionaryRetainedBytes{0};
    bool dictionaryCaptureFailed{false};
    // APPID/VIEW/UCS/VPORT callback copies share a container-lifetime budget.
    // Other baseline TABLES families have additional dynamic members and are
    // admitted by separate family-specific accounting.
    std::size_t basicTableRetainedRecords{0};
    std::size_t basicTableRetainedItems{0};
    std::size_t basicTableRetainedBytes{0};
    bool basicTableCaptureFailed{false};
    std::list<DRW_SortEntsTable>sortEntsTables; //typed draw-order tables
    // Adapter-owned typed SORTENTSTABLE copies have independent aggregate
    // bounds from the raw companion and graph-validation work budgets.
    std::size_t sortEntsRetainedRecords{0};
    std::size_t sortEntsRetainedEntries{0};
    std::size_t sortEntsRetainedBytes{0};
    // DWG parser receipts preserve the null-sort => entity-handle fallback
    // distinction needed when projecting SORTENTSTABLE into DXF semantics.
    // They are bounded separately from the typed table and raw DXF carrier.
    std::list<DRW_DwgSortEntsMembership> dwgSortEntsMemberships;
    std::size_t dwgSortEntsMembershipRetainedRecords{0};
    std::size_t dwgSortEntsMembershipRetainedEntries{0};
    std::size_t dwgSortEntsMembershipRetainedBytes{0};
    bool sortEntsCaptureFailed{false};
    // Typed GROUP objects retain their dictionary-derived identity and member
    // entity edges. Name association is resolved only after the complete
    // OBJECTS callback sequence has been read.
    std::list<DRW_Group> groups;
    std::size_t groupRetainedRecords{0};
    std::size_t groupRetainedMembers{0};
    std::size_t groupRetainedBytes{0};
    bool groupCaptureFailed{false};
    bool groupNameResolutionFailed{false};
    // Semantic ACAD_TABLE snapshots retained separately from the established
    // DRW_Insert geometry fallback stored in each block's entity sequence.
    std::list<DRW_Table>tables;
    // Retain decoded DATALINK callbacks so export can reject an unqualified
    // object graph instead of silently dropping external-data connections.
    std::vector<DRW_DataLink> dataLinks;
    // Callback copies have a container-lifetime budget; a failed capture
    // remains sticky so partial DATALINK state cannot pass export preflight.
    std::size_t dataLinkRetainedRecords{0};
    std::size_t dataLinkRetainedItems{0};
    std::size_t dataLinkRetainedBytes{0};
    bool dataLinkCaptureFailed{false};
    // Typed FIELD/FIELDLIST callbacks are retained for a later graph-aware
    // writer gate; raw DXF companions remain separate and are not emitted yet.
    std::list<DRW_Field>fields;
    std::list<DRW_FieldList>fieldLists;
    std::list<DRW_DictionaryWithDefault>dictionaryWithDefaults;
    std::list<DRW_AcDbPlaceholder>placeholders;
    // Typed DICTIONARYWDFLT/PLACEHOLDER copies share a lifetime budget
    // separate from their already-bounded raw DXF companion carriers.
    std::size_t dictionaryDefaultTypedRetainedRecords{0};
    std::size_t dictionaryDefaultTypedRetainedItems{0};
    std::size_t dictionaryDefaultTypedRetainedBytes{0};
    bool dictionaryDefaultTypedCaptureFailed{false};
    std::list<DRW_MLineStyle>mLineStyles;
    // Aggregate admission for typed MLineStyle callback copies, independent
    // of the named-resource raw DXF carrier receipt.
    std::size_t mLineStyleRetainedRecords{0};
    std::size_t mLineStyleRetainedItems{0};
    std::size_t mLineStyleRetainedBytes{0};
    bool mLineStyleCaptureFailed{false};
    std::list<DRW_MLeaderStyle>mLeaderStyles;
    // One container-lifetime budget covers all four typed named-resource
    // callback stores. Per-family graph/wire qualification remains separate.
    std::size_t namedResourceTypedRetainedRecords{0};
    std::size_t namedResourceTypedRetainedBytes{0};
    bool namedResourceTypedResourceLimitExceeded{false};
    bool namedResourceTypedCaptureFailed{false};
    std::list<DRW_TableStyle>tableStyles;
    // Adapter-owned typed TABLESTYLE copies are bounded independently from
    // their raw carrier and the parser's per-object item limits.
    std::size_t tableStylesRetainedRecords{0};
    std::size_t tableStylesRetainedItems{0};
    std::size_t tableStylesRetainedBytes{0};
    bool tableStylesCaptureFailed{false};
    // CELLSTYLEMAP is a separate object that owns custom TABLESTYLE cell
    // definitions. Typed callbacks and DXF raw carriers are retained
    // separately. Same-version DXF replay additionally requires private,
    // bounded typed/raw baselines and the rooted TABLESTYLE owner graph.
    std::list<DRW_CellStyleMap>cellStyleMaps;
    std::size_t cellStyleMapsRetainedRecords{0};
    std::size_t cellStyleMapsRetainedItems{0};
    std::size_t cellStyleMapsRetainedBytes{0};
    bool cellStyleMapsCaptureFailed{false};
    std::list<DRW_DetailViewStyle>detailViewStyles;
    std::list<DRW_SectionViewStyle>sectionViewStyles;
    // Typed detail/section callbacks share aggregate record/item/byte limits;
    // raw companions, reference closure, and replay qualification stay separate.
    std::size_t modelDocViewStyleTypedRetainedRecords{0};
    std::size_t modelDocViewStyleTypedRetainedItems{0};
    std::size_t modelDocViewStyleTypedRetainedBytes{0};
    bool modelDocViewStyleTypedResourceLimitExceeded{false};
    bool modelDocViewStyleTypedCaptureFailed{false};
    // Complete DXF-origin carriers remain paired with typed projections for
    // lossless same-version replay; DWG-origin styles have no raw companion
    // and use the separately gated typed-only serializer.
    std::list<DRW_RawDxfObject>rawDetailViewStyles;
    std::list<DRW_RawDxfObject>rawSectionViewStyles;
    std::list<DRW_Scale>scales;
    std::list<DRW_DictionaryVar>dictionaryVars;
    DRW_Dictionary namedObjectDictionary; //position-identified DXF NOD
    bool hasNamedObjectDictionary {false};
    std::list<DRW_XRecord>xRecords;      //typed XRECORD objects
    // Adapter-owned XRECORD callback copies are bounded for their full
    // lifetime, including duplicated payload retained by an immutable DWG
    // source snapshot. A failed capture makes DXF export fail closed.
    std::size_t xRecordRetainedRecords{0};
    std::size_t xRecordRetainedItems{0};
    std::size_t xRecordRetainedBytes{0};
    bool xRecordCaptureFailed{false};
    std::list<DRW_Material>materials;    //typed MATERIAL objects
    // Aggregate admission for adapter-owned MATERIAL callback copies. The
    // per-import raw-carrier receipt remains separate from this typed budget.
    std::size_t materialRetainedRecords{0};
    std::size_t materialRetainedBytes{0};
    bool materialCaptureFailed{false};
    std::list<DRW_RawDxfObject>rawMaterialObjects; //DXF-only ordered carriers
    std::list<DRW_VisualStyle>visualStyles; //typed visual style metadata
    // Typed VISUALSTYLE payload copies have an aggregate budget independent
    // of their bounded ordered raw companions.
    std::size_t visualStyleTypedRetainedRecords{0};
    std::size_t visualStyleTypedRetainedBytes{0};
    bool visualStyleTypedCaptureFailed{false};
    std::list<DRW_RawDxfObject>rawVisualStyleObjects; //full DXF style tag carrier
    // Adapter-owned raw VISUALSTYLE companion accounting is aggregate across
    // imports appended to this data container; these counters are not reset
    // with the per-import provenance receipt.
    std::size_t rawVisualStyleRetainedRecords{0};
    std::size_t rawVisualStyleRawGroups{0};
    std::size_t rawVisualStyleRetainedBytes{0};
    bool rawVisualStyleCaptureFailed{false};
    // Opaque associative DXF source records and their partial typed shells.
    // Strict output remains closed until a replay profile is qualified.
    std::vector<DRW_AssociativeObject> associativeObjects;
    std::vector<DRW_RawDxfObject> rawAssociativeObjects;
    // Preserve clipped-reference filter callbacks and DXF tag companions.
    // Export remains fail-closed until graph admission and serialization exist.
    std::vector<DRW_SpatialFilter> spatialFilters;
    std::vector<DRW_RawDxfObject> rawSpatialFilterObjects;
    std::list<DRW_RawDxfObject>rawTableStyles; //full DXF TABLESTYLE carrier
    std::list<DRW_RawDxfObject>rawCellStyleMaps; //full DXF CELLSTYLEMAP carrier
    std::list<DRW_RawDxfObject>rawSortEntsTables; //source-ordered DXF SORT carriers
    // SORTENTSTABLE/TABLESTYLE/CELLSTYLEMAP raw callback copies share a
    // container-lifetime resource budget. CELLSTYLEMAP DXF baseline copies
    // are charged as additional records/groups/bytes; owner-graph and
    // serializer rules remain distinct.
    std::size_t rawSortTableStyleRetainedRecords{0};
    std::size_t rawSortTableStyleRawGroups{0};
    std::size_t rawSortTableStyleRetainedBytes{0};
    bool rawSortTableStyleCaptureFailed{false};
    std::list<DRW_RawDxfObject>rawFields; //full DXF FIELD tag carriers
    std::list<DRW_RawDxfObject>rawFieldLists; //full DXF FIELDLIST tag carriers
    // Aggregate admission for FIELD/FIELDLIST raw DXF companions. The
    // per-import receipt resets independently; these lifetime counters and
    // sticky failure stay with the retained carriers.
    std::size_t rawFieldCarrierRetainedRecords{0};
    std::size_t rawFieldCarrierRawGroups{0};
    std::size_t rawFieldCarrierRetainedBytes{0};
    bool rawFieldCarrierCaptureFailed{false};
    // FIELD/FIELDLIST typed callback admission is separate from the raw
    // carrier copy budget and persists across appended imports.
    std::size_t fieldCarrierTypedRetainedRecords{0};
    std::size_t fieldCarrierTypedItemCount{0};
    std::size_t fieldCarrierTypedRetainedBytes{0};
    bool fieldCarrierTypedCaptureFailed{false};
    // DXF tag companions for the named-resource families whose adapter
    // callbacks also retain typed values. DWG bodies are never stored here.
    std::list<DRW_RawDxfObject>rawNamedResourceObjects;
    // Aggregate admission for SCALE/DICTIONARYVAR/MLINESTYLE/MLEADERSTYLE
    // raw DXF tag companions, independent of import-scoped counters.
    std::size_t rawNamedResourceRetainedRecords{0};
    std::size_t rawNamedResourceRawGroups{0};
    std::size_t rawNamedResourceRetainedBytes{0};
    bool rawNamedResourceCaptureFailed{false};
    // DXF-origin dictionary-with-default and placeholder tags are separate
    // from the four ordinary named-resource classes above because their
    // default-pointer/owner graph has a different closure contract.
    std::list<DRW_RawDxfObject>rawDictionaryWithDefaults;
    std::list<DRW_RawDxfObject>rawPlaceholders;
    // Aggregate admission for the paired raw DXF carriers. These counters
    // persist across imports appended to this container; a prior failed
    // capture must not be cleared by resetting the per-import receipt.
    std::size_t rawDictionaryDefaultRetainedRecords{0};
    std::size_t rawDictionaryDefaultRawGroups{0};
    std::size_t rawDictionaryDefaultRetainedBytes{0};
    bool rawDictionaryDefaultCaptureFailed{false};
    std::list<DRW_ProxyObject>proxyObjects; //typed ACAD_PROXY_OBJECT records
    std::list<DRW_RawDxfObject>rawProxyObjects; //lossless DXF proxy passthrough
    std::size_t proxyObjectRetainedBytes{0};
    bool proxyObjectCaptureFailed{false};
    // Generic OBJECTS callbacks without a typed or explicitly bounded raw
    // adapter route are not retained. Strict conversion must not silently
    // publish a document after observing one of these records.
    bool unmodeledDxfObjectSeen{false};
    // Standalone table dependencies have typed read callbacks, but no
    // converter-owned replay/semantic writer yet. Refuse rather than drop
    // them in either compatible or strict conversion (DXF or native DWG).
    bool standaloneTableContentPresent{false};
    bool standaloneTableGeometryPresent{false};
    bool rawDxfEntityCaptureFailed {false}; //fail closed on incomplete capture
    std::size_t rawDxfEntityRetainedRecords{0};
    std::size_t rawDxfEntityRawGroups{0};
    std::size_t rawDxfEntityRetainedBytes{0};
    std::vector<DRW_RawDxfSection> rawDxfSections; //opaque DXF section passthrough
    // Adapter-owned opaque DXF section copies are bounded independently of
    // the parser's source-text lease and the section parser's per-record cap.
    // Managed ACDSDATA imports charge their immutable replay baseline as a
    // second group/byte owner, not as an additional source callback.
    std::size_t rawDxfSectionRetainedRecords{0};
    std::size_t rawDxfSectionRetainedGroups{0};
    std::size_t rawDxfSectionRetainedBytes{0};
    bool rawDxfSectionCaptureFailed{false};
    std::vector<DRW_Class> dxfClasses; //readback of emitted CLASS metadata
    // Adapter-owned CLASSES callback copies are bounded across this data
    // container's lifetime.
    std::size_t dxfClassRetainedRecords{0};
    std::size_t dxfClassRetainedBytes{0};
    bool dxfClassCaptureFailed{false};
    DRW_DwgClassCoverageReport dwgClassCoverage; //source DWG class-number binding
    std::vector<DRW_DataStorageSection> dataStorageSections; //typed DWG AcDs data
    // Adapter-owned section transfers/copies are independently budgeted from
    // parser admission and the DataStorage payload-retention cap.
    std::size_t dataStorageRetainedSections{0};
    std::size_t dataStorageRetainedBytes{0};
    bool dataStorageCaptureFailed{false};
    // Typed DWG callbacks needed to inspect bounded ACIS history closures.
    // The unsupported-object companions are DWG frame bodies, not DXF groups;
    // they are retained only for correlation/diagnostics, never DXF replay.
    std::vector<DRW_EvaluationGraph> evaluationGraphs;
    std::vector<DRW_RawDxfObject> rawAcdsHistoryObjects;
    dx_acdsHistoryDxfImportReceipt acdsHistoryDxfImport;
    std::size_t evaluationGraphRetainedBytes{0};
    bool evaluationGraphCaptureFailed{false};
    std::vector<DRW_AcShHistoryObject> acshHistoryObjects;
    // Adapter accounting for typed AcSh history callbacks. A rejected
    // callback makes export fail closed instead of silently omitting it.
    std::size_t acshHistoryRetainedBytes{0};
    bool acshHistoryCaptureFailed{false};
    std::vector<DRW_UnsupportedObject> acdsHistoryDwgFrames;
    std::size_t acdsHistoryDwgFrameRetainedBytes{0};
    bool acdsHistoryDwgFrameCaptureFailed{false};
    // Standalone DWG annotation-context frames have no qualified DXF replay
    // route. Keep a sticky presence marker so conversion cannot silently drop
    // a typed shell or its opaque companion.
    bool standaloneObjectContextDataPresent{false};
    // Retain dynamic-block callbacks so export can fail closed rather than
    // silently discarding the graph. No dynamic-block DXF writer is qualified.
    std::vector<DRW_DynamicBlockObject> dynamicBlockObjects;
    std::size_t dynamicBlockRetainedBytes{0};
    bool dynamicBlockCaptureFailed{false};
    std::list<dx_ifaceBlock*>blocks;    //stores a copy of all blocks and the entities in it
    std::list<dx_ifaceImg*>images;      //temporary list to find images for link with DRW_ImageDef. Do not delete it!!

    // IMAGEDEF is delivered after IMAGE entities. Preserve full definitions
    // under their import generation and source handle; a filename is not a
    // complete definition and is not an identity.
    using ImageDefinitionKey = std::pair<std::uint64_t, std::uint32_t>;
    std::map<ImageDefinitionKey, DRW_ImageDef> imageDefinitions;
    std::map<ImageDefinitionKey, DRW_ImageDefinitionReactor> imageDefinitionReactors;
    std::size_t imageDefinitionReactorRetainedRecords{0u};
    std::size_t imageDefinitionReactorRetainedBytes{0u};
    bool imageDefinitionReactorCaptureFailed{false};
    // Callback-owned IMAGEDEF map copies are bounded across this container's
    // lifetime. Filename paths copied into IMAGE entities use the separate
    // typed-entity aggregate counter because they live with those entities.
    std::size_t imageDefinitionRetainedRecords{0};
    std::size_t imageDefinitionRetainedBytes{0};
    static constexpr std::size_t ImageDefinitionAuxiliaryStatusKeyMax =
        100000u;
    // Bounds both invalid-generation and conflicting-key status sets together.
    std::size_t imageDefinitionAuxiliaryStatusKeysRetained{0};
    bool imageDefinitionCaptureFailed{false};
    std::set<ImageDefinitionKey> conflictingImageDefinitions;
    std::set<std::uint64_t> invalidImageDefinitionGenerations;

    bool retainImageDefinition(std::uint64_t generation,
                               const DRW_ImageDef& definition) {
        if (definition.handle == 0 || definition.handle == DRW::NoHandle) {
            if (invalidImageDefinitionGenerations.find(generation)
                == invalidImageDefinitionGenerations.end()) {
                if (!admitImageDefinitionAuxiliaryStatusKey())
                    return false;
                invalidImageDefinitionGenerations.insert(generation);
                ++imageDefinitionAuxiliaryStatusKeysRetained;
            }
            return false;
        }
        const ImageDefinitionKey key{generation, definition.handle};
        const auto inserted = imageDefinitions.emplace(key, definition);
        if (!inserted.second) {
            if (conflictingImageDefinitions.find(key)
                == conflictingImageDefinitions.end()) {
                if (!admitImageDefinitionAuxiliaryStatusKey())
                    return false;
                conflictingImageDefinitions.insert(key);
                ++imageDefinitionAuxiliaryStatusKeysRetained;
            }
            return false;
        }
        return true;
    }

    bool admitImageDefinitionAuxiliaryStatusKey() {
        const std::size_t conflicting = conflictingImageDefinitions.size();
        const std::size_t invalid = invalidImageDefinitionGenerations.size();
        if (conflicting > ImageDefinitionAuxiliaryStatusKeyMax
            || invalid > ImageDefinitionAuxiliaryStatusKeyMax - conflicting) {
            imageDefinitionCaptureFailed = true;
            return false;
        }
        const std::size_t actualCount = conflicting + invalid;
        if (imageDefinitionAuxiliaryStatusKeysRetained < actualCount)
            imageDefinitionAuxiliaryStatusKeysRetained = actualCount;
        if (imageDefinitionAuxiliaryStatusKeysRetained
            >= ImageDefinitionAuxiliaryStatusKeyMax) {
            imageDefinitionCaptureFailed = true;
            return false;
        }
        return true;
    }

    const DRW_ImageDef* findImageDefinition(std::uint64_t generation,
                                            std::uint32_t sourceHandle) const {
        const ImageDefinitionKey key{generation, sourceHandle};
        if (conflictingImageDefinitions.find(key)
            != conflictingImageDefinitions.end())
            return nullptr;
        const auto found = imageDefinitions.find(key);
        return found == imageDefinitions.end() ? nullptr : &found->second;
    }

    dx_ifaceBlock* mBlock;              //container to store model entities

    // True only while the entire typed/raw style set still belongs to one
    // successful, versioned import. Authored/mixed/reused containers fail
    // closed; this does not gate the existing DXF raw-replay path.
    bool hasSingleSuccessfulVisualStyleImport() const noexcept {
        const dx_visualStyleImportReceipt& receipt = visualStyleImport;
        if (!receipt.successful || receipt.captureFailed
            || receipt.resourceLimitExceeded || rawVisualStyleCaptureFailed
            || receipt.typedCaptureFailed
            || receipt.typedResourceLimitExceeded
            || visualStyleTypedCaptureFailed
            || receipt.generation != 1
            || receipt.visualStyleBegin != 0 || receipt.rawVisualStyleBegin != 0
            || receipt.visualStyleEnd != visualStyles.size()
            || receipt.rawVisualStyleEnd != rawVisualStyleObjects.size()
            || receipt.visualStyleBegin > receipt.visualStyleEnd
            || receipt.typedRetainedRecords
                != receipt.visualStyleEnd - receipt.visualStyleBegin
            || receipt.typedRetainedRecords
                != visualStyleTypedRetainedRecords
            || receipt.typedRetainedBytes
                != visualStyleTypedRetainedBytes
            || receipt.typedRetainedRecords
                > kMaxVisualStyleTypedRetainedRecords
            || receipt.typedRetainedBytes
                > kMaxVisualStyleTypedRetainedBytes
            || visualStyleTypedRetainedRecords
                > kMaxVisualStyleTypedRetainedRecords
            || visualStyleTypedRetainedBytes
                > kMaxVisualStyleTypedRetainedBytes
            || receipt.visualStyleEnd == 0
            || receipt.sourceVersion == DRW::UNKNOWNV
            || sourceFormat != receipt.sourceFormat
            || sourceVersion != receipt.sourceVersion)
            return false;

        if (receipt.sourceFormat == dx_source_format::DXF) {
            return dxfReadOutcome.attempted && dxfReadOutcome.readReturned
                && dxfReadOutcome.operationSucceeded
                && dxfReadOutcome.sourceVersion == receipt.sourceVersion
                && receipt.rawVisualStyleEnd - receipt.rawVisualStyleBegin
                    == receipt.visualStyleEnd - receipt.visualStyleBegin;
        }
        if (receipt.sourceFormat == dx_source_format::DWG) {
            return dwgReadOutcome.attempted && dwgReadOutcome.operationSucceeded
                && dwgReadOutcome.sourceVersion == receipt.sourceVersion
                && receipt.rawVisualStyleEnd == receipt.rawVisualStyleBegin;
        }
        return false;
    }

    static constexpr std::size_t kMaxVisualStyleTypedRetainedRecords =
        100000u;
    // Keep the adapter header independent of src/intern's parser-limit header.
    static constexpr std::size_t kMaxVisualStyleTypedRetainedBytes =
        64u * 1024u * 1024u;

    // Structural source-capture gate for the first DWG-origin VISUALSTYLE
    // serializer profile. This verifies one successful AC1032 import and the
    // complete 58-operation / 8-CMC R2013b body. It does not prove that body
    // values remain unedited, map to DXF tags, or satisfy the separate CMC
    // output policy; those are serializer-admission checks.
    bool hasCompleteAc1032VisualStyleSourceProfile() const noexcept {
        if (!hasSingleSuccessfulVisualStyleImport()
            || visualStyleImport.sourceFormat != dx_source_format::DWG
            || visualStyleImport.sourceVersion != DRW::AC1032
            || visualStyles.empty() || !rawVisualStyleObjects.empty())
            return false;

        for (const DRW_VisualStyle& style : visualStyles) {
            const DRW_VisualStyleBody& body = style.m_body;
            const DRW_VisualStyleEncoding& encoding = body.m_encoding;
            if (!style.m_bodyDecoded || !body.hasR2013bExpansion
                || encoding.companionCount
                    != DRW_VisualStyleEncoding::kPropertyCount
                || encoding.colorCount != DRW_VisualStyleEncoding::kColorCount
                || !encoding.hasSourceIsolines
                || encoding.sourceIsolines < 0
                || encoding.sourceIsolines > DRW_VisualStyleBody::kMaxEdgeIsolines
                || body.edgeIsolines != encoding.sourceIsolines
                || body.extLightingModel < 0
                || body.extLightingModel > 0xFFFF)
                return false;

            for (const DRW_VisualStyleColor& color : encoding.colors) {
                if (!color.present || !color.modern)
                    return false;
            }
        }
        return true;
    }

    // Verifies one fresh, successful import and (for DXF) a one-to-one
    // type/handle/owner join between typed values and their raw carriers.
    // It does not certify each family's decoded payload or writer profile.
    bool hasSingleSuccessfulNamedResourceImport() const;
    // Verifies one successful import and, for DXF, one raw MATERIAL carrier
    // per typed handle/owner. It does not certify dictionary closure or replay.
    bool hasSingleSuccessfulMaterialImport() const;
    bool hasSingleSuccessfulModelDocViewStyleImport() const;
    // Resolve DWG Detail/SectionViewStyle names from their validated NOD child
    // dictionaries after object decoding has collected the complete graph.
    // The operation is transactional: ambiguous graphs leave all names as-is.
    bool resolveDwgModelDocViewStyleNames();
    // Verifies the captured ModelDocViewStyle references resolve uniquely to
    // the expected STYLE, LTYPE, or BLOCK_RECORD table-entry family. It does
    // not certify dictionary rooting, handle remapping, or writer support.
    bool hasClosedModelDocViewStyleResourceReferences() const;
    bool hasSingleSuccessfulDictionaryDefaultImport() const;

    // Verifies one fresh DXF import and a one-to-one class/handle/owner join
    // between FIELD/FIELDLIST values and their raw tag carriers. It does not
    // certify reference closure, field expression semantics, or writeability.
    bool hasSingleSuccessfulFieldCarrierImport() const;

    // Verifies a bounded AutoCAD FIELD graph: one NOD FIELDLIST, exact FIELD
    // membership, roots owned through entity extension dictionaries (including
    // TABLE group-344 references), and an acyclic, closed child-FIELD graph.
    // Object-pointer/CadValue handles must resolve locally; emitted/remapped
    // output closure and FIELD serialization remain unqualified.
    bool hasCompleteAutoCadFieldGraph() const;

    bool hasStandardDwgObjectCaptureFailure() const noexcept {
        return m_standardDwgObjectCaptureFailed;
    }
    // Read-only current-import source lease; null means no report, never an
    // empty/qualified CLASSES section. Mutable public class rows are separate.
    const DRW_DwgClassSourceReport* dwgClassSource() const noexcept {
        if (importGeneration != 0u && m_classSourceFailureGeneration == importGeneration)
            return &m_classSourceFailure;
        const auto found = m_classSourceReports.find(importGeneration);
        return found == m_classSourceReports.end() ? nullptr : &found->second;
    }
    // Actual import-owned correlation, not the advisory public-generation
    // lookup above. Even Joined does not qualify a schema or support claim.
    const dx_class_evidence::Result& dwgClassEvidence() const noexcept {
        if (!m_classEvidenceFallback) {
            const auto found = m_classEvidenceImports.find(m_classEvidenceGeneration);
            if (found != m_classEvidenceImports.end()
                && found->second.result.attempt == m_classEvidenceAttempt)
                return found->second.result;
        }
        return m_classEvidenceFailure;
    }
    // Checks public coverage/profile observations against the private actual
    // read. It does not certify editable CLASS output, raster objects or DWG.
    bool hasUnchangedDwgClassEvidence() const noexcept;
    bool hasStandardDwgObjectPayloadFailure() const noexcept {
        return m_standardDwgObjectPayloadFailed;
    }
    // Pre-export source check, while this container owns its sections (not
    // during their temporary synchronous transfer into the writer).
    bool hasUnchangedAcdsRawDxfSections() const noexcept;

private:
    // Proof identity is not an address (which can be reused) and is not copied
    // or assigned with another container's retained import evidence.
    struct EntitiesContainerIdentity {
        static std::uint64_t next() noexcept {
            static std::atomic<std::uint64_t> counter{1u};auto id=counter.load(std::memory_order_relaxed);
            while(id!=(std::numeric_limits<std::uint64_t>::max)())
                if(counter.compare_exchange_weak(id,id+1u,std::memory_order_relaxed))return id;
            return 0u;
        }
        std::uint64_t value{next()};
        EntitiesContainerIdentity()=default;
        EntitiesContainerIdentity(const EntitiesContainerIdentity&) noexcept:value(next()){}
        EntitiesContainerIdentity& operator=(const EntitiesContainerIdentity&) noexcept {return *this;}
    } m_entitiesContainerIdentity;
    // Immutable-by-API import baselines, not editable drawing state or an
    // output certificate. Never reset these to launder an appended import.
    struct TableDxfSourceCapture {
        struct Table {
            Table(const DRW_Table& model, const DRW_RawDxfObject& source)
                : typed(model), raw(source) {}
            DRW_Table typed;
            DRW_RawDxfObject raw;
        };
        std::uint64_t generation{0};
        DRW::Version version{DRW::UNKNOWNV};
        const void* reader{nullptr};
        bool closed{false}, successful{false}, failed{false}, contextFailed{false};
        bool contentPayloadMismatch{false};
        std::size_t records{0}, groups{0}, bytes{0}, selectionWork{0};
        std::size_t maxRecords{100000u}, maxGroups{4000000u};
        std::size_t maxBytes{64u * 1024u * 1024u}, maxWork{4000000u};
        std::size_t maxContentProofWork{4000000u};
        std::map<std::uint32_t, Table> tables;
        std::map<std::uint32_t, DRW_TableContentObject> contents;
        std::map<std::uint32_t, DRW_TableGeometry> geometries;
        std::map<std::uint32_t, DRW_Dictionary> dictionaries;
        std::map<std::uint32_t, DRW_XRecord> xrecords;
        std::map<std::uint32_t, DRW_RawDxfObject> rawObjects;
        std::map<std::uint32_t, std::vector<dxfTableContent::Reference>> contentReferences;
        std::size_t contentProofWork{0};
        struct AttdefOrigin {
            std::uint64_t generation{0},scopeId{0};
            std::uint32_t block{0},blockRecord{0},handle{0},owner{0};
            std::size_t dispatch{0},sourceEvent{0},typedEvent{0},physical{0},index{0};
            DRW_DxfPhysicalRecordSpan begin,end;
            std::shared_ptr<const DRW_DxfBlockDeliverySource> delivery;
            std::shared_ptr<const DRW_DxfBlockChildrenSource> children;
            std::shared_ptr<const dx_block_membership::Audit> membership;
            std::shared_ptr<const DRW_Attdef> model;
            std::shared_ptr<const DRW_RawDxfObject> raw,blockBegin,blockEnd;
            std::shared_ptr<const DRW_Block> original,effective,footer;
        };
        struct EntitiesReadContext;
        struct Resource {
            dx_tableResourceFamily family{dx_tableResourceFamily::Entity};
            std::uint32_t owner{0};
            std::shared_ptr<const DRW_TableEntry> model;
            // ATTDEF is an entity, never a forged DRW_TableEntry. Pairing
            // capture is not raw replay, target/profile or output proof.
            std::shared_ptr<const DRW_Attdef> attdefModel;
            dxfAttdef::SourceState attdefState;
            DRW_DxfAttdefSourceContext attdefContext;
            std::shared_ptr<const EntitiesReadContext> attdefReadContext;
            std::uint32_t attdefBlockHandle{0},attdefBlockRecord{0};
            UTF8STRING attdefBlockName;
            bool awaitingAttdef{false},attdefPairMatched{false};
            bool attdefFieldsMatched{false},attdefFieldMismatch{false};
            dx_attdef_source::Audit attdefProfile;
            // Immutable original membership, not a handle-only association or
            // complete compound/dependency/live/output certificate.
            std::shared_ptr<const AttdefOrigin> attdefOrigin;
            enum class OriginState : std::uint8_t {NotCaptured,Standalone,Unavailable,SourceBound,PairBound,EnclosingMatched,Failed};
            OriginState attdefOriginState{OriginState::NotCaptured};
            std::set<int> styleIds;
            bool duplicateStyleId{false}, invalidStyleId{false}, namespaceComplete{false};
            std::uint32_t cellMapTarget{0};
            unsigned cellMapEntries{0};
            bool cellMapOwnerEdge{false};
            bool invalidName{false}, ambiguousName{false};
            std::shared_ptr<const DRW_RawDxfObject> raw;
            std::vector<dx_tableResourceReference> references;
            std::vector<UTF8STRING> referenceNameKeys;
            bool invalidTextStyleName{false};
            bool rawTypedMatched{false}, rawTypedMismatch{false};
            bool awaitingSymbol{false};
        };
        std::map<std::uint32_t,Resource> resources;
        std::map<std::pair<dx_tableResourceFamily,UTF8STRING>,std::uint32_t> resourceNames;
        // Independent from compound callback counters: these freeze inputs
        // even when TABLE follows the symbol tables/OBJECTS in the source.
        std::size_t resourceRecords{0}, resourceItems{0}, resourceBytes{0};
        std::size_t maxResourceRecords{100000u}, maxResourceItems{4000000u};
        std::size_t maxResourceBytes{64u*1024u*1024u}, maxBindingWork{4000000u};
        bool resourceCaptureFailed{false};
        std::size_t resourceProofRecords{0}, resourceProofGroups{0}, resourceProofBytes{0}, resourceProofWork{0};
        std::size_t maxResourceProofRecords{100000u}, maxResourceProofGroups{4000000u};
        std::size_t maxResourceProofBytes{64u*1024u*1024u}, maxResourceProofWork{4000000u};
        bool resourceProofCaptureFailed{false};
        std::size_t attdefPending{0};
        struct IdentityInput;
        struct IdentityAudit;
        struct AttdefSourcePins {
            std::shared_ptr<const DRW_Attdef> model;
            std::shared_ptr<const DRW_RawDxfObject> raw;
            std::shared_ptr<const AttdefOrigin> origin;
            std::shared_ptr<const EntitiesReadContext> readContext;
            dxfAttdef::SourceState state;DRW_DxfAttdefSourceContext context;
        };
        struct CompoundBody {
            enum class State : std::uint8_t { Captured, ModeledReplayMatched, ModelMismatch, SourceStateMismatch, DecodeFailed, ContextMismatch };
            std::shared_ptr<const DRW_Entity> model;
            std::shared_ptr<const dx_block_membership::Audit> membership;
            std::shared_ptr<const EntitiesReadContext> context;
            std::size_t dispatch{0};
            std::vector<dxfCompound::SourceState> sourceStates;
            std::vector<dxfCompound::SourceState> replayStates;
            std::vector<dx_attdef_source::Audit> profiles;
            AttdefSourcePins attdef;
            bool profilesComplete{false};
            State state{State::Captured};
        };
        // Original BLOCK versus reconciled callback and distinct ENDBLK.
        // Closure/pairing are transport facts, NOT raw/profile/child/graph proof.
        struct BlockSource {
            std::shared_ptr<const DRW_Block> original,effective,footer;
            std::shared_ptr<const DRW_RawDxfObject> beginRaw,endRaw;
            std::shared_ptr<const EntitiesReadContext> context,contextOwner;
            bool contextMatched{false};
            std::uint16_t originPhases{0};std::size_t originEvents{0};bool originFailed{false},originPending{false},originValue{false};
            const DRW_Block* publishedEndpoint{nullptr};const DRW_RawDxfObject* publishedEndpointRaw{nullptr};
            const DRW_DxfBlockChildrenSource* publishedChildren{nullptr};const DRW_DxfBlockDeliverySource* publishedDelivery{nullptr};
            // Exact table-row owners used by endpoint reconciliation, not a
            // later handle-only lookup of an interchangeable model/raw pair.
            std::shared_ptr<const DRW_TableEntry> recordModel;
            std::shared_ptr<const DRW_RawDxfObject> recordRaw;
            dxfAttdef::SourceState::Cursor originalCursor,footerCursor;
            bool beginPaired{false},footerPaired{false},closed{false};
            bool beginFieldsMatched{false},footerFieldsMatched{false};
            dx_attdef_source::Audit beginProfile,endProfile;
            enum class Reconciliation : std::uint8_t { NotChecked, MissingBlockRecord, SourceMismatch, WrongOwner, Matched };
            Reconciliation reconciliation{Reconciliation::NotChecked};
            std::shared_ptr<const DRW_DxfBlockChildrenSource> children;
            std::shared_ptr<const DRW_DxfBlockDeliverySource> delivery;
            std::size_t deliveryNext{0};
            bool deliveryExpected{false},deliveryPending{false},deliveryObserved{false},deliveryFinished{false};
            std::shared_ptr<const dx_block_membership::Audit> membership;
            std::size_t membershipObserved{0};
            std::map<std::size_t,std::shared_ptr<const CompoundBody>> compoundBodies;
            std::map<std::size_t,std::shared_ptr<const CompoundBody>> compoundBodyOwners;
            std::vector<std::uint32_t> attdefOrigins;
        };
        std::map<std::uint32_t,BlockSource> blockSources;
        std::uint32_t blockSourceActive{0};
        std::size_t blockSourcePending{0},blockSourceBytes{0},blockSourceGroups{0},blockSourceWork{0};
        std::size_t maxBlockSourceRecords{100000u},maxBlockSourceBytes{64u*1024u*1024u};
        std::size_t maxBlockSourceGroups{4000000u},maxBlockSourceWork{4000000u};
        bool blockSourceCaptureFailed{false};
        struct BlockOriginAudit {
            const void* container{nullptr};std::uint64_t generation{0},containerId{0};
            std::shared_ptr<const DRW_Block> original,effective,footer;
            std::shared_ptr<const DRW_RawDxfObject> beginRaw,endRaw;
            std::shared_ptr<const DRW_DxfBlockChildrenSource> children;
            std::shared_ptr<const DRW_DxfBlockDeliverySource> delivery;
            std::shared_ptr<const EntitiesReadContext> context;
            std::shared_ptr<const IdentityInput> identity;
            std::shared_ptr<const IdentityAudit> identities;
            std::size_t events{0},first{0},last{0};bool publications{false},bound{false};
        };
        std::map<std::uint32_t,std::shared_ptr<const BlockOriginAudit>> blockOriginAudits,blockOriginOwners;
        std::size_t blockOriginBytes{0},blockOriginWork{0};
        std::size_t maxBlockOriginBytes{64u*1024u*1024u},maxBlockOriginWork{64u*1024u*1024u};
        bool blockOriginCaptureFailed{false};
        std::size_t blockChildrenRecords{0},blockChildrenGroups{0},blockChildrenBytes{0},blockChildrenWork{0};
        std::size_t maxBlockChildrenRecords{100000u},maxBlockChildrenGroups{4000000u};
        std::size_t maxBlockChildrenBytes{64u*1024u*1024u},maxBlockChildrenWork{4000000u};
        bool blockChildrenCaptureFailed{false};
        std::size_t blockDeliveryDispatches{0},blockDeliveryEvents{0},blockDeliveryBytes{0},blockDeliveryWork{0};
        std::size_t maxBlockDeliveryDispatches{100000u},maxBlockDeliveryEvents{400000u};
        std::size_t maxBlockDeliveryBytes{64u*1024u*1024u},maxBlockDeliveryWork{64u*1024u*1024u};
        bool blockDeliveryCaptureFailed{false};
        std::size_t attdefOriginRecords{0},attdefOriginBytes{0},attdefOriginWork{0};
        std::size_t maxAttdefOriginRecords{100000u},maxAttdefOriginBytes{64u*1024u*1024u},maxAttdefOriginWork{64u*1024u*1024u};
        bool attdefOriginCaptureFailed{false};
        std::size_t blockMembershipRecords{0},blockMembershipBytes{0},blockMembershipWork{0};
        std::size_t maxBlockMembershipRecords{100000u},maxBlockMembershipBytes{64u*1024u*1024u},maxBlockMembershipWork{64u*1024u*1024u};
        bool blockMembershipCaptureFailed{false};
        std::size_t compoundBodyRecords{0},compoundBodyBytes{0},compoundBodyWork{0};
        std::size_t maxCompoundBodyRecords{100000u},maxCompoundBodyBytes{64u*1024u*1024u},maxCompoundBodyWork{64u*1024u*1024u};
        bool compoundBodyCaptureFailed{false};
        std::size_t maxBlockBindingBytes{64u*1024u*1024u};
        struct EntitiesSource {
            const void* container{nullptr};std::uint64_t generation{0},containerId{0};
            std::shared_ptr<const DRW_DxfEntitiesSource> physical;
        };
        struct EntitiesTableOrigin {
            std::size_t section{0},record{0};
            std::shared_ptr<const DRW_DxfEntitiesSource> physical;
        };
        std::vector<EntitiesSource> entitiesSources;
        std::map<std::uint32_t,EntitiesTableOrigin> entitiesTables;
        std::size_t entitiesRecords{0},entitiesGroups{0},entitiesBytes{0},entitiesWork{0};
        std::size_t maxEntitiesSections{1024u},maxEntitiesRecords{100000u},maxEntitiesGroups{4000000u};
        std::size_t maxEntitiesBytes{64u*1024u*1024u},maxEntitiesWork{64u*1024u*1024u};
        bool entitiesCaptureFailed{false},entitiesTableIdentityFailed{false};
        struct EntitiesObservedValue {
            DRW_DxfEntitiesDeliveryEvent event;
            bool valueObserved{false},modelUnavailable{false};
            std::shared_ptr<const DRW_Entity> model;
            std::shared_ptr<const DRW_RawDxfObject> raw;
            std::vector<dxfCompound::SourceState> compoundStates;
            bool compoundStatesCaptured{false};
        };
        struct EntitiesReadContext {
            DRW::Version sourceVersion{DRW::UNKNOWNV},codecVersion{DRW::UNKNOWNV};
            UTF8STRING codecCodePage,declaration;
            dxfRW::DxfCompatibilityProfile compatibilityProfile{dxfRW::DxfCompatibilityProfile::StandaloneSafe};
            DRW::DxfTextEncoding textEncoding{DRW::DxfTextEncoding::Declared};
            DRW::DxfCodePagePolicy codePagePolicy{DRW::DxfCodePagePolicy::Compatible};
            bool hasDeclaration{false},applyExtrusion{false},binary{false},captured{false};
        };
        struct TableInput {
            const void* container{nullptr};std::uint64_t generation{0},containerId{0};
            std::uint32_t block{0};std::uint64_t scope{0};std::size_t event{0},section{0};bool inBlock{false},typed{false};
            std::shared_ptr<const DRW_Table> model;
            std::shared_ptr<const DRW_RawDxfObject> raw;
            std::shared_ptr<const EntitiesReadContext> context;
            std::shared_ptr<const TableInput> source;
            std::shared_ptr<const DRW_DxfBlockChildrenSource> blockPhysical;
            std::shared_ptr<const DRW_DxfBlockDeliverySource> blockDelivery;
            std::shared_ptr<const dx_block_membership::Audit> membership;
            std::shared_ptr<const DRW_Block> blockBegin,blockEffective,blockEnd;
            dxfTable::SourceState state;
            std::vector<dxfCompound::SourceState> linked;
        };
        struct TablePendingInput {
            const DRW_Table* model{nullptr};const DRW_RawDxfObject* raw{nullptr};
        };
        std::map<std::uint32_t,std::shared_ptr<const TableInput>> tableInputs,tableInputOwners,tableTypedInputs,tableTypedOwners;
        std::map<std::uint32_t,TablePendingInput> tablePendingInputs;
        std::size_t tableInputRecords{0},tableInputBytes{0},tableInputWork{0},tableInputUnavailable{0};
        std::size_t maxTableInputRecords{100000u},maxTableInputBytes{64u*1024u*1024u},maxTableInputWork{64u*1024u*1024u};
        bool tableInputCaptureFailed{false},tableInputIncomplete{false};
        struct TableBody {
            enum class State : std::uint8_t {EnclosureUnavailable,ContextMismatch,SourceMismatch,SequenceMismatch,SourceStateMismatch,DecodeFailed,ModelMismatch,Matched};
            State state{State::EnclosureUnavailable};
            std::shared_ptr<const TableInput> source,typed;
            std::shared_ptr<const DRW_DxfEntitiesSource> entitiesPhysical;
            std::shared_ptr<const DRW_DxfEntitiesDeliverySource> entitiesDelivery;
            std::size_t dispatch{0},first{0},next{0},failedRecord{0};
            dxfTable::SourceState replay;
            std::vector<dxfCompound::SourceState> linked;
            std::vector<dx_attdef_source::Audit> profiles;
            bool sourceMatched{false},typedMatched{false},semanticComplete{false},profilesMatched{false};
        };
        std::map<std::uint32_t,std::shared_ptr<const TableBody>> tableBodies,tableBodyOwners;
        std::size_t tableBodyRecords{0},tableBodyBytes{0},tableBodyWork{0};
        std::size_t maxTableBodyRecords{100000u},maxTableBodyBytes{64u*1024u*1024u},maxTableBodyWork{64u*1024u*1024u};
        bool tableBodyCaptureFailed{false};
        struct EntitiesCompoundProof {
            enum class State : std::uint8_t {OtherPrimary,Raw,NoCallback,ModelUnavailable,Identity,Sequence,Owner,DuplicateIdentity,
                Callback,FollowFlag,DecodeFailed,SourceStateMismatch,ModelMismatch,ProfileMismatch,SpaceMismatch,LayoutUnresolved,Matched};
            State state{State::OtherPrimary};
            std::size_t dispatch{0},first{0},next{0},event{0},failedRecord{0};
            std::vector<dx_attdef_source::Audit> profiles;
            std::vector<dxfCompound::SourceState> replayStates;
            AttdefSourcePins attdef;
            std::shared_ptr<const TableBody> table;
            bool attributeDefinition{false};
            bool tablePrimary{false},compound{false},primary{false},modeledMatched{false},profilesMatched{false},spaceMatched{false};
        };
        struct EntitiesCompoundAudit {
            const void* container{nullptr};std::uint64_t generation{0},containerId{0};std::size_t section{0};
            std::shared_ptr<const DRW_DxfEntitiesSource> physical;
            std::shared_ptr<const DRW_DxfEntitiesDeliverySource> delivery;
            std::shared_ptr<const std::vector<EntitiesObservedValue>> observed;
            std::shared_ptr<const EntitiesReadContext> context;
            std::vector<dx_block_membership::Member> members;
            std::vector<EntitiesCompoundProof> proofs;
        };
        struct EntitiesDelivery {
            struct Certificate {
                const void* container{nullptr};std::uint64_t generation{0},containerId{0};std::size_t section{0},values{0},models{0},raw{0};
                std::shared_ptr<const DRW_DxfEntitiesSource> physical;
                std::shared_ptr<const DRW_DxfEntitiesDeliverySource> delivery;
                std::shared_ptr<const std::vector<EntitiesObservedValue>> observed;
                std::shared_ptr<const EntitiesReadContext> context;
            };
            const void* container{nullptr};std::uint64_t generation{0},containerId{0};std::size_t section{0};
            std::shared_ptr<const DRW_DxfEntitiesSource> physical;
            std::shared_ptr<const DRW_DxfEntitiesDeliverySource> delivery;
            std::shared_ptr<const std::vector<EntitiesObservedValue>> observed;
            std::shared_ptr<const Certificate> certificate;
            std::shared_ptr<const EntitiesReadContext> context;
        };
        struct EntitiesPending {
            const void* container{nullptr};std::uint64_t generation{0},scope{0},containerId{0};std::size_t section{0},next{0};
            bool pending{false};std::vector<EntitiesObservedValue> observed;
            std::shared_ptr<const EntitiesReadContext> context;
        } entitiesPending;
        std::vector<EntitiesDelivery> entitiesDeliveries;
        std::size_t entitiesDeliveryScopes{0},entitiesDeliveryDispatches{0},entitiesDeliveryEvents{0};
        std::size_t entitiesDeliveryBytes{0},entitiesDeliveryWork{0},entitiesDeliveryModels{0},entitiesDeliveryRaw{0};
        std::size_t maxEntitiesDeliveryScopes{1024u},maxEntitiesDeliveryDispatches{100000u},maxEntitiesDeliveryEvents{4000000u};
        std::size_t maxEntitiesDeliveryBytes{64u*1024u*1024u},maxEntitiesDeliveryWork{64u*1024u*1024u};
        bool entitiesDeliveryCaptureFailed{false};
        const dxfRW* entitiesReadInput{nullptr}; // borrowed only within actual fileImport
        std::vector<std::shared_ptr<const EntitiesCompoundAudit>> entitiesCompoundAudits;
        // Pin the actual published audit owners separately from the query rows.
        // A copied/substituted cache must not borrow its input certificates.
        std::vector<std::shared_ptr<const EntitiesCompoundAudit>> entitiesCompoundOwners;
        std::size_t entitiesCompoundRecords{0},entitiesCompoundBytes{0},entitiesCompoundWork{0};
        std::size_t maxEntitiesCompoundRecords{100000u},maxEntitiesCompoundBytes{64u*1024u*1024u},maxEntitiesCompoundWork{64u*1024u*1024u};
        bool entitiesCompoundCaptureFailed{false};
        struct LayoutInput {
            const void* container{nullptr};std::uint64_t generation{0},containerId{0};
            std::shared_ptr<const DRW_Layout> model;
            std::shared_ptr<const DRW_RawDxfObject> raw;
            std::shared_ptr<const EntitiesReadContext> context;
            dxfLayout::SourceState state;
        };
        enum class LayoutPair : std::uint8_t {Pending,Matched,ModelMismatch,StateMismatch};
        struct LayoutCaptured {std::shared_ptr<const LayoutInput> input;LayoutPair pair{LayoutPair::Pending};};
        struct LayoutAudit {
            enum class State : std::uint8_t {DecodeFailed,PairMismatch,StateMismatch,ModelMismatch,ProfileMismatch,Matched};
            State state{State::DecodeFailed};std::size_t index{0};
            std::shared_ptr<const LayoutInput> input;dx_layout_source::Audit profile;dxfLayout::SourceState replayState;
            bool modeledMatched{false},profilesMatched{false};
        };
        std::map<std::uint32_t,LayoutCaptured> layoutInputs;
        std::map<std::uint32_t,std::shared_ptr<const LayoutAudit>> layoutAudits;
        std::vector<std::shared_ptr<const LayoutAudit>> layoutAuditOwners;
        const DRW_Layout* layoutPendingModel{nullptr};std::uint32_t layoutPendingHandle{0};
        std::size_t layoutRecords{0},layoutGroups{0},layoutBytes{0},layoutWork{0};
        std::size_t maxLayoutRecords{100000u},maxLayoutGroups{4000000u},maxLayoutBytes{64u*1024u*1024u},maxLayoutWork{64u*1024u*1024u};
        bool layoutCaptureFailed{false};
        struct IdentityInput {
            const void* container{nullptr};std::uint64_t generation{0},containerId{0};
            std::shared_ptr<const DRW_DxfIdentitySource> source;
            std::shared_ptr<const EntitiesReadContext> context;
        };
        struct IdentityAudit {
            std::shared_ptr<const IdentityInput> input;
            std::map<std::uint64_t,std::size_t> identities;
            std::size_t missing{0},duplicates{0},unknown{0},admitted{0},unobserved{0},duplicateAdmissions{0},opaque{0},dimstyleNames{0};
            bool namespaceReady{false};
        };
        std::shared_ptr<const IdentityInput> identityInput,identityInputOwner;
        std::shared_ptr<const IdentityAudit> identityAudit,identityAuditOwner;
        std::size_t identityBytes{0},identityWork{0};
        std::size_t maxIdentityBytes{64u*1024u*1024u},maxIdentityWork{64u*1024u*1024u};
        bool identityCaptureFailed{false};
        struct StructuralInput {
            const void* container{nullptr};std::uint64_t generation{0},containerId{0};
            std::size_t index{0};
            dxfRW::DxfStructuralSourceView::Kind kind{dxfRW::DxfStructuralSourceView::Kind::None};
            std::pair<bool,std::uint8_t> originalState;
            std::shared_ptr<const DRW_TableEntry> model;
            std::shared_ptr<const DRW_RawDxfObject> raw;
            std::shared_ptr<const EntitiesReadContext> context;
            bool paired{false};
        };
        struct StructuralPending {
            StructuralInput input;const DRW_TableEntry* model{nullptr};const DRW_RawDxfObject* raw{nullptr};
        };
        struct StructuralAudit {
            std::shared_ptr<const StructuralInput> input;
            std::shared_ptr<const IdentityAudit> identities;
            std::size_t physical{DRW_DxfIdentitySource::None};
            std::size_t index{0};
            bool bodyMatched{false},stateComplete{false},originBound{false};
        };
        std::map<std::uint32_t,StructuralPending> structuralPending;
        std::map<std::uint32_t,std::shared_ptr<const StructuralInput>> structuralInputs;
        std::vector<std::shared_ptr<const StructuralInput>> structuralInputOwners;
        std::map<std::uint32_t,std::shared_ptr<const StructuralAudit>> structuralAudits;
        std::vector<std::shared_ptr<const StructuralAudit>> structuralAuditOwners;
        std::size_t structuralRecords{0},structuralBytes{0},structuralWork{0};
        std::size_t maxStructuralRecords{100000u},maxStructuralBytes{64u*1024u*1024u},maxStructuralWork{64u*1024u*1024u};
        bool structuralCaptureFailed{false};
        struct LayoutRoleAudit {
            enum class State {NotCaptured,Reading,ImportFailed,ContextMismatch,SourceNotReady,CaptureFailed,NoLayouts,
                MissingRoot,AmbiguousRoot,MissingBody,MissingLayoutDictionary,WrongOwner,InvalidNames,ReferenceMismatch,Matched};
            State state{State::SourceNotReady};
            const void* container{nullptr};std::uint64_t generation{0},containerId{0};
            std::shared_ptr<const IdentityInput> identity;
            std::shared_ptr<const IdentityAudit> identities;
            std::vector<std::shared_ptr<const StructuralAudit>> structural;
            std::vector<std::shared_ptr<const LayoutAudit>> layouts;
            std::size_t blocks{0},dictionaryEntries{0},associations{0};
            std::uint32_t root{0},layoutDictionary{0},firstHandle{0};
        };
        std::shared_ptr<const LayoutRoleAudit> layoutRoleAudit,layoutRoleOwner;
        std::size_t layoutRoleBytes{0},layoutRoleWork{0};
        std::size_t maxLayoutRoleBytes{64u*1024u*1024u},maxLayoutRoleWork{64u*1024u*1024u};
        bool layoutRoleCaptureFailed{false};
        struct EntitiesRoleAudit {
            enum class State {NotCaptured,Reading,ImportFailed,ContextMismatch,CaptureFailed,SourceNotReady,NoSelected,SelectedRolesReady,Unqualified};
            enum class Reason {Matched,UnsupportedPrimary,BodyNotReady,OriginMismatch,OwnerMissing,NotLayoutBlock,InactivePaper,SpaceConflict,LayoutConflict,TextUnqualified};
            struct Row {
                std::size_t section{0},record{0},physical{0},layoutGroup{0};
                std::uint32_t self{0},wireOwner{0},block{0},layout{0};
                Reason reason{Reason::BodyNotReady};bool primary{false},paper{false},spacePresent{false},layoutPresent{false};
            };
            State state{State::SourceNotReady};
            const void* container{nullptr};std::uint64_t generation{0},containerId{0};
            std::shared_ptr<const LayoutRoleAudit> graph;
            std::vector<std::shared_ptr<const EntitiesCompoundAudit>> scopes;
            std::vector<Row> rows;
            std::size_t selected{0},matched{0},unsupported{0};
        };
        std::shared_ptr<const EntitiesRoleAudit> entitiesRoleAudit,entitiesRoleOwner;
        std::size_t entitiesRoleBytes{0},entitiesRoleWork{0};
        std::size_t maxEntitiesRoleBytes{64u*1024u*1024u},maxEntitiesRoleWork{64u*1024u*1024u};
        bool entitiesRoleCaptureFailed{false};
        struct BlockRoleAudit {
            enum class State {NotCaptured,Reading,ImportFailed,ContextMismatch,CaptureFailed,SourceNotReady,NoScopes,RolesReady,Unqualified};
            enum class Reason {Matched,InvalidBoundary,NotPublished,SourceNotReady,OriginMismatch,BodyNotReady,UnsupportedPrimary,
                OwnerMissing,WrongPlacement,DuplicateDefinition,SpaceConflict,LayoutConflict,LayoutUnresolved,TextUnqualified};
            struct Scope {
                std::size_t section{0},first{0},last{0};std::uint32_t handle{0},block{0},layout{0};
                Reason reason{Reason::NotPublished};bool boundary{false},paper{false},definition{false};
                std::shared_ptr<const BlockOriginAudit> origin;
                std::shared_ptr<const dx_block_membership::Audit> membership;
                std::shared_ptr<const StructuralAudit> record;
                std::vector<std::pair<std::size_t,std::shared_ptr<const CompoundBody>>> bodies;
                std::vector<std::pair<std::size_t,std::shared_ptr<const TableBody>>> tableBodies;
            };
            struct Row {
                std::size_t scope{0},physical{0},parent{0},layoutGroup{0};std::uint32_t self{0},wireOwner{0},block{0},layout{0};
                Reason reason{Reason::BodyNotReady};bool endpoint{false},primary{false},selected{false},paper{false},definition{false},spacePresent{false},layoutPresent{false};
                int space{0};
            };
            State state{State::SourceNotReady};const void* container{nullptr};std::uint64_t generation{0},containerId{0};
            std::shared_ptr<const IdentityInput> identity;std::shared_ptr<const IdentityAudit> identities;
            std::shared_ptr<const LayoutRoleAudit> graph;
            std::vector<Scope> scopes;std::vector<Row> rows;
            std::size_t records{0},published{0};bool censusComplete{false};
        };
        std::shared_ptr<const BlockRoleAudit> blockRoleAudit,blockRoleOwner;
        std::size_t blockRoleBytes{0},blockRoleWork{0};
        std::size_t maxBlockRoleBytes{64u*1024u*1024u},maxBlockRoleWork{64u*1024u*1024u};
        bool blockRoleCaptureFailed{false};
        struct ResourceOriginInput {
            const void* container{nullptr};std::uint64_t generation{0},containerId{0};std::size_t index{0};
            dxfRW::DxfResourceSourceView::Kind kind{dxfRW::DxfResourceSourceView::Kind::None};
            std::shared_ptr<const DRW_TableEntry> model;std::shared_ptr<const DRW_RawDxfObject> raw;
            std::shared_ptr<const DRW_TableEntry> sourceModel;
            std::shared_ptr<const EntitiesReadContext> context;
            std::pair<bool,std::uint8_t> state;bool payloadComplete{false},paired{false};
        };
        struct ResourceOriginPending {
            ResourceOriginInput input;const DRW_TableEntry* model{nullptr};const DRW_RawDxfObject* raw{nullptr};
        };
        struct ResourceOriginAudit {
            std::shared_ptr<const ResourceOriginInput> input;std::shared_ptr<const IdentityAudit> identities;
            std::size_t index{0},physical{DRW_DxfIdentitySource::None};bool bodyMatched{false},stateComplete{false},originBound{false};
        };
        std::map<std::uint32_t,ResourceOriginPending> resourceOriginPending;
        std::map<std::uint32_t,std::shared_ptr<const ResourceOriginInput>> resourceOriginInputs;
        std::vector<std::shared_ptr<const ResourceOriginInput>> resourceOriginInputOwners;
        std::map<std::uint32_t,std::shared_ptr<const ResourceOriginAudit>> resourceOriginAudits;
        std::vector<std::shared_ptr<const ResourceOriginAudit>> resourceOriginAuditOwners;
        std::size_t resourceOriginRecords{0},resourceOriginRawOnly{0},resourceOriginBytes{0},resourceOriginWork{0};
        std::size_t maxResourceOriginRecords{100000u},maxResourceOriginBytes{64u*1024u*1024u},maxResourceOriginWork{64u*1024u*1024u};
        bool resourceOriginCaptureFailed{false};
        struct PlotSourceInput {
            const void* container{nullptr};std::uint64_t generation{0},containerId{0};std::size_t index{0};
            std::shared_ptr<const DRW_RawDxfObject> raw;
            std::shared_ptr<const DRW_DictionaryWithDefault> model;
            std::shared_ptr<const EntitiesReadContext> context;
            std::pair<bool,std::uint8_t> originalState{};
            bool dictionary{false},paired{false};
        };
        struct PlotSourcePending {PlotSourceInput input;const DRW_TableEntry* model{nullptr};const DRW_RawDxfObject* raw{nullptr};};
        struct PlotSourceRecord {
            std::shared_ptr<const PlotSourceInput> input;std::size_t physical{DRW_DxfIdentitySource::None};
            std::vector<dx_tableResourceReference> references;
            bool bodyMatched{false},originBound{false};
        };
        struct PlotSourceAudit {
            enum class State {NotCaptured,Reading,ImportFailed,ContextMismatch,CaptureFailed,SourceNotReady,NoSources,SourcesReady,Unqualified};
            State state{State::SourceNotReady};const void* container{nullptr};std::uint64_t generation{0},containerId{0};
            std::shared_ptr<const IdentityInput> identity;std::shared_ptr<const IdentityAudit> identities;
            std::shared_ptr<const StructuralAudit> root;
            std::size_t rootKeyGroup{DRW_DxfIdentitySource::None},rootTargetGroup{DRW_DxfIdentitySource::None};
            std::map<std::uint32_t,std::shared_ptr<const PlotSourceRecord>> records;
            std::set<std::uint32_t> plotTargets;
            std::uint32_t dictionary{0},defaultTarget{0};std::size_t physicalRecords{0},namedTargets{0};
        };
        std::map<std::uint32_t,PlotSourcePending> plotSourcePending;
        std::map<std::uint32_t,std::shared_ptr<const PlotSourceInput>> plotSourceInputs;
        std::vector<std::shared_ptr<const PlotSourceInput>> plotSourceInputOwners;
        std::shared_ptr<const PlotSourceAudit> plotSourceAudit,plotSourceOwner;
        std::size_t plotSourceRecords{0},plotSourceGroups{0},plotSourceBytes{0},plotSourceWork{0};
        std::size_t maxPlotSourceRecords{100000u},maxPlotSourceGroups{4000000u},maxPlotSourceBytes{64u*1024u*1024u},maxPlotSourceWork{128u*1024u*1024u};
        bool plotSourceCaptureFailed{false};
        struct ViewUcsAudit;
        struct ViewUcsRecord;
        struct TableReferenceAudit {
            enum class State {NotCaptured,Reading,ImportFailed,ContextMismatch,CaptureFailed,SourceNotReady,NoTables,NoReferences,ReferencesReady,Unqualified};
            enum class Reason {Matched,NullValue,NullRequired,MissingTarget,WrongKind,OriginalTargetUnavailable,AmbiguousOrigin,WrongCellBlock};
            enum class Provider {Structural,Layout,Entities,Blocks,Resource,PlotSource,ViewUcs};
            struct Target {
                Provider provider{Provider::Structural};std::size_t physical{DRW_DxfIdentitySource::None};
                std::uint32_t block{0};bool ambiguous{false};
                std::shared_ptr<const StructuralAudit> structural;
                std::shared_ptr<const LayoutAudit> layout;
                std::shared_ptr<const ViewUcsRecord> viewUcs;
                std::shared_ptr<const ResourceOriginAudit> resource;
                std::shared_ptr<const PlotSourceRecord> plot;
            };
            struct Row {
                std::uint32_t table{0},target{0},cellBlock{0};std::size_t group{0},physical{DRW_DxfIdentitySource::None};
                dx_attdef_source::Role role{dx_attdef_source::Role::None};Reason reason{Reason::OriginalTargetUnavailable};
            };
            State state{State::SourceNotReady};const void* container{nullptr};std::uint64_t generation{0},containerId{0};
            std::shared_ptr<const IdentityInput> identity;std::shared_ptr<const IdentityAudit> identities;
            std::shared_ptr<const EntitiesRoleAudit> entities;std::shared_ptr<const BlockRoleAudit> blocks;
            std::shared_ptr<const PlotSourceAudit> plots;
            std::shared_ptr<const ViewUcsAudit> views;
            std::vector<std::shared_ptr<const TableBody>> tables;
            std::map<std::uint32_t,Target> targets;
            std::vector<Row> rows;
        };
        std::shared_ptr<const TableReferenceAudit> tableReferenceAudit,tableReferenceOwner;
        std::size_t tableReferenceBytes{0},tableReferenceWork{0};
        std::size_t maxTableReferenceBytes{64u*1024u*1024u},maxTableReferenceWork{64u*1024u*1024u},maxTableReferenceRows{200000u};
        bool tableReferenceCaptureFailed{false};
        struct ViewUcsInput {
            const void* container{nullptr};std::uint64_t generation{0},containerId{0};std::size_t index{0};
            bool view{false},paired{false},payloadComplete{false};std::pair<bool,std::uint8_t> state;
            std::shared_ptr<const DRW_TableEntry> model;std::shared_ptr<const DRW_RawDxfObject> raw;
            std::shared_ptr<const EntitiesReadContext> context;
        };
        struct ViewUcsPending {ViewUcsInput input;const DRW_TableEntry* model{nullptr};const DRW_RawDxfObject* raw{nullptr};};
        struct ViewUcsRecord {
            std::shared_ptr<const ViewUcsInput> input;dx_viewucs_source::Audit profile;
            std::size_t physical{DRW_DxfIdentitySource::None};bool bodyMatched{false},originBound{false},stateComplete{false};
            std::vector<dx_tableResourceReference> references;
        };
        struct ViewUcsAudit {
            enum class State {NotCaptured,Reading,ImportFailed,ContextMismatch,CaptureFailed,SourceNotReady,NoSources,SourcesReady,Unqualified};
            State state{State::SourceNotReady};const void* container{nullptr};std::uint64_t generation{0},containerId{0};
            std::shared_ptr<const IdentityInput> identity;std::shared_ptr<const IdentityAudit> identities;
            std::map<std::uint32_t,std::shared_ptr<const ViewUcsRecord>> records;std::size_t physicalRecords{0};
        };
        std::map<std::uint32_t,ViewUcsPending> viewUcsPending;
        std::map<std::uint32_t,std::shared_ptr<const ViewUcsInput>> viewUcsInputs;
        std::vector<std::shared_ptr<const ViewUcsInput>> viewUcsInputOwners;
        std::shared_ptr<const ViewUcsAudit> viewUcsAudit,viewUcsOwner;
        std::size_t viewUcsRecords{0},viewUcsGroups{0},viewUcsBytes{0},viewUcsWork{0};
        std::size_t viewUcsInputBytes{0},viewUcsInputWork{0};
        std::size_t maxViewUcsRecords{100000u},maxViewUcsGroups{4000000u},maxViewUcsBytes{64u*1024u*1024u},maxViewUcsWork{128u*1024u*1024u};
        bool viewUcsCaptureFailed{false};
        struct DependencyAudit {
            enum class State {NotCaptured,Reading,ImportFailed,ContextMismatch,CaptureFailed,SourceNotReady,NoSources,OriginalTargetsMatched,Unqualified};
            enum class Reason {Matched,NullOptional,Inherited,Literal,OriginalTableOwner,MissingTarget,WrongKind,OriginalUnavailable,
                MissingName,AmbiguousName,NameSourceUnavailable,InvalidName,UnknownRole,NullRequired,WrongOwner,OwnershipCycle,OwnershipUnknown,
                CoupledTargetMismatch,CoupledTargetUnavailable,Inactive};
            // Effective original edge policy, separate from its physical code.
            enum class Ownership {None,PhysicalHard,DictionaryFlagHard,ExplicitSoft,Unqualified};
            struct Node {
                std::uint32_t handle{0};std::size_t physical{DRW_DxfIdentitySource::None};
                dx_tableResourceFamily family{dx_tableResourceFamily::Entity};
                std::shared_ptr<const ResourceOriginAudit> resource;
                std::shared_ptr<const StructuralAudit> structural;
                std::shared_ptr<const PlotSourceRecord> plot;
                std::shared_ptr<const LayoutAudit> layout;
                std::shared_ptr<const ViewUcsRecord> viewUcs;
                std::array<int,3> layoutModes{{-1,-1,-1}};
                std::array<std::size_t,3> layoutModeGroups{{DRW_DxfIdentitySource::None,DRW_DxfIdentitySource::None,DRW_DxfIdentitySource::None}};
                std::shared_ptr<const TableBody> table;std::size_t tableRecord{0};
                std::shared_ptr<const EntitiesRoleAudit> entities;
                std::shared_ptr<const BlockRoleAudit> blocks;std::size_t entityRow{0};
                bool entity{false};std::uint32_t originalOwner{0};
                std::vector<dx_tableResourceReference> references;
            };
            struct Row {
                std::uint32_t source{0},target{0};std::size_t group{DRW_DxfIdentitySource::None};int code{0};
                dx_tableResourceRole role{dx_tableResourceRole::Unknown};Reason reason{Reason::OriginalUnavailable};
                Ownership ownership{Ownership::None};std::size_t dictionaryFlagGroup{DRW_DxfIdentitySource::None};
                int mode{-1};std::size_t modeGroup{DRW_DxfIdentitySource::None};
                // Original TABLE geometry pair only; preserve each physical
                // role and its own target even when the other side disagrees.
                bool geometryCoupled{false};std::size_t coupledGroup{DRW_DxfIdentitySource::None};std::uint32_t coupledTarget{0};
                bool present{true},name{false};UTF8STRING key;
            };
            struct NamedTarget {std::uint32_t handle{0};bool ambiguous{false};};
            State state{State::SourceNotReady};const void* container{nullptr};std::uint64_t generation{0},containerId{0};
            std::shared_ptr<const TableReferenceAudit> targets;
            std::map<std::uint32_t,Node> nodes;
            std::map<std::pair<dx_tableResourceFamily,UTF8STRING>,NamedTarget> names;
            std::array<std::size_t,7> physicalNames{},backedNames{};
            std::array<bool,7> nameCensusComplete{};
            std::vector<Row> rows;std::size_t unavailableSources{0};
            std::size_t physicalEntityRows{0},backedEntityRows{0},entityReferences{0};
            std::size_t physicalLayoutRows{0},backedLayoutRows{0},layoutReferences{0};
        };
        std::shared_ptr<const DependencyAudit> dependencyAudit,dependencyOwner;
        std::size_t dependencyBytes{0},dependencyWork{0},dependencyRowsReserved{0};
        std::size_t maxDependencyBytes{128u*1024u*1024u},maxDependencyWork{128u*1024u*1024u},maxDependencyRows{400000u};
        bool dependencyCaptureFailed{false};
    } m_tableDxfSource;
    friend class DxIfaceTableSourceTestAccess;
    struct ClassEvidenceImport {
        dx_class_evidence::Result result;
        DRW_DwgClassSourceReport source;
        std::shared_ptr<const dx_class_evidence::Rows> rows;
        std::size_t retainedBytes{0};
        std::uint16_t codePage{0};
        bool codePagePresent{false}, codePageRecognized{false};
        bool rowsSeen{false}, sourceSeen{false}, closed{false};
    };
    std::map<std::uint64_t, ClassEvidenceImport> m_classEvidenceImports;
    dx_class_evidence::Result m_classEvidenceFailure;
    std::uint64_t m_classEvidenceAttempt{0}, m_classEvidenceGeneration{0}, m_classEvidenceHighWater{0};
    const void* m_classEvidenceOwner{nullptr};
    std::size_t m_classEvidenceBytes{0}, m_classEvidenceByteLimit{DRW::kMaxDwgClassSourceBytes};
    std::size_t m_classEvidenceCountLimit{1024u};
    bool m_classEvidenceFallback{true};
    std::map<std::uint64_t, DRW_DwgClassSourceReport> m_classSourceReports;
    DRW_DwgClassSourceReport m_classSourceFailure;
    std::uint64_t m_classSourceFailureGeneration{0};
    std::size_t m_classSourceRetainedBytes{0};
    std::size_t m_classSourceByteLimit{DRW::kMaxDwgClassSourceBytes};
    std::size_t m_classSourceReportLimit{1024u};
    friend class DxIfaceDwgClassSourceTestAccess;
    struct AcdsRawDxfSectionBaseline {
        std::uint64_t generation{0};
        std::size_t sectionIndex{0};
        DRW_RawDxfSection section;
    };
    // Exact import-time opaque carrier, not a semantic/kernel verdict. Charged
    // as a second owner to the existing raw-section groups/bytes budget.
    std::list<AcdsRawDxfSectionBaseline> m_acdsRawDxfSectionBaselines;
    bool m_acdsRawDxfSectionCaptureFailed{false};
    struct StandardDwgObjectFrame {
        DRW_UnsupportedObject raw;
        DRW_DwgFramePublication publication;
        DRW_DwgDictionaryMembership dictionaryMembership;
        DRW_DwgGroupMembership groupMembership;
        // Import-matched canonical model. Never refreshed from editable state
        // at export, and never itself evidence of actual output preservation.
        std::shared_ptr<const DRW_TableEntry> typedBaseline;
        std::uint64_t generation{0};
        std::uint16_t sourceCodePageId{0};
        bool sourceCodePageFieldPresent{false};
        bool sourceCodePageRecognized{false};
        bool sourceProfileBound{false};
        bool publicationSeen{false};
        bool membershipSeen{false};
        bool payloadMatched{false};
        bool opaquePayload{false};
        // Frozen observations from the import-matched raw reparse, never
        // inferred from the resulting scalar or refreshed at export.
        bool textDecodeObserved{false};
        std::uint32_t textDecodeIssues{0u};
        std::vector<std::uint32_t> appIdDependencies;
        std::vector<std::uint64_t> layerDependencies;
        bool hasNoObservedTextDecodeIssues() const noexcept {
            return textDecodeObserved && textDecodeIssues == 0u;
        }
    };
    // Source-bound staging only. Never replay these bytes as DXF, and never
    // use these counts to waive strict semantic/output completeness.
    using StandardDwgFrameKey = std::pair<std::uint64_t, std::uint32_t>;
    using StandardDwgOrdinalKey = std::pair<std::uint64_t, std::uint64_t>;
    using StandardDwgLocationKey = std::pair<std::uint64_t,
        std::pair<DRW_DwgFrameOffsetSpace, std::uint32_t>>;
    // IMAGEDEF/reactor source correlation is deliberately separate from the
    // qualified standard-object output lane. These records never increase its
    // completeness counters or acquire output certificates by self-readback.
    struct RasterDwgNameEvent {
        std::uint64_t ordinal{0u};
        std::uint32_t handle{0u};
        UTF8STRING name;
        std::shared_ptr<const DRW_DwgTableNameSource> source;
        bool duplicate{false};
    };
    struct RasterDwgNameImport {
        std::uint64_t attempt{0u}, generation{0u}, appEvents{0u}, layerEvents{0u};
        DRW::Version version{DRW::UNKNOWNV};
        std::uint16_t codePage{0u};
        bool codePagePresent{false}, codePageRecognized{false}, closed{false}, valid{true};
        std::map<std::uint32_t, RasterDwgNameEvent> apps, layers;
        std::size_t retainedBytes{0u};
    };
    std::map<std::uint64_t, std::shared_ptr<RasterDwgNameImport>> m_rasterDwgNameImports;
    std::size_t m_rasterDwgNameBytes{0u}, m_rasterDwgNameByteLimit{64u * 1024u * 1024u};
    std::size_t m_rasterDwgNameImportLimit{1024u};
    struct RasterDwgDependency {
        enum class Binding { MissingSource, WideLayer, UnboundNameProfile, UnboundReadContext, SourceName };
        dx_raster_wire::EedToken wire;
        // Shared immutable callback owner, not another copy of its name bytes.
        std::shared_ptr<const DRW_TableEntry> source;
        // Stable map entry owned by the closed actual-read receipt. Neither
        // the entry nor its buffers may be refreshed after publication.
        std::shared_ptr<const RasterDwgNameImport> read;
        const RasterDwgNameEvent* event{nullptr};
        Binding binding{Binding::MissingSource};
    };
    struct RasterDwgObjectFrame : StandardDwgObjectFrame {
        std::uint64_t classReadAttempt{0}, classPublicationAttempt{0};
        dx_class_evidence::RasterBinding classBinding;
        bool classBindingObserved{false};
        std::shared_ptr<const RasterDwgNameImport> nameImport;
        std::size_t appIdBegin{0u}, layerBegin{0u};
        dx_raster_wire::Proof wire;
        dx_raster_wire::Profile profile;
        std::vector<RasterDwgDependency> eedDependencies;
        std::vector<dx_raster_wire::EedToken> eedStrings;
        std::uint32_t profileIssues{0};
        bool dependenciesObserved{false};
        bool wireObserved{false};
    };
    std::map<StandardDwgFrameKey, RasterDwgObjectFrame> m_rasterDwgSources;
    std::map<StandardDwgOrdinalKey, std::uint32_t> m_rasterDwgOrdinals;
    std::map<StandardDwgLocationKey, std::uint32_t> m_rasterDwgLocations;
    std::size_t m_rasterDwgSourceBytes{0};
    std::uint64_t m_rasterDwgClosedThrough{0};
    bool m_rasterDwgSourceFailed{false};
    // Import-callback snapshots, separate from editable symbol-table rows.
    // Name source bytes/profile/issues remain immutable inside DRW_AppId.
    std::map<StandardDwgFrameKey, std::shared_ptr<const DRW_AppId>>
        m_standardDwgAppIdSources;
    std::map<StandardDwgFrameKey, std::shared_ptr<const DRW_Layer>>
        m_standardDwgLayerSources;
    std::uint64_t m_standardDwgNamesClosedThrough{0u};
    std::map<StandardDwgFrameKey, StandardDwgObjectFrame> m_standardDwgObjectFrames;
    std::map<StandardDwgOrdinalKey, std::uint32_t> m_standardDwgObjectOrdinals;
    std::map<StandardDwgLocationKey, std::uint32_t> m_standardDwgObjectLocations;
    std::size_t m_standardDwgObjectRetainedBytes{0};
    std::size_t m_standardDwgObjectMembershipEntries{0};
    bool m_standardDwgObjectCaptureFailed{false};
    bool m_standardDwgObjectPayloadFailed{false};
    friend class DxIfaceStandardDwgCaptureTestAccess;
    // Immutable-by-API snapshots used to admit exact same-version DXF replay
    // of CELLSTYLEMAP. They are adapter provenance, not consumer-editable
    // drawing state. Snapshot storage shares the family resource ceilings
    // with the corresponding typed/raw retained carriers.
    friend class dx_iface;
    std::list<DRW_CellStyleMap> m_cellStyleMapDxfBaselines;
    std::size_t m_cellStyleMapDxfBaselineRetainedRecords{0};
    std::size_t m_cellStyleMapDxfBaselineRetainedItems{0};
    std::size_t m_cellStyleMapDxfBaselineRetainedBytes{0};
    bool m_cellStyleMapDxfBaselineCaptureFailed{false};
    std::list<DRW_RawDxfObject> m_rawCellStyleMapDxfBaselines;
};

#endif // DX_DATA_H
