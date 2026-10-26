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

#include <cstdint>
#include <cstddef>
#include <array>
#include <iosfwd>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

class DxIfaceExtensionGraphTestAccess;
class DxIfaceTableSourceTestAccess;

class dx_iface : public DRW_Interface, public DRW_UnmodeledDxfObjectHandler,
                 public DRW_DwgClassSourceHandler, public DRW_DxfTableSourceHandler,
                 public DRW_DxfSymbolSourceHandler, public DRW_DxfRootDictionarySourceHandler,
                 public DRW_DxfBlockRecordSourceHandler, public DRW_DxfAttdefSourceHandler,
                 public DRW_DxfBlockSourceHandler, public DRW_DxfBlockChildrenSourceHandler,
                 public DRW_DxfBlockDeliveryHandler {
public:
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
        ResourceProofMissing, ResourceRawTypedMismatch, WrongSymbolKind
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
        // Rich-child direct roles/namespace, with exact selected map source
        // proof. Other resources' raw fields, full transitive/live and output
        // qualification remain separate gates.
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
    static constexpr std::size_t TypedEntityCaptureMaxRecords = 100000u;
    static constexpr std::size_t TypedEntityCaptureMaxRetainedBytes =
        64u * 1024u * 1024u;
    static constexpr std::size_t TypedEntityListNodeBytes =
        sizeof(DRW_Entity*) + 2u * sizeof(void*);
    static constexpr std::size_t ImageDefinitionCaptureMaxRecords = 100000u;
    static constexpr std::size_t ImageDefinitionCaptureMaxRetainedBytes =
        64u * 1024u * 1024u;
    static constexpr std::size_t RawDxfSectionCaptureMaxRecords = 100000u;
    static constexpr std::size_t RawDxfSectionCaptureMaxGroups = 4000000u;
    static constexpr std::size_t RawDxfSectionCaptureMaxRetainedBytes =
        64u * 1024u * 1024u;
    static constexpr std::size_t TypedBlockCaptureMaxRetainedBytes =
        64u * 1024u * 1024u;
    static constexpr std::size_t TypedBlockListNodeBytes =
        sizeof(dx_ifaceBlock*) + 2u * sizeof(void*);
    static constexpr std::size_t PlotSettingsCaptureMaxRecords = 100000u;
    static constexpr std::size_t PlotSettingsCaptureMaxRetainedBytes =
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
    // A null raw carrier is accepted only for same-version, typed-only DWG
    // input; the helper constructs a bounded DXF shell and refuses common
    // object metadata that has no typed serialization path.
    bool writeDetailViewStyleRaw(DRW_RawDxfObject* raw,
                                 const DRW_DetailViewStyle* typed);
    bool writeSectionViewStyleRaw(DRW_RawDxfObject* raw,
                                  const DRW_SectionViewStyle* typed);
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
    void addHeader(const DRW_Header* data) override{
        cData->headerC = *data;
        if (cData->dxfReadOutcome.attempted)
            cData->dxfReadOutcome.headerDelivered = true;
    }
    void addDxfClass(const DRW_Class& data) override;
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
    void addDictionaryVar(const DRW_DictionaryVar& data) override;
    void addNamedObjectDictionary(const DRW_Dictionary& data) override;
    void addXRecord(const DRW_XRecord& data) override;
    void addDataLink(const DRW_DataLink& data) override;
    void addMaterial(const DRW_Material& data) override;
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
    void addUnsupportedObject(const DRW_UnsupportedObject& data) override;
    void addImageDefinitionReactor(const DRW_ImageDefinitionReactor& data) override;

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
        retainTypedEntity<DRW_Mesh>(data);
    }
    virtual void addSpline(const DRW_Spline* data) override{
        if (data != nullptr)
            retainTypedEntity<DRW_Spline>(*data);
    }
    virtual void addHelix(const DRW_Helix* data) override{
        if (data != nullptr)
            retainTypedEntity<DRW_Helix>(*data);
    }
    // ¿para que se usa?
    virtual void addKnot(const DRW_Entity& data) override{(void)data;}

    virtual void addInsert(const DRW_Insert& data) override{
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
            ++cData->typedTableRetainedRecords;
            cData->typedEntityRetainedBytes += estimatedBytes;
            cData->typedTableRetainedBytes += estimatedBytes;
        } catch (...) {
            cData->typedEntityCaptureFailed = true;
            return;
        }
        // Keep the pre-existing 2D consumer path unchanged while retaining
        // cells, merged-range metadata and style links in the typed snapshot.
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
        retainTypedEntity<DRW_MText>(data);
    }
    void addTolerance(const DRW_Tolerance& data) override{
        retainTypedEntity<DRW_Tolerance>(data);
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
            retainTypedEntity<DRW_DimAligned>(
                static_cast<const DRW_Dimension&>(*data));
    }
    virtual void addDimLinear(const DRW_DimLinear *data) override{
        if (data != nullptr)
            retainTypedEntity<DRW_DimLinear>(
                static_cast<const DRW_Dimension&>(*data));
    }
    virtual void addDimRadial(const DRW_DimRadial *data) override{
        if (data == nullptr)
            return;
        if (const auto* large = dynamic_cast<const DRW_DimLargeRadial*>(data))
            retainTypedEntity<DRW_DimLargeRadial>(*large);
        else
            retainTypedEntity<DRW_DimRadial>(
                static_cast<const DRW_Dimension&>(*data));
    }
    virtual void addDimDiametric(const DRW_DimDiametric *data) override{
        if (data != nullptr)
            retainTypedEntity<DRW_DimDiametric>(
                static_cast<const DRW_Dimension&>(*data));
    }
    virtual void addDimAngular(const DRW_DimAngular *data) override{
        if (data != nullptr)
            retainTypedEntity<DRW_DimAngular>(
                static_cast<const DRW_Dimension&>(*data));
    }
    virtual void addDimAngular3P(const DRW_DimAngular3p *data) override{
        if (data != nullptr)
            retainTypedEntity<DRW_DimAngular3p>(
                static_cast<const DRW_Dimension&>(*data));
    }
    virtual void addDimOrdinate(const DRW_DimOrdinate *data) override{
        if (data != nullptr)
            retainTypedEntity<DRW_DimOrdinate>(
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

    void addWipeout(const DRW_Wipeout *data) override {
        // WIPEOUT owns a masking boundary, not a raster file/IMAGEDEF link.
        if (data == nullptr)
            return;
        retainTypedEntity<DRW_Wipeout>(*data);
    }

    virtual void addSurface(const DRW_Surface *data) override{
        if (data == nullptr)
            return;
        switch (data->eType) {
        case DRW::PLANESURFACE:
            retainTypedEntity<DRW_PlaneSurface>(
                *static_cast<const DRW_PlaneSurface*>(data));
            break;
        case DRW::EXTRUDEDSURFACE:
            retainTypedEntity<DRW_ExtrudedSurface>(
                *static_cast<const DRW_ExtrudedSurface*>(data));
            break;
        case DRW::REVOLVEDSURFACE:
            retainTypedEntity<DRW_RevolvedSurface>(
                *static_cast<const DRW_RevolvedSurface*>(data));
            break;
        case DRW::SWEPTSURFACE:
            retainTypedEntity<DRW_SweptSurface>(
                *static_cast<const DRW_SweptSurface*>(data));
            break;
        case DRW::LOFTEDSURFACE:
            retainTypedEntity<DRW_LoftedSurface>(
                *static_cast<const DRW_LoftedSurface*>(data));
            break;
        case DRW::NURBSURFACE:
            retainTypedEntity<DRW_NurbsSurface>(
                *static_cast<const DRW_NurbsSurface*>(data));
            break;
        default:
            break;
        }
    }

    void linkImage(const DRW_ImageDef *data) override;

    virtual void addModelerGeometry(const DRW_ModelerGeometry& data) override{
        retainTypedEntity<DRW_ModelerGeometry>(data);
    }
    virtual void addModelerGeometry(DRW_ModelerGeometry&& data) override{
        retainTypedEntity<DRW_ModelerGeometry>(std::move(data));
    }

    void addRawDxfSection(const DRW_RawDxfSection& data) override;

    void addDataStorage(const DRW_DataStorageSection& data) override;
    void addDataStorage(DRW_DataStorageSection&& data) override;

//writer part, send all in class dx_data to writer
    virtual void addComment(const char* /*comment*/) override{}
    void addPlotSettings(const DRW_PlotSettings *data) override;
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
        std::unordered_map<std::uint32_t, const DRW_Block_Record*> records;
        for (const DRW_Block_Record& record : cData->blockRecords)
            records.emplace(record.handle, &record);
        for (std::list<dx_ifaceBlock*>::iterator it=cData->blocks.begin(); it != cData->blocks.end(); ++it) {
            if (isFixedSpaceBlock(*it))
                continue;
            const auto metadata = records.find((*it)->parentHandle);
            if (metadata != records.end())
                dxfW->writeBlockRecord((*it)->name, metadata->second->insUnits,
                                      metadata->second->previewData, {},
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
        if (dxfW->getVersion() > DRW::AC1009) {
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
        for (const std::uint32_t handle : m_tableStylesToEmit) {
            if (dxfW->preallocateEntityHandle(handle) == 0) {
                (void)dxfW->writeTable(nullptr);
                return;
            }
        }
        for (const std::uint32_t handle : m_extensionCellStyleMapHandles) {
            if (dxfW->preallocateEntityHandle(handle) == 0) {
                (void)dxfW->writeTable(nullptr);
                return;
            }
        }
    }
    //write entities of model space and first paper_space
    virtual void writeEntities() override{
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
        for (std::list<DRW_LType>::iterator it=cData->lineTypes.begin(); it != cData->lineTypes.end(); ++it)
            dxfW->writeLineType(&(*it));
    }
    virtual void writeLayers() override{
        for (std::list<DRW_Layer>::iterator it=cData->layers.begin(); it != cData->layers.end(); ++it)
            dxfW->writeLayer(&(*it));
    }
    virtual void writeTextstyles() override{
        for (std::list<DRW_Textstyle>::iterator it=cData->textStyles.begin(); it != cData->textStyles.end(); ++it)
            dxfW->writeTextstyle(&(*it));
    }
    virtual void writeVports() override{
        for (std::list<DRW_Vport>::iterator it=cData->VPorts.begin(); it != cData->VPorts.end(); ++it) {
            dxfW->writeVport(&(*it));
        }
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

        if (!hasUnchangedAcdsDxfHistorySources() || !hasUnchangedDxfMaterialSources()) {
            dxfW->writeRawDxfObject(nullptr);
            return;
        }
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
        for (std::uint32_t handle : m_extensionDictionaryHandles) {
            if (handle == layoutDictionaryHandle
                || !emittedDictionaryHandles.insert(handle).second)
                continue;
            // The graph validates this subtree, but the named-dictionary
            // codec plan is its sole emitter. Do not emit its root twice.
            if (std::any_of(m_dxfMaterialWriteDictionaries.begin(), m_dxfMaterialWriteDictionaries.end(),
                    [handle](const DRW_Dictionary& dictionary) { return dictionary.handle == handle; })) continue;
            for (DRW_Dictionary& dictionary : cData->dictionaries) {
                if (dictionary.handle != handle)
                    continue;
                if (!dxfW->writeDictionary(&dictionary))
                    return;
                break;
            }
        }
        std::unordered_set<std::uint32_t> dictionaryWithDefaultHandles;
        std::unordered_set<std::uint32_t> placeholderHandles;
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
                if (rawMatches != 0u
                    || !dxfW->writeDictionaryWithDefault(typed))
                    return;
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
        for (std::uint32_t handle : m_extensionPlotSettingsHandles) {
            for (DRW_PlotSettings& settings : cData->plotSettings) {
                if (settings.handle != handle)
                    continue;
                if (!dxfW->writePlotSettings(&settings))
                    return;
                break;
            }
        }
        std::unordered_set<std::uint32_t> renderSettingsHandles;
        for (std::uint32_t handle : m_extensionRenderSettingsHandles) {
            std::size_t matches = 0;
            for (DRW_RawDxfObject& object : cData->rawRenderSettingsObjects) {
                if (object.handle != handle)
                    continue;
                ++matches;
                if (!dxfW->writeRawDxfObject(&object))
                    return;
            }
            if (matches != 1u
                || !renderSettingsHandles.insert(handle).second) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
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
        std::unordered_set<std::uint32_t> sunHandles;
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
        std::unordered_set<std::uint32_t> sortDictionaries(
            m_sortDictionaryHandles.begin(), m_sortDictionaryHandles.end());
        for (DRW_Dictionary& dictionary : cData->dictionaries) {
            if (sortDictionaries.erase(dictionary.handle) == 0)
                continue;
            if (emittedDictionaryHandles.insert(dictionary.handle).second
                && !dxfW->writeDictionary(&dictionary))
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
                if (!dxfW->writeLayout(&layout))
                    return;
            }
        }
        for (std::uint32_t handle : m_extensionXRecordHandles) {
            for (DRW_XRecord& record : cData->xRecords) {
                if (record.handle == handle) {
                    if (!dxfW->writeXRecord(&record))
                        return;
                    break;
                }
            }
        }
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
                if (rawMatches != 0u || typedMatches != 1u
                    || typedVar == nullptr
                    || !dxfW->writeDictionaryVar(typedVar))
                    return;
            } else if (rawMatches != 1u || typedMatches != 1u
                       || rawVar == nullptr
                       || !dxfW->writeRawDxfObject(rawVar)) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        std::unordered_set<std::uint32_t> scaleHandles;
        for (std::uint32_t handle : m_extensionScaleHandles) {
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
                if (rawMatches != 0u || typedMatches != 1u
                    || typedScale == nullptr
                    || !dxfW->writeScale(typedScale))
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
        for (std::uint32_t handle : m_extensionMLineStyleHandles) {
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
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
            if (cData->sourceFormat == dx_source_format::DWG) {
                if (rawMatches != 0u || typedMatches != 1u
                    || typedStyle == nullptr
                    || !cData->hasSingleSuccessfulNamedResourceImport()
                    || !dxfW->writeMLineStyle(typedStyle))
                    return;
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
            } else if (rawMatches != 1u || typedMatches != 1u
                       || rawStyle == nullptr
                       || !cData->hasSingleSuccessfulNamedResourceImport()
                       || !dxfW->writeRawDxfObject(rawStyle)) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        std::unordered_set<std::uint32_t> visualStyleHandles;
        for (std::uint32_t handle : m_extensionVisualStyleHandles) {
            std::size_t matches = 0;
            for (DRW_RawDxfObject& object : cData->rawVisualStyleObjects) {
                if (object.handle != handle)
                    continue;
                ++matches;
                if (!dxfW->writeRawDxfObject(&object))
                    return;
            }
            if (matches != 1u || !visualStyleHandles.insert(handle).second) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
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
                if (rawMatches != 0u || typedMatches != 1u
                    || typedStyle == nullptr
                    || !dxfW->writeTableStyle(typedStyle, cData->textStyles))
                    return;
            } else if (rawMatches != 1u || typedMatches != 1u
                       || rawStyle == nullptr
                       || !writeTableStyleRaw(rawStyle, typedStyle)) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
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
            if ((!typedDwgStyle && !dxfCarrierStyle)
                || !detailViewStyleHandles.insert(handle).second
                || (typedDwgStyle && rawMatches != 0u)
                || typedMatches != 1u || typedStyle == nullptr
                || !cData->hasSingleSuccessfulModelDocViewStyleImport()
                || !writeDetailViewStyleRaw(
                    typedDwgStyle ? nullptr : rawStyle, typedStyle)) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
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
        std::unordered_set<std::uint32_t> rawProxyHandles;
        for (DRW_RawDxfObject& object : cData->rawProxyObjects) {
            if (object.handle == 0
                || !rawProxyHandles.insert(object.handle).second
                || !dxfW->writeRawDxfObject(&object)) {
                return;
            }
        }
        if (!cData->dataStorageSections.empty()) {
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
                        || !dxfW->writeRawDxfObject(&object)) {
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
        std::unordered_set<std::uint32_t> materialHandles;
        for (const std::uint32_t handle : m_dxfMaterialHandles) {
            std::size_t matches = 0;
            for (DRW_RawDxfObject& object : cData->rawMaterialObjects) {
                if (object.handle != handle)
                    continue;
                ++matches;
                if (!materialHandles.insert(handle).second
                    || !dxfW->writeRawDxfObject(&object)) {
                    dxfW->writeRawDxfObject(nullptr);
                    return;
                }
            }
            if (matches != 1u) {
                dxfW->writeRawDxfObject(nullptr);
                return;
            }
        }
        // The bounded typed EvaluationGraph writer is independently usable
        // for ordinary DXF input. AC1027 DataStorage closures stay on their
        // stricter proxy-graph gate until ACSH field serialization is complete.
        if (cData->dataStorageSections.empty() && m_acdsDxfHistoryObjects.empty()) {
            for (DRW_EvaluationGraph& graph : cData->evaluationGraphs) {
                if (!dxfW->writeEvaluationGraph(
                        &graph, "ACAD_EVALUATION_GRAPH")) {
                    return;
                }
            }
        }
        for (DRW_SortEntsTable& table : m_sortEmissionTables) {
            if (!dxfW->writeSortEntsTable(&table))
                return;
        }
    }
    virtual void writeAppId() override{
        for (std::list<DRW_AppId>::iterator it=cData->appIds.begin(); it != cData->appIds.end(); ++it)
            dxfW->writeAppId(&(*it));
    }

    dxfRW* dxfW {nullptr}; //pointer to writer, needed to send data
    dx_data* cData {nullptr}; // class to store or read data
    dx_ifaceBlock* currentBlock {nullptr};

private:
    dx_data* m_tableSourceReadData{nullptr};
    void observeDxfBlockDeliveryValue(std::int32_t,std::uint32_t,std::uint32_t,
                                    DRW_DxfBlockCallback,const DRW_RawDxfObject* = nullptr,const DRW_Entity* = nullptr) noexcept;
    using AttdefOriginResource=dx_data::TableDxfSourceCapture::Resource;
    using AttdefOriginBlock=dx_data::TableDxfSourceCapture::BlockSource;
    void captureDxfBlockMembership(AttdefOriginBlock&) noexcept;
    void observeDxfBlockMembership(AttdefOriginBlock&,const DRW_DxfBlockDeliveryEvent&,const DRW_Entity*) noexcept;
    static bool hasMatchedBlockMembership(const dx_data&,std::uint32_t) noexcept;
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
    friend class DxIfaceTableSourceTestAccess;
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
        std::vector<std::uint32_t> references;
    };
    std::map<std::uint32_t, StandardDwgOutputProof> m_standardDwgOutputProofs;
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
    bool inspectRasterOutput(const dxfRW::DxfObjectsWriteView& view) noexcept;
    bool inspectRasterImageEdges(const dxfRW::DxfObjectsWriteView& view, std::string_view section) noexcept;
    bool finalizeRasterOutputProofs() noexcept;
    void resetStandardDwgOutputProofs() noexcept;
    bool inspectStandardDwgOutput(const dxfRW::DxfObjectsWriteView& view) noexcept;
    bool inspectStandardDwgTablesOutput(const dxfRW::DxfTablesWriteView& view) noexcept;
    bool hasQualifiedStandardDwgOutputNames(std::uint64_t generation,
                                          std::uint32_t handle, bool finalizing) const;
    bool finalizeStandardDwgOutputProofs() noexcept;
    bool estimateTypedEntityRetainedBytes(
        const DRW_Entity& entity, std::size_t inlineBytes,
        std::size_t& estimatedBytes) const noexcept;
    bool estimateTypedBlockRetainedBytes(
        const DRW_Block& block, bool includeContainerStorage,
        std::size_t& estimatedBytes) const noexcept;
    bool prepareDwgViewportProjection(std::string& failure);
    bool planLeaderAnnotationHandles();
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
            return entity;
        } catch (...) {
            delete entity;
            cData->typedEntityCaptureFailed = true;
            return nullptr;
        }
    }
    bool prepareExtensionObjectGraph(dx_data* data,
                                     bool includeNamedObjectRoot);
    bool prepareTableFallbackGraph(dx_data& data, DRW::Version targetVersion);
    void writeEntityOrTableFallback(DRW_Entity* entity);
    bool validateSortEntsGraph(const dx_data& data);
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
    static bool collectAcdsTypedBoxHistoryObjects(
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
        DRW_RawDxfObject raw;
        DRW_Material common;
    };
    std::vector<DxfMaterialWriteSource> m_dxfMaterialWriteSources;
    std::vector<DRW_Dictionary> m_dxfMaterialWriteDictionaries;
    dx_materialImportReceipt m_dxfMaterialWriteReceipt;
    bool m_dxfMaterialWriteInspected{false};
    bool m_dxfMaterialWriteRequired{false};
    bool prepareDxfMaterialSources(DRW::Version version);
    bool hasUnchangedDxfMaterialSources() const noexcept;
    bool inspectDxfMaterialOutput(const dxfRW::DxfObjectsWriteView& view) noexcept;

    std::vector<std::uint32_t> m_extensionDictionaryHandles;
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
    std::vector<std::uint32_t> m_extensionTableStyleHandles;
    std::vector<std::uint32_t> m_extensionCellStyleMapHandles;
    std::vector<std::uint32_t> m_extensionDetailViewStyleHandles;
    std::vector<std::uint32_t> m_extensionSectionViewStyleHandles;
    std::vector<std::uint32_t> m_extensionPlotSettingsHandles;
    std::vector<std::uint32_t> m_extensionRenderSettingsHandles;
    std::vector<std::uint32_t> m_spatialFilterHandlesToWrite;
    std::vector<std::uint32_t> m_backgroundHandlesToWrite;
    std::vector<std::uint32_t> m_sunHandlesToWrite;
    std::vector<std::uint32_t> m_sortDictionaryHandles;
    std::vector<std::uint32_t> m_sortOwnerHandles;
    std::vector<DRW_SortEntsTable> m_sortEmissionTables;
    std::unordered_map<std::uint32_t, DRW_Table*> m_tableFallbacks;
    std::unordered_set<std::uint32_t> m_emittedTableHandles;
    std::vector<std::uint32_t> m_tableStylesToEmit;
    std::vector<std::uint32_t> m_tableStyleDictionaryHandles;
    // Output-only DXF group 68/69 values for DWG-origin paper viewports.
    // Keys are borrowed source records and are cleared at fileExport exit.
    std::unordered_map<const DRW_Viewport*, std::pair<int, int>>
        m_dwgViewportProjections;
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
    std::unordered_map<std::uint32_t, std::string> m_hatchOwnerAliases;
    std::unordered_map<std::string, bool> m_hatchOwnerBlocks;
    std::string m_hatchOwnerContext;
    bool m_preserveGraphSourceIdentities{false};

    static bool isFixedSpaceBlock(const dx_ifaceBlock* block) {
        if (block == nullptr)
            return false;
        // DWG producers also use the uppercase reserved spellings. Match
        // only these ASCII names, without locale-dependent UTF-8 casing.
        std::string name = block->name;
        for (char& ch : name) {
            if (ch >= 'a' && ch <= 'z')
                ch = static_cast<char>(ch - 'a' + 'A');
        }
        return name == "*MODEL_SPACE" || name == "*PAPER_SPACE";
    }
};

#endif // DX_IFACE_H
