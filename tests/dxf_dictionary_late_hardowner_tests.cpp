// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include "intern/dxfwriter.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <vector>
struct Capture : LC3CompatCheck {
 void addDictionary(const DRW_Dictionary& d) override { records.push_back(d); }
 void addXRecord(const DRW_XRecord& x) override { targets.push_back(x); }
 void addNamedObjectDictionary(const DRW_Dictionary& d) override {root=d;records.push_back(d);}
 DRW_Dictionary root; std::vector<DRW_XRecord> targets;
 void addDictionaryWithDefault(const DRW_DictionaryWithDefault&) override {++defaults;}
 void addRawDxfObject(const DRW_RawDxfObject& r) override {raw.push_back(r);}
 int defaults=0; std::vector<DRW_Dictionary> records; std::vector<DRW_RawDxfObject> raw;
};
struct Export : LC3CompatCheck {
 explicit Export(dxfRW& writer):out(writer) {}
 void writeObjects() override { DRW_XRecord a,b; a.handle=0x83; b.handle=0x84; a.parentHandle=b.parentHandle=0x78;ok=out.writeDictionary(&dict)&&out.writeXRecord(&a)&&out.writeXRecord(&b); }
 bool finalizeDxfWrite() override {return ok;}
 dxfRW& out; DRW_Dictionary dict; bool ok=false;
};
static bool input(const std::filesystem::path& p,bool binary,int pos,int flag=1,bool wdf=false) {
 std::ofstream f(p,std::ios::binary|std::ios::trunc);
 if(binary) f<<"AutoCAD Binary DXF\r\n"<<char(26)<<'\0';
 std::unique_ptr<dxfWriter> w=binary?std::unique_ptr<dxfWriter>(std::make_unique<dxfWriterBinary>(&f)):std::unique_ptr<dxfWriter>(std::make_unique<dxfWriterAscii>(&f));w->setVersion("AC1027",true);
 if(!w->writeString(0,"SECTION")||!w->writeString(2,"HEADER")||!w->writeString(9,"$ACADVER")||!w->writeString(1,"AC1027")||!w->writeString(0,"ENDSEC")||!w->writeString(0,"SECTION")||!w->writeString(2,"OBJECTS")||!w->writeString(0,"DICTIONARY")||!w->writeString(5,"C")||!w->writeString(330,"0")||!w->writeString(100,"AcDbDictionary")||!w->writeInt16(280,0)||!w->writeInt16(281,1)||!w->writeString(0,wdf?"ACDBDICTIONARYWDFLT":"DICTIONARY")||!w->writeString(5,"78")||!w->writeString(330,"C")||!w->writeString(100,"AcDbDictionary")||!w->writeInt16(281,1))return false;
 if(pos==0&&!w->writeInt16(280,flag))return false;
 if(!w->writeString(3,"first"))return false;
 if(pos==1&&!w->writeInt16(280,flag))return false;
 if(!w->writeString(360,"83"))return false;
 if(pos==2&&!w->writeInt16(280,flag))return false;
 if(!w->writeString(3,"second")||!w->writeString(350,"84"))return false;
 if(pos==5&&(!w->writeString(100,"AcDbOther")||!w->writeInt16(280,flag)))return false;
 if(pos==6&&(!w->writeString(100,"AcDbOther")||!w->writeString(100,"AcDbDictionary")||!w->writeInt16(280,flag)))return false;
 if(pos==7&&(!w->writeString(102,"{APP")||!w->writeString(100,"AcDbOther")||!w->writeInt16(280,0)||!w->writeString(102,"}")||!w->writeInt16(280,flag)))return false;
 if(pos==3&&!w->writeInt16(280,flag))return false;
 if(pos==4&&(!w->writeInt16(280,0)||!w->writeInt16(280,flag)))return false;
 if(pos==8){for(int i=0;i<1500;++i)if(!w->writeString(3,"bulk_"+std::to_string(i))||!w->writeString(350,"ABC"))return false;for(int i=0;i<1500;++i)if(!w->writeInt16(280,1))return false;}
 if(wdf&&(!w->writeString(100,"AcDbDictionaryWithDefault")||!w->writeString(340,"83")))return false;
 return w->writeString(0,"ENDSEC")&&w->writeString(0,"EOF")&&!w->hasWriteError();
}
static bool good(const DRW_Dictionary& d){return d.handle==0x78&&d.hardOwner==1&&d.cloning==1&&d.m_entries.size()==2&&d.m_entries[0].m_name=="first"&&d.m_entries[0].m_handle==0x83&&d.m_entries[0].m_dxfHandleCode==DRW_Dictionary::Entry::DxfHandleCode::HardOwner&&d.m_entries[1].m_name=="second"&&d.m_entries[1].m_handle==0x84&&d.m_entries[1].m_dxfHandleCode==DRW_Dictionary::Entry::DxfHandleCode::SoftOwner;}
int main(int argc,char**argv){if(argc==2){Capture c;dxfRW r(argv[1]);if(!r.read(&c,false)||c.records.size()!=2||c.records[1].handle!=0x78||c.records[1].hardOwner!=1||c.records[1].m_entries.size()!=1||c.records[1].m_entries[0].m_handle!=0x83||c.records[1].m_entries[0].m_name!="ACAD_XREC_ROUNDTRIP")return 20;return 0;}if(argc!=1)return 21;
 auto dir=std::filesystem::temp_directory_path()/("dictionary-late-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(dir);struct Cleanup {std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} clean{dir};
 for(bool binary:{false,true}){auto p=dir/(binary?"b.dxf":"a.dxf"),out=dir/(binary?"bo.dxf":"ao.dxf");
  for(int pos:{0,2,3,4,6,7}) {if(!input(p,binary,pos))return 1;Capture c;dxfRW r(p.string().c_str());if(!r.read(&c,false)||c.records.size()!=2||!good(c.records[1])||c.raw.size()!=1)return 2;std::vector<int> codes;for(const auto& g:c.raw[0].groups)codes.push_back(g.code());auto first=std::find(codes.begin(),codes.end(),360),flag=std::find(codes.begin(),codes.end(),280);if(first==codes.end()||flag==codes.end()||(pos==0 ? flag>first : flag<first))return 2;DRW_Dictionary copy(c.records[1]),assigned;assigned=copy;if(!good(assigned))return 3;dxfRW writer(out.string().c_str());Export e(writer);e.dict=assigned;writer.setRootDictEntries({{"TEST_DICT","78"}});if(!writer.write(&e,DRW::AC1027,binary)||!e.ok)return 4;Capture decoded;dxfRW check(out.string().c_str());if(!check.read(&decoded,false))return 5;bool found=false;for(const auto& d:decoded.records)if(d.handle==0x78){if(found||!good(d))return 5;found=true;}if(!found||decoded.targets.size()!=2)return 5;bool rootEdge=false;for(const auto& entry:decoded.root.m_entries)if(entry.m_name=="TEST_DICT"&&entry.m_handle==0x78)rootEdge=true; if(!rootEdge)return 5;for(const auto& target:decoded.targets)if((target.handle!=0x83&&target.handle!=0x84)||target.parentHandle!=0x78)return 5;}
  if(!input(p,binary,8))return 14;Capture bulk;dxfRW many(p.string().c_str());if(!many.read(&bulk,false)||bulk.records.size()!=2||bulk.records[1].m_entries.size()!=1502||bulk.records[1].hardOwner!=1)return 15;
  if(!input(p,binary,5))return 12;Capture wrongSubclass;dxfRW wrong(p.string().c_str());if(wrong.read(&wrongSubclass,false)||wrongSubclass.records.size()!=1||!wrongSubclass.raw.empty())return 13;
  if(!input(p,binary,3,1,true))return 10;Capture subtype;dxfRW sub(p.string().c_str());if(sub.read(&subtype,false)||subtype.defaults||subtype.records.size()!=1)return 11;
  for(auto item:std::vector<std::pair<int,int>>{{1,1},{3,2},{3,-1}}){if(!input(p,binary,item.first,item.second))return 6;Capture bad;dxfRW retry(p.string().c_str());if(retry.read(&bad,false)||bad.records.size()!=1||!bad.raw.empty())return 7;if(!input(p,binary,3))return 8;Capture fixed;if(!retry.read(&fixed,false)||fixed.records.size()!=2||!good(fixed.records[1]))return 9;}
 }
 return 0;
}
