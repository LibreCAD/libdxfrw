// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFENTITIESSOURCE_H
#define DXFENTITIESSOURCE_H
#include "dxfphysicalentities.h"
#include <atomic>

namespace dxfEntitiesSource {
class Capture final:public dxfPhysicalEntities::Capture<DRW_DxfEntitiesSource,DRW_DxfEntitiesSourceLimits> {
    using Base=dxfPhysicalEntities::Capture<DRW_DxfEntitiesSource,DRW_DxfEntitiesSourceLimits>;
    static std::uint64_t newScope() noexcept {
        static std::atomic<std::uint64_t> next{1u};auto id=next.load(std::memory_order_relaxed);
        while(id!=(std::numeric_limits<std::uint64_t>::max)()) {
            if(next.compare_exchange_weak(id,id+1u,std::memory_order_relaxed))return id;
        }
        return 0u; // never wrap and lend a prior interval's identity
    }
public:
    explicit Capture(dxfReader& value) noexcept:Base(value,"ENDSEC"){}
    void start(DRW_DxfEntitiesSourceLimits cap) noexcept {
        auto& result=storage();using S=DRW_DxfEntitiesSource::Status;
        if(result.status!=S::NotStarted) {result.status=S::InvalidBoundary;return;}
        result.scopeId=newScope();if(!result.scopeId) {result.status=S::InvalidBoundary;return;}
        startCapture(cap);result.beginMarker=initialMarker();
    }
};
}
#endif
