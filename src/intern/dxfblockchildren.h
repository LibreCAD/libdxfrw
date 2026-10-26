// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFBLOCKCHILDREN_H
#define DXFBLOCKCHILDREN_H
#include "dxfphysicalentities.h"
#include <optional>
#include <type_traits>

namespace dxfBlockChildren {
class Capture final:public dxfPhysicalEntities::Capture<DRW_DxfBlockChildrenSource,DRW_DxfBlockChildrenSourceLimits> {
    using Base=dxfPhysicalEntities::Capture<DRW_DxfBlockChildrenSource,DRW_DxfBlockChildrenSourceLimits>;
    std::optional<const DRW_DxfBlockChildrenSource> frozen;
public:
    explicit Capture(dxfReader& value) noexcept:Base(value,"ENDBLK"){}
    void start(std::uint32_t block,std::uint32_t owner,DRW_DxfBlockChildrenSourceLimits cap) noexcept {
        auto& result=storage();
        if(result.status!=DRW_DxfBlockChildrenSource::Status::NotStarted) {result.status=DRW_DxfBlockChildrenSource::Status::InvalidBoundary;return;}
        result.blockHandle=block;result.blockRecord=owner;startCapture(cap);
    }
    void freeze() noexcept {
        static_assert(std::is_nothrow_move_constructible<DRW_DxfBlockChildrenSource>::value,"physical publication must move, not copy");
        detach();if(!frozen)frozen.emplace(std::move(storage()));
    }
    const DRW_DxfBlockChildrenSource& source() const noexcept {return frozen?*frozen:Base::source();}
};
}
#endif
