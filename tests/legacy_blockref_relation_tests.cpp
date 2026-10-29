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
 static bool unknownCensusRow(dx_data& data) {
  auto& s=data.m_tableDxfSource;const auto old=s.legacyIdentityObservation,oldOwner=s.legacyIdentityObservationOwner;
  const auto oldSource=s.legacyIdentitySourceOwner;const auto it=s.legacyEntitiesCompoundJoins.find(0xE0u);
  if(!old||!old->source||it==s.legacyEntitiesCompoundJoins.end()||!it->second)return false;const auto oldJoin=it->second;
  auto forged=std::make_shared<DRW_DxfIdentitySource>(*old->source);auto found=std::find_if(forged->records.begin(),forged->records.end(),
      [](const DRW_DxfIdentitySource::Record& row){return row.kind==DRW_DxfIdentitySource::Kind::TableRecord&&row.name=="BLOCK_RECORD";});
  if(found==forged->records.end())return false;auto row=*found;row.name="UNRECOGNIZED";forged->records.push_back(std::move(row));
  auto input=std::make_shared<dx_data::TableDxfSourceCapture::IdentityInput>(*old);input->source=forged;
  auto join=std::make_shared<dx_data::TableDxfSourceCapture::LegacyEntitiesCompoundJoin>(*oldJoin);join->identity=input;
  s.legacyIdentityObservation=input;s.legacyIdentityObservationOwner=input;s.legacyIdentitySourceOwner=forged;it->second=join;
  blockref_source_relation::Result relation;const bool prior=dx_iface::legacyBlockRefSourceRelation(data,0xB0u,0xE0u,64u*1024u*1024u,64u*1024u*1024u,relation);
  const bool refused=!dx_iface::legacyBlockRefUniqueSourceName(data,0xB0u,0xE0u,64u*1024u*1024u,64u*1024u*1024u);
  s.legacyIdentityObservation=old;s.legacyIdentityObservationOwner=oldOwner;s.legacyIdentitySourceOwner=oldSource;it->second=oldJoin;
  return prior&&refused;
 }
 static bool appidCrossAndContext(dx_data& target,const dx_data& foreign) {
  auto& a=target.m_tableDxfSource;const auto& b=foreign.m_tableDxfSource;if(!a.wholeInput||!b.wholeInput)return false;
  const auto input=a.wholeInput,owner=a.wholeInputOwner;const auto source=a.wholeSourceOwner;
  a.wholeInput=b.wholeInput;a.wholeInputOwner=b.wholeInputOwner;a.wholeSourceOwner=b.wholeSourceOwner;
  std::uint32_t provider=UINT32_MAX;const bool cross=!dx_iface::legacyCompoundAppIdSource(target,0xB0u,0xE0u,64u*1024u*1024u,64u*1024u*1024u,provider)&&provider==0u;
  a.wholeInput=input;a.wholeInputOwner=owner;a.wholeSourceOwner=source;
  auto& value=const_cast<dx_data::TableDxfSourceCapture::WholeSourceInput&>(*a.wholeInput);value.context.binary=!value.context.binary;
  provider=UINT32_MAX;const bool context=!dx_iface::legacyCompoundAppIdSource(target,0xB0u,0xE0u,64u*1024u*1024u,64u*1024u*1024u,provider)&&provider==0u;
  value.context.binary=!value.context.binary;return cross&&context;
 }
 static bool unknownAppCensusRow(dx_data& data) {
  auto& s=data.m_tableDxfSource;const auto old=s.legacyIdentityObservation,oldOwner=s.legacyIdentityObservationOwner;
  const auto oldSource=s.legacyIdentitySourceOwner;const auto it=s.legacyEntitiesCompoundJoins.find(0xE0u);
  if(!old||!old->source||it==s.legacyEntitiesCompoundJoins.end()||!it->second)return false;const auto oldJoin=it->second;
  auto forged=std::make_shared<DRW_DxfIdentitySource>(*old->source);auto found=std::find_if(forged->records.begin(),forged->records.end(),
      [](const DRW_DxfIdentitySource::Record& row){return row.kind==DRW_DxfIdentitySource::Kind::TableRecord&&row.name=="APPID";});
  if(found==forged->records.end())return false;auto row=*found;row.name="UNRECOGNIZED";forged->records.push_back(std::move(row));
  auto input=std::make_shared<dx_data::TableDxfSourceCapture::IdentityInput>(*old);input->source=forged;
  auto join=std::make_shared<dx_data::TableDxfSourceCapture::LegacyEntitiesCompoundJoin>(*oldJoin);join->identity=input;
  s.legacyIdentityObservation=input;s.legacyIdentityObservationOwner=input;s.legacyIdentitySourceOwner=forged;it->second=join;
  blockref_source_relation::Result relation;const bool prior=dx_iface::legacyBlockRefSourceRelation(data,0xB0u,0xE0u,64u*1024u*1024u,64u*1024u*1024u,relation);
  std::uint32_t provider=UINT32_MAX;const bool refused=!dx_iface::legacyCompoundAppIdSource(data,0xB0u,0xE0u,64u*1024u*1024u,64u*1024u*1024u,provider)&&provider==0u;
  s.legacyIdentityObservation=old;s.legacyIdentityObservationOwner=oldOwner;s.legacyIdentitySourceOwner=oldSource;it->second=oldJoin;return prior&&refused;
 }
 static bool uniqueContextFault(dx_data& data) {
  auto& s=data.m_tableDxfSource;if(!s.wholeInput)return false;
  auto& input=const_cast<dx_data::TableDxfSourceCapture::WholeSourceInput&>(*s.wholeInput);
  input.context.binary=!input.context.binary;
  const bool refused=!dx_iface::legacyBlockRefUniqueSourceName(data,0xB0u,0xE0u,64u*1024u*1024u,64u*1024u*1024u);
  input.context.binary=!input.context.binary;return refused;
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
 static bool appidRefuses(const dx_data& data,std::uint32_t blockId,std::uint32_t insertId,std::size_t limit,std::size_t bytesLimit) {
  std::uint32_t provider=UINT32_MAX;return !dx_iface::legacyCompoundAppIdSource(data,blockId,insertId,limit,bytesLimit,provider)&&provider==0u;
 }
 static bool appid(const dx_data& data,std::uint32_t blockId,std::uint32_t insertId,std::size_t limit,std::size_t bytesLimit,std::uint32_t expected) {
  std::uint32_t provider=0u;return dx_iface::legacyCompoundAppIdSource(data,blockId,insertId,limit,bytesLimit,provider)&&provider==expected;
 }
 static bool unique(const dx_data& data,std::uint32_t blockId,std::uint32_t insertId,std::size_t limit,std::size_t bytesLimit) {
  return dx_iface::legacyBlockRefUniqueSourceName(data,blockId,insertId,limit,bytesLimit);
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
 bool unique(const dx_data& d)const{return accepted&&DxIfaceTableSourceTestAccess::unique(d,blockId,insertId,joinWork,joinBytes);}
 bool appid(const dx_data& d,std::uint32_t expected=0xA0u)const{return accepted&&DxIfaceTableSourceTestAccess::appid(d,blockId,insertId,joinWork,joinBytes,expected);}
};
static bool write(const std::filesystem::path& path,const Tags& tags,bool bin){std::ofstream out(path,std::ios::binary);const auto word=[&](std::uint64_t n,unsigned k){for(unsigned i=0;i<k;++i)out.put(static_cast<char>(n>>(i*8u)));};
 if(bin)out.write("AutoCAD Binary DXF\r\n\x1a\0",22);
 for(const auto& t:tags)if(!bin)out<<t.code<<'\n'<<t.value<<'\n';else {word(static_cast<unsigned>(t.code),2);if(t.code>=10&&t.code<=59){double v=std::stod(t.value);std::uint64_t bits=0;std::memcpy(&bits,&v,8);word(bits,8);}else if((t.code>=60&&t.code<=79)||t.code==1070)word(static_cast<std::uint16_t>(std::stoi(t.value)),2);else{out<<t.value;out.put('\0');}}
 return out.good();}
int main(int argc,char**argv){if(argc>1){Reader real;real.enableLegacyDxfIdentityObservation();real.blockId=0x6F2u;real.insertId=0x704u;real.spoofOriginal=0x703u;real.spoof=argc>2;dx_data data;if(!real.import(argv[1],data))return 30;const bool matched=real.relation(data),unique=real.unique(data),appid=real.appid(data,0x1457u);std::cout<<"real same-read relation="<<matched<<" unique="<<unique<<" appid="<<appid<<" spoof="<<real.spoof<<"\n";return matched==!real.spoof&&unique==!real.spoof&&appid==!real.spoof?0:31;}
 std::error_code ec;const auto dir=libdxfrw_test::reserveTestScratch(std::filesystem::temp_directory_path(),"same-read-blockref",ec);if(ec)return 1;struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{dir};
 Tags doc={{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,"AC1018"},{9,"$DWGCODEPAGE"},{3,"ANSI_1252"},{0,"ENDSEC"},
 {0,"SECTION"},{2,"TABLES"},{0,"TABLE"},{2,"APPID"},{5,"A9"},{100,"AcDbSymbolTable"},{70,"1"},{0,"APPID"},{5,"A0"},{330,"A9"},{100,"AcDbSymbolTableRecord"},{100,"AcDbRegAppTableRecord"},{2,"AcDbAttr"},{70,"0"},{0,"ENDTAB"},{0,"TABLE"},{2,"BLOCK_RECORD"},{5,"B9"},{100,"AcDbSymbolTable"},{70,"1"},
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

 unsigned checks=0;for(bool binary:{false,true}){const auto path=dir/(binary?"binary.dxf":"ascii.dxf");if(!write(path,doc,binary))return 2;Reader reader;reader.enableLegacyDxfIdentityObservation();dx_data good;if(!reader.import(path.string(),good)||!reader.relation(good)||!reader.unique(good)||!reader.appid(good)||!DxIfaceTableSourceTestAccess::unknownAppCensusRow(good)||!reader.appid(good)||!DxIfaceTableSourceTestAccess::uniqueContextFault(good)||!reader.unique(good)||!DxIfaceTableSourceTestAccess::unknownCensusRow(good)||!reader.unique(good))return 3;reader.joinWork=0;if(reader.relation(good))return 15;reader.joinWork=64u*1024u*1024u;if(!reader.relation(good))return 16;reader.joinBytes=0;if(reader.relation(good))return 17;reader.joinBytes=64u*1024u*1024u;if(!reader.relation(good)||!DxIfaceTableSourceTestAccess::contextFault(good)||!reader.relation(good)||!DxIfaceTableSourceTestAccess::manySourceNoDelivery(good)||!reader.relation(good))return 18;
  for(int dimension=0;dimension<2;++dimension){std::size_t lo=0,hi=64u*1024u*1024u;while(lo<hi){const auto mid=lo+(hi-lo)/2u;
    if(dimension==0)reader.joinWork=mid;else reader.joinBytes=mid;if(reader.relation(good))hi=mid;else lo=mid+1u;}
   if(!lo)return 26;if(dimension==0)reader.joinWork=lo;else reader.joinBytes=lo;if(!reader.relation(good))return 27;
   if(dimension==0)reader.joinWork=lo-1u;else reader.joinBytes=lo-1u;if(reader.relation(good))return 28;
   reader.joinWork=64u*1024u*1024u;reader.joinBytes=64u*1024u*1024u;if(!reader.relation(good))return 29;}
  ++checks;
  dx_data other;if(!reader.import(path.string(),other)||!reader.relation(other)||!reader.relation(good)||!DxIfaceTableSourceTestAccess::crossRead(good,other)||!reader.relation(good)||!DxIfaceTableSourceTestAccess::appidCrossAndContext(good,other)||!reader.appid(good))return 4;++checks;
  for(int mode=1;mode<=4;++mode){Reader bad;bad.enableLegacyDxfIdentityObservation();bad.mode=mode;dx_data data;if(!bad.import(path.string(),data)||bad.relation(data))return 5;bad.mode=0;dx_data retry;if(!bad.import(path.string(),retry)||!bad.relation(retry))return 6;++checks;}
  // Each mutation is refused before a provider is published; retries use
  // fresh data with the same adapter, regardless of whether import itself fails.
  for(unsigned variant=0u;variant<7u;++variant){auto bad=doc;
   auto firstEnd=std::find_if(bad.begin(),bad.end(),[](const Tag& t){return t.code==0&&t.value=="ENDTAB";});if(firstEnd==bad.end())return 87;
   if(variant==0u){auto table=std::find_if(bad.begin(),bad.end(),[](const Tag& t){return t.code==2&&t.value=="APPID";});
    if(table==bad.end()||table==bad.begin())return 88;bad.erase(table-1,firstEnd+1);}
   else if(variant==1u){Tags second={{0,"TABLE"},{2,"APPID"},{5,"A8"},{100,"AcDbSymbolTable"},{70,"0"},{0,"ENDTAB"}};bad.insert(firstEnd+1,second.begin(),second.end());}
   else if(variant==2u){Tags duplicate={{0,"APPID"},{5,"A0"},{330,"A9"},{100,"AcDbSymbolTableRecord"},{100,"AcDbRegAppTableRecord"},{2,"AcDbAttr"},{70,"0"}};bad.insert(firstEnd,duplicate.begin(),duplicate.end());}
   else {auto attr=std::find_if(bad.begin(),bad.end(),[](const Tag& t){return t.code==0&&t.value=="ATTRIB";});if(attr==bad.end())return 89;
    auto app=std::find_if(attr+1,bad.end(),[](const Tag& t){return t.code==1001&&t.value=="AcDbAttr";});if(app==bad.end())return 90;
    if(variant==3u)app->value="OTHER";else if(variant==4u)bad.erase(app);else if(variant==5u)bad.insert(app+1,{1001,"AcDbAttr"});
    else {auto value=*app;bad.erase(app);auto sub=std::find_if(attr+1,bad.end(),[](const Tag& t){return t.code==100&&t.value=="AcDbAttribute";});if(sub==bad.end())return 91;bad.insert(sub,value);}}
   if(!write(path,bad,binary))return 92;Reader rejected;rejected.enableLegacyDxfIdentityObservation();dx_data damaged;
   if(rejected.import(path.string(),damaged)&&!DxIfaceTableSourceTestAccess::appidRefuses(damaged,0xB0u,0xE0u,64u*1024u*1024u,64u*1024u*1024u))return 93;
   if(!write(path,doc,binary))return 94;dx_data retry;if(!rejected.import(path.string(),retry)||!rejected.appid(retry))return 95;}
  // Resolve the selected 1001 spelling only against the complete APPID table.
  for(int variant=0;variant<4;++variant){auto extra=doc;auto end=std::find_if(extra.begin(),extra.end(),[](const Tag& t){return t.code==0&&t.value=="ENDTAB";});
   if(end==extra.end())return 65;Tags row={{0,"APPID"},{5,"A1"},{330,"A9"},{100,"AcDbSymbolTableRecord"},{100,"AcDbRegAppTableRecord"},{2,variant==0?"AcDbAttr":variant==1?"acdbattr":variant==2?"bad name":"nonASCII-Ω"},{70,"0"}};
   extra.insert(end,row.begin(),row.end());if(!write(path,extra,binary))return 66;Reader ambiguous;ambiguous.enableLegacyDxfIdentityObservation();dx_data duplicate;
   if(!ambiguous.import(path.string(),duplicate)||!ambiguous.relation(duplicate)||!DxIfaceTableSourceTestAccess::appidRefuses(duplicate,0xB0u,0xE0u,64u*1024u*1024u,64u*1024u*1024u))return 67;
   if(!write(path,doc,binary))return 68;dx_data fixed;if(!ambiguous.import(path.string(),fixed)||!ambiguous.appid(fixed))return 69;}
  auto wrongOwner=doc;bool changedOwner=false;for(std::size_t i=0;i+2u<wrongOwner.size();++i)if(wrongOwner[i].code==0&&wrongOwner[i].value=="APPID"){
   for(std::size_t j=i+1u;j<wrongOwner.size()&&wrongOwner[j].code!=0;++j)if(wrongOwner[j].code==330){wrongOwner[j].value="B9";changedOwner=true;break;}break;}
  if(!changedOwner||!write(path,wrongOwner,binary))return 78;Reader ownerReader;ownerReader.enableLegacyDxfIdentityObservation();dx_data wrongOwnerData;
  if(!ownerReader.import(path.string(),wrongOwnerData)||!ownerReader.relation(wrongOwnerData)||!DxIfaceTableSourceTestAccess::appidRefuses(wrongOwnerData,0xB0u,0xE0u,64u*1024u*1024u,64u*1024u*1024u))return 79;
  if(!write(path,doc,binary))return 80;dx_data fixedOwner;if(!ownerReader.import(path.string(),fixedOwner)||!ownerReader.appid(fixedOwner))return 81;
  auto alienApp=doc;auto entityStart=std::find_if(alienApp.begin(),alienApp.end(),[](const Tag& t){return t.code==2&&t.value=="ENTITIES";});
  if(entityStart==alienApp.end())return 82;Tags alienRecord={{0,"APPID"},{5,"A1"},{330,"A9"},{100,"AcDbRegAppTableRecord"},{2,"AcDbAttr"}};
  alienApp.insert(entityStart+1,alienRecord.begin(),alienRecord.end());if(!write(path,alienApp,binary))return 83;Reader foreignAppReader;foreignAppReader.enableLegacyDxfIdentityObservation();dx_data foreignAppData;
  if(!foreignAppReader.import(path.string(),foreignAppData)||!foreignAppReader.relation(foreignAppData)||!DxIfaceTableSourceTestAccess::appidRefuses(foreignAppData,0xB0u,0xE0u,64u*1024u*1024u,64u*1024u*1024u))return 84;
  if(!write(path,doc,binary))return 85;dx_data fixedForeign;if(!foreignAppReader.import(path.string(),fixedForeign)||!foreignAppReader.appid(fixedForeign))return 86;
  auto missingApp=doc;bool changedApp=false;for(std::size_t i=0;i+1u<missingApp.size();++i)if(missingApp[i].code==100&&missingApp[i].value=="AcDbRegAppTableRecord"&&missingApp[i+1u].code==2){missingApp[i+1u].value="OTHER";changedApp=true;break;}
  if(!changedApp||!write(path,missingApp,binary))return 70;Reader missingReader;missingReader.enableLegacyDxfIdentityObservation();dx_data missingData;
  if(!missingReader.import(path.string(),missingData)||!missingReader.relation(missingData)||!DxIfaceTableSourceTestAccess::appidRefuses(missingData,0xB0u,0xE0u,64u*1024u*1024u,64u*1024u*1024u))return 71;
  if(!write(path,doc,binary))return 72;dx_data fixedApp;if(!missingReader.import(path.string(),fixedApp)||!missingReader.appid(fixedApp))return 73;
  for(int dimension=0;dimension<2;++dimension){std::size_t lo=0,hi=64u*1024u*1024u;while(lo<hi){const auto mid=lo+(hi-lo)/2u;
   if(dimension==0)reader.joinWork=mid;else reader.joinBytes=mid;if(reader.appid(good))hi=mid;else lo=mid+1u;}
   if(!lo)return 74;if(dimension==0)reader.joinWork=lo;else reader.joinBytes=lo;if(!reader.appid(good))return 75;
   if(dimension==0)reader.joinWork=lo-1u;else reader.joinBytes=lo-1u;if(reader.appid(good))return 76;
   reader.joinWork=64u*1024u*1024u;reader.joinBytes=64u*1024u*1024u;if(!reader.appid(good))return 77;}
  // Count every original BLOCK_RECORD, not just captured selected resources.
  for(int variant=0;variant<4;++variant){auto extra=doc;auto end=std::find_if(extra.begin(),extra.end(),[](const Tag& t){return t.code==0&&t.value=="ENDTAB";});if(end!=extra.end()) end=std::find_if(end+1,extra.end(),[](const Tag& t){return t.code==0&&t.value=="ENDTAB";});
   if(end==extra.end())return 43;Tags row={{0,"BLOCK_RECORD"},{5,"B2"},{330,"B9"},{100,"AcDbSymbolTableRecord"},{100,"AcDbBlockTableRecord"},{2,variant==0?"TEST_BLOCK":variant==1?"test_block":variant==2?"bad name":"nonASCII-Ω"},{340,"0"}};
   extra.insert(end,row.begin(),row.end());if(!write(path,extra,binary))return 44;Reader ambiguous;ambiguous.enableLegacyDxfIdentityObservation();dx_data duplicated;
   if(!ambiguous.import(path.string(),duplicated)||!ambiguous.relation(duplicated)||ambiguous.unique(duplicated))return 45;
   if(!write(path,doc,binary))return 46;dx_data fixed;if(!ambiguous.import(path.string(),fixed)||!ambiguous.unique(fixed))return 47;}
  // Unknown table rows with target-like group 2 must not evade the census.
  auto unknown=doc;auto endUnknown=std::find_if(unknown.begin(),unknown.end(),[](const Tag& t){return t.code==0&&t.value=="ENDTAB";});if(endUnknown!=unknown.end()) endUnknown=std::find_if(endUnknown+1,unknown.end(),[](const Tag& t){return t.code==0&&t.value=="ENDTAB";});
  if(endUnknown==unknown.end())return 60;Tags unknownRow={{0,"UNRECOGNIZED"},{5,"B2"},{330,"B9"},{100,"AcDbBlockTableRecord"},{2,"test_block"}};
  unknown.insert(endUnknown,unknownRow.begin(),unknownRow.end());if(!write(path,unknown,binary))return 61;Reader unknownReader;unknownReader.enableLegacyDxfIdentityObservation();dx_data unknownData;
  if(unknownReader.import(path.string(),unknownData)||unknownReader.unique(unknownData))return 62;
  if(!write(path,doc,binary))return 63;dx_data correctedUnknown;if(!unknownReader.import(path.string(),correctedUnknown)||!unknownReader.unique(correctedUnknown))return 64;
  // An unclassified BLOCK_RECORD outside TABLES cannot be ignored.
  auto foreign=doc;auto ent=std::find_if(foreign.begin(),foreign.end(),[](const Tag& t){return t.code==2&&t.value=="ENTITIES";});
  if(ent==foreign.end())return 55;Tags alien={{0,"BLOCK_RECORD"},{5,"B2"},{330,"B9"},{100,"AcDbSymbolTableRecord"},{100,"AcDbBlockTableRecord"},{2,"OTHER"},{340,"0"}};
  foreign.insert(ent+1,alien.begin(),alien.end());if(!write(path,foreign,binary))return 56;Reader foreignReader;foreignReader.enableLegacyDxfIdentityObservation();dx_data foreignData;
  if(!foreignReader.import(path.string(),foreignData)||!foreignReader.relation(foreignData)||foreignReader.unique(foreignData))return 57;
  if(!write(path,doc,binary))return 58;dx_data restored;if(!foreignReader.import(path.string(),restored)||!foreignReader.unique(restored))return 59;
  // ASCII case comparison is intentional, independent native collation is not claimed.
  auto mixed=doc;bool changed=false;for(std::size_t i=0;i+3u<mixed.size();++i)if(mixed[i].code==100&&mixed[i].value=="AcDbBlockReference"){
   for(std::size_t j=i+1u;j<mixed.size()&&mixed[j].code!=0;++j)if(mixed[j].code==2){mixed[j].value="test_block";changed=true;break;}break;}
  if(!changed||!write(path,mixed,binary))return 48;Reader caseReader;caseReader.enableLegacyDxfIdentityObservation();dx_data caseData;
  if(!caseReader.import(path.string(),caseData)||!caseReader.unique(caseData))return 49;if(!write(path,doc,binary))return 50;
  for(int dimension=0;dimension<2;++dimension){std::size_t lo=0,hi=64u*1024u*1024u;while(lo<hi){const auto mid=lo+(hi-lo)/2u;
   if(dimension==0)reader.joinWork=mid;else reader.joinBytes=mid;if(reader.unique(good))hi=mid;else lo=mid+1u;}
   if(!lo)return 51;if(dimension==0)reader.joinWork=lo;else reader.joinBytes=lo;if(!reader.unique(good))return 52;
   if(dimension==0)reader.joinWork=lo-1u;else reader.joinBytes=lo-1u;if(reader.unique(good))return 53;
   reader.joinWork=64u*1024u*1024u;reader.joinBytes=64u*1024u*1024u;if(!reader.unique(good))return 54;}
  auto altered=doc;for(auto& t:altered)if(t.code==331&&t.value=="E0")t.value="E3";if(!write(path,altered,binary))return 7;Reader attack;attack.enableLegacyDxfIdentityObservation();attack.spoof=true;dx_data rejected;if(!attack.import(path.string(),rejected)||!DxIfaceTableSourceTestAccess::spoofPriorPass(rejected)||attack.relation(rejected))return 8;++checks;
  if(!write(path,doc,binary))return 9;attack.spoof=false;dx_data retry;if(!attack.import(path.string(),retry)||!attack.relation(retry))return 10;++checks;
  Reader limited;limited.enableLegacyDxfIdentityObservation();limited.limits.fields=0;dx_data noFields;if(!limited.import(path.string(),noFields)||limited.relation(noFields))return 11;limited.limits={};dx_data recovered;if(!limited.import(path.string(),recovered)||!limited.relation(recovered))return 12;++checks;
  if(!reader.import(path.string(),other)||reader.relation(other))return 13;dx_data fresh;if(!reader.import(path.string(),fresh)||!reader.relation(fresh))return 14;++checks;
 }
 std::cout<<"same-read source-only ascii-binary=2 controls="<<checks<<" callback-spoof=2 quota-retry=2 join-work-byte-retry=4 same-data-refusal=2\n";return 0;}
