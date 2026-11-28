#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <cstdio>
struct Capture : LC3CompatCheck {
  void addPolyline(const DRW_Polyline& p) override {
    ++count; std::printf("parent=%X flags=%d vertices=%zu wire_handles=%zu seqend=%X\n",p.handle,p.flags,p.vertlist.size(),p.dwgHandleListSize(),p.dwgSeqEndHandle());
    for(const auto& v:p.vertlist) if(v) std::printf("vertex=%X owner=%X point=(%.17g,%.17g,%.17g) tangent=%.17g flags=%d\n",v->handle,v->parentHandle,v->basePoint.x,v->basePoint.y,v->basePoint.z,v->tgdir,v->flags);
  }
  int count=0;
};
int main(int argc,char**argv) {if(argc!=2)return 2; Capture c;dwgRW r(argv[1]);bool ok=r.read(&c,false);std::printf("read=%d callbacks=%d\n",int(ok),c.count);return ok&&c.count==1?0:1;}
