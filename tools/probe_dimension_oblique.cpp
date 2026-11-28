#include "dwg2dxf/dx_iface.h"
#include <iostream>
#include <string>
int main(int argc,char**argv) {
 if(argc!=4)return 2;
 const bool binary=std::string(argv[3])=="binary";
 dx_data source;dx_iface reader;
 if(!reader.fileImport(argv[1],&source,false))return 3;
 dx_data selected;dx_iface writer;writer.cData=&selected;writer.currentBlock=selected.mBlock;
 for(auto it=source.mBlock->ent.begin();it!=source.mBlock->ent.end();) {
  if(dynamic_cast<DRW_Dimension*>(*it)){(*it)->parentHandle=DRW::NoHandle;selected.mBlock->ent.push_back(*it);it=source.mBlock->ent.erase(it);}else ++it;
 }
 if(selected.mBlock->ent.size()!=7)return 4;
 selected.lineTypes=source.lineTypes;selected.layers=source.layers;selected.dimStyles=source.dimStyles;selected.textStyles=source.textStyles;
 for(auto it=source.blocks.begin();it!=source.blocks.end();) {
  auto* block=*it;
  if(block && block->name!="*Model_Space" && block->name!="*Paper_Space" && block->name!="*MODEL_SPACE" && block->name!="*PAPER_SPACE") {for(auto* entity:block->ent)if(entity)entity->parentHandle=DRW::NoHandle;selected.blocks.push_back(block);it=source.blocks.erase(it);}else ++it;
 }
 auto* first=dynamic_cast<DRW_DimLinear*>(selected.mBlock->ent.front());if(!first||first->getOblique()!=20)return 5;
 auto* named=new dx_ifaceBlock();named->name="ANGLE_BLOCK";named->handle=0xF610;
 auto* copy=new DRW_DimLinear(static_cast<const DRW_Dimension&>(*first));copy->handle=0xF611;named->ent.push_back(copy);selected.blocks.push_back(named);
 DRW_Group group;group.handle=0xF600;group.name="DIM_TARGET";group.m_entityHandles={first->handle};selected.groups.push_back(group);first->reactorHandles.push_back(group.handle);
 if(!writer.fileExport(argv[2],DRW::AC1024,binary,&selected,false,true))return 6;
 std::cout<<"selected DXF dimension graph written\n";return 0;
}
