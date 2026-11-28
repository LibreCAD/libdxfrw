// SPDX-License-Identifier: GPL-2.0-or-later
#include "tests/lc3_compat_check.cpp"
#include "libdxfrw.h"
#include <iostream>
#include <string>
#include <vector>
struct Reader final:LC3CompatCheck {std::vector<DRW_Light> lights;void addLight(const DRW_Light& v)override{lights.push_back(v);}};
struct Writer final:LC3CompatCheck {dxfRW& output;DRW_Light light;bool ok=false;explicit Writer(dxfRW& w):output(w){}void writeEntities()override{ok=output.writeLight(&light);}bool finalizeDxfWrite()override{return ok;}};
int main(int argc,char**argv){if(argc!=4)return 2;dxfRW in(argv[1]);Reader r;if(!in.read(&r,false)||r.lights.size()!=1u)return 1;
 auto& light=r.lights[0];std::cerr<<"version="<<int(light.proxyGraphicsProvenance().sourceVersion)<<" codepage="<<light.proxyGraphicsProvenance().sourceCodePage<<" count="<<light.numProxyGraph<<"\n";
 if(!light.authorizeProxyGraphicsReplay())return 1;dxfRW out(argv[2]);Writer w(out);w.light=light;return out.write(&w,DRW::AC1027,std::string(argv[3])=="binary")&&w.ok?0:1;}
