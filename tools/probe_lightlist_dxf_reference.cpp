// SPDX-License-Identifier: GPL-2.0-or-later
#include "tests/lc3_compat_check.cpp"
#include "libdxfrw.h"
#include <iostream>
#include <vector>
struct Reader final : LC3CompatCheck {
 void addLightList(const DRW_LightList& v) override {lists.push_back(v);}
 void addRawDxfObject(const DRW_RawDxfObject& v) override {if(v.name=="LIGHTLIST")raw.push_back(v);}
 std::vector<DRW_LightList> lists;std::vector<DRW_RawDxfObject> raw;
};
int main(int argc,char**argv){if(argc!=2)return 2;dxfRW input(argv[1]);Reader r;const bool complete=input.read(&r,false);
 std::uint32_t dict=0u;const bool observed=r.lists.size()==1u && r.raw.size()==1u && r.lists[0].handle==0xF000u
 && r.lists[0].parentHandle==0x30u && r.lists[0].getAcLightDictionaryHandle(dict)&&dict==0x30u
 && r.lists[0].m_lights.size()==1u && r.lists[0].m_lights[0].m_handle==0x2Fu
 && r.lists[0].m_lights[0].m_name=="LOCAL_LIGHT" && r.raw[0].handle==0xF000u;
 std::cout<<"complete="<<complete<<" selected LIGHTLIST="<<(observed?"PASS":"FAIL")<<'\n';return observed?0:1;}
