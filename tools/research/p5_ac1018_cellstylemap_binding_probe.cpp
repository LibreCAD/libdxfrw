// Optional caller-supplied physical source; proves only selected wire fields
// and private reader association, never graph closure or output authority.
#include "dx_iface.h"
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <string>
#include <iostream>
#include <new>
#include <stdexcept>
#include <vector>
namespace fault {long remaining=-1;bool fired=false;}
void* operator new(std::size_t n){if(fault::remaining==0){fault::remaining=-1;fault::fired=true;throw std::bad_alloc();}
    if(fault::remaining>0)--fault::remaining;if(void* p=std::malloc(n?n:1u))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
class DxIfaceStandardDwgCaptureTestAccess {
public:
 static void deliver(dx_iface& i,const DwgVerifiedTableCandidateTicket& t) noexcept{i.dx_iface::verifiedTableCandidate(t);}
 static DRW_UnsupportedObject& raw(dx_data& d,std::uint32_t h){return d.m_opaqueDwgSourceFrames.at({d.importGeneration,h});}
 static void transplant(dx_data& dst,const dx_data& src){dst.m_verifiedTableCandidateSource=src.m_verifiedTableCandidateSource;}
};
namespace {
const std::vector<std::uint32_t> xs{0xbcu,0xbeu,0xbfu,0xc0u,0xc1u,0xc2u,0xc3u,0xc4u,0xc5u,0xc6u};
const std::vector<std::uint32_t> all{0xbcu,0xbeu,0xbfu,0xc0u,0xc1u,0xc2u,0xc3u,0xc4u,0xc5u,0xc6u,0xdcu};
bool enable(dx_iface& r,std::uint32_t map=0xddu){return r.enableVerifiedTableCellStyleResourceSource(
 xs,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu,0xdbu,0xdcu,map,
 0x3u,0x11u,0x5u,0x16u,0x14u,0x15u);}
bool live(const dx_data& d){DwgCellStyleMapAc1018Candidate::Result r;
 return dx_iface::readVerifiedTableCellStyleResourceSource(d,xs,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu,0xdbu,0xdcu,0xddu,
  0x3u,0x11u,0x5u,0x16u,0x14u,0x15u,r)
  && r.handle==0xddu && r.owner==0xdbu && r.styles[0].id==3 && r.styles[1].id==1 && r.styles[2].id==2
  && r.dataEndBit==5287u && r.handleEndBit==5655u
  && r.styles[0].textStyle.target==0x11u && r.styles[2].linetypes[5].target==0x14u
  && dx_iface::hasUnchangedVerifiedXRecordSource(d,0xbcu)
  && dx_iface::hasUnchangedVerifiedXRecordSources(d,all)
  && !dx_iface::hasUnchangedVerifiedXRecordSources(d,xs);}
class Probe final:public dx_iface {
public:
 enum class Mode {Normal,Spoof,Duplicate,Throw,Nested};Mode mode{Mode::Normal};bool hit=false;
 dx_data* nestedData=nullptr;dx_iface* nestedReader=nullptr;std::string path;
 void addCellStyleMap(const DRW_CellStyleMap& v) override {
  if(v.handle==0xddu){hit=true;if(mode==Mode::Throw)throw std::runtime_error("map callback failure");
   if(mode==Mode::Nested){(void)(nestedReader?nestedReader:this)->fileImport(path,nestedData,false);return;}
   if(mode==Mode::Spoof){auto copy=v;copy.m_cellStyles[0].m_contentFormat.m_textHeight=42.;dx_iface::addCellStyleMap(copy);return;}
   if(mode==Mode::Duplicate){dx_iface::addCellStyleMap(v);dx_iface::addCellStyleMap(v);return;}}
  dx_iface::addCellStyleMap(v);
 }
};
class FaultProbe final:public dx_iface {
public:bool inject=true,seen=false;
 void verifiedTableCandidate(const DwgVerifiedTableCandidateTicket& t) noexcept override {
  if(inject && t.kind()==DwgVerifiedTableCandidateKind::CellStyleMap){seen=true;fault::remaining=0;}
  DxIfaceStandardDwgCaptureTestAccess::deliver(*this,t);fault::remaining=-1;
 }
};
}
int main(int argc,char**argv){if(argc!=2)return 2;Probe r;dx_data d;
 if(!enable(r) || !r.fileImport(argv[1],&d,false)||!r.hit||!live(d))return 3;
 auto map=std::find_if(d.cellStyleMaps.begin(),d.cellStyleMaps.end(),[](const DRW_CellStyleMap& x){return x.handle==0xddu;});
 if(map==d.cellStyleMaps.end())return 4;const auto original=*map;
 const auto change=[&](const auto& mutation){mutation(*map);const bool failed=!live(d)&&!dx_iface::hasUnchangedVerifiedXRecordSource(d,0xbcu);*map=original;return failed&&live(d);};
 if(!change([](DRW_CellStyleMap& x){std::swap(x.m_cellStyles[0],x.m_cellStyles[1]);})
  ||!change([](DRW_CellStyleMap& x){x.parentHandle^=1u;})
  ||!change([](DRW_CellStyleMap& x){x.reactorHandles[0]^=1u;})
  ||!change([](DRW_CellStyleMap& x){x.m_cellStyles[0].m_contentFormat.m_textStyleHandle^=1u;})
  ||!change([](DRW_CellStyleMap& x){x.m_cellStyles[2].m_borders[5].m_lineTypeHandle^=1u;})
  ||!change([](DRW_CellStyleMap& x){x.m_cellStyles[1].m_contentFormat.m_textHeight=42.;}))return 5;
 auto textStyle=std::find_if(d.textStyles.begin(),d.textStyles.end(),[](const DRW_Textstyle& x){return x.handle==0x11u;});
 if(textStyle==d.textStyles.end())return 51;const auto originalTextStyle=*textStyle;
 textStyle->name+="_MUTATED";if(live(d))return 52;*textStyle=originalTextStyle;if(!live(d))return 53;
 auto byBlock=std::find_if(d.lineTypes.begin(),d.lineTypes.end(),[](const DRW_LType& x){return x.handle==0x14u;});
 if(byBlock==d.lineTypes.end())return 54;const auto originalByBlock=*byBlock;
 byBlock->desc+="_MUTATED";if(live(d))return 55;*byBlock=originalByBlock;if(!live(d))return 56;
 auto appId=std::find_if(d.appIds.begin(),d.appIds.end(),[](const DRW_AppId& x){return x.handle==0x12u;});
 if(appId==d.appIds.end())return 57;const auto originalAppId=*appId;
 appId->name+="_MUTATED";if(live(d))return 58;*appId=originalAppId;if(!live(d))return 59;
 map->m_subrecordRanges.push_back({});if(!live(d))return 6;*map=original;
 auto& raw=DxIfaceStandardDwgCaptureTestAccess::raw(d,0xddu);raw.m_rawBytes[0]^=1u;if(live(d))return 7;raw.m_rawBytes[0]^=1u;if(!live(d))return 8;
 d.cellStyleMaps.push_back(original);if(live(d))return 9;d.cellStyleMaps.pop_back();if(!live(d))return 10;
 auto dict=std::find_if(d.dictionaries.begin(),d.dictionaries.end(),[](const DRW_Dictionary& x){return x.handle==0xdbu;});
 if(dict==d.dictionaries.end()||dict->m_entries.size()!=2u)return 11;
 const auto dictOriginal=*dict;dict->m_entries[1].m_handle^=1u;if(live(d))return 12;*dict=dictOriginal;if(!live(d))return 13;
 dx_data second;if(!r.fileImport(argv[1],&second,false)||!live(second))return 14;
 DxIfaceStandardDwgCaptureTestAccess::transplant(second,d);if(live(second))return 15;
 for(auto mode:{Probe::Mode::Spoof,Probe::Mode::Duplicate,Probe::Mode::Throw}){Probe p;p.mode=mode;dx_data bad;if(!enable(p))return 16;
  const bool read=p.fileImport(argv[1],&bad,false);if(!p.hit||live(bad)||dx_iface::hasUnchangedVerifiedXRecordSource(bad,0xbcu)||(mode==Probe::Mode::Throw&&read))return 17;
  p.mode=Probe::Mode::Normal;dx_data retry;if(!p.fileImport(argv[1],&retry,false)||!live(retry))return 18;}
 FaultProbe p;dx_data failed;if(!enable(p))return 19;fault::fired=false;
 if(!p.fileImport(argv[1],&failed,false)||!p.seen||!fault::fired||live(failed))return 20;
 p.inject=false;dx_data retry;if(!p.fileImport(argv[1],&retry,false)||!live(retry))return 21;
 Probe absent;dx_data missing;if(!enable(absent,0xcau))return 22;(void)absent.fileImport(argv[1],&missing,false);if(live(missing))return 23;
 if(!enable(absent))return 24;dx_data retry2;if(!absent.fileImport(argv[1],&retry2,false)||!live(retry2))return 25;
 Probe nested;nested.mode=Probe::Mode::Nested;nested.path=argv[1];dx_data nestedData;nested.nestedData=&nestedData;
 if(!enable(nested))return 26;(void)nested.fileImport(argv[1],&nestedData,false);if(live(nestedData))return 27;
 nested.mode=Probe::Mode::Normal;dx_data nestedRetry;if(!nested.fileImport(argv[1],&nestedRetry,false)||!live(nestedRetry))return 28;
 Probe outer;dx_iface inner;dx_data cross;outer.mode=Probe::Mode::Nested;outer.path=argv[1];outer.nestedData=&cross;outer.nestedReader=&inner;
 if(!enable(outer)||!enable(inner))return 29;(void)outer.fileImport(argv[1],&cross,false);if(live(cross))return 30;
 outer.mode=Probe::Mode::Normal;dx_data crossRetry;if(!outer.fileImport(argv[1],&crossRetry,false)||!live(crossRetry))return 31;
 const char* out="/private/tmp/p5-cellstylemap-binding-sentinel.dxf";{std::ofstream f(out,std::ios::binary|std::ios::trunc);f<<"MAP_SENTINEL";if(!f)return 32;}
 const bool exported=r.fileExport(out,DRW::AC1018,false,&d,false,true,dx_dxfSourceTextPolicy::Compatible,
     DRW::DxfTextWritePolicy::Compatible,dx_dxfProxyGraphicsPolicy::Compatible,dx_dwgReadCompletenessPolicy::RequireComplete);
 std::ifstream file(out,std::ios::binary);const std::string sentinel((std::istreambuf_iterator<char>(file)),{});std::remove(out);
 if(exported||sentinel!="MAP_SENTINEL")return 33;
 dx_iface selectionFault;fault::remaining=0;fault::fired=false;
 const bool selected=enable(selectionFault);fault::remaining=-1;
 if(selected||!fault::fired||!enable(selectionFault))return 37;
 dx_data selectorRetry;if(!selectionFault.fileImport(argv[1],&selectorRetry,false)||!live(selectorRetry))return 38;
 Probe downgraded;dx_data downgradeData;DwgTableStyleRoundtripXRecordAc1018Candidate::Result oldXrecord;
 if(!downgraded.enableVerifiedTableStyleXRecordSource(xs,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu,0xdbu,0xdcu,0xddu)
    ||!downgraded.fileImport(argv[1],&downgradeData,false))return 39;
 auto downgradedMap=std::find_if(downgradeData.cellStyleMaps.begin(),downgradeData.cellStyleMaps.end(),[](const DRW_CellStyleMap& x){return x.handle==0xddu;});
 if(downgradedMap==downgradeData.cellStyleMaps.end())return 40;
 std::swap(downgradedMap->m_cellStyles[0],downgradedMap->m_cellStyles[1]);
 if(!dx_iface::readVerifiedTableStyleXRecordSource(downgradeData,xs,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu,0xdbu,0xdcu,0xddu,oldXrecord)
    ||live(downgradeData))return 41;
 Probe prior;dx_data old;DwgTableStyleExtensionDictionaryAc1018Candidate::Result oldResult;
 if(!prior.enableVerifiedTableStyleDictionarySource(xs,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu,0xdbu,0xdcu,0xddu)
    ||!prior.fileImport(argv[1],&old,false)
    ||!dx_iface::readVerifiedTableStyleDictionarySource(old,xs,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu,0xdbu,0xdcu,0xddu,oldResult))return 34;
 auto oldMap=std::find_if(old.cellStyleMaps.begin(),old.cellStyleMaps.end(),[](const DRW_CellStyleMap& x){return x.handle==0xddu;});
 if(oldMap==old.cellStyleMaps.end())return 35;std::swap(oldMap->m_cellStyles[0],oldMap->m_cellStyles[1]);
 if(!dx_iface::readVerifiedTableStyleDictionarySource(old,xs,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu,0xdbu,0xdcu,0xddu,oldResult))return 36;
 std::cout<<"CELLSTYLEMAP source order, dictionary targets, same-import STYLE/LTYPE resources, mutation, failure/retry, old mode and output gate controls passed\n";return 0;}
