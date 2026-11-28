#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include <iostream>
class Reader:public LC3CompatCheck{public:void addEllipse(const DRW_Ellipse& e)override{value=e;++count;}DRW_Ellipse value;int count=0;};
class Writer:public LC3CompatCheck,public DRW_DxfR12CcwEllipseOptIn{public:Writer(dxfRW& w,DRW_Ellipse& e):out(w),ellipse(e){}void writeEntities()override{ok=out.writeEllipse(&ellipse);}bool finalizeDxfWrite()override{return ok;}dxfRW& out;DRW_Ellipse& ellipse;bool ok=false;};
int main(int argc,char** argv){if(argc!=3)return 2;dxfRW input(argv[1]);Reader source;if(!input.read(&source,false)||source.count!=1)return 1;dxfRW output(argv[2]);Writer writer(output,source.value);if(!output.write(&writer,DRW::AC1009,false)||!writer.ok)return 1;std::cout<<"complete selected-source import and opt-in R12 export\n";return 0;}
