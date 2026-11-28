/****************************************************************************
**  libDXFrw - DXF table capacity helpers                                  **
**                                                                        **
**  This file is part of libdxfrw.                                        **
****************************************************************************/

#ifndef DXFTABLECAPACITY_H
#define DXFTABLECAPACITY_H

#include <cstddef>
#include <cstdint>
#include <limits>

namespace DRW {
namespace intern {

constexpr bool dxfTableCapacityFitsGroup70(std::size_t count) noexcept {
    return count <= static_cast<std::size_t>(
        (std::numeric_limits<std::int16_t>::max)());
}

} // namespace intern
} // namespace DRW

#endif // DXFTABLECAPACITY_H
