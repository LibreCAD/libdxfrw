#include "dx_iface.h"
#include "intern/dwgbufferw.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <cstdlib>
#include <new>
#include <stdexcept>
#include <string>

// Test-only, one-shot fault injection; armed at the optional sink query, after
// the selected record's normal callback and source lease have completed.
namespace fault {
long remaining = -1;
bool fired = false;
bool observe = false;
std::size_t largest = 0;
std::size_t allocations = 0;
bool (*predicate)() noexcept = nullptr;
std::size_t predicateSize = 0;
}
void* operator new(std::size_t n) {
    if (fault::observe) { ++fault::allocations; if (n > fault::largest) fault::largest = n; }
    if (fault::predicate && fault::predicate()) {
        fault::predicate=nullptr;
        fault::predicateSize=n;
        fault::fired=true;
        throw std::bad_alloc();
    }
    if (fault::remaining == 0) {
        fault::remaining = -1;
        fault::fired = true;
        throw std::bad_alloc();
    }
    if (fault::remaining > 0) --fault::remaining;
    if (void* p = std::malloc(n ? n : 1u)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

class DxIfaceStandardDwgCaptureTestAccess {
public:
    static DRW_UnsupportedObject& raw(dx_data& d, std::uint32_t h) {
        return d.m_standardDwgObjectFrames.at({d.importGeneration,h}).raw;
    }
    static DRW_DwgFramePublication& publication(dx_data& d, std::uint32_t h) {
        return d.m_standardDwgObjectFrames.at({d.importGeneration,h}).publication;
    }
    static void transplant(dx_data& to, const dx_data& from) {
        to.m_verifiedXRecordSource = from.m_verifiedXRecordSource;
    }
    static void deliver(dx_iface& i, const DwgVerifiedXRecordTicket& ticket) noexcept {
        i.dx_iface::verifiedXRecord(ticket);
    }
    static bool selection(dx_iface& i,const std::vector<std::uint32_t>& expected,bool strong) noexcept {
        return i.m_verifiedXRecordSelected==expected && i.m_verifiedStyleXRecordRequired==strong
            && i.m_verifiedStyleDictionarySelected==0xdbu && i.m_verifiedStyleXRecordSelected==0xdcu
            && i.m_verifiedCellStyleMapSelected==0xddu;
    }
    static bool sealReady(dx_iface& i, std::uint32_t h) noexcept {
        const auto* data=i.m_verifiedXRecordReadData;
        if (!data || i.m_verifiedXRecordPending.empty() || data->m_verifiedXRecordSource
            || !data->dwgReadOutcome.operationSucceeded) return false;
        const auto it=data->m_standardDwgObjectFrames.find({data->importGeneration,h});
        return it!=data->m_standardDwgObjectFrames.end() && std::next(it)==data->m_standardDwgObjectFrames.end()
            && it->second.typedBaseline && it->second.payloadMatched && it->second.sourceProfileBound;
    }
};
namespace {
constexpr std::uint32_t handle=0xA602u;
bool styleXRecordSelectorControl() {
    const std::vector<std::uint32_t> ten{0xbcu,0xbeu,0xbfu,0xc0u,0xc1u,0xc2u,0xc3u,0xc4u,0xc5u,0xc6u};
    auto eleven=ten;eleven.push_back(0xdcu);
    const auto weak=[&](dx_iface& i){return i.enableVerifiedTableStyleDictionarySource(ten,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu,0xdbu,0xdcu,0xddu);};
    const auto strong=[&](dx_iface& i){return i.enableVerifiedTableStyleXRecordSource(ten,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu,0xdbu,0xdcu,0xddu);};
    dx_iface counted;if(!weak(counted))return false;fault::allocations=0;fault::observe=true;const bool good=strong(counted);fault::observe=false;
    const auto count=fault::allocations;if(!good || !count || !DxIfaceStandardDwgCaptureTestAccess::selection(counted,eleven,true))return false;
    for(std::size_t n=0;n<count;++n){dx_iface a;if(!weak(a))return false;fault::remaining=static_cast<long>(n);fault::fired=false;
        const bool selected=strong(a);fault::remaining=-1;
        if(selected || !fault::fired || !DxIfaceStandardDwgCaptureTestAccess::selection(a,ten,false) || !strong(a)
            || !DxIfaceStandardDwgCaptureTestAccess::selection(a,eleven,true))return false;}
    if(counted.enableVerifiedTableStyleXRecordSource(ten,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu,0xdbu,0xbcu,0xddu)
        || !DxIfaceStandardDwgCaptureTestAccess::selection(counted,eleven,true) || !weak(counted)
        || !DxIfaceStandardDwgCaptureTestAccess::selection(counted,ten,false))return false;
    return true;
}
class Writer final : public dx_iface {
public:
    explicit Writer(dwgRW& w, std::size_t payloadSize=0u, unsigned count=2u,
                    bool repeatPayload=false)
        :w_(w),payloadSize_(payloadSize),count_(count),repeatPayload_(repeatPayload) {}
    void writeHeader(DRW_Header& h) override {h.vars.clear();}
    void writeDwgClasses() override {
        registered=true;
        for (unsigned i=0;i<count_;++i)
            registered=registered && w_.registerDwgNamedObjectDictionaryEntry(
                "SOURCE_"+std::to_string(i),handle+i);
    }
    void collectDwgAppIds() override {}
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override {}
    void writeVports() override {}
    void writeViews() override {}
    void writeUCSs() override {}
    void writeDimstyles() override {}
    void writeAppId() override {}
    void writeObjects() override {
        DRW_XRecord x; x.handle=handle; x.parentHandle=DRW::DwgNamedObjectsDictionaryHandle;
        written=registered;
        for (unsigned i=0;i<count_;++i) {
            x.handle=handle+i; x.m_values.clear();
            if (payloadSize_ && (i==0u || repeatPayload_))
                x.m_values.emplace_back(310,std::vector<std::uint8_t>(payloadSize_,0xA5u));
            else x.m_values.emplace_back(1,UTF8STRING("LOCALLY_GENERATED"));
            written=written && w_.writeXRecord(&x);
        }
    }
    dwgRW& w_; std::size_t payloadSize_; unsigned count_; bool repeatPayload_;
    bool registered=false, written=false;
};
class FaultSink final : public dx_iface {
public:
    long failAt=-1; bool captureEnabled=true; mutable bool selected=false; bool following=false;
    bool wantsVerifiedXRecord(std::uint32_t h) const noexcept override {
        if (h!=handle) return false;
        if (!selected) {
            selected=true;
            fault::remaining=failAt;
            fault::observe=true;
        }
        return captureEnabled;
    }
    void addXRecord(const DRW_XRecord& x) override {
        if (x.handle==handle+1u && selected) {
            following=true;
            fault::remaining=-1;
            fault::observe=false;
        }
        dx_iface::addXRecord(x);
    }
};
class AdapterFaultSink final : public dx_iface {
public:
    long snapshotFail=-1;
    std::size_t snapshotAllocations=0;
    bool ticketReceived=false;
    bool wantsVerifiedXRecord(std::uint32_t h) const noexcept override {return h==handle+1u;}
    void verifiedXRecord(const DwgVerifiedXRecordTicket& ticket) noexcept override {
        ticketReceived=true;
        fault::allocations=0; fault::remaining=snapshotFail; fault::observe=true;
        DxIfaceStandardDwgCaptureTestAccess::deliver(*this,ticket);
        snapshotAllocations=fault::allocations;
        fault::remaining=-1; fault::observe=false;
    }
};
class MultiFaultSink final : public dx_iface {
public:
    enum class Injection {None, Candidate, Ticket};
    Injection injection{Injection::None};
    mutable bool armed=false;
    bool ticketReached=false;
    bool wantsVerifiedXRecord(std::uint32_t h) const noexcept override {
        if (h<handle || h>=handle+10u) return false;
        if (injection==Injection::Candidate && h==handle+1u && !armed) {
            armed=true; fault::remaining=0;
        }
        return true;
    }
    void verifiedXRecord(const DwgVerifiedXRecordTicket& ticket) noexcept override {
        if (injection==Injection::Ticket && ticket.handle()==handle+1u) {
            ticketReached=true; fault::remaining=0;
        }
        DxIfaceStandardDwgCaptureTestAccess::deliver(*this,ticket);
        fault::remaining=-1;
    }
};
AdapterFaultSink* sealReader=nullptr;
bool sealFaultReady() noexcept {
    return sealReader && DxIfaceStandardDwgCaptureTestAccess::sealReady(*sealReader,handle+1u);
}
class FailingCallback final : public dx_iface {
public:
    bool selected=false, failed=false;
    void addXRecord(const DRW_XRecord& value) override {
        if (value.handle==handle) selected=true;
        if (value.handle==handle+1u) {failed=true; throw std::runtime_error("callback failure");}
        dx_iface::addXRecord(value);
    }
};
class Reentry final : public dx_iface {
public:
    dx_iface* nested=nullptr; dx_data* data=nullptr; std::string path;
    bool attempted=false;
    void addXRecord(const DRW_XRecord& record) override {
        dx_iface::addXRecord(record);
        if (!attempted) {attempted=true; (void)nested->fileImport(path,data,false);}
    }
};
bool frameCrcControl(const std::vector<std::uint8_t>& body) {
    dwgBufferW out; out.putModularShort(static_cast<std::int32_t>(body.size()));
    out.putBytes(body.data(),body.size()); out.putRawShort16(out.crc16(0xC0C1u,0u,out.size()));
    auto bytes=out.data(); dwgBuffer input(bytes.data(),bytes.size(),nullptr); DwgObjectFrame frame;
    if (!frame.readAt(input,DRW::AC1018,0u) || frame.body()!=body) return false;
    bytes.back()^=1u;
    if (frame.readAt(input,DRW::AC1018,0u) || !frame.body().empty()) return false;
    bytes.back()^=1u;
    return frame.readAt(input,DRW::AC1018,0u) && frame.body()==body;
}
bool physicalCrcImportControl(const std::filesystem::path& path) {
    if (std::filesystem::exists(path)) return false;
    struct Cleanup { std::filesystem::path path; ~Cleanup() {std::error_code e; std::filesystem::remove(path,e);} } cleanup{path};
    dwgRW out(path.string().c_str()); Writer writer(out);
    if (!out.write(&writer,DRW::AC1015,true) || !writer.written) return false;
    dx_data clean; dx_iface reader; reader.enableVerifiedXRecordSource(handle);
    if (!reader.fileImport(path.string(),&clean,false)
        || !dx_iface::hasUnchangedVerifiedXRecordSource(clean,handle)) return false;
    const auto& pub=DxIfaceStandardDwgCaptureTestAccess::publication(clean,handle);
    if (pub.m_sourceOffsetSpace!=DRW_DwgFrameOffsetSpace::PhysicalFile) return false;
    std::ifstream in(path,std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)),{});
    if (!in || pub.m_sourceOffset>=bytes.size()) return false;
    dwgBuffer input(bytes.data(),bytes.size(),nullptr); DwgObjectFrame frame;
    if (!frame.readAt(input,DRW::AC1015,pub.m_sourceOffset)
        || frame.body()!=DxIfaceStandardDwgCaptureTestAccess::raw(clean,handle).m_rawBytes
        || input.getPosition()<2u || input.getPosition()>bytes.size()) return false;
    const auto crcOffset=static_cast<std::size_t>(input.getPosition()-2u);
    bytes[crcOffset]^=1u;
    {std::ofstream corrupt(path,std::ios::binary|std::ios::trunc); corrupt.write(reinterpret_cast<const char*>(bytes.data()),bytes.size()); if(!corrupt)return false;}
    dx_data bad; (void)reader.fileImport(path.string(),&bad,false);
    if (dx_iface::hasUnchangedVerifiedXRecordSource(bad,handle)) return false;
    bytes[crcOffset]^=1u;
    {std::ofstream restored(path,std::ios::binary|std::ios::trunc); restored.write(reinterpret_cast<const char*>(bytes.data()),bytes.size()); if(!restored)return false;}
    dx_data retry;
    return reader.fileImport(path.string(),&retry,false)
        && dx_iface::hasUnchangedVerifiedXRecordSource(retry,handle);
}
bool sameImportedXRecord(const dx_data& a,const dx_data& b,std::uint32_t selected=handle) {
    const auto find=[selected](const dx_data& d) -> const DRW_XRecord* {
        const DRW_XRecord* result=nullptr;
        for (const auto& x:d.xRecords) if (x.handle==selected) {
            if (result) return nullptr;
            result=&x;
        }
        return result;
    };
    const auto* x=find(a); const auto* y=find(b);
    if (!x || !y || x->parentHandle!=y->parentHandle || x->m_cloning!=y->m_cloning
        || x->m_values.size()!=y->m_values.size()
        || x->m_dataEntries.size()!=y->m_dataEntries.size()
        || x->m_handleValues!=y->m_handleValues) return false;
    const auto same=[](const auto& left,const auto& right) {
        for (std::size_t i=0;i<left.size();++i) {
            if (left[i].code()!=right[i].code() || left[i].type()!=right[i].type()) return false;
            if (left[i].type()==DRW_Variant::BINARY && *left[i].binary()!=*right[i].binary()) return false;
            if (left[i].type()==DRW_Variant::STRING && *left[i].content.s!=*right[i].content.s) return false;
        }
        return true;
    };
    return same(x->m_values,y->m_values) && same(x->m_dataEntries,y->m_dataEntries);
}
bool candidateAllocationControl(const std::filesystem::path& path) {
    dx_iface baselineReader; dx_data baseline;
    if (!baselineReader.fileImport(path.string(),&baseline,false)) return false;
    const auto calibrate=[&](bool enabled,std::size_t& count) {
        FaultSink reader; reader.captureEnabled=enabled; reader.enableVerifiedXRecordSource(handle);
        dx_data imported; fault::allocations=0; fault::remaining=-1; fault::observe=false;
        const bool ok=reader.fileImport(path.string(),&imported,false);
        fault::remaining=-1; fault::observe=false;
        count=fault::allocations;
        return ok && reader.selected && reader.following && sameImportedXRecord(baseline,imported)
            && dx_iface::hasUnchangedVerifiedXRecordSource(imported,handle)==enabled;
    };
    std::size_t on=0,off=0;
    if (!calibrate(true,on) || !calibrate(false,off) || on<=off || on-off>64u) return false;
    const std::size_t expected=on-off;
    std::size_t covered=0;
    for (long offset=0; offset<static_cast<long>(expected); ++offset) {
        FaultSink reader; reader.enableVerifiedXRecordSource(handle); reader.failAt=offset;
        dx_data failed; fault::fired=false; fault::largest=0;
        bool imported=false;
        try {imported=reader.fileImport(path.string(),&failed,false);} catch (...) {
            fault::remaining=-1; fault::observe=false; return false;
        }
        fault::remaining=-1; fault::observe=false;
        if (!reader.selected) return false;
        const bool hit=fault::fired;
        const bool sealed=dx_iface::hasUnchangedVerifiedXRecordSource(failed,handle);
        if (!hit || sealed || !imported) return false;
        if (failed.dwgReadOutcome.observations.objectParseFailures!=0u
            || failed.dwgReadOutcome.observations.entityParseFailures!=0u
            || !sameImportedXRecord(baseline,failed)) return false;
        ++covered;
        dx_data retry; reader.failAt=-1; reader.selected=false; reader.following=false;
        if (!reader.fileImport(path.string(),&retry,false)
            || !dx_iface::hasUnchangedVerifiedXRecordSource(retry,handle)) return false;
        fault::remaining=-1; fault::observe=false;
    }
    std::cout<<"optional candidate allocation faults covered="<<covered<<'\n';
    return covered==expected;
}
bool adapterAllocationControl(const std::filesystem::path& path) {
    constexpr std::uint32_t target=handle+1u;
    dx_iface plainReader; dx_data plain;
    if (!plainReader.fileImport(path.string(),&plain,false)) return false;
    AdapterFaultSink reader; reader.enableVerifiedXRecordSource(target);
    dx_data calibration;
    if (!reader.fileImport(path.string(),&calibration,false) || !reader.ticketReceived
        || !dx_iface::hasUnchangedVerifiedXRecordSource(calibration,target)
        || !sameImportedXRecord(plain,calibration,target)) return false;
    const auto count=reader.snapshotAllocations;
    if (count==0u || count>64u) return false;
    for (std::size_t i=0;i<count;++i) {
        reader.snapshotFail=static_cast<long>(i); reader.ticketReceived=false;
        fault::fired=false; dx_data failed;
        const bool ok=reader.fileImport(path.string(),&failed,false);
        fault::remaining=-1; fault::observe=false;
        if (!ok || !reader.ticketReceived || !fault::fired
            || failed.dwgReadOutcome.observations.objectParseFailures!=0u
            || failed.dwgReadOutcome.observations.entityParseFailures!=0u
            || !sameImportedXRecord(plain,failed,target)
            || dx_iface::hasUnchangedVerifiedXRecordSource(failed,target)) return false;
        reader.snapshotFail=-1; reader.ticketReceived=false;
        dx_data retry;
        if (!reader.fileImport(path.string(),&retry,false) || !reader.ticketReceived
            || !dx_iface::hasUnchangedVerifiedXRecordSource(retry,target)) return false;
    }
    // The selector below becomes true only once the final selected frame has
    // its typed baseline and the real reader-delivered ticket is pending.
    // The next allocation is the capability's final shared-owner creation.
    reader.snapshotFail=-1; reader.ticketReceived=false;
    fault::fired=false; fault::predicateSize=0;
    sealReader=&reader; fault::predicate=sealFaultReady;
    dx_data failedSeal; const bool ok=reader.fileImport(path.string(),&failedSeal,false);
    fault::predicate=nullptr; sealReader=nullptr;
    if (!ok || !reader.ticketReceived || !fault::fired || fault::predicateSize==0u
        || failedSeal.dwgReadOutcome.observations.objectParseFailures!=0u
        || failedSeal.dwgReadOutcome.observations.entityParseFailures!=0u
        || !sameImportedXRecord(plain,failedSeal,target)
        || dx_iface::hasUnchangedVerifiedXRecordSource(failedSeal,target)) return false;
    reader.ticketReceived=false; dx_data retry;
    if (!reader.fileImport(path.string(),&retry,false) || !reader.ticketReceived
        || !dx_iface::hasUnchangedVerifiedXRecordSource(retry,target)) return false;
    std::cout<<"adapter snapshot allocation faults covered="<<count
             <<" final owner allocation bytes="<<fault::predicateSize<<'\n';
    return true;
}
bool multipleSourceControl(const std::filesystem::path& path) {
    if (std::filesystem::exists(path)) return false;
    struct Cleanup {std::filesystem::path path; ~Cleanup(){std::error_code e; std::filesystem::remove(path,e);}} cleanup{path};
    dwgRW out(path.string().c_str()); Writer writer(out,0u,10u);
    if (!out.write(&writer,DRW::AC1015,true) || !writer.written) return false;
    std::vector<std::uint32_t> wanted;
    for (unsigned i=0;i<10u;++i) wanted.push_back(handle+9u-i);
    dx_iface reader;
    if (!reader.enableVerifiedXRecordSources(wanted)) return false;
    auto invalid=wanted; invalid.push_back(handle);
    if (reader.enableVerifiedXRecordSources(invalid)) return false;
    invalid.clear(); for (unsigned i=0;i<65u;++i) invalid.push_back(handle+i);
    if (reader.enableVerifiedXRecordSources(invalid)) return false;
    invalid={0u}; if (reader.enableVerifiedXRecordSources(invalid)) return false;
    invalid={DRW::NoHandle}; if (reader.enableVerifiedXRecordSources(invalid)) return false;
    dx_data data;
    if (!reader.fileImport(path.string(),&data,false)
        || !dx_iface::hasUnchangedVerifiedXRecordSources(data,wanted)) return false;
    for (const auto h:wanted)
        if (!dx_iface::hasUnchangedVerifiedXRecordSource(data,h)) return false;
    invalid=wanted; invalid[0]=invalid[1];
    if (dx_iface::hasUnchangedVerifiedXRecordSources(data,invalid)) return false;
    invalid=wanted; invalid[0]=handle+20u;
    if (dx_iface::hasUnchangedVerifiedXRecordSources(data,invalid)) return false;
    auto& raw=DxIfaceStandardDwgCaptureTestAccess::raw(data,handle+5u);
    raw.m_rawBytes[0]^=1u;
    if (dx_iface::hasUnchangedVerifiedXRecordSources(data,wanted)
        || dx_iface::hasUnchangedVerifiedXRecordSource(data,handle)) return false;
    raw.m_rawBytes[0]^=1u;
    auto selected=std::find_if(data.xRecords.begin(),data.xRecords.end(),[](const DRW_XRecord& x){return x.handle==handle+5u;});
    if (selected==data.xRecords.end()) return false;
    selected->m_cloning^=1;
    if (dx_iface::hasUnchangedVerifiedXRecordSources(data,wanted)
        || dx_iface::hasUnchangedVerifiedXRecordSource(data,handle)) return false;
    selected->m_cloning^=1;
    if (!dx_iface::hasUnchangedVerifiedXRecordSources(data,wanted)) return false;
    invalid=wanted; invalid.push_back(handle+20u);
    if (!reader.enableVerifiedXRecordSources(invalid)) return false;
    dx_data missing;
    if (!reader.fileImport(path.string(),&missing,false)
        || dx_iface::hasUnchangedVerifiedXRecordSource(missing,handle)) return false;
    if (!reader.enableVerifiedXRecordSources(wanted)) return false;
    dx_data retried;
    if (!reader.fileImport(path.string(),&retried,false)
        || !dx_iface::hasUnchangedVerifiedXRecordSources(retried,wanted)) return false;

    const auto& pub=DxIfaceStandardDwgCaptureTestAccess::publication(retried,handle+5u);
    if (pub.m_sourceOffsetSpace!=DRW_DwgFrameOffsetSpace::PhysicalFile) return false;
    std::ifstream in(path,std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)),{});
    dwgBuffer input(bytes.data(),bytes.size(),nullptr); DwgObjectFrame frame;
    if (!in || !frame.readAt(input,DRW::AC1015,pub.m_sourceOffset)
        || frame.body()!=DxIfaceStandardDwgCaptureTestAccess::raw(retried,handle+5u).m_rawBytes
        || input.getPosition()<2u || input.getPosition()>bytes.size()) return false;
    const auto crcOffset=static_cast<std::size_t>(input.getPosition()-2u);
    bytes[crcOffset]^=1u;
    {std::ofstream f(path,std::ios::binary|std::ios::trunc); f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size()); if(!f)return false;}
    dx_data corrupt; (void)reader.fileImport(path.string(),&corrupt,false);
    if (dx_iface::hasUnchangedVerifiedXRecordSource(corrupt,handle)) return false;
    bytes[crcOffset]^=1u;
    {std::ofstream f(path,std::ios::binary|std::ios::trunc); f.write(reinterpret_cast<const char*>(bytes.data()),bytes.size()); if(!f)return false;}
    dx_data restored;
    if (!reader.fileImport(path.string(),&restored,false)
        || !dx_iface::hasUnchangedVerifiedXRecordSources(restored,wanted)) return false;
    dx_data nestedData; Reentry nested; nested.data=&nestedData; nested.path=path.string(); nested.nested=&nested;
    if (!nested.enableVerifiedXRecordSources(wanted)) return false;
    (void)nested.fileImport(path.string(),&nestedData,false);
    if (!nested.attempted || dx_iface::hasUnchangedVerifiedXRecordSource(nestedData,handle)) return false;
    dx_data retryNested;
    nested.attempted=true;
    if (!nested.fileImport(path.string(),&retryNested,false)
        || !dx_iface::hasUnchangedVerifiedXRecordSources(retryNested,wanted)) return false;
    for (const auto injection:{MultiFaultSink::Injection::Candidate,MultiFaultSink::Injection::Ticket}) {
        MultiFaultSink tested; if (!tested.enableVerifiedXRecordSources(wanted)) return false;
        tested.injection=injection; fault::fired=false; dx_data failed;
        const bool ok=tested.fileImport(path.string(),&failed,false);
        fault::remaining=-1;
        if (!ok || !fault::fired || dx_iface::hasUnchangedVerifiedXRecordSource(failed,handle)
            || (injection==MultiFaultSink::Injection::Ticket && !tested.ticketReached)) return false;
        tested.injection=MultiFaultSink::Injection::None;
        dx_data retry;
        if (!tested.fileImport(path.string(),&retry,false)
            || !dx_iface::hasUnchangedVerifiedXRecordSources(retry,wanted)) return false;
    }
    std::cout<<"ten-source atomic capture and refusal controls passed\n";
    return true;
}
bool aggregateLimitControl(const std::filesystem::path& path) {
    if (std::filesystem::exists(path)) return false;
    struct Cleanup {std::filesystem::path path; ~Cleanup(){std::error_code e; std::filesystem::remove(path,e);}} cleanup{path};
    dwgRW out(path.string().c_str()); Writer writer(out,800u*1024u,3u,true);
    if (!out.write(&writer,DRW::AC1015,true) || !writer.written) return false;
    dx_iface reader; std::vector<std::uint32_t> selected{handle,handle+1u,handle+2u};
    if (!reader.enableVerifiedXRecordSources(selected)) return false;
    dx_data refused;
    if (!reader.fileImport(path.string(),&refused,false)
        || refused.dwgReadOutcome.observations.objectParseFailures!=0u
        || dx_iface::hasUnchangedVerifiedXRecordSource(refused,handle)) return false;
    selected.pop_back(); if (!reader.enableVerifiedXRecordSources(selected)) return false;
    dx_data retry;
    if (!reader.fileImport(path.string(),&retry,false)
        || !dx_iface::hasUnchangedVerifiedXRecordSources(retry,selected)) return false;
    std::cout<<"aggregate source quota refusal/retry passed\n";
    return true;
}
bool oversizedCandidateControl(const std::filesystem::path& path) {
    if (std::filesystem::exists(path)) return false;
    struct Cleanup {std::filesystem::path path; ~Cleanup(){std::error_code e; std::filesystem::remove(path,e);}} cleanup{path};
    constexpr std::size_t payload=1024u*1024u;
    dwgRW out(path.string().c_str()); Writer writer(out,payload);
    if (!out.write(&writer,DRW::AC1015,true) || !writer.written) return false;
    dx_data plain; dx_iface plainReader;
    if (!plainReader.fileImport(path.string(),&plain,false)) return false;
    const auto& raw=DxIfaceStandardDwgCaptureTestAccess::raw(plain,handle);
    if (raw.m_rawBytes.size()<=payload) return false;
    FaultSink reader; reader.enableVerifiedXRecordSource(handle);
    dx_data selected; fault::largest=0; fault::fired=false;
    const bool imported=reader.fileImport(path.string(),&selected,false);
    fault::remaining=-1; fault::observe=false;
    if (!imported || !reader.selected || !reader.following
        || fault::largest>=raw.m_rawBytes.size()
        || selected.xRecords.size()!=plain.xRecords.size() || !sameImportedXRecord(plain,selected)
        || DxIfaceStandardDwgCaptureTestAccess::raw(selected,handle).m_rawBytes!=raw.m_rawBytes
        || plain.dwgReadOutcome.observations.objectParseFailures!=0u
        || plain.dwgReadOutcome.observations.entityParseFailures!=0u
        || selected.dwgReadOutcome.observations.objectParseFailures!=0u
        || selected.dwgReadOutcome.observations.entityParseFailures!=0u
        || dx_iface::hasUnchangedVerifiedXRecordSource(selected,handle)) return false;
    std::cout<<"oversized candidate bytes="<<raw.m_rawBytes.size()
             <<" maximum allocation after selection="<<fault::largest<<'\n';
    return true;
}
bool run(const std::filesystem::path& path) {
    const auto fail=[](const char* s) {std::cerr<<s<<'\n'; return false;};
    if (!styleXRecordSelectorControl()) return fail("style XRECORD selection transaction failed");
    dwgRW out(path.string().c_str()); Writer writer(out);
    if (!out.write(&writer,DRW::AC1018,true) || !writer.written) return fail("generated DWG write failed");
    dx_data a,b; dx_iface first,second; first.enableVerifiedXRecordSource(handle); second.enableVerifiedXRecordSource(handle);
    if (!first.fileImport(path.string(),&a,false) || !dx_iface::hasUnchangedVerifiedXRecordSource(a,handle)) return fail("first selected source not sealed");
    if (!second.fileImport(path.string(),&b,false) || !dx_iface::hasUnchangedVerifiedXRecordSource(b,handle)) return fail("second selected source not sealed");
    if (!frameCrcControl(DxIfaceStandardDwgCaptureTestAccess::raw(a,handle).m_rawBytes)) return fail("CRC corruption or retry failed");
    DxIfaceStandardDwgCaptureTestAccess::transplant(b,a);
    if (dx_iface::hasUnchangedVerifiedXRecordSource(b,handle)) return fail("cross-import capability accepted");
    auto& raw=DxIfaceStandardDwgCaptureTestAccess::raw(a,handle); raw.m_rawBytes[0]^=1u;
    if (dx_iface::hasUnchangedVerifiedXRecordSource(a,handle)) return fail("changed body accepted");
    raw.m_rawBytes[0]^=1u;
    auto& pub=DxIfaceStandardDwgCaptureTestAccess::publication(a,handle); ++pub.m_sourceMapOrdinal;
    if (dx_iface::hasUnchangedVerifiedXRecordSource(a,handle)) return fail("changed ordinal accepted");
    --pub.m_sourceMapOrdinal;
    auto selected=std::find_if(a.xRecords.begin(),a.xRecords.end(),[](const DRW_XRecord& x){return x.handle==handle;});
    if (selected==a.xRecords.end()) return fail("missing generated XRECORD");
    auto saved=*selected; selected->m_cloning^=1;
    if (dx_iface::hasUnchangedVerifiedXRecordSource(a,handle)) return fail("changed model accepted");
    *selected=saved; selected->m_values.emplace_back(1,UTF8STRING("mutation"));
    if (dx_iface::hasUnchangedVerifiedXRecordSource(a,handle)) return fail("changed legacy values accepted");
    *selected=saved; selected->m_dataEntries.emplace_back(1,UTF8STRING("mutation"));
    if (dx_iface::hasUnchangedVerifiedXRecordSource(a,handle)) return fail("changed ordered values accepted");
    *selected=saved; selected->m_handleValues.emplace_back(360,0x123u);
    if (dx_iface::hasUnchangedVerifiedXRecordSource(a,handle)) return fail("changed handle values accepted");
    *selected=saved; a.xRecords.push_back(saved);
    if (dx_iface::hasUnchangedVerifiedXRecordSource(a,handle)) return fail("duplicate model accepted");
    a.xRecords.pop_back();
    if (!dx_iface::hasUnchangedVerifiedXRecordSource(a,handle)) return fail("corrected model retry failed");
    if (first.fileImport(path.string()+".missing",&a,false) || dx_iface::hasUnchangedVerifiedXRecordSource(a,handle)) return fail("failed reimport retained evidence");
    dx_data retry;
    if (!first.fileImport(path.string(),&retry,false) || !dx_iface::hasUnchangedVerifiedXRecordSource(retry,handle)) return fail("failed-import retry failed");
    dx_data plain; dx_iface plainReader;
    if (!plainReader.fileImport(path.string(),&plain,false) || plain.xRecords.size()!=retry.xRecords.size()
        || plain.dwgReadOutcome.observations.objectParseFailures!=retry.dwgReadOutcome.observations.objectParseFailures
        || dx_iface::hasUnchangedVerifiedXRecordSource(plain,handle)) return fail("opt-out import changed");
    dx_data wrong; dx_iface wrongReader; wrongReader.enableVerifiedXRecordSource(0xBEEFu);
    if (!wrongReader.fileImport(path.string(),&wrong,false) || dx_iface::hasUnchangedVerifiedXRecordSource(wrong,handle)) return fail("wrong selected handle accepted");
    dx_data failed; FailingCallback bad; bad.enableVerifiedXRecordSource(handle);
    (void)bad.fileImport(path.string(),&failed,false);
    if (!bad.selected || !bad.failed || failed.dwgReadOutcome.observations.objectParseFailures==0u
        || dx_iface::hasUnchangedVerifiedXRecordSource(failed,handle)) return fail("later callback failure sealed evidence");
    dx_data nestedData; Reentry reentry; reentry.data=&nestedData; reentry.path=path.string(); reentry.nested=&reentry; reentry.enableVerifiedXRecordSource(handle);
    (void)reentry.fileImport(path.string(),&nestedData,false);
    if (!reentry.attempted || dx_iface::hasUnchangedVerifiedXRecordSource(nestedData,handle)) return fail("same-iface reentry sealed evidence");
    dx_data crossData; Reentry outer; dx_iface inner; outer.data=&crossData; outer.path=path.string(); outer.nested=&inner;
    outer.enableVerifiedXRecordSource(handle); inner.enableVerifiedXRecordSource(handle);
    (void)outer.fileImport(path.string(),&crossData,false);
    if (!outer.attempted || dx_iface::hasUnchangedVerifiedXRecordSource(crossData,handle)) return fail("cross-iface reentry sealed evidence");
    if (!physicalCrcImportControl(path.string()+".ac1015.dwg")) return fail("AC1015 physical CRC refusal/retry failed");
    if (!candidateAllocationControl(path)) return fail("optional candidate allocation fault/retry failed");
    if (!adapterAllocationControl(path)) return fail("adapter source allocation fault/retry failed");
    if (!multipleSourceControl(path.string()+".multiple.dwg")) return fail("multiple source capture failed");
    if (!aggregateLimitControl(path.string()+".aggregate.dwg")) return fail("aggregate source quota failed");
    if (!oversizedCandidateControl(path.string()+".oversized.dwg")) return fail("oversized candidate preflight failed");
    return true;
}
}
int main(int argc,char**argv) {
    if(argc!=2)return 2;
    const std::filesystem::path path(argv[1]);
    if (std::filesystem::exists(path)) {std::cerr<<"scratch path already exists\n";return 1;}
    const bool ok=run(path); std::error_code e; std::filesystem::remove(path,e);
    if(ok) std::cout<<"verified XRECORD source controls passed\n";
    return ok?0:1;
}
