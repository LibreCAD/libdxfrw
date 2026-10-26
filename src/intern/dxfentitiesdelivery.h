// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFENTITIESDELIVERY_H
#define DXFENTITIESDELIVERY_H
#include "drw_interface.h"
#include "dxfreader.h"
#include <limits>

namespace dxfEntitiesDelivery {
// No callback values are retained here. These are actual dispatch/return
// observations, not replay, source-body reconstruction or semantic proof.
class Capture final {
    using S=DRW_DxfEntitiesDeliverySource::Status;
    DRW_DxfEntitiesDeliverySource result;
    DRW_DxfEntitiesDeliveryLimits limits;
    dxfReader* reader{nullptr};
    std::shared_ptr<DxfReadRetainedLease> lease;
    bool active{false};
    DRW_DxfPhysicalRecordSpan next;
    static DRW_DxfPhysicalRecordSpan span(const DxfReadRecordView& v) noexcept {
        return {v.ordinal,v.beginOffset,v.endOffset,v.ignoredComment};
    }
    static bool equal(const DRW_DxfPhysicalRecordSpan& a,const DRW_DxfPhysicalRecordSpan& b) noexcept {
        return a.ordinal==b.ordinal&&a.beginOffset==b.beginOffset&&a.endOffset==b.endOffset&&a.ignoredComment==b.ignoredComment;
    }
    bool charge(std::size_t bytes) {
        if(bytes>(std::numeric_limits<std::size_t>::max)()-64u||result.bytes>limits.bytes||bytes>limits.bytes-result.bytes
            ||result.work>limits.work||bytes+64u>limits.work-result.work
            ||!retainDxfReadBytes(lease,reader->retainedByteBudget(),bytes)) {result.status=S::Quota;return false;}
        result.bytes+=bytes;result.work+=bytes+64u;return true;
    }
public:
    static constexpr std::size_t NoEvent=(std::numeric_limits<std::size_t>::max)();
    void start(dxfReader& input,const DRW_DxfEntitiesSource& physical,DRW_DxfEntitiesDeliveryLimits cap) noexcept {
        if(result.status!=S::NotStarted) {result.status=S::InvalidBoundary;return;}
        reader=&input;limits=cap;result.status=S::Collecting;
        result.version=physical.version;result.binary=physical.binary;result.scopeId=physical.scopeId;
        result.beginMarker=physical.beginMarker;next=physical.beginMarker;
        if(!physical.scopeId||!next.ordinal||next.ignoredComment||next.beginOffset>=next.endOffset
            ||(physical.status!=DRW_DxfEntitiesSource::Status::Collecting&&physical.status!=DRW_DxfEntitiesSource::Status::Complete)) {
            result.status=S::InvalidBoundary;return;
        }
        try {(void)charge(sizeof(result)+sizeof(DxfReadRetainedLease)+8u*sizeof(void*));}
        catch(...) {result.status=S::Allocation;}
    }
    void begin(dxfReader& input) noexcept {
        if(result.status!=S::Collecting)return;
        try {
            DxfReadRecordView v;
            if(active||!input.currentRecordView(v)||v.code!=0||v.ignoredComment||!equal(span(v),next)) {result.status=S::InvalidBoundary;return;}
            if(result.dispatches.size()>=limits.dispatches||!charge(sizeof(DRW_DxfEntitiesDispatch))) {result.status=S::Quota;return;}
            result.dispatches.push_back({span(v),{},result.events.size(),0u});active=true;
        }catch(...) {result.status=S::Allocation;}
    }
    void end(dxfReader& input) noexcept {
        if(result.status!=S::Collecting)return;
        DxfReadRecordView v;
        if(!active||!input.currentRecordView(v)||v.code!=0||v.ignoredComment
            ||v.ordinal<=result.dispatches.back().begin.ordinal||v.beginOffset<result.dispatches.back().begin.endOffset
            ||v.beginOffset>=v.endOffset||result.completedEvents!=result.events.size()) {result.status=S::InvalidBoundary;return;}
        auto& d=result.dispatches.back();d.end=span(v);d.events=result.events.size()-d.firstEvent;
        next=d.end;active=false;
    }
    std::size_t event(DRW_DxfEntitiesCallback callback,DRW_DxfEntitiesDeliveryEvent::Origin origin,
                      std::int32_t type,std::uint32_t handle,std::uint32_t owner,bool present) noexcept {
        if(result.status!=S::Collecting)return NoEvent;
        try {
            if(!active||callback>=DRW_DxfEntitiesCallback::Count||result.completedEvents!=result.events.size()) {result.status=S::InvalidBoundary;return NoEvent;}
            if(result.events.size()>=limits.events||!charge(sizeof(DRW_DxfEntitiesDeliveryEvent))) {result.status=S::Quota;return NoEvent;}
            const auto index=result.events.size();result.events.push_back({result.dispatches.size()-1u,callback,origin,type,handle,owner,present});return index;
        }catch(...) {result.status=S::Allocation;return NoEvent;}
    }
    void returned(std::size_t index) noexcept {
        if(result.status!=S::Collecting)return;
        if(index!=result.completedEvents||index>=result.events.size()) {result.status=S::InvalidBoundary;return;}
        ++result.completedEvents;
    }
    void complete(const DRW_DxfEntitiesSource& physical) noexcept {
        if(result.status!=S::Collecting)return;
        if(active||result.completedEvents!=result.events.size()||physical.status!=DRW_DxfEntitiesSource::Status::Complete
            ||physical.scopeId!=result.scopeId||physical.version!=result.version||physical.binary!=result.binary
            ||!equal(physical.beginMarker,result.beginMarker)||!equal(physical.endMarker,next)) {result.status=S::InvalidBoundary;return;}
        result.endMarker=physical.endMarker;result.status=S::Complete;
    }
    const DRW_DxfEntitiesDeliverySource& source() const noexcept {return result;}
};

// Immediate forwarding preserves dynamic types, pointers, source facets and
// target-internal fallback calls. There is no queue or payload copy/lease.
class Sink final:public DRW_Interface,public DRW_DxfTableSourceHandler,public DRW_DxfAttdefSourceHandler {
    using C=DRW_DxfEntitiesCallback;
    using O=DRW_DxfEntitiesDeliveryEvent::Origin;
    DRW_Interface& target;
    Capture& capture;
    DRW_DxfEntitiesDeliveryHandler* handler;
    DRW_DxfTableSourceHandler* table;
    DRW_DxfAttdefSourceHandler* attdef;
    O origin{O::Parser};
    template<class F>void invoke(C kind,std::int32_t type,std::uint32_t handle,std::uint32_t owner,bool present,F callback,O provenance) {
        if(!handler) {callback();return;}
        const auto index=capture.event(kind,provenance,type,handle,owner,present);
        if(index==Capture::NoEvent) {callback();return;}
        const auto event=capture.source().events[index];const auto scope=capture.source().scopeId;
        handler->beginDxfEntitiesDelivery(scope,index,event);
        callback();capture.returned(index);
        handler->endDxfEntitiesDelivery(scope,index,event);
    }
    template<class T,class F>void value(C kind,const T* data,F callback) {
        invoke(kind,data?static_cast<std::int32_t>(data->eType):0,data?data->handle:0u,data?data->parentHandle:0u,data!=nullptr,callback,origin);
    }
public:
    Sink(DRW_Interface& destination,Capture& observation,DRW_DxfEntitiesDeliveryHandler* receiver) noexcept
        :target(destination),capture(observation),handler(receiver),table(dynamic_cast<DRW_DxfTableSourceHandler*>(&destination)),
         attdef(dynamic_cast<DRW_DxfAttdefSourceHandler*>(&destination)){}
    class ProxyScope final {
        Sink* sink;O previous{O::Parser};
    public:
        explicit ProxyScope(DRW_Interface* value) noexcept:sink(dynamic_cast<Sink*>(value)) {
            if(sink) {previous=sink->origin;sink->origin=O::ProxyRendering;}
        }
        ~ProxyScope(){if(sink)sink->origin=previous;}
        ProxyScope(const ProxyScope&)=delete;ProxyScope& operator=(const ProxyScope&)=delete;
    };
    bool wantsDxfTableSource() const override {return table&&table->wantsDxfTableSource();}
    void addDxfTableSource(const DRW_Table& data,const DRW_RawDxfObject& raw) override {
        invoke(C::TableSource,static_cast<std::int32_t>(data.eType),data.handle,data.parentHandle,true,
            [&](){if(table)table->addDxfTableSource(data,raw);},O::SourceObserver);
    }
    bool wantsDxfAttdefSource() const override {return attdef&&attdef->wantsDxfAttdefSource();}
    void addDxfAttdefSource(const DRW_Attdef& data,const DRW_RawDxfObject& raw,const DRW_DxfAttdefSourceContext& context) override {
        invoke(C::AttdefSource,static_cast<std::int32_t>(data.eType),data.handle,data.parentHandle,true,
            [&](){if(attdef)attdef->addDxfAttdefSource(data,raw,context);},O::SourceObserver);
    }
#define DRW_ENTITIES_VALUE(method,type) \
    void method(const type& data) override {value(C::method,&data,[&](){target.method(data);});}
#define DRW_ENTITIES_POINTER(method,type) \
    void method(const type* data) override {value(C::method,data,[&](){target.method(data);});}
    DRW_ENTITIES_VALUE(addPoint,DRW_Point)
    DRW_ENTITIES_VALUE(addLine,DRW_Line)
    DRW_ENTITIES_VALUE(add3DLine,DRW_3DLine)
    DRW_ENTITIES_VALUE(addRay,DRW_Ray)
    DRW_ENTITIES_VALUE(addXline,DRW_Xline)
    DRW_ENTITIES_VALUE(addArc,DRW_Arc)
    DRW_ENTITIES_VALUE(addCircle,DRW_Circle)
    DRW_ENTITIES_VALUE(addEllipse,DRW_Ellipse)
    DRW_ENTITIES_VALUE(addLWPolyline,DRW_LWPolyline)
    DRW_ENTITIES_POINTER(addMLine,DRW_MLine)
    DRW_ENTITIES_POINTER(addUnderlay,DRW_Underlay)
    DRW_ENTITIES_VALUE(addShape,DRW_Shape)
    DRW_ENTITIES_VALUE(addOle2Frame,DRW_Ole2Frame)
    DRW_ENTITIES_VALUE(addOleFrame,DRW_OleFrame)
    DRW_ENTITIES_VALUE(addProxyEntity,DRW_ProxyEntity)
    DRW_ENTITIES_VALUE(addPolyline,DRW_Polyline)
    DRW_ENTITIES_POINTER(addSpline,DRW_Spline)
    DRW_ENTITIES_POINTER(addHelix,DRW_Helix)
    DRW_ENTITIES_VALUE(addMesh,DRW_Mesh)
    DRW_ENTITIES_VALUE(addKnot,DRW_Entity)
    DRW_ENTITIES_VALUE(addInsert,DRW_Insert)
    DRW_ENTITIES_VALUE(addTable,DRW_Table)
    DRW_ENTITIES_VALUE(addTrace,DRW_Trace)
    DRW_ENTITIES_VALUE(add3dFace,DRW_3Dface)
    DRW_ENTITIES_VALUE(addSolid,DRW_Solid)
    DRW_ENTITIES_VALUE(addMText,DRW_MText)
    DRW_ENTITIES_VALUE(addText,DRW_Text)
    DRW_ENTITIES_VALUE(addAttDef,DRW_Attdef)
    DRW_ENTITIES_VALUE(addTolerance,DRW_Tolerance)
    DRW_ENTITIES_VALUE(addCamera,DRW_Camera)
    DRW_ENTITIES_VALUE(addGeoPositionMarker,DRW_GeoPositionMarker)
    DRW_ENTITIES_VALUE(addSectionObject,DRW_SectionObject)
    DRW_ENTITIES_POINTER(addDimAlign,DRW_DimAligned)
    DRW_ENTITIES_POINTER(addDimLinear,DRW_DimLinear)
    DRW_ENTITIES_POINTER(addDimRadial,DRW_DimRadial)
    DRW_ENTITIES_POINTER(addDimDiametric,DRW_DimDiametric)
    DRW_ENTITIES_POINTER(addDimAngular,DRW_DimAngular)
    DRW_ENTITIES_POINTER(addDimAngular3P,DRW_DimAngular3p)
    DRW_ENTITIES_POINTER(addDimOrdinate,DRW_DimOrdinate)
    DRW_ENTITIES_POINTER(addDimArc,DRW_DimArc)
    DRW_ENTITIES_POINTER(addLeader,DRW_Leader)
    DRW_ENTITIES_POINTER(addHatch,DRW_Hatch)
    DRW_ENTITIES_POINTER(addMPolygon,DRW_MPolygon)
    DRW_ENTITIES_VALUE(addViewport,DRW_Viewport)
    DRW_ENTITIES_POINTER(addImage,DRW_Image)
    DRW_ENTITIES_POINTER(addWipeout,DRW_Wipeout)
    DRW_ENTITIES_POINTER(addPointCloud,DRW_PointCloud)
    DRW_ENTITIES_POINTER(addPointCloudEx,DRW_PointCloudEx)
    DRW_ENTITIES_POINTER(addNavisworksModel,DRW_NavisworksModel)
    DRW_ENTITIES_POINTER(addSurface,DRW_Surface)
    DRW_ENTITIES_POINTER(addMLeader,DRW_MLeader)
    DRW_ENTITIES_VALUE(addModelerGeometry,DRW_ModelerGeometry)
#undef DRW_ENTITIES_POINTER
#undef DRW_ENTITIES_VALUE
    void addRawDxfEntity(const DRW_RawDxfObject& data) override {
        invoke(C::addRawDxfEntity,0,data.handle,data.parentHandle,true,[&](){target.addRawDxfEntity(data);},origin);
    }
    void addHeader(const DRW_Header* data) override {target.addHeader(data);}
    void addLType(const DRW_LType& data) override {target.addLType(data);}
    void addLayer(const DRW_Layer& data) override {target.addLayer(data);}
    void addDimStyle(const DRW_Dimstyle& data) override {target.addDimStyle(data);}
    void addVport(const DRW_Vport& data) override {target.addVport(data);}
    void addTextStyle(const DRW_Textstyle& data) override {target.addTextStyle(data);}
    void addAppId(const DRW_AppId& data) override {target.addAppId(data);}
    void addBlock(const DRW_Block& data) override {target.addBlock(data);}
    void setBlock(int handle) override {target.setBlock(handle);}
    void endBlock() override {target.endBlock();}
    void linkImage(const DRW_ImageDef* data) override {target.linkImage(data);}
    void addComment(const char* data) override {target.addComment(data);}
    void writeHeader(DRW_Header& data) override {target.writeHeader(data);}
#define DRW_ENTITIES_WRITE(method) void method() override {target.method();}
    DRW_ENTITIES_WRITE(writeBlocks)
    DRW_ENTITIES_WRITE(writeBlockRecords)
    DRW_ENTITIES_WRITE(writeEntities)
    DRW_ENTITIES_WRITE(writeLTypes)
    DRW_ENTITIES_WRITE(writeLayers)
    DRW_ENTITIES_WRITE(writeTextstyles)
    DRW_ENTITIES_WRITE(writeVports)
    DRW_ENTITIES_WRITE(writeDimstyles)
    DRW_ENTITIES_WRITE(writeObjects)
    DRW_ENTITIES_WRITE(writeAppId)
#undef DRW_ENTITIES_WRITE
};
}
#endif
