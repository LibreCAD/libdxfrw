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
#include <unordered_set>
#include <vector>

class dx_iface : public DRW_Interface {
public:
    dx_iface() = default;
    ~dx_iface() = default;
    bool fileImport(const std::string& fileI, dx_data *fData, bool debug);
    bool fileExport(const std::string& file, DRW::Version v, bool binary, dx_data *fData, bool debug);
    void writeEntity(DRW_Entity* e);

//reimplement virtual DRW_Interface functions

//reader part, stores all in class dx_data
    //header
    void addHeader(const DRW_Header* data) override{
        cData->headerC = *data;
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
    virtual void addTextStyle(const DRW_Textstyle& data) override{
        cData->textStyles.push_back(data);
    }
    virtual void addAppId(const DRW_AppId& data) override{
        cData->appIds.push_back(data);
    }
    virtual void addDictionary(const DRW_Dictionary& data) override {
        cData->dictionaries.push_back(data);
    }
    virtual void addXRecord(const DRW_XRecord& data) override {
        cData->xRecords.push_back(data);
    }
    virtual void addMaterial(const DRW_Material& data) override {
        if (cData != nullptr)
            cData->materials.push_back(data);
    }
    virtual void addProxyObject(const DRW_ProxyObject& data) override {
        if (cData != nullptr)
            cData->proxyObjects.push_back(data);
    }
    virtual void addRawDxfObject(const DRW_RawDxfObject& data) override {
        if (cData != nullptr && data.name == "ACAD_PROXY_OBJECT")
            cData->rawProxyObjects.push_back(data);
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
    virtual void addUnsupportedObject(const DRW_UnsupportedObject& data) override {
        if (cData == nullptr || data.m_isEntity)
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
        if (evaluationGraph || historyObject)
            cData->acdsHistoryDwgFrames.push_back(data);
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
        //complete copy of header vars:
        data = cData->headerC;
        //or copy one by one:
//        for (auto it=cData->headerC.vars.begin(); it != cData->headerC.vars.end(); ++it)
//            data.vars[it->first] = new DRW_Variant( *(it->second) );
    }

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
            dxfW->writeBlock(bk);
            //and write each entity in block
            for (std::list<DRW_Entity*>::const_iterator it=bk->ent.begin(); it!=bk->ent.end(); ++it)
                writeEntity(*it);
        }
    }
    //only send the name, needed by the reader to prepare handles of blocks & blockRecords
    virtual void writeBlockRecords() override{
        for (std::list<dx_ifaceBlock*>::iterator it=cData->blocks.begin(); it != cData->blocks.end(); ++it) {
            if (isFixedSpaceBlock(*it))
                continue;
            dxfW->writeBlockRecord((*it)->name);
        }
    }
    //write entities of model space and first paper_space
    virtual void writeEntities() override{
        for (std::list<DRW_Entity*>::const_iterator it=cData->mBlock->ent.begin(); it!=cData->mBlock->ent.end(); ++it)
            writeEntity(*it);
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
    virtual void writeDimstyles() override{
        for (std::list<DRW_Dimstyle>::iterator it=cData->dimStyles.begin(); it != cData->dimStyles.end(); ++it)
            dxfW->writeDimstyle(&(*it));
    }
    virtual void writeObjects() override {
        for (std::uint32_t handle : m_extensionDictionaryHandles) {
            for (DRW_Dictionary& dictionary : cData->dictionaries) {
                if (dictionary.handle != handle)
                    continue;
                if (!dxfW->writeDictionary(&dictionary))
                    return;
                break;
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
    bool prepareExtensionObjectGraph(dx_data* data);
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

    static bool isFixedSpaceBlock(const dx_ifaceBlock* block) {
        return block != nullptr
            && (block->name == "*Model_Space"
                || block->name == "*Paper_Space");
    }
};

#endif // DX_IFACE_H
