#include "dx_iface.h"
#include "intern/dwgreader15.h"
#include "intern/dwgbufferw.h"
#include <fstream>
#include <iterator>
#include <optional>
#include <string>
class DictionarySink final:public dx_iface {
public:
    std::optional<DRW_Dictionary> model;std::optional<DRW_UnsupportedObject> raw;
    int tickets=0;bool selectStyle=true,wants=true;DwgVerifiedTableCandidateKind kind{DwgVerifiedTableCandidateKind::TableEntity};
    void addDictionary(const DRW_Dictionary& value) override{if(value.handle==0xdbu)model=value;}
    void addUnsupportedObject(const DRW_UnsupportedObject& value) override{if(value.m_handle==0xdbu)raw=value;}
    std::uint32_t verifiedStyleExtensionDictionarySelection() const noexcept override{return selectStyle?0xdbu:0u;}
    bool wantsVerifiedTableCandidate(std::uint32_t h) const noexcept override{return wants && h==0xdbu;}
    void verifiedTableCandidate(const DwgVerifiedTableCandidateTicket& t) noexcept override {
        if(t.handle()==0xdbu && t.classNumber()==42u && t.ordinal()==7u && t.offset()==0u
            && t.space()==DRW_DwgFrameOffsetSpace::DecodedBuffer){++tickets;kind=t.kind();}
    }
};
class DwgCodePageTestAccess {
public:
 static bool run(dwgReader& reader,DictionarySink& sink,dwgBuffer& b,int mode){
    reader.version=DRW::AC1018;reader.decoder.setVersion(reader.version,false);
    if(mode==1)sink.selectStyle=false;if(mode==2)sink.wants=false;
    if(mode==3)reader.m_verifiedXRecordPendingBytes=2u*1024u*1024u;
    objHandle o(42u,0xdbu,0u,7u,DRW_DwgFrameOffsetSpace::DecodedBuffer);reader.objObjectMap.emplace(o.handle,o);
    DRW_DwgFrameCoverageEntry e;e.m_handle=o.handle;e.m_sourceOffset=o.loc;e.m_sourceMapOrdinal=o.sourceOrdinal;
    e.m_sourceOffsetSpace=o.sourceOffsetSpace;e.m_disposition=DRW_DwgFrameDisposition::Pending;
    reader.m_dwgSourceFrameLedger.push_back(e);reader.m_dwgSourceFrameIndexes.emplace(o.handle,0u);
    reader.m_dwgFrameCoverageStatus=DRW_DwgFrameCoverageStatus::InProgress;
    dwgReader::DwgFrameClassification c;if(!reader.classifyDwgSourceFrame(&b,o,c))return mode==4;
    reader.m_deferredFrameClassifications.emplace(o.handle,c);
    if(mode==7){dwgReader::VerifiedTableCandidatePending pending;pending.kind=DwgVerifiedTableCandidateKind::TableEntity;
        pending.ordinal=o.sourceOrdinal;pending.offset=o.loc;pending.space=o.sourceOffsetSpace;reader.m_verifiedTableCandidates.push_back(std::move(pending));}
    const bool read=reader.readDwgObjects(sink,&b,DwgIntegrityAddressSpace::DecodedBuffer);
    reader.m_dwgFrameCoverageStatus=mode==5?DRW_DwgFrameCoverageStatus::FinalizedPartial:DRW_DwgFrameCoverageStatus::FinalizedComplete;
    if(mode==6)reader.m_entityParseFailures=1u;reader.deliverVerifiedTableCandidates(sink);
    if(mode==8)return read && sink.tickets==0 && !sink.model;
    if(!read || !sink.model || !sink.raw)return false;
    DwgTableStyleExtensionDictionaryAc1018Candidate::Result r;
    const bool observed=DwgTableStyleExtensionDictionaryAc1018Candidate::inspect(*sink.raw,*sink.model,r);
    if(mode==10)return !observed && sink.tickets==1;
    if(mode==9)return observed && sink.tickets==1 && r.roundtrip==0xddu && r.cellStyleMap==0xdcu
        && !DwgTableStyleExtensionDictionaryAc1018Candidate::matchesSelected(r,0x6bu,0xdcu,0xddu);
    if(!observed)return false;
    if(mode==0)return sink.tickets==1 && sink.kind==DwgVerifiedTableCandidateKind::StyleExtensionDictionary && r.owner==0x6bu && r.roundtrip==0xdcu && r.cellStyleMap==0xddu;
    if(mode==1)return sink.tickets==1 && sink.kind==DwgVerifiedTableCandidateKind::ExtensionDictionary;
    return (mode==2 || mode==3 || mode==5 || mode==6 || mode==7) && sink.tickets==0;
 }
};
std::vector<std::uint8_t> frame(const std::vector<std::uint8_t>& body){dwgBufferW b;b.putModularShort(static_cast<std::int32_t>(body.size()));b.putBytes(body.data(),body.size());b.putRawShort16(b.crc16(0xc0c1u,0u,b.size()));return b.data();}
int main(int argc,char**argv){if(argc!=5)return 2;std::ifstream f(argv[1],std::ios::binary),bad(argv[2],std::ios::binary),swapped(argv[3],std::ios::binary),reversed(argv[4],std::ios::binary);
 const std::vector<std::uint8_t> body((std::istreambuf_iterator<char>(f)),{}),badBody((std::istreambuf_iterator<char>(bad)),{}),swapBody((std::istreambuf_iterator<char>(swapped)),{}),reverseBody((std::istreambuf_iterator<char>(reversed)),{});
 if(!f||!bad||!swapped||!reversed||body.empty()||badBody.empty()||swapBody.empty()||reverseBody.empty())return 3;
 for(int mode=0;mode<=10;++mode){dwgReader15 reader(nullptr,nullptr);DictionarySink sink;auto bytes=frame(mode==8?badBody:mode==9?swapBody:mode==10?reverseBody:body);if(mode==4)bytes.back()^=1u;dwgBuffer b(bytes.data(),bytes.size(),nullptr);if(!DwgCodePageTestAccess::run(reader,sink,b,mode))return 10+mode;}
 return 0;}
