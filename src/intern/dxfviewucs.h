// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFVIEWUCS_H
#define DXFVIEWUCS_H
#include "drw_objects.h"
#include "dxfreader.h"
#include <cstring>
#include <tuple>

// Shared production/replay body grammar. Tuple state remains outside the
// installed models; independent field/source admission is a separate audit.
namespace dxfViewUcs {
class Decoder {
    DRW_View* view=nullptr;DRW_UCS* ucs=nullptr;
    DRW_UCSOrthoPoint pending;bool hasPending=false;unsigned mask=0u;
    static bool real(double a,double b) noexcept {return std::memcmp(&a,&b,sizeof(a))==0;}
    static bool point(const DRW_Coord& a,const DRW_Coord& b) noexcept {return real(a.x,b.x)&&real(a.y,b.y)&&real(a.z,b.z);}
    static bool handle(const dwgHandle& a,const dwgHandle& b) noexcept {return std::tie(a.code,a.size,a.ref,a.ref64)==std::tie(b.code,b.size,b.ref,b.ref64);}
    static bool color(const DRW_DwgCmcColorMetadata& a,const DRW_DwgCmcColorMetadata& b) noexcept {
        return std::tie(a.present,a.method,a.flags,a.rawIndex,a.rgb24,a.hasRgb,a.colorName,a.bookName)
            ==std::tie(b.present,b.method,b.flags,b.rawIndex,b.rgb24,b.hasRgb,b.colorName,b.bookName);
    }
    bool finishOption() {
        if(!hasPending)return true;
        if(mask!=7u||pending.type<1||pending.type>6||ucs->orthographicOptions.size()>=DRW_UCS::kMaxOrthographicOptions
            ||!DRW::reserve(ucs->orthographicOptions,static_cast<int>(ucs->orthographicOptions.size()+1u)))return false;
        if(ucs->orthographicOptions.empty())ucs->orthoOrigin=pending.point;
        ucs->orthographicOptions.push_back(pending);hasPending=false;mask=0u;pending={};return true;
    }
public:
    explicit Decoder(DRW_View& value):view(&value){}
    explicit Decoder(DRW_UCS& value):ucs(&value){}
    void reset() noexcept {hasPending=false;mask=0u;pending={};}
    bool parseCode(int code,const std::unique_ptr<dxfReader>& reader) {
        if(view)return view->parseCode(code,reader);
        if(code==71){if(!finishOption())return false;pending.type=reader->getInt32();pending.point={0.0,0.0,0.0};hasPending=true;mask=0u;return true;}
        if(code==13||code==23||code==33){const unsigned bit=code==13?1u:code==23?2u:4u;if(!hasPending||(mask&bit))return false;
            const auto value=reader->getDouble();if(code==13)pending.point.x=value;else if(code==23)pending.point.y=value;else pending.point.z=value;mask|=bit;return true;}
        return ucs->parseCode(code,reader);
    }
    bool finish() {return view?view->validateDxf():finishOption()&&ucs->validateDxf();}
    static std::pair<bool,std::uint8_t> observe(const DRW_TableEntry& value) noexcept {return {value.curr!=nullptr,value.currCoordMask};}
    template<class Common>static bool sameModel(const DRW_TableEntry& a,const DRW_TableEntry& b,Common common) noexcept {
        if(!common(a,b))return false;
        const auto* x=dynamic_cast<const DRW_UCS*>(&a);const auto* y=dynamic_cast<const DRW_UCS*>(&b);
        if(x||y){if(!x||!y||x->orthoType!=y->orthoType||x->orthographicOptions.size()!=y->orthographicOptions.size()
                ||!point(x->origin,y->origin)||!point(x->xAxisDirection,y->xAxisDirection)||!point(x->yAxisDirection,y->yAxisDirection)
                ||!point(x->orthoOrigin,y->orthoOrigin)||!real(x->elevation,y->elevation)||!handle(x->xrefBlockHandle,y->xrefBlockHandle)
                ||!handle(x->baseUcsHandle,y->baseUcsHandle)||!handle(x->namedUcsHandle,y->namedUcsHandle))return false;
            for(std::size_t i=0u;i<x->orthographicOptions.size();++i)if(x->orthographicOptions[i].type!=y->orthographicOptions[i].type||!point(x->orthographicOptions[i].point,y->orthographicOptions[i].point))return false;
            return true;}
        const auto* v=dynamic_cast<const DRW_View*>(&a);const auto* w=dynamic_cast<const DRW_View*>(&b);if(!v||!w)return false;
        if(std::tie(v->viewMode,v->renderMode,v->hasUCS,v->cameraPlottable,v->ucsOrthoType,v->namedUCS_ID,v->baseUCS_ID,
            v->m_useDefaultLights,v->m_defaultLightingType,v->m_ambientColor,v->m_ambientColorRgb,v->m_ambientColorName,
            v->m_backgroundHandle,v->m_visualStyleHandle,v->m_sunHandle,v->m_liveSectionHandle)
            !=std::tie(w->viewMode,w->renderMode,w->hasUCS,w->cameraPlottable,w->ucsOrthoType,w->namedUCS_ID,w->baseUCS_ID,
            w->m_useDefaultLights,w->m_defaultLightingType,w->m_ambientColor,w->m_ambientColorRgb,w->m_ambientColorName,
            w->m_backgroundHandle,w->m_visualStyleHandle,w->m_sunHandle,w->m_liveSectionHandle))return false;
        return point(v->size,w->size)&&point(v->center,w->center)&&point(v->viewDirectionFromTarget,w->viewDirectionFromTarget)&&point(v->targetPoint,w->targetPoint)
            &&point(v->ucsOrigin,w->ucsOrigin)&&point(v->ucsXAxis,w->ucsXAxis)&&point(v->ucsYAxis,w->ucsYAxis)
            &&real(v->lensLen,w->lensLen)&&real(v->frontClippingPlaneOffset,w->frontClippingPlaneOffset)&&real(v->backClippingPlaneOffset,w->backClippingPlaneOffset)
            &&real(v->twistAngle,w->twistAngle)&&real(v->ucsElevation,w->ucsElevation)&&real(v->m_brightness,w->m_brightness)&&real(v->m_contrast,w->m_contrast)
            &&handle(v->xrefBlockHandle,w->xrefBlockHandle)&&color(v->ambientColorDwgCmc(),w->ambientColorDwgCmc());
    }
};
}
#endif
