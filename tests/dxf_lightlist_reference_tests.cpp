// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include "intern/dxfwriter.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
namespace {
struct Reader final : LC3CompatCheck {
 void addLight(const DRW_Light& v) override { lights.push_back(v); }
 void addLightList(const DRW_LightList& v) override { lists.push_back(v); }
 void addRawDxfObject(const DRW_RawDxfObject& v) override {if(v.name=="LIGHTLIST")raw.push_back(v);}
 std::vector<DRW_Light> lights;std::vector<DRW_LightList> lists;std::vector<DRW_RawDxfObject> raw;
};
bool make(const std::filesystem::path& path,bool binary,int mode) {
 std::ofstream f(path,std::ios::binary);if(!f)return false;
 if(binary){const char sentinel[]="AutoCAD Binary DXF\r\n\x1a";f.write(sentinel,sizeof(sentinel));}
 std::unique_ptr<dxfWriter> w=binary?std::unique_ptr<dxfWriter>(new dxfWriterBinary(&f)):std::unique_ptr<dxfWriter>(new dxfWriterAscii(&f));
 const auto str=[&](int c,const char* x){return w->writeString(c,x);};
 const auto n=[&](int c,int x){return w->writeInt32(c,x);};
 if(!str(0,"SECTION")||!str(2,"HEADER")||!str(9,"$ACADVER")||!str(1,"AC1027")||!str(0,"ENDSEC")
 ||!str(0,"SECTION")||!str(2,"ENTITIES")||!str(0,"LIGHT")||!str(5,"2F")||!str(100,"AcDbEntity")
 ||!str(8,"0")||!str(100,"AcDbLight")||!n(90,0)||!str(1,"LOCAL_LIGHT")||!w->writeInt16(70,2))return false;
 if(!str(0,"ENDSEC")||!str(0,"SECTION")||!str(2,"OBJECTS")||!str(0,"LIGHTLIST"))return false;
 if(mode!=13 && !str(5,"F000"))return false;
 if(!str(102,"{ACAD_REACTORS")||!str(330,"30")||!str(102,"}")
 ||!str(330,"30")||!str(100,"AcDbLightList")||!n(90,1)||!n(90,mode==10?2:(mode==11 || mode==14)?0:1))return false;
 if(mode==12 && !n(90,1))return false;
 if(mode==9 && !str(1,"ORPHAN"))return false;
 if(mode!=14 && !str(5,mode==3?"NOHEX":mode==6?"0":"2F"))return false;
 if(mode==4 && !str(5,"2F"))return false;
 if(mode==8 && (!str(100,"AcDbOther") || !str(100,"AcDbLightList")))return false;
 if(mode!=5 && mode!=14 && !str(1,"LOCAL_LIGHT"))return false;
 if(mode==1 && (!str(0,"DICTIONARY")||!str(5,"F000")||!str(100,"AcDbDictionary")))return false;
 if(!str(0,"ENDSEC")||!str(0,"EOF"))return false;
 return !w->hasWriteError();
}
}
int main(){
 const auto path=std::filesystem::temp_directory_path()/("lightlist-ref-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dxf");
 struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove(p,e);}}cleanup{path};
 for(bool binary:{false,true}) {
  const auto name=path.string();dxfRW reader(name.c_str());
  for(int mode:{0,1,3,4,5,6,8,9,10,11,12,13,14,0}) {
  if(!make(path,binary,mode))return 1;Reader out;
  const bool ok=reader.read(&out,false);
  if(mode!=0 && mode!=14){if(ok){std::cerr<<"invalid accepted "<<mode<<'\n';return 1;}continue;}
  if(!ok||out.lights.size()!=1u||out.lights[0].handle!=0x2Fu||out.lists.size()!=1u
     ||out.lists[0].handle!=0xF000u||out.lists[0].m_lights.size()!=(mode==14?0u:1u)
     ||(mode!=14 && (out.lists[0].m_lights[0].m_handle!=0x2Fu||out.lists[0].m_lights[0].m_name!="LOCAL_LIGHT"))
     ||out.raw.size()!=1u||out.raw[0].handle!=0xF000u){std::cerr<<"reference parse binary="<<binary<<"\n";return 1;}
  std::uint32_t owner=0u;
  if(!out.lists[0].getAcLightDictionaryHandle(owner)||owner!=0x30u
     ||out.lists[0].parentHandle!=0x30u||!out.lists[0].reactorHandles.empty())return 1;
  std::vector<std::string> rawFives;
  for(const auto& group:out.raw[0].groups)
   if(group.code()==5 && group.c_str())rawFives.emplace_back(group.c_str());
  if(rawFives!=(mode==14?std::vector<std::string>{"F000"}:std::vector<std::string>{"F000","2F"}))return 1;
  }
 }
 return 0;
}
