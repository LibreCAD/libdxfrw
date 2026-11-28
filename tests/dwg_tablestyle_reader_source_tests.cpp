// Scratch-generated CRC frame exercises actual object classification and
// typed/raw source dispatch; this does not construct a reader-private ticket.
#include "dx_iface.h"
#include "drw_classes.h"
#include "intern/dwgreader15.h"
#include "intern/dwg_tablestyle_ac1018_candidate.h"
#include "intern/dwgbufferw.h"
#include <fstream>
#include <iterator>
#include <optional>
#include <vector>
class StyleSourceSink final : public dx_iface {
public:
    void addTableStyle(const DRW_TableStyle& t) override {if(model)duplicate=true;model=t;}
    void addUnsupportedObject(const DRW_UnsupportedObject& r) override {if(raw)duplicate=true;raw=r;}
    void addDwgFramePublication(const DRW_DwgFramePublication& p) override {if(publication)duplicate=true;publication=p;}
    std::optional<DRW_TableStyle> model;
    std::optional<DRW_UnsupportedObject> raw;
    std::optional<DRW_DwgFramePublication> publication;
    bool duplicate=false;
    bool selected=true;int tickets=0;
    bool wantsVerifiedTableCandidate(std::uint32_t h) const noexcept override {return selected && h==0x6bu;}
    void verifiedTableCandidate(const DwgVerifiedTableCandidateTicket& t) noexcept override {
        if(t.kind()==DwgVerifiedTableCandidateKind::TableStyle && t.handle()==0x6bu
            && t.classNumber()==505u && t.offset()==0u && t.ordinal()==7u
            && t.space()==DRW_DwgFrameOffsetSpace::DecodedBuffer)++tickets;
    }
};
class DwgCodePageTestAccess {
public:
    static bool run(dwgReader& reader,StyleSourceSink& sink,dwgBuffer& b,int mode) {
        reader.version=DRW::AC1018;reader.decoder.setVersion(reader.version,false);
        if(mode==5)sink.selected=false;
        if(mode==7)reader.m_verifiedXRecordPendingBytes=2u*1024u*1024u;
        auto* cls=new DRW_Class;cls->classNum=505;cls->dwgType=505;cls->entityFlag=mode==1?1:0;
        cls->recName=mode==2?"OTHER":"TABLESTYLE";cls->className=mode==3?"Other":"AcDbTableStyle";
        reader.classesmap.emplace(505,cls);
        objHandle o(505u,0x6bu,0u,7u,DRW_DwgFrameOffsetSpace::DecodedBuffer);
        reader.objObjectMap.emplace(o.handle,o);
        DRW_DwgFrameCoverageEntry e;e.m_handle=o.handle;e.m_sourceOffset=o.loc;e.m_sourceMapOrdinal=o.sourceOrdinal;
        e.m_sourceOffsetSpace=o.sourceOffsetSpace;e.m_disposition=DRW_DwgFrameDisposition::Pending;
        reader.m_dwgSourceFrameLedger.push_back(e);reader.m_dwgSourceFrameIndexes.emplace(o.handle,0u);
        reader.m_dwgFrameCoverageStatus=DRW_DwgFrameCoverageStatus::InProgress;
        dwgReader::DwgFrameClassification c;
        if(!reader.classifyDwgSourceFrame(&b,o,c))return mode==4;
        reader.m_deferredFrameClassifications.emplace(o.handle,c);
        const bool read=reader.readDwgObjects(sink,&b,DwgIntegrityAddressSpace::DecodedBuffer);
        const auto& status=reader.m_dwgSourceFrameLedger[0];
        if(mode==1)return !read && !sink.model && !sink.raw && !sink.publication;
        reader.m_dwgFrameCoverageStatus=mode==6?DRW_DwgFrameCoverageStatus::FinalizedPartial:DRW_DwgFrameCoverageStatus::FinalizedComplete;
        reader.deliverVerifiedTableCandidates(sink);
        if(!read || !sink.model || !sink.raw || !sink.publication || sink.duplicate
            || status.m_disposition!=DRW_DwgFrameDisposition::Published || status.m_publicationCount!=1u)return false;
        DwgTableStyleAc1018Candidate::Result result;
        const bool accepted=DwgTableStyleAc1018Candidate::inspect(*sink.raw,*sink.model,result);
        // Dispatch recognizes either class spelling, whereas source admission
        // requires both reader-owned spellings and the object-class route.
        if(mode==2 || mode==3)return !accepted && sink.tickets==0;
        if(mode==5 || mode==6 || mode==7)return accepted && sink.tickets==0;
        return mode==0 && sink.tickets==1 && accepted && result.handle==0x6bu && result.owner==0x6au
            && result.tableReactor==0x9cu && result.xdictionary==0xdbu
            && result.textStyles==std::array<std::uint32_t,3>{{0x11u,0x11u,0x11u}}
            && sink.publication->m_hasSourceLocation && sink.publication->m_sourceMapOrdinal==7u
            && sink.publication->m_sourceOffset==0u
            && sink.publication->m_sourceOffsetSpace==DRW_DwgFrameOffsetSpace::DecodedBuffer;
    }
};
std::vector<std::uint8_t> sourceFrame(const std::vector<std::uint8_t>& body) {
    dwgBufferW b;b.putModularShort(static_cast<std::int32_t>(body.size()));b.putBytes(body.data(),body.size());
    b.putRawShort16(b.crc16(0xc0c1u,0u,b.size()));return b.data();
}
int main(int argc,char** argv) {
    if(argc!=2)return 2;std::ifstream in(argv[1],std::ios::binary);
    const std::vector<std::uint8_t> body((std::istreambuf_iterator<char>(in)),{});
    if(!in || body.empty())return 3;
    for(int mode=0;mode<8;++mode){dwgReader15 reader(nullptr,nullptr);StyleSourceSink sink;auto bytes=sourceFrame(body);
        if(mode==4)bytes.back()^=1u;dwgBuffer b(bytes.data(),bytes.size(),nullptr);
        if(!DwgCodePageTestAccess::run(reader,sink,b,mode))return 10+mode;}
    return 0;
}
