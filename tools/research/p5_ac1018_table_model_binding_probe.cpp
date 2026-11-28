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
    static bool wantsModel(const dx_iface& i) noexcept{return i.dx_iface::wantsVerifiedTableEntityModel();}
};
namespace {
const std::vector<std::uint32_t> selected{0xbcu,0xbeu,0xbfu,0xc0u,0xc1u,0xc2u,0xc3u,0xc4u,0xc5u,0xc6u};
bool enable(dx_iface& r){return r.enableVerifiedTableModelSource(selected,0xb9u,0xbdu,0xbau,0x9cu,0xbbu);}
bool live(const dx_data& d){return dx_iface::hasUnchangedVerifiedTableModelSource(d,selected,0xb9u,0xbdu,0xbau,0x9cu,0xbbu);}
class FaultProbe final:public dx_iface {
public:bool inject=true,seen=false;
    void verifiedTableCandidate(const DwgVerifiedTableCandidateTicket& t) noexcept override {
        if(inject && t.kind()==DwgVerifiedTableCandidateKind::TableEntity){seen=true;fault::remaining=0;}
        DxIfaceStandardDwgCaptureTestAccess::deliver(*this,t);fault::remaining=-1;
    }
};
class ParseFaultProbe final:public dx_iface {
public:mutable bool inject=true,seen=false;
    bool wantsVerifiedTableEntityModel() const noexcept override {
        const bool result=DxIfaceStandardDwgCaptureTestAccess::wantsModel(*this);
        if(result && inject && !seen){seen=true;fault::remaining=0;}return result;
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
    if(!spoof.fileImport(argv[1],&changed,false)||!spoof.hit||live(changed)
        || dx_iface::hasUnchangedVerifiedXRecordSource(changed,0xbcu))return 8;
    const auto table=std::find_if(changed.tables.begin(),changed.tables.end(),[](const DRW_Table& t){return t.handle==0x9cu;});
    if(table==changed.tables.end() || table->xDictHandle!=0u)return 15;
    Probe sourceOnly;sourceOnly.mode=Probe::Mode::Spoof;dx_data sourceOnlyData;
    if(!sourceOnly.enableVerifiedTableReciprocalSource(selected,0xb9u,0xbdu,0xbau,0x9cu,0xbbu)
        || !sourceOnly.fileImport(argv[1],&sourceOnlyData,false))return 29;
    DwgTableEntityXDictionaryAc1018Candidate::Result sourceResult;
    if(!dx_iface::readVerifiedTableReciprocalSource(sourceOnlyData,selected,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,sourceResult))return 30;
    const char* out="/private/tmp/p5-reciprocal-output-sentinel.dxf";
    {std::ofstream f(out,std::ios::binary|std::ios::trunc);f<<"RECIPROCAL_SENTINEL";if(!f)return 9;}
    const bool exported=spoof.fileExport(out,DRW::AC1018,false,&changed,false,true,dx_dxfSourceTextPolicy::Compatible,
        DRW::DxfTextWritePolicy::Compatible,dx_dxfProxyGraphicsPolicy::Compatible,dx_dwgReadCompletenessPolicy::RequireComplete);
    const bool validExported=reader.fileExport(out,DRW::AC1018,false,&data,false,true,dx_dxfSourceTextPolicy::Compatible,
        DRW::DxfTextWritePolicy::Compatible,dx_dxfProxyGraphicsPolicy::Compatible,dx_dwgReadCompletenessPolicy::RequireComplete);
    std::ifstream f(out,std::ios::binary);const std::string sentinel((std::istreambuf_iterator<char>(f)),{});std::remove(out);
    if(exported||validExported||sentinel!="RECIPROCAL_SENTINEL")return 10;
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
    Probe missing;if(!missing.enableVerifiedTableModelSource(selected,0xb9u,0xbdu,0xbau,0xcau,0xbbu))return 22;
    dx_data notFound;(void)missing.fileImport(argv[1],&notFound,false);if(live(notFound)||dx_iface::hasUnchangedVerifiedXRecordSource(notFound,0xbcu))return 23;
    if(!enable(missing))return 24;dx_data missingRetry;if(!missing.fileImport(argv[1],&missingRetry,false)||!live(missingRetry))return 25;
    auto selectedTable=std::find_if(data.tables.begin(),data.tables.end(),[](const DRW_Table& t){return t.handle==0x9cu;});
    if(selectedTable==data.tables.end())return 31;const DRW_Table original=*selectedTable;
    const auto check=[&](const auto& change){change(*selectedTable);const bool refused=!live(data)
        && !dx_iface::hasUnchangedVerifiedXRecordSource(data,0xbcu);*selectedTable=original;return refused&&live(data);};
    if(!check([](DRW_Table& t){t.basePoint.x+=1.0;}) || !check([](DRW_Table& t){t.xDictHandle=0u;})
        || !check([](DRW_Table& t){t.proxyGraphics[0]^=1;}) || !check([](DRW_Table& t){t.revokeProxyGraphicsReplayAuthorization();})
        || !check([](DRW_Table& t){t.m_tableRowHeightOverrides[0]+=1.0;})
        || !check([](DRW_Table& t){t.m_tableRowAlignmentOverrides[0]^=1;})
        || !check([](DRW_Table& t){t.m_tableRowTextStyleHandles[0]^=1u;})
        || !check([](DRW_Table& t){t.m_tableRowTextStyleNames[0]+="x";})
        || !check([](DRW_Table& t){t.m_tableRowTextColors[0].method^=1u;})
        || !check([](DRW_Table& t){t.m_tableRowFillNone[0]=!t.m_tableRowFillNone[0];})
        || !check([](DRW_Table& t){t.m_tableRowFillColors[0].flags^=1u;})
        || !check([](DRW_Table& t){t.m_tableBorderColors[17].index^=1u;})
        || !check([](DRW_Table& t){t.m_tableBorderLineweights[17]^=1;})
        || !check([](DRW_Table& t){t.m_tableBorderVisibilities[17]^=1;})
        || !check([](DRW_Table& t){t.m_hasTableBorderLineweightOverrides[17]=!t.m_hasTableBorderLineweightOverrides[17];})
        || !check([](DRW_Table& t){t.m_hasTableBorderVisibilityOverrides[17]=!t.m_hasTableBorderVisibilityOverrides[17];})
        || !check([](DRW_Table& t){t.m_content.m_subrecordRanges[0].m_rawHandleCode^=1u;})
        || !check([](DRW_Table& t){t.m_content.m_rows[0].m_cells[0].m_contents[0].m_text+="x";})
        || !check([](DRW_Table& t){t.m_content.m_rows[0].m_cells[0].m_contents[0].m_value.m_value.addString(1,"changed");}))return 32;
    data.tables.push_back(original);if(live(data))return 33;data.tables.pop_back();if(!live(data))return 34;
    DRW_Entity* projection=nullptr;for(auto* entity:data.mBlock->ent)if(entity && entity->handle==0x9cu){projection=entity;break;}
    if(!projection)for(auto* block:data.blocks)if(block)for(auto* entity:block->ent)if(entity && entity->handle==0x9cu){projection=entity;break;}
    if(!projection)return 35;const auto projectionHandle=projection->xDictHandle;projection->xDictHandle^=1u;
    if(!live(data))return 36;projection->xDictHandle=projectionHandle;
    ParseFaultProbe parseFault;if(!enable(parseFault))return 37;dx_data parseFailure;fault::fired=false;
    if(!parseFault.fileImport(argv[1],&parseFailure,false)||!parseFault.seen||!fault::fired||live(parseFailure)
        || dx_iface::hasUnchangedVerifiedXRecordSource(parseFailure,0xbcu))return 38;
    parseFault.inject=false;dx_data parseRetry;if(!parseFault.fileImport(argv[1],&parseRetry,false)||!live(parseRetry))return 39;
    FaultProbe injected;if(!enable(injected))return 26;dx_data allocationFailure;fault::fired=false;
    if(!injected.fileImport(argv[1],&allocationFailure,false)||!injected.seen||!fault::fired
        || live(allocationFailure)||dx_iface::hasUnchangedVerifiedXRecordSource(allocationFailure,0xbcu))return 27;
    injected.inject=false;dx_data allocationRetry;if(!injected.fileImport(argv[1],&allocationRetry,false)||!live(allocationRetry))return 28;
    Probe prior;dx_data plain;if(!prior.fileImport(argv[1],&plain,false)||!prior.hit||!prior.unknown||live(plain))return 14;
    std::cout<<"physical model-bound source, block replay failure/retry, spoof, cross/same reentry, selection, mutation, ticket allocation failure and strict-output controls passed\n";
    return 0;
}
