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

#include <list>
#include <vector>

// Read result and observations have separate meanings. This envelope belongs
// to one CLI import, not a support claim or an output-completeness verdict.
struct dx_dwgReadOutcome {
    bool attempted{false};
    bool operationSucceeded{false};
    DRW::Version sourceVersion{DRW::UNKNOWNV};
    DRW::error error{DRW::BAD_NONE};
    DwgReadObservations observations;
    // Receipt callbacks precede some companion callbacks and final ledger
    // marking. These are observed deliveries, not unique terminal counts.
    std::array<std::size_t, 4> observedReceiptCarriers{};
    std::size_t opaqueViewCallbacks{0};
    std::size_t opaqueFramesRetainedByAdapter{0};
    bool callbackCountsSaturated{false};
};

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
        for (std::list<DRW_Entity*>::const_iterator it=ent.begin(); it!=ent.end(); ++it)
            delete *it;
    }
    std::list<DRW_Entity*>ent; //stores the entities list
};

enum class dx_source_format : std::uint8_t {
    Unknown,
    DXF,
    DWG
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
    dx_dwgReadOutcome dwgReadOutcome;     //bounded most-recent-import evidence
    std::list<DRW_LType>lineTypes;      //stores a copy of all line types
    std::list<DRW_Layer>layers;         //stores a copy of all layers
    std::list<DRW_Dimstyle>dimStyles;   //stores a copy of all dimension styles
    std::list<DRW_Vport>VPorts;         //stores a copy of all vports
    std::list<DRW_UCS>ucsList;          //named UCS table records
    std::list<DRW_Textstyle>textStyles; //stores a copy of all text styles
    std::list<DRW_AppId>appIds;         //stores a copy of all line types
    std::vector<DRW_Block_Record> blockRecords; //owned source identity/metadata
    std::vector<DRW_Layout> layouts; //typed DXF LAYOUT graph records
    std::list<DRW_Dictionary>dictionaries; //typed OBJECTS dictionaries
    DRW_Dictionary namedObjectDictionary; //position-identified DXF NOD
    bool hasNamedObjectDictionary {false};
    std::list<DRW_XRecord>xRecords;      //typed XRECORD objects
    std::list<DRW_Material>materials;    //typed MATERIAL objects
    std::list<DRW_VisualStyle>visualStyles; //typed visual style metadata
    std::list<DRW_RawDxfObject>rawVisualStyleObjects; //full DXF style tag carrier
    std::list<DRW_ProxyObject>proxyObjects; //typed ACAD_PROXY_OBJECT records
    std::list<DRW_RawDxfObject>rawProxyObjects; //lossless DXF proxy passthrough
    std::vector<DRW_RawDxfSection> rawDxfSections; //opaque DXF section passthrough
    std::vector<DRW_Class> dxfClasses; //readback of emitted CLASS metadata
    DRW_DwgClassCoverageReport dwgClassCoverage; //source DWG class-number binding
    std::vector<DRW_DataStorageSection> dataStorageSections; //typed DWG AcDs data
    // Typed DWG callbacks needed to inspect bounded ACIS history closures.
    // The unsupported-object companions are DWG frame bodies, not DXF groups;
    // they are retained only for correlation/diagnostics, never DXF replay.
    std::vector<DRW_EvaluationGraph> evaluationGraphs;
    std::vector<DRW_AcShHistoryObject> acshHistoryObjects;
    std::vector<DRW_UnsupportedObject> acdsHistoryDwgFrames;
    // Retain dynamic-block callbacks so export can fail closed rather than
    // silently discarding the graph. No dynamic-block DXF writer is qualified.
    std::vector<DRW_DynamicBlockObject> dynamicBlockObjects;
    std::list<dx_ifaceBlock*>blocks;    //stores a copy of all blocks and the entities in it
    std::list<dx_ifaceImg*>images;      //temporary list to find images for link with DRW_ImageDef. Do not delete it!!

    dx_ifaceBlock* mBlock;              //container to store model entities


};

#endif // DX_DATA_H
