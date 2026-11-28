#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include <iostream>
struct Adapter:LC3CompatCheck{void addSectionObject(const DRW_SectionObject& e)override{entity=e;count++;}void addRawDxfEntity(const DRW_RawDxfObject& r)override{for(const auto& v:r.groups)if(v.code()==411){if(v.type()==DRW_Variant::STRING)std::cout<<"raw411="<<v.c_str()<<'\n';++raw;}}void writeEntities()override{ok=out->writeSectionObject(&entity);}DRW_SectionObject entity;dxfRW*out=nullptr;int count=0,raw=0;bool ok=false;};
int main(int argc,char**argv){if(argc!=3)return 2;Adapter a;dxfRW input(argv[1]);bool read=input.read(&a,false);a.entity.handle=0;a.entity.parentHandle=DRW::NoHandle;dxfRW output(argv[2]);a.out=&output;bool written=output.write(&a,DRW::AC1027,false);std::cout<<"read="<<read<<" count="<<a.count<<" raw="<<a.raw<<" write="<<written<<" entity="<<a.ok<<'\n';return 0;}
