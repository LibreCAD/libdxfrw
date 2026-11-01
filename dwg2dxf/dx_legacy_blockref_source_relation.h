// SPDX-License-Identifier: GPL-2.0-or-later
// Source-only relation. Caller must authenticate and bind all sources to the same
// accepted read, callback/codec context and immutable adapter owners first,
// and precharge the complete record, lexical scans and identity lookups.
#pragma once
#include "dx_legacy_blockrecord_whole_source.h"
#include "dx_legacy_block_record_source_profile.h"
#include "dx_legacy_attribute_compound_join.h"
namespace blockref_source_relation {
using Whole=WholeSourceV2::Source;using Census=DRW_DxfIdentitySource;
using Audit=dx_legacy_block_record_source::Audit;
struct Result {std::uint32_t record{0},insert{0};DRW_DxfPhysicalRecordSpan name,reference;};
inline DRW_DxfPhysicalRecordSpan span(const WholeSourceV2::Field& f) noexcept{return {f.ordinal,f.begin,f.end,f.ignored};}
inline bool find(const Whole& whole,const Census::Record& record,std::size_t index,WholeSourceV2::Field& out) noexcept {
 if(!record.marker.ordinal||record.endMarker.ordinal<=record.marker.ordinal||record.endMarker.ordinal>whole.fieldCount())return false;
 std::size_t group=0u;for(std::size_t i=record.marker.ordinal;i<record.endMarker.ordinal-1u;++i){const auto f=whole.field(i);
   if (f.ignored)
     continue;
   if (group++ == index) {
     out = f;
     return true;
   }
 }
 return false;
}
inline bool equalSpan(const DRW_DxfPhysicalRecordSpan& a,const DRW_DxfPhysicalRecordSpan& b)noexcept {
 return a.ordinal==b.ordinal&&a.beginOffset==b.beginOffset&&a.endOffset==b.endOffset&&a.ignoredComment==b.ignoredComment;
}
inline bool match(const Whole& whole,const Census& census,const dx_legacy_identity_join::Index& index,
                  const DRW_RawDxfObject& raw,const Audit& profile,
                  const dx_legacy_attribute_compound_join::Result& entity,Result& out) noexcept {
 out={};if(census.status!=Census::Status::Complete||census.version!=whole.version()||census.binary!=whole.binary()
   ||!profile.complete()||profile.spans.size()!=raw.groups.size()||!raw.handle||!entity.root.self||entity.root.block.empty())return false;
 const auto* record=dx_legacy_identity_join::record(census,index,raw.handle,Census::Kind::TableRecord,Census::Section::Tables,"BLOCK_RECORD",0u);
 if(!record||!raw.parentHandle||!blockrecord_body_match::equal(whole,*record,raw))return false;
 const auto* table=dx_legacy_identity_join::record(census,index,raw.parentHandle,Census::Kind::TableControl,Census::Section::Tables,"TABLE",0u);
 if(!table||record->table!=static_cast<std::size_t>(table-census.records.data())||record->section!=table->section
    ||!dx_legacy_identity_join::sameName(table->tableName,"BLOCK_RECORD"))return false;
 const auto hit=index.ids.find(raw.handle);if(hit==index.ids.end()||hit->second.candidate>=census.candidates.size())return false;
 WholeSourceV2::Field self{};if(!find(whole,*record,0u,self)||!equalSpan(span(self),census.candidates[hit->second.candidate].span))return false;
 std::size_t nameIndex=Census::NoIndex,insertIndex=Census::NoIndex;
 for(std::size_t i=0u;i<profile.spans.size();++i){const auto& x=profile.spans[i];
  if(x.index!=i||x.code!=raw.groups[i].code())return false;
  if(x.role==dx_legacy_block_record_source::Role::BlockName){if(nameIndex!=Census::NoIndex||x.field!=dx_legacy_block_record_source::Field::Name||x.code!=2)return false;nameIndex=i;}
  if(x.role==dx_legacy_block_record_source::Role::BlockInsert){if(insertIndex!=Census::NoIndex||x.field!=dx_legacy_block_record_source::Field::Application||x.code!=331||x.handle!=entity.root.self)return false;insertIndex=i;}
 }
 if(nameIndex==Census::NoIndex||insertIndex==Census::NoIndex||raw.groups[nameIndex].type()!=DRW_Variant::STRING||!raw.groups[nameIndex].content.s
    ||!dx_legacy_block_record_source::sameAsciiName(*raw.groups[nameIndex].content.s,entity.root.block))return false;
 WholeSourceV2::Field name{},reference{};
 if(!find(whole,*record,nameIndex,name)||!find(whole,*record,insertIndex,reference))return false;
 out.record=raw.handle;out.insert=entity.root.self;out.name=span(name);out.reference=span(reference);return true;
}
}
