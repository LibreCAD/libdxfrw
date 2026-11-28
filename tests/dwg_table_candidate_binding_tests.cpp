#include "dx_iface.h"
#include "intern/dwgutil.h"
#include "table_child_candidate_fixture.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>

namespace fault {long remaining=-1;bool fired=false;bool (*predicate)() noexcept=nullptr;std::size_t count=0;bool counting=false;}
void* operator new(std::size_t n) {
    if(fault::counting)++fault::count;
    if(fault::predicate && fault::predicate()) {
        fault::predicate=nullptr;fault::fired=true;throw std::bad_alloc();
    }
    if (fault::remaining==0) {fault::remaining=-1;fault::fired=true;throw std::bad_alloc();}
    if (fault::remaining>0)--fault::remaining;
    if (void* p=std::malloc(n?n:1u))return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}

class DxIfaceStandardDwgCaptureTestAccess {
public:
    static DRW_UnsupportedObject& raw(dx_data& d,std::uint32_t h) {
        return d.m_opaqueDwgSourceFrames.at({d.importGeneration,h});
    }
    static const DRW_UnsupportedObject& standardRaw(const dx_data& d,std::uint32_t h) {
        return d.m_standardDwgObjectFrames.at({d.importGeneration,h}).raw;
    }
    static DRW_DwgFramePublication& publication(dx_data& d,std::uint32_t h) {
        return d.m_standardDwgObjectFrames.at({d.importGeneration,h}).publication;
    }
    static void deliver(dx_iface& i,const DwgVerifiedTableCandidateTicket& ticket) noexcept {
        i.dx_iface::verifiedTableCandidate(ticket);
    }
    static void transplant(dx_data& dst,const dx_data& src) {
        dst.m_verifiedTableCandidateSource=src.m_verifiedTableCandidateSource;
    }
    static bool writerObservationCheck(const dx_iface& writer,
            const dx_data& data) noexcept {
        return writer.hasUnchangedVerifiedSourceObservations(data);
    }
    static bool sealStage(dx_iface& a,int stage) noexcept {
        const auto* d=a.m_verifiedXRecordReadData;
        if(!d || !d->dwgReadOutcome.operationSucceeded || d->m_verifiedXRecordSource
            || d->m_verifiedTableCandidateSource)return false;
        if(stage==1 && (a.m_verifiedXRecordPending.size()!=a.m_verifiedXRecordSelected.size()
                        || a.m_verifiedTableCandidatePending.size()!=(a.m_verifiedTableDictionarySelected?4u:a.m_verifiedTableGeometrySelected?3u:2u)))return false;
        if(stage==2 && (!a.m_verifiedXRecordPending.empty() || a.m_verifiedTableCandidatePending.size()!=(a.m_verifiedTableDictionarySelected?4u:a.m_verifiedTableGeometrySelected?3u:2u)))return false;
        if(stage==3 && (!a.m_verifiedXRecordPending.empty() || !a.m_verifiedTableCandidatePending.empty()))return false;
        for(const auto& row:d->m_standardDwgObjectFrames)
            if(row.first.first==d->importGeneration && !row.second.payloadMatched)return false;
        return true;
    }
};
namespace {
constexpr std::uint32_t contentHandle=0xb9u,rootHandle=0xbcu,dataHandle=0xbdu;
const std::vector<std::uint32_t> xrecords{0xbcu,0xbeu,0xbfu,0xc0u};
const std::vector<std::uint32_t> fullXrecords{0xbcu,0xbeu,0xbfu,0xc0u,0xc1u,0xc2u,0xc3u,0xc4u,0xc5u,0xc6u};
using Content=DwgTableContentAc1018Candidate::Result;
using Data=DwgDataTableAc1018Candidate::Result;
using Geometry=DwgTableGeometryAc1018Candidate::Result;
constexpr std::uint32_t geometryHandle=0xbau;

DRW_UnsupportedObject raw(const char* path,bool content) {
    std::ifstream f(path,std::ios::binary);
    DRW_UnsupportedObject r;r.m_rawBytes.assign(std::istreambuf_iterator<char>(f),{});
    r.m_version=DRW::AC1018;r.m_isCustomClass=r.m_hasClassDefinition=true;
    r.m_objectType=content?514:517;r.m_handle=content?contentHandle:dataHandle;
    r.m_recordName=content?"TABLECONTENT":"DATATABLE";
    r.m_className=content?"AcDbTableContent":"AcDbDataTable";
    r.m_classAppName="LOCALLY_AUTHORED";r.m_classProxyFlag=0x401u;r.m_classEntityFlagRaw=0x1f3u;
    r.m_objectSize=static_cast<std::uint32_t>(r.m_rawBytes.size());
    return r;
}
class Writer final : public dx_iface {
public:
    Writer(dwgRW& output,DRW_UnsupportedObject a,DRW_UnsupportedObject b,std::size_t big=0u,DRW_UnsupportedObject g={},bool rootProfile=false,bool childProfile=false)
        :w(output),content(std::move(a)),table(std::move(b)),geometry(std::move(g)),bigSize(big),profile(rootProfile),childrenProfile(childProfile){}
    void writeHeader(DRW_Header& h) override {h.vars.clear();}
    void writeDwgClasses() override {registered=w.registerRawDwgObjectClass(&content)&&w.registerRawDwgObjectClass(&table)
        && (!geometry.m_handle || w.registerRawDwgObjectClass(&geometry));}
    void collectDwgAppIds() override {}
    void writeBlocks() override {} void writeBlockRecords() override {} void writeEntities() override {}
    void writeLTypes() override {} void writeLayers() override {} void writeTextstyles() override {}
    void writeVports() override {} void writeViews() override {} void writeUCSs() override {}
    void writeDimstyles() override {} void writeAppId() override {}
    void writeObjects() override {
        wrote=registered;
        const auto& handles=childrenProfile?fullXrecords:xrecords;
        for (std::size_t i=0;i<handles.size();++i) {
            const auto h=handles[i];
            DRW_XRecord x;x.handle=h;x.parentHandle=h==rootHandle?0xbbu:dataHandle;
            if(profile && h==rootHandle) {
                x.reactorHandles={0xbbu};x.setDwgCommonObjectState(1,1u,false);x.m_cloning=1;auto& e=x.m_dataEntries;
                e.emplace_back(102,UTF8STRING("ACAD_ROUNDTRIP_2008_TABLE_ENTITY"));
                e.emplace_back(360,std::uint64_t{0xb9u});e.emplace_back(70,std::int32_t{2});e.emplace_back(90,std::int32_t{1});
                e.emplace_back(10,DRW_Coord(0,0,0));e.emplace_back(90,std::int32_t{0});e.emplace_back(90,std::int32_t{2});
                e.emplace_back(361,std::uint64_t{0xbau});e.emplace_back(102,UTF8STRING("ACAD_ROUNDTRIP_PRE2007_TABLE"));
                e.emplace_back(90,std::int32_t{3});e.emplace_back(91,std::int32_t{3});
                e.emplace_back(102,UTF8STRING("ACAD_ROUNDTRIP_PRE2007_TABLECELL"));e.emplace_back(360,std::uint64_t{0xbdu});
            } else if(childrenProfile && h!=rootHandle) {
                ChildFixture::Options o;o.handle=h;o.empty=(h==0xbfu || h==0xc0u);
                if(h>=0xc1u)o.value=std::to_string(h-0xc0u);
                x=ChildFixture::build(o).typed;
            } else if(bigSize && i<2u)x.m_values.emplace_back(310,std::vector<std::uint8_t>(bigSize,0xa5u));
            else x.m_values.emplace_back(1,UTF8STRING("LOCALLY_AUTHORED"));
            wrote=w.writeXRecord(&x)&&wrote;
        }
        if(childrenProfile){DRW_Dictionary d;d.handle=0xbbu;d.parentHandle=0x9cu;d.hardOwner=1;d.cloning=1;d.setDwgCommonObjectState(0,1u,false);
            d.m_entries.push_back({"ACAD_XREC_ROUNDTRI",rootHandle});wrote=w.writeDictionary(&d)&&wrote;}
        wrote=w.writeRawDwgObject(&content)&&w.writeRawDwgObject(&table)&&wrote;
        if(geometry.m_handle)wrote=w.writeRawDwgObject(&geometry)&&wrote;
    }
    bool finalizeDwgWrite() override{return wrote;}
    bool finalizeDwgWriteStructure() override{return true;}
    dwgRW& w;DRW_UnsupportedObject content,table,geometry;std::size_t bigSize;bool profile=false,childrenProfile=false,registered=false,wrote=false;
};
class DxfStageProbe final : public dx_iface {
public:
    dxfRW* activeWriter{nullptr};
    bool blockRecordWriterBound{false};
    bool lineTypesCalled{false};
    bool observationFreshAtLineTypes{false};
    void writeBlockRecords() override {
        blockRecordWriterBound = dxfW != nullptr;
        if (dxfW != nullptr)
            dx_iface::writeBlockRecords();
    }
    void writeHeader(DRW_Header& header) override {
        dxfW = activeWriter;
        dx_iface::writeHeader(header);
    }
    void writeLTypes() override {
        lineTypesCalled = true;
        observationFreshAtLineTypes = cData != nullptr
            && DxIfaceStandardDwgCaptureTestAccess::writerObservationCheck(
                *this, *cData);
        dx_iface::writeLTypes();
    }
    bool finalizeDxfWrite() override { return true; }
};
bool write(const std::filesystem::path& path,DRW_UnsupportedObject a,DRW_UnsupportedObject b,std::size_t big=0u,const DRW_UnsupportedObject* geometry=nullptr,bool profile=false,bool childrenProfile=false) {
    dwgRW output(path.string().c_str());Writer writer(output,std::move(a),std::move(b),big,geometry?*geometry:DRW_UnsupportedObject{},profile,childrenProfile);
    return output.write(&writer,DRW::AC1018,true)&&writer.wrote;
}
bool selected(dx_iface& a){return a.enableVerifiedTableCandidates(xrecords,contentHandle,dataHandle);}
bool live(const dx_data& d) {
    Content c;Data t;
    return dx_iface::readVerifiedTableCandidates(d,xrecords,contentHandle,dataHandle,c,t)
        && c.handle==contentHandle && t.handle==dataHandle && c.parentHandle==rootHandle
        && t.parentHandle==rootHandle && t.ownedChildren==std::vector<std::uint32_t>({0xbeu,0xbfu,0xc0u});
}
class FaultAdapter final : public dx_iface {
public:
    enum class Injection {None,Candidate,Ticket};
    Injection injection{Injection::None};mutable bool armed=false;bool ticket=false;mutable bool wantedTarget=false;unsigned targetTickets=0;std::uint32_t target=dataHandle;
    bool wantsVerifiedTableCandidate(std::uint32_t h) const noexcept override {
        if(h!=contentHandle && h!=dataHandle && h!=geometryHandle)return false;
        if(h==target)wantedTarget=true;
        if(injection==Injection::Candidate && h==target && !armed){armed=true;fault::remaining=0;}
        return true;
    }
    void verifiedTableCandidate(const DwgVerifiedTableCandidateTicket& value) noexcept override {
        if(value.handle()==target)++targetTickets;
        if(injection==Injection::Ticket && value.handle()==target){ticket=true;fault::remaining=0;}
        DxIfaceStandardDwgCaptureTestAccess::deliver(*this,value);fault::remaining=-1;
    }
};
class Reentry final : public dx_iface {
public:
    dx_data* data=nullptr;dx_iface* nested=nullptr;std::string path;bool attempted=false;
    void addUnsupportedObject(const DRW_UnsupportedObject& value) override {
        dx_iface::addUnsupportedObject(value);
        if(value.m_handle==contentHandle && !attempted){attempted=true;(void)(nested?nested:this)->fileImport(path,data,false);}
    }
};
dx_iface* sealingAdapter=nullptr;int sealingStage=0;
bool sealFaultReady() noexcept {
    return sealingAdapter && DxIfaceStandardDwgCaptureTestAccess::sealStage(*sealingAdapter,sealingStage);
}
class Spoofing final : public dx_iface {
public:
    int mode{0};bool seen=false;std::uint32_t target=contentHandle;
    void addUnsupportedObject(const DRW_UnsupportedObject& value) override {
        if(value.m_handle==target && mode) {
            seen=true;auto altered=value;
            if(mode==1)altered.m_className="Other";
            else if(mode==2)altered.m_recordName="OTHER";
            else ++altered.m_objectType;
            dx_iface::addUnsupportedObject(altered);return;
        }
        dx_iface::addUnsupportedObject(value);
    }
};
class Throwing final : public dx_iface {
public:
    bool fail=true,seen=false;std::uint32_t target=dataHandle;
    void addUnsupportedObject(const DRW_UnsupportedObject& value) override {
        if(value.m_handle==target && fail){seen=true;throw std::runtime_error("callback failure");}
        dx_iface::addUnsupportedObject(value);
    }
};
std::uint32_t get32(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) | static_cast<std::uint32_t>(p[1])<<8u
        | static_cast<std::uint32_t>(p[2])<<16u | static_cast<std::uint32_t>(p[3])<<24u;
}
void put32(std::uint8_t* p,std::uint32_t n) {
    for(unsigned i=0;i<4;++i)p[i]=static_cast<std::uint8_t>(n>>(i*8u));
}
bool corruptSelectedCrc(const std::filesystem::path& path,const DRW_UnsupportedObject& source,
                        std::vector<std::uint8_t>& original) {
    std::ifstream f(path,std::ios::binary);original.assign(std::istreambuf_iterator<char>(f),{});
    if(!f || source.m_rawBytes.empty())return false;
    const auto hit=std::search(original.cbegin(),original.cend(),source.m_rawBytes.cbegin(),source.m_rawBytes.cend());
    if(hit==original.cend() || std::search(std::next(hit),original.cend(),source.m_rawBytes.cbegin(),source.m_rawBytes.cend())!=original.cend())return false;
    const auto at=static_cast<std::size_t>(hit-original.cbegin());
    dwgBuffer in(original.data(),original.size(),nullptr);DwgObjectFrame frame;bool exactFrame=false;std::size_t frameOffset=0u;
    for(std::size_t prefix=2u;prefix<=4u;prefix+=2u) {
        if(at>=prefix && frame.readAt(in,DRW::AC1018,at-prefix) && frame.body()==source.m_rawBytes
            && in.getPosition()==at+source.m_rawBytes.size()+2u) {exactFrame=true;frameOffset=at-prefix;break;}
    }
    if(!exactFrame)return false;
    std::size_t page=0u;std::vector<std::uint8_t> header(32u);
    for(std::size_t p=0x100u;p+32u<=at;p+=0x20u) {
        std::copy_n(original.data()+p,32u,header.data());dwgCompressor::decrypt18Hdr(header.data(),32u,p);
        const auto size=get32(header.data()+8u);
        if(get32(header.data())!=0x4163043bu || size!=get32(header.data()+12u)
            || size>original.size()-(p+32u) || at<p+32u
            || at+source.m_rawBytes.size()+2u>p+32u+size)continue;
        const auto dataCrc=dwgUtil::checksum18(0u,original.data()+p+32u,size);
        const auto expectedHeader=get32(header.data()+24u);put32(header.data()+24u,0u);
        if(dataCrc!=get32(header.data()+28u) || dwgUtil::checksum18(dataCrc,header.data(),32u)!=expectedHeader)continue;
        if(page!=0u)return false;page=p;
    }
    if(page==0u)return false;
    auto bytes=original;
    const auto crcAt=at+source.m_rawBytes.size();bytes[crcAt]^=1u;
    std::copy_n(bytes.data()+page,32u,header.data());dwgCompressor::decrypt18Hdr(header.data(),32u,page);
    const auto size=get32(header.data()+8u);const auto dataCrc=dwgUtil::checksum18(0u,bytes.data()+page+32u,size);
    put32(header.data()+28u,dataCrc);put32(header.data()+24u,0u);
    put32(header.data()+24u,dwgUtil::checksum18(dataCrc,header.data(),32u));
    dwgCompressor::decrypt18Hdr(header.data(),32u,page);
    std::copy(header.begin(),header.end(),bytes.begin()+static_cast<std::ptrdiff_t>(page));
    dwgBuffer damaged(bytes.data(),bytes.size(),nullptr);
    if(frame.readAt(damaged,DRW::AC1018,frameOffset))return false;
    std::ofstream out(path,std::ios::binary|std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());return static_cast<bool>(out);
}
bool replaceSelectedBody(const std::filesystem::path& path,const DRW_UnsupportedObject& source,
                         const std::vector<std::uint8_t>& replacement) {
    if(source.m_rawBytes.size()!=replacement.size() || replacement.empty())return false;
    std::ifstream f(path,std::ios::binary);std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(f)),{});
    if(!f)return false;
    const auto hit=std::search(bytes.cbegin(),bytes.cend(),source.m_rawBytes.cbegin(),source.m_rawBytes.cend());
    if(hit==bytes.cend() || std::search(std::next(hit),bytes.cend(),source.m_rawBytes.cbegin(),source.m_rawBytes.cend())!=bytes.cend())return false;
    const auto at=static_cast<std::size_t>(hit-bytes.cbegin());dwgBuffer in(bytes.data(),bytes.size(),nullptr);
    DwgObjectFrame frame;std::size_t frameOffset=0u;bool matched=false;
    for(std::size_t prefix=2u;prefix<=4u;prefix+=2u) {
        if(at>=prefix && frame.readAt(in,DRW::AC1018,at-prefix) && frame.body()==source.m_rawBytes
            && in.getPosition()==at+source.m_rawBytes.size()+2u){frameOffset=at-prefix;matched=true;break;}
    }
    if(!matched)return false;
    std::size_t page=0u;std::vector<std::uint8_t> header(32u);
    for(std::size_t p=0x100u;p+32u<=at;p+=0x20u) {
        std::copy_n(bytes.data()+p,32u,header.data());dwgCompressor::decrypt18Hdr(header.data(),32u,p);
        const auto size=get32(header.data()+8u);
        if(get32(header.data())!=0x4163043bu || size!=get32(header.data()+12u)
            || size>bytes.size()-(p+32u) || at<p+32u || at+replacement.size()+2u>p+32u+size)continue;
        const auto dataCrc=dwgUtil::checksum18(0u,bytes.data()+p+32u,size);
        const auto expected=get32(header.data()+24u);put32(header.data()+24u,0u);
        if(dataCrc!=get32(header.data()+28u) || dwgUtil::checksum18(dataCrc,header.data(),32u)!=expected)continue;
        if(page!=0u)return false;page=p;
    }
    if(!page)return false;
    std::copy(replacement.begin(),replacement.end(),bytes.begin()+static_cast<std::ptrdiff_t>(at));
    dwgBuffer updated(bytes.data(),bytes.size(),nullptr);
    const auto crc=updated.crc8(0xc0c1,static_cast<std::int32_t>(frameOffset),static_cast<std::int32_t>(at+replacement.size()));
    bytes[at+replacement.size()]=static_cast<std::uint8_t>(crc);
    bytes[at+replacement.size()+1u]=static_cast<std::uint8_t>(crc>>8u);
    std::copy_n(bytes.data()+page,32u,header.data());dwgCompressor::decrypt18Hdr(header.data(),32u,page);
    const auto size=get32(header.data()+8u);const auto dataCrc=dwgUtil::checksum18(0u,bytes.data()+page+32u,size);
    put32(header.data()+28u,dataCrc);put32(header.data()+24u,0u);
    put32(header.data()+24u,dwgUtil::checksum18(dataCrc,header.data(),32u));
    dwgCompressor::decrypt18Hdr(header.data(),32u,page);std::copy(header.begin(),header.end(),bytes.begin()+static_cast<std::ptrdiff_t>(page));
    dwgBuffer verify(bytes.data(),bytes.size(),nullptr);
    if(!frame.readAt(verify,DRW::AC1018,frameOffset) || frame.body()!=replacement)return false;
    std::ofstream out(path,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());return static_cast<bool>(out);
}
bool run(const char* contentPath,const char* dataPath,const char* wrongPath,
         const char* largeContentPath,const std::filesystem::path& path) {
    auto content=raw(contentPath,true),table=raw(dataPath,false);
    if(content.m_rawBytes.empty()||table.m_rawBytes.empty()||!write(path,content,table))return false;
    dx_iface reader;dx_data baseline;if(!selected(reader))return false;
    auto invalid=xrecords;invalid[0]=DRW::NoHandle;
    if(reader.enableVerifiedTableCandidates(invalid,contentHandle,dataHandle)
        || reader.enableVerifiedTableCandidates(xrecords,DRW::NoHandle,dataHandle)
        || reader.enableVerifiedTableCandidates(xrecords,contentHandle,DRW::NoHandle)
        || reader.enableVerifiedTableCandidates(xrecords,contentHandle,contentHandle)
        || reader.enableVerifiedTableCandidates(xrecords,rootHandle,dataHandle))return false;
    invalid=xrecords;invalid.push_back(rootHandle);
    if(reader.enableVerifiedTableCandidates(invalid,contentHandle,dataHandle))return false;
    if(!reader.fileImport(path.string(),&baseline,false)||!live(baseline))return false;
    auto& snapshot=DxIfaceStandardDwgCaptureTestAccess::raw(baseline,dataHandle);
    DRW_DwgClassCoverageEntry* selectedClass=nullptr;
    std::size_t selectedClassMatches=0u;
    for (DRW_DwgClassCoverageEntry& entry : baseline.dwgClassCoverage.m_entries) {
        if (entry.m_classNumber != snapshot.m_objectType)
            continue;
        selectedClass=&entry;
        ++selectedClassMatches;
    }
    if (selectedClassMatches!=1u || selectedClass==nullptr
        || !baseline.hasUnchangedDwgClassEvidence())return false;
    selectedClass->m_proxyFlag^=1;
    if(live(baseline))return false;
    selectedClass->m_proxyFlag^=1;
    if(!live(baseline))return false;
    ++selectedClass->m_instanceCount;
    if(live(baseline))return false;
    --selectedClass->m_instanceCount;
    if(!live(baseline))return false;
    snapshot.m_rawBytes[0]^=1u;
    Content c;Data t;c.handle=1u;t.handle=1u;
    if(dx_iface::hasUnchangedVerifiedXRecordSource(baseline,rootHandle)
        || dx_iface::readVerifiedTableCandidates(baseline,xrecords,contentHandle,dataHandle,c,t)
        || c.handle || t.handle)return false;
    snapshot.m_rawBytes[0]^=1u;if(!live(baseline))return false;
    const auto child=std::find_if(baseline.xRecords.begin(),baseline.xRecords.end(),[](const DRW_XRecord& x){return x.handle==0xbeu;});
    if(child==baseline.xRecords.end())return false;child->m_cloning^=1;
    if(live(baseline)||dx_iface::hasUnchangedVerifiedXRecordSource(baseline,rootHandle))return false;
    child->m_cloning^=1;if(!live(baseline))return false;
    auto wrong=xrecords;wrong[1]=0xc1u;
    if(dx_iface::readVerifiedTableCandidates(baseline,wrong,contentHandle,dataHandle,c,t)
        || dx_iface::readVerifiedTableCandidates(baseline,xrecords,dataHandle,contentHandle,c,t))return false;
    dx_data second;if(!reader.fileImport(path.string(),&second,false)||!live(second))return false;
    DxIfaceStandardDwgCaptureTestAccess::transplant(second,baseline);
    if(live(second)||dx_iface::hasUnchangedVerifiedXRecordSource(second,rootHandle))return false;
    if(!reader.enableVerifiedTableCandidates(xrecords,contentHandle,0xcau))return false;
    dx_data absent;(void)reader.fileImport(path.string(),&absent,false);
    if(live(absent)||dx_iface::hasUnchangedVerifiedXRecordSource(absent,rootHandle))return false;
    if(!selected(reader))return false;dx_data retry;
    if(!reader.fileImport(path.string(),&retry,false)||!live(retry))return false;
    for(int stage=1;stage<=3;++stage) {
        dx_iface a;if(!selected(a))return false;dx_data failed;
        sealingAdapter=&a;sealingStage=stage;fault::predicate=sealFaultReady;fault::fired=false;
        const bool ok=a.fileImport(path.string(),&failed,false);
        fault::predicate=nullptr;sealingAdapter=nullptr;
        if(!ok||!fault::fired||live(failed)||dx_iface::hasUnchangedVerifiedXRecordSource(failed,rootHandle))return false;
        dx_data again;if(!a.fileImport(path.string(),&again,false)||!live(again))return false;
    }
    for(const auto injection:{FaultAdapter::Injection::Candidate,FaultAdapter::Injection::Ticket}) {
        FaultAdapter a;if(!selected(a))return false;a.injection=injection;fault::fired=false;dx_data failed;
        const bool ok=a.fileImport(path.string(),&failed,false);fault::remaining=-1;
        if(!ok||!fault::fired||live(failed)||dx_iface::hasUnchangedVerifiedXRecordSource(failed,rootHandle)
            || (injection==FaultAdapter::Injection::Ticket&&!a.ticket))return false;
        a.injection=FaultAdapter::Injection::None;dx_data again;
        if(!a.fileImport(path.string(),&again,false)||!live(again))return false;
    }
    Reentry nested;dx_data nestedData;nested.data=&nestedData;nested.path=path.string();
    if(!selected(nested))return false;(void)nested.fileImport(path.string(),&nestedData,false);
    if(!nested.attempted||live(nestedData)||dx_iface::hasUnchangedVerifiedXRecordSource(nestedData,rootHandle))return false;
    dx_data nestedRetry;if(!nested.fileImport(path.string(),&nestedRetry,false)||!live(nestedRetry))return false;
    Reentry outer;dx_iface inner;dx_data crossData;outer.data=&crossData;outer.nested=&inner;outer.path=path.string();
    if(!selected(outer)||!selected(inner))return false;(void)outer.fileImport(path.string(),&crossData,false);
    if(!outer.attempted||live(crossData)||dx_iface::hasUnchangedVerifiedXRecordSource(crossData,rootHandle))return false;
    dx_data crossRetry;if(!outer.fileImport(path.string(),&crossRetry,false)||!live(crossRetry))return false;
    Throwing throwing;if(!selected(throwing))return false;dx_data failed;
    (void)throwing.fileImport(path.string(),&failed,false);
    if(!throwing.seen||live(failed)||dx_iface::hasUnchangedVerifiedXRecordSource(failed,rootHandle))return false;
    throwing.fail=false;dx_data callbackRetry;
    if(!throwing.fileImport(path.string(),&callbackRetry,false)||!live(callbackRetry))return false;
    for(int mode=1;mode<=3;++mode) {
        Spoofing spoof;if(!selected(spoof))return false;spoof.mode=mode;dx_data altered;
        (void)spoof.fileImport(path.string(),&altered,false);
        if(!spoof.seen||live(altered)||dx_iface::hasUnchangedVerifiedXRecordSource(altered,rootHandle))return false;
        spoof.mode=0;dx_data fresh;if(!spoof.fileImport(path.string(),&fresh,false)||!live(fresh))return false;
    }
    const auto badRole=raw(wrongPath,false);const auto bad=path.string()+".bad.dwg";
    struct Cleanup {std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove(p,e);}} cleanup{bad};
    if(!write(bad,content,badRole))return false;
    dx_data badData;(void)reader.fileImport(bad,&badData,false);
    if(live(badData)||dx_iface::hasUnchangedVerifiedXRecordSource(badData,rootHandle))return false;
    dx_data finalRetry;if(!reader.fileImport(path.string(),&finalRetry,false)||!live(finalRetry))return false;
    const auto output=path.string()+".dxf";
    Cleanup outputCleanup{output};
    {std::ofstream sentinel(output,std::ios::binary|std::ios::trunc);sentinel<<"SOURCE_SENTINEL";if(!sentinel)return false;}
    if(reader.fileExport(output,DRW::AC1018,false,&finalRetry,false,true,
            dx_dxfSourceTextPolicy::Compatible,DRW::DxfTextWritePolicy::Compatible,
            dx_dxfProxyGraphicsPolicy::Compatible,dx_dwgReadCompletenessPolicy::RequireComplete))return false;
    {std::ifstream in(output,std::ios::binary);const std::string value((std::istreambuf_iterator<char>(in)),{});
     if(value!="SOURCE_SENTINEL")return false;}
    std::vector<std::uint8_t> original;
    if(!corruptSelectedCrc(path,DxIfaceStandardDwgCaptureTestAccess::raw(finalRetry,contentHandle),original))return false;
    dx_data crcBad;(void)reader.fileImport(path.string(),&crcBad,false);
    if(live(crcBad)||dx_iface::hasUnchangedVerifiedXRecordSource(crcBad,rootHandle))return false;
    bool targetFrameFailure=false;
    const auto& integrity=crcBad.dwgReadOutcome.observations.integrity;
    if(integrity.extractionOmitted!=0u || integrity.reportedUpstreamDropped!=0u
        || integrity.aggregateSaturated)return false;
    for(std::size_t i=0;i<integrity.retained;++i) {
        const auto& observation=integrity.rows[i];
        if(observation.kind==DwgIntegrityCheckKind::DataPageChecksum
            || observation.kind==DwgIntegrityCheckKind::DataPageCrc)return false;
        if(observation.kind==DwgIntegrityCheckKind::ObjectFrameBounds
            && observation.hasLogicalHandle && observation.logicalHandle==contentHandle)
            targetFrameFailure=true;
    }
    if(!targetFrameFailure)return false;
    {std::ofstream f(path,std::ios::binary|std::ios::trunc);f.write(reinterpret_cast<const char*>(original.data()),original.size());if(!f)return false;}
    dx_data crcRetry;if(!reader.fileImport(path.string(),&crcRetry,false)||!live(crcRetry))return false;
    auto larger=raw(largeContentPath,true);const auto largePath=path.string()+".large.dwg";Cleanup largeCleanup{largePath};
    Content largeCandidate;if(!DwgTableContentAc1018Candidate::inspect(larger,largeCandidate))return false;
    if(!write(largePath,larger,table,1035000u))return false;
    dx_iface xonly;if(!xonly.enableVerifiedXRecordSources(xrecords))return false;dx_data quotaSource;
    if(!xonly.fileImport(largePath,&quotaSource,false)||!dx_iface::hasUnchangedVerifiedXRecordSources(quotaSource,xrecords))return false;
    std::size_t total=0;for(const auto h:xrecords)total+=DxIfaceStandardDwgCaptureTestAccess::standardRaw(quotaSource,h).m_rawBytes.size();
    if(total>=2u*1024u*1024u || total+larger.m_rawBytes.size()+table.m_rawBytes.size()<=2u*1024u*1024u)return false;
    dx_iface quota;if(!selected(quota))return false;dx_data refusedQuota;
    if(!quota.fileImport(largePath,&refusedQuota,false)||live(refusedQuota)
        || dx_iface::hasUnchangedVerifiedXRecordSource(refusedQuota,rootHandle))return false;
    dx_data quotaRetry;if(!quota.fileImport(path.string(),&quotaRetry,false)||!live(quotaRetry))return false;
    std::cout<<"combined source-binding, mutation, failure and retry controls passed\n";
    return true;
}
DRW_UnsupportedObject rawGeometry(const char* path) {
    auto value=raw(path,true);value.m_objectType=515;value.m_handle=geometryHandle;
    value.m_recordName="TABLEGEOMETRY";value.m_className="AcDbTableGeometry";return value;
}
bool selectedGeometry(dx_iface& a) {
    return a.enableVerifiedTableCandidates(xrecords,contentHandle,dataHandle,geometryHandle);
}
bool liveGeometry(const dx_data& d) {
    Content c;Data t;Geometry g;
    return dx_iface::readVerifiedTableCandidates(d,xrecords,contentHandle,dataHandle,geometryHandle,c,t,g)
        && c.handle==contentHandle && t.handle==dataHandle && g.handle==geometryHandle
        && c.parentHandle==rootHandle && t.parentHandle==rootHandle && g.parentHandle==rootHandle;
}
bool runGeometry(const char* contentPath,const char* dataPath,const char* geometryPath,
                 const char* badGeometryPath,const std::filesystem::path& path) {
#define CHECK_G(expr) do { if(!(expr)){std::cerr<<"geometry binding failed line "<<__LINE__<<'\n';return false;} } while(false)
    auto content=raw(contentPath,true),table=raw(dataPath,false),geometry=rawGeometry(geometryPath);
    CHECK_G(!geometry.m_rawBytes.empty());CHECK_G(write(path,content,table,0,&geometry));
    dx_iface reader;CHECK_G(selectedGeometry(reader));
    CHECK_G(!reader.enableVerifiedTableCandidates(xrecords,contentHandle,dataHandle,DRW::NoHandle));
    CHECK_G(!reader.enableVerifiedTableCandidates(xrecords,contentHandle,dataHandle,contentHandle));
    CHECK_G(!reader.enableVerifiedTableCandidates(xrecords,contentHandle,dataHandle,dataHandle));
    CHECK_G(!reader.enableVerifiedTableCandidates(xrecords,contentHandle,dataHandle,rootHandle));
    dx_data baseline;CHECK_G(reader.fileImport(path.string(),&baseline,false));CHECK_G(liveGeometry(baseline));
    CHECK_G(dx_iface::hasUnchangedVerifiedXRecordSources(baseline,xrecords));
    CHECK_G(DxIfaceStandardDwgCaptureTestAccess::writerObservationCheck(reader,baseline));
    DxfStageProbe stageProbe;CHECK_G(selectedGeometry(stageProbe));
    dx_data stagedSource;CHECK_G(stageProbe.fileImport(path.string(),&stagedSource,false));
    const auto stageOutput=path.string()+".stage.dxf";
    {std::ofstream f(stageOutput,std::ios::binary|std::ios::trunc);f<<"STAGE_SENTINEL";CHECK_G(static_cast<bool>(f));}
    auto& stagedGeometry=DxIfaceStandardDwgCaptureTestAccess::raw(stagedSource,geometryHandle);
    stagedGeometry.m_rawBytes[0]^=1u;
    dxfRW stageWriter(stageOutput.c_str());stageProbe.activeWriter=&stageWriter;stageProbe.dxfW=&stageWriter;
    const bool stagedWrite=stageWriter.write(&stageProbe,DRW::AC1018,false);
    stageProbe.dxfW=nullptr;
    CHECK_G(stageProbe.blockRecordWriterBound && !stagedWrite && stageProbe.lineTypesCalled
        && !stageProbe.observationFreshAtLineTypes);
    {std::ifstream f(stageOutput,std::ios::binary);const std::string value((std::istreambuf_iterator<char>(f)),{});CHECK_G(value=="STAGE_SENTINEL");}
    stagedGeometry.m_rawBytes[0]^=1u;
    stageProbe.lineTypesCalled=false;stageProbe.observationFreshAtLineTypes=false;
    const bool stageRetry=stageWriter.write(&stageProbe,DRW::AC1018,false);
    stageProbe.dxfW=nullptr;
    CHECK_G(stageRetry && stageProbe.lineTypesCalled
        && stageProbe.observationFreshAtLineTypes);
    std::error_code stageError;CHECK_G(std::filesystem::remove(stageOutput,stageError)&&!stageError);
    Content c;Data t;Geometry g;c.handle=t.handle=g.handle=1u;
    CHECK_G(!dx_iface::readVerifiedTableCandidates(baseline,xrecords,contentHandle,dataHandle,c,t));
    CHECK_G(!dx_iface::readVerifiedTableCandidates(baseline,xrecords,contentHandle,dataHandle,0xcau,c,t,g));
    CHECK_G(!c.handle && !t.handle && !g.handle);
    auto& snapshot=DxIfaceStandardDwgCaptureTestAccess::raw(baseline,geometryHandle);
    snapshot.m_rawBytes[0]^=1u;
    CHECK_G(!liveGeometry(baseline));CHECK_G(!dx_iface::hasUnchangedVerifiedXRecordSource(baseline,rootHandle));
    CHECK_G(!DxIfaceStandardDwgCaptureTestAccess::writerObservationCheck(reader,baseline));
    snapshot.m_rawBytes[0]^=1u;CHECK_G(liveGeometry(baseline));
    CHECK_G(DxIfaceStandardDwgCaptureTestAccess::writerObservationCheck(reader,baseline));
    dx_data second;CHECK_G(reader.fileImport(path.string(),&second,false));CHECK_G(liveGeometry(second));
    DxIfaceStandardDwgCaptureTestAccess::transplant(second,baseline);
    CHECK_G(!liveGeometry(second));CHECK_G(!dx_iface::hasUnchangedVerifiedXRecordSource(second,rootHandle));
    CHECK_G(!DxIfaceStandardDwgCaptureTestAccess::writerObservationCheck(reader,second));
    CHECK_G(reader.enableVerifiedTableCandidates(xrecords,contentHandle,dataHandle,0xcau));
    dx_data missing;(void)reader.fileImport(path.string(),&missing,false);
    CHECK_G(!liveGeometry(missing));CHECK_G(!dx_iface::hasUnchangedVerifiedXRecordSource(missing,rootHandle));
    CHECK_G(selectedGeometry(reader));dx_data retry;CHECK_G(reader.fileImport(path.string(),&retry,false));CHECK_G(liveGeometry(retry));
    for(int stage=1;stage<=3;++stage) {
        dx_iface a;CHECK_G(selectedGeometry(a));dx_data failed;sealingAdapter=&a;sealingStage=stage;
        fault::predicate=sealFaultReady;fault::fired=false;const bool ok=a.fileImport(path.string(),&failed,false);
        fault::predicate=nullptr;sealingAdapter=nullptr;
        CHECK_G(ok && fault::fired);CHECK_G(!liveGeometry(failed));
        CHECK_G(!dx_iface::hasUnchangedVerifiedXRecordSource(failed,rootHandle));
        dx_data again;CHECK_G(a.fileImport(path.string(),&again,false));CHECK_G(liveGeometry(again));
    }
    for(const auto injection:{FaultAdapter::Injection::Candidate,FaultAdapter::Injection::Ticket}) {
        FaultAdapter a;CHECK_G(selectedGeometry(a));a.target=geometryHandle;a.injection=injection;fault::fired=false;dx_data failed;
        const bool ok=a.fileImport(path.string(),&failed,false);fault::remaining=-1;
        CHECK_G(ok && fault::fired);CHECK_G(!liveGeometry(failed));
        CHECK_G(!dx_iface::hasUnchangedVerifiedXRecordSource(failed,rootHandle));
        CHECK_G(injection!=FaultAdapter::Injection::Ticket || a.ticket);
        a.injection=FaultAdapter::Injection::None;dx_data again;
        CHECK_G(a.fileImport(path.string(),&again,false));CHECK_G(liveGeometry(again));
    }
    Reentry same;dx_data sameData;same.data=&sameData;same.path=path.string();CHECK_G(selectedGeometry(same));
    (void)same.fileImport(path.string(),&sameData,false);CHECK_G(same.attempted && !liveGeometry(sameData));
    CHECK_G(!dx_iface::hasUnchangedVerifiedXRecordSource(sameData,rootHandle));
    dx_data sameRetry;CHECK_G(same.fileImport(path.string(),&sameRetry,false));CHECK_G(liveGeometry(sameRetry));
    Reentry outer;dx_iface inner;dx_data cross;outer.data=&cross;outer.nested=&inner;outer.path=path.string();
    CHECK_G(selectedGeometry(outer) && selectedGeometry(inner));(void)outer.fileImport(path.string(),&cross,false);
    CHECK_G(outer.attempted && !liveGeometry(cross));CHECK_G(!dx_iface::hasUnchangedVerifiedXRecordSource(cross,rootHandle));
    dx_data crossRetry;CHECK_G(outer.fileImport(path.string(),&crossRetry,false));CHECK_G(liveGeometry(crossRetry));
    Throwing throwing;throwing.target=geometryHandle;CHECK_G(selectedGeometry(throwing));dx_data thrown;
    (void)throwing.fileImport(path.string(),&thrown,false);CHECK_G(throwing.seen && !liveGeometry(thrown));
    CHECK_G(!dx_iface::hasUnchangedVerifiedXRecordSource(thrown,rootHandle));
    throwing.fail=false;dx_data throwRetry;CHECK_G(throwing.fileImport(path.string(),&throwRetry,false));CHECK_G(liveGeometry(throwRetry));
    for(int mode=1;mode<=3;++mode) {
        Spoofing spoof;spoof.target=geometryHandle;spoof.mode=mode;CHECK_G(selectedGeometry(spoof));dx_data altered;
        (void)spoof.fileImport(path.string(),&altered,false);CHECK_G(spoof.seen && !liveGeometry(altered));
        CHECK_G(!dx_iface::hasUnchangedVerifiedXRecordSource(altered,rootHandle));
        spoof.mode=0;dx_data fresh;CHECK_G(spoof.fileImport(path.string(),&fresh,false));CHECK_G(liveGeometry(fresh));
    }
    const auto bad=path.string()+".geometry-bad.dwg";
    struct Cleanup {std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove(p,e);}} cleanup{bad};
    auto badGeometry=rawGeometry(badGeometryPath);CHECK_G(write(bad,content,table,0,&badGeometry));dx_data badData;
    (void)reader.fileImport(bad,&badData,false);CHECK_G(!liveGeometry(badData));
    CHECK_G(!dx_iface::hasUnchangedVerifiedXRecordSource(badData,rootHandle));
    dx_data good;CHECK_G(reader.fileImport(path.string(),&good,false));CHECK_G(liveGeometry(good));
    const auto output=path.string()+".geometry.dxf";Cleanup outputCleanup{output};
    {std::ofstream f(output,std::ios::binary|std::ios::trunc);f<<"SOURCE_SENTINEL";CHECK_G(static_cast<bool>(f));}
    CHECK_G(!reader.fileExport(output,DRW::AC1018,false,&good,false,true,dx_dxfSourceTextPolicy::Compatible,
        DRW::DxfTextWritePolicy::Compatible,dx_dxfProxyGraphicsPolicy::Compatible,dx_dwgReadCompletenessPolicy::RequireComplete));
    {std::ifstream f(output,std::ios::binary);const std::string value((std::istreambuf_iterator<char>(f)),{});CHECK_G(value=="SOURCE_SENTINEL");}
    std::vector<std::uint8_t> original;CHECK_G(corruptSelectedCrc(path,DxIfaceStandardDwgCaptureTestAccess::raw(good,geometryHandle),original));
    dx_data crcBad;(void)reader.fileImport(path.string(),&crcBad,false);CHECK_G(!liveGeometry(crcBad));
    CHECK_G(!dx_iface::hasUnchangedVerifiedXRecordSource(crcBad,rootHandle));
    const auto& integrity=crcBad.dwgReadOutcome.observations.integrity;
    CHECK_G(!integrity.extractionOmitted && !integrity.reportedUpstreamDropped && !integrity.aggregateSaturated);
    bool targetFailure=false;
    for(std::size_t i=0;i<integrity.retained;++i) {
        const auto& row=integrity.rows[i];CHECK_G(row.kind!=DwgIntegrityCheckKind::DataPageChecksum && row.kind!=DwgIntegrityCheckKind::DataPageCrc);
        if(row.kind==DwgIntegrityCheckKind::ObjectFrameBounds && row.hasLogicalHandle && row.logicalHandle==geometryHandle)targetFailure=true;
    }
    CHECK_G(targetFailure);
    {std::ofstream f(path,std::ios::binary|std::ios::trunc);f.write(reinterpret_cast<const char*>(original.data()),original.size());CHECK_G(static_cast<bool>(f));}
    dx_data crcRetry;CHECK_G(reader.fileImport(path.string(),&crcRetry,false));CHECK_G(liveGeometry(crcRetry));
    auto oversized=geometry;oversized.m_rawBytes.resize(4097u,0u);oversized.m_objectSize=4097u;
    CHECK_G(write(bad,content,table,0,&oversized));dx_data quota;FaultAdapter quotaReader;
    quotaReader.target=geometryHandle;CHECK_G(selectedGeometry(quotaReader));
    CHECK_G(quotaReader.fileImport(bad,&quota,false));CHECK_G(quotaReader.wantedTarget && quotaReader.targetTickets==0u);CHECK_G(!liveGeometry(quota));
    CHECK_G(!dx_iface::hasUnchangedVerifiedXRecordSource(quota,rootHandle));
    dx_data quotaRetry;CHECK_G(quotaReader.fileImport(path.string(),&quotaRetry,false));
    CHECK_G(quotaReader.targetTickets==1u && liveGeometry(quotaRetry));
    std::cout<<"geometry bundle failure and retry controls passed\n";
#undef CHECK_G
    return true;
}

bool selectedRoundtrip(dx_iface& a) {
    return a.enableVerifiedTableRoundtripCandidates(xrecords,contentHandle,dataHandle,geometryHandle);
}
bool liveRoundtrip(const dx_data& d) {
    Content c;Data t;Geometry g;DwgTableRoundtripXRecordAc1018Candidate::Result root;
    return dx_iface::readVerifiedTableRoundtripCandidates(d,xrecords,contentHandle,dataHandle,geometryHandle,c,t,g,root)
        && root.handle==rootHandle && root.references[0].target==contentHandle
        && root.references[1].target==geometryHandle && root.references[2].target==dataHandle;
}

void patchRawBits(std::vector<std::uint8_t>& body,std::size_t offset,std::uint64_t value,std::size_t byteCount) {
    for(std::size_t i=0;i<byteCount;++i)for(std::size_t bit=0;bit<8u;++bit) {
        const auto dest=offset+i*8u+bit;const auto mask=static_cast<std::uint8_t>(1u<<(7u-dest%8u));
        if((value>>(i*8u+7u-bit))&1u)body.at(dest/8u)|=mask;else body.at(dest/8u)&=static_cast<std::uint8_t>(~mask);
    }
}
class SpoofRoot final : public dx_iface {
public:
    bool enabled=true,seen=false;
    void addXRecord(const DRW_XRecord& value) override {
        if(enabled && value.handle==rootHandle){seen=true;auto changed=value;
            changed.m_dataEntries[1]=DRW_Variant(360,std::uint64_t{0xcau});dx_iface::addXRecord(changed);return;}
        dx_iface::addXRecord(value);
    }
};
class Downgrade final : public dx_iface {
public:
    bool attempted=false,accepted=false;
    void addXRecord(const DRW_XRecord& value) override {
        if(value.handle==rootHandle && !attempted){attempted=true;
            accepted=enableVerifiedTableCandidates(xrecords,contentHandle,dataHandle,geometryHandle);}
        dx_iface::addXRecord(value);
    }
};
bool runRoundtrip(const char* contentPath,const char* dataPath,const char* geometryPath,
                  const char* rootPath,const std::filesystem::path& path) {
#define CHECK_R(expr) do { if(!(expr)){std::cerr<<"roundtrip binding failed line "<<__LINE__<<'\n';return false;} } while(false)
    auto content=raw(contentPath,true),table=raw(dataPath,false),geometry=rawGeometry(geometryPath);
    CHECK_R(write(path,content,table,0,&geometry,true));
    dx_iface source;dx_data initial;CHECK_R(source.fileImport(path.string(),&initial,false));
    const auto writerRoot=DxIfaceStandardDwgCaptureTestAccess::standardRaw(initial,rootHandle);
    dx_iface defaultReader;CHECK_R(selectedGeometry(defaultReader));dx_data oldProfile;
    CHECK_R(defaultReader.fileImport(path.string(),&oldProfile,false));CHECK_R(liveGeometry(oldProfile));
    dx_iface narrow;CHECK_R(selectedRoundtrip(narrow));dx_data wrongPage;
    CHECK_R(narrow.fileImport(path.string(),&wrongPage,false));CHECK_R(!liveRoundtrip(wrongPage));
    CHECK_R(!dx_iface::hasUnchangedVerifiedXRecordSource(wrongPage,rootHandle));
    std::ifstream f(rootPath,std::ios::binary);std::vector<std::uint8_t> expected((std::istreambuf_iterator<char>(f)),{});
    CHECK_R(!expected.empty());CHECK_R(replaceSelectedBody(path,writerRoot,expected));
    dx_iface reader;CHECK_R(selectedRoundtrip(reader));
    CHECK_R(!reader.enableVerifiedTableRoundtripCandidates(xrecords,contentHandle,dataHandle,DRW::NoHandle));
    CHECK_R(!reader.enableVerifiedTableRoundtripCandidates(xrecords,contentHandle,dataHandle,rootHandle));
    dx_data baseline;
    CHECK_R(reader.fileImport(path.string(),&baseline,false));CHECK_R(liveRoundtrip(baseline));

    CHECK_R(dx_iface::hasUnchangedVerifiedXRecordSources(baseline,xrecords));
    CHECK_R(liveGeometry(baseline));
    Content c;Data t;Geometry g;DwgTableRoundtripXRecordAc1018Candidate::Result root;
    const std::array<std::uint32_t,3> mappedTargets{{0x501u,0x502u,0x503u}};
    std::vector<DRW_Variant> projectedEntries;
    projectedEntries.emplace_back(90,std::int32_t{77});
    CHECK_R(dx_iface::projectVerifiedTableRoundtripReferences(baseline,xrecords,
        contentHandle,dataHandle,geometryHandle,mappedTargets,projectedEntries,root));
    CHECK_R(root.handle==rootHandle && root.owner==0xbbu
        && projectedEntries.size()==13u
        && projectedEntries[1].code()==360 && projectedEntries[1].i64_val()==0x501u
        && projectedEntries[7].code()==361 && projectedEntries[7].i64_val()==0x502u
        && projectedEntries[12].code()==360 && projectedEntries[12].i64_val()==0x503u);
    fault::count=0;fault::counting=true;
    const bool projectionCounted=dx_iface::projectVerifiedTableRoundtripReferences(
        baseline,xrecords,contentHandle,dataHandle,geometryHandle,mappedTargets,
        projectedEntries,root);
    fault::counting=false;const auto projectionAllocations=fault::count;
    CHECK_R(projectionCounted && projectionAllocations>0u);
    const auto projectedSnapshot=projectedEntries;
    const std::array<std::size_t,2> projectionFaults{{0u,projectionAllocations-1u}};
    for(const auto allocation:projectionFaults) {
        projectedEntries=projectedSnapshot;root.handle=9u;fault::fired=false;
        fault::remaining=static_cast<long>(allocation);
        const bool accepted=dx_iface::projectVerifiedTableRoundtripReferences(
            baseline,xrecords,contentHandle,dataHandle,geometryHandle,mappedTargets,
            projectedEntries,root);
        fault::remaining=-1;
        CHECK_R(!accepted && fault::fired && !root.handle
            && projectedEntries.size()==projectedSnapshot.size()
            && projectedEntries[1].i64_val()==projectedSnapshot[1].i64_val()
            && projectedEntries[7].i64_val()==projectedSnapshot[7].i64_val()
            && projectedEntries[12].i64_val()==projectedSnapshot[12].i64_val());
        CHECK_R(dx_iface::projectVerifiedTableRoundtripReferences(baseline,xrecords,
            contentHandle,dataHandle,geometryHandle,mappedTargets,projectedEntries,root));
    }
    CHECK_R(reader.enableVerifiedTableRoundtripCandidates(xrecords,contentHandle,dataHandle,0xcau));
    dx_data missing;(void)reader.fileImport(path.string(),&missing,false);CHECK_R(!liveRoundtrip(missing));
    CHECK_R(!dx_iface::hasUnchangedVerifiedXRecordSource(missing,rootHandle));
    CHECK_R(selectedRoundtrip(reader));dx_data missingRetry;
    CHECK_R(reader.fileImport(path.string(),&missingRetry,false));CHECK_R(liveRoundtrip(missingRetry));
    CHECK_R(!dx_iface::readVerifiedTableRoundtripCandidates(oldProfile,xrecords,contentHandle,dataHandle,geometryHandle,c,t,g,root));
    const auto item=std::find_if(baseline.xRecords.begin(),baseline.xRecords.end(),[](const DRW_XRecord& x){return x.handle==rootHandle;});
    CHECK_R(item!=baseline.xRecords.end());
    const auto originalEntry=item->m_dataEntries[1];item->m_dataEntries[1]=DRW_Variant(360,std::uint64_t{0xcau});
    CHECK_R(!liveRoundtrip(baseline) && !dx_iface::hasUnchangedVerifiedXRecordSource(baseline,rootHandle));
    root.handle=9u;
    CHECK_R(!dx_iface::projectVerifiedTableRoundtripReferences(baseline,xrecords,
        contentHandle,dataHandle,geometryHandle,mappedTargets,projectedEntries,root));
    CHECK_R(!root.handle && projectedEntries.size()==projectedSnapshot.size()
        && projectedEntries[1].i64_val()==projectedSnapshot[1].i64_val()
        && projectedEntries[7].i64_val()==projectedSnapshot[7].i64_val()
        && projectedEntries[12].i64_val()==projectedSnapshot[12].i64_val());
    item->m_dataEntries[1]=originalEntry;CHECK_R(liveRoundtrip(baseline));
    CHECK_R(dx_iface::projectVerifiedTableRoundtripReferences(baseline,xrecords,
        contentHandle,dataHandle,geometryHandle,mappedTargets,projectedEntries,root));
    CHECK_R(projectedEntries[1].i64_val()==0x501u
        && projectedEntries[7].i64_val()==0x502u
        && projectedEntries[12].i64_val()==0x503u);
    item->m_values.emplace_back(90,std::int32_t{5});CHECK_R(!liveRoundtrip(baseline));item->m_values.pop_back();
    item->m_handleValues.emplace_back(360,0xcau);CHECK_R(!liveRoundtrip(baseline));item->m_handleValues.pop_back();
    CHECK_R(liveRoundtrip(baseline));
    auto& rootRaw=const_cast<DRW_UnsupportedObject&>(DxIfaceStandardDwgCaptureTestAccess::standardRaw(baseline,rootHandle));
    rootRaw.m_rawBytes[0]^=1u;CHECK_R(!liveRoundtrip(baseline));rootRaw.m_rawBytes[0]^=1u;CHECK_R(liveRoundtrip(baseline));
    baseline.xRecords.push_back(*item);CHECK_R(!liveRoundtrip(baseline));baseline.xRecords.pop_back();CHECK_R(liveRoundtrip(baseline));
    dx_data second;CHECK_R(reader.fileImport(path.string(),&second,false));CHECK_R(liveRoundtrip(second));
    DxIfaceStandardDwgCaptureTestAccess::transplant(second,baseline);CHECK_R(!liveRoundtrip(second));
    for(int stage=1;stage<=3;++stage) {
        dx_iface a;CHECK_R(selectedRoundtrip(a));dx_data failed;sealingAdapter=&a;sealingStage=stage;
        fault::predicate=sealFaultReady;fault::fired=false;const bool ok=a.fileImport(path.string(),&failed,false);
        fault::predicate=nullptr;sealingAdapter=nullptr;CHECK_R(ok && fault::fired && !liveRoundtrip(failed));
        CHECK_R(!dx_iface::hasUnchangedVerifiedXRecordSource(failed,rootHandle));
        dx_data again;CHECK_R(a.fileImport(path.string(),&again,false));CHECK_R(liveRoundtrip(again));
    }
    c.handle=t.handle=g.handle=root.handle=9u;fault::remaining=0;fault::fired=false;
    const bool oom=dx_iface::readVerifiedTableRoundtripCandidates(baseline,xrecords,contentHandle,dataHandle,geometryHandle,c,t,g,root);
    fault::remaining=-1;CHECK_R(!oom && fault::fired && !c.handle && !t.handle && !g.handle && !root.handle);CHECK_R(liveRoundtrip(baseline));
    fault::count=0;fault::counting=true;const bool counted=dx_iface::readVerifiedTableRoundtripCandidates(
        baseline,xrecords,contentHandle,dataHandle,geometryHandle,c,t,g,root);fault::counting=false;
    const auto queryAllocations=fault::count;CHECK_R(counted && queryAllocations>0u);
    c.handle=t.handle=g.handle=root.handle=9u;fault::remaining=static_cast<long>(queryAllocations-1u);fault::fired=false;
    const bool lateOom=dx_iface::readVerifiedTableRoundtripCandidates(baseline,xrecords,contentHandle,dataHandle,geometryHandle,c,t,g,root);
    fault::remaining=-1;CHECK_R(!lateOom && fault::fired && !c.handle && !t.handle && !g.handle && !root.handle);
    CHECK_R(liveRoundtrip(baseline));std::cout<<"root query allocations="<<queryAllocations<<" (first and last faults sampled)\n";
    Reentry same;dx_data sameData;same.data=&sameData;same.path=path.string();CHECK_R(selectedRoundtrip(same));
    (void)same.fileImport(path.string(),&sameData,false);CHECK_R(same.attempted && !liveRoundtrip(sameData));
    dx_data sameRetry;CHECK_R(same.fileImport(path.string(),&sameRetry,false));CHECK_R(liveRoundtrip(sameRetry));
    Reentry outer;dx_iface inner;dx_data cross;outer.data=&cross;outer.nested=&inner;outer.path=path.string();
    CHECK_R(selectedRoundtrip(outer) && selectedRoundtrip(inner));(void)outer.fileImport(path.string(),&cross,false);
    CHECK_R(outer.attempted && !liveRoundtrip(cross));dx_data crossRetry;
    CHECK_R(outer.fileImport(path.string(),&crossRetry,false));CHECK_R(liveRoundtrip(crossRetry));
    Downgrade downgrade;CHECK_R(selectedRoundtrip(downgrade));dx_data protectedData;
    CHECK_R(downgrade.fileImport(path.string(),&protectedData,false));
    CHECK_R(downgrade.attempted && !downgrade.accepted && liveRoundtrip(protectedData));
    SpoofRoot spoof;CHECK_R(selectedRoundtrip(spoof));dx_data spoofed;(void)spoof.fileImport(path.string(),&spoofed,false);
    CHECK_R(spoof.seen && !liveRoundtrip(spoofed));spoof.enabled=false;dx_data spoofRetry;
    CHECK_R(spoof.fileImport(path.string(),&spoofRetry,false));CHECK_R(liveRoundtrip(spoofRetry));
    const auto output=path.string()+".roundtrip.dxf";
    {std::ofstream out(output,std::ios::binary|std::ios::trunc);out<<"SENTINEL";CHECK_R(static_cast<bool>(out));}
    CHECK_R(!reader.fileExport(output,DRW::AC1018,false,&baseline,false,true,dx_dxfSourceTextPolicy::Compatible,
        DRW::DxfTextWritePolicy::Compatible,dx_dxfProxyGraphicsPolicy::Compatible,dx_dwgReadCompletenessPolicy::RequireComplete));
    {std::ifstream in(output,std::ios::binary);const std::string value((std::istreambuf_iterator<char>(in)),{});CHECK_R(value=="SENTINEL");}
    {std::error_code e;std::filesystem::remove(output,e);}
    const auto goodRaw=DxIfaceStandardDwgCaptureTestAccess::standardRaw(baseline,rootHandle);
    // Every modified root remains CRC-valid and inside a valid data page.
    for(int mode=0;mode<8;++mode) {
        auto changed=expected;
        if(mode==0)patchRawBits(changed,377u+16u,0xcau,8u);
        if(mode==1)patchRawBits(changed,841u+16u,0xcau,8u);
        if(mode==2)patchRawBits(changed,1577u+16u,0xcau,8u);
        if(mode==3){patchRawBits(changed,377u+16u,0xbau,8u);patchRawBits(changed,841u+16u,0xb9u,8u);}
        if(mode==4)patchRawBits(changed,841u+16u,0xb9u,8u);
        if(mode==5)patchRawBits(changed,377u,361u,2u);
        if(mode==6)patchRawBits(changed,113u,30u,1u);
        if(mode==7)patchRawBits(changed,97u,31u,2u);
        CHECK_R(replaceSelectedBody(path,goodRaw,changed));
        if(mode<7) {dx_data observational;CHECK_R(defaultReader.fileImport(path.string(),&observational,false));
            CHECK_R(liveGeometry(observational));}
        dx_data bad;(void)reader.fileImport(path.string(),&bad,false);CHECK_R(!liveRoundtrip(bad));
        CHECK_R(!dx_iface::hasUnchangedVerifiedXRecordSource(bad,rootHandle));
        auto current=goodRaw;current.m_rawBytes=std::move(changed);
        CHECK_R(replaceSelectedBody(path,current,expected));dx_data restored;
        CHECK_R(reader.fileImport(path.string(),&restored,false));CHECK_R(liveRoundtrip(restored));
    }
    std::vector<std::uint8_t> original;CHECK_R(corruptSelectedCrc(path,goodRaw,original));dx_data badCrc;
    (void)reader.fileImport(path.string(),&badCrc,false);CHECK_R(!liveRoundtrip(badCrc));
    CHECK_R(!dx_iface::hasUnchangedVerifiedXRecordSource(badCrc,rootHandle));
    const auto& integrity=badCrc.dwgReadOutcome.observations.integrity;
    CHECK_R(!integrity.extractionOmitted && !integrity.reportedUpstreamDropped && !integrity.aggregateSaturated);
    bool rootFailure=false;for(std::size_t i=0;i<integrity.retained;++i){const auto& row=integrity.rows[i];
        CHECK_R(row.kind!=DwgIntegrityCheckKind::DataPageChecksum && row.kind!=DwgIntegrityCheckKind::DataPageCrc);
        if(row.kind==DwgIntegrityCheckKind::ObjectFrameBounds && row.hasLogicalHandle && row.logicalHandle==rootHandle)rootFailure=true;}
    CHECK_R(rootFailure);
    {std::ofstream out(path,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char*>(original.data()),original.size());CHECK_R(static_cast<bool>(out));}
    dx_data crcRetry;CHECK_R(reader.fileImport(path.string(),&crcRetry,false));CHECK_R(liveRoundtrip(crcRetry));
    std::cout<<"root roundtrip binding initial generated pass\n";
#undef CHECK_R
    return true;
}

bool selectedChildren(dx_iface& reader) {
    return reader.enableVerifiedTableRoundtripChildren(fullXrecords,contentHandle,dataHandle,geometryHandle);
}
bool liveChildren(const dx_data& d) {
    Content c;Data t;Geometry g;DwgTableRoundtripXRecordAc1018Candidate::Result root;
    std::array<DwgTableChildXRecordAc1018Candidate::Result,9> children;
    if(!dx_iface::readVerifiedTableRoundtripChildren(d,fullXrecords,contentHandle,dataHandle,geometryHandle,c,t,g,root,children))return false;
    for(std::size_t i=0;i<children.size();++i)if(children[i].handle!=0xbeu+i || children[i].owner!=dataHandle)return false;
    return children[0].text=="Title" && !children[1].hasText && !children[2].hasText && children[8].text=="6";
}
class SpoofChild final : public dx_iface {
public:
    bool enabled=true,seen=false;
    void addXRecord(const DRW_XRecord& value) override {
        if(enabled && value.handle==0xbeu){seen=true;auto altered=value;
            altered.m_dataEntries[2]=DRW_Variant(1,UTF8STRING("spoofed"));dx_iface::addXRecord(altered);return;}
        dx_iface::addXRecord(value);
    }
};
class SpoofDictionary final : public dx_iface {
public:
    bool enabled=true,seen=false;
    void addDictionary(const DRW_Dictionary& value) override {
        if(enabled && value.handle==0xbbu){seen=true;auto altered=value;
            altered.m_entries[0].m_handle=0xcau;dx_iface::addDictionary(altered);return;}
        dx_iface::addDictionary(value);
    }
};
bool selectedDictionary(dx_iface& a){
    return a.enableVerifiedTableExtensionDictionary(fullXrecords,contentHandle,dataHandle,geometryHandle,0x9cu,0xbbu);
}
bool liveDictionary(const dx_data& d){
    DwgTableExtensionDictionaryAc1018Candidate::Result r;
    return dx_iface::readVerifiedTableExtensionDictionary(d,fullXrecords,contentHandle,dataHandle,geometryHandle,0x9cu,0xbbu,r)
        && r.handle==0xbbu && r.owner==0x9cu && r.root==rootHandle && r.dataEndBit==253u
        && r.ownerStartBit==253u && r.ownerEndBit==269u && r.rootStartBit==269u && r.rootEndBit==285u;
}
bool runChildren(const char* contentPath,const char* dataPath,const char* geometryPath,const char* rootPath,
                 const std::filesystem::path& path) {
#define CHECK_C(expr) do { if(!(expr)){std::cerr<<"child binding failed line "<<__LINE__<<'\n';return false;} } while(false)
    auto content=raw(contentPath,true),table=raw(dataPath,false),geometry=rawGeometry(geometryPath);
    CHECK_C(write(path,content,table,0,&geometry,true,true));
    dx_iface source;dx_data initial;CHECK_C(source.fileImport(path.string(),&initial,false));
    std::ifstream f(rootPath,std::ios::binary);std::vector<std::uint8_t> expectedRoot((std::istreambuf_iterator<char>(f)),{});
    CHECK_C(replaceSelectedBody(path,DxIfaceStandardDwgCaptureTestAccess::standardRaw(initial,rootHandle),expectedRoot));
    const auto writerDict=DxIfaceStandardDwgCaptureTestAccess::standardRaw(initial,0xbbu);
    auto normalizedDict=writerDict.m_rawBytes;patchRawBits(normalizedDict,245u,'P',1u);
    CHECK_C(replaceSelectedBody(path,writerDict,normalizedDict));
    dx_iface old;CHECK_C(old.enableVerifiedTableRoundtripCandidates(fullXrecords,contentHandle,dataHandle,geometryHandle));
    dx_data initialOld;CHECK_C(old.fileImport(path.string(),&initialOld,false));
    Content oc;Data ot;Geometry og;DwgTableRoundtripXRecordAc1018Candidate::Result oroot;
    CHECK_C(dx_iface::readVerifiedTableRoundtripCandidates(initialOld,fullXrecords,contentHandle,dataHandle,geometryHandle,oc,ot,og,oroot));
    dx_iface reader;CHECK_C(selectedChildren(reader));dx_data incomplete;
    CHECK_C(reader.fileImport(path.string(),&incomplete,false));CHECK_C(!liveChildren(incomplete));
    CHECK_C(!dx_iface::hasUnchangedVerifiedXRecordSource(incomplete,rootHandle));
    for(std::uint32_t h=0xbeu;h<=0xc6u;++h){
        ChildFixture::Options o;o.handle=h;o.empty=(h==0xbfu||h==0xc0u);if(h>=0xc1u)o.value=std::to_string(h-0xc0u);
        CHECK_C(replaceSelectedBody(path,DxIfaceStandardDwgCaptureTestAccess::standardRaw(initial,h),ChildFixture::build(o).raw.m_rawBytes));
    }
    dx_iface extension;CHECK_C(selectedDictionary(extension));
    CHECK_C(!extension.enableVerifiedTableExtensionDictionary(fullXrecords,contentHandle,dataHandle,geometryHandle,0x9cu,rootHandle));
    dx_data dictBaseline;CHECK_C(extension.fileImport(path.string(),&dictBaseline,false));
    CHECK_C(liveDictionary(dictBaseline));
    CHECK_C(dx_iface::hasUnchangedVerifiedXRecordSources(dictBaseline,fullXrecords));
    const auto dict=std::find_if(dictBaseline.dictionaries.begin(),dictBaseline.dictionaries.end(),[](const DRW_Dictionary& d){return d.handle==0xbbu;});
    CHECK_C(dict!=dictBaseline.dictionaries.end());dict->m_entries[0].m_handle=0xcau;
    CHECK_C(!liveDictionary(dictBaseline));CHECK_C(!dx_iface::hasUnchangedVerifiedXRecordSource(dictBaseline,rootHandle));
    dict->m_entries[0].m_handle=rootHandle;CHECK_C(liveDictionary(dictBaseline));
    auto& dictRaw=const_cast<DRW_UnsupportedObject&>(DxIfaceStandardDwgCaptureTestAccess::standardRaw(dictBaseline,0xbbu));
    dictRaw.m_rawBytes[0]^=1u;CHECK_C(!liveDictionary(dictBaseline));dictRaw.m_rawBytes[0]^=1u;CHECK_C(liveDictionary(dictBaseline));
    auto& pub=DxIfaceStandardDwgCaptureTestAccess::publication(dictBaseline,0xbbu);++pub.m_sourceMapOrdinal;
    CHECK_C(!liveDictionary(dictBaseline));--pub.m_sourceMapOrdinal;CHECK_C(liveDictionary(dictBaseline));
    const auto dictCopy=*dict;dictBaseline.dictionaries.push_back(dictCopy);CHECK_C(!liveDictionary(dictBaseline));dictBaseline.dictionaries.pop_back();CHECK_C(liveDictionary(dictBaseline));
    dx_data another;CHECK_C(extension.fileImport(path.string(),&another,false));CHECK_C(liveDictionary(another));
    DxIfaceStandardDwgCaptureTestAccess::transplant(another,dictBaseline);CHECK_C(!liveDictionary(another));
    for(int stage=1;stage<=3;++stage){dx_iface a;CHECK_C(selectedDictionary(a));dx_data failed;
        sealingAdapter=&a;sealingStage=stage;fault::predicate=sealFaultReady;fault::fired=false;
        const bool ok=a.fileImport(path.string(),&failed,false);fault::predicate=nullptr;sealingAdapter=nullptr;
        CHECK_C(ok && fault::fired && !liveDictionary(failed));CHECK_C(!dx_iface::hasUnchangedVerifiedXRecordSource(failed,rootHandle));
        dx_data again;CHECK_C(a.fileImport(path.string(),&again,false));CHECK_C(liveDictionary(again));}
    SpoofDictionary spoofDict;CHECK_C(selectedDictionary(spoofDict));dx_data spoofedDict;
    (void)spoofDict.fileImport(path.string(),&spoofedDict,false);CHECK_C(spoofDict.seen && !liveDictionary(spoofedDict));
    spoofDict.enabled=false;dx_data spoofRetryDict;CHECK_C(spoofDict.fileImport(path.string(),&spoofRetryDict,false));CHECK_C(liveDictionary(spoofRetryDict));
    Reentry sameDict;dx_data reentered;sameDict.data=&reentered;sameDict.path=path.string();CHECK_C(selectedDictionary(sameDict));
    (void)sameDict.fileImport(path.string(),&reentered,false);CHECK_C(sameDict.attempted && !liveDictionary(reentered));
    dx_data retryDict;CHECK_C(sameDict.fileImport(path.string(),&retryDict,false));CHECK_C(liveDictionary(retryDict));
    Reentry outerDict;dx_iface innerDict;dx_data crossDict;outerDict.data=&crossDict;outerDict.nested=&innerDict;outerDict.path=path.string();
    CHECK_C(selectedDictionary(outerDict) && selectedDictionary(innerDict));(void)outerDict.fileImport(path.string(),&crossDict,false);
    CHECK_C(outerDict.attempted && !liveDictionary(crossDict));dx_data crossDictRetry;
    CHECK_C(outerDict.fileImport(path.string(),&crossDictRetry,false));CHECK_C(liveDictionary(crossDictRetry));

    const auto dictOutput=path.string()+".dictionary.dxf";
    {std::ofstream out(dictOutput,std::ios::binary|std::ios::trunc);out<<"DICTIONARY_SENTINEL";CHECK_C(static_cast<bool>(out));}
    CHECK_C(!extension.fileExport(dictOutput,DRW::AC1018,false,&dictBaseline,false,true,dx_dxfSourceTextPolicy::Compatible,
        DRW::DxfTextWritePolicy::Compatible,dx_dxfProxyGraphicsPolicy::Compatible,dx_dwgReadCompletenessPolicy::RequireComplete));
    {std::ifstream in(dictOutput,std::ios::binary);const std::string value((std::istreambuf_iterator<char>(in)),{});CHECK_C(value=="DICTIONARY_SENTINEL");}
    {std::error_code e;std::filesystem::remove(dictOutput,e);}
    DwgTableExtensionDictionaryAc1018Candidate::Result dictResult;
    fault::count=0;fault::counting=true;const bool countedDict=dx_iface::readVerifiedTableExtensionDictionary(
        dictBaseline,fullXrecords,contentHandle,dataHandle,geometryHandle,0x9cu,0xbbu,dictResult);fault::counting=false;
    const auto dictAllocations=fault::count;CHECK_C(countedDict && dictAllocations>0u);dictResult.handle=9u;
    fault::remaining=static_cast<long>(dictAllocations-1u);fault::fired=false;
    const bool dictOom=dx_iface::readVerifiedTableExtensionDictionary(dictBaseline,fullXrecords,contentHandle,dataHandle,geometryHandle,0x9cu,0xbbu,dictResult);
    fault::remaining=-1;CHECK_C(!dictOom && fault::fired && !dictResult.handle);CHECK_C(liveDictionary(dictBaseline));
    std::cout<<"dictionary query allocations="<<dictAllocations<<" (last fault sampled)\n";
    const auto originalDict=DxIfaceStandardDwgCaptureTestAccess::standardRaw(dictBaseline,0xbbu);

    {
        auto candidateRaw=originalDict;auto candidateTyped=*dict;
        DwgTableExtensionDictionaryAc1018Candidate::Result observed;
        CHECK_C(DwgTableExtensionDictionaryAc1018Candidate::inspect(candidateRaw,candidateTyped,observed));
        const auto reject=[&](){observed.handle=1u;return !DwgTableExtensionDictionaryAc1018Candidate::inspect(candidateRaw,candidateTyped,observed) && !observed.handle;};
        for(std::size_t i=0;i<originalDict.m_rawBytes.size();++i){candidateRaw.m_rawBytes.assign(originalDict.m_rawBytes.begin(),originalDict.m_rawBytes.begin()+i);
            candidateRaw.m_objectSize=static_cast<std::uint32_t>(i);CHECK_C(reject());}
        candidateRaw=originalDict;candidateTyped.m_entries[0].m_name="WRONG";CHECK_C(reject());candidateTyped=*dict;
        candidateTyped.handle++;CHECK_C(reject());candidateTyped=*dict;
        candidateRaw.m_rawBytes.resize(4097u);candidateRaw.m_objectSize=4097u;CHECK_C(reject());candidateRaw=originalDict;
        candidateRaw.m_rawBytes.back()|=1u;CHECK_C(reject());candidateRaw=originalDict;
        patchRawBits(candidateRaw.m_rawBytes,93u,18u,1u);CHECK_C(reject());candidateRaw=originalDict;
        patchRawBits(candidateRaw.m_rawBytes,261u,0xbcu,1u);CHECK_C(reject());candidateRaw=originalDict;
        // Make the owner handle claim five bytes (>32 bits), with too little data remaining.
        candidateRaw.m_rawBytes[258u/8u]^=static_cast<std::uint8_t>(1u<<(7u-258u%8u));CHECK_C(reject());candidateRaw=originalDict;
        // Invalid BL selector 11 in item count; a two-bit replacement preserves frame width.
        candidateRaw.m_rawBytes[63u/8u]|=static_cast<std::uint8_t>(1u<<(7u-63u%8u));
        candidateRaw.m_rawBytes[64u/8u]|=static_cast<std::uint8_t>(1u<<(7u-64u%8u));CHECK_C(reject());candidateRaw=originalDict;
        fault::count=0;fault::counting=true;const bool measured=DwgTableExtensionDictionaryAc1018Candidate::inspect(candidateRaw,candidateTyped,observed);fault::counting=false;
        const auto allocations=fault::count;CHECK_C(measured && allocations>0u);
        for(std::size_t i=0;i<allocations;++i){fault::remaining=static_cast<long>(i);CHECK_C(reject());fault::remaining=-1;
            CHECK_C(DwgTableExtensionDictionaryAc1018Candidate::inspect(candidateRaw,candidateTyped,observed));}
        std::cout<<"dictionary observer allocation faults swept="<<allocations<<"\n";
    }
    for(int mode=0;mode<6;++mode){auto changed=originalDict.m_rawBytes;
        if(mode==0)changed[256u/8u]^=static_cast<std::uint8_t>(1u<<(7u-256u%8u));
        if(mode==1)changed[272u/8u]^=static_cast<std::uint8_t>(1u<<(7u-272u%8u));
        if(mode==2)patchRawBits(changed,261u,0x9du,1u);
        if(mode==3)patchRawBits(changed,277u,0xcau,1u);
        if(mode==4)patchRawBits(changed,101u,'X',1u);
        if(mode==5)patchRawBits(changed,83u,0u,1u);
        CHECK_C(replaceSelectedBody(path,originalDict,changed));
        if(mode==2 || mode==3){dx_data prior;CHECK_C(reader.fileImport(path.string(),&prior,false));CHECK_C(liveChildren(prior));}
        dx_data bad;(void)extension.fileImport(path.string(),&bad,false);CHECK_C(!liveDictionary(bad));
        CHECK_C(!dx_iface::hasUnchangedVerifiedXRecordSource(bad,rootHandle));
        auto current=originalDict;current.m_rawBytes=changed;CHECK_C(replaceSelectedBody(path,current,originalDict.m_rawBytes));
        dx_data recovered;CHECK_C(extension.fileImport(path.string(),&recovered,false));CHECK_C(liveDictionary(recovered));
    }
    std::vector<std::uint8_t> dictOriginalFile;CHECK_C(corruptSelectedCrc(path,originalDict,dictOriginalFile));dx_data dictCrc;
    (void)extension.fileImport(path.string(),&dictCrc,false);CHECK_C(!liveDictionary(dictCrc));
    const auto& dictIntegrity=dictCrc.dwgReadOutcome.observations.integrity;
    CHECK_C(!dictIntegrity.extractionOmitted && !dictIntegrity.reportedUpstreamDropped && !dictIntegrity.aggregateSaturated);
    bool dictFailure=false;for(std::size_t i=0;i<dictIntegrity.retained;++i){const auto& row=dictIntegrity.rows[i];
        CHECK_C(row.kind!=DwgIntegrityCheckKind::DataPageChecksum && row.kind!=DwgIntegrityCheckKind::DataPageCrc);
        if(row.kind==DwgIntegrityCheckKind::ObjectFrameBounds && row.hasLogicalHandle && row.logicalHandle==0xbbu)dictFailure=true;}
    CHECK_C(dictFailure);{std::ofstream out(path,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char*>(dictOriginalFile.data()),dictOriginalFile.size());CHECK_C(static_cast<bool>(out));}
    dx_data dictRetry;CHECK_C(extension.fileImport(path.string(),&dictRetry,false));CHECK_C(liveDictionary(dictRetry));
    auto shortSet=fullXrecords;shortSet.pop_back();CHECK_C(!reader.enableVerifiedTableRoundtripChildren(shortSet,contentHandle,dataHandle,geometryHandle));
    auto duplicate=fullXrecords;duplicate[1]=duplicate[2];CHECK_C(!reader.enableVerifiedTableRoundtripChildren(duplicate,contentHandle,dataHandle,geometryHandle));
    dx_data baseline;CHECK_C(reader.fileImport(path.string(),&baseline,false));CHECK_C(liveChildren(baseline));
    CHECK_C(dx_iface::hasUnchangedVerifiedXRecordSources(baseline,fullXrecords));
    auto child=std::find_if(baseline.xRecords.begin(),baseline.xRecords.end(),[](const DRW_XRecord& x){return x.handle==0xbeu;});
    CHECK_C(child!=baseline.xRecords.end());const auto entry=child->m_dataEntries[2];
    child->m_dataEntries[2]=DRW_Variant(1,UTF8STRING("changed"));CHECK_C(!liveChildren(baseline));
    CHECK_C(!dx_iface::hasUnchangedVerifiedXRecordSource(baseline,rootHandle));child->m_dataEntries[2]=entry;CHECK_C(liveChildren(baseline));
    auto& childRaw=const_cast<DRW_UnsupportedObject&>(DxIfaceStandardDwgCaptureTestAccess::standardRaw(baseline,0xbeu));
    childRaw.m_rawBytes[0]^=1u;CHECK_C(!liveChildren(baseline));childRaw.m_rawBytes[0]^=1u;CHECK_C(liveChildren(baseline));
    baseline.xRecords.push_back(*child);CHECK_C(!liveChildren(baseline));baseline.xRecords.pop_back();CHECK_C(liveChildren(baseline));
    dx_data other;CHECK_C(reader.fileImport(path.string(),&other,false));CHECK_C(liveChildren(other));
    DxIfaceStandardDwgCaptureTestAccess::transplant(other,baseline);CHECK_C(!liveChildren(other));
    for(int stage=1;stage<=3;++stage){dx_iface a;CHECK_C(selectedChildren(a));dx_data failed;
        sealingAdapter=&a;sealingStage=stage;fault::predicate=sealFaultReady;fault::fired=false;
        const bool ok=a.fileImport(path.string(),&failed,false);fault::predicate=nullptr;sealingAdapter=nullptr;
        CHECK_C(ok && fault::fired && !liveChildren(failed));CHECK_C(!dx_iface::hasUnchangedVerifiedXRecordSource(failed,rootHandle));
        dx_data again;CHECK_C(a.fileImport(path.string(),&again,false));CHECK_C(liveChildren(again));}
    Reentry same;dx_data sameData;same.data=&sameData;same.path=path.string();CHECK_C(selectedChildren(same));
    (void)same.fileImport(path.string(),&sameData,false);CHECK_C(same.attempted && !liveChildren(sameData));
    dx_data again;CHECK_C(same.fileImport(path.string(),&again,false));CHECK_C(liveChildren(again));

    Reentry outer;dx_iface inner;dx_data cross;outer.data=&cross;outer.nested=&inner;outer.path=path.string();
    CHECK_C(selectedChildren(outer) && selectedChildren(inner));(void)outer.fileImport(path.string(),&cross,false);
    CHECK_C(outer.attempted && !liveChildren(cross));dx_data crossRetry;
    CHECK_C(outer.fileImport(path.string(),&crossRetry,false));CHECK_C(liveChildren(crossRetry));
    SpoofChild spoof;CHECK_C(selectedChildren(spoof));dx_data spoofed;(void)spoof.fileImport(path.string(),&spoofed,false);
    CHECK_C(spoof.seen && !liveChildren(spoofed));spoof.enabled=false;dx_data spoofRetry;
    CHECK_C(spoof.fileImport(path.string(),&spoofRetry,false));CHECK_C(liveChildren(spoofRetry));
    Content c;Data t;Geometry g;DwgTableRoundtripXRecordAc1018Candidate::Result root;
    std::array<DwgTableChildXRecordAc1018Candidate::Result,9> children;
    fault::count=0;fault::counting=true;const bool counted=dx_iface::readVerifiedTableRoundtripChildren(
        baseline,fullXrecords,contentHandle,dataHandle,geometryHandle,c,t,g,root,children);fault::counting=false;
    const auto allocations=fault::count;CHECK_C(counted && allocations>0u);
    c.handle=t.handle=g.handle=root.handle=9u;children[0].handle=9u;fault::remaining=static_cast<long>(allocations-1u);fault::fired=false;
    const bool oom=dx_iface::readVerifiedTableRoundtripChildren(baseline,fullXrecords,contentHandle,dataHandle,geometryHandle,c,t,g,root,children);
    fault::remaining=-1;CHECK_C(!oom && fault::fired && !c.handle && !t.handle && !g.handle && !root.handle && !children[0].handle);
    CHECK_C(liveChildren(baseline));std::cout<<"child query allocations="<<allocations<<" (last fault sampled)\n";
    const auto output=path.string()+".child.dxf";
    {std::ofstream out(output,std::ios::binary|std::ios::trunc);out<<"CHILD_SENTINEL";CHECK_C(static_cast<bool>(out));}
    CHECK_C(!reader.fileExport(output,DRW::AC1018,false,&baseline,false,true,dx_dxfSourceTextPolicy::Compatible,
        DRW::DxfTextWritePolicy::Compatible,dx_dxfProxyGraphicsPolicy::Compatible,dx_dwgReadCompletenessPolicy::RequireComplete));
    {std::ifstream in(output,std::ios::binary);const std::string value((std::istreambuf_iterator<char>(in)),{});CHECK_C(value=="CHILD_SENTINEL");}
    {std::error_code e;std::filesystem::remove(output,e);}
    const auto original=DxIfaceStandardDwgCaptureTestAccess::standardRaw(baseline,0xbeu);
    for(int mode=0;mode<5;++mode){ChildFixture::Options o;
        if(mode==0)o.role=5u;if(mode==1)o.page=30u;if(mode==2)o.owner=0xbau;
        auto changed=ChildFixture::build(o).raw.m_rawBytes;
        if(mode==3)patchRawBits(changed,81u,95u,2u);
        if(mode==4)patchRawBits(changed,97u,7u,4u);
        CHECK_C(replaceSelectedBody(path,original,changed));
        if(mode==1 || mode==3 || mode==4){dx_data observational;CHECK_C(old.fileImport(path.string(),&observational,false));
            CHECK_C(dx_iface::readVerifiedTableRoundtripCandidates(observational,fullXrecords,contentHandle,dataHandle,geometryHandle,oc,ot,og,oroot));}
        dx_data bad;(void)reader.fileImport(path.string(),&bad,false);
        CHECK_C(!liveChildren(bad));CHECK_C(!dx_iface::hasUnchangedVerifiedXRecordSource(bad,rootHandle));
        auto current=original;current.m_rawBytes=changed;CHECK_C(replaceSelectedBody(path,current,original.m_rawBytes));
        dx_data retry;CHECK_C(reader.fileImport(path.string(),&retry,false));CHECK_C(liveChildren(retry));
    }
    // This is a wire-shape observer, not a fixed semantic census of source cells.
    ChildFixture::Options alternate;alternate.value="Other";const auto alternateBody=ChildFixture::build(alternate).raw.m_rawBytes;
    CHECK_C(replaceSelectedBody(path,original,alternateBody));dx_data alternateData;
    CHECK_C(reader.fileImport(path.string(),&alternateData,false));
    CHECK_C(dx_iface::readVerifiedTableRoundtripChildren(alternateData,fullXrecords,contentHandle,dataHandle,geometryHandle,c,t,g,root,children));
    CHECK_C(children[0].text=="Other");auto alternateRaw=original;alternateRaw.m_rawBytes=alternateBody;
    CHECK_C(replaceSelectedBody(path,alternateRaw,original.m_rawBytes));
    std::vector<std::uint8_t> originalFile;CHECK_C(corruptSelectedCrc(path,original,originalFile));dx_data badCrc;
    (void)reader.fileImport(path.string(),&badCrc,false);CHECK_C(!liveChildren(badCrc));
    CHECK_C(!dx_iface::hasUnchangedVerifiedXRecordSource(badCrc,rootHandle));
    const auto& integrity=badCrc.dwgReadOutcome.observations.integrity;
    CHECK_C(!integrity.extractionOmitted && !integrity.reportedUpstreamDropped && !integrity.aggregateSaturated);
    bool childFailure=false;for(std::size_t i=0;i<integrity.retained;++i){const auto& row=integrity.rows[i];
        CHECK_C(row.kind!=DwgIntegrityCheckKind::DataPageChecksum && row.kind!=DwgIntegrityCheckKind::DataPageCrc);
        if(row.kind==DwgIntegrityCheckKind::ObjectFrameBounds && row.hasLogicalHandle && row.logicalHandle==0xbeu)childFailure=true;}
    CHECK_C(childFailure);
    {std::ofstream out(path,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char*>(originalFile.data()),originalFile.size());CHECK_C(static_cast<bool>(out));}
    dx_data crcRetry;CHECK_C(reader.fileImport(path.string(),&crcRetry,false));CHECK_C(liveChildren(crcRetry));
    std::cout<<"nine-child source binding controls passed\n";
#undef CHECK_C
    return true;
}

}
int main(int argc,char**argv){if(argc!=10)return 2;std::filesystem::path p(argv[5]);if(std::filesystem::exists(p))return 2;const bool ok=run(argv[1],argv[2],argv[3],argv[4],p) && runGeometry(argv[1],argv[2],argv[6],argv[7],p) && runRoundtrip(argv[1],argv[2],argv[6],argv[8],p) && runChildren(argv[1],argv[9],argv[6],argv[8],p);std::error_code e;std::filesystem::remove(p,e);return ok?0:1;}
