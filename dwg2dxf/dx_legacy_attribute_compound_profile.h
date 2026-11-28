// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_LEGACY_ATTRIBUTE_COMPOUND_PROFILE_H
#define DX_LEGACY_ATTRIBUTE_COMPOUND_PROFILE_H
#include "drw_interface.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <string_view>

// A narrow source-field inventory, not a compound membership, target,
// graph, native-semantic or output certificate. The caller must charge all
// field and string scans, independently authenticate the physical owner and
// delivery, and bind original self/owner identities before using these roles.
namespace dx_legacy_attribute_compound_profile {
using Record = DRW_DxfPhysicalEntitySource;
struct Roles {
  std::uint32_t self{0}, owner{0};
  std::string_view layer, block, tag, appId;
};
inline bool text(const DRW_Variant &v, std::string_view &out,
                 bool ascii = true) noexcept {
  if (v.type() != DRW_Variant::STRING || !v.content.s || v.content.s->empty())
    return false;
  out = *v.content.s;
  for (unsigned char c : out)
    if (c == 0u || (ascii && c >= 128u))
      return false;
  return true;
}
inline bool identity(const DRW_Variant &v, std::uint32_t &out) noexcept {
  std::string_view spelling;
  if (!text(v, spelling))
    return false;
  std::uint64_t number = 0u;
  const auto parsed = std::from_chars(
      spelling.data(), spelling.data() + spelling.size(), number, 16);
  if (parsed.ec != std::errc{} ||
      parsed.ptr != spelling.data() + spelling.size() || !number ||
      number > UINT32_MAX)
    return false;
  out = static_cast<std::uint32_t>(number);
  return true;
}
inline bool real(const DRW_Variant &v) noexcept {
  return v.type() == DRW_Variant::DOUBLE && std::isfinite(v.d_val());
}
inline bool integer(const DRW_Variant &v, int low, int high) noexcept {
  return v.type() == DRW_Variant::INTEGER && v.i_val() >= low &&
         v.i_val() <= high;
}
template <std::size_t N>
inline bool shape(const Record &r, std::string_view name,
                  const std::array<int, N> &codes) noexcept {
  if (r.name != name || r.groups.size() != N || r.spans.size() != N ||
      r.rawValues.size() != N)
    return false;
  for (std::size_t i = 0u; i < N; ++i)
    if (r.groups[i].code() != codes[i] || r.spans[i].ignoredComment ||
        r.groups[i].isLayerRef() || r.groups[i].rawDwgLayerReference() != 0u ||
        r.groups[i].dwgCmColorValue())
      return false;
  return true;
}
inline bool root(const Record &r, DRW::Version version, Roles &out) noexcept {
  out = {};
  if (version != DRW::AC1018)
    return false;
  constexpr std::array<int, 13> codes{
      {5, 330, 100, 8, 100, 66, 2, 10, 20, 30, 41, 42, 43}};
  if (!shape(r, "INSERT", codes))
    return false;
  const auto &g = r.groups;
  std::string_view marker;
  if (!identity(g[0], out.self) || !identity(g[1], out.owner) ||
      out.self == out.owner || !text(g[2], marker) || marker != "AcDbEntity" ||
      !text(g[3], out.layer) || !text(g[4], marker) ||
      marker != "AcDbBlockReference" || !integer(g[5], 1, 1) ||
      !text(g[6], out.block))
    return false;
  for (std::size_t i = 7u; i < g.size(); ++i)
    if (!real(g[i]))
      return false;
  return true;
}
inline bool attribute(const Record &r, DRW::Version version,
                      Roles &out) noexcept {
  out = {};
  if (version != DRW::AC1018)
    return false;
  constexpr std::array<int, 16> codes{
      {5, 330, 100, 8, 100, 10, 20, 30, 40, 1, 100, 2, 70, 1001, 1070, 1070}};
  if (!shape(r, "ATTRIB", codes))
    return false;
  const auto &g = r.groups;
  std::string_view marker, value;
  if (!identity(g[0], out.self) || !identity(g[1], out.owner) ||
      out.self == out.owner || !text(g[2], marker) || marker != "AcDbEntity" ||
      !text(g[3], out.layer) || !text(g[4], marker) || marker != "AcDbText")
    return false;
  for (std::size_t i = 5u; i <= 8u; ++i)
    if (!real(g[i]))
      return false;
  if (g[8].d_val() <= 0.0 || !text(g[9], value) || !text(g[10], marker) ||
      marker != "AcDbAttribute" || !text(g[11], out.tag) ||
      !integer(g[12], 0, 0) || !text(g[13], out.appId) ||
      out.appId != "AcDbAttr" || !integer(g[14], -32768, 32767) ||
      !integer(g[15], -32768, 32767))
    return false;
  return true;
}
inline bool footer(const Record &r, DRW::Version version, Roles &out) noexcept {
  out = {};
  if (version != DRW::AC1018)
    return false;
  constexpr std::array<int, 4> codes{{5, 330, 100, 8}};
  if (!shape(r, "SEQEND", codes))
    return false;
  const auto &g = r.groups;
  std::string_view marker;
  return identity(g[0], out.self) && identity(g[1], out.owner) &&
         out.self != out.owner && text(g[2], marker) &&
         marker == "AcDbEntity" && text(g[3], out.layer);
}
} // namespace dx_legacy_attribute_compound_profile
#endif
