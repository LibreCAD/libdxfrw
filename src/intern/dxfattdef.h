// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFATTDEF_H
#define DXFATTDEF_H

#include "drw_entities.h"
#include "dxfreader.h"
#include <cstring>
#include <tuple>
#include <type_traits>

// Production ATTDEF body admission/finalization, shared with bounded replay.
// Entity/subclass parsing remains in DRW_Attdef, including the existing 101
// and legacy-100 embedded MTEXT routes. No independent native layout claim.
namespace dxfAttdef {
// Non-owning values observed before model copies discard parser state. These
// are not a semantic/profile certificate; incomplete XDATA remains visible.
struct SourceState {
    struct Cursor {
        bool present=false;
        std::uint8_t coordinateMask=0;
        bool operator==(const Cursor& b) const noexcept {
            return present==b.present&&coordinateMask==b.coordinateMask;
        }
    } host,nested;
    struct MTextScope {
        std::uint8_t mode=0,directionComponents=0,insertionComponents=0;
        bool attachment=false,rectangleWidth=false,rectangleHeight=false;
        bool totalWidth=false,totalHeight=false,columnType=false,columnCount=false;
        bool columnWidth=false,columnGutter=false,autoHeight=false,flowReversed=false;
        bool operator==(const MTextScope& b) const noexcept {
            return std::tie(mode,directionComponents,insertionComponents,attachment,
                rectangleWidth,rectangleHeight,totalWidth,totalHeight,columnType,
                columnCount,columnWidth,columnGutter,autoHeight,flowReversed)
                ==std::tie(b.mode,b.directionComponents,b.insertionComponents,b.attachment,
                b.rectangleWidth,b.rectangleHeight,b.totalWidth,b.totalHeight,b.columnType,
                b.columnCount,b.columnWidth,b.columnGutter,b.autoHeight,b.flowReversed);
        }
    } scope;
    bool attributeSubclass=false,hasNested=false;
    bool operator==(const SourceState& b) const noexcept {
        return host==b.host&&nested==b.nested&&scope==b.scope
            &&attributeSubclass==b.attributeSubclass&&hasNested==b.hasNested;
    }
    bool hasNoPendingXdata() const noexcept {
        return !host.present&&!host.coordinateMask&&!nested.present&&!nested.coordinateMask;
    }
};
static_assert(std::is_trivially_copyable<SourceState>::value,"ATTDEF state observation must not allocate");

class Decoder {
    DRW_Attdef& value;
    bool hasText=false,hasTag=false,hasX=false,hasY=false,hasHeight=false;
public:
    explicit Decoder(DRW_Attdef& target):value(target){}
    // Shared read-only entity inspection for converter source envelopes. These
    // helpers take the real entity type; no forged ATTDEF/table-entry cast.
    static SourceState::Cursor entityCursor(const DRW_Entity& value) noexcept {
        return {static_cast<bool>(value.curr),value.currCoordMask};
    }
    static bool hasNoNativeEntityWriteBindings(const DRW_Entity& value) noexcept {
        return value.dwgEedAppIdWriteRefs.empty()&&value.dwgEedLayerWriteRefs.empty();
    }
    template<class Equal>
    static bool sameEntityModel(const DRW_Entity& a,const DRW_Entity& b,Equal equal) noexcept {
        return sameCommon(a,b,equal);
    }
    static SourceState observeAttribute(const DRW_Attrib& value) noexcept {
        SourceState state;
        state.host={static_cast<bool>(value.curr),value.currCoordMask};
        state.attributeSubclass=value.dxfInAttributeSubclass;
        state.hasNested=static_cast<bool>(value.mtext);
        if(value.mtext) {
            const auto& m=*value.mtext;const auto& s=m.m_dxfOrientationScope;
            state.nested={static_cast<bool>(m.curr),m.currCoordMask};
            state.scope={static_cast<std::uint8_t>(s.mode),s.directionComponents,s.insertionComponents,
                s.attachmentSeen,s.rectangleWidthSeen,s.rectangleHeightSeen,s.totalWidthSeen,
                s.totalHeightSeen,s.columnTypeSeen,s.columnCountSeen,s.columnWidthSeen,
                s.columnGutterSeen,s.autoHeightSeen,s.flowReversedSeen};
        }
        return state;
    }
    static SourceState observe(const DRW_Attdef& value) noexcept {return observeAttribute(value);}
    static bool finishAttribute(DRW_Attrib& value) {
        if(value.mtext&&!value.mtext->finalizeDxfColumnObject())return false;
        if(value.mtext)value.mtext->updateAngle();
        return true;
    }
    static bool hasNoNativeWriteBindings(const DRW_Attdef& value) noexcept {
        const auto empty=[](const DRW_Entity& entity) {
            return entity.dwgEedAppIdWriteRefs.empty()&&entity.dwgEedLayerWriteRefs.empty();
        };
        return empty(value)&&(!value.mtext||empty(*value.mtext));
    }
    // Exact retained model equality, not source or graph qualification. Cursor
    // and grammar state are compared separately using observe() BEFORE copying.
    // Accounting leases/counters, storage capacity, vptrs and padding are not
    // persisted model semantics. Native-only values are otherwise included.
    template<class VariantEqual>
    static bool sameModel(const DRW_Attdef& a,const DRW_Attdef& b,VariantEqual equal) noexcept {
        return a.prompt==b.prompt&&sameAttributeModel(a,b,equal);
    }
    template<class VariantEqual>
    static bool sameAttributeModel(const DRW_Attrib& a,const DRW_Attrib& b,VariantEqual equal) noexcept {
        return sameText(a,b,equal)&&std::tie(a.tag,a.attribFlags,a.m_fieldLength,a.lockPosition,a.attVersion,
                a.m_attributeType,a.keepDuplicateRecords,a.m_r2018OuterAnnotativeData,
                a.m_r2018OuterAnnotativeUnknown,a.m_r2018OuterAnnotativeAppHandle,
                a.m_r2018OuterAnnotativeAppIdName)
            ==std::tie(b.tag,b.attribFlags,b.m_fieldLength,b.lockPosition,b.attVersion,
                b.m_attributeType,b.keepDuplicateRecords,b.m_r2018OuterAnnotativeData,
                b.m_r2018OuterAnnotativeUnknown,b.m_r2018OuterAnnotativeAppHandle,
                b.m_r2018OuterAnnotativeAppIdName)
            &&static_cast<bool>(a.mtext)==static_cast<bool>(b.mtext)
            &&(!a.mtext||sameMText(*a.mtext,*b.mtext,equal));
    }
    bool parseCode(int code,const std::unique_ptr<dxfReader>& reader) {
        hasText=hasText||code==1;
        hasTag=hasTag||code==2;
        hasX=hasX||code==10;
        hasY=hasY||code==20;
        hasHeight=hasHeight||code==40;
        return value.parseCode(code,reader);
    }
    // The existing host parser delegates application groups to nested MTEXT
    // once present, even after the attribute-definition subclass. Capturing
    // source must preserve that routing, not attach them all to the host.
    DRW_Entity& applicationEntity() const noexcept {
        return value.mtext ? static_cast<DRW_Entity&>(*value.mtext)
                           : static_cast<DRW_Entity&>(value);
    }
    bool parseApplication(const std::unique_ptr<dxfReader>& reader) {
        auto& entity=applicationEntity();
        entity.clearParserCursor();
        return entity.parseDxfGroups(102,reader);
    }
    bool finish(bool applyExtrusion) {
        if(!hasText||!hasTag||value.tag.empty()||!hasX||!hasY||!hasHeight||!finishAttribute(value))return false;
        if(applyExtrusion)value.applyExtrusion();
        return true;
    }
private:
    static bool real(double a,double b) noexcept {return std::memcmp(&a,&b,sizeof(a))==0;}
    static bool point(const DRW_Coord& a,const DRW_Coord& b) noexcept {
        return real(a.x,b.x)&&real(a.y,b.y)&&real(a.z,b.z);
    }
    static bool handle(const dwgHandle& a,const dwgHandle& b) noexcept {
        return std::tie(a.code,a.size,a.ref,a.ref64)==std::tie(b.code,b.size,b.ref,b.ref64);
    }
    static bool refs(const std::vector<DRW_Entity::PendingHandleRef>& a,
                     const std::vector<DRW_Entity::PendingHandleRef>& b) noexcept {
        if(a.size()!=b.size())return false;
        for(std::size_t i=0;i<a.size();++i)
            if(std::tie(a[i].indexInExtData,a[i].handleRef,a[i].rawHandleRef,a[i].useRawHandleRef)
                !=std::tie(b[i].indexInExtData,b[i].handleRef,b[i].rawHandleRef,b[i].useRawHandleRef))return false;
        return true;
    }
    template<class Equal>
    static bool sameCommon(const DRW_Entity& a,const DRW_Entity& b,Equal equal) noexcept {
        if(std::tie(a.eType,a.handle,a.parentHandle,a.space,a.layer,a.lineType,a.material,a.color,
            a.lWeight,a.visible,a.numProxyGraph,a.proxyGraphics,a.color24,a.colorName,a.transparency,
            a.plotStyle,a.shadow,a.shadowHandle,a.fullVisualStyleHandle,a.faceVisualStyleHandle,
            a.edgeVisualStyleHandle,a.reactorHandles,a.xDictHandle,a.hasDataStorageRecord,
            a.dataStorageHandle,a.dataStorageHandleKey,a.dataStorageData,a.dataStorageSegmentIndex,
            a.dataStorageSchemaIndex,a.hasDataStoragePayloadMarker,a.dataStoragePayloadMarkerOffset,
            a.dataStoragePayloadMarkerLength,a.dataStoragePayloadMarkerSection,a.haveExtrusion)
            !=std::tie(b.eType,b.handle,b.parentHandle,b.space,b.layer,b.lineType,b.material,b.color,
            b.lWeight,b.visible,b.numProxyGraph,b.proxyGraphics,b.color24,b.colorName,b.transparency,
            b.plotStyle,b.shadow,b.shadowHandle,b.fullVisualStyleHandle,b.faceVisualStyleHandle,
            b.edgeVisualStyleHandle,b.reactorHandles,b.xDictHandle,b.hasDataStorageRecord,
            b.dataStorageHandle,b.dataStorageHandleKey,b.dataStorageData,b.dataStorageSegmentIndex,
            b.dataStorageSchemaIndex,b.hasDataStoragePayloadMarker,b.dataStoragePayloadMarkerOffset,
            b.dataStoragePayloadMarkerLength,b.dataStoragePayloadMarkerSection,b.haveExtrusion)
            ||!real(a.ltypeScale,b.ltypeScale)
            ||std::tie(a.haveNextLinks,a.hasDsData,a.plotFlags,a.ltFlags,a.materialFlag,a.shadowFlag,
                a.hasFullVisualStyle,a.hasFaceVisualStyle,a.hasEdgeVisualStyle,a.hasAcDbColorH,
                a.acDbColorHandle,a.nextEntLink,a.prevEntLink,a.ownerHandle,a.xDictFlag,a.numReactors,
                a.commonLinkTailValidated,a.objSize,a.dwgDataEndBit,a.oType,a.dwgEedCodePage)
            !=std::tie(b.haveNextLinks,b.hasDsData,b.plotFlags,b.ltFlags,b.materialFlag,b.shadowFlag,
                b.hasFullVisualStyle,b.hasFaceVisualStyle,b.hasEdgeVisualStyle,b.hasAcDbColorH,
                b.acDbColorHandle,b.nextEntLink,b.prevEntLink,b.ownerHandle,b.xDictFlag,b.numReactors,
                b.commonLinkTailValidated,b.objSize,b.dwgDataEndBit,b.oType,b.dwgEedCodePage)
            ||!handle(a.lTypeH,b.lTypeH)||!handle(a.layerH,b.layerH)
            ||!point(a.extAxisX,b.extAxisX)||!point(a.extAxisY,b.extAxisY)
            ||!refs(a.pendingAppIdResolutions,b.pendingAppIdResolutions)
            ||!refs(a.pendingLayerRefResolutions,b.pendingLayerRefResolutions)
            ||!refs(a.dwgEedAppIdWriteRefs,b.dwgEedAppIdWriteRefs)
            ||!refs(a.dwgEedLayerWriteRefs,b.dwgEedLayerWriteRefs)
            ||a.extData.size()!=b.extData.size()||a.appData.size()!=b.appData.size())return false;
        const auto& x=a.m_proxyGraphicsProvenance;const auto& y=b.m_proxyGraphicsProvenance;
        if(std::tie(x.origin,x.encoding,x.sourceVersion,x.sourceCodePage,x.countCode,x.declaredByteCount,x.replayAuthorized)
            !=std::tie(y.origin,y.encoding,y.sourceVersion,y.sourceCodePage,y.countCode,y.declaredByteCount,y.replayAuthorized))return false;
        for(std::size_t i=0;i<a.extData.size();++i) {
            if(static_cast<bool>(a.extData[i])!=static_cast<bool>(b.extData[i])
                ||(a.extData[i]&&!equal(*a.extData[i],*b.extData[i])))return false;
        }
        auto group=b.appData.begin();
        for(const auto& values:a.appData) {
            if(values.size()!=group->size())return false;
            auto field=group->begin();
            for(const auto& value:values)if(!equal(value,*field++))return false;
            ++group;
        }
        return true;
    }
    template<class Equal>
    static bool sameText(const DRW_Text& a,const DRW_Text& b,Equal equal) noexcept {
        return sameCommon(a,b,equal)&&point(a.basePoint,b.basePoint)&&point(a.secPoint,b.secPoint)
            &&point(a.extPoint,b.extPoint)&&real(a.thickness,b.thickness)&&real(a.xAxisAngle,b.xAxisAngle)
            &&real(a.height,b.height)&&real(a.angle,b.angle)&&real(a.widthscale,b.widthscale)
            &&real(a.oblique,b.oblique)&&handle(a.styleH,b.styleH)
            &&std::tie(a.text,a.style,a.textgen,a.alignH,a.alignV)==std::tie(b.text,b.style,b.textgen,b.alignH,b.alignV);
    }
    template<class Equal>
    static bool sameMText(const DRW_MText& a,const DRW_MText& b,Equal equal) noexcept {
        if(!sameText(a,b,equal)||!real(a.interlin,b.interlin)||!real(a.m_definedHeight,b.m_definedHeight)
            ||!real(a.m_backgroundScale,b.m_backgroundScale)||!real(a.m_r2018RectWidth,b.m_r2018RectWidth)
            ||!real(a.m_r2018RectHeight,b.m_r2018RectHeight)||!real(a.m_r2018ExtentsHeight,b.m_r2018ExtentsHeight)
            ||!real(a.m_r2018ExtentsWidth,b.m_r2018ExtentsWidth)||!real(a.m_r2018ColumnWidth,b.m_r2018ColumnWidth)
            ||!real(a.m_r2018ColumnGutter,b.m_r2018ColumnGutter)||!real(a.m_dxfAngleSnapshot,b.m_dxfAngleSnapshot)
            ||!point(a.m_r2018XAxisDir,b.m_r2018XAxisDir)||!point(a.m_r2018InsertionPoint,b.m_r2018InsertionPoint)
            ||!point(a.m_dxfDirectionSnapshot,b.m_dxfDirectionSnapshot)
            ||a.m_r2018ColumnHeights.size()!=b.m_r2018ColumnHeights.size())return false;
        for(std::size_t i=0;i<a.m_r2018ColumnHeights.size();++i)
            if(!real(a.m_r2018ColumnHeights[i],b.m_r2018ColumnHeights[i]))return false;
        return std::tie(a.linespacingStyle,a.m_definedHeightPresent,a.m_backgroundFlags,a.m_backgroundColor,
            a.m_backgroundColorTrue,a.m_backgroundColorName,a.m_backgroundTransparency,a.m_r2018IsNotAnnotative,
            a.m_r2018ReallyLocked,a.m_r2018Version,a.m_r2018DefaultFlag,a.m_r2018AppIdHandle,a.m_r2018Attachment,
            a.m_r2018ColumnType,a.m_r2018ColumnCount,a.m_r2018ColumnAutoHeight,a.m_r2018ColumnFlowReversed,
            a.m_r2018DxfColumnContextPresent,a.m_r2018AnnotativeData,a.m_r2018AnnotativeUnknown,
            a.m_r2018AnnotativeAppHandle,a.m_backgroundColorDwgMethod,a.m_backgroundColorDwgFlags,
            a.m_backgroundColorDwgIndex,a.m_backgroundColorDwgMetadataPresent,a.hasXAxisVec,
            a.m_dxfOrientationSeen,a.m_dxfOrientationSnapshotValid)
            ==std::tie(b.linespacingStyle,b.m_definedHeightPresent,b.m_backgroundFlags,b.m_backgroundColor,
            b.m_backgroundColorTrue,b.m_backgroundColorName,b.m_backgroundTransparency,b.m_r2018IsNotAnnotative,
            b.m_r2018ReallyLocked,b.m_r2018Version,b.m_r2018DefaultFlag,b.m_r2018AppIdHandle,b.m_r2018Attachment,
            b.m_r2018ColumnType,b.m_r2018ColumnCount,b.m_r2018ColumnAutoHeight,b.m_r2018ColumnFlowReversed,
            b.m_r2018DxfColumnContextPresent,b.m_r2018AnnotativeData,b.m_r2018AnnotativeUnknown,
            b.m_r2018AnnotativeAppHandle,b.m_backgroundColorDwgMethod,b.m_backgroundColorDwgFlags,
            b.m_backgroundColorDwgIndex,b.m_backgroundColorDwgMetadataPresent,b.hasXAxisVec,
            b.m_dxfOrientationSeen,b.m_dxfOrientationSnapshotValid);
    }
};
} // namespace dxfAttdef
#endif
