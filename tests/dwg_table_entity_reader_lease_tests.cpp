#include "dx_iface.h"
#include "drw_classes.h"
#include "intern/dwgreader15.h"
#include "intern/dwgbufferw.h"
#include <fstream>
#include <iterator>
#include <string>
#include <cstdlib>
#include <new>
namespace fault {long remaining=-1;bool fired=false;}
void* operator new(std::size_t n){if(fault::remaining==0){fault::remaining=-1;fault::fired=true;throw std::bad_alloc();}
    if(fault::remaining>0)--fault::remaining;if(void* p=std::malloc(n?n:1u))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
class Sink final:public dx_iface {
public:
    bool enabled{true};int tickets{0};std::uint64_t ordinal{0u};std::uint32_t offset{0u};
    std::uint32_t verifiedTableEntitySelection() const noexcept override{return enabled?0x9cu:0u;}
    bool wantsVerifiedTableCandidate(std::uint32_t h) const noexcept override{return enabled && h==0x9cu;}
    void verifiedTableCandidate(const DwgVerifiedTableCandidateTicket& ticket) noexcept override {
        if(ticket.kind()==DwgVerifiedTableCandidateKind::TableEntity && ticket.handle()==0x9cu
            && ticket.classNumber()==513u && ticket.space()==DRW_DwgFrameOffsetSpace::DecodedBuffer){++tickets;ordinal=ticket.ordinal();offset=ticket.offset();}
    }
};
class DwgCodePageTestAccess {
public:
    static void setup(dwgReader& r,int disposition,int count,int space,int identity,int classMode) {
        r.version=DRW::AC1018;DRW_DwgFrameCoverageEntry e;e.m_handle=identity?0x9du:0x9cu;e.m_sourceOffset=0u;e.m_sourceMapOrdinal=7u;
        e.m_sourceOffsetSpace=space?DRW_DwgFrameOffsetSpace::PhysicalFile:DRW_DwgFrameOffsetSpace::DecodedBuffer;
        e.m_disposition=disposition?DRW_DwgFrameDisposition::Staged:DRW_DwgFrameDisposition::Published;e.m_publicationCount=static_cast<std::uint8_t>(count);
        r.m_dwgSourceFrameLedger.push_back(e);r.m_dwgSourceFrameIndexes.emplace(0x9cu,0u);
        auto* cls=new DRW_Class;cls->classNum=513u;cls->entityFlag=classMode==1?0:1;
        cls->recName=classMode==2?"OTHER":"ACAD_TABLE";cls->className=classMode==4?"OtherClass":"AcDbTable";cls->dwgType=classMode==3?514:513;
        r.classesmap.emplace(513,cls);
    }
    static bool stage(dwgReader& r,dx_iface& s,dwgBuffer& b){return r.readDwgObjects(s,&b,DwgIntegrityAddressSpace::DecodedBuffer);}
    static std::size_t pending(const dwgReader& r){return r.m_verifiedTableCandidates.size();}
    static bool failed(const dwgReader& r){return r.m_verifiedTableCandidateFailed;}
    static void fillBudget(dwgReader& r){r.m_verifiedXRecordPendingBytes=2u*1024u*1024u;}
    static void removeSelected(dwgReader& r){r.m_dwgSourceFrameIndexes.clear();}
    static void removeClass(dwgReader& r){delete r.classesmap.at(513);r.classesmap.clear();}
    static void deliver(dwgReader& r,dx_iface& s,int coverage,int entityFailures,int objectFailures,bool violation){
        r.m_dwgFrameCoverageStatus=coverage?DRW_DwgFrameCoverageStatus::FinalizedPartial:DRW_DwgFrameCoverageStatus::FinalizedComplete;
        r.m_entityParseFailures=entityFailures;r.m_objectParseFailures=objectFailures;r.m_dwgFrameCoverageIntegrityViolation=violation;
        r.deliverVerifiedTableCandidates(s);
    }
};
std::vector<std::uint8_t> frame(const std::vector<std::uint8_t>& body) {
    dwgBufferW b;b.putModularShort(static_cast<std::int32_t>(body.size()));b.putBytes(body.data(),body.size());
    const auto crc=b.crc16(0xc0c1u,0u,b.size());b.putRawShort16(crc);return b.data();
}
bool run(const std::vector<std::uint8_t>& bytes,int mode){
    dwgReader15 reader(nullptr,nullptr);Sink sink;
    DwgCodePageTestAccess::setup(reader,mode==1,mode==2?2:1,mode==3,mode==4,mode>=5&&mode<=7?mode-4:mode==18?4:0);
    auto altered=bytes;if(mode==8)altered.back()^=1u;if(mode==9)sink.enabled=false;
    if(mode==14)altered=frame(std::vector<std::uint8_t>(65537u,0u));
    if(mode==15)DwgCodePageTestAccess::fillBudget(reader);
    if(mode==16)DwgCodePageTestAccess::removeSelected(reader);
    if(mode==19)DwgCodePageTestAccess::removeClass(reader);
    dwgBuffer buffer(altered.data(),altered.size(),nullptr);
    if(mode==20){fault::remaining=0;fault::fired=false;}
    const bool stageOk=DwgCodePageTestAccess::stage(reader,sink,buffer);fault::remaining=-1;
    if(!stageOk || (mode==20 && !fault::fired))return false;
    if(mode==17) {
        if(DwgCodePageTestAccess::pending(reader)!=1u || !DwgCodePageTestAccess::stage(reader,sink,buffer)
            || !DwgCodePageTestAccess::failed(reader) || DwgCodePageTestAccess::pending(reader)!=0u)return false;
        DwgCodePageTestAccess::deliver(reader,sink,0,0,0,false);return sink.tickets==0;
    }
    const bool staged=mode==0 || (mode>=10 && mode<=13);
    if(DwgCodePageTestAccess::pending(reader)!=(staged?1u:0u))return false;
    if(((mode>=1 && mode<=8) || (mode>=14 && mode<=16) || mode==18 || mode==19 || mode==20) && !DwgCodePageTestAccess::failed(reader))return false;
    DwgCodePageTestAccess::deliver(reader,sink,mode==10,mode==11,mode==12,mode==13);
    return sink.tickets==(mode==0?1:0) && (mode!=0 || (sink.ordinal==7u && sink.offset==0u));
}
int main(int argc,char**argv){if(argc!=2)return 2;std::ifstream in(argv[1],std::ios::binary);
    std::vector<std::uint8_t> body((std::istreambuf_iterator<char>(in)),{});if(!in||body.empty())return 3;
    const auto bytes=frame(body);for(int mode=0;mode<=20;++mode)if(!run(bytes,mode))return 10+mode;return 0;}
