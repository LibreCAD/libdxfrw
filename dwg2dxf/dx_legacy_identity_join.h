// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_LEGACY_IDENTITY_JOIN_H
#define DX_LEGACY_IDENTITY_JOIN_H
#include "drw_interface.h"
#include <charconv>
#include <cstdint>
#include <map>
#include <string_view>
#include <system_error>
#include <vector>

// Original selected self identities only. This is not a namespace, target,
// graph or output certificate; unselected records do not become qualified.
namespace dx_legacy_identity_join {
using Source=DRW_DxfIdentitySource;
struct Hit { std::size_t record{Source::NoIndex},candidate{Source::NoIndex}; bool ambiguous{false}; };
struct Index {std::map<std::uint64_t,Hit> ids;std::vector<std::uint8_t> selfCounts;};
inline bool hex(std::string_view value,std::uint64_t& out) noexcept {
    if(value.empty())return false;
    // from_chars accepts arbitrarily many leading zeroes. The entire lexical
    // scan is precharged by the caller, even if the 64-bit value overflows.
    const auto p=std::from_chars(value.data(),value.data()+value.size(),out,16);
    return p.ec==std::errc{}&&p.ptr==value.data()+value.size();
}
inline bool sameName(std::string_view x,std::string_view y) noexcept {
    if(x.size()!=y.size())return false;
    for(std::size_t i=0;i<x.size();++i){const auto upper=[](unsigned char c){return c>='a'&&c<='z'?static_cast<unsigned char>(c-('a'-'A')):c;};
        if(upper(static_cast<unsigned char>(x[i]))!=upper(static_cast<unsigned char>(y[i])))return false;}
    return true;
}
inline bool spanInside(const DRW_DxfPhysicalRecordSpan& inner,const DRW_DxfPhysicalRecordSpan& first,const DRW_DxfPhysicalRecordSpan& last) noexcept {
    return inner.ordinal>first.ordinal&&inner.ordinal<last.ordinal&&inner.beginOffset>=first.endOffset
        &&inner.beginOffset<inner.endOffset&&inner.endOffset<=last.beginOffset&&!inner.ignoredComment;
}
// Caller charges vector/map storage and O(n log n) plus all lexical scans
// before calling. Each record's self-field count is computed once here.
inline bool buildIndex(const Source& source,Index& out) {
    if(source.status!=Source::Status::Complete||!source.eofMarker.ordinal||source.unmatchedAdmissions)return false;
    out.selfCounts.assign(source.records.size(),0u);
    for(std::size_t i=0u;i<source.candidates.size();++i){const auto& c=source.candidates[i];
        if(c.code!=5&&c.code!=105)return false;
        if(c.scope==Source::Scope::Application||c.scope==Source::Scope::Xdata||c.scope==Source::Scope::Embedded
            ||c.scope==Source::Scope::OpaqueBody||c.scope==Source::Scope::SortOrder)continue;
        if(c.scope!=Source::Scope::Outer&&c.scope!=Source::Scope::Unframed)return false;
        bool valid=c.scope==Source::Scope::Outer&&c.record<source.records.size()&&c.section<source.sections.size();
        if(c.scope==Source::Scope::Outer) {
            if(!valid)return false;const auto& r=source.records[c.record];
            if(r.section!=c.section||r.firstCandidate>source.candidates.size()
                ||r.candidates>source.candidates.size()-r.firstCandidate||i<r.firstCandidate||i-r.firstCandidate>=r.candidates
                ||!spanInside(c.span,r.marker,r.endMarker))return false;
            if(r.kind==Source::Kind::TableRecord&&sameName(r.name,"DIMSTYLE")&&c.code==5)continue;
            if(out.selfCounts[c.record]<2u)++out.selfCounts[c.record];
            valid=c.admission==Source::Admission::Accepted&&c.repeatedAdmissions<=1u
                &&c.code==(r.kind==Source::Kind::TableRecord&&sameName(r.name,"DIMSTYLE")?105:5);
        }
        if(!source.binary&&c.rawValue!=c.lexeme)return false;
        std::uint64_t id=0u;if(!hex(c.lexeme,id)||!id)continue;
        auto pair=out.ids.emplace(id,Hit{c.record,i,!valid});if(!pair.second)pair.first->second.ambiguous=true;
    }
    return true;
}
inline const Source::Record* record(const Source& source,const Index& index,std::uint32_t handle,
    Source::Kind kind,Source::Section section,std::string_view name,std::size_t expectedRepeats) noexcept {
    const auto found=index.ids.find(handle);if(found==index.ids.end()||found->second.ambiguous
        ||found->second.record>=source.records.size()||found->second.record>=index.selfCounts.size()
        ||index.selfCounts[found->second.record]!=1u||found->second.candidate>=source.candidates.size()
        ||source.candidates[found->second.candidate].repeatedAdmissions!=expectedRepeats)return nullptr;
    const auto& row=source.records[found->second.record];
    if(row.kind!=kind||row.section>=source.sections.size()||source.sections[row.section].kind!=section||!sameName(row.name,name)
        ||!row.marker.ordinal||row.marker.beginOffset>=row.endMarker.beginOffset||row.marker.ordinal>=row.endMarker.ordinal)return nullptr;
    const auto& container=source.sections[row.section];
    if(!spanInside(row.marker,container.marker,container.endMarker)
        ||row.endMarker.ordinal>container.endMarker.ordinal||row.endMarker.beginOffset>container.endMarker.beginOffset
        ||row.firstCandidate>source.candidates.size()||row.candidates>source.candidates.size()-row.firstCandidate)return nullptr;
    return &row;
}
}
#endif
