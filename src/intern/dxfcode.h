#ifndef DXFCODE_H
#define DXFCODE_H

#include <cstdint>

// One canonical DXF group-code classifier shared by ASCII/Binary readers and
// raw-capture validation. Codes 80-89, 150-159, 180-209, 240-269, and
// 482-998 have no general type assigned in the published Autodesk tables, so
// the standalone-safe
// profile preserves them as unknown in ASCII and fails closed for binary
// input rather than guessing a width. Explicit legacy profiles retain only
// the historical interpretations needed for compatibility probes.
enum class DxfValueKind { Str, Dbl, I16, I32, I64, Bln, Bin, Unknown };

// Handle spelling does not determine reference semantics. In particular,
// 320-329 are application values taken as-is during INSERT/XREF/WBLOCK;
// they must never follow a source-object or structural-collision remap.
enum class DxfHandleRole {
    None,
    Self,
    Arbitrary,
    SoftPointer,
    HardPointer,
    SoftOwner,
    HardOwner
};

inline constexpr DxfHandleRole dxfHandleRoleForCode(int code) {
    if (code == 5 || code == 105)
        return DxfHandleRole::Self;
    if (code >= 320 && code <= 329)
        return DxfHandleRole::Arbitrary;
    if ((code >= 330 && code <= 339) || code == 1005)
        return DxfHandleRole::SoftPointer;
    if ((code >= 340 && code <= 349) || (code >= 390 && code <= 399)
        || (code >= 480 && code <= 481))
        return DxfHandleRole::HardPointer;
    if (code >= 350 && code <= 359)
        return DxfHandleRole::SoftOwner;
    if (code >= 360 && code <= 369)
        return DxfHandleRole::HardOwner;
    return DxfHandleRole::None;
}

inline constexpr bool isDxfHandleCode(int code) {
    return dxfHandleRoleForCode(code) != DxfHandleRole::None;
}

inline constexpr bool isDxfTranslatedHandleCode(int code) {
    const DxfHandleRole role = dxfHandleRoleForCode(code);
    return role != DxfHandleRole::None && role != DxfHandleRole::Arbitrary;
}

// Autodesk XRECORD schema: application payload uses normal object groups
// 1..369 except the self handles. Trailing XDATA is a separate common channel.
// General kind/profile validation still applies within this envelope.
inline constexpr bool isDxfXRecordApplicationCode(int code) {
    return code >= 1 && code <= 369 && code != 5 && code != 105;
}

// The pinned LibreCAD tree predates the standalone safety correction for two
// disputed ranges.  Keep that legacy interpretation available to an explicit
// compatibility probe without changing the standalone-safe default used by
// readers, capture, validation, and replay.
enum class DxfClassifierProfile {
    StandaloneSafe,
    LibreCadMasterLegacy
};

struct DxfCodeRange {
    int lo;
    int hi;
    DxfValueKind kind;
};

inline constexpr DxfCodeRange kDxfCodeRanges[] = {
    {   0,    9, DxfValueKind::Str},
    {  10,   59, DxfValueKind::Dbl},
    {  60,   79, DxfValueKind::I16},
    {  80,   89, DxfValueKind::Unknown},
    {  90,   99, DxfValueKind::I32},
    { 100,  109, DxfValueKind::Str},
    { 110,  149, DxfValueKind::Dbl},
    { 150,  159, DxfValueKind::Unknown},
    { 160,  169, DxfValueKind::I64},
    { 170,  179, DxfValueKind::I16},
    { 180,  209, DxfValueKind::Unknown},
    { 210,  239, DxfValueKind::Dbl},
    { 240,  259, DxfValueKind::Unknown},
    { 260,  269, DxfValueKind::Unknown},
    { 270,  289, DxfValueKind::I16},
    { 290,  299, DxfValueKind::Bln},
    { 300,  309, DxfValueKind::Str},
    { 310,  319, DxfValueKind::Bin},
    { 320,  369, DxfValueKind::Str},
    { 370,  389, DxfValueKind::I16},
    { 390,  399, DxfValueKind::Str},
    { 400,  409, DxfValueKind::I16},
    { 410,  419, DxfValueKind::Str},
    { 420,  429, DxfValueKind::I32},
    { 430,  439, DxfValueKind::Str},
    { 440,  459, DxfValueKind::I32},
    { 460,  469, DxfValueKind::Dbl},
    { 470,  481, DxfValueKind::Str},
    { 482,  998, DxfValueKind::Unknown},
    { 999, 1003, DxfValueKind::Str},
    {1004, 1004, DxfValueKind::Bin},
    {1005, 1005, DxfValueKind::Str},
    {1006, 1009, DxfValueKind::Unknown},
    {1010, 1059, DxfValueKind::Dbl},
    {1060, 1070, DxfValueKind::I16},
    {1071, 1071, DxfValueKind::I32},
};

inline constexpr DxfValueKind dxfValueKindForCode(int code) {
    for (const DxfCodeRange& range : kDxfCodeRanges) {
        if (code >= range.lo && code <= range.hi)
            return range.kind;
    }
    return DxfValueKind::Unknown;
}

inline constexpr DxfValueKind dxfValueKindForCode(
    int code, DxfClassifierProfile profile) {
    if (profile == DxfClassifierProfile::LibreCadMasterLegacy) {
        if ((code >= 80 && code <= 89)
            || (code >= 150 && code <= 159)
            || (code >= 180 && code <= 209))
            return DxfValueKind::Str;
        if (code >= 240 && code <= 259)
            return DxfValueKind::Dbl;
        if (code >= 260 && code <= 269)
            return DxfValueKind::Bln;
        if (code >= 482 && code <= 998)
            return DxfValueKind::Dbl;
        if (code >= 1006 && code <= 1009)
            return DxfValueKind::Str;
    }
    return dxfValueKindForCode(code);
}

// Keep the table a complete, ordered description of the DXF group-code
// domain. Unknown spans must not disappear from one reader/capture/replay path
// merely because their values are opaque. A compile-time invariant catches
// accidental gaps or overlaps when a range is edited.
inline constexpr bool dxfCodeRangesAreCanonical() {
    constexpr unsigned count =
        static_cast<unsigned>(sizeof(kDxfCodeRanges)
                              / sizeof(kDxfCodeRanges[0]));
    if (count == 0 || kDxfCodeRanges[0].lo != 0
        || kDxfCodeRanges[count - 1].hi != 1071) {
        return false;
    }
    for (unsigned index = 1; index < count; ++index) {
        if (kDxfCodeRanges[index - 1].hi + 1
            != kDxfCodeRanges[index].lo) {
            return false;
        }
    }
    return true;
}

static_assert(dxfCodeRangesAreCanonical(),
              "DXF group-code classifier ranges must be contiguous");

inline constexpr bool isDxfBinaryChunkCode(int code) {
    return (code >= 310 && code <= 319) || code == 1004;
}

#endif // DXFCODE_H
