// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include "intern/dxfwriter.h"
#include "intern/dxfreader.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
struct Capture final : LC3CompatCheck {
 void addViewport(const DRW_Viewport& v) override { if (count++ == 0) first=v; else second=v; }
 DRW_Viewport first,second; int count=0;
};
struct Export final : LC3CompatCheck {
 explicit Export(dxfRW& w):out(w){} void writeEntities() override {ok=out.writeViewport(&first)&&out.writeViewport(&second);} bool finalizeDxfWrite() override{return ok;}
 dxfRW& out; DRW_Viewport first,second; bool ok=false;
};
struct Field{int code; int val; const char* text;};
struct DirectViewport: DRW_Viewport {using DRW_Viewport::parseCode;};
static bool metadataReset(int group,const std::string& value){
 DirectViewport v;v.ambientColorName="BOOK$OLD";v.ambientColorDwgMetadataPresent=true;v.ambientColorMethod=42;v.ambientColorDwgFlags=3;v.ambientColorDwgIndex=7;
 std::istringstream stream(std::to_string(group)+"\n"+value+"\n");std::unique_ptr<dxfReader> reader=std::make_unique<dxfReaderAscii>(&stream);int code=0;
 return reader->readRec(&code)&&code==group&&v.parseCode(code,reader)&&!v.ambientColorDwgMetadataPresent&&v.ambientColorMethod==0&&v.ambientColorDwgFlags==0&&v.ambientColorDwgIndex==0&&v.ambientColorName==(group==431?"BOOK$NEW":"BOOK$OLD");
}
static bool create(const std::filesystem::path& p,bool binary,const std::vector<Field>& fields){
 std::ofstream f(p,std::ios::binary|std::ios::trunc); if(binary) f<<"AutoCAD Binary DXF\r\n"<<char(26)<<'\0';
 std::unique_ptr<dxfWriter>w=binary?std::unique_ptr<dxfWriter>(std::make_unique<dxfWriterBinary>(&f)):std::unique_ptr<dxfWriter>(std::make_unique<dxfWriterAscii>(&f));w->setVersion("AC1027",true);
 if(!w->writeString(0,"SECTION")||!w->writeString(2,"HEADER")||!w->writeString(9,"$ACADVER")||!w->writeString(1,"AC1027")||!w->writeString(0,"ENDSEC")||!w->writeString(0,"SECTION")||!w->writeString(2,"ENTITIES"))return false;
 for(int i=0;i<2;++i){
  if(!w->writeString(0,"VIEWPORT")||!w->writeString(5,i?"31":"30")||!w->writeString(100,"AcDbEntity")||!w->writeString(8,"0")||!w->writeString(100,"AcDbViewport")||!w->writeDouble(10,0)||!w->writeDouble(20,0)||!w->writeDouble(30,0)||!w->writeDouble(40,10)||!w->writeDouble(41,10)||!w->writeInt16(68,1)||!w->writeInt16(69,i+1))return false;
  if(i==0)for(const auto& x:fields){if(x.code==63&&!w->writeInt16(63,x.val))return false;if(x.code==421&&!w->writeInt32(421,x.val))return false;if(x.code==431&&!w->writeUtf8String(431,x.text))return false;}
 }
 return w->writeString(0,"ENDSEC")&&w->writeString(0,"EOF")&&!w->hasWriteError();
}
static bool read(const std::filesystem::path&p,Capture&c){dxfRW in(p.string().c_str());return in.read(&c,false);}
static bool check(const Capture& c,int aci,int rgb,const std::string& name){return c.count==2&&c.first.ambientColor==static_cast<unsigned>(aci)&&c.first.ambientColorRgb==rgb&&c.first.ambientColorName==name&&c.second.ambientColorName.empty()&&c.second.ambientColorRgb==-1&&!c.first.ambientColorDwgMetadataPresent;}
int main(int argc,char** argv){
 if(argc==2){Capture c;if(!read(argv[1],c)||c.count!=1||c.first.handle!=0x2f||c.first.ambientColor!=148||c.first.ambientColorRgb!=1193046||c.first.ambientColorName!="TEST$BLUE")return 10;return 0;}
 if(argc!=1)return 11;
 if(!metadataReset(63,"7")||!metadataReset(421,"1193046")||!metadataReset(431,"BOOK$NEW"))return 16;
 const auto dir=std::filesystem::temp_directory_path()/("viewport-color-order-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(dir);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{dir};
 for(bool binary:{false,true}){const auto in=dir/(binary?"input-b.dxf":"input-a.dxf");std::array<int,3> order{{0,1,2}};do{const std::array<Field,3> all{{{63,7,nullptr},{421,1193046,nullptr},{431,0,"BOOK$BLUE"}}};std::vector<Field> fields;for(int n:order)fields.push_back(all[static_cast<std::size_t>(n)]);if(!create(in,binary,fields))return 1;Capture c;if(!read(in,c)||!check(c,7,1193046,"BOOK$BLUE"))return 2;auto outpath=dir/(binary?"out-b.dxf":"out-a.dxf");dxfRW out(outpath.string().c_str());Export a(out);a.first=c.first;a.second=c.second;if(!out.write(&a,DRW::AC1027,binary)||!a.ok)return 3;Capture reread;if(!read(outpath,reread)||!check(reread,7,1193046,"BOOK$BLUE"))return 4;if(const char* save=std::getenv("VIEWPORT_COLOR_OUTPUT")){std::filesystem::create_directories(save);std::filesystem::copy_file(outpath,std::filesystem::path(save)/(binary?"binary.dxf":"ascii.dxf"),std::filesystem::copy_options::overwrite_existing);}}while(std::next_permutation(order.begin(),order.end()));
 const std::vector<std::vector<Field>> cases{{{431,0,"BOOK$BLACK"},{421,0,nullptr},{63,7,nullptr}},{{63,7,nullptr}},{{431,0,"FIRST"},{63,3,nullptr},{421,0,nullptr},{431,0,"SECOND"},{63,7,nullptr},{421,1193046,nullptr}}};
 const std::array<int,3> rgbs{{0,-1,1193046}};const std::array<std::string,3> names{{"BOOK$BLACK","","SECOND"}};
 for(std::size_t n=0;n<cases.size();++n){if(!create(in,binary,cases[n]))return 5;Capture c;if(!read(in,c)||!check(c,7,rgbs[n],names[n]))return 6;const auto path=dir/(binary?"case-b.dxf":"case-a.dxf");dxfRW out(path.string().c_str());Export a(out);a.first=c.first;a.second=c.second;if(!out.write(&a,DRW::AC1027,binary)||!a.ok)return 7;Capture after;if(!read(path,after)||!check(after,7,rgbs[n],names[n]))return 8;}
 if(!create(in,binary,{{431,0,"TEST"},{421,0x1000000,nullptr},{63,7,nullptr}}))return 12;dxfRW retry(in.string().c_str());Capture bad;if(retry.read(&bad,false))return 13;if(!create(in,binary,{{431,0,"BOOK$BLACK"},{421,0,nullptr},{63,7,nullptr}}))return 14;Capture good;if(!retry.read(&good,false)||!check(good,7,0,"BOOK$BLACK"))return 15;
 }
 return 0;}
