// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_LEGACY_BLOCK_RECORD_SOURCE_PROFILE_H
#define DX_LEGACY_BLOCK_RECORD_SOURCE_PROFILE_H
#include "drw_objects.h"
#include <array>
#include <charconv>
#include <cstdint>
#include <string_view>
#include <vector>
// Witness-bounded physical inventory, not a document/target/output certificate.
// Caller must precharge group-count storage and linear inspection work.
namespace dx_legacy_block_record_source {
enum class Field:std::uint8_t {Identity,Subclass,Name,Layout,Application,Preview,Xdata};
enum class Role:std::uint8_t {None,Self,Owner,BlockName,Layout,ExtensionDictionary,BlockInsert,AppIdName,XdataObjectId};
enum class Reason:std::uint8_t {NotChecked,Complete,Revision,Identity,Unexpected,InvalidValue,Application,Xdata,Duplicate,Limit};
struct Span {std::size_t index;int code;Field field;Role role;std::uint32_t handle;};
struct Audit {Reason reason{Reason::NotChecked};std::size_t rejectedGroup{0};std::vector<Span> spans;
 bool complete()const noexcept{return reason==Reason::Complete;}};
inline std::string_view text(const DRW_Variant& v)noexcept {
 return v.type()==DRW_Variant::STRING&&v.content.s?std::string_view(*v.content.s):std::string_view{};
}
inline bool asciiName(const DRW_Variant& v,std::size_t max)noexcept {
 const auto s=text(v);if(s.empty()||s.size()>max)return false;
 for(const unsigned char c:s)if(c<=32u||c>=127u||c=='\\')return false;
 return true;
}
inline bool sameAsciiName(std::string_view a,std::string_view b)noexcept {
 if(a.size()!=b.size())return false;
 for(std::size_t i=0;i<a.size();++i){const auto up=[](unsigned char c){return c>='a'&&c<='z'?c-('a'-'A'):c;};
  if(up(static_cast<unsigned char>(a[i]))!=up(static_cast<unsigned char>(b[i])))return false;}
 return true;
}
inline bool handle(const DRW_Variant& v,std::uint32_t& out)noexcept {
 const auto s=text(v);if(s.empty()||s.size()>16u)return false;std::uint64_t n=0;
 const auto p=std::from_chars(s.data(),s.data()+s.size(),n,16);
 if(p.ec!=std::errc{}||p.ptr!=s.data()+s.size()||n>UINT32_MAX)return false;out=static_cast<std::uint32_t>(n);return true;
}
inline Audit audit(const DRW_RawDxfObject& raw) {
 Audit out;const auto reject=[&](Reason why,std::size_t i){out.reason=why;out.rejectedGroup=i;out.spans.clear();return std::move(out);};
 if(raw.m_version!=DRW::AC1018)return reject(Reason::Revision,0u);
 if(raw.name!="BLOCK_RECORD"||!raw.handle||!raw.parentHandle)return reject(Reason::Identity,0u);
 if(raw.groups.size()>256u)return reject(Reason::Limit,0u);
 out.spans.reserve(raw.groups.size());std::size_t at=0u;
 const auto current=[&]()->const DRW_Variant*{return at<raw.groups.size()?&raw.groups[at]:nullptr;};
 const auto take=[&](int code,Field f,Role r,std::uint32_t h){out.spans.push_back({at++,code,f,r,h});};
 const auto expectText=[&](int code,std::string_view expected,Field field)->bool {
  const auto* v=current();if(!v||v->code()!=code||text(*v)!=expected)return false;take(code,field,Role::None,0u);return true;};
 std::uint32_t h=0u;const auto* v=current();
 if(!v||v->code()!=5||!handle(*v,h)||h!=raw.handle)return reject(Reason::Identity,at);
 take(5,Field::Identity,Role::Self,h);
 if(!expectText(102,"{ACAD_XDICTIONARY",Field::Application))return reject(Reason::Application,at);
 v=current();if(!v||v->code()!=360||!handle(*v,h)||!h)return reject(Reason::Application,at);
 take(360,Field::Application,Role::ExtensionDictionary,h);
 if(!expectText(102,"}",Field::Application))return reject(Reason::Application,at);
 v=current();if(!v||v->code()!=330||!handle(*v,h)||h!=raw.parentHandle)return reject(Reason::Identity,at);
 take(330,Field::Identity,Role::Owner,h);
 if(!expectText(100,"AcDbSymbolTableRecord",Field::Subclass)
  ||!expectText(100,"AcDbBlockTableRecord",Field::Subclass))return reject(Reason::Unexpected,at);
 v=current();if(!v||v->code()!=2||!asciiName(*v,255u))return reject(Reason::InvalidValue,at);
 take(2,Field::Name,Role::BlockName,0u);
 v=current();if(!v||v->code()!=340||!handle(*v,h)||h!=0u)return reject(Reason::InvalidValue,at);
 take(340,Field::Layout,Role::Layout,0u); // explicit null, never inferred from absence
 v=current();
 if(v&&v->code()==102&&text(*v)=="{BLKREFS") {
  take(102,Field::Application,Role::None,0u);
  v=current();if(!v||v->code()!=331||!handle(*v,h)||!h)return reject(Reason::Application,at);
  take(331,Field::Application,Role::BlockInsert,h);
  if(!expectText(102,"}",Field::Application))return reject(Reason::Application,at);
 }
 std::array<std::string_view,4> names{};std::size_t count=0u,depth=0u,values=0u;
 while((v=current())) {
  const int code=v->code();
  if(code==310) {
   const auto payload=text(*v);
   if(count||v->type()!=DRW_Variant::STRING||payload.size()>254u||(payload.size()&1u))
    return reject(Reason::InvalidValue,at);
   for(const unsigned char c:payload)if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))
    return reject(Reason::InvalidValue,at);
   take(code,Field::Preview,Role::None,0u);
  } else if(code==1001) {
   if(depth||(count&&values==0u)||!asciiName(*v,31u))return reject(Reason::Xdata,at);
   if(count==names.size())return reject(Reason::Limit,at);
   for(std::size_t i=0;i<count;++i)if(sameAsciiName(names[i],text(*v)))return reject(Reason::Duplicate,at);
   names[count++]=text(*v);values=0u;take(code,Field::Xdata,Role::AppIdName,0u);
  } else if(!count)return reject(Reason::Xdata,at);
  else if(code==1000) {
   const auto s=text(*v);if(v->type()!=DRW_Variant::STRING||!v->content.s||s.size()>255u||s.find('\0')!=std::string_view::npos)return reject(Reason::InvalidValue,at);
   ++values;take(code,Field::Xdata,Role::None,0u);
  } else if(code==1002) {
   const auto s=text(*v);if(s=="{"){if(depth==4u)return reject(Reason::Limit,at);++depth;}
   else if(s=="}"){if(!depth)return reject(Reason::Xdata,at);--depth;}
   else return reject(Reason::Xdata,at);
   take(code,Field::Xdata,Role::None,0u);
  } else if(code==1070) {
   if(v->type()!=DRW_Variant::INTEGER||v->i_val()<-32768||v->i_val()>32767)return reject(Reason::InvalidValue,at);
   ++values;take(code,Field::Xdata,Role::None,0u);
  } else if(code==1071) {
   if(v->type()!=DRW_Variant::INTEGER)return reject(Reason::InvalidValue,at);
   ++values;take(code,Field::Xdata,Role::None,0u);
  } else if(code==1005) {
   if(!handle(*v,h)||!h)return reject(Reason::InvalidValue,at);
   ++values;take(code,Field::Xdata,Role::XdataObjectId,h);
  } else return reject(Reason::Unexpected,at);
 }
 if(!count||!values||depth)return reject(Reason::Xdata,at);
 out.reason=Reason::Complete;out.rejectedGroup=at;return out;
}
}
#endif
