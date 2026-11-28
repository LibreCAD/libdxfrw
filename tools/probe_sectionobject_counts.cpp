#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include <iostream>
struct Adapter:LC3CompatCheck{void addSectionObject(const DRW_SectionObject& s)override{std::cout<<"verts="<<s.m_verts.size()<<" back="<<s.m_blVerts.size()<<'\n';++count;}int count=0;};
int main(int argc,char**argv){if(argc!=2)return 2;dxfRW r(argv[1]);Adapter a;bool ok=r.read(&a,false);std::cout<<"read="<<ok<<" count="<<a.count<<'\n';return ok?0:1;}
