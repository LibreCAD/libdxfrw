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
namespace fault {long remaining=-1;bool fired=false;}
void* operator new(std::size_t n){if(fault::remaining==0){fault::remaining=-1;fault::fired=true;throw std::bad_alloc();}
    if(fault::remaining>0)--fault::remaining;if(void* p=std::malloc(n?n:1u))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
class DxIfaceStandardDwgCaptureTestAccess {
public:static void deliver(dx_iface& i,const DwgVerifiedTableCandidateTicket& t) noexcept{i.dx_iface::verifiedTableCandidate(t);}
};
namespace {
const std::vector<std::uint32_t> xs{0xbcu,0xbeu,0xbfu,0xc0u,0xc1u,0xc2u,0xc3u,0xc4u,0xc5u,0xc6u};
bool enable(dx_iface& r,std::uint32_t h=0x6bu){return r.enableVerifiedTableStyleSource(xs,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,h);}
bool live(const dx_data& d){DwgTableStyleAc1018Candidate::Result r;
 return dx_iface::readVerifiedTableStyleSource(d,xs,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu,r)
  && r.handle==0x6bu && r.owner==0x6au && r.tableReactor==0x9cu && r.xdictionary==0xdbu
  && r.textStyles==std::array<std::uint32_t,3>{{0x11u,0x11u,0x11u}}
  && dx_iface::hasUnchangedVerifiedXRecordSource(d,0xbcu) && dx_iface::hasUnchangedVerifiedXRecordSources(d,xs);}
class Probe final:public dx_iface {
public:
 enum class Mode{Normal,Spoof,Throw,Duplicate,Nested};Mode mode{Mode::Normal};bool hit=false;
 dx_data* nestedData=nullptr;dx_iface* nestedReader=nullptr;std::string path;
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
  if(inject && t.kind()==DwgVerifiedTableCandidateKind::TableStyle){seen=true;fault::remaining=0;}
  DxIfaceStandardDwgCaptureTestAccess::deliver(*this,t);fault::remaining=-1;
 }
};
}
int main(int argc,char**argv){if(argc!=2)return 2;Probe r;dx_data d;
 if(!enable(r)||!r.fileImport(argv[1],&d,false)||!r.hit||!live(d))return 3;
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
 std::cout<<"TABLESTYLE selected source, strict tuple, mutation, spoof, duplicate, callback/ticket failure and retry controls passed\n";return 0;}
