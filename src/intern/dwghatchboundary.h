/******************************************************************************
** Internal DWG HATCH/MPOLYGON boundary-handle stream helpers.
******************************************************************************/

#ifndef LIBDXFRW_DWGHATCHBOUNDARY_H
#define LIBDXFRW_DWGHATCHBOUNDARY_H

#include <cstdint>
#include <memory>
#include <vector>

#include "dwgbuffer.h"
#include "drw_entities.h"

namespace dwgHatchInternal {

bool readBoundaryHandles(
    dwgBuffer& handleBuffer, std::uint64_t handleEndBit,
    std::uint32_t baseHandle, const std::vector<std::uint32_t>& counts,
    const std::vector<std::shared_ptr<DRW_HatchLoop>>& loops);

} // namespace dwgHatchInternal

#endif
