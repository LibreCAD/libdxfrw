// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_legacy_blockref_source_relation.h"
#include "dx_iface.h"
#include "dx_data.h"
#include "test_scratch.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cstring>
#include <stdexcept>
#include <sstream>
struct Tag{int code;std::string value;};using Tags=std::vector<Tag>;
class DxIfaceTableSourceTestAccess {public:
 static const dxfRW* input(const dx_data& d){return d.m_tableDxfSource.entitiesReadInput;}
 static bool spoofPriorPass(const dx_data& data){const auto& s=data.m_tableDxfSource;const auto row=s.structuralInputs.find(0xB0u);
  if(row==s.structuralInputs.end()||!row->second||!row->second->raw||!row->second->paired||!row->second->legacyBlockRecordReplayMatched||!row->second->legacyBlockRecordProfile.complete()||s.legacyEntitiesCompoundJoins.count(0xE0u)!=1u)return false;
  for(const auto& x:row->second->legacyBlockRecordProfile.spans)if(x.role==dx_legacy_block_record_source::Role::BlockInsert)return x.handle==0xE0u;return false;}
 static bool crossRead(dx_data& target,const dx_data& foreign) {
  auto& a=target.m_tableDxfSource;const auto& b=foreign.m_tableDxfSource;
  const auto input=a.wholeInput,owner=a.wholeInputOwner;const auto source=a.wholeSourceOwner;
  a.wholeInput=b.wholeInput;a.wholeInputOwner=b.wholeInputOwner;a.wholeSourceOwner=b.wholeSourceOwner;
  blockref_source_relation::Result out;const bool rejected=!dx_iface::legacyBlockRefSourceRelation(target,0xB0u,0xE0u,64u*1024u*1024u,64u*1024u*1024u,out);
  a.wholeInput=input;a.wholeInputOwner=owner;a.wholeSourceOwner=source;return rejected;
 }
 static bool contextFault(dx_data& data) {
  auto& s=data.m_tableDxfSource;if(!s.wholeInput)return false;
  auto& input=const_cast<dx_data::TableDxfSourceCapture::WholeSourceInput&>(*s.wholeInput);
  input.context.binary=!input.context.binary;blockref_source_relation::Result out;
  const bool refused=!dx_iface::legacyBlockRefSourceRelation(data,0xB0u,0xE0u,64u*1024u*1024u,64u*1024u*1024u,out);
  input.context.binary=!input.context.binary;return refused;
 }
 static bool manySourceNoDelivery(dx_data& data) {
  auto& s=data.m_tableDxfSource;if(s.entitiesSources.empty())return false;
  auto sources=s.entitiesSources;auto deliveries=std::move(s.entitiesDeliveries);
  s.entitiesSources.resize(4096u,sources.front());blockref_source_relation::Result out;
  const bool refused=!dx_iface::legacyBlockRefSourceRelation(data,0xB0u,0xE0u,1u,64u*1024u*1024u,out);
  s.entitiesSources=std::move(sources);s.entitiesDeliveries=std::move(deliveries);return refused;
 }
 static bool relation(const dx_data& data,std::uint32_t blockId,std::uint32_t insertId,std::size_t limit,std::size_t bytesLimit) {
  blockref_source_relation::Result out;return dx_iface::legacyBlockRefSourceRelation(data,blockId,insertId,limit,bytesLimit,out)
    &&out.record==blockId&&out.insert==insertId&&out.name.ordinal&&out.reference.ordinal;
 }
};
using namespace WholeSourceV2;
class Reader:public dx_iface {
 bool accepted=false;
public:
 bool spoof=false;int mode=0;Limits limits{};std::size_t joinWork=64u*1024u*1024u,joinBytes=64u*1024u*1024u;std::uint32_t blockId=0xB0u,insertId=0xE0u,spoofOriginal=0xE3u;
 bool wantsDxfBlockSource()const override{return true;}bool wantsDxfEntitiesSource()const override{return true;}bool wantsDxfEntitiesDeliverySource()const override{return true;}
 Limits wholeSourceLimits()const noexcept override{return limits;}
 void wholeSourcePending(const void* id,const Source& value,const Ticket& t)noexcept override {
  if(mode==1)return;
  if(mode==2){const auto f=value.field(0);const_cast<char*>(value.value(f).data())[0]^=1;dx_iface::wholeSourcePending(id,value,t);const_cast<char*>(value.value(f).data())[0]^=1;return;}
  dx_iface::wholeSourcePending(id,value,t);if(mode==3)dx_iface::wholeSourcePending(id,value,t);
 }
 void wholeSourceFinished(const void* id,const FinishToken& t)noexcept override{if(mode!=4)dx_iface::wholeSourceFinished(id,t);}
 void rewrite(const DRW_Block_Record& row,const DRW_RawDxfObject& raw,bool source){if(!spoof||row.handle!=blockId){if(source)dx_iface::addDxfBlockRecordSource(row,raw);else dx_iface::addBlockRecord(row);return;}
  auto& v=const_cast<DRW_Block_Record&>(row);auto& x=const_cast<DRW_RawDxfObject&>(raw);auto saved=v.insertHandles;auto original=x;bool changed=false;std::ostringstream hex;hex<<std::uppercase<<std::hex<<insertId;const auto spelling=hex.str();
  for(std::size_t i=0;i<x.groups.size();++i)if(x.groups[i].code()==331){x.groups[i].addString(331,spelling);if(x.hasRawValues)x.rawValues[i]=spelling;changed=true;}
  if(!changed||v.insertHandles.size()!=1u||v.insertHandles[0]!=spoofOriginal)throw std::runtime_error("fixture");v.insertHandles[0]=insertId;
  if(source)dx_iface::addDxfBlockRecordSource(row,raw);else dx_iface::addBlockRecord(row);v.insertHandles=saved;x=std::move(original);}
 void addDxfBlockRecordSource(const DRW_Block_Record& row,const DRW_RawDxfObject& raw)override{rewrite(row,raw,true);}
 void addBlockRecord(const DRW_Block_Record& row)override{dxfRW::DxfStructuralSourceView view;const auto* d=cData?DxIfaceTableSourceTestAccess::input(*cData):nullptr;
  if(spoof&&row.handle==blockId&&d&&d->getDxfStructuralSourceView(view)&&view.source)rewrite(row,*view.source,false);else dx_iface::addBlockRecord(row);}
 bool import(const std::string& path,dx_data& d){enableWholeDxfSourceObservation();accepted=fileImport(path,&d,false);return accepted;}
 bool relation(const dx_data& d)const{return accepted&&DxIfaceTableSourceTestAccess::relation(d,blockId,insertId,joinWork,joinBytes);}
};
static bool write(const std::filesystem::path& path,const Tags& tags,bool bin){std::ofstream out(path,std::ios::binary);const auto word=[&](std::uint64_t n,unsigned k){for(unsigned i=0;i<k;++i)out.put(static_cast<char>(n>>(i*8u)));};
 if(bin)out.write("AutoCAD Binary DXF\r\n\x1a\0",22);
 for(const auto& t:tags)if(!bin)out<<t.code<<'\n'<<t.value<<'\n';else {word(static_cast<unsigned>(t.code),2);if(t.code>=10&&t.code<=59){double v=std::stod(t.value);std::uint64_t bits=0;std::memcpy(&bits,&v,8);word(bits,8);}else if((t.code>=60&&t.code<=79)||t.code==1070)word(static_cast<std::uint16_t>(std::stoi(t.value)),2);else{out<<t.value;out.put('\0');}}
 return out.good();}
int main(int argc,char**argv){if(argc>1){Reader real;real.enableLegacyDxfIdentityObservation();real.blockId=0x6F2u;real.insertId=0x704u;real.spoofOriginal=0x703u;real.spoof=argc>2;dx_data data;if(!real.import(argv[1],data))return 30;const bool matched=real.relation(data);std::cout<<"real same-read relation="<<matched<<" spoof="<<real.spoof<<"\n";return matched==!real.spoof?0:31;}
 std::error_code ec;const auto dir=libdxfrw_test::reserveTestScratch(std::filesystem::temp_directory_path(),"same-read-blockref",ec);if(ec)return 1;struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{dir};
 Tags doc={{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1018"},{9,"$DWGCODEPAGE"},{3,"ANSI_1252"},{0,"ENDSEC"},
 {0,"SECTION"},{2,"TABLES"},{0,"TABLE"},{2,"BLOCK_RECORD"},{5,"B9"},{100,"AcDbSymbolTable"},{70,"1"},
 {0,"BLOCK_RECORD"},{5,"B0"},{102,"{ACAD_XDICTIONARY"},{360,"D0"},{102,"}"},{330,"B9"},{100,"AcDbSymbolTableRecord"},{100,"AcDbBlockTableRecord"},{2,"TEST_BLOCK"},{340,"0"},{102,"{BLKREFS"},{331,"E0"},{102,"}"},{1001,"TEST_APP"},{1000,"x"},{0,"ENDTAB"},{0,"ENDSEC"},
 {0,"SECTION"},{2,"ENTITIES"},{0,"INSERT"},{5,"E0"},{330,"B0"},{100,"AcDbEntity"},{8,"LayerA"},{100,"AcDbBlockReference"},{66,"1"},{2,"TEST_BLOCK"},{10,"2"},{20,"3"},{30,"0"},{41,"1"},{42,"1"},{43,"1"},
 {0,"ATTRIB"},{5,"E1"},{330,"E0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbText"},{10,"2"},{20,"3"},{30,"0"},{40,"2"},{1,"value"},{100,"AcDbAttribute"},{2,"TEST_TAG"},{70,"0"},{1001,"AcDbAttr"},{1070,"0"},{1070,"1"},
 {0,"SEQEND"},{5,"E2"},{330,"E0"},{100,"AcDbEntity"},{8,"LayerA"},{0,"ENDSEC"},{0,"EOF"}};
 // Ignored comments before the self scalar, inside BLKREFS and at the
 // record tail must not change physical-to-retained group alignment.
 auto head=std::find_if(doc.begin(),doc.end(),[](const Tag& t){return t.code==0&&t.value=="BLOCK_RECORD";});
 if(head==doc.end())return 23;doc.insert(head+1,{999,"before self"});
 auto app=std::find_if(doc.begin(),doc.end(),[](const Tag& t){return t.code==102&&t.value=="{BLKREFS";});
 if(app==doc.end())return 24;doc.insert(app+1,{999,"inside references"});
 auto tail=std::find_if(doc.begin(),doc.end(),[](const Tag& t){return t.code==1000&&t.value=="x";});
 if(tail==doc.end())return 25;doc.insert(tail+1,{999,"record tail"});

 unsigned checks=0;for(bool binary:{false,true}){const auto path=dir/(binary?"binary.dxf":"ascii.dxf");if(!write(path,doc,binary))return 2;Reader reader;reader.enableLegacyDxfIdentityObservation();dx_data good;if(!reader.import(path.string(),good)||!reader.relation(good))return 3;reader.joinWork=0;if(reader.relation(good))return 15;reader.joinWork=64u*1024u*1024u;if(!reader.relation(good))return 16;reader.joinBytes=0;if(reader.relation(good))return 17;reader.joinBytes=64u*1024u*1024u;if(!reader.relation(good)||!DxIfaceTableSourceTestAccess::contextFault(good)||!reader.relation(good)||!DxIfaceTableSourceTestAccess::manySourceNoDelivery(good)||!reader.relation(good))return 18;
  for(int dimension=0;dimension<2;++dimension){std::size_t lo=0,hi=64u*1024u*1024u;while(lo<hi){const auto mid=lo+(hi-lo)/2u;
    if(dimension==0)reader.joinWork=mid;else reader.joinBytes=mid;if(reader.relation(good))hi=mid;else lo=mid+1u;}
   if(!lo)return 26;if(dimension==0)reader.joinWork=lo;else reader.joinBytes=lo;if(!reader.relation(good))return 27;
   if(dimension==0)reader.joinWork=lo-1u;else reader.joinBytes=lo-1u;if(reader.relation(good))return 28;
   reader.joinWork=64u*1024u*1024u;reader.joinBytes=64u*1024u*1024u;if(!reader.relation(good))return 29;}
  ++checks;
  dx_data other;if(!reader.import(path.string(),other)||!reader.relation(other)||!reader.relation(good)||!DxIfaceTableSourceTestAccess::crossRead(good,other)||!reader.relation(good))return 4;++checks;
  for(int mode=1;mode<=4;++mode){Reader bad;bad.enableLegacyDxfIdentityObservation();bad.mode=mode;dx_data data;if(!bad.import(path.string(),data)||bad.relation(data))return 5;bad.mode=0;dx_data retry;if(!bad.import(path.string(),retry)||!bad.relation(retry))return 6;++checks;}
  auto altered=doc;for(auto& t:altered)if(t.code==331&&t.value=="E0")t.value="E3";if(!write(path,altered,binary))return 7;Reader attack;attack.enableLegacyDxfIdentityObservation();attack.spoof=true;dx_data rejected;if(!attack.import(path.string(),rejected)||!DxIfaceTableSourceTestAccess::spoofPriorPass(rejected)||attack.relation(rejected))return 8;++checks;
  if(!write(path,doc,binary))return 9;attack.spoof=false;dx_data retry;if(!attack.import(path.string(),retry)||!attack.relation(retry))return 10;++checks;
  Reader limited;limited.enableLegacyDxfIdentityObservation();limited.limits.fields=0;dx_data noFields;if(!limited.import(path.string(),noFields)||limited.relation(noFields))return 11;limited.limits={};dx_data recovered;if(!limited.import(path.string(),recovered)||!limited.relation(recovered))return 12;++checks;
  if(!reader.import(path.string(),other)||reader.relation(other))return 13;dx_data fresh;if(!reader.import(path.string(),fresh)||!reader.relation(fresh))return 14;++checks;
 }
 std::cout<<"same-read source-only ascii-binary=2 controls="<<checks<<" callback-spoof=2 quota-retry=2 join-work-byte-retry=4 same-data-refusal=2\n";return 0;}
