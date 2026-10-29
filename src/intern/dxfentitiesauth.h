// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFENTITIESAUTH_H
#define DXFENTITIESAUTH_H
#include "drw_interface.h"
#include <cstring>
#include <tuple>

namespace dxfEntitiesAuth {
// The original physical collector constructs fresh scalar variants only.
// Reject invisible retained/native state that could be introduced into a
// callback copy even when the active scalar appears unchanged.
class VariantAccess {
public:
 static bool canonical(const DRW_Variant& v) noexcept {
    const double zero=0.0;
    if(v.sIsLayerRef||v.hasDwgRawLayerReference||v.dwgRawLayerReference
        ||!v.dwgRawLayerReferenceText.empty()||v.dwgRawLayerReferenceVersion!=DRW::UNKNOWNV
        ||v.dwgCmColor||!v.bdata.empty()||std::memcmp(&v.vdata.x,&zero,sizeof(double))
        ||std::memcmp(&v.vdata.y,&zero,sizeof(double))||std::memcmp(&v.vdata.z,&zero,sizeof(double)))return false;
    if(v.vType==DRW_Variant::STRING)return v.content.s==&v.sdata;
    return (v.vType==DRW_Variant::INTEGER||v.vType==DRW_Variant::INTEGER64||v.vType==DRW_Variant::DOUBLE)&&v.sdata.empty();
 }
 static bool equal(const DRW_Variant& a,const DRW_Variant& b) noexcept {
    if(!canonical(a)||!canonical(b)||a.vCode!=b.vCode||a.vType!=b.vType)return false;
    switch(a.vType){
    case DRW_Variant::STRING:return a.sdata==b.sdata;
    case DRW_Variant::INTEGER:return a.content.i==b.content.i;
    case DRW_Variant::INTEGER64:return a.content.i64==b.content.i64;
    case DRW_Variant::DOUBLE:return std::memcmp(&a.content.d,&b.content.d,sizeof(double))==0;
    default:return false;
    }
 }
};
inline bool span(const DRW_DxfPhysicalRecordSpan& a,const DRW_DxfPhysicalRecordSpan& b) noexcept {
    return std::tie(a.ordinal,a.beginOffset,a.endOffset,a.ignoredComment)==std::tie(b.ordinal,b.beginOffset,b.endOffset,b.ignoredComment);
}
inline bool physical(const DRW_DxfEntitiesSource& a,const DRW_DxfEntitiesSource& b) noexcept {
    if(std::tie(a.status,a.version,a.binary,a.scopeId,a.groups,a.bytes,a.work,a.ignoredComments)
        !=std::tie(b.status,b.version,b.binary,b.scopeId,b.groups,b.bytes,b.work,b.ignoredComments)
        ||!span(a.beginMarker,b.beginMarker)||!span(a.endMarker,b.endMarker)||a.records.size()!=b.records.size())return false;
    for(std::size_t i=0;i<a.records.size();++i) {const auto& x=a.records[i];const auto& y=b.records[i];
        if(x.name!=y.name||x.endOrdinal!=y.endOrdinal||x.endOffset!=y.endOffset||!span(x.marker,y.marker)
            ||x.groups.size()!=y.groups.size()||x.rawValues.size()!=y.rawValues.size()||x.spans.size()!=y.spans.size())return false;
        for(std::size_t j=0;j<x.groups.size();++j)if(!VariantAccess::equal(x.groups[j],y.groups[j]))return false;
        for(std::size_t j=0;j<x.rawValues.size();++j)if(x.rawValues[j]!=y.rawValues[j])return false;
        for(std::size_t j=0;j<x.spans.size();++j)if(!span(x.spans[j],y.spans[j]))return false;
    }
    return true;
}
inline bool manifest(const DRW_DxfEntitiesDeliverySource& a,const DRW_DxfEntitiesDeliverySource& b) noexcept {
    if(std::tie(a.status,a.version,a.binary,a.scopeId,a.completedEvents,a.bytes,a.work)
        !=std::tie(b.status,b.version,b.binary,b.scopeId,b.completedEvents,b.bytes,b.work)
        ||!span(a.beginMarker,b.beginMarker)||!span(a.endMarker,b.endMarker)
        ||a.dispatches.size()!=b.dispatches.size()||a.events.size()!=b.events.size())return false;
    for(std::size_t i=0;i<a.dispatches.size();++i){const auto& x=a.dispatches[i];const auto& y=b.dispatches[i];
        if(x.firstEvent!=y.firstEvent||x.events!=y.events||!span(x.begin,y.begin)||!span(x.end,y.end))return false;}
    for(std::size_t i=0;i<a.events.size();++i){const auto& x=a.events[i];const auto& y=b.events[i];
        if(std::tie(x.dispatch,x.callback,x.origin,x.entityType,x.handle,x.owner,x.hasValue)
            !=std::tie(y.dispatch,y.callback,y.origin,y.entityType,y.handle,y.owner,y.hasValue))return false;}
    return true;
}
}
#endif
