// SPDX-License-Identifier: GPL-2.0-or-later
#include "libdwgr.h"
#include "libdxfrw.h"
#include "lc3_compat_check.cpp"
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <vector>
namespace {
bool close(double a,double b){return std::fabs(a-b)<1e-10;}
bool point(const DRW_Coord& a,const DRW_Coord& b){return close(a.x,b.x)&&close(a.y,b.y)&&close(a.z,b.z);}
class DxfReader final:public LC3CompatCheck {public: void addMText(const DRW_MText& e) override{records.push_back(e);}std::vector<DRW_MText> records;};
bool source(const std::filesystem::path& path,std::vector<DRW_MText>& out){
    std::ofstream f(path);if(!f)return false;
    f<<"0\nSECTION\n2\nENTITIES\n";
    for(bool rotationLast:{true,false}){
        f<<"0\nMTEXT\n10\n3\n20\n4\n30\n2\n40\n2\n41\n10\n71\n1\n1\nHELLO\n210\n0\n220\n1\n230\n0\n";
        if(rotationLast) f<<"11\n0\n21\n0\n31\n1\n50\n30\n";
        else f<<"50\n30\n11\n0\n21\n0\n31\n1\n";
    }
    f<<"0\nENDSEC\n0\nEOF\n";f.close();
    DxfReader reader;dxfRW dxf(path.string().c_str());if(!dxf.read(&reader,false)||reader.records.size()!=2)return false;
    out=std::move(reader.records);return true;
}
class Writer final:public LC3CompatCheck {
public:
    Writer(dwgRW& w,const std::vector<DRW_MText>& parsed,bool skip):out(w),rotationLast(parsed[0]),directionLast(parsed[1]),skipInvalid(skip) {}
    void writeEntities() override {
        ok=false; DRW_MText invalid;invalid.text="invalid";invalid.height=2.;invalid.angle=30.;invalid.extPoint={0.,0.,0.};
        if(!skipInvalid && (out.writeMText(&invalid)||invalid.handle!=0u))return;
        invalid.extPoint={std::numeric_limits<double>::infinity(),0.,1.};
        if(!skipInvalid && (out.writeMText(&invalid)||invalid.handle!=0u))return;
        invalid.extPoint={0.,0.,1.}; invalid.angle=std::numeric_limits<double>::quiet_NaN();
        if(!skipInvalid && (out.writeMText(&invalid)||invalid.handle!=0u))return;
        invalid.angle=30.; if(!out.writeMText(&invalid))return;
        const std::array<DRW_Coord,3> normals{{{0.,0.,1.},{0.,1.,0.},{0.,0.,-1.}}};
        for(std::size_t i=0;i<normals.size();++i){
            DRW_MText text;text.text=std::to_string(i);text.height=2.;text.angle=30.;text.extPoint=normals[i];
            const auto before=text.secPoint;
            if(!out.writeMText(&text)||!point(text.secPoint,before)||text.angle!=30.)return;
        }
        DRW_MText nonunit;nonunit.text="nonunit";nonunit.height=2.;nonunit.angle=30.;nonunit.extPoint={0.,2.,0.};
        if(!out.writeMText(&nonunit)||!point(nonunit.extPoint,{0.,2.,0.})||!point(nonunit.secPoint,{0.,0.,0.}))return;
        DRW_MText legacy;legacy.text="legacy";legacy.height=2.;legacy.angle=30.;legacy.secPoint={0.,1.,0.};
        if(!out.writeMText(&legacy)||!point(legacy.secPoint,{0.,1.,0.}))return;
        DRW_MText explicit3d;explicit3d.text="legacy-3d";explicit3d.height=2.;explicit3d.angle=30.;
        explicit3d.extPoint={0.,1.,0.};explicit3d.secPoint={0.,0.,5.};
        if(!out.writeMText(&explicit3d)||!point(explicit3d.secPoint,{0.,0.,5.}))return;
        rotationLast.text="rotation-last";directionLast.text="direction-last";
        const auto before=rotationLast.secPoint;
        if(!out.writeMText(&rotationLast)||!out.writeMText(&directionLast)||!point(rotationLast.secPoint,before))return;
        DRW_MText editedDirection=rotationLast;editedDirection.text="edited-direction";editedDirection.handle=0;editedDirection.secPoint={0.,0.,-1.};
        DRW_MText editedAngle=directionLast;editedAngle.text="edited-angle";editedAngle.handle=0;editedAngle.angle=60.;
        // Copies retain orientation snapshots. Editing one public field selects it.
        if(!out.writeMText(&editedDirection)||!out.writeMText(&editedAngle)
           || !point(editedDirection.secPoint,{0.,0.,-1.}) || editedAngle.angle!=60.)return;
        DRW_Line next;next.basePoint={10.,11.,0.};next.secPoint={12.,13.,0.};ok=out.writeLine(&next);
    }
    bool finalizeDwgWrite() override{return ok;}
    dwgRW& out;DRW_MText rotationLast,directionLast;bool skipInvalid=false,ok=false;
};
class Reader final:public LC3CompatCheck {
public:void addMText(const DRW_MText& e) override{values[e.text]=e.secPoint;}
    void addLine(const DRW_Line& e) override{if(e.basePoint.x==10.&&e.secPoint.y==13.)++lines;}
    std::map<std::string,DRW_Coord> values;int lines=0;
};
}
int main(int argc,char** argv){
    const bool skipInvalid=argc==2 && std::string(argv[1])=="--geometry-only";
    if(argc>2 || (argc==2 && !skipInvalid)) return 2;
    const auto stem=std::filesystem::temp_directory_path()/("mtext-rotation-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const auto dxf=std::filesystem::path(stem.string()+".dxf"),dwg=std::filesystem::path(stem.string()+".dwg");
    struct Cleanup{std::filesystem::path a,b;~Cleanup(){std::error_code e;std::filesystem::remove(a,e);std::filesystem::remove(b,e);}}cleanup{dxf,dwg};
    std::vector<DRW_MText> parsed;if(!source(dxf,parsed)){std::cerr<<"DXF source failed\n";return 1;}
    const double c=std::cos(std::acos(-1.)/6.),s=.5;
    for(auto v:{DRW::AC1015,DRW::AC1018,DRW::AC1021,DRW::AC1024,DRW::AC1027,DRW::AC1032}){
        dwgRW w(dwg.string().c_str());Writer author(w,parsed,skipInvalid);if(!w.write(&author,v,true)||!author.ok){std::cerr<<"write failed "<<static_cast<int>(v)<<'\n';return 1;}
        dwgRW r(dwg.string().c_str());Reader read;if(!r.read(&read,false)||read.values.size()!=11u||read.lines!=1
           || !point(read.values["invalid"],{c,s,0.})||!point(read.values["0"],{c,s,0.})
           || !point(read.values["1"],{-c,0.,s})||!point(read.values["2"],{-c,s,0.})
           || !point(read.values["nonunit"],{-c,0.,s})|| !point(read.values["legacy"],{0.,1.,0.})||!point(read.values["legacy-3d"],{0.,0.,5.})
           || !point(read.values["rotation-last"],{-c,0.,s})
           || !point(read.values["direction-last"],{0.,0.,1.})
           || !point(read.values["edited-direction"],{0.,0.,-1.})
           || !point(read.values["edited-angle"],{-.5,0.,c})) {std::cerr<<"read failed "<<static_cast<int>(v)<<'\n';return 1;}
    }
}
