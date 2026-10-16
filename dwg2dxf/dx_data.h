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

#include <array>
#include <cstdint>
#include <list>
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
    bool successful{false};
};

struct dx_importedRecordRange {
    std::size_t begin{0};
    std::size_t end{0};
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
    dx_visualStyleImportReceipt visualStyleImport;
    dx_namedResourceImportReceipt namedResourceImport;
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
    std::list<DRW_Dictionary>dictionaries; //typed OBJECTS dictionaries
    std::list<DRW_DictionaryWithDefault>dictionaryWithDefaults;
    std::list<DRW_AcDbPlaceholder>placeholders;
    std::list<DRW_MLineStyle>mLineStyles;
    std::list<DRW_MLeaderStyle>mLeaderStyles;
    std::list<DRW_TableStyle>tableStyles;
    std::list<DRW_DetailViewStyle>detailViewStyles;
    std::list<DRW_SectionViewStyle>sectionViewStyles;
    std::list<DRW_Scale>scales;
    std::list<DRW_DictionaryVar>dictionaryVars;
    DRW_Dictionary namedObjectDictionary; //position-identified DXF NOD
    bool hasNamedObjectDictionary {false};
    std::list<DRW_XRecord>xRecords;      //typed XRECORD objects
    std::list<DRW_Material>materials;    //typed MATERIAL objects
    std::list<DRW_VisualStyle>visualStyles; //typed visual style metadata
    std::list<DRW_RawDxfObject>rawVisualStyleObjects; //full DXF style tag carrier
    // DXF tag companions for the named-resource families whose adapter
    // callbacks also retain typed values. DWG bodies are never stored here.
    std::list<DRW_RawDxfObject>rawNamedResourceObjects;
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

    // True only while the entire typed/raw style set still belongs to one
    // successful, versioned import. Authored/mixed/reused containers fail
    // closed; this does not gate the existing DXF raw-replay path.
    bool hasSingleSuccessfulVisualStyleImport() const noexcept {
        const dx_visualStyleImportReceipt& receipt = visualStyleImport;
        if (!receipt.successful || receipt.generation != 1
            || receipt.visualStyleBegin != 0 || receipt.rawVisualStyleBegin != 0
            || receipt.visualStyleEnd != visualStyles.size()
            || receipt.rawVisualStyleEnd != rawVisualStyleObjects.size()
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

    // Verifies one fresh, successful import and (for DXF) a one-to-one
    // type/handle/owner join between typed values and their raw carriers.
    // It does not certify each family's decoded payload or writer profile.
    bool hasSingleSuccessfulNamedResourceImport() const;


};

#endif // DX_DATA_H
