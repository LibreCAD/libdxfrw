#include "drw_entities.h"
#include "intern/dwgbufferw.h"
#include <iomanip>
#include <iostream>
#include <memory>
template<class T> struct Probe:T {using T::encodeDwg;};
template<class T> bool run(DRW::Version ver,bool patterned,bool grad){
 Probe<T> h; h.name=patterned?"ANSI31":"SOLID";h.solid=patterned?0:1;h.angle=30./ARAD;h.scale=1.;DRW_Hatch::PatternLine line;line.angle=75./ARAD;line.offsetX=1.;line.offsetY=2.;if(patterned)h.patternLines.push_back(line); if(grad){h.isGradient=1;h.gradAngle=.25;h.gradName="LINEAR";}
 auto path=std::make_shared<DRW_HatchLoop>(1);auto arc=std::make_shared<DRW_Arc>();arc->basePoint={1,2,0};arc->radious=3;arc->staangle=.25;arc->endangle=1.5;auto ellipse=std::make_shared<DRW_Ellipse>();ellipse->basePoint={2,3,0};ellipse->secPoint={4,0,0};ellipse->ratio=.5;ellipse->staparam=.25;ellipse->endparam=1.5;path->objlist={arc,ellipse};path->update();h.appendLoop(path);
 dwgBufferW b,s,handles;if(!h.encodeDwg(ver,&b,0,&s,&handles))return false;
 for(const auto* x:{&b,&s,&handles}){std::cout<<x->bitCount()<<":";for(auto c:x->data())std::cout<<std::hex<<std::setw(2)<<std::setfill('0')<<int(c);std::cout<<std::dec<<" ";}std::cout<<"\n";return true;
}
int main(){for(auto v:{DRW::AC1015,DRW::AC1018,DRW::AC1021,DRW::AC1024,DRW::AC1027,DRW::AC1032})for(int i=0;i<3;i++)if(!(i==0?run<DRW_MPolygon>(v,true,false):run<DRW_Hatch>(v,false,i==2)))return 1;}
