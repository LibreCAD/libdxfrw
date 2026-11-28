// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include "intern/dxfwriter.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
struct Capture:LC3CompatCheck {void addSectionObject(const DRW_SectionObject&)override{++typed;}void addRawDxfEntity(const DRW_RawDxfObject& v)override{raw=v;++rawCount;}int typed=0,rawCount=0;DRW_RawDxfObject raw;};
struct PolicyCapture:Capture,DRW_DxfSectionObjectRaw411Policy {bool enabled=false;bool wantsDxfSectionObjectRaw411Only()const noexcept override{return enabled;}};
struct Export:LC3CompatCheck {explicit Export(dxfRW&w):out(w){}void writeEntities()override{ok=out.writeRawDxfEntity(&raw,DRW::DxfModelSpaceBlockRecordHandle);}bool finalizeDxfWrite()override{return ok;}dxfRW&out;DRW_RawDxfObject raw;bool ok=false;};
enum class Mode{Normal,Body,Empty,App,AppBody,Other};
static bool makeInput(const std::filesystem::path&p,bool binary,Mode mode){
 std::ofstream f(p,std::ios::binary|std::ios::trunc);if(binary)f<<"AutoCAD Binary DXF\r\n"<<char(26)<<'\0';
 std::unique_ptr<dxfWriter>w=binary?std::unique_ptr<dxfWriter>(std::make_unique<dxfWriterBinary>(&f)):std::unique_ptr<dxfWriter>(std::make_unique<dxfWriterAscii>(&f));w->setVersion("AC1027",true);
 if(!w->writeString(0,"SECTION")||!w->writeString(2,"HEADER")||!w->writeString(9,"$ACADVER")||!w->writeString(1,"AC1027")||!w->writeString(0,"ENDSEC")||!w->writeString(0,"SECTION")||!w->writeString(2,"ENTITIES")||!w->writeString(0,"SECTIONOBJECT")||!w->writeString(5,"33")||!w->writeString(330,"1F")||!w->writeString(100,"AcDbEntity")||!w->writeString(8,"0"))return false;
 if(mode==Mode::App&&(!w->writeString(102,"{APP")||!w->writeString(411,"APP-DATA")||!w->writeString(102,"}")))return false;
 if(mode==Mode::Other&&(!w->writeString(100,"AcDbOther")||!w->writeString(411,"OTHER-DATA")))return false;
 if(!w->writeString(100,"AcDbSection"))return false;
 if(mode==Mode::AppBody&&(!w->writeString(102,"{APP")||!w->writeString(411,"APP-DATA")||!w->writeString(102,"}")))return false;
 if(!w->writeInt32(90,1)||!w->writeInt32(91,1)||!w->writeString(1,"SECT_A")||!w->writeInt16(63,7))return false;
 if((mode==Mode::Body||mode==Mode::Empty)&&!w->writeString(411,mode==Mode::Body?"BOOK$BLUE":""))return false;
 return w->writeInt32(92,1)&&w->writeDouble(11,2.)&&w->writeDouble(21,3.)&&w->writeDouble(31,4.)&&w->writeInt32(93,0)&&w->writeString(0,"ENDSEC")&&w->writeString(0,"EOF")&&!w->hasWriteError();
}
static bool read(const std::filesystem::path&p,Capture&c){dxfRW r(p.string().c_str());return r.read(&c,false);}
static bool has411(const DRW_RawDxfObject&r,const std::string& value){int count=0;for(const auto&g:r.groups)if(g.code()==411){if(g.type()!=DRW_Variant::STRING||g.c_str()==nullptr||g.c_str()!=value)return false;++count;}return count==1;}
static std::string content(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
int main(int argc,char**argv){
 if(argc==3){PolicyCapture c;c.enabled=true;if(!read(argv[1],c)||c.typed||c.rawCount!=1||!has411(c.raw,"BOOK$BLUE"))return 11;for(bool binary:{false,true}){std::filesystem::create_directories(argv[2]);auto p=std::filesystem::path(argv[2])/(binary?"binary.dxf":"ascii.dxf");dxfRW output(p.string().c_str());Export exp(output);exp.raw=c.raw;if(!output.write(&exp,DRW::AC1027,binary)||!exp.ok)return 12;PolicyCapture round;round.enabled=true;if(!read(p,round)||round.typed||round.rawCount!=1||!has411(round.raw,"BOOK$BLUE"))return 13;}return 0;}
 if(argc!=1)return 14;
 const auto dir=std::filesystem::temp_directory_path()/("section411-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(dir);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{dir};
 for(bool binary:{false,true}){auto in=dir/(binary?"in-b.dxf":"in-a.dxf"),outpath=dir/(binary?"out-b.dxf":"out-a.dxf");
  for(auto mode:{Mode::Normal,Mode::App,Mode::AppBody,Mode::Other}){if(!makeInput(in,binary,mode))return 1;Capture c;if(!read(in,c)||c.typed!=1||c.rawCount!=1)return 2;}
  for(auto mode:{Mode::Body,Mode::Empty}){if(!makeInput(in,binary,mode))return 3;Capture old;dxfRW ordinary(in.string().c_str());if(ordinary.read(&old,false)||old.typed||old.rawCount||ordinary.getLastDiagnostic().code!="sectionobject-411-raw-only")return 4;PolicyCapture disabled;if(read(in,disabled)||disabled.typed||disabled.rawCount)return 5;PolicyCapture enabled;enabled.enabled=true;if(!read(in,enabled)||enabled.typed||enabled.rawCount!=1||!has411(enabled.raw,mode==Mode::Body?"BOOK$BLUE":""))return 6;
   {std::ofstream f(outpath,std::ios::binary|std::ios::trunc);f<<"sentinel";}dxfRW output(outpath.string().c_str());Export exp(output);exp.raw=enabled.raw;if(output.write(&exp,DRW::AC1024,binary)||content(outpath)!="sentinel")return 7;if(!output.write(&exp,DRW::AC1027,binary)||!exp.ok)return 8;PolicyCapture round;round.enabled=true;if(!read(outpath,round)||round.typed||round.rawCount!=1||!has411(round.raw,mode==Mode::Body?"BOOK$BLUE":""))return 9;Capture oldRound;if(read(outpath,oldRound)||oldRound.typed||oldRound.rawCount)return 10;
  }
  if(!makeInput(in,binary,Mode::Body))return 15;dxfRW retry(in.string().c_str());Capture rejected;if(retry.read(&rejected,false)||rejected.typed||rejected.rawCount)return 16;if(!makeInput(in,binary,Mode::Normal))return 17;Capture corrected;if(!retry.read(&corrected,false)||corrected.typed!=1||corrected.rawCount!=1||retry.getLastDiagnostic().failed())return 18;
 }
 return 0;
}
