// SPDX-License-Identifier: GPL-2.0-or-later
// Internal source-only relation helper. The caller must
// authenticate the whole-read source, census, structural raw/model and exact
// read/codec context, and precharge all field and string work before calling.
#pragma once
#include "intern/dxfwholesource.h"
#include "drw_interface.h"
#include "drw_objects.h"
#include <cstring>
#include <string_view>
namespace dx_mline_layer_body {
using Field=WholeSourceV2::Field;using Whole=WholeSourceV2::Source;using Census=DRW_DxfIdentitySource;
inline bool bounds(const Whole& whole,const Field& f) noexcept {return whole.bounds(f);}
inline bool marker(const Field& f,const DRW_DxfPhysicalRecordSpan& s) noexcept {
 return f.code==0&&f.kind==DxfReadRecordView::Text&&!f.ignored&&!s.ignoredComment
     &&f.ordinal==s.ordinal&&f.begin==s.beginOffset&&f.end==s.endOffset;
}
inline bool equal(const Whole& whole,const Census::Record& record,const DRW_RawDxfObject& raw) noexcept {
 if(whole.state()!=WholeSourceV2::State::ThroughEof||whole.version()!=DRW::AC1027||raw.m_version!=whole.version()
    ||raw.name!="LAYER"||raw.hasRawValues==whole.binary()||raw.groups.size()!=raw.rawValues.size()
    ||record.kind!=Census::Kind::TableRecord||record.name!="LAYER"||!record.marker.ordinal
    ||record.endMarker.ordinal<=record.marker.ordinal||record.endMarker.ordinal>whole.fieldCount())return false;
 const auto first=record.marker.ordinal-1u,last=record.endMarker.ordinal-1u;
 if(first>=whole.fieldCount()||last>=whole.fieldCount())return false;
 const auto& open=whole.field(first);const auto& end=whole.field(last);
 if(!bounds(whole,open)||!bounds(whole,end)||!marker(open,record.marker)||!marker(end,record.endMarker)
    ||whole.value(open)!="LAYER")return false;
 std::size_t group=0u,previousEnd=open.end;
 for(std::size_t i=first+1u;i<last;++i) {
  const auto& f=whole.field(i);
  if(!bounds(whole,f)||f.ordinal!=i+1u||f.code==0||f.begin!=previousEnd||f.end<=f.begin)return false;
  previousEnd=f.end;
  if(f.ignored){if(f.code!=999)return false;continue;}
  if (group >= raw.groups.size())
    return false;
  const auto &v = raw.groups[group];
  if(f.code!=v.code()||v.isLayerRef()||v.rawDwgLayerReference()!=0u||v.dwgCmColorValue())return false;
  if(!whole.binary()&&whole.raw(f)!=raw.rawValues[group])return false;
  if(whole.binary()&&!raw.rawValues[group].empty())return false;
  switch(f.kind){
   case DxfReadRecordView::Integer:if(v.type()!=DRW_Variant::INTEGER||v.i_val()!=f.integer)return false;break;
   case DxfReadRecordView::WideInteger:if(v.type()!=DRW_Variant::INTEGER64||v.i64_val()!=f.wide)return false;break;
   case DxfReadRecordView::Real:if(v.type()!=DRW_Variant::DOUBLE||std::memcmp(&v.content.d,&f.real,sizeof(double)))return false;break;
   case DxfReadRecordView::Text:case DxfReadRecordView::Binary:
     if (v.type() != DRW_Variant::STRING || !v.content.s ||
         *v.content.s != whole.raw(f))
       return false;
     break;
   default:return false;
  }
  ++group;
 }
 return group==raw.groups.size()&&previousEnd==end.begin;
}
}

namespace mline_layer_relation {
enum class Result { Match, Refused, Limit };
struct Limits {std::size_t fields=4096u,groups=4096u,work=64u*1024u*1024u;};
struct Proof {Result result=Result::Refused;std::size_t charged=0u;};
// Caller must authenticate all inputs as bound to one accepted read. This is
// a bounded body comparison only, not live/output/geometry authorization.
inline Proof compare(const WholeSourceV2::Source& whole,const DRW_DxfIdentitySource::Record& record,
                     const DRW_RawDxfObject& raw,Limits limits={}) noexcept {
 Proof p;std::size_t bytes=0u,fields=0u,total=0u;
 // At most two byte-sensitive equality comparisons per source field: captured
 // raw spelling and typed text against the whole source's captured pool. A
 // size mismatch short-circuits before reading an oversized caller string.
 // Scalar, bounds, ordinal and container steps are separately budgeted.
 if(whole.fieldCount()>limits.fields||raw.groups.size()>limits.groups
    ||!WholeSourceV2::product(whole.peakBytes(),2u,bytes)
    ||!WholeSourceV2::product(whole.fieldCount(),32u,fields)
    ||!WholeSourceV2::sum(bytes,fields,total)||!WholeSourceV2::sum(total,512u,total)
    ||total>limits.work){p.result=Result::Limit;return p;}
 p.charged=total;p.result=dx_mline_layer_body::equal(whole,record,raw)?Result::Match:Result::Refused;return p;
}
}
