#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <cstdio>
struct Capture:LC3CompatCheck{int count=0;void addVport(const DRW_Vport& v) override{if(v.name=="*Active"){++count;printf("snap=%.17g twist=%.17g\n",v.snapAngle,v.twistAngle);}}};
int main(int n,char**v){if(n!=2)return 2;Capture c;dwgRW r(v[1]);bool ok=r.read(&c,false);printf("read=%d count=%d\n",int(ok),c.count);return c.count==1?0:1;}
