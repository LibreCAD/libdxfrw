// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFLAYOUT_H
#define DXFLAYOUT_H
#include "drw_objects.h"
#include "dxfreader.h"
#include <cstring>
#include <tuple>

// Shared production/replay admission. The existing model parser owns subclass
// routing and tolerant defaults; physical scalar/role admission is a separate
// audit. Observe state before copies reset subclass and XDATA cursor state.
namespace dxfLayout {
struct SourceState {
    int subclass=0;bool pending=false;std::uint8_t coordinateMask=0;
    bool operator==(const SourceState& b) const noexcept {
        return std::tie(subclass,pending,coordinateMask)==std::tie(b.subclass,b.pending,b.coordinateMask);
    }
    bool complete() const noexcept {return !pending&&!coordinateMask;}
};
class Decoder {
    DRW_Layout& value;
    static bool real(double a,double b) noexcept {return std::memcmp(&a,&b,sizeof(a))==0;}
    static bool point(const DRW_Coord& a,const DRW_Coord& b) noexcept {return real(a.x,b.x)&&real(a.y,b.y)&&real(a.z,b.z);}
    static bool handle(const dwgHandle& a,const dwgHandle& b) noexcept {return std::tie(a.code,a.size,a.ref,a.ref64)==std::tie(b.code,b.size,b.ref,b.ref64);}
public:
    explicit Decoder(DRW_Layout& target):value(target){}
    bool parseCode(int code,const std::unique_ptr<dxfReader>& reader) {return value.parseCode(code,reader);}
    static SourceState observe(const DRW_Layout& v) noexcept {return {v.m_dxfSubclass,v.curr!=nullptr,v.currCoordMask};}
    template<class EqualCommon>static bool sameModel(const DRW_Layout& a,const DRW_Layout& b,EqualCommon common) noexcept {
        if(!common(a,b)||std::tie(a.pageSetupName,a.printerConfig,a.paperSize,a.plotViewName,a.currentStyleSheet,
            a.plotLayoutFlags,a.paperUnits,a.plotRotation,a.plotType,a.scaleType,a.shadePlotMode,a.shadePlotResLevel,a.shadePlotCustomDPI,
            a.layoutFlags,a.tabOrder,a.orthoViewType,a.viewportCount,a.viewportHandles)
            !=std::tie(b.pageSetupName,b.printerConfig,b.paperSize,b.plotViewName,b.currentStyleSheet,
            b.plotLayoutFlags,b.paperUnits,b.plotRotation,b.plotType,b.scaleType,b.shadePlotMode,b.shadePlotResLevel,b.shadePlotCustomDPI,
            b.layoutFlags,b.tabOrder,b.orthoViewType,b.viewportCount,b.viewportHandles))return false;
        return real(a.marginLeft,b.marginLeft)&&real(a.marginBottom,b.marginBottom)&&real(a.marginRight,b.marginRight)&&real(a.marginTop,b.marginTop)
            &&real(a.paperWidth,b.paperWidth)&&real(a.paperHeight,b.paperHeight)&&real(a.plotOriginX,b.plotOriginX)&&real(a.plotOriginY,b.plotOriginY)
            &&real(a.windowMinX,b.windowMinX)&&real(a.windowMinY,b.windowMinY)&&real(a.windowMaxX,b.windowMaxX)&&real(a.windowMaxY,b.windowMaxY)
            &&real(a.realWorldUnits,b.realWorldUnits)&&real(a.drawingUnits,b.drawingUnits)&&real(a.scaleFactor,b.scaleFactor)
            &&real(a.paperImageOriginX,b.paperImageOriginX)&&real(a.paperImageOriginY,b.paperImageOriginY)
            &&real(a.limMinX,b.limMinX)&&real(a.limMinY,b.limMinY)&&real(a.limMaxX,b.limMaxX)&&real(a.limMaxY,b.limMaxY)
            &&real(a.elevation,b.elevation)&&point(a.insPoint,b.insPoint)&&point(a.extMin,b.extMin)&&point(a.extMax,b.extMax)
            &&point(a.ucsOrigin,b.ucsOrigin)&&point(a.ucsXAxis,b.ucsXAxis)&&point(a.ucsYAxis,b.ucsYAxis)
            &&handle(a.plotViewHandle,b.plotViewHandle)&&handle(a.shadePlotHandle,b.shadePlotHandle)
            &&handle(a.paperSpaceBlockRecordHandle,b.paperSpaceBlockRecordHandle)&&handle(a.lastActiveViewportHandle,b.lastActiveViewportHandle)
            &&handle(a.baseUcsHandle,b.baseUcsHandle)&&handle(a.namedUcsHandle,b.namedUcsHandle);
    }
};
}
#endif
