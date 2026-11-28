// Scratch-only source observations and import-failure controls. No output or
// TABLE typed-model semantics are qualified by this probe.
#include "dx_iface.h"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstdlib>
#include <new>
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
const std::vector<std::uint32_t> selected{0xbcu,0xbeu,0xbfu,0xc0u,0xc1u,0xc2u,0xc3u,0xc4u,0xc5u,0xc6u};
bool enable(dx_iface& r){return r.enableVerifiedTableReciprocalSource(selected,0xb9u,0xbdu,0xbau,0x9cu,0xbbu);}
bool live(const dx_data& d){DwgTableEntityXDictionaryAc1018Candidate::Result r;
    return dx_iface::readVerifiedTableReciprocalSource(d,selected,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,r)
        && r.handle==0x9cu && r.dictionary==0xbbu && r.dataEndBit==18156u
        && r.dictionaryStartBit==18156u && r.dictionaryEndBit==18172u
        && r.layer==0x10u && r.block==0x9du && r.style==0x6bu;}
class FaultProbe final:public dx_iface {
public:bool inject=true,seen=false;
    void verifiedTableCandidate(const DwgVerifiedTableCandidateTicket& t) noexcept override {
        if(inject && t.kind()==DwgVerifiedTableCandidateKind::TableEntity){seen=true;fault::remaining=0;}
        DxIfaceStandardDwgCaptureTestAccess::deliver(*this,t);fault::remaining=-1;
    }
};
class Probe final:public dx_iface {
public:
    enum class Mode {Normal,Throw,Spoof,Nested};Mode mode{Mode::Normal};bool hit=false,unknown=false;
    dx_data* nestedData=nullptr;dx_iface* nestedReader=nullptr;std::string path;
    void addTable(const DRW_Table& value) override {
        if(value.handle==0x9cu){hit=true;
            if(mode==Mode::Throw)throw std::runtime_error("block replay failure");
            if(mode==Mode::Nested){(void)(nestedReader?nestedReader:this)->fileImport(path,nestedData,false);return;}
            if(mode==Mode::Spoof){auto altered=value;altered.xDictHandle=0u;dx_iface::addTable(altered);return;}}
        dx_iface::addTable(value);
    }
    void addDwgFramePublication(const DRW_DwgFramePublication& p) override {
        if(p.m_handle==0x9cu)unknown=p.m_commonLinkEvidence==DRW_DwgCommonLinkEvidence::Unknown;
        dx_iface::addDwgFramePublication(p);
    }
};
}
int main(int argc,char**argv) {
    if(argc!=2)return 2;
    Probe reader;reader.path=argv[1];dx_data data;if(!enable(reader) || !reader.fileImport(argv[1],&data,false)
        || !reader.hit || !reader.unknown || !live(data))return 3;
    Probe throwing;throwing.mode=Probe::Mode::Throw;if(!enable(throwing))return 4;dx_data failed;
    const bool read=throwing.fileImport(argv[1],&failed,false);
    if(read || !throwing.hit || live(failed) || dx_iface::hasUnchangedVerifiedXRecordSource(failed,0xbcu))return 5;
    throwing.mode=Probe::Mode::Normal;dx_data retry;if(!throwing.fileImport(argv[1],&retry,false)||!live(retry))return 6;
    Probe spoof;spoof.mode=Probe::Mode::Spoof;if(!enable(spoof))return 7;dx_data changed;
    if(!spoof.fileImport(argv[1],&changed,false)||!spoof.hit||!live(changed))return 8;
    const auto table=std::find_if(changed.tables.begin(),changed.tables.end(),[](const DRW_Table& t){return t.handle==0x9cu;});
    if(table==changed.tables.end() || table->xDictHandle!=0u)return 15;
    const char* out="/private/tmp/p5-reciprocal-output-sentinel.dxf";
    {std::ofstream f(out,std::ios::binary|std::ios::trunc);f<<"RECIPROCAL_SENTINEL";if(!f)return 9;}
    const bool exported=spoof.fileExport(out,DRW::AC1018,false,&changed,false,true,dx_dxfSourceTextPolicy::Compatible,
        DRW::DxfTextWritePolicy::Compatible,dx_dxfProxyGraphicsPolicy::Compatible,dx_dwgReadCompletenessPolicy::RequireComplete);
    std::ifstream f(out,std::ios::binary);const std::string sentinel((std::istreambuf_iterator<char>(f)),{});std::remove(out);
    if(exported||sentinel!="RECIPROCAL_SENTINEL")return 10;
    Probe nested;nested.mode=Probe::Mode::Nested;nested.path=argv[1];dx_data nestedData;nested.nestedData=&nestedData;
    if(!enable(nested))return 11;(void)nested.fileImport(argv[1],&nestedData,false);
    if(!nested.hit||live(nestedData)||dx_iface::hasUnchangedVerifiedXRecordSource(nestedData,0xbcu))return 12;
    nested.mode=Probe::Mode::Normal;dx_data nestedRetry;if(!nested.fileImport(argv[1],&nestedRetry,false)||!live(nestedRetry))return 13;
    Probe outer;dx_iface inner;dx_data cross;outer.mode=Probe::Mode::Nested;outer.path=argv[1];
    outer.nestedData=&cross;outer.nestedReader=&inner;if(!enable(outer)||!enable(inner))return 16;
    (void)outer.fileImport(argv[1],&cross,false);if(!outer.hit||live(cross)||dx_iface::hasUnchangedVerifiedXRecordSource(cross,0xbcu))return 17;
    outer.mode=Probe::Mode::Normal;dx_data crossRetry;if(!outer.fileImport(argv[1],&crossRetry,false)||!live(crossRetry))return 18;
    auto root=std::find_if(data.xRecords.begin(),data.xRecords.end(),[](const DRW_XRecord& x){return x.handle==0xbcu;});
    if(root==data.xRecords.end())return 19;root->m_cloning^=1;if(live(data))return 20;root->m_cloning^=1;if(!live(data))return 21;
    Probe missing;if(!missing.enableVerifiedTableReciprocalSource(selected,0xb9u,0xbdu,0xbau,0xcau,0xbbu))return 22;
    dx_data notFound;(void)missing.fileImport(argv[1],&notFound,false);if(live(notFound)||dx_iface::hasUnchangedVerifiedXRecordSource(notFound,0xbcu))return 23;
    if(!enable(missing))return 24;dx_data missingRetry;if(!missing.fileImport(argv[1],&missingRetry,false)||!live(missingRetry))return 25;
    FaultProbe injected;if(!enable(injected))return 26;dx_data allocationFailure;fault::fired=false;
    if(!injected.fileImport(argv[1],&allocationFailure,false)||!injected.seen||!fault::fired
        || live(allocationFailure)||dx_iface::hasUnchangedVerifiedXRecordSource(allocationFailure,0xbcu))return 27;
    injected.inject=false;dx_data allocationRetry;if(!injected.fileImport(argv[1],&allocationRetry,false)||!live(allocationRetry))return 28;
    Probe prior;dx_data plain;if(!prior.fileImport(argv[1],&plain,false)||!prior.hit||!prior.unknown||live(plain))return 14;
    std::cout<<"physical reciprocal source, block replay failure/retry, spoof, cross/same reentry, selection, mutation, ticket allocation failure and strict-output controls passed\n";
    return 0;
}
