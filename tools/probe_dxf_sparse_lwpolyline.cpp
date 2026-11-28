// SPDX-License-Identifier: GPL-2.0-or-later
// Read the locally authored ezdxf sparse-vertex witness with libdxfrw.
#include "../tests/lc3_compat_check.cpp"
#include "libdxfrw.h"
#include <iostream>

struct Capture : LC3CompatCheck {
    void addLWPolyline(const DRW_LWPolyline& line) override {
        ++polylines;
        if(line.vertlist.size()!=3u) return;
        const auto& a=*line.vertlist[0];const auto& b=*line.vertlist[1];const auto& c=*line.vertlist[2];
        valid=a.x==1.&&a.y==2.&&a.stawidth==0.&&a.endwidth==0.&&a.bulge==0.
            &&b.x==3.&&b.y==4.&&b.stawidth==.4&&b.endwidth==.8&&b.bulge==-.5
            &&c.x==5.&&c.y==6.&&c.stawidth==0.&&c.endwidth==0.&&c.bulge==.25;
    }
    void addLine(const DRW_Line& line) override {
        ++lines;lineValid=line.basePoint.x==11.&&line.secPoint.z==16.;
    }
    int polylines=0,lines=0;bool valid=false,lineValid=false;
};
int main(int argc,char** argv) {
    if(argc!=2){std::cerr<<"usage: probe_dxf_sparse_lwpolyline <scratch.dxf>\n";return 2;}
    dxfRW reader(argv[1]);Capture capture;
    const bool read=reader.read(&capture,false);
    if(!read||capture.polylines!=1||capture.lines!=1||!capture.valid||!capture.lineValid) {
        std::cerr<<"read="<<read<<" polylines="<<capture.polylines<<" lines="<<capture.lines
                 <<" fields="<<capture.valid<<" following="<<capture.lineValid<<'\n';return 1;
    }
    std::cout<<"libdxfrw sparse per-vertex source PASS\n";return 0;
}
