// SPDX-License-Identifier: GPL-2.0-or-later
#include "drw_dxf_extrusion.h"
#include "libdxfrw.h"
#include "lc3_compat_check.cpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {
bool eq(const DRW_Coord& a, const DRW_Coord& b) {
 return std::fabs(a.x-b.x)<1e-12 && std::fabs(a.y-b.y)<1e-12 && std::fabs(a.z-b.z)<1e-12;
}
struct Capture:LC3CompatCheck {
 void addLWPolyline(const DRW_LWPolyline& data) override { poly=data; ++count; }
 DRW_LWPolyline poly; int count=0;
};
bool conversion(DRW_LWPolyline& input, const std::vector<DRW_Coord>& expected) {
 std::vector<DRW_Coord> result{{9.,9.,9.}};
 if(!DRW_DxfGeometry::lwPolylineVerticesToWcs(input,result)||result.size()!=expected.size())return false;
 for(std::size_t i=0;i<result.size();++i)if(!eq(result[i],expected[i]))return false;
 return true;
}
bool run() {
 std::string doc="0\nSECTION\n2\nHEADER\n9\n$ACADVER\n1\nAC1027\n0\nENDSEC\n0\nSECTION\n2\nENTITIES\n0\nLWPOLYLINE\n100\nAcDbEntity\n8\n0\n100\nAcDbPolyline\n90\n3\n70\n0\n38\n2\n10\n0\n20\n0\n42\n0.25\n10\n5\n20\n0\n40\n0.5\n41\n1.0\n10\n5\n20\n3\n210\n0\n220\n1\n230\n0\n0\nENDSEC\n0\nEOF\n";
 Capture native,legacy;dxfRW reader(nullptr);
 if(!reader.readAscii(&native,false,doc)||native.count!=1)return false;
 if(!reader.readAscii(&legacy,true,doc)||legacy.count!=1)return false;
 const std::vector<DRW_Coord> tilt{{0.,2.,0.},{-5.,2.,0.},{-5.,2.,3.}};
 if(!conversion(native.poly,tilt))return false;
 if(native.poly.elevation!=2.||native.poly.extPoint.y!=1.
    ||native.poly.vertlist[0]->bulge!=.25||native.poly.vertlist[1]->stawidth!=.5
    ||native.poly.vertlist[1]->endwidth!=1.||native.poly.vertlist[2]->y!=3.)return false;
 // The existing ext=true callback still exposes its historical 2D projection.
 if(legacy.poly.elevation!=2.||legacy.poly.vertlist[0]->x!=0.||legacy.poly.vertlist[0]->y!=2.
    ||legacy.poly.vertlist[1]->x!=-5.||legacy.poly.vertlist[1]->y!=2.
    ||legacy.poly.vertlist[2]->x!=-5.||legacy.poly.vertlist[2]->y!=2.)return false;
 auto axial=native.poly;axial.extPoint={0.,0.,1.};
 if(!conversion(axial,{{0.,0.,2.},{5.,0.,2.},{5.,3.,2.}}))return false;
 axial.extPoint={0.,0.,-1.};
 if(!conversion(axial,{{0.,0.,-2.},{-5.,0.,-2.},{-5.,3.,-2.}}))return false;
 axial.extPoint={1.,1.,1.};
 if(!conversion(axial,{{1.1547005383792517,1.1547005383792517,1.1547005383792517},
                       {-2.3808333675534863,4.690234444311989,1.1547005383792517},
                       {-3.6055782389450757,3.4654895729204007,3.6041902811624302}}))return false;
 // Physical-independent arbitrary-axis threshold values from ezdxf OCS.
 for (int axis=0;axis<2;++axis) for (int boundary=0;boundary<2;++boundary) {
  const double t=boundary?1./64.:(1./64.-1e-8); const double z=std::sqrt(1.-t*t);
  axial.extPoint=axis==0?DRW_Coord(t,0.,z):DRW_Coord(0.,t,z);
  std::vector<DRW_Coord> expected;
  if(axis==0 && boundary)expected={{.03125,0.,1.9997558444720196},{.03125,5.,1.9997558444720196},{-2.9683837667080293,5.,2.04663084447202}};
  if(axis==0 && !boundary)expected={{.03124998,0.,1.9997558447845576},{5.030639591961394,0.,1.9216308947845575},{5.030639591961394,3.,1.9216308947845575}};
  if(axis==1 && boundary)expected={{0.,.03125,1.9997558444720196},{-5.,.03125,1.9997558444720196},{-5.,-2.9683837667080293,2.04663084447202}};
  if(axis==1 && !boundary)expected={{0.,.03124998,1.9997558447845576},{5.,.03124998,1.9997558447845576},{5.,3.030883747176837,1.9528808747845576}};
  if(!conversion(axial,expected))return false;
 }
 axial.extPoint={0.,2.,0.};if(!conversion(axial,tilt))return false;
 axial.extPoint={0.,std::numeric_limits<double>::denorm_min(),0.};if(!conversion(axial,tilt))return false;
 const double largest=std::numeric_limits<double>::max();
 auto extreme=native.poly;
 extreme.extPoint={-0.5598695611870558,-0.5866966310980342,0.585092417925947};
 extreme.elevation=0.14921939908714688*largest;
 for(auto& vertex:extreme.vertlist){
  vertex->x=-0.869681926769897*largest;
  vertex->y=0.9829157734821004*largest;
 }
 std::vector<DRW_Coord> extremeOutput;
 if(!DRW_DxfGeometry::lwPolylineVerticesToWcs(extreme,extremeOutput)
    ||extremeOutput.size()!=extreme.vertlist.size())return false;
 for(const auto& point:extremeOutput){
  if(!std::isfinite(point.x)||!std::isfinite(point.y)||!std::isfinite(point.z)
     ||std::fabs(point.x/largest-(-0.31568660864772236))>=1e-14
     ||std::fabs(point.y/largest-0.9289141273359566)>=1e-14
     ||std::fabs(point.z/largest-0.884419023840071)>=1e-14)return false;
 }
 for(int mutation=0;mutation<11;++mutation) {
  auto bad=native.poly;const double inf=std::numeric_limits<double>::infinity();
  if(mutation==0)bad.extPoint={0.,0.,0.};
  if(mutation==1)bad.extPoint.x=inf;
  if(mutation==2)bad.extPoint.y=std::numeric_limits<double>::quiet_NaN();
  if(mutation==3)bad.elevation=inf;
  if(mutation==4)bad.vertlist[1].reset();
  if(mutation==5)bad.vertlist[1]->x=inf;
  if(mutation==6)bad.eType=DRW::LINE;
  if(mutation==7){bad.extPoint={1.,1.,1.};bad.vertlist[0]->x=-std::numeric_limits<double>::max();bad.vertlist[0]->y=-std::numeric_limits<double>::max();}
  if(mutation==8){bad.extPoint={1e-200,0.,1e200};bad.elevation=1e308;}
  if(mutation==9){bad.extPoint={1e-310,0.,1.};bad.elevation=1e308;}
  if(mutation==10){bad.extPoint={1e-200,1e-200,1.};bad.vertlist[0]->y=1e308;}
  std::vector<DRW_Coord> output{{7.,8.,9.}};
  if(DRW_DxfGeometry::lwPolylineVerticesToWcs(bad,output)||output.size()!=1||!eq(output[0],{7.,8.,9.}))return false;
 }
 std::vector<DRW_Coord> output{{7.,8.,9.}};
 if(!DRW_DxfGeometry::lwPolylineVerticesToWcs(native.poly,output)||output.size()!=3)return false;
 return true;
}
}
int main(int argc,char** argv){
 if(argc==2){
  dxfRW reader(argv[1]);Capture capture;const bool complete=reader.read(&capture,false);
  const bool ok=capture.count==1&&conversion(capture.poly,{{0.,2.,0.},{-5.,2.,0.},{-5.,2.,3.}});
  std::cout<<"physical callback="<<ok<<" complete-read="<<complete<<'\n';return ok?0:1;
 }
 if(argc!=1)return 2;
 if(!run()){std::cerr<<"LWPOLYLINE WCS conversion failed\n";return 1;}return 0;
}
