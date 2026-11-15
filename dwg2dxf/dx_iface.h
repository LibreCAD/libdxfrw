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

#ifndef DX_IFACE_H
#define DX_IFACE_H

#include "drw_interface.h"
#include "libdxfrw.h"
#include "dx_data.h"
#include "intern/dxf_ac1027_bound_layer.h"
#include "dx_mline_source_dependency.h"
#include "intern/dxfcode.h"
#include "intern/dxfparserlimits.h"
#include "intern/drw_dbg.h"
#include "intern/dxfwholesource.h"
#include "intern/dwg_xrecord_verified_source.h"
#include "intern/dwg_datatable_ac1018_candidate.h"
#include "intern/dwg_tablecontent_ac1018_candidate.h"
#include "intern/dwg_tablegeometry_ac1018_candidate.h"
#include "intern/dwg_table_roundtrip_xrecord_ac1018_candidate.h"
#include "intern/dwg_table_child_xrecord_ac1018_candidate.h"
#include "intern/dwg_table_extension_dictionary_ac1018_candidate.h"
#include "intern/dwg_table_entity_xdictionary_ac1018_candidate.h"
#include "intern/dwg_tablestyle_ac1018_candidate.h"
#include "intern/dwg_cellstylemap_ac1018_candidate.h"
#include "intern/dwg_table_cache_membership_ac1018_candidate.h"
#include "intern/dwg_tablestyle_extension_dictionary_ac1018_candidate.h"
#include "intern/dwg_tablestyle_roundtrip_xrecord_ac1018_candidate.h"
namespace blockref_source_relation { struct Result; }

#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <array>
#include <iostream>
#include <iosfwd>
#include <limits>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class DxIfaceExtensionGraphTestAccess;
class DxIfaceTableSourceTestAccess;
class DxIfaceDwgLtypeControlOutputTestAccess;

class dx_iface : public DRW_Interface, public DRW_DxfClassSourceHandler, public DRW_DwgHeaderMaterialSourceHandler, public DRW_DwgProxyGraphicsResourceOrderHandler, public DRW_UnmodeledDxfObjectHandler,
                 public DRW_DwgDbColorEedSourceHandler,
                 public DRW_DwgClassSourceHandler, public DRW_DxfTableSourceHandler,
                 public DRW_DxfSymbolSourceHandler, public DRW_DxfRootDictionarySourceHandler,
                 public DRW_DxfBlockRecordSourceHandler, public DRW_DxfAttdefSourceHandler,
                 public DRW_DxfBlockSourceHandler, public DRW_DxfBlockChildrenSourceHandler,
                 public DRW_DxfBlockDeliveryHandler, public DRW_DxfEntitiesSourceHandler,
                 public DRW_DxfEntitiesDeliveryHandler, public DRW_DxfLayoutSourceHandler,
                 public DRW_DxfIdentitySourceHandler, public DRW_DxfDictionarySourceHandler,
                 public DRW_DxfResourceSourceHandler, public DRW_DxfNamespaceSourceHandler,
                 public DRW_DxfLayerTableControlHandler,
                 public DRW_DxfViewUcsSourceHandler,
                 public DRW_DxfObjectPhysicalSourcePolicy,
                 public DRW_DxfSparseMaterialRawPolicy,
                 public DRW_DxfPointCloudDefinitionSourceHandler,
                 public WholeSourceV2::Handler, public DwgVerifiedXRecordSink,
                 public DwgVerifiedTableCandidateSink,
                 public DwgVerifiedTableCacheSourceSink {
public:
    // Source observation only. This does not permit DWG or DXF replay.
    bool enableVerifiedXRecordSource(std::uint32_t handle) noexcept;
    // Empty disables observation; nonempty sets are all-or-none per import.
    bool enableVerifiedXRecordSources(const std::vector<std::uint32_t>& handles) noexcept;
    // Selected AC1018 STYLE_CONTROL-to-STYLE source observation. It does not
    // authorize resource replay or output.
    bool enableVerifiedStyleResourceSource(std::uint32_t styleControl,
                                           std::uint32_t styleRecord) noexcept;
    static bool hasUnchangedVerifiedStyleResourceSource(
        const dx_data&, std::uint32_t styleControl,
        std::uint32_t styleRecord) noexcept;
    // Bounded AC1018 LTYPE_CONTROL plus Continuous/ByBlock/ByLayer source
    // observation. This never authorizes output or a producer-wide order.
    bool enableVerifiedLTypeResourceSource(std::uint32_t controlHandle,
        std::uint32_t ordinaryHandle, std::uint32_t byBlockHandle,
        std::uint32_t byLayerHandle) noexcept;
    static bool hasUnchangedVerifiedLTypeResourceSource(const dx_data&,
        std::uint32_t controlHandle, std::uint32_t ordinaryHandle,
        std::uint32_t byBlockHandle, std::uint32_t byLayerHandle) noexcept;
    static bool hasUnchangedVerifiedXRecordSource(
        const dx_data&, std::uint32_t handle) noexcept;
    static bool hasUnchangedVerifiedXRecordSources(
        const dx_data&, const std::vector<std::uint32_t>& handles) noexcept;
    // Opt-in same-import source observations only; never authorizes output.
    // Ordinary selections do not verify root XRECORD outgoing 360/361 pairs.
    // The stricter opt-in checks those three pairs; other graph dependencies
    // remain unverified. Reader classification binds class kind/names/number only;
    // other class metadata is callback data, not authenticated evidence.
    // Geometry selection binds its owner to the other candidates, not its
    // outgoing graph. Read with the overload matching the selected set; a
    // three-candidate selection cannot be read as a two-candidate bundle.
    bool enableVerifiedTableCandidates(const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable) noexcept;
    bool enableVerifiedTableCandidates(const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry) noexcept;
    // Binds the root XRECORD ordered references to all three selected frames.
    // This remains a source observation; it is not a graph or output certificate.
    bool enableVerifiedTableRoundtripCandidates(const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry) noexcept;
    // Stricter nine-child wire observation; still not output authorization.
    bool enableVerifiedTableRoundtripChildren(const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry) noexcept;
    // One-sided dictionary source evidence. Does not prove the selected TABLE
    // exists, is a TABLE, or points back to this dictionary; no output authority.
    bool enableVerifiedTableExtensionDictionary(const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        std::uint32_t tableEntity,std::uint32_t dictionary) noexcept;
    // Reader-source H3 reciprocity only. The live TABLE model is not frozen or
    // compared; this observation is never authorization to write/output.
    bool enableVerifiedTableReciprocalSource(const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        std::uint32_t tableEntity,std::uint32_t dictionary) noexcept;
    // Stricter opt-in: unique live TABLE must match a bounded private reader
    // snapshot. Still source evidence, never output or graph authorization.
    bool enableVerifiedTableModelSource(const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        std::uint32_t tableEntity,std::uint32_t dictionary) noexcept;
    // Also require the uniquely retained INSERT-shaped compatibility view to
    // match the private TABLE source. No block membership/output authority.
    bool enableVerifiedTableProjectionSource(const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        std::uint32_t tableEntity,std::uint32_t dictionary) noexcept;
    static bool hasUnchangedVerifiedTableProjectionSource(const dx_data&,
        const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        std::uint32_t tableEntity,std::uint32_t dictionary) noexcept;
    // Exact AC1018 *T1 cache membership observation layered on the verified
    // TABLE projection query. This is not output authorization or geometry
    // semantic evidence.
    static bool matchesVerifiedTableCacheMembershipCandidate(const dx_data&,
        const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        std::uint32_t tableEntity,std::uint32_t dictionary) noexcept;
    static bool hasUnchangedVerifiedTableCacheSourceFrames(
        const dx_data&, std::uint32_t blockRecordHandle,
        std::uint32_t blockHandle) noexcept;
    bool enableVerifiedTableCacheSourceFrames(
        std::uint32_t blockRecordHandle,
        std::uint32_t blockHandle) noexcept;
    std::uint32_t verifiedTableCacheBlockRecordSelection()
        const noexcept override;
    std::uint32_t verifiedTableCacheBlockSelection()
        const noexcept override;
    void verifiedTableCacheSource(
        const DwgVerifiedTableCacheSourceTicket&) noexcept override;
    // Reader-authenticated TABLESTYLE selected wire projection only; this
    // does not certify the complete typed model or grant output authority.
    bool enableVerifiedTableStyleSource(const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        std::uint32_t tableEntity,std::uint32_t dictionary,std::uint32_t tableStyle) noexcept;
    // Style extension dictionary entries are numeric targets only; this does
    // not authenticate the target frames/types or authorize output.
    bool enableVerifiedTableStyleDictionarySource(const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        std::uint32_t tableEntity,std::uint32_t dictionary,std::uint32_t style,
        std::uint32_t styleDictionary,std::uint32_t styleXRecord,std::uint32_t cellStyleMap) noexcept;
    static bool readVerifiedTableStyleDictionarySource(const dx_data&,
        const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        std::uint32_t tableEntity,std::uint32_t dictionary,std::uint32_t style,
        std::uint32_t styleDictionary,std::uint32_t styleXRecord,std::uint32_t cellStyleMap,
        DwgTableStyleExtensionDictionaryAc1018Candidate::Result& result) noexcept;
    // Additionally binds the dictionary's first target to a reader-verified
    // XRECORD source and its owner/reactor to the selected dictionary. The map
    // target, text semantics, other dependencies and output remain unqualified.
    bool enableVerifiedTableStyleXRecordSource(const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        std::uint32_t tableEntity,std::uint32_t dictionary,std::uint32_t style,
        std::uint32_t styleDictionary,std::uint32_t styleXRecord,std::uint32_t cellStyleMap) noexcept;
    // Opt-in selected CELLSTYLEMAP wire fields only, preserving source order;
    // does not certify the whole model, resources, graph or output.
    bool enableVerifiedTableCellStyleMapSource(const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        std::uint32_t tableEntity,std::uint32_t dictionary,std::uint32_t tableStyle,
        std::uint32_t styleDictionary,std::uint32_t styleXRecord,std::uint32_t cellStyleMap) noexcept;
    static bool readVerifiedTableCellStyleMapSource(const dx_data&,
        const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        std::uint32_t tableEntity,std::uint32_t dictionary,std::uint32_t tableStyle,
        std::uint32_t styleDictionary,std::uint32_t styleXRecord,std::uint32_t cellStyleMap,
        DwgCellStyleMapAc1018Candidate::Result& result) noexcept;
    // Composes the selected CELLSTYLEMAP bundle with same-import STYLE and
    // LTYPE control/record tickets. It verifies the selected profile's text
    // style and six border linetype handle targets; it grants no output or
    // font/rendering authority.
    bool enableVerifiedTableCellStyleResourceSource(
        const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        std::uint32_t tableEntity,std::uint32_t dictionary,std::uint32_t tableStyle,
        std::uint32_t styleDictionary,std::uint32_t styleXRecord,std::uint32_t cellStyleMap,
        std::uint32_t styleControl,std::uint32_t styleRecord,
        std::uint32_t ltypeControl,std::uint32_t ordinaryLtype,
        std::uint32_t byBlockLtype,std::uint32_t byLayerLtype) noexcept;
    static bool readVerifiedTableCellStyleResourceSource(const dx_data&,
        const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        std::uint32_t tableEntity,std::uint32_t dictionary,std::uint32_t tableStyle,
        std::uint32_t styleDictionary,std::uint32_t styleXRecord,std::uint32_t cellStyleMap,
        std::uint32_t styleControl,std::uint32_t styleRecord,
        std::uint32_t ltypeControl,std::uint32_t ordinaryLtype,
        std::uint32_t byBlockLtype,std::uint32_t byLayerLtype,
        DwgCellStyleMapAc1018Candidate::Result& result) noexcept;
    static bool readVerifiedTableStyleXRecordSource(const dx_data&,
        const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        std::uint32_t tableEntity,std::uint32_t dictionary,std::uint32_t style,
        std::uint32_t styleDictionary,std::uint32_t styleXRecord,std::uint32_t cellStyleMap,
        DwgTableStyleRoundtripXRecordAc1018Candidate::Result& result) noexcept;
    static bool readVerifiedTableStyleSource(const dx_data&,
        const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        std::uint32_t tableEntity,std::uint32_t dictionary,std::uint32_t tableStyle,
        DwgTableStyleAc1018Candidate::Result& result) noexcept;
    static bool hasUnchangedVerifiedTableModelSource(const dx_data&,
        const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        std::uint32_t tableEntity,std::uint32_t dictionary) noexcept;
    static bool readVerifiedTableReciprocalSource(const dx_data&,
        const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        std::uint32_t tableEntity,std::uint32_t dictionary,
        DwgTableEntityXDictionaryAc1018Candidate::Result& result) noexcept;
    static bool readVerifiedTableExtensionDictionary(const dx_data&,
        const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        std::uint32_t tableEntity,std::uint32_t dictionary,
        DwgTableExtensionDictionaryAc1018Candidate::Result& result) noexcept;
    static bool readVerifiedTableRoundtripChildren(const dx_data&,
        const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        DwgTableContentAc1018Candidate::Result& content,
        DwgDataTableAc1018Candidate::Result& table,
        DwgTableGeometryAc1018Candidate::Result& geometry,
        DwgTableRoundtripXRecordAc1018Candidate::Result& root,
        std::array<DwgTableChildXRecordAc1018Candidate::Result,9>& children) noexcept;
    static bool readVerifiedTableRoundtripCandidates(const dx_data&,
        const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        DwgTableContentAc1018Candidate::Result& content,
        DwgDataTableAc1018Candidate::Result& table,
        DwgTableGeometryAc1018Candidate::Result& geometry,
        DwgTableRoundtripXRecordAc1018Candidate::Result& root) noexcept;
    // Revalidate the complete selected import capability and project only the
    // root's ordered payload. Mapped target roles/identities, recursive output
    // closure, and permission to write remain the caller's responsibility.
    static bool projectVerifiedTableRoundtripReferences(const dx_data&,
        const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        const std::array<std::uint32_t,3>& mappedTargets,
        std::vector<DRW_Variant>& projectedEntries,
        DwgTableRoundtripXRecordAc1018Candidate::Result& root) noexcept;
    static bool readVerifiedTableCandidates(const dx_data&,
        const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,
        DwgTableContentAc1018Candidate::Result& content,
        DwgDataTableAc1018Candidate::Result& table) noexcept;
    static bool readVerifiedTableCandidates(const dx_data&,
        const std::vector<std::uint32_t>& xrecords,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        DwgTableContentAc1018Candidate::Result& content,
        DwgDataTableAc1018Candidate::Result& table,
        DwgTableGeometryAc1018Candidate::Result& geometry) noexcept;
    bool wantsDxfSparseMaterialRawCapture() const noexcept override {
        // Old subclasses may override callbacks without retaining raw records.
        // They must explicitly opt in instead of inheriting this policy.
        return typeid(*this) == typeid(dx_iface);
    }
    // Internal-only groundwork; opting in captures decoded physical scalars,
    // not source bytes, graph completeness or write/semantic eligibility.
    bool enableWholeDxfSourceObservation(bool enabled=true) noexcept {
        if (m_tableSourceReadData) {
            return false;
        }
        m_wholeEnabled = enabled;
        return true;
    }
    bool wantsWholeSource() const noexcept override;
    WholeSourceV2::Limits wholeSourceLimits() const noexcept override;
    void wholeSourceBegin(const void*) noexcept override;
    bool wholeSourceSelected(const void*) const noexcept override;
    void wholeSourcePending(const void*,const WholeSourceV2::Source&,const WholeSourceV2::Ticket&) noexcept override;
    void wholeSourceFinished(const void*,const WholeSourceV2::FinishToken&) noexcept override;
    // Explicit opt-in for an AC1018 whole-source identity observation.
    // Private groundwork only; there is no public observation accessor and it
    // establishes no namespace, ownership or output authority. Applies to
    // later imports; never alter it during a read.
    bool enableLegacyDxfIdentityObservation(bool enabled=true) noexcept {
        if (m_tableSourceReadData) return false;
        m_legacyIdentityObservationEnabled=enabled; return true;
    }

    using RenderingInputState=dx_data::TableDxfSourceCapture::RenderingInputAudit::State;
    struct RenderingInputReport {
        RenderingInputState state{RenderingInputState::NotCaptured};
        std::size_t records{0},physical{0},pairs{0},rawStable{0},stateStable{0},origins{0},bytes{0},work{0};
        bool publicationInputsCaptured{false};
        bool fullModelsMatched{false},originalSourcesMatched{false},ownerTargetsResolved{false},outputEligible{false};
    };
    static RenderingInputReport renderingInputReport(const dx_data&) noexcept;
    enum class RenderingModelState {NotCaptured,Reading,ImportFailed,ContextMismatch,CaptureFailed,Limited,SourceNotReady,NoInputs,ModeledStateMatched,Unqualified};
    struct RenderingModelReport {
        RenderingModelState state{RenderingModelState::NotCaptured};
        std::size_t records{0},decoded{0},matched{0},bytes{0},work{0};
        // Exact retained model/parser-state replay is separate from full
        // physical property coverage and dependency/output qualification.
        bool modeledStateMatched{false};
        bool propertyCoverageMatched{false},originalSourcesMatched{false},ownerTargetsResolved{false},outputEligible{false};
    };
    static RenderingModelReport renderingModelReport(const dx_data&) noexcept;
    using RenderingSourceState=dx_data::TableDxfSourceCapture::RenderingSourceAudit::State;
    struct RenderingSourceReport {
        RenderingSourceState state{RenderingSourceState::NotCaptured};
        std::size_t records{0},physical{0},profiles{0},origins{0},namedTargets{0},fields{0},references{0},bytes{0},work{0};
        // Complete selected canonical source shapes only. Other rendering
        // profiles, recursive/live/emitted and semantic support stay open.
        bool originalSourcesMatched{false};
        bool ownerTargetsResolved{false},recursiveClosureMatched{false},outputEligible{false};
    };
    static RenderingSourceReport renderingSourceReport(const dx_data&) noexcept;
    bool wantsDxfViewUcsSource() const override {return tableSourceCaptureOpen();}
    void addDxfViewUcsSource(const DRW_TableEntry&,const DRW_RawDxfObject&) override;
    using ViewUcsState=dx_data::TableDxfSourceCapture::ViewUcsAudit::State;
    struct ViewUcsReport {
        ViewUcsState state{ViewUcsState::NotCaptured};
        std::size_t records{0},physical{0},paired{0},bodies{0},origins{0},bytes{0},work{0};
        bool originalSourcesMatched{false},ownerTargetsResolved{false},outputEligible{false};
    };
    static ViewUcsReport viewUcsSourceReport(const dx_data&) noexcept;
    bool wantsDxfResourceSource() const override;
    bool wantsDxfNamespaceSource() const override;
    void addDxfNamespaceSource(const DRW_TableEntry&,const DRW_RawDxfObject&) override;
    bool wantsDxfLayerTableControl() const override;
    bool addDxfLayerTableControl(std::uint32_t handle,
                                 std::uint32_t xDictHandle) override;
    bool getDxfLayerTableControl(std::uint32_t& handle,
                                 std::uint32_t& xDictHandle) const override;
    enum class ResourceOriginState {NotCaptured,Reading,ImportFailed,ContextMismatch,CaptureFailed,SourceNotReady,NoOrigins,OriginsBound,Unqualified};
    struct ResourceOriginReport {
        ResourceOriginState state{ResourceOriginState::NotCaptured};
        std::size_t records{0},paired{0},stateComplete{0},bodyMatched{0},originsBound{0},rawOnly{0},bytes{0},work{0};
        bool ownerTargetsResolved{false},outputEligible{false};
    };
    static ResourceOriginReport resourceOriginReport(const dx_data&) noexcept;
    using PlotSourceState=dx_data::TableDxfSourceCapture::PlotSourceAudit::State;
    struct PlotSourceReport {
        PlotSourceState state{PlotSourceState::NotCaptured};
        std::size_t records{0},physicalRecords{0},dictionaries{0},placeholders{0},bodyMatched{0},originsBound{0},plotTargets{0},namedTargets{0},bytes{0},work{0};
        std::uint32_t defaultTarget{0};
        bool originalSourcesMatched{false},ownerTargetsResolved{false},outputEligible{false};
    };
    static PlotSourceReport plotStyleSourceReport(const dx_data&) noexcept;
    using DependencyState=dx_data::TableDxfSourceCapture::DependencyAudit::State;
    using DependencyReason=dx_data::TableDxfSourceCapture::DependencyAudit::Reason;
    struct DependencyReport {
        DependencyState state{DependencyState::NotCaptured};
        std::size_t sources{0},names{0},references{0},matched{0},inherited{0},literals{0},nulls{0},tableOwners{0},unavailableSources{0},bytes{0},work{0},reservedRows{0};
        std::size_t missing{0},wrongKind{0},unavailable{0},ambiguous{0},invalidNames{0},unknownRoles{0},wrongOwner{0},ownershipCycles{0};
        std::size_t hardOwnerReferences{0},dictionaryFlagHardOwners{0},explicitSoftOwners{0},unknownOwnership{0};
        std::size_t geometryPairs{0},geometryMatched{0},geometryMismatches{0},geometryUnavailable{0};
        std::size_t entitySources{0},entityReferences{0},physicalEntityRows{0},backedEntityRows{0};
        std::size_t layoutSources{0},layoutReferences{0},physicalLayoutRows{0},backedLayoutRows{0},inactiveReferences{0};
        std::size_t physicalNamedRows{0},backedNamedRows{0};bool nameNamespacesComplete{false};
        std::uint32_t firstSource{0},firstTarget{0};
        // Original selected source reference targets only. Recursive
        // dependency semantics, live state, emitted output and native consumers
        // remain separate; this does not certify whole-document ownership.
        bool originalTargetsMatched{false},recursiveClosureMatched{false},ownerTargetsResolved{false},outputEligible{false};
    };
    static DependencyReport dependencyReport(const dx_data&) noexcept;
    enum class TableInputState {NotCaptured,Reading,ImportFailed,ContextMismatch,CaptureFailed,NoInputs,OriginalPairsReady,Unqualified};
    struct TableInputReport {
        TableInputState state{TableInputState::NotCaptured};
        std::size_t sources{0},typed{0},paired{0},unavailable{0},records{0},bytes{0},work{0};
        // cleanStates means only original parent/linked XDATA cursors, not
        // complete TABLE cells,scalar semantics,dependencies or output.
        bool originalPairsMatched{false},cleanStates{false},modeledMatched{false},profilesMatched{false},ownerTargetsResolved{false},outputEligible{false};
    };
    static TableInputReport tableInputReport(const dx_data&) noexcept;
    enum class TableBodyState {NotCaptured,Reading,ImportFailed,ContextMismatch,CaptureFailed,NoInputs,ModeledMatched,Unqualified};
    struct TableBodyReport {
        TableBodyState state{TableBodyState::NotCaptured};
        std::size_t selected{0},matched{0},records{0},bytes{0},work{0},semanticComplete{0};
        // Selected modeled bodies only; physical profiles and actual target,
        // whole-document, geometry and output qualification remain separate.
        bool modeledMatched{false},profilesMatched{false},ownerTargetsResolved{false},outputEligible{false};
    };
    static TableBodyReport tableBodyReport(const dx_data&) noexcept;
    using TableReferenceState=dx_data::TableDxfSourceCapture::TableReferenceAudit::State;
    using TableReferenceReason=dx_data::TableDxfSourceCapture::TableReferenceAudit::Reason;
    struct TableReferenceReport {
        TableReferenceState state{TableReferenceState::NotCaptured};
        std::size_t tables{0},references{0},matched{0},nullValues{0},values{0},matchedValues{0},targets{0},bytes{0},work{0};
        std::size_t missing{0},wrongKind{0},unavailable{0},ambiguous{0},wrongCellBlock{0},nullRequired{0};
        std::uint32_t firstTable{0},firstTarget{0};
        // Original origins of the six inventoried TABLE handle roles. Absent
        // roles, target dependencies, common/name roles, edits and output
        // require separate qualification; this is not full TABLE closure.
        bool sourceTargetsMatched{false},valueSourceTargetsMatched{false},ownerTargetsResolved{false},outputEligible{false};
    };
    static TableReferenceReport tableReferenceReport(const dx_data&) noexcept;
    using BlockRoleState=dx_data::TableDxfSourceCapture::BlockRoleAudit::State;
    struct BlockRoleReport {
        BlockRoleState state{BlockRoleState::NotCaptured};
        std::size_t scopes{0},records{0},published{0},qualifiedScopes{0},selected{0},matched{0},unsupported{0},endpoints{0},children{0},paper{0},definitions{0},spacePresent{0},layoutPresent{0},bytes{0},work{0};
        bool physicalCensusComplete{false},selectedRolesMatched{false},wholeDocumentMatched{false},ownerTargetsResolved{false},nativeNamesQualified{false},outputEligible{false};
    };
    static BlockRoleReport blockRoleReport(const dx_data&) noexcept;
    enum class BlockOriginState {NotCaptured,Reading,ImportFailed,ContextMismatch,CaptureFailed,PublicationsReady,OriginsBound,Unqualified};
    struct BlockOriginReport {
        BlockOriginState state{BlockOriginState::NotCaptured};std::size_t blocks{0},publications{0},bound{0},events{0},bytes{0},work{0};
        bool ownerTargetsResolved{false},outputEligible{false};
    };
    static BlockOriginReport blockOriginReport(const dx_data&) noexcept;
    using EntitiesRoleState=dx_data::TableDxfSourceCapture::EntitiesRoleAudit::State;
    struct EntitiesRoleReport {
        EntitiesRoleState state{EntitiesRoleState::NotCaptured};
        std::size_t selected{0},matched{0},unsupported{0},primaries{0},children{0},model{0},paper{0},spacePresent{0},layoutPresent{0},bytes{0},work{0};
        bool selectedRolesMatched{false},wholeDocumentMatched{false},nativeNamesQualified{false},ownerTargetsResolved{false},outputEligible{false};
    };
    static EntitiesRoleReport entitiesRoleReport(const dx_data&) noexcept;
    using LayoutRoleState=dx_data::TableDxfSourceCapture::LayoutRoleAudit::State;
    struct LayoutRoleReport {
        LayoutRoleState state{LayoutRoleState::NotCaptured};
        std::size_t layouts{0},blocks{0},dictionaryEntries{0},associations{0},bytes{0},work{0};
        std::uint32_t root{0},layoutDictionary{0},firstHandle{0};
        bool rolesMatched{false},nativeNamesQualified{false},ownerTargetsResolved{false},outputEligible{false};
    };
    static LayoutRoleReport layoutRoleReport(const dx_data&) noexcept;
    enum class StructuralSourceState {NotCaptured,Reading,ImportFailed,ContextMismatch,CaptureFailed,ProducerPairsReady,OriginsBound,Unqualified};
    struct StructuralSourceReport {
        StructuralSourceState state{StructuralSourceState::NotCaptured};
        std::size_t records{0},paired{0},bodyMatched{0},stateComplete{0},originsBound{0},bytes{0},work{0};
        bool namespaceReady{false},ownerTargetsResolved{false},outputEligible{false};
    };
    static StructuralSourceReport structuralSourceReport(const dx_data&) noexcept;
    bool wantsDxfDictionarySource() const override;
    void addDxfDictionarySource(const DRW_Dictionary&,const DRW_RawDxfObject&) override;
    bool wantsDxfPointCloudDefinitionSource() const override;
    void addDxfPointCloudDefinitionSource(
        const DRW_PointCloudDef&, const DRW_RawDxfObject&,
        DRW_DxfObjectPhysicalSource::Status,
        std::shared_ptr<const DRW_DxfObjectPhysicalSource>) override;
    enum class IdentitySourceState {NotCaptured,Reading,ImportFailed,ContextMismatch,CaptureFailed,NamespaceReady,Unqualified};
    struct IdentitySourceReport {
        IdentitySourceState state{IdentitySourceState::NotCaptured};
        std::size_t sections{0},records{0},candidates{0},identities{0},missing{0},duplicates{0},unknown{0};
        std::size_t admitted{0},unobserved{0},duplicateAdmissions{0},opaque{0},dimstyleNames{0},bytes{0},work{0};
        // Source namespace only; no body, actual typed target or output proof.
        bool ownerTargetsResolved{false},outputEligible{false};
    };
    static IdentitySourceReport identitySourceReport(const dx_data&) noexcept;
    bool wantsDxfIdentitySource() const override;
    void addDxfIdentitySource(const DRW_DxfIdentitySource&) override;
    enum class LayoutSourceState {NotCaptured,Reading,ImportFailed,ContextMismatch,CaptureFailed,ProofsReady,Unqualified};
    struct LayoutSourceReport {
        LayoutSourceState state{LayoutSourceState::NotCaptured};
        std::size_t records{0},groups{0},bytes{0},work{0},matched{0},paired{0},modeledMatched{0},profilesMatched{0};
        std::uint32_t firstHandle{0};
        bool outputEligible{false},ownerTargetsResolved{false};
    };
    static LayoutSourceReport layoutSourceReport(const dx_data&) noexcept;
    bool wantsDxfLayoutSource() const override;
    void addDxfLayoutSource(const DRW_Layout&,const DRW_RawDxfObject&) override;
    enum class EntitiesCompoundState {NotCaptured,Reading,ImportFailed,SourceNotReady,CaptureFailed,ContextMismatch,NoCompounds,ProofsReady,Unqualified};
    struct EntitiesCompoundReport {
        // ProofsReady covers only the selected compound subset. Other bodies
        // have separate reports; raw/no-callback and actual targets stay open.
        EntitiesCompoundState state{EntitiesCompoundState::NotCaptured};
        std::size_t sections{0},dispatches{0},compounds{0},matched{0},modeledMatched{0},profilesMatched{0};
        std::size_t otherPrimaries{0},raw{0},noCallback{0},unavailable{0},records{0},bytes{0},work{0};
        std::size_t firstSection{0},firstDispatch{0},firstRecord{0};
        bool outputEligible{false},ownerTargetsResolved{false};
    };
    static EntitiesCompoundReport entitiesCompoundReport(const dx_data&) noexcept;
    enum class EntitiesPrimaryState {NotCaptured,Reading,ImportFailed,SourceNotReady,CaptureFailed,ContextMismatch,NoPrimaries,ProofsReady,Unqualified};
    struct EntitiesPrimaryReport {
        // Selected POINT/LINE bodies only, not all primaries or resolved targets.
        EntitiesPrimaryState state{EntitiesPrimaryState::NotCaptured};
        std::size_t sections{0},dispatches{0},primaries{0},matched{0},modeledMatched{0},profilesMatched{0},unavailable{0};
        std::size_t records{0},bytes{0},work{0},firstSection{0},firstDispatch{0},firstRecord{0};
        bool outputEligible{false},ownerTargetsResolved{false};
    };
    static EntitiesPrimaryReport entitiesPrimaryReport(const dx_data&) noexcept;
    enum class EntitiesDeliveryState {NotCaptured,Reading,ImportFailed,CaptureFailed,ContextMismatch,ReportMissing,ObservedTransportComplete};
    struct EntitiesDeliveryReport {
        // Actual callback/original-interval transport, NOT field/space/output proof.
        EntitiesDeliveryState state{EntitiesDeliveryState::NotCaptured};
        std::size_t sections{0},dispatches{0},events{0},valuesObserved{0},models{0},rawBodies{0},bytes{0},work{0};
        bool outputEligible{false};
    };
    static EntitiesDeliveryReport entitiesDeliveryReport(const dx_data&) noexcept;
    bool wantsDxfEntitiesDeliverySource() const override;
    void beginDxfEntitiesDelivery(std::uint64_t,std::size_t,const DRW_DxfEntitiesDeliveryEvent&) override;
    void endDxfEntitiesDelivery(std::uint64_t,std::size_t,const DRW_DxfEntitiesDeliveryEvent&) override;
    void addDxfEntitiesDeliverySource(const DRW_DxfEntitiesDeliverySource&) override;
    enum class EntitiesSourceState {NotCaptured,Reading,ImportFailed,CaptureFailed,ContextMismatch,TransportComplete};
    struct EntitiesSourceReport {
        // Retained opted-in scopes only, not every section or its semantics.
        EntitiesSourceState state{EntitiesSourceState::NotCaptured};
        std::size_t sections{0},chargedRecords{0},groups{0},bytes{0},work{0};
        bool outputEligible{false};
    };
    static EntitiesSourceReport entitiesSourceReport(const dx_data&) noexcept;
    bool wantsDxfEntitiesSource() const override;
    void addDxfEntitiesSource(const DRW_DxfEntitiesSource&) override;
    enum class TableSourceState {
        NotCaptured, Reading, ImportFailed, CaptureFailed, ContextMismatch,
        InvalidGraph, UnsupportedProfile, WorkLimit, StructurallyComplete,
        RawTypedMismatch
    };
    struct TableSourceReport {
        // Frozen selected owner-chain/dimensions only. Does not compare
        // editable consumer state, resolve auxiliary resources or prove output.
        TableSourceState state{TableSourceState::NotCaptured};
        std::uint64_t generation{0};
        std::size_t tables{0}, contents{0}, geometries{0}, compounds{0};
        std::size_t dictionaryCandidates{0}, tableWrappers{0};
        std::size_t records{0}, groups{0}, retainedBytes{0}, examinedSlots{0};
        std::size_t contentRawMatches{0}, nonnullUnknownContentReferences{0};
        std::size_t contentProofSlots{0};
        std::array<std::size_t, static_cast<std::size_t>(dxfTableContent::ReferenceKind::Count)>
            contentReferenceSlots{};
        // Structural source correlation never authorizes replay/support.
        bool outputEligible{false};
    };
    static TableSourceReport tableSourceReport(const dx_data& data) noexcept;
    enum class TableBindingState {
        SourceNotReady, CaptureFailed, WorkLimit, MissingTarget, WrongFamily,
        AmbiguousIdentity, InvalidNamespace, UnknownRole, Bound, WrongOwner,
        ResourceProofMissing, ResourceRawTypedMismatch, WrongSymbolKind,
        BlockSourceMissing, AmbiguousBlockDefinition, BlockSourceIncomplete,
        BlockMemberUnsupported, BlockMemberMismatch, BlockSpaceMismatch,
        BlockNameCollationUnqualified, BlockReferenceCycle,
        EntitiesSourceMissing,EntitiesSourceIncomplete,EntitiesSourceMismatch,
        EntitiesDeliveryMissing,EntitiesDeliveryIncomplete,EntitiesDeliveryMismatch
    };
    struct TableContentBindingReport {
        TableBindingState state{TableBindingState::SourceNotReady};
        std::size_t resourceRecords{0}, resourceItems{0}, retainedBytes{0}, examinedSlots{0};
        std::size_t boundReferences{0}, nullReferences{0}, localStyleUses{0}, absentStyleUses{0};
        std::uint32_t firstHandle{0};
        std::size_t resourceProofRecords{0}, resourceProofGroups{0}, resourceProofBytes{0}, resourceProofWork{0};
        std::size_t sourceMapMatches{0}, sourceStyleMatches{0}, unknownResourceReferences{0};
        std::size_t boundResourceReferences{0}, inheritedResourceReferences{0};
        std::size_t symbolProofUses{0};
        std::size_t lineTypeDependencyRows{0}, complexTextStyleBindings{0}, complexShapeStyleBindings{0};
        std::size_t fieldProofUses{0}, dataLinkProofUses{0};
        std::size_t dictionaryProofUses{0}, xRecordProofUses{0};
        std::size_t blockRecordProofUses{0};
        std::size_t blockSourceDefinitions{0},blockSourceMembers{0},blockSourceWorkspaceBytes{0};
        std::size_t blockNameReferences{0},blockNameBindings{0};
        bool blockNameGraphAcyclic{false}; // admitted name graph only, not all dependencies
        std::uint32_t firstSourceBlock{0};
        std::size_t firstSourceMember{(std::numeric_limits<std::size_t>::max)()};
        bool directRolesBound{false}; // not full block/dependency/live/output proof
        std::size_t entitiesPhysicalTablesMatched{0},entitiesDeliveredTablesMatched{0}; // origins, not complete body/space semantics
        // Rich-child direct roles/namespace, with exact selected map source
        // proof. Other resources' raw fields, full transitive/live and output
        // qualification remain separate gates. Geometry-block/ATTDEF-owner
        // selections additionally require their complete admitted definition
        // and acyclic transitive admitted INSERT name graph;
        // a TABLE's space-owner record is metadata, not an ENTITIES certificate.
        bool outputEligible{false};
    };
    static TableContentBindingReport tableContentBindingReport(const dx_data& data) noexcept;
    bool wantsDxfTableSource() const override;
    void addDxfTableSource(const DRW_Table& table,
                           const DRW_RawDxfObject& source) override;
    bool wantsDxfSymbolSource() const override;
    void addDxfSymbolSource(const DRW_TableEntry& row,const DRW_RawDxfObject& source) override;
    bool wantsDxfRootDictionarySource() const override;
    void addDxfRootDictionarySource(const DRW_Dictionary& dictionary,const DRW_RawDxfObject& source) override;
    bool wantsDxfBlockRecordSource() const override;
    void addDxfBlockRecordSource(const DRW_Block_Record& row,const DRW_RawDxfObject& source) override;
    bool wantsDxfAttdefSource() const override;
    void addDxfAttdefSource(const DRW_Attdef& value,const DRW_RawDxfObject& source,
                           const DRW_DxfAttdefSourceContext& context) override;
    // Converter resource policy, not a DXF nesting/size restriction. Separate
    // limits also provide a small deterministic boundary-test seam.
    struct ExtensionGraphLimits {
        std::size_t maxNodes{1000000u};
        std::size_t maxEdges{4000000u};
        std::size_t maxStagedHandles{1000000u};
        // Bound inventory indexing independently of reachable traversal.
        std::size_t maxSourceRecords{4u * 1024u * 1024u};
        // XRECORD payload validation scans values even when they are not edges.
        std::size_t maxXRecordPayloadSlots{4u * 1024u * 1024u};
    };
    enum class ExtensionGraphFailure {
        None, InvalidGraph, NodeLimit, EdgeLimit, StagedHandleLimit,
        SourceRecordLimit, XRecordPayloadLimit
    };
    struct ExtensionGraphStats {
        // Accepted work before success/failure; these are not live output sizes.
        std::size_t visitedNodes{0};
        // Reference slots, including owners and validated XRECORD pointers.
        std::size_t examinedEdges{0};
        // Prepared dictionary/XRECORD/VisualStyle entries, not payload bytes.
        std::size_t stagedHandles{0};
        std::size_t sourceRecords{0};
        // XRECORD value representations actually inspected; not retained bytes.
        std::size_t inspectedXRecordPayloadSlots{0};
    };

    dx_iface() = default;
    ~dx_iface() = default;
    // Explicit input-only selection; ordinary/CLI calls retain Compatible.
    // Owned outcome distinguishes this request from the read-entry snapshot.
    bool fileImport(const std::string& fileI, dx_data *fData, bool debug,
                    DRW::DxfCodePagePolicy codePagePolicy = DRW::DxfCodePagePolicy::Compatible);
    static constexpr std::size_t DwgReadReportMaxBytes = 32u * 1024u;
    static constexpr std::size_t StandardDwgObjectCaptureMaxRecords = 100000u;
    static constexpr std::size_t StandardDwgObjectCaptureMaxEntries = 1000000u;
    static constexpr std::size_t StandardDwgObjectCaptureMaxRetainedBytes =
        64u * 1024u * 1024u;
    static constexpr std::size_t OpaqueDwgSourceFrameCaptureMaxRecords =
        100000u;
    static constexpr std::size_t OpaqueDwgSourceFrameCaptureMaxRetainedBytes =
        64u * 1024u * 1024u;
    static constexpr std::size_t TypedEntityCaptureMaxRecords = 100000u;
    static constexpr std::size_t TypedEntityCaptureMaxRetainedBytes =
        64u * 1024u * 1024u;
    static constexpr std::size_t TypedEntityListNodeBytes =
        sizeof(DRW_Entity*) + 2u * sizeof(void*);
    static constexpr std::size_t ActionPathObjectCaptureMaxRecords = 100000u;
    static constexpr std::size_t ActionPathObjectCaptureMaxRetainedBytes =
        64u * 1024u * 1024u;
    static constexpr std::size_t ActionPathObjectListNodeBytes =
        2u * sizeof(void*);
    static constexpr std::size_t SunStudyCaptureMaxRecords = 100000u;
    static constexpr std::size_t SunStudyCaptureMaxRetainedBytes =
        64u * 1024u * 1024u;
    static constexpr std::size_t SunStudyListNodeBytes = 2u * sizeof(void*);
    static constexpr std::size_t LightListCaptureMaxRecords = 100000u;
    static constexpr std::size_t LightListCaptureMaxItems = 1000000u;
    static constexpr std::size_t LightListCaptureMaxRetainedBytes =
        64u * 1024u * 1024u;
    static constexpr std::size_t LightListNodeBytes = 2u * sizeof(void*);
    static constexpr std::size_t GeoMapImageCaptureMaxRecords = 100000u;
    static constexpr std::size_t GeoMapImageCaptureMaxRetainedBytes =
        64u * 1024u * 1024u;
    static constexpr std::size_t GeoMapImageListNodeBytes =
        2u * sizeof(void*);
    static constexpr std::size_t ImageDefinitionCaptureMaxRecords = 100000u;
    static constexpr std::size_t ImageDefinitionCaptureMaxRetainedBytes =
        64u * 1024u * 1024u;
    static constexpr std::size_t DwgDbColorCaptureMaxRecords = 100000u;
    static constexpr std::size_t DwgDbColorCaptureMaxRetainedBytes =
        64u * 1024u * 1024u;
    static constexpr std::size_t DwgDbColorReferenceMaxRecords =
        TypedEntityCaptureMaxRecords;
    static constexpr std::size_t DwgDbColorReferenceMaxRetainedBytes =
        TypedEntityCaptureMaxRetainedBytes;
    static constexpr std::size_t DwgDbColorPatchMaxRetainedBytes =
        8u * 1024u * 1024u;
    static constexpr std::size_t RawDxfSectionCaptureMaxRecords =
        DRW::kMaxDxfRawSections;
    static constexpr std::size_t RawDxfSectionCaptureMaxGroups =
        DRW::kMaxDxfRawSectionGroups;
    static constexpr std::size_t RawDxfSectionCaptureMaxRetainedBytes =
        DRW::kMaxDxfRawSectionsRetainedBytes;
    static constexpr std::size_t TypedBlockCaptureMaxRetainedBytes =
        64u * 1024u * 1024u;
    static constexpr std::size_t TypedBlockListNodeBytes =
        sizeof(dx_ifaceBlock*) + 2u * sizeof(void*);
    static constexpr std::size_t PlotSettingsCaptureMaxRecords = 100000u;
    static constexpr std::size_t PlotSettingsCaptureMaxRetainedBytes =
        64u * 1024u * 1024u;
    static constexpr std::size_t SectionCaptureMaxRecords = 100000u;
    static constexpr std::size_t SectionCaptureMaxRetainedBytes =
        64u * 1024u * 1024u;
    static constexpr std::size_t SectionRawCaptureMaxGroups = 4000000u;
    static constexpr std::size_t SectionRawCaptureMaxRetainedBytes =
        64u * 1024u * 1024u;
    static constexpr std::size_t GroupCaptureMaxRecords = 100000u;
    static constexpr std::size_t GroupCaptureMaxMembers = 1000000u;
    static constexpr std::size_t GroupCaptureMaxRetainedBytes =
        64u * 1024u * 1024u;
    static constexpr std::size_t DataStorageCaptureMaxSections = 100000u;
    static constexpr std::size_t DataStorageCaptureMaxRetainedBytes =
        64u * 1024u * 1024u;
    static void reportDwgReadOutcome(std::ostream& output,
                                     const dx_dwgReadOutcome& outcome);
    static dx_dxfSourceTextAdmission admitDxfSourceText(const dx_data& data) noexcept;
    dx_dxfSourceTextAdmission sourceTextAdmission() const noexcept {
        return m_sourceTextAdmission;
    }
    bool fileExport(const std::string& file, DRW::Version v, bool binary,
                    dx_data *fData, bool debug, bool requireResolvedOwners = false,
                    dx_dxfSourceTextPolicy sourceTextPolicy = dx_dxfSourceTextPolicy::Compatible,
                    DRW::DxfTextWritePolicy targetTextPolicy = DRW::DxfTextWritePolicy::Compatible,
                    dx_dxfProxyGraphicsPolicy proxyGraphicsPolicy =
                        dx_dxfProxyGraphicsPolicy::Compatible,
                    dx_dwgReadCompletenessPolicy readCompletenessPolicy =
                        dx_dwgReadCompletenessPolicy::Legacy);
    // Writes a selected AC1024 DWG associative root plus explicitly selected
    // nested network branches. The source graph remains untouched; excluded
    // root actions are omitted only from the emitted root-network projection.
    bool fileExportSelectedDwgAssociativeNetworks(
        const std::string& file, DRW::Version version, bool binary,
        dx_data* source, std::uint32_t rootNetworkHandle,
        const std::vector<std::uint32_t>& selectedChildNetworkHandles,
        bool debug = false);
    // A null raw carrier is accepted only for same-version, typed-only DWG
    // input; the helper constructs a bounded DXF shell and refuses common
    // object metadata that has no typed serialization path.
    bool writeDetailViewStyleRaw(DRW_RawDxfObject* raw,
                                 const DRW_DetailViewStyle* typed);
    bool writeSectionViewStyleRaw(DRW_RawDxfObject* raw,
                                  const DRW_SectionViewStyle* typed);
    bool writeVisualStyleRaw(DRW_RawDxfObject* raw,
                             const DRW_VisualStyle* typed);
    bool writeTableStyleRaw(DRW_RawDxfObject* raw,
                            const DRW_TableStyle* typed);
    bool writeCellStyleMapRaw(std::uint32_t handle,
                              const DRW_CellStyleMap* typed,
                              DRW::Version version);
    void writeEntity(DRW_Entity* e);
    std::uint32_t rawEntityOwnerToWrite(
        const DRW_RawDxfObject& entity,
        const dx_ifaceBlock* containingBlock) const;
    bool writeEntitySequence(dx_ifaceBlock* block);
    void setExtensionGraphLimits(const ExtensionGraphLimits& limits) noexcept {
        m_extensionGraphLimits = limits;
    }
    ExtensionGraphStats extensionGraphStats() const noexcept {
        return m_extensionGraphStats;
    }
    ExtensionGraphFailure extensionGraphFailure() const noexcept {
        return m_extensionGraphFailure;
    }
    const std::string& extensionGraphFailureDetail() const noexcept {
        return m_extensionGraphFailureDetail;
    }
    const char* extensionGraphFailureName() const noexcept {
        switch (m_extensionGraphFailure) {
        case ExtensionGraphFailure::None: return "none";
        case ExtensionGraphFailure::InvalidGraph: return "invalid-graph";
        case ExtensionGraphFailure::NodeLimit: return "node-limit";
        case ExtensionGraphFailure::EdgeLimit: return "edge-limit";
        case ExtensionGraphFailure::StagedHandleLimit: return "staged-handle-limit";
        case ExtensionGraphFailure::SourceRecordLimit: return "source-record-limit";
        case ExtensionGraphFailure::XRecordPayloadLimit: return "xrecord-payload-limit";
        }
        return "invalid-graph";
    }

    // Separate selected HATCH graph admission from the OBJECTS closure. These
    // budgets count preflight records/edges, not geometry evaluation. The
    // source-record limit also caps one per-call identity snapshot; reported
    // graph stats do not include those temporary slots or memory/RSS.
    void setHatchGraphLimits(const ExtensionGraphLimits& limits) noexcept {
        m_hatchGraphLimits = limits;
    }
    ExtensionGraphStats hatchGraphStats() const noexcept {
        return m_hatchGraphStats;
    }
    ExtensionGraphFailure hatchGraphFailure() const noexcept {
        return m_hatchGraphFailure;
    }

//reimplement virtual DRW_Interface functions

//reader part, stores all in class dx_data
    //header
    void addDwgAc1027DefaultLayerSource(const DRW_DwgAc1027LayerSource& source) noexcept override {
        if (!cData || typeid(*this) != typeid(dx_iface) || source.m_raw.m_version != DRW::AC1027
            || cData->sourceFormat != dx_source_format::DWG
            || m_classEvidenceReadData != cData || !m_classEvidenceReadAttempt
            || cData->m_classEvidenceOwner != this
            || cData->m_classEvidenceAttempt != m_classEvidenceReadAttempt
            || cData->importGeneration != cData->m_classEvidenceGeneration) return;
        if (cData->m_ac1027DefaultLayerDuplicate) return;
        if (cData->m_ac1027DefaultLayerSource || source.m_raw.m_rawBytes.size() > 1024u) {
            cData->m_ac1027DefaultLayerDuplicate = true; return;
        }
        try { cData->m_ac1027DefaultLayerSource = source;
              cData->m_ac1027DefaultLayerGeneration = cData->importGeneration; }
        catch (...) {
            cData->m_ac1027DefaultLayerSource.reset();
            cData->m_ac1027DefaultLayerDuplicate = true;
        }
    }
    void addDwgHeaderMaterialSource(DRW::Version version, const dwgHandle& source) noexcept override {
        if (!cData || typeid(*this) != typeid(dx_iface) || version != DRW::AC1027
            || cData->sourceFormat != dx_source_format::DWG
            || m_classEvidenceReadData != cData || !m_classEvidenceReadAttempt
            || cData->m_classEvidenceOwner != this
            || cData->m_classEvidenceAttempt != m_classEvidenceReadAttempt
            || cData->importGeneration != cData->m_classEvidenceGeneration) return;
        if (cData->m_ac1027HeaderMaterialSource) { cData->m_ac1027HeaderMaterialDuplicate = true; return; }
        cData->m_ac1027HeaderMaterialSource = source;
        cData->m_ac1027HeaderMaterialGeneration = cData->importGeneration;
    }
    void addDwgProxyGraphicsResourceOrder(
            const std::vector<std::string>& layerNames,
            const std::vector<std::string>& linetypeNames) noexcept override {
        if (!cData || cData->sourceFormat != dx_source_format::DWG
            || cData->importGeneration == 0u) return;
        static constexpr std::size_t maxResourceNames = 100000u;
        static constexpr std::size_t maxResourceNameBytes =
            16u * 1024u * 1024u;
        const auto bounded = [](const std::vector<std::string>& names) {
            std::size_t bytes = 0u;
            if (names.size() > maxResourceNames) return false;
            for (const std::string& name : names) {
                if (name.size() > maxResourceNameBytes - bytes)
                    return false;
                bytes += name.size();
            }
            return true;
        };
        if (!bounded(layerNames) || !bounded(linetypeNames)) {
            cData->dwgProxyResourceOrderCaptureFailed = true;
            cData->dwgProxyResourceOrderCaptured = false;
            cData->dwgProxyLayerNameOrder.clear();
            cData->dwgProxyLinetypeNameOrder.clear();
            return;
        }
        try {
            std::vector<std::string> layers(layerNames);
            std::vector<std::string> linetypes(linetypeNames);
            cData->dwgProxyLayerNameOrder.swap(layers);
            cData->dwgProxyLinetypeNameOrder.swap(linetypes);
            cData->dwgProxyResourceOrderCaptureFailed = false;
            cData->dwgProxyResourceOrderCaptured = true;
        } catch (...) {
            cData->dwgProxyResourceOrderCaptureFailed = true;
            cData->dwgProxyResourceOrderCaptured = false;
            cData->dwgProxyLayerNameOrder.clear();
            cData->dwgProxyLinetypeNameOrder.clear();
        }
    }
    void addHeader(const DRW_Header* data) override{
        cData->headerC = *data;
        if (cData->dxfReadOutcome.attempted)
            cData->dxfReadOutcome.headerDelivered = true;
    }
    void addDxfClass(const DRW_Class& data) override;
    bool wantsDxfClassSource() const noexcept override { return typeid(*this)==typeid(dx_iface) && tableSourceCaptureOpen(); }
    void addDxfClassSource(const DRW_Class&, bool) noexcept override;
    void addDwgClassCoverageReport(const DRW_DwgClassCoverageReport& data) override;
    bool wantsDwgClassSource() const override { return true; }
    void addDwgClassSourceReport(const DRW_DwgClassSourceReport& data) noexcept override;

    //tables
    virtual void addLType(const DRW_LType& data) override;
    virtual void addLayer(const DRW_Layer& data) override;
    virtual void addDimStyle(const DRW_Dimstyle& data) override;
    virtual void addVport(const DRW_Vport& data) override;
    void addView(const DRW_View& data) override;
    virtual void addUCS(const DRW_UCS& data) override;
    virtual void addTextStyle(const DRW_Textstyle& data) override;
    virtual void addAppId(const DRW_AppId& data) override;
    void addBlockRecord(const DRW_Block_Record& data) override;
    bool wantsDxfBlockSource() const override;
    void addDxfBlockBeginSource(const DRW_Block&,const DRW_RawDxfObject&) override;
    void addDxfBlockEndSource(const DRW_Block&,const DRW_RawDxfObject&) override;
    bool wantsDxfBlockChildrenSource() const override;
    void addDxfBlockChildrenSource(const DRW_DxfBlockChildrenSource&) override;
    bool wantsDxfBlockDeliverySource() const override;
    void addDxfBlockDeliverySource(const DRW_DxfBlockDeliverySource&) override;
    void beginDxfBlockDelivery(std::uint64_t,std::uint32_t,std::size_t,const DRW_DxfBlockDeliveryEvent&) override;
    void endDxfBlockDelivery(std::uint64_t,std::uint32_t,std::size_t,const DRW_DxfBlockDeliveryEvent&) override;
    void finishDxfBlockDelivery(std::uint64_t,std::uint32_t) override;
    void addLayout(const DRW_Layout& data) override;
    void addDictionary(const DRW_Dictionary& data) override;
    void addSortEntsTable(const DRW_SortEntsTable& data) override;
    void addDwgSortEntsMembership(
            const DRW_DwgSortEntsMembership& data) override;
    void addGroup(const DRW_Group& data) override;
    void addField(const DRW_Field& data) override;
    void addFieldList(const DRW_FieldList& data) override;
    void addDictionaryWithDefault(
            const DRW_DictionaryWithDefault& data) override;
    void addAcDbPlaceholder(const DRW_AcDbPlaceholder& data) override;
    void addMLineStyle(const DRW_MLineStyle& data) override;
    void addMLeaderStyle(const DRW_MLeaderStyle* data) override;
    void addTableStyle(const DRW_TableStyle& data) override;
    void addCellStyleMap(const DRW_CellStyleMap& data) override;
    void addDetailViewStyle(const DRW_DetailViewStyle& data) override;
    void addSectionViewStyle(const DRW_SectionViewStyle& data) override;
    void addScale(const DRW_Scale& data) override;
    void addDimensionAssociation(
        const DRW_DimensionAssociation& data) override;
    void addDictionaryVar(const DRW_DictionaryVar& data) override;
    void addNamedObjectDictionary(const DRW_Dictionary& data) override;
    void addXRecord(const DRW_XRecord& data) override;
    void addDataLink(const DRW_DataLink& data) override;
    void addPointCloudDef(const DRW_PointCloudDef& data) override;
    void addNavisworksModelDef(
        const DRW_NavisworksModelDef& data) override;
    void addPointCloudColorMap(
        const DRW_PointCloudColorMap& data) override;
    void addMaterial(const DRW_Material& data) override;
    void addDbColor(const DRW_DbColor& data) override;
    bool wantsDwgDbColorEedSource() const noexcept override { return true; }
    void addDwgDbColorEedSource(
        DRW::Version version, std::uint32_t objectHandle,
        const std::vector<DRW_TableEntry::PendingHandleRef>& appIds,
        const std::vector<DRW_TableEntry::PendingHandleRef>& layers) noexcept override;
    void addGeoData(const DRW_GeoData& data) override;
    void addRasterVariables(const DRW_RasterVariables& data) override;
    void addWipeoutVariables(const DRW_WipeoutVariables& data) override;
    void addVisualStyle(const DRW_VisualStyle& data) override;
    void addBackground(const DRW_Background& data) override;
    void addSun(const DRW_Sun& data) override;
    void addAssociativeObject(
            const DRW_AssociativeObject& data) override;
    void addProxyObject(const DRW_ProxyObject& data) override;
    virtual void addRawDxfObject(const DRW_RawDxfObject& data) override;
    void addUnmodeledDxfObject(const DRW_RawDxfObject& data) override;
    void addSpatialFilter(const DRW_SpatialFilter& data) override;
    void addRawDxfEntity(const DRW_RawDxfObject& data) override;
    void addEvaluationGraph(const DRW_EvaluationGraph& data) override;
    void addBlockRepresentationData(
            const DRW_BlockRepresentationData& data) override;
    void addDynamicBlockObject(
            const DRW_DynamicBlockObject& data) override;
    void addAcShHistoryObject(const DRW_AcShHistoryObject& data) override;
    void addDwgFramePublication(const DRW_DwgFramePublication& data) override;
    void addDwgDictionaryMembership(
            const DRW_DwgDictionaryMembership& data) override;
    void addDwgGroupMembership(const DRW_DwgGroupMembership& data) override;
    void addObjectContextData(
            const DRW_ObjectContextData& data) override;
    void addTableContent(const DRW_TableContentObject& data) override;
    void addTableGeometry(const DRW_TableGeometry& data) override;
    void addMotionPath(const DRW_MotionPath& data) override;
    void addCurvePath(const DRW_CurvePath& data) override;
    void addPointPath(const DRW_PointPath& data) override;
    void addObjectPtr(const DRW_ObjectPtr& data) override;
    void addSunStudy(const DRW_SunStudy& data) override;
    void addLightList(const DRW_LightList& data) override;
    void addGeoMapImage(const DRW_GeoMapImage& data) override;
    void addUnsupportedObject(const DRW_UnsupportedObject& data) override;
    void addImageDefinitionReactor(const DRW_ImageDefinitionReactor& data) override;
    void addProxyEntity(const DRW_ProxyEntity& data) override;

    //blocks
    virtual void addBlock(const DRW_Block& data) override{
        pairDxfBlockSource(data,false);
        if (cData == nullptr || cData->blocks.size()
                >= TypedEntityCaptureMaxRecords) {
            if (cData != nullptr)
                cData->typedEntityCaptureFailed = true;
            return;
        }
        std::size_t estimatedBytes = 0u;
        if (!estimateTypedBlockRetainedBytes(data, true, estimatedBytes)
            || cData->typedBlockRetainedBytes
                > TypedBlockCaptureMaxRetainedBytes
            || estimatedBytes > TypedBlockCaptureMaxRetainedBytes
                - cData->typedBlockRetainedBytes) {
            cData->typedEntityCaptureFailed = true;
            return;
        }
        dx_ifaceBlock* bk = nullptr;
        try {
            bk = new dx_ifaceBlock(data);
            cData->blocks.push_back(bk);
            cData->typedBlockRetainedBytes += estimatedBytes;
            currentBlock = bk;
        } catch (...) {
            delete bk;
            cData->typedEntityCaptureFailed = true;
        }
    }
    void addBlockEnd(const DRW_Block& data) override {
        pairDxfBlockSource(data,true);
        if (currentBlock == nullptr || cData == nullptr)
            return;
        std::size_t estimatedBytes = 0u;
        if (!estimateTypedBlockRetainedBytes(data, false, estimatedBytes)
            || cData->typedBlockRetainedBytes
                > TypedBlockCaptureMaxRetainedBytes
            || estimatedBytes > TypedBlockCaptureMaxRetainedBytes
                - cData->typedBlockRetainedBytes) {
            cData->typedEntityCaptureFailed = true;
            return;
        }
        try {
            currentBlock->endBlockData = data;
            currentBlock->hasEndBlockData = true;
            cData->typedBlockRetainedBytes += estimatedBytes;
        } catch (...) {
            cData->typedEntityCaptureFailed = true;
        }
    }
    virtual void endBlock() override{
        closeDxfBlockSource();
        currentBlock = cData->mBlock;
    }

    virtual void setBlock(const int /*handle*/) override{}//unused

    //entities
    virtual void addPoint(const DRW_Point& data) override{
        retainTypedEntity<DRW_Point>(data);
    }
    virtual void addLine(const DRW_Line& data) override{
        retainTypedEntity<DRW_Line>(data);
    }
    virtual void add3DLine(const DRW_3DLine& data) override{
        retainTypedEntity<DRW_3DLine>(data);
    }
    virtual void addRay(const DRW_Ray& data) override{
        retainTypedEntity<DRW_Ray>(data);
    }
    virtual void addXline(const DRW_Xline& data) override{
        retainTypedEntity<DRW_Xline>(data);
    }
    virtual void addArc(const DRW_Arc& data) override{
        retainTypedEntity<DRW_Arc>(data);
    }
    virtual void addCircle(const DRW_Circle& data) override{
        retainTypedEntity<DRW_Circle>(data);
    }
    virtual void addEllipse(const DRW_Ellipse& data) override{
        retainTypedEntity<DRW_Ellipse>(data);
    }
    virtual void addLWPolyline(const DRW_LWPolyline& data) override{
        retainTypedEntity<DRW_LWPolyline>(data);
    }
    virtual void addPolyline(const DRW_Polyline& data) override{
        retainTypedEntity<DRW_Polyline>(data);
    }
    virtual void addMesh(const DRW_Mesh& data) override{
        if (data.eType != DRW::MESH || typeid(data) != typeid(DRW_Mesh)) {
            if (cData != nullptr)
                cData->typedEntityCaptureFailed = true;
            return;
        }
        retainTypedEntity<DRW_Mesh>(data);
    }
    virtual void addSpline(const DRW_Spline* data) override{
        if (data == nullptr)
            return;
        if (data->eType != DRW::SPLINE || typeid(*data) != typeid(DRW_Spline)) {
            if (cData != nullptr)
                cData->typedEntityCaptureFailed = true;
            return;
        }
        retainTypedEntity<DRW_Spline>(*data);
    }
    virtual void addHelix(const DRW_Helix* data) override{
        if (data == nullptr)
            return;
        if (data->eType != DRW::HELIX || typeid(*data) != typeid(DRW_Helix)) {
            if (cData != nullptr)
                cData->typedEntityCaptureFailed = true;
            return;
        }
        retainTypedEntity<DRW_Helix>(*data);
    }
    // ¿para que se usa?
    virtual void addKnot(const DRW_Entity& data) override{(void)data;}

    virtual void addInsert(const DRW_Insert& data) override{
        const auto* table = dynamic_cast<const DRW_Table*>(&data);
        if (m_entitiesTableProjection && table != nullptr
            && table->m_semanticContentComplete
            && (data.numProxyGraph != 0 || !data.proxyGraphics.empty())) {
            // The typed TABLE record retains and serializes this exact cache.
            // Its INSERT-shaped compatibility projection must not advertise a
            // second independent proxy cache; DWG reading already delivered
            // the decoded display primitives through the event journal.
            DRW_Insert projection(data);
            projection.numProxyGraph = 0;
            projection.proxyGraphics.clear();
            projection.revokeProxyGraphicsReplayAuthorization();
            retainTypedEntity<DRW_Insert>(projection);
            return;
        }
        if (m_entitiesTableProjection && table != nullptr
            && !table->m_semanticContentComplete
            && (data.numProxyGraph != 0 || !data.proxyGraphics.empty())) {
            // An incomplete TABLE is delivered to legacy 2D consumers as an
            // INSERT-shaped projection. Its source-version metadata is
            // populated only after DWG callbacks finish, so use the parser's
            // provenance on this unchanged copy; the writer still enforces
            // same-revision output and exact framing.
            DRW_Insert projection(data);
            if (projection.proxyGraphicsProvenance().origin
                == DRW_ProxyGraphicsOrigin::ParsedDwg)
                (void)projection.authorizeParsedDwgProxyGraphicsReplay();
            retainTypedEntity<DRW_Insert>(projection);
            return;
        }
        retainTypedEntity<DRW_Insert>(data);
    }
    void addMLine(const DRW_MLine* data) override {
        if (data != nullptr)
            retainTypedEntity<DRW_MLine>(*data);
    }
    void addMLeader(const DRW_MLeader* data) override {
        if (data != nullptr)
            retainTypedEntity<DRW_MLeader>(*data);
    }
    void addTable(const DRW_Table& data) override {
        captureTableInput(data,nullptr,true);
        observeDxfBlockDeliveryValue(static_cast<std::int32_t>(data.eType),data.handle,data.parentHandle,
                                    DRW_DxfBlockCallback::addTable,nullptr,&data);
        observeDxfEntitiesTyped<DRW_Table>(data,DRW_DxfEntitiesCallback::addTable);
        constexpr std::size_t tableListNodeLinkBytes = 2u * sizeof(void*);
        if (cData == nullptr)
            return;
        if (cData->typedTableRetainedRecords
            >= TypedEntityCaptureMaxRecords) {
            cData->typedEntityCaptureFailed = true;
            return;
        }
        std::size_t estimatedBytes = 0u;
        if (!estimateTypedEntityRetainedBytes(
                static_cast<const DRW_Entity&>(data), sizeof(DRW_Table),
                estimatedBytes)
            || estimatedBytes > TypedEntityCaptureMaxRetainedBytes
                - tableListNodeLinkBytes
            || cData->typedEntityRetainedBytes
                > TypedEntityCaptureMaxRetainedBytes
            || estimatedBytes + tableListNodeLinkBytes
                > TypedEntityCaptureMaxRetainedBytes
                - cData->typedEntityRetainedBytes) {
            cData->typedEntityCaptureFailed = true;
            return;
        }
        estimatedBytes += tableListNodeLinkBytes;
        try {
            cData->tables.push_back(data);
            DRW_Table& retainedTable = cData->tables.back();
            if (retainedTable.proxyGraphicsProvenance().origin
                    == DRW_ProxyGraphicsOrigin::ParsedDwg)
                (void)retainedTable.authorizeParsedDwgProxyGraphicsReplay();
            ++cData->typedTableRetainedRecords;
            cData->typedEntityRetainedBytes += estimatedBytes;
            cData->typedTableRetainedBytes += estimatedBytes;
        } catch (...) {
            cData->typedEntityCaptureFailed = true;
            return;
        }
        // Keep the pre-existing 2D consumer path unchanged while retaining
        // cells, merged-range metadata and style links in the typed snapshot.
        struct ProjectionRestore final {
            bool& active;bool previous;
            ~ProjectionRestore(){active=previous;}
        } restore{m_entitiesTableProjection,m_entitiesTableProjection};
        m_entitiesTableProjection=true;
        addInsert(data);
    }
    virtual void addTrace(const DRW_Trace& data) override{
        retainTypedEntity<DRW_Trace>(data);
    }
    virtual void add3dFace(const DRW_3Dface& data) override{
        retainTypedEntity<DRW_3Dface>(data);
    }
    virtual void addSolid(const DRW_Solid& data) override{
        retainTypedEntity<DRW_Solid>(data);
    }
    virtual void addMText(const DRW_MText& data) override{
        if (const auto* columns = dynamic_cast<const DRW_MTextLegacyColumns*>(&data))
            retainTypedEntity<DRW_MTextLegacyColumns>(*columns);
        else
            retainTypedEntity<DRW_MText>(data);
    }
    void addTolerance(const DRW_Tolerance& data) override{
        retainTypedEntity<DRW_Tolerance>(data);
    }
    void addCamera(const DRW_Camera& data) override{
        retainTypedEntity<DRW_Camera>(data);
    }
    void addLight(const DRW_Light& data) override {
        if (data.eType != DRW::LIGHT
            || typeid(data) != typeid(DRW_Light)) {
            if (cData != nullptr)
                cData->typedEntityCaptureFailed = true;
            return;
        }
        if (!m_collectDwgLightImport || cData == nullptr
            || cData->sourceFormat != dx_source_format::DWG) {
            retainTypedEntity<DRW_Light>(data);
            return;
        }
        try {
            if (!m_dwgLightImport) {
                constexpr std::size_t originBytes =
                    sizeof(dx_dwgLightImport) + 2u * sizeof(void*);
                if (cData->typedEntityRetainedBytes > TypedEntityCaptureMaxRetainedBytes
                    || originBytes > TypedEntityCaptureMaxRetainedBytes
                        - cData->typedEntityRetainedBytes) {
                    cData->typedEntityCaptureFailed = true;
                    return;
                }
                m_dwgLightImport = std::make_shared<dx_dwgLightImport>();
                m_dwgLightImport->generation = cData->importGeneration;
                cData->typedEntityRetainedBytes += originBytes;
            }
            auto* retained = retainTypedEntity<dx_ifaceDwgLight>(data);
            if (retained != nullptr) {
                retained->source = m_dwgLightImport;
                retained->capturedSource = m_dwgLightImport;
                retained->sourceGeneration = cData->importGeneration;
            }
        } catch (...) {
            cData->typedEntityCaptureFailed = true;
        }
    }
    void addShape(const DRW_Shape& data) override {
        if (data.eType != DRW::SHAPE) {
            if (cData != nullptr)
                cData->typedEntityCaptureFailed = true;
            return;
        }
        retainTypedEntity<DRW_Shape>(data);
    }
    void addOle2Frame(const DRW_Ole2Frame& data) override {
        if (data.eType != DRW::OLE2FRAME
            || typeid(data) != typeid(DRW_Ole2Frame)) {
            if (cData != nullptr)
                cData->typedEntityCaptureFailed = true;
            return;
        }
        if (cData != nullptr && cData->dwgReadOutcome.attempted
            && cData->sourceFormat == dx_source_format::DWG) {
            constexpr std::size_t maxSourceRecords = 100000u;
            constexpr std::size_t maxSourceBytes = 64u * 1024u * 1024u;
            constexpr std::size_t sourceIndexBytes =
                sizeof(dx_data::StandardDwgFrameKey) + 4u * sizeof(void*);
            if (cData->m_dwgOle2FrameSourceGeneration
                    != cData->importGeneration) {
                cData->m_dwgOle2FrameSources.clear();
                cData->m_dwgOle2FrameSourceRetainedBytes = 0u;
                cData->m_dwgOle2FrameSourceCaptureFailed = false;
                cData->m_dwgOle2FrameSourceGeneration =
                    cData->importGeneration;
            }
            std::size_t estimatedBytes = 0u;
            if (cData->m_dwgOle2FrameSourceCaptureFailed
                || cData->importGeneration == 0u
                || data.handle == 0u || data.handle == DRW::NoHandle
                || data.m_rawBytes.size() != data.m_objectSize
                || data.m_payloadTooLarge || data.m_payloadTruncated
                || !estimateTypedEntityRetainedBytes(
                    data, sizeof(DRW_Ole2Frame), estimatedBytes)
                || cData->m_dwgOle2FrameSources.size() >= maxSourceRecords
                || cData->m_dwgOle2FrameSourceRetainedBytes > maxSourceBytes
                || estimatedBytes > maxSourceBytes - sourceIndexBytes
                || estimatedBytes + sourceIndexBytes > maxSourceBytes
                    - cData->m_dwgOle2FrameSourceRetainedBytes) {
                cData->m_dwgOle2FrameSourceCaptureFailed = true;
                cData->typedEntityCaptureFailed = true;
                return;
            }
            const dx_data::StandardDwgFrameKey key{
                cData->importGeneration, data.handle};
            try {
                if (!cData->m_dwgOle2FrameSources.emplace(key, data).second) {
                    cData->m_dwgOle2FrameSourceCaptureFailed = true;
                    cData->typedEntityCaptureFailed = true;
                    return;
                }
                cData->m_dwgOle2FrameSourceRetainedBytes +=
                    estimatedBytes + sourceIndexBytes;
            } catch (...) {
                cData->m_dwgOle2FrameSourceCaptureFailed = true;
                cData->typedEntityCaptureFailed = true;
                return;
            }
        }
        if (retainTypedEntity<DRW_Ole2Frame>(data) == nullptr
            && cData != nullptr && cData->dwgReadOutcome.attempted
            && cData->sourceFormat == dx_source_format::DWG)
            cData->m_dwgOle2FrameSourceCaptureFailed = true;
    }
    void addOleFrame(const DRW_OleFrame& data) override {
        if (data.eType != DRW::OLEFRAME) {
            if (cData != nullptr)
                cData->typedEntityCaptureFailed = true;
            return;
        }
        retainTypedEntity<DRW_OleFrame>(data);
    }
    void addGeoPositionMarker(
            const DRW_GeoPositionMarker& data) override {
        retainTypedEntity<DRW_GeoPositionMarker>(data);
    }
    void addSectionObject(const DRW_SectionObject& data) override {
        retainTypedEntity<DRW_SectionObject>(data);
    }
    virtual void addText(const DRW_Text& data) override{
        if (const auto* rtext = dynamic_cast<const DRW_RText*>(&data))
            retainTypedEntity<DRW_RText>(*rtext);
        else if (const auto* arcText =
                     dynamic_cast<const DRW_ArcAlignedText*>(&data))
            retainTypedEntity<DRW_ArcAlignedText>(*arcText);
        else
            retainTypedEntity<DRW_Text>(data);
    }
    virtual void addAttDef(const DRW_Attdef& data) override{
        retainTypedEntity<DRW_Attdef>(data);
    }
    virtual void addDimAlign(const DRW_DimAligned *data) override{
        if (data != nullptr)
            retainDimension<DRW_DimAligned>(
                static_cast<const DRW_Dimension&>(*data));
    }
    virtual void addDimLinear(const DRW_DimLinear *data) override{
        if (data != nullptr)
            retainDimension<DRW_DimLinear>(
                static_cast<const DRW_Dimension&>(*data));
    }
    virtual void addDimRadial(const DRW_DimRadial *data) override{
        if (data == nullptr)
            return;
        if (const auto* large = dynamic_cast<const DRW_DimLargeRadial*>(data))
            retainTypedEntity<DRW_DimLargeRadial>(*large);
        else
            retainDimension<DRW_DimRadial>(
                static_cast<const DRW_Dimension&>(*data));
    }
    virtual void addDimDiametric(const DRW_DimDiametric *data) override{
        if (data != nullptr)
            retainDimension<DRW_DimDiametric>(
                static_cast<const DRW_Dimension&>(*data));
    }
    virtual void addDimAngular(const DRW_DimAngular *data) override{
        if (data != nullptr)
            retainDimension<DRW_DimAngular>(
                static_cast<const DRW_Dimension&>(*data));
    }
    virtual void addDimAngular3P(const DRW_DimAngular3p *data) override{
        if (data != nullptr)
            retainDimension<DRW_DimAngular3p>(
                static_cast<const DRW_Dimension&>(*data));
    }
    virtual void addDimOrdinate(const DRW_DimOrdinate *data) override{
        if (data != nullptr)
            retainDimension<DRW_DimOrdinate>(
                static_cast<const DRW_Dimension&>(*data));
    }
    void addDimArc(const DRW_DimArc *data) override{
        if (data != nullptr)
            retainTypedEntity<DRW_DimArc>(*data);
    }
    virtual void addLeader(const DRW_Leader *data) override{
        if (data != nullptr)
            retainTypedEntity<DRW_Leader>(*data);
    }
    virtual void addHatch(const DRW_Hatch *data) override{
        if (data != nullptr)
            retainTypedEntity<DRW_Hatch>(*data);
    }
    virtual void addMPolygon(const DRW_MPolygon *data) override{
        if (data != nullptr)
            retainTypedEntity<DRW_MPolygon>(*data);
    }
    virtual void addViewport(const DRW_Viewport& data) override{
        retainTypedEntity<DRW_Viewport>(data);
    }
    virtual void addImage(const DRW_Image *data) override{
        if (data == nullptr || cData == nullptr)
            return;
        constexpr std::size_t imageIndexNodeBytes =
            sizeof(dx_ifaceImg*) + 2u * sizeof(void*);
        std::size_t entityBytes = 0u;
        if (!estimateTypedEntityRetainedBytes(
                *data, sizeof(dx_ifaceImg), entityBytes)
            || entityBytes > TypedEntityCaptureMaxRetainedBytes
                - TypedEntityListNodeBytes) {
            cData->typedEntityCaptureFailed = true;
            return;
        }
        entityBytes += TypedEntityListNodeBytes;
        if (cData->typedEntityRetainedBytes
                > TypedEntityCaptureMaxRetainedBytes
            || entityBytes > TypedEntityCaptureMaxRetainedBytes
                - cData->typedEntityRetainedBytes
            || imageIndexNodeBytes
                > TypedEntityCaptureMaxRetainedBytes
                    - cData->typedEntityRetainedBytes - entityBytes) {
            cData->typedEntityCaptureFailed = true;
            return;
        }
        dx_ifaceImg *img = retainTypedEntity<dx_ifaceImg>(*data);
        if (img == nullptr)
            return;
        img->sourceImportGeneration = cData->importGeneration;
        try {
            cData->images.push_back(img);
            cData->typedEntityRetainedBytes += imageIndexNodeBytes;
        } catch (...) {
            cData->typedEntityCaptureFailed = true;
        }
    }

    void addUnderlay(const DRW_Underlay *data) override {
        if (data == nullptr || cData == nullptr)
            return;
        constexpr std::size_t underlayIndexNodeBytes =
            sizeof(dx_ifaceUnderlay*) + 2u * sizeof(void*);
        std::size_t entityBytes = 0u;
        if (!estimateTypedEntityRetainedBytes(
                *data, sizeof(dx_ifaceUnderlay), entityBytes)
            || entityBytes > TypedEntityCaptureMaxRetainedBytes
                - TypedEntityListNodeBytes) {
            cData->typedEntityCaptureFailed = true;
            return;
        }
        entityBytes += TypedEntityListNodeBytes;
        if (cData->typedEntityRetainedBytes
                > TypedEntityCaptureMaxRetainedBytes
            || entityBytes > TypedEntityCaptureMaxRetainedBytes
                - cData->typedEntityRetainedBytes
            || underlayIndexNodeBytes
                > TypedEntityCaptureMaxRetainedBytes
                    - cData->typedEntityRetainedBytes - entityBytes) {
            cData->typedEntityCaptureFailed = true;
            return;
        }
        dx_ifaceUnderlay* underlay =
            retainTypedEntity<dx_ifaceUnderlay>(*data);
        if (underlay == nullptr)
            return;
        underlay->sourceImportGeneration = cData->importGeneration;
        underlay->sourceDwgPdfRotationRadians =
            cData->sourceFormat == dx_source_format::DWG
            && data->kind == DRW_Underlay::PDF;
        try {
            cData->underlays.push_back(underlay);
            cData->typedEntityRetainedBytes += underlayIndexNodeBytes;
        } catch (...) {
            cData->typedEntityCaptureFailed = true;
        }
    }

    void addWipeout(const DRW_Wipeout *data) override {
        // WIPEOUT owns a masking boundary, not a raster file/IMAGEDEF link.
        if (data == nullptr)
            return;
        retainTypedEntity<DRW_Wipeout>(*data);
    }

    void addPointCloud(const DRW_PointCloud *data) override {
        if (data == nullptr)
            return;
        if (data->eType != DRW::POINTCLOUD
            || typeid(*data) != typeid(DRW_PointCloud)) {
            if (cData != nullptr)
                cData->typedEntityCaptureFailed = true;
            return;
        }
        retainTypedEntity<DRW_PointCloud>(*data);
    }

    void addPointCloudEx(const DRW_PointCloudEx *data) override {
        if (data == nullptr)
            return;
        if (data->eType != DRW::POINTCLOUDEX
            || typeid(*data) != typeid(DRW_PointCloudEx)) {
            if (cData != nullptr)
                cData->typedEntityCaptureFailed = true;
            return;
        }
        retainTypedEntity<DRW_PointCloudEx>(*data);
    }

    void addNavisworksModel(const DRW_NavisworksModel *data) override {
        if (data == nullptr)
            return;
        if (data->eType != DRW::NAVISWORKSMODEL
            || typeid(*data) != typeid(DRW_NavisworksModel)) {
            if (cData != nullptr)
                cData->typedEntityCaptureFailed = true;
            return;
        }
        retainTypedEntity<DRW_NavisworksModel>(*data);
    }

    virtual void addSurface(const DRW_Surface *data) override{
        if (data == nullptr) {
            if (cData != nullptr)
                cData->typedEntityCaptureFailed = true;
            return;
        }
        const auto rejectMismatchedSurface = [this]() {
            if (cData != nullptr)
                cData->typedEntityCaptureFailed = true;
        };
        switch (data->eType) {
        case DRW::PLANESURFACE: {
            const auto* typed = dynamic_cast<const DRW_PlaneSurface*>(data);
            if (typed != nullptr && typeid(*data) == typeid(DRW_PlaneSurface))
                retainTypedEntity<DRW_PlaneSurface>(*typed);
            else
                rejectMismatchedSurface();
            break;
        }
        case DRW::EXTRUDEDSURFACE: {
            const auto* typed = dynamic_cast<const DRW_ExtrudedSurface*>(data);
            if (typed != nullptr
                && typeid(*data) == typeid(DRW_ExtrudedSurface))
                retainTypedEntity<DRW_ExtrudedSurface>(*typed);
            else
                rejectMismatchedSurface();
            break;
        }
        case DRW::REVOLVEDSURFACE: {
            const auto* typed = dynamic_cast<const DRW_RevolvedSurface*>(data);
            if (typed != nullptr
                && typeid(*data) == typeid(DRW_RevolvedSurface))
                retainTypedEntity<DRW_RevolvedSurface>(*typed);
            else
                rejectMismatchedSurface();
            break;
        }
        case DRW::SWEPTSURFACE: {
            const auto* typed = dynamic_cast<const DRW_SweptSurface*>(data);
            if (typed != nullptr
                && typeid(*data) == typeid(DRW_SweptSurface))
                retainTypedEntity<DRW_SweptSurface>(*typed);
            else
                rejectMismatchedSurface();
            break;
        }
        case DRW::LOFTEDSURFACE: {
            const auto* typed = dynamic_cast<const DRW_LoftedSurface*>(data);
            if (typed != nullptr
                && typeid(*data) == typeid(DRW_LoftedSurface))
                retainTypedEntity<DRW_LoftedSurface>(*typed);
            else
                rejectMismatchedSurface();
            break;
        }
        case DRW::NURBSURFACE: {
            const auto* typed = dynamic_cast<const DRW_NurbsSurface*>(data);
            if (typed != nullptr
                && typeid(*data) == typeid(DRW_NurbsSurface))
                retainTypedEntity<DRW_NurbsSurface>(*typed);
            else
                rejectMismatchedSurface();
            break;
        }
        default:
            rejectMismatchedSurface();
            break;
        }
    }

    void linkImage(const DRW_ImageDef *data) override;
    void linkUnderlay(const DRW_UnderlayDefinition *data) override;

    virtual void addModelerGeometry(const DRW_ModelerGeometry& data) override{
        if (typeid(data) != typeid(DRW_ModelerGeometry)
            || (data.eType != DRW::E3DSOLID
                && data.eType != DRW::REGION
                && data.eType != DRW::BODY)) {
            if (cData != nullptr)
                cData->typedEntityCaptureFailed = true;
            return;
        }
        retainTypedEntity<DRW_ModelerGeometry>(data);
    }
    virtual void addModelerGeometry(DRW_ModelerGeometry&& data) override{
        if (typeid(data) != typeid(DRW_ModelerGeometry)
            || (data.eType != DRW::E3DSOLID
                && data.eType != DRW::REGION
                && data.eType != DRW::BODY)) {
            if (cData != nullptr)
                cData->typedEntityCaptureFailed = true;
            return;
        }
        retainTypedEntity<DRW_ModelerGeometry>(std::move(data));
    }

    void addRawDxfSection(const DRW_RawDxfSection& data) override;

    void addDataStorage(const DRW_DataStorageSection& data) override;
    void addDataStorage(DRW_DataStorageSection&& data) override;

//writer part, send all in class dx_data to writer
    virtual void addComment(const char* /*comment*/) override{}
    void addPlotSettings(const DRW_PlotSettings *data) override;
    void addSection(const DRW_Section& data) override;
    virtual void addRenderSettings(const DRW_RenderSettings& data) override;

    virtual void writeHeader(DRW_Header& data) override{
        // These target maps and preallocations belong only to DXF export.
        // The same adapter callback is also used by dwgRW::write(), where
        // dxfW is intentionally null and DWG owns its separate handle map.
        if (dxfW != nullptr) {
            // write() has reset its derived maps and planned BLOCK_RECORDs,
            // but no TABLES/reactor callback has run yet. Plan forward HATCH
            // links now.
            const auto planViewportClipTargets = [this](
                    const dx_ifaceBlock* block) {
                if (block == nullptr)
                    return true;
                for (const DRW_Entity* entity : block->ent) {
                    if (entity == nullptr
                        || entity->eType != DRW::VIEWPORT)
                        continue;
                    const auto* viewport =
                        dynamic_cast<const DRW_Viewport*>(entity);
                    if (viewport == nullptr
                        || (viewport->clipBoundaryHandle != 0
                            && dxfW->getVersion() > DRW::AC1014
                            && dxfW->preallocateEntityHandle(
                                   viewport->clipBoundaryHandle) == 0))
                        return false;
                    for (const DRW_Viewport::LayerOverrideReference& ref :
                            viewport->layerOverrideReferences) {
                        if ((ref.groupCode != 335 && ref.groupCode != 343
                             && ref.groupCode != 344)
                            || ref.handle == 0u
                            || ref.handle == DRW::NoHandle
                            || ref.handle
                                > std::numeric_limits<std::uint32_t>::max()
                            || dxfW->preallocateEntityHandle(
                                   static_cast<std::uint32_t>(ref.handle))
                                == 0u)
                            return false;
                    }
                }
                return true;
            };
            bool viewportTargetsPlanned =
                planViewportClipTargets(cData->mBlock);
            for (const dx_ifaceBlock* block : cData->blocks)
                viewportTargetsPlanned = viewportTargetsPlanned
                    && planViewportClipTargets(block);
            if (!viewportTargetsPlanned) {
                (void)dxfW->writeTable(nullptr);
                return;
            }
            for (std::uint32_t source : m_hatchSourceHandles) {
                const std::uint32_t emitted =
                    dxfW->preallocateEntityHandle(source);
                const auto target = m_hatchTargets.find(source);
                if (emitted == 0 || target == m_hatchTargets.end()) {
                    m_hatchPlanningFailed = true;
                    break;
                }
                target->second.emitted = emitted;
            }
            // VIEW code 334 is emitted before BLOCK bodies. Reserve the
            // uniquely validated SECTIONOBJECT target identities now so the
            // forward references and the later entity records share one ID.
            for (std::uint32_t source : m_liveSectionHandlesToWrite) {
                if (dxfW->preallocateEntityHandle(source) == 0u) {
                    (void)dxfW->writeTable(nullptr);
                    return;
                }
            }
            // LEADER code 340 is a hard reference and may point forward in
            // the physical entity sequence. Reserve typed annotation targets
            // before any table/owner records consume output handles.
            if (!planLeaderAnnotationHandles()) {
                (void)dxfW->writeLeader(nullptr);
                return;
            }
        }
        //complete copy of header vars:
        data = cData->headerC;
        if (usesAc1027DefaultLayerProfile()) {
            std::uint32_t source = 0u;
            if (!validateAc1027HeaderMaterial(source)) {
                data.addStr("$CMATERIAL", "-1", 347);
                return;
            }
            data.addStr("$CMATERIAL", DRW::toHexStr(source), 347);
        }
        if (dxfW != nullptr) {
            static constexpr std::array<std::pair<const char*, int>, 3>
                visualStyleReferences{{
                    {"$INTERFEREOBJVS", 345},
                    {"$INTERFEREVPVS", 346},
                    {"$DRAGVS", 349}
                }};
            for (const auto& reference : visualStyleReferences) {
                std::uint32_t sourceHandle = 0;
                bool present = false;
                if (!data.getDxfVisualStyleReferenceHandle(
                        reference.first, sourceHandle, present)) {
                    (void)dxfW->writeTable(nullptr);
                    return;
                }
                if (!present || sourceHandle == 0)
                    continue;
                const std::uint32_t emittedHandle =
                    dxfW->remapHandle(sourceHandle);
                if (emittedHandle == 0) {
                    (void)dxfW->writeTable(nullptr);
                    return;
                }
                data.addStr(reference.first,
                            dxfW->toHexStrHandle(emittedHandle),
                            reference.second);
            }
            std::uint64_t plotStyleHandle = 0;
            bool hasPlotStyleHandle = false;
            if (!data.getDxfPlotStyleIdHandle(plotStyleHandle,
                                              hasPlotStyleHandle)) {
                (void)dxfW->writeTable(nullptr);
                return;
            }
            if (hasPlotStyleHandle && plotStyleHandle != 0) {
                if (plotStyleHandle
                        > (std::numeric_limits<std::uint32_t>::max)()
                    || std::find(m_extensionPlaceholderHandles.begin(),
                                 m_extensionPlaceholderHandles.end(),
                                 static_cast<std::uint32_t>(plotStyleHandle))
                        == m_extensionPlaceholderHandles.end()) {
                    (void)dxfW->writeTable(nullptr);
                    return;
                }
                const std::uint32_t emittedHandle = dxfW->remapHandle(
                    static_cast<std::uint32_t>(plotStyleHandle));
                if (emittedHandle == 0) {
                    (void)dxfW->writeTable(nullptr);
                    return;
                }
                data.addStr("$CEPSNID",
                            dxfW->toHexStrHandle(emittedHandle), 390);
            }
        }
        //or copy one by one:
//        for (auto it=cData->headerC.vars.begin(); it != cData->headerC.vars.end(); ++it)
//            data.vars[it->first] = new DRW_Variant( *(it->second) );
    }
    bool finalizeDxfWrite() override;

    virtual void writeBlocks() override{
        if(!hasUnchangedDxfFullStyles()||!hasUnchangedDxfCommonReferences(347,m_dxfMaterialReferenceState)){dxfW->writeRawDxfEntity(nullptr);return;}
        //write each block
        for (std::list<dx_ifaceBlock*>::iterator it=cData->blocks.begin(); it != cData->blocks.end(); ++it){
            dx_ifaceBlock* bk = *it;
            // dxfRW emits the canonical Model_Space and Paper_Space BLOCK
            // records itself.  The DWG reader also reports those fixed blocks
            // through addBlock(); replaying them here would collide with the
            // codec's reserved handles and abort the whole DXF transaction.
            if (isFixedSpaceBlock(bk))
                continue;
            if (!m_hatchTargets.empty())
                m_hatchOwnerContext = bk->name;
            recordHatchBlockPublication(bk->name, dxfW->writeBlock(bk));
            // Preserve typed/opaque callback order within the block body.
            if (!writeEntitySequence(bk))
                return;
            if (!dxfW->writeBlockEnd(
                    bk->hasEndBlockData ? &bk->endBlockData : nullptr))
                return;
        }
        m_hatchOwnerContext.clear();
    }
    // Preplan emitted block identities while retaining source table metadata.
    virtual void writeBlockRecords() override{
        if (!validateDxfBlockRecordInsertReferences(false)) {
            (void)dxfW->writeTable(nullptr);
            return;
        }
        std::unordered_map<std::uint32_t, const DRW_Block_Record*> records;
        for (const DRW_Block_Record& record : cData->blockRecords)
            records.emplace(record.handle, &record);
        for (std::list<dx_ifaceBlock*>::iterator it=cData->blocks.begin(); it != cData->blocks.end(); ++it) {
            if (isFixedSpaceBlock(*it))
                continue;
            const auto metadata = records.find((*it)->parentHandle);
            if (metadata != records.end())
                dxfW->writeBlockRecord((*it)->name, metadata->second->insUnits,
                                      metadata->second->previewData,
                                      metadata->second->insertHandles,
                                      metadata->second->extData);
            else
                dxfW->writeBlockRecord((*it)->name);
        }
        // ENDBLK handles are structural and reserved with their BLOCK_RECORD
        // handles. Bind retained source footer identities now, before earlier
        // XDATA/table records can serialize references to those handles.
        if (dxfW->getVersion() > DRW::AC1009) {
            for (const dx_ifaceBlock* block : cData->blocks) {
                if (block == nullptr || !block->hasEndBlockData
                    || block->endBlockData.handle == 0
                    || block->endBlockData.handle == DRW::NoHandle)
                    continue;
                const std::uint32_t emittedHandle =
                    dxfW->getBlockEndHandleToWrite(block->name);
                if (emittedHandle == DRW::NoHandle
                    || !dxfW->bindSourceEntityHandle(
                           block->endBlockData.handle, emittedHandle)) {
                    (void)dxfW->writeTable(nullptr);
                    return;
                }
            }
        }
        // BLOCK_RECORD XDATA is emitted before entity bodies, so plan typed
        // entity code-1005 targets while their source handles are intact.
        if (!preallocateBlockRecordXdataEntityHandles()) {
            (void)dxfW->writeTable(nullptr);
            return;
        }
        if (!preallocateDxfMLineLinetypeXdataEntityHandles()) {
            (void)dxfW->writeTable(nullptr);
            return;
        }
        if (dxfW->getVersion() > DRW::AC1009) {
            // TABLESTYLE objects are emitted later with OBJECTS, but their
            // source identities must be reserved before table records mint
            // output handles. A low source TABLESTYLE handle can otherwise
            // alias the next generated TEXTSTYLE handle.
            std::unordered_set<std::uint32_t> selectedTableStyleHandles;
            std::unordered_set<std::uint32_t> extensionCellStyleMapHandles(
                m_extensionCellStyleMapHandles.begin(),
                m_extensionCellStyleMapHandles.end());
            for (const std::uint32_t handle : m_tableStylesToEmit) {
                if (!selectedTableStyleHandles.insert(handle).second
                    || dxfW->preallocateEntityHandle(handle) == 0) {
                    (void)dxfW->writeTable(nullptr);
                    return;
                }
            }
            std::unordered_set<std::uint32_t> extensionTableStyleHandles;
            for (const std::uint32_t handle : m_extensionTableStyleHandles) {
                // A typed TABLESTYLE with an extension dictionary also
                // arrives through the generic extension-object inventory.
                // Reserve its identity once, through the selected typed row.
                if (selectedTableStyleHandles.count(handle) != 0u
                    || extensionCellStyleMapHandles.count(handle) != 0u)
                    continue;
                if (!extensionTableStyleHandles.insert(handle).second
                    || dxfW->preallocateEntityHandle(handle) == 0) {
                    (void)dxfW->writeTable(nullptr);
                    return;
                }
            }
            std::unordered_set<std::uint32_t> sourceStyleHandles;
            for (const DRW_Textstyle& style : cData->textStyles) {
                if (style.handle == 0)
                    continue;
                if ((cData->hasNamedObjectDictionary
                     && style.handle == cData->namedObjectDictionary.handle)
                    || !sourceStyleHandles.insert(style.handle).second
                    || dxfW->preallocateTableRecordHandle(style.handle) == 0) {
                    (void)dxfW->writeTable(nullptr);
                    return;
                }
            }
            for (const DRW_LType& lineType : cData->lineTypes) {
                for (const DRW_LTypeSegment& segment : lineType.segments) {
                    const std::uint32_t sourceStyle = segment.styleHandle.ref;
                    if (segment.shapeFlags == 0 || sourceStyle == 0)
                        continue;
                    if (sourceStyleHandles.count(sourceStyle) != 1u) {
                        (void)dxfW->writeTable(nullptr);
                        return;
                    }
                }
            }
        }
        std::unordered_set<std::uint32_t> preallocatedCellStyleMaps;
        for (const std::uint32_t handle : m_extensionCellStyleMapHandles) {
            if (!preallocatedCellStyleMaps.insert(handle).second
                || dxfW->preallocateEntityHandle(handle) == 0) {
                (void)dxfW->writeTable(nullptr);
                return;
            }
        }
        if (!planDwgProxyEntityObjectIds()) {
            std::cerr << "proxy entity object-ID graph has a missing, ambiguous, or unsupported target\n";
            (void)dxfW->writeTable(nullptr);
            return;
        }
    }
    //write entities of model space and first paper_space
    virtual void writeEntities() override{
        if(!hasUnchangedDxfFullStyles()||!hasUnchangedDxfCommonReferences(347,m_dxfMaterialReferenceState)){dxfW->writeRawDxfEntity(nullptr);return;}
        m_hatchOwnerContext.clear();
        if (!writeEntitySequence(cData->mBlock))
            return;
        // Fixed space BLOCK/ENDBLK records are regenerated, but their owned
        // source entities are not previews and must not disappear with them.
        for (dx_ifaceBlock* block : cData->blocks) {
            if (!isFixedSpaceBlock(block))
                continue;
            if (!writeEntitySequence(block))
                return;
        }
    }
    virtual void writeLTypes() override{
        if (dxfW != nullptr && cData != nullptr
            && !hasUnchangedVerifiedSourceObservations(*cData)) {
            (void)dxfW->writeLineType(nullptr);
            return;
        }
        for (std::list<DRW_LType>::iterator it=cData->lineTypes.begin(); it != cData->lineTypes.end(); ++it)
            dxfW->writeLineType(&(*it));
    }
    bool validateAc1027HeaderMaterial(std::uint32_t&) const noexcept;
    bool usesAc1027DefaultLayerProfile() const noexcept;
    bool validateAc1027DefaultLayer(const DRW_Layer&, std::uint32_t&) const noexcept;
    bool inspectAc1027DefaultLayerOutput(const dxfRW::DxfTablesWriteView&) noexcept;
    bool finalizeAc1027DefaultLayerOutput() const noexcept;
    virtual void writeLayers() override {
        if (!hasUnchangedDxfLayerMaterials()) { dxfW->writeLayer(nullptr); return; }
        const auto isLayerName = [](std::string_view value,
                                    std::string_view expected) {
            return value.size() == expected.size()
                && std::equal(value.begin(), value.end(), expected.begin(),
                    [](unsigned char left, unsigned char right) {
                        const auto upper = [](unsigned char c) {
                            return c >= 'a' && c <= 'z'
                                ? static_cast<unsigned char>(c - 'a' + 'A')
                                : c;
                        };
                        return upper(left) == upper(right);
                    });
        };
        const bool hasCanonicalConstraintLayer = std::any_of(
            cData->layers.begin(), cData->layers.end(), [&](const DRW_Layer& layer) {
                return isLayerName(layer.name, "*ADSK_CONSTRAINTS");
            });
        const auto writeLayerAndConstraintAlias =
            [this, hasCanonicalConstraintLayer, &isLayerName](DRW_Layer& layer) {
                if (!dxfW->writeLayer(&layer))
                    return false;
                if (!isLayerName(layer.name,
                                 "*LayerNameForDynamicConstraint")
                    || hasCanonicalConstraintLayer)
                    return true;
                const auto prepareAlias = [](DRW_Layer& alias) {
                    alias.name = "*ADSK_CONSTRAINTS";
                    alias.handle = 0u;
                    alias.parentHandle = 0u;
                    alias.reactorHandles.clear();
                    alias.xDictHandle = 0u;
                    alias.appData.clear();
                    for (DRW_Variant* value : alias.extData)
                        delete value;
                    alias.extData.clear();
                };
                if (auto* bound = dynamic_cast<DRW_Ac1027BoundLayer*>(&layer)) {
                    DRW_Ac1027BoundLayer alias(*bound, bound->sourcePlotStyle);
                    prepareAlias(alias);
                    if (!dxfW->writeLayer(&alias))
                        return false;
                    return true;
                }
                DRW_Layer alias(layer);
                prepareAlias(alias);
                if (!dxfW->writeLayer(&alias))
                    return false;
                return true;
            };
        for (auto& layer : cData->layers) {
            if (!checkDxfLayerMaterial(layer)) { dxfW->writeLayer(nullptr); return; }
            if (m_dxfLayerMaterialState.plotActive) {
                if (!hasUnchangedDxfLayerPlot()) { dxfW->writeLayer(nullptr); return; }
                DRW_Ac1027BoundLayer projection(layer, m_dxfLayerMaterialState.plotTarget);
                if (!writeLayerAndConstraintAlias(projection)) return;
                continue;
            }
            if (usesAc1027DefaultLayerProfile()) {
                std::uint32_t plotStyle = 0u;
                if (!validateAc1027DefaultLayer(layer, plotStyle)) {
                    dxfW->writeLayer(nullptr); return;
                }
                DRW_Ac1027BoundLayer projection(layer, plotStyle);
                if (!writeLayerAndConstraintAlias(projection)) return;
                continue;
            }
            if (cData->sourceFormat != dx_source_format::DXF
                && (!layer.handleMaterialS.empty() || layer.materialHandle.ref
                    || layer.materialHandle.ref64)) {
                // The legacy DWG/Unknown facade projection omits this
                // unqualified dependency. Keep the source model intact.
                auto projection = layer;
                projection.handleMaterialS.clear();
                projection.materialHandle = dwgHandle{};
                if (!writeLayerAndConstraintAlias(projection)) return;
            } else if (!writeLayerAndConstraintAlias(layer)) return;
        }
    }
    virtual void writeTextstyles() override{
        if (dxfW != nullptr && cData != nullptr
            && !hasUnchangedVerifiedSourceObservations(*cData)) {
            (void)dxfW->writeTextstyle(nullptr);
            return;
        }
        const bool selectedSourcePair = cData != nullptr
            && cData->m_verifiedStyleResourceSource != nullptr;
        if (selectedSourcePair
            && (!dxfW || !hasUnchangedVerifiedStyleResourceSource(
                    *cData, m_verifiedStyleControlSelected,
                    m_verifiedStyleRecordSelected))) {
            if (m_debugDwgOutputQualification)
                std::cerr << "STYLE source changed before TABLES staging\n";
            if (dxfW != nullptr)
                (void)dxfW->writeTextstyle(nullptr);
            return;
        }
        for (std::list<DRW_Textstyle>::iterator it=cData->textStyles.begin(); it != cData->textStyles.end(); ++it) {
            if (it->name.empty()) {
                if (mayOmitUnreferencedEmptyTextStyle(*it)) {
                    std::cerr << "DXF projection omitted unreferenced empty-name STYLE handle=0x"
                              << std::hex << it->handle << std::dec << '\n';
                    continue;
                }
                // Even shape styles cannot be emitted with an empty DXF
                // symbol name. Refuse whenever the selected projection does
                // not prove the omission safe.
                (void)dxfW->writeTextstyle(nullptr);
                return;
            }
            dxfW->writeTextstyle(&(*it));
        }
    }
    virtual void writeVports() override{
        for (std::list<DRW_Vport>::iterator it=cData->VPorts.begin(); it != cData->VPorts.end(); ++it)
            dxfW->writeVport(&(*it));
    }
    void writeViews() override {
        for (DRW_View& view : cData->views)
            dxfW->writeView(&view);
    }
    virtual void writeUCSs() override{
        for (std::list<DRW_UCS>::iterator it=cData->ucsList.begin(); it != cData->ucsList.end(); ++it)
            dxfW->writeUCS(&(*it));
    }
    virtual void writeDimstyles() override{
        for (std::list<DRW_Dimstyle>::iterator it=cData->dimStyles.begin(); it != cData->dimStyles.end(); ++it)
            dxfW->writeDimstyle(&(*it));
    }
    virtual void writeObjects() override {
        // dwgRW also invokes this interface callback.  This implementation
        // emits DXF object records only; during DWG writes dxfW is
        // intentionally null because the DWG writer owns its object stream.
        if (dxfW == nullptr)
            return;

        const char* objectWritePhase = "source snapshot validation";
        bool objectWriteCompleted = false;
        struct ObjectWriteDiagnostic {
            const char*& phase;
            bool& completed;
            dxfRW* writer;
            ~ObjectWriteDiagnostic() {
                if (!completed) {
                    // DRW_Interface::writeObjects() has no return value. A
                    // bare return from this callback must therefore latch a
                    // failure on dxfRW, or the enclosing OBJECTS transaction
                    // can commit a truncated section. A null raw object is
                    // the writer's established failure signal.
                    writer->writeRawDxfObject(nullptr);
                    std::cerr << "DXF OBJECTS callback aborted in "
                              << phase << '\n';
                }
            }
        } objectWriteDiagnostic{objectWritePhase, objectWriteCompleted, dxfW};

        if (!hasUnchangedAcdsDxfHistorySources()) {
            objectWritePhase = "ACDS history source snapshot";
            dxfW->writeRawDxfObject(nullptr);
            return;
        }
        if (!hasUnchangedDxfMaterialSources()) {
            objectWritePhase = "DXF MATERIAL source snapshot";
            dxfW->writeRawDxfObject(nullptr);
            return;
        }
        if (!hasUnchangedDxfAssociativeGeometrySources()
            || !hasUnchangedDxfAssociativeValueSources()) {
            objectWritePhase = "DXF associative source snapshots";
            dxfW->writeRawDxfObject(nullptr);
            return;
        }
        if (!hasUnchangedExtensionDictionaryOwnerSources()) {
            objectWritePhase = "extension dictionary owner snapshots";
            dxfW->writeRawDxfObject(nullptr);
            return;
        }
        if (!hasUnchangedDwgGeoDataGraphs()) {
            objectWritePhase = "DWG GEODATA source snapshots";
            dxfW->writeRawDxfObject(nullptr);
            return;
        }
        if (cData->sourceFormat == dx_source_format::DWG
            && !m_extensionDbColorHandles.empty()
            && !hasUnchangedDwgDbColorEedDependencies(*cData)) {
            objectWritePhase = "DWG DBCOLOR EED source snapshots";
            dxfW->writeRawDxfObject(nullptr);
            return;
        }
        if (cData->sourceFormat == dx_source_format::DXF
            && !cData->hasUnchangedDxfDbColorRawObjects()) {
            objectWritePhase = "DXF DBCOLOR raw source snapshots";
            dxfW->writeRawDxfObject(nullptr);
            return;
        }
        if (!hasUnchangedDynamicBlockOutputProfile()) {
            objectWritePhase = "dynamic-block source snapshots";
            dxfW->writeRawDxfObject(nullptr);
            return;
        }
        if (!hasUnchangedExtensionMaterialSources()) {
            objectWritePhase = "extension MATERIAL source snapshots";
            dxfW->writeRawDxfObject(nullptr);
            return;
        }
        if (!hasUnchangedDxfDictionaryVars() || !hasUnchangedDxfScales()) {
            objectWritePhase = "DXF DICTIONARYVAR/SCALE source snapshots";
            dxfW->writeRawDxfObject(nullptr);
            return;
        }
        if (!hasUnchangedDwgDictionaryVars() || !hasUnchangedDwgScales()) {
            objectWritePhase = "DWG DICTIONARYVAR/SCALE source snapshots";
            dxfW->writeRawDxfObject(nullptr);
            return;
        }
        if (!hasUnchangedDwgLayoutOutputSources()
            || !hasUnchangedDwgPlotSettingsSources()) {
            objectWritePhase = "DWG LAYOUT/PLOTSETTINGS source snapshots";
            dxfW->writeRawDxfObject(nullptr);
            return;
        }
        if (!hasUnchangedVerifiedSourceObservations(*cData)) {
            objectWritePhase = "verified source observation snapshots";
            dxfW->writeRawDxfObject(nullptr);
            return;
        }
        if (!hasUnchangedDxfMLineStyles()
            || !hasUnchangedDxfMLineConsumers()) {
            objectWritePhase = "DXF MLINE source snapshots";
            dxfW->writeRawDxfObject(nullptr);
            return;
        }
        objectWritePhase = "ACDS history records";
        for (auto& record : m_acdsDxfHistoryObjects) {
            if (!dxfW->writeRawDxfObject(&record)) return;
        }

        std::uint32_t layoutDictionaryHandle = 0;
        if (cData->hasNamedObjectDictionary) {
            for (const DRW_Dictionary::Entry& entry :
                 cData->namedObjectDictionary.m_entries) {
                std::string key = entry.m_name;
                for (char& character : key) {
                    if (character >= 'a' && character <= 'z')
                        character = static_cast<char>(character - 'a' + 'A');
                }
                if (key == "ACAD_LAYOUT") {
                    layoutDictionaryHandle = entry.m_handle;
                    break;
                }
            }
        }
        std::unordered_set<std::uint32_t> emittedDictionaryHandles;
        objectWritePhase = "extension dictionaries";
        for (std::uint32_t handle : m_extensionDictionaryHandles) {
            if (handle == layoutDictionaryHandle
                || handle == m_pointCloudClassicDictionaryHandle
                || !emittedDictionaryHandles.insert(handle).second)
                continue;
            // The graph validates this subtree, but the named-dictionary
            // codec plan is its sole emitter. Do not emit its root twice.
            if (std::any_of(m_dxfMaterialWriteDictionaries.begin(), m_dxfMaterialWriteDictionaries.end(),
                    [handle](const DRW_Dictionary& dictionary) { return dictionary.handle == handle; })) continue;
            for (DRW_Dictionary& dictionary : cData->dictionaries) {
                if (dictionary.handle != handle)
                    continue;
                const auto sortProjection =
                    m_sortDictionaryOutputs.find(handle);
                if (sortProjection != m_sortDictionaryOutputs.end()) {
                    if (!dxfW->writeDictionary(&sortProjection->second))
                        return;
                    break;
                }
                if (m_selectedDwgAssociativeSource == cData) {
                    const auto projection =
                        m_selectedDwgAssociativeDictionaryOutputs.find(handle);
                    if (projection
                            != m_selectedDwgAssociativeDictionaryOutputs.end()) {
                        if (!hasUnchangedSelectedDwgAssociativeNetworks()
                            || !dxfW->writeDictionary(&projection->second))
                            return;
                        break;
                    }
                }
                if (!dxfW->writeDictionary(&dictionary))
                    return;
                break;
            }
        }
        objectWritePhase = "classic POINTCLOUD root dictionary";
        if (m_pointCloudClassicDictionaryHandle != 0u
            && emittedDictionaryHandles.insert(
                   m_pointCloudClassicDictionaryHandle).second) {
            DRW_Dictionary* selected = nullptr;
            std::size_t matches = 0u;
            for (DRW_Dictionary& dictionary : cData->dictionaries) {
                if (dictionary.handle != m_pointCloudClassicDictionaryHandle)
                    continue;
                selected = &dictionary;
                ++matches;
            }
            if (matches != 1u || selected == nullptr
                || !dxfW->writeDictionary(selected))
                return;
        }
        objectWritePhase = "POINTCLOUDDEFINITION/POINTCLOUDDEFREACTOR";
        const auto writeClassicPointCloudObject = [this](
                std::uint32_t handle,
                DRW_PointCloudDef::Kind expectedKind) {
            DRW_PointCloudDef* selected = nullptr;
            std::size_t matches = 0u;
            for (DRW_PointCloudDef& object : cData->pointCloudDefinitions) {
                if (object.handle != handle)
                    continue;
                selected = &object;
                ++matches;
            }
            if (matches != 1u || selected == nullptr
                || selected->m_kind != expectedKind
                || !dxfW->writePointCloudDef(selected))
                return false;
            return true;
        };
        for (const std::uint32_t handle :
             m_pointCloudClassicDefinitionHandles) {
            if (!writeClassicPointCloudObject(
                    handle, DRW_PointCloudDef::Definition)) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        for (const std::uint32_t handle :
             m_pointCloudClassicReactorHandles) {
            if (!writeClassicPointCloudObject(
                    handle, DRW_PointCloudDef::Reactor)) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        objectWritePhase = "POINTCLOUDDEFINITIONEX/REACTOREX";
        const auto writePointCloudObject = [this](
                std::uint32_t handle,
                DRW_PointCloudDef::Kind expectedKind) {
            DRW_PointCloudDef* selected = nullptr;
            std::size_t matches = 0u;
            for (DRW_PointCloudDef& object : cData->pointCloudDefinitions) {
                if (object.handle != handle)
                    continue;
                selected = &object;
                ++matches;
            }
            if (matches != 1u || selected == nullptr
                || selected->m_kind != expectedKind
                || !dxfW->writePointCloudDef(selected))
                return false;
            return true;
        };
        for (const std::uint32_t handle :
             m_pointCloudExDefinitionHandles) {
            if (!writePointCloudObject(
                    handle, DRW_PointCloudDef::DefinitionEx)) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        for (const std::uint32_t handle : m_pointCloudExReactorHandles) {
            if (!writePointCloudObject(
                    handle, DRW_PointCloudDef::ReactorEx)) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        std::unordered_set<std::uint32_t> dictionaryWithDefaultHandles;
        std::unordered_set<std::uint32_t> placeholderHandles;
        objectWritePhase = "dictionary-with-default records";
        for (const std::uint32_t handle :
             m_extensionDictionaryWithDefaultHandles) {
            std::size_t rawMatches = 0;
            std::size_t typedMatches = 0;
            DRW_RawDxfObject* raw = nullptr;
            DRW_DictionaryWithDefault* typed = nullptr;
            for (DRW_RawDxfObject& object :
                 cData->rawDictionaryWithDefaults) {
                if (object.handle != handle)
                    continue;
                ++rawMatches;
                raw = &object;
            }
            for (DRW_DictionaryWithDefault& dictionary :
                 cData->dictionaryWithDefaults) {
                if (dictionary.handle != handle)
                    continue;
                ++typedMatches;
                typed = &dictionary;
            }
            if (!dictionaryWithDefaultHandles.insert(handle).second
                || typedMatches != 1u || typed == nullptr
                || !cData->hasSingleSuccessfulDictionaryDefaultImport()) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            if (cData->sourceFormat == dx_source_format::DWG) {
                if (rawMatches != 0u) return;
                if (usesAc1027DefaultLayerProfile()) {
                    DRW_Ac1027BoundDictionary projection(*typed);
                    if (!dxfW->writeDictionaryWithDefault(&projection)) return;
                } else if (!dxfW->writeDictionaryWithDefault(typed)) return;
            } else if (rawMatches != 1u || raw == nullptr
                       || !dxfW->writeRawDxfObject(raw)) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        for (const std::uint32_t handle : m_extensionPlaceholderHandles) {
            std::size_t rawMatches = 0;
            std::size_t typedMatches = 0;
            DRW_RawDxfObject* raw = nullptr;
            DRW_AcDbPlaceholder* typed = nullptr;
            for (DRW_RawDxfObject& object : cData->rawPlaceholders) {
                if (object.handle != handle)
                    continue;
                ++rawMatches;
                raw = &object;
            }
            for (DRW_AcDbPlaceholder& placeholder : cData->placeholders) {
                if (placeholder.handle != handle)
                    continue;
                ++typedMatches;
                typed = &placeholder;
            }
            if (!placeholderHandles.insert(handle).second
                || !cData->hasSingleSuccessfulDictionaryDefaultImport()) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            if (cData->sourceFormat == dx_source_format::DWG) {
                if (rawMatches != 0u || typedMatches != 1u || typed == nullptr
                    || !dxfW->writeAcDbPlaceholder(typed))
                    return;
            } else if (rawMatches != 1u || typedMatches != 0u || raw == nullptr
                       || !dxfW->writeRawDxfObject(raw)) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        objectWritePhase = "plot settings";
        for (std::uint32_t handle : m_extensionPlotSettingsHandles) {
            for (DRW_PlotSettings& settings : cData->plotSettings) {
                if (settings.handle != handle)
                    continue;
                if (!dxfW->writePlotSettings(&settings))
                    return;
                break;
            }
        }
        objectWritePhase = "GEODATA";
        for (const std::uint32_t handle : m_extensionGeoDataHandles) {
            const dx_data::GeoDataKey key{cData->importGeneration, handle};
            const auto geoData = cData->geoDataObjects.find(key);
            if (geoData == cData->geoDataObjects.end()) {
                std::cerr << "GEODATA output binding lost before emission\n";
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            if (!dxfW->writeGeoData(&geoData->second)) {
                std::cerr << "GEODATA writer refused source handle "
                          << handle << '\n';
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            if (!recordOpaqueDwgOutputQualification(handle)) {
                std::cerr << "GEODATA output frame qualification failed for handle "
                          << handle << '\n';
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        objectWritePhase = "RASTERVARIABLES";
        for (const std::uint32_t handle : m_extensionRasterVariablesHandles) {
            const dx_data::RasterVariablesKey key{
                cData->importGeneration, handle};
            DRW_RasterVariables* variables = nullptr;
            if (cData->sourceFormat == dx_source_format::DWG) {
                const auto source = cData->dwgRasterVariables.find(key);
                if (source != cData->dwgRasterVariables.end())
                    variables = &source->second;
            } else if (cData->sourceFormat == dx_source_format::DXF) {
                const auto source = cData->dxfRasterVariables.find(key);
                if (source != cData->dxfRasterVariables.end())
                    variables = &source->second;
            }
            if (variables == nullptr) {
                std::cerr << "RASTERVARIABLES output binding lost before emission\n";
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            if (!dxfW->writeRasterVariables(variables)) {
                std::cerr << "RASTERVARIABLES writer refused source handle "
                          << handle << '\n';
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            if (cData->sourceFormat == dx_source_format::DWG
                && !recordOpaqueDwgOutputQualification(handle)) {
                std::cerr << "RASTERVARIABLES output frame qualification failed for handle "
                          << handle << '\n';
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        objectWritePhase = "DBCOLOR";
        for (const std::uint32_t handle : m_extensionDbColorHandles) {
            const dx_data::DwgDbColorKey key{cData->importGeneration, handle};
            if (cData->sourceFormat == dx_source_format::DWG) {
                const auto color = cData->dwgDbColorObjects.find(key);
                if (color == cData->dwgDbColorObjects.end()
                    || !dxfW->writeDbColor(&color->second, "DBCOLOR")) {
                    dxfW->writeRawDxfObject(nullptr);
                    return;
                }
                if (!recordOpaqueDwgOutputQualification(handle)) {
                    dxfW->writeRawDxfObject(nullptr);
                    return;
                }
            } else if (cData->sourceFormat == dx_source_format::DXF) {
                const auto raw = cData->dxfDbColorRawObjects.find(key);
                if (raw == cData->dxfDbColorRawObjects.end()
                    || !cData->hasUnchangedDxfDbColorRawObjects()
                    || raw->second.handle != handle
                    || !dxfW->writeRawDxfObject(&raw->second)) {
                    dxfW->writeRawDxfObject(nullptr);
                    return;
                }
            } else {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        objectWritePhase = "WIPEOUTVARIABLES";
        for (const std::uint32_t handle : m_extensionWipeoutVariablesHandles) {
            const dx_data::WipeoutVariablesKey key{
                cData->importGeneration, handle};
            const auto variables = cData->dwgWipeoutVariables.find(key);
            if (variables == cData->dwgWipeoutVariables.end()) {
                std::cerr << "WIPEOUTVARIABLES output binding lost before emission\n";
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            if (!dxfW->writeWipeoutVariables(&variables->second)) {
                std::cerr << "WIPEOUTVARIABLES writer refused source handle "
                          << handle << '\n';
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            if (!recordOpaqueDwgOutputQualification(handle)) {
                std::cerr << "WIPEOUTVARIABLES output frame qualification failed for handle "
                          << handle << '\n';
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        objectWritePhase = "SUNSTUDY";
        for (const std::uint32_t handle : m_extensionSunStudyHandles) {
            if (!hasUnchangedSunStudyOutputGraph()) { dxfW->writeRawDxfObject(nullptr); return; }
            auto found = std::find_if(cData->sunStudies.begin(), cData->sunStudies.end(),
                [handle](const DRW_SunStudy& study) { return study.handle == handle; });
            if (found == cData->sunStudies.end() || !dxfW->writeSunStudy(&*found)) {
                dxfW->writeRawDxfObject(nullptr); return;
            }
        }
        objectWritePhase = "underlay definitions";
        std::unordered_set<std::uint32_t> underlayHandles;
        for (const dx_data::UnderlayDefinitionKey& key :
             m_underlayDefinitionKeysToWrite) {
            const auto definition = cData->underlayDefinitions.find(key);
            if (key.first != cData->importGeneration
                || definition == cData->underlayDefinitions.end()
                || definition->second.handle != key.second
                || !underlayHandles.insert(key.second).second
                || !dxfW->writeUnderlayDefinition(&definition->second)) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        objectWritePhase = "render settings";
        if (!hasUnchangedDxfRenderPresetSources()) {
            dxfW->writeRawDxfObject(nullptr);
            return;
        }
        std::unordered_set<std::uint32_t> renderSettingsHandles;
        for (std::uint32_t handle : m_extensionRenderSettingsHandles) {
            std::size_t matches = 0;
            for (DRW_RawDxfObject& object : cData->rawRenderSettingsObjects) {
                if (object.handle != handle)
                    continue;
                ++matches;
                if (!writeDxfRenderPreset(object))
                    return;
            }
            if (matches != 1u
                || !renderSettingsHandles.insert(handle).second) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        objectWritePhase = "background objects";
        std::unordered_set<std::uint32_t> backgroundHandles;
        for (std::uint32_t handle : dxfW->resolvedBackgroundHandles()) {
            std::size_t matches = 0;
            for (DRW_RawDxfObject& object : cData->rawBackgroundObjects) {
                if (object.handle != handle)
                    continue;
                ++matches;
                if (!dxfW->writeRawDxfObject(&object))
                    return;
            }
            if (matches != 1u || !backgroundHandles.insert(handle).second) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        objectWritePhase = "sun objects";
        std::unordered_set<std::uint32_t> sunHandles;
        if (cData->sourceFormat == dx_source_format::DWG) {
            for (std::uint32_t handle : dxfW->resolvedSunHandles()) {
                std::size_t matches = 0;
                for (DRW_Sun& sun : cData->suns) {
                    if (sun.handle != handle)
                        continue;
                    ++matches;
                    if (!dxfW->writeSun(&sun))
                        return;
                }
                if (matches != 1u || !sunHandles.insert(handle).second) {
                    dxfW->writeRawDxfObject(nullptr);
                    return;
                }
            }
        } else {
            for (std::uint32_t handle : dxfW->resolvedSunHandles()) {
                std::size_t matches = 0;
                for (DRW_RawDxfObject& object : cData->rawSunObjects) {
                    if (object.handle != handle)
                        continue;
                    ++matches;
                    if (!dxfW->writeRawDxfObject(&object, object.parentHandle))
                        return;
                }
                if (matches != 1u || !sunHandles.insert(handle).second) {
                    dxfW->writeRawDxfObject(nullptr);
                    return;
                }
            }
        }
        objectWritePhase = "sort dictionaries and layouts";
        std::unordered_set<std::uint32_t> sortDictionaries(
            m_sortDictionaryHandles.begin(), m_sortDictionaryHandles.end());
        for (DRW_Dictionary& dictionary : cData->dictionaries) {
            if (sortDictionaries.erase(dictionary.handle) == 0)
                continue;
            if (!emittedDictionaryHandles.insert(dictionary.handle).second)
                continue;
            const auto projection =
                m_sortDictionaryOutputs.find(dictionary.handle);
            DRW_Dictionary* output = projection
                    == m_sortDictionaryOutputs.end()
                ? &dictionary : &projection->second;
            if (!dxfW->writeDictionary(output))
                return;
        }
        if (!sortDictionaries.empty()) {
            dxfW->writeRawDxfObject(nullptr);
            return;
        }
        if (layoutDictionaryHandle != 0) {
            std::size_t matchingDictionaries = 0;
            for (DRW_Dictionary& dictionary : cData->dictionaries) {
                if (dictionary.handle != layoutDictionaryHandle)
                    continue;
                ++matchingDictionaries;
                if (!dxfW->writeDictionary(&dictionary))
                    return;
            }
            if (matchingDictionaries != 1u) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            for (DRW_Layout& layout : cData->layouts) {
                const auto frozen = m_dwgLayoutOutputSources.find(
                    layout.handle);
                if (frozen == m_dwgLayoutOutputSources.end()) {
                    if (!dxfW->writeLayout(&layout))
                        return;
                } else {
                    if (!hasUnchangedDwgLayoutOutputSources())
                        return;
                    DRW_Layout output = frozen->second.model;
                    if (!dxfW->writeLayout(&output))
                        return;
                }
            }
        }
        objectWritePhase = "XRECORDs";
        for (std::uint32_t handle : m_extensionXRecordHandles) {
            for (DRW_XRecord& record : cData->xRecords) {
                if (record.handle == handle) {
                    const auto& values = record.m_dataEntries.empty()
                        ? record.m_values : record.m_dataEntries;
                    const bool hasDwgCompatibilityCode = std::any_of(
                        values.cbegin(), values.cend(), [](const auto& value) {
                            return isDxfXRecordDwgCompatibilityCode(
                                value.code());
                        });
                    const bool hasLayerStateCompatibilityCode =
                        std::any_of(values.cbegin(), values.cend(),
                            [](const auto& value) {
                                return isDxfXRecordLayerStateCompatibilityCode(
                                    value.code());
                            });
                    const bool isSelectedLayerStateCompatibilityRecord =
                        hasLayerStateCompatibilityCode
                        && (isDwgLayerStateCompatibilityRecord(record)
                            || isDxfLayerStateCompatibilityReplaySource(record));
                    const bool useDwgCompatibility = hasDwgCompatibilityCode
                        && !hasLayerStateCompatibilityCode
                        && ((cData->sourceFormat == dx_source_format::DWG
                             && cData->sourceVersion >= DRW::AC1021
                             && cData->sourceVersion == dxfW->getVersion())
                            || isDxfXRecordCompatibilityReplaySource(record));
                    const bool written = isSelectedLayerStateCompatibilityRecord
                        ? dxfW->writeXRecordLayerStateCompatibility(&record)
                        : useDwgCompatibility
                        ? dxfW->writeXRecordDwgCompatibility(&record)
                        : dxfW->writeXRecord(&record);
                    if (!written) {
                        std::cerr << "XRECORD write failure: handle="
                                  << record.handle << " compatibility="
                                  << (useDwgCompatibility ? 1 : 0)
                                  << " source-version="
                                  << static_cast<int>(cData->sourceVersion)
                                  << " target-version="
                                  << static_cast<int>(dxfW->getVersion())
                                  << " dwg-compatibility="
                                  << (hasDwgCompatibilityCode ? 1 : 0)
                                  << " layerstate-370="
                                  << (hasLayerStateCompatibilityCode ? 1 : 0)
                                  << " raw-data-valid="
                                  << (record.m_rawDataValid ? 1 : 0)
                                  << " dwg-raw-source="
                                  << (record.hasDwgRawDataSourceRecord() ? 1 : 0)
                                  << '\n';
                        return;
                    }
                    break;
                }
            }
        }
        objectWritePhase = "spatial filters";
        std::unordered_set<std::uint32_t> spatialFilterHandles;
        for (const std::uint32_t handle : m_spatialFilterHandlesToWrite) {
            std::size_t matches = 0;
            for (DRW_SpatialFilter& filter : cData->spatialFilters) {
                if (filter.handle != handle)
                    continue;
                ++matches;
                if (!dxfW->writeSpatialFilter(&filter))
                    return;
            }
            if (matches != 1u
                || !spatialFilterHandles.insert(handle).second) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        std::unordered_set<std::uint32_t> dictionaryVarHandles;
        const auto isDictionaryVarName = [](std::string name) {
            for (char& character : name) {
                if (character >= 'a' && character <= 'z')
                    character = static_cast<char>(character - 'a' + 'A');
            }
            return name == "DICTIONARYVAR";
        };
        const auto isScaleName = [](std::string name) {
            for (char& character : name) {
                if (character >= 'a' && character <= 'z')
                    character = static_cast<char>(character - 'a' + 'A');
            }
            return name == "SCALE";
        };
        objectWritePhase = "DICTIONARYVAR records";
        for (std::uint32_t handle : m_extensionDictionaryVarHandles) {
            std::size_t rawMatches = 0;
            std::size_t typedMatches = 0;
            DRW_RawDxfObject* rawVar = nullptr;
            DRW_DictionaryVar* typedVar = nullptr;
            for (DRW_RawDxfObject& object : cData->rawNamedResourceObjects) {
                if (!isDictionaryVarName(object.name)
                    || object.handle != handle)
                    continue;
                ++rawMatches;
                rawVar = &object;
            }
            for (DRW_DictionaryVar& value : cData->dictionaryVars) {
                if (value.handle != handle)
                    continue;
                ++typedMatches;
                typedVar = &value;
            }
            if (!dictionaryVarHandles.insert(handle).second) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            if (cData->sourceFormat == dx_source_format::DWG) {
                const auto frozen =
                    m_dwgDictionaryVarOutputSources.find(handle);
                if (rawMatches != 0u || typedMatches != 1u
                    || typedVar == nullptr
                    || frozen == m_dwgDictionaryVarOutputSources.end()
                    || !hasUnchangedDwgDictionaryVars()
                    || !dxfW->writeDictionaryVar(&frozen->second.model))
                    return;
            } else if (m_dxfDictionaryVars.count(handle) != 0u) {
                if (!hasUnchangedDxfDictionaryVars()) return;
                auto& source=m_dxfDictionaryVars.at(handle);
                if (source.stringOverrides.empty()
                    ? !dxfW->writeRawDxfObject(&source.output)
                    : !dxfW->writeRawDxfObjectWithStringOverrides(&source.output,source.stringOverrides)) return;
            } else if (rawMatches != 1u || typedMatches != 1u
                       || rawVar == nullptr
                       || !dxfW->writeRawDxfObject(rawVar)) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        objectWritePhase = "SCALE records";
        std::unordered_set<std::uint32_t> scaleHandles;
        for (std::uint32_t handle : m_extensionScaleHandles) {
            if (cData->sourceFormat == dx_source_format::DXF) {
                const auto source = m_dxfScales.find(handle);
                if (source == m_dxfScales.end()) {
                    dxfW->writeRawDxfObject(nullptr);
                    return;
                }
                auto& plan = source->second;
                if (plan.stringOverrides.empty()
                    ? !dxfW->writeRawDxfObject(&plan.output)
                    : !dxfW->writeRawDxfObjectWithStringOverrides(&plan.output, plan.stringOverrides)) return;
                continue;
            }
            std::size_t rawMatches = 0;
            std::size_t typedMatches = 0;
            DRW_RawDxfObject* rawScale = nullptr;
            DRW_Scale* typedScale = nullptr;
            for (DRW_RawDxfObject& object : cData->rawNamedResourceObjects) {
                if (!isScaleName(object.name) || object.handle != handle)
                    continue;
                ++rawMatches;
                rawScale = &object;
            }
            for (DRW_Scale& value : cData->scales) {
                if (value.handle != handle)
                    continue;
                ++typedMatches;
                typedScale = &value;
            }
            if (!scaleHandles.insert(handle).second) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            if (cData->sourceFormat == dx_source_format::DWG) {
                const auto frozen = m_dwgScaleOutputSources.find(handle);
                if (rawMatches != 0u || typedMatches != 1u
                    || typedScale == nullptr
                    || frozen == m_dwgScaleOutputSources.end()
                    || !hasUnchangedDwgScales()
                    || !dxfW->writeScale(&frozen->second.model))
                    return;
            } else if (rawMatches != 1u || typedMatches != 1u
                       || rawScale == nullptr
                       || !dxfW->writeRawDxfObject(rawScale)) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        std::unordered_set<std::uint32_t> mlineStyleHandles;
        const auto isMLineStyleName = [](std::string name) {
            for (char& character : name) {
                if (character >= 'a' && character <= 'z')
                    character = static_cast<char>(character - 'a' + 'A');
            }
            return name == "MLINESTYLE";
        };
        objectWritePhase = "MLINESTYLE records";
        for (std::uint32_t handle : m_extensionMLineStyleHandles) {
            if (cData->sourceFormat == dx_source_format::DXF) {
                const auto source=m_dxfMLineStyles.find(handle);
                if (source==m_dxfMLineStyles.end()) { dxfW->writeRawDxfObject(nullptr); return; }
                auto& plan=source->second;
                if (plan.stringOverrides.empty()
                    ? !dxfW->writeRawDxfObject(&plan.output)
                    : !dxfW->writeRawDxfObjectWithStringOverrides(&plan.output,plan.stringOverrides)) return;
                continue;
            }
            std::size_t rawMatches = 0;
            std::size_t typedMatches = 0;
            DRW_RawDxfObject* rawStyle = nullptr;
            DRW_MLineStyle* typedStyle = nullptr;
            for (DRW_RawDxfObject& object : cData->rawNamedResourceObjects) {
                if (!isMLineStyleName(object.name) || object.handle != handle)
                    continue;
                ++rawMatches;
                rawStyle = &object;
            }
            for (DRW_MLineStyle& style : cData->mLineStyles) {
                if (style.handle != handle)
                    continue;
                ++typedMatches;
                typedStyle = &style;
            }
            if (!mlineStyleHandles.insert(handle).second) {
                DRW_DBG("DXF MLINESTYLE output has duplicate selected handle ");
                DRW_DBG(handle);
                DRW_DBG("\n");
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            if (cData->sourceFormat == dx_source_format::DWG) {
                if (rawMatches != 0u || typedMatches != 1u
                    || typedStyle == nullptr) {
                    DRW_DBG("DXF MLINESTYLE source binding mismatch handle/raw/typed=");
                    DRW_DBG(handle); DRW_DBG("/"); DRW_DBG(rawMatches);
                    DRW_DBG("/"); DRW_DBG(typedMatches); DRW_DBG("\n");
                    return;
                }
                if (!cData->hasSingleSuccessfulNamedResourceImport()) {
                    DRW_DBG("DXF MLINESTYLE has no unique successful named-resource import handle=");
                    DRW_DBG(handle); DRW_DBG("\n");
                    return;
                }
                const auto frozen = m_dwgMLineStyleOutputSources.find(handle);
                DRW_MLineStyle frozenStyle;
                DRW_MLineStyle* outputStyle = typedStyle;
                if (frozen != m_dwgMLineStyleOutputSources.end()) {
                    if (!hasUnchangedDwgMLineStyleOutputSources())
                        return;
                    frozenStyle = frozen->second.model;
                    outputStyle = &frozenStyle;
                }
                if (!dxfW->writeMLineStyle(outputStyle)) {
                    DRW_DBG("DXF MLINESTYLE writer rejected handle=");
                    DRW_DBG(handle); DRW_DBG(" name=");
                    DRW_DBG(typedStyle->name.c_str()); DRW_DBG("\n");
                    return;
                }
            } else if (rawMatches != 1u || typedMatches != 1u
                       || rawStyle == nullptr
                       || !cData->hasSingleSuccessfulNamedResourceImport()
                       || !dxfW->writeRawDxfObject(rawStyle)) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        std::unordered_set<std::uint32_t> mleaderStyleHandles;
        const auto isMLeaderStyleName = [](std::string name) {
            for (char& character : name) {
                if (character >= 'a' && character <= 'z')
                    character = static_cast<char>(character - 'a' + 'A');
            }
            return name == "MLEADERSTYLE";
        };
        objectWritePhase = "MLEADERSTYLE records";
        for (std::uint32_t handle : m_extensionMLeaderStyleHandles) {
            std::size_t rawMatches = 0;
            std::size_t typedMatches = 0;
            DRW_RawDxfObject* rawStyle = nullptr;
            DRW_MLeaderStyle* typedStyle = nullptr;
            for (DRW_RawDxfObject& object : cData->rawNamedResourceObjects) {
                if (!isMLeaderStyleName(object.name) || object.handle != handle)
                    continue;
                ++rawMatches;
                rawStyle = &object;
            }
            for (DRW_MLeaderStyle& style : cData->mLeaderStyles) {
                if (style.handle != handle)
                    continue;
                ++typedMatches;
                typedStyle = &style;
            }
            if (!mleaderStyleHandles.insert(handle).second) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            if (cData->sourceFormat == dx_source_format::DWG) {
                if (rawMatches != 0u || typedMatches != 1u
                    || typedStyle == nullptr
                    || !cData->hasSingleSuccessfulNamedResourceImport()
                    || !dxfW->writeMLeaderStyle(typedStyle))
                    return;
            } else {
                const auto prepared = m_dxfMLeaderStyles.find(handle);
                DRW_RawDxfObject* outputStyle = rawStyle;
                if (prepared != m_dxfMLeaderStyles.end())
                    outputStyle = &prepared->second.output;
                if (rawMatches != 1u || typedMatches != 1u
                    || rawStyle == nullptr
                    || !cData->hasSingleSuccessfulNamedResourceImport()
                    || !dxfW->writeRawDxfObject(outputStyle)) {
                    dxfW->writeRawDxfObject(nullptr);
                    return;
                }
            }
        }
        objectWritePhase = "VISUALSTYLE records";
        std::unordered_set<std::uint32_t> visualStyleHandles;
        for (std::uint32_t handle : m_extensionVisualStyleHandles) {
            if (cData->sourceFormat == dx_source_format::DWG) {
                std::size_t rawMatches = 0u;
                std::size_t typedMatches = 0u;
                const DRW_VisualStyle* typed = nullptr;
                for (DRW_RawDxfObject& object :
                     cData->rawVisualStyleObjects) {
                    rawMatches += object.handle == handle ? 1u : 0u;
                }
                for (const DRW_VisualStyle& style : cData->visualStyles) {
                    if (style.handle != handle)
                        continue;
                    ++typedMatches;
                    typed = &style;
                }
                if (rawMatches != 0u || typedMatches != 1u
                    || typed == nullptr
                    || !cData->hasCompleteDwgVisualStyleSourceProfile()
                    || dxfW->getVersion() != cData->sourceVersion
                    || !writeVisualStyleRaw(nullptr, typed))
                    return;
            } else {
                std::size_t matches = 0u;
                for (DRW_RawDxfObject& object :
                     cData->rawVisualStyleObjects) {
                    if (object.handle != handle)
                        continue;
                    ++matches;
                    if (!dxfW->writeRawDxfObject(&object)) {
                        const DRW_OperationDiagnostic diagnostic =
                            dxfW->getLastDiagnostic();
                        std::cerr << "DXF-origin VISUALSTYLE raw replay failed: handle="
                                  << handle << " code=" << diagnostic.code
                                  << " message=" << diagnostic.message << '\n';
                        return;
                    }
                }
                if (matches != 1u) {
                    std::cerr << "DXF-origin VISUALSTYLE raw replay identity mismatch: handle="
                              << handle << " matches=" << matches << '\n';
                    dxfW->writeRawDxfObject(nullptr);
                    return;
                }
            }
            if (!visualStyleHandles.insert(handle).second) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        objectWritePhase = "TABLESTYLE records";
        std::unordered_set<std::uint32_t> tableStyleHandles;
        for (std::uint32_t handle : m_extensionTableStyleHandles) {
            std::size_t rawMatches = 0;
            std::size_t typedMatches = 0;
            DRW_RawDxfObject* rawStyle = nullptr;
            DRW_TableStyle* typedStyle = nullptr;
            for (DRW_RawDxfObject& object : cData->rawTableStyles) {
                if (object.handle != handle)
                    continue;
                ++rawMatches;
                rawStyle = &object;
            }
            for (DRW_TableStyle& style : cData->tableStyles) {
                if (style.handle != handle)
                    continue;
                ++typedMatches;
                typedStyle = &style;
            }
            if (!tableStyleHandles.insert(handle).second) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            if (cData->sourceFormat == dx_source_format::DWG) {
                const auto frozen = m_dwgTableStyleOutputSources.find(handle);
                if (rawMatches != 0u || typedMatches != 1u
                    || typedStyle == nullptr
                    || (frozen != m_dwgTableStyleOutputSources.end()
                        ? !hasUnchangedDwgTableStyleOutputSources()
                            || !dxfW->writeTableStyle(
                                &frozen->second, cData->textStyles)
                        : !dxfW->writeTableStyle(
                            typedStyle, cData->textStyles)))
                    return;
            } else if (rawMatches != 1u || typedMatches != 1u
                       || rawStyle == nullptr
                       || !writeTableStyleRaw(rawStyle, typedStyle)) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        objectWritePhase = "CELLSTYLEMAP records";
        std::unordered_set<std::uint32_t> cellStyleMapHandles;
        for (std::uint32_t handle : m_extensionCellStyleMapHandles) {
            if (!cellStyleMapHandles.insert(handle).second) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            std::size_t matches = 0;
            DRW_CellStyleMap* cellStyleMap = nullptr;
            for (DRW_CellStyleMap& candidate : cData->cellStyleMaps) {
                if (candidate.handle != handle)
                    continue;
                ++matches;
                cellStyleMap = &candidate;
            }
            if (matches != 1u || cellStyleMap == nullptr)
                return;
            if (cData->sourceFormat == dx_source_format::DXF) {
                if (!writeCellStyleMapRaw(
                        handle, cellStyleMap, dxfW->getVersion())) {
                    dxfW->writeRawDxfObject(nullptr);
                    return;
                }
            } else if (cData->sourceFormat != dx_source_format::DWG
                       || !dxfW->writeCellStyleMap(cellStyleMap)) {
                return;
            }
        }
        objectWritePhase = "detail-view style records";
        std::unordered_set<std::uint32_t> detailViewStyleHandles;
        for (std::uint32_t handle : m_extensionDetailViewStyleHandles) {
            std::size_t rawMatches = 0;
            std::size_t typedMatches = 0;
            DRW_RawDxfObject* rawStyle = nullptr;
            DRW_DetailViewStyle* typedStyle = nullptr;
            for (DRW_RawDxfObject& object : cData->rawDetailViewStyles) {
                if (object.handle != handle)
                    continue;
                ++rawMatches;
                rawStyle = &object;
            }
            for (DRW_DetailViewStyle& style : cData->detailViewStyles) {
                if (style.handle == handle) {
                    ++typedMatches;
                    typedStyle = &style;
                }
            }
            const bool typedDwgStyle =
                cData->sourceFormat == dx_source_format::DWG;
            const bool dxfCarrierStyle =
                cData->sourceFormat == dx_source_format::DXF
                && rawMatches == 1u && rawStyle != nullptr;
            const bool styleImportValid =
                cData->hasSingleSuccessfulModelDocViewStyleImport();
            if ((!typedDwgStyle && !dxfCarrierStyle)
                || !detailViewStyleHandles.insert(handle).second
                || (typedDwgStyle && rawMatches != 0u)
                || typedMatches != 1u || typedStyle == nullptr
                || !styleImportValid
                || !writeDetailViewStyleRaw(
                    typedDwgStyle ? nullptr : rawStyle, typedStyle)) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        objectWritePhase = "section-view style records";
        std::unordered_set<std::uint32_t> sectionViewStyleHandles;
        for (std::uint32_t handle : m_extensionSectionViewStyleHandles) {
            std::size_t rawMatches = 0;
            std::size_t typedMatches = 0;
            DRW_RawDxfObject* rawStyle = nullptr;
            DRW_SectionViewStyle* typedStyle = nullptr;
            for (DRW_RawDxfObject& object : cData->rawSectionViewStyles) {
                if (object.handle != handle)
                    continue;
                ++rawMatches;
                rawStyle = &object;
            }
            for (DRW_SectionViewStyle& style : cData->sectionViewStyles) {
                if (style.handle == handle) {
                    ++typedMatches;
                    typedStyle = &style;
                }
            }
            const bool typedDwgStyle =
                cData->sourceFormat == dx_source_format::DWG;
            const bool dxfCarrierStyle =
                cData->sourceFormat == dx_source_format::DXF
                && rawMatches == 1u && rawStyle != nullptr;
            if ((!typedDwgStyle && !dxfCarrierStyle)
                || !sectionViewStyleHandles.insert(handle).second
                || (typedDwgStyle && rawMatches != 0u)
                || typedMatches != 1u || typedStyle == nullptr
                || !cData->hasSingleSuccessfulModelDocViewStyleImport()
                || !writeSectionViewStyleRaw(
                    typedDwgStyle ? nullptr : rawStyle, typedStyle)) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        objectWritePhase = "proxy records";
        std::unordered_set<std::uint32_t> rawProxyHandles;
        for (DRW_RawDxfObject& object : cData->rawProxyObjects) {
            if (object.handle == 0
                || !rawProxyHandles.insert(object.handle).second
                || !dxfW->writeRawDxfObject(&object)) {
                return;
            }
        }
        objectWritePhase = "ACDS records";
        if (!cData->dataStorageSections.empty()
            || !m_acdsTypedHistoryObjects.empty()) {
            if (!m_acdsTypedHistoryObjects.empty()) {
                for (DRW_RawDxfObject& object : m_acdsTypedHistoryObjects) {
                    if (object.handle == 0
                        || !rawProxyHandles.insert(object.handle).second
                        || !dxfW->writeRawDxfObject(&object)) {
                        return;
                    }
                }
                for (DRW_EvaluationGraph& graph :
                     m_acdsTypedEvaluationGraphs) {
                    if (graph.handle == 0
                        || !rawProxyHandles.insert(graph.handle).second
                        || !dxfW->writeEvaluationGraph(
                            &graph, "ACAD_EVALUATION_GRAPH")) {
                        return;
                    }
                }
            } else {
                for (DRW_RawDxfObject& object : m_acdsHistoryObjects) {
                    if (rawProxyHandles.count(object.handle) != 0
                        || !dxfW->writeRawDxfObject(&object)
                        || (cData->sourceFormat == dx_source_format::DWG
                            && cData->dwgReadOutcome.attempted
                            && !recordOpaqueDwgOutputQualification(
                                object.handle))) {
                        return;
                    }
                }
            }
            for (std::uint32_t handle : m_acdsMaterialHandles) {
                std::size_t matches = 0;
                for (DRW_Material& material : cData->materials) {
                    if (material.handle != handle)
                        continue;
                    ++matches;
                    if (!dxfW->writeMaterial(&material)) {
                        return;
                    }
                }
                if (matches != 1u) {
                    dxfW->writeRawDxfObject(nullptr);
                    return;
                }
            }
        }
        objectWritePhase = "MATERIAL records";
        for (const std::uint32_t handle : m_extensionMaterialHandles) {
            std::size_t matches = 0u;
            for (DRW_Material& material : cData->materials) {
                if (material.handle != handle)
                    continue;
                ++matches;
                if (!dxfW->writeMaterial(&material))
                    return;
            }
            if (matches != 1u) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        objectWritePhase = "DXF-origin MATERIAL records";
        for (const std::uint32_t handle : m_dxfMaterialHandles) {
            if(!writeDxfMaterialSource(handle)) {
                dxfW->writeRawDxfObject(nullptr);return;
            }
        }
        objectWritePhase = "evaluation graph records";
        // The bounded typed EvaluationGraph writer is independently usable
        // for ordinary DXF input. AC1027 DataStorage closures stay on their
        // stricter proxy-graph gate until ACSH field serialization is complete.
        if (cData->dataStorageSections.empty()
            && m_acdsDxfHistoryObjects.empty()
            && m_acdsTypedHistoryObjects.empty()) {
            for (DRW_EvaluationGraph& graph : cData->evaluationGraphs) {
                if (!dxfW->writeEvaluationGraph(
                        &graph, "ACAD_EVALUATION_GRAPH")) {
                    return;
                }
            }
        }
        objectWritePhase = "dynamic-block object records";
        if (m_dxfDynamicReplayPrepared) {
            std::unordered_set<std::uint32_t>
                typedBlockRepresentationHandles;
            try {
                typedBlockRepresentationHandles.reserve(
                    m_extensionBlockRepresentationDataHandles.size());
                typedBlockRepresentationHandles.insert(
                    m_extensionBlockRepresentationDataHandles.begin(),
                    m_extensionBlockRepresentationDataHandles.end());
            } catch (...) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            for (DxfDynamicBlockReplaySource& source :
                 m_dxfDynamicBlockReplaySources) {
                if (source.hasBlockRepresentationModel
                    && typedBlockRepresentationHandles.count(
                        source.raw.handle) != 0u)
                    continue;
                if (!dxfW->writeRawDxfObject(&source.raw))
                    return;
            }
        } else {
            for (DRW_DynamicBlockObject& object : cData->dynamicBlockObjects) {
                if (!dxfW->writeDynamicBlockObject(&object)) {
                    DRW_DBG("dynamic-block writer rejected record="
                            + object.m_recordName + " handle="
                            + std::to_string(object.handle) + " kind="
                            + std::to_string(static_cast<int>(object.m_kind)));
                    return;
                }
            }
        }
        objectWritePhase = "block representation data records";
        for (const std::uint32_t handle :
             m_extensionBlockRepresentationDataHandles) {
            std::size_t matches = 0u;
            for (DRW_BlockRepresentationData& object :
                 cData->blockRepresentationData) {
                if (object.handle != handle)
                    continue;
                ++matches;
                if (!dxfW->writeBlockRepresentationData(&object))
                    return;
            }
            if (matches != 1u) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        objectWritePhase = "sort-entity tables and data links";
        for (DRW_SortEntsTable& table : m_sortEmissionTables) {
            if (!dxfW->writeSortEntsTable(&table))
                return;
        }
        for (DRW_DataLink& dataLink : cData->dataLinks) {
            if (!dxfW->writeDataLink(&dataLink))
                return;
        }
        objectWritePhase = "FIELD and FIELDLIST records";
        std::unordered_set<std::uint32_t> tableFieldHandles;
        if (!cData->fields.empty()) {
            if (!hasTableContentFieldOutputProfile(
                    *cData, dxfW->getVersion())) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            try {
                tableFieldHandles.reserve(cData->fields.size());
                for (const DRW_Field& field : cData->fields) {
                    if (!tableFieldHandles.insert(field.handle).second) {
                        dxfW->writeRawDxfObject(nullptr);
                        return;
                    }
                }
            } catch (...) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            for (DRW_Field& field : cData->fields) {
                if (!dxfW->writeField(&field))
                    return;
            }
            for (DRW_FieldList& fieldList : cData->fieldLists) {
                if (!dxfW->writeFieldList(&fieldList))
                    return;
            }
        }
        objectWritePhase = "TABLECONTENT records";
        for (DRW_TableContentObject& content : cData->tableContents) {
            if (!tableContentDirectReferencesClosed(
                    content, tableFieldHandles)) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            if (!dxfW->writeTableContent(&content))
                return;
        }
        objectWritePhase = "TABLEGEOMETRY records";
        for (DRW_TableGeometry& geometry : cData->tableGeometries) {
            if (!dxfW->writeTableGeometry(&geometry))
                return;
        }
        objectWritePhase = "associative records";
        if (m_dxfAssociativeReplayPrepared) {
            for (DxfAssociativeReplaySource& source :
                 m_dxfAssociativeReplaySources) {
                if (!dxfW->writeRawDxfObject(&source.raw))
                    return;
            }
        }
        std::unordered_set<std::uint32_t> associativeHandles;
        if (!m_dxfAssociativeReplayPrepared) {
        for (const std::uint32_t handle : m_extensionAssociativeHandles) {
            if (!associativeHandles.insert(handle).second) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            std::size_t matches = 0u;
            DRW_AssociativeObject* object = nullptr;
            for (DRW_AssociativeObject& candidate :
                 cData->dwgAssociativeObjects) {
                if (candidate.handle != handle)
                    continue;
                ++matches;
                object = &candidate;
            }
            if (cData->sourceFormat != dx_source_format::DWG
                || matches != 1u || object == nullptr) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            bool written = false;
            if (object->m_recordName == "ACDBASSOCNETWORK") {
                DRW_AssociativeObject* networkOutput = object;
                if (m_selectedDwgAssociativeSource == cData
                    && handle == m_selectedDwgAssociativeRootHandle) {
                    if (!hasUnchangedSelectedDwgAssociativeNetworks()) {
                        dxfW->writeRawDxfObject(nullptr);
                        return;
                    }
                    networkOutput = &m_selectedDwgAssociativeRootOutput;
                }
                written = dxfW->writeAssociativeNetwork(networkOutput);
            }
            else if (object->m_recordName
                     == "ACDBASSOC2DCONSTRAINTGROUP")
                written = dxfW->writeAssociativeConstraintGroup(object);
            else if (object->m_recordName == "ACDBASSOCVARIABLE")
                written = dxfW->writeAssociativeVariable(object);
            else if (object->m_recordName
                         == "ACDBASSOCDIMDEPENDENCYBODY"
                     || object->m_recordName
                         == "ASSOCDIMDEPENDENCYBODY")
                written = dxfW->writeAssociativeDependencyBody(object);
            else if (object->m_recordName == "ACDBASSOCDEPENDENCY"
                     || object->m_recordName == "ACDBASSOCGEOMDEPENDENCY"
                     || object->m_recordName
                         == "ACDBASSOCVALUEDEPENDENCY")
                written = dxfW->writeAssociativeDependency(object);
            if (!written) {
                DRW_DBG("DXF OBJECTS associative writer failed: record=");
                DRW_DBG(object->m_recordName.c_str());
                DRW_DBG(" handle=");
                DRW_DBG(object->handle);
                DRW_DBG("\n");
                return;
            }
        }
        }
        objectWritePhase = "SECTIONMANAGER/SECTIONSETTINGS records";
        for (DRW_Section& section : cData->sectionRecords) {
            const char* recordName = section.m_kind == DRW_Section::Manager
                ? "SECTIONMANAGER" : "SECTIONSETTINGS";
            if (!dxfW->writeSection(&section, recordName))
                return;
        }
        objectWritePhase = "DWG DIMASSOC records";
        const auto currentDimensionAssociationBegin =
            cData->dwgDimensionAssociations.lower_bound(
                {cData->importGeneration, 0u});
        const bool hasCurrentDimensionAssociations =
            currentDimensionAssociationBegin
                != cData->dwgDimensionAssociations.end()
            && currentDimensionAssociationBegin->first.first
                == cData->importGeneration;
        if (cData->dimensionAssociationCaptureFailed) {
            dxfW->writeRawDxfObject(nullptr);
            return;
        }
        if (hasCurrentDimensionAssociations) {
            if (cData->sourceFormat != dx_source_format::DWG
                || !cData->dwgReadOutcome.attempted
                || cData->importGeneration == 0u
                || dxfW->getVersion() != cData->sourceVersion) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            try {
                std::unordered_map<std::uint32_t, DRW::ETYPE> entityTypes;
                std::size_t entityCount = 0u;
                const auto countEntities = [&entityCount](
                        const dx_ifaceBlock* block) {
                    if (block == nullptr)
                        return true;
                    if (block->ent.size()
                        > DRW::kMaxDxfTypedTableRecords - entityCount)
                        return false;
                    entityCount += block->ent.size();
                    return true;
                };
                if (!countEntities(cData->mBlock)) {
                    dxfW->writeRawDxfObject(nullptr);
                    return;
                }
                for (const dx_ifaceBlock* block : cData->blocks) {
                    if (!countEntities(block)) {
                        dxfW->writeRawDxfObject(nullptr);
                        return;
                    }
                }
                entityTypes.reserve(entityCount);
                const auto indexEntities = [&entityTypes](
                        const dx_ifaceBlock* block) {
                    if (block == nullptr)
                        return true;
                    for (const DRW_Entity* entity : block->ent) {
                        if (entity == nullptr || entity->handle == 0u
                            || entity->handle == DRW::NoHandle
                            || !entityTypes.emplace(
                                entity->handle, entity->eType).second)
                            return false;
                    }
                    return true;
                };
                if (!indexEntities(cData->mBlock)) {
                    dxfW->writeRawDxfObject(nullptr);
                    return;
                }
                for (const dx_ifaceBlock* block : cData->blocks) {
                    if (!indexEntities(block)) {
                        dxfW->writeRawDxfObject(nullptr);
                        return;
                    }
                }
                const auto isAssociativeAnnotationEntityType =
                    [](DRW::ETYPE type) {
                    switch (type) {
                    case DRW::DIMENSION:
                    case DRW::DIMALIGNED:
                    case DRW::DIMLINEAR:
                    case DRW::DIMRADIAL:
                    case DRW::DIMDIAMETRIC:
                    case DRW::DIMANGULAR:
                    case DRW::DIMANGULAR3P:
                    case DRW::DIMORDINATE:
                    case DRW::DIMARC:
                    // Autodesk describes AcDbDimAssoc as updating a
                    // dimension or leader; physical DWGs also bind it to
                    // AcDbMLeader targets.
                    case DRW::LEADER:
                    case DRW::MLEADER:
                        return true;
                    default:
                        return false;
                    }
                };
                for (auto item = currentDimensionAssociationBegin;
                     item != cData->dwgDimensionAssociations.end()
                         && item->first.first == cData->importGeneration;
                     ++item) {
                    const std::uint32_t sourceHandle = item->first.second;
                    const DRW_DimensionAssociation& source = item->second;
                    const dx_data::StandardDwgFrameKey frameKey{
                        cData->importGeneration, sourceHandle};
                    const auto rawFrame = cData->m_opaqueDwgSourceFrames.find(
                        frameKey);
                    const auto dimension = entityTypes.find(
                        source.m_dimensionHandle);
                    if (item->first.first != cData->importGeneration
                        || source.handle != sourceHandle
                        || cData->m_opaqueDwgSourceFrameIdentities.count(
                               frameKey) != 1u
                        || rawFrame == cData->m_opaqueDwgSourceFrames.end()
                        || dimension == entityTypes.end()
                        || !isAssociativeAnnotationEntityType(
                               dimension->second)) {
                        dxfW->writeRawDxfObject(nullptr);
                        return;
                    }
                    const auto hasEmittedDictionaryTarget = [this](
                            std::uint32_t handle) {
                        if (handle == 0u || handle == DRW::NoHandle)
                            return true;
                        const std::uint32_t emitted =
                            dxfW->remapHandle(handle);
                        return emitted != 0u && emitted != DRW::NoHandle
                            && dxfW->dxfWriteRecordHandleCount(emitted) == 1u
                            && dxfW->dxfWriteRecordHasIdentity(
                                emitted, "DICTIONARY", "OBJECTS");
                    };
                    if (!hasEmittedDictionaryTarget(
                            static_cast<std::uint32_t>(source.parentHandle))
                        || source.reactorCount() < 0
                        || static_cast<std::size_t>(source.reactorCount())
                            != source.reactorHandles.size()
                        || !hasEmittedDictionaryTarget(source.xDictHandle)) {
                        dxfW->writeRawDxfObject(nullptr);
                        return;
                    }
                    std::unordered_set<std::uint32_t> reactorHandles;
                    reactorHandles.reserve(source.reactorHandles.size());
                    for (const std::uint32_t reactor :
                         source.reactorHandles) {
                        if (!reactorHandles.insert(reactor).second
                            || reactor == 0u || reactor == DRW::NoHandle
                            || !hasEmittedDictionaryTarget(reactor)) {
                            dxfW->writeRawDxfObject(nullptr);
                            return;
                        }
                    }
                    const auto hasEntityTarget = [&entityTypes](
                            std::uint32_t handle) {
                        return handle != 0u && handle != DRW::NoHandle
                            && entityTypes.count(handle) == 1u;
                    };
                    for (const DRW_DimensionAssociationOsnapRef& ref :
                         source.m_osnapRefs) {
                        if (ref.m_objectHandles.empty()) {
                            if (!hasEntityTarget(ref.m_objectHandle)) {
                                dxfW->writeRawDxfObject(nullptr);
                                return;
                            }
                        } else {
                            for (const std::uint32_t handle :
                                 ref.m_objectHandles) {
                                if (!hasEntityTarget(handle)) {
                                    dxfW->writeRawDxfObject(nullptr);
                                    return;
                                }
                            }
                        }
                        if (ref.m_hasIntersectionObjectHandle
                            && !hasEntityTarget(
                                ref.m_intersectionObjectHandle)) {
                            dxfW->writeRawDxfObject(nullptr);
                            return;
                        }
                    }

                    DRW_DimensionAssociation output = source;
                    if (!dxfW->writeDimensionAssociation(&output)) {
                        std::cerr << "DIMASSOC writer refusal handle="
                                  << sourceHandle << " dimension="
                                  << source.m_dimensionHandle << " eType="
                                  << static_cast<int>(dimension->second)
                                  << " refs=" << source.m_osnapRefs.size()
                                  << '\n';
                        dxfW->writeRawDxfObject(nullptr);
                        return;
                    }
                    const std::uint32_t emittedHandle =
                        dxfW->remapHandle(sourceHandle);
                    if (emittedHandle == 0u
                        || emittedHandle == DRW::NoHandle
                        || dxfW->dxfWriteRecordHandleCount(emittedHandle) != 1u
                        || !dxfW->dxfWriteRecordHasIdentity(
                            emittedHandle, "DIMASSOC", "OBJECTS")
                        || !recordOpaqueDwgOutputQualification(sourceHandle)) {
                        std::cerr << "DIMASSOC output identity refusal source="
                                  << sourceHandle << " emitted="
                                  << emittedHandle << " handleCount="
                                  << dxfW->dxfWriteRecordHandleCount(
                                         emittedHandle)
                                  << " identity="
                                  << dxfW->dxfWriteRecordHasIdentity(
                                         emittedHandle, "DIMASSOC", "OBJECTS")
                                  << '\n';
                        dxfW->writeRawDxfObject(nullptr);
                        return;
                    }
                }
            } catch (...) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        objectWriteCompleted = true;
    }
    virtual void writeAppId() override{
        for (std::list<DRW_AppId>::iterator it=cData->appIds.begin(); it != cData->appIds.end(); ++it)
            dxfW->writeAppId(&(*it));
    }

    dxfRW* dxfW {nullptr}; //pointer to writer, needed to send data
    dx_data* cData {nullptr}; // class to store or read data
    dx_ifaceBlock* currentBlock {nullptr};

private:
    bool wantsVerifiedXRecord(std::uint32_t handle) const noexcept override;
    void verifiedXRecord(const DwgVerifiedXRecordTicket&) noexcept override;
    std::uint32_t verifiedTableEntitySelection() const noexcept override;
    std::uint32_t verifiedStyleExtensionDictionarySelection() const noexcept override;
    bool wantsVerifiedTableEntityModel() const noexcept override;
    bool wantsVerifiedTableCandidate(std::uint32_t handle) const noexcept override;
    void verifiedTableCandidate(const DwgVerifiedTableCandidateTicket&) noexcept override;
    void finishVerifiedXRecordSource(bool successfulRead) noexcept;
    static bool verifiedXRecordMemberUnchanged(const dx_data&,
        const dx_data::VerifiedXRecordSource&) noexcept;
    static bool verifiedXRecordSelectionMatches(const dx_data::VerifiedXRecordSources&,
        const std::vector<std::uint32_t>& handles) noexcept;
    static bool validateVerifiedTableCandidates(const dx_data&,
        const dx_data::VerifiedXRecordSources&,const dx_data::VerifiedTableCandidateSources&,
        std::uint32_t tableContent,std::uint32_t dataTable,std::uint32_t tableGeometry,
        DwgTableContentAc1018Candidate::Result& content,
        DwgDataTableAc1018Candidate::Result& table,
        DwgTableGeometryAc1018Candidate::Result* geometry) noexcept;
    std::vector<std::uint32_t> m_verifiedXRecordSelected;
    dx_data* m_verifiedXRecordReadData{nullptr};
    std::shared_ptr<const unsigned char> m_verifiedXRecordReadToken;
    std::uint64_t m_verifiedXRecordReadGeneration{0u};
    bool m_verifiedXRecordFailed{false};
    std::vector<dx_data::VerifiedXRecordSource> m_verifiedXRecordPending;
    std::size_t m_verifiedXRecordPendingBytes{0u};
    std::uint32_t m_verifiedTableContentSelected{0u},m_verifiedDataTableSelected{0u},m_verifiedTableGeometrySelected{0u};
    bool m_verifiedTableRoundtripSelected{false};
    bool m_verifiedTableRoundtripChildrenSelected{false};
    std::uint32_t m_verifiedTableEntitySelected{0u},m_verifiedTableDictionarySelected{0u};
    std::uint32_t m_verifiedTableCacheBlockRecordSelected{0u};
    std::uint32_t m_verifiedTableCacheBlockSelected{0u};
    bool m_verifiedReciprocalTableSelected{false};
    bool m_verifiedTableModelSelected{false};
    bool m_verifiedTableProjectionSelected{false};
    std::uint32_t m_verifiedTableStyleSelected{0u};
    std::uint32_t m_verifiedStyleDictionarySelected{0u},m_verifiedStyleXRecordSelected{0u},m_verifiedCellStyleMapSelected{0u};
    std::uint32_t m_verifiedStyleControlSelected{0u},m_verifiedStyleRecordSelected{0u};
    std::uint32_t m_verifiedLTypeControlSelected{0u};
    std::uint32_t m_verifiedLTypeOrdinarySelected{0u};
    std::uint32_t m_verifiedLTypeByBlockSelected{0u};
    std::uint32_t m_verifiedLTypeByLayerSelected{0u};
    bool m_verifiedStyleXRecordRequired{false};
    bool m_verifiedCellStyleMapRequired{false};
    std::vector<dx_data::VerifiedTableCandidateSource> m_verifiedTableCandidatePending;
    std::size_t m_verifiedTableCandidatePendingBytes{0u};
    std::vector<dx_data::VerifiedTableCandidateSource>
        m_verifiedStyleResourcePending;
    std::size_t m_verifiedStyleResourcePendingBytes{0u};
    std::vector<dx_data::VerifiedTableCandidateSource>
        m_verifiedLTypeResourcePending;
    std::size_t m_verifiedLTypeResourcePendingBytes{0u};
    friend class DxWholeSourceTestAccess;
    friend class DxIfaceStyleResourceTestAccess;
    friend class DxIfaceMlineDependencyTestAccess;
    friend class DxIfaceDwgLtypeControlOutputTestAccess;
    friend class DxIfaceStandardDwgCaptureTestAccess;
    bool mayOmitUnreferencedEmptyTextStyle(
        const DRW_Textstyle& style) const noexcept;
    bool hasUnchangedVerifiedSourceObservations(
        const dx_data& data) const noexcept;
    bool mlineRootDictionarySourceLiveProof(const dx_data&,std::size_t,std::size_t*) const noexcept;
    bool mlineStyleDictionarySourceLiveProof(const dx_data&,std::size_t,std::size_t*) const noexcept;
    bool mlineStyleBodiesSourceLiveProof(const dx_data&,std::size_t,std::size_t*) const noexcept;
    bool mlineLayerSourceLiveProof(const dx_data&,std::size_t,std::size_t*) const noexcept;
    bool mlineLtypeSourceLiveProof(const dx_data&,std::size_t,std::size_t*) const noexcept;
    bool hasTableContentFieldOutputProfile(
        const dx_data&, DRW::Version outputVersion) const noexcept;
    bool validateDxfBlockRecordInsertReferences(
        bool outputReady) const noexcept;
    bool tableContentDirectReferencesClosed(
        const DRW_TableContentObject&,
        const std::unordered_set<std::uint32_t>&) const noexcept;
    dx_mline_source_dependency::Proof mlineDependencySourceProof(const dx_data&,
        dx_mline_source_dependency::Limits={}) const noexcept;
    // Source relation only; no target uniqueness, graph or output authority.
    static bool legacyBlockRefSourceRelation(const dx_data&,std::uint32_t,std::uint32_t,
        std::size_t,std::size_t,blockref_source_relation::Result&) noexcept;
    // Selected source-name uniqueness only; no BLOCK or output authority.
    static bool legacyBlockRefUniqueSourceName(const dx_data&,std::uint32_t,std::uint32_t,
        std::size_t,std::size_t) noexcept;
    // Selected ATTDEF XDICTIONARY source-to-live-model binding only; no
    // recursive graph or emitted-output authority.
    static bool legacyAttdefExtensionDictionaryLiveSource(const dx_data&,
        std::uint32_t) noexcept;
    // Selected original BLOCK/ENDBLK identities only; no geometry or output authority.
    static bool legacyBlockTargetSource(const dx_data&,std::uint32_t,std::uint32_t,
        std::size_t,std::size_t,std::array<std::uint32_t,2>&) noexcept;
    // Selected original ATTRIB-to-ATTDEF tag source, no output authority.
    static bool legacyCompoundAttdefSource(const dx_data&,std::uint32_t,std::uint32_t,
        std::size_t,std::size_t,std::uint32_t&) noexcept;
    // Selected original APPID provider name, not semantic/output authority.
    static bool legacyCompoundAppIdSource(const dx_data&,std::uint32_t,std::uint32_t,
        std::size_t,std::size_t,std::uint32_t&) noexcept;
    // Selected original BLOCK_RECORD EED APPID dependency, source-only.
    enum class LegacyBlockRecordAppIdResult : std::uint8_t {NotSelected,Matched,Rejected,Failed};
    static LegacyBlockRecordAppIdResult legacyBlockRecordAppIdSource(const dx_data&,std::uint32_t,
        std::size_t,std::size_t,
        std::shared_ptr<const dx_data::TableDxfSourceCapture::LegacyBlockRecordAppIdJoin>&) noexcept;
    void finishLegacyBlockRecordAppIdJoins(dx_data&) noexcept;
    // Selected original INSERT/ATTRIB/SEQEND layer providers only.
    static bool legacyCompoundLayerSources(const dx_data&,std::uint32_t,std::uint32_t,
        std::size_t,std::size_t,std::array<std::uint32_t,3>&) noexcept;
    bool m_wholeEnabled{false};WholeSourceV2::Limits m_wholeLimits{};
    void captureTableInput(const DRW_Table&,const DRW_RawDxfObject*,bool typed) noexcept;
    using TableBody=dx_data::TableDxfSourceCapture::TableBody;
    void finishTableBodies(dx_data&) noexcept;
    void finishTableReferences(dx_data&) noexcept;
    void finishDependencies(dx_data&) noexcept;
    void observeViewUcsSource(const DRW_TableEntry*,const DRW_RawDxfObject*) noexcept;
    void observeRenderingInput(const DRW_TableEntry*,const DRW_RawDxfObject*) noexcept;
    void finishRenderingInputs(dx_data&) noexcept;
    void replayRenderingInput(dx_data&,dx_data::TableDxfSourceCapture::RenderingInputRow&) noexcept;
    void finishRenderingSources(dx_data&) noexcept;
    void finishViewUcsSources(dx_data&) noexcept;
    void observeResourceOrigin(const DRW_TableEntry*,const DRW_RawDxfObject*) noexcept;
    void finishResourceOrigins(dx_data&) noexcept;
    void observePlotStyleSource(const DRW_TableEntry*,const DRW_RawDxfObject*) noexcept;
    void finishPlotStyleSources(dx_data&) noexcept;
    bool replayTableBody(dx_data&,TableBody&,bool entitiesReady);
    static bool sameTableBodyModel(const DRW_Table&,const DRW_Table&,DRW::Version) noexcept;
    static bool sameDwgTableModel(const DRW_Table&,const DRW_Table&) noexcept;
    static bool hasUnchangedDwgTableProjection(const dx_data&,const DRW_Table&) noexcept;
    static bool hasMatchedTableSource(const dx_data&,const std::shared_ptr<const TableBody>&) noexcept;
    static bool chargeTableBody(dx_data&,std::size_t,std::size_t,std::size_t=0u) noexcept;
    dx_data* m_tableSourceReadData{nullptr};
    bool m_legacyIdentityObservationEnabled{false};
    bool m_entitiesTableProjection{false};
    bool chargeEntitiesDelivery(std::size_t,std::size_t) noexcept;
    bool startEntitiesDeliveryScope(std::uint64_t) noexcept;
    bool captureEntitiesReadContext() noexcept;
    using EntitiesObservedValue=dx_data::TableDxfSourceCapture::EntitiesObservedValue;
    EntitiesObservedValue* observeDxfEntitiesValue(std::int32_t,std::uint32_t,std::uint32_t,DRW_DxfEntitiesCallback) noexcept;
    void observeDxfEntitiesRaw(const DRW_RawDxfObject&,DRW_DxfEntitiesCallback) noexcept;
    static bool concreteEntitiesBodyChildren(const DRW_Entity&) noexcept;
    void captureEntitiesCompoundStates(EntitiesObservedValue&,const DRW_Entity&) noexcept;
    using EntitiesCompoundAudit=dx_data::TableDxfSourceCapture::EntitiesCompoundAudit;
    using EntitiesCompoundProof=dx_data::TableDxfSourceCapture::EntitiesCompoundProof;
    void finishEntitiesCompoundAudits(dx_data&) noexcept;
    bool chargeEntitiesCompound(std::size_t,std::size_t,std::size_t=0u) noexcept;
    bool replayEntitiesCompound(EntitiesCompoundAudit&,EntitiesCompoundProof&);
    bool chargeLayoutSource(std::size_t,std::size_t,std::size_t) noexcept;
    void pairLayoutSource(const DRW_Layout&) noexcept;
    void finishLayoutSourceAudits(dx_data&) noexcept;
    template<class Model,class Value>void observeDxfEntitiesTyped(const Value& value,
                            DRW_DxfEntitiesCallback route=DRW_DxfEntitiesCallback::Count) noexcept {
        auto* observed=observeDxfEntitiesValue(static_cast<std::int32_t>(value.eType),value.handle,value.parentHandle,route);
        if(!observed)return;
        // Do not slice a richer unknown subtype or freeze a converter-only
        // IMAGE wrapper as if it were the actual reader's model.
        const auto* typed=dynamic_cast<const Model*>(&value);
        if(!typed||typeid(value)!=typeid(Model)){observed->modelUnavailable=true;return;}
        std::size_t bytes=0u;
        if(!estimateTypedEntityRetainedBytes(*typed,sizeof(Model),bytes)
            ||bytes>((std::numeric_limits<std::size_t>::max)()-64u-8u*sizeof(void*))/2u
            ||!chargeEntitiesDelivery(bytes*2u+8u*sizeof(void*),bytes*2u+64u)) {
            cData->m_tableDxfSource.entitiesDeliveryCaptureFailed=true;return;
        }
        if(!concreteEntitiesBodyChildren(value)){observed->modelUnavailable=true;return;}
        captureEntitiesCompoundStates(*observed,value);
        if(cData->m_tableDxfSource.entitiesDeliveryCaptureFailed||observed->modelUnavailable)return;
        try {observed->model=std::make_shared<const Model>(*typed);++cData->m_tableDxfSource.entitiesDeliveryModels;}
        catch(...){cData->m_tableDxfSource.entitiesDeliveryCaptureFailed=true;}
    }
    void observeDxfBlockDeliveryValue(std::int32_t,std::uint32_t,std::uint32_t,
                                    DRW_DxfBlockCallback,const DRW_RawDxfObject* = nullptr,const DRW_Entity* = nullptr) noexcept;
    using AttdefOriginResource=dx_data::TableDxfSourceCapture::Resource;
    using AttdefOriginBlock=dx_data::TableDxfSourceCapture::BlockSource;
    void captureDxfBlockReadContext(AttdefOriginBlock&) noexcept;
    void observeDxfBlockReadContext(AttdefOriginBlock&) noexcept;
    void observeDxfBlockOrigin(AttdefOriginBlock&,dxfRW::DxfBlockSourceView::Phase,const DRW_Entity* model=nullptr,
        const DRW_RawDxfObject* raw=nullptr,const DRW_DxfBlockChildrenSource* physical=nullptr,
        const DRW_DxfBlockDeliverySource* delivery=nullptr,const DRW_DxfBlockDeliveryEvent* event=nullptr,
        std::uint64_t scope=0u,std::size_t index=0u) noexcept;
    void finishDxfBlockOriginAudits(dx_data&) noexcept;
    void finishBlockRoleAudit(dx_data&) noexcept;
    void captureDxfBlockMembership(AttdefOriginBlock&) noexcept;
    void observeDxfBlockMembership(AttdefOriginBlock&,const DRW_DxfBlockDeliveryEvent&,const DRW_Entity*) noexcept;
    static bool hasMatchedBlockMembership(const dx_data&,std::uint32_t) noexcept;
    using CompoundBody=dx_data::TableDxfSourceCapture::CompoundBody;
    using AttdefSourcePins=dx_data::TableDxfSourceCapture::AttdefSourcePins;
    static bool attdefPinsCurrent(const dx_data&,const AttdefSourcePins&) noexcept;
    static bool attdefPairReady(const dx_data&,const AttdefSourcePins&,std::uint32_t block) noexcept;
    void captureDxfCompoundBody(AttdefOriginBlock&,const DRW_DxfBlockDeliveryEvent&,const DRW_Entity*) noexcept;
    bool replayDxfCompoundBody(CompoundBody&);
    static bool hasMatchedCompoundBody(const dx_data&,std::uint32_t,std::size_t) noexcept;
    static bool hasMatchedCompoundSource(const dx_data&,std::uint32_t,std::size_t) noexcept;
    static TableBindingState inspectTableBlockSource(const dx_data&,std::uint32_t,std::uint32_t,TableContentBindingReport&) noexcept;
    bool chargeAttdefOrigin(std::size_t,std::size_t,bool) noexcept;
    void captureAttdefOrigin(AttdefOriginResource&) noexcept;
    void pairAttdefOrigin(AttdefOriginResource&,const DRW_Attdef&) noexcept;
    void closeAttdefOrigins(std::uint32_t) noexcept;
    static bool consistentAttdefOrigin(const dx_data::TableDxfSourceCapture&,const AttdefOriginResource&,const AttdefOriginBlock&,
                                      const dx_data::TableDxfSourceCapture::AttdefOrigin&) noexcept;
    static bool hasMatchedAttdefOrigin(const dx_data&,std::uint32_t) noexcept;
    bool tableSourceCaptureOpen() const noexcept;
    bool chargeTableSource(std::size_t bytes, std::size_t groups,
                           std::size_t records = 1u) noexcept;
    void finishTableSourceRead(bool success) noexcept;
    void finishIdentitySourceAudit(dx_data&) noexcept;
    void finishLegacyAttdefScopeJoins(dx_data&) noexcept;
    void finishLegacyBlockRecordXDictionaryChains(dx_data&) noexcept;
    void finishLegacyEntitiesCompoundJoins(dx_data&) noexcept;
    void observeStructuralSource(const DRW_TableEntry&,bool source,const DRW_RawDxfObject* raw=nullptr,
                                 bool legacyBlockOptIn=false) noexcept;
    void finishStructuralSourceAudits(dx_data&) noexcept;
    void finishLayoutRoleAudit(dx_data&) noexcept;
    void finishEntitiesRoleAudit(dx_data&) noexcept;
    void retainTableSourceDictionary(const DRW_Dictionary& data);
    void retainTableSourceXRecord(const DRW_XRecord& data);
    bool retainTableSourceRaw(const DRW_RawDxfObject& data);
    bool proveTableContentSource(const DRW_RawDxfObject& data);
    bool chargeTableResource(std::size_t bytes, std::size_t items) noexcept;
    template<class T> void retainTableResource(dx_tableResourceFamily family,const T& data) noexcept;
    void retainTableEntityIdentity(const DRW_Entity& data) noexcept;
    void retainTableResourceSource(const DRW_RawDxfObject& data,const DRW_TableEntry* symbol=nullptr) noexcept;
    void proveTableResourceSource(std::uint32_t handle) noexcept;
    void proveAttdefSourceFields(std::uint32_t handle) noexcept;
    void retainDxfBlockSource(const DRW_Block&,const DRW_RawDxfObject&,bool footer) noexcept;
    void pairDxfBlockSource(const DRW_Block&,bool footer) noexcept;
    void closeDxfBlockSource() noexcept;
    void proveDxfBlockSource(std::uint32_t handle) noexcept;
    bool chargeDxfBlockSource(const DRW_Block&,const DRW_RawDxfObject*) noexcept;
    friend class DxIfaceDwgClassSourceTestAccess;
    dx_data* m_classEvidenceReadData{nullptr};
    std::uint64_t m_classEvidenceReadAttempt{0};
    void rejectDwgClassEvidence(dx_data& data, dx_class_evidence::State reason) noexcept;
    dx_data::ClassEvidenceImport* activeDwgClassEvidence() noexcept;
    void beginDwgClassEvidenceRead(dx_data& data) noexcept;
    void retainDwgClassEvidenceRows(const DRW_DwgClassCoverageReport& data) noexcept;
    void retainDwgClassEvidenceSource(const DRW_DwgClassSourceReport& data) noexcept;
    void finishDwgClassEvidenceRead(bool success, DRW::Version version, std::uint16_t codePage,
                                   bool present, bool recognized) noexcept;
    void abortDwgClassEvidenceRead() noexcept;
    friend class DxIfaceExtensionGraphTestAccess;
    friend class DxIfaceStandardDwgCaptureTestAccess;
    static bool testAc1024DynamicBlockOwnerDictionary(
        const dx_data& data, std::uint32_t dictionaryHandle) noexcept;
    friend class DxIfaceTableSourceTestAccess;
    friend class DxIfaceMaterialReferenceTestAccess;
    void rejectStandardDwgObjectCapture() noexcept;
    void retainStandardDwgObjectFrame(const DRW_UnsupportedObject& data);
    void finishStandardDwgObjectCapture(bool successfulRead) noexcept;
    void retainRasterDwgSource(const DRW_UnsupportedObject& data) noexcept;
    void retainRasterDwgPublication(const DRW_DwgFramePublication& data) noexcept;
    std::uint64_t rasterClassReadAttempt() noexcept;
    dx_class_evidence::RasterBinding rasterClassBinding(const dx_data::RasterDwgObjectFrame& frame) const noexcept;
    std::shared_ptr<dx_data::RasterDwgNameImport> m_rasterDwgNameRead;
    void beginRasterDwgNamesRead(dx_data& data) noexcept;
    void finishRasterDwgNamesRead(bool successfulRead, DRW::Version version, std::uint16_t page,
                                 bool present, bool recognized) noexcept;
    void retainRasterDwgName(std::uint32_t handle, const UTF8STRING& name,
                            const std::shared_ptr<const DRW_DwgTableNameSource>& source, bool layer) noexcept;
    std::shared_ptr<const dx_data::RasterDwgNameImport> rasterDwgNameImport(
        const dx_data::RasterDwgObjectFrame& frame) const noexcept;
    void finishRasterDwgSources(bool successfulRead, std::size_t appIdBegin = 0u, std::size_t layerBegin = 0u) noexcept;
    bool hasUnchangedRasterDwgSources() const noexcept;
    struct StandardDwgObjectImportRange {
        std::size_t dictionaryBegin{0};
        std::size_t groupBegin{0};
        std::size_t xRecordBegin{0};
        std::size_t appIdBegin{0};
        std::size_t layerBegin{0};
    };
    void retainStandardDwgAppIdSource(const DRW_AppId& data);
    void retainStandardDwgLayerSource(const DRW_Layer& data);
    void validateStandardDwgObjectPayloads(
        const StandardDwgObjectImportRange& range) noexcept;
    bool hasUnchangedStandardDwgObjectSources() const noexcept;
    const std::vector<DRW_Group>* m_standardDwgBorrowedGroups{nullptr};
    struct StandardDwgOutputProof {
        std::uint64_t generation{0};
        std::uint32_t emitted{0}, owner{0}, extensionDictionary{0};
        int objectType{0};
        bool identityOnlyDictionary{false}, ownerIsBlockRecord{false};
        bool ownerIsLayerControl{false};
        std::vector<std::uint32_t> references;
    };
    std::map<std::uint32_t, StandardDwgOutputProof> m_standardDwgOutputProofs;
    struct StandardDwgLayerControlOutputProof {
        std::uint64_t generation{0};
        std::uint32_t sourceControl{0}, sourceDictionary{0};
        std::uint32_t emittedTable{0}, emittedDictionary{0};
        bool inspected{false}, matched{false};
    };
    StandardDwgLayerControlOutputProof m_standardDwgLayerControlOutputProof;
    struct StandardDwgOutputNameProof {
        std::uint64_t generation{0};
        std::uint32_t source{0}, emitted{0}, table{0};
        bool layer{false}, eligible{true}, matched{false};
        unsigned nameRows{0u}, handleRows{0u};
    };
    using StandardDwgOutputNameKey = std::pair<std::uint64_t, std::uint32_t>;
    std::map<StandardDwgOutputNameKey, StandardDwgOutputNameProof> m_standardDwgOutputNameProofs;
    std::size_t m_standardDwgOutputNameBytes{0u};
    bool m_standardDwgOutputNamesInspected{false};
    std::size_t m_standardDwgOutputObserved{0};
    bool m_standardDwgOutputInspected{false};
    bool m_standardDwgOutputFinalized{false};
    bool m_standardDwgOutputPublished{false};
    struct DwgPlotSettingsOutputProof {
        std::uint64_t generation{0u};
        std::uint32_t source{0u}, emitted{0u}, owner{0u};
        std::vector<std::uint32_t> references;
    };
    std::map<std::uint32_t, DwgPlotSettingsOutputProof>
        m_dwgPlotSettingsOutputProofs;
    std::size_t m_dwgPlotSettingsOutputBytes{0u};
    bool m_dwgPlotSettingsOutputInspected{false};
    bool m_dwgPlotSettingsOutputFinalized{false};
    bool m_dwgPlotSettingsOutputPublished{false};
    struct RasterOutputProof {
        dxfRW::ImageDefinitionKey definition;
        std::uint32_t sourceReactor{0}, emitted{0}, owner{0}, extension{0};
        bool reactor{false};
        std::vector<std::uint32_t> references;
    };
    std::vector<RasterOutputProof> m_rasterOutputProofs;
    std::vector<std::uint32_t> m_rasterDictionaryReferences;
    std::uint32_t m_rasterDictionaryEmitted{0u};
    bool m_rasterDictionaryCommonCompared{false};
    bool inspectRasterDictionaryOutput(const dxfRW::DxfObjectsWriteView& view) noexcept;
    bool hasUnchangedRasterDictionaryOutputSource(bool final = false) const;
    struct GeneratedRootOutputProof {
        std::uint32_t source{0u}, emitted{0u}, owner{0u}, extension{0u};
        bool groups{false}, commonCompared{false};
        std::vector<std::uint32_t> references;
    };
    std::vector<GeneratedRootOutputProof> m_generatedRootOutputProofs;
    std::size_t m_generatedRootOutputBytes{0u};
    std::size_t m_generatedRootOutputByteLimit{ImageDefinitionCaptureMaxRetainedBytes};
    bool m_generatedRootOutputInspected{false}, m_generatedRootOutputFinalized{false}, m_generatedRootOutputPublished{false};
    bool inspectGeneratedRootOutput(const dxfRW::DxfObjectsWriteView& view) noexcept;
    bool hasUnchangedGeneratedRootSources(bool final) const;
    bool finalizeGeneratedRootOutput() noexcept;
    // Per-write typed snapshots, not independent import/support evidence.
    std::vector<DRW_XRecord> m_xrecordWriteSources;
    std::vector<std::uint32_t> m_xrecordWriteSourceHandles;
    // Freeze the selecting ordinary dictionary closure as well as its leaves.
    // Generated NOD/GROUP roots retain their separate source/output guards.
    std::vector<DRW_Dictionary> m_xrecordWriteDictionaries;
    std::vector<std::uint32_t> m_xrecordWriteDictionaryHandles;
    const dx_data* m_xrecordWriteSourceOwner{nullptr};
    dx_source_format m_xrecordWriteSourceFormat{dx_source_format::Unknown};
    DRW::Version m_xrecordWriteSourceVersion{DRW::UNKNOWNV};
    std::uint64_t m_xrecordWriteSourceGeneration{0u};
    std::size_t m_xrecordWriteSourceBytes{0u};
    std::size_t m_xrecordWriteSourceByteLimit{ImageDefinitionCaptureMaxRetainedBytes};
    bool captureXrecordWriteSources() noexcept;
    bool hasUnchangedXrecordWriteSources() const noexcept;
    bool isDxfXRecordCompatibilityReplaySource(
        const DRW_XRecord& record) const noexcept;
    bool isDxfLayerStateCompatibilityReplaySource(
        const DRW_XRecord& record) const noexcept;
    struct RasterImageEdges {
        std::uint32_t source{0}, emitted{0}, owner{0}, definition{0}, reactor{0};
        dxfRW::ImageDefinitionKey sourceDefinition;
        std::uint32_t sourceReactor{0};
        bool blocks{false};
    };
    std::map<std::uint32_t, RasterImageEdges> m_rasterImageEdges;
    unsigned m_rasterEntitySectionsInspected{0};
    std::size_t m_rasterOutputBytes{0u};
    std::size_t m_rasterOutputByteLimit{ImageDefinitionCaptureMaxRetainedBytes};
    bool m_rasterOutputInspected{false}, m_rasterOutputFinalized{false}, m_rasterOutputPublished{false};
    bool m_dwgOle2FrameOutputInspected{false};
    bool m_requireCompleteDwgOutputQualification{false};
    bool m_debugDwgOutputQualification{false};
    using OpaqueDwgOutputKey = std::pair<std::uint64_t, std::uint32_t>;
    std::set<OpaqueDwgOutputKey>
        m_pendingOpaqueDwgOutputQualificationFrameIdentities;
    bool inspectRasterOutput(const dxfRW::DxfObjectsWriteView& view) noexcept;
    bool inspectRasterImageEdges(const dxfRW::DxfObjectsWriteView& view, std::string_view section) noexcept;
    bool inspectDwgOle2FrameOutput(
        const dxfRW::DxfObjectsWriteView& view,
        std::string_view section) noexcept;
    bool finalizeRasterOutputProofs() noexcept;
    void resetStandardDwgOutputProofs() noexcept;
    bool inspectStandardDwgOutput(const dxfRW::DxfObjectsWriteView& view) noexcept;
    bool inspectDxfSunStudyOutput(const dxfRW::DxfObjectsWriteView& view) noexcept;
    bool hasUnchangedSunStudyOutputGraph() const noexcept;
    bool inspectStandardDwgTablesOutput(const dxfRW::DxfTablesWriteView& view) noexcept;
    bool retainDwgPlotSettingsPublication(
        const DRW_DwgFramePublication& publication) noexcept;
    bool hasUnchangedDwgPlotSettingsSources() const noexcept;
    bool inspectDwgPlotSettingsOutput(
        const dxfRW::DxfObjectsWriteView& view) noexcept;
    bool finalizeDwgPlotSettingsOutputProofs() noexcept;
    bool hasQualifiedStandardDwgOutputNames(std::uint64_t generation,
                                          std::uint32_t handle, bool finalizing) const;
    bool finalizeStandardDwgOutputProofs() noexcept;
    bool recordOpaqueDwgSourceFrameIdentity(
        const DRW_UnsupportedObject& data) noexcept;
    // Call only after an exact source identity has passed its writer-output
    // and fresh-readback proof; a callback or aggregate count is insufficient.
    bool recordOpaqueDwgOutputQualification(
        std::uint32_t sourceHandle) noexcept;
    bool hasCompletePendingOpaqueDwgOutputQualification() const noexcept;
    bool retainOpaqueDwgSourceFrame(
        const DRW_UnsupportedObject& data) noexcept;
    bool estimateTypedEntityRetainedBytes(
        const DRW_Entity& entity, std::size_t inlineBytes,
        std::size_t& estimatedBytes) const noexcept;
    bool estimateTypedBlockRetainedBytes(
        const DRW_Block& block, bool includeContainerStorage,
        std::size_t& estimatedBytes) const noexcept;
    bool prepareDwgViewportProjection(std::string& failure);
    bool inspectDwgViewportXrecords(
        const dxfRW::DxfObjectsWriteView& view) noexcept;
    bool finalizeDwgViewportOutputQualifications() noexcept;
    bool planLeaderAnnotationHandles();
    bool planDwgProxyEntityObjectIds();
    bool finalizeDwgProxyEntityObjectIds();
    bool preallocateBlockRecordXdataEntityHandles();

    dx_dxfSourceTextAdmission m_sourceTextAdmission;
    void countDwgReadCallback(std::size_t& count) noexcept {
        if (count != std::numeric_limits<std::size_t>::max())
            ++count;
        else
            cData->dwgReadOutcome.callbackCountsSaturated = true;
    }
    template <typename EntityT, typename SourceT>
    EntityT* retainTypedEntity(SourceT&& source) noexcept {
        if (cData == nullptr)
            return nullptr;
        observeDxfBlockDeliveryValue(static_cast<std::int32_t>(source.eType),source.handle,source.parentHandle,
                                    DRW_DxfBlockCallback::Count,nullptr,&source);
        using OriginalModel=typename std::conditional<
            std::is_same<EntityT,dx_ifaceImg>::value, DRW_Image,
            typename std::conditional<
                std::is_same<EntityT, dx_ifaceUnderlay>::value,
                DRW_Underlay, typename std::conditional<
                    std::is_same<EntityT, dx_ifaceDwgLight>::value,
                    DRW_Light, EntityT>::type>::type>::type;
        observeDxfEntitiesTyped<OriginalModel>(source);
        retainTableEntityIdentity(static_cast<const DRW_Entity&>(source));
        if (currentBlock == nullptr
            || cData->typedEntityRetainedRecords
                >= TypedEntityCaptureMaxRecords) {
            cData->typedEntityCaptureFailed = true;
            return nullptr;
        }

        std::size_t estimatedBytes = 0u;
        if (!estimateTypedEntityRetainedBytes(
                static_cast<const DRW_Entity&>(source), sizeof(EntityT),
                estimatedBytes)
            || estimatedBytes > TypedEntityCaptureMaxRetainedBytes
                - TypedEntityListNodeBytes
            || cData->typedEntityRetainedBytes
                > TypedEntityCaptureMaxRetainedBytes
            || estimatedBytes + TypedEntityListNodeBytes
                > TypedEntityCaptureMaxRetainedBytes
                - cData->typedEntityRetainedBytes) {
            cData->typedEntityCaptureFailed = true;
            return nullptr;
        }
        estimatedBytes += TypedEntityListNodeBytes;

        EntityT* entity = nullptr;
        try {
            entity = new EntityT(std::forward<SourceT>(source));
            currentBlock->ent.push_back(entity);
            ++cData->typedEntityRetainedRecords;
            cData->typedEntityRetainedBytes += estimatedBytes;
            if (cData->sourceFormat == dx_source_format::DWG
                && cData->importGeneration != 0u
                && entity->hasDwgAcDbColorHandle()) {
                const std::uint32_t colorHandle =
                    entity->dwgAcDbColorHandle();
                const dx_data::DwgDbColorKey key{
                    cData->importGeneration, colorHandle};
                constexpr std::size_t mapNodeOverhead = 4u * sizeof(void*);
                constexpr std::size_t referenceNodeBytes =
                    sizeof(std::pair<const dx_data::DwgDbColorKey,
                                     DRW_Entity*>)
                    + mapNodeOverhead;
                if (colorHandle == 0u || colorHandle == DRW::NoHandle
                    || cData->dwgDbColorReferenceRetainedRecords
                        >= DwgDbColorReferenceMaxRecords
                    || cData->dwgDbColorReferenceRetainedBytes
                        > DwgDbColorReferenceMaxRetainedBytes
                    || referenceNodeBytes
                        > DwgDbColorReferenceMaxRetainedBytes
                            - cData->dwgDbColorReferenceRetainedBytes) {
                    cData->dwgDbColorCaptureFailed = true;
                    cData->typedEntityCaptureFailed = true;
                    return entity;
                }
                try {
                    cData->dwgDbColorEntityReferences.emplace(key, entity);
                    ++cData->dwgDbColorReferenceRetainedRecords;
                    cData->dwgDbColorReferenceRetainedBytes +=
                        referenceNodeBytes;
                } catch (...) {
                    cData->dwgDbColorCaptureFailed = true;
                    cData->typedEntityCaptureFailed = true;
                }
            }
            return entity;
        } catch (...) {
            delete entity;
            cData->typedEntityCaptureFailed = true;
            return nullptr;
        }
    }
    std::shared_ptr<dx_dwgLightImport> m_dwgLightImport;
    bool m_collectDwgLightImport{false};
    std::shared_ptr<dx_dwgDimensionImport> m_dwgDimensionImport;
    bool m_collectDwgDimensionImport{false};
    template<class Dimension>
    void retainDimension(const DRW_Dimension& value) noexcept {
        if (!m_collectDwgDimensionImport || cData == nullptr
            || cData->sourceFormat != dx_source_format::DWG) {
            retainTypedEntity<Dimension>(value);
            return;
        }
        try {
            if (!m_dwgDimensionImport) {
                constexpr std::size_t originBytes =
                    sizeof(dx_dwgDimensionImport) + 2u * sizeof(void*);
                if (cData->typedEntityRetainedBytes > TypedEntityCaptureMaxRetainedBytes
                    || originBytes > TypedEntityCaptureMaxRetainedBytes
                        - cData->typedEntityRetainedBytes) {
                    cData->typedEntityCaptureFailed = true;
                    return;
                }
                m_dwgDimensionImport = std::make_shared<dx_dwgDimensionImport>();
                m_dwgDimensionImport->generation = cData->importGeneration;
                cData->typedEntityRetainedBytes += originBytes;
            }
            auto* entity = retainTypedEntity<dx_ifaceDwgDimension<Dimension>>(value);
            if (entity != nullptr) {
                entity->source = m_dwgDimensionImport;
                entity->capturedSource = m_dwgDimensionImport;
                entity->sourceGeneration = cData->importGeneration;
            }
        } catch (...) {
            cData->typedEntityCaptureFailed = true;
        }
    }
    bool prepareExtensionObjectGraph(dx_data* data,
                                     bool includeNamedObjectRoot);
    bool hasUnchangedDwgDbColorEedDependencies(const dx_data& data) const noexcept;
    bool preparePointCloudExOutputGraph(dx_data& data,
                                        DRW::Version targetVersion,
                                        bool debug);
    bool preparePointCloudClassicOutputGraph(dx_data& data,
                                             DRW::Version targetVersion,
                                             bool debug);
    bool preparePointCloudOutputGraph(dx_data& data,
                                      DRW::Version targetVersion,
                                      bool debug);
    bool validateImportedDxfPointCloudSourceBindings(
        dx_data& data, std::size_t& boundCount,
        const char*& failure,
        bool allowDefinitionReferences = false) noexcept;
    bool validateImportedDxfPointCloudExSourceBindings(
        dx_data& data, std::size_t& boundCount,
        const char*& failure) noexcept;
    bool prepareDwgGeoDataOutput(bool debug);
    bool prepareDwgRasterVariablesOutput(DRW::Version targetVersion,
                                         bool debug);
    bool prepareDwgWipeoutVariablesOutput(DRW::Version targetVersion,
                                          bool debug);
    bool prepareTableFallbackGraph(dx_data& data, DRW::Version targetVersion);
    void writeEntityOrTableFallback(DRW_Entity* entity);
    bool validateSortEntsGraph(const dx_data& data, bool debug = false);
    bool validateHatchBoundaryGraph(const dx_data& data);
    void recordHatchBlockPublication(const std::string& name, bool written);
    void failHatchPublication() noexcept;
    bool validateVisualStyleReferences(const dx_data& data,
                                       DRW::Version targetVersion,
                                       const char** failure) const;
    bool validatePlotSettingsGraph(const dx_data& data,
                                   DRW::Version targetVersion,
                                   const char** failure) const;
    bool validateRenderSettingsGraph(const dx_data& data,
                                     DRW::Version targetVersion,
                                     const char** failure) const;
    static bool collectAcdsHistoryProxyObjects(
        const dx_data& data, std::vector<DRW_RawDxfObject>& objects,
        std::vector<std::uint32_t>& materialHandles, bool debug);
    static bool collectAc1024AcdsOpaqueProxyObjects(
        const dx_data& data, std::vector<DRW_RawDxfObject>& objects,
        std::vector<std::uint32_t>& materialHandles, bool debug);
    static bool collectAcdsTypedPrimitiveHistoryObjects(
        const dx_data& data, std::vector<DRW_RawDxfObject>& objects,
        std::vector<DRW_EvaluationGraph>& graphs,
        std::vector<std::uint32_t>& materialHandles, bool debug);
    std::vector<DRW_RawDxfObject> m_acdsHistoryObjects;
    std::vector<DRW_RawDxfObject> m_acdsDxfHistoryObjects;
    std::vector<DRW_Class> m_acdsDxfHistoryClasses;
    dx_acdsHistoryDxfImportReceipt m_acdsDxfHistoryReceipt;
    std::uint64_t m_acdsDxfHistoryGeneration{0};
    bool m_acdsDxfHistoryInspected{false};
    bool prepareAcdsDxfHistory(DRW::Version version);
    bool hasUnchangedAcdsDxfHistorySources() const noexcept;
    bool inspectAcdsDxfHistory(const dxfRW::DxfObjectsWriteView& view) noexcept;
    std::vector<DRW_RawDxfObject> m_acdsTypedHistoryObjects;
    std::vector<DRW_EvaluationGraph> m_acdsTypedEvaluationGraphs;
    std::vector<std::uint32_t> m_acdsMaterialHandles;
    std::vector<std::uint32_t> m_dxfMaterialHandles;
    struct DxfMaterialWriteSource {
        std::shared_ptr<const DRW_RawDxfObject> baseline;
        DRW_RawDxfObject raw;
        DRW_Material common;
        DRW_RawDxfObject output;
        UTF8STRING outputName;
    };
    std::vector<DxfMaterialWriteSource> m_dxfMaterialWriteSources;
    struct DxfDictionaryVarSource {
        DRW_DictionaryVar model;
        DRW_RawDxfObject raw, output;
        std::shared_ptr<const DRW_RawDxfObject> baseline;
        int schema{0};
        UTF8STRING value;
        std::vector<std::pair<std::size_t,UTF8STRING>> stringOverrides;
    };
    std::unordered_set<std::uint32_t> m_customDxfDictionaryVarHandles;
    std::unordered_map<std::uint32_t,DxfDictionaryVarSource> m_dxfDictionaryVars;
    const dx_data* m_dxfDictionaryVarOwner{nullptr};
    std::uint64_t m_dxfDictionaryVarGeneration{0};
    bool m_dxfDictionaryVarsInspected{false};
    DRW::Version m_dxfDictionaryVarOutputVersion{DRW::UNKNOWNV};
    UTF8STRING m_dxfDictionaryVarOutputPage, m_dxfDictionaryVarOutputDeclaration;
    bool m_dxfDictionaryVarDeclarationPresent{false};
    bool m_dxfDictionaryVarOutputBinary{false};
    bool prepareDxfDictionaryVars(DRW::Version version,bool binary,
        DRW::DxfTextWritePolicy policy=DRW::DxfTextWritePolicy::Compatible) noexcept;
    bool hasUnchangedDxfDictionaryVars() const noexcept;
    bool inspectDxfDictionaryVars(const dxfRW::DxfObjectsWriteView&) noexcept;
    struct DxfNamedResourceState {
        const dx_data* owner{nullptr};
        std::uint64_t generation{0u};
        DRW::Version version{DRW::UNKNOWNV};
        UTF8STRING codePage, declaration;
        bool declarationPresent{false}, binary{false}, nodPresent{false};
        std::unordered_map<std::uint32_t,DRW_Dictionary> dictionaries;
    };
    bool prepareDxfNamedResourceState(DxfNamedResourceState&, DRW::Version,
        bool binary, std::size_t& chargedBytes);
    bool hasUnchangedDxfNamedResourceState(const DxfNamedResourceState&) const noexcept;
    struct DxfScaleSource {
        DRW_Scale model, projection;
        DRW_RawDxfObject raw, output;
        std::shared_ptr<const DRW_RawDxfObject> baseline;
        std::vector<std::pair<std::size_t,UTF8STRING>> stringOverrides;
    };
    struct DwgScaleOutputSource {
        DRW_Scale model;
    };
    struct DwgDictionaryVarOutputSource {
        DRW_DictionaryVar model;
    };
    std::map<std::uint32_t,DwgDictionaryVarOutputSource>
        m_dwgDictionaryVarOutputSources;
    std::map<std::uint32_t,DRW_Dictionary> m_dwgDictionaryVarOutputDictionaries;
    const dx_data* m_dwgDictionaryVarOutputOwner{nullptr};
    std::uint64_t m_dwgDictionaryVarOutputGeneration{0u};
    DRW::Version m_dwgDictionaryVarOutputVersion{DRW::UNKNOWNV};
    DRW::Version m_dwgDictionaryVarTargetVersion{DRW::UNKNOWNV};
    bool m_dwgDictionaryVarOutputBinary{false};
    bool m_dwgDictionaryVarsInspected{false};
    std::size_t m_dwgDictionaryVarOutputBytes{0u};
    bool prepareDwgDictionaryVars(DRW::Version version, bool binary,
                                  bool debug) noexcept;
    bool hasUnchangedDwgDictionaryVars() const noexcept;
    bool inspectDwgDictionaryVars(
        const dxfRW::DxfObjectsWriteView& view, bool debug) noexcept;
    std::unordered_map<std::uint32_t,DxfScaleSource> m_dxfScales;
    std::map<std::uint32_t,DwgScaleOutputSource> m_dwgScaleOutputSources;
    std::map<std::uint32_t,DRW_Dictionary> m_dwgScaleOutputDictionaries;
    const dx_data* m_dwgScaleOutputOwner{nullptr};
    std::uint64_t m_dwgScaleOutputGeneration{0u};
    DRW::Version m_dwgScaleOutputVersion{DRW::UNKNOWNV};
    DRW::Version m_dwgScaleTargetVersion{DRW::UNKNOWNV};
    bool m_dwgScaleOutputBinary{false};
    bool m_dwgScalesInspected{false};
    std::size_t m_dwgScaleOutputBytes{0u};
    DxfNamedResourceState m_dxfScaleState;
    bool m_dxfScalesInspected{false};
    bool prepareDxfScales(DRW::Version version, bool binary,
        DRW::DxfTextWritePolicy policy=DRW::DxfTextWritePolicy::Compatible) noexcept;
    bool prepareDwgScales(DRW::Version version, bool binary,
                          bool debug) noexcept;
    bool hasUnchangedDwgScales() const noexcept;
    bool inspectDwgScales(const dxfRW::DxfObjectsWriteView&, bool debug) noexcept;
    bool hasUnchangedDxfScales() const noexcept;
    bool inspectDxfScales(const dxfRW::DxfObjectsWriteView&) noexcept;
    struct DxfMLeaderStyleSource {
        DRW_MLeaderStyle model, projection;
        DRW_RawDxfObject raw, output;
        std::shared_ptr<const dx_data::NamedResourceBodyBaseline> baseline;
        std::vector<int> editedFieldCodes;
    };
    struct DwgMLeaderStyleOutputSource {
        DRW_MLeaderStyle model;
        std::vector<DRW_Variant> eed;
    };
    std::unordered_map<std::uint32_t,DxfMLeaderStyleSource> m_dxfMLeaderStyles;
    std::unordered_map<std::uint32_t,DwgMLeaderStyleOutputSource>
        m_dwgMLeaderStyleOutputSources;
    const dx_data* m_dwgMLeaderStyleOutputOwner{nullptr};
    std::uint64_t m_dwgMLeaderStyleOutputGeneration{0u};
    bool m_dwgMLeaderStylesInspected{false};
    DxfNamedResourceState m_dxfMLeaderStyleState;
    bool m_dxfMLeaderStylesInspected{false};
    bool prepareDxfMLeaderStyles(DRW::Version version, bool binary,
        DRW::DxfTextWritePolicy policy=DRW::DxfTextWritePolicy::Compatible) noexcept;
    bool hasUnchangedDxfMLeaderStyles() const noexcept;
    bool hasUnchangedDwgMLeaderStyleOutputSources() const noexcept;
    bool inspectDxfMLeaderStyles(const dxfRW::DxfObjectsWriteView&) noexcept;
    bool inspectDwgMLeaderStyles(const dxfRW::DxfObjectsWriteView&) noexcept;
    struct DwgMLineStyleOutputSource {
        DRW_MLineStyle model;
        DRW_Dictionary rootOwner;
        std::uint32_t nodHandle{0u};
        int nodHardOwner{0};
        DRW_Dictionary::Entry nodRootEntry;
        std::vector<std::pair<std::uint32_t, std::string>> linetypeDependencies;
    };
    std::map<std::uint32_t,DwgMLineStyleOutputSource>
        m_dwgMLineStyleOutputSources;
    const dx_data* m_dwgMLineStyleOutputOwner{nullptr};
    std::uint64_t m_dwgMLineStyleOutputGeneration{0u};
    bool m_dwgMLineStylesInspected{false};
    bool prepareDwgMLineStyles(DRW::Version version) noexcept;
    bool hasUnchangedDwgMLineStyleOutputSources() const noexcept;
    bool inspectDwgMLineStyles(const dxfRW::DxfObjectsWriteView&,
                               bool debug) noexcept;
    struct DwgLayoutOutputSource {
        DRW_Layout model;
        DRW_Block_Record blockRecord;
        DRW_Dictionary extensionDictionary;
        std::vector<std::uint32_t> extensionXRecordHandles;
    };
    struct DwgNodDictionaryOutputSource {
        DRW_Dictionary model;
        DRW_Dictionary::Entry nodEntry;
    };
    std::map<std::uint32_t,DwgLayoutOutputSource>
        m_dwgLayoutOutputSources;
    std::map<std::uint32_t,DwgNodDictionaryOutputSource>
        m_dwgNodDictionaryOutputSources;
    DRW_Dictionary m_dwgLayoutOutputDictionary;
    DRW_Dictionary m_dwgLayoutOutputNod;
    bool m_dwgLayoutOutputNodComplete{false};
    std::uint32_t m_dwgLayoutOutputNodHandle{0u};
    int m_dwgLayoutOutputNodHardOwner{0};
    DRW_Dictionary::Entry m_dwgLayoutOutputNodEntry;
    const dx_data* m_dwgLayoutOutputOwner{nullptr};
    std::uint64_t m_dwgLayoutOutputGeneration{0u};
    DRW::Version m_dwgLayoutOutputSourceVersion{DRW::UNKNOWNV};
    bool m_dwgLayoutsObjectsInspected{false};
    bool m_dwgLayoutBlocksInspected{false};
    bool prepareDwgLayouts(DRW::Version version, bool debug) noexcept;
    bool hasUnchangedDwgLayoutOutputSources(bool debug=false) const noexcept;
    bool inspectDwgLayouts(const dxfRW::DxfObjectsWriteView&,
                           bool debug) noexcept;
    bool inspectDwgLayoutBlockRecords(const dxfRW::DxfTablesWriteView&,
                                     bool debug) noexcept;
    struct DwgLayerStateOutputSource {
        std::uint32_t layerControlHandle{0u};
        DRW_Dictionary extensionDictionary;
        DRW_Dictionary layerStatesDictionary;
        std::vector<DRW_Dictionary> layerStateCompanionDictionaries;
        DRW_XRecord layerStateRecord;
        bool hasLayerStateRecord{false};
    };
    DwgLayerStateOutputSource m_dwgLayerStateOutputSource;
    const dx_data* m_dwgLayerStateOutputOwner{nullptr};
    std::uint64_t m_dwgLayerStateOutputGeneration{0u};
    DRW::Version m_dwgLayerStateOutputSourceVersion{DRW::UNKNOWNV};
    bool m_dwgLayerStateOutputPrepared{false};
    bool m_dwgLayerStateOutputInspected{false};
    bool prepareDwgLayerStateOutput(DRW::Version targetVersion,
                                    bool debug) noexcept;
    bool isDwgLayerStateCompatibilityRecord(
        const DRW_XRecord& record) const noexcept;
    bool hasUnchangedDwgLayerStateOutputSource() const noexcept;
    bool inspectDwgLayerStateOutput(const dxfRW::DxfObjectsWriteView&,
                                    bool debug) noexcept;
    struct DxfMLineStyleSource {
        DRW_MLineStyle model, projection;
        DRW_RawDxfObject raw, output;
        std::shared_ptr<const DRW_RawDxfObject> baseline;
        std::shared_ptr<const dx_data::TableDxfSourceCapture::StructuralAudit> dictionaryOrigin;
        std::vector<std::pair<std::size_t,UTF8STRING>> stringOverrides;
        bool geometryChanged{false};
        bool referencesChanged{false};
        bool linetypesBound{false};
        bool countChanged{false};
    };
    std::unordered_map<std::uint32_t,DxfMLineStyleSource> m_dxfMLineStyles;
    DxfNamedResourceState m_dxfMLineStyleState;
    bool m_dxfMLineStylesInspected{false};
    // Selected LAYER, dictionary, LTYPE and MLINESTYLE body proofs are cached
    // only within one export; finalization repeats them after writer callbacks.
    mutable bool m_dxfMLineDependencySourceLivePreflightPassed{false};
    static bool chargeMlineComparisonWork(const std::vector<DRW_MLineElement>& elements,
        std::size_t passes,std::size_t& work,std::size_t limit) noexcept;
    bool prepareDxfMLineStyles(DRW::Version version, bool binary,
        DRW::DxfTextWritePolicy policy=DRW::DxfTextWritePolicy::Compatible) noexcept;
    bool hasUnchangedDxfMLineStyles(bool deferConsumedGeometry=false) const noexcept;
    bool hasCompleteDxfMLineConsumerCensus(bool requireUnchangedModels) const noexcept;
    bool needsDxfMLineNoRawSectionsGuard() const noexcept;
    bool needsDxfMLineConsumerInspection() const noexcept;
    bool inspectDxfMLineStyles(const dxfRW::DxfObjectsWriteView&) noexcept;
    enum class DxfMLineXdataEntitySection : std::uint8_t {
        Entities,
        Blocks
    };
    struct DxfMLineLinetypeSource {
        const DRW_LType* live{nullptr};
        DRW_LType model;
        std::shared_ptr<const dx_data::TableDxfSourceCapture::ResourceOriginAudit> origin;
        std::vector<std::uint32_t> appIdHandles;
        std::vector<std::uint32_t> layerHandles;
        std::vector<std::uint32_t> xdataEntityHandles;
        std::vector<const DRW_Entity*> xdataEntityModels;
        std::vector<std::uint32_t> xdataEntityOwners;
        std::vector<DRW::ETYPE> xdataEntityTypes;
        std::vector<DxfMLineXdataEntitySection> xdataEntitySourceSections;
        std::vector<std::uint32_t> xdataEntityBlockRecords;
        std::uint32_t emitted{0},table{0};
    };
    struct DxfMLineXdataEntitySource {
        const DRW_Entity* live{nullptr};
        std::uint32_t owner{0u};
        DRW::ETYPE entityType{DRW::UNKNOWN};
        std::uint32_t referencedBlockRecord{0u};
        DxfMLineXdataEntitySection sourceSection{
            DxfMLineXdataEntitySection::Entities};
    };
    struct DxfMLineAppIdSource {
        const DRW_AppId* live{nullptr};
        DRW_AppId model;
        std::shared_ptr<const dx_data::TableDxfSourceCapture::ResourceOriginAudit> origin;
        std::uint32_t emitted{0},table{0};
    };
    struct DxfMLineLayerSource {
        const DRW_Layer* live{nullptr};
        DRW_Layer model;
        std::shared_ptr<const dx_data::TableDxfSourceCapture::ResourceOriginAudit> origin;
        std::uint32_t emitted{0},table{0};
    };
    std::map<std::uint32_t,DxfMLineLinetypeSource> m_dxfMLineLinetypes;
    std::map<std::uint32_t,DxfMLineAppIdSource> m_dxfMLineAppIds;
    std::map<std::uint32_t,DxfMLineLayerSource> m_dxfMLineLayers;
    bool m_dxfMLineLinetypesInspected{false};
    std::size_t m_dxfMLineLinetypeCount{0u};
    std::size_t m_dxfMLineLayerCount{0u};
    struct DwgLtypeOutputBinding {
        std::uint32_t sourceHandle{0u};
        std::uint32_t emittedHandle{0u};
        std::string sourceName;
        std::string outputName;
    };
    struct DwgLayerOutputBinding {
        std::uint32_t sourceHandle{0u};
        std::uint32_t emittedHandle{0u};
        std::string sourceName;
        std::string outputName;
    };
    bool m_dwgLtypeControlOutputRequired{false};
    bool m_dwgLtypeControlOutputInspected{false};
    std::vector<DwgLtypeOutputBinding> m_dwgLtypeControlOutputBindings;
    std::unordered_map<std::uint32_t, std::size_t>
        m_dwgLtypeControlOutputBindingIndex;
    bool m_dwgLayerOutputInspected{false};
    std::vector<DwgLayerOutputBinding> m_dwgLayerOutputBindings;
    std::unordered_map<std::uint32_t, std::size_t>
        m_dwgLayerOutputBindingIndex;
    bool prepareDxfMLineLinetypes(std::size_t chargedBytes=0u) noexcept;
    bool hasUnchangedDxfMLineLinetypes() const noexcept;
    bool collectDxfMLineXdataEntitySources(
        std::map<std::uint32_t,DxfMLineXdataEntitySource>&,
        std::size_t&,std::size_t&) const noexcept;
    bool inspectDxfMLineLinetypes(const dxfRW::DxfTablesWriteView&) noexcept;
    bool inspectDwgLineTypeControlOutput(
        const dxfRW::DxfTablesWriteView&) noexcept;
    bool finalizeDwgLineTypeControlOutput() const noexcept;
    bool hasClosedDwgLinetypeReference(std::uint32_t, const std::string&,
                                       bool allowImplicit) const noexcept;
    bool hasClosedDwgLayerLinetypeReference(
        const DRW_Layer&) const noexcept;
    bool hasClosedDwgEntityLayerReferenceTree(
        const DRW_Entity*) const noexcept;
    bool hasClosedDwgEntityLinetypeReferenceTree(
        const DRW_Entity*) const noexcept;
    bool preallocateDxfMLineLinetypeXdataEntityHandles();
    struct DxfMLineConsumerSource {
        const DRW_MLine* live{nullptr};
        const dx_ifaceBlock* block{nullptr};
        DRW_MLine model,projection;
        bool geometryProjected{false};
        std::shared_ptr<const DRW_MLine> original;
        std::shared_ptr<const dx_data::TableDxfSourceCapture::EntitiesDelivery::Certificate> entities;
        std::shared_ptr<const dx_data::TableDxfSourceCapture::BlockOriginAudit> blocks;
        std::size_t dispatch{0};
    };
    std::unordered_map<std::uint32_t,DxfMLineConsumerSource> m_dxfMLineConsumers;
    std::uint8_t m_dxfMLineConsumerInspected{0};
    bool prepareDxfMLineConsumers(DRW::Version,bool) noexcept;
    bool hasUnchangedDxfMLineConsumers() const noexcept;
    std::uint32_t dxfMLineConsumerOwnerToWrite(const DxfMLineConsumerSource&) const;
    bool inspectDxfMLineConsumers(const dxfRW::DxfObjectsWriteView&,std::string_view) noexcept;
    std::vector<DRW_Dictionary> m_dxfMaterialWriteDictionaries;
    dx_materialImportReceipt m_dxfMaterialWriteReceipt;
    bool m_dxfMaterialWriteInspected{false};
    bool m_dxfMaterialWriteRequired{false};
    bool prepareDxfMaterialSources(DRW::Version version,bool binary);
    bool writeDxfMaterialSource(std::uint32_t handle);
    bool hasUnchangedDxfMaterialSources() const noexcept;
    bool inspectDxfMaterialOutput(const dxfRW::DxfObjectsWriteView& view) noexcept;
    struct DxfHeaderMaterialState {
        const dx_data* owner{nullptr};
        std::uint64_t generation{0};
        dx_source_format format{dx_source_format::Unknown};
        DRW::Version version{DRW::UNKNOWNV};
        std::uint32_t target{0}, targetOwner{0};
        bool present{false}, active{false}, inspected{false};
    } m_dxfHeaderMaterialState;
    bool prepareDxfHeaderMaterial(DRW::Version) noexcept;
    bool hasUnchangedDxfHeaderMaterial() const noexcept;
    bool inspectDxfHeaderMaterial(const dxfRW::DxfHeaderWriteView&) noexcept;
    bool finalizeDxfHeaderMaterial() const noexcept;
    struct DxfLayerMaterialState {
        struct Row {
            const DRW_Layer* layer{nullptr};
            std::uint32_t handle{0}, owner{0}, target{0}, emitted{0};
            std::string name;
            bool present{false}, named{false}, inspected{false};
        };
        struct Provider { std::uint32_t owner{0}; bool observed{false}; };
        std::vector<Row> rows;
        std::unordered_map<const DRW_Layer*,std::size_t> layers;
        std::unordered_map<std::uint32_t,Provider> providers;
        const dx_data* owner{nullptr};
        std::uint64_t generation{0};
        DRW::Version version{DRW::UNKNOWNV};
        std::size_t bytes{0}, byteLimit{64u*1024u*1024u}, recordLimit{100000u};
        bool active{false}, inspected{false};
        bool plotActive{false}, plotObjectsInspected{false};
        std::uint32_t plotTarget{0u}, plotOwner{0u};
        void clear() noexcept {
            decltype(rows){}.swap(rows); decltype(layers){}.swap(layers);
            decltype(providers){}.swap(providers);
            owner=nullptr; generation=0u; version=DRW::UNKNOWNV;
            bytes=0u; active=false; inspected=false; plotActive=false; plotObjectsInspected=false; plotTarget=plotOwner=0u;
        }
    } m_dxfLayerMaterialState;
    bool prepareDxfLayerMaterials(DRW::Version) noexcept;
    bool prepareDxfLayerPlot(DRW::Version, bool) noexcept;
    bool hasUnchangedDxfLayerPlot() const noexcept;
    bool inspectDxfLayerPlotObjects(const dxfRW::DxfObjectsWriteView&) noexcept;
    bool hasUnchangedDxfLayerMaterials() const noexcept;
    bool checkDxfLayerMaterial(const DRW_Layer&) const noexcept;
    bool inspectDxfLayerMaterials(const dxfRW::DxfTablesWriteView&) noexcept;
    bool finalizeDxfLayerMaterials() const noexcept;
    struct DxfVisualStyleWriteSource {
        DRW_VisualStyle model;
        DRW_RawDxfObject output;
    };
    std::map<std::uint32_t, DxfVisualStyleWriteSource> m_dxfVisualStyles;
    const dx_data* m_dxfVisualStyleOwner{nullptr};
    std::uint64_t m_dxfVisualStyleGeneration{0u};
    DRW::Version m_dxfVisualStyleOutputVersion{DRW::UNKNOWNV};
    bool m_dxfVisualStyleOutputBinary{false};
    bool m_dxfVisualStylesInspected{false};
    std::size_t m_dxfVisualStyleOutputBytes{0u};
    bool prepareDxfVisualStyles(DRW::Version version, bool binary) noexcept;
    bool hasUnchangedDxfVisualStyles() const noexcept;
    bool inspectDxfVisualStyles(const dxfRW::DxfObjectsWriteView&,
                                bool debug) noexcept;
    struct DxfRenderPresetWriteSource {
        DRW_RenderSettings live;
        UTF8STRING name;
        bool renamed{false};
        DRW_RawDxfObject output;
        std::shared_ptr<const DRW_RawDxfObject> original;
    };
    std::map<std::uint32_t, DxfRenderPresetWriteSource> m_dxfRenderPresetSources;
    const dx_data* m_dxfRenderPresetOwner{nullptr};
    std::uint64_t m_dxfRenderPresetGeneration{0};
    std::size_t m_dxfRenderPresetCount{0}, m_dxfRenderPresetBytes{0};
    std::size_t m_dxfRenderPresetByteLimit{64u * 1024u * 1024u};
    std::size_t m_dxfRenderPresetRecordLimit{100000u};
    bool m_dxfRenderPresetInspected{false};
    bool prepareDxfRenderPresetSources(DRW::Version version, bool binary) noexcept;
    bool hasUnchangedDxfRenderPresetSources() const noexcept;
    bool writeDxfRenderPreset(DRW_RawDxfObject& raw);
    bool inspectDxfRenderSettingsOutput(const dxfRW::DxfObjectsWriteView& view) noexcept;
    struct DxfCommonReferenceSource {
        enum Kind { Entity, Child, BlockBegin, BlockEnd, Raw } kind{Entity};
        const DRW_Entity* entity{nullptr};
        const DRW_RawDxfObject* raw{nullptr};
        const DRW_Entity* parent{nullptr};
        const dx_ifaceBlock* block{nullptr};
        std::uint32_t handle{0}, owner{0}, target{0}, targetOwner{0}, parentHandle{0};
        int space{0};
        std::string name, ownerName, parentName;
        bool blocks{false}, inspected{false};
        std::uint32_t emitted{0}, emittedOwner{0};
    };
    struct DxfCommonReferenceState {
        std::vector<DxfCommonReferenceSource> sources;
        std::unordered_map<const DRW_Entity*,std::size_t> entities;
        struct Provider {std::uint32_t owner{0};bool observed{false};};
        std::unordered_map<std::uint32_t,Provider> providers;
        std::size_t observedFields{0};
        const dx_data* owner{nullptr};std::uint64_t generation{0};
        dx_source_format sourceFormat{dx_source_format::Unknown};DRW::Version sourceVersion{DRW::UNKNOWNV};
        std::size_t bytes{0},byteLimit{64u*1024u*1024u},recordLimit{100000u};
        unsigned sections{0};
        void clear() noexcept {
            decltype(sources){}.swap(sources);decltype(entities){}.swap(entities);decltype(providers){}.swap(providers);
            observedFields=0u;owner=nullptr;generation=0u;bytes=0u;sections=0u;sourceFormat=dx_source_format::Unknown;sourceVersion=DRW::UNKNOWNV;
        }
    } m_dxfFullStyleState,m_dxfMaterialReferenceState;
    bool visitDxfCommonReferences(const dx_data&,int,const DxfCommonReferenceState&,
        const std::function<bool(DxfCommonReferenceSource&&)>&) const noexcept;
    bool prepareDxfCommonReferences(DRW::Version,int,DxfCommonReferenceState&) noexcept;
    bool hasUnchangedDxfCommonReferences(int,const DxfCommonReferenceState&) const noexcept;
    bool inspectDxfCommonReferences(const dxfRW::DxfObjectsWriteView&,std::string_view,int,DxfCommonReferenceState&) noexcept;
    bool finalizeDxfCommonReferences(int,const DxfCommonReferenceState&) const noexcept;
    bool checkDxfCommonEntity(const DRW_Entity*,int,const DxfCommonReferenceState&) const noexcept;
    bool visitDxfFullStyles(const dx_data& data,const std::function<bool(DxfCommonReferenceSource&&)>& visitor) const noexcept {
        return visitDxfCommonReferences(data,348,m_dxfFullStyleState,visitor);
    }
    bool prepareDxfFullStyles(DRW::Version version) noexcept {return prepareDxfCommonReferences(version,348,m_dxfFullStyleState);}
    bool hasUnchangedDxfFullStyles() const noexcept {return hasUnchangedDxfCommonReferences(348,m_dxfFullStyleState);}
    bool inspectDxfFullStyles(const dxfRW::DxfObjectsWriteView& view,std::string_view section) noexcept {
        return inspectDxfCommonReferences(view,section,348,m_dxfFullStyleState);
    }
    bool finalizeDxfFullStyles() const noexcept {return finalizeDxfCommonReferences(348,m_dxfFullStyleState);}
    bool checkDxfCommonEntity(const DRW_Entity* entity) const noexcept {
        return checkDxfCommonEntity(entity,348,m_dxfFullStyleState)&&checkDxfCommonEntity(entity,347,m_dxfMaterialReferenceState);
    }
    struct DxfShadeConsumerSource {
        enum Kind { PlotSettings, Layout, Viewport } kind{PlotSettings};
        std::uint32_t handle{0}, owner{0}, target{0}, visualStyle{0};
        std::uint32_t targetOwner{0}, visualStyleOwner{0};
        unsigned targetKind{0}; // 0 none, 1 VisualStyle, 2/3/4 render engines.
        int mode{0}, space{0};
        const dx_ifaceBlock* block{nullptr};
        std::string ownerName;
        bool blocks{false}, inspected{false};
        std::uint32_t emitted{0}, emittedOwner{0};
    };
    std::vector<DxfShadeConsumerSource> m_dxfShadeConsumers;
    const dx_data* m_dxfShadeConsumerOwner{nullptr};
    std::uint64_t m_dxfShadeConsumerGeneration{0};
    std::size_t m_dxfShadeConsumerBytes{0};
    std::size_t m_dxfShadeConsumerByteLimit{64u * 1024u * 1024u};
    std::size_t m_dxfShadeConsumerRecordLimit{100000u};
    unsigned m_dxfShadeConsumerSections{0};
    bool m_dxfShadeConsumerActive{false};
    bool m_dxfShadeConsumerEntities{false};
    bool visitDxfShadeConsumers(const std::function<bool(DxfShadeConsumerSource&&)>& visit) const;
    bool prepareDxfShadeConsumers(DRW::Version version) noexcept;
    bool hasUnchangedDxfShadeConsumers() const noexcept;
    bool inspectDxfShadeConsumers(const dxfRW::DxfObjectsWriteView& view,
                                 std::string_view section) noexcept;
    bool finalizeDxfShadeConsumers() const noexcept;

    struct DxfAssociativeGeometrySource {
        DRW_AssociativeObject source;
        std::uint32_t targetHandle{0u};
        std::string targetRecordName;
        std::string targetSection;
    };
    struct DxfAssociativeGeometryOutputProof {
        std::uint32_t sourceHandle{0u}, emittedHandle{0u}, emittedOwner{0u};
        std::uint32_t targetHandle{0u}, emittedTarget{0u};
        std::string targetRecordName, targetSection;
    };
    std::vector<DxfAssociativeGeometrySource>
        m_dxfAssociativeGeometrySources;
    std::vector<DxfAssociativeGeometryOutputProof>
        m_dxfAssociativeGeometryOutputProofs;
    const dx_data* m_dxfAssociativeGeometrySourceOwner{nullptr};
    std::uint64_t m_dxfAssociativeGeometrySourceGeneration{0u};
    std::size_t m_dxfAssociativeGeometrySourceBytes{0u};
    bool m_dxfAssociativeGeometryOutputInspected{false};
    bool prepareDxfAssociativeGeometrySources() noexcept;
    bool hasUnchangedDxfAssociativeGeometrySources() const noexcept;
    bool inspectDxfAssociativeGeometryOutput(
        const dxfRW::DxfObjectsWriteView& view) noexcept;
    bool finalizeDxfAssociativeGeometryOutput() const noexcept;

    struct DxfAssociativeValueSource {
        DRW_AssociativeObject source;
        std::string ownerRecordName;
        std::uint32_t ownerParentHandle{0u};
        std::uint32_t ownerNetworkHandle{0u};
        std::string targetRecordName;
        std::uint32_t targetParentHandle{0u};
        std::uint32_t targetNetworkHandle{0u};
    };
    struct DxfAssociativeValueOutputProof {
        std::uint32_t sourceHandle{0u}, emittedHandle{0u};
        std::uint32_t ownerHandle{0u}, emittedOwner{0u};
        std::uint32_t ownerParent{0u}, emittedOwnerParent{0u};
        std::string ownerRecordName;
        std::uint32_t targetHandle{0u}, emittedTarget{0u};
        std::uint32_t targetParent{0u}, emittedTargetParent{0u};
        std::uint32_t readDependencyHandle{0u}, emittedReadDependency{0u};
        std::uint32_t readDependencyOwner{0u}, emittedReadDependencyOwner{0u};
        std::uint32_t nodeDependencyHandle{0u}, emittedNodeDependency{0u};
        std::uint32_t nodeDependencyOwner{0u}, emittedNodeDependencyOwner{0u};
    };
    std::vector<DxfAssociativeValueSource>
        m_dxfAssociativeValueSources;
    std::vector<DxfAssociativeValueOutputProof>
        m_dxfAssociativeValueOutputProofs;
    const dx_data* m_dxfAssociativeValueSourceOwner{nullptr};
    std::uint64_t m_dxfAssociativeValueSourceGeneration{0u};
    std::size_t m_dxfAssociativeValueSourceBytes{0u};
    bool m_dxfAssociativeValueOutputInspected{false};
    bool prepareDxfAssociativeValueSources() noexcept;
    bool hasUnchangedDxfAssociativeValueSources() const noexcept;
    bool inspectDxfAssociativeValueOutput(
        const dxfRW::DxfObjectsWriteView& view) noexcept;
    bool finalizeDxfAssociativeValueOutput() const noexcept;

    struct DwgAssociativeVariableSource {
        DRW_AssociativeObject model;
    };
    struct DwgAssociativeVariableNetworkSource {
        DRW_AssociativeObject model;
    };
    struct DwgAssociativeVariableOutputProof {
        std::uint32_t sourceHandle{0u}, emittedHandle{0u};
        std::uint32_t sourceNetwork{0u}, emittedNetwork{0u};
        std::uint32_t sourceNetworkDictionary{0u};
        std::uint32_t emittedNetworkDictionary{0u};
        std::uint32_t sourceDictionaryOwner{0u};
        std::uint32_t emittedDictionaryOwner{0u};
    };
    struct DwgAssociativeVariableGraphOutputProof {
        std::uint32_t sourceHandle{0u}, emittedHandle{0u};
        std::uint32_t sourceOwner{0u}, emittedOwner{0u};
        bool dictionary{false};
    };
    std::map<std::uint32_t, DwgAssociativeVariableSource>
        m_dwgAssociativeVariableSources;
    std::map<std::uint32_t, DwgAssociativeVariableNetworkSource>
        m_dwgAssociativeVariableNetworks;
    std::map<std::uint32_t, DRW_Dictionary>
        m_dwgAssociativeVariableDictionaries;
    std::vector<DwgAssociativeVariableOutputProof>
        m_dwgAssociativeVariableOutputProofs;
    std::vector<DwgAssociativeVariableGraphOutputProof>
        m_dwgAssociativeVariableGraphOutputProofs;
    const dx_data* m_dwgAssociativeVariableSourceOwner{nullptr};
    std::uint64_t m_dwgAssociativeVariableSourceGeneration{0u};
    std::size_t m_dwgAssociativeVariableSourceBytes{0u};
    DRW::Version m_dwgAssociativeVariableOutputVersion{DRW::UNKNOWNV};
    bool m_dwgAssociativeVariableOutputBinary{false};
    bool m_dwgAssociativeVariablesInspected{false};
    bool prepareDwgAssociativeVariables(DRW::Version version, bool binary,
                                        bool debug) noexcept;
    bool hasUnchangedDwgAssociativeVariables() const noexcept;
    bool inspectDwgAssociativeVariables(
        const dxfRW::DxfObjectsWriteView& view, bool debug) noexcept;
    bool finalizeDwgAssociativeVariables() const noexcept;

    struct DwgAssociativeDimensionDependencySource {
        DRW_AssociativeObject dependency;
        DRW_AssociativeObject body;
        DRW_AssociativeObject constraintGroup;
        std::uint32_t constraintGroupHandle{0u};
        std::uint32_t constraintGroupOwnerHandle{0u};
        std::string constraintGroupRecordName;
        std::uint16_t constraintGroupClassVersion{0u};
        std::int32_t constraintGroupVersion{0};
        std::vector<std::uint32_t> constraintGroupActions;
        std::vector<DRW_AssociativePrefixStatus>
            constraintGroupPrefixStatuses;
        std::uint32_t dimensionHandle{0u};
        std::string dimensionSection;
    };
    struct DwgAssociativeDimensionDependencyOutputProof {
        std::uint32_t sourceDependency{0u}, emittedDependency{0u};
        std::uint32_t sourceBody{0u}, emittedBody{0u};
        std::uint32_t sourceGroup{0u}, emittedGroup{0u};
        std::uint32_t sourceDimension{0u}, emittedDimension{0u};
    };
    std::map<std::uint32_t, DwgAssociativeDimensionDependencySource>
        m_dwgAssociativeDimensionDependencies;
    std::vector<DwgAssociativeDimensionDependencyOutputProof>
        m_dwgAssociativeDimensionDependencyOutputProofs;
    const dx_data* m_dwgAssociativeDimensionDependencySourceOwner{nullptr};
    std::uint64_t m_dwgAssociativeDimensionDependencySourceGeneration{0u};
    std::size_t m_dwgAssociativeDimensionDependencySourceBytes{0u};
    DRW::Version m_dwgAssociativeDimensionDependencyOutputVersion{DRW::UNKNOWNV};
    bool m_dwgAssociativeDimensionDependencyOutputBinary{false};
    bool m_dwgAssociativeDimensionDependenciesInspected{false};
    bool prepareDwgAssociativeDimensionDependencies(
        DRW::Version version, bool binary, bool debug) noexcept;
    bool hasUnchangedDwgAssociativeDimensionDependencies() const noexcept;
    bool inspectDwgAssociativeDimensionDependencies(
        const dxfRW::DxfObjectsWriteView& view, bool debug) noexcept;
    bool finalizeDwgAssociativeDimensionDependencies() const noexcept;

    struct ExtensionDictionaryOwnerSource {
        enum class Kind : std::uint8_t {
            Entity, Layer, Layout, LayerTableControl
        };
        Kind kind{Kind::Entity};
        std::uint32_t handle{0u};
        std::uint32_t parentHandle{0u};
        std::uint32_t dictionaryHandle{0u};
    };
    std::vector<ExtensionDictionaryOwnerSource>
        m_extensionDictionaryOwnerSources;
    const dx_data* m_extensionDictionaryOwnerSourceOwner{nullptr};
    std::uint64_t m_extensionDictionaryOwnerSourceGeneration{0u};
    bool hasUnchangedExtensionDictionaryOwnerSources() const noexcept;
    bool hasUnchangedDynamicBlockOutputProfile() const noexcept;

    std::vector<std::uint32_t> m_extensionDictionaryHandles;
    std::vector<std::uint32_t> m_extensionGeoDataHandles;
    const dx_data* m_dwgGeoDataSourceOwner{nullptr};
    std::uint64_t m_dwgGeoDataSourceGeneration{0u};
    struct DwgGeoDataGraphBinding {
        std::uint32_t object{0u}, dictionary{0u}, host{0u};
    };
    std::vector<DwgGeoDataGraphBinding> m_dwgGeoDataGraphs;
    bool m_dwgGeoDataTablesInspected{false};
    bool m_dwgGeoDataObjectsInspected{false};
    bool isClosedDwgGeoDataGraph(const DRW_GeoData& geo) const noexcept;
    bool hasUnchangedDwgGeoDataGraphs() const noexcept;
    bool inspectDwgGeoDataTables(const dxfRW::DxfTablesWriteView& view) noexcept;
    bool inspectDwgGeoDataObjects(const dxfRW::DxfObjectsWriteView& view) noexcept;
    std::vector<std::uint32_t> m_extensionRasterVariablesHandles;
    std::vector<std::uint32_t> m_extensionDbColorHandles;
    std::vector<std::uint32_t> m_extensionWipeoutVariablesHandles;
    std::vector<std::uint32_t> m_extensionSunStudyHandles;
    bool m_sunStudyOutputInspected{false};
    std::unique_ptr<DRW_Dictionary> m_sunStudyRootSnapshot;
    std::unique_ptr<DRW_Dictionary> m_sunStudyOwnerSnapshot;
    std::uint32_t m_pointCloudExDictionaryHandle{0u};
    std::vector<std::uint32_t> m_pointCloudExDefinitionHandles;
    std::vector<std::uint32_t> m_pointCloudExReactorHandles;
    std::uint32_t m_pointCloudClassicDictionaryHandle{0u};
    std::string m_pointCloudClassicRootKey;
    std::vector<std::uint32_t> m_pointCloudClassicDefinitionHandles;
    std::vector<std::uint32_t> m_pointCloudClassicReactorHandles;
    std::vector<dx_data::UnderlayDefinitionKey>
        m_underlayDefinitionKeysToWrite;
    std::vector<std::uint32_t> m_extensionAssociativeHandles;
    const dx_data* m_selectedDwgAssociativeSource{nullptr};
    std::uint64_t m_selectedDwgAssociativeGeneration{0u};
    std::uint32_t m_selectedDwgAssociativeRootHandle{0u};
    DRW_AssociativeObject m_selectedDwgAssociativeRootSource;
    DRW_AssociativeObject m_selectedDwgAssociativeRootOutput;
    std::unordered_map<std::uint32_t, DRW_AssociativeObject>
        m_selectedDwgAssociativeChildSources;
    std::unordered_map<std::uint32_t, std::uint32_t>
        m_selectedDwgAssociativeChildDictionaries;
    std::unordered_map<std::uint32_t, DRW_Dictionary>
        m_selectedDwgAssociativeDictionarySources;
    std::unordered_map<std::uint32_t, DRW_Dictionary>
        m_selectedDwgAssociativeDictionaryOutputs;
    std::unordered_map<std::uint32_t, std::uint32_t>
        m_selectedDwgAssociativeExcludedDictionaryEdges;
    std::unordered_set<std::uint32_t>
        m_selectedDwgAssociativeExcludedHandles;
    bool hasUnchangedSelectedDwgAssociativeNetworks() const noexcept;
    std::vector<std::uint32_t> m_extensionBlockRepresentationDataHandles;
    std::vector<DRW_BlockRepresentationData>
        m_extensionBlockRepresentationDataSources;
    bool m_extensionBlockRepresentationDataOutputInspected{false};
    bool m_extensionBlockRepresentationDataOutputDebug{false};
    std::vector<std::uint32_t> m_extensionDictionaryWithDefaultHandles;
    std::vector<std::uint32_t> m_extensionPlaceholderHandles;
    std::vector<std::uint32_t> m_extensionXRecordHandles;
    std::vector<std::uint32_t> m_extensionDictionaryVarHandles;
    std::vector<std::uint32_t> m_extensionScaleHandles;
    std::vector<std::uint32_t> m_extensionMLineStyleHandles;
    std::vector<std::uint32_t> m_extensionMLeaderStyleHandles;
    std::unordered_map<std::uint32_t, std::string>
        m_mlineStyleNamesToWrite;
    std::vector<std::uint32_t> m_extensionVisualStyleHandles;
    std::vector<std::uint32_t> m_extensionMaterialHandles;
    std::vector<DRW_Material> m_extensionMaterialSources;
    std::vector<std::uint32_t> m_extensionMaterialDictionaryHandles;
    std::vector<DRW_Dictionary> m_extensionMaterialDictionaries;
    const dx_data* m_extensionMaterialSourceOwner{nullptr};
    DRW::Version m_extensionMaterialSourceVersion{DRW::UNKNOWNV};
    std::uint64_t m_extensionMaterialSourceGeneration{0u};
    std::size_t m_extensionMaterialSourceBytes{0u};
    bool hasUnchangedExtensionMaterialSources() const noexcept;
    std::vector<std::uint32_t> m_extensionTableStyleHandles;
    std::vector<std::uint32_t> m_extensionCellStyleMapHandles;
    std::vector<std::pair<std::uint32_t, std::int32_t>>
        m_extensionTableStyleBaseIds;
    std::vector<std::uint32_t> m_extensionDetailViewStyleHandles;
    std::vector<std::uint32_t> m_extensionSectionViewStyleHandles;
    std::vector<std::uint32_t> m_extensionPlotSettingsHandles;
    std::vector<std::uint32_t> m_extensionRenderSettingsHandles;
    std::vector<std::uint32_t> m_spatialFilterHandlesToWrite;
    std::vector<std::uint32_t> m_backgroundHandlesToWrite;
    std::vector<std::uint32_t> m_sunHandlesToWrite;
    std::vector<std::uint32_t> m_liveSectionHandlesToWrite;
    std::vector<std::uint32_t> m_sortDictionaryHandles;
    std::vector<std::uint32_t> m_sortOwnerHandles;
    std::unordered_map<std::uint32_t, DRW_Dictionary>
        m_sortDictionaryOutputs;
    std::vector<DRW_SortEntsTable> m_sortEmissionTables;
    struct DwgSortEntsOutputSource {
        DRW_SortEntsTable table;
        DRW_SortEntsTable emission;
        DRW_DwgSortEntsMembership membership;
        DRW_Dictionary ownerDictionary;
        DRW_Block_Record ownerBlock;
        int objectType{0};
        std::string recordName;
        std::string className;
    };
    std::map<std::uint32_t, DwgSortEntsOutputSource>
        m_dwgSortEntsOutputSources;
    struct DwgOmittedSortEntsOutputSource {
        DRW_SortEntsTable table;
        DRW_DwgSortEntsMembership membership;
        DRW_Dictionary ownerDictionary;
        DRW_Dictionary projectedOwnerDictionary;
        DRW_Block_Record ownerBlock;
        std::uint32_t emittedTable{0u};
        std::uint32_t emittedDictionary{0u};
        std::uint32_t emittedBlock{0u};
        int objectType{0};
        std::string recordName;
        std::string className;
    };
    std::map<std::uint32_t, DwgOmittedSortEntsOutputSource>
        m_dwgOmittedSortEntsOutputSources;
    const dx_data* m_dwgSortEntsOutputSourceOwner{nullptr};
    std::uint64_t m_dwgSortEntsOutputGeneration{0u};
    bool m_dwgSortEntsOutputInspected{false};
    bool hasUnchangedDwgSortEntsOutputSources() const noexcept;
    bool inspectDwgSortEntsOutput(
        const dxfRW::DxfObjectsWriteView& view, bool debug) noexcept;
    struct DwgDynamicBlockOutputSource {
        std::string recordName;
        std::uint32_t ownerHandle{0u};
        std::vector<DRW_Variant> groups;
    };
    struct DwgDynamicBlockEntityOutputSource {
        std::uint32_t handle{0u};
        std::uint32_t ownerBlockHandle{0u};
        std::string recordName;
    };
    struct DwgDynamicBlockOwnerOutputSource {
        std::uint32_t blockHandle{0u};
        std::uint32_t graphHandle{0u};
        std::uint32_t sortEntsHandle{0u};
        std::uint32_t purgeHandle{0u};
        std::uint32_t nestedNetworkDictionaryHandle{0u};
        std::string blockName;
    };
    struct DwgDynamicAssociativeDependencySource {
        DRW_AssociativeObject dependency;
        DRW_AssociativeObject constraintGroup;
        std::uint32_t dependencyBodyHandle{0u};
        std::uint32_t parameterHandle{0u};
    };
    struct DwgDynamicAssociativeOutputProof {
        std::uint32_t sourceDependency{0u}, emittedDependency{0u};
        std::uint32_t sourceGroup{0u}, emittedGroup{0u};
        std::uint32_t sourceBody{0u}, emittedBody{0u};
        std::uint32_t sourceParameter{0u}, emittedParameter{0u};
    };
    struct DxfDynamicBlockReplaySource {
        bool hasTypedModel{false};
        bool hasBlockRepresentationModel{false};
        DRW_DynamicBlockObject model;
        DRW_RawDxfObject raw;
    };
    struct DxfAssociativeReplaySource {
        DRW_AssociativeObject model;
        DRW_RawDxfObject raw;
    };
    std::vector<DxfDynamicBlockReplaySource>
        m_dxfDynamicBlockReplaySources;
    std::vector<DxfAssociativeReplaySource>
        m_dxfAssociativeReplaySources;
    const dx_data* m_dxfDynamicReplayOwner{nullptr};
    std::uint64_t m_dxfDynamicReplayGeneration{0u};
    DRW::Version m_dxfDynamicReplayVersion{DRW::UNKNOWNV};
    bool m_dxfDynamicReplayPrepared{false};
    bool m_dxfDynamicReplayInspected{false};
    const dx_data* m_dxfAssociativeReplayOwner{nullptr};
    std::uint64_t m_dxfAssociativeReplayGeneration{0u};
    DRW::Version m_dxfAssociativeReplayVersion{DRW::UNKNOWNV};
    std::size_t m_dxfAssociativeReplayPreparedRecords{0u};
    bool m_dxfAssociativeReplayPrepared{false};
    bool m_dxfAssociativeReplayInspected{false};
    bool prepareDxfDynamicBlockReplaySources(DRW::Version version,
                                             bool debug) noexcept;
    bool hasUnchangedDxfDynamicBlockReplaySources() const noexcept;
    bool prepareDxfAssociativeReplaySources(DRW::Version version,
                                             bool debug) noexcept;
    bool hasUnchangedDxfAssociativeReplaySources() const noexcept;
    bool inspectDxfDynamicBlockReplay(
        const dxfRW::DxfObjectsWriteView& view, bool debug) noexcept;
    bool inspectDxfAssociativeReplay(
        const dxfRW::DxfObjectsWriteView& view, bool debug) noexcept;
    std::map<std::uint32_t, std::uint32_t>
        m_dwgDynamicBlockOutputDictionaries;
    std::map<std::uint32_t, std::string> m_dwgDynamicBlockOutputBlocks;
    std::map<std::uint32_t, DwgDynamicBlockOwnerOutputSource>
        m_dwgDynamicBlockOutputOwners;
    std::map<std::uint32_t, DwgDynamicBlockOutputSource>
        m_dwgDynamicBlockOutputSources;
    std::map<std::uint32_t, DwgDynamicAssociativeDependencySource>
        m_dwgDynamicAssociativeDependencies;
    std::map<std::uint32_t, DRW_AssociativeObject>
        m_dwgDynamicAssociativeGroups;
    std::vector<DwgDynamicAssociativeOutputProof>
        m_dwgDynamicAssociativeOutputProofs;
    std::map<std::uint32_t, DRW_UnsupportedObject>
        m_dwgDynamicBlockSourceFrameSnapshots;
    std::vector<DwgDynamicBlockEntityOutputSource>
        m_dwgDynamicBlockEntityOutputSources;
    const dx_data* m_dwgDynamicBlockOutputSourceOwner{nullptr};
    std::uint64_t m_dwgDynamicBlockOutputGeneration{0u};
    std::uint32_t m_dwgDynamicBlockOutputDictionary{0u};
    std::uint32_t m_dwgDynamicBlockOutputBlock{0u};
    bool m_dwgDynamicBlockOutputPrepared{false};
    bool m_dwgDynamicBlockOutputInspected{false};
    bool prepareDwgDynamicBlockOutputSources(bool debug) noexcept;
    bool prepareDwgDynamicBlockOutputGraphSet(bool debug) noexcept;
    bool hasUnchangedDwgDynamicBlockOutputSources() const noexcept;
    bool hasUnchangedDwgDynamicBlockOutputGraphSet() const noexcept;
    bool inspectDwgDynamicBlockOutput(
        const dxfRW::DxfObjectsWriteView& view, bool debug) noexcept;
    bool inspectDwgDynamicBlockOutputGraphSet(
        const dxfRW::DxfObjectsWriteView& view, bool debug) noexcept;
    bool hasUnchangedDwgDynamicAssociativeSources() const noexcept;
    bool inspectDwgDynamicAssociativeOutput(
        const dxfRW::DxfObjectsWriteView& view, bool debug) noexcept;
    bool finalizeDwgDynamicBlockOutput() const noexcept;
    bool finalizeDwgDynamicBlockOutputGraphSet() const noexcept;
    bool inspectDwgBlockRepresentationDataOutput(
        const dxfRW::DxfObjectsWriteView& view, bool debug) noexcept;
    bool finalizeDwgBlockRepresentationDataOutput() const noexcept;
    std::unordered_map<std::uint32_t, DRW_Table*> m_tableFallbacks;
    std::unordered_set<std::uint32_t> m_emittedTableHandles;
    std::vector<std::uint32_t> m_tableStylesToEmit;
    std::vector<std::uint32_t> m_tableStyleDictionaryHandles;
    std::map<std::uint32_t, DRW_TableStyle>
        m_dwgTableStyleOutputSources;
    std::map<std::uint32_t, DRW_Dictionary>
        m_dwgTableStyleDictionarySources;
    std::map<std::uint32_t, DRW_Dictionary>
        m_dwgTableStyleCellMapDictionarySources;
    std::map<std::uint32_t, DRW_CellStyleMap>
        m_dwgTableStyleCellMapSources;
    DRW_Dictionary m_dwgTableStyleNamedObjectDictionarySource;
    bool m_dwgTableStyleRootSourceCaptured{false};
    const dx_data* m_dwgTableStyleOutputSourceOwner{nullptr};
    DRW::Version m_dwgTableStyleOutputSourceVersion{DRW::UNKNOWNV};
    std::uint64_t m_dwgTableStyleOutputSourceGeneration{0u};
    bool m_dwgTableStylesInspected{false};
    bool hasUnchangedDwgTableStyleOutputSources() const noexcept;
    bool inspectDwgTableStylesOutput(
        const dxfRW::DxfObjectsWriteView& view) noexcept;
    std::map<std::uint32_t, DRW_DetailViewStyle>
        m_dwgDetailViewStyleOutputSources;
    std::map<std::uint32_t, DRW_SectionViewStyle>
        m_dwgSectionViewStyleOutputSources;
    std::map<std::uint32_t, DRW_Dictionary>
        m_dwgModelDocViewStyleDictionarySources;
    std::map<std::uint32_t, DRW_XRecord>
        m_dwgModelDocViewStyleXRecordSources;
    DRW_Dictionary m_dwgModelDocViewStyleNamedObjectDictionarySource;
    const dx_data* m_dwgModelDocViewStyleOutputOwner{nullptr};
    std::uint64_t m_dwgModelDocViewStyleOutputGeneration{0u};
    bool m_dwgModelDocViewStyleRootCaptured{false};
    bool m_dwgModelDocViewStylesInspected{false};
    bool hasUnchangedDwgModelDocViewStyleOutputSources() const noexcept;
    bool inspectDwgModelDocViewStylesOutput(
        const dxfRW::DxfObjectsWriteView& view) noexcept;
    struct DwgDictionaryDefaultOutputSource {
        DRW_DictionaryWithDefault dictionary;
        DRW_AcDbPlaceholder placeholder;
        DRW_Dictionary namedObjectDictionary;
        int objectType{0};
        std::string recordName;
        std::string className;
        int placeholderObjectType{0};
        std::string placeholderRecordName;
        std::string placeholderClassName;
    };
    std::map<std::uint32_t, DwgDictionaryDefaultOutputSource>
        m_dwgDictionaryDefaultOutputSources;
    const dx_data* m_dwgDictionaryDefaultOutputSourceOwner{nullptr};
    DRW::Version m_dwgDictionaryDefaultOutputSourceVersion{DRW::UNKNOWNV};
    std::uint64_t m_dwgDictionaryDefaultOutputGeneration{0u};
    bool m_ac1027DefaultLayerInspected = false;
    bool m_dwgDictionaryDefaultOutputInspected{false};
    bool hasUnchangedDwgDictionaryDefaultOutputSources() const noexcept;
    bool inspectDwgDictionaryDefaultOutput(
        const dxfRW::DxfObjectsWriteView& view) noexcept;
    // Output-only DXF group 68/69 values for DWG-origin paper viewports.
    // Keys are borrowed source records and are cleared at fileExport exit.
    std::unordered_map<const DRW_Viewport*, std::pair<int, int>>
        m_dwgViewportProjections;
    struct DwgViewportXrecordOutputProof {
        std::uint32_t emitted{0u};
        std::uint32_t sourceViewport{0u};
        std::uint32_t sourceExtensionDictionary{0u};
        std::uint32_t emittedExtensionDictionary{0u};
    };
    std::map<std::uint32_t, DwgViewportXrecordOutputProof>
        m_dwgViewportXrecordOutputProofs;
    bool m_dwgViewportXrecordsInspected{false};
    ExtensionGraphLimits m_extensionGraphLimits;
    ExtensionGraphStats m_extensionGraphStats;
    ExtensionGraphFailure m_extensionGraphFailure{ExtensionGraphFailure::None};
    std::string m_extensionGraphFailureDetail;
    ExtensionGraphLimits m_hatchGraphLimits;
    ExtensionGraphStats m_hatchGraphStats;
    ExtensionGraphFailure m_hatchGraphFailure{ExtensionGraphFailure::None};
    std::vector<std::uint32_t> m_hatchSourceHandles;
    bool m_hatchPlanningFailed{false};
    struct HatchPublication {
        DRW::ETYPE type{DRW::UNKNOWN};
        std::string owner;
        std::uint32_t emitted{0};
        bool published{false};
    };
    std::unordered_map<std::uint32_t, HatchPublication> m_hatchTargets;
    // DWG proxy group-91 class numbers are source identifiers; DXF group 91
    // is an ordinal in the emitted CLASSES row list. Built per export only
    // after the matching source class metadata has been validated.
    std::unordered_map<std::uint32_t, std::uint32_t>
        m_dwgProxyEntityClassIdToOutput;
    struct DwgProxyEntityTarget {
        const DRW_Entity* entity{nullptr};
        const dx_ifaceBlock* block{nullptr};
        std::uint32_t sourceHandle{0u};
        std::uint32_t sourceOwnerHandle{DRW::NoHandle};
        std::uint32_t blockSourceHandle{DRW::NoHandle};
        std::uint32_t emittedHandle{0u};
        DRW::ETYPE type{DRW::UNKNOWN};
        std::string recordName;
        std::string sectionName;
        std::uint32_t ownerHandle{0u};
        std::size_t occurrences{0u};
    };
    struct DwgProxyEntityReference {
        std::uint8_t handleCode{0u};
        int dxfCode{0};
        std::uint32_t sourceHandle{0u};
        std::uint64_t rawHandle{0u};
    };
    struct DwgProxyEntityOutput {
        const DRW_ProxyEntity* entity{nullptr};
        const dx_ifaceBlock* block{nullptr};
        std::uint32_t sourceHandle{0u};
        std::uint32_t sourceOwnerHandle{DRW::NoHandle};
        std::uint32_t blockSourceHandle{DRW::NoHandle};
        std::uint32_t sourceClassId{0u};
        std::uint32_t outputClassId{0u};
        std::string sectionName;
        std::uint32_t ownerHandle{0u};
        std::vector<DwgProxyEntityReference> references;
    };
    std::unordered_map<std::uint32_t, DwgProxyEntityTarget>
        m_dwgProxyEntityTargets;
    std::unordered_map<const DRW_ProxyEntity*, DwgProxyEntityOutput>
        m_dwgProxyEntityOutputs;
    bool m_dwgProxyEntityGraphPlanned{false};
    std::unordered_map<std::uint32_t, std::string> m_hatchOwnerAliases;
    std::unordered_map<std::string, bool> m_hatchOwnerBlocks;
    std::string m_hatchOwnerContext;
    bool m_preserveGraphSourceIdentities{false};

    static bool isFixedSpaceBlock(const dx_ifaceBlock* block) {
        if (block == nullptr)
            return false;
        // DWG producers also use the uppercase reserved spellings. Match
        // only these ASCII names, without locale-dependent UTF-8 casing.
        const auto equal = [block](std::string_view expected) noexcept {
            if (block->name.size() != expected.size()) return false;
            for (std::size_t i = 0u; i < expected.size(); ++i) {
                char ch = block->name[i];
                if (ch >= 'a' && ch <= 'z') ch = static_cast<char>(ch - 'a' + 'A');
                if (ch != expected[i]) return false;
            }
            return true;
        };
        return equal("*MODEL_SPACE") || equal("*PAPER_SPACE");
    }
};

#endif // DX_IFACE_H
