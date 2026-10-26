// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_BLOCK_MEMBERSHIP_H
#define DX_BLOCK_MEMBERSHIP_H
#include "drw_interface.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <memory>
#include <string_view>
#include <vector>

// Converter-private original structural roles, not a geometry/body, target,
// editable-state, output or CAD-kernel certificate. No source normalization.
namespace dx_block_membership {
enum class Reason : std::uint8_t { Matched, Header, Identity, DuplicateIdentity, Owner, Dispatch, Sequence, Disposition, Callback, FollowFlag };
enum class Role : std::uint8_t { Primary, Vertex, Attribute, SequenceEnd };
enum class Owner : std::uint8_t { Unresolved, BlockRecord, CompoundParent };
struct Member {
    std::size_t dispatch{0},parent{0},selfGroup{0},ownerGroup{0};
    std::uint32_t self{0},owner{0};
    Reason reason{Reason::Matched};Role role{Role::Primary};Owner ownerRole{Owner::Unresolved};
    int follow{0};bool followPresent{false};
};
struct Range {std::size_t first{0},next{0};Reason reason{Reason::Matched};};
struct Audit {
    std::uint64_t generation{0};
    const void* container{nullptr};
    std::shared_ptr<const DRW_DxfBlockChildrenSource> children;
    std::shared_ptr<const DRW_DxfBlockDeliverySource> delivery;
    std::shared_ptr<const DRW_Block> begin,end;
    std::vector<Member> members;
    std::vector<Range> ranges;
    bool complete{false};
};
inline bool equal(std::string_view a,std::string_view b) noexcept {
    if(a.size()!=b.size())return false;
    for(std::size_t i=0;i<a.size();++i) {
        const auto upper=[](char c){return c>='a'&&c<='z'?static_cast<char>(c-'a'+'A'):c;};
        if(upper(a[i])!=upper(b[i]))return false;
    }
    return true;
}
inline std::string_view text(const DRW_Variant& value) noexcept {
    return value.type()==DRW_Variant::STRING&&value.content.s?std::string_view(*value.content.s):std::string_view{};
}
inline bool handle(const DRW_Variant& value,std::uint32_t& result) noexcept {
    const auto s=text(value);if(s.empty()||s.size()>16u)return false;
    std::uint64_t parsed=0u;const auto end=std::from_chars(s.data(),s.data()+s.size(),parsed,16);
    if(end.ec!=std::errc{}||end.ptr!=s.data()+s.size()||!parsed||parsed>UINT32_MAX)return false;
    result=static_cast<std::uint32_t>(parsed);return true;
}
inline Member identity(const DRW_DxfPhysicalEntitySource& record) noexcept {
    Member out;bool self=false,owner=false,common=false,body=false,xdata=false;
    std::array<std::string_view,DRW::kMaxDxfApplicationGroupNesting> applications{};std::size_t depth=0u;
    for(std::size_t i=0;i<record.groups.size();++i) {
        const auto& v=record.groups[i];const int code=v.code();const auto s=text(v);
        if(record.spans[i].ignoredComment)continue;
        if(code==102) {
            if(xdata){out.reason=Reason::Header;return out;}
            if(s=="}"){if(!depth){out.reason=Reason::Header;return out;}--depth;}
            else if(s.size()>1u&&s.front()=='{'&&depth<applications.size())applications[depth++]=s;
            else {out.reason=Reason::Header;return out;}
            continue;
        }
        if(depth)continue; // opaque/application 5/330/100 are never outer IDs
        if(code>=1000){xdata=true;continue;}
        if(xdata)continue;
        if(code==100||code==101) {
            if(code==100&&s=="AcDbEntity"&&!body&&!common)common=true;
            else body=true;
            continue;
        }
        // Base and AcDbEntity fields may be interleaved. Entity-specific and
        // embedded subclass handles are NOT common identity aliases.
        if(!body&&(code==5||code==330)) {
            auto& seen=code==5?self:owner;auto& target=code==5?out.self:out.owner;
            if(seen||!handle(v,target)){out.reason=Reason::Identity;return out;}
            seen=true;(code==5?out.selfGroup:out.ownerGroup)=i;
        }
        if((equal(record.name,"INSERT")||equal(record.name,"MINSERT"))&&code==66) {
            if(out.followPresent||v.type()!=DRW_Variant::INTEGER||(v.i_val()!=0&&v.i_val()!=1)) {
                out.reason=Reason::FollowFlag;return out;
            }
            out.followPresent=true;out.follow=v.i_val();
        }
    }
    if(depth||!common)out.reason=Reason::Header;
    else if(!self||!owner)out.reason=Reason::Identity;
    return out;
}
inline bool callbackName(DRW_DxfBlockCallback callback,std::string_view name) noexcept {
    using C=DRW_DxfBlockCallback;
    // These are the existing parser's routes/aliases, not schema equivalence.
    switch(callback) {
#define DX_MEMBER_NAME(callbackName,wireName) case C::callbackName:return equal(name,wireName)
    DX_MEMBER_NAME(addPoint,"POINT");DX_MEMBER_NAME(addLine,"LINE");DX_MEMBER_NAME(add3DLine,"3DLINE");
    DX_MEMBER_NAME(addRay,"RAY");DX_MEMBER_NAME(addXline,"XLINE");DX_MEMBER_NAME(addArc,"ARC");
    DX_MEMBER_NAME(addCircle,"CIRCLE");DX_MEMBER_NAME(addEllipse,"ELLIPSE");DX_MEMBER_NAME(addLWPolyline,"LWPOLYLINE");
    DX_MEMBER_NAME(addMLine,"MLINE");DX_MEMBER_NAME(addShape,"SHAPE");DX_MEMBER_NAME(addOle2Frame,"OLE2FRAME");
    DX_MEMBER_NAME(addOleFrame,"OLEFRAME");DX_MEMBER_NAME(addProxyEntity,"ACAD_PROXY_ENTITY");
    DX_MEMBER_NAME(addPolyline,"POLYLINE");DX_MEMBER_NAME(addSpline,"SPLINE");DX_MEMBER_NAME(addHelix,"HELIX");
    DX_MEMBER_NAME(addTable,"ACAD_TABLE");DX_MEMBER_NAME(addTrace,"TRACE");DX_MEMBER_NAME(add3dFace,"3DFACE");
    DX_MEMBER_NAME(addSolid,"SOLID");DX_MEMBER_NAME(addMText,"MTEXT");DX_MEMBER_NAME(addTolerance,"TOLERANCE");
    DX_MEMBER_NAME(addCamera,"CAMERA");DX_MEMBER_NAME(addLeader,"LEADER");DX_MEMBER_NAME(addHatch,"HATCH");
    DX_MEMBER_NAME(addMPolygon,"MPOLYGON");DX_MEMBER_NAME(addViewport,"VIEWPORT");DX_MEMBER_NAME(addImage,"IMAGE");
    DX_MEMBER_NAME(addWipeout,"WIPEOUT");DX_MEMBER_NAME(addPointCloud,"POINTCLOUD");DX_MEMBER_NAME(addPointCloudEx,"POINTCLOUDEX");
    DX_MEMBER_NAME(addNavisworksModel,"NAVISWORKSMODEL");DX_MEMBER_NAME(addMLeader,"MULTILEADER");
    case C::AttdefSource:case C::addAttDef:return equal(name,"ATTDEF");
    case C::addInsert:return equal(name,"INSERT")||equal(name,"MINSERT");
    case C::addMesh:return equal(name,"MESH")||equal(name,"SUBDIVISION");
    case C::addText:return equal(name,"TEXT")||equal(name,"RTEXT")||equal(name,"ARCALIGNEDTEXT")||equal(name,"ARC_ALIGNED_TEXT");
    case C::addGeoPositionMarker:return equal(name,"GEOPOSITIONMARKER")||equal(name,"POSITIONMARKER");
    case C::addSectionObject:return equal(name,"SECTIONOBJECT")||equal(name,"SECTION_OBJECT");
    case C::addUnderlay:return equal(name,"PDFUNDERLAY")||equal(name,"PDFREFERENCE")||equal(name,"DGNUNDERLAY")||equal(name,"DWFUNDERLAY");
    case C::addSurface:return equal(name,"PLANESURFACE")||equal(name,"EXTRUDEDSURFACE")||equal(name,"REVOLVEDSURFACE")
        ||equal(name,"SWEPTSURFACE")||equal(name,"LOFTEDSURFACE")||equal(name,"NURBSSURFACE");
    case C::addModelerGeometry:return equal(name,"3DSOLID")||equal(name,"REGION")||equal(name,"BODY");
    case C::addDimRadial:return equal(name,"DIMENSION")||equal(name,"LARGE_RADIAL_DIMENSION");
    case C::addDimArc:return equal(name,"ARC_DIMENSION");
    case C::addDimAlign:case C::addDimLinear:case C::addDimDiametric:case C::addDimAngular:
    case C::addDimAngular3P:case C::addDimOrdinate:return equal(name,"DIMENSION");
    case C::addRawDxfEntity:
        if(equal(name,"VERTEX")||equal(name,"ATTRIB")||equal(name,"SEQEND"))return false;
        for(unsigned i=0u;i<static_cast<unsigned>(C::addRawDxfEntity);++i)
            if(callbackName(static_cast<C>(i),name))return false;
        return true;
    case C::Count:return false;
    }
#undef DX_MEMBER_NAME
    return false;
}

// Caller has checked physical/dispatch partitions and precharged all vector
// owners and visits. Sorting indices detects duplicate self IDs in O(n log n),
// without an unbounded map allocation per physical record.
inline Audit audit(const void* container,std::uint64_t generation,std::shared_ptr<const DRW_DxfBlockChildrenSource> children,
                   std::shared_ptr<const DRW_DxfBlockDeliverySource> delivery,
                   std::shared_ptr<const DRW_Block> begin,std::shared_ptr<const DRW_Block> end) {
    Audit out;out.container=container;out.generation=generation;out.children=std::move(children);out.delivery=std::move(delivery);
    out.begin=std::move(begin);out.end=std::move(end);
    const auto& records=out.children->records;const auto& source=*out.delivery;
    out.members.reserve(records.size());out.ranges.reserve(source.dispatches.size());
    std::vector<std::size_t> indices;indices.reserve(records.size());
    for(std::size_t i=0;i<records.size();++i){out.members.push_back(identity(records[i]));indices.push_back(i);}
    std::size_t at=0u;
    for(std::size_t d=0;d<source.dispatches.size();++d) {
        const auto& dispatch=source.dispatches[d];const auto first=at;
        while(at<records.size()&&records[at].marker.ordinal<dispatch.end.ordinal)++at;
        Range range{first,at,Reason::Matched};auto& primary=out.members[first];
        const bool polyline=equal(records[first].name,"POLYLINE");
        const bool insert=equal(records[first].name,"INSERT")||equal(records[first].name,"MINSERT");
        const bool compound=polyline||(insert&&at-first>1u);
        for(std::size_t i=first;i<at;++i) {
            auto& member=out.members[i];member.dispatch=d;member.parent=first;
            if(i==first) {
                member.ownerRole=member.owner==source.blockRecord?Owner::BlockRecord:Owner::Unresolved;
            } else if(compound&&i+1u==at&&equal(records[i].name,"SEQEND")) {
                member.role=Role::SequenceEnd;
                // Preserve the observed wire owner separately from logical
                // parentage. Existing producer paths use both owner forms.
                member.ownerRole=member.owner==primary.self?Owner::CompoundParent:
                    member.owner==source.blockRecord?Owner::BlockRecord:Owner::Unresolved;
            } else if(compound&&equal(records[i].name,polyline?"VERTEX":"ATTRIB")) {
                member.role=polyline?Role::Vertex:Role::Attribute;
                member.ownerRole=member.owner==primary.self?Owner::CompoundParent:Owner::Unresolved;
            } else if(member.reason==Reason::Matched)member.reason=Reason::Sequence;
            if(member.reason==Reason::Matched&&member.ownerRole==Owner::Unresolved)member.reason=Reason::Owner;
        }
        // The reader preserves completed empty chains as parent+SEQEND.
        // Structural membership must not invent a required vertex/attribute.
        if(compound&&(at-first<2u||!equal(records[at-1u].name,"SEQEND")))range.reason=Reason::Sequence;
        if(!compound&&at-first!=1u)range.reason=Reason::Sequence;
        if(insert&&((compound&&primary.follow!=1)||(!compound&&primary.follow==1)))range.reason=Reason::FollowFlag;
        const bool attdef=equal(records[first].name,"ATTDEF");
        if(!dispatch.events)range.reason=Reason::Disposition;
        else if(dispatch.events!=(attdef&&source.events[dispatch.firstEvent].callback==DRW_DxfBlockCallback::AttdefSource?2u:1u))range.reason=Reason::Callback;
        for(std::size_t e=0;e<dispatch.events;++e) {
            const auto& event=source.events[dispatch.firstEvent+e];
            if(!event.hasValue||event.handle!=primary.self||event.owner!=primary.owner||!callbackName(event.callback,records[first].name)
                ||(attdef&&dispatch.events==2u&&event.callback!=(e?DRW_DxfBlockCallback::addAttDef:DRW_DxfBlockCallback::AttdefSource)))range.reason=Reason::Callback;
        }
        out.ranges.push_back(range);
    }
    std::sort(indices.begin(),indices.end(),[&](std::size_t a,std::size_t b){return out.members[a].self<out.members[b].self;});
    for(std::size_t i=0;i<indices.size();++i) {
        auto& member=out.members[indices[i]];
        if(member.self&&(member.self==source.blockHandle||member.self==source.blockRecord||member.self==out.end->handle
            ||(i&&member.self==out.members[indices[i-1u]].self)
            ||(i+1u<indices.size()&&member.self==out.members[indices[i+1u]].self)))member.reason=Reason::DuplicateIdentity;
    }
    out.complete=std::all_of(out.members.begin(),out.members.end(),[](const Member& m){return m.reason==Reason::Matched;})
        &&std::all_of(out.ranges.begin(),out.ranges.end(),[](const Range& r){return r.reason==Reason::Matched;});
    return out;
}

inline bool sameIdentity(const Member& member,const DRW_Entity* value) noexcept {
    return value&&value->handle==member.self&&value->parentHandle==member.owner;
}
inline bool compoundValue(const Audit& audit,std::size_t dispatch,const DRW_Entity* value) noexcept {
    if(dispatch>=audit.ranges.size())return false;
    const auto range=audit.ranges[dispatch];
    if(range.reason!=Reason::Matched||range.first>=range.next||range.next>audit.members.size())return false;
    const auto n=range.next-range.first;
    const auto& name=audit.children->records[range.first].name;
    const bool expectPolyline=equal(name,"POLYLINE");
    const bool expectInsert=equal(name,"INSERT")||equal(name,"MINSERT");
    const auto polyline=dynamic_cast<const DRW_Polyline*>(value);
    const auto insert=dynamic_cast<const DRW_Insert*>(value);
    if((expectPolyline&&!polyline)||(expectInsert&&!insert))return false;
    if(!expectPolyline&&!expectInsert)return n==1u;
    if(expectPolyline) {
        if(n<2u||polyline->vertlist.size()!=n-2u||!sameIdentity(audit.members[range.next-1u],polyline->seqEndEntity.get()))return false;
        for(std::size_t i=0;i<polyline->vertlist.size();++i)
            if(!sameIdentity(audit.members[range.first+1u+i],polyline->vertlist[i].get()))return false;
    } else {
        if(n==1u)return insert->attlist.empty()&&!insert->seqEndEntity;
        if(n<2u||insert->attlist.size()!=n-2u||!sameIdentity(audit.members[range.next-1u],insert->seqEndEntity.get()))return false;
        for(std::size_t i=0;i<insert->attlist.size();++i)
            if(!sameIdentity(audit.members[range.first+1u+i],insert->attlist[i].get()))return false;
    }
    return true;
}
} // namespace dx_block_membership
#endif
