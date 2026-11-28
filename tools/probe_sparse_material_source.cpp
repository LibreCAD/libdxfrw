// SPDX-License-Identifier: GPL-2.0-or-later
#include "tests/lc3_compat_check.cpp"
#include "libdxfrw.h"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
struct Reader final:LC3CompatCheck,DRW_DxfSparseMaterialRawPolicy {
 bool wantsDxfSparseMaterialRawCapture() const noexcept override{return true;}
 void addMaterial(const DRW_Material& v)override{typed.push_back(v);}
 void addRawDxfObject(const DRW_RawDxfObject& v)override{if(v.name=="MATERIAL")raw.push_back(v);}
 std::vector<DRW_Material> typed;std::vector<DRW_RawDxfObject> raw;
};
int main(int argc,char**argv){if(argc!=2)return 2;dxfRW input(argv[1]);Reader r;
 if(!input.read(&r,false)||r.typed.size()!=3u||r.raw.size()!=3u)return 1;
 const std::vector<int> codes={5,102,330,102,330,100,1,71,91,72,273,94};
 for(const char* name:{"ByBlock","ByLayer","Global"}){
  const auto m=std::find_if(r.typed.begin(),r.typed.end(),[name](const DRW_Material& v){return v.m_name==name;});
  if(m==r.typed.end()||!m->requiresRawDxfMaterialReplay()||m->m_hasRefractionMapSource||!m->m_hasChannelFlags||m->m_channelFlags!=63||m->m_diffuseColorValue!=0xC2FFFFFFu)return 1;
  const auto raw=std::find_if(r.raw.begin(),r.raw.end(),[&](const DRW_RawDxfObject& v){return v.handle==m->handle;});
  if(raw==r.raw.end())return 1;std::vector<int> got;for(const auto& g:raw->groups)got.push_back(g.code());if(got!=codes)return 1;
 }
 std::cout<<"AC1027 sparse MATERIAL source capture PASS\n";return 0;}
