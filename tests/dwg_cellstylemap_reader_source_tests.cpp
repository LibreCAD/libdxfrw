// Scratch-generated CRC frame exercises actual object classification and
// typed/raw source dispatch and reader-created ticket eligibility.
#include "dx_iface.h"
#include "drw_classes.h"
#include "intern/dwgreader15.h"
#include "intern/dwg_cellstylemap_ac1018_candidate.h"
#include "intern/dwgbufferw.h"
#include <array>
#include <fstream>
#include <iterator>
#include <optional>
#include <vector>
class CellStyleSourceSink final : public dx_iface {
public:
    void addCellStyleMap(const DRW_CellStyleMap& t) override {if(model)duplicate=true;model=t;}
    void addUnsupportedObject(const DRW_UnsupportedObject& r) override {if(raw)duplicate=true;raw=r;}
    void addDwgFramePublication(const DRW_DwgFramePublication& p) override {if(publication)duplicate=true;publication=p;}
    std::optional<DRW_CellStyleMap> model;
    std::optional<DRW_UnsupportedObject> raw;
    std::optional<DRW_DwgFramePublication> publication;
    bool duplicate=false;
    bool selected=true;int tickets=0;
    bool wantsVerifiedTableCandidate(std::uint32_t h) const noexcept override {return selected && h==0xddu;}
    void verifiedTableCandidate(const DwgVerifiedTableCandidateTicket& t) noexcept override {
        if(t.kind()==DwgVerifiedTableCandidateKind::CellStyleMap && t.handle()==0xddu
            && t.classNumber()==508u && t.offset()==0u && t.ordinal()==7u
            && t.space()==DRW_DwgFrameOffsetSpace::DecodedBuffer)++tickets;
    }
};
class DwgCodePageTestAccess {
public:
    static bool run(dwgReader& reader,CellStyleSourceSink& sink,dwgBuffer& b,int mode) {
        reader.version=DRW::AC1018;reader.decoder.setVersion(reader.version,false);
        if(mode==5)sink.selected=false;
        if(mode==7)reader.m_verifiedXRecordPendingBytes=2u*1024u*1024u;
        if(mode==11 || mode==12) {
            dwgReader::VerifiedTableCandidatePending prior;
            prior.kind=mode==11?DwgVerifiedTableCandidateKind::CellStyleMap:DwgVerifiedTableCandidateKind::TableStyle;
            prior.handle=mode==11?0xddu:0xeeu;prior.ordinal=7u;prior.space=DRW_DwgFrameOffsetSpace::DecodedBuffer;
            reader.m_verifiedTableCandidates.push_back(std::move(prior));
        }
        auto* cls=new DRW_Class;cls->classNum=508;cls->dwgType=508;cls->entityFlag=mode==1?1:0;
        cls->recName=mode==2?"OTHER":"CELLSTYLEMAP";cls->className=mode==3?"Other":"AcDbCellStyleMap";
        reader.classesmap.emplace(508,cls);
        objHandle o(508u,0xddu,0u,7u,DRW_DwgFrameOffsetSpace::DecodedBuffer);
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
        if(mode==11 || mode==12) {
            if(!reader.m_verifiedTableCandidateFailed || !reader.m_verifiedTableCandidates.empty())return false;
            reader.m_dwgFrameCoverageStatus=DRW_DwgFrameCoverageStatus::FinalizedComplete;
            reader.deliverVerifiedTableCandidates(sink);return sink.tickets==0;
        }
        reader.m_dwgFrameCoverageStatus=mode==6?DRW_DwgFrameCoverageStatus::FinalizedPartial:DRW_DwgFrameCoverageStatus::FinalizedComplete;
        if(mode==8)reader.m_dwgFrameCoverageIntegrityViolation=true;
        if(mode==9)reader.m_entityParseFailures=1u;
        if(mode==10)reader.m_objectParseFailures=1u;
        reader.deliverVerifiedTableCandidates(sink);
        if(!read || !sink.model || !sink.raw || !sink.publication || sink.duplicate
            || status.m_disposition!=DRW_DwgFrameDisposition::Published || status.m_publicationCount!=1u)return false;
        DwgCellStyleMapAc1018Candidate::Result result;
        const bool accepted=DwgCellStyleMapAc1018Candidate::inspect(*sink.raw,*sink.model,result);
        // Dispatch recognizes either class spelling, whereas source admission
        // requires both reader-owned spellings and the object-class route.
        if(mode==2 || mode==3)return !accepted && sink.tickets==0;
        if(mode>=5 && mode<=10)return accepted && sink.tickets==0;
        return mode==0 && sink.tickets==1 && accepted && result.handle==0xddu && result.owner==0xdbu
            && result.styles[0].id==3 && result.styles[1].id==1 && result.styles[2].id==2
            && result.styles[0].textStyle.target==0x11u && result.styles[2].linetypes[5].target==0x14u
            && sink.publication->m_hasSourceLocation && sink.publication->m_sourceMapOrdinal==7u
            && sink.publication->m_sourceOffset==0u
            && sink.publication->m_sourceOffsetSpace==DRW_DwgFrameOffsetSpace::DecodedBuffer;
    }
};
bool resourceSelectorGuards() {
    const std::vector<std::uint32_t> xrecords{
        0xbcu,0xbeu,0xbfu,0xc0u,0xc1u,0xc2u,0xc3u,0xc4u,0xc5u,0xc6u};
    const auto select=[&xrecords](dx_iface& adapter,const std::array<std::uint32_t,6>& r) {
        return adapter.enableVerifiedTableCellStyleResourceSource(xrecords,
            0xb9u,0xbdu,0xbau,0x9cu,0xbbu,0x6bu,0xdbu,0xdcu,0xddu,
            r[0],r[1],r[2],r[3],r[4],r[5]);
    };
    const std::array<std::uint32_t,6> valid{{0x3u,0x11u,0x5u,0x16u,0x14u,0x15u}};
    dx_iface adapter;
    if(!select(adapter,valid))return false;
    auto invalid=valid;invalid[1]=invalid[0];
    if(select(adapter,invalid))return false;
    invalid=valid;invalid[4]=0xddu;
    if(select(adapter,invalid))return false;
    invalid=valid;invalid[1]=0xbcu;
    if(select(adapter,invalid))return false;
    invalid=valid;invalid[0]=DRW::NoHandle;
    if(select(adapter,invalid))return false;
    auto conflict=valid;conflict[5]=0x17u;
    if(select(adapter,conflict)||!select(adapter,valid))return false;
    dx_iface duplicate;
    invalid=valid;invalid[4]=invalid[3];
    return !select(duplicate,invalid)&&select(duplicate,valid);
}
std::vector<std::uint8_t> sourceFrame(const std::vector<std::uint8_t>& body) {
    dwgBufferW b;b.putModularShort(static_cast<std::int32_t>(body.size()));b.putBytes(body.data(),body.size());
    b.putRawShort16(b.crc16(0xc0c1u,0u,b.size()));return b.data();
}
int main(int argc,char** argv) {
    if(argc!=2)return 2;if(!resourceSelectorGuards())return 20;std::ifstream in(argv[1],std::ios::binary);
    const std::vector<std::uint8_t> body((std::istreambuf_iterator<char>(in)),{});
    if(!in || body.empty())return 3;
    for(int mode=0;mode<13;++mode){dwgReader15 reader(nullptr,nullptr);CellStyleSourceSink sink;auto bytes=sourceFrame(body);
        if(mode==4)bytes.back()^=1u;dwgBuffer b(bytes.data(),bytes.size(),nullptr);
        if(!DwgCodePageTestAccess::run(reader,sink,b,mode))return 10+mode;}
    return 0;
}
