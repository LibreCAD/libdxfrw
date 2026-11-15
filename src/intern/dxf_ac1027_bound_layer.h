// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef LIBDXFRW_DXF_AC1027_BOUND_LAYER_H
#define LIBDXFRW_DXF_AC1027_BOUND_LAYER_H
#include "../drw_objects.h"
// Private, explicit adapter-to-writer profile. Ordinary callers never acquire
// this marker and retain their historical group-390 fallback.
struct DRW_Ac1027BoundLayer final : DRW_Layer {
    DRW_Ac1027BoundLayer(const DRW_Layer& source, std::uint32_t plot)
        : DRW_Layer(source), sourcePlotStyle(plot) {}
    std::uint32_t sourcePlotStyle;
};
// AC1024 is a separate opt-in profile: adapter admission and final graph
// closure must still be checked before publishing a converted document.
struct DRW_Ac1024BoundLayer final : DRW_Layer {
    DRW_Ac1024BoundLayer(const DRW_Layer& source, std::uint32_t plot)
        : DRW_Layer(source), sourcePlotStyle(plot) {}
    std::uint32_t sourcePlotStyle;
};
struct DRW_Ac1027BoundDictionary final : DRW_DictionaryWithDefault {
    explicit DRW_Ac1027BoundDictionary(const DRW_DictionaryWithDefault& source)
        : DRW_DictionaryWithDefault(source) {}
};
#endif
