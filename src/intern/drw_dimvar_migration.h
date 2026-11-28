/******************************************************************************
**  libDXFrw - library to read/write DXF and read DWG                         **
**                                                                           **
**  This file is part of libdxfrw.                                           **
**                                                                           **
**  libdxfrw is free software, licensed under the terms of the GNU           **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
******************************************************************************
*/

#ifndef DRW_DIMVAR_MIGRATION_H
#define DRW_DIMVAR_MIGRATION_H

namespace drwDimvarMigration {

struct ModernDimensionUnit {
    int linearUnit;
    int fraction;
};

struct ModernDimensionFit {
    int textArrowFit;
    int textMove;
};

// Autodesk's documented AcDmUtil conversion tables. Unsupported legacy enum
// values use the same documented fallback as the source API; callers that need
// a lossless downgrade use the strict inverse helpers below.
inline ModernDimensionUnit fromLegacyUnit(int legacy) noexcept {
    static constexpr ModernDimensionUnit mapping[] = {
        {2, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0},
        {5, 0}, {4, 2}, {5, 2}, {6, 0}
    };
    return legacy >= 1 && legacy <= 8 ? mapping[legacy]
                                       : mapping[0];
}

inline ModernDimensionFit fromLegacyFit(int legacy) noexcept {
    static constexpr ModernDimensionFit mapping[] = {
        {0, 0}, {1, 0}, {2, 0}, {3, 0}, {3, 1}, {3, 2}
    };
    return legacy >= 0 && legacy <= 5 ? mapping[legacy]
                                      : ModernDimensionFit{3, 0};
}

// Return true only when the modern tuple round-trips through a legacy value
// exactly. In particular, modern combinations with no legacy equivalent must
// not be silently coerced during a downgrade.
inline bool toLegacyUnit(int linearUnit, int fraction, int& legacy) noexcept {
    for (int candidate = 1; candidate <= 8; ++candidate) {
        const ModernDimensionUnit mapped = fromLegacyUnit(candidate);
        if (mapped.linearUnit == linearUnit && mapped.fraction == fraction) {
            legacy = candidate;
            return true;
        }
    }
    return false;
}

inline bool toLegacyFit(int textArrowFit, int textMove,
                        int& legacy) noexcept {
    for (int candidate = 0; candidate <= 5; ++candidate) {
        const ModernDimensionFit mapped = fromLegacyFit(candidate);
        if (mapped.textArrowFit == textArrowFit
            && mapped.textMove == textMove) {
            legacy = candidate;
            return true;
        }
    }
    return false;
}

inline bool isLegacyUnit(int value) noexcept {
    return value >= 1 && value <= 8;
}

inline bool isLegacyFit(int value) noexcept {
    return value >= 0 && value <= 5;
}

} // namespace drwDimvarMigration

#endif // DRW_DIMVAR_MIGRATION_H
