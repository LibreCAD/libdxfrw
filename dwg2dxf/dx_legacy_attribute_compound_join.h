// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_LEGACY_ATTRIBUTE_COMPOUND_JOIN_H
#define DX_LEGACY_ATTRIBUTE_COMPOUND_JOIN_H
#include "dx_legacy_attribute_compound_profile.h"
#include "dx_legacy_identity_join.h"

// The caller must independently authenticate both source publications and
// the identity census, bind the exact read/codec context and replayed model,
// and precharge index construction, lookup, spans and all lexical comparisons.
// This source-only helper never creates generic membership or output authority.
namespace dx_legacy_attribute_compound_join {
using Identity = DRW_DxfIdentitySource;
using namespace dx_legacy_attribute_compound_profile;
struct Result {
  Roles root, attribute, footer;
  std::size_t ownerRecord{Identity::NoIndex};
};
inline bool span(const DRW_DxfPhysicalRecordSpan &a,
                 const DRW_DxfPhysicalRecordSpan &b) noexcept {
  return a.ordinal == b.ordinal && a.beginOffset == b.beginOffset &&
         a.endOffset == b.endOffset && a.ignoredComment == b.ignoredComment;
}
inline bool join(const DRW_DxfEntitiesSource &physical,
                 const DRW_DxfEntitiesDeliverySource &delivery,
                 const Identity &census,
                 const dx_legacy_identity_join::Index &index, std::size_t first,
                 std::size_t dispatch, Result &out) noexcept {
  out = {};
  if (physical.status != DRW_DxfEntitiesSource::Status::Complete ||
      delivery.status != DRW_DxfEntitiesDeliverySource::Status::Complete ||
      census.status != Identity::Status::Complete ||
      physical.version != DRW::AC1018 || delivery.version != physical.version ||
      census.version != physical.version ||
      physical.binary != delivery.binary || physical.binary != census.binary ||
      physical.scopeId != delivery.scopeId || !physical.scopeId ||
      !span(physical.beginMarker, delivery.beginMarker) ||
      !span(physical.endMarker, delivery.endMarker) ||
      first > physical.records.size() || physical.records.size() - first < 3u ||
      dispatch >= delivery.dispatches.size())
    return false;
  const auto &a = physical.records[first];
  const auto &b = physical.records[first + 1u];
  const auto &c = physical.records[first + 2u];
  if (!root(a, physical.version, out.root) ||
      !attribute(b, physical.version, out.attribute) ||
      !footer(c, physical.version, out.footer) ||
      out.root.self == out.attribute.self || out.root.self == out.footer.self ||
      out.attribute.self == out.footer.self ||
      out.root.owner == out.attribute.self ||
      out.root.owner == out.footer.self ||
      out.attribute.owner != out.root.self || out.footer.owner != out.root.self)
    return false;
  const auto &d = delivery.dispatches[dispatch];
  const auto &after = first + 3u < physical.records.size()
                          ? physical.records[first + 3u].marker
                          : physical.endMarker;
  if (!span(d.begin, a.marker) || !span(d.end, after) || d.events != 1u ||
      d.firstEvent >= delivery.events.size())
    return false;
  const auto &event = delivery.events[d.firstEvent];
  if (event.dispatch != dispatch ||
      event.callback != DRW_DxfEntitiesCallback::addInsert ||
      event.origin != DRW_DxfEntitiesDeliveryEvent::Origin::Parser ||
      !event.hasValue ||
      event.entityType != static_cast<std::int32_t>(DRW::INSERT) ||
      event.handle != out.root.self || event.owner != out.root.owner)
    return false;
  const auto *ar = dx_legacy_identity_join::record(
      census, index, out.root.self, Identity::Kind::Entity,
      Identity::Section::Entities, "INSERT", 0u);
  const auto *br = dx_legacy_identity_join::record(
      census, index, out.attribute.self, Identity::Kind::Entity,
      Identity::Section::Entities, "ATTRIB", 0u);
  const auto *cr = dx_legacy_identity_join::record(
      census, index, out.footer.self, Identity::Kind::Entity,
      Identity::Section::Entities, "SEQEND", 0u);
  const auto *owner = dx_legacy_identity_join::record(
      census, index, out.root.owner, Identity::Kind::TableRecord,
      Identity::Section::Tables, "BLOCK_RECORD", 0u);
  if (!ar || !br || !cr || !owner || ar->section != br->section ||
      ar->section != cr->section || br - ar != 1 || cr - br != 1 ||
      owner->table >= census.records.size())
    return false;
  const auto &table = census.records[owner->table];
  if (table.kind != Identity::Kind::TableControl ||
      table.section != owner->section ||
      !dx_legacy_identity_join::sameName(table.tableName, "BLOCK_RECORD"))
    return false;
  const auto sameRecord = [&](const Record &r, const Identity::Record &row,
                              std::uint32_t self) noexcept {
    const auto found = index.ids.find(self);
    if (found == index.ids.end() ||
        found->second.candidate >= census.candidates.size())
      return false;
    const auto &candidate = census.candidates[found->second.candidate];
    if (!span(r.marker, row.marker) || r.endOrdinal != row.endMarker.ordinal ||
        r.endOffset != row.endMarker.beginOffset ||
        !span(r.spans[0], candidate.span) || !r.groups[0].content.s ||
        *r.groups[0].content.s != candidate.lexeme)
      return false;
    return physical.binary || r.rawValues[0] == candidate.rawValue;
  };
  if (!sameRecord(a, *ar, out.root.self) ||
      !sameRecord(b, *br, out.attribute.self) ||
      !sameRecord(c, *cr, out.footer.self) ||
      a.endOrdinal != b.marker.ordinal || a.endOffset != b.marker.beginOffset ||
      b.endOrdinal != c.marker.ordinal || b.endOffset != c.marker.beginOffset ||
      c.endOrdinal != after.ordinal || c.endOffset != after.beginOffset)
    return false;
  out.ownerRecord = static_cast<std::size_t>(owner - census.records.data());
  return true;
}
} // namespace dx_legacy_attribute_compound_join
#endif
