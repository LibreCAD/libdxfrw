// SPDX-License-Identifier: GPL-2.0-or-later
#include "tests/lc3_compat_check.cpp"
#include "libdxfrw.h"
#include <string>
struct Writer final : LC3CompatCheck {
 dxfRW& out; DRW_Light light; bool ok=false;
 explicit Writer(dxfRW& w,int color):out(w){light.m_type=2;light.m_name="LOCAL_LIGHT";light.m_status=true;light.m_color=7;light.m_colorRgb=color;light.m_intensity=2.5;light.m_position={1.,2.,3.};}
 void writeEntities() override {ok=out.writeLight(&light);}
 bool finalizeDxfWrite() override {return ok;}
};
int main(int argc,char** argv){if(argc!=4)return 2;dxfRW out(argv[1]);Writer writer(out,std::stoi(argv[3]));return out.write(&writer,DRW::AC1027,std::string(argv[2])=="binary")&&writer.ok?0:1;}
