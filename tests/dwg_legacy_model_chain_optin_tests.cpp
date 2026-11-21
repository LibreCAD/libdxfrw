// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include "intern/dwgbufferw.h"
#include <algorithm>
#include <atomic>
#include <thread>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {
std::array<std::uint32_t, 3> handles{{0x130u, 0x451u, 0x999u}};

bool scopeControls() {
    dwgBufferW body, handlesBuffer, other;
    const auto equals = [](const dwgBufferW* buffer, std::uint32_t a, std::uint32_t b) {
        std::uint32_t prev=0,next=0;
        return ScopedDwgLegacyEntityLinks::lookup(buffer,prev,next) && prev==a && next==b;
    };
    std::uint32_t prev=0,next=0;
    if (ScopedDwgLegacyEntityLinks::lookup(&body,prev,next)
        || ScopedDwgLegacyEntityLinks::lookup(nullptr,prev,next)) return false;
    for (std::uint32_t i=1; i<=128; ++i) {
        {
            ScopedDwgLegacyEntityLinks outer(&body,&handlesBuffer,i,i+1u);
            if(!equals(&body,i,i+1u)||!equals(&handlesBuffer,i,i+1u)
                || ScopedDwgLegacyEntityLinks::lookup(&other,prev,next)
                || ScopedDwgLegacyEntityLinks::lookup(nullptr,prev,next)) return false;
            {
                ScopedDwgLegacyEntityLinks inner(&body,&handlesBuffer,i+2u,i+3u);
                if(!equals(&body,i+2u,i+3u)||!equals(&handlesBuffer,i+2u,i+3u)) return false;
            }
            if(!equals(&body,i,i+1u)||!equals(&handlesBuffer,i,i+1u)) return false;
            {
                ScopedDwgLegacyEntityLinks different(&other,nullptr,44u,45u);
                if(!equals(&other,44u,45u)||!equals(&body,i,i+1u)
                    || ScopedDwgLegacyEntityLinks::lookup(nullptr,prev,next)) return false;
            }
            try {
                ScopedDwgLegacyEntityLinks inner(&body,&handlesBuffer,90u,91u);
                if(!equals(&body,90u,91u)) return false;
                throw 1;
            } catch(int) {}
            if(!equals(&body,i,i+1u)) return false;
        }
        if(ScopedDwgLegacyEntityLinks::lookup(&body,prev,next)) return false;
    }
    {
        ScopedDwgLegacyEntityLinks nullScope(nullptr,nullptr,1u,2u);
        if(ScopedDwgLegacyEntityLinks::lookup(nullptr,prev,next)) return false;
    }
    {
        dwgBufferW stagedBody, stagedHandles;
        ScopedDwgLegacyEntityLinks outer(&body, &handlesBuffer, 17u, 19u);
        {
            ScopedDwgStagedLegacyEntityLinks staged(&body, &handlesBuffer, &stagedBody, &stagedHandles);
            if (!staged.valid() || !equals(&stagedBody, 17u, 19u)
                || !equals(&stagedHandles, 17u, 19u)) return false;
        }
        if (ScopedDwgLegacyEntityLinks::lookup(&stagedBody, prev, next)) return false;
        {
            ScopedDwgLegacyEntityLinks conflicting(&handlesBuffer, nullptr, 21u, 22u);
            ScopedDwgStagedLegacyEntityLinks staged(&body, &handlesBuffer, &stagedBody, &stagedHandles);
            if (staged.valid() || ScopedDwgLegacyEntityLinks::lookup(&stagedBody, prev, next)) return false;
        }
        if (!equals(&body, 17u, 19u) || !equals(&handlesBuffer, 17u, 19u)) return false;
    }
    std::array<std::atomic<dwgBufferW*>,2> pointers{};
    std::atomic<unsigned> ready{0}, checked{0}; std::atomic<bool> release{false}, ok{true};
    std::array<std::thread,2> threads;
    for(unsigned i=0;i<2;++i) threads[i]=std::thread([&,i] {
        dwgBufferW local, localHandles;
        pointers[i].store(&local,std::memory_order_release);
        {
            ScopedDwgLegacyEntityLinks scope(&local,&localHandles,100u+i,200u+i);
            ready.fetch_add(1,std::memory_order_release);
            while(!release.load(std::memory_order_acquire)) std::this_thread::yield();
            std::uint32_t p=0,n=0;
            if(!equals(&local,100u+i,200u+i)||!equals(&localHandles,100u+i,200u+i)
                || ScopedDwgLegacyEntityLinks::lookup(pointers[1u-i].load(std::memory_order_acquire),p,n))
                ok.store(false,std::memory_order_relaxed);
            checked.fetch_add(1,std::memory_order_release);
            while(checked.load(std::memory_order_acquire)!=2u) std::this_thread::yield();
        }
        std::uint32_t p=0,n=0;
        if(ScopedDwgLegacyEntityLinks::lookup(&local,p,n)) ok.store(false,std::memory_order_relaxed);
    });
    while(ready.load(std::memory_order_acquire)!=2u) std::this_thread::yield();
    if(ScopedDwgLegacyEntityLinks::lookup(pointers[0].load(std::memory_order_acquire),prev,next)
        || ScopedDwgLegacyEntityLinks::lookup(pointers[1].load(std::memory_order_acquire),prev,next))
        ok.store(false,std::memory_order_relaxed);
    release.store(true,std::memory_order_release);
    for(auto& thread:threads) thread.join();
    return ok.load(std::memory_order_relaxed);
}

enum class Mode { Normal, WrongType, WrongOrder, WrongHandle, Missing, Extra,
                  Paper, OtherOwner, Polyline, Insert, IgnoredBadPolyline,
                  IgnoredBadInsert, EarlyNodAlias, EarlyGroupAlias, LateNodAlias,
                  LateGroupAlias, DictionaryAlias, DefaultDictionaryAlias,
                  XrecordOwnerAlias, ImageDefOwnerAlias, IgnoredBlock,
                  IgnoredRawObject, IgnoredRawSection, GroupMember, EmptyPlan, DuplicatePlan,
                  UnsupportedPlan, ZeroPlan, NoHandlePlan, ReservedPlan };

struct Adapter : LC3CompatCheck, DRW_DwgLegacyModelChainProvider,
                 DRW_DwgDefaultPlotStyleProvider {
    explicit Adapter(dwgRW& writer) : output(writer) {}
    std::vector<DRW_DwgLegacyModelChainEntry> dwgLegacyModelChain() const override {
        std::vector<DRW_DwgLegacyModelChainEntry> plan{
            {handles[0],DRW::LINE},{handles[1],DRW::CIRCLE},{handles[2],DRW::ARC}};
        if (mode==Mode::EmptyPlan) plan.clear();
        if (mode==Mode::DuplicatePlan) plan[1].handle=plan[0].handle;
        if (mode==Mode::UnsupportedPlan) plan[1].type=DRW::INSERT;
        if (mode==Mode::ZeroPlan) plan[1].handle=0;
        if (mode==Mode::NoHandlePlan) plan[1].handle=DRW::NoHandle;
        if (mode==Mode::ReservedPlan) plan[1].handle=0x12u;
        return plan;
    }
    void writeDwgClasses() override {
        if(mode==Mode::EarlyNodAlias)
            (void)output.registerDwgNamedObjectDictionaryEntry("ILLEGAL",handles[1]);
        if(mode==Mode::EarlyGroupAlias)
            (void)output.registerDwgGroupDictionaryEntry("ILLEGAL",handles[1]);
        if(mode==Mode::GroupMember)
            groupRegistered=output.registerDwgGroupDictionaryEntry("SAFE_GROUP",0xB300u);
        if(mode==Mode::DefaultDictionaryAlias) {
            DRW_DictionaryWithDefault dictionary; dictionary.handle=0xB301u;
            (void)output.registerDictionaryWithDefaultObjectClass(&dictionary);
        }
    }
    void writeLTypes() override {
        if(mode==Mode::LateNodAlias)
            rejectedAlias=!output.registerDwgNamedObjectDictionaryEntry("ILLEGAL",handles[1]);
        if(mode==Mode::LateGroupAlias)
            rejectedAlias=!output.registerDwgGroupDictionaryEntry("ILLEGAL",handles[1]);
    }
    void writeObjects() override {
        if(mode==Mode::DictionaryAlias) {
            DRW_Dictionary d; d.handle=0xB302u; d.parentHandle=DRW::DwgNamedObjectsDictionaryHandle;
            d.m_entries.push_back({"ILLEGAL",handles[1]}); rejectedAlias=!output.writeDictionary(&d);
        }
        if(mode==Mode::DefaultDictionaryAlias) {
            DRW_DictionaryWithDefault d; d.handle=0xB301u; d.parentHandle=DRW::DwgNamedObjectsDictionaryHandle;
            d.m_entries.push_back({"ILLEGAL",handles[1]}); d.m_defaultEntryHandle=handles[1];
            rejectedAlias=!output.writeDictionaryWithDefault(&d);
        }
        if(mode==Mode::GroupMember) {
            DRW_Group g; g.handle=0xB300u; g.parentHandle=DRW::DwgAcadGroupDictionaryHandle;
            g.name="SAFE_GROUP"; g.m_entityHandles={handles[1]};
            groupWritten=output.writeGroup(&g);
        }
    }
    void writeBlocks() override {
        if(mode==Mode::IgnoredBlock)
            rejectedAlias=output.defineBlock("UNPLANNED_BLOCK",DRW_Coord{})==0u;
    }
    void writeEntities() override {
        receipts.clear(); accepted.clear();
        DRW_Line line; line.handle=handles[0]; line.basePoint={1,2,3}; line.secPoint={4,5,6};
        DRW_Circle circle; circle.handle=handles[1]; circle.basePoint={7,8,0}; circle.radious=2.5;
        DRW_Arc arc; arc.handle=handles[2]; arc.basePoint={10,11,0}; arc.radious=4;
        arc.staangle=.25; arc.endangle=1.25;
        if (mode==Mode::Paper) line.space=DRW::PaperSpace;
        if (mode==Mode::OtherOwner) line.parentHandle=0xB200u;
        if (mode==Mode::WrongHandle) line.handle=0x131u;
        auto writeLine=[&] { record(output.writeLine(&line)); };
        auto writeCircle=[&] { record(output.writeCircle(&circle)); };
        auto writeArc=[&] { record(output.writeArc(&arc)); };
        if (mode==Mode::WrongOrder) { writeCircle(); writeLine(); writeArc(); return; }
        if (mode==Mode::WrongType) { circle.handle=handles[0]; writeCircle(); return; }
        if (mode==Mode::Polyline) { DRW_Polyline p; p.handle=handles[0]; record(output.writePolyline(&p)); return; }
        if (mode==Mode::Insert) { DRW_Insert i; i.handle=handles[0]; record(output.writeInsert(&i)); return; }
        if (mode==Mode::IgnoredBadPolyline) {
            DRW_Polyline p; p.handle=0xB201u; p.vertlist.push_back(nullptr);
            (void)output.writePolyline(&p);
        }
        if (mode==Mode::IgnoredBadInsert) {
            DRW_Insert i; i.handle=0xB202u; i.attlist.push_back(nullptr);
            (void)output.writeInsert(&i);
        }
        if (mode==Mode::XrecordOwnerAlias) {
            DRW_XRecord x; x.handle=0xB303u; x.parentHandle=handles[1];
            rejectedAlias=!output.writeXRecord(&x);
        }
        if (mode==Mode::ImageDefOwnerAlias) {
            DRW_ImageDef image; image.handle=0xB304u; image.parentHandle=handles[1];
            image.name="local-image.png";
            rejectedAlias=!output.writeImageDef(&image) && image.handle==0xB304u
                && image.parentHandle==handles[1];
        }
        if (mode==Mode::IgnoredRawObject) {
            DRW_UnsupportedObject raw; rejectedAlias=!output.writeRawDwgObject(&raw);
        }
        if (mode==Mode::IgnoredRawSection) {
            DRW_RawDwgSection raw; raw.m_name="AcDb:OpaqueProbe";
            raw.m_version=DRW::AC1015; raw.m_data={1,2,3};
            rejectedAlias=!output.writeRawDwgSection(&raw);
        }
        writeLine(); writeCircle();
        if (mode!=Mode::Missing) writeArc();
        if (mode==Mode::Extra) { DRW_Line extra; extra.handle=0xB220u; record(output.writeLine(&extra)); }
    }
    void record(bool ok) {
        accepted.push_back(ok);
        if (!ok) return;
        DRW::DwgObjectFrameReceipt receipt;
        if (!output.getLastDwgObjectFrame(receipt)) { validReceipts=false; return; }
        receipts.push_back(receipt);
    }
    bool verifyReceipts() const {
        if (!validReceipts || receipts.size()!=3u || accepted!=std::vector<bool>{true,true,true}) return false;
        for (std::size_t i=0;i<3;++i) {
            const auto& r=receipts[i];
            if(!r.valid || r.version!=DRW::AC1015 || r.objectHandle!=handles[i]) return false;
            std::vector<std::uint64_t> links;
            for(const auto& occurrence:r.occurrences)
                if(occurrence.stream==DRW::DwgObjectHandleStream::Handles
                   && occurrence.token.code==DRW::DwgSoftPointer)
                    links.push_back(occurrence.token.reference);
            const auto prev=i?handles[i-1]:0u;
            const auto next=i+1u<handles.size()?handles[i+1]:0u;
            if(links!=std::vector<std::uint64_t>{prev,next}) return false;
        }
        // Copies taken during each callback must retain their original IDs and
        // neighbor evidence after subsequent frames have been emitted.
        return receipts[0].objectHandle==handles[0] && receipts[0].generation!=receipts[1].generation;
    }
    dwgRW& output; Mode mode=Mode::Normal; bool validReceipts=true;
    bool rejectedAlias=false, groupRegistered=false, groupWritten=false;
    std::vector<bool> accepted;
    std::vector<DRW::DwgObjectFrameReceipt> receipts;
};
struct SingletonAdapter : LC3CompatCheck, DRW_DwgLegacyModelChainProvider,
                          DRW_DwgDefaultPlotStyleProvider {
    explicit SingletonAdapter(dwgRW& writer) : output(writer) {}
    std::vector<DRW_DwgLegacyModelChainEntry> dwgLegacyModelChain() const override {
        return {{0x130u, DRW::LINE}};
    }
    void writeEntities() override {
        DRW_Line line; line.handle=0x130u; line.basePoint={1,2,3}; line.secPoint={4,5,6};
        if(!output.writeLine(&line)) return;
        DRW::DwgObjectFrameReceipt receipt;
        if(!output.getLastDwgObjectFrame(receipt) || !receipt.valid || receipt.objectHandle!=line.handle) return;
        std::vector<std::uint64_t> links;
        for(const auto& occurrence:receipt.occurrences)
            if(occurrence.stream==DRW::DwgObjectHandleStream::Handles
               && occurrence.token.code==DRW::DwgSoftPointer)
                links.push_back(occurrence.token.reference);
        valid = links==std::vector<std::uint64_t>{0u,0u};
    }
    dwgRW& output; bool valid=false;
};

struct Capture : LC3CompatCheck {
    void addLine(const DRW_Line& e) override { if(e.handle==handles[0]) ++line; }
    void addCircle(const DRW_Circle& e) override { if(e.handle==handles[1]) ++circle; }
    void addArc(const DRW_Arc& e) override { if(e.handle==handles[2]) ++arc; }
    void addGroup(const DRW_Group& value) override {
        if(value.handle==0xB300u && value.m_entityHandles==std::vector<std::uint32_t>{handles[1]})
            ++group;
    }
    int line=0,circle=0,arc=0,group=0;
};
bool inspect(const std::string& path) {
    dwgRW reader(path.c_str()); Capture capture;
    return reader.read(&capture,false) && capture.line==1 && capture.circle==1 && capture.arc==1;
}
}
int main(int argc,char**argv) {
    const bool keep=argc==2 && std::string(argv[1])=="--keep";
    if(argc>2 || (argc==2&&!keep)) return 2;
    if(!scopeControls()) {std::cerr<<"thread-local scope control failed\n";return 1;}
    const auto dir=std::filesystem::temp_directory_path() /
        ("libdxfrw-legacy-chain-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    if(!std::filesystem::create_directory(dir))return 2;
    struct Cleanup {std::filesystem::path p;bool keep;~Cleanup(){if(!keep){std::error_code e;std::filesystem::remove_all(p,e);}}} cleanup{dir,keep};
    const auto path=(dir/"chain.dwg").string(); dwgRW output(path.c_str()); Adapter adapter(output);
    auto success=[&] {
        adapter.mode=Mode::Normal; adapter.validReceipts=true;
        const bool wrote = output.write(&adapter,DRW::AC1015,true);
        const bool receipts = wrote && adapter.verifyReceipts();
        const bool readback = receipts && inspect(path);
        if (!readback) std::cerr << "wrote=" << wrote << " receipts=" << receipts
                                 << " readback=" << readback << " error=" << output.getError() << '\n';
        return readback;
    };
    if(!success()) {std::cerr<<"normal write, frame receipt, or readback failed\n";return 1;}
    handles={{0x451u,0x130u,0x999u}};
    if(!success()) {std::cerr<<"nonmonotonic chain failed\n";return 1;}
    if(keep) std::filesystem::copy_file(path,dir/"chain-nonmonotonic.dwg");
    handles={{0x130u,0x451u,0x999u}};
    adapter.mode=Mode::GroupMember;
    if(!output.write(&adapter,DRW::AC1015,true)||!adapter.verifyReceipts()
       ||!adapter.groupRegistered||!adapter.groupWritten||!inspect(path)) {
        std::cerr<<"valid GROUP membership failed\n";return 1;
    }
    {dwgRW groupReader(path.c_str()); Capture captured;
     if(!groupReader.read(&captured,false)||captured.group!=1) return 1;}
    if(keep) std::filesystem::copy_file(path,dir/"chain-group.dwg");
    for (auto mode : {Mode::WrongType,Mode::WrongOrder,Mode::WrongHandle,Mode::Missing,
         Mode::Extra,Mode::Paper,Mode::OtherOwner,Mode::Polyline,Mode::Insert,
         Mode::IgnoredBadPolyline,Mode::IgnoredBadInsert,
         Mode::EarlyNodAlias,Mode::EarlyGroupAlias,Mode::LateNodAlias,
         Mode::LateGroupAlias,Mode::DictionaryAlias,Mode::DefaultDictionaryAlias,
         Mode::XrecordOwnerAlias,Mode::ImageDefOwnerAlias,Mode::IgnoredBlock,
         Mode::IgnoredRawObject,Mode::IgnoredRawSection,Mode::EmptyPlan,
         Mode::DuplicatePlan,Mode::UnsupportedPlan,Mode::ZeroPlan,Mode::NoHandlePlan,Mode::ReservedPlan}) {
        {std::ofstream f(path,std::ios::binary);f<<"prior destination";}
        adapter.mode=mode;adapter.rejectedAlias=false;
        if(output.write(&adapter,DRW::AC1015,true)) {std::cerr<<"accepted bad mode="<<int(mode)<<"\n";return 1;}
        {std::ifstream f(path,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(f),{})!="prior destination")return 1;}
        if((mode==Mode::LateNodAlias||mode==Mode::LateGroupAlias
            ||mode==Mode::DictionaryAlias||mode==Mode::DefaultDictionaryAlias
            ||mode==Mode::XrecordOwnerAlias||mode==Mode::ImageDefOwnerAlias
            ||mode==Mode::IgnoredBlock||mode==Mode::IgnoredRawObject||mode==Mode::IgnoredRawSection)
            && !adapter.rejectedAlias) {std::cerr<<"alias not rejected mode="<<int(mode)<<"\n";return 1;}
        if(!success()) {std::cerr<<"same-facade retry failed mode="<<int(mode)<<"\n";return 1;}
    }
    {std::ofstream f(path,std::ios::binary);f<<"prior destination";}
    if(output.write(&adapter,DRW::AC1024,true)) return 1;
    {std::ifstream f(path,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(f),{})!="prior destination")return 1;}
    if(!success())return 1;
    const auto singletonPath=(dir/"chain-singleton.dwg").string();
    dwgRW singletonOutput(singletonPath.c_str()); SingletonAdapter singleton(singletonOutput);
    if(!singletonOutput.write(&singleton,DRW::AC1015,true)||!singleton.valid) return 1;
    if(keep)std::cout<<"OUTPUT "<<path<<'\n';
    std::cout<<"AC1015 bounded legacy model chain PASS\n";return 0;
}
