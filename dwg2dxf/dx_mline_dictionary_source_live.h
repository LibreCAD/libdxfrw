// SPDX-License-Identifier: GPL-2.0-or-later
// Internal source-only relation helper. The caller authenticates the whole
// read, structural audit, physical view and read context before use.
#pragma once
#include "intern/dxfwholesource.h"
#include "drw_interface.h"
#include <charconv>
#include <cstring>
#include <string_view>

namespace dx_mline_dictionary_source_live {
using Whole = WholeSourceV2::Source;
using Census = DRW_DxfIdentitySource;
using Physical = DRW_DxfObjectPhysicalSource;

inline bool same(const DRW_DxfPhysicalRecordSpan& a,
                 const DRW_DxfPhysicalRecordSpan& b) noexcept {
  return a.ordinal == b.ordinal && a.beginOffset == b.beginOffset &&
         a.endOffset == b.endOffset && a.ignoredComment == b.ignoredComment;
}

inline bool parseHandle(std::string_view text, std::uint32_t& value) noexcept {
  value = 0u;
  if (text.empty() || text.size() > 8u)
    return false;
  const auto result = std::from_chars(text.data(), text.data() + text.size(),
                                      value, 16);
  return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}

inline bool equal(const Whole& whole, const Census::Record& record,
                  const std::shared_ptr<const Physical>& physical,
                  Physical::Status status,
                  const DRW_RawDxfObject& raw) noexcept {
  if (whole.state() != WholeSourceV2::State::ThroughEof ||
      whole.version() != DRW::AC1027 || record.kind != Census::Kind::Object ||
      record.name != "DICTIONARY" || status != Physical::Status::Complete ||
      !physical || physical->status != status ||
      physical->version != whole.version() ||
      physical->binary != whole.binary() || physical->name != "DICTIONARY" ||
      raw.name != "DICTIONARY" || raw.m_version != whole.version() ||
      raw.hasRawValues == whole.binary() ||
      raw.groups.size() != raw.rawValues.size() || !record.marker.ordinal ||
      record.endMarker.ordinal <= record.marker.ordinal ||
      !same(physical->marker, record.marker) ||
      !same(physical->endMarker, record.endMarker) ||
      record.endMarker.ordinal - record.marker.ordinal - 1u !=
          physical->fields.size())
    return false;

  std::size_t group = 0u;
  unsigned selfCount = 0u, ownerCount = 0u;
  std::uint32_t self = 0u, owner = 0u;
  std::size_t ordinal = record.marker.ordinal + 1u;
  std::size_t priorEnd = record.marker.endOffset;
  for (const auto& field : physical->fields) {
    const auto& span = field.span;
    if (field.code <= 0 || span.ordinal != ordinal++ ||
        span.beginOffset < priorEnd || span.endOffset <= span.beginOffset ||
        span.endOffset > record.endMarker.beginOffset ||
        span.ordinal > whole.fieldCount())
      return false;
    priorEnd = span.endOffset;
    const auto source = whole.field(span.ordinal - 1u);
    if (!whole.bounds(source) || source.ordinal != span.ordinal ||
        source.begin != span.beginOffset || source.end != span.endOffset ||
        source.code != field.code)
      return false;
    if (span.ignoredComment) {
      if (field.code != 999)
        return false;
      continue;
    }
    if (group >= raw.groups.size())
      return false;
    const auto& value = raw.groups[group];
    if (value.code() != field.code || value.isLayerRef() ||
        value.rawDwgLayerReference() != 0u || value.dwgCmColorValue())
      return false;
    if (!whole.binary() &&
        (raw.rawValues[group].size() > whole.peakBytes() ||
         whole.raw(source) != raw.rawValues[group]))
      return false;
    if (whole.binary() && !raw.rawValues[group].empty())
      return false;
    switch (source.kind) {
    case DxfReadRecordView::Integer:
      if (value.type() != DRW_Variant::INTEGER ||
          value.i_val() != source.integer)
        return false;
      break;
    case DxfReadRecordView::WideInteger:
      if (value.type() != DRW_Variant::INTEGER64 ||
          value.i64_val() != source.wide)
        return false;
      break;
    case DxfReadRecordView::Real:
      if (value.type() != DRW_Variant::DOUBLE ||
          std::memcmp(&value.content.d, &source.real, sizeof(double)) != 0)
        return false;
      break;
    case DxfReadRecordView::Text:
      if (value.type() != DRW_Variant::STRING || !value.content.s ||
          value.content.s->size() > whole.peakBytes() ||
          *value.content.s != whole.raw(source))
        return false;
      break;
    default:
      return false;
    }
    if (field.code == 5) {
      if (++selfCount != 1u ||
          !parseHandle(whole.value(source), self))
        return false;
    } else if (field.code == 330) {
      if (++ownerCount != 1u ||
          !parseHandle(whole.value(source), owner))
        return false;
    }
    ++group;
  }
  return group == raw.groups.size() && selfCount == 1u && self == raw.handle &&
         ownerCount <= 1u && (ownerCount == 0u ? raw.parentHandle == 0u
                                               : owner == raw.parentHandle) &&
         priorEnd == record.endMarker.beginOffset;
}
} // namespace dx_mline_dictionary_source_live
