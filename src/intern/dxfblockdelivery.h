// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFBLOCKDELIVERY_H
#define DXFBLOCKDELIVERY_H
#include "drw_interface.h"
#include "dxfreader.h"
#include <atomic>
#include <limits>
#include <optional>
#include <type_traits>

namespace dxfBlockDelivery {
class Capture final {
    using Status=DRW_DxfBlockDeliverySource::Status;
    DRW_DxfBlockDeliverySource result;
    std::optional<const DRW_DxfBlockDeliverySource> frozen;
    DRW_DxfBlockDeliveryLimits limits;
    std::shared_ptr<DxfReadRetainedLease> lease;
    dxfReader* reader{nullptr};
    bool active{false};
    std::size_t nextOrdinal{0},nextOffset{0};
    static std::uint64_t newScope() noexcept {
        static std::atomic<std::uint64_t> next{1u};
        auto id=next.load(std::memory_order_relaxed);
        while(id!=(std::numeric_limits<std::uint64_t>::max)()) {
            if(next.compare_exchange_weak(id,id+1u,std::memory_order_relaxed))return id;
        }
        return 0u; // Exhaustion stays closed; never wrap to a reused identity.
    }
    static DRW_DxfPhysicalRecordSpan span(const DxfReadRecordView& v) noexcept {
        return {v.ordinal,v.beginOffset,v.endOffset,v.ignoredComment};
    }
    bool charge(std::size_t bytes) {
        if(bytes>(std::numeric_limits<std::size_t>::max)()-64u
            ||result.bytes>limits.bytes||bytes>limits.bytes-result.bytes
            ||result.work>limits.work||bytes+64u>limits.work-result.work) {
            result.status=Status::Quota;return false;
        }
        const auto& budget=reader->retainedByteBudget();
        if(budget&& !lease) {
            DxfReadRetainedReservation initial(budget,bytes);
            if(!initial.reserved()){result.status=Status::Quota;return false;}
            auto owner=std::make_shared<DxfReadRetainedLease>(budget);
            if(!owner->adopt(initial)){result.status=Status::Quota;return false;}
            lease=std::move(owner);
        } else if(lease&&!lease->add(bytes)){result.status=Status::Quota;return false;}
        result.bytes+=bytes;result.work+=bytes+64u;return true;
    }
public:
    static constexpr std::size_t NoEvent=(std::numeric_limits<std::size_t>::max)();
    void start(dxfReader& input,std::uint32_t block,std::uint32_t owner,DRW_DxfBlockDeliveryLimits cap) noexcept {
        if(result.status!=Status::NotStarted){result.status=Status::InvalidBoundary;return;}
        reader=&input;limits=cap;result.blockHandle=block;result.blockRecord=owner;
        result.version=input.getSourceVersion();result.binary=input.isBinary();result.status=Status::Collecting;
        result.scopeId=newScope();if(!result.scopeId){result.status=Status::InvalidBoundary;return;}
        try {
            DxfReadRecordView first;
            if(!input.currentRecordView(first)||first.code!=0||first.ignoredComment
                ||!first.ordinal||first.beginOffset>=first.endOffset){result.status=Status::InvalidBoundary;return;}
            nextOrdinal=first.ordinal;nextOffset=first.beginOffset;result.endMarker=span(first);
            (void)charge(sizeof(result)+sizeof(DxfReadRetainedLease)+8u*sizeof(void*));
        }catch(...){result.status=Status::Allocation;}
    }
    void begin(const DxfReadRecordView& first) noexcept {
        if(result.status!=Status::Collecting)return;
        try {
            if(active||first.code!=0||first.ignoredComment||first.ordinal!=nextOrdinal
                ||first.beginOffset!=nextOffset||first.beginOffset>=first.endOffset){result.status=Status::InvalidBoundary;return;}
            if(result.dispatches.size()>=limits.dispatches||!charge(sizeof(DRW_DxfBlockDispatch))){result.status=Status::Quota;return;}
            DRW_DxfBlockDispatch dispatch;dispatch.begin=span(first);dispatch.firstEvent=result.events.size();
            result.dispatches.push_back(dispatch);active=true;
        }catch(...){result.status=Status::Allocation;}
    }
    void end(const DxfReadRecordView& next) noexcept {
        if(result.status!=Status::Collecting)return;
        if(!active||next.code!=0||next.ignoredComment||next.ordinal<=result.dispatches.back().begin.ordinal
            ||next.beginOffset<result.dispatches.back().begin.endOffset||next.beginOffset>=next.endOffset){result.status=Status::InvalidBoundary;return;}
        auto& dispatch=result.dispatches.back();dispatch.end=span(next);
        dispatch.events=result.events.size()-dispatch.firstEvent;
        nextOrdinal=next.ordinal;nextOffset=next.beginOffset;result.endMarker=span(next);active=false;
    }
    std::size_t event(DRW_DxfBlockCallback callback,std::int32_t type,std::uint32_t handle,std::uint32_t owner,bool hasValue) noexcept {
        if(result.status!=Status::Collecting)return NoEvent;
        try {
            if(!active||callback>=DRW_DxfBlockCallback::Count){result.status=Status::InvalidBoundary;return NoEvent;}
            // Report slot plus queued immutable origin and closure/control
            // allowance. This bounds the added capture, not all event-queue
            // payloads, allocator capacity or process RSS.
            constexpr auto bytes=2u*sizeof(DRW_DxfBlockDeliveryEvent)+sizeof(std::size_t)+4u*sizeof(void*);
            if(result.events.size()>=limits.events||!charge(bytes)){result.status=Status::Quota;return NoEvent;}
            const auto index=result.events.size();
            result.events.push_back({result.dispatches.size()-1u,callback,type,handle,owner,hasValue});return index;
        }catch(...){result.status=Status::Allocation;return NoEvent;}
    }
    void complete() noexcept {
        if(result.status==Status::Collecting)result.status=active?Status::InvalidBoundary:Status::Complete;
    }
    void freeze() noexcept {
        static_assert(std::is_nothrow_move_constructible<DRW_DxfBlockDeliverySource>::value,"delivery publication must move, not copy");
        if(!frozen)frozen.emplace(std::move(result));
    }
    bool completeManifest() const noexcept {return source().status==Status::Complete;}
    const DRW_DxfBlockDeliverySource& source() const noexcept {return frozen?*frozen:result;}
};
}
#endif
