#ifndef DXFCODE_H
#define DXFCODE_H

#include <cstdint>

// One canonical DXF group-code classifier shared by ASCII/Binary readers and
// raw-capture validation.  Codes 260-269 are 32-bit integers (not booleans),
// preserving non-zero values such as 2.  The unassigned 482-998 span is
// deliberately Unknown: ASCII callers may retain its spelling, while binary
// readers fail closed instead of guessing a floating-point representation.
enum class DxfValueKind { Str, Dbl, I16, I32, I64, Bln, Bin, Unknown };

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
    {  80,   89, DxfValueKind::Str},
    {  90,   99, DxfValueKind::I32},
    { 100,  109, DxfValueKind::Str},
    { 110,  149, DxfValueKind::Dbl},
    { 150,  159, DxfValueKind::Str},
    { 160,  169, DxfValueKind::I64},
    { 170,  179, DxfValueKind::I16},
    { 180,  209, DxfValueKind::Str},
    { 210,  259, DxfValueKind::Dbl},
    { 260,  269, DxfValueKind::I32},
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
    {1005, 1008, DxfValueKind::Str},
    {1009, 1059, DxfValueKind::Dbl},
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
        if (code >= 260 && code <= 269)
            return DxfValueKind::Bln;
        if (code >= 482 && code <= 998)
            return DxfValueKind::Dbl;
    }
    return dxfValueKindForCode(code);
}

// Keep the table a complete, ordered description of the DXF group-code
// domain.  The 482-998 span is intentionally present as Unknown; it must not
// disappear from one reader/capture/replay path merely because its values are
// opaque.  A compile-time invariant catches accidental gaps or overlaps when
// a range is edited.
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
