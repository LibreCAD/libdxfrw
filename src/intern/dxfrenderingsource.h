// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFRENDERINGSOURCE_H
#define DXFRENDERINGSOURCE_H
#include "drw_objects.h"
#include "dxfreader.h"
#include <cstring>
#include <tuple>
#include <utility>

namespace dxfRendering {
class Observation {
    static bool real(double a,double b) noexcept {return std::memcmp(&a,&b,sizeof(a))==0;}
    static std::size_t cursor(const DRW_TableEntry& value) noexcept {
        for(std::size_t i=0u;i<value.extData.size();++i)if(value.extData[i]==value.curr)return i;
        return value.extData.size();
    }
public:
    static std::pair<bool,bool> visualStyle(const DRW_VisualStyle& value) noexcept {
        return {value.m_dxfInVisualStyle,value.m_dxfInPropertyTail};
    }
    static bool parseVisual(DRW_VisualStyle& value,int code,const std::unique_ptr<dxfReader>& reader) {
        return value.parseCode(code,reader);
    }
    static bool privateCommon(const DRW_TableEntry& a,const DRW_TableEntry& b) noexcept {
        if(std::tie(a.oType,a.xDictFlag,a.numReactors,a.commonLinkTailValidated,a.hasDsData,a.objSize,a.handleStreamBitSize,a.currCoordMask)
            !=std::tie(b.oType,b.xDictFlag,b.numReactors,b.commonLinkTailValidated,b.hasDsData,b.objSize,b.handleStreamBitSize,b.currCoordMask)
            ||(a.curr!=nullptr)!=(b.curr!=nullptr))return false;
        if(!a.curr)return true;
        const auto x=cursor(a),y=cursor(b);return x<a.extData.size()&&y<b.extData.size()&&x==y;
    }
    static bool body(const DRW_VisualStyleBody& a,const DRW_VisualStyleBody& b) noexcept {
        const auto scalars=[](const DRW_VisualStyleBody& v){return std::tie(v.faceLightingModel,v.faceLightingQuality,v.faceColorMode,v.faceModifier,v.faceMonoColor,
            v.edgeModel,v.edgeStyle,v.edgeIntersectionColor,v.edgeObscuredColor,v.edgeObscuredLtype,v.edgeIntersectionLtype,v.edgeModifier,v.edgeColor,
            v.edgeWidth,v.edgeOverhang,v.edgeJitter,v.edgeSilhouetteColor,v.edgeSilhouetteWidth,v.edgeHaloGap,v.edgeIsolines,v.edgeStyleApply,v.edgeDoHidePrecision,
            v.displaySettings,v.displayShadowType,v.extLightingModel,v.internalOnly,v.hasR2013bExpansion,v.bProp1c,v.bProp1d,v.bProp1e,v.bProp1f,v.bProp20,v.bProp21,
            v.bProp22,v.bProp23,v.bProp24,v.blProp25,v.blProp28,v.cProp29,v.blProp2a,v.blProp2b,v.cProp2c,v.bProp2d,v.blProp2e,v.blProp2f,v.blProp30,
            v.bProp31,v.blProp32,v.cProp33,v.edgeWiggle,v.strokes,v.bProp37);};
        if(scalars(a)!=scalars(b))return false;
        const double x[]={a.faceOpacity,a.faceSpecular,a.edgeCreaseAngle,a.edgeOpacity,a.displayBrightness,a.bdProp26,a.bdProp27,a.bdProp34,a.bdProp38,a.bdProp39};
        const double y[]={b.faceOpacity,b.faceSpecular,b.edgeCreaseAngle,b.edgeOpacity,b.displayBrightness,b.bdProp26,b.bdProp27,b.bdProp34,b.bdProp38,b.bdProp39};
        for(std::size_t i=0u;i<sizeof(x)/sizeof(x[0]);++i)if(!real(x[i],y[i]))return false;
        const auto& u=a.m_encoding;const auto& v=b.m_encoding;
        if(std::tie(u.companions,u.companionCount,u.colorCount,u.hasSourceIsolines,u.sourceIsolines)
            !=std::tie(v.companions,v.companionCount,v.colorCount,v.hasSourceIsolines,v.sourceIsolines))return false;
        for(std::size_t i=0u;i<u.colors.size();++i){const auto& c=u.colors[i];const auto& d=v.colors[i];
            if(std::tie(c.present,c.modern,c.rawIndex,c.rawRgb,c.flags,c.normalizedIndex,c.name,c.bookName)
                !=std::tie(d.present,d.modern,d.rawIndex,d.rawRgb,d.flags,d.normalizedIndex,d.name,d.bookName))return false;}
        return true;
    }
    static bool visual(const DRW_VisualStyle& a,const DRW_VisualStyle& b) noexcept {
        return privateCommon(a,b)&&visualStyle(a)==visualStyle(b)&&a.desc==b.desc&&a.type==b.type
            &&a.m_bodyDecoded==b.m_bodyDecoded&&body(a.m_body,b.m_body);
    }
    static bool render(const DRW_RenderSettings& a,const DRW_RenderSettings& b) noexcept {
        if(!privateCommon(a,b)||std::tie(a.m_kind,a.m_classVersion,a.m_name,a.m_longs,a.m_strings,a.m_bools,a.m_shorts,a.m_bytes,
            a.m_fogEnabled,a.m_fogBackgroundEnabled,a.m_backfacesEnabled,a.m_environmentImageEnabled,a.m_environmentImageFilename,a.m_description,
            a.m_displayIndex,a.m_hasPredefined,a.m_hasPredefinedPresent,a.m_fogColorR,a.m_fogColorG,a.m_fogColorB,a.m_procedure,a.m_destination)
            !=std::tie(b.m_kind,b.m_classVersion,b.m_name,b.m_longs,b.m_strings,b.m_bools,b.m_shorts,b.m_bytes,
            b.m_fogEnabled,b.m_fogBackgroundEnabled,b.m_backfacesEnabled,b.m_environmentImageEnabled,b.m_environmentImageFilename,b.m_description,
            b.m_displayIndex,b.m_hasPredefined,b.m_hasPredefinedPresent,b.m_fogColorR,b.m_fogColorG,b.m_fogColorB,b.m_procedure,b.m_destination)
            ||a.m_doubles.size()!=b.m_doubles.size()||a.hasDwgRawData()!=b.hasDwgRawData()
            ||a.dwgRawData()!=b.dwgRawData()||a.dwgRawDataBitSize()!=b.dwgRawDataBitSize()||a.dwgRawDataVersion()!=b.dwgRawDataVersion())return false;
        for(std::size_t i=0u;i<a.m_doubles.size();++i)if(!real(a.m_doubles[i],b.m_doubles[i]))return false;
        return real(a.m_fogDensityNear,b.m_fogDensityNear)&&real(a.m_fogDensityFar,b.m_fogDensityFar)
            &&real(a.m_fogDistanceNear,b.m_fogDistanceNear)&&real(a.m_fogDistanceFar,b.m_fogDistanceFar);
    }
};
}
#endif
