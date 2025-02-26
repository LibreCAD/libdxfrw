#ifndef DRW_READER_H
#define DRW_READER_H

#include "libdxfrw.h"

//class to store image data and path from DRW_ImageDef
class dx_ifaceImg : public DRW_Image {
public:
    dx_ifaceImg(){}
    dx_ifaceImg(const DRW_Image& p):DRW_Image(p){}
    ~dx_ifaceImg(){}
    std::string path; //stores the image path
};

//container class to store entites.
class dx_ifaceBlock : public DRW_Block {
public:
    dx_ifaceBlock(){}
    dx_ifaceBlock(const DRW_Block& p):DRW_Block(p){}
    ~dx_ifaceBlock(){
        for (std::vector<DRW_Entity*>::const_iterator it=ent.begin(); it!=ent.end(); ++it)
            delete *it;
    }
    std::vector<DRW_Entity*> ent; //stores the entities list
};


//container class to store full dwg/dxf data.
class dx_data {
public:
    dx_data(){
        mBlock = new dx_ifaceBlock();
    }
    ~dx_data(){
        //cleanup,
        for (std::vector<dx_ifaceBlock*>::const_iterator it=blocks.begin(); it!=blocks.end(); ++it)
            delete *it;
        delete mBlock;
    }

    DRW_Header headerC;                 //stores a copy of the header vars
    std::vector<DRW_LType> lineTypes;      //stores a copy of all line types
    std::vector<DRW_Layer> layers;         //stores a copy of all layers
    std::vector<DRW_Dimstyle> dimStyles;   //stores a copy of all dimension styles
    std::vector<DRW_Vport> viewports;         //stores a copy of all vports
    std::vector<DRW_Textstyle> textStyles; //stores a copy of all text styles
    std::vector<DRW_AppId> appIds;         //stores a copy of all line types
    std::vector<dx_ifaceBlock*> blocks;    //stores a copy of all blocks and the entities in it
    std::vector<dx_ifaceImg*> images;      //temporary list to find images for link with DRW_ImageDef. Do not delete it!!

    dx_ifaceBlock* mBlock;              //container to store model entities
};

class dx_iface : public DRW_Interface {
public:
    dx_iface(){dxfW = NULL;}
    ~dx_iface(){}
    bool fileImport(const std::string& data, dx_data *fData, bool debug = false, bool isDXF = false);
    std::string fileExport(DRW::Version v, bool binary, dx_data *fData, bool debug = false);
    void writeEntity(DRW_Entity* e);

//reimplement virtual DRW_Interface functions

//reader part, stores all in class dx_data
    //header
    void addHeader(const DRW_Header* data){
        cData->headerC = *data;
    }

    //tables
    virtual void addLType(const DRW_LType& data){
        cData->lineTypes.push_back(data);
    }
    virtual void addLayer(const DRW_Layer& data){
        cData->layers.push_back(data);
    }
    virtual void addDimStyle(const DRW_Dimstyle& data){
        cData->dimStyles.push_back(data);
    }
    virtual void addVport(const DRW_Vport& data){
        cData->viewports.push_back(data);
    }
    virtual void addTextStyle(const DRW_Textstyle& data){
        cData->textStyles.push_back(data);
    }
    virtual void addAppId(const DRW_AppId& data){
        cData->appIds.push_back(data);
    }

    //blocks
    virtual void addBlock(const DRW_Block& data){
        dx_ifaceBlock* bk = new dx_ifaceBlock(data);
        currentBlock = bk;
        cData->blocks.push_back(bk);
    }
    virtual void endBlock(){
        currentBlock = cData->mBlock;
    }

    virtual void setBlock(const int /*handle*/){}//unused

    //entities
    virtual void addPoint(const DRW_Point& data){
        currentBlock->ent.push_back(new DRW_Point(data));
    }
    virtual void addLine(const DRW_Line& data){
        currentBlock->ent.push_back(new DRW_Line(data));
    }
    virtual void addRay(const DRW_Ray& data){
        currentBlock->ent.push_back(new DRW_Ray(data));
    }
    virtual void addXline(const DRW_Xline& data){
        currentBlock->ent.push_back(new DRW_Xline(data));
    }
    virtual void addArc(const DRW_Arc& data){
        currentBlock->ent.push_back(new DRW_Arc(data));
    }
    virtual void addCircle(const DRW_Circle& data){
        currentBlock->ent.push_back(new DRW_Circle(data));
    }
    virtual void addEllipse(const DRW_Ellipse& data){
        currentBlock->ent.push_back(new DRW_Ellipse(data));
    }
    virtual void addLWPolyline(const DRW_LWPolyline& data){
        currentBlock->ent.push_back(new DRW_LWPolyline(data));
    }
    virtual void addPolyline(const DRW_Polyline& data){
        currentBlock->ent.push_back(new DRW_Polyline(data));
    }
    virtual void addSpline(const DRW_Spline* data){
        currentBlock->ent.push_back(new DRW_Spline(*data));
    }
    // ¿para que se usa?
    virtual void addKnot(const DRW_Entity& data){(void)data;}

    virtual void addInsert(const DRW_Insert& data){
        currentBlock->ent.push_back(new DRW_Insert(data));
    }
    virtual void addTrace(const DRW_Trace& data){
        currentBlock->ent.push_back(new DRW_Trace(data));
    }
    virtual void add3dFace(const DRW_3Dface& data){
        currentBlock->ent.push_back(new DRW_3Dface(data));
    }
    virtual void addSolid(const DRW_Solid& data){
        currentBlock->ent.push_back(new DRW_Solid(data));
    }
    virtual void addMText(const DRW_MText& data){
        currentBlock->ent.push_back(new DRW_MText(data));
    }
    virtual void addText(const DRW_Text& data){
        currentBlock->ent.push_back(new DRW_Text(data));
    }
    virtual void addDimAlign(const DRW_DimAligned *data){
        currentBlock->ent.push_back(new DRW_DimAligned(*data));
    }
    virtual void addDimLinear(const DRW_DimLinear *data){
        currentBlock->ent.push_back(new DRW_DimLinear(*data));
    }
    virtual void addDimRadial(const DRW_DimRadial *data){
        currentBlock->ent.push_back(new DRW_DimRadial(*data));
    }
    virtual void addDimDiametric(const DRW_DimDiametric *data){
        currentBlock->ent.push_back(new DRW_DimDiametric(*data));
    }
    virtual void addDimAngular(const DRW_DimAngular *data){
        currentBlock->ent.push_back(new DRW_DimAngular(*data));
    }
    virtual void addDimAngular3P(const DRW_DimAngular3p *data){
        currentBlock->ent.push_back(new DRW_DimAngular3p(*data));
    }
    virtual void addDimOrdinate(const DRW_DimOrdinate *data){
        currentBlock->ent.push_back(new DRW_DimOrdinate(*data));
    }
    virtual void addLeader(const DRW_Leader *data){
        currentBlock->ent.push_back(new DRW_Leader(*data));
    }
    virtual void addHatch(const DRW_Hatch *data){
        currentBlock->ent.push_back(new DRW_Hatch(*data));
    }
    virtual void addViewport(const DRW_Viewport& data){
        currentBlock->ent.push_back(new DRW_Viewport(data));
    }
    virtual void addImage(const DRW_Image *data){
        dx_ifaceImg *img = new dx_ifaceImg(*data);
        currentBlock->ent.push_back(new dx_ifaceImg(*data));
        cData->images.push_back(img);
    }

    virtual void linkImage(const DRW_ImageDef *data){
        duint32 handle = data->handle;
        std::string path(data->name);
        for (std::vector<dx_ifaceImg*>::iterator it=cData->images.begin(); it != cData->images.end(); ++it){
            if ((*it)->ref == handle){
                dx_ifaceImg *img = *it;
                img->path = path;
            }
        }
    }

//writer part, send all in class dx_data to writer
    virtual void addComment(const char* /*comment*/){}
    virtual void addPlotSettings(const DRW_PlotSettings *data) {
        (void)data;
        // default implementation for new DRW_Interface method
    }

    virtual void writeHeader(DRW_Header& data){
        //complete copy of header vars:
        data = cData->headerC;
        //or copy one by one:
//        for (auto it=cData->headerC.vars.begin(); it != cData->headerC.vars.end(); ++it)
//            data.vars[it->first] = new DRW_Variant( *(it->second) );
    }

    virtual void writeBlocks(){
        //write each block
        for (std::vector<dx_ifaceBlock*>::iterator it=cData->blocks.begin(); it != cData->blocks.end(); ++it){
            dx_ifaceBlock* bk = *it;
            dxfW->writeBlock(bk);
            //and write each entity in block
            for (std::vector<DRW_Entity*>::const_iterator it=bk->ent.begin(); it!=bk->ent.end(); ++it)
                writeEntity(*it);
        }
    }
    //only send the name, needed by the reader to prepare handles of blocks & blockRecords
    virtual void writeBlockRecords(){
        for (std::vector<dx_ifaceBlock*>::iterator it=cData->blocks.begin(); it != cData->blocks.end(); ++it)
            dxfW->writeBlockRecord((*it)->name);
    }
    //write entities of model space and first paper_space
    virtual void writeEntities(){
        for (std::vector<DRW_Entity*>::const_iterator it=cData->mBlock->ent.begin(); it!=cData->mBlock->ent.end(); ++it)
            writeEntity(*it);
    }
    virtual void writeLTypes(){
        for (std::vector<DRW_LType>::iterator it=cData->lineTypes.begin(); it != cData->lineTypes.end(); ++it)
            dxfW->writeLineType(&(*it));
    }
    virtual void writeLayers(){
        for (std::vector<DRW_Layer>::iterator it=cData->layers.begin(); it != cData->layers.end(); ++it)
            dxfW->writeLayer(&(*it));
    }
    virtual void writeTextstyles(){
        for (std::vector<DRW_Textstyle>::iterator it=cData->textStyles.begin(); it != cData->textStyles.end(); ++it)
            dxfW->writeTextstyle(&(*it));
    }
    virtual void writeVports(){
        for (std::vector<DRW_Vport>::iterator it=cData->viewports.begin(); it != cData->viewports.end(); ++it)
            dxfW->writeVport(&(*it));
    }
    virtual void writeDimstyles(){
        for (std::vector<DRW_Dimstyle>::iterator it=cData->dimStyles.begin(); it != cData->dimStyles.end(); ++it)
            dxfW->writeDimstyle(&(*it));
    }
    virtual void writeObjects() {
        // default implementation for new DRW_Interface method
    }
    virtual void writeAppId(){
        for (std::vector<DRW_AppId>::iterator it=cData->appIds.begin(); it != cData->appIds.end(); ++it)
            dxfW->writeAppId(&(*it));
    }

    dxfRW* dxfW; //pointer to writer, needed to send data
    dx_data* cData; // class to store or read data
    dx_ifaceBlock* currentBlock;
};

#endif // DRW_READER_H