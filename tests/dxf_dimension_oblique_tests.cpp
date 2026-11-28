#include "lc3_compat_check.cpp"
#include "intern/dxfreader.h"
#include "libdxfrw.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <limits>
#include <iostream>
#include <vector>

template<class T> struct WithOblique:T {void setObliqueValue(double v){this->setOb52(v);} double obliqueValue()const{return this->getOb52();}};
struct Writer:LC3CompatCheck {
 dxfRW& out;bool ok=true,badAligned=false,ignoreFailures=false;
 explicit Writer(dxfRW& w):out(w){}
 void writeEntities()override{
  WithOblique<DRW_DimLinear> a;a.setAngle(30);a.setObliqueValue(20);a.setExtrusion({0,0,1});ok=out.writeDimension(&a)&&ok;
  WithOblique<DRW_DimAligned> b;b.setObliqueValue(badAligned?std::numeric_limits<double>::quiet_NaN():25);b.setExtrusion({1,2,3});ok=out.writeDimension(&b)&&ok;
  WithOblique<DRW_DimAngular> c;c.setObliqueValue(77);ok=out.writeDimension(&c)&&ok;
  WithOblique<DRW_DimAngular3p> d;d.setObliqueValue(77);ok=out.writeDimension(&d)&&ok;
  WithOblique<DRW_DimRadial> e;e.setObliqueValue(77);ok=out.writeDimension(&e)&&ok;
  WithOblique<DRW_DimDiametric> f;f.setObliqueValue(77);ok=out.writeDimension(&f)&&ok;
  WithOblique<DRW_DimOrdinate> g;g.setObliqueValue(77);ok=out.writeDimension(&g)&&ok;
  DRW_DimLinear defaultLinear;ok=out.writeDimension(&defaultLinear)&&ok;
  DRW_DimAligned defaultAligned;ok=out.writeDimension(&defaultAligned)&&ok;
 }
 bool finalizeDxfWrite()override{return ignoreFailures || ok;}
};
struct Reader:LC3CompatCheck {
 int linear=0,aligned=0,other=0;double linearAngle=0,alignedAngle=0;
 void addDimLinear(const DRW_DimLinear* d)override{++linear;if(d->getOblique()!=0)linearAngle=d->getOblique();}
 void addDimAlign(const DRW_DimAligned* d)override{++aligned;WithOblique<DRW_DimAligned> copy;static_cast<DRW_Dimension&>(copy)=*d;if(copy.obliqueValue()!=0)alignedAngle=copy.obliqueValue();}
 void addDimAngular(const DRW_DimAngular*)override{++other;}
 void addDimAngular3P(const DRW_DimAngular3p*)override{++other;}
 void addDimRadial(const DRW_DimRadial*)override{++other;}
 void addDimDiametric(const DRW_DimDiametric*)override{++other;}
 void addDimOrdinate(const DRW_DimOrdinate*)override{++other;}
};
std::string bytes(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
bool wire(const std::filesystem::path&p,bool binary){
 std::ifstream in(p,std::ios::binary);if(binary)in.seekg(22);std::unique_ptr<dxfReader> r=binary?std::unique_ptr<dxfReader>(new dxfReaderBinary(&in)):std::unique_ptr<dxfReader>(new dxfReaderAscii(&in));
 bool inside=false,aligned=false;int idx=-1,count=0,code=0;double angles[2]={0,0};
 while(r->readRec(&code)){
  if(code==0){if(inside&&idx<2&&!aligned)return false;inside=r->getString()=="DIMENSION";if(inside)++idx;continue;}
  if(!inside)continue;
  if(code==100&&r->getString()=="AcDbAlignedDimension")aligned=true;
  if(code==52){if(!aligned||idx>1)return false;angles[idx]=r->getDouble();++count;}
  if(code==100&&r->getString()=="AcDbDimension")aligned=false;
 }
 return idx==8&&count==2&&angles[0]==20&&angles[1]==25;
}
int main(){
 const auto path=std::filesystem::temp_directory_path()/("dim-oblique-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dxf");
 struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove(p,e);}} cleanup{path};
 for(bool binary:{false,true}) for(DRW::Version version:{DRW::AC1015,DRW::AC1018,DRW::AC1021,DRW::AC1024,DRW::AC1027,DRW::AC1032}) {
  {std::ofstream f(path,std::ios::binary);f<<"sentinel";}
  dxfRW w(path.string().c_str());Writer producer(w);if(w.write(&producer,DRW::AC1009,binary)||bytes(path)!="sentinel")return 1;
  producer.ok=true;if(!w.write(&producer,version,binary)||!producer.ok||!wire(path,binary))return 2;
  dxfRW r(path.string().c_str());Reader actual;if(!r.read(&actual,false)||actual.linear!=2||actual.aligned!=2||actual.other!=5||actual.linearAngle!=20||actual.alignedAngle!=25)return 3;
  {std::ofstream f(path,std::ios::binary|std::ios::trunc);f<<"sentinel";}
  producer.ok=true;producer.badAligned=true;producer.ignoreFailures=true;
  const bool rejectedWrite=w.write(&producer,version,binary);
  if(rejectedWrite||bytes(path)!="sentinel"){std::cerr << "nan result=" << rejectedWrite << " bytes=" << bytes(path).size() << " version=" << int(version) << " binary=" << binary << "\n";return 4;}
  producer.ok=true;producer.badAligned=false;producer.ignoreFailures=false;
  if(!w.write(&producer,version,binary)||!producer.ok||!wire(path,binary))return 5;
 }
 return 0;
}
