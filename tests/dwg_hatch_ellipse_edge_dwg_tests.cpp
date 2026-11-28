// SPDX-License-Identifier: GPL-2.0-or-later
#include "libdwgr.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "lc3_compat_check.cpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>

namespace {
template<class T> class BoundaryProbe : public T {
public:
    using DRW_Hatch::encodeDwgBoundaryData;
};

template<class T> bool checkBoundary(DRW::Version version) {
    BoundaryProbe<T> h;
    auto loop=std::make_shared<DRW_HatchLoop>(1);
    auto line=std::make_shared<DRW_Line>(); line->basePoint={0.,1.,0.}; line->secPoint={2.,3.,0.};
    auto arc=std::make_shared<DRW_Arc>(); arc->basePoint={4.,5.,0.}; arc->radious=2.; arc->staangle=.25; arc->endangle=1.5; arc->isccw=1;
    auto ellipse=std::make_shared<DRW_Ellipse>(); ellipse->basePoint={6.,7.,0.}; ellipse->secPoint={4.,0.,0.}; ellipse->ratio=.5; ellipse->staparam=.25; ellipse->endparam=1.5; ellipse->isccw=1;
    auto spline=std::make_shared<DRW_Spline>(); spline->degree=2; spline->knotslist={0.,0.,0.,1.,1.,1.};
    spline->controllist={std::make_shared<DRW_Coord>(0.,0.,0.),std::make_shared<DRW_Coord>(1.,2.,0.),std::make_shared<DRW_Coord>(3.,0.,0.)};
    loop->objlist={line,arc,ellipse,spline}; loop->m_boundaryHandles={0x51u,0x52u}; loop->update(); h.appendLoop(loop);
    dwgBufferW out;
    if (!h.encodeDwgBoundaryData(version,&out,true)) return false;
    out.putRawLong32(0xA1B2C3D4u);
    dwgBuffer in(out.data().data(),out.data().size());
    if (in.getBitLong()!=1 || in.getBitLong()!=1 || in.getBitLong()!=4) return false;
    if (in.getRawChar8()!=1u || in.getRawDouble()!=0. || in.getRawDouble()!=1.
        || in.getRawDouble()!=2. || in.getRawDouble()!=3.) return false;
    if (in.getRawChar8()!=2u || in.getRawDouble()!=4. || in.getRawDouble()!=5.
        || in.getBitDouble()!=2. || in.getBitDouble()!=.25 || in.getBitDouble()!=1.5 || in.getBit()!=1u) return false;
    if (in.getRawChar8()!=3u || in.getRawDouble()!=6. || in.getRawDouble()!=7.
        || in.getRawDouble()!=4. || in.getRawDouble()!=0. || in.getBitDouble()!=.5
        || in.getBitDouble()!=.25 || in.getBitDouble()!=1.5 || in.getBit()!=1u) return false;
    if (in.getRawChar8()!=4u || in.getBitLong()!=2 || in.getBit()!=0u || in.getBit()!=0u
        || in.getBitLong()!=6 || in.getBitLong()!=3) return false;
    for (double expected : {0.,0.,0.,1.,1.,1.}) if (in.getBitDouble()!=expected) return false;
    for (double expected : {0.,0.,1.,2.,3.,0.}) if (in.getRawDouble()!=expected) return false;
    if (version > DRW::AC1021) {
        if (in.getBitLong()!=0) return false;
        for (int i=0; i<4; ++i) if (in.getRawDouble()!=0.) return false;
    }
    return in.getBitLong()==2 && in.getRawLong32()==0xA1B2C3D4u && in.isGood();
}

class Writer final : public LC3CompatCheck {
public:
    explicit Writer(dwgRW& w):out(w) {}
    void writeEntities() override {
        DRW_Hatch h; h.name="SOLID"; h.solid=1;
        auto loop=std::make_shared<DRW_HatchLoop>(1);
        auto ellipse=std::make_shared<DRW_Ellipse>(); ellipse->basePoint={3.,4.,0.}; ellipse->secPoint={4.,0.,0.};
        ellipse->ratio=.5; ellipse->staparam=.25; ellipse->endparam=1.5; ellipse->isccw=1;
        loop->objlist.push_back(nullptr); loop->update(); h.appendLoop(loop);
        const bool rejected=!out.writeHatch(&h) && h.handle==0;
        loop->objlist[0]=ellipse;
        const bool written=out.writeHatch(&h);
        DRW_Line next; next.basePoint={10.,11.,0.}; next.secPoint={12.,13.,0.};
        ok=rejected && written && out.writeLine(&next);
    }
    bool finalizeDwgWrite() override {return ok;}
    dwgRW& out; bool ok=false;
};
class Reader final : public LC3CompatCheck {
public:
    void addHatch(const DRW_Hatch* h) override {
        if (!h || h->looplist.size()!=1 || h->looplist[0]->objlist.size()!=1) return;
        const auto* e=dynamic_cast<const DRW_Ellipse*>(h->looplist[0]->objlist[0].get());
        found=e && e->basePoint.x==3. && e->basePoint.y==4. && e->secPoint.x==4.
            && e->secPoint.y==0. && e->ratio==.5 && e->staparam==.25 && e->endparam==1.5 && e->isccw==1;
    }
    void addLine(const DRW_Line& l) override {if(l.basePoint.x==10.&&l.secPoint.y==13.) ++lines;}
    bool found=false; int lines=0;
};
}
int main(){
    for(auto version:{DRW::AC1015,DRW::AC1018,DRW::AC1021,DRW::AC1024,DRW::AC1027,DRW::AC1032}) {
        if(!checkBoundary<DRW_Hatch>(version)||!checkBoundary<DRW_MPolygon>(version)) {
            std::cerr<<"boundary fields failed revision "<<static_cast<int>(version)<<'\n'; return 1;
        }
    }
    const auto path=std::filesystem::temp_directory_path()/ ("dwg-hatch-ellipse-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dwg");
    struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove(p,e);}}cleanup{path};
    for(auto version:{DRW::AC1015,DRW::AC1018,DRW::AC1021,DRW::AC1024,DRW::AC1027,DRW::AC1032}) {
        dwgRW out(path.string().c_str()); Writer writer(out);
        if(!out.write(&writer,version,true)||!writer.ok) {std::cerr<<"write failed "<<static_cast<int>(version)<<'\n';return 1;}
        dwgRW in(path.string().c_str()); Reader reader;
        if(!in.read(&reader,false)||!reader.found||reader.lines!=1) {std::cerr<<"read failed "<<static_cast<int>(version)<<'\n';return 1;}
    }
    return 0;
}
