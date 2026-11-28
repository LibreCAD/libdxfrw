// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include "intern/dxfwriter.h"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <vector>
struct Field{int code;double value;};
struct Capture:LC3CompatCheck{void addSectionObject(const DRW_SectionObject& s)override{if(typed==0)value=s;else second=s;++typed;}void addRawDxfEntity(const DRW_RawDxfObject&)override{++raw;}DRW_SectionObject value,second;int typed=0,raw=0;};
struct RawCapture:Capture,DRW_DxfSectionObjectRaw411Policy{bool wantsDxfSectionObjectRaw411Only()const noexcept override{return true;}};
struct Export:LC3CompatCheck{explicit Export(dxfRW&w):out(w){}void writeEntities()override{ok=out.writeSectionObject(&section);}bool finalizeDxfWrite()override{return ok;}dxfRW&out;DRW_SectionObject section;bool ok=false;};
static bool input(const std::filesystem::path&p,bool binary,const std::vector<Field>& fields,bool raw411=false,bool appCounts=false,bool second=false,bool alias=false){std::ofstream f(p,std::ios::binary|std::ios::trunc);if(binary)f<<"AutoCAD Binary DXF\r\n"<<char(26)<<'\0';std::unique_ptr<dxfWriter>w=binary?std::unique_ptr<dxfWriter>(std::make_unique<dxfWriterBinary>(&f)):std::unique_ptr<dxfWriter>(std::make_unique<dxfWriterAscii>(&f));w->setVersion("AC1027",true);
 if(!w->writeString(0,"SECTION")||!w->writeString(2,"HEADER")||!w->writeString(9,"$ACADVER")||!w->writeString(1,"AC1027")||!w->writeString(0,"ENDSEC")||!w->writeString(0,"SECTION")||!w->writeString(2,"ENTITIES")||!w->writeString(0,alias?"SECTION_OBJECT":"SECTIONOBJECT")||!w->writeString(5,"33")||!w->writeString(100,"AcDbEntity")||!w->writeString(8,"0")||!w->writeString(100,"AcDbSection")||!w->writeInt32(90,1)||!w->writeInt32(91,1)||!w->writeString(1,"SECT_A")||!w->writeInt16(63,7))return false;
 if(raw411&&!w->writeString(411,"BOOK$BLUE"))return false;
 if(appCounts&&(!w->writeString(102,"{APP")||!w->writeInt32(92,999)||!w->writeInt32(93,-1)||!w->writeDouble(11,77)||!w->writeDouble(21,88)||!w->writeString(102,"}")))return false;
 for(const auto& x:fields){if(x.code==92||x.code==93){if(!w->writeInt32(x.code,static_cast<int>(x.value)))return false;}else if(!std::isfinite(x.value)){
  if(binary){std::uint64_t bits=0;static_assert(sizeof(bits)==sizeof(x.value),"double size");std::memcpy(&bits,&x.value,sizeof(bits));f.put(static_cast<char>(x.code&0xff));f.put(static_cast<char>((x.code>>8)&0xff));for(unsigned n=0;n<8;++n)f.put(static_cast<char>((bits>>(8u*n))&0xffu));}
  else f<<x.code<<'\n'<<(std::isnan(x.value)?"nan":"inf")<<'\n';
  if(!f)return false;
 }else if(!w->writeDouble(x.code,x.value))return false;}
 if(second&&(!w->writeString(0,"SECTIONOBJECT")||!w->writeString(5,"34")||!w->writeString(100,"AcDbEntity")||!w->writeString(8,"0")||!w->writeString(100,"AcDbSection")||!w->writeInt32(92,0)||!w->writeInt32(93,0)))return false;
 return w->writeString(0,"ENDSEC")&&w->writeString(0,"EOF")&&!w->hasWriteError();}
static bool read(const std::filesystem::path&p,Capture&c){dxfRW in(p.string().c_str());return in.read(&c,false);}
static bool geometry(const DRW_SectionObject& s){return s.m_verts.size()==2&&s.m_blVerts.size()==2&&s.m_verts[0].x==0&&s.m_verts[0].y==0&&s.m_verts[0].z==0&&s.m_verts[1].x==4&&s.m_verts[1].y==0&&s.m_verts[1].z==0&&s.m_blVerts[0].x==0&&s.m_blVerts[0].y==1&&s.m_blVerts[0].z==0&&s.m_blVerts[1].x==4&&s.m_blVerts[1].y==1&&s.m_blVerts[1].z==0;}
int main(int argc,char**argv){if(argc==2){Capture c;if(!read(argv[1],c)||c.typed!=1||!geometry(c.value))return 25;return 0;}if(argc!=1)return 26;
 const auto dir=std::filesystem::temp_directory_path()/("section-counts-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(dir);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{dir};
 const std::vector<Field> late{{11,0},{21,0},{31,0},{11,4},{21,0},{31,0},{92,2},{12,0},{22,1},{32,0},{12,4},{22,1},{32,0},{93,2}};
 const std::vector<Field> early{{92,2},{11,0},{21,0},{31,0},{11,4},{21,0},{31,0},{93,2},{12,0},{22,1},{32,0},{12,4},{22,1},{32,0}};
 const std::vector<Field> interleaved{{92,2},{93,2},{11,0},{12,0},{31,0},{32,0},{21,0},{22,1},{11,4},{12,4},{21,0},{22,1},{31,0},{32,0}};
 const std::vector<Field> noz{{92,2},{11,0},{21,0},{11,4},{21,0},{93,2},{12,0},{22,1},{12,4},{22,1}};
 const std::vector<Field> nocount{{11,0},{21,0},{31,0},{11,4},{21,0},{31,0},{12,0},{22,1},{32,0},{12,4},{22,1},{32,0}};
 const std::vector<std::vector<Field>> invalid{{{92,1},{11,0},{21,0},{11,4},{21,0}},{{93,1},{12,0},{22,1},{12,4},{22,1}},{{92,1},{92,1},{11,0},{21,0}},{{93,0},{93,0}},{{92,513}},{{93,-1}},{{11,0},{11,4},{21,0},{21,0}},{{12,0},{12,4},{22,1},{22,1}},{{11,0}},{{12,0}},{{21,1}},{{32,0}},{{11,0},{21,0},{21,1}},{{12,0},{22,1},{32,0},{32,0}},{{11,std::numeric_limits<double>::infinity()},{21,0}},{{12,0},{22,std::numeric_limits<double>::quiet_NaN()}}};
 for(bool binary:{false,true}){auto src=dir/(binary?"in-b.dxf":"in-a.dxf"),dst=dir/(binary?"out-b.dxf":"out-a.dxf");
  for(const auto&fields:std::vector<std::vector<Field>>{early,late,interleaved,noz,nocount}){if(!input(src,binary,fields))return 1;Capture c;if(!read(src,c)||c.typed!=1||c.raw!=1||!geometry(c.value))return 2;DRW_SectionObject copied(c.value),assigned;assigned=c.value;if(!geometry(copied)||!geometry(assigned))return 14;c.value.handle=0;c.value.parentHandle=DRW::NoHandle;dxfRW output(dst.string().c_str());Export e(output);e.section=c.value;if(!output.write(&e,DRW::AC1027,binary)||!e.ok)return 3;Capture round;if(!read(dst,round)||round.typed!=1||!geometry(round.value))return 4;if(const char* save=std::getenv("SECTION_COUNT_OUTPUT")){std::filesystem::create_directories(save);std::filesystem::copy_file(dst,std::filesystem::path(save)/(binary?"binary.dxf":"ascii.dxf"),std::filesystem::copy_options::overwrite_existing);}}
  if(!input(src,binary,late,false,true,true,true))return 15;Capture scope;if(!read(src,scope)||scope.typed!=2||scope.raw!=2||!geometry(scope.value)||!scope.second.m_verts.empty()||!scope.second.m_blVerts.empty())return 16;
  RawCapture validRaw;if(!input(src,binary,late,true)||!read(src,validRaw)||validRaw.typed||validRaw.raw!=1)return 17;
  for(const auto&fields:invalid){if(!input(src,binary,fields))return 5;Capture c;if(read(src,c)||c.typed||c.raw)return 6;RawCapture raw;if(!input(src,binary,fields,true)||read(src,raw)||raw.typed||raw.raw)return 7;Capture defaultPolicy;if(read(src,defaultPolicy)||defaultPolicy.typed||defaultPolicy.raw)return 21;}
  std::vector<Field> limit{{92,512}};for(int i=0;i<512;++i){limit.push_back({11,double(i)});limit.push_back({21,0});}if(!input(src,binary,limit))return 19;Capture full;if(!read(src,full)||full.typed!=1||full.value.m_verts.size()!=512||full.value.m_verts.back().x!=511.)return 20;
  std::vector<Field> over;for(int i=0;i<513;++i){over.push_back({11,double(i)});over.push_back({21,0});}if(!input(src,binary,over))return 8;Capture bad;if(read(src,bad)||bad.typed||bad.raw)return 9;
  if(!input(src,binary,invalid[0]))return 10;dxfRW retry(src.string().c_str());Capture failed;if(retry.read(&failed,false)||failed.typed||failed.raw)return 11;if(!input(src,binary,late))return 12;Capture good;if(!retry.read(&good,false)||good.typed!=1||!geometry(good.value))return 13;
 }
 return 0;}
