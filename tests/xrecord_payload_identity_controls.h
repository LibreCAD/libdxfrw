// SPDX-License-Identifier: GPL-2.0-or-later
// Candidate follow-up for tests/identity_source_controls.h after the raw fix.
bool xrecordPayloadIdentityTests(const std::filesystem::path& path) {
 using Source=DRW_DxfIdentitySource;using Scope=Source::Scope;
 struct Probe final:dx_iface {
  bool sourceOptIn=true;unsigned typed=0u,raw=0u,census=0u;DRW_XRecord model;DRW_RawDxfObject original;Source identity;
  bool wantsDxfResourceSource()const override{return sourceOptIn&&dx_iface::wantsDxfResourceSource();}
  void addXRecord(const DRW_XRecord& value)override{++typed;model=value;dx_iface::addXRecord(value);}
  void addRawDxfObject(const DRW_RawDxfObject& value)override {if(value.name=="XRECORD"){++raw;original=value;}dx_iface::addRawDxfObject(value);}
  void addDxfIdentitySource(const Source& source)override {++census;identity=source;dx_iface::addDxfIdentitySource(source);}
 };
 const auto check=[](bool ok,const char* msg){if(!ok)std::cerr<<msg<<'\n';return ok;};
 unsigned successes=0u,duplicates=0u,retries=0u;
 for(const std::string version:{"AC1018","AC1032"})for(bool binary:{false,true})for(bool headerOwner:{false,true})for(bool sourceOptIn:{false,true}) {
  Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"},
   {0,"SECTION"},{2,"OBJECTS"},{0,"DICTIONARY"},{5,"40"},{330,"0"},{100,"AcDbDictionary"},{280,"0"},{281,"1"},
   {0,"XRECORD"},{5,"41"}};
  if(headerOwner)tags.push_back({330,"0"});
  tags.insert(tags.end(),{{100,"AcDbXrecord"},{280,"1"},{5,"41"},{5,"42"},{105,"43"},{330,"44"},
   {102,"{ATTRRECORD"},{102,"ATTRRECORD}"},{1001,"APP"},{1000,"payload"},{0,"ENDSEC"},
   {0,"SECTION"},{2,"ENTITIES"},{0,"POINT"},{5,"42"},{330,"0"},{100,"AcDbEntity"},{8,"0"},{100,"AcDbPoint"},{10,"0"},{20,"0"},{30,"0"},{0,"ENDSEC"},{0,"EOF"}});
  if(!write(path,tags,binary))return false;Probe reader;reader.sourceOptIn=sourceOptIn;dx_data data;
  if(!check(reader.fileImport(path.string(),&data,false)&&reader.typed==1u&&reader.raw==1u&&reader.census==1u
   &&reader.identity.status==Source::Status::Complete&&reader.model.handle==0x41u&&reader.model.parentHandle==0u
   &&reader.original.handle==0x41u&&reader.original.parentHandle==0u,
   "XRECORD payload IDs must not register as self or infer a parent"))return false;
  const std::vector<std::pair<int,std::uint32_t>> expected{{5,0x41u},{5,0x42u},{105,0x43u},{330,0x44u}};
  if(!check(reader.model.m_handleValues==expected,"XRECORD payload handles retain code/order/value"))return false;
  unsigned own=0u,body=0u,point=0u;
  for(const auto& c:reader.identity.candidates) {
   if(c.record>=reader.identity.records.size())return false;const auto& rec=reader.identity.records[c.record];
   if(rec.name=="XRECORD"&&c.scope==Scope::Outer&&c.lexeme=="41")++own;
   else if(rec.name=="XRECORD"&&c.scope==Scope::OpaqueBody&&(c.lexeme=="41"||c.lexeme=="42"||c.lexeme=="43")) {
    if(c.admission!=Source::Admission::Unobserved)return false;++body;
   } else if(rec.name=="POINT"&&c.scope==Scope::Outer&&c.lexeme=="42"&&c.admission==Source::Admission::Accepted)++point;
  }
  if(!check(own==1u&&body==3u&&point==1u,"XRECORD payload candidates do not reserve later entity identity"))return false;
  const auto report=dx_iface::identitySourceReport(data);
  if(!check(version=="AC1018"?report.state==dx_iface::IdentitySourceState::NotCaptured:
   report.state==dx_iface::IdentitySourceState::Unqualified&&!report.outputEligible&&!report.ownerTargetsResolved,
   "XRECORD payload read does not qualify source namespace"))return false;
  ++successes;
  auto bad=tags;auto pointTag=std::find_if(bad.begin(),bad.end(),[](const Tag& t){return t.code==0&&t.value=="POINT";});
  auto id=std::find_if(pointTag,bad.end(),[](const Tag& t){return t.code==5;});id->value="41";
  if(!write(path,bad,binary))return false;Probe rejected;rejected.sourceOptIn=sourceOptIn;dx_data failed;
  if(!check(rejected.fileImport(path.string(),&failed,false)&&rejected.census==1u,
   "duplicate typed entity remains a compatible import"))return false;
  unsigned duplicateCandidates=0u;
  for(const auto& c:rejected.identity.candidates)
   if(c.record<rejected.identity.records.size()&&rejected.identity.records[c.record].name=="POINT"
      &&c.scope==Scope::Outer&&c.lexeme=="41"&&c.admission==Source::Admission::Duplicate)++duplicateCandidates;
  const auto duplicateReport=dx_iface::identitySourceReport(failed);
  if(!check(duplicateCandidates==1u&&(version=="AC1018"?duplicateReport.state==dx_iface::IdentitySourceState::NotCaptured:
   duplicateReport.state==dx_iface::IdentitySourceState::Unqualified&&duplicateReport.duplicateAdmissions>=1u),
   "duplicate outer self cannot qualify the identity namespace"))return false;
  ++duplicates;
  if(!write(path,tags,binary))return false;dx_data retry;rejected.typed=rejected.raw=rejected.census=0u;
  if(!check(rejected.fileImport(path.string(),&retry,false)&&rejected.typed==1u&&rejected.raw==1u,
   "XRECORD duplicate corrected same-adapter retry"))return false;
  ++retries;
 }
 std::cout<<"XRECORD body identity routes="<<successes<<" true duplicates="<<duplicates<<" retries="<<retries<<'\n';
 return successes==16u&&duplicates==16u&&retries==16u;
}
