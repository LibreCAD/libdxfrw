#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include <iostream>
struct Adapter:LC3CompatCheck{void addRawDxfEntity(const DRW_RawDxfObject& r)override{if(r.name=="SECTIONOBJECT"){raw=r;++count;}}void writeEntities()override{ok=out->writeRawDxfEntity(&raw,DRW::DxfModelSpaceBlockRecordHandle);}DRW_RawDxfObject raw;dxfRW*out=nullptr;int count=0;bool ok=false;};
int main(int argc,char**argv){if(argc!=3)return 2;Adapter a;dxfRW input(argv[1]);bool read=input.read(&a,false);dxfRW output(argv[2]);a.out=&output;bool written=output.write(&a,DRW::AC1027,false);std::cout<<"read="<<read<<" count="<<a.count<<" write="<<written<<" raw="<<a.ok<<'\n';return read&&a.count==1&&written&&a.ok?0:1;}
