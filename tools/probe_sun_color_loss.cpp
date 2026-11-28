#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include <iostream>
struct Adapter:LC3CompatCheck{void addSun(const DRW_Sun& s)override{sun=s;++count;}void writeObjects()override{ok=out->writeSun(&sun);}DRW_Sun sun;dxfRW*out=nullptr;int count=0;bool ok=false;};
int main(int argc,char**argv){if(argc!=3)return 2;Adapter a;dxfRW in(argv[1]);bool r=in.read(&a,false);a.sun.handle=0;a.sun.parentHandle=0;dxfRW out(argv[2]);a.out=&out;bool w=out.write(&a,DRW::AC1027,false);std::cout<<"read="<<r<<" count="<<a.count<<" aci="<<a.sun.m_color<<" rgb="<<a.sun.m_color24<<" write="<<w<<" object="<<a.ok<<'\n';return r&&a.count==1&&w&&a.ok?0:1;}
