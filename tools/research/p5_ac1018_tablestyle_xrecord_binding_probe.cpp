// Optional caller-supplied physical source; this does not ship a drawing or
// claim complete TABLESTYLE semantics, resource closure or output support.
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
namespace fault {long remaining=-1;bool fired=false;std::size_t count=0;bool counting=false;bool (*predicate)() noexcept=nullptr;}
void* operator new(std::size_t n){if(fault::counting)++fault::count;
    if(fault::predicate && fault::predicate()){fault::predicate=nullptr;fault::fired=true;throw std::bad_alloc();}
    if(fault::remaining==0){fault::remaining=-1;fault::fired=true;throw std::bad_alloc();}
    if(fault::remaining>0)--fault::remaining;if(void* p=std::malloc(n?n:1u))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
class DxIfaceStandardDwgCaptureTestAccess {
public:static void deliver(dx_iface& i,const DwgVerifiedTableCandidateTicket& t) noexcept{i.dx_iface::verifiedTableCandidate(t);}
 static void deliverX(dx_iface& i,const DwgVerifiedXRecordTicket& t) noexcept{i.dx_iface::verifiedXRecord(t);}
 static DRW_UnsupportedObject& raw(dx_data& d,std::uint32_t h){return d.m_standardDwgObjectFrames.at({d.importGeneration,h}).raw;}
 static DRW_DwgFramePublication& publication(dx_data& d,std::uint32_t h){return d.m_standardDwgObjectFrames.at({d.importGeneration,h}).publication;}
 static void transplant(dx_data& dst,const dx_data& src){dst.m_verifiedTableCandidateSource=src.m_verifiedTableCandidateSource;}
 static bool sealStage(dx_iface& a,int stage) noexcept {
    const auto* d=a.m_verifiedXRecordReadData;
    if(!d || !d->dwgReadOutcome.operationSucceeded || d->m_verifiedXRecordSource || d->m_verifiedTableCandidateSource)return false;
    if(stage==1 && (a.m_verifiedXRecordPending.size()!=a.m_verifiedXRecordSelected.size() || a.m_verifiedTableCandidatePending.size()!=7u))return false;
    if(stage==2 && (!a.m_verifiedXRecordPending.empty() || a.m_verifiedTableCandidatePending.size()!=7u))return false;
    if(stage==3 && (!a.m_verifiedXRecordPending.empty() || !a.m_verifiedTableCandidatePending.empty()))return false;
    for(const auto& row:d->m_standardDwgObjectFrames)if(row.first.first==d->importGeneration && !row.second.payloadMatched)return false;
    return true;
 }
};
namespace {
const std::vector<std::uint32_t> xs{0xbcu,0xbeu,0xbfu,0xc0u,0xc1u,0xc2u,0xc3u,0xc4u,0xc5u,0xc6u};
bool enable(dx_iface& r,std::uint32_t h=0x6bu){return r.enableVerifiedTableStyleXRecordSource(xs,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,h,0xdbu,0xdcu,0xddu);}
bool live(const dx_data& d){DwgTableStyleRoundtripXRecordAc1018Candidate::Result r;
 auto all=xs;all.push_back(0xdcu);
 return dx_iface::readVerifiedTableStyleXRecordSource(d,xs,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu,0xdbu,0xdcu,0xddu,r)
  && r.handle==0xdcu && r.owner==0xdbu && r.reactor==0xdbu && r.dataEndBit==803u
  && dx_iface::hasUnchangedVerifiedXRecordSource(d,0xbcu) && dx_iface::hasUnchangedVerifiedXRecordSources(d,all)
  && !dx_iface::hasUnchangedVerifiedXRecordSources(d,xs);}

dx_iface* sealingAdapter=nullptr;int sealingStage=0;
bool sealFaultReady() noexcept{return sealingAdapter && DxIfaceStandardDwgCaptureTestAccess::sealStage(*sealingAdapter,sealingStage);}
class Probe final:public dx_iface {
public:
 enum class Mode{Normal,Spoof,Throw,Duplicate,Nested,DictSpoof,DictThrow,DictDuplicate,DictNested,XSpoof,XThrow,XDuplicate,XNested};Mode mode{Mode::Normal};bool hit=false;
 dx_data* nestedData=nullptr;dx_iface* nestedReader=nullptr;std::string path;
 void addXRecord(const DRW_XRecord& v) override {
  if(v.handle==0xdcu){hit=true;if(mode==Mode::XThrow)throw std::runtime_error("XRECORD callback failure");
   if(mode==Mode::XNested){(void)(nestedReader?nestedReader:this)->fileImport(path,nestedData,false);return;}
   if(mode==Mode::XSpoof){auto copy=v;copy.parentHandle=0xdau;dx_iface::addXRecord(copy);return;}
   if(mode==Mode::XDuplicate){dx_iface::addXRecord(v);dx_iface::addXRecord(v);return;}}
  dx_iface::addXRecord(v);
 }
 void addDictionary(const DRW_Dictionary& v) override {
  if(v.handle==0xdbu){hit=true;if(mode==Mode::DictThrow)throw std::runtime_error("dictionary callback failure");
   if(mode==Mode::DictNested){(void)(nestedReader?nestedReader:this)->fileImport(path,nestedData,false);return;}
   if(mode==Mode::DictSpoof){auto copy=v;copy.m_entries[0].m_handle=0xdeu;dx_iface::addDictionary(copy);return;}
   if(mode==Mode::DictDuplicate){dx_iface::addDictionary(v);dx_iface::addDictionary(v);return;}}
  dx_iface::addDictionary(v);
 }
 void addTableStyle(const DRW_TableStyle& v) override {
  if(v.handle==0x6bu){hit=true;if(mode==Mode::Throw)throw std::runtime_error("style callback failure");
   if(mode==Mode::Nested){(void)(nestedReader?nestedReader:this)->fileImport(path,nestedData,false);return;}
   if(mode==Mode::Spoof){auto copy=v;copy.m_rowStyles[0].m_textHeight=42.;dx_iface::addTableStyle(copy);return;}
   if(mode==Mode::Duplicate){dx_iface::addTableStyle(v);dx_iface::addTableStyle(v);return;}}
  dx_iface::addTableStyle(v);
 }
};
class FaultProbe final:public dx_iface {
public:bool inject=true,seen=false;
 void verifiedTableCandidate(const DwgVerifiedTableCandidateTicket& t) noexcept override {
  if(inject && t.kind()==DwgVerifiedTableCandidateKind::StyleExtensionDictionary){seen=true;fault::remaining=0;}
  DxIfaceStandardDwgCaptureTestAccess::deliver(*this,t);fault::remaining=-1;
 }
};
class XFaultProbe final:public dx_iface {
public:bool inject=true,seen=false;
 void verifiedXRecord(const DwgVerifiedXRecordTicket& t) noexcept override {
  if(inject && t.handle()==0xdcu){seen=true;fault::remaining=0;}
  DxIfaceStandardDwgCaptureTestAccess::deliverX(*this,t);fault::remaining=-1;
 }
};
}
int main(int argc,char**argv){if(argc!=2)return 2;Probe r;dx_data d;
 if(!enable(r) || r.enableVerifiedTableStyleXRecordSource(xs,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu,0xdbu,0xdcu,0xdcu)
    || r.enableVerifiedTableStyleXRecordSource(xs,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu,0xbbu,0xdcu,0xddu)
    || !r.fileImport(argv[1],&d,false)||!r.hit||!live(d))return 3;
 auto xrecord=std::find_if(d.xRecords.begin(),d.xRecords.end(),[](const DRW_XRecord& x){return x.handle==0xdcu;});
 if(xrecord==d.xRecords.end())return 60;const auto savedX=*xrecord;
 const auto checkX=[&](const auto& change){change(*xrecord);const bool failed=!live(d)
  && !dx_iface::hasUnchangedVerifiedXRecordSource(d,0xbcu) && !dx_iface::hasUnchangedVerifiedXRecordSource(d,0xdcu);
  *xrecord=savedX;return failed && live(d);};
 if(!checkX([](DRW_XRecord& x){x.parentHandle^=1u;}) || !checkX([](DRW_XRecord& x){x.reactorHandles[0]^=1u;})
  || !checkX([](DRW_XRecord& x){x.m_dataEntries[1]=DRW_Variant(90,std::int32_t{9});})
  || !checkX([](DRW_XRecord& x){std::swap(x.m_dataEntries[1],x.m_dataEntries[2]);})
  || !checkX([](DRW_XRecord& x){x.m_values.emplace_back(90,std::int32_t{99});})
  || !checkX([](DRW_XRecord& x){x.m_handleValues.emplace_back(360,0x999u);}))return 61;
 d.xRecords.push_back(savedX);if(live(d))return 62;d.xRecords.pop_back();if(!live(d))return 63;
 auto& xraw=DxIfaceStandardDwgCaptureTestAccess::raw(d,0xdcu);xraw.m_rawBytes[10]^=1u;if(live(d))return 64;xraw.m_rawBytes[10]^=1u;if(!live(d))return 65;
 auto& xpub=DxIfaceStandardDwgCaptureTestAccess::publication(d,0xdcu);++xpub.m_sourceMapOrdinal;if(live(d))return 66;--xpub.m_sourceMapOrdinal;if(!live(d))return 67;
 for(auto mode:{Probe::Mode::XSpoof,Probe::Mode::XThrow,Probe::Mode::XDuplicate}){
  Probe altered;altered.mode=mode;dx_data failed;if(!enable(altered))return 68;const bool read=altered.fileImport(argv[1],&failed,false);
  if(!altered.hit || live(failed) || dx_iface::hasUnchangedVerifiedXRecordSource(failed,0xbcu) || (mode==Probe::Mode::XThrow && read))return 69;
  altered.mode=Probe::Mode::Normal;dx_data retry;if(!altered.fileImport(argv[1],&retry,false)||!live(retry))return 70;
 }
 XFaultProbe xf;dx_data xfFailed;if(!enable(xf))return 71;fault::fired=false;
 if(!xf.fileImport(argv[1],&xfFailed,false) || !xf.seen || !fault::fired || live(xfFailed))return 72;
 xf.inject=false;dx_data xfRetry;if(!xf.fileImport(argv[1],&xfRetry,false)||!live(xfRetry))return 73;
 for(int cross=0;cross<2;++cross){Probe outer;dx_iface inner;dx_data shared;outer.mode=Probe::Mode::XNested;
  outer.path=argv[1];outer.nestedData=&shared;outer.nestedReader=cross?&inner:nullptr;
  if(!enable(outer)||(cross&&!enable(inner)))return 74;(void)outer.fileImport(argv[1],&shared,false);
  if(live(shared)||dx_iface::hasUnchangedVerifiedXRecordSource(shared,0xbcu))return 75;
  outer.mode=Probe::Mode::Normal;dx_data retry;if(!outer.fileImport(argv[1],&retry,false)||!live(retry))return 76;
 }
 Probe selector;if(!selector.enableVerifiedTableStyleDictionarySource(xs,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu,0xdbu,0xdcu,0xddu))return 77;
 fault::remaining=0;fault::fired=false;const bool selected=enable(selector);fault::remaining=-1;
 if(selected||!fault::fired)return 78;dx_data priorData;DwgTableStyleExtensionDictionaryAc1018Candidate::Result priorDict;
 if(!selector.fileImport(argv[1],&priorData,false) || !dx_iface::readVerifiedTableStyleDictionarySource(priorData,xs,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu,0xdbu,0xdcu,0xddu,priorDict))return 79;
 dx_iface older;dx_data olderData;DwgTableStyleAc1018Candidate::Result prior;
 if(!older.enableVerifiedTableStyleSource(xs,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu)
    || !older.fileImport(argv[1],&olderData,false))return 51;
 auto olderDict=std::find_if(olderData.dictionaries.begin(),olderData.dictionaries.end(),[](const DRW_Dictionary& x){return x.handle==0xdbu;});
 if(olderDict==olderData.dictionaries.end())return 52;std::swap(olderDict->m_entries[0].m_handle,olderDict->m_entries[1].m_handle);
 if(!dx_iface::readVerifiedTableStyleSource(olderData,xs,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu,prior))return 53;
 auto dict=std::find_if(d.dictionaries.begin(),d.dictionaries.end(),[](const DRW_Dictionary& x){return x.handle==0xdbu;});
 if(dict==d.dictionaries.end()||dict->m_entries.size()!=2u)return 32;
 const auto original=*dict;
 const auto check=[&](const auto& mutation){mutation(*dict);const bool failed=!live(d)&&!dx_iface::hasUnchangedVerifiedXRecordSource(d,0xbcu);
  *dict=original;return failed&&live(d);};
 if(!check([](DRW_Dictionary& x){x.parentHandle^=1u;}) || !check([](DRW_Dictionary& x){x.hardOwner=0;})
  || !check([](DRW_Dictionary& x){x.m_entries[0].m_name+="x";})
  || !check([](DRW_Dictionary& x){std::swap(x.m_entries[0],x.m_entries[1]);})
  || !check([](DRW_Dictionary& x){std::swap(x.m_entries[0].m_handle,x.m_entries[1].m_handle);})
  || !check([](DRW_Dictionary& x){x.m_entries[1].m_handle=0xdeu;}))return 33;
 auto& raw=DxIfaceStandardDwgCaptureTestAccess::raw(d,0xdbu);raw.m_rawBytes[0]^=1u;if(live(d))return 34;raw.m_rawBytes[0]^=1u;if(!live(d))return 35;
 auto& pub=DxIfaceStandardDwgCaptureTestAccess::publication(d,0xdbu);++pub.m_sourceMapOrdinal;if(live(d))return 36;--pub.m_sourceMapOrdinal;if(!live(d))return 37;
 d.dictionaries.push_back(original);if(live(d))return 38;d.dictionaries.pop_back();if(!live(d))return 39;
 dx_data second;if(!r.fileImport(argv[1],&second,false)||!live(second))return 40;
 DxIfaceStandardDwgCaptureTestAccess::transplant(second,d);if(live(second))return 41;
 for(auto mode:{Probe::Mode::DictSpoof,Probe::Mode::DictThrow,Probe::Mode::DictDuplicate}){
  Probe altered;altered.mode=mode;dx_data failed;if(!enable(altered))return 42;const bool read=altered.fileImport(argv[1],&failed,false);
  if(!altered.hit||live(failed)||dx_iface::hasUnchangedVerifiedXRecordSource(failed,0xbcu)
      || (mode==Probe::Mode::DictThrow && read))return 43;
  altered.mode=Probe::Mode::Normal;dx_data again;if(!altered.fileImport(argv[1],&again,false)||!live(again))return 44;
 }
 Probe nestedDict;nestedDict.mode=Probe::Mode::DictNested;nestedDict.path=argv[1];dx_data shared;nestedDict.nestedData=&shared;
 if(!enable(nestedDict))return 45;(void)nestedDict.fileImport(argv[1],&shared,false);
 if(live(shared)||dx_iface::hasUnchangedVerifiedXRecordSource(shared,0xbcu))return 46;
 nestedDict.mode=Probe::Mode::Normal;dx_data nestedAgain;if(!nestedDict.fileImport(argv[1],&nestedAgain,false)||!live(nestedAgain))return 47;
 Probe crossDict;dx_iface innerDict;dx_data crossData;crossDict.mode=Probe::Mode::DictNested;crossDict.path=argv[1];crossDict.nestedData=&crossData;crossDict.nestedReader=&innerDict;
 if(!enable(crossDict)||!enable(innerDict))return 48;(void)crossDict.fileImport(argv[1],&crossData,false);
 if(live(crossData)||dx_iface::hasUnchangedVerifiedXRecordSource(crossData,0xbcu))return 49;
 crossDict.mode=Probe::Mode::Normal;dx_data crossAgain;if(!crossDict.fileImport(argv[1],&crossAgain,false)||!live(crossAgain))return 50;
 for(int stage=1;stage<=3;++stage){dx_iface a;if(!enable(a))return 54;dx_data failed;
  sealingAdapter=&a;sealingStage=stage;fault::predicate=sealFaultReady;fault::fired=false;
  const bool ok=a.fileImport(argv[1],&failed,false);fault::predicate=nullptr;sealingAdapter=nullptr;
  if(!ok||!fault::fired||live(failed)||dx_iface::hasUnchangedVerifiedXRecordSource(failed,0xbcu))return 55;
  dx_data again;if(!a.fileImport(argv[1],&again,false)||!live(again))return 56;
 }
 fault::count=0;fault::counting=true;const bool counted=live(d);fault::counting=false;const auto allocations=fault::count;
 if(!counted||!allocations)return 57;fault::remaining=static_cast<long>(allocations-1u);fault::fired=false;
 const bool faulted=live(d);fault::remaining=-1;if(faulted||!fault::fired||!live(d))return 58;
 DwgTableStyleRoundtripXRecordAc1018Candidate::Result singleResult;
 fault::count=0;fault::counting=true;const bool single=dx_iface::readVerifiedTableStyleXRecordSource(
     d,xs,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu,0xdbu,0xdcu,0xddu,singleResult);fault::counting=false;
 if(!single)return 59;
 std::cout<<"style XRECORD single API allocations="<<fault::count<<"; three-query helper allocations="<<allocations<<" (last fault sampled)\n";
 auto it=std::find_if(d.tableStyles.begin(),d.tableStyles.end(),[](const DRW_TableStyle& x){return x.handle==0x6bu;});
 if(it==d.tableStyles.end())return 4;
 it->m_rowStyles[0].m_textHeight=42.;if(live(d))return 5;it->m_rowStyles[0].m_textHeight=4.5;if(!live(d))return 6;
 it->m_rowStyles[0].m_textStyleName="unobserved";it->m_r24ClassVersion=7u;if(!live(d))return 7;
 d.tableStyles.push_back(*it);if(live(d))return 8;d.tableStyles.pop_back();if(!live(d))return 9;
 Probe spoof;spoof.mode=Probe::Mode::Spoof;dx_data bad;if(!enable(spoof)||!spoof.fileImport(argv[1],&bad,false)||!spoof.hit||live(bad)||dx_iface::hasUnchangedVerifiedXRecordSource(bad,0xbcu))return 10;
 spoof.mode=Probe::Mode::Normal;dx_data retry;if(!spoof.fileImport(argv[1],&retry,false)||!live(retry))return 11;
 Probe duplicate;duplicate.mode=Probe::Mode::Duplicate;dx_data dup;if(!enable(duplicate)||!duplicate.fileImport(argv[1],&dup,false)||live(dup))return 12;
 duplicate.mode=Probe::Mode::Normal;dx_data retry2;if(!duplicate.fileImport(argv[1],&retry2,false)||!live(retry2))return 13;
 Probe throwing;throwing.mode=Probe::Mode::Throw;dx_data failed;if(!enable(throwing)||throwing.fileImport(argv[1],&failed,false)||live(failed))return 14;
 throwing.mode=Probe::Mode::Normal;dx_data retry3;if(!throwing.fileImport(argv[1],&retry3,false)||!live(retry3))return 15;
 Probe absent;dx_data missing;if(!enable(absent,0xcau))return 16;(void)absent.fileImport(argv[1],&missing,false);if(live(missing))return 17;
 if(!enable(absent))return 18;dx_data retry4;if(!absent.fileImport(argv[1],&retry4,false)||!live(retry4))return 19;
 FaultProbe injected;dx_data allocation;if(!enable(injected))return 20;fault::fired=false;
 if(!injected.fileImport(argv[1],&allocation,false)||!injected.seen||!fault::fired||live(allocation))return 21;
 injected.inject=false;dx_data retry5;if(!injected.fileImport(argv[1],&retry5,false)||!live(retry5))return 22;
 const char* out="/private/tmp/p5-tablestyle-binding-sentinel.dxf";
 {std::ofstream f(out,std::ios::binary|std::ios::trunc);f<<"STYLE_SENTINEL";if(!f)return 24;}
 const bool exported=r.fileExport(out,DRW::AC1018,false,&d,false,true,dx_dxfSourceTextPolicy::Compatible,
     DRW::DxfTextWritePolicy::Compatible,dx_dxfProxyGraphicsPolicy::Compatible,dx_dwgReadCompletenessPolicy::RequireComplete);
 std::ifstream f(out,std::ios::binary);const std::string sentinel((std::istreambuf_iterator<char>(f)),{});std::remove(out);
 if(exported || sentinel!="STYLE_SENTINEL")return 25;
 Probe nested;nested.mode=Probe::Mode::Nested;nested.path=argv[1];dx_data nestedData;nested.nestedData=&nestedData;
 if(!enable(nested))return 26;(void)nested.fileImport(argv[1],&nestedData,false);
 if(!nested.hit||live(nestedData)||dx_iface::hasUnchangedVerifiedXRecordSource(nestedData,0xbcu))return 27;
 nested.mode=Probe::Mode::Normal;dx_data nestedRetry;if(!nested.fileImport(argv[1],&nestedRetry,false)||!live(nestedRetry))return 28;
 Probe outer;dx_iface inner;dx_data cross;outer.mode=Probe::Mode::Nested;outer.path=argv[1];
 outer.nestedData=&cross;outer.nestedReader=&inner;if(!enable(outer)||!enable(inner))return 29;
 (void)outer.fileImport(argv[1],&cross,false);if(!outer.hit||live(cross)||dx_iface::hasUnchangedVerifiedXRecordSource(cross,0xbcu))return 30;
 outer.mode=Probe::Mode::Normal;dx_data crossRetry;if(!outer.fileImport(argv[1],&crossRetry,false)||!live(crossRetry))return 31;
 Probe plain;dx_data optout;if(!plain.fileImport(argv[1],&optout,false)||!plain.hit||live(optout))return 23;
 std::cout<<"TABLESTYLE XRECORD source, strict tuple, mutation, spoof, duplicate, callback/ticket failure and retry controls passed\n";return 0;}
