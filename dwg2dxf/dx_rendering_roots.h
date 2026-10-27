// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_RENDERING_ROOTS_H
#define DX_RENDERING_ROOTS_H
#include "drw_objects.h"
#include <string_view>
namespace dx_rendering_roots {
inline bool equal(std::string_view a,std::string_view b) noexcept {
    if(a.size()!=b.size())return false;
    for(std::size_t i=0u;i<a.size();++i){const auto c=a[i]>='a'&&a[i]<='z'?static_cast<char>(a[i]-'a'+'A'):a[i];if(c!=b[i])return false;}
    return true;
}
inline bool known(std::string_view key) noexcept {
    return equal(key,"ACAD_RENDER_SETTINGS")||equal(key,"ACAD_RENDER_RAPIDRT_SETTINGS")||equal(key,"ACAD_RENDER_PLOT_SETTINGS");
}
inline bool allows(std::string_view key,DRW_RenderSettings::Kind kind) noexcept {
    if(equal(key,"ACAD_RENDER_RAPIDRT_SETTINGS"))return kind==DRW_RenderSettings::RapidRT;
    if(equal(key,"ACAD_RENDER_SETTINGS"))return kind==DRW_RenderSettings::Settings||kind==DRW_RenderSettings::MentalRay;
    // Retain the established plotting-preset replay path for both engines.
    return equal(key,"ACAD_RENDER_PLOT_SETTINGS")&&(kind==DRW_RenderSettings::RapidRT||kind==DRW_RenderSettings::MentalRay);
}
}
#endif
