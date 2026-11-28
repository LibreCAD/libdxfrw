// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include "intern/dxfwriter.h"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <new>
#include <string>
#include <vector>

namespace {
long allocationsBeforeFailure = -1;
std::size_t failedAllocations = 0u;
struct AllocationFailure {
 explicit AllocationFailure(long count) { allocationsBeforeFailure = count; }
 ~AllocationFailure() { allocationsBeforeFailure = -1; }
};
}

void* operator new(std::size_t size) {
 if (allocationsBeforeFailure == 0) {
  ++failedAllocations;
  allocationsBeforeFailure = -1;
  throw std::bad_alloc();
 }
 if (allocationsBeforeFailure > 0) --allocationsBeforeFailure;
 if (void* value = std::malloc(size == 0u ? 1u : size)) return value;
 throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete(void* value, std::size_t) noexcept { std::free(value); }
void operator delete[](void* value, std::size_t) noexcept { std::free(value); }
struct Capture final : LC3CompatCheck {
 void addSun(const DRW_Sun& v) override {if(count++==0)first=v;else second=v;}
 void addRawDxfObject(const DRW_RawDxfObject& v) override {if(v.name=="SUN"&&rawCount++==0)raw=v;}
 DRW_Sun first,second;DRW_RawDxfObject raw;int count=0,rawCount=0;
};
struct Export final:LC3CompatCheck {
 explicit Export(dxfRW& w):out(w){}
 void writeObjects() override {
  if(failAt<0) accepted=out.writeSun(&sun);
  else {
   const std::size_t before=failedAllocations;
   AllocationFailure failure(failAt);
   try {accepted=out.writeSun(&sun);}
   catch(const std::bad_alloc&) {
    accepted=false;
   }
   reached=failedAllocations!=before?static_cast<long>(failedAllocations-before):failAt-allocationsBeforeFailure;
  }
  if(!accepted)(void)out.writeRawDxfObject(nullptr);
 }
 bool finalizeDxfWrite() override{return accepted;}
 dxfRW& out;DRW_Sun sun;long failAt=-1,reached=0;bool accepted=false;
};
struct Field{int code;int value;};
static bool input(const std::filesystem::path&p,bool binary,const std::vector<Field>& fields){
 std::ofstream f(p,std::ios::binary|std::ios::trunc);if(binary)f<<"AutoCAD Binary DXF\r\n"<<char(26)<<'\0';
 std::unique_ptr<dxfWriter>w=binary?std::unique_ptr<dxfWriter>(std::make_unique<dxfWriterBinary>(&f)):std::unique_ptr<dxfWriter>(std::make_unique<dxfWriterAscii>(&f));w->setVersion("AC1027",true);
 if(!w->writeString(0,"SECTION")||!w->writeString(2,"HEADER")||!w->writeString(9,"$ACADVER")||!w->writeString(1,"AC1027")||!w->writeString(0,"ENDSEC")||!w->writeString(0,"SECTION")||!w->writeString(2,"OBJECTS"))return false;
 for(int i=0;i<2;++i){if(!w->writeString(0,"SUN")||!w->writeString(5,i?"51":"50")||!w->writeString(100,"AcDbSun")||!w->writeInt32(90,1)||!w->writeBool(290,true))return false;
  if(i==0){for(const auto&x:fields){if(x.code==63&&!w->writeInt16(63,x.value))return false;if(x.code==421&&!w->writeInt32(421,x.value))return false;}}
  else if(!w->writeInt16(63,3))return false;
  if(!w->writeDouble(40,1.)||!w->writeBool(291,false)||!w->writeInt32(91,2456922)||!w->writeInt32(92,43200)||!w->writeBool(292,false)||!w->writeInt16(70,0)||!w->writeInt16(71,256)||!w->writeInt16(280,1))return false;
 }
 return w->writeString(0,"ENDSEC")&&w->writeString(0,"EOF")&&!w->hasWriteError();
}
static bool read(const std::filesystem::path&p,Capture&c){dxfRW in(p.string().c_str());return in.read(&c,false);}
static std::string file(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
static bool rawColor(const Capture&c,int expected){int n=0;for(const auto&g:c.raw.groups)if(g.code()==421){if(g.type()!=DRW_Variant::INTEGER||g.i_val()!=expected)return false;++n;}return n==1;}
static bool sunReadback(const std::filesystem::path& path,
                        DRW::Version expectedVersion) {
 Capture capture;
 dxfRW input(path.string().c_str());
 if(!input.read(&capture,false)||input.getSourceVersion()!=expectedVersion
    ||capture.count!=1)return false;
 const DRW_Sun& sun=capture.first;
 return sun.handle==0xD010u&&sun.parentHandle==0x30u
  &&sun.m_classVersion==1u&&sun.m_isOn&&sun.m_color==7u
  &&sun.m_intensity==1.25&&sun.m_hasShadow
  &&sun.m_julianDay==2458123&&sun.m_milliseconds==45678
  &&sun.m_isDaylightSavings&&sun.m_shadowType==2u
  &&sun.m_shadowMapSize==512u&&sun.m_shadowSoftness==3u;
}
static bool allocationSweep(const std::filesystem::path& dir) {
 const std::string prior="prior SUN bytes\n";
 const std::pair<DRW::Version,const char*> versions[]={
  {DRW::AC1021,"AC1021"},{DRW::AC1024,"AC1024"},
  {DRW::AC1027,"AC1027"},{DRW::AC1032,"AC1032"}};
 for(const auto& target:versions)for(bool binary:{false,true}) {
  const auto path=dir/(std::string("allocation-sun-")+target.second
                       +(binary?"-binary.dxf":"-ascii.dxf"));
  long allocationCount=0;std::string expected;
  {
   dxfRW codec(path.string().c_str());Export exporter(codec);
   exporter.sun.handle=0xD010u;exporter.sun.parentHandle=0x30u;
   exporter.sun.m_classVersion=1u;exporter.sun.m_isOn=true;
   exporter.sun.m_color=7u;exporter.sun.m_intensity=1.25;
   exporter.sun.m_hasShadow=true;exporter.sun.m_julianDay=2458123;
   exporter.sun.m_milliseconds=45678;exporter.sun.m_isDaylightSavings=true;
   exporter.sun.m_shadowType=2u;exporter.sun.m_shadowMapSize=512u;
   exporter.sun.m_shadowSoftness=3u;exporter.failAt=1000000;
   const bool written=codec.write(&exporter,target.first,binary);
   if(!written||!exporter.accepted||!sunReadback(path,target.first))return false;
   allocationCount=exporter.reached;expected=file(path);
  }
  if(allocationCount<=0||expected.empty())return false;
  for(long offset=0;offset<allocationCount;++offset) {
   {std::ofstream output(path,std::ios::binary|std::ios::trunc);output<<prior;if(!output)return false;}
   dxfRW codec(path.string().c_str());Export exporter(codec);
   exporter.sun.handle=0xD010u;exporter.sun.parentHandle=0x30u;
   exporter.sun.m_classVersion=1u;exporter.sun.m_isOn=true;
   exporter.sun.m_color=7u;exporter.sun.m_intensity=1.25;
   exporter.sun.m_hasShadow=true;exporter.sun.m_julianDay=2458123;
   exporter.sun.m_milliseconds=45678;exporter.sun.m_isDaylightSavings=true;
   exporter.sun.m_shadowType=2u;exporter.sun.m_shadowMapSize=512u;
   exporter.sun.m_shadowSoftness=3u;exporter.failAt=offset;
   const bool written=codec.write(&exporter,target.first,binary);
   exporter.failAt=-1;
   if(written||exporter.accepted||exporter.reached!=1||file(path)!=prior
      ||exporter.sun.handle!=0xD010u||exporter.sun.parentHandle!=0x30u
      ||exporter.sun.m_intensity!=1.25||exporter.sun.m_julianDay!=2458123)
    return false;
   const bool retried=codec.write(&exporter,target.first,binary);
   if(!retried||!exporter.accepted||file(path)!=expected
      ||!sunReadback(path,target.first))return false;
  }
  std::cout<<"SUN "<<target.second<<" DXF "<<(binary?"Binary":"ASCII")<<": allocation offsets="
           <<allocationCount<<", rollback/retry/readback=PASS\n";
 }
 return true;
}
int main(int argc,char**argv){
 if(argc==2){Capture c;if(!read(argv[1],c)||c.count!=1||c.rawCount!=1||c.first.handle!=0x30||c.first.m_color!=148||c.first.m_color24!=1193046||!rawColor(c,1193046))return 10;return 0;}
 if(argc!=1)return 11;
 const auto dir=std::filesystem::temp_directory_path()/("sun-color-order-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directories(dir);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}}cleanup{dir};
 for(bool binary:{false,true}){
  const auto in=dir/(binary?"input-b.dxf":"input-a.dxf"),outpath=dir/(binary?"out-b.dxf":"out-a.dxf");
  for(int rgb:{0,1193046})for(bool reversed:{false,true}){
   std::vector<Field> fields=reversed?std::vector<Field>{{421,rgb},{63,7}}:std::vector<Field>{{63,7},{421,rgb}};if(!input(in,binary,fields))return 1;Capture c;if(!read(in,c)||c.count!=2||c.rawCount!=2||c.first.m_color!=7||c.first.m_color24!=rgb||c.second.m_color!=3||c.second.m_color24!=-1||!rawColor(c,rgb))return 2;
   {std::ofstream f(outpath,std::ios::binary|std::ios::trunc);f<<"sentinel";}dxfRW out(outpath.string().c_str());Export exp(out);exp.sun=c.first;const auto original=exp.sun;if(out.write(&exp,DRW::AC1027,binary)||file(outpath)!="sentinel"||exp.sun.handle!=original.handle||exp.sun.m_color24!=original.m_color24)return 3;
   exp.sun.m_color24=-1;if(!out.write(&exp,DRW::AC1027,binary))return 4;Capture after;if(!read(outpath,after)||after.count!=1||after.first.m_color!=7||after.first.m_color24!=-1)return 5;if(const char* save=std::getenv("SUN_COLOR_OUTPUT")){std::filesystem::create_directories(save);std::filesystem::copy_file(outpath,std::filesystem::path(save)/(binary?"binary.dxf":"ascii.dxf"),std::filesystem::copy_options::overwrite_existing);}
  }
  if(!input(in,binary,{{63,7}}))return 6;Capture absent;if(!read(in,absent)||absent.count!=2||absent.first.m_color24!=-1)return 7;
 }
 if(!allocationSweep(dir))return 8;
 return 0;
}
