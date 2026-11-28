#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include <iostream>
struct Adapter:LC3CompatCheck {dxfRW& out;bool ok=false;explicit Adapter(dxfRW& w):out(w){} void writeEntities()override{DRW_SectionObject s;s.m_state=1;s.m_flags=1;s.m_name="SECT_A";s.m_vertDir={0,0,1};s.m_topHeight=2;s.m_bottomHeight=-2;s.m_indicatorAlpha=64;s.m_indicatorColor=7;s.m_verts={{0,0,0},{4,0,0}};s.m_blVerts={{0,1,0},{4,1,0}};ok=out.writeSectionObject(&s);}bool finalizeDxfWrite()override{return ok;}};
int main(int argc,char**argv){if(argc!=2)return 2;dxfRW out(argv[1]);Adapter a(out);if(!out.write(&a,DRW::AC1027,false)||!a.ok)return 1;}
