// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFBLOCKCHILDREN_H
#define DXFBLOCKCHILDREN_H
#include "dxfphysicalentities.h"

namespace dxfBlockChildren {
class Capture final:public dxfPhysicalEntities::Capture<DRW_DxfBlockChildrenSource,DRW_DxfBlockChildrenSourceLimits> {
    using Base=dxfPhysicalEntities::Capture<DRW_DxfBlockChildrenSource,DRW_DxfBlockChildrenSourceLimits>;
public:
    explicit Capture(dxfReader& value) noexcept:Base(value,"ENDBLK"){}
    void start(std::uint32_t block,std::uint32_t owner,DRW_DxfBlockChildrenSourceLimits cap) noexcept {
        auto& result=storage();
        if(result.status!=DRW_DxfBlockChildrenSource::Status::NotStarted) {result.status=DRW_DxfBlockChildrenSource::Status::InvalidBoundary;return;}
        result.blockHandle=block;result.blockRecord=owner;startCapture(cap);
    }
};
}
#endif
