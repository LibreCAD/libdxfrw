#include "../dwg2dxf/dx_iface.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <type_traits>

template<class T,class=void> struct HasPublicDimensionSource : std::false_type {};
template<class T> struct HasPublicDimensionSource<T,std::void_t<decltype(std::declval<T&>().source)>> : std::true_type {};
template<class T,class=void> struct HasPublicDimensionVersion : std::false_type {};
template<class T> struct HasPublicDimensionVersion<T,std::void_t<decltype(std::declval<T&>().version)>> : std::true_type {};
static_assert(!HasPublicDimensionSource<dx_ifaceDwgDimensionOrigin>::value,"source binding must be private");
static_assert(!HasPublicDimensionVersion<dx_dwgDimensionImport>::value,"source version must be private");

class DxIfaceDimensionAngleTestAccess {
public:
 static std::shared_ptr<dx_dwgDimensionImport> make(std::uint64_t g, DRW::Version v, bool accepted) {
  auto p=std::make_shared<dx_dwgDimensionImport>();p->generation=g;p->version=v;p->sealedVersion=v;p->accepted=accepted;return p;
 }
 static void bind(dx_ifaceDwgDimensionOrigin& target,const std::shared_ptr<dx_dwgDimensionImport>& source) {
  target.source=source;target.capturedSource=source;target.sourceGeneration=source->generation;
 }
 static void setGeneration(const std::shared_ptr<dx_dwgDimensionImport>& p,std::uint64_t v){p->generation=v;}
 static void setVersion(const std::shared_ptr<dx_dwgDimensionImport>& p,DRW::Version v){p->version=v;}
 static void setBothVersions(const std::shared_ptr<dx_dwgDimensionImport>& p,DRW::Version v){p->version=v;p->sealedVersion=v;}
 static void replaceSource(dx_ifaceDwgDimensionOrigin& target,const std::shared_ptr<dx_dwgDimensionImport>& p){target.source=p;}
 static void setAccepted(const std::shared_ptr<dx_dwgDimensionImport>& p,bool v){p->accepted=v;}
 static bool matches(const dx_ifaceDwgDimensionOrigin* p,std::uint64_t g,DRW::Version v) {
  return p && p->source && p->source==p->capturedSource.lock() && p->sourceGeneration==g && p->source->generation==g && p->source->version==v && p->source->sealedVersion==v && p->source->accepted;
 }
 static std::shared_ptr<dx_dwgDimensionImport> receipt(const dx_ifaceDwgDimensionOrigin* p){return p?p->source:nullptr;}
};

namespace {
std::string read(const std::filesystem::path& p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
bool near(double a,double b){return std::abs(a-b)<1e-10;}
template<class T> T* add(dx_ifaceBlock* b,const std::shared_ptr<dx_dwgDimensionImport>& origin,std::uint32_t h) {
 T base; base.handle=h;base.setName("*D1");base.setDir(std::acos(-1.)/4);base.setHDir(std::acos(-1.)/12);base.setExtrusion({1,2,3});
 auto* d=new dx_ifaceDwgDimension<T>(base);DxIfaceDimensionAngleTestAccess::bind(*d,origin);b->ent.push_back(d);return d;
}
}
int main(){
 const auto path=std::filesystem::temp_directory_path()/("dimension-angle-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dxf");
 struct Cleanup {std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove(p,e);}} cleanup{path};
 for(bool binary:{false,true}){
  dx_data data; data.importGeneration=11; dx_iface writer;writer.cData=&data;writer.currentBlock=data.mBlock;
  auto origin=DxIfaceDimensionAngleTestAccess::make(11,DRW::AC1024,true);
  auto* first=dynamic_cast<dx_ifaceDwgDimension<DRW_DimLinear>*>(add<DRW_DimLinear>(data.mBlock,origin,0xA100));if(!first)return 20;first->setAngle(30);first->setOblique(20);first->extData.push_back(std::make_shared<DRW_Variant>(1001,std::string("ACAD")));first->extData.push_back(std::make_shared<DRW_Variant>(1000,std::string("dimension metadata")));
  auto* aligned=add<DRW_DimAligned>(data.mBlock,origin,0xA101);aligned->setExtrusion({0,1,0});aligned->setTextPoint({2,3,4});aligned->applyExtrusion();const DRW_Coord alignedOcs{2,3,4};
  add<DRW_DimAngular>(data.mBlock,origin,0xA102);add<DRW_DimAngular3p>(data.mBlock,origin,0xA103);add<DRW_DimRadial>(data.mBlock,origin,0xA104);add<DRW_DimDiametric>(data.mBlock,origin,0xA105);add<DRW_DimOrdinate>(data.mBlock,origin,0xA106);
  auto* plain=new DRW_DimLinear();plain->handle=0xA107;plain->setName("*D1");plain->setDir(22);plain->setHDir(17);data.mBlock->ent.push_back(plain);
  auto* picture=new dx_ifaceBlock();picture->name="*D1";picture->handle=0xB100;auto* line=new DRW_Line();line->handle=0xB101;line->secPoint={1,1,0};picture->ent.push_back(line);data.blocks.push_back(picture);
  auto* named=new dx_ifaceBlock();named->name="ANGLE_BLOCK";named->handle=0xB200;auto* namedDim=new dx_ifaceDwgDimension<DRW_DimLinear>(*first);namedDim->handle=0xB201;named->ent.push_back(namedDim);data.blocks.push_back(named);
  DRW_Group g;g.handle=0xC100;g.name="DIM_TARGET";g.m_entityHandles={first->handle};data.groups.push_back(g);first->reactorHandles.push_back(g.handle);
  const auto original=first->handle;
  if(!writer.fileExport(path.string(),DRW::AC1024,binary,&data,false,true)||first->handle!=original||!near(first->getDir(),std::acos(-1.)/4))return 1;
  dx_data decoded;dx_iface reader;if(!reader.fileImport(path.string(),&decoded,false)||decoded.mBlock->ent.size()!=8||decoded.groups.size()!=1)return 2;
  int index=0;std::uint32_t emitted=0;
  for(auto* e:decoded.mBlock->ent){auto* dim=dynamic_cast<DRW_Dimension*>(e);if(!dim)return 3;if(index<7){if(!near(dim->getDir(),45)||!near(dim->getHDir(),15))return 4;}else if(!near(dim->getDir(),22)||!near(dim->getHDir(),17))return 5;if(index==1){const auto point=dim->getTextPoint();if(!near(point.x,alignedOcs.x)||!near(point.y,alignedOcs.y)||!near(point.z,alignedOcs.z))return 21;}if(index==0){auto* linear=dynamic_cast<DRW_DimLinear*>(e);if(!linear||!near(linear->getAngle(),30)||!near(linear->getOblique(),20)||linear->getExtrusion().x!=1||linear->extData.size()!=2||linear->extData[1]->type()!=DRW_Variant::STRING||!linear->extData[1]->c_str()||std::string(linear->extData[1]->c_str())!="dimension metadata")return 6;emitted=e->handle;}++index;}
  if(emitted==original||decoded.groups.front().m_entityHandles.size()!=1||decoded.groups.front().m_entityHandles.front()!=emitted)return 7;
  auto namedResult=std::find_if(decoded.blocks.begin(),decoded.blocks.end(),[](const dx_ifaceBlock* b){return b&&b->name=="ANGLE_BLOCK";});
  if(namedResult==decoded.blocks.end()||(*namedResult)->ent.size()!=1)return 8;
  const auto* nd=dynamic_cast<const DRW_Dimension*>((*namedResult)->ent.front());if(!nd||!near(nd->getDir(),45)||!near(nd->getHDir(),15))return 9;
  const std::string sentinel="ANGLE_SENTINEL\n";{std::ofstream f(path,std::ios::binary|std::ios::trunc);f<<sentinel;}
  first->setDir(std::numeric_limits<double>::max());if(writer.fileExport(path.string(),DRW::AC1024,binary,&data,false,true)||read(path)!=sentinel||first->handle!=original)return 10;
  first->setDir(std::acos(-1.)/4);if(!writer.fileExport(path.string(),DRW::AC1024,binary,&data,false,true))return 11;
  first->setHDir(std::numeric_limits<double>::max());{std::ofstream f(path,std::ios::binary|std::ios::trunc);f<<sentinel;}
  if(writer.fileExport(path.string(),DRW::AC1024,binary,&data,false,true)||read(path)!=sentinel)return 22;
  first->setHDir(std::numeric_limits<double>::quiet_NaN());if(writer.fileExport(path.string(),DRW::AC1024,binary,&data,false,true)||read(path)!=sentinel)return 23;
  first->setHDir(std::acos(-1.)/12);if(!writer.fileExport(path.string(),DRW::AC1024,binary,&data,false,true))return 24;
  DxIfaceDimensionAngleTestAccess::setAccepted(origin,false);{std::ofstream f(path,std::ios::binary|std::ios::trunc);f<<sentinel;}
  if(writer.fileExport(path.string(),DRW::AC1024,binary,&data,false,true)||read(path)!=sentinel)return 25;
  DxIfaceDimensionAngleTestAccess::setAccepted(origin,true);DxIfaceDimensionAngleTestAccess::setGeneration(origin,12);{std::ofstream f(path,std::ios::binary|std::ios::trunc);f<<sentinel;}
  if(writer.fileExport(path.string(),DRW::AC1024,binary,&data,false,true)||read(path)!=sentinel)return 12;
  DxIfaceDimensionAngleTestAccess::setGeneration(origin,11);if(!writer.fileExport(path.string(),DRW::AC1024,binary,&data,false,true))return 13;
  data.importGeneration=12;
  if(!writer.fileExport(path.string(),DRW::AC1024,binary,&data,false,true))return 29;
  dx_data later;dx_iface laterReader;if(!laterReader.fileImport(path.string(),&later,false)||later.mBlock->ent.empty())return 30;
  const auto* laterDim=dynamic_cast<const DRW_Dimension*>(later.mBlock->ent.front());
  if(!laterDim||!near(laterDim->getDir(),45)||!near(laterDim->getHDir(),15))return 31;
  DxIfaceDimensionAngleTestAccess::setVersion(origin,DRW::AC1021);
  {std::ofstream f(path,std::ios::binary|std::ios::trunc);f<<sentinel;}
  if(writer.fileExport(path.string(),DRW::AC1024,binary,&data,false,true)||read(path)!=sentinel)return 32;
  DxIfaceDimensionAngleTestAccess::setVersion(origin,DRW::AC1024);
  auto forged=DxIfaceDimensionAngleTestAccess::make(11,DRW::AC1024,true);
  DxIfaceDimensionAngleTestAccess::replaceSource(*first,forged);
  if(writer.fileExport(path.string(),DRW::AC1024,binary,&data,false,true)||read(path)!=sentinel)return 33;
  DxIfaceDimensionAngleTestAccess::replaceSource(*first,origin);
  if(!writer.fileExport(path.string(),DRW::AC1024,binary,&data,false,true))return 34;
  DxIfaceDimensionAngleTestAccess::setBothVersions(origin,DRW::AC1021);
  if(!writer.fileExport(path.string(),DRW::AC1024,binary,&data,false,true))return 26;
  dx_data legacy;dx_iface legacyReader;if(!legacyReader.fileImport(path.string(),&legacy,false)||legacy.mBlock->ent.empty())return 27;
  const auto* legacyDim=dynamic_cast<const DRW_Dimension*>(legacy.mBlock->ent.front());
  if(!legacyDim||!near(legacyDim->getDir(),std::acos(-1.)/4)||!near(legacyDim->getHDir(),std::acos(-1.)/12))return 28;
 }
 return 0;
}
