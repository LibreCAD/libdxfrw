// SPDX-License-Identifier: GPL-2.0-or-later
// Locally authored controls; not full IMAGE output or native semantic proof.
bool imageDefIdentityTests(const std::filesystem::path& path) {
 using Source=DRW_DxfIdentitySource;
 struct Probe final:dx_iface {
  unsigned images=0u,definitions=0u,xrecords=0u,censuses=0u;std::uint32_t reference=0u,defHandle=0u;std::string filename;Source census;
  void addImage(const DRW_Image* v)override{if(v){++images;reference=v->ref;}dx_iface::addImage(v);}
  void linkImage(const DRW_ImageDef* v)override{if(v){++definitions;defHandle=v->handle;filename=v->name;}dx_iface::linkImage(v);}
  void addXRecord(const DRW_XRecord& v)override{++xrecords;dx_iface::addXRecord(v);}
  void addDxfIdentitySource(const Source& v)override{++censuses;census=v;dx_iface::addDxfIdentitySource(v);}
 };
 const auto check=[](bool ok,const char* msg){if(!ok)std::cerr<<msg<<'\n';return ok;};
 const auto document=[](const std::string& version,const char* imageHandle,bool imageFirst,bool app) {
  Tags tags{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"},
   {0,"SECTION"},{2,"ENTITIES"},{0,"IMAGE"},{5,"30"},{330,"0"},{100,"AcDbEntity"},{8,"0"},
   {100,"AcDbRasterImage"},{90,"0"},{10,"1"},{20,"2"},{30,"0"},{11,"1"},{21,"0"},{31,"0"},
   {12,"0"},{22,"1"},{32,"0"},{13,"8"},{23,"8"},{340,"42"},{70,"3"},{280,"0"},
   {281,"50"},{282,"50"},{283,"0"},{0,"ENDSEC"},{0,"SECTION"},{2,"OBJECTS"},
   {0,"DICTIONARY"},{5,"40"},{330,"0"},{100,"AcDbDictionary"},{280,"0"},{281,"1"}};
  Tags image{{0,"IMAGEDEF"}};if(imageHandle)image.push_back({5,imageHandle});
  if(app)image.insert(image.end(),{{102,"{CUSTOM"},{5,"41"},{102,"}"}});
  image.insert(image.end(),{{330,"40"},{100,"AcDbRasterImageDef"},{90,"0"},{1,"image.png"},{10,"8"},{20,"8"},{11,"1"},{21,"1"},{280,"1"},{281,"0"}});
  const Tags other{{0,"XRECORD"},{5,"41"},{330,"40"},{100,"AcDbXrecord"},{280,"1"},{1,"payload"}};
  const auto add=[&](const Tags& values){tags.insert(tags.end(),values.begin(),values.end());};
  if(imageFirst){add(image);add(other);}else{add(other);add(image);}tags.insert(tags.end(),{{0,"ENDSEC"},{0,"EOF"}});return tags;
 };
 unsigned valid=0u,duplicates=0u,retries=0u,empty=0u,invalid=0u;
 for(const std::string version:{"AC1018","AC1032"})for(bool binary:{false,true})for(bool first:{false,true}) {
  const auto clean=document(version,"42",first,true);if(!write(path,clean,binary))return false;
  Probe reader;dx_data data;if(!check(reader.fileImport(path.string(),&data,false)&&reader.images==1u&&reader.reference==0x42u
   &&reader.definitions==1u&&reader.defHandle==0x42u&&reader.filename=="image.png"&&reader.xrecords==1u&&reader.censuses==1u
   &&reader.census.status==Source::Status::Complete,"IMAGEDEF valid IMAGE consumer and application payload"))return false;
  unsigned outer=0u,application=0u;
  for(const auto& c:reader.census.candidates)if(c.record<reader.census.records.size()&&reader.census.records[c.record].name=="IMAGEDEF") {
   if(c.code==5&&c.lexeme=="42"&&c.scope==Source::Scope::Outer&&c.admission==Source::Admission::Accepted)++outer;
   else if(c.code==5&&c.lexeme=="41"&&c.scope==Source::Scope::Application&&c.admission==Source::Admission::Unobserved)++application;
   else return false;
  }
  if(!check(outer==1u&&application==1u,"IMAGEDEF only registers the outer self"))return false;++valid;
  const auto duplicate=document(version,"41",first,false);if(!write(path,duplicate,binary))return false;
  Probe bad;dx_data failed;if(!check(!bad.fileImport(path.string(),&failed,false)&&bad.censuses==0u,
   "IMAGEDEF duplicate object handle fails without source publication"))return false;
  ++duplicates;
  if(!write(path,clean,binary))return false;dx_data retry;if(!check(bad.fileImport(path.string(),&retry,false)
   &&bad.definitions>=1u&&bad.censuses==1u,"IMAGEDEF duplicate corrected same-adapter retry"))return false;
  ++retries;
 }
 for(const std::string version:{"AC1018","AC1032"})for(bool binary:{false,true})for(bool present:{false,true}) {
  const auto tags=document(version,present?"0":nullptr,true,false);if(!write(path,tags,binary))return false;Probe reader;dx_data data;
  if(!check(reader.fileImport(path.string(),&data,false)&&reader.definitions==1u&&reader.defHandle==0u,
   "IMAGEDEF absent and explicit-null handles retain compatible callback"))return false;
  ++empty;
  const auto malformed=document(version,"INVALID",true,false);if(!write(path,malformed,binary))return false;Probe bad;dx_data rejected;
  if(!check(!bad.fileImport(path.string(),&rejected,false),"IMAGEDEF malformed handle remains rejected"))return false;++invalid;
 }
 std::cout<<"IMAGEDEF identity routes="<<valid<<" duplicate objects="<<duplicates<<" retries="<<retries
  <<" absent/null="<<empty<<" malformed="<<invalid<<'\n';
 return valid==8u&&duplicates==8u&&retries==8u&&empty==8u&&invalid==8u;
}
