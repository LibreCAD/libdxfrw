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
#include <iosfwd>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class dx_iface : public DRW_Interface {
public:
    // Converter resource policy, not a DXF nesting/size restriction. Separate
    // limits also provide a small deterministic boundary-test seam.
    struct ExtensionGraphLimits {
        std::size_t maxNodes{1000000u};
        std::size_t maxEdges{4000000u};
        std::size_t maxStagedHandles{1000000u};
        // Bound inventory indexing independently of reachable traversal.
        std::size_t maxSourceRecords{4u * 1024u * 1024u};
    };
    enum class ExtensionGraphFailure {
        None, InvalidGraph, NodeLimit, EdgeLimit, StagedHandleLimit, SourceRecordLimit
    };
    struct ExtensionGraphStats {
        // Accepted work before success/failure; these are not live output sizes.
        std::size_t visitedNodes{0};
        // Reference slots, including owners and validated XRECORD pointers.
        std::size_t examinedEdges{0};
        // Prepared dictionary/XRECORD/VisualStyle entries, not payload bytes.
        std::size_t stagedHandles{0};
        std::size_t sourceRecords{0};
    };

    dx_iface() = default;
    ~dx_iface() = default;
    bool fileImport(const std::string& fileI, dx_data *fData, bool debug);
    static constexpr std::size_t DwgReadReportMaxBytes = 32u * 1024u;
    static void reportDwgReadOutcome(std::ostream& output,
                                     const dx_dwgReadOutcome& outcome);
    static dx_dxfSourceTextAdmission admitDxfSourceText(const dx_data& data) noexcept;
    dx_dxfSourceTextAdmission sourceTextAdmission() const noexcept {
        return m_sourceTextAdmission;
    }
    bool fileExport(const std::string& file, DRW::Version v, bool binary,
                    dx_data *fData, bool debug, bool requireResolvedOwners = false,
                    dx_dxfSourceTextPolicy sourceTextPolicy = dx_dxfSourceTextPolicy::Compatible,
                    DRW::DxfTextWritePolicy targetTextPolicy = DRW::DxfTextWritePolicy::Compatible);
    void writeEntity(DRW_Entity* e);
    void setExtensionGraphLimits(const ExtensionGraphLimits& limits) noexcept {
        m_extensionGraphLimits = limits;
    }
    ExtensionGraphStats extensionGraphStats() const noexcept {
        return m_extensionGraphStats;
    }
    ExtensionGraphFailure extensionGraphFailure() const noexcept {
        return m_extensionGraphFailure;
    }
    const char* extensionGraphFailureName() const noexcept {
        switch (m_extensionGraphFailure) {
        case ExtensionGraphFailure::None: return "none";
        case ExtensionGraphFailure::InvalidGraph: return "invalid-graph";
        case ExtensionGraphFailure::NodeLimit: return "node-limit";
        case ExtensionGraphFailure::EdgeLimit: return "edge-limit";
        case ExtensionGraphFailure::StagedHandleLimit: return "staged-handle-limit";
        case ExtensionGraphFailure::SourceRecordLimit: return "source-record-limit";
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
    void addDxfClass(const DRW_Class& data) override {
        cData->dxfClasses.push_back(data);
    }
    void addDwgClassCoverageReport(const DRW_DwgClassCoverageReport& data) override {
        cData->dwgClassCoverage = data;
    }

    //tables
    virtual void addLType(const DRW_LType& data) override{
        cData->lineTypes.push_back(data);
    }
    virtual void addLayer(const DRW_Layer& data) override{
        cData->layers.push_back(data);
    }
    virtual void addDimStyle(const DRW_Dimstyle& data) override{
        cData->dimStyles.push_back(data);
    }
    virtual void addVport(const DRW_Vport& data) override{
        cData->VPorts.push_back(data);
    }
    void addView(const DRW_View& data) override {
        if (cData != nullptr)
            cData->views.push_back(data);
    }
    virtual void addUCS(const DRW_UCS& data) override{
        cData->ucsList.push_back(data);
    }
    virtual void addTextStyle(const DRW_Textstyle& data) override{
        cData->textStyles.push_back(data);
    }
    virtual void addAppId(const DRW_AppId& data) override{
        cData->appIds.push_back(data);
    }
    void addBlockRecord(const DRW_Block_Record& data) override {
        cData->blockRecords.push_back(data);
    }
    void addLayout(const DRW_Layout& data) override {
        if (cData != nullptr)
            cData->layouts.push_back(data);
    }
    virtual void addDictionary(const DRW_Dictionary& data) override {
        if (cData != nullptr)
            cData->dictionaries.push_back(data);
    }
    virtual void addNamedObjectDictionary(
        const DRW_Dictionary& data) override {
        if (cData != nullptr) {
            cData->namedObjectDictionary = data;
            cData->hasNamedObjectDictionary = true;
        }
    }
    virtual void addXRecord(const DRW_XRecord& data) override {
        cData->xRecords.push_back(data);
    }
    virtual void addMaterial(const DRW_Material& data) override {
        if (cData != nullptr)
            cData->materials.push_back(data);
    }
    void addVisualStyle(const DRW_VisualStyle& data) override {
        if (cData != nullptr)
            cData->visualStyles.push_back(data);
    }
    virtual void addProxyObject(const DRW_ProxyObject& data) override {
        if (cData != nullptr)
            cData->proxyObjects.push_back(data);
    }
    virtual void addRawDxfObject(const DRW_RawDxfObject& data) override {
        if (cData == nullptr)
            return;
        if (data.name == "ACAD_PROXY_OBJECT")
            cData->rawProxyObjects.push_back(data);
        else if (data.name == "VISUALSTYLE")
            cData->rawVisualStyleObjects.push_back(data);
    }
    virtual void addEvaluationGraph(const DRW_EvaluationGraph& data) override {
        if (cData != nullptr)
            cData->evaluationGraphs.push_back(data);
    }
    virtual void addDynamicBlockObject(
            const DRW_DynamicBlockObject& data) override {
        if (cData != nullptr)
            cData->dynamicBlockObjects.push_back(data);
    }
    virtual void addAcShHistoryObject(const DRW_AcShHistoryObject& data) override {
        if (cData != nullptr)
            cData->acshHistoryObjects.push_back(data);
    }
    void addDwgFramePublication(const DRW_DwgFramePublication& data) override {
        if (cData == nullptr || !cData->dwgReadOutcome.attempted)
            return;
        const auto index = static_cast<std::size_t>(data.m_carrier);
        if (index < cData->dwgReadOutcome.observedReceiptCarriers.size())
            countDwgReadCallback(
                cData->dwgReadOutcome.observedReceiptCarriers[index]);
    }
    virtual void addUnsupportedObject(const DRW_UnsupportedObject& data) override {
        if (cData == nullptr)
            return;
        if (cData->dwgReadOutcome.attempted)
            countDwgReadCallback(cData->dwgReadOutcome.opaqueViewCallbacks);
        if (data.m_isEntity)
            return;
        // Preserve only raw frames paired with this bounded AC1027 projection
        // profile. m_rawBytes are DWG object-body bits, not DXF group data;
        // keeping them here is for handle/class correlation and diagnostics,
        // never as a DXF serializer input.
        const bool evaluationGraph =
            data.m_recordName == "ACAD_EVALUATION_GRAPH"
            || data.m_className == "AcDbEvalGraph";
        const bool historyObject =
            data.m_recordName == "ACSH_HISTORY_CLASS"
            || data.m_recordName == "ACSH_BOX_CLASS"
            || data.m_recordName == "ACSH_EXTRUSION_CLASS";
        if (evaluationGraph || historyObject) {
            cData->acdsHistoryDwgFrames.push_back(data);
            if (cData->dwgReadOutcome.attempted)
                countDwgReadCallback(
                    cData->dwgReadOutcome.opaqueFramesRetainedByAdapter);
        }
    }

    //blocks
    virtual void addBlock(const DRW_Block& data) override{
        dx_ifaceBlock* bk = new dx_ifaceBlock(data);
        currentBlock = bk;
        cData->blocks.push_back(bk);
    }
    virtual void endBlock() override{
        currentBlock = cData->mBlock;
    }

    virtual void setBlock(const int /*handle*/) override{}//unused

    //entities
    virtual void addPoint(const DRW_Point& data) override{
        currentBlock->ent.push_back(new DRW_Point(data));
    }
    virtual void addLine(const DRW_Line& data) override{
        currentBlock->ent.push_back(new DRW_Line(data));
    }
    virtual void add3DLine(const DRW_3DLine& data) override{
        if (currentBlock != nullptr)
            currentBlock->ent.push_back(new DRW_3DLine(data));
    }
    virtual void addRay(const DRW_Ray& data) override{
        currentBlock->ent.push_back(new DRW_Ray(data));
    }
    virtual void addXline(const DRW_Xline& data) override{
        currentBlock->ent.push_back(new DRW_Xline(data));
    }
    virtual void addArc(const DRW_Arc& data) override{
        currentBlock->ent.push_back(new DRW_Arc(data));
    }
    virtual void addCircle(const DRW_Circle& data) override{
        currentBlock->ent.push_back(new DRW_Circle(data));
    }
    virtual void addEllipse(const DRW_Ellipse& data) override{
        currentBlock->ent.push_back(new DRW_Ellipse(data));
    }
    virtual void addLWPolyline(const DRW_LWPolyline& data) override{
        currentBlock->ent.push_back(new DRW_LWPolyline(data));
    }
    virtual void addPolyline(const DRW_Polyline& data) override{
        currentBlock->ent.push_back(new DRW_Polyline(data));
    }
    virtual void addMesh(const DRW_Mesh& data) override{
        if (currentBlock != nullptr)
            currentBlock->ent.push_back(new DRW_Mesh(data));
    }
    virtual void addSpline(const DRW_Spline* data) override{
        currentBlock->ent.push_back(new DRW_Spline(*data));
    }
    virtual void addHelix(const DRW_Helix* data) override{
        if (data != nullptr && currentBlock != nullptr)
            currentBlock->ent.push_back(new DRW_Helix(*data));
    }
    // ¿para que se usa?
    virtual void addKnot(const DRW_Entity& data) override{(void)data;}

    virtual void addInsert(const DRW_Insert& data) override{
        currentBlock->ent.push_back(new DRW_Insert(data));
    }
    virtual void addTrace(const DRW_Trace& data) override{
        currentBlock->ent.push_back(new DRW_Trace(data));
    }
    virtual void add3dFace(const DRW_3Dface& data) override{
        currentBlock->ent.push_back(new DRW_3Dface(data));
    }
    virtual void addSolid(const DRW_Solid& data) override{
        currentBlock->ent.push_back(new DRW_Solid(data));
    }
    virtual void addMText(const DRW_MText& data) override{
        currentBlock->ent.push_back(new DRW_MText(data));
    }
    virtual void addText(const DRW_Text& data) override{
        if (const auto* rtext = dynamic_cast<const DRW_RText*>(&data))
            currentBlock->ent.push_back(new DRW_RText(*rtext));
        else if (const auto* arcText =
                     dynamic_cast<const DRW_ArcAlignedText*>(&data))
            currentBlock->ent.push_back(new DRW_ArcAlignedText(*arcText));
        else
            currentBlock->ent.push_back(new DRW_Text(data));
    }
    virtual void addDimAlign(const DRW_DimAligned *data) override{
        currentBlock->ent.push_back(new DRW_DimAligned(
            static_cast<const DRW_Dimension&>(*data)));
    }
    virtual void addDimLinear(const DRW_DimLinear *data) override{
        currentBlock->ent.push_back(new DRW_DimLinear(
            static_cast<const DRW_Dimension&>(*data)));
    }
    virtual void addDimRadial(const DRW_DimRadial *data) override{
        if (const auto* large = dynamic_cast<const DRW_DimLargeRadial*>(data))
            currentBlock->ent.push_back(new DRW_DimLargeRadial(*large));
        else
            currentBlock->ent.push_back(new DRW_DimRadial(
                static_cast<const DRW_Dimension&>(*data)));
    }
    virtual void addDimDiametric(const DRW_DimDiametric *data) override{
        currentBlock->ent.push_back(new DRW_DimDiametric(
            static_cast<const DRW_Dimension&>(*data)));
    }
    virtual void addDimAngular(const DRW_DimAngular *data) override{
        currentBlock->ent.push_back(new DRW_DimAngular(
            static_cast<const DRW_Dimension&>(*data)));
    }
    virtual void addDimAngular3P(const DRW_DimAngular3p *data) override{
        currentBlock->ent.push_back(new DRW_DimAngular3p(
            static_cast<const DRW_Dimension&>(*data)));
    }
    virtual void addDimOrdinate(const DRW_DimOrdinate *data) override{
        currentBlock->ent.push_back(new DRW_DimOrdinate(
            static_cast<const DRW_Dimension&>(*data)));
    }
    void addDimArc(const DRW_DimArc *data) override{
        currentBlock->ent.push_back(new DRW_DimArc(*data));
    }
    virtual void addLeader(const DRW_Leader *data) override{
        currentBlock->ent.push_back(new DRW_Leader(*data));
    }
    virtual void addHatch(const DRW_Hatch *data) override{
        currentBlock->ent.push_back(new DRW_Hatch(*data));
    }
    virtual void addMPolygon(const DRW_MPolygon *data) override{
        currentBlock->ent.push_back(new DRW_MPolygon(*data));
    }
    virtual void addViewport(const DRW_Viewport& data) override{
        currentBlock->ent.push_back(new DRW_Viewport(data));
    }
    virtual void addImage(const DRW_Image *data) override{
        dx_ifaceImg *img = new dx_ifaceImg(*data);
        currentBlock->ent.push_back(img);
        cData->images.push_back(img);
    }

    void addWipeout(const DRW_Wipeout *data) override {
        // WIPEOUT owns a masking boundary, not a raster file/IMAGEDEF link.
        if (data == nullptr || currentBlock == nullptr)
            return;
        auto copy = std::make_unique<DRW_Wipeout>(*data);
        currentBlock->ent.push_back(copy.get());
        copy.release();
    }

    virtual void addSurface(const DRW_Surface *data) override{
        if (data == nullptr || currentBlock == nullptr)
            return;
        switch (data->eType) {
        case DRW::PLANESURFACE:
            currentBlock->ent.push_back(
                new DRW_PlaneSurface(*static_cast<const DRW_PlaneSurface*>(data)));
            break;
        case DRW::EXTRUDEDSURFACE:
            currentBlock->ent.push_back(
                new DRW_ExtrudedSurface(*static_cast<const DRW_ExtrudedSurface*>(data)));
            break;
        case DRW::REVOLVEDSURFACE:
            currentBlock->ent.push_back(
                new DRW_RevolvedSurface(*static_cast<const DRW_RevolvedSurface*>(data)));
            break;
        case DRW::SWEPTSURFACE:
            currentBlock->ent.push_back(
                new DRW_SweptSurface(*static_cast<const DRW_SweptSurface*>(data)));
            break;
        case DRW::LOFTEDSURFACE:
            currentBlock->ent.push_back(
                new DRW_LoftedSurface(*static_cast<const DRW_LoftedSurface*>(data)));
            break;
        case DRW::NURBSURFACE:
            currentBlock->ent.push_back(
                new DRW_NurbsSurface(*static_cast<const DRW_NurbsSurface*>(data)));
            break;
        default:
            break;
        }
    }

    virtual void linkImage(const DRW_ImageDef *data) override{
        std::uint32_t handle = data->handle;
        std::string path(data->name);
        for (std::list<dx_ifaceImg*>::iterator it=cData->images.begin(); it != cData->images.end(); ++it){
            if ((*it)->ref == handle){
                dx_ifaceImg *img = *it;
                img->path = path;
            }
        }
    }

    virtual void addModelerGeometry(const DRW_ModelerGeometry& data) override{
        if (currentBlock != nullptr)
            currentBlock->ent.push_back(new DRW_ModelerGeometry(data));
    }

    virtual void addRawDxfSection(const DRW_RawDxfSection& data) override {
        if (cData != nullptr)
            cData->rawDxfSections.push_back(data);
    }

    virtual void addDataStorage(const DRW_DataStorageSection& data) override {
        if (cData != nullptr)
            cData->dataStorageSections.push_back(data);
    }

//writer part, send all in class dx_data to writer
    virtual void addComment(const char* /*comment*/) override{}
    virtual void addPlotSettings(const DRW_PlotSettings *data) override {
        (void)data;
        // default implementation for new DRW_Interface method
    }

    virtual void writeHeader(DRW_Header& data) override{
        // write() has reset its derived maps and planned BLOCK_RECORDs, but
        // no TABLES/reactor callback has run yet. Plan forward HATCH links now.
        for (std::uint32_t source : m_hatchSourceHandles) {
            const std::uint32_t emitted = dxfW->preallocateEntityHandle(source);
            const auto target = m_hatchTargets.find(source);
            if (emitted == 0 || target == m_hatchTargets.end()) {
                m_hatchPlanningFailed = true;
                break;
            }
            target->second.emitted = emitted;
        }
        //complete copy of header vars:
        data = cData->headerC;
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
            //and write each entity in block
            for (std::list<DRW_Entity*>::const_iterator it=bk->ent.begin(); it!=bk->ent.end(); ++it)
                writeEntity(*it);
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
                                      metadata->second->previewData);
            else
                dxfW->writeBlockRecord((*it)->name);
        }
    }
    //write entities of model space and first paper_space
    virtual void writeEntities() override{
        m_hatchOwnerContext.clear();
        for (std::list<DRW_Entity*>::const_iterator it=cData->mBlock->ent.begin(); it!=cData->mBlock->ent.end(); ++it)
            writeEntity(*it);
        // Fixed space BLOCK/ENDBLK records are regenerated, but their owned
        // source entities are not previews and must not disappear with them.
        for (const dx_ifaceBlock* block : cData->blocks) {
            if (!isFixedSpaceBlock(block))
                continue;
            for (DRW_Entity* entity : block->ent)
                writeEntity(entity);
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
        for (std::uint32_t handle : m_extensionDictionaryHandles) {
            if (handle == layoutDictionaryHandle)
                continue;
            for (DRW_Dictionary& dictionary : cData->dictionaries) {
                if (dictionary.handle != handle)
                    continue;
                if (!dxfW->writeDictionary(&dictionary))
                    return;
                break;
            }
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
        // The bounded typed EvaluationGraph writer is independently usable
        // for ordinary DXF input. AC1027 DataStorage closures stay on their
        // stricter proxy-graph gate until ACSH field serialization is complete.
        if (cData->dataStorageSections.empty()) {
            for (DRW_EvaluationGraph& graph : cData->evaluationGraphs) {
                if (!dxfW->writeEvaluationGraph(
                        &graph, "ACAD_EVALUATION_GRAPH")) {
                    return;
                }
            }
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
    dx_dxfSourceTextAdmission m_sourceTextAdmission;
    void countDwgReadCallback(std::size_t& count) noexcept {
        if (count != std::numeric_limits<std::size_t>::max())
            ++count;
        else
            cData->dwgReadOutcome.callbackCountsSaturated = true;
    }
    bool prepareExtensionObjectGraph(dx_data* data,
                                     bool includeNamedObjectRoot);
    bool validateHatchBoundaryGraph(const dx_data& data);
    void recordHatchBlockPublication(const std::string& name, bool written);
    void failHatchPublication() noexcept;
    bool validateVisualStyleReferences(const dx_data& data,
                                       const char** failure) const;
    static bool collectAcdsHistoryProxyObjects(
        const dx_data& data, std::vector<DRW_RawDxfObject>& objects,
        std::vector<std::uint32_t>& materialHandles, bool debug);
    static bool collectAcdsTypedBoxHistoryObjects(
        const dx_data& data, std::vector<DRW_RawDxfObject>& objects,
        std::vector<DRW_EvaluationGraph>& graphs,
        std::vector<std::uint32_t>& materialHandles, bool debug);
    std::vector<DRW_RawDxfObject> m_acdsHistoryObjects;
    std::vector<DRW_RawDxfObject> m_acdsTypedHistoryObjects;
    std::vector<DRW_EvaluationGraph> m_acdsTypedEvaluationGraphs;
    std::vector<std::uint32_t> m_acdsMaterialHandles;

    std::vector<std::uint32_t> m_extensionDictionaryHandles;
    std::vector<std::uint32_t> m_extensionXRecordHandles;
    std::vector<std::uint32_t> m_extensionVisualStyleHandles;
    ExtensionGraphLimits m_extensionGraphLimits;
    ExtensionGraphStats m_extensionGraphStats;
    ExtensionGraphFailure m_extensionGraphFailure{ExtensionGraphFailure::None};
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
    bool m_preserveHatchSourceIdentities{false};

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
