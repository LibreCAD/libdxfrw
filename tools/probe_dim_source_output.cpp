#include "dwg2dxf/dx_iface.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <typeinfo>

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

template<class T> static bool addSelected(dx_ifaceBlock* block, DRW_Entity* base) {
 auto* source=dynamic_cast<dx_ifaceDwgDimension<T>*>(base);
 if(!source) return false;
 std::unique_ptr<dx_ifaceDwgDimension<T>> copy(new dx_ifaceDwgDimension<T>(*source));
 if(copy->getName().empty() || copy->eType != source->eType)return false;
 block->ent.push_back(copy.release());
 return true;
}
static std::string bytes(const char* path) {std::ifstream in(path,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
int main(int argc,char**argv) {
 if(argc!=6)return 2;
 const int expectedCount=std::stoi(argv[4]);
 const bool binary=std::string(argv[3])=="binary";
 dx_data data;dx_iface reader;
 if(!reader.fileImport(argv[1],&data,false) || data.sourceVersion!=DRW::AC1024) return 3;
 dx_data selected;dx_iface writer;writer.cData=&selected;writer.currentBlock=selected.mBlock;
 selected.importGeneration=data.importGeneration;
 int count=0;
 for(auto* e:data.mBlock->ent) {
  auto* d=dynamic_cast<DRW_Dimension*>(e);
  if(!d)continue;
  if(auto* linear=dynamic_cast<DRW_DimLinear*>(d)) std::cerr << "SRC angle=" << linear->getAngle() << " oblique=" << linear->getOblique() << "\n";
  auto* evidence=dynamic_cast<dx_ifaceDwgDimensionOrigin*>(e);
  if(!DxIfaceDimensionAngleTestAccess::matches(evidence,data.importGeneration,DRW::AC1024)||std::abs(d->getDir()-std::acos(-1.)/4)>1e-12||std::abs(d->getHDir()-std::acos(-1.)/12)>1e-12)return 4;
  const bool copied=addSelected<DRW_DimLinear>(selected.mBlock,e)||addSelected<DRW_DimAligned>(selected.mBlock,e)||addSelected<DRW_DimAngular>(selected.mBlock,e)||addSelected<DRW_DimAngular3p>(selected.mBlock,e)||addSelected<DRW_DimRadial>(selected.mBlock,e)||addSelected<DRW_DimDiametric>(selected.mBlock,e)||addSelected<DRW_DimOrdinate>(selected.mBlock,e);
  if(!copied)return 5;
  ++count;
 }
 if(count!=expectedCount)return 6;
 selected.lineTypes=data.lineTypes;selected.layers=data.layers;selected.dimStyles=data.dimStyles;selected.textStyles=data.textStyles;
 for(auto it=data.blocks.begin();it!=data.blocks.end();) {
  auto* block=*it;
  if(block && block->name!="*Model_Space" && block->name!="*Paper_Space" && block->name!="*MODEL_SPACE" && block->name!="*PAPER_SPACE") {
   selected.blocks.push_back(block);it=data.blocks.erase(it);
  } else ++it;
 }
 auto* first=dynamic_cast<dx_ifaceDwgDimension<DRW_DimLinear>*>(selected.mBlock->ent.front());
 if(!first)return 8;
 const auto origin=DxIfaceDimensionAngleTestAccess::receipt(first);
 if(!DxIfaceDimensionAngleTestAccess::matches(first,selected.importGeneration,DRW::AC1024))return 18;
 const auto firstSourceHandle=first->handle;
 DRW_Group group;group.handle=0xF600;group.name="DIM_TARGET";group.m_entityHandles={firstSourceHandle};selected.groups.push_back(group);
 first->reactorHandles.push_back(group.handle);
 auto* named=new dx_ifaceBlock();named->name="ANGLE_BLOCK";named->handle=0xF610;
 auto* namedDim=new dx_ifaceDwgDimension<DRW_DimLinear>(*first);namedDim->handle=0xF611;named->ent.push_back(namedDim);selected.blocks.push_back(named);
 std::cerr << "SELECTED angle=" << first->getAngle() << " oblique=" << first->getOblique() << "\n";
 if(!writer.fileExport(argv[2],DRW::AC1024,binary,&selected,false,true))return 9;
 if(first->handle!=firstSourceHandle || std::abs(first->getDir()-std::acos(-1.)/4)>1e-12)return 10;
 const std::string sentinel="DIMENSION_ANGLE_SENTINEL\n";
 {std::ofstream out(argv[2],std::ios::binary|std::ios::trunc);out<<sentinel;}
 first->setDir(std::numeric_limits<double>::max());
 if(writer.fileExport(argv[2],DRW::AC1024,binary,&selected,false,true)||bytes(argv[2])!=sentinel||first->handle!=firstSourceHandle)return 11;
 first->setDir(std::acos(-1.)/4);
 if(!writer.fileExport(argv[2],DRW::AC1024,binary,&selected,false,true))return 12;
 DxIfaceDimensionAngleTestAccess::setGeneration(origin,selected.importGeneration+1);
 {std::ofstream out(argv[2],std::ios::binary|std::ios::trunc);out<<sentinel;}
 if(writer.fileExport(argv[2],DRW::AC1024,binary,&selected,false,true)||bytes(argv[2])!=sentinel)return 13;
 DxIfaceDimensionAngleTestAccess::setGeneration(origin,selected.importGeneration);
 if(!writer.fileExport(argv[2],DRW::AC1024,binary,&selected,false,true))return 14;
 const auto priorGeneration=data.importGeneration;
 auto* prior=dynamic_cast<dx_ifaceDwgDimensionOrigin*>(data.mBlock->ent.front());
 if(!DxIfaceDimensionAngleTestAccess::matches(prior,priorGeneration,DRW::AC1024))return 15;
 const auto retainedSource=DxIfaceDimensionAngleTestAccess::receipt(prior);
 if(!reader.fileImport(argv[5],&data,false) || data.importGeneration!=priorGeneration+1
    || !DxIfaceDimensionAngleTestAccess::matches(prior,priorGeneration,DRW::AC1024)
    || DxIfaceDimensionAngleTestAccess::receipt(prior)!=retainedSource)return 16;
 int plainCount=0;
 for(auto* entity:data.mBlock->ent) if(dynamic_cast<DRW_Dimension*>(entity)
    && !dynamic_cast<dx_ifaceDwgDimensionOrigin*>(entity))++plainCount;
 if(plainCount!=expectedCount)return 17;
 std::cout<<"physical DWG dimensions, graph, rollback, retry and DXF reimport verified\n";
 return 0;
}
