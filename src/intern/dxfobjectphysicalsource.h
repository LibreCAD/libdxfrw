// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFOBJECTPHYSICALSOURCE_H
#define DXFOBJECTPHYSICALSOURCE_H
#include "drw_interface.h"
#include "dxfreader.h"

namespace dxfObjectPhysicalSource {
class Capture final:public DxfReadRecordObserver {
    using Source=DRW_DxfObjectPhysicalSource;using S=Source::Status;
    struct Owner {Source source;std::shared_ptr<DxfReadRetainedLease> lease;};
    dxfReader& reader;std::shared_ptr<Owner> owner;
    DRW_DxfObjectPhysicalSourceLimits limits;S state{S::NotCaptured};
    DxfReadRecordObserver* previous{nullptr};const void* publication{nullptr};
    bool attached{false};std::size_t nextOrdinal{0},lastOffset{0};
    static DRW_DxfPhysicalRecordSpan span(const DxfReadRecordView& v) noexcept {
        return {v.ordinal,v.beginOffset,v.endOffset,v.ignoredComment};
    }
    void status(S value) noexcept {state=value;if(owner)owner->source.status=value;}
    bool charge(std::size_t bytes,std::size_t work) {
        auto& s=owner->source;
        if(s.bytes>limits.bytes||bytes>limits.bytes-s.bytes||s.work>limits.work||work>limits.work-s.work){status(S::Quota);return false;}
        s.bytes+=bytes;s.work+=work;
        if(!retainDxfReadBytes(owner->lease,reader.retainedByteBudget(),bytes)){status(S::Quota);return false;}
        return true;
    }
    void accept(const DxfReadRecordView& v) {
        if(state!=S::Collecting)return;
        if(v.ordinal!=nextOrdinal||v.ordinal==(std::numeric_limits<std::size_t>::max)()
            ||v.beginOffset<lastOffset||v.endOffset<=v.beginOffset||(v.ignoredComment&&v.code!=999)){
            status(S::InvalidBoundary);return;}
        nextOrdinal=v.ordinal+1u;lastOffset=v.endOffset;
        if(!charge(0u,64u)||!charge(0u,v.text.size())||!charge(0u,v.raw.size()))return;
        if(v.code==0){owner->source.endMarker=span(v);status(S::Complete);return;}
        if(owner->source.fields.size()>=limits.fields){status(S::Quota);return;}
        if(!charge(sizeof(Source::Field),sizeof(Source::Field)))return;
        owner->source.fields.push_back({v.code,span(v)});
    }
public:
    Capture(dxfReader& value,const DRW_Interface& iface,bool selected,std::string_view name) noexcept:reader(value) {
        if(!selected)return;
        publication=reader.objectPhysicalPublication();reader.objectPhysicalPublication()=this;
        previous=reader.exchangeRecordObserver(this);attached=true;
        try {
            if(const auto* policy=dynamic_cast<const DRW_DxfObjectPhysicalSourcePolicy*>(&iface))limits=policy->dxfObjectPhysicalSourceLimits();
            const auto base=sizeof(Owner)+sizeof(DxfReadRetainedLease)+8u*sizeof(void*);
            if(name.size()>(std::numeric_limits<std::size_t>::max)()-base
                ||base+name.size()>limits.bytes||base+name.size()>limits.work){status(S::Quota);return;}
            owner=std::make_shared<Owner>();status(S::Collecting);
            if(!charge(base+name.size(),base+name.size()))return;
            auto& s=owner->source;s.version=reader.getSourceVersion();s.binary=reader.isBinary();s.name=name;
            DxfReadRecordView marker;
            if(!reader.currentRecordView(marker)||marker.code||!marker.ordinal||marker.endOffset<=marker.beginOffset
                ||marker.ordinal==(std::numeric_limits<std::size_t>::max)()){status(S::InvalidBoundary);return;}
            s.marker=span(marker);nextOrdinal=marker.ordinal+1u;lastOffset=marker.endOffset;
        }catch(...){status(S::Allocation);}
    }
    ~Capture() override {
        if(attached){reader.exchangeRecordObserver(previous);reader.objectPhysicalPublication()=publication;}
    }
    Capture(const Capture&)=delete;Capture& operator=(const Capture&)=delete;
    void observe(const DxfReadRecordView& value) noexcept override {
        try{accept(value);}catch(...){status(S::Allocation);}
        if(previous)previous->observe(value);
    }
    void selfHandle(std::size_t ordinal,Admission admission) noexcept override {if(previous)previous->selfHandle(ordinal,admission);}
    S sourceStatus()const noexcept{return state;}
    std::shared_ptr<const Source> source()const noexcept {
        return owner?std::shared_ptr<const Source>(owner,&owner->source):nullptr;
    }
};
}
#endif
