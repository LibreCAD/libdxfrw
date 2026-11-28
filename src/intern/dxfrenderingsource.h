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
    // Only fields with the same scalar role are projected into the historical
    // native-shaped model. Independent DXF colors62/63/421 use the property DTO.
    static bool applyPrimary(DRW_VisualStyle& value,int code,const std::unique_ptr<dxfReader>& reader) {
        auto& b=value.m_body;
        switch(code){
        case 71:b.faceLightingModel=reader->getInt32();break;
        case 72:b.faceLightingQuality=reader->getInt32();break;
        case 73:b.faceColorMode=reader->getInt32();break;
        case 90:b.faceModifier=reader->getInt32();break;
        case 40:b.faceOpacity=reader->getDouble();break;
        case 41:b.faceSpecular=reader->getDouble();break;
        case 74:b.edgeModel=reader->getInt32();break;
        case 91:b.edgeStyle=reader->getInt32();break;
        case 64:b.edgeIntersectionColor=reader->getInt32();break;
        case 65:b.edgeObscuredColor=reader->getInt32();break;
        case 75:b.edgeObscuredLtype=reader->getInt32();break;
        case 175:b.edgeIntersectionLtype=reader->getInt32();break;
        case 42:b.edgeCreaseAngle=reader->getDouble();break;
        case 92:b.edgeModifier=reader->getInt32();break;
        case 66:b.edgeColor=reader->getInt32();break;
        case 43:b.edgeOpacity=reader->getDouble();break;
        case 76:b.edgeWidth=reader->getInt32();break;
        case 77:b.edgeOverhang=reader->getInt32();break;
        case 78:b.edgeJitter=reader->getInt32();break;
        case 67:b.edgeSilhouetteColor=reader->getInt32();break;
        case 79:b.edgeSilhouetteWidth=reader->getInt32();break;
        case 170:b.edgeHaloGap=reader->getInt32();break;
        case 171:b.edgeIsolines=reader->getInt32();break;
        case 290:b.edgeDoHidePrecision=reader->getBool();break;
        case 174:b.edgeStyleApply=reader->getInt32();break;
        case 93:b.displaySettings=reader->getInt32();break;
        case 44:b.displayBrightness=reader->getDouble();break;
        case 173:b.displayShadowType=reader->getInt32();break;
        default:return false;
        }
        return true;
    }
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
            if(std::tie(c.present,c.modern,c.rawIndex,c.cmcIndex,c.rawRgb,c.flags,c.normalizedIndex,c.name,c.bookName)
                !=std::tie(d.present,d.modern,d.rawIndex,d.cmcIndex,d.rawRgb,d.flags,d.normalizedIndex,d.name,d.bookName))return false;}
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

inline bool propertiesEqual(const DRW_DxfVisualStyleProperties& a,const DRW_DxfVisualStyleProperties& b) noexcept {
    return std::tie(a.integers,a.integerOccurrences,a.realOccurrences,a.descriptionOccurrences,a.legacyDescriptionOccurrences,
        a.tailGroups,a.inVisualStyle,a.propertyTail,a.invalidBoolean)
        ==std::tie(b.integers,b.integerOccurrences,b.realOccurrences,b.descriptionOccurrences,b.legacyDescriptionOccurrences,
        b.tailGroups,b.inVisualStyle,b.propertyTail,b.invalidBoolean)
        &&std::memcmp(a.reals.data(),b.reals.data(),sizeof(double)*a.reals.size())==0;
}

class VisualStyleDecoder {
    DRW_VisualStyle& model;
    DRW_DxfVisualStyleProperties properties;
public:
    explicit VisualStyleDecoder(DRW_VisualStyle& value):model(value){}
    const DRW_DxfVisualStyleProperties& snapshot()const noexcept{return properties;}
    bool parseCode(int code,const std::unique_ptr<dxfReader>& reader) {
        const auto before=Observation::visualStyle(model);
        if(before.second&&code!=102)++properties.tailGroups;
        else if(before.first){
            if(code==2)++properties.descriptionOccurrences;
            if(code==4)++properties.legacyDescriptionOccurrences;
            for(std::size_t i=0u;i<properties.integerCodes.size();++i)if(code==properties.integerCodes[i]){
                const auto value=reader->getInt32();properties.integers[i]=value;++properties.integerOccurrences[i];
                if((code==290||code==291)&&value!=0&&value!=1)properties.invalidBoolean=true;
                break;
            }
            for(std::size_t i=0u;i<properties.realCodes.size();++i)if(code==properties.realCodes[i]){
                properties.reals[i]=reader->getDouble();++properties.realOccurrences[i];break;
            }
        }
        if(!Observation::parseVisual(model,code,reader))return false;
        const auto after=Observation::visualStyle(model);
        properties.inVisualStyle=after.first;properties.propertyTail=after.second;return true;
    }
};
}
#endif
