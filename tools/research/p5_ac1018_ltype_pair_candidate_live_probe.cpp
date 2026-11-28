// Optional caller-supplied raw bodies and a live typed callback. This is not a
// reader-private source/CRC ticket and does not prove graph or output closure.
#include "dx_iface.h"
#include "libdwgr.h"
#include "libdxfrw.h"
#include "intern/dwgobjectframe.h"
#include "intern/dwgreader.h"
#include "intern/dwgutil.h"
#include "intern/dwg_fixed_handles.h"
#include "intern/dwg_ltype_control_byblock_ac1018_candidate.h"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <memory>
#include <optional>
#include <algorithm>
#include <list>
#include <cstdlib>
#include <cstdint>
#include <vector>
#include <array>
#include <cctype>

class GeneratedSource final:public dx_iface {
public:
 explicit GeneratedSource(dwgRW& writer,const std::list<DRW_LType>* sourceRows=nullptr)
     :writer_(writer),sourceRows_(sourceRows){}
 void writeHeader(DRW_Header& header) override {header.vars.clear();}
 void collectDwgAppIds() override {}
 void writeBlocks() override {} void writeBlockRecords() override {} void writeEntities() override {}
 void writeLTypes() override {
  if(sourceRows_!=nullptr){
   const auto findRow=[this](const char* name)->std::optional<DRW_LType>{
    const auto found=std::find_if(sourceRows_->cbegin(),sourceRows_->cend(),
        [name](const DRW_LType& row){return row.name==name;});
    return found==sourceRows_->cend()?std::nullopt:std::optional<DRW_LType>(*found);
   };
   auto byblock=findRow("ByBlock");if(!byblock)byblock=findRow("BYBLOCK");
   auto bylayer=findRow("ByLayer");if(!bylayer)bylayer=findRow("BYLAYER");
   auto continuous=findRow("Continuous");if(!continuous)continuous=findRow("CONTINUOUS");
   if(!byblock||!bylayer||!continuous)return;
   generated=writer_.addLType(&*byblock)&&writer_.addLType(&*bylayer)
       &&writer_.addLType(&*continuous);
   byBlockHandle=byblock->handle;byLayerHandle=bylayer->handle;
   continuousHandle=continuous->handle;return;
  }
  DRW_LType byblock;byblock.handle=0x14u;byblock.name="ByBlock";byblock.flags=0x40;byblock.length=0.;byblock.alignment=65;
  DRW_LType bylayer;bylayer.handle=0x15u;bylayer.name="ByLayer";bylayer.flags=0x40;bylayer.length=0.;bylayer.alignment=65;
  DRW_LType continuous;continuous.handle=0x16u;continuous.name="Continuous";continuous.desc="Solid line";continuous.flags=0;continuous.length=0.;continuous.alignment=65;
  generated=writer_.addLType(&byblock)&&writer_.addLType(&bylayer)&&writer_.addLType(&continuous);
  byBlockHandle=byblock.handle;byLayerHandle=bylayer.handle;continuousHandle=continuous.handle;
 }
 void writeLayers() override {} void writeTextstyles() override {} void writeVports() override {}
 void writeViews() override {} void writeUCSs() override {} void writeDimstyles() override {} void writeAppId() override {}
 bool generated{false};
 std::uint32_t byBlockHandle{0u},byLayerHandle{0u},continuousHandle{0u};
private:dwgRW& writer_;const std::list<DRW_LType>* sourceRows_{nullptr};
};
class Probe final:public dx_iface {
public:
 void reset(){model.reset();duplicate=false;tickets=0;ticketValid=false;rowTickets=0;rowTicketValid=false;controlFrame.reset();byBlockFrame.reset();byBlockFrameModel.reset();}
 void addLType(const DRW_LType& v) override {dx_iface::addLType(v);if(v.name=="BYBLOCK"||v.name=="ByBlock"){if(model)duplicate=true;model=v;}}
 std::optional<DRW_LType> model;bool duplicate=false;unsigned tickets=0;bool ticketValid=false;unsigned rowTickets=0;bool rowTicketValid=false;
 std::optional<DRW_UnsupportedObject> controlFrame,byBlockFrame;std::optional<DRW_LType> byBlockFrameModel;
 bool wantsVerifiedTableCandidate(std::uint32_t h) const noexcept override {return h==5u||h==15u||h==16u||h==17u||h==0x14u||h==0x15u||h==0x16u;}
 void verifiedTableCandidate(const DwgVerifiedTableCandidateTicket& t) noexcept override {
  if(t.kind()==DwgVerifiedTableCandidateKind::LTypeControl&&t.version()==DRW::AC1018
      &&t.handle()==5u&&t.frame().body().size()<=4096u){++tickets;ticketValid=true;
    DRW_UnsupportedObject raw;raw.m_version=t.version();raw.m_handle=t.handle();raw.m_objectType=56;
    raw.m_objectOffset=t.offset();raw.m_objectSize=static_cast<std::uint32_t>(t.frame().body().size());
    raw.m_bodyBitSize=0u;raw.m_rawBytes=t.frame().body();controlFrame=std::move(raw);
  }
  if(t.kind()==DwgVerifiedTableCandidateKind::LTypeRecord&&t.version()==DRW::AC1018
      &&(t.handle()==15u||t.handle()==16u||t.handle()==17u||t.handle()==0x14u||t.handle()==0x15u||t.handle()==0x16u)
      &&t.frame().body().size()<=4096u&&t.ltypeModel()){
    ++rowTickets;
    const std::string expected=(t.handle()==15u||t.handle()==0x14u)?"BYBLOCK":(t.handle()==16u||t.handle()==0x15u)?"BYLAYER":"CONTINUOUS";
    std::string actual=t.ltypeModel()->name;
    std::transform(actual.begin(),actual.end(),actual.begin(),[](unsigned char c){return static_cast<char>(std::toupper(c));});
    if(t.ltypeModel()->handle==t.handle()&&actual==expected)rowTicketValid=true;
    if(t.handle()==15u||t.handle()==0x14u){byBlockFrameModel=*t.ltypeModel();DRW_UnsupportedObject raw;raw.m_version=t.version();raw.m_handle=t.handle();raw.m_objectType=57;
      raw.m_objectOffset=t.offset();
      raw.m_objectSize=static_cast<std::uint32_t>(t.frame().body().size());raw.m_bodyBitSize=0u;raw.m_rawBytes=t.frame().body();byBlockFrame=std::move(raw);}
  }
 }
};
class DxfLtypeSource final:public dx_iface {
public:
 void writeHeader(DRW_Header& header) override {header.vars.clear();}
 void collectDwgAppIds() override {}
 void writeBlocks() override {}
 void writeBlockRecords() override {}
 void writeEntities() override {}
 void writeLayers() override {}
 void writeTextstyles() override {}
 void writeVports() override {}
 void writeViews() override {}
 void writeUCSs() override {}
 void writeDimstyles() override {}
 void writeAppId() override {}
 bool finalizeDxfWrite() override {return true;}
 // dxfRW emits its three mandatory linetypes before this callback. The
 // selected source profile contains only those rows, so replaying them here
 // would duplicate TABLE entries.
 void writeLTypes() override {}
 bool complete{true};
};
std::uint32_t get32(const std::uint8_t* p){return static_cast<std::uint32_t>(p[0])|static_cast<std::uint32_t>(p[1])<<8u
 |static_cast<std::uint32_t>(p[2])<<16u|static_cast<std::uint32_t>(p[3])<<24u;}
void put32(std::uint8_t* p,std::uint32_t n){for(unsigned i=0;i<4u;++i)p[i]=static_cast<std::uint8_t>(n>>(i*8u));}
bool corruptObjectFrameCrc(const std::filesystem::path& path,const DRW_UnsupportedObject& source,
                           std::vector<std::uint8_t>& original){
 std::ifstream input(path,std::ios::binary);std::vector<char> chars((std::istreambuf_iterator<char>(input)),{});if(!input||source.m_rawBytes.empty())return false;
 original.reserve(chars.size());for(const auto c:chars)original.push_back(static_cast<std::uint8_t>(c));
 const auto hit=std::search(original.cbegin(),original.cend(),source.m_rawBytes.cbegin(),source.m_rawBytes.cend());
 if(hit==original.cend()||std::search(std::next(hit),original.cend(),source.m_rawBytes.cbegin(),source.m_rawBytes.cend())!=original.cend())return false;
 const auto bodyAt=static_cast<std::size_t>(hit-original.cbegin());dwgBuffer reader(original.data(),original.size(),nullptr);DwgObjectFrame frame;
 std::size_t frameAt=0u;bool foundFrame=false;for(const auto prefix:{2u,4u})if(bodyAt>=prefix
   &&frame.readAt(reader,DRW::AC1018,bodyAt-prefix)&&frame.body()==source.m_rawBytes
   &&reader.getPosition()==bodyAt+source.m_rawBytes.size()+2u){frameAt=bodyAt-prefix;foundFrame=true;break;}
 if(!foundFrame)return false;
 std::size_t pageAt=0u;std::vector<std::uint8_t> header(32u);
 for(std::size_t p=0x100u;p+32u<=original.size();p+=0x20u){std::copy_n(original.data()+p,32u,header.data());dwgCompressor::decrypt18Hdr(header.data(),32u,p);
  const auto size=get32(header.data()+8u);if(get32(header.data())!=0x4163043bu||size!=get32(header.data()+12u)
    ||size>original.size()-(p+32u)||bodyAt<p+32u||bodyAt+source.m_rawBytes.size()+2u>p+32u+size)continue;
  const auto dataCrc=dwgUtil::checksum18(0u,original.data()+p+32u,size);const auto headerCrc=get32(header.data()+24u);put32(header.data()+24u,0u);
  if(dataCrc!=get32(header.data()+28u)||dwgUtil::checksum18(dataCrc,header.data(),32u)!=headerCrc)continue;
  if(pageAt!=0u)return false;pageAt=p;}
 if(!pageAt)return false;auto damaged=original;damaged[bodyAt+source.m_rawBytes.size()]^=1u;
 std::copy_n(damaged.data()+pageAt,32u,header.data());dwgCompressor::decrypt18Hdr(header.data(),32u,pageAt);
 const auto size=get32(header.data()+8u);const auto dataCrc=dwgUtil::checksum18(0u,damaged.data()+pageAt+32u,size);
 put32(header.data()+28u,dataCrc);put32(header.data()+24u,0u);put32(header.data()+24u,dwgUtil::checksum18(dataCrc,header.data(),32u));
 dwgCompressor::decrypt18Hdr(header.data(),32u,pageAt);std::copy(header.begin(),header.end(),damaged.begin()+static_cast<std::ptrdiff_t>(pageAt));
 dwgBuffer verify(damaged.data(),damaged.size(),nullptr);if(frame.readAt(verify,DRW::AC1018,frameAt))return false;
 std::ofstream output(path,std::ios::binary|std::ios::trunc);output.write(reinterpret_cast<const char*>(damaged.data()),static_cast<std::streamsize>(damaged.size()));return static_cast<bool>(output);
}
// Map decoded bytes back to their source literals. The actual page decode is
// independently checked with dwgCompressor before any source mutation.
struct R2004DecodeTrace {
 std::vector<std::uint8_t> output;
 std::vector<std::int32_t> compressedByteForOutput;
 std::size_t outputBytes{0};
};
bool traceR2004Lz(const std::uint8_t* input,std::size_t inputSize,
                  std::size_t outputSize,R2004DecodeTrace& trace){
 if(!input||inputSize<2u||outputSize==0u)return false;
 trace={};trace.output.assign(outputSize,0u);
 trace.compressedByteForOutput.assign(outputSize,-1);
 std::size_t in=0u,out=0u;
 const auto readByte=[&](std::uint8_t& value){if(in>=inputSize)return false;value=input[in++];return true;};
 const auto literalLength=[&](std::uint64_t& length){
  std::uint8_t value=0u;if(!readByte(value))return false;
  if(value>0x0fu){--in;length=0u;return true;}
  std::uint64_t total=0u;
  if(value==0u){total=0x0fu;if(!readByte(value))return false;
   while(value==0u){total+=0xffu;if(!readByte(value))return false;}}
  length=total+value+3u;return true;
 };
 const auto longOffset=[&](std::uint64_t& value){
  value=0u;std::uint8_t b=0u;if(!readByte(b))return false;
  while(b==0u){value+=0xffu;if(!readByte(b))return false;}
  value+=b;return true;
 };
 const auto twoOffset=[&](std::uint32_t& distance,std::uint64_t& literals){
  std::uint8_t first=0u,second=0u;if(!readByte(first)||!readByte(second))return false;
  distance=(first>>2u)|(static_cast<std::uint32_t>(second)<<6u);
  literals=first&3u;return true;
 };
 std::uint64_t literals=0u;if(!literalLength(literals))return false;
 for(std::uint64_t i=0u;i<literals&&out<outputSize;++i){
  const std::size_t source=in;std::uint8_t value=0u;if(!readByte(value))return false;
  trace.output[out]=value;trace.compressedByteForOutput[out]=static_cast<std::int32_t>(source);++out;
 }
 while(out<outputSize&&in<inputSize){
  std::uint8_t opcode=0u;if(!readByte(opcode))return false;
  if(opcode==0x11u)break;
  std::uint64_t copied=0u;std::uint32_t distance=0u;
  if(opcode==0x10u){
   if(!longOffset(copied))return false;copied+=9u;
   if(!twoOffset(distance,literals))return false;distance+=0x3fffu;
   if(literals==0u&&!literalLength(literals))return false;
  }else if(opcode>=0x12u&&opcode<0x20u){
   const std::uint32_t low=opcode&7u;
   if(low==0u){if(!longOffset(copied))return false;copied+=9u;}else copied=low+2u;
   if(!twoOffset(distance,literals))return false;
   distance+=0x3fffu+static_cast<std::uint32_t>((opcode&8u)<<11u);
   if(literals==0u&&!literalLength(literals))return false;
  }else if(opcode==0x20u){
   if(!longOffset(copied)||!twoOffset(distance,literals))return false;copied+=0x21u;
   if(literals==0u&&!literalLength(literals))return false;
  }else if(opcode>0x20u&&opcode<0x40u){
   copied=opcode-0x1eu;if(!twoOffset(distance,literals))return false;
   if(literals==0u&&!literalLength(literals))return false;
  }else if(opcode>0x3fu){
   copied=((opcode&0xf0u)>>4u)-1u;std::uint8_t second=0u;if(!readByte(second))return false;
   distance=(static_cast<std::uint32_t>(second)<<2u)|((opcode&0x0cu)>>2u);
   literals=opcode&3u;if(literals==0u&&!literalLength(literals))return false;
  }else return false;
  if(distance>=out)return false;
  for(std::uint64_t i=0u;i<copied&&out<outputSize;++i){
   const std::size_t source=out-static_cast<std::size_t>(distance)-1u;
   trace.output[out]=trace.output[source];
   trace.compressedByteForOutput[out]=trace.compressedByteForOutput[source];++out;
  }
  for(std::uint64_t i=0u;i<literals&&out<outputSize;++i){
   const std::size_t source=in;std::uint8_t value=0u;if(!readByte(value))return false;
   trace.output[out]=value;trace.compressedByteForOutput[out]=static_cast<std::int32_t>(source);++out;
  }
 }
 trace.outputBytes=out;return out==outputSize;
}
bool corruptCompressedObjectFrameCrc(const std::filesystem::path& path,
    const DRW_UnsupportedObject& source,std::vector<std::uint8_t>& original,
    std::size_t& changedPhysicalOffset){
 std::ifstream input(path,std::ios::binary);std::vector<char> chars((std::istreambuf_iterator<char>(input)),{});
 if(!input||source.m_rawBytes.empty()||source.m_objectOffset==0u)return false;
 original.reserve(chars.size());for(const auto c:chars)original.push_back(static_cast<std::uint8_t>(c));
 for(std::size_t pageAt=0x100u;pageAt+32u<=original.size();pageAt+=0x20u){
  std::array<std::uint8_t,32u> header{};std::copy_n(original.data()+pageAt,header.size(),header.data());
  dwgCompressor::decrypt18Hdr(header.data(),header.size(),pageAt);
  const std::uint32_t pageType=get32(header.data()),section=get32(header.data()+4u);
  const std::uint32_t compressedSize=get32(header.data()+8u),pageBytes=get32(header.data()+12u);
  const std::uint32_t startOffset=get32(header.data()+16u);
  if(pageType!=0x4163043bu||section!=7u||compressedSize==0u
      ||compressedSize>=pageBytes||pageBytes>0x7400u
      ||compressedSize>original.size()-pageAt-32u
      ||source.m_objectOffset<startOffset
      ||source.m_objectOffset-startOffset>=0x7400u)continue;
  const std::uint32_t dataChecksum=dwgUtil::checksum18(0u,original.data()+pageAt+32u,compressedSize);
  const std::uint32_t storedHeaderChecksum=get32(header.data()+24u);
  if(dataChecksum!=get32(header.data()+28u))continue;
  put32(header.data()+24u,0u);
  if(dwgUtil::checksum18(dataChecksum,header.data(),header.size())!=storedHeaderChecksum)continue;
  R2004DecodeTrace trace;dwgCompressor decoder;
  std::vector<std::uint8_t> decoded(0x7400u,0u);
  if(!decoder.decompress18(original.data()+pageAt+32u,decoded.data(),compressedSize,decoded.size())
      ||!traceR2004Lz(original.data()+pageAt+32u,compressedSize,decoded.size(),trace)
      ||trace.output!=decoded)return false;
  const std::uint64_t localOffset=source.m_objectOffset-startOffset;
  if(localOffset>=decoded.size())return false;
  dwgBuffer frameBuffer(decoded.data(),decoded.size(),nullptr);DwgObjectFrame frame;
  if(!frame.readAt(frameBuffer,DRW::AC1018,localOffset)||frame.body()!=source.m_rawBytes
      ||frameBuffer.getPosition()<2u)return false;
  const std::size_t crcOffset=static_cast<std::size_t>(frameBuffer.getPosition()-2u);
  const std::int32_t compressedByte=trace.compressedByteForOutput[crcOffset];
  if(compressedByte<0)return false;
  const auto occurrences=std::count(trace.compressedByteForOutput.begin(),
      trace.compressedByteForOutput.end(),compressedByte);
  if(occurrences!=1)return false;
  std::vector<std::uint8_t> damaged=original;
  const std::size_t compressedAt=pageAt+32u+static_cast<std::size_t>(compressedByte);
  damaged[compressedAt]^=1u;
  R2004DecodeTrace damagedTrace;std::vector<std::uint8_t> damagedDecoded(0x7400u,0u);
  dwgCompressor damagedDecoder;
  if(!damagedDecoder.decompress18(damaged.data()+pageAt+32u,damagedDecoded.data(),compressedSize,damagedDecoded.size())
      ||!traceR2004Lz(damaged.data()+pageAt+32u,compressedSize,damagedDecoded.size(),damagedTrace)
      ||damagedTrace.output!=damagedDecoded)return false;
  for(std::size_t i=0u;i<decoded.size();++i)
    if((decoded[i]!=damagedDecoded[i])!=(i==crcOffset))return false;
  dwgBuffer damagedFrameBuffer(damagedDecoded.data(),damagedDecoded.size(),nullptr);
  DwgObjectFrame damagedFrame;
  if(damagedFrame.readAt(damagedFrameBuffer,DRW::AC1018,localOffset))return false;
  std::array<std::uint8_t,32u> changedHeader{};
  std::copy_n(damaged.data()+pageAt,changedHeader.size(),changedHeader.data());
  dwgCompressor::decrypt18Hdr(changedHeader.data(),changedHeader.size(),pageAt);
  const std::uint32_t changedDataChecksum=dwgUtil::checksum18(0u,
      damaged.data()+pageAt+32u,compressedSize);
  put32(changedHeader.data()+28u,changedDataChecksum);put32(changedHeader.data()+24u,0u);
  put32(changedHeader.data()+24u,dwgUtil::checksum18(changedDataChecksum,
      changedHeader.data(),changedHeader.size()));
  dwgCompressor::decrypt18Hdr(changedHeader.data(),changedHeader.size(),pageAt);
  std::copy(changedHeader.begin(),changedHeader.end(),damaged.begin()+static_cast<std::ptrdiff_t>(pageAt));
  std::ofstream output(path,std::ios::binary|std::ios::trunc);
  output.write(reinterpret_cast<const char*>(damaged.data()),static_cast<std::streamsize>(damaged.size()));
  if(!output)return false;changedPhysicalOffset=compressedAt;return true;
 }
 return false;
}
DRW_UnsupportedObject body(const char* path,std::uint32_t handle,int type) {
 std::ifstream in(path,std::ios::binary);DRW_UnsupportedObject r;r.m_version=DRW::AC1018;r.m_handle=handle;r.m_objectType=type;
 r.m_rawBytes.assign(std::istreambuf_iterator<char>(in),{});r.m_objectSize=static_cast<std::uint32_t>(r.m_rawBytes.size());return r;
}
int main(int argc,char**argv){
 if(argc==3&&(std::string(argv[1])=="--physical-crc-retry"
      ||std::string(argv[1])=="--physical-row-crc-retry")){
  const bool rowMode=std::string(argv[1])=="--physical-row-crc-retry";
  const std::uint32_t selectedHandle=rowMode?0x14u:5u;
  const std::filesystem::path sourcePath(argv[2]);
  const std::filesystem::path path=sourcePath.parent_path()
      /(sourcePath.stem().string()+".p5-physical-crc-probe"+sourcePath.extension().string());
  if(!std::filesystem::is_regular_file(sourcePath)||std::filesystem::exists(path)
      ||!std::filesystem::copy_file(sourcePath,path))return 40;
  struct RemoveCopy {const std::filesystem::path& path;~RemoveCopy(){std::error_code e;std::filesystem::remove(path,e);}} removeCopy{path};
  Probe probe;dx_data initial;
  const bool selected=probe.enableVerifiedLTypeResourceSource(5u,0x16u,0x14u,0x15u);
  const bool imported=selected&&probe.fileImport(path.string(),&initial,false);
  if(!imported||probe.tickets!=1u||probe.rowTickets!=3u
      ||(rowMode?!probe.byBlockFrame:!probe.controlFrame))return 44;
  const DRW_UnsupportedObject& selectedFrame=rowMode?*probe.byBlockFrame:*probe.controlFrame;
  std::vector<std::uint8_t> original;std::size_t physicalOffset=0u;
  if(!corruptCompressedObjectFrameCrc(path,selectedFrame,original,physicalOffset))return 41;
  struct RestoreFile {const std::filesystem::path& path;const std::vector<std::uint8_t>& bytes;bool active{true};
   ~RestoreFile(){if(active){std::ofstream out(path,std::ios::binary|std::ios::trunc);
    out.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));}}} restore{path,original};
  probe.reset();dx_data damaged;const bool damagedRead=probe.fileImport(path.string(),&damaged,false);
  bool frameDiagnostic=false,pageChecksumDiagnostic=false;
  const auto& rows=damaged.dwgReadOutcome.observations.integrity;
  for(std::size_t i=0u;i<rows.retained;++i){const auto& row=rows.rows[i];
   if(row.kind==DwgIntegrityCheckKind::ObjectFrameBounds&&row.hasLogicalHandle
       &&row.logicalHandle==selectedHandle)frameDiagnostic=true;
   if(row.kind==DwgIntegrityCheckKind::DataPageChecksum||row.kind==DwgIntegrityCheckKind::DataPageCrc)pageChecksumDiagnostic=true;}
  const bool noLeakedTickets=probe.tickets==0u&&probe.rowTickets==0u;
  {std::ofstream out(path,std::ios::binary|std::ios::trunc);
   out.write(reinterpret_cast<const char*>(original.data()),static_cast<std::streamsize>(original.size()));
   if(!out)return 42;}
  restore.active=false;probe.reset();dx_data retry;
  const bool retryRead=probe.fileImport(path.string(),&retry,false);
  const bool retryBound=retryRead&&probe.tickets==1u&&probe.rowTickets==3u
      &&probe.ticketValid&&probe.rowTicketValid&&probe.controlFrame;
  std::cout<<"imported="<<imported<<" damagedRead="<<damagedRead
      <<" frameDiagnostic="<<frameDiagnostic<<" pageChecksumDiagnostic="<<pageChecksumDiagnostic
      <<" noLeakedTickets="<<noLeakedTickets<<" retryRead="<<retryRead
      <<" retryBound="<<retryBound<<" retryControlTickets="<<probe.tickets
      <<" retryRowTickets="<<probe.rowTickets<<" retryControlValid="<<probe.ticketValid
      <<" retryRowsValid="<<probe.rowTicketValid<<" selectedHandle="<<selectedHandle
      <<" physicalCompressedByte="<<physicalOffset<<'\n';
  return imported&&frameDiagnostic&&!pageChecksumDiagnostic&&noLeakedTickets&&retryBound?0:43;
 }
 if(argc==5&&std::string(argv[1])=="--dxf-roundtrip-resource"){
  const std::filesystem::path sourcePath(argv[2]),asciiPath(argv[3]),binaryPath(argv[4]);
  if(!std::filesystem::is_regular_file(sourcePath)||std::filesystem::exists(asciiPath)
      ||std::filesystem::exists(binaryPath)||asciiPath==binaryPath)return 30;
  struct Cleanup{std::filesystem::path a,b;~Cleanup(){std::error_code e;std::filesystem::remove(a,e);std::filesystem::remove(b,e);}}cleanup{asciiPath,binaryPath};
  dx_iface sourceReader;dx_data source;
  const bool selected=sourceReader.enableVerifiedLTypeResourceSource(5u,0x16u,0x14u,0x15u);
  const bool imported=selected&&sourceReader.fileImport(sourcePath.string(),&source,false);
  const bool sourceBound=imported&&dx_iface::hasUnchangedVerifiedLTypeResourceSource(
      source,5u,0x16u,0x14u,0x15u);
  if(!sourceBound)return 31;
  const auto check=[&](const std::filesystem::path& outputPath,bool binary){
   dxfRW writer(outputPath.string().c_str());DxfLtypeSource outputSource;
   const bool written=writer.write(&outputSource,DRW::AC1018,binary)&&outputSource.complete;
   if(!written){const auto diagnostic=writer.getLastDiagnostic();
    std::cerr<<"DXF write failed binary="<<binary<<" error="<<writer.getError()
        <<" code="<<diagnostic.code<<" message="<<diagnostic.message<<'\n';return false;}
   dx_iface readbackReader;dx_data readback;
   if(!readbackReader.fileImport(outputPath.string(),&readback,false)){
    std::cerr<<"DXF readback failed binary="<<binary<<" error="<<readback.dxfReadOutcome.error
        <<" version="<<static_cast<int>(readback.sourceVersion)<<'\n';return false;
   }
   const auto equalName=[](std::string_view left,std::string_view right){
    return left.size()==right.size()&&std::equal(left.begin(),left.end(),right.begin(),
     [](unsigned char a,unsigned char b){
      const auto upper=[](unsigned char c){return c>='a'&&c<='z'
        ?static_cast<unsigned char>(c-'a'+'A'):c;};return upper(a)==upper(b);
     });
   };
   for(const auto handle:{0x16u,0x14u,0x15u}){
    const auto expected=std::find_if(source.lineTypes.cbegin(),source.lineTypes.cend(),
        [handle](const DRW_LType& row){return row.handle==handle;});
    if(expected==source.lineTypes.cend())return false;
    const auto actual=std::find_if(readback.lineTypes.cbegin(),readback.lineTypes.cend(),
        [&expected,&equalName](const DRW_LType& row){return equalName(row.name,expected->name);});
    if(actual==readback.lineTypes.cend()){
     std::cerr<<"DXF row missing="<<expected->name<<" binary="<<binary<<'\n';return false;
    }
    if(actual->desc!=expected->desc
        ||(actual->flags&~64)!=(expected->flags&~64)
        ||actual->alignment!=expected->alignment
        ||actual->length!=expected->length||actual->size!=expected->size
        ||actual->path!=expected->path||actual->segments.size()!=expected->segments.size()){
     std::cerr<<"DXF row mismatch="<<expected->name<<" binary="<<binary
       <<" flags="<<expected->flags<<"/"<<actual->flags
       <<" desc="<<expected->desc<<"/"<<actual->desc
       <<" alignment="<<expected->alignment<<"/"<<actual->alignment
       <<" length="<<expected->length<<"/"<<actual->length
       <<" size="<<expected->size<<"/"<<actual->size<<'\n';return false;
    }
   }
   return true;
  };
  const bool ascii=check(asciiPath,false),binary=check(binaryPath,true);
  std::cout<<"sourceBound="<<sourceBound<<" asciiReadback="<<ascii
      <<" binaryReadback="<<binary<<'\n';
  return ascii&&binary?0:32;
 }
 if(argc==4&&std::string(argv[1])=="--roundtrip-resource"){
  const std::filesystem::path sourcePath(argv[2]),outputPath(argv[3]);
  if(!std::filesystem::is_regular_file(sourcePath)||std::filesystem::exists(outputPath))return 30;
  dx_iface sourceReader;dx_data source;
  const bool sourceSelected=sourceReader.enableVerifiedLTypeResourceSource(5u,0x16u,0x14u,0x15u);
  const bool sourceRead=sourceSelected&&sourceReader.fileImport(sourcePath.string(),&source,false);
  const bool sourceBound=sourceRead&&dx_iface::hasUnchangedVerifiedLTypeResourceSource(
      source,5u,0x16u,0x14u,0x15u);
  if(!sourceBound)return 31;
  struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove(p,e);}}cleanup{outputPath};
  dwgRW writer(outputPath.string().c_str());GeneratedSource generated(writer,&source.lineTypes);
  const bool written=writer.write(&generated,DRW::AC1018,true)&&generated.generated;
  if(!written)return 32;
  dx_iface outputReader;dx_data output;
  const bool outputSelected=outputReader.enableVerifiedLTypeResourceSource(
      DRW::DwgLtypeControlHandle,generated.continuousHandle,
      generated.byBlockHandle,generated.byLayerHandle);
  const bool outputRead=outputSelected&&outputReader.fileImport(outputPath.string(),&output,false);
  const bool outputBound=outputRead&&dx_iface::hasUnchangedVerifiedLTypeResourceSource(
      output,DRW::DwgLtypeControlHandle,generated.continuousHandle,
      generated.byBlockHandle,generated.byLayerHandle);
  const auto sameValues=[&](std::uint32_t sourceHandle,std::uint32_t outputHandle){
   const auto a=std::find_if(source.lineTypes.cbegin(),source.lineTypes.cend(),
       [sourceHandle](const DRW_LType& row){return row.handle==sourceHandle;});
   const auto b=std::find_if(output.lineTypes.cbegin(),output.lineTypes.cend(),
       [outputHandle](const DRW_LType& row){return row.handle==outputHandle;});
   const bool sameName=a!=source.lineTypes.cend()&&b!=output.lineTypes.cend()
       &&a->name.size()==b->name.size()
       &&std::equal(a->name.begin(),a->name.end(),b->name.begin(),
          [](unsigned char left,unsigned char right){
           const auto upper=[](unsigned char c){return c>='a'&&c<='z'
             ?static_cast<unsigned char>(c-'a'+'A'):c;};
           return upper(left)==upper(right);
          });
   return sameName&&a->desc==b->desc&&a->flags==b->flags
       &&a->alignment==b->alignment&&a->length==b->length
       &&a->size==b->size&&a->path==b->path
       &&a->segments.size()==b->segments.size();
  };
  const bool valuesMatch=outputBound
      &&sameValues(0x16u,generated.continuousHandle)
      &&sameValues(0x14u,generated.byBlockHandle)
      &&sameValues(0x15u,generated.byLayerHandle);
  if(!valuesMatch){
   for(const auto pair:{std::pair<std::uint32_t,std::uint32_t>{0x16u,generated.continuousHandle},
       {0x14u,generated.byBlockHandle},{0x15u,generated.byLayerHandle}}){
    const auto a=std::find_if(source.lineTypes.cbegin(),source.lineTypes.cend(),
        [&](const DRW_LType& row){return row.handle==pair.first;});
    const auto b=std::find_if(output.lineTypes.cbegin(),output.lineTypes.cend(),
        [&](const DRW_LType& row){return row.handle==pair.second;});
   if(a!=source.lineTypes.cend()&&b!=output.lineTypes.cend())
     std::cerr<<"row="<<a->name<<"/"<<b->name<<" desc="<<a->desc.size()<<"/"<<b->desc.size()
       <<" flags="<<a->flags<<"/"<<b->flags<<" align="<<a->alignment<<"/"<<b->alignment
       <<" length="<<a->length<<"/"<<b->length<<" size="<<a->size<<"/"<<b->size
       <<" path="<<a->path.size()<<"/"<<b->path.size()
       <<" segments="<<a->segments.size()<<"/"<<b->segments.size()<<'\n';
   }
  }
  Probe observer;dx_data observedOutput;
  const bool observed=observer.fileImport(outputPath.string(),&observedOutput,false);
  bool normativeOrder=false;
  if(observed&&observer.controlFrame){
   dwgBuffer body(observer.controlFrame->m_rawBytes.data(),
       observer.controlFrame->m_rawBytes.size(),nullptr);DRW_ObjControl control;
   normativeOrder=control.parseDwg(DRW::AC1018,&body,
       static_cast<std::uint32_t>(observer.controlFrame->m_rawBytes.size()*8u))
       &&body.isGood()&&control.orderedHandleSlots.size()==3u
       &&control.orderedHandleRoles.size()==3u
       &&control.orderedHandleSlots[0]==generated.continuousHandle
       &&control.orderedHandleRoles[0]==2u
       &&control.orderedHandleSlots[1]==generated.byLayerHandle
       &&control.orderedHandleRoles[1]==3u
       &&control.orderedHandleSlots[2]==generated.byBlockHandle
       &&control.orderedHandleRoles[2]==3u;
  }
  std::cout<<"sourceBound="<<sourceBound<<" written="<<written
      <<" outputBound="<<outputBound<<" valuesMatch="<<valuesMatch
      <<" normativeOrder="<<normativeOrder<<'\n';
  return outputBound&&valuesMatch&&normativeOrder?0:33;
 }
 if(argc==7&&std::string(argv[1])=="--inspect-resource"){
  const auto parseHandle=[](const char* text,std::uint32_t& value){
   char* end=nullptr;const auto parsed=std::strtoul(text,&end,0);
   if(text==end||*end!='\0'||parsed==0u||parsed>0xfffffffful)return false;
   value=static_cast<std::uint32_t>(parsed);return true;
  };
  std::uint32_t control=0u,ordinary=0u,byBlock=0u,byLayer=0u;
  if(!parseHandle(argv[3],control)||!parseHandle(argv[4],ordinary)
      ||!parseHandle(argv[5],byBlock)||!parseHandle(argv[6],byLayer))return 2;
  dx_iface reader;dx_data data;
  const bool selected=reader.enableVerifiedLTypeResourceSource(
      control,ordinary,byBlock,byLayer);
  const bool imported=selected&&reader.fileImport(argv[2],&data,false);
  const bool bound=imported&&dx_iface::hasUnchangedVerifiedLTypeResourceSource(
      data,control,ordinary,byBlock,byLayer);
  std::cout<<"selected="<<selected<<" imported="<<imported
      <<" operationSucceeded="<<data.dwgReadOutcome.operationSucceeded
      <<" sourceBound="<<bound<<" generation="<<data.importGeneration<<'\n';
  return bound?0:1;
 }
 if(argc==3&&std::string(argv[1])=="--self-test"){
  const std::filesystem::path path(argv[2]);if(std::filesystem::exists(path))return 3;
  struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove(p,e);}} cleanup{path};
  dx_iface selectedLtype,selectedXrecord,duplicateLtype;
  const bool ltypeSelection=selectedLtype.enableVerifiedLTypeResourceSource(5u,0x16u,0x14u,0x15u)
      &&!selectedLtype.enableVerifiedXRecordSource(0xbcu);
  const bool xrecordSelection=selectedXrecord.enableVerifiedXRecordSource(0xbcu)
      &&!selectedXrecord.enableVerifiedLTypeResourceSource(5u,0x16u,0x14u,0x15u);
  const bool duplicateSelection=!duplicateLtype.enableVerifiedLTypeResourceSource(5u,0x16u,0x16u,0x15u);
  dwgRW writer(path.string().c_str());GeneratedSource generated(writer);
  if(!writer.write(&generated,DRW::AC1018,true)||!generated.generated)return 4;
  std::ifstream source(path,std::ios::binary);
  const std::vector<char> original((std::istreambuf_iterator<char>(source)),{});
  if(!source||original.size()<64u)return 5;
  dx_iface ltypeReader;dx_data ltypeData;
  const bool ltypeSelected=ltypeReader.enableVerifiedLTypeResourceSource(
      DRW::DwgLtypeControlHandle,generated.continuousHandle,
      generated.byBlockHandle,generated.byLayerHandle);
  const bool ltypeImported=ltypeSelected&&ltypeReader.fileImport(path.string(),&ltypeData,false);
  bool ltypeSourceBound=ltypeImported&&dx_iface::hasUnchangedVerifiedLTypeResourceSource(
      ltypeData,DRW::DwgLtypeControlHandle,generated.continuousHandle,
      generated.byBlockHandle,generated.byLayerHandle);
  bool ltypeAllRowMutationsRefused=true,ltypeAllRowRestoresAccepted=true;
  bool ltypeWrongSelectionRefused=false;
  if(ltypeSourceBound) {
   ltypeWrongSelectionRefused=!dx_iface::hasUnchangedVerifiedLTypeResourceSource(
       ltypeData,DRW::DwgLtypeControlHandle,generated.byBlockHandle,
       generated.continuousHandle,generated.byLayerHandle);
   const std::array<std::uint32_t,3> selectedRows{{generated.continuousHandle,
       generated.byBlockHandle,generated.byLayerHandle}};
   for(const auto handle:selectedRows) {
    const auto row=std::find_if(ltypeData.lineTypes.begin(),ltypeData.lineTypes.end(),
        [handle](const DRW_LType& value){return value.handle==handle;});
    if(row==ltypeData.lineTypes.end()) {
     ltypeAllRowMutationsRefused=false;ltypeAllRowRestoresAccepted=false;continue;
    }
    const auto description=row->desc;row->desc+="_MUTATED";
    ltypeAllRowMutationsRefused=ltypeAllRowMutationsRefused
        &&!dx_iface::hasUnchangedVerifiedLTypeResourceSource(ltypeData,
          DRW::DwgLtypeControlHandle,generated.continuousHandle,
          generated.byBlockHandle,generated.byLayerHandle);
    row->desc=description;
    ltypeAllRowRestoresAccepted=ltypeAllRowRestoresAccepted
        &&dx_iface::hasUnchangedVerifiedLTypeResourceSource(ltypeData,
          DRW::DwgLtypeControlHandle,generated.continuousHandle,
          generated.byBlockHandle,generated.byLayerHandle);
   }
  }
  const std::filesystem::path ltypeRoundtripPath(path.string()+".ltype-roundtrip.dwg");
  struct RoundtripCleanup{std::filesystem::path p;~RoundtripCleanup(){std::error_code e;std::filesystem::remove(p,e);}} roundtripCleanup{ltypeRoundtripPath};
  bool ltypeRoundtripWritten=false,ltypeRoundtripBound=false;
  bool ltypeRoundtripValuesMatch=false,ltypeRoundtripNormativeOrder=false;
  if(ltypeSourceBound&&!std::filesystem::exists(ltypeRoundtripPath)){
   dwgRW outputWriter(ltypeRoundtripPath.string().c_str());
   GeneratedSource outputSource(outputWriter,&ltypeData.lineTypes);
   ltypeRoundtripWritten=outputWriter.write(&outputSource,DRW::AC1018,true)
       &&outputSource.generated;
   if(ltypeRoundtripWritten){
    dx_iface outputReader;dx_data outputData;
    const bool outputSelected=outputReader.enableVerifiedLTypeResourceSource(
        DRW::DwgLtypeControlHandle,outputSource.continuousHandle,
        outputSource.byBlockHandle,outputSource.byLayerHandle);
    const bool outputImported=outputSelected
        &&outputReader.fileImport(ltypeRoundtripPath.string(),&outputData,false);
    ltypeRoundtripBound=outputImported
        &&dx_iface::hasUnchangedVerifiedLTypeResourceSource(outputData,
          DRW::DwgLtypeControlHandle,outputSource.continuousHandle,
          outputSource.byBlockHandle,outputSource.byLayerHandle);
    const auto sameSelectedValues=[&](std::uint32_t sourceHandle,std::uint32_t outputHandle){
     const auto sourceRow=std::find_if(ltypeData.lineTypes.cbegin(),ltypeData.lineTypes.cend(),
         [sourceHandle](const DRW_LType& row){return row.handle==sourceHandle;});
     const auto outputRow=std::find_if(outputData.lineTypes.cbegin(),outputData.lineTypes.cend(),
         [outputHandle](const DRW_LType& row){return row.handle==outputHandle;});
     return sourceRow!=ltypeData.lineTypes.cend()&&outputRow!=outputData.lineTypes.cend()
         &&sourceRow->name==outputRow->name&&sourceRow->desc==outputRow->desc
         &&sourceRow->flags==outputRow->flags&&sourceRow->alignment==outputRow->alignment
         &&sourceRow->length==outputRow->length&&sourceRow->size==outputRow->size
         &&sourceRow->path==outputRow->path&&sourceRow->segments.size()==outputRow->segments.size();
    };
    ltypeRoundtripValuesMatch=ltypeRoundtripBound
        &&sameSelectedValues(generated.continuousHandle,outputSource.continuousHandle)
        &&sameSelectedValues(generated.byBlockHandle,outputSource.byBlockHandle)
        &&sameSelectedValues(generated.byLayerHandle,outputSource.byLayerHandle);
    Probe outputProbe;dx_data observedOutput;
    const bool observedOutputRead=outputProbe.fileImport(
        ltypeRoundtripPath.string(),&observedOutput,false);
    if(observedOutputRead&&outputProbe.tickets==1u&&outputProbe.rowTickets==3u
        &&outputProbe.controlFrame){
     dwgBuffer controlBody(outputProbe.controlFrame->m_rawBytes.data(),
         outputProbe.controlFrame->m_rawBytes.size(),nullptr);
     DRW_ObjControl parsedControl;
     ltypeRoundtripNormativeOrder=parsedControl.parseDwg(DRW::AC1018,
         &controlBody,static_cast<std::uint32_t>(
           outputProbe.controlFrame->m_rawBytes.size()*8u))
         &&controlBody.isGood()&&parsedControl.handle==DRW::DwgLtypeControlHandle
         &&parsedControl.orderedHandleSlots.size()==3u
         &&parsedControl.orderedHandleRoles.size()==3u
         &&parsedControl.orderedHandleRoles[0]==2u
         &&parsedControl.orderedHandleSlots[0]==outputSource.continuousHandle
         &&parsedControl.orderedHandleRoles[1]==3u
         &&parsedControl.orderedHandleSlots[1]==outputSource.byLayerHandle
         &&parsedControl.orderedHandleRoles[2]==3u
         &&parsedControl.orderedHandleSlots[2]==outputSource.byBlockHandle;
    }
   }
  }
  auto data=std::make_unique<dx_data>();Probe probe;
  {std::ofstream broken(path,std::ios::binary|std::ios::trunc);broken.write(original.data(),20);if(!broken)return 6;}
  const bool failedRead=probe.fileImport(path.string(),data.get(),false);
  const bool leaked=probe.tickets!=0u||probe.rowTickets!=0u;
  {std::ofstream restored(path,std::ios::binary|std::ios::trunc);restored.write(original.data(),static_cast<std::streamsize>(original.size()));if(!restored)return 7;}
  const bool read=probe.fileImport(path.string(),data.get(),false);
  const bool ok=ltypeSelection&&xrecordSelection&&duplicateSelection
      &&ltypeSourceBound&&ltypeWrongSelectionRefused
      &&ltypeAllRowMutationsRefused&&ltypeAllRowRestoresAccepted
      &&ltypeRoundtripWritten&&ltypeRoundtripBound&&ltypeRoundtripValuesMatch
      &&ltypeRoundtripNormativeOrder
      &&!failedRead&&!leaked&&read&&probe.tickets==1u&&probe.ticketValid
      &&probe.rowTickets==3u&&probe.rowTicketValid;
  if(!ok){std::cerr<<"roundtripWritten="<<ltypeRoundtripWritten
      <<" roundtripBound="<<ltypeRoundtripBound
      <<" roundtripValuesMatch="<<ltypeRoundtripValuesMatch
      <<" roundtripNormativeOrder="<<ltypeRoundtripNormativeOrder<<'\n';return 8;}
  std::vector<std::uint8_t> originalBytes;if(!probe.controlFrame||!corruptObjectFrameCrc(path,*probe.controlFrame,originalBytes))return 9;
  probe.reset();auto damaged=std::make_unique<dx_data>();const bool damagedRead=probe.fileImport(path.string(),damaged.get(),false);
  bool controlDiagnostic=false;const auto& integrity=damaged->dwgReadOutcome.observations.integrity;
  for(std::size_t i=0;i<integrity.retained;++i){const auto& row=integrity.rows[i];
   if(row.kind==DwgIntegrityCheckKind::ObjectFrameBounds&&row.hasLogicalHandle&&row.logicalHandle==5u)controlDiagnostic=true;
   if(row.kind==DwgIntegrityCheckKind::DataPageChecksum||row.kind==DwgIntegrityCheckKind::DataPageCrc)return 10;}
  const bool leakedCrcTickets=probe.tickets!=0u||probe.rowTickets!=0u;
  {std::ofstream restored(path,std::ios::binary|std::ios::trunc);restored.write(reinterpret_cast<const char*>(originalBytes.data()),static_cast<std::streamsize>(originalBytes.size()));if(!restored)return 11;}
  probe.reset();auto retryData=std::make_unique<dx_data>();const bool crcRetryRead=probe.fileImport(path.string(),retryData.get(),false);
  const bool crcRetry=crcRetryRead&&probe.tickets==1u&&probe.ticketValid&&probe.rowTickets==3u&&probe.rowTicketValid;
  const auto byBlockHandle=probe.byBlockFrame?probe.byBlockFrame->m_handle:0u;
  std::vector<std::uint8_t> rowOriginalBytes;
  if(!probe.byBlockFrame||!corruptObjectFrameCrc(path,*probe.byBlockFrame,rowOriginalBytes))return 13;
  probe.reset();auto rowDamagedData=std::make_unique<dx_data>();
  const bool rowDamagedRead=probe.fileImport(path.string(),rowDamagedData.get(),false);
  bool rowControlDiagnostic=false;const auto& rowIntegrity=rowDamagedData->dwgReadOutcome.observations.integrity;
  for(std::size_t i=0;i<rowIntegrity.retained;++i){const auto& row=rowIntegrity.rows[i];
   if(row.kind==DwgIntegrityCheckKind::ObjectFrameBounds&&row.hasLogicalHandle&&row.logicalHandle==byBlockHandle)rowControlDiagnostic=true;
   if(row.kind==DwgIntegrityCheckKind::DataPageChecksum||row.kind==DwgIntegrityCheckKind::DataPageCrc)return 14;}
  const bool rowCrcLeakedTickets=probe.tickets!=0u||probe.rowTickets!=0u;
  {std::ofstream restored(path,std::ios::binary|std::ios::trunc);restored.write(reinterpret_cast<const char*>(rowOriginalBytes.data()),static_cast<std::streamsize>(rowOriginalBytes.size()));if(!restored)return 15;}
  probe.reset();auto rowRetryData=std::make_unique<dx_data>();
  const bool rowRetryRead=probe.fileImport(path.string(),rowRetryData.get(),false);
  const bool rowRetry=rowRetryRead&&probe.tickets==1u&&probe.ticketValid&&probe.rowTickets==3u&&probe.rowTicketValid;
  std::cout<<"generated=1 ltypeSelected="<<ltypeSelected<<" ltypeImported="<<ltypeImported
    <<" ltypeSourceBound="<<ltypeSourceBound<<" ltypeWrongSelectionRefused="<<ltypeWrongSelectionRefused
    <<" ltypeAllRowMutationsRefused="<<ltypeAllRowMutationsRefused
    <<" ltypeAllRowRestoresAccepted="<<ltypeAllRowRestoresAccepted
    <<" ltypeRoundtripWritten="<<ltypeRoundtripWritten
    <<" ltypeRoundtripBound="<<ltypeRoundtripBound
    <<" ltypeRoundtripValuesMatch="<<ltypeRoundtripValuesMatch
    <<" ltypeRoundtripNormativeOrder="<<ltypeRoundtripNormativeOrder
    <<" failedRead="<<failedRead<<" leakedTickets="<<leaked
    <<" retryRead="<<read<<" crcRead="<<damagedRead<<" crcDiagnostic="<<controlDiagnostic
    <<" crcLeakedTickets="<<leakedCrcTickets<<" crcRetry="<<crcRetry
    <<" rowCrcRead="<<rowDamagedRead<<" rowCrcDiagnostic="<<rowControlDiagnostic
    <<" rowCrcLeakedTickets="<<rowCrcLeakedTickets<<" rowCrcRetry="<<rowRetry<<"\n";
  return !damagedRead&&controlDiagnostic&&!leakedCrcTickets&&crcRetry
      &&!rowDamagedRead&&rowControlDiagnostic&&!rowCrcLeakedTickets&&rowRetry?0:16;
 }
 if(argc==3&&std::string(argv[1])=="--emit-dwg"){
  std::ifstream existing(argv[2],std::ios::binary);if(existing)return 3;
  dwgRW writer(argv[2]);GeneratedSource source(writer);
  return writer.write(&source,DRW::AC1018,true)&&source.generated?0:4;
 }
 if(argc!=2&&argc!=4)return 2;auto data=std::make_unique<dx_data>();auto p=std::make_unique<Probe>();
 const bool read=p->fileImport(argv[1],data.get(),false);
 if(argc==2){DwgLTypeControlByBlockAc1018Candidate::Result joined;
  const bool bound=p->controlFrame&&p->byBlockFrameModel
    &&DwgLTypeControlByBlockAc1018Candidate::inspect(*p->controlFrame,*p->byBlockFrame,
        *p->byBlockFrameModel,joined);
  std::cout<<"read="<<read<<" controlTickets="<<p->tickets<<" controlValid="<<p->ticketValid
  <<" rowTickets="<<p->rowTickets<<" rowValid="<<p->rowTicketValid<<" joined="<<bound<<"\n";
  return !(read&&p->tickets==1u&&p->ticketValid&&p->rowTickets==3u&&p->rowTicketValid);}
 const auto control=body(argv[2],5u,56);const auto row=body(argv[3],0x14u,57);
 DwgLTypeControlByBlockAc1018Candidate::Result r;const bool ok=read&&!p->duplicate&&p->model
  &&DwgLTypeControlByBlockAc1018Candidate::inspect(control,row,*p->model,r);
 std::cout<<"read="<<read<<" matched="<<ok<<" control="<<r.control<<" byblock="<<r.byBlock
          <<" ordinary="<<r.ordinary.target<<" bylayer="<<r.byLayerPhantom.target<<'\n';return !ok;}
