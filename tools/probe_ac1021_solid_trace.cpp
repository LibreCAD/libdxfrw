#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <filesystem>
#include <string>
class Probe final:public LC3CompatCheck, public DRW_DwgDefaultMaterialProvider, public DRW_DwgDefaultPlotStyleProvider {
public:
 Probe(dwgRW& w,std::string s):out(w),mode(std::move(s)){}
 bool wantsDwgDefaultMaterialGraph()const override{return true;}
 bool wantsDwgDefaultPlotStyleGraph()const override{return true;}
 bool dwgDefaultMaterialOpaqueReferencesAbsent(std::uint32_t a,std::uint32_t b,std::uint32_t c,std::uint32_t d)const override{return a&&b&&c&&d;}
 template<class T> void init(T& e) {e.basePoint={1.,2.,3.};e.secPoint={4.,2.,3.};e.thirdPoint={1.,4.,3.};e.fourPoint={4.,4.,3.};e.extPoint=mode.find("tilt")!=std::string::npos?DRW_Coord(0.,1.,0.):DRW_Coord(0.,0.,1.);e.thickness=mode.find("thick")!=std::string::npos?.5:0.;}
 void writeEntities()override{bool b=false;if(mode.find("solid")==0){DRW_Solid e;init(e);b=out.writeSolid(&e);}else{DRW_Trace e;init(e);b=out.writeTrace(&e);}DRW_Line l;l.basePoint={10.,11.,0.};l.secPoint={12.,13.,0.};ok=b&&out.writeLine(&l);}
 bool finalizeDwgWrite()override{return ok;}
 dwgRW& out;std::string mode;bool ok=false;
};
int main(int argc,char**argv){if(argc!=3||std::filesystem::exists(argv[2]))return 2;dwgRW w(argv[2]);Probe p(w,argv[1]);return w.write(&p,DRW::AC1021,true)&&p.ok?0:1;}
