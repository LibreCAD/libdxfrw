// Locally generated selected 0xdc XRECORD reader/CRC control. This does not
// construct or authorize a complete TABLESTYLE graph.
#define main table_candidate_binding_included_main
#include "dwg_table_candidate_binding_tests.cpp"
#undef main

namespace {
class StyleXRecordWriter final:public dx_iface {
public:
 explicit StyleXRecordWriter(dwgRW& w):writer(w){}
 void writeHeader(DRW_Header& h) override {h.vars.clear();}
 void collectDwgAppIds() override {}
 void writeBlocks() override {} void writeBlockRecords() override {} void writeEntities() override {}
 void writeLTypes() override {} void writeLayers() override {} void writeTextstyles() override {}
 void writeVports() override {} void writeViews() override {} void writeUCSs() override {}
 void writeDimstyles() override {} void writeAppId() override {}
 void writeObjects() override {
  DRW_XRecord x;x.handle=0xdcu;x.parentHandle=0xdbu;x.reactorHandles={0xdbu};x.setDwgCommonObjectState(1,1u,false);x.m_cloning=1;
  auto& e=x.m_dataEntries;e.emplace_back(102,UTF8STRING("ACAD_ROUNDTRIP_PRE2007_TABLESTYLE"));e.emplace_back(90,std::int32_t{4});
  e.emplace_back(91,std::int32_t{0});e.emplace_back(1,UTF8STRING(""));e.emplace_back(92,std::int32_t{4});
  e.emplace_back(93,std::int32_t{0});e.emplace_back(2,UTF8STRING(""));e.emplace_back(94,std::int32_t{4});
  e.emplace_back(95,std::int32_t{0});e.emplace_back(3,UTF8STRING(""));wrote=writer.writeXRecord(&x);
 }
 bool finalizeDwgWrite() override{return wrote;}
 bool finalizeDwgWriteStructure() override{return true;}
 dwgRW& writer;bool wrote=false;
};
void emitHandle(dwgBufferW& b,std::uint8_t role,std::uint32_t h){dwgHandle v;v.code=role;v.ref=h;b.putHandle(v);}
std::vector<std::uint8_t> authoredStyleXRecord(int versionValue=4) {
 dwgBufferW b;b.putObjType(DRW::AC1018,79u);const auto end=b.bitCount();b.putRawLong32(0u);
 emitHandle(b,0u,0xdcu);b.putBitShort(0u);b.putBitLong(1);b.putBit(1u);b.putBitLong(89);
 const auto text=[&](unsigned code,const std::string& s){b.putRawShort16(code);b.putRawShort16(static_cast<std::uint16_t>(s.size()));b.putRawChar8(45u);for(char c:s)b.putRawChar8(static_cast<std::uint8_t>(c));};
 const auto integer=[&](unsigned code,int value){b.putRawShort16(code);b.putRawLong32(static_cast<std::uint32_t>(value));};
 text(102,"ACAD_ROUNDTRIP_PRE2007_TABLESTYLE");integer(90,versionValue);integer(91,0);text(1,"");integer(92,4);integer(93,0);text(2,"");integer(94,4);integer(95,0);text(3,"");
 b.putBitShort(1u);b.patchRawLong32AtBit(end,static_cast<std::uint32_t>(b.bitCount()));emitHandle(b,4u,0xdbu);emitHandle(b,4u,0xdbu);b.alignToByte();return b.data();
}
bool inspectSelected(const dx_data& data,bool expected){
 if(!dx_iface::hasUnchangedVerifiedXRecordSource(data,0xdcu))return false;
 const auto it=std::find_if(data.xRecords.cbegin(),data.xRecords.cend(),[](const DRW_XRecord& x){return x.handle==0xdcu;});
 if(it==data.xRecords.cend())return false;
 const auto& body=DxIfaceStandardDwgCaptureTestAccess::standardRaw(data,0xdcu);
 DwgTableStyleRoundtripXRecordAc1018Candidate::Result r;
 return DwgTableStyleRoundtripXRecordAc1018Candidate::inspect(body,*it,r)==expected && (!expected || (r.handle==0xdcu && r.owner==0xdbu && r.reactor==0xdbu));
}
}
int main(int argc,char** argv){
 if(argc!=2)return 2;const std::filesystem::path path(argv[1]);if(std::filesystem::exists(path))return 3;
 struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code e;std::filesystem::remove(path,e);}} cleanup{path};
 dwgRW writer(path.string().c_str());StyleXRecordWriter source(writer);if(!writer.write(&source,DRW::AC1018,true)||!source.wrote)return 4;
 dx_iface plain;dx_data original;if(!plain.fileImport(path.string(),&original,false))return 5;
 const auto writerRaw=DxIfaceStandardDwgCaptureTestAccess::standardRaw(original,0xdcu);
 const auto expected=authoredStyleXRecord();if(!replaceSelectedBody(path,writerRaw,expected))return 6;
 dx_iface reader;if(!reader.enableVerifiedXRecordSource(0xdcu))return 7;dx_data good;
 if(!reader.fileImport(path.string(),&good,false)||!inspectSelected(good,true))return 8;
 const auto selected=DxIfaceStandardDwgCaptureTestAccess::standardRaw(good,0xdcu);
 const auto changed=authoredStyleXRecord(5);if(!replaceSelectedBody(path,selected,changed))return 9;
 dx_data altered;if(!reader.fileImport(path.string(),&altered,false)||!inspectSelected(altered,false))return 10;
 auto alteredRaw=selected;alteredRaw.m_rawBytes=changed;if(!replaceSelectedBody(path,alteredRaw,expected))return 11;
 dx_data recovered;if(!reader.fileImport(path.string(),&recovered,false)||!inspectSelected(recovered,true))return 12;
 std::vector<std::uint8_t> before;if(!corruptSelectedCrc(path,selected,before))return 13;
 dx_data broken;(void)reader.fileImport(path.string(),&broken,false);
 if(dx_iface::hasUnchangedVerifiedXRecordSource(broken,0xdcu))return 14;
 const auto& integrity=broken.dwgReadOutcome.observations.integrity;
 if(integrity.extractionOmitted || integrity.reportedUpstreamDropped || integrity.aggregateSaturated)return 17;
 bool targetFrameFailure=false;
 for(std::size_t i=0;i<integrity.retained;++i){const auto& row=integrity.rows[i];
  if(row.kind==DwgIntegrityCheckKind::DataPageChecksum || row.kind==DwgIntegrityCheckKind::DataPageCrc)return 18;
  if(row.kind==DwgIntegrityCheckKind::ObjectFrameBounds && row.hasLogicalHandle && row.logicalHandle==0xdcu)targetFrameFailure=true;}
 if(!targetFrameFailure)return 19;
 {std::ofstream out(path,std::ios::binary|std::ios::trunc);out.write(reinterpret_cast<const char*>(before.data()),before.size());if(!out)return 15;}
 dx_data retry;if(!reader.fileImport(path.string(),&retry,false)||!inspectSelected(retry,true))return 16;
 std::cout<<"Generated selected TABLESTYLE XRECORD valid CRC, valid-CRC payload mismatch, object CRC refusal and retry passed; no full graph claim\n";return 0;
}
