#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <cstdio>
struct Capture:LC3CompatCheck{int count=0;void addViewport(const DRW_Viewport& v) override{++count;printf("handle=%X snap=%.17g twist=%.17g dir=(%.17g,%.17g,%.17g) target=(%.17g,%.17g,%.17g) flags=%d\n",v.handle,v.snapAngle,v.twistAngle,v.viewDir.x,v.viewDir.y,v.viewDir.z,v.viewTarget.x,v.viewTarget.y,v.viewTarget.z,v.statusFlags);}};
int main(int n,char**v){if(n!=2)return 2;Capture c;dwgRW r(v[1]);bool ok=r.read(&c,false);printf("read=%d count=%d\n",int(ok),c.count);return c.count==1?0:1;}
