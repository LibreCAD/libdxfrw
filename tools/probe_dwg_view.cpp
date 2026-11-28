#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <cstdio>
struct Capture:LC3CompatCheck{int count=0;void addView(const DRW_View& v) override{if(v.name=="ANGLECHECK"){++count;printf("twist=%.17g dir=(%.17g,%.17g,%.17g) target=(%.17g,%.17g,%.17g)\n",v.twistAngle,v.viewDirectionFromTarget.x,v.viewDirectionFromTarget.y,v.viewDirectionFromTarget.z,v.targetPoint.x,v.targetPoint.y,v.targetPoint.z);}}};
int main(int n,char**v){if(n!=2)return 2;Capture c;dwgRW r(v[1]);bool ok=r.read(&c,false);printf("read=%d count=%d\n",int(ok),c.count);return c.count==1?0:1;}
