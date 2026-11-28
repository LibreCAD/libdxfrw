// SPDX-License-Identifier: GPL-2.0-or-later
#include "libdxfrw.h"
#include "lc3_compat_check.cpp"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>

namespace {
constexpr double turn = M_PIx2;
struct Writer : LC3CompatCheck {
    explicit Writer(dxfRW& w) : output(w) {}
    void writeEntities() override { accepted = output.writeEllipse(ellipse); }
    bool finalizeDxfWrite() override { return accepted; }
    dxfRW& output; DRW_Ellipse* ellipse = nullptr; bool accepted = false;
};
struct CcwWriter : Writer, DRW_DxfR12CcwEllipseOptIn { using Writer::Writer; };
struct CircleWriter : LC3CompatCheck {
    explicit CircleWriter(dxfRW& w):output(w){}
    void writeEntities() override {accepted=output.writeCircle(circle);}
    bool finalizeDxfWrite() override {return accepted;}
    dxfRW& output; DRW_Circle* circle=nullptr; bool accepted=false;
};
struct Reader : LC3CompatCheck {
    void addPolyline(const DRW_Polyline& value) override { polyline = value; ++count; }
    void addEllipse(const DRW_Ellipse& value) override { ellipse = value; ++ellipses; }
    DRW_Polyline polyline; DRW_Ellipse ellipse; int count = 0, ellipses = 0;
};
std::string contents(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}
DRW_Coord point(const DRW_Ellipse& e, double angle, bool threeD) {
    const double x = e.basePoint.x + e.secPoint.x * std::cos(angle);
    const double minor = e.secPoint.x * e.ratio * std::sin(angle);
    return threeD ? DRW_Coord(x, e.basePoint.y, e.basePoint.z - minor)
                  : DRW_Coord(x, e.basePoint.y + minor, 0.);
}
bool near(const DRW_Coord& a, const DRW_Coord& b) {
    return std::abs(a.x-b.x) < 1.e-7 && std::abs(a.y-b.y) < 1.e-7
        && std::abs(a.z-b.z) < 1.e-7;
}
bool checkPolyline(const DRW_Polyline& p, const DRW_Ellipse& e, bool threeD,
                   bool closed, std::size_t count) {
    if (p.vertlist.size() != count || ((p.flags & 1) != 0) != closed
        || ((p.flags & 8) != 0) != threeD || !p.vertlist.front()) return false;
    if (!closed && (!p.vertlist.back()
        || !near(p.vertlist.front()->basePoint, point(e, e.staparam, threeD))
        || !near(p.vertlist.back()->basePoint, point(e, e.endparam, threeD)))) return false;
    return true;
}
bool run(const std::filesystem::path& path, bool binary, bool threeD, bool opted,
         double start, double end, bool closed, std::size_t count) {
    DRW_Ellipse ellipse; ellipse.basePoint = threeD ? DRW_Coord(2.,3.,4.) : DRW_Coord(2.,3.,0.);
    ellipse.secPoint = {1.e8,0.,0.}; ellipse.extPoint = threeD ? DRW_Coord(0.,1.,0.) : DRW_Coord(0.,0.,1.);
    ellipse.ratio = .5; ellipse.staparam = start; ellipse.endparam = end; ellipse.isccw = 1;
    // Direct API uses its legacy signed sweep; the opt-in is a writer policy.
    DRW_Polyline direct; ellipse.toPolyline(&direct,128);
    const bool directClosed = start == end || end == start + turn || end == start - turn;
    const std::size_t directCount = directClosed ? 128u :
        static_cast<std::size_t>(std::max(1., std::ceil(128. * std::abs(end-start) / turn))) + 1u;
    if (!checkPolyline(direct,ellipse,threeD,directClosed,directCount)) return false;
    dxfRW output(path.string().c_str()); Writer legacy(output); CcwWriter ccw(output);
    Writer* writer = opted ? static_cast<Writer*>(&ccw) : &legacy;
    writer->ellipse = &ellipse;
    if (!output.write(writer,DRW::AC1009,binary) || !writer->accepted
        || ellipse.staparam != start || ellipse.endparam != end) return false;
    dxfRW input(path.string().c_str()); Reader reader;
    if (!input.read(&reader,false) || reader.count != 1
        || !checkPolyline(reader.polyline,ellipse,threeD,closed,count)) return false;
    dxfRW extrusionInput(path.string().c_str()); Reader extrusionReader;
    return extrusionInput.read(&extrusionReader,true) && extrusionReader.count == 1
        && checkPolyline(extrusionReader.polyline,ellipse,threeD,closed,count);
}
bool invalidRetry(const std::filesystem::path& path, bool binary) {
    {std::ofstream existing(path,std::ios::binary|std::ios::trunc); existing << "sentinel";}
    DRW_Ellipse ellipse; ellipse.secPoint={2.,0.,0.}; ellipse.ratio=.5;
    ellipse.staparam=.5; ellipse.endparam=.5+5.e-11;
    ellipse.extPoint={0.,0.,0.};
    dxfRW output(path.string().c_str()); Writer writer(output); writer.ellipse=&ellipse;
    if (output.write(&writer,DRW::AC1009,binary) || writer.accepted || contents(path)!="sentinel") return false;
    ellipse.extPoint={0.,0.,1.};
    if (!output.write(&writer,DRW::AC1009,binary) || !writer.accepted) return false;
    dxfRW input(path.string().c_str()); Reader reader;
    if (!input.read(&reader,false) || reader.count != 1 || reader.polyline.vertlist.size() != 2
        || (reader.polyline.flags & 1) != 0) return false;
    {std::ofstream existing(path,std::ios::binary|std::ios::trunc); existing << "sentinel";}
    ellipse.staparam = 0.; ellipse.endparam = turn + 5.e-11;
    if (output.write(&writer,DRW::AC1009,binary) || writer.accepted || contents(path)!="sentinel") return false;
    ellipse.endparam = turn - 5.e-11;
    return output.write(&writer,DRW::AC1009,binary) && writer.accepted;
}
bool scaledNear(const DRW_Coord& a, const DRW_Coord& b, double scale) {
    return std::abs(a.x/scale-b.x/scale)<1.e-13
        && std::abs(a.y/scale-b.y/scale)<1.e-13
        && std::abs(a.z/scale-b.z/scale)<1.e-13;
}
bool extreme(const std::filesystem::path& path, bool binary, bool threeD,
             double scale, double normalScale, bool closed) {
    DRW_Ellipse e;
    e.basePoint = threeD ? DRW_Coord(2*scale,3*scale,4*scale) : DRW_Coord(2*scale,3*scale,0.);
    e.secPoint={scale,0.,0.};
    e.extPoint=threeD ? DRW_Coord(0.,normalScale,0.) : DRW_Coord(0.,0.,normalScale);
    e.ratio=.5; e.staparam=0.; e.endparam=closed ? turn : turn/4.;
    const std::size_t expected=closed ? 8u : 3u;
    const auto check = [&](const DRW_Polyline& p) {
        if (p.vertlist.size()!=expected || ((p.flags&1)!=0)!=closed
            || ((p.flags&8)!=0)!=threeD || !p.vertlist.front() || !p.vertlist.back()) return false;
        return scaledNear(p.vertlist.front()->basePoint,point(e,0.,threeD),scale)
            && (closed || scaledNear(p.vertlist.back()->basePoint,point(e,turn/4.,threeD),scale));
    };
    DRW_Polyline direct; e.toPolyline(&direct,8);
    if (!check(direct)) return false;
    dxfRW output(path.string().c_str()); output.setEllipseParts(8); Writer writer(output); writer.ellipse=&e;
    if (!output.write(&writer,DRW::AC1009,binary) || !writer.accepted) return false;
    for (bool applyExtrusion : {false,true}) {
        dxfRW input(path.string().c_str()); Reader reader;
        if (!input.read(&reader,applyExtrusion) || reader.count!=1 || !check(reader.polyline)) return false;
    }
    return true;
}
bool extremeArithmetic(const std::filesystem::path& path, bool binary) {
    DRW_Ellipse e; e.basePoint={1.5e308,0.,0.}; e.secPoint={1.1e308,1.1e308,0.};
    e.extPoint={0.,0.,1.}; e.ratio=1.; e.staparam=turn/8.-.01; e.endparam=turn/8.+.01;
    DRW_Polyline direct; e.toPolyline(&direct,8);
    if (direct.vertlist.size()!=2u || !std::isfinite(direct.vertlist[0]->basePoint.x)
        || !std::isfinite(direct.vertlist[1]->basePoint.x)) return false;
    dxfRW output(path.string().c_str()); output.setEllipseParts(8); Writer writer(output); writer.ellipse=&e;
    if (!output.write(&writer,DRW::AC1009,binary) || !writer.accepted) return false;
    dxfRW input(path.string().c_str()); Reader reader;
    if (!input.read(&reader,false) || reader.count!=1 || reader.polyline.vertlist.size()!=2u
        || !scaledNear(reader.polyline.vertlist[0]->basePoint,direct.vertlist[0]->basePoint,1.e308)
        || !scaledNear(reader.polyline.vertlist[1]->basePoint,direct.vertlist[1]->basePoint,1.e308)) return false;
    const double max=std::numeric_limits<double>::max();
    e.basePoint={max,0.,0.}; e.secPoint={max/2.,0.,0.};e.ratio=.5;e.staparam=0.;e.endparam=turn;
    {std::ofstream existing(path,std::ios::binary|std::ios::trunc); existing<<"sentinel";}
    if (output.write(&writer,DRW::AC1009,binary) || writer.accepted || contents(path)!="sentinel") return false;
    e.basePoint={0.,0.,0.};
    if (!output.write(&writer,DRW::AC1009,binary) || !writer.accepted) return false;
    // The normal's mathematical norm overflows but its unit direction and
    // generated 3D coordinates are finite.
    e.secPoint={1.,-1.,0.}; e.extPoint={1.5e308,1.5e308,0.}; e.ratio=.5;
    e.staparam=.2; e.endparam=.4;
    direct.vertlist.clear(); e.toPolyline(&direct,8);
    if (!output.write(&writer,DRW::AC1009,binary) || !writer.accepted) return false;
    dxfRW normalInput(path.string().c_str());Reader normalRead;
    const DRW_Coord expectedNormal(std::cos(.2),-std::cos(.2),-std::sqrt(.5)*std::sin(.2));
    if (direct.vertlist.size()!=2u || (direct.flags&8)==0
        || !scaledNear(direct.vertlist.front()->basePoint,expectedNormal,1.)
        || !normalInput.read(&normalRead,false) || normalRead.count!=1
        || normalRead.polyline.vertlist.size()!=2u || (normalRead.polyline.flags&8)==0
        || !scaledNear(normalRead.polyline.vertlist.front()->basePoint,expectedNormal,1.)) return false;
    if (!output.write(&writer,DRW::AC1027,binary) || !writer.accepted) return false;
    dxfRW modernNormalInput(path.string().c_str());Reader modernNormalRead;
    if (!modernNormalInput.read(&modernNormalRead,false) || modernNormalRead.ellipses!=1
        || modernNormalRead.ellipse.extPoint.x!=e.extPoint.x
        || modernNormalRead.ellipse.extPoint.y!=e.extPoint.y) return false;
    // Both major components are finite even though their combined norm is not.
    e.secPoint={1.5e308,1.5e308,0.}; e.extPoint={0.,0.,1.};
    direct.vertlist.clear(); e.toPolyline(&direct,8);
    const DRW_Coord expectedMajor(1.5e308*std::cos(.2)-.75e308*std::sin(.2),
                                  1.5e308*std::cos(.2)+.75e308*std::sin(.2),0.);
    if (direct.vertlist.size()!=2u || !scaledNear(direct.vertlist.front()->basePoint,expectedMajor,1.e308)
        || !output.write(&writer,DRW::AC1009,binary) || !writer.accepted) return false;
    dxfRW majorInput(path.string().c_str());Reader majorRead;
    if (!majorInput.read(&majorRead,false) || majorRead.count!=1 || majorRead.polyline.vertlist.size()!=2u
        || !scaledNear(majorRead.polyline.vertlist.front()->basePoint,expectedMajor,1.e308)) return false;
    if (!output.write(&writer,DRW::AC1027,binary) || !writer.accepted) return false;
    dxfRW modernMajorInput(path.string().c_str());Reader modernMajorRead;
    if (!modernMajorInput.read(&modernMajorRead,false) || modernMajorRead.ellipses!=1
        || modernMajorRead.ellipse.secPoint.x!=e.secPoint.x
        || modernMajorRead.ellipse.secPoint.y!=e.secPoint.y) return false;
    // A small angular tilt is not a small WCS displacement at this scale.
    // A 2D classic POLYLINE would let consumers discard the resulting Z.
    e.extPoint={5.e-13,-5.e-13,1.}; e.ratio=.5;
    direct.vertlist.clear(); e.toPolyline(&direct,8);
    const double tiltedZ=(1.5e308*5.e-13)*std::sin(.2);
    if (direct.vertlist.size()!=2u || (direct.flags&8)==0
        || std::abs(direct.vertlist[0]->basePoint.z/tiltedZ-1.)>1.e-13
        || !output.write(&writer,DRW::AC1009,binary) || !writer.accepted) return false;
    for (bool applyExtrusion : {false,true}) {
        dxfRW tiltedInput(path.string().c_str());Reader tiltedRead;
        if (!tiltedInput.read(&tiltedRead,applyExtrusion) || tiltedRead.count!=1
            || (tiltedRead.polyline.flags&8)==0 || tiltedRead.polyline.vertlist.size()!=2u
            || std::abs(tiltedRead.polyline.vertlist[0]->basePoint.z/tiltedZ-1.)>1.e-13) return false;
    }
    // The major norm overflows while ratio is subnormal; the orthogonal Z
    // component remains ordinary-sized and must not be lost to 0 or infinity.
    e.extPoint={1.,-1.,0.}; e.ratio=1.e-308;
    direct.vertlist.clear(); e.toPolyline(&direct,8);
    const double expectedZ=1.5*std::sqrt(2.)*std::sin(.2);
    if (direct.vertlist.size()!=2u || std::abs(direct.vertlist[0]->basePoint.z-expectedZ)>1.e-14
        || !output.write(&writer,DRW::AC1009,binary) || !writer.accepted) return false;
    dxfRW tinyRatioInput(path.string().c_str()); Reader tinyRatioRead;
    if (!tinyRatioInput.read(&tinyRatioRead,false) || tinyRatioRead.count!=1
        || tinyRatioRead.polyline.vertlist.size()!=2u
        || std::abs(tinyRatioRead.polyline.vertlist[0]->basePoint.z-expectedZ)>1.e-14) return false;
    // The full minor Z axis exceeds DBL_MAX, but a short partial arc samples
    // finite Z values. Multiply the sine before restoring its exponent.
    e.ratio=1.; e.staparam=.1; e.endparam=.2;
    direct.vertlist.clear(); e.toPolyline(&direct,8);
    const double partialZ=(1.5e308*std::sin(.1))*std::sqrt(2.);
    if (direct.vertlist.size()!=2u || !std::isfinite(direct.vertlist[0]->basePoint.z)
        || std::abs(direct.vertlist[0]->basePoint.z/partialZ-1.)>1.e-13
        || !output.write(&writer,DRW::AC1009,binary) || !writer.accepted) return false;
    dxfRW partialInput(path.string().c_str());Reader partialRead;
    if (!partialInput.read(&partialRead,false) || partialRead.count!=1
        || partialRead.polyline.vertlist.size()!=2u
        || std::abs(partialRead.polyline.vertlist[0]->basePoint.z/partialZ-1.)>1.e-13) return false;
    {std::ofstream existing(path,std::ios::binary|std::ios::trunc); existing<<"sentinel";}
    e.staparam=0.;e.endparam=turn;
    if (output.write(&writer,DRW::AC1009,binary) || writer.accepted || contents(path)!="sentinel") return false;
    e.staparam=.1;e.endparam=.2;
    if (!output.write(&writer,DRW::AC1009,binary) || !writer.accepted) return false;
    // An individually overflowing minor contribution could cancel against the
    // center to a finite coordinate. Until a robust shared-exponent sum is
    // qualified, reject rather than publish an infinity or a rounded guess.
    e.basePoint={0.,0.,-5.e307};e.staparam=1.2;e.endparam=1.3;
    const double finiteResult=(1.5*std::sqrt(2.)*std::sin(1.2)-.5)*1.e308;
    if (!std::isfinite(finiteResult)) return false;
    direct.vertlist.clear();e.toPolyline(&direct,8);
    {std::ofstream existing(path,std::ios::binary|std::ios::trunc); existing<<"sentinel";}
    if (!direct.vertlist.empty() || output.write(&writer,DRW::AC1009,binary)
        || writer.accepted || contents(path)!="sentinel") return false;
    e.ratio=.5;
    if (!output.write(&writer,DRW::AC1009,binary) || !writer.accepted) return false;
    {std::ofstream existing(path,std::ios::binary|std::ios::trunc); existing<<"sentinel";}
    e.basePoint={0.,0.,0.};e.extPoint={0.,0.,1.};e.staparam=.1;e.endparam=.2;
    e.ratio=2.; // major/minor swapping is a separate, not-yet-qualified path
    if (output.write(&writer,DRW::AC1009,binary) || writer.accepted || contents(path)!="sentinel") return false;
    e.ratio=.5;
    if (!output.write(&writer,DRW::AC1009,binary) || !writer.accepted) return false;
    // A component lost during normalization can reappear after scaling by a
    // huge axis. Refuse these mixed-scale cases rather than flattening them.
    const double tiny=std::numeric_limits<double>::denorm_min();
    e.secPoint={0.,max,0.};e.extPoint={tiny,0.,max};e.staparam=0.;e.endparam=turn/4.;
    direct.vertlist.clear();e.toPolyline(&direct,8);
    {std::ofstream existing(path,std::ios::binary|std::ios::trunc); existing<<"sentinel";}
    if (!direct.vertlist.empty() || output.write(&writer,DRW::AC1009,binary)
        || writer.accepted || contents(path)!="sentinel") return false;
    e.extPoint={0.,0.,1.};
    if (!output.write(&writer,DRW::AC1009,binary) || !writer.accepted) return false;
    e.secPoint={tiny,max,0.};
    direct.vertlist.clear();e.toPolyline(&direct,8);
    {std::ofstream existing(path,std::ios::binary|std::ios::trunc); existing<<"sentinel";}
    if (!direct.vertlist.empty() || output.write(&writer,DRW::AC1009,binary)
        || writer.accepted || contents(path)!="sentinel") return false;
    e.secPoint={0.,max,0.};
    if (!output.write(&writer,DRW::AC1009,binary) || !writer.accepted) return false;
    // The shared CIRCLE extrusion preflight is intentionally unchanged.
    DRW_Circle circle;circle.radious=1.;circle.extPoint={1.5e308,1.5e308,0.};
    CircleWriter circleWriter(output);circleWriter.circle=&circle;
    {std::ofstream existing(path,std::ios::binary|std::ios::trunc); existing<<"sentinel";}
    if (output.write(&circleWriter,DRW::AC1009,binary) || circleWriter.accepted || contents(path)!="sentinel") return false;
    circle.extPoint={0.,0.,1.};
    return output.write(&circleWriter,DRW::AC1009,binary) && circleWriter.accepted;
}
}
int main() {
    const auto path=std::filesystem::temp_directory_path() /
        ("ellipse-boundary-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dxf");
    struct Cleanup { std::filesystem::path path; ~Cleanup(){std::error_code e;std::filesystem::remove(path,e);} } cleanup{path};
    for (bool binary : {false,true}) for (bool threeD : {false,true}) for (bool opted : {false,true}) {
        struct Case {double start,end;bool closed;std::size_t legacy,opt;};
        const Case cases[] = {
            {.5,.5+5.e-11,false,2u,2u},
            {.5,.5-5.e-11,false,2u,129u},
            {turn-5.e-11,0.,false,129u,2u},
            {0.,turn-5.e-11,false,129u,129u},
            {.5,.5,true,128u,128u},
            {0.,turn,true,128u,128u},
            {turn,0.,true,128u,128u},
            {.5,.5+turn,true,128u,128u},
            {.5,.5-turn,true,128u,128u}
        };
        for (const Case& c : cases) if (!run(path,binary,threeD,opted,c.start,c.end,c.closed,opted?c.opt:c.legacy)) {
            std::cerr << "ellipse boundary failed binary="<<binary<<" 3d="<<threeD<<" opted="<<opted
                      <<" start="<<c.start<<" end="<<c.end<<'\n'; return 1;
        }
    }
    for (bool binary : {false,true}) if (!invalidRetry(path,binary)) return 1;
    for (bool binary : {false,true}) {
        for (bool threeD : {false,true}) for (double scale : {1.e-200,1.e200})
            for (double normalScale : {1.,scale}) for (bool closed : {false,true})
                if (!extreme(path,binary,threeD,scale,normalScale,closed)) {
                    std::cerr<<"extreme ellipse failed binary="<<binary<<" 3d="<<threeD
                             <<" scale="<<scale<<" normal="<<normalScale<<'\n';return 1;
                }
        for (bool threeD : {false,true}) for (double normalScale : {1.e-200,1.e200})
            if (!extreme(path,binary,threeD,1.,normalScale,false)) return 1;
        if (!extremeArithmetic(path,binary)) {std::cerr<<"extreme ellipse arithmetic failed\n";return 1;}
    }
    return 0;
}
