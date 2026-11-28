#include "dx_iface.h"
#include "intern/dwgreader15.h"
#include "intern/dwgbufferw.h"
#include <cstdint>
#include <vector>
class LTypeTicketSink final : public dx_iface {
public:
    bool selected{true}; unsigned tickets{0};
    bool wantsVerifiedTableCandidate(std::uint32_t h) const noexcept override {return selected && h==5u;}
    void verifiedTableCandidate(const DwgVerifiedTableCandidateTicket& t) noexcept override {
        if(t.kind()==DwgVerifiedTableCandidateKind::LTypeControl && t.version()==DRW::AC1018
            && t.classNumber()==DRW::DwgLTypeControlObjectType && t.handle()==5u
            && t.offset()==17u && t.ordinal()==3u
            && t.space()==DRW_DwgFrameOffsetSpace::DecodedBuffer
            && t.frame().body().size()==4u) ++tickets;
    }
};
class DwgCodePageTestAccess {
public:
 static bool run(DRW_DwgFrameCoverageStatus coverage,bool failed,bool selected) {
    dwgReader15 reader(nullptr,nullptr);LTypeTicketSink sink; sink.selected=selected;
    dwgBufferW body;body.putRawChar8(1u);body.putRawChar8(2u);body.putRawChar8(3u);body.putRawChar8(4u);
    dwgBufferW framed;framed.putModularShort(4);framed.putBytes(body.data().data(),body.data().size());
    framed.putRawShort16(framed.crc16(0xc0c1u,0u,framed.size()));
    dwgBuffer input(framed.data().data(),framed.data().size(),nullptr);DwgObjectFrame frame;
    if(!frame.readAt(input,DRW::AC1018,0u))return false;
    dwgReader::VerifiedTableCandidatePending p;p.kind=DwgVerifiedTableCandidateKind::LTypeControl;
    p.version=DRW::AC1018;p.classNumber=DRW::DwgLTypeControlObjectType;p.handle=5u;p.offset=17u;
    p.ordinal=3u;p.space=DRW_DwgFrameOffsetSpace::DecodedBuffer;p.frame=std::move(frame);
    reader.m_verifiedTableCandidates.push_back(std::move(p));reader.m_verifiedTableCandidateFailed=failed;
    reader.m_dwgFrameCoverageStatus=coverage;reader.deliverVerifiedTableCandidates(sink);
    const bool should=coverage==DRW_DwgFrameCoverageStatus::FinalizedComplete&&!failed&&selected;
    return sink.tickets==(should?1u:0u);
 }
};
int main(){return DwgCodePageTestAccess::run(DRW_DwgFrameCoverageStatus::FinalizedComplete,false,true)
    &&DwgCodePageTestAccess::run(DRW_DwgFrameCoverageStatus::FinalizedPartial,false,true)
    &&DwgCodePageTestAccess::run(DRW_DwgFrameCoverageStatus::FinalizedComplete,true,true)
    &&DwgCodePageTestAccess::run(DRW_DwgFrameCoverageStatus::FinalizedComplete,false,false)?0:1;}
