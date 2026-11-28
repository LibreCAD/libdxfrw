// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXF_STYLE_FONT_H
#define DXF_STYLE_FONT_H

#include "../drw_base.h"
#include <limits>
#include <string_view>
#include <vector>

namespace dxfStyleFont {
inline bool isAcad(std::string_view name) noexcept {
    return name.size()==4u&&(name[0]=='A'||name[0]=='a')&&(name[1]=='C'||name[1]=='c')
        &&(name[2]=='A'||name[2]=='a')&&(name[3]=='D'||name[3]=='d');
}
inline bool acad(const DRW_Variant* value) noexcept {
    return value&&value->code()==1001&&value->type()==DRW_Variant::STRING&&value->content.s&&isAcad(*value->content.s);
}
inline bool fontName(const DRW_Variant* value) noexcept {
    return value&&value->code()==1000&&value->type()==DRW_Variant::STRING&&value->content.s;
}
// A bounded lookbehind in the existing model sequence; no parser/model layout
// state, backward scan or allocation. Preserve the legacy bare-ACAD first flag
// projection, but only the named-font prefix has the independently selected
// source profile. Every XDATA integer remains application data as well.
inline bool prefix(const std::vector<DRW_Variant*>& values,std::size_t end) noexcept {
    return end<=values.size()&&((end&&acad(values[end-1u]))
        ||(end>=2u&&acad(values[end-2u])&&fontName(values[end-1u])));
}
inline const DRW_Variant* flags(const std::vector<DRW_Variant*>& values) noexcept {
    for(std::size_t i=0u;i<values.size();++i) {
        const auto* value=values[i];
        if(!value||value->code()!=1071||!prefix(values,i))continue;
        if(value->type()==DRW_Variant::INTEGER||(value->type()==DRW_Variant::INTEGER64
            &&value->i64_val()>=std::numeric_limits<std::int32_t>::min()&&value->i64_val()<=std::numeric_limits<std::int32_t>::max()))return value;
    }
    return nullptr;
}
}
#endif
