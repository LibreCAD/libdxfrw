#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <cstdio>
struct Capture : LC3CompatCheck {
 void addHatch(const DRW_Hatch* h) override {if(!h)return; ++count;printf("name=%s solid=%d angle=%.17g lineCount=%zu",h->name.c_str(),h->solid,h->angle,h->patternLines.size());for(const auto& l:h->patternLines)printf(" line=%.17g",l.angle);puts("");}
 int count=0;
};
int main(int argc,char**argv){if(argc!=2)return 2;Capture c;dwgRW r(argv[1]);bool ok=r.read(&c,false);printf("read=%d count=%d\n",int(ok),c.count);return ok&&c.count==1?0:1;}
