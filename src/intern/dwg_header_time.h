/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#ifndef DWG_HEADER_TIME_H
#define DWG_HEADER_TIME_H

#include <cmath>
#include <cstdint>
#include <limits>

namespace dwgHeaderTime {

constexpr double kMillisecondsPerDay = 86400000.0;

/**
 * Split a stored Julian-day value into the two signed DWG BL fields.
 *
 * This checks only that the existing representation can encode the value;
 * it does not impose calendar-validity or positivity rules. Output arguments
 * are left unchanged on failure.
 */
inline bool split(double stored, std::int32_t& day,
                  std::int32_t& milliseconds) noexcept {
    if (!std::isfinite(stored))
        return false;

    const double dayFloor = std::floor(stored);
    const double minDay = static_cast<double>(
        (std::numeric_limits<std::int32_t>::min)());
    const double maxDay = static_cast<double>(
        (std::numeric_limits<std::int32_t>::max)());
    if (dayFloor < minDay || dayFloor > maxDay)
        return false;

    const double fraction = stored - dayFloor;
    const double roundedMilliseconds =
        std::round(fraction * kMillisecondsPerDay);
    if (!std::isfinite(roundedMilliseconds)
        || roundedMilliseconds < 0.0
        || roundedMilliseconds > kMillisecondsPerDay)
        return false;

    std::int32_t outputDay = static_cast<std::int32_t>(dayFloor);
    std::int32_t outputMilliseconds = 0;
    if (roundedMilliseconds == kMillisecondsPerDay) {
        if (outputDay == (std::numeric_limits<std::int32_t>::max)())
            return false;
        ++outputDay;
    } else {
        outputMilliseconds = static_cast<std::int32_t>(roundedMilliseconds);
    }

    day = outputDay;
    milliseconds = outputMilliseconds;
    return true;
}

} // namespace dwgHeaderTime

#endif // DWG_HEADER_TIME_H
